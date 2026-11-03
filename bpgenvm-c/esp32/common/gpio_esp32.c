/*
 * gpio_esp32.c — backend GPIO del HAL para ESP32-S3 (H4.5).
 *
 * Implementa bpvm_gpio_backend_t (la VM core llama bpvm_gpio_init/write/
 * read/pull → estas funciones tocan el HW vía driver/gpio.h de ESP-IDF).
 * Hermano de la impl Pico (pico/main.c s_pico_gpio_backend).
 *
 * Convención BP (igual que en Pico):
 *   - mode: 0 = INPUT, 1 = OUTPUT
 *   - value: 0 = LOW, !=0 = HIGH
 *   - pull: 0 = none, 1 = up, 2 = down
 */
#include "chip_cfg.h"   /* CHIP_MICRO: el id canonico de ESTE chip */
#include "hw_esp32.h"
#include "bpvm_gpio.h"
#include "bpvm_machine.h"
#include "bpvm_uart.h"
#include "bpvm_spi.h"
#include "bpvm_i2c.h"
#include "bpvm_pwm.h"
#include "bpvm_adc.h"
#include "bpvm_pulse.h"
#include "bpvm_wdt.h"
#include "bpvm_log.h"               /* V7/H3 (#552): un dispositivo SPI que el IDF rechaza, al log */

#include "esp_attr.h"               /* V7/H4: __NOINIT_ATTR para las migas */
#include "esp_pm.h"                 /* V8 #784: cambiar la frecuencia de la CPU */
#include "esp_private/esp_clk.h"    /* V8 #784: esp_clk_cpu_freq, la que lleva de verdad */
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"            /* H14: backend PWM */
#include "esp_adc/adc_oneshot.h"    /* H14: backend ADC */
/* #465 — el PCNT NO existe en todas las familias ESP32: el C3 no lo tiene
 * (`SOC_PCNT_SUPPORTED` sin definir). Lo dice el silicio, se le pregunta. */
#if SOC_PCNT_SUPPORTED
#include "driver/pulse_cnt.h"       /* H14: backend contador (PCNT) */
#endif
#if SOC_RMT_SUPPORTED
#include "driver/rmt_tx.h"          /* V7/H4: backend NeoPixel (WS2812) por RMT */
#include "driver/rmt_encoder.h"
#include "bpvm_neopixel.h"
#endif
#include "esp_private/esp_gpio_reserve.h"   /* #574: los pines que el IDF ya reservó (la flash, la PSRAM) */
#ifdef CHIP_GPIO_VDD_SPI
#include "esp_efuse.h"                      /* #581: ¿el pad VDD_SPI es un GPIO? Lo dice un eFuse (chip_cfg.h) */
#include "esp_efuse_table.h"
#endif
#if defined(CHIP_GPIO_OCTAL_DESDE) && CONFIG_SPIRAM
#include "esp_psram.h"                      /* #670: ¿la PSRAM (octal) está en marcha? Sólo la S3 */
#endif
#include "wire_v1.h"                        /* #574: los pines del cable del IDE */
#include "esp_system.h"             /* paso 4 cierre — esp_reset_reason (causa de reset) */
#include "esp_task_wdt.h"           /* cierre V3 — backend Wdt (Task Watchdog Timer) */
#include "freertos/FreeRTOS.h"   /* pdMS_TO_TICKS, portMAX_DELAY */
#include "esp_mac.h"      /* uniqueId desde la MAC de efuse */
#include "esp_timer.h"    /* uptime */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static void esp32_gpio_init_impl(int pin, int mode) {
    gpio_num_t g = (gpio_num_t) pin;
    gpio_reset_pin(g);
    gpio_set_direction(g, (mode == 1) ? GPIO_MODE_OUTPUT : (mode == 2) ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_INPUT);   /* V7/H4: 2 = colector abierto */
}

static void esp32_gpio_pull_impl(int pin, int pull_mode) {
    gpio_num_t g = (gpio_num_t) pin;
    switch (pull_mode) {
        case 1:  gpio_set_pull_mode(g, GPIO_PULLUP_ONLY);   break;
        case 2:  gpio_set_pull_mode(g, GPIO_PULLDOWN_ONLY); break;
        default: gpio_set_pull_mode(g, GPIO_FLOATING);      break;
    }
}

static void esp32_gpio_write_impl(int pin, int value) {
    gpio_set_level((gpio_num_t) pin, value != 0);
}

static int esp32_gpio_read_impl(int pin) {
    return gpio_get_level((gpio_num_t) pin) ? 1 : 0;
}

/* V7 #667 (8-oct) — UN PIN QUE UN PROGRAMA SUELTA, A COMO ESTABA (Eduardo: «los pines por defecto deben ser input o
 * nada. Si están como output se debería revertir»): el reset del IDF —función GPIO, sin salida ni entrada, con su
 * pull-up—. Lo usan el release() de un Pin, los de un PWM, un contador, una tira o una UART, y la red. */
static void esp32_pin_al_reset(int pin) {
    if (pin >= 0) gpio_reset_pin((gpio_num_t) pin);
}

static const bpvm_gpio_backend_t s_esp32_gpio_backend = {
    .init  = esp32_gpio_init_impl,
    .pull  = esp32_gpio_pull_impl,
    .write = esp32_gpio_write_impl,
    .read  = esp32_gpio_read_impl,
    .soltar = esp32_pin_al_reset,   /* V7 #667 */
};

/* ---- Pico (info de MCU) backend ----
 * Igual que el STM32 (s_pico_backend en gpio_stm32.c): da datos reales del
 * ESP32-S3 a los builtins Pico.* desde BP (boardName/uniqueId/cpuFreqHz/
 * gpioCount...). Sin esto, Pico.gpioCount() caia al stub host (30 GPIO,
 * board "host") aunque el INFO del wire ya diera los valores reales.
 * ESP32-S3 (DevKitC, WROOM-1): 45 GPIO utiles (0-21, 26-48). La frecuencia, la del chip (#784). */
