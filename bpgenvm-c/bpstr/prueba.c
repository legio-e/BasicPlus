/*
 * prueba.c — las pruebas de bpstr (V8 #730): cada función, contra los bucles de la VM de antes (el ORÁCULO: copiados
 * tal cual de include/bpvm_internal.h y src/builtins.c, con su nombre de entonces).
 *
 *   1. exhaustivo: todas las cadenas de 0 a 3 bytes (16,8 millones), en las cuatro alineaciones por turno;
 *   2. buscar: con un alfabeto pequeño (a, b, 0x80, 0xC3), todos los pajares hasta 7 bytes con todas las agujas hasta
 *      4; con el binario (a, b), hasta 12 y 8; agujas largas y periódicas; y las de 3 a 300 bytes en tres alfabetos (la
 *      frontera de 256, byte a byte). Compilada con BPSTR_AGUJA_CORTA=1 y BPSTR_AGUJA_PAREJAS=0, todo va por Two-Way;
 *      con un tope de la corta muy grande, todo por la corta; y con la corta y Two-Way hasta 2, de 3 a 256 bytes por
 *      parejas: el Makefile pasa las cuatro, y la de siempre con los topes de la arquitectura;
 *   3. comparar, copiar y rellenar: todas las parejas cortas, con las 16 combinaciones de alineación;
 *   4. los bordes: de 0 a 64 bytes con la cadena pegada al final de una página protegida (y una al principio de otra):
 *      leer un byte de más revienta la prueba (en el PC; en una placa no hay páginas);
 *   5. al azar, con una semilla fija: casi todo ASCII, con caracteres largos y trozos malos;
 *   6. el catálogo de UTF-8 de Markus Kuhn (third_party/utf8-kuhn), entero y línea a línea;
 *   7. con "larga": además, el validador de #711 con TODAS las cadenas de 4 bytes (4 300 millones).
 *
 *   prueba [larga] [ruta del catálogo de Kuhn]
 *
 * En una placa (BPSTR_PLACA: una imagen de prueba de la VM, con placa.c): las mismas, más cortas —la RAM y el tiempo
 * no dan para las del PC—: de las cadenas de 3 bytes, una de cada 4099; los pajares y las agujas, más pequeños; 3000 al
 * azar de hasta 1500 bytes; sin los bordes (no hay páginas protegidas); el catálogo de Kuhn, del FS de la placa; y lo
 * que dicen, línea a línea, a placa.c (/app/bpstr.txt). Se entra por bpstr_prueba_main.
 */
#include "bpstr.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef BPSTR_PLACA
void bpstr_banco_linea(const char* s);                               /* placa.c */
long bpstr_placa_lee(const char* ruta, uint8_t* dst, uint32_t cap);   /* placa.c: un fichero del FS de la placa */
uint64_t bpstr_banco_ns(void);                                         /* placa.c: el reloj */
int  bpstr_prueba_main(int larga, const char* ruta_kuhn);
#define BUF_N       24576u   /* cabe el catálogo de Kuhn (22 781 bytes) */
#define SAL_N       8192u
#define AZAR_VECES  3000u
#define AZAR_MAX    1500u
#define PASO_3      4099u    /* de las cadenas de 3 bytes, una de cada tantas */
#define BUSCA4_P    5u
#define BUSCA4_Q    3u
#define BUSCA2_P    10u
#define BUSCA2_Q    6u
#define PAJAR_AGUJAS 700u
#define PASO_AGUJAS 13u      /* de las agujas de 40 a 300, una de cada tantas (las de 248 a 259, todas) */
#define VUELTAS_PERIODO 6u   /* agujas de periodo de más de 255 bytes */
#else
#define BUF_N       80000u
#define SAL_N       70000u
#define AZAR_VECES  200000u
#define AZAR_MAX    4000u
#define PASO_3      1u
#define BUSCA4_P    7u
#define BUSCA4_Q    4u
#define BUSCA2_P    12u
#define BUSCA2_Q    8u
#define PAJAR_AGUJAS 1200u
#define PASO_AGUJAS 7u
#define VUELTAS_PERIODO 300u
#endif

/* Una línea de lo que dice la prueba: en el PC, a la salida; en una placa, a placa.c. */
static void di(const char* fmt, ...) {
    char l[300];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(l, sizeof l, fmt, ap);
    va_end(ap);
#ifdef BPSTR_PLACA
    bpstr_banco_linea(l);
#else
    fputs(l, stdout);
    fputc('\n', stdout);
    fflush(stdout);
#endif
}

