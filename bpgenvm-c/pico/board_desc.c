/*
 * board_desc.c — H7.3: implementación del descriptor de placa.
 *
 * Defaults por variante (tabla del CHIP) + override desde /sys/board.json
 * (datos de la PLACA). Ver board_desc.h para el porqué de la separación.
 */
#include "board_desc.h"
#include "fs.h"
#include "json_min.h"
#include "log.h"
#include "bpvm_fs.h"   /* #305 — lectura directa, sin pasar por el scratch */
#include "psram.h"
#include "hardware/flash.h"   /* flash_do_cmd (JEDEC ID) */
#include "hardware/sync.h"    /* save_and_disable_interrupts (#256) */
#include "hardware/structs/sysinfo.h"  /* SYSINFO.PACKAGE_SEL → variante A/B */

#include <stdint.h>
#include <string.h>

/* Tamaño de la flash QSPI leyendo su JEDEC ID (0x9F). El 3er byte de respuesta
 * es la capacidad como log2(bytes): 0x15=2MB, 0x16=4MB, 0x17=8MB, 0x18=16MB.
 *
 * IRQs OFF obligatorio (#256 post-mortem): flash_do_cmd corre desde RAM y
 * suspende el XIP, pero NO apaga interrupciones — si el tick de FreeRTOS o
 * la IRQ del USB (handlers en flash) saltan dentro de esa ventana, el fetch
 * desde flash con XIP suspendido es hard fault y la placa muere ANTES de
 * enumerar ("dispositivo desconocido"). El boot es determinista, así que no
 * era intermitente: cargar el FS persistido desplazaba la fase del tick
 * justo encima de la ventana y mataba el boot SIEMPRE; con FS vacío fallaba
 * la alineación y vivía SIEMPRE. Misma disciplina que fs.c/psram.c. */
static unsigned detect_flash_bytes(void) {
    uint8_t tx[4] = { 0x9Fu, 0, 0, 0 };
    uint8_t rx[4] = { 0 };
    uint32_t intr_stash = save_and_disable_interrupts();
    flash_do_cmd(tx, rx, 4);
    restore_interrupts(intr_stash);
    uint8_t cap = rx[3];
    if (cap < 0x10u || cap > 0x1Bu) return 0;   /* 64KB..2GB: fuera = sospechoso */
    return 1u << cap;
}

/* H2-B2 - el descriptor de particiones necesita el tamano de flash ANTES de
 * board_desc_init (que a su vez lee board.json DEL FS -> huevo y gallina).
 * Wrapper publico del probe JEDEC, sin dependencias del FS. */
unsigned board_desc_probe_flash_bytes(void) {
    return detect_flash_bytes();
}

static board_desc_t s_board;

/*
 * Tabla de caps por variante del CHIP (NO de la placa). Es lo único que
 * el firmware "sabe" sobre el RP2350A/B. Todo lo demás (qué pin es el LED,
 * el NeoPixel, etc.) viene de board.json.
 *
 * psram_cs_pin se inicializa aquí como DEFAULT por variante (Adafruit usa
 * GPIO0 en RP2350A y el último pin —GPIO47— en RP2350B); board.json lo
 * sobreescribe si la placa lo cablea distinto. (Pines a confirmar con el
 * esquemático del Metro.)
 */
static void apply_variant_caps(board_desc_t* d, char variant) {
    if (variant == 'B') {
        d->variant      = 'B';
        d->gpio_count   = 48;   /* RP2350B (QFN-80), Metro */
        d->pio_count    = 3;
        d->pwm_slices   = 12;
        d->adc_channels = 8;
        d->psram_cs_pin = -1;   /* NO asumimos CS de PSRAM por variante: el
                                 * sondeo QMI es boot-crítico, así que es OPT-IN
                                 * explícito — sólo se sondea si board.json
                                 * declara psramCsPin (Metro/Pimoroni: 47). */
    } else {                    /* 'A' por defecto */
        d->variant      = 'A';
        d->gpio_count   = 30;   /* RP2350A (QFN-60), Pico 2 */
        d->pio_count    = 3;
        d->pwm_slices   = 12;
        d->adc_channels = 4;
        d->psram_cs_pin = -1;   /* la mayoría de placas A no llevan PSRAM; las
                                 * que sí (CS=GP0) lo declaran en board.json. Así
                                 * un Pico no sondea GP0 (que es UART TX). */
    }
}

/* H9 — identidad en el SUELO (sin FS, sin env: hardware puro). Se llama la
 * PRIMERA del boot, antes de particiones/FS/heap. */
void board_desc_early_init(void) {
    board_desc_t* d = &s_board;
    memset(d, 0, sizeof *d);

    /*
     * Variante desde HARDWARE: SYSINFO.PACKAGE_SEL (RO, 1 bit). Mapeo CONFIRMADO
     * en placa (19-jul, Pico 2 = RP2350A lee 1): **1 = QFN-60 (RP2350A, Pico 2),
     * 0 = QFN-80 (RP2350B, Metro)** — al revés de lo que sugería el reset=0; el
     * chip lo fija por su package. INDEPENDIENTE del FS → sobrevive a un borrado
     * de flash. board.json puede seguir forzando la variante (placas atípicas).
     */
    strncpy(d->name, "rp2350-generic", sizeof d->name - 1);
    d->package_sel = (int) (sysinfo_hw->package_sel & 1u);
    apply_variant_caps(d, d->package_sel ? 'A' : 'B');
    d->led_pin       = -1;   /* lo declara la placa (board.json, estado >= 2) */
    d->neopixel_pin  = -1;   /* peculiar de cada placa */
    d->psram_present = 0;    /* lo decide board_desc_psram_from_env (H9, env) */
    d->psram_bytes   = 0;

    /* Tamaño de flash (JEDEC). Seguro (flash_do_cmd hace XIP-suspend). Con el
     * define a 16 MB (sobre máximo), el JEDEC es EL límite real de escritura:
     * si no se reconoce, asumir 4 MB conservador (nunca más de lo seguro). */
    d->flash_bytes = detect_flash_bytes();
    if (d->flash_bytes == 0) {
        d->flash_bytes = 4u * 1024u * 1024u;
        log_printf("board: JEDEC no reconocido -> asumo flash 4 MB (conservador)");
    }
    log_printf("board: variante %c (PACKAGE_SEL=%d) flash=%u MB",
               d->variant, d->package_sel, d->flash_bytes / (1024u * 1024u));
}

