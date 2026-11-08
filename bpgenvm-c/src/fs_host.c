/*
 * fs_host.c — backend de file I/O sobre libc, para la VM C en HOST.
 *
 * HOST-ONLY: no se linka en el firmware (el device registra su propio backend
 * sobre fs_get/fs_put). Lo registra test/main.c con bpvm_fs_register_host().
 * Sin sandbox: el path se usa tal cual contra el FS del host (paridad con la
 * VM-Java cuando no hay workdir configurado).
 *
 * V8 #808 (8-nov) — LA FLASH DE LA VM DEL PC, una carpeta (Eduardo: «las VM del PC deberían tener una carpeta que simule
 * la flash, así que lo se guarde ahí no borre»). Una ruta de la placa, absoluta ("/sys/bluetooth.json"), va dentro de
 * ella; antes iba a la raíz del disco. Las relativas y las del PC ("C:/…"), tal cual. Sin carpeta (las pruebas que no la
 * fijan), como antes. La fija test/main.c (--flash-dir, o %USERPROFILE%/.basicplus/flash), y la crea si no está, como
 * una placa con su flash formateada; la VM-Java hace lo mismo. Una ruta que sube más allá de la raíz ("/../x") no sale
 * de ella: falla, como en littlefs.
 */
#include "bpvm_fs.h"
#include "crc32.h"      /* #398 — CRC del fichero entero con UNA apertura */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>      /* V5/H2: malloc del truncate por copia */
#include <errno.h>       /* #628 — el motivo de un fallo del PC */

/* #628 — el MOTIVO de un fallo sale de errno. Los textos, nuestros y literales:
 * la fachada los guarda por puntero, y strerror puede devolver un búfer que la
 * llamada siguiente pisa. -1, para el `return` del que falla. */
static int host_fallo_de(int e) {
    bpvm_fs_fail_t c = BPVM_FS_FAIL_OTHER;
    const char*    t = "errno";
    switch (e) {
        case ENOENT: c = BPVM_FS_FAIL_NOT_FOUND; t = "ENOENT (no existe)"; break;
        case EEXIST: c = BPVM_FS_FAIL_EXISTS;    t = "EEXIST (ya existe)"; break;
        case ENOSPC: c = BPVM_FS_FAIL_NO_SPACE;  t = "ENOSPC (sin espacio)"; break;
        case EIO:    c = BPVM_FS_FAIL_IO;        t = "EIO (fallo de E/S)"; break;
#ifdef EFBIG
        case EFBIG:  c = BPVM_FS_FAIL_TOO_BIG;   t = "EFBIG (fichero demasiado grande)"; break;
#endif
#ifdef ENAMETOOLONG
        case ENAMETOOLONG: c = BPVM_FS_FAIL_NAME_TOO_LONG; t = "ENAMETOOLONG (nombre demasiado largo)"; break;
#endif
        case EACCES: t = "EACCES (sin permiso)"; break;
        case EINVAL: t = "EINVAL (argumento invalido)"; break;
        case ENOMEM: t = "ENOMEM (sin memoria)"; break;
#ifdef EISDIR
        case EISDIR: t = "EISDIR (es una carpeta)"; break;
#endif
#ifdef ENOTDIR
        case ENOTDIR: t = "ENOTDIR (no es una carpeta)"; break;
#endif
#ifdef ENOTEMPTY
        case ENOTEMPTY: t = "ENOTEMPTY (carpeta no vacia)"; break;
#endif
        default: break;
    }
    bpvm_fs_anota_fallo("host", e, c, t);
    return -1;
}
static int host_fallo(void) { return host_fallo_de(errno); }

static char s_flash[512] = "";   /* #808: "" = sin flash */
static void host_crea_carpeta(const char* dir);   /* más abajo, con el mkdir de cada sistema */

void bpvm_fs_host_set_flash(const char* dir) {
    size_t n = dir ? strlen(dir) : 0;
    while (n > 0 && (dir[n - 1] == '/' || dir[n - 1] == '\\')) n--;
    if (n >= sizeof s_flash) n = sizeof s_flash - 1;
    if (n) memcpy(s_flash, dir, n);
    s_flash[n] = '\0';
    if (s_flash[0]) host_crea_carpeta(s_flash);   /* la flash existe siempre, como en una placa */
}
const char* bpvm_fs_host_flash(void) { return s_flash; }

