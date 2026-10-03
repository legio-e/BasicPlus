/*
 * link.c — linker dinámico tras cargar todos los módulos (F3).
 *
 * Responsabilidades:
 *   1. Lookup de un símbolo cualificado en la global symbol table.
 *   2. Helpers de mapeo addr → módulo (getCSForDataAddr y similares).
 *   3. bpvm_link_all() — para cada módulo, resuelve las entradas de su
 *      ext-table escribiéndolas en memory[]. Aplica también los class
 *      fixups L2 v3 (parche de parent_offset cross-module).
 *
 * F3 v1 — la global symbol table vive como una lista plana de pares
 * (name, abs_addr) construida durante el load. Lookup linear O(N);
 * suficiente para ~100 símbolos. Si crece, hashmap.
 */

#include "bpvm_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */
#include "bpvm_platform.h" /* V7/H2 #523 (V0): el cronometro del enlace */

/* V7/H2 (#523, V0) — LA LINEA BASE del enlace: cuantas busquedas por nombre hace
 * un enlace y cuantos strcmp cuestan, para comparar antes/despues del enlace
 * tardio con numero y no con impresion. Estaticos: no tocan bpvm_t (el .mdn
 * depende de su layout). Se ponen a cero al empezar bpvm_link_all. */
static unsigned s_link_busquedas, s_link_strcmp;

/* ---- Helpers de mapeo addr → módulo ---- */

uint32_t bpvm_get_cs_for_data_addr(const bpvm_t* vm, uint32_t addr) {
    /* V7/H2 (#523) — el sello del dueño (+6 = indice + 1) que el enlazador escribe en
     * todo descriptor MOD9, cerrado o CONSTRUIDO: un descriptor construido vive en la
     * arena, fuera de todo modulo, y sin el sello no tendria CS. 0 = MOD7/8: por rango. */
    {
        uint16_t sello = bpvm_read_u16_be(vm->memory + addr + BPVM_CLS_OFF_MODULO);
        if (sello > 0 && sello <= (uint16_t) vm->module_count)
            return vm->modules[sello - 1].code_start;
    }
    /* Busca el módulo cuyo rango RAM [module_base, end_addr) contiene addr.
     * Devolvemos code_start (= CS del módulo).
     *
     * NOTA: para class descriptors, addr cae en [data_start, code_start).
     * Para code addrs de módulos RAM, en [code_start, end_addr) (la rama
     * amplia los cubre). H3.c: en módulos XIP end_addr == code_start (el
     * código no ocupa RAM) → fallback por rango de CÓDIGO [cb, cb+size). */
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (addr >= m->module_base && addr < m->end_addr) {
            return m->code_start;
        }
    }
    {
        const bpvm_module_t* m = bpvm_module_for_code_addr(vm, addr);
        if (m) return m->code_start;
    }
    return 0;
}

const bpvm_module_t* bpvm_module_for_code_addr(const bpvm_t* vm, uint32_t addr) {
    /* Rango de CÓDIGO = [cb, cb+code_size): módulos RAM tienen cb==code_start
     * (idéntico a antes); módulos XIP tienen cb en la región de packs. */
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (addr >= m->cb && addr - m->cb < m->code_size) {
            return m;
        }
    }
    return NULL;
}

uint32_t bpvm_get_cs_for_code_addr(const bpvm_t* vm, uint32_t code_addr) {
    /* Si code_addr está en el rango de código de un módulo, devuelve su CS. */
    const bpvm_module_t* m = bpvm_module_for_code_addr(vm, code_addr);
    return m ? m->code_start : 0;
}

/* H3.c — cb del módulo cuyo CS es `cs`, con caché de 1 entrada en el tc
 * (por-thread = SMP-safe). Para módulos RAM cb == cs, así que el fallback
 * identidad conserva el comportamiento de siempre. Transitoria hasta que el
 * CALL local sea PC-relativo (#307) — entonces esto se retira. */
uint32_t bpvm_cb_for_cs(const bpvm_t* vm, bpvm_thread_t* tc, uint32_t cs) {
    if (tc && tc->cb_cache_cs == cs && cs != 0) return tc->cb_cache_cb;
    uint32_t cb = cs;
    for (int i = 0; i < vm->module_count; i++) {
        if (vm->modules[i].code_start == cs) { cb = vm->modules[i].cb; break; }
    }
    if (tc) { tc->cb_cache_cs = cs; tc->cb_cache_cb = cb; }
    return cb;
}

uint32_t bpvm_get_ext_table_addr(const bpvm_t* vm, uint32_t cs) {
    /* Dado el CS de un módulo, devuelve su ext_table_addr (= module_base). */
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (m->code_start == cs) return m->ext_table_addr;
    }
    return 0;
}

/* ---- Global symbol table ----
 *
 * Dos piezas: `vm->symbols` (array de registros de 8 B) y `vm->sym_pool` (las
 * cadenas, seguidas). Los valores son direcciones absolutas en memory[].
 *
 * #449 — POR QUE EL POOL, y no un `char name[N]` mas corto. La medida del
 * 29-ago dice que los nombres reales son de 24 de media y 35 el mayor, asi que
 * un `name[40]` habria entrado igual de bien en memoria. Pero un array fijo
 * TRUNCA, y dos nombres que compartan prefijo y se pasen del tamaño se
 * convierten en el MISMO simbolo: el enlace resolveria a la funcion equivocada
 * en silencio. Eso es peor que quedarse sin memoria, que al menos se ve. El
 * pool no trunca nunca y ademas cuesta menos. */

/* Nombre del simbolo i. ⚠️ NO guardar el puntero a traves de un registro nuevo:
 * el pool se realoja al crecer y el puntero quedaria colgado. */
const char* bpvm_symbol_name(const bpvm_t* vm, int i) {
    if (!vm->sym_pool || i < 0 || i >= vm->symbol_count) return "";
    return vm->sym_pool + vm->symbols[i].name_off;
}

/* #639 (3-oct) — CRECER SIN PEDIR DE MAS. Las dos piezas de la tabla crecian siempre al DOBLE, y el doble de un bloque
 * grande es un bloque grande: en la C6 con la GUI habia 16 KB de nombres y el simbolo siguiente pedia 32 KB de un trozo a
 * un `malloc` con 12 KB libres (el mayor, de 7,9). Eduardo (3-oct): «que crezca menos cuando no hay memoria; en vez de
 * pasar de 8K a 16K, que pase a 10 o 12K». Asi que el doble si cabe; si no, x1,5; si no, x1,25; y si tampoco, lo justo.
 * Cada intento es un `realloc`, que si falla deja el bloque de antes como estaba. Se cuenta en elementos de `tam` bytes
 * (1 para el pool, un registro para el array). Devuelve la capacidad nueva, o 0 si ni lo justo cabe. */
static uint32_t tabla_crecer(void** p, uint32_t cap, uint32_t doble, uint32_t justo, size_t tam) {
    const uint32_t intentos[4] = { doble, cap + cap / 2u, cap + cap / 4u, justo };
    uint32_t anterior = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t u = intentos[i] < justo ? justo : intentos[i];
        if (anterior != 0 && u >= anterior) continue;      /* solo hacia abajo: ese tamaño ya no cupo */
        anterior = u;
        void* q = bpvm_realloc(*p, (size_t) u * tam);
        if (q) { *p = q; return u; }
    }
    return 0;
}

/* #639 — y si ni lo justo cabe, se APUNTA: cuántos no entraron y el primero, con lo que ocupaba la tabla, y se dice en
 * el log (una vez). La carga SIGUE: un programa que no use esos símbolos corre —así corría GuiShot en la C6, con los
 * últimos de Str perdidos sin saberlo—; y si el enlace busca uno que no está, su mensaje da esta causa
 * (bpvm_link_set_error, lt_error). Antes se tiraba en silencio —los seis sitios que registran no miraban esto— y el
 * enlace decía «presente pero no exporta... version vieja?», que mandaba a buscar donde no estaba el problema. Y que
 * la carga fallara (fd03af31) era peor que lo de antes: GuiShot dejó de correr en la C6. */
static bpvm_status_t tabla_sin_memoria(bpvm_t* vm, const char* qualified, size_t pedia) {
    if (vm->sym_perdidos++ == 0) {
        snprintf(vm->sym_perdido, sizeof vm->sym_perdido,
                 "no cupo '%.80s' (%d simbolos y %u B de nombres; ni %u B de un bloque)",
                 qualified, vm->symbol_count, (unsigned) vm->sym_pool_len, (unsigned) pedia);
        bpvm_diag_urgente("[bpvm] la tabla de simbolos se quedo sin memoria: %s; la carga sigue (el enlace lo dira si "
                          "algun simbolo de los que no entraron hace falta)", vm->sym_perdido);
    }
    return BPVM_OK;   /* apuntado: la carga sigue */
}

