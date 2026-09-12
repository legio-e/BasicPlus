# Installing the BasicPlus firmware on each micro

> 🇪🇸 [Versión en español](../INSTALAR_FIRMWARE.md)

How to put the VM image on each supported board. There are two routes:

- **A — Prebuilt image** (the normal one): **you already have it**. The
  seven images (nine boards) ship inside the package, in the `firmware/`
  folder; flash them with each family's tool — no toolchain needed. (If you
  don't have the package, download it from the GitHub *release*.)
- **B — Build it yourself**: for firmware hackers. The prerequisites are in
  `bpgenvm-c/{pico,esp32,stm32}/README.md`; the ESP32-P4, C3 and C6 ports
  share the S3's `main` (`bpgenvm-c/esp32/common/`) and what is specific to
  each lives in its `sdkconfig.defaults` and `main/chip_cfg.h`.

| image | boards |
|---|---|
| `bpvm_pico.uf2` | Raspberry Pi Pico 2 · Adafruit Metro RP2350 |
| `bpvm_esp32_merged.bin` | ESP32-S3 |
| `bpvm_esp32c3_merged.bin` | ESP32-C3 |
| `bpvm_esp32c6_merged.bin` | ESP32-C6 (Waveshare ESP32-C6-LCD-1.3) |
| `bpvm_esp32p4_merged.bin` | ESP32-P4-Function-EV · Waveshare ESP32-P4 4.3" |
| `bpvm_stm32_nucleo.bin` | Nucleo-U575ZI-Q |
| `bpvm_stm32_dk2.bin` | Discovery STM32U5G9J-DK2 |

After flashing, the flow is the same everywhere: IDE → Explorer →
**Connect** to the serial port → Run on Device / console / `autorun` (see
[QUICKSTART](QUICKSTART.md)).

> The firmware is only reflashed when the **VM** changes. Your programs
> (`.mod`) live in the board's internal filesystem and are uploaded from
> the IDE — updating your application never touches the firmware. FS files
> **survive** a reflash (they live in a separate flash region). The stdlib
> in `/lib` is restored by the firmware itself at boot (missing, older or
> different → the copy inside the image); `/app` is never touched — do not
> leave copies of stdlib modules there, they would shadow the ones in `/lib`.

> Board configuration comes in two halves. **Identity and pins** (`name`,
> `ledPin`, `neopixelPin`) → `/sys/board.json`, and only on RP2350.
> **PSRAM and panel** → the board's **ENV** (the IDE's **Entorno**
> (environment) button): `psram=1` on the Metro, `display=st7701` on the
> Waveshare P4. The ENV lives in its own partition and survives a reflash.

---

## Raspberry Pi Pico 2 / Adafruit Metro RP2350 — `bpvm_pico.uf2`

**A single image for both boards**: the chip variant (A/B) is detected from
the hardware at boot; pins and PSRAM are decided at runtime, not at build
time.

### A. Prebuilt image (BOOTSEL, nothing to install)

1. Unplug the board from USB.
2. Hold the **BOOTSEL** button while plugging it in → a USB drive named
   `RPI-RP2` appears.
3. Copy `bpvm_pico.uf2` to that drive. The board reboots by itself with
   the VM inside: done.

On the **Metro RP2350** the 48 GPIOs come on their own (variant B detected).
Afterwards, two settings:

- **PSRAM** (8 MB as heap): `psram=1` in the ENV — the IDE's **Entorno**
  button, PSRAM checkbox — and `reset`. RP2350B only; the CS is fixed (GP47).
- **Identity and pins**: upload `firmware/boards/metro-rp2350b.json` (from the package;
  `bpgenvm-c/pico/boards/` in the repo)
  as `/sys/board.json` (`name`, `ledPin`, `neopixelPin`; its `psramCsPin`
  is ignored). Without the file the board is called `generic` and does not
  light the NeoPixel at boot; everything else works the same.

### B. Building it

```sh
cd bpgenvm-c/pico && mkdir -p build && cd build
cmake -G Ninja -DPICO_BOARD=bp_rp2350b \
      -DFREERTOS_KERNEL_PATH=<path>/FreeRTOS-Kernel ..
ninja bpvm_pico        # → build/bpvm_pico.uf2
```

Prerequisites (pico-sdk, ARM toolchain) in `bpgenvm-c/pico/README.md`.

---

## ESP32-S3 — `bpvm_esp32_merged.bin`

The DevKit has **two USB ports**: flash and connect the IDE through the
**UART bridge** one (CP210x/CH340); the native USB is log console only.

### A. Prebuilt image (esptool)

`esptool` ships with the ESP-IDF (or `pip install esptool`). **With IDF v6
it is esptool v5**: the executable is `esptool` (not `esptool.py`) and the
subcommands use a DASH (`write-flash`, `merge-bin`, `--flash-size`). The
release ships the **merged** image (bootloader + partition table + app in a
single file, to be flashed at offset 0):

```sh
esptool --chip esp32s3 -p <bridge-port> write-flash 0 bpvm_esp32_merged.bin
```

### B. Building it (ESP-IDF v6.x)

```sh
cd bpgenvm-c/esp32
idf.py build
idf.py -p <bridge-port> flash      # flashes the 3 pieces at their offsets
```

To regenerate the release's merged image from the build:

```sh
cd build
esptool --chip esp32s3 merge-bin -o bpvm_esp32_merged.bin \
    --flash-mode dio --flash-freq 80m --flash-size 16MB \
    0x0 bootloader/bootloader.bin \
    0x8000 partition_table/partition-table.bin \
    0x10000 bpvm_esp32.bin
```

