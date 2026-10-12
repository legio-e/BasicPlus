# Notas de versión — BasicPlus

> Borrador del cuerpo de la *release* de GitHub. La etiqueta propuesta es
> `v7.0` (v1 cerró en `v1.0`, v2 en `v2.0`, v3 en `v3.0`/`v3.0.1`, v4 en `v4.0`, v5 en
> `v5.0`, v6 en `v6.0`). Ajusta versión/fecha al publicar.

---

## v7.0 — octubre 2026

En esta versión nos hemos centrado en tres puntos:

- **Los packs.** En las versiones anteriores se desarrolló la zona de packs: una zona fija de la flash donde se graban
  los packs, que no necesitan cargarse en RAM, y así se ahorra memoria. Se pensó primero para los módulos y luego se
  extendió al código nativo; el primer gran logro fue poder cargar SQLite. En esta versión 7 lo hemos extendido a
  LVGL, el motor de la interfaz gráfica. Todo esto representa un cambio de paradigma: hay una imagen por micro, y es el
  usuario el que decide si incorpora SQLite y el entorno gráfico, sin necesidad de crear imágenes ni compilar código C.
- **Terminar lo pendiente.** La versión 6 era para arreglar un montón de cosas que se habían quedado a medias, y bugs.
  Esta versión 7 es más de lo mismo: se ha resuelto una ingente cantidad de cosas pendientes. Entre ellas, las
  funciones nativas: había casos que no se podían convertir a `native`, y ahora prácticamente se han reducido a cero.
- **El segundo núcleo.** Lo hemos habilitado para aumentar el rendimiento de los programas BasicPlus que usan varios
  hilos. En todos los micros de dos núcleos ha mejorado el rendimiento, aunque no por igual: con dos hilos trabajando a
  la vez, en la ESP32-P4 se consigue el 200 % del rendimiento de un núcleo (en las cadenas, el 133 %), y en el RP2350,
  entre el 113 % (Metro) y el 168 % (Pico 2); allí, en cambio, las cadenas van más lentas que con uno. Todavía no sabemos
  la razón de la diferencia.

En resumen, esta versión, que podía haber sido una versión de transición, se ha convertido en una gran versión. Hay que
destacar el enorme trabajo de Claude Code, tanto en la revisión del código como en las pruebas y en el código nuevo.

Lo que no cambia es el invariante de siempre: el mismo bytecode, la misma salida
byte a byte en las dos VMs, y todo verificado en placa.

### Los packs: la pantalla, la base de datos y la biblioteca, a elección del usuario

**LVGL sale de las imágenes** (hito H6). Ninguna de las ocho imágenes lleva ya el motor
de la interfaz gráfica: va en un pack que se graba en la zona de packs, como SQLite desde
V5. Cada micro tiene una sola imagen, lleve la placa pantalla o no, y las imágenes
adelgazan: la de la P4, 644 KB de flash y 368 KB de RAM estática menos; la de la DK2,
627 KB de flash y 179 KB de RAM; la de la C6, 626 KB. La P4 y la DK2 siguen llevando en
la imagen el driver de su panel y de su táctil; en la C5 y la C6, también el driver es
un pack.

Qué se graba en cada placa con pantalla, además de la imagen:

| placa | pantalla | packs | bloque de RAM (en el Entorno) |
|---|---|---|---|
| ESP32-C5 (Waveshare ESP32-C5-Touch-LCD-2.8) | ST7789 240×320 por SPI + táctil CST3530 | `Gui` + `DriverSpi` | `packRam=512` |
| ESP32-C6 (Waveshare ESP32-C6-LCD-1.3) | ST7789 240×240 por SPI | `GuiMini` + `DriverSpi` + `Stdlib` | `packRam=136` |
| ESP32-P4 (kit EV y Waveshare 4,3") | MIPI-DSI + táctil | `Gui` | `SQLite=4` o `packRam=<KB>` |
| Discovery STM32U5G9J-DK2 | LTDC 800×480 + táctil GT911 | `Gui` | `packRam=768` |

En la C5 y la C6 la pantalla se describe además en el Entorno de la placa —las líneas
`spi2=` y `lcd=` y, en la C5, `i2c0=`, `exp=` y `touch=`—: es de la placa, no de la
imagen. Las líneas exactas, en [INSTALAR_FIRMWARE.md](INSTALAR_FIRMWARE.md), «Los packs
de la placa». `Gui.pack` es uno para todas las placas: lleva tres motores —Cortex-M33,
RISC-V con FPU y RISC-V sin FPU—, y a la placa sólo va el suyo.

**Cómo se graba**: en el IDE, botón **Placa** (la ventana *Gestión de placa*) → a la
derecha, el panel de packs → **Copiar pack a la placa…**. El IDE poda el pack a la familia
de la placa y sella su motor para el bloque de RAM que ella le da (si no cabe, lo dice
antes de grabar); si la zona está sin formatear, **Formatear zona…**. Si la placa ya tenía
motores cargados, grabar, borrar o formatear un pack la reinicia sola, y el IDE vuelve a
conectar (`#671`).

**El bloque de RAM de los motores** se pide en el **Entorno** de la placa —`packRam=<KB>`,
o `SQLite=<MB>` como en V6— y vale desde el siguiente arranque. Sin él, el INFO del IDE lo
dice («RAM packs: no hay: un pack nativo no podría cargar…») y no carga ningún motor, ni
el de la GUI ni el de SQLite; con él, se reparte entre todos los grabados: la P4 carga
LVGL y SQLite en el mismo arranque. Y sin el pack de la GUI, `Gui.Screen()` lanza un
`RuntimeError` con el motivo —«esta placa no tiene el pack 'LVGL' grabado (la GUI llega
por el pack)»—, y con el pack grabado pero su motor sin cargar, el porqué: «el pack de la
GUI esta, pero su motor no cargo: falta packRam= en el ENV» (`#682`).

**El `Stdlib.pack`** (`#468`): la biblioteca estándar entera en la zona de packs,
ejecutada desde la flash (XIP) sin copiarse a la RAM. Al grabarlo, la placa quita de
`/lib` las copias que el pack sirve con los mismos bytes, y las repone si se borra el
pack; y una copia idéntica que quede en el sistema de ficheros también se carga del pack
(`#650`). Conviene en las placas sin PSRAM —la C3, la Pico 2, la Nucleo y la DK2—, y en
la C6 es imprescindible para la GUI: sin él, sus módulos se cargan en la RAM y no caben.

**Los packs de librería ya no llevan la biblioteca estándar** (`#623`): `SQLite`, `Gui`,
`GuiMini` y `DriverSpi` traen sólo lo suyo y la toman del `Stdlib.pack` (o de `/lib`, a la
RAM). Antes cada uno llevaba sus copias, que se quedaban viejas y tapaban a las buenas. Al
grabar uno, el panel de Packs avisa si a la placa le falta algo («graba tambien el
Stdlib.pack»). Un pack *programa* —el que tiene `main`— sigue llevando todo lo que usa,
la stdlib incluida: es un contenedor.

**Los motores nativos, también en las STM32**: en la DK2 corren SQLite (`SqlDemo` entero)
y la GUI, cada uno con su parte del bloque.

**La S3 ya graba packs** (`#676`): se reiniciaba con un *panic* al grabar uno —leía la
zona por la dirección del bus de instrucciones, que en Xtensa no admite lecturas de un
byte—; ahora la lee por el de datos.

**Un panel SPI nuevo no necesita código C**: `DriverSpi` es un driver genérico para
paneles MIPI-DCS por SPI de 4 hilos. La placa se describe en el Entorno —una línea por bus
y una por dispositivo— y el controlador, en un fichero de texto con su secuencia de
arranque: el del ST7789 viene en el pack, y el de otro chip lo puede escribir el usuario
(`seq:`). El reloj del SPI no pasa del máximo de la hoja de datos del controlador (`#567`:
62 MHz en el ST7789).

### Placas

**Entra la ESP32-C5** (hito H11): la Waveshare ESP32-C5-Touch-LCD-2.8, un RISC-V sin FPU
con 8 MB de PSRAM, 32 MB de flash, pantalla ST7789 de 240×320 por SPI con táctil CST3530 y
ranura TF. Como la C3 y la C6, tiene **un solo USB**, el del cable del IDE, y se pone en
modo de grabación con BOOT+RESET (`esptool --chip esp32c5`); la consola del sistema sale
por la UART0 (GPIO11/12). La VM va a la PSRAM (7 677 KB), el sistema de ficheros a los
16 MB de arriba de la flash —solo, sin tocar las particiones—, sus `native` se compilan
(abajo) y su pantalla y su TF comparten el bus SPI.

El parque de V7 son **ocho imágenes y diez placas**, en tres arquitecturas (Cortex-M33,
RISC-V con y sin FPU, y Xtensa):

| imagen | placas que sirve |
|---|---|
| `bpvm_pico.uf2` | Pico 2 · Metro RP2350B |
| `bpvm_esp32_merged.bin` | ESP32-S3 |
| `bpvm_esp32c3_merged.bin` | ESP32-C3 |
| `bpvm_esp32c5_merged.bin` | ESP32-C5 *(nueva)* |
| `bpvm_esp32c6_merged.bin` | ESP32-C6 |
| `bpvm_esp32p4_merged.bin` | P4 kit · P4 Waveshare |
| `bpvm_stm32_nucleo.bin` | Nucleo U575 |
| `bpvm_stm32_dk2.bin` | Discovery U5G9J |

Las diez han pasado la ronda de pruebas de V7.

Y en las que ya estaban:

- **La Pico 2, la Metro, la S3 y la P4** ejecutan con dos núcleos (abajo).
- **La DK2 gira** (`Gui.setRotation`), que en V6 no hacía.
- **Las STM32 tienen `Adc`** (el ADC1, a 12 bits; en V6 lanzaba) y cargan packs con motor
  nativo.
- **La familia ESP32 tiene `Neopixel`** (por el RMT), y las migas de `Machine`
  (`setMark`…) funcionan en las tres familias.
- **La SD, en el código común**: la C5 y la C6 la leen por SPI, compartiendo el bus con la
  pantalla (`sd=bus:N,cs:M,khz:K` en el Entorno); la Metro deja sacar y meter la tarjeta
  sin reiniciar, y en la P4 `sd mount` monta la que se mete después de arrancar (`#566`).
- **La C3 y la C6**: las opciones del ESP-IDF, una a una, les devuelven 16 y 9,5 KB de
  SRAM (hito H13).
- **Los pines del sistema** —los de la flash y la PSRAM, y los del cable del IDE— y los de
  la pantalla que pone la imagen ya no se ofrecen al programa: pedirlos es un error con
  nombre (`#574`, `#669`, `#670`).

### Los dos núcleos

**Dos núcleos por defecto en la Pico 2, la Metro, la S3 y la P4** (hito H12): la VM
ejecuta dos hilos del programa a la vez, uno en cada núcleo. Las demás —la C3, la C5, la
C6 y las dos STM32— tienen un núcleo y siguen con uno. Un programa no cambia: si tiene un
solo hilo, no gana nada —en el RP2350, incluso va más lento: la tabla de abajo—; si tiene
varios, la VM los reparte. Y se vuelve atrás sin regrabar, con dos claves del
Entorno: `vm.runs=1` (un hilo a la vez) y `vm.cores=1` (todo en el primer núcleo).

En la Pico 2, la Metro y las ESP32, el sistema son el hilo `io` —el cable, la salida y el
log— y un hilo `run` por núcleo que ejecuta el programa (en V6, `vm` e `io`);
las STM32 siguen con su tarea `vm`. Dos casos van con un solo `run` aunque haya dos: un
programa que se depura y uno que usa SQLite. La GUI funciona con dos: sus llamadas las
ejecuta siempre el mismo `run`, y el árbol de componentes lleva su cerrojo.

Lo medido con dos hilos que trabajan a la vez, frente a un núcleo:

| placa | cálculo, arrays, objetos | cadenas | un programa de un hilo |
|---|---|---|---|
| ESP32-P4 | ×2 | ×1,33 | igual |
| Pico 2 | ×1,55–1,68 | ×0,83 | un 1,8 % más lento con la imagen de dos núcleos; un 8,4 % con dos `run` |
| Metro | ×1,13–1,57 | ×0,75 | un 35 % más lento (`#665`, abajo) |

**Lo compartido lo protege el programa.** Con dos núcleos, dos hilos que tocan los mismos
datos sin un `Mutex` pueden pisarse de verdad: es cosa del programa, como en Java. La VM,
en cambio, no se rompe —en la Pico 2, un programa que lo hace a propósito corrió 999
veces sin romperla (`#660`, `#661`)—, y un `print` sale entero aunque dos hilos impriman a
la vez (`#600`).

**Un fallo fatal con dos núcleos** (en la RP2350): la placa deja su informe en el log y se
reinicia en medio segundo; el INFO dice «fallo fatal», y el arranque siguiente no lanza el
autorun —un programa que fallara al arrancar sería un bucle de reinicios—. **Las ESP32
tampoco lanzan el autorun** tras un pánico o el perro guardián de las interrupciones, con
uno o con dos núcleos (`#681`): lo dicen en el log y en la consola, y el INFO da la causa
(`panic/exception`, `int-watchdog`). El perro guardián del módulo `Wdt` no es un fallo
fatal: tras él, el autorun se lanza.

### El lenguaje y la biblioteca estándar

**El módulo raíz** (hito H1). `Object`, `Thread`, `Mutex` y `StringBuilder` están escritas
en BasicPlus, en `Core.bp`, y `Core` es el módulo del sistema: **`import Core` ya no hace
falta** —lo pone el compilador en todo módulo; escribirlo sigue valiendo—. Un `Thread` no
es un `Object`, y redeclarar una de esas cuatro clases es un error.

**El destructor `~Clase()`.** El contrario del constructor: lo llama el lenguaje cada vez
que el objeto se libera, lo libere quien lo libere —su `var owner` al salir del ámbito,
al reasignarlo o con una excepción; el GC; el final del programa (`#668`)—. Es virtual y
encadena con el del padre; si lanza, se dice y la limpieza sigue. Y el bloque **`try var`**
suelta su recurso al salir del bloque, salga por donde salga:

```basic
module Registro
  public class Fichero
    var nombre: string
    public function Fichero(n: string)
      this.nombre := n
      print "abre " + n
    end Fichero
    public function ~Fichero()
      print "cierra " + this.nombre
    end ~Fichero
  end Fichero

  public function main()
    var owner a: Fichero := Fichero("a")
    try var b: Fichero := Fichero("b")
      print "trabajando con b"
    endtry                    // cierra b
    print "fin de main"
  end main                    // cierra a
end Registro
```

**Al acabar un programa, todo vuelve a como estaba** (`#666`–`#668`). Bien, con un error
o con el Stop del IDE, se destruyen sus objetos —el más nuevo primero, y de cada uno,
antes lo que posee—, y cada clase de la biblioteca que toma algo del hardware lo suelta
en su destructor: `Pwm.Slice`, `Pulse.Counter`, `Neopixel.Strip` y `Uart.Port` tienen
`release()` y destructor; `Gpio.Pin`, `Spi.Device` y `Adc.Channel` devuelven el pin al
estado de reset; `~Timer` desarma el perro guardián y `~Tcp` cierra la conexión. Usar un
objeto después de su `release()` es un error con nombre. Lo que no suelta ningún objeto
lo suelta el sistema al final del RUN, y lo dice.

**Arrays fijos que no son referencias** (`#19`). Un `tipo[N]` vive en el marco de la
función o dentro del objeto —un campo `private var`—, sin heap ni GC; se indexa, se
recorre y se mide con `len()`, pero no se pasa, ni se asigna, ni se devuelve. A nivel de
módulo ya no existe (allí, `tipo[]`).

```basic
public class Media
  private var muestras: integer[8]   // dentro del objeto: ni heap ni GC
  var n: integer := 0

  public function add(v: integer)
    this.muestras[this.n mod len(this.muestras)] := v
    this.n := this.n + 1
  end add
end Media
```

**Compatibilidad entre versiones, como Java** (hito H2, `#523`). El `.mod` pasa al formato
**MOD9**: ya no lleva las ranuras ni los desplazamientos de las clases de otros módulos,
sino referencias por nombre que la VM resuelve al cargar. Un programa compilado contra
una librería corre con una versión más nueva de ella —con métodos, campos o properties
añadidos o reordenados—, y lo incompatible se rechaza al cargar nombrando el símbolo
(«…: recompila 'X'»). Una dependencia más antigua que la que pide el módulo también se
rechaza, con su texto (`#500`; salida 2).

**Un solo orden para buscar los módulos** (`#493`), en el PC y en la placa y en las dos
VMs: el pack que se ejecuta → `/app` → `/lib` → la zona de packs. La ruta del `from` es
una pista para el compilador y no se usa al ejecutar; y si un módulo de `/app` tapa al de
`/lib`, la salida lo avisa (`aviso: 'Str' cargado desde /app, tapa al de /lib`). El
`--help` de la VM Java ya da ese orden (`#678`).

Y el resto del lenguaje:

- `Math.sign` se sobrecarga: `sign(float)` sustituye a `signF`; y `len` vale para una
  cadena y para cualquier array.
- Una global vale lo que dice su declaración (`#590`): un literal se graba en el `.mod`;
  cualquier otra expresión —un array literal, una llamada, un objeto— se calcula al cargar
  el módulo, antes de su función inicializadora. También las properties de módulo, cuyo
  valor inicial V6 descartaba.
- `Clase(x)` baja desde cualquier antepasado, no sólo desde `Object` (`Perro(a)` con
  `a: Animal`); e `instanceof` acepta una clase de otro módulo (`instanceof Lib.Clase`).
- El `step` de un `for` admite un entero con signo o una expresión (`#506`).
- Inicializadores de campo (`var n: integer := 5`), un constructor que delega en otro
  (`MiClase(args)` en su primera línea) y la visibilidad del constructor, que ahora se
  aplica (`#511`, `#515`).
- `switch` sobre un entero, un `long`, un `boolean`, un enum o un `string` (`#546`).
- `try … finally` sin `catch` (`#532`); un `break` o un `continue` que sale de un `try`
  corre su `finally` (`#673`); y un `finally` no puede salir con `return`, `break` ni
  `continue` (`#674`), porque taparía la excepción que fuera en vuelo:

```basic error
function lee(): integer
  try
    return 1
  finally
    return 2      // error: un 'finally' no puede salir con 'return'
  endtry
end lee
```

- Leer una variable local que todavía no tiene valor es un error de compilación (`#540`).
- Un enum en una cadena es su número (`"c=" + c` da `c=1`) (`#522`).

**La biblioteca estándar:**

- `IO.File` e `IO.TextFile` (`#491`): un fichero abierto con su cursor (`seek`, `skip`,
  `readLine`, `eof`…), todo su estado en el objeto y sin tope de ficheros abiertos; con
  `var owner` o `try var`, se cierra solo.
- `IO.dirExists`, y `fileExists` sólo para ficheros, igual en todas partes (`#630`);
  `IO.mkdir` crea las carpetas intermedias también en la SD (`#631`); y `listDir` funciona
  ya en la VM-C y en las placas.
- Cada verbo de fichero entero (`writeFile`, `appendFile`, `readFile`, `removeFile`…) es
  una apertura y un cierre: si choca con un fichero abierto, «ya está abierto» (`#657`).
- Dos módulos nuevos: `Time` (`Time.Date` sobre un `long` de segundos, sin el tope de 2038)
  y `Heap` (el estado del heap desde el programa, y `Heap.compact()`).
- `Collections.Map` con sobrecargas tipadas: claves `integer`, `string` u objeto, y el
  valor con su tipo (`getInteger`, `getString`…).
- `Machine.getEnv(clave)` lee el Entorno de la placa, y `Rtc.Clock.isSet()` dice si el
  reloj está en hora (`#626`).
- La tabla de capacidades de cada micro (`Machine.uarts()`, `i2cs()`, `spis()`,
  `hasPin()`…) y el estado del pin (`Pin.ANALOG`, `Pin.OPEN_COLLECTOR`, `Gpio.AnPin`, y
  `Adc.Channel` por el mapa de pines).
- Los buses compartidos: sus pines en el Entorno (`spiN=`, `i2cN=`), un árbitro que da el
  bus por turnos, y `Spi.Device`, `Spi.Transaction`, `I2c.Transaction` y `writeRead`.
- Las cadenas, mucho más rápidas en placa: 192 concatenaciones en la Metro, de 2 122 a
  44 ms (`#554`); y `StringBuilder` guarda piezas: `appendStr`, de 94 a 19 ms (`#587`).

```basic
print Math.sign(-2.5)                      // -1 (en V6, Math.signF)
var d: Time.Date := Time.Date(2038, 1, 19, 3, 14, 8)
print d.addDays(1).toDateString()          // 2038-01-20

try var f: IO.TextFile := IO.TextFile("datos/medidas.csv", "r")
  while not f.eof() do
    print f.readLine()
  endwh
endtry                                     // el fichero se cierra aquí
```

### El código nativo

**Una `native` es un hilo más** (hito H5). Si llama a algo que bloquea —`sleep`, un
`Mutex` ocupado, una función BP que espera— se suspende y los demás hilos siguen; en un
cálculo largo cede el turno cada 5 ms, y **el Stop del IDE la para** aunque esté en un
bucle infinito (una recursión cuenta como un bucle: `#580`). Mirarlo cuesta poco: en la
Pico 2, en los dos bucles de enteros con que se midió, una native sigue siendo 134 y 96
veces más rápida que interpretada.

**Casi todo se puede convertir a `native`.** Una native llama a cualquier función
intrínseca de la stdlib —`Math.sin`, `sqrt`, `startsWith`, `sleep`…— por el mismo código
que el intérprete; lleva `try`/`catch`/`finally` dentro (`#578`), lee y devuelve tuplas
(`#577`), hace conversiones estrechas (`byte(x)`, `int16(x)`…), usa `float`, `double` y
`long` en todas las placas, y llama a métodos de una clase de otro módulo (`#533`). Lo
poco que queda fuera —leer campos, los estáticos, `for … in`, `instanceof`, un `try var`
dentro, una tupla con un campo `float`, `long` o `double`…— lo dice el compilador con su
línea (manual §8.3), también en un método `native` de una clase, que en V6 corría
interpretado sin decir nada (`#519`).

```basic
native function media(datos: integer[], n: integer): integer
  try
    var s: integer := 0
    var i: integer := 0
    while i < n do
      s := s + datos[i]
      i := i + 1
    endwh
    return s / n
  catch e: RuntimeError
    return 0              // n = 0: la división lanza, y se atrapa dentro
  endtry
end media
```

**La C3, la C5 y la C6 compilan sus natives** (destino `riscv-imc`, ABI `ilp32`, con el
mismo `riscv32-esp-elf-gcc` del ESP-IDF que la P4). Medido en la C5: una suma que tarda
124 ms interpretada, 1 ms nativa; en la C3, con enteros, unas 100 veces más rápida; con
`double`, 5 veces (307 ms frente a 1 609), y con `float`, casi 6 (200 frente a 1 141): sin
FPU, las dos se emulan. La S3 (Xtensa) sigue interpretando.

**Depurar**: en el PC una native se depura como cualquier función; en la placa el
depurador no para dentro de ella —corre como código máquina—, y el IDE lo dice al poner el
breakpoint (`#583`). **Y la ABI**: los `.mdn` de V7 son de la ABI 9; el INFO del IDE dice
la de la placa («ABI nativo: 9»), y si no casa con la suya, el IDE no compila un nativo
que la placa rechazaría y dice cómo arreglarlo (`#576`).

### Interfaz gráfica

- **LVGL, en un pack** (arriba). Y la DK2 ya gira, como las demás.
- **El árbol de componentes, completo**: `Bar`, `Led`, `Chart` y `Table` se dan de alta en
  su contenedor —cuentan en `childCount()` y reciben clics—, y `toJson()` guarda los
  rangos, las opciones y los ítems, los botones de un `Msgbox`, los colores, la forma de un
  `Chart` y la rejilla de un `Table` (faltan los títulos de las pestañas, la imagen de un
  `ImageView`, el campo de un `Keyboard` y las series de un `Chart`). El dueño de un widget
  es su contenedor; con un `var owner` local, su destructor lo borra del árbol y de la
  pantalla.
- **`GuiMini`**, el LVGL de las pantallas pequeñas (la C6): 256 widgets, fuentes hasta
  24 px y un solo búfer de dibujo. Llenar la tabla de widgets lanza «Gui: no caben mas
  widgets: esta GUI tiene sitio para N» (512 con `Gui`; `#568`); en V6 el widget de más,
  simplemente, no aparecía.
- **Un manejador** (el `onClick` de un botón, Forms, un evento) puede ceder y lanzar
  (`#607`), y un hilo que muere sin atrapar su excepción lo dice en la salida.
- **El I2C del táctil de la P4 es un bus compartido** (`#669`): un programa se engancha a él
  con sus mismos pines a 100 kHz; con otros, recibe un error con nombre.
- **Pintar la pantalla entera**, con LVGL en el pack: 25 ms en la P4 (480×800), 35 en la
  C6, 48 en la DK2 (37 con LVGL en la imagen) y 70 en la C5.

### El IDE

- **Durante un RUN se pueden bajar ficheros y refrescar el árbol** (`#495`): el `GET` y el
  `LIST` van por trozos, entre la salida del programa, y un `KILL` no espera a que acaben.
  Lo que escribe en la placa sigue esperando a que el programa termine.
- **Las subidas, por trozos** (`#495`): la placa dice en el saludo qué trozo y qué ventana
  admite, el IDE reintenta si falla el cable, y una subida que se corta se borra —no queda
  un fichero a medias que parezca el nuevo—. La Pico 2 sube 64 KB en 1,1 s.
- **Lo dañado, en rojo**: el árbol marca los `.mod`, `.mdn` y `.pack` que la placa no
  cargaría, con el motivo; y el panel de Packs, los packs mal.
- **El INFO dice más**: el bloque de RAM de los motores («RAM packs», `#602`), la receta del
  AOT y la ABI de nativo de la placa (`#576`), y el PWM en la unidad de cada micro —12
  slices en la RP2350— (`#677`).
- **Las librerías se suben por su versión** (`#624`): una librería —la stdlib, lo
  instalado— sólo viaja si la placa no la tiene, si la tiene en otro formato o si su
  versión es anterior a la que pide el programa; otra compilación de la misma versión ya no
  se sube. Las dependencias salen de los `.mod`, también el `Core` implícito (`#538`).
- **El panel de Packs** avisa de la stdlib que le falta a un pack (`#623`), dice «no cabe»
  antes de grabar un motor y reconecta solo cuando la placa se reinicia tras grabar
  (`#671`). Y una placa con la zona de packs sin formatear lo dice al conectar, y el IDE
  ofrece la gestión de placa.
- **El depurador** avisa al poner un breakpoint dentro de una native (`#583`), y el Stop con
  el programa parado en un breakpoint de la placa ya lo para (`#634`).
- El IDE reinicia la placa tras aplicar las particiones, y el micro simulado se porta como
  una placa: su flash es tan estricta como la de verdad y aplica las particiones al
  arrancar (`#647`).
- El jar pasa a `BpIde-7.0.jar`.

### La ejecución y la memoria

- **Hilos sin tope** (`#604`): un programa crea los que quepan en la memoria —en V6 la VM-C
  admitía 32 en toda su vida—, y `Thread(stackSize)` se respeta (0 = 2 KB).
- **La pila se vigila** (`#605`): una función que no cabe lanza `RuntimeError`
  «desbordamiento de pila (el hilo tiene N B)», atrapable e igual en las dos VMs.
- **Una excepción sin atrapar en un hilo que no es el `main` acaba ese hilo** (`#608`): los
  demás siguen, y al final el programa sale con 1.
- **El módulo `Heap`** (`#594`): el tamaño, lo libre, el mayor bloque libre, los handles y
  las pasadas del GC, desde el programa; y `Heap.compact()` (`#595`), que compacta a
  petición —sólo desde `main` y sin otros hilos vivos—.
- **`vmheap=<KB>`** en el Entorno fija el tamaño del bloque de la VM (hito H7).
- **Cargar un programa gasta menos RAM del sistema** (`#651`): los nombres que necesita el
  enlazador van en un bloque por módulo y no en uno por nombre (`GuiWinJson`, de 818 a 54
  bloques). La C6 con la GUI deja ~34 KB libres en su peor momento (antes, 2), y su VM
  queda en 127 KB: 63 de heap y 64 de pilas.
- **En las placas de 32 MB** (la C5 y la P4 de Waveshare), el sistema de ficheros va en los
  16 MB de arriba de la flash y la zona de packs se queda el resto; ese reparto no se cambia
  desde el editor de particiones.
- El cuanto de la VM Java se cuenta en instrucciones (1 024), como en la VM-C: el
  intercalado de hilos y eventos es el mismo en las dos (`#531`).

### Pruebas

- **La ronda de V7**: 50 pruebas por placa (`tools/tanda_v7.json`), con la salida comparada
  con la del PC, verde en las nueve placas el 9-oct —saltando lo que una placa no tiene: la
  pantalla, el AOT en la S3, memoria en la C6—. Y la de soltar los recursos (`#667`), 13 por
  placa: un programa toma todos los PWM, las UART o las tiras NeoPixel de la placa, se para
  con un Stop, y el siguiente los vuelve a tener.
- **Los dos núcleos**: 110 minutos de resistencia con dos `run` en la Pico 2, la S3 y la P4,
  sin un fallo.
- **La red de paridad dual-VM** pasa de 59 a 148 casos; y las VMs de V7 siguen ejecutando
  byte a byte los 59 `.mod` de V6 de la cápsula, con su stdlib.
- **102 pruebas de C** de la VM en la red (el AOT, el depurador, el arranque, la memoria…), y
  18 programas de `samples/errores/`, que tienen que fallar, fallan con el mensaje que toca
  (49 comprobaciones).
- Los 399 programas de `samples/` compilan con el compilador de V7.

### Bugs de V6 corregidos

- **Una lista enlazada de unos cientos de nodos colgaba la placa**: el marcado del GC era
  recursivo y desbordaba la pila sin decir nada (en la Pico 2, unos 360 nodos; `#648`).
  Igual al soltar una cadena larga de `owner` (`#656`) y al cargar un programa con muchos
  niveles de `import` (`#649`).
- **Un hilo que recursaba de más pisaba en silencio la pila del vecino**: nadie vigilaba el
  desbordamiento (`#605`).
- **Un `switch` sobre un `string`, un `long`, un `double` o un objeto no casaba nunca** —iba
  siempre al `default`—, y un `break` dentro de un `case` salía del bucle de fuera (`#546`).
- **`continue` en un `for` no avanzaba la variable**: el bucle no acababa (`#548`).
- **Un `break` o un `continue` que salía de un `try` no corría su `finally`** y dejaba su
  `catch` armado para lo que lanzara el código de fuera (`#673`); y un `return` dentro de un
  `finally` tumbaba el compilador (`#674`).
- **`"c=" + c` con un enum reventaba el heap** en las dos VMs, y `Color(1)` devolvía basura
  (`#522`).
- **Imprimir un objeto de una subclase de `Thread` ejecutaba su `run()`** (`#526`).
- **Las sobrecargas de un método entre módulos llamaban a otro método en silencio** (`#514`),
  y una derivada sin constructor, construida desde otro módulo, perdía los argumentos
  (`#513`).
- **El valor inicial de una property de módulo se descartaba**: valía `null` (`#590`).
- **Una llamada usada como sentencia que devuelve 8 bytes** —una cadena, un `long`, un
  objeto— dejaba basura en la pila: en un bucle, la desbordaba (`#620`).
- **Un manejador de la GUI no podía ceder**: un `sleep` dentro lo cortaba en silencio; y si
  lanzaba, la VM-C acababa con «OK» (`#607`). Y **una excepción sin atrapar en un hilo**
  acababa el programa en una VM y no en la otra (`#608`).
- **En las natives**: la posición de las globales se calculaba mal tras una global `string`,
  `long` o `double`, y la native leía otra palabra (`#588`); un módulo con variables
  globales corría entero interpretado en placa (`#562`); y una native que llamaba a BP que
  llamaba a otra native podía matar el proceso al lanzar (`#571`).
- **La S3 se reiniciaba al grabarle un pack** (`#676`), **la C3 y la C6 perdían por el cable
  las líneas de más de 2 KB** (`#545`), y **la P4 no montaba una tarjeta metida después de
  arrancar** (`#566`).
- **Lo que un programa encendía seguía encendido**: el PWM sacando su señal tras el Stop, y
  los canales de PWM y NeoPixel de las ESP32 sin soltar (`#666`).
- **La VM-C recortaba en silencio** el `count` de los builtins de bus (a 64 en I2C, a 256 en
  SPI y UART; `#534`) y la tira NeoPixel, a 256 LEDs (`#536`).
- **`fileExists` de una carpeta** decía `true` en una VM y `false` en la otra (`#630`), y en
  la SD `mkdir` no creaba las carpetas intermedias (`#631`).

### Qué implica al actualizar desde V6

⚠️ **Graba la imagen de V7 y vuelve a grabar los packs.** El código nativo de los packs de
V7 es de la ABI 9 y sus motores van sellados para el bloque de RAM de la placa: los de V6
no cargan —SQLite deja de funcionar— y el `Stdlib.pack` de V6 lleva la biblioteca en el
formato viejo. Lo más limpio, en el panel de Packs: **Formatear zona…** y grabar los de V7.
Una imagen de V6 no ejecuta un `.mod` de V7: lo rechaza.

📌 **En las placas con pantalla** (la C5, la C6, las P4 y la DK2), sin los packs de la GUI
no se pinta nada: graba los de la tabla de arriba, pon el bloque de RAM en el Entorno y,
en la C5 y la C6, las líneas de su pantalla; y reinicia.

📌 **Recompila los `.mod`.** El compilador de V7 escribe MOD9. Las VMs de V7 todavía cargan
un `.mod` de V6, pero sólo corre si no usa clases de la biblioteca: con los 59 programas de
V6 de la cápsula de compatibilidad y la biblioteca de V7, 41 dan lo mismo que en V6 y 18 no
enlazan —los que construyen una clase de la biblioteca o llaman a sus métodos: una
excepción, un envoltorio, `Gui.Screen`, `Adc.Channel`…—, y lo dicen: «lib 'Core' presente
pero no exporta …». Y su código nativo es de otra ABI: la placa lo rechaza y la función
corre interpretada. El IDE recompila en cada Run; lo que hay que mirar es lo que quedó en
`/app` —el programa del autorun, sobre todo—: vuelve a ejecutarlo desde el IDE, o bórralo.

📌 **Lo que rompe fuentes de V6:**
- **`Math.signF(x)` es `Math.sign(x)`**: `signF` ya no existe.
- **Los arrays fijos `tipo[N]`**: a nivel de módulo no compilan (usa `tipo[]`), y uno local
  ya no se pasa a una función que pide `tipo[]`, ni se asigna, ni se devuelve.
- **Leer una variable local sin valor** no compila; en V6 leía lo que hubiera en la pila.
- **Un `switch` sobre un real o un objeto** no compila (en V6 compilaba y no casaba nunca).
- **Un `finally` no puede salir** con `return`, `break` ni `continue`.
- **`step 0`, `Color(1)` —un enum desde un entero— y concatenar a una cadena un array, una
  tupla o el literal `null`** son errores de compilación; y un nombre que empieza por `__`
  es del sistema.
- **La visibilidad se aplica** (`#515`, `#508`): un constructor sin `public` es protegido
  —lo usan su clase y sus subclases, no el resto: en V6 se llamaba desde cualquier sitio—,
  uno `private`, sólo su clase; y un campo o una property `private` ya no se lee desde una
  subclase.
- **Un método `native` de clase que el AOT no puede traducir** es un error (`#519`).

📌 **Lo que cambia de comportamiento sin que el compilador avise:**
- **Con dos núcleos, dos hilos corren a la vez de verdad** (la Pico 2, la Metro, la S3 y la
  P4): lo que compartan sin un `Mutex` puede pisarse más a menudo que antes. `vm.runs=1`
  deja la placa con un hilo a la vez, como en V6.
- **Al acabar un programa —también con el Stop— se destruyen sus objetos y todo vuelve al
  reset**: un PWM se para, una salida deja de serlo, el perro guardián se desarma. Un
  programa que contaba con dejar algo encendido al acabar, ya no lo deja.
- **Una excepción sin atrapar en un hilo** que no es el `main` acaba ese hilo y no el
  programa; al final, el programa sale con 1.
- **Un desbordamiento de pila es un `RuntimeError`** atrapable; sin atrapar, salida 1.
- **Las globales no literales se calculan al cargar**, antes de la función inicializadora,
  y una property de módulo con valor inicial ya no vale `null`.
- **`from "<ruta>"` ya no se usa al ejecutar**: la VM busca cada módulo por su nombre, en el
  orden de arriba.
- **Una librería se sube por su versión**: recompilarla sin subir su `"version"` no la
  vuelve a subir a la placa.
- **`fileExists` de una carpeta** da `false` en todas partes (para las carpetas,
  `IO.dirExists`).
- **Los pines del sistema y los de la pantalla** ya no se pueden pedir: es un error con
  nombre.
- **Tras un fallo fatal, la placa no relanza el autorun** (la RP2350 con dos núcleos y las
  ESP32): se queda esperando al IDE. Tras el perro guardián del módulo `Wdt`, sí.

📌 **Lo que no rompe:** `import Core` escrito sigue compilando; un `.mod` de V6 que no use
clases de la biblioteca se sigue ejecutando; `Pico.*` sigue siendo alias de `Machine`; y
`SQLite=<MB>` sigue valiendo para el bloque de RAM.

### Lo que todavía no

- **La imagen de dos núcleos de la RP2350 es más lenta de lo que debería** (`#665`): en la
  Metro, con los datos de la VM en la PSRAM, un programa de un hilo va un 35 % más lento que
  con la imagen de un núcleo —también con `vm.cores=1`: es la imagen—; en la Pico 2,
  `PrintBench` tarda 1 270 ms frente a 817 con la de un núcleo (el cálculo puro, un 1,8 %
  más); y las cadenas no escalan con dos hilos (×0,83 en la Pico 2, ×0,75 en la Metro). Se
  investiga en V8; mientras, la imagen de un núcleo se compila con
  `-DBPVM_PICO_NUM_CORES=1`.
- **Un error en la placa no dice su línea** (`#582`): el mensaje sí, la línea no —ni en
  código interpretado ni en una native—; la VM Java del PC da la traza entera. V8.
- **Un `.mod` lleva el nativo de una sola familia** (`#584`): llevado a mano a otra, sus
  natives corren interpretadas. El `.mod` multifamilia, en V8.
- **No hay red, ni Bluetooth, ni bajo consumo en placa**: `Net` lanza «Net: sin red en esta
  plataforma» y el micro nunca duerme. Son V8.
- **El depurador** va con un solo `run` en las placas de dos núcleos, no para dentro de una
  native en la placa y no despliega objetos (eso, en V8).
- **Un fallo fatal de la VM tumba la placa** —un HardFault, una pila del sistema
  desbordada—: la RP2350 con dos núcleos y las ESP32 se reinician sin lanzar el autorun
  (arriba), y las STM32 se quedan paradas; contenerlo en el hilo que falló es de V8.
- **La S3 no tiene AOT** (Xtensa): sus natives corren interpretadas.
- **El `Wdt` de las STM32 lanza**: su IWDG no se puede parar por software, y «completo o no
  se implementa». Y las STM32 no tienen ni `Neopixel` ni SD.
- **La C3 tiene menos VM de la que debería**: su bloque ya no llega a los 128 KB de V6 —el
  asignador del sistema no da tanto seguido—; devolvérselos es de V8.
- **`input()`** sigue funcionando sólo en la VM Java.

### Artefactos de la release

**Un solo adjunto**: `BasicPlus-7.0-win.zip` (su `sha256` va en la página de la release y
en `dist/BasicPlus-7.0-win.zip.sha256`). Dentro: `BpIde-7.0.jar` y `bpide.bat`, `bin/` (el
micro simulado, con LVGL, más `SDL2.dll`), `bpstdlib/` (fuentes y `.mod` de la librería),
**`packs/`** con los **cinco packs** de V7 —`Stdlib.pack`, `SQLite.pack`, `Gui.pack`,
`GuiMini.pack` y `DriverSpi.pack`, cada uno, byte a byte, el de su compilación (`#679`)—,
`bpdevices/`, `samples/`, `docs/` (la ayuda que abre el IDE, ES y EN), `bpgenvm-c/`
(cabeceras y fuentes que necesita el AOT) y **`firmware/`** con las **ocho imágenes** de la
tabla de arriba, su `SHA256SUMS.txt`, un `LEEME.md` y las plantillas de `board.json` del
RP2350 en `firmware/boards/`. Cómo instalar cada una, y qué packs lleva cada placa:
**[INSTALAR_FIRMWARE.md](INSTALAR_FIRMWARE.md)**.

---

## v6.0 — septiembre 2026 · «orden en los micros»

En la versión 6 de BasicPlus nos hemos enfocado en el código de los microcontroladores. Más que añadir prestaciones, hemos analizado el código que había y hemos puesto orden. Había código duplicado y código muy similar entre micros: hemos puesto en común todo lo que implementaba una funcionalidad equivalente y, cuando había diferencias, nos hemos quedado con la mejor versión. Al final, medido sobre lo que enlaza cada firmware, algo más del 90 % del código es común a todos los micros; un 7-8 % es común a cada familia —la capa HAL del fabricante y la capa HAL de BasicPlus que la envuelve—; y menos de un 1 % es propio de cada placa, concentrado en el arranque y en sus particularidades. Eso ahorra código, mejora el mantenimiento y hace que añadir un micro nuevo de una de las tres familias que ya implementamos sea relativamente sencillo — aunque hay un trabajo de probar y ajustar en la placa que apenas se refleja en el código.

También hemos añadido dos micros de la familia ESP32, el C3 y el C6 — y con el C6, la primera pantalla por SPI.

Por el camino han entrado algunas cosas que sí se ven: la interfaz gráfica corre en su propio hilo; el modelo de componentes es coherente (cada contenedor conoce a sus hijos y una ventana se puede guardar y volver a cargar como JSON); se puede capturar la pantalla del micro desde el PC; y el IDE atiende el cable mientras el programa corre.

Hemos implementado un sistema semiautomático de pruebas que las acelera y las sistematiza: un agente conduce las placas, ejecuta los programas y compara con el PC.

Y, por último pero no menos importante, hemos corregido bugs y hecho pequeñas mejoras por todo el sistema.

Lo que no cambia es el invariante de siempre: el mismo bytecode, la misma salida
byte a byte en las dos VMs, y todo verificado en placa.

### Placas nuevas

Entran el **ESP32-C3** y el **ESP32-C6**: RISC-V de un solo núcleo, sin FPU. Los dos
tienen **un único conector USB**, y por ahí va el cable del IDE (USB-Serial-JTAG); la
consola del sistema sale por la UART0. Se graban con `esptool --chip esp32c3` /
`esp32c6`, y para entrar en modo de grabación hay que pulsar BOOT+RESET a mano — no
hay truco de DTR.

Con el C6 llega la **primera pantalla por SPI**: la Waveshare ESP32-C6-LCD-1.3, un
ST7789 de 240×240 sin táctil, con rotación 0/90/180/270. Son ya **cuatro pantallas**
(la DK2, las dos P4 y el C6), todas con la misma GUI. El C6 con pantalla corre con un
bloque de VM de 128 KB.

El parque de V6 son **siete imágenes y nueve placas** (la P4 de Waveshare comparte
imagen con el kit de Espressif y elige su panel por el entorno, `display=st7701`):

| imagen | placas que sirve |
|---|---|
| `bpvm_pico.uf2` | Pico 2 · Metro RP2350B |
| `bpvm_esp32_merged.bin` | ESP32-S3 |
| `bpvm_esp32c3_merged.bin` | ESP32-C3 *(nueva)* |
| `bpvm_esp32c6_merged.bin` | ESP32-C6 *(nueva)* |
| `bpvm_esp32p4_merged.bin` | P4 kit · P4 Waveshare |
| `bpvm_stm32_nucleo.bin` | Nucleo U575 |
| `bpvm_stm32_dk2.bin` | Discovery U5G9J |

Y dos cosas de las placas que ya estaban: la **PSRAM del S3** llevaba desde siempre
sin activar —8 MB parados—, y ahora la VM la usa; y los **packs funcionan también en
el S3 y en el C6** (el mapeo de la zona vivía en el directorio del P4 y los demás no
lo llamaban).

Lo que no cambia: el S3 es Xtensa y **no tiene AOT**; y en el C3 y el C6 las funciones
`native` corren **interpretadas**: el compilador sólo tiene el destino RISC-V con FPU del
P4 (`ilp32f`) y el firmware de esos dos (`ilp32`) no carga ningún bloque nativo — un blob
del P4 se ignora. Falta dar de alta el destino `ilp32` (ficha `#502`, V7).

### Memoria por placa

La memoria de la VM la decide ahora **un planificador común** a las cinco familias, con
un número medido por placa en vez de constantes escritas a mano. Lo que sale:

| placa | bloque de la VM |
|---|---|
| Pico 2 | ~357 KB de SRAM |
| Metro RP2350B con `psram=1` en el entorno | 6 MB en PSRAM |
| ESP32-S3 | 160 KB sin PSRAM · **~7 MB** con ella |
| ESP32-C3 | 128 KB |
| ESP32-C6 | 128 KB (imagen con pantalla) |
| ESP32-P4 | ~28 MB en PSRAM |
| Nucleo U575 | 512 KB |
| Discovery U5G9J | **1536 KB** (heredaba los 512 de la Nucleo) |

Dos claves nuevas en el entorno de la placa (botón «Entorno» del IDE): **`stack=N`**, en
KB, reparte el bloque entre pilas y montón (por defecto un 25 % para pilas, con un suelo
de 64 KB — en el C3 y el C6, con 128 KB, es la mitad; si el valor no cabe, el firmware lo
ajusta y lo dice en el log); y **`quantum=N`**, los opcodes que
ejecuta un hilo antes de ceder el turno (1024 por defecto).

Y la **tabla de handles vive dentro del bloque de la VM**: antes salía de otra bolsa y un
programa de objetos pequeños se quedaba sin handles con el heap medio vacío. En la
Pico 2 la batería de V4 pasa de 40/48 a **48/48**.

### Ejecución

Todas las familias corren ahora con **dos hilos de sistema operativo**, iguales en las
cinco: `vm`, que sólo ejecuta opcodes, e `io`, que atiende el cable, la salida, el log y
la pantalla. FreeRTOS también en el STM32. Lo que se nota:

- **El IDE atiende KILL, HELLO y RESET al instante** mientras el programa corre (un
  KILL llega en 3-50 ms según la placa; algo más si hay salida encolada en un puerto
  lento), y **RESET funciona con un RUN vivo** — ya no hace falta `kill` y luego `reset`.
- **La salida va por líneas**: `PrintBench` (2.000 líneas) sale entre **1,9× y 5,3×** más
  rápido según la placa (1,9× en la Pico 2, 3,5× en el P4 y los STM32, 3,6× en el C6, 5,3×
  en el C3), con seis veces menos mensajes por el cable. El cálculo puro no cambia (~1 %).
- **La VM cede el turno al sistema entre quanta**: en el ESP32 un programa ya no
  bloquea el resto del sistema, `stop` vuelve a funcionar en el P4 y dice siempre
  `KILLED (130)`. Y un `sleep` de menos de 10 ms en las ESP32 **duerme de verdad**
  (antes redondeaba a cero ticks y no esperaba).
- El bucle de la GUI **ya no le quita tiempo a los demás hilos** del programa: duerme lo
  que LVGL dice que puede dormir.

Lo que **no** cambia todavía: `GET` y `LS` siguen contestando `BUSY` mientras hay un RUN
en marcha — el hilo `io` ya podría atenderlos, pero falta ordenar el cable; queda para V7.

### El lenguaje y la biblioteca estándar

**`import Core` es obligatorio** cuando un módulo usa un tipo de `Core` — `List`, los
envoltorios (`Integer`, `Long`…), `Comparable`, `Exception`, `RuntimeError`, y por tanto
cualquier `catch`. Antes el compilador lo inyectaba a escondidas, y lo hacía distinto en
la pasada de interfaz que en la completa; la norma sencilla quita el problema de raíz:

```basic
module Medidas
  import Core

  function Main()
    var l: List := List()
    l.add(42)
    print l.getInteger(0)
  end Main
end Medidas
```

Sin el `import`, el compilador dice `tipo 'List' no encontrado`. Los imports no son
transitivos: importar `Collections` no trae `Core`.

Un **`catch e` sin tipo es `catch e: Exception`**, y una excepción **se imprime como su
mensaje**. En V5 un `catch` sin tipo entregaba un valor roto (imprimía memoria de la VM):

```basic
try
  var a: integer[] := [1, 2, 3]
  print a[99]
catch e
  print e          // ALOAD: índice fuera de rango 99 (length=3)
endtry
```

**Las interfaces de módulo se retiran** del manual: BasicPlus no tiene interfaces, ni
de clase ni de módulo; sus demos se han borrado. El compilador todavía acepta `module
interface`, pero es un cabo suelto, no una función.

**`Main` acepta un argumento de ejecución**, con su valor por defecto declarado en el
propio parámetro. El IDE lo pasa con `run <fichero> <argumento>`; los dos CLI, como
segundo posicional. El `autorun` **no pasa ninguno**, a propósito: el valor por defecto
es la configuración de despliegue.

```basic
public function Main(arg: string := "medidas.db")
  print "fichero:", arg
end Main
```

**`Math`** gana `clamp`, `wrap`, `hypot` y `remap` (`wrap` lanza si `hi <= lo`; `remap`
si el rango de entrada es vacío):

```basic
print Math.clamp(12, 0, 10)          // 10
print Math.wrap(370, 0, 360)         // 10
print Math.hypot(3, 4)               // 5
print Math.remap(5, 0, 10, 0, 100)   // 50
```

**El módulo `Pico` pasa a llamarse `Machine`** — no era de la Pico, es de todos los
micros. `Pico` queda como alias que reenvía, así que los programas de V4 y V5 siguen
compilando. Y la identidad son ahora **dos nombres**, para que decida el programa cuál le
interesa:

```basic
print Machine.getMicro(), Machine.getBoard()   // en el PC: host host
```

`getMicro()` es el chip y siempre es real: `rp2350a`, `rp2350b`, `esp32s3`, `esp32c3`,
`esp32c6`, `esp32p4`, `stm32u5`, `host`. `getBoard()` es `generic` salvo que la imagen
conozca la placa. `boardName()` sigue existiendo y es lo mismo que `getBoard()`.

**`Uart` tiene un buffer de recepción de 512 bytes** en las cinco familias, alimentado
por interrupción. `available()` devuelve la cuenta real (en la Pico contestaba 1 ó 0, en
el STM32 −1), ya no se pierden bytes por encima del FIFO del chip (32 B en el RP2350, 8
en el U5), y `read(…, 0)` devuelve al instante lo que haya — en el STM32 colgaba la VM.

**Las fachadas sin driver ya no inventan valores**, en las dos VMs:
- **`Wdt`** está completo en RP2350 y ESP32 (`disable()` desactiva de verdad). En el
  STM32 U5 el IWDG no se puede parar por software, así que allí —y en el PC—
  `Wdt.Timer(…)`, `feed()` y `disable()` lanzan un `RuntimeError` atrapable. «Completo o
  no se implementa.»
- **`Adc`** lanza si la conversión falla (antes −1 y voltios negativos), y el ADC del
  ESP32 ya no falla con una lectura 0.
- **`Neopixel`** sólo tiene driver en el RP2350; en ESP32, STM32 y el PC lanza. La VM
  Java fingía éxito.

**`IO.pathAbsolute` cambia de significado**: es una función pura de la ruta y del
proyecto — lo que empieza por `/` o lleva esquema va tal cual; con proyecto,
`<proyecto>/<ruta>`; sin él, lo relativo **se queda relativo**. No mira el disco ni
normaliza `..`. Antes devolvía la ruta del sistema operativo del PC, que no existe en un
micro.

```basic
print IO.pathAbsolute("datos/x.txt")   // datos/x.txt
```

**`Collections.OwnerList` posee de verdad**: liberar la lista libera sus elementos.
Estaba hueca.

**Código nativo.** El `.mod` pasa a la **versión 7** y lleva el código nativo dentro: ya no
hay un `.mdn` suelto que subir junto al programa. Los **métodos** `native` se compilan
(en V5 sólo avisaban), `double` cruza a C, y una función `native` puede crear arrays de
cualquier ancho y objetos. Las VMs ejecutan `.mod` v6 y v7; v5 se rechaza.

### Interfaz gráfica

**La GUI corre en su propio hilo.** `Gui.start()` arranca el lazo y vuelve; `Gui.stop()`
lo para desde otro hilo o desde un handler; `Gui.join()` espera. `Gui.run()` sigue
bloqueando: es `start()` + `join()`. `Main` puede terminar: el hilo de la GUI mantiene el
programa vivo.

**El modelo de componentes es coherente.** Hay una clase **`Container`** entre
`Component` y los que contienen (`Screen`, `Panel`, `Window`, `TabPage`, `Tabview` y
`Button`), con sus hijos en una `OwnerList`: `childCount()`, `childAt(i)`, `getParent()`.
Todos los constructores con padre piden `parent: Container`, así que meter un `Label` en un
`Checkbox` deja de compilar. `Button(parent, text := "")` es un contenedor (botones con
icono y texto). `delete()` cascadea hijos antes que padre en los dos planos —el modelo
y LVGL—, se da de baja del padre y es idempotente. `Tabview.addTab()` devuelve un
`TabPage`. Y el teclado se engancha con **`Keyboard.setTextarea(ta)`** — `attach` es ahora
el alta de cualquier componente en el árbol.

**Una ventana se guarda como JSON**: `toJson()` y `toJsonText()` en cualquier componente,
recursivo en los contenedores, en el mismo formato de los ficheros `.win`. La ida y
vuelta con `main.win` (cargar → serializar → construir → serializar) da el mismo texto.
Todavía no se serializan `min`/`max`, las opciones de `Dropdown`/`ListBox`, los colores ni
`Chart`/`Table`/`ImageView`.

```basic
var scr: Gui.Screen := Gui.Screen()
var b: Gui.Button := Gui.Button(scr, "Pulsa")
Gui.start()             // el lazo corre en su hilo; esto vuelve
sleep(300)
print scr.childCount()  // 1
print b.toJsonText()    // {"type":"Button","text":"Pulsa"}
Gui.stop()
Gui.join()
```

**Se puede capturar la pantalla del micro**: `Gui.shot(path)` escribe lo que LVGL dibujó a
un fichero `.shot` (RGB565 comprimido; 2,9 KB en el C6, 7 KB en la DK2, 8,9 KB en el P4),
por el mismo camino en el PC y en las cuatro pantallas. Se baja con el `GET` de siempre y
`bpgenvm-c/tools/shot2png.py` lo pasa a PNG. Lanza si no hay pantalla.

```basic
var n: integer := Gui.shot("pantalla.shot")   // bytes escritos
```

**El screen mide lo que mide el panel**: en placa `scr.width`/`scr.height` dicen el
tamaño real (240×240 en el C6, 800×480 en la DK2, 1024×600 en el P4), no los 480×320 del
modelo. El PC sigue en 480×320 salvo que se le diga `--screen=WxH` — así imita a una placa.

Y tres cosas pequeñas: la cola de eventos de la placa avisa cuando descarta uno (antes en
silencio); el bucle de la GUI ya no busca sus handlers por nombre en cada vuelta; y
`App.mainModule()` funciona en placas sin pantalla.

### El IDE

- **`run <fichero> <argumento>`** pasa el argumento a `Main`; las comillas son opcionales.
- Al terminar dice **cuánto tardó**: `exit 0 (OK) en 94 ms` — la ejecución, medida igual
  en el PC y en las placas.
- **`help error`** explica los códigos de salida.
- **El IDE pregunta a la placa por cada dependencia** y sólo sube lo que falta, lo que
  es de versión anterior o lo que tiene otro CRC. Dice de dónde carga cada módulo, y
  avisa si esa copia no es la que el proyecto pondría. La stdlib sigue la misma regla
  (va embebida en el firmware, así que normalmente no viaja nada).
- **El firmware repone `/lib` solo** en cada arranque, en las cinco familias: un módulo
  de la stdlib que falte, que sea más viejo o que tenga otro CRC se vuelve a escribir.
  `/app` no se toca nunca.
- El árbol de ficheros va **por colores** según el tipo.
- El jar pasa a `BpIde-6.0.jar` (`bpide.bat` en la raíz del repo), y el `BpVM.cfg`
  junto al fichero manda sobre el del directorio actual.

Por el cable: el `GET` de ficheros grandes funciona (abría el fichero una vez por cada
256 bytes), el timeout mide inactividad y no tiempo total, el `PUT` crea las carpetas que
falten, las rutas ya no se truncan en silencio (el único tope son 256 caracteres), y una
línea del wire es atómica en las cinco imágenes.

**Los códigos de salida son los mismos en las dos VMs y las cinco familias**:

```
  0  terminó bien               3  fallo interno de la VM
  1  excepción BP no atrapada   4  sin memoria
  2  no se pudo cargar        130  parado con Stop/KILL     131  parado por el depurador
```

Un programa que muere lanzando sale con **1** — antes la VM Java decía 0 y la VM-C 11. El
informe del error va por `stderr` en el PC y en el `errorMessage` del `EXITED` en placa.

### Pruebas

- **`compat.sh`**, el arnés de paridad dual-VM, pasa de 38 a **59 casos**, con la GUI
  dentro por primera vez, un programa que muere lanzando, y las fachadas sin hardware.
  Y ya no tira el `stderr`: una línea `HEAP INCONSISTENTE` rompe la paridad.
- **`bpgenvm-c/tools/tanda.py`** conduce las placas: descubre lo conectado, sube lo que
  falte, ejecuta, compara el `stdout` con el del PC (con `--screen=WxH` para las
  gráficas), baja las capturas y las pasa a PNG, y deja un informe que prueba que
  corrió. La prueba de fuego pasó en Pico 2, C6, P4 y DK2 — y cazó una Metro con
  firmware viejo por su conducta.
- `wire_serie.py` habla con una placa por el puerto serie sin el IDE (`put`, `run`,
  `get`, `log`, `info`).
- Los programas de `samples/` (326) se han pasado por el compilador de V6: seis no
  compilaban y se han arreglado; los ocho de bases de datos necesitan su pack.

### Bugs de V5 corregidos

- **El GC de la VM Java descarrilaba** desde julio con cualquier cadena vacía: objetos
  vivos barridos y un `use-after-free` mucho después. El arnés de paridad lo tapaba.
- **SQLite estaba rota** (las pruebas finales de V6 lo cazaron): el puente nativo del
  pack era de una ABI anterior a la de la VM de V6 y se rechazaba al cargar, y en el P4
  además el firmware llevaba desde final de agosto sin cargar ningún bloque nativo. Pack
  regenerado (`packs/SQLite.pack`, hay que **volver a grabarlo** en la placa) y firmware
  del P4 arreglado: `SqlDemo` da en el P4 la misma salida que en el PC.
- **Guardar en un `word[]`** en la VM-C ponía a cero el elemento siguiente, y en el
  último escribía fuera del array. Todas las placas.
- **Un `catch` sin tipo** volcaba memoria de la VM por pantalla (arriba).
- **Un programa que moría lanzando salía con `exit 0`** en la VM Java y con 11 en la
  VM-C; y las dos escribían cosas distintas en `stdout`.
- **`Uart` perdía bytes** en la Pico y en el STM32, y `available()` no contaba (arriba).
- **La Pico moría muda**: un HardFault o un `panic` van ahora al log post-mortem con
  `CFSR`/`PC`/`LR`; y `malloc` ya no mata la placa, con lo que el error de memoria
  atrapable de V4 funciona también en RP2350.
- Seis opcodes de globales `byte`/`short` que faltaban en la VM-C; y el opcode
  desconocido dice en qué módulo y con qué bytes.
- **La VM Java no podía tocar un bus**: los builtins de `I2c`/`Spi`/`Uart` se quedaron
  fuera del cambio a handles de 8 bytes.
- Las seis fachadas sin hardware del PC escribían textos distintos en cada VM (trece
  textos, y `Uart.write` salía desordenado).
- **El cargador de `.win` perdía la `y`** de los widgets con `align`: los formularios con
  `y` en el fichero se mueven ahora.
- **`SQLite.bp` y `Orm.bp` no compilaban** con el compilador actual (les faltaba `import
  Core`), así que el pack de SQLite no se podía regenerar. Seis samples publicados,
  igual.
- **`README.es.md` estaba destruido** —un párrafo repetido 12.000 veces— y así se publicó
  en v5.0.
- El IDE resubía toda la stdlib la mitad de las veces: el CRC del wire salía negativo.
- El SMP opcional de la VM (`--smp=2`) ya no se cuelga; sigue sin ser el defecto.

### Qué implica al actualizar desde V5

⚠️ **Reflashea el firmware y borra `/app` de la placa.** La stdlib va embebida en la
imagen y el firmware repone `/lib` solo; lo que sobreviva en `/app` de otra versión **tapa**
a lo nuevo, y el síntoma apunta a otro sitio. La instrucción de V5 decía «borra `/lib` y
`/app`»: `/lib` ya no hace falta.

📌 **Lo que rompe fuentes de V5:**
- **`import Core`** hay que escribirlo en todo módulo que use `List`, los envoltorios,
  `Comparable`, `Exception`, `RuntimeError` o un `catch`. Es el cambio que más ficheros
  toca; el compilador lo dice.
- **`parent: Container`** en todos los constructores de la GUI: un programa que declaró el
  padre con tipo estático `Component`, o que metía hijos en una hoja, deja de compilar.
- **`Keyboard.attach(ta)` es `Keyboard.setTextarea(ta)`**. Con `attach` ahora se resuelve
  al alta en el árbol y no compila.
- **`Tabview.addTab()` devuelve `TabPage`**, no `Component`.

📌 **Lo que cambia de comportamiento sin que el compilador avise:**
- **`print e`** imprime el mensaje de la excepción (antes, basura de memoria).
- **El screen mide lo que mide el panel** en placa: un programa que asumía 480×320 cambia
  de geometría, y un `.win` guardado desde `toJson()` lleva el tamaño del panel — no es
  portable entre pantallas.
- **Los handlers de la GUI se despachan dentro de `run()`**, no después.
- **Un programa que muere lanzando sale con 1**, no con 0 ni con 11. En el repo nadie
  miraba el 11.
- **`IO.pathAbsolute`** ya no devuelve la ruta del sistema operativo: lo relativo se queda
  relativo.
- **`Wdt` en el STM32 lanza**; antes `disable()` lo reprogramaba a ~131 s y la placa se
  reseteaba sola. **`Adc`** lanza si falla; **`Neopixel`** lanza donde no hay driver.
- **`OwnerList` libera sus elementos**: una segunda referencia a uno de ellos muere con la
  lista (falla por handle, no corrompe).

📌 **Lo que no rompe:** `Pico.*` sigue compilando (alias de `Machine`); los `.mod` v6 se
siguen ejecutando; `Gui.run()` sigue bloqueando.

### Lo que todavía no

- **`GET` y `LS` durante un RUN** contestan `BUSY`. V7.
- **`listDir` e `input()` sólo funcionan en la VM Java**: en la VM-C —el PC y las placas—
  lanzan `RuntimeError` («builtin no soportado»).
- **Las bases de datos siguen pidiendo placa** con `bpgenvm-c` a secas (la VM-C de línea de
  comandos). El micro simulado que viene con el IDE **sí** lleva SQLite dentro: `SqlDemo`
  da en él la misma salida que en el P4.
- **El S3 no tiene AOT** (Xtensa); **el C3 y el C6 interpretan sus `native`** hasta que
  haya un destino RISC-V sin FPU.
- **No hay bajo consumo**: el micro nunca duerme. Y en el STM32 la tarea ociosa no llega a
  correr.
- **No hay ficheros como objetos** (`open`/`seek`/`close`): `readFile` trae el fichero
  entero al heap, así que un fichero mayor que la RAM no se puede leer.
- **`Adc` no tiene driver en el STM32**; **`Pulse` en el C3 cuenta 0** (el chip no tiene
  contador); el breadcrumb de `Machine` (`setMark`…) sólo funciona en el STM32;
  **`Neopixel`** sólo en el RP2350; **la SD no llega al STM32**; la rotación no está en la
  DK2.
- **Los campos protegidos no cruzan módulos**: un descendiente en otro módulo no los ve
  (usa un getter). Y sólo el primer constructor de una clase cruza la frontera del módulo.
- **Un módulo de la stdlib olvidado en `/app` tapa al de `/lib` sin aviso.**
- **La VM sigue en un solo núcleo**, también en los micros que tienen dos (S3 y P4
  arrancan con el segundo apagado: se midió que no costaba nada). Los dos núcleos para
  ejecutar BP quedan fuera del plan de versiones.

### Artefactos de la release

**Un solo adjunto**: `BasicPlus-6.0-win.zip` (~25 MB; su `sha256` va en la página de la
release y en `dist/BasicPlus-6.0-win.zip.sha256`). Dentro: `BpIde-6.0.jar` y `bpide.bat`,
`bin/` (el micro simulado con LVGL **y SQLite**, más `SDL2.dll`), `bpstdlib/` (fuentes y
`.mod` de la librería), `packs/` (`SQLite.pack`, `Stdlib.pack`), `bpdevices/`, `samples/`,
`docs/` (la ayuda que abre el IDE, ES y EN), `bpgenvm-c/` (cabeceras y fuentes que necesita
el AOT) y **`firmware/`** con las **siete imágenes** de la tabla de arriba, su
`SHA256SUMS.txt` y las plantillas de `board.json` del RP2350 en `firmware/boards/`. Cómo
instalar cada una: **[INSTALAR_FIRMWARE.md](INSTALAR_FIRMWARE.md)**.

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
  *(V6: el micro simulado del IDE ya lleva SQLite dentro: los programas con BD se prueban
  en el PC sin placa.)*
- **`listDir` no está en la VM-C**, o sea que un programa puede listar un
  directorio en el PC pero no en la placa. Es el único verbo de fichero que
  falta. *(V6: sigue así; `listDir` e `input()` sólo funcionan en la VM Java, y en la
  VM-C lanzan `RuntimeError`.)*
- **exFAT no está soportado**: formatea las tarjetas en FAT32.
- **La tarjeta SD no llega al STM32** todavía.
- **El ESP32-S3 no tiene AOT**: sus funciones `native` corren interpretadas — el AOT
  emite ARM y RISC-V, no Xtensa. El IDE lo avisa al compilar y el programa funciona
  igual, sólo que sin acelerar. El P4 sí acelera.
- **El S3 tampoco expone packs.** El resto de placas sí. *(V6: ya sí — el mapeo de la
  zona no se llamaba en el S3; corregido, y el C6 también los tiene.)*
- **La sustitución entre interfaces de módulo** (un impl de `LogApiV2` donde se pide
  `LogApi`) no funciona todavía. *(V6: las interfaces de módulo se retiran del lenguaje;
  deja de ser un pendiente.)*

### Qué implica al actualizar desde V4

⚠️ **Un paso, y no es opcional: borra `/lib` y `/app` de la placa.** El firmware repone
lo suyo al arrancar.

La stdlib de V5 cambió —`Object` sustituyó a `any` como raíz, y `Comparable` ganó las
conversiones—, así que un módulo de V4 que sobreviva en la placa **tapa al nuevo**. El
síntoma es un error al ejecutar del estilo:

```
exit 11 (lib 'Core' presente pero no exporta 'Core.__cls_new_List'; ¿version vieja?)
```

*(V6: los códigos de salida se unificaron; ese fallo de enlace sale hoy con `exit 1` y el
mismo mensaje.)*

📌 **Por qué las dos carpetas y no sólo `/lib`**: el IDE deja en `/app` las dependencias
del programa, y lo que hay ahí **tiene preferencia** sobre `/lib`. Limpiar sólo una no
basta — nos costó media hora encontrarlo durante las pruebas.

📌 **Y por qué la instrucción es la misma para todas las placas** aunque no la necesiten
todas: cada familia se comporta distinto —el RP2350 avisa del desfase al arrancar, el
STM32 rehace `/lib` solo en cada arranque, y el ESP32 ni lo toca ni avisa— y la regla de
borrar las dos carpetas es correcta en las tres sin tener que saber en cuál estás.

También hay que **reflashear el firmware**: la stdlib va embebida en la imagen.

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
