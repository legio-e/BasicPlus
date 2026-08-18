/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm.vm;

/**
 * Intérprete de bytecode .mod con GC mark-and-sweep conservativo.
 *
 * Layout del heap (cada objeto):
 *   [addr+0] tag       (4 bytes)
 *                      bit 31 = MARK
 *                      bit 30 = FREE
 *                      bits 24-29 = TYPE (6 bits)
 *                      bits  0-23 = reservado (class id futuro)
 *   [addr+4] length    (4 bytes; user_ref apunta AQUÍ)
 *   [addr+8] payload   (length * elem_size bytes)
 *
 *   Bloques libres: bit FREE puesto en tag, length = total_bytes del bloque
 *   (incluida la cabecera de 8 bytes), [addr+8] = puntero al siguiente bloque
 *   libre (free list enlazada).
 *
 * Allocator: first-fit en la free list, sino bump desde heapNext.
 * Si no cabe, dispara GC y reintenta. Si sigue sin caber, RuntimeException.
 *
 * Mark conservativo: escanea pila [STACK_BASE, SP) y data blocks de todos
 * los módulos. Para cada slot de 4 bytes que coincida con (validObjectAddr+4),
 * marca el objeto. Para arrays de refs (TYPE_ARRAY_REF), sigue las refs.
 *
 * Los arrays en data block / locales NO tienen cabecera GC (ref apunta
 * directamente a length); el GC los ignora vía el set de validObjects.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.Builtin;
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.gui.GuiBackend;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class VirtualMachine {
    /** Tamaño total de `memory[]`. Configurable vía VmConfig. */
    public static final int DEFAULT_MEMORY_SIZE = 512 * 1024;
    /** Offset donde empiezan los stacks. Configurable vía VmConfig. */
    public static final int DEFAULT_STACK_BASE  = 256 * 1024;

    private final byte[] memory;

    // Layout total:
    //   [0 .. heapStart)         data block (módulos cargados)
    //   [heapStart .. STACK_BASE) heap
    //   [STACK_BASE .. memory.length) regiones de stack (main + workers)
    //
    // STACK_BASE es instance-final ahora (antes static final). Permite
    // configurar el tamaño del heap vs. stacks por VM.
    public final int STACK_BASE;

    // ====================================================================
    // ThreadContext: estado de ejecución por hilo de BP.
    //
    // Cada Thread en BP tiene su propia copia de PC/SP/BP/CS y de la pila
    // de exception-handlers. El heap es compartido. El stack de cada
    // ThreadContext vive en una sub-región del rango [STACK_BASE..memory.end]
    // delimitada por (stackBase, stackTop).
    //
    // Por ahora (Fase 1/2) sólo existe el thread 0 (main). El bucle del
    // intérprete carga los campos en variables locales al entrar y los
    // sincroniza con el ThreadContext al salir (HALT, throw, futuro
    // context switch). Cuando lleguen N threads reales, save/loadContext
    // harán el switch entre cada quantum.
    // ====================================================================
    /**
     * Estado de un thread BP en el scheduler.
     */
    enum ThreadStatus {
        RUNNABLE,       // listo para correr, en la cola
        RUNNING,        // ejecutándose en este worker
        BLOCKED_SLEEP,  // dormido hasta wakeAtMs
        BLOCKED_JOIN,   // esperando que un thread target termine
        BLOCKED_MUTEX,  // esperando que un mutex se libere
        BLOCKED_PROMPT, // esperando promptResponse del cliente IDE (N20)
        TERMINATED      // run() devolvió
    }

    static final class ThreadContext {
        final int id;
        final int stackBase;     // dirección más baja de su región de pila
        final int stackTop;      // dirección máxima (exclusiva)
        int pc, sp, bp, cs;
        boolean running = true;
        ThreadStatus status = ThreadStatus.RUNNABLE;
        /**
         * Señal per-thread: un builtin (yield/sleep/join) O el timer preemptivo
         * ha pedido que el worker que ejecuta ESTE tc abandone el bucle inner
         * y vuelva al scheduler para hacer context switch. Se limpia al cargar
         * el tc de nuevo en un worker.
         * volatile: la fija el timer preemptivo (otro Java thread) y el bucle
         * inner debe verla en la siguiente iteración sin que el JIT la cachée.
         */
        volatile boolean yieldRequested = false;
        // Bloqueos:
        long wakeAtMs;           // para BLOCKED_SLEEP (System.currentTimeMillis() ≥ esto → despertar)
        int joiningTid;          // para BLOCKED_JOIN (id del thread esperado)
        int blockedOnMutexId = -1; // para BLOCKED_MUTEX (id del mutex en VM.mutexes)
        /** Última línea origen reportada al DebugHook (para detectar cambios de línea). */
        int lastDebugLine = 0;
        /**
         * Ancla GC: ref del último heapAlloc todavía no publicado al stack
         * BP. GC lo escanea como root para que el objeto recién alocado no
         * desaparezca entre el `return` de heapAlloc (que libera vmLock) y
         * el `push` posterior del caller (NEW_OBJECT, NEWARRAY, builtins
         * que alocan). Las allocs subsiguientes lo sobreescriben — sólo
         * protege la MÁS RECIENTE, que es la única que podría estar en
         * tránsito. Las anteriores ya están en stack/heap como roots
         * normales (o son basura legítima).
         */
        int allocAnchor = 0;
        /**
         * H5.c — profundidad de handlers de evento inyectados en este thread.
         * &gt;0 = hay un handler CORRIENDO y el drenaje NO inyecta otro. Sin esto,
         * el siguiente punto de planificación puede caer DENTRO del handler y
         * meterle otro encima: el segundo termina antes que el primero y los
         * eventos dejan de atenderse en orden. Un handler corre hasta el final
         * antes de despachar el siguiente, como el EDT de Swing. Un `raise`
         * DESDE un handler sigue valiendo: eso encola, no inyecta.
         */
        int evDepth = 0;
        /**
         * #342 — DEUDA DE EVENTOS AL MORIR. Un thread cuyo `raise` es lo último
         * que hace muere antes de llegar a una frontera de quantum, y su evento
         * se quedaba encolado para un tid muerto: desaparecía SIN UN RUIDO.
         *   -1 = vivo / sin calcular.
         *    N = eventos que tenía encolados EN EL MOMENTO de terminar. Es un
         *        PRESUPUESTO: se salda lo que se debía entonces, no lo que un
         *        handler post-mortem añada después — si no, un programa que se
         *        realimenta no terminaría nunca.
         *    0 = deuda saldada; ya no se resucita.
         */
        int evPostMortem = -1;
        // Lista de threads que esperan a que ÉSTE termine.
        final List<Integer> waiters = new ArrayList<>();
        // Excepciones: estado per-thread.
        int ehHandlerPc = -1;
        int ehSavedSp;
        int ehSavedBp;
        int ehSavedCs;
        int ehExpectedClass;
        final Deque<int[]> handlerStack = new ArrayDeque<>();
        ThreadContext(int id, int stackBase, int stackTop) {
            this.id = id;
            this.stackBase = stackBase;
            this.stackTop  = stackTop;
            this.sp = stackBase;
            this.bp = stackBase;
        }
    }

    private final List<ThreadContext> threads = new ArrayList<>();
    private int currentThreadId = 0;
    private ThreadContext currentThread;

    /**
     * Tamaño por defecto del stack de un Thread BP (en bytes). El usuario
     * puede pedir explícitamente otro valor al construir el Thread.
     */
    public static final int DEFAULT_THREAD_STACK_BYTES = 2 * 1024;

    /**
     * Stack del thread main: más grande que el de los workers porque suele
     * acumular la mayor parte de la recursión del programa.
     */
    public static final int MAIN_STACK_BYTES = 16 * 1024;

    /**
     * Alocador de regiones de stack: punto bajo de la próxima región libre
     * dentro del rango [STACK_BASE..memory.length]. Crece hacia arriba.
     * Cuando un Thread termina, su región se devuelve a `freedStackRegions`.
     */
    private int nextStackBase;   // se inicializa en el constructor (depende de STACK_BASE)
    private final Deque<int[]> freedStackRegions = new ArrayDeque<>();   // {base, top}

    private int[] allocStackRegion(int size) {
        // Primero intenta reusar una región libre del mismo tamaño.
        for (java.util.Iterator<int[]> it = freedStackRegions.iterator(); it.hasNext(); ) {
            int[] r = it.next();
            if (r[1] - r[0] >= size) {
                it.remove();
                return new int[]{r[0], r[0] + size};
            }
        }
        // Asignación incremental.
        int base = nextStackBase;
        int top  = base + size;
        if (top > memory.length) {
            throw new RuntimeException("Sin espacio para stack de " + size + " bytes (nextBase="
                    + base + ", memory.length=" + memory.length + ")");
        }
        nextStackBase = top;
        return new int[]{base, top};
    }

    private void freeStackRegion(int base, int top) {
        freedStackRegions.add(new int[]{base, top});
    }

    /**
     * Cola FIFO de threads en estado RUNNABLE listos para tomar la CPU.
     * Cada worker Java saca el primero de aquí cuando vuelve a buscar trabajo.
     * Toda mutación va bajo {@link #vmLock}.
     */
    private final Deque<Integer> runQueue = new ArrayDeque<>();

    // ====================================================================
    // Sincronización multi-worker (Fase 4b)
    //
    // vmLock cubre todo el estado compartido entre los workers Java:
    //   - runQueue, threads list, nextStackBase, freedStackRegions
    //   - ThreadContext.{status, waiters, wakeAtMs, joiningTid, yieldRequested}
    //   - currentThread / currentThreadId (sólo durante setup)
    //   - HEAP: heapNext, freeListHead, scan/sweep en gc()
    //
    // runOnContext NO lo adquiere — sería un cuello de botella absoluto.
    // En su lugar, las helpers que mutan estado compartido (heapAlloc,
    // freeOwnedObject, gc, spawnThread, blockTcSleep, blockTcJoin,
    // terminateThread) lo adquieren localmente.
    //
    // ATENCIÓN — race conocida (TODO Fase 4c):
    //   gc() lee tc.sp de TODOS los threads para el scan conservativo. Si
    //   otro worker está a media instrucción y aún no ha sincronizado tc.sp,
    //   la región (tc.sp, sp_local_actual) puede contener refs que el GC
    //   no marca, con riesgo de reciclar objetos vivos. La solución
    //   correcta es un safepoint stop-the-world: gc() activa una flag, los
    //   workers la chequean entre opcodes (o en jumps hacia atrás) y se
    //   parquean tras sincronizar tc.sp; gc() ejecuta cuando todos estén
    //   parqueados.
    // ====================================================================
    private final Object vmLock = new Object();

    /**
     * Safepoint para stop-the-world GC (B1 — Fase 4c).
     *
     * Antes existía una race entre GC y bytecode-runner: gc() escanea
     * tc.sp de cada thread, pero un worker mid-instrucción puede tener
     * refs en su stack local SOBRE tc.sp (aún no sincronizadas). El GC
     * no las veía como roots y podía liberar objetos vivos, corrompiendo
     * el heap. El síntoma típico: bytecodes inválidos, "Dirección X no
     * cae en data block", "mutex id inválido" y otros con valores
     * basura.
     *
     * Cómo funciona ahora:
     *   - Cuando un worker quiere correr GC (heapAlloc agotado), bajo
     *     vmLock pone stopTheWorld=true y NOTIFICA a todos los workers.
     *   - Los workers, entre opcodes, observan stopTheWorld; si está
     *     activo sincronizan tc.X y salen del runOnContext con YIELD.
     *   - La WorkerLoop, al volver al scheduler, ve stopTheWorld y
     *     parquea (vmLock.wait) en lugar de coger otro thread.
     *   - El worker que pidió GC espera a que no quede ningún tc en
     *     estado RUNNING (excepto él mismo) y entonces corre gcLocked.
     *   - Tras GC: stopTheWorld=false, notifyAll. Los workers despiertan
     *     y vuelven al scheduler normal.
     *
     * Es VOLATILE para que los workers lo vean sin tomar vmLock.
     */
    private volatile boolean stopTheWorld = false;

    /** P-run-stop (#257) — KILL: termina TODO el programa ordenadamente en
     *  el siguiente safepoint (el mismo check por-opcode que stopTheWorld).
     *  Lo levanta el DebugServer al recibir KILL del IDE. A diferencia de
     *  la PAUSA (reanudable) o del RESET, el kill termina la ejecución;
     *  el proceso/daemon reporta EXITED con exitCode 130. */
    private volatile boolean killRequested = false;

    /** Pide terminar la ejecución; despierta a los workers parqueados para
     *  que el shutdown no espere a un sleep/join largo. Seguro desde
     *  cualquier thread (el del DebugServer incluido). */
    public void requestKill() {
        killRequested = true;
        synchronized (vmLock) { vmLock.notifyAll(); }
    }

    public boolean isKillRequested() { return killRequested; }

    /**
     * True mientras un worker está orquestando un GC stop-the-world. Otros
     * workers que entren a heapAlloc deben esperar a que se ponga a false
     * en vez de iniciar SU PROPIO dance (que llevaría a deadlocks mutuos).
     * Guardado bajo vmLock.
     */
    private boolean gcInProgress = false;

    /**
     * tc.ids cuyos Java workers están bloqueados en vmLock.wait() dentro
     * de heapAlloc esperando a que el mundo se pare. Aunque su BP status
     * sigue siendo RUNNING (no han salido de runOnContext), su Java
     * thread no está ejecutando bytecode — está parqueado. anyOtherThreadRunning
     * los excluye para no entrar en deadlock cuando dos workers piden GC
     * a la vez.
     */
    private final java.util.HashSet<Integer> parkedInHeapAlloc = new java.util.HashSet<>();

    /**
     * Cada Java worker setea esta ThreadLocal al tc que está procesando
     * antes de entrar en runOnContext (lo limpia tras volver). Permite a
     * heapAlloc, llamado desde dentro de un opcode, saber qué tc.id es
     * "yo" para registrarse en parkedInHeapAlloc al entrar en wait().
     */
    private final ThreadLocal<ThreadContext> currentTcLocal = new ThreadLocal<>();
    private volatile boolean vmShutdown = false;
    /**
     * Número de workers Java en paralelo (cores físicos simulados). Default 1:
     * un único flujo de ejecución BP, igual que la VM-C del device (single-core).
     * Así la VM-Java es SEGURA de fábrica — la race B1 (corrupción worker↔worker)
     * necesita paralelismo REAL (workers≥2) y con 1 worker no aparece (medido:
     * w1=0% siempre). workers>1 activa SMP (opt-in vía --workers=N): da speedup
     * pero arrastra B1, cuyo fix se acopla al dual-core RP2350 (v2). Ver
     * docs/PENDIENTES.md §B1.
     */
    private int numWorkers = 1;
    public void setNumWorkers(int n) { this.numWorkers = Math.max(1, n); }
    public int getNumWorkers() { return numWorkers; }

    // ============================================================
    // H5.c — COLA DE EVENTOS (espejo de events.c en la VM-C)
    // ============================================================
    /**
     * Un `raise` pendiente. No se llama al handler en el raise: se encola, y
     * el scheduler le inyecta el frame al thread destino ENTRE QUANTA. Desde
     * ahí el handler es código BP corriente (puede ceder, bloquear y lanzar).
     *
     * `masks` la rellena el COMPILADOR: bits 0-3 = el argumento es referencia
     * (para el GC), bits 8-11 = ocupa 8 bytes (para montar el frame). La VM no
     * adivina ninguna de las dos cosas.
     */
    private static final class PendingEvent {
        final long recv; final int dest; final int tid; final int masks; final long[] args;
        PendingEvent(long recv, int dest, int tid, int masks, long[] args) {
            this.recv = recv; this.dest = dest; this.tid = tid;
            this.masks = masks; this.args = args;
        }
    }
    private static final int EVENT_MAX_ARGS   = 4;
    private static final int EVENT_QUEUE_CAP  = 16;
    private final java.util.ArrayDeque<PendingEvent> eventQueue = new java.util.ArrayDeque<>();

    /**
     * Quantum del scheduler preemptivo en milisegundos. Cada {@code quantumMs}
     * el timer marca yieldRequested=true en todos los tcs RUNNING, forzando
     * un context switch. 0 desactiva el timer (cooperativo puro).
     */
    private int quantumMs = 10;
    public void setQuantumMs(int ms) { this.quantumMs = Math.max(0, ms); }
    public int getQuantumMs() { return quantumMs; }

    /** Timer preemptivo: ScheduledExecutorService daemon que dispara cada quantum. */
    private java.util.concurrent.ScheduledExecutorService preemptTimer;

    /**
     * Estado interno de un Mutex BP. La clase BP {@code Mutex} guarda en su
     * field {@code __mid} (slot 0) el índice dentro de esta lista. Los
     * builtins __mutexLock/__mutexUnlock operan sobre la JavaMutex
     * correspondiente bajo vmLock.
     */
    private static final class JavaMutex {
        // ATENCIÓN: -1 = libre. NO podemos usar 0 como sentinela porque main
        // tiene tid = 0 y sería un owner legítimo. Toda comprobación abajo
        // debe usar este valor explícito.
        static final int FREE = -1;
        int ownerTid = FREE;
        final List<Integer> waiters = new ArrayList<>();
    }
    private final List<JavaMutex> mutexes = new ArrayList<>();

    /** N20 — Prompts en vuelo desde el builtin PROMPT.
     *  Key = requestId asignado por la VM. Value = tc bloqueado.
     *  Cuando el cliente IDE manda promptResponse, la VM lo busca aquí,
     *  aloja el VM string con `values`, lo empuja al stack del tc y lo
     *  despierta (status RUNNABLE + runQueue). */
    private final java.util.Map<Long, ThreadContext> pendingPrompts =
            new java.util.concurrent.ConcurrentHashMap<>();
    private final java.util.concurrent.atomic.AtomicLong nextPromptId =
            new java.util.concurrent.atomic.AtomicLong(1);

    /** H11 (#241) — sockets TCP del módulo Net. handle (int ≥ 1) → Socket.
     *  Tabla concurrente: los workers BP son threads reales y dos pueden
     *  abrir/cerrar a la vez. Espejo de la tabla de handles de net_host.c
     *  en la VM-C (BP nunca ve el fd/SOCKET del SO). */
    // Backend grafico (V3 GUI, H3). Sin estado hasta el primer __gui*; la
    // ventana Swing se abre en __guiRun. Inofensivo si el programa no usa GUI.
    private final GuiBackend gui = new GuiBackend();

    // H3.4 — cache de la entrada de la funcion BP Gui.__guiDispatch (resuelta
    // por nombre la 1a vez). -2 = sin resolver; -1 = no encontrada (Gui no cargado).
    private int guiDispatchPc = -2;
    private int guiDispatchCs = 0;
    private int guiDispatchChangePc = -2;   // V3/H6 — dispatcher de onChange (resuelto 1 vez)
    private int guiDispatchChangeCs = 0;

    private final java.util.Map<Integer, java.net.Socket> netSockets =
            new java.util.concurrent.ConcurrentHashMap<>();
    private final java.util.concurrent.atomic.AtomicInteger netNextHandle =
            new java.util.concurrent.atomic.AtomicInteger(1);

    /** Sender hacia el cliente IDE. Cableado por Main cuando hay --listen.
     *  Si está a null, el builtin PROMPT lanza RuntimeError BP (modo
     *  headless: no hay supervisor que muestre el form). */
    private volatile edu.bpgenvm.vm.debug.PromptSender promptSender;

    public void setPromptSender(edu.bpgenvm.vm.debug.PromptSender s) {
        this.promptSender = s;
    }

    /** Llamado por el DebugServer cuando llega un promptResponse del IDE.
     *  Aloja un VM string con `valuesJson` y lo empuja al stack del thread
     *  bloqueado, luego lo despierta. Si el requestId no está en el mapa
     *  (timeout, doble respuesta, etc.), se ignora silenciosamente. */
    public void deliverPromptResponse(long requestId, String valuesJson) {
        ThreadContext tc = pendingPrompts.remove(requestId);
        if (tc == null) return;
        synchronized (vmLock) {
            int ref = (int) allocVmString(valuesJson == null ? "" : valuesJson);
            writeInt32(tc.sp, ref);
            tc.sp += 4;
            tc.status = ThreadStatus.RUNNABLE;
            tc.blockedOnMutexId = -1;
            runQueue.addLast(tc.id);
            vmLock.notifyAll();
        }
    }

    /**
     * Crea un nuevo ThreadContext con el stackSize indicado (0 = default).
     * El thread arranca en estado RUNNABLE y queda añadido a la runQueue.
     * Debe llamarse bajo vmLock (o desde setup mono-thread).
     */
    int spawnThread(int stackSize) {
        if (stackSize <= 0) stackSize = DEFAULT_THREAD_STACK_BYTES;
        int[] region = allocStackRegion(stackSize);
        int newId = threads.size();
        ThreadContext nt = new ThreadContext(newId, region[0], region[1]);
        nt.status = ThreadStatus.RUNNABLE;
        threads.add(nt);
        runQueue.addLast(newId);
        return newId;
    }

    /**
     * Versión NO-BLOQUEANTE de scheduler: devuelve el siguiente ThreadContext
     * RUNNABLE o {@code null} si no hay ninguno listo AHORA. Despierta sleepers
     * cuyo wakeAt ya pasó. Debe llamarse bajo vmLock.
     */
    private ThreadContext pickNextRunnableTc() {
        long now = System.currentTimeMillis();
        for (ThreadContext t : threads) {
            if (t.status == ThreadStatus.BLOCKED_SLEEP && now >= t.wakeAtMs) {
                t.status = ThreadStatus.RUNNABLE;
                runQueue.addLast(t.id);
            }
        }
        while (!runQueue.isEmpty()) {
            int tid = runQueue.pollFirst();
            ThreadContext t = threads.get(tid);
            if (t.status == ThreadStatus.RUNNABLE) return t;
        }
        return null;
    }

    /** Más próximo wakeAt entre sleepers; Long.MAX_VALUE si no hay. Bajo vmLock. */
    private long earliestSleepWakeMs() {
        long e = Long.MAX_VALUE;
        for (ThreadContext t : threads) {
            if (t.status == ThreadStatus.BLOCKED_SLEEP && t.wakeAtMs < e) {
                e = t.wakeAtMs;
            }
        }
        return e;
    }

    /** ¿Queda algún thread BP no terminado? Bajo vmLock. */
    private boolean anyThreadAlive() {
        for (ThreadContext t : threads) {
            if (t.status != ThreadStatus.TERMINATED) return true;
        }
        return false;
    }

    /**
     * Marca el ThreadContext recibido como TERMINATED, libera su región de
     * stack (si no es main) y despierta a sus joiners. Debe llamarse bajo vmLock.
     */
    /**
     * #342 — ¿este thread deja eventos SUYOS sin atender? Entonces todavía no
     * está muerto: vuelve a RUNNABLE para saldarlos. Devuelve true si lo ha
     * resucitado.
     *
     * Aquí (y no en el scheduler, como en la VM-C) porque en miVM terminar un
     * thread LIBERA su región de pila: si se resucitara después, la inyección
     * escribiría en una pila que otro `Thread.start()` puede haber reutilizado.
     * Mismo comportamiento observable, distinto sitio — el sustrato manda.
     *
     * Debe llamarse bajo vmLock.
     */
    private boolean reviveForPendingEvents(ThreadContext t) {
        if (t.evPostMortem == 0) return false;          // ya saldó su deuda
        int n = 0;
        for (PendingEvent e : eventQueue) if (e.tid == t.id) n++;
        if (n == 0) { t.evPostMortem = 0; return false; }
        if (t.evPostMortem < 0) t.evPostMortem = n;     // deuda al morir
        t.status = ThreadStatus.RUNNABLE;
        runQueue.addLast(t.id);
        return true;
    }

    private void terminateThread(ThreadContext t) {
        if (reviveForPendingEvents(t)) return;   // #342 — aún debe eventos
        t.status = ThreadStatus.TERMINATED;
        if (t.id != 0) {
            freeStackRegion(t.stackBase, t.stackTop);
        }
        for (Integer wid : t.waiters) {
            ThreadContext w = threads.get(wid);
            if (w.status == ThreadStatus.BLOCKED_JOIN) {
                w.status = ThreadStatus.RUNNABLE;
                runQueue.addLast(wid);
            }
        }
        t.waiters.clear();
    }

    /**
     * Libera todos los mutexes que el thread `tid` tenía tomados, transfiriendo
     * ownership al primer waiter de cada uno (igual que un MUTEX_UNLOCK normal).
     * Se llama cuando un thread muere por fallo (BpThreadFault), para no dejar
     * a otros threads colgados esperando un mutex de un thread que ya no existe.
     * Llamar SIEMPRE bajo `synchronized (vmLock)`.
     */
    private void releaseMutexesOwnedBy(int tid) {
        for (JavaMutex jm : mutexes) {
            if (jm.ownerTid != tid) continue;
            if (jm.waiters.isEmpty()) {
                jm.ownerTid = JavaMutex.FREE;
            } else {
                int nextTid = jm.waiters.remove(0);
                jm.ownerTid = nextTid;
                ThreadContext nt = threads.get(nextTid);
                if (nt != null) {
                    nt.status = ThreadStatus.RUNNABLE;
                    nt.blockedOnMutexId = -1;
                    runQueue.addLast(nextTid);
                }
            }
        }
    }

    /**
     * @deprecated wrapper compatibilidad mono-thread — usar terminateThread(tc).
     */
    @SuppressWarnings("unused")
    @Deprecated
    private void terminateCurrentThread() {
        synchronized (vmLock) {
            terminateThread(currentThread);
        }
    }

    /**
     * Variante per-tc: marca el ThreadContext recibido como BLOCKED_SLEEP
     * (o RUNNABLE si ms≤0). Adquiere vmLock; safe para multi-worker.
     */
    private void blockTcSleep(ThreadContext tc, int ms) {
        synchronized (vmLock) {
            if (ms <= 0) {
                tc.status = ThreadStatus.RUNNABLE;
                runQueue.addLast(tc.id);
            } else {
                tc.status = ThreadStatus.BLOCKED_SLEEP;
                tc.wakeAtMs = System.currentTimeMillis() + ms;
            }
            vmLock.notifyAll();
        }
    }

    /**
     * Bloquea el thread actual esperando que el thread `targetTid` termine.
     * Si el target ya está TERMINATED, el caller no se bloquea (return).
     */
    @SuppressWarnings("unused")
    @Deprecated
    private boolean blockCurrentJoin(int targetTid) {
        return blockTcJoin(currentThread, targetTid);
    }

    /**
     * Variante per-tc: marca tc como BLOCKED_JOIN esperando que termine el
     * thread {@code targetTid}. Adquiere vmLock; safe para multi-worker.
     * Devuelve true si el caller debe cederse efectivamente la CPU; false si
     * el target ya estaba TERMINATED.
     */
    private boolean blockTcJoin(ThreadContext tc, int targetTid) {
        synchronized (vmLock) {
            if (targetTid < 0 || targetTid >= threads.size()) {
                throw new RuntimeException("join: id de thread inválido " + targetTid);
            }
            ThreadContext target = threads.get(targetTid);
            if (target.status == ThreadStatus.TERMINATED) return false;
            tc.status = ThreadStatus.BLOCKED_JOIN;
            tc.joiningTid = targetTid;
            target.waiters.add(tc.id);
            vmLock.notifyAll();
            return true;
        }
    }

    // Estos campos sólo se usan para la API legacy (setPC/setCS antes de
    // arrancar run(), y para que herramientas externas puedan inspeccionar
    // el estado final). El intérprete vivo (runOnContext) NUNCA los lee
    // ni los escribe: opera sólo sobre ThreadContext.
    private int PC = 0;
    private int CS = 0;
    private int SP;   // = STACK_BASE, en el constructor
    private int BP;   // = STACK_BASE, en el constructor

    // Heap y free list (compartidos entre threads).
    private int heapStart;   // = STACK_BASE, en el constructor
    private int heapNext;    // = STACK_BASE, en el constructor
    private int freeListHead = 0;          // 0 = lista vacía (no hay objetos en addr 0)
    // H3: GC proactivo + retorno de memoria.
    //   lastGcHeapNext  = heapNext en el último GC (mide el crecimiento de bump).
    //   gcBumpThreshold = bump máx. desde el último GC antes de colectar
    //                     proactivamente (evita over-commit; ~1/8 del heap).
    private int lastGcHeapNext = 0;
    private int gcBumpThreshold = 1 << 16;
    // V4 — migración a handles: el GC se SUSPENDE mientras se monta la tabla de
    // handles (pasos 2-5). Es la parte más delicada de hacer handle-aware y su
    // ejecución alteraría el heap durante la verificación; se reactiva ya-preciso
    // en el paso 6 (GC vía tabla). owner-free / FREE_REF (determinista) siguen ON.
    // Paso 6 — REACTIVADO: el GC es handle-aware (mark traza por la tabla + field-bitmap;
    // el sweep recicla los slots de lo recolectado vía handleKillIdx → contrato B también
    // para el GC). La reclamación-diferida-a-safepoint (SMP) se pliega al paso 7.
    private boolean gcSuspended = false;

    // V4 — TABLA DE HANDLES (paso 2b). Una referencia BP pasa a ser un HANDLE =
    // índice en esta tabla; refDeref(handle) → dirección física del objeto.
    // Modo neutro: monotónica (sin reciclaje ni chequeo de generación aún; eso
    // llega en pasos 3-4). slot 0 = null. Con el GC suspendido no se recicla, y
    // los samples cortos no agotan la tabla (crece por duplicación si hiciera falta).
    private int[] handleAddr = new int[4096];
    // GENERACIÓN por índice (contrato B). 0 = slot fresco; se incrementa en cada
    // owner-free. El handle 64b lleva la gen que tenía el slot al mintearse; el deref
    // valida gen(handle)==gen(slot) → un handle a un slot RECICLADO (gen bumpeada) no
    // matchea y grita "objeto eliminado".
    private int[] handleGen  = new int[4096];
    // Paso 4c — FREE-LIST de slots reciclables (pila LIFO). owner-free empuja el slot;
    // handleRegister lo reusa con su gen ya bumpeada. Reclamación INMEDIATA (segura en
    // 1 worker); la diferida-a-safepoint (SMP/ARM) se pliega al paso 6 (GC/STW).
    private int[] handleFreeList = new int[256];
    private int   handleFreeTop  = 0;
    private int   handleNext = 1;   // 0 reservado para null
    // #430 — LA MARCA: los últimos HANDLE_MARCA slots son zona marcada; repartir uno
    // arma handlePressure y la puerta de heapAlloc colecta (o crece). El margen cubre
    // los registros en vuelo entre un alloc y su register. Espejo de heap.c.
    private static final int HANDLE_MARCA = 64;
    private boolean handlePressure = false;
    // #430 — la excepción PREFABRICADA del OOM: se construye en el prólogo del run,
    // cuando construir es gratis, y se lanza cuando construir el error ya no cabe.
    // Raíz explícita del GC (scanRoots). 0 = no hay.
    private long oomExc = 0;

    // H1 — guarda anti-recursión del OOM: al quedarse sin heap, la alocación lanza un
    // RuntimeError BP ATRAPABLE, pero CONSTRUIR ese objeto-excepción vuelve a alocar; si
    // eso también OOMea, sin esta bandera recurriríamos hasta el StackOverflow. Con la
    // bandera, el 2º OOM cae al fallback incatchable (BpThreadFault). Se resetea en cada
    // alocación con éxito (la de la propia excepción → deja la bandera limpia al lanzar).
    private boolean throwingOom = false;

    // Paso 7c — A1 publicación segura: la VM-C pone RELEASE/ACQUIRE explícitos en el slot
    // (bpref_deref/handle_register, atómicos C11) porque VA a placa ARM/RISC-V. miVM es la
    // VM de HOST (x86, memoria fuerte) y NO se despliega a placa → aquí el RELEASE lo da
    // synchronized(vmLock) al publicar en handleRegister (monitor-exit = release) y el
    // ACQUIRE del lector es gratis en x86. El dual explícito (VarHandle get/setAcquire) es
    // Java 9+; el proyecto es Java 8 → se deja documentado (solo importaría en un host
    // ARM multi-worker, fuera del target).

    // Tag bits del header del objeto
    private static final int TAG_MARK_BIT  = 0x80000000;
    private static final int TAG_FREE_BIT  = 0x40000000;
    private static final int TAG_TYPE_MASK = 0x3F000000; // bits 24-29
    private static final int TAG_TYPE_SHIFT = 24;

    // Tipos almacenables en heap
    private static final int TYPE_ARRAY_I8  = 0;
    private static final int TYPE_ARRAY_I16 = 1;
    private static final int TYPE_ARRAY_I32 = 2;   // también f32 (4 bytes, opaco)
    private static final int TYPE_ARRAY_REF = 3;   // futuro: array de refs a objetos
    private static final int TYPE_OBJECT    = 4;   // instancia de clase; user_ref → class_ptr
    private static final int TYPE_ARRAY_I64 = 5;   // H1.2 (V2): array de long, 8 bytes/elem

    /*
     * Layout del class descriptor (en data block, apuntado por header+4 del objeto):
     *   [+0]   num_fields    (u16)
     *   [+2]   num_methods   (u16)
     *   [+4]   bitmap_words  (u16)   = ceil(num_fields/32)
     *   [+6]   _pad          (u16)
     *   [+8]   parent_offset (i32)   CS-relative al descriptor del padre (0 = sin padre)
     *   [+12]  field_bitmap  (bw*4)   bit k=1 ⇒ field[k] es ref (GC trace)
     *   [+12 + bw*4]  owner_bitmap (bw*4)   bit k=1 ⇒ field[k] es owner (FREE recursivo)
     *   [+12 + 2*bw*4]  vtable (num_methods * 4 bytes, offsets relativos a code)
     */
    private static final int CLS_OFF_NUM_FIELDS   = 0;
    private static final int CLS_OFF_NUM_METHODS  = 2;
    private static final int CLS_OFF_BITMAP_WORDS = 4;
    private static final int CLS_OFF_PARENT_OFF   = 8;
    private static final int CLS_OFF_FIELD_BITMAP = 12;
    /** @deprecated use CLS_OFF_FIELD_BITMAP — alias retro-compat (field_bitmap empieza aquí). */
    private static final int CLS_OFF_BITMAP_BASE  = CLS_OFF_FIELD_BITMAP;

    // Mínimo bloque libre: 8 bytes cabecera + 4 bytes para el next pointer
    private static final int MIN_FREE_BLOCK = 12;

    // Cabecera de objeto en el heap (tag + length = 8 bytes)
    private static final int OBJ_HEADER_SIZE = 8;

    private ModuleManager moduleManager;
    private boolean tracing = false;
    private java.io.BufferedReader stdinReader;

    // ====================================================================
    // Exception handler register + handler stack
    //   EH guarda dónde saltar si hay un THROW. La pila apila EHs previos
    //   para soportar try anidados (y se restaura en TRY_END).
    //   ehHandlerPc = -1 ⇒ sin handler activo (un THROW aquí es fatal).
    // ====================================================================
    private int ehHandlerPc     = -1;
    private int ehSavedSp       = 0;
    private int ehSavedBp       = 0;
    private int ehSavedCs       = 0;
    /** Class descriptor esperado por el handler actual (0 = atrapa cualquiera). */
    private int ehExpectedClass = 0;
    /**
     * Pila de exception-handlers del thread actual. Es un alias a
     * {@code currentThread.handlerStack}, no una colección independiente:
     * cuando se hace context switch (futuro multi-thread), {@link #loadContext}
     * reasigna este campo al deque del thread entrante.
     */
    private Deque<int[]> handlerStack;

    /** Listener de eventos del debugger no-pause: ExceptionEvent que la VM
     *  emite cuando un worker BP muere por BpThreadFault no atrapada. Lo
     *  cablea Main cuando arranca con --listen (apunta a
     *  DebugController.emitEvent). Si está a null, los eventos se descartan
     *  (modo headless). */
    private volatile java.util.function.Consumer<edu.bpgenvm.vm.debug.DebugEvent> debugEventListener;

    /** Sink al que se escribe el output de los opcodes PRINT_* del programa
     *  BP. Por defecto System.out; el IDE inyecta su propio sink cuando
     *  arranca la VM como subproceso (A1.2). NO incluye el output de
     *  diagnóstico interno de la VM (GC log, banner, trace). */
    private volatile OutputSink programOut = new StdoutSink();

    public VirtualMachine() {
        this(DEFAULT_MEMORY_SIZE, DEFAULT_STACK_BASE);
    }

    /** Reemplaza el sink de output del programa. Llamable en cualquier
     *  momento (los workers leen el campo volatile en cada print).
     *  Pasar null restaura el StdoutSink por defecto. */
    public void setProgramOut(OutputSink sink) {
        this.programOut = (sink != null) ? sink : new StdoutSink();
    }

    public OutputSink getProgramOut() { return programOut; }

    /** Cablea un listener para eventos del debugger emitidos directamente
     *  por la VM (hoy ExceptionEvent al morir un thread). NO sustituye al
     *  DebugHook — ése sigue siendo el camino de pausa/step. */
    public void setDebugEventListener(java.util.function.Consumer<edu.bpgenvm.vm.debug.DebugEvent> l) {
        this.debugEventListener = l;
    }

    /**
     * B1 instrumentation. Activable con `-Dbpvm.b1.diag=1` o env
     * `BPVM_B1_DIAG=1`. Cuando un thread cae con BpThreadFault /
     * RuntimeException no recuperable, vuelca al stderr un snapshot
     * completo: estado del thread fallido, snapshot de TODOS los
     * threads BP, runQueue, mutexes. Objetivo: cuando la race del
     * residual (~5-10%) dispara, capturar suficiente contexto para
     * razonar sobre el estado imposible que produjo el fallo.
     *
     * Cuando ENABLED es false (default), las llamadas a dumpFault son
     * un read de boolean estático y un return — JIT-eliminables.
     */
    private static final boolean B1_DIAG_ENABLED;
    private static final java.util.concurrent.atomic.AtomicLong B1_DIAG_COUNTER =
            new java.util.concurrent.atomic.AtomicLong(0);
    static {
        String prop = System.getProperty("bpvm.b1.diag");
        String env  = System.getenv("BPVM_B1_DIAG");
        B1_DIAG_ENABLED = "1".equals(prop) || "true".equalsIgnoreCase(prop)
               || "1".equals(env)  || "true".equalsIgnoreCase(env);
        if (B1_DIAG_ENABLED) {
            System.err.println("[B1Diag] instrumentación ACTIVADA — vuelco estado al primer fallo");
        }
    }

    private synchronized void dumpFault(int workerId, ThreadContext failedTc,
                                        String label, Throwable cause) {
        if (!B1_DIAG_ENABLED) return;
        long seq = B1_DIAG_COUNTER.incrementAndGet();
        StringBuilder sb = new StringBuilder();
        sb.append("\n======== [B1Diag #").append(seq).append("] ========\n");
        sb.append("worker=").append(workerId)
          .append(" failedTid=").append(failedTc == null ? -1 : failedTc.id)
          .append(" label=").append(label).append('\n');
        if (failedTc != null) {
            sb.append("  pc=").append(failedTc.pc)
              .append(" sp=").append(failedTc.sp)
              .append(" bp=").append(failedTc.bp)
              .append(" cs=").append(failedTc.cs)
              .append(" status=").append(failedTc.status)
              .append(" blockedOnMutex=").append(failedTc.blockedOnMutexId)
              .append(" stackBase=").append(failedTc.stackBase)
              .append(" stackTop=").append(failedTc.stackTop)
              .append('\n');
            sb.append("  ehHandlerPc=").append(failedTc.ehHandlerPc)
              .append(" ehSavedSp=").append(failedTc.ehSavedSp)
              .append(" handlerStack.size=")
              .append(failedTc.handlerStack == null ? 0 : failedTc.handlerStack.size())
              .append(" allocAnchor=").append(failedTc.allocAnchor)
              .append('\n');
        }
        if (cause != null) {
            sb.append("  cause: ").append(cause.getClass().getSimpleName())
              .append(": ").append(cause.getMessage()).append('\n');
            StackTraceElement[] st = cause.getStackTrace();
            int n = Math.min(8, st == null ? 0 : st.length);
            for (int i = 0; i < n; i++) {
                sb.append("    at ").append(st[i]).append('\n');
            }
        }
        synchronized (vmLock) {
            sb.append("--- threads (").append(threads.size()).append(") ---\n");
            for (ThreadContext t : threads) {
                if (t == null) continue;
                sb.append("  tid=").append(t.id)
                  .append(" status=").append(t.status)
                  .append(" pc=").append(t.pc)
                  .append(" sp=").append(t.sp)
                  .append(" bp=").append(t.bp)
                  .append(" blockedOnMutex=").append(t.blockedOnMutexId)
                  .append('\n');
            }
            sb.append("--- runQueue ").append(runQueue).append('\n');
            if (!mutexes.isEmpty()) {
                sb.append("--- mutexes (").append(mutexes.size()).append(") ---\n");
                for (int i = 0; i < mutexes.size(); i++) {
                    JavaMutex jm = mutexes.get(i);
                    sb.append("  mid=").append(i)
                      .append(" owner=").append(jm.ownerTid)
                      .append(" waiters=").append(jm.waiters)
                      .append('\n');
                }
            }
        }
        sb.append("======== /B1Diag #").append(seq).append(" ========");
        System.err.println(sb.toString());
    }

    /** Helper: emite un evento al listener si está cableado. Llamable desde
     *  cualquier thread; el listener decide cómo serializar al canal. */
    private void emitDebugEvent(edu.bpgenvm.vm.debug.DebugEvent ev) {
        java.util.function.Consumer<edu.bpgenvm.vm.debug.DebugEvent> l = this.debugEventListener;
        if (l != null) {
            try { l.accept(ev); }
            catch (Throwable t) {
                System.err.println("[VM] debugEventListener falló: " + t.getMessage());
            }
        }
    }

    /**
     * B3 v2 — Construye una instancia BP de RuntimeError(message) en el
     * thread {@code tc} y lanza {@link BpExceptionPending}. El dispatcher
     * del CALL_BUILTIN captura la excepción y ejecuta el unwind igual que
     * el opcode THROW: busca el handler `catch e: RuntimeError` más
     * cercano en {@code handlerStack} y, si encuentra uno, transfiere
     * control allí con el ref del exception en el top del stack.
     *
     * Si no se encuentra la clase RuntimeError en el módulo del tc o
     * cualquier otra falla impide construir el objeto, se delega a
     * {@link BpThreadFault} con el mensaje original (comportamiento
     * legacy: mata el thread).
     *
     * El llamador NO retorna normalmente — esta función SIEMPRE lanza.
     */
    /** Lee el field `msg` (slot 0) de un objeto BP RuntimeError y devuelve
     *  el contenido del string, o null si el ref no apunta a un objeto
     *  con un campo string válido. Tolerante a referencias inválidas
     *  (usado por el path de "uncaught"). */
    private String readRuntimeErrorMsg(long objRef) {
        try {
            int addr = refDeref(objRef);   // V4: el ref de excepción es un handle → dirección física
            if (addr <= 0) return null;
            long msgRef = readI64(memory, addr + 4 + 0 * 4);   // msg = handle 64b (campo slot 0)
            int msgAddr = refDeref(msgRef);
            if (msgAddr <= 0) return null;
            return readStringIfPossible(msgAddr);
        } catch (Throwable t) {
            return null;
        }
    }

    public void throwBpRuntimeError(ThreadContext tc, String message) {
        ModuleManager mm = getModuleManager();
        if (mm == null) {
            throw new BpThreadFault(message);
        }
        // #248 — primero la clase ÚNICA de Core (jerarquía Object -> Exception
        // -> RuntimeError). Fallback: la copia per-módulo legado (mods viejos),
        // resuelta contra el cs del thread.
        Integer classPtrBox = mm.resolveGlobal("Core.RuntimeError");
        if (classPtrBox == null) classPtrBox = mm.resolveExportInModule(tc.cs, "RuntimeError");
        if (classPtrBox == null) {
            // El módulo del thread actual no exportó RuntimeError (es un
            // .mod antiguo emitido antes de B3 v2, o un caso degenerado).
            // Fallback al comportamiento previo.
            throw new BpThreadFault(message);
        }
        int classPtr = classPtrBox;

        long msgRef;
        long objH;
        // Alocamos string + objeto bajo vmLock para sincronizar con GC.
        synchronized (vmLock) {
            msgRef = allocVmString(message == null ? "" : message);   // handle 64b
            int numFields = readInt16(classPtr + CLS_OFF_NUM_FIELDS) & 0xFFFF;
            int objAddr = heapAlloc(numFields * 4, TYPE_OBJECT);   // dirección física para el init
            writeInt32(objAddr, classPtr);
            for (int i = 0; i < numFields; i++) {
                writeInt32(objAddr + 4 + i * 4, 0);
            }
            // Field `msg` en slot 0 (la clase sintetizada lo declara primero). Es una
            // ref (string) → 8 bytes; guardamos el HANDLE completo (gen preservada).
            writeI64(memory, objAddr + 4 + 0 * 4, msgRef);
            objH = handleRegister(objAddr);   // addr → handle 64b (tras escribir todos los campos)
            // Empujamos el ref al stack del thread; el dispatcher hará el pop en el
            // unwind. 8 BYTES = handle 64b completo (gen preservada): el objeto-excepción
            // puede caer en un slot RECICLADO (gen>0); si se empujara a 4B la gen se perdería
            // y el catch, al leer e.msg (requireAlive), vería gen-mismatch y re-lanzaría
            // (lo cazó uafcascade). El catch BpExceptionPending pop-ea 8B a juego.
            writeI64(memory, tc.sp, objH);
            tc.sp += 8;
            // B1 residual — anclamos al thread para que el GC no lo libere entre soltar
            // vmLock y el unwind. El ancla es idx|TAG (refDeref dead-tolerant).
            tc.allocAnchor = (int) objH;
        }
        throw new BpExceptionPending((int) objH);
    }

    /** V3 Forms — valida que `parent` es un contenedor vivo; si no, lanza el mismo
     *  RuntimeError "widget sin contenedor" que la VM-C (gui_make_child) -> paridad. */
    private void guiRequireParent(ThreadContext tc, int parent) {
        if (!gui.parentAlive(parent))
            throwBpRuntimeError(tc,
                "Gui: no se puede crear un widget sin un contenedor valido; crea Gui.Screen() o Gui.Window() primero");
    }

    /** H19 — ruta del módulo principal (entry) en ejecución; la cablea Main al
     *  cargar el .mod. App.mainModulePath()/mainModule() la usan; App.projectPath()
     *  usa el workdir del ModuleManager. "" si no se fijó. */
    private String appMainModulePath = "";
    public void setAppMainModulePath(String p) { this.appMainModulePath = (p != null) ? p : ""; }

    /**
     * A2.1 + B3 v3 — Resuelve un path de usuario aplicando el sandbox del
     * workdir si está configurado. Si no hay workdir (legacy), el path se
     * usa tal cual.
     *
     * Llamado por los builtins de IO. Si el sandbox rechaza el path,
     * dispara un RuntimeError BP (vía throwBpRuntimeError) que el código
     * BP puede atrapar con `try/catch e: RuntimeError`.
     */
    private java.nio.file.Path sandboxPath(ThreadContext tc, String userPath) {
        ModuleManager mm = getModuleManager();
        if (mm != null && mm.getWorkdir() != null) {
            try {
                return mm.resolveInWorkdir(userPath);
            } catch (SecurityException se) {
                throwBpRuntimeError(tc, "sandbox: " + se.getMessage());
                return null;   // unreachable: throwBpRuntimeError siempre throws
            } catch (RuntimeException re) {
                throwBpRuntimeError(tc, "sandbox: " + re.getMessage());
                return null;
            }
        }
        return java.nio.file.Paths.get(userPath);
    }

    /**
     * #310 — ¿se lo lleva dentro el pack en ejecución? Devuelve los bytes del
     * recurso, o null si no hay pack o el pack no lo lleva.
     *
     * Es el espejo del overlay de LECTURA de la fachada FS de la VM-C
     * ({@code bpvm_fs_set_overlay}): lo consultan los builtins que LEEN
     * (readFile, readFileBytes) o que PREGUNTAN (fileExists, fileSize), nunca
     * los que escriben. Para el programa y para la VM, el pack es de sólo
     * lectura.
     */
    /**
     * #348 — mayúsculas/minúsculas LATIN-1. Antes esto era {@code toUpperCase()}
     * de Java: Unicode completo y —lo grave— DEPENDIENTE DEL LOCALE de la
     * máquina (en un locale turco, 'i' sube a 'İ'), o sea que miVM no era ni
     * determinista consigo misma. Reproducirlo en un micro pide tablas de
     * kilobytes, así que se acordó LATIN-1 en las DOS VMs: ASCII + el bloque
     * U+00C0..U+00FF, algorítmico y sin tablas.
     *
     * Réplica EXACTA de latin1_upper_cp/latin1_lower_cp de la VM-C: si una
     * cambia, la otra va detrás. Lo que se pierde a propósito: 'ß' no sube a
     * "SS" (crecería de longitud), y fuera de Latin-1 no se toca nada.
     */
    /**
     * #348 tanda 3 — rutas de BP: SIEMPRE '/'. La semántica se define aquí, no
     * se hereda de java.nio.file (que es dependiente de plataforma). Réplica
     * EXACTA de los BUILTIN_PATH_* de la VM-C: si una cambia, la otra detrás.
     *
     * join: a vacío → b; b vacío → a; si no, a sin '/' finales + '/' + b sin
     * '/' iniciales. Así {@code pathJoin("/", "x")} da {@code "/x"}.
     */
    private static String pathJoin(String a, String b) {
        if (a.isEmpty()) return b;
        if (b.isEmpty()) return a;
        int ae = a.length();  while (ae > 0 && a.charAt(ae - 1) == '/') ae--;
        int bs = 0;           while (bs < b.length() && b.charAt(bs) == '/') bs++;
        return a.substring(0, ae) + "/" + b.substring(bs);
    }
    /** parent: se ignoran los '/' finales; luego hasta el último '/' sin
     *  incluirlo. Sin '/' → ""; si el último está en 0 → "/" (raíz). */
    private static String pathParent(String p) {
        int e = p.length();  while (e > 0 && p.charAt(e - 1) == '/') e--;
        int slash = p.lastIndexOf('/', e - 1);
        if (slash < 0)  return "";
        if (slash == 0) return "/";
        return p.substring(0, slash);
    }
    /** basename: se ignoran los '/' finales; luego desde el último '/'. */
    private static String pathBasename(String p) {
        int e = p.length();  while (e > 0 && p.charAt(e - 1) == '/') e--;
        int slash = p.lastIndexOf('/', e - 1);
        return p.substring(slash + 1, e);
    }

    private static String latin1Case(String s, boolean up) {
        StringBuilder b = new StringBuilder(s.length());
        for (int i = 0; i < s.length(); ) {
            int c = s.codePointAt(i);
            i += Character.charCount(c);
            b.appendCodePoint(up ? latin1UpperCp(c) : latin1LowerCp(c));
        }
        return b.toString();
    }
    private static int latin1UpperCp(int c) {
        if (c >= 'a' && c <= 'z')                        return c - 32;
        if (c >= 0x00E0 && c <= 0x00FE && c != 0x00F7)   return c - 32;   // ÷ no es letra
        if (c == 0x00FF)                                 return 0x0178;   // ÿ → Ÿ
        return c;
    }
    private static int latin1LowerCp(int c) {
        if (c >= 'A' && c <= 'Z')                        return c + 32;
        if (c >= 0x00C0 && c <= 0x00DE && c != 0x00D7)   return c + 32;   // × no es letra
        if (c == 0x0178)                                 return 0x00FF;   // Ÿ → ÿ
        return c;
    }

    private byte[] packResource(String path) {
        ModuleManager mm = getModuleManager();
        return (mm != null) ? mm.packResource(path) : null;
    }

    /**
     * Variante con tamaño total y `stackBase` (donde termina el heap y
     * empiezan los stacks). Llamada desde {@link edu.bpgenvm.Main} cuando
     * hay un BpVM.cfg activo. Validaciones:
     *   memorySize &gt; 0
     *   stackBase  &gt; 0
     *   stackBase  &lt; memorySize
     *   stackBase + MAIN_STACK_BYTES &lt;= memorySize  (cabe al menos el main)
     */
    public VirtualMachine(int memorySize, int stackBase) {
        // #360 — el backend gráfico vive fuera de esta clase, así que hay que
        // decirle de dónde salen los recursos: el pack en ejecución primero. Se
        // instala aquí y no en cada uso porque la lambda se evalúa al llamarla
        // (el ModuleManager puede no existir todavía en este punto).
        gui.setResourceOverlay(this::packResource);
        if (memorySize <= 0)
            throw new IllegalArgumentException("memorySize debe ser > 0: " + memorySize);
        if (stackBase <= 0)
            throw new IllegalArgumentException("stackBase debe ser > 0: " + stackBase);
        if (stackBase >= memorySize)
            throw new IllegalArgumentException("stackBase (" + stackBase
                    + ") debe ser < memorySize (" + memorySize + ")");
        if (stackBase + MAIN_STACK_BYTES > memorySize)
            throw new IllegalArgumentException("no cabe el stack del main: stackBase="
                    + stackBase + " + MAIN_STACK_BYTES=" + MAIN_STACK_BYTES
                    + " > memorySize=" + memorySize);
        this.memory     = new byte[memorySize];
        this.STACK_BASE = stackBase;
        this.SP         = stackBase;
        this.BP         = stackBase;
        this.heapStart  = stackBase;
        this.heapNext   = stackBase;
        this.lastGcHeapNext = stackBase;
        this.handleNext = 1;   // V4: la tabla de handles se reinicia con el heap
        this.handlePressure = false;   // #430: y su marca
        this.nextStackBase = stackBase + MAIN_STACK_BYTES;

        // memory[0] = 0x70 (opcode THREAD_EXIT). Es la sentinela de salida
        // de los workers: cuando su run() hace RET, el saved PC apunta a 0
        // y la siguiente instrucción fetcheada es THREAD_EXIT, que termina
        // EL THREAD ACTUAL sin tumbar la VM. HALT (0x00) ahora es exclusivo
        // del thread main (termina la VM entera).
        memory[0] = (byte) 0x70;

        // memory[2] = 0x6F (EVENT_RETURN). H5.c: sentinela de vuelta de un
        // handler de evento. El frame que inyecta el scheduler lleva ahí su
        // saved PC; cuando el handler hace RET, el dispatch cae en esta celda,
        // tira el valor de retorno y salta al PC de reanudación. Espejo exacto
        // de BPVM_SENTINEL_EVENT_RETURN_ADDR de la VM-C.
        memory[2] = (byte) 0x6F;
        eventQueue.clear();

        // Thread 0 (main): región fija de MAIN_STACK_BYTES. El resto del
        // espacio queda libre para nuevos threads (alocados con allocStackRegion).
        ThreadContext main = new ThreadContext(0, STACK_BASE, STACK_BASE + MAIN_STACK_BYTES);
        threads.add(main);
        currentThread = main;
        currentThread.status = ThreadStatus.RUNNING;
        this.SP = main.sp;
        this.BP = main.bp;
        this.handlerStack = main.handlerStack;
    }

    /**
     * Vuelca los registros "activos" del intérprete (campos PC/SP/BP/CS y
     * EH) al ThreadContext actual. Se llama antes de un context switch.
     */
    /**
     * @deprecated runOnContext sincroniza su propio ThreadContext en el
     *             finally; este método ya no se llama desde el scheduler.
     *             Sigue existiendo por si en el futuro algún path externo
     *             pre-runOnContext necesita persistir this.X → tc.
     */
    @SuppressWarnings("unused")
    private void saveCurrentContext() {
        currentThread.pc = this.PC;
        currentThread.sp = this.SP;
        currentThread.bp = this.BP;
        currentThread.cs = this.CS;
    }

    /**
     * Cambia el ThreadContext activo. Actualiza el alias {@code currentThread}
     * y, por compatibilidad con setPC/setCS y herramientas que aún lean los
     * campos this.X, también vuelca pc/sp/bp/cs ahí. runOnContext SOLO lee
     * desde tc.X así que el this.X vive sólo para inspección externa.
     */
    private void loadContext(int threadId) {
        ThreadContext t = threads.get(threadId);
        this.currentThreadId = threadId;
        this.currentThread = t;
        this.PC = t.pc;
        this.SP = t.sp;
        this.BP = t.bp;
        this.CS = t.cs;
    }

    public void setModuleManager(ModuleManager manager) { this.moduleManager = manager; }
    public ModuleManager getModuleManager() { return moduleManager; }
    public void setPC(int pc) { this.PC = pc; }
    public void setCS(int cs) { this.CS = cs; }
    /** Habilita o deshabilita el trace per-instrucción (PC/Opcode/CS/SP). */
    public void setTracing(boolean v) { this.tracing = v; }

    /** Sincroniza el wall clock virtual de la VM con el segundo epoch
     *  dado — equivalente a la llamada BP `Rtc.Clock.setNowSec(s)`. Lo
     *  usa el wire protocol v1 TIME para que el IDE alinee el reloj del
     *  dispositivo con su propio clock. */
    public void setRtcEpochSec(long epochSec) {
        long targetMs = epochSec * 1000L;
        this.rtcOffsetMs = targetMs - System.currentTimeMillis();
    }

    /**
     * Hook de depuración. Cuando es null (default) la VM no paga ningún
     * coste: el chequeo es un único {@code if (debugHook != null)} por
     * opcode. Cuando se instala, la VM consulta al ModuleManager la línea
     * fuente del PC actual; si cambió respecto al opcode anterior, llama
     * a {@link DebugHook#onLineChange}. La implementación típica del IDE
     * bloquea ahí hasta recibir un comando de continuación.
     */
    private DebugHook debugHook = null;
    public void setDebugHook(DebugHook hook) { this.debugHook = hook; }

    public void setHeapStart(int addr) {
        if (addr < 0 || addr > STACK_BASE) {
            throw new RuntimeException("heapStart fuera de rango: " + addr);
        }
        this.heapStart = addr;
        this.heapNext  = addr;
        this.freeListHead = 0;
        // H3: base del umbral de GC proactivo, fijada con el heapStart real
        // (tras cargar el data block). Umbral ~1/8 del heap, con suelo de 4 KB.
        this.lastGcHeapNext = addr;
        this.handleNext = 1;   // V4: la tabla de handles se reinicia con el heap
        this.handlePressure = false;   // #430: y su marca
        this.gcBumpThreshold = Math.max(4096, (STACK_BASE - addr) / 8);
    }

    public void injectMemory(int targetAddress, byte[] data) {
        if (targetAddress + data.length > memory.length) {
            throw new RuntimeException("Error: El módulo excede el tamaño máximo de la memoria de la VM.");
        }
        System.arraycopy(data, 0, memory, targetAddress, data.length);
    }

    // ====================================================================
    // GC: mark-and-sweep conservativo
    // ====================================================================

    private int elemSize(int type) {
        switch (type) {
            case TYPE_ARRAY_I8:  return 1;
            case TYPE_ARRAY_I16: return 2;
            case TYPE_ARRAY_I32: return 4;
            case TYPE_ARRAY_I64: return 8;   // H1.2 (V2)
            case TYPE_ARRAY_REF: return 8;   // H1.2a (V4): ref plana = 8 bytes (low32 = dirección)
            default: throw new RuntimeException("Tipo de heap desconocido: " + type);
        }
    }

    /** Tamaño total en bytes del objeto que empieza en headerAddr (cabecera + payload, alineado a 4). */
    private int objectTotalSize(int headerAddr) {
        int tag = readInt32(headerAddr);
        if ((tag & TAG_FREE_BIT) != 0) {
            return readInt32(headerAddr + 4); // length field stores total bytes for free blocks
        }
        int type = (tag & TAG_TYPE_MASK) >>> TAG_TYPE_SHIFT;
        if (type == TYPE_OBJECT) {
            // header+4 contiene class_ptr (no length). El tamaño lo dicta el descriptor.
            int classPtr = readInt32(headerAddr + 4);
            int numFields = readInt16(classPtr + CLS_OFF_NUM_FIELDS) & 0xFFFF;
            return alignTo4(OBJ_HEADER_SIZE + numFields * 4);
        }
        int length = readInt32(headerAddr + 4);
        int payload = length * elemSize(type);
        int total = OBJ_HEADER_SIZE + payload;
        return alignTo4(total);
    }

    private static int alignTo4(int x) {
        return (x + 3) & ~3;
    }

    /**
     * Reserva un objeto en el heap. payloadBytes = bytes del payload (sin cabecera).
     * Devuelve user_ref = headerAddr + 4 (apunta al length, igual que data/local arrays).
     */
    private int heapAlloc(int payloadBytes, int type) {
        int totalSize = Math.max(MIN_FREE_BLOCK, alignTo4(OBJ_HEADER_SIZE + payloadBytes));
        synchronized (vmLock) {
            ThreadContext me = currentTcLocal.get();
            int myTid = (me != null) ? me.id : -1;

            // Si otro worker está ejecutando GC stop-the-world, esperamos a
            // que termine antes de hacer NADA. Nos registramos como parked
            // para que su check de "anyOtherThreadRunning" nos excluya.
            while (gcInProgress) {
                if (myTid >= 0) parkedInHeapAlloc.add(myTid);
                try { vmLock.wait(); }
                catch (InterruptedException ie) {
                    Thread.currentThread().interrupt();
                    if (myTid >= 0) parkedInHeapAlloc.remove(myTid);
                    throw new RuntimeException("heapAlloc interrumpido esperando otro GC");
                }
                if (myTid >= 0) parkedInHeapAlloc.remove(myTid);
            }

            // H3: GC PROACTIVO por umbral de crecimiento de bump. Evita el
            // over-commit (que el heap suba a su pico de bump antes de colectar):
            // si el bump ha avanzado >= umbral desde el último GC, colecta ahora.
            //
            // #430 — SEGUNDO EJE: la TABLA DE HANDLES. El umbral cuenta VOLUMEN
            // y un programa de objetos chicos se le escapa: son pocos KB pero
            // decenas de miles de SLOTS. Aquí la tabla crece con un Arrays.copyOf
            // que el runtime de Java paga sin rechistar, así que el síntoma es
            // sólo de placa (la Metro se colgaba: 512 KB de SRAM que no tiene) —
            // pero el ALGORITMO es el mismo en las dos VMs, y el invariante es
            // que lo sea. La presión llega como MARCA (handlePressure, armada en
            // handleRegister al repartir slot de la zona final).
            boolean porTabla = handlePressure
                    || (handleFreeTop == 0 && handleNext >= handleAddr.length);
            if (!gcSuspended
                    && (heapNext - lastGcHeapNext >= gcBumpThreshold || porTabla)) {
                gcSafepoint(myTid);
                // Resolver la presión: si la colecta recicló slots, resuelta; si
                // no (todo VIVO), crecer aquí — donde crecer es una decisión y no
                // un accidente en medio de un register.
                if (handlePressure) {
                    if (handleFreeTop > 0) {
                        handlePressure = false;
                    } else {
                        handleAddr = java.util.Arrays.copyOf(handleAddr, handleAddr.length * 2);
                        handleGen  = java.util.Arrays.copyOf(handleGen,  handleGen.length  * 2);
                        handlePressure = false;
                    }
                }
            }
            int addr = tryAllocateInner(totalSize);
            if (addr != -1) {
                int tag = (type << TAG_TYPE_SHIFT);
                writeInt32(addr, tag);
                int userRef = addr + 4;
                // Paridad con la VM-C (heap.c): zero-init de length slot +
                // payload. CRÍTICO al reusar bloques del free-list — sin esto
                // NEWARRAY/newIntArray()/NEW_OBJECT sobre un bloque reciclado
                // devuelven el contenido del objeto anterior (la VM-C hace
                // memset SIEMPRE; aquí faltaba).
                java.util.Arrays.fill(memory, userRef, userRef + 4 + payloadBytes, (byte) 0);
                // Ancla GC: el ref vuelve al caller que aún tiene que
                // publicarlo (write a stack/field) sin lock. Si entre
                // medias otro worker dispara GC, scanRegion ve este ancla
                // y marca el objeto como vivo.
                if (me != null) me.allocAnchor = userRef;
                throwingOom = false;   // H1: alocación con éxito → limpia la guarda
                return userRef;
            }

            // Sin espacio: hay que correr GC. Pedimos un safepoint
            // stop-the-world (B1) — todos los demás workers deben sincronizar
            // sus tc.{sp,bp,cs,pc} y parquear, si no el GC marcará desde tc.sp
            // desactualizado y liberará objetos que aún están en uso, dejando
            // refs colgantes que se manifiestan como bytecodes basura,
            // direcciones inválidas, mutex.id corruptos, etc.
            gcSafepoint(myTid);
            addr = tryAllocateInner(totalSize);
            if (addr == -1) {
                // H1 — OOM tras GC: RuntimeError BP ATRAPABLE (nunca colgar ni matar la VM
                // en silencio). throwBpRuntimeError construye el objeto (re-entra heapAlloc,
                // reentrante bajo vmLock); la guarda throwingOom evita recursión si NI la
                // excepción cabe → fallback incatchable (BpThreadFault).
                if (throwingOom || me == null) {
                    // #430 — LA PREFABRICADA (idea de Eduardo): construida en el
                    // prólogo del run, cuando construir era gratis. Lanzarla no
                    // aloja NADA, así que el "ni para la excepción" deja de ser
                    // una muerte incatchable y pasa a ser un OOM que el programa
                    // puede atrapar — igual que en la VM-C (exceptions.c).
                    if (oomExc != 0 && me != null) {
                        writeI64(memory, me.sp, oomExc);
                        me.sp += 8;
                        me.allocAnchor = (int) oomExc;
                        throwingOom = false;
                        throw new BpExceptionPending((int) oomExc);
                    }
                    throw new BpThreadFault("No space in heap (ni para la excepción): pido "
                            + totalSize + " bytes; libre=" + (STACK_BASE - heapNext));
                }
                throwingOom = true;
                throwBpRuntimeError(me, "No space in heap");   // lanza BpExceptionPending
                // (inalcanzable)
            }
            int tag = (type << TAG_TYPE_SHIFT);
            writeInt32(addr, tag);
            int userRef2 = addr + 4;
            // Mismo zero-init que la ruta sin GC (paridad VM-C).
            java.util.Arrays.fill(memory, userRef2, userRef2 + 4 + payloadBytes, (byte) 0);
            if (me != null) me.allocAnchor = userRef2;
            throwingOom = false;   // H1: alocación con éxito → limpia la guarda
            return userRef2;
        }
    }

    /**
     * Corre un GC stop-the-world (safepoint B1). DEBE invocarse con vmLock
     * adquirido. Lo usan tanto el disparo PROACTIVO por umbral como la ruta
     * de OOM (bump+free-list agotados).
     */
    private void gcSafepoint(int myTid) {
        gcInProgress = true;
        stopTheWorld = true;
        if (myTid >= 0) parkedInHeapAlloc.add(myTid);
        vmLock.notifyAll();
        try {
            while (anyOtherThreadRunning(myTid)) {
                try { vmLock.wait(); }
                catch (InterruptedException ie) {
                    Thread.currentThread().interrupt();
                    throw new RuntimeException("heapAlloc interrumpido esperando safepoint");
                }
            }
            // Mundo parado: GC seguro. Nos desmarcamos de parked porque ya
            // estamos ejecutando GC (no esperando).
            if (myTid >= 0) parkedInHeapAlloc.remove(myTid);
            gcLocked();
        } finally {
            if (myTid >= 0) parkedInHeapAlloc.remove(myTid);
            stopTheWorld = false;
            gcInProgress = false;
            vmLock.notifyAll();
        }
    }

    /**
     * True si HAY algún tc en estado RUNNING que NO sea el llamante
     * (`myTid`) y que tampoco esté parqueado en heapAlloc. Mientras
     * devuelva true, el llamante (que va a correr GC) debe esperar.
     *
     * Excluir parkedInHeapAlloc es crítico para evitar deadlock cuando
     * dos workers entran en heapAlloc concurrentemente: ambos verían al
     * otro como RUNNING y se esperarían eternamente. Como el "parked"
     * está bloqueado en vmLock.wait() (no ejecuta bytecode), es seguro
     * correr GC respecto a él aunque su BP status siga siendo RUNNING.
     */
    private boolean anyOtherThreadRunning(int myTid) {
        for (ThreadContext t : threads) {
            if (t.status != ThreadStatus.RUNNING) continue;
            if (t.id == myTid) continue;
            if (parkedInHeapAlloc.contains(t.id)) continue;
            return true;
        }
        return false;
    }

    /** Intenta asignar; devuelve la dirección de la cabecera o -1 si no cabe. */
    private int tryAllocateInner(int totalSize) {
        // 1) Free list first-fit
        int prev = 0;
        int cur = freeListHead;
        while (cur != 0) {
            int blockSize = readInt32(cur + 4);
            int next = readInt32(cur + 8);
            if (blockSize >= totalSize) {
                int remaining = blockSize - totalSize;
                // INVARIANTE DEL HEAP: un bloque ASIGNADO no guarda su tamaño en ningún
                // sitio — objectTotalSize lo RECALCULA desde type/length. Luego el tamaño
                // físico debe coincidir SIEMPRE con ese recálculo, o el recorrido del heap
                // (buildValidObjectsSet / gc) aterriza a mitad del siguiente y descarrila.
                //
                // BUG (cazado 15-jul en la VM-C, que es espejo de ésta; repro
                // `--mem=131072 samples/MemT4d_Count.bp` → petaba en el concat nº 193):
                // si el sobrante era < MIN_FREE_BLOCK (no representable como bloque libre)
                // se REGALABA al bloque asignado → ocupaba blockSize pero objectTotalSize
                // decía totalSize → recorrido corto → set de cabeceras incompleto →
                // objetos VIVOS no reconocidos → el barrido se los llevaba → UAF.
                // Aquí estaba LATENTE (el heap por defecto es grande y la free-list apenas
                // se ejerce); se arregla en lockstep con la VM-C, no cuando explote.
                //
                // FIX: aceptar el bloque sólo si encaja EXACTO o si el resto es un bloque
                // libre representable. Un sobrante-astilla se salta (sigue en la lista).
                if (remaining == 0 || remaining >= MIN_FREE_BLOCK) {
                    if (remaining >= MIN_FREE_BLOCK) {
                        // Split: usar cur..cur+totalSize, dejar cur+totalSize..cur+blockSize como libre
                        int newFreeAddr = cur + totalSize;
                        writeInt32(newFreeAddr, TAG_FREE_BIT);
                        writeInt32(newFreeAddr + 4, remaining);
                        writeInt32(newFreeAddr + 8, next);
                        if (prev == 0) freeListHead = newFreeAddr;
                        else writeInt32(prev + 8, newFreeAddr);
                    } else {
                        // Encaje exacto: usar el bloque completo; quitarlo de la lista
                        if (prev == 0) freeListHead = next;
                        else writeInt32(prev + 8, next);
                    }
                    return cur;
                }
                // sobrante no representable → este bloque no sirve, seguir buscando
            }
            prev = cur;
            cur = next;
        }
        // 2) Bump desde heapNext
        if (heapNext + totalSize > STACK_BASE) return -1;
        int addr = heapNext;
        heapNext += totalSize;
        return addr;
    }

    /** Construye el conjunto de cabeceras válidas recorriendo el heap. */
    private Set<Integer> buildValidObjectsSet() {
        Set<Integer> valid = new HashSet<>();
        int addr = heapStart;
        while (addr < heapNext) {
            int tag = readInt32(addr);
            int size = objectTotalSize(addr);
            if ((tag & TAG_FREE_BIT) == 0) {
                valid.add(addr);
            }
            if (size <= 0) break; // protección contra corrupción
            addr += size;
        }
        // GUARDIÁN DEL INVARIANTE (permanente; coste = UNA comparación por GC).
        // El heap es una tira contigua de bloques y ninguno guarda su tamaño: se
        // RECALCULA con objectTotalSize. Si todos miden lo que ese recálculo dice, el
        // recorrido aterriza EXACTAMENTE en heapNext. Si no, ya se ha desincronizado:
        // sigue leyendo payload como si fuera cabecera, el set sale incompleto → se
        // barren objetos VIVOS → use-after-free mucho más tarde y sin rastro del origen
        // (así se fueron varios días con MemT4 en la VM-C: el descarrilamiento era
        // SILENCIOSO). Que grite aquí, en el sitio y el instante del destrozo.
        if (addr != heapNext) {
            System.err.println("[gc] !! HEAP INCONSISTENTE: el recorrido de cabeceras acabó en "
                    + addr + ", no en heapNext=" + heapNext + ". Hay un bloque cuyo tamaño real no "
                    + "coincide con objectTotalSize() → el set del GC sale incompleto y se barrerán "
                    + "objetos vivos.");
        }
        return valid;
    }

    /** Marca un objeto como vivo y propaga si es array de refs o instancia con refs. */
    private void markObject(int headerAddr, Set<Integer> valid) {
        int tag = readInt32(headerAddr);
        if ((tag & TAG_MARK_BIT) != 0) return; // ya marcado
        writeInt32(headerAddr, tag | TAG_MARK_BIT);

        int type = (tag & TAG_TYPE_MASK) >>> TAG_TYPE_SHIFT;
        if (type == TYPE_ARRAY_REF) {
            int length = readInt32(headerAddr + 4);
            for (int i = 0; i < length; i++) {
                // H1.2a (V4): elemento ref = 8 bytes (stride 8); la dirección va en la
                // palabra baja (big-endian) → readI64 y (int) toma los 32 bajos.
                int childRef = (int) readI64(memory, headerAddr + OBJ_HEADER_SIZE + i * 8);
                int childHeader = refDeref(childRef) - 4; // user_ref apunta a length
                if (valid.contains(childHeader)) {
                    markObject(childHeader, valid);
                }
            }
        } else if (type == TYPE_OBJECT) {
            int classPtr = readInt32(headerAddr + 4);
            int numFields = readInt16(classPtr + CLS_OFF_NUM_FIELDS) & 0xFFFF;
            int bitmapBase = classPtr + CLS_OFF_BITMAP_BASE;
            // Los campos del objeto empiezan en headerAddr + 8 (justo tras tag y class_ptr).
            for (int i = 0; i < numFields; i++) {
                int word = readInt32(bitmapBase + (i >>> 5) * 4);
                if (((word >> (i & 31)) & 1) != 0) {
                    // H1.2a (V4): campo ref = 8 bytes (2 slots, bit en el slot base);
                    // dirección en la palabra baja → readI64 + (int) low32.
                    int childRef = (int) readI64(memory, headerAddr + OBJ_HEADER_SIZE + i * 4);
                    int childHeader = refDeref(childRef) - 4;
                    if (valid.contains(childHeader)) {
                        markObject(childHeader, valid);
                    }
                }
            }
        }
    }

    /** Escanea conservativamente una región de 4 bytes en 4 bytes. */
    private void scanRegion(int start, int endExclusive, Set<Integer> valid) {
        // Asegurar alineamiento a 4
        int s = (start + 3) & ~3;
        for (int p = s; p + 4 <= endExclusive; p += 4) {
            int candidate = readInt32(p);
            int headerAddr = refDeref(candidate) - 4;   // V4: deref handle→addr (conservador)
            if (valid.contains(headerAddr)) {
                markObject(headerAddr, valid);
            }
        }
    }

    /** Versión pública: adquiere vmLock y delega en gcLocked. */
    public void gc() {
        synchronized (vmLock) { gcLocked(); }
    }

    /**
     * Ejecuta una pasada completa de mark-and-sweep. DEBE llamarse con vmLock
     * adquirido (lo asegura {@link #gc()} y los internals de heapAlloc).
     */
    private void gcLocked() {
        if (gcSuspended) return;   // V4: GC suspendido durante la migración a handles (pasos 2-5)
        int beforeBumpUsed = heapNext - heapStart;
        int beforeFreeListBytes = 0;
        {
            int p = freeListHead;
            while (p != 0) { beforeFreeListBytes += readInt32(p + 4); p = readInt32(p + 8); }
        }

        Set<Integer> valid = buildValidObjectsSet();

        // Roots: pila operacional de TODOS los threads vivos. runOnContext
        // sincroniza tc.sp antes de cualquier llamada que pueda disparar GC,
        // así que aquí basta con leer t.sp para cada uno.
        for (ThreadContext t : threads) {
            if (t.status == ThreadStatus.TERMINATED) continue;
            scanRegion(t.stackBase, t.sp, valid);
            // Ancla de heapAlloc: el objeto recién alocado pero todavía no
            // publicado al stack queda referenciado por t.allocAnchor.
            // Lo tratamos como un root extra para evitar que se libere en
            // tránsito (B1: ventana entre return de heapAlloc y push del
            // caller).
            if (t.allocAnchor != 0) {
                int headerAddr = refDeref(t.allocAnchor) - 4;
                if (valid.contains(headerAddr)) {
                    markObject(headerAddr, valid);
                }
            }
        }

        // #430 — la excepción PREFABRICADA del OOM: construida en el prólogo del
        // run y sin más referencia que ésta. Tiene que sobrevivir TODAS las
        // colectas para que lanzarla, cuando no queda memoria ni para contar el
        // error, no aloje nada. Espejo de gc_mark_phase 2a-bis.
        if (oomExc != 0) {
            int oomHeader = refDeref((int) oomExc) - 4;
            if (valid.contains(oomHeader)) markObject(oomHeader, valid);
        }

        // Roots #302: objptr retenidos por el backend GUI (widgets con bindClick +
        // cola de eventos pendientes). Viven en objetos Java, FUERA de mem[] → el
        // scan conservador no los ve; sin esto un objeto cuyo único holder es el
        // widget se recolecta en vivo (UAF en el siguiente clic). Mismo patrón que
        // allocAnchor: refDeref tolerante (muerto → 0) + filtro por `valid`.
        gui.visitRoots(objptr -> {
            int headerAddr = refDeref(objptr) - 4;
            if (valid.contains(headerAddr)) markObject(headerAddr, valid);
        });

        // Roots H5.c: la cola de eventos vive en objetos Java, fuera de mem[]. El
        // receptor y los argumentos-referencia de un evento pendiente no los ve el
        // scan conservador; sin esto, un objeto cuyo único holder es un evento en
        // cola se recolecta EN VIVO. Mismo agujero que tapó #302 con el GUI.
        for (PendingEvent ev : eventQueue) {
            int h = refDeref(ev.recv) - 4;
            if (valid.contains(h)) markObject(h, valid);
            for (int i = 0; i < ev.args.length; i++) {
                if ((ev.masks & (1 << i)) == 0) continue;
                int ha = refDeref(ev.args[i]) - 4;
                if (valid.contains(ha)) markObject(ha, valid);
            }
        }

        // Roots: data blocks de todos los módulos
        if (moduleManager != null) {
            List<int[]> regions = moduleManager.getDataRegions();
            for (int[] region : regions) {
                scanRegion(region[0], region[0] + region[1], valid);
            }
        }

        // Paso 6 — BARRIDO DE TABLA (handle-aware): un slot VIVO (addr!=0) cuyo bloque
        // quedó SIN marcar es inalcanzable → reciclar el slot (bump gen + addr=0 + free-list)
        // para que un handle rancio a él GRITE (contrato B también para lo que libera el GC).
        // Debe ir ANTES del barrido de heap (que limpia el MARK_BIT). El bloque físico lo
        // libera el barrido de heap de abajo. Bajo el STW del GC → seguro reciclar ya.
        for (int hidx = 1; hidx < handleNext; hidx++) {
            int a = handleAddr[hidx];
            if (a == 0) continue;                          // slot libre
            int hh = a - 4;
            if (hh < heapStart || hh >= heapNext) continue;   // defensivo
            if ((readInt32(hh) & TAG_MARK_BIT) == 0) {     // no alcanzable
                handleKillIdx(hidx);
            }
        }

        // Sweep: reconstruir free list, coalescing adyacentes
        freeListHead = 0;
        int addr = heapStart;
        int aliveBytes = 0;
        int freedBytes = 0;

        int pendingFreeStart = -1; // dirección donde empieza un run de blocks libres consecutivos
        int pendingFreeSize = 0;

        while (addr < heapNext) {
            int size = objectTotalSize(addr);
            if (size <= 0) break;
            int tag = readInt32(addr);
            boolean isFree = (tag & TAG_FREE_BIT) != 0;
            boolean isUnmarked = !isFree && ((tag & TAG_MARK_BIT) == 0);

            if (isFree || isUnmarked) {
                if (pendingFreeStart == -1) {
                    pendingFreeStart = addr;
                    pendingFreeSize = 0;
                }
                pendingFreeSize += size;
                freedBytes += size;
            } else {
                // Vivo: cerrar run pendiente y limpiar mark
                if (pendingFreeStart != -1) {
                    addToFreeList(pendingFreeStart, pendingFreeSize);
                    pendingFreeStart = -1;
                }
                writeInt32(addr, tag & ~TAG_MARK_BIT);
                aliveBytes += size;
            }
            addr += size;
        }
        if (pendingFreeStart != -1) {
            // H3: el run libre FINAL toca heapNext → retroceder heapNext
            // (devolverlo al bump) en vez de meterlo en la free list. Recupera
            // memoria sin compactar. (Los runs intermedios sí van a free list.)
            heapNext = pendingFreeStart;
        }
        // H3: base para el umbral de GC proactivo (heapNext ya retrocedido).
        lastGcHeapNext = heapNext;

        int afterFreeListBytes = 0;
        {
            int p = freeListHead;
            while (p != 0) { afterFreeListBytes += readInt32(p + 4); p = readInt32(p + 8); }
        }

        // Diagnóstico → stderr, como el bpvm_diag de la VM-C. Por stdout
        // rompería el invariante en cuanto un programa colecta: el stdout
        // es del PROGRAMA, byte a byte entre las dos VMs.
        System.err.printf("VM [GC]: heap=%d bytes (alive=%d, libres=%d, bump_remain=%d) | antes free_list=%d%n",
                beforeBumpUsed, aliveBytes, afterFreeListBytes, STACK_BASE - heapNext, beforeFreeListBytes);
    }

    private void addToFreeList(int addr, int size) {
        writeInt32(addr, TAG_FREE_BIT);
        writeInt32(addr + 4, size);
        writeInt32(addr + 8, freeListHead);
        freeListHead = addr;
    }

    // ====================================================================
    // H3: herramientas de medición del heap (SOLO VM-Java). El GC es
    // implementación de VM y el .mod/bytecode es idéntico → lo medido aquí
    // transfiere conceptualmente; no se duplican en la VM C.
    // ====================================================================

    /**
     * Recorre el heap comprometido [heapStart, heapNext) bloque a bloque
     * (igual que el sweep) y devuelve un resumen de fragmentación.
     *   frag = 1 - mayorHueco/totalLibre  (fragmentación EXTERNA):
     *     0   = todo el libre en un único hueco contiguo;
     *     →1  = libre hecho añicos en muchos huecos pequeños.
     * El bump-remaining (reserva contigua al final del heap, aún sin
     * comprometer) se reporta aparte: NO es fragmentación.
     */
    public String heapFragReport() {
        synchronized (vmLock) {
            int addr = heapStart;
            long live = 0, free = 0;
            int holes = 0, largestHole = 0, liveCount = 0;
            while (addr < heapNext) {
                int size = objectTotalSize(addr);
                if (size <= 0) break;
                if ((readInt32(addr) & TAG_FREE_BIT) != 0) {
                    free += size; holes++;
                    if (size > largestHole) largestHole = size;
                } else {
                    live += size; liveCount++;
                }
                addr += size;
            }
            int committed = heapNext - heapStart;
            int bumpRemain = STACK_BASE - heapNext;
            double frag = (free > 0) ? 1.0 - (double) largestHole / (double) free : 0.0;
            double util = (committed > 0) ? (double) live / (double) committed : 0.0;
            return String.format(java.util.Locale.ROOT,
                "frag=%.3f util=%.3f | committed=%d vivos=%d(%dobj) libres=%d holes=%d mayorHole=%d bumpRemain=%d",
                frag, util, committed, live, liveCount, free, holes, largestHole, bumpRemain);
        }
    }

    /**
     * Mapa ASCII del heap comprometido [heapStart, heapNext). Cuantiza en
     * celdas de `cellBytes` bytes (1 char/celda): '.'=libre, '#'=lleno (todo
     * vivo), ':'=semi (mezcla vivo/libre dentro de la celda — surge de la
     * cuantización). Cadena partida cada `cols` columnas.
     */
    public String heapMap(int cols) {
        if (cols < 8) cols = 80;
        synchronized (vmLock) {
            int committed = heapNext - heapStart;
            if (committed <= 0) return "(heap vacío)";
            // cellBytes (potencia de 2) para que el mapa quepa en <= ~30 filas.
            int maxCells = cols * 30;
            int cellBytes = 64;
            while ((committed + cellBytes - 1) / cellBytes > maxCells) cellBytes <<= 1;
            int n = (committed + cellBytes - 1) / cellBytes;
            long[] liveInCell = new long[n];
            int addr = heapStart;
            while (addr < heapNext) {
                int size = objectTotalSize(addr);
                if (size <= 0) break;
                if ((readInt32(addr) & TAG_FREE_BIT) == 0) {   // bloque vivo
                    int s = addr - heapStart, e = s + size;
                    int c0 = s / cellBytes, c1 = (e - 1) / cellBytes;
                    for (int c = c0; c <= c1; c++) {
                        int lo = c * cellBytes, hi = lo + cellBytes;
                        int ov = Math.min(e, hi) - Math.max(s, lo);
                        if (ov > 0) liveInCell[c] += ov;
                    }
                }
                addr += size;
            }
            StringBuilder sb = new StringBuilder(n + n / cols + 96);
            sb.append("heap[").append(heapStart).append("..").append(heapNext).append(") ")
              .append(committed).append("B  1char=").append(cellBytes)
              .append("B  .=libre #=lleno :=semi\n");
            for (int c = 0; c < n; c++) {
                int cellCap = Math.min(cellBytes, committed - c * cellBytes); // última celda parcial
                long lv = liveInCell[c];
                sb.append(lv == 0 ? '.' : (lv >= cellCap ? '#' : ':'));
                if ((c + 1) % cols == 0) sb.append('\n');
            }
            return sb.toString();
        }
    }

    // ====================================================================
    // Bucle principal
    // ====================================================================

    /**
     * Helpers estáticos para leer/escribir enteros en `byte[]` SIN tocar
     * `this.memory`. Llamados desde el hot path de run(), donde cacheamos
     * memory en una local; el JIT inlinea estos métodos eliminando el
     * coste por instrucción.
     */
    /**
     * Reads/writes BIG-ENDIAN i32/i16. Reduced from the obvious 4-shifts form
     * to stay under HotSpot's default MaxInlineSize=35 bytes — sin inlining
     * el coste de method-call por opcode domina el bucle del intérprete.
     * El truco: el byte más significativo se desplaza sin máscara (la
     * promoción byte→int hace sign-extend, que es lo que queremos para el
     * bit alto del int big-endian); los bytes intermedios sí necesitan
     * `& 0xFF` para no contaminar los bits altos.
     */
    private static int readI32(byte[] mem, int addr) {
        return (mem[addr] << 24)
             | ((mem[addr + 1] & 0xFF) << 16)
             | ((mem[addr + 2] & 0xFF) <<  8)
             |  (mem[addr + 3] & 0xFF);
    }
    private static int readI16(byte[] mem, int addr) {
        return (mem[addr] << 8) | (mem[addr + 1] & 0xFF);
    }
    private static void writeI32(byte[] mem, int addr, int v) {
        mem[addr]     = (byte) (v >> 24);
        mem[addr + 1] = (byte) (v >> 16);
        mem[addr + 2] = (byte) (v >>  8);
        mem[addr + 3] = (byte) v;
    }
    // H1.2 (V2): long i64 big-endian (high word en addr, low en addr+4).
    private static long readI64(byte[] mem, int addr) {
        return ((long) readI32(mem, addr) << 32)
             | (readI32(mem, addr + 4) & 0xFFFFFFFFL);
    }
    private static void writeI64(byte[] mem, int addr, long v) {
        writeI32(mem, addr,     (int) (v >>> 32));
        writeI32(mem, addr + 4, (int)  v);
    }

    // ============================================================
    //  Referencia (V4) — réplica del bpref_* de la VM-C.
    //  En Java una ref sigue siendo un int (el user_ref); un wrapper por-ref
    //  mataría el bucle caliente. Pero el KNOB de tamaño (REF_SIZE) y las
    //  fronteras de codificación (refLoad/refStore) e indirección (refDeref)
    //  viven AQUÍ: cambiar el modelo (p.ej. handles) toca estos helpers, no los
    //  call sites. Ver docs/V4_REF_ABSTRACTION.md.
    // ============================================================
    private static final int REF_SIZE     = 8;   // bytes de una ref en memory[]/pila
    private static final int ARR_DATA_OFF = 4;   // offset user_ref → 1er elemento

    /** Lee una referencia de memory[at]. V4/paso4: HANDLE de 64b = [gen:32 | idx|TAG:32].
     *  refLoad devuelve los 64b completos (antes truncaba a low32 y perdía la gen). */
    private static long refLoad(byte[] mem, int at) { return readI64(mem, at); }
    /** Escribe una referencia (handle 64b completo) en memory[at]. */
    private static void refStore(byte[] mem, int at, long ref) {
        writeI64(mem, at, ref);
    }
    /** V4 — bit 30 (en la palabra BAJA) marca "es HANDLE de heap". Las direcciones
     *  directas (null=0 y las CONSTANTES del data block) tienen este bit a 0 y gen=0:
     *  no necesitan tabla ni generación. La memoria es <256KB → una dirección real
     *  jamás tiene el bit 30 puesto. Paso 4: la GENERACIÓN va en la palabra ALTA. */
    private static final int  HANDLE_TAG = 0x40000000;
    private static final long HANDLE_LOW = 0xFFFFFFFFL;
    /** Índice de tabla de un handle (palabra baja sin el tag). */
    private static int handleIdx(long ref) { return ((int) ref) & ~HANDLE_TAG; }
    /** Generación embebida en el handle (palabra alta). */
    private static int handleGenOf(long ref) { return (int) (ref >>> 32); }

    /** V4: registra un objeto de HEAP (dirección user_ref) en la tabla y devuelve su
     *  HANDLE 64b = gen<<32 | (idx|TAG). Paso 4a NEUTRO: monotónico, gen sembrada = 0
     *  (idéntico a antes). En 4b el handle empieza a llevar la gen del slot; en 4c el
     *  idx puede venir de la free-list (reuso), con la gen ya bumpeada del ocupante viejo. */
    private long handleRegister(int addr) {
        // Paso 7 — la tabla (free-list/handleNext) es estado COMPARTIDO: bajo multi-worker
        // dos threads registran a la vez → carrera en handleFreeTop → roban el mismo idx.
        // Serializamos con vmLock (regla explícita, espejo del bpvm_smp_lock de la VM-C).
        // Reentrante: los opcodes que ya envuelven alloc+register en synchronized(vmLock)
        // lo re-adquieren sin problema. El GC usa handleKillIdx bajo vmLock (STW).
        synchronized (vmLock) {
            int idx;
            if (handleFreeTop > 0) {
                idx = handleFreeList[--handleFreeTop];   // 4c: REUSO — slot reciclado (gen ya bumpeada por handleKill)
            } else {
                if (handleNext >= handleAddr.length) {
                    handleAddr = java.util.Arrays.copyOf(handleAddr, handleAddr.length * 2);
                    handleGen  = java.util.Arrays.copyOf(handleGen,  handleGen.length  * 2);
                }
                idx = handleNext++;
                handleGen[idx] = 0;   // slot fresco
                // #430 — LA MARCA (idea de Eduardo): repartir un slot de la zona
                // final anuncia la frontera; la puerta de heapAlloc la consulta y
                // colecta (o crece) ANTES de que la tabla se llene. Una comparación
                // por slot FRESCO — el reuso de free-list ni pasa por aquí, y con
                // reciclados no hay presión que anunciar.
                if (idx + HANDLE_MARCA >= handleAddr.length) handlePressure = true;
            }
            handleAddr[idx] = addr;   // paso 7c: publicado bajo synchronized(vmLock) → release en el monitor-exit
            // Handle 64b = gen(slot)<<32 | (idx|TAG). El deref valida gen(handle)==gen(slot):
            // un handle a un slot RECICLADO (gen vieja) no matchea → grita.
            return ((long) handleGen[idx] << 32) | ((long) (idx | HANDLE_TAG) & HANDLE_LOW);
        }
    }

    /** Contrato B / paso 4c — libera el slot de un handle (owner-free): BUMP de la
     *  generación (handles rancios dejan de matchear → gritan) y RECICLA el índice a la
     *  free-list para reuso. No-op para null/constantes. El TAG_FREE_BIT del bloque físico
     *  evita el doble-free real → aquí reciclamos el slot una sola vez. */
    private void handleKill(long ref) {
        if ((ref & HANDLE_TAG) == 0) return;
        synchronized (vmLock) {   // paso 7: serializa la free-list (espejo del bpvm_smp_lock de la VM-C)
            int idx = handleIdx(ref);
            if (idx <= 0 || idx >= handleNext) return;
            // Paso 7b.1 — FREE CON GENERACIÓN VALIDADA (refuerzo de la maqueta): solo el 1er
            // kill de un handle vivo actúa; un kill RANCIO (slot reciclado, gen no matchea) es
            // no-op — si no bumpearía la gen del ocupante NUEVO y lo corrompería.
            if (handleGen[idx] != handleGenOf(ref)) return;
            handleKillIdx(idx);
        }
    }

    /** Recicla un SLOT de la tabla por índice: bump de generación + addr=0 (marca el
     *  slot LIBRE — el GC lo distingue del vivo por addr!=0) + push a la free-list. Lo
     *  usan owner-free (handleKill) y el barrido de tabla del GC (paso 6). */
    private void handleKillIdx(int idx) {
        handleGen[idx]++;                       // gen bumpeada → handles rancios mueren
        handleAddr[idx] = 0;                     // slot libre en la tabla (paso 6: vivo ⟺ addr!=0)
        if (handleFreeTop >= handleFreeList.length) {
            handleFreeList = java.util.Arrays.copyOf(handleFreeList, handleFreeList.length * 2);
        }
        handleFreeList[handleFreeTop++] = idx;   // reciclar el slot
    }

    /** Contrato B — deref de PROGRAMA: si el ref es un handle a un objeto LIBERADO,
     *  lanza RuntimeError "objeto eliminado" (use-after-free que grita y salta). Se
     *  llama en los opcodes de deref (campo/array/invoke) donde hay tc+sp; el
     *  refDeref interno (GC/free) sigue siendo tolerante a muertos.
     *  Paso 4a NEUTRO: aún compara con el dead-flag (handleGen[idx] != 0), no con la
     *  gen del handle — eso llega en 4b (handleGen[idx] != handleGenOf(ref)). */
    private void requireAlive(ThreadContext tc, int sp, long ref) {
        if ((ref & HANDLE_TAG) == 0) return;           // null/constante: siempre vivo
        int idx = handleIdx(ref);
        // Paso 4b: compara la GENERACIÓN del handle con la del slot. En régimen
        // monotónico (4b) todo handle minteado lleva gen=0, así que esto equivale al
        // dead-flag (handleGen[idx]!=0); en 4c, con reuso, un slot reciclado tiene
        // gen bumpeada y un handle rancio (gen vieja) NO matchea → grita.
        if (idx > 0 && idx < handleNext && handleGen[idx] != handleGenOf(ref)) {
            if (System.getenv("BPVM_DEBUG_UAF") != null) {   // DIAG temporal #302
                System.err.printf("[uaf-diag] ref=0x%016x idx=%d handleGen[idx]=%d genOf(ref)=%d addr=%d%n",
                        ref, idx, handleGen[idx], handleGenOf(ref), handleAddr[idx]);
            }
            tc.sp = sp;
            throwBpRuntimeError(tc, "referencia a objeto eliminado (use-after-free)");
        }
    }

    /** Resuelve una referencia a su dirección física en memory[]. Sin TAG → null(0) o
     *  constante (identidad). Con TAG → tabla (defensivo: fuera de rango → 0). */
    private int refDeref(long ref) {
        if ((ref & HANDLE_TAG) == 0) return (int) ref;
        int idx = handleIdx(ref);
        // Paso 7c — A1: en x86 (host) el load ya es acquire; el dual explícito (VarHandle
        // getAcquire) es Java 9+ → ver nota en la declaración de la tabla. La VM-C sí lleva
        // el atómico C11 porque va a placa ARM/RISC-V.
        return (idx > 0 && idx < handleNext) ? handleAddr[idx] : 0;
    }

    /** #302 (espejo de bpref_regen, VM-C) — reconstruye el HANDLE 64b completo desde
     *  la palabra BAJA (idx|TAG) con la generación VIVA del slot. Para los upcalls del
     *  GUI, cuyo objptr viaja como int por la cola de eventos: escribirlo tal cual a
     *  4B dejaba la palabra alta del arg con BASURA rancia de pila → requireAlive
     *  comparaba esa basura como gen y gritaba (o callaba) POR LOTERÍA. Solo es sólido
     *  porque las raíces GUI del GC garantizan que el objeto no muere mientras el
     *  widget/evento lo retenga (el slot no se recicla → la gen viva es la suya). */
    private long regenRef(int word) {
        if ((word & HANDLE_TAG) == 0) return word & HANDLE_LOW;   // null/constante: identidad
        int idx = handleIdx(word);
        if (idx <= 0 || idx >= handleNext) return word & HANDLE_LOW;
        return (((long) handleGen[idx]) << 32) | (word & HANDLE_LOW);
    }
    /** Longitud (nº de elementos) de un array, de su cabecera. V4: deref primero. */
    private int arrLen(byte[] mem, long arr) { return readI32(mem, refDeref(arr)); }
    /** Offset del elemento idx: deref + cabecera + idx*elem_size. */
    private int arrElem(long arr, int idx, int elemSize) {
        return refDeref(arr) + ARR_DATA_OFF + idx * elemSize;
    }
    /** Offset del campo slot de un objeto: deref + cabecera + slot*4 (slots de 4B;
     *  el valor puede ser 4 u 8B). Layout: user_ref → [class_ptr u32][campos...]. */
    private int fieldAddr(long obj, int slot) {
        return refDeref(obj) + ARR_DATA_OFF + slot * 4;
    }

    // ============================================================
    // GAP-4 — formateo canónico de double/float para print (DPRINT/FPRINT).
    // Punto fijo estilo Str.doubleToString (entero-based: escala por 1e6 a un
    // long, redondea, separa parte entera/decimal, recorta ceros). Para
    // magnitudes fuera del rango seguro del long (|x| >= 1e12 o 0 < |x| < 1e-6)
    // cae a notación científica. TODO en aritmética IEEE determinista (solo
    // *,/,+ por literales exactos y cast a long) → byte-idéntico al puerto C de
    // bpvm_format_double en interp.c. No usa Double.toString ni printf.
    // ============================================================
    static String formatBpDouble(double v) {
        if (Double.isNaN(v)) return "NaN";
        if (Double.isInfinite(v)) return v > 0.0 ? "Infinity" : "-Infinity";
        boolean neg = v < 0.0;
        double ax = neg ? -v : v;
        if (ax == 0.0) return "0";
        if (ax >= 1e12 || ax < 1e-6) return formatBpSci(neg, ax);
        return formatBpFixed(neg, ax);
    }
    private static String trimFixedFrac(StringBuilder sb) {
        int stop = sb.length();
        while (stop > 0 && sb.charAt(stop - 1) == '0') stop--;
        if (stop > 0 && sb.charAt(stop - 1) == '.') stop--;
        return sb.substring(0, stop);
    }
    // Construye "[-]INT.FFFFFF" (6 decimales, padded) a partir de un scaled = round(ax*1e6).
    private static StringBuilder buildScaled(boolean neg, long scaled) {
        long intPart = scaled / 1000000L;
        long frac    = scaled % 1000000L;
        StringBuilder sb = new StringBuilder();
        if (neg) sb.append('-');
        sb.append(Long.toString(intPart));
        sb.append('.');
        String fs = Long.toString(frac);
        for (int k = fs.length(); k < 6; k++) sb.append('0');
        sb.append(fs);
        return sb;
    }
    private static String formatBpFixed(boolean neg, double ax) {
        long scaled = (long) (ax * 1e6 + 0.5);
        if (scaled == 0L) neg = false;          // evita "-0"
        return trimFixedFrac(buildScaled(neg, scaled));
    }
    private static String formatBpSci(boolean neg, double ax) {
        int e = 0;
        double m = ax;
        while (m >= 10.0) { m = m / 10.0; e++; }
        while (m < 1.0)   { m = m * 10.0; e--; }
        long scaled = (long) (m * 1e6 + 0.5);
        if (scaled >= 10000000L) { scaled = 1000000L; e++; }   // el redondeo subió a 10.0
        String mant = trimFixedFrac(buildScaled(neg, scaled));
        return mant + "E" + Integer.toString(e);
    }

    /**
     * Señales con las que {@link #runOnContext} indica al scheduler por qué
     * salió del bucle inner.
     */
    private enum ExitSignal { HALT, THREAD_EXIT, YIELD }

    public void run() {
        System.out.println("\n=== INICIANDO EJECUCIÓN DE LA VM ===");
        System.out.printf("    heapStart=%d  STACK_BASE=%d  (heap libre=%d bytes) workers=%d%n",
                heapStart, STACK_BASE, STACK_BASE - heapStart, numWorkers);
        // Propagamos PC/CS iniciales (puestos por setPC/setCS antes de run())
        // al ThreadContext main. A partir de aquí runOnContext opera SOLO
        // sobre tc.X; this.PC/SP/BP/CS quedan obsoletos durante la ejecución.
        ThreadContext main = threads.get(0);
        main.pc = this.PC;
        main.sp = this.SP;
        main.bp = this.BP;
        main.cs = this.CS;
        // #430 — LA EXCEPCIÓN PREFABRICADA (idea de Eduardo): el OOM se fabrica
        // AQUÍ, antes de arrancar el programa, cuando fabricarlo es gratis. Así,
        // cuando de verdad no quede memoria ni para construir el error, lanzarla
        // no aloja NADA y el OOM sigue siendo atrapable. Espejo de bpvm_run.
        // Raíz propia del GC (scanRoots). Se construye con el mecanismo normal y
        // se le quita el efecto: el prólogo no es un error.
        oomExc = 0;
        try {
            throwBpRuntimeError(main, "No space in heap");
        } catch (BpExceptionPending pend) {
            oomExc = readI64(memory, main.sp - 8);   // el handle 64b que empujó
            main.sp -= 8;                            // deshacer el push
            main.allocAnchor = 0;
        } catch (BpThreadFault ignored) {
            // Sin RuntimeError exportado (mod legado): no hay prefabricada, y el
            // camino de siempre (BpThreadFault) sigue cubriendo el caso.
        }

        // main arranca RUNNABLE en la cola; el primer worker que lo pille lo ejecuta.
        synchronized (vmLock) {
            main.status = ThreadStatus.RUNNABLE;
            if (!runQueue.contains(0)) runQueue.addFirst(0);
            vmShutdown = false;
        }
        // Spawneamos N workers Java reales. Cada uno toma threads BP de la
        // cola y los ejecuta hasta yield/halt/exit. Esto es la "VM dos cores".
        Thread[] workers = new Thread[numWorkers];
        for (int i = 0; i < numWorkers; i++) {
            workers[i] = new Thread(new WorkerLoop(i), "bpgenvm-worker-" + i);
            workers[i].setDaemon(false);
            workers[i].start();
        }
        // Timer preemptivo: cada quantumMs marca yieldRequested=true en
        // los tcs RUNNING. Daemon thread (no impide la salida de la JVM).
        startPreemptTimer();
        try {
            // Esperamos a que todos los workers terminen (vmShutdown=true).
            for (Thread w : workers) {
                try { w.join(); } catch (InterruptedException ignored) { Thread.currentThread().interrupt(); }
            }
        } finally {
            stopPreemptTimer();
        }
        // Vuelco final por compatibilidad con callers que lean this.X.
        this.PC = main.pc;
        this.SP = main.sp;
        this.BP = main.bp;
        this.CS = main.cs;
        // Asegura que el último output del programa BP llegue al sink
        // antes de que la VM termine — crítico cuando el sink es un
        // socket cuyo lado lector cierra al ver el evento "exited".
        programOut.flush();
        // #342 — GUARDIÁN DE SALIDA. Si aún queda algo en la cola es que su
        // destinatario no existe, o que un handler post-mortem levantó eventos
        // después de agotarse el presupuesto. Tirarlos es legítimo; tirarlos EN
        // SILENCIO es la familia de bug que costó #326. Que lo diga.
        int evLeft;
        synchronized (vmLock) { evLeft = eventQueue.size(); }
        if (evLeft > 0) {
            System.err.println("[bpvm] fin de ejecución con " + evLeft
                    + " evento(s) sin atender (destinatario muerto o encolados"
                    + " por un handler tardío)");
        }
        System.out.println("=== FIN DE LA EJECUCIÓN ===");
    }

    /**
     * Worker Java: simula un core físico. Pulla ThreadContexts (threads BP)
     * de la runQueue y los ejecuta hasta que ceden (YIELD), terminen
     * (THREAD_EXIT) o tumben la VM (HALT). Tantas instancias como
     * {@link #numWorkers} configurados.
     */
    private final class WorkerLoop implements Runnable {
        private final int workerId;
        WorkerLoop(int id) { this.workerId = id; }
        @Override public void run() {
            try {
                while (true) {
                    ThreadContext tc = null;
                    synchronized (vmLock) {
                        while (true) {
                            if (vmShutdown) return;
                            // P-run-stop (#257) — KILL: shutdown coordinado
                            // de todos los workers (los threads BP cesan
                            // entre opcodes; heap consistente).
                            if (killRequested) {
                                vmShutdown = true;
                                vmLock.notifyAll();
                                return;
                            }
                            // Si otro worker pidió GC stop-the-world, parquea
                            // aquí antes de coger nuevo thread.
                            if (stopTheWorld) {
                                try { vmLock.wait(); }
                                catch (InterruptedException ie) {
                                    Thread.currentThread().interrupt(); return;
                                }
                                continue;
                            }
                            tc = pickNextRunnableTc();
                            if (tc != null) break;
                            if (!anyThreadAlive()) {
                                vmShutdown = true;
                                vmLock.notifyAll();
                                return;
                            }
                            long earliest = earliestSleepWakeMs();
                            long now = System.currentTimeMillis();
                            try {
                                if (earliest == Long.MAX_VALUE) {
                                    vmLock.wait();          // sólo joins pendientes
                                } else {
                                    long delta = earliest - now;
                                    if (delta > 0) vmLock.wait(delta);
                                }
                            } catch (InterruptedException ie) {
                                Thread.currentThread().interrupt();
                                return;
                            }
                        }
                        tc.status = ThreadStatus.RUNNING;
                        // H5.c — ENTRE QUANTA: si hay un evento para este thread,
                        // le inyectamos el frame del handler antes de darle CPU.
                        // Dentro del vmLock y con el tc ya asignado a este worker:
                        // nadie más lo está ejecutando, su estado es el bueno.
                        drainOneEvent(tc);
                    }
                    ExitSignal sig;
                    currentTcLocal.set(tc);
                    try {
                        sig = runOnContext(tc);
                    } catch (BpThreadFault tf) {
                        // Fallo BP localizado a este thread (e.g. violación de
                        // Mutex). Imprimimos el mensaje, terminamos SOLO este
                        // thread y devolvemos al scheduler para que otros
                        // threads sigan ejecutándose. Si era el main, sí
                        // tumbamos la VM porque no hay quien continúe el
                        // programa principal.
                        System.err.println("[bpgenvm worker " + workerId + ", tid="
                                + tc.id + "] " + tf.getMessage());
                        dumpFault(workerId, tc, "BpThreadFault", tf);
                        // A1.7: notificar al IDE remoto si hay listener cableado.
                        emitDebugEvent(new edu.bpgenvm.vm.debug.ExceptionEvent(
                                tc.id, tf.getMessage(), ""));
                        synchronized (vmLock) {
                            if (tc.id == 0) {
                                // main: shutdown coordinado
                                vmShutdown = true;
                                vmLock.notifyAll();
                                return;
                            }
                            // worker thread: termina sólo este. Liberamos
                            // mutexes que pudiera tener tomados para no
                            // dejar a otros threads bloqueados eternamente.
                            releaseMutexesOwnedBy(tc.id);
                            terminateThread(tc);
                            vmLock.notifyAll();
                        }
                        continue;
                    } catch (RuntimeException ex) {
                        // Excepción no atrapada en BP: imprimimos y tumbamos la VM.
                        System.err.println("[bpgenvm worker " + workerId + ", tid="
                                + tc.id + "] " + ex.getMessage());
                        dumpFault(workerId, tc, "RuntimeException", ex);
                        emitDebugEvent(new edu.bpgenvm.vm.debug.ExceptionEvent(
                                tc.id, String.valueOf(ex.getMessage()), ""));
                        synchronized (vmLock) {
                            vmShutdown = true;
                            vmLock.notifyAll();
                        }
                        return;
                    }
                    synchronized (vmLock) {
                        switch (sig) {
                            case HALT:
                                // Sólo el main puede haber emitido HALT (verificado
                                // en runOnContext).
                                // #346 — HALT termina ESE THREAD, no la VM: el
                                // programa acaba cuando acaban TODOS los threads.
                                // Antes miVM tumbaba la VM aquí y un thread que
                                // sobreviviera a main se quedaba a medias; la VM-C
                                // siempre les dejó terminar, y miVM converge hacia
                                // ella. El bucle sale solo cuando no queda nadie
                                // vivo (abajo, !anyThreadAlive()).
                                // terminateThread ya contempla #342: si main deja
                                // eventos SUYOS sin atender, no muere todavía.
                                terminateThread(tc);
                                vmLock.notifyAll();
                                break;
                            case THREAD_EXIT:
                                terminateThread(tc);
                                vmLock.notifyAll();
                                break;
                            case YIELD:
                                // Dos orígenes posibles:
                                //   1) Builtin yield/sleep/join: ya cambió tc.status
                                //      (RUNNABLE+addLast, BLOCKED_SLEEP, o BLOCKED_JOIN).
                                //   2) Preempt timer: sólo activó yieldRequested,
                                //      tc.status sigue RUNNING → re-encolamos aquí.
                                if (tc.status == ThreadStatus.RUNNING) {
                                    tc.status = ThreadStatus.RUNNABLE;
                                    runQueue.addLast(tc.id);
                                }
                                vmLock.notifyAll();
                                break;
                        }
                    }
                }
            } catch (Throwable t) {
                synchronized (vmLock) {
                    vmShutdown = true;
                    vmLock.notifyAll();
                }
                throw t;
            } finally {
                // N4 — limpia el ThreadLocal del worker para que el último tc
                // ejecutado no quede referenciado tras shutdown (evita un
                // pequeño "leak" del context cuando el ClassLoader del worker
                // se mantiene vivo en escenarios embebidos o tests JUnit).
                currentTcLocal.remove();
            }
        }
    }

    /**
     * Arranca el timer preemptivo (si quantumMs > 0). Cada quantum, marca
     * tc.yieldRequested=true en todos los ThreadContexts RUNNING para forzar
     * un context switch en el próximo opcode. Los workers ven el flag en su
     * while-condition y abandonan runOnContext con señal YIELD.
     */
    private void startPreemptTimer() {
        if (quantumMs <= 0) return;
        preemptTimer = java.util.concurrent.Executors.newSingleThreadScheduledExecutor(r -> {
            Thread t = new Thread(r, "bpgenvm-preempt-timer");
            t.setDaemon(true);
            return t;
        });
        preemptTimer.scheduleAtFixedRate(() -> {
            if (vmShutdown) return;
            synchronized (vmLock) {
                for (ThreadContext t : threads) {
                    if (t.status == ThreadStatus.RUNNING) {
                        t.yieldRequested = true;
                    }
                }
            }
        }, quantumMs, quantumMs, java.util.concurrent.TimeUnit.MILLISECONDS);
    }

    private void stopPreemptTimer() {
        if (preemptTimer != null) {
            preemptTimer.shutdownNow();
            preemptTimer = null;
        }
    }

    /**
     * Bucle del intérprete que opera sobre UN ThreadContext.
     *
     * Lee/escribe SÓLO en tc — nada de this.PC/SP/BP/CS. Imprescindible para
     * Phase 4b (multi-worker): cada java worker llama a este método con su
     * propio tc y los dos pueden ejecutar simultáneamente sin race conditions
     * (el heap sigue siendo compartido y necesitará locks en Phase 4c).
     *
     * Cacheamos los registros y el byte[] memoria como locales. El JIT los
     * promueve a registros físicos y elimina los getfield/putfield del hot
     * path. Sync (tc.pc = pc, ...) antes de cualquier llamada externa que
     * pudiera observar el contexto (moduleManager.*, heapAlloc/gc,
     * freeOwnedObject, dispatchBuiltin, buildUnhandledExceptionMessage) y al
     * salir del bucle.
     *
     * El estado de exception-handling (ehHandlerPc, ehSavedSp, ehSavedBp,
     * ehSavedCs, ehExpectedClass) también vive en locales por perf, y se
     * sincroniza con tc al salir.
     *
     * @return motivo de salida: HALT (sólo main), THREAD_EXIT (worker que
     *         retornó de su run()), YIELD (builtin pidió ceder CPU).
     */
    private ExitSignal runOnContext(ThreadContext tc) {
        final byte[] mem = this.memory;
        int pc = tc.pc;
        int sp = tc.sp;
        int bp = tc.bp;
        int cs = tc.cs;
        int ehHandlerPc     = tc.ehHandlerPc;
        int ehSavedSp       = tc.ehSavedSp;
        int ehSavedBp       = tc.ehSavedBp;
        int ehSavedCs       = tc.ehSavedCs;
        int ehExpectedClass = tc.ehExpectedClass;
        final Deque<int[]> handlerStack = tc.handlerStack;
        ExitSignal exitSignal = ExitSignal.YIELD;  // si salimos por yieldRequested
        boolean running = true;
        try {
        while (running && !tc.yieldRequested) {
            int currentPC = pc;

            // ---- Safepoint stop-the-world para GC (B1) ----
            // stopTheWorld es volatile: lo levanta el worker que va a
            // correr gcLocked. Si lo vemos activo, sincronizamos toda
            // nuestra cache local (pc/sp/bp/cs) al ThreadContext y
            // pedimos yield. La WorkerLoop, al volver, parquea hasta
            // que stopTheWorld vuelva a false.
            // P-run-stop (#257): killRequested usa el MISMO safepoint —
            // el worker cede aquí y la WorkerLoop hace el shutdown.
            if (stopTheWorld || killRequested) {
                tc.pc = currentPC; tc.sp = sp; tc.bp = bp; tc.cs = cs;
                tc.yieldRequested = true;
                break;
            }

            // ---- DebugHook: notificación al cambiar de línea origen. ----
            // El check es un único getfield+null; sólo paga lookup si hook
            // está instalado. Cuando lo está, el hook puede bloquear el
            // worker (paro de step/breakpoint) hasta que el IDE pida
            // continuar. Sincronizamos tc.X antes para que el hook pueda
            // inspeccionar el estado completo.
            if (debugHook != null && moduleManager != null) {
                int dbgLine = moduleManager.getLineForPc(currentPC);
                if (dbgLine > 0 && dbgLine != tc.lastDebugLine) {
                    tc.lastDebugLine = dbgLine;
                    tc.pc = currentPC; tc.sp = sp; tc.bp = bp; tc.cs = cs;
                    String src = moduleManager.getSourceForPc(currentPC);
                    DebugContext dctx = new DebugContext(
                            this, tc.id, currentPC, dbgLine, src,
                            bp, sp, cs, tc.stackBase);
                    debugHook.onLineChange(dctx);
                    // Tras volver del hook, recargamos por si... (no aplica
                    // hoy; el hook no muta tc.pc/sp/bp/cs, pero futuro:
                    // edit-and-continue sí podría).
                }
            }

            int rawOp = mem[currentPC] & 0xFF;
            pc++;

            if (tracing) {
                tc.pc = pc; tc.sp = sp; tc.bp = bp; tc.cs = cs;
                OpCode op = OpCode.fromByte((byte) rawOp);
                System.out.printf("PC: %d | Opcode: %s (0x%02X) | CS: %d | SP: %d | tid: %d%n",
                        currentPC, op.name(), rawOp, cs, sp, tc.id);
            }

            try {
            switch (rawOp) {
                case 0x00: // HALT  (sólo legal en el thread main)
                    if (tc.id != 0) {
                        tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                        throw new RuntimeException("HALT en thread no-main (tid="
                                + tc.id + " en PC " + currentPC + "); "
                                + "los workers deben terminar con RET de su run()");
                    }
                    running = false;
                    exitSignal = ExitSignal.HALT;
                    break;

                case 0x6F: { // EVENT_RETURN — H5.c: vuelta de un handler de evento
                    // El RET del handler ya restauró bp y cs (le pasamos los
                    // buenos) y dejó en la pila su valor de retorno. Aquí lo
                    // tiramos junto con las 4 bytes del PC de reanudación que la
                    // inyección guardó DEBAJO de los argumentos, y seguimos donde
                    // estábamos: el código interrumpido no se entera de nada.
                    sp -= 8;
                    pc = readI32(mem, sp);
                    if (tc.evDepth > 0) tc.evDepth--;
                    break;
                }

                case 0x70: // THREAD_EXIT (sentinela de salida de workers)
                    // Termina sólo el thread actual; la VM sigue corriendo
                    // mientras quede al menos un thread vivo. Marca para
                    // que el outer scheduler haga terminate + switch.
                    running = false;
                    exitSignal = ExitSignal.THREAD_EXIT;
                    tc.status = ThreadStatus.TERMINATED;
                    break;

                case 0x01: { // PUSH
                    int val = readI32(mem, pc); pc += 4;
                    writeI32(mem, sp, val); sp += 4;
                    break;
                }

                case 0x02: { // ADD
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a + b); sp += 4;
                    break;
                }

                case 0x03: { // PRINT
                    sp -= 4; int v = readI32(mem, sp);
                    System.out.println("VM [PRINT]: " + v);
                    break;
                }

                case 0x04: { // GET_GLOBAL
                    short off = (short) readI16(mem, pc); pc += 2;
                    writeI32(mem, sp, readI32(mem, cs + off)); sp += 4;
                    break;
                }
                case 0x05: { // SET_GLOBAL
                    short off = (short) readI16(mem, pc); pc += 2;
                    sp -= 4; int v = readI32(mem, sp);
                    writeI32(mem, cs + off, v);
                    break;
                }

                case 0x06: { // CALL_EXT
                    if (moduleManager == null) throw new RuntimeException("ModuleManager no conectado.");
                    short extFuncIdx = (short) readI16(mem, pc); pc += 2;
                    this.PC = pc; this.SP = sp; this.BP = bp; this.CS = cs;
                    int extTableBase = moduleManager.getExternalTableAddressForCS(cs);
                    int targetCellAddr = extTableBase + (extFuncIdx * 4);
                    int targetPC = readI32(mem, targetCellAddr);

                    writeI32(mem, sp, pc); sp += 4;
                    writeI32(mem, sp, bp); sp += 4;
                    writeI32(mem, sp, cs); sp += 4;
                    bp = sp;
                    pc = targetPC;
                    cs = moduleManager.getModuleBaseFromPC(targetPC);
                    break;
                }
                case 0x07: { // CALL (intra-módulo)
                    int targetRel = readI32(mem, pc); pc += 4;
                    writeI32(mem, sp, pc); sp += 4;
                    writeI32(mem, sp, bp); sp += 4;
                    writeI32(mem, sp, cs); sp += 4;
                    bp = sp;
                    pc = cs + targetRel;
                    break;
                }
                case 0x08: { // RET
                    int paramsCount = mem[pc] & 0xFF;
                    pc++;

                    sp -= 4; int returnValue = readI32(mem, sp);

                    int pcAnterior = readI32(mem, bp - 12);
                    int bpAnterior = readI32(mem, bp - 8);
                    int csAnterior = readI32(mem, bp - 4);

                    int targetCallerSP = bp - 12 - (paramsCount * 4);

                    pc = pcAnterior;
                    bp = bpAnterior;
                    cs = csAnterior;
                    sp = targetCallerSP;

                    writeI32(mem, sp, returnValue); sp += 4;
                    break;
                }
                case 0x90: { // LRET — H1.2 (V2): return value de 8 bytes (long)
                    int paramsCount = mem[pc] & 0xFF; pc++;
                    sp -= 8; long retL = readI64(mem, sp);
                    int pcAnt = readI32(mem, bp - 12);
                    int bpAnt = readI32(mem, bp - 8);
                    int csAnt = readI32(mem, bp - 4);
                    int targetCallerSP = bp - 12 - (paramsCount * 4);
                    pc = pcAnt; bp = bpAnt; cs = csAnt; sp = targetCallerSP;
                    writeI64(mem, sp, retL); sp += 8;
                    break;
                }

                case 0x09: { // GET_LOCAL
                    short offsetGet = (short) readI16(mem, pc); pc += 2;
                    writeI32(mem, sp, readI32(mem, bp + offsetGet)); sp += 4;
                    break;
                }
                case 0x0A: { // SET_LOCAL
                    short offsetSet = (short) readI16(mem, pc); pc += 2;
                    sp -= 4; int v = readI32(mem, sp);
                    writeI32(mem, bp + offsetSet, v);
                    break;
                }

                case 0x0B: { // EQ
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a == b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x0C: { // LT
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a <  b ? 1 : 0); sp += 4;
                    break;
                }

                case 0x0D: { // JUMP
                    int currentInstructionAddr = pc - 1;
                    int relativeOffset = readI32(mem, pc);
                    pc = currentInstructionAddr + relativeOffset;
                    break;
                }
                case 0x0E: { // JUMP_IF_FALSE
                    int currentInstructionAddr = pc - 1;
                    int relativeOffset = readI32(mem, pc); pc += 4;
                    sp -= 4; int condition = readI32(mem, sp);
                    if (condition == 0) pc = currentInstructionAddr + relativeOffset;
                    break;
                }

                case 0x0F: { // ENTER
                    int localsBytes = readI16(mem, pc); pc += 2;
                    sp += localsBytes;
                    break;
                }

                case 0x10: { // SUB
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a - b); sp += 4;
                    break;
                }
                case 0x11: { // MUL
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a * b); sp += 4;
                    break;
                }
                case 0x12: { // DIV
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    if (b == 0) { tc.sp=sp; throwBpRuntimeError(tc, "División por cero"); }
                    writeI32(mem, sp, a / b); sp += 4;
                    break;
                }
                case 0x13: { // MOD
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    if (b == 0) { tc.sp=sp; throwBpRuntimeError(tc, "Módulo por cero"); }
                    writeI32(mem, sp, a % b); sp += 4;
                    break;
                }
                case 0x14: { // NEG
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, -a); sp += 4;
                    break;
                }

                case 0x15: { // AND
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, (a != 0 && b != 0) ? 1 : 0); sp += 4;
                    break;
                }
                case 0x16: { // OR
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, (a != 0 || b != 0) ? 1 : 0); sp += 4;
                    break;
                }
                case 0x17: { // NOT
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a == 0 ? 1 : 0); sp += 4;
                    break;
                }

                case 0x18: { // GT
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a >  b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x19: { // GE
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a >= b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x1A: { // LE
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a <= b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x1B: { // NEQ
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a != b ? 1 : 0); sp += 4;
                    break;
                }

                case 0x1C: { // LEA_GLOBAL — H1.2a: dirección de global = ref 8 bytes
                    short off = (short) readI16(mem, pc); pc += 2;
                    writeI64(mem, sp, ((long)(cs + off)) & 0xFFFFFFFFL); sp += 8;
                    break;
                }
                case 0x23: { // LEA_LOCAL — H1.2a: dirección de local (array inline) = ref 8 bytes
                    short off = (short) readI16(mem, pc); pc += 2;
                    writeI64(mem, sp, ((long)(bp + off)) & 0xFFFFFFFFL); sp += 8;
                    break;
                }

                // --- Arrays con allocator GC-aware ---
                case 0x1D: { // NEWARRAY
                    sp -= 4; int size = readI32(mem, sp);
                    if (size < 0) { tc.sp=sp; throwBpRuntimeError(tc, "NEWARRAY: tamaño negativo (" + size + ")"); }
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    synchronized (vmLock) {
                        int ref = heapAlloc(size * 4, TYPE_ARRAY_I32);
                        writeI32(mem, ref, size);
                                                refStore(mem, sp, handleRegister(ref)); sp += REF_SIZE;
                        tc.sp = sp;
                    }
                    break;
                }
                case 0x38: { // NEWARRAY_I8
                    sp -= 4; int size = readI32(mem, sp);
                    if (size < 0) { tc.sp=sp; throwBpRuntimeError(tc, "NEWARRAY_I8: tamaño negativo (" + size + ")"); }
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    synchronized (vmLock) {
                        int ref = heapAlloc(size, TYPE_ARRAY_I8);
                        writeI32(mem, ref, size);
                                                refStore(mem, sp, handleRegister(ref)); sp += REF_SIZE;
                        tc.sp = sp;
                    }
                    break;
                }
                case 0x39: { // NEWARRAY_I16
                    sp -= 4; int size = readI32(mem, sp);
                    if (size < 0) { tc.sp=sp; throwBpRuntimeError(tc, "NEWARRAY_I16: tamaño negativo (" + size + ")"); }
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    synchronized (vmLock) {
                        int ref = heapAlloc(size * 2, TYPE_ARRAY_I16);
                        writeI32(mem, ref, size);
                                                refStore(mem, sp, handleRegister(ref)); sp += REF_SIZE;
                        tc.sp = sp;
                    }
                    break;
                }

                case 0x1E: { // ALOAD
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) {
                        tc.sp = sp;
                        throwBpRuntimeError(tc, "ALOAD: índice fuera de rango "
                                + idx + " (length=" + length + ")");
                    }
                    writeI32(mem, sp, readI32(mem, arrElem(arr, idx, 4))); sp += 4;
                    break;
                }
                case 0x1F: { // ASTORE
                    sp -= 4; int val = readI32(mem, sp);
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) {
                        tc.sp = sp;
                        throwBpRuntimeError(tc, "ASTORE: índice fuera de rango "
                                + idx + " (length=" + length + ")");
                    }
                    writeI32(mem, arrElem(arr, idx, 4), val);
                    break;
                }
                case 0x20: { // ALEN
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    writeI32(mem, sp, arrLen(mem, arr)); sp += 4;
                    break;
                }

                case 0x21: { // PRINT_CHAR
                    sp -= 4; int c = readI32(mem, sp);
                    programOut.writeChar((char) c);
                    break;
                }
                case 0x22: { // PRINT_STRING (legacy: con \n al final)
                    sp -= REF_SIZE; long ref = refLoad(mem, sp);
                    programOut.writeText(readVmString(ref));  // H2: decodifica UTF-8
                    programOut.newline();
                    break;
                }

                case 0x24: { // JUMP8
                    int currentInstructionAddr = pc - 1;
                    int relativeOffset = (byte) mem[pc];
                    pc = currentInstructionAddr + relativeOffset;
                    break;
                }
                case 0x25: { // JUMP16
                    int currentInstructionAddr = pc - 1;
                    int relativeOffset = (short) readI16(mem, pc);
                    pc = currentInstructionAddr + relativeOffset;
                    break;
                }
                case 0x26: { // JUMP_IF_FALSE8
                    int currentInstructionAddr = pc - 1;
                    int relativeOffset = (byte) mem[pc];
                    pc += 1;
                    sp -= 4; int condition = readI32(mem, sp);
                    if (condition == 0) pc = currentInstructionAddr + relativeOffset;
                    break;
                }
                case 0x27: { // JUMP_IF_FALSE16
                    int currentInstructionAddr = pc - 1;
                    int relativeOffset = (short) readI16(mem, pc);
                    pc += 2;
                    sp -= 4; int condition = readI32(mem, sp);
                    if (condition == 0) pc = currentInstructionAddr + relativeOffset;
                    break;
                }

                case 0x28: { // FPUSH
                    int bits = readI32(mem, pc); pc += 4;
                    writeI32(mem, sp, bits); sp += 4;
                    break;
                }
                case 0x29: { // FADD
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, Float.floatToRawIntBits(a + b)); sp += 4;
                    break;
                }
                case 0x2A: { // FSUB
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, Float.floatToRawIntBits(a - b)); sp += 4;
                    break;
                }
                case 0x2B: { // FMUL
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, Float.floatToRawIntBits(a * b)); sp += 4;
                    break;
                }
                case 0x2C: { // FDIV
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, Float.floatToRawIntBits(a / b)); sp += 4;
                    break;
                }
                case 0x2D: { // FMOD
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, Float.floatToRawIntBits(a % b)); sp += 4;
                    break;
                }
                case 0x2E: { // FNEG
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, Float.floatToRawIntBits(-a)); sp += 4;
                    break;
                }
                case 0x2F: { // FEQ
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, a == b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x30: { // FNEQ
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, a != b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x31: { // FLT
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, a <  b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x32: { // FLE
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, a <= b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x33: { // FGT
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, a >  b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x34: { // FGE
                    sp -= 4; float b = Float.intBitsToFloat(readI32(mem, sp));
                    sp -= 4; float a = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, a >= b ? 1 : 0); sp += 4;
                    break;
                }
                case 0x35: { // FPRINT (legacy demo-only; print real usa FPRINT_NONL+PRINT_NL)
                    sp -= 4; float v = Float.intBitsToFloat(readI32(mem, sp));
                    // Debug verboso, simétrico con PRINT (0x03). Demo-only: los .mod
                    // compilados usan 0x57 (FPRINT_NONL); no afecta a la paridad dual-VM.
                    System.out.println("VM [FPRINT]: " + formatBpDouble((double) v));
                    break;
                }
                case 0x36: { // I2F
                    sp -= 4; int v = readI32(mem, sp);
                    writeI32(mem, sp, Float.floatToRawIntBits((float) v)); sp += 4;
                    break;
                }
                case 0x37: { // F2I
                    sp -= 4; float fv = Float.intBitsToFloat(readI32(mem, sp));
                    writeI32(mem, sp, (int) fv); sp += 4;
                    break;
                }

                case 0x3A: { // ALOAD_I8
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ALOAD_I8: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    writeI32(mem, sp, (int) mem[arrElem(arr, idx, 1)]); sp += 4;
                    break;
                }
                case 0x3B: { // ALOAD_U8
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ALOAD_U8: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    writeI32(mem, sp, mem[arrElem(arr, idx, 1)] & 0xFF); sp += 4;
                    break;
                }
                case 0x3C: { // ALOAD_I16
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ALOAD_I16: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    int addr = arrElem(arr, idx, 2);
                    int raw = ((mem[addr] & 0xFF) << 8) | (mem[addr + 1] & 0xFF);
                    writeI32(mem, sp, (short) raw); sp += 4;
                    break;
                }
                case 0x3D: { // ALOAD_U16
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ALOAD_U16: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    int addr = arrElem(arr, idx, 2);
                    int raw = ((mem[addr] & 0xFF) << 8) | (mem[addr + 1] & 0xFF);
                    writeI32(mem, sp, raw); sp += 4;
                    break;
                }

                case 0x3E: { // ASTORE_I8
                    sp -= 4; int val = readI32(mem, sp);
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ASTORE_I8: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    mem[arrElem(arr, idx, 1)] = (byte) val;
                    break;
                }
                case 0x3F: { // ASTORE_I16
                    sp -= 4; int val = readI32(mem, sp);
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ASTORE_I16: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    int addr = arrElem(arr, idx, 2);
                    mem[addr]     = (byte) ((val >> 8) & 0xFF);
                    mem[addr + 1] = (byte) ( val       & 0xFF);
                    break;
                }

                case 0x40: { // GET_GLOBAL_I8
                    short off = (short) readI16(mem, pc); pc += 2;
                    writeI32(mem, sp, (int) mem[cs + off]); sp += 4;
                    break;
                }
                case 0x41: { // GET_GLOBAL_U8
                    short off = (short) readI16(mem, pc); pc += 2;
                    writeI32(mem, sp, mem[cs + off] & 0xFF); sp += 4;
                    break;
                }
                case 0x42: { // GET_GLOBAL_I16
                    short off = (short) readI16(mem, pc); pc += 2;
                    int addr = cs + off;
                    int raw = ((mem[addr] & 0xFF) << 8) | (mem[addr + 1] & 0xFF);
                    writeI32(mem, sp, (short) raw); sp += 4;
                    break;
                }
                case 0x43: { // GET_GLOBAL_U16
                    short off = (short) readI16(mem, pc); pc += 2;
                    int addr = cs + off;
                    int raw = ((mem[addr] & 0xFF) << 8) | (mem[addr + 1] & 0xFF);
                    writeI32(mem, sp, raw); sp += 4;
                    break;
                }
                case 0x44: { // SET_GLOBAL_I8
                    short off = (short) readI16(mem, pc); pc += 2;
                    sp -= 4; int v = readI32(mem, sp);
                    mem[cs + off] = (byte) v;
                    break;
                }
                case 0x45: { // SET_GLOBAL_I16
                    short off = (short) readI16(mem, pc); pc += 2;
                    sp -= 4; int val = readI32(mem, sp);
                    int addr = cs + off;
                    mem[addr]     = (byte) ((val >> 8) & 0xFF);
                    mem[addr + 1] = (byte) ( val       & 0xFF);
                    break;
                }

                case 0x46: { // BAND
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a & b); sp += 4; break;
                }
                case 0x47: { // BOR
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a | b); sp += 4; break;
                }
                case 0x48: { // BXOR
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a ^ b); sp += 4; break;
                }
                case 0x49: { // BNOT
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, ~a); sp += 4; break;
                }
                case 0x4A: { // SHL
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a <<  (b & 31)); sp += 4; break;
                }
                case 0x4B: { // SHR_S
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a >>  (b & 31)); sp += 4; break;
                }
                case 0x4C: { // SHR_U
                    sp -= 4; int b = readI32(mem, sp);
                    sp -= 4; int a = readI32(mem, sp);
                    writeI32(mem, sp, a >>> (b & 31)); sp += 4; break;
                }

                case 0x4D: { // I32_TO_I8
                    sp -= 4; int v = readI32(mem, sp);
                    if (v < -128 || v > 127) { tc.sp=sp; throwBpRuntimeError(tc, "I32_TO_I8: valor fuera de rango " + v); }
                    writeI32(mem, sp, v); sp += 4;
                    break;
                }
                case 0x4E: { // I32_TO_U8
                    sp -= 4; int v = readI32(mem, sp);
                    if (v < 0 || v > 255) { tc.sp=sp; throwBpRuntimeError(tc, "I32_TO_U8: valor fuera de rango " + v); }
                    writeI32(mem, sp, v); sp += 4;
                    break;
                }
                case 0x4F: { // I32_TO_I16
                    sp -= 4; int v = readI32(mem, sp);
                    if (v < Short.MIN_VALUE || v > Short.MAX_VALUE) { tc.sp=sp; throwBpRuntimeError(tc, "I32_TO_I16: valor fuera de rango " + v); }
                    writeI32(mem, sp, v); sp += 4;
                    break;
                }
                case 0x50: { // I32_TO_U16
                    sp -= 4; int v = readI32(mem, sp);
                    if (v < 0 || v > 0xFFFF) { tc.sp=sp; throwBpRuntimeError(tc, "I32_TO_U16: valor fuera de rango " + v); }
                    writeI32(mem, sp, v); sp += 4;
                    break;
                }

                case 0x51: { // GC_COLLECT
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    gc();
                    break;
                }

                // --- Clases y objetos ---
                case 0x52: { // NEW_OBJECT
                    short csOff = (short) readI16(mem, pc); pc += 2;
                    int classPtr = cs + csOff;
                    int numFields = readI16(mem, classPtr + CLS_OFF_NUM_FIELDS) & 0xFFFF;
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    // B1: mantenemos vmLock hasta publicar el ref al stack
                    // y sincronizar tc.sp, para cerrar la ventana en que
                    // otro worker pudiera disparar GC con el ref en tránsito.
                    synchronized (vmLock) {
                        int ref = heapAlloc(numFields * 4, TYPE_OBJECT);
                        writeI32(mem, ref, classPtr);
                        for (int i = 0; i < numFields; i++) {
                            writeI32(mem, ref + 4 + i * 4, 0);
                        }
                                                refStore(mem, sp, handleRegister(ref)); sp += REF_SIZE;   // V4: push del ref nuevo
                        tc.sp = sp;
                    }
                    break;
                }
                case 0x53: { // GET_FIELD
                    int slot = mem[pc] & 0xFF; pc++;
                    sp -= REF_SIZE; long obj = refLoad(mem, sp);
                    requireAlive(tc, sp, obj);   // contrato B
                    writeI32(mem, sp, readI32(mem, fieldAddr(obj, slot))); sp += 4;
                    break;
                }
                case 0x54: { // SET_FIELD
                    int slot = mem[pc] & 0xFF; pc++;
                    sp -= 4; int val = readI32(mem, sp);
                    sp -= REF_SIZE; long obj = refLoad(mem, sp);
                    requireAlive(tc, sp, obj);   // contrato B
                    writeI32(mem, fieldAddr(obj, slot), val);
                    break;
                }
                // BUG-6: campos de instancia de 8 bytes (long/double/ref). slot = índice
                // de slot de 4 bytes; el valor ocupa 2 slots consecutivos en el campo.
                case 0xA8: { // GET_FIELD_LONG
                    int slot = mem[pc] & 0xFF; pc++;
                    sp -= REF_SIZE; long obj = refLoad(mem, sp);
                    requireAlive(tc, sp, obj);   // contrato B
                    writeI64(mem, sp, readI64(mem, fieldAddr(obj, slot))); sp += 8;
                    break;
                }
                case 0xA9: { // SET_FIELD_LONG
                    int slot = mem[pc] & 0xFF; pc++;
                    sp -= 8; long val = readI64(mem, sp);
                    sp -= REF_SIZE; long obj = refLoad(mem, sp);
                    requireAlive(tc, sp, obj);   // contrato B
                    writeI64(mem, fieldAddr(obj, slot), val);
                    break;
                }
                case 0x56: { // PRINT_NONL
                    sp -= 4; int v = readI32(mem, sp);
                    programOut.writeText(Integer.toString(v));
                    break;
                }
                case 0x57: { // FPRINT_NONL
                    sp -= 4; float v = Float.intBitsToFloat(readI32(mem, sp));
                    programOut.writeText(formatBpDouble((double) v));   // GAP-4
                    break;
                }
                case 0x58: { // PRINT_STR_NONL
                    sp -= REF_SIZE; long ref = refLoad(mem, sp);
                    programOut.writeText(readVmString(ref));  // H2: decodifica UTF-8
                    break;
                }
                case 0x59: { // PRINT_NL
                    programOut.newline();
                    break;
                }

                // ---- H1.2 (V2): long (i64). 8 bytes / 2 slots. ----
                case 0x71: { // LPUSH
                    long v = readI64(mem, pc); pc += 8;
                    writeI64(mem, sp, v); sp += 8;
                    break;
                }
                case 0x72: { // LADD
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, a + b); sp += 8; break;
                }
                case 0x73: { // LSUB
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, a - b); sp += 8; break;
                }
                case 0x74: { // LMUL
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, a * b); sp += 8; break;
                }
                case 0x75: { // LDIV
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    if (b == 0) { tc.sp = sp; throwBpRuntimeError(tc, "División por cero"); }
                    writeI64(mem, sp, a / b); sp += 8; break;
                }
                case 0x76: { // LMOD
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    if (b == 0) { tc.sp = sp; throwBpRuntimeError(tc, "Módulo por cero"); }
                    writeI64(mem, sp, a % b); sp += 8; break;
                }
                case 0x77: { // LNEG
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, -a); sp += 8; break;
                }
                case 0x78: { // LBAND
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, a & b); sp += 8; break;
                }
                case 0x79: { // LBOR
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, a | b); sp += 8; break;
                }
                case 0x7A: { // LBXOR
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, a ^ b); sp += 8; break;
                }
                case 0x7B: { // LBNOT
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, ~a); sp += 8; break;
                }
                case 0x7C: { // LSHL
                    sp -= 8; long n = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, a << (n & 63)); sp += 8; break;
                }
                case 0x7D: { // LSHR_S
                    sp -= 8; long n = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, a >> (n & 63)); sp += 8; break;
                }
                case 0x7E: { // LSHR_U
                    sp -= 8; long n = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI64(mem, sp, a >>> (n & 63)); sp += 8; break;
                }
                case 0x7F: { // LEQ
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI32(mem, sp, a == b ? 1 : 0); sp += 4; break;
                }
                case 0x80: { // LNEQ
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI32(mem, sp, a != b ? 1 : 0); sp += 4; break;
                }
                case 0x81: { // LLT
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI32(mem, sp, a <  b ? 1 : 0); sp += 4; break;
                }
                case 0x82: { // LLE
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI32(mem, sp, a <= b ? 1 : 0); sp += 4; break;
                }
                case 0x83: { // LGT
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI32(mem, sp, a >  b ? 1 : 0); sp += 4; break;
                }
                case 0x84: { // LGE
                    sp -= 8; long b = readI64(mem, sp);
                    sp -= 8; long a = readI64(mem, sp);
                    writeI32(mem, sp, a >= b ? 1 : 0); sp += 4; break;
                }
                case 0x85: { // LPRINT
                    sp -= 8; long v = readI64(mem, sp);
                    programOut.writeText(Long.toString(v)); programOut.newline();
                    break;
                }
                case 0x86: { // LPRINT_NONL
                    sp -= 8; long v = readI64(mem, sp);
                    programOut.writeText(Long.toString(v));
                    break;
                }
                case 0x87: { // I32_TO_I64 (sign-extend)
                    sp -= 4; int v = readI32(mem, sp);
                    writeI64(mem, sp, (long) v); sp += 8; break;
                }
                case 0x88: { // I64_TO_I32 (truncate low 32)
                    sp -= 8; long v = readI64(mem, sp);
                    writeI32(mem, sp, (int) v); sp += 4; break;
                }
                case 0x89: { // GET_LOCAL_L
                    short soff = (short) readI16(mem, pc); pc += 2;
                    writeI64(mem, sp, readI64(mem, bp + soff)); sp += 8;
                    break;
                }
                case 0x8A: { // SET_LOCAL_L
                    short soff = (short) readI16(mem, pc); pc += 2;
                    sp -= 8; long v = readI64(mem, sp);
                    writeI64(mem, bp + soff, v);
                    break;
                }
                case 0x8B: { // GET_GLOBAL_L
                    short soff = (short) readI16(mem, pc); pc += 2;
                    writeI64(mem, sp, readI64(mem, cs + soff)); sp += 8;
                    break;
                }
                case 0x8C: { // SET_GLOBAL_L
                    short soff = (short) readI16(mem, pc); pc += 2;
                    sp -= 8; long v = readI64(mem, sp);
                    writeI64(mem, cs + soff, v);
                    break;
                }
                case 0x8D: { // NEWARRAY_I64
                    sp -= 4; int size = readI32(mem, sp);
                    if (size < 0) { tc.sp=sp; throwBpRuntimeError(tc, "NEWARRAY_I64: tamaño negativo (" + size + ")"); }
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    synchronized (vmLock) {
                        int ref = heapAlloc(size * 8, TYPE_ARRAY_I64);
                        writeI32(mem, ref, size);
                                                refStore(mem, sp, handleRegister(ref)); sp += REF_SIZE;
                        tc.sp = sp;
                    }
                    break;
                }
                case 0x8E: { // ALOAD_I64
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) {
                        tc.sp = sp;
                        throwBpRuntimeError(tc, "ALOAD_I64: índice fuera de rango " + idx + " (length=" + length + ")");
                    }
                    writeI64(mem, sp, readI64(mem, arrElem(arr, idx, 8))); sp += 8;
                    break;
                }
                case 0x8F: { // ASTORE_I64
                    sp -= 8; long val = readI64(mem, sp);
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= REF_SIZE; long arr = refLoad(mem, sp); requireAlive(tc, sp, arr);   // contrato B
                    int length = arrLen(mem, arr);
                    if (idx < 0 || idx >= length) {
                        tc.sp = sp;
                        throwBpRuntimeError(tc, "ASTORE_I64: índice fuera de rango " + idx + " (length=" + length + ")");
                    }
                    writeI64(mem, arrElem(arr, idx, 8), val);
                    break;
                }

                // ---- H1.3 (V2): double (f64). Aritmética + conversiones. ----
                case 0x91: { // DPUSH
                    long bits = readI64(mem, pc); pc += 8;
                    writeI64(mem, sp, bits); sp += 8; break;
                }
                case 0x92: { // DADD
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI64(mem, sp, Double.doubleToRawLongBits(a + b)); sp += 8; break;
                }
                case 0x93: { // DSUB
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI64(mem, sp, Double.doubleToRawLongBits(a - b)); sp += 8; break;
                }
                case 0x94: { // DMUL
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI64(mem, sp, Double.doubleToRawLongBits(a * b)); sp += 8; break;
                }
                case 0xAC: { // IPOW — i32 base ^ i32 exp (exp. por cuadrados; exp<0 -> error)
                    sp -= 4; int e = readI32(mem, sp);
                    sp -= 4; int base = readI32(mem, sp);
                    if (e < 0) { tc.sp = sp; throwBpRuntimeError(tc, "exponente negativo en potencia entera"); }
                    int r = 1, bb = base;
                    while (e > 0) { if ((e & 1) != 0) r *= bb; bb *= bb; e >>= 1; }
                    writeI32(mem, sp, r); sp += 4; break;
                }
                case 0xAD: { // LPOW — i64 base ^ i64 exp
                    sp -= 8; long e = readI64(mem, sp);
                    sp -= 8; long base = readI64(mem, sp);
                    if (e < 0) { tc.sp = sp; throwBpRuntimeError(tc, "exponente negativo en potencia entera"); }
                    long r = 1L, bb = base;
                    while (e > 0) { if ((e & 1L) != 0) r *= bb; bb *= bb; e >>= 1; }
                    writeI64(mem, sp, r); sp += 8; break;
                }
                case 0xAE: { // DPOW — f64 base ^ f64 exp. Exponente entero (incl. x^2 float)
                    // -> exp. por cuadrados en f64 (parity-safe); fraccionario -> exp(e*ln base).
                    // MISMA lógica byte-a-byte que bpvm_dpow() en la VM-C (interp.c).
                    sp -= 8; double e = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double base = Double.longBitsToDouble(readI64(mem, sp));
                    double res;
                    if (e == Math.floor(e) && !Double.isInfinite(e) && Math.abs(e) <= 1024.0) {
                        long n = (long) e; boolean neg = n < 0; if (neg) n = -n;
                        double r = 1.0, bb = base;
                        while (n > 0) { if ((n & 1L) != 0) r *= bb; bb *= bb; n >>= 1; }
                        res = neg ? 1.0 / r : r;
                    } else {
                        res = Math.exp(e * Math.log(base));
                    }
                    writeI64(mem, sp, Double.doubleToRawLongBits(res)); sp += 8; break;
                }
                case 0x95: { // DDIV
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI64(mem, sp, Double.doubleToRawLongBits(a / b)); sp += 8; break;
                }
                case 0x96: { // DMOD
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI64(mem, sp, Double.doubleToRawLongBits(a % b)); sp += 8; break;
                }
                case 0x97: { // DNEG
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI64(mem, sp, Double.doubleToRawLongBits(-a)); sp += 8; break;
                }
                case 0x98: { // DEQ
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI32(mem, sp, a == b ? 1 : 0); sp += 4; break;
                }
                case 0x99: { // DNEQ
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI32(mem, sp, a != b ? 1 : 0); sp += 4; break;
                }
                case 0x9A: { // DLT
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI32(mem, sp, a <  b ? 1 : 0); sp += 4; break;
                }
                case 0x9B: { // DLE
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI32(mem, sp, a <= b ? 1 : 0); sp += 4; break;
                }
                case 0x9C: { // DGT
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI32(mem, sp, a >  b ? 1 : 0); sp += 4; break;
                }
                case 0x9D: { // DGE
                    sp -= 8; double b = Double.longBitsToDouble(readI64(mem, sp));
                    sp -= 8; double a = Double.longBitsToDouble(readI64(mem, sp));
                    writeI32(mem, sp, a >= b ? 1 : 0); sp += 4; break;
                }
                case 0x9E: { // DPRINT
                    sp -= 8; double v = Double.longBitsToDouble(readI64(mem, sp));
                    programOut.writeText(formatBpDouble(v)); programOut.newline(); break;   // GAP-4
                }
                case 0x9F: { // DPRINT_NONL
                    sp -= 8; double v = Double.longBitsToDouble(readI64(mem, sp));
                    programOut.writeText(formatBpDouble(v)); break;   // GAP-4
                }
                case 0xA0: { // I2D
                    sp -= 4; int v = readI32(mem, sp);
                    writeI64(mem, sp, Double.doubleToRawLongBits((double) v)); sp += 8; break;
                }
                case 0xA1: { // D2I
                    sp -= 8; double d = Double.longBitsToDouble(readI64(mem, sp));
                    writeI32(mem, sp, (int) d); sp += 4; break;
                }
                case 0xA2: { // L2D
                    sp -= 8; long v = readI64(mem, sp);
                    writeI64(mem, sp, Double.doubleToRawLongBits((double) v)); sp += 8; break;
                }
                case 0xA3: { // D2L
                    sp -= 8; double d = Double.longBitsToDouble(readI64(mem, sp));
                    writeI64(mem, sp, (long) d); sp += 8; break;
                }
                case 0xA4: { // F2D
                    sp -= 4; float f = Float.intBitsToFloat(readI32(mem, sp));
                    writeI64(mem, sp, Double.doubleToRawLongBits((double) f)); sp += 8; break;
                }
                case 0xA5: { // D2F
                    sp -= 8; double d = Double.longBitsToDouble(readI64(mem, sp));
                    writeI32(mem, sp, Float.floatToRawIntBits((float) d)); sp += 4; break;
                }
                case 0xA6: { // L2F
                    sp -= 8; long v = readI64(mem, sp);
                    writeI32(mem, sp, Float.floatToRawIntBits((float) v)); sp += 4; break;
                }
                case 0xA7: { // F2L
                    sp -= 4; float f = Float.intBitsToFloat(readI32(mem, sp));
                    writeI64(mem, sp, (long) f); sp += 8; break;
                }

                case 0x5A: { // CALL_BUILTIN
                    int id = readI16(mem, pc) & 0xFFFF; pc += 2;
                    // Sincronizamos al ThreadContext antes de entrar al builtin:
                    // dispatchBuiltin opera sobre tc.sp (thread-safe). Si el
                    // builtin lanza BpExceptionPending (B3 v2/v3), el catch
                    // del while exterior se hace cargo del unwind.
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    dispatchBuiltin(Builtin.byId(id), tc);
                    sp = tc.sp;
                    // tc.yieldRequested lo levantan los builtins yield/sleep/join;
                    // el while exterior lo observa y abandona el bucle.
                    break;
                }

                case 0x5B: { // TRY_BEGIN
                    int instrAddr = currentPC;
                    int relOff = readI32(mem, pc); pc += 4;
                    short expectedCsOff = (short) readI16(mem, pc); pc += 2;
                    int newHandlerPc = instrAddr + relOff;
                    int expectedClassPtr = (expectedCsOff != 0) ? (cs + expectedCsOff) : 0;
                    handlerStack.push(new int[]{ehHandlerPc, ehSavedSp, ehSavedBp, ehSavedCs, ehExpectedClass});
                    ehHandlerPc      = newHandlerPc;
                    ehSavedSp        = sp;
                    ehSavedBp        = bp;
                    ehSavedCs        = cs;
                    ehExpectedClass  = expectedClassPtr;
                    break;
                }
                case 0xAB: { // TRY_BEGIN_EXT (BUG-2): clsOff i32 (catch cross-module)
                    int instrAddr = currentPC;
                    int relOff = readI32(mem, pc); pc += 4;
                    int expectedCsOff = readI32(mem, pc); pc += 4;
                    int newHandlerPc = instrAddr + relOff;
                    int expectedClassPtr = (expectedCsOff != 0) ? (cs + expectedCsOff) : 0;
                    handlerStack.push(new int[]{ehHandlerPc, ehSavedSp, ehSavedBp, ehSavedCs, ehExpectedClass});
                    ehHandlerPc      = newHandlerPc;
                    ehSavedSp        = sp;
                    ehSavedBp        = bp;
                    ehSavedCs        = cs;
                    ehExpectedClass  = expectedClassPtr;
                    break;
                }
                case 0x5C: { // TRY_END
                    int[] prev = handlerStack.pop();
                    ehHandlerPc     = prev[0];
                    ehSavedSp       = prev[1];
                    ehSavedBp       = prev[2];
                    ehSavedCs       = prev[3];
                    ehExpectedClass = prev[4];
                    break;
                }
                case 0x5D: { // THROW
                    sp -= 8; long vFull = readI64(mem, sp); int v = (int) vFull;   // V4 #7: handle 64b COMPLETO (preserva gen); v = palabra baja (idx|TAG) para clase/mensaje
                    // Sync: classPtrOfRefOr0 puede leer this.memory; nuestro mem es el mismo.
                    int thrownClass = classPtrOfRefOr0(v);
                    boolean handled = false;
                    while (ehHandlerPc != -1) {
                        boolean matches = (ehExpectedClass == 0) ||
                                          (thrownClass != 0 && isDescendantOf(thrownClass, ehExpectedClass));
                        int handlerPc = ehHandlerPc;
                        int savedSp = ehSavedSp, savedBp = ehSavedBp, savedCs = ehSavedCs;
                        int[] prev = handlerStack.pop();
                        ehHandlerPc     = prev[0];
                        ehSavedSp       = prev[1];
                        ehSavedBp       = prev[2];
                        ehSavedCs       = prev[3];
                        ehExpectedClass = prev[4];
                        if (matches) {
                            sp = savedSp;
                            bp = savedBp;
                            cs = savedCs;
                            pc = handlerPc;
                            writeI64(mem, sp, vFull); sp += 8;   // V4 #7: re-empuja el handle COMPLETO (antes `& 0xFFFFFFFF` forzaba gen=0 → rompería con reuso de slots 4c; paridad VM-C)
                            handled = true;
                            break;
                        }
                    }
                    if (!handled) {
                        tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                        throw new RuntimeException(buildUnhandledExceptionMessage(
                                currentPC, v, thrownClass, bp, tc.stackBase));
                    }
                    break;
                }

                case 0x5F: { // FREE_REF
                    sp -= REF_SIZE; long ref = refLoad(mem, sp);
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    freeOwnedObject(ref);
                    break;
                }
                case 0x60: { // SET_FIELD_OWNER
                    int slot = mem[pc] & 0xFF; pc++;
                    sp -= REF_SIZE; long val = refLoad(mem, sp);   // V4: valor = ref nueva (8B, era 4B → drift + high-word)
                    sp -= REF_SIZE; long obj = refLoad(mem, sp);
                    requireAlive(tc, sp, obj);   // contrato B
                    int slotAddr = fieldAddr(obj, slot);
                    long old = refLoad(mem, slotAddr);             // V4: ref vieja (8B flat; era readI32 = high-word=0 → leak)
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    freeOwnedObject(old);
                    refStore(mem, slotAddr, val);                 // V4: escribe ref plana 8B
                    break;
                }

                case 0x5E: { // INSTANCEOF
                    short csOff = (short) readI16(mem, pc); pc += 2;
                    int expected = cs + csOff;
                    sp -= REF_SIZE; long ref = refLoad(mem, sp);
                    int objClass = classPtrOfRefOr0(ref);
                    boolean ok = (objClass != 0) && isDescendantOf(objClass, expected);
                    writeI32(mem, sp, ok ? 1 : 0); sp += 4;
                    break;
                }

                case 0xAF: { // CHECKCAST — #389, la mitad dinámica del cast de Object
                    short clsOff  = (short) readI16(mem, pc); pc += 2;
                    short nameOff = (short) readI16(mem, pc); pc += 2;
                    long ref = refLoad(mem, sp - REF_SIZE);   // peek: no consume
                    boolean ok = true;
                    if (ref != 0) {
                        if (clsOff == 0) {
                            // string(o): el bloque tiene que ser una cadena.
                            ok = blockTypeOfRefIs(ref, TYPE_ARRAY_I8);
                        } else {
                            int objClass = classPtrOfRefOr0(ref);
                            ok = (objClass != 0) && isDescendantOf(objClass, cs + clsOff);
                        }
                    }
                    if (!ok) {
                        int naddr = cs + nameOff;
                        int nlen  = readInt32(naddr);
                        if (nlen > 40) nlen = 40;
                        String nombre = new String(mem, naddr + 4, nlen,
                                java.nio.charset.StandardCharsets.UTF_8);
                        // ⚠️ MISMO mensaje, byte a byte, que interp.c — paridad.
                        tc.sp = sp; tc.bp = bp; tc.pc = pc; tc.cs = cs;
                        throwBpRuntimeError(tc,
                                "conversion invalida: el valor no es un '" + nombre + "'");
                    }
                    break;
                }

                case 0xB0: { // CHECKCAST_EXT — #444, el cast a una clase de OTRO módulo
                    // Idéntico al 0xAF salvo el ancho del cls_off (i32, lo parcha
                    // el linker con la dirección cs-relativa de la clase externa).
                    // Aquí cls_off == 0 NO es el centinela de cadena: `string(o)`
                    // no cruza módulos y sigue por el 0xAF.
                    int clsOff    = readI32(mem, pc); pc += 4;
                    short nameOff = (short) readI16(mem, pc); pc += 2;
                    long ref = refLoad(mem, sp - REF_SIZE);   // peek: no consume
                    boolean ok = true;
                    if (ref != 0) {
                        int objClass = classPtrOfRefOr0(ref);
                        ok = (objClass != 0) && isDescendantOf(objClass, cs + clsOff);
                    }
                    if (!ok) {
                        int naddr = cs + nameOff;
                        int nlen  = readInt32(naddr);
                        if (nlen > 40) nlen = 40;
                        String nombre = new String(mem, naddr + 4, nlen,
                                java.nio.charset.StandardCharsets.UTF_8);
                        // ⚠️ MISMO mensaje, byte a byte, que interp.c — paridad.
                        tc.sp = sp; tc.bp = bp; tc.pc = pc; tc.cs = cs;
                        throwBpRuntimeError(tc,
                                "conversion invalida: el valor no es un '" + nombre + "'");
                    }
                    break;
                }

                // --- Variantes compactas para reducir tamaño del bytecode.
                //     Estos cases tienen el writeI32/readI32 INLINE explícito
                //     porque el JIT del HotSpot considera readI32/writeI32
                //     (>35 bytes) demasiado grandes para inlinear, y como
                //     son ejecutados millones de veces en bucles calientes,
                //     evitar la method-call por instrucción es lo único que
                //     compensa el "bonus" de tamaño del bytecode.
                case 0x61: { // PUSH_0
                    mem[sp] = 0; mem[sp+1] = 0; mem[sp+2] = 0; mem[sp+3] = 0;
                    sp += 4;
                    break;
                }
                case 0x62: { // PUSH_1
                    mem[sp] = 0; mem[sp+1] = 0; mem[sp+2] = 0; mem[sp+3] = 1;
                    sp += 4;
                    break;
                }
                case 0x63: { // PUSH_2
                    mem[sp] = 0; mem[sp+1] = 0; mem[sp+2] = 0; mem[sp+3] = 2;
                    sp += 4;
                    break;
                }
                case 0x64: { // PUSH_3
                    mem[sp] = 0; mem[sp+1] = 0; mem[sp+2] = 0; mem[sp+3] = 3;
                    sp += 4;
                    break;
                }
                case 0x65: { // PUSH_4
                    mem[sp] = 0; mem[sp+1] = 0; mem[sp+2] = 0; mem[sp+3] = 4;
                    sp += 4;
                    break;
                }
                case 0x66: { // PUSH_NEG1
                    mem[sp] = -1; mem[sp+1] = -1; mem[sp+2] = -1; mem[sp+3] = -1;
                    sp += 4;
                    break;
                }
                case 0x67: { // GET_LOCAL_S8
                    int off = mem[pc]; pc++;
                    int a = bp + off;
                    int v = (mem[a] << 24) | ((mem[a+1] & 0xFF) << 16) | ((mem[a+2] & 0xFF) << 8) | (mem[a+3] & 0xFF);
                    mem[sp] = (byte)(v >> 24); mem[sp+1] = (byte)(v >> 16); mem[sp+2] = (byte)(v >> 8); mem[sp+3] = (byte)v;
                    sp += 4;
                    break;
                }
                case 0x68: { // SET_LOCAL_S8
                    int off = mem[pc]; pc++;
                    sp -= 4;
                    int v = (mem[sp] << 24) | ((mem[sp+1] & 0xFF) << 16) | ((mem[sp+2] & 0xFF) << 8) | (mem[sp+3] & 0xFF);
                    int a = bp + off;
                    mem[a] = (byte)(v >> 24); mem[a+1] = (byte)(v >> 16); mem[a+2] = (byte)(v >> 8); mem[a+3] = (byte)v;
                    break;
                }
                case 0x69: { // LEA_LOCAL_S8 — H1.2a: dirección de local = ref 8 bytes
                    int off = mem[pc]; pc++;
                    writeI64(mem, sp, ((long)(bp + off)) & 0xFFFFFFFFL); sp += 8;
                    break;
                }
                case 0x6A: { // GET_GLOBAL_S8
                    int off = mem[pc]; pc++;
                    int a = cs + off;
                    int v = (mem[a] << 24) | ((mem[a+1] & 0xFF) << 16) | ((mem[a+2] & 0xFF) << 8) | (mem[a+3] & 0xFF);
                    mem[sp] = (byte)(v >> 24); mem[sp+1] = (byte)(v >> 16); mem[sp+2] = (byte)(v >> 8); mem[sp+3] = (byte)v;
                    sp += 4;
                    break;
                }
                case 0x6B: { // SET_GLOBAL_S8
                    int off = mem[pc]; pc++;
                    sp -= 4;
                    int v = (mem[sp] << 24) | ((mem[sp+1] & 0xFF) << 16) | ((mem[sp+2] & 0xFF) << 8) | (mem[sp+3] & 0xFF);
                    int a = cs + off;
                    mem[a] = (byte)(v >> 24); mem[a+1] = (byte)(v >> 16); mem[a+2] = (byte)(v >> 8); mem[a+3] = (byte)v;
                    break;
                }
                case 0x6C: { // LEA_GLOBAL_S8 — H1.2a: dirección de global = ref 8 bytes
                    int off = mem[pc]; pc++;
                    int v = cs + off;
                    writeI64(mem, sp, ((long) v) & 0xFFFFFFFFL); sp += 8;
                    break;
                }

                case 0x55: { // INVOKE_VIRTUAL
                    int vtSlot  = mem[pc]     & 0xFF;
                    int numArgs = mem[pc + 1] & 0xFF;
                    pc += 2;
                    // H1.2a (V4): el receptor es una ref = 8 bytes (bajo los args, que ya
                    // van contados en slots) → sp-8-numArgs*4; readI64 + (int) low32.
                    long thisRef    = refLoad(mem, sp - REF_SIZE - numArgs * 4);
                    if (thisRef == 0) {
                        tc.sp = sp;
                        throwBpRuntimeError(tc, "INVOKE_VIRTUAL sobre null receiver"
                                + " (vtSlot=" + vtSlot + ", numArgs=" + numArgs + ")");
                        break;
                    }
                    requireAlive(tc, sp, thisRef);   // contrato B: método sobre objeto liberado → grita
                    // #389 — el "hermano" del cast: un Object con una CADENA
                    // dentro no despacha métodos. Validado, no leído a ciegas.
                    // ⚠️ MISMO mensaje que interp.c, byte a byte — paridad.
                    int classPtr   = classPtrOfRefOr0(thisRef);
                    if (classPtr == 0) {
                        tc.sp = sp;
                        throwBpRuntimeError(tc, "el receptor no es un objeto (una cadena o un array no despachan metodos)");
                        break;
                    }
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;

                    // L2 v3 — herencia cross-module: si vt[slot] == -1 o el slot
                    // está fuera del rango de num_methods del descriptor actual,
                    // sube al parent y reintenta. Cada iteración recalcula CS
                    // según el módulo dueño del descriptor (puede ser distinto al
                    // del child cuando el parent vive cross-module). Termina al
                    // encontrar un methodOff válido o al llegar a la raíz.
                    int desc = classPtr;
                    int methodOff = -1;
                    int targetCS = -1;
                    while (true) {
                        int bitmapW   = readI16(mem, desc + CLS_OFF_BITMAP_WORDS) & 0xFFFF;
                        int nMethods  = readI16(mem, desc + CLS_OFF_NUM_METHODS)  & 0xFFFF;
                        int vtBase    = desc + CLS_OFF_FIELD_BITMAP + 2 * bitmapW * 4;
                        if (vtSlot < nMethods) {
                            int off = readI32(mem, vtBase + vtSlot * 4);
                            if (off != -1) {
                                methodOff = off;
                                targetCS  = moduleManager.getCSForDataAddr(desc);
                                break;
                            }
                        }
                        int parentOff = readI32(mem, desc + CLS_OFF_PARENT_OFF);
                        if (parentOff == 0) {
                            // No hay padre: el slot no es resoluble por la cadena.
                            throwBpRuntimeError(tc, "INVOKE_VIRTUAL: slot "
                                    + vtSlot + " no resoluble en la cadena de herencia");
                            break;
                        }
                        int curCS = moduleManager.getCSForDataAddr(desc);
                        desc = curCS + parentOff;
                    }
                    if (methodOff == -1) break;   // throwBpRuntimeError ya hizo break
                    int targetPC = targetCS + methodOff;

                    writeI32(mem, sp, pc); sp += 4;
                    writeI32(mem, sp, bp); sp += 4;
                    writeI32(mem, sp, cs); sp += 4;
                    bp = sp;
                    pc = targetPC;
                    cs = targetCS;
                    break;
                }

                default:
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    throw new RuntimeException(String.format(
                            "Opcode no implementado: 0x%02X en PC %d",
                            rawOp, currentPC));
            }
            } catch (BpExceptionPending pending) {
                // B3 v3 — Cualquier opcode (no sólo CALL_BUILTIN) puede
                // lanzar BpExceptionPending si invoca throwBpRuntimeError.
                // Ejecutamos el unwind exacto del case 0x5D THROW sobre
                // las locales del intérprete. Si ningún handler atrapa,
                // convertimos a BpThreadFault para que WorkerLoop termine
                // el thread con un mensaje legible.
                sp = tc.sp;
                sp -= 8;
                long v = readI64(mem, sp);   // V4: excepción ref = handle 64b (gen preservada)
                int thrownClass = classPtrOfRefOr0(v);
                boolean handled = false;
                while (ehHandlerPc != -1) {
                    boolean matches = (ehExpectedClass == 0)
                            || (thrownClass != 0
                                && isDescendantOf(thrownClass, ehExpectedClass));
                    int handlerPc = ehHandlerPc;
                    int savedSp = ehSavedSp, savedBp = ehSavedBp, savedCs = ehSavedCs;
                    int[] prev = handlerStack.pop();
                    ehHandlerPc = prev[0]; ehSavedSp = prev[1]; ehSavedBp = prev[2];
                    ehSavedCs = prev[3]; ehExpectedClass = prev[4];
                    if (matches) {
                        sp = savedSp; bp = savedBp; cs = savedCs; pc = handlerPc;
                        writeI64(mem, sp, v); sp += 8;   // V4: excepción ref = handle 64b completo (gen preservada)
                        handled = true;
                        break;
                    }
                }
                if (!handled) {
                    String msg = readRuntimeErrorMsg(v);
                    tc.pc = pc; tc.sp = sp; tc.bp = bp; tc.cs = cs;
                    throw new BpThreadFault(msg != null ? msg : pending.getMessage());
                }
            }
        }
        } finally {
            // Sincronizamos estado de vuelta al ThreadContext, tanto en
            // salida normal (HALT, THREAD_EXIT, yield) como en excepción no
            // atrapada que escape de aquí. El scheduler outer (run()) lee tc
            // para hacer el switch.
            tc.pc = pc; tc.sp = sp; tc.bp = bp; tc.cs = cs;
            tc.ehHandlerPc     = ehHandlerPc;
            tc.ehSavedSp       = ehSavedSp;
            tc.ehSavedBp       = ehSavedBp;
            tc.ehSavedCs       = ehSavedCs;
            tc.ehExpectedClass = ehExpectedClass;
        }
        if (tc.yieldRequested) {
            tc.yieldRequested = false;
            return ExitSignal.YIELD;
        }
        return exitSignal;
    }

    // ====================================================================
    // Stdlib dispatch (CALL_BUILTIN)
    // ====================================================================

    /** Lee un string de la VM (length + chars) a partir de su user_ref y devuelve String Java. */
    private String readVmString(long ref) {
        // H2 (V2): strings son byte[] UTF-8. length = nº de bytes; payload en ref+4.
        // V4: la resolución ref→bytes pasa por la abstracción (arrLen/arrElem).
        if (ref == 0) return "";
        int nbytes = arrLen(memory, ref);
        byte[] buf = new byte[nbytes];
        System.arraycopy(memory, arrElem(ref, 0, 1), buf, 0, nbytes);
        return new String(buf, java.nio.charset.StandardCharsets.UTF_8);
    }

    /** Aloca un nuevo string en el heap (byte[] UTF-8) con el contenido de Java String. Devuelve user_ref. */
    private long allocVmString(String s) {
        byte[] utf8 = s.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        int len = utf8.length;
        int addr = heapAlloc(len, TYPE_ARRAY_I8);
        writeInt32(addr, len);
        System.arraycopy(utf8, 0, memory, addr + 4, len);
        long h = handleRegister(addr);   // V4: addr físico → handle 64b
        // B1 residual — el caller llama a allocVmString desde dispatchBuiltin
        // SIN vmLock, así que entre `return h` y el `pushTc(tc, h)` un GC
        // de otro worker podría liberar `h` (no hay raíz aún). Anclamos
        // explícitamente en el tc actual; el GC lo agrega a roots. El ancla es
        // idx|TAG (refDeref la usa dead-tolerant → la gen no importa).
        ThreadContext me = currentTcLocal.get();
        if (me != null) me.allocAnchor = (int) h;
        return h;
    }

    /** Aloca un array de refs (TYPE_ARRAY_REF) con n elementos, devolviendo el handle. Los slots quedan a 0. */
    private long allocVmRefArray(int n) {
        int addr = heapAlloc(n * 8, TYPE_ARRAY_REF);   // H1.2a (V4): ref plana = 8 bytes/elem
        writeInt32(addr, n);
        for (int i = 0; i < n; i++) writeI64(memory, addr + 4 + i * 8, 0L);
        long h = handleRegister(addr);   // V4: addr físico → handle 64b
        // B1 residual — ver allocVmString. Mismo motivo: ancla GC para el
        // intervalo entre alocación y publicación al stack del programa.
        ThreadContext me = currentTcLocal.get();
        if (me != null) me.allocAnchor = (int) h;
        return h;
    }

    /**
     * Despachador de builtins (CALL_BUILTIN). Recibe el {@link ThreadContext}
     * sobre el que opera — todos los push/pop van contra `tc.sp`, NUNCA
     * contra this.SP. Esto es lo que permite que dos workers Java ejecuten
     * dispatchBuiltin simultáneamente sobre tcs distintos sin pisarse.
     *
     * Los builtins que ceden la CPU (yield/sleep/join) levantan
     * {@code tc.yieldRequested = true}; el bucle del intérprete observa este
     * flag tras CALL_BUILTIN y abandona el bucle inner para que el scheduler
     * haga context switch.
     */
    /**
     * #347 — un float en [0, 1), la base de random() y de randomInt().
     *
     * Se construye con 24 bits enteros divididos por 2^24, NO con
     * {@code (float) Math.random()}. El motivo es un borde real: Math.random()
     * devuelve un double de [0,1), y un double muy pegado a 1 (0.99999999…)
     * REDONDEA A 1.0f al convertirlo a float — o sea que el "menor que 1" se
     * rompía justo en el extremo, raras veces y sin avisar. Con 24 bits (los
     * que caben en la mantisa de un float) el máximo posible es
     * 16777215/16777216, que sí es menor que 1 exactamente.
     *
     * La VM-C hace lo mismo en builtins.c con los 24 bits altos de
     * bpvm_platform_random_u32(). Las dos VMs nunca darán la misma SECUENCIA
     * —son fuentes distintas, y eso es lo correcto— pero sí el mismo CONTRATO.
     *
     * ThreadLocalRandom y no Math.random() porque esta última comparte un único
     * Random entre todos los threads: con varios workers se convierte en un
     * punto de contención, y aquí no hace falta ninguna sincronización.
     */
    private static float nextUnit01() {
        int bits = java.util.concurrent.ThreadLocalRandom.current().nextInt(1 << 24);
        return bits / 16777216.0f;
    }

    private void dispatchBuiltin(Builtin b, ThreadContext tc) {
        switch (b) {
            case STRLEN: {
                long ref = popTcRef(tc);   // H1.2a: string ref 8 bytes
                String s = readVmString(ref);          // H2: longitud en codepoints
                pushTc(tc, s.codePointCount(0, s.length()));
                break;
            }
            case PARSE_INT: {
                String s = readVmString(popTcRef(tc));
                pushTc(tc, (int) Long.parseLong(s.trim()));
                break;
            }
            case PARSE_FLOAT: {
                String s = readVmString(popTcRef(tc));
                pushTc(tc, Float.floatToRawIntBits((float) Double.parseDouble(s.trim())));
                break;
            }
            case EVAL: {
                // H7 — calculadora de constantes (descenso recursivo, evalúa sobre la
                // marcha; sin AST). Computa en double, devuelve float. Lógica byte-a-byte
                // idéntica a bpvm_eval_calc() de la VM-C (builtins.c). Error -> NaN.
                String s = readVmString(popTcRef(tc));
                pushTc(tc, Float.floatToRawIntBits((float) EvalCalc.run(s)));
                break;
            }
            case INT_TO_STRING: {
                int n = popTc(tc);
                pushTcRef(tc, allocVmString(Integer.toString(n)));
                break;
            }
            case FLOAT_TO_STRING: {
                float x = Float.intBitsToFloat(popTc(tc));
                // L13 — formateo canónico GAP-4 (el mismo que FPRINT), no
                // Float.toString: así `"" + f` y `print f` dan SIEMPRE lo
                // mismo, y el puerto C es byte-idéntico.
                pushTcRef(tc, allocVmString(formatBpDouble((double) x)));
                break;
            }
            case LONG_TO_STRING: {   // L13 — concat string + long
                int lo = popTc(tc);
                int hi = popTc(tc);
                long v = ((long) hi << 32) | (lo & 0xFFFFFFFFL);
                pushTcRef(tc, allocVmString(Long.toString(v)));
                break;
            }
            case DOUBLE_TO_STRING: {   // L13 — concat string + double (GAP-4)
                int lo = popTc(tc);
                int hi = popTc(tc);
                double v = Double.longBitsToDouble(((long) hi << 32) | (lo & 0xFFFFFFFFL));
                pushTcRef(tc, allocVmString(formatBpDouble(v)));
                break;
            }

            // ---- H11 (#241) — cliente TCP simple (módulo Net). Paridad con la
            // VM-C: el fallo de CONEXIÓN es un resultado normal (handle 0 →
            // boolean false en Net.Tcp.connect); los errores de send/recv sobre
            // una conexión establecida son RuntimeError ATRAPABLE con el MISMO
            // mensaje que la VM-C. recv siempre con timeout (contrato H11). ----
            case TCP_CONNECT: {
                int timeoutMs = popTc(tc);
                int port      = popTc(tc);
                String host   = readVmString(popTcRef(tc));
                int handle = 0;
                try {
                    java.net.Socket s = new java.net.Socket();
                    s.connect(new java.net.InetSocketAddress(host, port),
                              timeoutMs > 0 ? timeoutMs : 5000);
                    s.setTcpNoDelay(true);
                    handle = netNextHandle.getAndIncrement();
                    netSockets.put(handle, s);
                } catch (Exception e) {
                    handle = 0;   // rechazo/timeout/DNS: resultado normal, no error
                }
                pushTc(tc, handle);
                break;
            }
            case TCP_SEND: {
                long dataRefH = popTcRef(tc);   // byte[] (8B ref) — antes popTc 4B
                int h       = popTc(tc);        // socket handle (int opaco) — OK
                java.net.Socket s = netSockets.get(h);
                if (s == null) {
                    throwBpRuntimeError(tc, "Net.send: conexión cerrada o inválida");
                    break;
                }
                int dataRef = (dataRefH == 0) ? 0 : refDeref(dataRefH);   // handle → addr (faltaba deref)
                int n = (dataRef == 0) ? 0 : readInt32(dataRef);
                try {
                    if (n > 0) {
                        s.getOutputStream().write(memory, dataRef + 4, n);
                        s.getOutputStream().flush();
                    }
                    pushTc(tc, n);
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "Net.send: conexión cerrada o inválida");
                }
                break;
            }
            case TCP_RECV: {
                int timeoutMs = popTc(tc);
                int max       = popTc(tc);
                int h         = popTc(tc);
                if (max < 0) max = 0;
                if (max > 65536) max = 65536;          // tope sano por llamada
                java.net.Socket s = netSockets.get(h);
                if (s == null) {
                    throwBpRuntimeError(tc, "Net.recv: error de red");
                    break;
                }
                int n = 0;
                byte[] buf = new byte[max];
                try {
                    // setSoTimeout(0) = infinito: clamp a 1 ms — recv SIEMPRE
                    // con timeout (en device bloquearía el worker entero).
                    s.setSoTimeout(timeoutMs > 0 ? timeoutMs : 1);
                    if (max > 0) {
                        n = s.getInputStream().read(buf, 0, max);
                        if (n < 0) {
                            throwBpRuntimeError(tc, "Net.recv: conexión cerrada por el peer");
                            break;
                        }
                    }
                } catch (java.net.SocketTimeoutException te) {
                    n = 0;                              // timeout → byte[] vacío
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "Net.recv: error de red");
                    break;
                }
                int ref = heapAlloc(n, TYPE_ARRAY_I8);
                writeInt32(ref, n);
                if (n > 0) System.arraycopy(buf, 0, memory, ref + 4, n);
                long outRef = handleRegister(ref);   // V4: addr → handle 64b
                ThreadContext me = currentTcLocal.get();
                if (me != null) me.allocAnchor = (int) outRef;   // ancla GC = palabra baja (idx|TAG)
                pushTcRef(tc, outRef);   // 8B (byte[] es referencia — antes truncaba a 4B)
                break;
            }
            case TCP_CLOSE: {
                int h = popTc(tc);
                java.net.Socket s = netSockets.remove(h);
                if (s != null) {
                    try { s.close(); } catch (java.io.IOException ignored) { }
                }
                break;
            }

            // ---- H3 (V3) — GUI: delegan en GuiBackend (Swing). Los void hacen
            //      pushTc(tc, 0) (dummy ret); los demas empujan su resultado.
            //      Orden de pop: ultimo arg en top (igual que el resto). ----
            case GUI_SCREEN_ACTIVE: { pushTc(tc, gui.screenActive()); break; }
            case GUI_CREATE_OBJ:    { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createObj(p));    break; }
            case GUI_CREATE_LABEL:  { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createLabel(p));  break; }
            case GUI_CREATE_BUTTON: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createButton(p)); break; }
            case GUI_SET_TEXT: {
                long t = popTcRef(tc); int hnd = popTc(tc);   // 4→8B: el string es ref de 8 bytes; popTc (4B) truncaba el handle → texto vacío + id sin sacar (pila descuadrada)
                gui.setText(hnd, readVmString(t)); pushTc(tc, 0); break;
            }
            case GUI_SET_WIDTH:  { int w  = popTc(tc); int hnd = popTc(tc); gui.setWidth(hnd, w);   pushTc(tc, 0); break; }
            case GUI_SET_HEIGHT: { int hh = popTc(tc); int hnd = popTc(tc); gui.setHeight(hnd, hh); pushTc(tc, 0); break; }
            case GUI_ALIGN: {
                int dy = popTc(tc); int dx = popTc(tc); int a = popTc(tc); int hnd = popTc(tc);
                gui.align(hnd, a, dx, dy); pushTc(tc, 0); break;
            }
            case GUI_SET_BG_COLOR:   { int rgb = popTc(tc); int hnd = popTc(tc); gui.setBgColor(hnd, rgb);   pushTc(tc, 0); break; }
            case GUI_SET_TEXT_COLOR: { int rgb = popTc(tc); int hnd = popTc(tc); gui.setTextColor(hnd, rgb); pushTc(tc, 0); break; }
            case GUI_SET_FONT:       { int f   = popTc(tc); int hnd = popTc(tc); gui.setFont(hnd, f);        pushTc(tc, 0); break; }
            case GUI_LOAD_FONT:      { long ref = popTcRef(tc); pushTc(tc, gui.loadFont(readVmString(ref))); break; }
            case GUI_SET_ROTATION:   { int deg = popTc(tc); gui.setRotation(deg); pushTc(tc, 0); break; }

            // H19 — App.* introspección del proyecto (ids 211-213).
            case APP_MAIN_MODULE: {       // nombre del entry (basename sin extensión)
                String p = appMainModulePath;
                int sl = Math.max(p.lastIndexOf('/'), p.lastIndexOf('\\'));
                String base = (sl >= 0) ? p.substring(sl + 1) : p;
                int dot = base.indexOf('.');
                if (dot >= 0) base = base.substring(0, dot);
                pushTcRef(tc, allocVmString(base));
                break;
            }
            case APP_MAIN_MODULE_PATH: { pushTcRef(tc, allocVmString(appMainModulePath)); break; }
            case APP_PROJECT_PATH: {
                ModuleManager mm = getModuleManager();
                java.nio.file.Path wd = (mm != null) ? mm.getWorkdir() : null;
                pushTcRef(tc, allocVmString(wd != null ? wd.toString() : ""));
                break;
            }
            case GUI_CLEAN:       { int hnd = popTc(tc); gui.clean(hnd);      pushTc(tc, 0); break; }
            case GUI_DELETE:      { int hnd = popTc(tc); gui.delete(hnd);     pushTc(tc, 0); break; }
            case GUI_SCREEN_LOAD: { int hnd = popTc(tc); gui.screenLoad(hnd); pushTc(tc, 0); break; }
            case GUI_RUN:       { guiEventLoop(tc); pushTc(tc, 0); break; }
            case GUI_RUN_ONCE:  { pushTc(tc, guiEventLoopOnce(tc) ? 1 : 0); break; }
            // #324 tanda 2b — (obj, nombre) → slot de vtable. -1 si no existe:
            // "esta ventana no tiene un handler con ese nombre" es lo normal,
            // no un error, así que decide el llamante. Ver slotOfMarker.
            case GUI_SLOT_OF: {
                int nameRef = (int) popTcRef(tc);
                int objRef  = (int) popTcRef(tc);
                if (objRef == 0) { pushTc(tc, -1); break; }
                int classPtr = readInt32(refDeref(objRef));
                pushTc(tc, moduleManager.slotOfMarker(classPtr, readVmString(nameRef)));
                break;
            }
            case GUI_DUMP_TREE: { pushTcRef(tc, allocVmString(gui.dumpTree())); break; }

            /* ── #414 — recorrer los packs GRABADOS desde BP ──────────────────
             *
             * miVM NO tiene zona de packs: es la VM de referencia del PC y los
             * packs grabados son de la placa. Así que la respuesta honesta a
             * «dame el siguiente» es -1, «ya estás al final».
             *
             * Y eso da la paridad GRATIS, que era la duda abierta del diseño:
             * el bucle de `Packs.list()` no entra, devuelve una lista vacía, y
             * la salida es byte-idéntica a la de la VM-C sin un solo caso
             * especial ni un `if` de plataforma en el programa BP.
             *
             * Los `info` no debería llamarlos nadie —para llegar a ellos hay que
             * tener un cursor, y aquí no hay ninguno— pero si alguien lo hace,
             * cadena vacía: lo mismo que contesta la VM-C ante un cursor que no
             * vale. */
            case PACK_NEXT:       { popTc(tc);              pushTc(tc, -1); break; }
            case PACK_ENTRY_NEXT: { popTc(tc); popTc(tc);   pushTc(tc, -1); break; }
            case PACK_INFO:       { popTc(tc);              pushTcRef(tc, allocVmString("")); break; }
            case PACK_ENTRY_INFO: { popTc(tc); popTc(tc);   pushTcRef(tc, allocVmString("")); break; }
            case GUI_BIND_CLICK: {
                int self = popTc(tc); int hnd = popTc(tc);
                gui.bindClick(hnd, self); pushTc(tc, 0); break;
            }
            case GUI_CLICK:     { int obj = popTc(tc); gui.injectClick(obj); pushTc(tc, 0); break; }
            // H13 (V3) — Forms: call-by-name del handler. Resuelve `name` como
            // función pública del módulo de `owner` (la ventana) y la invoca con
            // `sender` como arg0. Los args se apilan owner, name, sender.
            case GUI_INVOKE_BY_NAME: {
                // #324 tanda 2a — los TRES son refs (8 bytes). Se sacaban con
                // popTc (4B) y la pila se leía descuadrada. Estaba latente
                // porque este camino no lo ejercitaba nadie: formdemo carga el
                // form pero no pulsa nada, así que en headless nunca se
                // despachaba. La VM-C ya sacaba pop_ref/pop_ref/pop_ref.
                int sender   = (int) popTcRef(tc);
                int nameRef  = (int) popTcRef(tc);
                int ownerRef = (int) popTcRef(tc);
                invokeHandlerByName(tc, ownerRef, readVmString(nameRef), sender);
                pushTc(tc, 0);
                break;
            }
            // H13.1 (V3) — Forms Camino A: dispatch por SLOT (handler = método de la
            // ventana, slot horneado por el IDE en el .win). Args apilados: win, slot, sender.
            case GUI_INVOKE_BY_SLOT: {
                // #324 tanda 2a — win y sender son refs (8 bytes); sólo `slot`
                // es un entero. Se sacaban los tres con popTc (4B): la pila
                // quedaba descuadrada y `winRef` salía basura. Lo delataba el
                // propio helper, que tres líneas más abajo dice "ambos son REFS
                // → 8 bytes". Mismo desliz del 4→8B que #20/#293, latente por
                // no ejercitarse. La VM-C ya sacaba pop_ref/pop_i32/pop_ref.
                int sender = (int) popTcRef(tc);
                int slot   = popTc(tc);
                int winRef = (int) popTcRef(tc);
                invokeHandlerBySlot(tc, winRef, slot, sender);
                pushTc(tc, 0);
                break;
            }
            // H5.c — `raise ev(args)`: ENCOLA, no llama. El handler lo inyecta el
            // scheduler entre quanta (drainOneEvent). Pila de arriba abajo:
            // recv(8B) dest(4B) nargs(4B) masks(4B) argN-1..arg0; los anchos de
            // los argumentos los DICE el compilador en masks, no los adivinamos.
            case EVENT_RAISE: {
                long recv = popTcRef(tc);
                int dest  = popTc(tc);
                int nargs = popTc(tc);
                int masks = popTc(tc);
                if (nargs < 0 || nargs > EVENT_MAX_ARGS) {
                    throwBpRuntimeError(tc, "__eventRaise: aridad " + nargs + " fuera de rango");
                    break;
                }
                long[] args = new long[nargs];
                for (int i = nargs - 1; i >= 0; i--) {          // se desapilan al revés
                    if ((masks & (1 << (8 + i))) != 0) { tc.sp -= 8; args[i] = readI64(memory, tc.sp); }
                    else                               { tc.sp -= 4; args[i] = readI32(memory, tc.sp); }
                }
                // Sin suscriptor no pasa nada: un evento es una NOTIFICACIÓN, y si
                // nadie escucha no hay error (decisión de diseño, modelo Swing).
                if (recv != 0 && dest != 0) {
                    synchronized (vmLock) {
                        if (eventQueue.size() >= EVENT_QUEUE_CAP) {
                            // Que GRITE: un evento perdido en silencio cuesta una tarde.
                            System.err.println("[bpvm] cola de eventos llena ("
                                    + EVENT_QUEUE_CAP + "): evento descartado (tid=" + tc.id
                                    + ", slot=" + dest + ")");
                        } else {
                            eventQueue.addLast(new PendingEvent(recv, dest, tc.id, masks, args));
                        }
                    }
                }
                pushTc(tc, 0);   // void
                break;
            }

            // V3/H6 — geometría (backend = verdad) + scroll (opt-in) + refresh.
            case GUI_SET_X:  { int v = popTc(tc); int hnd = popTc(tc); gui.setX(hnd, v); pushTc(tc, 0); break; }
            case GUI_GET_X:  { int hnd = popTc(tc); pushTc(tc, gui.getX(hnd)); break; }
            case GUI_SET_Y:  { int v = popTc(tc); int hnd = popTc(tc); gui.setY(hnd, v); pushTc(tc, 0); break; }
            case GUI_GET_Y:  { int hnd = popTc(tc); pushTc(tc, gui.getY(hnd)); break; }
            case GUI_GET_WIDTH:  { int hnd = popTc(tc); pushTc(tc, gui.getWidth(hnd));  break; }
            case GUI_GET_HEIGHT: { int hnd = popTc(tc); pushTc(tc, gui.getHeight(hnd)); break; }
            case GUI_SET_SCROLL_DIR: { int d = popTc(tc); int hnd = popTc(tc); gui.setScrollDir(hnd, d); pushTc(tc, 0); break; }
            case GUI_GET_SCROLL_DIR: { int hnd = popTc(tc); pushTc(tc, gui.getScrollDir(hnd)); break; }
            case GUI_REFRESH: { int hnd = popTc(tc); gui.refresh(hnd); pushTc(tc, 0); break; }
            // H6 widgets — checkbox.
            case GUI_CREATE_CHECKBOX: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createCheckbox(p)); break; }
            case GUI_SET_CHECKED: { int v = popTc(tc); int hnd = popTc(tc); gui.setChecked(hnd, v != 0); pushTc(tc, 0); break; }
            case GUI_GET_CHECKED: { int hnd = popTc(tc); pushTc(tc, gui.getChecked(hnd) ? 1 : 0); break; }
            case GUI_CHANGE: { int obj = popTc(tc); gui.injectChange(obj); pushTc(tc, 0); break; }
            // H6 widgets — switch + slider + bar (value-widgets enteros).
            case GUI_CREATE_SWITCH: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createSwitch(p)); break; }
            case GUI_CREATE_SLIDER: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createSlider(p)); break; }
            case GUI_CREATE_BAR:    { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createBar(p)); break; }
            case GUI_SET_VALUE: { int v = popTc(tc); int hnd = popTc(tc); gui.setValue(hnd, v); pushTc(tc, 0); break; }
            case GUI_GET_VALUE: { int hnd = popTc(tc); pushTc(tc, gui.getValue(hnd)); break; }
            case GUI_SET_RANGE: { int mx = popTc(tc); int mn = popTc(tc); int hnd = popTc(tc); gui.setRange(hnd, mn, mx); pushTc(tc, 0); break; }
            case GUI_CREATE_SPINBOX: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createSpinbox(p)); break; }
            case GUI_CREATE_LED:     { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createLed(p)); break; }
            // H7 — Chart (el eje Y va por GUI_SET_RANGE, el repintado por GUI_REFRESH)
            case GUI_CREATE_CHART:   { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createChart(p)); break; }
            case GUI_CHART_SET_POINTS: { int n = popTc(tc); int h = popTc(tc); gui.chartSetPoints(h, n); pushTc(tc, 0); break; }
            case GUI_CHART_ADD_SERIES: { int c = popTc(tc); int h = popTc(tc); pushTc(tc, gui.chartAddSeries(h, c)); break; }
            case GUI_CHART_PUSH:     { int v = popTc(tc); int s = popTc(tc); int h = popTc(tc); gui.chartPush(h, s, v); pushTc(tc, 0); break; }
            case GUI_CHART_SET_VALUE:{ int v = popTc(tc); int i = popTc(tc); int s = popTc(tc); int h = popTc(tc); gui.chartSetValue(h, s, i, v); pushTc(tc, 0); break; }
            case GUI_CHART_SET_TYPE: { int t = popTc(tc); int h = popTc(tc); gui.chartSetType(h, t); pushTc(tc, 0); break; }
            case GUI_CREATE_DROPDOWN: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createDropdown(p)); break; }
            case GUI_SET_OPTIONS: { long o = popTcRef(tc); int hnd = popTc(tc); gui.setOptions(hnd, readVmString(o)); pushTc(tc, 0); break; }   // 4→8B: opts = string ref 8B
            case GUI_CREATE_TEXTAREA: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createTextarea(p)); break; }
            case GUI_GET_TEXT: { int hnd = popTc(tc); pushTcRef(tc, allocVmString(gui.getText(hnd))); break; }
            case GUI_CREATE_LIST: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createList(p)); break; }
            case GUI_CREATE_KEYBOARD: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createKeyboard(p)); break; }
            case GUI_KEYBOARD_SET_TEXTAREA: { int ta = popTc(tc); int hnd = popTc(tc); gui.keyboardSetTextarea(hnd, ta); pushTc(tc, 0); break; }
            case GUI_CREATE_MSGBOX: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createMsgbox(p)); break; }
            case GUI_SET_BUTTONS: { long lbls = popTcRef(tc); int hnd = popTc(tc); gui.setButtons(hnd, readVmString(lbls)); pushTc(tc, 0); break; }   // 4→8B: labels = string ref 8B
            case GUI_CREATE_TABVIEW: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createTabview(p)); break; }
            case GUI_TABVIEW_ADD_TAB: { long nm = popTcRef(tc); int hnd = popTc(tc); pushTc(tc, gui.tabviewAddTab(hnd, readVmString(nm))); break; }   // 4→8B: name = string ref 8B
            case GUI_CREATE_TABLE: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createTable(p)); break; }
            case GUI_TABLE_SET_GRID: { int c = popTc(tc); int r = popTc(tc); int hnd = popTc(tc); gui.tableSetGrid(hnd, r, c); pushTc(tc, 0); break; }
            case GUI_TABLE_SET_CELL: { long t = popTcRef(tc); int c = popTc(tc); int r = popTc(tc); int hnd = popTc(tc); gui.tableSetCell(hnd, r, c, readVmString(t)); pushTc(tc, 0); break; }   // 4→8B: text = string ref 8B
            case GUI_TABLE_GET_CELL: { int c = popTc(tc); int r = popTc(tc); int hnd = popTc(tc); pushTcRef(tc, allocVmString(gui.tableGetCell(hnd, r, c))); break; }
            case GUI_IMAGE_NEW: { pushTc(tc, gui.imageNew()); break; }
            case GUI_IMAGE_LOAD_FILE: { long p = popTcRef(tc); int id = popTc(tc); pushTc(tc, gui.imageLoadFile(id, readVmString(p))); break; }   // 4→8B: path = string ref 8B
            case GUI_IMAGE_WIDTH: { int id = popTc(tc); pushTc(tc, gui.imageWidth(id)); break; }
            case GUI_IMAGE_HEIGHT: { int id = popTc(tc); pushTc(tc, gui.imageHeight(id)); break; }
            case GUI_CREATE_IMAGEVIEW: { int p = popTc(tc); guiRequireParent(tc, p); pushTc(tc, gui.createImageView(p)); break; }
            case GUI_IMAGEVIEW_SET_IMAGE: { int img = popTc(tc); int view = popTc(tc); gui.imageViewSetImage(view, img); pushTc(tc, 0); break; }
            case GUI_IMAGEVIEW_REFRESH: { int view = popTc(tc); gui.imageViewRefresh(view); pushTc(tc, 0); break; }
            case GUI_SET_FONT_SIZE: { int px = popTc(tc); int h = popTc(tc); gui.setFontSize(h, px); pushTc(tc, 0); break; }
            case GUI_GET_FONT_SIZE: { int h = popTc(tc); pushTc(tc, gui.getFontSize(h)); break; }
            case GUI_TEXTAREA_SET_READONLY: { int ro = popTc(tc); int h = popTc(tc); gui.setReadonly(h, ro); pushTc(tc, 0); break; }
            case GUI_TEXTAREA_GET_READONLY: { int h = popTc(tc); pushTc(tc, gui.getReadonly(h)); break; }
            case BOOL_TO_STRING: {
                int v = popTc(tc);
                pushTcRef(tc, allocVmString(v != 0 ? "true" : "false"));
                break;
            }
            case UPPER: { String s = readVmString(popTcRef(tc)); pushTcRef(tc, allocVmString(latin1Case(s, true)));  break; }
            case LOWER: { String s = readVmString(popTcRef(tc)); pushTcRef(tc, allocVmString(latin1Case(s, false))); break; }
            case TRIM:  { String s = readVmString(popTcRef(tc)); pushTcRef(tc, allocVmString(s.trim()));        break; }

            case SUBSTRING: {
                int end = popTc(tc); int start = popTc(tc);
                String s = readVmString(popTcRef(tc));
                int n = s.codePointCount(0, s.length());   // H2: índices en codepoints
                int from = Math.max(0, Math.min(n, start));
                int to   = Math.max(from, Math.min(n, end));
                pushTcRef(tc, allocVmString(s.substring(s.offsetByCodePoints(0, from),
                                                     s.offsetByCodePoints(0, to))));
                break;
            }
            case INDEX_OF: {
                String target = readVmString(popTcRef(tc));
                String s = readVmString(popTcRef(tc));
                int ci = s.indexOf(target);   // índice en char units (UTF-16)
                // H2: devolver índice en codepoints para ser consistente con
                // charAt/charCodeAt/substring (semántica de carácter).
                pushTc(tc, ci < 0 ? -1 : s.codePointCount(0, ci));
                break;
            }
            case STARTS_WITH: {
                String pre = readVmString(popTcRef(tc));
                String s = readVmString(popTcRef(tc));
                pushTc(tc, s.startsWith(pre) ? 1 : 0);
                break;
            }
            case ENDS_WITH: {
                String suf = readVmString(popTcRef(tc));
                String s = readVmString(popTcRef(tc));
                pushTc(tc, s.endsWith(suf) ? 1 : 0);
                break;
            }
            case CONTAINS: {
                String sub = readVmString(popTcRef(tc));
                String s = readVmString(popTcRef(tc));
                pushTc(tc, s.contains(sub) ? 1 : 0);
                break;
            }
            case CHAR_AT: {
                int i = popTc(tc);
                String s = readVmString(popTcRef(tc));
                int n = s.codePointCount(0, s.length());   // H2: índice en codepoints
                if (i < 0 || i >= n) {
                    throwBpRuntimeError(tc, "charAt: idx fuera de rango " + i + " (len=" + n + ")");
                }
                int cp = s.codePointAt(s.offsetByCodePoints(0, i));
                pushTcRef(tc, allocVmString(new String(Character.toChars(cp))));
                break;
            }
            case REPLACE: {
                String rep = readVmString(popTcRef(tc));
                String tgt = readVmString(popTcRef(tc));
                String s = readVmString(popTcRef(tc));
                pushTcRef(tc, allocVmString(s.replace(tgt, rep)));
                break;
            }

            case ABS: { int x = popTc(tc); pushTc(tc, Math.abs(x)); break; }
            case MIN: { int b2 = popTc(tc); int a = popTc(tc); pushTc(tc, Math.min(a, b2)); break; }
            case MAX: { int b2 = popTc(tc); int a = popTc(tc); pushTc(tc, Math.max(a, b2)); break; }

            case SQRT:  { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.sqrt(x))); break; }
            case POW:   { float e = Float.intBitsToFloat(popTc(tc)); float base = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.pow(base, e))); break; }
            case LOG:   { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.log(x))); break; }
            case LOG10: { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.log10(x))); break; }
            case EXP:   { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.exp(x))); break; }
            case SIN:   { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.sin(x))); break; }
            case COS:   { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.cos(x))); break; }
            case TAN:   { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.tan(x))); break; }

            case PI: { pushTc(tc, Float.floatToRawIntBits((float) Math.PI)); break; }
            case E:  { pushTc(tc, Float.floatToRawIntBits((float) Math.E));  break; }

            case FLOOR: { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, (int) Math.floor(x)); break; }
            case CEIL:  { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, (int) Math.ceil(x));  break; }
            case ROUND: { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Math.round(x));       break; }

            /* #347 — random / randomInt. MISMA FORMULA que la VM-C (builtins.c),
             * porque la definicion del lenguaje es la formula, no la prosa.
             *
             * Antes esto hacia lo + (int)(rnd * (hi-lo+1)), o sea [lo,hi] CON el
             * alto dentro — y contradecia a docs/BUILTINS.md y al backend JVM,
             * que decian [lo,hi). Tres fuentes, dos semanticas. Se unifica en la
             * forma clasica de BASIC: randomInt NO es un generador aparte, es
             * random() escalado, y de ahi sale solo que el alto NO entra. */
            case RANDOM:     { pushTc(tc, Float.floatToRawIntBits(nextUnit01())); break; }
            case RANDOM_INT: {
                int hi = popTc(tc); int lo = popTc(tc);
                if (hi <= lo) { pushTc(tc, lo); break; }   /* rango vacio -> lo */
                long span = (long) hi - (long) lo;         /* en 64b: INT_MAX-INT_MIN desborda int */
                long off  = (long) ((double) nextUnit01() * (double) span);
                if (off >= span) off = span - 1;
                pushTc(tc, (int) (lo + off));
                break;
            }

            case NOW:   { pushTc(tc, (int) (System.currentTimeMillis() & 0x7FFFFFFFL)); break; }
            case SLEEP: {
                int ms = popTc(tc);
                // Marca el thread como BLOCKED_SLEEP y cede CPU. Si no hay
                // otro thread RUNNABLE, pickNextRunnable() hace Thread.sleep
                // hasta el wakeAt más próximo.
                blockTcSleep(tc, ms);
                tc.yieldRequested = true;
                pushTc(tc, 0);  // dummy ret
                break;
            }
            case SLEEP_SEC: {
                // Misma semántica que SLEEP pero la entrada está en segundos.
                // Multiplicamos con long para evitar overflow si el usuario
                // pide muchas horas (s * 1000 saldría de i32 a partir de ~24
                // días). int_max ms = ~24.8 días — suficiente para uso normal.
                int s = popTc(tc);
                long ms = (long) s * 1000L;
                if (ms > Integer.MAX_VALUE) ms = Integer.MAX_VALUE;
                blockTcSleep(tc, (int) ms);
                tc.yieldRequested = true;
                pushTc(tc, 0);
                break;
            }
            case SLEEP_US: {
                // Busy-wait que NO cede el thread BP. La VM Java no puede
                // garantizar precisión sub-ms (jitter del scheduler del SO,
                // GC, JIT compilation, etc.) — el usuario debe tener esto
                // en cuenta. En el Pico la VM-C sí da precisión µs real
                // gracias a busy_wait_us() del SDK.
                int us = popTc(tc);
                if (us > 0) {
                    long deadlineNs = System.nanoTime() + (long) us * 1000L;
                    while (System.nanoTime() < deadlineNs) {
                        // spin sin yield — no marcamos BLOCKED_SLEEP ni
                        // tocamos yieldRequested. El intérprete sigue
                        // ocupando la CPU del worker Java.
                    }
                }
                pushTc(tc, 0);
                break;
            }

            case SPLIT: {
                String sep = readVmString(popTcRef(tc));
                String s   = readVmString(popTcRef(tc));
                // String.split con regex literal: pasamos java.util.regex.Pattern.quote.
                String[] parts = s.split(java.util.regex.Pattern.quote(sep), -1);
                // Aloca primero los strings individuales, luego el array (en ese orden el
                // GC tendrá los slots zero-init mientras se llenan).
                long[] refs = new long[parts.length];
                for (int i = 0; i < parts.length; i++) refs[i] = allocVmString(parts[i]);   // #6 (censo V4): handle 64b completo (era (int)→gen=0)
                long arrRef = allocVmRefArray(parts.length);
                int adir = refDeref(arrRef);   // V4: dirección física (antes usaba el handle crudo como addr, sin refDeref)
                for (int i = 0; i < parts.length; i++) writeI64(memory, adir + 4 + i * 8, refs[i]);   // ref 8B completa
                pushTcRef(tc, arrRef);
                break;
            }

            case INPUT: {
                if (stdinReader == null) {
                    stdinReader = new java.io.BufferedReader(new java.io.InputStreamReader(System.in));
                }
                try {
                    String line = stdinReader.readLine();
                    pushTcRef(tc, allocVmString(line != null ? line : ""));
                } catch (java.io.IOException e) {
                    throw new RuntimeException("input(): " + e.getMessage());
                }
                break;
            }

            case READ_FILE: {
                String path = readVmString(popTcRef(tc));
                try {
                    byte[] data = packResource(path);   // #310 — el pack en ejecución va primero
                    if (data == null) data = java.nio.file.Files.readAllBytes(sandboxPath(tc, path));
                    pushTcRef(tc, allocVmString(new String(data, java.nio.charset.StandardCharsets.UTF_8)));
                } catch (java.io.IOException e) {
                    // N-readfile-msg-skew — el mensaje era e.getMessage(): la ruta
                    // NORMALIZADA POR LA PLATAFORMA (en Windows, con barras
                    // invertidas), o sea distinto por SO y distinto de la VM-C.
                    // Toca el invariante: el mismo .mod debe fallar con el mismo
                    // texto. Gana el mensaje de la VM-C (builtins.c:1354), que
                    // ademas no repite la ruta que ya va delante.
                    throwBpRuntimeError(tc, "readFile('" + path + "'): no se pudo abrir");
                }
                break;
            }
            case WRITE_FILE: {
                String content = readVmString(popTcRef(tc));
                String path    = readVmString(popTcRef(tc));
                try {
                    java.nio.file.Files.write(sandboxPath(tc, path),
                            content.getBytes(java.nio.charset.StandardCharsets.UTF_8));
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "writeFile('" + path + "'): " + e.getMessage());
                }
                pushTc(tc, 0); // dummy
                break;
            }
            case APPEND_FILE: {
                String content = readVmString(popTcRef(tc));
                String path    = readVmString(popTcRef(tc));
                try {
                    java.nio.file.Files.write(sandboxPath(tc, path),
                            content.getBytes(java.nio.charset.StandardCharsets.UTF_8),
                            java.nio.file.StandardOpenOption.CREATE,
                            java.nio.file.StandardOpenOption.APPEND);
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "appendFile('" + path + "'): " + e.getMessage());
                }
                pushTc(tc, 0); // dummy
                break;
            }
            case FILE_EXISTS: {
                String path = readVmString(popTcRef(tc));
                boolean hay = (packResource(path) != null)   // #310 — el pack en ejecución va primero
                        || java.nio.file.Files.exists(sandboxPath(tc, path));
                pushTc(tc, hay ? 1 : 0);
                break;
            }
            case READ_FILE_BYTES: {
                // #247 binario: copia cruda a un byte[] (TYPE_ARRAY_I8), SIN pasar
                // por String → preserva NUL/>127/UTF-8 inválido (a diferencia de
                // readFile, que decodifica UTF-8 y es lossy para binario).
                String path = readVmString(popTcRef(tc));
                try {
                    byte[] data = packResource(path);   // #310 — el pack en ejecución va primero
                    if (data == null) data = java.nio.file.Files.readAllBytes(sandboxPath(tc, path));
                    int ref = heapAlloc(data.length, TYPE_ARRAY_I8);
                    writeInt32(ref, data.length);
                    System.arraycopy(data, 0, memory, ref + 4, data.length);
                    long outRef = handleRegister(ref);   // V4: addr → handle 64b
                    ThreadContext me = currentTcLocal.get();
                    if (me != null) me.allocAnchor = (int) outRef;   // ancla GC = palabra baja (idx|TAG)
                    pushTcRef(tc, outRef);   // 8B (byte[] es referencia — antes truncaba a 4B)
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "readFileBytes('" + path + "'): " + e.getMessage());
                }
                break;
            }
            case WRITE_FILE_BYTES: {
                // #247 binario: escribe los bytes crudos del byte[] (TYPE_ARRAY_I8),
                // SIN pasar por String. Args: (path, data) → data empujado el último.
                long dataRefH = popTcRef(tc);   // 8B (byte[] es referencia — antes popTc 4B)
                String path = readVmString(popTcRef(tc));
                int dataRef = (dataRefH == 0) ? 0 : refDeref(dataRefH);   // handle → addr físico (faltaba deref)
                int n = (dataRef == 0) ? 0 : readInt32(dataRef);
                byte[] out = new byte[n];
                if (n > 0) System.arraycopy(memory, dataRef + 4, out, 0, n);
                try {
                    java.nio.file.Files.write(sandboxPath(tc, path), out);
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "writeFileBytes('" + path + "'): " + e.getMessage());
                }
                pushTc(tc, 0); // dummy (void)
                break;
            }
            case THROW_RTE: {
                // #248 — lanza el RuntimeError nativo de la VM con el mensaje
                // dado (mismo path que div0/null deref → atrapable con
                // try/catch BP). No retorna.
                String msg = readVmString(popTcRef(tc));
                throwBpRuntimeError(tc, msg);
                break;   // unreachable: throwBpRuntimeError siempre lanza
            }
            case GC: {
                gc();
                pushTc(tc, 0);  // void → dummy
                break;
            }
            case LIST_DIR: {
                String path = readVmString(popTcRef(tc));
                java.io.File dir = sandboxPath(tc, path).toFile();
                String[] names = dir.list();
                if (names == null) names = new String[0];
                long[] refs = new long[names.length];
                for (int i = 0; i < names.length; i++) refs[i] = allocVmString(names[i]);   // #6 (censo V4): handle 64b completo (era (int)→gen=0)
                long arrRef = allocVmRefArray(names.length);   // ya es handle
                int adir = refDeref(arrRef);   // V4: dirección física
                for (int i = 0; i < names.length; i++) writeI64(memory, adir + 4 + i * 8, refs[i]);   // ref 8B completa
                pushTcRef(tc, arrRef);
                break;
            }

            // ---- Soporte para List / StringBuilder ----
            case NEW_REF_ARRAY: {
                int cap = popTc(tc);
                if (cap < 0) throwBpRuntimeError(tc, "__newRefArray: capacidad negativa: " + cap);
                pushTcRef(tc, allocVmRefArray(cap));   // V4: ref array = 8B (era pushTc 4B → SET_FIELD_LONG hacía underflow → cs=0)
                break;
            }
            case GROW_REF_ARRAY: {
                int newCap = popTc(tc);
                long oldRef = popTcRef(tc);   // H1.2a: array ref 8 bytes
                if (newCap < 0) throwBpRuntimeError(tc, "__growRefArray: capacidad negativa: " + newCap);
                int od = (oldRef != 0) ? refDeref(oldRef) : 0;   // V4: dirección física fuente
                int oldLen = (od != 0) ? readInt32(od) : 0;
                long newRef = allocVmRefArray(newCap);            // ya es handle
                int nd = refDeref(newRef);
                int copyLen = Math.min(oldLen, newCap);
                for (int i = 0; i < copyLen; i++) {   // ref plana = 8 bytes/elem
                    writeI64(memory, nd + 4 + i * 8, readI64(memory, od + 4 + i * 8));
                }
                pushTcRef(tc, newRef);
                break;
            }
            case GROW_INT_ARRAY: {
                int newCap = popTc(tc);
                long oldRef = popTcRef(tc);   // H1.2a: array ref 8 bytes
                if (newCap < 0) throwBpRuntimeError(tc, "__growIntArray: capacidad negativa: " + newCap);
                int od = (oldRef != 0) ? refDeref(oldRef) : 0;   // V4: dirección física fuente
                int oldLen = (od != 0) ? readInt32(od) : 0;
                int newRef = heapAlloc(newCap * 4, TYPE_ARRAY_I32);   // addr físico
                writeInt32(newRef, newCap);
                int copyLen = Math.min(oldLen, newCap);
                for (int i = 0; i < copyLen; i++) {
                    writeInt32(newRef + 4 + i * 4, readInt32(od + 4 + i * 4));
                }
                for (int i = copyLen; i < newCap; i++) {
                    writeInt32(newRef + 4 + i * 4, 0);
                }
                                pushTcRef(tc, handleRegister(newRef));
                break;
            }
            case CHARS_TO_STRING: {
                int len = popTc(tc);
                long charsRef = popTcRef(tc);   // #6 (censo V4): int[] ref = 8B (era popTc 4B → desalineaba)
                if (len < 0) throwBpRuntimeError(tc, "__charsToString: longitud negativa: " + len);
                int cd = (charsRef != 0) ? refDeref(charsRef) : 0;   // V4: dirección física
                int avail = (cd != 0) ? readInt32(cd) : 0;
                if (len > avail) throwBpRuntimeError(tc, "__charsToString: longitud " + len + " > capacidad " + avail);
                StringBuilder sb = new StringBuilder(len);
                for (int i = 0; i < len; i++) sb.appendCodePoint(readInt32(cd + 4 + i * 4));
                pushTcRef(tc, allocVmString(sb.toString()));   // H2: codifica UTF-8
                break;
            }
            case CHAR_CODE_AT: {
                int idx = popTc(tc);
                String s = readVmString(popTcRef(tc));      // #6 (censo V4): string ref = 8B; H2 índice en codepoints
                int n = s.codePointCount(0, s.length());
                if (idx < 0 || idx >= n) throwBpRuntimeError(tc, "charCodeAt: índice " + idx + " fuera de [0," + n + ")");
                pushTc(tc, s.codePointAt(s.offsetByCodePoints(0, idx)));
                break;
            }
            case TO_BYTES:
            case FROM_BYTES: {
                // H2 (V2): string y byte[] comparten layout (TYPE_ARRAY_I8). La
                // conversión es una copia defensiva (string inmutable / byte[]
                // mutable): mismos bytes, objeto nuevo.
                long ref = popTcRef(tc);   // #6 (censo V4): string/byte[] ref = 8B
                int rd = (ref == 0) ? 0 : refDeref(ref);   // V4: dirección física fuente
                int n = (rd == 0) ? 0 : readInt32(rd);
                int out = heapAlloc(n, TYPE_ARRAY_I8);   // addr físico
                writeInt32(out, n);
                if (n > 0) System.arraycopy(memory, rd + 4, memory, out + 4, n);
                long outRef = handleRegister(out);   // V4: addr → handle 64b COMPLETO (no truncar)
                ThreadContext me = currentTcLocal.get();
                if (me != null) me.allocAnchor = (int) outRef;   // ancla GC = palabra baja (idx|TAG) que refDeref usa
                pushTcRef(tc, outRef);
                break;
            }
            case HEAP_FRAG: {           // H3: diagnóstico (solo VM-Java)
                pushTcRef(tc, allocVmString(heapFragReport()));
                break;
            }
            case HEAP_MAP: {            // H3: diagnóstico (solo VM-Java)
                int cols = popTc(tc);
                pushTcRef(tc, allocVmString(heapMap(cols)));
                break;
            }

            // ---- Threading ----
            // Convención de fields en la clase Thread sintetizada:
            //   slot 0 = __tid         (id del ThreadContext, 0 = no spawneado todavía)
            //   slot 1 = __stackSize   (bytes; 0 = default)
            // Convención de vtable: slot 0 = run() (virtual).
            case THREAD_START: {
                // El argumento es una REF: 8 bytes. Con popTc (4B) se perdía la
                // generación del handle Y la pila quedaba 4 bytes alta.
                long threadRef = popTcRef(tc);
                // V4 (tanda 2): threadRef es un HANDLE → refDeref para tocar memory[];
                // el REF se conserva entero como `this` de run() (abajo, refStore(sb, threadRef)).
                int ta        = refDeref(threadRef);
                int classPtr  = readInt32(ta);
                int stackSize = readInt32(ta + 4 + 1 * 4);   // field 1
                synchronized (vmLock) {
                    // 1) Crear el nuevo ThreadContext con su región de stack.
                    int newTid = spawnThread(stackSize);
                    writeInt32(ta + 4 + 0 * 4, newTid);          // guardar tid en field 0
                    ThreadContext nt = threads.get(newTid);
                    // 2) Resolver dirección absoluta del run() en la vtable.
                    int targetCS   = moduleManager.getCSForDataAddr(classPtr);
                    int bitmapW    = readInt16(classPtr + CLS_OFF_BITMAP_WORDS) & 0xFFFF;
                    int vtableBase = classPtr + CLS_OFF_FIELD_BITMAP + 2 * bitmapW * 4;
                    int methodOff  = readInt32(vtableBase + 0 * 4);     // slot 0 = run()
                    int runPc      = targetCS + methodOff;
                    // 3) Preparar el frame inicial en el stack del nuevo thread.
                    //    Tiene que ser IGUAL al que monta INVOKE_VIRTUAL, porque
                    //    run() es un método corriente y su código lo da por hecho:
                    //    el emisor le pone GET_LOCAL_L -20, una carga de 8 BYTES.
                    //    [sb+0..7]  thisRef  (REF_SIZE — refStore, NO writeInt32)
                    //    [sb+8]     saved PC = 0  (sentinela: memory[0] = THREAD_EXIT)
                    //    [sb+12]    saved BP = sb
                    //    [sb+16]    saved CS = 0
                    //    bp = sb + 20; sp = sb + 20
                    //
                    //    Esto valía 16 y la ref se escribía en 4 bytes: el layout de
                    //    ANTES del ensanchado de ref 4→8B, que aquí no se migró. Con
                    //    bp = sb+16 run() leía sus 8 bytes en sb-4 y la mitad ALTA
                    //    —la generación— salía de debajo de su propia pila.
                    int sb = nt.stackBase;
                    refStore(memory, sb, threadRef);
                    writeInt32(sb + 8,  0);
                    writeInt32(sb + 12, sb);
                    writeInt32(sb + 16, 0);
                    nt.sp = sb + 20;
                    nt.bp = sb + 20;
                    nt.pc = runPc;
                    nt.cs = targetCS;
                    // 4) Notificamos a workers durmiendo en pickNextRunnableTc.
                    vmLock.notifyAll();
                }
                pushTc(tc, 0);   // dummy ret
                break;
            }
            case THREAD_JOIN: {
                long threadRef = popTcRef(tc);   // REF de 8 bytes, igual que pop_ref en la VM-C
                if (threadRef == 0) { pushTc(tc, 0); break; }   // null → no-op (como VM-C)
                int targetTid = readInt32(refDeref(threadRef) + 4 + 0 * 4);   // V4: handle→addr
                if (targetTid <= 0) {
                    // No spawneado o tid inválido → no-op.
                    pushTc(tc, 0);
                    break;
                }
                boolean blocked = blockTcJoin(tc, targetTid);
                if (blocked) tc.yieldRequested = true;
                pushTc(tc, 0);   // dummy ret
                break;
            }
            case YIELD: {
                blockTcSleep(tc, 0);            // marca RUNNABLE + addLast
                tc.yieldRequested = true;
                pushTc(tc, 0);   // dummy ret
                break;
            }

            // ---- Sync (Mutex) ----
            // Convención de fields en la clase Mutex sintetizada:
            //   slot 0 = __mid   (índice en VM.mutexes, >= 0)
            case MUTEX_CREATE: {
                int newId;
                synchronized (vmLock) {
                    newId = mutexes.size();
                    mutexes.add(new JavaMutex());
                }
                pushTc(tc, newId);
                break;
            }
            case MUTEX_LOCK: {
                int mutexRef = popTc(tc);
                int mid = readInt32(refDeref(mutexRef) + 4 + 0 * 4);   // V4: handle→addr
                if (mid < 0 || mid >= mutexes.size()) {
                    // B3 v2 — lanzamos RuntimeError BP atrapable en lugar de
                    // BpThreadFault, así el código BP puede try/catch.
                    throwBpRuntimeError(tc, "mutex.lock: id inválido " + mid);
                }
                synchronized (vmLock) {
                    JavaMutex jm = mutexes.get(mid);
                    if (jm.ownerTid == JavaMutex.FREE) {
                        jm.ownerTid = tc.id;
                    } else if (jm.ownerTid == tc.id) {
                        throwBpRuntimeError(tc, "mutex.lock: re-entrada por mismo thread tid="
                                + tc.id + " (los Mutex no son reentrantes)");
                    } else {
                        // Tomado por otro → nos bloqueamos. El que tenga ownership
                        // nos despertará en MUTEX_UNLOCK y nos dará ownership.
                        jm.waiters.add(tc.id);
                        tc.status = ThreadStatus.BLOCKED_MUTEX;
                        tc.blockedOnMutexId = mid;
                        tc.yieldRequested = true;
                    }
                    vmLock.notifyAll();
                }
                pushTc(tc, 0);   // dummy ret
                break;
            }
            case MUTEX_UNLOCK: {
                int mutexRef = popTc(tc);
                int mid = readInt32(refDeref(mutexRef) + 4 + 0 * 4);   // V4: handle→addr
                if (mid < 0 || mid >= mutexes.size()) {
                    throwBpRuntimeError(tc, "mutex.unlock: id inválido " + mid);
                }
                synchronized (vmLock) {
                    JavaMutex jm = mutexes.get(mid);
                    if (jm.ownerTid != tc.id) {
                        throwBpRuntimeError(tc, "mutex.unlock: thread " + tc.id
                                + " no es propietario (owner=" + jm.ownerTid + ")");
                    }
                    if (jm.waiters.isEmpty()) {
                        jm.ownerTid = JavaMutex.FREE;
                    } else {
                        // Hand-off directo: el primer waiter recibe ownership
                        // sin pasar por una "re-attempt" en su lado.
                        int nextTid = jm.waiters.remove(0);
                        jm.ownerTid = nextTid;
                        ThreadContext nt = threads.get(nextTid);
                        nt.status = ThreadStatus.RUNNABLE;
                        nt.blockedOnMutexId = -1;
                        runQueue.addLast(nextTid);
                    }
                    vmLock.notifyAll();
                }
                pushTc(tc, 0);   // dummy ret
                break;
            }

            // ---- Arrays ----
            // move(src, dst, srcStart, dstStart, count) → void
            // Copia `count` elementos de src[srcStart..] a dst[dstStart..].
            // Soporta overlapping cuando src y dst son el mismo array (System.arraycopy lo
            // maneja sobre el byte[] subyacente). Valida en runtime: ambos refs deben
            // apuntar a un array vivo del MISMO tipo (i8/i16/i32/ref). Tipos distintos →
            // RuntimeError, no se reinterpreta silenciosamente.
            case MOVE: {
                int count    = popTc(tc);
                int dstStart = popTc(tc);
                int srcStart = popTc(tc);
                long dstRefH = popTcRef(tc);   // #6 (censo V4): array ref = 8B (era popTc 4B + el handle se usaba como dirección física SIN refDeref)
                long srcRefH = popTcRef(tc);
                if (srcRefH == 0)
                    throwBpRuntimeError(tc, "move: src es null");
                if (dstRefH == 0)
                    throwBpRuntimeError(tc, "move: dst es null");
                int srcRef = refDeref(srcRefH);   // V4: handle → dirección física del dato del array
                int dstRef = refDeref(dstRefH);
                int srcHeader = srcRef - 4;
                int dstHeader = dstRef - 4;
                if (srcHeader < heapStart || srcHeader >= heapNext)
                    throwBpRuntimeError(tc, "move: src no es ref a heap");
                if (dstHeader < heapStart || dstHeader >= heapNext)
                    throwBpRuntimeError(tc, "move: dst no es ref a heap");
                int srcTag = readInt32(srcHeader);
                int dstTag = readInt32(dstHeader);
                if ((srcTag & TAG_FREE_BIT) != 0)
                    throwBpRuntimeError(tc, "move: src apunta a bloque libre");
                if ((dstTag & TAG_FREE_BIT) != 0)
                    throwBpRuntimeError(tc, "move: dst apunta a bloque libre");
                int srcType = (srcTag & TAG_TYPE_MASK) >>> TAG_TYPE_SHIFT;
                int dstType = (dstTag & TAG_TYPE_MASK) >>> TAG_TYPE_SHIFT;
                if (srcType != TYPE_ARRAY_I8 && srcType != TYPE_ARRAY_I16
                        && srcType != TYPE_ARRAY_I32 && srcType != TYPE_ARRAY_REF)
                    throwBpRuntimeError(tc, "move: src no es un array (type=" + srcType + ")");
                if (dstType != TYPE_ARRAY_I8 && dstType != TYPE_ARRAY_I16
                        && dstType != TYPE_ARRAY_I32 && dstType != TYPE_ARRAY_REF)
                    throwBpRuntimeError(tc, "move: dst no es un array (type=" + dstType + ")");
                if (srcType != dstType)
                    throwBpRuntimeError(tc, "move: tipos de array distintos (src=" + srcType
                            + " dst=" + dstType + ")");
                int srcLen = readInt32(srcRef);
                int dstLen = readInt32(dstRef);
                if (count < 0)
                    throwBpRuntimeError(tc, "move: count negativo (" + count + ")");
                if (srcStart < 0 || dstStart < 0)
                    throwBpRuntimeError(tc, "move: offset negativo (srcStart=" + srcStart
                            + " dstStart=" + dstStart + ")");
                if ((long) srcStart + count > srcLen)
                    throwBpRuntimeError(tc, "move: rango fuera de src (srcStart=" + srcStart
                            + " count=" + count + " srcLen=" + srcLen + ")");
                if ((long) dstStart + count > dstLen)
                    throwBpRuntimeError(tc, "move: rango fuera de dst (dstStart=" + dstStart
                            + " count=" + count + " dstLen=" + dstLen + ")");
                int elemSz = elemSize(srcType);
                int srcByte = srcRef + 4 + srcStart * elemSz;
                int dstByte = dstRef + 4 + dstStart * elemSz;
                int bytes   = count * elemSz;
                System.arraycopy(memory, srcByte, memory, dstByte, bytes);
                pushTc(tc, 0);   // dummy ret (void)
                break;
            }

            // ---- Math intrínsecos ----
            case SIGN_I: {
                int x = popTc(tc);
                pushTc(tc, Integer.compare(x, 0));      // -1, 0, 1
                break;
            }
            case SIGN_F: {
                float x = Float.intBitsToFloat(popTc(tc));
                int r;
                if (Float.isNaN(x))     r = 0;          // convención: NaN → 0
                else if (x > 0f)        r = 1;
                else if (x < 0f)        r = -1;
                else                    r = 0;          // ±0
                pushTc(tc, r);
                break;
            }
            case ASIN: { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.asin(x))); break; }
            case ACOS: { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.acos(x))); break; }
            case ATAN: { float x = Float.intBitsToFloat(popTc(tc)); pushTc(tc, Float.floatToRawIntBits((float) Math.atan(x))); break; }
            case ATAN2: {
                // pila (bottom→top): y, x. popTc devuelve top primero.
                float x = Float.intBitsToFloat(popTc(tc));
                float y = Float.intBitsToFloat(popTc(tc));
                pushTc(tc, Float.floatToRawIntBits((float) Math.atan2(y, x)));
                break;
            }
            case FACTORIAL_I: {
                int n = popTc(tc);
                if (n < 0)
                    throwBpRuntimeError(tc, "factorial: argumento negativo (" + n + ")");
                if (n > 12)
                    // 13! = 6227020800 desborda i32 con signo.
                    throwBpRuntimeError(tc, "factorial: " + n + " desborda integer (máx 12)");
                int r = 1;
                for (int i = 2; i <= n; i++) r *= i;
                pushTc(tc, r);
                break;
            }
            case GAMMA_F: {
                // Lanczos approximation con g=7, n=9. Para x natural devuelve (x-1)!.
                float x = Float.intBitsToFloat(popTc(tc));
                pushTc(tc, Float.floatToRawIntBits((float) lanczosGamma(x)));
                break;
            }

            // ---- IO intrínsecos ----
            // #348 tanda 3 — rutas SIEMPRE con '/'. Antes esto era
            // java.nio.file.Paths, que es DEPENDIENTE DE PLATAFORMA: en Windows
            // pathJoin("a","b") daba "a\b", o sea rutas que el FS del
            // dispositivo (que es '/') no entiende, y ademas distintas segun el
            // host. Mismo patron que el locale de upper/lower: miVM converge
            // hacia lo que el micro puede hacer. La regla vive en pathJoin/
            // pathParent/pathBasename de abajo, replicadas en la VM-C.
            case PATH_JOIN: {
                String b2 = readVmString(popTcRef(tc));
                String a  = readVmString(popTcRef(tc));
                pushTcRef(tc, allocVmString(pathJoin(a, b2)));
                break;
            }
            case PATH_PARENT: {
                String p = readVmString(popTcRef(tc));
                pushTcRef(tc, allocVmString(pathParent(p)));
                break;
            }
            case PATH_BASENAME: {
                String p = readVmString(popTcRef(tc));
                pushTcRef(tc, allocVmString(pathBasename(p)));
                break;
            }
            case PATH_EXTENSION: {
                String p = readVmString(popTcRef(tc));
                String name = pathBasename(p);
                int dot = name.lastIndexOf('.');
                String ext = (dot <= 0 || dot == name.length() - 1) ? "" : name.substring(dot + 1);
                pushTcRef(tc, allocVmString(ext));
                break;
            }
            case PATH_ABSOLUTE: {
                String p = readVmString(popTcRef(tc));
                // Con sandbox: devuelve el path absoluto DENTRO del workdir
                // (no filtra info del host). Sin sandbox: usa Paths.get raw.
                java.nio.file.Path resolved = sandboxPath(tc, p);
                String r = resolved.toAbsolutePath().normalize().toString();
                pushTcRef(tc, allocVmString(r));
                break;
            }
            case MKDIR: {
                String p = readVmString(popTcRef(tc));
                try { java.nio.file.Files.createDirectories(sandboxPath(tc, p)); }
                catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "mkdir('" + p + "'): " + e.getMessage());
                }
                pushTc(tc, 0);   // dummy void
                break;
            }
            case RMDIR: {
                String p = readVmString(popTcRef(tc));
                try { java.nio.file.Files.delete(sandboxPath(tc, p)); }
                catch (java.nio.file.DirectoryNotEmptyException e) {
                    throwBpRuntimeError(tc, "rmdir('" + p + "'): directorio no vacío");
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "rmdir('" + p + "'): " + e.getMessage());
                }
                pushTc(tc, 0);
                break;
            }
            case REMOVE_FILE: {
                String p = readVmString(popTcRef(tc));
                try { java.nio.file.Files.delete(sandboxPath(tc, p)); }
                catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "removeFile('" + p + "'): " + e.getMessage());
                }
                pushTc(tc, 0);
                break;
            }
            case RENAME: {
                String to   = readVmString(popTcRef(tc));
                String from = readVmString(popTcRef(tc));
                try {
                    java.nio.file.Files.move(sandboxPath(tc, from),
                            sandboxPath(tc, to),
                            java.nio.file.StandardCopyOption.REPLACE_EXISTING);
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "rename('" + from + "' → '" + to + "'): " + e.getMessage());
                }
                pushTc(tc, 0);
                break;
            }
            case COPY_FILE: {
                String to   = readVmString(popTcRef(tc));
                String from = readVmString(popTcRef(tc));
                try {
                    java.nio.file.Files.copy(sandboxPath(tc, from),
                            sandboxPath(tc, to),
                            java.nio.file.StandardCopyOption.REPLACE_EXISTING);
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "copyFile('" + from + "' → '" + to + "'): " + e.getMessage());
                }
                pushTc(tc, 0);
                break;
            }
            case FILE_SIZE: {
                String p = readVmString(popTcRef(tc));
                byte[] enPack = packResource(p);   // #310 — el pack en ejecución va primero
                if (enPack != null) { pushTc(tc, enPack.length); break; }
                try {
                    long sz = java.nio.file.Files.size(sandboxPath(tc, p));
                    // i32: si sobrepasa Integer.MAX_VALUE, error claro.
                    if (sz > Integer.MAX_VALUE)
                        throwBpRuntimeError(tc, "fileSize('" + p + "'): tamaño > 2GB no representable en integer");
                    pushTc(tc, (int) sz);
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "fileSize('" + p + "'): " + e.getMessage());
                }
                break;
            }
            case IS_DIRECTORY: {
                String p = readVmString(popTcRef(tc));
                pushTc(tc, java.nio.file.Files.isDirectory(sandboxPath(tc, p)) ? 1 : 0);
                break;
            }
            case LAST_MODIFIED: {
                String p = readVmString(popTcRef(tc));
                try {
                    long ms = java.nio.file.Files.getLastModifiedTime(sandboxPath(tc, p)).toMillis();
                    pushTc(tc, (int) (ms & 0x7FFFFFFFL));
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "lastModified('" + p + "'): " + e.getMessage());
                }
                break;
            }

            // ---- N20 — PROMPT(spec): string ----
            case PROMPT: {
                int specRef = popTc(tc);
                String spec = readVmString(specRef);
                // Sin sink al socket → no hay IDE → error atrapable.
                edu.bpgenvm.vm.debug.PromptSender sender = this.promptSender;
                if (sender == null) {
                    throwBpRuntimeError(tc, "prompt: no hay IDE conectado");
                    break;
                }
                long requestId = nextPromptId.getAndIncrement();
                // Registrar el tc ANTES de enviar — para que si la respuesta
                // llega muy rápido (rare, pero existe) el thread esté en el
                // mapa cuando lo busquen.
                pendingPrompts.put(requestId, tc);
                try {
                    sender.send(requestId, spec);
                } catch (Throwable t) {
                    pendingPrompts.remove(requestId);
                    throwBpRuntimeError(tc, "prompt: error enviando al IDE: " + t.getMessage());
                    break;
                }
                // Bloquear el thread: status BLOCKED_PROMPT, yieldRequested.
                // Cuando llegue promptResponse, deliverPromptResponse() (otro
                // thread Java) pondrá el ref del JSON resultado en tc.sp y
                // restaurará a RUNNABLE + runQueue.
                synchronized (vmLock) {
                    tc.status = ThreadStatus.BLOCKED_PROMPT;
                    tc.yieldRequested = true;
                }
                // NO pushTc dummy — la respuesta del IDE produce el ref que
                // se empuja al sp del tc.
                break;
            }

            // ---- Gpio — simulación en PC (logging) ----
            // En la VM Java no hay hardware real; mostramos la acción por
            // stdout para que el desarrollador pueda probar la lógica
            // antes de subir el código a un dispositivo.
            case GPIO_INIT: {
                int mode = popTc(tc);
                int pin  = popTc(tc);
                System.out.println("[gpio] init pin=" + pin + " mode="
                        + (mode == 0 ? "INPUT" : "OUTPUT"));
                pushTc(tc, 0);  // dummy retorno (función void)
                break;
            }
            case GPIO_PULL: {
                int pull = popTc(tc);
                int pin  = popTc(tc);
                String pullStr = pull == 0 ? "NONE" : (pull == 1 ? "UP" : "DOWN");
                System.out.println("[gpio] pull pin=" + pin + " mode=" + pullStr);
                pushTc(tc, 0);
                break;
            }
            case GPIO_WRITE: {
                int val = popTc(tc);
                int pin = popTc(tc);
                System.out.println("[gpio] write pin=" + pin + " value="
                        + (val == 0 ? "LOW" : "HIGH"));
                pushTc(tc, 0);
                break;
            }
            case GPIO_READ: {
                int pin = popTc(tc);
                System.out.println("[gpio] read pin=" + pin
                        + " (sim → siempre 0 en PC)");
                pushTc(tc, 0);   // siempre LOW en simulación
                break;
            }

            // ---- I2C — simulación en PC (logging del frame) ----
            case I2C_INIT: {
                int baud = popTc(tc);
                int scl  = popTc(tc);
                int sda  = popTc(tc);
                int bus  = popTc(tc);
                System.out.println("[i2c] init bus=" + bus
                        + " sda=" + sda + " scl=" + scl
                        + " baud=" + baud);
                pushTc(tc, 0);
                break;
            }
            case I2C_WRITE: {
                int count = popTc(tc);
                int dataRef = popTc(tc);
                int addr = popTc(tc);
                int bus  = popTc(tc);
                StringBuilder sb = new StringBuilder();
                sb.append("[i2c] write bus=").append(bus)
                  .append(" addr=0x").append(Integer.toHexString(addr))
                  .append(" bytes=[");
                for (int i = 0; i < count; i++) {
                    int b1 = readI32(memory, dataRef + 4 + i * 4) & 0xFF;
                    if (i > 0) sb.append(' ');
                    sb.append(String.format("%02X", b1));
                }
                sb.append("]");
                System.out.println(sb);
                pushTc(tc, count);  // devuelve bytes "escritos" en sim
                break;
            }
            case I2C_READ: {
                int count = popTc(tc);
                int dataRef = popTc(tc);
                int addr = popTc(tc);
                int bus  = popTc(tc);
                // En PC simulamos: llenamos con 0x00.
                for (int i = 0; i < count; i++) {
                    writeI32(memory, dataRef + 4 + i * 4, 0);
                }
                System.out.println("[i2c] read bus=" + bus
                        + " addr=0x" + Integer.toHexString(addr)
                        + " count=" + count + " (sim → ceros)");
                pushTc(tc, count);
                break;
            }

            case NEW_INT_ARRAY: {
                int size = popTc(tc);
                if (size < 0) {
                    throwBpRuntimeError(tc, "newIntArray: tamaño negativo: " + size);
                    break;
                }
                int ref = heapAlloc(size * 4, TYPE_ARRAY_I32);
                writeInt32(ref, size);
                for (int i = 0; i < size; i++) {
                    writeInt32(ref + 4 + i * 4, 0);
                }
                                pushTcRef(tc, handleRegister(ref));   // H1.2a: ref 8 bytes
                break;
            }

            // ---- SPI — simulación en PC (logging del frame) ----
            case SPI_INIT: {
                int mode = popTc(tc);
                int baud = popTc(tc);
                int miso = popTc(tc);
                int mosi = popTc(tc);
                int sck  = popTc(tc);
                int bus  = popTc(tc);
                System.out.println("[spi] init bus=" + bus
                        + " sck=" + sck + " mosi=" + mosi + " miso=" + miso
                        + " baud=" + baud + " mode=" + mode);
                pushTc(tc, 0);
                break;
            }
            case SPI_WRITE: {
                int count = popTc(tc);
                int dataRef = popTc(tc);
                int bus  = popTc(tc);
                StringBuilder sb = new StringBuilder();
                sb.append("[spi] write bus=").append(bus).append(" bytes=[");
                for (int i = 0; i < count; i++) {
                    int b1 = readI32(memory, dataRef + 4 + i * 4) & 0xFF;
                    if (i > 0) sb.append(' ');
                    sb.append(String.format("%02X", b1));
                }
                sb.append("]");
                System.out.println(sb);
                pushTc(tc, count);
                break;
            }
            case SPI_READ: {
                int count = popTc(tc);
                int dataRef = popTc(tc);
                int bus  = popTc(tc);
                for (int i = 0; i < count; i++) {
                    writeI32(memory, dataRef + 4 + i * 4, 0);
                }
                System.out.println("[spi] read bus=" + bus
                        + " count=" + count + " (sim → ceros)");
                pushTc(tc, count);
                break;
            }
            case SPI_TRANSFER: {
                int count = popTc(tc);
                int rxRef = popTc(tc);
                int txRef = popTc(tc);
                int bus  = popTc(tc);
                StringBuilder sb = new StringBuilder();
                sb.append("[spi] transfer bus=").append(bus).append(" tx=[");
                for (int i = 0; i < count; i++) {
                    int b1 = readI32(memory, txRef + 4 + i * 4) & 0xFF;
                    if (i > 0) sb.append(' ');
                    sb.append(String.format("%02X", b1));
                    writeI32(memory, rxRef + 4 + i * 4, 0);
                }
                sb.append("] (rx sim → ceros)");
                System.out.println(sb);
                pushTc(tc, count);
                break;
            }

            // ---- UART — simulación en PC (logging del frame) ----
            case UART_INIT: {
                int parity    = popTc(tc);
                int stopBits  = popTc(tc);
                int dataBits  = popTc(tc);
                int baud      = popTc(tc);
                int rx        = popTc(tc);
                int tx        = popTc(tc);
                int bus       = popTc(tc);
                char pchar = (parity == 1) ? 'O' : (parity == 2) ? 'E' : 'N';
                System.out.println("[uart] init bus=" + bus
                        + " tx=" + tx + " rx=" + rx
                        + " baud=" + baud
                        + " " + dataBits + pchar + stopBits);
                pushTc(tc, 0);
                break;
            }
            case UART_WRITE: {
                int count = popTc(tc);
                int dataRef = popTc(tc);
                int bus = popTc(tc);
                StringBuilder sb = new StringBuilder();
                StringBuilder ascii = new StringBuilder();
                sb.append("[uart] write bus=").append(bus).append(" bytes=[");
                for (int i = 0; i < count; i++) {
                    int b1 = readI32(memory, dataRef + 4 + i * 4) & 0xFF;
                    if (i > 0) sb.append(' ');
                    sb.append(String.format("%02X", b1));
                    ascii.append((b1 >= 32 && b1 < 127) ? (char) b1 : '.');
                }
                sb.append("] (\"").append(ascii).append("\")");
                System.out.println(sb);
                pushTc(tc, count);
                break;
            }
            case UART_READ: {
                int timeout = popTc(tc);
                int count   = popTc(tc);
                int dataRef = popTc(tc);
                int bus     = popTc(tc);
                for (int i = 0; i < count; i++) {
                    writeI32(memory, dataRef + 4 + i * 4, 0);
                }
                System.out.println("[uart] read bus=" + bus
                        + " count=" + count + " timeout=" + timeout
                        + " (sim → ceros)");
                pushTc(tc, count);
                break;
            }
            case UART_AVAILABLE: {
                int bus = popTc(tc);
                System.out.println("[uart] available bus=" + bus + " (sim → 0)");
                pushTc(tc, 0);
                break;
            }

            /* ---- Pulse counter ----
             * En host (Java) no hay HW PWM. Mantenemos un contador en
             * memoria por counterId que se incrementa con un método
             * helper (no se llama desde BP por usuario, solo desde el
             * propio __pulseValue para que el stub sea "no salta solo").
             * Para el host también exponemos un mecanismo de "tick"
             * sincrónico: el sample puede simular pulsos llamando a
             * Pulse.start() + un bucle que internamente cuenta, pero la
             * cadena real bpvm → builtin → return queda probada. La
             * comprobación de "cuenta lo que envías" solo es real en
             * Pico con backend HW. */
            case PULSE_INIT: {
                int edgeKind = popTc(tc);
                int pin      = popTc(tc);
                String e = edgeKind == 0 ? "RISING" : edgeKind == 1 ? "FALLING"
                         : edgeKind == 2 ? "BOTH" : "?";
                System.out.println("[pulse] init pin=" + pin + " edge=" + e
                        + " (sim → counterId=0)");
                pulseSimValue = 0;
                pushTc(tc, 0);
                break;
            }
            case PULSE_START: {
                int id = popTc(tc);
                System.out.println("[pulse] start id=" + id + " (sim)");
                pushTc(tc, 0);
                break;
            }
            case PULSE_STOP: {
                int id = popTc(tc);
                System.out.println("[pulse] stop id=" + id
                        + " (sim, value=" + pulseSimValue + ")");
                pushTc(tc, 0);
                break;
            }
            case PULSE_VALUE: {
                int id = popTc(tc);
                System.out.println("[pulse] value id=" + id
                        + " (sim → " + pulseSimValue + ")");
                pushTc(tc, pulseSimValue);
                break;
            }
            case PULSE_RESET: {
                int id = popTc(tc);
                System.out.println("[pulse] reset id=" + id + " (sim)");
                pulseSimValue = 0;
                pushTc(tc, 0);
                break;
            }

            /* ---- PWM ----
             * Host: solo loggea. La validación real (que la duty
             * coincida con la freq, etc.) se hace en Pico con HW. */
            case PWM_INIT: {
                int freqHz = popTc(tc);
                int pin    = popTc(tc);
                System.out.println("[pwm] init pin=" + pin + " freqHz=" + freqHz
                        + " (sim → sliceId=0)");
                pushTc(tc, 0);
                break;
            }
            case PWM_SET_FREQ: {
                int freqHz  = popTc(tc);
                int sliceId = popTc(tc);
                System.out.println("[pwm] setFreq slice=" + sliceId
                        + " freqHz=" + freqHz + " (sim)");
                pushTc(tc, 0);
                break;
            }
            case PWM_SET_DUTY: {
                int dutyPct = popTc(tc);
                int pin     = popTc(tc);
                int sliceId = popTc(tc);
                System.out.println("[pwm] setDuty slice=" + sliceId
                        + " pin=" + pin + " duty=" + dutyPct + "% (sim)");
                pushTc(tc, 0);
                break;
            }
            case PWM_START: {
                int sliceId = popTc(tc);
                System.out.println("[pwm] start slice=" + sliceId + " (sim)");
                pushTc(tc, 0);
                break;
            }
            case PWM_STOP: {
                int sliceId = popTc(tc);
                System.out.println("[pwm] stop slice=" + sliceId + " (sim)");
                pushTc(tc, 0);
                break;
            }

            /* ---- Pico (info MCU) ----
             * En host devolvemos valores estables y razonables para
             * que código BP que use Pico.* corra en desarrollo sin
             * HW. Solo el firmware Pico devuelve datos reales. */
            case PICO_UNIQUE_ID: {
                pushTcRef(tc, allocVmString("host-pc"));
                break;
            }
            case PICO_BOARD_NAME: {
                pushTcRef(tc, allocVmString("host"));
                break;
            }
            case PICO_RESET_CAUSE: {   // H10 — en host no hay causa de reset de MCU
                pushTcRef(tc, allocVmString("unknown"));
                break;
            }
            case PICO_SET_MARK: {      // H10 — breadcrumb (host: sin RAM retenida)
                popTc(tc);             // descarta el code
                pushTc(tc, 0);
                break;
            }
            case PICO_MARK_COUNT: {
                pushTc(tc, 0);         // host: sin trail
                break;
            }
            case PICO_MARK_AT: {
                popTc(tc);             // descarta i
                pushTc(tc, 0);
                break;
            }
            case PICO_BOOT_COUNT: {
                pushTc(tc, 1);         // host: el proceso = 1 arranque
                break;
            }
            case PICO_TEMP_C: {
                pushTc(tc, Float.floatToRawIntBits(25.0f));
                break;
            }
            case PICO_CPU_FREQ_HZ: {
                pushTc(tc, 0);   /* host: no aplica */
                break;
            }
            case PICO_GPIO_COUNT: {
                /* host: perfil RP2350A (30 GPIO). En el device el firmware
                 * lo resuelve desde board_desc (variante / board.json). */
                pushTc(tc, 30);
                break;
            }
            case PICO_ADC_CHANNELS: { pushTc(tc, 4);  break; }   /* host: perfil RP2350 */
            case PICO_PWM_SLICES:   { pushTc(tc, 12); break; }   /* host: perfil RP2350 */
            case NEOPIXEL_INIT: {
                /* device-only (PIO). Host: no-op — descarta args. */
                popTc(tc);            /* pin */
                pushTc(tc, 0);
                break;
            }
            case NEOPIXEL_SHOW: {
                popTc(tc);            /* count */
                popTc(tc);            /* grbRef */
                popTc(tc);            /* pin */
                pushTc(tc, 0);
                break;
            }
            case PICO_UPTIME_MS: {
                /* Sirve algo útil en host: ms del proceso JVM. */
                pushTc(tc, (int) System.currentTimeMillis());
                break;
            }
            case PICO_SET_CPU_FREQ_MHZ: {
                /* En host no hay PLL que reconfigurar. Aceptamos el
                 * parámetro, lo logueamos por trazabilidad de samples
                 * BP, y devolvemos true. El clamp real ocurre en BP
                 * (función setCpuFreqMHz en Pico.bp) usando la
                 * constante MAX_CPU_MHZ — así el contrato es idéntico
                 * en host y en firmware. */
                int mhz = popTc(tc);
                System.out.println("[pico] setCpuFreqMHz(" + mhz + ") (host, no-op)");
                pushTc(tc, 1);
                break;
            }

            /* ---- Rtc — wall clock con offset ajustable ----
             * En host usamos System.currentTimeMillis() directamente,
             * con un offset opcional para que setEpochSec() pueda
             * "rebobinar" o "adelantar" en tests. */
            case RTC_NOW_SEC: {
                long nowMs = System.currentTimeMillis() + rtcOffsetMs;
                pushTc(tc, (int) (nowMs / 1000L));
                break;
            }
            case RTC_SET_NOW_SEC: {
                int sec = popTc(tc);
                long targetMs = (long) sec * 1000L;
                rtcOffsetMs = targetMs - System.currentTimeMillis();
                pushTc(tc, 0);
                break;
            }

            /* ---- Adc — stub host con rampa por canal ---- */
            case ADC_INIT_CHANNEL: {
                int ch = popTc(tc);
                if (ch < 0 || ch > 3) {
                    System.out.println("[adc] initChannel(" + ch + ") fuera de rango");
                    pushTc(tc, -1);
                } else {
                    System.out.println("[adc] initChannel(" + ch + ") → GP" + (26 + ch) + " (host)");
                    pushTc(tc, 26 + ch);
                }
                break;
            }
            case ADC_READ_CHANNEL: {
                int ch = popTc(tc);
                if (ch < 0 || ch > 3) { pushTc(tc, -1); break; }
                /* Rampa determinista para tests: cada canal con offset */
                int v = (adcStubCounter + ch * 1024) & 0x0FFF;
                adcStubCounter = (adcStubCounter + 73) & 0xFFFF;
                pushTc(tc, v);
                break;
            }

            /* ---- Wdt — no-op en host con logging ---- */
            case WDT_ENABLE: {
                int ms = popTc(tc);
                System.out.println("[wdt] enable(" + ms + "ms) (host, no-op)");
                pushTc(tc, 0);
                break;
            }
            case WDT_FEED: {
                /* Silencio — feed se llama mucho. */
                pushTc(tc, 0);
                break;
            }
            case WDT_DISABLE: {
                System.out.println("[wdt] disable (host, no-op)");
                pushTc(tc, 0);
                break;
            }

            default:
                throw new RuntimeException("Builtin no implementado: " + b);
        }
    }

    /** Contador simulado para los hooks de Pulse en host. Solo un
     *  contador a la vez (counterId siempre 0). El sample puede
     *  validar el flow bpvm → builtin → return aunque no se cuente HW. */
    private int pulseSimValue = 0;

    /** Offset entre el wall clock pedido (setNowSec) y el reloj real del
     *  sistema. Mientras es 0, Rtc.nowSec() devuelve el wall clock real
     *  de la JVM. Tras setNowSec(s), el offset cambia para que nowSec()
     *  empiece a contar desde s. */
    private long rtcOffsetMs = 0L;

    /** Contador para el stub ADC en host. Determinista para que tests
     *  reproducibles vean los mismos valores en runs sucesivos. */
    private int adcStubCounter = 0;

    /**
     * Aproximación de Lanczos (g=7, coeficientes Numerical Recipes) para la
     * función gamma. Para enteros positivos pequeños, gamma(n) = (n-1)!.
     * Devuelve double y luego se trunca a float al pushear. Funciona para
     * x > 0 (reflexión de Euler para negativos no implementada — el usuario
     * que la necesite la pide).
     */
    private static double lanczosGamma(double x) {
        // Reflexión: Γ(1-x) Γ(x) = π / sin(π x)
        if (x < 0.5) {
            return Math.PI / (Math.sin(Math.PI * x) * lanczosGamma(1.0 - x));
        }
        double[] p = {
            0.99999999999980993,
            676.5203681218851,
            -1259.1392167224028,
            771.32342877765313,
            -176.61502916214059,
            12.507343278686905,
            -0.13857109526572012,
            9.9843695780195716e-6,
            1.5056327351493116e-7
        };
        x -= 1.0;
        double a = p[0];
        double t = x + 7.5;
        for (int i = 1; i < 9; i++) a += p[i] / (x + i);
        return Math.sqrt(2.0 * Math.PI) * Math.pow(t, x + 0.5) * Math.exp(-t) * a;
    }

    // ====================================================================
    // Helpers para excepciones tipadas e instanceof
    // ====================================================================

    /**
     * Si `ref` es una user-ref válida a una instancia de clase (objeto en heap
     * con header TAG_TYPE = TYPE_OBJECT), devuelve su class_ptr. En caso
     * contrario (null, no-ref, ref a array, ref fuera del heap, etc.) devuelve 0.
     */
    /** #389 — ¿el bloque de la ref es del tipo dado y está vivo? Para el modo
     *  cadena de CHECKCAST (`string(o)`). Mismas validaciones que
     *  classPtrOfRefOr0, sin exigir OBJETO. */
    private boolean blockTypeOfRefIs(long ref, int wantedType) {
        if (ref <= 0) return false;
        int addr = refDeref(ref);
        // Las cadenas viven en DOS sitios (lo cazó el caso 6 del reproductor):
        // heap (construidas, con cabecera) y REGIÓN DE DATOS (literales, sin
        // cabecera). Una dirección de datos en un Object sólo puede ser un
        // literal de cadena — el sistema de tipos no deja entrar otra cosa.
        // ESPEJO de ref_es_cadena en interp.c.
        if (addr > 0 && addr < heapStart) return wantedType == TYPE_ARRAY_I8;
        int headerAddr = addr - 4;
        if (headerAddr < heapStart || headerAddr >= heapNext) return false;
        int tag = readInt32(headerAddr);
        if ((tag & TAG_FREE_BIT) != 0) return false;
        return ((tag & TAG_TYPE_MASK) >>> TAG_TYPE_SHIFT) == wantedType;
    }

    private int classPtrOfRefOr0(long ref) {
        if (ref <= 0) return 0;
        int headerAddr = refDeref(ref) - 4;
        if (headerAddr < heapStart || headerAddr >= heapNext) return 0;
        int tag = readInt32(headerAddr);
        if ((tag & TAG_FREE_BIT) != 0) return 0;
        int type = (tag & TAG_TYPE_MASK) >>> TAG_TYPE_SHIFT;
        if (type != TYPE_OBJECT) return 0;
        return readInt32(headerAddr + 4);
    }

    /**
     * true si `obj_class` (descriptor absoluto) es target o desciende de él.
     * Sube por la cadena vía CLS_OFF_PARENT_OFF (CS-relative al módulo dueño).
     */
    private boolean isDescendantOf(int objClass, int target) {
        int cur = objClass;
        while (cur != 0) {
            if (cur == target) return true;
            int parentOff = readInt32(cur + CLS_OFF_PARENT_OFF);
            if (parentOff == 0) return false;
            int moduleCs = (moduleManager != null) ? moduleManager.getCSForDataAddr(cur) : 0;
            cur = moduleCs + parentOff;
        }
        return false;
    }

    /**
     * Construye un mensaje multi-línea con stack trace para una excepción no
     * atrapada. Recorre los frames de llamada usando los registros guardados
     * en BP-12 (saved PC), BP-8 (saved BP), BP-4 (saved CS) en cada frame.
     *
     * Recibe {@code bpStart} (el BP del thread en el momento del THROW) y
     * {@code stackBase} (la dirección más baja de su región de pila) en lugar
     * de leer this.BP/STACK_BASE. Imprescindible para multi-worker, donde
     * cada thread tiene su propio stack frame chain y su propia región de pila.
     */
    private String buildUnhandledExceptionMessage(int currentPC, int value, int thrownClass,
                                                  int bpStart, int stackBase) {
        StringBuilder sb = new StringBuilder();
        sb.append("Excepción no atrapada (valor=").append(value);
        if (thrownClass != 0) sb.append(", classPtr=").append(thrownClass);
        sb.append(")\nStack trace (innermost first):");
        int curPc = currentPC;
        int curBp = bpStart;
        int safetyCounter = 0;
        while (curBp > stackBase && safetyCounter < 256) {
            sb.append("\n  at ");
            sb.append(moduleManager != null ? moduleManager.describePc(curPc) : "PC=" + curPc);
            int savedPc = readInt32(curBp - 12);
            int savedBp = readInt32(curBp - 8);
            // saved CS no lo necesitamos para el trace
            curPc = savedPc;
            curBp = savedBp;
            safetyCounter++;
        }
        // Frame inicial (top-level main)
        sb.append("\n  at ");
        sb.append(moduleManager != null ? moduleManager.describePc(curPc) : "PC=" + curPc);
        return sb.toString();
    }

    /**
     * Libera de forma determinista el objeto al que apunta `ref` devolviéndolo
     * al free list del allocator. Antes de liberar el bloque propio, recorre
     * los campos marcados como owner en el descriptor y los libera recursivamente.
     *
     * No-op si:
     *   - ref == 0 (null).
     *   - ref no apunta a una instancia de clase válida en el heap.
     *   - el header ya está marcado como libre.
     */
    private void freeOwnedObject(long ref) {
        synchronized (vmLock) { freeOwnedObjectLocked(ref); }
    }

    /** Implementación de freeOwnedObject que asume vmLock ya adquirido (para llamadas recursivas). */
    private void freeOwnedObjectLocked(long ref) {
        if (ref == 0) return;
        // Paso 7b.1 — free RANCIO (slot reciclado, gen no matchea) = NO-OP seguro: si no,
        // derefearía al ocupante NUEVO y liberaría SU bloque → corrupción. ANTES del deref.
        if ((ref & HANDLE_TAG) != 0) {
            int hidx = handleIdx(ref);
            if (hidx > 0 && hidx < handleNext && handleGen[hidx] != handleGenOf(ref)) return;
        }
        int headerAddr = refDeref(ref) - 4;
        if (headerAddr < heapStart || headerAddr >= heapNext) return;
        int tag = readInt32(headerAddr);
        if ((tag & TAG_FREE_BIT) != 0) return;       // ya libre
        handleKill(ref);   // Paso 3/contrato B: el índice queda MUERTO → derefs futuros gritan
        int type = (tag & TAG_TYPE_MASK) >>> TAG_TYPE_SHIFT;

        if (type == TYPE_OBJECT) {
            // Liberación recursiva de campos owner según el class descriptor.
            int classPtr = readInt32(headerAddr + 4);
            int numFields  = readInt16(classPtr + CLS_OFF_NUM_FIELDS)   & 0xFFFF;
            int bitmapWords = readInt16(classPtr + CLS_OFF_BITMAP_WORDS) & 0xFFFF;
            int ownerBitmapBase = classPtr + CLS_OFF_FIELD_BITMAP + bitmapWords * 4;
            for (int i = 0; i < numFields; i++) {
                int word = readInt32(ownerBitmapBase + (i >>> 5) * 4);
                if (((word >> (i & 31)) & 1) != 0) {
                    // V4: sub-campo owner = ref plana 8B (bit en el 1er slot; addr en el low word).
                    long childRef = refLoad(memory, headerAddr + OBJ_HEADER_SIZE + i * 4);
                    freeOwnedObjectLocked(childRef);
                }
            }
        } else if (type == TYPE_ARRAY_REF) {
            // Cascada propietaria: si llegamos aquí es porque alguien (un campo
            // owner del objeto contenedor) considera que ESTE array de refs y
            // su contenido le pertenecen. Liberamos cada slot non-null
            // recursivamente y luego el array como bloque.
            int length = readInt32(headerAddr + 4);   // V4: vía headerAddr (ya derefeado; era readInt32(ref) crudo → con handle leía basura)
            for (int i = 0; i < length; i++) {
                long slotRef = refLoad(memory, headerAddr + OBJ_HEADER_SIZE + i * 8);   // V4 #2: handle 64b COMPLETO (gen) — como el caso OBJECT (5074) y VM-C bpref_load; era (int)readI64 = gen truncada
                if (slotRef != 0) freeOwnedObjectLocked(slotRef);
            }
        }
        // Para TYPE_ARRAY_I8/I16/I32 (sin refs) y para TYPE_OBJECT tras
        // procesar sus owners, simplemente liberamos el bloque.

        int totalSize = objectTotalSize(headerAddr);
        addToFreeList(headerAddr, totalSize);
    }

    /**
     * Push/pop sobre el {@link ThreadContext} recibido (NO sobre this.SP).
     * Imprescindible para multi-worker: cada java thread ejecuta su propio
     * ThreadContext y no comparte SP con otros. Toda la stdlib (builtins)
     * pasa por aquí.
     */
    private void pushTc(ThreadContext tc, int val) {
        writeInt32(tc.sp, val);
        tc.sp += 4;
    }

    private int popTc(ThreadContext tc) {
        tc.sp -= 4;
        return readInt32(tc.sp);
    }

    // H1.2a: refs viajan por el carril de 8 bytes (flat: high=0, low=addr).
    // Los builtins que producen/consumen una referencia (arrays, strings,
    // objetos) deben usar estos en vez de pushTc/popTc (4 bytes) para no
    // desalinear el operand stack contra SET_LOCAL_L/ALOAD/etc.
    private void pushTcRef(ThreadContext tc, long ref) {   // V4: base de la pila de builtins (análogo a push_ref de la VM-C)
        refStore(memory, tc.sp, ref);
        tc.sp += REF_SIZE;
    }
    private long popTcRef(ThreadContext tc) {
        tc.sp -= REF_SIZE;
        return refLoad(memory, tc.sp);
    }

    // ============================================================
    // H5.c — Drenaje de la cola de eventos (espejo de events.c)
    // ============================================================

    /**
     * Saca UN evento para {@code tc} y le monta el frame del handler. Lo llama
     * el scheduler ENTRE QUANTA, con el tc ya asignado a este worker: no está
     * corriendo, así que su pc/sp/bp/cs son los buenos.
     *
     * <p>Uno solo por punto de planificación: inyectar dos seguidos los
     * ejecutaría en orden inverso (el segundo frame queda encima) y los eventos
     * son FIFO.
     *
     * <p>El frame es el que montaría una llamada normal, byte a byte — no hay
     * convención de eventos aparte. Lo único propio es la vuelta: el saved PC
     * apunta al SENTINELA (memory[2] = EVENT_RETURN), porque el RET del handler
     * deja su valor de retorno en la pila y el código interrumpido no lo espera.
     * El PC real de reanudación va EN LA PILA, debajo de los argumentos, para
     * que un handler interrumpido por otro evento no pise la vuelta del primero.
     */
    private void drainOneEvent(ThreadContext tc) {
        if (tc.evDepth > 0) return;   // un handler a la vez: FIFO de verdad
        PendingEvent ev = null;
        for (java.util.Iterator<PendingEvent> it = eventQueue.iterator(); it.hasNext(); ) {
            PendingEvent e = it.next();
            if (e.tid == tc.id) { ev = e; it.remove(); break; }
        }
        if (ev == null) return;
        // #342 — si esto es drenaje POST-MORTEM, gasta presupuesto. Se descuenta
        // al SACARLO de la cola y no tras inyectar, para que un evento cuyo
        // receptor murió también cuente: si no, el thread volvería a resucitar
        // por un evento que ya no está.
        if (tc.evPostMortem > 0) tc.evPostMortem--;
        if (System.getenv("BPVM_DEBUG_EV") != null) {
            StringBuilder q = new StringBuilder();
            for (PendingEvent e : eventQueue) q.append(e.args.length > 0 ? e.args[0] : -1).append(' ');
            System.err.println("[ev] drain arg0=" + (ev.args.length > 0 ? ev.args[0] : -1)
                    + " tid=" + tc.id + " restoQ=[" + q.toString().trim() + "]");
        }

        // El receptor puede haber muerto entre el raise y el drenaje: es el caso
        // normal al destruir un suscriptor con eventos pendientes, y el diseño
        // dice que un evento sin quien lo escuche se IGNORA, no revienta.
        int idx = handleIdx(ev.recv);
        if ((ev.recv & HANDLE_TAG) != 0
                && (idx <= 0 || idx >= handleNext || handleGen[idx] != handleGenOf(ev.recv))) return;
        int obj = refDeref(ev.recv);
        if (obj == 0) return;

        // Resolver el slot sobre la clase REAL, subiendo por la herencia igual
        // que INVOKE_VIRTUAL.
        byte[] mem = memory;
        int desc = readI32(mem, obj), methodOff = -1, targetCS = -1;
        while (true) {
            int bitmapW  = readI16(mem, desc + CLS_OFF_BITMAP_WORDS) & 0xFFFF;
            int nMethods = readI16(mem, desc + CLS_OFF_NUM_METHODS)  & 0xFFFF;
            int vtBase   = desc + CLS_OFF_FIELD_BITMAP + 2 * bitmapW * 4;
            if (ev.dest >= 0 && ev.dest < nMethods) {
                int off = readI32(mem, vtBase + ev.dest * 4);
                if (off != -1) { methodOff = off; targetCS = moduleManager.getCSForDataAddr(desc); break; }
            }
            int parentOff = readI32(mem, desc + CLS_OFF_PARENT_OFF);
            if (parentOff == 0) {
                System.err.println("[bpvm] evento: slot " + ev.dest + " no resoluble en la"
                        + " clase del receptor — descartado");
                return;
            }
            desc = moduleManager.getCSForDataAddr(desc) + parentOff;
        }

        int need = 4 + REF_SIZE + 12;
        for (int i = 0; i < ev.args.length; i++) need += ((ev.masks & (1 << (8 + i))) != 0) ? 8 : 4;
        if (tc.sp + need > tc.stackTop) {
            System.err.println("[bpvm] evento: sin pila en tid=" + tc.id + " — descartado");
            return;
        }

        int sp = tc.sp;
        // #342 — si el thread ya había terminado y sólo lo hemos resucitado para
        // saldar su deuda, su pc apunta a DESPUÉS del HALT/THREAD_EXIT: volver
        // ahí sería ejecutar lo que hubiera detrás. La vuelta correcta es el
        // sentinela de fin de thread (memory[0]), que lo termina limpiamente.
        int resumePc = (tc.evPostMortem >= 0) ? 0 : tc.pc;
        writeI32(mem, sp, resumePc); sp += 4;              // PC de reanudación, bajo todo
        refStore(mem, sp, ev.recv); sp += REF_SIZE;        // this
        for (int i = 0; i < ev.args.length; i++) {
            if ((ev.masks & (1 << (8 + i))) != 0) { writeI64(mem, sp, ev.args[i]); sp += 8; }
            else { writeI32(mem, sp, (int) ev.args[i]); sp += 4; }
        }
        writeI32(mem, sp, 2); sp += 4;                     // saved PC = sentinela (memory[2])
        writeI32(mem, sp, tc.bp); sp += 4;
        writeI32(mem, sp, tc.cs); sp += 4;

        tc.sp = sp;
        tc.bp = sp;
        tc.cs = targetCS;
        tc.pc = targetCS + methodOff;
        tc.evDepth++;
    }

    // ============================================================
    // H3.4 — Upcall de eventos GUI (modelo de INTERRUPCION)
    // ============================================================

    /**
     * Cuerpo del builtin __guiRun: el lazo de eventos. El EDT de Swing publica
     * los clics en una cola (gui.takeEvent); ESTE worker — el que llamó a
     * __guiRun — los saca y ejecuta el handler onClick como llamada ANIDADA
     * sobre su propio tc, igual que una CPU atendiendo una IRQ entre
     * instrucciones. Termina al cerrarse la ventana (EVENT_CLOSE). El bytecode
     * BP corre SIEMPRE en este worker; el EDT nunca ejecuta BP.
     */
    private void guiEventLoop(ThreadContext tc) {
        gui.start();
        while (true) {
            int[] ev = gui.takeEvent();     // {objptr, kind}; objptr==EVENT_CLOSE para cerrar
            int objptr = ev[0], kind = ev[1];
            if (objptr == edu.bpgenvm.gui.GuiBackend.EVENT_CLOSE || killRequested) break;
            if (objptr != 0) invokeGuiDispatch(tc, objptr, kind);
        }
    }

    /* #324 — estado del bombeo por pasadas. start() NO es idempotente (en
     * headless encola EVENT_CLOSE cada vez), y el cierre hay que recordarlo
     * porque EVENT_CLOSE se consume una sola vez. */
    private boolean guiStarted = false;
    private boolean guiClosed  = false;

    /**
     * #324 — UNA pasada del bombeo, en vez del lazo entero. Devuelve true =
     * "vuelve a llamarme", false = "no queda nada".
     *
     * POR QUÉ. guiEventLoop bloqueaba dentro del builtin hasta cerrar la ventana,
     * y mientras tanto la VM entera estaba parada: ni avanzaban los threads ni se
     * drenaba la cola de eventos (el scheduler sólo inyecta ENTRE quanta, y no
     * había ninguno). Con el lazo en Gui.run() (BP) hay frontera de instrucción
     * entre pasadas. Medido en samples/GuiEvSpike.bp.
     *
     * El "una vuelta más" al cerrarse no es un detalle: si esta pasada drenó algo,
     * puede haber handlers de evento encolados que aún no han corrido. Devolver
     * false ya los perdería.
     */
    private boolean guiEventLoopOnce(ThreadContext tc) {
        if (!guiStarted) { gui.start(); guiStarted = true; }
        int drained = 0;
        int[] ev;
        while ((ev = gui.pollEvent()) != null) {
            int objptr = ev[0], kind = ev[1];
            if (objptr == edu.bpgenvm.gui.GuiBackend.EVENT_CLOSE) { guiClosed = true; break; }
            if (objptr != 0) { invokeGuiDispatch(tc, objptr, kind); drained++; }
        }
        if (killRequested) return false;
        // Ventana cerrada: sólo se sale cuando NO queda trabajo. "Trabajo" incluye
        // los eventos BP encolados y aún sin entregar — el `raise` de un handler
        // encola, y el frame lo inyecta el scheduler ENTRE quanta, así que salir
        // aquí los perdería. Misma condición que la VM-C (builtins.c, GUI_RUN_ONCE).
        if (guiClosed) return drained > 0 || !eventQueue.isEmpty();
        /* Ventana viva y cola vacía: espera corta para no girar en vacío. Es el
         * `delay` del lazo de LVGL, puesto donde el bombeo puede pagarlo sin que
         * el lazo BP tenga que saber de tiempos. */
        if (drained == 0) {
            try { Thread.sleep(15); } catch (InterruptedException ie) { Thread.currentThread().interrupt(); return false; }
        }
        return true;
    }

    /**
     * Llama (anidado) a la función BP Gui.__guiDispatch(self), cuyo cuerpo es
     * self.onClick() — dispatch virtual que el COMPILADOR resuelve por nombre
     * (respeta el override del usuario; la VM no conoce ningún slot). Resuelve
     * la entrada de la función por nombre (resolveGlobal, cacheada) y monta un
     * frame de llamada con PC de retorno centinela = 0 (mem[0] = THREAD_EXIT)
     * para cerrar el run anidado, como hace THREAD_START. La pila de Java
     * preserva el contexto del lazo exterior; aquí restauramos tc.* al volver.
     */
    private void invokeGuiDispatch(ThreadContext tc, int objptr, int kind) {
        if (guiDispatchPc == -2) {   // resolver una sola vez (ambos dispatchers)
            Integer pcClick = (moduleManager != null)
                    ? moduleManager.resolveGlobal("Gui.__guiDispatch") : null;
            if (pcClick == null) { guiDispatchPc = -1; }
            else { guiDispatchPc = pcClick; guiDispatchCs = moduleManager.getModuleBaseFromPC(pcClick); }
            Integer pcChange = (moduleManager != null)
                    ? moduleManager.resolveGlobal("Gui.__guiDispatchChange") : null;
            if (pcChange == null) { guiDispatchChangePc = -1; }
            else { guiDispatchChangePc = pcChange; guiDispatchChangeCs = moduleManager.getModuleBaseFromPC(pcChange); }
        }
        // V3/H6 — elige dispatcher por tipo de evento: click → onClick, change → onChange.
        int dispPc = (kind == edu.bpgenvm.gui.GuiBackend.KIND_CHANGE) ? guiDispatchChangePc : guiDispatchPc;
        int dispCs = (kind == edu.bpgenvm.gui.GuiBackend.KIND_CHANGE) ? guiDispatchChangeCs : guiDispatchCs;
        if (dispPc < 0) return;   // Gui no cargado / sin dispatcher

        int base = tc.sp;
        int savedPc = tc.pc, savedBp = tc.bp, savedCs = tc.cs;
        ThreadStatus savedStatus = tc.status;
        boolean savedYield = tc.yieldRequested;
        // Frame del dispatcher(self): [self(8B ref), savedPC=0(centinela), savedBP,
        // savedCS], con bp tras ellos (convención de CALL/INVOKE_VIRTUAL). Según
        // `kind`, dispPc/dispCs apuntan a __guiDispatch (click→onClick) o a
        // __guiDispatchChange (change→onChange).
        // #302: self es una REF → 8 bytes con la gen VIVA (regenRef). Escribirla a
        // 4B dejaba la palabra alta con basura rancia de pila → UAF por lotería
        // (espejo del ref_mask del bridge_run_bp_frame de la VM-C, 79ab1b9).
        refStore(memory, base, regenRef(objptr));   // arg: self (ref 8B)
        writeInt32(base + 8,  0);          // saved PC = 0 → mem[0] = THREAD_EXIT
        writeInt32(base + 12, savedBp);    // saved BP (valor sano; se restaura abajo)
        writeInt32(base + 16, savedCs);    // saved CS
        tc.bp = base + 20;
        tc.sp = base + 20;
        tc.pc = dispPc;
        tc.cs = dispCs;
        tc.yieldRequested = false;
        try {
            runOnContext(tc);   // corre el dispatcher → self.onClick()/onChange() hasta el centinela
        } catch (Throwable t) {
            // Un handler que lanza no debe tumbar el lazo GUI: lo reportamos.
            System.err.println("[gui dispatch] " + t);
        } finally {
            tc.sp = base;            // descarta el frame de onClick
            tc.bp = savedBp;
            tc.cs = savedCs;
            tc.pc = savedPc;
            tc.status = savedStatus; // el centinela THREAD_EXIT lo puso TERMINATED
            tc.yieldRequested = savedYield;
        }
    }

    /**
     * H13 (V3) — call-by-name de un handler de Forms. Resuelve `name` como
     * función pública del MÓDULO al que pertenece `ownerRef` (la ventana) y la
     * invoca ANIDADA con `sender` como arg0. Mismo patrón de frame que
     * invokeGuiDispatch ([arg, PC=0 centinela, BP, CS] + runOnContext hasta el
     * centinela THREAD_EXIT en mem[0]). Lanza RuntimeError BP si el handler no
     * existe (typo en el .win) para que el error sea claro y no silencioso.
     */
    private void invokeHandlerByName(ThreadContext tc, int ownerRef, String name, int sender) {
        if (moduleManager == null) return;
        if (ownerRef == 0) return;                          // sin ventana → ignorar
        int classPtr = readInt32(refDeref(ownerRef));       // V4: deref handle→addr; class_ptr en +0
        int moduleCs = moduleManager.getCSForDataAddr(classPtr);
        Integer pc = moduleManager.resolveExportInModule(moduleCs, name);
        if (pc == null) {
            // H13 (decisión de Eduardo): handler no implementado → IGNORAR (sin
            // excepción). El aviso se da UNA vez al cargar el form (Gui.Window),
            // no en cada pulsación: ni spam ni tumbar el GUI por un form a medias.
            return;
        }
        int cs = moduleManager.getModuleBaseFromPC(pc);
        int base = tc.sp;
        int savedPc = tc.pc, savedBp = tc.bp, savedCs = tc.cs;
        ThreadStatus savedStatus = tc.status;
        boolean savedYield = tc.yieldRequested;
        // #302: sender es una REF → 8 bytes con gen viva (ver regenRef).
        refStore(memory, base, regenRef(sender));   // arg0 = sender (ref 8B)
        writeInt32(base + 8,  0);        // saved PC = 0 → mem[0] = THREAD_EXIT (centinela)
        writeInt32(base + 12, savedBp);
        writeInt32(base + 16, savedCs);
        tc.bp = base + 20;
        tc.sp = base + 20;
        tc.pc = pc;
        tc.cs = cs;
        tc.yieldRequested = false;
        try {
            runOnContext(tc);
        } finally {
            tc.sp = base;
            tc.bp = savedBp;
            tc.cs = savedCs;
            tc.pc = savedPc;
            tc.status = savedStatus;
            tc.yieldRequested = savedYield;
        }
    }

    /**
     * H13.1 (V3) — Forms Camino A: invoca (anidado) un MÉTODO de la ventana por
     * su SLOT de vtable (que el IDE horneó en el .win, resuelto vía .bpi/slotOf).
     * Resuelve la dirección absoluta en vtable[slot] del class_ptr de `winRef`
     * (idéntico a como THREAD_START resuelve run()) y monta el frame de método
     * [this=win, sender, savedPC=0 (centinela), savedBP, savedCS]. Mismo patrón de
     * upcall anidado que invokeHandlerByName. slot < 0 (handler ausente) → ignora.
     */
    private void invokeHandlerBySlot(ThreadContext tc, int winRef, int slot, int sender) {
        if (winRef == 0 || slot < 0) return;            // sin ventana / handler ausente → ignorar
        int classPtr   = readInt32(refDeref(winRef));   // #302: deref handle→addr (como invokeHandlerByName)
        int targetCS   = moduleManager.getCSForDataAddr(classPtr);
        int bitmapW    = readInt16(classPtr + CLS_OFF_BITMAP_WORDS) & 0xFFFF;
        int vtableBase = classPtr + CLS_OFF_FIELD_BITMAP + 2 * bitmapW * 4;
        int methodOff  = readInt32(vtableBase + slot * 4);
        int methodPc   = targetCS + methodOff;
        int base = tc.sp;
        int savedPc = tc.pc, savedBp = tc.bp, savedCs = tc.cs;
        ThreadStatus savedStatus = tc.status;
        boolean savedYield = tc.yieldRequested;
        // Frame de método: [this=win(8B), sender(8B), savedPC=0, savedBP, savedCS],
        // bp tras ellos. #302: ambos son REFS → 8 bytes con gen viva (ver regenRef).
        refStore(memory, base,     regenRef(winRef));   // local0 = this (la ventana)
        refStore(memory, base + 8, regenRef(sender));   // local1 = sender (el widget que disparó)
        writeInt32(base + 16, 0);        // saved PC = 0 → mem[0] = THREAD_EXIT (centinela)
        writeInt32(base + 20, savedBp);
        writeInt32(base + 24, savedCs);
        tc.bp = base + 28;
        tc.sp = base + 28;
        tc.pc = methodPc;
        tc.cs = targetCS;
        tc.yieldRequested = false;
        try {
            runOnContext(tc);
        } finally {
            tc.sp = base;
            tc.bp = savedBp;
            tc.cs = savedCs;
            tc.pc = savedPc;
            tc.status = savedStatus;
            tc.yieldRequested = savedYield;
        }
    }

    public void writeInt32(int addr, int value) {
        memory[addr]     = (byte) ((value >> 24) & 0xFF);
        memory[addr + 1] = (byte) ((value >> 16) & 0xFF);
        memory[addr + 2] = (byte) ((value >> 8)  & 0xFF);
        memory[addr + 3] = (byte) ( value        & 0xFF);
    }

    public int readInt32(int addr) {
        return ((memory[addr]     & 0xFF) << 24)
             | ((memory[addr + 1] & 0xFF) << 16)
             | ((memory[addr + 2] & 0xFF) <<  8)
             |  (memory[addr + 3] & 0xFF);
    }

    private int readInt16(int addr) {
        return ((memory[addr] & 0xFF) << 8) | (memory[addr + 1] & 0xFF);
    }

    // =================================================================
    // API segura para el debugger (sin ejecutar bytecode)
    // =================================================================

    /** Lectura pública de 4 bytes signed-int. Usada por el debugger para
     *  inspeccionar globals/properties via ModuleManager. */
    public int readMemoryInt(int addr) {
        if (addr < 0 || addr + 4 > memory.length) return 0;
        return readInt32(addr);
    }

    /**
     * H7 — calculadora de constantes para el builtin eval(). Descenso recursivo
     * que evalúa SOBRE LA MARCHA (sin AST ni bytecode): + - * / paréntesis y
     * unario sobre literales numéricos. Computa en double. Error de sintaxis ->
     * NaN. Réplica byte-a-byte de bpvm_eval_calc() de la VM-C (builtins.c): mismas
     * operaciones double + parseo numérico MANUAL (no Double.parseDouble) para no
     * depender de la librería y garantizar paridad.
     */
    private static final class EvalCalc {
        private final String s; private int pos; private boolean err;
        private EvalCalc(String s) { this.s = s; }
        static double run(String s) {
            EvalCalc c = new EvalCalc(s);
            double v = c.expr(); c.ws();
            if (c.pos != s.length()) c.err = true;
            return c.err ? Double.NaN : v;
        }
        private void ws() { while (pos < s.length() && (s.charAt(pos) == ' ' || s.charAt(pos) == '\t')) pos++; }
        private double expr() {
            double v = term(); ws();
            while (pos < s.length() && (s.charAt(pos) == '+' || s.charAt(pos) == '-')) {
                char op = s.charAt(pos++); double r = term(); v = (op == '+') ? v + r : v - r; ws();
            }
            return v;
        }
        private double term() {
            double v = factor(); ws();
            while (pos < s.length() && (s.charAt(pos) == '*' || s.charAt(pos) == '/')) {
                char op = s.charAt(pos++); double r = factor(); v = (op == '*') ? v * r : v / r; ws();
            }
            return v;
        }
        private double factor() {
            ws();
            if (pos >= s.length()) { err = true; return 0; }
            char c = s.charAt(pos);
            if (c == '-') { pos++; return -factor(); }
            if (c == '+') { pos++; return factor(); }
            if (c == '(') {
                pos++; double v = expr(); ws();
                if (pos < s.length() && s.charAt(pos) == ')') pos++; else err = true;
                return v;
            }
            return number();
        }
        private double number() {
            ws();
            double v = 0; boolean any = false;
            while (pos < s.length() && s.charAt(pos) >= '0' && s.charAt(pos) <= '9') { v = v * 10 + (s.charAt(pos) - '0'); pos++; any = true; }
            if (pos < s.length() && s.charAt(pos) == '.') {
                pos++; double sc = 1;
                while (pos < s.length() && s.charAt(pos) >= '0' && s.charAt(pos) <= '9') { v = v * 10 + (s.charAt(pos) - '0'); sc *= 10; pos++; any = true; }
                v = v / sc;
            }
            if (!any) err = true;
            return v;
        }
    }

    /**
     * Intenta leer un BP string en `ref` (= user_ref a un array de chars con
     * length en bytes 0..3). Devuelve la representación Java o "@addr" si
     * no se puede interpretar. Sin side-effects: no aloca, no muta memoria.
     */
    public String readStringIfPossible(int ref) {
        try {
            if (ref <= 0 || ref + 4 > memory.length) return "@" + Integer.toHexString(ref);
            int nbytes = readInt32(ref);                       // H2: length en bytes (UTF-8)
            if (nbytes < 0 || nbytes > 65536) return "@" + Integer.toHexString(ref);  // sanity
            int end = ref + 4 + nbytes;
            if (end > memory.length) return "@" + Integer.toHexString(ref);
            byte[] buf = new byte[nbytes];
            System.arraycopy(memory, ref + 4, buf, 0, nbytes);
            String s = new String(buf, java.nio.charset.StandardCharsets.UTF_8);
            StringBuilder sb = new StringBuilder(s.length() + 2);
            sb.append('"');
            for (int i = 0; i < s.length(); ) {
                int cp = s.codePointAt(i);
                i += Character.charCount(cp);
                if (cp == '"') sb.append("\\\"");
                else if (cp == '\\') sb.append("\\\\");
                else if (cp == '\n') sb.append("\\n");
                else if (cp == '\r') sb.append("\\r");
                else if (cp == '\t') sb.append("\\t");
                else sb.appendCodePoint(cp);
            }
            sb.append('"');
            return sb.toString();
        } catch (Throwable t) {
            return "@" + Integer.toHexString(ref);
        }
    }
}
