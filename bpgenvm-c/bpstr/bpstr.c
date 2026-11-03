/*
 * bpstr.c — la biblioteca de cadenas de BasicPlus (V8 #730). Lo que promete cada función, en bpstr.h; por qué y cómo,
 * en docs/V8_IDEAS.md §E5, «#730: el diseño concreto».
 *
 * EL TRUCO DE 4 EN 4. Se lee una palabra de 4 bytes ALINEADA y se opera con enteros sobre los cuatro a la vez. Dos
 * máscaras dicen cuáles de sus bytes son de continuación (10xxxxxx), un XOR y una resta dicen si alguno es un byte
 * dado, y un OR, si alguno pasa de 0x7F. La cabeza (hasta alinear) y la cola (lo que no llena una palabra) van byte a
 * byte, así que nunca se lee más allá del final. Lo de dentro de cada función es el bucle de antes, el de un byte por
 * paso, que sigue haciendo lo raro (un carácter largo, un trozo malo): el resultado es el mismo, byte a byte.
 *
 * Que GCC no cambie los bucles de copiar y rellenar por llamadas a memcpy y memset: se compila con
 * -fno-tree-loop-distribute-patterns (ver el Makefile).
 */
#include "bpstr.h"

#include <string.h>

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "bpstr: las cuentas de 4 en 4 suponen little-endian (lo son todas nuestras familias y el PC)"
#endif

/* Una palabra de 4 bytes leída de memoria de bytes. `may_alias`: leer bytes como un entero sin romper las reglas de C
 * sobre los tipos (es lo que hace musl). Siempre de una dirección ALINEADA: la S3 no tolera otra cosa, y en las RISC-V
 * de Espressif va mal. */
typedef uint32_t __attribute__((__may_alias__)) palabra_t;

#define UNOS    0x01010101u
#define ALTOS   0x80808080u
#define CONT(b) (((b) & 0xC0u) == 0x80u)

static inline int alineado(const void* p) { return ((uintptr_t) p & 3u) == 0u; }
static inline uint32_t lee(const uint8_t* p) { return *(const palabra_t*) (const void*) p; }
static inline void escribe(uint8_t* p, uint32_t w) { *(palabra_t*) (void*) p = w; }

/* El bit 7 de los bytes de w que valen cero (el truco clásico). Tras el primer cero puede haber bits falsos, pero si
 * hay alguno, el primero es de verdad: sirve para saber SI hay un cero. */
static inline uint32_t ceros(uint32_t w) { return (w - UNOS) & ~w & ALTOS; }

/* El bit 7 de los bytes de continuación de w: el 7 encendido y el 6 apagado. (w << 1 lleva el 6 de cada byte al 7 de
 * ese mismo byte; lo que pasa al byte siguiente cae en su bit 0, que la máscara tira.) */
static inline uint32_t continuaciones(uint32_t w) { return w & ~(w << 1) & ALTOS; }

/* Cuántos de los bits 7 de los cuatro bytes están encendidos (0 a 4): bajados al bit 0 de cada byte, la multiplicación
 * los suma en el byte de arriba. */
static inline uint32_t cuenta_altos(uint32_t m) { return ((m >> 7) * UNOS) >> 24; }

/* ═══════════════════════════════ UTF-8 ═══════════════════════════════ */

int bpstr_es_ascii(const uint8_t* p, uint32_t n) {
    uint32_t acc = 0, i = 0;
    while (i < n && !alineado(p + i)) acc |= p[i++];
    while (n - i >= 16u) {
        acc |= lee(p + i) | lee(p + i + 4u) | lee(p + i + 8u) | lee(p + i + 12u);
        i += 16u;
        if (acc & ALTOS) return 0;   /* ya no lo es: no hace falta mirar más */
    }
    while (n - i >= 4u) { acc |= lee(p + i); i += 4u; }
    while (i < n) acc |= p[i++];
    return (acc & ALTOS) == 0u;
}

uint32_t bpstr_cuenta(const uint8_t* p, uint32_t n) {
    uint32_t i = 0, cont = 0;
    while (i < n && !alineado(p + i)) { cont += CONT(p[i]) ? 1u : 0u; i++; }
    while (n - i >= 4u) {
        /* hasta 255 palabras seguidas, con un contador por byte en acc (cada uno llega como mucho a 255) */
        uint32_t palabras = (n - i) / 4u, acc = 0, k;
        if (palabras > 255u) palabras = 255u;
        for (k = 0; k < palabras; k++, i += 4u) acc += continuaciones(lee(p + i)) >> 7;
        cont += (acc & 0xFFu) + ((acc >> 8) & 0xFFu) + ((acc >> 16) & 0xFFu) + (acc >> 24);
    }
    while (i < n) { cont += CONT(p[i]) ? 1u : 0u; i++; }
    return n - cont;
}

