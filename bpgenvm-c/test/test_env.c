/*
 * test_env.c — H9: unidad del bloque de env (bpvm_env). Host-only, sin placa ni
 * VM: opera sobre buffers en RAM que HACEN de "flash" (dos sectores A/B). Prueba
 * el formato, el CRC, la tolerancia a claves desconocidas, la selección A/B por
 * seq y el flujo de actualización (escribir en la copia rancia con seq+1).
 *
 *   make test-env
 */
#include "bpvm_env.h"
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

#define SECT 4096u   /* tamaño de sector (borrado) realista */

/* Un payload de identidad de placa típico (Metro RP2350B). */
static const char* PAYLOAD =
    "board=MetroRP2350B\n"
    "flashSizeBytes=16777216\n"
    "psram=1\n"
    "psramSizeBytes=8388608\n"
    "psramCsPin=47\n"
    "gpioCount=48\n";

int main(void) {
    printf("=== test_env (H9 bloque de env) ===\n");
    static uint8_t A[SECT], B[SECT];
    memset(A, 0xFF, sizeof A);   /* flash borrada */
    memset(B, 0xFF, sizeof B);

    /* --- 1. serialize + parse (round-trip) --- */
    int used = bpvm_env_serialize(PAYLOAD, strlen(PAYLOAD), 1u, A, SECT);
    CHECK(used == (int)(BPVM_ENV_HEADER_SIZE + strlen(PAYLOAD)), "serialize devuelve bytes usados");
    CHECK(A[used] == 0xFF && A[SECT-1] == 0xFF, "pad 0xFF tras el payload");

    bpvm_env_t e;
    CHECK(bpvm_env_parse(A, SECT, &e) == 1 && e.valid, "parse valida el bloque");
    CHECK(e.seq == 1u, "seq leída = 1");

    /* --- 2. get: string + tipadas --- */
    char v[32];
    CHECK(bpvm_env_get(&e, "board", v, sizeof v) == 12 && !strcmp(v, "MetroRP2350B"), "get board");
    CHECK(bpvm_env_get_long(&e, "flashSizeBytes", -1) == 16777216L, "get_long flashSizeBytes");
    CHECK(bpvm_env_get_bool(&e, "psram", 0) == 1, "get_bool psram=1");
    CHECK(bpvm_env_get_long(&e, "psramCsPin", -1) == 47L, "get_long psramCsPin");
    CHECK(bpvm_env_get_long(&e, "gpioCount", -1) == 48L, "get_long gpioCount");

    /* --- 3. clave desconocida → tolerante (no rompe) --- */
    CHECK(bpvm_env_get(&e, "noExiste", v, sizeof v) == -1, "clave desconocida = -1");
    CHECK(bpvm_env_get_long(&e, "noExiste", 999) == 999L, "get_long ausente → default");
    CHECK(bpvm_env_get_bool(&e, "noExiste", 1) == 1, "get_bool ausente → default");

    /* --- 4. bloque corrupto → parse rechaza --- */
    {
        static uint8_t C[SECT];
        memcpy(C, A, SECT);
        C[0] = 'X';                                   /* magic roto */
        CHECK(bpvm_env_parse(C, SECT, &e) == 0, "magic roto → rechaza");
        memcpy(C, A, SECT);
        C[5] = 0x09;                                  /* version rota */
        CHECK(bpvm_env_parse(C, SECT, &e) == 0, "version desconocida → rechaza");
        memcpy(C, A, SECT);
        C[BPVM_ENV_HEADER_SIZE + 2] ^= 0xFF;          /* flip de un byte de payload → CRC no cuadra */
        CHECK(bpvm_env_parse(C, SECT, &e) == 0, "payload corrupto (CRC) → rechaza");
        memcpy(C, A, SECT);
        C[6] = 0xFF; C[7] = 0xFF;                      /* len enorme, fuera de rango */
        CHECK(bpvm_env_parse(C, SECT, &e) == 0, "len fuera de rango → rechaza");
    }

    /* --- 5. A/B: gana la copia válida con seq mayor --- */
    /* B con seq=2 (más nueva). */
    bpvm_env_serialize("board=Pico2\nflashSizeBytes=4194304\npsram=0\n",
                       strlen("board=Pico2\nflashSizeBytes=4194304\npsram=0\n"), 2u, B, SECT);
    bpvm_env_t picked;
    int who = bpvm_env_pick(A, SECT, B, SECT, &picked);
    CHECK(who == 1 && picked.seq == 2u, "pick elige B (seq 2 > 1)");
    CHECK(bpvm_env_get_bool(&picked, "psram", 1) == 0, "la copia elegida es la de B (psram=0)");

    /* Si corrompemos B (la más nueva), gana A (la vieja pero válida). */
    {
        static uint8_t Bbad[SECT];
        memcpy(Bbad, B, SECT);
        Bbad[BPVM_ENV_HEADER_SIZE] ^= 0xFF;           /* corrompe payload de B */
        who = bpvm_env_pick(A, SECT, Bbad, SECT, &picked);
        CHECK(who == 0 && picked.seq == 1u, "B corrupta → pick cae a A (seq 1)");
    }

    /* Ambas borradas (flash virgen) → ninguna válida. */
    {
        static uint8_t Z1[SECT], Z2[SECT];
        memset(Z1, 0xFF, SECT); memset(Z2, 0xFF, SECT);
        who = bpvm_env_pick(Z1, SECT, Z2, SECT, &picked);
        CHECK(who == -1 && !picked.valid, "flash virgen (ambas 0xFF) → ninguna válida");
    }

    /* --- 6. flujo de actualización A/B: next_seq + escribir en la copia rancia --- */
    /* Estado: A seq=1, B seq=2 (B es la actual). next_seq = 3; la rancia es A. */
    {
        uint32_t ns = bpvm_env_next_seq(A, SECT, B, SECT);
        CHECK(ns == 3u, "next_seq = max(1,2)+1 = 3");
        int slotB = bpvm_env_pick(A, SECT, B, SECT, &picked);   /* actual = B */
        CHECK(slotB == 1, "la copia actual es B → escribimos en la rancia A");
        /* Escribimos la nueva config en A con seq=3. */
        bpvm_env_serialize("board=Pico2\npsram=0\ngpioCount=26\n",
                           strlen("board=Pico2\npsram=0\ngpioCount=26\n"), ns, A, SECT);
        int who2 = bpvm_env_pick(A, SECT, B, SECT, &picked);
        CHECK(who2 == 0 && picked.seq == 3u, "tras update, pick elige A (seq 3)");
        CHECK(bpvm_env_get_long(&picked, "gpioCount", -1) == 26L, "update visible (gpioCount=26)");
    }

    /* --- 7. payload_set: reemplazar / añadir / borrar una clave (base de ENV_SET/DEL) --- */
    {
        const char* base = "board=Pico2\npsram=0\ngpioCount=26\n";
        char out[256];

        /* (a) reemplazar una clave existente conserva las demás y el valor nuevo */
        int n = bpvm_env_payload_set(base, strlen(base), "psram", "1", out, sizeof out);
        CHECK(n > 0, "payload_set reemplaza → longitud > 0");
        out[n] = '\0';
        CHECK(strstr(out, "psram=1\n") && !strstr(out, "psram=0"), "psram reemplazado 0→1");
        CHECK(strstr(out, "board=Pico2\n") && strstr(out, "gpioCount=26\n"), "las demás claves intactas");

        /* re-parsear el resultado (via serialize) confirma que sigue siendo un env válido */
        {
            static uint8_t S[SECT];
            bpvm_env_serialize(out, (size_t) n, 5u, S, SECT);
            bpvm_env_t re;
            CHECK(bpvm_env_parse(S, SECT, &re) == 1
                  && bpvm_env_get_bool(&re, "psram", 0) == 1, "el env editado re-parsea (psram=1)");
        }

        /* (b) clave nueva se AÑADE al final con separador correcto */
        n = bpvm_env_payload_set(base, strlen(base), "part.fs.size", "1048576", out, sizeof out);
        out[n] = '\0';
        CHECK(strstr(out, "part.fs.size=1048576\n"), "clave nueva añadida");
        CHECK(strstr(out, "board=Pico2\n") && strstr(out, "gpioCount=26\n"), "las previas siguen");

        /* (c) value=NULL BORRA la clave (y solo esa) */
        n = bpvm_env_payload_set(base, strlen(base), "psram", NULL, out, sizeof out);
        out[n] = '\0';
        CHECK(!strstr(out, "psram"), "value=NULL borra la clave");
        CHECK(strstr(out, "board=Pico2\n") && strstr(out, "gpioCount=26\n"), "el borrado no toca las demás");

        /* (d) partir de un payload vacío: set crea la primera línea */
        n = bpvm_env_payload_set(NULL, 0, "board", "MetroRP2350B", out, sizeof out);
        out[n] = '\0';
        CHECK(n > 0 && !strcmp(out, "board=MetroRP2350B\n"), "set sobre payload vacío crea la 1ª línea");

        /* (e) capacidad insuficiente → -1 (no desborda) */
        CHECK(bpvm_env_payload_set(base, strlen(base), "x", "y", out, 4) == -1, "cap insuficiente → -1");
    }

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
