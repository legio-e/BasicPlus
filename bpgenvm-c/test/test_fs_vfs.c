/*
 * test_fs_vfs.c — B1.3 (H2 fase A): dirs reales + op `list` + MONTAJES.
 *
 * (A) LIST sobre el oráculo littlefs: /sys /lib /app como directorios REALES;
 *     list("/") enumera exactamente esos 3 dirs; list("/lib") da ficheros con
 *     sus tamaños; list de un path inexistente → -1.
 * (B) RESOLVE + BASE-DIR (H19) sobre dirs reales del oráculo: con proyecto
 *     "/app/proj" activo, un path RELATIVO resuelve a /app/proj/<f>; los
 *     ABSOLUTOS pasan tal cual.
 * (C) MONTAJES (andamiaje fase B): un backend STUB montado en "/mnt" recibe
 *     las ops bajo /mnt; el resto sigue yendo a littlefs; rename/copy ENTRE
 *     montajes → -1; re-registrar el backend raíz LIMPIA los montajes.
 * (D) LIST sobre el backend HOST (libc/dirent) — mismo contrato.
 *
 * make test-fsvfs. Verde = "VFS B1.3 OK (N asserts)".
 */
#include "bpvm_fs.h"
#include <stdio.h>
#include <string.h>

static int g_asserts = 0;

#define OK(cond, msg) do {                                   \
    if (!(cond)) { printf("FAIL: %s\n", (msg)); return 1; }  \
    g_asserts++;                                             \
} while (0)

/* acumulador del callback de list (fuera del lock se postprocesa) */
typedef struct { char names[16][64]; int isdir[16]; uint32_t size[16]; int n; } listing_t;
static void on_entry(const char* name, int is_dir, uint32_t size, void* user) {
    listing_t* L = (listing_t*) user;
    if (L->n >= 16) return;
    snprintf(L->names[L->n], sizeof L->names[0], "%s", name);
    L->isdir[L->n] = is_dir;
    L->size[L->n]  = size;
    L->n++;
}
static int find_entry(const listing_t* L, const char* name) {
    for (int i = 0; i < L->n; i++)
        if (strcmp(L->names[i], name) == 0) return i;
    return -1;
}

/* ---- backend STUB para el ruteo de montajes (cuenta llamadas) ---- */
static int g_stub_writes = 0, g_stub_stats = 0;
static int stub_stat(const char* path, uint32_t* size) {
    (void) path; g_stub_stats++;
    if (size) *size = 7;
    return 0;                                   /* "todo existe" en el stub */
}
static int stub_write(const char* path, const uint8_t* d, uint32_t len, int a) {
    (void) path; (void) d; (void) len; (void) a; g_stub_writes++;
    return 0;
}
static const bpvm_fs_backend_t s_stub = {
    .stat  = stub_stat,
    .write = stub_write,
    /* resto NULL → fallo limpio (contrato de la fachada) */
};

