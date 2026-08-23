# Formato binario `.mod` v5

Este documento es la **especificación canónica** del fichero `.mod` que
emite el frontend (`lexer-java`) y consume cualquier VM compatible
(`bpgenvm` Java; futura `bpgenvm-c`). Si una implementación discrepa
del documento, **el documento gana**.

Versión vigente: **5** (campo `MAGIC` = `0x4D4F4435` = ASCII `"MOD5"`).

Convención general:
- Todos los enteros son **big-endian** salvo indicación expresa.
- Todos los strings se serializan con la convención `writeUTF` de Java
  `DataOutputStream`: `u16 length` seguido de `length` bytes UTF-8
  (modificado — sin BOM, sin terminador NUL). Cuando el documento dice
  "UTF-8 raw" se refiere a sólo los bytes, sin el length prefix.
- Offsets dentro de las secciones (`relativeOffset`, `csOffset`, etc.)
  son `i32` con signo. Los offsets CS-relative al data block son
  típicamente **negativos** porque el data block precede al código en
  memoria runtime.

---

## 1. Layout del fichero

> ⚠️ **Este documento estuvo DOS versiones desatrasado** — describía v5 cuando el
> compilador llevaba tiempo emitiendo v6 — y se puso al día el 23-ago-2026 al
> añadir v7. Queda escrito porque volverá a pasar y porque el fallo es caro: **el
> header CRECE con la versión**, así que un lector que se equivoque de tamaño
> desplaza *todas* las secciones y corrompe el módulo en silencio. Leer el header
> según el MAGIC no es una precaución: es el contrato.

```
+─────────────────────── header (36 bytes en v7) ───────────────────────────+
│  +00  MAGIC          i32  0x4D4F4437 ("MOD7")                            │
│  +04  dataSize       i32  bytes de la sección data                       │
│  +08  mainOffset     i32  offset del entrypoint dentro de code (-1 = no) │
│  +12  importsSize    i32  bytes de la sección imports                    │
│  +16  exportsSize    i32  bytes de la sección exports                    │
│  +20  codeSize       i32  bytes de la sección code                       │
│  +24  librarySize    i32  bytes de la sección library (0 si no hay)      │
│  +28  interfaceSize  i32  bytes de la sección interface   ← v6 y v7      │
│  +32  nativeSize     i32  bytes de la sección native      ← sólo v7      │
+──────────────────────────────────────────────────────────────────────────+
+── library   ──+  librarySize   bytes, UTF-8 raw (sin length prefix)
+── imports   ──+  importsSize   bytes (ver §3)
+── exports   ──+  exportsSize   bytes (ver §4)
+── interface ──+  interfaceSize bytes (ver §4.5)   ← v6+
+── native    ──+  nativeSize    bytes (ver §4.6)   ← v7
+── data      ──+  dataSize      bytes (ver §5)
+── code      ──+  codeSize      bytes (ver §6)
```

**Header por versión:** v5 = 28 bytes (7 enteros) · v6 = 32 (8) · v7 = 36 (9).

📌 **Las secciones nuevas se insertan ANTES de `data`, nunca al final.** No es
estética: los offsets internos de `data` y `code` son relativos a su propia
sección, así que meter algo delante no obliga a recalcular nada — y meterlo
detrás convertiría el fichero en algo cuyo final hay que adivinar. Un `.mod` que
vive en un pack por XIP **no es un fichero**: es una región mapeada, y ahí "lo
que sobra al final" no es una señal que exista.

El fichero termina exactamente al final de la sección `code`. Sin
trailer, sin checksum, sin padding.


## 2. Library

String UTF-8 sin length prefix; `librarySize` lo determina. Si está
vacío (`librarySize == 0`), el módulo no pertenece a ninguna library.

Cuando hay library `L` y módulo `M`:
- Nombre de fichero por convención: `L.M.mod`.
- Símbolos en el `globalSymbolTable` se cualifican como `L.M.<symbol>`.

---

## 3. Sección IMPORTS

