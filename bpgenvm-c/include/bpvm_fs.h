/*
 * bpvm_fs.h — fachada de file I/O para la VM C (H10 / #247).
 *
 * Los builtins readFile/writeFile/appendFile/fileExists (src/builtins.c)
 * delegan en el backend registrado por la plataforma:
 *   - host (test/main.c): backend libc (bpvm_fs_register_host) → FS real.
 *   - device (Pico/STM32/ESP32): backend sobre el FS del firmware
 *     (fs_get/fs_put), registrado en el main de cada placa.
 * Sin backend, las operaciones fallan limpio (-1/0) y el builtin lanza un
 * RuntimeError BP atrapable — mismo comportamiento que un builtin no soportado.
 *
 * Mismo patrón que bpvm_gpio.h: tabla de punteros + set_backend + funciones
 * efectivas con fallback.
 */
#ifndef BPVM_FS_H
#define BPVM_FS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* 0 + *size si el fichero existe; -1 si no. */
    int  (*stat)(const char* path, uint32_t* size);
    /* lee hasta `cap` bytes en `dst`; devuelve nº de bytes leídos, o -1. */
    long (*read)(const char* path, uint8_t* dst, uint32_t cap);
    /* escribe `len` bytes; append!=0 → al final (crea si no existe). 0 / -1. */
    int  (*write)(const char* path, const uint8_t* data, uint32_t len, int append);
    /* #240 — ops opcionales (NULL → el builtin lanza "no soportado" limpio).
     * Campos AL FINAL: los backends existentes usan designated initializers
     * y los dejan a NULL sin tocarse. */
    /* borra el fichero. 0 / -1. */
    int  (*remove)(const char* path);
    /* renombra/mueve; si `to` existe lo SOBREESCRIBE (semántica de la VM-Java:
     * REPLACE_EXISTING). 0 / -1. */
    int  (*rename)(const char* from, const char* to);
    /* #240 (2ª pasada) — resto de IO.bp; opcionales como remove/rename. */
    /* crea el directorio (e intermedios); ok si ya existe. 0 / -1. */
    int  (*mkdir)(const char* path);
    /* borra el directorio SOLO si está vacío. 0 / -1. */
    int  (*rmdir)(const char* path);
    /* copia from → to sobreescribiendo. 0 / -1. */
    int  (*copy)(const char* from, const char* to);
    /* 1 si path es un directorio existente; 0 en otro caso. */
    int  (*isdir)(const char* path);
    /* mtime en ms epoch (puede truncarse a i32 arriba); -1 si error. */
    long long (*mtime_ms)(const char* path);
} bpvm_fs_backend_t;

/* Registra el backend (una vez al boot). */
void bpvm_fs_set_backend(const bpvm_fs_backend_t* backend);

/* Funciones efectivas (sin backend → fallo limpio). */
int  bpvm_fs_stat  (const char* path, uint32_t* size);
long bpvm_fs_read  (const char* path, uint8_t* dst, uint32_t cap);
int  bpvm_fs_write (const char* path, const uint8_t* data, uint32_t len, int append);
int  bpvm_fs_exists(const char* path);   /* 1 / 0 */
int  bpvm_fs_remove(const char* path);                    /* #240: 0 / -1 */
int  bpvm_fs_rename(const char* from, const char* to);    /* #240: 0 / -1 */
int  bpvm_fs_mkdir (const char* path);                    /* #240 2ª: 0 / -1 */
int  bpvm_fs_rmdir (const char* path);                    /* #240 2ª: 0 / -1 */
int  bpvm_fs_copy  (const char* from, const char* to);    /* #240 2ª: 0 / -1 */
int  bpvm_fs_isdir (const char* path);                    /* #240 2ª: 1 / 0 */
long long bpvm_fs_mtime_ms(const char* path);             /* #240 2ª: ms / -1 */

/* Backend host (libc). Implementado en fs_host.c (host-only); el firmware
 * registra el suyo (fs_get/fs_put). */
void bpvm_fs_register_host(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_FS_H */
