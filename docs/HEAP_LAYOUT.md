# Layout runtime del `memory[]`

Documento canónico de cómo se organiza el array `memory[]` (o el buffer
equivalente que el caller provee a una implementación C) durante la
ejecución de un programa BP. Cubre data blocks, heap, stacks, headers
de objeto y los registros del intérprete.

Si una implementación discrepa, **el documento gana**.

---

## 1. Organización general de `memory[]`

```
0x0000  ┌──────────────────────────────┐
        │ sentinela THREAD_EXIT (0x70) │  ← memory[0] = 0x70
        │                              │
0x0100  ┝━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┥  ← nextFreeAddress inicial
        │                              │
        │  ──── módulo 1 ──────        │
        │   ext-table   (extCount*4)   │  ← moduleBase
        │   data block  (dataSize)     │  ← dataStart
        │   code block  (codeSize)     │  ← codeStart = CS del módulo
        │                              │
        │  ──── módulo 2 ──────        │
        │   ext-table                  │
        │   data block                 │
        │   code block                 │
        │                              │
        │   ... más módulos ...        │
        │                              │
heapStart  ━━━━━━━━━━━━━━━━━━━━━━━━━━━ │  ← tras último módulo
        │                              │
        │  HEAP                        │
        │   (objetos, arrays, strings) │  ← crece hacia arriba con heapNext
        │   ┌─ free list intercalada   │
        │                              │
STACK_BASE ━━━━━━━━━━━━━━━━━━━━━━━━━━━ │  ← dirección configurable (BpVM.cfg)
        │                              │
        │  STACKS                      │
        │   thread 0 (main): 16 KiB    │  ← MAIN_STACK_BYTES
        │   thread 1 (worker): 2 KiB   │  ← DEFAULT_THREAD_STACK_BYTES
        │   thread 2: 2 KiB            │
        │   ... freedStackRegions      │
        │                              │
memorySize└──────────────────────────────┘  ← memory.length
```

- **memory[0] = 0x70**: byte sentinela. Cuando un worker BP (no-main)
  hace RET de su `run()`, el saved-PC del frame original apunta a
  `cs=0, pc=0`, y la siguiente instrucción fetcheada es `mem[0] = 0x70`
  = `THREAD_EXIT`, que termina el thread sin tumbar la VM. Para el
  thread main, el equivalente es `0x00 = HALT` que sí termina la VM.

- **memory[1] y memory[2]**: los centinelas de vuelta del puente native→BP y de un handler de evento (VM-C).

- **memory[0x80..0x84)** (VM-C, V7 `#592`): la MARCA del 2º temporizador —un contador de 32 bits en el orden de la
  CPU, no un valor de BP—, que el reloj de la plataforma sube cada ~5 ms y el KILL también; las natives la miran en
  sus bucles que llaman a algo. Lejos del 0 a propósito (un campo de un objeto null se lee en los primeros bytes).
  Contrato con los `.mdn`: no se mueve.

- **`nextFreeAddress` arranca en `0x0100`** (256 bytes reservados al
  inicio; pueden usarse para constantes especiales en el futuro). Cada
  `loadModuleToMemory` aloca su bloque a partir de ahí, redondeando
  al final.

- **`heapStart`**: marca el comienzo del heap. Lo fija `vm.setHeapStart()`
  tras cargar el último módulo (la VM Java lo hace en
  `executeRootModule` justo antes de `vm.run()`).

- **`STACK_BASE`**: configurable por `BpVM.cfg.stackBase` (default
  `memorySize / 2`). Divide heap de stacks.

