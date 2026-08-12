/*
 * board_mgr_pico.c — H9: adaptador del firmware para la gestión de placa (RP2350).
 * Cintura de flash del bloque de env (A/B) + puente al núcleo compartido
 * bpvm_bmgr_wire (mismo que el boardsim de host → replies byte-idénticas). Ver .h.
 */
#include "board_mgr_pico.h"

#include "bpvm.h"        /* #338: bpvm_scratch_take/give (zona de rascar compartida) */
#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_part.h"
#include "bpvm_pack.h"   /* #327: cintura de flash de la zona de packs */
#include "wire_v1.h"
#include "flash_lock.h"
#include "board_desc.h"
#include "flash_layout.h"
#include "pack_pico.h"   /* V5/H8: s_pack_ram_base — la RAM del motor nativo */

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

/*
 * flash_range_program pide DOS cosas, y la versión anterior sólo daba una:
 *
 *   "Flash address of the first byte to be programmed. Must be aligned to a
 *    256-byte flash page. [...] count [...] must be a multiple of 256 bytes."
 *                              — pico-sdk, hardware/flash.h
 *
 * El tamaño estaba contemplado (la cola se rellenaba con 0xFF); la DIRECCIÓN no.
 * Y el núcleo no las manda alineadas: con la cabecera de 128 B retenida, el
 * primer trozo entra en `off+128`, y al activar el pack la cabecera se escribe
 * en `off+16`. Tres `program` desalineados por grabación.
 *
 * En el SDK la comprobación es `invalid_params_if(...)`, que en release SE
 * COMPILA FUERA: la dirección torcida llega a la ROM sin una queja, y lo que
 * queda en flash no es lo que se mandó → el readback de `burn_end` no cuadra y
 * sale VERIFY_FAIL. Muy difícil de ver, porque en el RP2350 los packs son el
 * ÚNICO sitio que programa desalineado: littlefs escribe siempre en múltiplos
 * de su cache (256), así que el FS jamás toca este caso.
 *
 * La cintura del STM32 no tiene el problema porque el U5 escribe por quadword
 * en la dirección que le den — por eso allí los packs iban y aquí no.
 *
 * Ahora cada escritura se parte por FRONTERA DE PÁGINA: se calcula la página que
 * toca, se rellena de 0xFF y se coloca el tramo en su sitio dentro de ella. El
 * relleno es neutro en NOR (0xFF no baja ningún bit), así que reprogramar una
 * página para completar otro tramo no altera lo ya escrito.
 */
