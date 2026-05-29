# BasicPlus — Benchmarks

Suite de pruebas de rendimiento del lenguaje BP sobre las distintas VMs.
Las tablas se rellenan según vamos teniendo módulos validados.

## Estado v1

**Cubierto** (datos reales en las tablas):
- Enteros — call overhead + aritmética 32-bit (×66-90 speedup AOT).
- Floats — operaciones float + branches (×54 speedup AOT).
- Memoria / arrays — bubble/insertion/selection sort (×11-14 speedup AOT).

**Diferido a v2** (anotado en cada tabla):
- Strings — depende de #173 P-aot-strings.
- OO virtual dispatch — depende de #174 P-aot-methods.
- HW (GPIO/I2C/ADC) — requiere mesa de pruebas con osciloscopio.

Las 3 categorías cubiertas validan el AOT en los workloads más comunes
(CPU-puro, float-math, memory-bound). Las 3 diferidas son refinamientos.

## Objetivo

Tener un cuadro claro de:
1. **Performance absoluto** de BP en cada plataforma (Java host vs Pico
   interp vs Pico AOT).
2. **Speedup del AOT** (`.mdn`) por categoría de carga: ¿dónde merece la
   pena marcar `function native` y dónde el overhead lo come?
3. **Baseline histórico** para detectar regresiones cuando toquemos el
   intérprete o el GC.

## Metodología

- Cada `.bp` en `samples/benchmarks/` mide UNA sola cosa (no mezcla).
- Cada bench imprime `<nombre> <iteraciones> <ms>` para parseo
  automático.
- Reportamos el **mediana de 5 runs**. La primera ejecución se descarta
  (warm-up, especialmente en JVM por JIT).
- La frecuencia del Pico se fija en 150 MHz (default RP2350). Si en
  algún test usamos overclock lo anotamos.
- Para AOT: cada bench que valga la pena se compila con
  `function native` y se mide la diferencia con/sin `.mdn` en FS.

### Cómo lanzar un bench (cuando esté la suite armada)

```bash
# 1. Generar .mdn si el bench tiene native function
cd bpgenvm-c/pico
./build_mdn.sh <ModuleName>

# 2. Desde el IDE, "Run on Pico" sobre samples/benchmarks/<ModuleName>.bp
#    El IDE sube .mod + .mdn (si existe) y lanza RUN.

# 3. Anotar el tiempo del print final en la tabla correspondiente.
```

## Plataformas medidas

| ID | Plataforma | Notas |
|---|---|---|
| `J` | VM Java en host (PC) | Java 17, OpenJDK, JIT calienta. Upper-bound CPU. |
| `Ci` | VM-C en Pico, interpretado | RP2350 @ 150 MHz, FreeRTOS, USB CDC para wire. |
| `Ca` | VM-C en Pico, con AOT (`.mdn`) | Mismo + thunk Thumb-2 zero-copy desde FS. |

## Resultados

### Enteros — call overhead + aritmética 32-bit

