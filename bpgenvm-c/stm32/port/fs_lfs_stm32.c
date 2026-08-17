/*
 * fs_lfs_stm32.c — H9 · Paso 1 (#297): littlefs en el STM32 (U5). REEMPLAZA al
 * FS viejo (arena RAM + snapshot) de stm32_fs.c. Dos piezas:
 *
 * 1) CINTURA block-device de littlefs sobre la flash interna del U5:
 *      - read  : lectura MAPEADA (memcpy desde FLASH_BASE + offset).
 *      - prog  : HAL_FLASH_Program en QUADWORD (16 B) — el U5 solo programa así.
 *      - erase : HAL_FLASHEx_Erase por página (8 KB), consciente de dual-bank.
 *    Con ICACHE disable/invalidate alrededor (no servir instrucciones rancias).
 *    prog/erase DELEGAN en stm32_flash.c (primitivas COMPARTIDAS con board_mgr →
 *    un solo sitio escribe la flash del U5, sin copias que diverjan).
 *
 * 2) IMPLEMENTACIÓN del API legado stm32_fs.h SOBRE la fachada bpvm_fs+littlefs
 *    (mismo shape que fs_lfs_pico.c): fs_get a un scratch (contrato "válido hasta
 *    el siguiente fs op"), fs_put crea dirs padre, fs_count/fs_entry por recorrido
 *    BFS con paths PLANOS (como el FS viejo), fs_save no-op (littlefs committea en
 *    cada close), el boot monta. stm32_fs_register_bpvm no-op (el attach ya
 *    registró el backend).
 *
 * H9: la región del FS ya NO es fija — la pasa el boot a fs_init_at(offset,size),
 * con la región que define el env (bpvm_part). Nota: multi-.mdn con el scratch único es limitación
 * LATENTE compartida con la Pico (0-1 .mdn/RUN = caso real OK).
 */
#include "stm32_fs.h"
#include "bpvm_fs.h"
#include "bpvm_fs_lfs.h"

#include "main.h"         /* CMSIS: FLASH_BASE + FLASH_PAGE_SIZE (flash mapeada) */
#include "stm32_flash.h"  /* stm32_flash_write / erase — primitivas COMPARTIDAS con board_mgr */
#include "log.h"          /* #329 — el porqué de un fallo de FS, al log persistente */
#include <string.h>
#include <stdio.h>

/* ── región del FS (la fija fs_init_at) ───────────────────────────────── */
static uint32_t s_fs_offset = 0;   /* offset del volumen DESDE FLASH_BASE */
static uint32_t s_fs_size   = 0;   /* bytes (múltiplo de página) */

#define FS_BLOCK_SIZE   FLASH_PAGE_SIZE   /* 8 KB en U5 (página de borrado) */
#define FS_CACHE        256u              /* == BPVM_FS_LFS_CACHE; múltiplo de 16 (quadword U5) */

/* ── cintura block-device de littlefs (prog/erase → stm32_flash compartido) ── */

static int stm32_bd_read(const struct lfs_config* c, lfs_block_t block,
                         lfs_off_t off, void* buffer, lfs_size_t size) {
    (void) c;
    const uint8_t* src = (const uint8_t*) (uintptr_t)
        (FLASH_BASE + s_fs_offset + (uint32_t) block * FS_BLOCK_SIZE + off);
    memcpy(buffer, src, size);
    return 0;
}

static int stm32_bd_prog(const struct lfs_config* c, lfs_block_t block,
                         lfs_off_t off, const void* buffer, lfs_size_t size) {
    (void) c;
    /* `size` viene de littlefs = múltiplo de FS_CACHE (256) → siempre 16-aligned. */
    uint32_t dst = FLASH_BASE + s_fs_offset + (uint32_t) block * FS_BLOCK_SIZE + off;
    return (stm32_flash_write(dst, (const uint8_t*) buffer, size) == 0) ? 0 : LFS_ERR_IO;
}

static int stm32_bd_erase(const struct lfs_config* c, lfs_block_t block) {
    (void) c;
    uint32_t addr = FLASH_BASE + s_fs_offset + (uint32_t) block * FS_BLOCK_SIZE;
    return (stm32_flash_erase(addr, 1u) == 0) ? 0 : LFS_ERR_IO;   /* 1 página de 8 KB */
}

static int stm32_bd_sync(const struct lfs_config* c) { (void) c; return 0; }

/* buffers ESTÁTICOS de littlefs (cero malloc; los de fichero viven en fs_lfs.c). */
static uint8_t s_read_buf[FS_CACHE];
static uint8_t s_prog_buf[FS_CACHE];
static uint8_t s_look_buf[64] __attribute__((aligned(8)));
static struct lfs_config s_cfg;   /* debe sobrevivir (littlefs guarda el ptr) */

