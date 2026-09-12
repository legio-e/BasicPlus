# Instalar el firmware BasicPlus en cada micro

> 🇬🇧 [English version](en/INSTALAR_FIRMWARE.md)

Cómo poner la imagen de la VM en cada placa soportada. Hay dos caminos:

- **A — Imagen precompilada** (el normal): **ya la tienes**. Las siete
  imágenes (nueve placas) vienen dentro del paquete, en la carpeta
  `firmware/`; se flashean con la herramienta de cada familia y no necesitas
  ningún toolchain. (Si no tienes el paquete, se descarga de la *release* de
  GitHub.)
- **B — Compilarla tú**: para quien toca el firmware. Los prerequisitos
  están en `bpgenvm-c/{pico,esp32,stm32}/README.md`; los ports ESP32-P4, C3
  y C6 comparten el `main` del S3 (`bpgenvm-c/esp32/common/`) y lo propio de
  cada uno está en su `sdkconfig.defaults` y `main/chip_cfg.h`.

| imagen | placas |
|---|---|
| `bpvm_pico.uf2` | Raspberry Pi Pico 2 · Adafruit Metro RP2350 |
| `bpvm_esp32_merged.bin` | ESP32-S3 |
| `bpvm_esp32c3_merged.bin` | ESP32-C3 |
| `bpvm_esp32c6_merged.bin` | ESP32-C6 (Waveshare ESP32-C6-LCD-1.3) |
| `bpvm_esp32p4_merged.bin` | ESP32-P4-Function-EV · Waveshare ESP32-P4 4.3" |
| `bpvm_stm32_nucleo.bin` | Nucleo-U575ZI-Q |
| `bpvm_stm32_dk2.bin` | Discovery STM32U5G9J-DK2 |

Tras flashear, el flujo es el mismo en todas: IDE → Explorer → **Connect**
al puerto serie → Run on Device / consola / `autorun` (ver
[QUICKSTART](QUICKSTART.md)).

> El firmware solo se reflashea cuando cambia la **VM**. Tus programas
> (`.mod`) viven en el filesystem interno de la placa y se suben desde el
> IDE — actualizar tu aplicación nunca requiere tocar el firmware. Los
> ficheros del FS **sobreviven** a un reflasheo (viven en una región de
> flash aparte). La stdlib de `/lib` la repone el propio firmware al
> arrancar (falta, versión anterior o distinta → la copia de la imagen);
> `/app` no se toca nunca — no dejes ahí copias de módulos de la stdlib,
> taparían a las de `/lib`.

> La configuración de la placa va en dos mitades. **Identidad y pines**
> (`name`, `ledPin`, `neopixelPin`) → `/sys/board.json`, y solo en RP2350.
> **PSRAM y panel** → el **ENV** de la placa (botón **Entorno** del IDE):
> `psram=1` en la Metro, `display=st7701` en la P4 de Waveshare. El ENV
> vive en su propia partición y sobrevive al reflasheo.

---

## Raspberry Pi Pico 2 / Adafruit Metro RP2350 — `bpvm_pico.uf2`

**Una única imagen para ambas placas**: la variante del chip (A/B) se
detecta por hardware al arrancar; los pines y la PSRAM se deciden en
runtime, no al compilar.

### A. Imagen precompilada (BOOTSEL, sin instalar nada)

1. Desconecta la placa del USB.
2. Mantén pulsado el botón **BOOTSEL** mientras la conectas → aparece una
   unidad USB llamada `RPI-RP2`.
3. Copia `bpvm_pico.uf2` a esa unidad. La placa se reinicia sola con la
   VM dentro: fin.

En la **Metro RP2350** los 48 GPIO salen solos (variante B detectada).
Después, dos ajustes:

- **PSRAM** (8 MB como heap): `psram=1` en el ENV — botón **Entorno** del
  IDE, casilla PSRAM — y `reset`. Solo en RP2350B; el CS es fijo (GP47).
- **Identidad y pines**: sube `firmware/boards/metro-rp2350b.json` (del paquete;
  `bpgenvm-c/pico/boards/` en el repo)
  como `/sys/board.json` (`name`, `ledPin`, `neopixelPin`; su `psramCsPin`
  se ignora). Sin el fichero la placa se llama `generic` y no enciende el
  NeoPixel al arrancar; todo lo demás funciona igual.

