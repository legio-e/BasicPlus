# BasicPlus — Brújula de V2

> **V2 EN CURSO.** v1 cerrada (tag `v1.0` → `b92fb3f`). Esta es la brújula del
> ciclo **V2**: qué es, sus apartados, y qué define "V2 cerrada".
> Detalle de diseño: `docs/V2_BACKLOG.md`. Bugs/limitaciones: `docs/PENDIENTES.md`.
> Alcance de plataformas (el "sobre"): `docs/PHILOSOPHY.md`. Brújula histórica de
> v1: `docs/PROJECT_ROADMAP.md`. Cajón de V3: `docs/V3_IDEAS.md`.
>
> *Redactada 2026-06-07 (Eduardo + Claude).*

---

## 1. Qué es V2

Tras v1 (lenguaje + 2 VMs en paridad byte-idéntica + 2 familias de MCU + IDE +
debugger), V2 **consolida, endurece y amplía selectivamente**, suma una **tercera
familia** (STM32) y termina **documentando y publicando**.

Principios del ciclo:
- **Avanzar (features) y tapar agujeros (bugs) a la vez** (dirección del usuario).
- **Minimalismo de lenguaje**: cada feature se gana su sitio.
- **Paridad dual-VM byte-idéntica** (miVM Java ↔ bpgenvm-c C) como invariante en
  todo lo que toque la VM.
- **Diseñar para el piso** (ver el "sobre" en `PHILOSOPHY.md`): el dispositivo
  pequeño es el objetivo; lo grande es headroom, nunca requisito.

## 2. Apartados de V2 (mapa)

**Construido — la fase dura (VM + plataforma), ya cerrada:**

| Hito | Qué | Estado |
|---|---|---|
| H1 | Escalares: `byte`/`byte[]`, `long`, `double`, casts | ✅ |
| H2 | Strings = `byte[]` UTF-8 (índice por codepoint) | ✅ |
| H3 | GC non-moving (free-list, reuso real) | ✅ |
| H4 | Stdlib inicial (`Str`/`Map`/`Stats` + `parse*`) | ✅ |
| H5 | Modelo `Object` raíz (`toString`/`compareTo` polimórficos) | ✅ |
| H6 | Debugger de device (núcleo portable + oráculo Java↔C) | ✅ |
| H7 | Metro RP2350B: PSRAM, 48 GPIO, NeoPixel/PIO | ✅ |