int main(void) {
    const char* IMG = "build/fs_vfs_test.img";
    remove(IMG);
    listing_t L;
    char buf[64], out[600];

    OK(bpvm_fs_register_lfs_filebd(IMG, 4096, 64, 1) == 0, "attach oráculo");

    /* ---- (A) dirs reales + list sobre littlefs ---- */
    OK(bpvm_fs_mkdir("/sys") == 0 && bpvm_fs_mkdir("/lib") == 0 &&
       bpvm_fs_mkdir("/app") == 0, "jerarquía /sys /lib /app");
    OK(bpvm_fs_write("/lib/Core.mod", (const uint8_t*) "12345", 5, 0) == 0, "lib f1");
    OK(bpvm_fs_write("/lib/IO.mod", (const uint8_t*) "1234567", 7, 0) == 0, "lib f2");

    memset(&L, 0, sizeof L);
    OK(bpvm_fs_list("/", on_entry, &L) == 0, "list /");
    OK(L.n == 3, "raíz: exactamente 3 entradas");
    OK(find_entry(&L, "sys") >= 0 && find_entry(&L, "lib") >= 0 &&
       find_entry(&L, "app") >= 0, "raíz: sys+lib+app");
    OK(L.isdir[find_entry(&L, "lib")] == 1, "lib es dir");

    memset(&L, 0, sizeof L);
    OK(bpvm_fs_list("/lib", on_entry, &L) == 0, "list /lib");
    OK(L.n == 2, "/lib: 2 ficheros");
    int ic = find_entry(&L, "Core.mod");
    OK(ic >= 0 && L.isdir[ic] == 0 && L.size[ic] == 5, "Core.mod: fichero, size 5");
    int ii = find_entry(&L, "IO.mod");
    OK(ii >= 0 && L.size[ii] == 7, "IO.mod: size 7");

    OK(bpvm_fs_list("/no/existe", on_entry, &L) == -1, "list inexistente → -1");

    /* ---- (B) resolve + base-dir sobre dirs reales ---- */
    OK(bpvm_fs_mkdir("/app/proj") == 0, "mkdir /app/proj");
    OK(bpvm_fs_write("/app/proj/res.txt", (const uint8_t*) "recurso", 7, 0) == 0, "recurso");
    bpvm_fs_set_basedir("/app/proj");
    OK(strcmp(bpvm_fs_resolve("res.txt", out, sizeof out), "/app/proj/res.txt") == 0,
       "relativo resuelve bajo el proyecto");
    OK(bpvm_fs_read(out, (uint8_t*) buf, sizeof buf) == 7, "y se lee (7 bytes)");
    OK(strcmp(bpvm_fs_resolve("/lib/Core.mod", out, sizeof out), "/lib/Core.mod") == 0,
       "absoluto pasa tal cual");
    bpvm_fs_set_basedir(NULL);

    /* ---- (C) montajes: stub en /mnt, littlefs en el resto ---- */
    OK(bpvm_fs_mount("/mnt", &s_stub) == 0, "mount /mnt");
    OK(bpvm_fs_write("/mnt/x.txt", (const uint8_t*) "a", 1, 0) == 0 &&
       g_stub_writes == 1, "write /mnt/... va al stub");
    OK(bpvm_fs_exists("/mnt/lo-que-sea") == 1 && g_stub_stats >= 1,
       "stat /mnt/... va al stub");
    OK(bpvm_fs_write("/lib/real.txt", (const uint8_t*) "bb", 2, 0) == 0, "write /lib va a lfs");
    OK(bpvm_fs_read("/lib/real.txt", (uint8_t*) buf, sizeof buf) == 2, "y lfs lo tiene");
    OK(g_stub_writes == 1, "el write de /lib NO tocó el stub");
    OK(bpvm_fs_rename("/lib/real.txt", "/mnt/y.txt") == -1, "rename cross-mount → -1");
    OK(bpvm_fs_copy("/lib/real.txt", "/mnt/y.txt") == -1, "copy cross-mount → -1");
    OK(bpvm_fs_exists("/mntx") == 0, "'/mntx' NO matchea el prefijo /mnt (va a lfs)");
    /* desmontar todo re-registrando la raíz: /mnt vuelve a rutar a lfs */
    bpvm_fs_lfs_filebd_close();
    OK(bpvm_fs_exists("/mnt/lo-que-sea") == 0, "close limpia backend Y montajes");

    /* ---- (D) list sobre el backend HOST ---- */
    bpvm_fs_register_host();
    bpvm_fs_mkdir("t_vfs_host");
    OK(bpvm_fs_write("t_vfs_host/a.txt", (const uint8_t*) "123", 3, 0) == 0, "host f1");
    OK(bpvm_fs_write("t_vfs_host/b.txt", (const uint8_t*) "12345", 5, 0) == 0, "host f2");
    memset(&L, 0, sizeof L);
    OK(bpvm_fs_list("t_vfs_host", on_entry, &L) == 0, "list host");
    OK(L.n == 2, "host: 2 entradas");
    int ia = find_entry(&L, "a.txt");
    OK(ia >= 0 && L.isdir[ia] == 0 && L.size[ia] == 3, "a.txt: fichero, size 3");
    bpvm_fs_remove("t_vfs_host/a.txt");
    bpvm_fs_remove("t_vfs_host/b.txt");
    bpvm_fs_rmdir("t_vfs_host");
    bpvm_fs_set_backend(NULL);

    remove(IMG);
    printf("VFS B1.3 OK (%d asserts)\n", g_asserts);
    return 0;
}
