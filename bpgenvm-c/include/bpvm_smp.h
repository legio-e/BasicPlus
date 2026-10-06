/*
 * bpvm_smp.h — LA COLA (V7/H12, etapa 2a.2): el objeto PERMANENTE del que sacan su trabajo los runs.
 *
 * Era `bpvm_smp_t`, el estado del planificador de varios workers, que nacía y moría con cada RUN (H2). Desde la 2a.2
 * es la cola de Eduardo (4-oct: «hace falta la cola de Threads BP, que también es permanente, aunque esté vacía»):
 * nace con el arranque —en la placa, cuando la familia crea la tarea `run`; en el PC, con la primera VM— y no muere
 * nunca. Hay UNA, `bpvm_la_cola` (scheduler.c), y un programa a la vez. Guarda:
 *   - el cerrojo y la condición;
 *   - LA REFERENCIA: `punto`, el hilo por donde sigue la rueda (threading.c); NULL, la cola vacía. Los hilos del
 *     programa entran al colgarlo —el main— o con Thread.start, y salen al acabar (o con el programa, al destruirlo);
 *   - el programa PUBLICADO: el que ejecutan los runs (NULL, ninguno);
 *   - lo que necesitan los runs para repartirse el trabajo, y el mundo parado del GC (heap.c), con más de uno.
 * El diseño: docs/V7_IDEAS.md §H12 «Etapa 2a.2».
 *
 * `vm->smp` apunta aquí mientras el programa está publicado y lo ejecuta MÁS DE UN run: entonces el cerrojo es de
 * verdad también para el heap, los Mutex y los hilos (bpvm_smp_lock). Con un run, NULL: nadie más toca nada de eso
 * mientras corre (como el planificador simple de antes) y no se paga.
 */
#ifndef BPVM_SMP_H
#define BPVM_SMP_H

#include "bpvm_platform.h"
#include <setjmp.h>     /* V7/H12: los registros de una pila aparcada */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bpvm;
struct bpvm_thread;

/* ---------- Output queue (workers → comm task) -----------
 * Ring buffer de bytes. Si se llena, productor bloquea (back-pressure).
 * Tamaño fijo, configurable en init. */
typedef struct bpvm_output_queue {
    char*    buf;            /* anillo de bytes */
    size_t   cap;            /* capacidad total */
    size_t   head;           /* siguiente posición de escritura */
    size_t   tail;           /* siguiente posición de lectura */
    size_t   used;           /* bytes ocupados; cap - used = libre */
    bpvm_platform_mutex_handle_t mtx;
    bpvm_platform_cond_handle_t  not_empty;   /* consumer waits when used=0 */
    bpvm_platform_cond_handle_t  not_full;    /* producer waits when used=cap */
    volatile bool closed;    /* true tras stop → consumer drena y sale */
    /* V7/H12, 2b (la revisión) — un productor a mitad de una línea que NO CABE en la cola (va a trozos, soltando el
     * cerrojo entre uno y otro): los demás esperan a que acabe, o se colarían entre dos trozos (#600). */
    bool escribiendo;
} bpvm_output_queue_t;

/* SÓLO PRUEBAS (test_cola): sin esperar al que escribe una línea a trozos, como antes (comm_common.c). */
extern int bpvm_oq_sabotaje_lineas;

/* El aviso a quien colgó un programa, cuando acaba (lo da el último run que sale, ya descolgado). */
typedef void (*bpvm_cola_al_acabar_fn)(struct bpvm* vm, int rs, void* user);

/* V7/H12 — UNA PILA DE C APARCADA: la de una native que espera al mundo parado de OTRO run DENTRO de sí misma (una
 * reserva que encuentra a otro colectando, la zona de rascar, el puente native→BP). Sus handles viven en sus locales
 * de C y en registros, y el colector del otro run no los ve: aquí deja el trozo [lo, hi) —desde este nodo, que vive en
 * el marco de quien espera y guarda los registros volcados, hasta el techo de su native más externa— para que lo
 * escanee (heap.c, 2f). Una lista de nodos en las pilas de quienes esperan, bajo vm_lock. */
typedef struct bpvm_aparcada {
    jmp_buf               regs;   /* los registros, volcados por setjmp: un handle puede vivir sólo en uno */
    uintptr_t             lo;
    uintptr_t             hi;     /* 0: no se apuntó (no estaba dentro de una native) */
    struct bpvm_aparcada* sig;
} bpvm_aparcada_t;

