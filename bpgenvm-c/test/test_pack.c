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

/* --- "flash" ESTRICTA para el burn: write-once (programar solo sobre 0xFF,
 * el modelo mas duro = flash interna con ECC). Caza doble-program y erase
 * desalineado, que en placa serian corrupcion silenciosa. --- */
#define FLREG (16u * 1024u)
static uint8_t g_fl[FLREG];
/* #310 — read_at deliberadamente TACAÑO: sirve como mucho 7 bytes por llamada,
 * como haria un FS con su propio tamano de bloque. Un lector que se crea que
 * una lectura corta es un error (o un fin de datos) se rompe aqui y no en la
 * placa seis meses despues. */
static long stingy_read(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    const uint8_t* base = (const uint8_t*) user;
    if (off >= REGION) return 0;
    uint32_t avail = REGION - off;
    if (n > avail) n = avail;
    if (n > 7u) n = 7u;
    memcpy(dst, base + off, n);
    return (long) n;
}

static int fl_erase(void* u, uint32_t off, uint32_t len) {
    (void) u;
    if ((off % 4096u) != 0 || (len % 4096u) != 0 || off + len > FLREG) return -1;
    memset(g_fl + off, 0xFF, len);
    return 0;
}
static int fl_program(void* u, uint32_t off, const uint8_t* d, uint32_t len) {
    (void) u;
    if (off + len > FLREG) return -1;
    for (uint32_t i = 0; i < len; i++) {
        if (g_fl[off + i] != 0xFF) return -1;   /* write-once: doble-program = error */
        g_fl[off + i] = d[i];
    }
    return 0;
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

    /* --- 11. BURN por chunks sobre una "flash" ESTRICTA (write-once) --- */
    {
        memset(g_fl, 0xFF, sizeof g_fl);
        bpvm_pack_flash_t fl = { fl_erase, fl_program, NULL, 4096u };
        bpvm_pack_burn_t bs;
        memset(&bs, 0, sizeof bs);

        /* happy path en trozos de 1008 (16×63: parte cabecera y contenido pero
         * respeta el contrato de multiplos de 16) */
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, FIX_SIZE, &bs) == 0, "burn begin → off 0");
        uint32_t sent = 0;
        int derr = 0;
        while (sent < FIX_SIZE) {
            uint32_t n = FIX_SIZE - sent < 1008 ? FIX_SIZE - sent : 1008;
            if (bpvm_pack_burn_data(&bs, &fl, fixA + sent, n) != 0) { derr = 1; break; }
            sent += n;
            if (sent == 2016) {   /* a MITAD de burn: el pack NO existe aún */
                uint32_t e2 = 0;
                int n2 = bpvm_pack_scan(g_fl, FLREG, NULL, 0, 0, &e2);
                CHECK(n2 == 0 && e2 == 0, "a mitad de burn: invisible (magic 0xFF)");
            }
        }
        CHECK(!derr, "todos los chunks grabados");
        CHECK(bpvm_pack_burn_end(g_fl, FLREG, &bs, &fl) == 0, "burn end → visible en 0");
        n = bpvm_pack_scan(g_fl, FLREG, inf, 4, 1, &end);
        CHECK(n == 1 && inf[0].crc_ok && !strcmp(inf[0].nombre, "PackFixA") && end == FIX_SIZE,
              "tras burn: 1 pack valido, byte-identico (CRCs OK)");
        CHECK(memcmp(g_fl, fixA, FIX_SIZE) == 0, "flash == imagen original");

        /* torn burn: begin+mitad SIN end → invisible; el siguiente begin re-borra */
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, FIX_SIZE, &bs) == (int32_t) FIX_SIZE,
              "2o begin → append en 4096");
        CHECK(bpvm_pack_burn_data(&bs, &fl, fixB, 2048) == 0, "torn: mitad grabada");
        n = bpvm_pack_scan(g_fl, FLREG, NULL, 0, 0, &end);
        CHECK(n == 1 && end == FIX_SIZE, "torn abandonado: sigue habiendo 1 pack");
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, FIX_SIZE, &bs) == (int32_t) FIX_SIZE,
              "re-begin re-borra y reusa el hueco");
        CHECK(bpvm_pack_burn_data(&bs, &fl, fixB, FIX_SIZE) == 0, "reintento entero");
        CHECK(bpvm_pack_burn_end(g_fl, FLREG, &bs, &fl) == (int32_t) FIX_SIZE, "reintento OK");
        n = bpvm_pack_scan(g_fl, FLREG, inf, 4, 1, &end);
        CHECK(n == 2 && inf[1].crc_ok && !strcmp(inf[1].nombre, "PackFixB"),
              "cadena: PackFixA + PackFixB");

        /* contenido corrupto: el END lo caza EN FLASH y el pack no se activa */
        static uint8_t badimg[FIX_SIZE];
        memcpy(badimg, fixA, FIX_SIZE);
        badimg[300] ^= 0x01;
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, FIX_SIZE, &bs) == (int32_t) (2 * FIX_SIZE),
              "begin del corrupto");
        CHECK(bpvm_pack_burn_data(&bs, &fl, badimg, FIX_SIZE) == 0, "chunks del corrupto");
        CHECK(bpvm_pack_burn_end(g_fl, FLREG, &bs, &fl) == BPVM_PACK_ERR_VERIFY,
              "end → VERIFY falla (crc_contenido en flash)");
        n = bpvm_pack_scan(g_fl, FLREG, NULL, 0, 0, &end);
        CHECK(n == 2 && end == 2 * FIX_SIZE, "el corrupto NO se activo (invisible)");

        /* cabecera corrupta */
        memcpy(badimg, fixA, FIX_SIZE);
        badimg[12] ^= 0xFF;   /* nombre → crc_cab falla */
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, FIX_SIZE, &bs) >= 0, "begin cab-mala");
        CHECK(bpvm_pack_burn_data(&bs, &fl, badimg, FIX_SIZE) == 0, "chunks cab-mala");
        CHECK(bpvm_pack_burn_end(g_fl, FLREG, &bs, &fl) == BPVM_PACK_ERR_BADIMG,
              "end → BADIMG (crc_cab retenida)");

        /* validaciones del begin + estado */
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, 4095, &bs) == BPVM_PACK_ERR_ALIGN,
              "size no alineado al bloque → ALIGN");
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, 100, &bs) == BPVM_PACK_ERR_BADIMG,
              "size < cabecera → BADIMG");
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, FLREG, &bs) == BPVM_PACK_ERR_NOSPACE,
              "no cabe → NOSPACE");
        CHECK(bpvm_pack_burn_data(&bs, &fl, fixA, 100) == BPVM_PACK_ERR_STATE,
              "data sin sesion → STATE");
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, FIX_SIZE, &bs) >= 0, "begin overflow-test");
        CHECK(bpvm_pack_burn_data(&bs, &fl, fixA, FIX_SIZE) == 0, "data completa");
        CHECK(bpvm_pack_burn_data(&bs, &fl, fixA, 16) == BPVM_PACK_ERR_STATE,
              "data de mas → STATE (sesion cerrada)");
        CHECK(bpvm_pack_burn_begin(g_fl, FLREG, &fl, FIX_SIZE, &bs) >= 0, "begin chunk-desalineado");
        CHECK(bpvm_pack_burn_data(&bs, &fl, fixA, 1000) == BPVM_PACK_ERR_STATE,
              "chunk no multiplo de 16 → STATE (contrato quadword/ECC)");

        /* --- fase 3: DEL en flash real = RMW de la 1a pagina (gotcha ECC) --- */
        static uint8_t page[4096];
        CHECK(bpvm_pack_del(g_fl, FLREG, &fl, FIX_SIZE, page) == 0,
              "del del 2o pack (RMW de su 1a pagina)");
        n = bpvm_pack_scan(g_fl, FLREG, inf, 4, 1, &end);
        CHECK(n == 2 && inf[0].alive && !inf[1].alive, "cadena: A activo, B borrado");
        CHECK(inf[1].crc_ok && end == 2 * FIX_SIZE,
              "tombstone RMW conserva cabecera valida y la cadena entera");
        CHECK(bpvm_pack_del(g_fl, FLREG, &fl, FIX_SIZE, page) == BPVM_PACK_ERR_STATE,
              "del repetido → STATE (ya borrado)");
        CHECK(bpvm_pack_del(g_fl, FLREG, &fl, 12345, page) == BPVM_PACK_ERR_BADIMG,
              "del en offset invalido → BADIMG");
    }

    /* ── #310: la MISMA región leída por trozos ──────────────────────────────
     * El lector se generalizó a una FUENTE para poder leer un pack que vive en
     * el FS (donde no hay bytes contiguos que mapear). El riesgo de esa
     * generalización es que las dos fuentes NO den lo mismo, y eso no lo caza
     * nada de lo de arriba: todo va por la fuente de memoria.
     *
     * Así que aquí se leen LOS MISMOS BYTES por el camino de trozos y se
     * compara contra el mapeado. Y a propósito con un `read_at` TACAÑO, que
     * devuelve como mucho 7 bytes por llamada: si el lector se creyera que una
     * lectura corta es un error —o peor, un fin de datos— saldría aquí. */
    {
        static uint8_t region[REGION];
        memcpy(region, reg, REGION);

        bpvm_pack_src_t msrc, ssrc;
        bpvm_pack_src_mem(&msrc, region, REGION);
        bpvm_pack_src_stream(&ssrc, stingy_read, region, REGION);

        bpvm_pack_info_t im[4], is[4];
        uint32_t em = 0, es = 0;
        int nm = bpvm_pack_scan_src(&msrc, im, 4, 1, &em);
        int ns = bpvm_pack_scan_src(&ssrc, is, 4, 1, &es);
        CHECK(nm == ns && em == es && nm > 0,
              "#310 scan: mapeado y por trozos ven la misma cadena");
        int same = (nm == ns);
        for (int i = 0; same && i < nm; i++) {
            same = im[i].off == is[i].off && im[i].size_total == is[i].size_total
                && im[i].n_entries == is[i].n_entries && im[i].alive == is[i].alive
                && im[i].crc_ok == is[i].crc_ok
                && strcmp(im[i].nombre, is[i].nombre) == 0;
        }
        CHECK(same, "#310 scan: cada pack igual campo a campo (incl. crc_contenido)");

        /* El CRC de contenido por trozos es el punto que más fácil se rompe:
         * si crc_ok saliera 0 aquí, un pack bueno se rechazaría en el FS. */
        CHECK(nm > 0 && is[0].crc_ok, "#310 crc_contenido por trozos VALIDA el pack");

        bpvm_pack_entry_t em2[8], es2[8];
        int cm = bpvm_pack_entries_src(&msrc, 0, em2, 8);
        int cs = bpvm_pack_entries_src(&ssrc, 0, es2, 8);
        int eq = (cm == cs && cm > 0);
        for (int i = 0; eq && i < cm; i++) {
            eq = em2[i].len == es2[i].len && em2[i].data_off == es2[i].data_off
              && strcmp(em2[i].tipo, es2[i].tipo) == 0
              && strcmp(em2[i].nombre, es2[i].nombre) == 0;
        }
        CHECK(eq, "#310 entries: misma lista por los dos caminos");

        /* 'Lib' sigue en un pack ACTIVO; 'App' está en packs ya tombstoneados,
         * así que el "no está" también tiene que coincidir en las dos fuentes
         * — media generalización que sólo acierta cuando encuentra algo es
         * peor que ninguna. */
        bpvm_pack_entry_t fm, fs2;
        int okm = bpvm_pack_find_src(&msrc, "mod", "Lib", &fm);
        int oks = bpvm_pack_find_src(&ssrc, "mod", "Lib", &fs2);
        CHECK(okm && oks && fm.data_off == fs2.data_off && fm.len == fs2.len,
              "#310 find: misma entrada por los dos caminos");
        CHECK(bpvm_pack_find_src(&msrc, "mod", "App", &fm) == 0
           && bpvm_pack_find_src(&ssrc, "mod", "App", &fs2) == 0,
              "#310 find: el 'no esta' (tombstone) coincide en las dos");
        okm = bpvm_pack_find_src(&msrc, "mod", "Lib", &fm);
        oks = bpvm_pack_find_src(&ssrc, "mod", "Lib", &fs2);

        /* El predicado de "¿XIP o copia?": la fuente mapeada da puntero y la de
         * trozos no. De AHÍ sale la regla de con/sin código — no se programa. */
        CHECK(bpvm_pack_src_ptr(&msrc, fm.data_off, fm.len) == region + fm.data_off,
              "#310 la fuente mapeada da puntero directo (→ XIP)");
        CHECK(bpvm_pack_src_ptr(&ssrc, fs2.data_off, fs2.len) == NULL,
              "#310 la fuente por trozos NO da puntero (→ carga con codigo)");

        /* Y los BYTES del dato leídos a trozos son los del fichero. */
        uint8_t got[64], want[64];
        int rd = 1;
        if (okm && fm.len <= sizeof got) {
            memcpy(want, region + fm.data_off, fm.len);
            for (uint32_t o = 0; o < fm.len; ) {
                long r = stingy_read(region, fs2.data_off + o, got + o, fm.len - o);
                if (r <= 0) { rd = 0; break; }
                o += (uint32_t) r;
            }
            CHECK(rd && memcmp(got, want, fm.len) == 0,
                  "#310 el dato leido por trozos es byte a byte el mismo");
        }
    }

done:
    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
