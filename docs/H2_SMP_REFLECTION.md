# H2 — Reflexión sobre dual-core en RP2350 / ESP32-S3

Notas guardadas antes de empezar H2 propiamente. Se redacta al cerrar
H3 (AOT) — el siguiente hito natural según el roadmap es H1
(comunicaciones + debug-on-Pico), y luego H2.

## Modelo propuesto

El RP2350 tiene 2 cores Cortex-M33. FreeRTOS SMP permite repartir
hilos entre cores con o sin afinidad. Para aprovechar los 2 cores **el
bucle del intérprete debe correr en su propio hilo FreeRTOS** — el
bucle es el "punto caliente" y mientras esté en un solo hilo, solo
usa un core.

### Arquitectura de hilos

Hilos permanentes (al arrancar el firmware):
- **Comm thread** — USB CDC + futuro TCP/IP. Lee del wire, despacha
  comandos. Cuando hay output del programa BP, el intérprete escribe
  a un ring buffer compartido y el comm thread lo drena al USB.
- **VM main thread** — bucle del intérprete del módulo principal
  (`Main.bp`). Es el primer "worker de intérprete".

Hilos por demanda:
- **Worker thread N** (N ∈ {1, 2}) — segundo bucle de intérprete que
  ejecuta Threads BP del programa cuando hace falta paralelismo real.
  En v1, **arrancar con un solo worker (el main)** y solo añadir el
  segundo cuando se confirme que el programa usa `Thread`s.

### Por qué máximo 2 workers de intérprete

- El RP2350 (y el ESP32-S3) tienen 2 cores. Más bucles solo añaden
  contención por mutex (heap, FS, transport) sin aumentar throughput.
- Cada bucle de intérprete consume ~4-8 KB de stack C. Con 2 ya son
  8-16 KB; con 4 serían 16-32 KB de la RAM (520 KB total en RP2350,
  apretada cuando metes FS + heap BP + buffers).
- Diseño futuro-compatible: si una v2 quiere más cores (algún ESP32-P4
  con 2 LX7 + 1 LP), podemos subir el límite — pero la convención de
  "2 workers" cubre el 95% de los chips embebidos viables.

### Mapeo Thread BP → worker FreeRTOS

Dos modelos posibles:

**Modelo A — 1 Thread BP = 1 hilo FreeRTOS** (lo que primero apetece):
- Sencillo, aprovecha SMP automático.
- **Problema**: si una app crea 10 `Thread.start()`, son 10 hilos
  FreeRTOS con 4-8 KB de stack cada uno = 40-80 KB solo de stacks.

**Modelo B — pool fijo de workers, cola de Threads BP** (mejor para
embebido):
- N workers fijos (1 o 2 en v1). Cuando un Thread BP arranca, va a
  una cola; el primer worker libre lo recoge.
- Si todos los workers están ocupados, el nuevo Thread BP espera.
- Lo que el scheduler de la VM Java hacía antes (quantum + round-robin)
  se mantiene **dentro de cada worker**: el worker corre N opcodes de
  un Thread BP, hace yield, recoge otro Thread BP de la cola.

Recomendación: **Modelo B**. Da paralelismo real (2 workers en 2
cores) sin explotar el uso de memoria si el programa abusa de
`Thread.start()`.

## El elefante en la habitación: el GC

Mark-sweep concurrente NO es viable en RP2350 sin invertir mucho
esfuerzo:
- Requiere write barriers en cada `OP_AASTORE` / `OP_STOREFIELD`:
  ~5-10 ns extra por write, lo que se come ×0.7 del beneficio de AOT.
- Tri-color marking con sincronización entre el mutator y el collector.
- Race conditions sutiles que solo aparecen bajo carga real.

### Alternativa: stop-the-world disparado por umbral

- GC **no tiene hilo dedicado permanente**.
- Cualquier worker que detecte `heap_used > threshold` levanta un
  evento global.
- Los demás workers paran en su próximo OP-boundary (un check barato
  al inicio del inner loop, `if (vm->gc_pending) yield_to_gc(); `).
- UN solo worker ejecuta mark-sweep mientras los demás esperan en
  semáforo.
- Tras `gc_done`, todos reanudan.

Pros: cero overhead en steady-state. Las pausas son predecibles y
breves (mark-sweep en 520 KB de heap = unos ms).

Contras: durante el GC el sistema "salta" — para aplicaciones
hard-realtime habría que medir y posiblemente preempt el GC.

## Recursos compartidos

Estructuras que **sí** necesitan mutex cuando hay 2 workers:
- Heap BP (allocator + GC mark bits).
- FS (cuando se hace SAVE/LOAD desde código BP).
- Transport (un solo USB CDC para los dos workers).
- AOT registry (lectura compartida, escritura solo al cargar `.mdn`).

Estructuras que **NO** necesitan mutex:
- Per-thread state (`bpvm_thread_t`): cada worker dueño exclusivo del
  Thread BP en ejecución.
- Code section del `.mod` / `.mdn` (read-only tras cargar).
- `vm->aot_helpers` (puntero a tabla const).

## Plan de ataque concreto

1. **Pre-trabajo** (en H1 — ya está): tasks FreeRTOS independientes
   para comm y VM main (#136). Esto deja la base lista.
2. **H2.a — Un solo worker, multithread BP cooperativo**: el `vm_task`
   actual sigue siendo single-threaded para Threads BP pero rotando
   quantum (lo que ya hace la VM Java).
3. **H2.b — Mutex en recursos compartidos**: heap, FS, transport.
   Sin esto no se puede añadir el 2do worker.
4. **H2.c — Segundo worker opcional**: pool de workers (Modelo B),
   creado bajo demanda cuando el programa BP arranca un `Thread`.
5. **H2.d — STW GC**: implementar el barrier al inicio del inner loop
   y el rendezvous entre workers.

Smoke tests al cerrar H2:
- App con 2 Threads CPU-bound → ver 2 cores al ~100%, throughput ×1.8-2.
- App single-threaded → mismo throughput que antes (segundo worker
  duerme, sin overhead).
- App con muchos `Thread.start()` (>10) → workers procesan la cola sin
  desbordar memoria.

## Recordatorio

H1 va primero. Sin debug-on-Pico real, los bugs concurrentes de H2
son imposibles de cazar. El bug de hoy (4 calls AOT void → frame
corrupto) hubiera sido **5 minutos** con un debugger; nos costó **6
horas** sin él.