static void esp32_unique_id_impl(char* buf, size_t len) {
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);   /* MAC base de fabrica (efuse) */
    if (buf && len > 0) {
        snprintf(buf, len, "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

/* ── LOS DOS NOMBRES (V6, 9-sep) ───────────────────────────────────────────────
 *
 * Este fichero lo comparten el S3, el C3 y el C6, y antes tenia UN solo nombre
 * escrito a mano: "esp32s3-devkitc". O sea que un programa en un C3 se creia un
 * S3 — cinco de seis campos falsos, incluido el nombre. El arreglo del wire de
 * `#465` corrigio solo el otro camino y este se quedo mintiendo.
 *
 * Ahora el micro sale de `CHIP_MICRO`, una entrada POR MICRO en el `chip_cfg.h`
 * de cada uno — el mismo sitio del que ya salian su nombre y sus numeros de
 * memoria. Y la placa dice `generic` a proposito: construimos imagenes para el
 * MICRO y se quieren genericas, asi que la imagen NO sabe en que placa esta y no
 * debe inventarselo. Quien se haga la suya, que ponga lo que quiera.
 * (Decision de Eduardo, 8-sep.) */
static void esp32_micro_name_impl(char* buf, size_t len) {
    if (buf && len > 0) {
        strncpy(buf, CHIP_MICRO, len - 1);
        buf[len - 1] = '\0';
    }
}

static void esp32_board_name_impl(char* buf, size_t len) {
    if (buf && len > 0) {
        strncpy(buf, "generic", len - 1);
        buf[len - 1] = '\0';
    }
}

static float esp32_temp_c_impl(void) {
    return 0.0f;   /* sensor interno no cableado en el MVP (igual que STM32) */
}

/* V8 #784 (3-nov) — LA FRECUENCIA, LA DE VERDAD, Y CAMBIARLA. Hasta hoy, un 240 escrito a mano para las cuatro de este
 * fichero: la S3, la C3 y la C6 iban a 160 y lo decían mal. Ahora se le pregunta al chip.
 * Cambiarla, con la gestión de energía del IDF (CONFIG_PM_ENABLE, en los defaults de las cinco) y la mínima igual a la
 * máxima: la CPU se queda fija en la pedida, y sin pedir nada va a la de la imagen. Los buses van con un reloj de
 * 80 MHz que no cambia entre 80 y el máximo, así que no hay que volver a configurarlos (en la Pico sí); por debajo de
 * 80 cambiaría, y por eso el mínimo es 80. Lo usan también la P4 (p4_board_id.c), cuyo silicio sólo da 360, y la
 * clave `cpu.mhz` del ENV (bpvm_boot2.c). */
#if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C5
static const int s_cpu_mhz[] = {80, 160, 240};
#elif CONFIG_IDF_TARGET_ESP32C6
static const int s_cpu_mhz[] = {80, 120, 160};
#elif CONFIG_IDF_TARGET_ESP32C3
static const int s_cpu_mhz[] = {80, 160};
#else
static const int s_cpu_mhz[] = {CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ};   /* la P4: la de la imagen, y nada más */
#endif
#define N_CPU_MHZ ((int) (sizeof s_cpu_mhz / sizeof s_cpu_mhz[0]))

int esp32_cpu_freq_hz(void) {
    return (int) esp_clk_cpu_freq();
}

int esp32_set_cpu_freq_mhz(int mhz) {
    int i;
    for (i = 0; i < N_CPU_MHZ && s_cpu_mhz[i] != mhz; i++) ;
    if (i == N_CPU_MHZ) return 0;                          /* este chip no la da */
    if (esp32_cpu_freq_hz() == mhz * 1000000) return 1;    /* ya va a esa */
#if CONFIG_PM_ENABLE
    {
        esp_pm_config_t pm = { .max_freq_mhz = mhz, .min_freq_mhz = mhz, .light_sleep_enable = false };
        if (esp_pm_configure(&pm) != ESP_OK) return 0;
        return esp32_cpu_freq_hz() == mhz * 1000000;       /* esp_pm_configure la cambia en el acto */
    }
#else
    return 0;                                              /* sin la gestión de energía, no se puede */
#endif
}

static int esp32_uptime_ms_impl(void) {
    return (int) (esp_timer_get_time() / 1000);   /* us -> ms */
}

/* ══════════ V7/H4 — LA TABLA DE CAPACIDADES de la familia ESP32, CALCULADA ══════════
 * Aquí no hay un número escrito a mano: se le pregunta al IDF por el chip con
 * el que se compila (`soc_caps.h`: GPIO válidos y de sólo entrada, PCNT, cuántas
 * UART/I2C/SPI, canales LEDC, unidades de ADC) y al driver por el pin de cada
 * canal de ADC (`adc_oneshot_channel_to_io`). Antes este fichero —común a
 * S3/C3/C6/C5— devolvía 45 GPIO, 20 ADC y 8 PWM a pelo: los del S3 para todos,
 * y un C5 por BP se creía un S3 mientras el INFO decía 29/6/6 (#470). */
static uint16_t s_caps_pin[SOC_GPIO_PIN_COUNT];
static int8_t   s_caps_adc_u[SOC_GPIO_PIN_COUNT];
static int8_t   s_caps_adc_c[SOC_GPIO_PIN_COUNT];
static uint8_t  s_caps_sis[SOC_GPIO_PIN_COUNT];   /* #574: BPVM_PIN_SIS_* de los pines del sistema */
static uint8_t  s_caps_uart[SOC_UART_HP_NUM];
static uint8_t  s_caps_i2c[SOC_HP_I2C_NUM];
static uint8_t  s_caps_spi[SOC_SPI_PERIPH_NUM];
static bpvm_caps_t s_caps;
/* V7/H4 (#487) — el bloque de las migas: `.noinit` sobrevive a los resets por
 * software y por watchdog (a un power-on, no: el magic del común lo detecta). */
static __NOINIT_ATTR uint32_t s_esp_migas[20];

static void esp32_caps_init(void) {
    /* #574 (V7/H4, 26-sep) — los pines del SISTEMA no son del programa. `GPIO_IS_VALID_GPIO` daba por buenos los de
     * la flash y la PSRAM (en la C6 el GPIO25 es el MISO de la flash: un `Gpio.Pin(25, OUTPUT)` se la quitaba al
     * micro). El IDF ya los reservó al arrancar y, a estas alturas, nada más: esta tabla se calcula antes de
     * cualquier driver de pines (main.c). Y los del cable del IDE, que el IDF no reserva: los dice su transporte
     * (decidido por Eduardo: «los pines del wire también»). Máscara 0 —para BP no están— y el motivo aparte. */
    const uint64_t del_wire = wire_v1_transport_pines();
#ifdef CHIP_GPIO_VDD_SPI
    /* #581 (7-oct) — y el pad VDD_SPI de la C3, la C5 y la C6 (chip_cfg.h): la ALIMENTACIÓN de la flash, un GPIO
     * sólo con el eFuse VDD_SPI_AS_GPIO quemado. El IDF no lo reserva —no es un pin de datos—, y la tabla lo daba
     * por libre: en la C6, un `Gpio.Pin(27, OUTPUT)` «salía libre» siendo la alimentación de la flash. Lo mira aquí
     * el eFuse. */
    const bool vdd_spi_es_gpio = esp_efuse_read_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);
#endif
#ifdef CHIP_GPIO_OCTAL_DESDE
    /* #670 (8-oct) — y los de la memoria OCTAL de la S3 (chip_cfg.h): el IDF tampoco los reserva. Una flash octal
     * los usa siempre; una PSRAM octal, si arrancó. */
    bool memoria_octal = false;
#if CONFIG_ESPTOOLPY_OCT_FLASH
    memoria_octal = true;
#endif
#if CONFIG_SPIRAM && CONFIG_SPIRAM_MODE_OCT
    if (esp_psram_is_initialized()) memoria_octal = true;
#endif
#endif
    for (int g = 0; g < SOC_GPIO_PIN_COUNT; g++) {
        uint16_t m = 0;
        s_caps_adc_u[g] = -1; s_caps_adc_c[g] = -1;
        s_caps_sis[g] = 0;
        if (GPIO_IS_VALID_GPIO(g) && esp_gpio_is_reserved(1ULL << g)) {
            s_caps_sis[g] = BPVM_PIN_SIS_MEMORIA;
#ifdef CHIP_GPIO_VDD_SPI
        } else if (g == CHIP_GPIO_VDD_SPI && !vdd_spi_es_gpio) {
            s_caps_sis[g] = BPVM_PIN_SIS_MEMORIA;     /* #581: la alimentación de la flash */
#endif
#ifdef CHIP_GPIO_OCTAL_DESDE
        } else if (memoria_octal && g >= CHIP_GPIO_OCTAL_DESDE && g <= CHIP_GPIO_OCTAL_HASTA) {
            s_caps_sis[g] = BPVM_PIN_SIS_MEMORIA;     /* #670: la PSRAM (o la flash) octal */
#endif
        } else if (GPIO_IS_VALID_GPIO(g) && (del_wire & (1ULL << g))) {
            s_caps_sis[g] = BPVM_PIN_SIS_WIRE;
        } else if (GPIO_IS_VALID_GPIO(g)) {
            m = BPVM_PIN_DIGITAL;
            if (GPIO_IS_VALID_OUTPUT_GPIO(g)) m |= BPVM_PIN_PWM | BPVM_PIN_OPEN_DRAIN;
            else m |= BPVM_PIN_INPUT_ONLY;
#if SOC_PCNT_SUPPORTED
            m |= BPVM_PIN_PULSE;      /* el C3 es el único ESP sin PCNT: allí no hay contador (#480) */
#endif
        }
        s_caps_pin[g] = m;
    }
    for (int u = 0; u < SOC_ADC_PERIPH_NUM; u++) {
        for (int ch = 0; ch < SOC_ADC_CHANNEL_NUM(u); ch++) {
            int io = -1;
            if (adc_oneshot_channel_to_io((adc_unit_t) u, (adc_channel_t) ch, &io) == ESP_OK
                && io >= 0 && io < SOC_GPIO_PIN_COUNT && s_caps_pin[io] && s_caps_adc_u[io] < 0) {
                s_caps_pin[io] |= BPVM_PIN_ANALOG;
                s_caps_adc_u[io] = (int8_t) u; s_caps_adc_c[io] = (int8_t) ch;
            }
        }
    }
    int nu = 0, ni = 0, ns = 0;
    for (int b = 1; b < SOC_UART_HP_NUM && b <= 2; b++) s_caps_uart[nu++] = (uint8_t) b;   /* 0 = el wire; el backend mapea 1 y 2 */
    for (int b = 0; b < (int) SOC_HP_I2C_NUM; b++) s_caps_i2c[ni++] = (uint8_t) b;
    s_caps_spi[ns++] = 2;                                   /* SPI2_HOST; SPI0/1 son la flash */
#if SOC_SPI_PERIPH_NUM >= 3
    s_caps_spi[ns++] = 3;                                   /* SPI3_HOST donde lo hay (S3, P4) */
#endif
    s_caps.micro = CHIP_MICRO;
    s_caps.paquete = "";                                    /* el IDF no dice el encapsulado; la flash y la PSRAM se miden */
    s_caps.fabricante = "Espressif";
    s_caps.cpu_hz_max = (uint32_t) s_cpu_mhz[N_CPU_MHZ - 1] * 1000000u;   /* V8 #784: lo que da el chip */
    s_caps.cpu_hz_min = (uint32_t) s_cpu_mhz[0] * 1000000u;
    s_caps.cpu_ajustable = (N_CPU_MHZ > 1) ? 1 : 0;
    s_caps.uart = s_caps_uart; s_caps.n_uart = (uint8_t) nu;
    s_caps.i2c = s_caps_i2c;   s_caps.n_i2c = (uint8_t) ni;
    s_caps.spi = s_caps_spi;   s_caps.n_spi = (uint8_t) ns;
    /* V7 #667: los que el programa puede usar —los canales 2..; el 1 es de la retroiluminación de la P4—, no los del
     * chip (con ESP32_PWM_HAY, más abajo: 4 en la C3, la C5 y la C6; 6 en la S3 y la P4). */
    s_caps.n_pwm = ((SOC_LEDC_CHANNEL_NUM - 2) < 6) ? (SOC_LEDC_CHANNEL_NUM - 2) : 6; s_caps.pwm_unidad = "canal";
    s_caps.n_adc = SOC_ADC_PERIPH_NUM;
    s_caps.pin = s_caps_pin; s_caps.n_pin = SOC_GPIO_PIN_COUNT;
    s_caps.pin_adc_unit = s_caps_adc_u; s_caps.pin_adc_ch = s_caps_adc_c;
    s_caps.pin_sistema = s_caps_sis;                        /* #574 */
    s_caps.breadcrumb = 1;
    bpvm_caps_set(&s_caps);
    bpvm_breadcrumb_attach(s_esp_migas, 20u);
}

/* paso 4 cierre — causa del último reset vía registro HW (esp_reset_reason, lo
 * captura el ROM/bootloader; NO usa RAM retenida). No-static: lo comparte el
 * backend del P4 (p4_board_id.c). Declarado en hw_esp32.h. */
const char* esp32_reset_cause(void) {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "power-on";
        case ESP_RST_EXT:       return "external pin";
        case ESP_RST_SW:        return "software";
        case ESP_RST_PANIC:     return "panic/exception";
        case ESP_RST_INT_WDT:   return "int-watchdog";
        case ESP_RST_TASK_WDT:  return "task-watchdog";
        case ESP_RST_WDT:       return "watchdog";
        case ESP_RST_DEEPSLEEP: return "deep-sleep wake";
        case ESP_RST_BROWNOUT:  return "brown-out";
        case ESP_RST_SDIO:      return "SDIO";
        /* V8 #709 (29-oct) — los cinco que faltaban de los quince del IDF (6.0.1 y 6.1): tras `idf flash`, que reinicia
         * la C3 y la C5 por su USB, el INFO decía «unknown». Un motivo que no se dice es un diagnóstico mudo; «unknown»
         * queda para cuando el IDF tampoco lo sabe. */
        case ESP_RST_USB:        return "USB";
        case ESP_RST_JTAG:       return "JTAG";
        case ESP_RST_EFUSE:      return "eFuse error";
        case ESP_RST_PWR_GLITCH: return "power glitch";
        case ESP_RST_CPU_LOCKUP: return "CPU lockup";
        default:                return "unknown";
    }
}

