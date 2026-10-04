# Proyecto ESP-IDF — firmware BasicPlus VM para ESP32-C6 (V6/P1.C6 — por el camino del C3).
# Build:  idf.py set-target esp32c6 && idf.py build
# Flash:  idf.py -p <PUERTO> flash monitor
cmake_minimum_required(VERSION 3.16)
# V7/H6.b (25-sep) — LA GUI POR EL PACK, como la C5. Hasta hoy esta imagen llevaba LVGL 9.2.2
# vendorizada (EXTRA_COMPONENT_DIRS), gui.c y su driver ST7789 por `esp_lcd` (V6/P2): 664 KB de
# flash y 107 KB de RAM que pagaba todo programa, tuviera pantalla o no. Ahora lleva los 83
# builtins de Gui (BPVM_GUI) y los TRAMPOLINES de src/gui_pack.c (BPVM_GUI_PACK), que sirven cada
# función desde la tabla que publica el pack 'LVGL' (en esta placa, GuiMini: el de pantallas
# pequeñas); el driver de la pantalla es otro pack (DriverSpi, con la placa en las líneas del ENV).
# Sin pack, `Gui.screen()` lanza «GUI: esta placa no tiene el pack 'LVGL' grabado», con nombre.
# Decidido con Eduardo el 25-sep: V7_IDEAS §H6.14. El punto de restauración: dist/firmware-v7-25sep.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_GUI" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_GUI_PACK" APPEND)
# V7/H12, etapas 0 y 1 (4-oct) — la pila de la tarea `run` (esp32/common/repl_esp32.c), 10 KB y no 16: la C6 no tiene
# PSRAM, va justa de RAM interna (H13) y no lleva SQLite. Medido en su tanda de V7: `run` gasta como mucho ~5,1 KB (con la
# GUI: GuiShot). Con 12 KB, y `main` en 16, durante un RUN la RAM interna libre llegaba a 404 B (GuiArbolG2, sin memoria
# al enlazar); con 10, y `main` en 12 (sdkconfig.defaults), los 22 KB de antes de H12 (`main` y el `io` de cada RUN).
idf_build_set_property(COMPILE_OPTIONS "-DESP32_RUN_PILA_BYTES=10240" APPEND)
project(bpvm_esp32c6)
