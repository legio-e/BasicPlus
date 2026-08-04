# V5 — ideas y diseño

Documento hermano de `V3_IDEAS.md` y `V4_IDEAS.md`: aquí se registran las charlas
de diseño de V5 **antes** de escribir código, que es el método de la casa.

Nada de esto es de V4. V4 está en FEATURE FREEZE.

---

## El orden de V5 (Eduardo, 28-jul)

1. **Leer tarjetas SD.**
2. **FS para las SD** (FAT32 o lo que toque).
3. **SQLite.**
4. **Una librería BP** encima, con funciones `native` que llamen a la API de
   SQLite si hace falta.

Y sobre esa base, el ORM.

---

## ORM sencillo sobre SQLite — charla del 29-jul

Punto de partida de Eduardo: con SQLite pelado no basta. Escribir los `SELECT` a
mano y mapear columna→variable es el trabajo aburrido y propenso a fallos que un
lenguaje debería quitarte.

### La decisión de fondo: DAO GENERADO, no reflexión

Referencia: ORMLite. La diferencia es el mecanismo — **ORMLite accede a la clase
por reflexión en tiempo de ejecución; aquí se fabrica un DAO que lo hace**.

No es una preferencia estética, y conviene dejar dicho por qué:

- **BP no tiene reflexión**, y en un micro se pagaría dos veces: metadatos vivos
  en RAM y despacho dinámico en cada acceso.
- Generar el DAO mueve todo eso a tiempo de compilación: **coste cero en
  ejecución**.
- El DAO es **BP visible**: se lee, y se **depura con el depurador del proyecto**.
  Nada de magia opaca en tiempo de ejecución — que es justo lo que hace odiosos a
  la mitad de los ORM.

Es el mismo canje que ya se hizo con `FormBaker` (resolver evento nombre→slot al
hornear el `.win`, en vez de resolver por nombre en la placa). Coherente con la
casa, y con precedente que funciona.

### Forma — DECIDIDO

- **Entity = clase normal.** Sin clase base especial, sin interfaz que implementar.
- **Los campos van como `property` públicas** (norma del lenguaje: público ⇒
  property).
- **Anotaciones `@BD{ ... }`** sobre la property **y también sobre la clase**
  (ver más abajo: mecanismo general con PREFIJO de espacio de nombres).
- **Nombre de tabla** = nombre de la clase por defecto; la anotación de clase lo
  sobreescribe si no coincide.
- **Nombre del DAO** = nombre de la clase + `"Dao"`. Sencillo y predecible.
- **La Entity NO conoce a su DAO.** Queda como clase de datos pura: se construye,
  se pasa y se prueba sin base de datos delante. Si se acoplan, cualquier test
  necesita SQLite.

**Ventaja lateral sobre ORMLite:** como los campos son `property`, el DAO lee y
escribe **por los accesores**. ORMLite va al campo por reflexión y se salta el
contrato; aquí se pasa por él, así que la validación o normalización de un `set`
se ejecuta. Es más correcto y sale gratis.

### Dónde viven las anotaciones — MATIZ IMPORTANTE

Eduardo: "entiendo que irá a metadatos". Matiz que cambia el coste:

**Las anotaciones no tienen por qué llegar a la placa.** Quien las necesita es el
GENERADOR, no el runtime — precisamente porque el DAO ya viene generado. Meterlas
en los metadatos del `.mod` que carga el micro es pagar bytes de imagen por algo
que allí nadie lee.

Pero sí tienen que viajar en un caso real: cuando la Entity vive en **otro
módulo** y el generador trabaja sobre ella importada. Y para eso ya existe el
sitio exacto: **la interfaz embebida del `.mod` v6** — la lee el compilador
(`Main.java:1290`) y **las VMs la saltan**. Las anotaciones viajan donde hacen
falta y la placa no carga ni un byte de ORM.

### Anotaciones mínimas (v1)

Con cuatro cosas sobra para empezar:

- `PK` — clave primaria
- autoincremento
- nombre de columna, cuando no coincide con el de la property
- "no nulo"

Índices, tamaños y tipos forzados pueden esperar sin bloquear nada.

### Consultas — DECIDIDO para v1, con una condición

