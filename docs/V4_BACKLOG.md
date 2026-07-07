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
