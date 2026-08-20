# Notas de versión — BasicPlus

> Borrador del cuerpo de la *release* de GitHub. La etiqueta propuesta es
> `v5.0` (v1 cerró en `v1.0`, v2 en `v2.0`, v3 en `v3.0`/`v3.0.1`, v4 en `v4.0`). Ajusta
> versión/fecha al publicar.

---

## v5.0 — agosto 2026 · «los datos»

V4 arregló los cimientos. V5 va de lo que un microcontrolador no sabía hacer
todavía: **guardar datos de verdad**. Una tarjeta SD con gigabytes, una base de
datos SQLite corriendo dentro del micro, un ORM que escribe el SQL por ti, y los
**packs**, que son lo que hace posible meter algo tan grande como SQLite sin que
lo pague quien no lo usa.

Lo que no cambia es el invariante de siempre: el mismo bytecode, la misma salida
byte a byte en las dos VMs, y todo verificado en placa.

### La tarjeta SD

Un micro con tarjeta deja de estar limitado a los pocos megas de su flash. Y se
usa sin aprender nada nuevo: **es una ruta más**.

```basic
writeFile("/sd/medidas.csv", "hora;valor\n")
```

La monta el firmware al arrancar, así que no hay que montar nada a mano. Los
pines **no van en el código**: se declaran en el entorno de la placa, de modo que
el mismo programa vale para dos cableados distintos.

Funciona en **RP2350** (por SPI) y en **ESP32** (por SDIO, 1 o 4 bits). En STM32
todavía no. Y conviene saber que es una capacidad de la *imagen*, no de la placa:
la Metro y el P4 traen lector soldado, pero en una Pico 2 basta con cablear uno.

Debajo hay **FatFs**, para que puedas sacar la tarjeta y leerla en el PC. La
flash interna sigue con littlefs.

### Bases de datos

BasicPlus habla **SQLite** — el motor de verdad, la versión 3.53.4, corriendo
dentro del micro. El mismo fichero `.db` y el mismo código en el PC y en la
placa.

```basic
var db: SQLite.Db := SQLite.Db()
db.connect("/sd/medidas.db")
print db.execDouble("SELECT avg(valor) FROM medidas WHERE sensor = 'temp'")
```

Están los tres verbos que hacen falta —mandar, pedir un dato, recorrer filas— y
una regla que hace difícil equivocarse: **sólo lo que devuelve `query` hay que
liberarlo**.

Y una que sorprende a quien viene de JDBC: aquí **se pueden anidar consultas**
sobre una sola conexión. Cada cursor avanza por su cuenta.

### El ORM

Encima de SQLite hay un ORM. Anotas tus clases y el compilador genera el DAO:

```basic
@BD{ tabla = "medidas" }
public class Medida
  @BD{ pk }   public property id:    long
  @BD{}       public property valor: double
end Medida
```

A partir de ahí, `insert`, `update`, `delete`, `loadById`, `list`… sin escribir
una sentencia SQL. Las condiciones se construyen con `Where`, que **escapa los
valores** — un nombre con apóstrofo no rompe nada.

Y hay una pieza que no se ve pero se agradece: si el proyecto declara su base de
datos, **el compilador contrasta tus entidades con el esquema real** y avisa de
lo que no cuadra. Son avisos, nunca errores: una base se diseña entera antes que
el programa, y bloquear el build por eso convertiría la herramienta en un
estorbo.

### Packs

Un pack es un contenedor que se graba en la placa: módulos, recursos y, si hace
falta, código nativo. Es lo que permite distribuir una librería en una pieza —y
lo que hace viable meter SQLite, que ocupa más que toda la VM junta.

Lo importante no es que ahorren flash, sino **RAM**: un módulo que vive en el
sistema de ficheros hay que cargarlo entero en memoria para ejecutarlo; uno que
vive en un pack **se ejecuta en el sitio**, desde la flash. A RAM sólo va su
tabla de símbolos y su bloque de datos. En una placa con unos cientos de
kilobytes, eso es la diferencia entre que una librería quepa o no quepa.

Se construyen desde un proyecto y se graban desde el IDE, que también los lista,
los borra y formatea su zona. Un programa puede además **descubrir en ejecución**
qué packs hay y qué llevan dentro.

