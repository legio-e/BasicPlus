# BasicPlus — inicio rápido

> 🇬🇧 [English version](en/QUICKSTART.md)

De cero a un LED parpadeando (o un "hola" por consola) en pocos minutos,
por plataforma: PC, RP2350 (Pico 2 / Metro), ESP32-S3, ESP32-C3 y C6,
ESP32-P4 y STM32 (Nucleo y Discovery). Para el detalle fino de cada port:
los README de `bpgenvm-c/pico`, `bpgenvm-c/esp32` y `bpgenvm-c/stm32`, y el
`sdkconfig.defaults` de `bpgenvm-c/esp32p4`, `esp32c3` y `esp32c6`.

---

## 1. PC (sin placa) — 5 minutos

Requisitos: JDK 8+, Maven, GCC (MinGW en Windows), `make`. Opcional, para
`function native` en placa: el **Arm GNU Toolchain** (`arm-none-eabi-gcc`)
para RP2350/STM32 y el RISC-V del ESP-IDF (`riscv32-esp-elf-gcc`) para el
ESP32-P4 — instalación y configuración en la [guía del IDE](guia-ide.html#aot).

```sh
# Toolchain (compilador + VM Java) y VM C de host
mvn -f miVM/pom.xml install
mvn -f lexer-java/pom.xml install
cd bpgenvm-c && make && cd ..

# Compilar y ejecutar el blink en AMBAS VMs (en PC los GPIO loggean)
java -jar lexer-java/target/basicplus-frontend.jar samples/blink.bp \
     --compile samples --backend=mivm
java -jar miVM/target/bpgenvm-1.0.jar samples/Blink.mod
bpgenvm-c/build/bpgenvm-c samples/Blink.mod
```

Si la salida del programa no es idéntica en las dos (las cabeceras de cada
VM sí difieren), eso es un bug nuestro: la paridad dual-VM es el invariante
del proyecto.

**El IDE** (recomendado para todo lo que sigue):

```sh
mvn -f BpIde/pom.xml package
java -jar BpIde/target/BpIde-6.0.jar     # o bpide.bat, en la raíz del repo
```

Abre `samples/blink.bp`, botón **Compile**, menú **Run** — la salida
aparece en la consola inferior. **Run → Stop (Ctrl+F2)** aborta un
programa en marcha.

---

## 2. Raspberry Pi Pico 2 / Adafruit Metro RP2350

**Una sola imagen de firmware vale para las dos placas** — la variante
del chip (A/B) la detecta el firmware por hardware; lo demás se decide en
runtime (abajo, el caso Metro).

**Flashear**: BOOTSEL pulsado al conectar el USB → unidad `RPI-RP2` →
copia `bpvm_pico.uf2` → la placa rebota sola con la VM dentro. Dónde
conseguir/compilar la imagen y el caso Metro:
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md).

**Primer programa desde el IDE**:

1. Abre el IDE → panel inferior (*Explorer*) → selecciona el puerto COM
   de la placa → **Connect**. Verás el árbol de ficheros del micro
   (`/lib` con la stdlib, `/app`, `/sys`).
2. Abre `samples/blink.bp` → **Run on Device**. El IDE sube el `.mod`
   (y las dependencias que falten) y el LED parpadea. La salida del
   programa llega a la consola.
3. En la consola del Explorer: `help` lista los comandos (`dir`, `run`,
   `kill`, `autorun`, `log`, `save`, `reset`…).

**Hacerlo autónomo**: con tu programa ya en la placa,

```
/> autorun Blink      ← escribe /sys/auto.txt y lo persiste
/> reset
```

A partir de ahí la placa arranca tu programa sola al enchufarla, sin PC.
El IDE puede conectarse igualmente con el programa corriendo: `kill` lo
para, `autorun off` lo retira. Nunca hace falta reflashear.

**Metro RP2350 (variante B)**: los 48 GPIO salen solos (variante detectada).
Lo demás va en dos sitios:

- **PSRAM** (8 MB como heap): `psram=1` en el **ENV** de la placa — botón
  **Entorno** del IDE, casilla PSRAM ([guía del IDE §9.3](guia-ide.html#entorno));
  el firmware lo lee al arrancar, así que `reset` después.
- **Identidad y pines de la placa** (`name`, `ledPin`, `neopixelPin`): un
  `/sys/board.json` subido desde el IDE; plantilla en
  `bpgenvm-c/pico/boards/metro-rp2350b.json`. `name` es lo que devuelve
  `Machine.getBoard()`; con `neopixelPin` el firmware enciende el NeoPixel al
  arrancar. El `psramCsPin` de la plantilla se ignora (el CS es fijo, GP47).

---

## 3. ESP32-S3

¡Ojo con los **dos puertos USB** de la DevKit! El **wire** (lo que usa el
IDE) va por el **bridge UART0**; el USB nativo (USB-Serial-JTAG) es solo
consola de logs. Detalle en `bpgenvm-c/esp32/README.md`.

**Flashear**: con la imagen fusionada de la release y `esptool`
(`pip install esptool`) es un comando — ver
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md) (también la vía `idf.py` si
compilas tú).

Después: IDE → Connect al puerto del bridge → mismo flujo que en la Pico
(Run on Device, consola, autorun). La PSRAM de 8 MB del módulo se usa sola
como memoria de la VM. En el S3 las `native function` corren interpretadas
(no hay AOT para Xtensa) y el módulo `Net` aún no tiene backend.

---

## 4. ESP32-C3 y ESP32-C6

Las dos placas RISC-V tienen **un solo conector USB** (el USB-Serial-JTAG
nativo) y por él va el **wire**: un cable y el IDE conecta. La consola de
logs sale por UART0 (C3: GPIO20/21; C6: GPIO16/17) — solo hace falta con un
adaptador, para depurar; el log de arranque se lee igualmente desde el IDE
(`log`).

**Flashear**: el modo descarga se entra **a mano** — BOOT pulsado, pulsa y
suelta RESET, suelta BOOT — y se graba la imagen fusionada con `esptool
--chip esp32c3` (o `esp32c6`). Al terminar, RESET otra vez. Detalle en
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md).

Después: IDE → Connect al único puerto → mismo flujo (Run on Device,
consola, autorun). La VM dispone de 128 KB en las dos. El **C6 de Waveshare
(ESP32-C6-LCD-1.3)** trae una pantalla ST7789 de 240×240 por SPI, sin
táctil: los programas GUI pintan en ella ([guía de gráficos](gui.html)).
Las `native function` corren interpretadas (el IDE todavía no tiene destino
RISC-V sin FPU).

---

## 5. ESP32-P4 (placas con pantalla)

**Una sola imagen vale para las placas P4 soportadas** (ESP32-P4-Function-EV
de 7" y Waveshare Touch-LCD de 4.3"): el panel se elige en runtime con la
clave `display` del **ENV** de la placa — sin clave, arranca con el perfil
de la EV.

**Flashear**: con la imagen fusionada y `esptool` — ver
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md). El wire (lo que usa el IDE) va
por el puerto del **bridge USB-UART**, como en el S3.

Después: IDE → Connect → mismo flujo (Run on Device, consola, autorun). Y
**la pantalla**: los programas GUI (`import Gui`) pintan sobre el panel
táctil — todo el detalle en la [guía de gráficos](gui.html). En la
Waveshare, pon `display=st7701` desde el botón **Entorno** del IDE y
`reset` ([guía §23.4](gui.html#ej-pantalla)). AOT activo (RISC-V con FPU):
las `native function` se aceleran como en las placas ARM.

---

## 6. STM32 (Nucleo-U575ZI-Q y Discovery U5G9J-DK2)

Dos placas, dos imágenes (`bpvm_stm32_nucleo.bin` y `bpvm_stm32_dk2.bin`).
**Flashear**: lo más simple es arrastrar el `.bin` a la unidad USB del
ST-LINK (`NOD_U575ZI` en la Nucleo, `DIS_U5G9J` en la Discovery);
alternativas (`STM32_Programmer_CLI`) y compilación propia (CubeIDE) en
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md).

El wire sale por el **VCP del ST-LINK** — el mismo cable USB. IDE →
Connect a ese COM → Run on Device / consola / autorun, igual que en las
otras placas. AOT activo (mismo Cortex-M33 que el RP2350). La **Discovery**
añade la pantalla LTDC de 800×480 con táctil: los programas GUI pintan en
ella. En esta familia no hay `Adc` ni `Wdt` (lanzan `RuntimeError`).

---

## ¿Y ahora qué?

- **[Manual del lenguaje](manual.html)** y **[Referencia](referencia.html)**
  (stdlib, CLI, artefactos) — la documentación completa.
- **[Interfaz gráfica](gui.html)** — la GUI en placas con pantalla
  (ESP32-P4, STM32-DK2, ESP32-C6): widgets, color y fuentes, formularios `.win`.
- `samples/` — ejemplos: GPIO OO, I²C/SPI/UART, threads, excepciones,
  tuplas, `native` AOT, TCP…
- `docs/PENDIENTES.md` — las limitaciones conocidas del lenguaje, dichas sin adornos.
