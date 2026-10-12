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
#include "bpvm_boot.h"   /* #691: BPVM_BOOT_REASON_MAX, donde tiene que caber el motivo (sólo la macro) */
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

static void caso_c6(void) {
    /* MEDIDO el 3-sep (P1.C6.3, sin pantalla): heap: libre 410988 | mayor 385024; margen
     * 16932. Con 192 KB de objetivo cabia de sobra: mandaba el objetivo, no el techo. */
    bpvm_mem_region_t r[1] = {{ NULL, 385024, 410988, 0, 16932, "SRAM interna" }};
    bpvm_mem_cfg_t cfg = { 192u * 1024u, 64u * 1024u, 0, NULL };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_plan(r, 1, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 196608,          "C6 sin GUI: 192 KB, el objetivo (%u)", (unsigned) p.bytes);
    CHECK(p.techo == 385024 && !strcmp(p.limita, "el bloque contiguo"),
          "C6 sin GUI: techo 376 KB por el bloque contiguo");
    /* Y CON LA PANTALLA (P2.1): LVGL se lleva ~108 KB estaticos (libre 303164 | mayor 278528)
     * y hasta ~104 KB en marcha (dos pasadas: 92928 y 103824; el margen es el maximo). La
     * imagen del C6 con GUI pone objetivo 128 (el reparto del C3). */
    bpvm_mem_region_t g[1] = {{ NULL, 278528, 303164, 0, 103824, "SRAM interna" }};
    bpvm_mem_cfg_t cg = { 128u * 1024u, 64u * 1024u, 0, NULL };
    bpvm_mem_plan(g, 1, &cg, &p);
    bpvm_mem_plan_str(&p, g, &cg, s, sizeof s);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 131072,          "C6 con GUI: 128 KB, el objetivo (%u)", (unsigned) p.bytes);
    CHECK(p.techo == 303164 - 103824 && !strcmp(p.limita, "el margen del sistema"),
          "C6 con GUI: techo 194 KB por el margen del sistema (303164-103824=199340), no por el contiguo");
    CHECK(strstr(s, "objetivo 128, techo 194 por el margen del sistema") != NULL, "C6 con GUI: la linea: %s", s);
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

static void caso_s3_psram(void) {
    /* MEDIDO el 1-oct (V7/H7, el resto de U6): la S3 N16R8 con la imagen de ae3ca338 y el protocolo de
     * tools/medir_margen.py (reset, PUT, dos RUN de Bench, LIST). Con CONFIG_SPIRAM la región es la PSRAM,
     * EXCLUSIVA, con 512 KB de margen (esp32/common/main.c: prudente, no medido —en la S3 nadie más pide PSRAM—):
     *   vm: 7677 KB en PSRAM (todo lo que deja el margen de la región)
     *   vm: DRAM interna libre 8386308->316407 B (bloque mayor 8257536->221184 B) | margen 26564
     *       (los de ANTES de tomar son de la PSRAM, los de después de la DRAM interna: la línea los mezcla)
     *   mem: DRAM interna libre 305311 B | MINIMO HISTORICO 297535 B (bloque mayor 221184 B)
     * 8386308 - 524288 = 7862020 B, y la placa toma en KB enteros: 7677 KB. La SRAM, sin la VM, sobrada.
     * El caso de la S3 sin PSRAM (caso_s3, 30-ago) se queda: es la otra rama de la misma imagen. */
    bpvm_mem_region_t r[1] = {{ NULL, 8257536, 8386308, 1, 512u * 1024u, "PSRAM" }};
    bpvm_mem_cfg_t cfg = { 0, 64u * 1024u, 0, NULL };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_plan(r, 1, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 7862020u,        "S3 con PSRAM: 8386308 - 512 KB = 7862020 B (%u)", (unsigned) p.bytes);
    CHECK((p.bytes & ~1023u) / 1024u == 7677u,                 "S3 con PSRAM: en KB enteros, los 7677 KB del log");
    CHECK(!strcmp(p.limita, "el margen de la región"),         "S3 con PSRAM: manda el margen de la region, no el contiguo: %s", s);
    CHECK(strstr(s, "en PSRAM") != NULL && strstr(s, "todo lo que deja el margen de la regi") != NULL,
          "S3 con PSRAM: la linea: %s", s);
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

static void caso_discovery(void) {
    /* U6.12 — la Discovery (3008 KB de RAM) deja de heredar el techo de la Nucleo.
     * Hoy (G5.b, 26-sep) el array son 1792 KB y `packRam=768` aparta delante el bloque
     * de los motores nativos: la VM se queda 1024. Es la línea de su arranque del 30-sep:
     * «vm: 1024 KB en SRAM estática (todo lo que deja la región) | packs 768 KB delante».
     * Misma forma que la Nucleo: exclusiva, sin margen (el enlazador lo pone). */
    bpvm_mem_region_t r[1] = {{ (unsigned char*) 0x20000000, 1792u * 1024u, 1792u * 1024u, 1, 0, "SRAM estática" }};
    bpvm_mem_cfg_t cfg = { 0, 64u * 1024u, 768u * 1024u, "packs" };
    bpvm_mem_plan_t p; char s[160];
    bpvm_mem_plan(r, 1, &cfg, &p);
    bpvm_mem_plan_str(&p, r, &cfg, s, sizeof s);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 1024u * 1024u && p.reserva_bytes == 768u * 1024u,
          "Discovery: 1024 KB para la VM y 768 delante (%u + %u KB)", (unsigned)(p.bytes / 1024u),
          (unsigned)(p.reserva_bytes / 1024u));
    CHECK(strstr(s, "vm: 1024 KB en SRAM est") != NULL && strstr(s, "| packs 768 KB delante") != NULL,
          "Discovery: la linea: %s", s);
}

