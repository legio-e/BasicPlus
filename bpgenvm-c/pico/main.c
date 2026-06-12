/*
 * main.c — firmware bpgenvm-c para Pico 2 (RP2350) sobre FreeRTOS.
 *
 * Versión FP1-fix (baseline funcional): ejecuta Hello.mod embebido en
 * bucle, imprimiendo el output por USB CDC. Sirve como punto de
 * partida estable para iterar FP2 (FS + REPL) por capas.
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/gpio.h"

#include "bpvm.h"
#include "embedded_mods.h"
#include "fs.h"
#include "board_desc.h"
#include "psram.h"
#include "neopixel.h"
#include "bpvm_neopixel.h"
#include "repl.h"
#include "repl_v1.h"   /* P-autorun (#256) */
#include "log.h"
#include "bench.h"
#include "bpvm_gpio.h"
#include "bpvm_i2c.h"
#include "bpvm_spi.h"
#include "bpvm_uart.h"
#include "bpvm_pulse.h"
#include "bpvm_pwm.h"
#include "bpvm_pico.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "pico/unique_id.h"
#include "pico/time.h"

/* Buffer estático de la VM. 128 KB sobra para Hello.bp y deja sitio para
 * varios módulos. */
#define VM_BUFFER_SIZE   (128 * 1024)
/* H7.2.b — el heap de la VM es un PUNTERO elegido en boot: SRAM interna por
 * defecto (este array), o la ventana PSRAM (MBs) si hay PSRAM usable. El buffer
 * SRAM sigue existiendo como fallback (Pico sin PSRAM). */
static uint8_t s_sram_buffer[VM_BUFFER_SIZE] __attribute__((aligned(8)));
uint8_t* s_vm_buffer      = s_sram_buffer;
uint32_t s_vm_buffer_size = VM_BUFFER_SIZE;

/* --- LED on-board (GP25 en Pico 2 igual que Pico 1) -------------- */
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

static void led_init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}

static void led_set(int on) { gpio_put(PICO_DEFAULT_LED_PIN, on ? 1 : 0); }

/* --- Backend GPIO del Pico SDK conectado a los builtins Gpio.* ----
 *
 * La VM C llama bpvm_gpio_init/write/read/pull; aquí redirigimos esos
 * hooks a las funciones del Pico SDK, que tocan el hardware real.
 *
 * Reglas para BP:
 *   - mode: 0 = INPUT (gpio_set_dir(pin, GPIO_IN))
 *           1 = OUTPUT (gpio_set_dir(pin, GPIO_OUT))
 *   - value: 0 = LOW, !=0 = HIGH
 *   - pull: 0 = none (disable_pulls), 1 = up, 2 = down
 */
static void pico_gpio_init_impl(int pin, int mode) {
    gpio_init((uint) pin);
    gpio_set_dir((uint) pin, mode == 1 ? GPIO_OUT : GPIO_IN);
}
static void pico_gpio_pull_impl(int pin, int pull_mode) {
    switch (pull_mode) {
        case 1:  gpio_pull_up((uint) pin);    break;
        case 2:  gpio_pull_down((uint) pin);  break;
        default: gpio_disable_pulls((uint) pin); break;
    }
}
static void pico_gpio_write_impl(int pin, int value) {
    gpio_put((uint) pin, value != 0);
}
static int  pico_gpio_read_impl(int pin) {
    return gpio_get((uint) pin) ? 1 : 0;
}

static const bpvm_gpio_backend_t s_pico_gpio_backend = {
    .init  = pico_gpio_init_impl,
    .pull  = pico_gpio_pull_impl,
    .write = pico_gpio_write_impl,
    .read  = pico_gpio_read_impl,
};

/* --- Backend NeoPixel (WS2812 vía PIO, H7.4) -------------------- */
static int  pico_np_init_impl(int pin) { return neopixel_init(pin) ? 1 : 0; }
static void pico_np_show_impl(int pin, const uint32_t* grb, int count) {
    neopixel_show(pin, grb, count);
}
static const bpvm_neopixel_backend_t s_pico_neopixel_backend = {
    .init = pico_np_init_impl,
    .show = pico_np_show_impl,
};

/* --- Backend I2C del Pico SDK ----------------------------------- */
static i2c_inst_t* i2c_inst_for(int bus) {
    return (bus == 1) ? i2c1 : i2c0;
}
static void pico_i2c_init_impl(int bus, int sda, int scl, int baud) {
    i2c_inst_t* inst = i2c_inst_for(bus);
    i2c_init(inst, (uint) baud);
    gpio_set_function((uint) sda, GPIO_FUNC_I2C);
    gpio_set_function((uint) scl, GPIO_FUNC_I2C);
    gpio_pull_up((uint) sda);
    gpio_pull_up((uint) scl);
}
static int pico_i2c_write_impl(int bus, int addr, const uint8_t* data, size_t n) {
    i2c_inst_t* inst = i2c_inst_for(bus);
    int r = i2c_write_blocking(inst, (uint8_t) addr, data, n, false);
    return (r < 0) ? -1 : r;
}
static int pico_i2c_read_impl(int bus, int addr, uint8_t* data, size_t n) {
    i2c_inst_t* inst = i2c_inst_for(bus);
    int r = i2c_read_blocking(inst, (uint8_t) addr, data, n, false);
    return (r < 0) ? -1 : r;
}
static const bpvm_i2c_backend_t s_pico_i2c_backend = {
    .init  = pico_i2c_init_impl,
    .write = pico_i2c_write_impl,
    .read  = pico_i2c_read_impl,
};

