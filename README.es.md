# BasicPlus

> 🇬🇧 [English version](README.md)

**Un lenguaje de propósito general para microcontroladores de 32 bits, con
sintaxis BASIC moderna, orientación a objetos, interfaz gráfica y un debugger
de primera clase.**

Compila a bytecode (`.mod`) que corre **idéntico** en el PC y en el micro:
el mismo programa que depuras en tu sobremesa parpadea después un LED en una
Raspberry Pi Pico 2, un ESP32 (S3, C3, C5, C6 o P4) o un STM32 — sin recompilar,
sin `#ifdef`, sin sorpresas.

```basic
// blink.bp — parpadea el LED on-board de la Pico (GP25) con el API OO.
module Blink
  import Gpio

  function Main()
    var led: Gpio.Pin := Gpio.Pin(25, Gpio.Pin.OUTPUT)
    var i: integer := 0
    while i < 100 do
      led.on()
      sleep(300)
      led.off()
      sleep(300)
      i := i + 1
    endwh
    print "Blink terminado"
  end Main
end Blink
```

En un PC sin GPIO los builtins de hardware loggean por stdout; en la placa
mueven pines de verdad. El programa es el mismo byte a byte.

## Nuevo en V8

V8 trae tres cosas grandes:

- **La WiFi y el Bluetooth** en las ESP32 con PSRAM —la S3, la C5 y la P4 de
  Waveshare, que lleva la radio en su ESP32-C6—. La placa se conecta sola a la
  red al arrancar, y el IDE trabaja con ella sin cable: sube los ficheros, graba
  los packs y ejecuta los programas. Del Bluetooth, de momento, el teclado y el
  ratón. Las dos radios están apagadas de normal, y se apagan solas tras diez
  minutos sin uso.
- **JC**, un lenguaje nuevo: una mezcla de Java y C# para quien trabaja con esos
  lenguajes, sin pretender ser ninguno de los dos, y con nuestras librerías. Por
  dentro es BasicPlus —el mismo compilador y el mismo bytecode—, y un programa se
  traduce de uno a otro desde el IDE. Para probarlo, las pruebas de C# de Mono:
  de 2 322, funcionan 848, y se quedan como prueba de caja negra de los dos
  lenguajes. Por el camino, BP y JC ganaron los operadores compuestos y el
  ternario (`iif`), las interfaces y los genéricos.
- **La fiabilidad.** Cuatro placas, 48 horas seguidas con un programa que lo
  toca todo, sin un error ni un reinicio, buscando sobre todo que ningún recurso
  —la memoria— se fuera perdiendo con el tiempo; la prueba encontró dos fallos de
  verdad, ya arreglados. Todo lo que se prueba, en **[Cómo se prueba
  BasicPlus](docs/pruebas.html)**.

Y además: lo pendiente de V7 y lo que ha ido saliendo, arreglado; las cadenas,
mucho más rápidas (en la Pico 2, recorrer una de 2 KB, de 535 a 20 ms); los
errores de la VM, con su tipo (`ArithmeticError`, `IndexError`…); el sonido en
la P4 de Waveshare; y una placa más, la **ESP32-P4X**, la nueva revisión del P4,
con su imagen, del mismo código.

Para actualizar desde V7: graba la imagen de V8 antes que nada, y después la
stdlib y los packs de V8; la S3 y la C5 se borran enteras una vez (su aplicación
pasa a 2 MB por la WiFi); y recompila tus programas. Detalle completo en las
**[notas de versión](docs/RELEASES.md)**.

## Interfaces gráficas (desde V3)

