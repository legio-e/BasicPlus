/*
 * bpvm_aot_recetas.h — LAS RECETAS del código nativo (`.mdn`), V7/H2 (#441).
 *
 * La ÚNICA fuente de los flags con los que se compila un `.mdn`. La leen:
 *
 *   · el IDE (`AotBuild`), que la PARSEA en tiempo de ejecución para saber con
 *     qué banderas invocar a gcc y estampa en la cabecera del `.mdn` (v2,
 *     `flags_crc`) el CRC32 del literal de la receta;
 *   · el firmware (`mdn_loader.c`), que ELIGE su receta por las macros de la ISA
 *     con que se compiló, calcula el mismo CRC32 del mismo literal, y RECHAZA un
 *     `.mdn` que traiga otro (`MDN_ERR_RECETA`): la función sigue interpretada;
 *   · `pico/build_mdn.sh`, con `sed`.
 *
 * Por qué existe: el 17-ago añadir `-mcmodel=medany` dejó malos TODOS los `.mdn`
 * de RISC-V ya generados. Misma arquitectura, misma fecha, código inservible —
 * ni el gate de `arch` ni la comparación de fechas lo ven, y lo que falla es la
 * placa, muda. Con la receta sellada, un `.mdn` de otra receta se rechaza al
 * cargar con un mensaje, y el IDE ya no lo sube (compara con el `aotFlagsCrc`
 * del INFO).
 *
 * ⚠️ REGLAS DEL FICHERO (el IDE lo parsea con una expresión regular):
 *   1. Cada receta es `#define BPVM_AOT_RECETA_<NOMBRE> "<flags>"` en UNA línea,
 *      sin escapes (`\`) ni comillas dentro del literal.
 *   2. El NOMBRE es el juego de instrucciones y la ABI que produce el código, no
 *      una placa (decisión de Eduardo, 15-sep: *«debería ser por juego de
 *      instrucciones nada más»*). Dos placas con la misma ISA/ABI comparten
 *      receta y `.mdn` (Pico 2 y STM32U5 son las dos Cortex-M33).
 *   3. El CRC es el de los BYTES del literal, tal cual: cambiar un espacio cambia
 *      la huella. Es lo que se quiere — la receta es el texto.
 *   4. Sólo flags de COMPILACIÓN (los que deciden repertorio, ABI, modelo de
 *      código, PIC). Los de enlace (`LINK_FLAGS_COMUNES` del IDE, `aot/mdn.ld`)
 *      no entran: si cambian y el `.text` deja de bastarse, lo caza el guardián
 *      de relocalizaciones de `MdnPack` en el BUILD, no en la placa.
 *
 * Y NO se copian a mano a ningún otro sitio: un flag que sólo esté aquí es un
 * flag que el firmware exige y el IDE aplica, los dos a la vez.
 */
#ifndef BPVM_AOT_RECETAS_H
#define BPVM_AOT_RECETAS_H

/* [V7/H5 #560, 25-sep] `-ffp-contract=off` EN LAS TRES. Sin él, gcc funde `a * b + c` en UNA
 * instrucción (`vfma.f32` en el Cortex-M33, `fmadd.s` en el P4) que redondea una sola vez; el
 * intérprete hace OP_FMUL y luego OP_FADD, y redondea dos. El mismo programa daba otro número
 * con `.mdn` que sin él: con a = b = 1 + 2^-12 y c = -(1 + 2^-11), 0 interpretado y 5.96e-08
 * nativo (`samples/FloatNat.bp`, caso `fma`). Con el flag, ARM usa `vmla.f32` —encadenada: dos
 * redondeos, como el intérprete— y el P4, `fmul.s` y `fadd.s`. En la receta sin FPU no cambia
 * el código (no hay instrucción que fundir), pero va igual: la receta dice cómo se redondea. */

/* ARMv8-M Mainline con FPU simple, coma flotante softfp (args en registros
 * enteros): Cortex-M33 — RP2350 (Pico 2 / Metro) y STM32U5. Un solo `.mdn` para
 * las dos: la frontera con los helpers va fijada a aapcs por las dos puntas
 * (`BPVM_AOT_FP_ABI`, bpvm_aot_helpers.h), así que el STM32 (firmware `hard`)
 * ejecuta el mismo blob que la Pico (`softfp`). */
