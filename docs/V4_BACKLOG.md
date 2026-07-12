# BasicPlus — Backlog de V4

> **Fuente única del backlog de V4.** V4 = **consolidar + mejorar** lo que ya hay
> (un poco como fue V2), tocando lo **delicado** —módulos (slots/vtable) y GC— con
> **mucho cuidado y muchas pruebas**. NO se reestructura en V3; lo delicado se
> aparca aquí. Visión/decisiones de fase: memoria `v4-consolidacion-v3-bugs-obvios`.
>
> Estado: `pendiente` / `en curso` / `cerrado`. Convención: B=bug · L=limitación ·
> N=hallazgo · M=mejora · #NNN = id de task histórica.

---

## 📋 ÍNDICE DE V4 (Eduardo, 3-jul-2026) — los 10 temas

El índice que manda; el resto del documento es el detalle que va colgando de él.

1. **SD** — lectura de tarjeta SD (almacenamiento masivo removible). *(Detalle abajo, junto a Pack.)*
2. **Pack** — XIP de bytecode: código en flash sin copiar a RAM; stdlib como pack →
   actualizar `Gui` sin reflashear. *(Diseño detallado en `V3_IDEAS.md` §Packs, charla 2-jul.)*
3. **Pack manager** — **pantalla nueva del IDE** (formulación Eduardo 3-jul): dos lados, como el
   explorer pero de packs. Lado PC = **carpeta de packs** (biblioteca local): construir packs nuevos,
   borrar antiguos. Lado micro = ver los packs del device y borrar. El puente = **escoger cuáles
   subir y subirlos** (Burn por el wire). Pide un comando wire de inventario de packs (hermano del
   LS). *(Detalle en `V3_IDEAS.md` §Packs → "Pack manager".)*
4. **Sobrecarga de funciones** — overloading en el lenguaje (misma función, firmas distintas).
   *(Nuevo; tanda de lenguaje. Toca frontend + mangling de nombres en .mod/.bpi — hoy ya existe
   `nombre#arity` en los símbolos, base a estudiar.)*
5. **Funciones nativas RISC-V** — AOT/`native` en el ESP32-P4 (port del emisor/loader `.mdn` a
   RISC-V; hoy `native` cae a interpretado en ESP32). *(La placa gráfica insignia lo merece.)*
6. **Heap** — el frente GC/memoria: `B-gc-allocanchor` + `B-freeref-no-recursivo` (abajo) +
   fragmentación/rendimiento (las herramientas H3 ya existen) + layout compacto de narrow types.
7. **Módulo + mdi** — **el contenido del fichero de interfaz (hoy `.bpi`) TAMBIÉN dentro del `.mod`**
   (formulación Eduardo 3-jul): módulo AUTODESCRIPTIVO. El `.mod` crece un poco ("asumible" — la
   interfaz son nombres+firmas; y con Packs/XIP se ahorra RAM, "en conjunto salimos ganando").
   Beneficios que caen solos: (a) compilar contra un `.mod` sin su `.bpi`; (b) el DEVICE puede
   resolver nombre→slot en carga = **el Camino B de forms** (nº 10) sale de aquí; (c) el Pack
   manager puede inspeccionar qué exporta cada módulo de un pack; (d) **mata la familia de bugs de
   DESFASE `.bpi`/`.mod`/`.slots`** (Json rancio, slots de Gui — un artefacto, una verdad); el
   sidecar `.slots` de H13.1 queda subsumido.
