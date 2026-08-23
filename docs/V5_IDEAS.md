# V5 — ideas y diseño

Documento hermano de `V3_IDEAS.md` y `V4_IDEAS.md`: aquí se registran las charlas
de diseño de V5 **antes** de escribir código, que es el método de la casa.

**V5 se publicó el 22-ago-2026.** Este documento queda como ARCHIVO: es el registro
de cómo se decidió cada cosa, no una lista de trabajo. Fusionado el 23-ago con la
sección que vivía suelta en `docs/` (el pack nativo y su relocalización), para que
haya UN solo `V5_IDEAS`.

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

**⚠️ Y NO es un problema del STM32 — corrección del 6-ago.** Yo escribí esta ficha
como si fuera cosa de la familia STM32. Salió en el arranque de la Metro que el
**RP2350 tiene la misma forma**, sólo que en **unidades** en vez de en valores:

| fuente | dice | qué es |
|---|---|---|
| panel de INFO | `PWM 24` | **salidas** (12 slices × 2 canales A/B) |
| log de arranque | `pwm=12` | **slices** |
| `Pico.PWM_SLICES()` desde BP | `12` | slices |

Aquí **ninguno de los tres miente** —está decidido y explicado en
`pico/repl_v1.c:718-722`, y el campo del wire conserva el nombre histórico
`pwmSlices` aunque lleve canales—. Pero para quien mira, el efecto es idéntico al
del STM32: **el panel dice una cosa y el programa dice otra**, y no hay forma de
saber que son unidades distintas.

O sea que lo que pide #378 no es sólo *una* fuente de datos: es que la fuente
diga también **en qué unidad** habla, y que el nombre del campo del wire no
mienta sobre su contenido. Si al hacer la capa se arregla el valor pero se deja
el nombre `pwmSlices` llevando canales, la trampa sigue puesta.

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

---

## Golden de emisión V4 → V5 — «que el compilador no cambie sin que nos enteremos»

**Idea de Eduardo, 6-ago-2026** (justo después de publicar V4): *«en V4 hemos
cambiado el formato de los `.mod`; en V5 la idea es mantenerlo, así que podemos
volver a comparar los `.mod` generados en V5 con los de V4. Es un mecanismo
adicional de seguridad»*.

### Esto ya existió, y lo apagamos a propósito

`compat/compat.sh` verificaba **tres frentes** contra los goldens de V2:

| frente | qué comprobaba |
|---|---|
| comportamiento | la salida de cada `.mod` de V2 no cambia en las VMs nuevas |
| opcodes | los ids de opcode no se mueven (Java y C) |
| **emisión** | **el frontend nuevo emite `.mod` byte-idénticos a los de V2** |

El tercero es exactamente la idea. `check` está **desactivado desde el 17-jul**
por decisión de Eduardo, con el motivo escrito en la cabecera del script: V4
estaba rompiendo el formato **a propósito**, y un arnés que grita por un cambio
deliberado no es una red, es ruido. La cápsula no se borró.

Así que esto no es construir infraestructura: es **capturar una cápsula V4** y
volver a encender `check` con otro criterio.

### Lo que aporta, y que hoy NO tenemos

La paridad dual-VM compara **dos VMs ejecutando el mismo `.mod`**. Si mañana el
*compilador* empieza a emitir otra cosa, las dos VMs se ponen de acuerdo sobre el
bytecode nuevo y **la paridad sigue verde**. La red de hoy vigila a las VMs;
nadie vigila al emisor. El golden tapa ese agujero, y sólo ese.

### Medido antes de diseñar

- **La emisión es determinista**: `Threads`, `TryCatch` y `Strings` compilados dos
  veces dan ficheros byte-idénticos (6-ago). Sin esto, el arnés daría falsas
  alarmas y no valdría para nada.
- **La cápsula sale gratis y es la buena**: la de V2 hubo que archivarla a mano;
  la de V4 **es el artefacto publicado** (`BpIde-4.0.jar` de la release v4.0). El
  patrón de oro es, literalmente, lo que entregamos.

### El criterio: no es byte-idéntico, es GRADUADO

Criterio de Eduardo: *«si la herramienta es flexible, mejor; lo importante es que
detecte los errores de compilación»*. Y con dos ejemplos que fijan la escala:

> *«si en los `.mod` añadimos un nuevo bloque, no debería producirse un error, con
> un warning vale ("nuevo bloque encontrado"). En cambio si en el código aparece un
> opCode desconocido ahí debería saltar alguna alarma (podría ser correcto o no,
> pero habría que investigar)»*.

**Consecuencia técnica, y es la que decide la implementación: esto NO se puede
hacer comparando bytes.** Un `cmp` sólo sabe decir «difieren en el byte 1428».
Para distinguir *bloque nuevo* de *opcode desconocido* hay que **entender** el
`.mod`. O sea: el arnés compara **desensamblados**, no ficheros.

Y las tres piezas ya están hechas:
- `bpgenvm --disasm` da la vista estructurada: cabecera, bloques con nombre y
  tamaño, imports/exports, data y código.
- Ya imprime **`??? opcode desconocido`** (`tools/Disasm.java:336`) — el aviso que
  pide Eduardo existe literalmente.
