/*
 * test_fs_lfs.c — B1.1 (H2 fase A): la FACHADA bpvm_fs sobre littlefs.
 *
 * Primera conexión del motor a la VM: ejercita las funciones bpvm_fs_* (las
 * mismas que usan los builtins readFile/writeFile/...) contra el backend
 * littlefs-en-imagen (modo oráculo), afirmando la SEMÁNTICA DOCUMENTADA de la
 * fachada (= espejo de fs_host.c):
 *
 *   write sin padres → -1 (como fopen) · mkdir recursivo ok-si-existe ·
 *   stat/-1 en dirs · append acumula · truncate trunca · copy sobreescribe ·
 *   rename REPLACE_EXISTING · remove solo-ficheros · rmdir solo-dirs y
 *   falla-si-no-vacío · exists 1/0
 *
 * y los dos contratos del attach:
 *   PERSISTENCIA: close + re-attach → los datos siguen (la .img es real)
 *   MOUNT-PRIMERO: re-attach con format_if_needed=1 sobre imagen VÁLIDA debe
 *   MONTAR (no reformatear) — formatear solo si el mount falla.
 *
 * make test-fslfs. Verde = "fachada bpvm_fs sobre littlefs OK (N asserts)".
 */
#include "bpvm_fs.h"
#include "crc32.h"   /* #398 — el oraculo del CRC por trozos */
#include <stdio.h>
#include <string.h>

static int g_asserts = 0;

#define OK(cond, msg) do {                                   \
    if (!(cond)) { printf("FAIL: %s\n", (msg)); return 1; }  \
    g_asserts++;                                             \
} while (0)

static int read_str(const char* path, char* out, unsigned cap) {
    long n = bpvm_fs_read(path, (uint8_t*) out, cap - 1);
    if (n < 0) return -1;
    out[n] = '\0';
    return (int) n;
}

