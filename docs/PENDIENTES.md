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

**Lo que falta** (residual ≤10 %):
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

### B3 v2 — `try/catch e: RuntimeError` para errores de mutex (pendiente)
**Estado**: B3 v1 cierra el síntoma peor (los errores de Mutex no tumban la VM
gracias a `BpThreadFault`), pero el usuario sigue sin poder ATRAPAR el error en
código BP con `try/catch e: RuntimeError`. Para eso falta construir una instancia
BP de `RuntimeError` desde código nativo y disparar la lógica del opcode `THROW`.

**Plan**:
1. En `ModuleManager` indexar el `class_ptr` de `RuntimeError` por módulo durante
   el load (RuntimeError lo sintetiza el emisor en CADA módulo, así que siempre
   está disponible).
2. Helper `throwBpRuntimeError(tc, msg)` en `VirtualMachine`:
   - aloca BP-string con el mensaje;
   - aloca BP-`RuntimeError`;
   - setea `obj.msg := stringRef`;
   - empuja el ref y ejecuta la lógica de unwind del case `THROW` (0x5D).
3. Reemplazar los `throw new BpThreadFault(...)` de los builtins de mutex.

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

**Lo que queda pendiente (v2)**:
- Herencia cross-module (`class Bar extends Module.Foo`). Hoy se serializa el
  `extends` en el `.bpi` pero el importador lo ignora — el `ClassSig` tiene el
  campo `baseClassName`. Necesita extender el class section del .mod para que
  las vtables del subtipo reciban los slots del padre externo.
- Static members (consts / vars de clase). Hoy se ignoran en `extractClass`.
- Static methods públicos cross-module (mismo trato que functions, pero
  por nombre `Cls.method` en el namespace del módulo dueño).
- Class types cross-module en signatures: hoy admitimos clases del MISMO
  módulo (vía `UnresolvedClassRef` resuelto en el loader). Una signature
  con `OtroModulo.OtraClase` no se soporta.

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

### L10 — Tipos enteros estrechos (byte / word / short / int8 / int16)
**Estado**: pendiente. El lenguaje hoy sólo reconoce `integer`, `float`,
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
