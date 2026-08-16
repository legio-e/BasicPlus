/*
 * test_packskip.c — V5/H7: cuándo NO hace falta barrer la zona buscando el ancla.
 *
 * El barrido recorre la zona entera de 4 en 4 bytes: en la P4 son 2800 KB,
 * ~700.000 comparaciones y 338 ms MEDIDOS en placa. Como la carga del pack es
 * perezosa (primer `Run`) y sin pack no se marca como hecha, eso se pagaba en
 * cada ejecución.
 *
 * Se evita con una pregunta barata: un `.npk` vive SIEMPRE dentro de un pack, y
 * `bpvm_pack_scan` sabe si hay alguno leyendo la primera cabecera.
 *
 * LO QUE ESTE TEST PROTEGE, y es lo importante: el caso POSITIVO. Un falso «no
 * hay» saltaría el barrido y dejaría un pack grabado SIN CARGAR, en silencio —
 * mucho peor que los 338 ms que se ahorran. Por eso el caso con un pack de
 * verdad no es un extra: es la razón de este fichero.
 */
#include "bpvm_pack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* La MISMA pregunta que hacen pack_p4.c y pack_pico.c antes de barrer. */
static int hay_algun_pack(const uint8_t* base, uint32_t bytes) {
    if (base == 0 || bytes == 0) return 0;
    uint32_t fin = 0;
    return bpvm_pack_scan(base, bytes, 0, 0, 0, &fin) > 0;
}

static int fallos = 0;
static void ok(int c, const char* m) {
    printf(c ? "  ok  : %s\n" : "  FAIL: %s\n", m);
    if (!c) fallos++;
}

int main(int argc, char** argv) {
    static unsigned char zona[64 * 1024];

    memset(zona, 0xFF, sizeof zona);          /* flash borrada */
    ok(!hay_algun_pack(zona, sizeof zona), "zona virgen -> NO se barre");
    ok(!hay_algun_pack(0, 1024),           "base nula   -> NO se barre");
    ok(!hay_algun_pack(zona, 0),           "cero bytes  -> NO se barre");
    memset(zona, 0x5A, 64);                   /* basura, no un pack */
    ok(!hay_algun_pack(zona, sizeof zona), "basura que no es un pack -> NO se barre");

    /* EL CASO QUE IMPORTA: un pack de verdad, en una zona con el resto borrado
     * (que es como queda tras grabar uno). */
    const char* ruta = (argc > 1) ? argv[1] : "samples/PackFixA.pack";
    FILE* f = fopen(ruta, "rb");
    if (!f) {
        printf("  (skip: no encuentro %s — el caso positivo no se ha probado)\n", ruta);
        printf("[status=%s]\n", fallos ? "FAIL" : "OK");
        return fallos;
    }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint32_t zb = (uint32_t) n + 64u * 1024u;
    unsigned char* z = (unsigned char*) malloc(zb);
    memset(z, 0xFF, zb);
    size_t leidos = fread(z, 1, (size_t) n, f);
    fclose(f);
    ok(leidos == (size_t) n, "el pack de prueba se lee entero");
    ok(hay_algun_pack(z, zb), "con un pack GRABADO -> SI se barre (el falso 'no hay' seria mudo)");
    free(z);

    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
