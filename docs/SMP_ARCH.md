# BasicPlus VM — arquitectura SMP (H2)

Diseño del scheduler multi-worker para correr 2+ threads BP en
paralelo. Base teórica de #179, #156, #153.

## 1. Punto de partida

**Hoy (F4 v1)** la VM-C corre TODOS los threads BP en un único worker
(el thread main del proceso). El scheduler hace round-robin
cooperativo: pickea un `tc` RUNNABLE, ejecuta un quantum, lo guarda,
pickea el siguiente. Cero paralelismo.

Una `bpvm_t` mantiene:

- `memory[]`: heap + stacks + data + code, todo plano.
- `threads[]`: array de `bpvm_thread_t` (PCB de cada BP-thread).
- `symbols[]`, `modules[]`: read-only tras `bpvm_link_all`.
- `mutexes[]`: pool de mutex BP user-level.
- `heap_next`, `heap_start`: bump-allocator + mark-sweep.

**Limitación**: con RP2350 dual-core sólo usamos el core0. Un workload
multi-thread (productor/consumidor, paralelismo de datos, etc.) no
escala más allá de 1×.

## 2. Punto de llegada

**Target (H2)**: N workers + 1 comm task. Cada worker ejecuta el
interp loop sobre un `tc` RUNNABLE. La comm task atiende I/O externo
(USB CDC en Pico, stdout en host). Workers nunca tocan la salida
directamente — encolan a una **output queue** que la comm task drena.

```
        Worker pool (CPU-bound)
┌────────┐ ┌────────┐ ┌────────┐
│ Worker │ │ Worker │ │ Worker │   ← N pthreads (host) o N FreeRTOS
│ 0      │ │ 1      │ │ ...    │     tasks pinned (Pico, N=2)
│ (loop) │ │ (loop) │ │ (loop) │
└────┬───┘ └───┬────┘ └────┬───┘
     │         │           │ print()
     │         │           │
     ↓         ↓           ↓
┌─────────────────────────────┐
│ Output queue                │   ← thread-safe ring buffer
│ (mutex + cond)              │
└──────────────┬──────────────┘
               │ drain
               ↓
        ┌──────────────────┐
        │ Comm task        │   ← host: pthread, vuelca stdout
        │ (1 hilo dedicado)│      Pico: FreeRTOS task pinned core 0,
        │                  │            CDC TX exclusivo, también
        │                  │            atiende RX + wire v1 parser
        └──────────────────┘

────────── coordinación de scheduler / heap / status ──────────

         ┌─────────────────────┐
         │ vm_lock (UN solo)   │   ← scheduler + heap + spawn + status
         │ + sched_cond        │   ← signal cuando hay tc RUNNABLE nuevo
         └─────────────────────┘
              (workers + comm task agarran cuando mutan estado compartido)
```

**Speedup esperado**: ×1.5-2 en workloads CPU-bound multi-thread.

**Decisión "do it right on PC first"**: la arquitectura completa
(workers + comm task + queue + locks + STW GC) se implementa y valida
en host VM-C. La comm task del host es trivial (drena la queue a
stdout). La comm task del Pico hereda toda esa coordinación y solo
añade la parte transport-específica (CDC TX/RX + wire v1 + dispatch).
Esto reduce el port a Pico a "swap del fichero de comm".

## 3. Spec ejecutable: VM-Java

**La VM-Java YA implementa este modelo** desde la fase 4b (multi-
worker) + 4c (STW GC). Ver `miVM/.../VirtualMachine.java` líneas
194-260 (comentario del `vmLock`) y `WorkerLoop` líneas 1363+.

**Consecuencia operativa**: NO inventamos arquitectura. Portamos el
modelo VM-Java a pthread (host) y luego a FreeRTOS SMP (Pico). Para
cualquier duda de semántica, la VM-Java es la spec.

## 4. Decisiones arquitectónicas

