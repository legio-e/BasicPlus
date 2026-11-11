/*
 * bpvm_raton.c — V8/H2, paso 3 (9-nov): EL RATÓN DEL SO, lo común de la HAL BP. La interfaz y el porqué, en
 * bpvm_raton.h. El espejo, en la VM-Java: Raton.java (y el reparto, en VirtualMachine.repartirRaton): las dos VMs tienen
 * que repartir lo mismo y en el mismo orden.
 */
#include "bpvm_raton.h"
#include "bpvm_internal.h"
#include "bpvm_alloc.h"
#include "bpvm_bios.h"
#include "bpvm_bt.h"
#include "bpvm_pant.h"
#include "bpvm_platform.h"

#include <string.h>

typedef struct { int32_t tipo; int32_t dato; int32_t x; int32_t y; } aviso_t;

typedef struct {
    uint64_t ref;   /* el Mice.Mouse, con su generación: si muere, se borra (no lo mantiene vivo) */
    int32_t  tid;   /* el hilo que lo creó: el de sus eventos */
    int32_t  slot;  /* la ranura de su __despacha */
    uint32_t sig;   /* el número del siguiente aviso que le toca */
    int32_t  ux, uy;   /* la última posición que se le avisó */
    int      en_uso;
} apuntado_t;

typedef struct {
    int32_t    x, y;                         /* tal como está */
    uint32_t   botones;
    int32_t    w, h;                         /* paso 5: la pantalla de bpvm_raton_mueve (0: aún no se sabe) */
    aviso_t    avisos[BPVM_RATON_AVISOS];    /* los últimos, por su número de orden */
    uint32_t   avisos_fin;                   /* el número del siguiente que llegue */
    apuntado_t ap[BPVM_RATON_OBJETOS];
    struct { uint32_t sig; int usado; } lector[BPVM_RATON_LECTORES];   /* paso 4: los de C (LVGL) */
} raton_t;

static bpvm_platform_mutex_handle_t s_m;
static int s_listo = 0;
static raton_t* s_r = NULL;

/* El cerrojo y el estado, la primera vez. NULL si no hay memoria (y el ratón no hace nada). */
static raton_t* estado(void) {
    if (!s_listo) {
        if (bpvm_platform_mutex_init(&s_m) != 0) return NULL;
        s_listo = 1;
    }
    if (!s_r) {
        s_r = (raton_t*) bpvm_malloc_os(sizeof *s_r);
        if (!s_r) return NULL;
        memset(s_r, 0, sizeof *s_r);
    }
    return s_r;
}
static void coge(void)   { if (s_listo) bpvm_platform_mutex_lock(&s_m); }
static void suelta(void) { if (s_listo) bpvm_platform_mutex_unlock(&s_m); }

/* Con el cerrojo cogido: un aviso, en el punto donde está el ratón. */
static void aviso(raton_t* r, int32_t tipo, int32_t dato) {
    aviso_t* a = &r->avisos[r->avisos_fin % BPVM_RATON_AVISOS];
    a->tipo = tipo;
    a->dato = dato;
    a->x = r->x;
    a->y = r->y;
    r->avisos_fin++;
}

/* Con el cerrojo cogido: el punto, los botones (un aviso por cada uno que se aprieta ahora) y la rueda. */
static void pon(raton_t* r, int32_t x, int32_t y, uint32_t botones, int32_t rueda) {
    uint32_t nuevos, b;
    r->x = x;
    r->y = y;
    botones &= BPVM_RATON_BOTONES;
    nuevos = botones & ~r->botones;
    for (b = BPVM_RATON_IZQ; b <= BPVM_RATON_ADELANTE; b <<= 1)
        if (nuevos & b) aviso(r, BPVM_RATON_PULSA, (int32_t) b);   /* soltar no avisa, de momento */
    r->botones = botones;
    if (rueda != 0) aviso(r, BPVM_RATON_RUEDA, rueda);
}

void bpvm_raton_pon(int32_t x, int32_t y, uint32_t botones, int32_t rueda) {
    raton_t* r = estado();
    if (!r) return;
    coge();
    pon(r, x, y, botones, rueda);
    suelta();
}

/* ── V8/H2, paso 5: lo que manda un ratón de verdad, y su pantalla ─────────────────────────────────────────────────── */

static int32_t dentro(int32_t v, int32_t max) { return v < 0 ? 0 : v >= max ? max - 1 : v; }

/* Con el cerrojo cogido: ¿se sabe la pantalla? Si nadie la ha dicho, la del driver ('PANT', a 0°); la primera vez que
 * se sabe, el ratón va a su centro. Sin driver, no se guarda nada: se vuelve a mirar la próxima vez (en la C5 el driver
 * es un pack, y se carga con el primer RUN). Se pregunta cuando alguien empieza a usar el ratón (un Mice.Mouse que se
 * apunta, un lector de C), desde su hilo: NO desde el que lo mueve, la radio, que va con la pila justa (la tarea de
 * NimBLE, 4 KB) mientras que el driver puede leer el ENV y escribir en el log (el de la P4, p4_panel_select). */