| Bench | `J` (host) | `Ci` (Pico interp) | `Ca` (Pico AOT) | `Ci/Ca` speedup | Notas |
|---|---:|---:|---:|---:|---|
| `Bench.fib(28)` | _pendiente_ | ~5500 ms | 83 ms | **66×** | ✅ validado #158 |
| `Fibo.fib(30) × 5` | _pendiente_ | 80 645 ms | 1 095 ms | **73.6×** | ✅ fibobench.bp (mismo módulo, ambas versiones) |
| `IntBench.intOps(500) × 200` | _pendiente_ | 900 ms | ≤ 10 ms | **≥ 90×** | ✅ intbench.bp — recursión + mul/mod/sub/add. AOT por debajo del floor del timer (1 ms); ratio real probablemente >>200×. |
| `Factorial.run(10000)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `Primes.sieveTo(10000)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `Gcd.batch(1000)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |

### Floats — float ops + branches

| Bench | `J` (host) | `Ci` (Pico interp) | `Ca` (Pico AOT) | speedup | Notas |
|---|---:|---:|---:|---:|---|
| `FloatBench.piLeibniz(200000)` | _pendiente_ | 1 781 ms | 33 ms | **54×** | ✅ floatbench.bp — while-loop con división float + signo. Valida #165 (loops) y #166 (float). Ratio menor que int (54× vs 90×) — el intérprete ya paga overhead serio por op float (bits↔float wrap), así que la ventaja relativa del AOT se comprime. |
| `Mandelbrot.compute(80x40)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `NewtonSqrt.batch(10000)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |

### Strings — heap churn + memory ops

> **v1**: AOT no soporta strings (ver #173 deferred a v2). Estos benches
> ejecutarían sólo en `Ci` (Pico interp). Se difieren a v2 cuando #173
> aporte la columna `Ca` con valor real.

| Bench | `J` (host) | `Ci` (Pico interp) | `Ca` (Pico AOT) | speedup | Notas |
|---|---:|---:|---:|---:|---|
| `StringConcat.build(10000)` | _v2_ | _v2_ | n/a en v1 | n/a | Bloqueado por #173 P-aot-strings |
| `StringSplit.parse(N)` | _v2_ | _v2_ | n/a en v1 | n/a | Idem |
| `StringSearch.find(N)` | _v2_ | _v2_ | n/a en v1 | n/a | Idem |

### Memoria / arrays — array access + locality

| Bench | `J` (host) | `Ci` (Pico interp) | `Ca` (Pico AOT) | speedup | Notas |
|---|---:|---:|---:|---:|---|
| `ArrayBench.bubbleSort(200 rev)` | _pendiente_ | 359 ms | 33 ms | **10.9×** | ✅ arraybench.bp — bubble sort N=200 reverse-ordered (worst case, ~20k cmp + ~2.5k swap). Valida #167 (array read/write) y #170 (lvalues). Ratio compress vs int/float: el helper de array hace mismo bounds+null check que el OP del intérprete, sólo se ahorra el dispatch del bytecode + push/pop de stack. |
| `SumBench.sumArray(5000) × 50` | _pendiente_ | 1 525 ms | 70 ms | **21.8×** | ✅ sumbench.bp — read-only loop. Valida #168: native llama a `now()` internamente (self-measured = 70 ms, idéntico al external timing — confirma builtin call desde AOT). Ratio mejor que bubble (sin writes ni swap), pero limitado por bounds check del helper. |
| `SortBench.bubble(300 rev)` | _pendiente_ | 804 ms | 70 ms | **11.5×** | ✅ sortbench.bp — bubble sort N=300 reverse. Mismo perfil que ArrayBench pero N mayor → más swaps. |
| `SortBench.insertion(300 rev)` | _pendiente_ | 449 ms | 34 ms | **13.2×** | ✅ sortbench.bp — insertion sort N=300 reverse. Mejor ratio que bubble (menos branches, más read locality). Valida `and` short-circuit en AOT. |
| `SortBench.selection(300 rev)` | _pendiente_ | 347 ms | 25 ms | **13.9×** | ✅ sortbench.bp — selection sort N=300 reverse. Mejor ratio del trío (fewer writes total). Valida switch/case (#176): los 3 sorts se invocan via `sortDispatch(a, n, kind)` en el mismo módulo. |
| `Sort.quick(10000)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `Matrix.mult(32x32)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |

### OO — INVOKE_VIRTUAL overhead

> **v1**: AOT no soporta vtable dispatch desde nativo (ver #174 deferred
> a v2). En v1 los methods llaman fallback al interpreter aunque la
> función llamante sea AOT. Estos benches se difieren a v2.

| Bench | `J` (host) | `Ci` (Pico interp) | `Ca` (Pico AOT) | speedup | Notas |
|---|---:|---:|---:|---:|---|
| `VtableDispatch.loop(1M)` | _v2_ | _v2_ | n/a en v1 | n/a | Bloqueado por #174 P-aot-methods |
| `PinToggle.virtual(100k)` | _v2_ | _v2_ | n/a en v1 | n/a | Idem |

### HW (solo Pico) — builtin path

> **v1**: requieren hardware específico (osciloscopio para GPIO, bus I2C
> con periférico conocido, fuente analógica para ADC). Se difieren a v2
> cuando se monte la mesa de pruebas. Ratio esperado: ~1× (el AOT no
> aporta sobre el coste del builtin nativo HW).

| Bench | `Ci` (interp) | `Ca` (AOT) | speedup | Notas |
|---|---:|---:|---:|---|
| `GpioToggle.rate(1M edges)` | _v2_ | _v2_ | _esperado ~1×_ | Requiere oscilo. |
| `I2c.scanLatency` | _v2_ | _v2_ | _esperado ~1×_ | Requiere bus con periférico. |
| `Adc.sampleRate` | _v2_ | _v2_ | _esperado ~1×_ | Requiere señal analógica. |

### Mixto / dogfood

| Bench | `J` (host) | `Ci` (Pico interp) | `Ca` (Pico AOT) | speedup | Notas |
|---|---:|---:|---:|---:|---|
| `Parser.lexMathBp` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | BP lexa Math.bp |
| `Parser.parseMathBp` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |

## Análisis cualitativo (a rellenar)

- **¿Dónde da el AOT su mejor ratio?**  _pendiente_
- **¿Dónde NO merece la pena AOT-izar (overhead del thunk > beneficio)?**  _pendiente_
- **¿Cuál es la latencia base de un OP_CALL hijack-registry hit?**  _pendiente_ (medir con `EmptyCall.batch(N)` — fib(0) es la mejor aproximación)
- **¿Memoria heap consumida por categoría?**  _pendiente_ (instrumentar bpvm_used_heap antes/después de cada bench)

## Histórico de resultados

| Fecha | Commit | Plataforma | Nota |
|---|---|---|---|
| 2026-05-26 | `f9d22d7` | RP2350 @ 150 MHz | Baseline: `Bench.fib(28)` AOT = **83 ms** (vs ~5500 ms interp = 66× speedup) |

## Referencias

- Sample del baseline: `samples/benchmarks/Bench.bp` (recursive fib).
- Pipeline AOT: ver `docs/AOT_HYBRID_REFLECTION.md` + `bpgenvm-c/pico/build_mdn.sh`.
- Formato `.mdn`: ver `bpgenvm-c/src/mdn_format.h`.
