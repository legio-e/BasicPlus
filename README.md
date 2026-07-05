# BasicPlus

> 🇪🇸 [Versión en español](README.es.md)

**A general-purpose language for 32-bit microcontrollers, with modern BASIC
syntax, object orientation, a graphical UI and a first-class debugger.**

It compiles to bytecode (`.mod`) that runs **identically** on the PC and on
the micro: the same program you debug on your desktop later blinks an LED on
a Raspberry Pi Pico 2, an ESP32-S3, an ESP32-P4 or an STM32 — no recompiling,
no `#ifdef`, no surprises.

```basic
// blink.bp — blinks the Pico on-board LED (GP25) using the OO API.
module Blink
  import Gpio

  function Main()
    var led: Gpio.Pin := Gpio.Pin(25, Gpio.Pin.OUTPUT)
    var i: integer := 0
    while i < 100 do
      led.on()
      sleep(300)
      led.off()
      sleep(300)
      i := i + 1
    endwh
    print "Blink done"
  end Main
end Blink
```

On a PC with no GPIO the hardware builtins log to stdout; on the board they
drive real pins. The program is the same, byte for byte.

## New in V3 — graphical interfaces

V3 adds **graphical interfaces** built on **LVGL**: around twenty widgets, the
ability to **design screens in a JSON file** that becomes the window (with its
event handlers already wired) at runtime, and touch — all with the same "paints
on the PC, runs on the board". Verified on three screens: **STM32U5G9J-DK2**
(LTDC), **ESP32-P4-Function-EV** (EK79007) and a **Waveshare ESP32-P4** (ST7701).

![GuiColorDemo running on a board — GUI with LVGL](docs/img/guicolordemo.png)

*The `GuiColorDemo` sample — the same bytecode runs on the PC and on the micro.
Everything in the **[GUI guide](docs/en/gui.html)**.*

---

## The name (and a debt of gratitude)

BasicPlus takes its name from **Digital Equipment Corporation's BASIC-PLUS**,
the structured BASIC dialect that ran under RSTS/E on PDP-11s through the
70s and 80s. That language proved a BASIC could be serious: whole systems
were written in it, and a generation learned to program with it. This
project is a homage to that idea — **a language that is easy to read but
doesn't treat you like a beginner** — transplanted to today's small
hardware: 32-bit microcontrollers.

## What the language includes

- **Full object orientation**: classes with constructors, inheritance
  (across modules too), `instanceof`, properties with `get`/`set`,
  `sync` (atomic) properties, static members.
- **Types**: `integer`, `long` (i64), `float`, `double`, `boolean`, `string`
  (native UTF-8, codepoint-indexed), narrow types (`byte`, `word`,
  `short`, `int8`, `int16`), arrays (`integer[]`, `byte[]`, `long[]`, …) and
  fixed-size local arrays (`var buf: byte[64]`).
- First-class **tuples** (`(integer, string)`), **default parameter
  values**, multi-line expressions, `switch/case`.
- **Exceptions**: `try/catch/finally`, a hierarchy with a common `Exception`
  base, catchable native `RuntimeError` (division by zero, indexing,
  network…), user-defined exception classes — across modules too.
- **Concurrency**: BP threads on a quantum scheduler, `Mutex`,
  `synchronized`, `SyncList` queues with blocking consume.
- **Modules** with compiled interfaces (`.bpi`, the Modula-2 DEFINITION
  model): `import` resolves against the interface, not the source.
- **`native function`**: functions AOT-compiled to C/Thumb-2 (`.mdn`) that
  run at native speed on RP2350/STM32 — 40-90× over the interpreter on
  compute kernels, with faults propagated to BP `try/catch`.
- **Graphical UI**: the `Gui` module over **LVGL** — ~20 OO widgets, forms
  designed in JSON (`.win`), color and fonts, touch. The same bytecode paints
  in a window on the PC and on the board's screen.
- **Standard library**: `Core`, `Math`, `IO` (files + prompt), `Str`,
  `Collections`, `Stats`, `Compress` (LZSS), `Log`, `Json`, `Net` (TCP
  client) and the hardware zoo: `Gpio`, `I2c`, `Spi`, `Uart`, `Pwm`, `Adc`,
  `Rtc`, `Wdt`, `Timer`, `Pulse`, `Neopixel`, `Pico`. All new hardware is
  a class (`Gpio.Pin`, `I2c.Bus`, `Net.Tcp`, …).