### El lenguaje

`List` y sus parientes **ya no las sintetiza el compilador**: están escritas en
BasicPlus, en `Core`, y se leen como cualquier otra clase. Con ellas viven ahora
los **envoltorios** (`Integer`, `Long`, `Double`, `Float`, `Boolean`) y
`Comparable`.

`add` está sobrecargado, así que meter un número en una lista no pide ceremonia:

```basic
l.add(42)                        // se envuelve solo
var n: integer := l.getInteger(0)   // y sale convertido
```

Esos **captadores tipados** son nuevos y hacen bastante trabajo: `getInteger`,
`getLong`, `getFloat`, `getDouble`, `getString` y `getBoolean` no castean,
**convierten** — y protegen. `double` a entero trunca hacia cero; un `long` que
no cabe en un `integer` **lanza** en vez de recortar en silencio.

### Código nativo

Las funciones `native` aceptan ahora **`long`**, y con ellas los enteros de 64
bits cruzan a código C en los dos sentidos. Más importante todavía: **dividir por
cero desde código nativo lanza un error atrapable** en vez de reiniciar la placa,
que es lo que convierte `native` en algo que se puede usar sin miedo.

### Diagnóstico

El log post-mortem **sobrevive al reinicio**. Vive en una zona de RAM que el
arranque no borra, así que si un programa se cuelga y reinicias, la autopsia trae
las líneas de *antes* del cuelgue — que es justo cuando hacía falta y justo
cuando antes fallaba. Y el arranque dice de dónde viene lo que ha cargado, para
que nadie lea una autopsia sin saber de cuándo es.

### Lo que todavía no

Dicho sin adornos, porque conviene saberlo antes de empezar:

- **Las bases de datos necesitan placa.** El motor va en un pack de código nativo
  y todavía no hay uno para PC, así que un programa con SQLite no se puede probar en el PC.
- **`listDir` no está en la VM-C**, o sea que un programa puede listar un
  directorio en el PC pero no en la placa. Es el único verbo de fichero que
  falta.
- **exFAT no está soportado**: formatea las tarjetas en FAT32.
- **La tarjeta SD no llega al STM32** todavía.

---

## v4.0 — agosto 2026 · «consolidación»

En esta versión nos hemos centrado en **consolidar lo ya hecho** más que en
añadir características nuevas. En especial hemos reformado **la gestión de la
memoria** y **el sistema de archivos**, que eran las dos piezas que peor
envejecían. Aun así, por el camino han entrado algunas cosas nuevas que merecen
su propio apartado: los **eventos**, la **sobrecarga de funciones** y un **micro
simulado** dentro del IDE.

### Memoria

Hasta la versión 3 usábamos un **modelo de memoria plano**: una referencia era
una dirección absoluta. El problema de ese modelo es que es **poco robusto**,
sobre todo en entornos multitarea: una referencia a un objeto ya liberado sigue
pareciendo válida, y el fallo aparece mucho más tarde y muy lejos de su causa.

Lo hemos reformado por completo. Ahora una referencia es un **handle**: un
**índice** a una tabla más un **contador de generación**. Cuando un objeto se
libera, su contador cambia; si alguien conserva una referencia vieja y la usa,
los contadores **no coinciden y el error salta ahí mismo**, en vez de corromper
datos en silencio. Lo importante del modelo nuevo es doble: es **mucho más
robusto** y **apenas cuesta rendimiento** (se midió antes de adoptarlo).

Además hemos revisado las **zonas de RAM**. Hemos reducido la necesidad de
buffers y memorias intermedias, y hemos ampliado el uso de la RAM a **toda la
disponible**. Eso se traduce en más memoria para la pila de ejecución y más
memoria para crear objetos y vectores. Es especialmente importante en las placas
con poca memoria, pero también se aprovecha mejor en las que llevan PSRAM.

Y dos cambios en el **recolector de basura** que se notan en programas largos:

- **El heap ya no crece sin parar.** La lista de bloques libres pasa a estar
  **ordenada por dirección y a fusionar los huecos contiguos**, y el recolector
  se dispara por **volumen reservado** en vez de por cuánto ha crecido el heap.
  Antes, un programa consumía memoria nueva en cada recolección aunque no
  guardara nada, y acababa muriendo; ahora la memoria **se estabiliza y ahí se
  queda**. Verificado en placa con un bucle de 10.000 vueltas que antes no
  pasaba de la 1.000.
- **Quedarse sin memoria es ahora un error que puedes atrapar.** Antes, una
  reserva fallida podía devolver una referencia vacía sin decir nada y el
  programa seguía con datos corruptos. Ahora lanza una excepción normal, así que
  se recoge con `try` / `catch` como cualquier otra.

### Sistema de archivos

El sistema de archivos se ha reformado completamente. Lo primero, se ha
**unificado para todas las familias**: ahora hay un mismo sistema de archivos en
todas ellas, con el mismo comportamiento y los mismos límites.

También se ha dividido la **memoria flash en tres bloques**:

1. un bloque pequeño para las **variables de entorno** y el **registro del
   sistema**,
2. un segundo bloque para el **sistema de archivos** propiamente dicho,
3. y un tercero para grabar **Packs**, que es nuevo en esta versión.

El registro del sistema **sobrevive a un reinicio**, así que si una placa se
queda colgada puedes conectarte después y leer lo último que hizo.

### Packs

Un **Pack** no es más que una forma de guardar varios archivos dentro de uno
solo. A diferencia de un archivo comprimido, aquí los archivos se guardan **tal
cual, sin comprimir**, y ese es justo el objetivo: un Pack grabado en la flash
interna **se usa directamente desde la flash**, sin cargarlo en RAM. En un micro
donde la RAM es el recurso escaso, eso importa.

Los packs se gestionan **desde el IDE**: puedes crear proyectos que compilen a un
Pack, y hay una ventana que te permite ver y administrar los packs grabados en
una placa.

### Eventos

Otra novedad es la introducción de los **eventos**. Es especialmente relevante en
los entornos gráficos, aunque **no está limitado a ellos**: se puede usar en
cualquier objeto.

Con los eventos hemos añadido **llamadas asíncronas**, que por dentro usan hilos
de ejecución pero hacen la programación mucho más sencilla. Están pensadas sobre
todo para llamar a funciones que tardan sin que se bloquee el entorno gráfico.

> **Ojo**: las funciones que se llaman de forma asíncrona corren en **hilos
> distintos del principal**. Si vas a compartir variables entre ellas, usa los
> mecanismos de sincronización que ofrece el lenguaje para evitar corrupciones de
> datos.

### Sobrecarga de funciones

Ya se pueden declarar **varias funciones con el mismo nombre y distintos
parámetros**, y el compilador elige la que toca. Funciona con funciones libres,
métodos estáticos, **métodos de instancia con herencia** y **constructores**, y
también **entre módulos**.

### Un micro simulado dentro del IDE

Ahora el IDE trae un **micro simulado**: se comporta como una placa de verdad
—mismo protocolo, mismo sistema de archivos, misma consola— pero corre en el PC.
Puedes configurarle la RAM, la PSRAM, el tamaño de flash y la pantalla.

Sirve para **desarrollar y probar sin tener una placa delante**, y también para
comparar: si algo va en el simulado y no en la placa, ya sabes que el problema es
de la placa y no de tu programa.

### Código nativo

El compilador puede traducir a **código nativo** las funciones que marques, y la
placa lo carga y lo usa en lugar del bytecode. En esta versión llega también a
**RISC-V** (ESP32-P4), además de ARM.

La diferencia es grande: en el banco de pruebas, una función de cálculo puro pasa
de **58 segundos interpretada a 0,5 segundos en nativo**. No todo se puede
traducir —los límites están documentados y el compilador **avisa siempre al
compilar, nunca falla en ejecución**—, pero el bytecode sigue estando ahí, así
que es una optimización que se aplica donde interesa y no cambia nada más.

### Arranque por capas

El arranque de la placa ahora es **escalonado**: primero lo mínimo para
comunicarse, luego las particiones, luego el sistema de archivos, y por último la
máquina virtual. Si algo falla, la placa **se queda en el último nivel bueno y lo
dice**, en vez de quedarse muerta sin explicación. En la práctica significa que
una placa con el sistema de archivos estropeado **sigue respondiendo** y se puede
recuperar desde el IDE.

