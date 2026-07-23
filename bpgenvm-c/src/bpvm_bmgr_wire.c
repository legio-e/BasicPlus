/*
 * bpvm_bmgr_wire.c — H9: núcleo único del adaptador de wire (STATE, ENV_x, PART_x).
 * Construye replies JSON idénticas para el boardsim (host) y el firmware (device).
 * Sin heap, sin sockets, sin JSON-parse (el llamador ya extrajo el `req`). Ver .h.
 */
#include "bpvm_bmgr_wire.h"
#include "bpvm_boot.h"
#include "bpvm_pack.h"   /* H3 — PACK_LS sobre la zona de packs (bm->packs_base) */

#include <string.h>
#include <stdio.h>

/* --- string builder minúsculo (NUL-terminado; el llamador usa strlen o el ret) --- */
typedef struct { char* buf; size_t cap, off; int ok; } sb_t;

static void sb_init(sb_t* s, char* buf, size_t cap) { s->buf = buf; s->cap = cap; s->off = 0; s->ok = 1; if (cap) buf[0] = '\0'; }
static void sb_raw(sb_t* s, const char* str) {
    if (!s->ok) return;
    size_t n = strlen(str);
    if (s->off + n >= s->cap) { s->ok = 0; return; }
    memcpy(s->buf + s->off, str, n);
    s->off += n;
    s->buf[s->off] = '\0';
}
static void sb_long(sb_t* s, long v) { char t[24]; snprintf(t, sizeof t, "%ld", v); sb_raw(s, t); }
static void sb_esc(sb_t* s, const char* str) {   /* string JSON escapado, sin comillas */
    if (!s->ok) return;
    for (const char* p = str; *p; p++) {
        char c = *p;
        const char* rep = NULL; char u[8];
        switch (c) {
        case '"':  rep = "\\\""; break;
        case '\\': rep = "\\\\"; break;
        case '\n': rep = "\\n";  break;
        case '\r': rep = "\\r";  break;
        case '\t': rep = "\\t";  break;
        default:
            if ((unsigned char) c < 0x20) { snprintf(u, sizeof u, "\\u%04x", c); rep = u; }
            break;
        }
        if (rep) sb_raw(s, rep);
        else {
            if (s->off + 1 >= s->cap) { s->ok = 0; return; }
            s->buf[s->off++] = c;
            s->buf[s->off] = '\0';
        }
    }
}

int bpvm_bmgr_wire_state(const bpvm_bmgr_t* bm) {
    if (!bpvm_bmgr_env(bm, NULL)) return BPVM_BOOT_KERNEL;            /* sin env → suelo */
    bpvm_part_layout_t lay; int bad;
    if (bpvm_bmgr_part_layout(bm, bm->sector, &lay, &bad) != BPVM_PART_OK)
        return BPVM_BOOT_PARTITIONS;                                 /* env pero sin tamaños */
    return BPVM_BOOT_APP;                                            /* completo */
}

/* Construye un ERROR reply en `out`. Devuelve su longitud (siempre un reply válido). */
static int reply_error(char* out, size_t cap, long id, const char* code, const char* msg) {
    sb_t s; sb_init(&s, out, cap);
    sb_raw(&s, "{\"type\":\"ERROR\",\"id\":"); sb_long(&s, id);
    sb_raw(&s, ",\"code\":\""); sb_esc(&s, code);
    sb_raw(&s, "\",\"message\":\""); sb_esc(&s, msg); sb_raw(&s, "\"}");
    return s.ok ? (int) s.off : -1;
}

/* H3 — sesión de BURN (una a la vez: el wire de gestión es single-thread en
 * todos los llamadores; un BEGIN nuevo abandona la anterior sin daño porque
 * su magic nunca se escribió). static: fuera del stack de la comm task. */
static bpvm_pack_burn_t s_burn;

