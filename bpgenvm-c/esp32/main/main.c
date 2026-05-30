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

/* Buffer caller-provided de la VM. NO static — repl_esp32.c lo referencia
 * como extern (igual convención que repl_v1.c en la Pico). */
#define VM_BUFFER_SIZE (128 * 1024)
uint8_t        s_vm_buffer[VM_BUFFER_SIZE];
const uint32_t s_vm_buffer_size = VM_BUFFER_SIZE;

void app_main(void)
{
    /* Estos printf van a la CONSOLA = USB-Serial-JTAG (puerto nativo),
     * NO al wire (UART0). Sirven para depurar el arranque. */
    printf("\n=== BasicPlus VM en ESP32-S3 (H4.3 — wire v1) ===\n");
    printf("[boot] consola/logs = USB-Serial-JTAG | wire v1 = UART0 @115200\n");

    fs_init();
    esp32_hw_register();   /* backends de HW (GPIO, …) */
    wire_v1_uart_init();

    printf("[boot] REPL wire v1 escuchando en UART0. Conecta el IDE al puerto del bridge.\n");

    repl_esp32_run();   /* no retorna */
}
