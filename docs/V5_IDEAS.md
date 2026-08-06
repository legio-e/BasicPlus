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

## El PUENTE a SQLite: cómo se llama a un binario dentro de un pack — charla del 4-ago

Diseño de Eduardo, en dos partes:

1. **Un shim en C que se compila JUNTO a SQLite** y construye su tabla de
   funciones. Va dentro del mismo blob.
2. **`SQLite.mod`**, un módulo BP con muchas funciones `native` que usan esa
   tabla y exponen la API al resto del lenguaje.

Los dos viajan **en un mismo pack**, que se graba en la zona de packs.

### Lo que NO se toca, y por qué — criterio de Eduardo

**El código de SQLite NO puede tener dirección fija.** Yo propuse enlazarlo a la
dirección de la zona de packs para ahorrarnos las relocations, y Eduardo lo paró
con el argumento bueno: *«si no, el mecanismo que hemos montado de packs habría
que rehacerlo»*. La zona reparte huecos libremente; un pack que exija una
dirección concreta **impone esa restricción a todos los demás**. Era optimizar el
caso local a costa del mecanismo general — justo lo que la doctrina de la cintura
existe para evitar.

### Lo decidido

- **El pack lo construimos NOSOTROS**, una vez por ISA (**ARM** y **RISC-V**), y
  **no se recompila nunca**. El usuario sólo lo graba. Consecuencia estratégica:
  **el usuario no compila C jamás**, ni siquiera para tener una base de datos.
- **Entrar (exports)**: la tabla guarda **offsets**, no direcciones — igual que el
  `.mdn`. Al cargar, `base + offset` una vez y a correr.
- **Salir (imports)**: SQLite necesita llamar al anfitrión (VFS, `malloc`, hora).
  Esa tabla vive **en el firmware, en una dirección FIJA** — y ahí sí, porque la
  imagen la controlamos nosotros. Para el blob son constantes de compilación:
  **cero relocations en esa mitad**.
- **Y la misma estructura fija sirve en los dos sentidos**: el shim escribe ahí el
  puntero a su tabla de exports al cargar, y los `native` de `SQLite.mod` lo leen
  de esa dirección conocida. Un solo punto de encuentro; **sin registro de packs
  ni búsqueda por nombre**.
- El **VFS de SQLite** acaba siendo otra **cintura sobre la fachada del FS** — el
  mismo patrón que ya funcionó tres veces.
- Regalo de una decisión vieja: desde H2.1 las cadenas BP son **UTF-8 en array de
  bytes** y SQLite habla `const char*` UTF-8. **No hay capa de conversión.**

### Lo que queda ABIERTO, y se decide midiendo

**¿Necesita relocations el acceso de SQLite a sus PROPIOS datos estáticos?** Es la
única pregunta que queda, y es empírica. Hay motivos para el optimismo: la
amalgama es **una sola unidad de traducción** (todos los símbolos son locales) y
tanto RISC-V (`medany`) como ARM saben direccionar datos locales **PC-relativos,
sin GOT**. Si sale así, el esquema es **enteramente libre de relocations**:
PC-relativo por dentro, dirección fija por fuera, offsets para entrar.

Se comprueba en cinco minutos y sin placa:

```sh
riscv32-esp-elf-readelf -r sqlite_pack.o | head -40
```

Vacío ⇒ el diseño vale tal cual. Si aparece una GOT, sabremos **cuántas entradas**
hay que resolver y se decide entonces (la salida natural sería relocalizar **al
grabar**, que es el único momento en que se conoce la dirección y encima lo hace
el PC). **Hacer esa medida ANTES de escribir nada.**

### Lo único que cruza el tiempo

El pack se congela, pero **lo de enfrente sigue moviéndose**: los `native` de
`SQLite.mod` los compila el IDE en cada Run, y el firmware se actualiza. Alguien
puede tener quemado el pack de V5.0 y un IDE de V5.2. Eso pide **una versión de
ABI en la cabecera del pack** que el cargador compare y, si no casa, lo diga y se
pare. Mecanismo ya hecho dos veces: #284 en el `.mod` y el gate de arch del
`.mdn`. Sin él no falla — **funciona raro**, que es peor.

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

