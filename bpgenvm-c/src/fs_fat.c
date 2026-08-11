/*
 * fs_fat.c — V5/H2: una tarjeta como sistema de ficheros de la VM.
 *
 * Une tres cosas que ya existían y no se conocían: **FatFs** (motor
 * vendorizado), un **dispositivo de bloque** (`bpvm_blk.h`) y la **fachada
 * `bpvm_fs`** (lo que la VM y el wire saben usar).
 *
 * ─── V5/H6: AQUÍ DEBAJO YA NO HAY UNA SD, HAY UN DISPOSITIVO DE BLOQUE ───
 *
 * Hasta H2 este fichero llamaba a `bpvm_sd_leer_bloque` directamente y guardaba
 * los pines de SPI en sus propios estáticos. Funcionaba porque sólo había una
 * forma de hablar con una tarjeta. El P4 la parte en dos: su zócalo es SDIO y
 * el protocolo lo habla el SDK del fabricante, no nosotros (el porqué, en la
 * cabecera de `bpvm_blk.h`).
 *
 * Así que ahora la cintura de disco de FatFs pide sus bloques a un backend, y
 * este fichero **no sabe qué es un pin**. Lo de arriba —FatFs, la fachada, los
 * verbos del wire— sirve igual con SPI debajo que con SDMMC.
 *
 * Sigue sin llevar un solo #ifdef de familia. Pero SE DA DE ALTA SÓLO EN EL
 * BUILD DE LA PICO. Criterio de Eduardo: a fondo en una placa, y las demás en
 * bloque cuando la cadena esté probada.
 *
 * ─── EL DESPLAZAMIENTO DE LA PARTICIÓN LO SUMAMOS AQUÍ ───
 *
 * Una SD viene con un MBR, y el sistema de ficheros NO empieza en el bloque 0
 * (en la tarjeta de banco empieza en el 2048). FatFs sabe leer tablas de
 * particiones con `FF_MULTI_PARTITION`, pero eso es configuración, mapeos de
 * volumen y una capa más. Sumar una constante en `disk_read`/`disk_write` es una
 * línea, y a FatFs se le presenta un disco que empieza donde empieza el FS.
 * Decisión de simplicidad, y reversible si algún día hay varias particiones.
 *
 * ─── LA CONCURRENCIA: UN LOCK GRUESO, COMO littlefs ───
 *
 * Y hace falta AUNQUE HAYA UN SOLO NÚCLEO, que es lo que no es obvio: FreeRTOS
 * expropia, así que la tarea del wire puede quedarse a medias dentro de `f_read`
 * y dar paso a un thread BP que llame a `f_open`. Con el buffer de nombres
 * largos estático —que FatFs marca "always NOT thread-safe"— eso no revienta:
 * devuelve un nombre mal o corrompe una entrada de directorio. Silencioso.
 *
 * Se usa el MISMO patrón que `fs_lfs.c` (verificado allí rojo→verde con 4
 * hilos) en vez de `FF_FS_REENTRANT`: un mecanismo y no dos, porque dos
 * cerrojos de ámbitos distintos que no se conocen es como nacen los
 * interbloqueos. Que haya uno por backend no es problema — protegen volúmenes
 * DISJUNTOS y la fachada rechaza las operaciones que cruzarían dos montajes.
 *
 * ⚠️ REGLA DE LA CASA, y el guardián de más abajo la vigila: **nadie llama a
 * `f_*` fuera de este fichero, y aquí sólo por dentro del lock.**
 */
#include "bpvm_fs_fat.h"   /* el propio: así el compilador CONTRASTA los dos */
#include "bpvm_fs.h"
#include "bpvm_blk.h"
#include "bpvm_rtc.h"
#include "bpvm_platform.h"
#include "ff.h"
#include "diskio.h"

#include <string.h>

/* ── Estado. Uno solo: un medio, un volumen. ─────────────────────────────── */
static FATFS            s_fatfs;
/* V5/H6 — el dispositivo, no sus pines. Antes aquí vivían un `bpvm_sd_pines_t`
 * y un `bpvm_sd_info_t`: este fichero sabía lo que es una tarjeta SD y lo que
 * es un pin de SPI, y las dos cosas sobraban. Ahora ese estado está donde lo
 * entiende alguien (`bpvm_sd_blk.c`), y aquí sólo queda a quién preguntar. */