/* --- Backend SPI del Pico SDK ----------------------------------- */
static spi_inst_t* spi_inst_for(int bus) {
    return (bus == 1) ? spi1 : spi0;
}
static void pico_spi_init_impl(int bus, int sck, int mosi, int miso, int baud, int mode) {
    spi_inst_t* inst = spi_inst_for(bus);
    spi_init(inst, (uint) baud);
    /* mode 0..3 → (CPOL, CPHA). */
    spi_cpol_t  cpol = (mode & 2) ? SPI_CPOL_1 : SPI_CPOL_0;
    spi_cpha_t  cpha = (mode & 1) ? SPI_CPHA_1 : SPI_CPHA_0;
    spi_set_format(inst, 8, cpol, cpha, SPI_MSB_FIRST);
    gpio_set_function((uint) sck,  GPIO_FUNC_SPI);
    gpio_set_function((uint) mosi, GPIO_FUNC_SPI);
    gpio_set_function((uint) miso, GPIO_FUNC_SPI);
}
static int pico_spi_write_impl(int bus, const uint8_t* data, size_t n) {
    spi_inst_t* inst = spi_inst_for(bus);
    int r = spi_write_blocking(inst, data, n);
    return (r < 0) ? -1 : r;
}
static int pico_spi_read_impl(int bus, uint8_t* data, size_t n) {
    spi_inst_t* inst = spi_inst_for(bus);
    int r = spi_read_blocking(inst, 0xFF, data, n);
    return (r < 0) ? -1 : r;
}
static int pico_spi_transfer_impl(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    spi_inst_t* inst = spi_inst_for(bus);
    int r = spi_write_read_blocking(inst, tx, rx, n);
    return (r < 0) ? -1 : r;
}
static const bpvm_spi_backend_t s_pico_spi_backend = {
    .init     = pico_spi_init_impl,
    .write    = pico_spi_write_impl,
    .read     = pico_spi_read_impl,
    .transfer = pico_spi_transfer_impl,
};

/* --- Backend UART del Pico SDK ----------------------------------- */
static uart_inst_t* uart_inst_for(int bus) {
    return (bus == 1) ? uart1 : uart0;
}
static void pico_uart_init_impl(int bus, int tx, int rx, int baud,
                                 int data_bits, int stop_bits, int parity) {
    uart_inst_t* inst = uart_inst_for(bus);
    uart_init(inst, (uint) baud);
    /* Configurar pines TX/RX a la función UART. RP2350 deja que
     * cualquiera de varios pares sirva — el SDK lo resuelve por la
     * tabla de funciones por pin. */
    gpio_set_function((uint) tx, GPIO_FUNC_UART);
    gpio_set_function((uint) rx, GPIO_FUNC_UART);
    /* Formato del carácter. parity: 0=NONE, 1=ODD, 2=EVEN. */
    uart_parity_t p = UART_PARITY_NONE;
    if (parity == 1) p = UART_PARITY_ODD;
    else if (parity == 2) p = UART_PARITY_EVEN;
    uart_set_format(inst, data_bits, stop_bits, p);
    /* Sin flow control HW (3-wire TX/RX/GND). */
    uart_set_hw_flow(inst, false, false);
    /* FIFO ON para suavizar bursts. */
    uart_set_fifo_enabled(inst, true);
}
static int pico_uart_write_impl(int bus, const uint8_t* data, size_t n) {
    uart_inst_t* inst = uart_inst_for(bus);
    /* uart_write_blocking del SDK no devuelve nada — escribe todo el
     * buffer bloqueando cuando el TX FIFO se llena. */
    uart_write_blocking(inst, data, n);
    return (int) n;
}
static int pico_uart_read_impl(int bus, uint8_t* data, size_t n, int timeout_ms) {
    uart_inst_t* inst = uart_inst_for(bus);
    if (timeout_ms <= 0) {
        /* Bloqueante puro: garantiza n bytes. */
        uart_read_blocking(inst, data, n);
        return (int) n;
    }
    /* Con timeout total: leemos byte a byte usando
     * uart_is_readable_within_us, que espera hasta `us` por el
     * siguiente char. Si expira, devolvemos lo que llevemos. */
    size_t got = 0;
    uint64_t budget_us = (uint64_t) timeout_ms * 1000ULL;
    /* Repartimos el budget global entre los bytes restantes —
     * implementación simple: por cada byte esperamos hasta
     * `budget_us / (n - got)` us para que el primer carácter no
     * agote todo el timeout. */
    while (got < n) {
        uint32_t remaining = (uint32_t) (n - got);
        uint64_t per_byte_us = budget_us / (remaining > 0 ? remaining : 1);
        if (per_byte_us < 1000) per_byte_us = 1000;  /* mínimo 1ms */
        if (!uart_is_readable_within_us(inst, per_byte_us)) {
            break;
        }
        data[got++] = (uint8_t) uart_getc(inst);
        if (budget_us > per_byte_us) budget_us -= per_byte_us;
        else budget_us = 0;
        if (budget_us == 0) break;
    }
    return (int) got;
}
static int pico_uart_available_impl(int bus) {
    uart_inst_t* inst = uart_inst_for(bus);
    return uart_is_readable(inst) ? 1 : 0;
}
static const bpvm_uart_backend_t s_pico_uart_backend = {
    .init      = pico_uart_init_impl,
    .write     = pico_uart_write_impl,
    .read      = pico_uart_read_impl,
    .available = pico_uart_available_impl,
};

/* --- Backend Pulse counter del Pico SDK -------------------------
 *
 * Usa los slices PWM en modo input-gate edge counting: el contador
 * del slice avanza con flancos del pin B en lugar de con el reloj
 * del sistema. Hardware puro, sin coste de CPU. El counterId que
 * devolvemos al BP es el slice number (0..11) — así start/stop/
 * value/reset operan sobre el slice directamente.
 *
 * Constraint del HW: el pin debe ser canal B de algún slice. En el
 * RP2350 los canales B están en GPIOs impares (GP1, GP3, GP5, GP7,
 * GP9, GP11, GP13, GP15, GP17, GP19, GP21, GP23, GP25, GP27, GP29).
 * Si el pin no lo es, init devuelve -1 y la clase Counter lanza
 * RuntimeError.
 *
 * Limitación conocida: el SDK del Pico expone PWM_DIV_B_RISING y
 * PWM_DIV_B_FALLING pero NO un modo "ambos flancos". Si el usuario
 * pide BOTH, fallback a RISING (mejor que rechazar). Para BOTH
 * real haría falta un programa PIO de 4 instrucciones — se añade
 * cuando haya caso de uso. */
