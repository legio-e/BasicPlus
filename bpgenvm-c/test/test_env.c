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

    /* --- 3b. V7/H6.c G1: la que usa la BIOS — o entero o nada --- */
    {
        char w[13];                               /* "MetroRP2350B" = 12 + NUL: justo */
        CHECK(bpvm_env_get_exacto(&e, "board", w, sizeof w) == 12 && !strcmp(w, "MetroRP2350B"),
              "exacto: cabe JUSTO (12 + NUL en 13) -> el valor entero");
        CHECK(bpvm_env_get_exacto(&e, "BOARD", w, sizeof w) == 12,
              "exacto: la clave no distingue mayusculas (como el ENV)");
        char c[12]; memset(c, 'x', sizeof c);
        CHECK(bpvm_env_get_exacto(&e, "board", c, sizeof c) == -2 && c[0] == '\0',
              "exacto: UN byte corto -> -2 y el bufer VACIO (no 'MetroRP2350', que parece bueno)");
        CHECK(bpvm_env_get_exacto(&e, "noExiste", w, sizeof w) == -1 && w[0] == '\0',
              "exacto: clave que no esta -> -1 y el bufer vacio");
        CHECK(bpvm_env_get_exacto(&e, "", w, sizeof w) == -3, "exacto: clave vacia -> -3");
        CHECK(bpvm_env_get_exacto(&e, "board", NULL, 8) == -3, "exacto: bufer NULL -> -3");
        CHECK(bpvm_env_get_exacto(&e, "board", w, 0) == -3, "exacto: cap 0 -> -3");
        CHECK(bpvm_env_get_exacto(NULL, "board", w, sizeof w) == -1,
              "exacto: sin ENV -> -1 (ninguna clave esta), sin reventar");
    }

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

    /* --- 8. V7/H10 #626: AÑADIR una clave que falta, sin tercer búfer (la fecha de la imagen) --- */
    printf("--- 8. bpvm_env_anadir (#626) ---\n");
    {
        enum { S8 = 512 };
        static uint8_t a[S8], b[S8], out[S8], ca[S8], cb[S8];
        char v[32];
        bpvm_env_t e;
        int slot = -9;

        /* (a) virgen: el bloque nuevo, en `out` (A, seq 1, sólo la clave); las copias ni se tocan */
        memset(a, 0xFF, S8); memset(b, 0xFF, S8);
        CHECK(bpvm_env_anadir(a, b, S8, "imagen.fecha", "1790899200", out, &slot) == 0 && slot == 0,
              "virgen: 0, y toca escribir A");
        CHECK(bpvm_env_parse(out, S8, &e) == 1 && e.seq == 1
              && bpvm_env_get(&e, "imagen.fecha", v, sizeof v) == 10 && strcmp(v, "1790899200") == 0,
              "el bloque nuevo vale: seq 1, con la clave");
        CHECK(a[0] == 0xFF && b[0] == 0xFF, "con `out`, las copias sólo se leen (pueden ser la flash)");

        /* (b) A actual (seq 5): `out` NULL construye en la rancia (B, seq 6) y A no se toca */
        bpvm_env_serialize(PAYLOAD, strlen(PAYLOAD), 5u, a, S8);
        memcpy(ca, a, S8);
        CHECK(bpvm_env_anadir(a, b, S8, "imagen.fecha", "1790899200", NULL, &slot) == 0 && slot == 1,
              "out NULL: se construye en la rancia (B)");
        CHECK(memcmp(a, ca, S8) == 0, "la copia actual (A) no se toca");
        CHECK(bpvm_env_pick(a, S8, b, S8, &e) == 1 && e.seq == 6, "gana B, con seq 6");
        CHECK(bpvm_env_get(&e, "imagen.fecha", v, sizeof v) == 10
              && bpvm_env_get(&e, "board", v, sizeof v) > 0 && strcmp(v, "MetroRP2350B") == 0,
              "la clave nueva, y las de antes siguen");

        /* (c) ya está (sin distinguir mayúsculas): 1, y no se escribe NADA */
        memcpy(ca, a, S8); memcpy(cb, b, S8);
        CHECK(bpvm_env_anadir(a, b, S8, "IMAGEN.FECHA", "1", NULL, &slot) == 1, "ya estaba: 1");
        CHECK(memcmp(a, ca, S8) == 0 && memcmp(b, cb, S8) == 0, "ni se toca la copia: la flash no se gasta");

        /* (d) no cabe: -1, y la actual sigue siendo la buena */
        char largo[600];
        memset(largo, 'x', sizeof largo - 1); largo[sizeof largo - 1] = '\0';
        CHECK(bpvm_env_anadir(a, b, S8, "otra", largo, NULL, &slot) == -1, "no cabe: -1");
        CHECK(bpvm_env_pick(a, S8, b, S8, &e) == 1 && e.seq == 6, "y gana la de antes (B, seq 6)");

        /* (e) lo que rompería el formato `clave=valor\n` */
        CHECK(bpvm_env_anadir(a, b, S8, "a=b", "1", out, &slot) == -1
              && bpvm_env_anadir(a, b, S8, "a\nb", "1", out, &slot) == -1
              && bpvm_env_anadir(a, b, S8, "a", "1\n2", out, &slot) == -1
              && bpvm_env_anadir(a, b, S8, "", "1", out, &slot) == -1,
              "una clave con '=' o salto, vacía, o un valor con salto: -1");
    }

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
