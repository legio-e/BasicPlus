/*
 * mdn_loader.h — loader de archivos .mdn (H3 #158).
 *
 * Diseño zero-copy:
 *  1. Valida magic + version + abi_version + layout.
 *  2. Por cada símbolo, registra thunk_addr = (data + hdr_total
 *     + sym.thunk_offset) | 1u vía bpvm_aot_register_by_name.
 *  3. NO copia el código — confía en que el buffer `data` pasado
 *     vive en RAM ejecutable con dirección estable y persiste
 *     mientras los thunks estén registrados.
 *
 * Para .mdn embebido en firmware (.data array de xxd -i), trivial.
 * Para .mdn cargado desde FS, el caller debe asegurar que el buffer
 * del FS no se mueva mientras los thunks estén activos.
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

/* Legacy del approach con copy/pool. Hoy son no-ops — el registry
 * se limpia vía bpvm_aot_clear() (del aot_registry) y no hay pool
 * de RAM extra que resetear. Mantenidos por compat con callers. */
void   bpvm_mdn_reset(void);
size_t bpvm_mdn_used_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
