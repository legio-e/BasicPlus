/*
 * main.c — H4.3: firmware ESP32-S3 con REPL wire v1.
 *
 * Arranca el FS (RAM), el wire v1 sobre UART0 y entra al bucle del REPL.
 * El IDE (SerialBackend) conecta al UART0 (puerto del bridge USB-UART),
 * sube .mod (PUT), los ejecuta (RUN) y recibe la salida (OUTPUT/EXITED).
 *
 * Canales (ver esp32/README.md):
 *   - UART0 (bridge CP210x/CH340)  → wire v1, lo usa el IDE.
 *   - USB-Serial-JTAG (puerto nativo) → consola/logs ESP-IDF (printf),
 *     para depurar con `idf.py monitor` SIN contaminar el wire.
 */
#include <stdio.h>
#include <stdint.h>

#include "fs.h"
#include "repl_esp32.h"
#include "hw_esp32.h"
#include "esp32_mods.h"

/* Buffer caller-provided de la VM. repl_esp32.c lo referencia como extern
 * (PUNTERO, igual convención que repl_v1.c en la Pico/Metro y que el P4). El S3
 * lo mantiene en SRAM interna: s_vm_buffer apunta a este array estático. */
#define VM_BUFFER_SIZE (128 * 1024)
static uint8_t s_sram_vm[VM_BUFFER_SIZE];
uint8_t*       s_vm_buffer      = s_sram_vm;
uint32_t       s_vm_buffer_size = VM_BUFFER_SIZE;

void app_main(void)
{
    /* Estos printf van a la CONSOLA = USB-Serial-JTAG (puerto nativo),
     * NO al wire (UART0). Sirven para depurar el arranque. */
    printf("\n=== BasicPlus VM en ESP32-S3 (H4.3 — wire v1) ===\n");
    printf("[boot] consola/logs = USB-Serial-JTAG | wire v1 = UART0 @115200\n");

    fs_init();
    fs_register_bpvm();    /* #247 — file I/O desde BP sobre este FS */
    esp32_mods_install();  /* stdlib core embebida -> /lib (if-absent), como STM32/Pico */
    esp32_hw_register();   /* backends de HW (GPIO, pico/info) */
    wire_v1_uart_init();

    printf("[boot] REPL wire v1 escuchando en UART0. Conecta el IDE al puerto del bridge.\n");

    /* P-autorun (#256) — si /sys/auto.txt existe, arranca la app antes
     * del REPL. El wire ya está vivo y el poll del run atiende
     * HELLO/KILL: el IDE puede conectar y parar la app en cualquier
     * momento. */
    repl_esp32_autorun();
    repl_esp32_run();   /* no retorna */
}
