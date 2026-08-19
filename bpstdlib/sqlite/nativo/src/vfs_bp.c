/* vfs_bp.c — el VFS de SQLite sobre las RANURAS DE FICHERO de la tabla BIOS.
 *
 * Es el port de `D/vfs_min.c` (que iba sobre stdio y funcionó a la primera en el
 * PC). Lo que cambia es la CINTURA, no la forma: donde aquél hacía `fopen` /
 * `fseek+fread`, éste llama a `abrir` / `leer(fd, desde, ...)`. Las ranuras de la
 * BIOS ya son POSICIONALES —no hay estado de "puntero del fichero"—, que es
 * exactamente lo que SQLite quiere: nunca hace seek, siempre dice el offset.
 *
 * ⚠️ NO ES OPCIONAL NI APLAZABLE. `sqlite3_initialize()` FALLA con SQLITE_ERROR
 * si no hay ningún vfs registrado: `sqlite3MemdbInit()` hace `sqlite3_vfs_find(0)`
 * y se rinde si no encuentra ninguno — y va envuelto en `NEVER()`, así que ni
 * siquiera lo loguea. Costó tres grabaciones descubrirlo (8-ago). O sea que esto
 * hace falta para que SQLite ARRANQUE, no sólo para abrir una base de datos.
 *
 * La tabla se recibe por `bpvfs_instalar()` en vez de leerla de un global del
 * pack: así este fichero se puede probar en el PC con una tabla de mentira
 * montada sobre stdio, que es como se depuró.
 */
#include "sqlite3.h"
#include "bpvm_bios.h"

/* La tabla, guardada al instalar. Todo lo de aquí pasa por ella. */
static const bpvm_bios_t* vb;

/* Nombre máximo. Modesto a propósito: cada fichero abierto de SQLite se aloca
 * en la arena con este tamaño dentro, y en la placa los caminos son cortos
 * (`/sd/algo.db`). Tiene que coincidir con `mxPathname` de abajo. */
#define VBP_CAMINO 128

typedef struct BpFile {
    sqlite3_file base;              /* OBLIGATORIO el primero: SQLite castea  */
    int          fd;                /* el de la BIOS; base 1, 0 nunca es válido */
    int          borrar_al_cerrar;
    char         nombre[VBP_CAMINO];
} BpFile;

/* ── Guardia de los offsets ───────────────────────────────────────────────────
 * SQLite habla en `sqlite3_int64` y las ranuras en `uint32_t`. Con ficheros de
 * más de 4 GB el valor se truncaría y se leería/escribiría en OTRO sitio, en
 * silencio. Aquí se rechaza: en un micro no vamos a tener una BD de 4 GB, pero
 * un truncamiento mudo no se detecta nunca. */
static int cabe32(sqlite3_int64 v) { return v >= 0 && v <= (sqlite3_int64) 0xFFFFFFFFu; }

/* ── Métodos del fichero ── */

static int bfClose(sqlite3_file* p) {
    BpFile* m = (BpFile*) p;
    int r = SQLITE_OK;
    if (m->fd > 0 && vb->cerrar(m->fd) != 0) r = SQLITE_IOERR_CLOSE;
    m->fd = 0;
    if (m->borrar_al_cerrar) vb->borrar(m->nombre);
    return r;
}

static int bfRead(sqlite3_file* p, void* buf, int n, sqlite3_int64 off) {
    BpFile* m = (BpFile*) p;
    if (!cabe32(off) || n < 0) return SQLITE_IOERR_READ;

    long leidos = vb->leer(m->fd, (uint32_t) off, buf, (uint32_t) n);

    /* ── EL FICHERO QUE AÚN NO EXISTE ─────────────────────────────────────────
     *
     * `abrir(camino, 1)` NO crea: lo crea el primer `write_at`. Así que entre
     * abrir una BD nueva y escribir su primera página, el fichero no está — y
     * `leer` contesta −1, que aquí significa "no existe", no "el disco falló".
     *
     * Para SQLite esos dos casos son OPUESTOS: un fichero de cero bytes es una
     * base de datos NUEVA y perfectamente válida; un error de E/S aborta el
     * `open`. Traducir entre las dos semánticas es justo para lo que existe un
     * VFS, y por eso el arreglo va aquí y no en la tabla BIOS.
     *
     * ⚠️ La comprobación es ESTRECHA a propósito: sólo se perdona si el fichero
     * de verdad no existe. Si existe y la lectura falla, eso ES un error y se
     * dice. Perdonar de más convertiría un fallo de la tarjeta en una BD que
     * parece vacía, que es la peor forma de perder datos: en silencio. */
    if (leidos < 0) {
        if (vb->existe(m->nombre)) return SQLITE_IOERR_READ;
        leidos = 0;
    }
    if (leidos == n) return SQLITE_OK;

    /* Lectura corta: SQLite EXIGE rellenar a cero el resto y devolver
     * SHORT_READ. Devolver OK con basura detrás no falla aquí — corrompe la BD
     * y se manifiesta mucho después, en otro sitio y sin relación aparente. */
    vb->memset((char*) buf + leidos, 0, (size_t) (n - leidos));
    return SQLITE_IOERR_SHORT_READ;
}

