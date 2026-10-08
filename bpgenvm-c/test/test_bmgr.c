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
#include "bpvm_pack.h"        /* V7/H6.c G1: una zona de packs con un motor dentro */
#include "bpvm_npack.h"
#include "bpvm_fs.h"          /* el tipo del doble de abajo */
#include "mdn_loader.h"       /* ídem */
#include <stdio.h>
#include <string.h>

/* DOBLES. `bpvm_pack.c` engancha la zona al FS como respaldo de lectura (#362)
 * y `bpvm_npack.c` pregunta la arquitectura en su escalera: nada de eso se
 * ejercita aquí, pero el enlazador los pide.
 *
 * ⚠️ Sin el primero, esta prueba NO ENLAZABA desde el 13-ago (#362, d5552ed6)
 * y nadie lo vio en seis semanas: no está en ninguna de las redes que se pasan
 * a diario. Se descubrió el 23-sep al añadirle el caso de G1. */
void bpvm_fs_set_fallback(bpvm_fs_ov_stat_fn st, bpvm_fs_ov_read_fn rd,
                          bpvm_fs_ov_locate_fn locate, void* user)
{ (void) st; (void) rd; (void) locate; (void) user; }
uint16_t    bpvm_mdn_host_arch(void)      { return 0; }
const char* bpvm_mdn_host_float_abi(void) { return ""; }
/* V7/H7 (30-sep): y otra vez lo mismo —el wire de packs llama a bpvm_mods_resync desde #468 (25-sep) y la prueba
 * volvio a no enlazar—. Desde hoy esta en la red (check-testc), que es lo que faltaba. */
void        bpvm_mods_resync(void)        { }
/* V7 #671 — los motores «cargados», a mano: bpvm_npack_carga.c no entra en esta prueba. */
static int g_cargados = 0;
int bpvm_npack_motores_cargados(void) { return g_cargados; }

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

#define SECT 4096u
#define BASE 0x100000u   /* 1 MB reservado (imagen + env) */
#define U4M  0x400000u   /* 4 MB usable (placa tipo Pico) → 3 MB para particiones */

/* V7/H6.c G1 — una zona de packs EN RAM para PACK_BURN_BEGIN: borrar y
 * programar son memset/memcpy, y un pack con un motor se arma a mano con el
 * formato de pack/PackFormat.java (si no cuadrara, el tope no lo vería y el
 * caso saldría en rojo). */
/* ── V7/H10, el board_mgr común: una FAMILIA de prueba con su flash del ENV en RAM (dos sectores), que puede negarse
 * a escribir. Las dos formas de leer: en su sitio (la Pico, la STM32, el sim) y con espejo (la ESP32). ── */
static uint8_t g_fl[2][SECT];          /* la «flash» del ENV: A y B */
static uint8_t g_espejo[2 * SECT];     /* el espejo de la familia que no lee en su sitio */
static int     g_fl_falla = 0;         /* 1: la flash no escribe */
static int     g_fl_escrituras = 0;    /* cuántas escrituras llegaron a la flash */
static const uint8_t* fam_copia(int slot) { return g_fl[slot]; }
static int fam_leer(int slot, uint8_t* dst) { memcpy(dst, g_fl[slot], SECT); return 0; }
static int fam_escribir(int slot, const uint8_t* src) {
    if (g_fl_falla) return -1;
    memcpy(g_fl[slot], src, SECT);
    g_fl_escrituras++;
    return 0;
}
static const bpvm_bmgr_familia_t FAM_EN_SU_SITIO = { SECT, fam_copia, NULL, NULL, fam_escribir, NULL, NULL };
static const bpvm_bmgr_familia_t FAM_CON_ESPEJO  = { SECT, NULL, fam_leer, g_espejo, fam_escribir, NULL, NULL };

