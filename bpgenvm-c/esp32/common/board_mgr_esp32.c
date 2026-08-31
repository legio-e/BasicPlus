/*
 * board_mgr_esp32.c — H9: adaptador de gestión de placa de la familia ESP32.
 * Cintura del env A/B sobre la partición vendor "bpenv" (esp_partition) + arranque
 * escalonado + board_boot_status + ramo STATE/ENV_x/PART_x del wire. Ver el .h.
 *
 * Modelo de particiones (Eduardo 19-jul): la tabla vendor fija la ZONA DE DATOS
 * entera = partición "bpdata"; el LÍMITE FS|Packs vive en el env (un mando, FS es la
 * knob, Packs el resto). bpvm_part opera con base=0 (relativo a bpdata) y
 * usable=bpdata->size → el mismo núcleo que el RP2350, solo cambia de dónde salen.
 */
#include "board_mgr_esp32.h"
#include "bpvm_pack.h"   /* V5/H7: bpvm_pack_flash_t, la cintura de escritura */

#include "bpvm.h"        /* #338: bpvm_scratch_take/give (zona de rascar compartida) */
#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_part.h"
#include "bpvm_env.h"
#include "wire_v1.h"
#include "fs.h"
#include "log.h"
#if CONFIG_IDF_TARGET_ESP32P4
#include "pack_p4.h"   /* V5/H8: s_pack_ram_base — la RAM del motor nativo */
#endif          /* el resultado del climb, al log persistente */

#include "esp_partition.h"
#include "esp_flash.h"    /* #328 — el tamaño REAL del chip, no el que dice la tabla */
#include "esp_log.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define BP_ENV_SECTOR   4096u   /* sector de borrado de la flash SPI */

static const char* TAG = "board_mgr";

/* Particiones vendor (localizadas una vez en board_mgr_esp32_boot). */
static const esp_partition_t* s_bpenv  = NULL;   /* env A/B (sectores 0 y 1) */
static const esp_partition_t* s_bpdata = NULL;   /* zona de datos (FS + Packs) */

/* Estado del arranque escalonado + el env/layout del climb (estáticos: el layout
 * debe sobrevivir para que layer_fs lo use; el boot lo lee board_boot_status). */
static bpvm_boot_status_t s_boot;
static bpvm_part_layout_t s_layout;
static uint8_t s_env_a[BP_ENV_SECTOR];
static uint8_t s_env_b[BP_ENV_SECTOR];

/* #311 — el env VIVO del boot, para que lo consulte quien configure hardware
 * (hoy el panel del P4). Antes se parseaba en una local de layer_partitions y se
 * tiraba, así que la única config de placa accesible luego era /sys/board.json —
 * un fichero DENTRO del FS, que se pierde al formatear y depende de una capa que
 * arranca después. El env está en su partición, sobrevive al reflasheo y ya está
 * disponible en el estado 2. `payload` apunta dentro de s_env_a/s_env_b, que son
 * estáticos y sólo se tocan aquí en el boot (el ENV_SET del wire trabaja sobre el
 * scratch prestado) ⇒ esta vista sigue válida toda la sesión. Corolario: un
 * ENV_SET NO cambia el hardware ya configurado; surte efecto al reiniciar. */
static bpvm_env_t s_env;

/* La memoria de la VM la definen los main.c (S3 array SRAM / P4 puntero PSRAM). */
extern uint8_t* s_vm_buffer;
extern uint32_t s_vm_buffer_size;

const bpvm_boot_status_t* board_boot_status(void) { return &s_boot; }

const bpvm_env_t* board_mgr_env(void) { return &s_env; }

/*
 * ─── V5/H7 — LA ZONA DE PACKS, POR FIN CABLEADA EN ESTA FAMILIA ──────────────
 *
 * Hasta hoy `PACK_LS` contestaba "sin zona de packs" en los DOS ESP32, y no era
 * un fallo de configuración de nadie: `bm.packs_*` no se rellenaba. Los verbos
 * PACK_* estaban encaminados pero sin nada detrás — la media función de
 * [[portar-familia-adaptador-completo]], que ya mordió en el Pico (#327) y que
 * su propio comentario describe: *"encaminar los verbos y no rellenar su
 * petición es la misma media función que dejar bm.packs_* a cero"*. Aquí
 * faltaban LAS DOS mitades.
 *
 * ─── POR QUÉ LA VISTA SE REGISTRA Y NO SE CALCULA AQUÍ ───
 *
 * Leer la zona necesita un PUNTERO que la CPU pueda seguir, y ahí las dos
 * familias no se parecen: el P4 tiene que MAPEARLA (`esp_partition_mmap`, y la
 * dirección la asigna la MMU en runtime), mientras que el S3 hará lo suyo
 * cuando le toque. Calcularlo aquí obligaría a este fichero —que es común— a
 * saberse el mapeo de cada micro.
 *
 * Así que la familia la REGISTRA cuando la tiene. Y el que no la registre se
 * queda como estaba: `packs_base` a NULL y `PACK_LS` diciendo "sin zona de
 * packs", que para el S3 hoy es LA VERDAD. Un `#ifdef` habría dado el mismo
 * resultado escondiendo que una familia está a medias; esto lo deja a la vista.
 */
