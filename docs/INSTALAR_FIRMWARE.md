# Instalar el firmware BasicPlus en cada micro

Cómo poner la imagen de la VM en cada placa soportada. Hay dos caminos:

- **A — Imagen precompilada** (el normal): descarga el binario de la
  *release* de GitHub y flashéalo con la herramienta de cada familia.
  No necesitas ningún toolchain.
- **B — Compilarla tú**: para quien toca el firmware. Cada port tiene su
  README con los prerequisitos (`bpgenvm-c/{pico,esp32,stm32}/README.md`).

Tras flashear, el flujo es el mismo en todas: IDE → Explorer → **Connect**
al puerto serie → Run on Device / consola / `autorun` (ver
[QUICKSTART](QUICKSTART.md)).

> El firmware solo se reflashea cuando cambia la **VM**. Tus programas
> (`.mod`) viven en el filesystem interno de la placa y se suben desde el
> IDE — actualizar tu aplicación nunca requiere tocar el firmware. Los
> ficheros del FS **sobreviven** a un reflasheo (viven en una región de
> flash aparte).

---

## Raspberry Pi Pico 2 / Adafruit Metro RP2350 — `bpvm_pico.uf2`

**Una única imagen para ambas placas**: la variante del chip (A/B), los
pines y la PSRAM se deciden en runtime, no al compilar.

### A. Imagen precompilada (BOOTSEL, sin instalar nada)

1. Desconecta la placa del USB.
2. Mantén pulsado el botón **BOOTSEL** mientras la conectas → aparece una
   unidad USB llamada `RPI-RP2`.
3. Copia `bpvm_pico.uf2` a esa unidad. La placa se reinicia sola con la
   VM dentro: fin.

En la **Metro RP2350**, sube después un `/sys/board.json` desde el IDE
(variante "B", ledPin, neopixelPin, psramCsPin) para habilitar los 48
GPIO, el NeoPixel y los 8 MB de PSRAM como heap. Sin él, la imagen
arranca con el perfil conservador de Pico.

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

`esptool` es un script Python (`pip install esptool`). La release trae la
imagen **fusionada** (bootloader + tabla de particiones + app en un solo
fichero, para flashear en el offset 0):

```sh
esptool.py --chip esp32s3 -p <puerto-del-bridge> write_flash 0x0 bpvm_esp32_merged.bin
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
esptool.py --chip esp32s3 merge_bin -o bpvm_esp32_merged.bin \
    --flash_mode dio --flash_freq 80m --flash_size 2MB \
    0x0 bootloader/bootloader.bin \
    0x8000 partition_table/partition-table.bin \
    0x10000 bpvm_esp32.bin
```

---

## STM32 (Nucleo-U575ZI-Q) — `bpvm_stm32.bin` / `.hex`

El ST-LINK integrado de la Nucleo hace de programador — el mismo cable
USB sirve para flashear y para el wire (VCP).

### A. Imagen precompilada

Dos opciones, de más simple a más completa:

1. **Arrastrar y soltar**: la Nucleo aparece como una unidad USB
   (`NOD_U575ZI`). Copia el `.bin` a esa unidad y el ST-LINK lo graba
   solo (el LED del ST-LINK parpadea durante la grabación).
2. **STM32CubeProgrammer** (gratuito, GUI o CLI): conectar → abrir el
   `.hex`/`.bin` → *Download*. Es la vía robusta si el drag&drop diera
   problemas.

### B. Compilarla

El port se construye dentro de un proyecto **STM32CubeIDE**; la guía de
integración (include paths, carpeta enlazada del core, lista de fuentes)
está en `bpgenvm-c/stm32/port/README.md`. El binario resultante se
flashea desde el propio CubeIDE (Run/Debug) o exportando el `.bin`.

---

## ¿Qué versión tengo en la placa?

IDE → Connect → botón **INFO**: muestra micro, build del firmware
(`serverBuild`), flash, RAM y PSRAM. La fecha de build identifica la
imagen instalada.
