#!/usr/bin/env bash
# regen_stm32_mods.sh — la stdlib embebida del STM32 (Nucleo y Discovery): 14 módulos
# de bpstdlib a /lib. Emite stm32/port/stm32_mods.c + .h con scripts/regen_mods.sh
# (V6/U4: UN generador, ficheros de sólo datos; el bucle vive en src/bpvm_mods.c).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
bash "$HERE/../../scripts/regen_mods.sh" stm32 "$HERE/../port" \
    Core Math IO Gpio I2c Spi Uart Pulse Pwm Pico Rtc Adc Wdt Timer
