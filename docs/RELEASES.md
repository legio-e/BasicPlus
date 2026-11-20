# Notas de versión — BasicPlus

> Borrador del cuerpo de la *release* de GitHub. La etiqueta propuesta es
> `v8.0` (v1 cerró en `v1.0`, v2 en `v2.0`, v3 en `v3.0`/`v3.0.1`, v4 en `v4.0`, v5 en
> `v5.0`, v6 en `v6.0`, v7 en `v7.0`). Ajusta versión/fecha al publicar.

---

## v8.0 — noviembre 2026

En esta versión hemos añadido el soporte de la WiFi y el Bluetooth a la familia de micros ESP32: a las placas con
PSRAM, la S3, la C5 y la P4 de Waveshare, que lleva la radio en su ESP32-C6. La placa se conecta sola a la red al
arrancar, y el IDE trabaja con ella sin cable: sube los ficheros, graba los packs y ejecuta los programas. Del
Bluetooth hemos hecho, de momento, lo que más falta hacía en una placa: el teclado y el ratón. Las dos radios están
apagadas de normal, y se apagan solas tras diez minutos sin uso.

También hemos añadido un nuevo lenguaje, **JC**, una mezcla de Java y C# con algunas limitaciones, para facilitar el
desarrollo de aplicaciones a los programadores que trabajan habitualmente con esos lenguajes. No hemos pretendido hacer
ni un Java ni un C#, y las librerías no son las de esos lenguajes sino las nuestras. Por dentro es el mismo lenguaje que
BasicPlus, con el mismo compilador y el mismo bytecode, y un programa se traduce de uno a otro con una orden del IDE.

Con el fin de verificar el compilador y la máquina virtual, hemos aprovechado la batería de pruebas de C# de Mono y
hemos intentado traducirlas, compilarlas y ejecutarlas. De las 2 322 pruebas funcionan 848; el resto no se pueden
traducir por las limitaciones del lenguaje. En general el resultado ha sido muy positivo: han aparecido algunos bugs,
que hemos corregido, y hemos aprovechado para ampliar un poco el lenguaje, tanto BP como JC: algunos operadores (`*=`,
`/=`, `mod=`… y el ternario `iif`), las interfaces —una clase puede implementar varias: la herencia múltiple de tipos,
no de código— y los genéricos, principalmente. Esas 848 pruebas se quedan como una prueba más de los dos lenguajes, de
caja negra.

También hemos aprovechado para arreglar algunas cosas que teníamos pendientes de la versión 7, y pequeñas cosas que
han ido apareciendo durante el desarrollo de esta versión.

Hemos optimizado y mejorado las operaciones con cadenas, que eran lentas: en la Pico 2, por ejemplo, recorrer una
cadena de 2 KB ha pasado de 535 a 20 ms.

Y hemos puesto a prueba la fiabilidad: cuatro placas, 48 horas seguidas con un programa que lo toca todo, sin un error
ni un reinicio. El objetivo de la prueba era sobre todo que no hubiera una merma de recursos, como la memoria, que con
el tiempo pudiera hacer que un micro se bloquease o se reiniciase. La prueba encontró dos fallos de verdad, que ya están
arreglados. Una página nueva de la documentación cuenta todas las pruebas que pasa BasicPlus.

Para terminar, hemos añadido una imagen para el **ESP32-P4X**, la nueva revisión del P4, que corrige errores del
silicio. Es casi idéntica a la del ESP32-P4, pero una imagen del P4 no corre en el P4X ni al revés, así que tenemos una
imagen para cada uno de los micros. Con su reloj, 400 MHz frente a 360, los programas tardan entre un 8 y un 10 %
menos.

Poco a poco el proyecto va entrando en su madurez, así que los cambios ya no son tan drásticos, y nos centramos más en
ampliar las librerías y el soporte de más micros.

Lo que no cambia es el invariante de siempre: el mismo bytecode, la misma salida
byte a byte en las dos VMs, y todo verificado en placa.

### La WiFi: el IDE sin cable, y la red desde el programa

**La red en placa** (hito H3), en la P4 de Waveshare, la S3 y la C5. En V7, `Net` sólo funcionaba en el PC; ahora la
placa se conecta sola al arrancar y coge su IP —por DHCP o fija—, y la usan el IDE y los programas. La C3 y la C6 no la
llevan: sin PSRAM, sólo por enlazarla perderían de 33 a 88 KB de RAM, aunque no se encendiera nunca. Tampoco las
STM32; y la Pico 2 W, aplazada a otra versión.

**Se configura en dos ficheros de `/sys`**, los mismos en todas las placas. `/sys/wifi.json` lleva la red —`ssid`,
`password`, `country`—, `autoStart` (encenderla al arrancar) y `autoOff` (los segundos sin actividad para apagarse
sola: 600 por defecto; 0, nunca); `/sys/net.json`, si hace falta, el `hostname` para el DHCP o la IP fija (`ip`,
`mask`, `gateway`, `dns`). El primero lo escribe el botón **WiFi** del IDE. La clave `wifi=1` del Entorno la enciende
al arrancar, y `wifi=0` la deja apagada diga lo que diga el fichero. **La radio está apagada de normal**: se enciende
cuando hace falta y se apaga sola tras diez minutos sin actividad (una conexión abierta, del programa o del IDE, es
actividad). La clave de la red va en claro en el fichero: la seguridad, de momento, no.

En la P4 de Waveshare la WiFi va por la ESP32-C6 que la placa lleva al lado: su Entorno lo dice con `radio=c6`, y esos pines pasan
a ser del sistema (sin la línea, no se toca ningún pin y la placa no tiene WiFi). Esa C6 necesita un firmware nuevo
(abajo, «Qué implica»). La S3 y la C5 llevan la radio en su chip.

**El IDE por WiFi**: en el explorador, el backend **Placa (red)**, con la IP de la placa, o **Buscar en la red**, que
pregunta a toda la red qué placas hay. Todo va como por el cable —el árbol de ficheros, subir y bajar, los packs, Run
con su salida, el Stop, el log y la memoria del `INFO`—, y los dos cables funcionan a la vez: el IDE puede entrar por
la red en una placa que ya corre un programa —el autorun, por ejemplo— y pararlo. Si la placa desaparece de la red, el
IDE lo dice a los pocos segundos y vuelve a desconectado (`#803`). En la P4, subir 1 MB tarda 16 s y bajarlo, 1,5 (por
la UART, unos 95 s cada cosa); grabar `Gui`, `SQLite` y `Stdlib` —1,5 MB, sellándolos—, 14 s, frente a más de dos
minutos.

**Desde un programa**, el módulo nuevo `Air` enciende y apaga: `Air.Wifi`, con `turnOn()` —que espera a tener IP,
hasta 20 s, sin parar los demás hilos—, `turnOff()`, `isConnected()` e `isAvailable()`. Y `Net`, que en V7 sólo
funcionaba en el PC, abre conexiones TCP como cliente en las placas, con DNS y plazos, hasta ocho a la vez. Al acabar
el programa la WiFi vuelve a lo que diga la configuración: uno que la apaga no deja al IDE fuera, y uno que la enciende
no la deja gastando. En el PC, `turnOn()` da siempre `true`.

```basic
import Air
import Net

var wifi: Air.Wifi := Air.Wifi()
if wifi.turnOn() then                 // enciende y espera a tener IP
  var c: Net.Tcp := Net.Tcp()
  if c.connect("192.168.1.50", 8080, 3000) then
    c.sendStr("hola")
    c.close()
  endif
endif
wifi.turnOff()
```

Lo que no hay todavía: servir conexiones (`listen`/`accept`), UDP, TLS, la placa como punto de acceso, la hora por NTP
y la lista de redes desde un programa. Todo, en la referencia §14.19.

### El Bluetooth: el teclado y el ratón