8. **2 Núcleos** — dual-core: RP2350 (#153, incluye el fix de la race B1) y el P4 (2× RISC-V 360 MHz);
   SMP real en device (hoy 1 worker).
9. **Revisar IDE** — pase de consolidación (incluye multiplataforma jSerialComm, breadcrumb, y lo
   que deje la lista de V3).
10. **Revisar GUI** — paraguas gráfico: preview de forms en miVM/Swing, Camino B (nombre→slot en
    .mod), campos tipados de widgets (diseño Swing/NetBeans, 28-jun), repesca de widgets simples,
    bug estado-GUI-entre-runs, board-aware data-driven (params de panel en board.json), rotación
    LTDC, PPA del giro, retirar `esp32p4-ws/` de referencia.

---

## 🏗️ H1 — Modelo de memoria por handles  *(hito ACTIVO de V4, arrancado 8-jul-2026)*

La **columna de V4**: refs planas → **handle (índice + generación, 64 b decidido)**, contrato
**fail-fast** (deref de objeto liberado → RuntimeError, nunca UAF mudo). Vuelve imposibles por
construcción la familia de bugs de v3.0.1 (UAF, punteros crudos, alineación). Método: **paso a
paso, verificando cada escalón contra la paridad dual-VM**. Análisis en
`bp_propuesta_modelo_memoria/` (spec + verificación + ADR-0001). Los otros 9 temas del índice van
cogiendo H2, H3… según se secuencien.

- **H1.1 — Spike de coste de indirección. ✅ HECHO (8-jul) → handles UNIFORMES viables.**
  Experimento **desechable** (ya revertido: macro `HANDLE_DEREF` + tabla proxy en `interp.c`, toggle
  guardado `-DBPVM_HANDLE_SPIKE` en `pico/CMakeLists.txt`). El proxy = 1 carga a tabla + 1 check de
  generación devolviendo la **misma dirección** → paridad byte-idéntica en las 12 corridas
  (`chk`=504469776/11940000/-455008256 siempre). Kernels: campo (ref cte), recorrido de lista (ref
  variable), array. Medido en **host VM-C + Pico (heap SRAM) + Metro (heap PSRAM, tabla SRAM)**:
  **Host x86 (OoO+caché) ~0 %** (dentro de ruido) · **Pico Cortex-M33 +2–5 %** (field +4.7, walk
  +3.1, array +1.9) · **Metro +2–4 %** (field +4.2, walk +2.4, array +1.9). **La PSRAM NO lo
  empeora** (tabla en SRAM; la lentitud de PSRAM la pagan OFF y ON por igual en las lecturas del
  objeto). Confirma la hipótesis: el micro en-orden sin caché no esconde la carga extra → ~3 %; en
  `native` se resuelve-1-vez-y-pin → ~0. **Veredicto: uniforme viable — no hace falta híbrido para
  el intérprete.** Falta medir aparte (dentro de H1.2b): barrera de escritura A1 + alta de handle en
  la alocación (mucho más raras que el deref). Datos/kernels: `C:/tmp/spike_device/`.
- **Lead (8-jul, del spike) — cuelgue mudo en placa al construir estructuras medianas.** Construir
  una lista enlazada de ~1000 nodos (~12 KB) **cuelga MUDO** en Pico y Metro; a 200 nodos va. En
  **host corre hasta con `--mem=64 KB`** → NO reproduce en host = **específico del device**. Acotado:
  NO es el stack de operandos (`CallLoop` 3M llamadas void y `VarLoop` 3M var-en-bucle completan con
  128 KB en host) → apunta al **GC/heap del micro**. Caracterizar con la placa delante (empezar por
  el `mem` del device para ver el heap real). Candidato a que el contrato **fail-fast** de handles lo
  saque a la luz (gritar) en vez de colgar en silencio.
  - **Requisito (Eduardo 8-jul):** la función de **alocación del heap** (`heap.c` + su gemela en miVM)
    debe, al quedarse sin espacio tras el GC, **saltar un `RuntimeError` atrapable — "No space in
    heap"** (o similar) — y **NUNCA colgarse** (hoy, sospecha: bucle GC-sin-progreso cuando el GC no
    libera nada porque todo está vivo). Fail-fast **como norma ya**, no hace falta esperar a los
    handles para esto; el mensaje con **paridad dual-VM**. Es un arreglo contenido y de robustez.
    - **✅ HECHO (11-jul, commit `bbea3d7`).** Descartada la sospecha del bucle: `bpvm_heap_alloc` es
      **GC-once-then-fail** (colecta 1 vez y `return 0`, sin bucle). El "cuelgue MUDO" del device era
      el `0` propagándose como `BPVM_ERR_OOM` → **exit-4 invisible en placa** (en host imprime status;
      en el firmware no se surfaceaba). Ahora los 11 sitios de alocación (5 en `interp.c` vía
      `BPVM_RT_THROW`, 6 en `builtins.c` vía `builtin_throw`) + `heapAlloc` de miVM (guarda
      `throwingOom` anti-recursión + fallback `BpThreadFault`) lanzan un **RuntimeError BP atrapable
      "No space in heap"**. La excepción PEQUEÑA cabe en el hueco de bump que deja el alloc GRANDE que
      falló; si ni eso cabe → salida limpia (`BPVM_ERR_RUNTIME` / `BpThreadFault`), nunca hang. Test
      permanente `samples/OomCatch.bp` + `make test-oom`; **paridad dual-VM byte-idéntica** ("cazado
      OOM: No space in heap" / "vivo tras OOM" / exit 0); JUnit 34/34. **Pendiente menor (con la placa
      delante, diferido con el resto de trabajo de device):** confirmar que el caso de ~1000 nodos ya
      muestra el error en vez de aparentar colgarse — muy probable, pero sin verificar en HW.