static int con_pantalla(raton_t* r) {
    const bpvm_pack_api_t* api;
    bpvm_pant_t p;
    int w = 0, h = 0;
    if (r->w > 0 && r->h > 0) return 1;
    api = (const bpvm_pack_api_t*) bpvm_bios_busca(BPVM_PANT_MARCA);
    if (!api || bpvm_pant_resolver(api, &p) != 0 || p.size(&w, &h) != 1 || w <= 0 || h <= 0) return 0;
    r->w = w;
    r->h = h;
    r->x = w / 2;
    r->y = h / 2;
    return 1;
}

void bpvm_raton_mueve(int32_t dx, int32_t dy, uint32_t botones, int32_t rueda) {
    raton_t* r = estado();
    if (!r) return;
    coge();
    if (r->w > 0 && r->h > 0)   /* sin pantalla (o mientras nadie la ha preguntado), no hace nada (8-nov: la S3) */
        pon(r, dentro(r->x + dx, r->w), dentro(r->y + dy, r->h), botones, rueda);
    suelta();
}

void bpvm_raton_pantalla(int32_t w, int32_t h) {
    raton_t* r = estado();
    if (!r) return;
    coge();
    if (w > 0 && h > 0) {
        if (r->w <= 0 || r->h <= 0) { r->x = w / 2; r->y = h / 2; }   /* la primera vez, en su centro */
        else { r->x = dentro(r->x, w); r->y = dentro(r->y, h); }
        r->w = w;
        r->h = h;
    } else {
        r->w = r->h = 0;
    }
    suelta();
}

/* ── Preguntando: tal como está ────────────────────────────────────────────────────────────────────────────────────── */

int32_t bpvm_raton_x(void) {
    int32_t v;
    if (!s_r) return 0;
    coge(); v = s_r->x; suelta();
    return v;
}

int32_t bpvm_raton_y(void) {
    int32_t v;
    if (!s_r) return 0;
    coge(); v = s_r->y; suelta();
    return v;
}

uint32_t bpvm_raton_botones(void) {
    uint32_t v;
    if (!s_r) return 0;
    coge(); v = s_r->botones; suelta();
    return v;
}

/* ── Los ratones del programa y el reparto ─────────────────────────────────────────────────────────────────────────── */

int bpvm_raton_apunta(struct bpvm* vm, uint64_t ref, int tid, int slot) {
    raton_t* r = estado();
    int i, libre = -1;
    (void) vm;
    if (!r) return 0;
    coge();
    (void) con_pantalla(r);   /* paso 5: alguien va a usar el ratón; la pantalla, desde este hilo */
    for (i = 0; i < BPVM_RATON_OBJETOS; i++) {
        if (r->ap[i].en_uso && r->ap[i].ref == ref) { libre = i; break; }   /* el mismo, otra vez */
        if (!r->ap[i].en_uso && libre < 0) libre = i;
    }
    if (libre >= 0) {
        r->ap[libre].ref = ref;
        r->ap[libre].tid = tid;
        r->ap[libre].slot = slot;
        r->ap[libre].sig = r->avisos_fin;   /* los avisos que lleguen desde ahora */
        r->ap[libre].ux = r->x;             /* y está donde está el del SO */
        r->ap[libre].uy = r->y;
        r->ap[libre].en_uso = 1;
    }
    suelta();
    return libre >= 0;
}

void bpvm_raton_borra(uint64_t ref) {
    int i;
    if (!s_r) return;
    coge();
    for (i = 0; i < BPVM_RATON_OBJETOS; i++) if (s_r->ap[i].en_uso && s_r->ap[i].ref == ref) s_r->ap[i].en_uso = 0;
    suelta();
}

static bpvm_thread_t* hilo_de(bpvm_t* vm, int tid) {
    bpvm_thread_t* h;
    for (h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h)) if (h->id == tid) return h;
    return NULL;
}

/* ¿Tiene ya un evento suyo en la cola? Uno a la vez: la cola es de todos. */
static int en_la_cola(const bpvm_t* vm, uint64_t ref, int32_t slot) {
    int k;
    for (k = 0; k < vm->ev_count; k++) {
        const bpvm_event_t* e = &vm->ev_queue[(vm->ev_head + k) % BPVM_EVENT_QUEUE_CAP];
        if (e->recv == ref && e->dest == slot) return 1;
    }
    return 0;
}

