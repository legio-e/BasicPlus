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
#include "hw_esp32.h"
#include "bpvm_gpio.h"
#include "bpvm_pico.h"
#include "bpvm_uart.h"
#include "bpvm_spi.h"
#include "bpvm_i2c.h"
#include "bpvm_pwm.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"        /* H14: backend PWM */
#include "freertos/FreeRTOS.h"   /* pdMS_TO_TICKS, portMAX_DELAY */
#include "esp_mac.h"      /* uniqueId desde la MAC de efuse */
#include "esp_timer.h"    /* uptime */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static void esp32_gpio_init_impl(int pin, int mode) {
    gpio_num_t g = (gpio_num_t) pin;
    gpio_reset_pin(g);
    gpio_set_direction(g, (mode == 1) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
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

static const bpvm_gpio_backend_t s_esp32_gpio_backend = {
    .init  = esp32_gpio_init_impl,
    .pull  = esp32_gpio_pull_impl,
    .write = esp32_gpio_write_impl,
    .read  = esp32_gpio_read_impl,
};

/* ---- Pico (info de MCU) backend ----
 * Igual que el STM32 (s_pico_backend en gpio_stm32.c): da datos reales del
 * ESP32-S3 a los builtins Pico.* desde BP (boardName/uniqueId/cpuFreqHz/
 * gpioCount...). Sin esto, Pico.gpioCount() caia al stub host (30 GPIO,
 * board "host") aunque el INFO del wire ya diera los valores reales.
 * ESP32-S3 (DevKitC, WROOM-1): 45 GPIO utiles (0-21, 26-48), 240 MHz. */
static void esp32_unique_id_impl(char* buf, size_t len) {
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);   /* MAC base de fabrica (efuse) */
    if (buf && len > 0) {
        snprintf(buf, len, "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

static void esp32_board_name_impl(char* buf, size_t len) {
    if (buf && len > 0) {
        strncpy(buf, "esp32s3-devkitc", len - 1);
        buf[len - 1] = '\0';
    }
}

static float esp32_temp_c_impl(void) {
    return 0.0f;   /* sensor interno no cableado en el MVP (igual que STM32) */
}

static int esp32_cpu_freq_hz_impl(void) {
    return 240000000;   /* 240 MHz nominal del ESP32-S3 (sdkconfig.defaults) */
}

static int esp32_uptime_ms_impl(void) {
    return (int) (esp_timer_get_time() / 1000);   /* us -> ms */
}

static int esp32_set_cpu_freq_mhz_impl(int mhz) {
    (void) mhz;
    return 0;   /* cambiar el reloj en runtime no soportado */
}

static int esp32_gpio_count_impl(void) {
    return 45;   /* GPIO utiles del ESP32-S3 (0-21, 26-48) */
}

static const bpvm_pico_backend_t s_esp32_pico_backend = {
    .uniqueId      = esp32_unique_id_impl,
    .boardName     = esp32_board_name_impl,
    .tempC         = esp32_temp_c_impl,
    .cpuFreqHz     = esp32_cpu_freq_hz_impl,
    .uptimeMs      = esp32_uptime_ms_impl,
    .setCpuFreqMHz = esp32_set_cpu_freq_mhz_impl,
    .gpioCount     = esp32_gpio_count_impl,
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
    switch (bus) {
        case 1: return UART_NUM_1;
        case 2: return UART_NUM_2;
        default: return -1;   /* 0 = wire (reservado); el resto no existe */
    }
}

static void esp32_uart_init_impl(int bus, int tx, int rx, int baudrate,
                                 int data_bits, int stop_bits, int parity) {
    int p = esp32_uart_port(bus);
    if (p < 0) return;
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

static const bpvm_uart_backend_t s_esp32_uart_backend = {
    .init      = esp32_uart_init_impl,
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
    switch (bus) {
        case 2: return SPI2_HOST;
        case 3: return SPI3_HOST;
        default: return -1;   /* 0/1 = flash (reservados) */
    }
}

static spi_device_handle_t s_spi_dev[4];   /* por bus; 2 y 3 usados */
static int s_spi_bus_ready[4];

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
        .max_transfer_sz = 0,   /* sin DMA: limite = FIFO interno (~64 B) */
    };
    if (spi_bus_initialize((spi_host_device_t) host, &buscfg, SPI_DMA_DISABLED) != ESP_OK) return;
    s_spi_bus_ready[bus] = 1;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = baudrate,
        .mode           = mode,    /* 0..3 = CPOL/CPHA, igual que la fachada */
        .spics_io_num   = -1,      /* CS lo lleva el usuario por GPIO */
        .queue_size     = 1,
    };
    spi_bus_add_device((spi_host_device_t) host, &devcfg, &s_spi_dev[bus]);
}

/* tx NULL -> manda ceros; rx NULL -> descarta lo recibido. Full-duplex. */
static int esp32_spi_xfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    if (bus < 0 || bus >= 4 || !s_spi_dev[bus] || n == 0) return -1;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length    = n * 8;          /* en BITS */
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    return (spi_device_transmit(s_spi_dev[bus], &t) == ESP_OK) ? (int) n : -1;
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

static const bpvm_i2c_backend_t s_esp32_i2c_backend = {
    .init  = esp32_i2c_init_impl,
    .write = esp32_i2c_write_impl,
    .read  = esp32_i2c_read_impl,
};

/* ===================== PWM (H14) =====================================
 * Backend PWM sobre LEDC (low-speed). El "slice" BP = un canal LEDC + su timer.
 * Canales 2..7 y timers 0/2/3 — el canal 1 / timer 1 los usa el backlight del
 * display (gui_display_dsi.c). freq por TIMER, duty por CANAL. Reusa el slice
 * si se re-inicializa el MISMO pin (re-ejecución del .mod sin agotar canales).
 */
#define ESP32_PWM_MODE  LEDC_LOW_SPEED_MODE
#define ESP32_PWM_MAX   6              /* canales 2..7 */
static int s_pwm_n = 0;
static int s_pwm_pin[ESP32_PWM_MAX];
static int s_pwm_chan[ESP32_PWM_MAX];
static int s_pwm_tmr[ESP32_PWM_MAX];
static const ledc_timer_t s_pwm_free_tmr[3] = { LEDC_TIMER_0, LEDC_TIMER_2, LEDC_TIMER_3 };

static int esp32_pwm_init_impl(int pin, int freqHz) {
    int idx = -1;
    for (int i = 0; i < s_pwm_n; i++) { if (s_pwm_pin[i] == pin) { idx = i; break; } }
    if (idx < 0) {
        if (s_pwm_n >= ESP32_PWM_MAX) return -1;
        idx = s_pwm_n++;
        s_pwm_pin[idx]  = pin;
        s_pwm_chan[idx] = 2 + idx;                          /* canales 2..7 */
        s_pwm_tmr[idx]  = (int) s_pwm_free_tmr[idx % 3];    /* timers 0/2/3 */
    }
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
    if (slice < 0 || slice >= s_pwm_n) return;
    ledc_set_freq(ESP32_PWM_MODE, (ledc_timer_t) s_pwm_tmr[slice], (uint32_t) freqHz);
}

static void esp32_pwm_set_duty_impl(int slice, int pin, int dutyPct) {
    (void) pin;
    if (slice < 0 || slice >= s_pwm_n) return;
    if (dutyPct < 0)   dutyPct = 0;
    if (dutyPct > 100) dutyPct = 100;
    uint32_t maxd = (1u << 13) - 1u;               /* 13-bit */
    uint32_t d = (maxd * (uint32_t) dutyPct) / 100u;
    ledc_set_duty(ESP32_PWM_MODE, (ledc_channel_t) s_pwm_chan[slice], d);
    ledc_update_duty(ESP32_PWM_MODE, (ledc_channel_t) s_pwm_chan[slice]);
}

static void esp32_pwm_start_impl(int slice) {
    if (slice < 0 || slice >= s_pwm_n) return;
    ledc_update_duty(ESP32_PWM_MODE, (ledc_channel_t) s_pwm_chan[slice]);
}

static void esp32_pwm_stop_impl(int slice) {
    if (slice < 0 || slice >= s_pwm_n) return;
    ledc_stop(ESP32_PWM_MODE, (ledc_channel_t) s_pwm_chan[slice], 0);   /* salida a 0 */
}

static const bpvm_pwm_backend_t s_esp32_pwm_backend = {
    .init    = esp32_pwm_init_impl,
    .setFreq = esp32_pwm_set_freq_impl,
    .setDuty = esp32_pwm_set_duty_impl,
    .start   = esp32_pwm_start_impl,
    .stop    = esp32_pwm_stop_impl,
};

void esp32_hw_register(void) {
    bpvm_gpio_set_backend(&s_esp32_gpio_backend);
    bpvm_pico_set_backend(&s_esp32_pico_backend);   /* info real (no stub host) */
    bpvm_uart_set_backend(&s_esp32_uart_backend);   /* H16 */
    bpvm_spi_set_backend(&s_esp32_spi_backend);     /* H16 */
    bpvm_i2c_set_backend(&s_esp32_i2c_backend);     /* H16 */
    bpvm_pwm_set_backend(&s_esp32_pwm_backend);     /* H14 (LEDC) */
}