static enum pwm_clkdiv_mode pico_pulse_edge_to_mode(int edgeKind) {
    switch (edgeKind) {
        case 1:  return PWM_DIV_B_FALLING;
        case 0:
        case 2:  /* BOTH no soportado por HW; fallback */
        default: return PWM_DIV_B_RISING;
    }
}

static int pico_pulse_init_impl(int pin, int edgeKind) {
    /* Validación: el pin DEBE ser canal B de algún slice. */
    if (pwm_gpio_to_channel((uint) pin) != PWM_CHAN_B) {
        return -1;
    }
    uint slice = pwm_gpio_to_slice_num((uint) pin);

    /* Patrón canónico del SDK: configurar el slice ANTES de conectar
     * el GPIO al periférico (evita capturar flancos espurios del
     * estado inicial) y usar pwm_config + pwm_init en lugar de las
     * funciones sueltas — atómico, deja el slice en un estado
     * coherente sin transiciones intermedias raras.
     *
     * clkdiv a 1.0 explícito: aunque el default ya es 1.0, lo
     * forzamos por si la fábrica devuelve algo distinto en alguna
     * revisión del SDK. */
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 1.0f);
    pwm_config_set_clkdiv_mode(&cfg, pico_pulse_edge_to_mode(edgeKind));
    pwm_config_set_wrap(&cfg, 0xFFFFu);
    /* false = init sin enable; arrancamos con pwm_set_enabled en start(). */
    pwm_init(slice, &cfg, false);

    /* Ahora sí conectamos el pin al PWM. El bloque ya está
     * configurado, así que cualquier nivel inicial no incrementa
     * el contador erróneamente. */
    gpio_set_function((uint) pin, GPIO_FUNC_PWM);
    return (int) slice;
}

static void pico_pulse_start_impl(int counterId) {
    pwm_set_enabled((uint) counterId, true);
}

static void pico_pulse_stop_impl(int counterId) {
    pwm_set_enabled((uint) counterId, false);
}

static int pico_pulse_value_impl(int counterId) {
    return (int) pwm_get_counter((uint) counterId);
}

static void pico_pulse_reset_impl(int counterId) {
    pwm_set_counter((uint) counterId, 0);
}

static const bpvm_pulse_backend_t s_pico_pulse_backend = {
    .init  = pico_pulse_init_impl,
    .start = pico_pulse_start_impl,
    .stop  = pico_pulse_stop_impl,
    .value = pico_pulse_value_impl,
    .reset = pico_pulse_reset_impl,
};

/* --- Backend PWM del Pico SDK -----------------------------------
 *
 * Genera señal PWM hardware en un pin. Política:
 *
 *  - WRAP fijo a 999 para tener resolución de duty del 0.1% (1000
 *    pasos). Si la frecuencia objetivo exige clkdiv fuera del
 *    rango [1, 256], reducimos resolución (subiendo wrap) para
 *    encajar.
 *  - f_pwm = f_sys / (clkdiv * (wrap + 1))
 *    f_sys ≈ 150 MHz en RP2350 con la config por defecto.
 *  - Ambos canales del mismo slice comparten clkdiv+wrap, por
 *    eso setFreq afecta a los dos. Duty es independiente por
 *    canal (set_chan_level).
 */

#include "hardware/clocks.h"   /* clock_get_hz(clk_sys) */

/* Calcula clkdiv y wrap para acercarse a freqHz. wrap empieza a 999
 * (1000 pasos para duty); si clkdiv calculado se sale de rango,
 * subimos wrap. Devuelve (clkdiv, wrap) por punteros. */
static void pico_pwm_calc_div_wrap(int freqHz, float* out_clkdiv,
                                   uint16_t* out_wrap) {
    if (freqHz <= 0) freqHz = 1;
    uint32_t f_sys = clock_get_hz(clk_sys);
    /* Empezamos con wrap=999 (resolución 0.1% del duty). */
    uint32_t wrap = 999;
    float clkdiv = (float) f_sys / ((float) freqHz * (float)(wrap + 1));
    /* Si clkdiv > 256, no cabe → necesitamos wrap mayor. */
    while (clkdiv > 256.0f && wrap < 65535u) {
        wrap = wrap * 2u + 1u;
        if (wrap > 65535u) wrap = 65535u;
        clkdiv = (float) f_sys / ((float) freqHz * (float)(wrap + 1));
    }
    /* Si clkdiv < 1, freq objetivo demasiado alta para wrap actual →
     * bajar wrap para subir freq. */
    while (clkdiv < 1.0f && wrap > 0u) {
        wrap = wrap / 2u;
        clkdiv = (float) f_sys / ((float) freqHz * (float)(wrap + 1));
    }
    if (clkdiv < 1.0f)   clkdiv = 1.0f;
    if (clkdiv > 255.99f) clkdiv = 255.99f;
    *out_clkdiv = clkdiv;
    *out_wrap   = (uint16_t) wrap;
}

static int pico_pwm_init_impl(int pin, int freqHz) {
    if (pin < 0 || pin > 29) return -1;
    uint slice = pwm_gpio_to_slice_num((uint) pin);

    float clkdiv;
    uint16_t wrap;
    pico_pwm_calc_div_wrap(freqHz, &clkdiv, &wrap);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, clkdiv);
    pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_FREE_RUNNING);
    pwm_config_set_wrap(&cfg, wrap);
    pwm_init(slice, &cfg, false);   /* false = no enable todavía */

    /* Duty inicial 0 (pin LOW). */
    pwm_set_chan_level(slice, pwm_gpio_to_channel((uint) pin), 0);

    gpio_set_function((uint) pin, GPIO_FUNC_PWM);
    return (int) slice;
}

