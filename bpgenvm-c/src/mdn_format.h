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
#define MDN_ABI_VERSION    6   /* [V6/N1.5] 24-ago — un slot nuevo al final
                                * (`newarray_i64`) y tres que dejaron de ser
                                * stubs. Lo primero mueve el tamaño de la tabla;
                                * lo segundo cambia lo que hace un .mdn ya
                                * emitido (de devolver 0 a alocar), y eso no
                                * debe pasar en silencio.
                                *
                                * Histórico: 5 = 11-ago — el .mdn NO solo depende de la TABLA de
                                * helpers: su codigo lee `vm->memory` y
                                * `vm->aot_helpers` POR DESPLAZAMIENTO dentro de
                                * `struct bpvm`. Ayer añadi un campo a
                                * `bpvm_module_t` y el offset se corrio 128 bytes:
                                * los .mdn ya generados leian basura y saltaban
                                * ahi. CUELGUE MUDO en placa.
                                *
                                * Los dos campos viven ahora en un PREFIJO
                                * CONGELADO al principio de la struct, con
                                * _Static_assert: meter algo delante ya no
                                * compila. Pero los .mdn de antes llevan los
                                * offsets viejos, asi que la ABI sube y se
                                * RECHAZAN con mensaje.
                                *
                                * Histórico: 3 = V5/H4, seis slots nuevos de
                                * helpers. 2 = #302, refs como handles de 64b.
                                *
                                * ⚠️ SUBIR ESTE NUMERO INVALIDA TODOS LOS .mdn, y
                                * eso es lo correcto: son artefactos y se
                                * regeneran. Vale la pena subirlo tambien cuando
                                * cambie el PREFIJO, no solo la tabla. */
#define MDN_NAME_MAX       32  /* longitud max de qualified name */

/* Arquitectura del código nativo del .mdn = e_machine del ELF de origen (H4).
 * El loader RECHAZA un .mdn cuya arch no case con la del firmware (ejecutar
 * código de otra ISA = crash). 0 = sin tag (.mdn legacy pre-H4, siempre ARM). */
#define MDN_ARCH_NONE      0    /* sin tag — legacy (ARM) */
#define MDN_ARCH_ARM       40   /* EM_ARM   (Cortex-M Thumb-2) */
#define MDN_ARCH_RISCV     243  /* EM_RISCV (RV32 — ESP32-P4) */
/* EM_XTENSA (ESP32-S3). Aún NO hay toolchain AOT para Xtensa: se define para que
 * el S3 diga la VERDAD en el INFO. Antes caía en el `else` del mapa y se
 * declaraba MDN_ARCH_NONE, que no significa "Xtensa" sino "host, SIN gate" —
 * doble daño: el IDE no sabía a qué compilar (tiraba del ajuste del proyecto,
 * que puede decir "arm") y el gate de arquitectura quedaba desarmado. */
#define MDN_ARCH_XTENSA    94   /* EM_XTENSA (ESP32-S3) */

/* V6 — EL PC. Dos valores y NO uno, porque son binarios distintos: la
 * observacion es de Eduardo (6-sep), *«x86 es en realidad x64, lo digo porque
 * los binarios son diferentes»*, y es la misma leccion que ya costo una vez con
 * `MDN_ARCH_ARM`, que no distingue hard de softfp — ver mdn_loader.h:54: «esa
 * discrepancia no da error de enlace, da numeros mal en silencio». Aqui es peor:
 * codigo de 32 en un proceso de 64 no da numeros mal, no es codigo.
 *
 * ⚠️ Y ojo al conjunto: `MDN_ARCH_ARM` (40) aqui significa Cortex-M **Thumb-2, 32
 * bits**, y el sufijo de fichero que usa el IDE para eso es `ARMV8` — que fuera
 * de este proyecto se entiende como AArch64, 64 bits. El nombre ya esta cogido y
 * significa otra cosa; el dia que aparezca un ARM de 64 hay que recordarlo. */
#define MDN_ARCH_X86       3    /* EM_386    (PC de 32 bits) */
#define MDN_ARCH_X64       62   /* EM_X86_64 (PC de 64 bits — el host normal) */

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
