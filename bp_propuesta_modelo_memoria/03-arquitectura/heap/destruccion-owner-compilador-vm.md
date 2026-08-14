# Destrucción de objetos `owner`: reparto compilador ↔ VM

**Rol:** observador-analista del código (solo lectura). **Commit:** `c2f82929`
(el `pm` local coincide con GitHub `main`). **Fecha:** 2026-07-05.

> Principio del proyecto (Eduardo): **la VM es un autómata que no piensa; la
> inteligencia está en el compilador.** Por tanto, para juzgar cómo se destruyen
> los objetos `owner` hay que mirar **qué emite el compilador**, no solo cómo lo
> ejecuta la VM. Analizar solo la VM lleva a conclusiones falsas.

## Los tres casos, atribuidos a quién los resuelve

### Caso 1 — reasignar una variable `owner` (libera el valor anterior)
- **Quién:** el **compilador**. Ante `x := y` con `x` owner, emite `FREE_REF`
  (variable) o `SET_FIELD_OWNER` (campo) del valor anterior antes de escribir.
- **VM:** `OP_FREE_REF` (`interp.c:1490`) y `OP_SET_FIELD_OWNER` (`interp.c:1506`)
  ejecutan el mecanismo de leaf. `SET_FIELD_OWNER` libera el valor viejo del slot
  antes de escribir el nuevo (`interp.c:1513-1523`).
- **Veredicto:** correcto en el diseño. (Salvedad de bookkeeping → H-010, abajo.)

### Caso 2 — variable `owner` local liberada al terminar la función
- **Quién:** el **compilador**, y lo hace con elegancia. Cada `FuncScope` mantiene
  `ownerLocals` (orden de declaración) y canaliza **todas** las salidas por un
  **único `endLabel`**:
  - *fall-through*: `emitFunctionEnd()` llama a `emitFreeOwnerLocals()` antes del
    `RET` (`MivmEmitter.java:2588-2591`).
  - *return explícito*: emite su propia liberación de owners + `JUMP endLabel`
    (comentario `MivmEmitter.java:2585`).
  - `emitFreeOwnerLocals()` (`:2552`) hace `GET_LOCAL name` + `FREE_REF` por cada
    owner local, en orden de declaración.
- **VM:** `OP_RET` (`interp.c:802`) **no** toca owners — **correcto**: no tiene por
  qué; no conoce los tipos. La limpieza ya viene emitida antes del `RET`.
