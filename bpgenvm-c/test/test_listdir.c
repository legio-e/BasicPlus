/*
 * test_listdir.c — V5/H6 paso 3: el núcleo común de LIST_DIR.
 *
 * QUÉ SE PRUEBA Y POR QUÉ. Este verbo vivía dentro del despachador del Pico,
 * donde sólo se podía ejercer con placa. Al sacarlo a `src/` se puede correr
 * sobre el backend de HOST — ficheros de verdad — y comprobar lo que de verdad
 * importa: que el JSON que sale es el que el IDE espera, que los nombres van
 * escapados, y que un truncado NUNCA sale en silencio.
 *
 * El orden de las entradas lo decide el backend (readdir), así que se comprueba
 * por PRESENCIA, no por posición: fijar el orden sería probar el sistema de
 * ficheros del PC, no nuestro código.
 *
 *   make test-listdir
 */
#include "bpvm_listdir.h"
#include "bpvm_fs.h"

#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* El sumidero de la prueba: acumula. En el firmware uno escribe a stdout y el
 * otro trocea por el wire — aquí lo que interesa es el TEXTO resultante. */
static char g_buf[8192];
static size_t g_len;

static void sink(const char* txt, size_t n, void* user) {
    (void) user;
    if (g_len + n + 1 >= sizeof g_buf) return;
    memcpy(g_buf + g_len, txt, n);
    g_len += n;
    g_buf[g_len] = '\0';
}

static void reset(void) { g_len = 0; g_buf[0] = '\0'; }

static void escribe(const char* path, const char* txt) {
    bpvm_fs_write(path, (const uint8_t*) txt, (uint32_t) strlen(txt), 0);
}

int main(void)
{
    printf("== V5/H6: LIST_DIR (nucleo comun, backend host) ==\n\n");
    bpvm_fs_register_host();

    const char* DIR = "test_listdir_dir";
    bpvm_fs_mkdir(DIR);
    escribe("test_listdir_dir/uno.txt",  "hola");
    escribe("test_listdir_dir/dos.mod",  "0123456789");
    bpvm_fs_mkdir("test_listdir_dir/sub");

    /* ── El listado de un directorio con contenido ───────────────────────── */
    {
        reset();
        int om = -1;
        bpvm_listdir_res_t r = bpvm_listdir_emitir(DIR, 42, sink, NULL, &om);
        CHECK(r == BPVM_LISTDIR_OK,                    "un directorio con contenido se lista");
        CHECK(strstr(g_buf, "\"type\":\"LIST_DIR_REPLY\"") != NULL,
                                                       "el tipo del mensaje es el que espera el IDE");
        CHECK(strstr(g_buf, "\"id\":42") != NULL,      "el id vuelve tal cual");
        CHECK(strstr(g_buf, "\"name\":\"uno.txt\"") != NULL,  "sale 'uno.txt'");
        CHECK(strstr(g_buf, "\"size\":4") != NULL,     "y con su tamaño (4 bytes)");
        CHECK(strstr(g_buf, "\"name\":\"dos.mod\"") != NULL,  "sale 'dos.mod'");
        CHECK(strstr(g_buf, "\"size\":10") != NULL,    "y su tamaño (10 bytes)");
        CHECK(strstr(g_buf, "\"isDir\":true") != NULL, "el subdirectorio se marca isDir");
        CHECK(om == 0,                                 "no se omitio ninguna entrada");
        CHECK(strstr(g_buf, "\"omitidas\":0") != NULL, "y el JSON lo dice tambien");
        /* NO cierra la linea: eso lo pone el transporte, que es lo unico que
         * cada familia hace distinto. */
        CHECK(strchr(g_buf, '\n') == NULL,             "el NUCLEO no cierra la linea");
        /* Y NO calcula CRC: es LIST_DIR, no LIST. Con una SD montada, el CRC
         * seria leerse la tarjeta entera. */
        CHECK(strstr(g_buf, "\"crc\"") == NULL,        "sin CRC: es LIST_DIR, no LIST");
    }

    /* ── Un camino que no se puede listar: NADA, ni lista vacia ──────────── */
    {
        reset();
        bpvm_listdir_res_t r = bpvm_listdir_emitir("no_existe_este_dir", 7, sink, NULL, NULL);
        CHECK(r == BPVM_LISTDIR_NO_LISTA,  "un camino que no existe -> NO_LISTA");
        CHECK(g_len == 0,
              "y NO emite nada: una lista vacia se leeria como 'directorio vacio'");
    }

    /* ── El escapado, que es donde se rompe un JSON en silencio ──────────── */
    {
        const char* RARO = "test_listdir_dir/con\"comilla.txt";
        if (bpvm_fs_write(RARO, (const uint8_t*) "x", 1, 0) == 0) {
            reset();
            bpvm_listdir_emitir(DIR, 1, sink, NULL, NULL);
            CHECK(strstr(g_buf, "con\\\"comilla.txt") != NULL,
                                                       "las comillas del nombre van escapadas");
            bpvm_fs_remove(RARO);
        } else {
            /* Windows no deja crear ese nombre. No se finge un verde: se dice. */
            printf("  --  : (saltado) el SO no deja crear un nombre con comillas\n");
        }
    }

    /* ── Parametros que no deben reventar ────────────────────────────────── */
    {
        reset();
        CHECK(bpvm_listdir_emitir(DIR, 1, NULL, NULL, NULL) == BPVM_LISTDIR_NO_LISTA,
                                                       "sin sumidero -> rechazo, sin reventar");
        reset();
        bpvm_listdir_res_t r = bpvm_listdir_emitir(NULL, 1, sink, NULL, NULL);
        CHECK(r == BPVM_LISTDIR_OK || r == BPVM_LISTDIR_NO_LISTA,
                                                       "path NULL = la raiz, y no revienta");
    }

    /* Limpieza */
    bpvm_fs_remove("test_listdir_dir/uno.txt");
    bpvm_fs_remove("test_listdir_dir/dos.mod");
    bpvm_fs_rmdir("test_listdir_dir/sub");
    bpvm_fs_rmdir(DIR);

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
