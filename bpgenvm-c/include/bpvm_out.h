/*
 * bpvm_out.h — la traza de las FACHADAS sale por el mismo sitio que el `print`.
 *
 * V6/#478 — POR QUE EXISTE. Las fachadas de periferico (adc, gpio, i2c, spi,
 * uart, pwm, pulse, wdt, pico) escribian con `printf` DIRECTO a stdout, mientras
 * que la salida del programa BP va por `emit_text` (cola del hilo `io`, cola SMP,
 * callback, o fwrite). Dos caminos distintos, con dos buffers distintos, y eso
 * tenia dos consecuencias medidas:
 *
 *   1. EN EL HOST descoloca el orden contra el `print` del programa — hasta
 *      partir una linea por la mitad. Por eso `samples/BusBug.bp` y
 *      `samples/AdcDemo.bp` no salian byte-identicos entre las dos VMs aunque el
 *      CONTENIDO ya coincidiera: fallaba el orden, no el texto.
 *   2. EN UNA PLACA esos mensajes NO VIAJAN POR EL WIRE. El `printf` va a la
 *      consola de la placa, no a la cola de salida — asi que el IDE no los ve
 *      nunca. El aviso nuevo de #469 («esta placa no registra backend de ADC»)
 *      se perdia exactamente asi.
 *
 * COMO. `bpvm_out` formatea y entrega a `emit_text`, el mismo helper que usa el
 * `print` de BasicPlus. Sin VM activa (herramientas de host que enlazan una
 * fachada suelta) cae a stdout, que es lo que hacia antes.
 *
 * ⚠️ NO CONFUNDIR CON `bpvm_diag` (bpvm_util.c, #355), que ya existia y es OTRO canal:
 * aquel es DIAGNOSTICO —va a stderr o al log persistente de la placa, con sink
 * enchufable y volcado urgente— y este es SALIDA DEL PROGRAMA, ordenada con el
 * `print` y sujeta al contrato de paridad byte-identica. Que el enlazador me
 * pillara el choque de nombres fue la pista de que son dos cosas distintas.
 *
 * LA VM ACTIVA es un global a proposito: el modelo del proyecto es UNA app por
 * VM, y las fachadas no tienen —ni deben tener— un puntero a la VM. Mismo patron
 * que `s_dbg_vm` en `bpvm_dbg_wire.c`.
 */
#ifndef BPVM_OUT_H
#define BPVM_OUT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bpvm;

/** Traza de fachada: sale por el MISMO camino que el `print` del programa. */
void bpvm_out(const char* fmt, ...);

/** La fija `bpvm_init` y la borra `bpvm_destroy`. No la llames desde otro sitio. */
void bpvm_out_set_vm(struct bpvm* vm);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_OUT_H */
