# H14 — Plan de test en dispositivos reales

_Fase de cierre v2, pre-publicación. Iniciada 2026-06-14._

## Objetivo

Verificar el stack BasicPlus (compilador + VM-C + firmware + IDE) en **hardware
real** antes de publicar v2. Debió hacerse antes; se formaliza ahora.

## Principio rector

El **oráculo dual-VM** (miVM Java ↔ bpgenvm-c, en host) ya garantiza byte a byte
que la *lógica* del lenguaje es correcta. H14 **no** re-verifica eso en placa: se
concentra en lo que **solo se ve en el silicio** — la frontera firmware↔hardware,
que es donde han salido todos los bugs recientes:

| Capa solo-placa | Por qué el host no la cubre |
|---|---|
| Blobs stdlib embebidos → dispatch OO 9 clases | host usa .mod frescas; placa el blob horneado |
| Backends HW reales (GPIO/I2C/SPI/UART/ADC/PWM/RTC/WDT/Timer/NeoPixel) | en host son stubs/log |
| SMP en silicio (2 cores RP2350, cola de salida) | scheduler distinto |
| FS en flash + persistencia tras reset + autorun | host usa fichero/RAM |
| USB CDC + wire v1 + Stop/KILL en caliente | host usa TCP/stdio |
| Alineación/endianness del bytecode en ARM/Xtensa | — |
| Deltas de placa: PSRAM, gpioCount board-aware, NeoPixel PIO (Metro) | específico de silicio |
| Debug on Device (breakpoints/step/locals) sobre wire real | — |

## Escalera de bring-up

Cada peldaño es un **control** que aísla una capa; si falla, sabes exactamente
dónde mirar. Orden de menor a mayor complejidad:

1. **Ejecución** — la VM arranca + `print` (`T.bp` / `hello`)
2. **OO** — dispatch de método/clase
3. **Hardware:**
   - a. **blink** — GPIO out
   - b. **I2C** — bus + device real
   - c. **SPI** — bus + device real
   - d. **UART** — TX/RX (loopback jumper)
   - e. **timers y demás** — Pwm / Rtc / Adc / Wdt / Timer
   - f. **Pulse** — contador hardware (PWM + edge counting)

## Matriz placa × peldaño

Estado: ✅ pasa · ⬜ pendiente · — n/a · 🔌 necesita HW/cableado externo

| Peldaño | Pico 2 W (RP2350A) | Metro (RP2350B) | ESP32-S3 | STM32 |
|---|---|---|---|---|
| 1 Ejecución | ✅ | ⬜ | ⬜ | ✅ |
| 2 OO | ✅¹ | ⬜ | ⬜ | ✅⁷ |
| 3a blink (GPIO) | ✅⁴ | ⬜ | ⬜ | ✅⁸ |
| 3b I2C | ✅² | ⬜ | — | — |
| 3c SPI | ✅✅³ | ⬜ | — | — |
| 3d UART | ✅⁵ | ⬜ | — | — |
| 3e timers | ✅⁶ | ⬜ | — | — |
| 3f Pulse | ✅³ | — | — | — |

¹ vía BusBug (I2c.Bus.read, array cross-module). OO puro sin HW (`OoSmoke`) = opcional.
² BMP280 T/P top-level (13-jun) + I2c.Bus.
³ SPI: **T1** chip_id=0x61 + **T2** T/HR/P/gas reales con paginación (14-jun). Pulse vía `PwmTest`.
⁴ `blink.bp` en Pico 2 (LED GP25) ✓; en Pico 2 W el LED está en el CYW43 → GPIO out validado por el CS del SPI.
⁵ `uartecho` loopback GP0↔GP1: eco "Hola UART" correcto (+ 0xFF de arranque, ver Hallazgos).
⁶ `PicoInfo` (ADC/tempC 23.9 °C real), `RtcDemo` (Rtc.Clock), `TimerBlink` (Timer.Alarm), `WdtTest` (Wdt.Timer), `PwmTest` (Pwm.Slice).
⁷ STM32: `OoSmoke` (OO puro) + `Gpio.Pin` (OO cross-module) — sin skew.
⁸ STM32: LED verde PC7=39 vía `Gpio.Pin` (`samples/BlinkStm32.bp`). Las filas 3b–3f en STM32 no aplican: backend HAL no portado aún [v3].

## Estado: tanda Pico 2 / Pico 2 W COMPLETA ✅ (2026-06-14)

**8/8 peldaños en verde, cero bugs bloqueantes.** Resultados en placa (vía IDE, Run on Device):

| Sample | Resultado en placa | Valida |
|---|---|---|
| `T.bp` | exit 0 | ejecución VM |
| `BusBug.bp` | `4: read ok` exit 0 | OO cross-module (I2c.Bus) |
| `blink.bp` | LED parpadea (Pico 2) | Gpio.Pin |
| `Bme688SpiId.bp` (T1) | `chip_id = 97` | bus SPI |
| `Bme688Spi.bp` (T2) | T=27.9 °C, HR=52 %, P=1009.6 hPa, gas/IAQ | SPI + paginación + `double` |
| `PicoInfo.bp` | serial real, 150 MHz, **temp 23.9 °C** (ADC), GPIO 30 | ADC interno + board-aware |
| `RtcDemo.bp` | reloj avanza + recalibra exacto | Rtc.Clock |
| `TimerBlink.bp` | 3 modos, timings exactos (250/200 ms) | Timer.Alarm |
| `WdtTest.bp` | 5× feed sin reset + disable OK | Wdt.Timer |
| `uartecho.bp` | eco "Hola UART\r\n" (loopback GP0↔GP1) | UART |
| `PwmTest.bp` | contado 1001 / 500 / 200 (<0.1 % error) | Pwm.Slice + Pulse.Counter |

