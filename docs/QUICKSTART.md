# BasicPlus — inicio rápido

De cero a un LED parpadeando (o un "hola" por consola) en pocos minutos,
por plataforma. Para el detalle fino de cada port: los README de
`bpgenvm-c/pico`, `bpgenvm-c/esp32` y `bpgenvm-c/stm32`.

---

## 1. PC (sin placa) — 5 minutos

Requisitos: JDK 8+, Maven, GCC (MinGW en Windows), `make`. Opcional, para
`function native` en RP2350/STM32: el **Arm GNU Toolchain**
(`arm-none-eabi-gcc`) — instalación y configuración en la
[guía del IDE](guia-ide.html#aot).

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

Si las dos salidas no son idénticas, eso es un bug nuestro: la paridad
dual-VM es el invariante del proyecto.

**El IDE** (recomendado para todo lo que sigue):

```sh
mvn -f BpIde/pom.xml package
java -jar BpIde/target/BpIde-1.0-SNAPSHOT-shaded.jar
```

Abre `samples/blink.bp`, botón **Compile**, menú **Run** — la salida
aparece en la consola inferior. **Run → Stop (Ctrl+F2)** aborta un
programa en marcha.

---

## 2. Raspberry Pi Pico 2 / Adafruit Metro RP2350

**Una sola imagen de firmware vale para las dos placas** — la variante
del chip, los pines y la PSRAM se detectan en runtime.

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

**Metro RP2350 (variante B)**: sube un `/sys/board.json` declarando la
placa (variante, ledPin, neopixelPin, psramCsPin) — con él, el firmware
habilita los 48 GPIO, el NeoPixel y los 8 MB de PSRAM como heap.

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
(Run on Device, consola, autorun). En el ESP32 las `native function`
corren interpretadas (el AOT es ARM) y el módulo `Net` aún no tiene
backend (llegará con lwIP).

---

## 4. STM32 (Nucleo-U575ZI-Q)

**Flashear**: lo más simple es arrastrar el `.bin` a la unidad USB del
ST-LINK (`NOD_U575ZI`); alternativas y compilación propia (CubeIDE) en
[INSTALAR_FIRMWARE](INSTALAR_FIRMWARE.md).

El wire sale por el **VCP del ST-LINK** — el mismo cable USB. IDE →
Connect a ese COM → Run on Device / consola / autorun, igual que en las
otras placas. AOT activo (mismo Cortex-M33 que el RP2350).

---

## ¿Y ahora qué?

- **[Manual del lenguaje](manual.html)** y **[Referencia](referencia.html)**
  (stdlib, CLI, artefactos) — la documentación completa.
- `samples/` — ejemplos: GPIO OO, I²C/SPI/UART, threads, excepciones,
  tuplas, `native` AOT, TCP…
- `docs/PENDIENTES.md` — el backlog vivo y honesto del proyecto.
