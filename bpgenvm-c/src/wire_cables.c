/*
 * wire_cables.c — V8/H3 (6-nov): DOS CABLES A LA VEZ. La interfaz y el porqué, en bpvm_wire_cables.h; lo que tiene que hacer
 * cada función, en bpvm_wire_v1.h.
 *
 * Sólo lee UNA tarea, la del REPL —también durante un RUN, que hace de `io`—, y por eso lo de leer no lleva cerrojo. Escriben
 * varias: ella, `io` y el depurador.
 */
#ifdef BPVM_WIRE_CABLES

#include "bpvm_wire_cables.h"
#include "bpvm_log.h"
#include "bpvm_platform.h"
#include "bpvm_repl.h"
#include "bpvm_wire_v1.h"

#define ESPERA_SOLO_MS      100   /* sin otro cable que mirar */
#define ESPERA_CON_OTRO_MS   20   /* lo que tarda en mirarse el otro: el activo no pierde tiempo */
#define ESPERA_BYTE_MS      100   /* dentro de una línea */
#define ESPERA_BULK_MS      200   /* dentro de un bulk */

typedef const bpvm_wire_cable_t* cable_t;

static cable_t s_local = 0;
static cable_t s_red   = 0;
static cable_t volatile s_activo = 0;   /* a quién se contesta */
static cable_t s_leyendo = 0;           /* de quién era el `{` que dio wire_v1_try_getchar */

/* LA LINEA ES ATOMICA, y lo exige el contrato (bpvm_wire_v1.h). Los escritores son reales: `io` saca los OUTPUT mientras la
 * tarea del REPL contesta —el poll atiende HELLO en caliente, durante un RUN—; sin cerrojo, dos JSON entrelazados en el IDE.
 * Y lo es del cable, no de la VM: el `tx_mtx` de bpvm_io.c no cubre el poll, y además vive dentro de la VM. Creado al
 * escribir por primera vez, con una sola tarea viva: el arranque. */
static bpvm_platform_mutex_handle_t s_tx;
static int s_tx_listo = 0;

void bpvm_wire_cables_bloquear(void) {
    if (!s_tx_listo) {
        if (bpvm_platform_mutex_init(&s_tx) != 0) return;
        s_tx_listo = 1;
    }
    bpvm_platform_mutex_lock(&s_tx);
}

void bpvm_wire_cables_soltar(void) {
    if (s_tx_listo) bpvm_platform_mutex_unlock(&s_tx);
}

void bpvm_wire_cables_local(const bpvm_wire_cable_t* c) {
    s_local = c;
    if (!s_activo) s_activo = c;
}

void bpvm_wire_cables_red(const bpvm_wire_cable_t* c) { s_red = c; }

static void habla(cable_t c) {
    bpvm_wire_cables_bloquear();
    s_activo = c;
    bpvm_wire_cables_soltar();
    log_printf("wire: ahora habla %s", c->nombre);
}

/* Lo que se le mandaba al de antes, cortado, y a éste. El aviso, antes de cambiar: si tiene algo que decir, es a él. */
static void cambia_a(cable_t c) {
    bpvm_repl_cliente_ido();
    habla(c);
}

/* El cable de la red a lo suyo. Si su cliente se va y era el que hablaba, habla el local. */
static void sondear(void) {
    if (s_red && s_red->sondeo && s_red->sondeo() == BPVM_WIRE_CLIENTE_IDO && s_activo == s_red && s_local) cambia_a(s_local);
}

static int abierto(cable_t c) { return c && (!c->abierto || c->abierto()); }
static cable_t otro(cable_t c) { return (c == s_local) ? s_red : s_local; }

/* Un `{` de este cable, si lo hay: lo que venga antes es ruido, y se tira aquí mismo. Ruido en un cable no hace esperar al
 * otro; esperar a un '\n' por un byte suelto sería perder hasta 3 s. */
