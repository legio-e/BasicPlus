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
- **H9.2** ✅ — RX `wire_v1` sobre USART (`stm32_wire.c`) + REPL bare-metal
  (`stm32_repl.c`) + FS en RAM (`stm32_fs.c`) + `json_min.c` → **"Run on STM32"**
  con resolución de imports. **Requiere 160 MHz** (a 4 MHz el bulk del PUT
  desborda la RX del USART).
- **H9.3** (✅ pendiente placa) — **FS persistente** en flash interna
  (`stm32_fs.c`: `fs_save`/`fs_load`). Región = últimos **128 KB** (`0x081E0000`),
  reservados en el `.ld` (FLASH 1920 KB + `FS_FLASH`) → disjunta del programa.
  Auto-persist en PUT/DEL/FORMAT (los PUT a `/lib` no persisten). Al boot
  restaura `/app`; `/lib` lo re-instala el embebido. Borra por páginas (8 KB),
  programa quad-words (16 B), banco/página por `DUALBANK`, ICACHE off (sin tocar
  IRQs → timeouts HAL intactos).
- **H9.4** (parcial ✅) — **stdlib core embebida** (`stm32_mods.c`, generado por
  `scripts/regen_stm32_mods.sh`): pre-instala IO/Math/Gpio/Pico/… en `/lib` al
  boot (como `EMBEDDED_CORE_MODS` de la Pico) → los programas que importan stdlib
  resuelven sin uploads. **Backend GPIO + info de MCU** (`gpio_stm32.c`,
  `stm32_hw_register`): `init/pull/write/read` sobre `HAL_GPIO_*` y `gpioCount/
  cpuFreqHz/uptimeMs/uniqueId/boardName` sobre la HAL.
  **Modelo de pin** (plano): `pin = (puerto<<4) | bit`, puerto 0=A..7=H →
  PA5=5, PB7=23 (LED azul), PC7=39 (LED verde), PG2=98 (LED rojo).
  Pendiente: UART/I2C/SPI/ADC/PWM (uno a uno).
- **H9.5** — AOT Thumb-2 (`.mdn`) reutilizando el `AotCEmitter` (ARM→ARM).

## Single-core ("diseñar para el piso")
El U575 es **Cortex-M33 single-core** ⇒ `bpvm_run` (no `bpvm_run_smp`). Se
**descartan**: SMP / `comm_pico` / `flash_lock` / PSRAM / PIO-NeoPixel / overclock.
`FreeRTOSConfig`: `configNUMBER_OF_CORES=1`, `configCPU_CLOCK_HZ=160000000`,
tick 1000 Hz.

## LEDs de diagnóstico (firmware)
- 🟢 **verde** (PC7) — heartbeat: parpadea ~2 Hz entre RUNs = vivo (congelado = colgado).
- 🔵 **azul** (PB7) — encendido mientras un programa se ejecuta (RUN en curso).
- 🔴 **rojo** (PG2) — error serio (FATAL del wire, o RUN que falló).

Durante un RUN el heartbeat se pausa (el super-bucle está bloqueado), así que un
programa que controle el LED verde lo hace sin interferencia; al terminar, el
heartbeat lo retoma.

## Estado
- **H9.1 ✅** — la VM corre en el U575 (Hello por VCP, paridad triple-VM).
- **H9.2 ✅** — dev-loop completo: conectar + subir + ejecutar + imports, sólido
  a **160 MHz**.
- **H9.4 (parcial) ✅** — stdlib core embebida + backend GPIO/Pico → `Blink.bp`
  controla el LED verde desde BasicPlus (**✅ verificado en placa 2026-06-08**).
- **H9.3 ✅ (verificado en placa 2026-06-08)** — FS persistente en flash interna:
  un fichero subido a `/app` sobrevive al reset.
- **Núcleo de H9 cerrado**: el U575 corre end-to-end (Run + imports + GPIO real +
  FS persistente + paridad). *Stretch diferidos*: H9.5 (AOT), H9.6 (L496).
