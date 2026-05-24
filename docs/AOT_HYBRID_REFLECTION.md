# Reflexión: AOT híbrido por módulo

> Documento de reflexión, no de spec. Recoge una conversación abierta
> sobre cómo se aceleraría BP sin entrar en un JIT completo. La idea
> nuclear es de Eduardo. Lo guardamos para retomarlo cuando se decida
> abordar el tema — hoy NO está planificado.

---

## Punto de partida

BP corre hoy interpretado por la VM (Java y C). El bucle del
intérprete tiene un coste fijo por opcode (fetch + decode + dispatch
indirecto + reanudación) que en Cortex-M33 a 150 MHz ronda los
**5-10 ns por opcode**. La operación útil que ejecuta el opcode varía:
un `ADD_I32` son ~1-2 ns, un `NEW_OBJECT` ~200 ns.

Cualquier intento de acelerar BP gira en torno a **eliminar (o
reducir) ese coste de dispatch** para los opcodes en los que el
dispatch domina sobre la operación útil.

## La idea: AOT por módulo, no JIT

En lugar de un JIT que compile bytecode a nativo en runtime dentro
del dispositivo, **el compilador del PC genera código nativo offline**
para los módulos que el usuario marca como "estables". Convive con la
forma interpretada — cada módulo decide.

### Flujo de uso esperado

1. Mientras desarrollas y depuras, programas en BP normal: editor,
   compilación a bytecode, ejecución interpretada, debugger paso a
   paso, modificar y reiterar.
2. Cuando una librería está estable y validada (p.ej. `Json.bp`),
   se pasa por el "compilador AOT" del PC. Resultado: `Json.mod`
   con código nativo en lugar de bytecode.
3. Subes ese `Json.mod` al dispositivo. Los módulos que lo importan
   no notan diferencia salvo que **van 2-3× más rápidos** en las
   partes CPU-bound.
4. Bytecode y nativo conviven en el mismo FS, en el mismo runtime.

### Por qué AOT y no JIT

| Coste / propiedad | JIT en runtime | AOT en PC (esta idea) |
|---|---|---|
| Compilador en el dispositivo | Sí (50-200 KB extra) | No |
| Latencia al cargar un módulo | Compilation pause | Cero |
| Calidad del código generado | Limitada por tiempo | Tan buena como quieras |
| Memoria RAM extra para code cache | Sí | No |
| Complejidad: tiered, profile, deopt | Sí | No |
| Debuggable (inspeccionar el .mod nativo) | Difícil | Trivial |

Para un sistema embebido el AOT gana en casi todas las celdas.
El JIT solo ganaría si BP fuese dinámicamente tipado o si el código
mutara en runtime — no es nuestro caso. **BP es estáticamente tipado
y los módulos son estables tras compilar**.

---

## Transportabilidad del código nativo

El `.mod` con código nativo debe cargarse en una dirección física
que **no se conoce al compilar**. Dos caminos clásicos:

### A) Position-Independent Code (PIC)

- Direccionamiento relativo al PC para constantes y saltos internos
  (Cortex-M33 lo hace nativamente con `ldr Rd, [pc, #off]` y `b`
  con offset).
- Una **GOT** (Global Offset Table) pequeña con un slot por cada
  symbol externo (helper del runtime, función de otro módulo).
  Un registro reservado (`r9` por convención) apunta a la GOT del
  módulo actual.
- Cargar = parchear la GOT con direcciones reales; el código mismo
  no se toca.

Ventaja: el código puede vivir **en flash** sin reescribirse — se
referencia por XIP desde la dirección donde aterrizó.
Desventaja: una indirección extra por call externo (despreciable a
150 MHz).

### B) Relocatable (linker tradicional)

- El código lleva una **tabla de relocations** que marca huecos
  donde van direcciones absolutas.
- El loader parchea cada hueco con la dirección real al cargar.
- Más eficiente en runtime (no hay GOT), pero el código se
  modifica al cargar → tiene que vivir en RAM.

