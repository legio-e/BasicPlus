/*
 * test_sqlmem.c — V5/H: unidad de la regla del bloque de memoria de la BD.
 * Host-only, sin placa y sin VM: la regla es aritmética pura sobre dos números
 * (lo que pide el entorno, lo que hay), así que se prueba entera aquí.
 *
 * Lo que se verifica NO es "que salga un número", sino los BORDES, que es donde
 * viven los fallos: el mínimo medido, el suelo que la VM tiene que conservar, el
 * desbordamiento al multiplicar, y —lo importante— que cada NO tenga su MOTIVO
 * distinguible (no se pidió ≠ se pidió mal ≠ no cabe).
 *
 *   make test-sqlmem
 */
#include "bpvm_sqlmem.h"
#include <stdio.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

#define MB(n) ((size_t)(n) * 1024u * 1024u)

/* Atajo: ejecuta la regla y compara motivo + bytes de una vez. */
static int caso(long env_mb, size_t total, bpvm_sqlite_res_t esperado,
                size_t bytes_esperados) {
    size_t got = 12345;                     /* basura: la regla debe pisarla */
    bpvm_sqlite_res_t r = bpvm_sqlite_region(env_mb, total, &got);
    return r == esperado && got == bytes_esperados;
}

int main(void) {
    printf("== regla del bloque de la BD (V5/H) ==\n");

    /* ── 1. No se pide: silencio, y sin reservar nada ── */
    CHECK(caso(0,  MB(8), BPVM_SQLITE_OFF, 0), "SQLite=0 -> OFF, 0 bytes");
    CHECK(caso(-1, MB(8), BPVM_SQLITE_OFF, 0), "SQLite negativo -> OFF (no es un error, es que no)");

    /* ── 2. El mínimo MEDIDO: 2 MB. Justo debajo NO se activa ── */
    CHECK(caso(1, MB(8), BPVM_SQLITE_MUY_POCO, 0),
          "SQLite=1 -> MUY_POCO (el bloque contiguo del ordenador no cabe)");
    CHECK(caso(BPVM_SQLITE_MIN_MB, MB(8), BPVM_SQLITE_OK, MB(2)),
          "SQLite=2 -> OK, 2 MB (el suelo medido, exacto)");

    /* Y el MOTIVO tiene que ser distinguible de "no se pidió": si los dos
     * dieran OFF, el usuario que escribe 1 no se enteraría de nada. */
    CHECK(bpvm_sqlite_region(1, MB(8), NULL) != bpvm_sqlite_region(0, MB(8), NULL),
          "pedir de menos NO se confunde con no pedir");

    /* ── 3. Casos normales ── */
    CHECK(caso(2,  MB(8),  BPVM_SQLITE_OK, MB(2)),  "Metro 8 MB, SQLite=2 -> 2 MB");
    CHECK(caso(4,  MB(8),  BPVM_SQLITE_OK, MB(4)),  "Metro 8 MB, SQLite=4 -> 4 MB");
    CHECK(caso(8,  MB(32), BPVM_SQLITE_OK, MB(8)),  "P4 32 MB, SQLite=8 -> 8 MB");

    /* ── 4. El suelo de la VM: no se le quita hasta dejarla inviable ── */
    CHECK(caso(8, MB(8), BPVM_SQLITE_NO_CABE, 0),
          "pedir TODO el bloque -> NO_CABE (la VM se queda sin nada)");
    {
        /* Justo en el borde: total = want + el mínimo de la VM. Debe CABER. */
        size_t total = MB(2) + BPVM_VM_MIN_BYTES;
        CHECK(caso(2, total, BPVM_SQLITE_OK, MB(2)),
              "borde exacto: want + minimo de VM -> OK");
        /* Un byte menos: NO. */
        CHECK(caso(2, total - 1, BPVM_SQLITE_NO_CABE, 0),
              "un byte por debajo del borde -> NO_CABE");
    }

    /* ── 5. Placa sin PSRAM: la Pico entera son 343 KB ── */
    CHECK(caso(2, 343u * 1024u, BPVM_SQLITE_NO_CABE, 0),
          "Pico sin PSRAM (343 KB) -> NO_CABE, no se activa a medias");

    /* ── 6. Desbordamiento: el valor lo escribe una persona ──
     * En size_t de 32 bits, 4096 MB da 0 al multiplicar por 1<<20, y un 0
     * pasaría todas las comprobaciones de "cabe". Se corta ANTES. */
    CHECK(caso(4096, MB(32), BPVM_SQLITE_NO_CABE, 0),
          "4096 MB -> NO_CABE (se corta antes de multiplicar)");
    CHECK(caso(999999, MB(32), BPVM_SQLITE_NO_CABE, 0),
          "un numero absurdo -> NO_CABE, no un tamano basura");

    /* ── 7. out_bytes opcional: no debe explotar con NULL ── */
    CHECK(bpvm_sqlite_region(2, MB(8), NULL) == BPVM_SQLITE_OK,
          "out_bytes NULL es valido (el INFO solo quiere el motivo)");

    /* ── 8. Cada motivo tiene texto, y son distintos ── */
    CHECK(bpvm_sqlite_res_str(BPVM_SQLITE_OFF)[0]      != '\0' &&
          bpvm_sqlite_res_str(BPVM_SQLITE_OK)[0]       != '\0' &&
          bpvm_sqlite_res_str(BPVM_SQLITE_MUY_POCO)[0] != '\0' &&
          bpvm_sqlite_res_str(BPVM_SQLITE_NO_CABE)[0]  != '\0',
          "los cuatro motivos tienen texto para el log");

    printf("\n[status=%s]\n", g_fail == 0 ? "OK" : "FAIL");
    return g_fail != 0;
}
