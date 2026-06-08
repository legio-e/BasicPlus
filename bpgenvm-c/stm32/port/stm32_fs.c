/*
 * stm32_fs.c — filesystem en RAM (mini). Arena lineal + tabla de entradas.
 * Borrar compacta el arena (reclama espacio) → re-subir no crece sin fin.
 */
#include "stm32_fs.h"

#include <string.h>

#define FS_MAX_FILES   40   /* 13 stdlib core en /lib + módulos de app + holgura */
#define FS_NAME_MAX    64
#define FS_ARENA_SIZE  (96u * 1024u)

typedef struct {
    char     name[FS_NAME_MAX];
    uint32_t off;       /* offset dentro del arena */
    uint32_t size;
    int      used;
} fs_entry_t;

static uint8_t    s_arena[FS_ARENA_SIZE];
static uint32_t   s_arena_used = 0;
static fs_entry_t s_files[FS_MAX_FILES];

static fs_entry_t* find(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_files[i].used && strcmp(s_files[i].name, name) == 0) {
            return &s_files[i];
        }
    }
    return NULL;
}

int fs_del(const char* name) {
    fs_entry_t* e = find(name);
    if (!e) return -1;
    uint32_t hole_off = e->off;
    uint32_t hole_len = e->size;
    /* Compacta: mueve todo lo que hay detrás del hueco hacia delante. */
    uint32_t tail = s_arena_used - (hole_off + hole_len);
    if (tail > 0) {
        memmove(&s_arena[hole_off], &s_arena[hole_off + hole_len], tail);
    }
    s_arena_used -= hole_len;
    /* Reajusta offsets de las entradas que estaban detrás del hueco. */
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_files[i].used && s_files[i].off > hole_off) {
            s_files[i].off -= hole_len;
        }
    }
    e->used = 0;
    return 0;
}

int fs_put(const char* name, const uint8_t* data, uint32_t size) {
    if (!name || name[0] == '\0' || strlen(name) >= FS_NAME_MAX) return -1;
    /* Sobreescritura: borra el viejo (compacta) y vuelve a añadir al final. */
    fs_del(name);
    if (s_arena_used + size > FS_ARENA_SIZE) return -1;   /* sin espacio */
    /* Busca slot libre. */
    fs_entry_t* slot = NULL;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!s_files[i].used) { slot = &s_files[i]; break; }
    }
    if (!slot) return -1;   /* demasiados ficheros */

    slot->off  = s_arena_used;
    slot->size = size;
    strncpy(slot->name, name, FS_NAME_MAX - 1);
    slot->name[FS_NAME_MAX - 1] = '\0';
    slot->used = 1;
    if (size > 0 && data) memcpy(&s_arena[slot->off], data, size);
    s_arena_used += size;
    return 0;
}

int fs_get(const char* name, const uint8_t** data, uint32_t* size) {
    fs_entry_t* e = find(name);
    if (!e) return -1;
    if (data) *data = &s_arena[e->off];
    if (size) *size = e->size;
    return 0;
}

int fs_count(void) {
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) if (s_files[i].used) n++;
    return n;
}

int fs_entry(int i, const char** name, uint32_t* size) {
    int seen = 0;
    for (int k = 0; k < FS_MAX_FILES; k++) {
        if (!s_files[k].used) continue;
        if (seen == i) {
            if (name) *name = s_files[k].name;
            if (size) *size = s_files[k].size;
            return 0;
        }
        seen++;
    }
    return -1;
}

uint32_t fs_total_bytes(void) { return FS_ARENA_SIZE; }
uint32_t fs_used_bytes(void)  { return s_arena_used; }

void fs_format(void) {
    s_arena_used = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) s_files[i].used = 0;
}
