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
 * H9 (unificación 19-jul): monta littlefs en la REGIÓN dada (offset+size
 * absolutos de flash, que vienen del ENV vía bpvm_part — offsets derivados).
 * Ya NO hay descriptor propio ni auto-init: sin particiones definidas NO se
 * monta nada (el boot se queda en estado 1 y el host conduce). Formatea si
 * no monta (región recién definida por el usuario = legítimo). Llamar UNA
 * vez, no concurrente con otras operaciones del FS. */
fs_status_t fs_init_at(uint32_t fs_offset, uint32_t fs_size);

/* Reformatea el volumen EN CALIENTE y recrea /sys, /lib y /app (el FORMAT del
 * wire). 0 / -1, con el motivo anotado en la fachada (#473, 466: era `void`, y
 * el FORMAT contestaba OK aunque no hubiera podido). */
int fs_format_ram(void);

/* No-op: littlefs persiste en cada close (el SAVE del wire). FS_OK. */
fs_status_t fs_save_to_flash(void);

/* H11 — `fs_get` RETIRADO. Devolvía un puntero "válido hasta el siguiente
 * fs_put/fs_delete", y ese contrato exigía tener el fichero ENTERO en RAM: un
 * scratch estático de 128 KB. Lo que de verdad hacía falta era cargar un
 * módulo, y eso ahora va por trozos (`bpvm_load_mod_stream`) leyendo cada
 * bloque directamente a su sitio en memory[]. Para mirar/leer un fichero: la
 * fachada (`bpvm_fs_stat` / `bpvm_fs_read` / `bpvm_fs_read_at`). */

/* Escribe (o sobrescribe) el fichero entero, creando las carpetas que falten.
 * Lo usa el instalador de la stdlib. #473 (264, 3-oct): fs_exists,
 * fs_put_append, fs_delete, fs_free_bytes y fs_status_str, fuera: sin
 * llamadores (el wire va por la fachada desde U3). */
fs_status_t fs_put(const char* name, const uint8_t* data, uint32_t size);

/* Stats (del volumen littlefs). */
uint32_t fs_total_bytes(void);
uint32_t fs_used_bytes(void);
int      fs_file_count(void);

/* Registra este FS como backend de file I/O de BP (readFile/writeFile/
 * appendFile/fileExists, #247). Llamar una vez al boot. */
void fs_register_bpvm(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_FS_H */