```
imports ::= count:i32  import_entry*

import_entry ::= name:UTF  fromPath:UTF
```

- `name` es el qualifiedName del símbolo importado:
  - `"<Module>.<func>"` si el dueño no declara library.
  - `"<library>.<Module>.<func>"` si declara library.
- `fromPath`: string libre que el loader usa para localizar el `.mod`
  dueño en disco. `""` = aplica la convención por defecto
  (`<library>.<Module>.mod` o `<Module>.mod`).

Una vez resuelto el dueño en el linker, el loader escribe la dirección
absoluta del símbolo en la **tabla de ext** (ver §7), entrada `i` → byte
`moduleBase + i*4`. El opcode `CALL_EXT` (ver `OPCODES.md`) lee de esa
tabla por índice.

---

## 4. Sección EXPORTS

Estructura con sub-secciones opcionales detectables por bytes
remanentes del `exportsSize` declarado en la cabecera. Esto permite
ampliar el formato manteniendo compatibilidad con loaders antiguos
(verlos sólo como `exportsSize` mayor del esperado).

### 4.1 Sub-sección de funciones exportadas (obligatoria)

```
funcs ::= count:i32  func_entry*

func_entry ::= name:UTF  relativeOffset:i32
```

`relativeOffset` es relativo al inicio del bloque `code` del módulo. El
loader lo absolutiza con `codeStart + relativeOffset` y lo registra en
`globalSymbolTable` con la clave `<library>.<module>.<name>` (o
`<module>.<name>` si no hay library).

### 4.2 Sub-sección de data exports (opcional — B3 v2 / L2 v3)

Presente sólo si quedan bytes después de leer la sub-sección 4.1.

```
data_exports ::= count:i32  data_entry*

data_entry ::= name:UTF  csOffset:i32
```

`csOffset` es CS-relative al `codeStart` del módulo (es decir, suele ser
negativo porque el data block está ANTES del code en memoria runtime).
La dirección absoluta es `codeStart + csOffset`.

Uso típico:
- `RuntimeError` (clase sintética para `try/catch e: RuntimeError`).
- Cualquier `public class X`: se exporta el descriptor de la clase
  como data symbol para que importadores con `extends` resuelvan
  `parentOff` en `linkAll` (ver §4.3).

### 4.3 Sub-sección de class fixups (opcional — L2 v3)

Presente sólo si quedan bytes después de leer 4.2. Si 4.3 está pero 4.2
no, **4.2 debe escribirse con count=0** (es un marcador necesario para
que el loader sepa dónde empieza 4.3).

```
class_fixups ::= count:i32  fixup_entry*

fixup_entry ::= childClassName:UTF  childCsOffset:i32  parentQualified:UTF
```

- `childClassName`: nombre simple de la clase del child (que vive en
  ESTE módulo).
- `childCsOffset`: offset del descriptor del child relativo al
  `codeStart` de este módulo. Negativo (data block precede).
- `parentQualified`: nombre cualificado del parent en `globalSymbolTable`,
  e.g. `"L2Lib.Counter"`.

Aplicación en `linkAll` (tras resolver imports):

```
for each fixup:
    parentAbs = globalSymbolTable[parentQualified]
    childAbs  = moduleCodeStart + childCsOffset
    parentOff = parentAbs - moduleCodeStart        // CS-relative al child
    writeI32(childAbs + 8, parentOff)              // CLS_OFF_PARENT_OFF
```

Esto deja la cadena de herencia navegable cross-module por
`isDescendantOf` e `INVOKE_VIRTUAL` (que usan
`moduleManager.getCSForDataAddr` para saber qué CS sumar).

### 4.4 Sub-sección de eh-class fixups (opcional — BUG-2)

Presente sólo si quedan bytes después de leer 4.3. Si 4.4 está pero 4.3
no, **4.3 debe escribirse con count=0** (marcador, igual que 4.2→4.3).
Parchea el operando `clsOff` (i32) de cada `TRY_BEGIN_EXT` (0xAB) usado para
un `catch` de una clase de excepción definida en OTRO módulo.

