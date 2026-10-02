/*
 * test_fsfallo.c — V7/H10 #628 (2-oct): EL MOTIVO DE UN FALLO DEL FS, en los motores y en la fachada.
 *
 * El motivo (`bpvm_fs_last_fail`, lo que el REPL convierte en el código de error de DEL, PUT y SAVE) vivía en
 * fs_lfs.c: FAT y el motor del PC no anotaban nada, y dentro de littlefs sólo `write` y el cursor —un DEL que
 * fallaba contestaba con el motivo de la operación ANTERIOR—. Esto fuerza cada clase de fallo en el motor del PC y
 * en littlefs (FAT, por el cable y en el micro simulado: `sd_smoke`) y, sobre todo, lo que lo rompía: que un fallo NO
 * herede el motivo de otro —un motor de mentira que falla sin decir nada—. Y la línea del log, con el motor.
 */
#include "bpvm_fs.h"
#include "bpvm_log.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int g_fail = 0;
#define CHECK(c, m) do { if (c) printf("  ok  : %s\n", (m)); \
                         else { printf("  FAIL: %s (motivo: %s)\n", (m), clase(bpvm_fs_last_fail())); g_fail++; } } while (0)

static const char* clase(bpvm_fs_fail_t f) {
    switch (f) {
        case BPVM_FS_FAIL_NO_SPACE:      return "NO_SPACE";
        case BPVM_FS_FAIL_TOO_BIG:       return "TOO_BIG";
        case BPVM_FS_FAIL_NAME_TOO_LONG: return "NAME_TOO_LONG";
        case BPVM_FS_FAIL_EXISTS:        return "EXISTS";
        case BPVM_FS_FAIL_NOT_FOUND:     return "NOT_FOUND";
        case BPVM_FS_FAIL_IO:            return "IO";
        default:                         return "OTHER";
    }
}
static int es(bpvm_fs_fail_t f) { return bpvm_fs_last_fail() == f; }

/* El log, en RAM: lo que escribe bpvm_fs_log_fail. */
static uint8_t g_logbuf[4096];
static char    g_vol[4096];
static size_t  g_vol_n;
static void vuelca(const char* d, size_t n, void* u) {
    (void) u;
    if (g_vol_n + n < sizeof g_vol) { memcpy(g_vol + g_vol_n, d, n); g_vol_n += n; g_vol[g_vol_n] = '\0'; }
}
static const char* linea_del_fallo(const char* op, const char* path) {
    log_clear_ram();
    bpvm_fs_log_fail(op, path, 0, 0, 0);
    g_vol_n = 0; g_vol[0] = '\0';
    log_dump(vuelca, NULL);
    return g_vol;
}

/* Un motor de mentira que falla SIN decir por qué: el caso que la fachada tiene que cubrir. */
static int mudo_stat(const char* p, uint32_t* s) { (void) p; (void) s; return -1; }
static int mudo_remove(const char* p) { (void) p; return -1; }
static const bpvm_fs_backend_t MUDO = { .stat = mudo_stat, .remove = mudo_remove };