static const bpvm_machine_backend_t s_esp32_pico_backend = {
    .uniqueId      = esp32_unique_id_impl,
    .microName     = esp32_micro_name_impl,
    .boardName     = esp32_board_name_impl,
    .tempC         = esp32_temp_c_impl,
    .cpuFreqHz     = esp32_cpu_freq_hz,
    .uptimeMs      = esp32_uptime_ms_impl,
    .setCpuFreqMHz = esp32_set_cpu_freq_mhz,
    .resetCause    = esp32_reset_cause,        /* paso 4 cierre */
};

/* ===================== UART (H16) =====================================
 * Backend UART sobre el driver de ESP-IDF. bus N -> UART_NUM_N. El bus 0
 * (UART0, GPIO43/44) es el WIRE del IDE -> RESERVADO, no se expone; el
 * usuario usa bus 1 (UART1) o bus 2 (UART2). Pines libres: el GPIO-matrix
 * del S3 rutea TX/RX a los GPIO que pida el .bp (uart_set_pin).
 */
#define ESP32_UART_RX_BUF 512

/* bus N -> UART_NUM_N (numero de bus = instancia HW). bus 0 = wire. */
static int esp32_uart_port(int bus) {
    /* #465 — se pregunta al SILICIO cuántos hay (`SOC_UART_NUM` del IDF) en vez de
     * dar por hecho los del S3. Salió al compilar el ensayo del C3: allí son DOS
     * UART y `UART_NUM_2` ni existe, así que este fichero —que es la cintura HW
     * COMPARTIDA de la familia ESP32— no compilaba. Un bus que la placa no tiene
     * devuelve -1, que es lo que ya hacía para los que no existen.
     *
     * V6/P1.C6 (3-sep) — y la cuenta correcta es `SOC_UART_HP_NUM`, no
     * `SOC_UART_NUM`: el C6 cuenta TRES UART (dos HP + la LP del núcleo de baja
     * potencia) pero `UART_NUM_2` no existe — la tercera es `LP_UART_NUM_0`. El
     * silicio nuevo hizo verdadera una condición que el S3 y el C3 no
     * distinguían (3 y 2 en las dos cuentas). Los buses BP son los HP. */
    switch (bus) {
        case 1: return UART_NUM_1;
#if SOC_UART_HP_NUM > 2
        case 2: return UART_NUM_2;
#endif
        default: return -1;   /* 0 = wire (reservado); el resto no existe */
    }
}