static const bpvm_blk_backend_t* s_blk = NULL;
static uint32_t         s_lba0      = 0;   /* primer bloque de la partición   */
static int              s_montado   = 0;
/* V5/H2 — ¿ya se ha intentado montar ESTA inserción? Evita que un montaje que
 * falla se reintente en cada vuelta de la vigilancia. Se limpia cuando el
 * zócalo se queda vacío, que es lo que separa una inserción de la siguiente. */
static int              s_intentada = 0;
static char             s_prefijo[16] = "/sd";

static bpvm_platform_mutex_handle_t s_lock;
static int s_lock_listo = 0;
static void trabar(void)   { if (s_lock_listo) bpvm_platform_mutex_lock(&s_lock); }
static void destrabar(void){ if (s_lock_listo) bpvm_platform_mutex_unlock(&s_lock); }

/* ─────────────────────────────────────────────────────────────────────────
 * La cintura que FatFs espera. Nombres impuestos por diskio.h.
 * ───────────────────────────────────────────────────────────────────────── */

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    if (!s_montado || !s_blk) return STA_NOINIT;
    /* Si la placa tiene detector y dice que no hay tarjeta, decirlo: mejor un
     * "no hay disco" honesto que veinte comandos al vacío. */
    if (!s_blk->hay_medio()) return STA_NODISK;
    return 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
    /* La tarjeta ya la arrancó `bpvm_fs_fat_montar`. Aquí sólo se contesta,
     * porque FatFs llama a esto en el primer acceso al volumen. */
    return disk_status(pdrv);
}

/* El `count` se pasa ENTERO al dispositivo, en vez de trocearlo en un bucle de
 * bloques sueltos como se hacía antes. Con SPI da exactamente lo mismo —la
 * implementación hace el bucle por dentro—, pero en SDMMC la lectura múltiple
 * (CMD18/CMD25) es donde está la ganancia, y trocear aquí la habría hecho
 * imposible sin volver a tocar este fichero. */
DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || !s_montado || !s_blk) return RES_NOTRDY;
    /* AQUÍ se suma el desplazamiento de la partición (ver cabecera). */
    if (s_blk->leer(s_lba0 + (uint32_t) sector, (uint32_t) count, buff) != 0)
        return RES_ERROR;
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || !s_montado || !s_blk) return RES_NOTRDY;
    if (s_blk->escribir(s_lba0 + (uint32_t) sector, (uint32_t) count, buff) != 0)
        return RES_ERROR;
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv != 0 || !s_montado || !s_blk) return RES_NOTRDY;
    switch (cmd) {
    case CTRL_SYNC:
        /* Si el dispositivo no tiene nada en el aire no ofrece `sincronizar`, y
         * entonces esto es un OK de verdad y no un OK por no saber qué hacer —
         * es el caso de SPI, donde la escritura ya espera a que la tarjeta
         * termine de grabar. */
        if (s_blk->sincronizar && s_blk->sincronizar() != 0) return RES_ERROR;
        return RES_OK;
    case GET_SECTOR_SIZE:    *(WORD*)  buff = BPVM_BLK_TAM;  return RES_OK;
    case GET_BLOCK_SIZE:     *(DWORD*) buff = 1;             return RES_OK;
    case GET_SECTOR_COUNT: {
        /* Los de la PARTICIÓN, no los del medio: FatFs cree que el disco
         * empieza donde empieza el FS, y creerse la capacidad entera le dejaría
         * direccionar más allá del final. */
        uint32_t total = s_blk->bloques();
        *(LBA_t*) buff = (LBA_t) (total > s_lba0 ? total - s_lba0 : 0);
        return RES_OK;
    }
    default: return RES_PARERR;
    }
}

