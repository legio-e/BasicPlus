# Installing the BasicPlus firmware on each micro

> 🇪🇸 [Versión en español](../INSTALAR_FIRMWARE.md)

How to put the VM image on each supported board —and, on the ones with a
display, their packs—. There are two routes:

- **A — Prebuilt image** (the normal one): **you already have it**. The
  eight images (ten boards) ship inside the package, in the `firmware/`
  folder, and the packs in `packs/`; flash them with each family's tool — no
  toolchain needed. (If you don't have the package, download it from the
  GitHub *release*.)
- **B — Build it yourself**: for firmware hackers. The prerequisites are in
  `bpgenvm-c/{pico,esp32,stm32}/README.md`; the ESP32-C3, C5 and C6 ports
  share the S3's `main` (`bpgenvm-c/esp32/common/`) and what is specific to
  each lives in its `sdkconfig.defaults` and `main/chip_cfg.h`; the P4 has
  its own `main.c` and shares the rest of `esp32/common/`.

| image | boards |
|---|---|
| `bpvm_pico.uf2` | Raspberry Pi Pico 2 · Adafruit Metro RP2350 |
| `bpvm_esp32_merged.bin` | ESP32-S3 |
| `bpvm_esp32c3_merged.bin` | ESP32-C3 |
| `bpvm_esp32c5_merged.bin` | ESP32-C5 (Waveshare ESP32-C5-Touch-LCD-2.8) — *new in V7* |
| `bpvm_esp32c6_merged.bin` | ESP32-C6 (and the Waveshare ESP32-C6-LCD-1.3, with its display through packs) |
| `bpvm_esp32p4_merged.bin` | ESP32-P4-Function-EV · Waveshare ESP32-P4 4.3" |
| `bpvm_stm32_nucleo.bin` | Nucleo-U575ZI-Q |
| `bpvm_stm32_dk2.bin` | Discovery STM32U5G9J-DK2 |

After flashing, the flow is the same everywhere: IDE → Explorer →
**Connect** to the serial port → Run on Device / console / `autorun` (see
[QUICKSTART](QUICKSTART.md)). And two more steps, the first time only:

