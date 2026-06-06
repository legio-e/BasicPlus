# Backlog de BasicPlus — pendientes y mejoras

Documento vivo. Actualizar cuando se cierre un item o se descubra uno nuevo.

Convención de IDs:
- **B**# — bug funcional (afecta corrección).
- **L**# — limitación del lenguaje (intencionada o circunstancial).
- **E**# — edge case / diagnóstico mejorable.
- **M**# — mejora de calidad de vida.
- **N**# — descubierto durante una sesión de trabajo (no estaba en la lista inicial).

Estado: `pendiente` / `parcial` / `cerrado` / `descartado`.

---

## 🔴 Bugs en curso (afectan corrección)

### B1 — VM scheduler / GC race bajo contención (parcial, mejor pass-rate)
**Síntoma**: ~5-10 % de corridas con paralelismo intenso (`SyncListTest` y similares)
terminan con corrupción puntual: `INVOKE_VIRTUAL` sobre `null`, "Dirección 0",
opcode `0xA6`, `mutex.lock re-entrada` con valores inverosímiles.

**Estado de la fix** (todo en `VirtualMachine.java`):
- `stopTheWorld` volatile + `gcInProgress` flag + `parkedInHeapAlloc` set
  + `tc.allocAnchor` (resuelve la ventana entre `heapAlloc` y la primera
  escritura).
- Safepoint check entre opcodes en `runOnContext`.
- `synchronized(vmLock)` en torno a los opcodes `NEW_OBJECT`,
  `NEWARRAY`, `NEWARRAY_I8`, `NEWARRAY_I16` para que el ciclo
  completo "alocar → escribir tag → zero fields → push" salga
  publicado antes de soltar el lock.
- `BpThreadFault` (B3) garantiza que el fallo no tumba la VM entera.

Mejora medida (todos los fixes acumulados):
- `SyncTest`: ~75 % → ~90 % pass-rate (5000 iters × 2 threads).
- `SyncListTest`: ~80 % → ~93 % pass-rate (1000 producers + 2 consumers).
- `modpropsync.bp`: ~95 % pass-rate.

**Lo que falta** (residual ≤10 % → ~40 % en runs frescos tras B3 v2/v3):
1. Builtins que alocan y luego pushean siguen liberando `vmLock` entre
   la alocación y el push: `allocVmString`, `allocVmRefArray`, y
   varios builtins de stdlib (`__newRefArray`, etc., ≈15 callers).
   Probé envolver `GROW_REF_ARRAY` en `vmLock` y empeoró ligeramente
   (24/30 → 22/30), probablemente por contención excesiva. Hace
   falta una estrategia diferente — quizá usar la ancla
   `tc.allocAnchor` también para builtins, no solo para `heapAlloc`.
2. Investigación de N1 (modelo de memoria `mem[]` sin volátil) podría
   estar contribuyendo: aunque las escrituras están bajo `vmLock`,
   las lecturas fuera de `vmLock` (`GET_FIELD`, `ALOAD`, etc.) pueden
   ver bytes desactualizados en multi-core.

### B1 residual — caracterización con instrumentación (parcial)
**Estado**: instrumentación añadida, fail rate empeoró tras B3 v2/v3.
Investigación pendiente.

**Instrumentación añadida** (`VirtualMachine.dumpFault`):
Activable con `-Dbpvm.b1.diag=1` o env `BPVM_B1_DIAG=1`. Vuelca al
stderr al primer fallo: thread fallado (pc, sp, bp, status,
blockedOnMutex, ehHandlerPc, allocAnchor) + stack trace de la
excepción + snapshot de TODOS los threads + runQueue + mutexes
(owner + waiters). Hooks cableados en los catch de `BpThreadFault`
y `RuntimeException` del `WorkerLoop`.

**Fail rate medido** (SyncListTest, 10 runs frescos tras B3 v2/v3):
- **40 % fail rate** (6 OK / 4 FAIL) — peor que el ~10 % previo.
- Hipótesis: B3 v3 (que envuelve TODO el while del intérprete con
  try-catch BpExceptionPending) puede haber introducido nuevas
  ventanas de race. Verificar revirtiendo a antes del commit
  `9857730` y medir baseline.

**Firmas observadas**:
1. **"HALT en thread no-main"**: tid=1 (Consumer) reporta haber
   ejecutado opcode `0x00` (HALT) en PC ≈ 35000. Al mismo tiempo, su
   `status=BLOCKED_MUTEX, blockedOnMutex=0`. Sugiere PC corrupto —
   probablemente el PC saltó a la posición de un byte 0x00 en el
   stack o heap. Stack trace dice
   `runOnContext(VirtualMachine.java:1555)` (case 0x00).
2. **"se esperaban N items / suma inesperada"**: la firma "clásica"
   B1 — items perdidos o duplicados en SyncList bajo contención.
3. **"BpExceptionPending(ref=...)" filtrada al stdout/stderr**: el
   mensaje INTERNO de mi excepción Java se cuela al output como
   string. Indica que se lanzó BpExceptionPending por un thread sin
   handler, llegó a BpThreadFault, y `readRuntimeErrorMsg(v)` no
   pudo leer el field `msg` (objeto BP mal formado o ref inválida).

**Próximo paso (no en esta sesión)**: re-correr instrumentado con
diff de baseline pre/post B3 v3 para confirmar si el path nuevo del
unwind (catch BpExceptionPending dentro del while) es la fuente.
Si lo confirma, considerar: a) sólo envolver opcodes que llaman a
throwBpRuntimeError directamente, no todo el while; b) cachear el
class_ptr de RuntimeError al cargar el módulo en lugar de buscarlo
por nombre en cada throw (pero eso requeriría persistencia del
class_ptr).

### B1 — caracterización v2 + decisión (2026-06-06)
**Acotado con barrido paramétrico (`--workers` × heap chico/grande vía `--config`)
+ dumps `BPVM_B1_DIAG=1`:**
- **GC-INDEPENDIENTE**: con heap de 64 MB y **0 GCs medidos**, w2 sigue fallando
  ~25 % y w4 100 % (misma firma). Refuta la hipótesis "es el GC". El heap chico
  (100 % fallo) solo *amplifica* (GC constante añade un use-after-free con firma
  distinta `Dirección X no cae en data block`).
- **Es PARALELISMO REAL**: w1=**0 %** siempre; w2≈25 %; w4=**100 %**. Escala con
  workers físicos. Con 1 worker (cooperativo, = modelo del device) NO aparece.