**El teclado y el ratón Bluetooth LE** (hito H2, `#697`), en la P4 de Waveshare, la C5 y la S3 —en la P4, por la misma
C6 que la WiFi—. Se emparejan una vez y, desde entonces, vuelven solos por su dirección cada vez que se enciende el Bluetooth.
Lo que mandan llega a un teclado y a un ratón del sistema, que juntan todos los aparatos conectados, y de ahí, a la
GUI y al programa:

- **En un programa con pantalla, sin escribir nada**: el ratón mueve una flecha y hace clic, la rueda desplaza las
  listas, y el teclado escribe en el campo que tiene el foco; Tab pasa al siguiente. Los programas de V7 funcionan así
  tal cual. Lo hace la GUI de la P4 y la de la C5 (la S3 no tiene pantalla).
- **Desde el programa**, dos módulos nuevos: `Keyboards.Keyboard` —el evento `onKey` por cada pulsación, o preguntar
  con `hasKey()` y `readKey()`; la tecla, con su código HID (con nombre en `Keyboards.Key`) y su carácter— y
  `Mice.Mouse` —`onPress`, `onMove` y `onWheel`, y los botones en `Mice.Button`—. La pantalla tiene además
  `scr.onKey`. La distribución de teclado, de momento, la americana.
- **El Bluetooth**, en `Air.Bluetooth`: `turnOn`, `scan(ms)`, `connect`, `disconnect`, `forget` y `listDevices`, con
  los aparatos en `Air.BtDevice`. Lo recordado va en `/sys/bluetooth.json` —con `autoStart` y `autoOff`, como la
  WiFi— y las claves de los emparejamientos, aparte, en `/sys/bluetooth.keys`. La clave `bt=1` del Entorno lo enciende
  al arrancar, y se apaga solo tras diez minutos sin uso (una tecla o el ratón cuentan como uso).

```basic
import Air
var bt: Air.Bluetooth := Air.Bluetooth()
bt.turnOn()
var vistos: List := bt.scan(10000)          // 10 s: los aparatos en modo de emparejar
var d: Air.BtDevice := Air.BtDevice(vistos.get(0))
if bt.connect(d) then                       // la primera vez empareja y lo recuerda
  print d.getName() + " conectado"
endif
```

**En el PC**, un Bluetooth simulado, igual en las dos VMs y en el micro simulado del IDE, con dos aparatos de prueba,
«Teclado simulado» y «Ratón simulado»: conectados, el teclado y el ratón del PC llegan al programa por la ventana de su
GUI, como llegarían en la placa; sin conectar, el programa no recibe teclas, como una placa sin teclado.

Dos cosas de los aparatos, no de la placa: un ratón dormido del todo puede no despertarse al moverlo —con un clic, sí,
y vuelve en unos segundos (`#820`)—; y algunos cambian de dirección al emparejarse con otro equipo, y entonces hay que
emparejarlos otra vez. El programa de configuración del Bluetooth, escrito en BasicPlus, queda para V9 (`#819`):
mientras, se empareja con unas líneas como las de arriba (referencia §14.19 a §14.21).

### JC: BasicPlus con la ropa de Java y C#

**JC** (hito H1, `#698`) es BasicPlus con llaves, punto y coma y el tipo delante del nombre. No es otro lenguaje: el
mismo compilador lee los `.bp` y los `.jc` y los lleva al mismo árbol, y el mismo programa, escrito en BP o en JC, da
el mismo `.mod` byte a byte (sólo cambia el `.dbg`, que cita su fuente). Las VMs no se enteran: un programa en JC corre
igual en el PC y en las placas, se depura igual y usa las mismas librerías.

```
module Hola
{
   function void main(string arg = "mundo")
   {
      println("hola, ", arg);
   }
}
```

- **El compilador** lee un `.jc` por su extensión, también en un proyecto (su main, en `<main>.jc`; si están el `.bp`
  y el `.jc`, lo dice) y en el AOT. Y traduce: `--escribir-jc` y `--escribir-bp` llevan un fuente de un lenguaje al
  otro, y `--comprobar-jc` comprueba que la ida y la vuelta dan lo mismo.
- **El IDE** abre, guarda, colorea y pliega los `.jc`, los compila, los ejecuta y los depura, en el PC y en la placa;
  y **Edit → Traducir a JC** y **Traducir a BP** abren la traducción en una pestaña nueva. Los comentarios no viajan al
  traducir.
- **`print` y `println`**: en JC, `print(a, b)` no salta de línea y `println(a, b)` sí (`#703`). Y en BP, un `print`
  que acaba en `;` deja la línea abierta (`#700`).
- Un módulo en JC importa los de BP —la biblioteca estándar está escrita en BP—, y se mezclan en un proyecto.

El volumen nuevo, **`docs/jc.html`** («El lenguaje JC»), cuenta la sintaxis entera, cada cosa en los dos lenguajes, y
lo que JC no tiene porque BP tampoco lo tiene (las lambdas, los `struct`, los genéricos propios con `<T>`…).

**Y C# como banco de pruebas** (hitos H6, H7 y H8). Con un traductor de C# a JC —`compat/csharp/cs2jc`, en el
repositorio; necesita el SDK de .NET— se pasaron por JC las 2322 pruebas válidas del compilador de C# de Mono: **848
funcionan** —traducidas, compilan y dan lo mismo en las dos VMs— y desde el 7-nov son la prueba de caja negra de BP y
de JC: una que deje de funcionar es una regresión. Lo que pedían y tenía sentido para BasicPlus entró en los dos
lenguajes (H7.1 a H7.5, abajo: los compuestos y el ternario, los genéricos, las interfaces, los parámetros `var` y los
errores con su tipo). Lo demás de C# —los delegados, los `struct` con estado, los operadores propios, `?.`, los
patrones…— no se promete para V9 (Eduardo: *«Hay muchas cosas de C# que no quiero migrar, hacen al lenguaje
complicado y en mi opinión aportan poco»*).

### El sonido

**El módulo `Sound`** (hito H4, `#708`) reproduce un WAV —PCM de 16 u 8 bits, mono o estéreo— siempre en paralelo:
`play` vuelve en seguida y el programa sigue mientras suena. Con `stop`, `isPlaying`, el volumen (`setVolume` y
`getVolume`, de 0 a 100; 85 por defecto) y el progreso (`getPosition` y `getLength`); un sonido a la vez.

```basic
import Sound
Sound.play("/sd/aviso.wav")   // vuelve en seguida: suena mientras el programa sigue
```

Suena en la **P4 de Waveshare**, por su códec ES8311 y su amplificador, con la línea `audio` del Entorno
(`audio=es8311,i2c:0,mclk:13,bclk:12,ws:10,dout:9,pa:53`); el amplificador sólo se enciende mientras suena. **Y en el
PC**, en las dos VMs, unos 20 dB más bajo, para que el 85 no deje sordo a nadie. En una placa sin sonido,
`isAvailable()` da `false` y `play` lanza con el motivo, el mismo texto en todas. La C5 lleva el mismo códec, pero su
altavoz no entra en el conector de la placa: a V9. El MP3, fuera.

### Placas

El parque crece en una placa: la **ESP32-P4X**, la revisión v3.x de la P4, con su imagen (abajo). **Nueve imágenes y
once placas**: las de V7 (la tabla de v7.0, abajo) y el kit de la P4X. Lo que cambia es lo que lleva cada una:

| placa | WiFi | Bluetooth (teclado y ratón) | sonido |
|---|---|---|---|
| ESP32-P4 de Waveshare | sí, por su C6 (`radio=c6`) | sí, por la misma C6 | sí (`audio=`) |
| ESP32-P4 de Espressif (el kit) | sin probar | sin probar | sin probar |
| ESP32-P4X de Espressif (el kit), nueva | sin probar | sin probar | sin probar |
| ESP32-S3 | sí | sí | — |
| ESP32-C5 | sí | sí | en V9 |
| ESP32-C3 y C6 | no (sin PSRAM) | no | — |
| Pico 2 y Metro | no (la Pico 2 W, aplazada) | no | — |
| Nucleo y DK2 | no | no | — |

