/* test_mods.c — la regla del /lib de la imagen (#466) contra un littlefs REAL en
 * fichero (el mismo oráculo que test_fs_vfs). Cada caso es una situación que las
 * placas han tenido de verdad: el /lib de V5 (MOD6) bajo una imagen MOD7 (la
 * Metro, 2-sep), un módulo recompilado con la misma versión (CRC distinto), uno
 * más nuevo que la imagen (se deja), basura, y el /app del usuario (no se toca).
 *
 * V7/H6 (#468) — Y LA ZONA DE PACKS: si un pack grabado sirve un módulo con los mismos
 * bytes que la imagen, la copia de /lib sobra (lo tapaba: /lib va delante). Los packs
 * los construye el escritor de verdad (PackCli, desde el Makefile), como en
 * test_packres: un pack fabricado aquí a mano probaría un formato que quizá ya no existe.
 * Y la objeción de Eduardo (25-sep) va como caso propio: *«si el usuario borra el pack
 * se queda sin la librería»* — no: la imagen sigue siendo la fuente y lo repone.
 *
 *   make test-mods
 */
#include "bpvm_mods.h"
#include "bpvm_fs.h"
#include "bpvm_pack.h"
#include "bpvm_entry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0;
#define CHECK(cond, ...) do { if (cond) printf("  ok  : " __VA_ARGS__); \
                              else { printf("  FAIL: " __VA_ARGS__); fallos++; } printf("\n"); } while (0)

static int put_fs(const char* p, const uint8_t* d, uint32_t n) { return bpvm_fs_write(p, d, n, 0); }
static int put_roto(const char* p, const uint8_t* d, uint32_t n) { (void) p; (void) d; (void) n; return -1; }
static int  g_lineas = 0;
static void cuenta_linea(const char* l) { (void) l; g_lineas++; }

static int mismo_contenido(const char* path, const uint8_t* data, uint32_t len) {
    uint8_t buf[64]; long n = bpvm_fs_read(path, buf, sizeof buf);
    return n == (long) len && memcmp(buf, data, len) == 0;
}

static uint8_t* leer(const char* path, uint32_t* len) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "no puedo abrir %s\n", path); exit(2); }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t* b = (uint8_t*) malloc((size_t) n);
    if (fread(b, 1, (size_t) n, f) != (size_t) n) { fclose(f); exit(2); }
    fclose(f); *len = (uint32_t) n; return b;
}
static int existe(const char* path) { uint32_t sz = 0; return bpvm_fs_stat(path, &sz) == 0; }
/* De dónde lo cargaría el RUN (el resolvedor de verdad). */
static const char* resuelve(const char* name) {
    static char out[128]; uint32_t sz = 0;
    return bpvm_entry_resolve(name, out, sizeof out, &sz) == 0 ? out : "(no está)";
}
static int g_resync = 0;
static void cuenta_resync(void) { g_resync++; }

