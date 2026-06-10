/*
 * mdn_loader.c — loader .mdn (H3 #158).
 *
 * Diseño zero-copy: el código Thumb-2 del thunk vive en el buffer
 * `data` que el caller nos pasa (típicamente `.data` con el blob
 * embebido o el buffer del FS donde guardamos el .mdn). Como ese
 * buffer ya está en RAM ejecutable con una dirección estable, NO
 * hacemos memcpy NI necesitamos pool de RAM aparte. Solo:
 *
 *   1. Validamos magic + version + abi.
 *   2. Validamos consistencia de tamaños (header + symtab + code).
 *   3. Para cada símbolo, calculamos thunk_addr = (data + hdr_total
 *      + sym.thunk_offset) | 1u   (bit Thumb).
 *   4. bpvm_aot_register_by_name lo guarda en el registry global.
 *
 * REQUISITO IMPORTANTE: el buffer pasado a bpvm_load_mdn debe
 * permanecer válido y en RAM mientras los thunks estén registrados.
 * Para .mdn embebidos en firmware (.data array) eso es trivial. Para
 * .mdn cargados desde FS, el caller debe pinear la región en RAM
 * mientras dure el RUN.
 */

#include "mdn_loader.h"
#include "mdn_format.h"
#include "aot_registry.h"

#include "bpvm.h"

#include <stdint.h>
#include <string.h>

/* H9.5 — el loader es compartido entre ports (Pico, STM32, ...). Las trazas
 * van por bpvm_mdn_log, débil no-op aquí: el Pico da una implementación
 * fuerte sobre su log persistente (pico/aot_funcs.c); el STM32 (wire-only)
 * se queda con el silencio. Así el fichero no depende de ningún log.h. */
__attribute__((weak)) void bpvm_mdn_log(const char* fmt, ...) { (void) fmt; }

/* El FS pinea cada fichero 4-aligned (fs.c v4), así que data viene
 * ya correctamente alineado para Thumb-2. NO necesitamos staging.
 * Las funciones legacy quedan como no-ops por compat. */
void   bpvm_mdn_reset(void)        { /* no-op en zero-copy */ }
size_t bpvm_mdn_used_bytes(void)   { return 0; }

int bpvm_load_mdn(struct bpvm* vm, const uint8_t* data, size_t size) {
    if (!data || size < sizeof(mdn_header_t)) return MDN_ERR_TRUNCATED;

    /* Validar alineación. Thumb-2 requiere PC bit 1 = 0, lo que exige
     * que el code_base (= data + hdr_total) sea al menos 2-aligned.
     * Como hdr_total siempre es múltiplo de 4 (header=20, sym=36 cada
     * uno), basta con que data sea 4-aligned. El FS v4 lo garantiza
     * para ficheros del FS; los .mdn embebidos en .data del firmware
     * también vienen alineados por el compilador. Si llega misaligned,
     * es bug del caller. */
    if (((uintptr_t) data) & 0x3u) {
        bpvm_mdn_log("MDN: ABORT — data %p no alineado a 4 (FS v4 debería garantizar)",
                   (const void*) data);
        return MDN_ERR_TRUNCATED;
    }

    const mdn_header_t* h = (const mdn_header_t*) data;

    /* Magic + version + ABI. */
    static const uint8_t expected[4] = MDN_MAGIC;
    if (memcmp(h->magic, expected, 4) != 0) return MDN_ERR_MAGIC;
    if (h->version != MDN_VERSION)          return MDN_ERR_VERSION;
    if (h->abi_version > MDN_ABI_VERSION)   return MDN_ERR_ABI;

    /* Layout sanity. */
    size_t hdr_total = sizeof(mdn_header_t)
                     + (size_t) h->sym_count * sizeof(mdn_symbol_t);
    if (size < hdr_total + h->code_size) return MDN_ERR_TRUNCATED;

    /* Registro zero-copy: el código Thumb-2 ya está en RAM en data[],
     * solo apuntamos el thunk ahí. Bit 0 del address = modo Thumb. */
    const uint8_t*      code_base = data + hdr_total;
    const mdn_symbol_t* syms      = (const mdn_symbol_t*)
                                      (data + sizeof(mdn_header_t));
    int registered = 0;
    for (uint32_t i = 0; i < h->sym_count; i++) {
        uintptr_t thunk_addr = (uintptr_t)(code_base + syms[i].thunk_offset) | 1u;
        int rc = bpvm_aot_register_by_name(vm, syms[i].name,
                                             (bpvm_aot_thunk_t) thunk_addr);
        if (rc == 0) {
            registered++;
        } else {
            bpvm_mdn_log("MDN: skip '%s' rc=%d (symbol no en .mod?)",
                       syms[i].name, rc);
        }
    }
    bpvm_mdn_log("MDN: %d/%u thunks registrados, %u code bytes (zero-copy)",
               registered, (unsigned) h->sym_count, (unsigned) h->code_size);
    return MDN_OK;
}
