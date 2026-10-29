/*
 * threading.c — helpers para Thread y Mutex BP (F4 v1).
 *
 * Cada hilo BP es, para la VM, una bpvm_thread_t pedida con malloc; la COLA de hilos son esas structs (una lista
 * doble y circular: V7/H12, etapa 2, más abajo), y los planificadores la recorren en rueda.
 *
 * Mutex BP: pool dinámico indexado por mid (id de mutex). __mutexCreate
 * devuelve el id. El layout del objeto Mutex BP es opaco para nosotros
 * — el frontend pone el mid en un field del objeto que es lo que llega
 * a __mutexLock/Unlock.
 */

#include "bpvm_internal.h"
#include <stdio.h>
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */

/* ---- Mutex pool ---- */

/* H2 — Las ops del pool de mutex BP mutan estado compartido
 * (mutex_count, mutexes[mid].waiters/waiter_count). En SMP las
 * envolvemos en vm_lock. En legacy es no-op. NO recursivas — el
 * caller (typically un builtin desde el interp) está fuera de
 * vm_lock. */

int bpvm_mutex_alloc(bpvm_t* vm) {
    bpvm_smp_lock(vm);
    if (vm->mutex_count >= vm->mutex_capacity) {
        int new_cap = vm->mutex_capacity == 0 ? 8 : vm->mutex_capacity * 2;
        bpvm_bp_mutex_t* arr = (bpvm_bp_mutex_t*) bpvm_realloc(vm->mutexes,
                                (size_t) new_cap * sizeof(bpvm_bp_mutex_t));
        if (!arr) { bpvm_smp_unlock(vm); return -1; }
        vm->mutexes = arr;
        vm->mutex_capacity = new_cap;
    }
    int mid = vm->mutex_count++;
    bpvm_bp_mutex_t* m = &vm->mutexes[mid];
    m->owner_tid = -1;
    m->waiters = NULL;
    m->waiter_count = 0;
    m->waiter_capacity = 0;
    bpvm_smp_unlock(vm);
    return mid;
}

/* ── LA PROPIEDAD DE UN MUTEX: UN SOLO SITIO QUE LA TOCA ───────────────────────
 *
 * Criterio de Eduardo (9-sep), el mismo que arreglo B1 en miVM: si el mutex es
 * una clase (aqui una struct), el estado se cambia por UN metodo y es AHI donde
 * se protege. Antes el builtin hacia el check-and-set a pelo:
 *
 *     if (m->owner_tid < 0) { m->owner_tid = tc->id; ... }   // LEE y ESCRIBE
 *
 * ...y el interprete corre SIN vm_lock (lo dice el propio bpvm_internal.h sobre
 * `sched_owner`). Con 2 workers, los dos ven el mutex libre y los dos lo toman.
 *
 * 🔴 Y el cuelgue venia de una ventana peor, un LOST WAKEUP de manual:
 *   1. el thread A se apunta como waiter... y todavia NO se ha puesto BLOCKED,
 *   2. el thread B hace unlock, saca a A de la cola, le da la propiedad y lo pone
 *      RUNNABLE,
 *   3. A ejecuta por fin `status = BLOCKED_MUTEX` y PISA el despertar.
 * A tiene el mutex y esta bloqueado para siempre: nadie lo va a despertar.
 * Medido: 7 de cada 8 pasadas con --smp=2 se colgaban ahi, mudas.
 *
 * Por eso apuntarse Y bloquearse van en la MISMA seccion critica. El cerrojo es
 * no-op cuando no hay SMP, asi que el camino normal no paga nada. */

int bpvm_mutex_try_acquire(bpvm_t* vm, int mid, int tid) {
    int r;
    bpvm_smp_lock(vm);
    {
        bpvm_bp_mutex_t* m = &vm->mutexes[mid];
        if (m->owner_tid == tid) {
            r = -1;                       /* re-entrada: la denuncia el llamante */
        } else if (m->owner_tid < 0) {
            m->owner_tid = tid;
            r = 1;                        /* tomado */
        } else {
            /* Ocupado: apuntarse Y bloquearse, juntos y bajo el cerrojo. (El que llama es el hilo en curso: está en la
             * cola. H12, etapa 2: por su handle.) */
            bpvm_thread_t* yo = bpvm_hilo_por_tid(vm, tid);
            bpvm_mutex_add_waiter_locked(vm, mid, tid);
            if (yo != NULL) {
                yo->blocked_on_mutex = mid;
                yo->status = BPVM_THREAD_BLOCKED_MUTEX;
            }
            r = 0;                        /* bloqueado */
        }
    }
    bpvm_smp_unlock(vm);
    return r;
}

