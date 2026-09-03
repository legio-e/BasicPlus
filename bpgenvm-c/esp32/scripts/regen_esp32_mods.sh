#!/usr/bin/env bash
# regen_esp32_mods.sh — la stdlib embebida de la familia ESP32 (S3, C3, P4): 14 módulos
# de bpstdlib a /lib. Emite esp32/common/esp32_mods.c + .h con scripts/regen_mods.sh
# (V6/U4: UN generador, ficheros de sólo datos; el bucle vive en src/bpvm_mods.c).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
bash "$HERE/../../scripts/regen_mods.sh" esp32 "$HERE/../common" \
    Core Math IO Gpio I2c Spi Uart Pulse Pwm Pico Rtc Adc Wdt Timer
