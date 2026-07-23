/*
 * board_mgr_stm32.c — H9: adaptador de gestión de placa del STM32 (U5). Ver el .h.
 *
 * Cintura del env A/B sobre la flash interna (offsets FIJOS de flash_layout_stm32.h,
 * páginas de 8K; el U5 no tiene tabla vendor como la ESP32 → constantes como la Pico)
 * + arranque escalonado + board_boot_status + ramo STATE/ENV_x/PART_x del wire.
 * Modelo "un mando": la zona BP_DATA [BP_PART_BASE, BP_USABLE_FLASH) es de las
 * particiones; el env reparte FS|Packs (FS knob, Packs resto). Mismo núcleo
 * bpvm_part/bpvm_bmgr_wire que el resto de familias.
 */
#include "board_mgr_stm32.h"

#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_pack.h"           /* H3 — cintura de burn de la zona de packs */
#include "bpvm_part.h"
#include "bpvm_env.h"
#include "stm32_wire.h"          /* stm32_wire_send_error / send_line (no wire_v1) */
#include "stm32_fs.h"            /* fs_init_at */
#include "stm32_flash.h"         /* stm32_flash_write / erase (compartidas con el FS) */
#include "flash_layout_stm32.h"  /* BP_ENV_A/B_OFFSET, BP_PART_BASE, BP_USABLE_FLASH, símbolos .ld */

#include "main.h"                /* FLASH_BASE */
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Estado del boot + el layout del climb (estáticos: el layout debe sobrevivir para
 * que layer_fs lo use; el boot lo lee board_boot_status). s_layout_ok = el layout
 * validó en el boot → la zona de packs (BPVM_PART_PACKS) es direccionable (H3). */
static bpvm_boot_status_t s_boot;
static bpvm_part_layout_t s_layout;
static int s_layout_ok = 0;
/* Copias A/B del env para layer_partitions (8 KB c/u en .bss; fuera del stack). */
static uint8_t s_env_a[BP_ENV_SECTOR];
static uint8_t s_env_b[BP_ENV_SECTOR];

const bpvm_boot_status_t* board_boot_status(void) { return &s_boot; }

/* ── cintura del env: lee/escribe las 2 copias A/B (flash interna mapeada) ── */

static void env_read_slots(uint8_t* a, uint8_t* b) {
    memcpy(a, (const void*) (uintptr_t) (FLASH_BASE + BP_ENV_A_OFFSET), BP_ENV_SECTOR);
    memcpy(b, (const void*) (uintptr_t) (FLASH_BASE + BP_ENV_B_OFFSET), BP_ENV_SECTOR);
}

static void env_write_slot(int slot, const uint8_t* src) {
    uint32_t addr = FLASH_BASE + ((slot == 0) ? BP_ENV_A_OFFSET : BP_ENV_B_OFFSET);
    if (stm32_flash_erase(addr, 1u) == 0)          /* 1 página de 8 KB */
        stm32_flash_write(addr, src, BP_ENV_SECTOR);
}

/* ── H3: cintura de BURN de la zona de packs (offsets RELATIVOS a la región;
 * la región = partición PACKS del layout, s_packs_off relativo a FLASH_BASE).
 * El U5 borra por página de 8K y programa por quadword de 16 con ECC: un
 * quadword solo se programa UNA vez → los grupos todo-0xFF se SALTAN (quedan
 * de verdad borrados; el slack del pack no se toca). ── */
static uint32_t s_packs_off;   /* offset flash de la región (lo fija handle()) */

static int packs_fl_erase(void* u, uint32_t off, uint32_t len) {
    (void) u;
    if ((len % BP_ENV_SECTOR) != 0) return -1;
    return stm32_flash_erase(FLASH_BASE + s_packs_off + off, len / BP_ENV_SECTOR);
}
static int packs_fl_program(void* u, uint32_t off, const uint8_t* d, uint32_t len) {
    (void) u;
    uint32_t base = FLASH_BASE + s_packs_off + off;
    uint32_t i = 0;
    while (i < len) {
        uint32_t run = len - i < 16u ? len - i : 16u;   /* grupo de quadword */
        int all_ff = 1;
        for (uint32_t k = 0; k < run; k++)
            if (d[i + k] != 0xFF) { all_ff = 0; break; }
        if (!all_ff && stm32_flash_write(base + i, d + i, run) != 0) return -1;
        i += run;
    }
    return 0;
}
static const bpvm_pack_flash_t s_packs_fl =
    { packs_fl_erase, packs_fl_program, NULL, BP_ENV_SECTOR /* pagina 8K */ };