/* En una placa, por dónde va y cuánto lleva: una línea por paso largo (en el PC, nada). */
#ifdef BPSTR_PLACA
static uint64_t t_inicio;
static void progreso(const char* fmt, ...) {
    char l[200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(l, sizeof l, fmt, ap);
    va_end(ap);
    di("  ... %s (%lu ms)", l, (unsigned long) ((bpstr_banco_ns() - t_inicio) / 1000000u));
}
#define PROGRESO(...) progreso(__VA_ARGS__)
#else
#define PROGRESO(...) ((void) 0)
#endif

/* Los topes de buscar (BPSTR_AGUJA_CORTA, _DOS y _PAREJAS), los de bpstr.h: el Makefile pasa los mismos a los dos. */

/* El oráculo, los bucles de la VM de antes: oraculo.h (el mismo que usa el banco). */
#include "oraculo.h"

/* ═══════════════════════════════ LOS FALLOS ═════════════════════════════ */

static unsigned long long comprobaciones = 0, fallos = 0;

/* Un entero en decimal, sin %lld ni %llu: el printf de una placa puede no tenerlos (las ESP llevan picolibc). */
static const char* dec(long long v, char* b) {
    char t[24];
    int k = 0, j = 0;
    unsigned long long u = v < 0 ? 0ull - (unsigned long long) v : (unsigned long long) v;
    do { t[k++] = (char) ('0' + (int) (u % 10u)); u /= 10u; } while (u);
    if (v < 0) b[j++] = '-';
    while (k) b[j++] = t[--k];
    b[j] = '\0';
    return b;
}

static void muestra(const char* que, const uint8_t* p, uint32_t n, const char* detalle) {
    if (fallos <= 12) {
        char bytes[130];
        uint32_t k = 0;
        for (uint32_t i = 0; i < n && i < 40; i++) k += (uint32_t) snprintf(bytes + k, sizeof bytes - k, " %02X", p[i]);
        di("FALLO %s (%s) con %u bytes:%s%s", que, detalle, (unsigned) n, k ? bytes : "", n > 40 ? " ..." : "");
    }
}
#define IGUAL(que, a, b, p, n) do { comprobaciones++; \
        if ((long long) (a) != (long long) (b)) { char d_[96]; fallos++; \
            char da_[24], db_[24]; \
            snprintf(d_, sizeof d_, "%s en vez de %s", dec((long long) (a), da_), dec((long long) (b), db_)); \
            muestra(que, p, n, d_); } \
    } while (0)

/* ═══════════════════════════════ UNA CADENA, TODO ═══════════════════════ */

static uint8_t sal_a[SAL_N], sal_b[SAL_N];   /* las salidas (corregir, mayúsculas): la de bpstr y la del oráculo */

/* Todas las funciones de UTF-8 y de transformar con la cadena p[0, n). Con `todo`, avanzar y retroceder con todos los
 * puntos de partida y todos los pasos; sin él, unos cuantos. */
static void una_cadena(const uint8_t* p, uint32_t n, int todo) {
    uint32_t a1, b1, a2, b2, h1, h2, o1, o2;
    IGUAL("es_ascii", bpstr_es_ascii(p, n), bpvm_bytes_ascii(p, n), p, n);
    IGUAL("cuenta", bpstr_cuenta(p, n), utf8_cp_count(p, n), p, n);
    IGUAL("primer_malo", bpstr_primer_malo(p, n), utf8_primer_malo(p, n), p, n);
    o1 = bpstr_largo_corregido(p, n);
    IGUAL("largo_corregido", o1, utf8_largo_corregido(p, n), p, n);
    if (o1 < sizeof sal_a) {
        o1 = bpstr_corrige(p, n, sal_a);
        o2 = utf8_corrige(p, n, sal_b);
        IGUAL("corrige (largo)", o1, o2, p, n);
        IGUAL("corrige (bytes)", memcmp(sal_a, sal_b, o1 < o2 ? o1 : o2), 0, p, n);
    }
    if (n < sizeof sal_a) {
        for (int up = 0; up <= 1; up++) {
            memset(sal_a, 0xAA, n + 4); memset(sal_b, 0xAA, n + 4);
            o1 = up ? bpstr_mayusculas(p, n, sal_a) : bpstr_minusculas(p, n, sal_a);
            o2 = ref_cambia(p, n, sal_b, up);
            IGUAL(up ? "mayusculas (largo)" : "minusculas (largo)", o1, o2, p, n);
            IGUAL(up ? "mayusculas (bytes)" : "minusculas (bytes)", memcmp(sal_a, sal_b, n + 4), 0, p, n);
        }
    }
    bpstr_recorta(p, n, &a1, &b1);
    ref_recorta(p, n, &a2, &b2);
    IGUAL("recorta (a)", a1, a2, p, n);
    IGUAL("recorta (b)", b1, b2, p, n);
    if (todo) {
        for (uint32_t bo = 0; bo <= n + 1; bo++)
            for (uint32_t k = 0; k <= n + 2; k++) {
                IGUAL("avanza", bpstr_avanza(p, n, bo, k, &h1), ref_avanza(p, n, bo, k, &h2), p, n);
                IGUAL("avanza (pasos)", h1, h2, p, n);
                if (bo <= n) IGUAL("retrocede", bpstr_retrocede(p, bo, k), ref_retrocede(p, bo, k), p, n);
            }
    } else {
        static uint32_t semilla = 12345u;
        for (int t = 0; t < 6; t++) {
            uint32_t bo, k;
            semilla = semilla * 1103515245u + 12345u;
            bo = n ? (semilla >> 8) % (n + 1) : 0;
            k = (semilla >> 3) % (n + 3);
            IGUAL("avanza", bpstr_avanza(p, n, bo, k, &h1), ref_avanza(p, n, bo, k, &h2), p, n);
            IGUAL("avanza (pasos)", h1, h2, p, n);
            IGUAL("retrocede", bpstr_retrocede(p, bo, k), ref_retrocede(p, bo, k), p, n);
        }
    }
}

