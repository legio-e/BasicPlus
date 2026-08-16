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
#include "hardware/watchdog.h"   /* paso 4 cierre — watchdog_caused_reboot (resetCause) */

#include "bpvm.h"
#include "embedded_mods.h"
#include "fs.h"
#include "bpvm_fs.h"
#include "crc32.h"    /* #422: comparar lo desplegado con lo embebido */     /* H11: stat + lectura por trozos (el .mod no pasa entero por RAM) */
#include "board_desc.h"
#include "psram.h"
#include "neopixel.h"
#include "bpvm_neopixel.h"
#include "repl_v1.h"   /* wire v1: despachador + bucle (#305) + autorun (#256) */
#include "log.h"
#include "pack_pico.h"
#include "board_mgr_pico.h"   /* H9: board_boot_status (estado real del boot) */
#include "flash_layout.h"     /* H9: BP_ENV_*, BP_PART_BASE (layout 3 zonas) */
#include "bpvm_env.h"         /* H9: env de la zona 2 (bloque A/B) */
#include "bpvm_part.h"        /* H9: particiones derivadas del env */
#include "bpvm_boot.h"        /* H9: máquina de estados del arranque */
#include "bpvm_sqlmem.h"      /* V5/H: regla del bloque de memoria de la BD */
#include "bpvm_sd.h"          /* V5/H1: lector de SD por SPI (pines del env)  */
#include "bpvm_sd_blk.h"      /* V5/H6: esa SD, vista como dispositivo de bloque */
#include "bpvm_fs_fat.h"      /* V5/H2: montarla al arranque si hay tarjeta   */
#include "bpvm_bios.h"        /* V5/I: tabla prestada al pack nativo */
/* pico/bios_pico.c — la tabla de ESTA placa, ya verificada (NULL si tiene
 * huecos; el motivo lo deja él mismo en el log). */
const bpvm_bios_t* bios_pico_get(void);
const bpvm_ancla_t* bios_pico_ancla(void);

/* Cuánto barrer buscando el ancla. El firmware (texto+rodata) cabe de sobra en
 * 1 MB; barrer más sería tiempo de arranque tirado. Si algún día la imagen
 * creciera por encima, el propio arranque lo diría: "ancla: NO SE ENCUENTRA". */
#define ANCLA_BARRIDO_BYTES  (1u * 1024u * 1024u)
#include "hardware/flash.h"   /* FLASH_SECTOR_SIZE */
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

/* Buffer de la VM en SRAM interna. 128 KB sobra para Hello.bp y deja sitio
 * para varios módulos. */
/* H11 — AQUI ESTABA VM_BUFFER_SIZE (128 KB fijos). Se ha quedado sin
 * usuarios: la region ya no es un tamaño elegido a dedo sino TODO lo que
 * queda libre. Un numero que ya no manda sobre nada, fuera. */

/* H11 — NO es un array estático, es una REGIÓN DE LA COLA DE LA RAM.
 *
 * Antes eran 128 KB de `.bss` reservados a piñón. Pero la decisión de dónde
 * vive el heap de la VM es de RUNTIME (PSRAM si la hay) y la imagen es ÚNICA
 * para Pico y Metro ⇒ en la Metro esos 128 KB se quedaban DORMIDOS: reservados
 * en SRAM interna y sin que nadie los tocara nunca, porque el heap se iba a la
 * ventana PSRAM.
 *
 * Ahora el sitio se toma del final de la RAM (`__HeapLimit`), que el linker ya
 * deja libre: tras `.bss` sólo hay el placeholder de `.heap` y las pilas del
 * CPU viven en los bancos scratch, aparte. `malloc` crece desde `end` hacia
 * arriba y nosotros ocupamos el techo, así que crecen el uno hacia el otro con
 * todo el hueco de por medio — y `vm_sram_region()` comprueba que no se toquen.
 *
 * En la Metro no se reserva NADA: si hay PSRAM la región ni se mira, y esa RAM
 * queda entera para malloc. */
extern char end;                 /* fin de .bss — desde aquí crece sbrk/malloc */
extern char __HeapLimit;         /* tope de la RAM principal */

/* Margen que se le deja a malloc por debajo del buffer de la VM. El loader
 * pide calloc/strdup por cada import de cada módulo; con 32 KB va sobrado. */
#define VM_SRAM_MALLOC_MARGIN  (64 * 1024)

/* Piso: por debajo de esto la VM no da para nada útil y hay que DECIRLO. */
#define VM_SRAM_MIN            (64 * 1024)

/* Eduardo, 28-jul: "en la Pico quedaba RAM sin usar, ¿la podemos usar toda?".
 * Se usaban 128 KB FIJOS de ~520 y quedaban ~255 KB de SRAM sin dueño, porque el
 * único cliente serio de malloc es el loader (calloc/strdup por import).
 * Ahora la VM se queda con TODO lo que hay entre el final de .bss y el techo,
 * menos la reserva de malloc. Sin número mágico: si mañana crece el .bss, la VM
 * recibe menos AUTOMÁTICAMENTE en vez de pisarle la memoria a nadie. */
static uint8_t* vm_sram_region(uint32_t* size_out) {
    /* V5/I — el techo NO es __HeapLimit: encima vive la RAM del pack, y la VM
     * tiene que pararse debajo. Sin esto la VM se quedaria con TODO hasta el
     * techo y el `.data` del pack iria a parar dentro de su heap — un fallo que
     * en la Metro no se veria nunca (alli el heap se va a la PSRAM) y solo
     * reventaria en la Pico 2, con la MISMA imagen. Lo cazo Eduardo. */
    uintptr_t top          = (uintptr_t) PACK_RAM_SRAM_BASE;
    uintptr_t malloc_floor = ((uintptr_t) &end + VM_SRAM_MALLOC_MARGIN + 7u) & ~(uintptr_t) 7u;
    if (malloc_floor >= top) { *size_out = 0; return NULL; }
    *size_out = (uint32_t) (top - malloc_floor);
    return (uint8_t*) malloc_floor;
}

