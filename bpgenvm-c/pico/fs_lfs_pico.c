/*
 * fs_lfs_pico.c — H2·B2 + H9: littlefs en el RP2350 (Metro 16M + Pico 2 4M,
 * MISMO .uf2). DOS piezas:
 *
 * 1) CINTURA block-device de littlefs (§2.3 del plan): read por XIP
 *    (memcpy desde XIP_BASE), prog/erase por flash_range_* BAJO
 *    bpvm_flash_lock (el guard XIP de #153: IRQs off en single-core;
 *    parquea el otro core en dual-core). El quirk se absorbe AQUÍ.
 *
 * 2) SHIM del API legado fs.h SOBRE la fachada bpvm_fs (retirarlo = #305).
 *    - fs_get: read a un scratch estático (contrato documentado del API:
 *      "puntero válido hasta el siguiente fs_put/fs_delete").
 *    - fs_put: crea los dirs padre (el FS viejo era plano; littlefs no).
 *    - fs_list: paseo BFS que emite paths PLANOS como el FS viejo.
 *    - fs_save_to_flash: no-op (littlefs committea en cada close).
 *    - fs_format_ram: reformateo EN CALIENTE + recrear /sys /lib /app.
 *
 * H9 (unificación, 19-jul — norma de Eduardo: "si no hay partición, nada
 * con el sistema de ficheros"): el DESCRIPTOR binario propio (bp_ptable_t
 * en 0x3FF000, B2.b) MURIÓ, y con él su auto-init. La región del FS ya no
 * se decide aquí: viene del ENV (bpvm_part: tamaños `part.*.size`, offsets
 * DERIVADOS desde BP_PART_BASE) y la pasa el boot a fs_init_at(). Sin
 * particiones definidas no se monta nada — el boot se queda en estado 1,
 * reporta por STATE, y el host conduce (FrmBoard: defaults → apply →
 * reinicio). El clamp #292 vive en bpvm_part_usable_flash (el llamador).
 * El log ya no acota el volumen: se mudó a la zona 2 (flash_layout.h) y
 * el espacio [BP_PART_BASE, usable) es TODO de las particiones.
 */
#include "fs.h"
#include "bpvm_fs.h"
#include "bpvm.h"          /* #338: la zona de rascar compartida */
#include "bpvm_fs_lfs.h"
#include "flash_lock.h"
#include "log.h"            /* log_printf: al log PERSISTENTE (printf = solo consola USB) */

#include "pico/stdlib.h"
#include "hardware/flash.h"

#include <string.h>
#include <stdio.h>

/* ───────────────── región del FS (viene del env vía fs_init_at) ───────
 * H9: el volumen littlefs vive en la partición FS que definió el usuario
 * (env → bpvm_part → offset derivado). Nada se auto-decide aquí. */

static uint32_t s_fs_offset = 0;   /* offset absoluto en flash del volumen */
static uint32_t s_fs_size   = 0;   /* bytes (múltiplo de sector) */

/* ───────────────────────── 1) cintura block-device ──────────────────── */

static int pico_bd_read(const struct lfs_config* c, lfs_block_t block,
                        lfs_off_t off, void* buffer, lfs_size_t size) {
    (void) c;
    const uint8_t* src = (const uint8_t*)(XIP_BASE + s_fs_offset
                                          + block * FLASH_SECTOR_SIZE + off);
    memcpy(buffer, src, size);
    return 0;
}

static int pico_bd_prog(const struct lfs_config* c, lfs_block_t block,
                        lfs_off_t off, const void* buffer, lfs_size_t size) {
    (void) c;
    uint32_t tok = bpvm_flash_lock_begin();
    flash_range_program(s_fs_offset + block * FLASH_SECTOR_SIZE + off,
                        buffer, size);
    bpvm_flash_lock_end(tok);
    return 0;
}

static int pico_bd_erase(const struct lfs_config* c, lfs_block_t block) {
    (void) c;
    uint32_t tok = bpvm_flash_lock_begin();
    flash_range_erase(s_fs_offset + block * FLASH_SECTOR_SIZE,
                      FLASH_SECTOR_SIZE);
    bpvm_flash_lock_end(tok);
    return 0;
}

static int pico_bd_sync(const struct lfs_config* c) { (void) c; return 0; }

/* buffers ESTÁTICOS de littlefs (cero malloc; los de fichero viven en
 * fs_lfs.c). cache_size DEBE ser BPVM_FS_LFS_CACHE (attach lo valida). */
static uint8_t s_read_buf[256];
static uint8_t s_prog_buf[256];
static uint8_t s_look_buf[64] __attribute__((aligned(8)));

static struct lfs_config s_cfg;   /* debe sobrevivir (littlefs guarda el ptr) */

