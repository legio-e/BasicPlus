/*
 * stm32_fs.h — filesystem en RAM (mini) para el STM32 (H9.2.b).
 *
 * Namespace plano: los ficheros se guardan por path completo (p.ej.
 * "/app/Hello.mod"). Sin flash todavía → se pierde al resetear (suficiente
 * para el dev-loop "subir + ejecutar"; la persistencia en flash es H9.3).
 */
#ifndef STM32_FS_H
#define STM32_FS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Guarda (o sobreescribe) un fichero. 0 OK, -1 sin espacio / demasiados. */
int  fs_put(const char* name, const uint8_t* data, uint32_t size);

/* #294 streaming PUT — apende un trozo (el PUT_BEGIN del wire crea/trunca con
 * fs_put(name,NULL,0)). Sube ficheros > buffer del wire por trozos. */
int  fs_put_append(const char* name, const uint8_t* data, uint32_t size);

/* Devuelve puntero (dentro del arena) + tamaño. 0 OK, -1 no existe.
 * El puntero es válido hasta el siguiente fs_put/fs_del/fs_format. */
/* H11 — fs_get RETIRADO: su contrato (devolver un PUNTERO a los bytes) obligaba
 * a un espejo estático del tamaño de la arena (496 KB / 96 KB). Usar la fachada
 * por trozos: bpvm_fs_stat / bpvm_fs_read_at / bpvm_fs_read / bpvm_fs_crc32. */

/* Borra (compacta el arena). 0 OK, -1 no existe. */
int  fs_del(const char* name);

/* Itera entradas: fs_count() y fs_entry(i,...). 0 OK en fs_entry. */
int  fs_count(void);
int  fs_entry(int i, const char** name, uint32_t* size);

uint32_t fs_total_bytes(void);
uint32_t fs_used_bytes(void);
void     fs_format(void);

/* --- Persistencia en flash interna (H9.3) --- */

/* Vuelca el FS (arena + tabla) a la región reservada de flash. Best-effort:
 * si falla, el próximo arranque lo detecta (magic) y arranca con FS vacío.
 * Llamar tras cada mutación que deba sobrevivir al reset (PUT/DEL/FORMAT). */
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