- **Firma**: corrupción del estado de ejecución de un thread (`pc`/`bp` → "HALT en
  PC basura", "INVOKE_VIRTUAL null receiver"). Dump GC-free: mutex `owner=2` con
  tid=2 RUNNABLE y tid=1 RUNNING petando → race worker↔worker, no del GC.
- **Descartados LEYENDO el código (todos correctos)**: claim del scheduler
  atómico (`pickNextRunnableTc`+`status=RUNNING` bajo el mismo `vmLock`);
  `currentThread`/`currentThreadId` compartidos son solo inspección de debugger
  (la ejecución usa variantes per-tc); `MUTEX_LOCK` cede por-opcode
  (`yieldRequested` se chequea al inicio de cada opcode) + hand-off directo
  correcto; guardado de `tc.pc/sp/bp/cs` en el `finally` de `runOnContext` en
  TODA salida. ⇒ el bug es una data race sutil en `mem[]` compartido bajo
  paralelismo real, NO en los sitios obvios.

**Decisión (usuario, 2026-06-06)**: B1 solo muerde con paralelismo REAL
(workers≥2 en el host de desarrollo, o el futuro **dual-core RP2350**). El device
single-core es **inmune** (= caso w1=0 %; el `heap_stress` cross-thread lo
confirmó en placa). Por tanto:
- **La VM-Java pasa a `numWorkers=1` por defecto** (igual que el device → segura
  de fábrica). El SMP queda **opt-in** (`--workers=N`), documentado como
  experimental.
- **El fix de B1 se ACOPLA al dual-core (v2)**: cuando se enciendan los 2 núcleos
  del RP2350 habrá que resolverlo (probable foco: protección de `mem[]` o de la
  pila por-thread bajo ejecución simultánea). Hasta entonces, bancado y bien
  caracterizado. Repros: `samples/synclisttest.bp` con `--workers=2/4` + configs
  de heap en `samples/holes/` (regenerables).

### B3 v2 — `try/catch e: RuntimeError` para errores de mutex (cerrado)
**Estado**: cerrado. El código BP ya atrapa errores nativos de mutex.

**Implementado**:
1. **ModWriter** soporta `exportDataSymbol(name)`: data symbols con nombre
   se serializan al final de la sección exports (subsección opcional;
   compat con .mods viejos por boundary del `exportsSize`).
2. **MivmEmitter** llama `w.exportDataSymbol("RuntimeError")` al
   sintetizar la clase, así cada `.mod` expone su descriptor.
3. **ModuleManager** lee la subsección y registra el descriptor en
   `globalSymbolTable` con dirección `codeStart + csOffset` (CS-relative
   negativo; misma convención que NEW_OBJECT). Nuevo
   `resolveExportInModule(cs, name)` para consultarlo desde la VM.
4. **VirtualMachine** nueva clase `BpExceptionPending` (señal interna) +
   helper `throwBpRuntimeError(tc, msg)` que:
   - Resuelve el class_ptr de RuntimeError del módulo actual.
   - Aloja BP-string con el mensaje (allocVmString).
   - Aloja BP-RuntimeError (heapAlloc + write class_ptr + zero fields).
   - Setea `obj.msg := stringRef` (slot 0).
   - Empuja el ref a tc.sp.
   - Lanza BpExceptionPending.
5. **Dispatcher de CALL_BUILTIN** (case 0x5A) cubre con `try/catch` la
   excepción y ejecuta el unwind exacto del opcode THROW (0x5D) sobre
   las locales del intérprete. Si ningún handler atrapa, convierte a
   BpThreadFault con el mensaje original (path "uncaught").
6. **Mutex builtins** (MUTEX_LOCK/MUTEX_UNLOCK) cambian
   `throw new BpThreadFault(...)` por `throwBpRuntimeError(tc, ...)`.

**Verificado**: sample `catchmutex.bp` con dos casos:
- `m.lock()` reentrant → catch atrapa, `e.msg = "mutex.lock: re-entrada
  por mismo thread tid=0 ..."`, programa CONTINÚA.
- `m.unlock()` sin posesión → mismo patrón, programa CONTINÚA.

**Bug encontrado y arreglado durante la implementación**: el loader
inicialmente sumaba `dataStart + csOffset` para calcular la dirección
absoluta del descriptor, pero la convención de la VM (NEW_OBJECT,
catch handlers) es `codeStart + csOffset` (csOffset negativo). Esto
hacía que `thrownClass` y `ehExpectedClass` divergieran por
`dataSize` bytes y nunca matcheaba el catch.

**Fuera de scope (posibles futuros B3.v3)**:
- Aplicar el mismo patrón al resto de errores nativos (sandbox path,
  cast narrow fuera de rango, división por cero, etc.). Hoy varios
  todavía van por `BpThreadFault` o `RuntimeException`.
- Stack-trace BP atrapable (`e.stackTrace`).

---

## 🟡 Limitaciones del lenguaje (pre-existentes)

### L1 — `any → primitive` permitido (cerrado)
**Estado**: cerrado. `PrimitiveType.isAssignableFrom(AnyType)` ahora
devuelve `true`. Verificado con `List<integer>`, `List<float>`,
`List<boolean>`, `List<string>`. Si el `any` en runtime NO es del tipo
esperado, se leen bytes basura (no hay check de runtime — sigue siendo
asignación estructural).

**Limitación**: para usar el valor directamente en una expresión
(`l.get(0) + l.get(1)`), hace falta asignar primero a una variable
tipada — `any + any` sigue rechazándose porque el operando no se
puede inferir bidireccionalmente.

### L2 — Exportación de clases en `.bpi` (cerrado, v1)
**Estado**: cerrado v1. Cross-module funciona para:
- construcción (`Module.ClassName(args)` via factoría sintética
  `__cls_new_<Cls>` en el módulo dueño — CALL_EXT existente, sin opcode nuevo).
- llamadas a métodos públicos (`instance.method(args)` via INVOKE_VIRTUAL con
  vtSlot precalculado del `.bpi`; el runtime resuelve el CS por receiver).
- read/write de properties públicas (incluido `sync property` — el lock vive
  en el módulo dueño y se mantiene atómico cross-module).

Sample: `samples/l2lib.bp` + `samples/l2app.bp`.

**L2 v2 parcial (cerrado)** — herencia cross-module a nivel typecheck:
- Parser acepta `class X extends Mod.Y` y tipos cualificados `var c: Mod.T`.
- `SemanticAnalyzer.resolveBaseClassName` resuelve `Mod.Foo` lookup en
  el `ImportedNamespaceSymbol.classes` del alias `Mod`.
- `resolveNamedType` reconoce nombres dotted en cualquier posición de tipo.
- `MivmEmitter.emitClassDef`: si `baseClass.isExternal`, pasa null al
  ModWriter (no propaga herencia local). El descriptor del child sale con
  parentOff=0 a nivel runtime.
- `super(...)` sobre padre cross-module bloqueado con mensaje claro:
  "no soportado en L2 v2".
- Sample: `samples/l2v2app.bp` — `class FastCounter extends L2Lib.Counter`,
  asignación polimórfica `var c: L2Lib.Counter := fc` typecheckea,
  método local de FastCounter ejecuta. OK.

**Lo que funciona en v2**: extends cross-module a nivel typecheck +
asignación polimórfica (FastCounter → L2Lib.Counter) + métodos LOCALES
de la clase hija + redeclaración de métodos.

**Limitaciones documentadas (L2 v3)**:
- Vtable inheritance cross-module: hoy el child no hereda los métodos
  del padre cross-module. Cualquier `fc.parentMethod()` falla (semantic
  ya lo reporta como "no tiene miembro"). Para soportarlo haría falta:
  a) emitir trampolines en el child que hagan CALL_EXT al método del
  parent, o b) cambiar runtime para que INVOKE_VIRTUAL caiga al
  parent's vtable cuando el slot del child no exista.
