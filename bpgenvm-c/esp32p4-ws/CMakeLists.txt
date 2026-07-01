# ESP32-P4 Waveshare 4.3" — firmware VM incremental. Bloque grafico (paso 6): LVGL 9.2.2
# vendorizada (third_party/lvgl) como componente IDF, igual que el esp32p4/ de la Function-EV.
cmake_minimum_required(VERSION 3.16)

# LVGL se registra por su esp.cmake (rama if(ESP_PLATFORM)). third_party/ es el contenedor:
# IDF escanea sus subdirectorios y solo lvgl/ tiene CMakeLists. Fijar ANTES de project().
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../third_party")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32p4_ws)

# Flags GLOBALES (a TODOS los componentes, incluida la LVGL) — copiados del esp32p4/:
#  BPVM_BOARD_P4 -> LV_COLOR_DEPTH 16 (RGB565) en include/lv_conf.h. DEBE verlo TODA unidad
#  que incluya lv_conf.h (si unas quedan a 16 y otras a 32, lv_color_t difiere -> corrupcion).
#  -I.../include -> donde vive lv_conf.h (LVGL usa LV_CONF_INCLUDE_SIMPLE).
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_BOARD_P4" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_CURRENT_LIST_DIR}/../include" APPEND)

# GUI de BasicPlus: BPVM_GUI activa los builtins/modelo GUI (gui.c); BPVM_LVGL el render LVGL.
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_GUI" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_LVGL" APPEND)

# Forzar NUESTRA include/lv_conf.h (la misma que host/STM32/Function-EV) sobre la config Kconfig
# de LVGL. En lv_conf_internal.h, LV_CONF_PATH se evalua ANTES que el SKIP -> gana nuestra config.
get_filename_component(BPVM_LV_CONF "${CMAKE_CURRENT_LIST_DIR}/../include/lv_conf.h" ABSOLUTE)
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_PATH=${BPVM_LV_CONF}" APPEND)

# Gui.mod (~24 KB) supera el buffer de PUT del wire por defecto (16 KB). El P4 tiene RAM de
# sobra -> 64 KB (cabe Gui.mod + resources moderados).
idf_build_set_property(COMPILE_OPTIONS "-DV1_PUT_BUF_SIZE=65536" APPEND)