int main(void) {
    const char* IMG = "build/fs_lfs_test.img";
    remove(IMG);   /* arranque limpio */
    char buf[128];
    uint32_t sz = 0;

    /* attach con formateo de primera vez (imagen nueva, 128 KB) */
    OK(bpvm_fs_register_lfs_filebd(IMG, 4096, 32, 1) == 0, "attach+format inicial");

    /* -- write sin padres → -1 (como fopen); tras mkdir -p → OK -- */
    OK(bpvm_fs_write("/lib/a/b/f.txt", (const uint8_t*) "hola", 4, 0) == -1,
       "write sin padres debe fallar");
    OK(bpvm_fs_mkdir("/lib/a/b") == 0, "mkdir -p /lib/a/b");
    OK(bpvm_fs_isdir("/lib") == 1 && bpvm_fs_isdir("/lib/a") == 1 &&
       bpvm_fs_isdir("/lib/a/b") == 1, "intermedios creados");
    OK(bpvm_fs_mkdir("/lib/a/b") == 0, "mkdir ok-si-existe");
    OK(bpvm_fs_write("/lib/a/b/f.txt", (const uint8_t*) "hola", 4, 0) == 0,
       "write tras mkdir");

    /* -- stat / exists / read -- */
    OK(bpvm_fs_stat("/lib/a/b/f.txt", &sz) == 0 && sz == 4, "stat size=4");
    OK(bpvm_fs_stat("/lib/a/b", &sz) == -1, "stat en dir → -1 (como host)");
    OK(bpvm_fs_exists("/lib/a/b/f.txt") == 1, "exists=1");
    OK(bpvm_fs_exists("/lib/a/b/no.txt") == 0, "exists=0");
    OK(read_str("/lib/a/b/f.txt", buf, sizeof buf) == 4 && strcmp(buf, "hola") == 0,
       "read == hola");

    /* -- append acumula; truncate trunca -- */
    OK(bpvm_fs_write("/lib/a/b/f.txt", (const uint8_t*) " mundo", 6, 1) == 0, "append");
    OK(read_str("/lib/a/b/f.txt", buf, sizeof buf) == 10 &&
       strcmp(buf, "hola mundo") == 0, "append acumulado");
    OK(bpvm_fs_write("/lib/a/b/f.txt", (const uint8_t*) "x", 1, 0) == 0, "truncate");
    OK(bpvm_fs_stat("/lib/a/b/f.txt", &sz) == 0 && sz == 1, "truncado a 1");

    /* -- copy sobreescribe; rename REPLACE_EXISTING -- */
    OK(bpvm_fs_write("/lib/orig.txt", (const uint8_t*) "contenido", 9, 0) == 0, "orig");
    OK(bpvm_fs_copy("/lib/orig.txt", "/lib/a/b/f.txt") == 0, "copy sobre existente");
    OK(read_str("/lib/a/b/f.txt", buf, sizeof buf) == 9 &&
       strcmp(buf, "contenido") == 0, "copy byte-igual");
    OK(bpvm_fs_rename("/lib/orig.txt", "/lib/nuevo.txt") == 0, "rename a hueco");
    OK(bpvm_fs_exists("/lib/orig.txt") == 0 && bpvm_fs_exists("/lib/nuevo.txt") == 1,
       "rename movió");
    OK(bpvm_fs_write("/lib/otro.txt", (const uint8_t*) "yo", 2, 0) == 0, "otro");
    OK(bpvm_fs_rename("/lib/otro.txt", "/lib/nuevo.txt") == 0,
       "rename sobre existente (REPLACE)");
    OK(read_str("/lib/nuevo.txt", buf, sizeof buf) == 2 && strcmp(buf, "yo") == 0,
       "REPLACE dejó el contenido nuevo");

    /* -- remove solo-ficheros; rmdir solo-dirs y no-vacío -- */
    OK(bpvm_fs_remove("/lib/a/b") == -1, "remove en dir → -1");
    OK(bpvm_fs_rmdir("/lib/nuevo.txt") == -1, "rmdir en fichero → -1");
    OK(bpvm_fs_rmdir("/lib/a/b") == -1, "rmdir no vacío → -1");
    OK(bpvm_fs_remove("/lib/a/b/f.txt") == 0, "remove fichero");
    OK(bpvm_fs_rmdir("/lib/a/b") == 0, "rmdir ya vacío");
    OK(bpvm_fs_isdir("/lib/a/b") == 0, "dir borrado");

    /* -- #398: EL CRC DEL BACKEND == EL BUCLE DE read_at, SOBRE LITTLEFS --
     *
     * Es el motor del FS interno de las tres placas, o sea el que de verdad va
     * a correr el camino nuevo. Se comprueba aquí y no sólo en `test_crc`
     * (que va sobre el backend de host) porque lo que se sustituye es la
     * implementación DE CADA BACKEND: que el host coincida no dice nada de
     * littlefs.
     *
     * El camino nuevo abre el fichero una vez; el viejo hacía un
     * `opencfg`+`seek`+`read`+`close` por cada 256 B — 512 aperturas para
     * 128 KB, 1589 ms medidos en la P4. Lo que NO puede cambiar es el número:
     * el IDE lo compara con `java.util.zip.CRC32` para saltarse subidas. */
    {
        const uint32_t TAMS[] = { 0, 1, 255, 256, 257, 512, 1000, 5001 };
        for (unsigned t = 0; t < sizeof(TAMS)/sizeof(TAMS[0]); t++) {
            uint32_t n = TAMS[t];
            static uint8_t datos[5001];
            for (uint32_t i = 0; i < n; i++) datos[i] = (uint8_t) (i * 31 + 7);
            OK(bpvm_fs_write("/crc.bin", datos, n, 0) == 0, "crc: escribir el caso");

            uint32_t via_backend = 0xDEADu;
            OK(bpvm_fs_crc32("/crc.bin", &via_backend) == 0, "crc: el backend contesta");

            uint32_t st = BPVM_CRC32_INIT, off = 0;
            uint8_t  tr[256];
            int malo = 0;
            while (off < n) {
                long got = bpvm_fs_read_at("/crc.bin", off, tr, sizeof tr);
                if (got <= 0) { malo = 1; break; }
                st = bpvm_crc32_update(st, tr, (size_t) got);
                off += (uint32_t) got;
            }
            OK(!malo && bpvm_crc32_final(st) == via_backend,
               "crc: backend == bucle de read_at (littlefs)");
        }
        OK(bpvm_fs_remove("/crc.bin") == 0, "crc: limpiar");
    }

    /* -- PERSISTENCIA entre attach + mount-primero-no-reformatear -- */
    OK(bpvm_fs_write("/marca.txt", (const uint8_t*) "sobrevivo", 9, 0) == 0, "marca");
    bpvm_fs_lfs_filebd_close();
    OK(bpvm_fs_exists("/marca.txt") == 0, "sin backend → fallo limpio");
    /* re-attach con format_if_needed=1: imagen VÁLIDA → debe MONTAR, no formatear */
    OK(bpvm_fs_register_lfs_filebd(IMG, 4096, 32, 1) == 0, "re-attach");
    OK(read_str("/marca.txt", buf, sizeof buf) == 9 && strcmp(buf, "sobrevivo") == 0,
       "persistió y NO se reformateó");
    bpvm_fs_lfs_filebd_close();

    remove(IMG);
    printf("fachada bpvm_fs sobre littlefs OK (%d asserts)\n", g_asserts);
    return 0;
}
