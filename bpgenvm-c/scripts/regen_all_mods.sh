#!/usr/bin/env bash
# ============================================================
# regen_all_mods.sh — regenera los blobs de stdlib embebidos en los TRES
# firmwares (Pico, STM32, ESP32) desde bpstdlib/*.mod, de un solo tiro.
#
# Ejecutar tras recompilar la stdlib (IDE/frontend) para mantener los .mod
# embebidos sincronizados con bpstdlib y evitar el ".mod skew" (que un
# firmware lleve una version del bytecode distinta de la que emite el
# compilador). Despues hay que RECOMPILAR cada firmware:
#   Pico:  cd pico/build && ninja bpvm_pico        (-> bpvm_pico.uf2)
#   STM32: recompilar en STM32CubeIDE
#   ESP32: cd esp32 && idf.py build
#
# Uso:  bash bpgenvm-c/scripts/regen_all_mods.sh
# ============================================================
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
echo "OK: blobs de stdlib regenerados en las 3 familias desde bpstdlib."
echo "Recuerda RECOMPILAR cada firmware (ver cabecera de este script)."
