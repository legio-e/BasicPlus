/*
 * json_min.c — implementación del parser JSON minimal.
 *
 * Tokenización lineal: avanzamos un cursor por el buffer y, en cada
 * iteración del objeto top-level, leemos {key, ":", value, ","? }.
 *
 * El estilo es defensivo: cualquier carácter inesperado → -1.
 */

#include "json_min.h"

#include <string.h>

/* ============================ Helpers ============================ */

/* Salta whitespace ASCII estándar. */
static const char* skip_ws(const char* p, const char* end) {
    while (p < end) {
        char c = *p;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') p++;
        else break;
    }
    return p;
}

/* Lee un literal exacto (e.g. "true", "false", "null"). Devuelve
 * el cursor avanzado o NULL si no coincide. */
static const char* match_literal(const char* p, const char* end, const char* lit) {
    size_t n = strlen(lit);
    if ((size_t)(end - p) < n) return NULL;
    if (memcmp(p, lit, n) != 0) return NULL;
    return p + n;
}

/* Lee un string JSON con comillas. Devuelve el puntero PASADAS las
 * comillas finales. Rellena *content_start y *content_len con el
 * interior (sin comillas, con escapes literales).
 *
 * Soporta solo escapes \", \\, \n, \r, \t — los demás se mantienen
 * literales (defensivo). */
static const char* parse_string(const char* p, const char* end,
                                 const char** content_start,
                                 size_t* content_len) {
    if (p >= end || *p != '"') return NULL;
    p++;
    *content_start = p;
    while (p < end) {
        if (*p == '\\') {
            /* Saltar el siguiente char (sea cual sea). */
            p++;
            if (p >= end) return NULL;
            p++;
            continue;
        }
        if (*p == '"') {
            *content_len = (size_t)(p - *content_start);
            return p + 1;
        }
        p++;
    }
    return NULL;   /* string sin cerrar */
}

/* Lee un número entero (con signo). Avanza p hasta el primer char
 * no-dígito (que puede ser ',' o '}' o ws). */
static const char* parse_number(const char* p, const char* end,
                                 const char** num_start, size_t* num_len) {
    *num_start = p;
    if (p < end && (*p == '-' || *p == '+')) p++;
    if (p >= end || *p < '0' || *p > '9') return NULL;
    while (p < end && *p >= '0' && *p <= '9') p++;
    *num_len = (size_t)(p - *num_start);
    return p;
}

/* Lee un objeto anidado: empieza en '{', termina en el '}' que
 * cierra al mismo nivel (cuenta llaves). Devuelve el cursor pasado
 * el '}' final. Rellena content_start y content_len con TODO el
 * objeto incluyendo las llaves (e.g. "{a:1}"). */
static const char* parse_nested_object(const char* p, const char* end,
                                        const char** content_start,
                                        size_t* content_len) {
    if (p >= end || *p != '{') return NULL;
    *content_start = p;
    int depth = 0;
    while (p < end) {
        char c = *p;
        if (c == '"') {
            /* Saltar string completa. */
            const char* dummy_s; size_t dummy_l;
            p = parse_string(p, end, &dummy_s, &dummy_l);
            if (p == NULL) return NULL;
            continue;
        }
        if (c == '{') depth++;
        else if (c == '}') {
            depth--;
            if (depth == 0) {
                p++;
                *content_len = (size_t)(p - *content_start);
                return p;
            }
        }
        p++;
    }
    return NULL;   /* objeto sin cerrar */
}

/* ============================ Parser ============================ */