/* #639 — si hubo símbolos que no cupieron, LA CAUSA de un enlace que no encuentra algo: se dice, con los números. */
static void con_la_causa(bpvm_t* vm) {
    if (vm->sym_perdidos <= 0) return;
    size_t n = strlen(vm->link_error);
    if (n + 1 >= sizeof vm->link_error) return;
    snprintf(vm->link_error + n, sizeof vm->link_error - n,
             " — la tabla de simbolos se quedo sin memoria: %d sin registrar; el primero, %s",
             vm->sym_perdidos, vm->sym_perdido);
}

bpvm_status_t bpvm_link_register_symbol(bpvm_t* vm, const char* qualified,
                                         uint32_t abs_addr) {
    size_t n = strlen(qualified);

    /* 1) sitio en el pool para el nombre + su terminador. */
    if (vm->sym_pool_len + n + 1u > vm->sym_pool_cap) {
        uint32_t justo = vm->sym_pool_len + (uint32_t) n + 1u;
        uint32_t doble = vm->sym_pool_cap ? vm->sym_pool_cap * 2u : 1024u;
        while (doble < justo) doble *= 2u;
        void* p = vm->sym_pool;
        uint32_t cap = tabla_crecer(&p, vm->sym_pool_cap, doble, justo, 1u);
        if (cap == 0) return tabla_sin_memoria(vm, qualified, justo);
        vm->sym_pool = (char*) p;
        vm->sym_pool_cap = cap;
    }

    /* 2) sitio en la tabla. Se hace DESPUES del pool para que un fallo aqui no
     * deje una cadena huerfana ocupando sitio. */
    if (vm->symbol_count >= vm->symbol_capacity) {
        uint32_t justo = (uint32_t) vm->symbol_count + 1u;
        uint32_t doble = vm->symbol_capacity == 0 ? 32u : (uint32_t) vm->symbol_capacity * 2u;
        void* p = vm->symbols;
        uint32_t cap = tabla_crecer(&p, (uint32_t) vm->symbol_capacity, doble, justo, sizeof(bpvm_symbol_t));
        if (cap == 0) return tabla_sin_memoria(vm, qualified, (size_t) justo * sizeof(bpvm_symbol_t));
        vm->symbols = (bpvm_symbol_t*) p;
        vm->symbol_capacity = (int) cap;
    }

    bpvm_symbol_t* s = &vm->symbols[vm->symbol_count++];
    s->name_off = vm->sym_pool_len;
    s->abs_addr = abs_addr;
    memcpy(vm->sym_pool + vm->sym_pool_len, qualified, n + 1u);
    vm->sym_pool_len += (uint32_t)(n + 1u);
    return BPVM_OK;
}

uint32_t bpvm_link_lookup(const bpvm_t* vm, const char* qualified) {
    s_link_busquedas++;
    for (int i = 0; i < vm->symbol_count; i++) {
        s_link_strcmp++;
        if (strcmp(bpvm_symbol_name(vm, i), qualified) == 0) {
            return vm->symbols[i].abs_addr;
        }
    }
    return 0;
}

/* Paso 4 (V3) — compone un mensaje legible para un símbolo cross-module no
 * resuelto y lo guarda en vm->link_error, para que el handler de RUN lo mande
 * al wire (antes el detalle solo iba a stderr → exit-code mudo en el IDE).
 * Distingue "falta la lib" (el módulo dueño NO está cargado) de "lib presente
 * pero rancia" (cargado pero sin ese símbolo/slot). `who` = módulo que la usa.
 * El nombre de la lib = la parte antes del primer '.' del nombre cualificado
 * (p.ej. 'Json' en 'Json.JsonValue#asObject#15'). */
static void bpvm_link_set_error(bpvm_t* vm, const char* needed, const char* who) {
    char lib[40];
    size_t li = 0;
    while (needed[li] && needed[li] != '.' && li + 1 < sizeof(lib)) {
        lib[li] = needed[li]; li++;
    }
    lib[li] = '\0';

    int loaded = 0;
    if (lib[0]) {
        for (int i = 0; i < vm->module_count; i++) {
            if (strcmp(vm->modules[i].name, lib) == 0) { loaded = 1; break; }
        }
    }

    if (lib[0] && !loaded) {
        snprintf(vm->link_error, sizeof(vm->link_error),
                 "falta la lib '%s' (la usa '%s'; simbolo '%s')", lib, who, needed);
    } else if (lib[0] && vm->sym_perdidos > 0) {
        /* #639 — no se sabe si ESTE es de los que no cupieron, pero «version vieja?» seguro que no es la pista */
        snprintf(vm->link_error, sizeof(vm->link_error),
                 "no se encuentra '%s' de la lib '%s' (la usa '%s')", needed, lib, who);
    } else if (lib[0]) {
        snprintf(vm->link_error, sizeof(vm->link_error),
                 "lib '%s' presente pero no exporta '%s' (la usa '%s'; version vieja?)",
                 lib, needed, who);
    } else {
        snprintf(vm->link_error, sizeof(vm->link_error),
                 "simbolo no resuelto '%s' (la usa '%s')", needed, who);
    }
    /* V7/H2 (#500) — el marcador de LAYOUT (`Cls#layout#nF#nM#…`) no resuelve
     * cuando el padre cambió de forma: la clase hija se compiló con otra idea de
     * dónde caen sus campos y métodos. Misma pista que miVM. */
    if (strstr(needed, "#layout#")) {
        size_t l = strlen(vm->link_error);
        snprintf(vm->link_error + l, sizeof(vm->link_error) - l,
                 " [el padre cambio de forma: recompila la clase hija]");
    }
    con_la_causa(vm);   /* #639: si la tabla se quedó sin memoria, se dice */
}

/* Paso 4 (V3) — accessor público del detalle de fallo de link (ver bpvm.h). */
const char* bpvm_link_error(const bpvm_t* vm) {
    return (vm && vm->link_error[0]) ? vm->link_error : "";
}

/* ============================================================================
 * V7/H2 (#523) — EL ENLACE TARDÍO (MOD9). El MISMO algoritmo que EnlaceTardio.java
 * (miVM es la referencia), en C y sin materializar tablas: lo que en Java es un
 * HashMap efectivo por clase aqui se recalcula subiendo por la cadena y leyendo
 * las filas de la subtabla CLASES (4.5) tal como vienen en el fichero (flash en
 * XIP, RAM si no). Lo unico que se aloja de verdad son los descriptores
 * CONSTRUIDOS de las clases abiertas y la tabla de ranuras efectivas de sus
 * metodos propios: en la arena de memory[], entre el ultimo modulo y el heap.
 * Los textos de rechazo son byte-identicos a los de miVM (mezcla.py los compara).
 * ========================================================================== */

typedef struct {
    uint8_t  mod;                 /* indice del modulo */
    uint8_t  flags;               /* bit0 ABIERTA, bit1 PUBLICA */
    uint8_t  estado;              /* 0 pendiente, 1 construyendo, 2 hecha */
    int16_t  padre;               /* indice en el registro; -1 raiz */
    int16_t  parent_dep;          /* -1 raiz, -2 local, >=0 dep */
    int32_t  cs_off;
    const uint8_t* name;          /* UTF: u16 len + bytes */
    const uint8_t* parent_name;   /* UTF */
    uint16_t nf_own, nm_own, bw_own;
    const uint8_t* fb_own;        /* bw_own words, luego ob_own */
    const uint8_t* metodos; uint16_t n_met;   /* (clave UTF, slot u16, codeOff i32, flags u8)* */
    const uint8_t* campos;  uint16_t n_cam;   /* (clave UTF, slot u16, ancho u8, flags u8)* */
    const uint8_t* eventos; uint16_t n_ev;    /* (clave UTF, recv u16)* */
    /* efectivo */
    uint32_t desc_abs;            /* descriptor (cerrada: data; abierta: arena) */
    uint16_t nf, nm;              /* efectivos */
    uint16_t base_f, base_m;      /* abierta: donde empiezan los propios */
    uint32_t ranuras_off;         /* abierta: arena, u16[n_met] ranura efectiva de cada metodo propio */
} lt_clase_t;

typedef struct {
    bpvm_t*     vm;
    lt_clase_t* reg;
    int         n;
    uint32_t    arena;            /* siguiente libre */
    uint32_t    arena_ini;
    int         construidas, refs;
    int         fallo;            /* 1 = link_error puesto */
} lt_ctx_t;

