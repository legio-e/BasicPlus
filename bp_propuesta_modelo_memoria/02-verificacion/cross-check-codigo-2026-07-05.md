# Cross-check de las afirmaciones del código contra `main`

**Fecha:** 2026-07-05 · **Commit:** `c2f82929` (el `pm` local == GitHub `main`) ·
**Rol:** verificación independiente (solo lectura). **Motivo:** antes de comprometer
el rediseño del modelo de memoria (V4), confirmar que las afirmaciones `fichero:línea`
sobre las que se apoya la propuesta **siguen siendo ciertas en el código real**.

## Veredicto: las tres afirmaciones núcleo se confirman EXACTAS

| # | Afirmación (doc) | Evidencia verificada | Veredicto |
|---|---|---|---|
| **H-006** | La cascada de `owner` **no** está implementada; `FREE_REF` solo libera la raíz y es NOP para arrays | `interp.c:1497-1501` — dentro de `if (type == BPVM_TYPE_OBJECT)` hay un `/* TODO: recorrer owner_bitmap… F3 v1 sólo libera el objeto raíz */` y solo hace `write(header, tag \| FREE_BIT)`. `interp.c:1492-1493` — comentario *"Sólo objetos. Para arrays / strings, NOP"*. | ✅ EXACTA |
| **GC-1** | `long[]`/`double[]` (tipo 5) invisibles al GC → UAF latente | `heap.c:90` — `is_heap_ref` devuelve `type >= 0 && type <= BPVM_TYPE_OBJECT`; `BPVM_TYPE_OBJECT=4`, `BPVM_TYPE_ARRAY_I64=5` (`bpvm_internal.h:43-44`). Un tipo 5 se rechaza en el escaneo **y** en el trazado. | ✅ EXACTA |
| **GC-2 / H-001 / H-012** | `gc_mark_phase` solo escanea pilas; ni `allocAnchor` ni data blocks de módulo son raíces | `heap.c:134-146` — solo bucle sobre stacks de threads; `/* 2. allocAnchor — TODO en F2.b */`. Ninguna referencia a globales de módulo. | ✅ EXACTA |

**Conclusión:** el rediseño se apoya en observaciones **precisas y actuales** del código.
Las tres son fallos de seguridad de memoria (UAF latente / recolección prematura /
cascada determinista ausente) que el modelo de handles vuelve **imposibles por
construcción** (GC preciso por tabla, reclamación centralizada).

## Hallazgo confirmado de paso: H-010 (grave, camino `owner`)

`OP_FREE_REF` (`interp.c:1500`) y `OP_SET_FIELD_OWNER` (`interp.c:1519`) al liberar
**solo** ponen `FREE_BIT`: no escriben el tamaño en `+4` ni enlazan en la free-list.
Pero `block_total_size` (`heap.c:36-37`) para un bloque con `FREE_BIT` **lee el tamaño
de `+4`**, donde tras `FREE_REF` sigue el `class_ptr`/`length` del objeto. → el
siguiente `gc_sweep_phase` hace `cur += class_ptr` y **desincroniza el barrido del
heap**. Es decir, **una sola liberación `owner` puede corromper el recorrido del GC**.
Severidad alta y en el camino de la feature estrella (`OwnerList`). El modelo nuevo lo
elimina (no hay bloques medio-liberados; la reclamación es centralizada en el safepoint).

## Pregunta de paridad: ¿es observable el valor crudo de una referencia?

**Respuesta: NO en la salida de programa; SÍ solo en herramientas de depuración.**

- **No hay builtin de identidad/hash** de objeto en ninguna VM (los únicos `hash` del
  VM-C son tablas internas de `aot_registry`/`link`, no expuestas a BP).
- El `System.identityHashCode` del emisor (`MivmEmitter.java:2214,2254,2314,3319`) es
  **de tiempo de compilación**: acuña nombres únicos de locales sintéticos
  (`__for_end_…`, `__switch_…`). No llega al runtime. *(Nota menor: al ir en el nombre
  del local, puede hacer el `.dbg` no reproducible entre compilaciones — no afecta a la
  paridad dual-VM, que corre UN mismo `.mod`.)*
- El `@<hex>` de `ModuleManager.formatPropertyValue:821` (`case "ref"`) y el fallback de
  `VirtualMachine.readStringIfPossible:5135-5158` son **display del depurador**, no
  salida de programa.

**Consecuencia para el modelo de 64b:**
1. La **paridad byte-idéntica de programa** NO exige que la asignación de handles sea
   idéntica entre VMs (las refs son opacas para el programa). El invariante sagrado
   está a salvo con handles independientes por VM.
2. Pero el **depurador** y el **oráculo Java↔C** (H6) sí muestran/comparan refs (`@hex`
   → pasará a `@slot#gen`). Para que el oráculo no reporte falsos desajustes, lo barato
   y duradero es **mantener la asignación de slots determinista e idéntica entre VMs**
   (política de free-list igual; single-thread es determinista). Es **compatible con el
   tiering**: el handle `(slot,gen)` es independiente de dónde viva `slot.addr`.
3. Guardarraíl a futuro: si algún día se añade identidad/hash de objeto observable, que
   derive de algo estable en paridad (p. ej. orden de asignación), **no** del `addr`.

## Ficheros y líneas (todo verificado en `c2f82929`)
`interp.c:1490` (FREE_REF), `:1497` (TODO cascada), `:1506` (SET_FIELD_OWNER),
`:1519` (solo FREE_BIT) · `heap.c:36` (block_total_size lee +4), `:82-90` (is_heap_ref
0..4), `:134-146` (mark solo pilas), `:149` (add_to_free_list escribe +4/next) ·
`bpvm_internal.h:43-44` (OBJECT=4, ARRAY_I64=5) · `MivmEmitter.java:2214` (identityHashCode
compile-time) · `VirtualMachine.java:5133` (readStringIfPossible, display) ·
`ModuleManager.java:815-824` (formatPropertyValue, display del depurador).
