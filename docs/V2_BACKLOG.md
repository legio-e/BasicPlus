# BasicPlus — Backlog V2 (notas de diseño)

Documento vivo. Puntos a desarrollar en v2, **sin compromiso de scope
todavía** — son ideas con su contexto técnico para que arrancar v2 no
empiece de cero. Cada punto lleva: qué, por qué, implicaciones/coste, y
dependencias con otros puntos.

> Además de lo de aquí, ya hay tareas marcadas `[v2]` en el backlog
> principal: #138 cdc-multiplex, #140 debug-on-Pico real, #145 wifi-tcp,
> #153 dual-core RP2350 (con SWD), #169 AOT cross-module.

---

## ▶ Arranque de V2 — LEER PRIMERO  *(estado al cerrar V1, 2026-05-30)*

**V1 cerrada oficialmente**: tag anotado `v1.0` → commit `b92fb3f` en
`master`. Repo **local, sin remoto** (si algún día se publica:
`git push origin master v1.0`).

**Operativa de carpetas** (acordado): V2 continúa en `master`; V1 queda
congelada en el tag `v1.0`. Para tener una carpeta V1 física lado-a-lado
SIN duplicar ficheros: `git worktree add ../basicplus-v1 v1.0`. **NO**
copiar el árbol a carpetas `v1/` `v2/`.

**Pendientes operativos heredados de V1**:
- Recompilar el jar del IDE para que embeba el manual actualizado (§8.1
  native/AOT + §15.4 ejecución en MCU ya están en el fuente, commit b92fb3f).
- Test físico de GPIO en ESP32-S3: **diferido sine die** — la placa
  ESP32-S3-Touch-LCD-4.3 no tiene pines de usuario libres; necesita otra
  placa. El backend (`esp32/gpio_esp32.c`) está completo y commiteado.

**Primer frente de V2 (propuesto por el usuario, 2026-05-30 — H1)**: la
capa de **tipos** — añadir `long`/`double` y revisar `byte` y strings
UTF-8. El análisis técnico ya está hecho en **§1–§4** de este documento
(coste, opcodes, dependencias). Hechos confirmados por exploración del
código (mayo 2026):
- Slot de stack/local/global = **4 bytes (int32)** en AMBAS VMs (C y Java);
  float = f32 bit-casteado en el mismo slot. No hay unión etiquetada.
- `byte` ya existe (UINT8: máscara &0xFF al store, zero-ext al load,
  arrays I8 de 1 byte/elem) — falta hacerlo *first-class* + `byte[]` cómodo.
- Strings = array de **codepoints i32** (4 bytes/char), indexados por
  codepoint. NO hay UTF-8 en heap; la salida trunca no-ASCII a `'?'`.
  → el modelo interno YA es Unicode; el hueco está SOLO en los bordes de I/O.
- **Dependencia dura**: `byte[]` ANTES que UTF-8 (los strings UTF-8 se
  implementan COMO byte[]).

### Roadmap V2 confirmado (2026-05-30) — el «trío que toca la VM», al principio

Estrategia: **concentrar al principio de V2 todo lo que toca la VM**, y
después tocarla lo mínimo. Tres hitos seguidos:

- **H1 — escalares** ✅ **CERRADO (2026-05-31)**: `byte`/`byte[]`
  first-class + `long`/`double` + casts numéricos + pasada de portabilidad
  `PRIu32`. Cambia la representación de valores.
- **H2 — strings UTF-8** ✅ **CERRADO (2026-05-31)**: string = `byte[]`
  UTF-8 (§4), índice por codepoint, conversión `string↔byte[]`. Construido
  sobre el `byte[]` de H1.
- **H3 — GC** ✅ **CERRADO (2026-05-31)**: allocator **non-moving** con
  free-list (first-fit + split + coalescing) + **GC proactivo por umbral**
  + **heapNext-retreat**, en AMBAS VMs (la VM C era bump-sin-reuse → ahora
  reusa de verdad). Compactación (moving) diferida, con pinning ya
  documentado, solo si un workload real exhibe la patología. Ver §8 +
  `h3bench/NOTAS.md`.

**Con H3 cerrado, el «trío que toca la VM» está COMPLETO y la VM queda
efectivamente congelada.** Lo que sigue se construye encima sin tocarla:

- **H4 — librerías BP** (diseñado 2026-05-31; **primer hito post-VM**). Es
  el lote de stdlib de esta tarde. Se organiza por su **impacto en el
  COMPILADOR**:
  - **H4.A — NO afecta al compilador** (BP puro, **programable YA**, no
    bloquea con nada):
    * **String**, subconjunto sin tuplas: `longToString`, `doubleToString`,
      `formatDouble`, `padLeft`/`padRight`, `toHex`, `lastIndexOf`,
      `fromCharCode`, `compare` (§14b).
    * **`Map`** (§14c) — pura BP sobre `List` + `synchronized`; usa `compare`.
    * **`Stats`** (§14d) — pura BP; `Accumulator` (Welford) + funciones
      sobre `double[]`; usa la clase `LinFit` mientras no haya tuplas.
  - **H4.B — SÍ afecta al compilador** (requiere feature de lenguaje antes):
    * **Tuplas (§2)** — prerrequisito (campos nombrados).
    * Luego los `parse*` (`parseLong`/`parseDouble`/`parseHex`) que devuelven
      `{err, valor}` + unificar `parseInt`/`parseFloat` a tupla (§14b).
  - **Orden natural**: **H4.A ya** (no bloquea nada); **H4.B** cuando se
    prototipen las tuplas.
  - **NO entran en H4** (son "posterior a H4, sin turno"): TCP (§13),
    `Compress`/`Archive` (§14e) y parámetros por defecto (§10).

**Estado H1 (cerrado 2026-05-31)** — entregado y validado en doble VM
(miVM Java ↔ bpgenvm-c C, salida idéntica) + compilación limpia en Xtensa
(ESP32-S3, oráculo de portabilidad) y host gcc:
- **H1.1** `byte`/`byte[]` first-class (`newByteArray`, dispatch de ancho
  en subíndice; el `byte` en stack es i32, máscara &0xFF al store). Bug de
  `byte[]` cross-module en `.bpi` corregido de paso (`uint8`/`int8`/
  `uint16`/`int16` no se reconocían al parsear el tipo serializado).
- **H1.2** `long` (i64) completo: locals/params/returns/globals/`long[]`
  vía contabilidad de 2 slots + truco RET-operand-as-slot-count
  (retrocompatible) + LRET para retornos de 8 bytes. Opcodes 0x71–0x90,
  `OperandKind` IMM_I64, `TYPE_ARRAY_I64`.
- **H1.3** `double` (f64) reusando TODA la maquinaria de 8 bytes de `long`
  ("is8Byte") + casts numéricos `integer()/long()/float()/double()`
  type-directed + promoción aritmética (double>float>long>int). Opcodes
  0x91–0xA7.
- **H1.4** portabilidad `PRId32`/`PRIu32` (`inttypes.h`) en los 12 sitios
  de diagnóstico con args int32_t/uint32_t; eliminada la supresión
  `-Wno-error=format` del CMakeLists del ESP32 → build Xtensa limpio.
- **Principio honrado**: el contenedor `.mod` NO cambió — solo opcodes y
  tipos *aditivos*. La VM se tocó al principio de V2, como estaba previsto.

**Estado H2 (cerrado 2026-05-31)** — string pasó de "array de codepoints i32"
a **`byte[]` UTF-8** (`TYPE_ARRAY_I8`), validado en doble VM (salida Unicode
byte-idéntica: `café`, `ñ`, `€`):
- **H2.1** representación núcleo: string = `TYPE_ARRAY_I8` UTF-8 en ambas VMs
  (reusa la maquinaria I8 de H1.1) + literal `.mod` UTF-8 + print directo (la
  VM C deja de truncar a `?`) + `Main` Java fuerza stdout/stderr a UTF-8.
- **H2.2** índice por codepoint: `strlen`/`charAt`/`charCodeAt`/`substring`/
  `indexOf` operan en codepoints (decodifican UTF-8; ASCII idéntico a V1).
- **H2.3** helpers AOT de string a UTF-8; helpers `utf8_*` como fuente única
  en `bpvm_internal.h` (bytecode ↔ native coinciden byte a byte).
- **H2.4** conversión `toBytes(s):byte[]` / `fromBytes(b:byte[]):string`
  (copia defensiva; string y byte[] comparten layout I8).
- **H2.5** stdlib recompilada (13 módulos), docs (HEAP_LAYOUT §6, MOD_FORMAT
  §5, BUILTINS) y debugger (`readStringIfPossible`) actualizados.
- **Decisión clave** (usuario): índice por codepoint + bytes crudos vía
  `string↔byte[]` (no `byteAt` en el string). El contenedor `.mod` NO cambió.
- **Gaps registrados (pre-existentes, NO de H2)**: (a) la VM C implementa solo
  un *subset* de builtins — IO filesystem (`pathBasename`…) y algunos de Math
  son Java-VM-only; si se portan a C, los de string deben respetar codepoint;
  (b) los arrays no exponen longitud al usuario (`len`/`.length`) — solo
  for-each; afecta a la ergonomía de `byte[]`; (c) el disassembler (`Disasm`)
  aún detecta literales con la heurística vieja (codepoints) → degrada a "no
  detectados", el disasm de opcodes no se ve afectado; (d) la stdlib **embebida
  en firmware** (Pico/ESP32) sigue con literales viejos → re-embeber al tocar
  firmware. Candidatos a backlog de robustez/paridad.

**Por qué este orden** (no es "robustez después"): H1 y H2 cambian la VM y
*añaden tipos de objeto nuevos* (I64/F64/byte[], string-UTF-8). Hacer el GC
(H3) DESPUÉS → el nuevo allocator ve el zoo de objetos FINAL y se afina UNA
vez; hacerlo antes obligaría a re-tocarlo al añadir los tipos. Tras H3 la VM
queda **efectivamente congelada**; el resto de V2 (interfaces, stdlib, IDE,
multi-MCU, debugger) se construye encima sin tocarla.

### Principios de arquitectura (confirmados 2026-05-30)

- **La estructura del `.mod` es la base INMUTABLE.** Es lo que permitió
  todos los saltos de V1 (backend → VM propia → micro → 2º micro, sin
  recompilar código de usuario). Aunque V2 toque la VM, el **contenedor**
  del `.mod` (cabeceras, secciones, constant pool, descriptores de clase)
  NO cambia. Solo se retoca lo esencial; cambiarlo exige análisis a fondo +
  decisión conjunta. ⚠️ **Matiz clave**: *añadir opcodes/tipos nuevos*
  (LOAD64, DPUSH, aritmética 64-bit…) NO es "cambiar la estructura" — el
  bytecode es aditivo y versionado (loaders viejos saltan opcodes
  desconocidos). **H1 añade opcodes 64-bit sin tocar el contenedor.**
