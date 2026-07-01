// main.c — Firmware de la VM BasicPlus para la 2ª placa ESP32-P4 (Waveshare 4.3").
//
// Construido INCREMENTAL: copiando trozos del firmware esp32p4/ uno a uno. El esp32p4/
// original queda intacto (known-good de la Function-EV).
//
// PASO 1 ✅ nucleo VM-C ejecutando Hello.mod embebido (Fibonacci).
// PASO 2 ✅ heap de la VM (2 MB) en PSRAM.
// PASO 3 ✅ FS persistente (particion "bpfs") + stdlib en /lib.
// PASO 4: WIRE v1 por UART0 (GPIO37/38 = COM15). El IDE sube/ejecuta apps (HELLO/PUT/RUN).
//         Consola NONE -> UART0 LIBRE para el wire; SIN Ethernet (la placa no lo tiene).
//         El arranque ya se verifico con consola en los pasos 0-3; ahora la señal de vida
//         es el IDE por el wire. Mismo wire_v1.c/repl_esp32.c que el S3 (pines 37/38 en P4).

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "fs.h"                       // FS (fs_init, fs_register_bpvm, fs_status_*, fs_file_count)
#include "repl_esp32.h"              // wire_v1_uart_init, repl_esp32_run/_autorun, repl_set_board_id
#include "hw_esp32.h"                // esp32_hw_register (backends GPIO/UART/SPI/I2C...)

// stdlib embebida -> /lib. Definida en ../../esp32/main/esp32_mods.c.
void esp32_mods_install(void);

static const char *TAG = "p4ws";

// memory[] de la VM: 2 MB en PSRAM. GLOBALES no-static: repl_esp32.c los referencia por extern.
#define VM_MEM_SIZE (2u * 1024u * 1024u)
uint8_t  *s_vm_buffer      = NULL;
uint32_t  s_vm_buffer_size = 0;

// Identidad de la placa para INFO/HELLO del wire (valores del P4; ajustables). Sin esto el
// wire reportaria el default "ESP32-S3".
static const repl_board_id_t s_board_id = {
    .board_name   = "ESP32-P4 Waveshare 4.3\"",
    .server_name  = "esp32p4-ws",
    .cpu_freq_hz  = 360000000,
    .gpio_count   = 55,
    .pio_count    = 0,
    .pwm_slices   = 0,
    .adc_channels = 8,
    .sram_bytes   = 768 * 1024,
};

static void vm_buffer_init_psram(void)
{
    s_vm_buffer = (uint8_t *) heap_caps_malloc(VM_MEM_SIZE, MALLOC_CAP_SPIRAM);
    if (s_vm_buffer == NULL) {
        ESP_LOGE(TAG, "PSRAM: no se pudo reservar el heap de la VM - abort");
        abort();
    }
    s_vm_buffer_size = VM_MEM_SIZE;
    ESP_LOGI(TAG, "VM heap en PSRAM: %u KiB @ %p",
             (unsigned)(VM_MEM_SIZE / 1024u), (void *) s_vm_buffer);
}

void app_main(void)
{
    // OJO: consola NONE en este build -> estos ESP_LOG NO salen por COM15 (UART0 = wire).
    ESP_LOGI(TAG, "==== BasicPlus VM :: ESP32-P4 Waveshare (PASO 4: wire UART0/COM15) ====");

    vm_buffer_init_psram();

    // FS persistente + stdlib en /lib.
    fs_status_t r = fs_init();
    fs_register_bpvm();
    esp32_mods_install();
    ESP_LOGI(TAG, "FS+stdlib: %s, %d ficheros", fs_status_str(r), fs_file_count());

    // Backends HW + identidad de la placa (para INFO/HELLO).
    esp32_hw_register();
    repl_set_board_id(&s_board_id);

    // Wire v1 por UART0 (COM15): el IDE conecta, sube (.mod -> FS) y ejecuta. No retorna.
    wire_v1_uart_init();
    repl_esp32_autorun();          // /sys/auto.txt si existiera (aun no hay apps)
    repl_esp32_run();              // bucle del wire; NO retorna
}
