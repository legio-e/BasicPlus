/*
 * mdn_loader.c — implementación del loader .mdn (H3 #158).
 *
 * Diseño minimalista:
 *  - Pool estático de 16 KB en .bss (RAM ejecutable por defecto en
 *    Cortex-M33 cuando no hay MPU configurada).
 *  - Bump allocator simple: las cargas son add-only; bpvm_mdn_reset
 *    descarta todo para empezar fresh.
 *  - I-cache invalidation tras cada copia.
 *
 * El thunk emitido por AotCEmitter no toca el stack BP de forma
 * que invalide la convención del caller — solo lee args del top y
 * escribe el resultado. Recordar: en Thumb-2 la dirección de una
 * función tiene bit 0 = 1 (indicador de modo).
 */

#include "mdn_loader.h"
#include "mdn_format.h"
#include "aot_registry.h"
#include "log.h"

#include "bpvm.h"

#include <stdint.h>
#include <string.h>

/* No incluimos cabeceras CMSIS — usamos inline asm directo para
 * DSB/ISB que están en cualquier toolchain ARM. */

/* Pool ejecutable. 4 KB suficiente para Bench (~80 bytes) + varios
 * módulos pequeños. Aumentar cuando AOT-ee módulos grandes. */
#define BPVM_AOT_RAM_SIZE  (4 * 1024)

/* Alineado a 8 para que cualquier subdivisión cumpla la alineación
 * mínima requerida por Thumb-2 (2 bytes) con margen. */
static uint8_t g_aot_ram[BPVM_AOT_RAM_SIZE] __attribute__((aligned(8)));
static size_t  g_aot_used = 0;

void bpvm_mdn_reset(void) {
    g_aot_used = 0;
    /* No tocamos g_aot_ram[] — los bytes viejos no importan, se
     * sobreescriben. */
}

size_t bpvm_mdn_used_bytes(void) {
    return g_aot_used;
}

/* Sincronización tras escribir código a memoria que vamos a ejecutar.
 * DSB completa los writes pendientes (que el código nuevo está en SRAM).
 * ISB descarga la instruction pipeline y fuerza re-fetch.
 *
 * Si la I-cache está habilitada y el rango ya estaba cacheado de una
 * carga previa (raro pero posible si reusamos addresses), faltaría
 * SCB_InvalidateICache_by_Addr. Lo añadimos si vemos crashes en
 * recargas — para la primera carga del pool fresh no es necesario. */
static void code_write_barrier(void) {
#if defined(__arm__)
    __asm__ __volatile__ ("dsb sy" ::: "memory");
    __asm__ __volatile__ ("isb sy" ::: "memory");
#else
    __asm__ __volatile__ ("" ::: "memory");
#endif
}

int bpvm_load_mdn(struct bpvm* vm, const uint8_t* data, size_t size) {
    if (!data || size < sizeof(mdn_header_t)) return MDN_ERR_TRUNCATED;

    /* Lee header. El formato es little-endian del lado del firmware,
     * que es lo que produce gcc en Cortex-M33. Si producimos el .mdn
     * desde Java big-endian, la tool tiene que swappear los enteros
     * del header — el code section ya es nativo. */
    const mdn_header_t* h = (const mdn_header_t*) data;

    /* Magic check. */
    static const uint8_t expected[4] = MDN_MAGIC;
    if (memcmp(h->magic, expected, 4) != 0) return MDN_ERR_MAGIC;
    if (h->version != MDN_VERSION) return MDN_ERR_VERSION;
    if (h->abi_version > MDN_ABI_VERSION) return MDN_ERR_ABI;

    /* Validación tamaño total. */
    size_t hdr_total = sizeof(mdn_header_t) + h->sym_count * sizeof(mdn_symbol_t);
    if (size < hdr_total + h->code_size) return MDN_ERR_TRUNCATED;
    if (h->code_size > BPVM_AOT_RAM_SIZE - g_aot_used) return MDN_ERR_TOO_LARGE;

    /* Copia code a RAM ejecutable. Alinear el destino a 4 bytes para
     * que el código Thumb-2 de los thunks tenga PC bien aligned. */
    g_aot_used = (g_aot_used + 3u) & ~3u;
    uint8_t* code_dst = g_aot_ram + g_aot_used;
    const uint8_t* code_src = data + hdr_total;
    memcpy(code_dst, code_src, h->code_size);
    g_aot_used += h->code_size;

    code_write_barrier();

    /* Registrar cada symbol via su qualified name. La dirección del
     * thunk es (code_dst + offset) | 1 — el bit 0 indica modo Thumb. */
    const mdn_symbol_t* syms = (const mdn_symbol_t*)(data + sizeof(mdn_header_t));
    int registered = 0;
    for (uint32_t i = 0; i < h->sym_count; i++) {
        uintptr_t thunk_addr = (uintptr_t)(code_dst + syms[i].thunk_offset) | 1u;
        int rc = bpvm_aot_register_by_name(vm, syms[i].name,
                                             (bpvm_aot_thunk_t) thunk_addr);
        if (rc == 0) {
            registered++;
            log_printf("MDN: registered '%s' @ %p", syms[i].name, (void*) thunk_addr);
        } else {
            log_printf("MDN: skip '%s' rc=%d (símbolo no en .mod?)", syms[i].name, rc);
        }
    }
    log_printf("MDN loaded: %u syms, %d registered, %u code bytes",
               (unsigned) h->sym_count, registered, (unsigned) h->code_size);
    log_flush();
    return MDN_OK;
}
