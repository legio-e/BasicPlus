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

```
+──────────────────────────── header (28 bytes) ───────────────────────────+
│  +00  MAGIC          i32  0x4D4F4435 ("MOD5")                            │
│  +04  dataSize       i32  bytes de la sección data                       │
│  +08  mainOffset     i32  offset del entrypoint dentro de code (-1 = no) │
│  +12  importsSize    i32  bytes de la sección imports                    │
│  +16  exportsSize    i32  bytes de la sección exports                    │
│  +20  codeSize       i32  bytes de la sección code                       │
│  +24  librarySize    i32  bytes de la sección library (0 si no hay)      │
+──────────────────────────────────────────────────────────────────────────+
+── library  ──+  librarySize bytes, UTF-8 raw (sin length prefix)
+── imports  ──+  importsSize bytes (ver §3)
+── exports  ──+  exportsSize bytes (ver §4)
+── data     ──+  dataSize    bytes (ver §5)
+── code     ──+  codeSize    bytes (ver §6)
```

El fichero termina exactamente al final de la sección `code`. Sin
trailer, sin checksum, sin padding.

---

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
1. Leer 4 bytes y comprobar `MAGIC == 0x4D4F4435`.
2. Si no coincide, abortar con error "firma mágica inválida".
3. Versiones futuras del `.mod` (v6+) cambiarán el MAGIC. Es la única
   forma fiable de detección — no hay byte de versión separado.

Para añadir un campo sin romper compat se usa el truco de subsección
opcional detectable por bytes remanentes (ver §4.2 / §4.3). Para
cambios incompatibles se sube el número.

---

## 11. Tabla de tamaños fijos

| Símbolo | Valor | Significado |
|---|---|---|
| `MAGIC` | `0x4D4F4435` | "MOD5" en ASCII big-endian |
| `HEADER_SIZE` | 28 | bytes del header binario |
| `FORMAT_VERSION` | 5 | versión lógica (informativa) |
| `CLS_OFF_NUM_FIELDS` | 0 | offset del campo en el class descriptor |
| `CLS_OFF_NUM_METHODS` | 2 | |
| `CLS_OFF_BITMAP_WORDS` | 4 | |
| `CLS_OFF_PARENT_OFF` | 8 | |
| `CLS_OFF_FIELD_BITMAP` | 12 | (los bitmaps empiezan aquí) |
| `EXT_TABLE_ENTRY_SIZE` | 4 | bytes por entrada de la ext-table |

---

## 12. Cambios entre versiones

- **v5** (actual): cada import lleva `fromPath` además de `name`.
- **v4**: se añade `librarySize` al header + sección `library`.
- **v3**: layout actual del class descriptor (con `bitmap_words` y
  `parent_offset` en sus posiciones actuales).
- **v2**: introducción de `bitmap_words`.
- **v1**: formato inicial mínimo.

Los .mod v4 e inferiores ya no se cargan (el MAGIC cambió). El frontend
emite siempre la versión actual.
