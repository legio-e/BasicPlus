# BasicPlus — Backlog de V3

> **Fuente única del backlog de V3.** Las *tasks* del IDE/sesión son efímeras
> (el trabajo en curso de cada sesión); **este doc manda**. Visión y principios:
> `V3_ROADMAP.md`. Charlas de diseño: `V3_IDEAS.md`. Al cerrar un ítem se anota
> su resultado aquí y, cuando toque, pasa a un futuro `HECHO_V3.md`.
>
> Estado: `pendiente` / `en curso` / `cerrado`. (`#NNN` = id de task histórica.)

---

## 🗺️ Mapa de hitos (post-H7, Eduardo 20-jun)

H6 (widgets GUI, en la DK2) ✅ · H7 (lenguaje: `^`, `eval`, separador `_`, continuación) ✅.
Orden siguiente:
- **H8 — librería estándar** (revisión/ampliación). *Anotado; SE SALTA por ahora.*
- **H9 — revisión del IDE** (cosillas a mejorar). *Anotado; SE SALTA por ahora.*
- **H10 — paridad HW del STM32 + reset-cause (ACTIVO):**
  - (a) Implementar en STM32 (HAL U5) las clases de control de HW que existen para Pico/ESP32
    pero **NO** para STM32: **I2C** (`I2c.Bus`), **ADC** (`Adc.Channel`), **PWM** (`Pwm.Slice`),
    **Timer** (`Timer.Alarm`), **WDT** (`Wdt.Timer`), **RTC** (`Rtc.Clock`), **pulse-counter**,
    **Neopixel**… (UART + SPI ya hechos en H15). **Primero en la Nucleo-U575** (placa STM32 de V2).
  - (b) **Reset-cause** — partido en dos (Eduardo 26-jun, "hagamos a y b pasa a pendiente"):
    - **(a) ✅ HECHO (`445a823`) — causa-de-reset por REGISTRO HW en el INFO de las 3 familias +
      IDE.** ESP32 `esp_reset_reason`, Pico `watchdog_caused_reboot`, STM32 `RCC->CSR` (ya tenía);
      campo `resetReason` en INFO_REPLY; el IDE pinta "Reset : <causa>". Sin RAM retenida. Falta el
      reflash de Eduardo para el e2e.
    - **(b) 🔜 PENDIENTE — portar el BREADCRUMB retenido** (traza de 16 + 1ª pegajosa, hoy solo
      STM32 vía registros TAMP backup; `gpio_stm32.c` + interfaz `setMark/markAt/bootCount` del
      backend pico, ya abstracta) a **ESP32** (`RTC_NOINIT_ATTR`) y **Pico** (sección no-init RAM;
      sin dominio con pila → se borra en power-off, aceptable). Es la parte HW-específica
      (almacén retenido por chip). V3 si se quiere la simetría, o V4. Liga con la observabilidad
      de bring-up (V3_IDEAS §6).
  - (c) Al terminar, **portar el desarrollo a la Discovery DK2** (misma familia U5).

(GUI en pausa ≥1 semana tras cerrar H6 en la DK2.)

## ✅ Estado y decisiones de cierre de V3 (24-jun)

**Buque insignia HECHO:** GUI de BasicPlus, MISMO bytecode, en 3 silicios (Java host ·
ARM/STM32-DK2 · RISC-V/ESP32-P4). Las dos columnas (DK2 + confirmación P4) en pie → V3 en
**fase de cierre** (rematar + tapar agujeros + documentar). Cerrado además: H10 (HW STM32
Nucleo+DK2), stdlib Compress/Json/Freq (core), P4 completa (VM + G1-G6 gráficos + Ethernet/wire).

**Decisiones (Eduardo, 24-jun):**
- **P4 = placa BP COMPLETA (HW al 100 %) → DENTRO de V3.** Motivo de peso: hay **pantallas
  con P4 integrado, baratas → candidatas reales para proyectos**; por eso la P4 debe hacerlo
  TODO, no solo gráficos. Portar los backends HW del firmware P4 (GPIO/I2C/SPI/ADC/PWM/UART…
  hoy **stubs**), al estilo S3 (ESP-IDF). **NO ahora** (atracón de flashes) — más adelante.
- **TCP server (por la Ethernet del P4): APLAZADO** (sin decidir; aplazar no es grave). La
  Ethernet del P4 ya funciona → desbloqueado técnicamente, pero no prioritario.
- **Rollout de gráficos a otros kits: solo equipos con recursos de sobra (flash/RAM/pines);
  NO prioritario.** La pantalla con S3 "podría caber pero me da igual" → no se persigue.
  DK2 + P4 son las plataformas gráficas de V3.
- **Pausa de flasheo** tras el maratón del P4 → el trabajo inmediato debería evitar reflashear
  (docs / IDE host-side / AOT host).

### Plan de cierre de V3 — ORDEN (24-jun, visto bueno de Eduardo)

**NO se añaden más features a V3.** Solo queda ejecutar, en este orden (lo de PC primero —
venimos de un atracón de flashes; el P4-HW, flash-intensivo, cuando se recarguen pilas):

**Numeración de hitos** — V3 reinicia su propia serie. Hechos: H1–H7; **H8** (stdlib) = núcleo
✅ (Compress/Json/Freq), resto→V4; **H10** (HW STM32) ✅; **H11 = ESP32-P4 / familia RISC-V** ✅
(VM + gráficos G1-G6 — etiqueta retro para anclar la serie). **H8 y H9 eran los diferidos** del
mapa post-H7; **H9 (revisión del IDE) se materializa AHORA** dentro del cierre. Los 6 pasos del
cierre = **H12…H17** (1=H12 · 2=H13 · 3=H14 · 4=H15 · 5=H16 · 6=H17):

1. **(H12) IDE Fase 1** *(PC, sin flasheo)* — (a) comodidad del editor: comentar/indentar + barra de
   botones + fix del árbol remoto cortado; (b) AOT automático (ARM: RP2350+STM32): config
   por-proyecto + detección toolchain + pipeline detect→C→gcc→.mdn→subir + degrade a interpretado
   con warning.
2. **Forms (la "ventana")** *(PC)* — (a) cargador `Gui.Form.load` (JSON→árbol Component, binding
   por nombre) + esquema del form; (b) IDE Fase 2: editar forms desde el IDE (+ candidato: repaso
   AOT-casts).
3. **P4 HW completo** *(flasheo intensivo → cuando se recarguen pilas)* — backends GPIO/I2C/SPI/
   ADC/PWM/UART en el firmware del P4 (estilo S3).
4. **Tapar huecos** — bugs conocidos + INFO/causa-de-reset en el IDE + pulidos sueltos del backlog.
5. **Documentación** — documentar (usuario + interno). **AYUDA ESPECÍFICA DE GRÁFICOS en un
   DOCUMENTO APARTE** (NO en manual.html): el que no usa gráficos no la necesita, el que sí
   necesita TODOS los detalles del API `Gui.*` (widgets, eventos, color/align/fuente, forms/`.win`)
   — decisión de Eduardo 26-jun.
