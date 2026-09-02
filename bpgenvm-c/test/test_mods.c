/* test_mods.c — la regla del /lib de la imagen (#466) contra un littlefs REAL en
 * fichero (el mismo oráculo que test_fs_vfs). Cada caso es una situación que las
 * placas han tenido de verdad: el /lib de V5 (MOD6) bajo una imagen MOD7 (la
 * Metro, 2-sep), un módulo recompilado con la misma versión (CRC distinto), uno
 * más nuevo que la imagen (se deja), basura, y el /app del usuario (no se toca).
 *
 *   make test-mods
 */
#include "bpvm_mods.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <string.h>

static int fallos = 0;
#define CHECK(cond, ...) do { if (cond) printf("  ok  : " __VA_ARGS__); \
                              else { printf("  FAIL: " __VA_ARGS__); fallos++; } printf("\n"); } while (0)

static int put_fs(const char* p, const uint8_t* d, uint32_t n) { return bpvm_fs_write(p, d, n, 0); }
static int put_roto(const char* p, const uint8_t* d, uint32_t n) { (void) p; (void) d; (void) n; return -1; }

static int mismo_contenido(const char* path, const uint8_t* data, uint32_t len) {
    uint8_t buf[64]; long n = bpvm_fs_read(path, buf, sizeof buf);
    return n == (long) len && memcmp(buf, data, len) == 0;
}

int main(void) {
    const char* IMG = "build/fs_mods_test.img";
    remove(IMG);
    if (bpvm_fs_register_lfs_filebd(IMG, 4096, 64, 1) != 0) { printf("FAIL: attach\n"); return 1; }
    bpvm_fs_mkdir("/lib"); bpvm_fs_mkdir("/app");
    printf("=== test_mods: la regla del /lib de la imagen ===\n");

    static const uint8_t EMB[]  = "MOD7--la stdlib de la imagen--";   /* 30 B + NUL */
    static const uint8_t V5[]   = "MOD6--la stdlib del dist V5--x";   /* mismo tamaño, MOD6 */
    static const uint8_t OTRO[] = "MOD7--recompilada con arreglo-";   /* mismo tamaño, otro CRC */
    static const uint8_t CORTO[]= "MOD7--corta";
    static const uint8_t NUEVO[]= "MOD8--de una imagen futura----";
    const uint32_t L = (uint32_t) sizeof EMB;
    char linea[160]; bpvm_mods_res_t r;

    r = bpvm_mods_sincronizar("/lib/Math.mod", EMB, L, put_fs, linea, sizeof linea);
    CHECK(r == BPVM_MODS_INSTALADO && !strncmp(linea, "preinstall:", 11), "falta → INSTALADO: %s", linea);
    CHECK(mismo_contenido("/lib/Math.mod", EMB, L), "y el FS tiene el embebido");

    r = bpvm_mods_sincronizar("/lib/Math.mod", EMB, L, put_fs, linea, sizeof linea);
    CHECK(r == BPVM_MODS_IGUAL && linea[0] == 0, "idéntico → IGUAL, en silencio");

    bpvm_fs_write("/lib/Math.mod", V5, L, 0);
    r = bpvm_mods_sincronizar("/lib/Math.mod", EMB, L, put_fs, linea, sizeof linea);
    CHECK(r == BPVM_MODS_REPUESTO && strstr(linea, "versión anterior") && strstr(linea, "MOD6") && strstr(linea, "MOD7"),
          "MOD6 bajo imagen MOD7 (la Metro) → REPUESTO: %s", linea);
    CHECK(mismo_contenido("/lib/Math.mod", EMB, L), "y el FS vuelve a tener el embebido");

    bpvm_fs_write("/lib/Math.mod", OTRO, L, 0);
    r = bpvm_mods_sincronizar("/lib/Math.mod", EMB, L, put_fs, linea, sizeof linea);
    CHECK(r == BPVM_MODS_REPUESTO && strstr(linea, "CRC distinto"), "misma versión, CRC distinto → REPUESTO: %s", linea);
    CHECK(mismo_contenido("/lib/Math.mod", EMB, L), "y el FS tiene el embebido");

    bpvm_fs_write("/lib/Math.mod", CORTO, (uint32_t) sizeof CORTO, 0);
    r = bpvm_mods_sincronizar("/lib/Math.mod", EMB, L, put_fs, linea, sizeof linea);
    CHECK(r == BPVM_MODS_REPUESTO && strstr(linea, "tamaño distinto"), "misma versión, tamaño distinto → REPUESTO (sin leer CRC): %s", linea);

    bpvm_fs_write("/lib/Math.mod", NUEVO, L, 0);
    r = bpvm_mods_sincronizar("/lib/Math.mod", EMB, L, put_fs, linea, sizeof linea);
    CHECK(r == BPVM_MODS_SE_DEJA && strstr(linea, "NUEVO"), "MOD8 bajo imagen MOD7 → SE DEJA y se dice: %s", linea);
    CHECK(mismo_contenido("/lib/Math.mod", NUEVO, L), "y el FS conserva el más nuevo");

    bpvm_fs_write("/lib/Math.mod", (const uint8_t*) "xyz", 3, 0);
    r = bpvm_mods_sincronizar("/lib/Math.mod", EMB, L, put_fs, linea, sizeof linea);
    CHECK(r == BPVM_MODS_REPUESTO && strstr(linea, "no es un módulo"), "basura en /lib → REPUESTO: %s", linea);

    r = bpvm_mods_sincronizar("/app/Hello.mod", EMB, L, put_fs, linea, sizeof linea);
    CHECK(r == BPVM_MODS_INSTALADO, "/app/Hello.mod falta → INSTALADO (la muestra)");
    bpvm_fs_write("/app/Hello.mod", V5, L, 0);
    r = bpvm_mods_sincronizar("/app/Hello.mod", EMB, L, put_fs, linea, sizeof linea);
    CHECK(r == BPVM_MODS_SE_DEJA && linea[0] == 0, "/app/Hello.mod del usuario, distinto → SE DEJA, sin ruido");
    CHECK(mismo_contenido("/app/Hello.mod", V5, L), "y no se ha tocado");

    bpvm_fs_write("/lib/Math.mod", V5, L, 0);
    r = bpvm_mods_sincronizar("/lib/Math.mod", EMB, L, put_roto, linea, sizeof linea);
    CHECK(r == BPVM_MODS_FALLO && strstr(linea, "NO se pudo"), "el put falla → FALLO y se dice: %s", linea);

    CHECK(bpvm_mods_version((const uint8_t*) "MOD7") == 7 && bpvm_mods_version((const uint8_t*) "xMOD") == 0,
          "version(): MOD7 → 7, otra cosa → 0");

    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
