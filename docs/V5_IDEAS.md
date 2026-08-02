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