/* ---------- La cola ----------------------------------------- */
typedef struct bpvm_cola {
    /* — lo permanente — */
    int iniciada;                             /* el cerrojo y la condición, creados (bpvm_cola_asegurar) */
    /* El cerrojo. Cubre la cola (la referencia, el programa publicado, el estado del fin), y con más de un run también
     * el estado de cada hilo, sus next/back, las regiones de pila devueltas, heap_next/free_list, mutexes[],
     * symbols[] y gc_in_progress. */
    bpvm_platform_mutex_handle_t vm_lock;
    /* La condición: un programa publicado, un hilo que pasa a listo (un sueño que vence, un Mutex que se suelta, un join
     * cumplido, el GC que acaba), un KILL, el fin. */
    bpvm_platform_cond_handle_t  sched_cond;
    /* V7/H12, la 3.2 — EL MUNDO PARADO, con su propia condición: la esperan el colector (a quedarse solo), los runs que
     * esperan a que acabe una colecta y la zona de rascar; la avisan parar y reanudar el mundo y cada «un run menos en
     * marcha». Antes era `sched_cond`, y cada GC despertaba en vano a los runs OCIOSOS (que esperan trabajo, no al
     * mundo): con dos runs en dos núcleos, un programa de un hilo que reserva mucho —cadenas— iba un 27 % más lento,
     * con tantas «vueltas sin hilo» como colectas y su único hilo saltando de un run a otro. */
    bpvm_platform_cond_handle_t  mundo_cond;
    /* V7/H12, 2b (la revisión) — EL CERROJO DE LA HAL (builtins.c): reclamar un pin, abrir un bus, el ADC… con dos
     * runs. Propio y no el de la VM: lo que la HAL hace con él cogido puede escribir en la salida (las trazas de los
     * stubs del PC son stdout), la salida puede esperar a `io`, e `io` coge el de la VM para avisar de un KILL. */
    bpvm_platform_mutex_handle_t hal_lock;
    int permanentes;                          /* runs permanentes registrados (la tarea `run` de una placa) */

    /* — los hilos (threading.c) — */
    struct bpvm_thread* punto;   /* LA REFERENCIA: el hilo por donde sigue la rueda; NULL, la cola vacía */
    struct bpvm* dueno;          /* el programa cuyos hilos están en la cola: de colgarlo a destruirlo; NULL, ninguno */
    int32_t en_cola;             /* cuántos hilos hay en la cola */

    /* — el programa publicado: lo que ejecutan los runs — */
    struct bpvm* programa;       /* NULL, ninguno */
    int para_permanentes;        /* 1: lo ejecutan los runs permanentes; 0: quien llamó a bpvm_run o bpvm_run_smp */
    int n_runs;                  /* cuántos lo ejecutan; con más de uno, vm->smp apunta aquí */
    int acabando;                /* el último run que sale lo está cerrando: los demás ya no lo tocan */
    /* Los runs DENTRO del programa: de cogerlo (bpvm_cola_correr, bajo el cerrojo) a salir de él. No entran más que
     * `n_runs`; y el que lo cierra espera a ser el único dentro antes de descolgarlo y avisar: un run que no tiene un
     * hilo entre manos —dormido en la cola, o entre dos vueltas— todavía puede mirar la VM (la revisión adversarial de
     * la 2a.2: con dos runs permanentes, el otro la leía ya destruida). */
    int dentro;
    /* Los runs con un hilo entre manos: del momento de elegirlo a soltarlo, AUNQUE esperen dentro del cuanto a que otro
     * acabe de colectar. Es lo que decide el fin y el interbloqueo: «nadie tiene un hilo entre manos». No es
     * `running_workers` (el del GC), que descuenta al que espera al colector: contado con ése, otro run vería un
     * interbloqueo que no existe (un hilo que espera el Mutex que tiene ése). */
    int con_hilo;
    int parar;                   /* un KILL, un fallo o el STOP del depurador: no se elige más */
    int fallo;                   /* V7/H7 #596: el estado (bpvm_status_t) del PRIMER fallo; 0 = ninguno */
    uint32_t cuantos[2];         /* 2b: los cuantos que dio cada run (0 y 1) en este programa: la línea `runs:` del final */
    /* 2b: colectas con OTRO run dentro de su cuanto (heap.c, bpvm_gc_cuerpo). No debe pasar nunca: el fin del programa
     * lo dice, y la prueba del `gc()` con dos runs lo mira. Por programa. */
    uint32_t gc_con_otro_dentro;
    /* #662: las vueltas de un run que no encontró hilo que elegir —un run ocioso que se despierta en vano—. Por programa:
     * la línea `runs:` del final, y la prueba de que un programa de un hilo no despierta al otro run tras cada cuanto. */
    uint32_t vueltas_sin_hilo;
    /* V7/H12, LVGL con dos runs: los VIAJES al run 0 —un hilo que llegó a un builtin de la GUI en otro run y volvió a
     * la cola para el 0 (interp.c)— y los builtins de la GUI ejecutados FUERA del run 0 (builtins.c), que no deben
     * pasar nunca. Por programa: la línea `runs:` del final, y test-cola 18. */
    uint32_t gui_viajes;
    uint32_t gui_fuera_del_run0;
    int rs;                      /* lo que dio el programa: lo apunta el último run, al descolgarlo */
    bpvm_cola_al_acabar_fn al_acabar;
    void* al_acabar_user;

    /* — el mundo parado del GC, con más de un run (heap.c) — */
    /* Safepoint. Volatile para que el interp loop lo lea sin tomar lock (hot path). Cuando un run la pone true, los
     * demás ceden en el siguiente safepoint. */
    volatile bool stop_the_world;
    /* True mientras un run orquesta GC. Guardada bajo vm_lock. */
    bool gc_in_progress;
    /* Runs DENTRO del intérprete (el que espera a otro colector se descuenta: heap.c). El colector espera a quedarse
     * solo. */
    int running_workers;
    /* V7/H12 — las pilas de C de las natives que esperan a este mundo parado DENTRO de sí mismas (heap.c: el colector
     * las escanea). Bajo vm_lock. */
    bpvm_aparcada_t* aparcadas;

    /* — el `--smp` del PC sin `io`: la comm task (comm_host.c) drena la salida — */
    int comm_task_on;
    bpvm_output_queue_t output_queue;
    bpvm_platform_thread_handle_t comm_thread;

    /* — SÓLO PRUEBAS (test/test_cola.c y test/test_kill.c) — */
    int     ensayo_tras_cuanto_ms;   /* un run se entretiene después de su cuanto, con el hilo entre manos (el 0, esos
                                      * ms; el 1, cinco veces más: uno vuelve claramente antes que el otro) */
    int     ensayo_stw_ms;           /* el que esperó a otro colector tarda en volver a contarse (heap.c) */
    int     ensayo_salida_ms;        /* un run que sale de un programa que se cierra tarda en contarse fuera */
    int     sabotaje_fin;            /* decidir el fin sin mirar si alguien tiene un hilo entre manos */
    int     sabotaje_contador;       /* contar «con un hilo entre manos» con el contador del GC */
    int     sabotaje_despertar;      /* el KILL no avisa a la cola (bpvm_request_kill) */
    int     sabotaje_salida;         /* el que cierra no espera a que salgan los demás runs */
    int     sabotaje_gc;             /* 2b: el `gc()` del programa por el camino de antes (sin parar el mundo) */
    int     sabotaje_eventos;        /* 2b: `raise` encola sin el cerrojo */
    int     sabotaje_revive;         /* 2b: se resucita a un hilo que otro run aún tiene entre manos */
    int     ensayo_eventos;          /* 2b: encolar cede el turno a mitad (la ventana ensanchada) */
    int     sabotaje_ficheros;       /* 2b: los File (abrir, cerrar, el cursor) sin el cerrojo */
    int     ensayo_ficheros;         /* 2b: abrir, cerrar y escribir ceden el turno a mitad (la ventana ensanchada) */
    int     sabotaje_zona;           /* 2b: la zona de rascar ocupada por el otro run es un error, como antes */
    int     sabotaje_colecta;        /* 2b: quien esperó a otro mundo parado vuelve sin mirar si colectó (gc_stw) */
    int     sabotaje_owner;          /* 2b: soltar un owner como antes (sin mirar quién lo mató; el hueco, en el acto) */
    int     sabotaje_suyo;           /* 2b: el despertar y el join, sin mirar si otro run tiene el hilo entre manos */
    int     ensayo_linea_ms;         /* 2b: un hilo que termina tarda en soltar su línea a medias (interp.c) */
    int     sabotaje_error;          /* 2b: el error del programa, como antes: lo escribe cada throw y gana el último */
    int     sabotaje_aviso;          /* #662: avisar a la cola tras CADA cuanto, como antes (despierta al ocioso en vano) */
    int     sabotaje_mundo;          /* 3.2: el mundo parado avisa también en sched_cond, como antes (despierta al ocioso) */
    int     sabotaje_gui;            /* LVGL con dos runs: un builtin de la GUI se ejecuta en el run que lo encuentra */
    int     sabotaje_anidada;        /* H12: el puente (una llamada anidada) no se aparca con el mundo parado, como antes */
    int     sabotaje_aparcada;       /* H12: una native que espera al mundo dentro de sí no apunta su pila de C */
    int     sabotaje_deposito;       /* H12: el depósito de ejecutores sin el cerrojo, como antes */
    int     ensayo_deposito_ms;      /* H12: entre ver libre un ejecutor y reservarlo (la ventana ensanchada) */
    int     ensayo_sin_ejecutor;     /* H12: ninguna native consigue ejecutor (como sin memoria): corren en la del run */
    int     ensayo_colecta_us;       /* H12: cada colecta, más larga con el mundo parado (la ventana ensanchada de N3) */
    int64_t t_ultimo_cuanto;         /* con el ensayo: cuándo volvió el último cuanto (µs)... */
    int64_t t_fin;                   /* ...y cuándo se decidió el fin (µs): el fin, siempre después */
} bpvm_cola_t;

