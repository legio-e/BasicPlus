# BasicPlus

**Un lenguaje de propósito general para microcontroladores de 32 bits, con
sintaxis BASIC moderna, orientación a objetos y un debugger de primera clase.**

Compila a bytecode (`.mod`) que corre **idéntico** en el PC y en el micro:
el mismo programa que depuras en tu sobremesa parpadea después un LED en una
Raspberry Pi Pico 2, un ESP32-S3 o un STM32 — sin recompilar, sin `#ifdef`,
sin sorpresas.

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

- **Orientación a objetos completa**: clases con constructores, herencia
  (también entre módulos), `instanceof`, properties con `get`/`set`,
  properties `sync` (atómicas), miembros estáticos.
- **Tipos**: `integer`, `long` (i64), `float`, `double`, `boolean`, `string`
  (UTF-8 nativo, indexado por codepoints), tipos estrechos (`byte`, `word`,
  `short`, `int8`, `int16`), arrays (`integer[]`, `byte[]`, `long[]`, …) y
  arrays locales de tamaño fijo (`var buf: byte[64]`).
- **Tuplas** first-class (`(integer, string)`), **parámetros con valor por
  defecto**, expresiones multilínea, `switch/case`.
- **Excepciones**: `try/catch/finally`, jerarquía con base común `Exception`,
  `RuntimeError` nativo atrapable (división por cero, índices, red…),
  clases de excepción propias — también entre módulos.
- **Concurrencia**: threads BP sobre un scheduler de quanta, `Mutex`,
  `synchronized`, colas `SyncList` con consumo bloqueante.
- **Módulos** con interfaces compiladas (`.bpi`, modelo DEFINITION de
  Modula-2): `import` resuelve contra la interfaz, no contra el fuente.
- **`native function`**: funciones compiladas AOT a C/Thumb-2 (`.mdn`) que
  corren a velocidad nativa en RP2350/STM32 — 40-90× sobre el intérprete en
  kernels de cómputo, con faults propagados a `try/catch` BP.
- **Biblioteca estándar**: `Core`, `Math`, `IO` (ficheros + prompt), `Str`,
  `Collections`, `Stats`, `Compress` (LZSS), `Log`, `Json`, `Net` (cliente
  TCP) y el zoo de hardware: `Gpio`, `I2c`, `Spi`, `Uart`, `Pwm`, `Adc`,
  `Rtc`, `Wdt`, `Timer`, `Pulse`, `Neopixel`, `Pico`. Todo hardware nuevo
  es una clase (`Gpio.Pin`, `I2c.Bus`, `Net.Tcp`, …).

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
| PC (Windows/Linux/macOS) | — | Ambas VMs; daemon TCP para el IDE |
| Raspberry Pi **Pico 2** y Adafruit **Metro RP2350** | USB-CDC | **Una imagen de firmware única** para ambas placas: la variante (A/B), los pines y la PSRAM se deciden en *runtime* (`/sys/board.json`), no con macros de compilación. AOT activo. |
| **ESP32-S3** (Xtensa) | UART0 | FS persistente en partición dedicada; consola por USB nativo |
| **STM32** (ref. Nucleo-U575ZI-Q) | VCP del ST-LINK | FS en flash interna; AOT activo (mismo Cortex-M33 que el RP2350) |

En todas: REPL "wire v1" (JSON por línea) con subida de ficheros, RUN
remoto, **Stop** (KILL cooperativo sin resetear la placa), **autorun**
(`/sys/auto.txt` arranca tu programa al encender — dispositivo autónomo de
verdad, y el IDE puede conectarse en caliente y recuperar el control) y
debug on-device con breakpoints.

## El IDE

`BpIde` (Swing, un único jar): editor con pestañas, compilación con
diagnósticos, Run/Debug local y remoto, explorador de ficheros de la placa
con consola (`dir`, `run`, `kill`, `autorun`, `log`, …), INFO del micro,
breakpoints y paso a paso tanto en la VM local como dentro del dispositivo.

## Estructura del repositorio

```
lexer-java/   compilador (frontend): .bp → .mod + .bpi + .dbg (+ AOT .mdn)
miVM/         VM Java + debugger + daemon TCP
bpgenvm-c/    VM C99: host, firmware Pico/RP2350, ESP32-S3, STM32
BpIde/        IDE Swing (fat-jar)
bpstdlib/     biblioteca estándar (fuentes .bp + .mod compilados)
samples/      programas de ejemplo
docs/         manual, specs (.mod, opcodes, heap, wire), backlog
```

## Compilar y probar (PC)

Requisitos: JDK 8+, Maven, GCC (MinGW en Windows), `make`.

```sh
# 1. Toolchain Java (compilador + VM Java)
mvn -f miVM/pom.xml install
mvn -f lexer-java/pom.xml install

# 2. VM C de host
cd bpgenvm-c && make && cd ..

# 3. Compilar y ejecutar un ejemplo en AMBAS VMs
java -jar lexer-java/target/basicplus-frontend.jar samples/blink.bp \
     --compile samples --backend=mivm
java -jar miVM/target/bpgenvm-1.0.jar samples/Blink.mod
bpgenvm-c/build/bpgenvm-c samples/Blink.mod

# 4. (Opcional) el IDE
mvn -f BpIde/pom.xml package
java -jar BpIde/target/BpIde-1.0-SNAPSHOT-shaded.jar
```

Los firmwares se compilan con sus toolchains habituales (pico-sdk + ninja,
ESP-IDF, STM32CubeIDE); ver `bpgenvm-c/{pico,esp32,stm32}/`.

## Documentación

- **[Manual del lenguaje](docs/manual.html)** — léxico, tipos, clases,
  módulos, excepciones, concurrencia, biblioteca estándar, CLI y artefactos.
- [Filosofía del proyecto](docs/PHILOSOPHY.md) — qué es y qué no quiere ser.
- Especificaciones: [formato .mod](docs/MOD_FORMAT.md),
  [opcodes](docs/OPCODES.md), [layout del heap](docs/HEAP_LAYOUT.md),
  [builtins](docs/BUILTINS.md), [protocolo wire](docs/BPVM_WIRE_PROTOCOL.md).
- [Backlog vivo](docs/PENDIENTES.md) — el diario honesto del proyecto.

## Estado

**V2 — código congelado, en fase de documentación** (junio 2026). La V1
cerró con las tres familias de micros funcionando end-to-end; la V2 añadió
long/double, strings UTF-8, tuplas, defaults, static properties, herencia
cross-module completa, file I/O, logger, autorun, Stop, cliente TCP y un
IDE consolidado. Solo se aceptan correcciones de bugs; todo lo demás va a v3.

## Licencia

[MIT](LICENSE). Hecho con cariño, dos placas en la mesa y memoria de los
PDP-11.