## Two twin VMs, one sacred invariant

The bytecode is executed by **two independent virtual machines**:

| VM | Language | Where it runs |
|---|---|---|
| **miVM** | Java | PC (development, debugging, CI) |
| **bpgenvm-c** | C99 | PC (host) and the microcontroller firmwares |

The project's golden rule: **a program's output must be byte-identical on
both VMs**. Every feature is verified against that invariant before it gets
in. It is what makes "debug on the PC, deploy on the micro" more than a
slogan.

## Supported platforms

| Platform | Transport | Notes |
|---|---|---|
| PC (Windows/Linux/macOS) | — | Both VMs; TCP daemon for the IDE. VM-C + LVGL/SDL renders the GUI in a window. |
| Raspberry Pi **Pico 2** and Adafruit **Metro RP2350** | USB-CDC | **A single firmware image** for both boards: variant (A/B), pins and PSRAM are decided at *runtime* (`/sys/board.json`), not with compile-time macros. AOT enabled. |
| **ESP32-S3** (Xtensa) | UART0 | Persistent FS on a dedicated partition; console over native USB. |
| **ESP32-P4** (RISC-V) — with a screen | UART | GUI over LVGL on a MIPI-DSI panel + touch. **A single image** for the Function-EV (EK79007 1024×600) and the Waveshare 4.3" (ST7701 480×800); the panel is chosen at *runtime* (`/sys/board.json`). |
| **STM32** (Nucleo-U575ZI-Q · Discovery U5G9J-DK2) | ST-LINK VCP | FS in internal flash; AOT enabled (same Cortex-M33 as the RP2350). The **Discovery DK2** adds an LTDC screen (800×480) + touch (GUI). |

On all of them: a "wire v1" REPL (JSON per line) with file upload, remote
RUN, **Stop** (cooperative KILL without resetting the board), **autorun**
(`/sys/auto.txt` starts your program on power-up — a truly standalone
device, and the IDE can attach live and take back control) and on-device
debugging with breakpoints.

## Verified on real hardware

This is not desk theory: the full stack has been exercised on silicon.
On a **Raspberry Pi Pico 2 / Pico 2 W** (RP2350) the whole hardware ladder
has been validated end to end — from the IDE, with *Run on Device*, running
the **same bytecode** as on the PC:

| Subsystem | On-board test | |
|---|---|:--:|
| Execution / OO | cross-module stdlib classes dispatching on the micro | ✅ |
| GPIO | blinking LED (`Gpio.Pin`) | ✅ |
| I2C | real BMP280 sensor (T/P) + bus scan | ✅ |
| SPI | BME688 over SPI: `chip_id` + T/RH/P/gas reads with memory paging | ✅ |
| UART | loopback, byte echo | ✅ |
| PWM + counter | 1 kHz / 500 Hz counted in hardware, < 0.1 % error | ✅ |
| ADC | chip internal temperature (23.9 °C) | ✅ |
| RTC | monotonic clock + recalibration | ✅ |
| Watchdog | feed / timeout / disable | ✅ |
| Timers | hardware alarms (polling, synchronized, stopwatch) | ✅ |

On the **STM32** (Nucleo-U575ZI-Q) the **four critical buses** were also
validated on the board with real sensors: GPIO, **SPI** (BME688), **UART**
(loopback) and **I2C** (BMP280, T/P). And on the **ESP32-S3** (DevKitC)
those **same four buses** were validated on the board with real sensors
(BME688 over SPI, BME280 over I2C, GPIO and UART loopbacks) — the **three
non-graphical families are on par**. The **Metro RP2350B** shares the firmware
image with the Pico.

**V3 — the boards with a screen.** The GUI (LVGL) was verified on the board on
all three: **STM32U5G9J-DK2** (LTDC 800×480 + GT911 touch),
**ESP32-P4-Function-EV** (EK79007 1024×600) and **Waveshare ESP32-P4** (ST7701
480×800) — widget catalog, color, `.win` forms and touch, the same bytecode on
all three.

## The IDE