#define BPVM_AOT_RECETA_ARMV8M_MAIN_SOFTFP "-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -fpic -fno-jump-tables -ffp-contract=off -Os"

/* RV32IMAFC, ABI ilp32f (float en registros de la FPU, double por software):
 * ESP32-P4. `-mcmodel=medany` + `-mno-relax` son los que hacen el `.mdn`
 * cargable en cualquier dirección (#440); `-fno-pic` porque el enlace a
 * `-Ttext=0` resuelve lo interno y deja todo PC-relativo. */
#define BPVM_AOT_RECETA_RV32IMAFC_ILP32F "-march=rv32imafc_zicsr_zifencei -mabi=ilp32f -fno-pic -mcmodel=medany -mno-relax -fno-jump-tables -ffp-contract=off -Os"

/* RV32IMC, ABI ilp32 (sin FPU): ESP32-C3 / C6 / C5 (el C6 y el C5 son IMAC,
 * superconjunto). El IDE la elige por el destino `riscv-imc` (V7/H6), y desde
 * el 25-sep corre en placa (la C5). Sin FPU, sus `float` van por los helpers de
 * la tabla (`BPVM_AOT_FLOAT_SOFT`, bpvm_aot_helpers.h, #559). */
#define BPVM_AOT_RECETA_RV32IMC_ILP32 "-march=rv32imc_zicsr_zifencei -mabi=ilp32 -fno-pic -mcmodel=medany -mno-relax -fno-jump-tables -ffp-contract=off -Os"

/* ── LA RECETA DE ESTE FIRMWARE, por la ISA con que se compila ESTE fichero ──
 *
 * Compile-time, como `MDN_HOST_ARCH` en mdn_loader.c. En ARM decide sólo la
 * arquitectura (ARMv8-M Mainline), NO el float-abi del firmware: el `.mdn` ARM
 * es uno para las dos familias (ver arriba). En RISC-V decide la ABI de coma
 * flotante, que es lo que separa al P4 de los C3/C6 (#502).
 *
 * Sin receta (PC, Xtensa): cadena vacía y CRC 0 → el cargador no aplica el
 * gate de receta, igual que `MDN_ARCH_NONE` desarma el de arquitectura. */
#if defined(BPVM_AOT_HOST_RECETA_FORZADA)
/* Sólo para los tests de host (test_mdnreceta): el PC no tiene receta y el gate
 * no se podría ejercitar. Ningún firmware define esto. */
#  define BPVM_AOT_HOST_RECETA          BPVM_AOT_HOST_RECETA_FORZADA
#  define BPVM_AOT_HOST_RECETA_NOMBRE   "FORZADA"
#elif defined(__ARM_ARCH_8M_MAIN__)
#  define BPVM_AOT_HOST_RECETA          BPVM_AOT_RECETA_ARMV8M_MAIN_SOFTFP
#  define BPVM_AOT_HOST_RECETA_NOMBRE   "ARMV8M_MAIN_SOFTFP"
#elif defined(__riscv) && (__riscv_xlen == 32) && defined(__riscv_float_abi_single)
#  define BPVM_AOT_HOST_RECETA          BPVM_AOT_RECETA_RV32IMAFC_ILP32F
#  define BPVM_AOT_HOST_RECETA_NOMBRE   "RV32IMAFC_ILP32F"
#elif defined(__riscv) && (__riscv_xlen == 32) && defined(__riscv_float_abi_soft)
#  define BPVM_AOT_HOST_RECETA          BPVM_AOT_RECETA_RV32IMC_ILP32
#  define BPVM_AOT_HOST_RECETA_NOMBRE   "RV32IMC_ILP32"
#else
#  define BPVM_AOT_HOST_RECETA          ""
#  define BPVM_AOT_HOST_RECETA_NOMBRE   ""
#endif

#endif /* BPVM_AOT_RECETAS_H */
