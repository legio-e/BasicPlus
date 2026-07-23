/*
 * test_pack.c — H3: unidad de la zona de packs (bpvm_pack). Host-only, sin
 * placa ni VM: una región RAM hace de "flash" (0xFF = borrada, como la NOR).
 *
 * PARIDAD: las fixtures samples/PackFix{A,B}.pack están generadas por Pack.jar
 * (PackCli build --date fijo → deterministas). Este test recalcula los CRCs en
 * C sobre esos bytes Java y verifica el recorrido completo — si el C y el Java
 * divergen en un byte, esto se pone rojo (el micro rechazaría los packs del PC).
 *
 * Flujo (orden de Eduardo): LIST → ADD (append) → REMOVE (tombstone) → el
 * hueco lo recupera una compactación (lado PC, aquí solo se comprueba que el
 * tombstone NO invalida la cabecera y que el scan lo sigue saltando).
 *
 *   make test-pack
 */
#include "bpvm_pack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

#define FIX_SIZE 4096u          /* ambas fixtures = 1 bloque de borrado */
#define REGION   (64u * 1024u)  /* región de packs simulada */

/* Contenido esperado de App.mod dentro de PackFixA (misma fórmula que generó
 * la fixture: data[i] = (i*7+3)&0xFF, con [5]=0x00 y [6]=0xFF). */
static void app_mod_bytes(uint8_t* d) {
    for (int i = 0; i < 37; i++) d[i] = (uint8_t) ((i * 7 + 3) & 0xFF);
    d[5] = 0x00; d[6] = 0xFF;
}

static int load_fix(const char* path, uint8_t* dst, uint32_t size) {
    FILE* f = fopen(path, "rb");
    if (!f) { printf("  FAIL: no puedo abrir %s\n", path); g_fail++; return 0; }
    size_t n = fread(dst, 1, size + 1, f);
    fclose(f);
    if (n != size) { printf("  FAIL: %s mide %zu B (esperaba %u)\n", path, n, size); g_fail++; return 0; }
    return 1;
}

