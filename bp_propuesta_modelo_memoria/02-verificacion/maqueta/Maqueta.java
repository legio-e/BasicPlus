/*
 * Maqueta de validación del modelo de memoria de BasicPlus (spike, foco 02).
 *
 * NO es la VM real ni código de producción. Es un modelo mínimo para validar
 * la LÓGICA del modelo propuesto en
 *   03-arquitectura/heap/objetivo-modelo-memoria-smp.md
 * bajo paralelismo real (hilos de SO), en la VM-Java (la de referencia).
 *
 * Qué valida (reproducible en x86, son carreras LÓGICAS):
 *   - Tabla de handles con contador de GENERACIÓN.
 *   - Contrato B: usar una referencia a un objeto liberado → "objeto eliminado",
 *     nunca leer el objeto equivocado (use-after-free silencioso).
 *   - Reclamación DIFERIDA a un safepoint (stop-the-world): el slot no se reutiliza
 *     mientras los mutadores corren.
 *   - Publicación segura: la generación se escribe/lee con semántica volatile
 *     (AtomicIntegerArray) → modela el apretón de manos release/acquire de A1.
 *
 * Qué NO valida aquí (es la fase de hardware, ver plan-validacion-maqueta.md):
 *   - Las barreras de publicación sobre ARM (x86 tiene modelo de memoria fuerte y
 *     ESCONDE las reordenaciones de store que rompen la publicación en ARM).
 *
 * Compara dos modos corriendo el MISMO test concurrente:
 *   BROKEN = como el modelo viejo (handle = índice, free reutiliza el slot ya, sin
 *            chequeo)  →  se espera CORRUPCIÓN.
 *   MODEL  = el modelo propuesto (generación + reclamación diferida)  →  se espera
 *            0 corrupciones y detección limpia de los accesos colgantes.
 */
import java.util.concurrent.*;
import java.util.concurrent.atomic.*;
import java.util.concurrent.locks.*;

public class Maqueta {

    // -------- Parámetros --------
    static final int SLOTS       = 64;      // heap pequeño → reutilización frecuente
    static final int FIELDS      = 6;       // campos por objeto (todos = value)
    static final int PUBLISHED   = 512;     // ranuras de referencias compartidas
    static int THREADS     = 4;       // configurable por args[0]
    static long RUN_MS      = 3000;   // configurable por args[1] (ms)

    // Excepción que modela el contrato B ("referencia a objeto eliminado").
    static final class ObjetoEliminado extends RuntimeException {
        ObjetoEliminado() { super("referencia a objeto eliminado"); }
    }

    // -------- El heap-maqueta --------
    static final class Heap {
        final boolean useGeneration;   // MODEL=true, BROKEN=false
        final boolean deferReclaim;    // MODEL=true, BROKEN=false

        // payload[slot] = FIELDS ints (todos = value del objeto vivo ahí).
        final int[][] payload = new int[SLOTS][FIELDS];
        // generación por slot (semántica volatile por elemento = punto de publicación).
        final AtomicIntegerArray gen = new AtomicIntegerArray(SLOTS);
        // free-list de slots realmente disponibles.
        final ConcurrentLinkedQueue<Integer> freeList = new ConcurrentLinkedQueue<Integer>();
        // slots liberados pendientes de reclamar en el safepoint (MODEL).
        final ConcurrentLinkedQueue<Integer> pending = new ConcurrentLinkedQueue<Integer>();
        // read lock = mutadores en paralelo ; write lock = safepoint stop-the-world.
        final ReentrantReadWriteLock stw = new ReentrantReadWriteLock();

        Heap(boolean useGeneration, boolean deferReclaim) {
            this.useGeneration = useGeneration;
            this.deferReclaim  = deferReclaim;
            for (int i = 0; i < SLOTS; i++) freeList.add(i);
        }

        /** Aloca un objeto con 'value'. Devuelve handle (slot,gen) o null si no cabe. */
        long alloc(int value) {
            stw.readLock().lock();
            try {
                Integer slot = freeList.poll();
                if (slot == null) return -1L;              // heap lleno este instante
                int s = slot.intValue();
                // 1) inicializar el payload ANTES de publicar (lado escritor).
                for (int f = 0; f < FIELDS; f++) payload[s][f] = value;
                // 2) publicar: escribir la generación (volatile) = release.
                int g = gen.get(s);
                return handle(s, g);
            } finally {
                stw.readLock().unlock();
            }
        }

