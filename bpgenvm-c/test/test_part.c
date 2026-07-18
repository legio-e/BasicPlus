/*
 * test_part.c — H9: unidad de la tabla de particiones (bpvm_part) sobre el env.
 * Host-only, sin placa: construye envs en RAM y valida el parse, la fachada
 * kind→región, el round-trip (to_payload) y la validación del layout — incluido
 * el clamp de #292 (partición válida en 16 MB reales pero fuera de una imagen de
 * 4 MB) reproducido como test.
 *
 *   make test-part
 */
#include "bpvm_part.h"
#include "bpvm_env.h"
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

#define SECT 4096u
#define S4K  4096u

/* Serializa `payload` en `sect` y lo parsea a `e`. Devuelve 1 si válido. */
static int mkenv(const char* payload, uint8_t* sect, bpvm_env_t* e) {
    int u = bpvm_env_serialize(payload, strlen(payload), 1u, sect, SECT);
    if (u < 0) return 0;
    return bpvm_env_parse(sect, SECT, e);
}

int main(void) {
    printf("=== test_part (H9 tabla de particiones) ===\n");
    static uint8_t sect[SECT];
    bpvm_env_t e;
    bpvm_part_table_t t;

    /* --- 1. parse + fachada kind→región + por nombre --- */
    const char* GOOD =
        "flashSizeBytes=4194304\n"
        "partitions=fs,packs\n"
        "part.fs.offset=1048576\n"      /* 0x100000 */
        "part.fs.size=131072\n"          /* 128K */
        "part.packs.offset=1179648\n"    /* 0x120000 */
        "part.packs.size=262144\n";      /* 256K */
    CHECK(mkenv(GOOD, sect, &e), "env con particiones serializa+parsea");
    CHECK(bpvm_part_parse(&e, &t) == 2 && t.count == 2, "parse: 2 particiones");
    const bpvm_part_t* fs = bpvm_part_find(&t, BPVM_PART_FS);
    const bpvm_part_t* pk = bpvm_part_find(&t, BPVM_PART_PACKS);
    CHECK(fs && fs->offset == 0x100000u && fs->size == 131072u, "find(FS) = región correcta");
    CHECK(pk && pk->offset == 0x120000u && pk->size == 262144u, "find(PACKS) = región correcta");
    CHECK(bpvm_part_find_name(&t, "fs") == fs, "find_name(fs)");
    CHECK(bpvm_part_find(&t, BPVM_PART_APP) == NULL, "find(APP) = NULL (no hay)");

    /* --- 2. validación de un layout bueno --- */
    int bad = -99;
    /* reserved_end = 1 MB (imagen + env por debajo); flash usable = 4 MB. */
    CHECK(bpvm_part_validate(&t, 0x400000u, 0x100000u, S4K, &bad) == BPVM_PART_OK && bad == -1,
          "layout bueno → OK");

    /* --- 3. clamp #292: 16 MB reales, imagen declara 4 MB → usable = 4 MB --- */
    CHECK(bpvm_part_usable_flash(0x1000000u, 0x400000u) == 0x400000u, "usable_flash = min(16M, 4M) = 4M");
    CHECK(bpvm_part_usable_flash(0x400000u, 0) == 0x400000u, "usable_flash sin clamp (image_max=0)");
    {
        /* partición a 5 MB: válida en 16 MB, pero fuera de la imagen de 4 MB. */
        const char* BIG =
            "partitions=fs\npart.fs.offset=5242880\npart.fs.size=131072\n"; /* 0x500000 */
        bpvm_env_t e2; static uint8_t s2[SECT];
        mkenv(BIG, s2, &e2); bpvm_part_table_t t2; bpvm_part_parse(&e2, &t2);
        uint32_t usable = bpvm_part_usable_flash(0x1000000u, 0x400000u);   /* clamp → 4M */
        CHECK(bpvm_part_validate(&t2, usable, 0x100000u, S4K, &bad) == BPVM_PART_ERR_OUT_OF_FLASH,
              "#292: partición a 5M con imagen 4M → fuera de flash usable");
    }

    /* --- 4. errores de validación --- */
    {
        bpvm_env_t x; static uint8_t sx[SECT]; bpvm_part_table_t tx;

        mkenv("partitions=fs\npart.fs.offset=1048577\npart.fs.size=131072\n", sx, &x); /* 0x100001 */
        bpvm_part_parse(&x, &tx);
        CHECK(bpvm_part_validate(&tx, 0x400000u, 0x100000u, S4K, &bad) == BPVM_PART_ERR_UNALIGNED,
              "offset no alineado → UNALIGNED");

        mkenv("partitions=fs\npart.fs.offset=524288\npart.fs.size=131072\n", sx, &x); /* 0x80000 < 1M */
        bpvm_part_parse(&x, &tx);
        CHECK(bpvm_part_validate(&tx, 0x400000u, 0x100000u, S4K, &bad) == BPVM_PART_ERR_BELOW_RESERVED,
              "por debajo de reserved_end → BELOW_RESERVED");

        mkenv("partitions=fs\npart.fs.offset=1048576\npart.fs.size=0\n", sx, &x);
        bpvm_part_parse(&x, &tx);
        CHECK(bpvm_part_validate(&tx, 0x400000u, 0x100000u, S4K, &bad) == BPVM_PART_ERR_EMPTY,
              "size 0 → EMPTY");

        /* dos particiones que se solapan */
        mkenv("partitions=fs,packs\n"
              "part.fs.offset=1048576\npart.fs.size=131072\n"       /* [0x100000, 0x120000) */
              "part.packs.offset=1114112\npart.packs.size=131072\n", /* 0x110000 dentro de fs */
              sx, &x);
        bpvm_part_parse(&x, &tx);
        CHECK(bpvm_part_validate(&tx, 0x400000u, 0x100000u, S4K, &bad) == BPVM_PART_ERR_OVERLAP,
              "solape → OVERLAP");
    }

    /* --- 5. tolerancia: sin partitions= → 0; nombre desconocido → APP --- */
    {
        bpvm_env_t x; static uint8_t sx[SECT]; bpvm_part_table_t tx;
        mkenv("flashSizeBytes=4194304\n", sx, &x);                  /* env sin tabla */
        CHECK(bpvm_part_parse(&x, &tx) == 0, "sin partitions= → 0 (estado 1 virgen)");

        mkenv("partitions=misc\npart.misc.offset=2097152\npart.misc.size=65536\n", sx, &x);
        bpvm_part_parse(&x, &tx);
        const bpvm_part_t* m = bpvm_part_find_name(&tx, "misc");
        CHECK(m && m->kind == BPVM_PART_APP, "nombre desconocido → kind APP");
    }

    /* --- 6. round-trip: to_payload → env → parse → misma tabla --- */
    {
        char pl[256];
        int w = bpvm_part_to_payload(&t, pl, sizeof pl);
        CHECK(w > 0, "to_payload produce fragmento");
        bpvm_env_t e3; static uint8_t s3[SECT]; bpvm_part_table_t t3;
        mkenv(pl, s3, &e3); bpvm_part_parse(&e3, &t3);
        const bpvm_part_t* fs3 = bpvm_part_find(&t3, BPVM_PART_FS);
        const bpvm_part_t* pk3 = bpvm_part_find(&t3, BPVM_PART_PACKS);
        CHECK(t3.count == 2 && fs3 && fs3->offset == 0x100000u && fs3->size == 131072u
              && pk3 && pk3->offset == 0x120000u && pk3->size == 262144u,
              "round-trip to_payload → parse conserva la tabla");
    }

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