| # | Decisión | Razón |
|---|---|---|
| 1 | **N workers configurable**. Default 2 (= cores RP2350). | Mismo número en host que en Pico facilita la portabilidad de tests. |
| 2 | **UN solo `vm_lock` global**. | La VM-Java empezó con fine-grained, terminó en uno solo. La granularidad fina añadió bugs sin ganar throughput porque el interp loop YA corre fuera del lock. |
| 3 | **Interp loop LOCK-FREE**. El worker toma `vm_lock` para decidir el siguiente `tc`, lo libera, ejecuta el quantum sobre memoria compartida sin lock. Re-toma `vm_lock` solo si necesita `heap_alloc`, `gc`, `thread_spawn`, mutex BP, o ajustar `tc.status`. | Sin esto no hay speedup: si el interp tomara el lock por opcode, todo serializaría. |
| 4 | **STW GC con safepoint volatile**. Una flag `vm->stop_the_world` (volatile bool). Cuando un worker quiere GC, la pone a true, espera a que todos los demás workers se "parquen". Workers chequean la flag entre opcodes (al inicio de cada iteración del switch), sincronizan `tc.X` y ceden. Tras GC: flag a false + `cond_broadcast`. | Resuelve la race "GC escanea tc.sp obsoleto" sin tomar lock por opcode. Mismo patrón B1/4c del VM-Java. |
| 5 | **Worker idle = cond_wait, NO busy-wait**. Cuando no hay tc RUNNABLE: `pthread_cond_timedwait(sched_cond, vm_lock, dt)`. Quien despierte un tc (sleep expirado, mutex_unlock, join completado) hace `cond_signal`/`cond_broadcast`. | Eficiencia energética + libera CPU para el otro worker. |
| 6 | **Affinity**. Host: sin pinning (el OS decide). Pico: worker[i] pinned a core[i]. Abstraído en `bpvm_platform_thread_create_pinned(core_id)`. | Sin afinidad en Pico, el scheduler de FreeRTOS puede migrar tasks entre cores → pérdida de cache + comportamiento no determinista en medidas. |

## 5. Locks y qué protegen

**Bajo `vm_lock`** (lo agarra el helper, no el caller del interp):

| Estado | Helpers que mutan |
|---|---|
| `threads[].status` | scheduler (pick), spawn, terminate, sleep, mutex_lock/unlock, join |
| `thread_count`, `next_thread_stack` | thread_spawn |
| `heap_next`, free-list | heap_alloc, gc |
| `mutexes[]`, `mutex_count` | mutex_alloc, mutex_add_waiter, mutex_pop_waiter |
| `symbol_count`, `symbols[]` | link_register_symbol (solo en load — no en run) |
| `current_thread_idx` | scheduler (informativo) |
| `output_cb` invocations | OPCIONAL: el caller del print toma vm_lock para no entrelazar chars |

**Read-only tras link** (sin lock):

- `memory[]` regiones data y code de los módulos.
- `modules[]` después de `bpvm_link_all`.
- `aot_helpers`, `aot_registry`.

**Per-thread, no compartido** (sin lock):

- `tc->pc/sp/bp/cs`: SOLO mutados por el worker que está ejecutando ese tc.
- Stack del tc en `memory[stack_base..stack_top)`: mismo.
- Eh stack del tc.

**Caveat**: el GC necesita leer `tc->sp` de TODOS los threads → de ahí la safepoint. Tras safepoint, todos los workers han sincronizado su sp local a `tc->sp`, así GC puede leer sin race.

## 6. Pseudocódigo del worker