- **A new board comes without partitions.** On connecting, the IDE offers
  to open **Gestión de placa** (board management): *Proponer defaults*
  (propose defaults) → *Aplicar tamaños* (apply sizes), and the board
  restarts with its
  filesystem and its pack zone ([IDE guide §9.2](guia-ide.html#particiones)).
  The C5 and the Waveshare P4, with 32 MB of flash, lay themselves out.
- **A board with a display needs its packs** —the C5, the C6, the P4s and
  the DK2—: since V7 the GUI is not in the image. See
  [The board's packs](#packs).

> **Coming from V6?** Flash the V7 image as usual: the filesystem, the
> partitions and the ENV stay. But on the display boards **the GUI is no
> longer in the image**: without its packs, the display does not paint. And
> any V6 packs you had (SQLite) are not valid in V7: format the zone and
> burn the ones in the package. All of it in [The board's packs](#packs).
> **And recompile your programs**: a V6 `.mod` that uses library classes
> does not link against the V7 library. The IDE recompiles on every Run;
> what to check is what was left in `/app` —the autorun program above all—:
> run it again from the IDE, or delete it ([v7.0 release
> notes](../RELEASES.md), *Spanish*).

> The firmware is only reflashed when the **VM** changes. Your programs
> (`.mod`) live in the board's internal filesystem and are uploaded from
> the IDE — updating your application never touches the firmware. FS files
> and packs **survive** a reflash (they live in a separate flash region),
> although packs with an engine —the GUI, SQLite— may need burning again
> ([when](#regrabar)). The stdlib in `/lib` is put there by the firmware
> itself at boot: a module that is missing, older or different → the copy
> inside the image; a newer one is left alone; and whatever the
> `Stdlib.pack` serves identically leaves `/lib` ([the `Stdlib.pack`](#stdlib-pack)).
> `/app` is never touched — do not leave copies of stdlib modules there,
> they would shadow the ones in `/lib`.

> Board configuration comes in two halves. **Identity and pins** (`name`,
> `ledPin`, `neopixelPin`) → `/sys/board.json`, and only on RP2350.
> **Everything else** → the board's **ENV** (the IDE's **Entorno**
> (environment) button): the PSRAM (`psram=1` on the Metro), the panel
> (`display=st7701` on the Waveshare P4; the SPI panel lines on the C5 and
> the C6), the RAM block for packs (`packRam`, `SQLite`), the card (`sd`)
> and the cores (`vm.runs`, `vm.cores`). The ENV lives in its own
> partition, is read at boot —after changing it, `reset`— and survives a
> reflash. The keys are in the [IDE guide §9.3](guia-ide.html#entorno).

<a id="nucleos"></a>
> **Two cores** (since V7). On the Pico 2, the Metro, the S3 and the P4s the
> image uses both cores of the micro and, by default, two *runs*: two threads
> of your program execute at the same time (on the C3, the C5, the C6 and the
> STM32s, one). A single-threaded program gains nothing —and on the Pico 2
> and the Metro, with two runs, it loses a little—: if that is your case,
> `vm.runs=1` in the ENV. A program being debugged runs with one run. And
> should anything go wrong with two cores, `vm.cores=1` and `vm.runs=1`
> leave the board on one without reflashing it.
> **Known limitation (`#665`, moved to V8):** on the Pico 2 and the Metro,
> the dual-core image makes a single-threaded program slower —on the Metro,
> by 35 %, also with `vm.cores=1`; on the Pico 2, PrintBench 1 270 ms
> against 817—; if you notice it, you can build the single-core one (below,
> *Building it*).

---

## Raspberry Pi Pico 2 / Adafruit Metro RP2350 — `bpvm_pico.uf2`

**A single image for both boards**: the chip variant (A/B) is detected from
the hardware at boot; pins and PSRAM are decided at runtime, not at build
time. It uses both cores of the RP2350 ([two cores](#nucleos)).

### A. Prebuilt image (BOOTSEL, nothing to install)

1. Unplug the board from USB.
2. Hold the **BOOTSEL** button while plugging it in → a USB drive named
   `RP2350` appears.
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

With two cores, a fatal system fault (a `HardFault`, for instance) restarts
the board: the report stays in the log (`log` in the console), **Info** gives
«fallo fatal» (fatal fault) as the cause of the last reset, and that boot
**does not launch the autorun** —a program that fails at startup would be a
reboot loop—: launch it by hand from the IDE.

### B. Building it

```sh
cd bpgenvm-c/pico && mkdir -p build && cd build
cmake -G Ninja -DPICO_BOARD=bp_rp2350b \
      -DFREERTOS_KERNEL_PATH=<path>/FreeRTOS-Kernel ..
ninja bpvm_pico        # → build/bpvm_pico.uf2
```

The image comes out dual-core; the single-core one, by adding
`-DBPVM_PICO_NUM_CORES=1` to the `cmake` line (going back by reflashing;
without reflashing, `vm.cores=1` and `vm.runs=1` in the ENV). Prerequisites
(pico-sdk, ARM toolchain) in `bpgenvm-c/pico/README.md`.

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
old one stays. Check the date of the file you flash. (Applies to all five
ESP32 images.)

The module's 8 MB of PSRAM (WROOM-1) is enabled on its own: nothing to
configure. Since V7 the S3 uses both its cores ([two cores](#nucleos)).

If the previous boot ended in a fatal fault —a panic (an exception) or the
interrupt watchdog—, the board **does not launch the autorun**: it says so
in the log (`log` in the console) and on its log console («[autorun] … NO
arranca: el arranque anterior acabo en un fallo fatal», it does not start:
the previous boot ended in a fatal fault), **Info** gives the cause of the
last reset (`panic/exception` or `int-watchdog`) and the program is launched
by hand from the IDE. The
watchdog of the `Wdt` module is not a fatal fault: after it, the autorun is
launched. (Applies to all five ESP32 images.)

---

## ESP32-C3 and ESP32-C6 — `bpvm_esp32c3_merged.bin` · `bpvm_esp32c6_merged.bin`

Both have **a single USB connector**, the native USB-Serial-JTAG, and the
**wire** goes through it (the other way round from the S3). The log console
comes out on UART0 (C3: GPIO20/21; C6: GPIO16/17) and is only needed with an
adapter; the boot log can be read from the IDE (`log`). The C6 image is the
one for any ESP32-C6; the **Waveshare ESP32-C6-LCD-1.3** (ST7789 240×240 SPI
display, no touch, and a TF slot) paints with its packs and a few lines in
the ENV ([The board's packs](#packs)). On both, `native` functions are compiled
and accelerated (AOT, `ilp32` ABI).

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

<a id="esp32-c5"></a>
## ESP32-C5 — `bpvm_esp32c5_merged.bin`

*New in V7.* The **Waveshare ESP32-C5-Touch-LCD-2.8**: one RISC-V core at
240 MHz, 8 MB of PSRAM, 32 MB of flash, a 2.8" touch display (ST7789 240×320
over SPI, CST3530 touch) and a TF slot. Like the C3 and the C6, it has **a
single USB connector**, the native USB-Serial-JTAG, and the **wire** goes
through it; the log console comes out on UART0 (GPIO11 TX, GPIO12 RX) and is
only needed with an adapter.

### A. Prebuilt image (esptool)

Download mode by hand, as on the C3 and the C6: hold BOOT, press and release
RESET, release BOOT.

```sh
esptool --chip esp32c5 -p <port> write-flash 0 bpvm_esp32c5_merged.bin
```

When done, press **RESET** (alone).

### B. Building it (ESP-IDF v6.x)

```sh
cd bpgenvm-c/esp32c5
idf.py set-target esp32c5   # first time only
idf.py build
idf.py -p <port> flash      # with the board in download mode (BOOT+RESET)
```

The merged image is **not made like the C3's or the C6's**: on the C5 the
bootloader goes at **0x2000** and the flash is **32 MB** at 80 MHz (as
`build/flash_args` says); the C3's offsets do not work.

```sh
cd build
esptool --chip esp32c5 merge-bin -o bpvm_esp32c5_merged.bin \
    --flash-mode dio --flash-freq 80m --flash-size 32MB \
    0x2000 bootloader/bootloader.bin \
    0x8000 partition_table/partition-table.bin \
    0x10000 bpvm_esp32c5.bin
```

### What is specific to the C5

- **The PSRAM** (8 MB) is enabled on its own and the VM lives in it: nothing
  to configure.
- **The flash**: the filesystem goes by itself into the top 16 MB and the
  pack zone takes the rest. There are no partitions to lay out: the
  partition editor's split does not apply on this board.
- **`native`**: compiled and accelerated (AOT, `ilp32` ABI), as on the C3
  and the C6.
- **The display**: `Gui.pack` + `DriverSpi.pack`, `packRam=512` and the panel
  lines in the ENV ([The board's packs](#packs)).
- **The TF slot**: `sd=bus:2,cs:23,khz:10000` in the ENV. It sits on the same
  SPI bus as the display (its `spi2=` line) and mounts on `/sd`
  ([reference, the SD card](referencia.html#stdlib-sd)).

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

**The display comes through a pack** (since V7): `Gui.pack` and a RAM block
in the ENV (`SQLite=4`, the same one SQLite uses) —see
[The board's packs](#packs)—. The MIPI-DSI panel and touch driver does live
in the image. The P4 uses both its cores ([two cores](#nucleos)). The
Waveshare, with 32 MB of flash, puts the filesystem into the top 16 MB by
itself; the 16 MB EV kit is laid out like the others. In V7 both were tested;
the EV kit needs no key: its profile is the default one.

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
port notes (ESP-IDF 6.1, silicon revision, PSRAM) are in
`bpgenvm-c/esp32p4/sdkconfig.defaults`.

---

## STM32 — two boards, two images

| board | image | display |
|---|---|---|
| **Nucleo-U575ZI-Q** (c1) | `bpvm_stm32_nucleo.bin` | no |
| **Discovery STM32U5G9J-DK2** (c2) | `bpvm_stm32_dk2.bin` | yes: the driver in the image; the GUI, with `Gui.pack` ([packs](#packs)) |

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
its display driver and the Nucleo one does not. Flashing the wrong one breaks nothing, but
the board will not do what you expect.

On both, a fatal system fault (a `HardFault`) leaves the board stopped: it does not restart
by itself.

### B. Building it

The port is built inside an **STM32CubeIDE** project (one per board); the integration
guide (include paths, linked core folder, source list) is in
`bpgenvm-c/stm32/port/README.md`. The resulting binary is flashed from CubeIDE itself
(Run/Debug) or by exporting the `.bin`.

---

<a id="packs"></a>
## The board's packs

Since V7 **the graphical interface is in no image**: LVGL, the display
engine, comes in a **pack** that is burned separately, into the board's pack
zone —like SQLite—. That way there is one image per micro, and the display is
the choice of whoever has one. **Without its packs, a display board does not
paint**: the first `Gui.Screen()` throws «GUI: esta placa no tiene el pack
'LVGL' grabado» (this board has no 'LVGL' pack burned).

The packs ship in the package's **`packs/`** folder:

| pack | what it is |
|---|---|
| `Gui.pack` | LVGL 9.2.2, the GUI engine (for ARM and RISC-V: the IDE burns the board's one) |
| `GuiMini.pack` | the same LVGL, trimmed for the C6, which has no PSRAM (256 widgets, fonts up to 24 px; RISC-V without FPU only); burned **instead of** `Gui.pack` |
| `DriverSpi.pack` | the driver for SPI panels (ST7789…) and their touch, for the C5 and the C6; the board is described in the ENV |
| `Stdlib.pack` | the standard library, run from flash ([below](#stdlib-pack)) |
| `SQLite.pack` | the database ([database guide](basedatos.html#pack)) |

### What each display board takes

| board | packs, in this order | RAM block (ENV) | more ENV |
|---|---|---|---|
| ESP32-C5 (Waveshare 2.8") | `Gui.pack`, `DriverSpi.pack` | `packRam=512` | the panel lines ([below](#panel-spi)) |
| ESP32-C6 (Waveshare 1.3") | `Stdlib.pack`, `GuiMini.pack`, `DriverSpi.pack` | `packRam=136` | the panel lines ([below](#panel-spi)) |
| ESP32-P4 (EV kit and Waveshare 4.3") | `Gui.pack` | `SQLite=4` | `display=st7701` on the Waveshare |
| Discovery STM32U5G9J-DK2 | `Stdlib.pack` (recommended), `Gui.pack` | `packRam=768` | — |

The P4 and the DK2 take no driver pack: the driver for their display
(MIPI-DSI and LTDC, with their touch) is in the image.

### How to burn them

1. **The ENV.** The IDE's **Entorno** button → **Añadir / editar…** (add /
   edit), one line at a time (the key, what goes left of the `=`; the value,
   what goes right): the RAM block and, on the C5 and the C6, the panel
   lines. Then `reset` in the console: the ENV is read at boot.
2. **The zone.** The **Placa** button (the *Gestión de placa* window, board
   management): on the right, the packs panel. If it says «zona sin
   formatear» (zone not formatted), **Formatear zona…** (format zone; it
   wipes it).
3. **The packs.** **Copiar pack a la placa…** (copy pack to the board) and
   pick each one in `packs/`, in the order of the table. The IDE prepares it
   for that board as it burns it —it keeps the engine for its family and
   seals it for the spot the board assigns— and, if the engine does not fit
   in the RAM block, it says so before burning.
4. **Try it.** `samples/GuiDemo.bp` → **Run on Device**: the title and the
   button show up on the display (**Stop** to end).

If it does not paint, the program says what is missing:

- «GUI: esta placa no tiene el pack 'LVGL' grabado» (no 'LVGL' pack burned)
  — the GUI pack is missing.
- «GUI: el pack de la GUI esta, pero su motor no cargo: …» (the GUI pack is
  there, but its engine did not load) — the pack is burned and its engine
  does not load. After the colon comes the reason: «falta packRam= en el
  ENV» (packRam= missing in the ENV: no RAM block; **Info** says so too),
  the engine's warning (for example, «realojado para OTRA direccion…»,
  relocated for ANOTHER address: [when to burn again](#regrabar)) or «mira el
  log de la placa» (look at the board's log).
- «GUI: no hay driver de pantalla ('PANT')…» (no display driver) — on the
  C5 and the C6, `DriverSpi.pack` is missing.
- «GUI: pantalla: …» (display: …) — a typo in the panel lines
  ([below](#panel-spi)).
- «GUI: la pantalla no arranco…» (the display did not start) — the driver
  failed to initialise; the reason is in the log (`log` in the console).

If the board already had engines loaded (the GUI or SQLite from before),
burning, deleting or formatting **restarts it by itself** to load the current
ones, and the IDE reconnects: wait for it to come back before going on. More
about the panel in the [IDE guide §10.3](guia-ide.html#packs-panel).

### The engines' RAM block

Native engines —the GUI, SQLite— keep their data in a RAM block that the
board sets aside **at boot**, before the VM's memory, with the size the ENV
gives: `packRam=<KB>` or, if absent, `SQLite=<MB>` (the database one, which
also serves the GUI). With neither there is no block (on the Pico 2 and the
Metro, 8 KB by default) and **no engine loads**: the **Info** button says so
(«RAM packs: no hay: un pack nativo no podría cargar…», there is none: a
native pack could not load). Whatever goes to the block is taken from the
VM; the values in the table are those of the tested boards.

<a id="panel-spi"></a>
### The panel lines (the C5 and the C6)

The SPI driver knows nothing about boards: each board is described in its
ENV, with one line per bus and one per device. They belong to the board, not
to the image: they are written once and survive a reflash.

**ESP32-C5** (Waveshare ESP32-C5-Touch-LCD-2.8):

```text
spi2=sck:6,mosi:7,miso:8
i2c0=sda:0,scl:1,khz:400
lcd=st7789,spi:2,dc:9,cs:10,rst:x1,bl:xpwm,mhz:40,w:240,h:320,inv
exp=ch32,i2c:0,addr:0x24
touch=cst3530,i2c:0,addr:0x58,rst:x0
packRam=512
```

**ESP32-C6** (Waveshare ESP32-C6-LCD-1.3):

```text
spi2=sck:7,mosi:6,miso:5
lcd=st7789,spi:2,dc:15,cs:14,rst:21,bl:22,mhz:40,w:240,h:240,ram:240x320,inv
packRam=136
```

For the TF slot, one more line: `sd=bus:2,cs:23,khz:10000` on the C5 and
`sd=bus:2,cs:4,khz:10000` on the C6 (the card sits on the same bus as the
display; the `miso` of `spi2` is for it). A typo does not just leave the
display black: the program gets the reason (for example, «GUI: pantalla: ENV
lcd: falta 'dc'», 'dc' is missing). What each field means, and how to
describe another SPI panel, is in the header of
`bpstdlib/gui/drivers/spi/nativo/src/driver_spi.c` (in the repository).

<a id="stdlib-pack"></a>
### The `Stdlib.pack`

It is the standard library (`Core`, `Str`, `Collections`, `Json`, `Gui`…) in
the pack zone. From the pack, each module runs **in place, from flash**, and
only its data goes to RAM; from `/lib` it goes to RAM whole. When it is
burned, the board **removes from `/lib`** whatever the pack serves with the
same bytes as its image (a newer copy you put there yourself stays, and
shadows the pack's); if you delete it, `/lib` is restored from the image on
the spot. Burn the one from the same package as the image.

It is recommended on the boards without PSRAM —the C3, the Pico 2, the
Nucleo and the DK2—, and on the C6 it is **essential for the GUI**: without
it, `Gui` and the modules it uses go to RAM and do not fit. Library packs
(the GUI, SQLite) carry no copy of the library: they take it from the
`Stdlib.pack` or from `/lib`. If the board is missing something when you
burn one, the panel warns: «graba tambien el Stdlib.pack» (burn the
Stdlib.pack too).

<a id="regrabar"></a>
### When to burn again

A pack with an engine (`Gui`, `GuiMini`, `DriverSpi`, `SQLite`) is **sealed**
for its spot in flash and in the RAM block. If that spot changes, the engine
does not load (and the GUI says so: «el pack de la GUI esta, pero su motor
no cargo: …», the GUI pack is there but its engine did not load). It must
be burned again:

- after changing `packRam` or `SQLite` in the ENV;
- after flashing a new image, if it moves the zone or the block (simplest:
  burn them again whenever you change the image);
- when moving from V6 to V7: **all of them**, with the ones in the package
  (V6 packs are not valid).

The safe way is **Formatear zona…** and burn them all again, in the order of
the table: the RAM block is handed out in burning order, and a new engine
behind the old one may not fit (the IDE says so: «no cabe», it does not fit).
Formatting also wipes the `Stdlib.pack`: it goes first.

---

## Which version is on my board?

IDE → Connect → **Info** button: the micro, flash, RAM and PSRAM, the packs'
RAM block («RAM packs») and the image's **native ABI** («ABI nativo»). That
last line says whether the image and the IDE match: «9 (la misma que genera
este IDE)» (the same one this IDE generates) is a V7 image with a V7 IDE. If
they do not match, or the line is missing (an image older than V7), flash the
image from this package —or, if the image is newer than the IDE, update the
IDE—. (On the S3, which has no AOT, a V7 image says «sin receta», no
recipe.) Info does not show the image's build date.