static uint16_t lt_u16(const uint8_t* p) { return bpvm_read_u16_be(p); }
static const uint8_t* lt_utf_skip(const uint8_t* p) { return p + 2 + lt_u16(p); }
static int lt_utf_eq_c(const uint8_t* u, const char* s) {
    size_t n = strlen(s);
    return lt_u16(u) == n && memcmp(u + 2, s, n) == 0;
}
static int lt_utf_eq(const uint8_t* a, const uint8_t* b) {
    uint16_t n = lt_u16(a);
    return n == lt_u16(b) && memcmp(a + 2, b + 2, n) == 0;
}
static void lt_utf_copy(const uint8_t* u, char* out, size_t cap) {
    uint16_t n = lt_u16(u); if (n >= cap) n = (uint16_t) (cap - 1);
    memcpy(out, u + 2, n); out[n] = '\0';
}
/* el nombre canonico del modulo ([lib.]Mod) */
static void lt_canon(const bpvm_module_t* m, char* out, size_t cap) {
    if (m->library[0]) snprintf(out, cap, "%s.%s", m->library, m->name);
    else               snprintf(out, cap, "%s", m->name);
}
/* «'Lib' (gen 8)» o «'Lib' (no cargado)» */
static void lt_quien(const bpvm_t* vm, const char* canon, char* out, size_t cap) {
    for (int i = 0; i < vm->module_count; i++) {
        char c[160]; lt_canon(&vm->modules[i], c, sizeof c);
        if (strcmp(c, canon) == 0) { snprintf(out, cap, "'%s' (gen %u)", vm->modules[i].name, (unsigned) vm->modules[i].gen); return; }
    }
    snprintf(out, cap, "'%s' (no cargado)", canon);
}
static void lt_error(lt_ctx_t* x, const char* fmt, ...) {
    if (x->fallo) return;
    va_list ap; va_start(ap, fmt);
    vsnprintf(x->vm->link_error, sizeof(x->vm->link_error), fmt, ap);
    va_end(ap);
    con_la_causa(x->vm);
    x->fallo = 1;
}

/* Lee una fila de 4.5 en `c`; devuelve el puntero a la siguiente. */
static const uint8_t* lt_leer_fila(const uint8_t* p, lt_clase_t* c) {
    c->name = p; p = lt_utf_skip(p);
    c->flags = *p++;
    c->cs_off = bpvm_read_i32_be(p); p += 4;
    c->parent_dep = (int16_t) lt_u16(p); p += 2;
    c->parent_name = p; p = lt_utf_skip(p);
    c->nf_own = lt_u16(p); p += 2;
    c->nm_own = lt_u16(p); p += 2;
    c->bw_own = lt_u16(p); p += 2;
    c->fb_own = p; p += 8u * c->bw_own;
    c->n_met = lt_u16(p); p += 2; c->metodos = p;
    for (uint16_t i = 0; i < c->n_met; i++) { p = lt_utf_skip(p); p += 2 + 4 + 1; }
    c->n_cam = lt_u16(p); p += 2; c->campos = p;
    for (uint16_t i = 0; i < c->n_cam; i++) { p = lt_utf_skip(p); p += 2 + 1 + 1; }
    c->n_ev = lt_u16(p); p += 2; c->eventos = p;
    for (uint16_t i = 0; i < c->n_ev; i++) { p = lt_utf_skip(p); p += 2; }
    c->padre = -1; c->estado = 0; c->desc_abs = 0;
    return p;
}

/* ¿la clase `i` del registro es `[lib.]Mod.Cls`? */
static int lt_es(const lt_ctx_t* x, int i, const char* cualificado) {
    const lt_clase_t* c = &x->reg[i];
    char canon[160]; lt_canon(&x->vm->modules[c->mod], canon, sizeof canon);
    size_t cl = strlen(canon);
    if (strncmp(cualificado, canon, cl) != 0 || cualificado[cl] != '.') return 0;
    return lt_utf_eq_c(c->name, cualificado + cl + 1);
}
static int lt_buscar_clase(const lt_ctx_t* x, const char* cualificado) {
    for (int i = 0; i < x->n; i++) if (lt_es(x, i, cualificado)) return i;
    return -1;
}
/* la clase `nombre` (UTF) del modulo `mod` */
static int lt_buscar_local(const lt_ctx_t* x, uint8_t mod, const uint8_t* nombre) {
    for (int i = 0; i < x->n; i++)
        if (x->reg[i].mod == mod && lt_utf_eq(x->reg[i].name, nombre)) return i;
    return -1;
}
static void lt_nombre_cualificado(const lt_ctx_t* x, int i, char* out, size_t cap) {
    char canon[160], nm[128];
    lt_canon(&x->vm->modules[x->reg[i].mod], canon, sizeof canon);
    lt_utf_copy(x->reg[i].name, nm, sizeof nm);
    snprintf(out, cap, "%.120s.%.120s", canon, nm);
}
/* «MOD7» del magic */
static int lt_version(const bpvm_module_t* m) { return m->fmt; }

/* 1. registro: todas las clases de todos los modulos MOD9; sello +6 en las cerradas */
static bpvm_status_t lt_registrar(lt_ctx_t* x) {
    bpvm_t* vm = x->vm;
    int total = 0;
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (!m->clases || m->clases_len < 4) continue;
        if (lt_u16(m->clases) != 1) { lt_error(x, "'%s': subtabla CLASES de version %u desconocida", m->name, (unsigned) lt_u16(m->clases)); return BPVM_ERR_RUNTIME; }
        total += lt_u16(m->clases + 2);
    }
    x->n = 0;
    if (total == 0) return BPVM_OK;
    x->reg = (lt_clase_t*) bpvm_calloc((size_t) total, sizeof(lt_clase_t));
    if (!x->reg) return BPVM_ERR_OOM;
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (!m->clases || m->clases_len < 4) continue;
        uint16_t n = lt_u16(m->clases + 2);
        const uint8_t* p = m->clases + 4;
        for (uint16_t k = 0; k < n; k++) {
            lt_clase_t* c = &x->reg[x->n++];
            p = lt_leer_fila(p, c);
            c->mod = (uint8_t) i;
            if (!(c->flags & 1)) {
                c->desc_abs = (uint32_t)((int32_t) m->code_start + c->cs_off);
                bpvm_write_u16_be(vm->memory + c->desc_abs + BPVM_CLS_OFF_MODULO, (uint16_t) (i + 1));
                c->nf = bpvm_read_u16_be(vm->memory + c->desc_abs + BPVM_CLS_OFF_NUM_FIELDS);
                c->nm = bpvm_read_u16_be(vm->memory + c->desc_abs + BPVM_CLS_OFF_NUM_METHODS);
            }
        }
    }
    return BPVM_OK;
}

/* el padre de una fila: indice en el registro, -1 raiz, -2 + error */
static int lt_padre_de(lt_ctx_t* x, int ci) {
    lt_clase_t* c = &x->reg[ci];
    const bpvm_module_t* m = &x->vm->modules[c->mod];
    if (c->parent_dep == BPVM_DEP_RAIZ) return -1;
    if (c->parent_dep == BPVM_DEP_LOCAL) {
        int p = lt_buscar_local(x, c->mod, c->parent_name);
        if (p >= 0) return p;
        char nm[128], pn[128]; lt_utf_copy(c->name, nm, sizeof nm); lt_utf_copy(c->parent_name, pn, sizeof pn);
        lt_error(x, "'%s' extiende la clase '%s.%s' y '%s' (gen %u) no la tiene: recompila '%s'", m->name, m->name, pn, m->name, (unsigned) m->gen, m->name);
        return -2;
    }
    char q[256]; lt_utf_copy(c->parent_name, q, sizeof q);
    int p = lt_buscar_clase(x, q);
    if (p >= 0) return p;
    /* E1 (padre) o E9 (el dueno es MOD7/8) */
    char modp[160]; { const char* d = strrchr(q, '.'); size_t l = d ? (size_t)(d - q) : strlen(q); if (l >= sizeof modp) l = sizeof modp - 1; memcpy(modp, q, l); modp[l] = 0; }
    for (int i = 0; i < x->vm->module_count; i++) {
        char canon[160]; lt_canon(&x->vm->modules[i], canon, sizeof canon);
        if (strcmp(canon, modp) == 0 && !x->vm->modules[i].clases) {
            lt_error(x, "'%s' es MOD%d y '%s' necesita su tabla de clases: recompila '%s'", x->vm->modules[i].name, lt_version(&x->vm->modules[i]), m->name, x->vm->modules[i].name);
            return -2;
        }
    }
    char quien[200]; lt_quien(x->vm, modp, quien, sizeof quien);
    lt_error(x, "'%s' extiende la clase '%s' y %s no la tiene: recompila '%s'", m->name, q, quien, m->name);
    return -2;
}