/* #808 — la ruta del PC: una de la placa, dentro de la flash; las demás, tal cual. NULL si se sale de la flash: un ".."
 * que sube más allá de la raíz, que en la placa es un error de littlefs (LFS_ERR_INVAL, «unmatched '..'»). Se cuenta
 * con '/' y con '\\', que en Windows también separa: "/..\x" se saldría igual. */
static const char* en_flash(const char* path, char* buf, size_t cap) {
    if (!s_flash[0] || !path || path[0] != '/') return path;
    int hondo = 0;
    for (const char* s = path; *s; ) {
        while (*s == '/' || *s == '\\') s++;
        size_t n = strcspn(s, "/\\");
        if (n == 2 && s[0] == '.' && s[1] == '.') { if (--hondo < 0) return NULL; }
        else if (n > 0 && !(n == 1 && s[0] == '.')) hondo++;
        s += n;
    }
    snprintf(buf, cap, "%s%s", s_flash, path);
    return buf;
}
/* Una ruta que se sale de la flash: el fallo, con el motivo de la placa. */
static int host_fuera(void) {
    bpvm_fs_anota_fallo("host", EINVAL, BPVM_FS_FAIL_OTHER, "INVAL (la ruta se sale de la flash)");
    return -1;
}
/* la flash (511) y una ruta larga caben; si se sale, la función devuelve `fuera` */
#define EN_FLASH_O(p, fuera) char p##_pc[1300]; p = en_flash(p, p##_pc, sizeof p##_pc); if (!p) return (fuera)
#define EN_FLASH(p) EN_FLASH_O(p, host_fuera())
/* Una escritura CORTA: errno, si la libc lo puso (se pone a 0 antes); si no, lleno. */
static int host_corto(void) { return host_fallo_de(errno ? errno : ENOSPC); }

static int host_stat(const char* path, uint32_t* size) {
    EN_FLASH(path);
    FILE* f = fopen(path, "rb");
    if (!f) return host_fallo();
    if (fseek(f, 0, SEEK_END) != 0) { int e = errno; fclose(f); return host_fallo_de(e); }
    long sz = ftell(f);
    int e = errno;
    fclose(f);
    if (sz < 0) return host_fallo_de(e);
    if (size) *size = (uint32_t) sz;
    return 0;
}

static long host_read(const char* path, uint8_t* dst, uint32_t cap) {
    EN_FLASH(path);
    FILE* f = fopen(path, "rb");
    if (!f) return host_fallo();
    size_t n = fread(dst, 1, (size_t) cap, f);
    fclose(f);
    return (long) n;
}

/* #398 — el CRC del fichero entero con UNA apertura. En el host el camino viejo
 * (un `fopen`+`fseek`+`fread`+`fclose` por cada 256 B) no se notaba —el SO
 * cachea— y por eso el problema sólo apareció en placa: 5432 aperturas para
 * 1,3 MB de tarjeta. Se implementa aquí IGUAL que en los otros dos backends
 * para que el host siga siendo un oráculo válido: si el host tomara un camino
 * y la placa otro, un CRC distinto entre los dos no lo cazaría nadie. */
static int host_crc32(const char* path, uint32_t* crc_out) {
    EN_FLASH(path);
    FILE* f = fopen(path, "rb");
    if (!f) return host_fallo();
    uint8_t  buf[512];
    uint32_t st = BPVM_CRC32_INIT;
    size_t   n;
    errno = 0;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0)
        st = bpvm_crc32_update(st, buf, n);
    int err = ferror(f), e = errno;
    fclose(f);
    if (err) return host_fallo_de(e ? e : EIO);
    if (crc_out) *crc_out = bpvm_crc32_final(st);
    return 0;
}

/* #305 — lectura desde un offset (el simulador la usa igual que el micro). */
/* V5/H2 — las gemelas de escritura. Van TAMBIÉN en el host y no sólo en placa:
 * si el host no las tuviera, un programa que escriba posicionalmente iría en la
 * placa y fallaría en el PC, y la paridad dual-VM dejaría de valer justo para
 * lo que más falta hace probar en el PC — una base de datos.
 *
 * "r+b" es abrir sin truncar; si no existe, "w+b" lo crea. Ese orden importa:
 * al revés se vaciaría el fichero cada vez que se escribe una página. */
