# BasicPlus — guía general del proyecto

> **V1 CERRADA (2026-06-07).** Los 4 hitos (H1–H4: comunicaciones, dual-core,
> AOT, ESP32-S3) están completos; ESP32-S3 end-to-end fue la condición de cierre
> (tarea #147). Este documento queda como **brújula histórica de v1**; el ciclo
> en curso es **V2** (su organización vive en su propia brújula). Aviso de
> alcance: la familia "Linux embedded" listada en §6 ("más familias MCU") queda
> **fuera** del proyecto — ver el *sobre de plataformas* en `docs/PHILOSOPHY.md`.

> **Documento de orientación.** Captura dónde estamos, dónde vamos y
> qué hitos quedan antes de cerrar la **versión 1**. No es spec de
> implementación; es la brújula. Cuando se añadan ideas nuevas, van
> al apartado de "wishlist v2" — v1 tiene alcance cerrado.

*Fecha de redacción: 24-mayo-2026. Conversación entre Eduardo y
Claude Opus 4.7.*

---

## 1. Qué es BasicPlus

Un lenguaje de propósito general portable a microcontroladores
con:

- **Sintaxis BASIC moderno**: `function`, `if/then/endif`,
  `while/do/endwh`, `:=` para asignación. Fácil de leer.
- **Tipado estático fuerte**: integer, float, boolean, string, arrays,
  clases, enums. Errores en compile time, no runtime.
- **Programación OO completa**: clases, herencia simple, properties,
  static members, módulos cross-module.
- **Concurrencia**: threads cooperativos, mutex, sync properties,
  scheduler con quantum.
- **Manejo de errores**: try/catch BP con jerarquía de excepciones
  tipadas.
- **Debugger usable**: breakpoints, paso a paso, inspección de
  variables, propiedades de módulos.

Compilador en Java → bytecode `.mod` portable. Dos VMs idénticas
semánticamente:

- **VM Java** (host PC) para desarrollo y debugging cómodo.
- **VM C** (host + embedded) para correr en el dispositivo final.

El mismo `.mod` compilado en PC ejecuta en ambos.

---

## 2. Estado actual (24-may-2026)

Lo que **ya funciona end-to-end**:

| Categoría | Estado |
|---|---|
| Frontend Java (lenguaje completo + tipos) | ✅ Completo |
| VM Java host con debugger + multi-thread | ✅ Completo |
| VM C portable (host + Pico) | ✅ Completo |
| Firmware Raspberry Pi Pico 2 (RP2350) | ✅ Completo |
| IDE BpIde con multi-tab, debug, JTree del FS remoto | ✅ Completo |
| Manual HTML completo | ✅ Completo |
| Suite HW = clase OO (10 clases + 1 módulo funcional) | ✅ Completa |
| Overclock controlado con auto-vreg | ✅ Validado HW |
| RTC con sync IDE→Pico | ✅ Validado HW |
| FS persistente con jerarquía `/sys` `/lib` `/app` | ✅ Funcional |

**Suite HW**: Gpio.Pin, I2c.Bus, Spi.Bus, Uart.Port, Pulse.Counter,
Pwm.Slice, Adc.Channel, Rtc.Clock, Wdt.Timer, Timer.Alarm + módulo
Pico (info MCU). Todas validadas en hardware real.

**Drivers de dispositivo de prueba**: MCP9804 (sensor temp I2C),
PCA9554 (GPIO expander), AD7177-2 (ADC SPI).

---

## 3. Filosofía

### Cerrar v1, evitar feature creep

Los proyectos sin "v1 cerrada" mueren — no por bugs ni falta de
features, sino por **agotamiento del mantenedor**. La disciplina de
declarar "v1 = este alcance, nada más" es lo que distingue un
proyecto que termina de un proyecto que sigue creciendo
indefinidamente hasta que un día se abandona.

### La fase de pulir tiene valor propio

Cerrar v1 obliga a **pulir, documentar, samples, errores legibles,
mensajes de instalación**. Esa fase es la que distingue una "demo
impresionante a un dev solo" de una "herramienta usable por
terceros". Sin esa fase, el código nunca sale del laptop del autor.

### Ideas nuevas → wishlist v2

Todo lo que se nos ocurra durante la fase final de v1 que no sea
estrictamente necesario para cerrar v1 va a `docs/V2_BACKLOG.md`.
Es valioso precisamente porque NO contamina v1.

---

## 4. Los cuatro hitos para cerrar v1

Hay cuatro hitos pendientes, en este orden:

### H1 — Comunicaciones limpias (USB CDC + TCP/IP)

**Estado v1: CERRADA con scope reducido.**

**Tasks v1 (completadas)**:
- ✅ #137 transport-abstract — `AbstractBpvmBackend` + `BpvmClient`
  con `connectRemote()`/`connectSerial()` ya comparten 100% del wire
  v1. `PicoClient` legacy retirado.
- ✅ #139 interp-debug-hook — plumbing del hook en el inner loop
  de la VM-C, no-op por default (hook=NULL = un null-check por
  opcode). Queda como cimiento para v2.

**Tasks deferidas a v2**:
- ⏭️ #136 arch-tasks — separar firmware en tasks FreeRTOS. Beneficio
  v1 (REPL responsive durante cómputo BP largo) marginal vs riesgo
  de brick (ya hubo uno).
- ⏭️ #138 cdc-multiplex — sin trigger sin #140.
- ⏭️ #140 debug-pico-impl — back-end real del debug sobre el Pico.
- ⏭️ #145 wifi-tcp — Pico 2 W only, requiere stack lwIP + cyw43;
  nice-to-have.

**Razón del defer**: el flujo Debug Run (Shift+F5 en VM-Java local
con breakpoints, step, locals) + Run on Pico (HW real) cubre el 95%
del valor que tendría debug-on-Pico. Bugs HW-específicos se cazan
con `print` instrumentado. El plumbing #139 deja el cimiento listo
para que v2 implemente el back-end sin tener que tocar la API
pública de la VM-C.

**Documentos guardados**: `docs/WIFI_TCP_REFLECTION.md`.

### H2 — Dual-core en el Pico (y futuro ESP32)

**Tasks**: #153 smp-tx-exclusive, #156 vm-dual-worker, #179 SMP
architecture. Nota: #136 arch-tasks (deferida con H1 a v2) prepararía
el terreno; H2 lo extendería a "una task FreeRTOS por thread BP" o
"scheduler con afinidad de core".

**Qué incluye**:
- F4 v2 — refactor del scheduler para usar los 2 cores del RP2350.
- Mutex en estructuras compartidas (heap, FS, transport).
- Smoke tests de rendimiento con apps multi-thread.

**Por qué segundo**: rediseño contenido del scheduler. NO bloquea AOT
(el código nativo no sabe ni le importa qué core lo ejecuta). Da
×1.5-2 con esfuerzo razonable.

**Estado v1 — CERRADA con scope: single-core SMP validado en placa;
dual-core diferido a v2.**

- ✅ **#156 / #179 — runtime SMP en host VM-C**: pool de N workers +
  STW GC + output queue + comm task. Speedup ×1.97-2.4 medido en host.
- ✅ **Cabling Pico** (#136, #180-#184): comm task FreeRTOS, ring buffer
  compartido (comm_common.c), pinning por core, `bpvm_run_smp` opt-in
  (`-DBPVM_PICO_SMP_WORKERS`).
- ✅ **SMP single-core VALIDADO EN HARDWARE** (RP2350): print_stress
  (60 líneas íntegras), heap_stress (20100×2, alloc cross-thread sin
  corrupción), fib_bench (cómputo + join + timing). Ver SMP_ARCH.md.
  El runtime SMP corre de verdad en el M33.
- ⏭️ **#153 — dual-core RP2350: DIFERIDO A v2.** El scaffolding está
  completo y build-verificado (config gated `-DBPVM_PICO_NUM_CORES=2`,
  flash XIP-safe en flash_lock.c, lockout multicore, passive idle,
  modelo asimétrico I/O-core0 / VM-core1). PERO el boot dual-core se
  cuelga al **lanzar el core 1** (core 0 bloqueado en
  `multicore_launch_core1` esperando ACK; el core 1 falla en su init).
  Es bring-up del kernel FreeRTOS-SMP en RP2350, no de nuestro código.
  Diferido a v2 porque pide un **depurador SWD (Picoprobe + gdb)** para
  poner breakpoint en el arranque del core 1 — inviable por LED binario
  remoto. Estado completo en SMP_ARCH.md §"Dual-core: diferido a v2".

- ❌ **#185 — atomic-line per-tc: REVERTIDO.** Infló `bpvm_t` 4 KB
  (×32 threads) → OOM en la Pico, ningún módulo corría. Lección
  registrada: cambios al struct `bpvm_thread` se multiplican ×32.

**Por qué single-core es suficiente para v1**: el modelo asimétrico
(I/O en un core, VM en otro) que daría el beneficio real —REPL
responsivo mientras la VM cruje— requiere el mismo arranque dual-core
que falla. Single-core entrega un runtime SMP correcto y completo en
el MCU; el paralelismo de cores es una optimización para v2.

### H3 — AOT por módulo (compilación a Thumb-2 nativo)

**Tasks**: #144 P-aot-hybrid.

**Qué incluye**:
- Refactor del intérprete: cada `case` del switch a función nombrada
  en tabla expuesta (helpers).
- Definir ABI v1 de helpers.
- Loader de módulos nativos: aplica relocs/GOT, resuelve helpers.
- Generador en el frontend Java: opcodes baratos inline en Thumb-2,
  opcodes caros via `bl helper`.
- Peephole opts (constant folding, dead code, register promotion).

**Documentos guardados**: `docs/AOT_HYBRID_REFLECTION.md`. Política
"bytecode = pata negra, nativo = caché derivada".

**Por qué tercero**: el más invasivo. Se beneficia de H2
(paralelización de apps que usen código nativo) y, cuando v2 traiga
debug-on-Pico real, también del debug del código generado.

**Estado v1**: ✅ ya entregado. AOT por módulo con helpers expuestos
funcionando — speedup ×54-90 vs intérprete medido en BENCHMARKS.md.

### H4 — Segunda familia de MCU: ESP32-S3

**Por qué ESP32-S3**: barato (~3 € módulo bare), potente (2× Xtensa
LX7 @ 240 MHz, 512 KB SRAM + opción PSRAM hasta 8 MB), popular
(dominante en IoT hobby/pro), **mismo RTOS que el Pico** (FreeRTOS
oficial vía ESP-IDF).

**Reutilización ~80%**:

| Componente | Reutilizable |
|---|---|
| Frontend Java (compiler BP→.mod) | 100% |
| VM C (interp, heap, GC, scheduler, exceptions, threading) | 100% |
| Stdlib BP (lógica, no HW) | 100% |
| API BP de drivers HW | 100% |
| IDE BpIde | 100% |
| Wrappers FreeRTOS | 90% |
| **Backends nativos C de cada driver** | **0%** — reescribir |
| **Backend platform (sleep, now)** | **0%** — reescribir |
| **Firmware main.c** | 30% |

**Trabajo neto**: ~1500-2500 líneas de C. El patrón
`bpvm_xxx_set_backend(...)` ya está establecido — solo cambia la
implementación.

**Decisión importante: módulo `Mcu` portable + `Pico` / `Esp32`
específicos**. El usuario hace `import Mcu` para cosas comunes
(uniqueId, boardName, tempC, cpuFreqHz, setCpuFreqMHz, uptimeMs)
— funciona en ambas plataformas. Si necesita algo específico,
importa `Pico` o `Esp32` según target.

**AOT en ESP32**: NO en v1. El ESP32 usa Xtensa, una ISA distinta a
ARM Thumb-2. Escribir un emisor Xtensa es 3-5× más trabajo que el
Thumb-2. En v1 el ESP32 ejecuta solo bytecode interpretado.
Speedup vs MicroPython en ESP32: ~×4-5 (sin AOT). En Pico con AOT:
×10-16. Ambos son competitivos.

**Por qué último**: cuando llega ESP32 ya tienes todo lo demás
sólido. El refactor de los backends es mecánico — la arquitectura ya
está demostrada en Pico. **Cuando ESP32-S3 funcione end-to-end, v1
se declara cerrada.**

---

## 5. Speedup esperado vs MicroPython

Análisis de los factores de mejora, conservador (rango medio):

| Factor | Speedup | Notas |
|---|---|---|
| **VM** | ×2.5 | BP estáticamente tipado, sin dispatch dinámico de tipos. Pico ya da ×2.8 real vs MicroPython sin optimizar. |
| **Overclock** | ×1.7 | 150→250 MHz sostenible (24/7). 300 MHz alcanzable puntual. Solo Pico. |
| **Dual-core** | ×1.4 | Solo si la app es paralelizable. IO-bound ~×1.3, CPU-puro paralelizable ~×1.9. |
| **AOT a nativo** | ×3 | Promedio ponderado. CPU-puro ×10-20, IO-bound ×1.2. Solo Pico en v1. |
| **TOTAL Pico** | **~×18** | Conservador. Optimista hasta ×100 en CPU-puro paralelizable. |
| **TOTAL ESP32 (sin AOT)** | **~×6** | Sin AOT pero con CPU más rápida y dual-core nativo. |

Pesimista (workload puro IO-bound + sin paralelismo + sin AOT): ×4.
Sigue siendo más rápido que MicroPython.

### Lo que ningún factor cubre

- **Memoria**: BP requiere 3-5× menos RAM que MicroPython por tipado
  estático. Apps más grandes en la misma Pico.
- **Determinismo**: BP allocas menos → menos GC pauses. Importa para
  real-time (motores, audio, comms críticas).
- **PIO del RP2350**: 8 máquinas de estado programables hacen I/O
  paralelo a la CPU. Para drivers tipo WS2812, DVI, custom protocols:
  ×10-50 sin tocar CPU. **No es factor de speedup BP** — es un
  superpoder del chip que algún driver BP podría exponer.

---

## 6. Wishlist v2 (lo que NO entra en v1)

Todo lo que se nos ocurra durante v1 y no sea estrictamente necesario
va aquí. Algunas ideas que ya tenemos:

### Extensiones del AOT (existe el core, faltan refinamientos)

Estas tasks están en el backlog como `pending` pero NO son blockers de
v1. Se hacen "cuando haya hueco" entre las grandes:

- **#161** `P-aot-asm-inline` — reescribir helpers críticos en ARM
  asm inline. Ganancia probable ×1.3-1.5 en hot path.
- **#163** `P-mod-disasm` — disassembler de `.mod` para diagnóstico.
  Útil pero el debugger de H1 cubrirá lo importante.
- **#169** `P-aot-cross-module-call` — llamadas native → native entre
  módulos. Workaround actual: AOT-izar módulos uno a uno.
- **#171** `P-aot-mixed-types` — más tipos en signatures AOT (hoy int + float).
- **#172** `P-aot-module-globals` — acceso a variables nivel-módulo desde native.
- **#173** `P-aot-strings` — string ops en native. Hoy strings van por
  interpreted, que es perfectamente usable.
- **#174** `P-aot-methods` — dispatch virtual (vtable) desde código native.
- **#175** `P-aot-try-catch` — try/catch + throw dentro de native.

### Más allá

- **GC generacional** (mark-sweep actual es simple — un generational
  daría ×2-3 en código alloc-heavy).
- **JIT real** (tracing JIT estilo LuaJIT v2, ×10× más speedup pero
  ×10 más complejidad).
- **AOT para Xtensa** (cuando el ESP32 quiera el speedup ×2-3 del
  AOT que el Pico ya tiene).
- **Más familias MCU**: STM32 (Cortex-M4/M7), RISC-V (CH32V, GD32V),
  Linux embedded (Raspberry Pi normal).
- **Bluetooth/BLE** (el CYW43 del Pico 2 W y el ESP32 lo soportan;
  diferido por coste de BTstack).
- **Lenguaje features**: closures, generics, traits, pattern
  matching, list comprehensions.
- **FFI**: invocar funciones C externas desde BP, registrar
  intrinsics dinámicas, módulos nativos cargables (ver `AOT_HYBRID_REFLECTION.md`).
- **Lua/Python interop**: poder llamar BP desde un host Python o Lua.
- **PIO del RP2350 expuesto como API BP** (driver de WS2812, DVI, etc).
- **Self-hosting**: el compilador BP escrito en BP. Romántico, no
  necesariamente útil.

---

## 7. Definición de "v1 cerrada"

V1 se declara cerrada cuando:

1. **Los 4 hitos están completos** (H1 comunicaciones, H2 dual-core,
   H3 AOT, H4 ESP32-S3).
2. **Samples para cada feature** corren end-to-end en ambas
   plataformas.
3. **Documentación HTML** cubre todo lo expuesto al usuario.
4. **Manual de instalación** "desde cero a Hello World en Pico" en
   <15 minutos para un dev nuevo.
5. **Bugs conocidos críticos = 0** (los menores quedan en backlog v2).

Tras cerrar v1: solo se aceptan **bug fixes, mejoras de
documentación, samples adicionales**. Cualquier feature nueva se
difiere a v2 sin discusión. Esa disciplina es lo que hace que un
proyecto sea **estable** y, por tanto, **adoptable por terceros**.

---

## 8. Donde estamos vs donde vamos

```
                            ┌── H1: Comunicaciones ──┐
                            │                        │
                            │  H2: Dual-core ────────┤
   v0 → [hoy] ──────────────┤                        ├── v1 CERRADA
                            │  H3: AOT (Pico) ───────┤
                            │                        │
                            └── H4: ESP32-S3 ────────┘

         ────── ESTAMOS AQUÍ                         ↑
                                              objetivo "estable
                                               y adoptable"
```

Falta bastante, pero **vamos muy bien**. La base es sólida. Los 4
hitos son trabajo conocido — no investigación. Cuando llegue v1, el
proyecto será una plataforma usable, no un demo.

---

*Conversación entre Eduardo y Claude Opus 4.7, 24-mayo-2026.
Documento vivo: actualizar cuando un hito se complete o se
re-priorice. Las ideas nuevas van a `docs/V2_BACKLOG.md` (futuro).*
