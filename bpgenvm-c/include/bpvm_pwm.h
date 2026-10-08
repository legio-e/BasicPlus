/*
 * bpvm_pwm.h — hooks de plataforma para los builtins Pwm.*
 *
 * Cada GPIO está fijado por hardware a un slice + canal del PWM.
 * En el RP2350: pares=canal A, impares=canal B. Como salida PWM
 * sirven los dos canales independientemente, pero cada slice tiene
 * UN solo divisor de reloj y wrap — así que los dos canales del
 * mismo slice comparten frecuencia (solo difieren en duty).
 *
 * Convenciones de los hooks:
 *
 *   slicerId  Identificador opaco. En la Pico C es el slice number
 *             (0..11). En host stub, irrelevante.
 *
 *   pin       GPIO de salida. Determina el slice (cualquier canal
 *             sirve, no como Pulse que pedía canal B).
 *
 *   freqHz    Frecuencia objetivo en Hz. El backend calcula clkdiv
 *             y wrap para aproximarse lo más posible (limitaciones
 *             del HW: clkdiv 1..256, wrap 0..65535, f_sys=150MHz en
 *             RP2350).
 *
 *   dutyPct   0..100. 0 = pin siempre LOW, 100 = pin siempre HIGH.
 *
 *   init devuelve sliceId (>= 0) o -1 si el pin no es válido. Una
 *   vez devuelto el id, las demás funciones operan sobre él.
 */
#ifndef BPVM_PWM_H
#define BPVM_PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int  (*init)(int pin, int freqHz);
    void (*setFreq)(int sliceId, int freqHz);
    void (*setDuty)(int sliceId, int pin, int dutyPct);
    void (*start)(int sliceId);
    void (*stop)(int sliceId);
    /* V7 #667 — SOLTARLO (el destructor de Pwm.Slice): lo para, el canal queda libre para otro y el pin vuelve a como
     * estaba en el reset. Por slice Y pin: los dos canales de un slice pueden ser de dos Slice. */
    void (*release)(int sliceId, int pin);
    /* V7 #667 — al acabar el RUN, lo que el programa dejó en marcha y nadie soltó (un programa que usa las funciones
     * del módulo sin objetos, o una destrucción cortada): la red. Lo que suelta, lo dice. */
    int  (*fin_de_run)(void);   /* cuántas soltó: lo dice la fachada */
} bpvm_pwm_backend_t;

void bpvm_pwm_set_backend(const bpvm_pwm_backend_t* backend);

/* Funciones efectivas. Stubs con logging si backend NULL. */
int  bpvm_pwm_init(int pin, int freqHz);
void bpvm_pwm_set_freq(int sliceId, int freqHz);
void bpvm_pwm_set_duty(int sliceId, int pin, int dutyPct);
void bpvm_pwm_start(int sliceId);
void bpvm_pwm_stop(int sliceId);
void bpvm_pwm_release(int sliceId, int pin);   /* V7 #667 */
void bpvm_pwm_fin_de_run(void);                /* V7 #667: la red del final del RUN (silenciosa en el PC) */

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PWM_H */
