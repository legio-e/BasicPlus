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
#include "bpvm.h"             /* #338: la zona de rascar compartida (take/give) */
#include "bpvm_bmgr_wire.h"   /* #338: la frontera env/packs y su guardián */
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

    /* --- 8. #338: LA FRONTERA env / packs ------------------------------------
     * Desde #338 las dos copias del env ya no viven en el buffer del bulk: se las
     * presta la zona de rascar compartida, la MISMA que usan los PACK_*. Eso sólo
     * es seguro porque los dos grupos de comandos no coinciden nunca — y quien lo
     * decide es bpvm_bmgr_needs_env(). Si esa frontera se corre, en la placa
     * aparecería como "scratch OCUPADA" o, peor, como dos operaciones pisándose
     * la misma memoria. Por eso se prueba aquí verbo a verbo, y no de oídas. */
    {
        static const char* CON_ENV[] = { "STATE", "ENV_LS", "ENV_GET", "ENV_SET",
                                         "ENV_DEL", "PART_LS", "PART_DEFAULTS",
                                         "PART_APPLY" };
        static const char* SIN_ENV[] = { "PACK_LS", "PACK_ENTRIES", "PACK_BURN_BEGIN",
                                         "PACK_BURN_DATA", "PACK_BURN_END",
                                         "PACK_DEL", "PACK_FORMAT" };
        int bien = 1;
        for (size_t i = 0; i < sizeof CON_ENV / sizeof CON_ENV[0]; i++)
            if (!bpvm_bmgr_needs_env(CON_ENV[i])) { printf("    (%s deberia pedir env)\n", CON_ENV[i]); bien = 0; }
        CHECK(bien, "#338: los 8 verbos del entorno piden las copias A/B");
        bien = 1;
        for (size_t i = 0; i < sizeof SIN_ENV / sizeof SIN_ENV[0]; i++)
            if (bpvm_bmgr_needs_env(SIN_ENV[i])) { printf("    (%s NO deberia pedir env)\n", SIN_ENV[i]); bien = 0; }
        CHECK(bien, "#338: los 7 verbos de packs NO piden las copias A/B");
        CHECK(bpvm_bmgr_needs_env("VERBO_QUE_NO_EXISTE") == 1,
              "#338: un verbo desconocido pide env (el lado conservador: mejor de mas)");
        CHECK(bpvm_bmgr_needs_env(NULL) == 1, "#338: type NULL no revienta y pide env");

        /* Y el guardián: si una cintura se despista y no presta los buffers, el
         * verbo del entorno tiene que salir por el wire con su explicación, NO
         * leer un puntero nulo (en un micro eso es un reset sin nota). */
        char rep[512];
        bpvm_bmgr_t vacio;
        memset(&vacio, 0, sizeof vacio);      /* a = b = scratch = NULL */
        vacio.sector = SECT;
        bpvm_bmgr_req_t rq;
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "ENV_LS");
        rq.id = 7;
        int wr2 = -1;
        int nn = bpvm_bmgr_wire_dispatch(&vacio, &rq, rep, sizeof rep, &wr2);
        CHECK(nn > 0 && strstr(rep, "INTERNAL_ERROR") != NULL,
              "#338: verbo del entorno SIN buffers -> ERROR por el wire, no puntero nulo");
        CHECK(wr2 == -1, "#338: y sin escritura pendiente (no toca flash)");

        /* La otra mitad, que es la que de verdad importa: un PACK_* con los
         * buffers del env a NULL tiene que PASAR el guardián. Si no pasara, la
         * cintura estaría obligada a prestarlos siempre y el ahorro se esfuma. */
        snprintf(rq.type, sizeof rq.type, "PACK_LS");
        nn = bpvm_bmgr_wire_dispatch(&vacio, &rq, rep, sizeof rep, &wr2);
        CHECK(nn > 0 && strstr(rep, "UNSUPPORTED") != NULL,
              "#338: PACK_LS sin env pasa el guardian (llega a su propio 'sin zona de packs')");

        /* Y AHORA EL RIESGO DE VERDAD DE #338: que una cintura se deje la zona
         * cogida. No revienta en el acto —revienta en el comando SIGUIENTE, que
         * puede ser minutos despues y de otra familia de verbos—, asi que se
         * reproduce aqui la secuencia que hace la cintura, dos veces seguidas y
         * con un PACK_ en medio. Si el segundo take falla, es que el primero no
         * solto: exactamente el fallo que en placa se veria como un panel de
         * gestion que deja de responder sin motivo aparente. */
        CHECK(bpvm_scratch_capacity() >= 2u * SECT,
              "#338: la zona compartida da para las 2 copias del env de esta familia");
        for (int vuelta = 1; vuelta <= 3; vuelta++) {
            uint8_t* za = (uint8_t*) bpvm_scratch_take(2u * SECT, "bmgr-env");
            CHECK(za != NULL, vuelta == 1 ? "#338: vuelta 1 — la cintura coge la zona"
                            : vuelta == 2 ? "#338: vuelta 2 — la coge OTRA VEZ (la anterior solto)"
                                          : "#338: vuelta 3 — y otra mas");
            if (!za) break;
            bpvm_bmgr_t bmz;
            memset(&bmz, 0, sizeof bmz);
            bmz.a = za; bmz.b = za + SECT; bmz.scratch = SCRATCH; bmz.sector = SECT;
            bmz.part_base = BASE; bmz.usable_flash = U4M;
            memcpy(za, A, SECT); memcpy(za + SECT, B, SECT);
            snprintf(rq.type, sizeof rq.type, "ENV_LS");
            nn = bpvm_bmgr_wire_dispatch(&bmz, &rq, rep, sizeof rep, &wr2);
            CHECK(nn > 0 && strstr(rep, "ENV_LS_REPLY") != NULL,
                  "#338: ...y el ENV_LS responde con los buffers prestados");
            bpvm_scratch_give("bmgr-env");
            /* Entre medias, un PACK_ pide la zona para lo suyo: si el ENV la
             * hubiera retenido, aqui saldria NULL. */
            void* zp = bpvm_scratch_take(256, "PACK_LS");
            CHECK(zp != NULL, "#338: ...y un PACK_ la coge despues sin encontrarla ocupada");
            bpvm_scratch_give("PACK_LS");
        }
    }

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