static const uint8_t* s_packs_view      = 0;
static uint32_t       s_packs_view_size = 0;

void board_mgr_esp32_set_packs_view(const void* base, uint32_t size) {
    s_packs_view      = (const uint8_t*) base;
    s_packs_view_size = size;

    /* ─── Y MONTARLA, que son DOS consumidores, no uno ────────────────────
     *
     * Con la vista registrada, el IDE ya puede listar y grabar. Pero la VM
     * busca los módulos (`mod`) y los puentes (`mdn`) del pack por otro sitio
     * —`bpvm_pack_mounted()`—, y si nadie ha montado, ahí no hay nada que
     * encontrar. Son la misma dirección para dos usos distintos.
     *
     * ⚠️ Esto FALTABA en la familia ESP32 y no era una regresión: el camino
     * «cargar un módulo desde la zona» no se había ejercitado nunca aquí. El
     * pack nativo de V5/H8 sí corría, porque a ése se le encuentra BARRIENDO
     * la zona, que no necesita montaje. Se vio con el caso mínimo de Eduardo
     * (13-ago): un pack con UN módulo dentro y `falta el modulo 'mod1'`,
     * mientras el mismo pack en host iba. En la Pico ya se había cazado igual
     * — ver el aviso gemelo en `pico/pack_pico.c`.
     *
     * Va AQUÍ y no en `pack_p4.c` a propósito: éste es el punto por el que
     * pasa cualquier familia que consiga su puntero, así que el S3 lo tendrá
     * el día que registre el suyo sin que nadie se acuerde de esta línea.
     *
     * Y va pase lo que pase con el código nativo: un pack de sólo módulos,
     * sin `npk`, tiene que valer igual. */
    bpvm_pack_mount(s_packs_view, s_packs_view_size);
    log_printf("pack: zona montada en %p (%u KB) — modulos y .mdn visibles",
               (const void*) s_packs_view, (unsigned) (s_packs_view_size / 1024u));
}

/* ── Cintura de ESCRITURA. Ésta SÍ es común a los dos ESP32: los dos graban por
 *    `esp_partition_*`, así que no hay nada que separar por familia.
 *
 * Los offsets que llegan son RELATIVOS a la región de packs; aquí se suman al
 * offset de la región dentro de `bpdata`, que es la partición real. Esa suma es
 * justo lo que la cintura existe para hacer: el núcleo de packs no sabe —ni
 * tiene por qué— dónde empieza la región. */
static uint32_t packs_off_en_bpdata(void) {
    const bpvm_part_t* pp = bpvm_part_get(&s_layout, BPVM_PART_PACKS);
    return pp ? pp->offset : 0u;
}

static int packs_fl_erase(void* user, uint32_t off, uint32_t len) {
    (void) user;
    if (!s_bpdata) return -1;
    return esp_partition_erase_range(s_bpdata, packs_off_en_bpdata() + off, len)
           == ESP_OK ? 0 : -1;
}

static int packs_fl_program(void* user, uint32_t off, const uint8_t* d, uint32_t len) {
    (void) user;
    if (!s_bpdata) return -1;
    return esp_partition_write(s_bpdata, packs_off_en_bpdata() + off, d, len)
           == ESP_OK ? 0 : -1;
}

static const bpvm_pack_flash_t s_packs_fl = {
    packs_fl_erase, packs_fl_program, 0,
    4096u    /* granularidad de borrado del ESP32 */
};

/* V5/H7 — dónde está la zona de packs, para que el cargador nativo la mapee.
 * Devuelto como `const void*` en el .h para no arrastrar `esp_partition.h` a
 * quien sólo quiera el layout; el que mapea ya lo incluye de todas formas. */