static long host_write_at(const char* path, uint32_t off,
                          const uint8_t* data, uint32_t len) {
    EN_FLASH(path);
    FILE* f = fopen(path, "r+b");
    if (!f) f = fopen(path, "w+b");
    if (!f) return host_fallo();
    /* Más allá del final, fseek+fwrite extiende. El hueco lo rellena de ceros
     * la libc de POSIX, pero el contrato promete INDEFINIDO porque FatFs no lo
     * hace — no se puede prometer aquí lo que allí no se cumple. */
    if (fseek(f, (long) off, SEEK_SET) != 0) { int e = errno; fclose(f); return host_fallo_de(e); }
    errno = 0;
    size_t n = fwrite(data, 1, (size_t) len, f);
    if (n != (size_t) len) { int r = host_corto(); fclose(f); return r; }
    if (fclose(f) != 0) return host_corto();
    return (long) n;
}

static int host_truncate(const char* path, uint32_t size) {
    EN_FLASH(path);
    /* No hay truncate portable en C99: Windows tiene _chsize_s y POSIX
     * ftruncate. Se copia el trozo que sobrevive y se reescribe — es O(n) y
     * sólo vale porque en el host no hay presión de rendimiento; en las placas
     * lo hace el motor de verdad (f_truncate / lfs_file_truncate). */
    FILE* f = fopen(path, "rb");
    uint8_t* buf = NULL;
    size_t leidos = 0;
    if (f) {
        if (size > 0) {
            buf = (uint8_t*) malloc(size);
            if (!buf) { fclose(f); return host_fallo_de(ENOMEM); }
            leidos = fread(buf, 1, (size_t) size, f);
        }
        fclose(f);
    }
    FILE* o = fopen(path, "wb");
    if (!o) { int e = errno; free(buf); return host_fallo_de(e); }
    int mal = 0;
    errno = 0;
    if (leidos && fwrite(buf, 1, leidos, o) != leidos) mal = 1;
    /* Agrandar: ceros hasta el tamaño pedido (mismo hueco "indefinido" del
     * contrato — aquí sale barato dejarlo definido y no cuesta prometer menos). */
    for (uint32_t i = (uint32_t) leidos; !mal && i < size; i++)
        if (fputc(0, o) == EOF) mal = 1;
    if (fclose(o) != 0) mal = 1;
    free(buf);
    return mal ? host_corto() : 0;
}

static long host_read_at(const char* path, uint32_t off, uint8_t* dst, uint32_t cap) {
    EN_FLASH(path);
    FILE* f = fopen(path, "rb");
    if (!f) return host_fallo();
    if (fseek(f, (long) off, SEEK_SET) != 0) { int e = errno; fclose(f); return host_fallo_de(e); }
    size_t n = fread(dst, 1, (size_t) cap, f);
    fclose(f);
    return (long) n;
}

static int host_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    EN_FLASH(path);
    FILE* f = fopen(path, append ? "ab" : "wb");
    if (!f) return host_fallo();
    errno = 0;
    if (len > 0 && data) {
        if (fwrite(data, 1, (size_t) len, f) != (size_t) len) { int r = host_corto(); fclose(f); return r; }
    }
    /* #628: el close VUELCA, y se miraba sin mirar: un disco lleno al volcar
     * contestaba «escrito». */
    if (fclose(f) != 0) return host_corto();
    return 0;
}

/* #240 — borrado y rename para el logger (rotación y limpieza). */
static int host_remove(const char* path) {
    EN_FLASH(path);
    return (remove(path) == 0) ? 0 : host_fallo();
}

static int host_rename(const char* from, const char* to) {
    EN_FLASH(from);
    EN_FLASH(to);
    /* Semántica de la VM-Java (REPLACE_EXISTING): en Windows rename() falla
     * si el destino existe — lo borramos antes (en POSIX rename ya
     * sobreescribe y el remove previo es inocuo). */
    FILE* f = fopen(to, "rb");
    if (f) { fclose(f); remove(to); }
    return (rename(from, to) == 0) ? 0 : host_fallo();
}