/* Buscar q en p: hacia delante desde varios sitios, hacia atrás hasta varios, y las apariciones. */
static void una_busqueda(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn) {
    uint32_t desdes[5] = {0, 1, 3, n / 2, n + 1};
    for (int d = 0; d < 5; d++) {
        IGUAL("busca", bpstr_busca(p, n, q, qn, desdes[d]), bp_find_bytes(p, n, q, qn, desdes[d]), p, n);
        IGUAL("busca_atras", bpstr_busca_atras(p, n, q, qn, desdes[d]), ref_busca_atras(p, n, q, qn, desdes[d]), p, n);
    }
    IGUAL("cuenta_apariciones", bpstr_cuenta_apariciones(p, n, q, qn), ref_apariciones(p, n, q, qn), p, n);
    if (qn == 1) {
        long r = bp_find_bytes(p, n, q, 1, 0);
        IGUAL("busca_byte", bpstr_busca_byte(p, n, q[0]), r, p, n);
    }
}

/* ═══════════════════════════════ LAS PRUEBAS ════════════════════════════ */

static uint8_t buf[BUF_N], buf2[BUF_N];

static void grupo(const char* nombre, unsigned long long antes_c, unsigned long long antes_f) {
    char c_[24];
    di("%s %-46s %12s comprobaciones%s", fallos == antes_f ? "ok  " : "MAL ", nombre,
       dec((long long) (comprobaciones - antes_c), c_), fallos == antes_f ? "" : " (con fallos)");
}

static void exhaustivo_corto(void) {
    unsigned long long c0 = comprobaciones, f0 = fallos;
    uint32_t indice = 0;
    for (uint32_t n = 0; n <= 3; n++) {
        uint32_t total = 1u << (8 * n), paso = (n == 3) ? PASO_3 : 1u;
        for (uint32_t v = 0; v < total; v += paso, indice++) {
            uint8_t* p = buf + 16 + (indice & 3u);   /* las cuatro alineaciones, por turno */
            for (uint32_t i = 0; i < n; i++) p[i] = (uint8_t) (v >> (8 * i));
            una_cadena(p, n, 1);
        }
        PROGRESO("exhaustivo: hechas las de %u bytes", (unsigned) n);
    }
    grupo(PASO_3 == 1u ? "exhaustivo: todas las cadenas de 0 a 3 bytes"
                       : "exhaustivo: hasta 2 bytes; de 3, una de cada 4099", c0, f0);
}

static uint32_t llena_de(uint8_t* p, uint32_t v, uint32_t n, const uint8_t* alfa, uint32_t na) {
    for (uint32_t i = 0; i < n; i++) { p[i] = alfa[v % na]; v /= na; }
    return n;
}