/* Lo de antes: while (ci < k && bo < n) { bo++; while (bo < n && CONT(p[bo])) bo++; ci++; }. O sea, el paso m cae en
 * el m-ésimo principio de carácter (un byte que no es de continuación) por detrás de bo, o en n si no lo hay, y ahí se
 * para. Basta contar principios desde bo + 1; de 4 en 4, los de una palabra son 4 menos sus continuaciones. */
uint32_t bpstr_avanza(const uint8_t* p, uint32_t n, uint32_t bo, uint32_t k, uint32_t* hechos) {
    uint32_t falta = k, j;
    if (k == 0u || bo >= n) { if (hechos) *hechos = 0u; return bo; }
    j = bo + 1u;
    while (j < n && !alineado(p + j)) {                 /* la cabeza */
        if (!CONT(p[j]) && --falta == 0u) goto llega;
        j++;
    }
    while (n - j >= 4u) {                               /* de 4 en 4, mientras no esté en esta palabra */
        uint32_t principios = 4u - cuenta_altos(continuaciones(lee(p + j)));
        if (principios >= falta) break;
        falta -= principios;
        j += 4u;
    }
    for (; j < n; j++)                                  /* la palabra donde está, o la cola */
        if (!CONT(p[j]) && --falta == 0u) goto llega;
    if (hechos) *hechos = k - falta + 1u;               /* se acabó: el último paso cae en n */
    return n;
llega:
    if (hechos) *hechos = k;
    return j;
}

/* Lo de antes: while (k > 0 && bo > 0) { bo--; while (bo > 0 && CONT(p[bo])) bo--; k--; }. El paso m cae en el
 * m-ésimo principio de carácter por debajo de bo, contando el byte 0 como principio siempre (de ahí no se baja). */
uint32_t bpstr_retrocede(const uint8_t* p, uint32_t bo, uint32_t k) {
    uint32_t falta = k, j = bo;                         /* se miran j-1, j-2…: j es el tope, sin él */
    if (k == 0u || bo == 0u) return bo;
    while (j > 0u && !alineado(p + j)) {                /* la cola, hasta que [j-4, j) sea una palabra alineada */
        j--;
        if ((j == 0u || !CONT(p[j])) && --falta == 0u) return j;
    }
    while (j > 4u) {                                    /* de 4 en 4 hacia abajo, sin llegar al byte 0 */
        uint32_t principios = 4u - cuenta_altos(continuaciones(lee(p + j - 4u)));
        if (principios >= falta) break;
        falta -= principios;
        j -= 4u;
    }
    while (j > 0u) {
        j--;
        if ((j == 0u || !CONT(p[j])) && --falta == 0u) return j;
    }
    return 0u;
}

/* Un paso desde p[i] (i < n, p[i] NO ASCII), la regla de #711: los bytes del carácter que empieza ahí (2 a 4) si es
 * válido, o -k si los k bytes desde i son un trozo malo (k >= 1). Es utf8_paso de bpvm_internal.h, tal cual. */
static inline int paso(const uint8_t* p, uint32_t i, uint32_t n) {
    uint8_t c = p[i];
    int necesita, k;
    uint8_t lo = 0x80u, hi = 0xBFu;
    if (c >= 0xC2u && c <= 0xDFu) {
        necesita = 1;
    } else if (c >= 0xE0u && c <= 0xEFu) {
        necesita = 2;
        if (c == 0xE0u) lo = 0xA0u;          /* sin sobrelargos de tres bytes */
        else if (c == 0xEDu) hi = 0x9Fu;     /* sin sustitutos (D800..DFFF) */
    } else if (c >= 0xF0u && c <= 0xF4u) {
        necesita = 3;
        if (c == 0xF0u) lo = 0x90u;          /* sin sobrelargos de cuatro bytes */
        else if (c == 0xF4u) hi = 0x8Fu;     /* nada por encima de 10FFFF */
    } else {
        return -1;                            /* 80..C1 y F5..FF no empiezan ningún carácter */
    }
    for (k = 1; k <= necesita; k++) {
        uint8_t d;
        if (i + (uint32_t) k >= n) return -k; /* cortado por el final: el trozo son los k que hay */
        d = p[i + (uint32_t) k];
        if (d < lo || d > hi) return -k;      /* el byte k no sigue: el trozo son los k de antes, y él empieza otro */
        lo = 0x80u;
        hi = 0xBFu;
    }
    return necesita + 1;
}

