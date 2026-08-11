/*
 * blk_sdmmc_p4.c — V5/H6 paso 2: la cintura SDMMC del ESP32-P4.
 *
 * Implementa `bpvm_blk_backend_t` (el contrato de bloque, H6 paso 1) sobre el
 * `sdmmc_host` del ESP-IDF. Encima de esto NO cambia nada: el mismo FatFs, la
 * misma fachada `bpvm_fs` y los mismos verbos que ya funcionan en la Metro.
 *
 * ─── LO QUE NO SE USA, Y POR QUÉ ────────────────────────────────────────────
 *
 * El IDF trae `esp_vfs_fat_sdmmc_mount()`, que monta la tarjeta en cuatro
 * líneas. No se usa: arrastra el FatFs DEL IDF, y tendríamos DOS motores con
 * dos configuraciones divergiendo sin avisar — la misma familia de problema que
 * las copias rancias de la stdlib. Aquí sólo se toma la capa de sectores.
 *
 * ─── ⚠️ LA TRAMPA QUE ESTABA ESPERANDO EN `SDMMC_SLOT_CONFIG_DEFAULT()` ─────
 *
 * Esa macro, en su variante de ESP32-P4, trae:
 *
 *     .d4 = GPIO_NUM_45 ... .d7 = GPIO_NUM_48
 *     .width = SDMMC_SLOT_WIDTH_DEFAULT     (== 0 == "el máximo de la ranura")
 *
 * Y en ESTA placa **el GPIO45 es el que enciende la alimentación de la
 * tarjeta** (MOSFET Q1, ver el esquema). Con la macro tal cual, el periférico
 * reclamaría el 45 como línea de datos D4 y el raíl de la SD quedaría a merced
 * del tráfico del bus. El fallo no sería limpio: la tarjeta se apagaría y
 * encendería a mitad de transferencia.
 *
 * Por eso la configuración se construye **campo a campo**, con `d4..d7` a
 * `GPIO_NUM_NC` y el ancho EXPLÍCITO. Cuesta seis líneas y evita un fallo que
 * habría costado días — y que además sólo aparecería con la tarjeta trabajando,
 * no al montar.
 *
 * ─── LOS PULL-UPS SON EXTERNOS ──────────────────────────────────────────────
 *
 * No se pone `SDMMC_SLOT_FLAG_INTERNAL_PULLUP`: la placa lleva R5-R10 de 51 K.
 * El propio IDF avisa de que los internos son insuficientes y valen sólo para
 * depurar. Si algún día hiciera falta activarlos, eso NO sería un arreglo: sería
 * un hallazgo sobre la placa, y tocaría bajar el reloj y anotarlo.
 */
#include "bpvm_blk_sdmmc.h"

#include "driver/sdmmc_host.h"
/* Sin prefijo `driver/`: este vive en el componente `sdmmc`, no en
 * `esp_driver_sdmmc` como su vecino `driver/sdmmc_host.h`. Lo mismo que
 * `sdmmc_cmd.h` de abajo, y lo mismo que escribe el ejemplo del IDF. */
#include "sd_pwr_ctrl_by_on_chip_ldo.h"   /* el LDO que alimenta las E/S */
#include "driver/gpio.h"
#include "sdmmc_cmd.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

/* ── Estado: una tarjeta, como en el resto de la casa. ────────────────────── */
static bpvm_sdio_pines_t s_p;
static int          s_configurado = 0;
static int          s_arrancada   = 0;
static sdmmc_card_t s_card;
static esp_err_t    s_ultimo      = ESP_OK;
/* El LDO interno de las E/S. Se crea una vez y se conserva (ver p4_init 2.a). */
static sd_pwr_ctrl_handle_t s_ldo = NULL;

/* Reloj por defecto si el ENV no dice otra cosa. CONSERVADOR a propósito: los
 * pull-ups de esta placa son de 51 K, que es flojo para ir rápido, y un bus
 * marginal no falla al montar — falla a ratos y con la placa caliente. Subir
 * esto es una decisión que se toma MIDIENDO (el patrón conocido de ida y
 * vuelta), no por optimismo. */
#define SDIO_KHZ_POR_DEFECTO  20000

