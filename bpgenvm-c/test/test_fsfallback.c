/*
 * test_fsfallback.c — la regla que hace usable una fachada de ficheros:
 *
 *     SI `stat` DICE QUE UN FICHERO EXISTE, SE TIENE QUE PODER LEER.
 *     Entero (`read`) y por trozos (`read_at`). Las tres funciones tienen que
 *     mirar en los MISMOS sitios.
 *
 * Se escribe porque esa regla se rompió y costó una tarde (16-ago, P4 con el
 * SQLite.pack grabado): `stat` y `read` consultaban la zona de packs y
 * `read_at` no. Resultado: el resolutor probaba `/app/SQLite.mod`, el `stat`
 * contestaba que sí —8325 B, los del pack, aunque en `/app` no hubiera nada— y
 * la carga del módulo, que va por `read_at` desde #305, moría con `IO error`.
 * El firmware llegaba a avisar de que «el FS eclipsa al del pack» sin que
 * hubiera un solo fichero en el FS.
 *
 * No fue una regresión: `read_at` llegó en #305, el fallback de la zona en
 * V5/H4, y nunca se juntaron. Por eso esto se prueba y no se confía.
 */
#include "bpvm_fs.h"
#include "bpvm_pack.h"
#include <stdio.h>
#include <string.h>

static int fallos = 0;
static void ok(int c, const char* m) {
    printf(c ? "  ok  : %s\n" : "  FAIL: %s\n", m);
    if (!c) fallos++;
}

int main(int argc, char** argv) {
    const char* ruta_pack = (argc > 1) ? argv[1] : "samples/PackFixA.pack";
    bpvm_fs_register_host();

    FILE* f = fopen(ruta_pack, "rb");
    if (!f) { printf("  (skip: no encuentro %s)\n", ruta_pack); return 0; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    static unsigned char zona[256 * 1024];
    memset(zona, 0xFF, sizeof zona);
    size_t leidos = fread(zona, 1, (size_t) n, f);
    fclose(f);
    ok(leidos == (size_t) n, "el pack de prueba se lee entero");
    bpvm_pack_mount(zona, sizeof zona);

    /* Una RUTA DEL FS QUE NO EXISTE, con el nombre de un módulo que sí está en
     * el pack: es exactamente lo que prueba el resolutor al buscar en /app. */
    const char* ruta = "/app/App.mod";
    uint32_t sz = 0;
    int hay = (bpvm_fs_stat(ruta, &sz) == 0);
    ok(hay && sz > 0, "stat lo encuentra en la zona de packs");
    if (!hay) { printf("[status=FAIL]\n"); return 1; }

    unsigned char a[128], b[128];
    long r_entero = bpvm_fs_read(ruta, a, sizeof a);
    ok(r_entero == (long) sz, "read entero: lo lee");

    long r_troz = bpvm_fs_read_at(ruta, 0, b, sizeof b);
    ok(r_troz == (long) sz, "read_at: LO LEE TAMBIEN (la regla que se rompio)");
    ok(r_entero == r_troz && memcmp(a, b, (size_t) (r_entero > 0 ? r_entero : 0)) == 0,
       "y los dos caminos dan los MISMOS bytes");

    /* Por trozos de verdad, que es como carga un módulo. */
    if (sz > 4) {
        unsigned char t[8];
        long mitad = bpvm_fs_read_at(ruta, sz / 2, t, sizeof t);
        ok(mitad > 0, "read_at DESDE UN OFFSET tambien va");
        ok(memcmp(t, a + sz / 2, (size_t) mitad) == 0, "y devuelve el trozo correcto");
    }

    /* Y lo que NO debe pasar: un nombre que no está en ninguna parte. */
    ok(bpvm_fs_stat("/app/NoExisteEnNingunSitio.mod", &sz) != 0,
       "un modulo inexistente sigue sin existir");

    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
