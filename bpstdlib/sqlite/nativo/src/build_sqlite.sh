#!/bin/sh
# build_sqlite.sh — el PACK DE VERDAD, con el mismo pipeline que validó el mini.
#
# Diferencias con `build.sh` (el mini), y son sólo dos:
#
#   1. Ninguna: la amalgama se compila aquí, igual que en el hermano de RISC-V.
#      Hasta el 19-ago dependía de un `A/sqlite3.o` conservado en `notas/`, y al
#      borrarse esa carpeta el pack habría dejado de poder reconstruirse.
#   2. Hacen falta los 18 __aeabi_* de libgcc. El mini no los necesitaba: no
#      hacía aritmética de 64 bits ni de coma flotante. SQLite sí.
#
# ─── EL FICHERO .link Y POR QUÉ EXISTE ───
#
# `pack.py verify` hace de oráculo VOLVIENDO A ENLAZAR con `ld` en la dirección
# destino, y comparando byte a byte. Para que eso signifique algo, el oráculo
# tiene que enlazar EXACTAMENTE los mismos objetos que el artefacto. Si las dos
# listas viven en sitios distintos, el día que una cambie el oráculo seguirá
# diciendo IDÉNTICO — comparando otro programa consigo mismo. Un PAR falso.
#
# Por eso la lista se escribe UNA vez, aquí, en `sqlite.link`, y de ahi lo lee el oraculo.
set -e

BIN="C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin"
GCC="$BIN/arm-none-eabi-gcc.exe"
LD="$BIN/arm-none-eabi-ld.exe"
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
# Las MISMAS banderas del firmware. Que la ABI de coma flotante coincida no es
# opcional: un desajuste NO da error de enlace, da números mal en silencio.
CF="-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16 \
    -fno-jump-tables -Os -ffunction-sections -fdata-sections \
    -fno-builtin -ffreestanding -Wall -Wextra -I$INC -I$SQLH"

# libgcc DE ESTAS BANDERAS, preguntado al compilador en vez de escrito a mano:
# hay una copia por multilib (thumb/v8-m.main+fp/softfp) y coger la de otra
# combinación enlaza sin protestar.
LIBGCC=$("$GCC" -mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16 \
         -print-libgcc-file-name)

echo "[1/5] la amalgama de SQLite (tarda ~15 s la primera vez)"
[ -f "$HERE/sqlite3.o" ] || "$GCC" $CF $DEF -c "$AMALGAMA" -o "$HERE/sqlite3.o"

echo "[2/5] compilando el pegamento (shim de libc + entrada) y el VFS"
"$GCC" $CF -c "$HERE/sqlite_pack.c" -o "$HERE/sqlite_pack.o"
"$GCC" $CF -c "$HERE/vfs_bp.c"      -o "$HERE/vfs_bp.o"
"$GCC" $CF -c "$HERE/sqlite_shim.c" -o "$HERE/sqlite_shim.o"

# La lista de enlace, en UN sitio. La usan el enlace de abajo y el oráculo.
{ echo "$HERE/sqlite_pack.o"; echo "$HERE/vfs_bp.o"; echo "$HERE/sqlite_shim.o"
  echo "$HERE/sqlite3.o"; echo "$LIBGCC"; } \
    > "$HERE/sqlite.link"

echo "[3/5] enlazando en BASE 0 con --emit-relocs"
# El troceado, SÓLO por saltos de línea: la ruta de libgcc lleva espacios
# ("Program Files") y con el IFS de serie se partiría en cuatro argumentos.
OLDIFS="$IFS"; IFS='
'
# shellcheck disable=SC2046
"$LD" -Ttext=0x0 -Tdata=0x20000000 --emit-relocs \
      -e bp_pack_init -o "$HERE/sqlite.elf" $(cat "$HERE/sqlite.link")
IFS="$OLDIFS"

echo "[4/5] sin simbolos sin resolver?"
U=$("$BIN/arm-none-eabi-nm.exe" -u "$HERE/sqlite.elf" 2>/dev/null || true)
if [ -n "$U" ]; then
    echo "  FALLO — quedan símbolos abiertos:"; echo "$U"; exit 1
fi
echo "  OK: cero"

echo "[5/5] tamaños"
"$BIN/arm-none-eabi-size.exe" -A "$HERE/sqlite.elf" | grep -E "^\.(text|rodata|data|bss)"