static void caso_vmheap(void) {
    /* V7/H7 (30-sep) — `vmheap=N` (KB): el BLOQUE entero, decidido por Eduardo. Sobre la
     * Pico de caso_pico_sram (357 KB de techo, 64 de suelo). */
    CHECK(bpvm_mem_objetivo_env(0, 12345) == 12345 && bpvm_mem_objetivo_env(-7, 12345) == 12345,
          "vmheap: sin la clave (o no positiva), el objetivo de la imagen");
    CHECK(bpvm_mem_objetivo_env(200, 0) == 200u * 1024u, "vmheap=200: 200 KB");
    {   /* En 32 bits, 0x7FFFFFFF KB no caben en un size_t: satura. En 64, cabe entero. Lo
         * que no puede salir es la vuelta (0xFFFFFC00 en 32 bits: un bloque de basura). */
        size_t e = bpvm_mem_objetivo_env(0x7FFFFFFFL, 0);
        CHECK(e == (size_t) -1 || e / 1024u == (size_t) 0x7FFFFFFFu, "vmheap enorme: satura o cabe, no da la vuelta");
    }

    bpvm_mem_region_t r[1] = {{ (unsigned char*) 0x20014BF8, 431432, 431432, 0, 65536, "SRAM interna" }};
    bpvm_mem_plan_t p; char s[200];

    bpvm_mem_cfg_t c1 = { bpvm_mem_objetivo_env(200, 0), 64u * 1024u, 0, NULL };
    bpvm_mem_plan(r, 1, &c1, &p);
    bpvm_mem_plan_str(&p, r, &c1, s, sizeof s);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 200u * 1024u, "vmheap=200 en la Pico: 200 KB (%u)", (unsigned) p.bytes);
    CHECK(strstr(s, "vm: 200 KB en SRAM interna (objetivo 200, techo 357") != NULL, "vmheap=200: la linea: %s", s);

    bpvm_mem_cfg_t c2 = { bpvm_mem_objetivo_env(1000, 0), 64u * 1024u, 0, NULL };
    bpvm_mem_plan(r, 1, &c2, &p);
    bpvm_mem_plan_str(&p, r, &c2, s, sizeof s);
    CHECK(p.res == BPVM_MEM_OK && p.bytes == 365896, "vmheap=1000 en la Pico: manda el techo (%u)", (unsigned) p.bytes);
    CHECK(strstr(s, "POR DEBAJO del objetivo (1000)") != NULL, "vmheap=1000: la linea: %s", s);

    bpvm_mem_cfg_t c3 = { bpvm_mem_objetivo_env(32, 0), 64u * 1024u, 0, NULL };
    bpvm_mem_plan(r, 1, &c3, &p);
    bpvm_mem_plan_str(&p, r, &c3, s, sizeof s);
    CHECK(p.res == BPVM_MEM_NO_CABE && p.bytes == 0, "vmheap=32 en la Pico: por debajo del suelo, sin VM");
    CHECK(strstr(s, "el objetivo (32 KB) no llega al suelo (64 KB)") != NULL && strstr(s, "cabían 357 KB") != NULL,
          "vmheap=32: la linea dice que es lo PEDIDO, no la region: %s", s);
}