Y en cada una:

- **La ESP32-P4X, nueva** (`#846`): la revisión v3.x de la P4, que corrige errores del silicio de la v1.x y con el
  tiempo la sustituirá. El IDF no deja que una imagen sirva para las dos revisiones, así que tiene la suya,
  `bpvm_esp32p4x_merged.bin`, **del mismo código**: cambian la revisión —con ella, lo que pone el IDF: la CPU a 400 MHz
  y sus arreglos para la v3— y el cable del wire, que va por el USB-Serial-JTAG del chip, porque su kit no lleva puente
  USB-UART. Probada en el kit ESP32-P4X-Function-EV, con la tanda de V8 y la GUI en su panel de 7", táctil
  incluido. Frente a la P4 de Waveshare, los programas de `samples/benchmarks` tardan entre un 8 y un 10 % menos, lo
  del reloj; y, con el cable por USB, imprimir 2 000 líneas cuesta 0,3 s en vez de 20,5 por la UART.
- **La familia ESP32, al ESP-IDF 6.1** (`#707`): las cinco imágenes de entonces, con la tanda de V7 sin un cambio (0
  de 250 pruebas); la de la P4X, nueva, también con él. En la S3, la C5 y la P4, el perro guardián de las interrupciones pasa de 300 a 800 ms.
- **La S3 y la C5**, para la WiFi, pasan de 1 a 2 MB de aplicación: la tabla de particiones nueva obliga a borrar la
  flash una vez (abajo, «Qué implica»).
- **La S3 dice la frecuencia a la que va** —160 MHz; decía 240— y se puede subir a 240 con la clave `cpu.mhz` del
  Entorno o con `Machine.setCpuFreqMHz` (`#784`): a 240, el cálculo tarda un tercio menos. Y sus cachés, al máximo
  —32 KB de instrucciones y 64 de datos— (`#737`): las operaciones con cadenas tardan del 18 al 27 % menos, a cambio
  de 48 KB de RAM interna.
- **La P4**: su caché L2, de 128 a 256 KB (`#737`): un programa que mueve muchos datos en la PSRAM ya no echa al
  intérprete de la caché —interpretar después de leer 128 KB costaba un 97 % más; ahora, un 1 %—, a cambio de 128 KB
  de RAM interna.
- **Las ESP32**: `Machine.cpuFreqHz()` y el `INFO` dicen la frecuencia de verdad, y `setCpuFreqMHz` funciona
  —la S3 y la C5, 80, 160 o 240 MHz; la C3, 80 o 160; la C6, 80, 120 o 160; la P4, 360 fija, y la P4X, 400— (`#784`);
  `Machine.tempC()` lee el sensor del chip, que en V7 daba 0, y el `INFO` lo enseña también en las STM32 (`#826`); y
  el motivo del reinicio ya no sale «unknown» tras un reinicio por el USB (`#709`).
- **La C3 y la C6, con mucho más heap** (`#740`): las pilas de los hilos BP salen ahora del heap, y la región fija
  de pilas desaparece. La C3 pasa de 48 a 96 KB de heap, y la C6, de 61 a 112; y también la Nucleo, de 384 a 496, y la
  DK2, de 768 a 1008. El log de la C3 y la C6, de 8 a 4 KB.
- **La Pico 2 y la Metro**: la imagen única lleva ahora dos firmwares. Con `psram=1`, en una RP2350B —la Metro—
  arranca M, con lo más usado de la VM y del sistema en la SRAM: frente a P, las operaciones con cadenas tardan del 25
  al 31 % menos, y el GC, un 33 % menos. La Pico 2 sigue con P, y la Metro vuelve a P con `imagen=P` en el Entorno; el
  `INFO` dice cuál corre. **La Metro ya llena sus 6 MB de heap**: en V7, hacia los 4,7 MB, el GC se quedaba sin
  memoria para su mapa y el programa moría con «heap descarrilado» (`#787`).
- **Las STM32**: tras un fallo fatal se reinician solas (`#839`, abajo); y copiar, rellenar y comparar memoria, que
  su biblioteca de C hacía byte a byte, va mucho más deprisa en todo el firmware (`#785`: en la Nucleo, copiar 2 KB,
  de 87,5 a 13,2 µs; un `gc()`, de 2,13 a 0,94 ms).

Las nueve placas del lote final —las diez de V7 menos la P4 del kit— han pasado la tanda de V8 con la imagen que se
publica (abajo, «Pruebas»); el kit de la P4X, con la suya, el 19-nov; y la P4 del kit, el 20-nov, con la de la P4 de
Waveshare: **las once**.

### La robustez en el campo

Un dispositivo puede pasar días encendido sin que nadie lo mire. V8 ha mirado qué pasa entonces.

**La prueba de resistencia**: 48 horas seguidas, del 15 al 17 de noviembre, en cuatro placas a la vez —la Pico 2 y
la Metro (la misma imagen; la diferencia, la PSRAM), la P4 y la C6—, con el autorun y un programa que en cada vuelta
calcula, reserva memoria, escribe ficheros, lanza hilos y, en las que la tienen, pinta la pantalla. **Las cuatro, sin
un error ni un reinicio**: 61 360 vueltas entre todas, la memoria libre tras el GC, quieta, y los tiempos de cada fase,
los del principio. Encontró dos fallos de verdad, ya arreglados: el `Mutex` de cada `Collections.Map` no se soltaba
hasta el final del RUN, y un programa que crea mapas en un bucle acababa sin mutex (`#831`: la Pico 2 empezó a fallar
a las ~257 vueltas); y un número guardado en una global que valía justo la dirección de un objeto lo retenía horas,
partiendo el heap (`#833`). El programa y su guion, en `compat/resistencia/`, para repetirla.

**La tabla de la robustez** (`V8_IDEAS` §E15): dieciséis situaciones —el programa que se queda sin memoria, el error
sin atrapar, el bucle sin fin, el pánico, el corte de corriente…—, qué hace hoy la placa y qué deja registrado. De ella
salieron cinco arreglos pequeños, hechos y vistos en las placas:

- **Con el log apagado** —`log=0`, lo de serie—, los avisos urgentes de la VM llegan al log igual: una excepción sin
  atrapar, con su informe entero; un interbloqueo; sin memoria (`#837`). En el campo, de un programa que muere ya queda
  algo. Los avisos de una referencia caducada, los tres primeros.
- **Las ESP32 apuntan en el log la causa de cada reinicio** («boot: causa del reset = …»), como la Pico; y la línea
  del autorun que no arranca tras un fallo fatal se escribe aunque el log esté apagado (`#838`).
- **Las STM32, tras un fallo fatal** —un HardFault, la pila o el heap del sistema—, dejan su informe, se reinician a
  los ~2 s por su perro guardián y arrancan sin autorun, como las demás (`#839`). En V7 se quedaban paradas hasta que
  alguien las desenchufara.
- **Un RESET con un programa que no para** —un hilo metido en una función de C que no vuelve— se sirve a los 2 s: la
  placa se reinicia por software y conserva su log; y tras un KILL, la placa sigue leyendo el cable (`#840`). En V7,
  la Pico y las STM32 sólo volvían con las manos.
- **`Wdt` con dos runs** (la P4 y la S3) ya no reinicia una placa sana: un `feed` desde cualquier hilo cuenta
  (`#836`).

Y de paso: una carga que fallaba a medias ya no se deja memoria del sistema en cada intento (`#835`); con dos runs, se
acabó un «No space in heap» falso (`#762`, `#829`); y un motor nativo que no carga ya no llena el log, repitiéndolo
en cada `INFO` (`#798`).

Lo que no es sencillo —contener el fallo en el run que lo tiene, el MPU, un contador de reinicios contra el bucle del
autorun, un perro guardián para el sistema y el informe del pánico de las ESP32— queda para V9 (abajo, «Lo que todavía no»).

### Las cadenas, mucho más rápidas