/* Los tres de #711 saltan de 4 en 4 lo que es ASCII, que no puede tener nada malo; lo demás, como antes. */
int32_t bpstr_primer_malo(const uint8_t* p, uint32_t n) {
    uint32_t i = 0;
    while (i < n) {
        int s;
        if (alineado(p + i)) {
            while (n - i >= 4u && (lee(p + i) & ALTOS) == 0u) i += 4u;
            if (i >= n) break;
        }
        if (p[i] < 0x80u) { i++; continue; }
        s = paso(p, i, n);
        if (s < 0) return (int32_t) i;
        i += (uint32_t) s;
    }
    return -1;
}

uint32_t bpstr_largo_corregido(const uint8_t* p, uint32_t n) {
    uint32_t i = 0, largo = 0;
    while (i < n) {
        int s;
        if (alineado(p + i)) {
            uint32_t i0 = i;
            while (n - i >= 4u && (lee(p + i) & ALTOS) == 0u) i += 4u;
            largo += i - i0;
            if (i >= n) break;
        }
        if (p[i] < 0x80u) { largo++; i++; continue; }
        s = paso(p, i, n);
        if (s > 0) { largo += (uint32_t) s; i += (uint32_t) s; }
        else       { largo += 3u; i += (uint32_t) -s; }
    }
    return largo;
}

uint32_t bpstr_corrige(const uint8_t* p, uint32_t n, uint8_t* out) {
    uint32_t i = 0, o = 0;
    while (i < n) {
        int s, k;
        if (alineado(p + i)) {
            while (n - i >= 4u) {
                uint32_t w = lee(p + i);
                if (w & ALTOS) break;
                if (alineado(out + o)) escribe(out + o, w);
                else { out[o] = p[i]; out[o + 1u] = p[i + 1u]; out[o + 2u] = p[i + 2u]; out[o + 3u] = p[i + 3u]; }
                i += 4u;
                o += 4u;
            }
            if (i >= n) break;
        }
        if (p[i] < 0x80u) { out[o++] = p[i++]; continue; }
        s = paso(p, i, n);
        if (s > 0) {
            for (k = 0; k < s; k++) out[o++] = p[i + (uint32_t) k];
            i += (uint32_t) s;
        } else {
            out[o++] = 0xEFu; out[o++] = 0xBFu; out[o++] = 0xBDu;
            i += (uint32_t) -s;
        }
    }
    return o;
}

/* ═══════════════════════════════ BUSCAR ══════════════════════════════ */

int32_t bpstr_busca_byte(const uint8_t* p, uint32_t n, uint8_t c) {
    uint32_t i = 0;
    while (i < n && !alineado(p + i)) { if (p[i] == c) return (int32_t) i; i++; }
    if (n - i >= 4u) {
        uint32_t cc = (uint32_t) c * UNOS;             /* c en los cuatro bytes: el XOR deja a cero el que casa */
        while (n - i >= 8u) {
            if (ceros(lee(p + i) ^ cc) | ceros(lee(p + i + 4u) ^ cc)) break;
            i += 8u;
        }
        if (n - i >= 4u && !ceros(lee(p + i) ^ cc)) i += 4u;
    }
    for (; i < n; i++) if (p[i] == c) return (int32_t) i;   /* dentro de la palabra que casa, o la cola */
    return -1;
}

/* Los tres caminos de buscar, por lo larga que es la aguja. Hasta AGUJA_CORTA, la CORTA: se busca su primer byte de 4 en
 * 4 y se compara el resto. Hasta AGUJA_PAREJAS (y desde 3 bytes), por PAREJAS, el de Arm. Más larga, Two-Way. Los dos
 * primeros, lineales porque la aguja no pasa de su tope (en el peor caso, qn comparaciones por posición); Two-Way, lineal
 * siempre. Los topes, por medir. Las pruebas los cambian al compilar para probar cada camino con agujas pequeñas: todo
 * por Two-Way (1 y 0), todo por la corta (uno muy grande) y todo por parejas (2 y 256). */
#ifndef BPSTR_AGUJA_CORTA
#define BPSTR_AGUJA_CORTA 16u
#endif
#ifndef BPSTR_AGUJA_PAREJAS
#define BPSTR_AGUJA_PAREJAS 256u
#endif
#define AGUJA_CORTA   ((uint32_t) (BPSTR_AGUJA_CORTA))
#define AGUJA_PAREJAS ((uint32_t) (BPSTR_AGUJA_PAREJAS))
#if BPSTR_AGUJA_PAREJAS > 256
#error "bpstr: la tabla de las parejas es de bytes: la aguja, de 256 como mucho"
#endif

