# BasicPlus — Backlog V2 (notas de diseño)

Documento vivo. Puntos a desarrollar en v2, **sin compromiso de scope
todavía** — son ideas con su contexto técnico para que arrancar v2 no
empiece de cero. Cada punto lleva: qué, por qué, implicaciones/coste, y
dependencias con otros puntos.

> Además de lo de aquí, ya hay tareas marcadas `[v2]` en el backlog
> principal: #138 cdc-multiplex, #140 debug-on-Pico real, #145 wifi-tcp,
> #153 dual-core RP2350 (con SWD), #169 AOT cross-module.

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

## 2. Tuplas  *(si no resulta demasiado complejo)*

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