static void pico_pwm_set_freq_impl(int sliceId, int freqHz) {
    float clkdiv;
    uint16_t wrap;
    pico_pwm_calc_div_wrap(freqHz, &clkdiv, &wrap);
    pwm_set_clkdiv((uint) sliceId, clkdiv);
    pwm_set_wrap((uint) sliceId, wrap);
    /* Reset del counter interno para que el nuevo wrap arranque
     * desde 0. Sin esto, si el counter está por encima del nuevo
     * wrap, se tira varios ciclos viejos hasta wrap-around — esos
     * "ciclos zombi" salen a frecuencia intermedia y aparecen como
     * pulsos perdidos en ventanas cortas (ver Test 3 de PwmTest). */
    pwm_set_counter((uint) sliceId, 0);
}

static void pico_pwm_set_duty_impl(int sliceId, int pin, int dutyPct) {
    if (dutyPct < 0)   dutyPct = 0;
    if (dutyPct > 100) dutyPct = 100;
    uint16_t wrap = pwm_hw->slice[sliceId].top;
    /* level = (dutyPct/100) * (wrap+1). Usamos enteros para evitar float. */
    uint32_t level = ((uint32_t)(wrap + 1u) * (uint32_t) dutyPct) / 100u;
    if (level > (uint32_t)(wrap + 1u)) level = wrap + 1u;
    pwm_set_chan_level((uint) sliceId, pwm_gpio_to_channel((uint) pin),
                       (uint16_t) level);
}

static void pico_pwm_start_impl(int sliceId) {
    pwm_set_enabled((uint) sliceId, true);
}

static void pico_pwm_stop_impl(int sliceId) {
    pwm_set_enabled((uint) sliceId, false);
}

static const bpvm_pwm_backend_t s_pico_pwm_backend = {
    .init    = pico_pwm_init_impl,
    .setFreq = pico_pwm_set_freq_impl,
    .setDuty = pico_pwm_set_duty_impl,
    .start   = pico_pwm_start_impl,
    .stop    = pico_pwm_stop_impl,
};

/* --- Backend de info del MCU (Pico) -----------------------------
 *
 * Identificación: pico_get_unique_board_id_string() escribe los
 *   8 bytes del flash chip ID como string ASCII de 16 hex chars.
 *
 * Temperatura interna: el sensor del die va al ÚLTIMO canal del mux ADC,
 *   y su índice depende de la VARIANTE del chip: input 4 en RP2350A
 *   (QFN-60) pero input 8 en RP2350B (QFN-80), donde los inputs 0-7 son
 *   los GPIO40-47. Con el 4 hardcodeado, en el Metro se leía el GPIO44
 *   flotante y salían "grados" absurdos. Como la imagen es ÚNICA
 *   (bp_rp2350b genérico, decisión 2026-06-06), la variante se decide en
 *   RUNTIME con board_desc() — las macros del SDK (NUM_ADC_CHANNELS,
 *   ADC_BASE_PIN, ADC_TEMPERATURE_CHANNEL_NUM) compilan aquí SIEMPRE como
 *   package B y en un Pico 2 seleccionarían un input inexistente.
 *   Fórmula de conversión del datasheet:
 *     T_C = 27 - (V_ADC - 0.706) / 0.001721
 *   donde V_ADC = raw * 3.3 / 4095.
 *
 * Reloj: clock_get_hz(clk_sys) — afectado por overclocks o
 *   downclocks; en boot por defecto 150 MHz en RP2350.
 *
 * Uptime: to_ms_since_boot(get_absolute_time()) — uint32 con
 *   wrap a 49 días.
 *
 * El ADC se inicializa lazy en la primera llamada a tempC(). Si
 * más adelante exponemos Adc.Channel como periférico de usuario,
 * habrá que coordinar el adc_init para que no se llame dos veces
 * (el SDK es idempotente, pero por orden). */

static bool s_adc_inited = false;
static void ensure_adc_inited(void) {
    if (s_adc_inited) return;
    adc_init();
    adc_set_temp_sensor_enabled(true);
    s_adc_inited = true;
}

static void pico_pico_unique_id_impl(char* buf, size_t len) {
    if (len == 0) return;
    /* El SDK pide len >= PICO_UNIQUE_BOARD_ID_SIZE_BYTES*2 + 1.
     * Para RP2350 son 8 bytes → 17 chars con null. */
    if (len >= 17) {
        pico_get_unique_board_id_string(buf, (uint) len);
    } else {
        /* Buffer corto: escribimos en uno temporal y truncamos. */
        char tmp[17];
        pico_get_unique_board_id_string(tmp, sizeof(tmp));
        size_t n = len - 1;
        memcpy(buf, tmp, n);
        buf[n] = '\0';
    }
}

static void pico_pico_board_name_impl(char* buf, size_t len) {
    /* H7.3: el nombre lo da el descriptor de placa (board_desc_init lo fija
     * desde la variante por defecto o desde /sys/board.json). */
    const char* name = board_desc()->name;
    size_t n = strlen(name);
    if (len == 0) return;
    if (n > len - 1) n = len - 1;
    memcpy(buf, name, n);
    buf[n] = '\0';
}

static float pico_pico_temp_c_impl(void) {
    ensure_adc_inited();
    /* Último canal del mux = sensor del die; por variante en runtime. */
    adc_select_input(board_desc()->variant == 'B' ? 8u : 4u);
    /* Promediamos 8 muestras para reducir ruido del ADC. */
    uint32_t acc = 0;
    for (int i = 0; i < 8; i++) acc += adc_read();
    float raw = (float) acc / 8.0f;
    /* V = raw * 3.3 / 4095. Sensor: T_C = 27 - (V - 0.706) / 0.001721. */
    float v = raw * 3.3f / 4095.0f;
    float t = 27.0f - (v - 0.706f) / 0.001721f;
    return t;
}

static int pico_pico_cpu_freq_hz_impl(void) {
    return (int) clock_get_hz(clk_sys);
}

static int pico_pico_uptime_ms_impl(void) {
    return (int) to_ms_since_boot(get_absolute_time());
}

