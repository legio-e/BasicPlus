/*
 * bpstr.h — la biblioteca de cadenas de BasicPlus (V8 #730).
 *
 * Eduardo, 21-oct: «una librería en C para manejar nuestros strings. La librería debería estar optimizada a nivel de
 * algoritmo y de codificación y verificada.» Hecha aparte y, al acabar, incorporada a la VM; comparada con la libc de
 * cada familia: «Si son similares podemos dar a nuestra librería por válida» (similar: ±25 %, decidido el 3-nov). El
 * diseño, en docs/V8_IDEAS.md §E5, «#730: el diseño concreto».
 *
 * LA FRONTERA: todo trabaja sobre bytes que le dan, un puntero y una longitud. No reserva memoria y no conoce la VM:
 * quien crea la cadena (la VM, el AOT) reserva y le pide que la llene. Así se prueba sola, en el PC y en cada placa.
 *
 * LO QUE SE PROMETE, en todas: el MISMO resultado que los bucles de la VM de antes (bpvm_internal.h y builtins.c, de un
 * byte por paso), también con bytes que no son UTF-8. Lo comprueba prueba.c contra esos bucles, copiados tal cual.
 *
 * CÓMO VA DEPRISA: leyendo palabras de 4 bytes alineadas (el truco de 4 en 4: unas pocas operaciones de enteros por
 * palabra, en vez de una comparación y un salto por byte), con la cabeza y la cola byte a byte; nunca lee más allá del
 * final. Sin instrucciones especiales: el mismo C en ARM, RISC-V, Xtensa y el PC (32 bits o 64, little-endian).
 *
 * Las longitudes y las posiciones son de 32 bits: una cadena de la VM cabe de sobra.
 */
#ifndef BPSTR_H
#define BPSTR_H

#include <stdint.h>
#include <string.h>   /* memcpy: bpstr_copia, fuera de las ARM */

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════ UTF-8 ═══════════════════════════════ */

/* ¿Son todos ASCII (menores de 0x80)? Con n = 0, sí. (Antes, bpvm_bytes_ascii: lo mira cada cadena al nacer.) */
int bpstr_es_ascii(const uint8_t* p, uint32_t n);

/* Cuántos caracteres: los bytes que no son de continuación (10xxxxxx). (Antes, utf8_cp_count.) */
uint32_t bpstr_cuenta(const uint8_t* p, uint32_t n);

/* Avanza k caracteres desde el byte bo: cada paso se come el byte en el que está y los de continuación que le siguen.
 * Devuelve dónde queda (n si se acabó antes) y, en *hechos (si no es NULL), los pasos dados: k, o menos si llegó al
 * final. (Antes, el paso hacia delante del cursor, strcur_byte_de, y utf8_byte_offset.) */
uint32_t bpstr_avanza(const uint8_t* p, uint32_t n, uint32_t bo, uint32_t k, uint32_t* hechos);

/* Retrocede k caracteres desde el byte bo: cada paso baja uno y salta hacia atrás los de continuación; no baja del 0.
 * (Antes, el paso hacia atrás del cursor.) */
uint32_t bpstr_retrocede(const uint8_t* p, uint32_t bo, uint32_t k);

/* La regla de #711 (Unicode §3.9, «U+FFFD Substitution of Maximal Subparts»): un trozo malo es el principio más largo
 * de un carácter que se corta, o un byte que no puede empezar ninguno; fuera quedan los sobrelargos, los sustitutos y
 * lo que pasa de 10FFFF.
 *   bpstr_primer_malo:     dónde empieza el primer trozo malo; -1 si todo es UTF-8 válido;
 *   bpstr_largo_corregido: los bytes que salen al cambiar cada trozo malo por U+FFFD (tres bytes);
 *   bpstr_corrige:         los escribe en out (que no se solapa con p) y devuelve cuántos.
 * (Antes, utf8_primer_malo, utf8_largo_corregido y utf8_corrige.) */
int32_t  bpstr_primer_malo(const uint8_t* p, uint32_t n);
uint32_t bpstr_largo_corregido(const uint8_t* p, uint32_t n);
uint32_t bpstr_corrige(const uint8_t* p, uint32_t n, uint8_t* out);

/* Un carácter: el que empieza en p[0] (*adv, los bytes que ocupa; un byte que no empieza ninguno vale por sí mismo,
 * como Latin-1), y escribirlo (devuelve sus bytes, de 1 a 4). (Antes, utf8_decode y utf8_encode: los mismos.) */
