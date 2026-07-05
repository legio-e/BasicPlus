# Notas de versión — BasicPlus

> Borrador del cuerpo de la *release* de GitHub. La etiqueta propuesta es
> `v3.0` (v1 cerró en `v1.0`, v2 en `v2.0`). Ajusta versión/fecha al publicar.

---

## v3.0 — julio 2026 · «interfaz gráfica»

BasicPlus llega a su versión **V3**, cuya novedad principal es el **diseño de
interfaces gráficas** con la librería **LVGL**. LVGL trae una buena colección de
widgets y permite construir interfaces vistosas; nuestro trabajo ha consistido en
**integrarla en BasicPlus** de forma nativa. Además de la integración, hemos
añadido la posibilidad de **diseñar las pantallas en un fichero JSON** que, en
tiempo de ejecución, se convierte en la ventana con sus manejadores de eventos ya
conectados.

Aparte de la interfaz gráfica, hay **mejoras pequeñas** en el lenguaje, el IDE y
la máquina virtual, y hemos **reforzado el soporte de proyectos** — necesario para
las aplicaciones gráficas. Y por último, se suma el soporte de **tres placas con
pantalla**: un kit **Discovery** con STM32U5, el kit de **Espressif para el
ESP32-P4** y una pantalla de **4,3" de Waveshare** con ESP32-P4.

Después de esta versión vendrán otras, con nuevas mejoras y soporte de más
microcontroladores.

Y lo de siempre, que no cambia: **el mismo bytecode corre byte-idéntico en el PC y
en el micro** — ahora también la GUI.

### Lo nuevo desde v2.0

**Interfaz gráfica — el módulo `Gui` (LVGL)**
- Una **veintena de widgets** OO que heredan de `Component`: `Screen`, `Panel`,
  `Label`, `Button`, `Checkbox`, `Toggle`, `Slider`, `Bar`, `Spinbox`, `Led`,
  `Dropdown`, `Textarea`, `ListBox`, `Keyboard`, `Msgbox`, `Tabview`, `Table`,
  `Image`/`ImageView` y `Window`.
- **Layout por anclas** (`align`), **color** (`Gui.Color(0xRRGGBB)`) y **fuentes**
  (catálogo compilado + `.bin` LVGL cargables en runtime).
- **Táctil** — el toque enruta al widget y dispara su evento, igual que el clic
  sintético del host.
- **Formularios en JSON** (`.win`): diseñas la pantalla en un fichero y en runtime
  `Window.load()` la construye y **ata los handlers** (traducidos a *slots* de vtable
  al subir el proyecto). Alternativa OO: sobrescribir `onClick`/`onChange`.
- **Rotación en runtime** al estilo LVGL (`Gui.setRotation(90)`): intercambia
  dimensiones y transforma el táctil solo (ESP32-P4 y host).
- **Tres formas de correr una GUI**, el mismo `.mod`: vista previa rápida en el PC
  (miVM/Swing), **render pixel-exacto en el PC** (VM-C + LVGL/SDL) y la placa.

**Plataformas gráficas — tres placas con pantalla**
- **STM32U5G9J-DK2** (Discovery): panel LTDC 800×480 + táctil GT911, con AOT activo.
- **ESP32-P4-Function-EV** (RISC-V): panel EK79007 1024×600 (MIPI-DSI) + GT911.
- **Waveshare ESP32-P4 Touch-LCD-4.3"**: panel ST7701 480×800 + táctil.
- **Imagen única del ESP32-P4**: un **solo binario** sirve a las dos placas P4; el
  panel se elige en *runtime* leyendo `/sys/board.json` (como la imagen única del
  RP2350 para Pico 2 / Metro).

**Lenguaje, VM e IDE — mejoras**
- **AOT nativo desde el IDE**: *Run on Device* compila las funciones `native` a
  `.mdn` y las sube automáticamente (degrada a interpretado si algo falla). En una
  DK2, `fibobench` va ~95× más rápido en nativo.
- **`super()` implícito** al estilo Java cuando el constructor no lo llama.
- **Soporte de proyectos reforzado**: carpeta `resources/` que se sube en cada Run,
  ficheros `.win` con los handlers **horneados a slots**, y un *sidecar* `.slots`
  que el compilador emite por módulo.
- Capacidades de hardware **por placa** (nº de canales ADC/PWM correcto según el
  micro) y robustez del parser (control de flujo de una línea).

**Herramientas**
- **VM-C de host con LVGL/SDL**: abre una ventana SDL y pinta **exactamente igual
  que la placa** — la forma de previsualizar formularios `.win` en el PC.

### Artefactos de la release