int main(void) {
    printf("=== test_pack (H3 zona de packs) ===\n");

    /* --- 1. CRC-16/CCITT-FALSE: valor de comprobación estándar --- */
    CHECK(bpvm_pack_crc16(BPVM_PACK_CRC16_INIT, (const uint8_t*) "123456789", 9) == 0x29B1,
          "crc16(\"123456789\") == 0x29B1");

    /* --- 2. PARIDAD: leer la fixture construida por Pack.jar --- */
    static uint8_t fixA[FIX_SIZE], fixB[FIX_SIZE];
    if (!load_fix("samples/PackFixA.pack", fixA, FIX_SIZE)) goto done;
    if (!load_fix("samples/PackFixB.pack", fixB, FIX_SIZE)) goto done;

    bpvm_pack_info_t inf[4];
    uint32_t end = 0;
    int n = bpvm_pack_scan(fixA, FIX_SIZE, inf, 4, /*verify_content=*/1, &end);
    CHECK(n == 1, "scan de la fixture: 1 pack");
    CHECK(inf[0].crc_ok, "crc_cab + crc_contenido (Java) validan en C");
    CHECK(inf[0].alive, "pack activo");
    CHECK(inf[0].verfmt == 1 && inf[0].size_total == FIX_SIZE, "verfmt 1, size_total 4096");
    CHECK(strcmp(inf[0].nombre, "PackFixA") == 0, "nombre 'PackFixA'");
    CHECK(strcmp(inf[0].vercont, "1.0") == 0, "version contenido '1.0'");
    CHECK(inf[0].fecha == 1700000000uL, "fecha fija de la fixture");
    CHECK(inf[0].n_entries == 3, "3 ficheros");
    CHECK(inf[0].content_end == 128u + (48+40) + (48+12) + (48+8),
          "content_end incluye el pad de la ultima entrada");
    CHECK(end == FIX_SIZE, "end_off = fin del pack");

    /* --- 3. entradas: tipo/nombre/longitud + datos byte a byte --- */
    bpvm_pack_entry_t es[4];
    n = bpvm_pack_entries(fixA, FIX_SIZE, 0, es, 4);
    CHECK(n == 3, "entries devuelve 3");
    CHECK(!strcmp(es[0].tipo, "mod") && !strcmp(es[0].nombre, "App") && es[0].len == 37,
          "entrada 0 = mod/App 37 B");
    CHECK(!strcmp(es[1].tipo, "mft") && !strcmp(es[1].nombre, "manifest") && es[1].len == 9,
          "entrada 1 = mft/manifest 9 B");
    CHECK(!strcmp(es[2].tipo, "txt") && !strcmp(es[2].nombre, "logo") && es[2].len == 5,
          "entrada 2 = txt/logo 5 B");
    {
        uint8_t want[37];
        app_mod_bytes(want);
        CHECK(memcmp(fixA + es[0].data_off, want, 37) == 0, "datos de App.mod byte-identicos");
        CHECK(memcmp(fixA + es[1].data_off, "main=App\n", 9) == 0, "datos del manifest");
        CHECK(fixA[es[0].data_off + 37] == 0xFF, "pad de alineacion = 0xFF (NOR)");
    }

    /* --- 4. find (XIP): puntero directo al dato dentro de la región --- */
    {
        uint32_t len = 0;
        const uint8_t* p = bpvm_pack_find(fixA, FIX_SIZE, "txt", "logo", &len);
        CHECK(p && len == 5 && memcmp(p, "hola\n", 5) == 0, "find txt/logo");
        CHECK(bpvm_pack_find(fixA, FIX_SIZE, "mod", "NoEsta", &len) == NULL, "find inexistente = NULL");
    }

    /* --- 5. LIST + ADD sobre región virgen (flash simulada) --- */
    static uint8_t reg[REGION];
    memset(reg, 0xFF, sizeof reg);
    CHECK(bpvm_pack_scan(reg, REGION, NULL, 0, 0, &end) == 0 && end == 0,
          "region virgen: 0 packs, append en 0");
    CHECK(bpvm_pack_add(reg, REGION, fixA, FIX_SIZE) == 0, "add PackFixA → off 0");
    CHECK(bpvm_pack_add(reg, REGION, fixB, FIX_SIZE) == (int32_t) FIX_SIZE, "add PackFixB → off 4096");
    n = bpvm_pack_scan(reg, REGION, inf, 4, 1, &end);
    CHECK(n == 2 && end == 2 * FIX_SIZE, "cadena: 2 packs, append en 8192");
    CHECK(inf[0].crc_ok && inf[1].crc_ok, "ambos validan tras el burn");
    {
        uint32_t len = 0;
        const uint8_t* p = bpvm_pack_find(reg, REGION, "mod", "Lib", &len);
        CHECK(p && len == 11 && memcmp(p, "LIBCONTENT\n", 11) == 0, "find en el 2o pack");
    }

    /* --- 6. version nueva del mismo pack: el ULTIMO de la cadena gana --- */
    CHECK(bpvm_pack_add(reg, REGION, fixA, FIX_SIZE) == (int32_t) (2 * FIX_SIZE),
          "add PackFixA otra vez (version nueva) → off 8192");
    {
        uint32_t len = 0;
        const uint8_t* p = bpvm_pack_find(reg, REGION, "mod", "App", &len);
        CHECK(p && (uint32_t) (p - reg) > 2 * FIX_SIZE, "find resuelve a la copia MAS reciente");
    }

    /* --- 7. REMOVE = tombstone (bit ALIVE 1→0, sin borrar nada) --- */
    CHECK(bpvm_pack_remove(reg, REGION, "PackFixA") == (int32_t) (2 * FIX_SIZE),
          "remove tumba el ultimo PackFixA activo");
    n = bpvm_pack_scan(reg, REGION, inf, 4, 1, &end);
    CHECK(n == 3, "el tombstone SIGUE en la cadena (no se borra)");
    CHECK(inf[0].alive && inf[1].alive && !inf[2].alive, "solo el 3o esta tombstoned");
    CHECK(inf[2].crc_ok, "tombstone NO invalida crc_cab (flags fuera del CRC)");
    {
        uint32_t len = 0;
        const uint8_t* p = bpvm_pack_find(reg, REGION, "mod", "App", &len);
        CHECK(p && (uint32_t) (p - reg) < FIX_SIZE, "find cae a la copia anterior (off 0)");
    }
    CHECK(bpvm_pack_remove_at(reg, REGION, 0) == 0, "remove_at del PackFixA original");
    CHECK(bpvm_pack_remove_at(reg, REGION, 0) == -1, "remove_at repetido = -1 (ya tombstoned)");
    CHECK(bpvm_pack_remove(reg, REGION, "PackFixA") == -1, "remove por nombre: ya no hay activo");
    {
        uint32_t len = 0;
        CHECK(bpvm_pack_find(reg, REGION, "mod", "App", &len) == NULL, "App ya no se ve");
        CHECK(bpvm_pack_find(reg, REGION, "mod", "Lib", &len) != NULL, "Lib (activo) sigue visible");
    }

    /* --- 8. corrupcion: cabecera mala CORTA la cadena; contenido malo NO --- */
    {
        static uint8_t bad[REGION];
        memcpy(bad, reg, REGION);
        bad[12] ^= 0xFF;                       /* nombre del 1er pack → crc_cab falla */
        n = bpvm_pack_scan(bad, REGION, inf, 4, 0, &end);
        CHECK(n == 1 && !inf[0].crc_ok && end == BPVM_PACK_NO_SPACE,
              "cabecera corrupta: para el scan, sin punto de append");
        CHECK(bpvm_pack_add(bad, REGION, fixB, FIX_SIZE) == BPVM_PACK_ERR_NOSPACE,
              "add se niega sobre cadena corrupta");

        memcpy(bad, reg, REGION);
        bad[200] ^= 0xFF;                      /* datos del 1er pack → solo crc_contenido */
        n = bpvm_pack_scan(bad, REGION, inf, 4, 1, &end);
        CHECK(n == 3 && !inf[0].crc_ok && inf[1].crc_ok && end == 3 * FIX_SIZE,
              "contenido corrupto: pack marcado pero la cadena sigue (size_total fiable)");
    }

    /* --- 9. ADD valida la imagen entera antes de tocar la flash --- */
    {
        static uint8_t img[FIX_SIZE];
        memcpy(img, fixA, FIX_SIZE);
        img[300] ^= 0x01;                      /* un bit de datos */
        CHECK(bpvm_pack_add(reg, REGION, img, FIX_SIZE) == BPVM_PACK_ERR_BADIMG,
              "imagen con crc_contenido malo → rechazada");
        CHECK(bpvm_pack_add(reg, REGION, fixA, FIX_SIZE - 1) == BPVM_PACK_ERR_BADIMG,
              "imagen truncada → rechazada");
    }

    /* --- 10. sin espacio: la región se llena y el add lo dice --- */
    {
        static uint8_t small[2 * FIX_SIZE];
        memset(small, 0xFF, sizeof small);
        CHECK(bpvm_pack_add(small, sizeof small, fixA, FIX_SIZE) == 0, "add 1/2 cabe");
        CHECK(bpvm_pack_add(small, sizeof small, fixB, FIX_SIZE) == (int32_t) FIX_SIZE, "add 2/2 cabe");
        CHECK(bpvm_pack_add(small, sizeof small, fixA, FIX_SIZE) == BPVM_PACK_ERR_NOSPACE,
              "add 3/2 → NO_SPACE");
    }

done:
    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
