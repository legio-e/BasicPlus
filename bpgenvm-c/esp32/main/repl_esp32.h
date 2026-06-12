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

/* P-autorun (#256) — ejecuta el módulo de /sys/auto.txt (si existe)
 * antes de entrar al REPL. Bloquea hasta que el programa termina o lo
 * matan por el wire (el poll atiende HELLO/KILL durante el run).
 * Llamar UNA vez, tras wire_v1_uart_init() y antes de repl_esp32_run. */
void repl_esp32_autorun(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ESP32_REPL_H */
