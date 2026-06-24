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

# G6 — GUI de BasicPlus: BPVM_GUI activa los builtins/modelo GUI (gui.c); BPVM_LVGL el
# render LVGL. gui.c entra en SRCS del main; la costura bpvm_gui_disp_* la implementa
# gui_display_dsi.c (reutiliza el panel de G3 + el táctil de G5).
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_GUI" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_LVGL" APPEND)

# G6 — forzar NUESTRA include/lv_conf.h (la misma que host/STM32) sobre la config
# Kconfig de LVGL. El componente LVGL de IDF trae CONFIG_LV_CONF_SKIP=y por defecto:
# IGNORA lv_conf.h y usa Kconfig (solo montserrat_14, sin lodepng) -> gui.c no enlaza
# (fuentes 12-48 + lv_lodepng_init). En lv_conf_internal.h, LV_CONF_PATH se evalúa
# ANTES que el SKIP -> gana nuestra config (fuentes, lodepng, RGB565 via BPVM_BOARD_P4).
# Ruta absoluta normalizada (sin '..') para el #include de LVGL.
get_filename_component(BPVM_LV_CONF "${CMAKE_CURRENT_LIST_DIR}/../include/lv_conf.h" ABSOLUTE)
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_PATH=${BPVM_LV_CONF}" APPEND)

# G6 — Gui.mod (~24 KB) supera el buffer de PUT del wire (16 KB por defecto). El P4
# tiene RAM de sobra -> 64 KB (cabe Gui.mod + apps/resources moderados). El S3 (con
# menos RAM) mantiene su default vía el #ifndef de repl_esp32.c.
idf_build_set_property(COMPILE_OPTIONS "-DV1_PUT_BUF_SIZE=65536" APPEND)