- **En la VM-C** el reparto es otro (desde `#451`, 11-sep): entre el techo del heap y las pilas vive
  la **tabla de handles** (`gen[]` y `addr[]`; baja desde las pilas y sólo sobre heap libre), y la
  región de pilas la decide `bpvm_stack_region_bytes` (un 25 % del bloque de la VM, de 64 a 512 KB, o
  `stack=N` del ENV), no `BpVM.cfg`. En miVM la tabla son arrays de Java, fuera de `memory[]`. Lo que
  ve el programa es lo mismo en las dos (§4: el tope de cada pila). El dibujo de arriba es el de miVM; el
  tramo de la VM-C, del heap para abajo:

  ```
  heap_start ━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ← tras el último módulo
          │  HEAP (bump: heap_next ↑)     │
  heap_top   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ← baja cuando la tabla crece, nunca sobre heap vivo
          │  TABLA DE HANDLES             │
          │   gen[cap]  │  addr[cap]      │  ← crece HACIA ABAJO, con el mundo parado (#610)
  stack_base ━━━━━━━━━━━━━━━━━━━━━━━━━━━━
          │  PILAS (main e hilos)         │  ← bpvm_stack_region_bytes / stack=N; cada una crece hacia arriba
  ```

- **La tabla de la VM-C crece MOVIÉNDOSE** (`bpvm_handles_grow`: un `memmove` dentro del bloque; el `addr[]` nuevo
  cae encima del `gen[]` viejo) y los workers la leen SIN cerrojo (`bpref_deref`, `bpvm_ref_dead`). Por eso sólo
  crece **con el mundo parado**, como se colecta (V7/H7 `#610`): en la puerta de la reserva (`tabla_con_sitio` →
  `tabla_crecer`, `heap.c`), antes de reservar el bloque y otra vez tras la colecta del reintento, que suelta el
  cerrojo mientras espera. El alta del handle no crece la tabla con otros workers en marcha. En miVM crecer hace
  arrays nuevos y deja intactos los viejos.

---

## 2. Header de objeto en heap

Cada objeto en el heap ocupa `OBJ_HEADER_SIZE = 8 bytes` de cabecera
+ payload (alineado a 4 bytes).

```
                    ↓ headerAddr
                    ┌────────────┬────────────┐
                    │   tag      │  length    │   ← 8 bytes header
                    │  (u32)     │  (u32)     │
                    ├────────────┴────────────┤
                    │                         │
                    │   payload               │   ← user_ref = headerAddr + 4
                    │                         │
                    └─────────────────────────┘
```

- `user_ref` (lo que ve el programa BP) = `headerAddr + 4`. Apunta al
  primer byte del `length`/`class_ptr`.
- `length` (en `+4`, dentro del payload):
  - Para arrays: número de elementos (no de bytes).
  - Para objetos: `class_ptr` (dirección absoluta del descriptor en el
    data block del módulo dueño).

### 2.1 Bits del `tag`

```
bit 31  TAG_MARK_BIT      0x80000000   GC: objeto marcado vivo
bit 30  TAG_FREE_BIT      0x40000000   bloque libre (tras GC)
bit 29..24  TAG_TYPE      0x3F000000   tipo (6 bits, 0..63)
bit 23  (CLAVADO)         0x00800000   sólo mientras se compacta (abajo)
bit 22  (SOLTANDO)        0x00400000   sólo mientras se suelta un owner en la VM-C (abajo)
bit 21  TAG_ASCII         0x00200000   una cadena «todo ASCII» (V8/H5, 21-oct)
bit 20  TAG_SOLO_LECTURA  0x00100000   un bloque de solo lectura: todas las cadenas (V8/H5, #714, 23-oct)
bit 19..0   reservados                 (0 hoy)
```

**`TAG_ASCII` (bit 21)** — los dos algoritmos de las cadenas (`V8_IDEAS` §E5, «Dos algoritmos»): lo pone quien crea la
cadena, por los bytes que escribe; 0 es «no se sabe, o no lo es», y va por el algoritmo lento, que siempre acierta.