/* Lo llama el planificador entre turno y turno, después del de las teclas y con lo mismo cogido: la cola es suya. */
void bpvm_raton_reparte(struct bpvm* vm) {
    int i;
    if (!s_r) return;   /* nadie ha usado el ratón */
    coge();
    for (i = 0; i < BPVM_RATON_OBJETOS; i++) {
        apuntado_t* a = &s_r->ap[i];
        bpref_t ref;
        bpvm_thread_t* h;
        int64_t args[4];
        int pend, avanza = 0;
        int32_t nx, ny;
        if (!a->en_uso) continue;
        pend = a->sig != s_r->avisos_fin;
        if (!pend && a->ux == s_r->x && a->uy == s_r->y) continue;   /* al día */
        ref.v = a->ref;
        if (bpvm_ref_dead(vm, ref)) { a->en_uso = 0; continue; }   /* el ratón ya no existe */
        h = hilo_de(vm, a->tid);
        if (!h || h->status == BPVM_THREAD_TERMINATED) { a->en_uso = 0; continue; }   /* ni su hilo */
        if (s_r->avisos_fin - a->sig > BPVM_RATON_AVISOS) a->sig = s_r->avisos_fin - BPVM_RATON_AVISOS;   /* atrás: pierde los viejos */
        if (en_la_cola(vm, a->ref, a->slot)) continue;
        if (vm->ev_count >= BPVM_EVENT_QUEUE_CAP) break;   /* la cola, llena: en el turno siguiente */
        if (pend) {
            const aviso_t* av = &s_r->avisos[a->sig % BPVM_RATON_AVISOS];
            nx = av->x;
            ny = av->y;
            if (a->ux != nx || a->uy != ny) {   /* primero, llegar adonde pasó */
                args[0] = BPVM_RATON_MUEVE; args[1] = 0;
            } else {
                args[0] = av->tipo; args[1] = av->dato;
                avanza = 1;
            }
        } else {
            nx = s_r->x;
            ny = s_r->y;
            args[0] = BPVM_RATON_MUEVE; args[1] = 0;
        }
        args[2] = nx;
        args[3] = ny;
        if (bpvm_event_enqueue(vm, a->tid, a->ref, a->slot, 4, 0u, args)) {
            a->ux = nx;
            a->uy = ny;
            if (avanza) a->sig++;
        }
    }
    suelta();
}

void bpvm_raton_fin_de_run(void) {
    int32_t x, y, w, h;
    uint32_t b;
    int i, usado[BPVM_RATON_LECTORES];
    if (!s_r) return;
    coge();
    x = s_r->x; y = s_r->y; b = s_r->botones;   /* la posición y los botones son del ratón, no del programa: se quedan */
    w = s_r->w; h = s_r->h;                     /* y la pantalla, de la placa */
    for (i = 0; i < BPVM_RATON_LECTORES; i++) usado[i] = s_r->lector[i].usado;
    memset(s_r, 0, sizeof *s_r);                /* lo del programa, fuera: sus ratones y lo que no se avisó */
    s_r->x = x; s_r->y = y; s_r->botones = b;
    s_r->w = w; s_r->h = h;
    for (i = 0; i < BPVM_RATON_LECTORES; i++) s_r->lector[i].usado = usado[i];   /* los de C siguen, desde cero */
    suelta();
}

/* ── V8/H2, paso 4: los lectores de C ──────────────────────────────────────────────────────────────────────────────── */

int bpvm_raton_lector_abre(void) {
    raton_t* r = estado();
    int i, id = -1;
    if (!r) return -1;
    coge();
    (void) con_pantalla(r);   /* paso 5: la GUI va a leer el ratón; la pantalla, desde su hilo */
    for (i = 0; i < BPVM_RATON_LECTORES; i++) {
        if (!r->lector[i].usado) {
            r->lector[i].usado = 1;
            r->lector[i].sig = r->avisos_fin;
            id = i;
            break;
        }
    }
    suelta();
    return id;
}

int bpvm_raton_lector_lee(int id, int32_t* x, int32_t* y, int* izq, int32_t* rueda) {
    uint32_t s;
    int pulsado = 0;
    int32_t muescas = 0;
    if (!s_r || id < 0 || id >= BPVM_RATON_LECTORES) return 0;
    coge();
    if (!s_r->lector[id].usado) { suelta(); return 0; }
    s = s_r->lector[id].sig;
    if (s_r->avisos_fin - s > BPVM_RATON_AVISOS) s = s_r->avisos_fin - BPVM_RATON_AVISOS;   /* atrás: los viejos, fuera */
    for (; s != s_r->avisos_fin; s++) {
        const aviso_t* a = &s_r->avisos[s % BPVM_RATON_AVISOS];
        if (a->tipo == BPVM_RATON_PULSA && a->dato == (int32_t) BPVM_RATON_IZQ) pulsado = 1;
        else if (a->tipo == BPVM_RATON_RUEDA) muescas += a->dato;
    }
    s_r->lector[id].sig = s;
    *x = s_r->x;
    *y = s_r->y;
    *izq = pulsado || (s_r->botones & BPVM_RATON_IZQ) != 0;
    *rueda = muescas;
    suelta();
    return 1;
}

/* ── El PC: el ratón de la ventana de la GUI ───────────────────────────────────────────────────────────────────────── */

void bpvm_raton_pc(int32_t x, int32_t y, uint32_t botones, int32_t rueda) {
    if (!bpvm_bt_hay_raton()) return;   /* sin un ratón conectado, la ventana no llega a Mice (como táctil, sigue igual) */
    bpvm_raton_pon(x, y, botones, rueda);
}