⚠️ The `idf.py merge-bin` shortcut writes `build/merged-binary.bin`, **not**
`bpvm_esp32_merged.bin`: if a `_merged.bin` was already sitting there, the
old one stays. Check the date of the file you flash. (Applies to all four
ESP32 images.)

The module's 8 MB of PSRAM (WROOM-1) is enabled on its own: nothing to
configure.

---

## ESP32-C3 and ESP32-C6 — `bpvm_esp32c3_merged.bin` · `bpvm_esp32c6_merged.bin`

Both have **a single USB connector**, the native USB-Serial-JTAG, and the
**wire** goes through it (the other way round from the S3). The log console
comes out on UART0 (C3: GPIO20/21; C6: GPIO16/17) and is only needed with an
adapter; the boot log can be read from the IDE (`log`). The C6 image is the
one for the **Waveshare ESP32-C6-LCD-1.3** (ST7789 240×240 SPI display, no
touch).

### A. Prebuilt image (esptool)

Download mode is entered **by hand** with the two buttons: hold BOOT, press
and release RESET, release BOOT. The port is still the same USB.

```sh
esptool --chip esp32c3 -p <port> write-flash 0 bpvm_esp32c3_merged.bin
esptool --chip esp32c6 -p <port> write-flash 0 bpvm_esp32c6_merged.bin
```

When done, press **RESET** (alone): `esptool`'s automatic reset does not
always take the chip out of download mode, and in download mode nothing
answers.

### B. Building it (ESP-IDF v6.x)

```sh
cd bpgenvm-c/esp32c3        # or esp32c6
idf.py set-target esp32c3   # or esp32c6 (first time only)
idf.py build
idf.py -p <port> flash      # with the board in download mode (BOOT+RESET)
```

Merged image: the offsets are in `build/flash_args` (0x0 bootloader, 0x8000
partition table, 0x10000 app; 4 MB flash at 80 MHz):

```sh
cd build
esptool --chip esp32c3 merge-bin -o bpvm_esp32c3_merged.bin \
    --flash-mode dio --flash-freq 80m --flash-size 4MB \
    0x0 bootloader/bootloader.bin \
    0x8000 partition_table/partition-table.bin \
    0x10000 bpvm_esp32c3.bin
```

(For the C6, `esp32c6` and `bpvm_esp32c6.bin`.)

---

## ESP32-P4 (display boards) — `bpvm_esp32p4_merged.bin`

**A single image for the supported P4 boards** (the 7" 1024×600
ESP32-P4-Function-EV and the 4.3" 480×800 Waveshare Touch-LCD): the panel is
chosen at runtime from the board's **ENV** (`display` key: `ek79007` or
`st7701`); with no key, it boots with the EV profile. On the Waveshare, once:
the IDE's **Entorno** button → `display=st7701` → `reset`. The ENV survives a
reflash, so there is no need to repeat it ([GUI guide
§23.4](gui.html#ej-pantalla)). `/sys/board.json` is not read on the P4.

The wire (IDE) goes through the board's **USB-UART bridge** port.

### A. Prebuilt image (esptool)

```sh
esptool --chip esp32p4 -p <bridge-port> write-flash 0 bpvm_esp32p4_merged.bin
```

### B. Building it (ESP-IDF v6.x)

```sh
cd bpgenvm-c/esp32p4
idf.py build
idf.py -p <bridge-port> flash      # flashes the pieces at their offsets
```

To regenerate the merged image, the exact build offsets are in
`build/flash_args` (`esptool merge-bin` with that list; on the P4 the
bootloader goes at 0x2000 and the flash runs at 40 MHz, unlike the S3). The
port notes (ESP-IDF v6.0.1, silicon revision, PSRAM) are in
`bpgenvm-c/esp32p4/sdkconfig.defaults`.

---

## STM32 — two boards, two images

| board | image | display |
|---|---|---|
| **Nucleo-U575ZI-Q** (c1) | `bpvm_stm32_nucleo.bin` | no |
| **Discovery STM32U5G9J-DK2** (c2) | `bpvm_stm32_dk2.bin` | yes (LVGL GUI) |

On both, the **on-board ST-LINK acts as the programmer**: the same USB cable is used for
flashing and for the wire (VCP).

### A. Prebuilt image

Two options, simplest first:

1. **Drag and drop**: the board shows up as a USB drive (`NOD_U575ZI` on the Nucleo,
   `DIS_U5G9J` on the Discovery). Copy the matching `.bin` onto that drive and the
   ST-LINK flashes it (its LED blinks while writing).
2. **STM32CubeProgrammer** (free, GUI or CLI). From the command line:
   ```
   STM32_Programmer_CLI -c port=SWD -d bpvm_stm32_nucleo.bin 0x08000000 -hardRst
   ```
   This is the robust route if drag and drop misbehaves. With two boards connected,
   pick the probe with `port=SWD sn=<serial>`.

⚠️ **Do not mix the two images.** Same family, different boards: the Discovery one carries
display support and the Nucleo one does not. Flashing the wrong one breaks nothing, but
the board will not do what you expect.

### B. Building it

The port is built inside an **STM32CubeIDE** project (one per board); the integration
guide (include paths, linked core folder, source list) is in
`bpgenvm-c/stm32/port/README.md`. The resulting binary is flashed from CubeIDE itself
(Run/Debug) or by exporting the `.bin`.
---

## Which version is on my board?

IDE → Connect → **INFO** button: it shows the micro, firmware build
(`serverBuild`), flash, RAM and PSRAM. The build date identifies the
installed image.