Sobre **LVGL**: una veintena de widgets, la posibilidad de **diseñar las
pantallas en un fichero JSON** que en tiempo de ejecución se convierte en la
ventana (con sus manejadores de eventos ya conectados), y táctil — todo con el
mismo "pinta en el PC, corre en la placa". Cinco placas con pantalla:
**STM32U5G9J-DK2** (LTDC), **ESP32-P4-Function-EV** (EK79007), una
**Waveshare ESP32-P4** (ST7701), la pequeña **Waveshare ESP32-C6-LCD-1.3**
(ST7789 240×240 por SPI) y, nueva en V7, la **Waveshare
ESP32-C5-Touch-LCD-2.8** (ST7789 240×320 por SPI, táctil). Desde V7, LVGL va
en un **pack** que se graba en la placa: que una placa tenga pantalla lo
decides tú, y la que no la tiene no la paga.

![GuiColorDemo en placa — GUI con LVGL](docs/img/guicolordemo.png)

*La demo `GuiColorDemo` — el mismo bytecode corre en el PC y en el micro.
Todo en la **[guía de interfaz gráfica](docs/gui.html)**.*

---

## El nombre (y una deuda de gratitud)

BasicPlus toma su nombre del **BASIC-PLUS de Digital Equipment Corporation**,
el dialecto estructurado de BASIC que corría bajo RSTS/E en los PDP-11 durante
los años 70 y 80. Aquel lenguaje demostró que un BASIC podía ser serio: con él
se escribieron sistemas completos, y con él aprendió a programar una
generación. Este proyecto es un homenaje a esa idea — **un lenguaje sencillo
de leer que no te trata como a un principiante** — trasplantada al hardware
pequeño de hoy: los microcontroladores de 32 bits.

## Qué incluye el lenguaje

- **Orientación a objetos completa**: clases con constructores y destructor
  (`~Clase()`), herencia (también entre módulos), `instanceof`, properties
  con `get`/`set`, properties `sync` (atómicas), miembros estáticos. Las
  clases raíz (`Object`, `Thread`, `Mutex`, `StringBuilder`) están escritas
  en el propio BasicPlus, en `Core`.
- **Tipos**: `integer`, `long` (i64), `float`, `double`, `boolean`, `string`
  (UTF-8 nativo, indexado por codepoints), tipos estrechos (`byte`, `word`,
  `short`, `int8`, `int16`), arrays (`integer[]`, `byte[]`, `long[]`, …) y
  arrays de tamaño fijo (`var buf: byte[64]`), en una función o dentro de un
  objeto.
- **Tuplas** first-class (`(integer, string)`), **parámetros con valor por
  defecto**, expresiones multilínea, `switch/case`.
- **Excepciones**: `try/catch/finally`, jerarquía con base común `Exception`,
  `RuntimeError` nativo atrapable (división por cero, índices, red…),
  clases de excepción propias — también entre módulos.
- **Concurrencia**: threads BP sobre un scheduler de quanta —en las placas de
  dos núcleos, dos de ellos a la vez de verdad, uno por núcleo—, `Mutex`,
  `synchronized`, colas `SyncList` con consumo bloqueante.
- **Módulos** (modelo DEFINITION de Modula-2): `import` resuelve contra la
  interfaz compilada del módulo, que viaja dentro del propio `.mod` — un
  fichero por módulo.
- **`native function`**: funciones compiladas AOT a código nativo (Thumb-2 en
  RP2350/STM32, RISC-V en el ESP32-P4 y, desde V7, en los ESP32-C3, C5 y C6)
  que viaja dentro del `.mod` — 40-90× sobre el intérprete en kernels de
  cómputo, con faults propagados a `try/catch` BP. Dentro, casi todo el
  lenguaje: cualquier builtin, llamadas a BP, `try/catch/finally`; y una larga
  cede a los demás hilos y se para con Stop.
- **Interfaz gráfica**: el módulo `Gui` sobre **LVGL** — ~20 widgets OO,
  formularios diseñados en JSON (`.win`), color y fuentes, táctil. El mismo
  bytecode pinta en una ventana del PC y en la pantalla de la placa.
