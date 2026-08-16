/*
 * repl_esp32.h — REPL wire v1 del firmware ESP32-S3 (H4.3).
 *
 * Habla el wire BPVM v1 por UART0 (mismo protocolo que la Pico). El IDE
 * (SerialBackend) sube .mod (PUT → FS RAM), los ejecuta (RUN) y recibe
 * la salida del programa como eventos OUTPUT, más EXITED al terminar.
 */
#ifndef BPVM_ESP32_REPL_H
#define BPVM_ESP32_REPL_H

/* Esta cabecera no incluía nada y funcionaba porque sus declaraciones no usaban
 * tipos de ancho fijo. `repl_set_packs_loader` sí (V5/H7), y una cabecera que
 * depende de que el .c haya incluido antes lo que ella necesita es una trampa
 * para el siguiente — más aún aquí, donde el build de esta familia no se puede
 * comprobar desde el PC. */
#include <stdint.h>

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

/* Identidad de la placa para INFO/HELLO (solo informativo del wire). Por
 * defecto = ESP32-S3; una placa instala la suya con repl_set_board_id() ANTES
 * de arrancar el REPL (p.ej. el ESP32-P4 en p4_board_id.c). No tocar el default
 * = el binario del S3 no cambia. */
typedef struct {
    const char *board_name;    /* INFO.boardName    */
    const char *server_name;   /* HELLO.serverName  */
    long cpu_freq_hz;
    long gpio_count;
    long pio_count;
    long pwm_slices;
    long adc_channels;
    long sram_bytes;
} repl_board_id_t;

/* Fija la identidad reportada por INFO/HELLO. Idempotente; id NULL se ignora. */
void repl_set_board_id(const repl_board_id_t *id);

/* V5/H7 - quien carga el pack nativo, y se hace en el PRIMER `Run` (no al
 * arrancar: el salto es el unico paso que puede colgar, y un cuelgue en el
 * arranque obliga a regrabar). La familia que tenga pack nativo registra aqui
 * su cargador; el S3 no registra ninguno y no pasa nada.
 * Devuelve >=0 si salto (lo que devolvio el pack), <0 si no. */
void repl_set_packs_loader(int32_t (*fn)(void));

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ESP32_REPL_H */
