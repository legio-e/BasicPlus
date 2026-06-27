/*
 * crc32.h — CRC-32 IEEE 802.3, idéntico a java.util.zip.CRC32 / zlib.
 *
 * Polinomio 0xEDB88320 (reflejado), init 0xFFFFFFFF, xorout 0xFFFFFFFF.
 * Paso 4 (cierre V3): el firmware reporta el CRC de cada fichero del FS en el
 * LS del wire y el IDE lo compara contra el CRC local (java.util.zip.CRC32) →
 * skip-PUT por contenido REAL del device, no por tamaño ni por la memoria de
 * sesión del IDE. Por eso el algoritmo TIENE que dar el MISMO valor en C y Java.
 *
 * Vectores canónicos de verificación:
 *   ""                                          -> 0x00000000
 *   "123456789"                                 -> 0xCBF43926  (check value CRC-32)
 *   "The quick brown fox jumps over the lazy dog" -> 0x414FA339
 */
#ifndef BPVM_CRC32_H
#define BPVM_CRC32_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t bpvm_crc32(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_CRC32_H */