uint8_t* s_vm_buffer      = NULL;   /* se fija en boot (vm_sram_region o PSRAM) */
uint32_t s_vm_buffer_size = 0;

/* V5/H — bloque de la BD: se reserva en el arranque desde el ENV (`SQLite=<MB>`)
 * y NO se toca nunca más (criterio de Eduardo). Sale del principio de la ventana
 * PSRAM, ANTES de que el buffer de la VM se quede con el resto, para que su
 * dirección sea DETERMINISTA y no dependa de cuánta PSRAM lleve la placa: el
 * pack se pre-enlaza contra ella en el PC. 0/NULL = no hay BD. */
uint8_t* s_sqlite_base    = NULL;
uint32_t s_sqlite_size     = 0;

/* V5/H1 — el lector de SD. El arranque SÓLO lee los pines del env y anota si
 * la línea era usable; no habla con la tarjeta (eso lo hace SD_INFO cuando el
 * usuario lo pide). `s_sd_motivo` guarda POR QUÉ no hay configuración, que es
 * la diferencia entre "esta placa no tiene lector" y "la línea está mal
 * escrita" — dos sitios distintos donde mirar. */
bpvm_sd_pines_t s_sd_pines;
int             s_sd_hay_config = 0;
char            s_sd_motivo[96] = "sin leer";
/* Decision de V5/H: el AVISO vive en el INFO (criterio de Eduardo, 7-ago:
 * "es algo que el usuario puede consultar facilmente, y arreglar con
 * facilidad"). El INFO es de CONSULTA y refleja el estado ACTUAL — un aviso al
 * escribir la clave solo aparece en ese instante y luego se pierde. Para que el
 * aviso no MIENTA hace falta el MOTIVO, no solo el resultado: con SQLite=1 los
 * bytes son 0 igual que si la clave no estuviera, y son cosas distintas. */
int      s_sqlite_res      = 0;      /* bpvm_sqlite_res_t                     */
long     s_sqlite_asked_mb = 0;      /* lo que pedia el ENV (para citarlo)    */

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
/* Timeout en vez de bloqueante puro: un dispositivo ausente o un bus
 * atascado devuelve error limpio (-1) en lugar de colgar/abortar el
 * periférico, que en el RP2350 podía tumbar el run (RuntimeError no
 * atrapable). Imprescindible para que I2c.scan() tolere direcciones
 * vacías. Holgura: 2 ms fijos + 1 ms/byte (a 100 kHz un byte son ~0,2 ms,
 * así que no hay falsos timeouts con un dispositivo real). */
