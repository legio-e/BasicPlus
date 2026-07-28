/*
 * test_lfs_dirty.c — #330 / secuela de #328: ¿qué le pasa a littlefs cuando la
 * región donde va a vivir NO está borrada?
 *
 * POR QUÉ ESTE BANCO. El S3 (#328) montaba un volumen aparentemente sano
 * ("0 ficheros, 8192 B usados" = sólo los 2 superbloques) y fallaba TODA
 * escritura de fichero con LFS_ERR_CORRUPT, con 7,8 MB libres. Se arregló
 * borrando el chip entero, y dimos por buena la explicación "flash sucia de la
 * disposición anterior". Pero esa explicación tiene un agujero: littlefs BORRA
 * cada bloque antes de programarlo, así que el contenido previo no debería
 * importarle. Antes de escribir un arreglo que cuesta un minuto de arranque
 * (borrar la región entera al formatear), hay que VER el mecanismo.
 *
 * Aquí es gratis: en el host, en milisegundos, y con las trazas de littlefs
 * encendidas (los test-lfs* compilan el motor sin LFS_NO_*).
 *
 * FIDELIDAD. lfs_filebd NO sirve para esto: borra a 0x00 y programa
 * sobrescribiendo. La NOR real hace otras dos cosas, y son justo las que
 * importan aquí:
 *   - borrar deja los bits a 1 (0xFF),
 *   - programar sólo puede bajar 1→0 (AND), nunca subir 0→1.
 * Con esa semántica, escribir sobre algo NO borrado deja basura: el byte
 * resultante es (viejo AND nuevo), no el nuevo. Ese es el mecanismo que hay
 * que confirmar o descartar.
 *
 * ESCENARIOS (el 2 y el 3 son hipótesis distintas, y conviene no confundirlas):
 *   1 limpio   — región a 0xFF. Control: tiene que ir bien.
 *   2 sucio    — región con basura, pero el erase FUNCIONA. Si littlefs borra
 *                antes de programar, deberia ir bien IGUAL que el 1. Si falla,
 *                es que hay caminos que programan sin borrar.
 *   3 sin-erase— región a 0xFF pero el erase es un NO-OP (simula un borrado que
 *                falla en silencio, p.ej. una cintura de flash que devuelve OK
 *                sin hacer nada). Aísla "no se borra" de "estaba sucio".
 *
 * Cada escenario repite la secuencia REAL del firmware (fs_init_at):
 * mount→(falla)→format→mount→mkdir /sys /lib /app→escribir un .mod→releer.
 */
#include "lfs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BLOCK_SIZE   4096u
#define BLOCK_COUNT  64u                 /* 256 KB: de sobra y rápido */
#define REGION_LEN   (BLOCK_SIZE * BLOCK_COUNT)

/* Geometría IDÉNTICA a la del firmware (fs_lfs_esp32.c / fs_lfs_pico.c): si el
 * fallo dependiera de estos números, con otros no se reproduciría. */
#define RD_SIZE      256u
#define PR_SIZE      256u
#define CACHE_SIZE   256u
#define LOOKAHEAD    64u

static uint8_t region[REGION_LEN];
static int     erase_es_nop = 0;   /* escenario 3 */
static long    n_erase = 0, n_prog = 0;

/* ── block-device con semántica NOR de verdad ─────────────────────────────── */

static int bd_read(const struct lfs_config* c, lfs_block_t blk,
                   lfs_off_t off, void* buf, lfs_size_t size) {
    (void) c;
    memcpy(buf, region + blk * BLOCK_SIZE + off, size);
    return 0;
}

static int bd_prog(const struct lfs_config* c, lfs_block_t blk,
                   lfs_off_t off, const void* buf, lfs_size_t size) {
    (void) c;
    /* LA CLAVE: programar es un AND. Sobre flash borrada (0xFF) equivale a
     * escribir; sobre flash sucia, el resultado NO es lo que pediste. */
    uint8_t*       d = region + blk * BLOCK_SIZE + off;
    const uint8_t* s = (const uint8_t*) buf;
    for (lfs_size_t i = 0; i < size; i++) d[i] &= s[i];
    n_prog++;
    return 0;
}

static int bd_erase(const struct lfs_config* c, lfs_block_t blk) {
    (void) c;
    n_erase++;
    if (erase_es_nop) return 0;               /* "OK" sin hacer nada */
    memset(region + blk * BLOCK_SIZE, 0xFF, BLOCK_SIZE);
    return 0;
}

static int bd_sync(const struct lfs_config* c) { (void) c; return 0; }

static uint8_t rb[RD_SIZE], pb[PR_SIZE];
static uint8_t lb[LOOKAHEAD] __attribute__((aligned(8)));

static const struct lfs_config cfg = {
    .read = bd_read, .prog = bd_prog, .erase = bd_erase, .sync = bd_sync,
    .read_size = RD_SIZE, .prog_size = PR_SIZE,
    .block_size = BLOCK_SIZE, .block_count = BLOCK_COUNT,
    .block_cycles = 500, .cache_size = CACHE_SIZE, .lookahead_size = LOOKAHEAD,
    .read_buffer = rb, .prog_buffer = pb, .lookahead_buffer = lb,
};

/* ── la secuencia REAL del firmware ───────────────────────────────────────── */

#define MOD_LEN 2894u   /* el tamaño exacto de Core.mod, el 1º que falló */

