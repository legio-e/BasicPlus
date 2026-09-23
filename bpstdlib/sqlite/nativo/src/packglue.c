/*
 * packglue.c — la tabla BIOS de HOST: el doble del firmware, sobre la libc.
 * ============================================================================
 *
 * QUE ES ESTO. El pack de SQLite no llama a `malloc` ni a `fopen`: todo lo que
 * depende del hierro se lo presta el firmware en una tabla de ranuras (40 desde
 * la v6, `bpvm_bios_t`) cuando lo carga. Eso es lo que permite que el MISMO binario
 * corra en la Metro y en el P4 sin recompilar. En el PC no hay firmware, asi
 * que aqui esta la misma tabla implementada sobre la libreria estandar — y con
 * ella `make test-sqldemo` ejecuta en el PC exactamente el SQLite que
 * lleva la placa.
 *
 * ── DE LAS 40 RANURAS, SOLO SEIS SON NUESTRAS ──────────────────────────────
 *
 * Las otras 34 las rellena `BPVM_BIOS_TABLA` (en `bpvm_bios.h`, pegada a la
 * struct) con las portables: las de `string.h`, las de fichero de
 * `src/bpvm_bios_fs.c`, la HAL y —desde V7/H6.c G1— la ARENA, que con varios
 * motores tiene dueño y es común (`bpvm_bios_arena`). Escribirlas a mano seria
 * una SEGUNDA COPIA del orden de la tabla, y ese orden cruza una frontera
 * binaria — el error de #299/#315 otra vez. Aqui solo van la voz, las tres de
 * memoria, el tiempo y el ENV, igual que en `bios_pico.c` y `bios_esp32.c`.
 *
 * ── ⚠️ ESTO ES UN DOBLE, Y UN DOBLE MAS AMABLE QUE EL ORIGINAL ES UNA TRAMPA ─
 *
 * De ahi la decision que mas se nota al leer el codigo: `malloc`, `free` y
 * `realloc` NO FUNCIONAN, igual que en la Pico. Gritan en el log y devuelven
 * NULL. Podrian funcionar perfectamente —estamos en un PC, ahi esta la libc—
 * y precisamente por eso no deben: en la placa el pack tiene PROHIBIDO tocar el
 * heap de la plataforma (para eso existe la arena separada), asi que si aqui se
 * lo dejaramos hacer, un pack que llamara a `malloc` pasaria en el PC y moriria
 * en el micro. El doble sirve de oraculo solo mientras dice que NO a lo mismo
 * que el original.
 *
 * Lo que si funciona de verdad, porque en la placa tambien: el log, el reloj y
 * las ranuras de fichero (sobre `stdio`, via `bpvm_bios_fs.c`). La arena no
 * pasa por aqui: `test_sqldemo.c` le da el heap a SQLite directamente.
 *
 * REESCRITO el 19-ago-2026 desde `bios_pico.c`. Ver `packglue.h`.
 */
#include "packglue.h"
#include "bpvm_env.h"     /* V7/H6.c G1: la regla de env_get */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── 1. La voz del pack ─────────────────────────────────────────────────────
 * A stderr y no a stdout: el programa de prueba compara su SALIDA, y mezclar
 * las dos cosas convertiria cualquier diff en ruido. */
static int s_callado = 0;

void packglue_callar(int callar) { s_callado = callar ? 1 : 0; }

static void glue_log(const char* msg) {
    if (s_callado) return;
    fprintf(stderr, "pack: %s\n", msg ? msg : "(null)");
}

/* ── 2. Las tres de memoria: STUBS QUE GRITAN ───────────────────────────────
 * No es que falten por hacer: es que tienen que fallar. Ver la cabecera. */
static void* glue_malloc(size_t n) {
    fprintf(stderr, "BIOS(host): el pack llamo a malloc(%lu) — PROHIBIDO,"
                    " tiene su arena -> NULL\n", (unsigned long) n);
    return NULL;
}

static void glue_free(void* p) {
    if (p) fprintf(stderr, "BIOS(host): el pack llamo a free(%p) sin arena\n", p);
}

static void* glue_realloc(void* p, size_t n) {
    fprintf(stderr, "BIOS(host): el pack llamo a realloc(%p,%lu) — PROHIBIDO,"
                    " tiene su arena -> NULL\n", p, (unsigned long) n);
    return NULL;
}

/* ── 3. El tiempo ───────────────────────────────────────────────────────────
 * Aqui SI hay reloj, al contrario que en la Pico (que devuelve NULL porque no
 * tiene RTC conectado). El parametro llega como `const void*` para que la tabla
 * no arrastre <time.h> a todos sus clientes. */
static struct tm* glue_localtime(const void* t) {
    if (t == NULL) return NULL;
    return localtime((const time_t*) t);
}

/* ── 4. El ENV (v6, V7/H6.c G1) ─────────────────────────────────────────────
 * En el PC no hay ENV de placa: ninguna clave está. Con la MISMA regla que la
 * placa (`bpvm_env_get_exacto`, que también juzga los argumentos): un doble que
 * aceptara lo que la placa rechaza sería el doble amable de la cabecera.
 *
 * (Aquí vivía una arena estática de 4 MB. Desde G1 la ranura `arena` la llena
 * la común —`bpvm_bios_arena`, con dueño—, y este doble nunca la prestaba de
 * verdad: `test_sqldemo.c` le da el heap a SQLite directamente.) */
static int glue_env_get(const char* clave, char* buf, size_t cap) {
    return bpvm_env_get_exacto(NULL, clave, buf, cap);
}

/* ── 5. La tabla ────────────────────────────────────────────────────────────
 * Estatica: su direccion no cambia y se le puede prestar al pack sin que nadie
 * tenga que mantenerla viva. */
static const bpvm_bios_t s_bios = BPVM_BIOS_TABLA(
    glue_log,
    glue_malloc, glue_free, glue_realloc,
    glue_localtime,
    glue_env_get);

const bpvm_bios_t* packglue_bios(void) { return &s_bios; }

/* ── 6. Los dos ganchos que SQLite exige con SQLITE_OS_OTHER=1 ──────────────
 *
 * Compilado asi, SQLite no trae capa de sistema: la pone entera quien lo
 * integra, y la puerta es `sqlite3_os_init()`. `sqlite3_initialize()` lo llama
 * por dentro, y ahi es donde se registra el VFS.
 *
 * ⚠️ Y NO ES OPCIONAL, aunque el programa no vaya a abrir ningun fichero:
 * `sqlite3_initialize()` FALLA con SQLITE_ERROR si no hay ningun vfs
 * registrado —`sqlite3MemdbInit()` hace `sqlite3_vfs_find(0)` y se rinde—, y
 * va envuelto en `NEVER()`, asi que ni siquiera lo loguea. Costo tres
 * grabaciones descubrirlo (8-ago); esta escrito en `vfs_bp.c`.
 *
 * En la placa esto mismo lo hace `sqlite_pack.c`, que tiene la tabla en un
 * global suyo (`g_bios`). Aqui la tabla es la de este fichero.
 */
extern int bpvfs_instalar(const bpvm_bios_t* bios);   /* vfs_bp.c */

int sqlite3_os_init(void) { return bpvfs_instalar(packglue_bios()); }
int sqlite3_os_end (void) { return 0; }               /* 0 = SQLITE_OK */
