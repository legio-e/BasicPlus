#!/usr/bin/env bash
# build_mdn.sh — pipeline AotMain → arm-gcc → MdnPack
#
# Uso:
#   ./build_mdn.sh <ModuleName>
#
# Genera <ModuleName>.mdn en ./mdn_build/ a partir del .bp en samples/.
# Requiere:
#   - lexer-java compilado (mvn compile)
#   - arm-none-eabi-gcc en PATH (o ajustar GCC=...)
#
# El .mdn resultante se puede subir al Pico vía IDE (Upload→) a
# /app/<ModuleName>.mdn. El firmware (con la fase D activa) lo cargará
# automáticamente alongside del .mod.

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Uso: $0 <ModuleName>" >&2
    exit 2
fi

MOD="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PM_ROOT="$SCRIPT_DIR/../.."
LEXER="$PM_ROOT/lexer-java"
SAMPLES="$PM_ROOT/samples"
OUT_DIR="$SCRIPT_DIR/mdn_build"
BP_FILE="$SAMPLES/${MOD}.bp"

GCC="${GCC:-/c/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin/arm-none-eabi-gcc.exe}"

mkdir -p "$OUT_DIR"

if [ ! -f "$BP_FILE" ]; then
    echo "ERROR: $BP_FILE no existe" >&2
    exit 3
fi

echo "[1/3] AotMain → ${MOD} (modo --mdn)"
java -cp "$LEXER/target/classes" basicplus.frontend.AotMain \
    "$BP_FILE" "$OUT_DIR" --mdn

C_FILE="$OUT_DIR/aot_${MOD}.c"
O_FILE="$OUT_DIR/aot_${MOD}.o"
MDN_FILE="$OUT_DIR/${MOD}.mdn"

if [ ! -f "$C_FILE" ]; then
    echo "ERROR: AotMain no produjo $C_FILE" >&2
    echo "       ¿el .bp tiene 'function native ...'?" >&2
    exit 4
fi

echo "[2/3] arm-none-eabi-gcc → ${MOD}.o (PIC Thumb-2)"
"$GCC" -mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16 \
    -fpic -fno-jump-tables -Os \
    -I"$PM_ROOT/bpgenvm-c/include" \
    -I"$PM_ROOT/bpgenvm-c/src" \
    -c "$C_FILE" -o "$O_FILE"

echo "[3/3] MdnPack → ${MOD}.mdn"
java -cp "$LEXER/target/classes" basicplus.frontend.MdnPack \
    "$O_FILE" "$MDN_FILE" "$MOD"

echo ""
echo "=== OK ==="
echo "  C source: $C_FILE"
echo "  Object:   $O_FILE"
echo "  .mdn:     $MDN_FILE"
echo ""
echo "Subir al Pico con el IDE (Upload→):"
echo "  $MDN_FILE  →  /app/${MOD}.mdn"