| Artefacto | Para | Cómo se instala |
|---|---|---|
| `BpIde-3.0.jar` | El IDE (PC) | `java -jar …` (requiere JDK 8+) |
| `bpvm_pico.uf2` | Pico 2 **y** Metro RP2350 | BOOTSEL + copiar el `.uf2` |
| `bpvm_esp32_merged.bin` | ESP32-S3 | `esptool write_flash 0x0 …` |
| `bpvm_esp32p4.bin` | ESP32-P4-EV **y** Waveshare P4 | `esptool` / `idf.py flash` (panel por `/sys/board.json`) |
| `bpvm_stm32_nucleo.bin` | STM32 Nucleo-U575 | ST-LINK / STM32CubeProgrammer |
| `bpvm_stm32_dk2.bin` | STM32U5G9J-DK2 (con pantalla) | ST-LINK / STM32CubeProgrammer |
| `bpgenvm-c` (Windows) | Preview gráfico en el PC (LVGL/SDL) | `bpgenvm-c MiApp.mod` desde la carpeta del programa |

Instrucciones detalladas por placa: **[INSTALAR_FIRMWARE.md](INSTALAR_FIRMWARE.md)**.

### Limitaciones conocidas

Ninguna impide trabajar; todas tienen salida (detalle en la guía gráfica, §25):
- Tras parar una GUI con **Stop**, **resetea la placa** antes del siguiente Run (la
  pantalla puede quedar en estado inconsistente).
- **Preview de forms `.win` en el PC**: se hace con la VM-C + LVGL (§23.2); la vista
  previa rápida de Swing no copia `resources/`, así que no encuentra el `.win`.
- **STM32 (DK2)**: la rotación de pantalla **no** está en esta versión (avisa y sigue).
- El **modelo lógico** (480×320) y el **panel físico** aún no están unificados: un
  formulario sin tamaño explícito puede ocupar solo parte del panel (fija
  `width`/`height` en la raíz para llenarlo).
- **FS del RP2350**: con `/app` muy lleno de módulos, el firmware puede colgarse
  (caso límite; a investigar en v4).
- **`LIST`** en STM32 muestra ~14 entradas por pantalla (cosmético; a mejorar en v4).

### Cómo empezar

- **[Inicio rápido](QUICKSTART.md)** — de cero a blink, por plataforma.
- **[Interfaz gráfica](gui.html)** — la guía completa del módulo `Gui`: widgets,
  layout, color y fuentes, eventos y formularios, y cómo ejecutar en PC y placa.
- **[Documentación](index.html)** — manual del lenguaje, referencia de la stdlib,
  guía del IDE y la arquitectura por dentro.

### Diferido a v4

Anotado, no implementado (ver **[V4_BACKLOG.md](V4_BACKLOG.md)**): unificación del
modelo lógico y el panel físico, preview de forms en la vista Swing, rotación en la
STM32/LTDC, el cuelgue del FS con `/app` lleno, el `LIST` paginado, y — la línea
que no cambia — **más microcontroladores**.

### El invariante

La regla de oro sigue intacta: **la salida de un programa es byte-idéntica en la VM
de Java (`miVM`) y en la VM de C (`bpgenvm-c`)**, en el PC y en el micro. En V3 la
GUI se suma al invariante: el mismo `.mod` que previsualizas en el PC pinta en la
placa.

---

## v2.0 — junio 2026 · «código congelado»

Segunda entrega de BasicPlus, el lenguaje de propósito general para
microcontroladores de 32 bits que compila a un bytecode que corre
**byte-idéntico** en el PC y en el micro. Si v1 demostró la idea (lenguaje
+ dos VMs en paridad + dos familias de micro + IDE + depurador), **v2
consolida, endurece y amplía**: suma una tercera familia de micro, un
puñado de features de lenguaje muy pedidas, una biblioteca estándar de
verdad y un IDE redondeado — sin romper nunca la paridad dual-VM.

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
  mismo bytecode que corre en el PC.
- En el **STM32** (Nucleo-U575ZI-Q) y el **ESP32-S3** (DevKitC), validados en placa
  con sensores reales los **cuatro buses críticos**: GPIO, SPI (BME688), UART (loopback)
  e I2C (BME280) — las **tres familias a la par**. En ambas, los periféricos no críticos
  (PWM/ADC/RTC/WDT) existen en la API y se ejecutan, con backend HW para v3. El **Metro
  RP2350B** comparte la imagen de firmware con la Pico.

### Artefactos de la release

| Artefacto | Para | Cómo se instala |
|---|---|---|
| `BpIde-2.0.jar` | El IDE (PC) | `java -jar …` (requiere JDK 8+) |
| `bpvm_pico.uf2` | Pico 2 **y** Metro RP2350 | BOOTSEL + copiar el `.uf2` |
| `bpvm_esp32_merged.bin` | ESP32-S3 | `esptool write_flash 0x0 …` |
| `bpvm_stm32.bin` | STM32 Nucleo-U575 | copiar al disco del ST-LINK / STM32CubeProgrammer |

### El invariante

La regla de oro no cambió en toda la v2: **la salida de un programa es
byte-idéntica en la VM de Java (`miVM`) y en la VM de C (`bpgenvm-c`)**, en
el PC y en el micro. Es lo que hace que «depura en el PC, despliega en el
micro» no sea un eslogan.

---

*Hecho con cariño, tres placas en la mesa y memoria de los PDP-11.*