### Otros

- **El compilador ya no genera archivos `.bpi`**: cada módulo compilado lleva su
  propia interfaz dentro. Menos archivos que sincronizar y menos formas de que
  algo se quede desfasado.
- **Subida de archivos grandes por trozos**, para que un archivo grande no
  dependa de que quepa entero en memoria.
- **Nuevo widget de gráfica** para representar series de datos de sensores.
- **`random` y `randomInt`**, con la fuente de entropía de cada micro.
- El **depurador** muestra mejor las variables, y se han pulido bastantes
  detalles del lenguaje, la parte gráfica y el IDE.

---

## v3.0.1 — julio 2026 · «parche de memoria»

Versión de mantenimiento. Preparando el modelo de memoria de la próxima V4
salieron a la luz **cuatro fallos** en el **recolector de basura** y la
**liberación de objetos** del núcleo en C —el que comparten las tres familias de
micro—. Ninguno se dispara a diario, pero podían **corromper datos en silencio**
en programas que sobreviven a varias recolecciones. Lo correcto y lo honesto es
arreglarlos, así que aquí están: en **un único sitio**, de modo que **las tres
familias heredan el arreglo** al recompilar. No hay cambios de lenguaje ni de API
—tus programas compilan y corren igual—; solo el motor es más robusto.

### Lo que se corrige

- **Arrays de `long`/`double`** que sobrevivían a una recolección podían
  desaparecer. Ahora el GC los reconoce.
- Un **entero cuyo valor coincidía con una dirección del heap** podía hacer que
  el recolector **pisara datos vivos** (corrupción no determinista). Ahora el GC
  valida los candidatos contra las **cabeceras reales de objeto**, igual que la
  VM de Java.
- **Liberar un objeto «con dueño»** dejaba el heap en un estado que descuadraba
  el barrido posterior. Ahora la liberación deja el bloque consistente.
- Una **variable global de módulo** que fuera el único camino a un objeto vivo
  podía recolectarse **en vivo** (uso-tras-liberación). La causa de fondo era de
  **alineación**: el bloque de constantes y variables no siempre quedaba alineado
  a 4, y entonces los globales caían en direcciones que **ningún** recolector
  miraba. Se corrige alineando ese bloque en el compilador y añadiendo su escaneo
  en la VM-C. De regalo, evita accesos a enteros no alineados que **fallan en
  ARM/RISC-V**.

Y **dos más**, cazados al probar los binarios en las **seis placas** antes de publicar:

- **`gc()` colgaba en placas con heap grande** (Metro RP2350B, ESP32-P4, STM32
  Discovery): el mapa de cabeceras del recolector se dimensionaba por la capacidad
  total del heap, y en varios MB de PSRAM eso disparaba una reserva enorme en cada
  recolección. Ahora se dimensiona por la memoria realmente usada.
- **Un `long` se imprimía como «ld» en el STM32** (su `printf` reducido no incluye
  `long long`). Ahora los enteros de 64 bits se formatean sin depender de la
  librería C → correctos y byte-idénticos en todas las placas.

### Qué implica al actualizar

- Por la alineación, el **bytecode de la stdlib cambia unos pocos bytes**. Hay que
  **reflashear el firmware** de las placas con esta versión. El IDE resube la
  stdlib de host automáticamente (compara por CRC, así que solo sube lo que
  cambió).
- Nada más que hacer: mismo lenguaje, misma API, mismos artefactos.

### Artefactos de la release

Los mismos que en v3.0 (el IDE pasa a `BpIde-3.0.1.jar`); reconstruidos con el
núcleo parcheado. Ver la tabla de v3.0 más abajo y **[INSTALAR_FIRMWARE.md](INSTALAR_FIRMWARE.md)**.

### El invariante

Sin novedad donde importa: **la salida de un programa sigue siendo byte-idéntica
en la VM de Java (`miVM`) y en la VM de C (`bpgenvm-c`)**, en el PC y en el micro.
Los cuatro arreglos se verificaron con esa vara de medir.

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
