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

## H7.1 — Board target propio del Metro
Hoy flasheamos el `.uf2` de `pico2` y **funciona** (mismo MCU, boot2/flash
compatibles) — verificado: Hello, T y Debug on Pico corren en Metro. Pero no es
"lo correcto":
- La pico-sdk actual NO trae board header del Metro RP2350. Opciones: crear uno
  propio (`adafruit_metro_rp2350.h`), o usar uno cercano (`pimoroni_pico_plus2_rp2350`
  / genérico RP2350B) con overrides (flash 16 MB, pin del NeoPixel, CS de PSRAM).
- `PICO_BOARD` se selecciona en `bpgenvm-c/pico/CMakeLists.txt` (default `pico2`).
- **Hecho parcial**: baseline de ejecución confirmado (2026-06-05). Falta el
  target board correcto.

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
  minimal + README con el esquema). Default sin board.json: variante B permisiva
  (lo afinará el sondeo de PSRAM en H7.2). **Falta cablear validación de rango
  (H7.3.b) y exponer caps en BP (H7.3.c).**
- **H7.3.b — stdlib HW board-aware.** Gpio/Pwm/Adc/Spi/Uart/Pio respetan los
  límites de la placa (validación de rango por board). `Gpio.Pin(47)` OK en
  Metro, error en Pico.
- **H7.3.c — exponer en BP** (`Pico.board` / `Mcu.gpioCount` / capacidades) para
  que el programa se adapte a la placa.

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
- **H7.3.a**: ✅ descriptor de placa (chip↔placa) + board.json + tabla de variantes.
  Firmware compila/linka (BSS sin cambio relevante). Falta validación + BP (H7.3.b/c).
- Resto: por arrancar. Orden flexible.

## Decisiones abiertas
1. **PSRAM/SDK** (H7.2.a): actualizar pico-sdk con soporte psram vs init manual QMI.
2. **Board target** (H7.1): board header propio del Metro vs uno cercano con overrides.