**`TAG_SOLO_LECTURA` (bit 20)** — una cadena es de solo lectura desde que nace (`#714`). Lo pone el nacimiento de cada
cadena (`bpvm_str_nace` / `bpvm_str_nace_ascii` en la VM-C, `strNace` en miVM, y el builtin 283 `__strNace` que llaman
los `__strconcat` del compilador) y lo mira `move` en su destino: `move: dst es de solo lectura`. Las escrituras
`b[i] := x` no lo miran: un programa no puede llevar una cadena a un `byte[]`.

Los dos bits los conserva la compactación, y una reserva nueva nace sin ellos: la etiqueta se escribe entera, sólo con
el tipo. Un hueco reutilizado no hereda nada.

Mientras dura `Heap.compact()` (§2.5), y sólo entonces, el bit 31 de una cabecera significa ENHEBRADO (la
cabecera guarda un índice de la tabla de handles, no la etiqueta) y el bit 23, CLAVADO. Fuera de ahí valen 0.

Mientras se suelta un owner en la VM-C (`bpvm_heap_soltar_owned`, `#656`), y sólo entonces, el bit 22 es SOLTANDO
(`0x00400000`): un bloque ya muerto cuyos hijos owner están por soltar y que no cupo en la pila de trabajo; el repaso
del heap lo recoge y lo borra antes de acabar la instrucción. Fuera de ahí vale 0. (La VM-Java no lo usa: su pila es
de Java y no se llena.)

### 2.2 Códigos de tipo (`TAG_TYPE`)

| Valor | Constante | Significado |
|---|---|---|
| 0 | `TYPE_ARRAY_I8`  | array de bytes (1 byte por elemento) |
| 1 | `TYPE_ARRAY_I16` | array de int16 (2 bytes por elemento) |
| 2 | `TYPE_ARRAY_I32` | array de int32 / float (4 bytes — strings también) |
| 3 | `TYPE_ARRAY_REF` | array de refs (4 bytes — el GC traza cada slot) |
| 4 | `TYPE_OBJECT`    | instancia de clase; `payload[0]` = class_ptr |

### 2.3 Tamaño total

`totalSize = align4(OBJ_HEADER_SIZE + payloadBytes)`. Para
`TYPE_OBJECT`, `payloadBytes = num_fields * 4`. Para arrays,
`payloadBytes = length * sizeofElement(type)`.

Mínimo de bloque (incluso si el payload sería más pequeño):
`MIN_FREE_BLOCK = 12` (8 header + 4 next-pointer). Garantiza que un
bloque liberado quepa el puntero al siguiente en la free-list.

### 2.4 Free list

Tras `sweep()` del GC, los bloques no marcados se enlazan en una lista
con head `freeListHead`:

```
header_libre:
  [+0]  tag con TAG_FREE_BIT set, TAG_TYPE = type original
  [+4]  next_user_ref (o 0 = fin de lista)
```

`tryAllocateInner` busca en la free-list un bloque con `totalSize`
suficiente antes de bumpear `heapNext`.

### 2.5 Compactación (`Heap.compact()`, V7/H7 #595)

El GC no mueve nada; `Heap.compact()` sí, a petición del programa y en condiciones controladas (sólo desde `Main`,
sin otros hilos vivos, nunca desde una `native`: `BUILTINS.md` §270). Tras una colecta completa, con el mundo aún
parado y sin memoria aparte:

1. **Clavar** (bit 23) lo que no se puede mover aunque tenga handle: el bloque de estado de un fichero abierto (en
   la VM-C el motor del FS guarda ahí su estado, y littlefs lo enlaza en su lista de abiertos).
2. **Enhebrar** la tabla de handles en las cabeceras: por cada handle vivo, la cabecera de su bloque pasa a
   guardar `ENHEBRADO | idx` y la entrada `idx`, lo que había antes en la cabecera (la etiqueta, o el índice
   anterior si dos handles apuntan al mismo bloque). Al final de la cadena está siempre la etiqueta.
