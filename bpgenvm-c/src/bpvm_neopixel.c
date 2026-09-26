/*
 * bpvm_neopixel.c — fachada NeoPixel para la VM C. Ver bpvm_neopixel.h.
 * El firmware registra el backend de su familia (la PIO en la Pico, el RMT en la ESP32).
 * En host (sin HW) no hay backend: el builtin lanza «no implementado en esta plataforma».
 */
#include "bpvm_neopixel.h"

static const bpvm_neopixel_backend_t* g_backend = NULL;

void bpvm_neopixel_set_backend(const bpvm_neopixel_backend_t* backend) {
    g_backend = backend;
}

int bpvm_neopixel_disponible(void) {
    return g_backend != NULL && g_backend->init != NULL;
}

int bpvm_neopixel_init(int pin, char* err, size_t n) {
    if (err && n) err[0] = '\0';
    if (g_backend && g_backend->init) {
        return g_backend->init(pin, err, n);
    }
    return 0;   /* host: no hay NeoPixel */
}

void bpvm_neopixel_show(int pin, const uint32_t* grb, int count) {
    if (g_backend && g_backend->show) {
        g_backend->show(pin, grb, count);
    }
}