/* H11 — AQUÍ VIVÍA `fs_get` Y SU ESPEJO, el mayor estático de todo el proyecto:
 * 496 KB en la Discovery, 96 KB en la Nucleo (el desaparecido BOARD_FS_ARENA_SIZE
 * de board.h, retirado al quedarse sin usuarios).
 *
 * El shim heredaba de stm32_fs.c un contrato imposible de cumplir barato:
 * devolver un PUNTERO a los bytes. Con littlefs los ficheros no están mapeados,
 * así que sostener ese puntero obligaba a copiar el fichero ENTERO a RAM. Se
 * dimensionó igual que la arena del FS viejo para NO mover el presupuesto
 * durante la migración a littlefs — un apaño consciente, y H11 es el momento
 * de cobrarlo.
 *
 * Todos los llamantes usan ya la fachada portable, por trozos:
 *   ¿existe?/¿tamaño? → bpvm_fs_stat · leer un cacho → bpvm_fs_read_at
 *   cargar módulo     → bpvm_load_mod_stream        · CRC → bpvm_fs_crc32
 * Y el .mdn, que se ejecuta ZERO-COPY in-place (era la razón del alineado a 8
 * y de que el espejo tuviera que ser permanente), se reserva ahora de la arena
 * de la VM con bpvm_arena_reserve: exactamente los bytes que ocupa. Mismo
 * camino que el Pico en #305. */

/* ── listado (compartido por fs_count/fs_entry y clear_lib) ──────────────
 * EFECTO VENTANA (Eduardo, 28-jul): había UN SOLO tope de 64 para las dos
 * cosas —los ficheros de un directorio y los del FS ENTERO— y al llenarse se
 * cortaba EN SILENCIO. Como el recorrido es en anchura (raíz → /app → /lib →
 * /sys), cuantos más módulos había en /app menos plazas le quedaban a /lib:
 * los ficheros seguían ahí, pero DESAPARECÍAN DEL LISTADO. Un recorte mudo
 * disfrazado de "no está".
 * Ahora son dos topes distintos, dimensionados a lo que hay de verdad
 * (la stdlib sola son ~47 módulos), y si alguno se queda corto SE DICE. */
#define LIST_MAX_ENTRIES  96    /* ficheros de UN directorio (/lib lleva la stdlib) */
#define SNAP_MAX_FILES    192   /* ficheros del FS ENTERO que puede listar el wire */
#define LIST_MAX_DIRS     16
#define LIST_NAME_MAX     64

typedef struct {
    char     names[LIST_MAX_ENTRIES][LIST_NAME_MAX];
    uint8_t  isdir[LIST_MAX_ENTRIES];
    uint32_t sizes[LIST_MAX_ENTRIES];
    int      n;
    int      overflow;   /* 1 si este directorio tiene MÁS de los que caben */
} dirlist_t;

/* El cb de la fachada corre BAJO el lock → SOLO acumula un directorio; el
 * descenso a subdirs y las mutaciones pasan FUERA del callback. */
static void dirlist_cb(const char* name, int is_dir, uint32_t size, void* user) {
    dirlist_t* d = (dirlist_t*) user;
    if (d->n >= LIST_MAX_ENTRIES) { d->overflow++; return; }   /* NO en silencio (#425: CUENTA) */
    snprintf(d->names[d->n], LIST_NAME_MAX, "%s", name);
    d->isdir[d->n] = (uint8_t) is_dir;
    d->sizes[d->n] = size;
    d->n++;
}

/* snapshot BFS de paths PLANOS (raíz → "Hello.mod"; subdir → "/lib/Core.mod"),
 * como el FS viejo. Lo rellena fs_count(); fs_entry() lo lee (el wire llama
 * fs_count y luego fs_entry en secuencia). */
/* #425 — las que NO cupieron en el ULTIMO snapshot, para que el wire lo diga
 * (ver fs_list_omitidas en fs.h). El STM32 tiene TRES topes, no dos: entradas
 * por directorio, directorios a visitar y ficheros del FS entero — y el tercero
 * cortaba con un `return` seco. Los tres suman aqui. */
static int      s_list_omitidas = 0;
int fs_list_omitidas(void) { return s_list_omitidas; }

static char     s_snap_names[SNAP_MAX_FILES][LIST_NAME_MAX];
static uint32_t s_snap_sizes[SNAP_MAX_FILES];
static int      s_snap_n = 0;