### B. Compilarla

```sh
cd bpgenvm-c/pico && mkdir -p build && cd build
cmake -G Ninja -DPICO_BOARD=bp_rp2350b \
      -DFREERTOS_KERNEL_PATH=<ruta>/FreeRTOS-Kernel ..
ninja bpvm_pico        # → build/bpvm_pico.uf2
```

Prerequisitos (pico-sdk, toolchain ARM) en `bpgenvm-c/pico/README.md`.

---

## ESP32-S3 — `bpvm_esp32_merged.bin`

La DevKit tiene **dos puertos USB**: flashea y conecta el IDE por el del
**bridge UART** (CP210x/CH340); el USB nativo es solo consola de logs.

### A. Imagen precompilada (esptool)

`esptool` viene con el ESP-IDF (o `pip install esptool`). **Con el IDF v6 es
esptool v5**: el ejecutable es `esptool` (no `esptool.py`) y los subcomandos
llevan GUION (`write-flash`, `merge-bin`, `--flash-size`). La release trae la
imagen **fusionada** (bootloader + tabla de particiones + app en un solo
fichero, para flashear en el offset 0):

```sh
esptool --chip esp32s3 -p <puerto-del-bridge> write-flash 0 bpvm_esp32_merged.bin
```

### B. Compilarla (ESP-IDF v6.x)

```sh
cd bpgenvm-c/esp32
idf.py build
idf.py -p <puerto-del-bridge> flash      # flashea las 3 piezas con sus offsets
```

Para regenerar la imagen fusionada de la release a partir del build:

```sh
cd build
esptool --chip esp32s3 merge-bin -o bpvm_esp32_merged.bin \
    --flash-mode dio --flash-freq 80m --flash-size 16MB \
    0x0 bootloader/bootloader.bin \
    0x8000 partition_table/partition-table.bin \
    0x10000 bpvm_esp32.bin
```

⚠️ El atajo `idf.py merge-bin` escribe `build/merged-binary.bin`, **no**
`bpvm_esp32_merged.bin`: si ya había una `_merged.bin` al lado, se queda la
vieja. Comprueba la fecha del fichero que flasheas. (Vale para las cuatro
imágenes ESP32.)

La PSRAM de 8 MB del módulo (WROOM-1) se activa sola: no hay nada que
configurar.

---

## ESP32-C3 y ESP32-C6 — `bpvm_esp32c3_merged.bin` · `bpvm_esp32c6_merged.bin`

Las dos tienen **un solo conector USB**, el USB-Serial-JTAG nativo, y por él
va el **wire** (al revés que en el S3). La consola de logs sale por UART0
(C3: GPIO20/21; C6: GPIO16/17) y solo hace falta con un adaptador; el log de
arranque se lee desde el IDE (`log`). La imagen del C6 es la de la
**Waveshare ESP32-C6-LCD-1.3** (pantalla ST7789 240×240 por SPI, sin táctil).

### A. Imagen precompilada (esptool)

El modo descarga se entra **a mano** con los dos pulsadores: BOOT pulsado,
pulsa y suelta RESET, suelta BOOT. El puerto sigue siendo el mismo USB.

```sh
esptool --chip esp32c3 -p <puerto> write-flash 0 bpvm_esp32c3_merged.bin
esptool --chip esp32c6 -p <puerto> write-flash 0 bpvm_esp32c6_merged.bin
```

Al terminar, pulsa **RESET** (solo): el reset automático de `esptool` no
siempre saca al chip del modo descarga, y en modo descarga nada contesta.

### B. Compilarla (ESP-IDF v6.x)

```sh
cd bpgenvm-c/esp32c3        # o esp32c6
idf.py set-target esp32c3   # o esp32c6 (solo la primera vez)
idf.py build
idf.py -p <puerto> flash    # con la placa en modo descarga (BOOT+RESET)
```

Imagen fusionada: los offsets están en `build/flash_args` (0x0 bootloader,
0x8000 tabla de particiones, 0x10000 app; flash de 4 MB a 80 MHz):