3. **Deslizar** en orden de dirección: un bloque enhebrado baja al primer sitio libre (el destino nunca pasa del
   origen) y cada índice de su cadena pasa a apuntar ahí; uno sin enhebrar —clavado, o vivo sin handle— se queda,
   y el hueco que le quede delante vuelve a la lista de libres. `heapNext` acaba tras el último bloque.

Las dos VMs hacen lo mismo (`heap.c` `compactar`; `VirtualMachine.compactarLocked`).

### 2.6 La reserva: UN método (V7/H7 #593)

Un bloque sólo entra en el heap por `bpvm_heap_nuevo(vm, tipo, cabecera, payload)` (VM-C) y
`heapNuevo(tipo, cabecera, payload)` (miVM). Bajo el cerrojo de la VM y de una vez: el hueco (lista de libres o
bump), la cabecera ENTERA —el tipo y `cabecera`, la longitud de un array o el `class_ptr` de un objeto—, el
contenido a cero, la cuenta del bloque y el handle. El bloque nace completo y con su handle: ningún GC lo ve a
medias, y la tabla de handles conoce todo bloque vivo. Quien llama sólo llena el contenido (por el handle). Antes
el asignador dejaba un 0 donde va la longitud o la clase y lo escribían a mano 23 sitios de la VM-C y 15 de miVM,
que además daban de alta el handle aparte (Eduardo: *«al final la reserva la ha de hacer un método único, no n
métodos»*).

**La cuenta** (`heap_bloques` / `heapBloques`): los bloques no libres que hay según la reserva. `+1` al nacer,
`-1` al soltar un owner (`FREE_REF`), y el barrido deja los que quedan; vuelve a 0 donde el heap se vacía (el
arranque, la arena, el enlace, `setHeapStart`).

**Las comprobaciones de cada GC**, en el recorrido de cabeceras (§2.3), además de que acabe en `heapNext`:

1. los bloques no libres que ve el recorrido, contra la cuenta; y
2. todo handle vivo que apunta dentro del heap cae en una cabecera vista y no libre.

Si algo no cuadra, el heap se ha descarrilado (§ del GC, `#494`): ese GC no marca ni barre y la reserva que lo
pidió lanza la excepción prefabricada «heap descarrilado (fallo interno de la VM)». Sin corregir nada: detectarlo
en un programa en marcha es una excepción (Eduardo). Lo que cazan y el guardián de `#494` no: un bloque que dice
lo justo para tragarse el SIGUIENTE, con lo que el recorrido se recompone y acaba en su sitio (el punto ciego de
`#593`, por donde se coló `#597` una vez de cada tres); o uno que dice MENOS de lo que mide, que deja al recorrido
inventarse bloques en su contenido. Contar bytes no valdría: el que miente se traga justo los bytes que se salta.

---

## 3. Layout del Class Descriptor

Vive en el data block del módulo dueño. Lo apunta `class_ptr` (= primer
slot de un objeto `TYPE_OBJECT`). Reproducido aquí por completitud
desde `MOD_FORMAT.md §8`:

```
+0    u16   num_fields
+2    u16   num_methods
+4    u16   bitmap_words   = ceil(num_fields/32)
+6    i16   modulo         el sello del dueño. 0 = MOD7/8 (era _pad):
                           el CS se busca por rango. MOD9: el cargador
                           escribe índice del módulo dueño + 1 en TODO
                           descriptor al registrarlo, cerrado o
                           construido — un construido (clase abierta)
                           vive en la arena, fuera de todo módulo, y
                           sin el sello no tendría CS.
+8    i32   parent_offset  CS-relative al CS del módulo dueño;
                           0 = sin padre; sentinel cross-module
                           se PARCHA en linkAll (v8); en MOD9 lo
                           escribe el cargador al construir.
+12   bw*4  field_bitmap   bit k=1 ⇒ field[k] es ref (GC trace)
+12+bw*4 bw*4 owner_bitmap bit k=1 ⇒ field[k] es owner (FREE recursivo)
+12+2*bw*4 num_methods*4 vtable  i32 offsets relativos al codeStart
                                  del módulo dueño. -1 = no
                                  implementado localmente (la VM
                                  sube al parent).
```