### Preferencia para Pico

**PIC + GOT pequeña**. Razones:
- El módulo persiste en flash sin reescritura → cross-reboot.
- La GOT es pequeña (4 bytes × imports, típicamente 20-40 bytes).
- La indirección extra cuesta 1-2 ciclos: ruido.

---

## Convivencia bytecode/nativo

El `.mod` actual tiene una entry por función con offset al
bytecode. La extensión natural es:

```
function_t {
  name_ref, ofs, locals, params, ...
  code_kind   // 0 = bytecode, 1 = native
  code        // unión: offset al bytecode, o dirección Thumb-2 cargada
}
```

`CALL_EXT` y `INVOKE_VIRTUAL` hacen dispatch:

- `code_kind == 0`: empuja un frame interpretado, salta al
  intérprete.
- `code_kind == 1`: `bx code | 1` directo a Thumb-2 (el `|1` es
  por el bit Thumb del Cortex-M).

**El llamador no sabe ni le importa qué tipo es el callee.** Por
eso reemplazar `Math.mod` bytecode por `Math.mod` nativo es
invisible para los usuarios — todos los `import Math` siguen
funcionando, ahora más rápidos.

---

## La pieza brillante: helpers expuestos para opcodes caros

Aquí está el insight que cambia toda la conversación.

### Observación

Los opcodes no son iguales. **El coste de dispatch es fijo, el
coste de la operación útil varía 100×** entre opcodes baratos
(ADD) y caros (NEW_OBJECT).

- Si el opcode es **barato** (≤ dispatch), eliminarlo da 5-10×
  speedup en ese opcode.
- Si el opcode es **caro** (≫ dispatch), eliminar el dispatch da
  ~2-5% de mejora — apenas medible.

Por tanto: **solo merece la pena compilar inline los opcodes
baratos**. Los caros pueden delegar al runtime sin perder casi
nada.

### Cómo lo hace el código generado

**Opcode barato** (inline): el generador del PC emite las
instrucciones Thumb-2 equivalentes directamente.

```
PUSH_I32 42          →  movw r0, #42 ; str r0, [sp_bp], #4
ADD_I32              →  ldr r0, [sp_bp, #-4]!
                        ldr r1, [sp_bp, #-4]!
                        add r0, r0, r1
                        str r0, [sp_bp], #4
```

(Donde `sp_bp` es el puntero al stack BP, probablemente promovido
a un registro callee-saved como `r10`.)

**Opcode caro** (helper): el generador emite una llamada al
handler C que ya implementa ese opcode en el intérprete.

```
NEW_OBJECT class_id  →  mov r0, vm
                        mov r1, tc
                        mov r2, #class_id
                        bl handler_new_object
```

`handler_new_object` es literalmente la función C que el
intérprete invoca para su `case NEW_OBJECT`. Reusada, no duplicada.

### Qué opcodes a cada lado

**Inline (los baratos)**:
- Constantes: PUSH_I32, PUSH_F32, PUSH_TRUE, PUSH_FALSE, PUSH_NULL
- Stack: POP, DUP, SWAP
- Frame: LOAD_LOCAL, STORE_LOCAL
- Aritmética: ADD_*, SUB_*, MUL_*, DIV_*, MOD_*, NEG_*
- Lógica/bits: AND, OR, XOR, NOT, SHL, SHR, USHR
- Comparación: CMP_*, EQ_*, NEQ_*, LT_*, LE_*, GT_*, GE_*
- Flujo: JUMP, JUMP_IF_TRUE, JUMP_IF_FALSE, RETURN simple

~15-20 opcodes que cubren el 70-80% del tráfico de un programa
típico.

**Helper (los caros)**:
- Heap: NEW_OBJECT, NEW_ARRAY, ALLOC_REF_ARRAY
- Llamadas: INVOKE_VIRTUAL, CALL_EXT, CALL_BUILTIN
- Excepciones: THROW, CATCH_BEGIN, CATCH_END
- Strings/Arrays: STRING_CONCAT, MOVE, ARRAY_LOAD/STORE (con
  bounds check), LOAD_FIELD/STORE_FIELD (con posible GC barrier)