const void* board_mgr_esp32_bpdata(void) { return (const void*) s_bpdata; }

const bpvm_part_t* board_mgr_esp32_packs(void) {
    /* Sin layout válido no hay zona: el arranque no llegó al estado 2. Devolver
     * NULL y no un rango a cero, porque "no hay" y "hay pero mide 0" mandan a
     * sitios distintos y el llamante tiene que poder decirlo. */
    if (!s_layout.complete && s_layout.parts[BPVM_PART_PACKS].size == 0u) return 0;
    return bpvm_part_get(&s_layout, BPVM_PART_PACKS);
}

/* ── cintura del env: lee/escribe los 2 sectores A/B de la partición bpenv ── */

static int env_read_slots(uint8_t* a, uint8_t* b) {
    if (!s_bpenv) { memset(a, 0xFF, BP_ENV_SECTOR); memset(b, 0xFF, BP_ENV_SECTOR); return 0; }
    int ok = 1;
    if (esp_partition_read(s_bpenv, 0u,            a, BP_ENV_SECTOR) != ESP_OK) { memset(a, 0xFF, BP_ENV_SECTOR); ok = 0; }
    if (esp_partition_read(s_bpenv, BP_ENV_SECTOR, b, BP_ENV_SECTOR) != ESP_OK) { memset(b, 0xFF, BP_ENV_SECTOR); ok = 0; }
    return ok;
}

static void env_write_slot(int slot, const uint8_t* buf) {
    if (!s_bpenv) return;
    uint32_t off = (slot == 0) ? 0u : BP_ENV_SECTOR;
    /* esp_partition_{erase_range,write} son seguros para multitarea (esp-idf) y no
     * suspenden el XIP como el RP2350 → sin gimnasia de IRQs aquí. */
    esp_partition_erase_range(s_bpenv, off, BP_ENV_SECTOR);
    esp_partition_write(s_bpenv, off, buf, BP_ENV_SECTOR);
}

static uint32_t data_usable(void) { return s_bpdata ? (uint32_t) s_bpdata->size : 0u; }

/*
 * V5/H7 — EL ENV, ANTES DE TIEMPO. Sólo el env: ni particiones, ni FS, ni VM.
 *
 * ─── POR QUÉ HACE FALTA ───
 *
 * El bloque de memoria de la BD tiene DOS exigencias que chocan en el orden de
 * arranque del P4:
 *
 *   · su TAMAÑO lo dice el env (`SQLite=<MB>`)      -> hay que leer flash
 *   · su DIRECCIÓN sólo es determinista si muerde
 *     ANTES que el heap de la VM                    -> hay que reservar pronto
 *
 * Y en el P4 el env se carga DESPUÉS del reparto de PSRAM: `app_main` llama a
 * `vm_buffer_init_psram()` enseguida, y `board_mgr_esp32_boot()` sólo corre más
 * tarde, ya dentro de la tarea de transporte. (En el Pico no se nota porque allí
 * el env ya está leído cuando se parte la PSRAM.)
 *
 * Que la dirección sea determinista NO es un lujo: el pack nativo se realoja en
 * el PC para una dirección concreta y lleva ese SELLO dentro. Si `heap_caps_malloc`
 * devolviera otra en el siguiente arranque, el pack grabado dejaría de valer —
 * y el síntoma sería de los peores: funciona una vez y luego no, sin patrón.
 *
 * ─── POR QUÉ ADITIVA Y NO MOVER EL BOOT ───
 *
 * Adelantar `board_mgr_esp32_boot()` entero movería el arranque de una placa que
 * hoy funciona, y arrastraría con él las particiones, el FS y la VM. Esto es
 * leer dos sectores y elegir el bueno: exactamente lo que `layer_partitions`
 * vuelve a hacer luego. Leerlo dos veces no cuesta nada y no cambia nada del
 * camino existente — que sigue intacto.
 *
 * Idempotente a propósito: rellena los MISMOS `s_env_a`/`s_env_b`, así que la
 * vista de `board_mgr_env()` (que apunta dentro de ellos) sigue siendo válida
 * después de que el boat de verdad los relea.
 */
void board_mgr_esp32_env_temprano(void) {
    if (!s_bpenv) {
        s_bpenv = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                           ESP_PARTITION_SUBTYPE_ANY, "bpenv");
    }
    env_read_slots(s_env_a, s_env_b);
    bpvm_env_pick(s_env_a, BP_ENV_SECTOR, s_env_b, BP_ENV_SECTOR, &s_env);
}