```
eh_class_fixups ::= count:i32  eh_entry*

eh_entry ::= codeOffset:i32  parentQualified:UTF
```

- `codeOffset`: offset del operando `clsOff` (i32) a parchear, relativo al
  `codeStart` de este módulo (positivo; dentro del code block).
- `parentQualified`: nombre cualificado de la clase de excepción en
  `globalSymbolTable`, e.g. `"HoleLibA.MyError"`.

Aplicación en `linkAll` (tras resolver imports y class fixups):

```
for each eh_fixup:
    parentAbs = globalSymbolTable[parentQualified]
    clsOff    = parentAbs - moduleCodeStart        // CS-relative
    writeI32(moduleCodeStart + codeOffset, clsOff) // parchea el operando
```

En runtime `TRY_BEGIN_EXT` computa `expectedClass = cs + clsOff = parentAbs`,
de modo que `isDescendantOf` casa el `catch` con la clase del otro módulo.

---

## 4.5 Sección INTERFACE (v6+)

El texto de la interfaz del módulo — lo que hasta V4 era un fichero `.bpi`
aparte. UTF-8 crudo, sin length prefix; `interfaceSize` lo delimita. Vacía
(`interfaceSize == 0`) es legítimo: los módulos emitidos a mano por los tests no
la llevan.

**Por qué se fundió** (V4/H6.a): dos ficheros que tienen que ir juntos son dos
ficheros que se pueden desparejar. Al meterla dentro desaparecieron **71 `.bpi`**
y con ellos una clase entera de fallo. Es el mismo argumento que justifica §4.6.

Las VMs la **saltan**: ejecutan sin ella. La usa el compilador, para resolver
imports sin recompilar el módulo dueño.

## 4.6 Sección NATIVE (v7)

El código nativo AOT del módulo — lo que hasta V5 era un fichero `.mdn` aparte.
Vacía (`nativeSize == 0`) mientras el módulo no lleve nativo, que es el caso de
casi todos.

### Su contenido: N blobs `.mdn` concatenados, uno por familia

```
+── blob 0 ──+  cabecera mdn_header_t + N symbols + code   (alineado a 4)
+── blob 1 ──+  idem, otra arquitectura
+──   …    ──+
```

### ⚠️ La sección se ALINEA A SÍ MISMA

Las secciones del `.mod` **no están alineadas**: se empaquetan seguidas, y hoy
`data` y `code` empiezan en offsets impares. Da igual para ellas, porque el
cargador las **copia** a `memory[]` en direcciones alineadas — la alineación del
fichero no se hereda.

**Con `native` no da igual.** Un `.mod` dentro de un pack se ejecuta por **XIP,
en su sitio**, así que la posición del blob en el fichero **es su dirección de
ejecución**. Un blob RISC-V en offset impar no arranca, y el propio `.mdn` ya
pide sus datos alineados a 4.

Por eso la sección empieza con **0–3 bytes de relleno** hasta el múltiplo de 4, y
`nativeSize` **los incluye**. Ninguna otra sección se toca; el lector hace:

```
inicio_del_primer_blob = align4(inicio_de_la_seccion)
```

📌 Lo señaló Eduardo el 23-ago al revisar el formato, **antes** de que hubiera un
solo blob emitido. Sin eso, el fallo sólo se habría visto en placa, con XIP, y en
una arquitectura sí y en otra no.

**No hay tabla de contenidos, y no hace falta**: un `.mdn` ya se describe a sí
mismo. Su cabecera (`src/mdn_format.h`) trae `code_size` y `sym_count`, así que
el tamaño total de un blob se calcula desde él:

```
total = sizeof(mdn_header_t) + sym_count * sizeof(mdn_symbol_t) + code_size
```