/* ── La alimentación de la TARJETA: un GPIO ───────────────────────────────
 *
 * El VDD del zócalo sale de 3V3 conmutado por un MOSFET cuya puerta manda el
 * GPIO45 (leído del esquema de la placa).
 *
 * ⚠️ Esto NO sustituye al LDO interno, que es lo que alimenta las E/S del
 * micro — son dos raíles distintos y hacen falta los dos (ver `p4_init` 2.a).
 * Leer el `Kconfig` del ejemplo del IDF como «o uno o el otro» costó una tarde
 * de depuración: su ayuda habla del «SD VDD» y el API dice «SDMMC **IO**».
 *
 * ⚠️ La POLARIDAD sale del ENV (`pwralto`), no horneada, y no está cerrada:
 * el análisis del transistor decía activo bajo y la medida en placa apunta a
 * lo contrario. Hasta que se cierre, el ENV manda.
 *
 * Se conduce SIEMPRE y explícitamente, sin confiar en el estado de reposo: el
 * divisor de la puerta deja 1,65 V si nadie manda, y a esa tensión el MOSFET no
 * está ni abierto ni cerrado — conduce a medias, calienta, y el resultado
 * depende de la unidad y de la temperatura. */
static void alimentar(int encender) {
    if (s_p.pwr < 0) return;                     /* raíl fijo: nada que hacer */
    int nivel = s_p.pwr_activo_alto ? (encender ? 1 : 0)
                                    : (encender ? 0 : 1);
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << s_p.pwr,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level((gpio_num_t) s_p.pwr, nivel);
}

static int p4_init(void) {
    if (!s_configurado) { s_ultimo = ESP_ERR_INVALID_STATE; return -1; }
    if (s_arrancada) return 0;                   /* volver a llamar no duele */

    /* 1 — el raíl de la TARJETA. Y esperar: la tarjeta exige la alimentación
     *     estable ANTES de los primeros comandos, y el condensador de desacoplo
     *     tarda. Diez milisegundos es de sobra y se pagan una vez. */
    alimentar(1);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* 2 — el periférico. */
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    /* 2.a — ⚠️ EL DOMINIO DE E/S DEL MICRO, que NO es lo mismo que el raíl de
     *       la tarjeta y costó una tarde confundirlos.
     *
     *       En el ESP32-P4 los pads de SDMMC los alimenta un LDO INTERNO. Con
     *       la tarjeta alimentada y este LDO apagado, los pads no pueden
     *       conducir el bus: `sdmmc_card_init` agota el plazo y las líneas se
     *       quedan en tensiones intermedias (medidas: 1,2 V con pull-ups de
     *       51 K a 3V3). O sea, el sintoma es EXACTAMENTE el de un bus roto.
     *
     *       El handle se crea UNA vez y se conserva: es del periférico, no de
     *       la tarjeta, y no hay motivo para soltarlo entre montajes. */
    if (s_p.ldo > 0) {
        if (s_ldo == NULL) {
            sd_pwr_ctrl_ldo_config_t cfg = { .ldo_chan_id = s_p.ldo };
            s_ultimo = sd_pwr_ctrl_new_on_chip_ldo(&cfg, &s_ldo);
            if (s_ultimo != ESP_OK) { s_ldo = NULL; alimentar(0); return -1; }
        }
        host.pwr_ctrl_handle = s_ldo;
    }
    host.slot         = s_p.slot;
    host.max_freq_khz = s_p.khz > 0 ? s_p.khz : SDIO_KHZ_POR_DEFECTO;
    if (s_p.ancho == 1) {
        host.flags &= ~SDMMC_HOST_FLAG_4BIT;
        host.flags |=  SDMMC_HOST_FLAG_1BIT;
    }

    /* 3 — la ranura, CAMPO A CAMPO (ver la cabecera: la macro por defecto se
     *     llevaría el GPIO45, que aquí es la alimentación). */
    sdmmc_slot_config_t slot;
    memset(&slot, 0, sizeof slot);
    slot.clk   = (gpio_num_t) s_p.clk;
    slot.cmd   = (gpio_num_t) s_p.cmd;
    slot.d0    = (gpio_num_t) s_p.d0;
    slot.d1    = (s_p.ancho == 4) ? (gpio_num_t) s_p.d1 : GPIO_NUM_NC;
    slot.d2    = (s_p.ancho == 4) ? (gpio_num_t) s_p.d2 : GPIO_NUM_NC;
    slot.d3    = (s_p.ancho == 4) ? (gpio_num_t) s_p.d3 : GPIO_NUM_NC;
    slot.d4    = GPIO_NUM_NC;     /* ⚠️ NO tocar: en esta placa el 45 es pwr */
    slot.d5    = GPIO_NUM_NC;
    slot.d6    = GPIO_NUM_NC;
    slot.d7    = GPIO_NUM_NC;
    slot.cd    = SDMMC_SLOT_NO_CD;   /* el zócalo no lleva deteccion a GPIO */
    slot.wp    = SDMMC_SLOT_NO_WP;
    slot.width = s_p.ancho;          /* EXPLÍCITO: 0 significaria "el maximo" */
    slot.flags = 0;                  /* pull-ups EXTERNOS, ver cabecera       */

    s_ultimo = sdmmc_host_init();
    if (s_ultimo != ESP_OK) { alimentar(0); return -1; }

    s_ultimo = sdmmc_host_init_slot(s_p.slot, &slot);
    if (s_ultimo != ESP_OK) { alimentar(0); return -1; }

    /* 4 — la tarjeta. Aquí el protocolo lo habla el SDK: CMD0, CMD8, ACMD41,
     *     el ancho por ACMD6... nada de eso es nuestro, y ése es justo el
     *     motivo de que `bpvm_sd.c` no tenga sitio en este camino. */
    memset(&s_card, 0, sizeof s_card);
    s_ultimo = sdmmc_card_init(&host, &s_card);
    if (s_ultimo != ESP_OK) {
        /* Cortar el raíl al fallar no es limpieza por gusto: deja la tarjeta en
         * un estado conocido para el siguiente intento. Una SD que se quedó a
         * medias de la negociación puede no volver en frío, y apagarla es la
         * ÚNICA forma fiable de reiniciarla. */
        alimentar(0);
        return -1;
    }
    s_arrancada = 1;
    return 0;
}