/* H7.3 — board-aware: nº de GPIO desde el descriptor de placa (variante
 * RP2350A=30 / RP2350B=48, u override de /sys/board.json). */
static int pico_pico_gpio_count_impl(void) {
    return board_desc()->gpio_count;
}

/* setCpuFreqMHz — cambia el clk_sys del RP2350.
 *
 * `set_sys_clock_khz(khz, required=false)` busca la combinación de PLL
 * más cercana al objetivo y la aplica. Con `required=false` no resetea
 * si no encuentra una válida; devolvemos 0 al BP en ese caso para que
 * pueda detectar el fallo y reintentar con otro valor.
 *
 * El clamp al máximo soportado (MAX_CPU_MHZ) lo hace BP en Pico.bp
 * ANTES de llamar — aquí no necesitamos saber el techo absoluto del
 * chip. Sí descartamos valores ridículamente bajos (< 18 MHz que es
 * el mínimo razonable de la PLL del RP2350).
 *
 * VOLTAJE: el RP2350 nominal a Vdd_core=1.10V solo sostiene ~200 MHz.
 * Por encima necesita subir Vdd_core. Escalamos automáticamente para
 * que el contrato del builtin sea estable ("pides MHz, te los da si
 * puede"):
 *   - mhz <= 200: 1.10 V (default, no tocamos vreg)
 *   - 200 <  mhz <= 250: 1.15 V
 *   - 250 <  mhz <= 280: 1.20 V
 *   - mhz >  280       : 1.30 V  (techo razonable sin overshoot)
 * Tras vreg_set_voltage hay que dar tiempo para que la rampa se
 * estabilice (10 ms es el valor que recomiendan los ejemplos del SDK).
 * Si BAJAMOS clk_sys, bajamos voltaje también — ahorra energía.
 *
 * AVISO documentado: tras cambiar clk_sys, los periféricos derivados
 * de clk_peri (UART/SPI/I2C/PWM) que ya estuvieran configurados
 * quedan con la frecuencia mal — el SDK del Pico recalcula clk_peri
 * automáticamente, pero los baudrates/clkdivs precalculados en
 * Uart.init/Spi.init/etc. NO se reajustan. Llamar a setCpuFreqMHz
 * ANTES de configurar periféricos. Las funciones sleep* siguen
 * exactas porque el timer hardware del Pico corre a 1 MHz
 * independiente del clk_sys. */
#include "hardware/vreg.h"

static int pico_pico_set_cpu_freq_mhz_impl(int mhz) {
    if (mhz < 18) mhz = 18;

    enum vreg_voltage target_v;
    if (mhz <= 200)      target_v = VREG_VOLTAGE_1_10;
    else if (mhz <= 250) target_v = VREG_VOLTAGE_1_15;
    else if (mhz <= 280) target_v = VREG_VOLTAGE_1_20;
    else                 target_v = VREG_VOLTAGE_1_30;

    uint32_t cur_khz = clock_get_hz(clk_sys) / 1000u;
    uint32_t new_khz = (uint32_t) mhz * 1000u;
    bool going_up = (new_khz > cur_khz);

    /* Regla anti-cuelgue:
     *   - SUBIR freq → subir voltaje PRIMERO, luego subir freq.
     *     Asegura que el core no opere a más MHz de los que el
     *     voltaje actual sostiene.
     *   - BAJAR freq → bajar freq PRIMERO, luego bajar voltaje.
     *     Asegura que el core no quede a freq alta con voltaje
     *     reducido (cuelgue garantizado a 300 MHz con 1.10 V).
     * El vreg HW filtra writes redundantes, así que llamar siempre
     * no daña aunque target_v == voltaje actual. */
    bool ok;
    if (going_up) {
        vreg_set_voltage(target_v);
        busy_wait_us(10000);   /* 10 ms para estabilizar la rampa */
        ok = set_sys_clock_khz(new_khz, false);
    } else {
        ok = set_sys_clock_khz(new_khz, false);
        vreg_set_voltage(target_v);
        /* Sin sleep aquí: la freq ya bajó, no hay riesgo. */
    }
    return ok ? 1 : 0;
}

static const bpvm_pico_backend_t s_pico_pico_backend = {
    .uniqueId      = pico_pico_unique_id_impl,
    .boardName     = pico_pico_board_name_impl,
    .tempC         = pico_pico_temp_c_impl,
    .cpuFreqHz     = pico_pico_cpu_freq_hz_impl,
    .uptimeMs      = pico_pico_uptime_ms_impl,
    .setCpuFreqMHz = pico_pico_set_cpu_freq_mhz_impl,
    .gpioCount     = pico_pico_gpio_count_impl,
};

/* ============================ Adc ============================ */

#include "bpvm_adc.h"
#include "hardware/adc.h"   /* ya incluido para tempC pero idempotente */

/* Canales ADC externos según VARIANTE, decidida en RUNTIME (board_desc):
 * RP2350A = 4 (GPIO26-29, sensor en input 4), RP2350B = 8 (GPIO40-47,
 * sensor en input 8). Imagen única → NO usar ADC_BASE_PIN /
 * NUM_ADC_CHANNELS del SDK (en este build genérico son siempre los del
 * package B): con el 26 hardcodeado el Metro configuraba un GPIO digital
 * y leía otro pin. bd->adc_channels ya trae 4/8 por variante. Adc.bp
 * sigue limitando a 0..3 en BP (subset común; ampliar = P-adc-8ch). */
static int s_adc_chan_inited[8] = { 0 };   /* máx. externos (RP2350B) */

static int adc_base_gpio(void) {
    return board_desc()->variant == 'B' ? 40 : 26;
}

static int pico_adc_init_channel_impl(int ch) {
    if (ch < 0 || ch >= board_desc()->adc_channels) return -1;
    if (!s_adc_chan_inited[ch]) {
        /* ensure_adc_inited() (más arriba) ya hace adc_init() la
         * primera vez que tempC se usa. Lo replicamos aquí por si
         * Adc.Channel se usa SIN haber tocado Pico.tempC antes. El
         * SDK es idempotente. */
        ensure_adc_inited();
        adc_gpio_init((uint)(adc_base_gpio() + ch));
        s_adc_chan_inited[ch] = 1;
    }
    return adc_base_gpio() + ch;
}

