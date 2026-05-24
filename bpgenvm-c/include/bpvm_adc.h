/*
 * bpvm_adc.h — hooks de plataforma para los builtins Adc.*
 *
 * El RP2350 tiene 4 canales ADC externos:
 *   CH0 → GP26, CH1 → GP27, CH2 → GP28, CH3 → GP29
 * Más un CH4 que es el sensor de temperatura interno (NO se expone
 * aquí — para eso ya existe Pico.tempC() que hace la conversión a
 * grados Celsius).
 *
 * Resolución 12-bit (0..4095), referencia Vref = 3.3 V típico.
 *
 *   initChannel(ch): configura el GPIO correspondiente al ch en
 *                    modo ADC. Devuelve el número de pin físico
 *                    (26..29) si OK, o -1 si ch fuera de rango.
 *   readChannel(ch): lee la conversión más reciente. Devuelve el
 *                    raw 0..4095 (BP convierte a volts con
 *                    raw * 3.3 / 4095).
 *
 * En host sin HW el stub devuelve un valor en rampa para que
 * código BP pueda probarse sin Pico.
 */
#ifndef BPVM_ADC_H
#define BPVM_ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int (*initChannel)(int ch);
    int (*readChannel)(int ch);
} bpvm_adc_backend_t;

void bpvm_adc_set_backend(const bpvm_adc_backend_t* backend);

int bpvm_adc_init_channel(int ch);
int bpvm_adc_read_channel(int ch);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ADC_H */