```c
void worker_loop(bpvm_t* vm, int worker_id) {
    for (;;) {
        bpvm_thread_t* tc = NULL;
        pthread_mutex_lock(&vm->vm_lock);
        for (;;) {
            if (vm->shutdown) {
                pthread_mutex_unlock(&vm->vm_lock);
                return;
            }
            if (vm->stop_the_world) {
                pthread_cond_wait(&vm->sched_cond, &vm->vm_lock);
                continue;
            }
            tc = pick_next_runnable(vm);
            if (tc) break;
            if (!any_thread_alive(vm)) {
                vm->shutdown = true;
                pthread_cond_broadcast(&vm->sched_cond);
                pthread_mutex_unlock(&vm->vm_lock);
                return;
            }
            int64_t dt = earliest_sleep_wake_ms(vm) - now_ms();
            if (dt > 0) cond_timedwait(&vm->sched_cond, &vm->vm_lock, dt);
            else        pthread_cond_wait(&vm->sched_cond, &vm->vm_lock);
        }
        tc->status = BPVM_THREAD_RUNNING;
        pthread_mutex_unlock(&vm->vm_lock);

        /* ----- Interp loop LOCK-FREE sobre memoria compartida ----- */
        int yielded = 0;
        bpvm_status_t s = bpvm_interp_run_quantum(vm, tc, vm->quantum_ops, &yielded);
        /* dentro de run_quantum, ANTES del fetch de cada opcode:
         *   if (vm->stop_the_world) { sync tc.X; return YIELD; }
         */

        pthread_mutex_lock(&vm->vm_lock);
        /* Handle del status devuelto: yield, sleep, blocked, terminated.
         * Si se ha despertado algún sleep/mutex/join: cond_signal. */
        ...
        pthread_mutex_unlock(&vm->vm_lock);
    }
}
```

## 7. STW GC dance

```c
void heap_alloc_with_gc(bpvm_t* vm, ...) {
    pthread_mutex_lock(&vm->vm_lock);
    if (heap_full(vm)) {
        if (vm->gc_in_progress) {
            /* Otro worker ya orquesta GC. Espera. */
            while (vm->gc_in_progress) pthread_cond_wait(&vm->sched_cond, &vm->vm_lock);
        } else {
            vm->gc_in_progress = true;
            vm->stop_the_world = true;
            pthread_cond_broadcast(&vm->sched_cond);
            /* Espera a que todos los otros workers cedan */
            while (any_other_worker_running(vm)) pthread_cond_wait(&vm->sched_cond, &vm->vm_lock);
            bpvm_gc(vm);   /* ya tenemos vm_lock + mundo parado */
            vm->stop_the_world = false;
            vm->gc_in_progress = false;
            pthread_cond_broadcast(&vm->sched_cond);
        }
    }
    /* Aloca. */
    ...
    pthread_mutex_unlock(&vm->vm_lock);
}
```

## 8. Plan de validación en 3 capas

```
1) VM-Java --workers=2          ← spec ejecutable, ya existe
        ↓ portar arquitectura
2) Host VM-C + pthread + TSan   ← caza races con ThreadSanitizer
        ↓ swap capa platform
3) Pico VM-C + FreeRTOS SMP     ← target final, validación HW
```

**Spec test cruzado**: cualquier sample BP multi-thread debe producir
el MISMO output en VM-Java(`--workers=2`) y VM-C SMP. Output != →
divergencia de semántica → bug a investigar.

**Test races con TSan**: `gcc -fsanitize=thread -g` sobre el host VM-C.
Detecta races automáticamente — invaluable. NO funciona en FreeRTOS.

**Caveat memory model**: x86 es TSO (Total Store Ordering); ARM
Cortex-M33 es más débil. Bugs de memory ordering pueden no aparecer
en host. Mitigación: usar SOLO primitivas de `bpvm_platform.h`
(pthread_* / FreeRTOS mutex+sem), que tienen barreras correctas en
ambas plataformas. Evitar atomics ad-hoc.

## 9. Samples de test multi-thread

A escribir bajo `samples/smp/`:

- `producer_consumer.bp` — dos threads que intercambian datos via
  Mutex + array compartido.
- `parallel_sum.bp` — N threads sumando trozos de un array, join al
  final.
- `mutex_contention.bp` — alto contention en un solo mutex (stress).
- `gc_stress.bp` — alloc agresivo desde 2 threads para disparar STW
  GC en mid-loop.

## 10. Subtasks de H2 (por orden)