/* ranura efectiva del metodo propio k de la clase ci */
static uint16_t lt_ranura_propia(const lt_ctx_t* x, int ci, uint16_t k, const uint8_t* fila_met) {
    const lt_clase_t* c = &x->reg[ci];
    if (c->flags & 1) return bpvm_read_u16_be(x->vm->memory + c->ranuras_off + 2u * k);
    return lt_u16(lt_utf_skip(fila_met));   /* cerrada: absoluta en su fila */
}
/* Busca `clave` (C string) entre los metodos de la cadena efectiva de ci.
 * Devuelve 1 y (slot, privado, en) si esta. Si `saltar_privados`, una privada no cuenta. */
static int lt_buscar_metodo(const lt_ctx_t* x, int ci, const char* clave, uint16_t* slot, int* privado, int* en) {
    for (int c = ci; c >= 0; c = x->reg[c].padre) {
        const lt_clase_t* k = &x->reg[c];
        const uint8_t* p = k->metodos;
        for (uint16_t i = 0; i < k->n_met; i++) {
            const uint8_t* fila = p;
            const uint8_t* q = lt_utf_skip(p);            /* clave */
            uint8_t fl = q[2 + 4];
            if (lt_utf_eq_c(p, clave)) {
                if (slot) *slot = lt_ranura_propia(x, c, i, fila);
                if (privado) *privado = (fl & 2) != 0;
                if (en) *en = c;
                return 1;
            }
            p = q + 2 + 4 + 1;
        }
    }
    return 0;
}
static int lt_buscar_campo(const lt_ctx_t* x, int ci, const char* clave, uint16_t* slot) {
    for (int c = ci; c >= 0; c = x->reg[c].padre) {
        const lt_clase_t* k = &x->reg[c];
        const uint8_t* p = k->campos;
        for (uint16_t i = 0; i < k->n_cam; i++) {
            const uint8_t* q = lt_utf_skip(p);
            if (lt_utf_eq_c(p, clave)) { if (slot) *slot = (uint16_t) (lt_u16(q) + ((k->flags & 1) ? k->base_f : 0)); return 1; }
            p = q + 2 + 1 + 1;
        }
    }
    return 0;
}
static int lt_buscar_evento(const lt_ctx_t* x, int ci, const char* clave, uint16_t* recv) {
    for (int c = ci; c >= 0; c = x->reg[c].padre) {
        const lt_clase_t* k = &x->reg[c];
        const uint8_t* p = k->eventos;
        for (uint16_t i = 0; i < k->n_ev; i++) {
            const uint8_t* q = lt_utf_skip(p);
            if (lt_utf_eq_c(p, clave)) { if (recv) *recv = (uint16_t) (lt_u16(q) + ((k->flags & 1) ? k->base_f : 0)); return 1; }
            p = q + 2;
        }
    }
    return 0;
}
/* D4: el cerrojo `sync` es UNO por objeto, el del PRIMER eslabon desde la raiz que lo declara */
static int lt_mutex_desde_la_raiz(const lt_ctx_t* x, int ci, uint16_t* slot) {
    /* V10 (16-sep, F2): sin tope de profundidad (antes `cadena[32]`: con 33 eslabones la VM-C
     * cerraba OTRO cerrojo que miVM). De la raiz hacia abajo: el i-esimo ancestro se busca andando. */
    int n = 0;
    for (int c = ci; c >= 0; c = x->reg[c].padre) n++;
    for (int i = n - 1; i >= 0; i--) {
        int ac = ci; for (int s = 0; s < i; s++) ac = x->reg[ac].padre;
        const lt_clase_t* k = &x->reg[ac];
        const uint8_t* p = k->campos;
        for (uint16_t j = 0; j < k->n_cam; j++) {
            const uint8_t* q = lt_utf_skip(p);
            if (lt_utf_eq_c(p, "__syncMutex:LMutex;")) { *slot = (uint16_t) (lt_u16(q) + ((k->flags & 1) ? k->base_f : 0)); return 1; }
            p = q + 2 + 1 + 1;
        }
    }
    return 0;
}
/* « (tiene 'k()i')»: las otras claves con el mismo nombre en la cadena, ordenadas, sin repetir */
static void lt_tiene(const lt_ctx_t* x, int ci, const char* clave, char sep, char* out, size_t cap) {
    out[0] = 0;
    const char* fin = strchr(clave, sep); if (!fin) return;
    size_t nl = (size_t)(fin - clave);
    const uint8_t* cand[64]; int nc = 0;   /* V10 (F3): se recogen TODAS, se ordenan, y se citan 8 (como miVM) */
    for (int c = ci; c >= 0; c = x->reg[c].padre) {
        const lt_clase_t* k = &x->reg[c];
        const uint8_t* p = (sep == '(') ? k->metodos : k->campos;
        uint16_t n = (sep == '(') ? k->n_met : k->n_cam;
        for (uint16_t i = 0; i < n; i++) {
            const uint8_t* q = lt_utf_skip(p);
            uint16_t len = lt_u16(p);
            if (len > nl && memcmp(p + 2, clave, nl) == 0 && p[2 + nl] == (uint8_t) sep && !lt_utf_eq_c(p, clave)) {
                int dup = 0; for (int d = 0; d < nc; d++) if (lt_utf_eq(cand[d], p)) { dup = 1; break; }
                if (!dup && nc < 64) cand[nc++] = p;
            }
            p = q + ((sep == '(') ? 2 + 4 + 1 : 2 + 1 + 1);
        }
    }
    if (nc == 0) return;
    /* orden lexicografico (como el TreeSet de miVM) */
    for (int i = 1; i < nc; i++) for (int j = i; j > 0; j--) {
        char a[256], b[256]; lt_utf_copy(cand[j - 1], a, sizeof a); lt_utf_copy(cand[j], b, sizeof b);
        if (strcmp(a, b) > 0) { const uint8_t* t = cand[j]; cand[j] = cand[j - 1]; cand[j - 1] = t; } else break;
    }
    size_t l = 0;
    if (nc > 8) nc = 8;   /* MAX_CANDIDATAS de EnlaceTardio.java */
    l += (size_t) snprintf(out + l, cap - l, " (tiene ");
    for (int i = 0; i < nc && l < cap; i++) {
        char a[256]; lt_utf_copy(cand[i], a, sizeof a);
        l += (size_t) snprintf(out + l, cap - l, "%s'%s'", i ? ", " : "", a);
    }
    if (l < cap) snprintf(out + l, cap - l, ")");
}