int bpvm_mutex_release(bpvm_t* vm, int mid, int tid, int* owner_out) {
    int r;
    bpvm_smp_lock(vm);
    {
        bpvm_bp_mutex_t* m = &vm->mutexes[mid];
        if (m->owner_tid != tid) {
            if (owner_out) *owner_out = m->owner_tid;
            r = -1;                       /* no es suyo: lo denuncia el llamante */
        } else {
            int next = bpvm_mutex_pop_waiter_locked(vm, mid);
            bpvm_thread_t* sig = (next >= 0) ? bpvm_hilo_por_tid(vm, next) : NULL;   /* H12: por su handle */
            if (sig != NULL) {
                m->owner_tid = next;
                sig->blocked_on_mutex = -1;
                if (sig->status == BPVM_THREAD_BLOCKED_MUTEX) {
                    sig->status = BPVM_THREAD_RUNNABLE;
                }
            } else {
                m->owner_tid = -1;
            }
            r = 0;
        }
    }
    bpvm_smp_unlock(vm);
    return r;
}

void bpvm_mutex_add_waiter_locked(bpvm_t* vm, int mid, int tid) {
    if (mid < 0 || mid >= vm->mutex_count) { return; }
    bpvm_bp_mutex_t* m = &vm->mutexes[mid];
    if (m->waiter_count >= m->waiter_capacity) {
        int new_cap = m->waiter_capacity == 0 ? 4 : m->waiter_capacity * 2;
        int32_t* arr = (int32_t*) bpvm_realloc(m->waiters,
                        (size_t) new_cap * sizeof(int32_t));
        if (!arr) { return; }
        m->waiters = arr;
        m->waiter_capacity = new_cap;
    }
    m->waiters[m->waiter_count++] = tid;
}

/* Publica: toma el cerrojo y delega. La version `_locked` es la que usan
   `try_acquire`/`release`, que YA lo sostienen — y el vm_lock NO es
   recursivo, asi que llamar a esta desde alli se colgaria. */
void bpvm_mutex_add_waiter(bpvm_t* vm, int mid, int tid) {
    bpvm_smp_lock(vm);
    bpvm_mutex_add_waiter_locked(vm, mid, tid);
    bpvm_smp_unlock(vm);
}

/* Saca y devuelve el primer waiter en FIFO. -1 si no hay. */
int bpvm_mutex_pop_waiter_locked(bpvm_t* vm, int mid) {
    if (mid < 0 || mid >= vm->mutex_count) { return -1; }
    bpvm_bp_mutex_t* m = &vm->mutexes[mid];
    if (m->waiter_count == 0) { return -1; }
    int tid = m->waiters[0];
    /* shift down (FIFO). Pocos waiters habitualmente. */
    for (int i = 1; i < m->waiter_count; i++) {
        m->waiters[i - 1] = m->waiters[i];
    }
    m->waiter_count--;
    /* Algún waiter despertado: avisar al scheduler para que repick. */
    if (vm->smp) {
        bpvm_platform_cond_broadcast(&vm->smp->sched_cond);
    }
    return tid;
}

/* Publica: toma el cerrojo y delega. La version `_locked` es la que usan
   `try_acquire`/`release`, que YA lo sostienen — y el vm_lock NO es
   recursivo, asi que llamar a esta desde alli se colgaria. */
int bpvm_mutex_pop_waiter(bpvm_t* vm, int mid) {
    bpvm_smp_lock(vm);
    int r = bpvm_mutex_pop_waiter_locked(vm, mid);
    bpvm_smp_unlock(vm);
    return r;
}

