# BasicPlus V4 — Catálogo de auditoría de referencias (censo antes de corregir)

> ## 📍 ESTADO A 15-jul-2026 *(saneado y verificado CONTRA EL CÓDIGO, no de memoria)*
>
> **Todas las semillas (S1-S5) están CERRADAS.** El catálogo tenía entradas rancias que
> daban por abierto lo ya arreglado (S1 y #10) — se decidió sobre ellas dos veces y se
> perdió tiempo. Regla nueva: **al arreglar algo se actualiza AQUÍ en el mismo commit**;
> si no, este fichero miente y es peor que no tenerlo.
>
> **Lo único ABIERTO hoy:**
> - **#4 (=H4.b)** — `bpvm_aot_helpers.c` no handle-aware. **Causa los 9 tests rojos**
>   (ver §6). Real y acotado; no bloquea (el default es intérprete).
> - **H1.8 fases 2 y 3** — la conversión centralizadora está hecha **solo para los
>   GLOBALES** (`7896494`: cazó #16 + #17 + #18). **Faltan `declareParam` (46 sitios) y
>   `declareLocal` (97 sitios)**, que siguen decidiendo el ancho eligiendo el método —
>   o sea, siguen expuestos al mismo patrón (#9 fue un param; las tuplas y el
>   `appendStr` fueron locales). H1 NO cierra hasta terminarlas.
>
> **Latentes/inocuos (no tocar sin motivo):** #3 (degradado a higiene: su premisa se
> DESMONTÓ, ver entrada), #5, #12, #13.
>
> **Verificado hoy:** batería VM-C 16/25 (los 9 rojos = #4, **idénticos antes y después
> del fix del alocador** → preexistentes) · JUnit miVM 34/34 · tren de memoria MemT1-T5 +
> MemStress × 4 tamaños de heap = 24/24 · `FileTest` (S1) `status=OK` · en placa (Pico):
> MemT4/MemT4d verdes.

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

**✅ LAS 5 SEMILLAS ESTÁN CERRADAS (15-jul).** El catálogo cumplió su función: cada una
quedó explicada por una raíz concreta, no por una teoría.

| # | Bug | Estado | Dónde cae |
|---|---|---|---|
| S1 | **FileTest use-after-free** — `fileExists`+concat en host, ambas rutas FS (libc/lfs), GC on/off → estructural 4B/8B. **La hipótesis `__strconcat` era FALSA** (ese path ya estaba migrado a 8B el 9-jul, `d2dcbe9`); la raíz era **`StringBuilder.chars` declarado a 4B = #1** (`e2d56fe`): el GC no lo marcaba → recolectado en vivo. **VERIFICADO 15-jul: `FileTest` corre `status=OK`** (incl. `--mem=65536`). | ✅ CERRADA (= #1) | Emisor (F) — `declareField` |
| S2 | **GC `i*8` vs `i*4`** — miVM lee refs-hijas con stride distinto en `VirtualMachine.java:1289` vs `:1305` + `(int) readI64` (trunca handle) | ✅ DESCARTADA (no es bug — ver §5) | GC (D) |
| S3 | **Concat de strings peta solo-Pico** (gen basura en SRAM del device) | ✅ CERRADA (= #15, y **no era de placa**) | Alocador del heap (C) |
| **S5** | **Tuplas (#281)** — el emisor las trataba en 4B (olvidó `isRefType(TupleType)` + `declareLocalLong`). ARREGLADO, pero es el **EXEMPLAR de la clase emisor/ciego-a-paridad**. El §6 lista HERMANOS del mismo patrón sin verificar: `__strconcat`, var de `catch`, `__newref` de Mutex, `__stf_`. | FIXED (verificar hermanos) | **Emisor (F)** |
| S4 | Familia ya cerrada: ~45+34 builtins tandas 1/2, gen truncada en heap_alloc_string | CERRADOS | (referencia — verificar que no dejaron hermanos) |

---

## 4. Hallazgos por subsistema  *(EN CURSO — se rellena con el censo de 6 agentes + verificación propia)*

### F. EMISOR (compilador) — `MivmEmitter.java` + `ModWriter.java`  ⚠️ ciego a la paridad, LA MÁS CRÍTICA
`isRefType`/`occupies8Bytes` (¿cubren TODOS los tipos-ref?), `declareLocal` vs
`declareLocalLong` de refs (emitConstruction, `__stf_`, catch var, `__strconcat`,
Mutex `__newref`, temps de tupla), `declareParam(this,8)`, `declareField(is8)` +
nextSlot, GET_FIELD vs GET_FIELD_LONG, `returnsLong`→LRET, `paramSlots`, el hack
`I32_TO_I64`.

**⚠️ ESTADO REAL (15-jul): BARRIDA CON HALLAZGOS, pero NUNCA cerrada formalmente** (no tiene
un "0 SUSPECT" como B/C/D/E). Fue la superficie más productiva del censo — salieron de aquí
**#1, #9, #10, #11, #13, #14, S5(tuplas)** — y sigue teniendo **#16 vivo**. La hipótesis
`__strconcat` de S1 era **FALSA** (ya estaba a 8B desde `d2dcbe9`); la raíz de S1 fue #1.
**Patrón que REINCIDE aquí (3 veces ya: #9, #10, #16): "se migró un camino y el gemelo se
quedó atrás".** Mientras el ancho se decida sitio a sitio volverá a pasar.

> ### ▶ EL CIERRE DE F ESTÁ DECIDIDO: **H1.8** (Eduardo, 15-jul — tarea #287, última entrada de H1)
> **F no se cierra cazando gemelos uno a uno, se cierra haciendo el bug IMPOSIBLE.** Extender a
> `declareParam`/`declareLocal`/`declareGlobal` el movimiento que YA funcionó en `declareField`
> (#1, `e2d56fe`: `is8 = is8 || isRef`) → una ref SIEMPRE 8B por construcción. #9, #10 y #16
> habrían sido imposibles de escribir.
>
> **Y la verificación es lo bonito (exigida por Eduardo): el DIFF de los `.mod` antes/después ES
> LA AUDITORÍA DE F.** Un sitio ya correcto no cambia nada; **cada `.mod` que cambie delata un
> sitio que declaraba una ref de 4B**. Es barrer F por construcción en vez de a ojo — que es
> justo lo que este catálogo no consiguió hacer con F. Plan completo en la tarea #287.

### A. Pila de operandos (ALOAD/ASTORE/GET_FIELD/SET_FIELD/INSTANCEOF/ALEN/receptor)
**⚠️ ESTADO REAL (15-jul): la ÚNICA superficie sin barrido sistemático.** Solo ha producido
hallazgos de rebote (#1(a): `emitGetField/SetField` eligen opcode por `is8` → 4B sobre una ref).
No está volcada ni cruzada VM-C↔miVM. **Es el hueco conocido del censo** — tenerlo presente
antes de dar la auditoría por cerrada.

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

### D. GC y ciclo de vida de handles (mark/sweep, refs-hijas, cascada de free, tabla de handles) — ✅ BARRIDO
S2 **DESCARTADA** (no es bug). Salieron y se arreglaron: **#2** (`freeOwnedObjectLocked`
truncaba el child), **#1(b)** (el GC no marcaba la hija de un campo-ref de 4B), **#14**
(campos `any`/Object fuera del bitmap de refs). Quedan **#3** (degradado a higiene) y **#5**
(latente). ⚠️ **El bug más grave de todo el censo NO era de refs y por poco se escapa de
aquí: #15, el alocador** — el GC barría objetos VIVOS. Ver §5 #15 y la lección de §7.

### E. Builtins y throw (string, array, file I/O, Thread/Mutex, GUI, Net, HW, msg de throw) — ✅ BARRIDO (salvo GUI de miVM)
S1 **CERRADA** (era #1, no el I/O: ver "✅ CONFIRMADO OK — el path de file I/O NO es el bug").
S3 **CERRADA** (era #15). Arreglados: **#6** (racimo runtime), **#7** (`OP_THROW` truncaba la
gen), **#8** (`push_i32` de string en APP_*). **THREAD/MUTEX = NO es bug** (probado).
**Único hueco: los builtins GUI de miVM** — reservados a Eduardo por exigir verificación
visual; los handles de widget son enteros opacos (4B correcto, NO tocar), los latentes son
solo string-args y las refs de objeto de Forms. Ver #6.

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

### ✅ #10 ARREGLADO (`6b16414`) — init de var-string de módulo declaraba el global a 4B
`MivmEmitter.java:642` (`bakeModuleVarInit`): `var s: string := "lit"` a nivel módulo declaraba
el global con `declareGlobal(name)` (4B) → `LEA_GLOBAL`(8B)/`SET_GLOBAL`(4B)/`GET_GLOBAL`(4B)
→ truncaba el ref + fuga 4B. El path SIN-init (622) ya usaba `declareGlobalLong` (migrado 9-jul);
el path CON-init se quedó atrás. Fix = `declareGlobalLong` en el init. **Verificado en el código
15-jul** (la línea 642 lleva el fix + comentario del censo). ⚠️ **Su patrón "se migró un camino y
el gemelo se quedó" REINCIDE: ver #16.**

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

### ✅ #14 ARREGLADO (`381f034`, 14-jul) — campos `any`/Object ahora en el bitmap de refs del GC
Fix en el emisor (helper `isGcRef = isRefType || AnyType` en los 3 sitios del bitmap: campos,
backing field de property, elementos de tupla) → ambas VMs trazan los campos `any` en la marca
PRECISA. Seguro por construcción (mismo `mark_recursive`/`valid.contains` que ARRAY_REF; un número
en un campo any se valida y se salta — probado 0x40000001 en `samples/AnyNumGc.bp`). NO reproducible
desde BP (el scan conservador de pila rescata el objeto siempre — ni deep-recursion+GC_EVERY=1 en el
build buggy lo recolecta) → defensa-en-profundidad. Verificado por MECANISMO: diff .mod fixed-vs-buggy
= EXACTAMENTE el bit de ref del campo (`samples/AnyGcHard.bp`). Radio stdlib = SOLO Gui (Component.__win,
único campo Object escalar; Gui NO está en los blobs embebidos → sube por el FS del IDE). JUnit
lexer-java + miVM 34/34; ParseTest paridad dual-VM. _(censo original abajo)_

<!-- CENSO ORIGINAL (histórico): -->
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

### ⚪ #3 DEGRADADO a higiene (15-jul) — `bpvm_handle_register` realloc sin zero-init
El `realloc` de `handle_addr`/`handle_gen` no cerea `[old_cap,new_cap)` (sigue así, verificado
15-jul en `heap.c:362-364`). **Su premisa se DESMONTÓ:** decía "device = SRAM basura → gen
deforme, región exacta del bug del concat de la Pico". Pero ese bug era **#15 (el alocador)**, y
la teoría de la memoria sin inicializar se descartó **con datos** (`BPVM_POISON=1` llena el
buffer de 0xAA y NO reproduce). Hoy **no muerde**: los lectores se guardan con `idx<handle_next`,
así que nunca se lee un slot no escrito. Queda como higiene barata (un `memset` al crecer), no
como candidato a bug. **Lección: era un candidato construido sobre una teoría, no sobre una
prueba — y la teoría era falsa.** _(barrido C)_

### 🟡 #4 ABIERTO, diferido (=H4.b) — `bpvm_aot_helpers.c` no handle-aware
Los accesores de objeto/ref-array del AOT nunca se ensancharon a 8B → fabrican handles
truncados → el contrato B los caza. **Es la causa ÚNICA de los 9 tests rojos** (§6): todos dan
`use-after-free` y `test-throwmsg` además segfaultea (139). Solo afecta al camino `.mdn` (AOT);
**no bloquea** porque el default de envío es intérprete. Ya en [[v4-pendientes-diferidos]].
**Es la deuda más gorda que queda en V4.** _(barrido C)_

### 🆕 ✅ #15 ARREGLADO (`5ea0155`, 15-jul) — el ALOCADOR regalaba la astilla → el GC barría objetos VIVOS
**El bug más grave del censo, y NO es de refs — por eso este catálogo por poco no lo encuentra.**
Un bloque asignado no guarda su tamaño: `block_total_size()` lo RECALCULA desde type/length.
`try_allocate_inner`, al servir de la free-list un bloque con sobrante `< MIN_FREE_BLOCK` (no
representable), **se lo regalaba al bloque asignado** → tamaño físico ≠ recálculo → el recorrido
del heap se quedaba corto, leía payload como cabecera y **descarrilaba EN SILENCIO** → mapa/set
de cabeceras truncado → `is_heap_ref` rechazaba objetos **VIVOS** → barridos → UAF cientos de
asignaciones más tarde. **Estaba en LAS DOS VMs** (en miVM latente: su heap por defecto es
grande; estrechándolo reproduce → UAF en el concat 137). Explica **S3** y el residuo de **S1**.
Fix: aceptar el bloque solo si encaja exacto o el resto es representable. **+ GUARDIÁN permanente
en ambas** (1 comparación por GC): si el recorrido no aterriza en `heap_next` → grita `[gc] !!
HEAP INCONSISTENTE`. Repro: `bpgenvm-c --mem=131072 MemT4d.mod`. Verificado en placa (Pico).

### 🆕 ✅ #16 + #17 + #18 ARREGLADOS (`7896494`, H1.8 fase globales) — 3 globales-ref a 4B
La conversión centralizadora (`declareGlobalByType`, por TIPO vía `occupies8Bytes`) cazó
**TRES** sitios, no uno. Los 3 verificados en el disasm (4B → opcodes `_L` de 8B):
- **#16** `:857 emitModulePropertyBacking` — el sospechado. `public property nombre: string`
  emitía `GET/SET_GLOBAL -4` sobre un handle de 8B. CONFIRMADO y arreglado.
- **#17** `:1018` **var ESTÁTICA de clase** — el hermano que estaba sin verificar. `var
  Caja.etiqueta: string` iba a 4B; su gemela (la static *property* de `:1023`) ya era
  width-aware. El patrón de #10, otra vez.
- **#18** `:235` **Mutex de las `sync property` de módulo — NO ESTABA EN EL CENSO.** El peor
  de los tres: el `__init` lo guarda con `emitGetLocal(newref)`, que empuja **8B** (el local
  sí era `declareLocalLong`), y `emitSetGlobal` popeaba **4** → handle truncado **+ fuga de
  4B en la pila**. Firma exacta de #10.

**⚠️ EL HALLAZGO DE VERDAD — el diff salió 0/39 (ni un byte) y eso NO es que el fix no haga
nada:** regenerado TODO el corpus (25 stdlib + 14 samples), **ningún módulo usa esas tres
construcciones**. *Ese* era el agujero que las mantuvo vivas: nada las compilaba, así que
nada podía delatarlas — ni la paridad, ni los tests, ni este catálogo. **Un censo por lectura
no encuentra lo que el corpus no ejercita.** Tapado con `samples/RefGlobals.bp` (los 3 sitios
+ churn, paridad dual-VM byte-idéntica) → a partir de ahora, estrechar una ref cambia un .mod
y salta a la vista. Consecuencia buena: **stdlib y blobs NO cambian → sin device batch.**

<!-- entrada original de #16 (histórico): -->
### (orig) 🟠 #16 SOSPECHA sin probar (15-jul, EMISOR) — backing de `property` de MÓDULO a 4B
`MivmEmitter.java:857` (`emitModulePropertyBacking`): `w.declareGlobal(moduleBackingName(...))`
**sin mirar el ancho** → una `property` de módulo de tipo ref se declara a 4B. **Confirmado en el
disasm** (`samples`-sonda con `public property nombre: string` → `__prop_get_nombre` emite
`GET_GLOBAL -4` y `__prop_set_nombre` `SET_GLOBAL -4`, 4B para un string). Firma idéntica a #1/#10.
**PERO: NO se ha conseguido hacerlo petar** en 4 intentos (churn ligero, churn pesado T4-style,
400 asignaciones en bucle, `--mem` de 262144 a 40960; ambas VMs verdes). Explicación probable:
truncar pierde la GEN, y con gen=0 el ref truncado cuela — hace falta que ESE slot venga reciclado.
**HERMANO SIN VERIFICAR: `MivmEmitter.java:1018`** — `var` estática de clase, mismo `declareGlobal`
pelado, mientras que la static **property** de la línea 1023 SÍ es width-aware (`occupies8Bytes`).
Es literalmente el patrón de #10 ("se migró un camino y el gemelo se quedó"). Fix propuesto = el
centralizador de #1 llevado a los globales: que `declareGlobal` fuerce 8B si el tipo es ref, y que
sea IMPOSIBLE declarar un global-ref de 4B. _(F — pendiente: probarlo o descartarlo con un repro)_

### ⚪ #5 latente — `ModWriter.declareInstanceProperty`→`declareField(name,isRef)` (is8=false)
Si algún día se usa con `isRef=true` para una property-ref, crea otro campo-ref de 4B
(mismo bug #1). Hoy el path de properties del emisor usa el `declareField` de 4 args
width-aware → probablemente sin uso para refs. Lo cubre el fix centralizador de #1. _(barrido D)_

### ✅ DESCARTADO — S2 (`i*8` vs `i*4` del GC) NO es bug
Son conceptos distintos y correctos: `i*8` indexa ELEMENTOS de ref-array (8B c/u); `i*4`
indexa SLOTS de 4B de objeto (un campo-ref ocupa 2 slots). El `(int)readI64` del MARCADO es
inocuo (`refDeref` ignora la gen; solo necesita idx→addr). Simétrico en ambas VMs. _(barrido D)_

---

## 6. LÍNEA BASE DE LA BATERÍA *(medida 15-jul — para no volver a preguntarse "¿esto ya estaba rojo?")*

**Medida DOS veces, con y sin el fix de #15 → los rojos son IDÉNTICOS = preexistentes.**
Cualquier rojo NUEVO respecto a esta lista es una regresión de verdad.

| Suite | Verde | Rojo |
|---|---|---|
| `make test-*` (VM-C, 25 targets) | **16** | **9 — TODOS = #4 (AOT)** |
| JUnit miVM | **34/34** | 0 |
| JUnit lexer-java | verde | 0 |
| Tren de memoria (MemT1-T5 + MemStress × 4 tamaños de heap) | **24/24** | 0 |

**Los 9 rojos, todos con la misma firma (`use-after-free`; `test-throwmsg` además SIGSEGV=139):**
`test-method` · `test-callbp` · `test-bytenat` · `test-xmodule` · `test-xmethodnat` ·
`test-throwmsg` · `test-throwuser` · `test-compressnat` · `test-compressbench`
→ **causa única: #4**. Ojo, `test-xmodnat` SÍ es verde: no vale con decir "todos los AOT".

**Cómo se mide** (el `make` no distingue skip de fallo; hay que mirar el exit code):
```
for t in $(grep -oE '^test-[a-z0-9-]+' Makefile | sort -u); do make $t >/dev/null 2>&1; echo "$t exit=$?"; done
```

---

## 7. Lecciones del método *(lo que de verdad cambió el resultado)*

1. **⚠️ `BPVM_GC_EVERY=1` ENMASCARA los bugs de GC, no los expone.** Fuerza GC en cada alloc →
   el heap nunca se llena → la free-list nunca se ejerce. Nuestro "estrés de GC" era MÁS DÉBIL
   que el caso real. **Para estresar el GC hay que ENCOGER el heap** (`--mem` en VM-C,
   `stackBase` en `BpVM.cfg` en miVM). Así cayó #15.
2. **⚠️ "Es de placa" es casi siempre falso — pregunta qué hace distinto el host y anúlalo.**
   El host por defecto (512K) nunca llenaba el heap, así que el GC natural NUNCA corría. Costó
   días de teorías de SMP/ARM/SRAM, todas descartadas después CON DATOS.
3. **⚠️ Un test que pasa no prueba que el código sea correcto.** #16 pasa en las dos VMs y su
   bytecode es demostrablemente de 4B. Truncar una ref pierde la GEN, y **con gen=0 el ref
   truncado cuela**: por eso toda esta familia es LATENTE hasta que hay reuso de slots. El
   disasm es más fiable que el verde.
4. **⚠️ Artefactos rancios: la trampa recurrente de este proyecto.** BpIde empaquetaba un
   compilador pre-4→8B (días perdidos); `make` tiene granularidad de 1s y no recompila si
   encadenas comandos en el mismo segundo (conclusión falsa a media verificación). **Al tocar
   miVM hay que `mvn install` + repackage BpIde**; al medir un fix, `touch` y build separado.
5. **⚠️ Un catálogo desfasado es peor que no tenerlo.** S1 y #10 figuraban abiertos estando
   arreglados. **Al arreglar algo, actualizar aquí en el MISMO commit.**
6. **El censo por CONTRATO funcionó** (encontró #1/#2/#6/#7/#8/#9/#10/#11/#14)… **pero #15, el
   más grave, no era de refs y se le escapó**: lo cazó un repro mínimo con el heap encogido.
   Censar por contrato Y estresar por comportamiento; ninguna de las dos basta sola.
