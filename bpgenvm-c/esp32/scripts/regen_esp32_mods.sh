#!/usr/bin/env bash
# ============================================================
# regen_esp32_mods.sh — regenera esp32/main/esp32_mods.c embebiendo la
# stdlib core de BasicPlus en el firmware del ESP32-S3.
#
# Gemelo de stm32/scripts/regen_stm32_mods.sh: cada <Name>.mod de bpstdlib/
# se convierte a un array C con `xxd -i` y se agrega a una tabla;
# esp32_mods_install() los pre-instala en /lib del FS (RAM) al arrancar
# (idéntico patrón al firmware de la Pico y del STM32).
#
# Uso:   bash bpgenvm-c/esp32/scripts/regen_esp32_mods.sh
# Tras cambiar la API de un módulo stdlib: recompílalo (IDE), re-ejecuta
# este script y reflashea el firmware (idf.py build flash).
#
# El set embebido = EMBEDDED_CORE_MODS del IDE (FrmMain.java): los módulos
# que el IDE da por pre-instalados en el device y por tanto NO sube.
# ============================================================
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
STDLIB="$(cd "$HERE/../../.." && pwd)/bpstdlib"
OUT="$HERE/../main/esp32_mods.c"

# Mismo conjunto que EMBEDDED_CORE_MODS en el IDE. Nombre tal cual (la
# resolución de imports busca "<Module>.mod" preservando mayúsculas).
MODS=(Core Math IO Gpio I2c Spi Uart Pulse Pwm Pico Rtc Adc Wdt Timer)

for m in "${MODS[@]}"; do
    if [ ! -f "$STDLIB/$m.mod" ]; then
        echo "ERROR: falta $STDLIB/$m.mod (compila la stdlib primero)" >&2
        exit 1
    fi
done

var_of() { echo "$1" | tr '[:upper:]' '[:lower:]'; }

{
    echo "/*"
    echo " * esp32_mods.c — GENERADO por scripts/regen_esp32_mods.sh. NO EDITAR A MANO."
    echo " *"
    echo " * stdlib core de BasicPlus embebida en flash. esp32_mods_install() la"
    echo " * pre-instala en /lib del FS (RAM) al boot, igual que EMBEDDED_CORE_MODS"
    echo " * de la Pico / stm32_mods.c → los programas que importan stdlib resuelven"
    echo " * sin que el IDE tenga que subir las dependencias."
    echo " */"
    echo '#include "esp32_mods.h"'
    echo '#include "fs.h"'
    echo '#include <stdint.h>'
    echo ""
    for m in "${MODS[@]}"; do
        var="$(var_of "$m")_mod"
        # xxd: `unsigned char X[] = {...}; unsigned int X_len = N;`
        # → lo hacemos `static const` para no exportar símbolos.
        xxd -i -n "$var" "$STDLIB/$m.mod" | sed 's/^unsigned /static const unsigned /'
        echo ""
    done
    echo "typedef struct { const char* path; const unsigned char* data; unsigned len; } mod_entry_t;"
    echo ""
    echo "static const mod_entry_t s_mods[] = {"
    for m in "${MODS[@]}"; do
        var="$(var_of "$m")_mod"
        printf '    { "/lib/%s.mod", %s, %s_len },\n' "$m" "$var" "$var"
    done
    echo "};"
    echo ""
    echo "void esp32_mods_install(void) {"
    echo "    const uint8_t* d; uint32_t sz;"
    echo "    unsigned n = (unsigned) (sizeof(s_mods) / sizeof(s_mods[0]));"
    echo "    for (unsigned i = 0; i < n; i++) {"
    echo "        /* No sobreescribas si ya está (p.ej. el usuario subió una versión). */"
    echo "        if (fs_get(s_mods[i].path, &d, &sz) != 0) {"
    echo "            fs_put(s_mods[i].path, s_mods[i].data, s_mods[i].len);"
    echo "        }"
    echo "    }"
    echo "}"
} > "$OUT"

echo "generado: $OUT"
echo "modulos embebidos: ${MODS[*]}"