- **Biblioteca estándar**: `Core`, `Math`, `IO` (ficheros con `IO.File` e
  `IO.TextFile`, prompt), `Str`, `Collections` (con un `Map` tipado), `Stats`,
  `Compress` (LZSS), `Log`, `Json`, `Net` (cliente TCP), `App`, `Packs`,
  `Machine` (id del chip, temperatura, reloj, reset), `Time` (fechas), `Heap`
  (memoria) y el zoo de hardware: `Gpio`, `I2c`, `Spi`, `Uart`, `Pwm`, `Adc`,
  `Rtc`, `Wdt`, `Timer`, `Pulse`, `Freq`, `Neopixel`. **SQLite** y su ORM van
  como pack. Todo el hardware es una clase (`Gpio.Pin`, `I2c.Bus`, `Net.Tcp`,
  …), y lo que un programa toma se suelta al acabar.

## Dos VMs gemelas, un invariante sagrado

El bytecode lo ejecutan **dos máquinas virtuales independientes**:

| VM | Lenguaje | Dónde corre |
|---|---|---|
| **miVM** | Java | PC (desarrollo, debugging, CI) |
| **bpgenvm-c** | C99 | PC (host) y los firmwares de los micros |

La regla de oro del proyecto: **la salida de un programa debe ser
byte-idéntica en ambas VMs**. Cada feature se verifica contra ese invariante
antes de entrar. Es lo que hace que "depura en el PC, despliega en el micro"
no sea un eslogan.

## Plataformas soportadas

| Plataforma | Transporte | Notas |
|---|---|---|
| PC (Windows/Linux/macOS) | — | Ambas VMs; daemon TCP para el IDE. La VM-C + LVGL/SDL pinta la GUI en una ventana (`--screen=WxH` imita el panel de una placa). |
| Raspberry Pi **Pico 2** y Adafruit **Metro RP2350** (Cortex-M33, dos núcleos) | USB-CDC | **Una imagen de firmware única** para ambas placas: la variante (A/B) se detecta por hardware; los 8 MB de PSRAM de la Metro se activan con `psram=1` en el ENV de la placa, y la identidad/pines de la placa (`name`, `ledPin`, `neopixelPin`) salen de `/sys/board.json`. Por defecto, dos hilos BP corren a la vez, uno por núcleo. AOT activo. |
| **ESP32-S3** (Xtensa, dos núcleos) | bridge UART0 | 8 MB de PSRAM como memoria de la VM; FS persistente y packs en particiones dedicadas; consola por el USB nativo. Por defecto, dos hilos BP corren a la vez, uno por núcleo. `native` corre interpretado (sin AOT Xtensa). |
| **ESP32-C3** y **ESP32-C6** (RISC-V sin FPU, un núcleo) — *desde V6* | USB-Serial-JTAG (el único USB) | El wire va por el único conector USB; la consola, por UART0. Sin PSRAM: la VM, hasta 128 KB. La **Waveshare ESP32-C6-LCD-1.3** tiene pantalla por SPI (ST7789 240×240, sin táctil), que pintan los packs `GuiMini` y `DriverSpi` (con el pack `Stdlib` también, o la GUI no cabe). AOT activo desde V7 (`ilp32`). |
| **ESP32-C5** (RISC-V sin FPU, un núcleo) — *nueva en V7*, con pantalla | USB-Serial-JTAG (el único USB) | La **Waveshare ESP32-C5-Touch-LCD-2.8**: 8 MB de PSRAM para la VM, 32 MB de flash (el FS en los 16 MB de arriba), ranura TF y pantalla ST7789 240×320 por SPI con táctil, que pintan los packs `Gui` y `DriverSpi`. El wire va por el único conector USB; la consola, por UART0. AOT activo (`ilp32`). |
| **ESP32-P4** (RISC-V, dos núcleos) — con pantalla | bridge UART; en la P4X, USB-Serial-JTAG | GUI con LVGL sobre panel MIPI-DSI + táctil: el driver del panel va en la imagen y LVGL, en el pack `Gui`. **Una imagen única** para las placas de la revisión v1.x del chip, el kit Function-EV (EK79007 1024×600) y la Waveshare 4.3" (ST7701 480×800); el panel se elige en *runtime* con `display=st7701` en el ENV de la placa. Desde V8, otra para la v3.x, la **ESP32-P4X** (su kit Function-EV, con el mismo panel de 7"), del mismo código: el IDF no deja que una imagen sirva para las dos revisiones. Por defecto, dos hilos BP corren a la vez, uno por núcleo. AOT activo (RISC-V). |
| **STM32** (Nucleo-U575ZI-Q · Discovery U5G9J-DK2) | VCP del ST-LINK | FS en flash interna; AOT activo (mismo Cortex-M33 que el RP2350). El **Discovery DK2** añade pantalla LTDC (800×480) + táctil: el driver del panel va en la imagen y LVGL, en el pack `Gui`. El `Adc` funciona (ADC1, desde V7); el `Wdt` no tiene backend en esta familia (su IWDG no se puede parar una vez arrancado): lanza. |