        /** Lee y valida el objeto. Devuelve su value, o lanza ObjetoEliminado. */
        int read(long h, int expected) {
            int s = slotOf(h), g = genOf(h);
            stw.readLock().lock();
            try {
                if (useGeneration) {
                    // acquire: leer la generación (volatile) antes del payload.
                    if (gen.get(s) != g) throw new ObjetoEliminado();
                }
                // leer el payload y comprobar consistencia interna (no medio construido)
                int v0 = payload[s][0];
                for (int f = 1; f < FIELDS; f++) {
                    if (payload[s][f] != v0) {
                        // objeto inconsistente (medio construido / pisado)
                        throw new Corrupcion("campos inconsistentes en slot " + s);
                    }
                }
                if (useGeneration && gen.get(s) != g) throw new ObjetoEliminado();
                return v0;
            } finally {
                stw.readLock().unlock();
            }
        }

        /** Libera (owner). En MODEL la liberación es GENERATION-CHECKED: solo la
         *  primera liberación de un handle vivo tiene efecto; una liberación stale
         *  o doble es un no-op seguro (si no, bumpearía la generación del objeto
         *  vivo que ahora ocupa el slot y provocaría doble-free → corrupción). */
        void free(long h) {
            int s = slotOf(h), g = genOf(h);
            stw.readLock().lock();
            try {
                if (useGeneration) {
                    // CAS: solo libera si el handle sigue siendo el vivo (gen coincide).
                    if (!gen.compareAndSet(s, g, g + 1)) return;   // stale/doble → no-op
                    pending.add(Integer.valueOf(s));               // reclamación diferida
                } else {
                    freeList.add(Integer.valueOf(s));              // BROKEN: reusable YA
                }
            } finally {
                stw.readLock().unlock();
            }
        }

        /** Safepoint stop-the-world: reclama los slots pendientes (MODEL). */
        void safepoint() {
            if (!deferReclaim) return;
            stw.writeLock().lock();   // para el mundo: ningún mutador dentro
            try {
                Integer s;
                while ((s = pending.poll()) != null) freeList.add(s);
            } finally {
                stw.writeLock().unlock();
            }
        }

        static long handle(int slot, int gen) { return ((long) gen << 32) | (slot & 0xffffffffL); }
        static int  slotOf(long h) { return (int) (h & 0xffffffffL); }
        static int  genOf(long h)  { return (int) (h >>> 32); }
    }

    static final class Corrupcion extends RuntimeException {
        Corrupcion(String m) { super(m); }
    }

    // Una referencia publicada: handle + el value que esperamos leer.
    static final class Pub {
        final long handle; final int expected;
        Pub(long h, int e) { handle = h; expected = e; }
    }

    // -------- El test concurrente (idéntico para los dos modos) --------
    static final class Result {
        final AtomicLong reads = new AtomicLong();
        final AtomicLong liveOk = new AtomicLong();         // leyó un objeto vivo y CORRECTO
        final AtomicLong corruptions = new AtomicLong();   // leyó el objeto EQUIVOCADO
        final AtomicLong cleanDangling = new AtomicLong();  // "objeto eliminado" (bien)
        final AtomicLong allocs = new AtomicLong();
        final AtomicLong frees = new AtomicLong();
    }