/*
 * La fecha para los ficheros nuevos. FF_FS_NORTC=0, o sea que las placas tienen
 * reloj y los ficheros llevan fecha DE VERDAD — que es lo que hace útil el
 * `mtime_ms` de la fachada, y lo que permite que el PC ordene por fecha.
 *
 * Formato FAT: bits 31..25 año-1980, 24..21 mes, 20..16 día, 15..11 hora,
 * 10..5 minuto, 4..0 segundo/2.
 */
DWORD get_fattime(void) {
    int64_t ms = bpvm_rtc_now_ms();
    if (ms <= 0) return ((DWORD)(2025 - 1980) << 25) | ((DWORD) 1 << 21) | ((DWORD) 1 << 16);
    /* Conversión epoch→civil sin `time.h` (en el micro arrastra bastante).
     * Algoritmo de días→(a,m,d) de Howard Hinnant, dominio público. */
    int64_t s   = ms / 1000;
    int64_t dias = s / 86400;
    int32_t sod = (int32_t) (s - dias * 86400);
    int64_t z = dias + 719468;
    int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t) (z - era * 146097);
    uint32_t yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    int64_t  y   = (int64_t) yoe + era * 400;
    uint32_t doy = doe - (365*yoe + yoe/4 - yoe/100);
    uint32_t mp  = (5*doy + 2)/153;
    uint32_t d   = doy - (153*mp+2)/5 + 1;
    uint32_t m   = mp + (mp < 10 ? 3 : -9);
    y += (m <= 2);
    if (y < 1980) y = 1980;
    return ((DWORD)(y - 1980) << 25) | ((DWORD) m << 21) | ((DWORD) d << 16)
         | ((DWORD)(sod / 3600) << 11) | ((DWORD)((sod / 60) % 60) << 5)
         | ((DWORD)((sod % 60) / 2));
}

/* ─────────────────────────────────────────────────────────────────────────
 * Camino: la fachada pasa el camino COMPLETO, con el prefijo del montaje
 * ───────────────────────────────────────────────────────────────────────── */

/*
 * "/sd/dir/f.txt" → "dir/f.txt". Devuelve NULL si el camino no es de este
 * montaje — no debería pasar (la fachada enruta), pero un camino ajeno tratado
 * como propio escribiría en el sitio equivocado, y eso no puede fallar mudo.
 */
static const char* sin_prefijo(const char* path) {
    if (!path) return NULL;
    size_t n = strlen(s_prefijo);
    if (strncmp(path, s_prefijo, n) != 0) return NULL;
    if (path[n] == '\0') return "";              /* la raíz del montaje */
    if (path[n] != '/')  return NULL;
    const char* r = path + n + 1;
    return r;                                    /* relativo: FatFs lo admite */
}

/* ─────────────────────────────────────────────────────────────────────────
 * Las operaciones de la fachada. TODAS entran y salen por el lock.
 * ───────────────────────────────────────────────────────────────────────── */

static int fat_stat(const char* path, uint32_t* size) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado) return -1;
    FILINFO fi;
    trabar();
    FRESULT r = (p[0] == '\0') ? FR_INVALID_NAME : f_stat(p, &fi);
    destrabar();
    if (r != FR_OK) return -1;
    if (size) *size = (uint32_t) fi.fsize;
    return 0;
}

static long fat_read(const char* path, uint8_t* dst, uint32_t cap) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado) return -1;
    FIL f; UINT leidos = 0;
    trabar();
    FRESULT r = f_open(&f, p, FA_READ);
    if (r == FR_OK) { r = f_read(&f, dst, cap, &leidos); f_close(&f); }
    destrabar();
    return (r == FR_OK) ? (long) leidos : -1;
}

static long fat_read_at(const char* path, uint32_t off, uint8_t* dst, uint32_t cap) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado) return -1;
    FIL f; UINT leidos = 0;
    trabar();
    FRESULT r = f_open(&f, p, FA_READ);
    if (r == FR_OK) {
        r = f_lseek(&f, off);
        /* Más allá del final NO es un error: son cero bytes, que es como la
         * fachada dice "se acabó" (#305, la lectura por trozos cuenta con ello). */
        if (r == FR_OK && f_tell(&f) == off) r = f_read(&f, dst, cap, &leidos);
        else if (r == FR_OK) leidos = 0;
        f_close(&f);
    }
    destrabar();
    return (r == FR_OK) ? (long) leidos : -1;
}