**Las cadenas** (hito H5) eran lo más lento del lenguaje: recorrerlas por índice, compararlas o construir un `Map`
con muchas claves tenía un coste cuadrático. Ya no:

- **Dos caminos**: el rápido, si la cadena no tiene acentos —un carácter es un byte—, y el lento si los tiene, que
  recuerda por dónde iba (dos cursores por hilo, `#731`). Se mira una vez, al crear la cadena. También en los
  literales (`#732`) y en las natives (`#733`).
- **Comparar, en C**: `==`, `<`, `>`… y `Str.compare`, con una sola comparación por bytes, que en UTF-8 es el orden de
  los caracteres (`#713`).
- **bpstr**, una biblioteca de cadenas propia en C, la misma en ARM, RISC-V y Xtensa y verificada contra un oráculo,
  para buscar, reemplazar, pasar a mayúsculas o recortar (`#730`): `indexOf` en 2 KB, de 5 a 9 veces más rápido.
- **El `Map`** desplaza sus elementos de una vez al meter y al quitar.
- **El GC y la reserva**: los huecos libres, en 48 listas por tamaño (`#718`, `#715`, abajo).

Medido en la Pico 2: recorrer una cadena de 2 KB, de 535 a 20 ms; una de 4000 caracteres con acentos, de 1 992 a
44 ms; un `==` entre dos de 2 KB iguales, de 22,8 ms a 68 µs; `Str.compare` de dos de 4000 caracteres con acentos,
cinco veces, de 7,3 s a 1 ms; y un JSON de 8 KB, de 16,7 a 2,5 s. En la P4, un `Map` de 2000 claves en desorden, de
56,7 a 1,5 s. (Medido el 21-oct, al hacerlo.)

Y **un string es un invariante**: nace válido y no cambia.

- `move` ya no escribe dentro de una cadena —lo que no debía cambiar cambiaba, y lo veían todas sus referencias—:
  lanza «move: dst es de solo lectura» (`#714`). Para un texto que cambia, `StringBuilder`.
- El texto de fuera que no es UTF-8 válido —un fichero, el puerto serie, la red— se descodifica al nacer la cadena:
  `fromBytes`, `readFile`, `readLine`… lanzan `EncodingError`, o, con su indicador (`fromBytes(b, true)`,
  `readFile(ruta, true)`, `readLine(true)`), ponen «�» en lo que no vale (`#711`). En V7 las dos VMs contaban distinto
  un texto mal formado.
- `Net.Tcp.recvStr` espera el resto de un carácter partido entre dos paquetes; y el texto de un pack —SQLite— llega
  entero y descodificado, con `getStr(i)` estricto y `getStr(i, ignorarError)` (`#724`).

### El lenguaje y la biblioteca estándar

**Las interfaces** (H7.3, `#756`). Una clase hereda de una clase e implementa las interfaces que quiera: la herencia
múltiple de tipos, no de código. Una interfaz lleva métodos —sólo su firma— y properties, y puede heredar de otras; lo
que implementa un miembro de una interfaz lo dice con `@implement{ … }`, y `@override{}`, opcional, marca lo que
redefine del padre. Se convierte con su nombre, como una clase, y vale con `instanceof`.

```basic
public interface Poligono
  function lados(): integer
end Poligono

public class Cuadrado implements Poligono
  @implement{ Poligono }
  public function lados(): integer
    return 4
  end lados
end Cuadrado
```

**Los genéricos, una orden al compilador** (`#793`, `#749`). `List<Sensor>` es la `List` de siempre con una orden
para el compilador: le dice qué guarda, comprueba lo que entra y escribe él la conversión de lo que sale. La VM no se
entera. Lo mismo `Collections.Map<string, Tarea>` y las clases propias que dicen su tipo base con `@element{tipo}`; y
`for s in sensores` recorre la lista. **Y `x[i]` sobre un objeto** (`#794`) es `x.get(i)`; `x[i] := v`, `x.set(i, v)`
(en un `Map`, `put`); y `x.pin[i]`, `x.getPin(i)`. No hay nada que declarar.

```basic
var sensores: List<Sensor> := List()
sensores.add(Sensor(4))
sensores.add("hola")            // error: 'List<Sensor>' guarda 'Sensor', no 'string' (add)
print sensores[0].valor()       // sale como un Sensor: sin cast
```

**Los errores de la VM, con su tipo** (H7.5), como en Java: `ArithmeticError`, `NullReferenceError`, `IndexError`,
`CastError` e `IOError` —con su hija `EncodingError`—, todas hijas de `RuntimeError`. `catch e: RuntimeError` las
sigue atrapando todas, con el mensaje de siempre, y con un `catch` de su clase delante se tratan aparte; también lo
que lanza una `native`. Lo demás —los pines, los buses, la red, el heap lleno, la pila desbordada— sigue siendo
`RuntimeError`.

```basic
try
  media := total / n
catch e: ArithmeticError
  print "sin datos: " + e.msg     // sin datos: División por cero
endtry
```

**Una variable `string` puede valer `null`** (`#774`), como en C#: la variable no referencia ninguna cadena; una
cadena, en cambio, nunca es `null`. Un campo, una property o una global `string` sin valor nace `""`. Con un `null`,
escribir y concatenar dan lo mismo que con `""`, `== null` es `true`, y lo que pide una cadena —`strlen`, `upper`,
abrir un fichero…— lanza «referencia nula: strlen de un string null». Una constante, en cambio, no puede ser `null`
(`#777`).

**Los compuestos y el ternario** (H7.1, `#747`): `*=`, `/=`, `&=`, `|=`, `mod=`, `xor=`, `shl=` y `shr=`, con los tipos
como en C# —sobre un `byte` o un `int16` dan la vuelta—; e `iif(c, a, b)`, que sólo calcula la rama que toca
(`iif(p != null, p.nombre, "nadie")` no falla con `p` nulo). **Los parámetros `var`** (H7.4): una función recibe una
property de un objeto, y la lee y la escribe —el `ref` de C#, sólo con properties—:

```basic
function suma(var n: integer, m: integer)
  n += m                          // lee y escribe la property que le pasaron
end suma
```

con `suma(c::valor, 5)`. Y además:

- `print x;` —con `;` al final— deja la línea abierta, y el siguiente `print` sigue en ella (`#700`).
- Un bucle puede llevar nombre, y `break` y `continue` lo nombran para salir de varios a la vez (`do fuera` …
  `break fuera`); el `continue` de un `do … loop c` va a la condición, como en C (`#699`).
- `T[n]` crea un array de cualquier tipo con su tamaño: `Punto[10]`, `string[n + 1]`.
- `&`, `|` y `xor` valen entre dos `boolean`, y calculan los dos lados.
- Un enum se convierte en su entero con `integer(c)` (`#754`), y `Enum.getName(c)` da su nombre (`#778`).
- Las sobrecargas: gana la más específica, como en Java (`#772`), y una que no se ve desde donde se llama no compite
  (`#796`). Lo protegido sobre otro objeto, con la regla de Java y C# (`#760`).
- Un evento se compara con `null` dentro de su clase, para saber si tiene suscriptor: `if onClick != null then …`
  (`#771`).
- Los datos de un módulo —sus literales, globales y constantes— pueden llegar a 64 KB, y si no caben, el error dice
  cuál (`#768`).
- `eol()` da el fin de línea de la plataforma, y un `\n` dentro de una cadena es siempre LF, en las dos VMs (`#755`).

**Los overlays** (hito H9, `#786`): un programa carga a otro con sus módulos, lo ejecuta y lo descarga, con la clase
`Overlays.Overlay`. Lo que se usa poco —la configuración, exportar o importar datos— va aparte y sólo ocupa memoria
mientras corre, como los overlays de MS-DOS con la barrera de los 640 K. `tryExe` atrapa los errores del programa
cargado y dice cuál fue. Probado en el PC, la Metro, la P4 y la Nucleo; lo que todavía no hace, en `PENDIENTES`
(«Los overlays»).