int main(void) {
    bpvm_log_cintura_t lc;
    memset(&lc, 0, sizeof lc);
    lc.region_buf = g_logbuf;
    lc.region_size = (uint32_t) sizeof g_logbuf;
    bpvm_log_init(&lc);

    printf("--- #628: el motor del PC ---\n");
    bpvm_fs_register_host();
    CHECK(bpvm_fs_mount("/mudo", &MUDO) == 0, "montado un motor que falla sin decir por que, en /mudo");
    bpvm_fs_mkdir("build/fsfallo");
    bpvm_fs_remove("build/fsfallo/f");
    bpvm_fs_rmdir("build/fsfallo/f");

    CHECK(bpvm_fs_remove("build/fsfallo/no-esta.txt") == -1 && es(BPVM_FS_FAIL_NOT_FOUND),
          "borrar lo que no esta: NOT_FOUND");
    const char* l = linea_del_fallo("del", "build/fsfallo/no-esta.txt");
    CHECK(strstr(l, "FALLO host=2 ENOENT (no existe)") != NULL, "la linea del log dice el motor y su codigo");
    CHECK(bpvm_fs_write("build/fsfallo/no/hay/x.txt", (const uint8_t*) "a", 1, 0) == -1 && es(BPVM_FS_FAIL_NOT_FOUND),
          "escribir en una carpeta que no esta: NOT_FOUND");
    CHECK(bpvm_fs_write("build/fsfallo/f", (const uint8_t*) "x", 1, 0) == 0, "un fichero f");
    CHECK(bpvm_fs_mkdir("build/fsfallo/f") == -1 && es(BPVM_FS_FAIL_EXISTS),
          "una carpeta donde hay un fichero: EXISTS");
    /* EL CASO: tras un EXISTS, un motor que falla sin decir nada. Antes, el EXISTS de antes. */
    CHECK(bpvm_fs_remove("/mudo/x") == -1 && es(BPVM_FS_FAIL_OTHER),
          "un motor que falla sin decir por que: OTHER, no el EXISTS de antes");
    l = linea_del_fallo("del", "/mudo/x");
    CHECK(strstr(l, "FALLO fs=0 el motor no dice por que") != NULL, "...y la linea lo dice");
    CHECK(bpvm_fs_write("build/fsfallo/no/hay/x.txt", (const uint8_t*) "a", 1, 0) == -1 && es(BPVM_FS_FAIL_NOT_FOUND),
          "y el siguiente fallo vuelve a ser el suyo: NOT_FOUND");
    CHECK(bpvm_fs_mkdir("/mudo/d") == -1 && es(BPVM_FS_FAIL_OTHER)
          && strstr(linea_del_fallo("mkdir", "/mudo/d"), "esa unidad no lo hace") != NULL,
          "una unidad sin esa operacion: OTHER, «esa unidad no lo hace»");
    CHECK(bpvm_fs_rename("build/fsfallo/f", "/mudo/f") == -1 && es(BPVM_FS_FAIL_OTHER)
          && strstr(linea_del_fallo("rename", "build/fsfallo/f"), "entre unidades distintas") != NULL,
          "mover entre unidades: OTHER, y lo dice");
    {
        uint8_t chico[16];
        CHECK(bpvm_fs_fopen(chico, (uint32_t) sizeof chico, "build/fsfallo/f", BPVM_FS_O_READ) == -1
              && es(BPVM_FS_FAIL_TOO_BIG), "un bloque de fichero que no cabe: TOO_BIG");
        static uint8_t blk[512];
        CHECK(bpvm_fs_fopen(blk, (uint32_t) sizeof blk, "build/fsfallo/no-esta.txt", BPVM_FS_O_READ) == -1
              && es(BPVM_FS_FAIL_NOT_FOUND), "abrir para leer lo que no esta: NOT_FOUND");
        CHECK(bpvm_fs_fread(blk, chico, 1) == -1 && es(BPVM_FS_FAIL_OTHER), "leer de un fichero cerrado: OTHER");
    }
    bpvm_fs_remove("build/fsfallo/f");

    printf("--- #628: littlefs ---\n");
    const char* IMG = "build/fsfallo.img";
    remove(IMG);
    CHECK(bpvm_fs_register_lfs_filebd(IMG, 4096, 16, 1) == 0, "littlefs de 64 KB, recien formateado");
    CHECK(bpvm_fs_mount("/mudo", &MUDO) == 0, "y el motor mudo otra vez en /mudo");
    CHECK(bpvm_fs_remove("/no-esta.txt") == -1 && es(BPVM_FS_FAIL_NOT_FOUND),
          "DEL de lo que no esta: NOT_FOUND (antes: el motivo de la operacion anterior)");
    l = linea_del_fallo("del", "/no-esta.txt");
    CHECK(strstr(l, "FALLO lfs=-2 NOENT (no existe)") != NULL, "la linea del log, la de siempre: «FALLO lfs=-2 NOENT»");
    {
        static uint8_t lleno[12 * 1024];
        int i, escritos = 0;
        char p[32];
        for (i = 0; i < 12; i++) {
            snprintf(p, sizeof p, "/f%d", i);
            if (bpvm_fs_write(p, lleno, (uint32_t) sizeof lleno, 0) != 0) break;
            escritos++;
        }
        CHECK(i < 12 && es(BPVM_FS_FAIL_NO_SPACE), "llenar el volumen: NO_SPACE");
        printf("       (%d ficheros de 12 KB antes de llenarse)\n", escritos);
    }
    /* EL CASO en littlefs: tras un NO_SPACE, otro fallo. Antes, el NO_SPACE de antes. */
    CHECK(bpvm_fs_remove("/tampoco.txt") == -1 && es(BPVM_FS_FAIL_NOT_FOUND),
          "tras un NO_SPACE, borrar lo que no esta: NOT_FOUND, no el NO_SPACE de antes");
    CHECK(bpvm_fs_mkdir("/f0") == -1 && es(BPVM_FS_FAIL_EXISTS), "una carpeta donde hay un fichero: EXISTS");
    {
        char largo[300];
        largo[0] = '/';
        memset(largo + 1, 'a', 280);
        largo[281] = '\0';
        CHECK(bpvm_fs_write(largo, (const uint8_t*) "x", 1, 0) == -1 && es(BPVM_FS_FAIL_NAME_TOO_LONG),
              "un nombre de 280 caracteres: NAME_TOO_LONG");
    }
    CHECK(bpvm_fs_rmdir("/f0") == -1 && es(BPVM_FS_FAIL_OTHER), "rmdir de un fichero: OTHER (NOTDIR)");
    CHECK(bpvm_fs_remove("/mudo/x") == -1 && es(BPVM_FS_FAIL_OTHER), "y el motor mudo, tambien aqui: OTHER");
    bpvm_fs_lfs_filebd_close();
    remove(IMG);

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