| Task | Fase | Notas |
|---|---|---|
| #179 | DISEÑO | Este documento. Cerrado al consensuar con el usuario. |
| **VM-Java SMP study** | EXPLORACIÓN | Correr `--workers=2` sobre samples. Baseline medido: ×1.52 speedup en `smp_fib_bench` (3453 ms → 2273 ms). |
| **#156a** | IMPLEMENTACIÓN host | `scheduler_smp.c` (worker_loop + STW GC dance) + `comm_host.c` (drena output queue → stdout) + output queue. Locks en heap_alloc, mutex pool, spawn, terminate. Safepoint check en `bpvm_interp_run_quantum`. |
| (TSan pass) | VALIDACIÓN host | `make CFLAGS_EXTRA=-fsanitize=thread`, correr samples smp, fix races. |
| (Bench host) | VALIDACIÓN host | Speedup 1-worker vs 2-worker. Target: ≥ ×1.5 en `smp_fib_bench`. |
| **#136** | PREREQ Pico | Separar firmware en `vm_task` (legacy) → `worker_task[0..N]` + `comm_task`. **YA NO está en v2 — sube como prereq H2**. |
| **#138** | PREREQ Pico | CDC multiplex framing: distinguir BP-program output vs replies wire-v1. Solo afecta a Pico. |
| **#153** | IMPLEMENTACIÓN Pico | `comm_pico.c` (CDC RX wire v1 + dispatch + TX exclusive core0). Worker pinning con `vTaskCoreAffinitySet`. |
| (Bench Pico) | VALIDACIÓN Pico | Mismos samples, comparar 1-worker (F4 v1 actual) vs 2-worker. Verificar Run-on-Pico no se rompe. |

## Estado al cierre de hoy (sesión 1)

**Esqueleto entregado y compile-clean:**

| Fichero | Contenido |
|---|---|
| `include/bpvm_comm.h` | API pública: start/stop/output_enqueue. |
| `include/bpvm_smp.h` | Struct interno `bpvm_smp_t` (locks + flags + queue). |
| `include/bpvm_internal.h` | Añadido `struct bpvm_smp* smp;` a `bpvm_t`. |
| `src/comm_host.c` | Output queue ring-buffer + comm task pthread que drena a stdout. |
| `src/scheduler_smp.c` | `worker_loop` + dispatch + `bpvm_smp_init/destroy` + `bpvm_scheduler_run_smp`. |
| `src/bpvm.c` | `bpvm_run_smp(vm, n_workers)` paralelo a `bpvm_run`. |
| `test/main.c` | Flag `--smp=N`. |
| `Makefile` | Añadidos los 2 nuevos .c. |

**Tests pasados:**
- `Arith.mod` legacy + `--smp=1` + `--smp=2`: todos 7 outputs idénticos. ✅
- `Threads.mod` legacy + `--smp=2`: ambos devuelven 200 (mutex BP cross-worker OK, dispatch OK). ✅

**Bug cazado y corregido — sesión 2:**

**Diagnóstico**: el race era exactamente el que sospechábamos pero el
mecanismo era sutil. El intérprete escribe `tc->status = RUNNABLE` al
agotarse el quantum (`interp.c:101`) **sin sostener `vm_lock`**. Esto
abre una ventana entre "interp escribe status=RUNNABLE" y "worker
toma vm_lock para el epilogue". Combinado con que `pthread_cond_wait`
puede retornar **spuriously** (POSIX lo permite), otro worker puede:
1. Despertar espuriamente.
2. Reacquirir `vm_lock`.
3. Ver `tc->status == RUNNABLE` (set por el interp).
4. Picar ese mismo tc → 2 workers ejecutando el mismo tc, machacando
   sus respectivos pc/sp/bp/cs locales sobre la misma memoria de
   tc → PC corrupto a stack region → RET pop garbage → TERMINATED
   prematuro → shutdown sin output.

