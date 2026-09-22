# Proyecto ESP-IDF — firmware BasicPlus VM para ESP32-C5 (V7/H11, 1.ª etapa: SIN LVGL).
# Build:  idf.py set-target esp32c5 && idf.py build
# Flash:  idf.py -p <PUERTO> flash
#
# Waveshare ESP32-C5-Touch-LCD-2.8 (Eduardo, 21-sep): «en dos etapas: 1.º la imagen sin LVGL
# y luego, sobre ella, montar el LVGL en un pack e ir probando». Así que esta imagen es la
# de una placa NO gráfica más (el camino del C3/C6: familia activada, no construida); la
# pantalla y el táctil llegan por el pack (H6), no por aquí.
cmake_minimum_required(VERSION 3.16)
# V7/H6 P2 — LA GUI POR EL PACK: la imagen lleva los 83 builtins de Gui (BPVM_GUI, 4,7 KB) y
# los TRAMPOLINES de src/gui_pack.c (BPVM_GUI_PACK, generados de bpvm_gui.h), que sirven cada
# función desde la tabla que el pack 'LVGL' publica. Ni gui.c ni LVGL van dentro: sin pack,
# `Gui.screen()` lanza «GUI: esta placa no tiene el pack 'LVGL' grabado», con nombre.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_GUI" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_GUI_PACK" APPEND)
project(bpvm_esp32c5)
