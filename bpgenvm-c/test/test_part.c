/*
 * test_part.c — H9: unidad de la tabla de particiones (bpvm_part), modelo FIJO +
 * ORDENADO (Eduardo 18-jul): el usuario solo edita TAMAÑOS; los OFFSETS se DERIVAN
 * (contiguos desde la base) → sin solapes por construcción; el env guarda solo
 * tamaños. Host-only. Prueba defaults, offsets derivados, validación (cero/
 * alineación/overflow), la virginidad (falta tamaño → MISSING), el clamp de #292 y
 * el round-trip sizes→env→layout.
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
#define BASE 0x100000u   /* 1 MB reservado (imagen + env) */
#define U4M  0x400000u   /* 4 MB usable */

static int mkenv(const char* payload, uint8_t* sect, bpvm_env_t* e) {
    int u = bpvm_env_serialize(payload, strlen(payload), 1u, sect, SECT);
    if (u < 0) return 0;
    return bpvm_env_parse(sect, SECT, e);
}

int main(void) {
    printf("=== test_part (H9 particiones: conjunto fijo, offsets derivados) ===\n");
    static uint8_t sect[SECT];
    bpvm_env_t e;
    bpvm_part_layout_t lay;
    int bad = -99;

    /* --- 1. nombres/kinds fijos --- */
    CHECK(BPVM_PART_COUNT == 2, "conjunto fijo de 2 particiones");
    CHECK(!strcmp(bpvm_part_name(BPVM_PART_FS), "fs")
          && !strcmp(bpvm_part_name(BPVM_PART_PACKS), "packs"), "nombres fijos fs/packs");

    /* --- 2. defaults: reparten [base, usable) alineado, suma exacta --- */
    {
        uint32_t sizes[BPVM_PART_COUNT];
        bpvm_part_defaults(BASE, U4M, S4K, sizes);
        uint32_t avail = U4M - BASE;   /* 3 MB */
        CHECK(sizes[BPVM_PART_FS] % S4K == 0 && sizes[BPVM_PART_PACKS] % S4K == 0,
              "defaults alineados al sector");
        CHECK(sizes[BPVM_PART_FS] + sizes[BPVM_PART_PACKS] == avail, "defaults suman el espacio disponible");
    }

    /* --- 3. layout desde tamaños: offsets DERIVADOS contiguos desde base --- */
    {
        uint32_t sizes[BPVM_PART_COUNT] = { 0x100000u /*fs 1M*/, 0x200000u /*packs 2M*/ };
        bpvm_part_err_t err = bpvm_part_layout_from_sizes(sizes, BASE, U4M, S4K, &lay, &bad);
        CHECK(err == BPVM_PART_OK && bad == -1, "layout bueno → OK");
        const bpvm_part_t* fs = bpvm_part_get(&lay, BPVM_PART_FS);
        const bpvm_part_t* pk = bpvm_part_get(&lay, BPVM_PART_PACKS);
        CHECK(fs && fs->offset == BASE && fs->size == 0x100000u, "FS: offset=base (derivado)");
        CHECK(pk && pk->offset == BASE + 0x100000u && pk->size == 0x200000u,
              "PACKS: offset = base + size(FS) (derivado, sin solape posible)");
    }

    /* --- 4. layout desde el env (solo tamaños; offsets no se guardan) --- */
    {
        CHECK(mkenv("flashSizeBytes=4194304\npart.fs.size=1048576\npart.packs.size=2097152\n",
                    sect, &e), "env con tamaños serializa+parsea");
        bpvm_part_err_t err = bpvm_part_layout(&e, BASE, U4M, S4K, &lay, &bad);
        CHECK(err == BPVM_PART_OK && lay.complete, "layout desde env → OK, completo");
        CHECK(bpvm_part_get(&lay, BPVM_PART_PACKS)->offset == BASE + 0x100000u,
              "offset de PACKS derivado desde el env");
    }

    /* --- 5. virgen: falta un tamaño → MISSING (1ª vez → proponer defaults) --- */
    {
        mkenv("flashSizeBytes=4194304\npart.fs.size=1048576\n", sect, &e);  /* sin packs */
        bpvm_part_err_t err = bpvm_part_layout(&e, BASE, U4M, S4K, &lay, &bad);
        CHECK(err == BPVM_PART_ERR_MISSING && !lay.complete, "falta tamaño → MISSING (virgen)");
        mkenv("flashSizeBytes=4194304\n", sect, &e);                        /* sin ninguno */
        CHECK(bpvm_part_layout(&e, BASE, U4M, S4K, &lay, &bad) == BPVM_PART_ERR_MISSING,
              "env sin tamaños → MISSING");
    }

    /* --- 6. validación: cero, no-alineado, overflow --- */
    {
        uint32_t z[BPVM_PART_COUNT]  = { 0u, 0x100000u };
        CHECK(bpvm_part_layout_from_sizes(z, BASE, U4M, S4K, &lay, &bad) == BPVM_PART_ERR_ZERO && bad == 0,
              "tamaño 0 → ZERO");
        uint32_t ua[BPVM_PART_COUNT] = { 0x100001u, 0x100000u };
        CHECK(bpvm_part_layout_from_sizes(ua, BASE, U4M, S4K, &lay, &bad) == BPVM_PART_ERR_UNALIGNED && bad == 0,
              "tamaño no alineado → UNALIGNED");
        uint32_t ov[BPVM_PART_COUNT] = { 0x200000u, 0x200000u };  /* 2M+2M=4M > avail 3M */
        CHECK(bpvm_part_layout_from_sizes(ov, BASE, U4M, S4K, &lay, &bad) == BPVM_PART_ERR_OVERFLOW && bad == 1,
              "suma no cabe → OVERFLOW");
    }

    /* --- 7. clamp #292: 16 MB reales, imagen 4 MB → usable 4 MB; un layout de 7 MB no cabe --- */
    {
        CHECK(bpvm_part_usable_flash(0x1000000u, 0x400000u) == 0x400000u, "usable = min(16M, 4M) = 4M");
        uint32_t big[BPVM_PART_COUNT] = { 0x100000u, 0x600000u };  /* 1M + 6M = 7M */
        uint32_t usable = bpvm_part_usable_flash(0x1000000u, 0x400000u);   /* clamp → 4M */
        CHECK(bpvm_part_layout_from_sizes(big, BASE, usable, S4K, &lay, &bad) == BPVM_PART_ERR_OVERFLOW,
              "#292: layout de 7M con imagen de 4M → OVERFLOW");
        /* SIN clamp (16M) el MISMO layout cabría */
        CHECK(bpvm_part_layout_from_sizes(big, BASE, 0x1000000u, S4K, &lay, &bad) == BPVM_PART_OK,
              "el mismo layout en 16M reales → OK");
    }

    /* --- 8. round-trip: sizes → payload → env → layout conserva los tamaños --- */
    {
        uint32_t sizes[BPVM_PART_COUNT] = { 0x100000u, 0x200000u };
        char pl[128];
        int w = bpvm_part_sizes_to_payload(sizes, pl, sizeof pl);
        CHECK(w > 0, "sizes_to_payload produce fragmento (solo tamaños)");
        CHECK(strstr(pl, "part.fs.size=1048576") && strstr(pl, "part.packs.size=2097152")
              && strstr(pl, "offset") == NULL, "el payload lleva SOLO tamaños (ningún offset)");
        mkenv(pl, sect, &e);
        bpvm_part_layout(&e, BASE, U4M, S4K, &lay, &bad);
        CHECK(bpvm_part_get(&lay, BPVM_PART_FS)->size == 0x100000u
              && bpvm_part_get(&lay, BPVM_PART_PACKS)->size == 0x200000u,
              "round-trip conserva los tamaños");
    }

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
