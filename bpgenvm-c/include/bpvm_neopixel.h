/*
 * bpvm_neopixel.h — H7.4.b: fachada NeoPixel (WS2812) para la VM C.
 *
 * Mismo patrón que bpvm_gpio/bpvm_pico: el firmware registra un backend que
 * conecta con el driver PIO (neopixel.c); en host (sin HW) los stubs son no-op.
 * El builtin NEOPIXEL_SHOW lee el array GRB del heap y lo pasa aquí.
 */
#ifndef BPVM_NEOPIXEL_H
#define BPVM_NEOPIXEL_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    int  (*init)(int pin);                                   /* 1 OK, 0 fallo */
    void (*show)(int pin, const uint32_t* grb, int count);   /* empuja count palabras GRB */
} bpvm_neopixel_backend_t;

void bpvm_neopixel_set_backend(const bpvm_neopixel_backend_t* backend);

/* Funciones efectivas (no-op en host sin backend). */
int  bpvm_neopixel_init(int pin);
void bpvm_neopixel_show(int pin, const uint32_t* grb, int count);

#endif /* BPVM_NEOPIXEL_H */