static void exhaustivo_buscar(const uint8_t* alfa, uint32_t na, uint32_t max_p, uint32_t max_q, const char* nombre) {
    unsigned long long c0 = comprobaciones, f0 = fallos;
    uint32_t vuelta = 0;
    for (uint32_t n = 0; n <= max_p; n++) {
        uint32_t tp = 1; for (uint32_t i = 0; i < n; i++) tp *= na;
        for (uint32_t vp = 0; vp < tp; vp++) {
            uint8_t* p = buf + 16 + (vuelta++ & 3u);
            llena_de(p, vp, n, alfa, na);
            for (uint32_t qn = 0; qn <= max_q; qn++) {
                uint32_t tq = 1; for (uint32_t i = 0; i < qn; i++) tq *= na;
                for (uint32_t vq = 0; vq < tq; vq++) {
                    uint8_t* q = buf2 + 16 + ((vq + vuelta) & 3u);
                    llena_de(q, vq, qn, alfa, na);
                    una_busqueda(p, n, q, qn);
                }
            }
        }
        PROGRESO("%s: hechos los pajares de %u bytes", nombre, (unsigned) n);
    }
    grupo(nombre, c0, f0);
}

static void exhaustivo_comparar_copiar(void) {
    static const uint8_t alfa[4] = {'a', 'b', 0x80, 0xC3};
    unsigned long long c0 = comprobaciones, f0 = fallos;
    for (uint32_t na = 0; na <= 4; na++)
        for (uint32_t va = 0; va < (1u << (2 * na)); va++)
            for (uint32_t nb = 0; nb <= 4; nb++)
                for (uint32_t vb = 0; vb < (1u << (2 * nb)); vb++)
                    for (uint32_t al = 0; al < 16; al++) {
                        uint8_t* a = buf + 16 + (al & 3u);
                        uint8_t* b = buf2 + 16 + (al >> 2);
                        llena_de(a, va, na, alfa, 4);
                        llena_de(b, vb, nb, alfa, 4);
                        IGUAL("compara", bpstr_compara(a, na, b, nb), ref_compara(a, na, b, nb), a, na);
                        IGUAL("igual", bpstr_igual(a, na, b, nb), ref_compara(a, na, b, nb) == 0, a, na);
                    }
    /* copiar y rellenar: de 0 a 300 bytes (en las ARM, la copia de Arm va por bloques de 64 y de 16), las 16
     * alineaciones, y que no toquen nada fuera */
    for (uint32_t n = 0; n <= 300; n++)
        for (uint32_t al = 0; al < 16; al++) {
            uint8_t* d = buf + 32 + (al & 3u);
            uint8_t* s = buf2 + 32 + (al >> 2);
            for (uint32_t i = 0; i < n + 40u; i++) { buf[i] = 0x5A; buf2[i] = (uint8_t) (i * 7u + 1u); }
            bpstr_copia(d, s, n);
            IGUAL("copia (bytes)", memcmp(d, s, n), 0, s, n);
            IGUAL("copia (antes)", d[-1], 0x5A, s, n);
            IGUAL("copia (despues)", d[n], 0x5A, s, n);
            bpstr_rellena(d, 0xC3, n);
            for (uint32_t i = 0; i < n; i++) IGUAL("rellena (bytes)", d[i], 0xC3, s, n);
            IGUAL("rellena (antes)", d[-1], 0x5A, s, n);
            IGUAL("rellena (despues)", d[n], 0x5A, s, n);
        }
    grupo("comparar (cortas), copiar y rellenar (0-300): 16 alineaciones", c0, f0);
}

/* ── al azar ── */
static uint32_t azar_estado = 0x2545F491u;
static uint32_t azar(void) {   /* xorshift32: la misma serie en todas partes */
    uint32_t x = azar_estado;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return azar_estado = x;
}
/* Una cadena «de verdad»: casi todo ASCII (letras, comas, espacios), con caracteres de 2, 3 y 4 bytes y, a veces, un
 * byte suelto cualquiera (un trozo malo). */
