# bpgenvm-c on Raspberry Pi Pico 2

Firmware FreeRTOS que corre la VM BasicPlus en una **Raspberry Pi Pico 2**
(RP2350, Cortex-M33 @ 150 MHz, 520 KB SRAM, 4 MB flash). El bytecode
`.mod` se embebe en la flash al compilar; los `print` salen por USB CDC
(la Pico aparece como puerto serie virtual en el PC).

## Pre-requisitos

| Pieza | Versión testada | Notas |
|---|---|---|
| Pico SDK | 2.2.x | `git clone https://github.com/raspberrypi/pico-sdk` + submodules |
| FreeRTOS-Kernel con port RP2350_ARM_NTZ | LTS V11.x | El port está en `Community-Supported-Ports/GCC/RP2350_ARM_NTZ` |
| `arm-none-eabi-gcc` | 13.x+ | ARM GNU Toolchain |
| CMake | ≥ 3.13 | El w64devkit lo trae |
| Ninja | cualquiera | También en w64devkit |
| `xxd` | cualquiera | Para regenerar los `.mod` embebidos |
| `picotool` (opcional) | 2.x | Para flashear sin BOOTSEL manual |

Variables de entorno necesarias:

```cmd
set PICO_SDK_PATH=C:\lenguajes\pm\pico-sdk
set FREERTOS_KERNEL_PATH=C:\lenguajes\pm\FreeRTOS-LTS\FreeRTOS\FreeRTOS-Kernel
set PICO_PLATFORM=rp2350
set PICO_BOARD=pico2
```

## Build

Desde `bpgenvm-c/pico/`:

```cmd
mkdir build && cd build
cmake -G "Ninja" ..
cmake --build .
```

Salida: `build/bpvm_pico.uf2` (y `.elf` + `.bin`).

## Flash

1. Aprieta **BOOTSEL** y conecta la Pico al USB → aparece como pendrive
   `RPI-RP2350`.
2. Arrastra `bpvm_pico.uf2` al pendrive. La Pico se reinicia.

Alternativa con picotool:
```cmd
picotool load build\bpvm_pico.uf2 -fx
```

## Ver la salida

Tras reset la Pico vuelve a enumerarse como un **COM port virtual**
(USB CDC). En Windows aparece como `COMn`. Abrir con cualquier terminal:

- PuTTY: Serial, COMn, 115200 (la velocidad es nominal en USB CDC, da igual)
- VS Code Serial Monitor
- `screen /dev/ttyACM0 115200` (Linux/Mac)

Deberías ver:

```
===========================================
 bpgenvm-c on RP2350 / FreeRTOS — FP1 boot
===========================================
buffer: 131072 bytes @ 0x20000000
loaded Hello.mod (1911 bytes)
--- VM output ---
Hola mundo desde BasicPlus
...
--- VM finished: OK ---
```

El LED on-board (GP25) parpadea rápido tras OK, lento si hubo error.

## Regenerar los .mod embebidos

Si modificas `samples/Hello.bp` y recompilas, regenera los arrays:

```sh
cd bpgenvm-c/samples
java -jar ../../lexer-java/target/basicplus-frontend.jar Hello.bp --compile . --backend=mivm
cd ../pico
xxd -i -n hello_mod ../samples/Hello.mod | sed '1s/.*/const uint8_t hello_mod[] = {/; $s/.*/const unsigned int hello_mod_len = & /' > hello_mod.c
# La stdlib embebida (core/gpio/i2c/...) -> scripts/regen_pico_mods.sh,
# o ../scripts/regen_all_mods.sh para regenerar las 3 familias de un tiro.
```

## Estructura

```
pico/
  CMakeLists.txt        — build del firmware
  FreeRTOSConfig.h      — config del kernel (M33, FPU, sin TZ/MPU)
  platform_freertos.c   — backend de bpvm_platform.h sobre FreeRTOS
  main.c                — entry: stdio_usb + task vm + LED
  embedded_mods.h       — declaración de arrays .mod
  hello_mod.c           — Hello.mod como array C (xxd -i)
  README.md             — este fichero
```