6. **Finalización (estilo V2, = H14)** — batería de PRUEBAS en dispositivos reales para asegurar
   que todo funciona; los bugs que aparezcan, se arreglan; + la **publicación del cierre** (V3 ha
   ido en LOCAL todo el tiempo, repo público congelado en V2 → publicar V3 es la decisión del final).

Orden flexible donde las piezas son independientes (cargador ↔ AOT intercambiables; el P4-HW puede
adelantarse en cuanto haya ganas de flashear).

**ALCANCE V3 — acotado (Eduardo 26-jun):** **H18 (memoria) y H19 (modelo de proyecto) SE QUEDAN en
V3** (se planteó moverlos a V4 y Eduardo dijo que NO). Regla: **acotar, no crecer sin fin**; lo que
salga a arreglar, se arregla. Estimación **~1 semana / 10 días**; si se alarga un poco, sin problema.
El norte es **converger** (que todo FUNCIONE), no añadir features nuevas grandes.
- **Verificación HW FUSIONADA** (pasos 1+3 de la lista de cierre = UN lote): reflash de confirmación
  (CRC del PUT + reset-cause + RtcDemo) **junto con** las pruebas de pin (I2C/SPI con dispositivo).
- **Documentación (paso 5):** hoy 2 documentos (el **lenguaje** + las **librerías estándar**); la
  **parte gráfica = 3er documento aparte** (el que no quiera gráficos se ahorra esa doc). Es la
  PRÓXIMA tarea al retomar. Después: ver qué documentación EXTRA necesita V3.

**Refinamientos (Eduardo, 26-jun, tras cerrar Forms en placa — H13 ✅):**
- **Gráficos: YA HAY MÁS QUE SUFICIENTE para una 1ª versión.** La repesca de widgets (paso 4) se
  limita a **revisar alguna cosa SIMPLE** — nada complicado.