    static Result run(Heap heap) throws InterruptedException {
        final Result r = new Result();
        final AtomicReferenceArray<Pub> published = new AtomicReferenceArray<Pub>(PUBLISHED);
        final AtomicInteger valueSeq = new AtomicInteger(1);
        final long deadline = System.currentTimeMillis() + RUN_MS;
        final CountDownLatch done = new CountDownLatch(THREADS);

        // Coordinador de safepoints: periódicamente para el mundo y reclama.
        final Thread coordinator = new Thread(new Runnable() {
            public void run() {
                while (System.currentTimeMillis() < deadline) {
                    heap.safepoint();
                    try { Thread.sleep(1); } catch (InterruptedException e) { return; }
                }
            }
        });
        coordinator.setDaemon(true);
        coordinator.start();

        for (int t = 0; t < THREADS; t++) {
            new Thread(new Runnable() {
                public void run() {
                    ThreadLocalRandom rnd = ThreadLocalRandom.current();
                    // Objetos vivos de ESTE hilo (semántica owner: el dueño libera los suyos).
                    java.util.ArrayDeque<Pub> mine = new java.util.ArrayDeque<Pub>();
                    try {
                        while (System.currentTimeMillis() < deadline) {
                            int dice = rnd.nextInt(100);
                            if (dice < 35) {
                                // ALLOC: crear, guardarlo como propio, y publicarlo (visible a todos)
                                int v = valueSeq.getAndIncrement();
                                long h = heap.alloc(v);
                                if (h != -1L) {
                                    r.allocs.incrementAndGet();
                                    Pub p = new Pub(h, v);
                                    mine.addLast(p);
                                    published.set(rnd.nextInt(PUBLISHED), p);
                                }
                            } else if (dice < 85) {
                                // READ una referencia publicada al azar (viva o colgante)
                                Pub p = published.get(rnd.nextInt(PUBLISHED));
                                if (p != null) {
                                    r.reads.incrementAndGet();
                                    try {
                                        int got = heap.read(p.handle, p.expected);
                                        if (got == p.expected) r.liveOk.incrementAndGet();
                                        else r.corruptions.incrementAndGet();
                                    } catch (ObjetoEliminado e) {
                                        r.cleanDangling.incrementAndGet();  // detectado, bien
                                    } catch (Corrupcion c) {
                                        r.corruptions.incrementAndGet();     // objeto inconsistente
                                    }
                                }
                            } else {
                                // FREE: el dueño libera UNO de los suyos. La Pub sigue publicada
                                // a propósito → se convierte en referencia colgante para otros.
                                Pub p = mine.pollFirst();
                                if (p != null) {
                                    heap.free(p.handle);
                                    r.frees.incrementAndGet();
                                }
                            }
                        }
                    } finally {
                        done.countDown();
                    }
                }
            }).start();
        }
        done.await();
        return r;
    }

    public static void main(String[] args) throws Exception {
        if (args.length > 0) THREADS = Integer.parseInt(args[0]);
        if (args.length > 1) RUN_MS  = Long.parseLong(args[1]);
        System.out.println("Maqueta modelo de memoria BasicPlus — " + THREADS
                + " hilos, heap " + SLOTS + " slots, " + RUN_MS + " ms/modo\n");

        System.out.println(">>> Modo BROKEN (modelo viejo: sin generación, reuso inmediato)");
        Result broken = run(new Heap(false, false));
        report("BROKEN", broken);

        System.out.println("\n>>> Modo MODEL (propuesto: generación + reclamación diferida)");
        Result model = run(new Heap(true, true));
        report("MODEL", model);

        System.out.println("\n================ VEREDICTO ================");
        System.out.printf("BROKEN corrupciones: %d  (leyó el objeto equivocado / inconsistente)%n",
                broken.corruptions.get());
        System.out.printf("MODEL  corrupciones: %d%n", model.corruptions.get());
        System.out.printf("MODEL  colgantes detectados limpiamente ('objeto eliminado'): %d%n",
                model.cleanDangling.get());
        boolean pass = broken.corruptions.get() > 0 && model.corruptions.get() == 0;
        System.out.println(pass
                ? "\nRESULTADO: PASA — el modelo elimina la corrupción que el viejo sí sufre."
                : "\nRESULTADO: NO CONCLUYENTE — revisar (¿reprodujo BROKEN la corrupción?).");
    }

    static void report(String mode, Result r) {
        System.out.printf("  %-6s allocs=%d frees=%d reads=%d | vivas-OK=%d corrupciones=%d colgantes-limpios=%d%n",
                mode, r.allocs.get(), r.frees.get(), r.reads.get(),
                r.liveOk.get(), r.corruptions.get(), r.cleanDangling.get());
    }
}