/* V5/H2 — escritura POSICIONAL. `FA_OPEN_ALWAYS` (abre o crea, sin truncar) es
 * la elección que la hace usable para una base de datos: crea el fichero la
 * primera vez y a partir de ahí sólo pisa la página que le toca. */
static long fat_write_at(const char* path, uint32_t off,
                         const uint8_t* data, uint32_t len) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado || p[0] == '\0') return -1;
    FIL f; UINT esc = 0;
    trabar();
    FRESULT r = f_open(&f, p, FA_WRITE | FA_OPEN_ALWAYS);
    if (r == FR_OK) {
        /* Más allá del final, `f_lseek` EXTIENDE el fichero. Ahí está el hueco
         * de contenido indefinido que anuncia el contrato: FatFs deja lo que
         * hubiera en los clústeres. */
        r = f_lseek(&f, off);
        if (r == FR_OK && f_tell(&f) != off) r = FR_DISK_ERR;   /* no llegó */
        if (r == FR_OK) {
            r = f_write(&f, data, len, &esc);
            if (r == FR_OK && esc != len) r = FR_DISK_ERR;      /* disco lleno */
        }
        FRESULT rc = f_close(&f);        /* el close es donde se vuelca */
        if (r == FR_OK) r = rc;
    }
    destrabar();
    return (r == FR_OK) ? (long) esc : -1;
}

/* Fija el tamaño EXACTO. `f_truncate` de FatFs corta por donde esté el puntero,
 * así que el lseek no es preparación: es quien decide dónde. Y como el lseek
 * también extiende, la misma pareja sirve para encoger y para agrandar. */
static int fat_truncate(const char* path, uint32_t size) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado || p[0] == '\0') return -1;
    FIL f;
    trabar();
    FRESULT r = f_open(&f, p, FA_WRITE | FA_OPEN_ALWAYS);
    if (r == FR_OK) {
        r = f_lseek(&f, size);
        if (r == FR_OK && f_tell(&f) != size) r = FR_DISK_ERR;
        if (r == FR_OK) r = f_truncate(&f);
        FRESULT rc = f_close(&f);
        if (r == FR_OK) r = rc;
    }
    destrabar();
    return (r == FR_OK) ? 0 : -1;
}

static int fat_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado) return -1;
    FIL f; UINT esc = 0;
    trabar();
    FRESULT r = f_open(&f, p, append ? (FA_WRITE | FA_OPEN_APPEND)
                                     : (FA_WRITE | FA_CREATE_ALWAYS));
    if (r == FR_OK) {
        r = f_write(&f, data, len, &esc);
        if (r == FR_OK && esc != len) r = FR_DISK_ERR;   /* disco lleno */
        FRESULT rc = f_close(&f);                        /* el close SÍ importa:
                                                          * es donde se vuelca */
        if (r == FR_OK) r = rc;
    }
    destrabar();
    return (r == FR_OK) ? 0 : -1;
}

/* `exists` no está en el backend a propósito: la fachada lo resuelve con
 * `stat` (fs_facade.c:174), y tener aquí una segunda respuesta a la misma
 * pregunta es justo el sitio donde las dos acaban discrepando. */

static int fat_remove(const char* path) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado || p[0] == '\0') return -1;
    trabar(); FRESULT r = f_unlink(p); destrabar();
    return (r == FR_OK) ? 0 : -1;
}

static int fat_rename(const char* from, const char* to) {
    const char* a = sin_prefijo(from);
    if (!a || !s_montado) return -1;
    /* `to` puede venir sin prefijo si el llamante ya lo quitó; se admiten los
     * dos, pero un `to` de OTRO montaje se rechaza: mover entre volúmenes no es
     * renombrar, y hacerlo a medias sería peor que negarse. */
    const char* b = sin_prefijo(to);
    if (!b) { if (to && to[0] == '/') return -1; b = to; }
    if (!b || !a[0] || !b[0]) return -1;
    trabar();
    f_unlink(b);                       /* semántica REPLACE_EXISTING de la VM */
    FRESULT r = f_rename(a, b);
    destrabar();
    return (r == FR_OK) ? 0 : -1;
}