/* #240 (2ª pasada) — resto de IO.bp en host. fs_host.c es HOST-ONLY, así que
 * podemos salirnos de C99 puro: _mkdir/_stat en Windows, mkdir/stat POSIX. */
#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#define HOST_MKDIR(p)  _mkdir(p)
#define HOST_RMDIR(p)  _rmdir(p)
typedef struct _stat host_stat_t;
#define HOST_STATFN(p, st) _stat((p), (st))
#define HOST_ISDIR(m)  (((m) & _S_IFDIR) != 0)
#else
#include <sys/stat.h>
#include <unistd.h>
#define HOST_MKDIR(p)  mkdir((p), 0777)
#define HOST_RMDIR(p)  rmdir(p)
typedef struct stat host_stat_t;
#define HOST_STATFN(p, st) stat((p), (st))
#define HOST_ISDIR(m)  S_ISDIR(m)
#endif

/* mkdir recursivo (crea intermedios; ok si ya existe — semántica VM-Java
 * Files.createDirectories). Con una ruta del PC (#808: la de la placa ya lleva delante la flash). */
static int host_mkdir_pc(const char* path) {
    char buf[1300];   /* #808: la flash y la ruta de la placa, como EN_FLASH */
    size_t n = strlen(path);
    if (n == 0 || n >= sizeof(buf)) {
        bpvm_fs_anota_fallo("host", 0, n ? BPVM_FS_FAIL_NAME_TOO_LONG : BPVM_FS_FAIL_OTHER,
                            n ? "ruta demasiado larga" : "ruta vacia");
        return -1;
    }
    memcpy(buf, path, n + 1);
    for (size_t i = 1; i < n; i++) {
        if (buf[i] == '/' || buf[i] == '\\') {
            char c = buf[i];
            buf[i] = '\0';
            HOST_MKDIR(buf);   /* ignora fallo: puede existir o ser "C:" */
            buf[i] = c;
        }
    }
    if (HOST_MKDIR(buf) == 0) return 0;
    int e = errno;   /* antes del stat, que lo puede pisar */
    /* ya existía como directorio → ok */
    host_stat_t st;
    if (HOST_STATFN(buf, &st) == 0 && HOST_ISDIR(st.st_mode)) return 0;
    return host_fallo_de(e);
}
static int host_mkdir(const char* path) {
    EN_FLASH(path);
    return host_mkdir_pc(path);
}
/* #808 — la carpeta de la flash, al fijarla; si no se puede, lo dirá la primera escritura. */
static void host_crea_carpeta(const char* dir) { (void) host_mkdir_pc(dir); }

static int host_rmdir(const char* path) {
    EN_FLASH(path);
    return (HOST_RMDIR(path) == 0) ? 0 : host_fallo();   /* falla si no vacío — como Java */
}

static int host_copy(const char* from, const char* to) {
    EN_FLASH(from);
    EN_FLASH(to);
    FILE* fi = fopen(from, "rb");
    if (!fi) return host_fallo();
    FILE* fo = fopen(to, "wb");   /* sobreescribe (REPLACE_EXISTING) */
    if (!fo) { int e = errno; fclose(fi); return host_fallo_de(e); }
    char buf[4096];
    size_t n;
    int rc = 0;
    errno = 0;
    while ((n = fread(buf, 1, sizeof(buf), fi)) > 0) {
        if (fwrite(buf, 1, n, fo) != n) { rc = host_corto(); break; }
    }
    if (rc == 0 && ferror(fi)) rc = host_fallo_de(errno ? errno : EIO);
    fclose(fi);
    if (fclose(fo) != 0 && rc == 0) rc = host_corto();
    return rc;
}

static int host_isdir(const char* path) {
    EN_FLASH_O(path, 0);   /* no es una carpeta (aquí, -1 sería «sí») */
    host_stat_t st;
    return (HOST_STATFN(path, &st) == 0 && HOST_ISDIR(st.st_mode)) ? 1 : 0;
}

static long long host_mtime_ms(const char* path) {
    EN_FLASH(path);
    host_stat_t st;
    if (HOST_STATFN(path, &st) != 0) return -1;
    return (long long) st.st_mtime * 1000LL;
}

