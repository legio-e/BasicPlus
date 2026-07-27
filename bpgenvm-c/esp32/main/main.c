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
#include "board_mgr_esp32.h"   /* H9: arranque escalonado + estado del boot */
#include "log.h"               /* log persistente (post-mortem) — lo antes posible */
#include "esp_heap_caps.h"     /* H11: el heap de la VM se pide en runtime, no es .bss */

/* Buffer caller-provided de la VM. repl_esp32.c lo referencia como extern
 * (PUNTERO, igual convención que repl_v1.c en la Pico/Metro y que el P4).
 *
 * H11 — deja de ser 128 KB de .bss. Era un array estático, o sea 128 KB de
 * dram0_0_seg RESERVADOS a piñón en el enlace, se usara la VM o no. En el S3 eso
 * no es teoría: con 128 KB aquí + 64 KB del espejo del FS + 48 KB del PUT, el
 * enlace iba tan justo que al añadir el log se pasó del segmento por 304 bytes.
 * Ahora se pide al heap en el arranque, que es lo que ya hacía el P4 con la
 * PSRAM: mismos bytes en marcha, pero el enlazador deja de tener que encajar un
 * bloque contiguo enorme y, si no hay sitio, el boot lo DICE (se queda por
 * debajo del estado 3 con su motivo) en vez de fallar al enlazar. */
#define VM_BUFFER_SIZE (128 * 1024)
uint8_t*       s_vm_buffer      = NULL;
uint32_t       s_vm_buffer_size = 0;

static void vm_buffer_init(void) {
    /* SRAM interna a propósito: el S3 de referencia no lleva PSRAM y el heap de
     * la VM tiene que ser rápido. Si el módulo trae PSRAM, ampliarlo aquí es un
     * cambio local (el P4 ya lo hace en su main.c). */
    s_vm_buffer = heap_caps_malloc(VM_BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_vm_buffer_size = s_vm_buffer ? VM_BUFFER_SIZE : 0;
    if (!s_vm_buffer) {
        /* No es fatal: el kernel sigue vivo y el host puede hablar con la placa
         * (H9). Lo que no habrá es VM — y el climb lo reportará. */
        printf("[boot] AVISO: sin RAM para el heap de la VM (%d KB)\n", VM_BUFFER_SIZE / 1024);
        log_printf("boot: heap de la VM NO reservado (%d KB) — sin VM", VM_BUFFER_SIZE / 1024);
    }
}

void app_main(void)
{
    /* Estos printf van a la CONSOLA = USB-Serial-JTAG (puerto nativo),
     * NO al wire (UART0). Sirven para depurar el arranque. */
    printf("\n=== BasicPlus VM en ESP32-S3 (H4.3 — wire v1) ===\n");
    printf("[boot] consola/logs = USB-Serial-JTAG | wire v1 = UART0 @115200\n");

    /* LO PRIMERO: el log persistente. Recupera el snapshot de la sesión anterior
     * (post-mortem: si el arranque previo se fue al garete, aquí está escrito) y
     * queda grabando desde antes del climb del boot. */
    log_init();
    log_printf("=== boot ESP32-S3 ===");

    /* El heap de la VM, ANTES del climb: layer_app comprueba s_vm_buffer. */
    vm_buffer_init();

    /* H9 — arranque escalonado: identidad → particiones del env → FS → VM. Sin
     * particiones/FS el climb se queda abajo y el host conduce (nada se
     * auto-inicializa); stdlib solo con el FS montado. */
    board_mgr_esp32_boot();
    if (board_boot_status()->state >= BPVM_BOOT_FS) {
        fs_register_bpvm();    /* #247 — file I/O desde BP sobre este FS */
        esp32_mods_install();  /* stdlib core embebida -> /lib (if-absent) */
    }
    esp32_hw_register();   /* backends de HW (GPIO, pico/info) — siempre */
    wire_v1_uart_init();

    printf("[boot] REPL wire v1 escuchando en UART0. Conecta el IDE al puerto del bridge.\n");

    /* P-autorun (#256) — si /sys/auto.txt existe, arranca la app antes
     * del REPL. El wire ya está vivo y el poll del run atiende
     * HELLO/KILL: el IDE puede conectar y parar la app en cualquier
     * momento. */
    if (board_boot_status()->state == BPVM_BOOT_APP && !board_boot_status()->degraded)
        repl_esp32_autorun();   /* H9: autorun solo con la placa sana en estado 3 */
    repl_esp32_run();   /* no retorna */
}
