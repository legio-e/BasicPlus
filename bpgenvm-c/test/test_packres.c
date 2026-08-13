/*
 * test_packres.c — #362: la ZONA de packs también sirve RECURSOS, no sólo
 * módulos.
 *
 * <h3>Qué protege</h3>
 *
 * Una fuente o un icono grabados en un pack no se podían cargar: los recursos
 * se resuelven por la fachada del FS, y la zona sólo se consultaba para
 * MÓDULOS. Al arreglarlo hay tres reglas que son fáciles de romper sin
 * enterarse, porque el síntoma de todas es el mismo (sale UNA imagen, y parece
 * bien):
 *
 *     pack en ejecución  →  FS  →  zona de packs grabados
 *
 *   1. la zona sirve el recurso cuando no está en el FS;
 *   2. el FS ECLIPSA a la zona (spec §4) — un fichero puesto a mano en el
 *      dispositivo tiene que poder tapar al de un pack;
 *   3. `pack:<Pack>/<fichero>` va a ESE pack y se salta todo lo demás.
 *
 * <h3>Por qué DOS packs con el mismo fichero</h3>
 *
 * Porque es el único montaje en el que se distingue "cualificar funciona" de
 * "cualificar se ignora y ha salido el de siempre": los dos packs traen
 * `logo.png` con contenidos distintos, y sin cualificar gana el ÚLTIMO grabado.
 * Si al pedir el PRIMERO por su nombre saliera el del último, el prefijo no
 * estaría haciendo nada — y con un solo pack eso pasaría desapercibido.
 *
 * <h3>Los packs se construyen con la herramienta de verdad</h3>
 *
 * El Makefile los genera con PackCli antes de compilar esto. Fabricar aquí una
 * imagen de pack a mano sería reimplementar el formato (cabecera, CRCs) en un
 * segundo sitio: el día que el escritor cambiara, este test seguiría verde
 * probando un formato que ya no existe.
 */
#include "bpvm_fs.h"
#include "bpvm_pack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0;
static void ok(int c, const char* m) { printf(c ? "  ok  : %s\n" : "  FAIL: %s\n", m); if (!c) fallos++; }

#define ZONA_BYTES (256u * 1024u)

static uint8_t* leer(const char* path, uint32_t* len) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "no puedo abrir %s\n", path); exit(2); }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t* b = (uint8_t*) malloc((size_t) n);
    if (fread(b, 1, (size_t) n, f) != (size_t) n) { fclose(f); exit(2); }
    fclose(f); *len = (uint32_t) n; return b;
}

/* Lee un recurso por la FACHADA (que es por donde van loadFont y las imágenes)
 * y devuelve 1 si existe, dejando el contenido en dst. */
static int leer_recurso(const char* path, char* dst, size_t cap) {
    uint32_t sz = 0;
    if (bpvm_fs_stat(path, &sz) != 0) return 0;
    long n = bpvm_fs_read(path, (uint8_t*) dst, (uint32_t) cap - 1);
    if (n < 0) return 0;
    dst[n] = '\0';
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "uso: %s <iconos.pack> <otros.pack>\n", argv[0]); return 2; }
    bpvm_fs_register_host();

    /* Los recursos del test son texto a propósito: lo que se prueba es de
     * DÓNDE sale el fichero, y con texto la comparación dice cuál salió. */
    char buf[128];

    /* ── 1) sin zona montada: nadie sirve nada ─────────────────────────────
     * El CONTROL. Sin él, un respaldo que reclamara SIEMPRE pasaría los demás
     * casos y este test no valdría para nada. */
    bpvm_pack_mount(NULL, 0);
    remove("logo.png");
    ok(!leer_recurso("logo.png", buf, sizeof buf), "sin zona y sin fichero: no está");

    /* ── 2) con la zona: el recurso sale del pack ───────────────────────── */
    uint8_t* zona = (uint8_t*) calloc(1, ZONA_BYTES);
    memset(zona, 0xFF, ZONA_BYTES);              /* flash virgen, como en la placa */
    uint32_t l1 = 0, l2 = 0;
    uint8_t* p1 = leer(argv[1], &l1);            /* Iconos */
    uint8_t* p2 = leer(argv[2], &l2);            /* Otros, grabado DESPUÉS */
    ok(bpvm_pack_add(zona, ZONA_BYTES, p1, l1) >= 0, "se graba el pack 'Iconos'");
    bpvm_pack_mount(zona, ZONA_BYTES);
    ok(leer_recurso("logo.png", buf, sizeof buf) && strcmp(buf, "ICONOS") == 0,
       "la ZONA sirve el recurso (era el agujero de #362)");

    /* ── 3) el FS eclipsa a la zona ─────────────────────────────────────── */
    bpvm_fs_write("logo.png", (const uint8_t*) "DELFS", 5, 0);
    ok(leer_recurso("logo.png", buf, sizeof buf) && strcmp(buf, "DELFS") == 0,
       "el FS ECLIPSA a la zona (spec §4): gana el fichero del dispositivo");
    remove("logo.png");
    ok(leer_recurso("logo.png", buf, sizeof buf) && strcmp(buf, "ICONOS") == 0,
       "quitado el del FS, vuelve a servir la zona");

    /* ── 4) dos packs con el mismo fichero: gana el ÚLTIMO grabado ──────── */
    ok(bpvm_pack_add(zona, ZONA_BYTES, p2, l2) >= 0, "se graba encima el pack 'Otros'");
    ok(leer_recurso("logo.png", buf, sizeof buf) && strcmp(buf, "OTROS") == 0,
       "sin cualificar gana el ULTIMO de la cadena");

    /* ── 5) cualificado: ese pack, y ningún otro ────────────────────────── */
    ok(leer_recurso("pack:Iconos/logo.png", buf, sizeof buf) && strcmp(buf, "ICONOS") == 0,
       "'pack:Iconos/logo.png' saca el del PRIMERO, no el que ganaba");
    ok(leer_recurso("pack:Otros/logo.png", buf, sizeof buf) && strcmp(buf, "OTROS") == 0,
       "'pack:Otros/logo.png' saca el suyo");

    /* Y cualificar tiene que saltarse tambien el FS: si no, poner un fichero
     * en el dispositivo secuestraría una peticion que decía de donde queria el
     * recurso. */
    bpvm_fs_write("logo.png", (const uint8_t*) "DELFS", 5, 0);
    ok(leer_recurso("pack:Iconos/logo.png", buf, sizeof buf) && strcmp(buf, "ICONOS") == 0,
       "cualificado NO lo tapa un fichero del FS con ese nombre");
    remove("logo.png");

    /* ── 6) pedir un pack que no está: no está (y el que lo pide, se entera) */
    ok(!leer_recurso("pack:NoExiste/logo.png", buf, sizeof buf),
       "un pack que no esta grabado no sirve nada");
    ok(!leer_recurso("pack:Iconos/nohay.png", buf, sizeof buf),
       "un pack que esta pero no lleva el fichero, tampoco");

    free(p1); free(p2); free(zona);
    printf(fallos ? "[status=FAIL] %d fallo(s)\n" : "[status=OK]\n", fallos);
    return fallos ? 1 : 0;
}