/* 2. construccion memoizada padre -> hijo */
static bpvm_status_t lt_asegurar(lt_ctx_t* x, int ci) {
    lt_clase_t* c = &x->reg[ci];
    bpvm_t* vm = x->vm;
    if (c->estado == 2) return BPVM_OK;
    if (c->estado == 1) { char q[256]; lt_nombre_cualificado(x, ci, q, sizeof q); lt_error(x, "ciclo de herencia en '%s'", q); return BPVM_ERR_RUNTIME; }
    c->estado = 1;
    int p = lt_padre_de(x, ci);
    if (p == -2) return BPVM_ERR_RUNTIME;
    c->padre = (int16_t) p;
    if (p >= 0) { bpvm_status_t st = lt_asegurar(x, p); if (st != BPVM_OK) return st; }
    if (!(c->flags & 1)) { c->estado = 2; return BPVM_OK; }   /* cerrada: solo se indexa */
    /* ABIERTA: contra el padre real */
    const bpvm_module_t* m = &vm->modules[c->mod];
    if (p < 0) { char q[256]; lt_nombre_cualificado(x, ci, q, sizeof q); lt_error(x, "la clase abierta '%s' no tiene padre: .mod corrupto", q); return BPVM_ERR_RUNTIME; }
    const lt_clase_t* P = &x->reg[p];
    c->base_f = P->nf; c->base_m = P->nm;
    uint32_t nf = (uint32_t) P->nf + c->nf_own;
    uint32_t nm = P->nm;
    /* las ranuras efectivas de los metodos propios: u16[n_met] en la arena */
    uint32_t ran = (x->arena + 3u) & ~3u;
    if (ran + 2u * c->n_met > vm->heap_top) return BPVM_ERR_OOM;
    c->ranuras_off = ran; x->arena = ran + 2u * c->n_met;
    {
        const uint8_t* q = c->metodos;
        for (uint16_t i = 0; i < c->n_met; i++) {
            char clave[256]; lt_utf_copy(q, clave, sizeof clave);
            const uint8_t* r = lt_utf_skip(q);
            uint8_t fl = r[2 + 4];
            uint16_t her; int priv = 0;
            int hay = lt_buscar_metodo(x, p, clave, &her, &priv, NULL);
            uint16_t ranura;
            if (hay && !priv) {
                ranura = her;                                  /* override (criterio JVM) */
            } else {
                if (fl & 1) {                                  /* E2: override huerfano */
                    char nm[128], canon[160], quien[200]; lt_utf_copy(c->name, nm, sizeof nm);
                    lt_canon(&vm->modules[P->mod], canon, sizeof canon); lt_quien(vm, canon, quien, sizeof quien);
                    lt_error(x, "'%s.%s' redefine '%s' y %s ya no lo tiene: recompila '%s'", m->name, nm, clave, quien, m->name);
                    return BPVM_ERR_RUNTIME;
                }
                ranura = (uint16_t) nm++;
            }
            bpvm_write_u16_be(vm->memory + ran + 2u * i, ranura);
            q = r + 2 + 4 + 1;
        }
    }
    if (nf > 0xFFFF || nm > 0xFFFF) {
        char nm2[128]; lt_utf_copy(c->name, nm2, sizeof nm2);
        lt_error(x, "'%s.%s' tendria %u ranuras (maximo 65535)", m->name, nm2, (unsigned) (nf > nm ? nf : nm));
        return BPVM_ERR_RUNTIME;
    }
    /* el descriptor: cabecera, bitmaps (padre OR propios desplazados), vtable */
    uint32_t bw = (nf + 31u) >> 5;
    uint32_t tam = 12u + 8u * bw + 4u * nm;
    uint32_t desc = (x->arena + 3u) & ~3u;
    if (desc + tam > vm->heap_top) return BPVM_ERR_OOM;
    x->arena = desc + tam;
    uint8_t* d = vm->memory + desc;
    memset(d, 0, tam);
    bpvm_write_u16_be(d + BPVM_CLS_OFF_NUM_FIELDS, (uint16_t) nf);
    bpvm_write_u16_be(d + BPVM_CLS_OFF_NUM_METHODS, (uint16_t) nm);
    bpvm_write_u16_be(d + BPVM_CLS_OFF_BITMAP_WORDS, (uint16_t) bw);
    bpvm_write_u16_be(d + BPVM_CLS_OFF_MODULO, (uint16_t) (c->mod + 1));
    bpvm_write_i32_be(d + BPVM_CLS_OFF_PARENT_OFF, (int32_t) P->desc_abs - (int32_t) m->code_start);
    {
        const uint8_t* pd = vm->memory + P->desc_abs;
        uint16_t pbw = bpvm_read_u16_be(pd + BPVM_CLS_OFF_BITMAP_WORDS);
        for (uint32_t w = 0; w < pbw && w < bw; w++) {
            bpvm_write_u32_be(d + BPVM_CLS_OFF_FIELD_BITMAP + 4u * w, bpvm_read_u32_be(pd + BPVM_CLS_OFF_FIELD_BITMAP + 4u * w));
            bpvm_write_u32_be(d + BPVM_CLS_OFF_FIELD_BITMAP + 4u * bw + 4u * w, bpvm_read_u32_be(pd + BPVM_CLS_OFF_FIELD_BITMAP + 4u * pbw + 4u * w));
        }
        for (uint32_t bit = 0; bit < c->nf_own; bit++) {
            uint32_t w = bit >> 5, mk = 1u << (bit & 31);
            uint32_t fb = (w < c->bw_own) ? bpvm_read_u32_be(c->fb_own + 4u * w) : 0;
            uint32_t ob = (w < c->bw_own) ? bpvm_read_u32_be(c->fb_own + 4u * c->bw_own + 4u * w) : 0;
            uint32_t abs = P->nf + bit;
            uint8_t* pf = d + BPVM_CLS_OFF_FIELD_BITMAP + 4u * (abs >> 5);
            uint8_t* po = d + BPVM_CLS_OFF_FIELD_BITMAP + 4u * bw + 4u * (abs >> 5);
            if (fb & mk) bpvm_write_u32_be(pf, bpvm_read_u32_be(pf) | (1u << (abs & 31)));
            if (ob & mk) bpvm_write_u32_be(po, bpvm_read_u32_be(po) | (1u << (abs & 31)));
        }
    }
    {
        uint8_t* vt = d + BPVM_CLS_OFF_FIELD_BITMAP + 8u * bw;
        for (uint32_t s2 = 0; s2 < nm; s2++) bpvm_write_i32_be(vt + 4u * s2, -1);
        const uint8_t* q = c->metodos;
        for (uint16_t i = 0; i < c->n_met; i++) {
            const uint8_t* r = lt_utf_skip(q);
            int32_t code_off = bpvm_read_i32_be(r + 2);
            uint16_t ranura = bpvm_read_u16_be(vm->memory + ran + 2u * i);
            bpvm_write_i32_be(vt + 4u * ranura, code_off);
            q = r + 2 + 4 + 1;
        }
    }
    c->desc_abs = desc; c->nf = (uint16_t) nf; c->nm = (uint16_t) nm;
    x->construidas++;
    /* el data-symbol, para quien la pida por nombre (nietos, refs CLASE, la reflexion) */
    {
        char canon[160], nm2[128], qual[320];
        lt_canon(m, canon, sizeof canon); lt_utf_copy(c->name, nm2, sizeof nm2);
        snprintf(qual, sizeof qual, "%s.%s", canon, nm2);
        bpvm_status_t rsim = bpvm_link_register_symbol(vm, qual, desc);
        if (rsim == BPVM_OK && m->library[0]) { char sq[320]; snprintf(sq, sizeof sq, "%s.%s", m->name, nm2); if (bpvm_link_lookup(vm, sq) == 0) rsim = bpvm_link_register_symbol(vm, sq, desc); }
        if (rsim != BPVM_OK) { lt_error(x, "%s", vm->load_error); return rsim; }   /* #639: sin memoria para la tabla */
    }
    c->estado = 2;
    return BPVM_OK;
}

