/*
 * bpvm_teclado.c — V8/H2, paso 2 (9-nov): EL TECLADO DEL SO, lo común de la HAL BP. La interfaz y el porqué, en
 * bpvm_teclado.h. El espejo, en la VM-Java: Teclado.java (y el reparto, en VirtualMachine.repartirTeclas): las dos VMs
 * tienen que repartir lo mismo y en el mismo orden.
 */
#include "bpvm_teclado.h"
#include "bpvm_internal.h"
#include "bpvm_alloc.h"
#include "bpvm_bt.h"
#include "bpvm_platform.h"

#include <string.h>

typedef struct { int32_t k; int32_t cp; } tecla_t;

typedef struct {
    uint64_t ref;   /* el Keyboards.Keyboard, con su generación: si muere, se borra (no lo mantiene vivo) */
    int32_t  tid;   /* el hilo que lo creó: el de sus eventos */
    int32_t  slot;  /* la ranura de su __despacha */
    uint32_t sig;   /* el número de la siguiente tecla que le toca */
    int      en_uso;
} apuntado_t;

typedef struct {
    tecla_t    bufer[BPVM_TECLADO_BUFER];   /* lo que lee readKey */
    int        bufer_ini, bufer_n;
    tecla_t    avisos[BPVM_TECLADO_AVISOS]; /* las últimas, por su número de orden */
    uint32_t   avisos_fin;                  /* el número de la siguiente que llegue */
    apuntado_t ap[BPVM_TECLADO_OBJETOS];
    struct { int32_t tid; int32_t cp; int usado; } ultimo[BPVM_TECLADO_HILOS];   /* el carácter de la última leída, por hilo */
    int        ultimo_sig;                  /* dónde va el de un hilo nuevo (rueda) */
    int        pend;                        /* el PC: una pulsación que espera su texto */
    int32_t    pend_k, pend_codigo;
} teclado_t;

static bpvm_platform_mutex_handle_t s_m;
static int s_listo = 0;
static teclado_t* s_t = NULL;

/* El cerrojo y el estado, la primera vez. NULL si no hay memoria (y el teclado no hace nada). */
static teclado_t* estado(void) {
    if (!s_listo) {
        if (bpvm_platform_mutex_init(&s_m) != 0) return NULL;
        s_listo = 1;
    }
    if (!s_t) {
        s_t = (teclado_t*) bpvm_malloc_os(sizeof *s_t);
        if (!s_t) return NULL;
        memset(s_t, 0, sizeof *s_t);
    }
    return s_t;
}
static void coge(void)   { if (s_listo) bpvm_platform_mutex_lock(&s_m); }
static void suelta(void) { if (s_listo) bpvm_platform_mutex_unlock(&s_m); }

/* Con el cerrojo cogido. */
static void pon_en(teclado_t* t, int32_t k, int32_t cp) {
    if (t->bufer_n < BPVM_TECLADO_BUFER) {
        t->bufer[(t->bufer_ini + t->bufer_n) % BPVM_TECLADO_BUFER].k = k;
        t->bufer[(t->bufer_ini + t->bufer_n) % BPVM_TECLADO_BUFER].cp = cp;
        t->bufer_n++;
    }   /* lleno: la que llega se pierde, como en un BIOS */
    t->avisos[t->avisos_fin % BPVM_TECLADO_AVISOS].k = k;
    t->avisos[t->avisos_fin % BPVM_TECLADO_AVISOS].cp = cp;
    t->avisos_fin++;
}

void bpvm_teclado_pon(int32_t k, int32_t cp) {
    teclado_t* t = estado();
    if (!t) return;
    coge();
    pon_en(t, k, cp);
    suelta();
}

/* ── Preguntando ───────────────────────────────────────────────────────────────────────────────────────────────────── */

int bpvm_teclado_hay(void) {
    int n;
    if (!s_t) return 0;
    coge(); n = s_t->bufer_n; suelta();
    return n > 0;
}

