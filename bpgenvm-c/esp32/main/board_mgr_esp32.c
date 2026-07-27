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

#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_part.h"
#include "bpvm_env.h"
#include "wire_v1.h"
#include "fs.h"
#include "log.h"          /* el resultado del climb, al log persistente */

#include "esp_partition.h"
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

void board_mgr_esp32_handle(long id, const json_obj_t* obj, const char* type,
                            unsigned char* scratch, unsigned long scratch_len) {
    if (scratch == NULL || scratch_len < (unsigned long) (3u * BP_ENV_SECTOR + 512u)) {
        wire_v1_send_error(id, "INTERNAL_ERROR", "scratch insuficiente");
        return;
    }
    if (!s_bpenv) {
        wire_v1_send_error(id, "INTERNAL_ERROR", "sin particion bpenv (reflashear tabla)");
        return;
    }
    uint8_t* a         = scratch + 0u * BP_ENV_SECTOR;
    uint8_t* b         = scratch + 1u * BP_ENV_SECTOR;
    uint8_t* sc        = scratch + 2u * BP_ENV_SECTOR;
    char*    reply     = (char*)  (scratch + 3u * BP_ENV_SECTOR);
    size_t   reply_cap = (size_t) (scratch_len - 3u * BP_ENV_SECTOR);

    env_read_slots(a, b);   /* copias frescas del env desde flash */

    bpvm_bmgr_t bm;
    memset(&bm, 0, sizeof bm);         /* campos nuevos (p.ej. packs H3) nunca con basura de pila */
    bm.a = a; bm.b = b; bm.scratch = sc;
    bm.sector = BP_ENV_SECTOR;
    bm.part_base = 0u;                 /* offsets relativos a bpdata */
    bm.usable_flash = data_usable();   /* la tabla vendor ES el límite; sin clamp #292 */
    bm.live = &s_boot;                 /* STATE cuenta el estado REAL del boot */

    bpvm_bmgr_req_t req;
    memset(&req, 0, sizeof req);
    snprintf(req.type, sizeof req.type, "%s", type);
    req.id = id;
    req.has_key   = json_get_str(obj, "key",   req.key,   sizeof req.key)   >= 0;
    req.has_value = json_get_str(obj, "value", req.value, sizeof req.value) >= 0;
    for (int i = 0; i < BPVM_PART_COUNT; i++)
        req.part_sizes[i] = json_get_long(obj, bpvm_part_name((bpvm_part_kind_t) i), -1);

    int wrote = -1;
    int n = bpvm_bmgr_wire_dispatch(&bm, &req, reply, reply_cap, &wrote);
    if (n < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "reply de gestion no cabe"); return; }
    if (wrote >= 0) env_write_slot(wrote, wrote == 0 ? a : b);   /* RAM → flash */
    wire_v1_send_line(reply, (size_t) n);
}