- **H1.2 — Migración a handles.** En **dos escalones** (idea de Eduardo 8-jul: aislar la fontanería
  de 64 b antes de la semántica, porque es la **misma clase que el `long`** y ya nos mordió en STM32):
  - **H1.2a — Refs a 8 bytes, semántica PLANA** (prevención/de-risk). Ensanchar toda ref 4→8 B
    **reutilizando el carril de 8 bytes de `long`/`double`** (H1.2 de V3, ya probado); los 8 B
    guardan la misma dirección (32 bajos = dir, 32 altos = 0), deref igual que hoy. **Verificación:
    TODO el suite byte-idéntico dual-VM + shakedown** → cualquier divergencia = bug del ensanchado,
    aislado. Riesgo real: **GC** (marcar/trazar ref de 8 B) + tamaño de elemento de **arrays de ref**
    (4→8) y **campos ref** (→ 2 slots + bitmap). **NO se publica** — es un checkpoint interno (8 B
    planos = coste sin beneficio; es el escalón). **Plan detallado:**
    `bp_propuesta_modelo_memoria/03-arquitectura/plan-h1.2a-ensanchado-refs.md` (análisis cerrado
    8-jul: inventario dual-VM de sitios 4B + orden de ataque + crux del bitmap/GC con la trampa
    big-endian).
  - **H1.2b — Semántica de handle.** Los mismos 8 B pasan a `(índice, generación)`; deref → lookup
    en tabla de slots `{addr, gen, flags}` + check de generación; contrato B (fail-fast) + **barrera
    A1** en el punto único de publicación (TSan en host SMP + litmus en device 2-core). Cambio
    pequeño y localizado, sobre fontanería ya probada.
- **Diferidos dentro de H1:** compactación (cota de pausa STW, H-004) + arrays fijos locales pasados
  por-ref (sin entrada en la tabla — ver propuesta §9 decisión 5 / [[v4-modelo-memoria-handles]]).
