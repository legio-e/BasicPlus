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

/* Devuelve puntero (dentro del arena) + tamaño. 0 OK, -1 no existe.
 * El puntero es válido hasta el siguiente fs_put/fs_del/fs_format. */
int  fs_get(const char* name, const uint8_t** data, uint32_t* size);

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
 * si falla, el próximo fs_load lo detecta (magic) y arranca con FS vacío.
 * Llamar tras cada mutación que deba sobrevivir al reset (PUT/DEL/FORMAT). */
void fs_save(void);

/* Restaura el FS desde flash al boot. Salta las entradas /lib/ (las re-instala
 * el firmware embebido → sin desincronización de stdlib). 0 si cargó algo,
 * -1 si la flash está vacía/corrupta (FS queda vacío). */
int  fs_load(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32_FS_H */
