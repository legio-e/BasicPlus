# BasicPlus V4 — Abstracción de Referencia (charla de diseño)

> Charla de diseño, **9-jul-2026** (Eduardo + Claude). Origen: el ensanchado de
> referencias 4→8B (H1.2a) se estaba haciendo **a parches por miles de líneas**.
> Eduardo paró: *"eso no es forma de trabajar"*. Este documento fija el diseño
> correcto para que **cambiar el tamaño o la representación de una referencia sea
> un cambio de UN sitio**, no un barrido por todo el árbol.
>
> **Alcance decidido (Eduardo, 9-jul):** se refactoriza **SOBRE el estado actual**
> (paridad dual-VM canónica 16/17 verde; el único rojo es `idxtest`, hueco
> conocido de subconjunto de builtins de la VM-C). **NO se revierte** lo hecho;
> se aprovecha. "Tardemos lo que tardemos, pero hagámoslo bien."

---

## 0. TL;DR

- **Problema raíz:** *"referencia" está confundida con "entero de 64 bits".* El `8`
  se usa como proxy de **dos hechos distintos**: "es un número de 64 bits"
  (`long`/`double`) y "es una referencia". Al compartir el proxy, comparten
  opcodes (`ALOAD_I64`, `GET_FIELD_LONG`), predicado (`occupies8Bytes`) y
  aritmética (`i*8`, `sp-=8`) → cambiar el ancho obliga a tocarlo **todo**.
- **Diseño (los 3 requisitos de Eduardo):**
  1. **`REF` = *kind* de primera clase.** El ancho se **deriva** del kind, nunca al revés.
  2. **Operaciones de referencia propias** — que *conste* que es una referencia
     (no reusar las de `long`; no ensanchar `int→long` para meter en hueco de ref).
  3. **Un único `REF_SIZE`** + `readRef`/`writeRef` que encapsulan la codificación.
- **Premio:** H1.2a pasa a ser *"poner `REF_SIZE=8`"*. El modelo de handles
  ([memoria `v4-modelo-memoria-handles`], `bp_propuesta_modelo_memoria/`) pasa a
  ser *"reescribir `readRef`/`writeRef`"*. **Sin volver a tocar mil sitios.**

---

## 1. El error: "referencia" ≡ "long 64-bit"

Hoy el sistema decide el ancho de un valor en **tres capas**, y **ninguna** tiene
un concepto de "referencia". Todas usan "mide 8 bytes" como sinónimo de dos cosas.

### Capa de tipos (frontend, `basicplus.frontend` / `BpType`)
- Una referencia **no es un tipo**: es propiedad emergente de `ClassType` /
  `ArrayType` / `String` (`isRefType`).
- El ancho se pregunta con `occupies8Bytes(t) = is8Byte(t) || isRefType(t) || AnyType`,
  que **mezcla** "es long/double" con "es referencia" con "es any".

### Capa de emisor (`MivmEmitter` + `ModWriter`)
- Para mover referencias **reusa los opcodes de `long`** (`ALOAD_I64`,
  `ASTORE_I64`, `GET_FIELD_LONG`, `SET_FIELD_LONG`, `GET_LOCAL_L`, `LRET`…),
  porque hoy ambos miden 8.
- Para pasar un `int` a un hueco de referencia (`any`) emite **`I32_TO_I64`** —
  semánticamente un disparate: una conversión int→long haciendo de "coloca esto
  en un slot de referencia".

### Capa de runtime (`VirtualMachine.java` Java · `interp.c`/`builtins.c`/`heap.c` C)
- `4` y `8` **a piñón** por todas partes: `sp-=8`, `i*8`, `+4` de cabecera,
  `writeI64`/`bpvm_write_i64_be`, `elementSize()==8`, `paramsCount*4`…

**La confusión ES el bug.** En cuanto quieras que una referencia **no** mida 8
(handles con otra codificación, ref de 4 otra vez, handle de 6 bytes…), el proxy
se rompe y vuelves a parchear. `long` seguirá midiendo 8 siempre (ES 64-bit); una
referencia mide `REF_SIZE`, que es **otra cosa**.

---

## 2. Requisitos (Eduardo, textual)

1. *"Debería haber una definición de referencia."*
2. *"En todos los sitios donde se emplee una referencia debería constar que es
   una referencia, no pasar de un int32 a un long (int64)."*
3. *"Poder cambiar el tamaño de la referencia de 4 bytes a 8."*
4. *(Corolario)* *"De esta manera la mayoría del trabajo se haría solo sin
   necesidad de estar retocando miles y miles de líneas."*

---

## 3. Diseño

### 3.1 `REF` como *kind* de primera clase (capa de tipos)

Introducir un **kind de valor explícito**:

```
ValueKind ::= INT32 | INT64 | FLOAT32 | FLOAT64 | REF | ...
```

