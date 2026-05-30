/*
 * repl_esp32.h — REPL wire v1 del firmware ESP32-S3 (H4.3).
 *
 * Habla el wire BPVM v1 por UART0 (mismo protocolo que la Pico). El IDE
 * (SerialBackend) sube .mod (PUT → FS RAM), los ejecuta (RUN) y recibe
 * la salida del programa como eventos OUTPUT, más EXITED al terminar.
 */
#ifndef BPVM_ESP32_REPL_H
#define BPVM_ESP32_REPL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el driver UART0 para el wire (lo provee wire_v1.c). */
void wire_v1_uart_init(void);

/* Bucle principal del REPL: lee mensajes v1 de UART0 y los despacha.
 * No retorna. Llamar tras fs_init() + wire_v1_uart_init(). */
void repl_esp32_run(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ESP32_REPL_H */
