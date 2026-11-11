/*
 * bpvm_hid.c — V8/H2, paso 5, la pieza 2 (11-nov): LO QUE MANDA UN TECLADO O UN RATÓN HID, al teclado y al ratón del
 * SO. La interfaz y el porqué, en bpvm_hid.h; las pruebas, en test_hid.c (make test-hid).
 *
 * El descriptor es una lista de «items» (la especificación HID 1.11, §6.2.2): los GLOBALES (la página de usos, el
 * tamaño y la cuenta de los campos, el número del informe...) valen hasta que otro los cambia; los LOCALES (los usos)
 * valen para el siguiente item PRINCIPAL (Input, Output, Collection...), que los gasta. Cada Input pone sus campos detrás
 * de los anteriores del mismo informe: así sale dónde está cada cosa.
 */
#include "bpvm_hid.h"
#include "bpvm_teclado.h"
#include "bpvm_raton.h"

#include <stdio.h>
#include <string.h>

#define PILA    4    /* Push y Pop de los globales */
#define USOS    16   /* los usos de un item principal */
#define SALIDAS 4    /* los informes de salida que se van midiendo */

typedef struct {
    uint32_t pagina;
    int32_t  lmin, lmax;
    uint32_t tam, cuenta;
    uint8_t  id;
} global_t;

typedef struct { uint8_t id; uint32_t bits; } salida_t;

typedef struct {
    global_t g, pila[PILA];
    int      npila;
    uint32_t usos[USOS];      /* con su página en los 16 bits altos */
    int      nusos;
    uint32_t umin, umax;      /* Usage Minimum y Maximum, con su página */
    int      rango;           /* 1, el mínimo; 2, el máximo; 3, los dos */
    salida_t sal[SALIDAS];
    int      nsal;
} recorrido_t;

/* Los bloqueos, en los bits del teclado del SO. Al arrancar, Bloq Num encendido. Sólo los toca quien recibe los
 * informes (bpvm_hid.h). */
static uint32_t s_bloq = BPVM_TECLA_BLOQ_NUM;

/* ── El descriptor ─────────────────────────────────────────────────────────────────────────────────────────────────── */

static void sin_campo(bpvm_hid_campo_t* c) { c->bit = BPVM_HID_SIN; c->bits = 0; c->con_signo = 0; }

static bpvm_hid_informe_t* informe_de(bpvm_hid_mapa_t* m, uint8_t id) {
    int i;
    bpvm_hid_informe_t* r;
    for (i = 0; i < m->n; i++) if (m->inf[i].id == id) return &m->inf[i];
    if (m->n >= BPVM_HID_INFORMES) return NULL;   /* de más: no se apunta */
    r = &m->inf[m->n++];
    memset(r, 0, sizeof *r);
    r->id = id;
    for (i = 0; i < 8; i++) r->mod[i] = BPVM_HID_SIN;
    for (i = 0; i < 5; i++) r->boton[i] = BPVM_HID_SIN;
    sin_campo(&r->teclas);
    sin_campo(&r->x);
    sin_campo(&r->y);
    sin_campo(&r->rueda);
    return r;
}

/* El uso del campo k: Usage Minimum + k; o los Usage en orden, y el último para los que sobran. */
static uint32_t uso_de(const recorrido_t* e, uint32_t k) {
    if (e->rango == 3) {
        uint32_t u = e->umin + k;
        return u > e->umax ? e->umax : u;
    }
    if (e->nusos == 0) return e->g.pagina << 16;
    return e->usos[k < (uint32_t) e->nusos ? k : (uint32_t) e->nusos - 1];
}

static uint16_t hasta(uint32_t v) { return (uint16_t) (v > 0xFFFEu ? 0xFFFEu : v); }