/* 3. las refs de un modulo -> ext[ext_count31 + i] */
static bpvm_status_t lt_resolver_refs(lt_ctx_t* x, int mi) {
    bpvm_t* vm = x->vm;
    bpvm_module_t* m = &vm->modules[mi];
    char canon_m[160]; lt_canon(m, canon_m, sizeof canon_m);
    for (int i = 0; i < m->ref_count; i++) {
        const bpvm_ref_t* r = &m->refs[i];
        char q[320];
        if (r->dep_idx == BPVM_DEP_LOCAL) snprintf(q, sizeof q, "%s.%s", canon_m, r->clase);
        else                              snprintf(q, sizeof q, "%s", r->clase);
        char modc[160]; { const char* d = strrchr(q, '.'); size_t l = d ? (size_t)(d - q) : strlen(q); if (l >= sizeof modc) l = sizeof modc - 1; memcpy(modc, q, l); modc[l] = 0; }
        int ci = lt_buscar_clase(x, q);
        if (ci < 0) {
            for (int k = 0; k < vm->module_count; k++) {
                char canon[160]; lt_canon(&vm->modules[k], canon, sizeof canon);
                if (strcmp(canon, modc) == 0 && !vm->modules[k].clases) {
                    lt_error(x, "'%s' es MOD%d y '%s' necesita su tabla de clases: recompila '%s'", vm->modules[k].name, lt_version(&vm->modules[k]), m->name, vm->modules[k].name);
                    return BPVM_ERR_RUNTIME;
                }
            }
            char quien[200]; lt_quien(vm, modc, quien, sizeof quien);
            lt_error(x, "'%s' necesita la clase '%s' y %s no la tiene: recompila '%s'", m->name, q, quien, m->name);
            return BPVM_ERR_RUNTIME;
        }
        bpvm_status_t st = lt_asegurar(x, ci); if (st != BPVM_OK) return st;
        char dueno_canon[160], quien[200];
        lt_canon(&vm->modules[x->reg[ci].mod], dueno_canon, sizeof dueno_canon);
        lt_quien(vm, dueno_canon, quien, sizeof quien);
        uint32_t valor = 0;
        switch (r->kind) {
            case BPVM_REF_CLASE: valor = x->reg[ci].desc_abs; break;
            case BPVM_REF_MET: {
                uint16_t slot; int priv = 0, en = -1;
                int hay = lt_buscar_metodo(x, ci, r->clave, &slot, &priv, &en);
                if (!hay || (priv && x->reg[en].mod != mi)) {
                    static char tiene[1400]; lt_tiene(x, ci, r->clave, '(', tiene, sizeof tiene);   /* 8 x 160 + el texto; static: el enlace no es reentrante */
                    lt_error(x, "'%s' necesita '%s.%s' y %s no lo tiene%s: recompila '%s'", m->name, q, r->clave, quien, tiene, m->name);
                    return BPVM_ERR_RUNTIME;
                }
                valor = slot; break;
            }
            case BPVM_REF_CAMPO: {
                uint16_t slot; int hay = (strcmp(r->clave, "__syncMutex:LMutex;") == 0) ? lt_mutex_desde_la_raiz(x, ci, &slot) : lt_buscar_campo(x, ci, r->clave, &slot);
                if (!hay) {
                    static char tiene[1400]; lt_tiene(x, ci, r->clave, ':', tiene, sizeof tiene);
                    lt_error(x, "'%s' necesita el campo '%s.%s' y %s no lo tiene%s: recompila '%s'", m->name, q, r->clave, quien, tiene, m->name);
                    return BPVM_ERR_RUNTIME;
                }
                valor = slot; break;
            }
            case BPVM_REF_EV_RECV: case BPVM_REF_EV_DEST: {
                uint16_t recv;
                if (!lt_buscar_evento(x, ci, r->clave, &recv)) {
                    lt_error(x, "'%s' necesita el evento '%s.%s' y %s no lo tiene: recompila '%s'", m->name, q, r->clave, quien, m->name);
                    return BPVM_ERR_RUNTIME;
                }
                valor = (r->kind == BPVM_REF_EV_DEST) ? (uint32_t) recv + 2u : recv; break;
            }
            case BPVM_REF_SUPER: {
                /* V10/J-3: la primera fila con la clave subiendo por la cadena desde el padre; un privado
                 * de otro modulo no cuenta (como MET). El valor es la direccion ABSOLUTA del metodo
                 * (base de CODIGO del modulo dueno + codeOff: en XIP, la flash), la misma que un import. */
                int hallado = 0;
                for (int c = ci; c >= 0 && !hallado; c = x->reg[c].padre) {
                    const lt_clase_t* k = &x->reg[c];
                    const uint8_t* p = k->metodos;
                    for (uint16_t i2 = 0; i2 < k->n_met; i2++) {
                        const uint8_t* fq = lt_utf_skip(p);
                        if (lt_utf_eq_c(p, r->clave)) {
                            uint8_t fl = fq[2 + 4];
                            if (!((fl & 2) && k->mod != mi)) { valor = vm->modules[k->mod].cb + (uint32_t) bpvm_read_i32_be(fq + 2); hallado = 1; break; }
                        }
                        p = fq + 2 + 4 + 1;
                    }
                }
                if (!hallado) {
                    static char tiene[1400]; lt_tiene(x, ci, r->clave, '(', tiene, sizeof tiene);
                    lt_error(x, "'%s' necesita '%s.%s' y %s no lo tiene%s: recompila '%s'", m->name, q, r->clave, quien, tiene, m->name);
                    return BPVM_ERR_RUNTIME;
                }
                break;
            }
            case BPVM_REF_SUBTIPO: {
                char qs[320];
                if (strchr(r->clave, '.')) snprintf(qs, sizeof qs, "%s", r->clave); else snprintf(qs, sizeof qs, "%s.%s", canon_m, r->clave);
                int ok = 0;
                for (int a = ci; a >= 0; a = x->reg[a].padre) if (lt_es(x, a, qs)) { ok = 1; break; }
                if (!ok) { lt_error(x, "'%s' asume que '%s' extiende '%s' y %s ya no: recompila '%s'", m->name, q, qs, quien, m->name); return BPVM_ERR_RUNTIME; }
                valor = 1; break;
            }
            default:
                lt_error(x, "'%s' pide una referencia de tipo desconocido (%u): .mod corrupto", m->name, (unsigned) r->kind);
                return BPVM_ERR_RUNTIME;
        }
        bpvm_write_u32_be(vm->memory + m->ext_table_addr + (m->ext_count31 + (uint32_t) i) * 4u, valor);
        x->refs++;
    }
    return BPVM_OK;
}

/* 4. verificacion: ninguna ref sin resolver; ninguna ranura heredada sin dueno */
static bpvm_status_t lt_verificar(lt_ctx_t* x) {
    bpvm_t* vm = x->vm;
    for (int i = 0; i < x->n; i++) {
        const lt_clase_t* c = &x->reg[i];
        if (!(c->flags & 1) || c->estado != 2) continue;
        const uint8_t* d = vm->memory + c->desc_abs;
        uint16_t bw = bpvm_read_u16_be(d + BPVM_CLS_OFF_BITMAP_WORDS);
        const uint8_t* vt = d + BPVM_CLS_OFF_FIELD_BITMAP + 8u * bw;
        for (uint32_t s2 = 0; s2 < c->nm; s2++) {
            if (bpvm_read_i32_be(vt + 4u * s2) == -1 && (c->padre < 0 || s2 >= x->reg[c->padre].nm)) {
                char nm[128]; lt_utf_copy(c->name, nm, sizeof nm);
                lt_error(x, "la clase '%s.%s' construida es inconsistente (ranura %u sin dueno): .mod corrupto o bug del cargador", vm->modules[c->mod].name, nm, (unsigned) s2);
                return BPVM_ERR_RUNTIME;
            }
        }
    }
    for (int mi = 0; mi < vm->module_count; mi++) {
        const bpvm_module_t* m = &vm->modules[mi];
        for (int i = 0; i < m->ref_count; i++)
            if (bpvm_read_u32_be(vm->memory + m->ext_table_addr + (m->ext_count31 + (uint32_t) i) * 4u) == 0xFFFFFFFFu) {
                lt_error(x, "la referencia %s.%s de '%s' quedo sin resolver: bug del cargador", m->refs[i].clase, m->refs[i].clave, m->name);
                return BPVM_ERR_RUNTIME;
            }
    }
    return BPVM_OK;
}

static bpvm_status_t bpvm_enlace_tardio(bpvm_t* vm) {
    lt_ctx_t x; memset(&x, 0, sizeof x);
    x.vm = vm; x.arena = x.arena_ini = vm->next_free_address;
    bpvm_status_t st = lt_registrar(&x);
    if (st == BPVM_OK) for (int i = 0; i < x.n && st == BPVM_OK; i++) st = lt_asegurar(&x, i);
    if (st == BPVM_OK) for (int i = 0; i < vm->module_count && st == BPVM_OK; i++) st = lt_resolver_refs(&x, i);
    if (st == BPVM_OK) st = lt_verificar(&x);
    if (st == BPVM_OK && x.arena > x.arena_ini) {
        /* la arena es del RUN: el heap empieza detras (mismo gesto que el final de la carga) */
        uint32_t nf = (x.arena + 63u) & ~63u;
        vm->next_free_address = nf; vm->heap_start = nf; vm->heap_next = nf;
        vm->last_gc_heap_next = nf; vm->free_list_head = 0;
        vm->heap_bloques = 0;   /* V7/H7 #593: el heap, vacío */
    }
    if (x.construidas > 0 || x.refs > 0)
        bpvm_diag("[bpvm] enlace tardio: %d clases construidas, %d refs, %u bytes de arena",
                  x.construidas, x.refs, (unsigned) (x.arena - x.arena_ini));
    if (x.reg) bpvm_free(x.reg);
    if (st != BPVM_OK && !x.fallo) snprintf(vm->link_error, sizeof(vm->link_error), "enlace tardio: sin memoria para construir las clases");
    return st;
}

/* La reflexion de Forms (#324) con MOD9: el slot de un metodo por su nombre simple, a
 * partir del descriptor de la clase. Sin marcadores: se busca la fila de la clase en la
 * subtabla CLASES de su modulo (el sello +6 dice cual), y su cadena. Para una clase
 * CONSTRUIDA la ranura efectiva de un metodo propio se lee de su propia vtable (la
 * entrada cuyo codeOff es el suyo); el registro del enlace ya no existe. -1 = no esta
 * o la clase no es MOD9 (el llamante prueba los marcadores). */
/* V7/H2 (#533) — la busqueda comun de las dos de abajo. `exacto` = la CLAVE entera (`m(i)i`),
 * la primera fila que la tenga subiendo por la cadena (un override tiene la misma ranura que
 * el metodo del padre, asi que la primera vale); no exacto = el nombre simple `nombre(`, la
 * ranura MINIMA de la cadena (la reflexion de Forms). Con `modulo_llamante` (canonico), una
 * fila PRIVADA de otro modulo no cuenta: la misma regla que el enlace (MET, SUPER). */
