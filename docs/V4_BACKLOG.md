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

> **Reorganización 13-jul-2026 (Eduardo):** **SD y el FS de la SD (FAT32/…) salen de V4 → V5.**
> **H3 = Packs** SIN la parte del IDE. Ver §H3 abajo para el scope.

1. ~~**SD**~~ → **MOVIDO A V5** (lectura de tarjeta SD + su FS FAT32). Fuera de V4. (13-jul)
2. **H3 — Packs** — XIP de bytecode: `.mod`+resources en flash sin copiar a RAM; actualizar la
   stdlib (p.ej. `Gui`) sin reflashear. **SIN la parte del IDE.** Scope detallado en **§H3** abajo.
   *(Diseño: `V3_IDEAS.md` §Packs, charla 2-jul.)*
3. **Pack manager (pantalla del IDE)** → **H8.a** (fuera de H3; la pantalla vive en el hito del IDE,
   apoyándose en la infraestructura de packs de H3). Dos lados como el explorer: biblioteca PC ↔ packs
   del micro, Burn por el wire. Formulación en `V3_IDEAS.md` §Packs → "Pack manager".
4. **H5 — Compilador** — mejora principal **sobrecarga de funciones** (overloading) + **`import
   pack:modulo`** + **llamadas asíncronas sencillas** (eventos LVGL). **Acoplado a H6.** Ver **§H5**.