/* H2·B1.3 — listado (dirent existe en MinGW y POSIX; fs_host es host-only). */
#include <dirent.h>
static int host_list(const char* path,
                     void (*cb)(const char* name, int is_dir, uint32_t size, void* user),
                     void* user) {
    EN_FLASH(path);
    DIR* d = opendir((path && path[0]) ? path : ".");
    if (!d) return host_fallo();
    struct dirent* e;
    char full[1600];
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        snprintf(full, sizeof full, "%s/%s", path, e->d_name);
        host_stat_t st;
        int isd = 0;
        uint32_t sz = 0;
        if (HOST_STATFN(full, &st) == 0) {
            isd = HOST_ISDIR(st.st_mode) ? 1 : 0;
            sz  = (uint32_t) st.st_size;
        }
        cb(e->d_name, isd, sz, user);
    }
    closedir(d);
    return 0;
}

/* V7/H3 (#491) — EL CURSOR en el host: el estado es un `FILE*`, copiado con
 * memcpy porque el bloque del llamador va alineado a 4 y un puntero mide 8. El
 * "a" abre en "r+b" (o crea) y la fachada pone el cursor al final: el "ab" de
 * la libc forzaría CADA escritura al final, que no es el contrato de las placas. */
static FILE* host_fh(void* st) { FILE* f; memcpy(&f, st, sizeof f); return f; }
static int host_fopen(void* st, const char* path, int mode) {
    EN_FLASH(path);
    FILE* f = NULL;
    switch (mode) {
        case BPVM_FS_O_READ:   f = fopen(path, "rb"); break;
        case BPVM_FS_O_WRITE:  f = fopen(path, "w+b"); break;
        case BPVM_FS_O_APPEND: f = fopen(path, "r+b"); if (!f) f = fopen(path, "w+b"); break;
        case BPVM_FS_O_RW:     f = fopen(path, "r+b"); break;
        default: errno = EINVAL; break;
    }
    if (!f) return host_fallo();
    memcpy(st, &f, sizeof f);
    return 0;
}
static int host_fclose(void* st) { return (fclose(host_fh(st)) == 0) ? 0 : host_corto(); }
static long host_fread(void* st, uint8_t* dst, uint32_t n) {
    FILE* f = host_fh(st);
    errno = 0;
    size_t r = fread(dst, 1, (size_t) n, f);
    return (r == 0 && ferror(f)) ? host_fallo_de(errno ? errno : EIO) : (long) r;
}
static long host_fwrite(void* st, const uint8_t* src, uint32_t n) {
    errno = 0;
    size_t r = fwrite(src, 1, (size_t) n, host_fh(st));
    return (r == (size_t) n) ? (long) r : host_corto();
}
static int host_fseek(void* st, uint32_t pos) { return (fseek(host_fh(st), (long) pos, SEEK_SET) == 0) ? 0 : host_fallo(); }
static long host_fsize(void* st) {
    FILE* f = host_fh(st);
    long cur = ftell(f);
    if (cur < 0 || fseek(f, 0, SEEK_END) != 0) return host_fallo();
    long sz = ftell(f);
    if (fseek(f, cur, SEEK_SET) != 0) return host_fallo();
    return sz;
}
static int host_fsync(void* st) { errno = 0; return (fflush(host_fh(st)) == 0) ? 0 : host_corto(); }

static const bpvm_fs_backend_t s_host_fs = {
    .stat     = host_stat,
    .read     = host_read,
    .write    = host_write,
    .remove   = host_remove,
    .rename   = host_rename,
    .mkdir    = host_mkdir,
    .rmdir    = host_rmdir,
    .copy     = host_copy,
    .isdir    = host_isdir,
    .mtime_ms = host_mtime_ms,
    .list     = host_list,   /* B1.3 */
    .read_at  = host_read_at, /* #305 — lectura por trozos */
    .write_at = host_write_at, /* V5/H2 — paridad con la placa */
    .truncate = host_truncate,
    .crc32    = host_crc32,   /* #398 — mismo camino que la placa: oraculo valido */
    .file_state_size = (uint32_t) sizeof(FILE*),   /* V7/H3 (#491) — el cursor */
    .fopen = host_fopen, .fclose = host_fclose, .fread = host_fread, .fwrite = host_fwrite,
    .fseek = host_fseek, .fsize = host_fsize, .fsync = host_fsync,
};