static void entrada(recorrido_t* e, bpvm_hid_mapa_t* m, uint32_t flags) {
    bpvm_hid_informe_t* r = informe_de(m, e->g.id);
    uint32_t k, tam = e->g.tam, cuenta = e->g.cuenta;
    if (!r) return;
    if (!(flags & 0x01)) {                                   /* datos (no una constante, que sólo rellena) */
        if (flags & 0x02) {                                  /* variable: un valor en cada campo */
            for (k = 0; k < cuenta; k++) {
                uint32_t u = uso_de(e, k), bit = r->bits + k * tam;
                uint16_t pag = (uint16_t) (u >> 16), id = (uint16_t) u;
                if (bit >= 0xFFFEu) break;
                if (pag == 0x07 && tam == 1) {               /* el teclado */
                    if (id >= 0xE0 && id <= 0xE7) r->mod[id - 0xE0] = (uint16_t) bit;
                    else m->mapa_de_bits = 1;                /* una tecla en cada bit (NKRO) */
                } else if (pag == 0x09 && tam == 1) {        /* los botones */
                    if (id >= 1 && id <= 5) r->boton[id - 1] = (uint16_t) bit;
                } else if (pag == 0x01 && (flags & 0x04) && tam >= 2 && tam <= 32) {   /* el escritorio, relativo */
                    bpvm_hid_campo_t* c = id == 0x30 ? &r->x : id == 0x31 ? &r->y : id == 0x38 ? &r->rueda : NULL;
                    if (c) { c->bit = (uint16_t) bit; c->bits = (uint8_t) tam; c->con_signo = e->g.lmin < 0; }
                }
            }
        } else if ((tam == 8 || tam == 16) && r->teclas.bits == 0) {   /* una lista: el código de cada tecla apretada */
            uint32_t u = (e->rango & 1) ? e->umin : uso_de(e, 0);
            if ((u >> 16) == 0x07 && (uint32_t) r->bits < 0xFFFEu) {
                r->teclas.bit = (uint16_t) r->bits;
                r->teclas.bits = (uint8_t) tam;
                r->nteclas = (uint8_t) (cuenta > BPVM_HID_TECLAS ? BPVM_HID_TECLAS : cuenta);
            }
        }
    }
    r->bits = hasta(r->bits + tam * cuenta);
}

static void salida(recorrido_t* e, bpvm_hid_mapa_t* m, uint32_t flags) {
    salida_t* s = NULL;
    uint32_t k;
    int i;
    for (i = 0; i < e->nsal; i++) if (e->sal[i].id == e->g.id) s = &e->sal[i];
    if (!s && e->nsal < SALIDAS) { s = &e->sal[e->nsal++]; s->id = e->g.id; s->bits = 0; }
    if (!s) return;
    if (!(flags & 0x01) && (flags & 0x02) && e->g.tam == 1) {
        for (k = 0; k < e->g.cuenta; k++) {
            uint32_t u = uso_de(e, k);
            uint16_t id = (uint16_t) u;
            /* Las luces (la página 0x08): 1, Bloq Num; 2, Bloq Mayús; 3, Bloq Despl. Las de un solo informe. */
            if ((u >> 16) == 0x08 && id >= 1 && id <= 3 && (!m->luces || m->luces_id == e->g.id)) {
                m->luces = 1;
                m->luces_id = e->g.id;
                m->luz[id - 1] = hasta(s->bits + k);
            }
        }
    }
    s->bits += e->g.tam * e->g.cuenta;
    if (m->luces && m->luces_id == s->id) m->luces_bits = hasta(s->bits);
}