/* ---- Thread spawn ---- *
 *
 * Llamado por el builtin __threadStart. `thread_ref` es el user_ref del
 * objeto Thread BP (que tiene un campo `run` virtual). Creamos un
 * nuevo ThreadContext con su propia región de stack, inicializamos su
 * frame para llamar a t.run(), y lo marcamos RUNNABLE para que el
 * scheduler lo pickee.
 *
 * Convención del frontend (Thread sintetizado): el método `run` está
 * en vtSlot 0 de la vtable de Thread. Lo emitimos como CALL al método
 * (no INVOKE_VIRTUAL porque el frame es nuevo y el `this` ya está
 * pusheado). Pero como nosotros NO sabemos el offset del método run
 * — depende de la subclass del usuario — tenemos que hacerlo via
 * INVOKE_VIRTUAL: empujamos `this` y llamamos `run` (slot 0).
 *
 * Devuelve el tid del thread nuevo; -1 si no hay memoria para su struct (H12), -2 si run() no
 * se resuelve en la cadena de herencia, -3 si no cabe su pila (V7/H7 #604: el builtin lanza
 * cada uno con su texto; el de la pila, el de miVM) y -4 si no hay objeto.
 */

/* ═══ V7/H12, etapa 2 (4-oct) — LA COLA DE HILOS ══════════════════════════════════════════════════════════════
 *
 * El diseño, de Eduardo: el hilo, para la VM, es una struct de C (bpvm_thread_t) pedida con malloc al arrancarlo y
 * devuelta cuando sale de la cola; la cola son esas structs, una lista doble y circular (`next`, `back`) —sin array y
 * sin tope: lo pone la memoria—; hay UNA referencia a ella (el punto donde sigue la rueda; NULL, vacía); y el Thread de
 * BP tiene un handle a la suya, su `__tid`, que no se reutiliza nunca (`vm->sig_tid` sólo crece): resolverlo es dar una
 * vuelta, y si no está, el hilo acabó. Antes (#604): 32 huecos fijos dentro de bpvm_t, reutilizados, y el `__tid` del
 * Thread viejo había que ponerlo a 0 para que su `join` no esperase al hilo nuevo del mismo hueco. Desde la 2a.2 la
 * cola es PERMANENTE (`bpvm_la_cola`, bpvm_smp.h: la referencia es su `punto`): los hilos de un programa entran al
 * colgarlo —el main— o con Thread.start, salen al acabar, y los que quedan se van con él al destruirlo. Un programa a
 * la vez: los hilos de `vm` son los de la cola si `vm` es su dueño; si no, ninguno. Todo lo que toca la cola está aquí
 * y va bajo vm_lock (no-op con un run). Y toda búsqueda da como mucho UNA vuelta: se para al volver al punto de
 * partida. */

bpvm_thread_t* bpvm_hilos_primero(const bpvm_t* vm) {
    return (bpvm_la_cola.dueno == vm) ? bpvm_la_cola.punto : NULL;
}

bpvm_thread_t* bpvm_hilos_siguiente(const bpvm_t* vm, const bpvm_thread_t* h) {
    (void) vm;
    return (h->next == bpvm_la_cola.punto) ? NULL : h->next;   /* de vuelta al punto de partida: la vuelta acaba */
}

bpvm_thread_t* bpvm_hilo_por_tid(const bpvm_t* vm, int32_t tid) {
    for (bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h))
        if (h->id == tid) return h;
    return NULL;
}

bpvm_thread_t* bpvm_hilos_elegir(bpvm_t* vm, int libre, int run) {
    bpvm_thread_t* c = bpvm_hilos_primero(vm);
    if (c == NULL) return NULL;
    bpvm_thread_t* h = c;
    do {                                   /* desde el siguiente al punto de la rueda hasta él mismo: una vuelta */
        h = h->next;
        /* V7/H12 — el que espera al run 0 (un builtin de la GUI encontrado en otro run: interp.c) sólo lo elige el 0 */
        if (h->status == BPVM_THREAD_RUNNABLE && (!libre || h->sched_owner == -1) && (!h->al_run0 || run == 0)) {
            bpvm_la_cola.punto = h;        /* la rueda sigue en el elegido: el próximo empieza en su siguiente */
            vm->latido_tid = h->id;        /* para el latido, que lo lee desde otra tarea (sin punteros) */
            vm->latido_pc = h->pc;
            return h;
        }
    } while (h != c);
    return NULL;
}

