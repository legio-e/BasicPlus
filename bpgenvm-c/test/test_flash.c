/*
 * test_flash.c — V7/H10, el board_mgr común (#637, 3-oct): la regla de lo que se puede escribir de la flash
 * (include/bpvm_flash_zona.h), con los números de verdad de las dos STM32 y el reparto de la Pico.
 *
 * La STM32 sólo comprobaba su `.ld` en el `handle` del gestor, y su primitiva borraba cualquier página que existiera:
 * un `.ld` y una cabecera que no casaran habrían borrado firmware al anotar la fecha de la imagen en el ARRANQUE. Desde
 * #637 la guarda va dentro de `stm32_flash.c`, con esta regla; aquí se prueba la regla, que en la placa no se puede.
 *
 *     make test-flash        (esperado: sólo 'ok', [status=OK])
 */
#include "bpvm_flash_zona.h"

#include <stdio.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) printf("  ok  : %s\n", msg); \
    else { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

#define K8 0x2000u

int main(void) {
    /* ── la Discovery (STM32U5G9, 4 MB): el log en 0x0817E000, el ENV A/B detrás, las particiones desde 0x08184000 hasta
     * el final; lo de antes del log es el firmware ── */
    const uintptr_t base = 0x08000000u;
    const uintptr_t log  = base + 0x17E000u, part = base + 0x184000u, fin = base + 0x400000u;
    const bpvm_flash_zona_t dk2[] = { { log, fin } };
    CHECK(bpvm_flash_zona_ok(base + 0x180000u, K8, dk2, 1), "DK2: el ENV A (una página) se escribe");
    CHECK(bpvm_flash_zona_ok(log, K8, dk2, 1), "DK2: el log (su página) se escribe");
    CHECK(bpvm_flash_zona_ok(part, 64u * 1024u, dk2, 1), "DK2: el principio de las particiones se escribe");
    CHECK(bpvm_flash_zona_ok(fin - K8, K8, dk2, 1), "DK2: la última página de la flash se escribe");
    CHECK(!bpvm_flash_zona_ok(base, K8, dk2, 1), "DK2: la primera página del firmware, NO");
    CHECK(!bpvm_flash_zona_ok(log - K8, K8, dk2, 1), "DK2: la página de antes del log (firmware), NO");
    CHECK(!bpvm_flash_zona_ok(log - 16u, 32u, dk2, 1), "DK2: un tramo que pisa el firmware y el log, NO");
    CHECK(!bpvm_flash_zona_ok(fin, K8, dk2, 1), "DK2: pasado el final de la flash, NO");
    CHECK(!bpvm_flash_zona_ok(fin - 16u, 32u, dk2, 1), "DK2: un tramo que se sale por el final, NO");
    CHECK(!bpvm_flash_zona_ok(fin - 16u, (uintptr_t) 0 - 8u, dk2, 1), "DK2: un tramo que da la vuelta, NO");
    CHECK(bpvm_flash_zona_ok(base, 0, dk2, 1), "DK2: un tramo vacío no escribe nada (vale)");

    /* el layout: lo que da el .ld contra la cabecera */
    CHECK(bpvm_flash_layout_casa(log, part, log, log, part), "DK2: el .ld y la cabecera casan");
    CHECK(!bpvm_flash_layout_casa(log + K8, part, log, log, part), "DK2: el log del .ld movido una página: NO casa");
    CHECK(!bpvm_flash_layout_casa(log, part + K8, log, log, part), "DK2: las particiones del .ld movidas: NO casa");
    CHECK(!bpvm_flash_layout_casa(log, part, log + K8, log, part), "DK2: el firmware que se mete en el log: NO casa");

    /* ── la Nucleo (STM32U575, 2 MB): el log en 0x080FE000, las particiones desde 0x08104000 ── */
    const uintptr_t nlog = base + 0xFE000u, npart = base + 0x104000u, nfin = base + 0x200000u;
    const bpvm_flash_zona_t nucleo[] = { { nlog, nfin } };
    CHECK(bpvm_flash_zona_ok(base + 0x102000u, K8, nucleo, 1), "Nucleo: el ENV B se escribe");
    CHECK(!bpvm_flash_zona_ok(base + 0xFC000u, K8, nucleo, 1), "Nucleo: la última página del firmware, NO");
    CHECK(!bpvm_flash_zona_ok(nfin, K8, nucleo, 1), "Nucleo: fuera de sus 2 MB, NO");
    CHECK(bpvm_flash_layout_casa(nlog, npart, nlog, nlog, npart), "Nucleo: el .ld y la cabecera casan");

    /* ── la Pico (RP2350), dos zonas: la 2 del enlazador (ENV y log) y las particiones hasta el final de la flash ── */
    const bpvm_flash_zona_t pico[] = { { 0x010000u, 0x014000u }, { 0x200000u, 0x1000000u } };
    CHECK(bpvm_flash_zona_ok(0x010000u, 0x1000u, pico, 2), "Pico: el ENV A se escribe");
    CHECK(bpvm_flash_zona_ok(0x200000u, 0x1000u, pico, 2), "Pico: el principio de las particiones se escribe");
    CHECK(!bpvm_flash_zona_ok(0x00F000u, 0x1000u, pico, 2), "Pico: la imagen, NO");
    CHECK(!bpvm_flash_zona_ok(0x013000u, 0x2000u, pico, 2), "Pico: un tramo que sale de la zona 2, NO");
    CHECK(!bpvm_flash_zona_ok(0x100000u, 0x1000u, pico, 2), "Pico: entre la zona 2 y las particiones, NO");

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