- `super(...)` cross-module: idem.
- `instanceof` cross-module runtime: el parentOff = 0 en el descriptor
  del child rompe `isDescendantOf` en la VM.
- Static members (consts / vars de clase): siguen ignorados en
  `extractClass`.
- Static methods públicos cross-module.
- Class types cross-module en signatures de método (e.g. parámetro
  `OtraMod.OtraClase`): no se resuelve hoy.

### L3 — Métodos de clase sin forward references
**Impacto**: dos métodos de la misma clase no pueden llamarse mutuamente si
están declarados en cualquier orden. Encontrado al portar `Json.bp` — solución
fue convertirlos en funciones módulo.

**Causa**: resolución de slots eager en `MivmEmitter.emitClassDef`.

### L4 — `get` y `set` son palabras reservadas
**Impacto**: no puedes nombrar un método de usuario `get` ni `set`. Bloqueó la
implementación natural de `JsonArray.get(idx)` — tuvimos que renombrar a `at`.

**Fix**: hacer que `get`/`set` sean contextuales (sólo reservadas dentro de
`property`). Cambio en el lexer + parser.

### L5 — Sin expresiones multi-línea
**Impacto**: una expresión partida en varias líneas falla en el parser. Forzosamente
una línea. Pena con `throw RuntimeError("mensaje largo\n con contexto")`.

### L6 — `static property` de clase no soportada
**Estado**: marcado como TODO en `MivmEmitter`.
```
errors.add("property estática no soportada todavía: " + ...);
```

### L7 — `owner`/`final` no aplican a property de módulo
**Decisión**: documentada. `owner` requiere semántica de FREE_REF cascada que
sólo aplica a campos de instancia; `final` aplica a herencia que módulos no
tienen. Reabrible si surge caso de uso.

### L9 — `Mutex` no reentrante
**Decisión por diseño**: documentada en el manual. Para algoritmos que requieren
re-entrada, el usuario debe usar un patrón diferente (e.g. flag + condvar).

### L10 — Tipos enteros estrechos (cerrado v1)
**Estado**: cerrado v1 — locales + casts. Faltan los layouts compactos
(narrow arrays + narrow globals) que se anotan como follow-up. El lenguaje hoy sólo reconoce `integer`, `float`,
`string`, `boolean` (en `Lexer.KEYWORDS` y `BpType.PrimitiveType.Kind`).

**Lo que ya existe a bajo nivel** (no expuesto a BP source):
- Opcodes: `NEWARRAY_I8`, `NEWARRAY_I16`, `ALOAD_I8/U8/I16/U16`,
  `ASTORE_I8/I16`.
- Globals: `GET_GLOBAL_I8`, `GET_GLOBAL_U8`, `GET_GLOBAL_I16`, etc.
- `ModWriter.addConstantInt8`, `addConstantInt16`.

**Decisión semántica clave** (del usuario):
- **Load**: SIEMPRE promociona a `integer` (i32) en la pila. La VM
  internamente sólo opera con i32, así que `byte + byte` es i32, igual
  que `byte + integer`. No hay "tipo aritmético byte". Las operaciones
  intermedias usan i32 puro.
- **Store**: requiere **conversión explícita** del programador
  cuando el valor de origen puede no caber. `var b: byte := 300` no es
  válido — el usuario tiene que escribir algo como `b := byte(300)` o
  `b := 300 as byte` para señalar que sabe que va a truncar. Sin cast
  explícito, error semántico "posible pérdida de información al
  asignar integer a byte".

**Lo que falta** (5 capas):
1. **Lexer**: añadir entradas a `KEYWORDS` con los nombres canónicos.
   Propuesta: `byte` (= uint8), `int8` (i8 signed), `int16` (i16 signed),
   `word` (= uint16), `short` (alias de `int16`).
2. **BpType**: extender `Kind` con `INT8`, `UINT8`, `INT16`, `UINT16`.
   Reglas: load promociona a `INTEGER`; store desde integer/wider
   requiere cast explícito.
3. **Sintaxis del cast explícito**: decidir entre `byte(x)` (call-like)
   o `x as byte` (operador postfix). El lexer/parser lo introduce.
4. **MivmEmitter**:
   - Load: usar `ALOAD_I8`/`U8`/`I16`/`U16` o `GET_GLOBAL_I8`/etc. — el
     opcode ya sign-/zero-extiende a i32. Coherente con la regla.
   - Store: el cast explícito emite truncación (AND con 0xFF/0xFFFF
     según firma) seguido del `ASTORE_I8`/`I16` o `SET_GLOBAL_I8`/etc.
5. **Check de rango estático para literales**: `var b: byte := 300`
   (sin cast) → error. `var b: byte := 250` (en rango) → permitido sin
   cast porque no hay pérdida. Cast explícito siempre acepta.
6. **Manual** §9: tabla de tipos primitivos completa + sintaxis del cast.

**Esfuerzo**: medio. La maquinaria de la VM ya está. Lo más fiddly es
la sintaxis del cast (decisión de diseño) y el typecheck del store
(detectar cuándo el origen "definitivamente cabe" vs cuándo no).

**Implementado en v1**:
- Lexer + TokenType: 5 keywords nuevos (`byte`, `int8`, `word`, `int16`,
  `short`). `short` = alias gramatical de `int16` (mismo Kind tras parser).
- `BpType.PrimitiveType.Kind`: 4 valores nuevos (INT8, UINT8, INT16,
  UINT16). Helpers `isNarrowInteger()`, `isIntegerLike()`, `rangeMin/Max()`.
- `isAssignableFrom`: load auto-promociona narrow → INTEGER. Store
  INTEGER → narrow está bloqueado (necesita cast).
- Cast functions: `byte(x)`/`int8(x)`/`word(x)`/`int16(x)`/`short(x)`
  como builtins registrados en `addBuiltin`. El emisor especial-casea
  estos nombres y emite `I32_TO_{U8,I8,U16,I16}` (en vez de CALL_BUILTIN)
  que hace check de rango en runtime → RuntimeError si no cabe.