/* ── arranque escalonado: particiones del env → FS (sub-rango de BP_DATA) → VM ── */

static bpvm_boot_step_t layer_partitions(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.ok = 0; r.reason[0] = '\0';
    env_read_slots(s_env_a, s_env_b);
    bpvm_env_t env;
    bpvm_env_pick(s_env_a, BP_ENV_SECTOR, s_env_b, BP_ENV_SECTOR, &env);
    int bad = -1;
    /* base = BP_PART_BASE, usable = fin de flash (fijo por placa; sin clamp #292
     * — el U5 conoce su tamaño de flash en compilación, no hay sorpresa JEDEC). */
    bpvm_part_err_t e = bpvm_part_layout(&env, BP_PART_BASE, BP_USABLE_FLASH,
                                         BP_ENV_SECTOR, &s_layout, &bad);
    if (e == BPVM_PART_OK) { s_layout_ok = 1; r.ok = 1; return r; }
    snprintf(r.reason, sizeof r.reason, "%s", bpvm_part_err_str(e));
    return r;
}

static bpvm_boot_step_t layer_fs(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.ok = 0; r.reason[0] = '\0';
    const bpvm_part_t* fsp = bpvm_part_get(&s_layout, BPVM_PART_FS);
    if (!fsp) { snprintf(r.reason, sizeof r.reason, "sin region FS"); return r; }
    /* fsp->offset es RELATIVO a FLASH_BASE (base=BP_PART_BASE) → fs_init_at monta
     * littlefs en ese sub-rango de la zona de datos. */
    if (fs_init_at(fsp->offset, fsp->size) != 0) {
        snprintf(r.reason, sizeof r.reason, "littlefs no monta ni formatea");
        return r;
    }
    r.ok = 1;
    return r;
}

static bpvm_boot_step_t layer_app(void* u) {
    (void) u;
    /* La memoria de la VM del STM32 es un array estático (s_vm_mem en el repl) →
     * SIEMPRE presente (a diferencia del ESP32, que la saca de PSRAM en runtime). */
    bpvm_boot_step_t r; r.ok = 1; r.reason[0] = '\0';
    return r;
}

void board_mgr_stm32_boot(void) {
    bpvm_boot_layers_t layers;
    layers.to_partitions = layer_partitions;
    layers.to_fs         = layer_fs;
    layers.to_app        = layer_app;
    layers.user          = NULL;
    layers.max_state     = BPVM_BOOT_APP;
    bpvm_boot_climb(&layers, &s_boot);
    /* Sin log de consola (el STM32 no tiene una): el estado se consulta por STATE. */

    /* H3.c — montar la zona de packs (XIP) para la resolución de módulos del
     * RUN: el repl la consulta como fallback tras el FS (el FS eclipsa). El
     * mismo montaje que hace el host con --pack=, aquí sobre la flash real. */
    if (s_layout_ok) {
        const bpvm_part_t* pp = bpvm_part_get(&s_layout, BPVM_PART_PACKS);
        if (pp && pp->size > 0) {
            bpvm_pack_mount((const uint8_t*) (uintptr_t) (FLASH_BASE + pp->offset),
                            pp->size);
        }
    }
}

/* ── ramo del wire (STATE/ENV_x/PART_x): el repl encamina aquí ── */

