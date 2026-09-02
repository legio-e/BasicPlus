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
    bpvm_mem_region_t r[1] = {{ NULL, 139264, 280032, 0, 17588, "SRAM interna" }};
    bpvm_mem_cfg_t cfg = { 128u * 1024u, 64u * 1024u, 0, NULL };
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
    bpvm_mem_region_t r[1] = {{ NULL, 270336, 338368, 0, 26564, "SRAM interna" }};
    bpvm_mem_cfg_t cfg = { 160u * 1024u, 64u * 1024u, 0, NULL };
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
        { (unsigned char*) 0x20014BF8, 431432, 431432, 0, 65536, "SRAM interna" },
        { (unsigned char*) 0x11000000, 8u * 1024u * 1024u, 8u * 1024u * 1024u, 1, 0, "PSRAM" },
    };
    bpvm_mem_cfg_t cfg = { 0, 64u * 1024u, 2u * 1024u * 1024u, "SQLite" };
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
    bpvm_mem_region_t r[1] = {{ (unsigned char*) 0x20014BF8, 431432, 431432, 0, 65536, "SRAM interna" }};
    bpvm_mem_cfg_t cfg = { 0, 64u * 1024u, 0, NULL };
    bpvm_mem_plan_t p;
    bpvm_mem_plan(r, 1, &cfg, &p);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 365896,           "Pico sin PSRAM: 365896 B = los 357 KB del log (%u)", (unsigned) p.bytes);
    CHECK(!strcmp(p.limita, "el margen del sistema"),          "Pico: manda el margen (la region es un solo bloque)");
}

static void caso_suelo(void) {
    /* Una placa imaginaria con 90 KB contiguos y 40 KB de margen: quedan 50 < 64. */
    bpvm_mem_region_t r[1] = {{ NULL, 90u * 1024u, 90u * 1024u, 0, 40u * 1024u, "SRAM" }};
    bpvm_mem_cfg_t cfg = { 128u * 1024u, 64u * 1024u, 0, NULL };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_res_t res = bpvm_mem_plan(r, 1, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    CHECK(res == BPVM_MEM_NO_CABE && p.bytes == 0,             "suelo: no se arranca a medias");
    CHECK(strstr(s, "NO CABE") && strstr(s, "suelo 64 KB"),    "suelo: y se dice con los numeros: %s", s);
    CHECK(bpvm_mem_plan(NULL, 0, &cfg, &p) == BPVM_MEM_SIN_REGIONES, "sin regiones: se dice, no se cae");
}

static void caso_exclusiva_con_margen(void) {
    /* La forma del P4: PSRAM exclusiva, SQLite aparte (no entra aqui) y 4 MiB que se
     * dejan LIBRES para el display. Numeros redondos: el fixture medido se anade
     * cuando el P4 de la mesa de el suyo (los medidos no se sustituyen, se suman). */
    bpvm_mem_region_t r[1] = {{ NULL, 30u * 1024u * 1024u, 30u * 1024u * 1024u, 1, 4u * 1024u * 1024u, "PSRAM" }};
    bpvm_mem_cfg_t cfg = { 0, 2u * 1024u * 1024u, 0, NULL };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_plan(r, 1, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    CHECK(p.bytes == 26u * 1024u * 1024u,                      "exclusiva con margen: 30 - 4 = 26 MiB (%u KB)", (unsigned)(p.bytes / 1024u));
    CHECK(!strcmp(p.limita, "el margen de la región"),         "exclusiva con margen: lo dice: %s", s);
}

static void caso_p4(void) {
    /* MEDIDO el 2-sep: psram: libre 32765 KB | mayor 32256 KB | bloques: 1 libres, 3 usados
     * La imagen anterior daba 28668 KiB (= 32765 - 4096 alineado a pagina). La primera
     * version del planificador dio 28160 (restaba el display al CONTIGUO): este caso
     * es el que lo cazo. El margen va contra el TOTAL. */
    bpvm_mem_region_t r[1] = {{ NULL, 32256u * 1024u, 32765u * 1024u, 1, 4u * 1024u * 1024u, "PSRAM" }};
    bpvm_mem_cfg_t cfg = { 0, 2u * 1024u * 1024u, 0, NULL };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_plan(r, 1, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    size_t pagina = p.bytes & ~((size_t) 4095u);              /* el P4 alinea a pagina al tomar */
    CHECK(p.bytes == 28669u * 1024u,                          "P4: techo 32765 - 4096 = 28669 KiB (%u)", (unsigned)(p.bytes / 1024u));
    CHECK(pagina == 28668u * 1024u,                            "P4: alineado a pagina = 28668 KiB, IGUAL que la imagen anterior");
    CHECK(!strcmp(p.limita, "el margen de la región"),         "P4: manda el margen de la region (el display), no el contiguo: %s", s);
}

static void caso_stm32(void) {
    /* U6.11 — la memoria mas simple del parque: el array estatico de 512 KB del port
     * (stm32_repl.c), exclusivo de la VM y SIN margen (el margen de esta familia lo
     * exige el enlazador: _Min_Heap_Size + _Min_Stack_Size detras del estatico, o no
     * enlaza). Objetivo 0 = el array entero. Nucleo y Discovery comparten el numero. */
    bpvm_mem_region_t r[1] = {{ (unsigned char*) 0x20000000, 512u * 1024u, 512u * 1024u, 1, 0, "SRAM estática" }};
    bpvm_mem_cfg_t cfg = { 0, 64u * 1024u, 0, NULL };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_plan(r, 1, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 512u * 1024u,     "STM32: los 512 KB del array, enteros (%u KB)", (unsigned)(p.bytes / 1024u));
    CHECK(!strcmp(p.limita, "la región"),                      "STM32: manda la region (ni contiguo ni margen)");
    CHECK(strstr(s, "512 KB en SRAM est") != NULL && strstr(s, "todo lo que deja la regi") != NULL,
          "STM32: la linea: %s", s);
}

int main(void) {
    printf("=== test_mem: el planificador contra las placas medidas ===\n");
    caso_c3(); caso_s3(); caso_metro_psram(); caso_pico_sram(); caso_suelo(); caso_exclusiva_con_margen(); caso_p4(); caso_stm32();
    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
