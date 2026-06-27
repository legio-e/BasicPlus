/*
 * crc32.c — CRC-32 IEEE 802.3 (== java.util.zip.CRC32 / zlib). Ver crc32.h.
 *
 * Implementación bit a bit (sin tabla): footprint mínimo, MCU-friendly. Los
 * ficheros del FS son pequeños (.mod de pocos KB), así que la diferencia con la
 * versión por tabla es irrelevante.
 */
#include "crc32.h"

uint32_t bpvm_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint32_t) data[i];
        for (int k = 0; k < 8; k++)
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
    }
    return crc ^ 0xFFFFFFFFu;
}