void board_mgr_stm32_handle(long id, const json_obj_t* obj, const char* type,
                            unsigned char* scratch, unsigned long scratch_len,
                            const unsigned char* bulk, unsigned long bulk_len) {
    /* Red anti-divergencia .ld ↔ flash_layout_stm32.h: el env debe caer entre el
     * fin del firmware y el inicio de los datos. Si alguien mueve las regiones del
     * linker sin tocar el header (o al revés), tocar flash borraría CÓDIGO → se
     * rechaza a gritos, nunca se escribe. */
    if ((uintptr_t) (FLASH_BASE + BP_ENV_A_OFFSET) < (uintptr_t) __bp_flash_end
        || (uintptr_t) (FLASH_BASE + BP_ENV_B_OFFSET + BP_ENV_SECTOR) > (uintptr_t) __bp_part_start) {
        stm32_wire_send_error(id, "INTERNAL_ERROR",
                              "layout de flash inconsistente (env fuera de la zona 2)");
        return;
    }
    if (scratch == NULL || scratch_len < (unsigned long) (3u * BP_ENV_SECTOR + 512u)) {
        stm32_wire_send_error(id, "INTERNAL_ERROR", "scratch insuficiente");
        return;
    }
    uint8_t* a         = scratch + 0u * BP_ENV_SECTOR;
    uint8_t* b         = scratch + 1u * BP_ENV_SECTOR;
    uint8_t* sc        = scratch + 2u * BP_ENV_SECTOR;
    char*    reply     = (char*)  (scratch + 3u * BP_ENV_SECTOR);
    size_t   reply_cap = (size_t) (scratch_len - 3u * BP_ENV_SECTOR);

    env_read_slots(a, b);   /* copias frescas del env desde flash */

    bpvm_bmgr_t bm;
    memset(&bm, 0, sizeof bm);           /* campos nuevos futuros nunca con basura */
    bm.a = a; bm.b = b; bm.scratch = sc;
    bm.sector = BP_ENV_SECTOR;
    bm.part_base = BP_PART_BASE;
    bm.usable_flash = BP_USABLE_FLASH;   /* tamaño de flash fijo por placa; sin clamp */
    bm.live = &s_boot;                   /* STATE cuenta el estado REAL del boot */
    /* H3 — zona de packs: XIP directo sobre la partición PACKS del layout del boot
     * (offset relativo a FLASH_BASE, como el FS). Sin layout válido → sin packs. */
    if (s_layout_ok) {
        const bpvm_part_t* pp = bpvm_part_get(&s_layout, BPVM_PART_PACKS);
        if (pp && pp->size > 0) {
            bm.packs_base = (const uint8_t*) (uintptr_t) (FLASH_BASE + pp->offset);
            bm.packs_size = pp->size;
            s_packs_off   = pp->offset;      /* la cintura de burn escribe aquí */
            bm.packs_flash = &s_packs_fl;
        }
    }

    bpvm_bmgr_req_t req;
    memset(&req, 0, sizeof req);
    snprintf(req.type, sizeof req.type, "%s", type);
    req.id = id;
    req.has_key   = json_get_str(obj, "key",   req.key,   sizeof req.key)   >= 0;
    req.has_value = json_get_str(obj, "value", req.value, sizeof req.value) >= 0;
    for (int i = 0; i < BPVM_PART_COUNT; i++)
        req.part_sizes[i] = json_get_long(obj, bpvm_part_name((bpvm_part_kind_t) i), -1);
    req.off = json_get_long(obj, "offset", -1);     /* H3: PACK_ENTRIES/DEL/READ */
    req.has_off = req.off >= 0;
    req.size = json_get_long(obj, "size", -1);      /* H3: PACK_BURN_BEGIN */
    req.has_size = req.size >= 0;
    req.bulk = bulk;                                /* H3: PACK_BURN_DATA (ya recibido) */
    req.bulk_len = (long) bulk_len;
    {                                               /* H3: PACK_FORMAT (confirm=YES) */
        char confirm[8];
        req.confirm_yes = json_get_str(obj, "confirm", confirm, sizeof confirm) >= 0
                          && strcmp(confirm, "YES") == 0;
    }

    int wrote = -1;
    int n = bpvm_bmgr_wire_dispatch(&bm, &req, reply, reply_cap, &wrote);
    if (n < 0) { stm32_wire_send_error(id, "INTERNAL_ERROR", "reply de gestion no cabe"); return; }
    if (wrote >= 0) env_write_slot(wrote, wrote == 0 ? a : b);   /* RAM → flash */
    stm32_wire_send_line(reply, (size_t) n);
}
