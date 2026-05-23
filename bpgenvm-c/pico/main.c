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
#include "repl.h"
#include "log.h"
#include "bpvm_gpio.h"
#include "bpvm_i2c.h"
#include "bpvm_spi.h"
#include "bpvm_uart.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/uart.h"

/* Buffer estático de la VM. 128 KB sobra para Hello.bp y deja sitio para
 * varios módulos. */
#define VM_BUFFER_SIZE   (128 * 1024)
uint8_t s_vm_buffer[VM_BUFFER_SIZE] __attribute__((aligned(8)));
const uint32_t s_vm_buffer_size = VM_BUFFER_SIZE;

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

    /* Stdlib siempre pre-instalada: si no está en el FS, la copiamos
     * desde el array embebido. NO se persiste automáticamente a flash
     * (el usuario decide con SAVE) — así un FORMAT borra apps pero la
     * stdlib resurge en el siguiente reboot desde la imagen. */
    const uint8_t* dummy; uint32_t dummy_sz;
    if (fs_get("Gpio.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("Gpio.mod", gpio_mod, gpio_mod_len);
        log_printf("stdlib: Gpio.mod installed (%u bytes)", gpio_mod_len);
    }
    if (fs_get("I2c.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("I2c.mod", i2c_mod, i2c_mod_len);
        log_printf("stdlib: I2c.mod installed (%u bytes)", i2c_mod_len);
    }
    if (fs_get("Spi.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("Spi.mod", spi_mod, spi_mod_len);
        log_printf("stdlib: Spi.mod installed (%u bytes)", spi_mod_len);
    }
    if (fs_get("Uart.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("Uart.mod", uart_mod, uart_mod_len);
        log_printf("stdlib: Uart.mod installed (%u bytes)", uart_mod_len);
    }
    /* Drivers de dispositivo (PCA9554, BME280, SSD1306, ...) NO se
     * pre-instalan aquí — los sube el IDE como deps al hacer Run. */
    if (fs_get("Hello.mod", &dummy, &dummy_sz) != FS_OK) {
        fs_put("Hello.mod", hello_mod, hello_mod_len);
        log_printf("stdlib: Hello.mod installed (%u bytes)", hello_mod_len);
    }

    log_printf("fs: %d ficheros, %u/%u bytes usados",
               fs_file_count(), (unsigned) fs_used_bytes(),
               (unsigned) fs_total_bytes());

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

int main(void) {
    led_init();
    stdio_init_all();

    /* Log persistente: carga el snapshot anterior antes de pisarlo con
     * mensajes del boot actual. */
    log_init();
    log_printf("=== boot ===");

    /* Conecta los backends de HW reales (Pico SDK) a los builtins de
     * la VM. Sin esto los handlers caen al stub con logging. */
    bpvm_gpio_set_backend(&s_pico_gpio_backend);
    bpvm_i2c_set_backend(&s_pico_i2c_backend);
    bpvm_spi_set_backend(&s_pico_spi_backend);
    bpvm_uart_set_backend(&s_pico_uart_backend);

    BaseType_t r = xTaskCreate(vm_task, "vm_task", 4096, NULL,
                                tskIDLE_PRIORITY + 2, NULL);
    if (r != pdPASS) {
        log_printf("xTaskCreate(vm_task) FAILED");
        log_flush();
        led_set(1);
        for (;;) {}
    }

    vTaskStartScheduler();
    for (;;) {}
    return 0;
}