- **PENDIENTE — TANDA 2 de pops de ref en builtins.c (regresión H1.2a, parcialmente arreglada).**
  La suite BP de IO (H2·B1.2, 12-jul) destapó que ~45 builtins de la VM-C popean refs con
  `pop_i32` (4B) cuando la pila de operandos los apila a 8B → multi-arg corruptos + 4B de basura
  por llamada que corrompe EXPRESIONES (`IO.fileSize(p) + 1`). miVM NO afectada (su pila usa refs
  4B — convención interna distinta). **Tanda 1 HECHA (`c2fe54d`): familia FS + THROW_RTE (15
  sitios → `pop_ref`).** **QUEDAN (inventario 12-jul, líneas pre-fix):** GUI string/objeto
  (set_text 597, load_font 616, invokeByName 699-701 ¡3 refs!, invokeBySlot 722-724, bindClick
  750/757, set_options 789, set_buttons 805, tabview_add_tab 811, table_set_cell 821,
  image_load_file 834), Net (connect 533, send 544, recv 597-área), string↔bytes (1210 «pop 4B
  preservado (inconsistencia pre-existente)», 1239, 1499/1500), Thread spawn/join (1306/1318),
  Mutex lock/unlock (1347/1379), HW dataRef (SPI/I2C/UART 1459-1623, NeoPixel 1792). Método: cada
  sitio cross-checkeado contra la VM-Java (orden top-first + ancho por tipo BP: string/array/
  objeto=8B ref, int/handle/slot=4B). OJO: los sitios de 1 ref "funcionan" de chiripa hoy — el
  riesgo es en expresiones y multi-arg. **Candidato estrella: Thread spawn/join popeando a 4B
  puede ser el bug diferido "owner-alloc en Thread.run() peta" de H1** — verificar al arreglarlo.
  Verificación de la tanda: paridad dual-VM de samples GUI (dumpTree), threads, Mutex, HW-host.
- **Diferido — AOT/native no es handle-aware (regresión conocida, NO bloquea).** El puente
  `native→BP` (`aot_helpers.c` / `aot_call_bp` / `call_method`) pasa **direcciones planas** mientras
  la VM interpretada ya usa **handles** → los tests `make test-throwmsg` (SIGSEGV) y
  `make test-throwuser` (use-after-free en `call_bp`) están **ROJOS** desde la migración de handles.
  Es exactamente el pendiente ya registrado *"aot_helpers.c handle-aware (refs en AOT, solo .mdn)"*.
  **NO afecta a la ruta interpretada** (lo que se despacha por defecto: 1 worker, interpretado); la
  suite interpretada (JUnit 34/34 + paridad dual-VM + `test-smphandles` + `test-oom`) está VERDE. Se
  aborda al migrar el AOT a handles.

---

## 🔴 Bugs delicados (movidos de `PENDIENTES.md`, 27-jun)

Bugs que exigen tocar maquinaria delicada (slots/vtable, GC, o el FS del firmware) → se
hacen en V4 con red de pruebas (no son fixes contenidos de V3).

### B-174b — slot de vtable divergente al añadir métodos a clase base con subclases  ⭐ (el que más desbloquea)
Añadir métodos a una clase que tiene subclases desplaza su vtable y `ClassSymbol.ensureMethodSlots`
calcula slots distintos en el frontend y en el `ModWriter`. Síntomas: en `Component` (Gui) da
**error del emisor** al compilar ("slot divergente para X.setChecked frontend=37 ModWriter=29"); en
`Window` **no** da error (no hay subclase suya en el mismo módulo) pero en **runtime el VM-C despacha
al slot equivocado y CUELGA** (miVM lo resuelve bien). **Desbloquea:** `Gui.Window.find(name)`, en
general extender clases base con subclases, **y AOT cross-módulo #169** (mismo `slotOf`). El propio
compilador señala el sitio: `ClassSymbol.ensureMethodSlots`. Encontrado 28-jun (intento de `find()`
revertido; queda `Component.name`, commit `6f711c1`). **Es el cimiento compartido de Forms-find +
AOT-cross-module + extensión de clases base** → prioritario en V4.

### B-gc-allocanchor — el GC no escanea la raíz `allocAnchor` (F2.b diferido)
`gc_mark_phase` (`bpgenvm-c/src/heap.c:145`) tiene `/* allocAnchor — TODO en F2.b cuando se añada el campo
al thread */`: el GC mark-sweep no recorre esa raíz (el campo no existe aún en `bpvm_thread_t`). Riesgo
LATENTE: un objeto recién alocado y aún no guardado en stack/global podría recolectarse a mitad bajo presión
de memoria/GC. No ha mordido (workloads reales OK), pero es un agujero de corrección del GC. Hallado 28-jun.
**→ MOVIDO a v3.0.1** (es parte del fix GC-2; seguimiento en `V3_BACKLOG.md` §v3.0.1).

