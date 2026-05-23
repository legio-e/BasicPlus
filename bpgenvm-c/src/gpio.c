/*
 * gpio.c — fachada de los hooks GPIO para la VM C.
 *
 * Por defecto (sin backend registrado) usa stubs con logging por stdout
 * — mismo comportamiento que la VM Java en PC. El firmware del Pico
 * llama a bpvm_gpio_set_backend(&pico_gpio_backend) al boot para
 * redirigir al SDK.
 */

#include "bpvm_gpio.h"
#include <stdio.h>

static const bpvm_gpio_backend_t* g_backend = NULL;

void bpvm_gpio_set_backend(const bpvm_gpio_backend_t* backend) {
    g_backend = backend;
}

void bpvm_gpio_init(int pin, int mode) {
    if (g_backend && g_backend->init) {
        g_backend->init(pin, mode);
        return;
    }
    printf("[gpio] init pin=%d mode=%s\n", pin,
           mode == 0 ? "INPUT" : "OUTPUT");
}

void bpvm_gpio_pull(int pin, int pull_mode) {
    if (g_backend && g_backend->pull) {
        g_backend->pull(pin, pull_mode);
        return;
    }
    const char* m = (pull_mode == 0) ? "NONE"
                  : (pull_mode == 1) ? "UP"
                  : "DOWN";
    printf("[gpio] pull pin=%d mode=%s\n", pin, m);
}

void bpvm_gpio_write(int pin, int value) {
    if (g_backend && g_backend->write) {
        g_backend->write(pin, value);
        return;
    }
    printf("[gpio] write pin=%d value=%s\n", pin,
           value == 0 ? "LOW" : "HIGH");
}

int bpvm_gpio_read(int pin) {
    if (g_backend && g_backend->read) {
        return g_backend->read(pin);
    }
    printf("[gpio] read pin=%d (stub → 0)\n", pin);
    return 0;
}