static int lt_slot_por(const bpvm_t* vm, uint32_t class_ptr, const char* texto, int exacto, const char* modulo_llamante) {
    size_t nl = strlen(texto);
    const char* nombre = texto;
    uint32_t desc = class_ptr;
    int mejor = -1;
    for (int prof = 0; prof < 64 && desc != 0; prof++) {
        uint16_t sello = bpvm_read_u16_be(vm->memory + desc + BPVM_CLS_OFF_MODULO);
        if (sello == 0 || sello > (uint16_t) vm->module_count) return -1;
        const bpvm_module_t* m = &vm->modules[sello - 1];
        if (!m->clases || m->clases_len < 4) return -1;
        char canon[160]; lt_canon(m, canon, sizeof canon);
        uint16_t n = lt_u16(m->clases + 2);
        const uint8_t* p = m->clases + 4;
        int hallada = 0;
        for (uint16_t k = 0; k < n && !hallada; k++) {
            lt_clase_t c; p = lt_leer_fila(p, &c);
            uint32_t abs;
            if (!(c.flags & 1)) abs = (uint32_t)((int32_t) m->code_start + c.cs_off);
            else { char nm[128], q[320]; lt_utf_copy(c.name, nm, sizeof nm); snprintf(q, sizeof q, "%.120s.%.120s", canon, nm); abs = bpvm_link_lookup(vm, q); }
            if (abs != desc) continue;
            hallada = 1;
            const uint8_t* q = c.metodos;
            for (uint16_t i = 0; i < c.n_met; i++) {
                const uint8_t* r = lt_utf_skip(q);
                uint16_t len = lt_u16(q);
                int casa = exacto ? (len == nl && memcmp(q + 2, nombre, nl) == 0)
                                  : (len > nl && memcmp(q + 2, nombre, nl) == 0 && q[2 + nl] == '(');
                if (casa && (r[2 + 4] & 2) && modulo_llamante && strcmp(canon, modulo_llamante) != 0) casa = 0;   /* privado ajeno: no cuenta */
                if (casa) {
                    int s = -1;
                    if (!(c.flags & 1)) s = (int) lt_u16(r);
                    else {
                        /* construida: la entrada de SU vtable con este codeOff */
                        int32_t code_off = bpvm_read_i32_be(r + 2);
                        uint16_t bw = bpvm_read_u16_be(vm->memory + desc + BPVM_CLS_OFF_BITMAP_WORDS);
                        uint16_t nm2 = bpvm_read_u16_be(vm->memory + desc + BPVM_CLS_OFF_NUM_METHODS);
                        const uint8_t* vt = vm->memory + desc + BPVM_CLS_OFF_FIELD_BITMAP + 8u * bw;
                        for (uint16_t s2 = 0; s2 < nm2; s2++) if (bpvm_read_i32_be(vt + 4u * s2) == code_off) { s = (int) s2; break; }
                    }
                    if (s >= 0 && (mejor < 0 || s < mejor)) mejor = s;
                    if (s >= 0 && exacto) return s;   /* la primera fila con la clave, subiendo */
                }
                q = r + 2 + 4 + 1;
            }
        }
        if (!hallada) break;
        int32_t parent_off = bpvm_read_i32_be(vm->memory + desc + BPVM_CLS_OFF_PARENT_OFF);
        if (parent_off == 0) break;
        desc = (uint32_t)((int32_t) bpvm_get_cs_for_data_addr(vm, desc) + parent_off);
    }
    return mejor;
}
int bpvm_link_slot_por_nombre(const bpvm_t* vm, uint32_t class_ptr, const char* nombre) {
    /* V10 (16-sep, F6/REF-1): la ranura MINIMA entre todas las claves `nombre(` de la cadena
     * entera — la misma regla que slotPorNombre en miVM y que los marcadores MOD8 (la primera
     * declarada). Antes devolvia la primera fila de la clase mas derivada que tuviera una, y
     * con sobrecargas homonimas cada VM elegia otra (y otro ancho de frame). */
    return lt_slot_por(vm, class_ptr, nombre, 0, NULL);
}
int bpvm_link_nombre_de_clase(const bpvm_t* vm, uint32_t class_ptr, char* out, size_t cap) {
    if (cap) out[0] = '\0';
    if (class_ptr == 0 || class_ptr + BPVM_CLS_OFF_FIELD_BITMAP > vm->memory_size) return 0;
    uint16_t sello = bpvm_read_u16_be(vm->memory + class_ptr + BPVM_CLS_OFF_MODULO);
    if (sello == 0 || sello > (uint16_t) vm->module_count) return 0;
    const bpvm_module_t* m = &vm->modules[sello - 1];
    if (!m->clases || m->clases_len < 4) return 0;
    char canon[160]; lt_canon(m, canon, sizeof canon);
    uint16_t n = lt_u16(m->clases + 2);
    const uint8_t* p = m->clases + 4;
    for (uint16_t k = 0; k < n; k++) {
        lt_clase_t c; p = lt_leer_fila(p, &c);
        char nm[128]; lt_utf_copy(c.name, nm, sizeof nm);
        uint32_t abs;
        if (!(c.flags & 1)) abs = (uint32_t)((int32_t) m->code_start + c.cs_off);
        else { char q[320]; snprintf(q, sizeof q, "%.120s.%.120s", canon, nm); abs = bpvm_link_lookup(vm, q); }
        if (abs != class_ptr) continue;
        snprintf(out, cap, "%.120s.%.120s", canon, nm);
        return 1;
    }
    return 0;
}

int bpvm_link_slot_por_clave(const bpvm_t* vm, uint32_t class_ptr, const char* clave, const char* modulo_llamante) {
    return lt_slot_por(vm, class_ptr, clave, 1, modulo_llamante);
}

/* V10 (16-sep, R2/R3/R6/R9) — la 4.5 recien retenida, con los limites comprobados ANTES de que
 * nadie la lea (el registro, la reflexion, la construccion): un .mod corrupto o truncado se
 * rechaza con motivo en vez de leer basura o escribir fuera de rango. */
#define LTV_NEED(n) do { if ((size_t)(end - p) < (size_t)(n)) { motivo = "fila truncada"; goto mal; } } while (0)
static const uint8_t* ltv_utf(const uint8_t* p, const uint8_t* end, const char** motivo_out) {
    if ((size_t)(end - p) < 2) { *motivo_out = "fila truncada"; return NULL; }
    uint16_t len = lt_u16(p);
    if ((size_t)(end - p) < 2u + len) { *motivo_out = "nombre truncado"; return NULL; }
    return p + 2 + len;
}
int bpvm_link_validar_clases(bpvm_t* vm, const bpvm_module_t* mod) {
    const char* motivo = NULL;
    const uint8_t* p = mod->clases;
    const uint8_t* end = mod->clases + mod->clases_len;
    if (!p || mod->clases_len < 4) { motivo = "sin cabecera"; goto mal; }
    if (lt_u16(p) != 1) { snprintf(vm->load_error, sizeof vm->load_error, "'%s': subtabla CLASES de version %u (esperada 1)", mod->name, (unsigned) lt_u16(p)); return BPVM_ERR_BAD_HEADER; }
    uint16_t n = lt_u16(p + 2); p += 4;
    for (uint16_t k = 0; k < n; k++) {
        const uint8_t* q;
        if (!(q = ltv_utf(p, end, &motivo))) goto mal;          /* name */
        p = q; LTV_NEED(1 + 4 + 2);
        uint8_t flags = *p++;
        int32_t cs_off = bpvm_read_i32_be(p); p += 4;
        int16_t pdep = (int16_t) lt_u16(p); p += 2;
        if (!(q = ltv_utf(p, end, &motivo))) goto mal;          /* parent name */
        p = q; LTV_NEED(6);
        uint16_t nf_own = lt_u16(p), nm_own = lt_u16(p + 2), bw_own = lt_u16(p + 4); p += 6;
        LTV_NEED(8u * bw_own); p += 8u * bw_own;
        if (bw_own != (uint16_t)((nf_own + 31u) >> 5)) { motivo = "bitmap incoherente con nFieldsOwn"; goto mal; }
        if (pdep < -2 || pdep >= mod->dep_count) { motivo = "parentDepIdx fuera de la subtabla deps"; goto mal; }
        uint32_t desc_nf = 0, desc_nm = 0;
        if (!(flags & 1)) {
            uint32_t abs = (uint32_t)((int32_t) mod->code_start + cs_off);
            if (abs < mod->data_start || abs + 12u > mod->data_start + mod->data_size) { motivo = "csOff fuera de la seccion de datos del modulo"; goto mal; }
            desc_nf = bpvm_read_u16_be(vm->memory + abs); desc_nm = bpvm_read_u16_be(vm->memory + abs + 2);
        }
        LTV_NEED(2); uint16_t n_met = lt_u16(p); p += 2;
        for (uint16_t i = 0; i < n_met; i++) {
            if (!(q = ltv_utf(p, end, &motivo))) goto mal;
            p = q; LTV_NEED(2 + 4 + 1);
            uint16_t slot = lt_u16(p); p += 2 + 4 + 1;
            if ((flags & 1) ? slot >= nm_own : slot >= desc_nm) { motivo = "ranura de metodo fuera de la clase"; goto mal; }
        }
        LTV_NEED(2); uint16_t n_cam = lt_u16(p); p += 2;
        for (uint16_t i = 0; i < n_cam; i++) {
            if (!(q = ltv_utf(p, end, &motivo))) goto mal;
            p = q; LTV_NEED(2 + 1 + 1);
            uint16_t slot = lt_u16(p); uint8_t ancho = p[2]; p += 2 + 1 + 1;
            /* V7/H1 #19: un array fijo `private var b: tipo[N]` ocupa 1 + N*ancho/4 ranuras (hasta 255) */
            if (ancho < 1 || ((flags & 1) ? slot + ancho > nf_own : slot + ancho > desc_nf)) { motivo = "ranura de campo fuera de la clase"; goto mal; }
        }
        LTV_NEED(2); uint16_t n_ev = lt_u16(p); p += 2;
        for (uint16_t i = 0; i < n_ev; i++) {
            if (!(q = ltv_utf(p, end, &motivo))) goto mal;
            p = q; LTV_NEED(2);
            uint16_t recv = lt_u16(p); p += 2;
            if ((flags & 1) ? recv + 3u > nf_own : recv + 3u > desc_nf) { motivo = "ranura de evento fuera de la clase"; goto mal; }
        }
    }
    return 0;
mal:
    snprintf(vm->load_error, sizeof vm->load_error, "'%s': subtabla CLASES corrupta (%s): .mod corrupto", mod->name, motivo ? motivo : "?");
    return BPVM_ERR_BAD_HEADER;
}
#undef LTV_NEED

