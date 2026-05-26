/*
 * mdn_loader.h — loader de archivos .mdn (H3 #158).
 *
 * El loader:
 *  1. Valida magic + version + abi_version.
 *  2. Copia code section a un pool de RAM dedicado.
 *  3. Invalida I-cache del rango copiado (M33).
 *  4. Registra cada symbol vía bpvm_aot_register_by_name.
 *
 * Pool de RAM: 16 KB estáticos en .bss. Suficiente para varios
 * módulos típicos (fib + thunk son ~40 bytes; un Math.bp pequeño
 * ronda 1-2 KB).
 */
#ifndef BPVM_PICO_MDN_LOADER_H
#define BPVM_PICO_MDN_LOADER_H

#include <stdint.h>
#include <stddef.h>

struct bpvm;

#ifdef __cplusplus
extern "C" {
#endif

/* Códigos de retorno. */
#define MDN_OK              0
#define MDN_ERR_MAGIC      -1
#define MDN_ERR_VERSION    -2
#define MDN_ERR_ABI        -3
#define MDN_ERR_TOO_LARGE  -4   /* code section excede el pool */
#define MDN_ERR_TRUNCATED  -5   /* size < layout esperado */

/* Carga un .mdn ya en memoria (data + size).
 * Tras OK, los símbolos están registrados en el aot_registry y
 * el código vive en el pool g_aot_ram.
 *
 * Llamar tras bpvm_load_mod_buffer del .mod correspondiente — el
 * registry por nombre necesita los símbolos exportados del .mod
 * para resolver la dirección absoluta. */
int bpvm_load_mdn(struct bpvm* vm, const uint8_t* data, size_t size);

/* Limpia el pool de RAM de AOT y resetea el registry. Útil entre
 * cargas para evitar fragmentación. Llamar antes de un RUN nuevo
 * si quieres descartar AOTs de RUNs previos. */
void bpvm_mdn_reset(void);

/* Para diagnóstico. */
size_t bpvm_mdn_used_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