```basic
import Overlays
function Main()
  var ov: Overlays.Overlay := Overlays.Overlay()
  ov.load("/app/Demo.mod")            // un .mod o un pack
  if ov.tryExe("su argumento") then
    print "acabó bien"
  else
    print "falló: " + ov.getError()
  endif
  ov.unload()
end Main
```

**La biblioteca estándar:**

- Cinco módulos nuevos: `Air`, `Keyboards`, `Mice`, `Sound` y `Overlays` (arriba).
- `Json.parseJson` sigue tras un error, como un compilador, y los da todos —diez como mucho— en un `AggregateError`,
  cada uno un `JsonError` con su línea y su columna (`#725`). Quien atrapaba `RuntimeError` los sigue atrapando.
- `List.get`, `set` y `remove` miran el tamaño de la lista, no su capacidad: fuera de la lista, siempre un error
  (`#795`).
- `parseInt` y `parseFloat`, con una regla, la misma en las dos VMs: un texto que no es un número limpio es un
  `RuntimeError` atrapable —«parseInt: '12abc' no es un número entero»— (`#716`); y `charCodeAt` fuera de rango lanza
  también en la VM-C (`#717`).
- `/data`, la carpeta para los datos de los programas —cada uno en la suya, `/data/<proyecto>/`—, en las placas, en
  el PC y en el micro simulado; el IDE no la toca nunca (`#816`). Y las rutas relativas, documentadas: mejor `/data`
  (`#823`).
- `IO.rmdir` sólo borra una carpeta vacía, e `IO.removeFile`, sólo un fichero, en todas partes (`#824`).
- `Machine.setCpuFreqMHz` en las ESP32, con la clave `cpu.mhz` (`#784`); `Machine.tempC()` en las ESP32 (`#826`); y
  en la VM-Java, `Machine.uptimeMs()` cuenta desde el arranque, como en la VM-C (`#827`).

### El código nativo

- **Una `native` da lo mismo que interpretada cuando un entero da la vuelta** (`#790`): el AOT escribe las cuentas
  sin signo, y el compilador de C ya no se salta una comparación como `x + 1 > x`.
- **Dividir por cero dentro de una `native` de enteros lanza**, como interpretada, y se atrapa dentro y fuera; y en el
  PC, el mínimo entre -1 ya no tumba la VM-C (`#791`).
- **Convertir una referencia dentro de una `native`** (`Perro(a)`) es un error de compilación: el AOT construía un
  `Perro` en vez de convertir, sin decir nada (`#757`). Se hace fuera de la native.
- Una `native` recorre una cadena por índice en un tiempo lineal (`#733`) y lanza los errores con su tipo (H7.5).
  La ABI no cambia: los `.mdn` siguen en la ABI 9.
- No se llama por una interfaz desde una `native`: se llama desde BP y se le pasa lo que haga falta.

### Interfaz gráfica

- **El teclado y el ratón**, en la GUI de la P4 y la C5 (arriba, el Bluetooth): la flecha, los clics, la rueda en las
  listas y el teclado en el campo con el foco; y `scr.onKey`. En el PC, con el «Ratón simulado» conectado, el ratón
  deja de hacer de táctil y hace de ratón.
- **En el PC, la ventana gira con la pantalla** (`#813`): con `Gui.setRotation(90)`, la ventana de la VM-C y la del
  micro simulado salían pálidas y con rayas; ahora miden lo que la pantalla del modelo, como la de la VM-Java.
- **En la C5, con la WiFi y el Bluetooth encendidos, la pantalla dejaba de refrescarse**: el DMA del SPI lee ya de la
  PSRAM (`#818`; también en la S3 y la P4). Y el driver de la pantalla podía quedar grabado encima del final del motor
  de LVGL, con la pantalla a puntos y la placa caída (`#817`): los packs de la GUI están rehechos.
- Al parar un programa de GUI, el destructor de `Gui.Button` ya no lanza, y no quedan objetos sin destruir (`#695`).
  En la VM-Java, un clic de verdad en un `Gui.Button` llega al programa (`#811`).
- La GUI recorta su texto por caracteres, no por bytes (`#711`).

### El IDE

- **La placa por la red**: **Placa (red)**, **Buscar en la red** y el botón **WiFi**, que escribe `/sys/wifi.json`
  (arriba). Si la placa desaparece, el IDE lo dice y vuelve a desconectado (`#803`).
- **JC**: abrir, colorear, plegar, compilar, ejecutar y depurar los `.jc`, y **Edit → Traducir a JC / Traducir a BP**
  (arriba).
- **Un error dice dónde** (`#582`): un error sin atrapar en la placa trae su cadena de llamadas, y el IDE la traduce a
  fichero y línea con los `.dbg` («en Sensor.bp:42», «llamado desde …»); con la VM local, lo mismo (`#788`). En V7,
  en la placa, sólo salía el mensaje. Dentro de una `native`, la línea es la de quien la llamó.
- **Un programa a la vez** (`#822`): con uno en marcha, Run, Run Window, Debug y Run on Device no lanzan otro, y lo
  dicen; el Stop para también la VM-C de Run Window, y al cerrar el IDE se para lo que tenga en marcha en el PC. Y el
  Stop y el Run esperan la respuesta de la placa y dicen qué pasó: que para el programa, que no había ninguno, que no
  contesta o por qué rechaza el RUN (`#694`).
- **La flash del PC** (`#808`): las dos VMs del PC tienen la suya, una carpeta —por defecto
  `%USERPROFILE%\.basicplus\flash`, u otra con `--flash-dir=`— que hace de `/` de la placa. Con la VM local, el
  programa corre en `<flash>/app/<proyecto>`, y lo que escribe se queda al acabar.
- **El micro simulado** trae la WiFi y el Bluetooth simulados, con el teclado y el ratón de prueba.
- El jar pasa a `BpIde-8.0.jar`.

### La ejecución y la memoria

- **Las pilas de los hilos, del heap** (`#740`): cada hilo pide la suya al heap al arrancar —2 KB, o lo que diga
  `Thread(stackSize)`— y la devuelve al acabar; la región fija de pilas desaparece, y las placas sin PSRAM ganan heap
  (arriba, «Placas»). Y la VM mira la pila antes de cada instrucción: lo que en V7 podía escribir fuera de su pila sin
  aviso —una expresión muy anidada— es ahora un «desbordamiento de pila». La clave `stack=` del Entorno pide sólo la
  pila del `main`.
- **Reservar con muchos huecos** (`#718`, `#715`): los huecos libres van en 48 listas por tamaño, y el GC ya no rehace
  su lista con un coste cuadrático. En V7, con 4000 cadenas vivas, cada `gc()` tardaba 3,6 s en la P4; con 4000
  huecos, diez `gc()` pasan en la Metro de 11,9 s a 0,64, y en la Pico 2, de 1,2 a 0,19.
- **Sin memoria del sistema, el error dice cuál falta y qué hacer** (`#828`): «no hay memoria del sistema (malloc)
  para las tablas de las clases (en una placa sin PSRAM, un vmheap= menor en el ENV le deja mas)», o «no hay sitio en
  el bloque de la VM». En la Pico 2, un programa con `Gui` y `Json` necesita un `vmheap=` menor.
- **Una referencia caducada** —a un objeto ya soltado— que llega a un builtin da `NullReferenceError` «referencia a
  objeto eliminado (use-after-free)», como al usarla directamente (`#805`, `#807`); y el `throw` de una excepción
  `null` es un `NullReferenceError` que se atrapa (`#806`).
- **El GC sólo se fía de una referencia de verdad**, con su contador (`#799`, `#833`): un número que valga justo la
  dirección de un objeto ya no lo retiene, y un objeto que murió sin cerrar se destruye en el primer `gc()`.