**Fix aplicado**: nuevo campo `sched_owner` en `bpvm_thread_t`,
gestionado ÚNICAMENTE bajo `vm_lock`:
- -1 = libre, pickeable.
- otro = wid del worker que actualmente ejecuta el tc.

`pick_next_runnable_locked` ahora exige `status == RUNNABLE` AND
`sched_owner == -1`. El interp puede mutar status libremente (race
benigno) porque el scheduler ya no se basa en él para pickability.

**Resultados tras el fix:**

| Test | Resultado |
|---|---|
| `Arith.mod --smp=2` | 7 outputs correctos ✅ |
| `Bench.mod --smp=2` | 870 ✅ |
| `BenchCpu.mod --smp=2` (fib 30) | 832040 ✅ |
| `Threads.mod --smp=2` ×3 runs | 200, 200, 200 ✅ (mutex BP serializa correctamente) |
| **`SmpFibBench.mod --smp=2` ×3 runs** | **paralelo 2×fib(30): 103, 105, 103 ms** ✅ |
| Speedup paralelo: `--smp=1` (203ms) vs `--smp=2` (103ms) | **×1.97 ≈ ideal lineal** |
| Comparativa VM-Java baseline: ×1.49 | **VM-C escala mejor (no JVM GC, dispatch más simple)** |

**Locks añadidos (sesión 2 continuación):**

1. ✅ **Helper `bpvm_smp_lock/unlock`** en `bpvm_internal.h` — no-op
   en single-worker, agarra `vm->smp->vm_lock` en SMP. Inline static.
2. ✅ **Lock en `bpvm_heap_alloc`** + `bpvm_heap_gc` (manual).
3. ✅ **Lock en `bpvm_thread_spawn`** + broadcast tras crear el tc
   nuevo (avisa al scheduler que hay RUNNABLE adicional).
4. ✅ **Lock en `bpvm_mutex_alloc/add_waiter/pop_waiter`** + broadcast
   tras `pop_waiter` (despierta scheduler para repick).
5. ✅ **STW GC dance**:
   - Nuevo safepoint en `interp.c:99` — al inicio de cada opcode,
     `if (vm->smp && vm->smp->stop_the_world)` rompe el quantum.
     Coste hot-path en single-worker = 1 null-check.
   - `bpvm_heap_alloc` cuando dispara GC: iza `stop_the_world`,
     broadcast a sched_cond, espera con cond_wait a que
     `running_workers == 1` (sólo el orquestador), corre GC, baja
     flag, broadcast.
   - `worker_loop` decremento de running_workers: si
     `stop_the_world`, broadcast (el orquestador despierta).
   - `worker_loop` top de inner loop: si `stop_the_world`, cond_wait
     en sched_cond (worker parqueado durante GC).

**Validación completa tras locks + STW:**

| Test | Workers | Iteraciones | Resultado |
|---|---|---|---|
| Arith legacy | 1 | 1 | ✅ output exacto |
| Bench / BenchCpu `--smp=2` | 2 | 1 | ✅ resultados correctos |
| Threads `--smp=2` (200 incrementos con Mutex BP) | 2 | 3 | ✅ 200, 200, 200 |
| SmpFibBench `--smp=2` paralelo | 2 | 3 | ✅ 106-108 ms |
| Speedup vs `--smp=1` | 2 | 3 | **×1.9 ≈ ideal lineal** |
| HeapStress N=3000 objetos × 2 workers | 2 | 5 | ✅ 4501500 / 4501500 (deterministic) |

**Comm task host cableado (sesión 3):**

- `interp.c` emit_text ahora rutea según prioridad:
  1. `output_cb` instalada (test harness / IDE) → callback.
  2. `vm->smp` activo → encola en output queue via
     `bpvm_comm_output_enqueue`. La comm task pthread la drena a
     stdout.
  3. Legacy single-worker → `fwrite(stdout)` (sin overhead extra).

**Test print-stress** (`samples/smp_print_stress.bp`): 2 threads, 30
prints/thread paralelos.