static int corre(const char* nombre) {
    lfs_t lfs;
    int fallos = 0;

    printf("\n=== escenario: %s ===\n", nombre);
    n_erase = n_prog = 0;

    /* fs_init_at: intenta montar; si no, formatea y remonta. */
    int e = lfs_mount(&lfs, &cfg);
    if (e < 0) {
        printf("  mount inicial falla (%d) -> format, como el firmware\n", e);
        e = lfs_format(&lfs, &cfg);
        if (e < 0) { printf("  !! FORMAT falla: %d\n", e); return 1; }
        e = lfs_mount(&lfs, &cfg);
        if (e < 0) { printf("  !! MOUNT tras format falla: %d\n", e); return 1; }
    }
    printf("  montado OK\n");

    /* los 3 mkdir de fs_init_at, cuyo retorno el firmware IGNORA (por eso el
     * S3 se quedaba en 8192 B: ya fallaban aquí y nadie se enteraba) */
    const char* dirs[] = { "/sys", "/lib", "/app" };
    for (int i = 0; i < 3; i++) {
        e = lfs_mkdir(&lfs, dirs[i]);
        if (e < 0 && e != LFS_ERR_EXIST) {
            printf("  !! mkdir %s -> %d\n", dirs[i], e);
            fallos++;
        }
    }

    /* el PUT que fallaba: /lib/Core.mod, 2894 B */
    static uint8_t datos[MOD_LEN];
    for (unsigned i = 0; i < MOD_LEN; i++) datos[i] = (uint8_t)(i * 31u + 7u);

    lfs_file_t f;
    static uint8_t fbuf[CACHE_SIZE];
    struct lfs_file_config fcfg; memset(&fcfg, 0, sizeof fcfg); fcfg.buffer = fbuf;

    e = lfs_file_opencfg(&lfs, &f, "/lib/Core.mod",
                         LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC, &fcfg);
    if (e < 0) { printf("  !! open(w) -> %d\n", e); fallos++; }
    else {
        lfs_ssize_t n = lfs_file_write(&lfs, &f, datos, MOD_LEN);
        if (n < 0 || (uint32_t) n != MOD_LEN) { printf("  !! write -> %ld\n", (long) n); fallos++; }
        e = lfs_file_close(&lfs, &f);
        if (e < 0) { printf("  !! close (aqui COMMITEA) -> %d\n", e); fallos++; }
    }

    /* releer y comparar: que el open+write digan OK no basta */
    if (!fallos) {
        static uint8_t leido[MOD_LEN];
        e = lfs_file_opencfg(&lfs, &f, "/lib/Core.mod", LFS_O_RDONLY, &fcfg);
        if (e < 0) { printf("  !! open(r) -> %d\n", e); fallos++; }
        else {
            lfs_ssize_t n = lfs_file_read(&lfs, &f, leido, MOD_LEN);
            lfs_file_close(&lfs, &f);
            if (n < 0 || (uint32_t) n != MOD_LEN) { printf("  !! read -> %ld\n", (long) n); fallos++; }
            else if (memcmp(datos, leido, MOD_LEN) != 0) { printf("  !! los bytes NO coinciden\n"); fallos++; }
        }
    }

    lfs_ssize_t usados = lfs_fs_size(&lfs);
    printf("  usados: %ld bloques (%ld B) | erase=%ld prog=%ld\n",
           (long) usados, (long) usados * BLOCK_SIZE, n_erase, n_prog);
    lfs_unmount(&lfs);

    printf("  RESULTADO: %s\n", fallos ? "FALLA" : "va bien");
    return fallos ? 1 : 0;
}

int main(void) {
    int malos = 0;

    /* 1 — limpio (control) */
    memset(region, 0xFF, REGION_LEN);
    erase_es_nop = 0;
    if (corre("1 limpio (region a 0xFF)")) { printf("  ^ el CONTROL falla: el banco esta mal\n"); malos++; }

    /* 2 — sucio, con erase funcionando. La explicacion que dimos por buena. */
    for (unsigned i = 0; i < REGION_LEN; i++) region[i] = (uint8_t)(i * 131u + 17u);
    erase_es_nop = 0;
    malos += corre("2 SUCIO (basura previa) pero el erase FUNCIONA");

    /* 3 — limpio + erase NO-OP. Sanity: partiendo de 0xFF y sin reusar bloques,
     * que el borrado no haga nada da IGUAL (no había nada que borrar). Sirve
     * para no confundir "el erase miente" con "el erase miente Y hay basura". */
    memset(region, 0xFF, REGION_LEN);
    erase_es_nop = 1;
    malos += corre("3 limpio + ERASE NO-OP (control: deberia dar igual)");

    /* 4 — EL CASO REAL de una cintura de flash rota: hay basura previa Y el
     * borrado dice OK sin hacer nada. Entonces programar = (basura AND dato),
     * littlefs relee, no coincide, y sale CORRUPT. */
    for (unsigned i = 0; i < REGION_LEN; i++) region[i] = (uint8_t)(i * 131u + 17u);
    erase_es_nop = 1;
    printf("\n(el 4 DEBE fallar: es el sintoma del #328 reproducido)\n");
    if (corre("4 SUCIO + ERASE NO-OP (el borrado miente)") == 0) {
        printf("  ^ OJO: esperaba que fallara y no falla\n");
        malos++;
    }

    printf("\n================ veredicto ================\n");
    printf("2 va bien  -> la flash sucia SOLA no rompe: littlefs borra antes de\n"
           "              programar. La explicacion de ayer NO se sostiene.\n"
           "4 falla    -> lo que rompe es que el BORRADO NO OCURRA de verdad.\n"
           "=> el #328 no era 'flash sucia': era que el erase de esos sectores\n"
           "   no estaba surtiendo efecto. Y eso NO lo arregla el #330 tal como\n"
           "   estaba planteado (borrar la region al formatear usa EL MISMO\n"
           "   erase que no funciona).\n");
    return malos ? 1 : 0;
}