int bpvm_hilos_esperan_al_run0(const bpvm_t* vm) {
    for (const bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h))
        if (h->al_run0 && h->status == BPVM_THREAD_RUNNABLE && h->sched_owner == -1) return 1;
    return 0;
}

/* Mete `h` al final de la rueda: justo detrás del último elegido, así que le toca cuando le hayan tocado a todos. */
static void cola_meter(bpvm_t* vm, bpvm_thread_t* h) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (++q->en_cola > vm->hilos_max) vm->hilos_max = q->en_cola;
    bpvm_thread_t* c = q->punto;
    if (c == NULL) { h->next = h; h->back = h; q->punto = h; return; }
    h->next = c;
    h->back = c->back;
    c->back->next = h;
    c->back = h;
}

/* Saca `h`. Si era el punto de la rueda, el punto pasa a su ANTERIOR: la próxima elección empieza en el siguiente de
 * ése, que es el que venía detrás de `h` (con el siguiente se saltaría un turno). Si era el único, la cola queda vacía. */
static void cola_sacar(bpvm_thread_t* h) {
    bpvm_cola_t* q = &bpvm_la_cola;
    q->en_cola--;
    if (h->next == h) {
        q->punto = NULL;
    } else {
        h->back->next = h->next;
        h->next->back = h->back;
        if (q->punto == h) q->punto = h->back;
    }
    h->next = h->back = NULL;
}

/* ¿Puede salir de la cola? Si su hilo acabó DEL TODO: terminado, sin un worker encima (SMP), sin un ejecutor de
 * natives atado y sin eventos que saldar (#342: a un hilo muerto con eventos pendientes se le resucita para
 * atenderlos). El main, no: sus cifras son del final del RUN (cerrar_run). Los que esperan su `join` no lo impiden:
 * resuelven su handle, no lo encuentran y vuelven. V8 #740: es también lo que mira el GC —un hilo así ya no es raíz, y
 * su pila, un bloque del heap, se barre como cualquiera (bpvm_hilo_acabado, heap.c)—. */
static int puede_salir(const bpvm_t* vm, const bpvm_thread_t* h) {
    return h != vm->hilo_main && h->status == BPVM_THREAD_TERMINATED && h->sched_owner == -1
        && h->nativa == NULL && !bpvm_events_con_deuda(vm, h);
}

int bpvm_hilo_acabado(const bpvm_t* vm, const bpvm_thread_t* h) {
    return puede_salir(vm, h);
}

/* #605 — ¿el más cerca de su tope, de los que no son el main? (la línea `pila:` del final del RUN, en bpvm.c) */
void bpvm_hilo_apuntar_pila(bpvm_t* vm, const bpvm_thread_t* h) {
    if (h == vm->hilo_main || h->pila_max <= h->stack_base) return;
    uint32_t u = h->pila_max - h->stack_base, tot = h->stack_top - h->stack_base;
    if (vm->pila_peor_total == 0 || (uint64_t) u * vm->pila_peor_total > (uint64_t) vm->pila_peor_usada * tot) {
        vm->pila_peor_usada = u;
        vm->pila_peor_total = tot;
    }
}

static void hilo_devolver(bpvm_t* vm, bpvm_thread_t* h) {
    bpvm_hilo_apuntar_pila(vm, h);
    /* (V8 #740: su pila no se devuelve aquí: es un bloque del heap, y la barre el GC; la del main es fija) */
    bpvm_free(h->eh_stack);
    bpvm_free(h->linea);
    bpvm_free(h);
}

int bpvm_hilos_recoger(bpvm_t* vm) {
    int n = 0;
    for (;;) {
        bpvm_thread_t* fuera = NULL;
        for (bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h))
            if (puede_salir(vm, h)) { fuera = h; break; }
        if (fuera == NULL) return n;
        cola_sacar(fuera);   /* uno por vuelta: sacar mientras se recorre rompería la vuelta */
        hilo_devolver(vm, fuera);
        n++;
    }
}