static int32_t busca_corta(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn, uint32_t desde) {
    const uint32_t ultimo = n - qn;                     /* la última posición donde cabe la aguja */
    uint32_t i = desde;
    while (i <= ultimo) {
        int32_t r = bpstr_busca_byte(p + i, ultimo - i + 1u, q[0]);
        uint32_t k;
        if (r < 0) return -1;
        i += (uint32_t) r;
        for (k = 1u; k < qn && p[i + k] == q[k]; k++) ;
        if (k == qn) return (int32_t) i;
        i++;
    }
    return -1;
}

/*
 * PAREJAS: el memmem de newlib, escrito por Arm, para las agujas de 3 a 256 bytes. Es Horspool, pero el salto no lo da
 * UN byte sino la PAREJA que queda bajo el final de la aguja: los dos bytes, mezclados en una tabla de 256 saltos de un
 * byte (la última aparición de cada mezcla en la aguja). Una pareja que no está en la aguja salta la aguja entera, y de
 * las parejas posibles una aguja tiene pocas. El bucle de saltar sólo sale cuando la pareja es la del final; entonces
 * se compara la aguja (si es larga, primero 4 bytes de un sitio que se mueve cada vez que no sirve para descartar: un
 * filtro que aprende dónde suele fallar). El último byte no se compara: la mezcla, con el penúltimo igual, lo fija.
 * Medido en la RP2350 (#730, el paso 2): con una aguja de 32 bytes, 2,9 veces más deprisa que nuestro Two-Way.
 *
 * Adaptado de newlib (newlib/libc/string/memmem.c), con su licencia:
 *   Copyright (c) 2018 Arm Ltd.  All rights reserved.
 *   SPDX-License-Identifier: BSD-3-Clause
 *   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 *   following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 *      disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *      following disclaimer in the documentation and/or other materials provided with the distribution.
 *   3. The name of the company may not be used to endorse or promote products derived from this software without
 *      specific prior written permission.
 *   THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ARM
 *   LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * Lo nuestro: posiciones de 32 bits en vez de punteros; comparar con bpstr_igual (la memcmp de la STM32 es de un byte
 * por vuelta); y el filtro, de 4 bytes en todas partes (allí, sizeof (long): en el PC de 64 bits serían 8).
 */
#define PAREJA(p) ((uint32_t) ((uint32_t) (p)[0] - ((uint32_t) (p)[-1] << 3)) & 255u)   /* p[-1] y p[0], mezclados */

static int32_t busca_parejas(const uint8_t* h, uint32_t hn, const uint8_t* q, uint32_t qn) {
    uint8_t salto[256];
    const uint32_t m1 = qn - 1u, ultimo = hn - qn;      /* qn de 3 a 256, y hn >= qn: lo mira bpstr_busca */
    uint32_t j = 0, t, i, salto1, filtro = 0;
    memset(salto, 0, sizeof salto);
    for (i = 1; i < m1; i++) salto[PAREJA(q + i)] = (uint8_t) i;
    salto1 = m1 - salto[PAREJA(q + m1)];                /* tras fallar con el final casado: la aparición de antes */
    salto[PAREJA(q + m1)] = (uint8_t) m1;
    while (j <= ultimo) {                               /* j: el principio de la ventana */
        do {                                            /* saltar las parejas que no están en la aguja */
            j += m1;                                    /* j: el último byte de la ventana */
            t = salto[PAREJA(h + j)];
        } while (j <= ultimo && t == 0u);
        j -= t;
        if (t < m1) continue;                           /* está, pero no al final: alinearla y otra vez */
        if (m1 <= 15u || bpstr_igual(h + j + filtro, 4u, q + filtro, 4u)) {
            if (bpstr_igual(h + j, m1, q, m1)) return (int32_t) j;
            filtro = (filtro >= 4u ? filtro : m1) - 4u;   /* el filtro no la descartó: probar en otro sitio */
        }
        j += salto1;
    }
    return -1;
}

/*
 * TWO-WAY (Crochemore y Perrin, 1991): buscar una aguja larga en tiempo LINEAL siempre, sin memoria aparte. La aguja
 * se parte en dos por su «factorización crítica» (el sufijo máximo, con las dos órdenes de los bytes); en cada ventana
 * se compara la mitad derecha de izquierda a derecha y, si casa, la izquierda de derecha a izquierda, y lo que se salta
 * al fallar no deja atrás ninguna aparición. Con una aguja periódica se recuerda lo ya comparado (mem). Antes de
 * comparar se mira el último byte de la ventana: si no está en la aguja se salta entera, y si está, hasta alinearlo
 * con su última aparición en ella (lo de Horspool, que no cambia lo de arriba: sólo salta antes).
 *
 * Adaptado de musl (src/string/memmem.c, twoway_memmem), con su licencia:
 *   Copyright © 2005-2020 Rich Felker, et al.
 *   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 *   documentation files (the "Software"), to deal in the Software without restriction, including without limitation
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *   The above copyright notice and this permission notice shall be included in all copies or substantial portions of
 *   the Software.
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *   THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *   CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *   IN THE SOFTWARE.
 * Lo nuestro: posiciones de 32 bits; y la tabla de saltos, de bytes (256 B de pila en vez de 1 KB): un salto de más de
 * 255 se queda en 255. Saltar menos nunca se deja una aparición atrás.
 */