/* H9 — PSRAM conducida por el ENV (`psram=1`), no por board.json. Decisión de
 * Eduardo (19-jul): solo RP2350B, CS = GPIO47 (el último pin, el estándar de
 * las placas B); en la A no se sondea — no se conoce placa A con PSRAM y el
 * sondeo QMI es boot-crítico. Si aparece una, se añade la posibilidad.
 * detect → enable QPI → auto-test RW; psram_present sólo si los TRES pasan
 * (= PSRAM USABLE para el heap). */
void board_desc_psram_from_env(int psram_flag) {
    board_desc_t* d = &s_board;
    if (!psram_flag) {
        log_printf("psram: no activada por env (psram!=1)");
        return;
    }
    if (d->variant != 'B') {
        log_printf("psram: env la pide pero la variante es %c (solo RP2350B)", d->variant);
        return;
    }
    d->psram_cs_pin = 47;   /* último GPIO de la RP2350B */
    size_t sz = psram_detect_init(d->psram_cs_pin);
    unsigned mb = (unsigned)(sz / (1024u * 1024u));
    if (sz == 0) {
        log_printf("psram: no detectada en GP%d", d->psram_cs_pin);
    } else if (!psram_enable_xip()) {
        log_printf("psram: detectada (%u MB) pero enable QPI FALLO", mb);
    } else if (!psram_rw_selftest(sz)) {
        log_printf("psram: detectada (%u MB) + QPI, pero RW test FALLO", mb);
    } else {
        d->psram_present = 1;
        d->psram_bytes   = (unsigned) sz;
        log_printf("psram: %u MB usable @ GP%d (QPI + RW OK, ventana 0x%08x)",
                   mb, d->psram_cs_pin, (unsigned) PSRAM_XIP_BASE);
    }
}

/* Overrides de /sys/board.json — datos de la PLACA que aún viven en el FS.
 * Llamar SOLO con el FS montado (estado >= 2). La variante base, la flash y
 * la PSRAM ya NO salen de aquí (suelo + env, H9); psramCsPin se IGNORA. */
void board_desc_init(void) {
    board_desc_t* d = &s_board;
    /* #305 — board.json es un puñado de campos: cabe de sobra en la pila y no
     * hay razón para pasar por el scratch del FS. Si algún día no cupiese, se
     * dice por el log en vez de parsear medio JSON y quedarse tan tranquilo con
     * los defaults (un truncado silencioso aquí = una placa mal descrita). */
    uint8_t data[512];
    uint32_t size = 0;
    if (bpvm_fs_stat("/sys/board.json", &size) == 0 && size > sizeof data) {
        log_printf("board.json: %u B no caben en %u — se ignora",
                   (unsigned) size, (unsigned) sizeof data);
        size = 0;
    } else {
        long n = bpvm_fs_read("/sys/board.json", data, sizeof data);
        size = (n > 0) ? (uint32_t) n : 0;
    }
    if (size > 0) {
        json_obj_t obj;
        if (json_parse((const char*) data, (size_t) size, &obj) == 0) {
            /* variant primero: re-aplica la tabla de caps de la variante. */
            char vbuf[4] = {0};
            if (json_get_str(&obj, "variant", vbuf, sizeof vbuf) >= 0 && vbuf[0]) {
                apply_variant_caps(d, (vbuf[0] == 'b' || vbuf[0] == 'B') ? 'B' : 'A');
            }
            json_get_str(&obj, "name", d->name, sizeof d->name);
            d->led_pin      = (int) json_get_long(&obj, "ledPin",     d->led_pin);
            d->neopixel_pin = (int) json_get_long(&obj, "neopixelPin", d->neopixel_pin);
            /* gpioCount: override explícito de la tabla (placas atípicas). */
            d->gpio_count   = (int) json_get_long(&obj, "gpioCount",  d->gpio_count);
            log_printf("board: /sys/board.json aplicado");
        } else {
            log_printf("board: /sys/board.json invalido, uso defaults");
        }
    } else {
        log_printf("board: sin /sys/board.json, uso defaults por variante");
    }

    /* «pwm=N slices», con la unidad DICHA. El INFO responde 24 para esta misma
     * placa —SALIDAS: cada slice tiene canales A y B— y quien pusiera las dos
     * lineas una al lado de otra veia una contradiccion y se iba a buscarla.
     * Las dos cifras son correctas; lo que faltaba era decir de que. */
    log_printf("board: %s variant=%c gpio=%d pio=%d pwm=%d slices adc=%d led=%d npx=%d psram=%uMB",
               d->name, d->variant, d->gpio_count, d->pio_count, d->pwm_slices,
               d->adc_channels, d->led_pin, d->neopixel_pin,
               d->psram_bytes / (1024u * 1024u));
}

const board_desc_t* board_desc(void) { return &s_board; }
