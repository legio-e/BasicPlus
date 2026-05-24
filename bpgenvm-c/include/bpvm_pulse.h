/*
 * bpvm_pulse.h — hooks de plataforma para los builtins Pulse.*
 *
 * Permite contar pulsos hardware en un pin de entrada sin coste de
 * CPU. En el RP2350 esto lo da el slice PWM correspondiente al pin,
 * configurado en modo "input gate edge counting": el contador del
 * slice avanza con cada flanco del pin B en lugar de con el reloj
 * del sistema.
 *
 * Mismo patrón que bpvm_{gpio,i2c,spi,uart}.h. Por defecto stubs con
 * logging por stdout (útil en host para desarrollar sin HW). En el
 * firmware Pico, main.c registra un backend cuyas funciones llaman a
 * pwm_set_clkdiv_mode, pwm_set_counter, pwm_get_counter, etc., del
 * SDK del Pico.
 *
 * El módulo Pulse.bp expone una clase `Pulse.Counter` por encima:
 *
 *   var c: Pulse.Counter := Pulse.Counter(11, Pulse.RISING)
 *   c.start()
 *   ... generar pulsos ...
 *   var n: integer := c.value()
 *   c.stop()
 *
 * Convenciones de los hooks:
 *
 *   counterId  Identificador opaco del contador para que el caller
 *              guarde su referencia. En la Pico C el counterId es
 *              el slice number (0..11). En host stub se ignora.
 *
 *   pin        GPIO del pin de entrada. Debe ser un pin que mapee
 *              al canal B del slice PWM (impares: GP1, GP3, GP5, …).
 *              Si no lo es, init() devuelve -1.
 *
 *   edgeKind   0 = RISING (sube),
 *              1 = FALLING (baja),
 *              2 = BOTH (ambos flancos).
 *              Se traduce a PWM_DIV_B_RISING / FALLING / HIGH del SDK.
 *
 *   init devuelve el counterId asignado (>= 0) o -1 si el pin no es
 *   válido para contar (no es pin B de ningún slice, ya en uso, etc.).
 *
 *   start arranca el conteo. value() devuelve el contador actual.
 *   reset() pone el contador a cero. stop() suelta el slice para
 *   que pueda usarse en modo PWM normal de nuevo.
 */
#ifndef BPVM_PULSE_H
#define BPVM_PULSE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Devuelve counterId (>= 0) o -1 si el pin no es válido. */
    int (*init)(int pin, int edgeKind);
    void (*start)(int counterId);
    void (*stop)(int counterId);
    /* Valor actual del contador (típicamente 0..65535 — el contador
     * del PWM en el RP2350 es de 16 bits). Si lo necesitamos extender
     * para más cuentas, el wrapper BP suma con un offset. */
    int  (*value)(int counterId);
    void (*reset)(int counterId);
} bpvm_pulse_backend_t;

void bpvm_pulse_set_backend(const bpvm_pulse_backend_t* backend);

/* Funciones efectivas. Stubs con logging si backend NULL. */
int  bpvm_pulse_init(int pin, int edgeKind);
void bpvm_pulse_start(int counterId);
void bpvm_pulse_stop(int counterId);
int  bpvm_pulse_value(int counterId);
void bpvm_pulse_reset(int counterId);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PULSE_H */