static int pico_adc_read_channel_impl(int ch) {
    if (ch < 0 || ch >= board_desc()->adc_channels) return -1;
    adc_select_input((uint) ch);
    return (int) adc_read();
}

static const bpvm_adc_backend_t s_pico_adc_backend = {
    .initChannel = pico_adc_init_channel_impl,
    .readChannel = pico_adc_read_channel_impl,
};

/* ============================ Wdt ============================ */

#include "bpvm_wdt.h"
#include "hardware/watchdog.h"

static int s_wdt_active = 0;

static void pico_wdt_enable_impl(int timeoutMs) {
    /* watchdog_enable(ms, pause_on_debug=true). pause_on_debug evita
     * que el watchdog dispare cuando el chip está paused en debugger,
     * que es lo razonable para desarrollo. */
    if (timeoutMs <= 0) timeoutMs = 100;
    watchdog_enable((uint32_t) timeoutMs, true);
    s_wdt_active = 1;
}

static void pico_wdt_feed_impl(void) {
    if (s_wdt_active) watchdog_update();
}

static void pico_wdt_disable_impl(void) {
    /* RP2350: el watchdog no se puede deshabilitar completamente
     * sin re-bootear. Aproximamos seteando el max valor (~8.4M ms)
     * que en práctica nunca dispara. */
    watchdog_enable(0x7FFFFF, true);
    s_wdt_active = 0;
}

static const bpvm_wdt_backend_t s_pico_wdt_backend = {
    .enable  = pico_wdt_enable_impl,
    .feed    = pico_wdt_feed_impl,
    .disable = pico_wdt_disable_impl,
};

/* --- Sink para los `print` de la VM. Sale por USB CDC. ----------- */
static void usb_sink(const char* data, size_t len, void* user) {
    (void) user;
    fwrite(data, 1, len, stdout);
    fflush(stdout);
}

/* Ejecuta una vez la VM sobre Hello.mod cargado del FS. */
static bpvm_status_t run_vm_once(int iteration) {
    printf("\n");
    printf("===========================================\n");
    printf(" bpgenvm-c on RP2350 / FreeRTOS — FP2 step 1\n");
    printf(" iteration #%d (FS-backed)\n", iteration);
    printf("===========================================\n");

    /* Lee el .mod desde el FS, no del array directo. */
    const uint8_t* data; uint32_t size;
    fs_status_t fs = fs_get("Hello.mod", &data, &size);
    if (fs != FS_OK) {
        printf("[ERR] fs_get: %s\n", fs_status_str(fs));
        return BPVM_ERR_IO;
    }
    printf("[fs] Hello.mod %u bytes  (fs_used=%u/%u)\n",
           (unsigned) size, (unsigned) fs_used_bytes(),
           (unsigned) fs_total_bytes());

    bpvm_t* vm = bpvm_init(s_vm_buffer, VM_BUFFER_SIZE, 0);
    if (!vm) {
        printf("[ERR] bpvm_init failed\n");
        return BPVM_ERR_OOM;
    }
    bpvm_set_output(vm, usb_sink, NULL);

    bpvm_status_t s = bpvm_load_mod_buffer(vm, data, size, "Hello");
    if (s != BPVM_OK) {
        printf("[ERR] load_mod_buffer: %s\n", bpvm_status_str(s));
        bpvm_destroy(vm);
        return s;
    }

    printf("--- VM output ---\n");
    s = bpvm_run(vm);
    printf("\n--- VM finished: %s ---\n", bpvm_status_str(s));

    bpvm_destroy(vm);
    return s;
}

/* vm_task — arranca FS + stdlib + REPL, sin ruido visible.
 *
 * El bring-up histórico tenía blinks de hitos (mark()) y prints
 * `[boot N] bpvm_pico vivo`, `[stdlib] X.mod (Y bytes)`, etc. — útiles
 * los primeros días cuando el USB CDC fallaba a oscuras, pero
 * innecesarios ahora que la cadena está estable. Si vuelven a hacer
 * falta para post-mortem, log_printf() sigue ahí escribiendo al log
 * persistente en flash (silencioso pero recuperable con LOG en REPL).
 */
