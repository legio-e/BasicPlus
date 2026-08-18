#!/usr/bin/env bash
# ============================================================
# regen_hello_blob.sh — regenera el `hello_mod.c` del Pico desde `samples/hello.bp`.
#
# POR QUÉ EXISTE. La cabecera de esos ficheros decía «GENERADO por
# scripts/regen-hello-blob.sh» y ESE SCRIPT NO EXISTÍA: un puntero muerto que
# hacía pasar por generado algo que se mantenía a mano. Consecuencia medida el
# 18-ago (#427, punto 8):
#
#   pico    4034 B  ·  MOD6  ← el ÚNICO vivo (se preinstala en /app/Hello.mod)
#   esp32   4034 B  ·  MOD6  ← el .c ni siquiera entra en SRCS
#   esp32p4 4034 B  ·  MOD6  ← idem
#   stm32   1720 B  ·  MOD5  ← fósil de otra época, y encima SÍ se compila
#
# Y lo que el censo no decía: **el vivo también estaba rancio** — 4034 B
# embebidos contra los 3965 que emite el compilador de hoy. O sea que el `.mod`
# skew que los `regen_*_mods.sh` evitan para la stdlib, aquí no lo evitaba
# nadie. El arreglo no es tocar un blob: es que exista el generador.
#
# Uso:  bash bpgenvm-c/scripts/regen_hello_blob.sh
# Después: reconstruir el firmware que lo lleve (sólo el Pico lo ejecuta).
#
# ============================================================
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"          # raíz del repo
FE="$ROOT/lexer-java/target/basicplus-frontend.jar"
SRC="$ROOT/samples/hello.bp"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

[ -f "$FE" ]  || { echo "ERROR: falta $FE (mvn -f lexer-java/pom.xml install)" >&2; exit 1; }
[ -f "$SRC" ] || { echo "ERROR: falta $SRC" >&2; exit 1; }

echo "compilando $SRC ..."
java -jar "$FE" "$SRC" --compile "$TMP" --backend=mivm >/dev/null 2>&1
MOD="$TMP/Hello.mod"
[ -f "$MOD" ] || { echo "ERROR: el frontend no produjo Hello.mod" >&2; exit 1; }

# Cada familia con SU forma: el Pico incluye `embedded_mods.h` (declara el
# extern) y las demás no. El tipo es `const uint8_t` en las tres modernas; el
# STM32 lo declara como `unsigned char` en su bpvm_app.c, así que se respeta.
emitir() {   # $1 = fichero destino, $2 = incluir embedded_mods.h (si/no), $3 = tipo
    local out="$1" inc="$2" tipo="$3"
    {
        printf '/*\n'
        printf ' * hello_mod.c — bytecode .mod de samples/hello.bp embebido en flash.\n'
        printf ' * GENERADO por scripts/regen_hello_blob.sh. NO editar a mano: el arreglo\n'
        printf ' * manual muere en la siguiente regeneración.\n'
        printf ' */\n'
        printf '#include <stdint.h>\n'
        [ "$inc" = "si" ] && printf '#include "embedded_mods.h"\n'
        xxd -i -n hello_mod "$MOD" \
            | sed "s/^unsigned char /const $tipo /; s/^unsigned int /const unsigned int /"
    } > "$out"
    printf '  %-44s %s B\n' "${out#$ROOT/}" "$(stat -c%s "$MOD")"
}

# SOLO EL PICO. Las otras tres copias se BORRARON el 18-ago (decisión de
# Eduardo: «se puede borrar, ya no lo utilizo nunca»): ESP32 y P4 ni las
# compilaban, y la del STM32 se compilaba para un `bpvm_app_run_hello()` que no
# llamaba nadie — gastaba flash para nada. Aquí sólo queda el que se usa: el
# Pico lo preinstala como /app/Hello.mod.
emitir "$ROOT/bpgenvm-c/pico/hello_mod.c"         si  "uint8_t"

echo "OK: Hello del Pico regenerado. Reconstruye su firmware (ninja bpvm_pico)."