- **La VM se toca al PRINCIPIO de V2 y luego se congela.** Las
  modificaciones de VM se concentran en H1–H3; después, tocar la VM lo
  mínimo (principio de siempre). Si el futuro demuestra que hay que cambiar
  la estructura de módulos o la VM, se analiza a fondo y se decide — nunca
  por inercia.

---

## 0. Visión y principios de la V2  *(reflexión de cierre de V1)*

> Las reflexiones, salvo tonterías, conviene guardarlas: aportan el
> "por qué" que el "qué" de los puntos siguientes no cuenta.

### Mirando atrás: lo que demuestran los saltos

V1 empezó como un compilador modesto (ASM → bytecode Java para PC) y dio
cuatro saltos: **VM propia** → **VM en microcontrolador** → **segunda
familia de MCU**. Lo importante no es el esfuerzo de cada salto, sino que
cada uno **validó una decisión de arquitectura anterior**:

- VM propia ← el compilador estaba desacoplado del backend (cambiar de
  emitir JVM a emitir `.mod` no rehizo el front-end).
- VM en micro ← la VM se diseñó con *caller-provided buffer* + C99 (C0).
- Segundo micro ← la abstracción de HW (transport, platform_\*) **aguantó**.
  Una VM que corre en una sola placa no prueba nada; en dos familias,
  prueba que la frontera está bien puesta.

El activo real de V1 no es "un lenguaje que funciona", sino **una
separación limpia (compilador / bytecode / VM / frontera HW) que ha
resistido cuatro saltos**. Eso es lo que hace viable todo lo de adelante.

### ¿Se puede usar en una solución real? — sí, con matices

El compilador y la VM funcionan bien → por ahí sí. El límite es el **GC**:
es una solución de compromiso (mark-sweep **bump-sin-reuse**) que no
aguanta estrés de heap. Pero — punto clave — **eso es un problema de la
implementación de la VM, no del lenguaje.**

Esa distinción es la mejor noticia técnica del proyecto: se puede cambiar
el modelo de memoria entero **sin tocar el lenguaje y sin recompilar el
código de usuario** (el `.mod` es estable). Es la recompensa de haber
separado bien.

Victoria concreta y contenida esperando ahí: hoy el GC marca FREE pero
**no reusa**. Solo añadir **reuso por free-list** al mark-sweep existente
quita el acantilado "estrés de heap = OOM" — sin tocar lenguaje, formato
ni compilador. Es el cambio de **mayor palanca por línea** de toda la V2.

### Robustez es más que el GC

Para *"que cualquier usuario lo use con confianza"*, la palabra clave es
**predecibilidad**, no solo "que no se quede sin memoria". Todo esto es
VM, no lenguaje (lo cual refuerza la tesis):

- **Latencia acotada** — un STW GC pausa; para un lazo de control eso
  puede ser inaceptable. La pregunta de V2 no es solo "¿reusa?" sino
  "¿cuánto pausa como máximo?".
- **Recuperación de fallos** — #186 fue el primer paso (un fault ya no
  corrompe). "Confianza" = OOM / stack overflow / etc. contenibles y
  diagnosticables, no "se muere el thread".
- **Límites de recursos** — memoria y stacks acotados y declarables;
  en un micro no puedes "pedir más".

### Replanteo: robustez y docs PRIMERO, ampliación después

La V2 nació como "ampliación". Pero para el objetivo real —
**gratis, abierto, que la gente confíe y contribuya**— la robustez y la
documentación no son un complemento de la ampliación: son **la condición
para que la ampliación importe**. Un lenguaje pequeño, sólido y bien
documentado se adopta; uno lleno de features pero frágil, no. Encaja con
el principio de siempre: *en el lenguaje, pocos cambios*; la energía de V2
va a la VM y a los docs.

Tres pilares, **en este orden**:

1. **Robustez** — modelo de memoria (free-list → GC mejor → quizá
   latencia acotada), recuperación de fallos, límites de recursos. Que
   sea de fiar.
2. **Documentación doble** — manual de usuario **+** doc técnica de
   internals. Lo segundo es lo que habilita contribuciones externas:
   nadie contribuye a una VM que no entiende. Base ya empezada
   (MOD_FORMAT, OPCODES, HEAP_LAYOUT, BUILTINS, AOT_CROSS_MODULE,
   SMP_ARCH) → consolidar y completar.
3. **Ampliación** — el resto de este backlog. Más capaz, sin prisa, sin
   comprometer 1 y 2.

### Para el open source

Lo que más baja la barrera de contribución no es el código limpio (que lo
está), sino **un documento de arquitectura de una página** que cuente la
historia de arriba: "compilador que emite `.mod` → VM que lo interpreta →
AOT → frontera HW". Ese mapa mental es lo que falta para que alguien
externo sepa *dónde* tocar. Es el primer doc a escribir cuando arranque
la fase de documentación.

**Objetivo último**: que cualquier usuario lo use con confianza, gratis;
código abierto; que otros prueben y, si quieren, aporten código.

---

## 1. Tipos de 64 bits: `long` (i64) y `double` (f64)

**Qué**: dos primitivos nuevos de 64 bits, paralelos a `integer`(i32) y
`float`(f32).

**Por qué**: rango entero grande (timestamps, contadores, IDs),
precisión doble para cálculo numérico serio.

**Implicaciones / coste (medio-alto, camino conocido)**:
- El stack BP es de **slots de 4 bytes**. i64/f64 ocupan 2 slots → o
  bien opcodes nuevos que operan sobre 2 slots contiguos (LOAD64/STORE64,
  aritmética i64/f64, conversiones i32↔i64 / f32↔f64), o un rediseño del
  ancho de slot. Decisión de diseño: **2 slots contiguos** es lo menos
  invasivo y compatible con el resto.
- Frontend: tipos `long`/`double`, literales (`123L`, `1.5`), reglas de
  coerción y promoción (i32→i64, f32→f64).
- Heap: arrays `long[]`/`double[]` (8 bytes/elem → TYPE_ARRAY_I64 /
  TYPE_ARRAY_F64).
- AOT: helpers `read/write_i64_be` y `read/write_f64_be`; el thunk pasa
  2 slots por arg/return de 64 bits. La ABI aot_helpers crece (slots al
  final, como siempre).

---

## 2. Tuplas  *(✅ CONFIRMADA — mejora de compilador/lenguaje, 2026-05-31)*

**Decisión (usuario, 2026-05-31)**: SÍ se quieren — útiles para devolver 2+
valores de una función (caso concreto que lo motiva: `Stats.linfit` →
`(slope, intercept)`). **Enfoque: tupla = objeto heap sintético** (clase oculta
`__TupleN` con campos), usando opcodes existentes (`NEW_OBJECT`/`SET_FIELD`/
`GET_FIELD`) → **CERO coste de VM** (principio "lo que hace el compilador se lo
ahorra la VM"). El coste real está en el **frontend** (tipos tupla, inferencia,
destructuring `var (x,y) := f()`, igualdad estructural) + 1 alocación pequeña por
tupla (despreciable en retornos ocasionales; solo importaría en bucles muy
calientes). Una tupla *alloc-free general* pediría un `RET` multi-valor
(mini-cambio aditivo de VM); curiosidad: el caso 2×4-byte cabría gratis en el
retorno de 8 bytes existente (`LRET`). **Plan: prototipar el caso mínimo**
(retorno de 2 valores + destructuring en `var`) como objeto heap, evaluar,
ampliar. Mientras tanto, las funciones que devuelven 2 valores usan una clase
pequeña (p.ej. `Stats.LinFit`) y migran a tupla sin romper a nadie.

**Actualización (usuario, 2026-05-31) — pasa de "algún día" a PRERREQUISITO**:
- **Consumidores que la disparan**: (1) `Stats.LinFit` `(slope, intercept)`;
  (2) **las funciones `parse*` de string** — decisión del usuario: los `parse*`
  NO lanzan excepción (try/catch es una pesadez para input que falla por
  diseño), **devuelven una tupla `{ err, valor }`** estilo Go. Con ≥2
  consumidores reales, las tuplas dejan de ser opcionales.
- **Las tuplas deben tener CAMPOS NOMBRADOS** (record-like): acceso `r.err`,
  `r.valor`, no solo posicional `r.0`/`r.1`. (El usuario escribió `{err, valor}`.)
- **Bloqueo**: la tanda de **stdlib de string** (los `parse*`) **depende de las
  tuplas**. Camino elegido = **(a) tuplas primero, luego `parse*`** (tipado
  limpio, `r.valor` es el tipo numérico real, sin `any`). Alternativa (b)
  descartada salvo prisa: clase transitoria `ParseResult { err: boolean,
  valor: any }` con la misma ergonomía, a migrar luego.
- **Contenido de `parse*`**: `err: boolean` (true = falló); el `valor` solo es
  válido si `err` es false. (Para parsear no se necesita el *por qué* del fallo.)

**Qué**: tipo producto anónimo `(a, b, c)`. Casos de uso: retorno
múltiple de funciones, agrupación ligera sin declarar una clase.

**Por qué**: ergonomía — `var (q, r) := divmod(a, b)` en vez de objetos
ad-hoc o parámetros out.

**Implicaciones / coste (el más incierto — necesita prototipo antes de
comprometerse)**:
- Decisiones de diseño abiertas:
  * ¿Heap-allocated (objeto con N campos tipados) o multi-valor en
    stack? La opción **más barata**: tupla = objeto heap inmutable con
    campos tipados → reusa NEW_OBJECT / GET_FIELD existentes; el retorno
    múltiple es azúcar sobre eso.
  * ¿Inmutables? (recomendado, encaja con el modelo).
  * ¿Destructuring en `var (x, y) := ...` y en parámetros?
  * ¿Igualdad estructural?
- El punto que más se complica es el **sistema de tipos del frontend**
  (inferencia de tipos tupla, firmas, unificación). El runtime es fácil
  si se modela como objeto heap.
- **Recomendación**: prototipar primero el caso mínimo (retorno de 2
  valores + destructuring en `var`) y decidir scope a partir de ahí.

---

## 3. `byte` (u8) first-class + `byte[]`

