/*
 * test_bmgr.c — H9: board manager (núcleo del protocolo de gestión de placa).
 * Host-only: los dos sectores A/B viven en RAM y HACEN de flash (en el host el bmgr
 * escribe directamente en ellos, así que no hay "volcado" aparte). Recorre el flujo
 * que describió Eduardo: placa VIRGEN → el escrutinio no encuentra nada → asistente
 * fija el entorno → propone/ajusta/aplica tamaños de particiones → las ediciones
 * persisten (A/B, seq creciente) y una validación fallida NO toca el env.
 *
 *   make test-bmgr
 */
#include "bpvm_bmgr.h"
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

#define SECT 4096u
#define BASE 0x100000u   /* 1 MB reservado (imagen + env) */
#define U4M  0x400000u   /* 4 MB usable (placa tipo Pico) → 3 MB para particiones */

int main(void) {
    printf("=== test_bmgr (H9 protocolo de gestión de placa) ===\n");
    static uint8_t A[SECT], B[SECT], SCRATCH[SECT];
    memset(A, 0xFF, sizeof A);   /* flash borrada (placa virgen) */
    memset(B, 0xFF, sizeof B);

    bpvm_bmgr_t bm;
    memset(&bm, 0, sizeof bm);   /* campos nuevos (p.ej. packs H3) nunca con basura */
    bm.a = A; bm.b = B; bm.scratch = SCRATCH; bm.sector = SECT;
    bm.part_base = BASE; bm.usable_flash = U4M;

    int slot = -9;
    char v[32];
    bpvm_part_layout_t lay;
    int bad = -9;

    /* --- 1. PLACA VIRGEN: el escrutinio no encuentra nada --- */
    CHECK(bpvm_bmgr_env(&bm, NULL) == 0, "sin env (virgen)");
    CHECK(bpvm_bmgr_env_count(&bm) == 0, "0 variables de entorno");
    CHECK(bpvm_bmgr_part_layout(&bm, SECT, &lay, &bad) == BPVM_PART_ERR_MISSING,
          "particiones MISSING (virgen) → el IDE abre el asistente");

    /* --- 2. ASISTENTE paso 1: entorno (identidad de placa) --- */
    CHECK(bpvm_bmgr_env_set(&bm, "board", "Pico2", &slot) == 0 && slot == 0,
          "set board (1ª escritura → slot A)");
    CHECK(bpvm_bmgr_env_set(&bm, "flashSizeBytes", "4194304", &slot) == 0 && slot == 1,
          "set flashSizeBytes (2ª → slot B)");
    CHECK(bpvm_bmgr_env_set(&bm, "psram", "0", &slot) == 0 && slot == 0,
          "set psram (3ª → vuelve a A, ping-pong A/B)");
    CHECK(bpvm_bmgr_env_set(&bm, "gpioCount", "26", &slot) == 0,
          "set gpioCount");

    CHECK(bpvm_bmgr_env_count(&bm) == 4, "ahora 4 variables");
    CHECK(bpvm_bmgr_env_get(&bm, "board", v, sizeof v) == 5 && !strcmp(v, "Pico2"), "get board");
    CHECK(bpvm_bmgr_env_get(&bm, "flashSizeBytes", v, sizeof v) > 0 && !strcmp(v, "4194304"),
          "get flashSizeBytes");

    /* ENV_LS: la tabla nombre|valor de la pestaña "Variables del entorno" */
    {
        int seen_board = 0, seen_gpio = 0, n = bpvm_bmgr_env_count(&bm);
        for (int i = 0; i < n; i++) {
            char k[24], val[24];
            if (bpvm_bmgr_env_pair_at(&bm, i, k, sizeof k, val, sizeof val)) {
                if (!strcmp(k, "board") && !strcmp(val, "Pico2")) seen_board = 1;
                if (!strcmp(k, "gpioCount") && !strcmp(val, "26")) seen_gpio = 1;
            }
        }
        CHECK(seen_board && seen_gpio, "ENV_LS enumera los pares (board + gpioCount)");
    }

    /* --- 3. ASISTENTE paso 2: particiones (ya conocido el tamaño de flash) --- */
    {
        uint32_t sizes[BPVM_PART_COUNT];
        bpvm_bmgr_part_defaults(&bm, SECT, sizes);
        CHECK(sizes[BPVM_PART_FS] + sizes[BPVM_PART_PACKS] == (U4M - BASE),
              "PART_DEFAULTS reparte los 3 MB disponibles");

        /* el usuario ajusta: FS 1 MB, PACKS 1 MB (deja holgura), y aplica */
        uint32_t chosen[BPVM_PART_COUNT] = { 0x100000u, 0x100000u };
        CHECK(bpvm_bmgr_part_apply(&bm, SECT, chosen, &bad, &slot) == BPVM_PART_OK,
              "PART_APPLY con tamaños válidos → OK");
    }
    /* tras aplicar, ya NO es virgen a nivel de particiones */
    CHECK(bpvm_bmgr_part_layout(&bm, SECT, &lay, &bad) == BPVM_PART_OK && lay.complete,
          "PART_LS → OK, completo (ya no virgen)");
    CHECK(lay.parts[BPVM_PART_FS].offset == BASE
          && lay.parts[BPVM_PART_PACKS].offset == BASE + 0x100000u,
          "offsets derivados coherentes (FS@base, PACKS@base+fs)");

    /* las variables de entorno SIGUEN ahí (apply no las tocó) */
    CHECK(bpvm_bmgr_env_get(&bm, "board", v, sizeof v) > 0 && !strcmp(v, "Pico2"),
          "apply conserva las variables de entorno");
    CHECK(bpvm_bmgr_env_count(&bm) == 5, "5 vars: 4 de entorno + 1 part.fs.size (packs deriva)");

    /* --- 4. EDITAR una variable existente (psram 0→1) persiste y no rompe nada --- */
    CHECK(bpvm_bmgr_env_set(&bm, "psram", "1", &slot) == 0, "edita psram 0→1");
    CHECK(bpvm_bmgr_env_get(&bm, "psram", v, sizeof v) > 0 && !strcmp(v, "1"), "psram=1 persiste");
    CHECK(bpvm_bmgr_part_layout(&bm, SECT, &lay, &bad) == BPVM_PART_OK,
          "las particiones siguen intactas tras editar entorno");
    CHECK(bpvm_bmgr_env_count(&bm) == 5, "siguen 5 (edición, no alta)");

    /* --- 5. VALIDACIÓN fallida NO toca el env (transaccional) --- */
    {
        /* snapshot del layout bueno actual */
        bpvm_part_layout_t before;
        bpvm_bmgr_part_layout(&bm, SECT, &before, &bad);

        uint32_t bad_sizes[BPVM_PART_COUNT] = { 0x400000u, 0u }; /* FS 4M > 3M avail (la knob no cabe) */
        int wr = -9;
        bpvm_part_err_t e = bpvm_bmgr_part_apply(&bm, SECT, bad_sizes, &bad, &wr);
        CHECK(e == BPVM_PART_ERR_OVERFLOW, "PART_APPLY inválido → OVERFLOW");
        CHECK(wr == -9, "apply inválido no reporta wrote_slot (no escribió)");

        bpvm_part_layout_t after;
        bpvm_bmgr_part_layout(&bm, SECT, &after, &bad);
        CHECK(after.parts[BPVM_PART_FS].size == before.parts[BPVM_PART_FS].size
              && after.parts[BPVM_PART_PACKS].size == before.parts[BPVM_PART_PACKS].size,
              "el env quedó INTACTO tras la validación fallida");
    }

    /* --- 6. RE-APPLY no duplica claves; una part.* huérfana se descarta --- */
    {
        /* inyecta una part huérfana escribiéndola como variable suelta */
        bpvm_bmgr_env_set(&bm, "part.zzz.size", "999", &slot);
        CHECK(bpvm_bmgr_env_get(&bm, "part.zzz.size", v, sizeof v) > 0, "huérfana part.zzz.size presente");

        uint32_t again[BPVM_PART_COUNT] = { 0x180000u, 0x080000u }; /* 1.5M + 0.5M */
        CHECK(bpvm_bmgr_part_apply(&bm, SECT, again, &bad, &slot) == BPVM_PART_OK, "re-apply OK");
        CHECK(bpvm_bmgr_env_get(&bm, "part.zzz.size", v, sizeof v) == -1,
              "re-apply descarta la part.* huérfana (reescribe el bloque gestionado)");

        /* exactamente una fs.size y una packs.size, con los valores nuevos */
        CHECK(bpvm_bmgr_env_get(&bm, "part.fs.size", v, sizeof v) > 0 && !strcmp(v, "1572864"),
              "part.fs.size = 1.5M (valor nuevo, sin duplicar)");
        CHECK(bpvm_bmgr_part_layout(&bm, SECT, &lay, &bad) == BPVM_PART_OK
              && lay.parts[BPVM_PART_PACKS].offset == BASE + 0x180000u,
              "offset de PACKS re-derivado del tamaño nuevo de FS");
    }

    /* --- 7. clamp #292 propagado por el bmgr (usable_flash ya viene clampado) --- */
    {
        bpvm_bmgr_t bm2 = bm;
        bm2.usable_flash = bpvm_part_usable_flash(0x1000000u, 0x400000u);  /* 16M reales, imagen 4M → 4M */
        static uint8_t A2[SECT], B2[SECT], S2[SECT];
        memset(A2, 0xFF, SECT); memset(B2, 0xFF, SECT);
        bm2.a = A2; bm2.b = B2; bm2.scratch = S2;
        uint32_t big[BPVM_PART_COUNT] = { 0x600000u, 0u };  /* FS 6M (la knob) */
        int wr;
        CHECK(bpvm_bmgr_part_apply(&bm2, SECT, big, &bad, &wr) == BPVM_PART_ERR_OVERFLOW,
              "#292: FS de 6M con imagen de 4M → OVERFLOW (aunque la flash real sea 16M)");
    }

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
