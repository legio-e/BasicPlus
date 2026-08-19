#!/bin/sh
# build_sqlite_rv.sh — el pack de SQLite para RISC-V (ESP32-P4). V5/H7.
#
# Hermano de `build_sqlite.sh`, y las diferencias son TODAS del silicio — están
# aquí y no repartidas:
#
#   1. Toolchain del IDF (riscv32-esp-elf), no el de ARM.
#   2. `-march`/`-mabi` LEÍDOS DEL FIRMWARE, no elegidos: el ELF del P4 dice
#      "RVC, single-float ABI" ⇒ rv32imafc + ilp32f. Un desajuste de ABI de coma
#      flotante NO da error de enlace: da números mal, en silencio.
#   3. `-mno-relax` y `--no-relax`. Sin esto el enlazador acorta instrucciones
#      según la distancia, o sea que el LAYOUT dependería de la dirección — y
#      todo el modelo del pack es "un layout fijo que se parchea". Con
#      relajación, realojar dejaría de ser parchear.
#
# Lo que NO cambia: el `.link` en un sitio, que es de donde lo lee el oráculo de
# `pack.py`. Si el oráculo enlazara su propia lista, el día que una de las dos
# cambiara seguiría diciendo IDÉNTICO comparando otro programa consigo mismo.
set -e

RV="$HOME/.espressif/tools/riscv32-esp-elf/esp-15.2.0_20251204/riscv32-esp-elf/bin"
GCC="$RV/riscv32-esp-elf-gcc.exe"
LD="$RV/riscv32-esp-elf-ld.exe"
# ─── RUTAS ───
# Derivadas de la POSICIÓN del script, no absolutas: esto vive dentro del repo
# (`bpstdlib/sqlite/nativo/src/`) y tiene que funcionar en cualquier clon.
# `pwd -W` da la ruta en forma Windows (C:/...): hace falta porque estas rutas
# acaban escritas en el `.link`, que lo lee un Python NATIVO — Git Bash traduce
# las rutas POSIX al invocar un .exe, pero no al escribirlas en un fichero.
HERE="$(cd "$(dirname "$0")" && { pwd -W 2>/dev/null || pwd; })"
ROOT="$(cd "$(dirname "$0")/../../../.." && { pwd -W 2>/dev/null || pwd; })"
INC="$ROOT/bpgenvm-c/include"

# La amalgama es de TERCEROS y NO está en el repo (sqlite.org, 3.53.4): se baja
# y se descomprime ahí. Comprobarlo aquí evita el fallo mudo de más abajo.
AMALGAMA="$ROOT/bpgenvm-c/build/sql/sqlite3.c"
SQLH="$(dirname "$AMALGAMA")"                # sqlite3.h, al lado
if [ ! -f "$AMALGAMA" ]; then
    echo "FALTA la amalgama de SQLite: $AMALGAMA"
    echo "  baja sqlite-amalgamation-3530400.zip de sqlite.org y descomprimela ahi"
    exit 1
fi

# Las opciones de SQLite, las MISMAS en las dos arquitecturas. No supuestas: se
# leyeron del .o de ARM con
#     strings sqlite3.o | grep -E "^(THREADSAFE|TEMP_STORE|ENABLE_)"
DEF="-DSQLITE_OS_OTHER=1 -DSQLITE_THREADSAFE=0 -DSQLITE_ENABLE_MEMSYS5"
DEF="$DEF -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_TEMP_STORE=3"
ISA="-march=rv32imafc_zicsr_zifencei -mabi=ilp32f"
CF="$ISA -mno-relax -fno-jump-tables -Os -ffunction-sections -fdata-sections \
    -fno-builtin -ffreestanding -Wall -Wextra -I$INC -I$SQLH"

echo "[1/5] la amalgama de SQLite (tarda ~15 s)"
[ -f "$HERE/sqlite3_rv.o" ] || "$GCC" $CF $DEF -c "$AMALGAMA" -o "$HERE/sqlite3_rv.o"

echo "[2/5] el pegamento (shim de libc + entrada) y el VFS"
"$GCC" $CF -c "$HERE/sqlite_pack.c"  -o "$HERE/sqlite_pack_rv.o"
"$GCC" $CF -c "$HERE/vfs_bp.c"       -o "$HERE/vfs_bp_rv.o"
"$GCC" $CF -c "$HERE/sqlite_shim.c"  -o "$HERE/sqlite_shim_rv.o"

# libgcc DE ESTAS BANDERAS, preguntado al compilador en vez de escrito a mano:
# hay una copia por multilib (rv32imafc.../ilp32f) y coger la de otra
# combinación enlaza sin protestar.
LIBGCC=$("$GCC" $ISA -print-libgcc-file-name)
echo "      libgcc: $LIBGCC"

echo "[3/5] la lista de enlace, en UN sitio (la lee el oraculo)"
{ echo "$HERE/sqlite_pack_rv.o"; echo "$HERE/vfs_bp_rv.o"; echo "$HERE/sqlite_shim_rv.o"
  echo "$HERE/sqlite3_rv.o";     echo "$LIBGCC"; } > "$HERE/sqlite_rv.link"

echo "[4/5] enlazando en BASE 0 con --emit-relocs"
OLDIFS="$IFS"; IFS='
'
# shellcheck disable=SC2046
"$LD" -Ttext=0x0 -Tdata=0x20000000 --emit-relocs --no-relax \
      -e bp_pack_init -o "$HERE/sqlite_rv.elf" $(cat "$HERE/sqlite_rv.link")
IFS="$OLDIFS"

echo "[5/5] sin simbolos sin resolver?"
U=$("$RV/riscv32-esp-elf-nm.exe" -u "$HERE/sqlite_rv.elf" 2>/dev/null || true)
if [ -n "$U" ]; then
    echo "  FALLO — quedan símbolos abiertos:"; echo "$U"; exit 1
fi
echo "  OK: cero"

"$RV/riscv32-esp-elf-size.exe" -A "$HERE/sqlite_rv.elf" \
    | grep -E "^\.(text|rodata|eh_frame|data|sdata|sbss|bss)"
