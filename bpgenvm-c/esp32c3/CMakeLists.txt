# Proyecto ESP-IDF — firmware BasicPlus VM para ESP32-C3 (ENSAYO V6/P1).
# Build:  idf.py set-target esp32c3 && idf.py build
# Flash:  idf.py -p <PUERTO> flash monitor
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# V7/H12, etapas 0 y 1 (4-oct) — la pila de la tarea `run` (esp32/common/repl_esp32.c), 12 KB y no 16: la C3 no tiene
# PSRAM, va justa de RAM interna (H13) y no lleva SQLite; en `run`, el peor caso medido en el P4 fue ~6,5 KB.
idf_build_set_property(COMPILE_OPTIONS "-DESP32_RUN_PILA_BYTES=12288" APPEND)
project(bpvm_esp32c3)