static void vm_task(void* arg) {
    (void) arg;

    log_printf("vm_task: started");
    setvbuf(stdout, NULL, _IONBF, 0);
    log_printf("vm_task: setvbuf OK");

    fs_init();
    log_printf("fs_init: %d ficheros, %u bytes",
               fs_file_count(), (unsigned) fs_used_bytes());

    /* H7.3: descriptor de placa — caps por variante (A/B) + override de
     * /sys/board.json. Tras fs_init para poder leer el board.json del FS.
     * Sondea/habilita la PSRAM (si board.json declara psramCsPin). */
    board_desc_init();

    /* H7.2.b — si hay PSRAM USABLE (detectada + QPI + RW test OK), el heap de la
     * VM pasa de los 128 KB de SRAM interna a la ventana PSRAM (MBs). El resto
     * del firmware no cambia: sólo de dónde sale s_vm_buffer. Sin PSRAM (Pico)
     * se queda en s_sram_buffer. */
    if (board_desc()->psram_present && board_desc()->psram_bytes >= (1u << 20)) {
        s_vm_buffer      = (uint8_t*) (uintptr_t) PSRAM_XIP_BASE;
        s_vm_buffer_size = board_desc()->psram_bytes;
        log_printf("vm: heap en PSRAM %u MB @ 0x%08x",
                   (unsigned)(s_vm_buffer_size / (1024u * 1024u)),
                   (unsigned) PSRAM_XIP_BASE);
    } else {
        log_printf("vm: heap en SRAM interna %u KB",
                   (unsigned)(s_vm_buffer_size / 1024u));
    }

    /* H7.4.a — test del driver NeoPixel: si la placa declara neopixelPin, enciende
     * el LED onboard en verde tenue al boot. Valida la 1ª infra PIO de forma
     * visible antes de la clase BP (H7.4.b). Sin neopixelPin (Pico) → no-op. */
    if (board_desc()->neopixel_pin >= 0) {
        if (neopixel_init(board_desc()->neopixel_pin)) {
            uint32_t verde = (8u << 16);   /* GRB: g=8 (tenue), r=0, b=0 */
            neopixel_show(board_desc()->neopixel_pin, &verde, 1);
            log_printf("neopixel: onboard @ GP%d verde tenue (test H7.4.a)",
                       board_desc()->neopixel_pin);
        } else {
            log_printf("neopixel: init FALLO en GP%d", board_desc()->neopixel_pin);
        }
    }

    /* Stdlib pre-instalada en /lib/, Hello en /app/. La resolución de
     * imports en cmd_run busca también en estos directorios además del
     * root (ver fs_get_resolve en repl.c), así que el usuario sigue
     * pudiendo subir ficheros sin prefijo y todo funciona.
     *
     * NO persistimos automáticamente a flash — el FORMAT borra apps
     * pero la stdlib resurge en el siguiente reboot desde la imagen. */
    const uint8_t* dummy; uint32_t dummy_sz;
    if (fs_get("/lib/Core.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Core.mod", core_mod, core_mod_len);
        log_printf("stdlib: /lib/Core.mod installed (%u bytes)", core_mod_len);
    }
    if (fs_get("/lib/Gpio.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Gpio.mod", gpio_mod, gpio_mod_len);
        log_printf("stdlib: /lib/Gpio.mod installed (%u bytes)", gpio_mod_len);
    }
    if (fs_get("/lib/I2c.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/I2c.mod", i2c_mod, i2c_mod_len);
        log_printf("stdlib: /lib/I2c.mod installed (%u bytes)", i2c_mod_len);
    }
    if (fs_get("/lib/Spi.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Spi.mod", spi_mod, spi_mod_len);
        log_printf("stdlib: /lib/Spi.mod installed (%u bytes)", spi_mod_len);
    }
    if (fs_get("/lib/Uart.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Uart.mod", uart_mod, uart_mod_len);
        log_printf("stdlib: /lib/Uart.mod installed (%u bytes)", uart_mod_len);
    }
    if (fs_get("/lib/Pulse.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Pulse.mod", pulse_mod, pulse_mod_len);
        log_printf("stdlib: /lib/Pulse.mod installed (%u bytes)", pulse_mod_len);
    }
    if (fs_get("/lib/Pwm.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Pwm.mod", pwm_mod, pwm_mod_len);
        log_printf("stdlib: /lib/Pwm.mod installed (%u bytes)", pwm_mod_len);
    }
    if (fs_get("/lib/Pico.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Pico.mod", pico_mod, pico_mod_len);
        log_printf("stdlib: /lib/Pico.mod installed (%u bytes)", pico_mod_len);
    }
    if (fs_get("/lib/Rtc.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Rtc.mod", rtc_mod, rtc_mod_len);
        log_printf("stdlib: /lib/Rtc.mod installed (%u bytes)", rtc_mod_len);
    }
    if (fs_get("/lib/Adc.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Adc.mod", adc_mod, adc_mod_len);
        log_printf("stdlib: /lib/Adc.mod installed (%u bytes)", adc_mod_len);
    }
    if (fs_get("/lib/Wdt.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Wdt.mod", wdt_mod, wdt_mod_len);
        log_printf("stdlib: /lib/Wdt.mod installed (%u bytes)", wdt_mod_len);
    }
    if (fs_get("/lib/Timer.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Timer.mod", timer_mod, timer_mod_len);
        log_printf("stdlib: /lib/Timer.mod installed (%u bytes)", timer_mod_len);
    }
    if (fs_get("/lib/Neopixel.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/lib/Neopixel.mod", neopixel_mod, neopixel_mod_len);
        log_printf("stdlib: /lib/Neopixel.mod installed (%u bytes)", neopixel_mod_len);
    }
    /* Drivers de dispositivo (PCA9554, BME280, SSD1306, ...) NO se
     * pre-instalan aquí — los sube el IDE como deps al hacer Run a
     * /app/ o a root, da igual: la resolución encuentra ambos. */
    if (fs_get("/app/Hello.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("/app/Hello.mod", hello_mod, hello_mod_len);
        log_printf("app: /app/Hello.mod installed (%u bytes)", hello_mod_len);
    }

    log_printf("fs: %d ficheros, %u/%u bytes usados",
               fs_file_count(), (unsigned) fs_used_bytes(),
               (unsigned) fs_total_bytes());

    /* H3 micro-bench (#159) — fib_native(28) en C puro para medir el
     * techo de rendimiento del AOT. Compara con Bench.mod corriendo
     * fib(28) en BP. Si el ratio C:BP < 2× → aparcamos AOT. */
    bench_run_native();

#ifdef BPVM_PICO_BOOT_LED
    /* #153 bring-up: llegamos al REPL → boot OK. Apaga el LED que
     * main() encendió antes del scheduler. Si tras flashear el LED
     * queda ENCENDIDO fijo → el boot se colgó ANTES de aquí (arranque
     * del scheduler / core 1 / vm_task). Si queda APAGADO → boot llegó
     * al REPL y un "no conecta" es problema de USB/conexión, no de boot. */
    led_set(0);
#endif
    /* P-autorun (#256) — si /sys/auto.txt existe, arranca la app ANTES
     * de entrar al REPL. El orden que pide el diseño (comm siempre
     * antes) se cumple solo: el wire ya está vivo y el poll del run
     * atiende HELLO/KILL, así que el IDE puede conectar y parar la app
     * aunque sea un bucle infinito. */
    repl_v1_autorun();
    repl_run();
    (void) run_vm_once;  /* silenciar unused warning */
}

/* --- assert hook --------------------------------------------------- */
void vAssertCalled(const char *pcFileName, unsigned long ulLine) {
    /* Best-effort: registra el assert en el log y flushea a flash
     * ANTES de detener los IRQs. Si log_flush funciona, en el siguiente
     * arranque el comando LOG mostrará exactamente este ASSERT. */
    log_printf("ASSERT %s:%lu", pcFileName, ulLine);
    log_flush();

    taskDISABLE_INTERRUPTS();
    printf("[ASSERT] %s:%lu\n", pcFileName, ulLine);
    for (;;) {
        led_set(1); for (volatile int i = 0; i < 2000000; i++);
        led_set(0); for (volatile int i = 0; i < 2000000; i++);
    }
}

void vApplicationMallocFailedHook(void) {
    log_printf("MALLOC FAIL");
    log_flush();
    printf("[MALLOC FAIL]\n");
    vAssertCalled(__FILE__, __LINE__);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void) xTask;
    log_printf("STACK OVERFLOW task=%s", pcTaskName ? pcTaskName : "?");
    log_flush();
    printf("[STACK OVERFLOW] task=%s\n", pcTaskName);
    vAssertCalled(__FILE__, __LINE__);
}