static int packs_fl_program(void* user, uint32_t off, const uint8_t* d, uint32_t len) {
    (void) user;
    static uint8_t page[FLASH_PAGE_SIZE];
    uint32_t abs  = s_packs_off + off;
    uint32_t done = 0;
    while (done < len) {
        uint32_t cur  = abs + done;
        uint32_t base = cur & ~(uint32_t) (FLASH_PAGE_SIZE - 1);  /* la página */
        uint32_t sesgo = cur - base;                  /* dónde caemos dentro   */
        uint32_t run  = FLASH_PAGE_SIZE - sesgo;      /* hasta el fin de página */
        if (run > len - done) run = len - done;
        memset(page, 0xFF, sizeof page);
        memcpy(page + sesgo, d + done, run);
        uint32_t tok = bpvm_flash_lock_begin();
        flash_range_program(base, page, FLASH_PAGE_SIZE);
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

/* #338 — la zona compartida tiene que dar para las DOS copias del env de ESTA
 * familia. Si alguien cambia el sector de borrado (o porta a un micro con uno
 * mayor) sin subir BPVM_SCRATCH_BYTES, aqui no compila — en vez de descubrirlo
 * en placa como un "zona de scratch no disponible" al abrir el panel. */
typedef char bp_chk_scratch_env[(BPVM_SCRATCH_BYTES >= 2u * BP_ENV_SECTOR) ? 1 : -1];

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
    /* Sin BSS propio, y desde #338 con DOS prestamistas en vez de uno:
     *
     *   - del buffer prestado (s_put_buf, libre durante un comando de gestión)
     *     salen sólo el sector de TRABAJO y la respuesta → 1 sector + reply;
     *   - las dos copias del env las presta la ZONA DE RASCAR compartida, que
     *     durante un comando del entorno está libre: sus otros usuarios son los
     *     PACK_*, y ésos no tocan el env (bpvm_bmgr_needs_env, la frontera).
     *
     * El motivo es que antes el buffer del bulk tenía que dar para los TRES
     * sectores a la vez (12 KB en la Pico) + la respuesta = 20 KB permanentes de
     * SRAM que en esta placa se le restan al heap de la VM. Compartiendo con una
     * zona que ya existía, el buffer baja a 8 KB y la memoria NO reaparece en
     * otro sitio: se comparte, no se duplica. */
    const int con_env = bpvm_bmgr_needs_env(type);
    /* Los PACK_* no necesitan sector de trabajo (sólo part_apply usa bm->scratch),
     * así que el buffer prestado va ENTERO a la respuesta. Importa: en el
     * PACK_BURN_DATA el bulk ya se ha comido el principio del buffer y aquí llega
     * sólo el resto — pedirle un sector que no va a usar lo dejaría sin sitio. */
    const unsigned long minimo = con_env ? (unsigned long) BP_ENV_SECTOR + 512u : 512u;
    if (scratch == NULL || scratch_len < minimo) {
        wire_v1_send_error(id, "INTERNAL_ERROR", "scratch insuficiente");
        return;
    }
    uint8_t* sc        = con_env ? scratch : NULL;
    char*    reply     = (char*)  (con_env ? scratch + BP_ENV_SECTOR : scratch);
    size_t   reply_cap = (size_t) (con_env ? scratch_len - BP_ENV_SECTOR : scratch_len);

    uint8_t* a = NULL;
    uint8_t* b = NULL;
    if (con_env) {
        /* Contiguas y en ese orden: `b` cuelga de `a`, como cuando las dos salían
         * del mismo buffer. Una sola petición = un solo dueño que soltar. */
        a = (uint8_t*) bpvm_scratch_take(2u * (size_t) BP_ENV_SECTOR, "bmgr-env");
        if (a == NULL) {
            wire_v1_send_error(id, "INTERNAL_ERROR",
                               "zona de scratch no disponible para el entorno");
            return;
        }
        b = a + BP_ENV_SECTOR;
        /* Lee las dos copias del env desde flash (XIP) a la zona prestada. */
        memcpy(a, (const void*)(XIP_BASE + BP_ENV_A_OFFSET), BP_ENV_SECTOR);
        memcpy(b, (const void*)(XIP_BASE + BP_ENV_B_OFFSET), BP_ENV_SECTOR);
    }

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

    /* V5/H8 — la RAM de trabajo de un motor nativo: el PRINCIPIO del bloque de
     * la BD (`[estaticos | arena]`). Ya lo calcula el arranque; aquí sólo se
     * expone para que `PACK_BURN_BEGIN` pueda decírselo al IDE, que es quien
     * relocaliza. Si no hay bloque, queda a 0 y el IDE lo dirá. */
    bm.pack_ram_base = (uint32_t) (uintptr_t) s_pack_ram_base;
    bm.pack_ram_size = s_sqlite_size;

    bpvm_bmgr_req_t req;
    memset(&req, 0, sizeof req);
    snprintf(req.type, sizeof req.type, "%s", type);
    req.id = id;
    req.has_key   = json_get_str(obj, "key",   req.key,   sizeof req.key)   >= 0;
    req.has_value = json_get_str(obj, "value", req.value, sizeof req.value) >= 0;
    for (int i = 0; i < BPVM_PART_COUNT; i++)
        req.part_sizes[i] = json_get_long(obj, bpvm_part_name((bpvm_part_kind_t) i), -1);

    /* H3 — los campos que piden los PACK_*. De los seis, el Pico rellenaba SOLO
     * `bulk`: sin `off` no van PACK_ENTRIES/DEL/READ, sin `size` no arranca el
     * BURN_BEGIN, y sin `confirm_yes` el FORMAT rechaza aunque el IDE mande el
     * confirm (que lo manda). Encaminar los verbos y no rellenar su petición es
     * la misma media función que dejar bm.packs_* a cero. Copiado del STM32,
     * que es el que estaba completo. */
    req.off      = json_get_long(obj, "offset", -1);   /* PACK_ENTRIES/DEL/READ */
    req.has_off  = req.off >= 0;
    req.size     = json_get_long(obj, "size", -1);     /* PACK_BURN_BEGIN */
    req.has_size = req.size >= 0;
    req.bulk     = bulk;                               /* PACK_BURN_DATA (ya recibido) */
    req.bulk_len = (long) bulk_len;
    {                                                  /* PACK_FORMAT (confirm=YES) */
        char confirm[8];
        req.confirm_yes = json_get_str(obj, "confirm", confirm, sizeof confirm) >= 0
                          && strcmp(confirm, "YES") == 0;
    }

    int wrote = -1;
    int n = bpvm_bmgr_wire_dispatch(&bm, &req, reply, reply_cap, &wrote);
    /* La zona se suelta en TODAS las salidas: quedársela colgada dejaría mudos
     * los PACK_* y los siguientes comandos del entorno. Y se suelta DESPUÉS del
     * env_write, porque el sector que hay que volcar a flash vive en ella. */
    if (n < 0) {
        if (con_env) bpvm_scratch_give("bmgr-env");
        wire_v1_send_error(id, "INTERNAL_ERROR", "reply de gestion no cabe");
        return;
    }
    if (wrote >= 0) env_write(wrote, wrote == 0 ? a : b);   /* la cintura: RAM → flash */
    if (con_env) bpvm_scratch_give("bmgr-env");
    wire_v1_send_line(reply, (size_t) n);
}