/* ── arranque escalonado: particiones del env → FS (sub-rango de bpdata) → VM ── */

static bpvm_boot_step_t layer_partitions(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.ok = 0; r.reason[0] = '\0';
    if (!s_bpenv)  { snprintf(r.reason, sizeof r.reason, "sin particion bpenv"); return r; }
    if (!s_bpdata) { snprintf(r.reason, sizeof r.reason, "sin particion bpdata"); return r; }
    env_read_slots(s_env_a, s_env_b);
    bpvm_env_pick(s_env_a, BP_ENV_SECTOR, s_env_b, BP_ENV_SECTOR, &s_env);
    int bad = -1;
    bpvm_part_err_t e = bpvm_part_layout(&s_env, 0u, data_usable(), BP_ENV_SECTOR, &s_layout, &bad);
    if (e == BPVM_PART_OK) { r.ok = 1; return r; }
    snprintf(r.reason, sizeof r.reason, "%s", bpvm_part_err_str(e));
    return r;
}

static bpvm_boot_step_t layer_fs(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.ok = 0; r.reason[0] = '\0';
    const bpvm_part_t* fsp = bpvm_part_get(&s_layout, BPVM_PART_FS);
    if (!fsp) { snprintf(r.reason, sizeof r.reason, "sin region FS"); return r; }
    /* fsp->offset es RELATIVO a bpdata (base=0) → el 1er byte de la partición de
     * datos. fs_init_at monta littlefs en ese sub-rango. */
    if (fs_init_at(fsp->offset, fsp->size) != FS_OK) {
        snprintf(r.reason, sizeof r.reason, "littlefs no monta ni formatea");
        return r;
    }
    r.ok = 1;
    return r;
}

static bpvm_boot_step_t layer_app(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.reason[0] = '\0';
    r.ok = (s_vm_buffer != NULL && s_vm_buffer_size > 0);
    if (!r.ok) snprintf(r.reason, sizeof r.reason, "heap de la VM no disponible");
    return r;
}

void board_mgr_esp32_boot(void) {
    s_bpenv  = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "bpenv");
    s_bpdata = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "bpdata");

    /* #328 — la tabla de particiones DICE dónde acaba la flash; el chip decide.
     * Si la tabla promete más de lo que hay, las particiones altas existen para
     * el SDK pero no tienen silicio detrás: se escribe, no se guarda, y al releer
     * sale otra cosa — que es exactamente lo que littlefs reporta como CORRUPT.
     * Nadie se entera hasta que algo escribe lejos, así que se comprueba aquí y
     * se DICE, en vez de descubrirlo por un error que habla de otra cosa. */
    {
        uint32_t cfgsz = 0, phys = 0;
        esp_err_t e1 = esp_flash_get_size(NULL, &cfgsz);            /* el CONFIGURADO */
        esp_err_t e2 = esp_flash_get_physical_size(NULL, &phys);    /* el del CHIP */
        uint32_t need = s_bpdata ? (uint32_t) (s_bpdata->address + s_bpdata->size) : 0u;
        if (e1 == ESP_OK && e2 == ESP_OK) {
            /* SON DOS COSAS DISTINTAS y ahí estuvo la trampa del #328: el chip
             * puede ser de 16 MB mientras el driver trabaja con el tamaño que
             * trae la cabecera del BOOTLOADER. Si el bootloader es viejo (2 MB),
             * todo acceso por encima de ese límite no surte efecto — sin error:
             * se escribe, no se guarda, y al releer sale otra cosa, que es lo
             * que littlefs reporta como CORRUPT. Y explica el patrón exacto que
             * vimos: los superbloques (al principio) bien y el resto mal.
             * Un reflasheo SOLO de la app no actualiza el bootloader ⇒ la placa
             * arrastra el límite viejo y nada te lo dice. Aquí sí. */
            log_printf("flash: configurada %u KB | chip fisico %u KB | bpdata acaba en %u KB%s%s",
                       (unsigned) (cfgsz / 1024u), (unsigned) (phys / 1024u),
                       (unsigned) (need / 1024u),
                       (cfgsz < phys) ? "  <<< EL BOOTLOADER USA MENOS FLASH DE LA QUE HAY (reflashear bootloader)" : "",
                       (need > cfgsz) ? "  <<< LA TABLA SE SALE DE LA FLASH CONFIGURADA" : "");
        } else {
            log_printf("flash: no se pudo leer el tamano (cfg=%d fis=%d)", (int) e1, (int) e2);
        }
    }

    bpvm_boot_layers_t layers;
    layers.to_partitions = layer_partitions;
    layers.to_fs         = layer_fs;
    layers.to_app        = layer_app;
    layers.user          = NULL;
    layers.max_state     = BPVM_BOOT_APP;
    bpvm_boot_climb(&layers, &s_boot);

    ESP_LOGI(TAG, "boot: estado %d (%s)%s%s", (int) s_boot.state,
             bpvm_boot_state_name(s_boot.state),
             s_boot.degraded ? " DEGRADADO: " : "",
             s_boot.degraded ? s_boot.reason : "");
    /* Al log PERSISTENTE también: la consola se pierde al desconectar y en el P4
     * puede ni existir. Si el climb se queda corto, el porqué sobrevive al reset
     * y se lee luego con LOG_DUMP desde el IDE — que es para lo que está. */
    log_printf("boot: estado %d (%s)%s%s", (int) s_boot.state,
               bpvm_boot_state_name(s_boot.state),
               s_boot.degraded ? " DEGRADADO: " : "",
               s_boot.degraded ? s_boot.reason : "");
    log_flush();
}