/* ───────────────────────── 3) shim del API legado fs.h ──────────────── */

/* H11 — AQUÍ VIVÍAN 128 KB. El shim `fs_get` devolvía un puntero "válido hasta
 * el siguiente put/delete", y sostener ese contrato costaba un scratch estático
 * del tamaño del fichero más grande imaginable — el mismo presupuesto que el
 * mirror del fs.c viejo, sólo que mudado de sitio. Retirado: quien necesitaba
 * los bytes era el loader de módulos, y ahora los lee POR TROZOS directamente a
 * su sitio final (bpvm_load_mod_stream); el overlay .mdn, que sí necesita el
 * blob residente porque ejecuta en sitio, se lo pide a la arena de la VM
 * (bpvm_arena_reserve) y pide lo que ocupa de verdad. */

fs_status_t fs_init_at(uint32_t fs_offset, uint32_t fs_size) {
    /* La región viene VALIDADA por bpvm_part (alineada, no-cero, cabe en la
     * flash usable con el clamp #292). Aquí solo se monta. */
    s_fs_offset = fs_offset;
    s_fs_size   = fs_size;
    log_printf("fs: littlefs off=0x%06X size=%uKB (region del env)",
               (unsigned) s_fs_offset, (unsigned)(s_fs_size >> 10));

    memset(&s_cfg, 0, sizeof(s_cfg));
    s_cfg.read  = pico_bd_read;
    s_cfg.prog  = pico_bd_prog;
    s_cfg.erase = pico_bd_erase;
    s_cfg.sync  = pico_bd_sync;
    s_cfg.read_size      = 256;
    s_cfg.prog_size      = 256;                 /* página de flash */
    s_cfg.block_size     = FLASH_SECTOR_SIZE;   /* 4096 */
    s_cfg.block_count    = s_fs_size / FLASH_SECTOR_SIZE;
    s_cfg.cache_size     = 256;                 /* == BPVM_FS_LFS_CACHE */
    s_cfg.lookahead_size = 64;
    s_cfg.block_cycles   = 500;
    s_cfg.read_buffer      = s_read_buf;
    s_cfg.prog_buffer      = s_prog_buf;
    s_cfg.lookahead_buffer = s_look_buf;

    if (bpvm_fs_lfs_attach(&s_cfg, 1) != 0) {   /* formatea si no monta */
        log_printf("fs: ERROR littlefs no monta ni formatea");
        return FS_ERR_BAD_FLASH;
    }
    bpvm_fs_mkdir("/sys");
    bpvm_fs_mkdir("/lib");
    bpvm_fs_mkdir("/app");
    return FS_OK;
}

void fs_format_ram(void) {
    /* legado: "vaciar". Con littlefs = reformateo EN CALIENTE del volumen. */
    if (bpvm_fs_lfs_format() == 0) {
        bpvm_fs_mkdir("/sys");
        bpvm_fs_mkdir("/lib");
        bpvm_fs_mkdir("/app");
    }
}

fs_status_t fs_save_to_flash(void) {
    /* littlefs committea en cada close (durabilidad-por-llamada, B1) —
     * el SAVE del wire queda como no-op de compatibilidad. */
    return FS_OK;
}

/* #305 — existencia sin leer nada. El fs_get de abajo ya empieza por este mismo
 * stat; la diferencia es que aquí no seguimos leyendo el fichero al scratch. */
int fs_exists(const char* name) {
    uint32_t sz = 0;
    return bpvm_fs_stat(name, &sz) == 0 ? 1 : 0;
}


/* fs_put crea los dirs padre: el FS viejo era PLANO ("/lib/x" era un nombre
 * con barras); en littlefs /lib debe existir antes de escribir /lib/x. */
static void ensure_parent_dirs(const char* name) {
    const char* last = strrchr(name, '/');
    if (!last || last == name) return;          /* raíz o sin dir */
    char dir[128];
    size_t n = (size_t)(last - name);
    if (n >= sizeof(dir)) return;
    memcpy(dir, name, n);
    dir[n] = '\0';
    bpvm_fs_mkdir(dir);                          /* recursivo, ok-si-existe */
}

/* #329 — que el fallo de escritura NO MIENTA (la 3ª y última familia; ESP32 y
 * STM32 ya lo tienen). La fachada bpvm_fs colapsa todo a 0/-1 por ser
 * backend-agnóstica, y aquí ese -1 se traducía a "FS lleno" pasara lo que
 * pasara. En el S3 (#328) ese mensaje falso mandó la depuración al sitio
 * equivocado una tarde entera: el volumen estaba al 1% y el error decía que
 * estaba lleno. Ahora se reporta el lfs_error REAL y queda en el log
 * persistente, que sobrevive al reset y se saca con LOG_DUMP. */