### B-freeref-no-recursivo — `OP_FREE_REF` no libera en cascada los campos `owner` (F3 v1)
`interp.c:1498`: `FREE_REF` sólo libera el objeto raíz; el TODO pide recorrer el `owner_bitmap` y liberar
recursivamente los campos `owner`. Un árbol de objetos con dueños no se libera en cascada → **fuga hasta que
el GC mark-sweep lo recoja** (no permanente, pero las owner-semantics prometen free determinista). Hallado
28-jun (relacionado con L7).

### B-fs-pico-hang — cuelgue mudo del pico con `/app` lleno de módulos (batch 4-jul)
Correr una demo (p.ej. `NeoDemo`) con `/app` MUY lleno de `.mod` colgó la Metro (hubo que resetear, sin
mensaje). NO es tope limpio del FS (`fs_put` ya devuelve `NO_SPACE`/`TABLE_FULL`) → overflow/loop/corrupción
en el `fs_put`/`compact` del pico. **LOCALIZADO al firmware pico**: la DK2/stm32 con el FS lleno da error
limpio `NO_SPACE`, NO cuelga → no es el núcleo. Riesgo: es código compartido del FS (`s_data` + cargador),
podría morder en cualquier placa con `/app` a tope. **Método (Eduardo):** reproducir EN FRÍO primero (subir
`.mod` 1 a 1 sin resetear + correr la demo entre medias → nº y punto exactos), localizar, y SOLO ENTONCES
tocar (territorio delicado). Documentado como limitación conocida en la release v3.0.

### N-dtblock-align — layout del data block: variables a 4 vs constantes empaquetadas (raíz de GC-2)
**✅ RESUELTO en v3.0.1 (7-jul):** `registerSymbol` alinea **cada símbolo** a múltiplo de 4
(`slot=(len+3)&~3`) → cada global 4-alineado, el bloque auto-alineado. Cierra GC-2 del todo
(`GcMisalign2` 3437→320, 21/21 paridad). Los objetos ya eran seguros (campos word-slotted +
el compilador prohíbe arrays fijos como campo de clase). Lo de abajo queda como **contexto
histórico** de por qué el data block se empaqueta; para V4 (modelo de handles) el layout se
revisará igualmente:
Al arreglar **GC-2** (v3.0.1) salió que el bloque const+globales **no queda alineado a 4 por
construcción**, y Eduardo pidió registrar el porqué. Causa (verificada en `ModWriter.registerSymbol`,
cada símbolo ocupa su `bytes.length` **crudo**): las **variables** escalares normales y los arrays
numéricos SÍ son múltiplo de 4 (`integer`/`float`=4, `long`/`double`=8, `int[]`/`float[]`=`4+n·4`,
`long[]`=`4+n·8`), pero las **constantes empaquetadas** no: un **`string`** es `4+N` (N=bytes UTF-8,
arbitrario ⇒ el culpable más común), un `int8`/`byte` const **1** byte, un `int16`/`short` **2**, un
`byte[]`/`int16[]` const `4+N`/`4+2n`. Demostrado: un módulo con solo `var g:integer` da bloque de 380
(múltiplo de 4); añadir `const MSG:string:="abc"` (7 B) lo lleva a 387 → padding a 388. **v3.0.1 parchea
el síntoma** rellenando `dataSize` al múltiplo de 4 por el extremo bajo (documentado en `MOD_FORMAT.md`
§5). **Para V4 (revisar, ligado al modelo de handles que toca el codegen de `.mod`):** decidir si la
alineación se hace **intrínseca** —cada entrada alineada por construcción— en vez de un padding global a
posteriori; y **resolver la inconsistencia** que señala Eduardo: hoy conviven variables acolchadas a 4
(un `byte` var podría estar ocupando 4 B — **confirmar**, el emisor vive fuera de `miVM/generador`) con
constantes empaquetadas byte a byte. Es cuestión de coherencia y de que la alineación deje de ser un caso
a recordar. (El acceso a `integer` no alineado, además, **falla en ARM/RISC-V** — otra razón para hacerlo
por construcción.)

