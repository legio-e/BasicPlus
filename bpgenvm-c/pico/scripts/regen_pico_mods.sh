#!/usr/bin/env bash
# regen_pico_mods.sh — la stdlib embebida de la Pico/Metro: 15 módulos de bpstdlib
# a /lib, más el Hello.mod de muestra (samples/hello.bp compilado) a /app. Emite
# pico/pico_mods.c + .h con scripts/regen_mods.sh (V6/U4: UN generador, sólo datos).
# Antes eran 16 ficheros *_mod.c + embedded_mods.h + una tabla a mano en main.c.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
FE="$ROOT/lexer-java/target/basicplus-frontend.jar"
[ -f "$FE" ] || { echo "ERROR: falta $FE (mvn -f lexer-java/pom.xml install)" >&2; exit 1; }
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
java -jar "$FE" "$ROOT/samples/hello.bp" --compile "$TMP" --backend=mivm >/dev/null 2>&1
[ -f "$TMP/Hello.mod" ] || { echo "ERROR: el frontend no produjo Hello.mod" >&2; exit 1; }
bash "$ROOT/bpgenvm-c/scripts/regen_mods.sh" pico "$HERE/.." \
    Core Math IO Gpio I2c Spi Uart Pulse Pwm Pico Rtc Adc Wdt Timer Neopixel \
    --extra "/app/Hello.mod=$TMP/Hello.mod"