redondeado a 4. Con eso un lector camina de blob en blob hasta agotar
`nativeSize`. Y cada blob dice **para qué arquitectura es**, en su campo `arch`
(`MDN_ARCH_ARM`, `_RISCV`, `_XTENSA`), así que el cargador se queda con el suyo y
salta los demás.

📌 **Por eso el formato de N familias es el mismo que el de una.** Hoy se emite
N=1 —la familia de la placa conectada— y el multifamilia no pide ningún cambio de
formato: sólo que el compilador emita más blobs. La poda del IDE se vuelve
*«quédate con el blob cuyo `arch` coincide»*, que es tirar bytes, no reformatear.

### Lo que mata

Igual que §4.5, pero peor, porque el `.mdn` se puede desparejar **de dos maneras**
y las dos nos han pasado:

- **En el tiempo** — *«el `.mdn` es MÁS VIEJO que su `.mod`»*. Existe un guardián
  en el IDE porque hacía falta; con un solo fichero no hay de qué guardarse.
- **De familia** — se subió un `.mdn` de ARM a una P4. El `arch` estaba en el
  fichero y la placa decía el suyo, pero nadie los comparaba. Con bloques
  etiquetados **dentro**, elegir el equivocado deja de ser posible por
  construcción.

### Estado (23-ago-2026)

El **formato** está cerrado y los dos cargadores lo leen. La sección se emite
**vacía**: falta que el compilador meta el blob y que el cargador registre sus
thunks desde ahí en vez de buscar un `.mdn` en el FS. Ver `FICHAS.md`, hito N1
punto 4.
---

## 5. Sección DATA

`dataSize` bytes opacos al loader. Contiene:
- Constantes (`const X: integer := 42` → 4 bytes con el valor).
- Globals (`var g: T` → tamaño según `T`).
- Class descriptors (ver §8).
- String literals: `[u32 byte_len BE][bytes UTF-8]` (V2 H2). Hasta V1 era
  `[u32 cp_count][i32 codepoint]×N`; ahora son bytes UTF-8 (≈4× más compacto
  para ASCII). Es solo un cambio de **codificación del contenido** del símbolo
  (dato), no de la estructura del contenedor. El layout coincide con un heap
  string `TYPE_ARRAY_I8` (sin el tag), así que `LEA_GLOBAL` empuja la dirección
  y el string se usa in-situ.

**Alineación a 4 (v3.0.1).** Cada símbolo ocupa su tamaño **natural**, y no todos
son múltiplo de 4: `integer`/`float`/`long`/`double` y los arrays numéricos sí lo
son, pero un **string** es `4 + N` bytes (N = longitud UTF-8, arbitraria), un
`int8`/`byte` const ocupa **1** byte, un `int16`/`short` **2**, y un
`byte[]`/`int16[]` const `4 + N` / `4 + 2n`. Si se empaquetaran a pelo, un símbolo
impar dejaría a los que le siguen (y al propio CS) en direcciones **no 4-alineadas**.
Por eso el frontend **redondea el tamaño de CADA símbolo al siguiente múltiplo de
4** (`slot = (len + 3) & ~3`): el dato va al extremo **bajo** del slot (donde apunta
su offset CS-relativo) y los bytes sobrantes quedan al alto, a cero. Como todos los
slots son múltiplo de 4, **cada símbolo cae en un offset 4-alineado** y el bloque
entero —y con él `codeStart = dataStart + dataSize` (= **CS**)— queda alineado sin
padding extra al final. Importa porque el barrido conservador del GC —en **ambas**
VMs— solo lee palabras **alineadas a 4**: un símbolo de referencia (un global) en
dirección no alineada sería invisible al GC → recolección de un objeto vivo (el bug
**GC-2** de v3.0.1). Como extra, evita accesos a `integer` no alineados, que
**fallan** en ARM/RISC-V. (Los campos DENTRO de un objeto no sufren esto: van en
slots de 4 bytes indexados —ver §8— y el compilador prohíbe arrays de tamaño fijo
como campo de clase, así que un campo de referencia siempre queda 4-alineado.)

