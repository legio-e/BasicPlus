# H9 — Tercera familia: STM32 (ref. Nucleo-U575ZI-Q)

> Brújula del hito **H9** de V2. Qué es, la superficie de port (qué se hereda /
> reescribe / descarta), las decisiones tomadas, y el plan por hitos.
> Mapa global de V2: `docs/V2_ROADMAP.md`. El "sobre" de plataformas:
> `docs/PHILOSOPHY.md`.
>
> *Arrancado 2026-06-07 (Eduardo + Claude), tras analizar la superficie de port
> con 3 exploraciones del código (costura portable↔vendor, firmware Pico como
> plantilla, lecciones del ESP32).*

---

## 1. Qué es H9 y por qué el U575

Tercera familia de MCU. Foco **STM32** (no dispersión a N fabricantes).
**Referencia: Nucleo-U575ZI-Q** — STM32U575ZI, Cortex-**M33**, 2 MB flash,
~786 KB SRAM, 160 MHz.

La clave del ahorro: el U575 es **Cortex-M33, el mismo core que el RP2350** de la
Pico. Por ser ARM Cortex-M (**Thumb-2**) se reutiliza **lo difícil** —el port de
FreeRTOS ARM-M33, la convención de llamada AAPCS, y **el AOT Thumb-2**— justo lo
que el ESP32 (Xtensa) **no** pudo. ⇒ **STM32 = 2ª familia con AOT.**

Placas disponibles (el usuario las tiene):
- **Nucleo-U575ZI-Q** — referencia de H9 (M33, Nucleo-144, ST-LINK VCP).
- **Nucleo-L496ZG** — Cortex-M4 / familia L4: prueba *barata* de que la capa HAL
  generaliza entre familias STM32 (H9.6).
