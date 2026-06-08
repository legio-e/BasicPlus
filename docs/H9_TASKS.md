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
> El `main.c` quedó cableado a `stm32_repl_run()` + reloj a 160 MHz (PLL desde
> MSI, FLASH_LATENCY_4, VOS1).
>
> **Siguiente → hecho en H9.4** (embeber stdlib + GPIO).

### H9.3 — FS persistente en flash interna  *(= H4.4)*  · ✅ HECHO (2026-06-08, verificado en placa)

`stm32_fs.c` gana `fs_save()`/`fs_load()` sobre la **HAL FLASH** del U575:
- **Región**: últimos **128 KB** de la flash (`0x081E0000..0x08200000`), reservados
  en el `.ld` (FLASH 2048→1920 KB + región `FS_FLASH`). **Disjunta del programa**
  ⇒ un fallo de escritura nunca corrompe el firmware (peor caso: pierdes el FS).
- **Layout**: header (magic `BPFS` + version + count + arena_used) + tabla en la
  1ª página (8 KB); arena cruda a partir de +8 KB. Borra por páginas de 8 KB,
  programa en quad-words (16 B); banco/página detectados en runtime
  (`FLASH->OPTR & DUALBANK`). ICACHE off durante la escritura; **sin** desactivar
  IRQs (el bus se para solo en cada op → no rompe los timeouts de la HAL).
- **Auto-persist**: PUT/DEL (fuera de `/lib`) y FORMAT vuelcan el FS. Los PUT a
  `/lib` (el IDE sube ahí cada Run) **no** persisten → cero flash por ejecución.
- **Boot**: `fs_load()` restaura **/app** (salta `/lib`, que re-instala el
  embebido → stdlib siempre fresca, sin skew tras actualizar firmware). Magic/
  version inválidos (flash borrada) → FS vacío.

> **✅ Verificado en placa (2026-06-08)**: un fichero subido a `/app` sobrevive al
> reset y se ejecuta sin re-subir. Con esto el **U575 cumple el criterio de cierre
> completo** (§5.1): Run + imports + GPIO real + FS persistente + paridad. Solo
> quedan los *stretch* (H9.5 AOT, H9.6 L496), diferidos.

### H9.4 — stdlib embebida + Backend GPIO  *(= H4.5)*  · ✅ HECHO (2026-06-08, verificado en placa)

**1) stdlib core embebida.** `stm32_mods.c` (GENERADO por
`scripts/regen_stm32_mods.sh` desde `bpstdlib/*.mod`) trae los 13 módulos de
`EMBEDDED_CORE_MODS` del IDE (Math, IO, Gpio, I2c, Spi, Uart, Pulse, Pwm, Pico,
Rtc, Adc, Wdt, Timer) como arrays C; `stm32_mods_install()` los pre-instala en
`/lib` del FS al boot (idempotente), como la Pico. Desbloquea los programas que
importan stdlib (el IDE da esos módulos por presentes y NO los sube). `FS_MAX_FILES`
24→40 para los 13 + módulos de app. *(IO/Math son intrínsecos puros, pero se
embeben igual para que su `import` resuelva sin sorpresas.)*

**2) Backend GPIO + info de MCU** (`gpio_stm32.c`, registrado por
`stm32_hw_register()` al boot):
- GPIO `init/pull/write/read` sobre `HAL_GPIO_*`. **Pin plano**
  `pin = (puerto<<4)|bit` (puerto 0=A..7=H) → PC7=39 (verde), PB7=23 (azul),
  PG2=98 (rojo). Caché de modo/pull por pin para que `pull()` no pise el modo.
- Backend `Pico` (info de MCU): `gpioCount()`→128 (8 puertos × 16, así la clase
  `Gpio.Pin` acepta cualquier pin de la placa), `cpuFreqHz`→`SystemCoreClock`,
  `uptimeMs`→`HAL_GetTick`, `uniqueId`→UID, `boardName`→"nucleo-u575zi".

**Demo `Blink.bp`** (en `stm32/port/`): `import Gpio`, API raw
`Gpio.init/write` sobre el LED verde (pin 39). Usa la API raw (no la clase `Pin`)
para ser **verificable en host**: el stub host de `Pico.gpioCount()` vale 30, así
que `Pin(39)` daría "fuera de rango" en host (en la placa el backend reporta 128).

> **✅ Verificado en host VM-C y en placa (2026-06-08)**: compila (`main` como
> entry), enlaza `Gpio`+`Pico` y conduce la fachada GPIO. En host las líneas
> `[gpio]` son el stub; en placa el backend conduce la HAL y el LED verde sigue el
> patrón del programa. El demo usa un patrón **inconfundible vs el heartbeat**
> (verde fijo 2 s / apagado 1.5 s / 6 rápidos) narrado por el wire → se ve sin
> ambigüedad que manda el programa. El IDE sube `Gpio.mod` a `/lib` (import directo)
> y el device resuelve `Pico` desde la stdlib embebida. **HW controlado desde
> BasicPlus en la 3ª familia.**
>
> Diagnóstico de LEDs del firmware: 🟢 verde heartbeat (lo toma el programa en un
> RUN) · 🔵 azul = RUN en curso · 🔴 rojo = error.
>
> Siguiente: UART/I2C/SPI/ADC/PWM (uno a uno, con prueba en placa cada uno).

### H9.5 — AOT en STM32  *(diferenciador vs ESP32)*
`.mdn` Thumb-2 reutiliza el `AotCEmitter` existente (ARM→ARM). Validar
`fib_native` en placa.

### H9.6 — 2ª placa STM32 (Nucleo-L496ZG, M4/L4)
Prueba *barata* de que la capa HAL generaliza entre familias STM32. Formaliza la
imagen-por-familia (§9b de la VM).

## 5. "H9 cerrada" cuando

1. **U575 end-to-end** ✅ **CUMPLIDO (2026-06-08)**: compilar BP en el IDE →
   **Run on STM32** → corre en placa, salida por VCP, **FS persistente**,
   **GPIO real**, y **paridad byte-idéntica** con la VM-Java. *(H9.1+H9.2+H9.4+H9.3
   verificados en placa.)*
2. *(stretch, diferido)* **AOT validado** en el U575 (H9.5).
3. *(stretch, diferido)* **L496ZG** corriendo el mismo firmware-base como prueba
   de generalización del HAL (H9.6).

> **Estado (2026-06-08)**: el **núcleo de H9 está cerrado** (criterio 1). Los
> stretch (AOT, L496) quedan aparcados — se retoman cuando toque. Próximo foco:
> H10 (stdlib, retoques ligeros) o tapar agujeros sueltos.

Candidata a ser **el hito de cierre de V2** (como el ESP32 cerró v1).

## 6. Primer paso — H9.1.0

Crear el proyecto base en STM32CubeIDE y validar toolchain + flasheo con un smoke
mínimo (LED / `printf` por VCP) **antes** de injertar la VM. Detalle operativo en
la conversación; en cuanto el smoke funcione, paso a H9.1.1 (injertar `src/`).
