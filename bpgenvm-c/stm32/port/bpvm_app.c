/*
 * bpvm_app.c — arranque de la VM BasicPlus en el STM32U575 (H9.1.2).
 *
 * Es el equivalente STM32 del trío host (test/main.c + comm_host.c) reducido al
 * mínimo: provee un buffer de RAM estático, registra un output_cb que vuelca al
 * VCP, carga el Hello.mod embebido y llama a bpvm_run(). Single-thread, sin
 * comm-task ni FreeRTOS (ver emit_text en interp.c: con output_cb instalado y
 * sin SMP, la VM llama el callback DIRECTO).
 *
 * Paridad: el output_cb pasa los bytes de la VM TAL CUAL (sin traducir '\n' a
 * "\r\n"), así que el stdout es byte-idéntico a la VM-Java y a la VM-C host.
 * Los banners "=== ... ===" de aquí son decoración del firmware (no forman
 * parte de la salida del programa, igual que los banners de la VM-C host).
 */
#include "bpvm_app.h"
#include "bpvm.h"

#include "main.h"               /* HAL */
#include "board.h"              /* placa: BOARD_WIRE_UART */

#include <string.h>

/* Módulo embebido (hello_mod.c, generado con `xxd -i Hello.mod`). */
extern const unsigned char hello_mod[];
extern const unsigned int  hello_mod_len;

/*
 * RAM que la VM gestiona como su memory[] (heap BP + stacks BP). "Diseñar
 * para el piso": 128 KB en SRAM interna; el U575 tiene 768 KB, así que hay
 * holgura de sobra. Se puede subir si un programa lo pide.
 */
#define BPVM_MEM_SIZE (128u * 1024u)
static uint8_t s_vm_mem[BPVM_MEM_SIZE];

/* Salida del programa BP (cada PRINT_* de la VM) → VCP. Bytes verbatim. */
static void uart_output_cb(const char* s, size_t len, void* user) {
    (void) user;
    HAL_UART_Transmit(BOARD_WIRE_UART, (uint8_t*) s, (uint16_t) len, HAL_MAX_DELAY);
}

/* Texto propio del firmware (banners/diagnóstico), con CRLF para el terminal. */
static void fw_puts(const char* s) {
    HAL_UART_Transmit(BOARD_WIRE_UART, (uint8_t*) s, (uint16_t) strlen(s), HAL_MAX_DELAY);
}

void bpvm_app_run_hello(void) {
    fw_puts("\r\n=== BasicPlus VM en STM32U575 (H9.1) ===\r\n");

    bpvm_t* vm = bpvm_init(s_vm_mem, sizeof(s_vm_mem), 0);
    if (!vm) { fw_puts("[bpvm] init FALLO (heap libc insuficiente?)\r\n"); return; }

    bpvm_set_output(vm, uart_output_cb, NULL);

    bpvm_status_t s = bpvm_load_mod_buffer(vm, hello_mod, hello_mod_len, "Hello");
    if (s != BPVM_OK) {
        fw_puts("[bpvm] load FALLO: ");
        fw_puts(bpvm_status_str(s));
        fw_puts("\r\n");
        bpvm_destroy(vm);
        return;
    }

    s = bpvm_run(vm);

    fw_puts("\r\n=== FIN VM (status=");
    fw_puts(bpvm_status_str(s));
    fw_puts(") ===\r\n");

    bpvm_destroy(vm);
}
