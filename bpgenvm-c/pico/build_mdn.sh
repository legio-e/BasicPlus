#!/usr/bin/env bash
# build_mdn.sh — pipeline AotMain → arm-gcc → MdnPack
#
# Uso:
#   ./build_mdn.sh <ModuleName>
#
# Genera <ModuleName>.mdn directamente en samples/out/ (alongside del
# .mod). El IDE al hacer "Run on Pico" detecta automáticamente el .mdn
# y lo sube al FS del Pico junto al .mod.
#
# Requiere:
#   - lexer-java compilado (mvn compile)
#   - arm-none-eabi-gcc en PATH (o ajustar GCC=...)

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
WORK_DIR="$SCRIPT_DIR/mdn_build"     # intermedios (.c, .o)

# Buscar el .bp que declara `module <MOD>` en samples/ y subdirs.
# Esto soporta que el filename NO coincida con el módulo (e.g.
# fibobench.bp con `module Fibo`).
#
# El IDE compila a <bp_dir>/out/<MOD>.mod — el .mdn va al mismo
# out/ para que el IDE lo encuentre alongside del .mod.
BP_FILE=""
while IFS= read -r candidate; do
    # primera línea no vacía con `module XXX` o `module interface XXX`
    name=$(grep -m1 -E "^[[:space:]]*module([[:space:]]+interface)?[[:space:]]+[A-Za-z_][A-Za-z0-9_]*" "$candidate" \
            | sed -E 's/^[[:space:]]*module([[:space:]]+interface)?[[:space:]]+([A-Za-z_][A-Za-z0-9_]*).*/\2/' \
            | head -1)
    if [ "$name" = "$MOD" ]; then
        BP_FILE="$candidate"
        break
    fi
done < <(find "$SAMPLES" -maxdepth 3 -name "*.bp" -type f 2>/dev/null)

if [ -z "$BP_FILE" ]; then
    echo "ERROR: no se encontró ningún .bp con 'module ${MOD}' bajo $SAMPLES" >&2
    exit 3
fi
BP_DIR="$(dirname "$BP_FILE")"
SAMPLES_OUT="$BP_DIR/out"
echo "[bp]   $BP_FILE"
echo "[out]  $SAMPLES_OUT"

GCC="${GCC:-/c/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin/arm-none-eabi-gcc.exe}"

mkdir -p "$WORK_DIR" "$SAMPLES_OUT"

if [ ! -f "$BP_FILE" ]; then
    echo "ERROR: $BP_FILE no existe" >&2
    exit 3
fi

echo "[1/3] AotMain → ${MOD} (modo --mdn)"
java -cp "$LEXER/target/classes" basicplus.frontend.AotMain \
    "$BP_FILE" "$WORK_DIR" --mdn

C_FILE="$WORK_DIR/aot_${MOD}.c"
O_FILE="$WORK_DIR/aot_${MOD}.o"
MDN_FILE="$SAMPLES_OUT/${MOD}.mdn"

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

echo "[3/3] MdnPack → ${MOD}.mdn (en samples/out/)"
java -cp "$LEXER/target/classes" basicplus.frontend.MdnPack \
    "$O_FILE" "$MDN_FILE" "$MOD"

echo ""
echo "=== OK ==="
echo "  .mdn:     $MDN_FILE"
echo ""
echo "Ahora desde el IDE pulsa 'Run on Pico' sobre ${MOD}.bp:"
echo "  el IDE compila → samples/out/${MOD}.mod"
echo "  detecta el .mdn alongside y lo sube junto al .mod"
echo "  el firmware al hacer RUN registra los thunks AOT zero-copy"
echo "  → 66× speedup vs interpretado"