static fs_status_t map_lfs_err(const char* op, const char* name, uint32_t size) {
    int e = bpvm_fs_lfs_last_err();
    uint32_t tot = fs_total_bytes(), usa = fs_used_bytes();
    log_printf("fs: %s '%s' (%lu B) FALLO lfs=%d %s  [libre %lu/%lu B]",
               op, name, (unsigned long) size, e, bpvm_fs_lfs_err_str(e),
               (unsigned long) ((tot > usa) ? (tot - usa) : 0u), (unsigned long) tot);
    switch (e) {
        case LFS_ERR_NOSPC:        return FS_ERR_NO_SPACE;
        case LFS_ERR_FBIG:         return FS_ERR_TOO_BIG;
        case LFS_ERR_NAMETOOLONG:  return FS_ERR_NAME_TOO_LONG;
        case LFS_ERR_EXIST:        return FS_ERR_EXISTS;
        case LFS_ERR_NOENT:        return FS_ERR_NOT_FOUND;
        case LFS_ERR_IO:
        case LFS_ERR_CORRUPT:      return FS_ERR_BAD_FLASH;
        default:                   return FS_ERR_INVALID;
    }
}

fs_status_t fs_put(const char* name, const uint8_t* data, uint32_t size) {
    if (strlen(name) >= 128) return FS_ERR_NAME_TOO_LONG;
    ensure_parent_dirs(name);
    if (bpvm_fs_write(name, data, size, 0) != 0) return map_lfs_err("put", name, size);
    return FS_OK;
}

/* #294 streaming PUT — apende un trozo (el PUT_BEGIN del wire crea/trunca con
 * fs_put(name,NULL,0)). Sube ficheros > buffer del wire sin buferizarlos enteros. */
fs_status_t fs_put_append(const char* name, const uint8_t* data, uint32_t size) {
    if (strlen(name) >= 128) return FS_ERR_NAME_TOO_LONG;
    if (size == 0) return FS_OK;
    if (bpvm_fs_write(name, data, size, 1) != 0) return map_lfs_err("append", name, size);
    return FS_OK;
}

fs_status_t fs_delete(const char* name) {
    if (!bpvm_fs_exists(name)) return FS_ERR_NOT_FOUND;
    return (bpvm_fs_remove(name) == 0) ? FS_OK : FS_ERR_INVALID;
}

/* ── fs_list: BFS emitiendo paths PLANOS como el FS viejo ──────────────
 * El cb de la fachada corre BAJO el lock → SOLO acumula; el descenso a
 * subdirs y la emisión al cliente pasan FUERA del callback. */
/* EFECTO VENTANA (Eduardo, 28-jul) — el Pico ya EMITE el listado según lo
 * recorre, pero el snapshot POR DIRECTORIO se cortaba a 32 EN SILENCIO. Con la
 * stdlib completa (~47 módulos en /lib) muerde: el IDE mostraría 32 y el resto
 * simplemente "no estaría". Mismo arreglo que en ESP32 y STM32: sube a 96 y
 * DEJA DE SER MUDO. El snapshot NO se puede quitar (el cb corre bajo el lock). */
#define LIST_MAX_DIRS     16
#define LIST_MAX_ENTRIES  96
#define LIST_NAME_MAX     64

typedef struct {
    char     names[LIST_MAX_ENTRIES][LIST_NAME_MAX];
    uint32_t sizes[LIST_MAX_ENTRIES];
    uint8_t  isdir[LIST_MAX_ENTRIES];
    int      n;
    int      overflow;   /* 1 si este directorio tiene MÁS de los que caben */
} dir_snapshot_t;

/* #425 — las que NO cupieron en el ÚLTIMO recorrido, para que el wire lo diga
 * (ver fs_list_omitidas en fs.h). Se pone a cero al empezar fs_list. */
static int s_list_omitidas = 0;
int fs_list_omitidas(void) { return s_list_omitidas; }

static void snap_cb(const char* name, int is_dir, uint32_t size, void* user) {
    dir_snapshot_t* s = (dir_snapshot_t*) user;
    if (s->n >= LIST_MAX_ENTRIES) { s->overflow++; s_list_omitidas++; return; }   /* NO en silencio */
    snprintf(s->names[s->n], LIST_NAME_MAX, "%s", name);
    s->sizes[s->n] = size;
    s->isdir[s->n] = (uint8_t) is_dir;
    s->n++;
}

/* #338 — los dos buffers del listado, JUNTOS y en un solo préstamo. Iban
 * `static` (6.632 + 1.024 = 7.656 B permanentes en .bss) para no reventar el
 * stack de la comm task; ahora salen de la zona compartida y viven lo que dura
 * el LIST. Cabe de sobra en los 8 KB, y en UNA estructura se deja al compilador
 * el relleno y la alineación en vez de repartir el bloque a mano. */
