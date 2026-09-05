/*
 * bpvm_io.h — V6/A1: EL HILO `io`, el segundo de los dos que tiene el sistema.
 *
 * ─── LA DECISIÓN (Eduardo, 4-sep-2026, ficha A1) ─────────────────────────────
 *
 *   *«El hilo de la VM debería dedicarse solamente a ejecutar los opcodes. ¿Y
 *   entonces qué pasa con todo lo demás? Hace falta al menos un segundo hilo de
 *   ejecución. Y la misma arquitectura en todos los micros.»*
 *
 * Así que son DOS hilos de sistema operativo, con el mismo papel en el PC y en
 * los cinco firmwares:
 *
 *   `vm` — ejecuta opcodes. Dentro: intérprete, scheduler de hilos verdes (que
 *          son verdes, no tareas del SO) y GC. NO toca ningún transporte: un
 *          `print` deja bytes en una cola y sigue.
 *   `io` — todo lo demás: leer el wire, despachar el REPL, enmarcar la salida
 *          en mensajes OUTPUT, el log y (desde A1.6) bombear LVGL.
 *
 * Entre los dos SÓLO cruzan colas. Nada de memoria compartida suelta: es la
 * única regla que hace que el diseño valga igual con un núcleo y con dos (en
 * dos núcleos, cada hilo en el suyo, y lo que cruza va por primitivas del RTOS,
 * que es donde están las barreras).
 *
 * ─── POR QUÉ, CON NÚMEROS (medidos en la C6 el 4-sep) ────────────────────────
 *
 * Esto NO acelera el intérprete, y decirlo importa: lo que la tarea de la VM
 * hacía «además de interpretar» entre cuantos cuesta un 1 % (cuanto 1024 vs
 * 65536: 23 640 vs 23 400 ms en fib(28)). Lo que arregla es la SALIDA: un
 * `print` costaba 0,84 ms — ochenta veces el bucle que lo produce — porque la
 * escritura al transporte es SÍNCRONA y cada `print "a", i, "b"` salía en SEIS
 * mensajes OUTPUT (122 KB de texto viajaron como 816 KB). Con `io`, el `print`
 * es un empujón a la cola; el marco JSON lo pone `io`, una vez POR LÍNEA.
 *
 * Y de paso desaparece `#462` por construcción: `vm` ya no puede matar de
 * hambre al resto del sistema, porque quien atiende el wire es otro hilo.
 *
 * ─── LO QUE ES COMÚN Y LO QUE PONE CADA FAMILIA ──────────────────────────────
 *
 * Todo lo de este módulo es común (`src/bpvm_io.c`). Cada familia pone sólo su
 * `bpvm_io_ops_t`: cómo se atiende su transporte y a dónde va una línea. El
 * lazo, la cola, el troceado por líneas y el apagado ordenado son los mismos en
 * todas partes — que era el encargo: *«esto debería ser código común, así que
 * una vez hecho para 1 micro debería funcionar en el resto»*.
 *
 * ─── EL FUTURO QUE NO SE CIERRA ──────────────────────────────────────────────
 *
 * *«En un futuro tendremos dos VM y una cola de threads BP.»* El hilo `vm` de
 * hoy es el worker único de mañana: la cola de salida ya es N-productores (toma
 * su mutex), así que dos VM escribiendo en ella no cambia nada aquí.
 */
#ifndef BPVM_IO_H
#define BPVM_IO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bpvm;

/* ── Lo único que NO es común: el transporte de cada familia ────────────────
 *
 * `poll` — atiende el transporte: si hay un mensaje del wire, lo lee y lo
 *   despacha. DEBE volver pronto (bloquear como mucho unos ms); el lazo de
 *   `io` lo llama una y otra vez. Devuelve != 0 para pedir que la ejecución
 *   pare (KILL): `io` pone entonces `kill_requested` en la VM, que lo ve en la
 *   siguiente frontera de cuanto. NULL = este `io` no atiende transporte (el
 *   CLI del PC, que sólo drena salida).
 *
 * `line` — entrega UNA línea de la salida del programa, CON su '\n' si lo
 *   tenía. Aquí es donde la familia (o el REPL) la enmarca como OUTPUT. Se
 *   llama SIEMPRE desde el hilo `io`, nunca desde `vm`: por eso el sink puede
 *   usar buffers estáticos sin cerrojo, como hacía cuando todo era un hilo.
 *   NULL = a `stdout` (el camino del CLI).
 *
 * Una línea más larga que el buffer interno llega TROCEADA (varias llamadas,
 * la última con el '\n'). El texto concatenado es idéntico byte a byte al de
 * antes: eso es lo que protege la paridad con la VM-Java. */
typedef struct {
    int  (*poll)(void* user);
    void (*line)(const char* s, size_t len, void* user);
    void* user;
} bpvm_io_ops_t;

/* Arranca el hilo `io`. `oq_cap` = bytes de la cola de salida (0 = el defecto,
 * 4096 en el PC; los firmwares pasan el suyo). Devuelve 0 si arrancó.
 * A partir de aquí, `print` NO escribe: encola. */
int  bpvm_io_start(struct bpvm* vm, const bpvm_io_ops_t* ops, size_t oq_cap);

/* Cierra la cola, espera a que `io` la drene ENTERA y lo une. Después de
 * volver, toda la salida del programa está entregada — llamarlo antes del
 * EXITED es lo que garantiza que no se pierda la última línea. */
void bpvm_io_stop(struct bpvm* vm);

/* La salida del programa, desde `vm`. Bloquea sólo si la cola está llena, que
 * es cuando el programa produce más deprisa de lo que el transporte traga
 * (contrapresión: ahí no hay hilo que lo arregle). */
void bpvm_io_write(struct bpvm* vm, const char* s, size_t len);

/* ¿Hay un `io` en marcha? Lo usa `emit_text` (y el scheduler, para no llamar al
 * `poll_cb` viejo cuando quien atiende el wire ya es `io`). */
int  bpvm_io_running(const struct bpvm* vm);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_IO_H */
