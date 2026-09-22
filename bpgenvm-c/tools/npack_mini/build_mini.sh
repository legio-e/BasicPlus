#!/bin/sh
# build_mini.sh — el mini para los TRES destinos del `.npk` (V7/H6 P1).
#
#     bash bpgenvm-c/tools/npack_mini/build_mini.sh [arm|p4|imc|todos]
#
# Misma cadena que los packs de verdad (bpstdlib/sqlite/nativo/src/build_sqlite*.sh):
# compilar con las banderas de la RECETA de esa familia, enlazar en BASE 0 con
# --emit-relocs, comprobar que no queda ni un símbolo abierto, y `NpackBuild`
# (el realojado lo hace el IDE al grabar). Salida, VERSIONADA, al lado:
#
#     mini.npk.ARMV8     Metro RP2350 / STM32   (arm-cortex-m33, softfp)
#     mini.npk.RISCV     ESP32-P4               (riscv32-esp-p4,  ilp32f)
#     mini.npk.RV32IMC   ESP32-C3 / C6 / C5     (riscv32-imc,     ilp32)
#
# Se graba con `wire_serie.py COMx pack <un .pack que lleve el mini> <arch>` — o
# mejor, el `.npk` suelto dentro de un `.pack` de un solo fichero (Pack.jar build).
set -e
HERE="$(cd "$(dirname "$0")" && { pwd -W 2>/dev/null || pwd; })"
ROOT="$(cd "$(dirname "$0")/../../.." && { pwd -W 2>/dev/null || pwd; })"
INC="$ROOT/bpgenvm-c/include"
CP="$ROOT/lexer-java/target/classes;$ROOT/pack/target/classes;$ROOT/miVM/target/classes"
QUE="${1:-todos}"

ARM_BIN="C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin"
RV_BIN="$HOME/.espressif/tools/riscv32-esp-elf/esp-15.2.0_20251204/riscv32-esp-elf/bin"

# uno <etiqueta> <gcc> <ld> <nm> <isa-flags> <ld-extra> <destino-npack> <sufijo>
uno() {
    ET="$1"; GCC="$2"; LD="$3"; NM="$4"; ISA="$5"; LDX="$6"; DEST="$7"; SUF="$8"
    CF="$ISA -fno-jump-tables -Os -ffunction-sections -fdata-sections -fno-builtin -ffreestanding -Wall -Wextra -I$INC"
    echo "== mini para $ET ($DEST) =="
    "$GCC" $CF -c "$HERE/mini.c" -o "$HERE/mini_$SUF.o"
    OLDIFS="$IFS"; IFS=' '
    # shellcheck disable=SC2086
    "$LD" -Ttext=0x0 -Tdata=0x20000000 --emit-relocs $LDX -e bp_pack_init -o "$HERE/mini_$SUF.elf" "$HERE/mini_$SUF.o"
    IFS="$OLDIFS"
    U=$("$NM" -u "$HERE/mini_$SUF.elf" 2>/dev/null || true)
    if [ -n "$U" ]; then echo "  FALLO — símbolos abiertos:"; echo "$U"; exit 1; fi
    java -cp "$CP" basicplus.frontend.NpackBuild "$HERE/mini_$SUF.elf" "$DEST" "$HERE/mini.npk.$SUF"
    rm -f "$HERE/mini_$SUF.o" "$HERE/mini_$SUF.elf"
}

if [ "$QUE" = arm ] || [ "$QUE" = todos ]; then
    uno "ARM Cortex-M33" "$ARM_BIN/arm-none-eabi-gcc.exe" "$ARM_BIN/arm-none-eabi-ld.exe" "$ARM_BIN/arm-none-eabi-nm.exe" \
        "-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16" "" arm-cortex-m33 ARMV8
fi
if [ "$QUE" = p4 ] || [ "$QUE" = todos ]; then
    uno "ESP32-P4 (ilp32f)" "$RV_BIN/riscv32-esp-elf-gcc.exe" "$RV_BIN/riscv32-esp-elf-ld.exe" "$RV_BIN/riscv32-esp-elf-nm.exe" \
        "-march=rv32imafc_zicsr_zifencei -mabi=ilp32f -mno-relax" "--no-relax" riscv32-esp-p4 RISCV
fi
if [ "$QUE" = imc ] || [ "$QUE" = todos ]; then
    uno "ESP32-C3/C6/C5 (ilp32)" "$RV_BIN/riscv32-esp-elf-gcc.exe" "$RV_BIN/riscv32-esp-elf-ld.exe" "$RV_BIN/riscv32-esp-elf-nm.exe" \
        "-march=rv32imc_zicsr_zifencei -mabi=ilp32 -mno-relax" "--no-relax" riscv32-imc RV32IMC
fi
ls -la "$HERE"/mini.npk.* | awk '{print $5, $9}'
