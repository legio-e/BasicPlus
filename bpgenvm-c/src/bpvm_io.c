/*
 * bpvm_io.c — V6/A1.1: el hilo `io`, común a las cinco familias y al PC.
 *
 * El porqué y el reparto están en `include/bpvm_io.h`. Aquí, el cómo — que son
 * tres cosas y ninguna sabe de transportes:
 *
 *   1. UN LAZO que alterna dos gestos: sacar bytes de la cola de salida (con
 *      TOPE de espera, no bloqueante para siempre) y llamar al `poll` de la
 *      familia para que atienda su wire. Un programa que no imprime no puede
 *      dejar dormido a `io`: por eso el pop es acotado.
 *
 *   2. EL TROCEADO POR LÍNEAS. Es la razón de ser de todo esto. Antes, cada
 *      `print "a", i, "b"` llegaba al transporte en SEIS trozos y cada trozo
 *      era un mensaje OUTPUT con su marco JSON: 122 KB de texto viajaban como
 *      816 KB y costaban 0,84 ms por línea (medido en la C6, 4-sep). Aquí los
 *      trozos se pegan y se entrega UNA línea, con su '\n'. El texto
 *      concatenado es el mismo byte a byte — que es lo que protege la paridad
 *      con la VM-Java.
 *
 *   3. UN APAGADO ORDENADO. `stop` cierra la cola y espera a que el lazo la
 *      drene ENTERA antes de unir el hilo. Sin eso, la última línea de un
 *      programa se perdería entre el final del RUN y el EXITED — la clase de
 *      fallo que no se ve hasta que un sample se queda sin su última línea.
 *
 * ⚠️ NADA de aquí toca el heap de la VM ni sus estructuras. Lo único que `io`
 * escribe de la VM es `kill_requested` (que es `volatile int` y lo lee el
 * scheduler entre cuantos, nunca a mitad de opcode). Es la regla de A1: entre
 * `vm` e `io` sólo cruzan colas.
 */
#include "bpvm_io.h"
#include "bpvm_internal.h"
#include "bpvm_platform.h"
#include "comm_queue.h"
#include "bpvm_alloc.h"
#include <stdio.h>
#include <string.h>

/* Cuánto espera el lazo por bytes antes de volver a mirar el wire. 5 ms es
 * inapreciable para la salida (la cola se llena mucho más deprisa) y hace que
 * un KILL se atienda en ese plazo aunque el programa esté mudo. */
#define IO_POLL_MS        5
/* Trozo con el que se vacía la cola. No es el tamaño de una línea: las líneas
 * se arman en `linea` (abajo). */
#define IO_CHUNK          256
/* Tope de una línea antes de entregarla partida. Cota DURA de RAM: no hay
 * reserva por línea, ni en el PC ni en el micro. Una línea más larga llega en
 * varias entregas y el texto sigue siendo el mismo. */
#define IO_LINE_CAP       512
#define IO_OQ_CAP_DEFAULT 4096

struct bpvm_io {
    bpvm_output_queue_t          oq;
    bpvm_io_ops_t                ops;
    bpvm_platform_thread_handle_t th;
    struct bpvm*                 vm;
    int                          arrancado;
    char                         linea[IO_LINE_CAP];
    size_t                       linea_n;
};

/* ─── la entrega: una línea (o un trozo de línea larga) ─────────────────────
 * Con `line` a NULL, a stdout — el camino del CLI del PC, donde la salida del
 * programa ES la salida del proceso. */
static void entregar(struct bpvm_io* io, const char* s, size_t n) {
    if (n == 0) return;
    if (io->ops.line) io->ops.line(s, n, io->ops.user);
    else              fwrite(s, 1, n, stdout);
}

static void purgar_linea(struct bpvm_io* io) {
    if (io->linea_n > 0) {
        entregar(io, io->linea, io->linea_n);
        io->linea_n = 0;
    }
}

/* Pega los bytes que salen de la cola y entrega por líneas completas. */
static void trocear(struct bpvm_io* io, const char* s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        io->linea[io->linea_n++] = s[i];
        if (s[i] == '\n' || io->linea_n == IO_LINE_CAP) purgar_linea(io);
    }
}

static void io_loop(void* arg) {
    struct bpvm_io* io = (struct bpvm_io*) arg;
    char tmp[IO_CHUNK];
    for (;;) {
        int eof = 0;
        size_t n = bpvm_oq_pop_timed(&io->oq, tmp, sizeof tmp, IO_POLL_MS, &eof);
        if (n > 0) trocear(io, tmp, n);
        /* El wire, SIEMPRE: aunque la cola venga llena, un KILL no espera a que
         * el programa deje de imprimir. */
        if (io->ops.poll && io->ops.poll(io->ops.user) != 0)
            bpvm_request_kill(io->vm);
        if (eof) break;                 /* cerrada y seca: no queda nada que dar */
    }
    purgar_linea(io);                   /* un `print` sin '\n' final también sale */
}

/* ─── API ───────────────────────────────────────────────────────────────── */

int bpvm_io_start(struct bpvm* vm, const bpvm_io_ops_t* ops, size_t oq_cap) {
    if (!vm || vm->io) return -1;
    struct bpvm_io* io = (struct bpvm_io*) bpvm_calloc(1, sizeof *io);
    if (!io) return -1;
    if (ops) io->ops = *ops;
    io->vm = vm;
    if (bpvm_oq_init(&io->oq, oq_cap > 0 ? oq_cap : IO_OQ_CAP_DEFAULT) != 0) {
        bpvm_free(io);
        return -1;
    }
    /* El puntero se publica ANTES de arrancar el hilo: en cuanto exista, el
     * lazo puede tocar la cola, y `emit_text` puede empezar a encolar. */
    vm->io = io;
    if (bpvm_platform_thread_create(&io->th, io_loop, io) != 0) {
        vm->io = NULL;
        bpvm_oq_destroy(&io->oq);
        bpvm_free(io);
        return -1;
    }
    io->arrancado = 1;
    return 0;
}

void bpvm_io_stop(struct bpvm* vm) {
    if (!vm || !vm->io) return;
    struct bpvm_io* io = vm->io;
    /* Orden: cerrar (el lazo drena lo que quede y sale) → unir → recién
     * entonces soltar el puntero. Al revés, un `print` tardío escribiría en una
     * cola liberada. */
    bpvm_oq_close(&io->oq);
    if (io->arrancado) bpvm_platform_thread_join(&io->th);
    vm->io = NULL;
    bpvm_oq_destroy(&io->oq);
    bpvm_free(io);
}

void bpvm_io_write(struct bpvm* vm, const char* s, size_t len) {
    if (!vm || !vm->io || !s || len == 0) return;
    bpvm_oq_push(&vm->io->oq, s, len);
}

int bpvm_io_running(const struct bpvm* vm) {
    return (vm && vm->io) ? 1 : 0;
}