Nueve imágenes, once placas. La configuración de la placa vive fuera del
programa, en dos mitades: identidad y pines en `/sys/board.json` (solo
RP2350); lo demás, en el **ENV** de la placa (botón *Entorno* del IDE): la
PSRAM, el panel, el bloque de RAM de los motores de los packs (`packRam=`),
las líneas del bus y del panel de una pantalla SPI, y los núcleos:
`vm.cores=1` y `vm.runs=1` dejan una placa de dos núcleos en uno sin
regrabarla. Qué grabar en una placa con pantalla, y qué líneas pide su ENV, en
**[Instalar el firmware](docs/INSTALAR_FIRMWARE.md)**.

En todas: REPL "wire v1" (JSON por línea) con subida de ficheros, RUN
remoto, **Stop** (KILL cooperativo sin resetear la placa), **autorun**
(`/sys/auto.txt` arranca tu programa al encender — dispositivo autónomo de
verdad, y el IDE puede conectarse en caliente y recuperar el control) y
debug on-device con breakpoints (mientras se depura, los hilos del programa se
turnan, como con un solo núcleo). El cable tiene su propio hilo de SO, `io`,
aparte del programa: el IDE sigue atendido (Stop, reset, HELLO y, desde V7,
listar y leer ficheros) mientras el programa corre.

## Verificado en hardware real

No es solo teoría de sobremesa: la pila completa se ha ejercitado en silicio.
En una **Raspberry Pi Pico 2 / Pico 2 W** (RP2350) se ha validado de punta a
punta —desde el IDE, con *Run on Device* y el **mismo bytecode** que corre en
el PC— toda la escalera de hardware:

| Subsistema | Prueba en placa | |
|---|---|:--:|
| Ejecución / OO | clases stdlib cross-module despachando en el micro | ✅ |
| GPIO | LED parpadeando (`Gpio.Pin`) | ✅ |
| I2C | sensor real BMP280 (T/P) + escaneo de bus | ✅ |
| SPI | BME688 por SPI: `chip_id` + lecturas T/HR/P/gas con paginación de memoria | ✅ |
| UART | loopback, eco de bytes | ✅ |
| PWM + contador | 1 kHz / 500 Hz contados por hardware, < 0.1 % de error | ✅ |
| ADC | temperatura interna del chip (23.9 °C) | ✅ |
| RTC | reloj monotónico + recalibración | ✅ |
| Watchdog | feed / timeout / disable | ✅ |
| Timers | alarmas hardware (polling, sincronizado, cronómetro) | ✅ |

