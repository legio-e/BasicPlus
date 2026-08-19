/* vfs_min.c — PRUEBA D: el VFS mínimo.
 *
 * Con SQLITE_OS_OTHER=1, SQLite no trae NINGUNA capa de sistema: la tiene que
 * poner entera quien lo integra, a través de sqlite3_os_init(). Esto mide
 * cuánto es "entera".
 *
 * Aquí va sobre stdio (host). En la placa, las mismas funciones irán sobre la
 * fachada del FS (littlefs). La FORMA es lo que se está probando: si el hueco
 * que hay que rellenar es de este tamaño, la versión de placa es un cambio de
 * cintura, no un diseño nuevo.
 *
 * Simplificaciones DELIBERADAS, y por qué se sostienen en nuestro caso:
 *   - Bloqueo: no-op. La VM ejecuta UN programa; no hay otro proceso abriendo
 *     la misma BD. (En el micro, literalmente imposible.)
 *   - Sin dlopen: SQLITE_OMIT_LOAD_EXTENSION.
 *   - Sin ficheros temporales en disco: SQLITE_TEMP_STORE=3 los deja en RAM.
 */
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── El fichero abierto ── */
typedef struct MinFile {
    sqlite3_file base;      /* OBLIGATORIO el primero: SQLite hace el cast */
    FILE*        f;
    char         nombre[512];
    int          borrar_al_cerrar;
} MinFile;

/* ── Métodos del fichero ── */

static int mfClose(sqlite3_file* p) {
    MinFile* m = (MinFile*) p;
    if (m->f) { fclose(m->f); m->f = NULL; }
    if (m->borrar_al_cerrar) remove(m->nombre);
    return SQLITE_OK;
}

static int mfRead(sqlite3_file* p, void* buf, int n, sqlite3_int64 off) {
    MinFile* m = (MinFile*) p;
    if (fseek(m->f, (long) off, SEEK_SET) != 0) return SQLITE_IOERR_READ;
    size_t leidos = fread(buf, 1, (size_t) n, m->f);
    if (leidos == (size_t) n) return SQLITE_OK;
    /* Lectura corta: SQLite EXIGE que el resto se rellene a cero y que se
     * devuelva SHORT_READ. Si se devuelve OK con basura detrás, la corrupción
     * aparece mucho después y en otro sitio. */
    memset((char*) buf + leidos, 0, (size_t) n - leidos);
    return SQLITE_IOERR_SHORT_READ;
}

static int mfWrite(sqlite3_file* p, const void* buf, int n, sqlite3_int64 off) {
    MinFile* m = (MinFile*) p;
    if (fseek(m->f, (long) off, SEEK_SET) != 0) return SQLITE_IOERR_WRITE;
    if (fwrite(buf, 1, (size_t) n, m->f) != (size_t) n) return SQLITE_IOERR_WRITE;
    return SQLITE_OK;
}

static int mfTruncate(sqlite3_file* p, sqlite3_int64 size) {
    MinFile* m = (MinFile*) p;
    fflush(m->f);
    /* stdio puro no trunca. En la placa, littlefs sí. Aquí no es crítico:
     * SQLite tolera un fichero más largo de la cuenta. */
    (void) size;
    return SQLITE_OK;
}

static int mfSync(sqlite3_file* p, int flags) {
    MinFile* m = (MinFile*) p;
    (void) flags;
    return fflush(m->f) == 0 ? SQLITE_OK : SQLITE_IOERR_FSYNC;
}

static int mfFileSize(sqlite3_file* p, sqlite3_int64* pSize) {
    MinFile* m = (MinFile*) p;
    fflush(m->f);
    long pos = ftell(m->f);
    if (fseek(m->f, 0, SEEK_END) != 0) return SQLITE_IOERR_FSTAT;
    *pSize = (sqlite3_int64) ftell(m->f);
    fseek(m->f, pos, SEEK_SET);
    return SQLITE_OK;
}

