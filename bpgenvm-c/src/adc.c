/*
 * adc.c — fachada Adc para la VM C.
 *
 * Stub portable: en host sin backend, devolvemos una rampa
 * determinista por canal (útil para tests). Si hay backend
 * (Pico), delegamos. Mismo patrón que pico.c / rtc.c.
 */

#include "bpvm_adc.h"
#include <stdio.h>
#include <stddef.h>

static const bpvm_adc_backend_t* g_backend = NULL;

/* Contador para que el stub devuelva valores distintos en cada
 * llamada — útil para que samples BP de prueba no se queden con
 * un valor estático en host. */
static int g_stub_counter = 0;

void bpvm_adc_set_backend(const bpvm_adc_backend_t* backend) {
    g_backend = backend;
}

int bpvm_adc_init_channel(int ch) {
    if (g_backend && g_backend->initChannel) {
        return g_backend->initChannel(ch);
    }
    if (ch < 0 || ch > 3) {
        printf("[adc] initChannel(%d) ERROR — fuera de rango\n", ch);
        return -1;
    }
    printf("[adc] initChannel(%d) → GP%d (stub)\n", ch, 26 + ch);
    return 26 + ch;   /* CH0=GP26, CH1=GP27, ... */
}

int bpvm_adc_read_channel(int ch) {
    if (g_backend && g_backend->readChannel) {
        return g_backend->readChannel(ch);
    }
    if (ch < 0 || ch > 3) return -1;
    /* Rampa por canal: cada canal tiene su propio counter
     * desfasado en 1024 unidades. Wrap a 4095. */
    int v = (g_stub_counter + ch * 1024) & 0x0FFF;
    g_stub_counter = (g_stub_counter + 73) & 0xFFFF;
    return v;
}
