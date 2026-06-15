#!/usr/bin/env bash
# ============================================================
# regen_pico_mods.sh — regenera los pico/*_mod.c (stdlib embebida del Pico)
# desde bpstdlib. Gemelo de regen_stm32/esp32_mods.sh, pero el Pico usa un
# .c POR modulo (formato `xxd -i -n <mod>_mod`, declarados en embedded_mods.h)
# en vez de un fichero monolitico con tabla.
#
# Uso:  bash bpgenvm-c/pico/scripts/regen_pico_mods.sh
# Tras cambiar la stdlib: recompilala (IDE/frontend), ejecuta esto (o el
# maestro scripts/regen_all_mods.sh) y reconstruye build/ (ninja bpvm_pico).
#
# El Pico embebe Core + el zoo de HW INCLUIDO Neopixel (del Metro), pero NO
# Math/IO (se suben por el IDE si hacen falta) — set historico de este puerto.
# ============================================================
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
STDLIB="$(cd "$HERE/../../.." && pwd)/bpstdlib"
PICODIR="$(cd "$HERE/.." && pwd)"

MODS=(Core Gpio I2c Spi Uart Pulse Pwm Pico Rtc Adc Wdt Timer Neopixel)

for Mod in "${MODS[@]}"; do
    if [ ! -f "$STDLIB/$Mod.mod" ]; then
        echo "ERROR: falta $STDLIB/$Mod.mod (compila la stdlib primero)" >&2
        exit 1
    fi
done

var_of() { echo "$1" | tr '[:upper:]' '[:lower:]'; }

for Mod in "${MODS[@]}"; do
    var="$(var_of "$Mod")_mod"
    out="$PICODIR/$(var_of "$Mod")_mod.c"
    {
        printf '/*\n'
        printf ' * %s.c - bytecode .mod de bpstdlib/%s.bp embebido en flash.\n' "$var" "$Mod"
        printf ' * Generado con `xxd -i -n %s bpstdlib/%s.mod`.\n' "$var" "$Mod"
        printf ' */\n'
        printf '#include <stdint.h>\n'
        printf '#include "embedded_mods.h"\n'
        xxd -i -n "$var" "$STDLIB/$Mod.mod" | sed 's/^unsigned char /const uint8_t /; s/^unsigned int /const unsigned int /'
    } > "$out"
    echo "generado: pico/$(var_of "$Mod")_mod.c"
done

echo "modulos Pico: ${MODS[*]}"