int32_t bpvm_teclado_lee(int tid) {
    int32_t k = 0, cp = 0;
    int i, j = -1;
    if (!s_t) return 0;
    coge();
    if (s_t->bufer_n > 0) {
        k = s_t->bufer[s_t->bufer_ini].k;
        cp = s_t->bufer[s_t->bufer_ini].cp;
        s_t->bufer_ini = (s_t->bufer_ini + 1) % BPVM_TECLADO_BUFER;
        s_t->bufer_n--;
    }
    /* el carácter, para getChar: el sitio de ese hilo, o uno nuevo, por turno */
    for (i = 0; i < BPVM_TECLADO_HILOS; i++) if (s_t->ultimo[i].usado && s_t->ultimo[i].tid == tid) { j = i; break; }
    if (j < 0) { j = s_t->ultimo_sig; s_t->ultimo_sig = (s_t->ultimo_sig + 1) % BPVM_TECLADO_HILOS; }
    s_t->ultimo[j].tid = tid;
    s_t->ultimo[j].cp = cp;
    s_t->ultimo[j].usado = 1;
    suelta();
    return k;
}

int32_t bpvm_teclado_caracter(int tid) {
    int32_t cp = 0;
    int i;
    if (!s_t) return 0;
    coge();
    for (i = 0; i < BPVM_TECLADO_HILOS; i++) if (s_t->ultimo[i].usado && s_t->ultimo[i].tid == tid) { cp = s_t->ultimo[i].cp; break; }
    suelta();
    return cp;
}

/* ── Los teclados del programa y el reparto ────────────────────────────────────────────────────────────────────────── */

int bpvm_teclado_apunta(struct bpvm* vm, uint64_t ref, int tid, int slot) {
    teclado_t* t = estado();
    int i, libre = -1;
    (void) vm;
    if (!t) return 0;
    coge();
    for (i = 0; i < BPVM_TECLADO_OBJETOS; i++) {
        if (t->ap[i].en_uso && t->ap[i].ref == ref) { libre = i; break; }   /* el mismo, otra vez */
        if (!t->ap[i].en_uso && libre < 0) libre = i;
    }
    if (libre >= 0) {
        t->ap[libre].ref = ref;
        t->ap[libre].tid = tid;
        t->ap[libre].slot = slot;
        t->ap[libre].sig = t->avisos_fin;   /* las teclas que lleguen desde ahora */
        t->ap[libre].en_uso = 1;
    }
    suelta();
    return libre >= 0;
}