- **Check estático de literal en rango**: `var b: byte := 250` ✓ ;
  `var b: byte := 300` → error "literal 300 fuera del rango de byte
  (0..255)". Aplica a IntLitExpr (con ParenExpr y unario `-` envueltos).
- **Mensajes claros**: si el LHS narrow tiene un RHS no-literal sin
  cast → "asignación a 'byte' requiere cast explícito: byte(<valor>)".
- **Pre-check de literales en casts**: `byte(99999)` → error
  compile-time (no espera al runtime).
- Operadores `mod`/`& | xor shl shr` aceptan narrow (load auto-promote
  → i32 → resultado integer).
- `print` reconoce narrow → usa `PRINT_NONL` (el valor ya está extendido
  a i32 en pila).
- Sample: `narrowtypes.bp` (happy path) + `narrowtypes_errors.bp` (7
  errores esperados) + `narrowtypes_runtime.bp` (RuntimeError en
  I32_TO_U8 con valor variable fuera de rango).

**Limitaciones documentadas (follow-up)**:
- **Locales narrow viven como i32 en pila** — sin ahorro de memoria
  real. El valor narrow es semántico (validación + intent del API).
- **Narrow arrays con layout compacto pendiente**. Hoy `var arr: byte[5]`
  se acepta sintácticamente, pero el emitter usa `NEWARRAY` (4 bytes por
  elemento) no `NEWARRAY_I8`. Cuando se implemente, los elementos pasarán
  por `ALOAD_U8`/`ASTORE_I8` etc. (la VM ya los tiene).
- **Narrow globals pendiente**. Mismo motivo — el data block tendría
  `addConstantInt8`/`Int16` y los accesores serían `GET_GLOBAL_I8`/etc.

---

## ✅ Cerrado en sesiones recientes — anotaciones

### BpVM.cfg — fichero de configuración JSON (cerrado)
Soporta:
- `memorySize` (bytes totales del array `memory`).
- `stackBase` (offset donde empiezan los stacks; se deriva
  `memorySize/2` si sólo está `memorySize`).
- `stdlibDir` (path donde la VM busca los `.mod` stdlib como fallback
  cuando un import no resuelve desde el cwd).

CLI: `bpgenvm --config <ruta>` o autodescubrimiento en cwd y junto al
`.mod`. `bpgenvm --no-config` desactiva la autodiscovery.

El BpIde llama a `VmConfig.loadDefaultFor(modPath)` antes de Run/Debug
y aplica `memorySize`/`stackBase` al constructor del `VirtualMachine`
y `stdlibDir` al `ModuleManager`.

Implementación: nueva clase `edu.bpgenvm.config.VmConfig` con parser
JSON minimalista (objetos planos, strings, números enteros, bool, null,
comentarios `//`).

---

## 🔵 Cambios arquitectónicos planificados

### A1 — Separar IDE y VM en dos procesos con canal bidireccional (pendiente)
**Motivación**: el destino final es un dispositivo remoto que ejecuta
la VM mientras el IDE vive en el PC del programador. Hacer el split
ahora, con los dos lados en la misma máquina, valida el protocolo
antes de añadir el cable. Ver `docs/PHILOSOPHY.md` §"IDE y VM
separados — y eventualmente remotos".

**Estado actual**: la VM se invoca in-process desde
`BpIde/.../FrmMain.java`. El debugger acopla las dos capas con
llamadas Java↔Java (`DebugSession` ↔ `DebugContext`).

**Diseño a decidir** (preguntas abiertas):
1. **Transporte**: ¿TCP sockets, UNIX domain sockets, o pipes
   stdio? TCP escala a remoto sin tocar nada; stdio es lo más
   simple para local; UDS está en medio.
2. **Protocolo**: ¿line-based (texto JSON por línea, fácil de
   depurar), binario length-prefixed (más eficiente, peor de
   depurar), o algo establecido (gRPC, MessagePack, DAP)?
3. **Quién arranca a quién**: ¿el IDE lanza la VM como subprocess
   y se conecta? ¿O la VM corre como daemon y el IDE se conecta
   cuando arranca?
4. **Multiplexación**: ¿una sola conexión que lleva control +
   eventos + stdout? ¿O dos sockets (control, output)?
5. **Lifecycle de la VM sin IDE**: la VM debe poder ejecutar un
   programa "headless" sin nadie conectado (un dispositivo
   desplegado). Hot-attach/detach del IDE es deseable.

**Sub-tareas previstas** (no necesariamente en este orden):
- **A1.1** — Decisión de transporte + protocolo (cerrado).
  TCP sockets + JSON por línea + IDE lanza VM como subproceso.
- **A1.2** — Refactor de `print` en la VM (cerrado).
  Nuevo `OutputSink` interface + `StdoutSink` default; VM expone
  `setProgramOut()`. Los opcodes PRINT_CHAR/STRING/NONL/STR_NONL/NL
  ahora van por el sink. Tests en `OutputSinkTest`. Regresiones OK.
- **A1.3** — Refactor de hooks del debugger (cerrado).
  Nuevo paquete `edu.bpgenvm.vm.debug` con `DebugEvent` (abstracta) +
  `PausedEvent` / `ResumedEvent` / `ExitedEvent` / `ExceptionEvent` —
  POJOs serializables. `DebugListener.onEvent(DebugEvent)` reemplaza al
  antiguo `DebugSession.Listener.onPaused/onResumed`. `DebugSession`
  emite eventos; `DebugContext` (las queries de memoria) sigue accesible
  vía `DebugSession.currentContext()` hasta que A1.4 lo sustituya por RPC.
- **A1.4** — Servidor en la VM (cerrado).
  - **A1.4.a** — `DebugController` mueve la lógica de pausa al VM
    (breakpoints, mode, queue de comandos). El IDE `DebugSession`
    queda como wrapper fino. `DebugControllerTest` con 5 tests.
  - **A1.4.b** — `DebugServer` con `--listen <port>` (+ `--wait-client`).
    Encoder JSON de los DebugEvent + parser de comandos planos
    (`edu.bpgenvm.util.Json`). `SocketSink` redirige los PRINT_* al
    canal. Verificado end-to-end con cliente Python: handshake →
    paused → continue → prints. `DebugServerTest` con 2 tests
    integración (incluye port reservation).
