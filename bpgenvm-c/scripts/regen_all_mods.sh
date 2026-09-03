#!/usr/bin/env bash
# regen_all_mods.sh — regenera la stdlib embebida de las TRES familias desde bpstdlib
# (V6/U4: un solo generador, scripts/regen_mods.sh; cada wrapper sólo dice su lista).
# El Hello.mod de muestra de la Pico va dentro de regen_pico_mods.sh.
#
# Tras cambiar la stdlib: recompílala (IDE/frontend), ejecuta esto y RECONSTRUYE los
# seis firmwares (Pico: cmake --build pico/build; ESP32: idf.py build en cada
# proyecto; STM32: CubeIDE). Lo que no se compila no se sabe si está roto.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"   # bpgenvm-c/
echo "== Pico =="
bash "$ROOT/pico/scripts/regen_pico_mods.sh"
echo "== STM32 =="
bash "$ROOT/stm32/scripts/regen_stm32_mods.sh"
echo "== ESP32 =="
bash "$ROOT/esp32/scripts/regen_esp32_mods.sh"
echo ""
echo "OK: stdlib embebida regenerada en las 3 familias. Recuerda RECOMPILAR cada firmware."
