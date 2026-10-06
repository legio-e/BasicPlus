# Proyecto ESP-IDF — firmware BasicPlus VM para ESP32-S3 (H4).
# Build:  idf.py set-target esp32s3 && idf.py build
# Flash:  idf.py -p <PUERTO> flash monitor
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# V7/H12 (N1) — el contexto de las natives, uno por TAREA (un `__thread` de 4 bytes: el IDF le da a cada tarea su zona
# TLS; esp32/common/platform_esp32.c): con dos runs, dos natives a la vez no se pisan el suyo. Sólo en las de dos
# núcleos (la S3 y la P4): las de uno van con un run (Eduardo).
idf_build_set_property(COMPILE_OPTIONS "-DBPVM_AOT_POR_TAREA" APPEND)
project(bpvm_esp32)