- **A1.5** — Cliente en el IDE (parcial: cerrado para Run, pendiente para Debug).
  - Hecho: `com.mycompany.bpide.VmClient` lanza `bpgenvm` como
    subproceso con `--listen <freePort> --wait-client`, conecta por
    TCP, parsea las líneas JSON entrantes en `DebugEvent`s y los
    inyecta al `DebugListener` configurado. Envía comandos como
    líneas JSON (`setBreakpoint`, `continue`, step*, `stop`).
    Localiza el jar de `bpgenvm` vía `ProtectionDomain` para
    construir el classpath del subproceso (funciona en Maven, IDE
    de Netbeans y fat jar). `awaitTermination` / `waitForExit` para
    que el SwingWorker bloquee hasta el fin del subproceso.
  - `doRun` del `FrmMain` ahora pasa por `VmClient`. El Run del
    IDE arranca la VM en proceso separado; los `print` vienen
    streamed a la consola del IDE. Verificado con `VmClientSmoke`
    (clase main ejecutable + sample MoveTest.mod): handshake →
    auto-continue → 36 mensajes print recibidos.
  - Pendiente: `doDebug` sigue usando la VM in-process porque las
    queries de variables/properties/locales aún acceden al
    `DebugContext` live. A1.6 las cablea por RPC y entonces se
    puede rewire también doDebug.
- **A1.6** — Queries por RPC (cerrado).
  - `Json.parse()` recursivo en `edu.bpgenvm.util.Json`
    (objetos + arrays + primitivos). `parseFlatObject` se mantiene
    como wrapper.
  - Request/response sobre el mismo socket: `{"req":..., "requestId":N}`
    desde el cliente, `{"resp":..., "requestId":N, ...}` desde el
    server. `error` como `resp` especial cuando algo falla.
  - Queries soportadas: `getLocals`, `stackFrames`, `moduleProperties`,
    `readInt`, `readString`. Las primeras tres requieren VM pausada;
    si no, devuelven error.
  - Cliente: `VmClient.sendRequest()` con `AtomicLong` para IDs y
    `Map<Long, CompletableFuture>` para correlar. Métodos
    convenientes `getLocals(timeout)`, `getStackFrames(timeout)`,
    `getModuleProperties(timeout)` con timeout configurable.
- **A1.7** — Pulido del wire (cerrado).
  - **Chunking de prints**: `SocketSink` consolida
    `writeText`/`writeChar`/`newline` en un solo mensaje JSON por
    línea. Flush forzado a 4 KiB para evitar acumular en programas
    sin saltos de línea. Test medido: 36 mensajes → 18 sobre
    MoveTest.mod.
  - **ExitedEvent**: `Main.java` emite `{"type":"exited","exitCode":0,
    "reason":"main returned"}` (o `exitCode=1` con la excepción
    correspondiente) al terminar `vm.run()`. El cliente sabe cuándo
    cerrar la sesión sin polling del subproceso.
  - **ExceptionEvent**: el `WorkerLoop` emite
    `{"type":"exception","tid":N,"message":"..."}` cuando un
    `BpThreadFault` o `RuntimeException` no atrapada llega al worker.
    `VirtualMachine.setDebugEventListener` cablea el sink; Main lo
    apunta a `controller.emitEvent`.
- **A1.8** — Smoke tests (cerrado).
  - JUnit en `DebugServerTest` (5 tests): handshake, paused JSON,
    chunking de prints, error de getLocals sin pausa, ExitedEvent.
  - Smoke Python: arranca VM real con MoveTest.mod, llega al
    paused, ejercita las 3 queries (getLocals/stackFrames/
    moduleProperties), continúa, recibe los 18 prints + exited.
  - Pendiente: smoke end-to-end con la GUI del IDE (requiere
    interacción manual; no se automatiza offline).
- **A1.9** — Rewire de `doDebug` (cerrado).
  - `DebugSession` ya no envuelve un `DebugController` in-process —
    es un wrapper de `VmClient` con `attach()`/`detach()` por sesión.
    `toggleBreakpoint` actualiza la ObservableList (UI) y replica al
    wire vía `setBreakpoint`. `sendCommand` / `getLocals` /
    `getStackFrames` / `getModuleProperties` forwarded al VmClient.
  - `FrmMain.onDebugPaused` cambia firma — recibe `PausedEvent` +
    snapshots de locals/frames/props ya cargados. Las queries se
    hacen en el listener del VmClient (executor dedicado), NO en EDT.
  - **Bug detectado y arreglado en `VmClient`**: el listener corría
    inline en el reader thread → deadlock cuando hacía una query
    síncrona (`sendRequest.future.get()` esperaba una respuesta que
    el mismo thread bloqueado tendría que leer). Solución: dispatch
    de listeners a un `Executor` single-thread (preserva el orden).
  - `doDebug` arranca un `VmClient` con `--listen --wait-client`,
    hace `debug.attach(client)`, espera `client.waitForExit()`.
  - **DebugSessionSmoke**: clase main de validación. Verificado en
    MoveTest.bp con BP en línea 22 → pause 1 (line=14, locals=0,
    frames=2) → pause 2 (line=22, locals=9, frames=2) → exit.

**Lo que queda en A1 / arquitectura IDE↔VM**:
- describePc remoto: el call-stack del debugger muestra `PC=<n>`
  porque el `ModuleManager` ya no vive en el IDE. Si se quiere
  el nombre del símbolo, añadir un `req: describePc(pc)` al wire.
- Smoke GUI manual: arrancar el IDE, abrir un proyecto, Run/Debug
  y comprobar que la consola muestra los `print` y los paneles
  Variables/Stack/Properties se rellenan al pausar.
- Stop real: hoy `STOP` no termina la sesión — sólo libera el
  step. Falta una señal de shutdown al `vm.run()` que detenga
  todos los workers.

---

## 🔵 A2 — VM con workdir + transferencia de ficheros

### A2.1 — workdir de la VM + sandbox de paths (cerrado)
- Nuevo flag `--workdir <dir>` en `Main`. Cuando está activo:
  - `ModuleManager.workdir` se fija al path (absoluto, normalizado).
  - Resolución de módulos prefiere `workdir/<basename>` antes que
    `modulePaths` y `stdlibDir`.
  - Los builtins de IO (`readFile`/`writeFile`/`appendFile`/
    `fileExists`/`listDir`/`mkdir`/`rmdir`/`removeFile`/`rename`/
    `copyFile`/`fileSize`/`isDirectory`/`lastModified`/`pathAbsolute`)
    pasan por `sandboxPath()` que llama a `resolveInWorkdir`.
  - Path absoluto o traversal con `..` que escape del workdir →
    `BpThreadFault("sandbox: path escapa del workdir...")`.
- Sin `--workdir` el comportamiento es el legacy (la VM ve todo el
  filesystem).

### A2.2 — Transferencia de ficheros sobre el wire (cerrado)
- Nuevos requests en el wire (workdir-relative, base64 para binarios):
  - `uploadFile {path, data}` → `{size}`
  - `downloadFile {path}` → `{size, data}`
  - `listFiles {path}` → `{files:[{name, size, isDirectory}]}`
  - `deleteFile {path}` → ok
  - `mkdir {path}` → ok
