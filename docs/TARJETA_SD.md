# BasicPlus — la tarjeta SD

> Todo lo de aquí está **medido en placa**, y donde algo no está probado se dice.
> Si algo no funciona como se cuenta, es un bug y va a `docs/FICHAS.md`.

Una tarjeta SD le da a un microcontrolador lo que no tiene: **sitio**. Gigabytes
para registros, medidas, bases de datos o ficheros de configuración, en vez de los
pocos megas de la flash interna. Y se usa igual que el resto del sistema de
ficheros: **es una ruta más**.

```basic
writeFile("/sd/medidas.csv", "hora;valor\n")
```

---

## 1. Dónde funciona

| familia | tarjeta SD | cómo |
|---|---|---|
| **RP2350** (Pico 2, Metro) | ✅ | por SPI |
| **ESP32** (P4) | ✅ | por SDIO, 1 o 4 bits |
| **STM32** | ❌ todavía no | — |

Y un matiz que importa más de lo que parece:

> **La tarjeta es una capacidad de la IMAGEN, no de la placa.** El firmware de
> RP2350 es único para Pico 2 y Metro. Lo que la Metro tiene de más es el **lector
> soldado**; en una Pico 2 puedes cablear un lector a los pines que quieras y
> funciona igual, sin firmware especial. Lo mismo en la familia ESP32: el P4 trae
> lector, y otra placa de la familia lo lleva si se lo pones.

Las placas que traen lector y funcionan enchufando la tarjeta y ya: **Metro** (RP2350)
y **ESP32-P4**.

---

## 2. Decirle dónde está el lector

Los pines **no van en el código**: van en el entorno de la placa, en la clave `sd`.
Así el mismo programa vale para dos cableados distintos, que es la misma idea que
hace que un `.mod` corra en cualquier familia.

**En RP2350** (SPI) — así están cableados los de la Metro:

```
sd=sck:34,mosi:35,miso:36,cs:39,cd:40
```

**En ESP32-P4** (SDIO) — así está el P4:

```
sd=clk:43,cmd:44,d0:39,pwr:45,pwralto:1
```

Las claves que acepta la variante SDIO son `clk`, `cmd`, `d0`…`d3` (para 4 bits),
`pwr` y `pwralto` (el pin que alimenta el raíl y con qué nivel se enciende), `slot`,
`khz` (velocidad, por defecto 20000) y `ldo` (el regulador interno de las E/S).

`cd` en SPI es el *card detect*, opcional: si no lo cableas, quítalo de la línea.

---

## 3. Comprobar que ha montado

El arranque lo dice, y conviene mirarlo antes de sospechar del programa. En el P4:

```
sd: SDIO slot 0, 4 bit(s), clk 43 cmd 44 d0 39 | pwr 45 (activo bajo) | ldo 4 | 20000 kHz
sd: montada en /sd (4 bits, particion en el bloque 2048)
```

**Las dos líneas dicen cosas distintas y las dos hacen falta.** La primera es la
CONFIGURACIÓN: con qué pines y a qué velocidad lo va a intentar. La segunda es el
RESULTADO. Si sólo sale la primera, el problema es de cableado, de alimentación o de
tarjeta; si sale la segunda, la tarjeta ya está y lo que falle es tuyo.

> Esa primera línea no es adorno: sin ella, «falla» no dice con qué datos lo intentó,
> y no se puede distinguir «el lector está mal» de «le estamos pasando otros pines».

---

## 4. Usarla: es sólo una ruta

Montada la tarjeta, todo lo que ya sabes hacer con ficheros funciona bajo `/sd`.

```basic
if not fileExists("/sd/config.txt") then
  writeFile("/sd/config.txt", "intervalo=60")
endif
var cfg: string := readFile("/sd/config.txt")
```

**Los verbos de fichero** son builtins, no hace falta importar nada:

| verbo | qué hace |
|---|---|
| `readFile(ruta)` | el contenido entero, como cadena |
| `writeFile(ruta, texto)` | lo escribe; si existía, **trunca** |
| `appendFile(ruta, texto)` | añade al final |
| `fileExists(ruta)` | `true` / `false` |
| `listDir(ruta)` | lo que hay en un directorio — ⚠️ **sólo en el PC**, ver §6 |
| `readFileBytes(ruta)` | el contenido como bytes |
| `writeFileBytes(ruta, datos)` | escribe bytes |

**Y el módulo `IO`** para lo demás: `mkdir`, `rmdir`, `removeFile`, `rename`,
`copyFile`, `fileSize`, `isDirectory`, `lastModified`, y la familia de rutas
`pathJoin`, `pathParent`, `pathBasename`, `pathExtension`, `pathAbsolute`.

```basic
import IO

IO.mkdir("/sd/registros")
var f: string := IO.pathJoin("/sd/registros", "2026-08.csv")
appendFile(f, "12:00;21.5\n")
print IO.fileSize(f), "bytes"
```