- **Veredicto:** correcto para el flujo normal. **Pregunta abierta (no bug):** un
  `throw` que desenrolla la pila salta directamente al handler (restaura
  sp/bp/cs/pc desde la entry del TRY, `OPCODES §0x5D`), **sin pasar por el
  `endLabel`** de los frames intermedios → sus owner locales **no** se liberan por
  esta vía. Probablemente sea intencionado (el GC es la red de seguridad en el
  camino de excepción; `owner` optimiza el camino normal). **A confirmar con el
  usuario** si es diseño aceptado o hueco. Ver [Q-owner-excepcion](#preguntas).

### Caso 3 — destruir un objeto con campos `owner` (cascada)  ⚠️
- **Quién PRETENDE resolverlo:** la **VM**, por diseño necesario — la cascada
  recorre el `owner_bitmap` del **descriptor de la clase**, que solo se conoce en
  runtime (polimorfismo); el compilador no puede emitirla genéricamente. El
  compilador lo da por hecho: `OwnerList` promueve su campo `items` a `owner`
  (`setFieldOwner("items")`, `MivmEmitter.java:3707`) y **confía** en que
  `FREE_REF` cascadee (comentario `:3695-3697`: *"el VM verá items marcado como
  owner y… cascadeará liberando primero cada slot… y después el array mismo"*).
- **Qué hace la VM en realidad:**
  - `OP_FREE_REF` (`interp.c:1490-1503`): el recorrido del `owner_bitmap` es un
    **TODO** (`:1498-1499`, *"F3 v1 sólo libera el objeto raíz"*). **No cascadea.**
  - Y además `FREE_REF` es **NOP para arrays** (`:1493`, *"Sólo objetos. Para
    arrays / strings, NOP"*): `items` es `TYPE_ARRAY_REF`, así que incluso si el
    bitmap-walk lo alcanzara, liberar el array sería un no-op.
- **Veredicto:** **la cascada del caso 3 NO está implementada** en la VM, mientras
  que el compilador construye su feature estrella (`OwnerList`) **encima de esa
  cascada inexistente**. Resultado: `OwnerList` **no libera sus elementos de forma
  determinista**; dependen del GC. Es [H-006](../../hallazgos/H-006-freeref-cascada.md),
  ahora confirmado desde **ambos lados** (expectativa del compilador + no-impl de
  la VM), con línea exacta.

## El defecto de bookkeeping de `FREE_REF` (nivel VM, independiente del compilador)

Esto **sí** es responsabilidad exclusiva de la VM: cómo deja el bloque. Tanto
`OP_FREE_REF` (`interp.c:1500`) como `OP_SET_FIELD_OWNER` (`interp.c:1519`)
liberan así:

```c
bpvm_write_u32_be(mem + header, tag | BPVM_TAG_FREE_BIT);   // marca FREE_BIT y ya
```

Solo ponen `FREE_BIT`. **No** escriben el tamaño del bloque en `+4` **ni** lo
enlazan en la `free_list`. Pero el invariante del heap (`heap.c`) es que un bloque
libre lleva `[tag FREE][size@+4][next@+8]` (`add_to_free_list`, `heap.c:149`), y
`block_total_size` (`heap.c:36-38`) **lee el tamaño de `+4`** para todo bloque con
`FREE_BIT`. Tras `FREE_REF`, en `+4` sigue el `class_ptr` del objeto (su antiguo
`length`). Consecuencias → [H-010](../../hallazgos/H-010-freeref-bloque-inconsistente.md):
1. El bloque **no entra en la free-list** → no se reutiliza hasta el próximo
   sweep (memoria retenida).
2. En el próximo `gc_sweep_phase`, `block_total_size` devuelve el `class_ptr` como
   "tamaño" → `cur += class_ptr` **desincroniza el barrido** del heap → objetos
   saltados no se limpian (mark bits pegados → retención permanente) o lectura
   descolocada.

## Hallazgos incidentales del GC (fuera del tema `owner`, pero de seguridad de memoria)

Encontrados de paso leyendo `heap.c`; **pendientes de elevar** si el usuario quiere:
- **[GC-1] `long[]`/`double[]` invisibles al GC.** `is_heap_ref` (`heap.c:90`)
  acepta `type ∈ [0..BPVM_TYPE_OBJECT=4]`, pero `BPVM_TYPE_ARRAY_I64 = 5`
  (`bpvm_internal.h:44`). Una ref a un `long[]`/`double[]` no se reconoce como raíz
  → no se marca → **se recolecta estando viva** (UAF latente).
- **[GC-2] Raíces incompletas.** `gc_mark_phase` (`heap.c:134-146`) escanea **solo
  las pilas**. `HEAP_LAYOUT §7` manda escanear también `allocAnchor` (TODO,
  [H-001](../../hallazgos/H-001-allocanchor-raiz-gc.md)) **y los data blocks de los
  módulos**. Los globales de módulo / campos estáticos que apunten a heap **no son
  raíces** → recolectables en vivo.

## <a name="preguntas"></a>Preguntas para el usuario
1. **Caso 2 / excepciones:** ¿es diseño aceptado que los owner locales de frames
   desenrollados por un `throw` se dejen al GC (no se liberen deterministamente)?
2. ¿Elevo GC-1 y GC-2 a hallazgos? Son de seguridad de memoria (UAF / recolección
   prematura), aunque ajenos al hilo `owner`.

## Evidencia (ficheros:línea)
`interp.c:802` (RET), `:1490` (FREE_REF), `:1506` (SET_FIELD_OWNER) ·
`heap.c:36` (block_total_size), `:90` (is_heap_ref), `:134` (gc_mark_phase),
`:149` (add_to_free_list) · `bpvm_internal.h:39-44` (tipos) ·
`MivmEmitter.java:149` (ownerLocals), `:2552` (emitFreeOwnerLocals), `:2588`
(emitFunctionEnd), `:3701` (synthesizeOwnerListClass).
