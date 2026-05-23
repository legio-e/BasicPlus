/*
 * repl.h — REPL sobre USB CDC: comandos LS/PUT/GET/DEL/RUN/MEM/...
 *
 * Wire format line-based. Comandos terminados en \n. Las respuestas:
 *   OK\n                  — éxito sin payload
 *   OK <size>\n<bytes>    — éxito con N bytes binarios después
 *   ERR <mensaje>\n       — error con mensaje en texto
 *
 * Ver comando HELP para la lista completa. El cliente Python
 * scripts/bpvm-pico.py habla este protocolo.
 */
#ifndef BPVM_PICO_REPL_H
#define BPVM_PICO_REPL_H

/* Loop infinito: lee comandos de stdin y los procesa. Pensado para
 * ejecutarse como una FreeRTOS task. */
void repl_run(void);

#endif /* BPVM_PICO_REPL_H */