- Server route via `ModuleManager.resolveInWorkdir`; si la VM no
  tiene workdir, error: "VM sin workdir; arrancar con --workdir".
- `VmClient` con helpers tipados: `uploadFile(Path, String, ms)`,
  `downloadFile`, `listFiles` → `List<RemoteFile>`, etc.

### A2.3 — Modo daemon + comando runModule (cerrado)
- Si `--listen` se pasa SIN un fichero, la VM arranca en modo
  daemon: monta server + workdir y espera a que el cliente envíe
  `runModule {module: "App.mod"}` (path relativo al workdir).
- `DebugServer.awaitRunModule(timeoutMs)` bloquea Main hasta recibir
  el comando. El path se resuelve contra el workdir y se carga
  como root module.
- Tras el `vm.run()` la VM emite ExitedEvent y termina. Una sola
  ejecución por proceso (el reset multi-run es futuro).

### A2.4 — Smoke end-to-end (cerrado)
- `WorkdirSmoke` (clase main): crea workdir temporal → spawn VM
  daemon → upload `MoveTest.mod` (3168 bytes) → listFiles confirma
  → runModule → recibe 18 prints + ExitedEvent → cleanup. Pasa.
- Valida que el flow "IDE compila localmente → sube por wire →
  manda runModule" funciona sin necesidad de que IDE y VM
  compartan filesystem.

### A2.5 — IDE usa el flow daemon + workdir + upload (cerrado)
- `FrmMain.runOnVmRemote(outDir, mainModName, pauseInitially, publish)`:
  1) crea workdir temporal (`bpide-vm-XXXX/`);
  2) `VmClient.startDaemon(workdir, waitClient=true)`;
  3) `uploadAppArtifacts`: sube `.mod` + `.bpi` + `.dbg` del outDir local;
  4) si pause inicial: `debug.attach(client)`;
  5) `client.runModule(<main>.mod)`; espera `waitForExit`;
  6) `deleteRecursively(workdir)`.
- `doRun` → `runOnVmRemote(..., pauseInitially=false, ...)` (auto-continue al primer paused).
- `doDebug` → `runOnVmRemote(..., pauseInitially=true, ...)` (attach DebugSession; usuario controla con Continue/Step).
- El stdlib NO se sube — vive preinstalado en el dispositivo, accesible via `--stdlibDir` que el lado VM lee de su propio `BpVM.cfg`.
- Hoy localhost; cambiar a IP remota es trivial cuando A2.6 esté hecho.

**Lo que queda en A2**:
- **A2.6** — Configurar host/puerto remoto en el IDE (hoy hard-coded a "localhost"). Una preferencia tipo "VM endpoint" en BpIde, leído de un fichero o de un menú. Cuando esté, **NO se toca código** para apuntar a un dispositivo: sólo se cambia esa preferencia.
- Multi-run en el mismo daemon: hoy la VM termina tras un
  runModule. Para soportar "Run otra vez" sin matar el proceso,
  habría que reiniciar memoria/heap/threads.
- Aprovisionamiento del dispositivo (subir/actualizar stdlib,
  configurar BpVM.cfg) — herramienta separada del IDE de usuario.

**Compatibilidad**: durante el desarrollo conviene mantener un
modo "in-process" funcional para no romper el flujo de trabajo,
o tener una bandera que conmute entre los dos. Una vez el modo
"out-of-process" sea estable, el in-process se puede retirar.

---

## 🟠 Hallazgos de la sesión actual

### N1 — Modelo de memoria Java en `mem[]` (investigado, no es la causa)
**Hipótesis inicial**: `mem` (byte[]) sin volátil → lecturas fuera de
`synchronized(vmLock)` pueden ver bytes desactualizados, contribuyendo al
residual de B1.

**Test ejecutado**: añadir un `synchronized(vmLock) {}` vacío al inicio
de cada iteración del intérprete (full membar). Resultado:
SyncListTest 23/30 vs baseline 25/30 — dentro del ruido. **JMM no es
la causa dominante del residual.**

**Test diagnóstico clave**: `bpgenvm --workers=1 SyncListTest.mod`
ejecutado 30 veces → **30/30 OK**. Con un solo worker el residual
desaparece por completo. Esto confirma que el residual es **100%
concurrencia**, pero NO se manifiesta como problema de visibilidad
de memoria — los `synchronized(vmLock)` ya establecidos en los caminos
críticos (heapAlloc + MUTEX_LOCK/UNLOCK + YIELD case + safepoint
mediante stopTheWorld) son suficientes para JMM.

**Conclusión**: el ~10-15% residual de B1 viene de una race de control
(scheduler, hand-off de mutex, parkedInHeapAlloc, etc.) y NO de JMM.
Para perseguirlo hace falta más diagnóstico (assertions internos en la
VM que detecten estados imposibles y vuelquen tc.id, status, runQueue
en el momento del fallo). Pendiente como sub-tarea.

### N2 — `JavaMutex.ownerTid` no volátil
Hoy todas las lecturas son bajo `vmLock`, así que está OK por ahora. Pero
documentar la convención: cualquier acceso a `JavaMutex.{ownerTid, waiters}`
DEBE ir bajo `vmLock` o llevar acquire/release explícito.

### N3 — Side-effects de B2 semántico (cerrado, + 4 mejoras de paso)
**Estado**: cerrado tras revisión completa. Hallazgos y fixes:
- **N3a**: `return value` en función/setter void se aceptaba silenciosamente.
  Arreglado: ahora emite error claro.
- **N3b**: `RuntimeError.msg` no typecheaba (`catch e: RuntimeError ...
  print e.msg` daba "no tiene miembro 'msg'"). El builtin scope declaraba
  el constructor pero no el field. Arreglado: añadido `VarSymbol msg` a
  `rt.instanceMembers`.
- **N3c**: clases con sync property y sin ctor de usuario no inicializaban
  `__syncMutex` (synth ctor emitido pero no llamado). Arreglado: el synth
  ctor ahora se registra como `cls.constructor` para que `findConstructor()`
  lo encuentre.
- **N3d**: `throw` dentro de un sync getter/setter dejaba el mutex
  tomado (mi fix B2 sólo cubría `return`). Arreglado: `emitThrowStmt`
  emite unlock antes del `THROW` si está en un accesor sync.
- `super.foo()` y `field := value` dentro de accesores funcionan
  correctamente sin cambios.

### N4 — `currentTcLocal` ThreadLocal nunca se limpia
**Coste**: una referencia al último `tc` por worker Java. No hay leak grave,
pero es feo. Añadir `currentTcLocal.remove()` al final de `WorkerLoop.run()`.

