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

#include "bpvm.h"              /* #338: bpvm_scratch_take/give (zona compartida) */
#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_repl.h"           /* V7/H10: el handle común (bpvm_bmgr_handle) */
#include "bpvm_pack.h"           /* H3 — cintura de burn de la zona de packs */
#include "bpvm_part.h"
#include "bpvm_env.h"
#include "stm32_wire.h"          /* el cable; los builders vienen del contrato comun */
#include "stm32_fs.h"            /* fs_init_at */
#include "stm32_flash.h"         /* stm32_flash_write / erase (compartidas con el FS) */
#include "flash_layout_stm32.h"  /* BP_ENV_A/B_OFFSET, BP_PART_BASE, BP_USABLE_FLASH, símbolos .ld */
#include "pack_stm32.h"          /* V7/H6 G5.b0: el bloque de RAM de los motores (PACK_BURN_BEGIN) */
#include "log.h"                 /* V7/H7: el gancho del montaje dice por qué no hay zona */

#include "main.h"                /* FLASH_BASE */
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Estado del boot + el layout del climb (estáticos: el layout debe sobrevivir para
 * que layer_fs lo use; el boot lo lee board_boot_status). s_layout_ok = el layout
 * validó en el boot → la zona de packs (BPVM_PART_PACKS) es direccionable (H3).
 * V7/H10 (el board_mgr común, D1): el ENV ya no se copia a la RAM —eran 16 KB de .bss—: la flash del U5 está mapeada
 * y la vista común (bpvm_bmgr_vista) lee las copias en su sitio. */
static bpvm_boot_status_t s_boot;
static bpvm_part_layout_t s_layout;
static int s_layout_ok = 0;

const bpvm_boot_status_t* board_boot_status(void) { return &s_boot; }

/* ── cintura del env: las 2 copias A/B (flash interna mapeada) ── */

