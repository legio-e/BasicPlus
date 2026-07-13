# BasicPlus V4 — Catálogo de auditoría de referencias (censo antes de corregir)

> **Método (Eduardo, 13-jul-2026):** siguen apareciendo bugs de memoria nuevos → los
> escaneos por síntoma dejaron huecos. Antes de tocar NADA, levantar el **catálogo
> completo** de todos los sitios donde se maneja una referencia en las DOS VMs,
> clasificar cada uno, y cruzar VM-C↔miVM. Solo cuando el catálogo esté cerrado se
> corrigen los bugs **1×1 con sus pruebas**. "Despacio pero seguro."
>
> Es la ejecución del §4 de `V4_REF_ABSTRACTION.md` (la abstracción ya existe en
> las dos VMs; falta **terminar la adopción** en cada sitio crudo). Enfoque
> decidido: **(B) centralizar**, no parchear sitio a sitio.

---

## 0. Decisiones fijadas

- **(B) Centralizar** el manejo de refs (no parchear): todo sitio pasa por el helper único.
- **Java = clase `BpRef` de LÓGICA sobre `long`** (encode/decode/gen/idx/validar como
  métodos; el almacenamiento sigue `long` para no matar el bucle caliente). Gemela del
  `bpref_t {uint64_t v}` + macros que ya existe en C. (Opción (ii).)
- **Catálogo primero, corrección 1×1 después** — no interlear (se pierde el norte).

## 1. El contrato de referencia (qué es "correcto")

Una referencia es un **handle de 64 bits** `[gen:32 | idx|TAG:32]`, `REF_SIZE = 8`.
Todo sitio que la toca DEBE pasar por la abstracción:

| | VM-C | miVM (Java) |
|---|---|---|
| tipo/tamaño | `bpref_t {uint64_t v}`, `BPVM_REF_SIZE=8` | `long` + `REF_SIZE=8` (futuro: clase `BpRef`) |
| memory[] | `bpref_load` / `bpref_store` | `refLoad` / `refStore` |
| pila | `bpref_pop` / `bpref_push` | `popRef` / `pushRef` |
| handle→addr | `bpref_deref` | (deref interno) |
| registrar handle | `bpvm_handle_register` | `handleRegister` |
| liveness (contrato B) | `bpvm_ref_dead` | `requireAlive` |

**Reglas de oro:**
1. Una ref **NUNCA** se trunca a 32 bits. `(int) readI64(ref)` = BUG (pierde la
   generación de la palabra alta → use-after-free no detectado).
2. Stride de ref-array / campo-ref = `REF_SIZE` (8), nunca 4. Un campo de 8B ocupa
   **2 slots** de 4B; el offset se calcula por tabla de slots, no `i*4`.
3. Antes de tocar `memory[]` con un handle hay que **dereferenciarlo** (`bpref_deref`).
4. `pop`/`push` de ref ajustan `sp` por `REF_SIZE`.

## 2. Rúbrica de clasificación

- **OK** — usa la abstracción correctamente.
- **RAW-SAFE** — usa camino crudo (`pop_i32`/`readI64`/offset a mano) pero el valor
  **NO** es una ref (índice/escalar `int`/`long` genuino) → no es bug.
- **SUSPECT** — camino crudo sobre algo que **SÍ** es una ref → posible bug 4B/8B.
- **CONFIRMED** — SUSPECT verificado leyendo el código (con repro si lo hay).

**Cruce VM-C↔miVM:** un sitio correcto en una VM y crudo en la otra = **bug latente**
en la cruda. Es el patrón que cazó el bug del Thread ("sabor Java").

### 2.1 TRES superficies, no dos — y el emisor es CIEGO a la paridad

