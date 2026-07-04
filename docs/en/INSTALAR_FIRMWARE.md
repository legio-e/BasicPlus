# Installing the BasicPlus firmware on each micro

> 🇪🇸 [Versión en español](../INSTALAR_FIRMWARE.md)

How to put the VM image on each supported board. There are two routes:

- **A — Prebuilt image** (the normal one): download the binary from the
  GitHub *release* and flash it with each family's tool. No toolchain
  needed.
- **B — Build it yourself**: for firmware hackers. Each port has its
  README with the prerequisites (`bpgenvm-c/{pico,esp32,esp32p4,stm32}/`).

After flashing, the flow is the same everywhere: IDE → Explorer →
**Connect** to the serial port → Run on Device / console / `autorun` (see
[QUICKSTART](QUICKSTART.md)).

> The firmware is only reflashed when the **VM** changes. Your programs
> (`.mod`) live in the board's internal filesystem and are uploaded from
> the IDE — updating your application never touches the firmware. FS files
> **survive** a reflash (they live in a separate flash region).

---

## Raspberry Pi Pico 2 / Adafruit Metro RP2350 — `bpvm_pico.uf2`

**A single image for both boards**: chip variant (A/B), pins and PSRAM are
decided at runtime, not at build time.

### A. Prebuilt image (BOOTSEL, nothing to install)

1. Unplug the board from USB.
2. Hold the **BOOTSEL** button while plugging it in → a USB drive named
   `RPI-RP2` appears.
3. Copy `bpvm_pico.uf2` to that drive. The board reboots by itself with
   the VM inside: done.

On the **Metro RP2350**, afterwards upload a `/sys/board.json` from the
IDE (variant "B", ledPin, neopixelPin, psramCsPin) to enable the 48 GPIOs,
the NeoPixel and the 8 MB of PSRAM as heap. Without it, the image boots
with the conservative Pico profile.

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

`esptool` is a Python script (`pip install esptool`). The release ships
the **merged** image (bootloader + partition table + app in a single file,
to be flashed at offset 0):

```sh
esptool.py --chip esp32s3 -p <bridge-port> write_flash 0x0 bpvm_esp32_merged.bin
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
esptool.py --chip esp32s3 merge_bin -o bpvm_esp32_merged.bin \
    --flash_mode dio --flash_freq 80m --flash_size 2MB \
    0x0 bootloader/bootloader.bin \
    0x8000 partition_table/partition-table.bin \
    0x10000 bpvm_esp32.bin
```

---

## ESP32-P4 (display boards) — `bpvm_esp32p4_merged.bin`

**A single image for the supported P4 boards** (the 7" 1024×600
ESP32-P4-Function-EV and the 4.3" 480×800 Waveshare Touch-LCD): the panel
is chosen at runtime by reading `/sys/board.json` (the `"display"` key);
with no file, it boots with the EV profile. After flashing a board other
than the EV, run `samples/SetDisplay.bp` once from the IDE to write the
configuration ([GUI guide §23.4](gui.html#ej-pantalla)).

The wire (IDE) goes through the board's **USB-UART bridge** port.

### A. Prebuilt image (esptool)

```sh
esptool.py --chip esp32p4 -p <bridge-port> write_flash 0x0 bpvm_esp32p4_merged.bin
```

### B. Building it (ESP-IDF v6.x)

```sh
cd bpgenvm-c/esp32p4
idf.py build
idf.py -p <bridge-port> flash      # flashes the pieces at their offsets
```

To regenerate the merged image, the exact build offsets are in
`build/flash_args` (`esptool.py merge_bin` with that list). Prerequisites
and port notes (ESP-IDF v6.0.1, silicon revision) in
`bpgenvm-c/esp32p4/`.

---

## STM32 (Nucleo-U575ZI-Q) — `bpvm_stm32.bin` / `.hex`

The Nucleo's integrated ST-LINK acts as the programmer — the same USB
cable serves for flashing and for the wire (VCP).

### A. Prebuilt image

Two options, simplest first:

1. **Drag and drop**: the Nucleo shows up as a USB drive
   (`NOD_U575ZI`). Copy the `.bin` to that drive and the ST-LINK burns it
   by itself (the ST-LINK LED blinks while programming).
2. **STM32CubeProgrammer** (free, GUI or CLI): connect → open the
   `.hex`/`.bin` → *Download*. The robust route if drag&drop ever
   misbehaves.

### B. Building it

The port is built inside an **STM32CubeIDE** project; the integration
guide (include paths, linked core folder, source list) is in
`bpgenvm-c/stm32/port/README.md`. The resulting binary is flashed from
CubeIDE itself (Run/Debug) or by exporting the `.bin`.

---

## Which version is on my board?

IDE → Connect → **INFO** button: it shows the micro, firmware build
(`serverBuild`), flash, RAM and PSRAM. The build date identifies the
installed image.