static int32_t dos_caminos(const uint8_t* h, uint32_t hn, const uint8_t* q, uint32_t l) {
    uint32_t i, ip, jp, k, per, ms, per0, mem, mem0, pos;
    uint32_t esta[8] = {0, 0, 0, 0, 0, 0, 0, 0};       /* qué bytes están en la aguja */
    uint8_t salto[256];
    memset(salto, 0, sizeof salto);
    for (i = 0; i < l; i++) {                           /* la última aparición de cada byte manda */
        uint32_t d = l - i - 1u;
        esta[q[i] >> 5] |= 1u << (q[i] & 31u);
        salto[q[i]] = (uint8_t) (d > 255u ? 255u : d);
    }
    /* el sufijo máximo, con un orden… */
    ip = (uint32_t) -1; jp = 0; k = per = 1;
    while (jp + k < l) {
        if (q[ip + k] == q[jp + k]) {
            if (k == per) { jp += per; k = 1; } else k++;
        } else if (q[ip + k] > q[jp + k]) {
            jp += k; k = 1; per = jp - ip;
        } else {
            ip = jp++; k = per = 1;
        }
    }
    ms = ip;
    per0 = per;
    /* …y con el contrario; se queda el más largo */
    ip = (uint32_t) -1; jp = 0; k = per = 1;
    while (jp + k < l) {
        if (q[ip + k] == q[jp + k]) {
            if (k == per) { jp += per; k = 1; } else k++;
        } else if (q[ip + k] < q[jp + k]) {
            jp += k; k = 1; per = jp - ip;
        } else {
            ip = jp++; k = per = 1;
        }
    }
    if (ip + 1u > ms + 1u) ms = ip;
    else per = per0;
    /* ¿es periódica? */
    if (memcmp(q, q + per, ms + 1u) != 0) {
        mem0 = 0;
        per = ((ms > l - ms - 1u) ? ms : l - ms - 1u) + 1u;
    } else {
        mem0 = l - per;
    }
    mem = 0;
    pos = 0;                                            /* el principio de la ventana en h */
    for (;;) {
        uint8_t c;
        if (hn - pos < l) return -1;                    /* lo que queda no cabe */
        c = h[pos + l - 1u];
        if (esta[c >> 5] & (1u << (c & 31u))) {
            k = salto[c];
            if (k) {
                if (k < mem) k = mem;
                pos += k;
                mem = 0;
                continue;
            }
        } else {
            pos += l;
            mem = 0;
            continue;
        }
        for (k = (ms + 1u > mem) ? ms + 1u : mem; k < l && q[k] == h[pos + k]; k++) ;   /* la mitad derecha */
        if (k < l) {
            pos += k - ms;
            mem = 0;
            continue;
        }
        for (k = ms + 1u; k > mem && q[k - 1u] == h[pos + k - 1u]; k--) ;              /* la izquierda */
        if (k <= mem) return (int32_t) pos;
        pos += per;
        mem = mem0;
    }
}

int32_t bpstr_busca(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn, uint32_t desde) {
    int32_t r;
    if (qn == 0u) return (int32_t) (desde <= n ? desde : n);
    if (qn > n || desde > n - qn) return -1;
    if (qn == 1u) {
        r = bpstr_busca_byte(p + desde, n - desde, q[0]);
        return r < 0 ? -1 : (int32_t) desde + r;
    }
    if (qn <= AGUJA_CORTA) return busca_corta(p, n, q, qn, desde);
    if (qn >= 3u && qn <= AGUJA_PAREJAS) r = busca_parejas(p + desde, n - desde, q, qn);
    else r = dos_caminos(p + desde, n - desde, q, qn);
    return r < 0 ? -1 : (int32_t) desde + r;
}

#ifdef BPSTR_CAMINOS
/* Sólo para el banco (banco.c, BPSTR_CAMINOS): cada camino de buscar por separado, desde el principio, para elegir los
 * topes por la medida. -1 si el camino no vale para esa aguja. */