`BpIde` (Swing, a single jar): tabbed editor, compilation with diagnostics,
local and remote Run/Debug, board file explorer with console (`dir`, `run`,
`kill`, `autorun`, `log`, …), micro INFO, breakpoints and stepping both in
the local VM and inside the device.

## Repository layout

```
lexer-java/   compiler (frontend): .bp → .mod + .bpi + .dbg (+ AOT .mdn)
miVM/         Java VM + debugger + TCP daemon
bpgenvm-c/    C99 VM: host, Pico/RP2350 firmware, ESP32-S3, ESP32-P4, STM32
BpIde/        Swing IDE (fat-jar)
bpstdlib/     standard library (.bp sources + compiled .mod)
samples/      example programs
docs/         manual, GUI guide, specs (.mod, opcodes, heap, wire), backlog
```

## Build and try it (PC)

Requirements: JDK 8+, Maven, GCC (MinGW on Windows), `make`.

```sh
# 1. Java toolchain (compiler + Java VM)
mvn -f miVM/pom.xml install
mvn -f lexer-java/pom.xml install

# 2. Host C VM  (add LVGL=1 for the GUI window; see bpgenvm-c/README)
cd bpgenvm-c && make && cd ..

# 3. Compile and run an example on BOTH VMs
java -jar lexer-java/target/basicplus-frontend.jar samples/blink.bp \
     --compile samples --backend=mivm
java -jar miVM/target/bpgenvm-1.0.jar samples/Blink.mod
bpgenvm-c/build/bpgenvm-c samples/Blink.mod

# 4. (Optional) the IDE
mvn -f BpIde/pom.xml package
java -jar BpIde/target/BpIde-3.0.jar
```

The firmwares are built with their usual toolchains (pico-sdk + ninja,
ESP-IDF, STM32CubeIDE); see `bpgenvm-c/{pico,esp32,esp32p4,stm32}/`. Or grab
the **prebuilt binaries** from the [latest release](https://github.com/legio-e/BasicPlus/releases/latest).

## Documentation

Documentation is available in **English** (`docs/en/`) and
**[Spanish](README.es.md)** (`docs/`).

- **[Quick start](docs/en/QUICKSTART.md)** — zero to blink, per platform.
- **[Installing the firmware](docs/en/INSTALAR_FIRMWARE.md)** — flashing
  the VM on each micro (prebuilt image or built by you).
- **[Language manual](docs/en/manual.html)** — lexicon, types, classes,
  modules, exceptions, concurrency.
- **[Reference](docs/en/referencia.html)** — the complete standard library,
  command line and on-disk artifacts.
- **[GUI guide](docs/en/gui.html)** — the `Gui` module: widgets, colors and
  fonts, forms (`.win`), running on host and board.
- **[IDE guide](docs/en/guia-ide.html)** — the window, projects, Run/Stop,
  the board explorer, the micro console and the debugger.
- **[BasicPlus from the inside](docs/bp-desde-dentro.html)** *(Spanish)* —
  the architecture: compiler, bytecode, the two VMs, GC, AOT and firmwares.
- [Project philosophy](docs/PHILOSOPHY.md) *(Spanish)* — what it is and what
  it does not want to be.
- Specs *(Spanish)*: [.mod format](docs/MOD_FORMAT.md),
  [opcodes](docs/OPCODES.md), [heap layout](docs/HEAP_LAYOUT.md),
  [builtins](docs/BUILTINS.md), [wire protocol](docs/BPVM_WIRE_PROTOCOL.md).
- [Living backlog](docs/PENDIENTES.md) *(Spanish)* — the project's honest
  diary.

## Status

**V3 — graphical interfaces** (July 2026). V1 proved the idea; V2 hardened and
broadened it; **V3 gives it a face**: a `Gui` module over LVGL, screens designed
in JSON, and three new boards with a display (STM32U5 Discovery, ESP32-P4-EV,
Waveshare ESP32-P4) — the same bytecode, now with graphics and touch, verified
on real hardware.

Downloads (7 prebuilt binaries) and full detail: the
**[v3.0 release](https://github.com/legio-e/BasicPlus/releases/tag/v3.0)** and the
**[release notes](docs/RELEASES.md)** *(Spanish)*.

## License

[MIT](LICENSE). Made with care, boards on the desk, and memories of the
PDP-11.