/* Buffers estáticos para idle/timer task (configSUPPORT_STATIC_ALLOCATION=1). */
static StaticTask_t s_idle_tcb;
static StackType_t  s_idle_stack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxTCB,
                                    StackType_t **ppxStack,
                                    uint32_t *pulStackSize) {
    *ppxTCB      = &s_idle_tcb;
    *ppxStack    = s_idle_stack;
    *pulStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t s_timer_tcb;
static StackType_t  s_timer_stack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTCB,
                                     StackType_t **ppxStack,
                                     uint32_t *pulStackSize) {
    *ppxTCB      = &s_timer_tcb;
    *ppxStack    = s_timer_stack;
    *pulStackSize = configTIMER_TASK_STACK_DEPTH;
}

#if ( configNUMBER_OF_CORES > 1 )
/* #153 — SMP necesita un idle "pasivo" por cada core extra (N-1). Con
 * static allocation hay que proveer su memoria igual que el idle/timer.
 * configNUMBER_OF_CORES-1 sets. */
static StaticTask_t s_passive_idle_tcb[ configNUMBER_OF_CORES - 1 ];
static StackType_t  s_passive_idle_stack[ configNUMBER_OF_CORES - 1 ]
                                        [ configMINIMAL_STACK_SIZE ];
void vApplicationGetPassiveIdleTaskMemory(StaticTask_t **ppxTCB,
                                          StackType_t **ppxStack,
                                          uint32_t *pulStackSize,
                                          BaseType_t xPassiveIdleTaskIndex) {
    *ppxTCB       = &s_passive_idle_tcb[ xPassiveIdleTaskIndex ];
    *ppxStack     =  s_passive_idle_stack[ xPassiveIdleTaskIndex ];
    *pulStackSize =  configMINIMAL_STACK_SIZE;
}
#endif

int main(void) {
    led_init();
    stdio_init_all();
    /* CRÍTICO para el wire v1: el stdout del Pico SDK trae la traducción
     * CRLF ACTIVADA por defecto (PICO_STDIO_DEFAULT_CRLF=1), que inserta un
     * '\r' antes de cada '\n'. Es inocua para las líneas JSON y la salida de
     * RUN (el cliente descarta '\r'), pero CORROMPE el bulk binario: en un
     * GET, cada 0x0A del fichero se transmite como 0x0D 0x0A, el cliente lee
     * sólo los `bulk` bytes declarados y el fichero llega TRUNCADO (se destapó
     * al añadir ver/editar ficheros del device, #231). El protocolo es 8-bit
     * limpio y pone sus propios '\n' → desactivamos la traducción. */
    stdio_set_translate_crlf(&stdio_usb, false);

    /* Log persistente: carga el snapshot anterior antes de pisarlo con
     * mensajes del boot actual. */
    log_init();
    log_printf("=== boot " __DATE__ " " __TIME__ " ===");

    /* Conecta los backends de HW reales (Pico SDK) a los builtins de
     * la VM. Sin esto los handlers caen al stub con logging. */
    bpvm_gpio_set_backend(&s_pico_gpio_backend);
    bpvm_i2c_set_backend(&s_pico_i2c_backend);
    bpvm_spi_set_backend(&s_pico_spi_backend);
    bpvm_uart_set_backend(&s_pico_uart_backend);
    bpvm_pulse_set_backend(&s_pico_pulse_backend);
    bpvm_pwm_set_backend(&s_pico_pwm_backend);
    bpvm_pico_set_backend(&s_pico_pico_backend);
    bpvm_adc_set_backend(&s_pico_adc_backend);
    bpvm_wdt_set_backend(&s_pico_wdt_backend);
    bpvm_neopixel_set_backend(&s_pico_neopixel_backend);   /* H7.4 */
    fs_register_bpvm();                                    /* #247 — file I/O desde BP */
    /* Rtc en Pico usa el stub portable (bpvm_platform_now_ms + offset).
     * Cuando reset, el offset = 0 → epochSec devuelve segundos desde
     * boot. El IDE envía TIME <epochsec> al conectar y el comando
     * del REPL llama a bpvm_rtc_set_now_ms — a partir de ahí el reloj
     * está calibrado. */

    BaseType_t r = xTaskCreate(vm_task, "vm_task", 4096, NULL,
                                tskIDLE_PRIORITY + 2, NULL);
    if (r != pdPASS) {
        log_printf("xTaskCreate(vm_task) FAILED");
        log_flush();
        led_set(1);
        for (;;) {}
    }

#ifdef BPVM_PICO_BOOT_LED
    /* #153 bring-up: LED ON justo antes de arrancar el scheduler (que en
     * dual-core lanza el core 1). vm_task lo apaga al llegar al REPL.
     * LED fijo encendido tras flashear = boot colgado entre aquí y el
     * REPL (scheduler/core1). LED apagado = boot OK. */
    led_set(1);
#endif
    vTaskStartScheduler();
    for (;;) {}
    return 0;
}