static inline uint32_t bpstr_decodifica(const uint8_t* p, uint32_t n, uint32_t* adv) {
    uint8_t b0;
    if (n == 0) { *adv = 0; return 0; }
    b0 = p[0];
    if (b0 < 0x80u)                     { *adv = 1; return b0; }
    if ((b0 & 0xE0u) == 0xC0u && n >= 2) { *adv = 2; return ((uint32_t) (b0 & 0x1Fu) << 6) | (p[1] & 0x3Fu); }
    if ((b0 & 0xF0u) == 0xE0u && n >= 3) {
        *adv = 3;
        return ((uint32_t) (b0 & 0x0Fu) << 12) | ((uint32_t) (p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu);
    }
    if ((b0 & 0xF8u) == 0xF0u && n >= 4) {
        *adv = 4;
        return ((uint32_t) (b0 & 0x07u) << 18) | ((uint32_t) (p[1] & 0x3Fu) << 12) | ((uint32_t) (p[2] & 0x3Fu) << 6)
               | (p[3] & 0x3Fu);
    }
    *adv = 1;
    return b0;
}
static inline uint32_t bpstr_codifica(uint32_t cp, uint8_t* out) {
    if (cp < 0x80u)    { out[0] = (uint8_t) cp; return 1; }
    if (cp < 0x800u)   { out[0] = (uint8_t) (0xC0u | (cp >> 6)); out[1] = (uint8_t) (0x80u | (cp & 0x3Fu)); return 2; }
    if (cp < 0x10000u) {
        out[0] = (uint8_t) (0xE0u | (cp >> 12)); out[1] = (uint8_t) (0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (uint8_t) (0x80u | (cp & 0x3Fu));
        return 3;
    }
    out[0] = (uint8_t) (0xF0u | (cp >> 18)); out[1] = (uint8_t) (0x80u | ((cp >> 12) & 0x3Fu));
    out[2] = (uint8_t) (0x80u | ((cp >> 6) & 0x3Fu)); out[3] = (uint8_t) (0x80u | (cp & 0x3Fu));
    return 4;
}

/* ═══════════════════════════════ BUSCAR ══════════════════════════════
 * Por bytes: en UTF-8 una aguja bien formada sólo casa en el principio de un carácter, así que el resultado es el de
 * buscar por caracteres. Las posiciones son de BYTES. */

/* El primer byte c de p; -1 si no está. */
int32_t bpstr_busca_byte(const uint8_t* p, uint32_t n, uint8_t c);

/* La primera aparición de q (qn bytes) en p que empiece en desde o después; -1 si no hay. Una aguja vacía casa en
 * desde (en n si desde pasa de n), como en Java. Nunca cuadrático: hasta 256 bytes, la aguja va por un camino lineal
 * porque no pasa de su tope (por parejas, el memmem de Arm, si tiene de 3 en adelante); más larga, por Two-Way, lineal
 * siempre. (Antes, bp_find_bytes: indexOf, contains, split, replace.) */
int32_t bpstr_busca(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn, uint32_t desde);

/* La última aparición de q que empiece en hasta o antes; -1 si no hay. La aguja vacía casa en hasta (en n si pasa de
 * n), como lastIndexOf de Java. */
int32_t bpstr_busca_atras(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn, uint32_t hasta);

/* Cuántas veces está q, sin solaparse, de izquierda a derecha (lo que cuenta replace). Con la aguja vacía, 0: ese caso
 * lo resuelve quien llama. */
uint32_t bpstr_cuenta_apariciones(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn);

/* ═══════════════════════════════ COMPARAR ════════════════════════════
 * Por bytes sin signo; si una es el principio de la otra, la más corta va antes. En UTF-8 es el orden de los
 * caracteres. (Antes, memcmp y las longitudes: Str.compare, <, <=, >, >= y ==.) */

int bpstr_compara(const uint8_t* a, uint32_t na, const uint8_t* b, uint32_t nb);   /* -1, 0 o 1 */
int bpstr_igual(const uint8_t* a, uint32_t na, const uint8_t* b, uint32_t nb);     /* 1 si son iguales */

/* ═══════════════════════════════ TRANSFORMAR ═════════════════════════ */

/* Mayúsculas y minúsculas LATIN-1 (#348): ASCII y el bloque U+00C0..U+00FF ('ß' se queda; 'ÿ' <-> 'Ÿ'). En UTF-8 los
 * bytes no cambian de número, así que out tiene los n de p; devuelve cuántos escribió (n, salvo con bytes que no son
 * UTF-8: entonces menos, y quien llama pone el resto a cero, como antes). out no se solapa con p. */
uint32_t bpstr_mayusculas(const uint8_t* p, uint32_t n, uint8_t* out);
uint32_t bpstr_minusculas(const uint8_t* p, uint32_t n, uint8_t* out);

/* Los bordes de trim: [*a, *b) sin los bytes de 0 a 32 de los dos extremos (ninguno es de continuación). */
void bpstr_recorta(const uint8_t* p, uint32_t n, uint32_t* a, uint32_t* b);

/* ═══════════════════════════════ COPIAR ══════════════════════════════
 * Como memcpy (sin solaparse) y memset. Copiar, lo mejor de cada familia: en las ARM de Thumb-2, la memcpy de Arm,
 * copiada en bpstr.c (así también en la STM32, cuya libc copia byte a byte); en las demás, la memcpy de su libc, aquí
 * mismo, sin una llamada de más (en la S3, con 16 bytes, esa llamada costaba un 23 %). Rellenar, la nuestra. */

#if defined(__ARM_ARCH_ISA_THUMB) && __ARM_ARCH_ISA_THUMB == 2 && !defined(__ARM_ARCH_ISA_ARM) && \
    defined(__ARM_FEATURE_UNALIGNED) && !defined(__ARM_BIG_ENDIAN)
#define BPSTR_COPIA_DE_ARM 1
void bpstr_copia(uint8_t* dst, const uint8_t* src, uint32_t n);
#else
static inline void bpstr_copia(uint8_t* dst, const uint8_t* src, uint32_t n) { if (n) memcpy(dst, src, n); }
#endif
void bpstr_rellena(uint8_t* dst, uint8_t c, uint32_t n);

#ifdef __cplusplus
}
#endif

#endif /* BPSTR_H */
