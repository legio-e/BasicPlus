# Plan H1.2a — ensanchar referencias 4 → 8 bytes, semántica PLANA (de-risk)

Estado: **análisis cerrado 2026-07-08** (Eduardo + Claude), pendiente de empezar código.
Fuente: inventario dual-VM de sitios de 4 bytes (miVM + bpgenvm-c, 2026-07-08) + spike H1.1
(`docs/V4_BACKLOG.md` H1.1: handles uniformes viables, deref +2–5 % en micro / ~0 % host).

---

## 1. Objetivo y contrato

Ensanchar **toda referencia** (object / array) de 4 → **8 bytes** con **semántica PLANA**:
los 8 bytes guardan la MISMA dirección (32 bajos = dirección, 32 altos = 0), el deref es
igual que hoy. **Cero tabla de handles todavía** — eso es H1.2b.

**Por qué este escalón (de-risk):** aislar la fontanería de 64 bits ANTES de la semántica.
Es la **misma clase de cambio que el `long`** (que ya nos mordió en STM32) → hacerlo solo
como ensanchado, con un invariante de aceptación fuerte, deja cualquier bug del ancho
**aislado** (no mezclado con la lógica de handles).

**Invariante de aceptación:** **TODO el suite byte-idéntico dual-VM** + estrés de GC.
8 bytes planos = coste sin beneficio → **NO se publica**; es un checkpoint interno.

---

## 2. El simplificador — las refs reutilizan los opcodes de 8 B del `long`

Las refs no son un tipo de pila aparte: son un entero-dirección que hoy viaja por los
opcodes i32 genéricos. El camino de 8 B de `long`/`double` **ya existe y ambas VMs lo
ejecutan idéntico**: `GET/SET_LOCAL_L`, `GET/SET_GLOBAL_L`, `GET/SET_FIELD_LONG`,
`ALOAD/ASTORE_I64`, `LRET`, `NEWARRAY_I64`.

⇒ **Gran parte del ensanchado es cambio de EMISOR**: que clasifique `object`/`array` como
8 B y emita el opcode `_L`/`_LONG`/`_I64` en vez del i32. La pila, locales, globales,
lectura/escritura de campos, params y returns caen casi solos por ahí (el emisor es la
fuente única de la verdad → mantiene las 2 VMs en sync). El riesgo NETO-NUEVO se concentra
en 3 zonas (§4).

---

## 3. La trampa que recorre TODOS los sitios precisos: endianness

`writeI64`/`bpvm_write_i64_be` son **big-endian**: palabra ALTA en `+0`, palabra BAJA en
`+4`. Una ref plana = (alto=0, bajo=dirección) → la **dirección vive en `+4`**; en `+0` hay
un cero.

**Regla de oro:** en cada **traza/lectura PRECISA** de una ref (campo por bitmap, elemento
de array de ref), leer **`i64` y quedarse con los 32 bajos** — NUNCA `i32` en la base (eso
lee el cero → toda ref parece `null` → no se traza → se liberan objetos vivos = **UAF**).

**Acotación clave:** el **scan CONSERVADOR** de pilas y data-blocks (`scanRegion` /
root-scan, paso de 4 B) **NO se toca** — la palabra baja (dirección) está 4-alineada, así
que la encuentra igual (solo escanea de más el cero, inofensivo). Solo los **trazados
PRECISOS** necesitan el arreglo endianness-aware. Eso deja el peligro en ~4 sitios/VM (§7).

---

## 4. Las 3 zonas de riesgo neto-nuevo

Todo lo demás es fontanería de emisor (§2). El trabajo de verdad:

1. **GC — trazado preciso (EL gordo).** `GET_FIELD_LONG` ya lee un campo de 8 B, pero el GC
   hoy **no traza** campos de 8 B (son `long`). Hay que (a) marcar el campo ref en el bitmap
   y (b) que el GC lo **siga** (i64 + low32). Igual para arrays de ref (`ARRAY_REF` stride
   4→8 + trazado, sin colarse por el camino NO-trazado de `ARRAY_I64`).
2. **Offset del receptor en `INVOKE_VIRTUAL`.** El `sp-4-num_args*4` codifica "receptor de
   4 B" → pasa a `sp-8`. Sutil porque los args ya van contados por slots. Se repite en el
   puente native (`bridge_run_bp_frame`, `call_method_i32 buf[0]`) y en `threading.c` al
   empujar `this`.
