/*
 * oraculo.h — los bucles de la VM de antes de bpstr (V8 #730), COPIADOS TAL CUAL de include/bpvm_internal.h y
 * src/builtins.c, con sus nombres de entonces: lo que tiene que dar bpstr, byte a byte. Lo usan prueba.c (contra esto
 * se comprueba) y banco.c (contra esto se mide «hoy»). No se toca: si cambiara, la prueba ya no diría nada.
 */
#ifndef BPSTR_ORACULO_H
#define BPSTR_ORACULO_H

#include <stdint.h>
#include <string.h>
#include "bpstr.h"

#define BP_CONT(b) (((b) & 0xC0u) == 0x80u)

/* bpvm_internal.h: bpvm_bytes_ascii */
static inline int bpvm_bytes_ascii(const uint8_t* p, uint32_t n) {
    uint8_t acc = 0;
    for (uint32_t i = 0; i < n; i++) acc |= p[i];
    return (acc & 0x80u) == 0;
}
/* bpvm_internal.h: utf8_cp_count */
static inline uint32_t utf8_cp_count(const uint8_t* p, uint32_t nbytes) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < nbytes; i++)
        if ((p[i] & 0xC0u) != 0x80u) count++;
    return count;
}
/* builtins.c, strcur_byte_de: el paso hacia delante del cursor */
static inline uint32_t ref_avanza(const uint8_t* p, uint32_t n, uint32_t bo, uint32_t k, uint32_t* hechos) {
    uint32_t ci = 0;
    while (ci < k && bo < n) {
        bo++;
        while (bo < n && BP_CONT(p[bo])) bo++;
        ci++;
    }
    *hechos = ci;
    return bo;
}
/* builtins.c, strcur_byte_de: el paso hacia atrás (aquí sin bajar del 0; el cursor nunca lo pide) */
static inline uint32_t ref_retrocede(const uint8_t* p, uint32_t bo, uint32_t k) {
    while (k > 0 && bo > 0) {
        bo--;
        while (bo > 0u && BP_CONT(p[bo])) bo--;
        k--;
    }
    return bo;
}
/* bpvm_internal.h: utf8_paso, utf8_primer_malo, utf8_largo_corregido, utf8_corrige (#711) */
static inline int utf8_paso(const uint8_t* p, uint32_t i, uint32_t n) {
    uint8_t c = p[i];
    int necesita;
    uint8_t lo = 0x80u, hi = 0xBFu;
    if (c >= 0xC2u && c <= 0xDFu) {
        necesita = 1;
    } else if (c >= 0xE0u && c <= 0xEFu) {
        necesita = 2;
        if (c == 0xE0u) lo = 0xA0u;
        else if (c == 0xEDu) hi = 0x9Fu;
    } else if (c >= 0xF0u && c <= 0xF4u) {
        necesita = 3;
        if (c == 0xF0u) lo = 0x90u;
        else if (c == 0xF4u) hi = 0x8Fu;
    } else {
        return -1;
    }
    for (int k = 1; k <= necesita; k++) {
        if (i + (uint32_t) k >= n) return -k;
        uint8_t d = p[i + (uint32_t) k];
        if (d < lo || d > hi) return -k;
        lo = 0x80u;
        hi = 0xBFu;
    }
    return necesita + 1;
}
static inline int32_t utf8_primer_malo(const uint8_t* p, uint32_t n) {
    uint32_t i = 0;
    while (i < n) {
        if (p[i] < 0x80u) { i++; continue; }
        int s = utf8_paso(p, i, n);
        if (s < 0) return (int32_t) i;
        i += (uint32_t) s;
    }
    return -1;
}
static inline uint32_t utf8_largo_corregido(const uint8_t* p, uint32_t n) {
    uint32_t i = 0, largo = 0;
    while (i < n) {
        int s = (p[i] < 0x80u) ? 1 : utf8_paso(p, i, n);
        if (s > 0) { largo += (uint32_t) s; i += (uint32_t) s; }
        else { largo += 3u; i += (uint32_t) -s; }
    }
    return largo;
}
static inline uint32_t utf8_corrige(const uint8_t* p, uint32_t n, uint8_t* out) {
    uint32_t i = 0, o = 0;
    while (i < n) {
        int s = (p[i] < 0x80u) ? 1 : utf8_paso(p, i, n);
        if (s > 0) {
            for (int k = 0; k < s; k++) out[o++] = p[i + (uint32_t) k];
            i += (uint32_t) s;
        } else {
            out[o++] = 0xEFu; out[o++] = 0xBFu; out[o++] = 0xBDu;
            i += (uint32_t) -s;
        }
    }
    return o;
}
/* builtins.c: bp_find_bytes (indexOf, contains, split) */
static inline long bp_find_bytes(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn, uint32_t from) {
    if (qn == 0) return (long) (from <= n ? from : n);
    if (qn > n) return -1;
    for (uint32_t i = from; i + qn <= n; i++) {
        if (memcmp(p + i, q, qn) == 0) return (long) i;
    }
    return -1;
}
/* lastIndexOf de Java: la última que empieza en `hasta` o antes (hoy, Str.lastIndexOf en BP) */
static inline long ref_busca_atras(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn, uint32_t hasta) {
    if (qn == 0) return (long) (hasta <= n ? hasta : n);
    if (qn > n) return -1;
    uint32_t i = n - qn;
    if (hasta < i) i = hasta;
    for (;;) {
        if (memcmp(p + i, q, qn) == 0) return (long) i;
        if (i == 0) return -1;
        i--;
    }
}
/* builtins.c, BUILTIN_REPLACE: la primera pasada, contar */
static inline uint32_t ref_apariciones(const uint8_t* sb, uint32_t sn, const uint8_t* tb, uint32_t tn) {
    uint32_t hits = 0;
    if (tn == 0) return 0;
    for (uint32_t i = 0; i + tn <= sn; ) {
        if (memcmp(sb + i, tb, tn) == 0) { hits++; i += tn; } else i++;
    }
    return hits;
}
/* builtins.c, BUILTIN_STR_COMPARE */
static inline int ref_compara(const uint8_t* a, uint32_t na, const uint8_t* b, uint32_t nb) {
    uint32_t n = (na < nb) ? na : nb;
    int r = (n > 0u) ? memcmp(a, b, n) : 0;
    if (r == 0) r = (na < nb) ? -1 : (na > nb) ? 1 : 0;
    return (r < 0) ? -1 : (r > 0) ? 1 : 0;
}
/* builtins.c, BUILTIN_UPPER / BUILTIN_LOWER: el rápido si la cadena es ASCII; si no, descodificar y volver a escribir
 * (utf8_decode y utf8_encode son bpstr_decodifica y bpstr_codifica, los mismos) */