## La degradación entre RUN: cómo se investiga — Eduardo, 5-ago

Dictado al cerrar el **hallazgo 34** de H13, que resultó ser el **24** otra vez.
No es una propuesta de arreglo: es **el método de la investigación**, escrito
antes de empezarla para no improvisarlo el día que toque.

### El razonamiento de Eduardo, que es el que manda

> «Dos errores similares con diferente hardware e imagen implica un fallo en lo
> que es común a los dos firmwares. Y puesto que estamos probando Threads, o es
> la gestión de los Threads o es la memoria (el Heap). Para cuando lo
> investiguemos: hay que hacerlo determinista, cuando lo podemos provocar de
> forma sistemática, y separar si es un problema de Threads o del Heap (o de
> otra cosa).»

**Lo común está delimitado y es poco**: el núcleo portable de `bpgenvm-c/src`,
los mismos 24 ficheros que enlazan las tres familias (`bpvm` `loader` `interp`
`heap` `builtins` `link` `scheduler` `threading` `exceptions` …). Lo que **no**
es común —la cintura por micro, el FS, el transporte, LVGL— queda fuera por
construcción: **no puede explicar un fallo que sale en dos familias**. Es el
mismo argumento que cerró la hipótesis de la flash en la c1, y vale igual aquí.

### Una precisión sobre los dos sospechosos

Los dos síntomas **no son el mismo test**: el 24 fue **la pantalla del P4** (con
LVGL) y el 34 fueron **los threads de la Nucleo** (sin pantalla). Y eso desempata
parcialmente la lista de Eduardo:

- **El heap explica LOS DOS.** El modelo del GUI **vive en la VM** —eso se
  demostró en #352, que se rompía a los 30 RUN— así que si lo que se degrada es
  memoria, pintar y crear threads se rompen los dos.
- **La gestión de threads explica SÓLO UNO.** El caso del P4 no crea threads de
  BP.

Luego, sin descartar nada, **el heap es la hipótesis que cubre más evidencia** y
por ahí conviene empezar. La tercera puerta —«o de otra cosa»— tiene un candidato
concreto que también es común y también acumula: **la tabla de handles**, que
crece bajo demanda (`0 -> 4096 slots, 32 KB`) y es de H1.

### Paso 1 — hacerlo determinista, y hacerlo EN EL HOST

La degradación es **entre RUN**, no dentro de uno: no se puede reproducir desde
un programa BP, porque el bucle tiene que estar **por encima** del RUN. Eso
descarta escribir un sample y obliga a un arnés que **encadene N ejecuciones**.

Y por eso el primer intento va **en el host**, no en placa: el host tiene modo
**daemon con `runModule`** (A2.3), y encima están los instrumentos que en placa
no hay —el guardián de fin de RUN de **#339** (dice *quién* se quedó la memoria,
con fichero y línea), el volcado del GC con huecos y fusiones, y `--nogc` para
partir el experimento—. Si reproduce en host, la investigación entera se vuelve
barata y sin reflashear. Si **no** reproduce en host con el mismo número de
vueltas, eso ya es un dato grande: apunta a algo que sólo pasa en placa.

### CORRECCIÓN de Eduardo (mismo día): primero REPRODUCIR, luego minimizar

> «No es por el tamaño, que aquí es razonablemente grande. Y tampoco creo que
> con un test sencillo lo podamos reproducir. Esto ocurrió después de una tanda
> completa de test. Quizás para hacerlo sistemático habría que **fusionar toda
> la tanda de test en uno solo** y ver qué ocurre. Parece que con un uso
> intensivo del heap algo se degrada, pero eso es una impresión, primero habrá
> que demostrarlo.»

Tiene razón y **el orden que yo había propuesto estaba del revés**: los dos
arneses mínimos del paso 2 sirven para **separar** una causa, no para
**provocarla**, y no se puede bisecar lo que todavía no se sabe disparar. El
paso que falta va **antes**: una carga **realista y grande** que lo dispare de
forma fiable. Su propuesta —**fusionar la tanda entera en un solo programa**—
es la más barata, porque esa carga ya existe y ya se sabe que rompe.

