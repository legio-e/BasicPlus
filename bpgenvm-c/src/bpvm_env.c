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
            if (nlen == klen && memcmp(ls, key, klen) == 0) {
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
        case '1': case 'y': case 'Y': case 't': case 'T': return 1;
        case '0': case 'n': case 'N': case 'f': case 'F': return 0;
        default: break;
        }
    }
    if (!strcmp(v, "true") || !strcmp(v, "yes") || !strcmp(v, "on"))  return 1;
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
