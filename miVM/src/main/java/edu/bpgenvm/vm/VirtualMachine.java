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
    /** Número de workers Java en paralelo (cores físicos simulados). Default 2. */
    private int numWorkers = 2;
    public void setNumWorkers(int n) { this.numWorkers = Math.max(1, n); }
    public int getNumWorkers() { return numWorkers; }

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
            int ref = allocVmString(valuesJson == null ? "" : valuesJson);
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
    private void terminateThread(ThreadContext t) {
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
    private String readRuntimeErrorMsg(int objRef) {
        try {
            if (objRef <= 0) return null;
            int msgRef = readInt32(objRef + 4 + 0 * 4);
            if (msgRef <= 0) return null;
            return readStringIfPossible(msgRef);
        } catch (Throwable t) {
            return null;
        }
    }

    public void throwBpRuntimeError(ThreadContext tc, String message) {
        ModuleManager mm = getModuleManager();
        if (mm == null) {
            throw new BpThreadFault(message);
        }
        Integer classPtrBox = mm.resolveExportInModule(tc.cs, "RuntimeError");
        if (classPtrBox == null) {
            // El módulo del thread actual no exportó RuntimeError (es un
            // .mod antiguo emitido antes de B3 v2, o un caso degenerado).
            // Fallback al comportamiento previo.
            throw new BpThreadFault(message);
        }
        int classPtr = classPtrBox;

        int msgRef;
        int objRef;
        // Alocamos string + objeto bajo vmLock para sincronizar con GC.
        synchronized (vmLock) {
            msgRef = allocVmString(message == null ? "" : message);
            int numFields = readInt16(classPtr + CLS_OFF_NUM_FIELDS) & 0xFFFF;
            objRef = heapAlloc(numFields * 4, TYPE_OBJECT);
            writeInt32(objRef, classPtr);
            for (int i = 0; i < numFields; i++) {
                writeInt32(objRef + 4 + i * 4, 0);
            }
            // Field `msg` está en slot 0 (la clase sintetizada lo declara
            // primero y es el único campo).
            writeInt32(objRef + 4 + 0 * 4, msgRef);
            // Empujamos el ref al stack del thread. El dispatcher hará el
            // pop como parte del unwind, igual que con el opcode THROW.
            writeInt32(tc.sp, objRef);
            tc.sp += 4;
            // B1 residual — anclamos el objRef al thread para que el GC
            // no lo libere entre soltar vmLock aquí y el unwind del catch
            // BpExceptionPending del intérprete. El stack tiene el ref pero
            // el GC no traza por type info — necesita la ancla.
            tc.allocAnchor = objRef;
        }
        throw new BpExceptionPending(objRef);
    }

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
     * Variante con tamaño total y `stackBase` (donde termina el heap y
     * empiezan los stacks). Llamada desde {@link edu.bpgenvm.Main} cuando
     * hay un BpVM.cfg activo. Validaciones:
     *   memorySize &gt; 0
     *   stackBase  &gt; 0
     *   stackBase  &lt; memorySize
     *   stackBase + MAIN_STACK_BYTES &lt;= memorySize  (cabe al menos el main)
     */
    public VirtualMachine(int memorySize, int stackBase) {
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
        this.nextStackBase = stackBase + MAIN_STACK_BYTES;

        // memory[0] = 0x70 (opcode THREAD_EXIT). Es la sentinela de salida
        // de los workers: cuando su run() hace RET, el saved PC apunta a 0
        // y la siguiente instrucción fetcheada es THREAD_EXIT, que termina
        // EL THREAD ACTUAL sin tumbar la VM. HALT (0x00) ahora es exclusivo
        // del thread main (termina la VM entera).
        memory[0] = (byte) 0x70;

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
            case TYPE_ARRAY_REF: return 4;
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

            int addr = tryAllocateInner(totalSize);
            if (addr != -1) {
                int tag = (type << TAG_TYPE_SHIFT);
                writeInt32(addr, tag);
                int userRef = addr + 4;
                // Ancla GC: el ref vuelve al caller que aún tiene que
                // publicarlo (write a stack/field) sin lock. Si entre
                // medias otro worker dispara GC, scanRegion ve este ancla
                // y marca el objeto como vivo.
                if (me != null) me.allocAnchor = userRef;
                return userRef;
            }

            // Sin espacio: hay que correr GC. Pedimos un safepoint
            // stop-the-world (B1) — todos los demás workers deben sincronizar
            // sus tc.{sp,bp,cs,pc} y parquear, si no el GC marcará desde tc.sp
            // desactualizado y liberará objetos que aún están en uso, dejando
            // refs colgantes que se manifiestan como bytecodes basura,
            // direcciones inválidas, mutex.id corruptos, etc.
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
                // Mundo parado: GC seguro. Nos desmarcamos de parked porque
                // ya estamos ejecutando GC (no esperando).
                if (myTid >= 0) parkedInHeapAlloc.remove(myTid);
                gcLocked();
            } finally {
                if (myTid >= 0) parkedInHeapAlloc.remove(myTid);
                stopTheWorld = false;
                gcInProgress = false;
                vmLock.notifyAll();
            }
            addr = tryAllocateInner(totalSize);
            if (addr == -1) {
                throw new RuntimeException("Heap overflow tras GC: pido " + totalSize
                        + " bytes; libre=" + (STACK_BASE - heapNext));
            }
            int tag = (type << TAG_TYPE_SHIFT);
            writeInt32(addr, tag);
            int userRef2 = addr + 4;
            if (me != null) me.allocAnchor = userRef2;
            return userRef2;
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
            if (blockSize >= totalSize) {
                int next = readInt32(cur + 8);
                int remaining = blockSize - totalSize;
                if (remaining >= MIN_FREE_BLOCK) {
                    // Split: usar cur..cur+totalSize, dejar cur+totalSize..cur+blockSize como libre
                    int newFreeAddr = cur + totalSize;
                    writeInt32(newFreeAddr, TAG_FREE_BIT);
                    writeInt32(newFreeAddr + 4, remaining);
                    writeInt32(newFreeAddr + 8, next);
                    if (prev == 0) freeListHead = newFreeAddr;
                    else writeInt32(prev + 8, newFreeAddr);
                } else {
                    // Usar bloque completo; quitarlo de la lista
                    if (prev == 0) freeListHead = next;
                    else writeInt32(prev + 8, next);
                }
                return cur;
            }
            prev = cur;
            cur = readInt32(cur + 8);
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
                int childRef = readInt32(headerAddr + OBJ_HEADER_SIZE + i * 4);
                int childHeader = childRef - 4; // user_ref apunta a length
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
                    int childRef = readInt32(headerAddr + OBJ_HEADER_SIZE + i * 4);
                    int childHeader = childRef - 4;
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
            int headerAddr = candidate - 4;
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
                int headerAddr = t.allocAnchor - 4;
                if (valid.contains(headerAddr)) {
                    markObject(headerAddr, valid);
                }
            }
        }

        // Roots: data blocks de todos los módulos
        if (moduleManager != null) {
            List<int[]> regions = moduleManager.getDataRegions();
            for (int[] region : regions) {
                scanRegion(region[0], region[0] + region[1], valid);
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
            addToFreeList(pendingFreeStart, pendingFreeSize);
        }

        int afterFreeListBytes = 0;
        {
            int p = freeListHead;
            while (p != 0) { afterFreeListBytes += readInt32(p + 4); p = readInt32(p + 8); }
        }

        System.out.printf("VM [GC]: heap=%d bytes (alive=%d, libres=%d, bump_remain=%d) | antes free_list=%d%n",
                beforeBumpUsed, aliveBytes, afterFreeListBytes, STACK_BASE - heapNext, beforeFreeListBytes);
    }

    private void addToFreeList(int addr, int size) {
        writeInt32(addr, TAG_FREE_BIT);
        writeInt32(addr + 4, size);
        writeInt32(addr + 8, freeListHead);
        freeListHead = addr;
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
                                // en runOnContext). Termina la VM entera.
                                vmShutdown = true;
                                vmLock.notifyAll();
                                return;
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
            if (stopTheWorld) {
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

                case 0x1C: { // LEA_GLOBAL
                    short off = (short) readI16(mem, pc); pc += 2;
                    writeI32(mem, sp, cs + off); sp += 4;
                    break;
                }
                case 0x23: { // LEA_LOCAL
                    short off = (short) readI16(mem, pc); pc += 2;
                    writeI32(mem, sp, bp + off); sp += 4;
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
                        writeI32(mem, sp, ref); sp += 4;
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
                        writeI32(mem, sp, ref); sp += 4;
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
                        writeI32(mem, sp, ref); sp += 4;
                        tc.sp = sp;
                    }
                    break;
                }

                case 0x1E: { // ALOAD
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    if (idx < 0 || idx >= length) {
                        tc.sp = sp;
                        throwBpRuntimeError(tc, "ALOAD: índice fuera de rango "
                                + idx + " (length=" + length + ")");
                    }
                    writeI32(mem, sp, readI32(mem, ref + 4 + idx * 4)); sp += 4;
                    break;
                }
                case 0x1F: { // ASTORE
                    sp -= 4; int val = readI32(mem, sp);
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    if (idx < 0 || idx >= length) {
                        tc.sp = sp;
                        throwBpRuntimeError(tc, "ASTORE: índice fuera de rango "
                                + idx + " (length=" + length + ")");
                    }
                    writeI32(mem, ref + 4 + idx * 4, val);
                    break;
                }
                case 0x20: { // ALEN
                    sp -= 4; int ref = readI32(mem, sp);
                    writeI32(mem, sp, readI32(mem, ref)); sp += 4;
                    break;
                }

                case 0x21: { // PRINT_CHAR
                    sp -= 4; int c = readI32(mem, sp);
                    programOut.writeChar((char) c);
                    break;
                }
                case 0x22: { // PRINT_STRING (legacy: con \n al final)
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    StringBuilder sb = new StringBuilder(length);
                    for (int i = 0; i < length; i++) {
                        sb.append((char) readI32(mem, ref + 4 + i * 4));
                    }
                    programOut.writeText(sb.toString());
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
                case 0x35: { // FPRINT
                    sp -= 4; float v = Float.intBitsToFloat(readI32(mem, sp));
                    System.out.println("VM [FPRINT]: " + v);
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
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ALOAD_I8: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    writeI32(mem, sp, (int) mem[ref + 4 + idx]); sp += 4;
                    break;
                }
                case 0x3B: { // ALOAD_U8
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ALOAD_U8: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    writeI32(mem, sp, mem[ref + 4 + idx] & 0xFF); sp += 4;
                    break;
                }
                case 0x3C: { // ALOAD_I16
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ALOAD_I16: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    int addr = ref + 4 + idx * 2;
                    int raw = ((mem[addr] & 0xFF) << 8) | (mem[addr + 1] & 0xFF);
                    writeI32(mem, sp, (short) raw); sp += 4;
                    break;
                }
                case 0x3D: { // ALOAD_U16
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ALOAD_U16: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    int addr = ref + 4 + idx * 2;
                    int raw = ((mem[addr] & 0xFF) << 8) | (mem[addr + 1] & 0xFF);
                    writeI32(mem, sp, raw); sp += 4;
                    break;
                }

                case 0x3E: { // ASTORE_I8
                    sp -= 4; int val = readI32(mem, sp);
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ASTORE_I8: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    mem[ref + 4 + idx] = (byte) val;
                    break;
                }
                case 0x3F: { // ASTORE_I16
                    sp -= 4; int val = readI32(mem, sp);
                    sp -= 4; int idx = readI32(mem, sp);
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    if (idx < 0 || idx >= length) { tc.sp=sp; throwBpRuntimeError(tc, "ASTORE_I16: idx fuera de rango " + idx + " (len=" + length + ")"); }
                    int addr = ref + 4 + idx * 2;
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
                        writeI32(mem, sp, ref); sp += 4;
                        tc.sp = sp;
                    }
                    break;
                }
                case 0x53: { // GET_FIELD
                    int slot = mem[pc] & 0xFF; pc++;
                    sp -= 4; int ref = readI32(mem, sp);
                    writeI32(mem, sp, readI32(mem, ref + 4 + slot * 4)); sp += 4;
                    break;
                }
                case 0x54: { // SET_FIELD
                    int slot = mem[pc] & 0xFF; pc++;
                    sp -= 4; int val = readI32(mem, sp);
                    sp -= 4; int ref = readI32(mem, sp);
                    writeI32(mem, ref + 4 + slot * 4, val);
                    break;
                }
                case 0x56: { // PRINT_NONL
                    sp -= 4; int v = readI32(mem, sp);
                    programOut.writeText(Integer.toString(v));
                    break;
                }
                case 0x57: { // FPRINT_NONL
                    sp -= 4; float v = Float.intBitsToFloat(readI32(mem, sp));
                    programOut.writeText(Float.toString(v));
                    break;
                }
                case 0x58: { // PRINT_STR_NONL
                    sp -= 4; int ref = readI32(mem, sp);
                    int length = readI32(mem, ref);
                    StringBuilder sb = new StringBuilder(length);
                    for (int i = 0; i < length; i++) {
                        sb.append((char) readI32(mem, ref + 4 + i * 4));
                    }
                    programOut.writeText(sb.toString());
                    break;
                }
                case 0x59: { // PRINT_NL
                    programOut.newline();
                    break;
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
                    sp -= 4; int v = readI32(mem, sp);
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
                            writeI32(mem, sp, v); sp += 4;
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
                    sp -= 4; int ref = readI32(mem, sp);
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    freeOwnedObject(ref);
                    break;
                }
                case 0x60: { // SET_FIELD_OWNER
                    int slot = mem[pc] & 0xFF; pc++;
                    sp -= 4; int val = readI32(mem, sp);
                    sp -= 4; int ref = readI32(mem, sp);
                    int slotAddr = ref + 4 + slot * 4;
                    int old = readI32(mem, slotAddr);
                    tc.pc=pc; tc.sp=sp; tc.bp=bp; tc.cs=cs;
                    freeOwnedObject(old);
                    writeI32(mem, slotAddr, val);
                    break;
                }

                case 0x5E: { // INSTANCEOF
                    short csOff = (short) readI16(mem, pc); pc += 2;
                    int expected = cs + csOff;
                    sp -= 4; int ref = readI32(mem, sp);
                    int objClass = classPtrOfRefOr0(ref);
                    boolean ok = (objClass != 0) && isDescendantOf(objClass, expected);
                    writeI32(mem, sp, ok ? 1 : 0); sp += 4;
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
                case 0x69: { // LEA_LOCAL_S8
                    int off = mem[pc]; pc++;
                    int v = bp + off;
                    mem[sp] = (byte)(v >> 24); mem[sp+1] = (byte)(v >> 16); mem[sp+2] = (byte)(v >> 8); mem[sp+3] = (byte)v;
                    sp += 4;
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
                case 0x6C: { // LEA_GLOBAL_S8
                    int off = mem[pc]; pc++;
                    int v = cs + off;
                    mem[sp] = (byte)(v >> 24); mem[sp+1] = (byte)(v >> 16); mem[sp+2] = (byte)(v >> 8); mem[sp+3] = (byte)v;
                    sp += 4;
                    break;
                }

                case 0x55: { // INVOKE_VIRTUAL
                    int vtSlot  = mem[pc]     & 0xFF;
                    int numArgs = mem[pc + 1] & 0xFF;
                    pc += 2;
                    int thisRef    = readI32(mem, sp - 4 - numArgs * 4);
                    if (thisRef == 0) {
                        tc.sp = sp;
                        throwBpRuntimeError(tc, "INVOKE_VIRTUAL sobre null receiver"
                                + " (vtSlot=" + vtSlot + ", numArgs=" + numArgs + ")");
                        break;
                    }
                    int classPtr   = readI32(mem, thisRef);
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
                sp -= 4;
                int v = readI32(mem, sp);
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
                        writeI32(mem, sp, v); sp += 4;
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
    private String readVmString(int ref) {
        int length = readInt32(ref);
        StringBuilder sb = new StringBuilder(length);
        for (int i = 0; i < length; i++) sb.append((char) readInt32(ref + 4 + i * 4));
        return sb.toString();
    }

    /** Aloca un nuevo string en el heap con el contenido de Java String. Devuelve user_ref. */
    private int allocVmString(String s) {
        int len = s.length();
        int ref = heapAlloc(len * 4, TYPE_ARRAY_I32);
        writeInt32(ref, len);
        for (int i = 0; i < len; i++) writeInt32(ref + 4 + i * 4, s.charAt(i));
        // B1 residual — el caller llama a allocVmString desde dispatchBuiltin
        // SIN vmLock, así que entre `return ref` y el `pushTc(tc, ref)` un GC
        // de otro worker podría liberar `ref` (no hay raíz aún). Anclamos
        // explícitamente en el tc actual; el GC lo agrega a roots.
        ThreadContext me = currentTcLocal.get();
        if (me != null) me.allocAnchor = ref;
        return ref;
    }

    /** Aloca un array de refs (TYPE_ARRAY_REF) con n elementos, devolviendo user_ref. Los slots quedan a 0. */
    private int allocVmRefArray(int n) {
        int ref = heapAlloc(n * 4, TYPE_ARRAY_REF);
        writeInt32(ref, n);
        for (int i = 0; i < n; i++) writeInt32(ref + 4 + i * 4, 0);
        // B1 residual — ver allocVmString. Mismo motivo: ancla GC para el
        // intervalo entre alocación y publicación al stack del programa.
        ThreadContext me = currentTcLocal.get();
        if (me != null) me.allocAnchor = ref;
        return ref;
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
    private void dispatchBuiltin(Builtin b, ThreadContext tc) {
        switch (b) {
            case STRLEN: {
                int ref = popTc(tc);
                pushTc(tc, readInt32(ref));
                break;
            }
            case PARSE_INT: {
                String s = readVmString(popTc(tc));
                pushTc(tc, (int) Long.parseLong(s.trim()));
                break;
            }
            case PARSE_FLOAT: {
                String s = readVmString(popTc(tc));
                pushTc(tc, Float.floatToRawIntBits((float) Double.parseDouble(s.trim())));
                break;
            }
            case INT_TO_STRING: {
                int n = popTc(tc);
                pushTc(tc, allocVmString(Integer.toString(n)));
                break;
            }
            case FLOAT_TO_STRING: {
                float x = Float.intBitsToFloat(popTc(tc));
                pushTc(tc, allocVmString(Float.toString(x)));
                break;
            }
            case BOOL_TO_STRING: {
                int v = popTc(tc);
                pushTc(tc, allocVmString(v != 0 ? "true" : "false"));
                break;
            }
            case UPPER: { String s = readVmString(popTc(tc)); pushTc(tc, allocVmString(s.toUpperCase())); break; }
            case LOWER: { String s = readVmString(popTc(tc)); pushTc(tc, allocVmString(s.toLowerCase())); break; }
            case TRIM:  { String s = readVmString(popTc(tc)); pushTc(tc, allocVmString(s.trim()));        break; }

            case SUBSTRING: {
                int end = popTc(tc); int start = popTc(tc);
                String s = readVmString(popTc(tc));
                int n = s.length();
                int from = Math.max(0, Math.min(n, start));
                int to   = Math.max(from, Math.min(n, end));
                pushTc(tc, allocVmString(s.substring(from, to)));
                break;
            }
            case INDEX_OF: {
                String target = readVmString(popTc(tc));
                String s = readVmString(popTc(tc));
                pushTc(tc, s.indexOf(target));
                break;
            }
            case STARTS_WITH: {
                String pre = readVmString(popTc(tc));
                String s = readVmString(popTc(tc));
                pushTc(tc, s.startsWith(pre) ? 1 : 0);
                break;
            }
            case ENDS_WITH: {
                String suf = readVmString(popTc(tc));
                String s = readVmString(popTc(tc));
                pushTc(tc, s.endsWith(suf) ? 1 : 0);
                break;
            }
            case CONTAINS: {
                String sub = readVmString(popTc(tc));
                String s = readVmString(popTc(tc));
                pushTc(tc, s.contains(sub) ? 1 : 0);
                break;
            }
            case CHAR_AT: {
                int i = popTc(tc);
                String s = readVmString(popTc(tc));
                if (i < 0 || i >= s.length()) {
                    throwBpRuntimeError(tc, "charAt: idx fuera de rango " + i + " (len=" + s.length() + ")");
                }
                pushTc(tc, allocVmString(String.valueOf(s.charAt(i))));
                break;
            }
            case REPLACE: {
                String rep = readVmString(popTc(tc));
                String tgt = readVmString(popTc(tc));
                String s = readVmString(popTc(tc));
                pushTc(tc, allocVmString(s.replace(tgt, rep)));
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

            case RANDOM:     { pushTc(tc, Float.floatToRawIntBits((float) Math.random())); break; }
            case RANDOM_INT: {
                int hi = popTc(tc); int lo = popTc(tc);
                int r = lo + (int) (Math.random() * (hi - lo + 1));
                pushTc(tc, r);
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
                String sep = readVmString(popTc(tc));
                String s   = readVmString(popTc(tc));
                // String.split con regex literal: pasamos java.util.regex.Pattern.quote.
                String[] parts = s.split(java.util.regex.Pattern.quote(sep), -1);
                // Aloca primero los strings individuales, luego el array (en ese orden el
                // GC tendrá los slots zero-init mientras se llenan).
                int[] refs = new int[parts.length];
                for (int i = 0; i < parts.length; i++) refs[i] = allocVmString(parts[i]);
                int arrRef = allocVmRefArray(parts.length);
                for (int i = 0; i < parts.length; i++) writeInt32(arrRef + 4 + i * 4, refs[i]);
                pushTc(tc, arrRef);
                break;
            }

            case INPUT: {
                if (stdinReader == null) {
                    stdinReader = new java.io.BufferedReader(new java.io.InputStreamReader(System.in));
                }
                try {
                    String line = stdinReader.readLine();
                    pushTc(tc, allocVmString(line != null ? line : ""));
                } catch (java.io.IOException e) {
                    throw new RuntimeException("input(): " + e.getMessage());
                }
                break;
            }

            case READ_FILE: {
                String path = readVmString(popTc(tc));
                try {
                    byte[] data = java.nio.file.Files.readAllBytes(sandboxPath(tc, path));
                    pushTc(tc, allocVmString(new String(data, java.nio.charset.StandardCharsets.UTF_8)));
                } catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "readFile('" + path + "'): " + e.getMessage());
                }
                break;
            }
            case WRITE_FILE: {
                String content = readVmString(popTc(tc));
                String path    = readVmString(popTc(tc));
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
                String content = readVmString(popTc(tc));
                String path    = readVmString(popTc(tc));
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
                String path = readVmString(popTc(tc));
                pushTc(tc, java.nio.file.Files.exists(sandboxPath(tc, path)) ? 1 : 0);
                break;
            }
            case GC: {
                gc();
                pushTc(tc, 0);  // void → dummy
                break;
            }
            case LIST_DIR: {
                String path = readVmString(popTc(tc));
                java.io.File dir = sandboxPath(tc, path).toFile();
                String[] names = dir.list();
                if (names == null) names = new String[0];
                int[] refs = new int[names.length];
                for (int i = 0; i < names.length; i++) refs[i] = allocVmString(names[i]);
                int arrRef = allocVmRefArray(names.length);
                for (int i = 0; i < names.length; i++) writeInt32(arrRef + 4 + i * 4, refs[i]);
                pushTc(tc, arrRef);
                break;
            }

            // ---- Soporte para List / StringBuilder ----
            case NEW_REF_ARRAY: {
                int cap = popTc(tc);
                if (cap < 0) throwBpRuntimeError(tc, "__newRefArray: capacidad negativa: " + cap);
                pushTc(tc, allocVmRefArray(cap));
                break;
            }
            case GROW_REF_ARRAY: {
                int newCap = popTc(tc);
                int oldRef = popTc(tc);
                if (newCap < 0) throwBpRuntimeError(tc, "__growRefArray: capacidad negativa: " + newCap);
                int oldLen = (oldRef != 0) ? readInt32(oldRef) : 0;
                int newRef = allocVmRefArray(newCap);
                int copyLen = Math.min(oldLen, newCap);
                for (int i = 0; i < copyLen; i++) {
                    writeInt32(newRef + 4 + i * 4, readInt32(oldRef + 4 + i * 4));
                }
                pushTc(tc, newRef);
                break;
            }
            case GROW_INT_ARRAY: {
                int newCap = popTc(tc);
                int oldRef = popTc(tc);
                if (newCap < 0) throwBpRuntimeError(tc, "__growIntArray: capacidad negativa: " + newCap);
                int oldLen = (oldRef != 0) ? readInt32(oldRef) : 0;
                int newRef = heapAlloc(newCap * 4, TYPE_ARRAY_I32);
                writeInt32(newRef, newCap);
                int copyLen = Math.min(oldLen, newCap);
                for (int i = 0; i < copyLen; i++) {
                    writeInt32(newRef + 4 + i * 4, readInt32(oldRef + 4 + i * 4));
                }
                for (int i = copyLen; i < newCap; i++) {
                    writeInt32(newRef + 4 + i * 4, 0);
                }
                pushTc(tc, newRef);
                break;
            }
            case CHARS_TO_STRING: {
                int len = popTc(tc);
                int charsRef = popTc(tc);
                if (len < 0) throwBpRuntimeError(tc, "__charsToString: longitud negativa: " + len);
                int avail = (charsRef != 0) ? readInt32(charsRef) : 0;
                if (len > avail) throwBpRuntimeError(tc, "__charsToString: longitud " + len + " > capacidad " + avail);
                StringBuilder sb = new StringBuilder(len);
                for (int i = 0; i < len; i++) sb.append((char) readInt32(charsRef + 4 + i * 4));
                pushTc(tc, allocVmString(sb.toString()));
                break;
            }
            case CHAR_CODE_AT: {
                int idx = popTc(tc);
                int sRef = popTc(tc);
                int len = readInt32(sRef);
                if (idx < 0 || idx >= len) throwBpRuntimeError(tc, "charCodeAt: índice " + idx + " fuera de [0," + len + ")");
                pushTc(tc, readInt32(sRef + 4 + idx * 4));
                break;
            }

            // ---- Threading ----
            // Convención de fields en la clase Thread sintetizada:
            //   slot 0 = __tid         (id del ThreadContext, 0 = no spawneado todavía)
            //   slot 1 = __stackSize   (bytes; 0 = default)
            // Convención de vtable: slot 0 = run() (virtual).
            case THREAD_START: {
                int threadRef = popTc(tc);
                int classPtr  = readInt32(threadRef);
                int stackSize = readInt32(threadRef + 4 + 1 * 4);   // field 1
                synchronized (vmLock) {
                    // 1) Crear el nuevo ThreadContext con su región de stack.
                    int newTid = spawnThread(stackSize);
                    writeInt32(threadRef + 4 + 0 * 4, newTid);          // guardar tid en field 0
                    ThreadContext nt = threads.get(newTid);
                    // 2) Resolver dirección absoluta del run() en la vtable.
                    int targetCS   = moduleManager.getCSForDataAddr(classPtr);
                    int bitmapW    = readInt16(classPtr + CLS_OFF_BITMAP_WORDS) & 0xFFFF;
                    int vtableBase = classPtr + CLS_OFF_FIELD_BITMAP + 2 * bitmapW * 4;
                    int methodOff  = readInt32(vtableBase + 0 * 4);     // slot 0 = run()
                    int runPc      = targetCS + methodOff;
                    // 3) Preparar el frame inicial en el stack del nuevo thread:
                    //    [sb+0]  thisRef
                    //    [sb+4]  saved PC = 0  (sentinela: memory[0] = THREAD_EXIT)
                    //    [sb+8]  saved BP = sb
                    //    [sb+12] saved CS = 0
                    //    bp = sb + 16; sp = sb + 16
                    int sb = nt.stackBase;
                    writeInt32(sb,      threadRef);
                    writeInt32(sb + 4,  0);
                    writeInt32(sb + 8,  sb);
                    writeInt32(sb + 12, 0);
                    nt.sp = sb + 16;
                    nt.bp = sb + 16;
                    nt.pc = runPc;
                    nt.cs = targetCS;
                    // 4) Notificamos a workers durmiendo en pickNextRunnableTc.
                    vmLock.notifyAll();
                }
                pushTc(tc, 0);   // dummy ret
                break;
            }
            case THREAD_JOIN: {
                int threadRef = popTc(tc);
                int targetTid = readInt32(threadRef + 4 + 0 * 4);
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
                int mid = readInt32(mutexRef + 4 + 0 * 4);
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
                int mid = readInt32(mutexRef + 4 + 0 * 4);
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
                int dstRef   = popTc(tc);
                int srcRef   = popTc(tc);
                if (srcRef <= 0)
                    throwBpRuntimeError(tc, "move: src es null");
                if (dstRef <= 0)
                    throwBpRuntimeError(tc, "move: dst es null");
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
            case PATH_JOIN: {
                String b2 = readVmString(popTc(tc));
                String a  = readVmString(popTc(tc));
                String r = java.nio.file.Paths.get(a, b2).toString();
                pushTc(tc, allocVmString(r));
                break;
            }
            case PATH_PARENT: {
                String p = readVmString(popTc(tc));
                java.nio.file.Path pa = java.nio.file.Paths.get(p).getParent();
                pushTc(tc, allocVmString(pa == null ? "" : pa.toString()));
                break;
            }
            case PATH_BASENAME: {
                String p = readVmString(popTc(tc));
                java.nio.file.Path pa = java.nio.file.Paths.get(p).getFileName();
                pushTc(tc, allocVmString(pa == null ? "" : pa.toString()));
                break;
            }
            case PATH_EXTENSION: {
                String p = readVmString(popTc(tc));
                java.nio.file.Path pa = java.nio.file.Paths.get(p).getFileName();
                String name = (pa == null) ? "" : pa.toString();
                int dot = name.lastIndexOf('.');
                String ext = (dot <= 0 || dot == name.length() - 1) ? "" : name.substring(dot + 1);
                pushTc(tc, allocVmString(ext));
                break;
            }
            case PATH_ABSOLUTE: {
                String p = readVmString(popTc(tc));
                // Con sandbox: devuelve el path absoluto DENTRO del workdir
                // (no filtra info del host). Sin sandbox: usa Paths.get raw.
                java.nio.file.Path resolved = sandboxPath(tc, p);
                String r = resolved.toAbsolutePath().normalize().toString();
                pushTc(tc, allocVmString(r));
                break;
            }
            case MKDIR: {
                String p = readVmString(popTc(tc));
                try { java.nio.file.Files.createDirectories(sandboxPath(tc, p)); }
                catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "mkdir('" + p + "'): " + e.getMessage());
                }
                pushTc(tc, 0);   // dummy void
                break;
            }
            case RMDIR: {
                String p = readVmString(popTc(tc));
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
                String p = readVmString(popTc(tc));
                try { java.nio.file.Files.delete(sandboxPath(tc, p)); }
                catch (java.io.IOException e) {
                    throwBpRuntimeError(tc, "removeFile('" + p + "'): " + e.getMessage());
                }
                pushTc(tc, 0);
                break;
            }
            case RENAME: {
                String to   = readVmString(popTc(tc));
                String from = readVmString(popTc(tc));
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
                String to   = readVmString(popTc(tc));
                String from = readVmString(popTc(tc));
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
                String p = readVmString(popTc(tc));
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
                String p = readVmString(popTc(tc));
                pushTc(tc, java.nio.file.Files.isDirectory(sandboxPath(tc, p)) ? 1 : 0);
                break;
            }
            case LAST_MODIFIED: {
                String p = readVmString(popTc(tc));
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
                pushTc(tc, ref);
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
                pushTc(tc, allocVmString("host-pc"));
                break;
            }
            case PICO_BOARD_NAME: {
                pushTc(tc, allocVmString("host"));
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
    private int classPtrOfRefOr0(int ref) {
        if (ref <= 0) return 0;
        int headerAddr = ref - 4;
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
    private void freeOwnedObject(int ref) {
        synchronized (vmLock) { freeOwnedObjectLocked(ref); }
    }

    /** Implementación de freeOwnedObject que asume vmLock ya adquirido (para llamadas recursivas). */
    private void freeOwnedObjectLocked(int ref) {
        if (ref == 0) return;
        int headerAddr = ref - 4;
        if (headerAddr < heapStart || headerAddr >= heapNext) return;
        int tag = readInt32(headerAddr);
        if ((tag & TAG_FREE_BIT) != 0) return;       // ya libre
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
                    int childRef = readInt32(headerAddr + OBJ_HEADER_SIZE + i * 4);
                    freeOwnedObjectLocked(childRef);
                }
            }
        } else if (type == TYPE_ARRAY_REF) {
            // Cascada propietaria: si llegamos aquí es porque alguien (un campo
            // owner del objeto contenedor) considera que ESTE array de refs y
            // su contenido le pertenecen. Liberamos cada slot non-null
            // recursivamente y luego el array como bloque.
            int length = readInt32(ref);
            for (int i = 0; i < length; i++) {
                int slotRef = readInt32(ref + 4 + i * 4);
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
     * Intenta leer un BP string en `ref` (= user_ref a un array de chars con
     * length en bytes 0..3). Devuelve la representación Java o "@addr" si
     * no se puede interpretar. Sin side-effects: no aloca, no muta memoria.
     */
    public String readStringIfPossible(int ref) {
        try {
            if (ref <= 0 || ref + 4 > memory.length) return "@" + Integer.toHexString(ref);
            int len = readInt32(ref);
            if (len < 0 || len > 65536) return "@" + Integer.toHexString(ref);  // sanity
            int end = ref + 4 + len * 4;
            if (end > memory.length) return "@" + Integer.toHexString(ref);
            StringBuilder sb = new StringBuilder(len + 2);
            sb.append('"');
            for (int i = 0; i < len; i++) {
                int cp = readInt32(ref + 4 + i * 4);
                if (cp < 0 || cp > 0x10FFFF) { sb.append('?'); continue; }
                if (cp == '"') sb.append("\\\"");
                else if (cp == '\\') sb.append("\\\\");
                else if (cp == '\n') sb.append("\\n");
                else if (cp == '\r') sb.append("\\r");
                else if (cp == '\t') sb.append("\\t");
                else if (cp >= 0x20 && cp < 0x7F) sb.append((char) cp);
                else if (cp <= 0xFFFF) sb.append((char) cp);
                else sb.append(new String(Character.toChars(cp)));
            }
            sb.append('"');
            return sb.toString();
        } catch (Throwable t) {
            return "@" + Integer.toHexString(ref);
        }
    }
}