Y de paso ese test fusionado contesta una pregunta que hoy no sabemos y que
cambia toda la investigación: **¿hace falta cruzar el límite del RUN, o basta
con muchas operaciones?** Si degrada **dentro de una sola ejecución**, el RUN
no pinta nada y todo se vuelve mucho más fácil de estudiar; si sólo degrada
encadenando RUN, entonces lo que se acumula sobrevive al fin de ejecución, que
es un sitio muy concreto donde mirar.

### La GRAVEDAD real, y por qué eso asciende UN experimento por encima de todos

> «Es de los que no salen con casos sencillos pero que pueden aparecer al cabo
> de 2 días de ejecución sin resetear, **mortal para una máquina de estados sin
> atención humana**. Lo digo por cuantificar el daño: **inofensivo en pruebas,
> mortal en explotación.**» — Eduardo, 5-ago

Esto **matiza la decisión del hallazgo 24**, que se cerró con el criterio de que
*encadenar RUN es actividad de desarrollo*. Sigue siendo verdad, pero sólo
protege **si lo que se degrada se degrada POR RUN**. Y ahí está la bifurcación,
que hoy **no sabemos resolver**:

| si lo que se acumula es… | un programa que corre para siempre… | gravedad |
|---|---|---|
| **por RUN** (algo que no se suelta al terminar) | **nunca cruza ese límite** | molestia de desarrollo — vale la decisión del 24 |
| **por OPERACIÓN** (por reserva, por thread, por handle) | **lo alcanza igual, sólo que más tarde** | **mortal en explotación** |

Luego el test fusionado que propone Eduardo **no es sólo la forma barata de
reproducirlo: es el que decide cuál de las dos filas es la buena**. Eso lo pone
por delante de todo lo demás de esta investigación — antes que separar threads
de heap, antes que mirar la tabla de handles. Primero saber **a quién mata**.

**Y hay una prueba parcial que ya está hecha, y cae del lado tranquilizador**:
**#357** dejó corriendo **10.000 vueltas en UN SOLO programa** en la Pico —exit
0, y el heap oscilando en una banda de ~130 bytes *indefinidamente*, «no hay
techo»—. Eso **es** el escenario de explotación en pequeño, y salió limpio. No
es concluyente y conviene no venderlo como tal: 10.000 vueltas de **un bucle**
no son dos días de una carga **variada**, no ejercita la tabla de handles ni el
GUI, y no cruza ningún fin de RUN. Pero es la mejor evidencia que hay hoy sobre
el caso que preocupa, y apunta a que lo que se acumula **vive en el límite del
RUN**, no en la operación.

📌 Conclusión operativa: la gravedad queda registrada como **condicional** —
*inofensivo en pruebas; mortal en explotación **si** resulta ser por
operación*— y el primer experimento de V5 es el que resuelve esa condición.

### Y el tamaño del heap queda DESCARTADO como causa — con números

La observación de Eduardo se puede cerrar del todo con lo que ya está medido:

| caso | placa | heap | ¿degradó? |
|---|---|--:|:--:|
| hallazgo 24 | ESP32-P4 | **27,5 MB** | sí |
| hallazgo 34 | STM32 Nucleo | **372 KB** | sí |

**Un factor de 75× en el heap no cambia nada.** Eso no es quedarse sin memoria:
lo que se degrada **no escala con el heap**. Y eso descarta un sospechoso y
asciende a otro:

- ❌ **Agotamiento**: descartado. Con 27 MB no se agota nada haciendo demos.
- ⬆️ **Recursos de tamaño FIJO que se consumen por operación**: suben al primer
  puesto. El candidato más claro es la **tabla de handles** (H1) —tiene *slots*,
  crece por demanda y **su tamaño no depende del heap**—; después, cualquier
  contador o tabla del núcleo que no se devuelva al terminar.
- ➡️ **Fragmentación**: sigue viva, pero con matices — el free list no fusiona
  (`fusion_izq=0 fusion_der=0`), y eso sí es independiente del tamaño.