**Navegación cross-module del parent**: cuando `parent_offset` apunta a
un descriptor que vive en otro módulo, la VM hace:

```
parentAbs = moduleManager.getCSForDataAddr(childDesc) + parent_offset
```

`getCSForDataAddr` busca en `loadedModules` cuál módulo contiene `childDesc`
en su rango `[moduleBase, codeStart + codeSize)` y devuelve su `codeStart`.
**MOD9**: si `+6 != 0` devuelve el `codeStart` de `modules[+6 − 1]` sin buscar —
los descriptores construidos viven en la **arena** (detrás del último módulo, delante
del heap; `MOD_FORMAT.md` §13.4) y ningún rango los contiene.

---

## 4. Stacks de los threads BP

Cada thread BP tiene una **región** dentro del área de pilas. La región se asigna al crear el
thread y, cuando el thread acaba del todo, **se reutiliza** para otro (en las dos VMs; la VM-C, desde
`#604`, 29-sep: antes no reutilizaba nada y un programa sólo podía crear 32 hilos en su vida).

```
stackBase ━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ↓ dirección baja
          │                            │
          │   frame del main del thread│  ← bp del primer call
          │   (args, locales)          │
          │                            │
          │   frame inner              │  ← bp tras CALL
          │   ...                      │
          │                            │
       sp ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ← top actual (apunta al
          │                            │     siguiente slot LIBRE,
          │   espacio libre            │     no al último ocupado)
          │                            │
stackTop  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ↑ dirección alta
                                          (excluida del rango)
```

Tamaños:
- **Thread main**: `MAIN_STACK_BYTES = 16 KiB`.
- **Los demás**: lo que pide su constructor, `Thread(stackSize)` (campo 1 de `Core.Thread`); con 0,
  `DEFAULT_THREAD_STACK_BYTES = 2 KiB`. El `stackTop` de un thread es SU tamaño pedido, aunque la
  región que le toque (una reutilizada) mida más: así el tope es el mismo en las dos VMs.
- **Hasta 32 threads vivos a la vez**, el main incluido (`BPVM_MAX_THREADS`; `MAX_HILOS_VIVOS` en
  miVM). Uno más, o una pila que no cabe: `RuntimeError` en `start()`.

**El muro (`#605`, 29-sep).** Cada función empieza por `ENTER`, y ahí se mira el tope: si
`sp + locales + ZONA_ROJA > stackTop`, `RuntimeError` «desbordamiento de pila (el hilo tiene N B)»,
atrapable. La **zona roja** (`BPVM_PILA_ZONA_ROJA` / `PILA_ZONA_ROJA` = 256 B) es lo que una función
puede apilar después de su `ENTER` sin volver a mirar —los operandos, los argumentos de una llamada y
los 12 B del `CALL`— hasta el `ENTER` siguiente. La misma cuenta en las dos VMs: la excepción sale en
la misma instrucción. Antes nadie miraba, y un thread que recursaba pisaba la pila del de al lado.

### 4.1 Layout de un frame de función

Cada `CALL` reserva en la pila del thread:

```
↑ direcciones altas

         locales_N
         ...
         locales_0          ← ENTER reserva `localsBytes` aquí
   bp →  ┌──────────────┐
         │ saved_cs     │   [bp - 4]   CS del caller
         │ saved_bp     │   [bp - 8]
         │ saved_pc     │   [bp - 12]  PC del caller (return address)
         ├──────────────┤
         │ arg_N        │   [bp - 16]
         │ ...          │
         │ arg_0        │
         └──────────────┘
↓ direcciones bajas
```

