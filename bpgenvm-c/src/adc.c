/*
 * adc.c — fachada Adc para la VM C.
 *
 * V6/#469 — EL STUB QUE MENTIA. Hasta el 6-sep-2026 esta fachada, cuando NADIE
 * habia registrado backend, no fallaba: `initChannel` imprimia el pinout del
 * RP2350 («→ GP26») y `readChannel` devolvia una RAMPA — un numero que se mueve,
 * que parece una lectura y que es falso. En el host eso es legitimo y util (los
 * tests necesitan un valor determinista); en una PLACA es un olvido disfrazado
 * de medida. Y el STM32 no registra backend de ADC, asi que en una Nucleo o una
 * Discovery `Adc.read()` devolvia ese numero inventado.
 *
 * El arreglo NO es quitar el stub: es hacerlo EXPLICITO. Quien no tiene hardware
 * —el host y el micro simulado— registra `bpvm_adc_backend_host()` a proposito,
 * y a partir de ahi **no tener backend deja de ser un caso valido**: falla con
 * ruido y dice que placa se lo dejo sin registrar. Asi una familia NUEVA no
 * puede caer en el agujero por defecto, que era lo peor del diseno anterior.
 */

#include "bpvm_adc.h"
#include <stdio.h>
#include <stddef.h>

static const bpvm_adc_backend_t* g_backend = NULL;

/* Contador para que el stub del host devuelva valores distintos en cada
 * llamada — util para que los samples BP de prueba no se queden con un valor
 * estatico. La formula es la MISMA que miVM (VirtualMachine.java, ADC_*): si se
 * tocara una sin la otra, la paridad dual-VM se rompe. */
static int g_stub_counter = 0;

void bpvm_adc_set_backend(const bpvm_adc_backend_t* backend) {
    g_backend = backend;
}

/* --- el stub del host, ahora un backend como cualquier otro ---------------
 *
 * ⚠️ LOS TEXTOS SON CONTRATO DE PARIDAD: salen por stdout y tienen que ser
 * byte-identicos a los de miVM. Hasta hoy NO lo eran —aqui se escribia
 * «(stub)» y «ERROR — fuera de rango», y alli «(host)» y «fuera de rango»—,
 * asi que `samples/AdcDemo.bp` no era byte-identico entre las dos VMs. No lo
 * vio nadie porque ese sample no esta en el corpus del arnes. */
static int host_init_channel(int ch) {
    if (ch < 0 || ch > 3) {
        printf("[adc] initChannel(%d) fuera de rango\n", ch);
        return -1;
    }
    printf("[adc] initChannel(%d) → GP%d (host)\n", ch, 26 + ch);
    return 26 + ch;   /* CH0=GP26, CH1=GP27, ... */
}

static int host_read_channel(int ch) {
    if (ch < 0 || ch > 3) return -1;
    /* Rampa por canal: cada canal con su offset de 1024. Wrap a 4095. */
    int v = (g_stub_counter + ch * 1024) & 0x0FFF;
    g_stub_counter = (g_stub_counter + 73) & 0xFFFF;
    return v;
}

static const bpvm_adc_backend_t s_backend_host = {
    host_init_channel,
    host_read_channel,
};

const bpvm_adc_backend_t* bpvm_adc_backend_host(void) { return &s_backend_host; }

/* --- y la ausencia de backend, que ya NO devuelve un numero plausible ----- */
static int sin_backend(const char* que) {
    static int avisado = 0;
    if (!avisado) {
        avisado = 1;
        printf("[adc] %s: ESTA PLACA NO REGISTRA BACKEND DE ADC.\n"
               "      No es que la lectura sea 0: es que no hay de donde leerla.\n"
               "      Si la placa tiene ADC, le falta su bpvm_adc_set_backend() en el\n"
               "      arranque; si no lo tiene, dilo tambien — pero no se inventa un valor.\n",
               que);
    }
    return -1;
}

int bpvm_adc_init_channel(int ch) {
    if (g_backend && g_backend->initChannel) return g_backend->initChannel(ch);
    (void) ch;
    return sin_backend("initChannel");
}

int bpvm_adc_read_channel(int ch) {
    if (g_backend && g_backend->readChannel) return g_backend->readChannel(ch);
    (void) ch;
    return sin_backend("readChannel");
}