## FP2: Carga dinámica de .mod sobre USB CDC

A partir de FP2 el firmware NO ejecuta automáticamente el módulo
embebido. En su lugar arranca un **REPL** sobre USB CDC con comandos
de filesystem + ejecución. El primer arranque pre-carga `Hello.mod`
en el filesystem (RAM) para que tengas algo que probar.

### Comandos del REPL

```
HELLO              banner
LS                 list files
PUT name size      upload a file (binary)
GET name           download a file
DEL name           delete a file
RUN name           execute a .mod
MEM                fs stats
SAVE               persist fs to flash
FORMAT             wipe fs (ram only)
RESET              soft reboot
BOOTSEL            reboot into bootloader (drag-drop uf2)
HELP               this list
```

Puedes hablar el protocolo manualmente desde PuTTY/VS Code Serial:

```
> LS
OK 1
Hello.mod 1911
> RUN Hello.mod
--- VM output ---
0
1
1
...
999
--- VM finished: OK ---
```

### Cliente Python (recomendado)

`scripts/bpvm-pico.py` automatiza todo. Requiere `pyserial`:

```cmd
pip install pyserial
```

Workflow típico:

```cmd
cd C:\lenguajes\pm\bpgenvm-c
java -jar ..\lexer-java\target\basicplus-frontend.jar samples\benchcpu.bp --compile samples
python pico\scripts\bpvm-pico.py put samples\BenchCpu.mod
python pico\scripts\bpvm-pico.py run BenchCpu.mod
python pico\scripts\bpvm-pico.py save        # persiste en flash si quieres
```

El script autodetecta el COM port de la Pico por VID 0x2E8A. También
acepta `--port COM5` explícito.

### Persistencia en flash

El FS vive en RAM por defecto. `SAVE` graba los 33 KB de FS en los
últimos sectores del chip (offset 0x3F7000 sobre 4 MB de flash). En
el siguiente arranque `fs_init()` lo recupera.

Si el FS vacío al arrancar, pre-cargamos `Hello.mod` desde el array
embebido — así siempre hay algo para `RUN`.

## Limitaciones FP2

- 16 ficheros máximo, 32 KB de datos totales, 40 chars por nombre.
- PUT requiere conocer el tamaño de antemano (no streaming).
- Single-core. SMP (los dos cores M33) llegará tras estabilizar bring-up.
- MSC (drag&drop nativo en Explorador) = FP3 (pendiente).

## Troubleshooting

**`arm-none-eabi-gcc: not found`** — instala la toolchain ARM (ver
README del proyecto raíz).

**El COM port aparece pero no sale nada** — casi siempre es uno de
estos tres:

1. **DTR no asertado por el terminal**. El Pico SDK considera que el
   host "está conectado" solo cuando el terminal asserta DTR. Si no,
   `stdio_usb_connected()` devuelve `false` y `printf` descarta el
   output silenciosamente.

   - **PuTTY**: en Connection → Serial, marca "Implicit DTR/RTS" o
     equivalente. PuTTY moderno suele asertarlo por defecto cuando
     abres la sesión.
   - **VS Code Serial Monitor**: asserta DTR automáticamente.
   - **screen / minicom (Linux)**: por defecto asertan DTR.
   - **Tera Term**: en Setup → Serial Port, "Flow control = none" pero
     en Setup → Serial Port options activa DTR.
   - **Arduino IDE Serial Monitor**: asserta DTR; correcto.
   - **SimplySerial**: asegura `--dtr=true`.

2. **Abriste el terminal demasiado tarde** — el firmware tiene un
   bloqueo de 5 s en boot (`PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS`)
   pero si abres después, el banner se pierde. La VM se re-ejecuta
   cada 3 s, así que verás la siguiente iteración pronto.

3. **COM equivocado** — al flashear, Windows puede asignar un nuevo
   COM number. Revisa el Administrador de dispositivos.

**`cmake` no encuentra el FreeRTOS-Kernel** — comprueba que
`FREERTOS_KERNEL_PATH` apunta a la raíz del kernel (la carpeta con
`portable/`, no a `Community-Supported-Ports/...`).
