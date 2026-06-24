# ESP32-P4 — VM.3 (wire v1 TCP) + gráficos V3.
# G4: LVGL 9.2.2 vendorizada (third_party/lvgl, gitignored) como componente IDF.
cmake_minimum_required(VERSION 3.16)

# LVGL se registra sola por su esp.cmake (rama if(ESP_PLATFORM)). third_party/ es el
# contenedor: IDF escanea sus subdirectorios y solo lvgl/ tiene CMakeLists (SDL2/ se
# ignora). Debe fijarse ANTES de project().
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../third_party")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bpvm_p4_netlog)

# G4 — flags GLOBALES (a TODOS los componentes, incluida la LVGL):
#  - BPVM_BOARD_P4 -> LV_COLOR_DEPTH 16 (RGB565) en include/lv_conf.h, como el DK2.
#    DEBE verlo TODA unidad que incluya lv_conf.h: si unas quedan a 16 y otras a 32,
#    lv_color_t tendría distinto tamaño -> corrupción/cuelgues.
#  - -I.../include -> donde vive lv_conf.h (LVGL usa LV_CONF_INCLUDE_SIMPLE, i.e.
#    #include "lv_conf.h" resuelto por el include path).
# (BPVM_LVGL —la costura gui.c + builtins GUI— se activará en G6, junto a gui.c.)
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_BOARD_P4" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_CURRENT_LIST_DIR}/../include" APPEND)
