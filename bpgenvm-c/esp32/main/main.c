/*
 * main.c — H4.1: arranque mínimo de la VM BasicPlus en ESP32-S3 (Xtensa).
 *
 * Objetivo del hito: demostrar que el INTÉRPRETE (core VM portable, C99)
 * corre en una SEGUNDA ARQUITECTURA (Xtensa LX7, no ARM). Carga un
 * Hello.mod embebido en flash y lo ejecuta; la salida va por la consola
 * (UART0 → puerto USB-UART de la placa, visible con `idf.py monitor`).
 *
 * NO hay AOT aquí: el .mdn es ARM Thumb-2 y no cruza a Xtensa. El
 * intérprete ejecuta TODO el bytecode igualmente.
 *
 * app_main() ya corre dentro de una task FreeRTOS (la arranca el startup
 * de ESP-IDF), así que ejecutamos la VM directamente — sin xTaskCreate
 * ni vTaskStartScheduler manuales.
 */
#include <stdio.h>
#include <stdint.h>

#include "bpvm.h"

/* Hello.mod embebido (hello_mod.c, generado con xxd). */
extern const uint8_t      hello_mod[];
extern const unsigned int hello_mod_len;

/* Buffer caller-provided de la VM (modelo C0). El ESP32-S3 tiene ~512 KB
 * de SRAM interna; 128 KB en .bss va sobrado para Hello. */
#define VM_BUFFER_SIZE (128 * 1024)
static uint8_t s_vm_buffer[VM_BUFFER_SIZE];

void app_main(void)
{
    printf("\n");
    printf("=== BasicPlus VM en ESP32-S3 (H4.1) ===\n");
    printf("[boot] ESP-IDF arriba, app_main en marcha\n");
    printf("[boot] VM buffer: %u bytes\n", (unsigned) VM_BUFFER_SIZE);

    bpvm_t* vm = bpvm_init(s_vm_buffer, VM_BUFFER_SIZE, 0);
    if (!vm) {
        printf("[ERR] bpvm_init fallo\n");
        return;
    }

    bpvm_status_t s = bpvm_load_mod_buffer(vm, hello_mod, hello_mod_len, "Hello");
    if (s != BPVM_OK) {
        printf("[ERR] load_mod_buffer: %s\n", bpvm_status_str(s));
        return;
    }

    printf("[run] ---- salida de Hello.mod ----\n");
    s = bpvm_run(vm);
    printf("[run] ---- fin ----\n");
    printf("[done] status = %s\n", bpvm_status_str(s));

    /* app_main retorna → ESP-IDF deja el sistema en idle. El hito está
     * cumplido si la salida de Hello aparece por la consola. */
}