int32_t bpstr_camino_corta(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn) {
    return (qn == 0u || qn > n) ? -1 : busca_corta(p, n, q, qn, 0u);
}
int32_t bpstr_camino_parejas(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn) {
    return (qn < 3u || qn > 256u || qn > n) ? -1 : busca_parejas(p, n, q, qn);
}
int32_t bpstr_camino_dos(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn) {
    return (qn == 0u || qn > n) ? -1 : dos_caminos(p, n, q, qn);
}
#endif

int32_t bpstr_busca_atras(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn, uint32_t hasta) {
    uint32_t i;
    if (qn == 0u) return (int32_t) (hasta <= n ? hasta : n);
    if (qn > n) return -1;
    i = n - qn;
    if (hasta < i) i = hasta;
    for (;;) {
        if (p[i] == q[0] && memcmp(p + i + 1u, q + 1u, qn - 1u) == 0) return (int32_t) i;
        if (i == 0u) return -1;
        i--;
    }
}

uint32_t bpstr_cuenta_apariciones(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn) {
    uint32_t veces = 0, i = 0;
    int32_t r;
    if (qn == 0u) return 0u;
    while ((r = bpstr_busca(p, n, q, qn, i)) >= 0) {
        veces++;
        i = (uint32_t) r + qn;
    }
    return veces;
}

/* ═══════════════════════════════ COMPARAR ════════════════════════════ */

int bpstr_compara(const uint8_t* a, uint32_t na, const uint8_t* b, uint32_t nb) {
    uint32_t n = na < nb ? na : nb, i = 0;
    if ((((uintptr_t) a ^ (uintptr_t) b) & 3u) == 0u) {   /* con la misma alineación, de 4 en 4 */
        while (i < n && !alineado(a + i)) {
            if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
            i++;
        }
        while (n - i >= 8u && lee(a + i) == lee(b + i) && lee(a + i + 4u) == lee(b + i + 4u)) i += 8u;
        while (n - i >= 4u && lee(a + i) == lee(b + i)) i += 4u;
        for (; i < n; i++) if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;   /* en la palabra distinta, cuál */
    } else if (n > 0u) {                                   /* desalineadas entre sí: lo de la libc */
        int r = memcmp(a, b, n);
        if (r != 0) return r < 0 ? -1 : 1;
    }
    return na < nb ? -1 : (na > nb ? 1 : 0);
}

int bpstr_igual(const uint8_t* a, uint32_t na, const uint8_t* b, uint32_t nb) {
    return na == nb && bpstr_compara(a, na, b, nb) == 0;
}

/* ═══════════════════════════════ TRANSFORMAR ═════════════════════════ */

/* Latin-1 (#348, decisión de Eduardo del 31-jul): latin1_upper_cp y latin1_lower_cp de builtins.c, tal cual. */
static uint32_t latin1_sube(uint32_t c) {
    if (c >= 'a' && c <= 'z')                         return c - 32u;
    if (c >= 0x00E0u && c <= 0x00FEu && c != 0x00F7u) return c - 32u;   /* ÷ no es letra */
    if (c == 0x00FFu)                                 return 0x0178u;    /* ÿ -> Ÿ */
    return c;
}
static uint32_t latin1_baja(uint32_t c) {
    if (c >= 'A' && c <= 'Z')                         return c + 32u;
    if (c >= 0x00C0u && c <= 0x00DEu && c != 0x00D7u) return c + 32u;   /* × no es letra */
    if (c == 0x0178u)                                 return 0x00FFu;    /* Ÿ -> ÿ */
    return c;
}

/* Cuatro bytes ASCII a la vez: el bit 7 marca los que están entre las dos letras (sumar 0x80 - 'a' pasa a 0x80 o más
 * lo que es >= 'a'; sumar 0x80 - ('z' + 1), lo que pasa de 'z'), y bajado al bit 5 (0x20) los cambia. Sin acarreo
 * entre bytes: ninguno pasa de 0x7F, y 0x7F + 0x3F no llega a 0x100. */
static inline uint32_t sube_palabra(uint32_t w) {
    uint32_t m = (w + 0x1F1F1F1Fu) & ~(w + 0x05050505u) & ALTOS;   /* 'a' = 0x61, 'z' + 1 = 0x7B */
    return w ^ (m >> 2);
}
static inline uint32_t baja_palabra(uint32_t w) {
    uint32_t m = (w + 0x3F3F3F3Fu) & ~(w + 0x25252525u) & ALTOS;   /* 'A' = 0x41, 'Z' + 1 = 0x5B */
    return w ^ (m >> 2);
}