static int s_uart_tx[3] = { -1, -1, -1 }, s_uart_rx[3] = { -1, -1, -1 };   /* V7 #667: por bus, para soltarla */
static void esp32_uart_init_impl(int bus, int tx, int rx, int baudrate,
                                 int data_bits, int stop_bits, int parity) {
    int p = esp32_uart_port(bus);
    if (p < 0) return;
    s_uart_tx[bus] = tx;
    s_uart_rx[bus] = rx;
    uart_config_t cfg = {
        .baud_rate  = baudrate,
        .data_bits  = (data_bits == 7) ? UART_DATA_7_BITS : UART_DATA_8_BITS,
        .parity     = (parity == 1) ? UART_PARITY_ODD
                    : (parity == 2) ? UART_PARITY_EVEN
                                    : UART_PARITY_DISABLE,
        .stop_bits  = (stop_bits == 2) ? UART_STOP_BITS_2 : UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    /* re-init seguro (p.ej. 2a ejecucion): reinstala limpio. */
    if (uart_is_driver_installed((uart_port_t) p)) uart_driver_delete((uart_port_t) p);
    uart_param_config((uart_port_t) p, &cfg);
    uart_set_pin((uart_port_t) p, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install((uart_port_t) p, ESP32_UART_RX_BUF, 0, 0, NULL, 0);
}

static int esp32_uart_write_impl(int bus, const uint8_t* data, size_t n) {
    int p = esp32_uart_port(bus);
    if (p < 0) return -1;
    return uart_write_bytes((uart_port_t) p, (const char*) data, n);  /* bytes escritos */
}

static int esp32_uart_read_impl(int bus, uint8_t* data, size_t n, int timeout_ms) {
    int p = esp32_uart_port(bus);
    if (p < 0) return -1;
    /* contrato: timeout<=0 bloquea hasta n bytes; >0 devuelve al tener n o expirar. */
    TickType_t to = (timeout_ms > 0) ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY;
    return uart_read_bytes((uart_port_t) p, data, n, to);  /* bytes leidos (0..n) */
}

static int esp32_uart_available_impl(int bus) {
    int p = esp32_uart_port(bus);
    if (p < 0) return -1;
    size_t len = 0;
    return (uart_get_buffered_data_len((uart_port_t) p, &len) == ESP_OK) ? (int) len : -1;
}

/* V7 #667 — soltarla: el driver fuera (su búfer y su interrupción) y sus pines al reset. */
static void esp32_uart_release_impl(int bus, int tx, int rx) {
    int p = esp32_uart_port(bus);
    if (p < 0) return;
    if (uart_is_driver_installed((uart_port_t) p)) uart_driver_delete((uart_port_t) p);
    esp32_pin_al_reset(tx >= 0 ? tx : s_uart_tx[bus]);
    esp32_pin_al_reset(rx >= 0 ? rx : s_uart_rx[bus]);
    s_uart_tx[bus] = s_uart_rx[bus] = -1;
}

static const bpvm_uart_backend_t s_esp32_uart_backend = {
    .init      = esp32_uart_init_impl,
    .release   = esp32_uart_release_impl,   /* V7 #667 */
    .write     = esp32_uart_write_impl,
    .read      = esp32_uart_read_impl,
    .available = esp32_uart_available_impl,
};

/* ===================== SPI (H16) =====================================
 * Backend SPI sobre el driver de ESP-IDF. bus N -> SPIN_HOST. SPI0/1 son
 * para la flash (reservados) -> el usuario usa bus 2 (SPI2/FSPI) o 3
 * (SPI3). El CS lo gestiona el usuario por GPIO (Gpio.Pin), igual que en
 * Pico/STM32 -> spics_io_num = -1. Sin DMA (polling): los .mod mueven pocas
 * decenas de bytes (sensores), que caben en el FIFO; evita los requisitos
 * de alineacion del DMA. [v3: DMA para transferencias grandes.]
 */
static int esp32_spi_host(int bus) {
    /* #465 — igual que el UART: `SOC_SPI_PERIPH_NUM` lo dice el silicio. El C3
     * tiene DOS periféricos SPI y `SPI3_HOST` no existe. */
    switch (bus) {
        case 2: return SPI2_HOST;
#if SOC_SPI_PERIPH_NUM > 2
        case 3: return SPI3_HOST;
#endif
        default: return -1;   /* 0/1 = flash (reservados) */
    }
}

static spi_device_handle_t s_spi_dev[4];   /* por bus; 2 y 3 usados */
static int s_spi_bus_ready[4];

/* V7/H6.c G4.b (25-sep) — EL BUS VA CON DMA (V7_IDEAS §H6.11.11). Sin DMA el periférico mueve 64 bytes por
 * transacción y una franja de LVGL (11 520 B en la C5) eran 180, con unos 22 µs de peaje cada una: dos tercios
 * de los 147 ms de una pantalla. Con DMA la franja es UNA. Hasta 16 KB de una vez (también la franja de un panel
 * de 320×480); por encima, la cintura trocea. */
#define SPI_DMA_MAX   16384
/* Hasta aquí, POLLING (≈12 µs por transacción frente a 22-24 con interrupción, según el IDF): lo pequeño —los
 * comandos de la pantalla, el byte a byte de la TF— no espera a una interrupción. Por encima, interrupción: la CPU
 * queda libre mientras el DMA manda (2,3 ms una franja a 40 MHz), y en la C5, de un núcleo, la aprovecha el hilo io. */
#define SPI_POLL_MAX  64

static void esp32_spi_init_impl(int bus, int sck, int mosi, int miso,
                                int baudrate, int mode) {
    int host = esp32_spi_host(bus);
    if (host < 0) return;
    /* re-init seguro (2a ejecucion): suelta device + bus antes de reabrir. */
    if (s_spi_dev[bus]) { spi_bus_remove_device(s_spi_dev[bus]); s_spi_dev[bus] = NULL; }
    if (s_spi_bus_ready[bus]) { spi_bus_free((spi_host_device_t) host); s_spi_bus_ready[bus] = 0; }

    spi_bus_config_t buscfg = {
        .mosi_io_num     = mosi,
        .miso_io_num     = miso,
        .sclk_io_num     = sck,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = SPI_DMA_MAX,   /* G4.b: con DMA (antes, sin él: el FIFO de 64 B) */
    };
    if (spi_bus_initialize((spi_host_device_t) host, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) return;
    s_spi_bus_ready[bus] = 1;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = baudrate,
        .mode           = mode,    /* 0..3 = CPOL/CPHA, igual que la fachada */
        .spics_io_num   = -1,      /* CS lo lleva el usuario por GPIO */
        .queue_size     = 1,
    };
    spi_bus_add_device((spi_host_device_t) host, &devcfg, &s_spi_dev[bus]);
}

/* ── V7/H6.c G4.a1 — ABRIR una vez, AJUSTAR aparte, CERRAR ─────────────────
 * `esp32_spi_init_impl` (arriba) suelta el bus y lo vuelve a crear en cada
 * llamada: el segundo usuario de un bus se lo quitaba al primero (la TF a la
 * pantalla de la C5) sin decir nada. La regla de quién lo usa y con qué pines es
 * de la HAL BP (`bpvm_bus.c`); aquí sólo queda el hardware:
 *
 *   abrir   — `spi_bus_initialize` SÓLO si no está abierto;
 *   ajustar — la velocidad y el modo son del DISPOSITIVO del IDF: se cambia el
 *             dispositivo (quitar + añadir), NUNCA el bus; si ya es ése, nada;
 *   cerrar  — quitar el dispositivo y `spi_bus_free`.
 *
 * ⚠️ Hasta G4.a2 hay UN dispositivo por bus: dos usuarios con velocidades
 * distintas se quedan con la del último que abrió. Un dispositivo por CS, con la
 * suya aplicada en cada transacción, es el árbitro (V7_IDEAS §H6.11.3). */
static int s_spi_hz[4], s_spi_modo[4];

static int esp32_spi_abrir(int bus, int sck, int mosi, int miso) {
    int host = esp32_spi_host(bus);
    if (host < 0 || bus < 0 || bus >= 4) return -1;
    if (s_spi_bus_ready[bus]) return 0;                   /* ya abierto: NO se reabre */
    spi_bus_config_t buscfg = {
        .mosi_io_num     = mosi,
        .miso_io_num     = miso,
        .sclk_io_num     = sck,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = SPI_DMA_MAX,   /* G4.b: con DMA (antes, sin él: el FIFO de 64 B) */
    };
    if (spi_bus_initialize((spi_host_device_t) host, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) return -1;
    s_spi_bus_ready[bus] = 1;
    s_spi_hz[bus] = s_spi_modo[bus] = -1;
    return 0;
}

static void esp32_spi_ajustar(int bus, int baudrate, int mode) {
    int host = esp32_spi_host(bus);
    if (host < 0 || bus < 0 || bus >= 4 || !s_spi_bus_ready[bus]) return;
    if (s_spi_dev[bus] && s_spi_hz[bus] == baudrate && s_spi_modo[bus] == mode) return;
    if (s_spi_dev[bus]) { spi_bus_remove_device(s_spi_dev[bus]); s_spi_dev[bus] = NULL; }
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = baudrate,
        .mode           = mode,    /* 0..3 = CPOL/CPHA, igual que la fachada */
        .spics_io_num   = -1,      /* CS lo lleva el usuario por GPIO */
        .queue_size     = 1,
    };
    esp_err_t e = spi_bus_add_device((spi_host_device_t) host, &devcfg, &s_spi_dev[bus]);
    if (e == ESP_OK) {
        s_spi_hz[bus] = baudrate; s_spi_modo[bus] = mode;
    } else {
        /* V7/H3 (#552): sin dispositivo, cada transferencia falla después LEJOS de aquí; que se diga dónde. */
        s_spi_dev[bus] = NULL;
        log_printf("spi: bus %d: el IDF no acepta %d Hz en modo %d (%s)", bus, baudrate, mode, esp_err_to_name(e));
    }
}

static void esp32_spi_cerrar(int bus) {
    int host = esp32_spi_host(bus);
    if (host < 0 || bus < 0 || bus >= 4) return;
    if (s_spi_dev[bus]) { spi_bus_remove_device(s_spi_dev[bus]); s_spi_dev[bus] = NULL; }
    if (s_spi_bus_ready[bus]) { spi_bus_free((spi_host_device_t) host); s_spi_bus_ready[bus] = 0; }
}

/* tx NULL -> manda ceros; rx NULL -> descarta lo recibido. Full-duplex.
 *
 * V7/H6.c G4.a3 — la CINTURA trocea, no quien llama: desde BP un Spi.write de más de 64
 * bytes fallaba ENTERO en el ESP32 (el tramo de los builtins es de 256). El CS es un GPIO
 * que lleva la HAL BP durante todo el turno: en el cable sólo quedan huecos entre trozos,
 * con el CS abajo.
 *
 * V7/H6.c G4.b (25-sep) — con DMA (arriba): la franja de LVGL va en UNA transacción. Tres
 * caminos, por tamaño (V7_IDEAS §H6.11.11):
 *   · hasta 4 bytes, en los búferes de la propia transacción: ni DMA ni búfer de rebote (en
 *     el C3, el C6 y el S3 un byte sin alinear hacía reservar uno POR BYTE);
 *   · hasta SPI_POLL_MAX, polling; por encima, interrupción, en tramos de SPI_DMA_MAX;
 *   · un búfer en PSRAM o sin alinear (el de LVGL vive en el bloque del pack) lo copia el IDF
 *     a uno interno apto para DMA. Si no hay memoria para eso, se dice UNA vez y -1. */
/* «tx NULL manda ceros», también con DMA. En RAM interna y alineado, NO `const`: el DMA no lee de la flash, y un
 * búfer en .rodata obligaría al IDF a copiarlo a uno de rebote en cada transacción. */
static uint8_t s_ceros[SPI_POLL_MAX] __attribute__((aligned(4)));
static int s_spi_dicho = 0;

static esp_err_t esp32_spi_una(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = n * 8;                 /* en BITS */
    if (n <= 4) {
        t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
        if (tx) memcpy(t.tx_data, tx, n);   /* sin tx, los ceros del memset */
        esp_err_t e = spi_device_polling_transmit(s_spi_dev[bus], &t);
        if (e == ESP_OK && rx) memcpy(rx, t.rx_data, n);
        return e;
    }
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    return (n <= SPI_POLL_MAX) ? spi_device_polling_transmit(s_spi_dev[bus], &t)
                               : spi_device_transmit(s_spi_dev[bus], &t);
}

static int esp32_spi_xfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    size_t hecho = 0;
    if (bus < 0 || bus >= 4 || !s_spi_dev[bus] || n == 0) return -1;
    /* Sin tx, los ceros salen de s_ceros: tramos de su tamaño (sólo lo usa Spi.read de BP). */
    size_t tope = tx ? (size_t) SPI_DMA_MAX : sizeof s_ceros;
    while (hecho < n) {
        size_t trozo = (n - hecho > tope) ? tope : (n - hecho);
        esp_err_t e = esp32_spi_una(bus, tx ? tx + hecho : s_ceros, rx ? rx + hecho : NULL, trozo);
        if (e != ESP_OK) {
            if (!s_spi_dicho) {
                s_spi_dicho = 1;
                log_printf("spi: bus %d: la transferencia de %u B falla (%s)%s", bus, (unsigned) trozo,
                           esp_err_to_name(e), e == ESP_ERR_NO_MEM ? ": no hay memoria interna para el DMA" : "");
            }
            return -1;
        }
        hecho += trozo;
    }
    return (int) n;
}

static int esp32_spi_write_impl(int bus, const uint8_t* data, size_t n) {
    return esp32_spi_xfer(bus, data, NULL, n);
}
static int esp32_spi_read_impl(int bus, uint8_t* data, size_t n) {
    return esp32_spi_xfer(bus, NULL, data, n);
}
static int esp32_spi_transfer_impl(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    return esp32_spi_xfer(bus, tx, rx, n);
}

static const bpvm_spi_backend_t s_esp32_spi_backend = {
    .init     = esp32_spi_init_impl,
    .write    = esp32_spi_write_impl,
    .read     = esp32_spi_read_impl,
    .transfer = esp32_spi_transfer_impl,
    .abrir    = esp32_spi_abrir,       /* V7/H6.c G4.a1 */
    .ajustar  = esp32_spi_ajustar,
    .cerrar   = esp32_spi_cerrar,
};

/* ===================== I2C (H16) =====================================
 * Backend I2C sobre el driver NUEVO de ESP-IDF (i2c_master). En IDF v6 el
 * legacy driver/i2c.h ya no existe. bus N -> I2C_NUM_N (bus 0 o 1).
 *
 * El contrato pasa la direccion en cada write/read; el i2c_master quiere un
 * "device handle" por direccion -> lo creamos ON-DEMAND (add_device +
 * transmit/receive + rm_device). Coste despreciable para sensores y permite
 * que I2c.scan() recorra 0x08..0x77 sin cachear 112 handles.
 *
 * Pull-ups: activamos el interno como red de seguridad, pero un bus I2C va
 * mejor con pull-ups EXTERNAS (las del modulo del sensor). addr de 7 bits.
 */
#define ESP32_I2C_TIMEOUT_MS 100

static i2c_master_bus_handle_t s_i2c_bus[2];
static int      s_i2c_ready[2];
static uint32_t s_i2c_baud[2];

static void esp32_i2c_init_impl(int bus, int sda, int scl, int baudrate) {
    if (bus < 0 || bus > 1) return;
    if (s_i2c_ready[bus]) { i2c_del_master_bus(s_i2c_bus[bus]); s_i2c_ready[bus] = 0; }
    i2c_master_bus_config_t cfg = {
        .i2c_port          = bus,
        .sda_io_num        = sda,
        .scl_io_num        = scl,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (i2c_new_master_bus(&cfg, &s_i2c_bus[bus]) == ESP_OK) {
        s_i2c_ready[bus] = 1;
        s_i2c_baud[bus]  = (baudrate > 0) ? (uint32_t) baudrate : 100000u;
    }
}

/* V7/H6.c G4.a1 — abrir UNA vez y cerrar (ver el SPI, arriba): `init_impl`
 * borraba el bus y lo volvía a crear, y el táctil de la C5 perdía el suyo cuando
 * un programa abría el I2C 0. La velocidad es del bus (la de la configuración). */
static int esp32_i2c_abrir(int bus, int sda, int scl, int baudrate) {
    if (bus < 0 || bus > 1) return -1;
    if (s_i2c_ready[bus]) return 0;                       /* ya abierto: NO se reabre */
    i2c_master_bus_config_t cfg = {
        .i2c_port          = bus,
        .sda_io_num        = sda,
        .scl_io_num        = scl,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (i2c_new_master_bus(&cfg, &s_i2c_bus[bus]) != ESP_OK) return -1;
    s_i2c_ready[bus] = 1;
    s_i2c_baud[bus]  = (baudrate > 0) ? (uint32_t) baudrate : 100000u;
    return 0;
}
/* V7 #669 — el bus del IDF de un I2C abierto (hw_esp32.h): el táctil de la P4 pone su dispositivo en él. */
i2c_master_bus_handle_t esp32_i2c_bus_handle(int bus) {
    return (bus >= 0 && bus <= 1 && s_i2c_ready[bus]) ? s_i2c_bus[bus] : NULL;
}

static void esp32_i2c_cerrar(int bus) {
    if (bus < 0 || bus > 1 || !s_i2c_ready[bus]) return;
    i2c_del_master_bus(s_i2c_bus[bus]);
    s_i2c_ready[bus] = 0;
}

/* add_device(addr) on-demand -> tx/rx -> rm_device. solo tx => write;
 * solo rx => read; ambos => write-then-read (repeated start). */
static int esp32_i2c_xfer(int bus, int addr, const uint8_t* tx, size_t txn,
                          uint8_t* rx, size_t rxn) {
    if (bus < 0 || bus > 1 || !s_i2c_ready[bus]) return -1;
    i2c_device_config_t dcfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = (uint16_t) addr,
        .scl_speed_hz    = s_i2c_baud[bus],
    };
    i2c_master_dev_handle_t dev;
    if (i2c_master_bus_add_device(s_i2c_bus[bus], &dcfg, &dev) != ESP_OK) return -1;
    esp_err_t e;
    if (tx && rx)  e = i2c_master_transmit_receive(dev, tx, txn, rx, rxn, ESP32_I2C_TIMEOUT_MS);
    else if (tx)   e = i2c_master_transmit(dev, tx, txn, ESP32_I2C_TIMEOUT_MS);
    else           e = i2c_master_receive(dev, rx, rxn, ESP32_I2C_TIMEOUT_MS);
    i2c_master_bus_rm_device(dev);
    return (e == ESP_OK) ? (int) (rx ? rxn : txn) : -1;
}

static int esp32_i2c_write_impl(int bus, int addr, const uint8_t* data, size_t n) {
    return esp32_i2c_xfer(bus, addr, data, n, NULL, 0);
}
static int esp32_i2c_read_impl(int bus, int addr, uint8_t* data, size_t n) {
    return esp32_i2c_xfer(bus, addr, NULL, 0, data, n);
}

/* V7/H6.c G4.a3 — LA TRANSACCIÓN: START, cada mensaje con un repeated start entre dos, y
 * STOP al final, en UNA lista de órdenes del IDF (`i2c_master_execute_defined_operations`).
 * La dirección la pone el IDF tras cada START (según lo que venga: escribir o leer). El
 * último byte de cada lectura va con NACK —el esclavo suelta la línea antes del START o del
 * STOP siguiente—, así que una lectura de más de un byte son dos órdenes. Caben 8
 * (I2C_LL_CMD_REG_NUM) y no hay forma de encadenar dos listas sin un STOP en medio: la
 * fachada ya lo ha comprobado (BPVM_I2C_ORDENES_MAX), igual en todas las familias.
 * 0, o -1 (NAK o plazo; el IDF deja el bus con su STOP). */
static int esp32_i2c_transaccion(int bus, int addr, const bpvm_i2c_msg_t* m, int nm) {
    i2c_operation_job_t op[BPVM_I2C_ORDENES_MAX];
    int k = 0, i;
    esp_err_t e;
    if (bus < 0 || bus > 1 || !s_i2c_ready[bus]) return -1;
    memset(op, 0, sizeof op);
    for (i = 0; i < nm; i++) {
        if (k + ((m[i].lee && m[i].n > 1) ? 3 : 2) + 1 > BPVM_I2C_ORDENES_MAX) return -1;   /* la fachada no lo deja pasar */
        op[k++].command = I2C_MASTER_CMD_START;
        if (!m[i].lee) {
            op[k].command = I2C_MASTER_CMD_WRITE;
            op[k].write.ack_check = true;
            op[k].write.data = m[i].buf;
            op[k].write.total_bytes = m[i].n;
            k++;
        } else {
            if (m[i].n > 1) {
                op[k].command = I2C_MASTER_CMD_READ;
                op[k].read.ack_value = I2C_ACK_VAL;
                op[k].read.data = m[i].buf;
                op[k].read.total_bytes = m[i].n - 1;
                k++;
            }
            op[k].command = I2C_MASTER_CMD_READ;
            op[k].read.ack_value = I2C_NACK_VAL;
            op[k].read.data = m[i].buf + m[i].n - 1;
            op[k].read.total_bytes = 1;
            k++;
        }
    }
    op[k++].command = I2C_MASTER_CMD_STOP;
    i2c_device_config_t dcfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = (uint16_t) addr,
        .scl_speed_hz    = s_i2c_baud[bus],
    };
    i2c_master_dev_handle_t dev;
    if (i2c_master_bus_add_device(s_i2c_bus[bus], &dcfg, &dev) != ESP_OK) return -1;
    e = i2c_master_execute_defined_operations(dev, op, (size_t) k, ESP32_I2C_TIMEOUT_MS);
    i2c_master_bus_rm_device(dev);
    return (e == ESP_OK) ? 0 : -1;
}

static const bpvm_i2c_backend_t s_esp32_i2c_backend = {
    .init   = esp32_i2c_init_impl,
    .write  = esp32_i2c_write_impl,
    .read   = esp32_i2c_read_impl,
    .abrir  = esp32_i2c_abrir,         /* V7/H6.c G4.a1 */
    .cerrar = esp32_i2c_cerrar,
    .transaccion = esp32_i2c_transaccion,   /* V7/H6.c G4.a3: repeated start */
};

/* ===================== PWM (H14) =====================================
 * Backend PWM sobre LEDC (low-speed). El "slice" BP = un canal LEDC + su timer.
 * Canales 2..7 y timers 0/2/3 — el canal 1 / timer 1 los usa el backlight del
 * display (gui_display_dsi.c). freq por TIMER, duty por CANAL. Reusa el slice
 * si se re-inicializa el MISMO pin (re-ejecución del .mod sin agotar canales).
 */
#define ESP32_PWM_MODE  LEDC_LOW_SPEED_MODE
#define ESP32_PWM_MAX   6              /* canales 2..7 */
/* V7 #667 — los que hay DE VERDAD: los canales del chip menos el 0 y el 1. La C3, la C5 y la C6 tienen 6 (2..5: cuatro);
 * la S3 y la P4, 8 (2..7: seis). Antes se daban seis en todas, y en las chicas el 5.º y el 6.º fallaban al configurar
 * el canal —con la ranura ya gastada—. */
#define ESP32_PWM_HAY   ((SOC_LEDC_CHANNEL_NUM - 2) < ESP32_PWM_MAX ? (SOC_LEDC_CHANNEL_NUM - 2) : ESP32_PWM_MAX)
/* s_pwm_n es cuántas ranuras se han estrenado; una suelta tiene el pin a -1 y la reutiliza la siguiente. No se
 * compacta: el sliceId de un programa ES el índice. */
static int s_pwm_n = 0;
static int s_pwm_pin[ESP32_PWM_MAX];
static int s_pwm_chan[ESP32_PWM_MAX];
static int s_pwm_tmr[ESP32_PWM_MAX];
static const ledc_timer_t s_pwm_free_tmr[3] = { LEDC_TIMER_0, LEDC_TIMER_2, LEDC_TIMER_3 };

static int esp32_pwm_init_impl(int pin, int freqHz) {
    int idx = -1;
    for (int i = 0; i < s_pwm_n; i++) { if (s_pwm_pin[i] == pin) { idx = i; break; } }
    if (idx < 0) for (int i = 0; i < s_pwm_n; i++) { if (s_pwm_pin[i] < 0) { idx = i; break; } }   /* V7 #667: una suelta */
    if (idx < 0) {
        if (s_pwm_n >= ESP32_PWM_HAY) return -1;
        idx = s_pwm_n++;
        s_pwm_chan[idx] = 2 + idx;                          /* canales 2..7 */
        s_pwm_tmr[idx]  = (int) s_pwm_free_tmr[idx % 3];    /* timers 0/2/3 */
    }
    s_pwm_pin[idx] = pin;
    ledc_timer_config_t tcfg = {
        .speed_mode      = ESP32_PWM_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num       = (ledc_timer_t) s_pwm_tmr[idx],
        .freq_hz         = (uint32_t) freqHz,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    if (ledc_timer_config(&tcfg) != ESP_OK) return -1;
    ledc_channel_config_t ccfg = {
        .gpio_num   = pin,
        .speed_mode = ESP32_PWM_MODE,
        .channel    = (ledc_channel_t) s_pwm_chan[idx],
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = (ledc_timer_t) s_pwm_tmr[idx],
        .duty       = 0,
        .hpoint     = 0,
    };
    if (ledc_channel_config(&ccfg) != ESP_OK) return -1;
    return idx;   /* sliceId = índice */
}

static void esp32_pwm_set_freq_impl(int slice, int freqHz) {
    if (slice < 0 || slice >= s_pwm_n || s_pwm_pin[slice] < 0) return;
    ledc_set_freq(ESP32_PWM_MODE, (ledc_timer_t) s_pwm_tmr[slice], (uint32_t) freqHz);
}

static void esp32_pwm_set_duty_impl(int slice, int pin, int dutyPct) {
    (void) pin;
    if (slice < 0 || slice >= s_pwm_n || s_pwm_pin[slice] < 0) return;
    if (dutyPct < 0)   dutyPct = 0;
    if (dutyPct > 100) dutyPct = 100;
    uint32_t maxd = (1u << 13) - 1u;               /* 13-bit */
    uint32_t d = (maxd * (uint32_t) dutyPct) / 100u;
    ledc_set_duty(ESP32_PWM_MODE, (ledc_channel_t) s_pwm_chan[slice], d);
    ledc_update_duty(ESP32_PWM_MODE, (ledc_channel_t) s_pwm_chan[slice]);
}

static void esp32_pwm_start_impl(int slice) {
    if (slice < 0 || slice >= s_pwm_n || s_pwm_pin[slice] < 0) return;
    ledc_update_duty(ESP32_PWM_MODE, (ledc_channel_t) s_pwm_chan[slice]);
}

static void esp32_pwm_stop_impl(int slice) {
    if (slice < 0 || slice >= s_pwm_n || s_pwm_pin[slice] < 0) return;
    ledc_stop(ESP32_PWM_MODE, (ledc_channel_t) s_pwm_chan[slice], 0);   /* salida a 0 */
}

/* V7 #667 — SOLTARLO: parado, el canal desconfigurado (salida fuera y el GPIO devuelto), el pin al reset y, si su
 * timer ya no lo usa ninguna otra ranura, también el timer —pausado antes: lo pide el IDF—. La ranura queda libre. */
static void esp32_pwm_release_impl(int slice, int pin) {
    (void) pin;
    if (slice < 0 || slice >= s_pwm_n || s_pwm_pin[slice] < 0) return;
    ledc_channel_t ch = (ledc_channel_t) s_pwm_chan[slice];
    ledc_stop(ESP32_PWM_MODE, ch, 0);
    ledc_channel_config_t cc = { .speed_mode = ESP32_PWM_MODE, .channel = ch, .deconfigure = true };
    ledc_channel_config(&cc);
    esp32_pin_al_reset(s_pwm_pin[slice]);
    s_pwm_pin[slice] = -1;
    int otro = 0;
    for (int i = 0; i < s_pwm_n; i++) if (s_pwm_pin[i] >= 0 && s_pwm_tmr[i] == s_pwm_tmr[slice]) otro = 1;
    if (!otro) {
        ledc_timer_pause(ESP32_PWM_MODE, (ledc_timer_t) s_pwm_tmr[slice]);
        ledc_timer_config_t tc = { .speed_mode = ESP32_PWM_MODE, .timer_num = (ledc_timer_t) s_pwm_tmr[slice], .deconfigure = true };
        ledc_timer_config(&tc);
    }
}

static int esp32_pwm_fin_de_run(void) {
    int n = 0;
    for (int i = 0; i < s_pwm_n; i++) if (s_pwm_pin[i] >= 0) { esp32_pwm_release_impl(i, s_pwm_pin[i]); n++; }
    return n;
}

static const bpvm_pwm_backend_t s_esp32_pwm_backend = {
    .init    = esp32_pwm_init_impl,
    .setFreq = esp32_pwm_set_freq_impl,
    .setDuty = esp32_pwm_set_duty_impl,
    .start   = esp32_pwm_start_impl,
    .stop    = esp32_pwm_stop_impl,
    .release    = esp32_pwm_release_impl,   /* V7 #667 */
    .fin_de_run = esp32_pwm_fin_de_run,
};

/* ===================== ADC (H14) =====================================
 * Backend ADC sobre esp_adc oneshot (ADC1). El "canal" BP = canal de ADC1
 * (0..N-1). atten 12 dB (~0..3.3 V), ancho por defecto (12 bits → raw 0..4095).
 * La unidad se crea perezosa en el 1er initChannel. readVolts() lo convierte
 * en BP (Adc.bp). Sin calibración de curva (raw lineal); suficiente para el MVP.
 */
static adc_oneshot_unit_handle_t s_adc1 = NULL;

/* #480 - EL CONTRATO SE CUMPLE O SE FALLA, PERO NO SE MIENTE.
 *
 * Estas dos devolvian 0 en TODOS sus caminos de fallo, y 0 es una lectura
 * perfectamente valida (0 V) y un pin plausible. Con `Adc.bp` lanzando solo si
 * el retorno es < 0, el resultado era el peor de los tres desenlaces: ADC roto
 * = 0 V constante y MUDO, en las cuatro familias ESP32 — y encima con backend
 * registrado, o sea que el arreglo de #469 (la ausencia de backend hace ruido)
 * no lo cubria.
 *
 * Ahora fallan con -1, que es lo que pide el contrato (bpvm_adc.h) y lo que ya
 * hacia la familia de referencia (pico/main.c: -1 si el canal no existe).
 *
 * Y de paso initChannel devuelve el GPIO DE VERDAD en vez de un 1 fijo: el
 * contrato dice "el numero de pin fisico", `Adc.Channel` lo publica en `pin_`,
 * y en un ESP32 ese 1 era otra mentira pequena. El IDF lo sabe:
 * adc_oneshot_channel_to_io. */
static int esp32_adc_init_channel_impl(int ch) {
    if (ch < 0 || ch >= SOC_ADC_CHANNEL_NUM(ADC_UNIT_1)) return -1;
    if (!s_adc1) {
        adc_oneshot_unit_init_cfg_t ucfg = { .unit_id = ADC_UNIT_1 };
        if (adc_oneshot_new_unit(&ucfg, &s_adc1) != ESP_OK) { s_adc1 = NULL; return -1; }
    }
    adc_oneshot_chan_cfg_t ccfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_oneshot_config_channel(s_adc1, (adc_channel_t) ch, &ccfg) != ESP_OK) return -1;
    int io = -1;
    if (adc_oneshot_channel_to_io(ADC_UNIT_1, (adc_channel_t) ch, &io) != ESP_OK) return -1;
    return io;                      /* el GPIO fisico, como en la Pico */
}

static int esp32_adc_read_channel_impl(int ch) {
    if (!s_adc1) return -1;         /* nadie llamo a initChannel, o fallo */
    int raw = 0;
    if (adc_oneshot_read(s_adc1, (adc_channel_t) ch, &raw) != ESP_OK) return -1;
    return raw;
}

static const bpvm_adc_backend_t s_esp32_adc_backend = {
    .initChannel = esp32_adc_init_channel_impl,
    .readChannel = esp32_adc_read_channel_impl,
};

/* ===================== CONTADOR DE PULSOS (H14) =====================
 * Backend sobre PCNT (el periférico contador del ESP32; el P4 no tiene PIO).
 * Cada "counter" BP = una unidad PCNT + 1 canal sobre el pin. Cuenta flancos
 * RISING(0)/FALLING(1)/BOTH(2). Contador 16-bit con signo (±32767) — como el
 * RP2350. Reusa el counter si se re-inicializa el MISMO pin.
 */
#if SOC_PCNT_SUPPORTED
#define ESP32_PULSE_MAX 4
static int                   s_pulse_n = 0;
static int                   s_pulse_pin[ESP32_PULSE_MAX];
static pcnt_unit_handle_t    s_pulse_unit[ESP32_PULSE_MAX];
static pcnt_channel_handle_t s_pulse_chan[ESP32_PULSE_MAX];

static int esp32_pulse_init_impl(int pin, int edgeKind) {
    for (int i = 0; i < s_pulse_n; i++) {
        if (s_pulse_pin[i] == pin) { pcnt_unit_clear_count(s_pulse_unit[i]); return i; }
    }
    int libre = -1;   /* V7 #667: una ranura suelta (pin a -1) se reutiliza; no se compacta: el id ES el índice */
    for (int i = 0; i < s_pulse_n; i++) if (s_pulse_pin[i] < 0) { libre = i; break; }
    if (libre < 0 && s_pulse_n >= ESP32_PULSE_MAX) return -1;
    pcnt_unit_config_t ucfg = { .high_limit = 32767, .low_limit = -32768 };
    pcnt_unit_handle_t unit;
    if (pcnt_new_unit(&ucfg, &unit) != ESP_OK) return -1;
    pcnt_chan_config_t ccfg = { .edge_gpio_num = pin, .level_gpio_num = -1 };
    pcnt_channel_handle_t chan;
    if (pcnt_new_channel(unit, &ccfg, &chan) != ESP_OK) { pcnt_del_unit(unit); return -1; }
    pcnt_channel_edge_action_t pos = PCNT_CHANNEL_EDGE_ACTION_HOLD;
    pcnt_channel_edge_action_t neg = PCNT_CHANNEL_EDGE_ACTION_HOLD;
    if (edgeKind == 1)      { neg = PCNT_CHANNEL_EDGE_ACTION_INCREASE; }   /* FALLING */
    else if (edgeKind == 2) { pos = neg = PCNT_CHANNEL_EDGE_ACTION_INCREASE; } /* BOTH */
    else                    { pos = PCNT_CHANNEL_EDGE_ACTION_INCREASE; }   /* RISING (0) */
    pcnt_channel_set_edge_action(chan, pos, neg);
    pcnt_unit_enable(unit);
    pcnt_unit_clear_count(unit);
    int idx = (libre >= 0) ? libre : s_pulse_n++;
    s_pulse_pin[idx] = pin; s_pulse_unit[idx] = unit; s_pulse_chan[idx] = chan;
    return idx;
}
static int  esp32_pulse_vivo(int id) { return id >= 0 && id < s_pulse_n && s_pulse_pin[id] >= 0; }   /* V7 #667 */
static void esp32_pulse_start_impl(int id) { if (esp32_pulse_vivo(id)) pcnt_unit_start(s_pulse_unit[id]); }
static void esp32_pulse_stop_impl(int id)  { if (esp32_pulse_vivo(id)) pcnt_unit_stop(s_pulse_unit[id]); }
static int  esp32_pulse_value_impl(int id) {
    if (!esp32_pulse_vivo(id)) return 0;
    int c = 0; pcnt_unit_get_count(s_pulse_unit[id], &c); return c;
}
static void esp32_pulse_reset_impl(int id) { if (esp32_pulse_vivo(id)) pcnt_unit_clear_count(s_pulse_unit[id]); }

/* V7 #667 — SOLTARLO: parada, deshabilitada, su canal y su unidad borrados (vuelven al IDF) y el pin al reset. La
 * ranura queda libre. */
static void esp32_pulse_release_impl(int id, int pin) {
    (void) pin;
    if (!esp32_pulse_vivo(id)) return;
    pcnt_unit_stop(s_pulse_unit[id]);
    pcnt_unit_disable(s_pulse_unit[id]);
    pcnt_del_channel(s_pulse_chan[id]);
    pcnt_del_unit(s_pulse_unit[id]);
    esp32_pin_al_reset(s_pulse_pin[id]);
    s_pulse_pin[id] = -1;
}

static int esp32_pulse_fin_de_run(void) {
    int n = 0;
    for (int i = 0; i < s_pulse_n; i++) if (s_pulse_pin[i] >= 0) { esp32_pulse_release_impl(i, s_pulse_pin[i]); n++; }
    return n;
}

static const bpvm_pulse_backend_t s_esp32_pulse_backend = {
    .init  = esp32_pulse_init_impl,
    .start = esp32_pulse_start_impl,
    .stop  = esp32_pulse_stop_impl,
    .value = esp32_pulse_value_impl,
    .reset = esp32_pulse_reset_impl,
    .release    = esp32_pulse_release_impl,   /* V7 #667 */
    .fin_de_run = esp32_pulse_fin_de_run,
};
#endif  /* SOC_PCNT_SUPPORTED */

/* ===================== RTC (H14) ====================================
 * El ESP32 usa A PROPÓSITO el stub portable de src/rtc.c (NO registra backend):
 * wall clock = reloj monotónico (bpvm_platform_now_ms, esp_timer) + offset de
 * calibración. El IDE lo calibra por el comando TIME del wire (repl_esp32) al
 * conectar, exactamente igual que el Pico (que también cae al stub) y que miVM
 * → paridad garantizada y verificable en host.
 *
 * NO reintroducir un backend con gettimeofday/settimeofday: en el P4 (26-jun)
 * gettimeofday NO avanzaba tras un settimeofday (quirk del time syscall de
 * newlib según sdkconfig) → tras calibrar, el reloj se quedaba congelado. El
 * modelo offset evita ese camino por completo y da la hora correcta.
 */

/* ============================ Wdt (cierre V3) =======================
 * Watchdog del ESP32 vía el Task Watchdog Timer (TWDT) de ESP-IDF. Vigila la
 * task que ejecuta el código BP (la que llama a Wdt.enable): si deja de
 * "alimentarlo" (feed) dentro del timeout, el TWDT dispara un panic que —con la
 * config de panic por defecto (print+reboot)— RESETEA el chip, igual que el
 * watchdog del Pico (HW) y del STM32 (IWDG). Paridad de comportamiento.
 *
 * Semántica (= Pico/STM32):
 *   enable(timeoutMs): arranca/reconfigura el TWDT con ese timeout y suscribe la
 *                      task actual; trigger_panic=true → timeout = reset.
 *   feed():            esp_task_wdt_reset() (resetea el contador de la task).
 *   disable():         des-suscribe la task (mejor esfuerzo; el TWDT global del
 *                      IDF puede seguir vigilando otras tasks, lo deseable).
 *   Si el programa BP termina SIN disable, la task deja de alimentar y el chip se
 *   resetea — MISMO aviso de seguridad que en Pico/STM32.
 *
 * El IDF puede haber iniciado ya el TWDT (CONFIG_ESP_TASK_WDT_INIT): en ese caso
 * esp_task_wdt_init devuelve ESP_ERR_INVALID_STATE y reconfiguramos.
 * (Nota: si el sdkconfig usa un panic-handler que NO reinicia —HALT/GDBSTUB— el
 * timeout pararía en vez de resetear; el default PRINT_REBOOT sí resetea.)
 */
static bool s_esp32_wdt_subscribed = false;

static void esp32_wdt_enable_impl(int timeoutMs) {
    if (timeoutMs < 1) timeoutMs = 1;
    esp_task_wdt_config_t cfg = {
        .timeout_ms     = (uint32_t) timeoutMs,
        .idle_core_mask = 0,        /* solo vigilamos la task BP suscrita, no los idle */
        .trigger_panic  = true,     /* timeout → panic → reboot (= reset Pico/STM32) */
    };
    esp_err_t err = esp_task_wdt_init(&cfg);
    if (err == ESP_ERR_INVALID_STATE) {
        esp_task_wdt_reconfigure(&cfg);     /* el IDF ya lo había iniciado al boot */
    }
    if (!s_esp32_wdt_subscribed && esp_task_wdt_add(NULL) == ESP_OK) {
        s_esp32_wdt_subscribed = true;
    }
    esp_task_wdt_reset();
}

static void esp32_wdt_feed_impl(void) {
    if (s_esp32_wdt_subscribed) esp_task_wdt_reset();
}

static void esp32_wdt_disable_impl(void) {
    if (s_esp32_wdt_subscribed) {
        esp_task_wdt_delete(NULL);
        s_esp32_wdt_subscribed = false;
    }
}

static const bpvm_wdt_backend_t s_esp32_wdt_backend = {
    .enable  = esp32_wdt_enable_impl,
    .feed    = esp32_wdt_feed_impl,
    .disable = esp32_wdt_disable_impl,
};

/* ══════════ V7/H4 (26-sep) — EL NEOPIXEL (WS2812) POR EL RMT, en toda la familia ══════════
 * El contrato es el de la Pico (bpvm_neopixel.h): init(pin) y show(pin, grb, n) BLOQUEANTE, palabras GRB de 24 bits
 * que salen desde el más alto. Aquí, con el driver RMT del NÚCLEO del IDF —decidido por Eduardo: no el componente
 * `led_strip`, que se descarga del registro— y un *simple encoder* que traduce las palabras a símbolos AL VUELO: sin
 * copia ni búfer. A 20 MHz (0,05 µs por tic) salen tal cual los tiempos de la hoja del WS2812B: el 0, 0,40 alto y
 * 0,85 bajo; el 1, 0,80 y 0,45 (±0,15). Al final de la trama, el RESET como un símbolo bajo de 300 µs (el WS2812B de
 * hoy pide ≥ 280), así que al volver de `rmt_tx_wait_all_done` la tira ya ha latcheado.
 * Canales de transmisión (leídos en el IDF): S3 y P4 4, C3, C5 y C6 2 → tantas tiras a la vez. Un LED (24 símbolos
 * y el reset) cabe en la memoria del canal (48): sin recargas. Una tira larga se recarga por interrupción, y una
 * escritura en la flash (que para la caché) podría retrasarla y deformar la trama (V7_IDEAS §H4.5.1). */
#if SOC_RMT_SUPPORTED
#define NP_RES_HZ     20000000u        /* 0,05 µs por tic */
#define NP_MAX_TIRAS  4                /* el mayor nº de canales de transmisión de la familia (S3, P4) */

static const rmt_symbol_word_t NP_CERO  = { .level0 = 1, .duration0 = 8,    .level1 = 0, .duration1 = 17 };   /* 0,40 / 0,85 µs */
static const rmt_symbol_word_t NP_UNO   = { .level0 = 1, .duration0 = 16,   .level1 = 0, .duration1 = 9 };    /* 0,80 / 0,45 µs */
static const rmt_symbol_word_t NP_RESET = { .level0 = 0, .duration0 = 3000, .level1 = 0, .duration1 = 3000 }; /* 300 µs bajo */

static struct { int pin; rmt_channel_handle_t ch; rmt_encoder_handle_t enc; } s_np[NP_MAX_TIRAS];
static int s_np_n = 0;

/* El traductor: de LED en LED —con min_chunk_size = 24 el encoder garantiza sitio para uno— y el reset al final.
 * `escritos` es la posición en la trama, en símbolos: múltiplo de 24 hasta que entra el reset. */
static size_t np_traduce(const void* data, size_t data_size, size_t escritos, size_t libres,
                         rmt_symbol_word_t* sym, bool* hecho, void* arg) {
    const uint32_t* grb = (const uint32_t*) data;
    size_t leds = data_size / sizeof(uint32_t), led = escritos / 24u, n = 0;
    (void) arg;
    while (led < leds && libres - n >= 24u) {
        uint32_t c = grb[led++];
        for (int b = 23; b >= 0; b--) sym[n++] = ((c >> b) & 1u) ? NP_UNO : NP_CERO;
    }
    if (led == leds && libres - n >= 1u) { sym[n++] = NP_RESET; *hecho = true; }
    return n;
}

static int np_busca(int pin) {
    for (int i = 0; i < s_np_n; i++) if (s_np[i].pin == pin) return i;
    return -1;
}

/* V7 #667 — soltar la tira i: el canal RMT (y su memoria, su cola y su interrupción) y el traductor, de vuelta al IDF;
 * la entrada, fuera de la tabla (se buscan por pin: el orden no importa). El pin lo deja quien llama. */
static void np_suelta(int i) {
    rmt_disable(s_np[i].ch);
    rmt_del_channel(s_np[i].ch);
    rmt_del_encoder(s_np[i].enc);
    s_np[i] = s_np[--s_np_n];
}

static int esp32_np_init_impl(int pin, char* err, size_t n) {
    int i = np_busca(pin);
    if (i >= 0) {
        /* Ya iniciado: se REHACE. Un `Gpio.Pin` sobre este pin entre medias reconfigura el pad, y el RMT dejaría de
         * llegar al cable sin un error. */
        np_suelta(i);
    }
    if (s_np_n >= NP_MAX_TIRAS) { snprintf(err, n, "como mucho %d tiras a la vez", NP_MAX_TIRAS); return 0; }
    rmt_tx_channel_config_t cc = {
        .gpio_num = pin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = NP_RES_HZ,
        .mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL,
        .trans_queue_depth = 1,
    };
    rmt_channel_handle_t ch = NULL;
    esp_err_t e = rmt_new_tx_channel(&cc, &ch);
    if (e != ESP_OK) {
        if (e == ESP_ERR_NOT_FOUND) snprintf(err, n, "no quedan canales RMT libres");
        else snprintf(err, n, "el RMT no lo aceptó (%s)", esp_err_to_name(e));
        return 0;
    }
    rmt_simple_encoder_config_t ec = { .callback = np_traduce, .arg = NULL, .min_chunk_size = 24 };
    rmt_encoder_handle_t enc = NULL;
    e = rmt_new_simple_encoder(&ec, &enc);
    if (e == ESP_OK) e = rmt_enable(ch);
    if (e != ESP_OK) {
        if (enc) rmt_del_encoder(enc);
        rmt_del_channel(ch);
        snprintf(err, n, "el RMT no arrancó (%s)", esp_err_to_name(e));
        return 0;
    }
    s_np[s_np_n].pin = pin; s_np[s_np_n].ch = ch; s_np[s_np_n].enc = enc; s_np_n++;
    return 1;
}

static void esp32_np_show_impl(int pin, const uint32_t* grb, int count) {
    int i = np_busca(pin);
    if (i < 0 || grb == NULL || count <= 0) return;
    rmt_transmit_config_t tc = { .loop_count = 0 };
    esp_err_t e = rmt_transmit(s_np[i].ch, s_np[i].enc, grb, (size_t) count * sizeof(uint32_t), &tc);
    /* BLOQUEANTE, como la Pico: el builtin suelta la zona de rascar al volver. 30 µs por LED y el reset: de sobra. */
    if (e == ESP_OK) e = rmt_tx_wait_all_done(s_np[i].ch, 100 + count / 20);
    if (e != ESP_OK) log_printf("neopixel: la trama del GPIO%d no salió (%s)", pin, esp_err_to_name(e));
}

/* V7 #667 — SOLTARLA (la Strip la apagó antes: show() es bloqueante, así que la trama ya salió entera) y su pin, al
 * reset. */
static void esp32_np_release_impl(int pin) {
    int i = np_busca(pin);
    if (i < 0) return;
    np_suelta(i);
    esp32_pin_al_reset(pin);
}

static int esp32_np_fin_de_run(void) {
    int n = 0;
    while (s_np_n > 0) { esp32_np_release_impl(s_np[0].pin); n++; }
    return n;
}

static const bpvm_neopixel_backend_t s_esp32_np_backend = {
    .init = esp32_np_init_impl,
    .show = esp32_np_show_impl,
    .release    = esp32_np_release_impl,   /* V7 #667 */
    .fin_de_run = esp32_np_fin_de_run,
};
#endif /* SOC_RMT_SUPPORTED */

void esp32_hw_register(void) {
    esp32_caps_init();                                 /* V7/H4: la tabla, preguntada al IDF */
    bpvm_gpio_set_backend(&s_esp32_gpio_backend);
    bpvm_machine_set_backend(&s_esp32_pico_backend);   /* info real (no stub host) */
    bpvm_uart_set_backend(&s_esp32_uart_backend);   /* H16 */
    bpvm_spi_set_backend(&s_esp32_spi_backend);     /* H16 */
    bpvm_i2c_set_backend(&s_esp32_i2c_backend);     /* H16 */
    bpvm_pwm_set_backend(&s_esp32_pwm_backend);     /* H14 (LEDC) */
    bpvm_adc_set_backend(&s_esp32_adc_backend);     /* H14 (esp_adc oneshot) */
#if SOC_PCNT_SUPPORTED
    bpvm_pulse_set_backend(&s_esp32_pulse_backend); /* H14 (PCNT) */
#else
    /* #465 — sin PCNT (C3) no se registra backend: `src/pulse.c` deja su stub
     * portable, que es lo que ya hacen las familias sin el periférico. Mejor
     * un verbo que dice que no puede que un enlace que no cierra. */
#endif
    bpvm_wdt_set_backend(&s_esp32_wdt_backend);     /* cierre V3 — Task WDT (reset) */
#if SOC_RMT_SUPPORTED
    bpvm_neopixel_set_backend(&s_esp32_np_backend); /* V7/H4 (26-sep) — WS2812 por RMT; lo prueba la S3 */
#endif
    /* RTC: sin backend a propósito → stub portable (modelo offset). Ver arriba. */
}
