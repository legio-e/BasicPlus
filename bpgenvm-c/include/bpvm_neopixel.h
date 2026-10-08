/*
 * bpvm_neopixel.h — H7.4.b: fachada NeoPixel (WS2812) para la VM C.
 *
 * Mismo patrón que bpvm_gpio/bpvm_pico: el firmware registra un backend que
 * conecta con su driver —la PIO en el RP2350 (pico/neopixel.c), el RMT en la
 * familia ESP32 (esp32/common/gpio_esp32.c, V7/H4)—; en host (sin HW) no hay
 * backend. El builtin NP_SHOW lee el array GRB del heap y lo pasa aquí.
 */
#ifndef BPVM_NEOPIXEL_H
#define BPVM_NEOPIXEL_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    /* 1 OK; 0 fallo, con el MOTIVO en err (V7/H4, 26-sep: «sin canal libre» no es «no implementado»).
     * Idempotente por pin; un segundo init del mismo pin puede rehacer lo suyo. */
    int  (*init)(int pin, char* err, size_t n);
    void (*show)(int pin, const uint32_t* grb, int count);   /* empuja count palabras GRB; BLOQUEANTE */
    /* V7 #667 — SOLTAR la tira del pin (el destructor de Neopixel.Strip, que antes la apaga): la máquina o el canal
     * quedan libres y el pin vuelve a como estaba en el reset. Un pin sin tira: nada. Y la red del final del RUN. */
    void (*release)(int pin);
    int  (*fin_de_run)(void);   /* cuántas soltó: lo dice la fachada */
} bpvm_neopixel_backend_t;

void bpvm_neopixel_set_backend(const bpvm_neopixel_backend_t* backend);

/* V7/H4 — 1 si esta imagen tiene driver de NeoPixel. Sin él, el builtin dice «no implementado en esta
 * plataforma» (el texto de miVM): así el PC y la placa sin driver hablan igual. */
int  bpvm_neopixel_disponible(void);

/* Funciones efectivas. init: 1 OK, 0 fallo con el motivo en err (sin backend: 0 y err vacío). */
int  bpvm_neopixel_init(int pin, char* err, size_t n);
void bpvm_neopixel_show(int pin, const uint32_t* grb, int count);
void bpvm_neopixel_release(int pin);   /* V7 #667 */
void bpvm_neopixel_fin_de_run(void);   /* V7 #667: la red del final del RUN */

#endif /* BPVM_NEOPIXEL_H */