**Qué**: `byte` como tipo u8 de pleno derecho y `byte[]` con
almacenamiento **compacto de 1 byte por elemento**.

**Por qué**: buffers de I/O (SPI/UART/I2C ya mueven bytes crudos),
parsers binarios, y — clave — es la **base de los strings UTF-8** (punto
4). En MCU, 1 byte/elem vs 4 bytes/elem importa mucho.

**Implicaciones / coste (bajo-medio — gran parte ya existe)**:
- Construye sobre **L10 (#22)**, que ya añadió `byte/word/short/int8/
  int16` al sistema de tipos estrechos. Y los helpers AOT ya tienen
  `newarray_i8` + el runtime un TYPE_ARRAY_I8.
- Falta: `byte` = u8 first-class con su semántica (0-255, wrap),
  `byte[]` con storage de 1 byte/elem (no 4), indexado read/write
  eficiente, y literales hex (`0xFF`) cómodos.
- AOT: helpers `array_load_i8` / `array_store_i8` (paralelos a los i32).

**Dependencia**: hacer ESTE antes que el punto 4 (strings UTF-8 se
construyen sobre byte[]).

---

## 4. Strings = `byte[]` inmutable, codificación interna UTF-8  *(el grande)*

**Qué**: cambiar la representación interna de los strings de "array de
codepoints (4 bytes/char)" a **byte[] UTF-8 inmutable** (1-4 bytes/char;
ASCII = 1 byte).

**Por qué**:
- **Memoria**: ~4× más compacto para texto ASCII (decisivo en MCU).
- **Corrección Unicode**: UTF-8 real en vez de "ASCII o '?'" del v1.
- **Inmutabilidad**: garantizada por el tipo — concat/substring crean
  strings nuevos, nunca mutación in-place. Modelo más limpio (estilo
  Java/Rust/Go).
- Alinea con el resto: el **wire v1 / JSON del IDE ya es UTF-8**.

**Implicaciones / coste (ALTO — probablemente el item más grande de V2;
toca VM + frontend + builtins + AOT + posiblemente el .mod)**:
- **Hoy** un string es TYPE_ARRAY_I32 de codepoints `[len][cp]×len`.
  V2 sería un `byte[]` UTF-8 `[nbytes][b0][b1]...`.
- **Semántica de índice — decisión a tomar**: `length` y `charAt`
  ¿operan por codepoint (requiere decodificar UTF-8, charAt(i) es O(n))
  o por byte (O(1) pero rompe la noción de "carácter")? Propuesta:
  `length` en codepoints O(n) cacheable, iteración por codepoint para
  recorrer, y exponer `byteLength`/acceso a bytes para casos crudos.
  Definir la API antes de implementar.
- **Se reescriben TODOS los ops de string**: concat, substring, ==,
  print, parseInt, charAt/charCodeAt, intToString.
- **⚠️ Interacción con #173 (AOT strings de v1)**: los helpers
  `string_concat`/`char_at`/`substring`/`eq`/... que estamos añadiendo
  en v1 ASUMEN el layout de codepoints. Al pasar a UTF-8 byte[], **la
  implementación interna de esos helpers se reescribe** — pero la
  arquitectura (emitter → helpers vía ABI) y las firmas se conservan, así
  que el trabajo de #173 no se tira: solo cambia el cuerpo de los
  helpers. Bueno tenerlo presente al cerrar #173.
- **.mod**: los literales de string en el data block pasarían a UTF-8
  (más compactos).

**Dependencia**: requiere el punto 3 (`byte[]`) primero.

---

## Orden sugerido (cuando arranque v2)

1. **`byte`/`byte[]`** (#3) — barato, desbloquea strings y mejora I/O.
2. **Strings UTF-8** (#4) — construido sobre byte[]; el grande.
3. **`long`/`double`** (#1) — independiente, camino conocido.
4. **Tuplas** (#2) — prototipo primero, scope según resultado.

(El orden no es rígido; #1 y #2 son independientes de #3/#4.)

---

# Lenguaje / OO / compilador

**Principio rector** (del usuario): NO meter muchos cambios en el
lenguaje — solo añadir algo cuando se eche en falta de verdad.
Minimalismo: cada feature nueva tiene que ganarse su sitio.

## 5. Interfaces a nivel de clase (programación por contrato)

**Qué**: interfaces Java-style sobre clases — `interface Drawable` con
firmas de método (sin cuerpo); `class Circle implements Drawable,
Comparable`. Una variable puede tiparse como la interfaz y despachar al
método de la clase concreta.

**Por qué**: el usuario las usa en Java y le gusta la programación por
contrato. Dan polimorfismo limpio y múltiples supertipos sin herencia
múltiple (ver #6).

**Estado actual — OJO, no confundir**: BP YA tiene "contracts" pero a
nivel de **MÓDULO** (`ModuleNode.isInterface` + `implementsName`, modo
de compilación `--interface`, `setImplementsContract` — estilo Modula-2
DEFINITION MODULE). Eso NO es lo mismo: lo de v2 son interfaces de
**CLASE**. Se puede reusar parte de la maquinaria conceptual, pero es
una feature distinta.

**Implicaciones / coste (bajo-medio)**:
- VM: el runtime YA tiene vtable dispatch (L2 v3). El único reto real es
  el **itable** (interface method table): mapear "método k de la
  interfaz I" al slot de vtable de la clase concreta que la implementa.
  Con single inheritance + interfaces NO hay problema del diamante.
  Opciones: itables por interfaz, o un esquema unificado nombre→slot.
- Frontend: parse `interface`; semántico (implements-checking: la clase
  provee todas las firmas con tipos compatibles; la interfaz es usable
  como tipo en firmas, variables, params); `.bpi` exporta las firmas de
  interfaz (encaja con las class-entries de L2.1).
- **Sub-opción** (DbC de verdad, estilo Eiffel: pre/postcondiciones +
  invariantes): es OTRA capa, independiente de las interfaces (asserts
  inyectados en entry/exit de método). Anotado por si se echa en falta;
  no es prerequisito de las interfaces.

## 6. Herencia múltiple — recomendación: NO; cubrir con interfaces (#5)

**Qué pregunta el usuario**: ¿permitimos herencia múltiple de clases?
¿cómo de complejo es a nivel de VM?

**Respuesta de diseño**: MI real de clases es **caro y propenso a
errores** a nivel de VM — problema del diamante, múltiples vtables,
ajuste del puntero `this` (thunks), conflictos de layout de campos
(C++ lo resuelve con vptr por base + ajuste de this). Mucha complejidad
en VM **y** frontend.

**Recomendación**: NO hacer MI de clases. Adoptar el modelo Java/C#:
**single inheritance de clases + múltiples interfaces** (#5). Eso da el
~90% del valor (múltiples supertipos, polimorfismo) SIN el diamante de
campos. Como el usuario ya quiere interfaces, esto cubre la necesidad.

Si en el futuro se echa en falta **reutilizar implementación** desde
varios sitios (no solo firmas): "default methods" en interfaces (estilo
Java 8) o mixins/traits — ambos mucho menos invasivos que MI de campos.
Anotar como escalón posterior, solo si hace falta.

## 7. Compilador: recuperación de errores (anti-cascada)

**Qué**: que un error no genere una cascada de errores espurios — el
problema de usabilidad #1 del compilador hoy.

**Por qué**: fricción diaria. Un fallo arriba debería dar 1 error claro,
no 20 derivados que tapan el real.

**Estado actual**: la infraestructura está EMPEZADA pero no aplicada a
fondo:
- Parser: ya hay `synchronize()` + `synchronizeToFunctionEnd()`
  (#115/#116) — panic-mode básico.
- Semántico: ya existe `ErrorType` (en BpType + SemanticAnalyzer).

**Las dos fuentes de cascada y su fix**:
- **(a) Parser** — tras un error de sintaxis se desincroniza y emite
  espurios. Reforzar: sync robusto a fronteras de statement/bloque/
  declaración, error productions, y no re-reportar dentro de una zona ya
  marcada como rota.
- **(b) Semántico** — tras un símbolo no definido o tipo incompatible,
  los usos posteriores generan MÁS errores. Fix clásico: **poisoning**
  — a la expresión fallida se le asigna `ErrorType`, que es compatible
  con todo → corta la propagación. El tipo ya existe; falta aplicarlo
  **consistentemente en TODOS los caminos** (lookup de símbolo fallido,
  tipo incompatible, método/propiedad inexistente, arg count, ...).
- **(c) Dedup / cap** — el mismo error en la misma línea una sola vez;
  límite de N errores por compilación con "... y M más".

**Coste**: medio, pero **ALTO valor de usabilidad**. Ortogonal al
sistema de tipos (no depende de #1-#6). Buen candidato a hacer pronto en
v2 porque mejora el día a día de escribir BP.

---

## 8. Funciones de primera clase / callbacks + eventos

**Qué**: poder pasar una función como parámetro (callback). Hoy BP **NO
lo tiene** (confirmado: `BpType.Kind` no tiene `FunctionType`; el Parser
no tiene lambdas ni referencias a función).

**Por qué (necesidad real, no estética)**: surge de dos casos concretos:
- **Comparador a medida** para el `Map` (orden de keys distinto al
  string por defecto) y un futuro `sort(list, cmp)`.
- **Eventos / listeners** (observer): `widget.onClick(handler)`. No
  tenemos NADA de eventos todavía, y un sistema de eventos necesita
  callbacks por definición.

**Stopgap HOY (cero coste de VM)**: el patrón **objeto‑callback** (estilo
`Comparator`/`Runnable` de Java pre‑lambdas): una clase con un método
(`compare(a,b)`, `onEvent(...)`) que se pasa como objeto y se invoca por
`INVOKE_VIRTUAL` (maquinaria existente). Es lo que usará el `Map` para el
comparador. **Funciona ya**, pero es verboso (una clase por callback) →
encaja con las **interfaces (#5)**: un `interface Comparator` / `Listener`.

**Feature de lenguaje (quitar el boilerplate)** — en fases:
- **Fase A — funciones nombradas como valor**: pasar una función top-level
  o método como valor (`onClick(miHandler)`). Necesita: tipo función en el
  frontend (firma) + un opcode **aditivo** `CALL_INDIRECT` en AMBAS VMs
  (pop dirección destino → llama). Aditivo = NO toca el contenedor .mod
  (consistente con "VM mínima"; como los opcodes de H1). Coste: bajo‑medio.
  **NO es cero‑coste** (a diferencia de tuplas), pero es pequeño.
- **Fase B — lambdas / clausuras**: funciones anónimas inline, con captura
  de contexto. Mucho más pesado (entorno en heap para las variables
  capturadas, lifetime/GC). Azúcar deseable, **diferir**.

**Decisión / orden recomendado**:
1. Ahora: el `Map` usa **objeto‑comparador** (stopgap, cero VM). No bloquea.
2. Cuando lleguen las interfaces (#5): `interface Comparator`/`Listener`.
3. Más adelante, si el boilerplate molesta: Fase A (función‑valor +
   `CALL_INDIRECT`). Lambdas (Fase B) solo si se justifica.

**Relación**: habilita un futuro **sistema de eventos** (§9) y un `sort`
con comparador. Pareja natural de tuplas (§2) como "mejoras de
lenguaje/compilador que no urgen pero se acumulan".

---

## 9. Interrupciones de hardware → eventos BP (Modelo B)

**Qué**: que un módulo de HW que necesite interrupciones (flanco GPIO,
alarma de timer, etc.) pueda invocar **una función handler en BP** sin que
el usuario programe ISRs a mano.

**Regla dura (no negociable)**: NUNCA se ejecuta bytecode BP dentro de una
ISR real. El intérprete no es reentrante (heap, GC, pila, vtables) y una
interrupción puede saltar a mitad de instrucción **o a mitad de un GC**.
Lo que corre en contexto de interrupción tiene que ser C, corto y sin
alocar.

**Decisión del usuario (2026-05-31): Modelo B** (handler BP **diferido**),
porque es el más sencillo para el usuario. Se descartó construir un
"Modelo N" genérico (callback nativo registrado) como paso intermedio:
los casos que de verdad necesitan trabajo *dentro* de la ISR (contar
pulsos, DMA…) ya los cubre cada driver en su backend nativo, p.ej.
`Pulse` (`bpvm_pulse_set_backend`). → No invertir en N si el destino es B.

**Cómo (Modelo B)**:
1. **ISR fija en C** (una por plataforma, RP2350 / ESP32): minimalista —
   **encola un evento** (pin, flanco, timestamp) en un ring buffer y vuelve.
2. **Ring buffer**: reutilizar el `comm_queue` que YA existe (productor/
   consumidor con mutex+condvar; `bpvm_oq_*`).
3. **Drain en safepoint**: el scheduler, en el safepoint donde ya chequea
   el GC, drena la cola y **llama al handler BP como bytecode normal**
   (fuera de contexto de interrupción).
4. **Registro del handler**: vía el mecanismo de callbacks de §8 — hoy con
   **objeto‑listener** (`gpio.onEdge(pin, RISING, listenerObj)` con método
   `onEdge(pin)`); el día de mañana, función‑valor (§8 fase A).

**Coste / latencia**: la respuesta llega con la latencia de un safepoint
(µs–ms). Vale para "botón pulsado", **no** para tiempo‑real duro (eso es
trabajo nativo en el driver). Toca la VM (drain + dispatch en el
scheduler) → es un cambio **aditivo** y se hace **más adelante** (V2),
cuando se ataque la VM otra vez; no ahora ("descansamos de la VM").

**Dependencias**: §8 (callback — basta el objeto‑listener) + plumbing VM
(cola + safepoint drain) + ISR trampolín por plataforma. N y B pueden
coexistir (un encoder cuenta en nativo *y* postea "objetivo alcanzado"
como evento BP).

---

## 10. Parámetros con valor por defecto (compilador + `.bpi`, cero VM)

**Qué**: declarar params con default — `function f(x: int, y: int = 10)` — y
poder omitirlos en la llamada.

**Decisión del usuario (2026-05-31)**: SÍ; **solo defaults CONSTANTES**
(literales/const) — "con constantes es suficiente". No se permiten expresiones
arbitrarias (evita decidir el contexto de evaluación; mantiene todo simple).

**Mecanismo — sustitución en el LLAMANTE (modelo C++), CERO coste de VM**:
- El compilador, en cada call site, ve que faltan argumentos **finales** y
  **empuja los valores por defecto** antes del `CALL`. La VM ve una llamada de
  **aridad completa** → no se entera de nada (filosofía "lo que hace el
  compilador se lo ahorra la VM").
- **Reglas**: defaults solo en parámetros **finales** (no un defaulted antes de
  uno obligatorio); el default debe ser **constante** (sustituible en cualquier
  sitio).
- **`.bpi`**: la firma exportada **lleva los valores por defecto**, para que los
  llamantes **cross-module** puedan sustituirlos (igual que C++ los pone en el
  header). Esto es lo que el usuario intuyó que "afecta a los .bpi".
- **AOT (`.mdn`) NO se entera**: la función nativa recibe los args completos
  (la sustitución ocurre antes del `CALL`).

**Sinergia**: es la **solución general** a la limitación "BP no tiene
sobrecarga ni params opcionales" que se topó en §14b. Con defaults,
`doubleToString(x, dec = 6)` podría ser **una** función en vez de dos
(`doubleToString` + `formatDouble`). Al implementar §10, revisar §14b para
unificar.

**Coste**: bajo, todo en el frontend (parser acepta `= const`; semántico
rellena args faltantes en el call site; serializar/leer defaults en `.bpi`).
Ortogonal a tuplas (§2) y callbacks (§8).

**Planificación (usuario, 2026-05-31)**: más adelante, **sin turno asignado**
— posterior a H4 (H4 = lo ya hablado).

---

# VM / memoria / multi-MCU

**Principios rectores** (del usuario):
- **VM: tocar lo MÍNIMO.** Funciona muy bien y costó mucho. Cambios solo
  si no queda otra; nada drástico salvo necesidad real.
- **El compilador hace, la VM se ahorra.** Todo lo que se pueda resolver
  en tiempo de compilación NO debe gastar ciclos/memoria en runtime. Es
  la misma filosofía del AOT ("bytecode = pata negra, nativo = caché
  derivada") llevada al resto: offsets, símbolos, constantes, checks.

## 8. Ahorro de memoria (sin rediseños drásticos)

Palancas, de menos a más invasivas:
- **Strings UTF-8** (ver #4): hoy 4 bytes/char (codepoints); UTF-8 da
  ~4× en ASCII. Es el mayor ahorro "fácil" de datos. (Ya anotado en #4.)
- **Compile-time en vez de runtime** (#7-filosofía aplicada a memoria):
  - Folding de constantes, dead-code elimination → menos bytecode.
  - Offsets/símbolos resueltos en compilación (ya se hace para module
    globals #172) → menos tablas en runtime.
  - Bounds-check elimination cuando el índice es probadamente válido;
    devirtualización cuando el tipo concreto se conoce → menos trabajo VM.
- **Buffers estáticos del firmware Pico**: hoy ~3×128 KB (s_vm_buffer,
  s_data del FS, tmp de compactación) comen casi toda la SRAM. Revisar
  si se pueden compartir/reducir/configurar por placa.
- **GC con reuse — LA palanca grande, pero toca GC (semi-drástico)**:
  hoy el mark-sweep marca libre pero el bump NO reusa (limitación F2) →
  el heap solo crece. Una free-list o compactación reduciría
  drásticamente la presión de heap. ⚠️ Toca el GC → evaluar con cuidado
  contra "tocar la VM lo mínimo". NOTA: si la placa tiene PSRAM (#10),
  la urgencia de esto baja muchísimo (8 MB de heap).

  **→ H3 (V2): decisión de diseño (confirmada 2026-05-30).** El usuario
  eleva el GC a hito propio: **reimplementar el modelo de memoria por
  completo, pero manteniendo la interfaz GC↔VM** (alloc, layout de cabecera
  de objeto, enumeración de roots) para que la VM cambie lo mínimo — misma
  filosofía que el AOT y el HAL: interfaz estable, implementación
  intercambiable.

  **✅ DECISIÓN DE H3 — TOMADA Y APLICADA (2026-05-31).** Tras construir 2
  herramientas (índice de fragmentación + mapa ASCII, solo VM-Java) y
  caracterizar el baseline con workloads de estrés (ver `h3bench/NOTAS.md`), la
  evidencia decidió:
  - **Modelo: NO-MOVING** — free-list first-fit + split + COALESCING de huecos
    adyacentes en cada sweep. La fragmentación patológica (frag 0.96) solo
    aparece con muchos objetos longevos DISPERSOS; el patrón realista
    (working-set + churn + retención acotada) NO la produce. Y el no-moving
    evita el PINNING que el moving exigiría para las funciones native.
  - **+ 2 mejoras model-agnostic** (valen igual si luego se añade compactación):
    (a) disparo de GC por UMBRAL de crecimiento de bump (no solo al llenarse →
    mata el over-commit: -59% de pico en el workload realista); (b) heap_next
    RETREAT (el run libre final devuelve memoria al bump).
  - **Aplicado en AMBAS VMs.** La VM-Java ya tenía free-list+coalescing → solo A.
    La VM-C era bump-SIN-reuse (el "acantilado" → OOM en churn largo, JUSTO en el
    MCU): se le construyó el free-list+coalescing+retreat+umbral completo.
    Validado dual-VM (reuso: 4.4 MB en heap de 254 KB sin OOM; supervivientes
    intactos; paridad con Java; sin regresión).
  - **Compactación (moving): DIFERIDA** — solo si un workload REAL exhibe la
    patología dispersa. Coste documentado: **PINNING** — una native recorre un
    buffer crudo, así que con moving habría que poder marcar bloques
    no-desplazables (pin-bit + pin/unpin en el thunk AOT).
  - **El programador colabora (palanca complementaria, usuario 2026-05-31).** GC
    robusto NO es todo: BP ya ofrece `owner` (libera determinista vía FREE_REF +
    cascada, sin trazar el GC) y `move()`/buffers fijos (menos churn → menos
    fragmentación). Usar arrays de tamaño fijo para datos pequeños + refs `owner`
    descarga mucho al GC → anotar en el manual como "buenas prácticas de memoria".
  - **Reversible** (meta-principio): el `.mod` y la interfaz alloc/roots aíslan
    el GC del lenguaje/bytecode → si el futuro lo pide, se cambia sin recompilar
    código de usuario.

  *(El análisis moving-vs-no-moving que llevó aquí se conserva abajo como
  registro.)*

  **La elección moving vs no-moving se decide AL ABRIR H3, tras estudio +
  benchmark de fragmentación — NO se pre-decide ahora.** Las opciones y su
  coste (la interfaz estable favorece la primera, pero no la impone):
  - **No-moving** (free-list / size-class segregado, estilo TLSF/buddy): las
    direcciones NO cambian → interfaz y todos los `vm->memory + off` intactos
    → VM sin tocar. Acota la fragmentación sin mover nada, pero no la
    elimina. **Candidato líder** por simplicidad y por respetar la interfaz;
    el más barato en RAM y CPU.
  - **Moving + tabla de handles** (la analogía "FAT" del usuario, muy
    acertada): cada referencia es un índice a una tabla que guarda la
    dirección real; mover un objeto actualiza UNA entrada, no todas las refs.
    Permite **compactar** (cero huecos → máxima memoria usable, valioso en
    MCU con poca RAM). Coste: indirección en CADA acceso (CPU) + memoria de
    la tabla.
  - **Moving + fixup al compactar** (copying / mark-compact, sin tabla
    permanente): las refs son direcciones reales; al compactar se reescriben
    TODAS. Sin coste de indirección en el acceso (rápido), pero pausa de GC
    más larga y **exige enumeración PRECISA de referencias** (stack maps) que
    hoy quizá no tengamos. Copying gasta media heap (semispace) salvo
    mark-compact.

  **Restricción de PINNING (usuario, 2026-05-31) — clave si elegimos moving.**
  Una función `native` (AOT) recibe un puntero CRUDO al payload de un objeto
  (`vm->memory + ref`) y recorre los bytes directamente (es su razón de ser:
  velocidad). Si un GC compacta y MUEVE ese buffer mientras la native trabaja
  (porque la propia native aloca, u otro worker dispara GC), el puntero cacheado
  queda colgante → corrupción. Por tanto, con CUALQUIER variante moving hay que
  poder **marcar bloques como no-desplazables (pinned)** mientras una native los
  usa. Implicaciones:
  - Un **pin-bit** en el tag del objeto (hay bits libres: tag = MARK|FREE|
    type<<24|...; bits 0..23 sin usar). El compactador trata los pinned como
    anclas fijas y compacta a su alrededor (reduce algo la efectividad, pero es
    obligatorio para correción).
  - Punto natural para pin/unpin: el **boundary del thunk AOT** (el mismo de
    #186): al entrar a la native se pinnan sus args-objeto, al salir se
    despinnan. (También aplicaría a DMA/periféricos que lean un buffer del heap.)
  - El handle-table no exime: si la native cacheó el puntero resuelto, mover el
    objeto y actualizar la tabla no le sirve → pinning igualmente.
  - **Lectura para la decisión**: el pinning es maquinaria EXTRA del lado moving
    y un punto A FAVOR del no-moving (direcciones estables → punteros native
    siempre válidos → cero pinning). Suma al "coste" de la columna moving.

  **La pregunta empírica que decide H3**: ¿nuestro workload fragmenta de
  verdad bajo un buen allocator no-moving (TLSF)? Si NO → no-moving gana
  (simple, rápido, VM intacta). Si SÍ (dispositivo de larga vida, tamaños de
  objeto diversos) → la compactación se gana su coste. **Benchmark primero,
  decidir después.** (Latencia de pausa acotada — GC incremental — sigue
  siendo una capa aparte, más invasiva; solo si un lazo de control real la
  exige.)

  **→ Arranque de H3 (usuario, 2026-05-31): MEDIR antes de decidir, con 2
  herramientas sobre la VM-Java ÚNICAMENTE** (el GC es implementación de VM;
  bytecode/`.mod` idénticos → lo medido en Java transfiere conceptualmente y
  no duplicamos trabajo en C):

  1. **Índice de fragmentación.** Recorre la lista de bloques del heap
     (`heap_start..heap_next`, igual que el sweep), clasifica cada bloque
     vivo/libre y reporta:
     - `frag = 1 − (mayor_hueco_libre / total_libre)` — fragmentación externa:
       0 = un único hueco contiguo (sin fragmentar); →1 = libre hecho añicos.
     - auxiliares: total_libre, nº de huecos, mayor_hueco, bytes_vivos,
       bytes_muertos, utilización = vivos / heap_usado.
     Con el allocator ACTUAL (bump-sin-reuse) "fragmentación" = huecos muertos
     que nunca se reúsan → el índice sobre el allocator actual es la **línea
     base (peor caso) a batir**.
  2. **Mapa de memoria ASCII** (más para calibrar a ojo). Cuantiza el heap
     usado en N celdas (1 char ≈ `heap_usado/N` bytes, p.ej. 256 B/celda). Por
     celda: `.`=libre, `#`=lleno (todo vivo), `:`=semi (mezcla vivo/libre
     dentro de la celda). Una sola cadena de N chars partida cada 80 (o 60)
     columnas → mapa. El "semi" surge de la cuantización (un bloque es vivo o
     libre; una celda que cubre varios bytes puede mezclar).

  Exposición: métodos en `VirtualMachine` (`heapFragIndex()`, `heapMap(cols)`)
  llamables desde un driver de test, y/o un builtin para imprimirlos desde un
  programa BP de estrés.

  **Workloads de estrés** (los MISMOS para cada modelo candidato):
  - churn: muchos objetos de vida corta (mucha basura) → eficiencia de reuso.
  - tamaños mixtos interleaved con frees → fragmentación externa.
  - vivos-largos + cortos mezclados → huecos alrededor de supervivientes.
  - grande → free → llenar de pequeños → coalescing.
  Comparar por modelo: tiempo (pausas GC), heap pico, índice de frag final,
  forma del mapa, y si tras el churn se puede satisfacer un alloc GRANDE
  (OOM-o-no). **Caracterizar primero el allocator ACTUAL (baseline), luego los
  candidatos (free-list/TLSF y, si procede, moving), y entonces decidir.**

  **Meta-principio (usuario, 2026-05-30)**: decidir NO es grabar en piedra.
  El GC es precisamente el sitio MÁS seguro para "decidir y, si luego se
  demuestra mejor otra opción, cambiar" — porque está aislado tras el `.mod`
  estable y la interfaz alloc/roots: el código de usuario y el bytecode no se
  enteran del cambio de GC. Es la recompensa de la separación (§0: "es un
  problema de implementación, no del lenguaje").
- Cuidado con el tamaño de structs replicados: `bpvm_thread` se
  multiplica ×32 (lección de #185 — un buffer de 128 B reventó la RAM).

## 9. Multi-MCU: UNA imagen por FAMILIA, no por placa (anti-MicroPython)

**Postura de diseño (del usuario)**: NO el modelo MicroPython (una
imagen de VM por placa). Una **misma imagen sirve para todos los micros
de la misma familia** (mismo fabricante + familia), sin importar que uno
tenga más pines, más periféricos o distinta memoria. "En el fondo son
todos iguales."

**Cómo se materializa** (clave: **descubrimiento de capacidades en
runtime**, no variantes en compile-time):
- El firmware NO hardcodea nº de pines / instancias de periférico /
  tamaño de RAM. En su lugar, un **descriptor de chip en runtime** (nº
  GPIO, cuántos I2C/SPI/UART, tamaño de RAM, ¿hay PSRAM?) que la VM lee
  al arrancar y al que se adapta. Ya hay base: identificación de chip
  (módulo Pico) + `/sys/device.json` (#134). Construir sobre eso.
- Acceso a periféricos **por índice con validación en runtime** ("¿existe
  el pin X? ¿la instancia SPI Y?") en vez de tablas fijas por placa.
  Pin/periférico inexistente → RuntimeError claro, no corrupción.
- **Impacto en el CORE de la VM: ~nulo** (la VM ya es genérica). Esto
  vive en la capa de backends de HW + stdlib (Gpio/I2c/... validando
  contra límites de runtime). Encaja perfecto con "tocar la VM mínimo".
- Beneficio: una sola .uf2/.bin por familia → menos builds, menos
  matriz de firmwares, despliegue trivial. (Enlaza con H4 ESP32-S3 #147:
  diseñar el descriptor de forma que sirva para RP2350 Y ESP32-Sx.)

## 9b. HAL: formalizar la abstracción HW (registro unificado + capabilities)

**Contexto / observación (del usuario, mayo 2026)**: al portar a ESP32-S3
surgió la duda de si la VM está demasiado acoplada al hardware. **No lo
está** — ya existe abstracción: cada periférico (gpio, i2c, spi, uart,
pwm, adc, pulse, rtc, wdt) tiene en `src/` una **fachada** que llama a un
**backend de punteros a función** registrable (`bpvm_<perif>_backend_t` +
`bpvm_<perif>_set_backend`). La VM core NUNCA llama al SDK del chip;
`src/*.c` no tiene un solo `#include` de pico-sdk ni de ESP-IDF. Por eso
el core saltó a Xtensa en H4 sin tocarse: solo hay que **escribir el
backend** del micro nuevo (`esp32/gpio_mod.c`, etc.) y registrarlo al
boot — la VM y el API BP (`import Gpio`, `Pin`, …) no cambian.

**Qué mejorar en V2** (el "siguiente paso" no es crear la abstracción —
existe — sino *formalizarla*):
- Hoy son **N mini-abstracciones paralelas ad-hoc** (un struct + un
  `set_backend` por periférico). Unificar en un **HAL coherente**: un
  único mecanismo de registro/descubrimiento de backends (p.ej. una tabla
  tipo `aot_helpers_v1_t` pero para HW), versionado y additivo.
- El interfaz actual asume un **modelo "tipo-Pico"** (i2c con sda/scl/bus,
  spi con sck/mosi/miso). Acoplar con el **descriptor de capabilities de
  runtime (#9)**: el HAL expone qué periféricos/instancias/pines existen,
  y la stdlib valida contra eso. Programa BP que pide algo inexistente →
  RuntimeError claro, no asunción por placa.
- Capa de **board/chip descriptor** (mapa de pines, defaults, nº de buses)
  separada del código — datos, no `#define`s dispersos.
- **Impacto en el CORE de la VM: nulo** (igual que #9). Esto vive en la
  frontera de backends + stdlib. Es exactamente "tocar la VM lo mínimo".

**Enlaza con**: #9 (multi-MCU), #147 (ESP32-S3), H4.5 (backend GPIO
ESP32). El HAL formalizado haría que añadir el 3er/4º micro sea casi
mecánico.

## 10. PSRAM externa: heap fuera, stack/ejecución dentro

**Idea (del usuario)**: muchos micros admiten PSRAM externa de 8 MB,
barata. Reorganizar: **SRAM interna para stack + ejecución (rápida),
PSRAM externa para el Heap (grande)**.

**Por qué encaja bien con nuestra arquitectura**: la VM **ya recibe el
buffer de memoria del caller** (`bpvm_init(memory, size, ...)`), y hoy
parte ese buffer en heap (mitad baja) + stacks (mitad alta). O sea, el
"de dónde sale la memoria" ya es responsabilidad de la plataforma, no de
la VM.

**Dos escalones, de menos a más toque de VM**:
1. **Casi gratis (recomendado primero)**: en una placa con PSRAM, pasar
   a `bpvm_init` un buffer que viva ENTERO en PSRAM (8 MB) para heap +
   stacks. **Cero cambios en la VM.** Ganas la capacidad (8 MB de heap →
   la presión de heap y la urgencia del GC-reuse #8 casi desaparecen).
   Coste: los stacks en PSRAM son más lentos que en SRAM, pero la PSRAM
   va memory-mapped con caché XIP, así que los frames calientes quedan
   cacheados — probablemente aceptable. Medir.
2. **Óptimo pero con toque de VM (solo si 1 se queda corto)**: separar
   físicamente — stacks en SRAM, heap en PSRAM. Problema: SRAM y PSRAM
   están en regiones de dirección NO contiguas (p.ej. RP2350: SRAM en
   0x2000_0000, PSRAM/XIP en otra ventana). La VM hoy usa UN base +
   offset u32 contiguo. Para dos regiones físicas hay que introducir una
   traducción offset-lógico→puntero-físico (`bpvm_ptr(vm, off)`) que
   elige base según rango. Es **mecánico pero pervasivo** (toca todos los
   `vm->memory + X`) → choca con "tocar la VM mínimo". Por eso: solo si
   el escalón 1 demuestra que stacks-en-PSRAM es demasiado lento.

**Recomendación**: escalón 1 (buffer en PSRAM, cero cambios) como
default cuando haya PSRAM; escalón 2 solo guiado por profiling. Y como
8 MB de heap hace casi irrelevante el GC-reuse, esto desactiva en gran
parte la presión de #8 para placas con PSRAM.

---

# Host VM (PC) + IDE

## 11. VM de host como SIMULADOR de MCU (perfiles de memoria/flash)

**Idea (del usuario)**: la VM en el PC ha ido muy bien para depurar
rápido — la mantenemos y ampliamos. Que con un **fichero de config (o
por parámetros)** se puedan fijar distintas configuraciones de memoria,
flash, etc. para **simular arquitecturas de micros reales** en el PC.

**Por qué es ALTO valor** (lección directa de esta sesión): el OOM de
#185 (el `out_buf` que infló `bpvm_t` y reventó la RAM) **solo se
manifestó en la Pico** — en el host, con RAM de sobra, pasó
desapercibido y nos costó una odisea de hardware. Si el host tuviera un
**perfil "RP2350"** (buffer 128 KB, heap ~64 KB), ese OOM habría saltado
en el PC en segundos. Convierte el host en un **pre-flight check**: cazas
los bugs de memoria ANTES de flashear.

**Cómo** (bajo coste — la VM ya recibe el buffer del caller y ya hay
`--mem=N` + BpVM.cfg):
- **Perfiles de placa** en config: tamaño de RAM, split heap/stack,
  tamaño de flash (región FS), ¿PSRAM?, y nº de pines / instancias de
  periférico. Cada perfil imita un chip real.
- Validación de periféricos en host igual que en chip: `Gpio.Pin(99)`
  debe fallar en el PC si el perfil tiene menos pines → cazas errores de
  HW sin la placa.
- **Sinergia con #9**: el perfil simulado = el **mismo descriptor de
  chip** que el firmware lee en runtime. Un solo esquema, dos usos
  (simular en host / descubrir en placa). Diseñarlos juntos.

## 12. IDE: usabilidad y aspecto (hoy funciona pero está "hecho un desastre")

**Idea (del usuario)**: el IDE funciona pero hay que hacerlo más
amigable. Recordar ficheros/proyectos cargados, mejorar el aspecto, y
—muy útil— poder **plegar funciones y clases** (code folding como VS
Code) para estudiar el código que interesa e ignorar el resto.

**Estado actual**: editor = `JTextPane` con resaltado propio
(`BpSyntaxHighlighter`); LAF por defecto (Nimbus); ya recuerda la última
carpeta del file chooser (#119) pero no una lista de recientes.

**Sub-puntos, con recomendación técnica**:
- **Recientes (ficheros + proyectos)**: lista MRU persistida en la
  config del IDE. Extensión natural del #119 (que ya guarda la última
  carpeta). Coste bajo.
- **Code folding (plegar funciones/clases)** — el punto pivotal:
  `JTextPane` NO soporta folding nativo; hacerlo a mano es mucho trabajo
  y frágil. **Recomendación: migrar el editor a `RSyntaxTextArea`
  (RSTA)**, el editor de código estándar de Swing. De un golpe trae
  **folding + resaltado + números de línea + bracket matching +
  find/replace**. El `BpSyntaxHighlighter` actual se reemplaza por un
  `TokenMaker` de BP para RSTA. Esfuerzo medio (swap de componente +
  token maker), pero es **la jugada de mayor leverage** del IDE: una
  dependencia y caen folding y modernización del editor juntos.
- **Aspecto / look & feel**: adoptar **FlatLaf** (LAF flat moderno con
  temas claro/oscuro). Una dependencia + una línea para instalarlo →
  modernización instantánea. Combina bien con RSTA.
- Otros candidatos baratos cuando se toque: layout más limpio, iconos,
  recordar tamaño/posición de ventana y disposición de paneles.

**Coste global**: medio. Alto retorno en comodidad diaria. Ortogonal a
VM/lenguaje. RSTA + FlatLaf son las dos decisiones que más cambian la
experiencia con menos código propio.

---

# Librerías estándar + debugger

**Estado actual**: buena colección ya — HW (Gpio/Pwm/Pulse/Adc/Rtc/Wdt/
Timer/Pico) + buses (I2c/Spi/Uart, todos refactorizados a clase OO) +
drivers de dispositivo (PCA9554, MCP9804, AD7177...) + Math/IO. Política
"todo HW nuevo es clase OO" (#123) ya en vigor.

## 13. Buses que faltan

- **TCP/IP** — ya es la tarea **#145** (P-pico-wifi-tcp, [v2]). Doble
  cara: (a) transporte para el wire IDE↔dispositivo por WiFi, y (b)
  módulo stdlib para programas de usuario (clase `Net.Socket`/`TcpConn`
  sobre lwIP + cyw43). Diseñar la API BP de sockets junto con el
  transporte para no duplicar.
  - **Decisión 2026-05-31 (sin prisa)**: empezar por **cliente**, que es
    más sencillo: `Net.TcpConn` con `connect / send / recv / close`.
    **Servidor** (`listen / accept`) para más adelante. Trabaja sobre
    `byte[]`/string (sinergia con H2.4). **Backend nativo por familia**
    (host = sockets BSD; Pico 2 W = lwIP+cyw43; ESP32 = lwIP) enchufado vía
    HAL (#9b) — **NO toca la VM**, es librería con backend nativo como los
    buses. Caveat: requiere HW con red (Pico 2 **W** o ESP32; la Pico
    normal no tiene).
  - **Planificación (usuario, 2026-05-31)**: más adelante, **sin turno
    asignado** — posterior a H4 (H4 = lo ya hablado).
- **CAN bus** — importante en automoción (el usuario no lo usa, pero lo
  quiere disponible). Clase `Can.Bus` (política HW-class). **Backend
  dependiente del chip**: ESP32 tiene CAN nativo (TWAI); el RP2350 no →
  vía MCP2515 externo por SPI, o PIO-CAN. Diseñar la interfaz BP una vez
  y enchufar backend por familia (encaja con #9 multi-MCU). Prioridad
  media (nice-to-have hasta que haya un usuario que lo pida).
- Posibles más adelante: 1-Wire, USB-host, etc. — solo si se echan en
  falta (principio de minimalismo).

## 14. Pulir las existentes

- **JSON** — ya hay parser interno (json_min.c en firmware, Json.java en
  miVM) pero falta un **módulo BP de usuario** robusto (parse +
  serialize) usable desde programas. Muy demandado (configs, APIs).
- Repaso de consistencia de toda la stdlib: manejo de errores uniforme
  (RuntimeError claros), nombres/firmas coherentes, y docs (manual.html)
  al día con cada módulo.

## 14b. String — funciones nuevas (catálogo + decisiones, 2026-05-31)

**Origen**: faltan conversiones para los **tipos nuevos** de H1 (`long`,
`double`), una función **Hex**, un **`indexOf` desde el final**, un
**comparador** (lo pide el `Map`), y **justificación** de campo. Sesión de
diseño 2026-05-31 (modo "pensar, no programar").

**Catálogo final**:

| función | firma | implementación | nota |
|---|---|---|---|
| `longToString` | `(x: long): string` | builtin (como `intToString`) | conversión base-10 |
| `doubleToString` | `(x: double): string` | **BP puro** | default: punto fijo, **sin sci**, sin ceros de cola |
| `formatDouble` | `(x: double, dec: int): string` | **BP puro** | decimales fijos (Pascal `:d`); `float` ensancha → cubre float |
| `parseLong` | `(s): {err, valor}` | builtin | **dep. tuplas** (§2) |
| `parseDouble` | `(s): {err, valor}` | builtin | **dep. tuplas** (§2) |
| `parseHex` | `(s): {err, valor}` | builtin/BP | **dep. tuplas** (§2) |
| `toHex` | `(x: long): string` | builtin/BP | minúsculas, **sin** prefijo `0x` |
| `lastIndexOf` | `(s, sub): int` | builtin/BP | `indexOf` desde el final; sin `fromIndex` |
| `fromCharCode` | `(cp: int): string` | builtin | codepoint→string; fuera de Unicode → `RuntimeError` |
| `compare` | `(a, b): int` | builtin/BP | lexicográfico −1/0/1; **comparador por defecto del Map** |
| `padLeft` / `padRight` | `(s, ancho): string` | **BP puro** | justifica con espacios; genérico (cualquier string) |

**Decisiones tomadas**:
1. **`parse*` NO lanzan excepción** — devuelven **tupla `{ err, valor }`**
   (Go-style); `err: boolean` (true = falló), `valor` válido solo si `err`
   es false. Para input que falla por diseño, `try/catch` es un antipatrón.
   → **bloquea con tuplas (§2)**; camino (a) tuplas primero. Los
   `parseInt`/`parseFloat` ACTUALES (valor pelado + throw) se **unifican a
   tupla al migrar** (rompe poco código, y es nuestro).
2. **`doubleToString`/`formatDouble` = formateador PROPIO** (no el nativo de
   cada VM) → **sin notación científica** (preferencia del usuario).
   **Implementado en BP puro** = mismo bytecode en ambas VMs → **paridad
   Java/C gratis** y cero trabajo de VM. Modelo Turbo Pascal `x:totalSpace:
   decimales` **descompuesto en dos piezas**: `formatDouble(x, dec)` (el
   `:dec`) + `padLeft/padRight(s, ancho)` (el `:totalSpace`, genérico y
   reutilizable para alinear tablas de cualquier tipo). Vigilar magnitudes
   enormes (desbordan `long` al escalar `x*10^dec`) — acotar.
3. **`toHex`**: minúsculas, sin `0x`, sobre `long`. **`fromCharCode`** fuera
   de rango → `RuntimeError` (ahí el fallo es del **programador**, no input
   de usuario; distinto de `parse*`). **`compare`** lexicográfico para el
   comparador por defecto del Map.

**Hechos del lenguaje que condicionan el diseño** (verificados):
- **Aridad fija**: ni builtins ni funciones de usuario tienen sobrecarga ni
  params opcionales → por eso `doubleToString(x)` y `formatDouble(x,dec)` son
  **nombres distintos** (no se puede 1 nombre con 2 aridades).
- `byteToString` **no hace falta**: un `byte` se carga como `int`; un "char"
  sacado de un string **es un int** (codepoint). (Decisión del usuario.)

**Preferencia de implementación**: **BP puro siempre que se pueda** (paridad
gratis + cero VM, encaja con "descansar de la VM"); builtin nativo solo donde
haga falta una primitiva (p.ej. `long`→string base-10, parse numérico).

**Secuencia**: independientes de tuplas y listas para programar ya:
`longToString`, `doubleToString`, `formatDouble`, `padLeft/Right`, `toHex`,
`lastIndexOf`, `fromCharCode`, `compare`. Bloqueados por tuplas (§2):
`parseLong`, `parseDouble`, `parseHex` + unificación de `parseInt/parseFloat`.

## 14c. `Map` / Diccionario (diseño cerrado, 2026-05-31)

**Qué**: una estructura clave→valor para la stdlib. Hoy hay `List`/`SyncList`
pero no hay Map.

**Decisiones del usuario**:
- **UNA sola clase `Map`** — sin variantes (`IntMap`/`StringMap`…): "no me
  gusta tener muchos maps, para el usuario es confuso". El comparador‑objeto
  (abajo) hace innecesarias las variantes.
- **Se usa menos que las listas** → no es hot path; el coste del lock es
  asumible.
- **Thread‑safe POR DENTRO** (`synchronized`) por defecto: "no todo el mundo
  entiende programar con hilos y los efectos perversos sobre estructuras de
  datos". "El Map es seguro" es un concepto simple de explicar.
- **Interno = lista ORDENADA por key + búsqueda binaria O(log n)** — NO tabla
  hash (decisión explícita del usuario).
- **keys/values `any`**.

**Comparador a medida = OBJETO, no función** (porque BP no tiene funciones de
primera clase, §8): clase con método `compare(a, b): integer` (−1/0/1),
invocado por `INVOKE_VIRTUAL` (maquinaria existente) → **cero coste de VM**.
- `Map()` → comparador por defecto = **comparador de string** (`compare` de
  §14b) → el caso común (keys string) funciona sin más.
- `Map(miCmp)` → orden a medida (int, etc.).
- Encaja con interfaces futuras (#5): `interface Comparator`. El día que haya
  funciones‑valor (§8 fase A) se podrá pasar también una función.

**API**: `put(k,v)`, `get(k)` → `null` si ausente, `containsKey(k)`,
`remove(k)`, `size()`, `keys()`, `values()`.

**Implementación**: **BP puro** sobre `List` + bloques `synchronized(this)`.
Cero VM. **Dependencia**: `compare` de string (§14b) para el comparador
por defecto. No depende de tuplas.

## 14d. `Stats` — módulo de estadística (diseño cerrado, 2026-05-31)

**Qué**: funciones estadísticas. **No tiene que ser estándar** — módulo
**adicional** (decisión del usuario).

**Dos APIs complementarias**:
- **`Stats.Accumulator`** (streaming): `add(x: double)` incremental con el
  **algoritmo de Welford** (varianza numéricamente estable, **memoria
  acotada** — no guarda la muestra). Expone `count`, `mean`, `variance`,
  `stdDev`, `min`, `max`. Ideal para MCU / flujos largos.
- **Funciones sobre `double[]`** (muestra en memoria): `sum`, `mean`, `min`,
  `max`, `range`, `variance`, `stdDev`, `median`, `percentile`, +
  `covariance`, `correlation`, regresión lineal `linfit`.

**Tipo de elemento = `double[]`** (confirmado). `add()` y las funciones
aceptan todos los numéricos por ensanchado.

**`linfit`** devuelve `(slope, intercept)` → mediante la clase **`Stats.LinFit`**
(slope/intercept) **hasta que existan las tuplas** (§2), entonces migra a
tupla. (Es uno de los 2 consumidores que disparan las tuplas.)

**Implementación**: **BP puro**, cero VM. No depende de tuplas (usa `LinFit`
mientras tanto).

## 14e. `Compress` / `Archive` — compresión + multi-archivo (diseño, 2026-05-31)

**Qué**: comprimir/descomprimir datos y empaquetar **varios archivos** en uno.
**No tiene que ser `.Zip`** (decisión del usuario) → libres de DEFLATE/zlib.

**Criterios del usuario**: **sencillo**, rendimiento **no crítico**, **poca
memoria**, y **multi‑archivo** ("mucho mejor"). **NO toca la VM** (librería).

**Dos capas**:
- **Códec (compresor de bytes)** = **LZSS de ventana pequeña estilo
  heatshrink**: RAM acotada (= ventana, 1–4 KB), streaming, sin `malloc`,
  formato bien definido, MIT. Cumple "poca memoria" + "rendimiento no crítico"
  y comprime de verdad. (RLE descartado como principal: apenas comprime datos
  generales.) **Fallback "stored"**: si un fichero no comprime, se guarda crudo
  → nunca mayor que el original.
- **Contenedor (multi‑archivo)** = formato propio **tipo‑zip simple**: cada
  fichero se comprime **independientemente** + un **índice** al final (nombre,
  offset, tamaño original, tamaño comprimido, flag stored/compressed). Clave:
  permite **extraer UN fichero sin descomprimir el resto** → RAM acotada en el
  MCU (justo el requisito). No es `.Zip`, pero la estructura es esa.

**API tentativa**:
- Códec suelto (blob en memoria): `Compress.compress(byte[]) / decompress(byte[])`.
- Archivo: `Archive.create(path)` → `addFile(name, data: byte[])` → `close()`;
  `Archive.open(path)` → `list(): string[]`, `read(name): byte[]`,
  `extract(name, outPath)`.

**Paridad host↔MCU** (confirmado): **mismo formato, dos implementaciones**
(Java + C), como con JSON — comprimir en uno, descomprimir en el otro.

**Implementación**: backend **nativo** (códec en C + Java, mismo formato);
opera sobre `byte[]` (sinergia H2.4). El contenedor puede ser **BP puro**
sobre el códec. No depende de tuplas ni de nada nuevo del lenguaje.

**Planificación (usuario, 2026-05-31)**: más adelante, **sin turno asignado**
— posterior a H4 (H4 = lo ya hablado).

## 15. Funciones native (AOT) en stdlib donde se preste

Donde una función de stdlib sea hot y AOT-able, marcarla `function
native` para el speedup (×50-90). Candidatos naturales: Math, parsers
(JSON/int/float), y ops de string intensivas.

**Dependencia directa con #173 (AOT strings, en curso en v1)**: en
cuanto el AOT soporte strings, los parsers y formatters de stdlib (JSON,
split, format) son los mayores beneficiados — son string-heavy. O sea:
terminar #173 desbloquea native en la parte de stdlib que más lo
aprovecha. Anotado para encadenar.

## 16. Debugger — FUNDAMENTAL

El usuario lo subraya: el debugger es core, no accesorio.

**Estado**: en el host (VM-Java) funciona bien — DebugServer + VmClient,
breakpoints/step/inspect/stack/watch sobre el wire (toda la serie A1).
En Pico está **diferido**: el hook en el inner loop está cableado (#139)
pero el back-end real de debug-on-Pico es **#140** ([v2]).

**Para v2**:
- Tratar **#140 (debug-on-Pico real)** como prioridad alta, no como
  nice-to-have — es lo que cierra el ciclo "programar y depurar en el
  dispositivo real". Requiere también #138 (multiplexar el CDC para no
  mezclar output del programa con eventos de debug).
- Mejoras de UX del debugger en el IDE (sirven a host y Pico):
  breakpoints condicionales, watch expressions, hover-inspect, y la
  visualización de la pila/objetos más clara.
- Sinergia con #11 (host simulador): depurar en el host CON el perfil de
  la placa = la forma más rápida de cazar bugs antes de ir al hardware.

---

# Overlays / carga dinámica de módulos

## 17. `loadModule` / `unloadModule` — overlays estilo MS-DOS

**Idea (del usuario)**: como los overlays del MS-DOS (640 K): una parte
del programa es **residente** y el resto se carga desde disco según se
necesita, permitiendo apps más grandes que la memoria, al coste del
tiempo de carga. En BP los **módulos ya parten la app en piezas**. En
vez de `import` de todo al arranque, importar solo lo necesario y cargar
el resto en runtime con dos comandos nuevos: **`loadModule`** y
**`unloadModule`**.

**Por qué encaja**: el sistema de módulos + el FS en flash ya están. Un
módulo es una unidad de código auto-contenida con su tabla de imports.
Cargar/descargar módulos en caliente es la evolución natural.

**Análisis de coste — DOS mitades muy distintas**:

- **`loadModule(name)` — relativamente fácil**: es hacer en runtime lo
  que el loader del boot ya hace (loader.c + link.c): leer el `.mod` del
  FS, colocarlo en la región de código, resolver sus imports contra los
  módulos ya cargados (y los imports pendientes hacia él), registrarlo en
  `vm->modules[]`. La maquinaria existe; falta hacerla incremental.

- **`unloadModule(name)` — la parte peliaguda** (dos problemas):
  1. **Recuperar la memoria**: el código del módulo ocupa una región del
     `memory[]` (zona CS), hoy bump-allocada contigua. Descargar uno del
     medio deja un hueco → o un asignador de la región de código (free
     list / compactación), o un esquema de **slot de overlay fijo**.
  2. **Seguridad de referencias**: si algo vivo referencia al módulo
     descargado (función en la pila de llamadas, objeto cuya clase vive
     en él, call pendiente), descargarlo corrompe. Hay que garantizar
     que NO hay referencias vivas — o restringir el modelo (ver abajo),
     o contar referencias y **rechazar el unload si está en uso**.

**Dos modelos de diseño**:
- **(A) Slot de overlay fijo (fiel a MS-DOS, RECOMENDADO para empezar)**:
  un conjunto **residente** (módulos cargados al boot, nunca descargados:
  core + el módulo con Main + lo hot) + una **región de overlay** de
  tamaño fijo donde se carga UN módulo (o pocos) on-demand. `loadModule`
  carga en el slot (evicta lo anterior); `unloadModule` libera el slot.
  **Ventaja clave**: el overlay carga siempre en una **dirección fija** →
  las referencias son estables, sin fixups de direcciones. Restricción:
  un overlay no puede llamar a otro arbitrariamente (lo evictaría). Ideal
  para "función rara que se usa de vez en cuando" (pantalla de config,
  rutina de diagnóstico, asistente). Simple y seguro.
- **(B) Carga/descarga general con asignador de región de código**: los
  módulos cargan en cualquier sitio, el unload libera con free-list o
  compactación (+ fixup de direcciones si se compacta). Mucho más
  flexible pero mucho más complejo (GC de la región de código + tracking
  de referencias + relocación). Solo si (A) se queda corto.

**Interacciones a tener presentes**:
- **Linking por direcciones**: hoy OP_CALL_EXT resuelve imports a
  direcciones absolutas al cargar. Si un módulo recarga en otra dirección,
  las referencias rompen → el slot fijo del modelo (A) lo evita; el
  modelo (B) necesitaría una tabla de indirección (thunks) re-parcheada.
- **AOT (.mdn)**: el código native cachea CS de módulos (find_module_cs)
  y baka direcciones. Descargar un módulo cuya CS quedó cacheada →
  stale. Overlays + AOT deben invalidar esas cachés en el unload (o
  prohibir AOT en módulos overlay en v1).
- **⚠️ Relación con #10 (PSRAM)**: si la placa tiene 8 MB de PSRAM, el
  problema "app más grande que la memoria" **casi desaparece** — cargas
  todo y ya. O sea: **los overlays son la respuesta para placas de poca
  memoria SIN PSRAM**; PSRAM es la respuesta para las que pueden
  llevarla. Ambas válidas, para targets distintos. Priorizar overlays
  según cuánto pesen las placas sin PSRAM en el roadmap.

**Coste**: load = bajo-medio; unload modelo (A) = medio; modelo (B) =
alto. Recomendación: empezar por (A) (slot fijo) si se aborda.

---

> **Cierre de la ronda de ideas V2** (2026-05-29). El backlog cubre:
> tipos (long/double/byte/tuplas/strings UTF-8), OO (interfaces, no-MI),
> compilador (anti-cascada), VM/memoria (mínimo, GC-reuse, PSRAM),
> overlays/carga dinámica de módulos, multi-MCU (una imagen por familia),
> host-simulador, IDE (RSTA+FlatLaf+recientes), stdlib (TCP/CAN/JSON +
> native) y debugger. Pendiente cuando arranque v2: pasada de
> priorización y convertir puntos en tareas.

---

# Bugs conocidos / a arreglar más adelante

Defectos encontrados durante el desarrollo. Los que NO se arreglan en el
momento (porque tocan VM o exceden el alcance de la tarea actual) se anotan
aquí para abordarlos en una ronda de bugfix dedicada.

## BUG-1 — `.bpi`: tipos `long`/`double` no se parseaban (✅ ARREGLADO 2026-06-01)

**Síntoma**: pasar un `int` a un parámetro `long` de una función **cross-module**
daba basura (la pila se desalineaba: 1 slot empujado donde el callee espera 2).
`toHex(255)`, `longToString(5)`, etc. → valores corruptos. Los literales `L` y
`long(...)` explícitos funcionaban.

**Causa**: `ModuleInterface.parseType` (lector del `.bpi`) tenía casos para
`integer/float/string/boolean/int8/uint8/int16/uint16/any` pero **faltaban `long`
y `double`** → un parámetro `long` importado se trataba como `UnresolvedClassRef`,
así que `coerceToTarget` (que no veía un `PrimitiveType`) no insertaba el
widening `I32_TO_I64`. Análogo al fix de tipos estrechos de H1.1; se olvidó
`long`/`double` (H1.2/H1.3) y nadie lo pisó hasta exportar el primer módulo
stdlib con parámetros `long`/`double` (Str, H4.A).

**Fix**: dos `case` en `parseType` (`long`→PrimitiveType.LONG, `double`→DOUBLE).
Solo el lector `.bpi`; NO toca VM ni `.mod`. Descubierto y arreglado en H4.A.

**Impacto retroactivo**: afectaba a CUALQUIER función cross-module con
parámetro/retorno `long`/`double`. El `double` no había saltado solo porque
los call-sites pasaban literales `d` (ya 64-bit, sin coerción).

## BUG-2 — Excepciones no se atrapan cross-module (ABIERTO)

**Síntoma**: un `throw RuntimeError(...)` lanzado **dentro de un módulo importado**
NO lo atrapa el `try/catch e: RuntimeError` del módulo que lo importa → la
excepción propaga sin atrapar y termina el programa. **Same-module SÍ atrapa**
(verificado: `caught LOCAL ok` vs cross-module no atrapado).

**Causa probable**: la identidad de clase del `RuntimeError` sintetizado **no se
unifica entre módulos** → el match de tipo del `catch` durante el unwind falla
(el `classPtr` del objeto lanzado no coincide con el `RuntimeError` que espera el
catch del importador). Familia de la identidad de clase cross-module (L2).

**Impacto (sistémico, no solo Str)**: afecta a TODO módulo importado que lance —
p.ej. `Json.parseJson(textoInvalido)` lanza RuntimeError que el usuario **no
puede atrapar** desde su programa. También a `Str.fromCharCode(cpInvalido)`.

**Por qué no se arregla ahora**: toca la maquinaria de excepciones / identidad de
clase (VM) — fuera del alcance "sin tocar VM" de H4.A. Mientras tanto, las
funciones de H4.A se diseñan para **no lanzar cross-module** (Map.get→null,
Stats→centinelas); `fromCharCode` mantiene el throw (error de programador, "falla
ruidoso" hasta que se arregle esto).

**Encontrado**: H4.A (2026-06-01).

## GAP-1 — VM-C: subconjunto de builtins de string (conocido, por diseño F2)

**Qué**: el VM-C implementa solo un subconjunto de los builtins de string del
VM-Java. SÍ tiene: `strlen`, `substring`, `charAt`, `charCodeAt`,
`__charsToString`, `intToString`, `parseInt`, `boolToString`, `newIntArray`,
`StringBuilder`. **NO** tiene: `indexOf`, `upper`, `lower`, `trim`, `startsWith`,
`endsWith`, `contains`, `replace`, `floatToString`, `parseFloat`.

**Implicación**: la stdlib **en BP puro pensada para correr en el MCU** debe
ceñirse al subconjunto del VM-C (lo hace `Str`). Si se quisieran esas funciones
en el MCU, habría que implementarlas en el VM-C (trabajo de VM). No es un bug —
es el estado conocido del "F2 subset"; anotado aquí por su impacto en H4.

## BUG-4 — métodos privados rompen el slot de vtable cross-module (workaround aplicado)

**Síntoma**: una clase de un módulo con **métodos privados** (no-`public`),
usada cross-module, **falla al despachar cualquier método** (fault / "null
receiver" / valor basura). Same-module funciona perfecto. Descubierto en H4.A.2
(clase `Map` con helpers privados `bsearch`/`insertAt`).

**Causa**: el `.bpi` exporta SOLO los métodos públicos, pero el **vtable real
incluye los privados** (ocupan slot). El importador calcula los slots de vtable
desde el `.bpi` (sin los privados) → **slots desalineados** → `m.size()`
despacha a la ranura equivocada. Por eso las clases L2 (sin privados) funcionaban
y el `Map` no.

**Workaround (aplicado en `Collections`)**: declarar los helpers como `public`
(entran en el `.bpi`, los slots cuadran). Feo (expone internals) pero funciona.

**Fix propio (pendiente, compilador)**: el `.bpi` debe **incluir los métodos
privados como reserva de slot** (marcados no-llamables) para que el importador
numere los slots igual que el dueño; o asignar slots con un orden canónico
estable. Beneficia a CUALQUIER clase stdlib con métodos privados. Encontrado y
caracterizado en H4.A.2 (2026-06-01). (Nota: el síntoma inicial de "construcción
cross-module deja campos null" era ESTE bug, no uno aparte — `Collections.Map()`
directo funciona una vez alineados los slots.)

## GAP-2 — tipos built-in (List/SyncList...) no unifican identidad cross-module

**Qué**: devolver un `List` (u otro tipo built-in sintetizado) desde la API
pública de un módulo falla en el importador: "valor de tipo 'List' no asignable
a variable de tipo 'List'" — el `List` de la firma importada no es identidad-
igual al `List` global del consumidor.

**Implicación**: la API pública de un módulo no debe **devolver/recibir** tipos
built-in (List, SyncList, StringBuilder, Mutex). El `Map` lo esquiva con
accesores por índice (`keyAt(i)/valueAt(i): Object`) en vez de `keys(): List`.

**Fix propio (pendiente, compilador)**: resolver los nombres built-in en firmas
importadas a la clase sintetizada global (unificar identidad). Encontrado en
H4.A.2 (2026-06-01).

## H5 (modelo de objetos) — progreso parcial (2026-06-01)

Habilitado lo MÍNIMO para el `Map` (Fase A, boxing manual):
- **`Object`** expresable en fuente = tipo raíz universal (`AnyType` interno).
  `resolveType` (source) + `isExportableType`/`typeToString`/`parseType` (.bpi)
  ahora aceptan `Object`/`any`. (Cambios de compilador, pequeños.)
- **`Map` (H4.A.2) HECHO** con claves string, comparador‑objeto, thread-safe
  (Mutex), búsqueda binaria, paridad dual-VM. En `bpstdlib/Collections.bp`.
- **Pendiente (siguiente paso)**: clases envoltorio `Integer`/`Long`/`Float`/
  `Double`/`Boolean` (boxing manual) + comparadores para claves no-string.
