# H7 — Metro RP2350B: PSRAM, más pines, protocolos nuevos (arranque 2026-06-05)

Fase H7 de V2 (tras H1 tipos · H2 strings · H3 GC · H4 ESP32 · H5 Object · H6 Debugger).
Dedicada casi en exclusiva a la **Adafruit Metro RP2350** (chip **RP2350B**,
8 MB PSRAM, 48 GPIO), pero abre temas que sirven a todo el proyecto. Mismo
método: **inventario → decidir alcance → tareas, una a una.**

## Principios de arquitectura (decisiones, usuario 2026-06-05)

> **1. Un solo firmware para Pico y Metro.** El mismo binario debe arrancar y
> funcionar en ambas placas, **con o sin PSRAM**. La detección es en runtime;
> nada de builds divergentes salvo el board target (flash/boot2/pines).
>
> **2. La PSRAM es opcional, no obligatoria.** Sin PSRAM (Pico) → layout actual
> (128 KB internos). Con PSRAM (Metro) → heap de la VM y/o FS en PSRAM (MBs).
> El código de la VM no cambia: sólo de dónde sale el buffer.
>
> **3. La stdlib HW es board-aware.** Los límites (rango GPIO, nº de slices PWM,
> canales ADC, instancias PIO…) dependen de la placa. `Gpio.Pin(47)` es válido
> en Metro (48 GPIO) y error en Pico (30 GPIO).
>
> **4. Todo HW nuevo es clase OO** (política P-hw-class-policy, #123).
>
> **5. Separación CHIP ↔ PLACA** (matiz usuario 2026-06-06). Dos capas:
> - **CHIP (variante A/B)**: caps que dependen del microcontrolador (gpioCount,
>   nº PIO/PWM/ADC). De una TABLA built-in por variante. El core del firmware NO
>   conoce "Metro"/"Pico", sólo variantes RP2350A/B.
> - **PLACA**: lo board-specific (LED, NeoPixel, CS de la PSRAM…) vive en DATOS:
>   `/sys/board.json`. El NeoPixel es de la Metro, **no** del RP2350 → va aquí,
>   no en el firmware. Una placa distinta = otro board.json, sin recompilar.

El **orden de las tareas es flexible** (improvisamos según ganas/bloqueos). La
PSRAM (H7.2) es el premio gordo pero la más arriesgada/HW-gated → razonable
dejarla para cuando lo demás esté.

---

## H7.1 — Board target genérico RP2350B  ✅ (build hecho 2026-06-06; falta smoke HW)
Decisión: en vez de un header atado a una placa concreta, un header **genérico
RP2350B** que es un clon de `pico2.h` con el único cambio `PICO_RP2350A 0` →
`NUM_BANK0_GPIOS = 48`. Lo board-specific (NeoPixel, CS de PSRAM, LED) NO se
hornea: vive en `/sys/board.json`. Una imagen para Pico 2 (A: pines 30-47 no-op
inocuo) y Metro/cualquier RP2350B.
- `bpgenvm-c/pico/sdk_board/bp_rp2350b.h` (header nuevo). Flash/boot2 idénticos a
  pico2 (4MB + W25Q080) — combinación ya verificada arrancando en el Metro.
- `CMakeLists.txt`: `PICO_BOARD_HEADER_DIRS += sdk_board`, default `PICO_BOARD`
  ahora `bp_rp2350b`. Reconfigura + compila/linka limpio (cmake confirma
  "Using board configuration from .../bp_rp2350b.h").
- La SDK confirmó: `#if PICO_RP2350A` → 30 GPIO, `#else` → 48. Dato útil: el CS de
  PSRAM es board-specific incluso entre placas B (Pimoroni Plus2 = GP47, WeAct
  RP2350B = GP0) → confirma que `psramCsPin` va en board.json.
- ✅ **VERIFICADO EN HW** (Metro, 2026-06-06): el build genérico B arranca bien.
  Precondición de H7.2.a (CS de PSRAM en GP47 = pin B-only) cumplida.

## H7.2 — PSRAM + redistribución del mapa de memoria
- **H7.2.a — detección + init de PSRAM** ✅ (build hecho 2026-06-06; falta HW).
  Confirmado: la pico-sdk NO trae driver psram → init manual QMI. `psram.{h,c}`:
  `psram_detect_init(cs_pin)` portado de SparkFun (BSD-3, sfe_psram.c) — enruta
  cs_pin a `XIP_CS1`, sondea APS6404 (reset 0x66/0x99 + read-ID 0x9F → KGD 0x5D,
  densidad→tamaño), y si hay PSRAM la pasa a QPI + mapea la ventana M1 escribible
  en `0x11000000`. Corre desde RAM (`__no_inline_not_in_flash_func`) con IRQs off
  (el direct-mode suspende el XIP). Salvaguardas: timeouts implícitos por BUSY,
  restaura XIP siempre, y restaura la función del pin si no detecta (no rompe un
  Pico). `board_desc_init` sondea `psram_cs_pin` (de board.json/variante) y
  rellena `psram_present`/`psram_bytes`; default A → cs=-1 (no sondea GP0=UART).
  Logueado en boot. ✅ **VERIFICADO EN HW** (Metro, 2026-06-06): con
  metro-rp2350b.json (psramCsPin:47) loguea `psram: 8388608 bytes (8 MB)
  detectada @ GP47` y arranca bien. Sondeo OPT-IN (sólo si board.json declara
  psramCsPin) + a prueba de cuelgues (timeouts + restaura XIP) tras un primer
  intento que colgaba el boot. ✅ Además H7.2.b paso 1: `psram_enable_xip` (QPI +
  ventana M1 escribible) + `psram_rw_selftest`; `psram_present` sólo si los tres
  (detect+QPI+RW) pasan = PSRAM USABLE. Verificado en HW (RW OK).
- **H7.2.b — buffers grandes como punteros runtime.** ✅ **HECHO + VERIFICADO EN
  HW** (Metro, 2026-06-06). `s_vm_buffer` pasa de array estático de 128 KB a
  PUNTERO elegido en boot: `s_sram_buffer` (SRAM, fallback) por defecto, o la
  ventana PSRAM (`0x11000000`, `psram_bytes`) si la PSRAM es USABLE. `repl_v1`
  usa el puntero+tamaño runtime. Verificado: `samples/PsramBig.bp` aloca un array
  de 4 MB (1M enteros), lo llena y verifica → "PSRAM OK" en Metro (heap 8 MB),
  `RuntimeError`/OOM en Pico (128 KB) — misma imagen. Falta (opcional): mover
  también `s_data` (FS) → eso se cubre mejor en H7.2.e (flash 16 MB).
- **H7.2.c — mapa de memoria configurable.** PENDIENTE (refinamiento): hoy el
  heap PSRAM usa el `psram_bytes` completo (8 MB). Honrar `memorySize` de
  BpVM.cfg hasta el tope PSRAM. Sin PSRAM: layout actual.
- **H7.2.d — verificación.** ✅ Metro (PSRAM, PsramBig OK) + el mismo binario en
  Pico (sin PSRAM, layout 128 KB). Falta confirmar PsramBig→OOM en Pico real y el
  debugger en ambas.
- **H7.2.e — flash 16 MB (#229).** Detección de flash por JEDEC ✅ (INFO muestra
  16 MB en Metro / 4 MB en Pico). Falta el FS flash-backed grande.

## H7.3 — RP2350B: más pines y periféricos
- **H7.3.a — descriptor de placa en runtime.** ✅ HECHO (2026-06-06). RP2350A
  (30 GPIO, Pico 2) vs RP2350B (48 GPIO, Metro). `board_desc.{h,c}`: struct
  `board_desc_t` + tabla de caps por variante (`apply_variant_caps`) + override
  desde `/sys/board.json` (datos de placa: name, ledPin, neopixelPin, psramCsPin,
  gpioCount). `board_desc_init()` en boot tras `fs_init`; `Pico.board` lee el
  name del descriptor. Plantillas en `pico/boards/` (pico2.json, metro-rp2350b.json
  minimal + README con el esquema). Default sin board.json: variante **A (30
  GPIO)** — seguro mientras el build sea target `pico2` (evita panic del SDK al
  tocar pines 30-47); H7.1 lo sube al pasar a target B, H7.2 lo hace dinámico
  por sondeo de PSRAM. ✅ Validación (H7.3.b) y caps en BP (H7.3.c) ya hechos.
- **H7.3.b — stdlib HW board-aware.** ✅ HECHO (2026-06-06) para Gpio. El
  constructor `Gpio.Pin(num, mode)` valida `num` contra `Pico.gpioCount()` y
  lanza `RuntimeError` si está fuera de rango: `Gpio.Pin(47)` OK en Metro
  (48 GPIO), error en Pico (30). Verificado en host (ambas VMs rechazan pin 47
  en perfil 30). **Pendiente**: extender a Pwm/Adc/Spi/Uart/Pio si se quiere.
- **H7.3.c — exponer en BP.** ✅ HECHO (2026-06-06). Nuevo intrínseco
  `Pico.gpioCount()` (builtin 123, lo resuelve el firmware desde board_desc) +
  `Pico.variant()` (deriva "A"/"B" en BP puro, sin builtin propio).
  `Pico.GPIO_COUNT()` delega en `gpioCount()` → board-aware automático.
  **Paridad dual-VM verificada** (`gpioCount=30 / variant=A / GPIO_COUNT=30`
  byte-idéntico en VM Java y VM-C host). `Pico.board` ya salía del descriptor
  (H7.3.a). Sample: `samples/BoardTest.bp`.

## H7.4 — NeoPixel (WS2812): protocolo nuevo vía PIO
La Metro lleva un **NeoPixel (WS2812)** onboard — LED RGB direccionable por un
protocolo **1-wire de timing crítico** (~800 kHz, bits por duración de pulso).
NO lo tenemos: hay que añadirlo, y en RP2350 se hace con **PIO** (state machine
de timing exacto). **Es la primera infraestructura PIO del firmware.**
- **H7.4.a — driver WS2812 en firmware.** ✅ COMPLETO + **VERIFICADO EN HW**
  (Metro, 2026-06-06: NeoPixel onboard GP25 en verde al boot). Programa PIO + SM
  para el WS2812 (basado en `ws2812.pio` del SDK). Primera infra PIO reusable.
- **H7.4.b — clase BP `Neopixel`** (OO): ✅ COMPLETO + **VERIFICADO EN HW**
  (Metro, 2026-06-06: `samples/NeoDemo.bp` cicla R/G/B desde BP). `Neopixel.Strip(pin, count)`
  con `setPixel(i, r, g, b)` / `setColor(r,g,b)` / `show` / `clear` / `count`. Buffer
  GRB en BP (`integer[]`); `show()` lo empuja vía intrínsecos `__npInit`/`__npShow`
  → builtins 124/125 → backend firmware → driver PIO. Cadena multi-VM completa
  (no-op en host/VM Java). Commit `510b574`.
- Abre: PIO como recurso general (futuros protocolos: más NeoPixels, DHT,
  servos por timing, etc.).

## H7.5 — Lo que H7 abre (cositas / futuro)
- Programas grandes de verdad (heap PSRAM de MBs).
- PIO como infraestructura para más protocolos de timing.
- Más I/O del RP2350B aprovechada desde BP.

---

## Estado
- **H7.1**: baseline confirmado (Metro corre el firmware pico2); falta board target.
- **H7.3**: ✅ COMPLETO (a+b+c) + **VERIFICADO EN HW** (Metro, 2026-06-06).
  Descriptor de placa (chip↔placa) + board.json + tabla de variantes (a);
  validación de rango GPIO en Gpio.Pin (b); intrínseco gpioCount + variant()
  expuestos en BP, paridad dual-VM verificada (c). En la Metro: sin board.json →
  `30/A`; con `boards/metro-rp2350b.json` como `/sys/board.json` → `48/B`. El
  MISMO binario reporta una u otra cosa según el dato de placa → la abstracción
  chip↔placa funciona end-to-end. Falta sólo (opcional) extender validación a
  otros periféricos. **OJO**: usar pines 30-47 de verdad necesita H7.1 (el build
  target `pico2` del SDK panicaría en gpio_init de pines ≥30).
- **H7.1**: ✅ COMPLETO + **VERIFICADO EN HW** (build genérico RP2350B, 48 GPIO,
  una sola imagen para Pico 2 y Metro).
- **H7.2.a/b**: ✅ COMPLETO + **VERIFICADO EN HW** (PSRAM APS6404 8 MB detectada @
  GP47, heap de la VM reubicado a PSRAM; `samples/PsramBig.bp` escribió+verificó un
  array de 4 MB en heap externo). Opt-in vía `psramCsPin` en board.json.
- **H7.4**: ✅ COMPLETO (a+b) + **VERIFICADO EN HW** (Metro, 2026-06-06). Driver
  WS2812 vía PIO (1ª infra PIO) + clase OO `Neopixel.Strip`; `samples/NeoDemo.bp`
  cicla el NeoPixel onboard por R/G/B desde BasicPlus. Cualquier placa con su
  `neopixelPin` en board.json lo usa igual.
- **IDE INFO + flash JEDEC**: ✅ botón INFO (micro/flash/RAM/PSRAM) + detección de
  flash (Metro: 16 MB W25Q128). Falta corregir lectura de temperatura (~412°C).
- Resto: por arrancar. Orden flexible.

## Decisiones abiertas
1. **PSRAM/SDK** (H7.2.a): actualizar pico-sdk con soporte psram vs init manual QMI.
2. **Board target** (H7.1): board header propio del Metro vs uno cercano con overrides.
