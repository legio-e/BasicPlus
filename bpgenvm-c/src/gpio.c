/*
 * gpio.c — fachada de los hooks GPIO para la VM C.
 *
 * Por defecto (sin backend registrado) usa stubs con logging por stdout
 * — mismo comportamiento que la VM Java en PC. El firmware del Pico
 * llama a bpvm_gpio_set_backend(&pico_gpio_backend) al boot para
 * redirigir al SDK.
 */

#include "bpvm_out.h"
#include "bpvm_gpio.h"
#include <stdio.h>

static const bpvm_gpio_backend_t* g_backend = NULL;

/* ══════════ V7/H4 (#479) — el estado del pin y la validación (ver bpvm_gpio.h) ══════════ */
#include "bpvm_machine.h"
#include <stdio.h>
#include <string.h>
static uint8_t s_estado[BPVM_CAPS_PIN_MAX];

int  bpvm_gpio_estado(int pin) { return (pin >= 0 && pin < BPVM_CAPS_PIN_MAX) ? s_estado[pin] : 0; }
void bpvm_gpio_liberar(int pin) { if (pin >= 0 && pin < BPVM_CAPS_PIN_MAX) s_estado[pin] = 0; }
void bpvm_gpio_estado_reset(void) { memset(s_estado, 0, sizeof s_estado); }

int bpvm_gpio_reclamar(int pin, int uso, char* err, size_t cap) {
    const bpvm_caps_t* c = bpvm_caps();
    int m = bpvm_caps_pin_mask(pin);
    if (!m || pin >= BPVM_CAPS_PIN_MAX) { snprintf(err, cap, "el pin %d no existe en %s", pin, c->micro); return -1; }
    int e = s_estado[pin];
    if (uso == BPVM_USO_ANALOG) {
        if (!(m & BPVM_PIN_ANALOG)) { snprintf(err, cap, "el pin %d de %s no es analógico", pin, c->micro); return -1; }
        if (e != BPVM_GPIO_LIBRE && e != BPVM_GPIO_ANALOG) { snprintf(err, cap, "el pin %d está en uso digital: primero release()", pin); return -1; }
        s_estado[pin] = BPVM_GPIO_ANALOG;
        return 0;
    }
    if (e == BPVM_GPIO_ANALOG) { snprintf(err, cap, "el pin %d está en uso analógico: primero release()", pin); return -1; }
    if ((uso == BPVM_USO_OUTPUT || uso == BPVM_USO_OD || uso == BPVM_USO_PWM) && (m & BPVM_PIN_INPUT_ONLY)) {
        snprintf(err, cap, "el pin %d de %s es sólo de entrada", pin, c->micro); return -1;
    }
    if (uso == BPVM_USO_OD && !(m & BPVM_PIN_OPEN_DRAIN)) { snprintf(err, cap, "el pin %d de %s no puede salir en colector abierto", pin, c->micro); return -1; }
    if (uso == BPVM_USO_PWM && !(m & BPVM_PIN_PWM)) { snprintf(err, cap, "el pin %d de %s no puede dar PWM", pin, c->micro); return -1; }
    if (uso == BPVM_USO_PULSE && !(m & BPVM_PIN_PULSE)) {
        if (bpvm_caps_pins_con(BPVM_PIN_PULSE) == 0) snprintf(err, cap, "%s no tiene contador de pulsos", c->micro);
        else snprintf(err, cap, "el pin %d de %s no puede contar pulsos", pin, c->micro);
        return -1;
    }
    if (uso <= BPVM_USO_OD) s_estado[pin] = (uint8_t) (uso + 1);   /* PWM y PULSE no cambian el modo (hoy) */
    return 0;
}

int bpvm_gpio_instancia_ok(int kind, int id, char* err, size_t cap) {
    const bpvm_caps_t* c = bpvm_caps();
    const uint8_t* l = (kind == 0) ? c->uart : (kind == 1) ? c->i2c : c->spi;
    int n = (kind == 0) ? c->n_uart : (kind == 1) ? c->n_i2c : c->n_spi;
    if (bpvm_caps_instancia_ok(l, n, id)) return 0;
    char lst[48]; size_t o = 0; lst[0] = 0;
    for (int i = 0; i < n && o < sizeof lst - 5; i++) o += (size_t) snprintf(lst + o, sizeof lst - o, "%s%d", i ? ", " : "", (int) l[i]);
    snprintf(err, cap, "%s %d no existe en %s (válidos: %s)", (kind == 0) ? "UART" : (kind == 1) ? "I2C" : "SPI", id, c->micro, n ? lst : "ninguno");
    return -1;
}

void bpvm_gpio_set_backend(const bpvm_gpio_backend_t* backend) {
    g_backend = backend;
}

void bpvm_gpio_init(int pin, int mode) {
    if (g_backend && g_backend->init) {
        g_backend->init(pin, mode);
        return;
    }
    bpvm_out("[gpio] init pin=%d mode=%s\n", pin,
           mode == 0 ? "INPUT" : mode == 1 ? "OUTPUT" : mode == 2 ? "OPEN_COLLECTOR" : "ANALOG");
}

void bpvm_gpio_pull(int pin, int pull_mode) {
    if (g_backend && g_backend->pull) {
        g_backend->pull(pin, pull_mode);
        return;
    }
    const char* m = (pull_mode == 0) ? "NONE"
                  : (pull_mode == 1) ? "UP"
                  : "DOWN";
    bpvm_out("[gpio] pull pin=%d mode=%s\n", pin, m);
}

void bpvm_gpio_write(int pin, int value) {
    if (g_backend && g_backend->write) {
        g_backend->write(pin, value);
        return;
    }
    bpvm_out("[gpio] write pin=%d value=%s\n", pin,
           value == 0 ? "LOW" : "HIGH");
}

int bpvm_gpio_read(int pin) {
    if (g_backend && g_backend->read) {
        return g_backend->read(pin);
    }
    bpvm_out("[gpio] read pin=%d (sim → siempre 0 en PC)\n", pin);
    return 0;
}