static int fat_mkdir(const char* path) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado || p[0] == '\0') return -1;
    trabar(); FRESULT r = f_mkdir(p); destrabar();
    /* "ya existe" es éxito: lo dice el contrato de la fachada. */
    return (r == FR_OK || r == FR_EXIST) ? 0 : -1;
}

static int fat_rmdir(const char* path) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado || p[0] == '\0') return -1;
    trabar(); FRESULT r = f_unlink(p); destrabar();   /* f_unlink borra dir vacío */
    return (r == FR_OK) ? 0 : -1;
}

static int fat_isdir(const char* path) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado) return 0;
    if (p[0] == '\0') return 1;                        /* la raíz es directorio */
    FILINFO fi;
    trabar(); FRESULT r = f_stat(p, &fi); destrabar();
    return (r == FR_OK && (fi.fattrib & AM_DIR)) ? 1 : 0;
}

static long long fat_mtime_ms(const char* path) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado || p[0] == '\0') return -1;
    FILINFO fi;
    trabar(); FRESULT r = f_stat(p, &fi); destrabar();
    if (r != FR_OK) return -1;
    /* De vuelta: FAT guarda año-1980, y la VM quiere epoch en ms. */
    int y = 1980 + ((fi.fdate >> 9) & 0x7F);
    int m = (fi.fdate >> 5) & 0x0F, d = fi.fdate & 0x1F;
    int hh = (fi.ftime >> 11) & 0x1F, mm = (fi.ftime >> 5) & 0x3F;
    int ss = (fi.ftime & 0x1F) * 2;
    if (m < 1) m = 1;
    if (d < 1) d = 1;
    /* días desde 1970 (Hinnant, al revés que arriba) */
    int yy = y - (m <= 2);
    int era = (yy >= 0 ? yy : yy - 399) / 400;
    int yoe = yy - era * 400;
    int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    int doe = yoe * 365 + yoe/4 - yoe/100 + doy;
    long long dias = (long long) era * 146097 + doe - 719468;
    return ((dias * 86400LL) + hh * 3600LL + mm * 60LL + ss) * 1000LL;
}

static int fat_list(const char* path,
                    void (*cb)(const char* name, int is_dir, uint32_t size, void* user),
                    void* user) {
    const char* p = sin_prefijo(path);
    if (!p || !s_montado || !cb) return -1;
    DIR dir; FILINFO fi;
    trabar();
    FRESULT r = f_opendir(&dir, p[0] ? p : "");
    if (r == FR_OK) {
        for (;;) {
            r = f_readdir(&dir, &fi);
            if (r != FR_OK || fi.fname[0] == '\0') break;   /* '' = se acabó */
            /* FatFs NO devuelve "." ni ".."; no hay que filtrarlos. */
            cb(fi.fname, (fi.fattrib & AM_DIR) ? 1 : 0, (uint32_t) fi.fsize, user);
        }
        f_closedir(&dir);
    }
    destrabar();
    return (r == FR_OK) ? 0 : -1;
}

static const bpvm_fs_backend_t s_backend = {
    .stat     = fat_stat,
    .read     = fat_read,
    .write    = fat_write,
    .remove   = fat_remove,
    .rename   = fat_rename,
    .mkdir    = fat_mkdir,
    .rmdir    = fat_rmdir,
    .copy     = NULL,          /* la fachada lo hará con read+write si hace falta */
    .isdir    = fat_isdir,
    .mtime_ms = fat_mtime_ms,
    .list     = fat_list,
    .read_at  = fat_read_at,
    .write_at = fat_write_at,      /* V5/H2: sin esto no hay base de datos */
    .truncate = fat_truncate,
};

/* ─────────────────────────────────────────────────────────────────────────
 * Montaje
 * ───────────────────────────────────────────────────────────────────────── */