- `ClassType`, `ArrayType`, `String`, y "any que contiene ref" reportan `kind = REF`.
- El **ancho se deriva** del kind: `slotBytes(REF) = REF_SIZE`, `slotBytes(INT64)=8`, `slotBytes(INT32)=4`.
- El emisor pregunta **`isReference(t)`**, jamás `occupies8Bytes`. Donde hoy hay
  `occupies8Bytes` se separa en dos preguntas independientes:
  `isReference(t)` (ancho = `REF_SIZE`, el GC lo traza) vs `is64BitScalar(t)`
  (long/double, ancho fijo 8, el GC lo ignora).

### 3.2 Operaciones de referencia propias (capa de emisor + opcodes)

Las referencias se mueven con **operaciones de referencia**, no con las de `long`.
Conceptualmente: `ALOAD_REF`/`ASTORE_REF`, `GET_FIELD_REF`/`SET_FIELD_REF`,
`LOAD_LOCAL_REF`/`STORE_LOCAL_REF`, `LEA_*` empuja un `REF`, `RET_REF`.

- **Por qué importa:** el disasm, el AOT y la VM *saben* que ahí viaja una
  referencia → su implementación consulta `REF_SIZE`/`readRef`/`writeRef`. Las
  ops de `long` se quedan en `sp-=8`/`readI64` **para siempre** (long ES 64-bit).
  Los dos motivos quedan **desacoplados**.
- Un `int` **nunca** se "ensancha" a referencia: son kinds distintos → el
  `I32_TO_I64` como puente desaparece. (El caso `any` es una unión etiquetada,
  §5, no un ensanchado.)
- **Nota de implementación** (a decidir en el plan): los opcodes de referencia
  pueden ser bytes **nuevos** (explícito, pero hay que enseñárselos a 2 VMs +
  disasm + AOT) o reusar los bytes `*_I64` **enrutando por metadato** (el array
  ya lleva `TYPE_ARRAY_REF`; el campo lleva `isRef`), de modo que la VM decida
  `readRef` vs `readI64` por el tipo del contenedor. Lo **irrenunciable** es la
  separación semántica; el reparto de bytes es táctica.

### 3.3 `REF_SIZE` + `readRef`/`writeRef` (capa de runtime/codificación)

- `REF_SIZE` en **UN** sitio, compartido conceptualmente por ambas VMs (mismo
  valor en `OpCode`/`bpvm.h`).
- `readRef(mem, addr)` / `writeRef(mem, addr, ref)` **encapsulan la codificación**:
  - **Hoy (plano):** `writeRef` = escribe i64 big-endian con `high=0, low=dirección`;
    `readRef` = `(int) readI64` (palabra baja). `REF_SIZE = 8`.
  - **Handles (V4):** `writeRef` = codifica `(índice, generación)`; `readRef` =
    decodifica → puntero real. `REF_SIZE` = lo que pida el handle.
  - **Ref de 4 (si algún día):** `REF_SIZE = 4`, `readRef`=`readI32`.
- Todo stride/offset/hueco se calcula de ahí: `sp -= REF_SIZE`,
  `elemAddr = base + HEADER + i*REF_SIZE`, `paramSlots` cuenta `REF_SIZE/4` por ref.

**Sutileza a formalizar — layout de campos en SLOTS.** Hoy el GC lee el campo-ref
`i` en `OBJ_HEADER_SIZE + i*4` con `readI64` (8 bytes) mientras los campos se
empaquetan en slots de 4 bytes y un campo de 8 ocupa 2 slots (encoding "plano"
solapado). La abstracción debe expresar **"offset del campo i" vía una tabla de
slots** (un ref ocupa `REF_SIZE/4` slots), no `i*4`, y leer/escribir con
`readRef`/`writeRef` a offset alineado a slot.

---

## 4. Plan de migración por capas (refactor sobre lo actual, sin revertir)

Orden sugerido (cada paso guarda la canónica dual-VM como red viva):

1. **Constante + helpers.** `REF_SIZE`, `readRef`/`writeRef` en ambas VMs
   (implementación = la plana actual). Cero cambio de comportamiento.
2. **Tipos.** `isReference(t)` + `slotBytes(kind)`; separar los usos de
   `occupies8Bytes` en `isReference` vs `is64BitScalar`.
3. **Emisor.** Donde se decide ancho de ref (locals, params, campos, subscript,
   return, frame) → derivar de `isReference`+`REF_SIZE`; retirar el `I32_TO_I64`
   int→any (§5).
4. **VMs.** Sustituir los `4`/`8` de referencia por `REF_SIZE` y los
   `readI64`/`writeI64` de referencia por `readRef`/`writeRef`. Layout de campos
   por tabla de slots.
5. **Cierre listas internas** (lo que quedó a medias esta sesión) cae **solo** al
   estar la abstracción: List/SyncList, tuplas, y el blocker `cs=0` (§6).
6. **Prueba:** canónica 18/18 + amplio + listas, dual-VM byte-idéntico.

---

## 5. `any` — unión etiquetada (concern APARTE, no volver a mezclar)

