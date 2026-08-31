# Proyecto ESP-IDF — firmware BasicPlus VM para ESP32-C3 (ENSAYO V6/P1).
# Build:  idf.py set-target esp32c3 && idf.py build
# Flash:  idf.py -p <PUERTO> flash monitor
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bpvm_esp32c3)
