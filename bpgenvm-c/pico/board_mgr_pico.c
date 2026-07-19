/*
 * board_mgr_pico.c — H9: adaptador del firmware para la gestión de placa (RP2350).
 * Cintura de flash del bloque de env (A/B) + puente al núcleo compartido
 * bpvm_bmgr_wire (mismo que el boardsim de host → replies byte-idénticas). Ver .h.
 */
#include "board_mgr_pico.h"

#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_part.h"
#include "wire_v1.h"
#include "flash_lock.h"
#include "board_desc.h"
#include "flash_layout.h"

#include "pico/stdlib.h"
#include "hardware/flash.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Bloque de env A/B: ZONA 2 del layout de 3 zonas (flash_layout.h) — el hueco
 * 0x010000-0x013FFF que el UF2 NO graba (el env sobrevive al reflasheo) y que
 * el linker (bp_memmap.ld) mantiene libre entre FLASH_BOOT y FLASH_MAIN. El
 * borrado es por sector de 4K; cada copia ocupa un sector. */
#define BP_ENV_SECTOR    FLASH_SECTOR_SIZE        /* 4096 */

/* Base conceptual de las particiones H9 (recordadas en el env). De momento NO
 * conduce el FS real (que sigue con bp_ptable_t); solo define el reparto que el IDE
 * muestra/edita. El tamaño usable = flash real por JEDEC (board_desc) → Pico 4 MB /
 * Metro 16 MB se distinguen solos en los defaults. La unificación con el FS real
 * llega cuando H9 subsuma bp_ptable_t. */
#define BP_PART_BASE     0x00100000u

/* Vuelca a flash un sector A/B (borra + programa el sector entero de 4K, bajo la
 * ventana XIP-safe). El otro sector queda intacto (A/B). `src` = el buffer RAM. */
static void env_write(int slot, const uint8_t* src) {
    uint32_t off = (slot == 0) ? BP_ENV_A_OFFSET : BP_ENV_B_OFFSET;
    uint32_t tok = bpvm_flash_lock_begin();
    flash_range_erase(off, BP_ENV_SECTOR);
    flash_range_program(off, src, BP_ENV_SECTOR);   /* 4096 = múltiplo de FLASH_PAGE_SIZE */
    bpvm_flash_lock_end(tok);
}

void board_mgr_pico_handle(long id, const json_obj_t* obj, const char* type,
                           unsigned char* scratch, unsigned long scratch_len) {
    /* Red anti-divergencia bp_memmap.ld ↔ flash_layout.h: los offsets del env
     * deben caer DENTRO del hueco real (zona 2, entre FLASH_BOOT y FLASH_MAIN).
     * Si alguien mueve las regiones del linker sin tocar el header (o al revés),
     * tocar flash borraría CÓDIGO → rechazar a gritos, nunca escribir. */
    if ((uintptr_t)(XIP_BASE + BP_ENV_A_OFFSET) < (uintptr_t) __bp_zone1_end
        || (uintptr_t)(XIP_BASE + BP_ENV_B_OFFSET + BP_ENV_SECTOR) > (uintptr_t) __bp_zone3_start) {
        wire_v1_send_error(id, "INTERNAL_ERROR",
                           "layout de flash inconsistente (env fuera de la zona 2)");
        return;
    }
    /* Sin BSS propio: troceamos el buffer prestado (s_put_buf, libre durante un comando
     * de gestión). 3 sectores (a/b/scratch) + el resto para la reply. */
    if (scratch == NULL || scratch_len < (unsigned long) (3u * BP_ENV_SECTOR + 512u)) {
        wire_v1_send_error(id, "INTERNAL_ERROR", "scratch insuficiente");
        return;
    }
    uint8_t* a         = scratch + 0u * BP_ENV_SECTOR;
    uint8_t* b         = scratch + 1u * BP_ENV_SECTOR;
    uint8_t* sc        = scratch + 2u * BP_ENV_SECTOR;
    char*    reply     = (char*)  (scratch + 3u * BP_ENV_SECTOR);
    size_t   reply_cap = (size_t) (scratch_len - 3u * BP_ENV_SECTOR);

    /* Lee las dos copias del env desde flash (XIP) al scratch prestado. */
    memcpy(a, (const void*)(XIP_BASE + BP_ENV_A_OFFSET), BP_ENV_SECTOR);
    memcpy(b, (const void*)(XIP_BASE + BP_ENV_B_OFFSET), BP_ENV_SECTOR);

    const board_desc_t* bd = board_desc();
    bpvm_bmgr_t bm;
    bm.a = a; bm.b = b; bm.scratch = sc;
    bm.sector = BP_ENV_SECTOR;
    bm.part_base = BP_PART_BASE;
    /* usable = flash real por JEDEC, sin clamp (el plan H9 se GUARDA, no se escribe;
     * el env vive en la zona 2 —0x010/0x011000, muy por debajo de 4 MB— → escritura segura). */
    bm.usable_flash = bpvm_part_usable_flash(bd ? bd->flash_bytes : 0u, 0u);

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
    if (wrote >= 0) env_write(wrote, wrote == 0 ? a : b);   /* la cintura: RAM → flash */
    wire_v1_send_line(reply, (size_t) n);
}