/* El motivo, copiado con el tope respetado en UN sitio. Lo que había antes eran
 * tres `memcpy` con la longitud contada a mano: el día que alguien retoca el
 * texto y no vuelve a contar, se copia de más o se queda sin NUL. */
static void di_motivo(char* motivo, unsigned cap, const char* texto) {
    if (!motivo || !cap) return;
    unsigned i = 0;
    while (texto[i] && i + 1 < cap) { motivo[i] = texto[i]; i++; }
    motivo[i] = '\0';
}

int bpvm_fs_fat_montar(const bpvm_blk_backend_t* blk, const char* prefijo,
                       char* motivo, unsigned motivo_cap)
{
    if (motivo && motivo_cap) motivo[0] = '\0';
    if (!blk || !blk->init || !blk->leer || !blk->bloques) return -1;
    /* Sea quien sea el que llame —el arranque, `sd mount` o la vigilancia—,
     * esto cuenta como el intento de ESTA inserción: si sale mal, el vigilante
     * no lo repite hasta que la tarjeta salga y vuelva a entrar. */
    s_intentada = 1;
    s_blk = blk;
    if (prefijo && prefijo[0] == '/' && strlen(prefijo) < sizeof s_prefijo) {
        memcpy(s_prefijo, prefijo, strlen(prefijo) + 1);
    }

    /* 1 — el medio. Si esto falla, su `motivo()` ya dice dónde se paró. */
    if (s_blk->init() != 0) {
        di_motivo(motivo, motivo_cap,
                  s_blk->motivo ? s_blk->motivo() : "el dispositivo no arranca");
        return -1;
    }

    /* 2 — ¿dónde empieza el sistema de ficheros? Una SD trae MBR y el FS NO
     *     está en el bloque 0. Se lee la tabla y se coge la primera partición.
     *     Si NO hubiera MBR (tarjeta "superfloppy", sin particionar), el FS
     *     empieza en el 0 y el desplazamiento es cero — los dos casos valen.
     *
     *     La decodificación está en `bpvm_blk_lba0_de_mbr`, fuera de aquí y
     *     pura: es el sitio con casos raros y ahora se prueba en el PC
     *     (`make test-blk`) en vez de sólo con una tarjeta en la mano. */
    /* ESTÁTICO, no de pila: esto lo llama la comm task, y medio kilo de buffer
     * en su pila no cabe (el mismo motivo por el que handle_sd_info lo hace
     * así). Un desbordamiento de pila aquí no se ve: se manifiesta más tarde y
     * en otro sitio. */
    static uint8_t sec0[BPVM_BLK_TAM];
    if (s_blk->leer(0, 1, sec0) != 0) {
        di_motivo(motivo, motivo_cap, "no se puede leer el sector 0");
        return -1;
    }
    s_lba0 = bpvm_blk_lba0_de_mbr(sec0);

    /* 3 — el lock ANTES de tocar FatFs: `f_mount` ya es una operación. */
    if (!s_lock_listo) {
        if (bpvm_platform_mutex_init(&s_lock) != 0) {
            di_motivo(motivo, motivo_cap, "no hay mutex");
            return -1;
        }
        s_lock_listo = 1;
    }

    s_montado = 1;                 /* disk_status necesita verlo ya */
    trabar();
    FRESULT r = f_mount(&s_fatfs, "", 1 /* montar YA, no perezoso */);
    destrabar();
    if (r != FR_OK) {
        s_montado = 0;
        /* El motivo más probable con diferencia, y el que manda a la decisión
         * de H2: la tarjeta viene en exFAT y no lo compilamos. */
        di_motivo(motivo, motivo_cap,
                  (r == FR_NO_FILESYSTEM)
                      ? "no hay FAT32 en la particion (exFAT? reformatea a FAT32)"
                      : "f_mount fallo");
        return -1;
    }

    return bpvm_fs_mount(s_prefijo, &s_backend);
}

