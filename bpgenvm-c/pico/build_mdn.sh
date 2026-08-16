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
# El frontend usa `basicplus.pack.*` (PackException) desde V5, y este script se
# quedo con el classpath de antes: fallaba con ClassNotFoundException nada mas
# arrancar. Nadie se entero porque el camino que se usa a diario es el del IDE
# (BpIde/AotBuild.java) — dos caminos al mismo artefacto y uno se pudrio.
# Ojo al armarlo: Git Bash traduce una ruta Unix suelta a formato Windows al
# pasarla a java, pero NO una lista separada por ';' — hay que traducirlas a
# mano con cygpath o java no encuentra ni la clase principal.
if command -v cygpath >/dev/null 2>&1; then
    CP="$(cygpath -w "$LEXER/target/classes");$(cygpath -w "$PM_ROOT/pack/target/classes")"
else
    CP="$LEXER/target/classes:$PM_ROOT/pack/target/classes"
fi
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

echo "[1/4] AotMain → ${MOD} (modo --mdn)"
java -cp "$CP" basicplus.frontend.AotMain \
    "$BP_FILE" "$WORK_DIR" --mdn

C_FILE="$WORK_DIR/aot_${MOD}.c"
O_FILE="$WORK_DIR/aot_${MOD}.o"
MDN_FILE="$SAMPLES_OUT/${MOD}.mdn"

if [ ! -f "$C_FILE" ]; then
    echo "ERROR: AotMain no produjo $C_FILE" >&2
    echo "       ¿el .bp tiene 'function native ...'?" >&2
    exit 4
fi

echo "[2/4] arm-none-eabi-gcc → ${MOD}.o (PIC Thumb-2)"
"$GCC" -mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16 \
    -fpic -fno-jump-tables -Os \
    -I"$PM_ROOT/bpgenvm-c/include" \
    -I"$PM_ROOT/bpgenvm-c/src" \
    -c "$C_FILE" -o "$O_FILE"

# #428 — PASO DE ENLACE. Sin el, un literal de cadena en una `native` acaba en
# `.rodata`, fuera de lo unico que un `.mdn` se lleva (`.text`), y MdnPack lo
# rechaza — con razon: en la placa seria un puntero a ninguna parte. El guion
# mete los literales DENTRO del codigo, y el enlace a direccion 0 resuelve las
# referencias dejandolas PC-relativas, asi que el resultado sigue siendo
# cargable en cualquier direccion (comprobado enlazando a dos bases distintas:
# el .text sale byte-identico).
#
# El pipeline del IDE (BpIde/AotBuild.java) hace lo MISMO y con el MISMO guion.
# Si uno de los dos cambia, el otro tambien: son dos caminos al mismo artefacto.
ELF_FILE="$WORK_DIR/aot_${MOD}.elf"
echo "[3/4] enlace → ${MOD}.elf (literales dentro del .text)"
"$GCC" -nostdlib -nostartfiles \
    -Wl,--unresolved-symbols=ignore-all -Wl,-e,0 \
    -Wl,-T,"$PM_ROOT/bpgenvm-c/aot/mdn.ld" \
    "$O_FILE" -o "$ELF_FILE"

echo "[4/4] MdnPack → ${MOD}.mdn (en samples/out/)"
java -cp "$CP" basicplus.frontend.MdnPack \
    "$ELF_FILE" "$MDN_FILE" "$MOD"

echo ""
echo "=== OK ==="
echo "  .mdn:     $MDN_FILE"
echo ""
echo "Ahora desde el IDE pulsa 'Run on Pico' sobre ${MOD}.bp:"
echo "  el IDE compila → samples/out/${MOD}.mod"
echo "  detecta el .mdn alongside y lo sube junto al .mod"
echo "  el firmware al hacer RUN registra los thunks AOT zero-copy"
echo "  → 66× speedup vs interpretado"
