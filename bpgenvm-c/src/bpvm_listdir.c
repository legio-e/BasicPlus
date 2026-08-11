/*
 * bpvm_listdir.c — V5/H6 paso 3: el núcleo de LIST_DIR, común a las familias.
 *
 * Traslado literal de lo que hacía `pico/repl_v1.c` desde V5/H2, con la única
 * diferencia de que la salida va por un sumidero en vez de a `stdout`. Los
 * motivos de diseño (dos tiempos, sin CRC, sin recursión) están en el .h.
 */
#include "bpvm_listdir.h"
#include "bpvm_fs.h"
#include "bpvm.h"        /* bpvm_scratch_take / give */
#include "bpvm_log.h"

#include <stdio.h>
#include <string.h>

/* Los topes. Son los de siempre; cambiarlos es cambiar la RAM de la foto, que
 * sale de la zona de rascar (~9 KB) y no de la pila. */
#define LISTDIR_MAX_ENTRIES  96
#define LISTDIR_NAME_MAX     64

typedef struct {
    char     nombres[LISTDIR_MAX_ENTRIES][LISTDIR_NAME_MAX];
    uint32_t tam[LISTDIR_MAX_ENTRIES];
    uint8_t  esdir[LISTDIR_MAX_ENTRIES];
    int      n;
    int      omitidas;      /* las que NO cupieron — jamás en silencio */
} listdir_foto_t;

static void listdir_cb(const char* name, int is_dir, uint32_t size, void* user) {
    listdir_foto_t* f = (listdir_foto_t*) user;
    if (f->n >= LISTDIR_MAX_ENTRIES) { f->omitidas++; return; }
    snprintf(f->nombres[f->n], LISTDIR_NAME_MAX, "%s", name);
    f->tam[f->n]   = size;
    f->esdir[f->n] = (uint8_t) (is_dir ? 1 : 0);
    f->n++;
}

/* Un trozo por el sumidero. Se usa siempre con literales o con un buffer de
 * pila pequeño: nada de montar la respuesta entera en memoria. */
static void mete(bpvm_txt_sink_t sink, void* user, const char* s) {
    sink(s, strlen(s), user);
}

bpvm_listdir_res_t bpvm_listdir_emitir(const char* path, long id,
                                       bpvm_txt_sink_t sink, void* user,
                                       int* omitidas)
{
    if (omitidas) *omitidas = 0;
    if (!sink) return BPVM_LISTDIR_NO_LISTA;

    const char* p = (path && path[0]) ? path : "/";

    listdir_foto_t* f = (listdir_foto_t*) bpvm_scratch_take(sizeof(*f), "LIST_DIR");
    if (!f) return BPVM_LISTDIR_OCUPADO;

    f->n = 0; f->omitidas = 0;
    int r = bpvm_fs_list(p, listdir_cb, f);
    if (r != 0 && f->n == 0) {
        bpvm_scratch_give("LIST_DIR");
        return BPVM_LISTDIR_NO_LISTA;
    }
    if (f->omitidas) {
        log_printf("fs: LIST_DIR '%s' INCOMPLETO — %d entradas fuera",
                   p, f->omitidas);
        log_flush();
    }

    /* Y ahora sí, FUERA del cerrojo del FS, a escupirlo. */
    char cab[48];
    snprintf(cab, sizeof cab, "{\"type\":\"LIST_DIR_REPLY\",\"id\":%ld,\"entries\":[", id);
    mete(sink, user, cab);

    for (int i = 0; i < f->n; i++) {
        if (i) mete(sink, user, ",");
        mete(sink, user, "{\"name\":\"");
        /* El escapado va por trozos para no necesitar un buffer del tamaño del
         * nombre ya escapado. Sólo hay dos caracteres que escapar en un nombre
         * de fichero dentro de JSON. */
        const char* ini = f->nombres[i];
        for (const char* q = f->nombres[i]; *q; q++) {
            if (*q == '"' || *q == '\\') {
                if (q > ini) sink(ini, (size_t) (q - ini), user);
                mete(sink, user, "\\");
                ini = q;
            }
        }
        if (*ini) mete(sink, user, ini);

        char cola[64];
        snprintf(cola, sizeof cola, "\",\"size\":%u,\"isDir\":%s}",
                 (unsigned) f->tam[i], f->esdir[i] ? "true" : "false");
        mete(sink, user, cola);
    }

    char fin[32];
    snprintf(fin, sizeof fin, "],\"omitidas\":%d}", f->omitidas);
    mete(sink, user, fin);

    if (omitidas) *omitidas = f->omitidas;
    bpvm_scratch_give("LIST_DIR");
    return BPVM_LISTDIR_OK;
}