/* El nombre de antes: `vm->smp` es la cola, cuando el programa lo ejecuta más de un run. */
typedef bpvm_cola_t bpvm_smp_t;

/* LA cola (scheduler.c). */
extern bpvm_cola_t bpvm_la_cola;

/* Crea el cerrojo y la condición, la primera vez (0; -1 sin memoria). La llaman bpvm_init y quien registra un run
 * permanente; en una placa, en el arranque. */
int  bpvm_cola_asegurar(void);

/* COLGAR un programa: su main, a la cola —antes de prepararlo: el GC busca ahí sus raíces—; PREPARARLO (enlazar, el
 * main desde el punto de entrada, las excepciones prefabricadas); y PUBLICARLO para `n_runs` runs, los permanentes
 * (`para_permanentes`) o los de quien llama. Si la preparación falla, no se publica (su estado: un fallo de enlace…);
 * sus hilos se van con él al destruirlo. Con la cola de otro programa, error (un programa a la vez). */
int  bpvm_cola_colgar(struct bpvm* vm, int n_runs, int para_permanentes,
                      bpvm_cola_al_acabar_fn al_acabar, void* user);

/* UN RUN: ejecuta lo publicado para él. Permanente: no vuelve nunca (sin programa, espera dormido en la cola).
 * Temporal: vuelve cuando su programa acaba (o si ya no hay ninguno). `id`: el número del run (su marca en los hilos
 * que tiene entre manos). */
void bpvm_cola_correr(int permanente, int id);

/* Que los runs que esperan dormidos miren otra vez (un KILL). */
void bpvm_cola_despertar(void);

/* V7/H12, la 3.2 — el aviso del MUNDO PARADO (parar, reanudar, un run menos en marcha), en su condición: sólo a quien
 * espera al mundo; los runs ociosos no se enteran. Con el cerrojo de la cola cogido. */
void bpvm_cola_aviso_mundo(bpvm_cola_t* q);

/* Registra un run permanente (la tarea `run` de una placa, antes de crearla): desde ahí, el RUN cuelga los programas
 * para los permanentes. 0, o -1 si la cola no se puede crear. */
int  bpvm_cola_registrar_permanente(void);
/* Y de baja, si la familia no pudo crear la tarea (bpvm_repl_run_tarea_no_nacio). */
void bpvm_cola_baja_permanente(void);

/* bpvm_run_smp: `n` runs temporales —el que llama y n-1 hilos— sobre lo publicado; vuelve cuando acaba (scheduler_smp.c). */
void bpvm_cola_correr_n(int n);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_SMP_H */