El `CALL` empuja args en orden, luego saved_pc, saved_bp, saved_cs, y
fija `bp = sp`. El callee hace `ENTER localsBytes` que avanza `sp`.

El `RET paramsCount` revierte: pop del **top del stack** del callee
para obtener el `returnValue`, restaura `pc/bp/cs` de los slots
guardados, `sp ← bp - 12 - paramsCount*4`, y push del `returnValue`
en el nuevo sp del caller. El callee debe asegurar que `returnValue`
está en el top justo antes del `RET` (típicamente con `GET_LOCAL
+slot` que apunta al "return slot" donde la función dejó su
resultado, o con `PUSH 0` para funciones void).

---

## 5. Registros del intérprete

Por thread (`ThreadContext`):

| Campo | Tamaño | Significado |
|---|---|---|
| `id` | int | tid (0 = main) |
| `pc` | int | program counter (dirección absoluta en `memory[]`) |
| `sp` | int | stack pointer |
| `bp` | int | base pointer del frame actual |
| `cs` | int | code-start del módulo actual (cambia con CALL_EXT/RET) |
| `status` | enum | RUNNABLE, RUNNING, BLOCKED_*, TERMINATED |
| `blockedOnMutex` | int | id del mutex que espera, o -1 |
| `wakeAt` | long | ms epoch para BLOCKED_SLEEP |
| `stackBase` | int | dirección baja de su región de pila |
| `stackTop` | int | dirección alta (excluida) |
| `yieldRequested` | bool | preempt timer puso esto a true |
| `allocAnchor` | int | ref del último heapAlloc todavía sin publicar (raíz GC) |
| `handlerStack` | Deque | pila de exception handlers (entradas de TRY) |
| `ehHandlerPc/SavedSp/SavedBp/SavedCs/ExpectedClass` | int×5 | top del handlerStack desempaquetado para acceso rápido |

Los registros se sincronizan así:
- **Durante `runOnContext(tc)`**: el intérprete trabaja sobre locales
  Java (`pc`, `sp`, `bp`, `cs`) inicializadas desde `tc.*`. Cualquier
  cambio se persiste a `tc.*` al final del while o antes de un context
  switch (yield, builtin que cede, fault).
- **Fuera del intérprete**: la fuente de verdad es `tc.*`.

---

## 6. Convención de Strings  *(V2 H2 — UTF-8)*

> **Cambio en V2 (H2).** Hasta V1 un string era un array de codepoints
> `TYPE_ARRAY_I32` (4 bytes/char). En V2 un string es un **`byte[]` UTF-8**
> `TYPE_ARRAY_I8` (1 byte/elem). Esto NO cambió la estructura del `.mod`
> (solo la codificación de los bytes de un símbolo de string en el data
> block) — fue aditivo, igual que añadir tipos.

Los strings BP se representan como `byte[]` con contenido **UTF-8** y tipo
`TYPE_ARRAY_I8`:

```
[+0]  tag (type = 0, TYPE_ARRAY_I8)
[+4]  length (en BYTES UTF-8, no en codepoints)
[+8]  byte 0
[+9]  byte 1
[ ...]   (ASCII = 1 byte; é = 2 bytes; € = 3 bytes; …)
```

Es **exactamente un `byte[]`** — comparte toda la maquinaria de arrays I8
(alloc, GC, tamaño de bloque). `string` y `byte[]` son indistinguibles en
runtime; la diferencia es solo el tipo estático del compilador.

**Semántica de índice = por codepoint** (no por byte): `strlen`, `charAt`,
`charCodeAt`, `substring`, `indexOf` cuentan/indexan codepoints (decodifican
UTF-8). Para ASCII, codepoint == byte (idéntico a V1). Acceso a bytes crudos
vía conversión `toBytes(s): byte[]` / `fromBytes(b: byte[]): string` (copia
defensiva: el string es inmutable).