int bpvm_hid_lee_mapa(const uint8_t* d, int n, bpvm_hid_mapa_t* m) {
    recorrido_t e;
    int i = 0, k;
    if (!m) return 0;
    memset(m, 0, sizeof *m);
    for (k = 0; k < 3; k++) m->luz[k] = BPVM_HID_SIN;
    memset(&e, 0, sizeof e);
    while (d && i < n) {
        uint8_t pre = d[i++];
        int tam, tipo, tag;
        uint32_t u = 0;
        int32_t s = 0;
        if (pre == 0xFE) {                                   /* un item largo (ninguno es nuestro): se salta */
            if (i + 1 >= n) break;
            i += 2 + d[i];
            continue;
        }
        tam = pre & 3;
        if (tam == 3) tam = 4;
        tipo = (pre >> 2) & 3;
        tag = pre >> 4;
        if (i + tam > n) break;
        for (k = 0; k < tam; k++) u |= (uint32_t) d[i + k] << (8 * k);
        s = tam == 1 ? (int32_t) (int8_t) u : tam == 2 ? (int32_t) (int16_t) u : (int32_t) u;
        i += tam;
        if (tipo == 1) {                                     /* global */
            switch (tag) {
            case 0x0: e.g.pagina = u & 0xFFFFu; break;
            case 0x1: e.g.lmin = s; break;
            case 0x2: e.g.lmax = s; break;
            case 0x7: e.g.tam = u; break;
            case 0x8: e.g.id = (uint8_t) u; break;
            case 0x9: e.g.cuenta = u; break;
            case 0xA: if (e.npila < PILA) e.pila[e.npila++] = e.g; break;   /* Push */
            case 0xB: if (e.npila > 0) e.g = e.pila[--e.npila]; break;      /* Pop */
            default: break;
            }
        } else if (tipo == 2) {                              /* local: con su página (la de ahora, si no la trae) */
            uint32_t uso = tam == 4 ? u : (e.g.pagina << 16) | (u & 0xFFFFu);
            if (tag == 0x0 && e.nusos < USOS) e.usos[e.nusos++] = uso;
            else if (tag == 0x1) { e.umin = uso; e.rango |= 1; }
            else if (tag == 0x2) { e.umax = uso; e.rango |= 2; }
        } else if (tipo == 0) {                              /* principal: gasta lo local */
            if (tag == 0x8) entrada(&e, m, u);
            else if (tag == 0x9) salida(&e, m, u);
            else if (tag == 0xA && u == 0x01 && m->clase == BPVM_HID_OTRO && e.nusos > 0) {
                /* Una colección de aplicación: la primera que es un teclado o un ratón dice la clase del aparato. */
                uint32_t uso = e.usos[e.nusos - 1];
                if ((uso >> 16) == 0x01) {
                    uint16_t id = (uint16_t) uso;
                    if (id == 0x06 || id == 0x07) m->clase = BPVM_HID_TECLADO;
                    else if (id == 0x02 || id == 0x01) m->clase = BPVM_HID_RATON;
                }
            }
            e.nusos = 0;
            e.rango = 0;
            e.umin = e.umax = 0;
        }
    }
    /* Lo que es cada informe, por lo que tiene. */
    for (k = 0; k < m->n; k++) {
        bpvm_hid_informe_t* r = &m->inf[k];
        int j, mods = 0;
        for (j = 0; j < 8; j++) if (r->mod[j] != BPVM_HID_SIN) mods = 1;
        r->que = BPVM_HID_OTRO;
        if (r->teclas.bits || mods) r->que |= BPVM_HID_TECLADO;
        if (r->x.bits && r->y.bits) r->que |= BPVM_HID_RATON;
    }
    return m->n;
}

const char* bpvm_hid_clase(const bpvm_hid_mapa_t* m) {
    if (m && m->clase == BPVM_HID_TECLADO) return "keyboard";
    if (m && m->clase == BPVM_HID_RATON) return "mouse";
    return "unknown";
}

int bpvm_hid_describe(const bpvm_hid_mapa_t* m, char* s, size_t n) {
    size_t o = 0;
    int i;
    if (!s || n == 0) return 0;
    s[0] = '\0';
    if (!m) return 0;
    for (i = 0; i < m->n && o < n; i++) {
        unsigned q = m->inf[i].que;
        const char* que = q == (BPVM_HID_TECLADO | BPVM_HID_RATON) ? "teclado y ratón"
                        : q == BPVM_HID_TECLADO ? "teclado" : q == BPVM_HID_RATON ? "ratón" : "otro";
        int w = snprintf(s + o, n - o, "%s%u %s", i ? ", " : "", (unsigned) m->inf[i].id, que);
        if (w < 0) break;
        o += (size_t) w;
    }
    if (m->luces && o < n) {
        int w = snprintf(s + o, n - o, "; luces en el %u", (unsigned) m->luces_id);
        if (w > 0) o += (size_t) w;
    }
    if (m->mapa_de_bits && o < n) {
        int w = snprintf(s + o, n - o, "; un teclado de mapa de bits (NKRO), que no se sabe leer");
        if (w > 0) o += (size_t) w;
    }
    return (int) (o < n ? o : n - 1);
}

