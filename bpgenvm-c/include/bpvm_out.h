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

/** V7/H5 (#558, 25-sep) — `len` bytes a la salida del programa por EL camino del `print` del intérprete: la
 *  cola del hilo `io` si está en marcha, la de SMP, o el callback. Lo usa quien escribe en nombre del programa
 *  desde C —los helpers del AOT (`print` desde una `native`)—: por el callback directo ADELANTABAN a lo que ya
 *  esperaba en la cola, y en placa el orden de la salida no era el del programa. */
void bpvm_emit(struct bpvm* vm, const char* s, size_t len);

/* ─── V7/H7 #600 (28-sep) — UN `print` SALE ENTERO ─────────────────────────────────────────────────────
 * `print "Resultado=", r` son varias instrucciones: una por pieza (la cadena, el separador, el número: los
 * *_NONL y PRINT_CHAR) y el salto (PRINT_NL). Salían pieza a pieza, y entre dos cabía la línea de otro hilo —con
 * dos workers en cualquier momento; con uno, en el cambio de turno—. Eduardo: «si haces print a; print b entre
 * los dos se puede colgar un print de otro hilo, eso es correcto. Ahora si haces print "Resultado=", r que se
 * cuele algo en medio no es correcto». Así que las piezas se guardan en la LÍNEA DEL HILO (bpvm_thread_t.linea)
 * y la línea sale con su salto de UN empujón. Lo que sale es lo mismo que antes; cambia cuándo. */

/** Un trozo de lo que sale de una vez. */
typedef struct { const char* p; size_t n; } bpvm_trozo_t;

/** Tope de la línea guardada de un hilo: lo que cabe en la cola de salida de la Pico (1 KB), la más pequeña.
 *  Una línea más larga ya no puede salir entera: sale por partes, como antes de #600. */
#ifndef BPVM_LINEA_MAX
#  define BPVM_LINEA_MAX 1024u
#endif

struct bpvm_thread;
/** Una pieza de la línea del `print` en curso de `tc` (NULL: sale ya, sin hilo al que guardársela). */
void bpvm_linea_pieza(struct bpvm* vm, struct bpvm_thread* tc, const char* s, size_t len);
/** El final de la línea: lo guardado + `s` + el salto, de UN empujón. */
void bpvm_linea_fin(struct bpvm* vm, struct bpvm_thread* tc, const char* s, size_t len);
/** Lo que quedó a medias, tal cual y sin salto: un hilo que acaba a mitad de un `print`, el fin del RUN. */
void bpvm_linea_vaciar(struct bpvm* vm, struct bpvm_thread* tc);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_OUT_H */