```sh
cd build
esptool --chip esp32c3 merge-bin -o bpvm_esp32c3_merged.bin \
    --flash-mode dio --flash-freq 80m --flash-size 4MB \
    0x0 bootloader/bootloader.bin \
    0x8000 partition_table/partition-table.bin \
    0x10000 bpvm_esp32c3.bin
```

(Para el C6, `esp32c6` y `bpvm_esp32c6.bin`.)

---

## ESP32-P4 (placas con pantalla) — `bpvm_esp32p4_merged.bin`

**Una única imagen para las placas P4 soportadas** (ESP32-P4-Function-EV de
7" 1024×600 y Waveshare Touch-LCD de 4.3" 480×800): el panel se elige en
runtime desde el **ENV** de la placa (clave `display`: `ek79007` o `st7701`);
sin clave, arranca con el perfil de la EV. En la Waveshare, una vez: botón
**Entorno** del IDE → `display=st7701` → `reset`. El ENV sobrevive al
reflasheo, así que no hay que repetirlo ([guía de gráficos
§23.4](gui.html#ej-pantalla)). `/sys/board.json` no se lee en el P4.

El wire (IDE) va por el puerto del **bridge USB-UART** de la placa.

### A. Imagen precompilada (esptool)

```sh
esptool --chip esp32p4 -p <puerto-del-bridge> write-flash 0 bpvm_esp32p4_merged.bin
```

### B. Compilarla (ESP-IDF v6.x)

```sh
cd bpgenvm-c/esp32p4
idf.py build
idf.py -p <puerto-del-bridge> flash      # flashea las piezas con sus offsets
```

Para regenerar la imagen fusionada, los offsets exactos del build están en
`build/flash_args` (`esptool merge-bin` con esa lista; en el P4 el
bootloader va en 0x2000 y la flash a 40 MHz, no como en el S3). Las notas
del port (ESP-IDF v6.0.1, revisión del silicio, PSRAM) están en
`bpgenvm-c/esp32p4/sdkconfig.defaults`.

---

## STM32 — dos placas, dos imágenes

| placa | imagen | pantalla |
|---|---|---|
| **Nucleo-U575ZI-Q** (c1) | `bpvm_stm32_nucleo.bin` | no |
| **Discovery STM32U5G9J-DK2** (c2) | `bpvm_stm32_dk2.bin` | sí (GUI LVGL) |

En las dos, el **ST-LINK integrado hace de programador**: el mismo cable USB sirve para
flashear y para el wire (VCP).

### A. Imagen precompilada

Dos opciones, de más simple a más completa:

1. **Arrastrar y soltar**: la placa aparece como una unidad USB (`NOD_U575ZI` en la
   Nucleo, `DIS_U5G9J` en la Discovery). Copia el `.bin` que le corresponda a esa unidad
   y el ST-LINK lo graba solo (su LED parpadea durante la grabación).
2. **STM32CubeProgrammer** (gratuito, GUI o CLI). Por línea de órdenes:
   ```
   STM32_Programmer_CLI -c port=SWD -d bpvm_stm32_nucleo.bin 0x08000000 -hardRst
   ```
   Es la vía robusta si el arrastrar y soltar diera problemas. Con dos placas
   conectadas, elige la sonda con `port=SWD sn=<serie>`.

⚠️ **No confundas las dos imágenes.** Comparten familia pero no placa: la de la Discovery
lleva el soporte de pantalla y la de la Nucleo no. Flashear la que no es no rompe nada,
pero la placa no hará lo que esperas.

### B. Compilarla

El port se construye dentro de un proyecto **STM32CubeIDE** (uno por placa); la guía de
integración (include paths, carpeta enlazada del core, lista de fuentes) está en
`bpgenvm-c/stm32/port/README.md`. El binario resultante se flashea desde el propio
CubeIDE (Run/Debug) o exportando el `.bin`.
---

## ¿Qué versión tengo en la placa?

IDE → Connect → botón **INFO**: muestra micro, build del firmware
(`serverBuild`), flash, RAM y PSRAM. La fecha de build identifica la
imagen instalada.