- **F769I-DISCO** — reservada para **V3** (LCD 4" + SDRAM 16 MB).

## 2. Superficie de port (del análisis)

El diseño de `bpgenvm-c` es muy portable: el **core es C99 puro, cero `#ifdef` de
plataforma**; cada MCU vive en su carpeta (`pico/`, `esp32/`) y el HW se enchufa
**en runtime** vía structs de punteros a función
(`bpvm_gpio_set_backend(&backend)`). La plataforma se selecciona por **qué `.c`
linka cada build**, no por compilación condicional.

### Se HEREDA sin tocar (núcleo portable)
- `src/`: `bpvm`, `loader`, `interp`, `heap`, `builtins`, `link`, `scheduler`,
  `threading`, `exceptions`, `comm_common`, `aot_registry`, `bpvm_aot_helpers`,
  y las **fachadas HAL** `gpio/i2c/spi/uart/pwm/adc/rtc/wdt/pulse/neopixel/pico`.
- `include/`: todos los `bpvm*.h`.
- `wire_v1.c/h`, `json_min.c/h` (framing JSON genérico).

### Se REESCRIBE (capa vendor nueva → carpeta `stm32/`)
- `main.c` — init HAL + clock + **registro de backends** + arranque FreeRTOS.
- `platform_freertos.c` — ≈ copia del de Pico (FreeRTOS API idéntica); swap de
  `time/delay` (`HAL_GetTick` / `DWT_CYCCNT`).
- **comm sobre USART** (no USB-CDC) — sink de salida + RX del wire.
- `fs.c` sobre **HAL FLASH interna** (región al final de los 2 MB).
- Backends `gpio/uart/i2c/spi/adc/pwm/wdt` sobre la HAL de ST.
- `mcu_mod` (info STM32U575: 2 MB flash, 786 KB SRAM, 160 MHz, sin overclock).
- Board header + `boards/nucleo_u575zi.json`.

### Se DESCARTA en el MVP ("diseñar para el piso"; el U575 base no lo tiene)
- **SMP / `comm_pico` / `flash_lock`** — el M33 del U575 es single-core ⇒
  `bpvm_run` (no `bpvm_run_smp`). Más simple que la Pico.
- **PSRAM** — la Nucleo base no la lleva (heap en SRAM interna).
- **PIO / NeoPixel** — STM32 no tiene PIO; diferir (o bit-bang).
- **overclock** — el chip está fijado a 160 MHz (sin setter).

## 3. Decisiones (2026-06-07)

- **Andamiaje**: **STM32CubeIDE + CubeMX genera la base** (clock tree RCC, linker
  script, startup, HAL, scaffolding FreeRTOS incl. el *timebase* en un TIM) e
  **injertamos** el core VM portable + la capa fina `stm32/`. Coherente con usar
  el SDK del fabricante (como `pico-sdk` / ESP-IDF). Minimiza el código vendor
  delicado escrito a mano —lo que no puedo testear yo en placa.
- **Transporte MVP**: **USART por el VCP del ST-LINK** (wire-v1 sin levantar
  primero el USB-CDC). Mismo framing que Pico/ESP32, sin cambios.
- **Flasheo**: vía **ST-LINK desde CubeIDE** (un clic). **STM32 NO usa `.uf2`**
  (eso es de la Pico). Alternativa "estilo uf2": arrastrar el `.bin`/`.hex` al
  disco USB que monta el ST-LINK de la Nucleo.
- **Paridad dual-VM byte-idéntica**: se **hereda gratis** (corre el mismo core
  portable). Invariante central intacto — se verifica diffeando stdout VM-Java ↔
  STM32 para el mismo `.mod`.
- **Modelo de trabajo**: yo escribo firmware/glue; **tú construyes + flasheas +
  observas** (no puedo flashear). Cada hito termina en un **checkpoint
  flasheable** con un observable claro.

## 4. Hitos

### H9.1 — Esqueleto + Hello por VCP ✅ HECHO (2026-06-07)  *(= H4.1 del ESP32)*
- **H9.1.0 (entorno)**: proyecto CubeIDE para Nucleo-U575ZI-Q con FreeRTOS +
  USART(VCP). Smoke = LED blink / `printf` por VCP, flasheado OK. *De-risk del
  toolchain + flasheo antes de injertar nada.*
- **H9.1.1**: injertar `src/` + `include/` → **compila** para M33 (link OK).
- **H9.1.2**: `platform_stm32.c` (bare-metal, **sin FreeRTOS** — el single-thread
  no lo necesita) + sink USART + `Hello.mod` embebido → **"Hello" sale por el VCP**.

> **✅ Resultado (2026-06-07)**: la VM corre en el U575. `Hola desde STM32
> (BasicPlus VM)` / `42` salen por el VCP, **byte-idénticos** a la VM-Java y a la
> VM-C host → **paridad triple-VM**. Glue: `bpgenvm-c/stm32/port/`
> (`platform_stm32.c` + `bpvm_app.c` + `hello_mod.c`); el core `src/` se heredó
> **intacto** (solo se excluyó `platform_pthread.c`, host-only). El análisis de la
> superficie de port se validó en la práctica: capa vendor mínima, paridad gratis.
> Integración CubeIDE: *Source Location ▸ Link Folder* para `src` + `stm32/port`.
> FreeRTOS + wire-v1 (`Run on STM32`) → H9.2.

### H9.2 — Wire v1 + REPL sobre USART → **"Run on STM32"** ✅ HECHO (2026-06-08)  *(= H4.3)*
Subir `.mod`, ejecutar, stream de salida. El IDE distingue por
`serverName="bpvm-stm32"` en el `HELLO_REPLY`.

> **✅ Resultado**: el **dev-loop completo** funciona — compilar BP en el IDE →
> PUT del `.mod` (bulk) → RUN → la salida del programa llega al IDE
> (`hola pico` / `exit 0`). Bare-metal single-thread (super-bucle REPL), wire-v1
> por el **VCP del ST-LINK**. En `bpgenvm-c/stm32/port/`: `stm32_repl.c`
> (handlers), `stm32_wire.c` (framing + RX rápida por registro + **FIFO 8B**
> para el bulk), `stm32_fs.c` (FS en RAM), `json_min.c` (copia portable).
> **Cero cambios en el IDE** (su `SerialBackend` ya es genérico). Lecciones del
> camino: `HAL_UART_Receive` era demasiado lenta (→ lectura directa de registro);
> el bulk a 4 MHz necesitaba el FIFO del USART.
> **✅ RESUELTO (2026-06-08)**: el **160 MHz arregló la intermitencia** del bulk
> (era el reloj, no el loop de deps). Con la base estable se **re-aplicó H9.2.c**
> (imports + guard + LEDs, commit `1a37731`) y se **verificó en placa**: `App.bp`
> (`import Lib`) → `imports OK en STM32` / `42` / exit 0. El IDE sube `Lib.mod` a
> `/app`; el RUN lo carga del FS y `bpvm_run` enlaza (`bpvm_link_all`).
> **Dev-loop completo en la 3ª familia: conectar + subir + ejecutar + imports,
> sólido.** LEDs: 🟢 vivo · 🔵 ejecutando · 🔴 error.
> **Requisito de placa**: correr a **160 MHz** (4 MHz no aguanta el bulk).
>
> **Siguiente:**
> 1. **Embeber la stdlib core** en el firmware (IO/Math/Gpio… pre-instalados en
>    `/lib` al boot, como `EMBEDDED_CORE_MODS` de la Pico) → desbloquea los
>    programas que importan stdlib (el IDE la da por presente en el device).
> 2. **H9.4 (GPIO)** — backend HAL + parpadear un LED desde BP.

### H9.3 — FS persistente en flash interna  *(= H4.4)*
`fs.c` sobre HAL FLASH; región al final de los 2 MB (~132 KB o menos). PUT desde
el IDE persiste y sobrevive al reset.

### H9.4 — Backend GPIO + extensión de periféricos  *(= H4.5)*
GPIO (parpadear LED) + IDE sube las deps de stdlib que falten. Luego extender
UART / I2C / SPI / ADC / PWM (uno a uno, con prueba en placa cada uno).

### H9.5 — AOT en STM32  *(diferenciador vs ESP32)*
`.mdn` Thumb-2 reutiliza el `AotCEmitter` existente (ARM→ARM). Validar
`fib_native` en placa.

### H9.6 — 2ª placa STM32 (Nucleo-L496ZG, M4/L4)
Prueba *barata* de que la capa HAL generaliza entre familias STM32. Formaliza la
imagen-por-familia (§9b de la VM).

## 5. "H9 cerrada" cuando

1. **U575 end-to-end**: compilar BP en el IDE → **Run on STM32** → corre en placa,
   salida por VCP, **FS persistente**, **GPIO real**, y **paridad byte-idéntica**
   con la VM-Java para los samples de prueba.
2. *(stretch)* **AOT validado** en el U575.
3. *(stretch)* **L496ZG** corriendo el mismo firmware-base como prueba de
   generalización del HAL.

Candidata a ser **el hito de cierre de V2** (como el ESP32 cerró v1).

## 6. Primer paso — H9.1.0

Crear el proyecto base en STM32CubeIDE y validar toolchain + flasheo con un smoke
mínimo (LED / `printf` por VCP) **antes** de injertar la VM. Detalle operativo en
la conversación; en cuanto el smoke funcione, paso a H9.1.1 (injertar `src/`).