Las consultas devuelven una **lista de entities**, y **debe haber un límite**
(Eduardo). Un `findAll` sin tope sobre una tabla mediana se come el heap de una
Pico (257 KB).

⚠️ **El límite NO puede ser silencioso.** Una consulta que devuelve 100 de 5000
filas sin decirlo es exactamente la misma mentira que el log que truncaba en
silencio y que nos mandó dos veces a un sitio equivocado en #326. El resultado
tiene que poder contestar "hay más" — o el DAO avisar — pero **callarse, no**.

### API mínima (v1)

Una clase → una tabla, y:

- `insert` / `update` / `delete`
- `findById`
- `find` por un campo

Sin relaciones, sin carga perezosa, sin lenguaje de consulta. Con eso ya se
escriben aplicaciones de verdad, y lo demás entra después sin romper nada.

### MÁS ADELANTE: la "ventana" sobre una tabla (idea de Eduardo)

Un objeto que sea una **ventana** de la tabla: permite navegar **arriba y abajo**
manteniendo en memoria sólo unos pocos registros.

Es la solución buena al problema de la memoria, y además es exactamente lo que
necesita una tabla en pantalla (el mismo concepto de ventana deslizante que ya usa
el widget `Chart`). No entra en v1, y no hay conflicto: la ventana es una API
ADICIONAL, no un reemplazo de las listas. El límite de v1 es lo que evita que se
escriba código encima que dé por hechas listas ilimitadas.

### Anotaciones: mecanismo GENERAL con prefijo — DECIDIDO (Eduardo, 29-jul)

No es del ORM: es del lenguaje. Y llevan **prefijo de espacio de nombres** —
`@BD{ ... }` para la base de datos, `@Json{ ... }` para serializar, etc.
("una vez que destapas la liebre salen muchas más").

El prefijo resuelve tres cosas de una vez:

1. **Sin colisiones** entre herramientas (dos que quieran `name` no se pisan).
2. **El compilador NO necesita conocer el vocabulario**: parsea `@Nombre{ … }`,
   comprueba que está bien formado y lo guarda tal cual. Cada herramienta lee lo
   suyo.
3. Y la consecuencia buena: **una herramienta nueva no obliga a tocar el
   compilador**. Para un equipo pequeño eso es mucho.

⚠️ **La pega, y su red barata.** Si te equivocas escribiendo el prefijo
(`@BB{PK}`), el compilador se lo traga —no conoce vocabularios— y **la PK
desaparece en silencio**. Es la familia de bug de #326 otra vez. La red no está en
el compilador sino en el generador: si se le pide un DAO de una clase **sin ningún
`@BD`**, o **sin PK declarada**, que se NIEGUE con un mensaje claro en vez de
fabricar un DAO roto. Un `if` ahora, o un bug que se descubre con la tabla ya en
producción.

### Abierto

- **¿De dónde sale el esquema?** Con lo decidido (Entity = clase + anotaciones) el
  camino es *code first*: de la clase salen la tabla y el DAO. Queda por decidir
  si además se ofrece introspección de una BD existente (`PRAGMA table_info` →
  generar el BP), que yo dejaría como herramienta aparte, no como camino
  principal.

---

## SQLite en un PACK no es sólo modularidad: adelgaza la construcción de imágenes — Eduardo, 4-ago

> «Lo que vamos a hacer en V5 de enviar el SQLite a un pack no solamente nos es
> útil por temas de hacerlo modular, es que además **simplifica la construcción
> de las imágenes de los micros**.»

El argumento es de coste de mantenimiento y es el más fuerte de los dos. Hoy,
**todo lo que el firmware necesita va compilado DENTRO de la imagen**: núcleo,
littlefs, LVGL, los blobs de stdlib (`esp32_mods.c`, `stm32_mods.c`…), el
`Hello` y el `Bench.mdn`. Meter SQLite ahí significaría:

- **Alta en los cinco builds** — el patrón que ya nos ha mordido (olvidar uno =
  esa familia no enlaza y no te enteras hasta reconstruirla).
- **Engordar TODAS las imágenes**, incluidas las que no lo van a usar nunca. La
  Pico tiene 4 MB de flash y 520 KB de SRAM: SQLite no cabe con holgura ahí, y
  aun así pagaría el peso en el binario.