int main(int argc, char** argv) {
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

    /* V7/H2 (#523) — mismo MAGIC (MOD8) y GENERACIONES distintas: la gen va be32 en el
     * offset 36. El caso real: el IDE sube a /lib el Core gen 9 que su app necesita; al
     * arrancar, la imagen (gen 8) no debe pisarlo. Y al revés, un /lib gen 7 rancio bajo
     * una imagen gen 8 se repone diciendo por qué. */
    {
        uint8_t emb8[48], lib9[48], lib7[48];
        memset(emb8, 0, sizeof emb8); memcpy(emb8, "MOD8", 4); emb8[39] = 8;
        memcpy(lib9, emb8, sizeof emb8); lib9[39] = 9; lib9[44] = 0x55;   /* otro contenido, otro CRC */
        memcpy(lib7, emb8, sizeof emb8); lib7[39] = 7;
        CHECK(bpvm_mods_gen(emb8, sizeof emb8) == 8 && bpvm_mods_gen(lib9, sizeof lib9) == 9
              && bpvm_mods_gen(EMB, L) == 0 && bpvm_mods_gen(emb8, 20) == 0,
              "gen(): MOD8 → su décimo entero; MOD7 → 0; cabecera corta → 0");
        bpvm_fs_write("/lib/Core.mod", lib9, sizeof lib9, 0);
        r = bpvm_mods_sincronizar("/lib/Core.mod", emb8, sizeof emb8, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_SE_DEJA && strstr(linea, "gen 9 > gen 8"), "MOD8 gen 9 en /lib bajo imagen gen 8 → SE DEJA: %s", linea);
        CHECK(mismo_contenido("/lib/Core.mod", lib9, sizeof lib9), "y el FS conserva el gen 9 (antes lo pisaba: mismo MAGIC, CRC distinto)");
        bpvm_fs_write("/lib/Core.mod", lib7, sizeof lib7, 0);
        r = bpvm_mods_sincronizar("/lib/Core.mod", emb8, sizeof emb8, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_REPUESTO && strstr(linea, "generación anterior") && strstr(linea, "gen 7 -> MOD8 gen 8"),
              "MOD8 gen 7 bajo imagen gen 8 → REPUESTO: %s", linea);
        CHECK(mismo_contenido("/lib/Core.mod", emb8, sizeof emb8), "y el FS tiene el embebido");
        r = bpvm_mods_sincronizar("/lib/Core.mod", emb8, sizeof emb8, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_IGUAL, "misma gen, idéntico → IGUAL");
    }

    /* U4 — el bucle común sobre una tabla como la que genera regen_mods.sh. */
    {
        static const bpvm_mod_embebido_t T[] = {
            { "/lib/Tabla1.mod", EMB, (uint32_t) sizeof EMB },   /* falta → instala */
            { "/lib/Math.mod",   EMB, (uint32_t) sizeof EMB },   /* quedó MOD6 arriba → repone */
        };
        g_lineas = 0;
        unsigned esc = bpvm_mods_instalar_tabla(T, 2, put_fs, cuenta_linea);
        CHECK(esc == 2 && g_lineas == 2, "tabla: 2 escritos (instalado + repuesto) y 2 lineas (%u/%d)", esc, g_lineas);
        g_lineas = 0;
        esc = bpvm_mods_instalar_tabla(T, 2, put_fs, cuenta_linea);
        CHECK(esc == 0 && g_lineas == 0, "tabla otra vez: nada que escribir, nada que decir");
    }

    /* ── V7/H6 (#468) — LA ZONA ─────────────────────────────────────────────────
     * argv: el Json.mod «de la imagen», un pack Stdlib con esos MISMOS bytes y un pack
     * Otro con un Json.mod distinto (los tres, del Makefile). */
    if (argc < 4) {
        printf("  FAIL: faltan el Json.mod y los dos packs (los construye `make test-mods`)\n");
        fallos++;
    } else {
        #define ZONA_BYTES (64u * 1024u)
        uint32_t lz = 0, l1 = 0, l2 = 0;
        uint8_t* emb = leer(argv[1], &lz);
        uint8_t* p1  = leer(argv[2], &l1);
        uint8_t* p2  = leer(argv[3], &l2);
        uint8_t* zona = (uint8_t*) malloc(ZONA_BYTES);
        memset(zona, 0xFF, ZONA_BYTES);          /* flash virgen, como en la placa */
        bpvm_fs_remove("/lib/Json.mod");

        /* El CONTROL: sin zona, la regla de siempre. */
        bpvm_pack_mount(NULL, NULL, 0);
        r = bpvm_mods_sincronizar("/lib/Json.mod", emb, lz, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_INSTALADO, "sin zona: falta → INSTALADO, como siempre");

        /* Un pack que lo sirve con los mismos bytes: la copia de la imagen SOBRA. */
        CHECK(bpvm_pack_add(zona, ZONA_BYTES, p1, l1) >= 0, "se graba el pack 'Stdlib' (Json.mod idéntico al de la imagen)");
        bpvm_pack_mount(zona, zona, ZONA_BYTES);
        CHECK(strcmp(resuelve("Json.mod"), "/lib/Json.mod") == 0, "con la copia en /lib, el RUN carga la de /lib (la que tapa al pack)");
        r = bpvm_mods_sincronizar("/lib/Json.mod", emb, lz, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_QUITADO && strstr(linea, "'Stdlib'") && strstr(linea, "XIP"),
              "la zona lo sirve igual → QUITADO de /lib: %s", linea);
        CHECK(!existe("/lib/Json.mod"), "y /lib ya no lo tiene");
        CHECK(strcmp(resuelve("Json.mod"), "pack:Stdlib/Json.mod") == 0,
              "y el RUN lo carga del pack, en sitio: %s", resuelve("Json.mod"));

        /* El siguiente arranque: ni se instala ni se dice (es el caso normal). */
        r = bpvm_mods_sincronizar("/lib/Json.mod", emb, lz, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_EN_ZONA && linea[0] == 0 && !existe("/lib/Json.mod"),
              "otro arranque → EN_ZONA, en silencio, y /lib sigue sin copia");

        /* Una MÁS NUEVA en /lib se queda (la regla de desarrollo de #310), y se dice. */
        static const uint8_t M8[] = "MOD8--una Json mas nueva que la imagen--";
        bpvm_fs_write("/lib/Json.mod", M8, (uint32_t) sizeof M8, 0);
        r = bpvm_mods_sincronizar("/lib/Json.mod", emb, lz, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_SE_DEJA && strstr(linea, "tapa al del pack"),
              "MOD8 en /lib con el pack sirviendo la de la imagen → SE DEJA, y dice que tapa: %s", linea);
        CHECK(mismo_contenido("/lib/Json.mod", M8, (uint32_t) sizeof M8), "y el FS conserva la más nueva");

        /* Una ANTERIOR: antes se REPONÍA (escribiendo); con el pack, se QUITA. */
        static const uint8_t M6[] = "MOD6--una Json de V5--";
        bpvm_fs_write("/lib/Json.mod", M6, (uint32_t) sizeof M6, 0);
        r = bpvm_mods_sincronizar("/lib/Json.mod", emb, lz, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_QUITADO && !existe("/lib/Json.mod"),
              "MOD6 en /lib y el pack con la de la imagen → QUITADO (no repuesto): %s", linea);

        /* LA OBJECIÓN DE EDUARDO, 1: borrar el MÓDULO con el pack puesto — no se pierde nada. */
        CHECK(strcmp(resuelve("Json.mod"), "pack:Stdlib/Json.mod") == 0,
              "sin Json.mod en /lib, el RUN lo sigue teniendo: el del pack");

        /* LA OBJECIÓN DE EDUARDO, 2: se BORRA EL PACK → la imagen lo repone, en el acto. */
        CHECK(bpvm_pack_remove(zona, ZONA_BYTES, "Stdlib") >= 0, "se borra el pack 'Stdlib'");
        CHECK(strcmp(resuelve("Json.mod"), "(no está)") == 0, "en ese momento, nadie sirve Json.mod");
        g_resync = 0;
        bpvm_mods_resync();                      /* sin registro: no hace nada, ni revienta */
        CHECK(g_resync == 0, "resync sin familia registrada: nada");
        bpvm_mods_set_resync(cuenta_resync);
        bpvm_mods_resync();
        CHECK(g_resync == 1, "resync con la familia registrada: la corre (el wire la llama al borrar un pack)");
        r = bpvm_mods_sincronizar("/lib/Json.mod", emb, lz, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_INSTALADO && mismo_contenido("/lib/Json.mod", emb, lz),
              "y la regla lo REPONE desde la imagen: %s", linea);
        CHECK(strcmp(resuelve("Json.mod"), "/lib/Json.mod") == 0, "el RUN vuelve a tenerlo, en /lib");

        /* Un pack con OTRO Json: la zona no sirve lo de la imagen → la regla de siempre. */
        CHECK(bpvm_pack_add(zona, ZONA_BYTES, p2, l2) >= 0, "se graba el pack 'Otro' (Json.mod distinto)");
        r = bpvm_mods_sincronizar("/lib/Json.mod", emb, lz, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_IGUAL && mismo_contenido("/lib/Json.mod", emb, lz),
              "el pack lleva OTRO Json → la copia de la imagen se queda en /lib, delante");

        /* Dos packs con Json: manda el ÚLTIMO de la cadena, como en la resolución. */
        CHECK(bpvm_pack_add(zona, ZONA_BYTES, p1, l1) >= 0, "se vuelve a grabar 'Stdlib', ahora el último");
        r = bpvm_mods_sincronizar("/lib/Json.mod", emb, lz, put_fs, linea, sizeof linea);
        CHECK(r == BPVM_MODS_QUITADO, "con 'Otro' y luego 'Stdlib', manda el último (el idéntico) → QUITADO");

        /* La tabla cuenta el QUITADO como escritura: la ESP32 guarda la partición por eso. */
        bpvm_fs_write("/lib/Json.mod", emb, lz, 0);
        {
            const bpvm_mod_embebido_t T2[] = { { "/lib/Json.mod", emb, lz } };
            g_lineas = 0;
            unsigned esc = bpvm_mods_instalar_tabla(T2, 1, put_fs, cuenta_linea);
            CHECK(esc == 1 && g_lineas == 1 && !existe("/lib/Json.mod"),
                  "tabla: el QUITADO cuenta como 1 escrito y 1 línea (%u/%d)", esc, g_lineas);
        }
        bpvm_pack_mount(NULL, NULL, 0);
        bpvm_mods_set_resync(NULL);
        free(emb); free(p1); free(p2); free(zona);
    }

    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
