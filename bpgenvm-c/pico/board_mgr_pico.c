/*
 * board_mgr_pico.c — H9: adaptador del firmware para la gestión de placa (RP2350).
 * Cintura de flash del bloque de env (A/B) + puente al núcleo compartido
 * bpvm_bmgr_wire (mismo que el boardsim de host → replies byte-idénticas). Ver .h.
 */
#include "board_mgr_pico.h"

#include "bpvm.h"        /* #338: bpvm_scratch_take/give (zona de rascar compartida) */
#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_repl.h"   /* V7/H10: el handle común (bpvm_bmgr_handle) */
#include "bpvm_part.h"
#include "bpvm_pack.h"   /* #327: cintura de flash de la zona de packs */
#include "wire_v1.h"
#include "pico_flash.h"  /* V7/H7 U1.4: la cintura de flash (el candado va dentro) */
#include "log.h"
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
    return pico_flash_erase(s_packs_off + off, len);   /* V7/H7 U1.4: la cintura, con el candado */
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
        if (pico_flash_write(base, page, FLASH_PAGE_SIZE) != 0) return -1;   /* U1.4: la cintura */
        done += run;
    }
    return 0;
}

static const bpvm_pack_flash_t s_packs_fl =
    { packs_fl_erase, packs_fl_program, NULL, FLASH_SECTOR_SIZE /* 4K */ };

/* Vuelca a flash un sector A/B (borra + programa el sector entero de 4K, bajo la
 * ventana XIP-safe). El otro sector queda intacto (A/B). `src` = el buffer RAM.
 * 0 escrito, -1 no (y lo dice). */
static int env_write(int slot, const uint8_t* src) {
    uint32_t off = (slot == 0) ? BP_ENV_A_OFFSET : BP_ENV_B_OFFSET;
    /* V7/H7 U1.4 — por la cintura: dos ventanas del candado en vez de una (borrar y
     * programar), y da igual: el A/B deja la otra copia buena si algo corta en medio. */
    if (pico_flash_erase(off, BP_ENV_SECTOR) != 0
        || pico_flash_write(off, src, BP_ENV_SECTOR) != 0) {   /* 4096 = múltiplo de FLASH_PAGE_SIZE */
        log_printf("env: la cintura de flash RECHAZÓ el sector %d (0x%05lX): no se ha escrito",
                   slot, (unsigned long) off);
        return -1;
    }
    return 0;
}

/* Red anti-divergencia bp_memmap.ld ↔ flash_layout.h: los offsets del env deben caer
 * DENTRO del hueco real (zona 2, entre FLASH_BOOT y FLASH_MAIN). Si alguien mueve las
 * regiones del linker sin tocar el header (o al revés), tocar flash borraría CÓDIGO →
 * rechazar a gritos, nunca escribir. 1 = el env cae fuera (no se escribe). */
static int env_fuera_de_su_zona(void) {
    return (uintptr_t)(XIP_BASE + BP_ENV_A_OFFSET) < (uintptr_t) __bp_zone1_end
        || (uintptr_t)(XIP_BASE + BP_ENV_B_OFFSET + BP_ENV_SECTOR) > (uintptr_t) __bp_zone3_start;
}

/* ── V7/H10, el board_mgr común (3-oct): LA CINTURA DE LA PICO (bpvm_bmgr_familia_t) ──
 * Lo que quedaba de propio en su `handle` y en su `env_anadir`: las copias se leen por XIP (sin copia en RAM: la Pico
 * se ahorra esos 8 KB), un sector se escribe por la cintura de flash (con su guarda y su candado), lo de la placa en el
 * gestor y la red bp_memmap.ld ↔ flash_layout.h. Lo demás —el `handle`, la vista del ENV y el `anadir`— es común. */
static const uint8_t* env_copia(int slot) {
    return (const uint8_t*) (XIP_BASE + (slot == 0 ? BP_ENV_A_OFFSET : BP_ENV_B_OFFSET));
}