/* Los paths planos están acotados a LIST_NAME_MAX por contrato del FS (nombres
 * ≤ 64 incl. el path); snprintf trunca a salvo → silenciamos el aviso conservador
 * -Wformat-truncation (el build STM32 va con -Wall). El -Wrestrict lo evitamos
 * copiando pending[head] a un local antes de escribir en pending[tail]. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
static void rebuild_snapshot(void) {
    static char      pending[LIST_MAX_DIRS][LIST_NAME_MAX];
    static dirlist_t dl;
    int head = 0, tail = 0;
    s_snap_n = 0;
    s_list_omitidas = 0;   /* #425 */
    snprintf(pending[tail++], LIST_NAME_MAX, "/");

    while (head < tail) {
        char dir[LIST_NAME_MAX];
        snprintf(dir, sizeof(dir), "%s", pending[head++]);   /* copia local: sin aliasing con pending */
        int is_root = (dir[1] == '\0');
        dl.n = 0; dl.overflow = 0;
        if (bpvm_fs_list(dir, dirlist_cb, &dl) != 0) continue;
        if (dl.overflow) {
            s_list_omitidas += dl.overflow;   /* #425 */
            log_printf("fs: LISTADO INCOMPLETO — '%s' tiene mas de %d entradas (%d fuera)",
                       dir, LIST_MAX_ENTRIES, dl.overflow);
        }
        for (int i = 0; i < dl.n; i++) {
            if (dl.isdir[i]) {
                if (tail < LIST_MAX_DIRS)
                    snprintf(pending[tail++], LIST_NAME_MAX, "%s%s%s",
                             dir, is_root ? "" : "/", dl.names[i]);
                else {
                    s_list_omitidas++;   /* #425: un directorio sin recorrer TAMBIEN falta */
                    log_printf("fs: LISTADO INCOMPLETO — mas de %d directorios; "
                               "'%s' sin recorrer", LIST_MAX_DIRS, dl.names[i]);
                }
                continue;                        /* los dirs no se emiten (legado plano) */
            }
            if (s_snap_n >= SNAP_MAX_FILES) {
                s_list_omitidas++;   /* #425: al menos este; el `return` corta el resto */
                /* EL efecto ventana. Antes era un `return` a secas: el resto del
                 * FS —tipicamente /lib— simplemente no salia, y desde el IDE
                 * parecia que los ficheros no estaban. */
                log_printf("fs: LISTADO TRUNCADO a %d ficheros — hay MAS en el FS "
                           "(se corto en '%s')", SNAP_MAX_FILES, dir);
                log_flush();
                return;
            }
            if (is_root) snprintf(s_snap_names[s_snap_n], LIST_NAME_MAX, "%s", dl.names[i]);
            else         snprintf(s_snap_names[s_snap_n], LIST_NAME_MAX, "%s/%s", dir, dl.names[i]);
            s_snap_sizes[s_snap_n] = dl.sizes[i];
            s_snap_n++;
        }
    }
}
#pragma GCC diagnostic pop

/* Vacía /lib tras montar: el FS viejo NO persistía /lib (fs_load lo saltaba) →
 * stm32_mods_install lo re-embebe FRESCO cada boot (evita stdlib rancia tras
 * actualizar el firmware). littlefs SÍ persiste → lo limpiamos a mano para
 * conservar esa semántica (mismo desgaste que el snapshot viejo). */
static void clear_lib(void) {
    static dirlist_t dl;
    dl.n = 0;
    if (bpvm_fs_list("/lib", dirlist_cb, &dl) != 0) return;   /* no existe aún → nada */
    for (int i = 0; i < dl.n; i++) {
        if (dl.isdir[i]) continue;                            /* /lib es plano */
        char path[LIST_NAME_MAX + 8];
        snprintf(path, sizeof(path), "/lib/%s", dl.names[i]);
        bpvm_fs_remove(path);
    }
}

/* ── crear dirs padre (el FS viejo era PLANO; littlefs necesita /lib antes de /lib/x) ── */
static void ensure_parent_dirs(const char* name) {
    const char* last = strrchr(name, '/');
    if (!last || last == name) return;          /* raíz o sin dir */
    char dir[128];
    size_t n = (size_t) (last - name);
    if (n >= sizeof(dir)) return;
    memcpy(dir, name, n);
    dir[n] = '\0';
    bpvm_fs_mkdir(dir);                          /* recursivo, ok-si-existe */
}

/* ── mount / boot (H9: región parametrizada) ──────────────────────────── */

