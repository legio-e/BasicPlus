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
- **Falta**: smoke en HW (arranca en Pico 2 y Metro; Gpio.Pin(40) ya válido en
  Metro sin panic). Precondición de H7.2.a (CS de PSRAM en GP47 = pin B-only).

## H7.2 — PSRAM + redistribución del mapa de memoria
- **H7.2.a — detección + init de PSRAM** (QMI/XIP, APS6404 8 MB en el Metro).
  ⚠️ La pico-sdk actual no trae soporte psram → o actualizar la SDK (fork
  Adafruit / pico-sdk reciente con `pico_sram`/psram) o init manual por
  registros QMI + chip-select. HW-gated. Sondear presencia al boot (sin PSRAM →
  no-op).
- **H7.2.b — buffers grandes como punteros runtime.** Hoy `s_vm_buffer[128KB]`
  y `s_data[128KB]` (FS) son arrays estáticos en SRAM interna. Convertirlos a
  PUNTEROS elegidos en boot: PSRAM si la hay (regiones grandes), SRAM interna si
  no (los arrays actuales). El resto del firmware igual.
- **H7.2.c — mapa de memoria configurable.** Con PSRAM: heap VM de MBs (honrar
  `memorySize` de BpVM.cfg hasta el tope PSRAM) + FS mayor. Sin PSRAM: layout
  actual. Una tabla de tamaños por presencia-de-PSRAM.
- **H7.2.d — verificación.** Pico (sin PSRAM) y Metro (con PSRAM): paridad de
  salida; un programa que reviente los 128 KB en Pico y corra en Metro; el
  debugger sigue funcionando en ambas.

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
- **H7.4.a — driver WS2812 en firmware.** Programa PIO + SM para el WS2812
  (basado en el ejemplo `ws2812` de pico-examples). Primera infra PIO reusable.
- **H7.4.b — clase BP `Neopixel`** (OO): `Neopixel.Strip(pin, count)` con
  `setPixel(i, r, g, b)` / `setColor(r,g,b)` / `brightness` / `show` / `clear`.
  Backend intrínseco que llama al driver PIO. Sample de diagnosis (arco iris,
  blink RGB).
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
- Resto: por arrancar. Orden flexible.

## Decisiones abiertas
1. **PSRAM/SDK** (H7.2.a): actualizar pico-sdk con soporte psram vs init manual QMI.
2. **Board target** (H7.1): board header propio del Metro vs uno cercano con overrides.