int bpvm_hilos_crear_main(bpvm_t* vm, uint32_t stack_base) {
    bpvm_thread_t* m = (bpvm_thread_t*) bpvm_calloc(1, sizeof(bpvm_thread_t));
    if (m == NULL) return -1;
    m->id = 0;
    /* V8 #740 — la región fija de pila, ENTERA: las de los hilos ya no viven aquí (salen del heap). Por defecto mide
     * BPVM_MAIN_STACK_BYTES (bpvm_stack_region_bytes), lo mismo que miVM; la clave `stack=` del ENV la cambia. */
    m->stack_base = stack_base;
    m->stack_top  = (uint32_t) vm->memory_size;
    m->stack_cap  = m->stack_top - stack_base;
    m->sp = m->stack_base;
    m->bp = m->stack_base;
    m->status = BPVM_THREAD_RUNNABLE;
    m->blocked_on_mutex = -1;
    m->blocked_on_join = -1;
    m->sched_owner = -1;
    m->ev_depth = 0;          /* H5.c: sin handler de evento en curso */
    m->ev_post_mortem = -1;   /* #342: vivo; la deuda se calcula al morir */
    vm->hilo_main = m;        /* V7/H12, etapa 2a.2: a la cola entra al colgar el programa (bpvm_hilos_meter_main) */
    vm->sig_tid = 1;
    vm->hilos_max = 0;
    return 0;
}

/* V7/H12, etapa 2a.2 — el main, a la cola, al COLGAR el programa (bpvm_cola_colgar, con el cerrojo de la cola cogido):
 * desde aquí, la cola es de `vm` hasta que se destruya. Antes de prepararlo: la preparación reserva memoria (las
 * excepciones prefabricadas, con el main de ancla) y el GC busca sus raíces en la cola. -1 si la cola es de otro
 * programa —o de éste, otra vez—: un programa a la vez. */
int bpvm_hilos_meter_main(bpvm_t* vm) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (q->dueno != NULL || q->punto != NULL || vm->hilo_main == NULL) return -1;
    q->dueno = vm;
    q->en_cola = 0;
    cola_meter(vm, vm->hilo_main);
    return 0;
}

/* Todas fuera (bpvm_destroy, con los ejecutores de natives ya terminados: hasta entonces apuntan a sus hilos). Si la
 * cola es de `vm`, se queda VACÍA —la referencia, a NULL— para el siguiente programa; si no se llegó a colgar, su main
 * no estaba en ninguna cola y se devuelve solo. */
void bpvm_hilos_liberar(bpvm_t* vm) {
    bpvm_cola_t* q = &bpvm_la_cola;
    bpvm_thread_t* c = NULL;
    int main_dentro = (vm->hilo_main != NULL && vm->hilo_main->next != NULL);
    if (q->iniciada) bpvm_platform_mutex_lock(&q->vm_lock);
    if (q->dueno == vm) {
        if (q->programa == vm) {
            /* no debe pasar: se destruye un programa publicado, que un run puede estar ejecutando */
            bpvm_diag_urgente("[bpvm] se destruye un programa que sigue publicado en la cola (H12): se descuelga");
            q->programa = NULL;
            vm->smp = NULL;
        }
        c = q->punto;
        q->punto = NULL;
        q->dueno = NULL;
        q->en_cola = 0;
    }
    if (q->iniciada) bpvm_platform_mutex_unlock(&q->vm_lock);
    if (c != NULL) {
        bpvm_thread_t* h = c;
        do {                                  /* una vuelta, guardando el siguiente antes de devolver */
            bpvm_thread_t* sig = h->next;
            bpvm_free(h->eh_stack);
            bpvm_free(h->linea);
            bpvm_free(h);
            h = sig;
        } while (h != c);
    }
    if (!main_dentro && vm->hilo_main != NULL) {
        bpvm_free(vm->hilo_main->eh_stack);
        bpvm_free(vm->hilo_main->linea);
        bpvm_free(vm->hilo_main);
    }
    vm->hilo_main = NULL;
}

