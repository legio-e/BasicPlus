/*
 * bpvm_env.c — H9: formato + lógica del bloque de env (identidad de placa).
 * SIN heap, dependencia mínima (solo crc32) — pensado para correr en el kernel
 * del estado 0. La cintura de flash (leer/borrar/escribir el sector) NO está
 * aquí: es por-micro y llega en su fase. Ver bpvm_env.h + docs/H9_KERNEL_CAPAS.md.
 */
#include "bpvm_env.h"
#include "crc32.h"

#include <string.h>
#include <stdlib.h>   /* strtol */

/* --- I/O big-endian local (sin arrastrar bpvm_internal.h: esto es del suelo) --- */
static uint16_t rd16(const uint8_t* p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}
static uint32_t rd32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}
static void wr16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)(v & 0xFF);
}
static void wr32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)(v & 0xFF);
}

int bpvm_env_parse(const uint8_t* block, size_t block_len, bpvm_env_t* env) {
    if (env) { env->payload = NULL; env->payload_len = 0; env->seq = 0; env->valid = 0; }
    if (!block || !env || block_len < BPVM_ENV_HEADER_SIZE) return 0;
    if (memcmp(block, BPVM_ENV_MAGIC, 4) != 0) return 0;          /* no es un bloque env */
    if (rd16(block + 4) != (uint16_t) BPVM_ENV_VERSION) return 0;  /* versión desconocida */
    uint16_t len = rd16(block + 6);
    uint32_t crc = rd32(block + 8);
    uint32_t seq = rd32(block + 12);
    if ((size_t) BPVM_ENV_HEADER_SIZE + (size_t) len > block_len) return 0;  /* len fuera de rango */
    /* CRC sobre [seq + payload] = block[12 .. 16+len) (contiguo). */
    if (bpvm_crc32(block + 12, (size_t) 4 + (size_t) len) != crc) return 0;
    env->payload = block + BPVM_ENV_HEADER_SIZE;
    env->payload_len = len;
    env->seq = seq;
    env->valid = 1;
    return 1;
}

int bpvm_env_pick(const uint8_t* blockA, size_t lenA,
                  const uint8_t* blockB, size_t lenB, bpvm_env_t* out) {
    bpvm_env_t a, b;
    int va = bpvm_env_parse(blockA, lenA, &a);
    int vb = bpvm_env_parse(blockB, lenB, &b);
    if (va && vb) {
        if (b.seq > a.seq) { if (out) *out = b; return 1; }
        if (out) *out = a;
        return 0;
    }
    if (va) { if (out) *out = a; return 0; }
    if (vb) { if (out) *out = b; return 1; }
    if (out) { out->payload = NULL; out->payload_len = 0; out->seq = 0; out->valid = 0; }
    return -1;
}

/* Comparación de CLAVES case-insensitive (decisión de Eduardo, 19-jul: en una
 * clave de env las mayúsculas no le importan a nadie y solo confunden —
 * "PSRAM" y "psram" son LA MISMA clave, tanto al leer como al reemplazar o
 * borrar; así no pueden coexistir duplicadas que difieren solo en caja). ASCII. */
static int key_eq_ci(const void* a, const void* b, size_t n) {
    const unsigned char* x = (const unsigned char*) a;
    const unsigned char* y = (const unsigned char*) b;
    for (size_t i = 0; i < n; i++) {
        unsigned char cx = x[i], cy = y[i];
        if (cx >= 'A' && cx <= 'Z') cx = (unsigned char)(cx + 32);
        if (cy >= 'A' && cy <= 'Z') cy = (unsigned char)(cy + 32);
        if (cx != cy) return 0;
    }
    return 1;
}

