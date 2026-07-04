# BasicPlus — quick start

> 🇪🇸 [Versión en español](../QUICKSTART.md)

From zero to a blinking LED (or a "hello" on the console) in a few minutes,
platform by platform. For the fine detail of each port: the READMEs in
`bpgenvm-c/pico`, `bpgenvm-c/esp32`, `bpgenvm-c/esp32p4` and
`bpgenvm-c/stm32`.

---

## 1. PC (no board) — 5 minutes

Requirements: JDK 8+, Maven, GCC (MinGW on Windows), `make`. Optional, for
`function native` on RP2350/STM32: the **Arm GNU Toolchain**
(`arm-none-eabi-gcc`) — installation and setup in the
[IDE guide](guia-ide.html#aot).

```sh
# Toolchain (compiler + Java VM) and host C VM
mvn -f miVM/pom.xml install
mvn -f lexer-java/pom.xml install
cd bpgenvm-c && make && cd ..

# Compile and run the blink on BOTH VMs (on a PC the GPIOs log)
java -jar lexer-java/target/basicplus-frontend.jar samples/blink.bp \
     --compile samples --backend=mivm
java -jar miVM/target/bpgenvm-1.0.jar samples/Blink.mod
bpgenvm-c/build/bpgenvm-c samples/Blink.mod
```

If the two outputs are not identical, that is a bug on our side: dual-VM
parity is the project's invariant.

**The IDE** (recommended for everything that follows):

```sh
mvn -f BpIde/pom.xml package
java -jar BpIde/target/BpIde-1.0-SNAPSHOT-shaded.jar
```

Open `samples/blink.bp`, **Compile** button, **Run** menu — the output
shows up in the lower console. **Run → Stop (Ctrl+F2)** aborts a running
program.

---

## 2. Raspberry Pi Pico 2 / Adafruit Metro RP2350

**A single firmware image serves both boards** — chip variant, pins and
PSRAM are detected at runtime.

**Flashing**: hold BOOTSEL while plugging the USB → an `RPI-RP2` drive
appears → copy `bpvm_pico.uf2` → the board reboots by itself with the VM
inside. Where to get/build the image and the Metro case:
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md).

**First program from the IDE**:

1. Open the IDE → lower panel (*Explorer*) → pick the board's COM port →
   **Connect**. You will see the micro's file tree (`/lib` with the
   stdlib, `/app`, `/sys`).
2. Open `samples/blink.bp` → **Run on Device**. The IDE uploads the
   `.mod` (and any missing dependencies) and the LED blinks. The program
   output arrives at the console.
3. In the Explorer console: `help` lists the commands (`dir`, `run`,
   `kill`, `autorun`, `log`, `save`, `reset`…).

**Making it standalone**: with your program already on the board,

```
/> autorun Blink      ← writes /sys/auto.txt and persists it
/> reset
```

From then on the board starts your program by itself when powered, no PC.
The IDE can still connect while the program runs: `kill` stops it,
`autorun off` removes it. Reflashing is never needed.

**Metro RP2350 (variant B)**: upload a `/sys/board.json` declaring the
board (variant, ledPin, neopixelPin, psramCsPin) — with it, the firmware
enables the 48 GPIOs, the NeoPixel and the 8 MB of PSRAM as heap.

---

## 3. ESP32-S3

Mind the DevKit's **two USB ports**! The **wire** (what the IDE uses) goes
through the **UART0 bridge**; the native USB (USB-Serial-JTAG) is log
console only. Details in `bpgenvm-c/esp32/README.md`.

**Flashing**: with the merged image from the release and `esptool`
(`pip install esptool`) it is one command — see
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md) (also the `idf.py` route if you
build it yourself).

Then: IDE → Connect to the bridge port → same flow as on the Pico
(Run on Device, console, autorun). On the ESP32, `native function`s run
interpreted (AOT is ARM) and the `Net` module has no backend yet (it will
come with lwIP).

---

## 4. ESP32-P4 (display boards)

**A single image serves the supported P4 boards** (the 7"
ESP32-P4-Function-EV and the 4.3" Waveshare Touch-LCD): the panel is
chosen at runtime via `/sys/board.json` — with no file, it boots with the
EV profile.

**Flashing**: with the merged image and `esptool` — see
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md). The wire (what the IDE uses)
goes through the **USB-UART bridge** port, as on the S3.

Then: IDE → Connect → same flow (Run on Device, console, autorun). And
**the display**: GUI programs (`import Gui`) paint on the touch panel —
full detail in the [GUI guide](gui.html). To pick the panel on a board
other than the EV, run `samples/SetDisplay.bp` once
([guide §23.4](gui.html#ej-pantalla)). `native function`s run interpreted
(AOT is ARM).

---

## 5. STM32 (Nucleo-U575ZI-Q)

**Flashing**: the simplest is dragging the `.bin` onto the ST-LINK USB
drive (`NOD_U575ZI`); alternatives and building it yourself (CubeIDE) in
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md).

The wire goes through the **ST-LINK VCP** — the same USB cable. IDE →
Connect to that COM → Run on Device / console / autorun, same as on the
other boards. AOT enabled (same Cortex-M33 as the RP2350).

---

## What next?

- **[Language manual](manual.html)** and **[Reference](referencia.html)**
  (stdlib, CLI, on-disk artifacts) — the full documentation.
- **[Graphical interface](gui.html)** — the GUI on display boards
  (ESP32-P4, STM32-DK2): widgets, color and fonts, `.win` forms.
- `samples/` — examples: OO GPIO, I²C/SPI/UART, threads, exceptions,
  tuples, `native` AOT, TCP…
- `docs/PENDIENTES.md` *(Spanish)* — the project's living, honest backlog.
