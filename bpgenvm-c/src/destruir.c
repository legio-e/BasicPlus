/* ============================================================
 * destruir.c — V7 #668 (8-oct): LIBERAR ES DESTRUIR.
 *
 * Eduardo: «Si liberamos el objeto, ha de ejecutar su destructor y si el destructor ha de liberar resources que lo
 * haga. Es la única manera que no queden resources perdidos o abiertos». Un objeto con destructor se destruye lo libere
 * quien lo libere: su `owner` (el compilador llama a Core.__destruye), el GC (no lo suelta: lo resucita y lo deja
 * pendiente) o el final del programa (todo lo vivo, el más nuevo primero, «sea de la forma que sea»: acabando bien, con
 * un error o con un KILL). Primero los hijos `owner`, después el objeto (la regla del 11-sep).
 *
 * El reparto (Eduardo: «La destrucción del objeto en realidad la hace en gran medida su propio destructor… la típica
 * división del trabajo de la POO»): cada clase suelta lo suyo en su destructor; el recorrido y las llamadas son BP
 * —Core.__destruye y Core.__destruyePendientes, el mismo bytecode en las dos VMs—, y aquí sólo están las piezas
 * pequeñas que BP no puede hacer: saber qué clase tiene destructor, el árbol de owner en el orden de destruir, la
 * memoria de un bloque, el registro de lo que tiene destructor, la cola de lo pendiente, meter la llamada a
 * __destruyePendientes en un hilo (como un evento, entre cuantos) y la fase del final. Los destructores corren como
 * código BP corriente —pueden esperar, bloquearse, tocar la GUI—, no llamados desde C (V7_IDEAS §«#666–#668», 8).
 *
 * Espejo del bloque «V7 #668» de VirtualMachine.java (miVM). La memoria: 8 B por objeto vivo con destructor (el
 * registro), más la cola y la marca por clase; nada si el programa no tiene ninguno.
 * ============================================================ */
#include "bpvm_internal.h"
#include "bpvm_alloc.h"
#include "bpvm_platform.h"
#include "bpvm_nativa.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* El tope de la destrucción del final: lo que no acabe en este tiempo se dice, y lo suelta la plataforma. */
#define TOPE_DESTRUCCION_MS 3000
/* Las rondas del final: lo que un destructor deje vivo con destructor, en la siguiente. */
#define RONDAS_MAX 8

/* La marca de una clase: 0, sin destructor propio; 1, el de una clase de Object; 2, el de un Thread. */
typedef struct { uint32_t desc; int32_t tipo; } marca_t;

struct bpvm_destruir {
    uint64_t* reg;        /* EL REGISTRO: los handles de lo que nació con destructor, en orden de creación. Los que
                           * mueren por otro camino se quedan hasta la siguiente purga (su generación ya no casa). */
    uint32_t  reg_n, reg_cap;
    uint64_t* pend;       /* LA COLA de lo pendiente de destruir, [pend_ini, pend_n): el siguiente, delante. Raíz. */
    uint32_t  pend_ini, pend_n, pend_cap;
    marca_t*  marcas;     /* la marca por clase, una vez por clase: direcciones abiertas */
    uint32_t  marcas_cap, marcas_n;
    int       vacios;     /* los destructores vacíos de Object y de Thread, ya mirados */
    uint32_t  vacio_obj_cs, vacio_thr_cs, desc_thread;
    int32_t   vacio_obj_off, vacio_thr_off;
    int32_t   drenador;   /* el tid + 1 del hilo en el que está metida __destruyePendientes; 0, ninguno */
    uint32_t  dir_pend;   /* Core.__destruyePendientes: 0, sin buscar; 1, no está (un Core de antes); si no, su pc */
    uint32_t  cs_pend;
    /* La fase del final. */
    int       fase;       /* 0, no ha empezado; 1, en curso; 2, hecha */
    int       matado;     /* el KILL de antes de la fase: es el que manda en el resultado */
    int       por_tope;   /* la cortó el tope (su KILL no es del usuario) */
    int       cortado;
    int       rondas;
    int64_t   fin_ms;
};

/* El estado, que nace con el primer objeto (un programa sin destructores no paga nada más). Con el cerrojo cogido. */
static struct bpvm_destruir* estado(bpvm_t* vm) {
    if (vm->destruir == NULL) vm->destruir = (struct bpvm_destruir*) bpvm_calloc(1, sizeof(struct bpvm_destruir));
    return vm->destruir;
}

