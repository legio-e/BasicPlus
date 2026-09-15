/*
 * test_fsfallback.c — la fachada de ficheros y la ZONA DE PACKS: dos reglas.
 *
 *  1. SI `stat` DICE QUE UN FICHERO EXISTE, SE TIENE QUE PODER LEER: entero
 *     (`read`), por trozos (`read_at`), en secuencia (`read_stream`) y su CRC
 *     (`crc32`). Las cinco funciones miran en los MISMOS sitios. Se rompió dos
 *     veces: el 16-ago (`read_at` no miraba la zona: «IO error» con el
 *     SQLite.pack en la P4) y hasta el 15-sep (`read_stream`/`crc32` iban
 *     directas al backend: el GET de un fichero de la zona mandaba la cabecera
 *     bulk y luego nada, y `STAT{crc}` daba -1, medido en el sim).
 *
 *  2. [V7/H2, #493] LA ZONA CONTESTA SÓLO EN SU PROPIO ESPACIO. Este test decía
 *     lo contrario —«stat lo encuentra en la zona» para `/app/App.mod`— y
 *     consagraba la sombra: la zona reclamaba cualquier ruta por su basename,
 *     `stat("/app/Str.mod")` decía que sí sin que en /app hubiera nada, un pack
 *     con una stdlib vieja tapaba a /lib disfrazado de /app, y la resolución
 *     de módulos copiaba a RAM lo que debía ejecutar XIP (muerto desde el
 *     13-ago). Ahora `/app/App.mod` NO existe; `pack:PackFixA/App.mod` sí; y
 *     «¿alguien tiene App.mod?» lo contesta `locate` (bpvm_fs_resolve, último
 *     candidato) con esa ruta cualificada.
 */
#include "bpvm_fs.h"
#include "bpvm_pack.h"
#include "crc32.h"
#include <stdio.h>
#include <string.h>

static int fallos = 0;
static void ok(int c, const char* m) {
    printf(c ? "  ok  : %s\n" : "  FAIL: %s\n", m);
    if (!c) fallos++;
}

/* read_stream: acumula en un buffer para comparar con read. */
static unsigned char g_acum[4096]; static uint32_t g_acum_n;
static int acumula(const uint8_t* p, uint32_t n, void* user) {
    (void) user;
    if (g_acum_n + n > sizeof g_acum) return 1;
    memcpy(g_acum + g_acum_n, p, n); g_acum_n += n;
    return 0;
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
    bpvm_pack_mount(zona, zona, sizeof zona);

    uint32_t sz = 0;

    /* ── 2. Sólo en su espacio ──────────────────────────────────────────── */
    ok(bpvm_fs_stat("/app/App.mod", &sz) != 0,  "/app/App.mod NO existe: la zona ya no contesta por basename (#493)");
    ok(bpvm_fs_stat("/lib/App.mod", &sz) != 0,  "/lib/App.mod tampoco");
    ok(bpvm_fs_stat("App.mod", &sz) != 0,       "ni el nombre pelado");
    ok(bpvm_fs_stat("/packs/PackFixA/App.mod", &sz) != 0, "ni /packs/<P>/x (no es una forma: solo pack:)");
    ok(bpvm_fs_stat("pack:Otro/App.mod", &sz) != 0, "pack:<OtroPack>/App.mod no: la forma cualificada es de ESE pack");

    const char* ruta = "pack:PackFixA/App.mod";
    int hay = (bpvm_fs_stat(ruta, &sz) == 0);
    ok(hay && sz > 0, "pack:PackFixA/App.mod SI existe (stat)");
    if (!hay) { printf("[status=FAIL]\n"); return 1; }

    /* «¿Alguien tiene App.mod?» → bpvm_fs_resolve lo pregunta el ULTIMO, a la
     * zona, y devuelve la ruta cualificada. */
    char res[256];
    const char* r = bpvm_fs_resolve("App.mod", res, sizeof res);
    ok(strcmp(r, ruta) == 0, "bpvm_fs_resolve(\"App.mod\") -> pack:PackFixA/App.mod (locate, ultimo candidato)");
    r = bpvm_fs_resolve("NoExisteEnNingunSitio.png", res, sizeof res);
    ok(strncmp(r, "pack:", 5) != 0, "un recurso que ningun pack tiene no se inventa una ruta pack:");

    /* ── 1. Si existe, se lee por los cinco caminos ────────────────────── */
    unsigned char a[128], b[128];
    long r_entero = bpvm_fs_read(ruta, a, sizeof a);
    ok(r_entero == (long) sz, "read entero: lo lee");

    long r_troz = bpvm_fs_read_at(ruta, 0, b, sizeof b);
    ok(r_troz == (long) sz, "read_at: lo lee tambien");
    ok(r_entero == r_troz && memcmp(a, b, (size_t) (r_entero > 0 ? r_entero : 0)) == 0,
       "y los dos caminos dan los MISMOS bytes");

    if (sz > 4) {
        unsigned char t[8];
        long mitad = bpvm_fs_read_at(ruta, sz / 2, t, sizeof t);
        ok(mitad > 0, "read_at DESDE UN OFFSET tambien va");
        ok(memcmp(t, a + sz / 2, (size_t) mitad) == 0, "y devuelve el trozo correcto");
    }

    g_acum_n = 0;
    long r_stream = bpvm_fs_read_stream(ruta, acumula, NULL);
    ok(r_stream == (long) sz && g_acum_n == sz && memcmp(g_acum, a, sz) == 0,
       "read_stream: lo lee (el GET del wire; antes mandaba la cabecera y nada)");

    uint32_t crc = 0;
    ok(bpvm_fs_crc32(ruta, &crc) == 0 && crc == bpvm_crc32(a, sz),
       "crc32: lo calcula (STAT{crc} daba -1 en littlefs)");

    /* Y lo que NO debe pasar: un nombre que no está en ninguna parte. */
    ok(bpvm_fs_stat("pack:PackFixA/NoExisteEnNingunSitio.mod", &sz) != 0,
       "un modulo inexistente sigue sin existir");

    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