/* V4 de #523 (17-sep) — E9 ANTES que los imports 3.1. Medido en mezcla.py: si el dueno es
 * MOD7/8, el import 3.1 del constructor con firma («no exporta 'Lib.__cls_new_Base(i)' ...
 * version vieja?») caia antes de que ninguna ref llegara a resolverse, y el mensaje que dice
 * lo que hay que hacer («recompila 'Lib'») no se veia nunca. Un consumidor MOD9 con una ref a
 * un modulo sin tabla de clases es E9, diga lo que diga la 3.1. Mismo orden que miVM. */
static bpvm_status_t lt_exigir_duenos_con_clases(bpvm_t* vm) {
    lt_ctx_t x; memset(&x, 0, sizeof x); x.vm = vm;
    for (int mi = 0; mi < vm->module_count; mi++) {
        const bpvm_module_t* m = &vm->modules[mi];
        for (int i = 0; i < m->ref_count; i++) {
            const bpvm_ref_t* r = &m->refs[i];
            if (r->dep_idx == BPVM_DEP_LOCAL) continue;
            char modc[160]; { const char* d = strrchr(r->clase, '.'); size_t l = d ? (size_t)(d - r->clase) : strlen(r->clase); if (l >= sizeof modc) l = sizeof modc - 1; memcpy(modc, r->clase, l); modc[l] = 0; }
            for (int k = 0; k < vm->module_count; k++) {
                char canon[160]; lt_canon(&vm->modules[k], canon, sizeof canon);
                if (strcmp(canon, modc) == 0 && !vm->modules[k].clases) {
                    lt_error(&x, "'%s' es MOD%d y '%s' necesita su tabla de clases: recompila '%s'", vm->modules[k].name, lt_version(&vm->modules[k]), m->name, vm->modules[k].name);
                    return BPVM_ERR_RUNTIME;
                }
            }
        }
    }
    return BPVM_OK;
}

/* ---- linkAll: resuelve imports y aplica class fixups ---- */

bpvm_status_t bpvm_link_all(bpvm_t* vm) {
    int64_t t0 = bpvm_platform_now_us();
    s_link_busquedas = 0; s_link_strcmp = 0;
    {
        bpvm_status_t st = lt_exigir_duenos_con_clases(vm);
        if (st != BPVM_OK) { bpvm_diag_urgente("[bpvm-c link] %s", vm->link_error); return BPVM_ERR_RUNTIME; }
    }
    for (int i = 0; i < vm->module_count; i++) {
        bpvm_module_t* m = &vm->modules[i];

        /* Resolver imports: m->imports[k] → ext_table_addr + k*4. */
        for (int k = 0; k < (int) m->import_count; k++) {
            const char* needed = m->imports[k];
            if (!needed || !needed[0]) continue;
            uint32_t addr = bpvm_link_lookup(vm, needed);
            if (addr == 0) {
                /* El loader registra el símbolo con y sin library-prefix, así
                 * que un lookup directo basta; si aún así falta, es lib ausente
                 * o rancia → mensaje claro al wire (paso 4). */
                bpvm_link_set_error(vm, needed, m->name);
                bpvm_diag_urgente("[bpvm-c link] %s", vm->link_error);
                return BPVM_ERR_RUNTIME;
            }
            bpvm_write_i32_be(vm->memory + m->ext_table_addr
                              + (uint32_t) k * 4, (int32_t) addr);
        }

        /* L2 v3 — aplicar class fixups del módulo. */
        for (int k = 0; k < m->class_fixup_count; k++) {
            bpvm_class_fixup_t* fx = &m->class_fixups[k];
            uint32_t parent_abs = bpvm_link_lookup(vm, fx->parent_qualified);
            if (parent_abs == 0) {
                bpvm_link_set_error(vm, fx->parent_qualified, m->name);
                bpvm_diag_urgente("[bpvm-c link] L2 v3 fixup (clase '%s'): %s",
                        fx->child_class_name, vm->link_error);
                return BPVM_ERR_RUNTIME;
            }
            uint32_t child_abs = m->code_start + fx->child_cs_off;
            int32_t parent_off_rel = (int32_t)(parent_abs - m->code_start);
            bpvm_write_i32_be(vm->memory + child_abs + BPVM_CLS_OFF_PARENT_OFF,
                              parent_off_rel);
        }

        /* BUG-2 — parcha el clsOff i32 de los TRY_BEGIN_EXT (catch cross-module).
         * H3.c: en un módulo XIP el operando vive en FLASH y no se puede
         * escribir → el valor resuelto queda en el fixup (tabla lateral) y
         * OP_TRY_BEGIN_EXT lo consulta por code_off (camino frío). */
        for (int k = 0; k < m->eh_class_fixup_count; k++) {
            bpvm_eh_class_fixup_t* fx = &m->eh_class_fixups[k];
            uint32_t parent_abs = bpvm_link_lookup(vm, fx->parent_qualified);
            if (parent_abs == 0) {
                bpvm_link_set_error(vm, fx->parent_qualified, m->name);
                bpvm_diag_urgente("[bpvm-c link] BUG-2 eh-fixup: %s", vm->link_error);
                return BPVM_ERR_RUNTIME;
            }
            int32_t cls_off = (int32_t)(parent_abs - m->code_start);
            if (m->cb != m->code_start) {          /* XIP: no tocar flash */
                fx->resolved_cls_off = cls_off;
                fx->resolved = 1;
            } else {
                bpvm_write_i32_be(vm->memory + m->code_start + (uint32_t) fx->code_off,
                                  cls_off);
            }
        }
    }

    /* V7/H2 (#523) — EL ENLACE TARDIO: clases abiertas contra su padre real, refs a la
     * ext-table, verificacion. Despues de los imports de TODOS los modulos (el mismo
     * orden que miVM: el primer error que se ve es el mismo en las dos VMs). */
    {
        bpvm_status_t st = bpvm_enlace_tardio(vm);
        if (st != BPVM_OK) { bpvm_diag_urgente("[bpvm-c link] %s", vm->link_error); return BPVM_ERR_RUNTIME; }
    }
    /* #449 — que la tabla DIGA lo que ocupa, igual que la de handles. Sale del
     * `malloc` de plataforma, que en la Pico 2 es un margen de 64 KB: cuando
     * este numero se acerca, el siguiente import no cabe. Sin esta linea el
     * consumidor mas grande del margen era invisible, y costo dos dias. */
    bpvm_diag("[bpvm] tabla de simbolos: %d simbolos, %u B de nombres, %u B en total",
              vm->symbol_count, (unsigned) vm->sym_pool_len,
              (unsigned)((size_t) vm->symbol_capacity * sizeof(bpvm_symbol_t)
                         + vm->sym_pool_cap));
    /* V7/H2 (#523, V0) — el cronometro del enlace, misma forma en las dos VMs. */
    bpvm_diag("[bpvm] enlace: %d modulos, %u busquedas, %u strcmp, %ld us",
              vm->module_count, s_link_busquedas, s_link_strcmp,
              (long) (bpvm_platform_now_us() - t0));
    return BPVM_OK;
}
