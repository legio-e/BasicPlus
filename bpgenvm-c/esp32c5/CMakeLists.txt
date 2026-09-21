# Proyecto ESP-IDF — firmware BasicPlus VM para ESP32-C5 (V7/H11, 1.ª etapa: SIN LVGL).
# Build:  idf.py set-target esp32c5 && idf.py build
# Flash:  idf.py -p <PUERTO> flash
#
# Waveshare ESP32-C5-Touch-LCD-2.8 (Eduardo, 21-sep): «en dos etapas: 1.º la imagen sin LVGL
# y luego, sobre ella, montar el LVGL en un pack e ir probando». Así que esta imagen es la
# de una placa NO gráfica más (el camino del C3/C6: familia activada, no construida); la
# pantalla y el táctil llegan por el pack (H6), no por aquí.
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bpvm_esp32c5)