| Métrica | Resultado |
|---|---|
| Total prints emitidos | 60 (30 + 30) ✅ ninguno perdido |
| Líneas íntegras (sin chars entrelazados de threads distintos) | ✅ |
| Determinismo | ✅ consistente entre runs |

**H2 host: HITO COMPLETO.** Lo que entrega:
- Workers en paralelo con dispatch lock-libre del interp loop.
- Mutex BP cross-worker funcional.
- Heap allocation segura cross-worker.
- STW GC con safepoint en el interp.
- Output queue + comm task drenando a stdout sin entrelazar chars.
- ×1.97 speedup en CPU-bound workloads (cerca del ideal lineal).

**Lo que sigue pendiente** (para v2 o port a Pico):

1. **TSan pass** — w64devkit (MinGW) no incluye `libtsan`. Pendiente
   correr en WSL/Linux para captar races sutiles que tests
   manuales no exponen.
2. ~~**Atomic-line print**~~ → **CERRADO** (2026-05-29).
   Cada `bpvm_thread_t` lleva un `out_buf[128]` propio. Bajo SMP,
   `emit_text` escribe ahí y flushea (un solo `bpvm_oq_push`) en
   newline o cuando se llena. Eso garantiza que un `print x, y, z`
   (varios emit_text) sale como UNA línea contigua al ring buffer,
   y por tanto contigua al transport. También flush en cada exit
   del quantum (yield/block/terminated) para que un BP sin newline
   final no quede colgado entre quantums. Cost: 32 × 128 = 4 KiB
   extra por VM. En legacy (`vm->smp NULL`) el buffer no se toca.
3. **F2.v2 GC compacting o free-list reuse** — fuera de scope H2.
   Hoy el sweep marca FREE pero el bump no reusa → heap leak con
   workloads largos. Independiente de SMP.
4. **Port a Pico** — #136 (separar tasks) + #138 (CDC multiplex) +
   #153 (TX exclusive). Con la arquitectura validada en host, este
   port es swap de `platform_pthread.c → platform_freertos.c` y
   `comm_host.c → comm_pico.c`. El resto del runtime no cambia.

## Port a Pico — estado del cableado (2026-05-29)

Toda la maquinaria SMP está integrada en el firmware Pico, opt-in
por compile-time. La validación se hace por escalones — flash entre
escalón y escalón.

**Build switch** (`pico/CMakeLists.txt`):

```
cmake .. -DBPVM_PICO_SMP_WORKERS=N
```

- **N indefinido (default)**: `repl_v1_run` usa `bpvm_run()` legacy.
  Cero cambio runtime — sigue funcionando lo de siempre.
- **N=1**: `bpvm_run_smp(vm, 1)` ejercita TODA la pipeline
  (1 worker FreeRTOS task + comm task + output queue + STW dance)
  sin paralelismo. Si rompe algo, sabemos que es la integración, no
  la concurrencia.
- **N=2**: paralelismo "intercalado" en un core hoy
  (`configNUMBER_OF_CORES=1` en FreeRTOSConfig.h). Sigue validando
  correctness — sin race, sin deadlock — pero sin gain de perf.
- **N=2 + #153 cerrado**: paralelismo REAL multi-core. Requiere
  `configNUMBER_OF_CORES=2` + `vTaskCoreAffinitySet` en los tasks +
  revisión per-core de los spinlocks (`xSemaphoreCreateMutex` los
  promueve a NMI-safe automáticamente, pero hay que confirmar).

**Tasks FreeRTOS en runtime**:

| Task          | Stack | Prio          | Quién la crea       | Vida   |
| ------------- | ----: | :-----------: | ------------------- | ------ |
| `vm_task`     |  4 KB | IDLE+2        | `main()`            | siempre |
| comm task     |  4 KB | IDLE+1        | `bpvm_comm_start`   | por sesión RUN |
| worker × N    |  4 KB | IDLE+1        | `bpvm_scheduler_run_smp` | por sesión RUN |

Tras la sesión, las tasks transientes terminan limpias (workers
joineados, comm cerrado con `bpvm_oq_close` + drain).

