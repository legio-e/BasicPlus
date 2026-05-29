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