void bpvm_fs_fat_desmontar(void) {
    if (!s_montado) return;
    trabar();
    /* `f_mount(NULL, ...)` desregistra el volumen en FatFs. Puede fallar si la
     * tarjeta ya no responde —que es justo el caso normal aquí— y da igual: lo
     * que importa es que a partir de `s_montado = 0` nadie vuelve a tocarla. */
    (void) f_mount(NULL, "", 0);
    s_montado = 0;
    s_lba0    = 0;
    destrabar();
    /* El montaje SIGUE en la tabla de la fachada (no hay forma de quitarlo, ni
     * hace falta): todas las ops miran `s_montado` y devuelven -1. O sea que
     * /sd existe y falla, que es exactamente lo que es una tarjeta sacada. */
}

int bpvm_fs_fat_vigilar(const bpvm_blk_backend_t* blk, const char* prefijo,
                        char* motivo, unsigned motivo_cap)
{
    if (motivo && motivo_cap) motivo[0] = '\0';
    if (!blk || !blk->hay_medio) return 0;

    int hay = blk->hay_medio();

    if (!hay) {
        /* Fuera. Si estaba montada, soltarla; y en cualquier caso rearmar el
         * intento, porque el zócalo vacío es lo que separa una inserción de la
         * siguiente. */
        s_intentada = 0;
        if (s_montado) {
            bpvm_fs_fat_desmontar();
            di_motivo(motivo, motivo_cap, "tarjeta RETIRADA — /sd desmontado");
            return 1;
        }
        return 0;
    }

    if (s_montado) return 0;            /* metida y montada: nada que hacer */

    /* Metida y sin montar. Se intenta UNA vez por inserción: si falla (viene en
     * exFAT, o está mal metida), reintentarlo cada vuelta llenaría el log y
     * machacaría el bus para volver a fallar igual. El flag se limpia arriba,
     * cuando el zócalo se queda vacío. */
    if (s_intentada) return 0;
    s_intentada = 1;
    if (bpvm_fs_fat_montar(blk, prefijo, motivo, motivo_cap) == 0) {
        di_motivo(motivo, motivo_cap, "tarjeta METIDA — /sd montado");
    }
    return 1;
}

uint32_t bpvm_fs_fat_lba_particion(void) { return s_lba0; }
int      bpvm_fs_fat_montado(void)       { return s_montado; }

int bpvm_fs_fat_resumen(bpvm_fs_fat_resumen_t* r) {
    if (!r || !s_montado) return -1;
    memset(r, 0, sizeof *r);

    trabar();
    /* La etiqueta puede no estar (no es obligatoria) — que falle no invalida
     * el resto, así que se ignora su resultado a propósito. */
    if (f_getlabel("", r->etiqueta, NULL) != FR_OK) r->etiqueta[0] = '\0';

    DWORD libres_cl = 0;
    FATFS* fs = NULL;
    FRESULT rc = f_getfree("", &libres_cl, &fs);
    if (rc == FR_OK && fs) {
        /* Cuentas en KB, no en bytes: una tarjeta de 128 GB desborda un
         * uint32 contado en bytes, y el desbordamiento no avisa. */
        DWORD kb_por_cl = ((DWORD) fs->csize * FF_MAX_SS) / 1024u;
        DWORD total_cl  = fs->n_fatent - 2;
        r->kb_total  = (uint32_t) (total_cl  * kb_por_cl);
        r->kb_libres = (uint32_t) (libres_cl * kb_por_cl);
    }

    /* El recorrido de la raíz. `DIR`+`FILINFO` son ~330 B de pila juntos, que
     * es lo mismo que ya cuesta un `stat` — no hace falta sacarlos fuera. */
    DIR d;
    FILINFO fi;
    FRESULT rd = f_opendir(&d, "");
    if (rd == FR_OK) {
        while (f_readdir(&d, &fi) == FR_OK && fi.fname[0]) {
            if (r->entradas_raiz == 0) {
                unsigned i = 0;
                while (fi.fname[i] && i + 1 < sizeof r->primera) {
                    r->primera[i] = fi.fname[i]; i++;
                }
                r->primera[i] = '\0';
            }
            r->entradas_raiz++;
        }
        f_closedir(&d);
    }
    destrabar();

    return (rc == FR_OK && rd == FR_OK) ? 0 : -1;
}