static int pico_i2c_write_impl(int bus, int addr, const uint8_t* data, size_t n) {
    i2c_inst_t* inst = i2c_inst_for(bus);
    int r = i2c_write_timeout_us(inst, (uint8_t) addr, data, n, false,
                                 (uint) (2000u + 1000u * n));
    return (r < 0) ? -1 : r;
}
static int pico_i2c_read_impl(int bus, int addr, uint8_t* data, size_t n) {
    i2c_inst_t* inst = i2c_inst_for(bus);
    int r = i2c_read_timeout_us(inst, (uint8_t) addr, data, n, false,
                                (uint) (2000u + 1000u * n));
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

/* board-aware: nº de canales ADC (RP2350A=4 / RP2350B=8, u override de
 * /sys/board.json). Sin este cb, Pico.ADC_CHANNELS() caía al fallback 4 de
 * src/pico.c y la Metro reportaba 4 en vez de 8 (el firmware INFO, que lee
 * board_desc directo, ya daba 8 — de ahí la discrepancia). */
static int pico_pico_adc_channels_impl(void) {
    return board_desc()->adc_channels;
}

/* board-aware: nº de slices PWM (12 en ambas variantes RP2350; el descriptor
 * es la fuente por si un board.json lo cambiara). Antes caía al fallback 12,
 * correcto por coincidencia; ahora es explícito por simetría con el ADC. */
static int pico_pico_pwm_slices_impl(void) {
    return board_desc()->pwm_slices;
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

/* paso 4 cierre — causa del último reset. Registro HW, sin RAM retenida.
 *
 * OJO con el watchdog del RP2350: el reinicio por software TAMBIÉN pasa por él.
 * El verbo RESET del wire llama a watchdog_reboot() (repl_v1.c), así que un
 * reinicio pedido desde el IDE —cambiar particiones, por ejemplo— dejaba el
 * INFO diciendo "watchdog", que se lee como un cuelgue. En H13 (3-ago) eso mandó
 * a buscar un cuelgue inexistente, y peor: con el mismo texto para las dos cosas,
 * la prueba del watchdog de verdad (WdtTest/WdtDemo) no demostraba nada.
 *
 * watchdog_enable_caused_reboot() del SDK sí los separa: sólo es cierto cuando
 * disparó un watchdog ARMADO con watchdog_enable(), no cuando alguien pidió el
 * reinicio con watchdog_reboot(). El orden importa — el caso específico primero. */
static const char* pico_pico_reset_cause_impl(void) {
    if (watchdog_enable_caused_reboot()) return "watchdog";        /* disparó de verdad */
    if (watchdog_caused_reboot())        return "reinicio pedido"; /* RESET del IDE */
    return "power-on/run";
}

static const bpvm_pico_backend_t s_pico_pico_backend = {
    .uniqueId      = pico_pico_unique_id_impl,
    .boardName     = pico_pico_board_name_impl,
    .tempC         = pico_pico_temp_c_impl,
    .cpuFreqHz     = pico_pico_cpu_freq_hz_impl,
    .uptimeMs      = pico_pico_uptime_ms_impl,
    .setCpuFreqMHz = pico_pico_set_cpu_freq_mhz_impl,
    .gpioCount     = pico_pico_gpio_count_impl,
    .adcChannels   = pico_pico_adc_channels_impl,  /* board-aware: Metro=8 (era fallback 4) */
    .pwmSlices     = pico_pico_pwm_slices_impl,    /* board-aware (12 ambas) */
    .resetCause    = pico_pico_reset_cause_impl,   /* paso 4 cierre */
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

/* Tope REAL de watchdog_enable(). El contador es de 24 bits (WATCHDOG_LOAD_BITS)
 * y cuenta µs —x2 en RP2040 por la errata E1—, así que el SDK hace
 * `load = delay_ms * (1000 * XFACTOR)` en uint32. Pasarse NO da "más plazo":
 * el producto desborda y/o el SDK recorta a WATCHDOG_LOAD_BITS. Sale ~16777 ms
 * en RP2350 y ~8388 ms en RP2040. Se CALCULA (no se pone a ojo) para que siga
 * bien si cambia el silicio; el #if es el mismo que usa el SDK en watchdog.c,
 * porque WATCHDOG_XFACTOR es privado de ese .c y no lo exporta ningún header. */
#if PICO_RP2040
#define PICO_WDT_XFACTOR 2u     /* errata RP2040-E1: decrementa 2 veces por tick */
#else
#define PICO_WDT_XFACTOR 1u
#endif
#define PICO_WDT_MAX_MS (WATCHDOG_LOAD_BITS / (1000u * PICO_WDT_XFACTOR))

static void pico_wdt_enable_impl(int timeoutMs) {
    /* watchdog_enable(ms, pause_on_debug=true). pause_on_debug evita
     * que el watchdog dispare cuando el chip está paused en debugger,
     * que es lo razonable para desarrollo. */
    if (timeoutMs <= 0) timeoutMs = 100;
    /* Recorte al tope: por encima, el uint32 del SDK desborda y el plazo que
     * sale no se parece al pedido — puede quedar en microsegundos y resetear
     * la placa al instante. Mejor un plazo corto que uno IMPREDECIBLE. */
    if ((uint32_t) timeoutMs > PICO_WDT_MAX_MS) timeoutMs = (int) PICO_WDT_MAX_MS;
    watchdog_enable((uint32_t) timeoutMs, true);
    s_wdt_active = 1;
}

static void pico_wdt_feed_impl(void) {
    if (s_wdt_active) watchdog_update();
}

static void pico_wdt_disable_impl(void) {
    /* watchdog_disable() limpia WATCHDOG_CTRL_ENABLE_BITS: PARA el contador de
     * verdad, sin rebotar ni dejar nada armado.
     * Antes esto llamaba a watchdog_enable(0x7FFFFF) creyendo que era un plazo
     * "tan largo que nunca dispara". No existe tal valor: el SDK recorta el load
     * a WATCHDOG_LOAD_BITS, así que el techo son ~16,8 s (RP2350) pase lo que
     * pase. O sea que Wdt.disable() NO desactivaba: dejaba el perro armado a
     * ~16,8 s y la placa se reseteaba sola si el programa seguía sin feed. */
    watchdog_disable();
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

/* Lector por trozos para el loader (H11): el .mod nunca pasa entero por RAM. */
static long boot_mod_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    return bpvm_fs_read_at((const char*) user, off, dst, n);
}

/* Ejecuta una vez la VM sobre Hello.mod cargado del FS. */
static bpvm_status_t run_vm_once(int iteration) {
    printf("\n");
    printf("===========================================\n");
    printf(" bpgenvm-c on RP2350 / FreeRTOS — FP2 step 1\n");
    printf(" iteration #%d (FS-backed)\n", iteration);
    printf("===========================================\n");

    /* H11 — el .mod se queda en el FS: sólo miramos que exista y cuánto ocupa;
     * los bytes los lee el loader por trozos, directamente a su sitio. */
    uint32_t size;
    if (bpvm_fs_stat("Hello.mod", &size) != 0) {
        printf("[ERR] Hello.mod no está en el FS\n");
        return BPVM_ERR_IO;
    }
    printf("[fs] Hello.mod %u bytes  (fs_used=%u/%u)\n",
           (unsigned) size, (unsigned) fs_used_bytes(),
           (unsigned) fs_total_bytes());

    /* H11 — el TAMAÑO REAL, no la constante: la región se recorta si malloc
     * necesita su margen, y decirle a la VM que tiene más de lo que hay es
     * exactamente cómo se pisa memoria. */
    /* EL MISMO reparto que el RUN y el INFO: vm_stack_region_bytes() (repl_v1.c)
     * es el único sitio donde vive la regla. Aquí se PIDE, no se recalcula —
     * copiarla fue justo lo que hizo que el INFO enseñara 171+171. */
    size_t stack_region = vm_stack_region_bytes();
    bpvm_t* vm = bpvm_init(s_vm_buffer, s_vm_buffer_size,
                           s_vm_buffer_size - stack_region);
    if (!vm) {
        printf("[ERR] bpvm_init failed\n");
        return BPVM_ERR_OOM;
    }
    bpvm_set_output(vm, usb_sink, NULL);

    bpvm_status_t s = bpvm_load_mod_stream(vm, boot_mod_read_at,
                                            (void*) "Hello.mod", size, "Hello");
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
/* ===== H9: arranque escalonado (unificación 19-jul) =====================
 * El boot deja de ser monolítico: 0) identidad en el SUELO → 1) particiones
 * del env → 2) FS → 3) VM. bpvm_boot_climb sube capa a capa y PARA en la
 * primera que falla (último estado bueno + motivo). STATE (wire) reporta este
 * estado REAL vía board_boot_status(), y repl_v1 gatea los comandos FS/RUN. */

static bpvm_env_t         s_env;      /* env de la zona 2 (payload apunta a XIP) */
static bpvm_part_layout_t s_layout;   /* particiones derivadas del env */
static bpvm_boot_status_t s_boot;     /* estado REAL alcanzado por el boot */

const bpvm_boot_status_t* board_boot_status(void) { return &s_boot; }

/* #355 — una clave booleana del ENV, para quien la necesite fuera de main.c.
 * El env ya vive aquí (s_env, payload en XIP); esto sólo abre la ventanilla en
 * vez de repartir copias del bloque. Hoy la usa repl_v1 para el interruptor del
 * GC (`gc=0`): sirve para PARTIR el experimento en dos —misma memoria, mismo
 * programa, con y sin recolector— sin regrabar la placa entre prueba y prueba. */
int board_env_bool(const char* key, int def) {
    return bpvm_env_get_bool(&s_env, key, def);
}

/* #327 — el layout REAL del arranque, para quien necesite una partición que no
 * sea el FS (hoy: la zona de packs del gestor de placa). NULL mientras el boot
 * no haya pasado de la capa 1: sin layout válido no hay particiones que dar.
 * El STM32 tenía su propio s_layout dentro del board_mgr; aquí se comparte el
 * del boot, que es el que manda. */
const bpvm_part_layout_t* board_partitions(void) {
    return (s_boot.state >= BPVM_BOOT_PARTITIONS) ? &s_layout : NULL;
}

/* 0→1: particiones del env (mismo base+clamp que board_mgr — una sola verdad). */
static bpvm_boot_step_t layer_partitions(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.ok = 0; r.reason[0] = '\0';
    uint32_t usable = bpvm_part_usable_flash(board_desc()->flash_bytes,
                                             PICO_FLASH_SIZE_BYTES);   /* clamp #292 */
    int bad = -1;
    bpvm_part_err_t e = bpvm_part_layout(&s_env, BP_PART_BASE, usable,
                                         FLASH_SECTOR_SIZE, &s_layout, &bad);
    if (e == BPVM_PART_OK) { r.ok = 1; return r; }
    snprintf(r.reason, sizeof r.reason, "%s", bpvm_part_err_str(e));
    return r;
}

/* 1→2: montar littlefs en la región FS derivada (formatea si no monta:
 * región recién definida por el usuario = legítimo). */
static bpvm_boot_step_t layer_fs(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.ok = 0; r.reason[0] = '\0';
    const bpvm_part_t* fsp = bpvm_part_get(&s_layout, BPVM_PART_FS);
    if (!fsp) { snprintf(r.reason, sizeof r.reason, "sin region FS"); return r; }
    if (fs_init_at(fsp->offset, fsp->size) != FS_OK) {
        snprintf(r.reason, sizeof r.reason, "littlefs no monta ni formatea");
        return r;
    }
    r.ok = 1;
    return r;
}

/* 2→3: la VM está lista si hay heap (SRAM o PSRAM, ya elegido). */
static bpvm_boot_step_t layer_app(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.reason[0] = '\0';
    r.ok = (s_vm_buffer != NULL && s_vm_buffer_size > 0);
    if (!r.ok) snprintf(r.reason, sizeof r.reason, "heap de la VM no disponible");
    return r;
}

/* #354 — handle de la task de la VM, para poder preguntarle su marca de agua
 * desde el INFO (lo responde la task de comms, no esta). Ver bpvm_stack_probe. */
TaskHandle_t g_vm_task = NULL;

static void vm_task(void* arg) {
    (void) arg;

    log_printf("vm_task: started");
    setvbuf(stdout, NULL, _IONBF, 0);
    log_printf("vm_task: setvbuf OK");

#if defined(BPVM_PICO_BRINGUP) && BPVM_PICO_BRINGUP == 9
    /* #292 — nivel 9: LA PELÍCULA EN DIRECTO, un solo flasheo (idea de Eduardo:
     * "la comunicación es nuestro chivato").
     *
     * El truco: ESPERAR a que el USB esté enumerado ANTES de tocar nada. Así el
     * puerto ya existe cuando llega el fallo, y no se pierde el desenlace. Y de
     * regalo: panic() del SDK IMPRIME ("*** PANIC ***" + el mensaje, panic.c:65)
     * — o sea que un hard_assert nos cuenta él mismo qué aserto reventó.
     *
     * Cada paso se ANUNCIA antes de darlo: la última línea que veas es el paso
     * que mató la placa. */
    /* PAUSA LARGA con cuenta atrás (Eduardo: "necesito que el arranque sea más
     * lento, que me dé tiempo a abrir el puerto"). No sirve esperar a
     * stdio_usb_connected(): con PICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1 eso es
     * tud_ready(), que es true en cuanto enumera — no espera a que ABRAS. Así que
     * esperamos por reloj: 30 s cantando cada segundo. Si ves la cuenta atrás,
     * la placa está viva y te da tiempo de sobra a abrir el terminal. */
    for (int s = 30; s > 0; s--) {
        printf("bringup-9: esperando %2d s antes de tocar nada (abre el puerto)\n", s);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("\n===== bringup-9: arranque paso a paso =====\n");
    printf("[1] USB listo. A partir de aquí, la ULTIMA linea que veas es la que mata.\n");

    printf("[2] probe JEDEC (flash_do_cmd 0x9F)...\n");
    unsigned fb = board_desc_probe_flash_bytes();
    printf("[2] OK -> flash = %u bytes (%u MB)\n", fb, fb / (1024u * 1024u));

    printf("[3] LO QUE VIENE: fs_init() escribe en flash. El board header declara\n");
    printf("    PICO_FLASH_SIZE_BYTES = %u (%u MB), y el SDK hace hard_assert de que\n",
           (unsigned) PICO_FLASH_SIZE_BYTES, (unsigned) PICO_FLASH_SIZE_BYTES / (1024u*1024u));
    printf("    todo flash_range_erase/program caiga DENTRO de ese limite.\n");
    printf("    Si el FS de esta placa empieza en 0x400000 y el limite son 4MB -> panic.\n");
    printf("[3] H9: el mount del FS lo conduce el env (particiones derivadas);\n");
    printf("    este nivel de bringup ya no monta nada por si mismo.\n");

    printf("[4] llamando a board_desc_init() (board.json + sondeo QMI de PSRAM)...\n");
    board_desc_init();
    printf("[4] OK -> variante %c, psram %u MB\n", board_desc()->variant,
           (unsigned)(board_desc()->psram_bytes / (1024u * 1024u)));

    printf("===== TODO EL ARRANQUE SOBREVIVIO =====\n");
    for (unsigned i = 0; ; i++) {
        printf("bringup-9 vivo %u (arranque completo OK)\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#endif

#if defined(BPVM_PICO_BRINGUP) && BPVM_PICO_BRINGUP == 26
    /* #292 — nivel 2.6: parte fs_init() por dentro. Hace SÓLO el primer paso,
     * el probe JEDEC (board_desc_probe_flash_bytes → flash_do_cmd 0x9F, que
     * suspende el XIP), y CANTA EL NÚMERO. No escribe el descriptor ni monta
     * ni formatea nada.
     *   · Si habla: el JEDEC es inocente y el asesino es escribir/formatear.
     *     Y de propina sabemos qué tamaño de flash ve la Metro — que es el
     *     único dato del que cuelga TODO el layout del FS (>4MB → 12 MB de
     *     volumen; si sale 0 o un valor raro, el descriptor sale mal).
     *   · Si calla: es el probe JEDEC, y estamos en el #256 otra vez. */
    for (unsigned i = 0; ; i++) {
        unsigned fb = board_desc_probe_flash_bytes();
        printf("bringup-2.6 vivo %u — JEDEC dice flash = %u bytes (%u MB)%s\n",
               i, fb, fb / (1024u * 1024u),
               fb == 0 ? "  <-- CERO: JEDEC no reconocido!" : "");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
#endif

#if defined(BPVM_PICO_BRINGUP) && BPVM_PICO_BRINGUP == 25
    /* #292 — nivel 2.5: parte el hueco entre el 2 y el 3, que metía DOS cosas
     * de golpe (scheduler+core1 Y fs_init). Aquí el scheduler YA corre y la
     * tarea vive, pero NO se ha tocado el FS. Si el 2 habla y el 2.5 calla,
     * el asesino es el scheduler/core 1, no el FS. Si el 2.5 habla y el 3
     * calla, es el FS. Una variable cada vez. */
    for (unsigned i = 0; ; i++) {
        printf("bringup-2.5 vivo %u (scheduler + vm_task OK, SIN tocar el FS)\n", i);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
#endif

    /* ===== H9: subida escalonada (unificación 19-jul) =====
     * 0) Identidad en el SUELO: variante (PACKAGE_SEL) + flash real (JEDEC).
     *    Hardware puro — sin FS, sin env. */
    board_desc_early_init();

    /* Env de la ZONA 2, por XIP (sin copia y sin FS): la verdad de la placa. */
    bpvm_env_pick((const uint8_t*)(XIP_BASE + BP_ENV_A_OFFSET), FLASH_SECTOR_SIZE,
                  (const uint8_t*)(XIP_BASE + BP_ENV_B_OFFSET), FLASH_SECTOR_SIZE,
                  &s_env);

    /* PSRAM conducida por el env (`psram=1` ∧ RP2350B → CS=GPIO47), ANTES de
     * elegir el heap — "distribuir la memoria pronto" (Eduardo, H9). */
    board_desc_psram_from_env(bpvm_env_get_bool(&s_env, "psram", 0));

    /* V5/H1 — los pines del lector de SD, en UNA entrada con etiquetas (decisión
     * de Eduardo: una sola línea es más simple para el usuario que cinco):
     *
     *     sd=sck:34,mosi:35,miso:36,cs:39,cd:40      (los de la Metro)
     *
     * Aquí SOLO se leen y se anotan: no se toca la tarjeta. El arranque no es
     * sitio para hablar con hardware que puede no estar — eso lo dispara el
     * verbo SD_INFO cuando el usuario quiere, por la misma razón que el pack
     * nativo (un cuelgue en el arranque se repite en cada arranque). */
    {
        char linea[96];
        int n = bpvm_env_get(&s_env, "sd", linea, sizeof linea);
        if (n < 0) {
            s_sd_hay_config = 0;
            snprintf(s_sd_motivo, sizeof s_sd_motivo,
                     "esta placa no declara lector de SD (no hay entrada 'sd' en el env)");
        } else if (bpvm_sd_pines_parse(linea, &s_sd_pines,
                                       s_sd_motivo, sizeof s_sd_motivo) != 0) {
            s_sd_hay_config = 0;        /* el motivo ya lo escribió el parser */
        } else {
            s_sd_hay_config = 1;
            s_sd_motivo[0] = '\0';
        }
        log_printf("sd: %s", s_sd_hay_config ? "pines configurados" : s_sd_motivo);
    }

#if defined(BPVM_PICO_BRINGUP) && BPVM_PICO_BRINGUP == 3
    /* nivel 3 (H9): identidad + env + psram hechos, SIN tocar el FS aún. */
    for (unsigned i = 0; ; i++) {
        printf("bringup-3 vivo %u (identidad: variante %c, flash %u MB, psram %u MB)\n",
               i, board_desc()->variant,
               (unsigned)(board_desc()->flash_bytes / (1024u * 1024u)),
               (unsigned)(board_desc()->psram_bytes / (1024u * 1024u)));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
#endif

    /* H7.2.b — si hay PSRAM USABLE (detectada + QPI + RW test OK), el heap de la
     * VM va a la ventana PSRAM (MBs). El resto del firmware no cambia: sólo de
     * dónde sale s_vm_buffer.
     * H11 — y si NO la hay, se toma del final de la RAM (vm_sram_region). La
     * diferencia con antes: en la placa CON PSRAM no se reserva nada de SRAM
     * interna, porque ya no hay array estático que reservar. */
    if (board_desc()->psram_present && board_desc()->psram_bytes >= (1u << 20)) {
        /* V5/H — la BD muerde PRIMERO, del principio de la ventana. Así su
         * dirección es la misma en toda placa con este layout (no depende de
         * cuánta PSRAM haya), que es lo que el IDE necesita para pre-enlazar el
         * pack. La regla vive en bpvm_sqlmem (un solo sitio, como #335). */
        size_t sqlbytes = 0;
        s_sqlite_asked_mb = bpvm_env_get_long(&s_env, "SQLite", 0);
        bpvm_sqlite_res_t sqlres = bpvm_sqlite_region(
                s_sqlite_asked_mb,
                (size_t) board_desc()->psram_bytes, &sqlbytes);
        s_sqlite_res = (int) sqlres;

        s_vm_buffer      = (uint8_t*) (uintptr_t) PSRAM_XIP_BASE;
        s_vm_buffer_size = board_desc()->psram_bytes;

        if (sqlres == BPVM_SQLITE_OK) {
            s_sqlite_base     = s_vm_buffer;
            s_sqlite_size     = (uint32_t) sqlbytes;
            s_vm_buffer      += sqlbytes;
            s_vm_buffer_size -= (uint32_t) sqlbytes;

            /* V5/I — la RAM del pack sale del PRINCIPIO de este bloque:
             * `[estáticos | arena]`, como se decidió al cerrar la RAM de la BD.
             * Criterio de Eduardo (7-ago): *"la reserva solamente hace falta si
             * SQLite=0"* — donde hay arena no se le quita nada a la SRAM. */
            s_pack_ram_base = s_sqlite_base;
        }

        log_printf("vm: heap en PSRAM %u MB @ 0x%08x (SRAM interna sin reservar)",
                   (unsigned)(s_vm_buffer_size / (1024u * 1024u)),
                   (unsigned)(uintptr_t) s_vm_buffer);
        /* El reparto, EXPLÍCITO — igual que en la rama de SRAM: que se vea, en
         * vez de deducirlo. Y el motivo de un NO se dice siempre: "se pidió mal"
         * no puede parecerse a "no se pidió" (patrón del clamp de #292). */
        if (sqlres != BPVM_SQLITE_OFF) {
            log_printf("bd: %s (SQLite=%ld) -> %u KB @ 0x%08x",
                       bpvm_sqlite_res_str(sqlres),
                       s_sqlite_asked_mb,
                       (unsigned)(s_sqlite_size / 1024u),
                       (unsigned)(uintptr_t) s_sqlite_base);
        }
    } else {
        /* Sin PSRAM no hay arena de donde sacarla, así que la RAM del pack se
         * reserva de la SRAM — y `vm_sram_region` ya se para justo debajo. Es el
         * caso degenerado: cuando hay arena, esto no cuesta nada. */
        s_pack_ram_base = (uint8_t*) (uintptr_t) PACK_RAM_SRAM_BASE;
        s_vm_buffer = vm_sram_region(&s_vm_buffer_size);
        if (s_vm_buffer == NULL) {
            log_printf("vm: NO HAY SITIO en SRAM para el buffer de la VM");
            log_flush();
        } else {
            /* El reparto, explícito: si algún día la reserva de malloc se queda
             * corta o el .bss se come la RAM, se ve aquí en vez de deducirlo. */
            size_t stk = vm_stack_region_bytes();   /* la regla, no una copia */
            log_printf("vm: SRAM interna %u KB @ 0x%08x -> heap %u KB + stacks %u KB | libre para malloc: %u KB",
                       (unsigned)(s_vm_buffer_size / 1024u), (unsigned)(uintptr_t) s_vm_buffer,
                       (unsigned)((s_vm_buffer_size - stk) / 1024u), (unsigned)(stk / 1024u),
                       (unsigned)(((uintptr_t) s_vm_buffer - (uintptr_t) &end) / 1024u));
        }
    }

    /* V5/I — la tabla BIOS del pack nativo, VERIFICADA aquí aunque no haya
     * ningún pack grabado. El motivo de comprobarla siempre: un hueco que sólo
     * se descubre el día que alguien graba un pack aparece cuando YA estás
     * depurando otra cosa, y entonces cuesta el doble. Que la placa diga en cada
     * arranque si su BIOS está entera. */
    {
        const bpvm_bios_t* bios = bios_pico_get();   /* él ya loguea qué falta */
        if (bios) log_printf("bios: lista (%d ranuras, v%u)",
                             bpvm_bios_slot_count(), (unsigned) bios->version);

        /* EL ANCLA, y su CONTROL. No basta con que la búsqueda encuentre algo:
         * tiene que encontrar EXACTAMENTE el objeto que este firmware conoce por
         * su nombre. Si difieren, la marca ha aparecido dos veces en la imagen y
         * el pack leería punteros de otro sitio — que es el fallo que el ancla
         * viene a evitar, así que sería especialmente tonto no mirarlo.
         *
         * Y se comprueba en CADA arranque, haya pack o no: si un día el enlace
         * deja la marca duplicada, se sabe ese día. */
        const bpvm_ancla_t* real = bios_pico_ancla();
        const bpvm_ancla_t* hallada =
            bpvm_ancla_buscar((const void*) XIP_BASE, ANCLA_BARRIDO_BYTES);
        if (hallada == real) {
            log_printf("ancla: en 0x%08lX (la busqueda la encuentra, y es LA misma)",
                       (unsigned long) (uintptr_t) real);
        } else if (hallada == 0) {
            log_printf("ancla: NO SE ENCUENTRA barriendo %u KB desde 0x%08lX "
                       "(esta en 0x%08lX) -> ningun pack podra hallar la BIOS",
                       (unsigned) (ANCLA_BARRIDO_BYTES / 1024),
                       (unsigned long) XIP_BASE, (unsigned long) (uintptr_t) real);
        } else {
            log_printf("ancla: DUPLICADA — la busqueda da 0x%08lX y la buena es "
                       "0x%08lX -> un pack leeria punteros de basura",
                       (unsigned long) (uintptr_t) hallada,
                       (unsigned long) (uintptr_t) real);
        }
    }

    /* 1)→3): particiones del env → FS → VM. bpvm_boot_climb para en la
     * PRIMERA capa que falla: último estado bueno + motivo (nada se
     * auto-inicializa; sin particiones NO hay FS — norma de Eduardo). */
    {
        bpvm_boot_layers_t layers;
        layers.to_partitions = layer_partitions;
        layers.to_fs         = layer_fs;
        layers.to_app        = layer_app;
        layers.user          = NULL;
        layers.max_state     = BPVM_BOOT_APP;
        bpvm_boot_climb(&layers, &s_boot);
    }
    log_printf("boot: estado %d (%s)%s%s", (int) s_boot.state,
               bpvm_boot_state_name(s_boot.state),
               s_boot.degraded ? " DEGRADADO: " : "",
               s_boot.degraded ? s_boot.reason : "");

    if (s_boot.state >= BPVM_BOOT_FS) {
    /* H7.3: overrides de /sys/board.json (name/led/neopixel) — ya con FS. La
     * variante, la flash y la PSRAM vienen del suelo/env, no de aquí. */
    board_desc_init();

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
    /* #305 — pre-instalación de la stdlib embebida. Antes eran 14 bloques
     * copiados a mano, cada uno preguntando "¿existe?" con fs_get, que LEE EL
     * FICHERO ENTERO al scratch de 128 KB para no mirar ni un byte (Gui.mod son
     * 42 KB). Ahora: una tabla —como las que ya tenían el ESP32 y el STM32— y
     * fs_exists, que es un stat y no toca el scratch.
     *
     * NO se persiste a flash a propósito: un FORMAT borra las apps del usuario
     * pero la stdlib resurge en el siguiente arranque desde la imagen. */
    /* La longitud se guarda POR DIRECCIÓN: los *_mod_len son variables
     * (`extern const unsigned int`), no constantes de compilación, así que su
     * VALOR no vale en un inicializador estático — su dirección sí. Con esto la
     * tabla vive en flash y no gasta ni un byte de pila. */
    static const struct { const char* path; const uint8_t* data; const unsigned int* len; }
    PREINSTALL[] = {
        { "/lib/Core.mod",     core_mod,     &core_mod_len     },
        { "/lib/Gpio.mod",     gpio_mod,     &gpio_mod_len     },
        { "/lib/I2c.mod",      i2c_mod,      &i2c_mod_len      },
        { "/lib/Spi.mod",      spi_mod,      &spi_mod_len      },
        { "/lib/Uart.mod",     uart_mod,     &uart_mod_len     },
        { "/lib/Pulse.mod",    pulse_mod,    &pulse_mod_len    },
        { "/lib/Pwm.mod",      pwm_mod,      &pwm_mod_len      },
        { "/lib/Pico.mod",     pico_mod,     &pico_mod_len     },
        { "/lib/Rtc.mod",      rtc_mod,      &rtc_mod_len      },
        { "/lib/Adc.mod",      adc_mod,      &adc_mod_len      },
        { "/lib/Wdt.mod",      wdt_mod,      &wdt_mod_len      },
        { "/lib/Timer.mod",    timer_mod,    &timer_mod_len    },
        { "/lib/Neopixel.mod", neopixel_mod, &neopixel_mod_len },
        /* Los drivers de dispositivo (PCA9554, BME280, SSD1306...) NO se
         * pre-instalan: los sube el IDE como deps al hacer Run. */
        { "/app/Hello.mod",    hello_mod,    &hello_mod_len    },
    };
    for (size_t i = 0; i < sizeof(PREINSTALL) / sizeof(PREINSTALL[0]); i++) {
        if (fs_exists(PREINSTALL[i].path)) {
            /* #422 — el /lib rancio, VISIBLE (espejo del chivato del ESP32:
             * mismo criterio, mismo mensaje). Reflashear no refresca estos
             * módulos; que al menos el arranque diga cuándo lo desplegado no
             * es lo embebido. Tamaño primero (gratis), CRC solo si empatan. */
            uint32_t sz = 0;
            int difiere = (bpvm_fs_stat(PREINSTALL[i].path, &sz) == 0
                           && sz != *PREINSTALL[i].len);
            if (!difiere && sz == *PREINSTALL[i].len) {
                uint32_t c_fs = 0;
                if (bpvm_fs_crc32(PREINSTALL[i].path, &c_fs) == 0)
                    difiere = (c_fs != bpvm_crc32(PREINSTALL[i].data,
                                                  *PREINSTALL[i].len));
            }
            if (difiere)
                log_printf("lib: %s NO es el de esta imagen (%u B en FS, %u embebido)"
                           " - ¿rancio de otro firmware, o subido por ti?",
                           PREINSTALL[i].path, (unsigned) sz,
                           (unsigned) *PREINSTALL[i].len);
            continue;
        }
        fs_put(PREINSTALL[i].path, PREINSTALL[i].data, *PREINSTALL[i].len);
        log_printf("preinstall: %s (%u bytes)", PREINSTALL[i].path,
                   (unsigned) *PREINSTALL[i].len);
    }

    log_printf("fs: %d ficheros, %u/%u bytes usados",
               fs_file_count(), (unsigned) fs_used_bytes(),
               (unsigned) fs_total_bytes());

    /* V5/H2 — la tarjeta SD, si la hay. AQUÍ y no antes: /sd se monta ENCIMA de
     * la fachada, y sin el FS de la placa no hay dónde colgarlo. Y antes del
     * autoarranque, para que una app encuentre la tarjeta ya montada.
     *
     * EL PIN DE DETECCIÓN MANDA (criterio de Eduardo, 8-ago): sin tarjeta
     * metida no tiene sentido montar, y así el arranque no toca ni el SPI. Es
     * lo que permite que esto pase a ser automático — hasta ahora se hacía a
     * mano justamente para no hablarle al vacío en cada encendido.
     *
     * ⚠️ Que falle NO degrada el arranque. Una tarjeta ilegible o en exFAT es
     * un accesorio que no va, no una placa rota: se anota el motivo y se sigue.
     * Degradar el boot por esto dejaría al usuario sin IDE para averiguar qué
     * pasa, que es exactamente cuando más falta hace. */
    if (!s_sd_hay_config) {
        log_printf("sd: sin configurar en el env — no se monta");
    } else if (!bpvm_sd_hay_tarjeta(&s_sd_pines)) {
        log_printf("sd: zocalo VACIO (pin de deteccion) — no se monta");
    } else {
        char motivo[80];
        /* V5/H6 — el montaje ya no recibe pines: recibe un DISPOSITIVO DE
         * BLOQUE. Aquí se elige cuál (la SD por SPI); en el P4 será el de
         * SDMMC y esta línea es la única que cambia. */
        if (bpvm_fs_fat_montar(bpvm_sd_blk(&s_sd_pines), "/sd",
                               motivo, sizeof motivo) == 0) {
            log_printf("sd: montada en /sd (particion en el bloque %u)",
                       (unsigned) bpvm_fs_fat_lba_particion());
        } else {
            log_printf("sd: NO montada — %s", motivo);
        }
    }
    }  /* fin estado >= FS (H9): sin particiones/FS no hay board.json/stdlib */

    /* H13 hallazgo 12 — AQUÍ CORRÍA UN fib(28) EN C EN CADA ARRANQUE. Era el
     * micro-bench de #159, el que tenía que decidir si el AOT valía la pena
     * ("si el ratio C:BP < 2× aparcamos AOT"). La decisión se tomó hace un
     * año y el AOT está publicado con 113× medidos, así que lo único que
     * seguía haciendo era gastar ~105 ms de CADA boot de CADA usuario para
     * escribir un número que ya nadie compara. Retirado con bench.c/.h. */

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
    /* #423 — A PARTIR DE AQUI, EL LOG LO MANDA EL ENTORNO (`log=0|1`).
     *
     * El arranque entero queda registrado SIEMPRE: son unas quince lineas y no
     * llenan nada, y son justo las que hacen falta cuando una placa no arranca.
     * Lo que se apaga es el rastro de EJECUCION — el que llena la region en
     * ~26 colectas del GC y hacia que un cuelgue no dejara su ultimo momento
     * escrito. Por defecto APAGADO: `log=1` cuando se va a depurar. */
    bpvm_log_set_enabled(bpvm_env_get_bool(&s_env, "log", 0));

    if (s_boot.state == BPVM_BOOT_APP && !s_boot.degraded)
        repl_v1_autorun();   /* H9: autorun solo con la placa SANA en estado 3 */
    repl_v1_run();
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

/* #353 — sink de diagnóstico de la VM: al log persistente, no al wire. */
static void diag_al_log(const char* linea) { log_printf("%s", linea); }

int main(void) {
#if defined(BPVM_PICO_BRINGUP) && BPVM_PICO_BRINGUP == 1
    /* #292 — bring-up nivel 1: el mínimo absoluto. Sólo USB y un bucle.
     * NADA de LED (en la Metro el pin 25 de PICO_DEFAULT_LED_PIN es la línea
     * del NeoPixel, no un LED), log, FS, board_desc, PSRAM ni scheduler.
     * Si esto no enumera, el problema no es nuestro software de arriba. */
    stdio_init_all();
    stdio_set_translate_crlf(&stdio_usb, false);
    for (unsigned i = 0; ; i++) {
        printf("bringup-1 vivo %u\n", i);
        sleep_ms(500);
    }
#else
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
    /* #353 — y en cuanto hay log, se le desvía lo que dice la VM. En la Pico
     * NO es cosmético: el `_write` del SDK manda stderr al MISMO USB CDC que el
     * wire, y a diferencia de wire_v1_send_line no coge el mutex TX → podía
     * partir un frame JSON por la mitad. Al log, además, sobrevive al reset. */
    bpvm_diag_set_sink(diag_al_log);
    /* #355 — y el volcado del canal URGENTE. `log_printf` sólo escribe en RAM;
     * sin esto, todo lo anotado desde el último volcado se pierde si la placa se
     * cuelga — que es justo cuando hace falta. Con esto, los avisos que pueden ir
     * seguidos de una muerte (sin memoria, excepción sin handler, bloque
     * descarrilado) están en flash ANTES de que pase lo que sea. */
    bpvm_diag_set_flush(log_flush);
    log_printf("=== boot " __DATE__ " " __TIME__ " ===");
    /* La causa del reset ya la sabía el firmware (era un builtin de BP), pero no
     * la contaba al arrancar: distinguir "watchdog" de "power-on" es gratis y
     * ahorra media investigación. */
    log_printf("boot: causa del reset = %s",
               watchdog_caused_reboot() ? "WATCHDOG" : "power-on/run");

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

#if defined(BPVM_PICO_BRINGUP) && BPVM_PICO_BRINGUP == 2
    /* #292 — nivel 2: todo lo de main() (led + stdio + log_init + backends) y
     * PARA. log_init() ya toca flash (su sector propio), así que este nivel
     * distingue "la flash mata el boot" de "el FS mata el boot". No arranca el
     * scheduler: sin core 1 no hay multicore_lockout, y fs_init() se colgaría. */
    for (unsigned i = 0; ; i++) {
        printf("bringup-2 vivo %u (led+stdio+log_init+backends OK)\n", i);
        sleep_ms(500);
    }
#endif
    /* #354 — el handle se GUARDA (antes se tiraba con NULL). Sin él no se le
     * puede preguntar su marca de agua desde la task de comms, que es la que
     * responde al INFO. Es la unica razon del cambio: diagnostico. */
    BaseType_t r = xTaskCreate(vm_task, "vm_task", 4096, NULL,
                                tskIDLE_PRIORITY + 2, &g_vm_task);
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
#endif  /* BPVM_PICO_BRINGUP == 1 */
    return 0;
}