El bytecode lo produce el **EMISOR** (`MivmEmitter.java` + `ModWriter.java`, en
`lexer-java/`), que decide el ANCHO de cada ref en tiempo de compilación. Si el
emisor emite una ref con ancho equivocado (`declareLocal` 4B en vez de
`declareLocalLong` 8B, un tipo-ref ausente de `isRefType`, `RET` en vez de `LRET`…),
**las DOS VMs ejecutan el mismo bytecode malo igual de mal** → misma salida → **la
paridad pasa en verde**. El bug de las **tuplas (#281) fue exactamente esto**: no lo
cazó la paridad, lo cazó que *petaba*. Por eso el emisor es la superficie más
peligrosa y hay que barrerla por CONTRATO, no por divergencia. Superficies:
1. **Emisor** (compilador) — ciego a la paridad. *(la más crítica)*
2. **Runtime VM-C**.
3. **Runtime miVM**. — 2 y 3 sí los cruza la paridad.

---

## 3. Semillas (bugs ya conocidos que el catálogo debe explicar)

| # | Bug | Estado | Dónde cae |
|---|---|---|---|
| S1 | **FileTest use-after-free** — `fileExists`+concat en host, ambas rutas FS (libc/lfs), GC on/off → estructural 4B/8B. Repro host sin placa. **HIPÓTESIS FUERTE: es un bug del EMISOR (F), hermano de las tuplas** — `__strconcat` declara su temp-String en 4B (`declareLocal`) en vez de 8B. Peta en el concat, no en el FS. | ABIERTO | **Emisor (F) — `__strconcat`** / o Builtins (E) |
| S2 | **GC `i*8` vs `i*4`** — miVM lee refs-hijas con stride distinto en `VirtualMachine.java:1289` vs `:1305` + `(int) readI64` (trunca handle) | SOSPECHA a la vista | GC (D) |
| S3 | **Concat de strings peta solo-Pico** (gen basura en SRAM del device) | APARCADO (Pico) | Builtins (E) — string / heap |
| **S5** | **Tuplas (#281)** — el emisor las trataba en 4B (olvidó `isRefType(TupleType)` + `declareLocalLong`). ARREGLADO, pero es el **EXEMPLAR de la clase emisor/ciego-a-paridad**. El §6 lista HERMANOS del mismo patrón sin verificar: `__strconcat`, var de `catch`, `__newref` de Mutex, `__stf_`. | FIXED (verificar hermanos) | **Emisor (F)** |
| S4 | Familia ya cerrada: ~45+34 builtins tandas 1/2, gen truncada en heap_alloc_string | CERRADOS | (referencia — verificar que no dejaron hermanos) |

---

## 4. Hallazgos por subsistema  *(EN CURSO — se rellena con el censo de 6 agentes + verificación propia)*

### F. EMISOR (compilador) — `MivmEmitter.java` + `ModWriter.java`  ⚠️ ciego a la paridad, LA MÁS CRÍTICA
`isRefType`/`occupies8Bytes` (¿cubren TODOS los tipos-ref?), `declareLocal` vs
`declareLocalLong` de refs (emitConstruction, `__stf_`, catch var, **`__strconcat`**,
Mutex `__newref`, temps de tupla), `declareParam(this,8)`, `declareField(is8)` +
nextSlot, GET_FIELD vs GET_FIELD_LONG, `returnsLong`→LRET, `paramSlots`, el hack
`I32_TO_I64`. **Incluye S1 (hipótesis `__strconcat`) y S5 (hermanos de tuplas).**
_(pendiente de volcar + verificar)_

### A. Pila de operandos (ALOAD/ASTORE/GET_FIELD/SET_FIELD/INSTANCEOF/ALEN/receptor)
_(pendiente de volcar + verificar)_

### B. Frame de llamada y locales (GET/SET_LOCAL, LEA, RET/LRET, slots de params, receptor) — ✅ AUDITADO: 0 SUSPECT
Aplicado correcta y **simétricamente** en ambas VMs. Los opcodes de 4B (GET/SET_LOCAL,
_S8, RET) son RAW-SAFE: el emisor es width-aware y jamás les entrega una ref (una ref
→ GET/SET_LOCAL_L 8B, LRET, `declareParam(this,8)`, `argSlotCount`=2 slots/ref,
receptor en `sp-REF_SIZE-numArgs*4` sin `(int)` truncante). **Blocker "cs=0 tras
INVOKE con arg 8B" = RESUELTO** (verificado: `paramSlots`/`argSlotCount` cuentan slots
con `this`+refs a 2). Único residuo trivial (NO bug): comentario stale en
`MivmEmitter.java:4646` dice "sp-4-numArgs*4" pero el código usa `sp-REF_SIZE`(8).
Nota del agente: si las tuplas dejaran algún hermano, NO está en frame/locales (un
local-tupla se declara 8B correctamente) → mirar en campos/arrays (A/C) y emisor (F).
**VERIFICADO A MANO (duda de Eduardo sobre `sp-REF_SIZE-numArgs*4`):** el operando
`numArgs` de INVOKE_VIRTUAL se hornea como `argSlotCount(fs)` (MivmEmitter:3246), que
cuenta SLOTS (param de 8B = 2 slots vía `occupies8Bytes`). ⇒ `numArgs*4` = bytes reales
de args, correcto con args de 8B. Fue BUG-6, ya arreglado (comentado en :3245/:4645).
NO es bug, pero SÍ **objetivo de limpieza (B)**: el operando se LLAMA `numArgs` pero es
`numArgSlots`, y el `*4` clava el slot-size → renombrar + pasar el `*4` por constante.

### C. Heap, layout de objeto y arrays — ✅ AUDITADO: 0 SUSPECT en el layout puro
Byte-simétrico y consistente en ambas VMs: objeto = slots de **4B**, `num_fields`=nº
de slots, un campo ref/long/double ocupa **2 slots** (bit del bitmap solo en el slot
base), acceso `deref+4+slot*4`; ref-array (TYPE_ARRAY_REF) `elementSize=8`, stride
`i*8` en TODOS los caminos (alloc/grow/copy/list/mark/free); `NEW_REF_ARRAY` ya en `*8`
(el viejo `*4` corrompía, documentado). Strings = TYPE_ARRAY_I8 (1B/elem, NO ref-array).
Tuplas = compilan a TYPE_OBJECT → el heap siempre fue correcto (8B); el bug fue 100%
codegen, ya resuelto. Sin asimetría `i*4`/`i*8`.
**Contrato load-bearing (para F):** la corrección de `num_fields*4` depende de que el
EMISOR cuente cada campo ref/long/double como **2 slots** + bit del bitmap solo en el
slot base. Si el emisor fallara, romperían las DOS VMs igual (ciego a paridad) → F lo verifica.
**2 SUSPECT fronterizos** (fuera del layout puro) → ver §5 B1, B2.

### D. GC y ciclo de vida de handles (mark/sweep, refs-hijas, cascada de free, tabla de handles)
_(pendiente — incluye S2)_

### E. Builtins y throw (string, array, file I/O, Thread/Mutex, GUI, Net, HW, msg de throw)
_(pendiente — incluye S1, S3)_

---

## 5. Bugs / candidatos (para corregir 1×1 después)  *(se llena según cierran los subsistemas)*

### ✅ #1 ARREGLADO (`e2d56fe`) — 3 campos-ref de stdlib declarados `is8=false` (4B)
**`StringBuilder.chars`, `__syncMutex` (clases sync), `SyncList.__mutex`** se declaran con
los atajos `declareField(name,isRef)` / `(name,isRef,isOwner)` de ModWriter (:817/:812),
que ponen **`is8=false`** (4 bytes, era pre-ensanchado). Pero los tres son REFS (array /
Mutex) = 8B. Sitios: `MivmEmitter.java:3817` (chars), `:1084` (__syncMutex), `:4076`
(SyncList.__mutex). **Dos fallos por el mismo origen:**
- **(a) truncado del handle** — `emitGetField/SetField` (ModWriter:1061/1069) eligen opcode
  por `is8` → `GET_FIELD`/`SET_FIELD` PLANOS de 4B sobre una ref de 8B → pierde la palabra
  alta (generación). En host "va" cuando la basura de pila es 0 (gen 0 casa); rompe cuando
  ≠0 o el slot se recicla tras un grow. _(barrido A)_
- **(b) GC no marca la hija** — `markObject`/`mark_recursive` asumen `is8=true` para todo
  campo-ref → leen el slot **i+1** (el campo siguiente) en vez del i → `refDeref(basura)` →
  la hija (el `int[]` de chars / el Mutex) **nunca se marca** → use-after-free latente al
  saltar el GC. _(barrido D — VM-C `heap.c:186`, miVM `VirtualMachine.java:1305`)_
**Simétrico en ambas VMs** (mismo `.mod`). Familia de [[v4-tuplas-ensanchado-olvidado]].
`StringBuilder` es el motor del concat → **candidato nº1 de FileTest (S1) y concat Pico (S3)**.
**Alcance CERRADO (enumeradas TODAS las `declareField` de MivmEmitter):** son EXACTAMENTE
estos 3 (líneas 1084/3817/4076, atajos de 2-3 args). El resto de campos-ref usan el
`declareField` de 4 args width-aware (`occupies8Bytes`): OO (1100), properties (1510),
`List.items` (3546, `is8=true`), tuplas (4718). No hay más hermanos.
**Fix (centralizador, opción B):** que `declareField` fuerce `is8=true` cuando `isRef=true`
(una ref SIEMPRE es 8B) → arregla los 3 y hace **imposible** volver a declarar una ref de 4B.

### ✅ #2 ARREGLADO (`80598c5`) — `freeOwnedObjectLocked` leía el child ref-array truncado
`VirtualMachine.java:5081` lee cada elemento de la cascada owner de array-de-refs con
`(int)readI64(base+i*8)` → **trunca la gen a 0**; luego los checks de gen (5051/1651) lo
toman por rancio → NO libera la hija poseída (leak) y NO recicla el slot. VM-C
(`interp.c:482`) usa `bpref_load` (handle completo) → correcto. **Fix 1 línea: `refLoad`
en vez de `(int)readI64`.** _(barrido D)_

### ✅ #9 ARREGLADO (`0aebec3`) — `__strequals` params a 4B → string `==`/`!=` ROTO
`MivmEmitter.java:5275-5276`: `emitStrequalsBody` declara `a`/`b` con `declareParam(name)`
(1-arg = **4B**) siendo strings de 8B. El `==`/`!=` de strings llama a `__strequals`
(`emitBinary:2928-2933`, "BP compara CONTENIDO"). **Repro host (`"abc"=="abc"` → `false`;
`"abc"=="abd"` → `false`; `a==b` → `false`)**: devuelve mal SIEMPRE + **fuga 8B de pila que
corrompe la expresión que lo envuelve** (`"x="+boolToString(str==str)` sale `"abcfalse"`).
Determinista, en AMBAS VMs (ciego a paridad → no lo cazó nada). Su hermano `__strconcat`
SÍ se migró (`declareParam(...,8)`) el 9-jul (`d2dcbe9`); `__strequals` se quedó atrás.
**Rota todo `==`/`!=` de strings desde el 9-jul.** Fix = `declareParam("a",8)`/`("b",8)`.

### 🔴 #10 CONFIRMADO (AMBAS VMs, EMISOR) — init de var-string de módulo declara global a 4B
`MivmEmitter.java:642` (`bakeModuleVarInit`, +484-486): `var s: string := "lit"` a nivel
módulo declara el global con `declareGlobal(name)` (4B) → emite `LEA_GLOBAL`(8B)/`SET_GLOBAL`(4B)/
`GET_GLOBAL`(4B) → trunca el ref + fuga 4B. El path SIN-init (622) usa `declareGlobalLong`.
git: 622 migrado 9-jul; el path CON-init se quedó. Fix = `declareGlobalLong` en el init.

### ✅ #11 ARREGLADO (`606592b`) — `null` a 4B + `==`/`!=` de refs con `EQ` de 4B
**Ampliado por insight de Eduardo:** el `==`/`!=` de refs usaba `OpCode.EQ` (4B, solo
`idx|TAG`) — el censo no lo había separado. Una ref es un handle de 64b; su igualdad
compara los 8 bytes (gen+idx): mismo idx + distinta gen = objetos DISTINTOS. Fix (3
piezas): null→8B, `occupies8Bytes(NullType)=true`, `==`/`!=` de 8B no-numérico→`LEQ`/`LNEQ`.
Oráculo `samples/NullRefEqTest.bp` (identidad mismo/distinto + null ambos sentidos).
<!-- entrada original: -->
### (orig) #11 — `null` → ref de tipo CONCRETO no se ensancha
`MivmEmitter.java:2842` (`NullLitExpr→emitInt(0)`, 4B) + `coerceToTarget:4923-4936` NO
ensancha `null`→ClassType/ArrayType/String/Tuple (solo `null`→AnyType). Bytecode: `var a:
Animal := null` → `PUSH 0`(4B) → `SET_LOCAL_L`(8B) sin `I32_TO_I64` → mete 4B en hueco 8B
(basura en word alto + underflow 4B). Vivo en `:=`, `return`, arg, campo, elem-array cuando
el destino estático es clase/array/string/tupla **concreta**. `Object`(=AnyType) SÍ va →
por eso `var x: Object := null` funciona. Fix = ensanchar null→ref-concreta (o coerceToTarget).

### ⚪ #12 ANALIZADO — INOCUO (no divergente, no observable) — `I32_TO_I64` int→any sign-extend
`coerceToTarget:4933`: un `int` NEGATIVO metido en `any` queda con word alto `0xFFFFFFFF`
(el modelo 4639 dice zero-extend). **VEREDICTO (13-jul): no se arregla.** El emisor emite el
MISMO opcode para AMBAS VMs → NO hay divergencia dual-VM. La discriminación ref-vs-número de
un `any` mira el TAG de la palabra BAJA (no la alta), y el scan conservador tolera 0xFFFFFFFF
en la palabra alta → sin efecto observable. Zero-extend costaría opcodes de máscara por cada
coerción int→any por pura cosmética. Si algún día se hashea/serializa el `any` crudo de 8B,
reconsiderar (o corregir el comentario del modelo a "sign-extend").

### ⚪ #13 latente (EMISOR) — `newarrayOpForElement:3403` sin la rama `NEWARRAY_I64` de su espejo
Un array-fijo local de refs caería a `NEWARRAY`(4B) + `ASTORE_I64`(8B); hoy inalcanzable
(el semántico rechaza arrays-fijos de refs). Mordería si se levanta esa restricción. _(F)_

### 🟡 #14 ANALIZADO — real pero ENMASCARADO + NO divergente (seguimiento con pruebas, decisión de Eduardo)
`isRefType:1646`=false para `any` → un campo `Object`/`any` (isRef=false en el bitmap de refs
del objeto) NO se traza en la marca PRECISA del GC (markObject usa el bitmap estático para
TYPE_OBJECT). En teoría: un objeto cuya ÚNICA ref viva está en un campo `any` → recolectado →
UAF. **VEREDICTO (13-jul): NO se toca esta sesión.** (1) **No es divergencia dual-VM**: ambas
VMs leen el MISMO bitmap del .mod y ambas tienen el scan conservador → comportamiento idéntico
(oráculo AnyGcTest byte-idéntico, `val=777` en las dos, incluso con stash+churn+GC_EVERY=1).
(2) **Enmascarado** por el scan conservador de pila (`scanRegion(stackBase,sp)` marca cualquier
patrón que parezca handle vivo) → no se pudo reproducir un fallo desde BP. (3) **Radio de
explosión**: el fix (meter `any` en el bitmap de refs, como ya hace TYPE_ARRAY_REF que traza
conservador cada slot) es del EMISOR → cambia el bitmap de TODA clase con campo `any`/`Object`
→ exige recompilar stdlib + reverificar paridad + regenerar blobs, y toca la precisión del GC
(lo delicado). Correcto arreglarlo como tarea deliberada con pruebas de GC, no a ciegas sobre
código que hoy funciona. Fix propuesto: `isRef = isRefType(t) || t instanceof AnyType` en los
sitios de `declareField` (1098/campos, +arrays/props si aplica); verificar el bit en el disasm
del descriptor de clase + JUnit + List/SyncList (usan `any[]`, ya trazado). _(F, observación 2ª)_

### ✅ #6 ARREGLADO (racimo runtime; GUI reservado a Eduardo) — builtins sin convertir a 8B
**Runtime CERRADO:** `MOVE`/`SPLIT`/`LIST_DIR` `fb4955a`, `READ`/`WRITE_FILE_BYTES` `dc1c0bb`, `TCP_SEND`/`RECV` `3eb7c09`, `TO`/`FROM_BYTES`+`CHARS_TO_STRING`+`CHAR_CODE_AT` `e0e3014` (todos con oráculo dual-VM byte-idéntico bajo GC_EVERY=1 + JUnit 34/34). **THREAD/MUTEX = NO es bug** (disasm probó que corren en métodos wrapper cuyo `RET` resetea sp → el `popTc`(4B) queda confinado y borrado, nunca acumula; valor `idx|TAG` correcto; VM-C descarta la gen idénticamente → paridad total; NO tocar). **GUI = reservado a Eduardo** (verificación visual): los handles de widget son ENTEROS OPACOS (4B correcto, NO tocar); latentes forward-compat solo los string-args (`GUI_SET_TEXT`/`SET_OPTIONS`/`SET_BUTTONS`, calcar `GUI_LOAD_FONT`) y las refs de objeto de Forms (`BIND_CLICK`/`INVOKE_BY_NAME`/`INVOKE_BY_SLOT`). _(nota histórica del censo abajo)_

<!-- CENSO ORIGINAL (histórico): -->
miVM dejó un grupo de builtins en el carril CRUDO de 4B (`popTc`/`pushTc`) o usando el
handle como dirección sin `refDeref`, mientras VM-C SÍ los convirtió (foco de las tandas).
Verificado directamente el racimo GUI (`GUI_BIND_CLICK`:3636 `self=popTc(4B)`;
`GUI_INVOKE_BY_NAME`:3644-46 **3×`popTc(4B)`** sobre 3 refs de 8B → desalinea 12B). No lo
cazó la paridad porque estos paths no se ejercitan (GUI en miVM/Swing = hueco anotado,
byte[] binario, `move`). **Activamente rotos** (multi-arg desalinea un pop posterior, o
truncan el handle-resultado): `MOVE`:4212, `WRITE_FILE_BYTES`:3938, `READ_FILE_BYTES`:3929,
`TCP_SEND`:3523, `TCP_RECV`:3578, `TO_BYTES/FROM_BYTES`:4044/4053, `SPLIT`:3857,
`LIST_DIR`:3974, `GUI_SET_TEXT`:3598, `GUI_SET_OPTIONS`:3686, `GUI_SET_BUTTONS`:3693,
`GUI_TABVIEW_ADD_TAB`:3695, `GUI_TABLE_SET_CELL`:3698, `GUI_BIND_CLICK`:3636,
`GUI_INVOKE_BY_NAME`:3644, `GUI_INVOKE_BY_SLOT`:3654. **Latentes** (1 ref al fondo,
enmascarados por reset de `sp` en RET + palabra baja): `THREAD_START`:4072, `THREAD_JOIN`:4111,
`MUTEX_LOCK`:4144, `MUTEX_UNLOCK`:4172, `CHARS_TO_STRING`:4020, `CHAR_CODE_AT`:4032.
VM-C equivalente = OK en todos. Fix = `popTcRef`/`pushTcRef`(8B) + `refDeref`. _(barrido E, racimo GUI verificado a mano; resto = spot-verify al corregir)_

### ✅ #7 ARREGLADO (`8551e30`) — `OP_THROW` trunca la gen de la excepción
`VirtualMachine.java:3125/3145`: lee la ref 8B pero la re-empuja al catch con `(int)v` /
`((long)v)&0xFFFFFFFF` → gen=0. Una excepción en slot RECICLADO (gen>0) daría gen-mismatch
al leer `e.msg` y se re-lanzaría. VM-C `exceptions.c:78` escribe el handle 64b completo = OK.
Enmascarado hoy solo en régimen sin-GC. _(barrido E)_

### ✅ #8 ARREGLADO (`1973009`) — 3 `push_i32` de string en builtins APP_*
Fix: los 3 pasan a `push_ref`. Red→green probado (samples/AppTest.bp): VM-C imprimía
`n=0` (12B de drift corrompían el local siguiente), ahora `n=42` byte-idéntico a miVM.
<!-- censo original: -->
`builtins.c:643/649/655` (`APP_MAIN_MODULE`/`_PATH`/`APP_PROJECT_PATH`) empujan el handle
de string con `push_i32`(4B) en vez de `push_ref`(8B) → trunca gen + desalinea. miVM
correcto (`pushTcRef`). Path poco ejercitado. Fix = `bpref_push`. _(barrido E)_

### ✅ CONFIRMADO OK — el path de file I/O de FileTest NO es el bug
`readFile`/`writeFile`/`appendFile`/`fileExists` son OK en AMBAS VMs (pop/push por la
abstracción). Refuerza que **FileTest peta por el concat (`StringBuilder.chars`, #1)**, no
por el I/O. _(barrido E)_

### 🟠 #3 candidato (device-only) — `heap.c:339-350 bpvm_handle_register` realloc sin zero-init
El `realloc` de `handle_addr`/`handle_gen` no pone a 0 `[old_cap,new_cap)`. Host inocuo
(lectores guardan `idx<handle_next`), device = SRAM basura → gen deforme. Región exacta de
[[v4-concat-handle-gen-basura-placa]]. Fix = `memset` al crecer. VM-C-only. _(barrido C)_

### 🟡 #4 diferido (ya conocido) — `bpvm_aot_helpers.c` no handle-aware
Accesores de objeto/ref-array del AOT no ensancharon refs a 8B. Solo `.mdn`. Ya en
[[v4-pendientes-diferidos]] (test-throwmsg/throwuser rojos). No bloquea. _(barrido C)_

### ⚪ #5 latente — `ModWriter.declareInstanceProperty`→`declareField(name,isRef)` (is8=false)
Si algún día se usa con `isRef=true` para una property-ref, crea otro campo-ref de 4B
(mismo bug #1). Hoy el path de properties del emisor usa el `declareField` de 4 args
width-aware → probablemente sin uso para refs. Lo cubre el fix centralizador de #1. _(barrido D)_

### ✅ DESCARTADO — S2 (`i*8` vs `i*4` del GC) NO es bug
Son conceptos distintos y correctos: `i*8` indexa ELEMENTOS de ref-array (8B c/u); `i*4`
indexa SLOTS de 4B de objeto (un campo-ref ocupa 2 slots). El `(int)readI64` del MARCADO es
inocuo (`refDeref` ignora la gen; solo necesita idx→addr). Simétrico en ambas VMs. _(barrido D)_