/* Mapea un error del núcleo de packs a (code, message) del wire. */
static int reply_pack_err(char* out, size_t cap, long id, int32_t e) {
    switch (e) {
    case BPVM_PACK_ERR_BADIMG:  return reply_error(out, cap, id, "BAD_PACK",      "pack invalido (magic/CRC/formato)");
    case BPVM_PACK_ERR_NOSPACE: return reply_error(out, cap, id, "NO_SPACE",      "no cabe en la zona de packs (o cadena corrupta)");
    case BPVM_PACK_ERR_ALIGN:   return reply_error(out, cap, id, "BAD_ALIGN",     "el tamano no es multiplo del bloque de borrado");
    case BPVM_PACK_ERR_STATE:   return reply_error(out, cap, id, "INVALID_STATE", "sesion de burn invalida (begin/data/end desordenados)");
    case BPVM_PACK_ERR_VERIFY:  return reply_error(out, cap, id, "VERIFY_FAIL",   "el CRC no cuadra al verificar en flash");
    case BPVM_PACK_ERR_IO:      return reply_error(out, cap, id, "FLASH_ERROR",   "fallo de erase/program en la flash");
    default:                    return reply_error(out, cap, id, "INTERNAL_ERROR", "error de packs desconocido");
    }
}

int bpvm_bmgr_wire_dispatch(bpvm_bmgr_t* bm, const bpvm_bmgr_req_t* req,
                            char* out, size_t cap, int* wrote_slot) {
    if (wrote_slot) *wrote_slot = -1;
    if (!bm || !req || !out) return -1;
    const long id = req->id;
    const char* type = req->type;
    sb_t s; sb_init(&s, out, cap);

    if (!strcmp(type, "STATE")) {
        /* Estado: el REAL del boot si el llamador lo provee (firmware, bm->live);
         * si no, el derivado del env (sim/host: plan completo equivale a APP). */
        int st             = bm->live ? (int) bm->live->state : bpvm_bmgr_wire_state(bm);
        int degraded       = bm->live ? bm->live->degraded : 0;
        const char* reason = bm->live ? bm->live->reason : "";
        sb_raw(&s, "{\"type\":\"STATE_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"state\":"); sb_long(&s, st);
        sb_raw(&s, ",\"name\":\""); sb_esc(&s, bpvm_boot_state_name((bpvm_boot_state_t) st));
        sb_raw(&s, degraded ? "\",\"degraded\":true,\"reason\":\""
                            : "\",\"degraded\":false,\"reason\":\"");
        sb_esc(&s, reason);
        sb_raw(&s, "\"}");
        return s.ok ? (int) s.off : -1;
    }
    if (!strcmp(type, "ENV_LS")) {
        int n = bpvm_bmgr_env_count(bm);
        sb_raw(&s, "{\"type\":\"ENV_LS_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"entries\":[");
        for (int i = 0; i < n; i++) {
            char k[64], val[192];
            if (!bpvm_bmgr_env_pair_at(bm, i, k, sizeof k, val, sizeof val)) continue;
            if (i) sb_raw(&s, ",");
            sb_raw(&s, "{\"key\":\""); sb_esc(&s, k);
            sb_raw(&s, "\",\"value\":\""); sb_esc(&s, val); sb_raw(&s, "\"}");
        }
        sb_raw(&s, "]}");
        return s.ok ? (int) s.off : reply_error(out, cap, id, "INTERNAL_ERROR", "reply no cabe");
    }
    if (!strcmp(type, "ENV_GET")) {
        if (!req->has_key) return reply_error(out, cap, id, "INVALID_PARAM", "falta key");
        char val[256];
        if (bpvm_bmgr_env_get(bm, req->key, val, sizeof val) < 0)
            return reply_error(out, cap, id, "NOT_FOUND", "clave no existe");
        sb_raw(&s, "{\"type\":\"ENV_GET_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"value\":\""); sb_esc(&s, val); sb_raw(&s, "\"}");
        return s.ok ? (int) s.off : -1;
    }
    if (!strcmp(type, "ENV_SET") || !strcmp(type, "ENV_DEL")) {
        if (!req->has_key) return reply_error(out, cap, id, "INVALID_PARAM", "falta key");
        int is_set = !strcmp(type, "ENV_SET");
        if (is_set && !req->has_value) return reply_error(out, cap, id, "INVALID_PARAM", "falta value");
        int slot = -1;
        if (bpvm_bmgr_env_set(bm, req->key, is_set ? req->value : NULL, &slot) != 0)
            return reply_error(out, cap, id, "NO_SPACE", "no cabe en el env");
        if (wrote_slot) *wrote_slot = slot;
        sb_raw(&s, "{\"type\":\""); sb_raw(&s, type); sb_raw(&s, "_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"slot\":"); sb_long(&s, slot); sb_raw(&s, "}");
        return s.ok ? (int) s.off : -1;
    }
    if (!strcmp(type, "PART_LS")) {
        bpvm_part_layout_t lay; int bad;
        bpvm_part_err_t e = bpvm_bmgr_part_layout(bm, bm->sector, &lay, &bad);
        sb_raw(&s, "{\"type\":\"PART_LS_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"base\":"); sb_long(&s, (long) bm->part_base);
        sb_raw(&s, ",\"usableFlash\":"); sb_long(&s, (long) bm->usable_flash);
        if (e == BPVM_PART_ERR_MISSING) {
            sb_raw(&s, ",\"missing\":true,\"parts\":[]}");
        } else {
            sb_raw(&s, ",\"missing\":false,\"parts\":[");
            for (int i = 0; i < BPVM_PART_COUNT; i++) {
                if (i) sb_raw(&s, ",");
                sb_raw(&s, "{\"name\":\""); sb_esc(&s, bpvm_part_name((bpvm_part_kind_t) i));
                sb_raw(&s, "\",\"offset\":"); sb_long(&s, (long) lay.parts[i].offset);
                sb_raw(&s, ",\"size\":"); sb_long(&s, (long) lay.parts[i].size); sb_raw(&s, "}");
            }
            sb_raw(&s, "]}");
        }
        return s.ok ? (int) s.off : reply_error(out, cap, id, "INTERNAL_ERROR", "reply no cabe");
    }
    if (!strcmp(type, "PACK_LS")) {
        /* H3 — LIST de la zona de packs. Lectura pura (scan de la cadena con
         * verificación de CRCs); las escrituras (BURN/DEL) llegan en su fase. */
        if (!bm->packs_base || bm->packs_size == 0)
            return reply_error(out, cap, id, "UNSUPPORTED", "sin zona de packs");
        /* static: ~1.4 KB que no queremos en el stack de la comm task del micro;
         * el wire de gestión es single-thread en todos los llamadores. */
        static bpvm_pack_info_t inf[16];
        enum { PACK_LS_MAX = (int) (sizeof inf / sizeof inf[0]) };
        uint32_t end = 0;
        int n = bpvm_pack_scan(bm->packs_base, bm->packs_size, inf, PACK_LS_MAX,
                               /*verify_content=*/1, &end);
        int chain_ok = (end != BPVM_PACK_NO_SPACE);
        sb_raw(&s, "{\"type\":\"PACK_LS_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"regionSize\":"); sb_long(&s, (long) bm->packs_size);
        sb_raw(&s, ",\"free\":"); sb_long(&s, chain_ok ? (long) (bm->packs_size - end) : 0L);
        sb_raw(&s, chain_ok ? ",\"chainOk\":true" : ",\"chainOk\":false");
        sb_raw(&s, ",\"count\":"); sb_long(&s, n);
        sb_raw(&s, ",\"packs\":[");
        for (int i = 0; i < n && i < PACK_LS_MAX; i++) {
            if (i) sb_raw(&s, ",");
            sb_raw(&s, "{\"name\":\""); sb_esc(&s, inf[i].nombre);
            sb_raw(&s, "\",\"version\":\""); sb_esc(&s, inf[i].vercont);
            sb_raw(&s, "\",\"date\":"); sb_long(&s, (long) inf[i].fecha);
            sb_raw(&s, ",\"size\":"); sb_long(&s, (long) inf[i].size_total);
            sb_raw(&s, ",\"files\":"); sb_long(&s, (long) inf[i].n_entries);
            sb_raw(&s, ",\"offset\":"); sb_long(&s, (long) inf[i].off);
            sb_raw(&s, inf[i].alive ? ",\"active\":true" : ",\"active\":false");
            sb_raw(&s, inf[i].crc_ok ? ",\"crcOk\":true}" : ",\"crcOk\":false}");
        }
        sb_raw(&s, "]}");
        return s.ok ? (int) s.off : reply_error(out, cap, id, "INTERNAL_ERROR", "reply no cabe");
    }
    if (!strcmp(type, "PACK_ENTRIES")) {
        /* H3 — ficheros DENTRO del pack en `offset` (lo da el PACK_LS previo).
         * Lectura pura; el panel del IDE lo pide al seleccionar un pack. */
        if (!bm->packs_base || bm->packs_size == 0)
            return reply_error(out, cap, id, "UNSUPPORTED", "sin zona de packs");
        if (!req->has_off || req->off < 0)
            return reply_error(out, cap, id, "INVALID_PARAM", "falta offset");
        static bpvm_pack_entry_t es[32];   /* static: fuera del stack de la comm task */
        enum { PACK_ENT_MAX = (int) (sizeof es / sizeof es[0]) };
        int n = bpvm_pack_entries(bm->packs_base, bm->packs_size,
                                  (uint32_t) req->off, es, PACK_ENT_MAX);
        if (n < 0)
            return reply_error(out, cap, id, "NOT_FOUND", "ahi no hay un pack valido");
        sb_raw(&s, "{\"type\":\"PACK_ENTRIES_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"offset\":"); sb_long(&s, req->off);
        sb_raw(&s, ",\"count\":"); sb_long(&s, n);
        sb_raw(&s, ",\"entries\":[");
        for (int i = 0; i < n && i < PACK_ENT_MAX; i++) {
            if (i) sb_raw(&s, ",");
            sb_raw(&s, "{\"tipo\":\""); sb_esc(&s, es[i].tipo);
            sb_raw(&s, "\",\"nombre\":\""); sb_esc(&s, es[i].nombre);
            sb_raw(&s, "\",\"size\":"); sb_long(&s, (long) es[i].len); sb_raw(&s, "}");
        }
        sb_raw(&s, "]}");
        return s.ok ? (int) s.off : reply_error(out, cap, id, "INTERNAL_ERROR", "reply no cabe");
    }
    if (!strcmp(type, "PACK_BURN_BEGIN")) {
        /* H3 — grabar un pack por CHUNKS (RAM constante; ver bpvm_pack.h §BURN).
         * BEGIN valida+borra y abre sesión; DATA graba según llega; END verifica
         * en flash y ACTIVA (magic al final). */
        if (!bm->packs_base || bm->packs_size == 0 || !bm->packs_flash)
            return reply_error(out, cap, id, "UNSUPPORTED", "sin zona de packs escribible");
        if (!req->has_size || req->size <= 0)
            return reply_error(out, cap, id, "INVALID_PARAM", "falta size");
        int32_t r = bpvm_pack_burn_begin(bm->packs_base, bm->packs_size, bm->packs_flash,
                                         (uint32_t) req->size, &s_burn);
        if (r < 0) return reply_pack_err(out, cap, id, r);
        sb_raw(&s, "{\"type\":\"PACK_BURN_BEGIN_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"offset\":"); sb_long(&s, r);
        sb_raw(&s, ",\"chunkMax\":"); sb_long(&s, BPVM_PACK_BURN_CHUNK); sb_raw(&s, "}");
        return s.ok ? (int) s.off : -1;
    }
    if (!strcmp(type, "PACK_BURN_DATA")) {
        if (!bm->packs_flash)
            return reply_error(out, cap, id, "UNSUPPORTED", "sin zona de packs escribible");
        if (!req->bulk || req->bulk_len <= 0 || req->bulk_len > BPVM_PACK_BURN_CHUNK)
            return reply_error(out, cap, id, "INVALID_PARAM", "falta bulk (o chunk demasiado grande)");
        int r = bpvm_pack_burn_data(&s_burn, bm->packs_flash,
                                    req->bulk, (uint32_t) req->bulk_len);
        if (r < 0) return reply_pack_err(out, cap, id, r);
        sb_raw(&s, "{\"type\":\"PACK_BURN_DATA_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"received\":"); sb_long(&s, (long) s_burn.received); sb_raw(&s, "}");
        return s.ok ? (int) s.off : -1;
    }
    if (!strcmp(type, "PACK_BURN_END")) {
        if (!bm->packs_flash)
            return reply_error(out, cap, id, "UNSUPPORTED", "sin zona de packs escribible");
        int32_t r = bpvm_pack_burn_end(bm->packs_base, bm->packs_size, &s_burn, bm->packs_flash);
        if (r < 0) return reply_pack_err(out, cap, id, r);
        sb_raw(&s, "{\"type\":\"PACK_BURN_END_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"offset\":"); sb_long(&s, r); sb_raw(&s, "}");
        return s.ok ? (int) s.off : -1;
    }
    if (!strcmp(type, "PART_DEFAULTS")) {
        uint32_t sizes[BPVM_PART_COUNT];
        bpvm_bmgr_part_defaults(bm, bm->sector, sizes);
        sb_raw(&s, "{\"type\":\"PART_DEFAULTS_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"parts\":[");
        for (int i = 0; i < BPVM_PART_COUNT; i++) {
            if (i) sb_raw(&s, ",");
            sb_raw(&s, "{\"name\":\""); sb_esc(&s, bpvm_part_name((bpvm_part_kind_t) i));
            sb_raw(&s, "\",\"size\":"); sb_long(&s, (long) sizes[i]); sb_raw(&s, "}");
        }
        sb_raw(&s, "]}");
        return s.ok ? (int) s.off : -1;
    }
    if (!strcmp(type, "PART_APPLY")) {
        /* Solo las KNOBS son obligatorias (hoy FS); la última (packs) es EL RESTO,
         * derivada → su valor es opcional y se ignora (el cliente puede mandarlo o no). */
        uint32_t sizes[BPVM_PART_COUNT];
        for (int i = 0; i < BPVM_PART_COUNT; i++) {
            if (i < BPVM_PART_COUNT - 1 && req->part_sizes[i] < 0)
                return reply_error(out, cap, id, "INVALID_PARAM", "falta el tamano del FS");
            sizes[i] = (req->part_sizes[i] >= 0) ? (uint32_t) req->part_sizes[i] : 0u;
        }
        int bad = -1, slot = -1;
        bpvm_part_err_t e = bpvm_bmgr_part_apply(bm, bm->sector, sizes, &bad, &slot);
        if (e != BPVM_PART_OK) {
            char m[128];
            snprintf(m, sizeof m, "%s (particion %d: %s)", bpvm_part_err_str(e), bad,
                     (bad >= 0 && bad < BPVM_PART_COUNT) ? bpvm_part_name((bpvm_part_kind_t) bad) : "?");
            return reply_error(out, cap, id, "INVALID_PARAM", m);
        }
        if (wrote_slot) *wrote_slot = slot;
        sb_raw(&s, "{\"type\":\"PART_APPLY_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"slot\":"); sb_long(&s, slot); sb_raw(&s, "}");
        return s.ok ? (int) s.off : -1;
    }

    return reply_error(out, cap, id, "UNSUPPORTED", "comando de gestion no soportado");
}