static int busca_mensaje(cable_t c, int ms) {
    uint8_t b;
    do {
        if (c->leer(&b, 1, ms) != 1) return 0;
        ms = 0;
    } while (b != '{');
    s_leyendo = c;
    return 1;
}

int wire_v1_try_getchar(void) {
    cable_t a;
    sondear();
    a = s_activo;
    if (a && busca_mensaje(a, 0)) return '{';
    if (abierto(otro(a)) && busca_mensaje(otro(a), 0)) return '{';
    return -1;
}

/* Sin mensaje a la vista: espera en el activo, y mira el otro sin esperar. */
static void espera_mensaje(void) {
    for (;;) {
        cable_t a, o;
        int hay_otro;
        sondear();
        a = s_activo;
        o = otro(a);
        hay_otro = abierto(o);
        if (busca_mensaje(a, hay_otro ? ESPERA_CON_OTRO_MS : ESPERA_SOLO_MS)) return;
        if (hay_otro && busca_mensaje(o, 0)) return;
    }
}

int wire_v1_recv_line(int first_char_already_read, char* buf, size_t buf_max) {
    cable_t c;
    size_t n = 0;
    int64_t ultimo;
    if (first_char_already_read < 0) espera_mensaje();
    c = s_leyendo;
    if (n + 1 >= buf_max) return -1;
    buf[n++] = '{';
    ultimo = bpvm_platform_now_ms();
    for (;;) {
        uint8_t b;
        int r = c->leer(&b, 1, ESPERA_BYTE_MS);
        if (r < 0) return 0;              /* se ha ido a mitad de la línea: no ha llegado nada */
        if (r == 0) {
            /* Sin plazo, un truncado se come el mensaje siguiente: el porqué, en WIRE_V1_ESTANCADA_MS. */
            if (bpvm_platform_now_ms() - ultimo >= (int64_t) WIRE_V1_ESTANCADA_MS) return -2;
            continue;
        }
        ultimo = bpvm_platform_now_ms();
        if (b == '\n') {
            if (c != s_activo) cambia_a(c);
            return (int) n;
        }
        if (b == '\r') continue;
        if (n + 1 >= buf_max) return -1;
        buf[n++] = (char) b;
    }
}

int wire_v1_recv_bulk(uint8_t* buf, size_t n, size_t buf_max) {
    cable_t c = s_activo;   /* el que mandó la cabecera */
    size_t got = 0;
    int64_t ultimo = bpvm_platform_now_ms();
    if (n > buf_max) return -1;
    while (got < n) {
        int r = c->leer(buf + got, n - got, ESPERA_BULK_MS);
        if (r < 0) return -1;
        if (r > 0) { got += (size_t) r; ultimo = bpvm_platform_now_ms(); continue; }
        if (bpvm_platform_now_ms() - ultimo >= (int64_t) WIRE_V1_BULK_ESTANCADO_MS) return -2;
    }
    return (int) n;
}

/* La línea, su '\n' y sus bytes, de una vez y al mismo cable. */
void wire_v1_send_msg(const char* line, size_t len, const uint8_t* bulk, size_t n) {
    cable_t c;
    bpvm_wire_cables_bloquear();
    c = s_activo;
    if (c) {
        if (len) c->escribir((const uint8_t*) line, len);
        c->escribir((const uint8_t*) "\n", 1);
        if (n) c->escribir(bulk, n);
        if (c->fin) c->fin();
    }
    bpvm_wire_cables_soltar();
}

void wire_v1_send_line(const char* data, size_t len) { wire_v1_send_msg(data, len, 0, 0); }

void wire_v1_send_bulk(const uint8_t* data, size_t n) {
    cable_t c;
    if (!n) return;
    bpvm_wire_cables_bloquear();
    c = s_activo;
    if (c) {
        c->escribir(data, n);
        if (c->fin) c->fin();
    }
    bpvm_wire_cables_soltar();
}

#else
typedef int wire_cables_no_es_de_esta_imagen;   /* ISO C no admite una unidad vacía */
#endif /* BPVM_WIRE_CABLES */
