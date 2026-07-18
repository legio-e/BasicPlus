/*
 * bpvm_part.c — H9: tabla de particiones sobre el env (estado 1). Solo lógica,
 * host-testable, sin heap; la cintura de flash por-micro va aparte. Subsume el
 * bp_ptable_t binario de fs_lfs_pico.c (B2.b) e incluye el clamp de #292 y la
 * validación del asistente. Ver bpvm_part.h.
 */
#include "bpvm_part.h"

#include <string.h>
#include <stdio.h>   /* snprintf (solo en to_payload, ruta no-caliente) */

/* nombre → kind (abierto: desconocido = APP). */
static bpvm_part_kind_t name_to_kind(const char* name) {
    if (!strcmp(name, "fs"))    return BPVM_PART_FS;
    if (!strcmp(name, "packs")) return BPVM_PART_PACKS;
    return BPVM_PART_APP;
}

/* Construye "part.<name>.<field>" en `key` (cap suficiente por el llamador). */
static void build_key(char* key, size_t cap, const char* name, const char* field) {
    /* part. + name + . + field + NUL — sin snprintf para mantenerlo minúsculo. */
    size_t i = 0;
    const char* pre = "part.";
    while (*pre && i + 1 < cap) key[i++] = *pre++;
    while (*name && i + 1 < cap) key[i++] = *name++;
    if (i + 1 < cap) key[i++] = '.';
    while (*field && i + 1 < cap) key[i++] = *field++;
    key[i] = '\0';
}

int bpvm_part_parse(const bpvm_env_t* env, bpvm_part_table_t* out) {
    if (out) out->count = 0;
    if (!env || !env->valid || !out) return -1;

    char list[128];
    int n = bpvm_env_get(env, "partitions", list, sizeof list);
    if (n <= 0) return 0;                       /* sin tabla de particiones */

    char* p = list;
    while (*p && out->count < (int) BPVM_PART_MAX) {
        char* c = p;                            /* fin del nombre (coma o fin) */
        while (*c && *c != ',') c++;
        size_t nl = (size_t)(c - p);
        if (nl > 0 && nl < BPVM_PART_NAME_MAX) {
            bpvm_part_t* pt = &out->parts[out->count];
            memcpy(pt->name, p, nl);
            pt->name[nl] = '\0';

            char key[8 + BPVM_PART_NAME_MAX];
            build_key(key, sizeof key, pt->name, "offset");
            long off = bpvm_env_get_long(env, key, -1);
            build_key(key, sizeof key, pt->name, "size");
            long sz  = bpvm_env_get_long(env, key, -1);

            if (off >= 0 && sz >= 0) {           /* entrada completa → la tomamos */
                pt->offset = (uint32_t) off;
                pt->size   = (uint32_t) sz;
                pt->kind   = name_to_kind(pt->name);
                out->count++;
            }
            /* si falta offset/size, se ignora (tolerante) */
        }
        p = (*c == ',') ? c + 1 : c;
    }
    return out->count;
}

const bpvm_part_t* bpvm_part_find(const bpvm_part_table_t* t, bpvm_part_kind_t kind) {
    if (!t) return NULL;
    for (int i = 0; i < t->count; i++)
        if (t->parts[i].kind == kind) return &t->parts[i];
    return NULL;
}

const bpvm_part_t* bpvm_part_find_name(const bpvm_part_table_t* t, const char* name) {
    if (!t || !name) return NULL;
    for (int i = 0; i < t->count; i++)
        if (!strcmp(t->parts[i].name, name)) return &t->parts[i];
    return NULL;
}

uint32_t bpvm_part_usable_flash(uint32_t flash_bytes, uint32_t image_max) {
    if (image_max > 0 && flash_bytes > image_max) return image_max;   /* clamp #292 */
    return flash_bytes;
}

bpvm_part_err_t bpvm_part_validate(const bpvm_part_table_t* t, uint32_t usable_flash,
                                   uint32_t reserved_end, uint32_t sector, int* bad_idx) {
    if (bad_idx) *bad_idx = -1;
    if (!t) return BPVM_PART_OK;
    for (int i = 0; i < t->count; i++) {
        const bpvm_part_t* a = &t->parts[i];
        if (a->size == 0) {
            if (bad_idx) *bad_idx = i;
            return BPVM_PART_ERR_EMPTY;
        }
        if (sector != 0 && ((a->offset % sector) != 0 || (a->size % sector) != 0)) {
            if (bad_idx) *bad_idx = i;
            return BPVM_PART_ERR_UNALIGNED;
        }
        if (a->offset < reserved_end) {
            if (bad_idx) *bad_idx = i;
            return BPVM_PART_ERR_BELOW_RESERVED;
        }
        /* fin = offset+size, cuidado con el desbordamiento de u32 */
        if (a->size > usable_flash || a->offset > usable_flash - a->size) {
            if (bad_idx) *bad_idx = i;
            return BPVM_PART_ERR_OUT_OF_FLASH;
        }
        for (int j = i + 1; j < t->count; j++) {
            const bpvm_part_t* b = &t->parts[j];
            /* solapan si a.start < b.end && b.start < a.end */
            if (a->offset < b->offset + b->size && b->offset < a->offset + a->size) {
                if (bad_idx) *bad_idx = j;
                return BPVM_PART_ERR_OVERLAP;
            }
        }
    }
    return BPVM_PART_OK;
}

const char* bpvm_part_err_str(bpvm_part_err_t e) {
    switch (e) {
    case BPVM_PART_OK:               return "ok";
    case BPVM_PART_ERR_EMPTY:        return "partición vacía (size 0)";
    case BPVM_PART_ERR_UNALIGNED:    return "offset/size no alineados al sector de borrado";
    case BPVM_PART_ERR_BELOW_RESERVED:return "pisa la zona reservada (imagen + env)";
    case BPVM_PART_ERR_OUT_OF_FLASH: return "se sale de la flash usable";
    case BPVM_PART_ERR_OVERLAP:      return "dos particiones se solapan";
    default:                         return "error desconocido";
    }
}

int bpvm_part_to_payload(const bpvm_part_table_t* t, char* buf, size_t cap) {
    if (!t || !buf || cap == 0) return -1;
    size_t off = 0;
    int w;
    /* partitions=name1,name2,... */
    w = snprintf(buf + off, cap - off, "partitions=");
    if (w < 0 || (size_t) w >= cap - off) return -1;
    off += (size_t) w;
    for (int i = 0; i < t->count; i++) {
        w = snprintf(buf + off, cap - off, "%s%s", (i ? "," : ""), t->parts[i].name);
        if (w < 0 || (size_t) w >= cap - off) return -1;
        off += (size_t) w;
    }
    w = snprintf(buf + off, cap - off, "\n");
    if (w < 0 || (size_t) w >= cap - off) return -1;
    off += (size_t) w;
    /* part.<name>.offset / .size por cada una */
    for (int i = 0; i < t->count; i++) {
        w = snprintf(buf + off, cap - off, "part.%s.offset=%lu\npart.%s.size=%lu\n",
                     t->parts[i].name, (unsigned long) t->parts[i].offset,
                     t->parts[i].name, (unsigned long) t->parts[i].size);
        if (w < 0 || (size_t) w >= cap - off) return -1;
        off += (size_t) w;
    }
    return (int) off;
}