- **`sleep(0)` cede el turno** también en la VM-C, como `yield()` (`#770`).
- **El turno de un hilo se cuenta en instrucciones** (`#801`): uno que pasa el rato en builtins largos —leer o escribir
  ficheros— hace esperar a los demás; `quantum=256` en el Entorno lo reparte mejor (manual §14.2).
- Un array enorme ya no desborda el cálculo de sus bytes: «No space in heap» (`#712`). Y el argumento del RUN, hasta
  127 bytes, con un error si no cabe (`#764`).
- **El rendimiento frente a V7**, medido el 31-oct con una herramienta nueva que compara en la placa con una versión
  publicada, `bpgenvm-c/tools/rendimiento_versiones.py` (`#743`): en la Metro, `AllocBench` tarda un 38 % menos y
  `PrintBench`, la mitad; y el cálculo puro, entre un 2 y un 7 % más (`fib(28)`, un 4 % en la Metro y en la P4); la
  causa probable, mirar la pila antes de cada instrucción (`#740`), sin medir aparte.

### Pruebas

- **Una página nueva, `docs/pruebas.html`** («Cómo se prueba BasicPlus»): todas las pruebas que pasa BasicPlus, qué
  comprueba cada una, cuántos casos tiene, un fallo real que cazó y cómo correrlas.
- **La red de paridad dual-VM** pasa de 149 casos a **217**; los mismos compilados en MOD8, el formato anterior a
  MOD9, **209**, y 8 que usan lo que sólo tiene el formato nuevo; y con dos workers, **217**. Las VMs de V8 siguen
  ejecutando los **59** `.mod` de V6 de la cápsula, con la salida de V6 salvo uno, que cambia a propósito (`#774`,
  abajo, «Lo que no rompe»). (La red, en verde el 19-nov, con el compilador que se publica.)
- **117 pruebas de C** de la VM en la red (en V7, 102) —el depurador, las natives contra su versión interpretada, los
  ficheros y la SD, el GC, los hilos, los packs, la memoria justa—, las de lo más delicado con su **sabotaje**: un
  interruptor que deshace el arreglo, para comprobar que la prueba se pone en rojo. Y el micro simulado, **15** casos;
  la sombra, **15**; la norma de versiones, **9**.
- **El compilador**: **402** pruebas propias, y **66** de la VM-Java; **39** programas de `samples/errores/` que
  tienen que fallar, con **151** comprobaciones de lo que dice cada error (en V7, 19 y 50); y los **415** samples
  compilan con el compilador de V8 (en V7, 399). Y un `.mod` compilado con V7 carga y corre en las dos VMs de V8.
- **Los ejemplos de la documentación**: los **377** fragmentos de BP y los **78** módulos de JC de 29 páginas compilan,
  y una línea que el manual enseña como un error lo da (`#832`); y ningún enlace interno de la ayuda está roto (el
  20-nov).
- **JC**: el corpus, pasado a JC, da la misma salida (`check-jc`, **215**, el 19-nov); y las
  **848** pruebas de Mono que funcionan, la caja negra de los dos lenguajes (repetido el 20-nov con el compilador que
  se publica: las mismas 848, y fallan las mismas 6).
- **En las placas**: la tanda de V8, **24** pruebas —la prueba de fuego y lo nuevo de V8— en las **once** placas —las
  nueve del lote final, la P4X y la P4 del kit—, con las imágenes que se publican y la salida comparada con la del
  PC: igual, salvo lo que difiere a propósito —`lastModified`, que el sistema de ficheros de las placas no tiene, y la
  GUI en las placas sin pantalla—. Y la prueba de resistencia: **48 horas** en cuatro placas, **61 360** vueltas, sin un
  error ni un reinicio (arriba).

### Bugs de V7 corregidos

- **Un programa que crea mapas en un bucle acababa sin `Mutex`**: el de cada `Collections.Map` y cada `SyncList` no
  se soltaba hasta el final del RUN, y tras horas daba «mutex.lock: id inválido -1» (`#831`). **Un número en una
  global podía retener un objeto**, horas o hasta el final del RUN, partiendo el heap (`#833`).
- **Con dos runs, un «No space in heap» falso** cuando otro hilo recogía la basura a la vez (`#762`, `#829`); y **una
  carga que fallaba a medias se dejaba memoria del sistema** en cada intento, hasta reiniciar (`#835`).
- **El GC rehacía la lista de huecos con un coste cuadrático**, y una reserva que no cabía en ningún hueco la recorría
  entera (`#715`, `#718`). **La Metro no llegaba a llenar sus 6 MB de heap** (`#787`). **Un array enorme pisaba otros
  objetos**: `newIntArray(2^30 + 1)` desbordaba el cálculo de sus bytes (`#712`).
- **Las cadenas**: `move` escribía dentro de una cadena, y lo veían todas sus copias (`#714`); `<`, `<=`, `>` y `>=`
  entre dos cadenas compilaban y daban basura, distinta en cada VM (`#713`); un texto que no era UTF-8 válido se
  contaba distinto en las dos VMs (`#711`); `charCodeAt` fuera de rango daba 0 en la VM-C (`#717`); `parseInt("12abc")`
  daba 12 en la VM-C y en la VM-Java mataba el hilo sin que el `try` lo viera (`#716`); una cadena guardada en un
  `Object` no se podía imprimir (`#748`); y el texto que devolvía SQLite se cortaba en 512 bytes, a veces a medio
  carácter (`#724`).
- **El compilador**: el `continue` de un `do … loop c` volvía arriba sin mirar la condición —y en una `native` sí la
  miraba: el mismo bucle daba otra cosa— (`#699`); un `if`, `while` o `for` de una línea al final de un bloque no
  compilaba (`#701`), ni un `raise` dentro del `set` de una property (`#702`); `a[f()] += 5` llamaba dos veces a `f()`
  (`#746`); `b += 1` con un `byte` a 255 guardaba 256 (`#747`); `super.prop` iba a la property de la hija (`#744`); un
  método no podía construir una clase declarada más abajo en el mismo módulo (`#751`); un método privado llamado
  sobre `null` corría con `this` nulo (`#752`); `super()` en una clase sin padre paraba el compilador sin decir por qué
  (`#753`); desde fuera se podía llamar a una sobrecarga `private`, porque la visibilidad se miraba en la primera
  (`#796`); y el mismo programa podía dar otro `.mod` en otra compilación (`#704`).
- **Las natives**: una cuenta que daba la vuelta daba otro resultado que interpretada (`#790`); dividir por cero no
  lanzaba (`#791`); y convertir una referencia construía un objeto, sin un error (`#757`).
- **Las dos VMs no hacían lo mismo**: con un `\n` dentro de lo que se imprime, en Windows (`#755`); en tres caminos de
  error —los textos de los ficheros, una cadena eliminada, un `Mutex` que no vale— (`#805`); con un `throw` de una
  excepción `null`, que no se atrapaba (`#806`); con `sleep(0)`, que en la VM-C no cedía (`#770`); con `IO.rmdir` e
  `IO.removeFile`, que en la VM-Java y en la SD borraban también lo que no les tocaba (`#824`); con
  `Machine.uptimeMs()`, que en la VM-Java era la hora del PC cortada a 32 bits (`#827`); con el argumento del RUN, que
  la VM-C cortaba a 127 bytes en silencio (`#764`); y en Windows, donde la VM-C del PC no hablaba UTF-8 en los nombres
  de fichero ni en el argumento (`#763`).
- **`List.get` y `set` miraban la capacidad, no el tamaño**: fuera de la lista no siempre había error, y `SyncList` se
  dejaba el cerrojo cogido si lo de dentro lanzaba (`#795`). **Una referencia caducada pasada a un builtin** no daba
  error, y podía leer el contenido de otro objeto (`#807`). **`Net.tcpClose`**, suelto, cambiaba las variables locales
  de antes (`#705`).