/* ── Los informes ──────────────────────────────────────────────────────────────────────────────────────────────────── */

/* `bits` bits desde el `bit`, el bajo primero (el orden del HID). Un informe más corto de lo declarado: 0. */
static uint32_t lee_bits(const uint8_t* d, int n, uint32_t bit, uint32_t bits) {
    uint32_t v = 0, k;
    if (bits == 0 || bits > 32 || bit + bits > (uint32_t) n * 8u) return 0;
    for (k = 0; k < bits; k++) {
        uint32_t b = bit + k;
        if (d[b >> 3] & (1u << (b & 7))) v |= 1u << k;
    }
    return v;
}

static int32_t valor(const uint8_t* d, int n, const bpvm_hid_campo_t* c) {
    uint32_t v;
    if (c->bits == 0) return 0;
    v = lee_bits(d, n, c->bit, c->bits);
    if (c->con_signo && c->bits < 32 && (v & (1u << (c->bits - 1)))) v |= ~((1u << c->bits) - 1u);
    return (int32_t) v;
}

/* Un atajo (Ctrl, Alt o Windows) no da carácter, como en el PC; y el AltGr tampoco: el teclado americano no tiene esa
 * capa. */
#define SIN_TEXTO (BPVM_TECLA_CTRL_IZQ | BPVM_TECLA_CTRL_DER | BPVM_TECLA_ALT | BPVM_TECLA_WIN_IZQ | \
                   BPVM_TECLA_WIN_DER | BPVM_TECLA_ALTGR)
#define CON_MAYUS (BPVM_TECLA_SHIFT_IZQ | BPVM_TECLA_SHIFT_DER)

static int teclado(const bpvm_hid_informe_t* r, const uint8_t* d, int n, bpvm_hid_teclado_t* t) {
    uint8_t ahora[BPVM_HID_TECLAS];
    uint32_t mods = 0;
    int na = 0, i, j, luces = 0;
    for (i = 0; i < 8; i++)
        if (r->mod[i] != BPVM_HID_SIN && lee_bits(d, n, r->mod[i], 1)) mods |= 1u << i;
    for (i = 0; i < r->nteclas; i++) {
        uint32_t c = lee_bits(d, n, (uint32_t) r->teclas.bit + (uint32_t) i * r->teclas.bits, r->teclas.bits);
        if (c >= 0x01 && c <= 0x03) return 0;                /* ErrorRollOver: más de las que caben; no vale */
        if (c < 0x04 || c > 0xFF || (c >= 0xE0 && c <= 0xE7)) continue;   /* nada, o un modificador */
        ahora[na++] = (uint8_t) c;
    }
    for (i = 0; i < na; i++) {
        uint32_t est;
        int32_t cp = 0;
        for (j = 0; j < t->n; j++) if (t->teclas[j] == ahora[i]) break;
        if (j < t->n) continue;                              /* ya estaba apretada: mantener no repite */
        if (ahora[i] == 0x39) { s_bloq ^= BPVM_TECLA_BLOQ_MAYUS; luces = 1; }
        else if (ahora[i] == 0x53) { s_bloq ^= BPVM_TECLA_BLOQ_NUM; luces = 1; }
        else if (ahora[i] == 0x47) { s_bloq ^= BPVM_TECLA_BLOQ_DESPL; luces = 1; }
        est = (mods << 16) | s_bloq;
        if (!(est & SIN_TEXTO)) {
            int mayus = (est & CON_MAYUS) != 0;
            if (ahora[i] >= 0x04 && ahora[i] <= 0x1D && (est & BPVM_TECLA_BLOQ_MAYUS)) mayus = !mayus;   /* letras */
            cp = bpvm_hid_caracter(ahora[i], mayus, (est & BPVM_TECLA_BLOQ_NUM) != 0);
        }
        bpvm_teclado_pon((int32_t) ((uint32_t) ahora[i] | est), cp);
    }
    t->n = (uint8_t) na;
    memcpy(t->teclas, ahora, (size_t) na);
    return luces;
}

