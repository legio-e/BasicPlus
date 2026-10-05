/*
 * repl_v1.h — el REPL del Pico: wire BPVM v1.
 *
 * #305: aquí vivía sólo el despachador, porque el BUCLE estaba en repl.c junto
 * a un REPL de texto legacy que elegía protocolo por el primer carácter. El modo
 * texto se retiró (duplicaba RUN/GET/PUT por un camino que nadie ejercitaba) y
 * el bucle se mudó aquí: un fichero, un protocolo — como el ESP32 y el STM32,
 * que nunca tuvieron modo texto.
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
#include <stdint.h>   /* V7/H12, 2b: g_run2_pila */

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

/* V7/H7 (Boot 2, fase 4) — registra la cintura del REPL común (bpvm_repl_set_ops). La
 * llama el arranque común antes del cable y del autorun: un IDE que se conecta con el
 * autorun corriendo saluda, y el saludo lo contesta el común con esta cintura. */
void repl_v1_cintura(void);

/* Bucle de transporte: lee de stdin y despacha cada request del wire. No
 * retorna. Pensado para correr como task de FreeRTOS. */
void repl_v1_run(void);

/* Reparto heap/stacks del bloque de la VM (ver el comentario largo en el .c).
 * Se expone para que el arranque de main.c use EL MISMO cálculo que el RUN y
 * el INFO, en vez de copiarlo. */
size_t vm_stack_region_bytes(void);

/* V7/H12 — la pila de cada tarea `run`: 16 KB (el GC corre en ella y su marcado es recursivo; el porqué, en el .c). */
#define PICO_RUN_PILA_BYTES (16u * 1024u)
/* V7/H12, 2b — la pila del 2º run (`vm.runs=2` en el ENV): la aparta el arranque del TECHO del bloque de la VM (main.c,
 * pico_memoria), así que sólo cuesta con dos runs. NULL: no se apartó (un run, o la VM en la PSRAM: entonces la
 * cintura la pide a malloc, que tiene la SRAM libre). */
extern uint8_t* g_run2_pila;

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_REPL_V1_H */