static int bfWrite(sqlite3_file* p, const void* buf, int n, sqlite3_int64 off) {
    BpFile* m = (BpFile*) p;
    if (!cabe32(off) || n < 0) return SQLITE_IOERR_WRITE;
    long esc = vb->escribir(m->fd, (uint32_t) off, buf, (uint32_t) n);
    return (esc == n) ? SQLITE_OK : SQLITE_IOERR_WRITE;
}

static int bfTruncate(sqlite3_file* p, sqlite3_int64 size) {
    BpFile* m = (BpFile*) p;
    if (!cabe32(size)) return SQLITE_IOERR_TRUNCATE;
    return vb->truncar(m->fd, (uint32_t) size) == 0 ? SQLITE_OK : SQLITE_IOERR_TRUNCATE;
}

static int bfSync(sqlite3_file* p, int flags) {
    BpFile* m = (BpFile*) p;
    (void) flags;
    /* Esto es lo que hace que la BD sobreviva a un corte. En la SD acaba en
     * `f_sync` de FatFs; en la flash interna, en el flush de littlefs. */
    return vb->sincronizar(m->fd) == 0 ? SQLITE_OK : SQLITE_IOERR_FSYNC;
}

static int bfFileSize(sqlite3_file* p, sqlite3_int64* pSize) {
    BpFile* m = (BpFile*) p;
    long t = vb->tamano(m->fd);
    if (t < 0) {
        /* Mismo caso que en `bfRead`: el fichero aún no existe porque nadie ha
         * escrito todavía. Su tamaño ES cero — eso es lo que le dice a SQLite
         * "base de datos nueva". Y estrecho igual: si existe y falla, es error. */
        if (vb->existe(m->nombre)) return SQLITE_IOERR_FSTAT;
        t = 0;
    }
    *pSize = (sqlite3_int64) t;
    return SQLITE_OK;
}

/* Bloqueo: no-op, y con motivo. La VM ejecuta UN programa y no hay otro proceso
 * que pueda abrir la misma BD — en el micro es literalmente imposible. Lo que
 * NO se puede es mentir en `xCheckReservedLock`: se dice que no hay reserva. */
static int bfLock     (sqlite3_file* p, int e) { (void)p;(void)e; return SQLITE_OK; }
static int bfUnlock   (sqlite3_file* p, int e) { (void)p;(void)e; return SQLITE_OK; }
static int bfCheckLock(sqlite3_file* p, int* r){ (void)p; *r = 0;  return SQLITE_OK; }
static int bfControl  (sqlite3_file* p, int op, void* a) {
    (void)p;(void)a;(void)op; return SQLITE_NOTFOUND;
}
static int bfSectorSize(sqlite3_file* p) { (void)p; return 512; }   /* el bloque de la SD */
static int bfDevChar   (sqlite3_file* p) { (void)p; return 0; }

static const sqlite3_io_methods BP_IO = {
    1,
    bfClose, bfRead, bfWrite, bfTruncate, bfSync, bfFileSize,
    bfLock, bfUnlock, bfCheckLock, bfControl, bfSectorSize, bfDevChar
};

/* ── Métodos del VFS ── */

/* Copia acotada y siempre terminada. No se usa `sqlite3_snprintf` aquí porque
 * esto corre DENTRO de `sqlite3_initialize` en el caso del registro, y cuanto
 * menos se dependa de la maquinaria a medio montar, mejor. */
