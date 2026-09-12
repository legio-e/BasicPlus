# BasicPlus — quick start

> 🇪🇸 [Versión en español](../QUICKSTART.md)

From zero to a blinking LED (or a "hello" on the console) in a few minutes,
platform by platform: PC, RP2350 (Pico 2 / Metro), ESP32-S3, ESP32-C3 and
C6, ESP32-P4 and STM32 (Nucleo and Discovery). For the fine detail of each
port: the READMEs in `bpgenvm-c/pico`, `bpgenvm-c/esp32` and
`bpgenvm-c/stm32`, and the `sdkconfig.defaults` of `bpgenvm-c/esp32p4`,
`esp32c3` and `esp32c6`.

---

## 1. PC (no board) — 5 minutes

Requirements: JDK 8+, Maven, GCC (MinGW on Windows), `make`. Optional, for
`function native` on a board: the **Arm GNU Toolchain** (`arm-none-eabi-gcc`)
for RP2350/STM32 and the ESP-IDF RISC-V one (`riscv32-esp-elf-gcc`) for the
ESP32-P4 — installation and setup in the [IDE guide](guia-ide.html#aot).

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

If the program's output is not identical on both (each VM's banner lines do
differ), that is a bug on our side: dual-VM parity is the project's
invariant.

**The IDE** (recommended for everything that follows):

```sh
mvn -f BpIde/pom.xml package
java -jar BpIde/target/BpIde-6.0.jar     # or bpide.bat, at the repo root
```

Open `samples/blink.bp`, **Compile** button, **Run** menu — the output
shows up in the lower console. **Run → Stop (Ctrl+F2)** aborts a running
program.

---

## 2. Raspberry Pi Pico 2 / Adafruit Metro RP2350

**A single firmware image serves both boards** — the chip variant (A/B) is
detected by the firmware from the hardware; the rest is decided at runtime
(the Metro case, below).

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

**Metro RP2350 (variant B)**: the 48 GPIOs come on their own (variant
detected). The rest lives in two places:

- **PSRAM** (8 MB as heap): `psram=1` in the board's **ENV** — the IDE's
  **Entorno** (environment) button, PSRAM checkbox
  ([IDE guide §9.3](guia-ide.html#entorno)); the firmware reads it at boot,
  so `reset` afterwards.
- **Board identity and pins** (`name`, `ledPin`, `neopixelPin`): a
  `/sys/board.json` uploaded from the IDE; template in
  `firmware/boards/metro-rp2350b.json` in the package (`bpgenvm-c/pico/boards/` in the repo). `name` is what
  `Machine.getBoard()` returns; with `neopixelPin` the firmware lights the
  NeoPixel at boot. The template's `psramCsPin` is ignored (the CS is fixed,
  GP47).

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
(Run on Device, console, autorun). The module's 8 MB of PSRAM is used as VM
memory on its own. On the S3, `native function`s run interpreted (there is
no AOT for Xtensa) and the `Net` module has no backend yet.

---

## 4. ESP32-C3 and ESP32-C6

Both RISC-V boards have **a single USB connector** (the native
USB-Serial-JTAG) and the **wire** goes through it: one cable and the IDE
connects. The log console comes out on UART0 (C3: GPIO20/21; C6:
GPIO16/17) — only needed with an adapter, for debugging; the boot log can
be read from the IDE anyway (`log`).

**Flashing**: download mode is entered **by hand** — hold BOOT, press and
release RESET, release BOOT — and the merged image is written with
`esptool --chip esp32c3` (or `esp32c6`). When done, RESET again. Details in
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md).

Then: IDE → Connect to the only port → same flow (Run on Device, console,
autorun). The VM gets 128 KB on both. The **Waveshare C6
(ESP32-C6-LCD-1.3)** carries an ST7789 240×240 SPI display, no touch: GUI
programs paint on it ([GUI guide](gui.html)). `native function`s run
interpreted (the IDE has no RISC-V target without FPU yet).

---

## 5. ESP32-P4 (display boards)

**A single image serves the supported P4 boards** (the 7"
ESP32-P4-Function-EV and the 4.3" Waveshare Touch-LCD): the panel is
chosen at runtime with the `display` key of the board's **ENV** — with no
key, it boots with the EV profile.

**Flashing**: with the merged image and `esptool` — see
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md). The wire (what the IDE uses)
goes through the **USB-UART bridge** port, as on the S3.

Then: IDE → Connect → same flow (Run on Device, console, autorun). And
**the display**: GUI programs (`import Gui`) paint on the touch panel —
full detail in the [GUI guide](gui.html). On the Waveshare, set
`display=st7701` from the IDE's **Entorno** (environment) button and
`reset` ([guide §23.4](gui.html#ej-pantalla)). AOT enabled (RISC-V with
FPU): `native function`s are accelerated as on the ARM boards.

---

## 6. STM32 (Nucleo-U575ZI-Q and Discovery U5G9J-DK2)

Two boards, two images (`bpvm_stm32_nucleo.bin` and `bpvm_stm32_dk2.bin`).
**Flashing**: the simplest is dragging the `.bin` onto the ST-LINK USB
drive (`NOD_U575ZI` on the Nucleo, `DIS_U5G9J` on the Discovery);
alternatives (`STM32_Programmer_CLI`) and building it yourself (CubeIDE) in
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md).

The wire goes through the **ST-LINK VCP** — the same USB cable. IDE →
Connect to that COM → Run on Device / console / autorun, same as on the
other boards. AOT enabled (same Cortex-M33 as the RP2350). The **Discovery**
adds the 800×480 LTDC display with touch: GUI programs paint on it. This
family has no `Adc` and no `Wdt` (they throw `RuntimeError`).

---

## What next?

- **[Language manual](manual.html)** and **[Reference](referencia.html)**
  (stdlib, CLI, on-disk artifacts) — the full documentation.
- **[Graphical interface](gui.html)** — the GUI on display boards
  (ESP32-P4, STM32-DK2, ESP32-C6): widgets, color and fonts, `.win` forms.
- `samples/` — examples: OO GPIO, I²C/SPI/UART, threads, exceptions,
  tuples, `native` AOT, TCP…
- `docs/PENDIENTES.md` *(Spanish)* — the language's known limitations, stated plainly.
