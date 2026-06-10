/*
 * fs.h — filesystem mínimo en RAM con persistencia a flash.
 *
 * Modelo:
 *  - N slots de fichero, cada uno con nombre (<= FS_NAME_LEN) y datos
 *    binarios. Los .mod típicos son pocos KB, cabe holgado.
 *  - Espacio total FS_DATA_SIZE: 128 KB de datos. Suficiente para
 *    docenas de .mod (típicos 2-5 KB cada uno).
 *  - Los ficheros viven en RAM mientras la Pico esté encendida.
 *    fs_save_to_flash() los persiste en una región reservada de flash.
 *    fs_init() carga lo que haya en flash al arrancar.
 *
 *  Limitación de esta arquitectura: el FS_DATA_SIZE entero vive en
 *  SRAM (mirror del contenido en flash). RP2350 tiene 520 KB SRAM,
 *  así que ~128-256 KB es el techo razonable manteniendo el mirror.
 *  Para FS más grande (varios MB) habría que rediseñar leyendo datos
 *  directamente de XIP y no mantener mirror — pendiente.
 *
 * Layout de la región flash (al final de los 4 MB):
 *    offset 0:    magic ('BPFV') + version + count + entry[N]
 *    offset 1K:   datos concatenados
 *
 * Operaciones de escritura/borrado afectan a RAM. Se persisten en
 * flash al llamar fs_save_to_flash(), que erase+rewrite toda la
 * región. Es lento (~50 ms) pero correcto y simple.
 */
#ifndef BPVM_PICO_FS_H
#define BPVM_PICO_FS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FS_NAME_LEN     40         /* incluyendo el NUL */
#define FS_MAX_FILES    64                       /* antes 32, sube poco BSS (entry=48B) */
#define FS_DATA_SIZE    (128 * 1024)             /* antes 64 KB */

typedef enum {
    FS_OK = 0,
    FS_ERR_NOT_FOUND   = -1,
    FS_ERR_EXISTS      = -2,
    FS_ERR_NO_SPACE    = -3,
    FS_ERR_NAME_TOO_LONG = -4,
    FS_ERR_TOO_BIG     = -5,
    FS_ERR_TABLE_FULL  = -6,
    FS_ERR_BAD_FLASH   = -7,
    FS_ERR_INVALID     = -8
} fs_status_t;

/* Inicializa el FS. Intenta cargar desde flash si hay magic válido;
 * si no, deja el FS vacío. Devuelve FS_OK en ambos casos.
 *
 * Llamar UNA vez antes de cualquier otra operación. No es seguro
 * llamarlo concurrentemente con otras operaciones del FS. */
fs_status_t fs_init(void);

/* Vacía el FS en RAM (no toca flash hasta el siguiente save). */
void fs_format_ram(void);

/* Persiste el estado actual a flash. Bloquea ~50 ms (erase+program).
 * Durante la operación se suspenden los IRQs. */
fs_status_t fs_save_to_flash(void);

/* Itera los ficheros. `cb` recibe (name, size, user) por cada uno.
 * Si cb devuelve != 0 la iteración se aborta. */
typedef int (*fs_list_cb_t)(const char* name, uint32_t size, void* user);
int fs_list(fs_list_cb_t cb, void* user);

/* Lookup. Si existe, devuelve puntero a los bytes en RAM y rellena
 * *size_out. El puntero es válido hasta el siguiente fs_put/fs_delete. */
fs_status_t fs_get(const char* name, const uint8_t** data_out, uint32_t* size_out);

/* Inserta o sobreescribe. Si ya existe, libera el slot antiguo. */
fs_status_t fs_put(const char* name, const uint8_t* data, uint32_t size);

/* Borra. */
fs_status_t fs_delete(const char* name);

/* Stats. */
uint32_t fs_total_bytes(void);
uint32_t fs_used_bytes(void);
uint32_t fs_free_bytes(void);
int      fs_file_count(void);

/* Devuelve un literal con el mensaje de error. */
const char* fs_status_str(fs_status_t s);

/* #247 — registra este FS como backend de file I/O de BP (readFile/writeFile/
 * appendFile/fileExists + readFileBytes/writeFileBytes). Llamar tras fs_init(). */
void fs_register_bpvm(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_FS_H */