void bpvm_fs_register_host(void) {
    bpvm_fs_set_backend(&s_host_fs);
}

/* ── [V7/H2, #493] `/lib` EN EL HOST ─────────────────────────────────────────
 * Un backend de SÓLO LECTURA montado en un prefijo y enraizado en un directorio
 * del PC: `/lib/Str.mod` → `<root>/Str.mod`. Es el espejo de `--stdlibDir` de
 * miVM. Sin esto la VM-C del PC no tenía manera de decir dónde vive la stdlib
 * (`/lib/X.mod` era un `fopen` literal, o sea `C:\lib`), y por eso el aviso de
 * «tapa al de /lib» y el orden /app → /lib no se podían probar por paridad.
 * Una sola raíz: la última llamada manda. Lo que no sea leer (escribir, borrar,
 * listar) no está: la stdlib del PC no se toca desde un programa. */
static char   s_dir_root[512];
static char   s_dir_prefix[32];
static size_t s_dir_prefix_len;

/* `/lib/X` → `<root>/X`; 0 si la ruta no cuelga del prefijo o no cabe (con el motivo anotado, #628). */
static int dir_map_(const char* path, char* out, size_t cap) {
    if (!path || !s_dir_root[0]) return 0;
    if (strncmp(path, s_dir_prefix, s_dir_prefix_len) != 0) return 0;
    const char* resto = path + s_dir_prefix_len;
    if (*resto == '/') resto++;
    else if (*resto != '\0') return 0;
    int n = snprintf(out, cap, "%s/%s", s_dir_root, resto);
    return n > 0 && (size_t) n < cap;
}
static int dir_map(const char* path, char* out, size_t cap) {
    if (dir_map_(path, out, cap)) return 1;
    bpvm_fs_anota_fallo("host", 0, BPVM_FS_FAIL_NOT_FOUND, "la ruta no es de esa carpeta o no cabe");
    return 0;
}

static int dir_stat(const char* path, uint32_t* size) {
    char real[768]; if (!dir_map(path, real, sizeof real)) return -1;
    return host_stat(real, size);
}
static long dir_read(const char* path, uint8_t* dst, uint32_t cap) {
    char real[768]; if (!dir_map(path, real, sizeof real)) return -1;
    return host_read(real, dst, cap);
}
static long dir_read_at(const char* path, uint32_t off, uint8_t* dst, uint32_t cap) {
    char real[768]; if (!dir_map(path, real, sizeof real)) return -1;
    return host_read_at(real, off, dst, cap);
}
static int dir_crc32(const char* path, uint32_t* crc_out) {
    char real[768]; if (!dir_map(path, real, sizeof real)) return -1;
    return host_crc32(real, crc_out);
}
static int dir_isdir(const char* path) {
    char real[768]; if (!dir_map(path, real, sizeof real)) return 0;
    return host_isdir(real);
}
static long long dir_mtime_ms(const char* path) {
    char real[768]; if (!dir_map(path, real, sizeof real)) return -1;
    return host_mtime_ms(real);
}

static const bpvm_fs_backend_t s_host_dir_fs = {
    .stat     = dir_stat,
    .read     = dir_read,
    .isdir    = dir_isdir,
    .mtime_ms = dir_mtime_ms,
    .read_at  = dir_read_at,
    .crc32    = dir_crc32,
};

int bpvm_fs_register_host_dir(const char* prefix, const char* root) {
    if (!prefix || prefix[0] != '/' || !root || !root[0]) return -1;
    size_t pl = strlen(prefix), rl = strlen(root);
    if (pl >= sizeof s_dir_prefix || rl >= sizeof s_dir_root) return -1;
    memcpy(s_dir_prefix, prefix, pl + 1);
    s_dir_prefix_len = pl;
    memcpy(s_dir_root, root, rl + 1);
    while (rl > 1 && (s_dir_root[rl - 1] == '/' || s_dir_root[rl - 1] == '\\')) s_dir_root[--rl] = '\0';
    return bpvm_fs_mount(prefix, &s_host_dir_fs);
}
