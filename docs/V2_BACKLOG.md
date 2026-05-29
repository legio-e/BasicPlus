# BasicPlus — Backlog V2 (notas de diseño)

Documento vivo. Puntos a desarrollar en v2, **sin compromiso de scope
todavía** — son ideas con su contexto técnico para que arrancar v2 no
empiece de cero. Cada punto lleva: qué, por qué, implicaciones/coste, y
dependencias con otros puntos.

> Además de lo de aquí, ya hay tareas marcadas `[v2]` en el backlog
> principal: #138 cdc-multiplex, #140 debug-on-Pico real, #145 wifi-tcp,
> #153 dual-core RP2350 (con SWD), #169 AOT cross-module.

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