**Clases OO HW validadas en placa:** I2c.Bus, Spi.Bus, Gpio.Pin, Rtc.Clock, Timer.Alarm,
Wdt.Timer, Pwm.Slice, Pulse.Counter (+ ADC interno vía Pico.tempC). Solo falta **Neopixel**
(es del Metro).

## Estado: tanda STM32 (Nucleo-U575ZI-Q) ✅ (2026-06-14)

**4/4 de lo que el firmware STM32 soporta hoy** (núcleo VM + GPIO; los buses HAL aún sin
portar). En placa, vía IDE (Run on Device):

| Sample | Resultado en placa | Valida |
|---|---|---|
| `T.bp` | `Hola mundo`, exit 0 | ejecución de la VM en Cortex-M33 |
| `OoSmoke.bp` | áreas + instanceof OK | OO puro (clases/herencia/`super`/virtual/`instanceof`) |
| `PicoInfo.bp` | `nucleo-u575zi`, serial 4230500D…, 160 MHz, gpioCount 128 | info MCU board-aware |
| `BlinkStm32.bp` | LED verde PC7 + prints | backend GPIO + **OO cross-module `Gpio.Pin` sin skew** |

Modelo de pin STM32: `pin=(puerto<<4)|bit`, A=0..H=7 (PC7=39 verde, PB7=23 azul, PG2=98 rojo).

### Pendiente (otras tandas)

- ⬜ **Metro RP2350B**: comparte firmware con la Pico → replicar la escalera + deltas
  PSRAM / NeoPixel onboard / 48 GPIO.
- ⬜ **ESP32-S3**: T1 smoke. **APLAZADO ~2-3 días** (sin pines libres en la placa actual;
  Eduardo pedirá una de repuesto).
- 🔵 **STM32 buses** (I2C/SPI/UART/PWM/ADC): portar los backends HAL = **feature [v3]**,
  fuera del *freeze*. `OoSmoke` queda listo para reusar en cualquier placa.

## Cableados

| Test | Conexiones |
|---|---|
| SPI (BME688) | SCK→GP2, SDI→GP3, SDO→GP4, CS→GP5, VIN→3V3, GND→GND |
| UART loopback | GP0 (TX) ↔ GP1 (RX) |
| PWM→contador | GP10 (PWM) ↔ GP13 (counter) |

## Hallazgos H14

- ✅ **`samples/PulseTest.bp`** (arreglado 14-jun): el comentario decía `GP10↔GP11`
  pero el código cuenta en **GP13**. Comentario alineado a GP13 + apunta a PwmTest.
- 🔌 **Pico 2 W** (la placa de test de Eduardo es una Pico 2 **W**): el LED onboard NO
  está en GP25 — está en el **CYW43439** (chip wifi), que necesita driver CYW43 (no
  soportado aún; va con WiFi #145 [v3]). GP23/24/25/29 dedicados al CYW43. → `blink.bp` y
  `TimerBlink.bp` corren pero el LED no parpadea en la W; para blink visible, LED externo
  en un GPIO libre (p.ej. GP15). GPIO out YA validado por el CS del SPI. El resto de tests
  usan GPIOs libres → no afectados. Nota: `Pico.boardName()` = `rp2350-generic` (la imagen
  única no distingue Pico 2 de Pico 2 W; misma RP2350A/30 GPIO).
- 🐟 **UART: byte 0xFF espurio al inicio** del RX en loopback (`recibidos = 12` en vez de
  11; el eco real "Hola UART\r\n" está en `byte[1..11]`). Es un glitch de arranque típico
  de UART (falso start bit al configurar el pin/conectar). No es bug de SW. Mejora [v3]:
  drenar el FIFO RX en `Uart.init()` o descartar el primer byte.
- 🐟 **RTC sin calibrar por el IDE**: `RtcDemo` mostró `epoch = 239 s` (segundos desde
  boot), no fecha real → el IDE no envió el comando TIME al conectar (sync de #127). El
  reloj funciona (avanza + recalibra exacto); falta confirmar si el IDE debe sincronizar
  la hora automáticamente. Pendiente de aclarar con Eduardo.
- **Nota rancia en `bpdevices/Bme688.bp`** ("pasar un array a un método de clase
  cross-module revienta en la VM del micro"): ese bug está **cerrado** — BusBug
  (`bus.read`, 1 array) y Bme688Spi (`bus.transfer`, 2 arrays) corren exit 0 en placa.
  El driver podría limpiarse a métodos OO (v3).
- 🔌 **STM32 (Nucleo) — alcance del firmware**: el backend de HW solo trae **GPIO + info
  de MCU**; I2C/SPI/UART/PWM/ADC no portados [v3]. `Pico.tempC()`=0 (sensor no cableado) y
  las constantes de capacidad (`I2C_BUSES`=2, `PWM_SLICES`=12…) son del módulo `Pico`
  genérico del RP2350, **no** del U575 (su `Stm32.bp` propio sería v3). Lo board-aware real
  (boardName / uniqueId / cpuFreqHz / gpioCount) sí refleja el U575. La 1ª prueba de OO
  cross-module en STM32 (`Gpio.Pin`) pasó **sin skew** → blobs stdlib del STM32 sanos.
