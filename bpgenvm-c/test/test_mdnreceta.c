/*
 * test_mdnreceta.c — V7/H2 (#441): EL GATE DE LA RECETA del cargador de `.mdn`.
 *
 * El PC no tiene receta (CRC 0, gate apagado), así que este test compila
 * mdn_loader.c APARTE con `-DBPVM_AOT_HOST_RECETA_FORZADA=<literal>` — la
 * costura que bpvm_aot_recetas.h deja sólo para esto — y fabrica en memoria
 * blobs `.mdn` de la arquitectura del host (arch = MDN_HOST_ARCH, sin símbolos,
 * 4 bytes de código) para que pasen el gate de arquitectura y lleguen al de la
 * receta. Lo que se comprueba es la escalera:
 *
 *   v1 (20 B, sin huella)         → MDN_OK        (se carga a ciegas, con aviso)
 *   v2 con la huella de MI receta → MDN_OK
 *   v2 con otra huella            → MDN_ERR_RECETA (misma arch, otros flags: #440)
 *   v2 con huella 0               → MDN_ERR_RECETA (una huella inventada no vale)
 *   v4                            → MDN_ERR_VERSION (la v3, P2b.3, es valida: la marca DIRECTA)
 *   v2 truncado a 22 B            → MDN_ERR_TRUNCATED (la cabecera v2 son 24)
 *   v1 de 20 B justos + 0 código  → MDN_OK        (sizeof(mdn_header_t) NO vale de mínimo)
 *
 * Y que la receta publicada (`bpvm_mdn_host_receta_crc`) es el CRC32 de los
 * bytes del literal — el mismo que estampa AotRecetas.crc en Java.
 */
#include "mdn_loader.h"
#include "mdn_format.h"
#include "bpvm_aot_recetas.h"
#include "crc32.h"
#include <stdio.h>
#include <string.h>

static int fallos = 0;
static void ok(int c, const char* m) { printf(c ? "  ok  : %s\n" : "  FAIL: %s\n", m); if (!c) fallos++; }

uint16_t bpvm_mdn_host_arch(void);

/* Un blob mínimo: cabecera (v1 o v2) + 4 bytes de código, alineado a 4. */
static uint32_t g_buf[16];
static size_t blob(uint16_t version, uint32_t flags_crc, uint32_t code_size) {
    uint8_t* b = (uint8_t*) g_buf;
    memset(g_buf, 0, sizeof g_buf);
    b[0] = 'M'; b[1] = 'D'; b[2] = 'N'; b[3] = 0;
    b[4] = (uint8_t) version; b[5] = (uint8_t) (version >> 8);
    b[6] = (uint8_t) MDN_ABI_VERSION; b[7] = (uint8_t) (MDN_ABI_VERSION >> 8);
    memcpy(b + 8, &code_size, 4);
    uint32_t syms = 0; memcpy(b + 12, &syms, 4);
    uint32_t arch = bpvm_mdn_host_arch(); memcpy(b + 16, &arch, 4);
    uint32_t hdr = version >= 2 ? 24 : 20;
    if (version >= 2) memcpy(b + 20, &flags_crc, 4);
    return hdr + code_size;
}

int main(void) {
    printf("=== V7/H2 (#441): el gate de la receta del .mdn ===\n");
    const char* mia = BPVM_AOT_HOST_RECETA;
    uint32_t crc = bpvm_crc32((const uint8_t*) mia, strlen(mia));
    char m[200];

    snprintf(m, sizeof m, "la receta forzada se publica: '%s' (%08x)", bpvm_mdn_host_receta_nombre(), (unsigned) crc);
    ok(bpvm_mdn_host_receta_crc() == crc && crc != 0 && !strcmp(bpvm_mdn_host_receta_nombre(), "FORZADA"), m);
    /* El vector clásico del CRC32 IEEE: lo mismo que java.util.zip.CRC32. */
    ok(bpvm_crc32((const uint8_t*) "123456789", 9) == 0xCBF43926u, "bpvm_crc32 es el CRC32 IEEE (\"123456789\" -> cbf43926)");

    if (bpvm_mdn_host_arch() == MDN_ARCH_NONE) {
        printf("  (arquitectura del host desconocida: el gate de arch no deja llegar al de receta)\n");
        return 0;
    }

    size_t n;
    n = blob(1, 0, 4);
    ok(bpvm_load_mdn(NULL, (const uint8_t*) g_buf, n) == MDN_OK, "v1 sin huella: se carga (a ciegas, con aviso en el log)");
    n = blob(2, crc, 4);
    ok(bpvm_load_mdn(NULL, (const uint8_t*) g_buf, n) == MDN_OK, "v2 con la huella de mi receta: se carga");
    n = blob(2, crc ^ 0x00000100u, 4);
    ok(bpvm_load_mdn(NULL, (const uint8_t*) g_buf, n) == MDN_ERR_RECETA, "v2 con otra huella: MDN_ERR_RECETA (misma arch, otros flags)");
    n = blob(2, 0, 4);
    ok(bpvm_load_mdn(NULL, (const uint8_t*) g_buf, n) == MDN_ERR_RECETA, "v2 con huella 0: MDN_ERR_RECETA (una huella inventada no vale)");
    n = blob(4, crc, 4);
    ok(bpvm_load_mdn(NULL, (const uint8_t*) g_buf, n) == MDN_ERR_VERSION, "v4: MDN_ERR_VERSION (la v3 es valida desde P2b.3)");
    n = blob(2, crc, 4);
    ok(bpvm_load_mdn(NULL, (const uint8_t*) g_buf, 22) == MDN_ERR_TRUNCATED, "v2 truncado a 22 B: MDN_ERR_TRUNCATED");
    n = blob(1, 0, 0);
    ok(n == 20 && bpvm_load_mdn(NULL, (const uint8_t*) g_buf, n) == MDN_OK, "v1 de 20 B justos: no es 'truncado' (el minimo es la cabecera v1, no sizeof)");
    n = blob(2, crc, 4);
    ok(bpvm_load_mdn(NULL, (const uint8_t*) g_buf, n - 1) == MDN_ERR_TRUNCATED, "v2 al que le falta un byte de codigo: MDN_ERR_TRUNCATED");

    printf(fallos ? "[status=ROJO] %d fallo(s)\n" : "[status=OK] %d fallo(s)\n", fallos);
    return fallos ? 1 : 0;
}
