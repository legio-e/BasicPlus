/*
 * mdn_format.h — formato binario del archivo .mdn (H3 #158).
 *
 * El .mdn lleva código AOT C-emitido y compilado a Thumb-2 PIC.
 * Se compone de:
 *
 *   [mdn_header_t]
 *   [N × mdn_symbol_t]      ← N = header.sym_count
 *   [code_size bytes]       ← código Thumb-2 puro, sin relocs
 *
 * El loader del firmware (bpvm_load_mdn) copia el code section a RAM
 * ejecutable, invalida I-cache, y registra cada símbolo en el
 * aot_registry vía bpvm_aot_register_by_name.
 *
 * Position-independence: el código se compila con -fpic, las calls
 * self son PC-relativas (resueltas por gcc al producir el .o, no
 * necesitan relocs), y las calls a helpers del runtime van via
 * vm->aot_helpers->func() (memoria indirect). Cero relocations a
 * resolver al cargar.
 *
 * Endian: little-endian para los enteros del header y del symbol
 * table. (Distinto del .mod, que es big-endian; el writer es
 * MdnPack.java en el frontend Java y usa ByteOrder.LITTLE_ENDIAN
 * explícito. El consumidor on-target son las arquitecturas ARM LE
 * / RISC-V LE, así que no hay byte-swap en runtime.)
 */
#ifndef BPVM_MDN_FORMAT_H
#define BPVM_MDN_FORMAT_H

#include <stdint.h>

#define MDN_MAGIC          { 'M', 'D', 'N', 0 }
#define MDN_VERSION        1   /* incrementar si cambia el header layout */
#define MDN_ABI_VERSION    2   /* #302 paso 2 — matches aot_helpers_v2_t (refs = handles).
                                * Un .mdn de ABI 1 (refs = offset crudo) pasaría offsets a
                                * helpers que ahora esperan handles → corrupción; el loader
                                * lo RECHAZA con MDN_ERR_ABI (gate estilo #284). */
#define MDN_NAME_MAX       32  /* longitud max de qualified name */

/* Arquitectura del código nativo del .mdn = e_machine del ELF de origen (H4).
 * El loader RECHAZA un .mdn cuya arch no case con la del firmware (ejecutar
 * código de otra ISA = crash). 0 = sin tag (.mdn legacy pre-H4, siempre ARM). */
#define MDN_ARCH_NONE      0    /* sin tag — legacy (ARM) */
#define MDN_ARCH_ARM       40   /* EM_ARM   (Cortex-M Thumb-2) */
#define MDN_ARCH_RISCV     243  /* EM_RISCV (RV32 — ESP32-P4) */

typedef struct {
    uint8_t  magic[4];     /* "MDN\0" */
    uint16_t version;      /* formato del header — actualmente 1 */
    uint16_t abi_version;  /* mínimo aot_helpers_vN_t que necesita */
    uint32_t code_size;    /* bytes del code section */
    uint32_t sym_count;    /* nº de entradas mdn_symbol_t que siguen */
    /* Arquitectura del código = e_machine del ELF (MDN_ARCH_*). Antes _reserved
     * (0); el loader trata 0 como legacy-ARM por compat. Gate de arch estilo #284. */
    uint32_t arch;
} mdn_header_t;

typedef struct {
    char     name[MDN_NAME_MAX];  /* qualified BP name, e.g. "Bench.fib" */
    uint32_t thunk_offset;        /* offset del thunk dentro del code section */
} mdn_symbol_t;

/* Total header + N symbols, alineado a 4. El code section empieza
 * inmediatamente después. */

#endif /* BPVM_MDN_FORMAT_H */