---

## 🧭 Temas de V4 (índice; el detalle de diseño vive en `V3_BACKLOG.md` §"V4 — fuera de V3")

**Tema general: consolidar + mejorar rendimiento** (como V2), además de lo diferido:

- **AOT cross-module #169** — sin puente del intérprete (hoy funciona vía `call_bp`+warning). Se
  apoya en el MISMO `slotOf` que B-174b. Diseño en `AOT_CROSS_MODULE.md`.
- **AOT — casts** (`byte()/int()/float()/long()/double()`) en `AotCEmitter` (hoy cae a interpretado).
  Bloquea `compress` native. Mismo paraguas que `^` en native.
- **Forms — diseñador visual drag&drop** + **preview de forms en miVM/Swing** (el cargador es BP y
  miVM ya pinta en Swing → "preview" ≈ correr el form en miVM dentro del IDE).
- **Forms — Camino B**: tabla `nombre→slot` en el descriptor de clase del `.mod` (resolver en el
  device en carga; editar/enviar pantallas sin IDE). Re-baselinea el compat de emisión.
- **PACK = XIP de bytecode** (código en flash, no en RAM; stdlib como pack → actualizar `Gui` sin
  reflashear) + **lectura de SD** (almacenamiento masivo removible).
- **#153 — Dual-core RP2350** (incluye el fix de **B1**, la race multi-worker; hoy mitigada a 1 worker).
- **Net.Listener / servidor TCP** (`listen`/`accept`) sobre la Ethernet del P4.
- **AOT en ESP32** (Xtensa/RISC-V — port del loader `.mdn`).
- **Neopixel en ESP32/P4** — backend WS2812 vía **RMT** (componente `led_strip` o encoder RMT propio).
  Hoy STUB en el ESP32 (el Pico ✅ lo hace vía PIO, #227/#228). Más plumbing que los demás periféricos
  (encoder + timing + dependencia de componente) → diferido por Eduardo (27-jun): "no es crítico ni urgente".
- **Rollout de gráficos a más kits** (solo equipos con recursos de sobra).
- **IDE multiplataforma** (`purejavacomm → jSerialComm`, lanzador `.sh`).
- **Strings multilínea + interpolación** (tanda de lenguaje).
- **Valores-función / closures** (tanda de lenguaje; charla 20-jun + 4-jul): hoy NO se pueden pasar
  métodos como parámetros (modismos: override virtual u objeto-Runnable). Habilitaría el
  `async(…)`/`invokeLater(…)` ergonómico del GUI (ver nota "GUI-blocking-from-event" en V3_BACKLOG).
  Interactúa con la sobrecarga (nº 4 del índice): una referencia a método debe elegir firma.
- **`deflate`-lite** (LZSS → +Huffman) · **multi-fichero/Archive**.
- **Layout compacto de narrow** (`byte[]`/`int16[]` con storage real; hoy i32).

## 🟢 Mejoras menores (de `PENDIENTES.md`, candidatas a V4)
- **L-list-stm32-trunc — `LIST` truncado a ~14 entradas en STM32** (batch 4-jul): `handle_list`
  (`stm32_repl.c:~112`) arma la respuesta en un buffer fijo de 1024 B y corta al no caber → el explorer del
  IDE ve ~14 ficheros aunque haya más ("efecto ventana"). El pico streamea con `fputs` y no lo sufre. Fix:
  paginar o streamear la respuesta del LIST en el stm32. Cosmético (no pierde datos, solo el listado).
- **M6 — `const := Color.RED`**: inlinar el valor de enum (conocido en compilación) desde
  `EnumSymbol.values` en vez de dar "requiere literal". (Sale de N17, ya resuelto.)