En el **STM32** (Nucleo-U575ZI-Q) se han validado además en placa, con sensores
reales, los **cuatro buses críticos**: GPIO, **SPI** (BME688), **UART** (loopback)
e **I2C** (BME280, T/P). Y en el **ESP32-S3** (DevKitC) se validaron en placa esos
**mismos cuatro buses** con sensores reales (BME688 por SPI, BME280 por I2C, loopbacks
de GPIO y UART) — las **tres familias no gráficas quedan a la par**. El **Metro
RP2350B** comparte la imagen de firmware con la Pico. El **ESP32-C3** y el
**ESP32-C6** se sumaron en V6, la **ESP32-C5** en V7 y la **ESP32-P4X** en V8, con el
mismo código común. La ronda de pruebas de V8 pasó en las once placas, con las
imágenes que se publican: la Pico 2, la Metro, la S3, la C3, la C5, la C6, las tres
P4 (el kit Function-EV, la de Waveshare y el kit de la P4X), la Nucleo y la DK2.

**Las placas con pantalla (desde V3).** La GUI (LVGL) se ha verificado en placa en las
seis: **STM32U5G9J-DK2** (LTDC 800×480 + táctil GT911), **ESP32-P4-Function-EV**
(EK79007 1024×600), **ESP32-P4X-Function-EV** (el mismo panel, desde V8),
**Waveshare ESP32-P4** (ST7701 480×800), **Waveshare ESP32-C6-LCD-1.3** (ST7789
240×240, sin táctil) y **Waveshare ESP32-C5-Touch-LCD-2.8** (ST7789 240×320 +
táctil) — catálogo de widgets, color, formularios `.win` y táctil, con el mismo
bytecode en todas. Desde V7, con LVGL desde un pack.

## El IDE

`BpIde` (Swing, un único jar): editor con pestañas, compilación con
diagnósticos, Run/Debug local y remoto, explorador de ficheros de la placa
con consola (`dir`, `run`, `kill`, `autorun`, `log`, …), INFO del micro,
gestión de placa (particiones, el entorno de la placa y el panel de
**Packs**, desde el que se graban la pantalla, SQLite o la biblioteca
estándar), breakpoints y paso a paso tanto en la VM local como dentro del
dispositivo.

## Estructura del repositorio

```
toolchain/    lo de Java:
  pack/         el formato de los packs
  miVM/         VM Java + debugger + daemon TCP
  lexer-java/   compilador de BP y JC: .bp/.jc → .mod + .dbg (el código nativo va dentro del .mod)
  BpIde/        IDE Swing (fat-jar)
bpgenvm-c/    VM C99: host + firmwares de RP2350, ESP32-S3/C3/C5/C6/P4, STM32
bpstdlib/     biblioteca estándar (fuentes .bp + .mod compilados); los packs de la GUI, en gui/
packs/        los packs; en el repo, sólo Stdlib.pack (los demás se construyen o vienen en la release)
samples/      programas de ejemplo
docs/         manual, guía gráfica, specs (.mod, opcodes, heap, wire), backlog
```

## Compilar y probar (PC)

Requisitos: JDK 8+, Maven, GCC (MinGW en Windows), `make`.

```sh
# 1. Toolchain Java (el formato de los packs, la VM Java y el compilador), en este orden
mvn -f toolchain/pack/pom.xml install
mvn -f toolchain/miVM/pom.xml install
mvn -f toolchain/lexer-java/pom.xml install

# 2. VM C de host  (añade LVGL=1 para la ventana de la GUI; ver bpgenvm-c/README)
cd bpgenvm-c && make && cd ..

# 3. Compilar y ejecutar un ejemplo en AMBAS VMs
java -jar toolchain/lexer-java/target/basicplus-frontend.jar samples/blink.bp \
     --compile samples --backend=mivm
java -jar toolchain/miVM/target/bpgenvm-1.0.jar samples/Blink.mod
bpgenvm-c/build/bpgenvm-c samples/Blink.mod

# 4. (Opcional) el IDE
mvn -f toolchain/BpIde/pom.xml package
java -jar toolchain/BpIde/target/BpIde-8.0.jar
```

