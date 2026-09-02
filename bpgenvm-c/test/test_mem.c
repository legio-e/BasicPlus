/* test_mem.c — el planificador de memoria contra los números MEDIDOS en placa.
 *
 * Es el arnés antes que el artefacto (U3.24): cada caso de aquí es una placa real
 * con sus números de verdad, tomados del log de arranque el 31-ago y el 2-sep. Si
 * el planificador se toca y alguna placa saldría distinta, esto lo dice en el host
 * en un segundo, sin flashear nada.
 *
 * Los cuatro casos cubren las dos clases de memoria (exclusiva/compartida), las
 * dos restricciones de la compartida (contiguo/margen), la reserva con nombre
 * delante de la exclusiva, y el suelo.
 *
 *   make test-mem
 */
#include "bpvm_mem.h"
#include <stdio.h>
#include <string.h>

static int fallos = 0;
#define CHECK(cond, ...) do { if (cond) printf("  ok  : " __VA_ARGS__); \
                              else { printf("  FAIL: " __VA_ARGS__); fallos++; } printf("\n"); } while (0)

static void caso_c3(void) {
    /* heap: libre 280032 | mayor 139264   (P1.C3.3, y otra vez el 2-sep) */
    bpvm_mem_region_t r[1] = {{ NULL, 139264, 280032, 0, "SRAM interna" }};
    bpvm_mem_cfg_t cfg = { 128u * 1024u, 17588, 64u * 1024u, 0, NULL };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_plan(r, 1, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 131072,          "C3: 128 KB, el objetivo (%u)", (unsigned) p.bytes);
    CHECK(p.techo == 139264 && !strcmp(p.limita, "el bloque contiguo"),
          "C3: techo 136 KB y manda el bloque contiguo (no el margen)");
    CHECK(strstr(s, "objetivo 128, techo 136 por el bloque contiguo") != NULL, "C3: la linea: %s", s);
}

static void caso_s3(void) {
    /* vm: ... DRAM interna libre 338368 (bloque mayor 270336)   (U6.4) */
    bpvm_mem_region_t r[1] = {{ NULL, 270336, 338368, 0, "SRAM interna" }};
    bpvm_mem_cfg_t cfg = { 160u * 1024u, 26564, 64u * 1024u, 0, NULL };
    bpvm_mem_plan_t p;
    bpvm_mem_plan(r, 1, &cfg, &p);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 163840,          "S3: 160 KB, el objetivo (%u)", (unsigned) p.bytes);
    CHECK(p.techo == 270336 && !strcmp(p.limita, "el bloque contiguo"),
          "S3: techo 264 KB por el bloque contiguo — cabria mas y NO se coge (Eduardo)");
}

static void caso_metro_psram(void) {
    /* psram: 8 MB usable | bd: reservada (SQLite=2) -> 2048 KB | vm: heap en PSRAM 6 MB
     * Y la SRAM se ofrece TAMBIEN: el planificador debe preferir la exclusiva. */
    bpvm_mem_region_t r[2] = {
        { (unsigned char*) 0x20014BF8, 431432, 431432, 0, "SRAM interna" },
        { (unsigned char*) 0x11000000, 8u * 1024u * 1024u, 8u * 1024u * 1024u, 1, "PSRAM" },
    };
    bpvm_mem_cfg_t cfg = { 0, 65536, 64u * 1024u, 2u * 1024u * 1024u, "SQLite" };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_plan(r, 2, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    CHECK(p.idx == 1,                                          "Metro: elige la PSRAM aunque este segunda");
    CHECK(p.reserva_bytes == 2u * 1024u * 1024u,               "Metro: aparta los 2 MB de SQLite delante");
    CHECK(p.bytes == 6u * 1024u * 1024u,                       "Metro: 6 MB para la VM (%u KB)", (unsigned)(p.bytes / 1024u));
    CHECK(strstr(s, "SQLite 2048 KB delante") != NULL,         "Metro: la linea cita la reserva: %s", s);
}

static void caso_pico_sram(void) {
    /* vm: SRAM interna 357 KB ... libre para malloc: 64 KB   (Metro con psram=0, 31-ago)
     * La region es [end, PACK_RAM_SRAM_BASE) = 431432 B; el margen de 64 KB lo resta el plan.
     * 431432 - 65536 = 365896 = 357 KB, que es lo que la placa dice. Sin objetivo: todo. */
    bpvm_mem_region_t r[1] = {{ (unsigned char*) 0x20014BF8, 431432, 431432, 0, "SRAM interna" }};
    bpvm_mem_cfg_t cfg = { 0, 65536, 64u * 1024u, 0, NULL };
    bpvm_mem_plan_t p;
    bpvm_mem_plan(r, 1, &cfg, &p);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 365896,           "Pico sin PSRAM: 365896 B = los 357 KB del log (%u)", (unsigned) p.bytes);
    CHECK(!strcmp(p.limita, "el margen del sistema"),          "Pico: manda el margen (la region es un solo bloque)");
}

static void caso_suelo(void) {
    /* Una placa imaginaria con 90 KB contiguos y 40 KB de margen: quedan 50 < 64. */
    bpvm_mem_region_t r[1] = {{ NULL, 90u * 1024u, 90u * 1024u, 0, "SRAM" }};
    bpvm_mem_cfg_t cfg = { 128u * 1024u, 40u * 1024u, 64u * 1024u, 0, NULL };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_res_t res = bpvm_mem_plan(r, 1, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    CHECK(res == BPVM_MEM_NO_CABE && p.bytes == 0,             "suelo: no se arranca a medias");
    CHECK(strstr(s, "NO CABE") && strstr(s, "suelo 64 KB"),    "suelo: y se dice con los numeros: %s", s);
    CHECK(bpvm_mem_plan(NULL, 0, &cfg, &p) == BPVM_MEM_SIN_REGIONES, "sin regiones: se dice, no se cae");
}

int main(void) {
    printf("=== test_mem: el planificador contra las placas medidas ===\n");
    caso_c3(); caso_s3(); caso_metro_psram(); caso_pico_sram(); caso_suelo();
    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
