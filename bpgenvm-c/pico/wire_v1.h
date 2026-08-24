/*
 * wire_v1.h (RP2350) — la API del wire para esta placa.
 *
 * [V6/U2.1, 24-ago-2026] Casi todo lo que declaraba está ahora en
 * `include/bpvm_wire_v1.h`, que es el contrato de las dos capas: el CABLE (lo
 * pone cada familia) y el PROTOCOLO (común, `src/wire_v1_proto.c`). Aquí queda
 * sólo lo que es propio del RP2350.
 *
 * El framing lo especifica `docs/BPVM_WIRE_PROTOCOL.md` §2: mensajes JSON
 * delimitados por '
' en UTF-8, y bulk binario inline tras el '
' cuando el
 * mensaje declara `"bulk":N`.
 *
 * IMPORTANTE: en modo wire v1 NO se hace eco de los caracteres entrantes (a
 * diferencia del REPL de texto humano). El IDE no lo espera y rompería el
 * framing.
 */
#ifndef BPVM_PICO_WIRE_V1_H
#define BPVM_PICO_WIRE_V1_H

#include "bpvm_wire_v1.h"   /* el contrato: cable + protocolo */

#ifdef __cplusplus
extern "C" {
#endif

/* P-autorun (#256) — mutex de transmisión, PROPIO de esta placa: aquí conviven
 * dos escritores (la task de comm con los OUTPUT y el poll contestando
 * HELLO/BUSY en-run), y cada línea tiene que salir entera. Un escritor que
 * emita una línea POR PARTES (v1_output_sink) debe envolverla en lock/unlock.
 *
 * No sube al común porque no todas las familias lo necesitan igual: el S3 y la
 * P4 resuelven la atomicidad en su propio transporte. */
void wire_v1_tx_lock(void);
void wire_v1_tx_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_WIRE_V1_H */
