# BasicPlus — Backlog de V4

> **Fuente única del backlog de V4.** V4 = **consolidar + mejorar** lo que ya hay
> (un poco como fue V2), tocando lo **delicado** —módulos (slots/vtable) y GC— con
> **mucho cuidado y muchas pruebas**. NO se reestructura en V3; lo delicado se
> aparca aquí. Visión/decisiones de fase: memoria `v4-consolidacion-v3-bugs-obvios`.
>
> Estado: `pendiente` / `en curso` / `cerrado`. Convención: B=bug · L=limitación ·
> N=hallazgo · M=mejora · #NNN = id de task histórica.

---

## 🔴 Bugs delicados (movidos de `PENDIENTES.md`, 27-jun)

Tres bugs que exigen tocar la maquinaria de slots/vtable o el GC → se hacen en V4
con red de pruebas (no son fixes contenidos de V3).

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

### B-freeref-no-recursivo — `OP_FREE_REF` no libera en cascada los campos `owner` (F3 v1)
`interp.c:1498`: `FREE_REF` sólo libera el objeto raíz; el TODO pide recorrer el `owner_bitmap` y liberar
recursivamente los campos `owner`. Un árbol de objetos con dueños no se libera en cascada → **fuga hasta que
el GC mark-sweep lo recoja** (no permanente, pero las owner-semantics prometen free determinista). Hallado
28-jun (relacionado con L7).

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
- **`deflate`-lite** (LZSS → +Huffman) · **multi-fichero/Archive**.
- **Layout compacto de narrow** (`byte[]`/`int16[]` con storage real; hoy i32).

## 🟢 Mejoras menores (de `PENDIENTES.md`, candidatas a V4)
- **M6 — `const := Color.RED`**: inlinar el valor de enum (conocido en compilación) desde
  `EnumSymbol.values` en vez de dar "requiere literal". (Sale de N17, ya resuelto.)
