/*
 * test_crc.c — #305: el CRC POR TROZOS tiene que dar exactamente el mismo valor
 * que el de una tacada. No es un detalle estético: el LS del wire publica el CRC
 * de cada fichero y el IDE lo compara con java.util.zip.CRC32 para saltarse el
 * PUT. Si divergen, el IDE se salta subidas que SÍ hacían falta — y falla en
 * silencio, con un fichero rancio en la placa.
 *
 * Cubre los vectores canónicos, el encadenado por sitios feos, y ficheros de
 * tamaños alrededor del buffer de 256 B de bpvm_fs_crc32 (255/256/257) porque
 * los off-by-one viven justo ahí.
 */
#include "crc32.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int fallos = 0;
static void ok(int c, const char* m) { printf(c?"  ok  : %s\n":"  FAIL: %s\n", m); if(!c) fallos++; }
int main(void) {
    ok(bpvm_crc32((const uint8_t*)"", 0) == 0x00000000u, "vector \"\"");
    ok(bpvm_crc32((const uint8_t*)"123456789", 9) == 0xCBF43926u, "vector \"123456789\"");
    const char* fox = "The quick brown fox jumps over the lazy dog";
    ok(bpvm_crc32((const uint8_t*)fox, strlen(fox)) == 0x414FA339u, "vector del zorro");

    /* Encadenado a mano: partir por sitios feos debe dar lo mismo. */
    uint32_t st = BPVM_CRC32_INIT;
    st = bpvm_crc32_update(st, (const uint8_t*)fox, 1);
    st = bpvm_crc32_update(st, (const uint8_t*)fox+1, 7);
    st = bpvm_crc32_update(st, (const uint8_t*)fox+8, strlen(fox)-8);
    ok(bpvm_crc32_final(st) == 0x414FA339u, "encadenado 1+7+resto == de una tacada");

    /* Y sobre un FICHERO real, por la fachada: tamaños que cruzan el buffer de
     * 256 B de bpvm_fs_crc32 y que no son múltiplos de él. */
    bpvm_fs_register_host();
    const uint32_t TAMS[] = { 0, 1, 255, 256, 257, 1000, 4096, 5001 };
    for (unsigned t = 0; t < sizeof(TAMS)/sizeof(TAMS[0]); t++) {
        uint32_t n = TAMS[t];
        uint8_t* buf = (uint8_t*) malloc(n ? n : 1);
        for (uint32_t i = 0; i < n; i++) buf[i] = (uint8_t)(i * 31 + 7);
        bpvm_fs_write("t_crc.bin", buf, n, 0);
        uint32_t esperado = bpvm_crc32(buf, n), obtenido = 0xDEAD;
        int rc = bpvm_fs_crc32("t_crc.bin", &obtenido);
        char msg[80]; snprintf(msg, sizeof msg, "fichero de %u B: por trozos == entero", n);
        ok(rc == 0 && obtenido == esperado, msg);
        free(buf);
    }
    remove("t_crc.bin");
    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