- La cabecera ya trae el **veredicto de ABI** (gate de #284): un `.mod` de formato
  anterior se rechaza explicando por qué.

**Escala de gravedad propuesta** — el criterio no es «han cambiado bytes», sino
**«¿esto rompe el `.mod` de otro?»**:

| qué cambia | veredicto |
|---|---|
| nada (mismo hash) | silencio — camino rápido, ni se desensambla |
| aparece un **bloque nuevo** que el lector no conoce | **aviso**: «nuevo bloque encontrado» — evolución aditiva, es lo esperado |
| tamaños/offsets de bloques conocidos | informativo — consecuencia normal de recompilar |
| **opcode desconocido en el código** | 🔔 **ALARMA** — puede ser correcto o no, pero hay que mirarlo |
| un opcode conocido **cambia de id** | 🔔 **ALARMA** — rompe todos los `.mod` ya publicados |
| desaparece un **export** que existía | 🔔 **ALARMA** — rompe a quien lo importa |
| cambia **magic/versión** de formato | 🔔 **ALARMA** — es cambio de ABI, y entonces toca re-sellar la cápsula a conciencia |
| cambia la **salida de ejecución** | ❌ **ERROR** — eso ya no es formato, es comportamiento (frente 1 del arnés viejo, sigue siendo fallo duro) |

Regla que ata todo: **una diferencia es una alarma que hay que explicar, no una
prohibición**. Si el cambio es deliberado se re-sella el golden *con su motivo
escrito*; si nadie sabe de dónde sale, es un bug. Mismo espíritu que
«no se publica con un bug conocido»: no es *no cambies*, es *no cambies sin
enterarte*.

### Cobertura

Los goldens de V2 son **17 módulos**; hoy hay ~260 samples. La red vale lo que
cubre → la captura debe ser ancha desde el primer día, no 17 por inercia.

### Estado

**Diseño acordado, sin implementar.** Pendiente de arrancar V5.

---

## SQLite — sesión del 7-ago: la prueba 0, las medidas, y el diseño que salió de ellas

Todo lo de abajo está **medido**, con las opciones exactas del pipeline
(`-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -fno-jump-tables -Os`).

### PRUEBA 0 — CONTESTADA, y sin necesidad de placa: sale que NO

Un `.c` de veinte líneas con las seis cosas que SQLite tiene seguro (literal, tabla
estática de punteros, estático mutable, división, `memcpy`, `float` y `double`), por el
camino `.mdn`:

```
R_ARM_ABS32      f_inc / f_dob        ← direcciones ABSOLUTAS (la tabla de punteros)
R_ARM_REL32      .data.rel.ro / .bss  ← secciones que MdnPack NI COPIA
R_ARM_THM_CALL   memcpy, __aeabi_*    ← símbolos externos que nadie resuelve
```

**Y `MdnPack` lo empaquetó con código de salida 0.** Un `.mdn` de 232 bytes
perfectamente roto: sólo viajaron los 176 bytes de `.text`, y el código que viaja
referencia datos que se quedaron en tierra.

**Hallazgo que va más allá de SQLite, y afecta a V4 ya publicado:** el único guardián de
`MdnPack` es de **nombres** (`thunk_<Módulo>_*`) — y ése funciona bien, falla en voz alta
y lista lo que encontró. Pero **no hay ningún guardián de corrección**. La disciplina que
hace que el AOT funcione está escrita en un comentario del código generado —*«no
referencia símbolos del runtime por nombre → el .o con -fpic es 100% relocatable»*— y
**nada la comprueba**. Si el emisor emitiera alguna vez un literal o una tabla `const`,
saldría un `.mdn` mudo y roto.

→ **Acción barata pendiente**: que `MdnPack` se niegue ante `ABS32` o símbolos sin
resolver. Unas líneas, y protege el pipeline actual.

### SQLite medido de verdad

Compila **limpio** para cortex-m33 en 17 s, cero avisos. La fama de portable la tiene
ganada.

**Símbolos sin resolver: 37.** Y `float` de 32 bits en su API pública: **cero** —
todo es `double`, y REAL es de 64 bits *en el formato de fichero*
(«big-endian IEEE 754-2008 64-bit»), no por decisión de compilación.

| grupo | nº | nota |
|---|---|---|
| runtime del compilador (`__aeabi_*`) | 18 | `double` + división de 64b. **No es dependencia de SQLite**: es del objetivo |
| libc `mem*` / `str*` | 12 | |
| alocador (`malloc`/`free`/`realloc`) | 3 | redirigibles (MEMSYS5) |
| `localtime` | 1 | |
| `sqlite3_os_init` / `_end` | 2 | **nuestro punto de extensión**, no dependencia |
| `_GLOBAL_OFFSET_TABLE_` | 1 | artefacto de `-fpic` |

**Variables externas: CERO** — la intuición de Eduardo era correcta.

**23 de los 37 YA están en `bpvm_pico.elf`.** El hueco real son **10 helpers de `double`
+ `localtime`**, todos de libgcc/newlib. La superficie de dependencias **no es el
problema**.

**Relocalizaciones y tamaños:**

| | con `-fpic` | **sin `-fpic`** |
|---|---|---|
| tipos distintos | 6 | **3** |
| `ABS32` | 979 | 2590 |
| `THM_CALL` / `JUMP24` | 1740 / 139 | 1631 / 139 |
| GOT (`GOT_BREL`+`BASE_PREL`) | 35 | **0** |
| `.text` | 362.652 | 355.000 |
| `.rodata` (+cadenas) | 46 KB | 50.387 → **flash** |
| **RAM (`.data`+`.bss`)** | 9.860 | **6.929 ≈ 6,8 KB** |

### La arquitectura que salió de ahí

**Decisión de Eduardo: SQLite NO va en la imagen.** *«Esto va a equipos que van a
utilizar BD y equipos que no… La Pico no tiene PSRAM y la Metro puede tenerla o no. El
micro y la imagen son la misma, pero en la que no lleva PSRAM no voy a meter una BD.»*

**Y la idea que lo desbloquea, también suya: reubicar en el PC, justo antes de grabar el
pack.** El dispositivo le dice al IDE dónde va a caer.

Consecuencias, en cadena:

1. **Si fijamos la dirección, sobra `-fpic`.** Y quitarlo elimina la GOT y baja de 6
   tipos de relocalización a 3.
2. **Se enlaza en 0 con el enlazador de verdad** (`ld -Ttext=0 --emit-relocs`). El
   enlazador resuelve *para siempre* las llamadas y saltos internos —son relativos al
   PC—, y `--emit-relocs` conserva la lista de lo que queda.
3. **Lo único que queda por tocar son los `ABS32`**, y como están enlazados en base 0,
   cada palabra contiene ya su offset: **realojar = sumar una constante**. El «motor de
   reubicación» se queda en *recorrer una lista y sumar*. Dos bases (flash para código,
   RAM para datos), así que cada entrada lleva un bit diciendo cuál.
4. **Los símbolos externos NO se parchean.** Si horneáramos la dirección de `memcpy` del
   firmware, una actualización de firmware rompería todos los packs grabados. En su
   lugar, un `bios_shim.c` **dentro del pack** los define como envoltorios que llaman por
   la tabla (`g_bios->memcpy(...)`). Tras enlazar: **cero símbolos sin resolver**, y el
   pack sólo necesita **un puntero** al arrancar.
5. **El enlazador de verdad como ORÁCULO** (idea de Eduardo): al principio se usa `ld`, y
   cuando escribamos el nuestro se compara contra él. Es la jugada de la paridad dual-VM
   aplicada aquí — enlazar en 0 y parchear a X debe dar un binario **byte-idéntico** a
   enlazar directamente en X.

**El flujo, tal como lo describió Eduardo:** nosotros pre-montamos el pack con direcciones
falsas (0) → el IDE lo lee, ve que hay que recolocar → pide las direcciones a la placa →
reempaqueta con el código realojado → **eso** es lo que se graba.

→ **El pack que distribuimos es por ARQUITECTURA, no por dispositivo.** Lo específico de
la placa lo pone el IDE al grabar, sumando dos constantes.

### La RAM: dos problemas distintos

**(1) Los 6,8 KB estáticos** — el problema es la *dirección*, no el tamaño.

**Solución (idea de Eduardo): una clave en el ENTORNO.** `SQLite=1` y el sistema reserva
al arrancar; `0` o ausente, no reserva nada. Encaja porque:

- `bpvm_env_get_long()` **ya existe** → leerla es una llamada;
- el entorno vive en su propio bloque de flash y **se lee en el arranque de kernel, antes
  de que la VM reparta su memoria** → la dirección es determinista y el IDE la calcula;
- hay precedente del patrón: el panel del P4 sale del ENV (#311).

*Refinamiento propuesto*: que **el valor sea el tamaño** (`SQLite=64` → 64 KB). Cuesta lo
mismo y hace que el tamaño de la arena también sea decisión del usuario. Un solo bloque
contiguo cubre las dos RAM: `[ datos estáticos ~7 KB | arena ]`.

⚠️ **Dos guardianes obligatorios:**
- **Clamp**: el número lo escribe el usuario; `SQLite=250` en una Pico no puede
  estrangular la VM. Validar y acotar, y reportar el reparto en el arranque (como el
  tamaño de flash en #292).
- **Sello**: cambiar el ENV mueve el bloque e invalida el pack pre-enlazado. El pack lleva
  **grabada la dirección para la que se enlazó** y el cargador se niega diciendo qué hacer
  («vuelve a grabarlo desde el IDE»). Patrón #284 con disparador concreto.

**Propiedad bonita**: es **reversible sin reflashear**. `SQLite=0`, reinicio, y la RAM
vuelve entera.

**(2) La arena** — la gorda. Tres palancas: `MEMSYS5` sobre buffer fijo (todo el consumo
pasa a ser **un número que elegimos**, y el OOM es limpio en vez de comerse el heap de la
VM — protege lo de #357); **página de 512 B** en vez de 4096 (palanca de 8×, y casa con el
sector de la SD); y **la arena en PSRAM cuando la haya**, con negativa clara si no cabe →
«¿esta placa puede con una BD?» pasa a ser respuesta de **tiempo de ejecución**.

### Lo que la placa tiene que decirle al IDE (por INFO)

- **La dirección de flash que ve la CPU** (`XIP_BASE + partición + offset del pack`),
  ⚠️ **no** el offset crudo — sólo la placa conoce ese mapeo, y equivocarse ahí desplaza
  todo por una constante en silencio.
- El **espacio disponible**, para negarse antes de compilar 400 KB que no caben.
- **Base y tamaño del bloque de RAM** reservado por el ENV.
- Y el sello: **arquitectura + convención de coma flotante** (`arm` a secas no distingue
  `hard` de `softfp`).

*Consecuencia de flujo*: construir un pack así **exige placa conectada**. El enlace ocurre
en el despliegue, no antes. Y lo limpio es que el IDE pida **reservar** un hueco de N
bytes y la placa conteste con la dirección, en vez de preguntar «¿dónde iría?» y grabar
después.

### Decisión sobre coma flotante

**Los `float` se quedan** (Eduardo): *«si se va a utilizar una BD en un micro, la mayoría
de los casos será para registrar datos, y ahí hacen falta»*.

Matices medidos: la elección **no tiene la forma que tiene en MicroPython** —allí el
intérprete decide el ancho al construir la imagen; aquí REAL es de 64 bits *en el
disco*—. Y elegir `float` en nuestra API **no ahorra ni uno** de los 18 helpers: SQLite
calcula en `double` por dentro pase lo que pase. Sí ahorra **en el lado BP**, donde
`float` es hardware y `double` software.

Como **BP tiene los dos tipos** (a diferencia de MicroPython), no hace falta elegir
globalmente: `getFloat` y `getDouble`, y que decida el programa. Se empieza por
`getFloat`, sin diseñar el `double` fuera — el dato de 64 bits está en el disco y tirarlo
en la frontera sería perder precisión en silencio.

### PLAN DE PRUEBAS REVISADO

Lo de ayer cambia bastante: la 0 está contestada, la 1 ya no procede en su forma
original (el bloqueo no era del ISA), y aparece un bloque nuevo —el flujo de
pre-enlazado— que se verifica **entero en el PC**.

| # | prueba | dónde | qué decide |
|---|---|---|---|
| ~~0~~ | ~~C ajeno por el camino .mdn~~ | — | ✅ **CONTESTADA: no, y en silencio** |
| A | `ld -Ttext=0 --emit-relocs` sobre SQLite + `bios_shim.c` → **cero símbolos sin resolver** | PC | si el pre-enlazado cierra la superficie externa |
| B | **oráculo**: parchear-desde-0 vs enlazar-en-X → ¿byte-idéntico? | PC | si nuestro «sumar una constante» es correcto |
| C | ídem A y B en **RISC-V** | PC | si los tipos de relocalización cambian el motor |
| D | VFS mínimo sobre `stdio` + `OS_OTHER=1`, abrir/escribir/leer una BD | PC | que el `sqlite3_os_init` que escribamos basta |
| E | **suelo de la arena**: `MEMSYS5` sobre buffer que encoge hasta fallar | PC | **qué placas entran** |
| F | 1 conexión, 2 `SELECT` alternando `step()` (+ `INSERT` en otra tabla y en la misma) | PC | `Connection`+`Statement` o conexiones: **son dos APIs** |
| G | puente BP → C ajeno **con un GC en medio** | PC | si #302 paso 3 nos alcanza |
| H | reserva por ENV + INFO con las dos direcciones + sello y clamp | placa | que el andamiaje del despliegue funciona |
| I | el pack de verdad, grabado y ejecutando | placa | fin del camino feliz |
| J | *bring-up de SD* → rendimiento y **corte de alimentación ×100** | placa + SD | la robustez que motivaba todo |

De la A a la G **no hace falta placa**. Ése es el gran cambio del día: el diseño nuevo
mueve casi toda la verificación al PC, donde iterar cuesta segundos.

**Estado: sin implementar.**

### ¿Cómo sabe el IDE que hay código que reubicar, y de qué arquitectura?

Pregunta de Eduardo (7-ago). Contestada mirando el formato que **ya existe**, no inventando
uno — y la mitad estaba puesta.

**Ya resuelto en V4:**

- **«¿hay nativo dentro?»** → el índice del pack lo dice. Cabecera de entrada de 48 B con
  `tipo(4)` = FourCC = la extensión. Y el nativo **ya viaja en packs hoy**:
  `PackStep.OUTDIR_TYPES = {"mod","mdn"}`. El IDE recorre cabeceras; no abre payloads.
- **«¿ARM o RISC-V?»** → el `.mdn` **ya lleva `arch`** (= `e_machine` del ELF, 40/243),
  escrito por `MdnPack.java:132`, y `mdn_loader.c` **ya se niega**:
  *«MDN: arch mismatch .mdn=%u firmware=%u — RECHAZADO»* (`MDN_ERR_ARCH`).
- **El manifest admite campos nuevos sin romper nada**: `bpvm_pack_manifest_get()` no
  parsea, **busca una clave** barriendo líneas, y **salta las que no conoce** → añadir
  campos no rompe ningún firmware ya grabado. (Y un pack **sin** manifest es válido: es
  una librería.)

**Lo que destapó la pregunta — hay DOS artefactos, no uno:**

La tabla de relocalización **la consume el PC**; el dispositivo no la necesita jamás.

| | contenido | lo lee |
|---|---|---|
| el que **distribuimos** | código enlazado en 0 + **tabla de relocs** + lo que exige | el IDE |
| el que **se graba** | código **ya realojado** + el sello | la placa |

Son ~10 KB que no tienen por qué ocupar flash del micro. **El IDE no firma el pack: lo
transforma.**

**Y de ahí sale la respuesta sin gastar nada nuevo** (regla de Eduardo: si ya hay algo,
el azúcar cuelga de ahí): **la tabla de relocs es una entrada más**, `tipo="rel"`, nombre =
el `.mdn` al que pertenece.

- «¿hay que reubicar?» ≡ «¿hay entradas `rel`?» — lo dice el índice, gratis.
- El IDE las consume y **no las copia** al pack grabado → **la marca desaparece justo
  cuando deja de ser verdad**. Un pack grabado no tiene `rel`: reubicar dos veces es
  imposible por construcción, no por disciplina.

**Al manifest sólo va lo que el índice NO puede decir:**

```
main=...                 ← ya existe
target=arm-m33-softfp    ← sello real (ver abajo)
ram=7168                 ← estáticos que exige reservados
arena=65536              ← arena que pide
linked=0                 ← base para la que está enlazado
```

Al grabar, el IDE reescribe la última: `linked=flash:0x10180000,ram:0x20038000`.
**Ese campo es a la vez el sello**: el cargador lo compara y se niega si el ENV cambió y
movió el bloque.

**El detalle que `arch` NO cubre:** `EM_ARM` **no distingue `softfp` de `hard`**. Hoy no
muerde porque todos los `.mdn` los construye la misma cadena con banderas fijas — pero un
pack **distribuido por separado** pierde esa garantía, y una discrepancia de ABI **no da
error de enlace: da números mal en silencio**. De ahí `target=` en el manifest, en vez de
fiarse del `e_machine`.

**Y un cambio de sitio de la negativa:** hoy vive en la **placa**. Para un pack
distribuible tiene que estar **en el PC, antes de grabar** — rechazar después gasta un
ciclo de flash y deja un pack muerto en el micro. La de la placa se queda de red, no de
primera línea.

### ✅ PRUEBAS A y B — el pre-enlazado FUNCIONA (7-ago, en el PC, sin placa)

**PRUEBA A — ¿cierra la superficie externa? SÍ, a cero.**

SQLite compilado sin `-fpic` deja **36** símbolos sin resolver (uno menos que con
`-fpic`: desaparece `_GLOBAL_OFFSET_TABLE_`). Y el reparto es limpio: **18 + 18**.

⚠️ **Los 18 `__aeabi_*` NO se pueden envolver.** `__aeabi_ldivmod` y `__aeabi_uldivmod`
devuelven un **par en r0:r1**, y eso no se expresa en C. Un envoltorio ahí daría números
mal en silencio. Van enlazados **de libgcc**, no por la tabla. (Y no son dependencia de
SQLite: son del objetivo.)

Los otros 18 sí: `bios_shim.c` los define como envoltorios que llaman por `g_bios`.
⚠️ Se compila con **`-fno-builtin`** o GCC convierte el cuerpo de `memcpy` en una llamada
a `memcpy` — recursión infinita. Medido: con la bandera, el shim **no deja nada abierto**.

```
ld -Ttext=0 -Tdata=0x20038000 --emit-relocs  sqlite3.o bios_shim.o libgcc.a
→ símbolos sin resolver: CERO
```

**El pack necesita UN puntero.** `g_bios` aparece 16 veces y vive en `.bss`. Eso es todo
lo que el cargador tiene que rellenar.

*Dato de propina que confirma lo del sello:* la ruta de libgcc es
`.../thumb/v8-m.main+fp/**softfp**/libgcc.a`. **El propio toolchain guarda una libgcc por
ABI de coma flotante.** `EM_ARM` no distingue eso → `target=` en el manifest es necesario,
no decorativo.

**PRUEBA B — ¿realojar es sumar una constante? SÍ, byte a byte.**

Los mismos objetos enlazados dos veces y comparados con el enlazador de juez:

| | flash 409.695 B | `.data` 6.264 B |
|---|---|---|
| ABS32 con delta-flash | 2.108 | 406 |
| ABS32 con delta-RAM | 165 | 7 |
| resultado | **IDÉNTICO** | **IDÉNTICO** |

**Total a parchear: 2.686 ABS32** (2.514 a flash, 172 a RAM). Y el resultado es
**byte-idéntico** a lo que saca `ld` enlazando directamente en esa dirección — **moviendo
las DOS bases a la vez**.

⚠️ **La primera versión de la prueba tenía un agujero mío**: movía la flash pero dejaba la
RAM quieta, así que de los 172 punteros a RAM no probaba nada. Un tercer enlace con las dos
bases distintas lo cerró. (Lección [[n-casos-del-mismo-sample-no-son-n-casos]]: probar dos
secciones con el MISMO delta no son dos casos.)

**Dos trampas medidas por el camino:**

1. **`--emit-relocs` conserva el registro, no una lista de pendientes.** Salen 9.930
   `THM_CALL` y 690 `JUMP24` — y **ya están resueltos**: son relativos al PC y se mueven
   con la imagen. Los únicos accionables son los ABS32.
2. **La 5ª columna de `readelf -r` es el NOMBRE del símbolo, no su sección.** Sólo pone
   `.text`/`.bss` cuando el destino es anónimo; para `g_bios` o `.LC42` pone su nombre.
   Clasificar por nombre falla en 282 de 2.273 — y el fallo es *silencioso salvo que se
   compare byte a byte*. **Se clasifica por el VALOR** (4ª columna).
   → Y ese valor **es** el bit por entrada del diseño: lo da el enlazador ya masticado.

**Números para el pack:**

| | |
|---|---|
| flash (`.text` 360.140 + `.rodata` 49.551 + `.data` inicial 6.264) | **≈ 416 KB** |
| RAM (`.data` 6.264 + `.bss` 812) | **≈ 6,9 KB** |
| tabla de relocs (2.686 offsets × 4 B) | ≈ 10,7 KB — **NO viaja a la placa** |

**Estado: A y B CERRADAS. Siguiente: C (RISC-V) y D (el VFS).**

### ✅ PRUEBAS D y F — SQLite FUNCIONA sobre nuestro VFS (7-ago, host)

**PRUEBA D — ¿basta el `sqlite3_os_init` que escribamos? SÍ.**

Con `SQLITE_OS_OTHER=1` SQLite no trae **ninguna** capa de sistema: la pone entera quien
integra. Medido: **son ~200 líneas**, compila sin un aviso, y funciona a la primera.

Lo que hay que rellenar, y nada más:

| | |
|---|---|
| `sqlite3_io_methods` (v1) | 12 funciones: close/read/write/truncate/sync/fileSize + 4 de bloqueo + sectorSize/devChar |
| `sqlite3_vfs` (v1) | 8: open/delete/access/fullPathname/randomness/sleep/currentTime/getLastError |
| `sqlite3_os_init/end` | registrar el VFS. Dos líneas. |

Resultado con 500 filas `INTEGER` + `REAL` + `TEXT`, **cerrando y REABRIENDO**:

```
fase 2: 500 filas leidas tras reabrir
        error maximo en REAL : 0        <- los double vuelven EXACTOS
        texto intacto        : si
fase 3: count=500 sum=187250.00 avg=374.5000 min=0.25 max=748.75
fichero: 20480 bytes
```

⚠️ **Reabrir es lo que prueba el VFS.** Sin cerrar, la primera pasada sale verde aunque
las escrituras no aterricen: se lee de la caché de páginas. Mismo patrón que
[[instrumento-mudo-dudar-de-el]].

**Simplificaciones que se sostienen en NUESTRO caso** (y por qué):
- **bloqueo = no-op**: la VM ejecuta UN programa; no hay otro proceso abriendo la BD. En
  el micro es literalmente imposible.
- **sin dlopen** (`OMIT_LOAD_EXTENSION`), **sin temporales en disco** (`TEMP_STORE=3`).
- **`mxPathname`=512 y sin directorios**: el FS del micro es plano.

⚠️ **Trampa que SQLite exige y es fácil saltarse**: en una lectura corta hay que **rellenar
el resto a cero** y devolver `SQLITE_IOERR_SHORT_READ`. Si se devuelve OK con basura
detrás, la corrupción aparece mucho más tarde y en otro sitio.

**Tres puntos donde la versión de placa cambia (y son cintura, no diseño):**
`xRandomness` → HAL de aleatorios (#347) · `xCurrentTime` → `Rtc` · read/write → fachada
del FS (littlefs). **`xSectorSize` = 512** ya casa con el sector de la SD.

---

**PRUEBA F — sentencias por conexión: SE PUEDE. La API es `Connection` + `Statement`.**

Eduardo: *«Juraría que en SQLite eso no se puede hacer… Igual estoy equivocado y el límite
es del puente Java-SQLite, no te lo puedo asegurar.»* → **era del puente.** Es la regla de
JDBC (un `ResultSet` por `Statement`), no de SQLite. Medido, con UNA conexión:

| caso | |
|---|---|
| dos `SELECT` alternando `step()` | ✅ 10 y 10 filas, valores correctos |
| `SELECT` a medias + `INSERT` en **otra** tabla | ✅ |
| `SELECT` a medias + `INSERT` en **la misma** tabla | ✅ sin error |
| el `SELECT` sigue recorriendo tras escribir | ✅ 19 filas más |
| `SELECT` a medias + `UPDATE` | ✅ |
| **sentencias vivas a la vez** | ✅ **64**, sin error |

⚠️ **Lo que esta prueba NO demuestra**: si una fila insertada a mitad de recorrido aparece
o no en él. No apareció, pero la consulta llevaba `ORDER BY` y SQLite probablemente ordenó
por delante (el recorrido iba sobre el ordenador, no sobre la tabla). Que **no da error**
está medido; la visibilidad, no. Si la API va a permitirlo, que la documentación diga lo
mismo que dice la de SQLite.

**Consecuencia para el diseño:** el límite de 4 conexiones es de **CONCURRENCIA** (hilos),
no de "consultas abiertas". Un programa de un solo hilo puede tener todas las consultas
vivas que quiera con **una** conexión. El array de referencias del puente que propuso
Eduardo sigue valiendo, pero dimensionado por sentencias, no por conexiones.

**Estado: D y F CERRADAS. Quedan E (suelo de la arena) y G (el GC en medio).**

### ✅ PRUEBA E — la memoria: acotada, y con UNA perilla

**Decisión de Eduardo (7-ago) que reencuadra la prueba:** *«Para ser realistas, puedes
hacer pruebas con un Heap de 8M, 4M como poco. Lo indicamos como requisito para utilizar
la BD. Una PSRAM de 8M vale 1.5$.»*

→ **PSRAM es requisito para la BD.** Eso tira a la basura media prueba (buscar el mínimo,
pelearse con la banda patológica de MEMSYS5, la palanca del tamaño de página) porque era
optimizar para un caso que no se soporta. La pregunta buena pasa a ser **¿se queda
acotado?**

**Antes de nada, una aclaración que yo tenía mal contada.** SQLite no tiene tres cajones
de memoria, tiene **dos**:

| | | ¿varía? |
|---|---|---|
| **(a) estático** — `.data`+`.bss` | **7.073 B** con MEMSYS5 / **6.929 B** sin él | **NO.** Es de tiempo de enlace. Ni con la consulta, ni con las filas, ni con la BD |
| **(b) todo lo demás** | por **un solo alocador** | sí, y es lo que se mide abajo |

No hay búferes internos fijos ni pools estáticos: caché de páginas, sentencias, ordenadores
y cadenas salen todos del mismo sitio. Así que **la «arena» ES la memoria dinámica** — no
son dos cosas. Y por tanto la idea de Eduardo (`SQLITE_CONFIG_MALLOC` → nuestro heap) es
un camino soportado, no un apaño: MEMSYS5 sólo aporta **144 bytes** de estático.

**Medido con nuestro propio alocador instrumentado, página 4096:**

| filas | abrir | insert | scan | sort (`TEMP_STORE=3`) | sort (`TEMP_STORE=1`) |
|---|---|---|---|---|---|
| 2.000 | 153K | 153K | 153K | 215K | 249K |
| 10.000 | 153K | 352K | 352K | 672K | 1.138K |
| 50.000 | 153K | 1.178K | 1.178K | 2.777K | 2.750K |
| 200.000 | 153K | **1.199K** | 1.199K | **7.577K** ↗ | **2.750K** ← saturado |

**Los cuatro hallazgos:**

1. **`abrir` es CONSTANTE (153 KB).** No crece con nada.
2. **`insert` y `scan` SATURAN** en el tamaño de caché que le pongamos. No crecen con los
   datos: entre 50.000 y 200.000 filas apenas se mueven.
3. 🔴 **`ORDER BY` sin índice crece LINEAL Y SIN TECHO... por culpa de `TEMP_STORE=3`.**
   Yo lo puse para no tener que dar ficheros temporales en el VFS, y eso **obliga** al
   ordenador a quedarse en RAM: 7,6 MB con 200.000 filas, y subiendo. Con `TEMP_STORE=1`
   vuelca a disco y **se satura en 2,75 MB**. De propina, las asignaciones bajan de
   404.614 a 4.678 — mucho menos trasiego para el heap.
4. **Es UNA sola perilla.** Con 200.000 filas y vuelco a disco, variando sólo `cache_size`:

   | caché | insert | sort | mayor petición |
   |---|---|---|---|
   | 1024 pág (4 MB) | 4.560K | 10.829K | 4.194.304 |
   | 256 pág (1 MB) | 1.199K | 2.750K | 1.048.576 |
   | 64 pág | 361K | 1.887K | 1.024.000 |
   | 16 pág | 173K | **1.699K** | 1.024.000 |

**Conclusión:** con 4–8 MB de PSRAM sobra. Un ajuste sensato (caché 1 MB) da **2,75 MB de
pico** con 200.000 filas, y **no crece con los datos**. El suelo de esa carga son 1,7 MB.

⚠️ **Y una advertencia que sí importa: la petición más grande.** Con temporales en RAM era
constante (87.360 B). Con vuelco a disco, el ordenador pide **1 MB de una vez** y no baja
de ahí. Nuestro heap tiene que poder servir **1 MB contiguo** aunque lleve rato
funcionando → esto es territorio [[v4-heap-estable-357]] y del muro de contención (#358).
**Verificar en placa, no dar por hecho.**

**Dos consecuencias de diseño:**

- 🔴 **`TEMP_STORE` NO puede ser 3.** El VFS **tiene** que dar ficheros temporales. Y en el
  micro eso significa escribir: en flash es desgaste, así que **la SD deja de ser un extra
  y pasa a ser el sitio natural de los temporales**. Enlaza con la prueba J.
- **`cache_size` lo ponemos nosotros**, no se deja por defecto (el defecto son −2000, o sea
  2000 **KiB** — ojo, se expresa en kibibytes, no en páginas, y por eso cambiar el tamaño
  de página no mueve el presupuesto). Es la única perilla que hace falta exponer.

**Descartado por la decisión de PSRAM (queda anotado por si algún día importa):** MEMSYS5
sobre arena pequeña tiene un paisaje de fallo **dentado y no monótono** — medido,
determinista: con 88 KB falla en el primer `PRAGMA`, y con 84 KB llega hasta el final. Las
potencias de dos se portan bien. Si alguna vez se quiere BD sin PSRAM, **el tamaño de
arena hay que medirlo, no calcularlo**.

**Estado: E CERRADA. Paquete A/B/D/E/F completo, todo en el PC.**

### Memoria: PSRAM como requisito, y arena propia (7-ago, cerrado con Eduardo)

**Decisión: PSRAM es requisito para la BD.** *«Una PSRAM de 8M vale 1.5$.»*

**Aclaración importante — el reparto de RAM ya está resuelto en V4.** Las pilas BP y el
heap BP viven LOS DOS dentro de `s_vm_buffer` (25/75, `repl_v1.c:115-123`), y ese bloque
entero se va a PSRAM cuando la hay: `main.c:1062` lo dice en el log —
*«heap en PSRAM 8 MB @ 0x11000000 (SRAM interna sin reservar)»*. Así que en una placa con
PSRAM **la SRAM interna ya está libre**, y los ~7 KB estáticos de SQLite son ruido.
→ El ENV vuelve a ser un simple **`SQLite=1`**, sin tamaño.

**Susto descartado (y por buen motivo):** la PSRAM se ve por la ventana XIP, y
`psram.c:10` avisa de que *«el direct-mode del QMI SUSPENDE el XIP»*. ¿Colisiona entonces
SQLite-en-PSRAM con escribir la BD en flash? **No**: los búferes de littlefs son TODOS
estáticos en SRAM (`s_read_buf`, `s_prog_buf`, `s_fbuf_a/b`, *«cero malloc»*), así que
`flash_range_program` siempre lee de SRAM. **El rebote ya existe por diseño** y SQLite lo
hereda gratis.

**¿SQLite libera bien? MEDIDO, no supuesto** (mismo instrumento que #339):

```
8 ciclos x 5000 filas, incluyendo caminos de ERROR a proposito
ciclo 1..8 -> en uso = 0 B, bloques vivos = 0, pico = 581.760 B (IDENTICO los 8)
tras sqlite3_shutdown(): 0 B, 0 bloques
```

Cero residuo **y pico plano** — lo segundo es lo que descarta la acumulación lenta, que un
solo ciclo no vería ([[test-bugs-probabilisticos]]). **No necesita GC**: libera a mano y lo
hace bien.

**Arena propia vs. nuestro heap BP → la ARENA es la opción SIMPLE.** Contraintuitivo, pero
es por la forma de los dos alocadores:

| | |
|---|---|
| nuestro heap | `bpvm_heap_alloc(vm, size, BPVM_TYPE_*)` → devuelve un **handle**, exige **tipo**, y lo alocado está **sujeto al GC** |
| lo que SQLite quiere | `void* malloc(int)`: puntero crudo, sin tipo, que nadie recoge |

Casarlos exigiría tipo falso + raíz de GC permanente por bloque + un GC paseándose por
miles de bloques que no sabe interpretar. La arena es **una llamada**:
`sqlite3_config(SQLITE_CONFIG_HEAP, trozo, tam, 16)` — y MEMSYS5 **ya viene en la
amalgamación** (bandera de compilación; no escribimos alocador).

Y las dos medidas encajan: como SQLite **devuelve todo y su pico es plano**, una arena fija
**nunca se va arriba**. Se dimensiona una vez y aguanta. Además los deja **aislados**: un
OOM de SQLite es un error con nombre y dueño, no un arrastre del programa BP.

### La RAM de la BD sale del ENV, y son 2 MB (7-ago, cerrado)

**Idea de Eduardo:** *«una variable de entorno que indique la RAM de trabajo de la BD. Se
reserva al arrancar y nosotros no la tocamos nunca.»* → sustituye al `SQLite=1`: un solo
número, y de ese bloque salen también los ~7 KB estáticos. `[estáticos | arena]`.

**Primer intento con arena fija: 4 MB, y con un ACANTILADO.** Medido, 200.000 filas,
`TEMP_STORE=1`:

| arena | filas que aguanta |
|---|---|
| 1 MB | 5.000 |
| 2 MB | 5.000 |
| 4 MB | 500.000 |

Que 1M y 2M den EXACTAMENTE lo mismo es la pista: no faltaba memoria, **no cabía un
bloque**. Con 4M/cache64 el pico en uso era 1,57 MB — cabía de sobra en 2 MB. Falla porque
MEMSYS5 es *buddy* y no podía servir **1 MB contiguo**.

**Eduardo: «4M es mucho para un micro; con una PSRAM de 8M es la mitad».** Y tenía razón,
porque ese 1 MB **no es una constante de la naturaleza**:

```c
#ifndef SQLITE_SORTER_PMASZ
# define SQLITE_SORTER_PMASZ 250      /* paginas */
#endif
```

250 × 4096 = **1.024.000** = exactamente la mayor petición medida. Y hay perilla **en
tiempo de ejecución**: `SQLITE_CONFIG_PMASZ` (opción 25) → no hay que recompilar por placa.

**Bajándola, el suelo se mueve:**

| arena | PMASZ 250 (1 MB) | 128 (512 K) | 64 (256 K) | 32 | 16 |
|---|---|---|---|---|---|
| 1 MB | ✗ | ✗ | ✗ | ✗ | ✗ |
| **2 MB** | ✗ | ✅ | ✅ | ✅ | ✅ |
| 4 MB | ✅ | ✅ | ✅ | ✅ | ✅ |

Y con **PMASZ=64** la capacidad se iguala: **2M, 4M y 8M aguantan 1.000.000 de filas**
(tope del barrido, no techo medido). **El tamaño de la arena nunca fue la restricción.**

**Números finales:** pico con 2M/PMASZ64 = **867 KB** = 42 % de la arena. La proporción se
repite en todas las medidas → **regla práctica: con MEMSYS5 cuenta con usar ~40 % de lo
que reserves.** Por debajo de PMASZ=64 no se gana nada (867.104 es el suelo): **64 es el
punto dulce**. Y **1 MB no llega ni a tope**, así que **2 MB es el suelo real**.

**El diseño, cerrado:**
- **`SQLite=<MB>` en el ENV. Mínimo 2.** Reservado en el arranque, intocable.
- **UN número, dos perillas DERIVADAS**: el firmware calcula `cache_size` y `PMASZ` de la
  arena. Exponer las tres es pedir una combinación contradictoria — medido: 4M+cache256
  **falla** y 4M+cache64 **va**.
- **Clamp con negativa clara** (#292): por debajo de 2M, *«SQLite necesita 2M, no se
  activa»*. Mejor un no limpio que arrancar y morir a las 6.000 filas.

⚠️ **NO medido: el coste en TIEMPO.** Un PMA menor = más volcados y más pasadas de mezcla
→ más lento y más escrituras a la SD. Que **quepa** está medido; que sea **rápido**, no.
Va a la prueba J.

### La prueba G, bien definida — y dónde vive el puente (7-ago)

**Pregunta de Eduardo:** *«si asignamos un bloque fijo de la PSRAM y lo gestiona SQLite el
GC no interviene. Supongo que estás hablando de las consultas y resultados, no?»* → **Sí.**
El GC **no ve la arena de SQLite en absoluto**; ésa es una ventaja de haberlas separado. La
G no va de la memoria de SQLite: va de **la frontera**.

Los dos momentos de riesgo, y sólo esos:
- `db.query("SELECT ...")` — la cadena SQL es objeto BP y hay que darle sus bytes a SQLite.
- `stmt.getString(0)` — SQLite devuelve puntero a SU memoria y el puente fabrica una cadena
  BP ⇒ **aloca en el heap BP** ⇒ **puede disparar un GC** con otras referencias BP vivas en
  locales de C, que el GC no ve.

**No es problema nuevo: la regla ya está escrita en casa.** `builtins.c:388-397` (arreglo de
#350): *«Con `peek` el argumento sigue por debajo de `sp`, o sea que sigue siendo raíz. Se
saca AL FINAL… Todo builtin que alogue y siga leyendo de un argumento tiene que usar esto.»*
Y con su medida: *«3.000 vueltas con el heap estrecho daban entre 3 y 6 resultados malos, y
subían al encoger el heap.»*

→ **G se REBAJA**: de pregunta abierta a **punto de checklist + prueba de esfuerzo**, y la
forma de la prueba ya la fijó #350 (bucle largo, heap estrecho, contar resultados malos).

**Decisión que salió al mirarlo — DÓNDE VIVE EL PUENTE:**

| | |
|---|---|
| dentro del pack | toca objetos BP ⇒ necesita `peek_ref` y las primitivas de raíces **exportadas por la BIOS**. La disciplina del GC se mete en el pack |
| **en el firmware**, y el pack exporta **sólo la API C de SQLite** | el puente usa `peek_ref` como cualquier builtin de casa y **la disciplina del GC nunca cruza la frontera** |

**Se elige la segunda.** Encaja con lo medido (el pack necesita **un solo puntero**,
`g_bios`, y su tabla no crece con conceptos de la VM) y deja el pack **reutilizable**: el
mismo binario sirve para cualquiera que quiera SQLite, sin saber que BP existe.

### ✅ PRUEBA G — contestada por la FORMA de la API (sin montar puente sintético)

**El patrón de recursos nativos YA existe en casa: `Net.Tcp`.** `bpvm_net_connect` devuelve
un **`int`**; `send`/`recv`/`close` lo toman (`builtins.c:757-812`). El socket vive en una
tabla del backend y **BP sostiene un entero, no una referencia**. El GC no ve nada nativo.
→ Es exactamente lo que propuso Eduardo el primer día: *«un pequeño array de referencias en
el puente»*. No hay que inventar nada.

**Con eso se disuelve el susto del FINALIZADOR** (que era real: un `Statement` envuelve un
`sqlite3_stmt*` que hay que finalizar, y la VM **no tiene finalizadores** — comprobado, no
hay ningún `destructor`/`on_collect`/hook de GC). Como `Connection` y `Statement` guardan un
**entero**, no hay envoltorio que el GC pueda llevarse dejando el recurso colgado. Queda el
olvido de `close()`, pero con **tabla acotada** eso es un **error limpio al agotarse**, no
crecimiento sin control — misma contención que hoy con TCP. **Nada nuevo que construir.**

**Y el riesgo de GC en la frontera NO SE DA, por la forma de la API.** Hace falta que
coincidan **DOS** cosas: una referencia BP viva **Y** una asignación en el heap BP.

| función del puente | ¿ref BP? | ¿aloca? | riesgo |
|---|---|---|---|
| `prepare(sql)` | sí | no (SQLite copia) | no |
| `bindString(i,s)` | sí | no (SQLite copia) | no |
| `step()` | no | no | no |
| `getString(i)` | no (enteros) | **sí** | **no** — no hay refs vivas |

No se juntan porque **los identificadores son enteros** y los datos viajan en una dirección
o en la otra, nunca en las dos a la vez. (Contraste: `TCP_RECV` hace `bpvm_heap_alloc` y
tampoco tiene refs vivas — misma forma.)

⚠️ **Dónde SÍ aparecería:** un atajo de conveniencia tipo `queryOne(sql) → string`, que toma
referencia **y** aloca. De ahí la regla, que cabe en una línea y es auditable de un vistazo:

> **Toda función del puente con un parámetro por REFERENCIA que además ALOQUE usa
> `peek_ref` y saca el argumento AL FINAL** (`builtins.c:388-397`, arreglo de #350).

**Honestidad sobre el método:** esto es razonamiento sobre la FORMA de la API, no medida. La
medida es la prueba de esfuerzo de #350 (bucle largo + heap estrecho + contar resultados
malos), y **tiene sentido cuando exista el puente de verdad** — no contra un puente
sintético montado para probar un riesgo que la forma descarta.

**Estado: G contestada. El paquete del PC queda COMPLETO (A/B/D/E/F/G). Todo lo que falta
es de placa: H (reserva+INFO+sello), I (el pack grabado), J (SD + corte de luz).**

### ✅ H (1ª mitad) VERIFICADA EN PLACA — Metro RP2350B, 7-ago-2026

```
[0] vm: heap en PSRAM 8 MB @ 0x11000000       <- sin la clave en el ENV
...
[0] vm: heap en PSRAM 6 MB @ 0x11200000       <- con SQLite=2
[0] bd: reservada (SQLite=2) -> 2048 KB @ 0x11000000
```

**Los DOS arranques en el MISMO log**, consecutivos: mismo firmware, lo único que cambió
entre ellos es una clave de texto. No hace falta comparar contra el recuerdo de otra
sesión — el control y el caso están uno debajo del otro.

Aritmética exacta: `0x11000000 + 0x200000 = 0x11200000`, 8 − 2 = 6 MB. El bloque se queda
en la **base de la ventana** ⇒ su dirección NO depende de cuánta PSRAM lleve la placa, que
es lo que el IDE necesita para pre-enlazar.

Y esos dos arranques **demuestran algo que sólo era afirmación de diseño**: es
**reversible sin reflashear**. Se quita la clave, se reinicia, y la RAM vuelve entera.

⚠️ **DOS INSTRUMENTOS MUDOS costaron media hora de diagnóstico equivocado** — y los dos
fallaron por lo mismo: **no distinguían firmware nuevo de viejo**.

1. **El panel INFO del IDE pinta una LISTA FIJA.** Los campos nuevos viajaban por el wire y
   nadie los mostraba. → Arreglado, y con criterio: la línea de la BD sale **aunque valga
   0** («no activada — falta SQLite=<MB>»). Un «no» explícito es una respuesta; el silencio
   no distingue nada.
2. 🔴 **El log NO es un anillo: se llena y PARA.** `bpvm_log.c:27` escribe `[LOG OVERFLOW]`
   y devuelve; `bpvm_log_init` recupera lo de flash y sigue desde ahí. Grabar firmware **no
   lo borra**. El log estaba lleno desde el 3-ago ⇒ **los arranques de hoy no escribieron
   NADA**, y yo leí el del 3 como si fuera fresco.

   **La pista estaba en los datos de Eduardo y se me pasó**: el log llegaba a 87 s de
   uptime y el INFO decía 39 s. Dos arranques distintos, delante de las narices.

   → Se arregla con `LOG clear` por wire, pero **eso no es la solución**: el log saturado
   **no avisa**. El `[LOG OVERFLOW]` queda al final de decenas de KB que nadie lee entero.

**Deuda de instrumento anotada (NO es de SQLite):**
- Un log lleno debe anunciarse **al principio**, o el arranque debe decir cuánto le queda.
- El banner de arranque debería llevar algo más que `__DATE__`: con el log limpio pero
  firmware viejo, no hay forma rápida de saberlo salvo recordar cuándo se compiló.

**Queda de la H:** el clamp (`SQLite=1` → negativa en voz alta), los campos del INFO
(necesita reconstruir el jar del IDE — ⚠️ con el IDE CERRADO) y el sello del pack.

### ✅ H CERRADA — los tres casos verificados en placa (Metro, 7-ago)

| ENV | heap | INFO |
|---|---|---|
| `SQLite=2` | 5,5 MB | `2.0 MB @ 0x11000000` |
| `SQLite=1` | **7,5 MB intacto** | *«SQLite=1 es MENOS del mínimo (2 MB) → no se activa. Pon SQLite=2 o más y reinicia»* |
| sin clave | 7,5 MB | *«no activada — añade SQLite=2 al entorno de la placa y reinicia»* |

**Decisión de Eduardo: el aviso vive en el INFO** — *«es algo que el usuario puede
consultar fácilmente, y arreglar con facilidad»*. Descartó el asistente en el IDE:
*«a mí me gustan las soluciones sencillas»*. Y es MEJOR que avisar al escribir la clave:
el INFO es de **consulta** y refleja el estado ACTUAL; un aviso al teclear sólo aparece en
ese instante y se pierde.

**Y ya viaja lo que el IDE necesita para pre-enlazar:**
```
Zona packs  : 4.2 MB @ 0x10BC4000  (direccion que ve la CPU, NO el offset crudo)
Coma flot.  : softfp
```
Cuadra: 0xBC4000 ≈ 11,8 MB + 4,2 de zona = los 16 MB de flash. La zona se lleva la cola.

**Detalles de implementación que costaron y conviene no repetir:**
- El **motivo** viaja aparte del resultado. Con `SQLite=1` los bytes son 0 **igual que si la
  clave no estuviera**, y son cosas distintas: sin el motivo, el aviso habría dicho «falta
  SQLite=<MB>» con la clave puesta delante. **Un aviso que miente es peor que no tenerlo.**
- El **mínimo lo dice la placa** (`sqliteMinMb`), no está copiado en el Java. Si algún día
  se mide que baja de 2, cambia en un sitio ([[v4-orden-trabajo]] #335).
- **Código del wire ≠ prosa del log**: el log dice «por debajo del minimo» para un humano,
  el wire manda `"low"` para el IDE. Mezclarlos ataría la redacción del log a la
  compatibilidad del protocolo con firmwares ya grabados.

### 🔴 DEUDA ANOTADA: el buffer del wire desborda EN SILENCIO

`s_reply_buf[1024]` (`pico/repl_v1.c:69`) lo comparten TODAS las respuestas. Cada
`wire_v1_field_*` devuelve −1 al no caber, y como cada llamada va guardada por
`if (off >= 0)`, **a partir de ahí todas se saltan sin decir nada**: sale un mensaje bien
formado pero CORTO, y el firmware cree que ha respondido.

⚠️ **Hoy NO fue la causa** (era el jar viejo en memoria) — pero el mecanismo es real y el
`LIST` con muchos ficheros comparte el mismo buffer. **No medido cuánto margen queda.**

**Arreglo propuesto, en dos partes:** agrandar el buffer *y* —lo importante— **que el
desbordamiento GRITE**: si `off` acaba negativo, responder "truncado" en vez de fingir que
está completo. Aplazado por decisión de Eduardo (7-ago): *«estamos en fase de pruebas,
todavía no estamos construyendo la versión final; con apuntar y que no se pierdan los
detalles es suficiente»*.

**Patrón del día, y van CUATRO**: una capa intermedia que descarta en silencio lo que no
conoce o no le cabe — el panel del IDE con lista fija · el log saturado · el jar viejo en
memoria · este buffer. Todos costaron diagnóstico. [[instrumento-mudo-dudar-de-el]]

### ✅ BIOS del pack nativo — montada y VERIFICADA EN PLACA (7-ago)

```
bd: reservada (SQLite=2) -> 2048 KB @ 0x11000000
bios: lista (17 ranuras, v1)          <- nueva
boot: estado 3 (app)
```
Otra vez los DOS arranques en el mismo log (12:11 sin BIOS / 12:33 con ella): la única
diferencia es la línea nueva, y sigue llegando a estado 3 con los mismos tiempos.

**Requisito de Eduardo que dio forma al diseño (7-ago):** *«hay que prever que algo puede
fallar, así que hay que poner chivatos que nos digan qué es lo que no funciona, para saber
dónde y no sólo funciona/no funciona»*.

**Tres decisiones que salen de ahí, y ninguna es cosmética:**

1. 🔑 **`log` es la ranura NÚMERO UNO, y se reporta antes que cualquier otra.** Sin ella el
   pack es **mudo por construcción** y todo lo que le ocurra dentro se reduce a "va / no
   va". El test lo fija: con `log` Y `memcpy` ausentes, el que se nombra es **`log`**.

2. **Las 4 ranuras que aún no existen NO están a NULL: GRITAN.** `malloc`/`free`/`realloc`
   /`localtime` apuntan a stubs que escriben en el log y devuelven el fallo.
   ⚠️ La alternativa tentadora —apuntarlas al heap del sistema— habría "funcionado" con el
   pack **comiendo del heap de FreeRTOS en silencio**, justo lo que la arena separada
   existe para evitar. Así, el día que el pack pida memoria por un camino no previsto se
   sabe **en el instante**, no como corrupción semanas después.

3. **Se verifica en CADA arranque, haya pack o no.** Un hueco que sólo aparece el día que
   grabas un pack te pilla ya depurando otra cosa: cuesta el doble.

**Y la lista de ranuras vive en UN array `{offset, nombre}`, no en 17 `if`.** Escrita a
mano, el día que alguien añada un campo y olvide su comprobación ese hueco vuelve a ser
mudo — exactamente lo que esto existe para evitar.

Las 17 no están inventadas: son las que **midió la prueba A** (16 de libc + `log`). Los 18
`__aeabi_*` NO están porque no pueden ir por tabla (`__aeabi_ldivmod` devuelve un par en
r0:r1; en C no se expresa) — se enlazan de libgcc dentro del pack.

**`test-bios` 10/10**, y el firmware la verifica al arrancar.

⚠️ **Dos tropiezos del día, los dos cazados por el compilador en segundos:**
- `mem*/str*` dentro de un comentario de bloque: el `*/` de en medio **cierra el comentario**
  y el resto del texto pasa a ser código.
- Una guarda de script que buscaba `int main` — aquí la entrada es `vm_task`, así que se
  saltó la inserción **informando de éxito**. Tercer automatismo del día que dice "hecho"
  sin hacer nada. Compilar cada tanda es lo que los caza.

**Siguiente: el CARGADOR de packs** — escalera de 7 peldaños (encontrar · sello · arch+ABI ·
dirección · BIOS · rellenar `g_bios` · saltar), cada uno con su motivo. Los 6 primeros
fallan limpio; el 7º puede colgar, y para ése la **miga de pan**: escribir en el log ANTES
de saltar y el resultado DESPUÉS — si se cuelga, el siguiente arranque muestra el último
paso sin resultado y ahí murió (técnica de #326).

---

## V5/I · La escalera del cargador — implementada y en verde (7-ago)

`bpvm_npack.h/.c` + `test_npack.c`. **20/20 en verde.** La cabecera del pack y las 8 formas
de estar mal, cada una con **su** motivo y **su** remedio.

### La cabecera, y por qué el sello no es un lujo

```c
typedef struct { uint32_t magic; uint16_t format; uint16_t arch; char float_abi[8];
    uint32_t entry_off, flags, flash_bytes, data_bytes, bss_bytes,
             linked_flash, linked_ram, reloc_count, reserved[2]; } bpvm_npack_hdr_t;
```

`linked_flash`/`linked_ram` = **para qué direcciones se realojó**. Si no cuadran con dónde
está de verdad, el código lleva dentro punteros a otra parte y saltar es un desastre MUDO.
Y el caso que lo dispara es **normal, no exótico**: el usuario cambia `SQLite=<MB>` y el
bloque de RAM se mueve. Comparar dos enteros lo convierte en *«regrábalo desde el IDE»*.

`reloc_count == 0` **es** la prueba de que pasó por el IDE. No hay un campo `ya_realojado`
aparte que pueda contradecir a la tabla: el contador y el hecho son lo mismo.

### Por qué la ABI es un peldaño APARTE de `arch` — medido en este repo

No es teórico y no hace falta salir del proyecto para verlo:

| familia | núcleo | `-mfloat-abi=` |
|---|---|---|
| Pico / Metro (RP2350) | Cortex-M33 | **softfp** |
| STM32 U575 | Cortex-M33 | **hard** |

**Mismo núcleo, ABI distinta.** Un pack de una placa enlazaría contra la otra sin una sola
queja del enlazador y daría **números mal** — no un fallo, números mal. Por eso `arch` sola
no basta y la comparación de `float_abi` es exacta y su propio peldaño.

### El orden importa: lo barato y lo grave primero

Con el magic malo NO se mira `arch`: la cabecera podría ser basura y cualquier campo que se
lea de ella es ruido. El test lo fija con un caso *«todo mal a la vez»* que debe responder
**MAGIC**, no lo que se rompió después.

Y el **bit Thumb vive en UNA función** (`bpvm_npack_entry_addr`), no repartido: olvidarlo es
hard fault en la primera instrucción, y es justo el detalle que se copia mal en el segundo
sitio donde se escriba. (Ya mordió una vez: `nm` **enmascara** ese bit y decía "Thumb: no".)

### Los dos rojos del test eran del TEST, no del cargador

Vale la pena porque los dos enseñan algo distinto:

1. **«.data+.bss no caben»** — puse `bss = SITIO_RAM` esperando que no cupiera, pero
   `0 + 2 MB > 2 MB` es **falso**: cabe justo. Aritmética mía. Ahora hay **dos** casos, uno
   a cada lado del borde: exactamente la RAM → cabe; **un byte más** → TAMAÑO.

2. **«ABI vacía»** — 🔑 esperaba que una ABI vacía fuera rechazada. Pero en el PC
   `bpvm_mdn_host_float_abi()` devuelve `""` (x86 no tiene pack nativo que sellar), así que
   `""` **coincide** y pasa — correctamente. **El caso daba por supuesto que el test corría
   en ARM.** Sustituido por *«ABI parecida pero distinta»*, construida a partir de la del
   anfitrión, que vale en cualquier sitio.

   El hallazgo detrás: **en el PC ese peldaño no puede fallar nunca**. No es un agujero —en
   el PC no hay packs nativos— pero sí un recordatorio de que **un test verde en el host no
   prueba un peldaño cuyo dato sólo existe en la placa**.

### El alta en los 5 builds — y la trampa que ya había mordido

Registrado `src/bpvm_npack.c` en host + pico + esp32 + esp32p4, **verificado contando
apariciones**, no leyendo la salida del script.

Al contar apareció algo peor: en el STM32, un script de alta **anterior** había escrito
`\n` **literales** en `subdir.mk` — los tres ficheros (`bpvm_boot`, `bpvm_bios`,
`bpvm_sqlmem`) apelotonados en una línea, con `\nC:/...` como nombre de fichero. Dos de los
tres **no tenían regla de compilación**: esa familia no habría enlazado.

🔑 Pero la lección no es "arreglar el `subdir.mk`": **es un fichero GENERADO**. `src` es una
**carpeta enlazada** del `.project`, y `-cleanBuild` regenera los makefiles escaneándola. El
parche a mano ni hacía falta ni sobrevive. Un `cleanBuild` headless dejó los cuatro ficheros
cada uno en su línea, con su regla, **0 errores** — y recogió `bpvm_npack.c` **solo**.
Otra vez [[generado-parcheado-a-mano]]: tocar el generador, no el resultado.

**Compilado y enlazado esta tanda:** host (20/20) · Pico (`.elf` OK) · STM32 Nucleo
(0 errores) · ESP32-S3 y P4.

### El formato binario, y el ensayo general en el PC

La cabecera pasa a **64 B exactos** (`BPVM_NPACK_HDR_BYTES`) aunque los campos sumen 56.
No es relleno caprichoso: **la imagen de código empieza justo detrás**, así que ese número
ES la base del código. Redondo, alineado, y con dos huecos de reserva pagados por si algún
día hace falta un campo más sin mover nada.

De ahí sale la trampa que había que dejar escrita: `linked_flash` **no es la base del
pack**, es la del CÓDIGO — `pack_base + 64`. `entry_off` cuenta desde ahí. Confundirlos son
64 bytes de desfase y un salto a mitad de instrucción. Está dicho en el parámetro de
`bpvm_npack_check`, que es donde alguien lo va a leer.

**El contrato entre Python y C, fijado por los dos lados.** La cabecera la escribe el
empaquetador con una cadena de `struct` y la lee el cargador con una `struct` de C. Los
mismos bytes, dos lenguajes — y hasta ahora cada lado sólo se comprobaba **contra sí mismo**.
Ahora: `offsetof` campo a campo en el test de C (13 campos, y si uno se mueve dice CUÁL) y
un `assert` de tamaño en `pack.py`.

Pero eso sigue sin probar que **coincidan entre sí**. Para eso, `leer.c`: C abre el
`.npack` que escribió Python y lo decodifica. 🔑 **Y el control positivo es que la escalera
se PARA en ARCH** — el pack es ARM, el PC no. Si los campos fueran corridos diría MAGIC
(basura en el primer entero); llegar al peldaño 2 ya demuestra que `magic` y `format` se
leen bien. Los peldaños restantes se comprueban con una copia que lleva la arch/ABI de este
anfitrión, así que **sello, realojado, tamaños y entrada se validan con los bytes REALES**.

Salida: magic `BPNP` · formato 1 · arch 40 · ABI `softfp` · entrada +0 Thumb · 190 B de
código · sello `0x10BC4040` / `0x11000000` · relocs 0. Saltaría a **`0x10BC4041`**.

⚠️ **Y aquí cayó un número inventado.** La primera versión de `pack.py` puso `arch = 2`
"de los de siempre". El catálogo real es el `e_machine` del ELF (`src/mdn_format.h`):
**ARM = 40**, RISC-V = 243. Lo malo no es que fallara — es que habría fallado **con el
motivo correcto por la razón equivocada**: el chivato diría ARCH y uno se creería que el
pack está mal compilado, cuando el error estaba en la etiqueta. Los catálogos se leen de su
fuente, no se recuerdan.

### Estado al cerrar la tanda

Commiteado en `d0ae180` (las tres piezas juntas: memoria de la BD + BIOS + escalera).
Compilan las **5**: host · Pico · STM32 Nucleo · ESP32-S3 · ESP32-P4.

**Lo que queda para tener el pack corriendo en la Metro:**
1. El cargador de placa: encontrar el pack en la zona XIP, copiar `.data`, poner `.bss` a
   cero, rellenar la BIOS y **saltar** — con la miga de pan del #326 (escribir en el log
   ANTES y DESPUÉS: si se cuelga, el siguiente arranque enseña el paso sin resultado).
2. Grabar el `.npack` en la zona de packs (hoy `pack.py` lo deja en fichero).
3. Los pines de la SD, **en el ENV** (ver abajo).


---

## Corrección de Eduardo (7-ago): la config de placa va al ENV, no a board.json

*"Quedamos que trabajamos con el environment, /sys/board.json ya no aplica."*

Me lo había saltado: puse los pines del lector microSD del Metro en
`pico/boards/metro-rp2350b.json`, que **es la plantilla de `/sys/board.json`** — un
fichero que se sube al FS del dispositivo. Es el mismo camino que ya se abandonó en
[[placa-p4-pantalla-flexibilidad-hw]] con #311, cuando el panel del P4 pasó de board.json
al ENV (`display=st7701`). Revertido.

**Y no era sólo el sitio: también el momento.** `pico/boards/README.md` es documentación
del repo, o sea de lo PUBLICADO. Meter ahí hallazgos de la versión en curso es justo lo que
[[publicar-es-prometer]] dice que no se hace. Van aquí.

Salió barato porque **todavía no los leía nadie**: eran datos puestos por delante de su
lector, así que mover el sitio no rompe una línea de C.

### Por qué el ENV y no un fichero

El ENV es `clave=valor`, se parsea en el estado 0 **sin heap**, vive en flash con copias
A/B y CRC, y —lo que importa aquí— **una clave desconocida se ignora**: añadir entradas
nunca rompe a un kernel viejo. Un fichero en el FS necesita que el FS esté montado, que es
más tarde y más frágil. Para algo que describe la placa, cuanto más abajo mejor.

Entradas propuestas (sin sufijo `Pin`, como `display=`):

    sdBus=spi0   sdSck=34   sdMosi=35   sdMiso=36
    sdCs=39      sdDat1=37  sdDat2=38   sdDetect=40

### El lector microSD del Metro — del pinout de Adafruit, confirmado 7-ago

| señal            | GPIO | función del mux RP2350 |
|------------------|------|------------------------|
| SD_SCK / CLK     | 34   | `SPI0_SCLK`            |
| SD_MOSI / CMD    | 35   | `SPI0_TX`              |
| SD_MISO / DAT0   | 36   | `SPI0_RX`              |
| SDIO_DAT1        | 37   | —                      |
| SDIO_DAT2        | 38   | —                      |
| SD_CS / DAT3     | 39   | GPIO por software      |
| SD_CARD_DETECT   | 40   | —                      |

🔑 **El cableado permite los DOS modos y no estropea ninguno.** En SPI0 por hardware, las
tres señales que exigen función de mux caen justo donde deben (34/35/36), y el CS en 39 no
la necesita porque lo mueve el software. Y en SDIO de 4 bits, `DAT0..DAT3 = 36,37,38,39`
son **cuatro GPIO CONSECUTIVOS**, que es lo que una máquina PIO necesita para tratarlos
como un grupo con una sola instrucción. Se puede empezar por SPI sin cerrarse la puerta.

⚠️ **Al escribir el driver de 4 bits, los nombres engañan.** Las señales se nombran por su
papel en SPI, que es el modo con el que se empieza, pero en 4 bits significan otra cosa:

    sdMiso (36) ES DAT0        sdCs (39) ES DAT3

Un driver de 4 bits que busque "dat0"/"dat3" no los encontrará.

⚠️ **Y el PIO no llega gratis a estos pines.** En RP2350B cada instancia de PIO direcciona
32 pines: hay que mover su ventana con `pio_set_gpio_base(pio, 16)` para alcanzar el 34-40,
y ESE bloque deja entonces de ver los GPIO 0-15. Hay 3 bloques; se le dedica uno. Si
`PICO_PIO_USE_GPIO_BASE` no valiera 1, el 5º bit del número de pin se **ignora en
silencio** y GPIO34 se convertiría en GPIO2 — no da error, da el pin equivocado. Nuestro
build ya va contra `bp_rp2350b` (`PICO_RP2350A=0`), así que sale a 1, pero es de lo que hay
que mirar ANTES de depurar un driver que "no responde".

---

## ✅ V5/I PASO 0 CERRADO EN PLACA (7-ago): el nativo llama al firmware y vuelve

```
control OK: corre el cuerpo escrito a mano
buscando el ancla desde 268435456 en 1024 KB...
  -> encontrada en 268753984          (= 0x1004DC40, la del arranque)
  crc16 del vector estandar = 10673   esperado: 10673
  -> OK: codigo nativo llama al firmware por puntero y VUELVE.
```

Código nativo **ejecutándose desde la PSRAM** barre la flash, encuentra el ancla y llama a
través de ella a una función del firmware con **tres argumentos**, recogiendo el retorno.

🔑 **Que salga 10673 descarta cuatro cosas a la vez** —dirección equivocada, bit Thumb
perdido, argumentos desalineados, retorno mal recogido— porque ninguna de ellas produce
`0x29B1` por casualidad. Un "no se colgó" no habría descartado ninguna. **Elegir un objetivo
con respuesta conocida es lo que convierte la prueba en prueba.**

### El ancla — la idea de Eduardo, y el fallo que la provocó

*"La discrepancia puede ser de 1 byte o de 1K. Podemos incluir un texto constante en la
imagen, justo antes de la tabla. Luego que la función escanee la memoria un poco antes
buscando el texto."*

Antes de eso, perseguí la dirección de una función del firmware sacada de su `.elf`. **Falló
dos veces seguidas y de dos maneras distintas**, y las dos veces el número seguía pareciendo
bueno:

1. **Colgó la placa.** Se saltó a ciegas a una dirección que no era.
2. **La imagen grabada no era la del `.elf`** que usé para calcularla.

Y en medio, una tercera: `bpvm_pack_crc16` se movió de `0x10026E35` a `0x10026F01`
**sólo por recompilar**. Que es el problema de fondo: *cada enlace mueve todo*.

Con el ancla eso deja de importar. Los punteros los rellena el enlazador, así que siempre
son correctos para ESA imagen. Se puede reconstruir el firmware las veces que haga falta.

### Tres detalles del ancla que no son adorno

1. **La marca sola no basta**: también `version`, `bytes` y punteros no nulos. Ocho bytes se
   repiten por casualidad en un megabyte de código; los cuatro campos a la vez, no.
2. **La marca va carácter a carácter**, no como literal `"BPANCLA1"` — con un literal el
   compilador podría dejar una copia suelta en `.rodata` y habría DOS sitios donde la
   búsqueda la encuentra, uno sin nada útil detrás. Verificado sobre la imagen: **1 vez**.
3. **El ancla lleva una función de respuesta conocida.** Un pack puede comprobar que sabe
   llamar al firmware antes de fiarse de nada.

### Lo que costó, medido

- El barrido de 1 MB: **~20 ms** hasta encontrarla (está a 318 KB). El millón entero serían
  ~65 ms. No molesta en el arranque; recortable luego si hiciera falta.

### Dos trampas del instrumento, y cómo se cazaron

**El IDE regeneraba mi `.mdn`.** `runAotPass` sólo respeta el ajuste «AOT» **cuando hay
proyecto** — con el `.bp` suelto lo rehace igual, sustituyendo el cuerpo escrito a mano por
el generado (`return 0`). Eso habría parecido «el salto no funciona» cuando lo que pasaba es
que ya no medíamos lo nuestro. Lo caza el **control**: con base 0 mi cuerpo devuelve −1 y el
generado 0, así que el programa distingue quién corre antes de concluir nada.

**El banner del boot no identifica la imagen.** `main.c:1365` usa `__DATE__ __TIME__`, que se
resuelven **al compilar ESE fichero**. En un build incremental main.c no se recompila y el
banner se queda anclado a una hora vieja — y hay un segundo sello en `repl_v1.c` aún más
antiguo, que es el del INFO. Me llevó a diagnosticar «la imagen es vieja» sobre una premisa
falsa. **PENDIENTE: sellar en el ENLACE, no en la compilación de un `.c`.**

### Lo que sigue

El pack ya no necesita que nadie le diga dónde está la BIOS: la encuentra. Queda llevar el
`.npack` a la flash y saltar a él — pero el mecanismo de «código ajeno llama al firmware y
vuelve» ya está probado en placa, que era lo que daba miedo.

### Idea de Eduardo al cerrar la tanda: una marca TAMBIÉN en el pack

*"Visto el problema que hemos tenido con la dirección, tampoco estaría mal tener una marca
en el Pack."*

Simétrico del ancla: **el ancla arregla «el firmware se mueve», la marca del pack arregla
«el pack se mueve»**. Misma medicina, otro lado.

**Media parte ya está hecha:** `bpvm_npack_hdr_t` lleva `magic = 'BPNP'` en el offset 0. Lo
que falta no es la marca, es que el cargador **la busque** en vez de que le digan dónde. Hoy
el diseño asume «el pack está al principio de la partición de packs» — exactamente la clase
de suposición que nos mordió tres veces esta tarde.

**Encaja sin inventar nada.** El barrido produce candidatos y sobre cada uno se corre
`bpvm_npack_check`, que ya son siete peldaños (formato, arch, ABI, sello, realojado,
tamaños, BIOS). La regla de validación sigue en UN sitio; el barrido sólo la alimenta.

🔑 **Y el sello no sobra: cambia de papel.** Si el pack se encuentra barriendo, `aqui_flash`
es *donde ha aparecido*, y el sello comprueba que se realojó **para ahí**. Deja de servir
para encontrarlo y pasa a servir para confirmar que **no se ha movido desde que se grabó** —
que es justo el caso de «alguien cambió `SQLite=<MB>` y la RAM bailó».

**Sobre el ancho de la marca:** la del ancla son 8 bytes porque va suelta entre código. La
del pack son 4 ('BPNP'), y con la aritmética a la vista basta: barriendo 4 MB alineado a 4
hay ~1M de posiciones, y la probabilidad de que alguna cuadre por azar es ~0,0002. Y aunque
cuadrara, la escalera la tumba. No hace falta ensancharla.

**Lo que queda por decidir** (para cuando se retome): dónde se escribe el `.npack` dentro de
la zona de packs, y si convive con la cadena `BPAK` o va aparte. El barrido hace que la
respuesta importe menos, que es precisamente la gracia.

---

## 📌 DECIDIDO: de dónde sale la RAM del pack (7-ago) — y por qué

Eduardo: *«Aunque sea una prueba lo podemos hacer con más o menos precisión. Lo importante
es que queda constancia de lo que se decidió y por qué. Así cuando hagamos la implementación
con el SQLite de verdad podamos tomar las decisiones correctas.»*

Esto es ese registro. **No es andamio del experimento: son decisiones que valen para SQLite.**

### El problema que lo obliga

El **SELLO** de un pack son DOS direcciones, `linked_flash` y `linked_ram`, y las dos se
calculan **en el PC** al realojar. La de flash sale de la tabla de particiones. La de RAM
tiene que ser igual de determinista — y **un `static` normal no vale**: vive en `.bss` y se
mueve en cada enlace. (Medido hoy: el ancla se movió de `0x1004DC40` a `0x1004E104` sólo por
añadir un fichero.)

### Decisión 1 — la RAM sale de la ARENA cuando la hay; de la SRAM sólo si no

Criterio de Eduardo: *«la reserva solamente hace falta si SQLite=0»*. Y es que
`SQLite>0` implica PSRAM, donde sobra sitio; quitarle KB a la SRAM de la VM sólo se
justifica cuando no queda otra.

| caso | de dónde | dirección |
|---|---|---|
| `SQLite>0` | el **principio** del bloque de la BD → `[estáticos \| arena]` | `sqliteBase`, ya determinista |
| `SQLite=0` | la cola de la SRAM principal, y la VM se para debajo | `0x2007E200` |

Esto **no es nuevo**: ya estaba decidido al cerrar la RAM de la BD (*«un solo número, y de
ese bloque salen también los ~7 KB estáticos»*). Lo de hoy es aterrizarlo.

### Decisión 2 — son 7168 bytes, y es un número MEDIDO

`ram=7168` en el manifest, de la prueba A. No es un redondeo a ojo. Mi primer intento puso
4 KB "que parecían suficientes" y era **corto para SQLite** — Eduardo lo cazó de memoria.

🔑 **Y el tamaño lo declara el PACK, no el firmware.** La constante del firmware es lo que
OFRECE; lo que cada pack NECESITA va en su cabecera (`data_bytes + bss_bytes`) y la escalera
los compara. **Ese mecanismo ya existía** y no hubo que añadir nada: un pack que pidiera más
sale con su peldaño TAMAÑO en vez de escribir fuera.

### ⚠️ El error que cazó Eduardo, y que sólo habría reventado en la otra placa

Mi primera versión puso el bloque "arriba del todo de la SRAM" sin más. Pero
`vm_sram_region` (main.c:97) le da a la VM **todo lo que hay entre el final del `.bss` y el
techo**. En una placa SIN PSRAM el bloque caía **dentro del heap de la VM**.

Y aquí está lo feo: **en la Metro no se habría visto NUNCA**, porque allí el heap se va a la
PSRAM y la SRAM queda libre. Sólo habría reventado en la Pico 2 — **con la misma imagen**.
Eduardo lo vio por instinto (*«no me cuadra tanta RAM libre; en la Pico era más reducido»*)
antes de que existiera ningún síntoma.

La corrección estructural: el techo de la VM y la base del bloque salen de **la misma
constante** (`PACK_RAM_SRAM_BASE`), en una cabecera que incluyen los dos. Romper uno sin ver
el otro deja de ser posible. Es [[focus-un-kit-batch-cross-family]] al revés: **una imagen
única obliga a pensar en la placa que NO tienes delante.**

### Consecuencia que hay que tener presente para SQLite

Si la RAM del pack sale del bloque de la BD, entonces:

- cambiar `SQLite=<MB>` **NO la mueve** (sigue al principio del bloque) ✅
- poner `SQLite=0` **SÍ la mueve** (se va a la SRAM) → el sello lo caza y dice
  «regrábalo», que es lo correcto, pero conviene saberlo.

Y cuando SQLite sea real, su arena empieza **detrás** de los 7168, no en `sqliteBase`.

---

## 🐛 HALLAZGO (7-ago): el BURN de packs NO FUNCIONA en la Metro

**No es de V5.** Es una función de **V4, ya publicada**, que falla en una placa soportada.

### Los hechos, y sólo los hechos

| | |
|---|---|
| grabar mi `.npack` envuelto en `.pack` | ❌ `VERIFY_FAIL` |
| grabar `packs/Stdlib.pack` — pack conocido y bueno | ❌ **el mismo error** |
| el `.pack` mío | ✅ correcto: Java lo lee, y los DOS CRC cuadran con `bpvm_pack_crc16`, el del propio firmware |
| la lógica de `burn` | ✅ correcta: **mismo `bpvm_pack.c`, misma imagen, graba OK en el PC** con flash simulada |
| tras el fallo | la zona queda mal y **hay que formatear** |

Que el `Stdlib.pack` falle igual es lo que cierra el asunto: **el pack no tiene nada que ver.
No graba la zona de packs de la Metro.**

### El contexto que faltaba

`#327` cerró packs *"verificado en placa (DK2)"* y en la Pico. **En la Metro no consta.** O sea
que es probable que esto no haya funcionado nunca ahí, y nadie lo hubiera notado porque
nadie había grabado un pack en esa placa.

### Sospechoso número uno (HIPÓTESIS, sin confirmar)

La Metro tiene la **PSRAM colgando del MISMO QMI que la flash**. Programar flash exige
suspender el XIP, y en esta placa el heap de la VM vive *en* la PSRAM — al otro lado del
mismo controlador. Es plausible y encaja con "va en la Pico, no va aquí", pero **no está
demostrado y no se parchea a ciegas** ([[debug-acotar-antes-de-tocar]]).

### El experimento que lo decide, cambiando UNA cosa

`psram=0` en el ENV: `board_desc_psram_from_env` ni siquiera sondea la PSRAM, así que el QMI
se queda sólo con la flash y el heap se va a la SRAM. Misma placa, misma imagen, **sin
reflashear**.

- **si el burn FUNCIONA** → la PSRAM/QMI queda señalada, y el arreglo va por ahí
- **si SIGUE fallando** → la PSRAM no es; hay que mirar otra diferencia de la Metro
  (chip de flash de 16 MB, la dirección de la zona, la variante B)

### Y un agujero del instrumento, aparte

`VERIFY_FAIL` se devuelve en **tres sitios** de `bpvm_pack_burn_end` —el recorrido de
entradas, el CRC del contenido, y la relectura final de la cabecera— con **el mismo código**.
No se puede saber cuál fue. Arreglarlo es barato y ayuda aquí y en el futuro, pero cuesta
reflashear, así que va después del experimento de arriba.

**Y otra cosa a mirar cuando toque:** un burn fallido deja la zona inservible. El diseño decía
que la zona quedaría *invisible* (el magic se escribe el último, así que un corte no deja un
pack a medias). Que haga falta formatear dice que esa escalera de recuperación no aguanta
este caso.

### ❌ Hipótesis REFUTADA: la PSRAM no es

`psram=0` + `SQLite=0` (el QMI se queda sólo con la flash, heap en SRAM): **el burn sigue
fallando igual**. La PSRAM compartiendo QMI encajaba muy bien con "va en la Pico, no va
aquí", pero encajar no es ser. Queda descartada.

Diferencias de la Metro que siguen vivas: el chip de flash de **16 MB** (otra pieza que la
de la Pico), la **dirección/tamaño de la zona** (0x10BC4000, 4.2 MB), la variante B.

### ⚠️ Y un error de método MÍO, que cazó Eduardo

Propuse contar candidatos en la zona con `cargarPack` para saber si los bytes habían
llegado a la flash. Eduardo: *"eso no vale. Ahora no aparece nada y no sabemos su estado de
verdad. Hacer una prueba sobre eso la invalida."*

Es exacto: la zona lleva **dos grabaciones fallidas** encima. Cualquier medida sobre ella
mezcla el efecto que buscas con la basura que dejaron los intentos anteriores. Habría
producido un número, y ese número no habría significado nada — que es peor que no medir,
porque parece un dato.

**Lo correcto es lo que hizo él: flash completo con el `.uf2`, para partir de un estado
CONOCIDO (todo a unos).** Es [[debug-acotar-antes-de-tocar]] aplicado al estado y no al
código: antes de medir, saber de dónde partes.

⚠️ Nota práctica: un borrado completo se lleva también el ENV, y `psram` **por defecto es 0**
(`bpvm_env_get_bool(&s_env,"psram",0)`). En la Metro hay que volver a ponerlo a 1.

**Y una posibilidad que hay que tener en cuenta al repetir:** puede que el PRIMER burn
fallara por algo que luego envenenó todos los demás. Con la zona limpia de verdad, el
resultado puede ser otro. Por eso no se instrumenta nada todavía.

---

## Drivers de pantalla en PACKS — el paso intermedio antes de LVGL (Eduardo, 8-ago)

**NO planificado.** Se retoma cuando la carga de trabajo lo permita; puede ser V5
o más adelante. **La prioridad ahora es SQLite.**

### La idea, en sus palabras

> *«Visto lo visto, algún día lo tendríamos que hacer con LVGL. Pero como lo veo
> delicado, podemos hacer un paso intermedio. La idea sería tener los drivers de
> los displays en Packs (con varios packs, que juntes varios drivers en un pack)
> y que el usuario grabe el pack que le interese. Así que el usuario en el
> environment indique el driver a utilizar y el sistema cargue el que se
> indique.»*

Nace de lo que se vio el 8-ago sin buscarlo: cuando SQLite falló, se rehicieron
**packs**, no imágenes. Lo que compra el pack no es tiempo, es **independencia** —
y eso escala con el número de imágenes (hoy 5; con C3/C6, 7).

### Por qué es MEJOR conejillo que LVGL

- Si sale mal te quedas **sin pantalla, no sin base de datos**.
- Prueba lo único que SQLite **no** probó: un pack que toca **HARDWARE**, no sólo
  ficheros. Ése es el riesgo real que hay que medir antes de mover LVGL.
- **La mitad ya está hecha**: desde #311 el panel sale del ENV (`display=st7701`),
  no de un `#define`. El mecanismo de SELECCIÓN existe; cambiaría dónde vive el
  código, no cómo se elige.
- **La costura ya está dibujada**: el `Display · táctil (seam)` es una fachada.
  Esto no crea una frontera nueva, mueve la implementación de una que ya hay.

### ⚠️ HALLAZGO que acota la idea (comprobado el 8-ago, no supuesto)

Los drivers de panel que hay HOY en el P4 —`ST7701`, `EK79007`— son
**`managed_components` de ESP-IDF** (`esp32p4/managed_components/espressif__esp_lcd_*`).
No son código nuestro ni portable: se apoyan en `esp_lcd`, o sea en el periférico
MIPI-DSI del chip. Empaquetar eso obligaría a meter una **API del fabricante** en
la tabla BIOS común — exactamente lo que prohíbe [[hal-bp-capa-comun]].

⇒ La idea **es viable, pero no uniformemente**. Se parte por CÓMO se ataca el panel:

| tipo de panel | viabilidad | por qué |
|---|---|---|
| **por SPI** (ST7789 y similares: comandos+datos por bus) | **ALTA** | el driver es una secuencia de bytes; con SPI+GPIO+`delay_us` en la tabla es portable de verdad |
| **sobre controlador dedicado** (MIPI-DSI del P4, LTDC del STM32) | **BAJA** | el driver es inseparable de la pila LCD del fabricante; el pack no compra nada y rompe la capa común |

Y eso NO arruina la idea: los paneles SPI son la mayoría de los baratos, que son
justo los que multiplican imágenes. Los difíciles se quedan enlazados, que ya es
donde están hoy.

### La pregunta que hay que resolver ANTES de tocar nada

**¿Hasta dónde tiene que crecer la tabla BIOS?** Hoy da libc + ficheros + arena.
Un driver de panel necesita además **SPI, GPIO (DC/RST/CS) y un retardo**.

Conviene diseñar esa sección **ENTERA de una vez**, no por goteo, porque cada
crecimiento sube `BPVM_BIOS_VERSION` e **invalida los packs ya grabados**. Eso se
aprendió en carne propia el mismo 8-ago: el pack del 7-ago devolvió `3` contra el
firmware nuevo. El gate funciona —dice el motivo en vez de saltar a la ranura
equivocada— pero cada versión nueva obliga a regrabar todo lo que haya fuera.

Segunda pregunta de diseño, más pequeña: ¿el pack llama **hacia abajo** al SPI por
la tabla, o recibe un **bus ya configurado**? Lo segundo es más delgado y deja la
configuración (pines, velocidad) donde ya está: en el ENV.

### Lo que NO hay que decidir todavía

Si un pack lleva **un** driver o **varios**. Eduardo apunta a varios agrupados, y
tiene sentido —el usuario graba el pack de su familia de paneles— pero eso es
empaquetado, no arquitectura: se decide cuando el primero funcione.

## El resultado de un SELECT: ¿hace falta un `ResultSet`? — Eduardo, 9-ago

Pregunta suya pensando en el paso siguiente a H4: *«cuando se haga un select el
resultado imagino será un string… creo recordar que se utilizaba un ResultSet»*.

**Dos correcciones a la premisa, y las dos importan.**

1. **El resultado NO es texto.** SQLite devuelve cada columna con su tipo, y la
   API los lee así (`getLong` / `getDouble` / `getStr` / `getBytes`). Pasar por
   string perdería precisión en los `double` y metería un parseo por celda —
   caro en un micro, y sin ninguna ventaja.
2. **El `ResultSet` YA existe: es `Stmt`.** Lo que en JDBC son dos objetos
   (`PreparedStatement` + `ResultSet`), en SQLite es uno: preparas, haces `bind`
   de los `?`, y `step()` va dando filas.

### Los tres niveles, y sólo el tercero está sin decidir

| nivel | qué es | memoria | estado |
|---|---|---|---|
| **cursor** (`Stmt`) | una fila cada vez, nada materializado | 1 fila | H4, en el borrador |
| **lista de entities** | resultado suelto, con LÍMITE obligatorio | N filas | decidido (ver ORM) |
| **ventana** | navegar ↑↓ con pocos registros vivos | K filas | idea de Eduardo, pendiente |

El nivel 2 ya tenía su límite obligatorio decidido, y ahora se ve POR QUÉ desde
el otro lado: materializar es exactamente lo que el cursor evita. Y la
**ventana** —la idea de Eduardo— es el `ResultSet` desplazable de JDBC, que es
lo que necesita una tabla en pantalla.

### Dos trampas que conviene tener escritas antes de construir encima

- **Un cursor abierto RETIENE la base de datos.** No hay destructores en BP: un
  `Stmt` olvidado no es sólo una fuga, mantiene viva una transacción de lectura.
  Es el argumento fuerte para que **el ORM devuelva listas y ventanas, no
  cursores crudos**: el usuario no debería poder olvidarse de cerrar algo cuyo
  coste no ve.
- **El tipo puede cambiar POR FILA.** SQLite es de tipado dinámico (afinidad,
  no restricción): una columna declarada `TEXT` puede traer un entero en la
  fila 3. Por eso `columnType(i)` está en la API. Quien venga de JDBC dará por
  hecho un esquema fijo, y ahí es donde se lleva la sorpresa — y en una BD que
  el usuario copió de su PC, que es el caso REAL (`SmartMini.db`), pasa.

## 💡 `try` CON RECURSO — idea de Eduardo, 9-ago (try-with-resources)

Textual: *«En Java hay un statement con un bloque para cuando abres un archivo,
trabajas con él y cuando cierras el statement se hace el close. De esa forma no
te puedes olvidar de cerrar.»*

Sale de la conversación de SQLite —un `Stmt` olvidado retiene la BD— pero **no
es de SQLite ni de H4**: sirve igual para `File`, `Net.Tcp` y `Mutex`.

```
try var q := db.query("SELECT sensor, valor FROM medidas WHERE sensor = ?", "temp")
  while q.next()
    print q.getStr(0), q.getDouble(1)
  end while
end try                  // se suelta q — salgas por donde salgas
```

### Por qué sale barato, y por qué encaja

- **Cero palabras reservadas.** Cuelga de `try`, que ya existe. Norma de Eduardo
  ([[no-gastar-palabras-reservadas]]): si ya hay algo especial, el azúcar cuelga
  de ahí — nada de `using` ni `with`.
- **CERO cambios en las VMs.** Es azúcar de front-end: se emite como el
  `try/finally` que YA funciona en las dos (`samples/fase4.bp` verifica incluso
  que el `finally` corre cuando el `catch` reemite). Mismo patrón que
  `Thread(obj::metodo())` y los parámetros por defecto.
- **Ya hay precedente en el lenguaje**: `synchronized(m)` es un bloque que suelta
  al salir. El concepto no es nuevo en BP; lo nuevo es generalizarlo.
- **Y aquí PESA MÁS que en Java**: BP no tiene destructores ni finalización por
  GC. En Java el recolector acaba tapando un `close` olvidado; aquí no hay nadie
  detrás. No es comodidad — es el único mecanismo bueno posible.

### Lo que falta decidir

Cómo sabe el compilador **qué** soltar. Lo natural: una base `Closeable` en
`Core.bp` —donde ya vive `Exception` (#248)— con `release()`, y que `try` exija
que el recurso descienda de ella. A decidir también si se admite más de un
recurso en el mismo `try`.

### ⚠️ Consecuencia PARA AHORA, en H4

Aunque la función del lenguaje se haga después, **la API de SQLite se diseña ya
para encajar**: el método se llama `release()` (no `close()`, que queda para
"he terminado con la BD" — la distinción es de Eduardo) y `Stmt` desciende de
`Closeable`. Así el día que entre el bloque, `SQLite.bp` no se toca.

### 🔄 REVISADO el mismo día: no es el bloque O el destructor — son CUATRO CAPAS

Eduardo, 9-ago, corrigiéndome: *«tú lo estás viendo como una cosa o la otra, yo
lo veo como una combinación de las dos… owner es voluntario, lo puede escribir el
usuario o no. Por eso lo del doble mecanismo: el try nos garantiza que la
consulta termina de forma normal, y el owner es una garantía doble, por si falla
la primera.»*

**Mi error de encuadre, y conviene que quede escrito**: yo argumentaba contra los
**finalizadores** —los que dispara el GC cuando le apetece, que es lo que Java
deprecó en la 9 y quitó en la 18— y Eduardo describía **destructores con
propiedad**, que no los dispara la presión de memoria sino el **final del
ámbito**. El GC no interviene, así que la objeción no aplicaba.

| capa | quién la pone | qué cubre |
|---|---|---|
| 1. `try` con recurso | el usuario (voluntaria) | el camino normal, y la excepción dentro del bloque |
| 2. `var owner q := …` | el usuario (voluntaria) | lo mismo SIN anidar, y cuando no hay bloque |
| 3. el agotamiento GRITA | nosotros | el que no escribió ninguna de las dos |
| 4. fin de RUN: suelta y CUENTA | nosotros | el recuento final, como #339 con la memoria |

Las dos primeras son voluntarias, y **por eso hay dos**: aumentan la probabilidad
de que al menos una esté. Las dos últimas no dependen del usuario y son las que
convierten un olvido en un mensaje en vez de en una muerte por inanición.

El **destructor** es lo que 1 y 2 invocan: «qué significa destruir esto» se
escribe UNA vez, en la clase, y no en cada sitio donde se usa.

#### Decidido

- **Sintaxis: `function ~Db()`**, el contrario del constructor (que ya es
  `function Db()`). ✅ VERIFICADO que `~` no se usa en ninguna fuente del
  frontend; y los operadores de bits de BP son PALABRAS (`shl`, `shr`), así que
  un NOT de bits futuro sería `bnot` por coherencia y no lo reclamaría.
  **Cero palabras reservadas gastadas** — sólo un símbolo que estaba libre.
- **Orden inverso al de declaración.** Un `q` sacado de un `db` tiene que morir
  ANTES que el `db`; al revés estaría soltando una consulta de una conexión ya
  cerrada. Con el orden inverso sale solo y el usuario no lo piensa.
- **`owner` NO puede escapar** (devolverlo, guardarlo en un campo) → error de
  compilación. Encaja con lo ya decidido: un DAO devuelve listas o ventanas, no
  cursores vivos.
- **Desazucarado a `try/finally`** → CERO cambios en las VMs, igual que
  `Thread(obj::metodo())` y los parámetros por defecto.
- Sobra el auto-soltado que había puesto en `Stmt.next()`: era un truco mágico y
  `owner` hace lo mismo explícito y para todo.

#### Abierto (una sola cosa)

**¿Corre `~Clase()` si el objeto muere SIN `owner` ni bloque, recogido por el
GC?** Mi recomendación es que **no**: ése es justo el camino no fiable —el GC
corre por presión de MEMORIA, y un descriptor o un statement son otro recurso
escaso. Con 8 descriptores agotados y el heap al 3 %, el GC no tiene motivo para
pasar y el destructor no correría nunca. Las capas 3 y 4 cubren ese caso, y lo
hacen DICIENDO lo que pasó en vez de tapándolo tarde.

---

## ✅✅ V5/H4 CERRADO EN LA METRO (10-ago): un SELECT de BasicPlus en el micro

    --- RUN /app/SqlDemo.mod on Placa (serial v1) ---
    base abierta: medidas.db
    filas insertadas: 6
    ...
    cerrada.
    --- VM finished: exit 0 (OK) ---

**25 líneas, `diff` contra `make test-sqldemo`: 0 diferencias.** No a ojo — las
dos salidas a fichero y comparadas. Con eso el host pasa a ser **oráculo** de
esta librería: si mañana la placa dice otra cosa, la diferencia está en la placa
([[doble-mas-amable-que-el-original]]).

### Qué queda demostrado, que no es poco

410 KB de C ajeno (SQLite 3.53.4) **pre-enlazados en base 0**, realojados sumando
una constante, **grabados en la zona de packs** y ejecutados en el RP2350;
publicando su API por nombre; un `.mdn` **generado por el compilador** llamándola
desde BP; y un VFS propio llevando la E/S del motor a la capa de ficheros de BP.

Tres cosas que sólo se podían saber aquí:

1. **#382 NO bloqueaba.** El puente se genera sin un solo literal — los nombres
   se materializan byte a byte y el texto de error vive en la VM (`pack_fallo`).
   Sale con 0 relocs y sin `.rodata`. #382 sigue abierto para OTROS usos (un
   `native` normal con literales), pero no para esto.
2. **Los helpers de 64 bits van en ARM.** `1754300000000` llega entero, así que
   la caja de salida `long[1]` + `array_store_i64` funciona fuera del PC.
3. **El gate de ABI hizo su trabajo**: `.mdn` ABI 3 / arch 40, y el firmware
   viejo lo habría rechazado en vez de saltar a basura.

### ✅ Y LA TARJETA, media hora después

`SqlDemoSd` → `base abierta: /sd/medidas.db`, y **24 de 25 líneas idénticas al
PC**: la única distinta es la que dice el camino. Eso encadena TODO lo de V5 en
una sola ejecución:

    SQLite -> VFS 'bp' -> capa de ficheros BP -> FatFs -> driver SD -> SPI -> tarjeta

H1 (el lector), H2 (FatFs), H3 (el pack) y H4 (la librería) funcionando **a la
vez**, que es distinto de funcionar cada uno en su prueba.

De camino salió un hallazgo que Eduardo cazó preguntando *«¿cómo paso
argumentos?»*: **no se puede**. El `arg` de `main` lo hornea el compilador como
cadena vacía y no hay mecanismo a ningún nivel — ni wire, ni VM, ni IDE
(tarea #386). El `if arg != ""` que yo había escrito era código muerto, y el
LEEME de la entrega llegó a mandar hacer algo imposible. Por eso hay DOS
módulos: el trabajo se escribe una vez en `SqlDemo.correr(camino)` y
`SqlDemoSd` sólo la llama con el otro camino. Se descartó a propósito el atajo
de cambiar el valor y entregar el `.mod`: habría dejado un artefacto sin fuente
que le corresponda —la trampa de [[generado-parcheado-a-mano]]— y habría roto
`make test-sqldemo`, que es el oráculo.

Y el gate de ABI de #284 se ganó el sueldo: al subir sólo el módulo nuevo, la
placa contestó *«lib 'SqlDemo' presente pero no exporta 'SqlDemo.correr' (la usa
'SqlDemoSd'; version vieja?)»* — el módulo, el símbolo, quién lo pedía y la
sospecha correcta. Sin él habría saltado a una dirección cualquiera.

### Lo que NO se ha probado, por no apuntarnos de más

**La persistencia.** La demo hace `DROP TABLE IF EXISTS` + `CREATE TABLE`: parte
de cero por diseño, así que la salida idéntica NO dice nada del fichero que
quedó. Necesita otro programa, no otro Run.

### Los dos cambios que la placa obligó (commit 84558c0)

- **El pack se carga en el `Run`, no en el arranque.** El salto a la entrada del
  pack es el único paso que puede colgar, y en una placa sin botón de reset un
  cuelgue en el arranque se repite en CADA arranque. Una vez, y la bandera se
  pone SÓLO si salió bien: así, después de grabar un pack, el siguiente Run lo
  coge sin reiniciar.
- **La entrada del pack adelgaza.** En H3 el pack ERA el experimento y su
  entrada hacía la prueba entera. En H4 es una librería: arranca, publica y se
  calla. El autotest queda bajo `-DBPSQL_AUTOTEST=1` para el día que una placa
  nueva no arranque y haga falta que conteste sola.

---

## 📌 DECIDIDO (10-ago): la SD a 4 bits **no** se hace en el RP2350; se hará en el P4

Criterio de Eduardo: *«si el RP2350 no soporta SDIO no se hace, se queda como
está. Ahora no nos vamos a meter con el PIO de la RP2350.»*

### Por qué el RP2350 se queda en SPI

Primero, una corrección de vocabulario que importa para no ir al sitio
equivocado: **no es QSPI**. QSPI es interfaz de *flash* (la que usan la flash y
la PSRAM del RP2350). Lo que da los 4 bits es el **modo SD nativo (SDIO)**, que
es otro protocolo: otra capa de comandos, otro CRC, otra inicialización
(`ACMD6` fija el ancho). No es el mismo driver con más patas.

Y el RP2350 **no tiene controlador SDMMC**, así que 4 bits saldría por PIO. Eso
es escribir una máquina de estados de SD a mano — mucho trabajo, mucha
superficie de fallo, y en una placa sin botón de reset.

Se queda como está. SPI además hay que mantenerlo igual por compatibilidad: las
tarjetas antiguas sólo hablan eso.

### El P4 sí, y mejor de lo que pedía la pregunta

Del IDF v6.0.1 instalado (`components/soc/esp32p4/include/soc/soc_caps.h`):

    SOC_SDMMC_HOST_SUPPORTED     1     controlador de verdad
    SOC_SDMMC_NUM_SLOTS          2
    SOC_SDMMC_DATA_WIDTH_MAX     8     (4 para SD; 8 es eMMC)
    SOC_SDMMC_USE_GPIO_MATRIX    1     los pines NO están clavados
    SOC_SDMMC_PSRAM_DMA_CAPABLE  1     DMA directo a/desde PSRAM
    SOC_SDMMC_UHS_I_SUPPORTED    1     SDR50/SDR104

Dos de esas líneas valen más que el ancho de bus para lo que hacemos:

- **DMA a PSRAM.** En el P4 el heap de la VM y el FS ya viven en PSRAM
  ([[esp32-p4-memoria-psram]]), así que los bloques pueden ir por DMA
  DIRECTAMENTE al sitio donde se necesitan, sin buffer de rebote. Para las
  páginas de 4 KB de SQLite eso pesa tanto como los 4 bits.
- **Pines por matriz GPIO**, que encaja con la flexibilidad de placa del ENV
  (#311) en vez de atarnos a unos pads.

### Lo que esto NO es

**El P4 no tiene HOY ninguna línea de SD** (comprobado: cero coincidencias de
`sdmmc|sdcard|microsd` en `bpgenvm-c/esp32p4/`). H1 y H2 se hicieron en la
Metro. O sea que esto es dar soporte de tarjeta a una familia entera, no
cambiar un flag — y aplica [[portar-familia-adaptador-completo]]: encaminar el
verbo no es implementarlo.

### La decisión de diseño que habrá que tomar, y mi recomendación

El IDF trae **su propio FatFs** y un atajo (`esp_vfs_fat_sdmmc_mount`) que monta
la tarjeta en cuatro líneas. Es tentador y **yo no lo usaría**: tendríamos DOS
FatFs con dos configuraciones, y por tanto dos comportamientos que divergen sin
avisar — la misma familia de problema que [[stdlib-mod-version-skew-oo-device]].

Lo que encaja con [[hal-bp-capa-comun]] es lo de siempre: **nuestro FatFs
arriba, y debajo una cintura de bloque nueva** que llame a `sdmmc_host` del IDF.
Con eso el FS se comporta IGUAL en las dos placas, y la Metro sigue sirviendo de
oráculo para el P4 ([[doble-mas-amable-que-el-original]]).

### Preguntas abiertas antes de empezar

1. ¿El zócalo de la placa P4 está cableado a 4 bits (DAT1/DAT2 conectadas) y con
   pull-ups? Un bus sin pull-up no calla: **miente** ([[bus-sin-pullup-miente]]).
2. ¿Para qué queremos la tarjeta en el P4? SQLite corre hoy en la Metro. Si el
   P4 es la placa de aplicación (pantalla + PSRAM), el caso de uso manda el
   orden.
3. La medida base: sin cronometrar lo que hace hoy la Metro, el 4 bits no se
   puede evaluar, sólo creer (#376: medir el REPARTO, no el tiempo).

### La placa es la **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3** — y ya hay datos

De las dos P4, sólo ésa tiene lector (criterio de Eduardo: *«la otra no cuenta»*).
Cuatro de las cinco preguntas de arriba están contestadas, y de la fuente buena:
el ejemplo `examples/esp-idf/05_sdmmc` del repo `waveshareteam/ESP32-P4-WIFI6-
Touch-LCD-4.3` — **de esta placa**, no de un modelo hermano. (Es el mismo repo
del que salió la tabla DCS del ST7701, así que ya está contrastado en placa.)

| | respuesta | fuente |
|---|---|---|
| ¿DAT1/DAT2 cableadas? | **SÍ** — su ejemplo tiene modo 4 bits con D1/D2/D3 | `05_sdmmc/main/Kconfig.projbuild` |
| pines | **CLK 43 · CMD 44 · D0 39 · D1 40 · D2 41 · D3 42** | idem |
| alimentación | **LDO interno, canal 4** (`default 4 if IDF_TARGET_ESP32P4`) | idem |
| protocolo del zócalo | **SDIO 3.0** (no SPI) | docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-4.3 |
| pull-ups | ⬜ sin confirmar, **pero se resuelve probando** (ver abajo) | carpeta `schematic/` del repo |

⚠️ **Confianza**: los pines son los *defaults del ejemplo del IDF tal como
Waveshare lo empaqueta para esta placa*. Muy probablemente son el cableado real
—no publicarían un ejemplo roto—, pero NO es lo mismo que leer el esquema. La
carpeta `schematic/` lo zanja.

✅ **Comprobado en local: ninguno de los pines 39–44 se usa hoy** en
`bpgenvm-c/esp32p4/main/`. El wire va por 37/38 ([[esp32-p4-uart-wire]]), así
que no chocan.

🔑 **El LDO es el `bsp_display_backlight_on()` de esta historia.** Su propia
ayuda lo dice: *«check if the SD VDD is connected to any internal LDO output»*,
con `default y` y canal 4. Si no se enciende antes de montar, la tarjeta no
aparece y **parece que el driver no va** cuando lo que pasa es que el raíl está
apagado. Ya sabemos el número; no hay excusa para caer en ésa.

### Lo único que puede complicar el trabajo: el reparto de slots con el C6

La placa lleva un **ESP32-C6 para WiFi6 colgado por SDIO** (lo dice el wiki, y
ya estaba en la investigación de V3: `docs/V3_IDEAS.md:964`). O sea **dos
periféricos SDIO** y el P4 tiene **dos slots**. Encaja, pero hay que confirmar
que no comparten pines ni se pisan al inicializar — si el WiFi se cae al montar
la tarjeta, o al revés, el problema aparecería como algo intermitente y raro.

Está en el mismo esquema. Es la pregunta a resolver ANTES de escribir código.

### Los pull-ups no bloquean: se prueba (criterio de Eduardo) — con UNA condición

*«Es hacer una prueba y si funciona ya está, si no se activan los pull-ups de
los pines y se repite.»* De acuerdo, y el flag existe:
`SDMMC_SLOT_FLAG_INTERNAL_PULLUP`. Pero el IDF pone un aviso en esa misma línea:

> *"The internal pullups **are insufficient** however, please make sure external
> pullups are connected on the bus. **This is for debug / example purpose only.**"*

O sea que los internos valen como **diagnóstico** (te dicen si el problema eran
los pull-ups), no como arreglo. Si hicieran falta, eso es un HALLAZGO sobre la
placa —y entonces toca bajar el reloj y anotarlo—, no una victoria.

🔑 **Y la condición, que es lo importante: «monta y se ven los ficheros» NO es
prueba suficiente.** Un bus marginal no falla en el `mount`: falla a ratos, a
reloj alto o con la placa caliente. Debajo de SQLite eso no da error, **corrompe
la base en silencio** — peor que no funcionar.

La prueba que distingue *funciona* de *parece que funciona* es barata: **escribir
unos MB de patrón conocido, releerlos y comparar byte a byte, al reloj
objetivo**. Una sola diferencia = mirar el hierro antes de seguir construyendo.

Motivo para ser MÁS optimista que en la Metro: allí el fallo fue en SPI, donde
una línea al aire leía `0x00` y el SD lo aceptaba como R1 válido — falso
positivo ([[bus-sin-pullup-miente]]). **SDMMC lleva CRC en CMD y en DAT**, así
que una línea mala tiende a fallar ruidosamente en vez de inventarse datos
plausibles. Y la placa publica su ejemplo en 4 bits, que nadie hace sin los
pull-ups puestos.

---

## 🧩 V5/H5 paso 3 — GENERAR el DAO (charla de diseño, 10-ago)

Con el DAO a mano terminado y verde en host (`Orm.bp` + `Medidas.bp` +
`Sensores.bp` + `DaoDemo.bp`), queda automatizar su generación. Esto es la
charla completa, antes de escribir código.

### Las dos piezas

1. **Marcar** qué clase es una entidad ligada a una tabla, y qué propiedad
   corresponde a un campo. Sintaxis acordada: `@BD{ ... }`.
2. **Generar** el DAO a partir de la entidad.

### La ventaja que lo decide todo: tenemos nuestro compilador

Idea de Eduardo: parsear la entidad **con el compilador de siempre**, quedarse
con el AST —ahora con `@BD{...}` dentro— y, en vez de compilar, emitir el DAO.

Lo importante de esto no es la comodidad: es que **no hay una segunda gramática
que mantener**. Un generador con su propio parser acaba, tarde o temprano,
entendiendo BP distinto de como lo entiende el compilador. Saliendo del AST eso
es imposible por construcción.

### Emite BP FUENTE, no `.mod`

Tentador saltarse el paso, pero el DAO generado es la clase **de la que hereda
el usuario** (tercer nivel del diseño), así que tiene que poder leerla. Además
entra por el mismo compilador que todo lo demás —no puede generar algo que el
lenguaje no exprese—, tiene números de línea, y sale en el depurador y en las
trazas.

### ✅ El criterio de aceptación, que ya existe

**El generador está terminado cuando genera `Medidas.bp` y el diff contra el
que escribimos a mano sale vacío.** No «compila», no «funciona»: *ése*. Es el
premio de haberlo escrito a mano primero y conviene cobrarlo — es binario y no
admite interpretación.

### La anotación, versión 1 (Eduardo: «el resto ya irá saliendo»)

Lo mínimo: marcar la clase como entidad ligada a una tabla, y marcar la
propiedad como campo. Nada más de momento.

- **Explícita por propiedad.** Yo había argumentado el defecto contrario —que
  mapee salvo que digas lo contrario— porque olvidarse de anotar una propiedad
  falla en SILENCIO (el campo no se guarda nunca). **Pero con la BD delante eso
  deja de ser cierto**: el verificador ve una columna que nadie mapea y lo dice.
  El contraste hace segura la anotación explícita.
- **Nombre de columna literal**, igual que la propiedad, salvo que se diga otro.
  Nada de convertir `sensorId` a `sensor_id` por nuestra cuenta: una regla más
  que recordar y que muerde cuando no la esperas.
- **Se valida aunque NO estés generando.** Si el módulo compila ignorando un
  `@BD{ tabl: "medidas" }` mal escrito, el error no se ve hasta que generes —o
  peor, sale un DAO con la tabla vacía.
- **Sintaxis extensible por nombre**, no posicional: el día que haga falta
  declarar una conversión (`columna TEXT` ↔ `propiedad long`) tiene que caber
  un campo más.

### La BD, propiedad del PROYECTO

```json
{ "sourceDir": "...", "outDir": "...", "main": "...",
  "database": "medidas.db",
  "dependencies": ["..."] }
```

**La tabla es del dominio; el fichero es del despliegue.** `Medida` está ligada
a `medidas` siempre; el fichero es `medidas.db` en el PC y `/sd/medidas.db` en
la Metro. Si la ruta viviera en la entidad, la entidad quedaría atada a un
despliegue.

⚠️ **La BD del proyecto es para la HERRAMIENTA, no para la ejecución.** Sirve
para verificar y generar; el programa sigue abriendo en runtime la ruta que le
dé la gana. Si se confunden, alguien dará por hecho que el programa abre la del
`.bpbuild`.

Efecto lateral bueno: **la anotación es también el registro**. No hace falta
lista de entidades — la herramienta barre el proyecto y las entidades son las
clases con `@BD`.

### Contrastar contra el esquema real — y sus LÍMITES

El esquema existe primero (la base se diseña en el PC), así que la entidad
anotada es una **segunda descripción de la misma tabla**: el mismo contrato
escrito dos veces, que es de lo que huimos todo el día. La diferencia es que
aquí la solución está al alcance: `PRAGMA table_info(<tabla>)` y comparar.

**Pero no todo se puede comprobar, y confundirlo convertiría la herramienta en
una limitación** (criterio de Eduardo: *«las herramientas deben ser una ayuda,
no una limitación»*). El motivo es exacto:

- **De tipo BP a clase de almacenamiento es una FUNCIÓN**: `boolean`→INTEGER,
  `long`→INTEGER, `double`→REAL, `string`→TEXT.
- **Al revés es una RELACIÓN**: un INTEGER puede ser un booleano, unos segundos,
  unos milisegundos, un enum, una clave ajena o un número. La información no
  está ahí. Y en SQLite es peor: el tipo declarado en el `CREATE TABLE` es
  **orientativo** (afinidad), una columna `BOOLEAN` acepta texto igualmente.

Por eso hay **dos clases de discrepancia y NO se tratan igual**:

| | qué es | qué hace |
|---|---|---|
| **Estructural** | columna que no existe, columna que nadie mapea, clave que no cuadra | **ERROR** — no hay decisión que tomar, alguien se equivocó |
| **De tipo** | entidad dice `long`, columna dice TEXT | **AVISO**, silenciable — puede ser un error o una decisión tuya |

Y por lo mismo se descartó **sembrar la entidad desde el esquema**: no se puede,
y forzarlo dejaría tirado al primero que guarde una fecha de una forma que no
se nos ocurrió.

### Verbo propio: `DAO build`

`Build` compila el proyecto. `DAO build` construye los DAOs y **se detiene** —
para que el usuario revise lo generado y escriba sus clases.

Esto además resuelve un problema de ORDEN que tenía la alternativa (generar
dentro del build): el compilador va dirigido por demanda, y al resolver
`import MedidasDao` se encontraría con que ese fichero todavía no existe. Con
un verbo aparte, cuando arranca el build el DAO ya es un `.bp` más. **Es lo
mismo que ya se hace con el `.win` del diseñador de ventanas**: se genera antes,
entra como fuente.

Y el argumento de fondo: **la regeneración pasa a ser un ACTO, no un efecto
secundario.** Con el usuario heredando del DAO generado, una regeneración
automática podría reescribirle por debajo la clase de la que cuelga su código.

### Generación manual, DETECCIÓN automática

Complemento imprescindible: si `Build` deja de mirar, te enteras de que la
entidad se quedó atrás el día que regeneres. Así que **es la misma maquinaria
con dos entradas**:

- **`Build`** → modo informe. Compara entidades, esquema y lo generado; dice lo
  que no cuadra; no escribe nada.
- **`DAO build`** → modo escritura. Lo mismo, y además genera.

Con eso, el comando suelto que Eduardo quería para el problema de la entidad
rancia no es una herramienta aparte: es el modo informe.

### El «si hace falta» tiene que mirar la BD

Con el `.mdn` basta comparar fechas del fuente. Aquí no: **añades una columna,
el `.bp` de la entidad no ha cambiado, y el DAO se queda rancio** — que es
exactamente el fallo a cazar. El esquema es una entrada del cálculo de
obsolescencia, no sólo de la validación. Hash del `PRAGMA table_info`, para no
regenerar cada vez que alguien inserta una fila.

### Lo hace el COMPILADOR; el IDE es pasarela

Decisión de Eduardo. Respaldo: el IDE empaqueta su propia copia del compilador,
y ya costó un diagnóstico falso (el `GuiColorDemo` en cian) porque esa copia iba
rancia. Si el `DAO build` viviera en el IDE, CLI e IDE podrían generar cosas
distintas.

Consecuencia práctica que lo hace gratis: **los diagnósticos del generador salen
por el mismo canal que los del compilador**, con fichero y línea apuntando a la
anotación. Así el IDE ya sabe pintarlos (cascada y dedup incluidos) y sólo hay
que añadirle el botón.

⚠️ Al meter el generador en el frontend habrá que reconstruir el fat-jar del
IDE — **nunca con el IDE abierto**.

### Lo generado

- Carpeta propia, separada del código escrito a mano.
- Cabecera **NO EDITAR — se regenera**.
- **Seguro contra el que lo edita igual**: un hash del contenido en la cabecera;
  al regenerar, si el fichero no cuadra con su propio hash, está tocado → se
  niega y lo dice, en vez de tirar el trabajo a la basura. Misma idea que los
  gates de ABI.
- **Reproducibilidad**: el DAO se deriva de la entidad **y del esquema**, y el
  esquema es un `.db` binario. Si el `.db` no está en el repositorio, un clon
  limpio no puede reconstruirlo → entonces hay que versionar lo generado. **O
  se versiona el `.db`, o se versiona el DAO; lo que no puede es no estar
  ninguno de los dos.**

### Lo que el generador NO puede emitir (limitaciones medidas del compilador)

- **#392** — nada de métodos SOBRECARGADOS en el DAO generado: una hija con
  sobrecargas, usada desde otro módulo, descoloca los slots y falla al ENLAZAR.
  La regla de nombrado (`load(e)` / `loadById(id)`) ya lo esquiva.
- **#388** — nada de encadenado: lo que devuelve un método pierde sus miembros.
- **#389** — el estrechamiento de `Object` a la entidad NO se comprueba.
- **#390 / #391** — sin `protected` ni `virtual`: lo heredable, `public`.

### 📌 DECIDIDO: la anotación NO se persiste

Va en los objetos del AST y se queda ahí. **No se toca el `.mod` ni nada**: son
anotaciones para el compilador y el generador, nada más (Eduardo, 10-ago).

El argumento: **la anotación no tiene consumidor en tiempo de ejecución** — todo
lo que dice acaba siendo código normal dentro del DAO generado. Y esquiva lo
caro: tocar el `.mod` arrastra gate de versión, regeneración de stdlib y el
desfase de las 4 copias, por un dato que sólo mira una herramienta del PC.

Precisión: **se parsea siempre, se analiza siempre, no se emite nunca.**
`@BD{...}` es gramática de verdad —si sólo lo aceptara la herramienta, la
entidad no compilaría en un build normal— y se valida en cualquier compilación.

Y esto fija dónde encaja el generador: necesita el **AST anotado + la tabla de
símbolos**, porque para emitir `q.getDouble(2)` tiene que saber que `valor` es
`double`, y eso lo pone el semántico. O sea, justo donde Eduardo lo puso —
después del análisis, junto al `.mod` y al `.mdn`. Efecto lateral bueno: **no
se puede generar un DAO de una entidad que no compila.**

⚠️ El único escenario que rompería esta decisión: distribuir entidades como
librería compilada, sin fuentes. Entonces la anotación tendría que viajar en el
`.mod`. No es el flujo actual, pero si algún día aparece, ésta es la decisión a
revisar.

---

## El pack nativo se relocaliza AL GRABAR — y lleva un binario por familia

**Decidido 12-ago-2026 (opción B).** Criterio de Eduardo al elegirla:

> *«No podemos ir haciendo packs forzando las cosas. Construimos las
> herramientas, las verificamos que funcionen bien, y entonces el pack sale
> solo.»*

### El problema, medido

Al intentar meter `SQLite.mod` y `SQLite.mdn` dentro del pack que ya lleva el
motor nativo, salieron tres cosas:

1. **`pack.py` da por supuesto un pack de UNA entrada.** Lo tiene escrito:
   `+0` cabecera del pack (128 B), `+128` cabecera de la entrada (48 B),
   `+176` los datos. Con tres entradas la tabla mide 3×48 y los datos empiezan
   en **+272**. El sello (`linked_flash`) seguiría diciendo +176.
   *Confirmado en placa: el P4 encontró el pack en `0x401590b0` = zona + 176.*
2. **`out:pack` no recoge el `.npk`**: filtra el `outDir` a `.mod`/`.mdn`.
3. **Y aunque entrara por `resources/`, iría al final** y en orden alfabético.

No sería un fallo mudo —la escalera compara el sello con dónde encuentra el
pack y diría «realojado para OTRA dirección»— pero no arrancaría.

### La causa de fondo, y el hueco que la explica

Que la posición sea cargante es el SÍNTOMA. La causa es que relocalizamos **a
mano y por adelantado**, con un script.

`bpvm_npack.h:68` dice: *«La tabla de relocalizaciones NO viaja: la consume el
IDE al grabar»*. Eso es el modelo **previsto**, no el implementado:

| lado | estado |
|---|---|
| dispositivo: escalera de validación, sello, `E_SIN_RELOC` | ✅ hecho |
| IDE: relocalizar al grabar | ❌ **no existe** |

Buscando `npack` / `linked_flash` / `R_RISCV` / `reloc_count` en todo el árbol
Java —IDE, `pack/`, frontend— no hay nada. El único relocalizador del repo
entero es `notas/v5-sqlite-prueba/I/pack.py`, un prototipo en la carpeta de
notas. **Ese es el trabajo de B**: portarlo a producto.

No es empezar de cero: los dos relocalizadores (ARM y RISC-V) funcionan y están
verificados contra `ld`. Y `pack.py` lleva un **oráculo** —compara su resultado
con lo que produce el enlazador— que se porta como test. Lo que hoy es un
script sin red pasaría a tener la suya.

### El modelo

1. **El build produce un `.npk` SIN relocalizar POR DESTINO** (ver D1: uno por
   fichero, con doble extension), cada uno con su tabla de relocalizaciones.
   `reloc_count > 0`, sello a cero.
2. **El IDE, al grabar**: mira la placa conectada, elige el FICHERO que le
   toca, calcula la dirección REAL (base de la zona + offset de la entrada
   dentro del pack que acaba de montar), aplica las relocalizaciones, escribe
   el sello y graba.
3. **El dispositivo valida y ejecuta en sitio.** Esa parte ya está hecha.

Con esto la posición dentro del pack **deja de importar**: el IDE conoce el
offset de cada entrada porque él mismo montó el pack.

### La llave es UN DESTINO, no la arquitectura ni el par (arch, float_abi)

Primera versión de este documento: la llave era el par `(arch, float_abi)`.
**Está mal, y lo destapó Eduardo preguntando lo obvio:** *«la cuestión de verdad
es si hay un sólo ARM»*.

No lo hay:

| perfil | núcleos | qué cambia |
|---|---|---|
| ARMv6-M | Cortex-M0, M0+, M1 | Thumb reducido, **sin FPU**, sin DSP |
| ARMv7-M / v7E-M | M3 / M4, M7 | Thumb-2 completo, DSP y FPU opcionales |
| ARMv8-M Baseline | M23 | v6-M + TrustZone |
| **ARMv8-M Mainline** | **M33**, M35P | Thumb-2 + DSP + FPU opcional |
| ARMv8.1-M | M55, M85 | + Helium (vectorial) |

Nuestras dos placas ARM (Metro RP2350 y Nucleo STM32U575) son **las dos
Cortex-M33**, mismo perfil y misma FPU de simple precision. Un solo fichero ARM
vale hoy no porque ARM sea uno, sino porque tenemos dos placas del mismo.

**El agujero:** `(arch, float_abi)` NO distingue ARMv6-M de ARMv8-M — los dos
son `EM_ARM = 40` y los dos pueden ser `softfp`. Un pack compilado para M33 en
una placa Cortex-M0+ **pasaría la escalera** y se estrellaría con una
instrucción indefinida. No diría «otra arquitectura»: reventaría. Exactamente
el modo de fallo que la escalera existe para evitar.

Y el ejemplo que lo hace incómodo: **el RP2350 lleva los dos** — dos Cortex-M33
*y* dos núcleos RISC-V (Hazard3), y se elige cuál arranca. «Una placa, una
familia» ya es falso hoy, en una placa que tenemos.

**Decisión: la llave es un `BP_TARGET_*`**, uno por destino, en la misma tabla
única donde viven los `MDN_ARCH_*`. La granularidad correcta ya existe en
`pack.py` (`target="arm-cortex-m33"`, `target="riscv32-esp-p4"`), sólo que hoy
es descriptiva y no es la llave. El `float_abi` deja de ser campo aparte: va
dentro de la definición del destino. Una llave en vez de un par — más simple y
más estricta.

**Y el micro declara qué destinos ACEPTA, en orden de preferencia.** Esto
devuelve lo que se pierde al hacer la llave estricta: una imagen ARMv6-M sí
corre en un M33. Si el firmware declara `[cortex-m33, cortex-m0]`, el cargador
coge el primer fichero que encaje — el optimizado si esta, el generico si no.
Quien sabe lo que puede ejecutar es el micro, no una tabla del PC. Encaja con
#378 («que cada micro DIGA lo que tiene»).

Hoy son DOS ficheros; el S3 hara tres cuando tenga toolchain, y un M0+ haria
cuatro. **El diseno no debe hornear el dos.**

Y el aviso que ya esta escrito en `pack.py`, que aqui vale doble: *«un desajuste
aqui no da error, da NUMEROS MAL»*. Por eso el VALIDADOR vive en el formato y lo
comprueba el dispositivo. El nombre de fichero (D1) es solo el SELECTOR con que
el IDE elige antes de grabar: si mintiera, la escalera lo caza con `E_ARCH`.

### Decisiones

**D1 — EL FORMATO NO SE TOCA. Un fichero por destino, con DOBLE EXTENSIÓN.**

```
sqlite.npk.ARMV8     sqlite.npk.RISCV
SQLite.mdn.ARMV8     SQLite.mdn.RISCV
```

Decisión de Eduardo (12-ago), y sustituye a lo primero que escribí aquí:

> *«Como eso no llega a los micros, no merece la pena hacer un cambio de
> formato. Lo que utilizamos es una doble extensión.»*

❌ **Descartado: una entrada `.npk` con N rebanadas dentro** (cabecera + un
directorio de N destinos, cada uno con su imagen y su tabla). Motivos:

1. **El micro pagaría por algo que sólo sirve en el PC.** El fichero gordo es
   un envase de DISTRIBUCIÓN: a la placa llega una sola familia. Meterle el
   directorio de N al formato de flash es hacerle cargar con una estructura
   cuyo único trabajo es descartar hermanas que nunca va a ver.
2. **El argumento que lo decide es de ESCALA, y es de Eduardo:** *«hoy tenemos
   2 familias, pero mañana pueden ser 4, 6 o n. Eso en el PC no es problema
   pero en un micro no.»* Con la doble extensión, n familias son n ficheros en
   el PC —gratis— y **siempre uno** en la placa. Con rebanadas, cada familia
   nueva ensancha el formato que valida el firmware.
3. **Coste real:** cero cambios en C, cero riesgo para lo que ya corre en
   placa. La alternativa tocaba `bpvm_npack.h`, la escalera y su test.

📌 **Y mi objeción a la doble extensión era floja: el nombre NO es una segunda
fuente de verdad.** Es un **selector** para que el IDE elija antes de grabar; el
**validador** sigue siendo la cabecera, que lleva `arch` y `float_abi` y que
comprueba el dispositivo. Si el nombre miente, la escalera lo caza con
`E_ARCH`. Dos papeles distintos, no dos verdades.

⚠️ **El sufijo sale de la TABLA DE DESTINOS, no se teclea.** Si `ARMV8` se
escribe a mano en el build, otra vez en el IDE al elegir y otra en los docs, son
tres sitios para el mismo dato — que es el error que sí cuesta caro (#299,
#315). `NpackReloc.Destino` ya existe: que lleve su sufijo y que todos tiren de
ahí.

⚠️ **La doble extensión es SÓLO del PC.** Dentro de un `.pack` de BP el tipo de
entrada es un fourcc de **4 caracteres, minúsculas `[a-z0-9]`**
(`PackFormat.TYPE_LEN = 4` + `isLowerFourcc`) y se deriva de la ÚLTIMA
extensión: `sqlite.npk.RISCV` daría tipo `riscv`, cinco caracteres y en
mayúsculas — **lo rechaza al construir**. Lo que entra en el pack es el
ELEGIDO, ya renombrado a `sqlite.npk`.

**D2 — Un mecanismo, dos problemas.** La misma convención resuelve el `.mdn`,
que también es por arquitectura y hoy viaja suelto (era la idea de Eduardo del
11-ago para `miModulo.mdn.arm`). No hay que inventar nada aparte.

**D3 — Y el sufijo lleva el PERFIL, no sólo la familia.** `ARMV6` y `ARMV8` son
destinos distintos porque un Cortex-M0+ no ejecuta código de un M33 (ver abajo).
La convención lo soporta sin cambios; una llave `(arch, float_abi)` no.

**D4 — El `.npk` es un tipo de PRIMERA, no un recurso.**
Añadirlo a `OUTDIR_TYPES` de `PackStep`, junto a `.mod` y `.mdn`. Meterlo por
`resources/` "funcionaría", pero un motor no es un recurso: esa carpeta acepta
cualquier cosa sin validar y por contrato va detrás de los módulos. Como tipo
de primera entra además en las comprobaciones de «este pack se puede ejecutar»
que `PackStep` ya hace al construir (#361).

**D5 — Y va el PRIMERO.** No hace falta para que funcione (ver el modelo), pero
sí para que el reparto sea predecible: de primero su offset es `128 + N×48` y
sólo se mueve si cambia el NÚMERO de entradas; en medio se mueve si cambia el
TAMAÑO de cualquier cosa anterior. Diagnósticos más fáciles.

**D6 — UN PACK PARA CUALQUIER PLACA: lo distribuido es universal, lo grabado
es de una placa. Y quien poda es el IDE, al grabar.**

Idea de Eduardo (12-ago). Un solo `.pack` que se instala en cualquier sitio:

```
sqlite.pack  (lo que se reparte)      →  grabado en el P4
  npk  sqlite.RISCV                        npk  sqlite    ← relocalizado
  npk  sqlite.ARMV8                        mod  SQLite
  mod  SQLite          (portable)          mdn  SQLite    ← el de RISC-V
  mdn  SQLite.RISCV                     (los otros NO viajan)
  mdn  SQLite.ARMV8
```

⚠️ **Y `.npk` y `.mdn` son DISPARADORES INDEPENDIENTES** — matiz de Eduardo, que
corrige una simplificación mía («si tiene código nativo, el IDE lo transforma»).
No es uno, son dos, y un pack puede necesitar cualquiera de ellos, los dos, o
ninguno:

| lleva | qué obliga |
|---|---|
| `.npk` | relocalizar al grabar + quedarse con el de la familia |
| `.mdn` | **rehacer el pack quitando los `.mdn` de las otras familias** |
| ninguno | nada: viaja tal cual, universal LITERAL |

Un pack puede tener `.mdn` **sin** `.npk`: una librería con funciones `native`
que se apoyan en el puente AOT de la VM, sin motor externo que realojar. Ahí no
hay nada que relocalizar pero sí que podar.

**Por qué el IDE y no el micro.** El micro también podría buscar
`<Modulo>.<DESTINO>` y caer a `<Modulo>` — cabe en el nombre de entrada
(`NAME_LEN = 32`) y es poco código. Pero entonces cada placa cargaría en su
flash con los `.mdn` de las demás, y la tabla de destinos entraría en el
firmware. Criterio: **en la placa, lo que no está no puede fallar.**

Y el coste es el bueno: *«es una operación que se hace de vez en cuando, aquí el
coste de transformar es un poco de tiempo extra pero que no afecta a nada»*
(Eduardo). Además el IDE YA tiene que rehacer el pack por el `.npk`, así que la
poda del `.mdn` se apunta a un viaje que ya se hacía.

📌 **Lo que esto NO es**: una capacidad nueva. El `.mdn` dentro de un pack ya
funciona y está verificado en la Metro (11-ago): `AOT: SQLite.mdn cargado del
pack (3292 B, 16 thunks)`. El dispositivo lo busca por tipo y nombre
(`bpvm_pack_find(zona, len, "mdn", mod, ...)`), y con la poda sigue encontrando
exactamente lo de siempre. **Cero cambios en C.**

**D7 — El contrato del formato, en UN sitio y clavado por un test.**
Como `mdn_format.h`, y como ya hace `pack.py` hoy: aserto de tamano en el lado
Python + `offsetof` en `test_npack.c`. ⚠️ Con D1 el formato de flash NO cambia,
asi que esta pinza no hay que rehacerla — hay que NO ROMPERLA al portar a Java — Java y C tienen que romper la compilación, no la placa, cuando alguien
mueva un campo. Ver [[mdn-depende-del-layout-de-bpvm]] para lo que cuesta
cuando el contrato no está clavado.

### Orden de construcción

Herramientas primero; el pack es la consecuencia, no el objetivo.

1. **El relocalizador en Java**, con el oráculo contra `ld` como test. Sin
   tocar nada más: entra un ELF y una dirección, sale una imagen. Verificable
   en el PC, sin placa.
2. **El `.npk` sin relocalizar** como salida del build, UNO POR DESTINO con
   su tabla (D1: doble extension, cero cambios de formato). El test: que el dispositivo lo RECHACE con `E_SIN_RELOC` — que
   es lo que debe hacer con un pack sin grabar.
3. **`PackStep`**: `npk` como tipo de primera, colocado el primero.
4. **El grabado del IDE**: elegir FICHERO preguntandole a la placa que
   destinos acepta (y en que orden), relocalizar, sellar. Al `.pack` entra el
   elegido, ya renombrado a `sqlite.npk` (ver D1: el fourcc no admite el sufijo).
5. Y entonces el pack de SQLite sale solo — en ARM y en RISC-V, del mismo
   proyecto.

**D8 — UN BUILD, UN `.mod`, N `.mdn`. Y el C se emite UNA VEZ.**

El criterio es de Eduardo, y es más estrecho de lo que yo iba a hacer:

> «Un build, un `.bp`, un `.mod`, un código C, múltiples `.mdn`. Lo único que
> se ha de compilar varias veces es el código C intermedio.»

O sea que el proyecto declara sus familias —

```json
"aot": { "enabled": true, "targets": ["arm", "riscv"] }
```

— y de ahí sale:

```
    aot_SQLite.c          <- UNO, compartido por todas
      arm   -> aot_SQLite.arm.o                 -> SQLite.mdn.ARMV8
      riscv -> aot_SQLite.riscv.o -> .elf       -> SQLite.mdn.RISCV
```

**Por qué importa que el `.c` sea uno.** No es ahorrar tiempo: emitirlo por
familia serían dos pasadas del emisor, y dos pasadas son dos ocasiones de
divergir. Con una sola emisión es *imposible por construcción* que dos `.mdn`
del mismo pack no se correspondan entre sí ni con el bytecode que llevan al
lado. En la corrida real del SQLite los dos declaran **16 thunks**, que es esa
propiedad hecha número.

Consecuencias de diseño, todas pequeñas y todas con motivo:

- **El sufijo sólo aparece al construir un pack.** En un Run al dispositivo el
  `.mdn` sale pelado, porque el firmware busca `<Modulo>.mdn` en su FS. Son dos
  productos distintos con dos consumidores distintos, no una heurística.
- **`target` y `targets` a la vez es un error**, no una preferencia a adivinar:
  cualquier regla de precedencia que pusiéramos sería una que nadie recuerda.
- **Los nombres se validan al LEER el `.bpbuild`.** Escribir `risc-v` se ve en
  el fichero, que es donde se arregla — no tres pasos después, en forma de una
  familia que falta en el pack.
- **Los intermedios llevan la familia en el nombre siempre** (`aot_X.arm.o`).
  Con una da igual; con dos, el segundo gcc pisaría el `.o` del primero y el
  `.mdn` saldría con el código de la otra ISA. Eso no daría error en el PC:
  daría un cuelgue en la placa.
- **El constructor del pack no se entera de nada.** `PackStep` ya sabía leer la
  doble extensión (D1), así que no se tocó. Era la señal de que el reparto de
  responsabilidades estaba bien puesto.

El hueco donde encaja: `Main.PasoAntesDelPack`, entre compilar y empaquetar.
El AOT no puede vivir en el frontend —necesita las rutas de los toolchain, que
son de cada máquina y las guarda el IDE—, así que el frontend deja el hueco y
quien sabe compilar lo rellena. La alternativa era partir el build en dos y
duplicar el paso de empaquetado en los dos llamantes.

**Lo que el AOT NO hace: bloquear el pack.** Un pack sin `.mdn` corre
interpretado — más lento, no roto —, y ése es el criterio desde H12. Lo que no
puede es fallar callando, y por eso los avisos van a la consola.