int bpvm_env_get(const bpvm_env_t* env, const char* key, char* val_out, size_t val_cap) {
    if (!env || !env->valid || !key) return -1;
    size_t klen = strlen(key);
    const uint8_t* p   = env->payload;
    const uint8_t* end = p + env->payload_len;
    while (p < end) {
        const uint8_t* ls = p;                       /* inicio de línea */
        const uint8_t* le = ls;                       /* fin de línea (al '\n' o al final) */
        while (le < end && *le != '\n') le++;
        const uint8_t* eq = ls;                       /* posición del '=' dentro de la línea */
        while (eq < le && *eq != '=') eq++;
        if (eq < le) {                                /* línea con '=' */
            size_t nlen = (size_t)(eq - ls);
            if (nlen == klen && key_eq_ci(ls, key, klen)) {
                const uint8_t* vs = eq + 1;
                size_t vlen = (size_t)(le - vs);
                if (val_out && val_cap) {
                    size_t c = (vlen < val_cap - 1) ? vlen : (val_cap - 1);
                    memcpy(val_out, vs, c);
                    val_out[c] = '\0';
                }
                return (int) vlen;
            }
        }
        p = (le < end) ? le + 1 : end;                /* saltar el '\n' */
    }
    return -1;                                         /* clave desconocida → tolerante */
}

int bpvm_env_get_bool(const bpvm_env_t* env, const char* key, int def) {
    char v[8];
    int n = bpvm_env_get(env, key, v, sizeof v);
    if (n < 0) return def;
    if (n == 1) {
        switch (v[0]) {
        case '1': case 'y': case 'Y': case 't': case 'T':
        case 's': case 'S':                               return 1;  /* sí */
        case '0': case 'n': case 'N': case 'f': case 'F': return 0;
        default: break;
        }
    }
    if (!strcmp(v, "true") || !strcmp(v, "yes") || !strcmp(v, "on")
        || !strcmp(v, "si") || !strcmp(v, "Si") || !strcmp(v, "SI")) return 1;
    if (!strcmp(v, "false")|| !strcmp(v, "no")  || !strcmp(v, "off")) return 0;
    return def;
}

long bpvm_env_get_long(const bpvm_env_t* env, const char* key, long def) {
    char v[24];
    int n = bpvm_env_get(env, key, v, sizeof v);
    if (n <= 0) return def;
    char* endp = NULL;
    long r = strtol(v, &endp, 0);   /* base 0: acepta 0x… y decimal */
    if (endp == v) return def;      /* nada parseable */
    return r;
}

int bpvm_env_serialize(const char* payload, size_t payload_len, uint32_t seq,
                       uint8_t* out, size_t out_cap) {
    if (!out) return -1;
    if (payload_len > BPVM_ENV_MAX_PAYLOAD) return -1;
    size_t used = (size_t) BPVM_ENV_HEADER_SIZE + payload_len;
    if (used > out_cap) return -1;
    memcpy(out, BPVM_ENV_MAGIC, 4);
    wr16(out + 4, (uint16_t) BPVM_ENV_VERSION);
    wr16(out + 6, (uint16_t) payload_len);
    wr32(out + 12, seq);
    if (payload_len && payload) memcpy(out + BPVM_ENV_HEADER_SIZE, payload, payload_len);
    /* crc sobre [seq + payload] = out[12 .. 16+payload_len) */
    wr32(out + 8, bpvm_crc32(out + 12, (size_t) 4 + payload_len));
    if (used < out_cap) memset(out + used, 0xFF, out_cap - used);   /* pad flash-borrada */
    return (int) used;
}

int bpvm_env_payload_set(const char* payload_in, size_t in_len, const char* key,
                         const char* value, char* out, size_t out_cap) {
    if (!out || !key) return -1;
    size_t klen = strlen(key);
    size_t o = 0;
    const char* p   = payload_in;
    const char* end = payload_in ? payload_in + in_len : payload_in;

    /* Copiar todas las líneas cuyo NOMBRE != key (así reemplazamos o borramos). */
    while (payload_in && p < end) {
        const char* ls = p;
        const char* le = ls;
        while (le < end && *le != '\n') le++;
        const char* eq = ls;
        while (eq < le && *eq != '=') eq++;
        size_t nlen = (size_t)(eq - ls);
        int match = (nlen == klen && key_eq_ci(ls, key, klen));
        if (!match) {
            size_t linelen = (size_t)(le - ls) + (le < end ? 1u : 0u);   /* incluye el '\n' */
            if (o + linelen > out_cap) return -1;
            memcpy(out + o, ls, linelen);
            o += linelen;
        }
        p = (le < end) ? le + 1 : end;
    }

    /* Añadir la nueva línea `key=value\n` si es un SET (value != NULL). */
    if (value) {
        if (o > 0 && out[o - 1] != '\n') {   /* asegurar separador */
            if (o + 1 > out_cap) return -1;
            out[o++] = '\n';
        }
        size_t vlen = strlen(value);
        if (o + klen + 1 + vlen + 1 > out_cap) return -1;
        memcpy(out + o, key, klen); o += klen;
        out[o++] = '=';
        memcpy(out + o, value, vlen); o += vlen;
        out[o++] = '\n';
    }
    return (int) o;
}