static void raton(const bpvm_hid_informe_t* r, const uint8_t* d, int n) {
    uint32_t botones = 0;
    int i;
    for (i = 0; i < 5; i++)
        if (r->boton[i] != BPVM_HID_SIN && lee_bits(d, n, r->boton[i], 1)) botones |= 1u << i;
    bpvm_raton_mueve(valor(d, n, &r->x), valor(d, n, &r->y), botones, valor(d, n, &r->rueda));
}

int bpvm_hid_informe(const bpvm_hid_mapa_t* m, uint8_t id, const uint8_t* d, int n, bpvm_hid_teclado_t* t) {
    int i, luces = 0;
    if (!m || !d || n <= 0) return 0;
    for (i = 0; i < m->n; i++) {
        const bpvm_hid_informe_t* r = &m->inf[i];
        if (r->id != id) continue;
        if ((r->que & BPVM_HID_TECLADO) && t) luces = teclado(r, d, n, t);
        if (r->que & BPVM_HID_RATON) raton(r, d, n);
        break;
    }
    return luces;
}

int bpvm_hid_luces(const bpvm_hid_mapa_t* m, uint8_t* d, int max) {
    static const uint32_t BLOQ[3] = { BPVM_TECLA_BLOQ_NUM, BPVM_TECLA_BLOQ_MAYUS, BPVM_TECLA_BLOQ_DESPL };
    int largo, i;
    if (!m || !m->luces || !d) return 0;
    largo = (m->luces_bits + 7) / 8;
    if (largo <= 0 || largo > max) return 0;
    memset(d, 0, (size_t) largo);
    for (i = 0; i < 3; i++)
        if (m->luz[i] != BPVM_HID_SIN && (uint32_t) m->luz[i] < (uint32_t) largo * 8u && (s_bloq & BLOQ[i]))
            d[m->luz[i] >> 3] |= (uint8_t) (1u << (m->luz[i] & 7));
    return largo;
}

/* ── La tabla del teclado americano ────────────────────────────────────────────────────────────────────────────────── */

int32_t bpvm_hid_caracter(uint16_t c, int mayus, int bloq_num) {
    /* 0x2D a 0x38, los signos; la 0x32 (la «#» de los teclados europeos) y la 0x64 (la tecla de más, junto a la Z),
     * como en el teclado americano de Windows: la barra invertida. */
    static const char SIGNOS[]      = "-=[]\\\\;'`,./";
    static const char SIGNOS_MAYUS[] = "_+{}||:\"~<>?";
    static const char NUMEROS[]      = "1234567890";
    static const char NUMEROS_MAYUS[] = "!@#$%^&*()";
    if (c >= 0x04 && c <= 0x1D) return (mayus ? 'A' : 'a') + (c - 0x04);
    if (c >= 0x1E && c <= 0x27) return mayus ? NUMEROS_MAYUS[c - 0x1E] : NUMEROS[c - 0x1E];
    if (c == 0x2C) return ' ';
    if (c >= 0x2D && c <= 0x38) return mayus ? SIGNOS_MAYUS[c - 0x2D] : SIGNOS[c - 0x2D];
    if (c == 0x64) return mayus ? '|' : '\\';
    if (c >= 0x54 && c <= 0x57) return "/*-+"[c - 0x54];                  /* el teclado numérico */
    if (c == 0x67) return '=';
    if (c >= 0x59 && c <= 0x62) return bloq_num ? NUMEROS[c - 0x59] : 0;   /* 1 a 9 y 0, con Bloq Num */
    if (c == 0x63) return bloq_num ? '.' : 0;
    return 0;
}

uint32_t bpvm_hid_bloqueos(void) { return s_bloq; }

void bpvm_hid_reinicia(void) { s_bloq = BPVM_TECLA_BLOQ_NUM; }