static void copia(char* dst, size_t max, const char* src) {
    size_t i = 0;
    if (max == 0) return;
    while (src && src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int bvOpen(sqlite3_vfs* v, const char* nombre, sqlite3_file* p,
                  int flags, int* pOutFlags) {
    (void) v;
    BpFile* m = (BpFile*) p;
    vb->memset(m, 0, sizeof *m);

    /* Nombre NULO = temporal anónimo. Con TEMP_STORE=3 los temporales van a RAM
     * y esto casi no se usa, pero el journal SÍ tiene nombre y pasa por aquí. */
    char tmp[VBP_CAMINO];
    if (nombre == 0) {
        static unsigned n = 0;
        sqlite3_snprintf((int) sizeof tmp, tmp, "bpsql_%u.tmp", n++);
        nombre = tmp;
        m->borrar_al_cerrar = 1;
    }
    copia(m->nombre, sizeof m->nombre, nombre);
    if (flags & SQLITE_OPEN_DELETEONCLOSE) m->borrar_al_cerrar = 1;

    /* `abrir(camino, para_escribir)`: para LEER exige que exista; para escribir
     * no, porque `write_at` lo crea — que es justo lo que hace una BD nueva. */
    int escribir = (flags & SQLITE_OPEN_READONLY) ? 0 : 1;
    m->fd = vb->abrir(nombre, escribir);
    if (m->fd <= 0) return SQLITE_CANTOPEN;

    m->base.pMethods = &BP_IO;
    if (pOutFlags) *pOutFlags = flags;
    return SQLITE_OK;
}

static int bvDelete(sqlite3_vfs* v, const char* nombre, int syncDir) {
    (void) v; (void) syncDir;
    vb->borrar(nombre);          /* que no exista no es un error para SQLite */
    return SQLITE_OK;
}

static int bvAccess(sqlite3_vfs* v, const char* nombre, int flags, int* pRes) {
    (void) v; (void) flags;      /* READ/WRITE/EXISTS: aquí todo es "existe"  */
    *pRes = vb->existe(nombre) ? 1 : 0;
    return SQLITE_OK;
}

static int bvFullPathname(sqlite3_vfs* v, const char* in, int nOut, char* out) {
    (void) v;
    /* Sin directorio de trabajo: el camino que da el usuario ES el camino
     * (`/sd/datos.db`). La fachada del FS ya resuelve el prefijo del montaje. */
    copia(out, (size_t) nOut, in);
    return SQLITE_OK;
}

static int bvRandomness(sqlite3_vfs* v, int n, char* out) {
    (void) v;
    /* ⚠️ FLOJO A PROPÓSITO, y anotado: la tabla BIOS todavía NO tiene ranura de
     * aleatorios (el HAL de #347 vive en el firmware pero no se presta al pack).
     * SQLite usa esto para la semilla de su PRNG; con una fuente pobre la BD es
     * igual de correcta, sólo menos impredecible — y aquí no hay adversario.
     * Cuando entre la ranura, esto es una línea. */
    static unsigned s = 0x2545F491u;
    for (int i = 0; i < n; i++) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        out[i] = (char) (s & 0xFF);
    }
    return n;
}

static int bvSleep(sqlite3_vfs* v, int us) { (void)v; (void)us; return 0; }

static int bvCurrentTime(sqlite3_vfs* v, double* pT) {
    (void) v;
    /* ⚠️ Igual: sin RTC prestado, una constante (1-ene-1970 en día juliano).
     * Sólo afecta a `CURRENT_TIMESTAMP`; el motor no la usa para nada interno.
     * La ranura `localtime` existe pero necesita un `time_t` que aquí no hay. */
    *pT = 2440587.5;
    return SQLITE_OK;
}

static int bvGetLastError(sqlite3_vfs* v, int n, char* s) {
    (void)v; (void)n; (void)s; return 0;
}

static sqlite3_vfs BP_VFS = {
    1,                      /* iVersion    */
    sizeof(BpFile),         /* szOsFile    */
    VBP_CAMINO,             /* mxPathname  */
    0,                      /* pNext       */
    "bp",                   /* zName       */
    0,                      /* pAppData    */
    bvOpen, bvDelete, bvAccess, bvFullPathname,
    0, 0, 0, 0,             /* dlOpen/Error/Sym/Close — OMIT_LOAD_EXTENSION   */
    bvRandomness, bvSleep, bvCurrentTime, bvGetLastError
};

/*
 * Instala el VFS como el POR DEFECTO. Lo llama `sqlite3_os_init()`, que SQLite
 * invoca desde dentro de `sqlite3_initialize()`.
 *
 * Se le pasa la tabla en vez de que la busque: así el mismo fichero vale para la
 * placa y para el arnés del PC, que le da una tabla montada sobre stdio.
 */
int bpvfs_instalar(const bpvm_bios_t* bios)
{
    if (bios == 0) return SQLITE_ERROR;
    /* Las ranuras que ESTE fichero usa, comprobadas de una vez: si falta alguna
     * es mejor un no limpio aquí que un salto a NULL en mitad de una escritura. */
    if (!bios->abrir || !bios->cerrar || !bios->leer || !bios->escribir ||
        !bios->truncar || !bios->tamano || !bios->sincronizar ||
        !bios->borrar || !bios->existe || !bios->memset) return SQLITE_ERROR;

    vb = bios;
    return sqlite3_vfs_register(&BP_VFS, 1);   /* 1 = por defecto */
}