`any` puede llevar **un escalar O una referencia**. No es "un valor de 8 bytes":
es una **unión** cuyo hueco es "lo bastante grande para cualquier kind"
(`max` sobre los kinds) y con su **propia disciplina** de carga/guardado (y, si
hiciera falta distinguir en runtime, su etiqueta). El `any=8B` de esta sesión fue
**otro parche del mismo error** (tratar `any` como long). En el diseño limpio,
`any` tiene su propio ancho y sus propias ops; no se "ensancha int→any".

---

## 6. Inventario de sitios donde se toca una referencia (el MAPA del refactor)

> Esto es lo que Eduardo llamó *"trabajo que recuperaremos cuando volvamos a
> transformar referencias"*. Es el censo real levantado por el parcheo de esta
> sesión — el mapa para hacerlo bien.

**Emisor — `MivmEmitter.java`:**
- `occupies8Bytes` (predicado de ancho) → separar en `isReference` + `is64BitScalar`.
- `aloadOpFor` / `astoreOpForElement` (dispatch de ancho en subscript).
- `declareLocal` vs `declareLocalLong` para refs: `emitConstruction`, store-a-campo
  `__stf_`, var de `catch`, `__strconcat`, sitios `__newref` de Mutex, temps de
  tupla `__tup_`/`__dst_`/`__de_`.
- `declareParam(name, 8)` para params ref (`this`, `item`).
- Elección `GET_FIELD` vs `GET_FIELD_LONG` según `field.is8`.
- `beginFunctionScope` (`returnsLong` para return de ref → `LRET`).
- `coerceToTarget` int→any (**el hack `I32_TO_I64`** — retirar).

**Emisor — `ModWriter.java`:**
- `addMethod`/`addPrivateMethod`: `declareParam("this", 8)`.
- `declareField(..., is8)` (4-arg) para campos ref; `nextSlot = slot + (is8?2:1)`.
- `paramSlots()` (RET pop = Σ `sz/4`), `paramOffset()`, `emitRet`/`emitLRet`.

**Runtime miVM — `VirtualMachine.java`:**
- Pops/push de ref (`sp-=8`+`(int)readI64` / `writeI64`+`sp+=8`) en: `ALOAD`/`ASTORE`,
  `ALEN`, tipados, `GET_FIELD`/`SET_FIELD`, `INSTANCEOF`, `FREE_REF`, receptor de
  `INVOKE_VIRTUAL` (`sp-8-numArgs*4`).
- `LEA_GLOBAL`/`LEA_LOCAL` (empujan ref 8B).
- `pushTcRef`/`popTcRef` (pila de builtins).
- `allocVmRefArray` (`n*8`), `GROW_REF_ARRAY`, `SPLIT`, `LIST_DIR`, free-cascade (stride 8).
- GC `markObject`: `TYPE_ARRAY_REF` (stride 8) + campos-ref del objeto (`readI64` en `i*4` — el solapado plano).
- `throwBpRuntimeError` (campo `msg` 8B).
- Layout de objeto: `heapAlloc(numFields*4)` + slot de campo por `is8`.
- Frame de llamada: `RET`/`LRET` `paramsCount*4`, offset del receptor, guardado pc/bp/cs.
- `elementSize()` `TYPE_ARRAY_REF → 8`.

**Runtime VM-C — `interp.c` / `builtins.c` / `heap.c` / `exceptions.c`:**
- **Espejo de todo lo anterior** (aún NO migrado esta sesión, solo miVM): `pop_ref`/`push_ref`,
  `bpvm_read_i64_be` del receptor, stride de ref-array, ancho de `GET/SET_FIELD` de ref,
  `GROW`/`newRefArray`, GC, `OP_THROW` msg.

**Números mágicos a centralizar:** `REF_SIZE` (=8 hoy), `OBJ_HEADER_SIZE`/`+4`,
la codificación plana (`high=0`/`low=addr` big-endian).

---

## 7. Blocker abierto que la abstracción debe cerrar

**`cs=0` tras `INVOKE_VIRTUAL` a método con arg de 8B** (`List.add(item)`): al
volver de `add()`, el registro `cs` queda a 0 → el siguiente `LEA_GLOBAL -N` de
Main resuelve a `-N` (negativo) → `AIOOBE` en `readVmString`. Repro `tm_add`
(`-392`). La aritmética de frame (RET pop = 4 slots) cuadra sobre papel; sospecha
en `getCSForDataAddr` de la clase sintetizada o en el layout de objeto cambiado
(`items` a 2 slots). **No es regresión** — es el eslabón que la migración limpia
del frame/layout debe resolver. (Detalle vivo en memoria `h1-2a-wip`.)

---

## 8. Estado

- **Guardado (commit `83b0afe`):** migración parcial de listas a refs-8B (emisor +
  runtime miVM). Canónica dual-VM **16/17** (rojo único = `idxtest`, hueco VM-C conocido).
- **Decidido:** refactor de la abstracción **sobre** este estado, sin revertir.
- **Siguiente:** ejecutar el §4 paso a paso, empezando por `REF_SIZE` + `readRef`/`writeRef`.
