/*
 * bpvm_repl.h — EL CONTRATO del REPL wire v1, separado de la familia.
 *
 * ─── Por qué existe (V6/U3, 26-ago-2026) ─────────────────────────────────────
 *
 * El REPL estaba TRIPLICADO: `repl_v1.c` (Pico, 2054 líneas), `repl_esp32.c`
 * (S3+P4, 1344) y `stm32_repl.c` (920) — 4.318 líneas sin contrato, y el 100 %
 * de las asimetrías que nos han mordido: verbos que faltan en una familia,
 * respuestas distintas al mismo caso, arreglos que no viajan.
 *
 * El censo U3.0/U3.0b (FICHAS) midió las dos cosas que hacían falta ANTES de
 * escribir esto: qué verbos tiene cada familia (la Pico es superconjunto
 * estricto de las tres), y qué hay DEBAJO de cada verbo ausente — porque un
 * verbo enrutado a nada no es un verbo (pregunta de Eduardo, y hasta la Pico
 * tenía verbos-fachada).
 *
 * ─── El reparto ──────────────────────────────────────────────────────────────
 *
 *   COMÚN (src/bpvm_repl.c)  lo que solo toca contratos ya comunes: el wire
 *                            (bpvm_wire_v1.h), el FS (bpvm_fs.h), el log
 *                            (bpvm_log.h), el RTC... Interpretar "PING" no es
 *                            hardware.
 *   FAMILIA                  el cable, RUN (por ahora), y los verbos de SU
 *                            hardware (BOOTSEL...). Donde no hay soporte, la
 *                            respuesta es un ERROR CON NOMBRE, nunca silencio.
 *
 * ─── Cómo se usa ─────────────────────────────────────────────────────────────
 *
 *   El dispatcher de la familia, tras parsear type/id/obj, pregunta PRIMERO:
 *
 *       if (bpvm_repl_dispatch(type, id, &obj)) return;   // atendido
 *       ...sigue su cadena de verbos propios...
 *
 *   Si una familia necesita sombrear un verbo del común (no debería), lo
 *   atiende ANTES de llamar aquí. La migración es por GRUPOS y cada grupo se
 *   verifica en placa antes del siguiente — criterio de Eduardo para todo el
 *   cordón: *«pasitos pequeños, cada cambio verificado en placa; nos importa
 *   más la seguridad que la velocidad»*.
 *
 * ─── Qué verbos atiende hoy el común ─────────────────────────────────────────
 *
 *   Grupo 1 (meta):  PING · TIME · LOG_DUMP · LOG_CLEAR
 *
 *   Divergencias que este grupo UNIFICA (medidas el 26-ago, U3.0b):
 *     · TIME sin `epochSec`: la Pico daba INVALID_PARAM; S3/STM32 contestaban
 *       OK en silencio. Gana la Pico: el error se DICE.
 *     · LOG_CLEAR: la Pico dejaba la marca «LOG cleared via wire v1» en el log
 *       nuevo; el STM32 no. Se deja siempre: fecha el corte al leer un dump.
 *     · el sink de LOG_DUMP del STM32 PERDÍA chunks en silencio si el escape
 *       no cabía en su buffer; el del común trocea y no pierde.
 */
#ifndef BPVM_REPL_H
#define BPVM_REPL_H

#include "json_min.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Atiende `type` si es un verbo del común. Devuelve 1 si lo atendió (la
 *  respuesta ya salió por el wire), 0 si no es suyo y la familia debe seguir
 *  con su cadena. `obj` es el mensaje ya parseado (para los parámetros). */
int bpvm_repl_dispatch(const char* type, long id, const json_obj_t* obj);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_REPL_H */