3. **ABI native — SOLO VM-C** (miVM es Java puro, no tiene). Ver §6.

### Codificación del campo ref (idéntica byte-a-byte en las 2 VMs)
- ref = **2 slots** en `num_fields` (como `is8` del long) + **bit de traza en el slot BASE**.
- GC: lee `i64` en la base, toma low-32, sigue esa dirección; el slot+1 queda a 0 (el bucle
  del bitmap lo salta solo, bit clear).
- `block_total_size` ya sale bien (`num_fields*4` cuenta los 2 slots = 8 bytes).
- Distinción limpia de los 3 tipos: **ref-8B** = 2 slots + bit · **long/double-8B** = 2 slots
  + sin bit · **int-4B** = 1 slot + sin bit.

---

## 5. Superficie de cambio por VM (con el `long` como plantilla)

### miVM (`edu.bpgenvm`, `VirtualMachine.java`=VM, `ModWriter.java`=MW)
| zona | sitios 4B → 8B | plantilla long |
|---|---|---|
| Pila/locales/globales | GET/SET_LOCAL VM:1985/1990, GET/SET_GLOBAL VM:1916/1921 | *_L VM:2669–2687 |
| Campos | GET/SET_FIELD VM:2503/2509 | *_FIELD_LONG VM:2516–2526 |
| Arrays ref | ALOAD/ASTORE VM:2161/2174, `elemSize` TYPE_ARRAY_REF VM:1018 (4→8), alloc VM:3209 | *_I64 VM:2695/2710/2722 |
| NEW_OBJECT / retornos / THROW / INSTANCEOF / FREE_REF / SET_FIELD_OWNER | VM:2496/1955/2915/2969/2949/2956 | LRET VM:1971 |
| INVOKE_VIRTUAL receptor | VM:3064 `sp-4-numArgs*4` → sp-8 | — |
| **GC preciso** | markObject campos VM:1238–1243, array VM:1227–1229; owner-bitmap VM:4834 | — |
| Data block globales | declareGlobal MW:421 (→ byte[8]/path long), registrar 8 B + emitir *_GLOBAL_L | declareGlobalLong MW:428 |
| Emisor (clasificar ancho) | emitGetLocal MW:1310, emitGetField MW:1061, emitGetGlobal MW:1355, declareLocal MW:603, campo bitmap MW:988–990 + slot-advance MW:826 | is8 / sizeBytes==8 |
| Wire/depurador | .dbg sizeBytes MW:603→8 + tag "ref"; ramas sizeBytes==8 ya existen (DS:683, DFR:66, MM:807) | — |

### bpgenvm-c (`src/`, `interp.c`/`heap.c`/…)
| zona | sitios 4B → 8B | plantilla long |
|---|---|---|
| Pila/locales/globales | GET/SET_LOCAL interp.c:674/679, _S8 683–693, GET/SET_GLOBAL 704–714 | *_L 1092–1111 |
| Campos | GET/SET_FIELD interp.c:1401/1410 | *_FIELD_LONG 1415–1430 |
| Arrays ref | ALOAD/ASTORE 1264/1275; `block_total_size` ARRAY_REF heap.c:48 (×4→×8); `BUILTIN_NEW_REF_ARRAY` builtins.c:1141 | *_I64 1122–1139; ARRAY_I64 heap.c:47 |
| NEW_OBJECT/RET/THROW/INSTANCEOF/FREE_REF/SET_FIELD_OWNER | interp.c:1388/824/1622/1488/1506/1526 | LRET 837–851 |
| INVOKE_VIRTUAL receptor | interp.c:1435 `sp-4-num_args*4` → sp-8 | — |
| **GC preciso** | mark_recursive campos heap.c:172–177, array heap.c:153–157; block_total_size heap.c:33–64 | — |
| Excepciones (fuera del opcode) | exceptions.c:76 push ref 4B; :87 lee ref-campo `ref+4` 4B; :159 escribe ref-campo | — |
| Threading | threading.c:179 empuja `this` 4B | — |
| **ABI native** | ver §6 | — |
| Loader/link | **limpio** — solo direcciones de código/descriptor (32-bit bajo plano); data block es blob opaco (loader.c:156) | — |

---

## 6. AOT / native — plano en H1.2a, resolve-en-borde en H1.2b