int fs_init_at(uint32_t fs_offset, uint32_t fs_size) {
    s_fs_offset = fs_offset;
    s_fs_size   = fs_size;

    memset(&s_cfg, 0, sizeof(s_cfg));
    s_cfg.read  = stm32_bd_read;
    s_cfg.prog  = stm32_bd_prog;
    s_cfg.erase = stm32_bd_erase;
    s_cfg.sync  = stm32_bd_sync;
    s_cfg.read_size      = FS_CACHE;
    s_cfg.prog_size      = FS_CACHE;
    s_cfg.block_size     = FS_BLOCK_SIZE;          /* 8 KB (página U5) */
    s_cfg.block_count    = fs_size / FS_BLOCK_SIZE;
    s_cfg.cache_size     = FS_CACHE;
    s_cfg.lookahead_size = 64;
    s_cfg.block_cycles   = 500;
    s_cfg.read_buffer      = s_read_buf;
    s_cfg.prog_buffer      = s_prog_buf;
    s_cfg.lookahead_buffer = s_look_buf;

    if (bpvm_fs_lfs_attach(&s_cfg, 1) != 0) return -1;   /* formatea si no monta */
    bpvm_fs_mkdir("/sys");
    bpvm_fs_mkdir("/lib");
    bpvm_fs_mkdir("/app");
    clear_lib();                                          /* stdlib siempre fresca del firmware */
    return 0;
}

/* H11 — AQUÍ ESTABA `fs_load()`, que montaba la región FIJA de la placa. Su
 * propio comentario decía "en el Paso 2 la fija el env", y ese paso 2 ya está
 * hecho: el arranque escalonado llama a fs_init_at con la región del env
 * (board_mgr_stm32.c). Nadie lo llamaba desde entonces, pero seguía siendo el
 * único usuario de BOARD_FS_FLASH_ADDR/_REGION_SIZE y los mantenía vivos. */

/* ── API legado stm32_fs.h sobre la fachada ───────────────────────────── */

/* #329 — que el fallo de escritura NO MIENTA (lo mismo que se hizo en el ESP32).
 * La fachada bpvm_fs colapsa todo a 0/-1 por ser backend-agnóstica, así que ese
 * -1 no dice nada y arriba se traducía a "FS lleno" pasara lo que pasara. En el
 * S3 (#328) eso mandó la depuración al sitio equivocado durante una tarde: el
 * volumen estaba al 1% y el error decía que estaba lleno. Aquí queda el código
 * REAL de littlefs en el log persistente, que sobrevive al reset. */
static void log_fallo_fs(const char* op, const char* name, uint32_t size) {
    int e = bpvm_fs_lfs_last_err();
    uint32_t tot = fs_total_bytes(), usa = fs_used_bytes();
    log_printf("fs: %s '%s' (%lu B) FALLO lfs=%d %s  [libre %lu/%lu B]",
               op, name, (unsigned long) size, e, bpvm_fs_lfs_err_str(e),
               (unsigned long) ((tot > usa) ? (tot - usa) : 0u), (unsigned long) tot);
    log_flush();
}

int fs_put(const char* name, const uint8_t* data, uint32_t size) {
    if (!name || name[0] == '\0') return -1;
    ensure_parent_dirs(name);
    if (bpvm_fs_write(name, data, size, 0) != 0) { log_fallo_fs("put", name, size); return -1; }
    return 0;
}

/* #294 streaming PUT — apende un trozo (el PUT_BEGIN del wire crea/trunca con
 * fs_put(name,NULL,0)). Sube ficheros > buffer del wire sin buferizarlos enteros. */
int fs_put_append(const char* name, const uint8_t* data, uint32_t size) {
    if (!name || name[0] == '\0') return -1;
    if (size == 0) return 0;
    if (bpvm_fs_write(name, data, size, 1) != 0) { log_fallo_fs("append", name, size); return -1; }
    return 0;
}


int fs_del(const char* name) {
    if (!bpvm_fs_exists(name)) return -1;
    return (bpvm_fs_remove(name) == 0) ? 0 : -1;
}

int fs_count(void) {
    rebuild_snapshot();
    return s_snap_n;
}

int fs_entry(int i, const char** name, uint32_t* size) {
    if (i < 0 || i >= s_snap_n) return -1;
    if (name) *name = s_snap_names[i];
    if (size) *size = s_snap_sizes[i];
    return 0;
}

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

void fs_format(void) {
    if (bpvm_fs_lfs_format() == 0) {
        bpvm_fs_mkdir("/sys");
        bpvm_fs_mkdir("/lib");
        bpvm_fs_mkdir("/app");
    }
}

void fs_save(void) {
    /* littlefs committea en cada close (durabilidad-por-llamada) → el SAVE del
     * wire queda como no-op de compatibilidad. */
}

void stm32_fs_register_bpvm(void) {
    /* no-op: bpvm_fs_lfs_attach (en fs_init_at) ya registró el backend littlefs
     * en la fachada → readFile/writeFile/... van DIRECTOS a littlefs. */
}
