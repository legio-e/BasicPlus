#!/bin/sh
# build_lvgl.sh — el PACK DE LA GUI (V7/H6 P3): LVGL + gui.c + el panel, para un destino.
#
#     bash bpstdlib/gui/nativo/src/build_lvgl.sh [imc|p4|arm] [KB del pool de LVGL, 256]
#
# La misma cadena que el pack de SQLite (bpstdlib/sqlite/nativo/src/build_sqlite_rv.sh):
# compilar con la RECETA de la familia, enlazar en BASE 0 con --emit-relocs, cero
# símbolos abiertos, y `NpackBuild` (el realojado lo hace el IDE al grabar). Lo que
# cambia respecto a SQLite:
#   · LVGL son ~450 ficheros (third_party/lvgl/src, con lodepng dentro); se compilan
#     todos y `--gc-sections` se queda con lo que gui.c y el panel alcanzan (las fuentes
#     Montserrat que font_for_px nombra entran enteras: 418 KB, medido en el C6).
#   · gui.c es EL MISMO fichero del host y de las imágenes con LVGL dentro, compilado
#     con -DBPVM_EN_PACK: lo que pedía a la VM lo sirve gui_pack_glue.c desde la BIOS.
#   · gui_pack_publics.c (GENERADO de bpvm_gui.h) es lo que el pack publica bajo 'LVGL'.
#   · el pool de LVGL (TLSF) es .bss del pack: la perilla es el segundo parámetro.
#
# Salida, VERSIONADA al lado (nativo/lvgl.npk.<SUFIJO>), y la lista de enlace en
# lvgl_<sufijo>.link para el oráculo, como manda la casa.
set -e
QUE="${1:-imc}"
POOL_KB="${2:-256}"
HERE="$(cd "$(dirname "$0")" && { pwd -W 2>/dev/null || pwd; })"
ROOT="$(cd "$(dirname "$0")/../../../.." && { pwd -W 2>/dev/null || pwd; })"
BPC="$ROOT/bpgenvm-c"
LVGL="$BPC/third_party/lvgl"
CP="$ROOT/lexer-java/target/classes;$ROOT/pack/target/classes;$ROOT/miVM/target/classes"
ARM_BIN="C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin"
RV_BIN="$HOME/.espressif/tools/riscv32-esp-elf/esp-15.2.0_20251204/riscv32-esp-elf/bin"

case "$QUE" in
  imc) GCC="$RV_BIN/riscv32-esp-elf-gcc.exe"; LD="$RV_BIN/riscv32-esp-elf-ld.exe"; NM="$RV_BIN/riscv32-esp-elf-nm.exe"; SZ="$RV_BIN/riscv32-esp-elf-size.exe"
       ISA="-march=rv32imc_zicsr_zifencei -mabi=ilp32 -mno-relax"; LDX="--no-relax"; DEST=riscv32-imc; SUF=RV32IMC ;;
  p4)  GCC="$RV_BIN/riscv32-esp-elf-gcc.exe"; LD="$RV_BIN/riscv32-esp-elf-ld.exe"; NM="$RV_BIN/riscv32-esp-elf-nm.exe"; SZ="$RV_BIN/riscv32-esp-elf-size.exe"
       ISA="-march=rv32imafc_zicsr_zifencei -mabi=ilp32f -mno-relax"; LDX="--no-relax"; DEST=riscv32-esp-p4; SUF=RISCV ;;
  arm) GCC="$ARM_BIN/arm-none-eabi-gcc.exe"; LD="$ARM_BIN/arm-none-eabi-ld.exe"; NM="$ARM_BIN/arm-none-eabi-nm.exe"; SZ="$ARM_BIN/arm-none-eabi-size.exe"
       ISA="-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16"; LDX=""; DEST=arm-cortex-m33; SUF=ARMV8 ;;
  *) echo "destino: imc | p4 | arm"; exit 1 ;;
esac

OBJ="$HERE/obj_$SUF"
rm -rf "$OBJ"; mkdir -p "$OBJ"
CF="$ISA -fno-jump-tables -Os -ffunction-sections -fdata-sections -fno-builtin -ffreestanding -Wall -Wno-unused-function \
    -DBPVM_EN_PACK -DBPVM_GUI -DBPVM_LVGL -DBPVM_PACK_LV_MEM_KB=$POOL_KB \
    -DLV_CONF_INCLUDE_SIMPLE -DLV_CONF_PATH=$BPC/include/lv_conf.h \
    -I$BPC/include -I$BPC/src -I$LVGL"
LIBGCC=$("$GCC" $ISA -print-libgcc-file-name)

echo "[1/5] LVGL (~450 ficheros; tarda un rato la primera vez)"
n=0
for f in $(find "$LVGL/src" -name "*.c" | sort); do
    o="$OBJ/lv_$(echo "$f" | sed 's|.*/src/||; s|/|__|g; s|\.c$|.o|')"
    "$GCC" $CF -w -c "$f" -o "$o"
    n=$((n+1))
done
echo "      $n objetos"

echo "[2/5] gui.c (el de la VM, bajo BPVM_EN_PACK) + lo publicado + la entrada + el pegamento + el panel"
"$GCC" $CF -c "$BPC/src/gui.c"               -o "$OBJ/gui.o"
"$GCC" $CF -c "$BPC/src/gui_pack_publics.c"  -o "$OBJ/gui_pack_publics.o"
"$GCC" $CF -c "$HERE/lvgl_pack.c"            -o "$OBJ/lvgl_pack.o"
"$GCC" $CF -c "$HERE/gui_pack_glue.c"        -o "$OBJ/gui_pack_glue.o"
"$GCC" $CF -c "$HERE/panel_c5.c"             -o "$OBJ/panel_c5.o"

echo "[3/5] la lista de enlace, en UN sitio (la lee el oráculo)"
{ echo "$OBJ/lvgl_pack.o"; echo "$OBJ/gui_pack_publics.o"; echo "$OBJ/gui.o"; echo "$OBJ/gui_pack_glue.o"; echo "$OBJ/panel_c5.o"
  ls "$OBJ"/lv_*.o; echo "$LIBGCC"; } > "$HERE/lvgl_$SUF.link"

echo "[4/5] enlazando en BASE 0 con --emit-relocs y --gc-sections"
OLDIFS="$IFS"; IFS='
'
# shellcheck disable=SC2046
"$LD" -Ttext=0x0 -Tdata=0x20000000 --emit-relocs --gc-sections $LDX \
      -e bp_pack_init -o "$HERE/lvgl_$SUF.elf" $(cat "$HERE/lvgl_$SUF.link")
IFS="$OLDIFS"

echo "[5/5] sin símbolos sin resolver?"
U=$("$NM" -u "$HERE/lvgl_$SUF.elf" 2>/dev/null || true)
if [ -n "$U" ]; then echo "  FALLO — quedan símbolos abiertos:"; echo "$U"; exit 1; fi
echo "  OK: cero"
"$SZ" -A "$HERE/lvgl_$SUF.elf" | grep -E "^\.(text|rodata|eh_frame|data|sdata|sbss|bss)"

java -cp "$CP" basicplus.frontend.NpackBuild "$HERE/lvgl_$SUF.elf" "$DEST" "$HERE/../lvgl.npk.$SUF"
