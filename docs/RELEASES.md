# Notas de versión — BasicPlus

> Borrador del cuerpo de la *release* de GitHub. La etiqueta propuesta es
> `v2.0` (la v1 cerró en el tag `v1.0`). Ajusta versión/fecha al publicar.

---

## v2.0 — junio 2026 · «código congelado»

Segunda entrega de BasicPlus, el lenguaje de propósito general para
microcontroladores de 32 bits que compila a un bytecode que corre
**byte-idéntico** en el PC y en el micro. Si v1 demostró la idea (lenguaje
+ dos VMs en paridad + dos familias de micro + IDE + depurador), **v2
consolida, endurece y amplía**: suma una tercera familia de micro, un
puñado de features de lenguaje muy pedidas, una biblioteca estándar de
verdad y un IDE redondeado — sin romper nunca la paridad dual-VM.

A partir de aquí el código queda **congelado**: solo correcciones de bugs;
todo lo nuevo va a v3.

### Lo nuevo desde v1.0

**Lenguaje**
- Escalares: `long` (i64), `double` (f64), tipos estrechos (`byte`, `word`,
  `short`, `int8`, `int16`) y casts numéricos generales.
- `string` = `byte[]` UTF-8, indexado por **codepoints**; conversión
  `string`↔`byte[]`.
- Arrays `byte[]` / `long[]` y arrays locales de tamaño fijo (`var buf: byte[64]`).
- **Tuplas** first-class (`(integer, string)`) con destructuring a lvalues
  no-simples.
- **Parámetros con valor por defecto** (coste cero en la VM).
- **Static properties** de clase.
- **Herencia cross-module completa**: vtables, `super(...)`, `instanceof`,
  miembros estáticos y tipos de clase en las signaturas — todo entre módulos.
- **Excepciones**: jerarquía con base común `Exception`, `RuntimeError`
  nativo atrapable (división por cero, índices, red…), clases de excepción
  propias, también cross-module.
- Expresiones multilínea (continuación implícita dentro de paréntesis/corchetes).
- **Funciones `native` (AOT)** muy ampliadas: bucles, `float`/`double`,
  arrays, llamadas a builtins, lvalues compuestos, tipos mixtos, variables
  de módulo, operaciones de string, puente native→BP y **faults propagados
  a `try/catch` BP**. ~40–90× sobre el intérprete en kernels de cómputo.

**Biblioteca estándar**
- `Core` (con `Exception`/`RuntimeError`, import implícito), `Math`, `IO`
  (ficheros + `prompt`), `Str`, `Collections`, `Stats`, `Compress` (LZSS),
  `Log`, `Json` y `Net` (cliente TCP).
- El zoo de hardware, todo como **clases OO**: `Gpio.Pin`, `I2c.Bus`,
  `Spi.Bus`, `Uart.Port`, `Pwm.Slice`, `Adc.Channel`, `Rtc.Clock`,
  `Wdt.Timer`, `Timer.Alarm`, `Pulse`, `Neopixel`, `Pico`.

**Plataformas y firmware**
- **Tercera familia: STM32** (ref. Nucleo-U575ZI-Q), con **AOT activo**
  (mismo Cortex-M33 que el RP2350; ~45× medido en placa en un kernel LZSS).
- **RP2350**: una **imagen única** para Raspberry Pi Pico 2 y Adafruit Metro
  RP2350 — la variante (A/B), los pines y la PSRAM se deciden en *runtime*
  (`/sys/board.json`), nunca con macros de compilación.
- **Stop**: KILL cooperativo de punta a punta, sin resetear la placa.
- **Autorun**: `/sys/auto.txt` arranca tu programa al encender — dispositivo
  autónomo de verdad, y el IDE puede **conectarse en caliente** y recuperar
  el control.
- File I/O persistente en la placa; `wire v1` (JSON por línea) con subida de
  ficheros y RUN remoto en las tres familias; debug on-device con breakpoints.

**IDE (`BpIde`)**
- Consola del micro con línea de comandos (`dir`, `run`, `kill`, `autorun`,
  `log`…), doble-clic para ver/editar ficheros del device, `File → New`,
  carpeta `resources/` que se sube en cada Run, INFO del micro y una UI
  genérica («Placa»/«Device»).

**Calidad del compilador**
- Anti-cascada del parser, recuperación semántica tras un error de parseo
  (más errores reales en una sola pasada) y *poisoning* de operadores para
  no filtrar tipos internos a los diagnósticos.
- **Paridad dual-VM byte-idéntica** verificada en cada feature que toca la VM.

**Verificado en hardware real**
- La escalera de hardware completa —GPIO, I2C (sensor BMP280), SPI (sensor
  BME688 con paginación de memoria), UART, PWM + contador, ADC, RTC, watchdog
  y timers— validada **en placa** sobre Raspberry Pi Pico 2 / Pico 2 W, con el
  mismo bytecode que corre en el PC. Las demás familias comparten ese núcleo de
  VM; la pasada exhaustiva de periféricos por placa continúa.

### Artefactos de la release

| Artefacto | Para | Cómo se instala |
|---|---|---|
| `BpIde-1.0-SNAPSHOT-shaded.jar` | El IDE (PC) | `java -jar …` (requiere JDK 8+) |
| `bpvm_pico.uf2` | Pico 2 **y** Metro RP2350 | BOOTSEL + copiar el `.uf2` |
| `bpvm_esp32_merged.bin` | ESP32-S3 | `esptool write_flash 0x0 …` |
| `bpvm_stm32.bin` | STM32 Nucleo-U575 | copiar al disco del ST-LINK / STM32CubeProgrammer |

Instrucciones detalladas por placa: **[INSTALAR_FIRMWARE.md](INSTALAR_FIRMWARE.md)**.

### Cómo empezar

- **[Inicio rápido](QUICKSTART.md)** — de cero a blink, por plataforma.
- **[Documentación](index.html)** — manual del lenguaje, referencia de la
  stdlib, guía del IDE y la arquitectura por dentro.

### Diferido a v3

Anotado, no implementado (ver **[PENDIENTES.md](PENDIENTES.md)**): WiFi/TCP
en placa (lwIP), AOT cross-module sin puente del intérprete, compilación
`native` integrada en el IDE, dual-core en el RP2350, multiplexado del USB
CDC, FS grande del Metro y mapa de memoria configurable por PSRAM.

### El invariante

La regla de oro no cambió en toda la v2: **la salida de un programa es
byte-idéntica en la VM de Java (`miVM`) y en la VM de C (`bpgenvm-c`)**, en
el PC y en el micro. Es lo que hace que «depura en el PC, despliega en el
micro» no sea un eslogan.

---

*Hecho con cariño, tres placas en la mesa y memoria de los PDP-11.*