Los firmwares se compilan con sus toolchains habituales (pico-sdk + ninja,
ESP-IDF, STM32CubeIDE); ver `bpgenvm-c/{pico,esp32,esp32c3,esp32c5,esp32c6,esp32p4,stm32}/`.
Los packs de la GUI (`Gui`, `GuiMini`, `DriverSpi`) se construyen desde
`bpstdlib/gui/` (cada `.bpbuild` dice cómo). O coge los **binarios
precompilados** —imágenes y packs— de la [última release](https://github.com/legio-e/BasicPlus/releases/latest).

## Documentación

- **[Inicio rápido](docs/QUICKSTART.md)** — de cero a blink, por plataforma.
- **[Instalar el firmware](docs/INSTALAR_FIRMWARE.md)** — flashear la VM en
  cada micro (imagen precompilada o compilada por ti), y los packs que pide
  una placa con pantalla.
- **[Manual del lenguaje](docs/manual.html)** — léxico, tipos, clases,
  módulos, excepciones, concurrencia.
- **[JC, BasicPlus con llaves](docs/jc.html)** — la sintaxis de Java y C# sobre
  el mismo compilador y el mismo bytecode: cada construcción, en JC y en BP, y
  cómo traducir de uno a otro.
- **[Referencia](docs/referencia.html)** — biblioteca estándar completa,
  línea de comandos y artefactos en disco.
- **[Guía de interfaz gráfica](docs/gui.html)** — el módulo `Gui`: widgets,
  color y fuentes, formularios (`.win`), ejecución en host y placa.
- **[Guía del IDE](docs/guia-ide.html)** — la ventana, proyectos, Run/Stop,
  el explorador de la placa, la consola del micro y el depurador.
- **[Basic Plus desde dentro](docs/bp-desde-dentro.html)** — la arquitectura:
  compilador, bytecode, las dos VMs, GC, AOT y firmwares.
- **[Cómo se prueba BasicPlus](docs/pruebas.html)** — todas las pruebas que pasa
  cada versión, en el PC y en las placas, con sus números y los fallos que cazaron.
- [Filosofía del proyecto](docs/PHILOSOPHY.md) — qué es y qué no quiere ser.
- Especificaciones: [formato .mod](docs/MOD_FORMAT.md),
  [opcodes](docs/OPCODES.md), [layout del heap](docs/HEAP_LAYOUT.md),
  [builtins](docs/BUILTINS.md), [protocolo wire](docs/BPVM_WIRE_PROTOCOL.md).
- [Limitaciones conocidas](docs/PENDIENTES.md) — lo que el lenguaje aún no hace, dicho sin adornos.

## Estado

**V8** (noviembre 2026). La V1 demostró la idea; la V2 la
endureció y la amplió; la V3 le puso cara; la V4 le arregló los cimientos; la V5
le dio dónde guardar cosas; la V6 puso orden en el código de los
microcontroladores; la V7 dejó una imagen por micro; **la V8 lo conecta**: la WiFi
y el Bluetooth en las ESP32 con PSRAM, y el IDE trabaja con la placa sin cable.
Trae además JC, un segundo lenguaje con el aire de Java y C# sobre el mismo
compilador y el mismo bytecode, probado con las pruebas de C# de Mono; y una
prueba de fiabilidad de 48 horas en cuatro placas. Entra una placa nueva, la
**ESP32-P4X**, la nueva revisión del P4. Nueve imágenes para once placas y tres
arquitecturas; verificado en hardware real en **las once**.

Descargas y detalle completo: la
**[release v8.0](https://github.com/legio-e/BasicPlus/releases/tag/v8.0)** y las
**[notas de versión](docs/RELEASES.md)**; lo que trajo V7 —una imagen por micro, la
pantalla y SQLite en packs, el segundo núcleo, la ESP32-C5—, en la
**[release v7.0](https://github.com/legio-e/BasicPlus/releases/tag/v7.0)**.

## Licencia

[MIT](LICENSE). Hecho con cariño, placas en la mesa y memoria de los
PDP-11.