Las rutas llevan **siempre `/`**, también en Windows cuando pruebas en el PC.

Y en los literales de cadena valen los escapes de siempre: `\n`, `\t`, `\r`,
`\\`, `\"` y `\0`.

---

## 5. Dos sistemas de ficheros, uno al lado del otro

Conviene tenerlo claro porque explica varias diferencias:

- **La flash interna** de la placa lleva **littlefs**, pensado para memoria que se
  desgasta y para cortes de corriente. Ahí viven `/app`, `/lib` y `/sys`.
- **La tarjeta** lleva **FAT**, que es lo que entiende un PC. Por eso puedes sacarla,
  meterla en el portátil y leer lo que ha escrito el micro.

Esa es toda la razón de que la tarjeta sea el sitio de los datos y la flash el de los
programas: los datos quieres poder leerlos en otro sitio.

**Formatea las tarjetas en FAT32.** Las de más de 32 GB vienen a menudo en exFAT de
fábrica, y **exFAT todavía no está soportado** — reformatéalas. Probadas en placa:
una SanDisk de 128 GB en Metro y P4, y una de 32 GB en la Metro.

---

## 6. Recorrer un directorio — ⚠️ hoy sólo en el PC

```basic
var nombres: string[] := listDir("/sd/registros")
for f in nombres do
  print "  ", f
next f
```

Devuelve un `string[]`, y los arrays se recorren con `for … in … next`. Ojo con esto,
que es fácil de confundir: `.length()` es de los **objetos** —una `List`, un
`StringBuilder`—, no de los arrays.

> 🔴 **`listDir` NO está en la VM-C, que es la que corre en las placas.** Funciona
> cuando pruebas con la VM de referencia en el PC, y en el micro lanza
> *«builtin 42 no soportado en esta VM (subconjunto C)»*. Es el **único** verbo de
> fichero que falta: `readFile`, `writeFile`, `appendFile`, `fileExists`,
> `readFileBytes` y `writeFileBytes` sí están en las dos.
>
> No lo confundas con el árbol de ficheros del IDE, que sí lista la tarjeta: eso lo
> hace el firmware por el protocolo de depuración, no tu programa.
>
> **Mientras tanto**, si tu programa necesita saber qué ficheros hay, llévate tú la
> cuenta: un índice en un fichero de texto, o nombres predecibles por fecha
> (`2026-08.csv`) que puedas construir en vez de descubrir.

Y una nota de coste para cuando esté: en un micro, listar un directorio con muchos
ficheros **cuesta tiempo real**, sobre todo si la tarjeta es lenta. No es una
operación gratis que se pueda meter dentro de un bucle.

---

## 7. Texto y bytes

`readFile`/`writeFile` trabajan con **texto**; `readFileBytes`/`writeFileBytes`, con
**bytes crudos**. Usa los de bytes para imágenes, binarios o cualquier cosa que no
sea texto: pasar un binario por la versión de texto lo estropea.

Y una advertencia de tamaño: `readFile` **se trae el fichero entero a memoria**. En
un micro con unos pocos megas de heap, leer así un fichero de 50 MB no va a acabar
bien. Para ficheros grandes, o lo partes, o usas una base de datos — que para eso
está (ver `docs/BASEDATOS.md`).

---

## 8. Trampas conocidas

**Sin `pull-up`, un bus no calla: MIENTE.** Si cableas tu propio lector, no te
saltes las resistencias. Un pad sin pull-up arranca en pull-down y devuelve `0x00`,
que la tarjeta acepta como respuesta válida — o sea que en vez de silencio obtienes
un falso positivo, que es mucho peor de depurar.

**La alimentación del raíl.** En placas que conmutan la alimentación de la tarjeta
con un transistor (el P4 lo hace por GPIO45), la polaridad importa: `pwralto:1` si
el raíl se enciende con el pin alto, y sin él se asume activo bajo. Si te equivocas,
la tarjeta simplemente no responde, sin decir por qué.

**No confundas la placa con la imagen.** Cuando leas «Pico» en la documentación o en
un mensaje, casi siempre se refiere a la **imagen** del firmware, que es única para
Pico 2 y Metro. La Pico 2 tal como viene no tiene lector; la Metro sí.

**Sacar la tarjeta con el programa escribiendo** corrompe lo que estuviera a medias,
igual que en un PC. No hay magia: cierra antes.

---

## Por dónde seguir

| quiero… | mira |
|---|---|
| escribir y leer ficheros | `samples/FileTest.bp` |
| las operaciones de `IO` | `samples/FileOpsTest.bp` |
| bytes crudos | `samples/FileBytesTest.bp` |
| una base de datos en la tarjeta | `samples/SqlDemoSd.bp` y `docs/BASEDATOS.md` |
