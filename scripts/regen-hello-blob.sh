#!/usr/bin/env bash
# ============================================================================
# regen-hello-blob.sh — regenera el Hello.mod EMBEBIDO en los firmwares.
#
# POR QUÉ EXISTE: la cabecera de hello_mod.c decía «regenerar con
# scripts/regen_embedded_mods.sh» y ese script NO EXISTÍA. Resultado: cuando en
# #271 se regeneraron los blobs de la stdlib, Hello se quedó fuera —no es
# stdlib, es un sample— y nadie lo miró. Siguió siendo un .mod v5, anterior al
# ensanchado de refs 4→8B, en las TRES familias que lo llevan. En H13 (3-ago) la
# placa lo rechazó al ejecutarlo: el gate de ABI (#284) hizo su trabajo, pero la
# imagen no debería llevar algo que no puede correr.
#
# Los tres ficheros eran DISTINTOS entre sí (12124, 12818 y 16638 bytes, de tres
# fechas diferentes): cada uno se generó a mano en su momento. Ahora salen los
# tres de aquí, de la misma fuente y a la vez.
#
#   bash scripts/regen-hello-blob.sh
#
# Después hay que RECOMPILAR cada firmware — el blob es código C.
# ============================================================================
set -euo pipefail

RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
FE="$RAIZ/lexer-java/target/basicplus-frontend.jar"
FUENTE="$RAIZ/samples/hello.bp"
WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT

DESTINOS="bpgenvm-c/pico/hello_mod.c
bpgenvm-c/esp32/main/hello_mod.c
bpgenvm-c/esp32p4/main/hello_mod.c"

[ -f "$FE" ]     || { echo "FALTA $FE"; exit 1; }
[ -f "$FUENTE" ] || { echo "FALTA $FUENTE"; exit 1; }

java -jar "$FE" "$FUENTE" --compile "$WORK" --backend=mivm > "$WORK/fe.log" 2>&1
MOD="$WORK/Hello.mod"
[ -f "$MOD" ] || { echo "hello.bp no compila:"; grep -E '^\[[0-9]+:' "$WORK/fe.log" | head -3; exit 1; }

# El .mod tiene que ser v6: si sale v5 es que el compilador que hay delante es
# viejo, y embeber eso es justo el problema que este script existe para evitar.
if [ "$(head -c 4 "$MOD")" != "MOD6" ]; then
    echo "el .mod generado NO es v6 — compilador rancio, abortando"; exit 1
fi

BYTES=$(stat -c%s "$MOD")
echo "  Hello.mod: $BYTES bytes (v6)"

for rel in $DESTINOS; do
    dst="$RAIZ/$rel"
    [ -f "$dst" ] || { echo "  NO ESTA $rel — se salta"; continue; }
    {
        printf '/*\n * hello_mod.c — bytecode .mod de samples/hello.bp embebido en flash.\n'
        printf ' * GENERADO por scripts/regen-hello-blob.sh. NO editar a mano: el arreglo\n'
        printf ' * manual muere en la siguiente regeneración.\n */\n'
        printf '#include <stdint.h>\n'
        grep -q 'embedded_mods.h' "$dst" && printf '#include "embedded_mods.h"\n'
        printf 'const uint8_t hello_mod[] = {\n'
        xxd -i < "$MOD" | sed 's/^/  /'
        printf '\n};\nconst unsigned int hello_mod_len = %s;\n' "$BYTES"
    } > "$dst.nuevo"
    mv "$dst.nuevo" "$dst"
    echo "  regenerado $rel"
done

echo
echo "  AHORA HAY QUE RECOMPILAR los firmwares: el blob es código C."