### N5 — `SyncList.popBlocking()` (cerrado)
Disponible en el SyncList sintetizado. Semántica: espera hasta haber un
item, lo extrae y lo devuelve (nunca null).

Implementación v1: loop con `lock → check size → extract|unlock+sleep(1)+retry`.
Inicialmente probé con `yield()` pero el consumer no se despeja del
scheduler — re-encolaba enseguida y monopolizaba la CPU mientras el
productor estaba en `BLOCKED_SLEEP`, así que el productor nunca llegaba a
ejecutar. `sleep(1)` cambia el estado del thread a `BLOCKED_SLEEP` (con
wakeAt ≈ now+1ms) y libera el slot real.

Sample: `samples/n5_popblocking.bp` (productor lento + consumer
blocking — 8 items con 20ms entre cada uno).

**Pendiente v2 (cuando se necesite)**: condvar real
(`waiters` list por SyncList + `notifyOne` al `add`) para evitar el
spin-poll de 1ms. Para v1 es suficiente; el spin a 1ms es invisible.

### N6 — Sin migración para `.bpi` v1
E2 ya regenera v2 → v3 cuando hay `.bp` fuente. Pero si llega un `.bpi` sin
`.bp` correspondiente (distribuciones), el reader peta con "directiva no
reconocida". Falta o un fallback de lectura tolerante o un mensaje guía claro.

### N7 — BpIde: errores vs avisos
La introducción de L8 emite *avisos* del semántico. La tabla de errores del
IDE los muestra mezclados con errores. Añadir columna "severidad" con color
distinto para avisos.

### N8 — Debugger no probado con sync property
Tras arreglar B2 (return interno en getter/setter sync), no verifiqué que el
debugger del BpIde pueda inspeccionar variables locales dentro de un cuerpo
de accesor custom. Test menor pero pendiente.

### N9 — Sintetizadas vs declaradas por el usuario
`synthesizeMutexClass` / `synthesizeSyncListClass` saltan si el usuario
declara una clase con el mismo nombre. Pero si el usuario declara
parcialmente (e.g. `class SyncList` con sólo `add`), la del usuario gana y
queda incompleta. Diagnosticar la incompatibilidad de firma sería útil.

### N10 — Ficheros de proyecto (añadido por el usuario, cerrado)
Dos ficheros de proyecto distintos:
1. **N10.vm — `.bpproject` para la VM** (cerrado): le dice a `bpgenvm` qué
   .mod arrancar (`main`) y dónde buscar los .mod de los imports
   (`modulePaths`). La VM acepta como arg `*.mod` o `*.bpproject` y
   detecta por extensión. Resolución de imports: as-given →
   modulePaths → stdlibDir (BpVM.cfg) → cwd. Sample anotado en
   `docs/BpProject.example`.
2. **N10.build — `.bpbuild` para el compilador y el IDE** (cerrado):
   formato JSON con:
   - `projectDir` (opcional, default = dir del `.bpbuild`).
   - `sourceDir` (obligatorio, dir con los `.bp` del proyecto).
   - `outDir` (obligatorio, dir donde se emiten `.mod` y `.bpi`).
   - `main` (obligatorio, nombre lógico del módulo principal).
   - `dependencies` (opcional, lista de paths a dirs o `.mod`/`.bpi`).

   **Compilador**: `lexer-java --project app.bpbuild` carga el build file,
   localiza `<sourceDir>/<main>.bp`, lo compila a `outDir`, y al resolver
   imports prueba (1) sources del proyecto, (2) outDir, (3) cada entrada
   de `dependencies` (dir → busca `.bpi` dentro; fichero → lo usa
   directo). Implementación en `basicplus.frontend.BpBuild` + cambios en
   `Main.java` (`locateImportBpi` extendido con `ctx.dependencyPaths`).
   Sample: `docs/BpBuild.example` + test e2e `/tmp/projbuild/`.

   **IDE**: nueva entrada de menú "Project" con "New Project...", "Open
   Project...", "Close Project". Al abrir/crear un proyecto se carga el
   `.bpbuild` y se muestra un árbol estilo NetBeans en el panel superior
   izquierdo (`jSplitPane2.setTopComponent` con `JTree`). El árbol tiene
   tres ramas: **Sources** (todos los `.bp` bajo `sourceDir`), **Output**
   (`.mod`/`.bpi` bajo `outDir`), **Dependencies** (entradas del campo
   `dependencies`). Doble click en cualquier fichero lo abre en el editor.
   - "New Project...": pide un dir destino y un nombre, crea
     `<dir>/<name>.bpbuild` + `<dir>/src/<name>.bp` (stub con un
     `main()` vacío) + `<dir>/out/`.
   - "Open Project...": JFileChooser filtrado por `.bpbuild`.
   - "Close Project": vuelve a modo single-file.
   `doRun`/`doDebug` detectan `currentProject != null` y compilan
   `<sourceDir>/<main>.bp` → `outDir/<main>.mod`, pasándole a la VM las
   dependencias del proyecto como `modulePaths` efímeros.

### N12 — `move(src, dst, srcStart, dstStart, count)` builtin (cerrado)
**Estado**: cerrado. Builtin implícito en SemanticAnalyzer; nuevo opcode
`MOVE` en `Builtin.java` + caso en `dispatchBuiltin`. Funciona sobre
arrays de cualquier tipo de elemento (i8/i16/i32/ref); valida en runtime:
ambos refs vivos, mismo tipo de array, count y rango dentro de las longitudes.
Soporta overlapping en el mismo array (System.arraycopy sobre el byte[]
subyacente). Sample: `samples/movetest.bp`.

### N13 — Infraestructura `intrinsic` (cerrado)
**Estado**: cerrado. Nuevo keyword `intrinsic`. `public intrinsic
function f(...)` declara sólo la signature; no se parsea body ni `end`.
El `.bpi` v5 lleva el flag (`func ... public intrinsic`). En el call-site,
si la `FunctionSymbol` resuelta tiene `isIntrinsic`, el emisor consulta el
registro `Intrinsics.java` (`Map<String, IntrinsicEmitter>` por nombre
cualificado `"Module.func"`) y mete los opcodes inline. NO se emite
CALL_EXT, así que el `.mod` dueño tampoco entra en la tabla de imports —
sólo persiste el CALL_EXT a `Module.__init` para mantener la cascada de
inicialización en orden.

El `.mod` del módulo dueño se compila igual pero los cuerpos de los
intrínsecos NO se emiten (la entrada al .bpi se preserva). Esto convierte
módulos como `Math` e `IO` en "namespaces con tipo" — el usuario los
importa para tener `Math.foo` con autocompletado y typecheck, pero las
llamadas se reducen a una secuencia mínima de opcodes.