/* #691 (12-oct) — EL MOTIVO de una placa sin VM, el que sube al estado del arranque, al panel del IDE y al NOT_READY de
 * un RUN. Lo cazó la P4 con `vmheap=1024` (su suelo son 2 MB): el motivo era «heap de la VM no disponible» y el RUN
 * mandaba a «configurar particiones». Cada texto, y que quepa en los 64 B del motivo con lo más ancho del parque. */
static void caso_motivo(void) {
    char m[BPVM_BOOT_REASON_MAX];
    bpvm_mem_plan_t p;

    /* La P4 del 12-oct: 32 MB de PSRAM, 4 de margen, SQLite=4 ya tomado; vmheap=1024 KB y el suelo, 2 MB. */
    bpvm_mem_region_t p4[1] = {{ NULL, 28160u * 1024u, 28669u * 1024u, 1, 4u * 1024u * 1024u, "PSRAM" }};
    bpvm_mem_cfg_t c1 = { bpvm_mem_objetivo_env(1024, 0), 2u * 1024u * 1024u, 0, NULL };
    bpvm_mem_plan(p4, 1, &c1, &p);
    bpvm_mem_sin_vm_motivo(&p, p4, &c1, 1024, m, sizeof m);
    CHECK(p.res == BPVM_MEM_NO_CABE && !strcmp(m, "vmheap=1024 KB no llega al suelo de la VM (2048 KB)"),
          "motivo, la P4 con vmheap=1024: nombra la clave del ENV: \"%s\"", m);

    /* Lo mismo con el objetivo de la imagen (sin la clave): no se nombra una clave que no está. */
    bpvm_mem_region_t pico[1] = {{ (unsigned char*) 0x20014BF8, 431432, 431432, 0, 65536, "SRAM interna" }};
    bpvm_mem_cfg_t c2 = { 32u * 1024u, 64u * 1024u, 0, NULL };
    bpvm_mem_plan(pico, 1, &c2, &p);
    bpvm_mem_sin_vm_motivo(&p, pico, &c2, 0, m, sizeof m);
    CHECK(!strcmp(m, "el objetivo (32 KB) no llega al suelo de la VM (64 KB)"),
          "motivo, un objetivo de la imagen bajo el suelo: \"%s\"", m);

    /* La región no da el suelo (caso_suelo: 90 KB, 40 de margen): no es lo pedido, es la memoria. */
    bpvm_mem_region_t chica[1] = {{ NULL, 90u * 1024u, 90u * 1024u, 0, 40u * 1024u, "SRAM" }};
    bpvm_mem_cfg_t c3 = { 128u * 1024u, 64u * 1024u, 0, NULL };
    bpvm_mem_plan(chica, 1, &c3, &p);
    bpvm_mem_sin_vm_motivo(&p, chica, &c3, 128, m, sizeof m);
    CHECK(!strcmp(m, "la VM no cabe en SRAM (50 KB; el suelo, 64 KB)"),
          "motivo, la region no da el suelo (aunque haya vmheap, no es lo pedido): \"%s\"", m);

    /* El plan cabía y el asignador no dio ni el suelo (la escalera de las ESP32). */
    bpvm_mem_cfg_t c4 = { 0, 2u * 1024u * 1024u, 0, NULL };
    bpvm_mem_plan(p4, 1, &c4, &p);
    bpvm_mem_sin_vm_motivo(&p, p4, &c4, 0, m, sizeof m);
    CHECK(p.res == BPVM_MEM_OK && !strcmp(m, "la VM no cabe en PSRAM: el asignador no da ni 2048 KB"),
          "motivo, el plan cabia y no se pudo tomar: \"%s\"", m);

    /* Sin regiones. */
    bpvm_mem_plan(NULL, 0, &c1, &p);
    bpvm_mem_sin_vm_motivo(&p, NULL, &c1, 0, m, sizeof m);
    CHECK(!strcmp(m, "la placa no ofrece memoria para la VM"), "motivo, sin regiones: \"%s\"", m);

    /* LO MÁS ANCHO: la región de nombre más largo («SRAM estática», 14 B con la tilde) y cifras de cinco dígitos, más
     * de las que da el parque (32 MB). Si no cabe en los 64 B, el motivo se trunca y se pierde justo la cifra. */
    bpvm_mem_region_t ancha[1] = {{ NULL, 32767u * 1024u, 32767u * 1024u, 1, 0, "SRAM estática" }};
    int n;
    bpvm_mem_cfg_t c5 = { 0, 33000u * 1024u, 0, NULL };            /* la región no da el suelo */
    bpvm_mem_plan(ancha, 1, &c5, &p);
    n = bpvm_mem_sin_vm_motivo(&p, ancha, &c5, 0, m, sizeof m);
    CHECK(n > 0 && n < BPVM_BOOT_REASON_MAX && strstr(m, "(32767 KB; el suelo, 33000 KB)") != NULL,
          "motivo, el de la region mas ancho cabe entero (%d B de %d): \"%s\"", n, BPVM_BOOT_REASON_MAX - 1, m);
    bpvm_mem_cfg_t c6 = { 0, 33000u * 1024u - 1024u * 1024u, 0, NULL };   /* cabe: el del asignador */
    bpvm_mem_plan(ancha, 1, &c6, &p);
    n = bpvm_mem_sin_vm_motivo(&p, ancha, &c6, 0, m, sizeof m);
    CHECK(n > 0 && n < BPVM_BOOT_REASON_MAX && strstr(m, "no da ni 31976 KB") != NULL,
          "motivo, el del asignador mas ancho cabe entero (%d B): \"%s\"", n, m);
    bpvm_mem_cfg_t c7 = { 32000u * 1024u, 33000u * 1024u, 0, NULL };   /* lo pedido no llega al suelo */
    bpvm_mem_plan(ancha, 1, &c7, &p);
    n = bpvm_mem_sin_vm_motivo(&p, ancha, &c7, 0, m, sizeof m);
    CHECK(n > 0 && n < BPVM_BOOT_REASON_MAX && strstr(m, "(32000 KB) no llega al suelo de la VM (33000 KB)") != NULL,
          "motivo, el del objetivo mas ancho cabe entero (%d B): \"%s\"", n, m);
    n = bpvm_mem_sin_vm_motivo(&p, ancha, &c7, 32000, m, sizeof m);
    CHECK(n > 0 && n < BPVM_BOOT_REASON_MAX && !strcmp(m, "vmheap=32000 KB no llega al suelo de la VM (33000 KB)"),
          "motivo, el de vmheap mas ancho cabe entero (%d B): \"%s\"", n, m);
}

int main(void) {
    printf("=== test_mem: el planificador contra las placas medidas ===\n");
    caso_c3(); caso_c6(); caso_s3(); caso_s3_psram(); caso_metro_psram(); caso_pico_sram(); caso_suelo(); caso_exclusiva_con_margen(); caso_p4(); caso_stm32(); caso_discovery(); caso_vmheap();
    caso_motivo();
    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