- GC: GC_TRIGGER (implícito)

---

## Beneficios derivados de este diseño

### 1. Paridad automática bytecode ↔ nativo

El handler de NEW_OBJECT es **una sola función C**, llamada desde
el intérprete y desde el código nativo. Cualquier fix beneficia
a ambos paths sin tocar el módulo nativo. Es factorización, no
duplicación.

### 2. El generador es pequeño

15-20 plantillas para opcodes inline. Para los demás, una
plantilla genérica "load args + bl helper". No hay register
allocator inteligente, no hay escape analysis, no hay type
inference. Template-driven.

### 3. ABI versionado

La tabla de handlers es parte del contrato firmware ↔ módulo
nativo. Se versiona desde el día 1. Un módulo nativo declara
"necesito ABI ≥ v3", el firmware comprueba al cargar. Si no
cuadra, fallback a la versión bytecode si existe.

### 4. Optimizaciones offline "gratis"

El compilador del PC puede hacer (sin coste de runtime):
- Constant folding (PUSH 3; PUSH 4; ADD → PUSH 7)
- Dead store elimination
- Branch folding
- Inlining trivial de funciones cortas
- Promoción de locals a registros en loops

Suma 20-40% extra sobre el speedup base.

### 5. Portable a tres targets

| Target | Backend del emisor | Helpers |
|---|---|---|
| VM C en Pico | Thumb-2 (Cortex-M33) | Funciones C del firmware |
| VM C en host (x64) | x86_64 / arm64 | Funciones C del runtime |
| VM Java en host | Bytecode JVM | Métodos Java del runtime |

El concepto se preserva. Solo cambia el emisor por target.

---

## Speedup esperado

Estimación rápida basada en la mezcla de opcodes:

| Tipo de código | Speedup esperado |
|---|---|
| Fibonacci puro (aritmética + recursión) | 10-20× |
| Json parser (loops + string + alloc moderada) | 2-3× |
| Driver I2C / SPI (dominado por timings físicos) | 1.0-1.2× |
| Programa heap-bound (lista enlazada grande) | 1.3-1.7× |

La estimación de Eduardo de "2-3× en Json es estupendo" es
realista. El JIT serio (LuaJIT trazador, V8) saca un orden de
magnitud más pero a coste enorme de complejidad.

---

## Interop C ↔ BP

### C llamando a BP

Hoy: solo limitado (`bpvm_run` ejecuta `Main()` y para).

Lo que se necesita es una **C API embebible** tipo:

```c
bpvm_value_t result;
bpvm_status_t s = bpvm_call(vm, "MiModulo", "calcular",
                             (bpvm_value_t[]){ bpvm_int(42), bpvm_int(7) },
                             2, &result);
```

Internamente:
1. Encuentra el `function_t` por nombre.
2. Crea o reusa un `bpvm_thread_t`.
3. Empuja args al stack BP.
4. Si bytecode: setea PC, intérprete hasta retorno.
5. Si nativo: `bx` con args en registros.
6. Recoge return.

Complicaciones a resolver:
- Strings/arrays como args (entran al heap BP via `bpvm_alloc_*`)
- Excepciones (status + error opcional)
- Threading (síncrono en el thread C llamante, vs nuevo thread BP)
- Reentrancy (BP→C→BP→C requiere frames mixtos)

**Es uno de los proyectos más útiles del backlog futuro**: abre la
puerta a "BP como motor de scripting embebido en una app C".

### BP llamando a C

Hoy: vía `intrinsic function` registradas estáticamente al
compilar el firmware. Para añadir una nueva hay que recompilar
el firmware.

Tres niveles posibles para permitir extensión dinámica:

1. **Registro dinámico**: el firmware expone
   `bpvm_register_native(name, func_ptr, signature)`. Un programa
   BP usa `extern function nativeFoo(...)` y la VM enlaza por
   nombre.