/* Lo de antes —descodificar cada carácter, cambiarlo y volver a escribirlo, sin pasar de los n bytes de out— con lo
 * ASCII de 4 en 4 (en ASCII es lo mismo, y no cambia de largo). */
static uint32_t cambia(const uint8_t* p, uint32_t n, uint8_t* out, int sube) {
    uint32_t i = 0, o = 0;
    while (i < n) {
        uint32_t adv = 0, cp, m, el, k;
        uint8_t enc[4];
        if (alineado(p + i) && alineado(out + o)) {
            while (n - i >= 4u && n - o >= 4u) {
                uint32_t w = lee(p + i);
                if (w & ALTOS) break;
                escribe(out + o, sube ? sube_palabra(w) : baja_palabra(w));
                i += 4u;
                o += 4u;
            }
            if (i >= n) break;
        }
        cp = bpstr_decodifica(p + i, n - i, &adv);
        m = sube ? latin1_sube(cp) : latin1_baja(cp);
        el = bpstr_codifica(m, enc);
        for (k = 0; k < el && o < n; k++) out[o++] = enc[k];
        i += (adv > 0u) ? adv : 1u;
    }
    return o;
}

uint32_t bpstr_mayusculas(const uint8_t* p, uint32_t n, uint8_t* out) { return cambia(p, n, out, 1); }
uint32_t bpstr_minusculas(const uint8_t* p, uint32_t n, uint8_t* out) { return cambia(p, n, out, 0); }

void bpstr_recorta(const uint8_t* p, uint32_t n, uint32_t* a, uint32_t* b) {
    uint32_t x = 0, y = n;
    while (x < y && p[x] <= 0x20u) x++;
    while (y > x && p[y - 1u] <= 0x20u) y--;
    *a = x;
    *b = y;
}

/* ═══════════════════════════════ COPIAR ══════════════════════════════
 * Copiar es de lo que mejor hace cada familia, que no es lo nuestro (#730, el paso 2): en la RP2350, la memcpy de
 * newlib, escrita por Arm, iba un 53 % más deprisa que nuestro bucle de 4 en 4; en la P4 y la S3, la de su libc, un 12 y
 * un 2 %. En las ARM de Thumb-2 (la M33 de la RP2350 y de la STM32U5), la de Arm, copiada aquí: así va igual en la
 * STM32, cuya libc (newlib-nano) la trae de un byte por vuelta. En las demás, la memcpy de su libc, en bpstr.h. */

