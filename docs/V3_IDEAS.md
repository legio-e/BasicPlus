# V3 — cajón de ideas (informal)

> **Aún no es formal.** Es el cajón de V3: cuando se cierre V2 y se abra V3 se
> organiza en condiciones (como se hizo con V2). Iniciado 2026-06-07; la sección
> GUI se desarrolló el 2026-06-13 (charla Eduardo + Claude).

## Principios de V3

- **V2 es la base.** El lenguaje se mantiene (se añaden *eventos* y poco más),
  se corrigen bugs y se amplía de forma selectiva. Eduardo contento con el
  lenguaje tal cual.
- **Objetivo cabecera: una librería GUI.** No el único, pero el que manda.

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

## Infraestructura que el GUI arrastra (movida desde V2)

- **Dual-core** (#153): un núcleo a lo gráfico para un rendimiento equilibrado.
  Con él viaja el **fix de B1** (la race solo aparece con paralelismo real; en V2
  mitigada a 1 worker, el device single-core es inmune).
- **Mapa de memoria configurable por PSRAM** (#225) y **FS grande** (#229):
  relevantes para framebuffers y recursos del GUI.
