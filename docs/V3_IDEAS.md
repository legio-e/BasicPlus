# V3 — cajón de ideas (informal)

> **Aún no es formal.** Es el cajón de V3: cuando se cierre V2 y se abra V3 se
> organiza en condiciones (como se hizo con V2). Iniciado 2026-06-07; la sección
> GUI se desarrolló el 2026-06-13 (charla Eduardo + Claude).

## Principios de V3

- **V2 es la base.** El lenguaje se mantiene (se añaden *eventos* y poco más),
  se corrigen bugs y se amplía de forma selectiva. Eduardo contento con el
  lenguaje tal cual.
- **Objetivo cabecera: una librería GUI.** No el único, pero el que manda.

## Mapa de hitos V3 (H0 — definido 18-jun-2026)

Con H5 cerrado (GUI interactiva en silicio real), Eduardo fija el **alcance de V3**
— la "2ª parte de H0" que estaba aplazada hasta cerrar el GUI:

- **H6 — Controles GUI (cierra la parte gráfica).** Añadir los **widgets más
  comunes de LVGL** en las **3 VMs** (miVM + VM-C + STM32). Alcance DELIBERADO:
  **~60–70 % de LVGL — "lo más interesante, NO todo LVGL".** **Criterio candidato
  de selección (Eduardo):** los controles **comunes a Swing ∩ LVGL** — garantiza
  que ambos backends (Swing en miVM, LVGL en VM-C/micro) los pintan → sin huecos
  de paridad, y acota a controles estándar bien entendidos (cae cerca del 60-70 %).
  Se afina al arrancar H6, con algún juicio en los bordes (p.ej. `arc`/`roller`/
  `msgbox` de LVGL sin equivalente Swing directo, y viceversa). Desarrollo en la
  DK2, paridad por dumpTree. Con esto la parte gráfica queda terminada.
- **Repaso estilo V2:** lenguaje + librerías estándar + IDE — pulir/ampliar según
  lo que pida el uso (como los hitos de consolidación de V2).