static int p4_leer(uint32_t lba, uint32_t n, uint8_t* dst) {
    if (!s_arrancada || !dst) return -1;
    s_ultimo = sdmmc_read_sectors(&s_card, dst, lba, n);
    return (s_ultimo == ESP_OK) ? 0 : -1;
}

static int p4_escribir(uint32_t lba, uint32_t n, const uint8_t* src) {
    if (!s_arrancada || !src) return -1;
    s_ultimo = sdmmc_write_sectors(&s_card, src, lba, n);
    return (s_ultimo == ESP_OK) ? 0 : -1;
}

/*
 * Sin pin de detección, esto NO puede saberlo, así que contesta 1 — decir
 * «no hay» sería mentir (lo dice el contrato). Es una diferencia de CAPACIDAD
 * con la Metro, no un descuido:
 *
 *   ⚠️ Por eso el P4 **no debe usar `bpvm_fs_fat_vigilar`**: esa función se
 *   rearma en la rama «no hay medio», que aquí no se ejecuta nunca, y quedaría
 *   dando UN intento por arranque. En esta placa el montaje es explícito.
 *
 * La detección de verdad, cuando alguien pregunte, se hace LEYENDO (H6, sección
 * de detección a demanda): el estado de la tarjeta más la firma del sector 0.
 */
static int p4_hay_medio(void) { return 1; }

static uint32_t p4_bloques(void) {
    if (!s_arrancada) return 0u;
    /* `capacity` viene en sectores; el tamaño de sector se comprueba en vez de
     * suponerlo, porque todo lo de arriba (FatFs incluido) está cerrado a 512 y
     * una tarjeta con otro tamaño daría direcciones desplazadas en silencio. */
    if (s_card.csd.sector_size != (int) BPVM_BLK_TAM) return 0u;
    return (uint32_t) s_card.csd.capacity;
}

static const char* p4_motivo(void) {
    if (!s_configurado) return "el dispositivo SDIO no esta configurado (falta 'sd' en el env)";
    return esp_err_to_name(s_ultimo);
}

/* `sincronizar` a NULL con el mismo criterio que en SPI: `sdmmc_write_sectors`
 * no vuelve hasta que la tarjeta suelta el ocupado. Si alguna vez apareciera
 * corrupción tras un corte de corriente, éste es el primer sitio donde mirar —
 * poner aquí una función que no hiciera nada diría lo contrario a quien lo lea. */
static const bpvm_blk_backend_t s_backend = {
    .init        = p4_init,
    .leer        = p4_leer,
    .escribir    = p4_escribir,
    .hay_medio   = p4_hay_medio,
    .bloques     = p4_bloques,
    .sincronizar = NULL,
    .motivo      = p4_motivo,
};

const bpvm_blk_backend_t* bpvm_blk_sdmmc(const bpvm_sdio_pines_t* pines) {
    if (pines) {
        if (!s_configurado || memcmp(&s_p, pines, sizeof s_p) != 0) {
            s_p = *pines;
            s_arrancada = 0;
            memset(&s_card, 0, sizeof s_card);
        }
        s_configurado = 1;
    }
    return &s_backend;
}
