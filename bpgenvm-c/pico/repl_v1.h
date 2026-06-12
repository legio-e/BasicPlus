/*
 * repl_v1.h — entrada del REPL en modo wire BPVM v1.
 *
 * Convive con el repl-texto legacy de repl.c durante la migración.
 * El despachador en repl.c detecta el primer carácter de cada línea:
 *   '{' → wire v1, delega aquí.
 *   letra → protocolo texto legacy.
 *
 * La función `repl_v1_handle_request` recibe la línea JSON YA leída
 * (sin el '{' inicial que el despachador consumió para detectar) y
 * el byte que faltaba (el '{' literal). Internamente reconstruye el
 * objeto, lo parsea, y manda la respuesta.
 *
 * Fase A: solo HELLO/HELLO_REPLY implementado. Los demás tipos
 * responden con UNSUPPORTED y se irán añadiendo en fases B-E.
 */
#ifndef BPVM_PICO_REPL_V1_H
#define BPVM_PICO_REPL_V1_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Procesa una request entrante en modo v1.
 *
 * Lee la línea completa desde stdin (sembrando con `first_char` que
 * el dispatcher ya consumió — típicamente '{'). Parsea el JSON,
 * despacha por `type`, y envía la reply/error correspondiente. Si
 * la línea trae `"bulk":N`, lee los N bytes raw que siguen antes
 * de despachar. */
void repl_v1_handle_request(int first_char);

/* P-autorun (#256) — si existe /sys/auto.txt, ejecuta el módulo que
 * indica (primera línea) por el mismo camino que un RUN del wire:
 * sesión + OUTPUT events + poll (HELLO/KILL atendidos en caliente) +
 * EXITED. Bloquea hasta que el programa termina (o lo matan); después
 * el llamante entra al REPL normal. Sin fichero / vacío / ruta mala →
 * log y retorno inmediato. Llamar UNA vez al boot, antes de repl_run. */
void repl_v1_autorun(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_REPL_V1_H */