⚠️ Todo esto sigue siendo **impresión hasta que se demuestre**, que es
literalmente lo que pidió Eduardo. Lo único demostrado es la tabla de arriba.

### Paso 2 — separar Threads de Heap, cambiando UNA cosa

Dos arneses mínimos, cada uno tocando **una sola variable**, ejecutados con el
mismo número de vueltas:

| arnés | qué hace en cada RUN | si degrada… |
|---|---|---|
| **A — threads sin memoria** | crea y joinea N threads que **casi no reservan** | es la **gestión de threads** |
| **B — memoria sin threads** | reserva y libera mucho **sin crear un solo thread** | es el **heap** |

Si degradan los dos, es algo por debajo de ambos (candidato: la tabla de
handles). Si no degrada ninguno, el disparador está en la combinación y hay que
volver al caso real. Es la receta de la casa —*partir de lo que va y cambiar una
cosa hasta romper*— aplicada a un fallo que hasta hoy sólo sabíamos provocar por
acumulación ciega.

### Los threads NO se descartan — y hay dos apuntes viejos que van al expediente

> «Sigue la duda si los threads afectan en algo (**el último bug que
> corregimos estaba en los threads**).» — Eduardo, 5-ago

Es un argumento de historial, y es bueno: ese camino **ya ha dado bugs hace
nada**, así que no se descarta por elegancia del razonamiento. Dos apuntes que
ya están escritos y que conviene tener delante el día que empiece:

- **#369** — *el frame inicial del thread se quedó en 4 bytes*. Cerrado y
  verificado en tres arquitecturas, pero es **el último bug de la casa** y
  estaba **en el arranque de un thread**: el camino tiene historial reciente de
  anchos de referencia mal migrados en la campaña 4→8B.