static int env_write_slot(int slot, const uint8_t* src) {
    uint32_t addr = FLASH_BASE + ((slot == 0) ? BP_ENV_A_OFFSET : BP_ENV_B_OFFSET);
    if (stm32_flash_erase(addr, 1u) != 0                 /* 1 página de 8 KB */
        || stm32_flash_write(addr, src, BP_ENV_SECTOR) != 0) {
        log_printf("env: la flash no escribe el sector %d del env: no se ha escrito", slot);
        return -1;
    }
    return 0;
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

/* ── V7/H10, el board_mgr común (3-oct): LA CINTURA DE LA STM32 (bpvm_bmgr_familia_t) ──
 * Lo que quedaba de propio en su `handle`: dónde están las copias (la flash mapeada), cómo se escribe un sector, lo de
 * la placa en el gestor y la red .ld ↔ flash_layout_stm32.h —que desde #637 también está DENTRO de stm32_flash.c, para
 * todo lo que escribe: aquí sólo da el motivo por el wire—. */
static const uint8_t* env_copia(int slot) {
    return (const uint8_t*) (uintptr_t) (FLASH_BASE + ((slot == 0) ? BP_ENV_A_OFFSET : BP_ENV_B_OFFSET));
}

static void env_rellenar(bpvm_bmgr_t* bm) {
    bm->part_base = BP_PART_BASE;
    bm->usable_flash = BP_USABLE_FLASH;   /* tamaño de flash fijo por placa; sin clamp */
    bm->live = &s_boot;                   /* STATE cuenta el estado REAL del boot */
    /* H3 — zona de packs: XIP directo sobre la partición PACKS. V7/H7 (Boot 2, fase 1): la que ve el gestor es la
     * MONTADA —con el FS, la regla del arranque—, no la del layout: sin FS no hay zona, y PACK_LS dice por qué. */
    uint32_t tam = 0;
    const uint8_t* base = bpvm_pack_mounted(&tam);
    if (base && tam > 0) {
        bm->packs_base = base;
        bm->packs_size = tam;
        s_packs_off    = (uint32_t) ((uintptr_t) base - FLASH_BASE);   /* la cintura de burn escribe aquí */
        bm->packs_flash = &s_packs_fl;
    }
    /* V7/H6 G5.b0 — la RAM de trabajo de un motor nativo: el bloque que el arranque apartó delante de la VM
     * (pack_stm32.h). Sale a PACK_BURN_BEGIN; sin bloque, 0, y el IDE lo dice en vez de grabar un motor que no
     * podría arrancar. */
    bm->pack_ram_base = (uint32_t) (uintptr_t) s_pack_ram_base;
    bm->pack_ram_size = s_pack_ram_size;
}

static const char* env_no_vale(void) {
    /* Red anti-divergencia .ld ↔ flash_layout_stm32.h: el env debe caer entre el fin del firmware y el inicio de los
     * datos. La guarda de verdad está en stm32_flash.c (#637); esto la dice por el wire, con su nombre. */
    if ((uintptr_t) (FLASH_BASE + BP_ENV_A_OFFSET) < (uintptr_t) __bp_flash_end
        || (uintptr_t) (FLASH_BASE + BP_ENV_B_OFFSET + BP_ENV_SECTOR) > (uintptr_t) __bp_part_start)
        return "layout de flash inconsistente (env fuera de la zona 2)";
    return NULL;
}

static const bpvm_bmgr_familia_t s_familia = {
    BP_ENV_SECTOR, env_copia, NULL, NULL, env_write_slot, env_rellenar, env_no_vale
};

/* La familia, registrada antes de la primera lectura del ENV: la vista común sin familia es un ENV vacío, y cualquier
 * clave saldría con su valor por defecto sin decir nada. Quien lea primero, la registra. */
static void asegura_familia(void) {
    if (bpvm_bmgr_familia_actual() != &s_familia) bpvm_bmgr_familia(&s_familia);
}

/* ── arranque escalonado: particiones del env → FS (sub-rango de BP_DATA) → VM ── */

/* #423 — leer una bandera del entorno desde fuera. El STM32 guardaba sus slots
 * en estaticos y no los publicaba: nadie de fuera habia necesitado preguntar.
 * Lo necesita el interruptor del log (`log=0|1`), y se resuelve con esto en vez
 * de sacar los slots, que es estado que no tiene por que salir de aqui. */
int board_mgr_stm32_env_bool(const char* key, int def) {
    asegura_familia();
    return bpvm_env_get_bool(bpvm_bmgr_vista(), key, def);   /* V7/H10 (D1): el ENV de la flash */
}

/* Lo mismo para una clave numerica (hoy: `stack`, el reparto pilas/heap en KB). */
long board_mgr_stm32_env_long(const char* key, long def) {
    asegura_familia();
    return bpvm_env_get_long(bpvm_bmgr_vista(), key, def);
}

/* V7/H6.c G4.a1 — y una clave de TEXTO, entera o nada (`bpvm_env_get_exacto`): la
 * primera que hace falta aquí son las líneas de los buses (`spi1=`, `i2c2=`). */
int board_mgr_stm32_env_get(const char* key, char* buf, size_t cap) {
    asegura_familia();
    return bpvm_env_get_exacto(bpvm_bmgr_vista(), key, buf, cap);
}

/* V7/H6 G5.b0 — ver el .h: el ENV antes que el climb, para repartir la memoria. V7/H10: ya no se copia nada; aquí se
 * crea el candado de la flash (D4: antes de que corra nadie más) y se registra la familia. */
void board_mgr_stm32_env_temprano(void) {
    stm32_flash_init();
    asegura_familia();
}

/* V7/H7 (Boot 2, fase 4) — el ENV ENTERO, de sólo lectura, para el arranque común (las claves del reparto, los buses,
 * `log=`) y Machine.getEnv. V7/H10 (D1): el de la flash —lo último que se escribió—, no una foto del arranque. */
const bpvm_env_t* board_mgr_stm32_env(void) {
    asegura_familia();
    return bpvm_bmgr_vista();
}

/* V7/H10 (#626) — el arranque común AÑADE una clave que falta (la fecha de la imagen, una vez), por el común: el
 * bloque nuevo en la zona de rascar, por la cintura (con la guarda de #637), y la vista lo ve sin reiniciar. */
int board_mgr_stm32_env_anadir(const char* clave, const char* valor) {
    asegura_familia();
    return bpvm_bmgr_anadir(clave, valor);
}

static bpvm_boot_step_t layer_partitions(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.ok = 0; r.reason[0] = '\0';
    asegura_familia();
    const bpvm_env_t* env = bpvm_bmgr_vista();   /* V7/H10: el ENV de la flash */
    int bad = -1;
    /* base = BP_PART_BASE, usable = fin de flash (fijo por placa; sin clamp #292
     * — el U5 conoce su tamaño de flash en compilación, no hay sorpresa JEDEC). */
    bpvm_part_err_t e = bpvm_part_layout(env, BP_PART_BASE, BP_USABLE_FLASH,
                                         BP_ENV_SECTOR, &s_layout, &bad);
    if (e == BPVM_PART_OK) {
        char l[160];   /* V7/H10: de dónde sale el reparto, y cómo queda (el TOTAL del FS bailaba el 26-ago) */
        if (bpvm_part_describe(&s_layout, "del ENV (part.fs.size)", l, sizeof l) > 0) log_printf("%s", l);
        s_layout_ok = 1;
        r.ok = 1;
        return r;
    }
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
}

/* H3.c — montar la zona de packs (XIP) para la resolución de módulos del RUN:
 * el último candidato tras /app y /lib. El mismo montaje que hace el host con
 * --pack=, aquí sobre la flash real.
 *
 * [V7/H2, #493] Vivía dentro de `board_mgr_stm32_boot`, ANTES de que el repl
 * sincronizara /lib desde la tabla embebida — comodidad de sitio del 23-jul,
 * inocua hasta que el 13-ago (#362) la zona empezó a contestar al `stat` de
 * cualquier ruta: el sync veía «/lib/Core.mod» en la zona, no podía leerle el
 * CRC y lo REPONÍA cada arranque con «no se pudo leer»; con un pack de MAGIC
 * mayor, /lib no se instalaba nunca. Las otras tres familias sincronizan antes
 * de montar; ahora ésta también.
 *
 * V7/H6 (#468): al revés otra vez, y en todas las familias: la zona se monta ANTES
 * de la stdlib (stm32_repl.c), porque el instalador —`bpvm_mods_sincronizar`— le
 * pregunta qué sirve ya con los mismos bytes que la imagen y no copia a /lib lo que
 * el pack sirve (esa copia idéntica lo tapaba). */
/* V7/H6 G5.b0 — la partición de packs del layout del boot, para el cargador. */
const bpvm_part_t* board_mgr_stm32_packs(void) {
    if (!s_layout_ok) return NULL;
    const bpvm_part_t* pp = bpvm_part_get(&s_layout, BPVM_PART_PACKS);
    return (pp && pp->size > 0) ? pp : NULL;
}

/* V7/H7 (Boot 2, fase 1) — el GANCHO de la familia para bpvm_pack_montar_arranque: dónde
 * ve la CPU la zona. 1 = montada; 0 = no hay zona (y lo dice). La regla —con el FS— y el
 * estado de la zona los pone el común. */
int board_mgr_stm32_montar_packs(void) {
    const bpvm_part_t* pp = s_layout_ok ? bpvm_part_get(&s_layout, BPVM_PART_PACKS) : NULL;
    if (!pp || pp->size == 0) {
        log_printf("packs: sin zona de packs (no hay particion de packs en el reparto)");
        return 0;
    }
    /* La flash del STM32 esta mapeada: la misma direccion sirve para leer
     * y para ejecutar, asi que va dos veces. */
    bpvm_pack_mount((const uint8_t*) (uintptr_t) (FLASH_BASE + pp->offset),
                    (const uint8_t*) (uintptr_t) (FLASH_BASE + pp->offset),
                    pp->size);
    return 1;
}

/* ── ramo del wire (STATE/ENV_x/PART_x): el repl encamina aquí ── */

/* #338 — la zona compartida tiene que dar para las DOS copias del env de ESTA
 * familia. Si alguien cambia el sector de borrado (o porta a un micro con uno
 * mayor) sin subir BPVM_SCRATCH_BYTES, aqui no compila — en vez de descubrirlo
 * en placa como un "zona de scratch no disponible" al abrir el panel. */
typedef char bp_chk_scratch_env[(BPVM_SCRATCH_BYTES >= 2u * BP_ENV_SECTOR) ? 1 : -1];

void board_mgr_stm32_handle(long id, const json_obj_t* obj, const char* type,
                            unsigned char* scratch, unsigned long scratch_len,
                            const unsigned char* bulk, unsigned long bulk_len) {
    /* V7/H10, el board_mgr común (3-oct): lo que había aquí —el reparto del búfer, las copias del ENV en la zona de
     * rascar, el gestor, la petición desde el JSON, despachar y volcar— es el `handle` común (bpvm_repl.c), el mismo de
     * las otras familias y del sim; lo propio de la STM32 es su cintura (s_familia, arriba). Y si la flash no escribe,
     * FLASH_ERROR en vez del OK (D2, #636). */
    asegura_familia();
    bpvm_bmgr_handle(id, obj, type, scratch, scratch_len, bulk, bulk_len);
}