**Cosas a observar al flashear con SMP=1**:

1. El boot debe ser idéntico al baseline (bench fib(28) y la
   pipeline AOT no cambian — usan `bpvm_run`, no SMP).
2. Al hacer Run desde el IDE: log debe contener
   `RUN/v1: SMP path n_workers=1` antes de la primera línea OUTPUT.
3. El output debe llegar BIEN al IDE — todos los eventos JSON
   OUTPUT enteros y en orden (la diff vs. legacy es que ahora va
   por la queue + comm task en vez de directo desde el interp).
4. `EXITED` debe mostrar `status=OK` y `elapsedMs` razonable
   (puede ser ligeramente mayor que legacy por el cost del comm
   task — esperado ~5-10% overhead).

Si algo falla en 1-4, fallback inmediato: rebuild sin
`-DBPVM_PICO_SMP_WORKERS`, vuelve a legacy en 1 minuto.

## #153 — Dual-core RP2350 (TX-exclusive por core)

Estado (2026-05-29): **scaffolding COMPLETO + build-verificado, NO
validado en placa.** El dual-core es opt-in por compile-time y NO
afecta al firmware que se envía (single-core por default, byte-idéntico).

### Build switch

```
cmake .. -DBPVM_PICO_NUM_CORES=2 -DBPVM_PICO_SMP_WORKERS=2
```

- `BPVM_PICO_NUM_CORES` (default **1**) → `configNUMBER_OF_CORES` en
  FreeRTOSConfig.h. Con =2 el port RP2350_ARM_NTZ (que es SMP-capaz)
  arranca el segundo core en `vTaskStartScheduler`, activa
  `configRUN_MULTIPLE_PRIORITIES` + `configUSE_CORE_AFFINITY`, y CMake
  enlaza `pico_multicore`.
- Build matrix verificada: =1 → 366 KiB uf2 (idéntico al baseline),
  =2 → 372 KiB uf2 (delta = kernel SMP + passive idle + multicore).
  Ambos linkan limpio.

### Modelo "TX-exclusive por core"

| Core | Rol | Toca USB/flash | Tasks |
| ---- | --- | -------------- | ----- |
| 0 | I/O + control | **SÍ** (USB CDC TX, SAVE/log flash) | `vm_task`, comm task, idle |
| 1 | cómputo BP puro | **NO** | worker, passive idle |

El worker (core 1) NUNCA transmite por USB ni escribe flash — solo
encola en el ring buffer (`bpvm_comm_output_enqueue`). El comm task
(core 0) es el único que llama `output_cb`/CDC. Por eso "TX-exclusive":
una sola CPU habla con el USB, sin contención del periférico ni de
TinyUSB entre cores. El pinning lo expresa
`bpvm_platform_thread_create_pinned` (comm→0, worker→1).

### Los tres peligros del dual-core y cómo se tratan

1. **Flash XIP durante erase/program** — `flash_range_erase` deja el
   flash inaccesible para fetch; el OTRO core, ejecutando desde XIP,
   haría hard fault. **Resuelto**: `pico/flash_lock.{h,c}` centraliza
   una ventana XIP-safe. fs.c y log.c ya NO llaman
   `save_and_disable_interrupts` directo — usan
   `bpvm_flash_lock_begin/end`. Single-core: idéntico a antes.
   Dual-core: además `multicore_lockout_start/end_blocking` parquea el
   core 1 en RAM. El worker se registra como víctima vía
   `bpvm_flash_lock_init_victim()` (lo llama el trampolín de
   platform_freertos cuando crea un task pinned a core != 0).

2. **USB / TinyUSB affinity** — el IRQ USB y `tud_task` viven en el
   core que hizo `stdio_init_all` (core 0, en `main`). **Resuelto por
   diseño**: el comm task pinned a core 0 es el único que toca CDC;
   core 1 nunca.