typedef struct {
    dir_snapshot_t snap;
    char           pending[LIST_MAX_DIRS][LIST_NAME_MAX];
} list_work_t;

int fs_list(fs_list_cb_t cb, void* user) {
    s_list_omitidas = 0;   /* #425 */
    list_work_t* w = (list_work_t*) bpvm_scratch_take(sizeof(list_work_t), "fs_list");
    if (!w) {
        /* Sin zona NO se devuelve un listado vacío: eso se leería como "el FS
         * está vacío", que es mentira y de las caras. Se dice y se sale con
         * error. */
        log_printf("fs: LIST sin zona de scratch — listado NO realizado");
        return 1;
    }
    dir_snapshot_t* snap = &w->snap;
    int head = 0, tail = 0;
    snprintf(w->pending[tail++], LIST_NAME_MAX, "/");

    while (head < tail) {
        const char* dir = w->pending[head++];
        snap->n = 0; snap->overflow = 0;
        if (bpvm_fs_list(dir, snap_cb, snap) != 0) continue;
        if (snap->overflow) {
            log_printf("fs: LISTADO INCOMPLETO — '%s' tiene mas de %d entradas (%d fuera)",
                       dir, LIST_MAX_ENTRIES, snap->overflow);
            log_flush();
        }
        for (int i = 0; i < snap->n; i++) {
            char full[LIST_NAME_MAX];
            int is_root = (dir[1] == '\0');
            if (snap->isdir[i]) {
                if (tail < LIST_MAX_DIRS) {
                    snprintf(w->pending[tail], LIST_NAME_MAX, "%s%s%s",
                             dir, is_root ? "" : "/", snap->names[i]);
                    tail++;
                } else {
                    s_list_omitidas++;   /* #425: un directorio sin recorrer TAMBIEN falta */
                    log_printf("fs: LISTADO INCOMPLETO — mas de %d directorios; "
                               "'%s' sin recorrer", LIST_MAX_DIRS, snap->names[i]);
                    log_flush();
                }
                continue;                        /* los dirs no se emiten (legado) */
            }
            /* raíz → nombre pelado ("Hello.mod"); subdir → "/lib/Core.mod" */
            if (is_root) snprintf(full, sizeof(full), "%s", snap->names[i]);
            else         snprintf(full, sizeof(full), "%s/%s", dir, snap->names[i]);
            if (cb(full, snap->sizes[i], user) != 0) {
                /* SALIDA TEMPRANA: el callante corta el listado. Aquí es donde
                 * se olvida uno de soltar la zona — y entonces el siguiente LIST
                 * (o el siguiente PACK_DEL) se la encuentra ocupada para
                 * siempre. */
                bpvm_scratch_give("fs_list");
                return 1;
            }
        }
    }
    bpvm_scratch_give("fs_list");
    return 0;
}

/* stats para INFO del IDE / logs de boot */
uint32_t fs_total_bytes(void) {
    uint32_t t = 0;
    bpvm_fs_lfs_stats(&t, NULL);
    return t;
}

uint32_t fs_used_bytes(void) {
    uint32_t u = 0;
    bpvm_fs_lfs_stats(NULL, &u);
    return u;
}

uint32_t fs_free_bytes(void) {
    uint32_t t = 0, u = 0;
    bpvm_fs_lfs_stats(&t, &u);
    return (t > u) ? (t - u) : 0;
}

static int count_cb(const char* name, uint32_t size, void* user) {
    (void) name; (void) size;
    (*(int*) user)++;
    return 0;
}

int fs_file_count(void) {
    int n = 0;
    fs_list(count_cb, &n);
    return n;
}

const char* fs_status_str(fs_status_t s) {
    switch (s) {
        case FS_OK:                return "OK";
        case FS_ERR_NOT_FOUND:     return "no encontrado";
        case FS_ERR_EXISTS:        return "ya existe";
        case FS_ERR_NO_SPACE:      return "sin espacio";
        case FS_ERR_NAME_TOO_LONG: return "nombre demasiado largo";
        case FS_ERR_TOO_BIG:       return "fichero demasiado grande";
        case FS_ERR_TABLE_FULL:    return "tabla llena";
        case FS_ERR_BAD_FLASH:     return "flash corrupta";
        case FS_ERR_INVALID:       return "operacion invalida";
        default:                   return "error desconocido";
    }
}

void fs_register_bpvm(void) {
    /* no-op: bpvm_fs_lfs_attach (en fs_init) ya registró el backend en la
     * fachada — los builtins readFile/writeFile/... van DIRECTOS a littlefs
     * (con el lock grueso de B1.4), sin pasar por este shim. */
}