int bpvm_thread_spawn(bpvm_t* vm, bpref_t thread_ref) {
    if (bpref_is_null(thread_ref)) return -4;

    /* run() es la ranura 0 de la vtable (Core.Thread la declara la primera) y se resuelve como
     * cualquier llamada virtual, SUBIENDO por la cadena. #530 (V7): una clase que hereda run() de
     * un padre de OTRO módulo lleva -1 en su ranura («heredada»); antes eso era «no soportado F4
     * v1» y el hilo no arrancaba (y miVM saltaba a cs - 1). Se resuelve ANTES de reservar nada:
     * si no se puede, no hay nada que deshacer. El descriptor vive en los datos del módulo, no en
     * el heap: su dirección no cambia. -2 = run() no resoluble (el builtin lo lanza con nombre). */
    uint32_t class_ptr = (uint32_t) bpvm_read_i32_be(vm->memory + bpref_deref(vm, thread_ref));
    int32_t  off = -1;
    uint32_t target_cs = 0;
    if (bpvm_vt_resolver(vm, class_ptr, 0u, &off, &target_cs) != 0) return -2;

    /* H2 — crear un hilo toca la cola y el heap (su pila): todo estado compartido. Bajo `vm_lock` en SMP; no-op con un
     * worker. El marco inicial (thread_ref, sp/bp/cs/pc, RUNNABLE) también va dentro, y el hilo entra en la cola el
     * ÚLTIMO, ya montado: el planificador no ve nunca uno a medio crear. */
    bpvm_smp_lock(vm);

    /* V7/H7 #604 — la pila que PIDE el hilo: el campo 1 del Thread (Thread(stackSize)); 0 = la de siempre, como en
     * miVM. Su tope es lo pedido: el muro de #605 salta en el mismo sitio en las dos VMs. */
    int32_t  pedido = bpvm_read_i32_be(vm->memory + bpref_deref(vm, thread_ref) + 4 + 1 * 4);
    uint32_t tam    = (pedido > 0) ? (uint32_t) pedido : BPVM_THREAD_STACK_BYTES;

    /* V8 #740 (29-oct) — LA PILA, DEL HEAP (Eduardo: «Los hilos BP no podrian tomar la RAM del heap?»): un bloque como
     * el de un `new`, sin handle, reservado BAJO el cerrojo —el que ya tiene cogido una reserva cualquiera: si colecta,
     * gc_stw lo suelta mientras espera, como siempre—, y colgado de su hilo antes de soltarlo: sin eso, un GC de otro
     * worker entre medias la barrería. El GC la marca mientras el hilo no acabe y la barre después (gc_mark_phase); el
     * compactador la deja en su sitio, como a todo bloque sin handle. Reservar puede colectar: el Thread sigue en la pila
     * de quien llama (BUILTIN_THREAD_START no lo saca hasta después). Su struct, con malloc; si no hay, la pila se queda
     * sin dueño y la barre el GC siguiente. Sin tope de hilos: el límite es la memoria. */
    uint32_t stack_base = bpvm_heap_reservar_pila(vm, tam);
    if (stack_base == 0u) {
        bpvm_smp_unlock(vm);
        return -3;                                             /* no cabe su pila */
    }
    bpvm_thread_t* tc = (bpvm_thread_t*) bpvm_calloc(1, sizeof(bpvm_thread_t));
    if (tc == NULL) {
        bpvm_smp_unlock(vm);
        return -1;                                             /* no hay memoria para su struct */
    }
    int32_t tid = vm->sig_tid++;                               /* el handle: no se reutiliza nunca */

    /* Layout inicial del frame: simulamos un CALL desde un caller
     * imaginario que ya pusheó `this` (thread_ref) en su stack.
     * - sp inicia en stack_base.
     * - Push `this` = thread_ref (arg para run).
     * - Push saved pc/bp/cs FALSOS apuntando a memory[0] (HALT/
     *   THREAD_EXIT sentinela). El intérprete tras RET de run hará
     *   sp -= 4 (ret), restaura cs/bp/pc desde los falsos, y luego
     *   ejecuta mem[0] = THREAD_EXIT que termina este thread sin
     *   tumbar la VM.
     * - bp = sp (justo tras los saves).
     *
     * El primer opcode que ejecutaremos es el INVOKE_VIRTUAL slot 0
     * (Thread.run). Eso ya hace push de pc/bp/cs reales y entra al
     * cuerpo de run. Pero antes hay que tener `this` en el stack.
     */
    /* (calloc: lo demás nace a cero —la pila de manejadores, el ancla del GC, el paso a paso, la línea de `print`, la
     * native, la marca de pila—.) */
    tc->id = tid;
    tc->stack_base = stack_base;
    tc->stack_top  = stack_base + tam;
    tc->stack_cap  = tam;
    tc->blocked_on_mutex = -1;
    tc->blocked_on_join = -1;
    tc->wake_at_ms = 0;
    tc->thread_ref_heap = thread_ref;
    tc->last_debug_line = 0;   /* #139: trigger en la primera línea vista */
    tc->ev_depth = 0;          /* H5.c: sin handler de evento en curso */
    tc->ev_post_mortem = -1;   /* #342: vivo; la deuda se calcula al morir */
    tc->sched_owner = -1;      /* H2: tc libre para ser pickeado */

    /* Guarda el tid en field[0] del Thread BP (convención del frontend
     * Java: el constructor de Thread sintetizado deja field[0]=0 hasta
     * que se llama start()). Esto es lo que __threadJoin lee para
     * encontrar el tc target. V4: thread_ref es un HANDLE → deref para
     * tocar memory[] (thread_ref_heap arriba se queda como REF: es el
     * ancla del GC y el `this` de run()). */
    uint32_t ta = bpref_deref(vm, thread_ref);
    bpvm_write_i32_be(vm->memory + ta + 4 + 0 * 4, (int32_t) tid);

    /* Frame inicial. Tiene que ser IGUAL al que monta OP_INVOKE_VIRTUAL, porque
     * run() es un método corriente y su código lo da por hecho: el emisor le
     * pone `GET_LOCAL_L -20`, o sea una carga de 8 BYTES en bp-20.
     *   [sb+0..7]   thisRef  (BPVM_REF_SIZE — bpref_store, NO write_i32)
     *   [sb+8]      saved PC = 0 (sentinela mem[0]=THREAD_EXIT cuando run() RET)
     *   [sb+12]     saved BP = sb
     *   [sb+16]     saved CS = 0
     *   bp = sb + 20, sp = sb + 20.
     *
     * Esto valía 16 y la ref se escribía en 4 bytes: era el layout de ANTES del
     * ensanchado de ref 4→8B, que aquí se quedó sin migrar. Con bp = sb+16,
     * run() leía sus 8 bytes en sb-4: la mitad baja (idx) salía bien y la ALTA
     * —la generación— venía de la memoria de debajo de su propia pila, casi
     * siempre 0. Handle con gen 0 → coincide de casualidad mientras el slot no
     * se haya reciclado, y da use-after-free en cuanto se recicla. De ahí que
     * el mismo programa fuera o no fuera según lo ejecutado antes. */
    uint32_t sb = stack_base;
    bpref_store(vm, sb, thread_ref);
    bpvm_write_i32_be(vm->memory + sb + 8,  0);
    bpvm_write_i32_be(vm->memory + sb + 12, (int32_t) sb);
    bpvm_write_i32_be(vm->memory + sb + 16, 0);
    tc->sp = sb + 20;
    tc->bp = sb + 20;
    tc->cs = target_cs;
    tc->pc = bpvm_cb_for_cs(vm, tc, target_cs) + (uint32_t) off;   /* H3.c: vtable → cb */
    tc->status = BPVM_THREAD_RUNNABLE;
    cola_meter(vm, tc);   /* H12, etapa 2: ya montado, a la cola */
    /* Aviso al scheduler: hay un tc nuevo RUNNABLE. */
    if (vm->smp) {
        bpvm_platform_cond_broadcast(&vm->smp->sched_cond);
    }
    bpvm_smp_unlock(vm);
    return tid;
}
