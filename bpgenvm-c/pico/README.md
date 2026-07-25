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

## Cómo se habla con la placa

El firmware no ejecuta el módulo embebido al arrancar: levanta el
**wire BPVM v1** sobre USB CDC y espera. Es el mismo protocolo en las
tres familias (RP2350, ESP32, STM32), descrito en
`docs/BPVM_WIRE_PROTOCOL.md`: JSON por líneas, con los binarios
grandes en bruto detrás de la línea que los anuncia.

Quien lo habla es el **IDE** (BpIde). No hay cliente de línea de
comandos: para trastear sin placa está el micro simulado
(`make sim` → `bpvm-sim`), que es un dispositivo wire v1 completo y
el IDE lo trata igual que a un RP2350.

> **Histórico.** Hasta #305 convivía aquí un REPL de TEXTO
> (`HELLO`/`LS`/`PUT`/`GET`/`RUN`/`HELP`…), el protocolo original del
> Pico de cuando aún no existía el wire, con su propio cliente en
> `scripts/bpvm-pico.py`. Era un segundo camino a las mismas cosas
> —`RUN` tenía su propia resolución de módulos, en paralelo a la del
> wire— que nadie ejercitaba y que iba divergiendo en silencio. El
> ESP32 y el STM32 nacieron ya wire-v1 y nunca lo tuvieron. Retirarlo
> dejó a las tres familias iguales y liberó 41 KB de flash. El script
> de Python habla ese protocolo, así que ya no sirve.

### Autorun

Si existe `/sys/auto.txt`, el firmware ejecuta al arrancar el módulo
que nombre su primera línea, antes de entrar al bucle del wire. El
wire sigue vivo durante la ejecución, así que el IDE puede conectar y
parar la app aunque sea un bucle infinito (#256).

## Filesystem

**littlefs** sobre una partición de la flash, con la misma fachada
(`bpvm_fs_*`) que usan las otras dos familias; lo específico del
RP2350 es sólo la cintura de bloque (`fs_lfs_pico.c`, sobre
`flash_range_*`). Sobrevive a reflashear el firmware, porque la
región del FS no es la del código: las particiones viven en un
descriptor fuera del propio FS (H2·B2).

Estructura: `/sys` (config de la placa), `/lib` (módulos de
librería), `/app` (la aplicación). Al primer arranque el firmware
pre-instala la stdlib que lleva embebida, fichero a fichero y sólo si
falta — así un `PUT` del IDE con una versión más nueva no se pisa en
el siguiente reset.

## Limitaciones conocidas

- Módulo cargado para ejecutar = residente en un scratch aparte de la
  RAM de la VM (128 KB). Es lo que queda de #305; se resuelve en H11
  junto con la reorganización de RAM/PSRAM, y es igual en las tres
  familias.
- MSC (drag&drop nativo en el Explorador) sigue pendiente.

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
