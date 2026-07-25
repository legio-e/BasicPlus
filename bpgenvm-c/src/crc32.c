/*
 * crc32.c — CRC-32 IEEE 802.3 (== java.util.zip.CRC32 / zlib). Ver crc32.h.
 *
 * Implementación bit a bit (sin tabla): footprint mínimo, MCU-friendly. Los
 * ficheros del FS son pequeños (.mod de pocos KB), así que la diferencia con la
 * versión por tabla es irrelevante.
 */
#include "crc32.h"

/* #305 — el bucle, ahora reutilizable por trozos. `state` entra y sale SIN el
 * xor final: encadenar update() sobre trozos consecutivos da exactamente lo
 * mismo que una sola pasada sobre el fichero entero. */
uint32_t bpvm_crc32_update(uint32_t state, const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        state ^= (uint32_t) data[i];
        for (int k = 0; k < 8; k++)
            state = (state & 1u) ? (state >> 1) ^ 0xEDB88320u : (state >> 1);
    }
    return state;
}

uint32_t bpvm_crc32_final(uint32_t state) {
    return state ^ 0xFFFFFFFFu;
}

/* De una tacada = init + un update + final. Se mantiene porque es lo natural
 * cuando los bytes YA están en RAM (un blob embebido, un buffer del wire). */
uint32_t bpvm_crc32(const uint8_t* data, size_t len) {
    return bpvm_crc32_final(bpvm_crc32_update(BPVM_CRC32_INIT, data, len));
}
