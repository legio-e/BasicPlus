/*
 * board_mgr_pico.c — H9: adaptador del firmware para la gestión de placa (RP2350).
 * Cintura de flash del bloque de env (A/B) + puente al núcleo compartido
 * bpvm_bmgr_wire (mismo que el boardsim de host → replies byte-idénticas). Ver .h.
 */
#include "board_mgr_pico.h"

#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_part.h"
#include "bpvm_pack.h"   /* #327: cintura de flash de la zona de packs */
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

/* Base de particiones: BP_PART_BASE de flash_layout.h (0x200000). Desde la
 * unificación H9 (19-jul) las particiones del env CONDUCEN el FS real (el
 * bp_ptable_t viejo murió) → base y clamp deben ser LOS MISMOS que usa el
 * boot (main.c layer_partitions), o FrmBoard propondría layouts que el
 * arranque luego rechaza. */

/* ── #327: zona de packs (H3) ─────────────────────────────────────────────
 * El Pico encaminaba los PACK_* desde f741693 pero NUNCA cableaba la región
 * detrás: bm.packs_* se quedaba a cero y el LS contestaba —correctamente— "esta
 * placa no expone packs", hiciera Eduardo lo que hiciera con las particiones.
 * Encaminar los comandos y no dar la zona es media función; el STM32 sí lo hacía
 * (board_mgr_stm32.c) y el simulado también, así que el Pico era el hueco.
 *
 * La cintura recibe offsets RELATIVOS a la región; el absoluto sale del layout
 * del boot, que es la ÚNICA verdad sobre dónde vive cada partición. */
static uint32_t s_packs_off = 0;      /* offset absoluto de la región en flash */

static int packs_fl_erase(void* user, uint32_t off, uint32_t len) {
    (void) user;
    uint32_t tok = bpvm_flash_lock_begin();
    flash_range_erase(s_packs_off + off, len);
    bpvm_flash_lock_end(tok);
    return 0;
}

static int packs_fl_program(void* user, uint32_t off, const uint8_t* d, uint32_t len) {
    (void) user;
    /* flash_range_program exige múltiplo de FLASH_PAGE_SIZE (256). El núcleo
     * garantiza múltiplos de 16 (contrato uniforme de los 3 micros), así que la
     * cola se completa con 0xFF —neutro en NOR, no altera lo ya escrito. */
    static uint8_t page[FLASH_PAGE_SIZE];
    uint32_t done = 0;
    while (done < len) {
        uint32_t run = len - done;
        if (run > FLASH_PAGE_SIZE) run = FLASH_PAGE_SIZE;
        memset(page, 0xFF, sizeof page);
        memcpy(page, d + done, run);
        uint32_t tok = bpvm_flash_lock_begin();
        flash_range_program(s_packs_off + off + done, page, FLASH_PAGE_SIZE);
        bpvm_flash_lock_end(tok);
        done += run;
    }
    return 0;
}

static const bpvm_pack_flash_t s_packs_fl =
    { packs_fl_erase, packs_fl_program, NULL, FLASH_SECTOR_SIZE /* 4K */ };

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
                           unsigned char* scratch, unsigned long scratch_len,
                           const unsigned char* bulk, unsigned long bulk_len) {
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
    memset(&bm, 0, sizeof bm);   /* campos nuevos (p.ej. packs H3) nunca con basura de pila */
    bm.a = a; bm.b = b; bm.scratch = sc;
    bm.sector = BP_ENV_SECTOR;
    bm.part_base = BP_PART_BASE;
    /* usable CON clamp #292 (= el mismo número que usa el boot): desde la
     * unificación el plan SE EJECUTA (el mount sale de aquí), así que proponer
     * más de lo escribible sería ofrecer un layout que el arranque rechaza. */
    bm.usable_flash = bpvm_part_usable_flash(bd ? bd->flash_bytes : 0u,
                                             PICO_FLASH_SIZE_BYTES);
    /* STATE cuenta el estado REAL alcanzado por el boot (no el plan del env). */
    bm.live = board_boot_status();

    /* #327 — zona de packs: XIP directo sobre la partición PACKS del layout del
     * BOOT (el mismo que conduce el FS: una sola verdad). Sin layout válido o
     * con la partición a 0 → sin packs, y el LS lo dice en vez de callarse. */
    const bpvm_part_layout_t* lay = board_partitions();
    if (lay) {
        const bpvm_part_t* pp = bpvm_part_get(lay, BPVM_PART_PACKS);
        if (pp && pp->size > 0) {
            bm.packs_base  = (const uint8_t*) (uintptr_t) (XIP_BASE + pp->offset);
            bm.packs_size  = pp->size;
            s_packs_off    = pp->offset;      /* la cintura de burn escribe aquí */
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

    req.bulk     = bulk;            /* #327 H3: PACK_BURN_DATA (ya recibido) */
    req.bulk_len = (long) bulk_len;

    int wrote = -1;
    int n = bpvm_bmgr_wire_dispatch(&bm, &req, reply, reply_cap, &wrote);
    if (n < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "reply de gestion no cabe"); return; }
    if (wrote >= 0) env_write(wrote, wrote == 0 ? a : b);   /* la cintura: RAM → flash */
    wire_v1_send_line(reply, (size_t) n);
}