- **Atar la versión del motor de BD a la del firmware**: actualizar SQLite
  pasaría por **reflashear**, con todo lo que eso arrastra.

Como pack: la imagen **no cambia**, sólo lo llevan las placas que lo necesitan,
actualizarlo es **quemar un pack** en vez de reflashear, y en placa la zona de
packs es **XIP**, así que el código ejecuta desde flash sin comerse la RAM.

### La base ya está probada, y se probó hoy

Los packs de **código nativo** se apoyan en el **loader `.mdn` del AOT** — el
mismo que en la tanda del P4 cargó `Bench.mdn` (1 thunk, 130 B) y dio **116×**.
O sea que el mecanismo de «traer código máquina a una VM en marcha, validarlo
por ABI y engancharlo» **no hay que inventarlo: está funcionando en placa**.

### Las dos partes duras, dichas de frente

1. **La escala.** Hoy el `.mdn` carga 130 bytes; SQLite son cientos de KB. Ahí
   el paso de **enlace** del `.text` (el que hizo falta en H4) deja de ser un
   detalle, y hay que decidir XIP contra copia a RAM.
2. **Las raíces del GC.** El **paso 3 de #302** —shadow stack / raíces GC del
   nativo compilado— sigue abierto y se difirió *a AOT-en-placa*. Un cuerpo
   nativo grande que maneje objetos BP sube esa apuesta: es el prerrequisito
   real, no un flanco.

## La zona de packs debe servir RECURSOS, no sólo módulos — Eduardo, 2-ago

Sale de una pregunta suya mientras se documentaban los packs de V4: *«en los
packs podemos incluir fuentes e imágenes; ahora, ¿cómo se pueden utilizar desde
BP?»*.

### Lo que hay hoy (V4), medido

Un pack **en ejecución** sirve sus recursos de forma transparente: no hay API de
packs, se leen **por su nombre con las funciones de fichero de siempre**. En la
VM-C está bien puesto —un *overlay* en la **fachada del FS**, un solo sitio— y
por eso lo heredan sin tocarlos `readFile`, `fileExists`, el `.win`, la carga de
imagen y la de fuente.

Pero eso vale **sólo para el pack que se está ejecutando**. Un pack **quemado en
la zona de packs** aporta **módulos** (por `import`) y **nada más**: sus fuentes
e imágenes están ahí, escritas en la flash, y no hay forma de leerlas.

### Lo que pide Eduardo, y por qué

Que la zona de packs sirva también recursos. Tres usos, y los tres son la misma
idea: **sacar de la imagen del sistema lo que no tiene por qué estar ahí**.

1. **Fuentes.** Hoy las que quepan en el firmware. En la zona de packs, un
   número **prácticamente ilimitado** — y se añaden sin reconstruir la imagen.
2. **Imágenes.** Aquí lo que tiene sentido no es cualquier imagen sino los
   **iconos de uso corriente y algún logo**: lo que se repite en todas las
   aplicaciones y hoy o viaja con cada una o no está.
3. **Drivers de pantalla** (esto mira más lejos). Hoy la imagen del sistema
   carga con todos los paneles que quiera soportar. La idea es darle la vuelta:
   **el usuario graba el driver de la pantalla que va a usar de verdad**, y en
   la imagen quedan **sólo los más comunes**. No es cosmético: es lo que evita
   que el firmware crezca sin techo según se van añadiendo paneles.

El 3 es el que más lejos llega y engancha con la línea que ya estaba apuntada
para V5 —**packs de código nativo**, con SQLite de piloto y LVGL después— porque
un driver de panel no es un recurso: es **código**. Los dos primeros, en cambio,
son datos y se pueden hacer con lo que ya existe.

### Lo que costaría (los dos primeros)

Poco, y ésa es la parte buena: **la maquinaria ya está**.
`bpvm_pack_find(base, size, tipo, nombre, len)` **no es específica de módulos**
—el tipo es un parámetro— y la zona ya está montada (`bpvm_pack_mount`) y ya se
consulta para resolver `import`. Falta que el overlay de recursos, que hoy sólo
mira `run_pack_src`, mire **también** la región montada.

### Lo que hay que DECIDIR antes de tocar nada

Dos cosas, y ninguna es de implementación:

- **El orden de precedencia.** Hoy conviven dos reglas distintas: el pack en
  ejecución va **antes** que el FS (sus recursos son «suyos»), y para los
  módulos el **FS eclipsa** al pack de la zona (spec §4). Lo natural sería
  encadenarlas —*pack en ejecución → FS → zona de packs*, de lo más propio a lo
  más general— pero conviene decirlo en voz alta, porque de eso depende si el
  usuario puede tapar un icono del sistema poniendo uno suyo en el FS.

- ~~Las colisiones de nombre~~ → **YA DECIDIDO** (Eduardo, 2-ago): *«lo de los
  nombres ya lo hablamos en su día para los módulos»*. Se aplica la MISMA regla,
  que no hay por qué inventar dos veces:

  > **Si quieres leerlo de un pack concreto, añades el nombre del pack al del
  > recurso. Si no lo indicas, lee el primero que encuentre.**

  Es exactamente el modelo de los módulos, donde `import Modulo` resuelve por
  búsqueda y `import Modulo from pack MiPack` fija el origen. Al escribir la
  forma concreta del nombre del recurso conviene que **se parezca a esa**, para
  que sea una regla y no dos.

Y un límite del formato que conviene recordar al elegir nombres: la extensión es
un FourCC —**4 caracteres como mucho**— y el nombre, 32. No es un recorte
silencioso: pasarse es error al construir el pack.

---

## Linux, y la Raspberry Pi como PLACA — Eduardo, 2-ago

Sale al decidir el alcance del paquete de V4: **el IDE, de momento, sólo
Windows**. Textual: *«para Linux hay que probarlo, me temo que hoy por hoy
muchas cosas no funcionarían por temas de nombres, paths y temas de permisos.
Eso necesita un trabajo específico que en esta versión no podemos abordar pero
sí en el futuro»*. Tiene una **Raspberry Pi 400** para las pruebas.

Y añade algo que **no es lo mismo** y conviene no mezclar:

> *«además en ese caso me gustaría que la VM-C soportase la librería de
> Raspberry y pudiese trabajar con pines, I2C, SPI, etc.»*

### Son dos trabajos, no uno

**(A) El IDE en Linux** — es portabilidad de aplicación de escritorio: rutas y
separadores, mayúsculas/minúsculas en los nombres de fichero (Windows perdona,
Linux no), el lanzador (`.bat` → script), los binarios que acompañan
(`.exe`/`.dll` → ELF/`.so`) y sobre todo **permisos**: el puerto serie en Linux
pide pertenecer a `dialout`, y eso es justo el tipo de detalle que convierte un
«no me funciona» en media tarde. Trabajo acotado y aburrido, pero real.

**(B) La Raspberry como PLACA de verdad** — esto es otra cosa, y es la
interesante: que un programa BP corra en la Pi **moviendo pines de verdad**,
con I2C y SPI. No es portar el IDE: es **una familia más** en el HAL.

### Por qué (B) encaja tan bien

Encaja con la doctrina de la casa —*motor único + cintura por micro*— con un
matiz bonito: aquí **la cintura es un sistema operativo, no un chip**. La VM-C
ya compila y corre en Linux (es C portable; el host build es eso). Lo que falta
son los **backends de hardware**, y en la Pi no los pone un SDK de fabricante
sino el propio Linux:

- **GPIO** → `libgpiod` (el `/sys/class/gpio` viejo está deprecado).
- **I2C** → `/dev/i2c-N` con `ioctl`.
- **SPI** → `/dev/spidev0.0` con `ioctl`.

Es decir: los mismos `Gpio.Pin`, `I2c.Bus` y `Spi.Bus` que ya existen en BP, con
una cintura nueva debajo. **El código BP del usuario no cambiaría** — que es
exactamente la promesa del proyecto, y la Pi sería la prueba más vistosa de
ella: el mismo `.mod` en una Pico, en un ESP32 y en un ordenador entero.

### Lo que hay que DECIDIR, porque la Pi no es un micro

En la Pi **no hay firmware que flashear**: la VM es un proceso. Eso rompe varias
suposiciones del modelo de «placa» que hoy damos por sentadas, y ninguna es
grave pero todas hay que responderlas:

- **El sistema de archivos** ya es un FS de verdad. ¿Sigue habiendo `/app`,
  `/lib`, `/sys` (como carpetas bajo un directorio raíz), o se usa el del
  sistema tal cual?
