# ESP32-P4 — VM.3 (wire v1 TCP) + gráficos V3.
# G4: LVGL 9.2.2 vendorizada (third_party/lvgl, gitignored) como componente IDF.
cmake_minimum_required(VERSION 3.16)

# LVGL se registra sola por su esp.cmake (rama if(ESP_PLATFORM)). third_party/ es el
# contenedor: IDF escanea sus subdirectorios y solo lvgl/ tiene CMakeLists (SDL2/ se
# ignora). Debe fijarse ANTES de project().
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../third_party")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# El binario se llama como la PLACA, no como una fase del desarrollo: se publica
# como bpvm_esp32p4.bin y sirve a las dos P4 (kit y Waveshare; el panel sale del
# ENV, #311). Se llamo "bpvm_p4_netlog" mientras el enlace con el IDE era por
# Ethernet; ese canal esta apagado desde V4 (ver BPVM_P4_NETLOG en main.c).
project(bpvm_esp32p4)

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

# G6 (RETIRADO en #338) — este override subía el buffer de PUT porque Gui.mod
# (~24 KB) no cabía entero en el buffer del wire. Eso era ANTES del streaming: desde
# #294 el IDE sube por trozos, así que el tamaño del fichero dejó de importar y el
# override sobraba —y encima su comentario decía 64 KB mientras ponía 20 KB—. El P4
# usa ahora el mismo default que el S3 (8 KB, repl_esp32.c): una familia, un número.
# Si algún día hiciera falta subirlo aquí, el guardián de compilación de repl_esp32.c
# sigue marcando el suelo.