- **TCP/IP:** ampliar — **WiFi/lwIP en placa** (#145, marcado [v3]) + lo que falte
  de `Net`. (+ "quizá algún tema más".)
- **Hardware:** soporte a **alguna plataforma/familia adicional**.
- **Bugs (transversal):** ir tapándolos según aparezcan.
- **Cierre de V3:** capítulo de **repesca** de los widgets que no entraron en H6 +
  **rollout cross-family en bloque** (GUI en los kits con pantalla; un kit a fondo
  primero, pruebas en placa de todos juntas y al final) + docs/publicación.

**Filosofía (Eduardo):** el GUI fue eficiente → queda holgura para varias cosas,
pero **no inflar** — añadir lo valioso y parar a tiempo; lo que sobre se deja para
**V4**. "Un buen fin, no arrastrarse." Orden de los bloques intermedios: flexible
(H6 primero, cierre al final); numeración formal H6/H7/… al pasar esto a
`V3_ROADMAP`.

## 1. GUI gráfica (objetivo cabecera)

> **DECISIÓN CONSOLIDADA — H0 de V3 (16-jun-2026, charla Eduardo + Claude).**
> El GUI se modela EXACTAMENTE como los buses (I2C/SPI/UART): un contrato
> `Gui.*` (subconjunto reducido de la API de LVGL) **idéntico en las 3 VMs**,
> con backend por plataforma — **VM-C (host+micro) → LVGL**; **miVM → Java puro
> reimplementando ese subconjunto** (NO un puente JNI Java↔LVGL: descartado por
> dependencia/caja negra, choca con el stack propio). **Paridad de
> comportamiento**, hecha OPERATIVA con un **volcado textual del árbol de
> widgets** que las 3 VMs emiten byte-idéntico → la verificación cruzada
> sobrevive, ahora sobre el *modelo de la UI* en vez del `print` (los píxeles
> quedan fuera: LVGL y Java rasterizan distinto y ahí nunca hubo paridad).
> Eventos por **upcall C→BP** (generalizar `call_bp` #210). Orden de trabajo:
> miVM → VM-C host (LVGL+SDL) → micro (LVGL+TFT). Dial para H1: cuánto del
> *layout* vive en la capa portable BP (cuanto más, más idéntico el volcado).
> **Esto PRECISA y prevalece sobre lo que sigue** (redactado antes con
> "backends de render intercambiables" genéricos + bake-off Swing/JavaFX).

> **TRANSPORTE PANTALLA↔MICRO — encuadre (16-jun, charla H2).** Ortogonal al
> contrato `Gui.*` (que NO tiene ni un pin — correcto). LVGL rasteriza a un
> framebuffer y habla con el panel por DOS callbacks: **`flush(area, pixels)`**
> (envía la región al display — *aquí vive el transporte*: SPI / RGB-paralelo /
> MIPI-DSI / HSTX-DVI) y **`read_touch() → (x,y,pulsado)`** (táctil). Eso es lo
> ÚNICO que cambia entre placas. Arquitectura = el patrón de los buses: facade
> **`bpvm_display`** (flush+read) + **config por placa** (`board.json`:
> transporte, pines, resolución, timings, color, rotación = DATO, no código) +
> reuso por **TIPO de transporte** (un backend por SPI/RGB/DSI/DVI, no uno por
> panel → principio 5). En el HOST no existe (se blittea a una ventana del SO);
> aparece SOLO en la fase micro (H4+). Por eso H2 (el contrato) cierra sin tocarlo.

### Decisión de arquitectura: un API `Gui.*`, varios backends de render

El contrato BP (`Gui.Screen`, `Gui.Button`, `Gui.Label`, eventos…) se define
**una sola vez**. Detrás, **render backends intercambiables**:

```
            un solo API  Gui.*   (contrato, en BP)
           /             |               \
      Java (PC)       LVGL+SDL          LVGL+TFT
   (miVM, ventana)   (VM-C, PC)       (VM-C, placa)
```

- **Placa:** LVGL sobre el display (objetivo real). LVGL = C, vive en
  `bpgenvm-c`. Se *envuelve* LVGL (no se reinventa): subconjunto razonable,
  incremental.
- **PC, VM-C:** el **simulador SDL de LVGL** enlazado en el host → "desarrolla
  en el PC, despliega en el micro" se mantiene también en la VM-C.
- **PC, miVM:** render en **Java puro** detrás del mismo `Gui.*`. Backend
  inicial **Swing/Java2D** (en el JDK, cero deps nativas). **Si conviene,
  migrar a JavaFX — que esa elección NO limite el diseño** (Eduardo, 13-jun):
  el `Gui.*` es el contrato estable y el toolkit Java de debajo es un detalle
  reemplazable. (JavaFX: scene graph acelerado, CSS, mejor para GUIs ricas y
  táctil; coste: ya no viene en el JDK → arrastra OpenJFX + sus nativos de
  render. Sigue siendo portable y sin LVGL. Swing: cero deps, más simple. Se
  decide al hacerlo, sin reescribir el API.)

### La verificación cruzada (paridad) revive — a nivel de comportamiento

El invariante dual-VM se **reformula** para GUI: no hay paridad de píxeles
(Java2D/JavaFX y LVGL rasterizan distinto), pero sí **paridad de lógica /
comportamiento** — mismo programa BP, mismos widgets, mismo estado, misma
secuencia de eventos en ambas VMs. Verificable con una suite que corra la app en
miVM y en VM-C y compare *eventos/estado*, no la pantalla. Las apps **sin** GUI
mantienen la paridad completa (byte-idéntica) intacta.

### La viga maestra: el upcall C→BP

Todo el modelo de eventos cuelga de **que C pueda llamar a una función/método
BP** (LVGL recibe el callback en C y "sube" al intérprete a ejecutar el handler).
Ya prototipado: el puente `call_bp` del AOT (#210, native→BP) hace justo eso; se
generaliza.

- Con el upcall, los **eventos por listener OO** (implementar una interfaz
  `Gui.ClickListener` / subclasear y override `onClick`) funcionan **sin tocar
  el lenguaje** — interfaces + dispatch virtual ya existen en V2. *Este* es el
  "eventos y poco más": casi nada nuevo en el lenguaje.
- **Funciones-como-valor** (§8, `CALL_INDIRECT`): azúcar ergonómico *opcional*
  encima (`btn.onClick(miFuncion)`); fase 2, no prerrequisito.
- El mismo upcall sirve a HW (interrupción→handler), timers y red: **§8/§9 es la
  pieza foundational de V3 y el GUI es su primer cliente.**

### Upcall en miVM (H3.4) — decisión 17-jun: modelo de INTERRUPCIÓN

> **DECISIÓN — upcall `onClick` en la VM-Java (17-jun, charla Eduardo + Claude).**
> En miVM el upcall NO es "otro hilo entra a la VM" — eso sería el bug clásico:
> BP corriendo en el EDT de Swing y en el worker a la vez sobre el mismo `mem[]`.
> Es una **INTERRUPCIÓN** (encuadre de Eduardo):
>
> - El `ActionListener` de Swing (hilo **EDT**) **no ejecuta BP**: solo **publica**
>   "pulsado el handle H" en una cola y hace `notify`. Como levantar un bit de IRQ
>   pendiente. Es el ÚNICO cruce entre hilos (cola + lock); `mem[]`/`tc` solo los
>   toca el worker.
> - El worker BP, aparcado dentro del builtin `__guiRun` (que ES el lazo de
>   eventos), despierta, saca el handle y ejecuta `onClick` **como llamada
>   anidada** sobre la pila de su propio `tc`; al `RET` vuelve al lazo (RTI).
>   `onClick` corre SIEMPRE en el worker, jamás en el EDT.
> - Más seguro que una ISR real: el flujo principal (`Main`) no está a medias de
>   una instrucción, está aparcado en UN punto conocido (`run()`). La reentrancia
>   sale gratis: el `pc/sp/bp/cs` del `runOnContext` exterior viven como locales
>   de Java → la pila de llamadas de Java ES el "salvado de contexto" de la IRQ.
>
> **Mecanismo — la VM NO conoce ningún slot (refinado 17-jun, a instancia de
> Eduardo).** Fijar el slot de `onClick` por posición es frágil: depende de la
> herencia (los slots 0/1 del `Object` raíz), del orden de declaración y de las
> properties (sus accesores también ocupan vtable). En su lugar, **el compilador
> resuelve `onClick` por nombre**: `Gui` añade una función normal
> `public function __guiDispatch(self: Obj)` cuyo cuerpo es `self.onClick()` — el
> compilador la emite como `INVOKE_VIRTUAL` con el slot CORRECTO (override +
> herencia respetados, sea cual sea el layout). La VM resuelve la ENTRADA de esa
> función por NOMBRE (`resolveGlobal("Gui.__guiDispatch")`, igual que resuelve
> `Core.RuntimeError`; las funciones de módulo están en `globalSymbolTable`,
> ModuleManager.java:426) y la llama **anidada** pasándole el `objptr`, con **PC
> de retorno centinela = 0** (`mem[0]`=`THREAD_EXIT`, técnica de `THREAD_START`)
> para cerrar el run. **Cero opcodes nuevos, cero slots hardcodeados.** Un único
> builtin nuevo, `__guiBindClick(handle, self)`, registra `handle→objptr`.
>
> **Slot vacío / sin handler** (Eduardo): como el dispatch es `self.onClick()`
> puro, un widget que NO reescribe `onClick` ejecuta el `onClick` no-op heredado
> de `Gui.Obj` → un clic no hace nada (correcto). No hay número de slot que
> mantener ni caso de "slot ausente": lo resuelve el compilador como cualquier
> otra llamada a método virtual.
>
> **¿Una función o varias? → UNA por widget** (el `onClick` virtual). Coherente con
> la decisión OO ya tomada ("subclasea y reescribe `onClick`"): un objeto, un
> método, un slot. La multiplicidad se hace DENTRO del handler (el `onClick` llama
> a lo que quiera). NO una lista de listeners estilo Swing/`lv_obj_add_event_cb` —
> más maquinaria y choca con el modelo OO. Si algún día se quiere, se añade
> ADITIVAMENTE encima (`addClickListener`) sin romper el override. En el borde
> VM↔backend también es **un** punto de entrada: el backend llama a un único
> `dispatchClick(handle)` de la VM.
>
> **Disciplina (la que la analogía predice):** el handler debe ser **corto y no
> bloqueante**, como una ISR. Con 1 worker, un `onClick` que duerma, espere un
> `Mutex` o llame a otro `run()` colgaría el bombeo. Suficiente para H3; se
> revisita si hiciera falta.
>
> **Binding:** el backend mantiene `handle → objptr`; el ctor de cada widget
> registra su `this` tras crear el handle. (Fino, pendiente: tratar esos objptr
> como raíces de GC, o confiar en que el programa BP mantiene viva la referencia
> durante `run()` — para H3, los widgets viven en locales de `Main`.)

### Camino crítico

1. **Upcall C→BP robusto** (generalizar `call_bp`; serializar contra el lazo de
   LVGL, que no es thread-safe).
2. **Driver de display + táctil** (de los "un par de drivers más":
   ST7789/ILI9341, o el DSI del F769I-DISCO, + el táctil). Prerrequisito físico.
3. **Wrappers OO `Gui.*`** sobre el subconjunto LVGL, incrementales.
4. **Lazo de eventos** `Gui.run()` (sobre la cola que ya insinúan `SyncList` + el
   comm-task SMP). Con dual-core (#153), un núcleo dedicado a lo gráfico.
5. **Dos imágenes** (con/sin LVGL): los dispositivos sin pantalla no pagan RAM.
   GUI = feature de clase PSRAM/SDRAM (F769I-DISCO 4" táctil + 16 MB SDRAM ya
   disponible; Metro + PSRAM).

### Kits gráficos disponibles (validación cross-family)

Eduardo tiene **varios kits con pantalla, cada uno de una familia de fabricante
distinta** → banco de pruebas ideal para que el GUI sea de verdad cross-family
(lo que cambia por placa es el driver de display/táctil; el API `Gui.*` y LVGL
por encima, no). **Empezar por uno** — el **STM32F769I-DISCO** (4" táctil DSI +
16 MB SDRAM) es el mejor candidato — y, si va bien, **ampliar al resto**: misma
mecánica que el despliegue de micros en V2 (Pico → ESP32 → STM32). Alimenta el
#258 (árbol familia/micro/placa).

**Kits por familia (confirmados por Eduardo, 13-jun):**
- **ST** — **STM32F769I-DISCO**: 4" táctil DSI + 16 MB SDRAM. *Cabeza de puente.*
- **Espressif** — **kit ESP32-P4 con pantalla incluida** (MIPI-DSI). OJO: el P4
  es **RISC-V**, familia distinta del ESP32-S3 (Xtensa) de V2 → es un *port
  nuevo* de firmware (target ESP-IDF propio, imagen distinta — ya anotado para
  #258), pero gran plataforma LVGL (Espressif empuja LVGL + `esp_lvgl_port`).
- **Raspberry / RP** — **Metro RP2350 + puerto HSTX → placa DVI de Adafruit**:
  salida a un **monitor DVI/HDMI** (estilo PicoDVI/HSTX), no un TFT pegado.
  Modelo de "pantalla externa"; LVGL lo trata igual (framebuffer → flush).

Tres **transportes de display distintos** (DSI / MIPI-DSI / HSTX-DVI) bajo un
mismo `Gui.*` = prueba cross-family fuerte. El asiento de la abstracción son los
dos callbacks de LVGL: *flush* (al display de cada placa) + *read* (táctil). Lo
que cambia por kit son esos dos; el API y los widgets, no.

### Orden de trabajo (la senda que ya funcionó: VM-Java → VM-C → VM-micro)

Metodología de V1/V2: probar primero en Java (iteración rápida + debugger),
luego portar a la VM-C de host, luego al micro. Aplicada al GUI:

0. **(Decisión previa) Bake-off Swing vs JavaFX** para el backend Java del GUI —
   barato, decide con evidencia. JavaFX parte con ventaja (scene graph *retained*
   ≈ árbol de LVGL → más fácil mantener la paridad de comportamiento). **Matiz:
   la VM-Java NO se "migra"** (es headless, hoy no tiene GUI) — se le *añade* un
   backend `Gui.*` nuevo, en JavaFX o Swing según el bake-off; el intérprete no
   se toca. El único *migrar* de verdad es el **IDE** (Swing→JavaFX).
   **[DECIDIDO 13-jun: NO migrar de momento]** El IDE se queda en **Swing** — ya
   funciona y migrar "por si acaso" es trabajo evitable (Eduardo: "todo lo que
   sea ahorrar trabajo, bienvenido"). Se reconsidera solo si más adelante hace
   falta el mismo toolkit para un preview/diseñador de GUI embebido en el IDE.
1. **miVM (Java)**: definir el API `Gui.*` + modelo de eventos. El upcall aquí es
   trivial (Java llama a su propio intérprete, sin JNI) → el sitio más barato
   para fijar la semántica del API y los eventos, con debugger.
2. **VM-C host**: LVGL + simulador SDL; aquí el upcall C→BP de verdad
   (generalizar `call_bp` #210). Arranca la paridad de comportamiento miVM↔VM-C.
3. **VM-micro**: LVGL sobre el TFT — Discovery primero (driver display+táctil,
   SDRAM, dual-core #153); luego ampliar a los demás kits (cross-family).

### Disciplina / coste

- `Gui.*` **pequeño** (mínimo común que LVGL y el backend Java puedan honrar) —
  restricción sana, muy "diseñar para el piso".
- Varios backends = mantenerlos en sync; se mitiga con la suite de comportamiento.
- Dial de diseño: cuanta más lógica de **layout** viva en BP portable, más
  paridad de comportamiento (a cambio de aprovechar menos el layout de LVGL).

### H5 — bring-up del GUI en el micro: DK2 (análisis 18-jun, a refinar)

**Cabeza de puente = STM32U5G9J-DK2** (la placa que tiene Eduardo), **no** el
F769I-DISCO que insinuaba el plan del 13-jun. Motivo: la DK2 es **misma familia U5
(Cortex-M33)** que el Nucleo-U575 de V2 → todo el `port/` STM32 (wire v1, FS, REPL,
GPIO, mods embebidos, platform) y el AOT Thumb-2 (#251) se reusan casi tal cual; el
F7 sería rehacer el port de cero. Camino más barato (principio 5).

**Specs DK2 (data brief ST):** 5″ **800×480**, panel **RGB 24-bit por LTDC** (NO
MIPI-DSI), **táctil capacitivo** (I²C), STM32U5G9ZJ con **3 MB SRAM + 4 MB flash
on-chip**, GPU2D **NeoChrom** + LTDC, OSPI flash, STLINK-V3EC, USB-C HS. **SIN
PSRAM** (esa es la DK1). → Un framebuffer 800×480@16bpp ≈ **750 KB cabe en SRAM
interna**; doble buffer 16bpp (~1.5 MB) también. No hace falta RAM externa para el
GUI — justo por eso ST vende el U5G9 como MCU "de pantalla". El #225/PSRAM no
aplica; el mapa de memoria es de SRAM interna.

**Lo que se reusa (el premio de haber hecho H4 en host primero):** el contrato
`Gui.*`, el modelo `gui.c`+dumpTree (paridad byte-idéntica), el render bajo
`BPVM_LVGL`, el upcall onClick y la matriz de 3 builds (lean/GUI/LVGL) — **todo**.
El micro solo cambia el *pegamento display+táctil*: en vez de
`lv_sdl_window_create`+`lv_sdl_mouse`, LVGL se cuelga de **LTDC (framebuffer en
SRAM) + táctil I²C**.

**Lo nuevo (el trabajo real):**
1. Board target DK2 (proyecto CubeMX propio): reloj, linker 4MB/3MB, VCP UART,
   LEDs, **mapa de memoria** (framebuffer en SRAM). = #258 hecho concreto: 2º
   target STM32 conviviendo con el Nucleo.
2. Driver display: init LTDC (CubeMX) + display LVGL (draw buf + flush_cb → fb).
3. Táctil: controlador I²C → indev LVGL → alimenta el MISMO upcall click→onClick.
4. Facade **`bpvm_display`** (flush/read), diferida desde H2: aterriza aquí; en
   `port/`, impl LTDC en el dir de la DK2, reusable por SPI/DSI/DVI.
5. Eje de build `BPVM_GUI`/`BPVM_LVGL` (ya en el Makefile del host) llevado al
   firmware CubeIDE; liblvgl para Thumb-2.

**Descomposición (bottom-up, verificar cada capa — la senda de siempre):**
- **H5.0 — base del DK2, SIN GUI.** El firmware VM lean corriendo en la DK2: board
  target + reloj + wire VCP + LED + stdlib embebida. Éxito = "Run on Device" en la
  DK2, Hello.mod imprime por el wire igual que el U575. Prueba el reuso del port U5
  y el board target ANTES de tocar pantalla.
  - *Aperitivo (experimento de Eduardo):* flashear el `.elf` del U575 en la DK2 como
    diagnóstico de 5 min — ver qué sobrevive. Esperado: reloj/VCP/LED difieren →
    casi mudo, pero mide el hueco. 1er sospechoso si cuelga: el reloj.
- **H5.1 — display (LTDC + LVGL, sin táctil).** `GuiClickDemo` RENDERIZANDO en el
  panel de 5″ (clic sintético `__guiClick` para ver el label cambiar, como headless).
  dumpTree sigue byte-idéntico (el modelo no cambia).
- **H5.2 — táctil.** I²C → indev LVGL → upcall. Éxito = tocar el botón real →
  onClick → label cambia (análogo a "Eduardo pulsó el botón SDL → onClick").
- **H5.3 — cierre.** Mapa de memoria documentado y holgado; 3 builds compilan para
  la DK2; paridad verde; check on-device. Cierra el tramo micro del camino crítico
  GUI → alimenta la "2ª parte de H0" (alcance).

**Decisiones a refinar con Eduardo:**
1. **Proyecto CubeMX nuevo** (`stm32/U5g9_dk2/`) en paralelo al `Nucleo_u575b/`,
   generado para la placa exacta (ST trae la DK2 en CubeMX con LTDC/táctil/OSPI ya
   cableados). NO retrofitear el proyecto Nucleo.
2. **CubeMX/BSP para el init, pero el pegamento LVGL es NUESTRO** (flush_cb, indev,
   facade `bpvm_display`) — principio 6 (envolvemos LVGL, no nos casamos). **NO
   TouchGFX** (el default de ST); seguimos con nuestra LVGL v9.2.2 vendorizada →
   paridad con el host.
3. **Framebuffer simple primero:** un buffer en SRAM, 16bpp (RGB565), refresh
   parcial, render software. Optimizar después (doble buffer, NeoChrom/GPU2D, 24bpp)
   solo si hace falta. Piso barato.
4. **Mapa de memoria:** 3 MB. Repartir framebuffer (~750 KB) + draw buf LVGL + heap
   VM (¿subir de 128 KB?) + FS + stacks. Mapa explícito de entrada.

**Fuera de alcance de H5** (disciplina): rollout cross-family (ESP32-P4 RISC-V,
Metro+DVI) — después, y en parte V4 (lo decide la "2ª parte de H0"); aceleración
NeoChrom — optimización, no bring-up; árbol #258 data-driven completo — H5 añade un
2º target hardcoded, generalizar puede esperar.

### H5.1 — display (LTDC+LVGL en la DK2): diseño (18-jun, a refinar)

**El gran hallazgo:** en `gui.c`, bajo `BPVM_LVGL`, el código que crea/actualiza
los `lv_obj` (label/button/align/text…) es **independiente de plataforma** → se
reusa TAL CUAL en el micro. Lo único atado a SDL son **3 funciones**:
`lvgl_ensure_init` (ventana SDL + ratón), `bpvm_gui_lvgl_pump`
(`lv_timer_handler`+`SDL_Delay`) y `bpvm_gui_lvgl_window_open`. Esa es justo la
costura del **backend de display** (la facade `bpvm_display` de H2): H5.1 =
sustituir SDL por **LTDC** en esas 3; el resto del render ya está.

**Lo nuevo (micro):**
- **Framebuffer en SRAM** (~750 KB a RGB565, array estático; cabe de sobra en los
  3 MB; el LTDC del U5 hace DMA desde SRAM interna).
- **Display LVGL por API cruda** (`lv_display_create` + draw buffer + `flush_cb`
  que copia la zona sucia al framebuffer) — en host lo envuelve
  `lv_sdl_window_create`; aquí a mano (hay ejemplos de ST).
- **Cablear la capa LTDC** al framebuffer: CubeMX la dejó con `FBStartAdress=0` y
  RGB888 → fijar nuestra dirección + formato + enable.
- **Tick** = `HAL_GetTick`; **pump** = `lv_timer_handler`.
- **Build:** LVGL vendorizada como carpeta fuente enlazada (como `port`/`src`) +
  `lv_conf.h` propio del micro (LV_COLOR_DEPTH 16, sin SDL, DMA2D off de momento)
  + símbolos `-DBPVM_GUI -DBPVM_LVGL`.

**Decisiones a refinar:**
1. **Color: RGB565 (16bpp, ~750 KB).** Piso amigable; la paridad es por dumpTree,
   NO por píxeles → el host sigue a 32bpp y el micro a 16bpp sin romper nada.
   Reconfigurar la capa LTDC (CubeMX la puso RGB888).
2. **Costura backend:** factorizar las 3 funciones SDL de `gui.c` a un backend de
   display (host=SDL / micro=LTDC); el código de widgets sigue COMPARTIDO. Es
   `bpvm_display` (flush ahora; read=táctil en H5.2).
3. **LVGL en CubeIDE:** carpeta fuente enlazada + `lv_conf.h` del micro (vs
   liblvgl.a prebuild). Enlazada = consistente con port/src.
4. **Alcance H5.1 = solo render, sin táctil.** Éxito = `GuiClickDemo` dibujándose
   en el panel de 5″ + un `__guiClick` sintético que cambia el label (visible).
   Táctil GT911 = H5.2.
5. **Lazo GUI_RUN en el micro:** `lv_timer_handler` + tick + drenar cola de clics
   → upcall, **y** sondear el wire (KILL/HELLO) para que el IDE pueda parar (como
   `stm32_run_poll_cb`).

**Fuera de H5.1:** táctil (H5.2), aceleración DMA2D/GPU2D (optim.), doble
buffer/tear-free (optim.).

### H5.2 — táctil GT911 + KILL en `Gui.run()` + wire RX por IRQ (18-jun, HECHO en placa)

`GuiClickDemo` en la DK2 con **dedo real** → `onClick`, y **Stop (Ctrl+F2) mata el
GUI** sin reset. Tres piezas:

1. **Táctil GT911 → indev LVGL.** Datos sacados del **BSP de ST** (no del PDF):
   `Drivers/BSP/STM32U5G9J-DK2/stm32u5g9j_discovery_ts.{c,h}` + `Components/gt911/`
   en el repo CubeU5 1.8.0 del disco → **GT911 en I²C2** (`hi2c2`, PF0/PF1),
   dirección **0xBA** (8-bit; 0x5D en 7-bit), registros de 16 bits: estado en
   **0x814E** (bit7 = frame listo, bits3:0 = nº puntos), punto 1 en **0x8150**
   (XL,XH,YL,YH). **Coords directas**: `GT911_MAX_X/Y_LENGTH = 800/480` +
   `Orientation = TS_SWAP_NONE` → la fórmula del BSP es identidad, el GT911 ya
   reporta 0..799/0..479 = rango del display. `read_cb` por **polling** sobre
   `hi2c2` (HAL_I2C_Mem_Read; ACK escribiendo 0 en 0x814E); INT (PE5) NO se usa.
   Indev pointer → el toque entra por el MISMO camino que el clic sintético
   (`lvgl_click_cb` → `inject_click` → upcall). Reusamos el protocolo de ST ligado
   a nuestro `hi2c2`, sin arrastrar el BSP. Todo en `port/gui_display_ltdc.c`.
2. **KILL durante `Gui.run()`.** El scheduler no corre quanta mientras el builtin
   `GUI_RUN` bombea, así que su lazo (`builtins.c`) **polea `vm->poll_cb` entre
   frames** (el mismo del scheduler); al recibir KILL rompe → push+return →
   `BPVM_KILLED` limpio.
3. **Wire RX por IRQ → ring (la pieza grande; idea de Eduardo).** El RX del wire
   STM32 era **lectura directa del registro, sin buffer software**: si el lazo no
   sondea >~700µs (la FIFO HW son 8B) se pierden bytes por overrun. El bombeo de
   LVGL deja el UART sin atender varios ms → se perdían los primeros bytes del KILL
   (la FIFO ya estaba activada y NO bastaba — 8B≈700µs ≪ hueco de ms). Fix: la
   **IRQ de RX del USART1** (DK2) drena la FIFO a un **ring de 256B** (SPSC
   monocore); `getchar` lee del ring → no se pierde nada aunque el lazo no sondee.
   Robustece **TODO el wire** (PUT/RUN/bulk), no solo el GUI. El pump del GUI ahora
   **duerme con `__WFI`** (sin busy-spin). Todo bajo `#if BPVM_BOARD_DK2`
   (`stm32_wire.c/.h`, `stm32_repl.c`, `board.h`, `it.c` del proyecto); Nucleo
   intacto (path directo). **Lección:** el wire STM32 era el único port sin buffer
   de entrada (Pico = USB CDC bufferizado; ESP32 = driver UART con ring). Para el
   próximo port STM32 con tráfico a ráfagas: RX por IRQ+ring desde el principio.

**Fuera de H5.2:** ring por IRQ en el Nucleo (cuando toque), GET (TX) por IRQ
(no hace falta hoy), bajar la latencia del Stop por debajo del SysTick.

### H5.3 — cierre del tramo H5 (18-jun, HECHO)

H5 cerrado: la GUI interactiva corre en la DK2 (render + táctil + KILL), verificada
en placa por Eduardo. Cierre:

- **Mapa de RAM (SRAM 3008 KB) — holgado:** framebuffer RGB565 800×480 = 750 KB ·
  drawbuf parcial (48 líneas) = 75 KB · pool LVGL (`LV_MEM_SIZE`) = 64 KB · heap
  VM (`s_vm_mem`) = 128 KB · arena FS en RAM = 96 KB · stack = 16 KB · ring RX +
  varios < 1 KB → **~1.1 MB usados, ~1.9 MB libres (63 %)**. Sitio de sobra para
  doble-buffer/tear-free o GUIs más ricas (optim. futura). Flash: el firmware LVGL
  entra en los 3968 KB de código (compila+flashea+corre); FS en los últimos 128 KB
  (`BOARD_FS_FLASH_ADDR`).
- **Las 3 builds compilan** (chequeo anti-podredumbre del camino sin-GUI, en el
  host de referencia): `make` (lean) · `make GUI=1` (modelo) · `make LVGL=1`
  (render) → las tres OK. En el micro la DK2 corre la build LVGL; lean/GUI se
  ejercitan en el host para que no se pudran.
- **Estrategia de aquí en adelante (decisión de Eduardo, 18-jun):** seguir a fondo
  en la DK2; el **rollout cross-family** (probar en placa otros kits) se hace en
  BLOQUE y más adelante, NO por feature — si no, no se termina nunca. El código se
  mantiene correcto para todas las familias (`#if BPVM_BOARD_*`), pero la prueba
  física se difiere.

**H5 = objetivo cabecera de V3 cumplido:** misma `Gui.*` byte-idéntica en las 3 VMs
y, en silicio real, se pinta + responde al dedo + se para limpio. Siguiente
inflexión: **"2ª parte de H0"** (alcance de V3 + mapa de hitos), aplazada hasta
cerrar el GUI — ahora toca.

### H6 — modelo de objetos + `Component` (diseño, 19-jun)

Antes de los widgets se fija la JERARQUÍA (Eduardo: la estructura antes que los
controles, para no rehacerla):

**`Object` (lenguaje) → `Component` (base visual, nuestra) → controles (widgets).**

- **`Object`** = base UNIVERSAL del lenguaje, **implícita** (clase sin `extends`
  mapea a `java/lang/Object` en `JvmEmitter`; `Object` es tipo usable — Collections;
  Core.bp: "Object → Exception → …"). **Norma: todo desciende de Object salvo
  `Thread`.** NO creamos `Gui.Object`: `Component` simplemente no escribe `extends`
  → cae bajo Object, como `Exception`.
- **`Component`** = base de TODO widget; reemplaza al `Obj` actual
  (`Screen/Panel/Label/Button` → `extends Component`; GuiClickDemo igual). Contrato:
  - **`lvglId`** (property) — id del objeto espejo (lv_obj / JComponent). **Lo asigna
    el MODELO** (contador `g_next_handle` en gui.c / ídem miVM) → idéntico en las 3
    VMs → seguro para paridad. Renombra el `handle` de hoy.
  - Geometría como **propiedades** `x, y, width, height` — denominador común
    (LVGL `set_pos`/`set_size` · Swing `setBounds`); el set empuja al backend; el
    MODELO es la verdad del `dumpTree` (que gana x,y,w,h → la paridad cubre la
    geometría, hoy solo cubre `align`). `align(ancla,dx,dy)` = azúcar que calcula x,y.
  - `setBgColor(c)` · **`refresh()`** (= Swing `repaint` / LVGL `lv_obj_invalidate`;
    render-only → no toca el modelo → no afecta paridad; los setters ya
    auto-repintan, refresh es el "forzar redibujo" explícito) · `clean()` · `delete()`.
  - Eventos: `onClick()` + **`onChange()`** (nuevo), override-ables (upcall).
- **Norma reforzada (#143): sin variables públicas** — lo público es **`property`**
  (el backing field queda privado en su slot; en la .bpi viajan solo los accesores).
  El `Obj.handle` de hoy era `public var` → el refactor lo corrige.

**Modelo de eventos (para los widgets de valor):** se generaliza el dispatch del
upcall — `__guiDispatch(self, kind)` con `kind ∈ {CLICK, CHANGE}` → `onClick()` o
`onChange()`. El handler lee el valor con getters (`getValue()`/`getText()`/
`getSelected()`/`isChecked()`), NO por el upcall. El `dumpTree` gana un campo
valor/estado por nodo → la paridad cubre los nuevos widgets.

**Orden de implementación:** la BASE primero (`Component` + geometría + refresh +
onChange + dumpTree) en **miVM (Swing)** y verificar por dumpTree → VM-C (host) →
micro; LUEGO los widgets encima.

**Set de H6 — CERRADO (19-jun):**
- **Widgets:** label, button, panel, checkbox, switch, slider, bar, dropdown, list,
  table, tabview, textarea, spinbox, image, msgbox + **led** + **keyboard** (ata a
  textarea; en host el teclado físico vale).
- **Scroll:** capacidad de `Component`, propiedad `scrollDir` = NONE/HOR/VER/BOTH,
  **default NONE** — lo controla el programador, NO AUTO.
- **Fuentes:** catálogo baked de 6 — SMALL 14 / NORMAL 18 / LARGE 28 / TITLE 40
  (regulares Montserrat, ya prebuilt en la LVGL vendorizada) + NORMAL_BOLD 18 /
  TITLE_BOLD 40 (LVGL NO trae negrita → convertir Montserrat-Bold UNA vez, offline).
  Swing espeja por id. Texto enorme / fuente extra → runtime (`tiny_ttf`), repesca.
- **Imágenes:** PNG desde archivos/resources en runtime (`lodepng` + puente del FS;
  ambos vendorizados, hoy off) — iconos/logos pequeños (poca RAM). Botón con icono =
  `Button` + `Image` hijo (composición; no hace falta `imagebutton`). Sub-tanda al
  FINAL del tramo. JPG → repesca.
- **Repesca:** radio, calendar, arc, roller, chart, scale, buttonmatrix, spinner,
  imagebutton, tileview, win, menu, line, canvas, span, anim/lottie, JPG, fuentes runtime.

## 2. Lenguaje (se mantiene; "eventos y poco más")

- §8 callbacks / función-valor (`CALL_INDIRECT`) — opcional, ver arriba.
- §9 eventos: HW interrupción→evento BP (Modelo B) + eventos GUI, sobre el upcall.
- Nada más previsto. Eduardo contento con la base del lenguaje.

## 3. Librerías estándar (mejorar / ampliar)

- **TCP/IP**: V2 deja un cliente simple. V3: WiFi en placa (lwIP, #145) +
  **servidor** (listen/accept), más API, ¿TLS/UDP/DNS? (Con un servidor HTTP
  mínimo, una "GUI web" sería un camino alternativo al LVGL para placas con red.)
- **Compresión**: subir de LZSS a deflate-lite (LZ77 + Huffman) y/o un formato
  Archive multi-fichero. Aislado, fácil de encajar.
- Lo que vaya surgiendo.

## 4. Hardware / dispositivos

- Un par de drivers más, según necesidad. El display + táctil del GUI ya cubre
  buena parte de la cuota.

## 5. Entorno IDE

- Base sólida; retoques pequeños y continuos.
- **Esfuerzo Windows + Linux**: Swing ya es cross-platform; los riesgos reales
  son el **puerto serie** (migrar purejavacomm → jSerialComm, más portable) y la
  fontanería (lanzador `.sh` además del `.bat`, rutas, asunciones de toolchain).
  Workstream propio, independiente del lenguaje/VM.
- Horizonte (stretch): un **diseñador visual de GUI** en el IDE que emita BP.

## 6. Portabilidad (núcleo + capa de port) y observabilidad de bring-up

> Tema fundacional y cross-cutting: abarata cada port nuevo y ataca el "momento
> oscuro" (el micro no arranca y no se ve por qué). **Habilitador directo del
> rollout de kits gráficos** (3 bring-ups nuevos, uno RISC-V). Charla 13-jun.
> Meta-regla del proyecto: *analizar → programar*; esto es "analizar el análisis".

**Cuándo hacerlo (timing en el orden de trabajo §1):** justo **antes de bajar a la
fase VM-micro** (paso 3). En la fase Java (miVM) NO hace falta — el debugger lo ve
todo; en VM-C host, casi tampoco. Es en el **descenso al micro** donde gana su
sueldo (Eduardo, 13-jun: "para la parte gráfica en Java no hace falta; en los
micros, toda ayuda es bienvenida"). Construirlo justo entonces, no antes.

### Estado de partida (medido, no impresión)

El **núcleo es ya independiente de la máquina**: **0 `#ifdef` de plataforma** en
interp/bpvm/loader/link/heap/builtins/exceptions/scheduler/threading/mdn_loader
(grep 13-jun). Habla solo con **facades finos**, patrón `bpvm_platform.h` (handles
opacos, backend elegido al enlazar: `platform_pthread.c` host / `platform_freertos.c`
micro). Ya hay facade de: threading+tiempo, comms (`bpvm_comm.h` + `comm_common.c`),
FS (`bpvm_fs.h`), net (`bpvm_net.h`), periféricos. **El host (PC) es un port más**
(`*_host.c`) → el *port de referencia*, depurable.

### Lo que falta de la capa de port

- **Boot + provisión de memoria** es el único concern SIN facade — vive ad-hoc en
  cada `main.c`/`board_desc`, y no es casualidad que sea ahí donde duele el
  bring-up. → darle el mismo trato (un `bpvm_boot.h`/`bpvm_mem.h` fino).
- **`PORTING.md` + esqueleto de port**: "para un micro nuevo, implementa estos
  facades + un `board_desc`". Convierte el coste por port en una checklist
  decreciente (objetivo: esfuerzo de migración cada vez menor).
- **Test de conformidad de port**: valida cada facade ANTES de arrancar la VM.
- Regla: **traer todo en el host primero** (donde se ve); en el micro solo difiere
  la capa de port → si falla en micro y va en host, el bug está en el port
  (pequeño), no en el núcleo (grande, probado).

### Observabilidad de bring-up — "ver dentro del momento oscuro"

Causa raíz del black-box: lo que necesitas para depurar (las comms) es justo la
parte difícil de portar → pez que se muerde la cola. Escalera de diagnóstico,
**complementaria al log de flash** (que ya es útil), ordenada por *qué sobrevive*:

| Mecanismo | Sobrevive a | Coste | Para qué |
|---|---|---|---|
| LED / código de parpadeo | (solo en vivo) | 1 pin | señal mínima de etapa; limitado |
| UART de debug 1-vía (levantada la 1ª) | (solo en vivo) | 1 pin, sin USB/FS | *ojos* aunque USB/FS/VM rotos |
| **Miga de pan en RAM de backup/RTC** | **reset + cuelgue** | ~8 words | "¿dónde morí?" legible al rearrancar |
| Backup VBAT (STM32) | reset + power-off | pila | idem, sobrevive a apagón |
| Log en flash | reset + power-off + historia | FS arriba | la historia completa, por el wire |

- **Boot por etapas con checkpoint**: `RESET → RELOJES → SRAM → PSRAM/SDRAM → FS →
  TRANSPORTE → VM_CARGADA → RUNNING`. Cada etapa escribe su id en la miga de pan;
  al colgarse, la última marca = dónde murió.
- **La miga de pan (idea de Eduardo, 13-jun): RAM de backup del RTC.** Casi todos
  los micros tienen un RTC con RAM/registros en el dominio always-on que sobreviven
  a reset y a cuelgue. Mismo facade, backend POR FAMILIA:
  - **STM32**: registros de backup (`RTC->BKPxR`) y/o BKPSRAM — dominio VBAT,
    sobrevive incluso a power-off con pila.
  - **RP2350**: `watchdog->scratch[0..7]` (8 words always-on; el bootrom ya los usa
    para el motivo de reboot) — sobrevive a reset; o SRAM con magic number.
  - **ESP32 (S3/P4)**: RTC slow memory (`RTC_NOINIT_ATTR`) — sobrevive a reset y
    deep-sleep.
  Contrato universal satisfacible: ~8 × 32-bit (magic + etapa + causa/PC del fallo
  + contador de reset).
- **Combo matador para los CUELGUES: miga de pan + watchdog.** Un cuelgue silencioso
  (ni LED ni log ayudan) → el WDT resetea → al rearrancar, la miga dice "colgué en la
  etapa X". Convierte un cuelgue mudo en un reboot con causa.
- **Por qué refuerza el log, no lo sustituye** (Eduardo): el log necesita el FS
  arriba (driver de flash OK) → en un bug de boot temprano o del propio flash (¡el
  bug JEDEC!) el log está ciego justo cuando más lo necesitas. La miga de pan no
  necesita NADA (ni FS ni flash, solo escribir un registro) → cubre exactamente la
  ventana pre-FS que el log no alcanza. Log = historia rica; miga = último suspiro
  instantáneo, legible aunque no funcione nada más. Son complementarios.
- **SWD/JTAG** como verdad absoluta (openocd/gdb por placa) + `HardFault_Handler`
  mínimo que vuelca PC/causa a la miga antes de resetear.
- **Self-test de port**: en un port nuevo, lo PRIMERO — test de RAM/PSRAM/flash/
  loopback del transporte por la UART de debug, antes de la VM. Separa "HW/init
  mal" de "lógica de VM mal".

### Síntesis: observabilidad DENTRO del contrato de port

`bpvm_port_boot_stage(id)` + `bpvm_port_crumb_*()` como parte del contrato,
enrutados a LED + UART de debug + RAM de backup. Un port nuevo **nace con ojos**,
no se apuntala con prisa ya metidos en el pozo. El bug JEDEC de esta sesión ("no
enumera, no sé por qué", resuelto con 4 capturas y triangulación) se habría
señalado solo: *"murió tras el primer save del FS"*.

### Pagar doble: fiabilidad en campo (V3)

Más allá del bring-up, la miga de pan sirve al dispositivo desplegado:
- **Distinguir "morí arrancando" de "estaba corriendo y peté".** Un flag "sano"
  que se marca al llegar a RUNNING estable; un fallo lo borra y anota el PC. Al
  rearrancar sabes si el bug fue de bring-up (etapa X) o de runtime (crash con la
  VM ya en marcha) — dos bichos distintos.
- **Contador de reset → guardia anti-bucle del autorun.** Si la miga cuenta "5
  resets en 10 s", el firmware detecta bucle de crash y cae a **modo seguro**: NO
  relanza la app que peta, espera al IDE. Hace robusto el "dispositivo autónomo"
  de V2 (autorun + Stop).

### Investigación pendiente (V3): qué sobrevive en cada familia

Hallazgo (web + a confirmar en placa, 13-jun): el **RP2350 tiene dominio
always-on** (POWMAN: `BOOT[0..3]`, `watchdog->scratch[0..7]`) y **la SRAM no se
borra en reset** → sobrevive a reset/sleep. PERO **no parece tener un dominio con
backup de pila** estilo VBAT: el "always-on" es "siempre encendido *mientras el
chip tiene alimentación*". → **quitar la alimentación lo borra, y una pila de
botón probablemente NO ayuda en la Pico2/Metro** (no hay pin de backup como en
STM32). La pila de botón es cosa de **STM32** (VBAT + BKPxR/BKPSRAM reales).
*Implicación:* en RP2350, para que la miga sobreviva a un reset manual, usar
**reset templado** (RUN/watchdog/soft), no quitar la alimentación; si se quita,
el único recurso es el **log en flash** (sí sobrevive a power-off).

Matriz de test a rellenar en placa (V3):

| Tipo de reset | ¿sobrevive scratch / BOOT / noinit-RAM? |
|---|---|
| soft reset (`watchdog_reboot`) | esperado SÍ |
| timeout del watchdog | esperado SÍ (para eso existen los scratch) |
| pin RUN / botón de reset | **a comprobar** (puede ser más "duro") |
| power cycle (quitar alimentación) | esperado NO |
| + pila de botón | RP2350: probablemente no aplica · STM32: SÍ |

Probe mínima (V3): firmware que escribe un magic + contador en scratch/noinit-RAM,
provoca cada tipo de reset y reporta si el magic sobrevivió. Cierra el diseño del
facade `bpvm_port_crumb_*` por familia. **Nada de esto es V2** (freeze): es diseño
e investigación para V3.

### Cómo encoger el rojo/ámbar (análisis 13-jun — "el tamaño del rojo = coste de migrar")

Principio: casi cada caja roja es **lógica portable + un primitivo de silicio**.
Separándolos, la lógica sube a verde y el rojo queda en un puñado de funciones.

| Caja hoy | Veredicto |
|---|---|
| Comms (framing) ámbar | framing del wire v1 = lógica de bytes → **VERDE**; rojo = `read_bytes`/`write_bytes` (ya casi en `comm_common`) |
| FS facade ámbar + Flash backend rojo | lógica del FS (árbol /sys //lib //app, ops) → **VERDE**; rojo = *block device* `read`/`prog`/`erase` (estilo littlefs) |
| Arranque rojo | la *secuencia* por etapas → **VERDE** (mismo state-machine que la observabilidad de §6); rojo = primitivas `clocks_init`/`mem_init` |
| Memoria rojo | heap+GC ya **VERDE**; rojo = `port_get_heap(&size)`; el *tamaño* = **DATO** (BpVM.cfg) |
| Periféricos backend rojo | pokes de registros = rojo irreductible; pines/nº GPIO/bus = **DATO** (board.json) |
| Transporte rojo | ya mínimo (2 funcs); no sube (es el HW) |
| Port FreeRTOS rojo | **un** fichero para todos los FreeRTOS (ya es el modelo) + capa **ARM-CM común** |
| AOT / .mdn ámbar | depende de la ISA (ARM), no de la placa; está bien donde está |

Tres mecanismos:
1. **Sacar lógica común al núcleo** (rojo→verde): comms, FS, boot, memoria — separar
   lógica del primitivo. Arriba portable; abajo una lista corta de funciones.
2. **Variación en DATO, no código** (rojo/ámbar → fichero, fuera del código): el
   `board_desc` data-driven hace que una Pico vs una Metro sea un JSON, no un `#ifdef`
   ni un build distinto (#258/#134/#225 — "una imagen por familia").
3. **Compartir por ARQUITECTURA, no por placa** (rojo→ámbar compartido): RP2350 y
   STM32 son ambos Cortex-M33 → comparten Thumb-2/AOT, AAPCS, port ARM de FreeRTOS,
   NVIC/SysTick/fault. Mucho "rojo" de STM32 y RP2350 es código ARM común → capa
   por-arquitectura. El rojo por-vendor queda en: relojes, controlador de flash,
   USB/UART, controlador de memoria, pin mux.

**Irreductiblemente rojo** (honestidad): init de relojes (RCC/PLL/QMI), pokes de
periféricos, init de PSRAM/SDRAM, primitivo de transporte. Pequeños y acotados — el
objetivo es que el rojo sea "toca estos registros para este chip", y nada más.

**Métrica:** "reducir el rojo" = **hacer `bpvm_port.h` más corto**. El largo del
contrato *es* el coste de portar. **ROI:** abstraer ahora ahorra en cada port después
(merece la pena con 3+ ports y el P4 RISC-V en camino); para un concern de un solo
port, no. Mover el FS a block-device cambia comportamiento → la **paridad** lo cubre.

### VM.3 — wire v1 sobre TCP en el P4 (diseño 23-jun, charla Eduardo + Claude)

Tras VM.1/VM.2 (la VM-C completa corre en RISC-V), el P4 deja de ejecutar un
módulo embebido y pasa a **target de desarrollo**: el IDE sube y ejecuta apps
por red. Es la 2ª zona de riesgo de Eduardo (comms; *"entre FS y comunicaciones
es donde aparecieron problemas"*) → se hace por sub-pasos.

**El diseño confirma en placa la tesis del "rojo/ámbar" (tabla §6, fila Comms):**
*el framing del wire v1 es lógica de bytes portable (VERDE); lo único específico
del transporte son `read_bytes`/`write_bytes` (rojo, mínimo).* La migración lo
demuestra de forma tajante:

- **`repl_esp32.c` se reutiliza TAL CUAL** (compilado en el build del P4): todo
  el dispatcher — HELLO/INFO/PUT/GET/LIST/DEL/RUN/KILL, resolución de deps desde
  /lib, OUTPUT/EXITED, poll de KILL — es agnóstico del transporte. CERO cambios.
- **Lo único nuevo: `esp32p4/main/wire_v1_tcp.c`** — implementa la misma API
  `wire_v1.h` (6 funcs de I/O de bytes + builders JSON) sobre **sockets lwIP**.
  El P4 es **servidor** (el IDE conecta a `192.168.2.2:3333`); el S3/Pico eran
  esclavos sobre UART/USB-CDC.
- **Truco clave:** el `accept()`/reconnect vive **dentro de `wire_v1_recv_line`**
  → el bucle `repl_esp32_run()` (recv_line→dispatch) funciona idéntico, sin
  enterarse de conexiones/caídas. En el camino "poll" de un RUN no se re-acepta
  (una caída no cuelga la VM). Builders JSON = copia de `wire_v1.c` (misma
  política deliberada S3↔Pico: "cada firmware lleva su copia para no acoplar los
  transportes").
- **Lado IDE: cero código** para el hito funcional — `BpvmBackend` ("VM (TCP v1)")
  ya habla wire v1 sobre socket; basta apuntarlo a `192.168.2.2:3333`.

**Compromiso conocido:** al reutilizar `repl_esp32.c` verbatim, INFO/HELLO
reportan identidad "esp32s3" (cosmético). Se arregla en el **sub-paso de pulido**
(parametrizar identidad de placa + target "ESP32-P4" dedicado en el IDE), que
toca el S3 y se re-verifica aparte — NO ahora, para no desestabilizar la placa
que funciona. *Backlog de de-dup:* extraer un `wire_dispatch` compartido por
firmwares es tentador, pero el repo eligió a propósito duplicar por placa
(boards independientes); se deja como limpieza post-V3.

**Sub-pasos (verificación escalonada sobre UN flasheo):** 3a META (HELLO/INFO —
prueba socket+framing+reconnect) → 3b FILES (PUT/GET sube un .mod) → 3c TERMINAL
(RUN/OUTPUT/EXITED/KILL = "Run on P4"). El código es una pieza (dispatcher
probado); se valida en ese orden porque HELLO ya ejercita todo el I/O nuevo.

## Infraestructura que el GUI arrastra (movida desde V2)

- **Dual-core** (#153): un núcleo a lo gráfico para un rendimiento equilibrado.
  Con él viaja el **fix de B1** (la race solo aparece con paralelismo real; en V2
  mitigada a 1 worker, el device single-core es inmune).
- **Mapa de memoria configurable por PSRAM** (#225) y **FS grande** (#229):
  relevantes para framebuffers y recursos del GUI.