**Hoy:** native trabaja con **memoria plana** — recibe una ref = offset crudo en `mem[]` y
hace `mem + ref + off` (`bpvm_aot_helpers.c` h_array_load_i32, etc.).

**H1.2a: sigue plano.** La ref es una dirección en 8 B (low-32); native extrae los 32 bajos
y hace la MISMA aritmética. Lo único de H1.2a en la parte native = **ensanchar el
marshalling del borde** (puente `bridge_run_bp_frame` interp.c:145/171, `bpvm_aot_helpers`,
y los **thunks generados** en el frontend/.mdn: empujar/leer refs como 8 B). Dentro del
kernel native, **nada**.

**H1.2b (handles, NO ahora):** la ref pasa a `(índice, generación)` → native ya no puede
`mem + ref + off`. Diseño (ya en la memoria [[v4-modelo-memoria-handles]]): **resolver el
handle → dirección cruda UNA vez en el borde + pin**, y dentro del kernel trabajar con la
dirección cruda (plano, coste por-acceso 0 → es el ~0 % native del spike). Matiz de
seguridad GC: una dirección cruda en manos de native es **invisible al GC**; si ese native
**aloca** o llama de vuelta a BP (`call_bp`, que podría alocar) → **pin** (o re-resolver tras
cada punto de GC). Native puro (solo cómputo) no dispara GC → sin pin, gratis.

⇒ **En H1.2a NO se toca la lógica native, solo el ancho del marshalling.** El resolve-en-borde
es trabajo de H1.2b.

---

## 7. Los 4 sitios donde se esconderá el bug (revisar con lupa + test dirigido)
1. **GC campo-ref** — `markObject`/`mark_recursive` (VM:1238 / heap.c:172) + builder del
   bitmap (MW:988 / frontend). La trampa endianness (§3): leer i64+low32, no i32-en-base;
   gemelo en el owner-bitmap.
2. **GC array-ref** — VM:1227 / heap.c:153. Stride 8 + low32, sin colarse por ARRAY_I64
   (no-trazado).
3. **`block_total_size` / tamaños** — heap.c:33–64 + alloc. Si el ancho no sube en lockstep
   con la alocación, el barrido del heap se **desincroniza** (corrupción global).
4. **Offset del receptor INVOKE_VIRTUAL** — VM:3064 / interp.c:1435, + gemelos en el puente
   native y threading.c.

---

## 8. Orden de ataque (rungs, cada uno con test dual-VM dirigido)

1. **Modelo de ancho en el emisor** — `object`/`array` = 8 B en
   locales/globales/campos/params/returns/elems; emite `_L`/`_LONG`/`_I64`; cuenta 2 slots;
   pone el bit de traza del campo ref.
2. **GC preciso, las 2 VMs** — mark de campo-ref y array-ref (i64+low32, stride 8, distinción
   de traza) + `block_total_size` ARRAY_REF×8 + offset del receptor a `sp-8`.
   *(1+2 = primer rung coherente testeable: hasta que ambos caigan, el GC malinterpreta.)*
3. **ABI native (VM-C)** — puente + helpers + thunks: ensanchar el marshalling (dentro,
   plano; §6).
4. **Wire/depurador** — emitir `sizeBytes=8` + tag `"ref"` (casi mecánico; ramas
   `sizeBytes==8` ya existen).
5. **Puerta de aceptación** — **suite entera byte-idéntica dual-VM** + programa de **estrés de
   GC** que fuerce recolección con campos/arrays de ref y verifique que no se libera nada
   vivo. Ese programa es la **lista enlazada que colgaba en placa** → cierre del círculo (si
   el ensanchado está bien, pasa a ir fino / fallar-gritando, no a colgarse mudo).

---

## 9. Diferido a H1.2b (no lo dispara el ensanchado)
- **Array LOCAL de tamaño fijo pasado por-ref.** En H1.2a se sigue pasando su **dirección
  cruda** (ensanchada a 8 B, alto=0) → sin problema. En H1.2b, "una dirección de pila **no es
  un handle**" (propuesta §9 decisión 5) → problema abierto ahí.
- **Semántica de handle + barrera A1** (H1.2b).
- **Coste de la barrera de escritura A1 + alta de handle en la alocación** — más raras que el
  deref; se miden al implementar H1.2b (el spike solo midió el deref).
