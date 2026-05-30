# Proyecto ESP-IDF — firmware BasicPlus VM para ESP32-S3 (H4).
# Build:  idf.py set-target esp32s3 && idf.py build
# Flash:  idf.py -p <PUERTO> flash monitor
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bpvm_esp32)