/* Bloqueo: no-op (ver cabecera). */
static int mfLock       (sqlite3_file* p, int e) { (void)p;(void)e; return SQLITE_OK; }
static int mfUnlock     (sqlite3_file* p, int e) { (void)p;(void)e; return SQLITE_OK; }
static int mfCheckLock  (sqlite3_file* p, int* r){ (void)p; *r = 0;  return SQLITE_OK; }
static int mfFileControl(sqlite3_file* p, int op, void* a) {
    (void)p;(void)a;(void)op; return SQLITE_NOTFOUND;
}
static int mfSectorSize (sqlite3_file* p) { (void)p; return 512; }
static int mfDevChar    (sqlite3_file* p) { (void)p; return 0; }

static const sqlite3_io_methods MIN_IO = {
    1,                  /* iVersion */
    mfClose, mfRead, mfWrite, mfTruncate, mfSync, mfFileSize,
    mfLock, mfUnlock, mfCheckLock, mfFileControl, mfSectorSize, mfDevChar
};

/* ── Métodos del VFS ── */

static int mvOpen(sqlite3_vfs* v, const char* nombre, sqlite3_file* p,
                  int flags, int* pOutFlags) {
    (void) v;
    MinFile* m = (MinFile*) p;
    memset(m, 0, sizeof *m);

    char tmp[512];
    if (nombre == NULL) {                       /* fichero temporal anónimo */
        static int n = 0;
        snprintf(tmp, sizeof tmp, "sqlite_tmp_%d.db", n++);
        nombre = tmp;
        m->borrar_al_cerrar = 1;
    }
    snprintf(m->nombre, sizeof m->nombre, "%s", nombre);
    if (flags & SQLITE_OPEN_DELETEONCLOSE) m->borrar_al_cerrar = 1;

    /* "r+b" no crea; "w+b" trunca. Para no perder una BD existente, se intenta
     * abrir y sólo si no existe se crea. */
    m->f = fopen(nombre, "r+b");
    if (!m->f && (flags & SQLITE_OPEN_CREATE)) m->f = fopen(nombre, "w+b");
    if (!m->f) return SQLITE_CANTOPEN;

    m->base.pMethods = &MIN_IO;
    if (pOutFlags) *pOutFlags = flags;
    return SQLITE_OK;
}

static int mvDelete(sqlite3_vfs* v, const char* nombre, int syncDir) {
    (void) v; (void) syncDir;
    remove(nombre);
    return SQLITE_OK;
}

static int mvAccess(sqlite3_vfs* v, const char* nombre, int flags, int* pRes) {
    (void) v; (void) flags;
    FILE* f = fopen(nombre, "rb");
    *pRes = f ? 1 : 0;
    if (f) fclose(f);
    return SQLITE_OK;
}

static int mvFullPathname(sqlite3_vfs* v, const char* in, int nOut, char* out) {
    (void) v;
    snprintf(out, (size_t) nOut, "%s", in);   /* sin directorios: el micro es plano */
    return SQLITE_OK;
}

static int mvRandomness(sqlite3_vfs* v, int n, char* out) {
    (void) v;
    /* En la placa esto sale del HAL de aleatorios (#347). */
    for (int i = 0; i < n; i++) out[i] = (char) (rand() & 0xFF);
    return n;
}

static int mvSleep(sqlite3_vfs* v, int us) { (void)v; (void)us; return 0; }

static int mvCurrentTime(sqlite3_vfs* v, double* pT) {
    (void) v;
    /* Día juliano. En la placa saldrá del Rtc; si no hay, una constante. */
    *pT = 2440587.5 + (double) time(NULL) / 86400.0;
    return SQLITE_OK;
}

static int mvGetLastError(sqlite3_vfs* v, int n, char* s) {
    (void)v; (void)n; (void)s; return 0;
}

static sqlite3_vfs MIN_VFS = {
    1,                      /* iVersion */
    sizeof(MinFile),        /* szOsFile */
    512,                    /* mxPathname */
    0,                      /* pNext */
    "bpmin",                /* zName */
    0,                      /* pAppData */
    mvOpen, mvDelete, mvAccess, mvFullPathname,
    0, 0, 0, 0,             /* dlOpen/dlError/dlSym/dlClose (OMIT_LOAD_EXTENSION) */
    mvRandomness, mvSleep, mvCurrentTime, mvGetLastError
};

/* ── Los dos que SQLite exige con OS_OTHER=1 ── */

int sqlite3_os_init(void) { return sqlite3_vfs_register(&MIN_VFS, 1); }
int sqlite3_os_end (void) { return SQLITE_OK; }