- **#356** — 🔴 **sigue ABIERTO y es el que más se parece a lo que buscamos**:
  *«la pérdida de bytes NO se manifiesta (era colateral de #357) — queda el
  **descarte mudo**, latente»*. Una pérdida que **no se manifiesta** es
  exactamente la forma de algo que se acumula sin avisar hasta que importa. Se
  rebajó porque no daba la cara; el 24 y el 34 podrían ser **la cara que le
  faltaba**. Nadie ha demostrado que sea lo mismo — pero es el primer sitio
  donde miraría después del test fusionado.

### Lo que hay que arreglar ANTES de poder investigar

- **El `exit 11` no dice nada** (hallazgo 34). El mensaje del `RuntimeError` es
  la primera pista y hoy no llega. Sin eso se investiga a ciegas.
- **El INFO del STM32 no da el heap** (hallazgo 30), que es justo el instrumento
  que hacía falta el día que salió.

Los dos están **en el lote de V4**, así que para cuando empiece V5 ya deberían
estar puestos.

## Una batería de rendimiento que sirva para DECIDIR, no para presumir — Eduardo, 5-ago

> «Para V5 tenemos que crear una batería de test de rendimiento de
> hardware+software, algo que sea **medible**. Lo importante de estas pruebas es,
> teniendo en cuenta las diferencias de hardware, **si se puede optimizar el
> software**. Por ejemplo, la escritura en ficheros lenta se puede amortiguar con
> buffer (en software, no el del SO) o trabajando con streams.»

Dictado al final de la fila c1, después de un día entero de impresiones sin
número: *«parece más ágil»*, *«los eventos también van más rápido»*, *«el FS va
igual en todas»*. Todas resultaron interesantes y **ninguna se pudo usar para
decidir nada** hasta que hubo un número —y el único número del día, los 207.017
ms del `paralleltest`, es el que destapó el hallazgo 33.

### Lo que la diferencia de un benchmark, que es lo que ya tenemos

**Ya existe una suite** (`samples/benchmarks/`: `Bench` `intbench` `floatbench`
`stringbench` `arraybench` `sortbench` `sumbench` `fibobench`, tarea #164). Mide
**cómputo**, y lo hace bien: `Bench.bp` incluso lleva el gemelo interpretado de
`fib` **para tener línea base en la misma placa y el mismo reloj**, que es
exactamente la idea correcta.

Lo que pide Eduardo **no es más de eso**. Es otra cosa, y la diferencia está en
la pregunta:

- Un **benchmark** contesta *«¿cuánto tarda?»* → sirve para comparar placas.
- Esta batería tiene que contestar *«**¿cuánto de esto es culpa nuestra?**»* →
  sirve para **decidir si merece la pena optimizar**.

Y eso obliga a que cada prueba traiga **su referencia dentro**, como ya hace
`Bench` con el gemelo interpretado. El patrón del día 5-ago vale de plantilla: el
`paralleltest` en bruto decía *«la Nucleo es lenta»* —falso como diagnóstico—;
**normalizado por reloj** decía *«va peor por ciclo que un Xtensa»*, que no
cuadra con el silicio y llevó derecho al `-O0`. **El número en bruto miente; el
número con su referencia habla.**

### Lo que hay que medir, que es el reparto

Para cada operación cara, separar **lo que impone el hardware** de **lo que
añadimos nosotros**, porque sólo lo segundo es accionable:

| operación | suelo del hardware | lo nuestro (accionable) |
|---|---|---|
| escribir fichero | ciclo de borrado + programación de la flash | nº de llamadas, tamaño de caché, copias intermedias |
| leer fichero | lectura de página | trocear, copias, conversiones |
| pintar | ancho de banda al panel (escala con **área**) | redibujados de más, invalidaciones |
| despachar evento | ~nada | cola, inyección del frame, asignaciones por evento |

Si de 100 ms el suelo son 95, **no hay nada que optimizar y hay que dejarlo
escrito** para no volver a intentarlo cada seis meses. Si son 40, hay 60 que
ganar y merece la pena.

### El ejemplo de Eduardo tiene un dato concreto detrás

Su propuesta —*amortiguar con buffer propio o trabajar con streams*— apunta a un
sitio que ya se puede señalar: **la caché de littlefs es de 256 B**
(`BPVM_FS_LFS_CACHE`, `src/fs_lfs.c:71`), la misma en host y en los tres micros,
con **dos buffers estáticos** para dos ficheros abiertos a la vez. O sea que hoy
una escritura pequeña desde BP **paga estructura cada 256 bytes**, y el sector de
borrado es de **4 KB** (8 KB en STM32): entre 16 y 32 veces más grande que la
caché. Ahí hay reparto que medir antes de tocar nada.

⚠️ **Y el orden importa**: primero la batería, después la optimización. Lo
contrario es exactamente lo que la casa no hace — hoy mismo el `-O0` apareció
porque había una **medida** que no cuadraba, no porque a nadie se le ocurriera
mirar los flags.

**Emparenta con** la tarea #373 (rendimiento del GUI: *medir antes de tocar los
eventos*), que es un caso particular de esto mismo, y con la hipótesis de la
flash interna de la fila c1, que sigue sin cronometrar por no tener justamente
esta batería.

### El mapa va ANTES que las cosas grandes — Eduardo, 5-ago

> «No es cuestión de optimizar por optimizar. Pero **ganancias de 300 % o 400 %
> no son despreciables**. Y antes de meternos con cosas más complicadas como
> trabajar con varios núcleos, una VM más eficiente o compilar a nativo,
> **conviene hacer un mapa**. Ver cuáles son nuestras fortalezas y debilidades.»

O sea que la batería **no es un fin, es el paso previo** a las tres piezas
gordas que ya están en la lista —SMP real en placa, intérprete más rápido, y
[[v6-nativo-y-profiler]]—. Y el orden protege de lo caro: **cualquiera de las
tres cuesta meses**, y elegir la equivocada sin mapa se paga entero. Hoy mismo
ha habido un ensayo de lo que evita: el `-Os` dio **3,65×** y costó una opción,
y sólo apareció porque un número no cuadraba.

La escala que pone Eduardo —**3× o 4×**— también fija el listón de qué se
persigue: no se trata de arañar un 5 %.

## Diagnóstico del heap DESDE BP — Eduardo, 5-ago

> «Para el tema del heap necesitamos herramientas para diagnosticar. Creo que ya
> tienes algunas hechas, así que creo que lo mejor es que **se puedan llamar
> desde un programa BP**. Las preguntas a contestar pueden ser: ¿hay algún handle
> de memoria degradado?, ¿la memoria está muy fragmentada?, etc.»

**Y tiene razón en las dos mitades: las herramientas existen, y están en el lado
equivocado.** Censo de lo que hay hoy:

| herramienta | dónde vive | ¿la ve un programa BP? | ¿en placa? |
|---|---|---|---|
| `heapFrag` / `heapMap` (builtins 121/122) | **sólo VM-Java** | sí | ❌ **no** |
| línea del GC (huecos, el mayor, fusiones, `de_lista`/`de_bump`/`astilla`) | VM-C | no, va a consola/log | sí, pero sólo mirando |
| guardián de fin de RUN (#339) | VM-C | no | sí, sólo al terminar |
| `MemInfo.bp` | BP, **por fuerza bruta** | sí | sí |

El agujero salta a la vista: **lo único que un programa BP puede preguntar hoy
no existe en la VM que corre en las placas.** `docs/BUILTINS.md:266` lo dice sin
rodeos —*«Java-only (diagnóstico) … ❌ intencional»*— y en su día fue una
decisión razonable; hoy es justo la que estorba, porque el bicho que hay que
cazar (24/34) **sólo sale en placa**.

**Y `MemInfo.bp` es la prueba de que falta la API.** Mide el heap *reservando
hasta que falla*, porque no hay forma de preguntar. Lo pagó caro: **dos versiones
midieron mal** antes de acertar, y las dos por la misma razón —**la medida
perturbaba lo que medía**—. Con una API que conteste, `MemInfo` son tres líneas
y no puede mentir.

### Lo que la VM ya sabe y no cuenta

No hay que inventar métricas: están calculadas y se tiran a una línea de texto.

- **Fragmentación**: nº de huecos, bytes libres totales y **el mayor hueco** →
  el índice sale solo, y el peor caso es *«hay 200 KB libres y el mayor bloque
  son 4»*.
- **Por qué se fragmenta**: `de_lista` / `de_bump` / **`astilla`** (`heap.c:500`)
  —reservas servidas de la lista, del bump, y **bloques rechazados porque el
  sobrante no era representable**—. Ese tercero es el que delata la degradación
  si «muerde» mucho.
- **Fusiones**: `fusion_izq` / `fusion_der`.
- **Handles**: la tabla tiene *slots*, crece por demanda (`0 -> 4096`) y cada
  entrada lleva **generación** — o sea que *«¿hay algún handle degradado?»* es
  contestable: slots en uso, slots nunca devueltos, generación máxima.

### Forma que debería tener

- **Módulo de stdlib con intrínsecos**, como `Math` e `IO` (#25/#26/#27). Clase
  OO si lleva estado, funciones sueltas si no — norma de la casa: [[bp-object-model-norms]].
- **En las DOS VMs**, o no sirve: la de placa es la VM-C. Esto **retira** la
  excepción de `BUILTINS.md:266`.
- **Barato y sin perturbar**: leer contadores, no recorrer el heap. La lección de
  `MemInfo` es que un instrumento que reserva memoria para medir memoria miente.
- **Números, no una cadena bonita**: `heapFrag` devuelve *string*; para que un
  programa **decida** (o un test falle) hacen falta enteros.

📌 **Y esto no es sólo comodidad: es el instrumento que hoy nos ha faltado.** El
paso «acorralar» del 24/34 pide preguntar *«¿cuántos slots de handle hay en uso?»*
al final de cada RUN, y hoy **no hay forma de preguntarlo desde un programa**.
Con esto, el test fusionado se contesta a sí mismo.

### La anomalía de los eventos: la placa MÁS LENTA va MÁS RÁPIDA — Eduardo, 5-ago

Observada durante la escalera de GUI de la c2, y anotada como lo que es: **una
anomalía sin medir**, para la batería de #376.

**El criterio que sí se cumple, y es el que manda para V4**: *«lo importante es
que para un ser humano los eventos van bien en esta placa»*. Eso es lo que la
tanda comprueba y eso pasa. Lo de abajo es curiosidad de ingeniería, no un
pendiente de publicación.

**La anomalía**: el STM32 va a **160 MHz** y el P4 a **360**; por ciclo el STM32
es un 31 % mejor (2.271 vs 2.976 ciclos/iteración) pero en reloj de pared el P4
debería ganar ~1,7×. Y sin embargo los eventos **se sienten más rápidos en el
STM32**. Es **la misma forma que la hipótesis de la flash** de por la mañana:
cuando la máquina más lenta gana, **la CPU no puede ser la explicación**.

**Hipótesis descartadas ya, y con qué:**

| candidato | lo mata |
|---|---|
| el transporte | **115200 en las tres familias** (`board.h`, `main.c:153`, `wire_v1.c:22`) — es una **constante** del parque |
| el área de pantalla | **contraejemplo de Eduardo**: el P4 vertical tiene **menos** píxeles y va **más lento** |
| la CPU | la placa más lenta gana |

**Y el control que lo estrecha de verdad, que lo aportó Eduardo**: en la
**Nucleo**, *sin pantalla y sin táctil*, la **tanda 5 de eventos salió verde** —
o sea que **el mecanismo de despacho está limpio** (cola + inyección del frame
entre quanta, núcleo portable). Luego lo que difiere **no está en los eventos**:
está en la capa de GUI/táctil que va encima.

**Lo que queda por mirar**, cuando toque:

- **cadencia de sondeo del táctil** (controlador y bus distintos por placa);
- **cada cuánto se bombea LVGL** — el periodo de refresco pone el **suelo** de
  latencia de evento, y no depende ni de píxeles ni de MHz;
- **el camino de volcado**: en la c2 es `memcpy` fila a fila (`gui_display_ltdc.c:88`),
  sin DMA; en el P4 es otro camino entero.

⚠️ **Y cómo hay que medirlo**: lo que Eduardo observa es *evento + print + wire*,
y el wire son 115200 fijos. O sea que **la tasa real de eventos es MAYOR que la
percibida** — su frase: *«sin el transporte sería mayor»*. Un cronómetro honesto
tiene que medir **dentro de la placa**, no contando líneas en la consola.


---

## #378 — Que cada micro DIGA lo que tiene (capa HAL BP de capacidades)

*Diseño de Eduardo, 6-ago-2026, a raíz del hallazgo 32 de H13.*

**El síntoma que lo destapó** es pequeño y tonto: el INFO del STM32 publica
`gpioCount:114` y el backend del lenguaje contesta `128`. Dos números a mano en
dos ficheros para el mismo hecho. Pero el arreglo evidente —tachar uno— es el
malo, y por eso esto es una tarea de V5 y no un parche de H13:

> «STM fabrica la misma CPU con diferentes encapsulados, diferentes pins pero
> también **diferentes periféricos**. Si queremos una imagen única para la
> familia tenemos que soportar el más grande y el más pequeño.»

O sea: **el número correcto no existe en tiempo de compilación**. Una imagen por
familia sirve a placas que no tienen el mismo pinado *ni los mismos periféricos*,
así que hornear una constante es equivocarse en todas las placas menos una.

**La forma que pide Eduardo:**

1. **Preguntarle al fabricante.** Primer paso de la tarea: *ver qué hay a nivel
   de HAL de ST* (y del SDK del RP2350 y del IDF) para consultar en runtime el
   pinado y los periféricos presentes — registros de identificación del device,
   tablas de la HAL, lo que haya.
2. **Una capa HAL BP de capacidades que tire de eso.** Igual que
   [[hal-bp-capa-comun]]: el verbo es común a las cuatro cinturas y la respuesta
   la da el micro. `GPIO_COUNT`, `ADC_CHANNELS`, `PWM_SLICES`… dejan de ser
   `#define` y pasan a ser una pregunta.
3. **El INFO y el lenguaje beben de la MISMA fuente.** Hoy son dos strings
   independientes; ahí es donde nace la divergencia. Emparenta con #371
   (librería de placa genérica: *el micro da el dato, la librería hace de
   puente*) — es la misma idea aplicada a las capacidades.

**Qué NO hay que hacer**: unificar los dos números a mano. Deja las dos fuentes
vivas y sólo tapa el síntoma en la placa que tengas delante.

**Estado en V4**: se queda como está, **a propósito**. Criterio de Eduardo: *«es
informativo, no afecta para el resto de funcionalidades»* → no es un bug que
bloquee la publicación. La divergencia queda **anotada en el código**
(`stm32/port/stm32_repl.c`, junto al string del INFO) para que nadie la
«arregle» por el camino corto.

**Un aviso sobre el número**: en el STM32 los pines se numeran `puerto*16+bit` y
son **dispersos** — ni 114 ni 128 permiten recorrer los pines de la placa con un
bucle. Lo que hace falta no es un contador mejor: es **poder preguntar si un pin
concreto existe**. Tenerlo en cuenta al diseñar la capa.


---

## #379 — El wire se queda desincronizado tras el Stop (y **sólo en unas placas**)

*Caracterizado por Eduardo entre el 4 y el 6-ago-2026, durante H13.*

**El síntoma, en sus palabras**: el KILL para el programa —eso funciona, es lo
que prometía #257— pero *«tras el Stop hay que cerrar la comunicación y
reconectar, y a veces hay que hacerlo una segunda vez»*.

**El dato que lo convierte en investigable** (6-ago): *«con las STM32 el stop
funcionó perfectamente, en cambio con las P4 iba a veces»*. **Misma versión, mismo
IDE, mismo día.** Eso descarta de golpe las dos mitades que uno miraría primero:

| candidato | por qué queda descartado |
|---|---|
| el drenaje del `connect` en el IDE | `BpvmClient.connectSerial` + `doHandshake` **no tienen ninguna rama por familia**: si fuera eso, la Nucleo fallaría igual |
| el KILL / fin de RUN | núcleo portable, idéntico en las tres familias; y la secuencia que emite la placa (`KILL_REPLY` → `EXITED`) está ordenada y verificada en el código |

**Lo que queda, que es por donde hay que empezar**: lo que de verdad cambia entre
una Discovery y un P4 — **el transporte y quién lo bufferiza**. La STM32 habla
por el VCP del ST-LINK (UART real a 115200); el P4 por su puente USB-UART. Un
programa que muere de golpe puede dejar mucho más texto en vuelo en un caso que
en el otro.

**Por dónde entrar**, en orden:

1. **Reproducirlo a propósito**: un programa que imprima sin parar y matarlo. La
   hipótesis a batir es que **el fallo escala con el volumen pendiente de salida**
   en el momento del KILL. Si es así, se reproduce a voluntad y deja de ser
   «a veces».
2. **Mirar el lado del micro**: qué pasa con la cola de salida (`oq_*`) y con el
   buffer del puente cuando la VM muere — ¿se descarta, se vacía, se queda a
   medias una línea?
3. **Sólo entonces, el lado del PC.** Y ahí hay dos cosas que están mal *aunque
   no sean la causa*, y conviene arreglar igual:
   - el drenaje inicial termina **por tiempo** (300 ms fijos, `BpvmClient:232`),
     no por sincronización: nada garantiza que el lector arranque en un límite de
     línea. En un protocolo de líneas, eso es un invariante que hoy no se cumple.
   - **`doHandshake` se traga su propio fallo**: si el HELLO da timeout, sólo
     escribe una traza de diagnóstico y el `connect` «tiene éxito» igualmente
     (`BpvmClient:378-380`). Un handshake que falla en silencio es justo lo que
     hace que el usuario tenga que adivinar que hay que reconectar otra vez.
     Esto es [[instrumento-mudo-dudar-de-el]] en el sitio menos oportuno.

⚠️ **Lo que NO hay que hacer, que es lo que estuve a punto de hacer yo**: tocar el
drenaje del `connect` sin repro. Ya se intentó una vez —un drenaje adaptativo—
y hubo que revertirlo por retrasos de ~10 s en el primer `LIST` (#154, el motivo
está escrito en el propio comentario del código). Es un sitio que ya ha mordido.

**Criterio de Eduardo para V4**: *«eso es mejor que tener que resetear»* →
**molesto, no bloqueante**. No bloquea la publicación.