El loader inyecta el bloque entero en memoria a partir de
`dataStart = moduleBase + extTableSize`. Los offsets dentro del bloque
los conoce el frontend al emitir (no se reportan al loader).

---

## 6. Sección CODE

`codeSize` bytes de bytecode. Ver `OPCODES.md` para la semántica de
cada byte y sus operandos.

El loader inyecta el bloque a partir de `codeStart = dataStart + dataSize`.
Después del último byte, `codeStart + codeSize` es la frontera donde
empieza el siguiente módulo (o `nextFreeAddress` para el próximo
`loadModuleToMemory`).

---

## 7. Layout runtime: organización del `memory[]`

Cuando el loader carga un módulo, el `memory[]` global de la VM queda:

```
   ↓ moduleBase (= nextFreeAddress alineado)
   ┌────────────────────────────────┐
   │ ext-table  (extCount * 4)      │  ← rellenado por linkAll con addr de imports
   ├────────────────────────────────┤
   │ data block (dataSize bytes)    │  ← ↑ dataStart
   ├────────────────────────────────┤
   │ code block (codeSize bytes)    │  ← ↑ codeStart  (= CS del módulo)
   └────────────────────────────────┘
   ↓ moduleBase + total = nextFreeAddress (del siguiente módulo)
```

Convenciones clave:
- **CS** del módulo = `codeStart`. Todos los opcodes (jumps, NEW_OBJECT,
  CALL/RET) trabajan con offsets relativos a CS.
- Los offsets a símbolos del data block son **negativos** desde CS
  (`dataStart - codeStart < 0`).
- La ext-table es per-module; cada import resuelto ocupa 4 bytes con
  la dirección absoluta. `CALL_EXT idx` lee `mem[extTableAddress +
  idx*4]` para obtener la dirección destino.

Detrás de todos los módulos cargados viene el **heap** (a partir de
`vm.heapStart = nextFreeAddress` tras el último módulo). Ver
`HEAP_LAYOUT.md`.

---

## 8. Layout del Class Descriptor (en el data block)

Cada `public class X` registra un símbolo en el data block con este
layout binario. La dirección del descriptor (CS-relative o absoluta
según el contexto) es lo que `NEW_OBJECT csOff` empuja al stack como
el `class_ptr` de la nueva instancia.

```
offset  size  campo
─────────────────────────────────────────────────────────────────────
  +0    u16   num_fields
  +2    u16   num_methods
  +4    u16   bitmap_words   = ceil(num_fields / 32)
  +6    u16   _pad           (= 0)
  +8    i32   parent_offset  CS-relative al descriptor del padre.
                              0 = sin padre.
                              Cross-module: se escribe 0 al emitir y
                              `linkAll` lo PARCHA usando class_fixups (§4.3).
 +12    bw*4  field_bitmap   bit k=1 ⇒ field[k] es ref (GC trace)
 +12+bw*4  bw*4  owner_bitmap  bit k=1 ⇒ field[k] es owner (FREE recursivo)
 +12+2*bw*4  num_methods*4  vtable  i32 offsets relativos a CS del módulo
                                     dueño. Sentinel -1 = método heredado
                                     cross-module no implementado localmente
                                     (la VM sube por parent_offset).
```

Notas:
- `bw = bitmap_words`. Si `num_fields == 0`, `bw == 0` y las dos
  bitmaps ocupan 0 bytes.
- `parent_offset` es CS-relative al CS del MÓDULO del descriptor que se
  está leyendo. Para navegar a un parent cross-module la VM hace
  `parentAbs = getCSForDataAddr(child) + parent_offset` (resolved via
  `loadedModules`).
- La vtable nunca incluye constructores; el `__init` de cada clase es
  una función normal accesible por nombre.

---

## 9. Header de objeto en heap

Aunque vive en el heap (no en el `.mod`), se documenta aquí por su
relación con `NEW_OBJECT`. Cuando `NEW_OBJECT csOff` ejecuta:

```
ref := heapAlloc(num_fields * 4, TYPE_OBJECT)
writeI32(ref, classPtr)         ; primer field = class_ptr
zero los siguientes (num_fields-1)*4 bytes
push ref
```

El `class_ptr` ocupa el slot `[0]` de la instancia; los fields del
usuario empiezan en el slot `[1]` (= ref+4). `GET_FIELD/SET_FIELD slot`
acceden a `mem[ref + 4 + slot*4]`.

El header del bloque heap (4 bytes ANTES del `ref` que ve el código BP)
guarda type+size — ver `HEAP_LAYOUT.md` §2 para detalle.

---

## 10. Detección de versión

El loader debe:
1. Leer 4 bytes y mirar el MAGIC. **No hay byte de versión separado**: el MAGIC
   ES la versión, y también la declaración de ABI (ver `#284`).
2. **El tamaño del header depende del MAGIC** — 28 / 32 / 36 bytes para v5 / v6 /
   v7. Leerlo mal desplaza todas las secciones. En Java está resuelto en
   `ModFormat.headerSizeDe(magic)`.
3. **Qué se ejecuta hoy: v6 y v7.** v5 se **rechaza** — es anterior al ensanchado
   de refs 4→8B y su ABI no se puede garantizar, así que correrlo corrompería en
   silencio.
4. 📌 **v7 NO obliga a regenerar nada.** Sólo añade una sección; el ABI (el ancho
   de referencia) no cambia, y por eso v6 sigue siendo ejecutable. El gate de
   `#284` vigila el ABI, no el número.

Para añadir un campo sin romper compat se usa el truco de subsección
opcional detectable por bytes remanentes (ver §4.2 / §4.3). Para
cambios incompatibles se sube el número.

---

## 11. Tabla de tamaños fijos

| Símbolo | Valor | Significado |
|---|---|---|
| `MAGIC` | `0x4D4F4435` | "MOD5" — rechazado (ABI ambiguo) |
| `MAGIC_V6` | `0x4D4F4436` | "MOD6" — con sección `interface` |
| `MAGIC_V7` | `0x4D4F4437` | "MOD7" — con sección `native` |
| `HEADER_SIZE` / `_V6` / `_V7` | 28 / 32 / 36 | bytes del header, según MAGIC |
| `FORMAT_VERSION` | 7 | versión lógica (informativa) |
| `CLS_OFF_NUM_FIELDS` | 0 | offset del campo en el class descriptor |
| `CLS_OFF_NUM_METHODS` | 2 | |
| `CLS_OFF_BITMAP_WORDS` | 4 | |
| `CLS_OFF_PARENT_OFF` | 8 | |
| `CLS_OFF_FIELD_BITMAP` | 12 | (los bitmaps empiezan aquí) |
| `EXT_TABLE_ENTRY_SIZE` | 4 | bytes por entrada de la ext-table |

---

## 12. Cambios entre versiones

- **v7** (actual, 23-ago-2026): sección **`native`** entre `interface` y `data`
  (§4.6) — el `.mdn` deja de ser un fichero aparte. Header de 36 bytes.
  **Aditivo**: no cambia el ABI, así que los `.mod` v6 se siguen ejecutando y no
  hubo que regenerar ninguno.
- **v6** (V4/H6.a): sección **`interface`** entre `exports` y `data` (§4.5) — el
  `.bpi` deja de ser un fichero aparte; se borraron 71. Header de 32 bytes. Y
  marca la era de refs de 8 bytes, que es lo que hace ejecutable un módulo (#284).
- **v5**: cada import lleva `fromPath` además de `name`.
- **v4**: se añade `librarySize` al header + sección `library`.
- **v3**: layout actual del class descriptor (con `bitmap_words` y
  `parent_offset` en sus posiciones actuales).
- **v2**: introducción de `bitmap_words`.
- **v1**: formato inicial mínimo.

Los .mod v4 e inferiores ya no se cargan (el MAGIC cambió). El frontend
emite siempre la versión actual.