static void env_rellenar(bpvm_bmgr_t* bm) {
    const board_desc_t* bd = board_desc();
    bm->part_base = BP_PART_BASE;
    /* usable CON clamp #292 (= el mismo número que usa el boot): desde la unificación el plan SE EJECUTA (el mount sale
     * de aquí), así que proponer más de lo escribible sería ofrecer un layout que el arranque rechaza. */
    bm->usable_flash = bpvm_part_usable_flash(bd ? bd->flash_bytes : 0u, PICO_FLASH_SIZE_BYTES);
    bm->live = board_boot_status();   /* STATE cuenta el estado REAL alcanzado por el boot (no el plan del env) */
    /* #327 — zona de packs: XIP directo sobre la partición PACKS. V7/H7 (Boot 2, fase 1): la MONTADA —con el FS, la
     * regla del arranque—, no la del layout. Sin zona, el LS dice por qué. */
    uint32_t tam = 0;
    const uint8_t* base = bpvm_pack_mounted(&tam);
    if (base && tam > 0) {
        bm->packs_base  = base;
        bm->packs_size  = tam;
        s_packs_off     = (uint32_t) ((uintptr_t) base - XIP_BASE);   /* la cintura de burn escribe aquí */
        bm->packs_flash = &s_packs_fl;
    }
    /* V5/H8 — la RAM de trabajo de un motor nativo: el PRINCIPIO del bloque de la BD. Ya lo calcula el arranque; aquí
     * se expone para que PACK_BURN_BEGIN se lo diga al IDE. Si no hay bloque, 0, y el IDE lo dirá. */
    bm->pack_ram_base = (uint32_t) (uintptr_t) s_pack_ram_base;
    bm->pack_ram_size = s_pack_ram_size;   /* V7/H6.c G1: el bloque que empieza ahí */
}

static const char* env_no_vale(void) {
    return env_fuera_de_su_zona() ? "layout de flash inconsistente (env fuera de la zona 2)" : NULL;
}

static const bpvm_bmgr_familia_t s_familia = {
    BP_ENV_SECTOR, env_copia, NULL, NULL, env_write, env_rellenar, env_no_vale
};

void board_mgr_pico_familia(void) {
    if (bpvm_bmgr_familia_actual() != &s_familia) bpvm_bmgr_familia(&s_familia);
}

/* V7/H10 (#626) — el arranque común AÑADE una clave que falta (la fecha de la imagen, una vez), por el común: el
 * bloque nuevo en la zona de rascar (libre en el arranque), por la cintura, y la vista lo ve sin reiniciar. */
int board_mgr_pico_env_anadir(const char* clave, const char* valor) {
    board_mgr_pico_familia();
    return bpvm_bmgr_anadir(clave, valor);
}

/* #338 — la zona compartida tiene que dar para las DOS copias del env de ESTA
 * familia. Si alguien cambia el sector de borrado (o porta a un micro con uno
 * mayor) sin subir BPVM_SCRATCH_BYTES, aqui no compila — en vez de descubrirlo
 * en placa como un "zona de scratch no disponible" al abrir el panel. */
typedef char bp_chk_scratch_env[(BPVM_SCRATCH_BYTES >= 2u * BP_ENV_SECTOR) ? 1 : -1];

void board_mgr_pico_handle(long id, const json_obj_t* obj, const char* type,
                           unsigned char* scratch, unsigned long scratch_len,
                           const unsigned char* bulk, unsigned long bulk_len) {
    /* V7/H10, el board_mgr común (3-oct): lo que había aquí es el `handle` común (bpvm_repl.c), el mismo de las otras
     * familias y del sim; lo propio de la Pico es su cintura (s_familia, arriba). Y si la flash no escribe,
     * FLASH_ERROR en vez del OK (D2, #636). */
    board_mgr_pico_familia();
    bpvm_bmgr_handle(id, obj, type, scratch, scratch_len, bulk, bulk_len);
}