void bpvm_teclado_borra(uint64_t ref) {
    int i;
    if (!s_t) return;
    coge();
    for (i = 0; i < BPVM_TECLADO_OBJETOS; i++) if (s_t->ap[i].en_uso && s_t->ap[i].ref == ref) s_t->ap[i].en_uso = 0;
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

/* Lo llama el planificador entre turno y turno, donde saca los eventos y con lo mismo cogido: la cola es suya. */
void bpvm_teclado_reparte(struct bpvm* vm) {
    int i;
    if (!s_t) return;   /* nadie ha usado el teclado */
    coge();
    for (i = 0; i < BPVM_TECLADO_OBJETOS; i++) {
        apuntado_t* a = &s_t->ap[i];
        bpref_t r;
        bpvm_thread_t* h;
        int64_t args[2];
        if (!a->en_uso || a->sig == s_t->avisos_fin) continue;   /* libre, o al día */
        r.v = a->ref;
        if (bpvm_ref_dead(vm, r)) { a->en_uso = 0; continue; }   /* el teclado ya no existe */
        h = hilo_de(vm, a->tid);
        if (!h || h->status == BPVM_THREAD_TERMINATED) { a->en_uso = 0; continue; }   /* ni su hilo */
        if (s_t->avisos_fin - a->sig > BPVM_TECLADO_AVISOS) a->sig = s_t->avisos_fin - BPVM_TECLADO_AVISOS;   /* atrás: pierde las viejas */
        if (en_la_cola(vm, a->ref, a->slot)) continue;
        if (vm->ev_count >= BPVM_EVENT_QUEUE_CAP) break;   /* la cola, llena: en el turno siguiente */
        args[0] = s_t->avisos[a->sig % BPVM_TECLADO_AVISOS].k;
        args[1] = s_t->avisos[a->sig % BPVM_TECLADO_AVISOS].cp;
        if (bpvm_event_enqueue(vm, a->tid, a->ref, a->slot, 2, 0u, args)) a->sig++;
    }
    suelta();
}

void bpvm_teclado_fin_de_run(void) {
    if (!s_t) return;
    coge();
    memset(s_t, 0, sizeof *s_t);   /* lo del programa, fuera: sus teclados, lo que no leyó y lo que no se avisó */
    suelta();
}

/* ── El PC: las teclas de la ventana de la GUI ─────────────────────────────────────────────────────────────────────── */

static int es_modificador(int32_t c) { return c >= 0xE0 && c <= 0xE7; }

/* ¿Puede dar un carácter? Las letras, los números, el espacio, los signos, la tecla de más de los teclados europeos y el
 * teclado numérico (sus números, sólo con Bloq Num). */
static int puede_dar_texto(int32_t c, uint32_t estado_) {
    if (c >= 0x59 && c <= 0x63) return (estado_ & BPVM_TECLA_BLOQ_NUM) != 0;
    return (c >= 0x04 && c <= 0x27) || c == 0x2C || (c >= 0x2D && c <= 0x38) || c == 0x64
        || (c >= 0x54 && c <= 0x57) || c == 0x67;
}

/* Un atajo (Ctrl, Alt o Windows; no AltGr) no da carácter: sale en seguida. */
static int con_atajo(uint32_t e) {
    return (e & (BPVM_TECLA_CTRL_IZQ | BPVM_TECLA_CTRL_DER | BPVM_TECLA_ALT | BPVM_TECLA_WIN_IZQ
               | BPVM_TECLA_WIN_DER)) != 0;
}

/* El AltGr de Windows trae un Ctrl izquierdo de más: un teclado de verdad no lo manda. */
static uint32_t limpia(uint32_t e) {
    e &= ~BPVM_TECLA_CODIGO;
    if (e & BPVM_TECLA_ALTGR) e &= ~BPVM_TECLA_CTRL_IZQ;
    return e;
}

void bpvm_teclado_pc_pulsada(int32_t codigo, uint32_t estado_) {
    teclado_t* t;
    int32_t k;
    if (codigo <= 0 || codigo > 0xFFFF || es_modificador(codigo)) return;   /* los modificadores, solos, no son teclas */
    if (!bpvm_bt_hay_teclado()) return;   /* sin un teclado conectado, la ventana no teclea */
    t = estado();
    if (!t) return;
    estado_ = limpia(estado_);
    k = (int32_t) ((uint32_t) codigo | estado_);
    coge();
    if (t->pend) { pon_en(t, t->pend_k, 0); t->pend = 0; }   /* la anterior no dio texto */
    if (puede_dar_texto(codigo, estado_) && !con_atajo(estado_)) {
        t->pend = 1; t->pend_k = k; t->pend_codigo = codigo;   /* espera a su carácter */
    } else {
        pon_en(t, k, 0);
    }
    suelta();
}

void bpvm_teclado_pc_texto(int32_t cp, uint32_t estado_) {
    teclado_t* t;
    if (cp <= 0) return;
    if (!bpvm_bt_hay_teclado()) return;
    t = estado();
    if (!t) return;
    coge();
    if (t->pend) { pon_en(t, t->pend_k, cp); t->pend = 0; }
    else pon_en(t, (int32_t) limpia(estado_), cp);   /* un carácter sin tecla (lo compone el sistema): código 0 */
    suelta();
}

void bpvm_teclado_pc_soltada(int32_t codigo) {
    if (!s_t) return;
    coge();
    if (s_t->pend && s_t->pend_codigo == codigo) { pon_en(s_t, s_t->pend_k, 0); s_t->pend = 0; }
    suelta();
}
