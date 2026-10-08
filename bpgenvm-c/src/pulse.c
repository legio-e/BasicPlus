/*
 * pulse.c — fachada de hooks Pulse para la VM C.
 *
 * Mismo patrón que i2c.c / spi.c / uart.c: si no hay backend
 * registrado, simulamos por software un contador para que el host
 * (sin HW PWM) pueda al menos verificar la cadena bpvm → builtin →
 * sample. El stub mantiene un único contador en memoria que se
 * incrementa "a posteriori": no detecta flancos reales (no hay GPIO
 * en host) pero exponemos un counterId estable y un value() que es
 * 0 hasta que el backend real lo llene.
 *
 * Cuando el firmware Pico arranca, main.c llama a
 * bpvm_pulse_set_backend() con la tabla que envuelve las funciones
 * pwm_set_clkdiv_mode / pwm_get_counter / pwm_set_enabled del SDK.
 */

#include "bpvm_out.h"
#include "bpvm.h"   /* V7 #667: bpvm_diag (la red) */
#include "bpvm_pulse.h"
#include <stdio.h>

static const bpvm_pulse_backend_t* g_backend = NULL;

/* Estado del stub (solo se usa cuando no hay backend real). */
static int s_stub_value = 0;

void bpvm_pulse_set_backend(const bpvm_pulse_backend_t* backend) {
    g_backend = backend;
}

int bpvm_pulse_init(int pin, int edgeKind) {
    if (g_backend && g_backend->init) {
        return g_backend->init(pin, edgeKind);
    }
    const char* edgeName = edgeKind == 0 ? "RISING"
                         : edgeKind == 1 ? "FALLING"
                         : edgeKind == 2 ? "BOTH" : "?";
    bpvm_out("[pulse] init pin=%d edge=%s (sim → counterId=0)\n",
           pin, edgeName);
    s_stub_value = 0;
    return 0;   /* counterId 0 — solo soportamos uno en stub */
}

void bpvm_pulse_start(int counterId) {
    if (g_backend && g_backend->start) {
        g_backend->start(counterId);
        return;
    }
    bpvm_out("[pulse] start id=%d (sim)\n", counterId);
}

void bpvm_pulse_stop(int counterId) {
    if (g_backend && g_backend->stop) {
        g_backend->stop(counterId);
        return;
    }
    bpvm_out("[pulse] stop id=%d (sim, value=%d)\n", counterId, s_stub_value);
}

int bpvm_pulse_value(int counterId) {
    if (g_backend && g_backend->value) {
        return g_backend->value(counterId);
    }
    bpvm_out("[pulse] value id=%d (sim → %d)\n", counterId, s_stub_value);
    return s_stub_value;
}

/* V7 #667 — soltarlo (el destructor de Pulse.Counter). En el PC, como los demás, se dice por la salida. */
void bpvm_pulse_release(int counterId, int pin) {
    if (g_backend && g_backend->release) {
        g_backend->release(counterId, pin);
        return;
    }
    bpvm_out("[pulse] release id=%d pin=%d (sim)\n", counterId, pin);
}

/* V7 #667 — la red del final del RUN: sólo en una placa. */
void bpvm_pulse_fin_de_run(void) {
    int n = (g_backend && g_backend->fin_de_run) ? g_backend->fin_de_run() : 0;
    if (n > 0) bpvm_diag("[bpvm] fin de RUN: %d contador(es) seguian contando: se paran y sus pines vuelven al reset (#667)", n);
}

void bpvm_pulse_reset(int counterId) {
    if (g_backend && g_backend->reset) {
        g_backend->reset(counterId);
        return;
    }
    bpvm_out("[pulse] reset id=%d (sim)\n", counterId);
    s_stub_value = 0;
}
