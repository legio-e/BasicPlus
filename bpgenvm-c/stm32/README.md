# stm32/ — Firmware STM32 (H9) — ref. Nucleo-U575ZI-Q

Capa *vendor* STM32 de `bpgenvm-c` (3ª familia de MCU, hito **H9**). Hereda el
**core portable** de `../src` + `../include` **sin tocarlo**; aquí vive sólo lo
específico de STM32. Plan completo: `../../docs/H9_TASKS.md`.

## Toolchain (verificado 2026-06-07)
- **STM32CubeIDE 2.1.1** (`C:\ST\STM32CubeIDE_2.1.1`) — IDE + CubeMX integrado +
  `arm-none-eabi-gcc` **14.3.1** + GDB + builder headless `stm32cubeidec.exe`.
- **STM32CubeProgrammer 2.22** — flasheo. (CubeIDE trae su propia copia.)
- **Flasheo**: botón verde *Run* (ST-LINK, 1 clic) **o** arrastrar `.hex`/`.bin`
  al disco USB del ST-LINK (D:) **o** `STM32_Programmer_CLI -c port=SWD -w fw.hex -rst`.
- **STM32 NO usa `.uf2`** (eso es de la Pico/RP2350).

## Modelo de proyecto
El proyecto CubeIDE vive **aquí** (`bpgenvm-c/stm32/`) para versionar el firmware
junto al resto (como `pico/` y `esp32/`). CubeMX gestiona `Core/` + `Drivers/` +
el `.ioc`; **nuestra capa va aparte** (fuera de `Core/`) para que regenerar desde
CubeMX no la pise.

## Core VM heredado — NO duplicar
- **Include path** del proyecto: añadir **`../include`**.
- **Fuentes del core** (de `../src`) = la lista `SRC` del `Makefile` **menos las 2
  de host**. Añadir como *carpeta enlazada* / source location, **24 ficheros**:
  ```
  bpvm  loader  interp  heap  builtins  link  scheduler  threading  exceptions
  gpio  i2c  spi  uart  pulse  pwm  pico  bpvm_neopixel  rtc  adc  wdt
  aot_registry  bpvm_aot_helpers  scheduler_smp  comm_common
  ```
- **EXCLUIDAS** (host-only; las sustituye la capa stm32):
  - `platform_pthread.c` → la sustituye el **platform layer FreeRTOS** de stm32.
  - `comm_host.c` → lo sustituye el **sink por USART**.
  - (`test/main.c` no es firmware.)
- El core es **C99** → compila con el `gnu11` por defecto de CubeIDE sin problema.

## Capa vendor STM32 (lo que escribimos aquí, por hito)
- **H9.1.2** — `platform_freertos.c` (mutex/cond/thread/sleep/now) ≈ copia de
  `../pico/platform_freertos.c`, con `now`→`HAL_GetTick`/`DWT_CYCCNT`; + **sink de
  salida por USART** (el VCP del ST-LINK); + **`Hello.mod` embebido** (array C vía
  `xxd -i`) → "Hello" sale por el VCP.
- **H9.2** — RX `wire_v1` sobre USART + `repl_v1` (heredado) → **"Run on STM32"**.
- **H9.3** — `fs` sobre **HAL FLASH** interna (región al final de los 2 MB).
- **H9.4** — backends `gpio/uart/i2c/spi/adc/pwm` sobre HAL de ST (registro en
  runtime con `bpvm_*_set_backend`).
- **H9.5** — AOT Thumb-2 (`.mdn`) reutilizando el `AotCEmitter` (ARM→ARM).

## Single-core ("diseñar para el piso")
El U575 es **Cortex-M33 single-core** ⇒ `bpvm_run` (no `bpvm_run_smp`). Se
**descartan**: SMP / `comm_pico` / `flash_lock` / PSRAM / PIO-NeoPixel / overclock.
`FreeRTOSConfig`: `configNUMBER_OF_CORES=1`, `configCPU_CLOCK_HZ=160000000`,
tick 1000 Hz.

## Estado
H9.1.0 (entorno ✅ + smoke LED/VCP) en curso. Falta del usuario tras el smoke:
qué `huartX` es el VCP, y confirmar estructura estándar del proyecto CubeIDE.