/* ─── lo pequeño ─────────────────────────────────────────────────────────────────────────────────────────────────── */

/* La cabecera de un handle VIVO (su generación, en el heap, no libre), o 0. Sin tocar nada y sin avisar: un muerto
 * aquí es lo normal (bpvm_ref_dead lo denunciaría). */
static uint32_t cabecera_viva(const bpvm_t* vm, uint64_t v) {
    if (v == 0u) return 0u;
    if ((v & BPVM_HANDLE_TAG) != 0u) {
        uint32_t idx = (uint32_t) v & ~BPVM_HANDLE_TAG;
        if (vm->handles.gen == NULL || idx == 0u || idx >= vm->handles.next
                || vm->handles.gen[idx] != (uint32_t) (v >> 32)) return 0u;
    }
    bpref_t r; r.v = v;
    uint32_t a = bpref_deref(vm, r);
    if (a < vm->heap_start + 4u || a >= vm->heap_next) return 0u;
    uint32_t h = a - 4u;
    if (bpvm_read_u32_be(vm->memory + h) & BPVM_TAG_FREE_BIT) return 0u;
    return h;
}

static int tipo_de_bloque(const bpvm_t* vm, uint32_t h) {
    return (int) ((bpvm_read_u32_be(vm->memory + h) & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
}

/* La ranura de `__dtor` en la clase: por la subtabla CLASES (MOD9) o por su marcador `Clase#__dtor#n` (MOD7/8), como
 * slotOfMarker en miVM. -1 si no tiene (una tupla). */
static int ranura_dtor(const bpvm_t* vm, uint32_t desc) {
    int s = bpvm_link_slot_por_nombre(vm, desc, "__dtor");
    if (s >= 0) return s;
    for (int i = 0; i < vm->symbol_count; i++) {
        if (vm->symbols[i].abs_addr != desc) continue;
        const char* nm = bpvm_symbol_name(vm, i);
        const char* h = strchr(nm, '#');
        if (h != NULL && strncmp(h + 1, "__dtor#", 7) == 0) return atoi(h + 8);
    }
    return -1;
}

static void asegura_vacios(bpvm_t* vm, struct bpvm_destruir* d) {
    if (d->vacios) return;
    d->vacios = 1;
    d->vacio_obj_off = d->vacio_thr_off = -1;
    uint32_t o = bpvm_link_lookup(vm, "Core.Object");
    int s;
    if (o != 0u && (s = ranura_dtor(vm, o)) >= 0)
        (void) bpvm_vt_resolver(vm, o, (uint32_t) s, &d->vacio_obj_off, &d->vacio_obj_cs);
    uint32_t t = bpvm_link_lookup(vm, "Core.Thread");
    d->desc_thread = t;
    if (t != 0u && (s = ranura_dtor(vm, t)) >= 0)
        (void) bpvm_vt_resolver(vm, t, (uint32_t) s, &d->vacio_thr_off, &d->vacio_thr_cs);
}

/* La marca de la clase, calculada: su destructor, por el despacho virtual, frente a los vacíos de las dos raíces. */
static int tipo_calculado(bpvm_t* vm, struct bpvm_destruir* d, uint32_t desc) {
    int s = ranura_dtor(vm, desc);
    if (s < 0) return 0;
    int32_t off; uint32_t cs;
    if (bpvm_vt_resolver(vm, desc, (uint32_t) s, &off, &cs) != 0) return 0;
    asegura_vacios(vm, d);
    if ((d->vacio_obj_off != -1 && cs == d->vacio_obj_cs && off == d->vacio_obj_off)
            || (d->vacio_thr_off != -1 && cs == d->vacio_thr_cs && off == d->vacio_thr_off)) return 0;
    return (d->desc_thread != 0u && bpvm_clase_desciende(vm, desc, d->desc_thread)) ? 2 : 1;
}

/* La marca de una clase, una vez por clase (direcciones abiertas, sin borrar nunca). Con el cerrojo, o con un run. */
static int tipo_de_clase(bpvm_t* vm, struct bpvm_destruir* d, uint32_t desc) {
    if (desc == 0u) return 0;
    if (d->marcas_cap != 0u) {
        uint32_t m = d->marcas_cap - 1u;
        for (uint32_t i = (desc >> 2) & m; d->marcas[i].desc != 0u; i = (i + 1u) & m)
            if (d->marcas[i].desc == desc) return (int) d->marcas[i].tipo;
    }
    int t = tipo_calculado(vm, d, desc);
    if ((d->marcas_n + 1u) * 2u > d->marcas_cap) {   /* a la mitad, el doble */
        uint32_t cap = d->marcas_cap ? d->marcas_cap * 2u : 32u;
        marca_t* nueva = (marca_t*) bpvm_calloc(cap, sizeof(marca_t));
        if (nueva == NULL) return t;                  /* sin memoria: se calcula cada vez, sin apuntarla */
        for (uint32_t k = 0; k < d->marcas_cap; k++) {
            if (d->marcas[k].desc == 0u) continue;
            uint32_t i = (d->marcas[k].desc >> 2) & (cap - 1u);
            while (nueva[i].desc != 0u) i = (i + 1u) & (cap - 1u);
            nueva[i] = d->marcas[k];
        }
        bpvm_free(d->marcas);
        d->marcas = nueva;
        d->marcas_cap = cap;
    }
    uint32_t i = (desc >> 2) & (d->marcas_cap - 1u);
    while (d->marcas[i].desc != 0u) i = (i + 1u) & (d->marcas_cap - 1u);
    d->marcas[i].desc = desc;
    d->marcas[i].tipo = t;
    d->marcas_n++;
    return t;
}

static int tipo_de_cabecera(bpvm_t* vm, struct bpvm_destruir* d, uint32_t h) {
    if (tipo_de_bloque(vm, h) != BPVM_TYPE_OBJECT) return 0;
    return tipo_de_clase(vm, d, bpvm_read_u32_be(vm->memory + h + 4u));
}

/* ─── el árbol de owner ──────────────────────────────────────────────────────────────────────────────────────────── */

/* Un vector que crece y un conjunto de cabeceras vistas (contra un ciclo o un hijo con dos dueños), para los
 * recorridos de abajo. Temporales: se sueltan al acabar el recorrido. */
typedef struct { uint64_t* v; uint32_t n, cap; int sin_memoria; } vec_t;
static void vec_mete(vec_t* a, uint64_t x) {
    if (a->n == a->cap) {
        uint32_t cap = a->cap ? a->cap * 2u : 16u;
        uint64_t* q = (uint64_t*) bpvm_realloc(a->v, (size_t) cap * sizeof(uint64_t));
        if (q == NULL) { a->sin_memoria = 1; return; }
        a->v = q;
        a->cap = cap;
    }
    a->v[a->n++] = x;
}
typedef struct { uint32_t* t; uint32_t n, cap; int sin_memoria; } vistos_t;
/* 1 si `h` es nueva (y queda apuntada); 0 si ya estaba. */
static int vistos_mete(vistos_t* s, uint32_t h) {
    if ((s->n + 1u) * 2u > s->cap) {
        uint32_t cap = s->cap ? s->cap * 2u : 32u;
        uint32_t* nt = (uint32_t*) bpvm_calloc(cap, sizeof(uint32_t));
        if (nt == NULL) { s->sin_memoria = 1; return 1; }
        for (uint32_t k = 0; k < s->cap; k++) {
            if (s->t[k] == 0u) continue;
            uint32_t i = (s->t[k] >> 2) & (cap - 1u);
            while (nt[i] != 0u) i = (i + 1u) & (cap - 1u);
            nt[i] = s->t[k];
        }
        bpvm_free(s->t);
        s->t = nt;
        s->cap = cap;
    }
    uint32_t m = s->cap - 1u, i = (h >> 2) & m;
    for (; s->t[i] != 0u; i = (i + 1u) & m) if (s->t[i] == h) return 0;
    s->t[i] = h;
    s->n++;
    return 1;
}

/* Los hijos owner VIVOS de un bloque vivo, en orden de declaración y sin tocarlos: los campos owner de un objeto (su
 * descriptor) o los elementos de un array de refs poseído. Como soltar_uno (heap.c), sin matar a nadie. */
static void hijos_owner(const bpvm_t* vm, uint32_t h, vec_t* out) {
    int type = tipo_de_bloque(vm, h);
    if (type == BPVM_TYPE_OBJECT) {
        uint32_t cls  = bpvm_read_u32_be(vm->memory + h + 4u);
        uint32_t nf   = bpvm_read_u16_be(vm->memory + cls + BPVM_CLS_OFF_NUM_FIELDS);
        uint32_t bw   = bpvm_read_u16_be(vm->memory + cls + BPVM_CLS_OFF_BITMAP_WORDS);
        uint32_t base = cls + BPVM_CLS_OFF_FIELD_BITMAP + bw * 4u;
        for (uint32_t i = 0; i < nf; i++) {
            uint32_t word = bpvm_read_u32_be(vm->memory + base + (i >> 5) * 4u);
            if (((word >> (i & 31)) & 1u) == 0u) continue;
            uint64_t r = bpref_load(vm, h + BPVM_OBJ_HEADER_SIZE + i * 4u).v;
            if (cabecera_viva(vm, r) != 0u) vec_mete(out, r);
        }
    } else if (type == BPVM_TYPE_ARRAY_REF) {
        uint32_t len = bpvm_read_u32_be(vm->memory + h + 4u);
        for (uint32_t i = 0; i < len; i++) {
            uint64_t r = bpref_load(vm, h + 4u + BPVM_ARR_DATA_OFF + i * BPVM_REF_SIZE).v;
            if (cabecera_viva(vm, r) != 0u) vec_mete(out, r);
        }
    }
}

/* ¿Algún destructor en el árbol de owner de `r`, él incluido? Sin tocar nada. Sin memoria para recorrerlo: sí (lo
 * prudente: se ordena y se destruye, en vez de soltarlo sin destructores). */
static int arbol_con_destructores(bpvm_t* vm, struct bpvm_destruir* d, uint64_t r) {
    uint32_t h0 = cabecera_viva(vm, r);
    if (h0 == 0u) return 0;
    vec_t pila = {0}, hijos = {0};
    vistos_t vistos = {0};
    int hay = 0;
    vec_mete(&pila, r);
    vistos_mete(&vistos, h0);
    while (pila.n > 0u && !hay) {
        uint32_t h = cabecera_viva(vm, pila.v[--pila.n]);
        if (h == 0u) continue;
        if (tipo_de_cabecera(vm, d, h) != 0) { hay = 1; break; }
        hijos.n = 0;
        hijos_owner(vm, h, &hijos);
        for (uint32_t k = 0; k < hijos.n; k++)
            if (vistos_mete(&vistos, cabecera_viva(vm, hijos.v[k]))) vec_mete(&pila, hijos.v[k]);
        if (pila.sin_memoria || hijos.sin_memoria || vistos.sin_memoria) { hay = 1; break; }
    }
    bpvm_free(pila.v); bpvm_free(hijos.v); bpvm_free(vistos.t);
    return hay;
}

/* El árbol de owner de `r` en el orden de destruir: los hijos antes que su padre y, entre hermanos, del último al
 * primero (Eduardo: «de los hijos hasta los padres, en orden inverso»); `r`, el último. Es el preorden con los hijos
 * en orden de declaración, dado la vuelta. En `out` (temporal: lo suelta quien llama). 0, o -1 sin memoria. */
static int orden_destruccion(bpvm_t* vm, uint64_t r, vec_t* out) {
    uint32_t h0 = cabecera_viva(vm, r);
    if (h0 == 0u) return 0;
    vec_t pila = {0}, hijos = {0};
    vistos_t vistos = {0};
    vec_mete(&pila, r);
    vistos_mete(&vistos, h0);
    while (pila.n > 0u) {
        uint64_t x = pila.v[--pila.n];
        vec_mete(out, x);
        hijos.n = 0;
        hijos_owner(vm, cabecera_viva(vm, x), &hijos);
        for (uint32_t k = hijos.n; k-- > 0u; )          /* a la pila al revés: salen en orden de declaración */
            if (vistos_mete(&vistos, cabecera_viva(vm, hijos.v[k]))) vec_mete(&pila, hijos.v[k]);
        if (pila.sin_memoria || hijos.sin_memoria || vistos.sin_memoria || out->sin_memoria) break;
    }
    int mal = pila.sin_memoria || hijos.sin_memoria || vistos.sin_memoria || out->sin_memoria;
    bpvm_free(pila.v); bpvm_free(hijos.v); bpvm_free(vistos.t);
    if (mal) return -1;
    for (uint32_t i = 0, j = out->n; i + 1u < j; i++, j--) {   /* dado la vuelta */
        uint64_t t = out->v[i]; out->v[i] = out->v[j - 1u]; out->v[j - 1u] = t;
    }
    return 0;
}

/* ─── las primitivas (builtins.c las llama, con la pila de BP) ───────────────────────────────────────────────────── */

int bpvm_destruir_libera_si_sin(bpvm_t* vm, bpref_t r) {
    bpvm_smp_lock(vm);
    struct bpvm_destruir* d = estado(vm);
    /* (sin memoria ni para la marca: se suelta como antes, sin destructores) */
    int hay = d != NULL && cabecera_viva(vm, r.v) != 0u && arbol_con_destructores(vm, d, r.v);
    bpvm_smp_unlock(vm);
    if (hay) return 0;
    bpvm_heap_soltar_owned(vm, r);   /* toma él los cerrojos que necesita */
    return 1;
}

int bpvm_destruir_orden(bpvm_t* vm, bpref_t r, uint64_t** out, uint32_t* n) {
    vec_t v = {0};
    *out = NULL;
    *n = 0;
    bpvm_smp_lock(vm);
    int rc = orden_destruccion(vm, r.v, &v);
    bpvm_smp_unlock(vm);
    if (rc != 0) { bpvm_free(v.v); return -1; }
    *out = v.v;
    *n = v.n;
    return 0;
}

int bpvm_destruir_prepara(bpvm_t* vm, bpref_t r) {
    bpvm_smp_lock(vm);
    struct bpvm_destruir* d = estado(vm);
    if (d == NULL) { bpvm_smp_unlock(vm); return 0; }
    int t = 0;
    uint32_t h = cabecera_viva(vm, r.v);
    if (h != 0u) t = tipo_de_cabecera(vm, d, h);
    if (t != 0) {
        /* En su destructor ya no tiene a sus hijos owner: se destruyeron antes que él (la regla del 11-sep), y un
         * handle muerto en el campo diría «no es null» y usarlo gritaría. A null; los que siguen vivos, no se tocan. */
        uint32_t cls  = bpvm_read_u32_be(vm->memory + h + 4u);
        uint32_t nf   = bpvm_read_u16_be(vm->memory + cls + BPVM_CLS_OFF_NUM_FIELDS);
        uint32_t bw   = bpvm_read_u16_be(vm->memory + cls + BPVM_CLS_OFF_BITMAP_WORDS);
        uint32_t base = cls + BPVM_CLS_OFF_FIELD_BITMAP + bw * 4u;
        for (uint32_t i = 0; i < nf; i++) {
            uint32_t word = bpvm_read_u32_be(vm->memory + base + (i >> 5) * 4u);
            if (((word >> (i & 31)) & 1u) == 0u) continue;
            uint32_t at = h + BPVM_OBJ_HEADER_SIZE + i * 4u;
            uint64_t x = bpref_load(vm, at).v;
            if (x != 0u && cabecera_viva(vm, x) == 0u) bpref_store(vm, at, bpref_null());
        }
    }
    bpvm_smp_unlock(vm);
    return t;
}

void bpvm_destruir_nombre(bpvm_t* vm, bpref_t r, char* out, size_t cap) {
    snprintf(out, cap, "?");
    uint32_t h = cabecera_viva(vm, r.v);
    if (h == 0u || tipo_de_bloque(vm, h) != BPVM_TYPE_OBJECT) return;
    uint32_t cls = bpvm_read_u32_be(vm->memory + h + 4u);
    if (bpvm_link_nombre_de_clase(vm, cls, out, cap)) return;
    for (int i = 0; i < vm->symbol_count; i++) {   /* MOD7/8: el data-symbol `Mod.Cls`, como nombreDeClase en miVM */
        if (vm->symbols[i].abs_addr != cls) continue;
        const char* nm = bpvm_symbol_name(vm, i);
        if (strchr(nm, '#') || strchr(nm, '$') || strchr(nm, '(') || !strchr(nm, '.')) continue;
        snprintf(out, cap, "%s", nm);
        return;
    }
}

uint64_t bpvm_destruir_siguiente(bpvm_t* vm, bpvm_thread_t* tc) {
    struct bpvm_destruir* d = vm->destruir;
    if (d == NULL) return 0u;
    uint64_t r = 0u;
    bpvm_smp_lock(vm);
    while (d->pend_ini < d->pend_n) {
        uint64_t x = d->pend[d->pend_ini++];
        if (cabecera_viva(vm, x) != 0u) { r = x; break; }
    }
    if (d->pend_ini >= d->pend_n) {
        d->pend_ini = d->pend_n = 0u;
        if (r == 0u && d->drenador == tc->id + 1) d->drenador = 0;   /* vacía: el que la vaciaba deja de hacerlo */
    }
    bpvm_smp_unlock(vm);
    return r;
}

/* ─── el registro y la cola ──────────────────────────────────────────────────────────────────────────────────────── */

/* Fuera los muertos, conservando el orden. */
static void registro_purga(const bpvm_t* vm, struct bpvm_destruir* d) {
    uint32_t w = 0;
    for (uint32_t k = 0; k < d->reg_n; k++) if (cabecera_viva(vm, d->reg[k]) != 0u) d->reg[w++] = d->reg[k];
    d->reg_n = w;
}

static int pend_mete(struct bpvm_destruir* d, uint64_t x) {
    if (d->pend_n == d->pend_cap) {
        if (d->pend_ini > 0u) {                       /* primero, lo ya sacado fuera */
            memmove(d->pend, d->pend + d->pend_ini, (size_t) (d->pend_n - d->pend_ini) * sizeof(uint64_t));
            d->pend_n -= d->pend_ini;
            d->pend_ini = 0u;
        } else {
            uint32_t cap = d->pend_cap ? d->pend_cap * 2u : 16u;
            uint64_t* q = (uint64_t*) bpvm_realloc(d->pend, (size_t) cap * sizeof(uint64_t));
            if (q == NULL) return 0;
            d->pend = q;
            d->pend_cap = cap;
        }
    }
    d->pend[d->pend_n++] = x;
    return 1;
}

void bpvm_destruir_al_nacer(bpvm_t* vm, bpref_t h, uint32_t class_ptr) {
    bpvm_smp_lock(vm);   /* el estado nace aquí, con el cerrojo: con dos runs, dos NEW_OBJECT pueden llegar a la vez */
    struct bpvm_destruir* d = estado(vm);
    if (d != NULL && tipo_de_clase(vm, d, class_ptr) != 0) {
        if (d->reg_n == d->reg_cap) registro_purga(vm, d);
        if (d->reg_n == d->reg_cap) {
            uint32_t cap = d->reg_cap ? d->reg_cap * 2u : 16u;
            uint64_t* q = (uint64_t*) bpvm_realloc(d->reg, (size_t) cap * sizeof(uint64_t));
            if (q != NULL) { d->reg = q; d->reg_cap = cap; }
        }
        if (d->reg_n < d->reg_cap) d->reg[d->reg_n++] = h.v;
        else bpvm_diag_urgente("[bpvm] sin memoria para el registro de destructores: un objeto con destructor no "
                               "se destruira al soltarlo el GC ni al acabar (#668)");
    }
    bpvm_smp_unlock(vm);
}

void bpvm_destruir_en_el_gc(bpvm_t* vm, void (*marca)(bpvm_t*, uint32_t)) {
    struct bpvm_destruir* d = vm->destruir;
    if (d == NULL) return;
    /* Lo pendiente sigue vivo hasta que se destruye. */
    for (uint32_t k = d->pend_ini; k < d->pend_n; k++)
        if (cabecera_viva(vm, d->pend[k]) != 0u) marca(vm, (uint32_t) d->pend[k]);
    /* Lo que tiene destructor y ha quedado sin marcar NO se suelta: a pendientes, el más nuevo primero, y se resucita
     * —con lo que alcanza, para que su destructor encuentre vivo lo que usa—. TODOS los sin marcar se apartan ANTES de
     * resucitar a ninguno: si no, el hijo de uno resucitado parecería alcanzable y se quedaría en el registro. */
    uint32_t w = 0, desde = d->pend_n;
    for (uint32_t k = 0; k < d->reg_n; k++) {
        uint64_t x = d->reg[k];
        uint32_t h = cabecera_viva(vm, x);
        if (h == 0u) continue;                                        /* ya lo soltó otro camino */
        if ((bpvm_read_u32_be(vm->memory + h) & BPVM_TAG_MARK_BIT) == 0u && pend_mete(d, x)) continue;
        d->reg[w++] = x;                                               /* alcanzable (o sin sitio en la cola: el siguiente) */
    }
    d->reg_n = w;
    for (uint32_t i = desde, j = d->pend_n; i + 1u < j; i++, j--) {   /* el más nuevo primero */
        uint64_t t = d->pend[i]; d->pend[i] = d->pend[j - 1u]; d->pend[j - 1u] = t;
    }
    for (uint32_t k = desde; k < d->pend_n; k++) marca(vm, (uint32_t) d->pend[k]);
    /* Y los que no cupieron en la cola siguen en el registro: también vivos, para la siguiente colecta. */
    for (uint32_t k = 0; k < d->reg_n; k++) {
        uint32_t h = cabecera_viva(vm, d->reg[k]);
        if (h != 0u && (bpvm_read_u32_be(vm->memory + h) & BPVM_TAG_MARK_BIT) == 0u) marca(vm, (uint32_t) d->reg[k]);
    }
}

/* ─── la llamada a Core.__destruyePendientes, metida en un hilo ──────────────────────────────────────────────────── */

static int resuelve(bpvm_t* vm, struct bpvm_destruir* d) {
    if (d->dir_pend == 0u) {
        uint32_t a = bpvm_link_lookup(vm, "Core.__destruyePendientes");
        const bpvm_module_t* m = (a != 0u) ? bpvm_module_for_code_addr(vm, a) : NULL;
        if (m == NULL) { d->dir_pend = 1u; return 0; }   /* un Core de antes de #668 */
        d->dir_pend = a;
        d->cs_pend = m->code_start;
    }
    return d->dir_pend != 1u;
}

/* El marco de una llamada a una FUNCIÓN de módulo sin argumentos en la pila de `tc`: como inject (events.c) —el PC de
 * reanudación debajo; vuelve por el sentinela EVENT_RETURN; al fin de hilo si lo habían resucitado—. */
static int inyecta_funcion(bpvm_t* vm, bpvm_thread_t* tc, uint32_t dir, uint32_t cs) {
    if (tc->sp + 16u > tc->stack_top) {
        bpvm_diag("[bpvm] destruir: sin pila en tid=%d — se deja para luego", (int) tc->id);
        return 0;
    }
    uint32_t sp = tc->sp;
    uint32_t resume_pc = (tc->ev_post_mortem >= 0) ? BPVM_SENTINEL_THREAD_EXIT_ADDR : tc->pc;
    bpvm_write_i32_be(vm->memory + sp, (int32_t) resume_pc); sp += 4;
    bpvm_write_i32_be(vm->memory + sp, (int32_t) BPVM_SENTINEL_EVENT_RETURN_ADDR); sp += 4;
    bpvm_write_i32_be(vm->memory + sp, (int32_t) tc->bp); sp += 4;
    bpvm_write_i32_be(vm->memory + sp, (int32_t) tc->cs); sp += 4;
    tc->sp = sp;
    tc->bp = sp;
    tc->cs = cs;
    tc->ev_depth++;
    tc->pc = dir;
    return 1;
}

void bpvm_destruir_inyecta(bpvm_t* vm, bpvm_thread_t* tc) {
    struct bpvm_destruir* d = vm->destruir;
    if (d == NULL || d->pend_ini >= d->pend_n || d->drenador != 0 || tc->ev_depth > 0) return;
    if (bpvm_nativa_suspendida(tc)) return;   /* dentro de una native: su pila de BP no es suya ahora */
    if (!resuelve(vm, d)) return;
    if (inyecta_funcion(vm, tc, d->dir_pend, d->cs_pend)) d->drenador = tc->id + 1;
}

/* ─── la fase del final ──────────────────────────────────────────────────────────────────────────────────────────── */

int bpvm_destruir_en_fase_final(const bpvm_t* vm) {
    return vm->destruir != NULL && vm->destruir->fase == 1;
}

static uint32_t cuenta_vivos(const bpvm_t* vm, const struct bpvm_destruir* d, uint64_t* uno) {
    uint32_t n = 0;
    *uno = 0u;
    for (uint32_t k = d->pend_ini; k < d->pend_n; k++)
        if (cabecera_viva(vm, d->pend[k]) != 0u) { if (n++ == 0u) *uno = d->pend[k]; }
    for (uint32_t k = 0; k < d->reg_n; k++)
        if (cabecera_viva(vm, d->reg[k]) != 0u) { if (n++ == 0u) *uno = d->reg[k]; }
    return n;
}

static int termina(bpvm_t* vm, struct bpvm_destruir* d) {
    uint64_t uno;
    uint32_t n = cuenta_vivos(vm, d, &uno);
    if (n > 0u) {
        char clase[160]; bpref_t r; r.v = uno;
        bpvm_destruir_nombre(vm, r, clase, sizeof clase);
        bpvm_diag_urgente("[bpvm] fin de RUN: quedaron sin destruir %u objeto(s) con destructor (el primero, de %s)",
                          (unsigned) n, clase);
    }
    d->reg_n = 0u;
    d->pend_ini = d->pend_n = 0u;
    d->drenador = 0;
    d->fase = 2;
    /* El resultado del programa: el KILL de antes, o uno del usuario durante la fase (el del tope no es suyo). */
    bpvm_kill_marcar(vm, d->matado || (bpvm_kill_pedido(vm) && !d->por_tope));
    return 0;
}

void bpvm_destruir_vigila(bpvm_t* vm) {
    struct bpvm_destruir* d = vm->destruir;
    if (d == NULL || d->fase != 1 || d->cortado || bpvm_platform_now_ms() < d->fin_ms) return;
    bpvm_diag_urgente("[bpvm] fin de RUN: la destruccion no ha acabado en %d ms: se corta (lo que quede lo suelta "
                      "la plataforma)", TOPE_DESTRUCCION_MS);
    d->cortado = 1;
    d->por_tope = 1;
    bpvm_kill_marcar(vm, 1);
}

int bpvm_destruir_fase_final(bpvm_t* vm) {
    struct bpvm_destruir* d = vm->destruir;
    if (d == NULL || d->fase == 2) return 0;
    bpvm_thread_t* main = vm->hilo_main;
    if (d->fase == 0) {
        uint64_t uno;
        uint32_t n = cuenta_vivos(vm, d, &uno);
        if (n == 0u) { d->fase = 2; return 0; }
        const char* sin = NULL;
        if (vm->gc_descarrilo)                        sin = "el heap esta descarrilado (lo suelta la plataforma)";
        else if (main == NULL || bpvm_nativa_suspendida(main))
                                                      sin = "el main esta dentro de una native (lo suelta la plataforma)";
        else if (!resuelve(vm, d))                    sin = "el Core no tiene __destruyePendientes (es de antes de #668)";
        if (sin != NULL) {
            bpvm_diag_urgente("[bpvm] fin de RUN: %u objeto(s) con destructor sin destruir: %s", (unsigned) n, sin);
            d->reg_n = 0u;
            d->pend_ini = d->pend_n = 0u;
            d->fase = 2;
            return 0;
        }
        d->fase = 1;
        d->matado = bpvm_kill_pedido(vm);
        d->fin_ms = bpvm_platform_now_ms() + TOPE_DESTRUCCION_MS;
    } else if (bpvm_kill_pedido(vm) || main->status != BPVM_THREAD_TERMINATED || main->ev_depth != 0) {
        d->cortado = 1;   /* la ronda no acabó sola: un KILL, el tope, un fallo o un interbloqueo */
    }
    if (d->cortado || d->rondas >= RONDAS_MAX) return termina(vm, d);
    /* Lo vivo del registro, a pendientes: el más nuevo primero. */
    for (uint32_t k = d->reg_n; k-- > 0u; )
        if (cabecera_viva(vm, d->reg[k]) != 0u) (void) pend_mete(d, d->reg[k]);
    d->reg_n = 0u;
    if (d->pend_ini >= d->pend_n) return termina(vm, d);
    /* Los demás hilos, abandonados (sin deudas de eventos: no resucitan); los mutex, todos libres y sin nadie esperando
     * —sus dueños se han ido, y los destructores pueden necesitarlos—. */
    for (bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h)) {
        if (h == main) continue;
        h->status = BPVM_THREAD_TERMINATED;
        h->ev_post_mortem = 0;
        h->blocked_on_mutex = -1;
        h->blocked_on_join = -1;
    }
    for (int i = 0; i < vm->mutex_count; i++) {
        vm->mutexes[i].owner_tid = -1;
        vm->mutexes[i].waiter_count = 0;
    }
    /* El main, con la pila limpia: al volver de la llamada, al fin de hilo; sin deudas de eventos. */
    main->sp = main->stack_base;
    main->bp = main->stack_base;
    main->eh_stack_size = 0;
    main->ev_depth = 0;
    main->ev_post_mortem = 0;
    main->alloc_anchor = 0;
    main->blocked_on_mutex = -1;
    main->blocked_on_join = -1;
    main->builtin_de_nativa = 0;
    main->excepcion_pendiente = 0u;
    main->al_run0 = 0;
    main->pc = BPVM_SENTINEL_THREAD_EXIT_ADDR;
    if (!inyecta_funcion(vm, main, d->dir_pend, d->cs_pend)) return termina(vm, d);
    d->drenador = main->id + 1;
    main->status = BPVM_THREAD_RUNNABLE;
    bpvm_kill_marcar(vm, 0);
    bpvm_la_cola.parar = 0;
    d->rondas++;
    return 1;
}

void bpvm_destruir_liberar(bpvm_t* vm) {
    struct bpvm_destruir* d = vm->destruir;
    if (d == NULL) return;
    bpvm_free(d->reg);
    bpvm_free(d->pend);
    bpvm_free(d->marcas);
    bpvm_free(d);
    vm->destruir = NULL;
}