static inline uint32_t latin1_upper_cp(uint32_t c) {
    if (c >= 'a' && c <= 'z')            return c - 32;
    if (c >= 0x00E0 && c <= 0x00FE && c != 0x00F7) return c - 32;
    if (c == 0x00FF)                     return 0x0178;
    return c;
}
static inline uint32_t latin1_lower_cp(uint32_t c) {
    if (c >= 'A' && c <= 'Z')            return c + 32;
    if (c >= 0x00C0 && c <= 0x00DE && c != 0x00D7) return c + 32;
    if (c == 0x0178)                     return 0x00FF;
    return c;
}
static inline uint32_t ref_cambia(const uint8_t* p, uint32_t n, uint8_t* out, int up) {
    if (bpvm_bytes_ascii(p, n)) {
        for (uint32_t i = 0; i < n; i++) {
            uint8_t c = p[i];
            if (up ? (c >= 'a' && c <= 'z') : (c >= 'A' && c <= 'Z')) c = (uint8_t) (c ^ 0x20u);
            out[i] = c;
        }
        return n;
    }
    uint32_t i = 0, o = 0;
    while (i < n) {
        uint32_t adv = 0;
        uint32_t cp = bpstr_decodifica(p + i, n - i, &adv);
        uint32_t m = up ? latin1_upper_cp(cp) : latin1_lower_cp(cp);
        uint8_t enc[4];
        uint32_t el = bpstr_codifica(m, enc);
        for (uint32_t k = 0; k < el && o < n; k++) out[o++] = enc[k];
        i += (adv > 0) ? adv : 1;
    }
    return o;
}
/* builtins.c, BUILTIN_TRIM */
static inline void ref_recorta(const uint8_t* p, uint32_t n, uint32_t* a, uint32_t* b) {
    uint32_t x = 0, e = n;
    if (n > 0) {
        while (x < e && p[x] <= 0x20) x++;
        while (e > x && p[e - 1] <= 0x20) e--;
    }
    *a = x;
    *b = e;
}

#endif /* BPSTR_ORACULO_H */
