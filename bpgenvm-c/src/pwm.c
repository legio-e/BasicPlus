/*
 * pwm.c — fachada de hooks PWM para la VM C.
 *
 * Mismo patrón que pulse.c / spi.c: si no hay backend registrado,
 * imprime por stdout (útil en host para inspección sin HW). En el
 * firmware Pico, main.c registra el backend que envuelve
 * pwm_init / pwm_set_clkdiv / pwm_set_wrap / pwm_set_chan_level /
 * pwm_set_enabled del SDK del Pico.
 */

#include "bpvm_out.h"
#include "bpvm_pwm.h"
#include "bpvm.h"   /* V7 #667: bpvm_diag (la red) */
#include <stdio.h>

static const bpvm_pwm_backend_t* g_backend = NULL;

void bpvm_pwm_set_backend(const bpvm_pwm_backend_t* backend) {
    g_backend = backend;
}

int bpvm_pwm_init(int pin, int freqHz) {
    if (g_backend && g_backend->init) {
        return g_backend->init(pin, freqHz);
    }
    bpvm_out("[pwm] init pin=%d freqHz=%d (sim → sliceId=0)\n", pin, freqHz);
    return 0;
}

void bpvm_pwm_set_freq(int sliceId, int freqHz) {
    if (g_backend && g_backend->setFreq) {
        g_backend->setFreq(sliceId, freqHz);
        return;
    }
    bpvm_out("[pwm] setFreq slice=%d freqHz=%d (sim)\n", sliceId, freqHz);
}

void bpvm_pwm_set_duty(int sliceId, int pin, int dutyPct) {
    if (g_backend && g_backend->setDuty) {
        g_backend->setDuty(sliceId, pin, dutyPct);
        return;
    }
    bpvm_out("[pwm] setDuty slice=%d pin=%d duty=%d%% (sim)\n",
           sliceId, pin, dutyPct);
}

void bpvm_pwm_start(int sliceId) {
    if (g_backend && g_backend->start) {
        g_backend->start(sliceId);
        return;
    }
    bpvm_out("[pwm] start slice=%d (sim)\n", sliceId);
}

void bpvm_pwm_stop(int sliceId) {
    if (g_backend && g_backend->stop) {
        g_backend->stop(sliceId);
        return;
    }
    bpvm_out("[pwm] stop slice=%d (sim)\n", sliceId);
}

/* V7 #667 — soltarlo (el destructor de Pwm.Slice). En el PC, como los demás, se dice por la salida. */
void bpvm_pwm_release(int sliceId, int pin) {
    if (g_backend && g_backend->release) {
        g_backend->release(sliceId, pin);
        return;
    }
    bpvm_out("[pwm] release slice=%d pin=%d (sim)\n", sliceId, pin);
}

/* V7 #667 — la red del final del RUN: sólo en una placa (en el PC no hay nada en marcha, y no se escribe nada). */
void bpvm_pwm_fin_de_run(void) {
    int n = (g_backend && g_backend->fin_de_run) ? g_backend->fin_de_run() : 0;
    if (n > 0) bpvm_diag("[bpvm] fin de RUN: %d PWM seguian en marcha: se paran y sus pines vuelven al reset (#667)", n);
}