3. **FIFO IRQ: lockout del SDK vs scheduler SMP** — analizado en
   código (2026-05-29), riesgo REVISADO A LA BAJA. La hipótesis inicial
   (mismo conflicto que tumbó `pico_flash` en FP2) era el modelo RP2040,
   donde FreeRTOS y el lockout compartían el FIFO. En el RP2350 son
   mecanismos HW DISJUNTOS:
     * El port FreeRTOS-SMP RP2350_ARM_NTZ señaliza entre cores con los
       **multicore doorbells** (`multicore_doorbell_*` en port.c) — NO
       toca el SIO FIFO IRQ.
     * `multicore_lockout_victim_init()` reclama el **SIO FIFO IRQ**
       (`SIO_FIFO_IRQ_NUM` + `irq_set_exclusive_handler`).
   Verificado además que NADIE más reclama el FIFO IRQ (ni el port ni
   nuestro main.c — los únicos hits "FIFO" son el FIFO HW del UART).
   Por tanto `irq_set_exclusive_handler` no debería panic-ar y los dos
   mecanismos coexisten. Durante la ventana de flash (pocos ms) el
   core 1 gira en el handler de lockout — el tick/doorbell de FreeRTOS
   en core 1 se starva brevemente, aceptable (es de facto una sección
   crítica). **Sigue necesitando confirmación en placa**, pero ya no
   es "probable brick" — es "correctness SMP estándar a verificar".
   Plan B si aun así falla: barrera FreeRTOS (suspender worker + girar
   idle de core 1 en RAM con IRQs off) en vez del lockout del SDK.

   Nota de recuperación: un "brick" en Pico NO es permanente —
   BOOTSEL + arrastrar un .uf2 bueno siempre recupera. El peor caso de
   cualquier escalón fallido = mantener BOOTSEL y reflashear el
   single-core. Por eso se puede iterar sin miedo.

### Runbook de validación (en placa, por escalones)

Cada escalón se flashea y valida ANTES del siguiente. Fallback siempre
= rebuild con el flag anterior.

0. **Baseline** (default, single-core) — confirmar que sigue todo.
1. **`-DBPVM_PICO_SMP_WORKERS=1`** (single-core) — valida pipeline SMP
   sin paralelismo. Ver checklist "SMP=1" arriba.
2. **`-DBPVM_PICO_NUM_CORES=2 -DBPVM_PICO_SMP_WORKERS=2`, programa SIN
   SAVE** — valida que arrancan 2 cores, el worker corre en core 1,
   el output llega entero por core 0, EXITED OK. AQUÍ se mide el
   speedup real (un fib BP en core 1 mientras core 0 drena). NO hacer
   SAVE todavía.
3. **dual-core + SAVE** — el momento de la verdad para el peligro #3.
   Hacer un PUT+SAVE pequeño. Si cuelga → es el conflicto FIFO; aplicar
   plan B. Si pasa → log persiste y reboot recarga: dual-core completo.

Mientras 2-3 no se validen en placa, `BPVM_PICO_NUM_CORES=2` queda
marcado **experimental** y NO se envía.

## Abstracción transport

```c
// bpvm_comm.h — interfaz común
int  bpvm_comm_init(bpvm_t* vm);
void bpvm_comm_task_run(bpvm_t* vm);          // body del hilo dedicado
void bpvm_comm_output_enqueue(bpvm_t* vm,
                              const char* buf,
                              size_t len);    // workers llaman aquí
```

Dos backends: `comm_host.c` y `comm_pico.c`. El resto del runtime no
sabe en qué plataforma corre — solo habla con `bpvm_comm_*`.

## 11. Lo que está fuera de scope de H2 v1

- Work-stealing queue: con 2 workers, round-robin protegido va
  sobrado. Reevaluar si N≥4.
- Per-core heap (NUMA-style): no aplica al RP2350 (memoria unificada).
- Concurrent GC: STW es suficiente para nuestros heaps pequeños
  (decenas/centenas de KB en target). Concurrent es invasivo.
- Lock-free data structures: complejidad alta, beneficio dudoso para
  N=2.