2. **Módulos nativos cargables**: el `.mod` nativo (de este
   documento) puede contener funciones escritas en C compiladas
   con `arm-none-eabi-gcc`. Equivalente a `.so` de Linux.
3. **FFI inline**: anotación en BP `@native("symbol")` que se
   resuelve contra la tabla de exports del firmware.

BP es estáticamente tipado → la signatura del intrinsic es
conocida en compile-time → el emisor genera la convención de
llamada correcta. **No necesita libffi**.

---

## Coste oculto: el ABI de helpers

La tabla de handlers expuesta es **inmutable** una vez publicada.
Decisiones que tomamos solo una vez:

- **Registros de paso**: ¿(vm, tc, args...) en r0-r3? ¿O vm/tc en
  registros callee-saved permanentes durante la ejecución de
  código nativo?
- **Stack pointer BP**: ¿en memoria (`tc->sp`) o promovido a un
  registro (`r10`)? Si lo metes en registro ganas ciclos pero
  todos los helpers deben respetar la convención.
- **Operandos inmediatos vs del stack BP**: para opcodes con
  argumento estático (`NEW_OBJECT class_id`), en registro. Para
  los que toman del stack BP, el helper los lee como hoy.
- **GC safepoints**: ¿el código nativo cede al GC en cada back-edge
  de loop? ¿En cada call externo? Esto interactúa con el mark-sweep.

Estas decisiones se publican como v1 del ABI. Cambios futuros
requieren bump de versión + recompilación de los `.mod` nativos.

---

## Sistemas reales con esta arquitectura

- **LuaJIT v1 (modo 1)**: opcodes simples inline, opcodes caros
  via helpers C. La "trampoline table" de helpers era pública.
- **CPython 3.11+**: specialized adaptive interpreter; opcodes
  rápidos con asm inline, lentos via call al runtime.
- **Dalvik (Android)**: trace compilation con helpers para todo
  lo que tocara el heap.
- **Wasm interpreters tipo wasm3, wasm-micro-runtime**: la misma
  separación rápido/lento.

La técnica tiene 30+ años y sigue siendo la respuesta correcta
para el rango "2-5× con esfuerzo razonable".

---

## Orden razonable si algún día se decide hacer

1. **Refactor del intérprete**: cada `case` del switch se
   convierte en una función C nombrada en una tabla. **No cambia
   comportamiento**, solo factoriza. Valor por sí solo.
2. **Definir el ABI v1**: convención de llamada, registros
   reservados, lista de helpers exportados, versionado.
3. **Loader de módulos nativos**: parsea formato, aplica
   relocs/GOT, registra `function_t` con `code_kind=1`.
4. **Generador básico**: cubre los 15-20 opcodes triviales
   inline + emite `bl helper` para los demás. Compila Hello.bp,
   carga, ejecuta. Match con la versión bytecode.
5. **Cobertura completa + peephole opts + register promotion**.
6. **C API `bpvm_call`** para invocar BP desde C (puede hacerse
   en paralelo con cualquiera de los pasos anteriores, es
   independiente del código nativo).
7. **Registro dinámico de helpers BP→C** (mismo comentario:
   independiente).

Los pasos 1-3 ya tienen valor sin generar código. El paso 4
da la primera demostración funcional. El paso 5 lo hace
production-ready. Los pasos 6-7 abren la interop bidireccional.

---

## Recap en una frase

**AOT por módulo, helpers para opcodes caros, inline para los
baratos, generación offline en PC, bytecode y nativo conviviendo,
ABI versionado. Ganancia esperada 2-3× típica, hasta 10-20× en
CPU puro, con un esfuerzo medible y escalable.**

Es lo correcto para un sistema embebido con módulos estables y
flujo de desarrollo iterativo en bytecode.

---

*Conversación entre Eduardo y Claude Opus 4.7, 24-mayo-2026.
La idea de los helpers expuestos para opcodes caros es de Eduardo.*
