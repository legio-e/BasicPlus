/*
 * bpvm_neopixel.c — fachada NeoPixel para la VM C. Ver bpvm_neopixel.h.
 * En el firmware Pico, main.c registra el backend que llama a neopixel.c (PIO).
 * En host (sin HW) los stubs son no-op.
 */
#include "bpvm_neopixel.h"

static const bpvm_neopixel_backend_t* g_backend = NULL;

void bpvm_neopixel_set_backend(const bpvm_neopixel_backend_t* backend) {
    g_backend = backend;
}

int bpvm_neopixel_init(int pin) {
    if (g_backend && g_backend->init) {
        return g_backend->init(pin);
    }
    return 0;   /* host: no hay NeoPixel */
}

void bpvm_neopixel_show(int pin, const uint32_t* grb, int count) {
    if (g_backend && g_backend->show) {
        g_backend->show(pin, grb, count);
    }
}
