#!/bin/sh
# build_driver.sh — un DRIVER DE PANTALLA en su pack, con UNA orden (V7/H6.c G2).
#
#     bash bpstdlib/gui/drivers/build_driver.sh <driver> [imc|p4|arm|todos]
#
# <driver> es su carpeta, bpstdlib/gui/drivers/<driver>/, con:
#     <Nombre>.bpbuild        el proyecto del pack (out: pack, provides: PANT)
#     <Nombre>.bp             el módulo BP que lo acompaña (un pack lleva al menos uno)
#     nativo/src/*.c          el driver: publica 'PANT' (ver bpgenvm-c/include/bpvm_pant.h)
#     resources/              (opcional) ficheros que viajan en el pack; el driver los lee
#                             por la BIOS de `pack:<Nombre>/<fichero>` (DriverSpi: st7789.seq)
#
# y deja:
#     nativo/driver_<driver>.npk.<SUFIJO>   el motor de cada destino (VERSIONADO)
#     out/<Nombre>.pack                     lo que se graba:
#         python bpgenvm-c/tools/wire_serie.py COMx pack <out/Nombre.pack> <arch>
#
# La cadena es la de siempre —la del mini y la del pack de LVGL—: compilar con la
# RECETA de la familia, enlazar en BASE 0 con --emit-relocs, comprobar que no
# queda ni un símbolo abierto, y `NpackBuild` (el realojado lo hace el IDE al
# grabar). Lo que añade es ARMAR EL PACK: copiar los .npk al out/ del proyecto y
# compilarlo. Un driver de pantalla es lo que escribirá quien tenga una pantalla
# que no conocemos, así que tiene que salir de una orden y no de cuatro copiadas
# de un comentario (el agujero que dejó escrito V7_IDEAS §H6.7.9).
#
# Destino por defecto: imc (C3/C6/C5). `p4` es el ESP32-P4 (ilp32f) y `arm` la
# Metro y los STM32 (Cortex-M33).
set -e
DRV="$1"
QUE="${2:-imc}"
if [ -z "$DRV" ]; then echo "uso: build_driver.sh <driver> [imc|p4|arm|todos]"; exit 1; fi
HERE="$(cd "$(dirname "$0")" && { pwd -W 2>/dev/null || pwd; })"
ROOT="$(cd "$(dirname "$0")/../../.." && { pwd -W 2>/dev/null || pwd; })"
D="$HERE/$DRV"
if [ ! -d "$D/nativo/src" ]; then echo "no existe $D/nativo/src"; exit 1; fi
BPB=$(ls "$D"/*.bpbuild 2>/dev/null | head -1)
if [ -z "$BPB" ]; then echo "falta el .bpbuild en $D"; exit 1; fi
INC="$ROOT/bpgenvm-c/include"
CP="$ROOT/toolchain/lexer-java/target/classes;$ROOT/toolchain/pack/target/classes;$ROOT/toolchain/miVM/target/classes"

ARM_BIN="C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin"
RV_BIN="$HOME/.espressif/tools/riscv32-esp-elf/esp-15.2.0_20251204/riscv32-esp-elf/bin"

# uno <etiqueta> <gcc> <ld> <nm> <isa-flags> <ld-extra> <destino-npack> <sufijo>
uno() {
    ET="$1"; GCC="$2"; LD="$3"; NM="$4"; ISA="$5"; LDX="$6"; DEST="$7"; SUF="$8"
    CF="$ISA -fno-jump-tables -Os -ffunction-sections -fdata-sections -fno-builtin -ffreestanding -Wall -Wextra -I$INC"
    echo "== driver '$DRV' para $ET ($DEST) =="
    OBJ="$D/nativo/obj_$SUF"
    rm -rf "$OBJ"; mkdir -p "$OBJ"
    for f in "$D"/nativo/src/*.c; do
        "$GCC" $CF -c "$f" -o "$OBJ/$(basename "$f" .c).o"
    done
    LIBGCC=$("$GCC" $ISA -print-libgcc-file-name)
    OLDIFS="$IFS"; IFS=' '
    # shellcheck disable=SC2086
    # El guion y no -Ttext/-Tdata: un driver suele no tener NINGUNA variable
    # inicializada, y sin .data, -Tdata no coloca su RAM (ver bpgenvm-c/tools/npk.ld).
    "$LD" -T "$ROOT/bpgenvm-c/tools/npk.ld" --emit-relocs --gc-sections $LDX -e bp_pack_init \
          -o "$OBJ/driver.elf" "$OBJ"/*.o "$LIBGCC"
    IFS="$OLDIFS"
    U=$("$NM" -u "$OBJ/driver.elf" 2>/dev/null || true)
    if [ -n "$U" ]; then echo "  FALLO — símbolos abiertos (el driver sólo puede usar la BIOS):"; echo "$U"; exit 1; fi
    java -cp "$CP" basicplus.frontend.NpackBuild "$OBJ/driver.elf" "$DEST" "$D/nativo/driver_$DRV.npk.$SUF"
    # El ELF se QUEDA (no versionado): con él, una dirección de un pánico dentro
    # del driver se traduce con addr2line (restando dónde se grabó su código).
    mv "$OBJ/driver.elf" "$D/nativo/driver_$DRV.$SUF.elf"
    rm -rf "$OBJ"
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

echo "== el pack =="
mkdir -p "$D/out"
rm -f "$D"/out/*.npk.*
cp "$D"/nativo/driver_"$DRV".npk.* "$D/out/"
java -jar "$ROOT/toolchain/lexer-java/target/basicplus-frontend.jar" --project "$BPB" --backend=mivm 2>&1 | grep -E "error|pack generado" || true
ls -la "$D"/out/*.pack "$D"/nativo/driver_"$DRV".npk.* | awk '{print $5, $9}'
