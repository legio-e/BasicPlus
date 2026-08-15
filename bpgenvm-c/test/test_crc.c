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
     * 256 B de bpvm_fs_crc32 y que no son múltiplos de él.
     *
     * #398 — AHORA SON TRES CAMINOS Y LOS TRES TIENEN QUE COINCIDIR:
     *   1. el CRC del buffer entero en memoria (la verdad de referencia);
     *   2. el CAMINO NUEVO: `crc32` del backend, que abre el fichero UNA vez;
     *   3. el CAMINO VIEJO: el bucle de `read_at` en trozos de 256 B, que es lo
     *      que hacía la fachada y aquí se reproduce como ORÁCULO.
     *
     * El 3 no es redundante: el camino nuevo se añadió por RENDIMIENTO (en la
     * P4, el CRC era el 99 % del refresco del árbol: 5432 aperturas de fichero
     * para 1,3 MB de tarjeta), y una optimización que cambie el VALOR haría que
     * el IDE dejara de subir ficheros que sí cambiaron, en silencio. Por eso el
     * que sustituye se compara contra el sustituido, y no sólo consigo mismo. */
    bpvm_fs_register_host();
    const uint32_t TAMS[] = { 0, 1, 255, 256, 257, 511, 512, 513, 1000, 4096, 5001 };
    for (unsigned t = 0; t < sizeof(TAMS)/sizeof(TAMS[0]); t++) {
        uint32_t n = TAMS[t];
        uint8_t* buf = (uint8_t*) malloc(n ? n : 1);
        for (uint32_t i = 0; i < n; i++) buf[i] = (uint8_t)(i * 31 + 7);
        bpvm_fs_write("t_crc.bin", buf, n, 0);
        uint32_t esperado = bpvm_crc32(buf, n), obtenido = 0xDEAD;
        int rc = bpvm_fs_crc32("t_crc.bin", &obtenido);
        char msg[96]; snprintf(msg, sizeof msg, "fichero de %u B: por trozos == entero", n);
        ok(rc == 0 && obtenido == esperado, msg);

        /* El oráculo: el bucle de 256 B por `read_at`, tal cual era. */
        uint32_t st2 = BPVM_CRC32_INIT, off = 0;
        uint8_t  t256[256];
        int malo = 0;
        while (off < n) {
            long got = bpvm_fs_read_at("t_crc.bin", off, t256, sizeof t256);
            if (got <= 0) { malo = 1; break; }
            st2 = bpvm_crc32_update(st2, t256, (size_t) got);
            off += (uint32_t) got;
        }
        snprintf(msg, sizeof msg, "fichero de %u B: backend == bucle de read_at (#398)", n);
        ok(!malo && bpvm_crc32_final(st2) == obtenido, msg);
        free(buf);
    }
    remove("t_crc.bin");
    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
