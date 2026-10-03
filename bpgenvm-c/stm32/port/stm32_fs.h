/*
 * stm32_fs.h — lo que queda del API legado del FS del STM32, sobre la fachada
 * bpvm_fs y littlefs en la flash interna (fs_lfs_stm32.c). Era un FS en RAM
 * (H9.2.b) hasta #297; el wire va por la fachada desde U3.
 */
#ifndef STM32_FS_H
#define STM32_FS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Escribe (o sobrescribe) el fichero entero, creando las carpetas que falten
 * (el instalador de /lib). 0 OK, -1 con el motivo anotado en la fachada. */
int  fs_put(const char* name, const uint8_t* data, uint32_t size);

/* Devuelve puntero (dentro del arena) + tamaño. 0 OK, -1 no existe.
 * El puntero es válido hasta el siguiente fs_put/fs_del/fs_format. */
/* H11 — fs_get RETIRADO: su contrato (devolver un PUNTERO a los bytes) obligaba
 * a un espejo estático del tamaño de la arena (496 KB / 96 KB). Usar la fachada
 * por trozos: bpvm_fs_stat / bpvm_fs_read_at / bpvm_fs_read / bpvm_fs_crc32. */

/* Cuantos ficheros hay. #461: lo cuenta el comun, sin indice persistente. */
int  fs_count(void);

uint32_t fs_total_bytes(void);
uint32_t fs_used_bytes(void);
/* Reformatea y recrea /sys, /lib y /app (el FORMAT del wire). 0 / -1 (#473,
 * 466: era `void`, y el FORMAT contestaba OK aunque no hubiera podido). #473
 * (264, 3-oct): fs_put_append, fs_del y fs_list_omitidas, fuera: sin llamadores. */
int      fs_format(void);

/* --- Persistencia en flash interna (H9.3) --- */

/* No-op: littlefs persiste en cada close (el SAVE del wire). */
void fs_save(void);

/* H9 — monta el FS (littlefs) en un SUB-RANGO de la flash: `fs_offset` DESDE
 * FLASH_BASE + `fs_size` (múltiplo de página). Lo llama el arranque escalonado con
 * la región que define el env (bpvm_part). 0 OK, -1 si no monta ni formatea. */
int  fs_init_at(uint32_t fs_offset, uint32_t fs_size);

/* Registra este FS como backend de file I/O de BP (readFile/writeFile/
 * appendFile/fileExists, #247). Llamar una vez al boot. */
void stm32_fs_register_bpvm(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32_FS_H */
