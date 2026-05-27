# BasicPlus — Benchmarks

Suite de pruebas de rendimiento del lenguaje BP sobre las distintas VMs.
Las tablas se rellenan según vamos teniendo módulos validados.

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
| `Mandelbrot.compute(80x40)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `NewtonSqrt.batch(10000)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |

### Strings — heap churn + memory ops

| Bench | `J` (host) | `Ci` (Pico interp) | `Ca` (Pico AOT) | speedup | Notas |
|---|---:|---:|---:|---:|---|
| `StringConcat.build(10000)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `StringSplit.parse(N)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `StringSearch.find(N)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |

### Memoria / arrays — array access + locality

| Bench | `J` (host) | `Ci` (Pico interp) | `Ca` (Pico AOT) | speedup | Notas |
|---|---:|---:|---:|---:|---|
| `Sort.bubble(1000)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `Sort.quick(10000)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `Matrix.mult(32x32)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | — |

### OO — INVOKE_VIRTUAL overhead

| Bench | `J` (host) | `Ci` (Pico interp) | `Ca` (Pico AOT) | speedup | Notas |
|---|---:|---:|---:|---:|---|
| `VtableDispatch.loop(1M)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | dispatch puro |
| `PinToggle.virtual(100k)` | _pendiente_ | _pendiente_ | _pendiente_ | _pendiente_ | Pin.set via vtable |

### HW (solo Pico) — builtin path

| Bench | `Ci` (interp) | `Ca` (AOT) | speedup | Notas |
|---|---:|---:|---:|---|
| `GpioToggle.rate(1M edges)` | _pendiente_ | _pendiente_ | _pendiente_ | Builtin nativo — esperamos ratio ~1× (no hay ops BP que AOT-izar). |
| `I2c.scanLatency` | _pendiente_ | _pendiente_ | _pendiente_ | — |
| `Adc.sampleRate` | _pendiente_ | _pendiente_ | _pendiente_ | — |

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