Transversal ya hecho: tuplas T1/T3/T4, puentes AOT native↔BP (parcial),
consolidación inicial (#232 cascada, #233 catch cross-module, #236 GAP-2),
B1 caracterizado + mitigado (1 worker por defecto).

**Restante:**

| Apartado | Qué | Estado |
|---|---|---|
| H8 | Ampliación de lenguaje (default params ✅, tuplas first-class ✅) | ✅ (cajón abierto) |
| H9 | 3ª familia: **STM32** (ref. Nucleo-U575ZI-Q) | ✅ núcleo cerrado (Run+imports+GPIO+FS persistente+paridad); *stretch* AOT/L496 diferidos |
| H10 | Librería estándar: **ajuste + ampliación** | 🟡 parcial — LinReg ✅, file I/O en VM-C ✅; resto (log, descompresión, byte[]…) **poco a poco / diferido** |
| H11 | **TCP/IP**: cliente simple (empezar) | ⏸️ **diferido al final** (probablemente V3) |
| H12 | **Consolidación del IDE** (consola MS-DOS + doble-clic + pulido) | 🟡 **en curso** — línea de comandos ✅ · ver/editar ficheros del device (#231) ✅ (a probar en placa) |
| H13 | **Documentación y cierre** (docs doble + publicación) — puerta final | pendiente |
| — | **Consolidación** (tapar agujeros) — transversal | en curso |

## 3. Apartados restantes en detalle

### H8 — Ampliación de lenguaje
Incremental, en paralelo con la consolidación. **H8.1 (parámetros por defecto) y
H8.2 (tuplas first-class): ✅ HECHO (2026-06-07).** H8.1: sintaxis `:=`, sustitución
en el llamante, `.bpi` v7 expone los defaults. H8.2: tupla como valor de primera
clase (guardar en var, pasar como parámetro, `return` de tupla almacenada,
colecciones) + destructuring a lvalues no-simples (`arr[i]`, `obj.x`). Ambas: cero
coste de VM, funciones/métodos/ctores, same+cross-module, paridad dual-VM. H8 sigue
como **cajón abierto** (pueden entrar más temas). Detalle: `docs/H8_TASKS.md`.
*(§8 callbacks y §9 eventos van a V3.)*

### H9 — Tercera familia: STM32
"Otras plataformas = **STM32**" (foco, no dispersión). **Referencia: Nucleo-U575ZI-Q**
— Cortex-**M33** (= gemelo del core del RP2350) → reuso máximo del AOT; Nucleo-144
con pines libres para tests HW reales; **ST-LINK VCP** = transporte wire-v1 sin
levantar primero el USB-CDC. Después **Nucleo-L496ZG** (M4 / familia L4) como prueba
*barata* de generalización del HAL. **F769I-DISCO** reservada (SDRAM 16 MB para
validar "memoria grande" + es la placa de V3 por su LCD 4").

Clave del ahorro: por ser ARM Cortex-M (**Thumb-2**) como la Pico, se reutiliza
**lo difícil** (AOT, port de FreeRTOS ARM-M, AAPCS, toolchain `arm-none-eabi-gcc`)
— justo lo que el ESP32 (Xtensa) **no** pudo. Se reescribe la capa
*vendor-specific*: drivers/HAL, clock tree (RCC), flash/FS, USB-CDC. **STM32 sería
la 2ª familia con AOT.** La diversidad de STM32 (F4/F7/H7/G4/L4/U5…) asciende a
**parte de H9** la formalización del HAL / imagen-por-familia (§9b VM). Detalle y
plan por hitos: **`docs/H9_TASKS.md`** (arrancado 2026-06-07; andamiaje =
CubeIDE/CubeMX + injerto de la VM, transporte USART/VCP, single-core).

### H10 — Librería estándar: ajuste + ampliación
Segunda pasada sobre la stdlib (la primera fue H4). **Ajustar** lo existente
(consistencia, codepoint-correctness tras el cambio a UTF-8, **paridad dual-VM**
incl. GAP-1 = el subset de builtins de la VM-C) y **ampliar un poco** (p. ej. el
catálogo §14b de String y utilidades que se echen en falta). Minimalismo — sin
explosión de stdlib.

### H11 — TCP/IP: cliente simple (empezar)
Alcance V2 (usuario): **un cliente sencillo** — abrir un socket, leer/escribir,
cerrar. **Nada más de momento.** Modelado como clase (política HW=clase OO), p. ej.
`Net.Socket` / `Tcp.Client`. Primero donde el stack ya existe (ESP32 WiFi+lwIP, o
Pico 2 W CYW43+lwIP; tarea #145). **V3 lo retoma y amplía** (servidor, más API,
TLS…).

### Consolidación (tapar agujeros) — transversal
Anti-cascada del parser (§7a) ✅ · GAP-1 (subset de
builtins de la VM-C: fallo limpio atrapable + `abs`/`min`/`max` byte-exactos;
resto de ports = H10) ✅ · N6 (`.bpi` tolerante + compat. de módulos) ✅ ·
L3/L5/L6 (limitaciones menores: decidir in/out) · flecos H7 (#225 mapa de memoria
configurable por PSRAM, #229 FS grande del Metro) · bug temperatura ~412 °C ·
flecos del split IDE↔VM (A1: *Stop* real, `describePc` remoto; A2: multi-run en el
daemon). **B1 → V3** (acoplado al dual-core; en V2 mitigado a 1 worker, el device
single-core es inmune).

### H12 — Consolidación del IDE — hacia el final
- **Consola estilo MS-DOS**: añadir una **línea de input** en la pestaña de consola
  para `dir` / `cd` / ejecutar un módulo, etc. (la salida se queda como está). La
  mayor parte ya existe en la comunicación.
- **Doble-clic** en un fichero del micro → ver/editar (#231). ✅ *(a probar en placa)*
  Doble-clic: `.mod` ejecuta, el resto abre `DeviceFileEditor` (texto editable
  + `put`; binario en volcado hex de solo lectura). También botón **Edit** y
  comandos de consola `type`/`edit`. Reusa `Backend.get/put` (ya existían).
- **Pulido pendiente** (de `V2_BACKLOG`, sin prisa): find/replace en el editor y
  reorganización de `FrmMain` (IDE-1).
- **Configurar la plataforma de destino (el micro)**: necesaria para compilar las
  `function native` — el AOT genera código para una ISA concreta (Thumb-2 en
  RP2350/STM32, Xtensa en ESP32, host en PC). El IDE debe permitir elegir el target
  y pasárselo al pipeline AOT. (Más relevante con H9/STM32.)
- **Seleccionar la versión de la librería estándar**: hoy hay una (`bpstdlib/`); en
  el futuro habrá v1, v2, … El IDE debe permitir elegir contra qué versión se
  compila/enlaza (vía `stdlibDir` / `.bpi`). (Liga con H10.)

### H13 — Documentación y cierre (la puerta final)
Manual de usuario al día con V2 · doc técnica de internals (base hecha:
MOD_FORMAT/OPCODES/HEAP_LAYOUT/BUILTINS/AOT/SMP) · **mapa de arquitectura de 1
página** (el primer doc que baja la barrera a contribuidores) · suite de tests / CI
verde · publicación en GitHub: licencia (MIT/Apache + NOTICE), README, y el **doble
público** (la mayoría: graba un firmware y programa en BP / minoría: construye sus
herramientas con `.mod` + protocolos documentados).

## 4. Los 3 pilares (cómo se cierra)

El §0 de `V2_BACKLOG` fijó **Robustez → Documentación → Ampliación** *en ese orden*.
La dirección reciente del usuario lo relaja a **los tres en paralelo** ("avanzar y
tapar a la vez"), con la **publicación como puerta final**:

- **Robustez** = Consolidación (tapar agujeros) (+ H3 GC, ya hecho).
- **Documentación** = **H13** (documentación y cierre).
- **Ampliación** = H8 + H9 + H10 + H11 (+ **H12**, consolidación del IDE).

## 5. "V2 cerrada" se declara cuando

1. **Bugs críticos = 0** (B1 cuenta como diferido/mitigado); huecos de consolidación
   cerrados o conscientemente diferidos.
2. **H8, H10, H11, H12 entregados** en su alcance acordado (H8/H10/H11 con
   **samples + paridad dual-VM**; H12 = IDE).
3. **H9: STM32 end-to-end** en la placa de referencia (U575) — candidato a ser
   **el hito de cierre** (igual que el ESP32 cerró v1).
4. **(H13) Documentación doble completa** (usuario + internals + mapa de 1 página)
   y **"de cero a Hello World en MCU"** reproducible en <15 min.
5. **(H13) Publicación lista** (licencia + README + repo público).

Tras cerrar V2: solo bug fixes / docs / samples; toda feature nueva → V3.

## 6. Qué NO entra en V2

- **→ V3** (`docs/V3_IDEAS.md`): GUI gráfica (LVGL), dual-core (#153) + fix de B1,
  eventos (§9), callbacks / función-valor (§8), TCP/IP ampliado.
- **→ backlog "cuando haya hueco"** (no condicionan el cierre): refinamientos AOT
  (#161/#169/#193/#212/#213), comms extra (#138 CDC-mux, #145 WiFi más allá del
  cliente simple), packaging (§9/§9b/§11 VM: imagen por familia, HAL formal,
  host-VM como simulador).

## 7. Próximo paso

**H8** ya en marcha: **H8.1 (parámetros por defecto) → H8.2 (tuplas first-class)**.
El resto (H9/H10/H11) sin orden rígido, por interés/necesidad, con la consolidación
en paralelo y la **publicación al final**.
