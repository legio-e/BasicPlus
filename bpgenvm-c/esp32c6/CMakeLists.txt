# Proyecto ESP-IDF — firmware BasicPlus VM para ESP32-C6 (V6/P1.C6 — por el camino del C3).
# Build:  idf.py set-target esp32c6 && idf.py build
# Flash:  idf.py -p <PUERTO> flash monitor
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bpvm_esp32c6)