static uint32_t cadena_azar(uint8_t* p, uint32_t max) {
    uint32_t n = 0, largo = azar() % 8u == 0 ? azar() % max : azar() % (max < 200u ? max : 200u);
    int malos = (azar() % 4u) == 0;
    while (n < largo) {
        uint32_t r = azar() % 100u;
        if (r < 70u || n + 4u > largo) {
            static const char letras[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ,.:;\"\n\t";
            p[n++] = (uint8_t) letras[azar() % (sizeof letras - 1u)];
        } else if (r < 82u) {
            uint32_t cp = 0x80u + azar() % (0x800u - 0x80u);
            n += bpstr_codifica(cp, p + n);
        } else if (r < 92u) {
            uint32_t cp = 0x800u + azar() % (0x10000u - 0x800u);
            if (cp >= 0xD800u && cp <= 0xDFFFu) cp = 0x20ACu;   /* sin sustitutos (salvo los malos de abajo) */
            n += bpstr_codifica(cp, p + n);
        } else if (r < 97u || !malos) {
            uint32_t cp = 0x10000u + azar() % (0x110000u - 0x10000u);
            n += bpstr_codifica(cp, p + n);
        } else {
            p[n++] = (uint8_t) azar();                         /* cualquier byte */
        }
    }
    return n;
}

static void al_azar(uint32_t veces) {
    unsigned long long c0 = comprobaciones, f0 = fallos;
    for (uint32_t t = 0; t < veces; t++) {
        if (t > 0 && t % 500u == 0) PROGRESO("al azar: %u de %u", (unsigned) t, (unsigned) veces);
        uint8_t* p = buf + 16 + (azar() & 3u);
        uint32_t n = cadena_azar(p, AZAR_MAX);
        una_cadena(p, n, 0);
        /* buscar: un trozo de la cadena (corto o largo: Two-Way) o algo al azar */
        for (int b = 0; b < 3; b++) {
            uint8_t* q = buf2 + 16 + (azar() & 3u);
            uint32_t qn;
            if (n > 0 && (azar() & 1u)) {
                uint32_t desde = azar() % n, largo = 1u + azar() % (azar() % 4u == 0 ? 80u : 20u);
                if (desde + largo > n) largo = n - desde;
                memcpy(q, p + desde, largo);
                qn = largo;
                if (azar() % 3u == 0 && qn > 0) q[azar() % qn] ^= (uint8_t) (1u << (azar() % 8u));   /* casi */
            } else {
                qn = cadena_azar(q, 40u);
            }
            una_busqueda(p, n, q, qn);
        }
        /* comparar con una copia, con algo cambiado o más corta */
        {
            uint8_t* q = buf2 + 16 + (azar() & 3u);
            uint32_t qn = n;
            memcpy(q, p, n);
            if (n > 0 && (azar() & 1u)) q[azar() % n] ^= (uint8_t) (1u + azar() % 255u);
            if (n > 0 && azar() % 4u == 0) qn = azar() % n;
            IGUAL("compara", bpstr_compara(p, n, q, qn), ref_compara(p, n, q, qn), p, n);
            IGUAL("igual", bpstr_igual(p, n, q, qn), ref_compara(p, n, q, qn) == 0, p, n);
        }
    }
    {
        char nombre[64];
        snprintf(nombre, sizeof nombre, "al azar: %u cadenas de hasta %u bytes", (unsigned) veces,
                 (unsigned) AZAR_MAX);
        grupo(nombre, c0, f0);
    }
}

/* Una aguja larga y periódica, el caso difícil de Two-Way: «abaabaab…» en un pajar casi igual. También de más de 256
 * bytes: el byte cambiado no está en la aguja, y su salto (la aguja entera) no cabe en la tabla de bytes. */
static void dos_caminos_dificil(void) {
    unsigned long long c0 = comprobaciones, f0 = fallos;
    for (uint32_t per = 1; per <= 9; per++)
        for (uint32_t qn = 17; qn <= 300; qn += (qn < 70u) ? 3u : (qn < 250u) ? 60u : 7u) {
            uint8_t* q = buf2 + 16;
            uint8_t* p = buf + 16;
            uint32_t n = 2000u;
            for (uint32_t i = 0; i < qn; i++) q[i] = (uint8_t) ('a' + ((i % per) == per - 1u));
            for (uint32_t i = 0; i < n; i++) p[i] = (uint8_t) ('a' + ((i % per) == per - 1u));
            p[n - 5u] ^= 1u;                                  /* que no case justo al final */
            una_busqueda(p, n, q, qn);
            p[1000] ^= 3u;
            una_busqueda(p, n, q, qn);
        }
    /* Y con un PERIODO de más de 255 bytes (#730: el Two-Way de newlib, con la tabla de bytes): un trozo B al azar, la
     * aguja B B… de hasta casi tres veces B, y el pajar, B una y otra vez con algunos bytes cambiados. Un byte que sólo
     * está al principio de B tiene un salto que no cabe en la tabla, y es ahí donde el atajo de la periódica pide el
     * salto exacto. */
    for (uint32_t t = 0; t < VUELTAS_PERIODO; t++) {
        const uint32_t per = 256u + azar() % 60u, qn = per + 1u + azar() % (per + 100u), n = 2000u;
        uint8_t* q = buf2 + 16;
        uint8_t* p = buf + 16;
        for (uint32_t i = 0; i < per; i++) q[i] = (uint8_t) (azar() % 200u);
        for (uint32_t i = per; i < qn; i++) q[i] = q[i - per];
        for (uint32_t i = 0; i < n; i++) p[i] = q[i % per];
        for (uint32_t k = 0; k < 1u + azar() % 4u; k++) p[azar() % n] = (uint8_t) azar();
        una_busqueda(p, n, q, qn);
    }
    grupo("buscar: agujas largas y periodicas", c0, f0);
}

/* La aguja en p desde pos (o lo más cerca del final que quepa): tal cual, o con un byte cambiado por otro del alfabeto
 * —el primero, el último, el penúltimo o uno cualquiera—, que es lo que más trabajo da: casa casi todo. */
static void mete(uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn, uint32_t pos, uint32_t cambia,
                 const uint8_t* alfa, uint32_t na) {
    uint32_t donde;
    uint8_t viejo;
    if (qn > n) return;
    if (pos > n - qn) pos = n - qn;
    memcpy(p + pos, q, qn);
    if (cambia == 0u) return;
    donde = (cambia == 1u) ? 0u : (cambia == 2u) ? qn - 1u : (cambia == 3u) ? qn - 2u : azar() % qn;
    viejo = p[pos + donde];
    do p[pos + donde] = alfa[azar() % na]; while (p[pos + donde] == viejo);
}

/* Las agujas de 3 a 300 bytes: por parejas (hasta 256) y por Two-Way (más), y la frontera, byte a byte. Tres alfabetos:
 * el binario (a, b), que casa a menudo el final y hace trabajar al filtro; uno cuyas parejas chocan en la tabla (00 01
 * 08 20: la mezcla de 01 08 es la de 00 00, y dos parejas cuyo primer byte se lleva 32, 00 y 20, con el mismo segundo,
 * se mezclan igual); y letras con el espacio, minúsculas y mayúsculas (la a y la A también se llevan 32). En cada
 * pajar, la aguja casi (dos veces) y, según la vuelta, tal cual en cualquier sitio, justo al final, o el pajar hecho de
 * la aguja una y otra vez. */
static void agujas_largas(void) {
    static const uint8_t binario[2] = {'a', 'b'};
    static const uint8_t choques[4] = {0x00, 0x01, 0x08, 0x20};
    static const uint8_t letras[8] = {'e', 'a', 'o', 's', 'E', 'A', ' ', 'l'};
    static const uint8_t* const alfas[3] = {binario, choques, letras};
    static const uint32_t nas[3] = {2u, 4u, 8u};
    unsigned long long c0 = comprobaciones, f0 = fallos;
    const uint32_t n = PAJAR_AGUJAS;
    for (uint32_t k = 0; k < 3; k++) {
        const uint8_t* a = alfas[k];
        const uint32_t na = nas[k];
        for (uint32_t qn = 3; qn <= 300u; qn += (qn < 40u || (qn >= 248u && qn < 260u)) ? 1u : PASO_AGUJAS) {
            uint8_t* p = buf + 16 + (qn & 3u);
            uint8_t* q = buf2 + 16 + ((qn >> 2) & 3u);
            for (uint32_t i = 0; i < qn; i++) q[i] = a[azar() % na];
            for (uint32_t v = 0; v < 4u; v++) {
                if (v < 3u) for (uint32_t i = 0; i < n; i++) p[i] = a[azar() % na];
                else        for (uint32_t i = 0; i < n; i++) p[i] = q[i % qn];
                mete(p, n, q, qn, azar() % n, 1u + azar() % 4u, a, na);
                mete(p, n, q, qn, azar() % n, 1u + azar() % 4u, a, na);
                if (v == 1u) mete(p, n, q, qn, azar() % n, 0u, a, na);
                if (v == 2u) mete(p, n, q, qn, n - qn, 0u, a, na);
                una_busqueda(p, n, q, qn);
            }
        }
        PROGRESO("agujas de 3 a 300: hecho el alfabeto %u de 3", (unsigned) k + 1u);
    }
    grupo("buscar: agujas de 3 a 300 bytes, tres alfabetos", c0, f0);
}

/* ── los bordes, contra una página protegida (sólo en el PC) ── */
#if defined(BPSTR_PLACA)
static uint8_t* pagina_protegida(uint32_t* tam) { *tam = 0; return NULL; }   /* en una placa no hay páginas */
#elif defined(_WIN32)
#include <windows.h>
static uint8_t* pagina_protegida(uint32_t* tam) {
    SYSTEM_INFO si;
    DWORD viejo;
    uint8_t* b;
    GetSystemInfo(&si);
    *tam = (uint32_t) si.dwPageSize;
    b = (uint8_t*) VirtualAlloc(NULL, 3u * *tam, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!b) return NULL;
    VirtualProtect(b, *tam, PAGE_NOACCESS, &viejo);
    VirtualProtect(b + 2u * *tam, *tam, PAGE_NOACCESS, &viejo);
    return b + *tam;   /* la del medio: antes y después, nada */
}
#elif defined(__unix__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
static uint8_t* pagina_protegida(uint32_t* tam) {
    uint8_t* b;
    *tam = (uint32_t) sysconf(_SC_PAGESIZE);
    b = (uint8_t*) mmap(NULL, 3u * *tam, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (b == MAP_FAILED) return NULL;
    mprotect(b, *tam, PROT_NONE);
    mprotect(b + 2u * *tam, *tam, PROT_NONE);
    return b + *tam;
}
#else
static uint8_t* pagina_protegida(uint32_t* tam) { *tam = 0; return NULL; }
#endif

static void bordes(void) {
    unsigned long long c0 = comprobaciones, f0 = fallos;
    uint32_t pg = 0, pg2 = 0;
    uint8_t* pag = pagina_protegida(&pg);
    uint8_t* pag2 = pagina_protegida(&pg2);   /* para las salidas */
    if (!pag || !pag2) { di("--   los bordes: sin paginas protegidas aqui, se saltan"); return; }
    for (int forma = 0; forma < 3; forma++)
        for (uint32_t n = 0; n <= 64; n++)
            for (uint32_t t = 0; t < 40; t++) {
                uint8_t* fin = pag + pg;          /* la cadena acaba justo donde empieza la página protegida */
                uint8_t* p = fin - n;
                uint8_t* ini = pag;               /* y otra que empieza justo después de la de antes */
                uint32_t q0 = 0, qn;
                uint8_t tmp[80];
                if (forma == 0) for (uint32_t i = 0; i < n; i++) tmp[i] = (uint8_t) ("hola, que tal "[(i + t) % 14]);
                else if (forma == 1) { uint32_t m = 0; while (m < n) { uint32_t cp = 0xE1u + (azar() % 40u); m += bpstr_codifica(m + 2u <= n ? cp : 'x', tmp + m); } }
                else for (uint32_t i = 0; i < n; i++) tmp[i] = (uint8_t) azar();
                memcpy(p, tmp, n);
                memcpy(ini, tmp, n);
                una_cadena(p, n, n <= 12);
                una_cadena(ini, n, 0);
                /* buscar el final (lo último que se lee) y algo que no está */
                if (n > 0) {
                    q0 = azar() % n;
                    qn = n - q0;
                    una_busqueda(p, n, p + q0, qn);
                    una_busqueda(ini, n, p + q0, qn);
                }
                una_busqueda(p, n, (const uint8_t*) "zzzzzzzzzzzzzzzzzzzzzz", n < 22 ? n : 22);
                IGUAL("compara (bordes)", bpstr_compara(p, n, ini, n), 0, p, n);
                /* las salidas, también pegadas al final */
                {
                    uint32_t largo = bpstr_largo_corregido(p, n);
                    uint8_t* sal = pag2 + pg2 - largo;
                    IGUAL("corrige (bordes)", bpstr_corrige(p, n, sal), utf8_corrige(p, n, sal_b), p, n);
                    sal = pag2 + pg2 - n;
                    bpstr_mayusculas(p, n, sal);
                    bpstr_copia(sal, p, n);
                    bpstr_rellena(sal, 0x20, n);
                }
            }
    grupo("los bordes: 0 a 64 bytes contra paginas protegidas", c0, f0);
}

/* ── el catálogo de Kuhn ── */
static void kuhn(const char* ruta) {
    unsigned long long c0 = comprobaciones, f0 = fallos;
    uint32_t n, a, lineas = 0;
#ifdef BPSTR_PLACA
    long leidos = bpstr_placa_lee(ruta, buf + 16, sizeof buf - 32);
    if (leidos < 0) { di("MAL  el catalogo de Kuhn: no se lee %s", ruta); fallos++; return; }
    n = (uint32_t) leidos;
#else
    FILE* f = fopen(ruta, "rb");
    if (!f) { di("MAL  el catalogo de Kuhn: no se abre %s", ruta); fallos++; return; }
    n = (uint32_t) fread(buf + 16, 1, sizeof buf - 32, f);
    fclose(f);
#endif
    una_cadena(buf + 16, n, 0);
    for (uint32_t al = 1; al < 4; al++) {   /* entero, en las otras tres alineaciones */
        memmove(buf + 16 + al, buf + 16 + al - 1, n);
        una_cadena(buf + 16 + al, n, 0);
    }
    memmove(buf + 16, buf + 19, n);
    for (a = 0; a < n; ) {                  /* línea a línea */
        uint32_t b = a;
        while (b < n && buf[16 + b] != '\n') b++;
        una_cadena(buf + 16 + a, b - a, b - a <= 8);
        una_busqueda(buf + 16, n, buf + 16 + a, b - a);
        lineas++;
        if (lineas % 50u == 0) PROGRESO("Kuhn: %u lineas", (unsigned) lineas);
        a = b + 1;
    }
    {
        char nombre[80];
        snprintf(nombre, sizeof nombre, "el catalogo de Kuhn: %u bytes, %u lineas", (unsigned) n, (unsigned) lineas);
        grupo(nombre, c0, f0);
    }
}

/* ── el validador con todas las cadenas de 4 bytes ── */
static void exhaustivo_largo(void) {
    unsigned long long c0 = comprobaciones, f0 = fallos;
    uint8_t* p = buf + 16;
    uint32_t v = 0;
    do {
        uint32_t o1, o2;
        p[0] = (uint8_t) v; p[1] = (uint8_t) (v >> 8); p[2] = (uint8_t) (v >> 16); p[3] = (uint8_t) (v >> 24);
        IGUAL("primer_malo (4)", bpstr_primer_malo(p, 4), utf8_primer_malo(p, 4), p, 4);
        IGUAL("largo_corregido (4)", bpstr_largo_corregido(p, 4), utf8_largo_corregido(p, 4), p, 4);
        o1 = bpstr_corrige(p, 4, sal_a);
        o2 = utf8_corrige(p, 4, sal_b);
        IGUAL("corrige (4)", o1, o2, p, 4);
        if (memcmp(sal_a, sal_b, o1) != 0) IGUAL("corrige (4, bytes)", 1, 0, p, 4);
        IGUAL("cuenta (4)", bpstr_cuenta(p, 4), utf8_cp_count(p, 4), p, 4);
        if ((v & 0x0FFFFFFFu) == 0x0FFFFFFFu) di("     ... %u/16", (unsigned) (v >> 28) + 1u);
    } while (++v != 0u);
    grupo("exhaustivo: el validador con TODAS las de 4 bytes", c0, f0);
}

/* Todas, en orden; devuelve 0 si salen en verde. Con `larga`, también el validador con las de 4 bytes. */
int bpstr_prueba_main(int larga, const char* ruta_kuhn) {
    static const uint8_t alfa4[4] = {'a', 'b', 0x80, 0xC3};
    static const uint8_t alfa2[2] = {'a', 'b'};
    char nombre[80];
    comprobaciones = fallos = 0;
#ifdef BPSTR_PLACA
    t_inicio = bpstr_banco_ns();
#endif
    di("bpstr: las pruebas (la corta hasta %lu bytes, Two-Way hasta %lu, por parejas hasta %lu y Two-Way por encima)",
       (unsigned long) (BPSTR_AGUJA_CORTA), (unsigned long) (BPSTR_AGUJA_DOS), (unsigned long) (BPSTR_AGUJA_PAREJAS));
    exhaustivo_corto();
    snprintf(nombre, sizeof nombre, "buscar: pajares de 0-%u y agujas de 0-%u (a b 80 C3)", BUSCA4_P, BUSCA4_Q);
    exhaustivo_buscar(alfa4, 4, BUSCA4_P, BUSCA4_Q, nombre);
    snprintf(nombre, sizeof nombre, "buscar: pajares de 0-%u y agujas de 0-%u (a b)", BUSCA2_P, BUSCA2_Q);
    exhaustivo_buscar(alfa2, 2, BUSCA2_P, BUSCA2_Q, nombre);
    dos_caminos_dificil();
    agujas_largas();
    exhaustivo_comparar_copiar();
    bordes();
    al_azar(AZAR_VECES);
    kuhn(ruta_kuhn);
    if (larga) exhaustivo_largo();
    {
        char c_[24], f_[24];
        di("%s: %s comprobaciones, %s fallos", fallos ? "ROJO" : "VERDE", dec((long long) comprobaciones, c_),
           dec((long long) fallos, f_));
    }
    return fallos ? 1 : 0;
}

#ifndef BPSTR_PLACA
int main(int argc, char** argv) {
    int larga = 0;
    const char* ruta_kuhn = "../third_party/utf8-kuhn/UTF-8-test.txt";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "larga") == 0) larga = 1;
        else ruta_kuhn = argv[i];
    }
    return bpstr_prueba_main(larga, ruta_kuhn);
}
#endif