/* Un ENV_SET como lo hace el común: las copias de la flash a la RAM, el gestor edita la rancia y se escribe. */
static int fam_env_set(const char* key, const char* value) {
    static uint8_t a[SECT], b[SECT], sc[SECT];
    bpvm_bmgr_t bm;
    memset(&bm, 0, sizeof bm);
    memcpy(a, g_fl[0], SECT);
    memcpy(b, g_fl[1], SECT);
    bm.a = a; bm.b = b; bm.scratch = sc; bm.sector = SECT;
    int slot = -1;
    if (bpvm_bmgr_env_set(&bm, key, value, &slot) != 0) return -2;
    return bpvm_bmgr_escribir(slot, slot == 0 ? a : b);
}

static int zf_erase(void* u, uint32_t off, uint32_t len) { memset((uint8_t*) u + off, 0xFF, len); return 0; }
static int zf_program(void* u, uint32_t off, const uint8_t* d, uint32_t len) { memcpy((uint8_t*) u + off, d, len); return 0; }
static void be32w(uint8_t* p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t) v; }
static void be16w(uint8_t* p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t) v; }
static void pon_pack_con_motor(uint8_t* p, uint32_t total, uint32_t ram, uint32_t data, uint32_t bss) {
    memset(p, 0xFF, total);
    memset(p, 0, 128);
    be32w(p, BPVM_PACK_MAGIC); be16w(p + 4, 1); be16w(p + 6, 0xFFFF); be32w(p + 8, total);
    strcpy((char*) p + 12, "SQLite");
    uint8_t* e = p + 128;
    memset(e, 0, 48 + BPVM_NPACK_HDR_BYTES);
    memcpy(e, "npk", 3); strcpy((char*) e + 4, "sqlite"); be32w(e + 36, BPVM_NPACK_HDR_BYTES);
    bpvm_npack_hdr_t h; memset(&h, 0, sizeof h);
    h.magic = BPVM_NPACK_MAGIC; h.linked_ram = ram; h.data_bytes = data; h.bss_bytes = bss;
    memcpy(e + 48, &h, sizeof h);
    be16w(p + 64, bpvm_pack_crc16(BPVM_PACK_CRC16_INIT, p + 128, 48u + BPVM_NPACK_HDR_BYTES));
    uint16_t crc = bpvm_pack_crc16(BPVM_PACK_CRC16_INIT, p, 6);
    be16w(p + 126, bpvm_pack_crc16(crc, p + 8, 126 - 8));
}

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

    /* --- 9. V7/H6.c G1: PACK_BURN_BEGIN dice el TOPE, no la base -------------
     * La placa le dice al IDE contra qué RAM sellar el motor que va a grabar.
     * Hasta G1 era siempre la base del bloque, y con dos motores eso ponía el
     * segundo ENCIMA del primero. Ahora es lo primero libre por encima de los
     * vivos, y `ramSize` lo que queda: el IDE mira si cabe ANTES de grabar. */
    {
        static union { uint8_t b[64u * 1024u]; uint32_t al; } zona;
        memset(zona.b, 0xFF, sizeof zona.b);
        bpvm_pack_flash_t fl = { zf_erase, zf_program, zona.b, 4096u };
        bpvm_bmgr_t bmp;
        memset(&bmp, 0, sizeof bmp);
        bmp.packs_base = zona.b; bmp.packs_size = (uint32_t) sizeof zona.b; bmp.packs_flash = &fl;
        bmp.pack_ram_base = 0x48000000u; bmp.pack_ram_size = 512u * 1024u;
        bpvm_bmgr_req_t rq;
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "PACK_BURN_BEGIN");
        rq.id = 9; rq.size = 4096; rq.has_size = 1;
        char rep[512];
        int wr3 = -1;

        int nn = bpvm_bmgr_wire_dispatch(&bmp, &rq, rep, sizeof rep, &wr3);
        CHECK(nn > 0 && strstr(rep, "\"ramBase\":1207959552") != NULL
                     && strstr(rep, "\"ramSize\":524288") != NULL,
              "G1: zona vacia -> ramBase = la BASE del bloque, ramSize = el bloque entero");

        /* SQLite ya grabado: 6916 + 1368 = 8284 B de estáticos -> 8288. */
        pon_pack_con_motor(zona.b, 4096u, 0x48000000u, 6916u, 1368u);
        nn = bpvm_bmgr_wire_dispatch(&bmp, &rq, rep, sizeof rep, &wr3);
        CHECK(nn > 0 && strstr(rep, "\"offset\":4096") != NULL,
              "G1: con un pack grabado, el siguiente va detras (offset 4096)");
        CHECK(strstr(rep, "\"ramBase\":1207967840") != NULL,
              "G1: ...y su RAM TAMBIEN detras: ramBase = base + 8288 (el tope)");
        CHECK(strstr(rep, "\"ramSize\":516000") != NULL,
              "G1: ...y ramSize = lo que queda (524288 - 8288)");

        /* Sin bloque de RAM: 0 y 0, que el IDE traduce en «esta placa no da RAM». */
        bmp.pack_ram_base = 0; bmp.pack_ram_size = 0;
        nn = bpvm_bmgr_wire_dispatch(&bmp, &rq, rep, sizeof rep, &wr3);
        CHECK(nn > 0 && strstr(rep, "\"ramBase\":0") != NULL && strstr(rep, "\"ramSize\":0") != NULL,
              "G1: sin bloque de RAM -> ramBase 0 (el IDE avisa, no graba un motor muerto)");
    }

    /* --- 10. V7 #671 (8-oct, Eduardo: «671 a»): con motores YA CARGADOS, cambiar la zona reinicia la placa ------
     * Los motores se cargan una vez por arranque y sus tablas apuntan a la zona de antes (la DK2 cayó así, en un
     * HardFault). Aquí, la mitad del común: la respuesta lo dice ("reinicia":true) y deja el reinicio PENDIENTE para el
     * REPL, que lo hace tras mandarla. Sin motores cargados, nada. */
    {
        static union { uint8_t b[64u * 1024u]; uint32_t al; } zona2;
        static uint8_t img[8192];
        memset(zona2.b, 0xFF, sizeof zona2.b);
        bpvm_pack_flash_t fl2 = { zf_erase, zf_program, zona2.b, 4096u };
        bpvm_bmgr_t bmr;
        memset(&bmr, 0, sizeof bmr);
        bmr.packs_base = zona2.b; bmr.packs_size = (uint32_t) sizeof zona2.b; bmr.packs_flash = &fl2;
        bpvm_bmgr_req_t rq;
        char rep[512];
        int wr = -1;
        (void) bpvm_bmgr_wire_reinicio_pendiente();   /* limpio */

        g_cargados = 0;
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "PACK_FORMAT"); rq.id = 31; rq.confirm_yes = 1;
        int nn = bpvm_bmgr_wire_dispatch(&bmr, &rq, rep, sizeof rep, &wr);
        CHECK(nn > 0 && strstr(rep, "PACK_FORMAT_REPLY") && !strstr(rep, "reinicia") && !bpvm_bmgr_wire_reinicio_pendiente(),
              "#671: sin motores cargados, PACK_FORMAT no reinicia (los de la zona nueva, en el siguiente INFO)");

        g_cargados = 1;
        pon_pack_con_motor(zona2.b, 4096u, 0x48000000u, 6916u, 1368u);
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "PACK_DEL"); rq.id = 32; rq.off = 0; rq.has_off = 1;
        nn = bpvm_bmgr_wire_dispatch(&bmr, &rq, rep, sizeof rep, &wr);
        CHECK(nn > 0 && strstr(rep, "PACK_DEL_REPLY") && strstr(rep, "\"reinicia\":true"),
              "#671: con motores cargados, PACK_DEL lo dice en la respuesta (\"reinicia\":true)");
        CHECK(bpvm_bmgr_wire_reinicio_pendiente() == 1 && bpvm_bmgr_wire_reinicio_pendiente() == 0,
              "#671: ...y deja el reinicio pendiente UNA vez (el REPL lo hace tras mandarla)");

        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "PACK_FORMAT"); rq.id = 33; rq.confirm_yes = 0;
        nn = bpvm_bmgr_wire_dispatch(&bmr, &rq, rep, sizeof rep, &wr);
        CHECK(nn > 0 && strstr(rep, "falta confirm=YES") && !bpvm_bmgr_wire_reinicio_pendiente(),
              "#671: un PACK_FORMAT rechazado (sin confirm) no reinicia nada");

        rq.confirm_yes = 1; rq.id = 34;
        nn = bpvm_bmgr_wire_dispatch(&bmr, &rq, rep, sizeof rep, &wr);
        CHECK(nn > 0 && strstr(rep, "\"reinicia\":true") && bpvm_bmgr_wire_reinicio_pendiente() == 1,
              "#671: con motores cargados, PACK_FORMAT tambien");

        /* y PACK_BURN_END: un pack entero por el wire (BEGIN, DATA y END), en la zona recién formateada */
        pon_pack_con_motor(img, (uint32_t) sizeof img, 0x48000000u, 6916u, 1368u);
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "PACK_BURN_BEGIN"); rq.id = 35; rq.size = (long) sizeof img; rq.has_size = 1;
        nn = bpvm_bmgr_wire_dispatch(&bmr, &rq, rep, sizeof rep, &wr);
        int bien = nn > 0 && strstr(rep, "PACK_BURN_BEGIN_REPLY") != NULL;
        for (uint32_t o = 0; bien && o < (uint32_t) sizeof img; o += 1024u) {
            memset(&rq, 0, sizeof rq);
            snprintf(rq.type, sizeof rq.type, "PACK_BURN_DATA"); rq.id = 36;
            rq.bulk = img + o; rq.bulk_len = 1024;
            nn = bpvm_bmgr_wire_dispatch(&bmr, &rq, rep, sizeof rep, &wr);
            bien = nn > 0 && strstr(rep, "PACK_BURN_DATA_REPLY") != NULL;
        }
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "PACK_BURN_END"); rq.id = 37;
        if (bien) nn = bpvm_bmgr_wire_dispatch(&bmr, &rq, rep, sizeof rep, &wr);
        CHECK(bien && nn > 0 && strstr(rep, "PACK_BURN_END_REPLY") && strstr(rep, "\"reinicia\":true")
              && bpvm_bmgr_wire_reinicio_pendiente() == 1,
              "#671: con motores cargados, PACK_BURN_END tambien");
        if (!bien) printf("    (el pack no llego entero: %s)\n", rep);
        g_cargados = 0;
    }

    /* V7/H7 (30-sep) — el FS ARRIBA (la P4 Waveshare y la C5: 32 MB con una tabla que acaba en 16): PART_LS cuenta
     * el reparto de verdad (el FS, el trozo de arriba; los packs, toda la zona) SIN mirar el env, y PART_APPLY y
     * PART_DEFAULTS no reparten: lo dicen. Con fs_arriba a 0, lo de siempre (lo prueban los casos de arriba). */
    {
        static uint8_t fa[SECT], fb[SECT], fsc[SECT];   /* un env VIRGEN: con el FS arriba no hace falta */
        memset(fa, 0xFF, SECT); memset(fb, 0xFF, SECT);
        bpvm_bmgr_t bmf;
        memset(&bmf, 0, sizeof bmf);
        bmf.a = fa; bmf.b = fb; bmf.scratch = fsc;
        bmf.sector = SECT; bmf.part_base = 0u; bmf.usable_flash = 0x9E8000u; bmf.fs_arriba = 0x1000000u;
        bpvm_bmgr_req_t rq;
        char rep[512];
        int wr = -1;
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "PART_LS"); rq.id = 21;
        int nn = bpvm_bmgr_wire_dispatch(&bmf, &rq, rep, sizeof rep, &wr);
        CHECK(nn > 0 && strstr(rep, "\"fsArriba\":16777216") != NULL && strstr(rep, "\"missing\":false") != NULL,
              "FS arriba: PART_LS lo dice (fsArriba) y no pide el env");
        CHECK(strstr(rep, "{\"name\":\"fs\",\"offset\":10387456,\"size\":16777216}") != NULL
              && strstr(rep, "{\"name\":\"packs\",\"offset\":0,\"size\":10387456}") != NULL,
              "FS arriba: el FS, el trozo de arriba entero; los packs, toda la zona");
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "PART_APPLY"); rq.id = 22;
        rq.part_sizes[0] = 1048576; rq.part_sizes[1] = -1;
        nn = bpvm_bmgr_wire_dispatch(&bmf, &rq, rep, sizeof rep, &wr);
        CHECK(nn > 0 && strstr(rep, "\"code\":\"UNSUPPORTED\"") != NULL && strstr(rep, "no se reparte") != NULL
              && wr == -1, "FS arriba: PART_APPLY no reparte, lo dice, y no escribe el env");
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "PART_DEFAULTS"); rq.id = 23;
        nn = bpvm_bmgr_wire_dispatch(&bmf, &rq, rep, sizeof rep, &wr);
        CHECK(nn > 0 && strstr(rep, "\"code\":\"UNSUPPORTED\"") != NULL,
              "FS arriba: PART_DEFAULTS tampoco propone un reparto");
    }

    /* ── V7/H10, el board_mgr común (3-oct): la VISTA del ENV es la de la flash (D1) y el `anadir` común (#626), con
     * las dos formas de leer las copias. Una escritura que no llega deja el valor de antes (D2). ── */
    for (int forma = 0; forma < 2; forma++) {
        const char* f = forma == 0 ? "en su sitio" : "con espejo";
        char val[64];
        memset(g_fl, 0xFF, sizeof g_fl);
        g_fl_falla = 0;
        g_fl_escrituras = 0;
        bpvm_bmgr_familia(forma == 0 ? &FAM_EN_SU_SITIO : &FAM_CON_ESPEJO);
        CHECK(!bpvm_bmgr_vista()->valid, "board_mgr común: la flash borrada, la vista sin ENV");

        CHECK(bpvm_bmgr_anadir("imagen.fecha", "123") == 0 && g_fl_escrituras == 1,
              "board_mgr común: anadir una clave que falta, escribe una vez");
        CHECK(bpvm_env_get_exacto(bpvm_bmgr_vista(), "imagen.fecha", val, sizeof val) == 3 && !strcmp(val, "123"),
              "board_mgr común: y la vista la ve sin reiniciar (D1)");
        CHECK(bpvm_bmgr_anadir("imagen.fecha", "999") == 1 && g_fl_escrituras == 1,
              "board_mgr común: anadir la que ya está no escribe (y no la cambia)");

        CHECK(fam_env_set("gc", "0") == 0 && fam_env_set("gc", "1") == 0,
              "board_mgr común: dos ENV_SET seguidos");
        CHECK(bpvm_env_get_exacto(bpvm_bmgr_vista(), "gc", val, sizeof val) == 1 && !strcmp(val, "1")
              && bpvm_env_get_exacto(bpvm_bmgr_vista(), "imagen.fecha", val, sizeof val) == 3,
              "board_mgr común: la vista ve el segundo, y lo anterior sigue (#638)");

        g_fl_falla = 1;
        CHECK(fam_env_set("gc", "7") == -1, "board_mgr común: con la flash que no escribe, la escritura falla");
        CHECK(bpvm_env_get_exacto(bpvm_bmgr_vista(), "gc", val, sizeof val) == 1 && !strcmp(val, "1"),
              "board_mgr común: y la vista sigue con el valor de antes (D2)");
        CHECK(bpvm_bmgr_anadir("otra", "x") == -1, "board_mgr común: anadir con la flash que no escribe: -1");
        g_fl_falla = 0;
        (void) f;
    }
    bpvm_bmgr_familia(NULL);
    CHECK(!bpvm_bmgr_vista()->valid, "board_mgr común: sin familia, la vista sin ENV");

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
