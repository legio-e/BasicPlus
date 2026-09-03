# Proyecto ESP-IDF — firmware BasicPlus VM para ESP32-C6 (V6/P1.C6 — por el camino del C3).
# Build:  idf.py set-target esp32c6 && idf.py build
# Flash:  idf.py -p <PUERTO> flash monitor
cmake_minimum_required(VERSION 3.16)
# V6/P2 — LVGL 9.2.2 vendorizada (third_party/lvgl) como componente IDF, igual que el
# P4: third_party/ es el contenedor y sólo lvgl/ tiene CMakeLists. ANTES de project().
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../third_party")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bpvm_esp32c6)

# V6/P2 — la GUI de BasicPlus en el C6 (la primera pantalla por SPI). Los mismos
# flags GLOBALES que el P4, a TODOS los componentes incluida la LVGL:
#  - BPVM_BOARD_C6 -> LV_COLOR_DEPTH 16 (RGB565) en include/lv_conf.h;
#  - -I.../include -> donde vive lv_conf.h (LV_CONF_INCLUDE_SIMPLE);
#  - BPVM_GUI (builtins/modelo GUI, gui.c) y BPVM_LVGL (el render y la costura
#    gui_display_st7789.c);
#  - LV_CONF_PATH absoluto para que gane NUESTRA lv_conf.h sobre el Kconfig de LVGL
#    (CONFIG_LV_CONF_SKIP=y por defecto ignoraría lv_conf.h: fuentes, lodepng, RGB565).
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_BOARD_C6" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_CURRENT_LIST_DIR}/../include" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_GUI" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_LVGL" APPEND)
get_filename_component(BPVM_LV_CONF "${CMAKE_CURRENT_LIST_DIR}/../include/lv_conf.h" ABSOLUTE)
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_PATH=${BPVM_LV_CONF}" APPEND)
