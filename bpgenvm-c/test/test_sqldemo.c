/*
 * test_sqldemo.c — V5/H4: el CICLO ENTERO, en el PC.
 *
 * Hasta aquí cada pieza estaba probada por su lado: el shim mueve SQLite (H/),
 * el VFS lee y escribe ficheros (vfstest), el puente resuelve por nombre y no
 * lleva relocs (test_packhelp + objdump), y `SQLite.bp` compila. Lo que NO
 * estaba era todo junto — y mientras no lo esté, «funciona» es una suma de
 * promesas.
 *
 * Esto lo junta:
 *
 *     SqlDemo.bp  (BP)
 *        -> SQLite.bp          la librería: Db, Query, exec*, query
 *        -> puente AOT          aot_SQLite.c, generado por el compilador
 *        -> pack 'SQLI'         sqlite_shim.c: handles + los 16 símbolos
 *        -> SQLite              la amalgama, tal cual viene
 *        -> VFS 'bp'            vfs_bp.c, sobre las ranuras de fichero del BIOS
 *        -> BIOS de host        packglue.c, sobre stdio
 *
 * DIFERENCIAS CON LA PLACA, dichas para que nadie se confunde con lo que esto
 * demuestra y lo que no:
 *
 *  · Aquí el puente va ENLAZADO en el binario. En la placa iría como `.mdn`, y
 *    ese camino todavía tiene el agujero de `.rodata` (tarea #382).
 *  · La tabla BIOS es el doble de host (stdio). En la placa es la de verdad,
 *    sobre littlefs o sobre la SD.
 *  · El pack está enlazado, no grabado en flash. En la placa se graba y se
 *    encuentra por su marca.
 *
 * O sea: esto demuestra que **el modelo entero encaja y que el SQL sale**. No
 * demuestra la placa.
 */
#include "bpvm.h"
#include "bpvm_fs.h"
#include "bpvm_bios.h"
#include "packglue.h"
#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>

extern void aot_SQLite_register(struct bpvm* vm);   /* generado por AotMain */
extern int  bpsql_publicar(const bpvm_bios_t* bios);/* sqlite_shim.c        */

/* La arena de SQLite. En la placa sale de la tabla BIOS (el firmware la
 * reserva); aquí es un bloque nuestro, pero el CAMINO es el mismo:
 * SQLITE_CONFIG_HEAP, o sea que SQLite no llama a malloc ni una vez. */
#define ARENA_BYTES (2u * 1024u * 1024u)
static unsigned char s_arena_cruda[ARENA_BYTES + 16];

int main(int argc, char** argv)
{
    const char* mod_path = (argc > 1) ? argv[1] : "SqlDemo.mod";
    unsigned char* arena;
    size_t mem_size = 4u * 1024u * 1024u;
    uint8_t* mem;
    bpvm_t* vm;
    bpvm_status_t s;
    int rc;

    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);

    /* ── 1. El pack se prepara y PUBLICA su tabla ──────────────────────────
     * En la placa esto lo hace el propio pack al ser cargado. */
    arena = (unsigned char*) (((uintptr_t) s_arena_cruda + 7u) & ~(uintptr_t) 7u);
    if (sqlite3_config(SQLITE_CONFIG_HEAP, arena, (int) ARENA_BYTES, 64) != SQLITE_OK) {
        fprintf(stderr, "no se pudo dar la arena a SQLite\n"); return 1;
    }
    if (sqlite3_config(SQLITE_CONFIG_PMASZ, (unsigned) 64) != SQLITE_OK) {
        fprintf(stderr, "no se pudo fijar PMASZ\n"); return 1;
    }
    /* OJO AL ORDEN: `initialize` EXIGE que haya un vfs registrado, y falla en
     * silencio si no lo hay (lección del 8-ago). El vfs 'bp' se registra desde
     * dentro de `bpsql_publicar`, que por eso va ANTES. */
    rc = bpsql_publicar(packglue_bios());
    if (rc != 0) { fprintf(stderr, "el pack no pudo publicar: %d\n", rc); return 1; }
    packglue_callar(1);          /* el log del BIOS taparía la salida del programa */

    /* ── 2. La VM ─────────────────────────────────────────────────────────── */
    mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }
    vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init fallo\n"); free(mem); return 1; }

    s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    /* ── 3. El puente: las 16 `native` de SQLite.bp pasan a ir por el pack ── */
    aot_SQLite_register(vm);

    s = bpvm_run(vm);
    fprintf(stderr, "\n[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