int bpvm_env_count(const bpvm_env_t* env) {
    if (!env || !env->valid) return 0;
    const uint8_t* p   = env->payload;
    const uint8_t* end = p + env->payload_len;
    int n = 0;
    while (p < end) {
        const uint8_t* ls = p;
        const uint8_t* le = ls;
        while (le < end && *le != '\n') le++;
        const uint8_t* eq = ls;
        while (eq < le && *eq != '=') eq++;
        if (eq < le) n++;                             /* solo líneas con '=' cuentan */
        p = (le < end) ? le + 1 : end;
    }
    return n;
}

int bpvm_env_pair_at(const bpvm_env_t* env, int idx,
                     char* key, size_t kcap, char* val, size_t vcap) {
    if (!env || !env->valid || idx < 0) return 0;
    const uint8_t* p   = env->payload;
    const uint8_t* end = p + env->payload_len;
    int n = 0;
    while (p < end) {
        const uint8_t* ls = p;
        const uint8_t* le = ls;
        while (le < end && *le != '\n') le++;
        const uint8_t* eq = ls;
        while (eq < le && *eq != '=') eq++;
        if (eq < le) {                                /* línea con '=' */
            if (n == idx) {
                size_t klen = (size_t)(eq - ls);
                size_t vlen = (size_t)(le - (eq + 1));
                if (key && kcap) {
                    size_t c = (klen < kcap - 1) ? klen : (kcap - 1);
                    memcpy(key, ls, c); key[c] = '\0';
                }
                if (val && vcap) {
                    size_t c = (vlen < vcap - 1) ? vlen : (vcap - 1);
                    memcpy(val, eq + 1, c); val[c] = '\0';
                }
                return 1;
            }
            n++;
        }
        p = (le < end) ? le + 1 : end;
    }
    return 0;                                          /* idx fuera de rango */
}

int bpvm_env_apply(uint8_t* a, uint8_t* b, size_t sector,
                   uint8_t* scratch, size_t scratch_cap,
                   const char* key, const char* value, int* wrote_slot) {
    if (!a || !b || !scratch || !key) return -1;
    bpvm_env_t cur;
    int who = bpvm_env_pick(a, sector, b, sector, &cur);   /* 0=A, 1=B, -1=ninguna */
    const char* pin = (who >= 0 && cur.valid) ? (const char*) cur.payload : NULL;
    size_t pin_len  = (who >= 0 && cur.valid) ? cur.payload_len : 0;
    /* payload_set lee de `pin` (dentro de la copia que gana) y escribe en `scratch`
     * (disjunto): nunca pisamos la copia actual. */
    int n = bpvm_env_payload_set(pin, pin_len, key, value, (char*) scratch, scratch_cap);
    if (n < 0) return -1;
    uint32_t ns = bpvm_env_next_seq(a, sector, b, sector);
    int stale = (who == 0) ? 1 : 0;    /* escribe en la que NO es actual; virgen→A(0) */
    uint8_t* dst = (stale == 0) ? a : b;
    int u = bpvm_env_serialize((const char*) scratch, (size_t) n, ns, dst, sector);
    if (u < 0) return -1;
    if (wrote_slot) *wrote_slot = stale;
    return 0;
}

uint32_t bpvm_env_next_seq(const uint8_t* blockA, size_t lenA,
                           const uint8_t* blockB, size_t lenB) {
    bpvm_env_t a, b;
    int va = bpvm_env_parse(blockA, lenA, &a);
    int vb = bpvm_env_parse(blockB, lenB, &b);
    uint32_t hi = 0;
    if (va && a.seq > hi) hi = a.seq;
    if (vb && b.seq > hi) hi = b.seq;
    return hi + 1u;   /* si ninguna válida → 1 */
}