- **Las placas**: `Wdt` con dos runs reiniciaba una P4 o una S3 sanas (`#836`); las STM32 se quedaban paradas tras un
  fallo fatal (`#839`); un RESET con un programa que no paraba no se servía, y tras un KILL la placa dejaba de leer el
  cable (`#840`); con el log apagado no quedaba nada de un programa que moría (`#837`), ni de por qué se había
  reiniciado una ESP32 (`#838`); la S3 decía ir a 240 MHz e iba a 160 (`#784`); `Machine.tempC()` daba 0 en las ESP32
  (`#826`); el motivo del reinicio salía «unknown» tras un reinicio por el USB (`#709`); y la STM32 copiaba la memoria
  byte a byte (`#785`).
- **La GUI**: al parar un programa, el destructor de `Gui.Button` lanzaba «referencia nula» y quedaban objetos sin
  destruir (`#695`); y en el PC, la ventana con la pantalla girada salía pálida y con rayas (`#813`).
- **El IDE**: decía «KILL enviado» sin saber si había llegado, y un RUN que la placa rechazaba lo dejaba esperando
  (`#694`); una línea del cable que se estancaba 300 ms se tiraba en silencio —así se perdió un KILL— (`#693`); dos
  Run seguidos con la VM local dejaban la primera dando vueltas, y ni el Stop ni cerrar el IDE la paraban (`#822`); y
  con la VM local, un error daba la posición y no la línea (`#788`). **En la placa, un error no decía su línea**
  (`#582`): ahora la dice, salvo dentro de una `native`.

### Qué implica al actualizar desde V7

⚠️ **Graba la imagen de V8 en cada placa antes que nada, y después la stdlib y los packs de V8.** El `.mod` sigue en
MOD9 y el código nativo en la ABI 9, pero lo compilado con V8 —tus programas y la stdlib del `Stdlib.pack`— usa
funciones de la VM que una imagen de V7 no tiene (comparar cadenas, crearlas de solo lectura, descodificar el texto de
fuera, lanzar los errores con su tipo…) y, si hay interfaces, opcodes nuevos. Graba los packs de V8: el `Stdlib.pack`
va con su imagen, los de la GUI están rehechos (`#817`) y el de SQLite también (`#724`). Si la imagen nueva mueve la
zona de packs o el bloque de RAM de los motores, un motor dice «realojado para OTRA direccion» hasta que se regraba
(`#741`). Lo más limpio, como en V7, en el panel de Packs: **Formatear zona…** y grabar los de V8. En la RP2350, graba
siempre el `bpvm_pico.uf2` entero: lleva los dos firmwares, P y M.

⚠️ **La S3 y la C5: la flash, borrada entera, una vez.** Su aplicación pasa de 1 a 2 MB para la WiFi, y la tabla de
particiones nueva pide borrar la flash: se pierden el Entorno, el sistema de ficheros y los packs de la placa. Baja
antes lo que quieras conservar, graba la imagen fusionada de V8 y vuelve a aprovisionarla: las particiones (la C5 se
reparte sola), el Entorno, los packs y tus programas.

📌 **La P4 de Waveshare, para la WiFi y el Bluetooth**: su C6 necesita el firmware ESP-Hosted 3.0.9 —el de fábrica no
vale para la WiFi—; en V8 se grabó por el conector P1 de la placa, con un adaptador USB-serie (`V8_IDEAS` §E3, «Paso
1»). Va en el ZIP, `firmware/p4_c6_esp_hosted_3.0.9_merged.bin`, y cómo grabarlo —los pines del conector P1, que
hay que soldar, y la copia de la de fábrica antes—, en `INSTALAR_FIRMWARE`. Y en su Entorno, `radio=c6` —sin esa
línea, ni WiFi ni Bluetooth— y, para el sonido, la línea `audio=…`.

📌 **Recompila los `.mod`.** El formato no cambia (MOD9), así que un `.mod` de V7 carga (comprobado en el PC con tres
programas compilados con V7, en las dos VMs de V8 y su stdlib), pero con la VM de V8 debajo: un campo, una property o una global `string` sin valor es para
él `null`, y se para con «referencia nula: strlen de un string null» si llega a un builtin (`#774`; recompilado, nace
`""`); y lo que lee texto de fuera ya es estricto (`#711`), recompilado o no. El IDE recompila en cada Run; lo que
hay que mirar es lo que quedó en `/app` —el programa del autorun, sobre todo—. Y recompila las natives para tener los
arreglos del AOT (`#790`, `#791`): un `.mdn` ya compilado sigue como estaba.

📌 **Revisa el Entorno.**
- **`stack=` pide ahora la pila del `main`**, no la región de todos los hilos (`#740`): una línea de V7 le quita heap
  a la placa —en la Pico 2, `stack=90` le quitaba 74 KB—; quítala si no la necesitas.
- **En una placa sin PSRAM**, un programa que no arranca por «no hay memoria del sistema (malloc)…» se arregla con un
  `vmheap=` menor (`#828`).
- **Lo nuevo**: `wifi=` y `bt=` (encender la radio al arrancar), `radio=c6` y `audio=` (la P4), `cpu.mhz=` (las
  ESP32) e `imagen=P` (la RP2350).

📌 **Lo que rompe fuentes de V7** (deja de compilar, y el compilador dice por qué):
- **Lo protegido de otra clase, leído por una referencia del padre** (`#760`): desde `B`, lo protegido se usa sobre un
  `B` o una hija suya; sobre un objeto que sólo es un `A`, por su getter.
- **Una sobrecarga `private` o protegida, llamada desde fuera** (`#796`).
- **Algunas llamadas con sobrecargas**: con la regla de Java, lo que antes era ambiguo compila, y alguna llamada que
  antes elegía una pasa a ser ambigua; el error dice cómo elegir (`#772`).
- **Una conversión de referencia dentro de una `native`** (`Perro(a)`): se hace fuera (`#757`).
- **Una función que se llame `iif`**: es el ternario (`#747`).
- **Un valor por defecto del argumento de `main` de más de 127 bytes** (`#764`).

📌 **Lo que cambia de comportamiento sin que el compilador avise:**
- **`print x;`**, con `;` al final, ya no salta de línea (`#700`).
- **El `continue` de un `do … loop c` va a la condición**: si ya no se cumple, el bucle acaba (`#699`).
- **`move` sobre una cadena** lanza «move: dst es de solo lectura» (`#714`).
- **`parseInt` y `parseFloat`** lanzan con un texto que no es un número limpio —`"12abc"`, `""`, un hexadecimal—
  (`#716`); y **`charCodeAt`** fuera de rango lanza también en la placa (`#717`).
- **El texto de fuera que no es UTF-8 válido** —`fromBytes`, `readFile`, `readLine`, `getStr` de SQLite— lanza
  `EncodingError` (`#711`, `#724`): un programa que lee ficheros en Latin-1 necesita el indicador, `readFile(ruta,
  true)`.
- **`Json.parseJson`** lanza un `AggregateError` con todos los errores, y un entero que no cabe en 32 bits ya es un
  error (`#725`).
- **Un `string` sin valor es `""`**: `campo == ""` da `true` (en V7, `false`; `#773`, `#774`); y `f(null)`, con una
  sobrecarga para `string`, va a ella (`#772`).
- **Los compuestos sobre un `byte` o un `int16` dan la vuelta**: `b += 1` con 255 da 0 (en V7, 256; `#747`); y el
  destino de un compuesto se calcula una vez (`#746`).
- **`super.prop` va a la property del padre** (`#744`).
- **`List.get`, `set` y `remove`**, fuera del tamaño de la lista, lanzan (`#795`).
- **Una referencia caducada que llega a un builtin** lanza `NullReferenceError` (`#807`), y **el `throw` de un `null`**,
  también, y se atrapa (`#806`).
- **`IO.rmdir` y `IO.removeFile`** sólo borran lo suyo: una carpeta vacía y un fichero (`#824`).
- **`sleep(0)`** cede el turno también en la VM-C (`#770`).
- **En la S3, `Machine.cpuFreqHz()` dice 160 MHz**, lo que va de verdad (`#784`).
- **Las STM32, tras un fallo fatal, se reinician y arrancan sin autorun** (`#839`); y **un RESET con un programa que
  no para reinicia la placa a los 2 s** (`#840`).