#ifdef BPSTR_COPIA_DE_ARM   /* bpstr.h: Thumb-2, little-endian y con acceso no alineado */
/*
 * La memcpy de Arm para Cortex-M (newlib/libc/machine/arm/memcpy-armv7m.S), en ensamblador dentro del C, con los #if
 * de su fuente resueltos para nuestro caso: Thumb-2 de ARMv7-M/ARMv8-M, little-endian, con acceso no alineado (lo
 * tienen la M33 de la RP2350 y la de la STM32U5), sin PAC (la M33 no lo tiene: su prólogo no guarda nada y su epílogo
 * es bx lr) y con sus tamaños de bloque (64 y 16 bytes). Lo que hace: con las dos direcciones alineadas, bloques de 64
 * bytes (16 palabras seguidas por vuelta), luego de 16, luego palabras y la cola; si no, alinea el destino y copia
 * igual, leyendo el origen sin alinear (lo hace el hardware); y con menos de 8 bytes, byte a byte. Lo nuestro: el
 * nombre, las etiquetas y su propia sección.
 *   Copyright (c) 2013 ARM Ltd
 *   All rights reserved.
 *   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 *   following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 *      disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *      following disclaimer in the documentation and/or other materials provided with the distribution.
 *   3. The name of the company may not be used to endorse or promote products derived from this software without
 *      specific prior written permission.
 *   THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ARM
 *   LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
__asm__(
    "    .pushsection .text.bpstr_copia,\"ax\",%progbits\n"
    "    .syntax  unified\n"
    "    .thumb\n"
    "    .align   2\n"
    "    .global  bpstr_copia\n"
    "    .thumb_func\n"
    "    .type    bpstr_copia, %function\n"
    "bpstr_copia:\n"                              /* r0: el destino; r1: el origen; r2: cuántos */
    "    mov      ip, r0\n"
    "    orr      r3, r1, r0\n"
    "    ands     r3, r3, #3\n"
    "    bne      .Lbpc_desalineado\n"
    ".Lbpc_bloque:\n"                             /* los bloques de 64 bytes */
    "    subs     r2, #64\n"
    "    blo      .Lbpc_medio\n"
    "    .align   2\n"
    ".Lbpc_bloque_vuelta:\n"
    "    .irp     d, 0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60\n"
    "    ldr      r3, [r1, #\\d]\n"
    "    str      r3, [r0, #\\d]\n"
    "    .endr\n"
    "    adds     r0, #64\n"
    "    adds     r1, #64\n"
    "    subs     r2, #64\n"
    "    bhs      .Lbpc_bloque_vuelta\n"
    ".Lbpc_medio:\n"                              /* los de 16 */
    "    adds     r2, #48\n"
    "    blo      .Lbpc_palabras\n"
    "    .align   2\n"
    ".Lbpc_medio_vuelta:\n"
    "    .irp     d, 0,4,8,12\n"
    "    ldr      r3, [r1, #\\d]\n"
    "    str      r3, [r0, #\\d]\n"
    "    .endr\n"
    "    adds     r0, #16\n"
    "    adds     r1, #16\n"
    "    subs     r2, #16\n"
    "    bhs      .Lbpc_medio_vuelta\n"
    ".Lbpc_palabras:\n"                           /* palabra a palabra */
    "    adds     r2, #12\n"
    "    blo      .Lbpc_menos_de_4\n"
    "    .align   2\n"
    ".Lbpc_palabras_vuelta:\n"
    "    ldr      r3, [r1], #4\n"
    "    str      r3, [r0], #4\n"
    "    subs     r2, #4\n"
    "    bhs      .Lbpc_palabras_vuelta\n"
    ".Lbpc_menos_de_4:\n"                         /* la cola: un byte si hace falta y media palabra si hace falta */
    "    adds     r2, #4\n"
    "    beq      .Lbpc_hecho\n"
    "    lsls     r2, r2, #31\n"
    "    itt      ne\n"
    "    ldrbne   r3, [r1], #1\n"
    "    strbne   r3, [r0], #1\n"
    "    bcc      .Lbpc_hecho\n"
    "    ldrh     r3, [r1]\n"
    "    strh     r3, [r0]\n"
    ".Lbpc_hecho:\n"
    "    mov      r0, ip\n"
    "    bx       lr\n"
    "    .align   2\n"
    ".Lbpc_desalineado:\n"                        /* sin alinear: con menos de 8, byte a byte */
    "    cmp      r2, #8\n"
    "    blo      .Lbpc_bytes\n"
    "    lsls     r3, r1, #30\n"                  /* el origen, alineado: a los bloques (escribe sin alinear) */
    "    beq      .Lbpc_bloque\n"
    "    ands     r3, r0, #3\n"                   /* el destino, alineado: a los bloques (lee sin alinear) */
    "    beq      .Lbpc_bloque\n"
    "    rsb      r3, r3, #4\n"                   /* alinear el destino: un byte, media palabra o las dos cosas */
    "    subs     r2, r3\n"
    "    lsls     r3, r3, #31\n"
    "    itt      ne\n"
    "    ldrbne   r3, [r1], #1\n"
    "    strbne   r3, [r0], #1\n"
    "    bcc      .Lbpc_bloque\n"
    "    ldrh     r3, [r1], #2\n"
    "    strh     r3, [r0], #2\n"
    "    b        .Lbpc_bloque\n"
    ".Lbpc_bytes:\n"
    "    subs     r2, #4\n"
    "    blo      .Lbpc_menos_de_4\n"
    ".Lbpc_bytes_vuelta:\n"
    "    subs     r2, #1\n"
    "    ldrb     r3, [r1], #1\n"
    "    strb     r3, [r0], #1\n"
    "    bhs      .Lbpc_bytes_vuelta\n"
    "    ldrb     r3, [r1]\n"
    "    strb     r3, [r0]\n"
    "    ldrb     r3, [r1, #1]\n"
    "    strb     r3, [r0, #1]\n"
    "    ldrb     r3, [r1, #2]\n"
    "    strb     r3, [r0, #2]\n"
    "    mov      r0, ip\n"
    "    bx       lr\n"
    "    .size    bpstr_copia, .-bpstr_copia\n"
    "    .popsection\n");
#endif

void bpstr_rellena(uint8_t* dst, uint8_t c, uint32_t n) {
    uint32_t i = 0, w = (uint32_t) c * UNOS;
    while (i < n && !alineado(dst + i)) dst[i++] = c;
    while (n - i >= 16u) {
        escribe(dst + i, w);
        escribe(dst + i + 4u, w);
        escribe(dst + i + 8u, w);
        escribe(dst + i + 12u, w);
        i += 16u;
    }
    while (n - i >= 4u) { escribe(dst + i, w); i += 4u; }
    for (; i < n; i++) dst[i] = c;
}