### N14 — Math.mod stdlib (cerrado)
**Estado**: cerrado. `bpstdlib/Math.bp` declara como intrínsecos:
- Constantes: `pi()`, `e()`.
- Trigonometría: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`.
- Logaritmos: `ln` (natural), `log10`.
- `sign(integer)`, `signF(float)` → integer en {-1,0,1}.
- `factorial(integer)` — exacto i32, error si n<0 o n>12 (desborda).
- `gamma(float)` — Lanczos approximation (g=7, n=9), con reflexión de
  Euler para 0 < x < 0.5. Γ(n+1) = n!.

Nuevos builtins en VM: SIGN_I, SIGN_F, ASIN, ACOS, ATAN, ATAN2,
FACTORIAL_I, GAMMA_F. Sample: `samples/mathtest.bp`.

**Migración**: sin/cos/tan/pi/e/log/log10 estaban antes como builtins
implícitos. Se MOVIERON a Math.* (`log` se renombró a `ln`). El usuario
con código previo debe añadir `import Math` y prefijar las llamadas
(`Math.sin`, `Math.pi()`, etc.). `exp` y `randomInt` siguen implícitos
— se usan con frecuencia en código de día a día y no merece pena el
ruido del namespace.

Como las intrínsecas se inlinean a los mismos opcodes que tenían como
builtins implícitos, el coste runtime es idéntico — solo cambia la
sintaxis del call-site.

### N15 — IO.mod stdlib (cerrado)
**Estado**: cerrado. `bpstdlib/IO.bp` declara como intrínsecos:
- Path (sin tocar filesystem): `pathJoin`, `pathParent`, `pathBasename`,
  `pathExtension`, `pathAbsolute`.
- Filesystem: `mkdir`, `rmdir`, `removeFile`, `rename`, `copyFile`,
  `fileSize`, `isDirectory`, `lastModified`.

Nuevos builtins VM. Errores se lanzan como `BpThreadFault` con mensaje
claro (el thread que llamó muere, los demás siguen — B3 v2 lo convertirá
en un `RuntimeError` BP atrapable). Sample: `samples/iotest.bp`.

### N16 — Auto-discovery de `stdlibDir` en el frontend (cerrado)
**Estado**: cerrado. El frontend (`Ctx`) ahora carga `BpVM.cfg` (mismo
mecanismo que la VM: cwd) y añade su `stdlibDir` a `dependencyPaths`.
Así `import Math` y `import IO` resuelven sin que el usuario configure
nada en cada .bpbuild. Si no hay BpVM.cfg, comportamiento idéntico al
anterior.

### N11 — Debug: properties públicas de módulos en el debugger (cerrado)
**Estado**: cerrado.

- `.dbg` bumped a v2: nueva sección `properties` con `<name> <type>
  <csOff>` por cada `public property` a nivel módulo. csOff es el offset
  (relativo al CS del módulo) del backing global `__prop_<name>`.
- `ModuleManager` parsea la sección; nueva API
  `snapshotAllProperties()` devuelve `List<PropertyView>` con módulo,
  nombre, tipo y representación lista para mostrar. **Lectura directa
  del data block — no ejecuta BP code**, así que es seguro llamarla
  con la VM pausada en cualquier punto, sin riesgo de side-effects, GC
  o deadlock con `sync property`.
- `DebugContext.moduleProperties()` expone la snapshot al hook del IDE.
- `BpIde/FrmMain` tiene nueva pestaña "Properties módulo" en el panel
  inferior; se actualiza en cada `onPaused` y se vacía en `onResumed`.

Tipos mostrados: `integer`, `boolean`, `float`, `string` (entrecomillada con
escapes mínimos), `ref` (clases, se muestra como `@<addr-hex>`).

---

## 🟢 Mejoras de pulido (no urgentes)

### M2 — Auto-unbox `any → primitive` con check de tipo runtime
Variante "segura" de L1: en vez de warning del semántico, generar
bytecode que verifique el tipo en runtime y throw `RuntimeError` si no
encaja. Coste: tag de tipo en cada `any`. Discutible si vale la pena.

### M3 — Exportar clases en `.bpi` (= L2)
Misma cosa, marcada como mejora porque su implementación es opcional.

### M4 — Namespace separado para identificadores sintéticos
Hoy `__prop_get_X`, `__strconcat`, `__strequals` etc. comparten el
namespace de funciones del módulo con el código de usuario. Aunque
el prefijo `__` está reservado (los identificadores BP no pueden
empezar por `_`), una verificación explícita en el emisor evitaría
sorpresas si esa regla cambia.

### M5 — Debugger: inspección de properties heredadas
No verifiqué si la cadena de herencia se recorre al inspeccionar
propiedades. Relacionado con N11.

### M6 — `popBlocking` / condvar en SyncList (= N5)
Mismo item, marcado como mejora porque hay un workaround (busy-wait).

---

## 📋 Prioridad sugerida para la siguiente sesión

Trabajo cerrado en esta sesión: B1 (mejor pass-rate), N3 + 4 fixes de paso, L1.

Cosas pendientes que cogería en este orden:

1. **B3 v2** — `try/catch` BP real para errores de mutex e IO/Math
   intrínsecos (construir RuntimeError BP desde código nativo y disparar
   THROW; muchos sitios usan `BpThreadFault` que sólo tira el thread).
2. **L10** — Tipos enteros estrechos (`byte`, `int8`, `int16`, `word`,
   `short`). Diseño semántico ya decidido (load promociona a i32, store
   requiere cast explícito si puede truncar). La maquinaria de la VM
   (`ALOAD_I8/U8/I16/U16`, `ASTORE_I8/I16`, `GET_GLOBAL_I8/...`) ya
   existe. Falta: lexer, BpType, sintaxis de cast, MivmEmitter, check
   de rango estático.
3. **L2 v2** — herencia cross-module, static members, class types
   cross-module en signatures.
4. **B1 residual** — instrumentación interna para localizar la race que
   produce el ~10% restante (workers=1 → 100% OK, multi-worker → ~90%).
5. **N5 v2 / M6** — condvar real para SyncList (hoy sleep(1) spin).
6. **Skip CALL_EXT __init para módulos all-intrinsic** — hoy MathTest.mod
   tiene `Math.__init` en imports aunque Math.mod está vacío de código
   de usuario. Sería bonito que un módulo all-intrinsic no requiera
   tener un .mod en runtime (sólo el .bpi en build-time). Optimización
   menor.
7. **N1 / N2** — formalizar la convención de acceso a `mem[]` /
   `JavaMutex` campos no-volátiles. Investigación dejó claro que JMM
   no es la causa dominante del residual de B1, pero la documentación
   sigue siendo útil.

Resto: pulido (N4, N7, N8, N9, M4, M5) y trade-offs de diseño (L3, L4, L5,
L6, L7, N2).