- **En el IDE, un programa a la vez** (`#822`).
- **En el PC**: una ruta absoluta va a la flash del PC (`%USERPROFILE%\.basicplus\flash`), no a la raíz del disco
  (`#808`); en la VM-Java, sin el «Teclado simulado» conectado, la GUI no recibe el teclado del PC (H2); y en Windows,
  un `\n` dentro de lo que se imprime es LF en las dos VMs (`#755`).

📌 **Lo que no rompe:** `catch e: RuntimeError` sigue atrapando todos los errores de la VM, que ahora son sus hijas;
el formato del `.mod` sigue siendo MOD9 y la ABI del nativo, la 9 (un `.mdn` de V7 está hecho para cargar en la
imagen de V8, pero no lo hemos probado: mejor recompilarlo, que además trae los arreglos del AOT); las VMs de V8 siguen ejecutando los 59 `.mod` de V6 de la cápsula, con su stdlib de entonces, y sólo
uno cambia, por `#774` (`GuiWinJson`, que mide un `string` sin valor de su `Gui` de V6); e `import Core`, escrito,
sigue compilando.

### Lo que todavía no

- **El bajo consumo** (`#490`) pasa a V9 entero: el micro nunca duerme, y un programa a batería no puede ahorrar hoy.
  Lo que sí hay: las radios, apagadas de normal y apagadas solas sin uso.
- **La WiFi**, sólo en la P4 de Waveshare, la S3 y la C5 (en los kits de Espressif, el de la P4 y el de la P4X, sin
  probar); no en la
  C3, la C6, las STM32 ni la Pico 2 W. Y no hay servidor (`listen`/`accept`), ni UDP, ni TLS, ni la placa como punto
  de acceso, ni la hora por NTP. Un módulo de radio externo para las demás placas, y un STM32 con WiFi, de V9 o más
  adelante.
- **El Bluetooth**: el teclado y el ratón BLE, en la P4, la C5 y la S3, con la distribución americana; el programa de
  configuración, a V9 (`#819`); y en la S3, mientras se conectan los dos aparatos, la RAM interna va justa (`#821`, a
  V9).
- **El sonido**: WAV, en la P4 y el PC; la C5, a V9 (`#708`); el MP3, fuera.
- **Los streams y los dispositivos** (`#723`), el tema de V9: los `Reader` y `Writer`, y con ellos, rehacer las clases
  de los dispositivos, la PIO del RP2350 programable desde BP, más UART en las ESP32 y el DMA de `Uart`. Mientras, con
  un run, un `Uart.read` que espera para a los demás hilos: el rodeo, `available()` y `sleep`.
- **Que el fallo fatal de un run no tumbe la placa** (la tabla de la robustez): contenerlo en el run, el MPU de la M33,
  un contador de reinicios contra el bucle del autorun, un perro guardián del sistema que vigile a `io` y el informe del pánico
  de las ESP32 en el log, a V9.
- **La imagen de dos núcleos de la RP2350 sigue siendo más lenta con un solo hilo** (`#665`, a V9): en la Pico 2,
  `PrintBench` tarda 1 321 ms frente a 745 con la de un núcleo, y el cálculo (`fib(26)`), un 7,7 % más; en la Metro,
  el cálculo, un 3,5 % (en V7, un 35 %). Con dos hilos, la de dos núcleos gana: ×1,68 en la Pico 2 y ×1,75 en la Metro.
- **Dentro de una `native`, un error no dice su línea**, sino la de quien la llamó (`#582`): para verla, quítale
  `native` y ejecútala interpretada.
- **El `.mod` multifamilia** (`#584`), a V9, y sólo dentro de un proyecto que diga qué familias lleva.
- **El depurador no despliega los objetos** (`M5`, a V9), y en las placas de dos núcleos va con un solo run.
- **JC no tiene lo que BP no tiene** —las lambdas, los delegados, los `struct`, los genéricos propios con `<T>`, los
  operadores propios, `?.`, los patrones, los arrays de varias dimensiones—; los comentarios no viajan al traducir;
  un módulo escrito en JC que otro importa hay que compilarlo antes; y los mensajes del compilador hablan en BP
  (`integer`, `boolean`).
- **La C3 va justa de memoria**, con sus 96 KB de heap: optimizarla, en V9. Y en las ESP32 con PSRAM, pasar a la PSRAM
  lo que la VM aún tiene en la RAM interna (`#710`), también.
- **La S3 no tiene AOT** (Xtensa): sus natives corren interpretadas.
- **El `Wdt` de las STM32 lanza**: su IWDG no se puede parar por software. Y las STM32 siguen sin `Neopixel` ni SD.
- **`input()`** sigue funcionando sólo en la VM Java.
- **Para V9, también**: los conjuntos, como en Pascal; los recursos grandes, fuera del bloque de datos del módulo
  (`#782`), y quitar su tope de 64 KB (`#781`); la zona de packs del PC en un fichero (`#809`); una interfaz del IDE
  para la IA (`#728`); el simulador con disfraces y la ventana del micro simulado.
- **La P4 se colgó una vez en `GuiShot`**, en el lote final (`#825`): no se repite —200 parejas y 500 capturas
  seguidas—, y queda apuntado por si vuelve.

### Artefactos de la release

**Un solo adjunto**: `BasicPlus-8.0-win.zip` (su `sha256` va en la página de la release y en
`dist/BasicPlus-8.0-win.zip.sha256`). Dentro: `BpIde-8.0.jar` y `bpide.bat`, `bin/` (el micro simulado,
con LVGL, más `SDL2.dll`), `bpstdlib/` (fuentes y `.mod` de la librería, con los módulos nuevos), **`packs/`** con los
**cinco packs** de V8 —`Stdlib.pack`, `SQLite.pack`, `Gui.pack`, `GuiMini.pack` y `DriverSpi.pack`, cada uno, byte a
byte, el de su compilación (`#679`)—, `bpdevices/`,
`samples/` (y el de JC, en `samplesJC/`), `docs/` (la ayuda que abre el IDE, ES y EN, con los dos
volúmenes nuevos, `jc.html` y `pruebas.html`, en los dos idiomas), `bpgenvm-c/` (cabeceras y fuentes
que necesita el AOT) y **`firmware/`** con las **nueve imágenes** —las ocho de siempre (la de la RP2350, con sus dos
firmwares) y la nueva de la P4X—, el firmware de la C6 de la P4 de Waveshare (el coprocesador de ESP-Hosted 3.0.9, de
Espressif, para su WiFi y su Bluetooth), su `SHA256SUMS.txt`, un `LEEME.md` y las plantillas de `board.json` del
RP2350 en `firmware/boards/`. Cómo instalar cada una, y qué packs lleva cada placa:
**[INSTALAR_FIRMWARE.md](INSTALAR_FIRMWARE.md)**.

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
  con la del PC, verde en las diez placas el 12-oct, con las imágenes que se publican
  —saltando lo que una placa no tiene: la pantalla, el AOT en la S3, memoria en la C6—. Y la
  de soltar los recursos (`#667`), 13 por placa: un programa toma todos los PWM, las UART o
  las tiras NeoPixel de la placa, se para con un Stop, y el siguiente los vuelve a tener.
- **Los dos núcleos**: 110 minutos de resistencia con dos `run` en la Pico 2, la S3 y la P4,
  sin un fallo.
- **La red de paridad dual-VM** pasa de 59 a 149 casos; y las VMs de V7 siguen ejecutando
  byte a byte los 59 `.mod` de V6 de la cápsula, con su stdlib.
- **102 pruebas de C** de la VM en la red (el AOT, el depurador, el arranque, la memoria…), y
  19 programas de `samples/errores/`, que tienen que fallar, fallan con el mensaje que toca
  (50 comprobaciones).
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
