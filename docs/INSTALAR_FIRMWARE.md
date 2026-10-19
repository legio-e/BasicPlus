# Instalar el firmware BasicPlus en cada micro

> 🇬🇧 [English version](en/INSTALAR_FIRMWARE.md)

Cómo poner la imagen de la VM en cada placa soportada —y, en las que tienen
pantalla, sus packs—. Hay dos caminos:

- **A — Imagen precompilada** (el normal): **ya la tienes**. Las ocho
  imágenes (diez placas) vienen dentro del paquete, en la carpeta
  `firmware/`, y los packs, en `packs/`; se flashean con la herramienta de
  cada familia y no necesitas ningún toolchain. (Si no tienes el paquete, se
  descarga de la *release* de GitHub.)
- **B — Compilarla tú**: para quien toca el firmware. Los prerequisitos
  están en `bpgenvm-c/{pico,esp32,stm32}/README.md`; los ports ESP32-C3, C5
  y C6 comparten el `main` del S3 (`bpgenvm-c/esp32/common/`) y lo propio de
  cada uno está en su `sdkconfig.defaults` y `main/chip_cfg.h`; el P4 tiene
  su propio `main.c` y comparte el resto de `esp32/common/`.

| imagen | placas |
|---|---|
| `bpvm_pico.uf2` | Raspberry Pi Pico 2 · Adafruit Metro RP2350 |
| `bpvm_esp32_merged.bin` | ESP32-S3 |
| `bpvm_esp32c3_merged.bin` | ESP32-C3 |
| `bpvm_esp32c5_merged.bin` | ESP32-C5 (Waveshare ESP32-C5-Touch-LCD-2.8) — *nueva en V7* |
| `bpvm_esp32c6_merged.bin` | ESP32-C6 (y la Waveshare ESP32-C6-LCD-1.3, con su pantalla por packs) |
| `bpvm_esp32p4_merged.bin` | ESP32-P4-Function-EV · Waveshare ESP32-P4 4.3" |
| `bpvm_stm32_nucleo.bin` | Nucleo-U575ZI-Q |
| `bpvm_stm32_dk2.bin` | Discovery STM32U5G9J-DK2 |

Tras flashear, el flujo es el mismo en todas: IDE → Explorer → **Connect**
al puerto serie → Run on Device / consola / `autorun` (ver
[QUICKSTART](QUICKSTART.md)). Y dos pasos más, sólo la primera vez:

- **Una placa nueva viene sin particiones.** Al conectar, el IDE se ofrece a
  abrir la **Gestión de placa**: *Proponer defaults* → *Aplicar tamaños*, y la
  placa se reinicia con su sistema de archivos y su zona de packs ([guía del
  IDE §9.2](guia-ide.html#particiones)). La C5 y la P4 de Waveshare, con 32 MB
  de flash, se reparten solas.
- **Una placa con pantalla necesita sus packs** —la C5, la C6, las P4 y la
  DK2—: desde V7 la GUI no va en la imagen. Ver
  [Los packs de la placa](#packs).

> **¿Vienes de V6?** Graba la imagen de V7 como siempre: el sistema de
> archivos, las particiones y el ENV se quedan. Pero en las placas con
> pantalla **la GUI ya no va en la imagen**: sin sus packs, la pantalla no
> pinta. Y los packs que tuvieras de V6 (SQLite) no valen en V7: formatea la
> zona y graba los del paquete. Todo en [Los packs de la placa](#packs).
> **Y recompila tus programas**: un `.mod` de V6 que use clases de la
> biblioteca no enlaza con la de V7. El IDE recompila en cada Run; lo que hay
> que mirar es lo que quedó en `/app` —el programa del autorun, sobre todo—:
> vuelve a ejecutarlo desde el IDE, o bórralo ([notas de v7.0](RELEASES.md)).

> El firmware solo se reflashea cuando cambia la **VM**. Tus programas
> (`.mod`) viven en el filesystem interno de la placa y se suben desde el
> IDE — actualizar tu aplicación nunca requiere tocar el firmware. Los
> ficheros del FS y los packs **sobreviven** a un reflasheo (viven en una
> región de flash aparte), aunque los packs con motor —la GUI, SQLite— puede
> que haya que regrabarlos ([cuándo](#regrabar)). La stdlib de `/lib` la pone
> el propio firmware al arrancar: un módulo que falta, de una versión
> anterior o distinto → la copia de la imagen; uno más nuevo se deja; y lo
> que el `Stdlib.pack` sirve igual, sale de `/lib` ([el `Stdlib.pack`](#stdlib-pack)).
> `/app` no se toca nunca — no dejes ahí copias de módulos de la stdlib,
> taparían a las de `/lib`.

> La configuración de la placa va en dos mitades. **Identidad y pines**
> (`name`, `ledPin`, `neopixelPin`) → `/sys/board.json`, y solo en RP2350.
> **Lo demás** → el **ENV** de la placa (botón **Entorno** del IDE): la
> PSRAM (`psram=1` en la Metro), el panel (`display=st7701` en la P4 de
> Waveshare; las líneas del panel SPI en la C5 y la C6), el bloque de RAM de
> los packs (`packRam`, `SQLite`), la tarjeta (`sd`) y los núcleos
> (`vm.runs`, `vm.cores`). El ENV vive en su propia partición, se lee al
> arrancar —tras cambiarlo, `reset`— y sobrevive al reflasheo. Las claves, en
> la [guía del IDE §9.3](guia-ide.html#entorno).

<a id="nucleos"></a>
> **Dos núcleos** (desde V7). En la Pico 2, la Metro, la S3 y las P4 la
> imagen usa los dos núcleos del micro y, por defecto, dos *runs*: dos hilos
> de tu programa ejecutan a la vez (en la C3, la C5, la C6 y las STM32, uno).
> Un programa de un solo hilo no gana nada —y en la Pico 2 y la Metro, con
> dos runs, pierde algo—: si es tu caso, `vm.runs=1` en el ENV. Un programa
> que se depura va con un run. Y si algo fallara con los dos núcleos,
> `vm.cores=1` y `vm.runs=1` dejan la placa en uno sin regrabarla.
> **Limitación conocida (`#665`, a V8):** en la Pico 2 y la Metro, la imagen
> de dos núcleos hace más lento un programa de un hilo —en la Metro, un 35 %,
> también con `vm.cores=1`; en la Pico 2, PrintBench 1 270 ms frente a
> 817—; quien lo note puede compilar la de un núcleo (abajo, *Compilarla*).

---

## Raspberry Pi Pico 2 / Adafruit Metro RP2350 — `bpvm_pico.uf2`

**Una única imagen para ambas placas**: la variante del chip (A/B) se
detecta por hardware al arrancar; los pines y la PSRAM se deciden en
runtime, no al compilar. Usa los dos núcleos del RP2350
([dos núcleos](#nucleos)).

### A. Imagen precompilada (BOOTSEL, sin instalar nada)

1. Desconecta la placa del USB.
2. Mantén pulsado el botón **BOOTSEL** mientras la conectas → aparece una
   unidad USB llamada `RP2350`.
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

Con los dos núcleos, un fallo fatal del sistema (un `HardFault`, por
ejemplo) reinicia la placa: el informe queda en el log (`log` en la
consola), el **Info** da «fallo fatal» como causa del último reset y ese
arranque **no lanza el autorun** —un programa que falla al arrancar sería un
bucle de reinicios—: se lanza a mano desde el IDE.

### B. Compilarla

```sh
cd bpgenvm-c/pico && mkdir -p build && cd build
cmake -G Ninja -DPICO_BOARD=bp_rp2350b \
      -DFREERTOS_KERNEL_PATH=<ruta>/FreeRTOS-Kernel ..
ninja bpvm_pico        # → build/bpvm_pico.uf2
```

La imagen sale de dos núcleos; la de uno, añadiendo
`-DBPVM_PICO_NUM_CORES=1` al `cmake` (la vuelta atrás grabando; sin grabar,
`vm.cores=1` y `vm.runs=1` en el ENV). Prerequisitos (pico-sdk, toolchain
ARM) en `bpgenvm-c/pico/README.md`.

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
vieja. Comprueba la fecha del fichero que flasheas. (Vale para las cinco
imágenes ESP32.)

La PSRAM de 8 MB del módulo (WROOM-1) se activa sola: no hay nada que
configurar. Desde V7 la S3 usa sus dos núcleos ([dos núcleos](#nucleos)).

Si el arranque anterior acabó en un fallo fatal —un pánico (una excepción) o
el perro guardián de las interrupciones—, la placa **no lanza el autorun**:
lo dice en el log (`log` en la consola) y en su consola de logs («[autorun]
… NO arranca: el arranque anterior acabo en un fallo fatal»), el **Info** da
la causa del último reset (`panic/exception` o `int-watchdog`) y el programa
se lanza a mano desde el IDE. El perro guardián del módulo `Wdt` no es un fallo fatal: tras él, el
autorun se lanza. (Vale para las cinco imágenes ESP32.)

---

## ESP32-C3 y ESP32-C6 — `bpvm_esp32c3_merged.bin` · `bpvm_esp32c6_merged.bin`

Las dos tienen **un solo conector USB**, el USB-Serial-JTAG nativo, y por él
va el **wire** (al revés que en el S3). La consola de logs sale por UART0
(C3: GPIO20/21; C6: GPIO16/17) y solo hace falta con un adaptador; el log de
arranque se lee desde el IDE (`log`). La imagen del C6 es la de cualquier
ESP32-C6; la **Waveshare ESP32-C6-LCD-1.3** (pantalla ST7789 240×240 por
SPI, sin táctil, y ranura TF) pinta con sus packs y unas líneas en el ENV
([Los packs de la placa](#packs)). En las dos, las `native` se compilan y se
aceleran (AOT, ABI `ilp32`).

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

<a id="esp32-c5"></a>
## ESP32-C5 — `bpvm_esp32c5_merged.bin`

*Nueva en V7.* La **Waveshare ESP32-C5-Touch-LCD-2.8**: un núcleo RISC-V a
240 MHz, 8 MB de PSRAM, 32 MB de flash, pantalla táctil de 2,8" (ST7789
240×320 por SPI, táctil CST3530) y ranura TF. Como la C3 y la C6, tiene **un
solo conector USB**, el USB-Serial-JTAG nativo, y por él va el **wire**; la
consola de logs sale por UART0 (GPIO11 TX, GPIO12 RX) y solo hace falta con
un adaptador.

### A. Imagen precompilada (esptool)

El modo descarga, a mano, como en la C3 y la C6: BOOT pulsado, pulsa y
suelta RESET, suelta BOOT.

```sh
esptool --chip esp32c5 -p <puerto> write-flash 0 bpvm_esp32c5_merged.bin
```

Al terminar, pulsa **RESET** (solo).

### B. Compilarla (ESP-IDF v6.x)

```sh
cd bpgenvm-c/esp32c5
idf.py set-target esp32c5   # solo la primera vez
idf.py build
idf.py -p <puerto> flash    # con la placa en modo descarga (BOOT+RESET)
```

La imagen fusionada **no se hace como la de la C3 o la C6**: en la C5 el
bootloader va en **0x2000** y la flash es de **32 MB** a 80 MHz (lo dice
`build/flash_args`); los offsets de la C3 no valen.

```sh
cd build
esptool --chip esp32c5 merge-bin -o bpvm_esp32c5_merged.bin \
    --flash-mode dio --flash-freq 80m --flash-size 32MB \
    0x2000 bootloader/bootloader.bin \
    0x8000 partition_table/partition-table.bin \
    0x10000 bpvm_esp32c5.bin
```

### Lo propio de la C5

- **La PSRAM** (8 MB) se activa sola y la VM va en ella: nada que configurar.
- **La flash**: el sistema de archivos va solo en los 16 MB de arriba y la
  zona de packs se queda con el resto. No hay particiones que repartir: el
  reparto del editor de particiones no se aplica en esta placa.
- **`native`**: se compila y se acelera (AOT, ABI `ilp32`), como en la C3 y
  la C6.
- **La pantalla**: `Gui.pack` + `DriverSpi.pack`, `packRam=512` y las líneas
  del panel en el ENV ([Los packs de la placa](#packs)).
- **La ranura TF**: `sd=bus:2,cs:23,khz:10000` en el ENV. Va en el mismo bus
  SPI que la pantalla (su línea `spi2=`) y se monta en `/sd`
  ([referencia, la tarjeta SD](referencia.html#stdlib-sd)).

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

**La pantalla llega por un pack** (desde V7): `Gui.pack` y un bloque de RAM
en el ENV (`SQLite=4`, el mismo que usa SQLite) —ver
[Los packs de la placa](#packs)—. El driver del panel MIPI-DSI y de su
táctil sí va en la imagen. La P4 usa sus dos núcleos
([dos núcleos](#nucleos)). La Waveshare, con 32 MB de flash, pone sola el
sistema de archivos en los 16 MB de arriba; el kit EV, de 16 MB, se reparte
como las demás. En V7 se han probado las dos; el kit EV no necesita clave: su
perfil es el de por defecto.

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
del port (ESP-IDF 6.1, revisión del silicio, PSRAM) están en
`bpgenvm-c/esp32p4/sdkconfig.defaults`.

---

## STM32 — dos placas, dos imágenes

| placa | imagen | pantalla |
|---|---|---|
| **Nucleo-U575ZI-Q** (c1) | `bpvm_stm32_nucleo.bin` | no |
| **Discovery STM32U5G9J-DK2** (c2) | `bpvm_stm32_dk2.bin` | sí: el driver, en la imagen; la GUI, con `Gui.pack` ([packs](#packs)) |

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
lleva el driver de su pantalla y la de la Nucleo no. Flashear la que no es no rompe nada,
pero la placa no hará lo que esperas.

En las dos, un fallo fatal del sistema (un `HardFault`) deja la placa parada: no se
reinicia sola.

### B. Compilarla

El port se construye dentro de un proyecto **STM32CubeIDE** (uno por placa); la guía de
integración (include paths, carpeta enlazada del core, lista de fuentes) está en
`bpgenvm-c/stm32/port/README.md`. El binario resultante se flashea desde el propio
CubeIDE (Run/Debug) o exportando el `.bin`.

---

<a id="packs"></a>
## Los packs de la placa

Desde V7 **la interfaz gráfica no va en ninguna imagen**: LVGL, el motor de
la pantalla, llega en un **pack** que se graba aparte, en la zona de packs de
la placa —como SQLite—. Así hay una imagen por micro, y la pantalla la decide
quien la tiene. **Sin sus packs, una placa con pantalla no pinta**: el primer
`Gui.Screen()` lanza «GUI: esta placa no tiene el pack 'LVGL' grabado».

Los packs vienen en la carpeta **`packs/`** del paquete:

| pack | qué es |
|---|---|
| `Gui.pack` | LVGL 9.2.2, el motor de la GUI (para ARM y RISC-V: el IDE graba el de la placa) |
| `GuiMini.pack` | el mismo LVGL, recortado para la C6, sin PSRAM (256 widgets, fuentes hasta 24 px; sólo RISC-V sin FPU); se graba **en vez de** `Gui.pack` |
| `DriverSpi.pack` | el driver de los paneles por SPI (ST7789…) y su táctil, para la C5 y la C6; la placa se describe en el ENV |
| `Stdlib.pack` | la biblioteca estándar, ejecutada desde la flash ([abajo](#stdlib-pack)) |
| `SQLite.pack` | la base de datos ([guía de la base de datos](basedatos.html#pack)) |

### Qué lleva cada placa con pantalla

| placa | packs, en este orden | bloque de RAM (ENV) | más ENV |
|---|---|---|---|
| ESP32-C5 (Waveshare 2,8") | `Gui.pack`, `DriverSpi.pack` | `packRam=512` | las líneas del panel ([abajo](#panel-spi)) |
| ESP32-C6 (Waveshare 1,3") | `Stdlib.pack`, `GuiMini.pack`, `DriverSpi.pack` | `packRam=136` | las líneas del panel ([abajo](#panel-spi)) |
| ESP32-P4 (kit EV y Waveshare 4,3") | `Gui.pack` | `SQLite=4` | `display=st7701` en la Waveshare |
| Discovery STM32U5G9J-DK2 | `Stdlib.pack` (recomendado), `Gui.pack` | `packRam=768` | — |

La P4 y la DK2 no llevan pack de driver: el de su pantalla (MIPI-DSI y LTDC,
con su táctil) va en la imagen.

### Cómo se graban

1. **El ENV.** Botón **Entorno** del IDE → **Añadir / editar…**, una línea
   cada vez (la clave, lo que va a la izquierda del `=`; el valor, lo de la
   derecha): el bloque de RAM y, en la C5 y la C6, las líneas del panel.
   Después, `reset` en la consola: el ENV se lee al arrancar.
2. **La zona.** Botón **Placa** (la ventana *Gestión de placa*): a la
   derecha, el panel de packs. Si dice «zona sin formatear», **Formatear
   zona…** (la deja en blanco).
3. **Los packs.** **Copiar pack a la placa…** y elige cada uno en `packs/`,
   en el orden de la tabla. El IDE lo prepara para esa placa al grabarlo —se
   queda con el motor de su familia y lo sella para el sitio que ella le
   da— y, si el motor no cabe en el bloque de RAM, lo dice antes de grabar.
4. **Probarlo.** `samples/GuiDemo.bp` → **Run on Device**: el título y el
   botón salen en la pantalla (**Stop** para acabar).

Si no pinta, el programa dice qué falta:

- «GUI: esta placa no tiene el pack 'LVGL' grabado» — falta el pack de la
  GUI.
- «GUI: el pack de la GUI esta, pero su motor no cargo: …» — el pack está
  grabado y su motor no carga. Detrás de los dos puntos va el porqué:
  «falta packRam= en el ENV» (sin bloque de RAM; también lo dice el
  **Info**), el aviso del motor (por ejemplo, «realojado para OTRA
  direccion…»: [cuándo regrabar](#regrabar)) o «mira el log de la placa».
- «GUI: no hay driver de pantalla ('PANT')…» — en la C5 y la C6, falta
  `DriverSpi.pack`.
- «GUI: pantalla: …» — una errata en las líneas del panel
  ([abajo](#panel-spi)).
- «GUI: la pantalla no arranco…» — el driver falló al iniciarse; el motivo,
  en el log (`log` en la consola).

Si la placa ya tenía motores cargados (la GUI o SQLite de antes), al grabar,
borrar o formatear se **reinicia sola** para cargar los de ahora, y el IDE
vuelve a conectar: espera a que vuelva antes de seguir. Más del panel en la
[guía del IDE §10.3](guia-ide.html#packs-panel).

### El bloque de RAM de los motores

Los motores nativos —la GUI, SQLite— guardan sus datos en un bloque de RAM
que la placa aparta **al arrancar**, antes que la memoria de la VM, con el
tamaño que diga el ENV: `packRam=<KB>` o, si no está, `SQLite=<MB>` (el de la
base de datos, que sirve también para la GUI). Sin ninguno de los dos no hay
bloque (en la Pico 2 y la Metro, 8 KB por defecto) y **ningún motor carga**:
el botón **Info** lo dice («RAM packs: no hay: un pack nativo no podría
cargar…»). Lo que se da al bloque se le quita a la VM; los valores de la
tabla son los de las placas probadas.

<a id="panel-spi"></a>
### Las líneas del panel (la C5 y la C6)

El driver SPI no sabe de placas: cada placa se describe en su ENV, con una
línea por bus y una por dispositivo. Son de la placa, no de la imagen: se
escriben una vez y sobreviven al reflasheo.

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

Para la ranura TF, una línea más: `sd=bus:2,cs:23,khz:10000` en la C5 y
`sd=bus:2,cs:4,khz:10000` en la C6 (la tarjeta va en el mismo bus que la
pantalla; el `miso` de `spi2` es suyo). Una errata no deja la pantalla negra
sin más: el programa recibe el motivo (por ejemplo, «GUI: pantalla: ENV lcd:
falta 'dc'»). Qué significa cada campo, y cómo describir otro panel SPI, está
en la cabecera de `bpstdlib/gui/drivers/spi/nativo/src/driver_spi.c` (en el
repositorio).

<a id="stdlib-pack"></a>
### El `Stdlib.pack`

Es la biblioteca estándar (`Core`, `Str`, `Collections`, `Json`, `Gui`…) en
la zona de packs. Desde el pack, cada módulo se ejecuta **en sitio, desde la
flash**, y a la RAM sólo van sus datos; desde `/lib` va entero a la RAM. Al
grabarlo, la placa **retira de `/lib`** lo que el pack sirve con los mismos
bytes que su imagen (una copia más nueva que hayas puesto tú se queda, y tapa
a la del pack); si lo borras, `/lib` se repone desde la imagen en el acto.
Graba el del mismo paquete que la imagen.

Se recomienda en las placas sin PSRAM —la C3, la Pico 2, la Nucleo y la
DK2—, y en la C6 es **imprescindible para la GUI**: sin él, `Gui` y los
módulos que usa van a la RAM y no caben. Los packs de librería (la GUI,
SQLite) no llevan copia de la biblioteca: la toman del `Stdlib.pack` o de
`/lib`. Si al grabar uno a la placa le falta algo, el panel avisa: «graba
tambien el Stdlib.pack».

<a id="regrabar"></a>
### Cuándo regrabar

Un pack con motor (`Gui`, `GuiMini`, `DriverSpi`, `SQLite`) queda **sellado**
para su sitio en la flash y en el bloque de RAM. Si ese sitio cambia, el motor
no carga (y la GUI lo dice: «el pack de la GUI esta, pero su motor no
cargo: …»). Hay que regrabarlo:

- al cambiar `packRam` o `SQLite` en el ENV;
- tras grabar una imagen nueva, si mueve la zona o el bloque (lo más sencillo:
  regrabarlos siempre que cambies la imagen);
- al pasar de V6 a V7: **todos**, con los del paquete (los de V6 no valen).

Lo seguro es **Formatear zona…** y grabarlos todos otra vez, en el orden de
la tabla: el bloque de RAM se reparte por orden de grabado, y un motor nuevo
detrás del viejo puede no caber (el IDE lo dice: «no cabe»). Formatear borra
también el `Stdlib.pack`: va el primero.

---

## ¿Qué versión tengo en la placa?

IDE → Connect → botón **Info**: el micro, la flash, la RAM y la PSRAM, el
bloque de RAM de los packs («RAM packs») y la **ABI de nativo** de la imagen.
Esa última línea dice si la imagen y el IDE casan: «9 (la misma que genera
este IDE)» es una imagen de V7 con un IDE de V7. Si no casan, o la línea no
aparece (una imagen anterior a V7), graba la imagen de este paquete —o, si la
imagen es más nueva que el IDE, pon el IDE al día—. (En la S3, que no tiene
AOT, una imagen de V7 dice «sin receta».) El Info no muestra la fecha de
compilación de la imagen.