Las dos VMs comparten los helpers UTF-8 (`utf8_cp_count/byte_offset/decode/
encode` en `bpvm_internal.h` para el lado C) para que bytecode y AOT, y Java
y C, coincidan byte a byte. El `print` emite los bytes UTF-8 directamente (la
VM C ya **no** trunca no-ASCII a `'?'`).

Concat (`__strconcat`) y comparación (`__strequals`) son funciones inyectadas
por el emisor (`s1 + s2`, `s1 == s2`); operan a nivel de **byte** (concatenar
o comparar dos secuencias UTF-8 válidas es correcto byte a byte) usando los
opcodes `NEWARRAY_I8`/`ALOAD_U8`/`ASTORE_I8`.

---

## 7. Raíces del GC

`gc()` corre stop-the-world (todos los workers parqueados en
`heapAlloc` o saliendo por safepoint). Marca desde:

1. **Stack de cada thread**: escanea `[stackBase, sp)` palabra por
   palabra. Conservadoramente: cualquier i32 que caiga dentro del rango
   del heap se trata como ref potencial.
2. **`allocAnchor` de cada thread**: el ref recién alocado todavía no
   publicado al stack queda anclado aquí (cubierto en `B1 residual`).
3. **Data block de cada módulo cargado**: tiene refs globales (e.g.
   variables module-level de tipo string/class).

Tras marcar, `sweep()` recorre el heap, libera los no marcados
(setea `TAG_FREE_BIT`) y los enlaza en `freeListHead`.

Para arrays `TYPE_ARRAY_REF`, el sweep traza cada slot también.

Para objetos `TYPE_OBJECT`, el sweep:
- Lee `class_ptr` del payload.
- Lee `field_bitmap` del descriptor.
- Para cada bit 1, traza el field como ref.
- Sube por `parent_offset` para incluir fields heredados (cuyo bitmap
  también puede tener refs en posiciones inferiores).

---

## 8. Numerología fija

| Constante | Valor | Significado |
|---|---|---|
| `OBJ_HEADER_SIZE` | 8 | bytes del header de heap (tag + length) |
| `MIN_FREE_BLOCK` | 12 | bytes mínimos para que el bloque libre quepa el next-pointer |
| `TAG_MARK_BIT` | `0x80000000` | bit 31 del tag |
| `TAG_FREE_BIT` | `0x40000000` | bit 30 del tag |
| `TAG_TYPE_MASK` | `0x3F000000` | bits 24..29 |
| `TAG_TYPE_SHIFT` | 24 | |
| `MAIN_STACK_BYTES` | 16384 | tamaño del stack del thread main |
| `DEFAULT_THREAD_STACK_BYTES` | 2048 | tamaño de stack por thread nuevo |
| `DEFAULT_MEMORY_SIZE` | 524288 | bytes totales (default) |
| `DEFAULT_STACK_BASE` | 262144 | `memorySize/2` |
| `EXT_TABLE_ENTRY_SIZE` | 4 | bytes por entrada en la ext-table |
| `INITIAL_FREE_ADDRESS` | `0x0100` | primer byte libre tras la sentinela |

---

## 9. Endianness

Toda la memoria interna de la VM (heap, stacks, data blocks, code
block) está en **big-endian** — coincide con el `.mod` en disco.
Cualquier acceso (lectura o escritura) de un i32 a `memory[addr]`
asume big-endian.

La VM C debe usar helpers que respeten esto independientemente de la
arquitectura host:

```c
static inline uint32_t read_u32_be(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}
static inline void write_u32_be(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8);
    p[3] = (uint8_t) v;
}
```

(En MCUs little-endian como ARM Cortex-M, esto significa un swap por
cada acceso. Es el precio de la portabilidad de los `.mod` entre
plataformas. Si el coste se vuelve relevante, se puede añadir una
variante "host-native endian" del `.mod` y un flag, pero ese trabajo
queda fuera de v5.)
