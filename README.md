# BasicPlus

**Object-oriented BASIC for 32-bit microcontrollers.** Write and debug your
program on the PC; the same bytecode then runs, byte for byte, on a Raspberry
Pi Pico 2, an ESP32 or an STM32 — no recompiling, no `#ifdef`.

[![Latest release](https://img.shields.io/github/v/release/micro32-suite/BasicPlus?label=release)](https://github.com/micro32-suite/BasicPlus/releases/latest)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
[![Tested on 11 boards](https://img.shields.io/badge/tested%20on-11%20boards-brightgreen)](https://micro32-suite.github.io/BasicPlus/en/pruebas.html)
[![Docs in English and Spanish](https://img.shields.io/badge/docs-English%20%C2%B7%20Espa%C3%B1ol-informational)](https://micro32-suite.github.io/BasicPlus/en/)

**[⬇️ Download for Windows](https://github.com/micro32-suite/BasicPlus/releases/latest)** ·
**[📖 Documentation](https://micro32-suite.github.io/BasicPlus/en/)** ·
**[🚀 Quick start](docs/en/QUICKSTART.md)** ·
🇪🇸 **[Versión en español](README.es.md)**

![The IDE with the GuiColorDemo sample open and, on the right, the simulated micro running it](docs/img/BpIde_VM_GuiColorDemo.png)

*The IDE with the `GuiColorDemo` sample and, on the right, the simulated micro
running it: the same bytecode paints the screen of a board.*

```basic
// blink.bp — blinks the Pico's on-board LED (GP25)
module Blink
  import Gpio

  function Main()
    var led: Gpio.Pin := Gpio.Pin(25, Gpio.Pin.OUTPUT)
    var i: integer := 0
    while i < 10 do
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

On a PC, with no GPIO, the hardware calls log to the console; on the board
they drive real pins. The program is the same, byte for byte.

<details>
<summary>The same program in <b>JC</b>, for those who come from Java or C#</summary>

```java jc
// blink.jc — the same program in JC
module Blink
{
   import Gpio;

   function void Main()
   {
      Gpio.Pin led = new Gpio.Pin(25, Gpio.Pin.OUTPUT);
      for (int i = 1; i <= 10; i++)
      {
         led.on();
         sleep(300);
         led.off();
         sleep(300);
      }
      println("Blink done");
   }
}
```

Same compiler, same bytecode, same output; the IDE translates a program from
one syntax to the other.

</details>

## Try it (Windows, nothing to build)

1. Have **Java 8 or later** installed.
2. Download **`Micro32-Suite-8.1-win.zip`** from the
   [latest release](https://github.com/micro32-suite/BasicPlus/releases/latest)
   and unzip it anywhere.
3. Run **`bpide.bat`**. Open `samples/blink.bp`, press **Compile** and then
   **Run**: the output shows up in the console. For the graphical interface, do
   the same with `samples/GuiColorDemo.bp`, using **Run Window**.

No board is needed to start. When you have one, the ZIP also carries the nine
firmware images and the packs: **[Installing the
firmware](docs/en/INSTALAR_FIRMWARE.md)** says what to burn on each board, and
**Run on Device** does the rest. On Linux or macOS, [build it from
source](#build-from-source).

## Why BasicPlus

- **Debug on the PC, deploy on the micro.** Two independent virtual machines,
  one in Java and one in C99, run the same bytecode, and a program's output
  must be byte-identical on both. Every feature is checked against that before
  it gets in.
- **Easy to read, without treating you like a beginner**: classes and
  inheritance, interfaces and generics, exceptions, modules, tuples,
  properties; and threads — on the dual-core boards, two of them run truly at
  the same time.
- **The hardware is a class**: `Gpio`, `I2c`, `Spi`, `Uart`, `Pwm`, `Adc`,
  `Neopixel`… (`Gpio.Pin`, `I2c.Bus`, `Net.Tcp`), and whatever a program takes
  is released when it ends. **SQLite**, with its ORM, comes as a pack.
- **A graphical interface on the board**: LVGL, with about twenty widgets,
  screens designed in a JSON file, and touch. On the PC, the same program
  paints a window.
- **WiFi and Bluetooth** on the ESP32s with PSRAM: the IDE uploads, burns and
  runs over the network; from Bluetooth, a keyboard and a mouse.
- **Native code where it counts**: a `native function` is compiled ahead of
  time to Thumb-2 or RISC-V and travels inside the bytecode file — 40-90× the
  interpreter on compute kernels.
- **Two syntaxes, one compiler**: BasicPlus, and **JC** for those who come from
  Java or C#.
- **Tested on real hardware**: the V8 test round passed on all eleven boards,
  and four of them ran a program that touches everything for 48 hours in a
  row, without a single error or restart. Every test, with its numbers, in
  **[How BasicPlus is tested](https://micro32-suite.github.io/BasicPlus/en/pruebas.html)**.

## Boards

Nine firmware images, eleven boards, three architectures — and the PC
(Windows, Linux or macOS), which runs both VMs and shows the graphical
interface in a window.

| Board | Chip | Screen (LVGL) | WiFi · Bluetooth | `native` (AOT) |
|---|---|---|---|---|
| Raspberry Pi **Pico 2** (and 2 W) · Adafruit **Metro RP2350** | RP2350 — Cortex-M33, 2 cores | — | — | Thumb-2 |
| **ESP32-S3** DevKitC | ESP32-S3 — Xtensa, 2 cores | — | ✅ | interpreted |
| **ESP32-C3** board | ESP32-C3 — RISC-V | — | — | RISC-V |
| Waveshare **ESP32-C6-LCD-1.3** | ESP32-C6 — RISC-V | 240×240, SPI | — | RISC-V |
| Waveshare **ESP32-C5-Touch-LCD-2.8** | ESP32-C5 — RISC-V | 240×320, SPI, touch | ✅ | RISC-V |
| Waveshare **ESP32-P4** 4.3" | ESP32-P4 — RISC-V, 2 cores | 480×800, MIPI-DSI, touch | ✅ (through its ESP32-C6) | RISC-V |
| Espressif **ESP32-P4-Function-EV** · **ESP32-P4X-Function-EV** | ESP32-P4 · P4X — RISC-V, 2 cores | 1024×600, MIPI-DSI, touch | untested | RISC-V |
| ST **Nucleo-U575ZI-Q** | STM32U575 — Cortex-M33 | — | — | Thumb-2 |
| ST **Discovery STM32U5G9J-DK2** | STM32U5G9 — Cortex-M33 | 800×480, LTDC, touch | — | Thumb-2 |

WiFi and Bluetooth need PSRAM, which the C3 and the C6 lack. The buses were
validated with real sensors on the three families without a screen — a BMP280
and a BME280 over I2C, a BME688 over SPI, UART loopbacks —, and on the Pico 2
the PWM, counted in hardware, with less than 0.1 % error. Flashing, the
board's configuration and the packs each one needs: **[Installing the
firmware](docs/en/INSTALAR_FIRMWARE.md)**; first steps, board by board:
**[Quick start](docs/en/QUICKSTART.md)**.

## How it works

The compiler turns a `.bp` (or a `.jc`) into a `.mod`: the bytecode, the
module's interface for `import` and, inside, the native code of its `native`
functions — one file per module. That file is executed by **two independent
virtual machines**:

| VM | Language | Where it runs |
|---|---|---|
| **miVM** | Java | The PC: development, debugging, the tests |
| **bpgenvm-c** | C99 | The PC too, and the boards' firmware |

The project's golden rule: **a program's output must be byte-identical on both
VMs**. Every feature is done first in Java, then in C — with its output
compared byte for byte against Java's — and only then does it go to the
boards. That is what makes "debug on the PC, deploy on the micro" more than a
slogan.

## The language

- **Full object orientation**: classes with constructors and destructors
  (`~Class()`), inheritance (across modules too), interfaces, generics,
  `instanceof`, properties with `get`/`set`, `sync` (atomic) properties,
  static members. The root classes (`Object`, `Thread`, `Mutex`,
  `StringBuilder`) are written in BasicPlus itself, in `Core`.
- **Types**: `integer`, `long` (i64), `float`, `double`, `boolean`, `string`
  (native UTF-8, codepoint-indexed), narrow types (`byte`, `word`, `short`,
  `int8`, `int16`), arrays (`integer[]`, `byte[]`, `long[]`, …) and fixed-size
  arrays (`var buf: byte[64]`), in a function or inside an object.
- First-class **tuples** (`(integer, string)`), **default parameter values**,
  compound operators and the ternary (`iif`), multi-line expressions,
  `switch/case`.
- **Exceptions**: `try/catch/finally`, a hierarchy with a common `Exception`
  base, the VM's errors with their type (`ArithmeticError`, `IndexError`…),
  user-defined exception classes — across modules too.
- **Concurrency**: BP threads on a quantum scheduler — on the dual-core boards,
  two of them truly at the same time, one per core —, `Mutex`, `synchronized`,
  `SyncList` queues with blocking consume.
- **Modules** (the Modula-2 DEFINITION model): `import` resolves against the
  module's compiled interface, which travels inside the `.mod` itself.
- **`native function`**: compiled ahead of time to Thumb-2 (RP2350, STM32) or
  RISC-V (ESP32-P4, C3, C5 and C6), with faults propagated to BP
  `try/catch`. Inside, almost the whole language: any builtin, calls to BP,
  `try/catch/finally`; and a long one yields to the other threads and stops
  with Stop.
- **Graphical interface**: the `Gui` module over **LVGL** — ~20 OO widgets,
  forms designed in JSON (`.win`), color and fonts, touch. Since V7 LVGL comes
  in a **pack** that you burn on the board: a board without a screen doesn't
  pay for it.
- **Standard library**: `Core`, `Math`, `IO` (files with `IO.File` and
  `IO.TextFile`, prompt), `Str`, `Collections` (with a typed `Map`), `Stats`,
  `Compress` (LZSS), `Log`, `Json`, `Net` (TCP client), `Air` (WiFi and
  Bluetooth), `App`, `Packs`, `Machine` (chip id, temperature, clock, reset),
  `Time` (dates), `Heap` (memory) and the hardware zoo: `Gpio`, `I2c`, `Spi`,
  `Uart`, `Pwm`, `Adc`, `Rtc`, `Wdt`, `Timer`, `Pulse`, `Freq`, `Neopixel`.

The whole language, in the **[manual](https://micro32-suite.github.io/BasicPlus/en/manual.html)**;
what it does not do yet, stated plainly, in the
[known limitations](docs/PENDIENTES.md) *(Spanish)*.

## The IDE and the board

`BpIde` is a single jar (Swing): a tabbed editor with diagnostics, Run and
Debug on the PC or inside the board — breakpoints and stepping in both —, and
a board explorer with its files, its console (`dir`, `run`, `kill`, `autorun`,
`log`, …) and its management: partitions, the board's environment and the
**Packs** panel, from which the screen, SQLite or the standard library are
burned. It reaches the board by cable or, on the boards with WiFi, over the
network. And when there is no board at hand, a **simulated micro**, with its
screen.

On the board, **Stop** halts a program without resetting it, and **autorun**
(`/sys/auto.txt`) starts your program on power-up — a truly standalone
device —; the IDE can attach later and take control back. The cable has an OS
thread of its own, apart from the program: the IDE keeps talking to the board
while a program runs.

## Build from source

Requirements: JDK 8+, Maven, GCC (MinGW on Windows), `make`.

```sh
# 0. Clone it WITH its submodules (toolchain/ and bpgenvm-c/); if you already
#    cloned it without them: git submodule update --init
git clone --recursive https://github.com/micro32-suite/BasicPlus.git
cd BasicPlus

# 1. Java toolchain (the pack format, the Java VM and the compiler), in this order
mvn -f toolchain/pack/pom.xml install
mvn -f toolchain/miVM/pom.xml install
mvn -f toolchain/lexer-java/pom.xml install

# 2. Host C VM  (add LVGL=1 for the GUI window; see bpgenvm-c/README)
cd bpgenvm-c && make && cd ..

# 3. Compile and run an example on BOTH VMs
java -jar toolchain/lexer-java/target/basicplus-frontend.jar samples/blink.bp \
     --compile samples --backend=mivm
java -jar toolchain/miVM/target/bpgenvm-1.0.jar samples/Blink.mod
bpgenvm-c/build/bpgenvm-c samples/Blink.mod

# 4. (Optional) the IDE
mvn -f toolchain/BpIde/pom.xml package
java -jar toolchain/BpIde/target/BpIde-8.1.jar
```

The firmwares are built with their usual toolchains (pico-sdk + ninja,
ESP-IDF, STM32CubeIDE); see `bpgenvm-c/{pico,esp32,esp32c3,esp32c5,esp32c6,esp32p4,stm32}/`.
The GUI packs (`Gui`, `GuiMini`, `DriverSpi`) are built from `bpstdlib/gui/`
(each `.bpbuild` says how). Or grab the **prebuilt binaries** — images and
packs — from the [latest release](https://github.com/micro32-suite/BasicPlus/releases/latest).

## Repository layout

Since V8.1 the project is a suite of three repositories in the
[micro32-suite](https://github.com/micro32-suite) organization: this one,
[toolchain](https://github.com/micro32-suite/toolchain) (the Java side) and
[runtime](https://github.com/micro32-suite/runtime) (the C side), which this
one brings in as submodules.

```
toolchain/    the Java side — submodule: micro32-suite/toolchain
  pack/         the pack format
  miVM/         Java VM + debugger + TCP daemon
  lexer-java/   BP and JC compiler: .bp/.jc → .mod + .dbg (native code inside the .mod)
  BpIde/        Swing IDE (fat-jar)
bpgenvm-c/    C99 VM: host + firmwares for RP2350, ESP32-S3/C3/C5/C6/P4, STM32
              — submodule: micro32-suite/runtime
bpstdlib/     standard library (.bp sources + compiled .mod); the GUI packs, in gui/
packs/        the packs; in the repo, only Stdlib.pack (the others are built or come in the release)
samples/      example programs
compat/       the test net: dual-VM parity, the old formats, the endurance test
scripts/      the packager, the generators and the tools for the docs and the corpus
docs/         manual, GUI guide, specs (.mod, opcodes, heap, wire), backlog
```

## Documentation

In **English** and **[Spanish](README.es.md)**, also on the
**[web](https://micro32-suite.github.io/BasicPlus/en/)**.

- **[Quick start](docs/en/QUICKSTART.md)** — zero to blink, per platform.
- **[Installing the firmware](docs/en/INSTALAR_FIRMWARE.md)** — flashing the
  VM on each micro (prebuilt image or built by you), and the packs a board
  with a screen needs.
- **[Language manual](https://micro32-suite.github.io/BasicPlus/en/manual.html)** —
  lexicon, types, classes, modules, exceptions, concurrency.
- **[JC, BasicPlus with braces](https://micro32-suite.github.io/BasicPlus/en/jc.html)** —
  the Java and C# syntax on the same compiler and the same bytecode, and how to
  translate between them.
- **[Reference](https://micro32-suite.github.io/BasicPlus/en/referencia.html)** —
  the complete standard library, command line and on-disk artifacts.
- **[GUI guide](https://micro32-suite.github.io/BasicPlus/en/gui.html)** — the
  `Gui` module: widgets, colors and fonts, forms (`.win`), running on host and
  board.
- **[IDE guide](https://micro32-suite.github.io/BasicPlus/en/guia-ide.html)** —
  the window, projects, Run/Stop, the board explorer, the micro console and the
  debugger.
- **[How BasicPlus is tested](https://micro32-suite.github.io/BasicPlus/en/pruebas.html)** —
  every test each version passes, on the PC and on the boards, with its
  numbers.
- **[BasicPlus from the inside](https://micro32-suite.github.io/BasicPlus/bp-desde-dentro.html)**
  *(Spanish)* — the architecture: compiler, bytecode, the two VMs, GC, AOT and
  firmwares.
- [Project philosophy](docs/PHILOSOPHY.md) *(Spanish)* — what it is and what it
  does not want to be.
- Specs *(Spanish)*: [.mod format](docs/MOD_FORMAT.md),
  [opcodes](docs/OPCODES.md), [heap layout](docs/HEAP_LAYOUT.md),
  [builtins](docs/BUILTINS.md), [wire protocol](docs/BPVM_WIRE_PROTOCOL.md).

## Status

**Version 8.1** (November 2026). V1 proved the idea; V2 hardened and broadened
it; V3 gave it a face; V4 fixed its foundations; V5 gave it somewhere to put
things; V6 put the microcontroller code in order; V7 left one image per micro;
**V8 connects it**: WiFi and Bluetooth, JC and one more board, the ESP32-P4X.
V8.1 changed nothing in the product: it reorganized the project as a suite.

Each version, in the [releases](https://github.com/micro32-suite/BasicPlus/releases)
and the [release notes](docs/RELEASES.md) *(Spanish)*.

## Feedback

Questions, ideas or a bug: [open an issue](https://github.com/micro32-suite/BasicPlus/issues)
or [start a discussion](https://github.com/micro32-suite/BasicPlus/discussions),
in English or in Spanish.

## The name (and a debt of gratitude)

BasicPlus takes its name from **Digital Equipment Corporation's BASIC-PLUS**,
the structured BASIC dialect that ran under RSTS/E on PDP-11s through the 70s
and 80s. That language proved a BASIC could be serious: whole systems were
written in it, and a generation learned to program with it. This project is a
homage to that idea — **a language that is easy to read but doesn't treat you
like a beginner** — transplanted to today's small hardware: 32-bit
microcontrollers.

## License

[MIT](LICENSE). Made with care, boards on the desk, and memories of the
PDP-11.