int json_parse(const char* buf, size_t len, json_obj_t* out) {
    out->n_pairs = 0;
    if (buf == NULL || len < 2) return -1;
    const char* p   = buf;
    const char* end = buf + len;
    p = skip_ws(p, end);
    if (p >= end || *p != '{') return -1;
    p++;
    p = skip_ws(p, end);
    /* Caso "{}": objeto vacío. */
    if (p < end && *p == '}') return 0;

    for (;;) {
        if (out->n_pairs >= JSON_MAX_PAIRS) return -1;
        json_pair_t* pr = &out->pairs[out->n_pairs];

        /* Key: string. */
        p = skip_ws(p, end);
        p = parse_string(p, end, &pr->key, &pr->key_len);
        if (p == NULL) return -1;

        /* ':' */
        p = skip_ws(p, end);
        if (p >= end || *p != ':') return -1;
        p++;

        /* Value: detectar tipo por primer char. */
        p = skip_ws(p, end);
        if (p >= end) return -1;
        char c = *p;
        if (c == '"') {
            p = parse_string(p, end, &pr->value, &pr->value_len);
            if (p == NULL) return -1;
            pr->type = JSON_TYPE_STRING;
        } else if (c == '{') {
            p = parse_nested_object(p, end, &pr->value, &pr->value_len);
            if (p == NULL) return -1;
            pr->type = JSON_TYPE_OBJECT;
        } else if (c == 't') {
            const char* q = match_literal(p, end, "true");
            if (q == NULL) return -1;
            pr->value     = p;
            pr->value_len = 4;
            pr->type      = JSON_TYPE_BOOL;
            p             = q;
        } else if (c == 'f') {
            const char* q = match_literal(p, end, "false");
            if (q == NULL) return -1;
            pr->value     = p;
            pr->value_len = 5;
            pr->type      = JSON_TYPE_BOOL;
            p             = q;
        } else if (c == 'n') {
            const char* q = match_literal(p, end, "null");
            if (q == NULL) return -1;
            pr->value     = p;
            pr->value_len = 4;
            pr->type      = JSON_TYPE_NULL;
            p             = q;
        } else {
            /* Asumimos número. */
            p = parse_number(p, end, &pr->value, &pr->value_len);
            if (p == NULL) return -1;
            pr->type = JSON_TYPE_NUMBER;
        }

        out->n_pairs++;

        /* ',' (siguiente par) o '}' (fin). */
        p = skip_ws(p, end);
        if (p >= end) return -1;
        if (*p == ',') { p++; continue; }
        if (*p == '}') return 0;
        return -1;
    }
}

/* ============================ Acceso ============================ */

const json_pair_t* json_find(const json_obj_t* obj, const char* key) {
    size_t klen = strlen(key);
    for (int i = 0; i < obj->n_pairs; i++) {
        const json_pair_t* p = &obj->pairs[i];
        if (p->key_len == klen && memcmp(p->key, key, klen) == 0) {
            return p;
        }
    }
    return NULL;
}

long json_get_long(const json_obj_t* obj, const char* key, long def) {
    const json_pair_t* p = json_find(obj, key);
    if (p == NULL || p->type != JSON_TYPE_NUMBER) return def;
    /* Parse manual (sin allocs ni strtol que requiere null-terminated). */
    long sign = 1;
    const char* q = p->value;
    const char* qe = p->value + p->value_len;
    if (q < qe && *q == '-') { sign = -1; q++; }
    else if (q < qe && *q == '+') { q++; }
    long v = 0;
    while (q < qe && *q >= '0' && *q <= '9') {
        v = v * 10 + (*q - '0');
        q++;
    }
    return sign * v;
}

int json_get_bool(const json_obj_t* obj, const char* key, int def) {
    const json_pair_t* p = json_find(obj, key);
    if (p == NULL || p->type != JSON_TYPE_BOOL) return def;
    return (p->value_len == 4) ? 1 : 0;   /* "true" tiene 4, "false" 5 */
}

int json_get_str(const json_obj_t* obj, const char* key,
                 char* dst, size_t dst_size) {
    if (dst_size == 0) return -1;
    dst[0] = '\0';
    const json_pair_t* p = json_find(obj, key);
    if (p == NULL || p->type != JSON_TYPE_STRING) return -1;
    /* Desescape: copia char a char interpretando \\X. */
    size_t out = 0;
    const char* q  = p->value;
    const char* qe = p->value + p->value_len;
    while (q < qe && out + 1 < dst_size) {
        char c = *q++;
        if (c == '\\' && q < qe) {
            char esc = *q++;
            switch (esc) {
                case 'n':  c = '\n'; break;
                case 'r':  c = '\r'; break;
                case 't':  c = '\t'; break;
                case '"':  c = '"';  break;
                case '\\': c = '\\'; break;
                case '/':  c = '/';  break;
                default:   c = esc;  break;   /* \uXXXX y otros: literal */
            }
        }
        dst[out++] = c;
    }
    dst[out] = '\0';
    return (int) out;
}

const char* json_get_str_raw(const json_obj_t* obj, const char* key,
                              size_t* out_len) {
    const json_pair_t* p = json_find(obj, key);
    if (p == NULL || p->type != JSON_TYPE_STRING) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    if (out_len) *out_len = p->value_len;
    return p->value;
}