/* ── ramo del wire (STATE/ENV_x/PART_x): repl_esp32 encamina aquí ── */

/* #338 — la zona compartida tiene que dar para las DOS copias del env de ESTA
 * familia. Si alguien cambia el sector de borrado (o porta a un micro con uno
 * mayor) sin subir BPVM_SCRATCH_BYTES, aqui no compila — en vez de descubrirlo
 * en placa como un "zona de scratch no disponible" al abrir el panel. */
typedef char bp_chk_scratch_env[(BPVM_SCRATCH_BYTES >= 2u * BP_ENV_SECTOR) ? 1 : -1];

void board_mgr_esp32_handle(long id, const json_obj_t* obj, const char* type,
                            unsigned char* scratch, unsigned long scratch_len,
                            const unsigned char* bulk, unsigned long bulk_len) {
    /* #338 — DOS prestamistas en vez de uno (mismo reparto que Pico y STM32):
     * del buffer prestado (s_put_buf, libre durante un comando de gestión) salen
     * el sector de TRABAJO y la respuesta; las dos copias del env las presta la
     * ZONA DE RASCAR compartida, libre aquí porque sus otros usuarios son los
     * PACK_x y ésos no tocan el env (bpvm_bmgr_needs_env). Antes este buffer
     * tenía que dar para los TRES sectores a la vez + la respuesta = 20 KB de
     * DRAM permanentes, que en el S3 es justo lo que falta. */
    const int con_env = bpvm_bmgr_needs_env(type);
    /* Los PACK_x no usan sector de trabajo (sólo part_apply toca bm->scratch), y
     * además llegan aquí con el buffer ya mordido por su bulk: pedirles un sector
     * que no van a usar los dejaría sin sitio. */
    const unsigned long minimo = con_env ? (unsigned long) BP_ENV_SECTOR + 512u : 512u;
    if (scratch == NULL || scratch_len < minimo) {
        wire_v1_send_error(id, "INTERNAL_ERROR", "scratch insuficiente");
        return;
    }
    if (!s_bpenv) {
        wire_v1_send_error(id, "INTERNAL_ERROR", "sin particion bpenv (reflashear tabla)");
        return;
    }
    uint8_t* sc        = con_env ? scratch : NULL;
    char*    reply     = (char*)  (con_env ? scratch + BP_ENV_SECTOR : scratch);
    size_t   reply_cap = (size_t) (con_env ? scratch_len - BP_ENV_SECTOR : scratch_len);

    uint8_t* a = NULL;
    uint8_t* b = NULL;
    if (con_env) {
        /* Contiguas y en ese orden: `b` cuelga de `a`. Una petición, un dueño. */
        a = (uint8_t*) bpvm_scratch_take(2u * (size_t) BP_ENV_SECTOR, "bmgr-env");
        if (a == NULL) {
            wire_v1_send_error(id, "INTERNAL_ERROR",
                               "zona de scratch no disponible para el entorno");
            return;
        }
        b = a + BP_ENV_SECTOR;
        env_read_slots(a, b);   /* copias frescas del env desde flash */
    }

    bpvm_bmgr_t bm;
    memset(&bm, 0, sizeof bm);         /* campos nuevos (p.ej. packs H3) nunca con basura de pila */
    bm.a = a; bm.b = b; bm.scratch = sc;
    bm.sector = BP_ENV_SECTOR;
    bm.part_base = 0u;                 /* offsets relativos a bpdata */
    bm.usable_flash = data_usable();   /* la tabla vendor ES el límite; sin clamp #292 */
    bm.live = &s_boot;                 /* STATE cuenta el estado REAL del boot */

    /* V5/H7 — la zona de packs. Sólo si la familia registró su vista Y el layout
     * dice que la región existe: las dos condiciones, porque una vista sin
     * región sería leer fuera, y una región sin vista no se puede leer. */
    {
        const bpvm_part_t* pp = bpvm_part_get(&s_layout, BPVM_PART_PACKS);
        if (s_packs_view && pp && pp->size > 0) {
            bm.packs_base  = s_packs_view;
            bm.packs_size  = (s_packs_view_size < pp->size) ? s_packs_view_size
                                                            : pp->size;
            bm.packs_flash = &s_packs_fl;
        }
    }

    /* V5/H8 — la RAM de trabajo de un motor nativo: el PRINCIPIO del bloque de
     * la BD (`[estaticos | arena]`). El P4 ya lo calcula al arrancar y lo
     * imprime; aquí sólo se expone para que `PACK_BURN_BEGIN` se lo diga al
     * IDE. En el S3 no hay bloque: queda a 0 y el IDE lo dirá en vez de grabar
     * un motor que no podría arrancar. */
#if CONFIG_IDF_TARGET_ESP32P4
    bm.pack_ram_base = (uint32_t) (uintptr_t) s_pack_ram_base;
    bm.pack_ram_size = s_sqlite_size;
#endif

    bpvm_bmgr_req_t req;
    memset(&req, 0, sizeof req);
    snprintf(req.type, sizeof req.type, "%s", type);
    req.id = id;
    req.has_key   = json_get_str(obj, "key",   req.key,   sizeof req.key)   >= 0;
    req.has_value = json_get_str(obj, "value", req.value, sizeof req.value) >= 0;
    for (int i = 0; i < BPVM_PART_COUNT; i++)
        req.part_sizes[i] = json_get_long(obj, bpvm_part_name((bpvm_part_kind_t) i), -1);
    req.bulk     = bulk;            /* #327 H3: PACK_BURN_DATA (ya recibido) */
    req.bulk_len = (long) bulk_len;

    /* V5/H7 — LA OTRA MITAD. De los seis campos que piden los PACK_*, el ESP32
     * rellenaba SOLO `bulk`: sin `off` no van ENTRIES/DEL/READ, sin `size` no
     * arranca el BURN_BEGIN, y sin `confirm_yes` el FORMAT rechaza aunque el IDE
     * mande el confirm (que lo manda). Es exactamente el mismo agujero que se
     * arregló en el Pico copiándolo del STM32 — y aquí seguía abierto. */
    req.off      = json_get_long(obj, "offset", -1);   /* PACK_ENTRIES/DEL/READ */
    req.has_off  = req.off >= 0;
    req.size     = json_get_long(obj, "size", -1);     /* PACK_BURN_BEGIN */
    req.has_size = req.size >= 0;
    {                                                  /* PACK_FORMAT (confirm=YES) */
        /* Es una CADENA "YES", no un booleano: el formateo borra la zona entera
         * y el contrato pide escribirlo, no marcar una casilla. Copiado literal
         * del Pico — leerlo como número aquí habría hecho que el FORMAT
         * rechazara SIEMPRE, con el IDE mandando el confirm correcto. */
        char confirm[8];
        req.confirm_yes = json_get_str(obj, "confirm", confirm, sizeof confirm) >= 0
                          && strcmp(confirm, "YES") == 0;
    }

    int wrote = -1;
    int n = bpvm_bmgr_wire_dispatch(&bm, &req, reply, reply_cap, &wrote);
    /* La zona se suelta en TODAS las salidas —quedársela colgada dejaría mudos los
     * PACK_x y los comandos del entorno siguientes— y DESPUÉS del volcado, porque
     * el sector que va a flash vive en ella. */
    if (n < 0) {
        if (con_env) bpvm_scratch_give("bmgr-env");
        wire_v1_send_error(id, "INTERNAL_ERROR", "reply de gestion no cabe");
        return;
    }
    if (wrote >= 0) env_write_slot(wrote, wrote == 0 ? a : b);   /* RAM → flash */
    if (con_env) bpvm_scratch_give("bmgr-env");
    wire_v1_send_line(reply, (size_t) n);
}