5. **H4 — AOT** (todo el AOT bajo un paraguas) — **novedad: compilar `native` a RISC-V** (ESP32-P4) +
   todos los bugs/huecos de AOT (handle-aware, cross-module #169, casts, try/catch en native…).
   Ver **§H4** abajo. *(La placa gráfica insignia lo merece.)*
6. **Heap** — el frente GC/memoria: `B-gc-allocanchor` + `B-freeref-no-recursivo` (abajo) +
   fragmentación/rendimiento (las herramientas H3 ya existen) + layout compacto de narrow types.
7. **H6 — Formato de módulos** — **módulo AUTODESCRIPTIVO** (la interfaz `.bpi` DENTRO del `.mod`) +
   **encoding de firmas para el overloading** (H5.a) + resolución `import pack:modulo` (H5.b). **Acoplado
   a H5.** Mata la familia de bugs de desfase `.bpi`/`.mod`/`.slots`; habilita el Camino B de forms. Ver **§H6**.
8. ~~**2 Núcleos** (dual-core / SMP real en device)~~ → **APLAZADO INDEFINIDAMENTE** (Eduardo 13-jul):
   con los problemas de memoria en curso NO se aborda; se retoma **cuando todo esté consolidado**. Hoy
   1 worker (el fix de la race B1 y el P4 2×RISC-V esperan con él).
9. **H8 — IDE** — novedad: **ventana de gestión de Packs** (construir/grabar/ver) + botón
   **"Previsualizar ventana"** (form en miVM/Swing). Sin diseñador drag&drop todavía. Ver **§H8**.
10. **H7 — GUI** — modesto: **algún widget más** (repesca de simples). El resto del viejo paraguas
    "Revisar GUI" repartido: preview de forms → **H8.b**, Camino B → **H6.a**, y los ítems de
    gráficos-device por reubicar. Ver **§H7**.

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
- **✅ TANDA 2 HECHA (13-jul, `7270e40`) — y RESUELVE el bug diferido de Thread de H1.** 34 sitios
  más de VM-C a `pop_ref` + deref de handle antes de `memory[]` (ref_addr) en TCP_SEND,
  resolve_handler, vtable de invokeBySlot, Thread, Mutex, HW ×8, NeoPixel y spawn (threading.c);
  y el MISMO bug sabor Java en miVM (THREAD_START escribía el tid en la dirección CRUDA del
  handle; JOIN + MUTEX ×2 → refDeref). El «owner-alloc en Thread.run() peta» de H1 ERA ESTO.
  Sample permanente `samples/ThreadsMin.bp` (2 threads + Mutex compartido contendido → total=1000,
  paridad dual-VM). Verificación: 9/9 paridad, test-net PASS end-to-end (ojo: TcpEchoTest.mod
  RANCIO del 12-jun crasheaba ambas VMs — regenerado; la trampa de blobs de siempre), IoLfs
  3-bandas, GuiLblMin PAR, JUnit 34/34. **Queda para el batch de placa:** verificar los HW
  dataRefs en device (host = backends "no soportado").
- ~~PENDIENTE — TANDA 2 de pops de ref en builtins.c (regresión H1.2a, parcialmente arreglada).~~
  **(HECHA — ver arriba.)**
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
- **Diferido — AOT/native no es handle-aware (regresión, NO bloquea la ruta interpretada).** →
  **movido al hito H4** (§H4.b). Resumen: el puente `native→BP` pasa direcciones planas →
  `make test-throwmsg`/`test-throwuser` ROJOS desde la migración de handles. La suite interpretada
  (default 1 worker: JUnit 34/34 + paridad dual-VM + `test-smphandles` + `test-oom`) está VERDE.

---

## 📦 H3 — Packs  *(hito de V4; scope fijado por Eduardo 13-jul)*

Un **pack** = zona de flash contigua (FUERA del littlefs) con `.mod` + resources, ejecutable por
**XIP** (bytecode en flash, sin copiar a RAM). Objetivo: **una imagen solo-VM** + añadidos como packs
opcionales → actualizar la stdlib (p.ej. `Gui`) sin reflashear. Diseño: `V3_IDEAS.md` §Packs (charla
2-jul). **Fuera de V4 → V5** ([[v5-packs-sqlite-sd]]): código NATIVO en packs, SQLite, **SD y su FS
(FAT32/…)**. **Fuera de H3:** la pantalla Pack manager del IDE → **H8.a** (hito del IDE).

> **📄 SPEC IMPLEMENTABLE (leer al arrancar H3):** `C:\lenguajes\bp-analisis\temas\packs\especificacion\ESPECIFICACION-PACKS-V4.md`
> — consolidada y autocontenida (Eduardo 13-jul). Gotchas de paridad (CRC-16/CCITT-FALSE exacto,
> big-endian, relleno `0xFF`, `size_total` múltiplo del bloque de borrado), formato binario, cargador
> (rebase de tabla pública a flash), resolución FS-eclipsa-pack, API/wire del micro. **2 cabos sueltos
> LOAD-BEARING (§10):** (1) mecanismo de **versión-compatible de módulos** (desempata duplicados en la
> resolución) — es la misma pieza que la mejora **module-info + gate de ABI del `.mod`** → diseñarlos
> JUNTOS; (2) **particionado unificado FS↔packs** (`packs_offset`/`packs_size` ya reservados en
> `fs_lfs_pico.c`, pero falta el layout+dimensionado escrito).

- **H3.a — Herramienta PC `pack` (consola).** Programa de línea de comandos del PC para **montar** un
  pack (empaquetar `.mod` + resources en el formato de pack) y **extraer** su contenido (listar/volcar).
  Es el vehículo de pruebas del formato (round-trip mount→extract byte-idéntico) ANTES de tocar el micro.
- **H3.b — Flash del micro (particiones + grabar/descargar).** Descriptor de **particiones** para la
  zona de packs (contigua, alineada, FUERA del littlefs — los campos `packs_offset`/`packs_size` ya
  están reservados en `fs_lfs_pico.c`). **Grabar (Burn) y descargar** packs por el **wire** + un comando
  de **inventario de packs** (hermano del `LS`).
- **H3.c — VM: resolución y carga.** La VM sabe **buscar un pack**, **buscar un archivo/módulo** dentro
  de un pack, y **cargar un módulo** desde el pack (loader `.mod` desde XIP; la base ya está resuelta por
  el loader `.mdn` del AOT). Integrar con la resolución de imports/stdlib.

Por dónde entrar: descriptor de particiones `fs_lfs_pico.c`; loader `.mdn` (`mdn_loader.c`) como base de
"cargar desde flash a direcciones de runtime"; formato `.mod` (`MOD_FORMAT.md`).

---

## ⚙️ H4 — AOT  *(hito de V4; todo el AOT bajo un paraguas — Eduardo 13-jul)*

Consolida TODO el AOT/`native`: la **novedad (RISC-V)** + los bugs y huecos pendientes. Hoy `native`
tiene siempre fallback a interpretado; compilado a nativo solo en **ARM** (Pico + STM32). Cadena:
`AotCEmitter.java` → C → `.mdn` (thunks Thumb-2) → `mdn_loader.c` + registry por nombre + `aot_helpers`.
Diseño: `AOT_CROSS_MODULE.md` (cross-module) · `AOT_HYBRID_REFLECTION.md` (híbrido).

- **H4.a — Native RISC-V ⭐ (la novedad).** Compilar `native` a **RISC-V** (ESP32-P4, la placa gráfica
  insignia): port del emisor + del loader `.mdn` a RV32 (hoy el `.mdn` es Thumb-2/ARM; en ESP32
  Xtensa/RISC-V `native` cae a interpretado). Incluye ABI + relocations de RISC-V. (Xtensa del S3, menor prioridad.)
- **H4.b — AOT handle-aware (BUG/regresión). 🎯 PRIORIZADO 18-jul = tarea #302.** El puente `native→BP`
  (`aot_helpers.c` / `aot_call_bp` / `call_method` / `bridge_run_bp_frame`) pasa **direcciones planas de
  4 bytes** mientras la VM ya usa **handles de 8 bytes** → `make test-throwmsg` (SIGSEGV) y
  `make test-throwuser` (use-after-free en `call_bp`) **ROJOS** desde la migración de handles (H1); y el
  clic de Forms en la VM-C (#301) es la misma raíz. **Diseño completo del arreglo en
  [`AOT_HANDLE_MODEL.md`](AOT_HANDLE_MODEL.md)** (charla 18-jul): un solo ABI con handles de 8B en toda
  frontera, dirección plana solo interna/transitoria; secuenciación **inline primero, AOT después**
  (cimientos hacia arriba, evita conflicto AOT-inline); capa de raíces-de-GC (shadow stack) diferida al
  native compilado en placa. Política: los bugs nuevos de esta familia se APARCAN aquí, no se parchean.
  *(Era el "Diferido" de la sección H1.)*
- **H4.c — AOT cross-module #169.** Sin puente del intérprete (hoy vía `call_bp`+warning). Se apoya en
  el MISMO `slotOf` que **B-174b** (slots de vtable) → van juntos. Diseño: `AOT_CROSS_MODULE.md`.
- **H4.d — AOT casts + `^`.** `byte()/int()/float()/long()/double()` y el operador `^` en `AotCEmitter`
  (hoy caen a interpretado). **Desbloquea `compress` native.**
- **H4.e — try/catch DENTRO de native.** Diferido con motivo (#213 parte 2): el código `.mdn` no puede
  llamar `setjmp` (cero relocations externas). Necesita helpers `eh_native_try` + locals en context
  struct. (El `throw` DESDE native ya va; falta el `try` dentro.)
- **H4.f — huecos de cobertura de `AotCEmitter`** (hoy → fallback interpretado, NO bloquean): método
  privado/`super`/estático, intrínsecos cross-module, `for-range` no numérico, y demás "no soportado
  todavía". Ampliar para que más módulos compilen enteros a nativo.
- **H4.g — auditar el AOT por el ensanchado de refs 4→8B (Eduardo 13-jul).** Al atacar H4, REVISAR
  sistemáticamente si alguna emisión/llamada de opcode del camino AOT quedó SUELTA con el cambio de
  tipo de referencia (refs pasaron a handle de 8B; ver campaña de refs en `docs/V4_REF_AUDIT.md`). El
  censo dual-VM cubrió el intérprete de ambas VMs y el emisor de bytecode, **NO el `AotCEmitter` ni
  `aot_helpers.c`** (el AOT es "acceso plano", H4.b) → cualquier opcode que el AOT emita o implemente
  a mano (loads/stores de ref, campos owner, arrays de refs, push/pop de handles, string builtins)
  puede seguir asumiendo 4B. Barrer con la MISMA rúbrica del censo (OK / raw-safe / SUSPECT) sobre
  `AotCEmitter.java` + `aot_helpers.c`, y usar `test-throwmsg`/`test-throwuser` como oráculos rojos de
  partida. Va de la mano de H4.b (handle-aware) — son la misma familia.

> Nota: la cabecera de `AotCEmitter.java` ("integer i32 únicamente; float/string/arrays → TODO") está
> **rancia** — float (#166), arrays (#167), strings (#173) y tipos mixtos (#171) YA están.

---

## 🛠️ H5 — Compilador  *(hito de V4; ACOPLADO a H6 — Eduardo 13-jul)*

Mejoras de lenguaje/frontend. **Va PEGADO a H6** (formato de módulos): la sobrecarga y el `import` con
pack tocan el `.mod`, así que se harán puntos de H5 y H6 **entrelazados**.

- **H5.a — Sobrecarga de funciones (overloading) ⭐ (mejora principal).** Misma función, firmas
  distintas. Toca el frontend (resolución por firma en la llamada) + el **mangling en el `.mod`/`.bpi`**
  (hoy ya existe `nombre#arity` en los símbolos — base a extender a **firma completa**, no solo aridad).
  El cambio de encoding en el módulo vive en **H6.b**.
- **H5.b — `import` con pack.** Indicar el pack en el import: `import nombrepack:nombremodulo`. Se apoya
  en H3 (packs) + la resolución del módulo (**H6.c**).
- **H5.c — Llamadas asíncronas sencillas (para eventos de LVGL).** Azúcar tipo `async(…)`/`invokeLater(…)`
  para lanzar trabajo desde un handler de evento sin bloquear el bucle GUI (ver "GUI-blocking-from-event"
  en `V3_BACKLOG.md`). Interactúa con **valores-función/closures** (abajo): una ref a método debe elegir
  firma → roza la sobrecarga (H5.a).
- **Relacionados de lenguaje (candidatos a H5):** strings **multilínea + interpolación**;
  **valores-función/closures** (base del async de H5.c).

---

## 📇 H6 — Formato de módulos  *(hito de V4; ACOPLADO a H5 — Eduardo 13-jul)*

**Va PEGADO a H5.** Cambios en el `.mod`/`.bpi` que H5 necesita, más el salto del módulo autodescriptivo.

- **H6.a — Módulo autodescriptivo ⭐ (índice nº 7).** Meter la **interfaz (`.bpi`) DENTRO del `.mod`** →
  módulo auto-descriptivo. El `.mod` crece un poco (nombres+firmas, "asumible"; con Packs/XIP se ahorra
  RAM). Beneficios: (a) compilar contra un `.mod` sin su `.bpi`; (b) el **device resuelve nombre→slot en
  carga** = el **Camino B de forms**; (c) el Pack manager inspecciona qué exporta cada módulo; (d) **mata
  la familia de bugs de desfase `.bpi`/`.mod`/`.slots`** (Json rancio, slots de Gui) — un artefacto, una
  verdad; el sidecar `.slots` de H13.1 queda subsumido.

  **Precisión de Eduardo (15-jul): el `.bpi` NO queda opcional — queda TEMPORAL y se BORRA.** En cuanto
  el `.mod` está generado, el `.bpi` deja de ser necesario y hay que eliminarlo. La diferencia con "(a)
  compilar sin su `.bpi`" no es cosmética: un `.bpi` que sigue existiendo se sigue encontrando y sigue
  mintiendo cuando se queda rancio. Solo si **no existe** es cierto el "un artefacto, una verdad" de (d);
  si es opcional, es un deseo. Evidencia del día 15-jul: 8 rojos falsos del barrido de samples salieron
  de `.bpi` desparejados (318 en la librería, 1 en el dir de compilación), y el `.bpi` ausente lleva todo
  el día disfrazándose de bug del compilador.

  **Cabo suelto a decidir**: ¿el `.bpi` se **escribe y luego se borra**, o **no se escribe nunca** (la
  interfaz se construye en memoria y se embebe directamente)? Escribir-y-borrar deja una ventana: si la
  compilación se cae entre el write y el delete, sobrevive un `.bpi` huérfano — y si algo lo lee como
  fallback, vuelve el bug que este hito venía a matar. No-escribirlo nunca cierra el modo de fallo por
  construcción, en la línea de H1.8 (el ancho de ref se decide en 3 helpers, no en 17 sitios).
- **H6.b — Encoding de firmas para la sobrecarga (H5.a).** El módulo debe distinguir sobrecargas por
  **firma completa** (tipos de params), no solo por aridad. Toca `.mod`/`.bpi` + el mangling.
- **H6.c — Resolución de módulo desde pack (H5.b + H3).** Soportar `import pack:modulo` — localizar el
  módulo dentro de un pack por nombre.
- **Relacionado:** `N-dtblock-align` — hacer la alineación del data block **intrínseca** (cada entrada
  alineada por construcción) en vez del padding global de v3.0.1; toca el codegen del `.mod`.

---

## 🖼️ H7 — GUI  *(hito de V4; Eduardo 13-jul)*

Paraguas gráfico, MODESTO. Eduardo: "solo añadimos algún widget más; no recuerdo nada más pendiente."
- **H7.a — Repesca de widgets simples.** Añadir algún widget más — cosas SIMPLES, nada complicado (ya
  hay 14 widgets + color). *(El preview de forms → IDE **H8.b**; el Camino B nombre→slot → **H6.a**.)*

*Repartidos (Eduardo 13-jul; NO van en H7):*
- Bug **estado-GUI-entre-runs** → **pool de bugs sueltos**.
- **Campos tipados** de widgets → **backlog no urgente** (semilla del futuro diseñador de forms).
- **Gráficos de device/HW** (board-aware data-driven, rotación LTDC, PPA del giro, retirar
  `esp32p4-ws/`, ajustes P4 portrait/mirror) → **backlog de pulido gráfico** no urgente (varios ya
  "aceptados como están").

---

## 🧰 H8 — IDE  *(hito de V4; Eduardo 13-jul)*

- **H8.a — Ventana de gestión de Packs ⭐ (la novedad).** Pantalla nueva del IDE para **construir**
  packs, **grabarlos** en los micros (Burn por el wire) y verlos/borrarlos. Es el **Pack manager** que
  se sacó de H3 (dos lados como el explorer: biblioteca PC ↔ packs del micro). Se apoya en la
  infraestructura de packs de H3 (comando de inventario, Burn).
- **H8.b — Botón "Previsualizar ventana". ✅ HECHO 17-jul (f96b35a) — pero con VM-C, no Swing.**
  El botón **Run Window (VM-C)** (barra + menú Run) compila el `.bp`, hornea `nombre→slot` en los `.win`
  (mismo `FormBaker` que Run on Device) y lanza la **VM-C nativa (LVGL/SDL)** con el `.mod` → la ventana
  se abre en el escritorio. Se eligió VM-C sobre miVM/Swing porque **pinta EXACTAMENTE igual que la placa**
  (mismo LVGL) → el preview es fiel, no una aproximación Swing. Verificado con FormDemo (render OK).
  Pendiente menor: el clic del form da UAF en la VM-C (objptr `uint32_t` = handle truncado, gemelo de
  #292h/#293 en el lado Java) → **#301**. **NO hay diseñador de ventanas (drag&drop) todavía** — diferido.

*A backlog (Eduardo 13-jul; NO en H8 de momento — revisar en la próxima pasada del roadmap):*
- IDE **multiplataforma**: `purejavacomm → jSerialComm` + lanzador `.sh` (correr el IDE en Linux/Mac).
- **breadcrumb** de navegación (no urgente).

---

## 🔴 Bugs delicados (movidos de `PENDIENTES.md`, 27-jun)

> **Política (Eduardo 13-jul): los bugs van SUELTOS, no son hito.** Se arreglan **uno a uno cuando se
> pueda**, con su red de pruebas. Aplica a estos delicados Y a los menores de `PENDIENTES.md`.

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

- **AOT (todo)** → consolidado en el hito **H4** (ver §H4): novedad RISC-V + cross-module #169 +
  casts/`^` + handle-aware (bug) + try/catch en native + huecos de cobertura.
- **Forms — diseñador visual drag&drop** + **preview de forms en miVM/Swing** (el cargador es BP y
  miVM ya pinta en Swing → "preview" ≈ correr el form en miVM dentro del IDE).
- **Forms — Camino B**: tabla `nombre→slot` en el descriptor de clase del `.mod` (resolver en el
  device en carga; editar/enviar pantallas sin IDE). Re-baselinea el compat de emisión.
- **PACK = XIP de bytecode** → ahora es el hito **H3** (ver §H3 arriba). ~~+ lectura de SD~~ → **V5**.
- ~~**#153 — Dual-core RP2350**~~ → **APLAZADO INDEFINIDAMENTE** (ver índice nº 8; con los líos de
  memoria no se aborda, se retoma tras consolidar). El fix de la race **B1** espera con él; hoy 1 worker.
- **Net.Listener / servidor TCP** (`listen`/`accept`) sobre la Ethernet del P4.
- *(AOT en ESP32 Xtensa/RISC-V → ahora H4.a.)*
- **Neopixel en ESP32/P4** — backend WS2812 vía **RMT** (componente `led_strip` o encoder RMT propio).
  Hoy STUB en el ESP32 (el Pico ✅ lo hace vía PIO, #227/#228). Más plumbing que los demás periféricos
  (encoder + timing + dependencia de componente) → diferido por Eduardo (27-jun): "no es crítico ni urgente".
- **Rollout de gráficos a más kits** (solo equipos con recursos de sobra).
- **IDE multiplataforma** (`purejavacomm → jSerialComm`, lanzador `.sh`).
- **Strings multilínea + interpolación** (tanda de lenguaje) → **candidato a H5**.
- **Valores-función / closures** (tanda de lenguaje; charla 20-jun + 4-jul) → **candidato a H5** (base
  del async de **H5.c**): hoy NO se pueden pasar métodos como parámetros (modismos: override virtual u
  objeto-Runnable). Habilitaría el `async(…)`/`invokeLater(…)` ergonómico del GUI. Interactúa con la
  sobrecarga (**H5.a**): una referencia a método debe elegir firma.
- **`deflate`-lite** (LZSS → +Huffman) · **multi-fichero/Archive**.
- **Layout compacto de narrow** (`byte[]`/`int16[]` con storage real; hoy i32).

## 🟢 Mejoras menores (de `PENDIENTES.md`, candidatas a V4)
- **L-list-stm32-trunc — `LIST` truncado a ~14 entradas en STM32** (batch 4-jul): `handle_list`
  (`stm32_repl.c:~112`) arma la respuesta en un buffer fijo de 1024 B y corta al no caber → el explorer del
  IDE ve ~14 ficheros aunque haya más ("efecto ventana"). El pico streamea con `fputs` y no lo sufre. Fix:
  paginar o streamear la respuesta del LIST en el stm32. Cosmético (no pierde datos, solo el listado).
- **M6 — `const := Color.RED`**: inlinar el valor de enum (conocido en compilación) desde
  `EnumSymbol.values` en vez de dar "requiere literal". (Sale de N17, ya resuelto.)