- **Las particiones y la zona de packs** no tienen sentido literal — no hay
  flash cruda que repartir. ¿Un fichero-imagen que las simule (como ya hace el
  micro simulado), o se desactivan y los packs se leen del FS?
- **El arranque por capas (H9)** — el estado 0 «kernel» no aplica: en la Pi el
  sistema operativo ya está vivo antes que nosotros.
- **Cómo se conecta el IDE**: por TCP, como al micro simulado, en vez de por
  serie. Eso ya existe.

Buena parte de esas respuestas **ya están escritas** en el micro simulado
(H10): es un proceso de PC que el IDE trata como una placa, con su flash y su
FS en ficheros. La Pi sería ese mismo modelo, pero con los pines de verdad
conectados por debajo.

## Una librería de placa GENÉRICA — Eduardo, 4-ago

> «`Pico.bp` es para la Pico y la Metro y las placas que lleven RP2350. Ésta es
> otra familia. Si acaso habría que hacer otra librería para Espressif, pero no
> en V4, ya dijimos que congelamos código. **Apunta hacer una librería genérica
> para todas las placas. Que sean los micros los que den la información y la
> librería haga de puente para BP.**»

### De dónde sale

Del **hallazgo 19** de H13. En el ESP32-S3, `BoardTest` imprimió:

```
gpioCount=45
variant=B
GPIO_COUNT=45
```

Los dos números son correctos; `variant=B` no significa nada en un S3 — «A/B» es
el bondeado del RP2350 (A = 30 GPIO, B = 48). Sale de `bpstdlib/Pico.bp:92`, que
**no es un intrínseco**: son cinco líneas de BP puro, `if gpioCount() >= 40 then
"B" else "A"`, y los 45 pines del S3 caen del lado del 48.

### El reencuadre de Eduardo — que es lo importante

Yo lo leí como «`variant()` miente». No es eso: **`variant()` hace exactamente lo
que documenta**. Lo que pasa es que `Pico.bp` **es la librería de la familia
RP2350**, y se está usando en otra familia porque para ésa no hay ninguna. El
fallo no está en la función: está en que **hay una sola librería de placa y es la
de un micro concreto**.

### Lo que hay hoy, medido

- `bpstdlib/Pico.bp` va **embebida en las tres familias** (`MODS=(… Pico …)` en
  los tres `regen_*_mods.sh`), y los samples la importan en todas.
- Los intrínsecos que expone (`gpioCount`, `boardName`, `resetCause`, `tempC`,
  `cpuFreqHz`, `uptimeMs`…) **sí bajan al backend por familia** — ésos ya están
  bien: el dato lo da el micro. El problema son las **derivaciones en BP**, como
  `variant()`, que hornean el modelo de un micro dentro de la librería común.
- El firmware del RP2350 **ya tiene el dato de verdad**: `board_desc()->variant`.
  Lo que falta no es el dato, es el camino hasta BP.

### Lo que pide Eduardo

Una **librería de placa única**, común a todas las familias, en la que:

- **el dato lo da el micro** (backend por familia), y
- **la librería es sólo el puente a BP** — no la fuente de verdad, no el sitio
  donde se decide nada.

O sea: la misma forma que ya tienen `Gpio`/`I2c`/`Spi`/`Uart` (fachada BP +
cintura por micro), aplicada también a la identificación de la placa.

### Lo que hay que decidir antes de tocar nada

- **El nombre.** `Board`, `Mcu`, `Device`… y qué pasa con `Pico`: ¿alias que se
  mantiene por compatibilidad, o desaparece? (Hoy `Pico` es el nombre de un
  producto ajeno usado como nombre genérico — parte del lío viene de ahí.)
- **Qué se hace con lo específico de una familia** (el A/B del RP2350, el PIO,
  la PSRAM del P4…). ¿Una librería por familia además de la común? ¿O queda todo
  en la común y cada micro contesta lo suyo?
- **Y la pregunta que ha destapado el hallazgo: qué contesta la librería cuando
  el micro NO tiene ese concepto.** Hoy la respuesta es la peor posible —
  contesta como si lo tuviera. Debe haber una forma de decir *«aquí eso no
  existe»* que el programa pueda ver, y que no se confunda con un valor válido.
