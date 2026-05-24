/*
 * pwm.c — fachada de hooks PWM para la VM C.
 *
 * Mismo patrón que pulse.c / spi.c: si no hay backend registrado,
 * imprime por stdout (útil en host para inspección sin HW). En el
 * firmware Pico, main.c registra el backend que envuelve
 * pwm_init / pwm_set_clkdiv / pwm_set_wrap / pwm_set_chan_level /
 * pwm_set_enabled del SDK del Pico.
 */

#include "bpvm_pwm.h"
#include <stdio.h>

static const bpvm_pwm_backend_t* g_backend = NULL;

void bpvm_pwm_set_backend(const bpvm_pwm_backend_t* backend) {
    g_backend = backend;
}

int bpvm_pwm_init(int pin, int freqHz) {
    if (g_backend && g_backend->init) {
        return g_backend->init(pin, freqHz);
    }
    printf("[pwm] init pin=%d freqHz=%d (stub → sliceId=0)\n", pin, freqHz);
    return 0;
}

void bpvm_pwm_set_freq(int sliceId, int freqHz) {
    if (g_backend && g_backend->setFreq) {
        g_backend->setFreq(sliceId, freqHz);
        return;
    }
    printf("[pwm] setFreq slice=%d freqHz=%d (stub)\n", sliceId, freqHz);
}

void bpvm_pwm_set_duty(int sliceId, int pin, int dutyPct) {
    if (g_backend && g_backend->setDuty) {
        g_backend->setDuty(sliceId, pin, dutyPct);
        return;
    }
    printf("[pwm] setDuty slice=%d pin=%d duty=%d%% (stub)\n",
           sliceId, pin, dutyPct);
}

void bpvm_pwm_start(int sliceId) {
    if (g_backend && g_backend->start) {
        g_backend->start(sliceId);
        return;
    }
    printf("[pwm] start slice=%d (stub)\n", sliceId);
}

void bpvm_pwm_stop(int sliceId) {
    if (g_backend && g_backend->stop) {
        g_backend->stop(sliceId);
        return;
    }
    printf("[pwm] stop slice=%d (stub)\n", sliceId);
}