- **Preview de forms en miVM/Swing → V4** (confirmado; NO se cuela en V3).
- **Documentación (paso 5): incluir una AYUDA ESPECÍFICA DE GRÁFICOS** (guía del API `Gui.*`).
- **HW del P4 (paso 3) = lo más delicado → DESPUÉS** (flasheo intensivo; sin prisa, primero
  lo de PC: huecos + docs).
  - **Progreso (26-jun):** backends del firmware ESP32 (S3 reusado por el P4) ya están:
    GPIO/UART/SPI/I2C + PWM (LEDC) + ADC (esp_adc) + temperatura interna (`temperature_sensor`,
    39.4 °C en placa) + counts board-aware (ADC/PWM `Pico.*` intrínsecos, P4=14/14) + **contador
    de pulsos (PCNT)** + **RTC (gettimeofday) con sync "TIME" del IDe** (commits `8c7e906`,
    `b369317`, `f1bf334`, `953d7d9`, `0b3a16a`). **WDT ✅ backend escrito** (`4dad832`, Task WDT;
    pendiente prueba EN PLACA). **Neopixel → V4** (Eduardo 27-jun: "si es muy difícil, a V4 sin
    problema, no es crítico" — en ESP32 va por RMT/led_strip, más plumbing). Ya nada bloquea.
  - **✅ VERIFICADO EN EL P4 (26-jun): `PwmCount.bp` = PWM→contador con puente → 999/1000 flancos
    a 1000 Hz × 1 s, "PWM->contador OK", exit 0.** Valida en HW real PCNT + LEDC + que la API
    ESP-IDF compila (el `idf.py build flash` pasó). RTC/`TIME`: compilan (misma unidad); el sync al
    conectar no se probó por separado.
  - **✅ CUARTETO DE LOOPBACKS VERIFICADO en el P4 (26-jun), todo con puente pin↔pin y exit 0:**
    `GpioLoop.bp` (out→in 4/4), `UartLoop.bp` (TX→RX "Hi!\n" 4/4 bytes), `PwmCount.bp` (PWM→PCNT
    999/1000), `RtcDemo.bp` (RTC avanza tras calibrar). Backends GPIO/UART/PWM/PCNT/RTC validados
    con pines reales.
  - **Falta de pin → PRUEBAS FINALES de V3 (Eduardo 26-jun):** I2C/SPI con un dispositivo real +
    ADC leyendo un pin analógico real (la temp interna es periférico aparte, ya probada). NO
    bloquea — GPIO/UART/PWM/PCNT ya prueban el camino GPIO↔periférico de punta a punta.
  - **WDT en el ESP32/P4 ✅ BACKEND ESCRITO (27-jun, `4dad832`)** — `gpio_esp32.c` (reusado por S3 y
    P4) usa el **Task Watchdog Timer** (`esp_task_wdt_init`/`reconfigure` + `add(NULL)`,
    `trigger_panic=true` → panic→reboot = reset, paridad con Pico/STM32). Maneja los 2 estados de init
    confirmados en sdkconfig (S3 init / P4 reconfigure); panic=PRINT_REBOOT en ambas. Camino BP
    validado en host (WdtTest → "fin"). **Falta SOLO la prueba EN PLACA** (lote de reflash): `WdtTest`
    = no resetea; `WdtDemo` = resetea ~3 s tras dejar de alimentar. Con esto, WDT a la par en las 3
    familias. (Neopixel ESP32 → V4 por decisión de Eduardo.)

### V4 — fuera de V3 (Eduardo, 24-jun)

**Tema de V4: consolidar + mejorar rendimiento** (un poco como fue V2), **además de lo diferido**:
AOT en ESP32 (Xtensa/RISC-V), servidor TCP (`Net.Listener`), **diseñador visual de forms drag&drop**,
preview de forms en PC (si no se cuela antes), rollout de gráficos a más kits, dual-core RP2350 (#153),
`deflate`-lite, multi-fichero/Archive, IDE multiplataforma (jSerialComm), strings multilínea + interpolación,
AOT cross-module (#169). **El alcance de V3 queda CERRADO** ("y ya nada más").

**PACK + lectura de SD (nuevas prestaciones V4, charla 24-jun):**
- **PACK = XIP de bytecode.** Un "pack" es como un ZIP **sin comprimir** (un TAR): cabecera +
  directorio `{nombre, offset, tamaño}` + blobs concatenados (módulos compilados, código alineado
  4 B). Se graba **entero en una región de flash propia, APARTE del FS**. **Objetivo: ahorrar RAM.**
  Hoy cargar un módulo = copiar el `.mod` entero a RAM; el grueso es el **código** (opcodes), que es
  **solo-lectura**. Como la flash está mapeada en memoria (RP2350 `0x10000000`, STM32 `0x08000000`,
  ESP32 vía `mmap`), el intérprete lee `code[pc]` **directo de flash** y NO lo copia. **El módulo SÍ
  va a RAM** (globales mutables + tablas), **su código NO** (se ejecuta en sitio / XIP). Clave en
  micros pequeños, donde el recurso escaso es la RAM (la flash sobra) — enlaza con "cosas increíbles
  en micros de ~1 $".
  - **Alcance (Eduardo): Fase 1 y basta — "con que funcione".** Solo el `code[]` en flash; el resto
    (pool de constantes, tablas, globales) en RAM como hoy. (Fase 2 hipotética, NO objetivo = pool de
    constantes también en flash con interning perezoso de strings — ojo GC.)
  - Simetría con el `.mdn`: ese YA es XIP para código **nativo**; PACK lo extiende al **bytecode**.
    Unidad desplegable unificada = pack con `.mod` (+ `.mdn` opcional). Cold path interpretado desde
    flash, hot path AOT nativo.
  - Grabado por el IDE ("Burn Pack" sobre el wire = borra+escribe la región, mini-flasheo). Orden de
    resolución de `import`: FS → pack → embebido. **Regalo:** la **stdlib como pack** → actualizar
    `Gui` sin reflashear firmware (justo el dolor de V3).
  - Aviso: leer de flash es un pelín más lento que de SRAM (lo tapa la cache XIP de cada chip + los
    bucles calientes van por AOT).
- **Lectura de SD.** Almacenamiento masivo removible (datos/assets/logs). **NO se puede XIP** (es
  dispositivo de bloques, no mapeado) → de la SD se **carga a RAM** como un archivo normal, o sirve
  para **transportar** packs que luego se graban en flash. Complementa al PACK (tier de capacidad),
  no compite. (Probable clase OO estilo `Sd.Card`/`Sd.File`, por la política "HW nuevo = clase".)

**Binding de eventos de Forms — tabla nombre→slot en metadatos del `.mod` (revisar en V4, decisión 25-jun):**
En V3/H13 los eventos se resuelven con el **Camino A**: el IDE resuelve `evento→slot` contra el `.bpi`
(el `ClassSig` ya lleva método→slot, de L2.1) y **hornea el slot** en el `.win` desplegado → handlers =
métodos REALES (virtuales, override-correcto) **SIN tocar el formato del `.mod`** (despacho en el device via
`INVOKE_VIRTUAL`/`call_method_i32`). Tag de versión de layout en el `.win` (como el ABI tag de los `.mdn`)
para rechazar pantallas rancias si los slots se mueven al recompilar.
- **Camino B (a valorar en V4, con las optimizaciones/cambios de `.mod` ya sobre la mesa):** tabla
  `nombre→slot` en el **descriptor de clase del `.mod`** (solo métodos `published`) para resolver en el
  **device en carga** → permite editar/enviar pantallas sueltas sin IDE. Aditivo, **re-baselinea la
  comprobación de emisión del compat** (como `^`/`eval` en H7).
- **Invariante a blindar con aserción** (idea de Eduardo + chat): todo método `published` hace round-trip
  `nombre↔slot↔nombre`, y el slot es **determinista e idéntico en las 2 VMs** (lo emite el compilador, lo
  consumen ambas — ya se cumple por el oráculo byte-idéntico). El desajuste se caza con un check, no con la
  memoria.
- **Sinergia con AOT (#169):** la misma resolución `nombre→slot` vía `.bpi` desbloquea las **llamadas a
  métodos cross-module desde native (AOT)**, hoy diferidas. El despacho AOT YA es por slot (`call_method_i32`
  con `ClassSymbol.slotOf`, #174b); para una clase importada solo falta tomar el slot del `.bpi` (ClassSig).
  → Forms (Camino A) y AOT-cross-module se apoyan en el MISMO cimiento; conviene dejar el `slotOf`
  cross-module sólido (verificar que el `ClassSymbol` importado lo lleva poblado).
- **Progreso H13.1 (25-jun):** **paso 1 ✅** keystone confirmado — el `.bpi` codifica el orden de vtable +
  `layout numMethods` (Window=30) y `ClassSymbol.ensureMethodSlots` ya calcula slots con base externa
  (= L2 v3.a verificada); el IDE puede hornear reusando `slotOf` **sin tocar `.bpi`/`.mod`**. **Paso 2 ✅**
  primitiva `Gui.__guiInvokeBySlot(win, slot, sender)` (builtin id 207) en **ambas VMs**: paseo de vtable con
  fallback al padre (idéntico a `INVOKE_VIRTUAL`) + frame de método `[this=win, sender]`; reusa la maquinaria de
  `invokeHandlerByName` (miVM) / `bpvm_call_bp_from_builtin` (VM-C). **Paridad dual-VM verificada** (despacho a
  método en slot conocido → byte-idéntico). **Paso 3 ✅** loader en `Gui.bp`: Component con campos
  `__win`/`__slotClic`/`__slotChange` + `__bindEvents`; `onClick/onChange` → `__guiInvokeBySlot`; clase `Window`
  + `load(.win)` (devuelve el raíz) + `buildForm`/`makeWidget`/`applyCommon` (privadas) + `import Json`.
  **Verificado end-to-end dual-VM** (FormTest: `.win` con `clicSlot` horneado a mano = 30 → `MyForm.onOk`,
  "ok-pulsado" byte-idéntico en miVM y VM-C → valida loader+binding+dispatch+slot de un tirón). Pendiente:
  **paso 4** el IDE hornea el slot en el `.win`: **4.0 ✅** el compilador emite el sidecar `<Módulo>.slots`
  (JSON `método→slot` de las clases públicas, autoritativo vía `ClassSymbol.slotOf`) en cada `--compile` del
  módulo raíz — verificado `FormTest.slots = { "MyForm": { "onOk": 30 } }` (coincide con el slot del paso 3);
  **4.1 ✅** `FormBaker` (lexer-java, herramienta de build como AotMain) resuelve `nombre→slot` con el `.slots`
  + `Json.write` para el round-trip del `.win`; el hook `FrmMain.bakeWinResources` hornea los `.win` de
  `resources/` en el Run-on-Device antes de subir (copia temporal; el `.win` de autoría intacto). Horneado
  VERIFICADO por CLI (`authored.win` con `"clic":"onOk"` → `clicSlot:30` → `onOk` corre); falta la prueba
  end-to-end EN EL IDE con un proyecto de form real (device) — de Eduardo. Fat-jar reconstruido. La clase
  ventana es `public` (normal). Hueco anotado: el Run-on-host (`doRun`) no sube `resources/` → el preview de
  forms en miVM/Swing necesitaría eso aparte. **paso 5** sample pulido +
  paridad + rebuild de blobs (fat-jar IDE + Gui.{mod,bpi} + firmwares).
- **Flujo del editor de Forms (diseño Eduardo, 25-jun):** se diseña PRIMERO el `.win` (JSON: controles +
  eventos por nombre + clase ventana) — a menudo sin BP todavía. El IDE **scaffoldea**: escribe en el `.bp`
  los stubs de los métodos handler que nombran los eventos del `.win` (los que falten). Proceso ITERATIVO:
  se alterna `.win` (añadir controles+eventos) ↔ `.bp` (rellenar handlers). Al **Previsualizar**: el IDE
  compila el `.bp`, hornea `nombre→slot` en el `.win` (Camino A) y lo ejecuta — en miVM/Swing = preview
  instantáneo en ventana, o en device para la prueba real. **La clase ventana es `public`** (lo normal) → el
  IDE lee su `método→slot` del `.bpi`. Editor VISUAL (arrastrar controles) = más adelante (V4 / H13.2+); por
  ahora el `.win` se escribe a mano. El **paso 4** de H13.1 = el **horneado** (en Preview/Deploy); el
  **scaffold** (generar stubs) es complemento ergonómico.
- **Organización del FS de proyectos en el micro (Eduardo, 25-jun):** **B HECHO** — el `readFile` de la VM-C
  cae a `/app/<nombre>` para paths RELATIVOS (nombre simple) cuando el directo falla (espejo del cargador de
  imágenes en `gui.c`), así `load("main.win")` encuentra el resource que el IDE subió a `/app/`. OJO: cambio de
  firmware → **reflash** para probarlo. **A PENDIENTE (la mejora que pide Eduardo):** cada proyecto en SU
  carpeta `/app/<proyecto>/` (no plano), con un **directorio base por ejecución** = esa carpeta para que
  `readFile`/`loadFile` e imports de módulos de la app resuelvan relativo a ella (como el workdir de la VM
  host); toca IDE (paths de subida + run) + device (resolución base en los 3 firmwares). **Norma de paths
  (Eduardo):** dentro de un proyecto los paths son RELATIVOS; ofrecer además una utilidad para convertir
  relativo→absoluto (para quien solo pueda trabajar con paths absolutos).

## 🎨 GUI (objetivo cabecera)

El camino crítico —upcall C→BP → `Gui.*` en miVM → VM-C host (LVGL+SDL) →
portabilidad → micro → cross-family— se detallará en los hitos H1+ (ver
`V3_ROADMAP.md` §6 y la decisión consolidada en `V3_IDEAS.md` §1). Las tareas
concretas se crean al arrancar cada hito.

- **Ventana = módulo + form JSON (diseño de UI por JSON) — concepto clave de V3 (charla 24-jun).**
  Una "ventana" se define con DOS ficheros: el **módulo** (`.bp`, el código) + un **form**
  (NUEVO: fichero **JSON** con el árbol de Componentes). El módulo, en su init, **carga el
  JSON y construye la ventana dinámicamente** → diseñar la UI desde el PC sin reprobar en el
  device. Encaja con lo hecho: modelo `Component` (H6) = el árbol; `Json.bp` = el parser;
  GuiBackend Swing de miVM = preview en el PC casi de regalo. **Alcance (Eduardo):**
  - **FUNDAMENTAL (V3): el cargador runtime** `Gui.Form.load(json)` — BP portable,
    byte-idéntico en los 3 silicios; recorre el árbol y crea los `Gui.*`. Implementación:
    dispatch tipo→constructor + propiedad→setter por **switch** (BP no tiene reflexión; el
    set de widgets es acotado → manejable).
  - **MEJOR (V3 si cabe): editar los forms desde el IDE** (editor de form / JSON asistido).
  - **PARA NOTA (puede ir a V4): preview en el PC** — y OJO: es **más barato de lo que parece**,
    porque el cargador es BP y miVM ya pinta en Swing → "preview" ≈ *correr el form en miVM
    dentro del IDE*. El diseñador visual drag&drop completo = V4.
  - **Binding evento↔código: por NOMBRE** (el form nombra el widget; el código hace
    `form.get("btnOk")` + handler/subclase). `onClick:"handler"` por-nombre = azúcar futuro.
    Modelo "diseñador + code-behind" (Qt/Android/NetBeans), conviviendo con el modo imperativo.
  - Form = **JSON soberano** (NO el XML privado de LVGL); mapea a nuestro árbol Component.
    Diseñar el esquema con anidación desde el principio.

- **GUI-blocking-from-event — ejecutar trabajo largo / diálogo modal con respuesta
  desde un handler de evento sin congelar el GUI** (Eduardo, 20-jun). Raíz: el
  upcall (`onClick`/`onChange`) corre en el worker que TAMBIÉN bombea el GUI
  (modelo ISR); si el handler bloquea, en LVGL/micro (un solo hilo) es **deadlock**
  (nadie procesa el toque que cerraría el modal / nadie repinta). Dos casos:
  (a) **proceso largo** desde un evento → seguramente `Thread` aparte + marshalling
  del resultado/repintado al pump del GUI (el GUI es single-thread en LVGL); (b)
  **diálogo modal síncrono** (`if confirmar("¿Borrar?") then …` en línea) → o el
  mismo patrón async-con-callback (ya cubierto por Msgbox+botones+`onChange`), o un
  **bombeo anidado** re-entrante en la llamada (más complejo, re-entrancia). Diseñar
  con calma; NO bloquea H6 (Msgbox-aviso es asíncrono y suficiente para H6).
  **Refinamiento (Eduardo, 21-jun) — la ERGONOMÍA es la clave:** el caso (a) debe
  resolverse con una **llamada sencilla async** (estilo Swing `invokeLater`/`SwingWorker`):
  el usuario NO crea ni arranca el Thread — escribe algo como `async(tarea)` dentro del
  `onClick` y **BP lo gestiona por debajo** (crea/arranca/limpia el hilo). Y un
  `invokeLater(closure)` que marshale de vuelta al hilo del GUI para repintar/actualizar
  widgets con seguridad (reusa la cola de eventos del upcall). Objetivo: que programar la
  GUI sea AMIGABLE, no un infierno de threads manuales. **Decisión de diseño clave:** ¿tiene
  BP valores-función/closures? Si NO, la API usaría objetos estilo Runnable (clase con `run()`
  override-able, como `onClick`); si se añaden closures, `async(() -> …)`. → **REPESCA del
  GUI** (no pertenece a ninguna H).

## 🛡️ Arnés de no-regresión V2→V3 (montar PRIMERO)

Instrumental del principio 7 (`V3_ROADMAP.md` §4): la red antes del trapecio.
- Oráculo **VM-Java V2 congelada** (jar del tag `v2.0`) para diff de comportamiento.
- **OpCodes congelados** (golden `nombre→id`; V3 = superconjunto, ids V2 intactos).
- **Guardián del `.mod`**: lectura (`.mod` V2 + salida esperada → VM-V3 byte-idéntico)
  y escritura (snapshot del emisor + disassembler #163 vs `MOD_FORMAT.md`).
- **Regresión V2 verde** en las VMs de V3.

## 🧩 Infraestructura (la arrastra el GUI)

- **#153 — Dual-core RP2350** (single-core SMP validado en placa; dual-core con
  SWD). **Incluye el fix de B1** (la race solo asoma con paralelismo real; en V2
  mitigada a 1 worker, el device single-core es inmune). Un núcleo a lo gráfico.
- **#225 — Mapa de memoria configurable por PSRAM** (`memorySize` de `BpVM.cfg`);
  verificar en ambas placas RP2350. Para framebuffers/recursos del GUI.
- **#229 — FS grande aprovechando la flash del Metro (16 MB)**: detección
  runtime + FS mayor.

## 🌐 Stdlib

- **#145 — WiFi en placa (Pico 2 W / ESP32-S3) + servidor TCP.** H11.a (cliente
  TCP host) ✅ en V2; queda **H11.b** (WiFi al boot con `/sys/wifi.json`) +
  **H11.c** (backend `Net` sobre lwIP), y de premio el wire v1 sobre TCP ("Run on
  Device" sin cable). Diseño en `HECHO_V2.md` (H11) + `WIFI_TCP_REFLECTION.md`.
- **Servidor TCP (`listen`/`accept`) — APLAZADO; se retoma con la Ethernet del ESP32-P4**
  (decisión Eduardo, 22-jun). El cliente `Net.Tcp` ✅ V2 (H11.a); falta el lado servidor
  (builtins listen/accept + `Net.Listener` + thread-por-conexión + server de prueba en PC).
  **Estrategia: desacoplar TCP/IP de WiFi.** Al arrancar el ESP32-P4 se valida primero su
  **Ethernet** (puerto físico, IP fija — liga con la idea del log-Ethernet del P4); si va
  bien, se retoma la librería TCP sobre ese transporte cableado. WiFi (#145) queda aparte
  (otras connotaciones: lwIP-sobre-WiFi, credenciales…). Cierra la tanda de stdlib de
  esta sesión (Compress ✅ / Json ✅ / Freq ✅ en placa).
- **Compresión (CORE hecho, 22-jun).** `Compress.decompress` (LZSS) ✅ V2 +
  **`Compress.compress` (LZSS) ✅** (mismo framing, round-trip verificado en host,
  commit `53bbd7b`). Pendiente:
  - **`compress` native — PENDIENTE, depende de "AOT — casts"** (sección Compilador).
    Hoy corre INTERPRETADO (necesita `byte()` chequeado y el AOT no emite casts). Es la
    dirección rara (comprimir para guardar/enviar) → interpretado vale; `decompress` sí
    es native-able (solo copia byte→byte).
  - **Multi-fichero / Archive — DIFERIDO por complejidad** (Eduardo gateó "si no es muy
    complejo"). El códec es buffer-a-buffer (dst + capacidad); un packer multi-entrada en
    BP puro pide o un builder OO con estado o colecciones (List&lt;byte[]&gt;) → cruza el
    "thin container". Retomar si surge necesidad real (p.ej. empaquetar recursos GUI).
  - **Futuro `deflate`-lite** (LZSS → +Huffman). Aditivo, sin tocar `decompress`
    (hay código V2 que depende — principio 7).
- **Json (CORE hecho, 22-jun).** `samples/Json.bp` (librería completa: jerarquía
  JsonValue, parser recursivo, serializador, escapes) promovido a `bpstdlib/Json.bp`
  (commit `48220e4`) + ergonomía: `writeJsonPretty` (indentado) + getters
  `getString/getInt/getFloat/getBool/getObject/getArray` y `getXxxOr(key, def)` en
  JsonObject. Smoke test → `samples/JsonDemo.bp`. **Paridad dual-VM byte-idéntica
  verificada en host** — al hacerlo se cerró un gap: `parseFloat` (builtin 2) portado
  a la VM-C (`strtod`→f32; antes era "portable diferido" de H10). Pendiente: embeber
  en firmware (tanda de placa); futuro UI-via-Json (mapear al árbol Component, NO V3).
- **Freq (HECHO + verificado en placa 22-jun).** `bpstdlib/Freq.bp`:
  `Freq.Meter` (frecuencímetro por conteo de flancos sobre `Pulse.Counter`) —
  `measureHz(windowMs)` / `measureHzAvg(windowMs, n)` / `maxHz(windowMs)` (techo por
  contador 16-bit). Sample `samples/FreqDemo_Dk2.bp` (puente PB8→PB7). **Verificado en
  la DK2** (sin reflash — el firmware ya tenía el pulse backend; el IDE subió las deps):
  1 kHz→1000, 500 Hz→500 (avg 501), 2 kHz/100 ms→2000. **Mejoras futuras (NO V3, idea de
  Eduardo 22-jun):** (a) combinar **dos contadores de 16 bits → 32 bits** (cascada de
  timers) → techo mucho mayor sin acortar la ventana, menos overflow; (b) **frecuencias
  < 1 Hz** y más precisión vía **input-capture** (medir el periodo entre flancos,
  freq = 1/periodo) — el conteo por ventana ya cubre bien la ALTA frecuencia, el periodo
  cubre el extremo BAJO/sub-Hz; necesita backend HW (timer en modo IC, firmware).

## ⚙️ Compilador / lenguaje (ampliación selectiva — "eventos y poco más")

### H7 — HECHO (20-jun): 4 cambios de lenguaje, dual-VM byte-idéntico

**Cerrado**: operador `^` (`7a426af`), `eval` (`e73b329`), separador `_` (`7caaea4`),
continuación de línea por operador colgante (`dcc95bd`). compat GATE + suites verdes;
IDE reempaquetado. **`^` y `eval` necesitan reflash** para la DK2 (tocan las VMs); el
separador y la continuación son solo frontend. **`^` en native/AOT: diferido** (ver
nota en el bullet del operador). Detalle de diseño original abajo (se conserva):

- **Operador `^` de potencia** — `x^2` = x al cuadrado. Token `^` **LIBRE** (en BP
  el XOR es la keyword `xor`, no `^` → sin colisión). **Toca:** lexer (token nuevo),
  parser (binario, **asociativo por la DERECHA** y de **precedencia ALTA** — por
  encima de `*` y del unario menos: `-x^2` ⇒ `-(x^2)`, `2^3^2` ⇒ `2^(3^2)`),
  semántico (tipos/resultado), emisores (MivmEmitter + AotCEmitter + JvmEmitter) y
  **las 2 VMs byte-idéntico**.
  - **Cómputo por tipos (encargo de Eduardo):** `float ^ integer` → **multiplicación
    repetida** / exponenciación por cuadrados (exacto y **parity-safe**; exponente
    negativo ⇒ `1/x^|n|`). `float ^ float` → **`exp(n·ln(x))`** (dominio x>0; `ln`
    indefinido para x≤0 → decidir error vs NaN). `int^int` → decidir (mult repetida;
    ¿overflow a long?). Resultado: `float^*` ⇒ float.
  - **RIESGO DE PARIDAD (el grande):** el camino `float^float` usa transcendentales —
    `Math.exp/log` (Java) vs `exp/log` (C) pueden diferir en el último ULP ⇒ resultado
    NO byte-idéntico. El camino `float^integer` (mult) SÍ es byte-idéntico. Mitigación
    a probar: computar en **double y estrechar a float32** (las diferencias de ULP en
    double suelen desaparecer al redondear a float) y **verificar con el arnés de
    paridad**. **Ya existe el builtin `POW`** (expuesto vía `Math.pow`): revisar su
    impl/paridad actual; `^` float^float puede **bajar a él** si es byte-idéntico en
    ambas VMs. Aditivo (principio 7): opcode nuevo (p.ej. `POW_F`/`POW_I`) o reuso de POW.
  - **HECHO (20-jun, commit `7a426af`):** `^` en el INTÉRPRETE (ambas VMs, dual-VM
    byte-idéntico) con IPOW/LPOW/DPOW (0xAC/0xAD/0xAE). **PENDIENTE — `^` en AOT/native**
    (Eduardo 20-jun: "dejarlo y anotar", retomar con el AOT): `AotCEmitter` no emite `^`
    → una `native function` con `^` **ABORTA la compilación** (el `native` exige AOT-able).
    El camino .mod (interpretado) funciona perfecto. Fix futuro: helpers
    `aot_ipow/aot_lpow/aot_dpow` (misma lógica que los opcodes) + emisión en AotCEmitter,
    O degradar AOT no-soportado a interpretado (best-effort) en vez de abortar. Mismo
    hueco para el backend JVM (JvmEmitter).

- **`eval("expresión")` — evaluador de expresiones en runtime, estilo BASIC
  (LIMITADO y SEGURO).** NO es el `eval` de Python (sin código arbitrario, sin acceso
  a variables/funciones) → **seguro por construcción** (gramática cerrada). Ampliable
  poco a poco en versiones futuras.
  - **Alcance inicial:** solo operaciones básicas (`+ - * /`, paréntesis, números;
    y `^` cuando exista). Nada sofisticado.
  - **Toca:** builtin `eval(s)` en las 2 VMs → cada VM lleva un **mini-parser de
    expresiones** (descenso recursivo) con resultado **byte-idéntico** (misma
    aritmética y mismo tipo numérico). Superficie de paridad NUEVA (un evaluador en
    Java + otro en C) → mantenerlos alineados; el grueso (aritmética) es parity-safe.
  - **Decidir:** tipo de retorno (¿double/float?), error de sintaxis ⇒ ¿RuntimeError
    BP cazable? **Futuro (NO ahora):** variables/funciones/llamadas, siempre dentro de
    una gramática cerrada (jamás el modelo Python ilimitado).

- **Separador `_` en literales numéricos** (Eduardo, 20-jun) — `100_000`,
  `0xDEAD_BEEF`, `0b1111_0000`, `1_000.5`, `2_000L`. **SOLO el scanner**
  (`Lexer.scanNumber`): consumir `_` únicamente si va **entre dígitos** (dígito antes
  y después) y descartarlo antes de `parseLong`/`parseDouble` → así `_100`/`100_` no
  chocan con identificadores. Aplica a decimal/hex/binario/float (todos pasan por
  `scanNumber`). No afecta a nada más (ni .mod, ni VMs).

- **Continuación de línea — DECIDIDO: (A) operador colgante** (Eduardo, 20-jun). La
  continuación IMPLÍCITA dentro de `()`/`[]` YA existe (#249/L5); para el resto, **si la
  línea termina en un operador binario / coma / `:=` / `.` / `and`/`or` (un "token
  continuador"), la sentencia continúa en la siguiente — SIN carácter especial**
  (estilo Kotlin/Go). **Implementación:** extiende la MISMA supresión de newline de
  #249 — al fin de línea, mirar el último token significativo; si es un continuador, no
  emitir el terminador de sentencia. Definir el conjunto exacto de tokens-continuadores
  (operadores binarios + `,` + `.` + `:=`/`=` + `and`/`or`/`xor`/`mod`/`shl`/`shr` + el
  `^` nuevo). Descartados `\` final y `_` final (este chocaría con el separador nuevo).

- **AOT — casts (`byte()`/`int()`/`float()`/`long()`/`double()`) — PENDIENTE; repaso
  del AOT en la fase IDE** (Eduardo, 22-jun). El `AotCEmitter` **no maneja NINGÚN nodo
  cast** → cualquier función con un cast cae a interpretado (verificado: 0 coincidencias
  de cast/conversión en `AotCEmitter.java`). **Bloquea `compress` native** (`byte()`
  chequeado 0..255) y todo `native` con conversiones. Fix: emitir el narrowing/conversión
  en C (para `byte()`, i32→u8 con la semántica chequeada de `I32_TO_U8`; el helper de
  store a `byte[]` ya existe, #193). Mismo paraguas que `^` en AOT (arriba) → cuando
  toque el IDE, **repaso del AOT de una pasada**.
- **#169 — AOT cross-module sin puente del intérprete** (MEJORA de rendimiento;
  hoy FUNCIONA vía `call_bp` + warning). Diseño en `AOT_CROSS_MODULE.md`.
- **Layout compacto de narrow** (L10 follow-up): `byte[]`/`int16[]` y globales
  narrow con storage real (hoy viven como i32; la maquinaria de la VM ya existe).
- **Fixed arrays `tipo[N]`** en campos de clase y en `native` (hoy error honesto;
  locales y de módulo ✅ en V2).
- **N-pubvar-warn**: `public var` de módulo se ignora en silencio → avisar
  ("usa property/const"). La gramática ya documenta la regla.
- **N5 v2 / condvar real** en `SyncList` (hoy spin-poll de 1 ms).
- **Strings multilínea** (Eduardo, 20-jun: *"estaría bien, pero no urgente"*) — **NO
  para H7**; futura mejora de lenguaje. Literal de cadena que abarca varias líneas
  (triple comilla u otra sintaxis a decidir), útil para JSON/plantillas embebidas.
  Junto con interpolación de strings, candidatas a una tanda de lenguaje posterior.

## 🛠️ IDE / herramientas

### Fase 1 del IDE — comodidad + AOT automático (decidida 24-jun; host-Java, SIN reflasheo de firmware)

El usuario es quien más toca el IDE → comodidad + que el AOT "just works".

**AOT automático desde el IDE:**
- Targets Fase 1 = **ARM: RP2350/Pico + STM32**. **ESP32 (S3/P4) → V4** (port del loader `.mdn` a Xtensa/RISC-V).
- **Elección de target POR PROYECTO** (sección `aot` en `.bpbuild`: `enabled`/`target`/`optLevel`/`flags`).
- Rutas de toolchain **por máquina** (IdePrefs: auto-detect PATH + carpetas estándar + override) + **panel Settings→AOT** (extensible para "lo que vaya surgiendo").
- Pipeline (FrmMain `doRunOnDevice` ~1765): detectar `function native` → `AotCEmitter` (Java in-IDE) → gcc del target (ProcessBuilder) → `MdnPack` (Java) → subir `.mdn` con el `.mod`.
- **Native NO AOT-able → WARNING + continúa interpretado (NO aborta).** "Para eso el `.mod` siempre se genera" (Eduardo).

**Comodidad del editor/IDE ✅ HECHA (commit `85ceb7d`, verificada por Eduardo 24-jun):**
- ✅ **Barra de botones superior** (FrmMain): izq archivos/proyecto · centro ejecución · der
  edición; botones redondeados (RoundToolButton) + separación + feedback.
- ✅ **Comentar/descomentar** con `//` (respeta indentación) y **Indentar/des-indentar**
  (4 esp / tab) sobre la SELECCIÓN. (Atajos de teclado Ctrl-/ · Tab/Shift-Tab = pendiente fácil.)
- ✅ **Árbol del micro (PicoExplorer) que se cortaba** → jSplitPane2 con resizeWeight 0.2 +
  divisor al 35% → se lleva el grueso del alto y crece al maximizar.
**Bloque B = AOT automático desde el IDE ✅ HECHO + VERIFICADO EN PLACA (commit `c65d915`, 24-jun;
proyecto de prueba `samples/aottest`):** Run on Device de `Fibo` desde el IDE → compiló →
`[aot] Fibo.mdn ✓ (1 thunk, 68 B)` → subió `.mod`+`.mdn` → en placa **`fiboNative(30)*5` = 1.095 s
vs `fibo(30)*5` interpretado = 104.15 s → ~95× speedup**, `total` idéntico (4160200), exit 0.
- ✅ **B1 — refactor a métodos invocables:** `AotMain.emitAotC(src,outDir,mdn)` y
  `MdnPack.pack(o,mdn,mod)` (sin `System.exit`; el CLI sigue funcionando igual).
- ✅ **B2 — config:** sección `aot` { `enabled`, `target` } en `.bpbuild` (por proyecto, parser
  en `BpBuild`); `aotGccPath` + `aotBpgenvmDir` en `IdePrefs` (por máquina; vacío = autodetect).
- ✅ **B3 — pipeline + wiring:** `AotBuild` (nuevo) hace emit→`arm-none-eabi-gcc`→`MdnPack`
  por cada módulo con `native` bajo `sourceDir`; intermedios `.c/.o` a **`<projectDir>/target/`**
  (decisión de Eduardo), `.mdn` final a `outDir` (PicoExplorer ya lo sube). Flags fijos del
  target `arm` = Cortex-M33, idénticos a `build_mdn.sh`. Autodetect de gcc y de bpgenvm-c.
  Llamado desde `runAotPass()` en `doRunOnPico`, en el hilo de fondo (gcc es subproceso).
  Plantilla de New Project incluye la sección `aot`.
- ✅ **B4 — UI:** menú **Project → "AOT (toolchain)…"**: edita gcc + raíz bpgenvm-c con botones
  *Detectar*; muestra el estado AOT del proyecto activo.
- ✅ **Degrade grácil:** native no AOT-able / sin toolchain / gcc-error → WARNING en consola y se
  ejecuta interpretado; **nunca aborta el Run** (el `.mod` siempre se genera).
- *(Nota: la elección `optLevel`/`flags` por proyecto del diseño quedó fuera — sólo `enabled`/`target`;
  ampliable en V4 si hace falta.)*

**Fase 2 del IDE (más adelante, "algún añadido más"):** TBD. Candidato fuerte: **repaso del AOT de una pasada** (casts `byte()/int()…` + `^` en native → más funciones AOT-ables). (AOT ESP32 = V4.)

---

- **N-ide-aot-button — AOT integrado en el IDE**: hoy el `.mdn` se compila por CLI
  aparte; que el Run/Build lo haga, con detección del toolchain ARM y flags por
  target. Liga con #258.
- **IDE multiplataforma (Windows + Linux)**: migrar el puerto serie
  `purejavacomm → jSerialComm`, lanzador `.sh`, rutas/asunciones de toolchain.
  Workstream propio, independiente del lenguaje/VM.
- **describePc remoto** (el call-stack del debugger muestra `PC=<n>`; falta
  `req: describePc(pc)` en el wire) · **multi-run en el daemon** (hoy 1
  ejecución/proceso) · **aprovisionamiento del device** (subir stdlib/`BpVM.cfg`).
- **Ver el diagnóstico de reset desde el IDE (H10 → IDE)** *(idea de Eduardo, 21-jun)*:
  mostrar en el botón **INFO** (o un comando/panel "Diagnóstico") la **causa del último
  reset** + **boot count** + el **rastro de migas** (`markAt(0)` = causa original).
  La fontanería del runtime YA está hecha en H10 (builtins `Pico.resetCause`/`bootCount`/
  `markCount`/`markAt`, ids 201-205; verificados en placa). Falta solo el lado IDE: que
  el reply de `INFO` del wire (`BpvmClient.requestInfo` → Map; el handler INFO del
  firmware) incluya esos campos y el panel los pinte → diagnosticar un cuelgue de campo
  **sin escribir un .bp**. Encaja con la "observabilidad de bring-up" (más abajo).

## 🧱 Portabilidad / multi-micro

- **#258 — Árbol familia/micro/placa + imagen por familia + target por proyecto**
  (`board_desc` data-driven, detección por wire). Base en `micros/`. **Habilita el
  rollout de kits gráficos** (incluido el ESP32-P4, que es RISC-V → port nuevo).
- **Capa de port + observabilidad de bring-up** (miga de pan en RAM de backup,
  boot por etapas con checkpoint, self-test de port, `PORTING.md`, "encoger el
  rojo"). Diseño completo en `V3_IDEAS.md` §6. Madura justo antes de bajar el GUI
  al micro.
- **#138 — Multiplexar USB CDC** en streams distinguibles.
- **P-adc-8ch**: `Adc.bp` valida `0..3`; exponer el nº de canales por placa
  (`Pico.adcChannels()`, como `gpioCount()`) para los 8 del RP2350B. Liga con #258.

## 🔴 PENDIENTE ACTIVO — H13.1 Forms en P4: bloqueado por `Json.mod` RANCIO en placa (2026-06-26)

**Síntoma:** formdemo / cualquier GUI cuyo Gui importe Json → en el P4 `exit 10`
(RuntimeError MUDO). GuiColorDemo, que ANTES iba, también petó al añadir `import Json`
a Gui (paso 3 de H13.1).

**Causa raíz CONFIRMADA** (vía `idf.py monitor`, que muestra el stderr del device):
```
[bpvm-c link] símbolo no resuelto: 'Json.JsonValue#asObject#15' (módulo='Gui')
```
El `Json.mod` del FS del P4 está **rancio**: su `JsonValue.asObject` NO está en el slot 15.
Gui.mod (compilado contra el Json.bpi actual) referencia `Json.JsonValue#asObject#15`; el
linkAll del device no lo encuentra → `BPVM_ERR_RUNTIME`. MISMO patrón que el skew de
`.mod` de stdlib en placa (método OO que peta en placa y no en host por `.mod` viejo).

**Pruebas (todas hechas esta sesión):**
- Bisección: Gui SIN `import Json` (loader stubbeado) → GuiColorDemo va en P4. CON Json → peta.
- Host VM-C (incl. `--mem=131072` = los 128 KB del P4) corre GuiColorDemo Y formdemo con el
  Gui+Json ACTUALES → bpstdlib coherente: Gui ref `#asObject#15` == Json export `#asObject#15`.
- Descartado: slot-shift de Component, builtin 207, buffer de la VM (128 KB basta en host),
  stack/FS por sí solos.

**Por qué persiste el Json rancio:** el PUT del IDE (`PicoExplorer.putIfChanged`) salta la
subida si `deviceSize == localSize && sentCrc[remote] == localCrc`. `sentCrc` solo sabe lo
que ESTE IDE envió en ESTA conexión — NO el contenido REAL del device. Un Json dejado por
una sesión/estado anterior (mismo tamaño, o nunca sobrescrito) se da por bueno. Además Json
se rutea a /app (no /lib) → puede haber copias en ambos.

**FIX (3 partes):**
1. **✅ HECHO (26-jun, `ef2499a`→`d0cffd6`→`64b7839`) — CRC por fichero en el PUT.** El LS del
   firmware reporta `"crc":<u32>` por entrada (`bpvm_crc32` = `java.util.zip.CRC32`, paridad
   verificada) y el IDE (`putIfChanged`) salta el PUT sii el CRC REAL del device coincide con el
   local → mata el rancio de CUALQUIER fuente, no solo de la sesión actual. Fallback al heurístico
   anterior (tamaño+sentCrc) si el firmware no manda crc. Las 3 familias reportan crc; BpIde
   compila + fat-jar reconstruido; falta el reflash de Eduardo para el end-to-end.
2. **Rutear Json (y toda dep de stdlib) → /lib** (no /app): en `FrmMain` la decisión libDeps
   debe mandar a /lib cualquier dep cuyo origen sea el stdlibDir (no solo EMBEDDED_CORE_MODS
   + "Gui"). (Ya estaba a medio leer en `FrmMain.java:2108-2119`.)
3. **✅ HECHO (26-jun, `48f80b9`) — "missing lib X" al wire.** `bpvm_t.link_error[160]`
   (bpvm_internal.h) + `bpvm_link_set_error()` en `link.c` en los 3 paths de no-resuelto
   (imports + class fixup L2v3 + eh-class fixup), distinguiendo **"falta la lib 'X'"** (dueño no
   cargado) de **"lib 'X' presente pero no exporta 'sym' (version vieja?)"** (rancia); nombre de
   lib derivado del símbolo cualificado. Accessor público `bpvm_link_error(vm)` (bpvm.h). Los 4
   reporters de RUN (host `test/main.c`, esp32 `repl_esp32.c`, pico `repl_v1.c`, stm32
   `stm32_repl.c`) lo mandan en el EXITED (status `LINK_ERROR` + `errorMessage`); el IDE ya pinta
   `errorMessage` → sin cambios. **Verificado en host VM-C** (ambos casos) + **arnés V2 verde**.
   miVM NO necesita cambio: auto-resuelve la stdlib desde `stdlibDir`, así que el "missing lib" es
   un problema del DEVICE (carga de su FS) — justo donde picó. **✅ VERIFICADO EN EL P4 (26-jun):**
   el IDE muestra `exit 10 (falta la lib 'Pwm' (la usa 'PwmCount'; simbolo 'Pwm.__init'))`. → se
   acabó el `exit 10` mudo, en hardware real.

**Workaround inmediato (sin código):** borrar el `Json.mod` del device (/app y /lib) por el
explorer → Run formdemo → el IDE sube uno fresco → slots casan → Forms debería ir. (Si el
explorer no puede borrar, hace falta el fix #1.)

**Estado del árbol:** Gui restaurado a la versión con Json (bf46585); FormDemo sample
pristino; backup de la bisección en `C:\tmp\wip\gui_json_bisect\`. Commits de esta sesión:
`15fb3ee` (resolveDeviceDeps transitivo: sube Json como dep de Gui), `6611592` (B: fallback
/app en readFile). Repro host listo en `C:\tmp\repro_gui\` (`bpgenvm-c --mem=131072 *.mod`).

### ✅ RESUELTO (26-jun)
**Causa real:** no era "stale en device por putIfChanged" — el **IDE subía** un `Json.mod`
RANCIO desde el `out/` del proyecto (`samples/out/Json.mod` del 18-may, 12351 B, `asObject`
en otro slot que el #15) porque Json NO estaba en `EMBEDDED_CORE_MODS` → `resolveDeviceDeps`
no le aplicaba la protección "stdlib-first" (que sí tenían los CORE desde el 13-jun).
**Fix `e73c44c`:** cualquier módulo que viva en `stdlibDir` (tiene su `.bp`/`.mod` allí) se
resuelve de bpstdlib PRIMERO → siempre el Json fresco (16519 B, `asObject#15`); el
`putIfChanged` lo sobrescribe en el device (tamaño distinto → DEL+PUT). **Verificado en el
P4:** los handlers de botón/checkbox disparan a los métodos de la ventana. **Pulidos `799d23d`:**
(1) el form llena la pantalla (Gui.Window toma el tamaño del screen + `load()` hace que el
widget raíz del `.win` llene la ventana salvo tamaño explícito); (2) `.win`/`.slots` se editan
DENTRO del IDE (`looksLikeTextFile`). Pendiente: confirmación VISUAL del layout en placa.
**Las 3 mejoras de robustez del plan original** (CRC device-side en LS/STAT, rutear stdlib→/lib,
subir el detalle de linkAll al wire = "missing lib X") quedan como PULIDO OPCIONAL (no bloquean;
el bug raíz ya no existe).

### Board descriptor data-driven — ampliar a periféricos y GRÁFICOS (Eduardo, 26-jun)
Hay **distintas variantes de P4** (y de otras familias) con diferencias en periféricos →
los counts (ADC/PWM/…) deben salir del **descriptor de placa** (board_desc + `/sys/board.json`),
no hardcodeados. **Paso 2d (en marcha)** cablea `ADC_CHANNELS`/`PWM_SLICES` al descriptor como
ya hace `gpioCount()`.
**Pendiente (MÁS ADELANTE, liga con #258):** llevar también los **GRÁFICOS** al descriptor —
si la placa **tiene pantalla o no**, **resolución**, **pines de control**, **transporte**
(DSI/RGB/SPI…). Hoy el P4 lo tiene hardcodeado en `gui_display_dsi.c`; debería ser data-driven
por placa (board.json del ESP32 = parte del #258). Eduardo: "lo de los gráficos para más adelante".
