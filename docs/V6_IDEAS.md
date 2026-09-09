# V6 — ideas y diseño

Documento hermano de `V3_IDEAS.md`, `V4_IDEAS.md` y `V5_IDEAS.md`: aquí se
registran las charlas de diseño de V6 **antes** de escribir código, que es el
método de la casa.

**V5 se publicó el 22-ago-2026** (release `v5.0`), así que V6 es la versión EN CURSO y
esto ya no es material aparcado: es el cuaderno de diseño vivo del hito. Guardado en
`docs/` el 23-ago al abrir V6, por decisión de Eduardo — hasta entonces vivía fuera del
repositorio porque era material de la versión siguiente.

El hilo de fondo de V6 ya está fijado en otros sitios y conviene tenerlo
delante al leer esto:

- `docs/CENSO_FAMILIAS.md` — el censo por FICHERO (hecho en V5) es la base; V6
  lo amplía a un censo **funcional** (Boot, SD, memoria, FS, GC, Packs, VM,
  drivers…) con cuatro ejes: implementado / específico-vs-común / capas
  respetadas / memoria y tiempos MEDIDOS.
- El reparto **común vs hardware**, para poder meter pruebas en medio. El
  modelo es el VFS de SQLite.
- Las fichas ya abiertas viven en `docs/FICHAS.md`, sección «Aplazadas a V6».

---

## Coser dos regiones de RAM en UN espacio de direcciones (Eduardo, 31-ago)

**La idea, con sus palabras**: *«un sólo modelo de memoria. Ahora añadimos un bloque ocupado
permanentemente que no podemos tocar (similar a la memoria reservada para el SQLite). Y ya
tenemos lo mismo pero con la infraestructura que ya hay implementada.»*

Sale de mirar el mapa de RAM del ESP32-C3 (`P1.C3.3` en FICHAS): 280 KB libres pero el bloque
contiguo mayor son 136 KB, así que la VM no puede pedir más de eso aunque sobre memoria.

### Lo que hace que la idea sea buena: las regiones NO están separadas

Lo que imprime la placa al arrancar:

```
heap_init: At 3FC98B10 len 000274F0 (157 KiB): RAM
           At 3FCC0000 len 0001C710 (113 KiB): Retention RAM
           At 3FCDC710 len 00002950 ( 10 KiB): Retention RAM
```

```
0x3FC98B10 + 0x274F0 = 0x3FCC0000   ← exactamente donde empieza la segunda
0x3FCC0000 + 0x1C710 = 0x3FCDC710   ← exactamente donde empieza la tercera
```

**Son contiguas.** La DRAM del C3 es un tramo seguido de ~281 KiB; ESP-IDF lo parte **por
capacidades** (las de retención sobreviven al deep sleep), no por direcciones. El techo de
136 KB es del contador del asignador, no del silicio — y eso es justo lo que esta idea explota.

### Por qué NO es «dos memorias», que ya sabemos que sale mal

Importa la distinción, porque la alternativa obvia —pilas en una región y heap en la otra— es
un camino que este proyecto ya recorrió y deshizo. La tabla de handles vivía en el `malloc` de
plataforma, y en `heap.c` está escrito lo que pasó:

> `synclisttest` moría con `No space in heap` **teniendo el heap al 20 %**: 200 KB parados
> mientras la tabla, que vive en otra bolsa, no podía crecer. **Dos memorias separadas que no
> se prestan nada.**

El 29-ago se metió **dentro** del bloque de la VM y eso lo arregló. La idea de Eduardo va en la
dirección contraria y por eso funciona: **sigue habiendo un solo espacio de direcciones** —una
referencia BP sigue siendo un desplazamiento dentro de `vm->memory`— y lo único que se añade es
que una parte de ese espacio no es nuestra.

### El hueco tiene sitio reservado desde `#451`

No hace falta mecanismo nuevo. `#451` separó `heap_top` de `stack_base` con esta nota:

> *«Son dos conceptos distintos: `stack_base` es dónde empieza la pila del main; `heap_top` es
> hasta dónde puede crecer el heap. Coinciden AHORA, y dejan de coincidir en cuanto la tabla de
> handles se aloje entre los dos.»*

Entre `heap_top` y `stack_base` ya hay una zona que no es ni heap ni pila. El hueco entre las
dos reservas es otro inquilino de esa misma zona.

### Lo que hay que MEDIR antes de que esto sea un plan

**Que los dos bloques caigan pegados.** El asignador no lo garantiza: se pide el mayor bloque
de la región A, se pide el de la B, y se comprueba

```c
    b == a + tam_a + hueco        /* hueco = cabeceras del asignador, unos pocos bytes */
```

- si caen pegados → `memory = a`, `memory_size = (b + tam_b) - a`, y el hueco se marca ocupado;
- si no → se usa sólo A, exactamente como hoy. **No se pierde nada por intentarlo.**

Esa comprobación es la que convierte esto de idea en plan, y es de una tarde en la placa que ya
está en la mesa.

### Lo que valdría

| | hoy | con esto |
|---|---|---|
| C3 | 136 KB (bloque contiguo) | ~250 KB |
| C6, S31 | por medir, misma familia | idem |

Casi el doble en el micro más justo del sobre, sin tocar el modelo de referencias, sin tocar el
GC y sin tocar el `sp`/`bp` que el debugger lee crudos por el wire.

⚠️ **Y una cautela**: la segunda región es *Retention RAM*. Hay que comprobar qué implica para
el deep sleep antes de meter ahí el heap de la VM — puede que nada (es DRAM normal que además
retiene), pero eso se mira, no se supone.


### 🔬 MEDIDO (31-ago): no hay «un hueco», hay treinta y siete reservas

La primera versión de esta idea decía que las regiones del C3 son contiguas y que el tope de
136 KB era «del contador del asignador». **Lo primero es cierto y lo segundo era una suposición
mía.** Eduardo lo vio: *«Ayer dijiste que el bloque de RAM del C3 es uno. Ahora parece como si
algo reservase RAM en medio. ¿Quién reserva esa memoria?»*

La cuenta no cuadraba y él tenía razón:

```
regiones (heap_init)   160 496 + 116 496 + 10 576 = 287 568 B
libre                                                280 032 B
⇒ ocupado                                              7 536 B   → el mayor hueco debería ser ~152 960
la placa dice                                        139 264 B   → faltan ~13,7 KB por explicar
```

Se le preguntó al chip (`heap_caps_get_info`, ahora una línea del arranque):

```
heap: libre 280032 | mayor 139264 | bloques: 7 libres, 37 usados, 44 total | usado 13424
```

📌 **Siete trozos libres, no uno ni tres.** Lo que los parte son **37 reservas** que suman
13 424 B: lo que **el propio ESP-IDF** ya tiene asignado cuando nuestro `app_main` arranca —
estructuras de tareas, pilas, drivers, el timer—, repartidas en el orden en que el sistema fue
arrancando. **Nadie reserva un bloque en medio**; hay 37 pequeños, y el mayor hueco que dejan es
el techo de la VM.

Que las regiones sean contiguas **en direcciones** era verdad y no bastaba: de ahí no se sigue
que el espacio LIBRE lo sea. Salté de lo uno a lo otro.

### Lo que eso cambia en la idea

**Peor**: el hueco entre los dos trozos grandes no son unos bytes de cabecera — son ~26 KB de
memoria **viva del IDF**, que además puede crecer.

**Pero la idea sigue en pie**, y con la misma forma: se piden los dos trozos (139 264 y 114 688),
se miran sus direcciones reales, y lo que quede entre medias se marca **ocupado permanentemente**.
No hay que tocarlo — sólo no escribir ahí.

### 🎯 El hueco tiene que ser un CONCEPTO DE LA VM, no un truco del arranque

Pregunta de Eduardo al aterrizarlo: *«¿el GC tiene algún problema con el orden?»*. Se miró, y el
GC está limpio — `gc_mark_phase` recorre sólo `[stack_base, sp)` de cada hilo y el barrido va
acotado por `heap_top`. Pero hay **tres** sitios que sí dan por hecho que todo el tramo es
nuestro:

| | qué hace hoy | qué pasaría con un hueco |
|---|---|---|
| **tabla de handles** (`heap.c`) | crece hacia abajo desde `stack_base`, con suelo en `heap_next` | **escribiría** dentro |
| **guarda del PC** (`interp.c:631`) | acepta cualquier `pc < memory_size` | **ejecutaría** memoria del IDF como bytecode |
| **debugger** (`bpvm_dbg_wire.c:106`) | lee crudo si `addr + n <= memory_size` | mostraría basura como memoria de la VM |

Los dos primeros son reales; el tercero es cosmético pero miente. De ahí que lo correcto sean dos
campos en la VM —`hueco_lo` / `hueco_hi`— que esas tres comprobaciones consulten. Así sirve para
el C3 y para cualquier micro futuro con la RAM repartida, que es lo que Eduardo quería del
mecanismo: *«nos da juego, para este micro pero también para otros futuros»*.

### El reparto, y por qué el orden lo decide el layout

Lo natural sería *«pilas en una región y heap en la otra»*, y el orden entre ellas da igual —
salvo por un hecho: **el espacio de direcciones de la VM tiene el heap ABAJO y las pilas
ARRIBA**. Como la región A está en direcciones más bajas, eso fuerza el reparto sin necesidad de
tocar el núcleo:

```
región A: [ modulos ][ heap →→→ ]  |  HUECO (del IDF)  |  región B: [ TABLA ][ pilas ]
                      heap_top                            suelo_tabla        stack_base
```

Y la tabla de handles necesita **un tope nuevo**: hoy su suelo es implícitamente `heap_next`
(`nuevo_top < vm->heap_next + vm->heap_reserve`), y pasaría a ser el principio del trozo de la
región B. Un campo y una comparación.

| | heap | pilas | útil | contra los 128 KB de hoy |
|---|---|---|---|---|
| heap en A, tabla+pilas en B | **136 KB** | 64 KB | **200 KB** | **+56 %** |

⚠️ Sigue faltando **medir que los dos trozos se pueden pedir a la vez** y con qué direcciones
salen: el asignador no promete nada. Se pide A, se pide B, se miran los punteros. Si el hueco
resultante es razonable se cose; si no, se usa uno solo y no se ha perdido nada.


### ✅ MEDIDO (31-ago): los dos trozos SE PUEDEN pedir, y el hueco son 8 KB

Era la única medida que separaba la idea de un plan — el asignador no promete que los dos trozos
mayores sean adyacentes, ni en qué orden salgan. Se pidieron los dos y se miraron los punteros:

```
coser: t1 @0x3fc9c414 139264 B | t2 @0x3fcc036c 114688 B
coser: tramo @0x3fc9c414..0x3fcdc36c = 261976 B | hueco 8024 B | util 253952 B
```

- **Se pueden pedir a la vez.**
- Salen **en orden de dirección** (el bajo primero), que es lo que el layout de la VM necesita.
- **El hueco son 8 024 B**, no los ~26 KB que se habían estimado. Ocho kilobytes de memoria viva
  del IDF entre los dos trozos.

| | bytes | |
|---|---|---|
| bloque de hoy | 131 072 | 64 KB heap + 64 KB pilas |
| tramo cosido, útil | **253 952** | **1,94×** |

⚠️ **Y la misma medida pone el límite**: tomando los dos trozos ENTEROS al sistema le quedan
26 080 B, contra un consumo medido en marcha de **17 588** (`U6.4`). Son 8,5 KB de holgura, que es
poco. Dejando el margen —digamos ~230 KB tomados— el heap quedaría en **~165 KB** contra los 64
de hoy: **2,6×**. Sigue siendo el cambio de memoria más grande que este micro puede tener.

### La forma final, que es la que Eduardo propuso desde el principio

El hueco **no** es «el espacio entre dos regiones»: es **un bloque más de la cadena del heap**,
marcado vivo para siempre — *«similar a la memoria reservada para el SQLite»*. La diferencia es
todo, porque el barrido del GC avanza con `cur += block_total_size(vm, cur)`: **lee la cabecera y
salta**. Con la cabecera al final del trozo bajo (memoria nuestra) y el tamaño cubriendo el hueco
(memoria del IDF), el recorrido pasa por encima sin tocarlo.

Y entonces **el heap es UNO** y abarca los dos trozos, con la disposición de siempre:

```
[ t1: modulos + heap →→→ ][ HUECO = bloque reservado ][ t2: …heap… ][ TABLA ][ pilas ]
```

📌 **Eso disuelve la pregunta del reparto.** Si el heap abarca las dos, no hay que decidir qué va
en cada región: la tabla y las pilas se quedan donde están y no hay «región del stack» que pueda
quedar grande. *(La primera lectura de esta ficha proponía repartir heap y pilas una por región,
que era peor y además exigía tocar el núcleo. La idea original era mejor que su lectura.)*

### Lo que hay que tocar, y no es poco pero está acotado

1. **El bloque del hueco tiene que sobrevivir al barrido.** Nadie lo va a referenciar, así que
   `gc_sweep_phase` lo vería «vivo pero sin marcar» y lo liberaría. Necesita un caso especial —
   una etiqueta propia, o `if (cur == vm->hueco)`.
2. **La tabla de handles** crece hacia abajo con suelo en `heap_next`: correcto tal cual, porque
   con el heap abarcando las dos regiones el suelo sigue siendo el heap.
3. **La guarda del PC** (`interp.c:631`) acepta cualquier `pc < memory_size` — con un hueco
   dentro, un PC desbocado podría ejecutar memoria del IDF. Hay que excluir el rango.
4. **El debugger** (`bpvm_dbg_wire.c:106`) lee crudo hasta `memory_size`: mostraría el hueco como
   memoria de la VM. Cosmético, pero miente.

Los cuatro miran lo mismo, así que lo que hace falta son **dos campos en la VM** (`hueco_lo`,
`hueco_hi`) y cuatro comprobaciones — no un mecanismo por sitio. Que es lo que le da el juego que
Eduardo le veía: *«nos da juego, para este micro pero también para otros futuros»*.


## Ficheros: no hay `seek` porque no hay `open` (Eduardo, 17-ago)

**La pregunta.** *«¿Tenemos una función seek o fseek para movernos dentro de un
fichero?»* No. Y el hueco es más grande que un `seek`: **BP no tiene el concepto
de fichero abierto**. Toda la API es de fichero entero.

| hoy | qué hace |
|---|---|
| `readFile(path)` → `string` | lee el fichero ENTERO |
| `readFileBytes(path)` → `byte[]` | ídem, en bytes |
| `writeFile(path, contenido)` | escribe el fichero entero (lo reemplaza) |
| `writeFileBytes(path, datos)` | ídem |
| `appendFile(path, texto)` | añade al final — la ÚNICA operación parcial |
| módulo `IO` | rutas, `mkdir`, `removeFile`, `rename`, `copyFile`, `fileSize`, `isDirectory`, `lastModified` |

No hay descriptor, así que no hay dónde colgar una posición.

**Por qué importa AHORA y no antes.** Mientras el FS era littlefs en flash
interna, un fichero cabía en RAM por construcción. Con las SD de V5/H1-H2 eso
se acabó: un log, un CSV o un volcado de datos en la tarjeta puede ser de
megas, y `readFile` exige metértelo entero en el heap. Hoy, en la práctica, **un
fichero más grande que el heap es un fichero que BP no puede tocar** — salvo
por SQLite, que sí sabe hacerlo porque no pasa por esta API.

**La buena noticia: la fontanería YA ESTÁ, y muy rodada.** La fachada del FS
tiene `read_at` y `write_at` (`include/bpvm_fs.h`), implementadas por las cinco
familias. No son código nuevo ni experimental: **SQLite las exige** —el propio
comentario del header lo dice, *«sin ellas no hay base de datos: una BD
reescribe la página N»*— así que llevan meses de uso duro en placa. Lo que falta
es sólo la superficie visible desde BP.

### DECIDIDO (Eduardo, 17-ago): un fichero es una CLASE

*«Un fichero para BP debe ser una clase, con los métodos habituales: open,
close, read, write, seek. Read tendrán que ser varios (readByte, readStr, etc.)
mientras que write se puede sobrecargar para diferentes tipos.»*

Queda descartada la variante sin handles (`readFileAt(path, off, n)`), que era
la otra opción sobre la mesa. Bien descartada, además: la trampa de #398
—`fat_read_at` hace `f_open`+`f_lseek`+`f_read`+`f_close` en CADA llamada, y por
eso el árbol tardaba 6.953 ms— desaparece sola cuando el fichero se abre UNA vez
y se queda abierto. Con handles no hay que inventar ninguna caché: la apertura
persistente **es** el handle.

**Hay molde en casa y no hay que inventarlo: `Net.Tcp`** (`bpstdlib/Net.bp`).
Sujeta un handle entero `h`, tiene `isOpen()`, un `close()` que lo pone a cero
—idempotente— y ya usa exactamente la forma que pide Eduardo: un método crudo y
su azúcar (`send`/`sendStr`, `recv`/`recvStr`). La asimetría read/write sale del
lenguaje, no del capricho: **BP no sobrecarga por tipo de retorno**, así que
leer necesita N nombres y escribir puede ser un solo nombre sobrecargado.

Forma de partida, a discutir:

```
class File
  public function File(path: string, modo: string)   // o open() aparte
  public function isOpen(): boolean
  public function close()

  public function seek(pos: integer)
  public function tell(): integer
  public function size(): integer
  public function eof(): boolean

  public function readByte(): integer          // -1 = fin
  public function readBytes(n: integer): byte[]
  public function readStr(n: integer): string
  public function readLine(): string

  public function write(s: string)             // sobrecargas por tipo
  public function write(b: byte[])
  public function write(n: integer)
  ...
end File
```

### DECIDIDO (Eduardo, 17-ago): DOS clases, y la segunda hereda

*«En Java tienen una clase para cada cosa: ficheros binarios, de texto, etc. En
otros lenguajes utilizan un tipo nada más. Aquí podemos tener un tipo `File` con
las funciones básicas y que trabaja a nivel de bytes. Después un 2º tipo que
herede del anterior y que trabaje con strings (UTF-8).»*

Punto medio entre el zoo de Java y el tipo único de C: **dos** clases, y la de
texto ES un fichero (hereda seek/tell/close, que valen igual). Y de paso
**disuelve la pregunta 1** —`write(42)`, ¿texto o binario?—: en `File` es
binario porque `File` es binario, y el texto vive en la subclase.

**La regla que lo deja limpio del todo: `TextFile` AÑADE sobrecargas, nunca
redefine el significado de una heredada.** Si `File.write(42)` escribiera 4
bytes y `TextFile.write(42)` escribiera `"42"`, la misma llamada haría cosas
distintas según el tipo dinámico, y un `TextFile` pasado como `File` no
cumpliría lo que promete su tipo estático. Con la regla, `write` significa
SIEMPRE crudo en toda la jerarquía. Lo formateado va por otro verbo — y hay uno
que ya significa exactamente eso en BP: **`print`**.

```
class File                       // BYTES, y nada mas
  public function File(path: string, modo: string)
  public function isOpen(): boolean
  public function close()
  public function seek(pos: integer)
  public function tell(): integer
  public function size(): integer
  public function eof(): boolean
  public function readByte(): integer            // -1 = fin
  public function readBytes(n: integer): byte[]
  public function write(b: byte[])               // siempre CRUDO
  public function write(n: integer)              // (falta decidir el ANCHO, ver abajo)
end File

class TextFile extends File      // UTF-8 encima, sin quitar nada
  public function readLine(): string
  public function readStr(n: integer): string    // n CARACTERES, no bytes
  public function write(s: string)               // sobrecarga NUEVA, no override
  public function writeLine(s: string)
  public function print(n: integer)              // FORMATEADO, como el print del lenguaje
  public function print(d: double)
end TextFile
```

`write(s: string)` no es un override: la base no tiene esa firma. El conflicto
sólo existía en los numéricos, y `print` lo desactiva.

### Lo que la herencia trae de nuevo (y hay que decidir)

**a. `readStr(n)`: ¿n bytes o n caracteres?** En UTF-8 no es lo mismo, y ahí está
justamente la razón de ser de la subclase: la base cuenta BYTES (`readBytes`) y
la de texto cuenta CARACTERES. Si `readStr` viviera en la base tendría que
mentir en una de las dos. Por eso arriba está sólo en `TextFile`.

**b. `seek` en un fichero de texto es en BYTES, y puede caer a media letra.** No
es un fallo del diseño —es lo que hace todo el mundo, y arreglarlo costaría un
índice— pero hay que ESCRIBIRLO: `seek` posiciona en bytes también en `TextFile`,
y quien salte a un offset arbitrario puede partir un carácter. Lo razonable es
que la siguiente lectura resincronice al principio del carácter siguiente y no
devuelva basura.

**c. Fin de línea, que es el clásico.** Un `.csv` escrito en Windows y leído en
la Metro trae `
`. Propuesta: `readLine` tolera los dos y NO devuelve el
terminador; `writeLine` escribe `
` y punto (uno solo, elegido, igual en las
dos VMs). Si alguien quiere `
` que lo escriba con `write`.

**d. Sigue abierto el ANCHO de `write(n: integer)`**: un `integer` de BP son 4
bytes, pero escribir un byte suelto es lo más común en binario. O `write(n)` son
4 bytes y hay `writeByte(n)` aparte, o al revés. Va con la pregunta del
endianness (abajo).

### Las preguntas que hay que contestar ANTES de escribirla

Por orden de lo que cuesta equivocarse. La que era la primera —`write(42)`,
¿texto o binario?— la resolvió el reparto en dos clases.

**1. ¿Qué pasa si no se cierra?** BP no tiene destructores, y el precedente
(`Net.Tcp`) tampoco resuelve esto: si el objeto se pierde sin `close()`, el
recurso nativo queda colgado hasta el reset. En un micro con pocas ranuras de
fichero abierto eso es un cuelgue diferido. La red que ya existe y hay que usar:
la VM **mide la memoria al final de cada RUN** (`#339`, dice quién se quedó qué
con fichero y línea) — lo natural es que el fin de RUN cierre lo que quede
abierto y lo DIGA, en vez de callar. Decidir si además hace falta algo en vivo.

**2. Cuántos ficheros a la vez, y qué pasa al pasarse.** Es una restricción de
micro, no de diseño: cada cintura tiene lo suyo (FatFs está hoy con
`FF_FS_LOCK 0`, o sea sin control de aperturas duplicadas). Hay que fijar un
número por familia y que pasarse sea un error atrapable, no un fallo raro —
misma lección que el tope de la tabla de handles (`#430`): el «no puedo» tiene
que ser honesto y temprano.

**3. Endianness de los tipos multibyte** (y con ella el ancho de `write(n)`, punto **d** de arriba). El `.mod` es big-endian por dentro,
pero un fichero de datos lo lee otro programa: hay que ELEGIR y escribirlo, no
heredarlo por accidente. Y sea cual sea, igual en las dos VMs.

**4. Detalles que parecen menores y luego no lo son:** el modo de apertura
(lectura/escritura/append/crear/truncar) y su ortografía; si `seek` es absoluto
o admite origen (inicio/actual/final); qué devuelve `readLine` al llegar al final;
si escribir más allá del final extiende y con qué se rellena el hueco.

### Lo que hay debajo, y lo que habrá que añadir

`read_at`/`write_at` de la fachada (`include/bpvm_fs.h`) resuelven el movimiento
de datos y están rodadas por SQLite. Lo que NO existe es el **descriptor**: la
fachada trabaja por PATH. Así que la implementación tendrá que llevar una tabla
pequeña de ficheros abiertos (path + handle nativo + posición) indexada por el
entero que sujeta la clase — igual que hace la capa TCP con sus sockets. Ahí es
donde se paga la apertura persistente, y donde deja de aplicar la trampa de
#398.

Y en **miVM** es directo (`RandomAccessFile`), así que la paridad no será el
problema: el problema es que la semántica de los cinco puntos de arriba esté
escrita ANTES, para que las dos VMs implementen lo mismo y no dos primos.


---

## El `.mdn` se funde en el `.mod`, con un bloque nativo por familia (Eduardo, 17-ago)

**La idea.** Igual que el `.bpi` desapareció dentro del `.mod` en V4/H6.a, que el
`.mdn` haga lo mismo. Un `.mod` tendría su bloque de código BP de siempre y,
además, **cero o varios bloques de código nativo, uno por familia**. Al compilar
desde el IDE con la placa conectada se genera el nativo de ESA familia; si hay
un proyecto abierto que declara varias, se generan las N y van todas dentro. **Al
micro no le llega el `.mod` gordo**: el IDE poda y le manda sólo el bloque de su
familia. Los packs, igual.

### Por qué esto no es una apuesta: ya funciona, en otro sitio

Hay **dos** precedentes, y el segundo es literalmente este mecanismo:

1. **H6.a**: el `.bpi` se fundió en el `.mod` (v6 autodescriptivo) y se borraron
   71 ficheros. Mismo movimiento, ya hecho una vez y salió bien.
2. **El pack de SQLite ya lleva DOS familias dentro y el IDE lo poda al
   grabar.** No es una analogía: es el mismo flujo.
   - el proyecto declara las familias en **`aot.targets`** — el concepto ya
     existe, no hay que inventarlo;
   - `AotBuild.buildPackTargets` emite **un `.mdn` por familia**, y —esto es lo
     bueno— **el `.c` intermedio se emite UNA sola vez y lo comparten todas las
     toolchains**. Su propio comentario dice por qué: *«no es un ahorro de
     tiempo: es que así los `.mdn` del pack no pueden divergir entre sí ni del
     bytecode que llevan al lado»*;
   - `PackBurn` se queda con el de la placa, le quita el sufijo y poda el resto:
     *«el micro encuentra `SQLite.mdn` de siempre y no se entera de que hubo
     hermanas»*. Medido: 1.122.304 B en disco → 569.344 en la placa.

   Hoy todo eso se apoya en el NOMBRE del fichero (`SQLite.mdn.RISCV`). La idea
   es subirlo al formato, donde se puede validar.

### Lo que MATA (y esto es lo que lo justifica, más que la comodidad)

Dos ficheros que tienen que ir juntos son dos ficheros que se pueden desparejar,
y ya nos ha pasado de las dos maneras posibles:

- **Desfase en el tiempo.** Hoy mismo: *«[Explorer] AOT `AotGcRt.mdn` es MÁS
  VIEJO que su `.mod` — NO se sube»*. Existe un guardián porque hace falta; con
  un solo fichero no habría de qué guardarse.
- **Desfase de familia.** Se subió un `.mdn` de ARM a la P4
  ([[artefacto-de-otra-familia-se-cuela]]): el `arch` está en el fichero y la
  placa dice el suyo, pero nadie los comparaba. Con bloques etiquetados dentro
  del `.mod`, elegir el equivocado deja de ser posible **por construcción**.

Es el mismo tipo de argumento que cerró H6.a: no «un fichero menos», sino **una
clase entera de fallo que deja de existir**.

### Lo que hay que decidir, y una trampa concreta

**⚠️ La trampa: el IDE compara para NO subir, y si poda, compara mal.** Hoy el
Explorer dice *«`/lib/Gui.mod` ya en FS (43077 bytes, contenido idéntico), salto
PUT»* — compara el fichero LOCAL con el del device. En cuanto el IDE pode antes
de enviar, el local y el del device **dejan de ser el mismo fichero a
propósito**, y esa comparación empieza a mentir: o resubiría siempre, o peor,
saltaría cuando no debe. Hay que comparar contra los **bytes podados**, no
contra el fuente. Y lo mismo mira el chivato del `/lib` (`#422`), que compara el
módulo del FS con el embebido en la imagen.

**Una sola implementación de la poda.** `PackBurn` ya poda; el camino del `.mod`
necesitará lo mismo. Si acaban siendo dos funciones parecidas, es
[[arreglo-que-no-viaja-entre-familias]] otra vez — y hoy hemos tenido tres casos
de esa forma en un día. Debe ser UNA, llamada desde los dos sitios.

**¿La poda es obligatoria o una optimización?** Merece decisión explícita. Si el
cargador del device sabe **saltarse** los bloques de otras familias, un `.mod`
gordo copiado a mano en una SD sigue funcionando (sólo ocupa más); si los
rechaza, es un error duro. Lo robusto parece: el device TOLERA y usa el suyo, y
la poda existe para no gastar flash — que en un micro no es poca cosa, pero es
un problema de tamaño, no de corrección.

**El gate de ABI (`#284`) y las copias rancias.** Cambiar el formato del `.mod`
sube su versión, y eso está bien —el loader rechaza lo incompatible y el desfase
GRITA en vez de corromper—. Pero arrastra el trabajo conocido: hay **cuatro
copias de los `.mod` de la stdlib** que se quedan rancias, incluida
`packs/Stdlib.pack` ([[stdlib-mod-version-skew-oo-device]]). Regenerarlas es
parte del hito, no un fleco.

**Qué hace el host con un `.mod` multifamilia.** En PC no hay `.mdn` (las
`native` corren interpretadas), así que las dos VMs de host deben **ignorar**
los bloques nativos sin quejarse, sea cual sea su `arch`. Es la prueba barata de
que el formato es tolerante.

**Dónde se declaran las familias.** Ya está: `aot.targets` del proyecto. Sin
proyecto abierto, la familia es la de la placa conectada — que es exactamente lo
que hace hoy el IDE (*«target riscv — lo dice la placa»*).

---

## `double` en funciones `native` — helpers, igual que hizo `long` (Eduardo, 17-ago)

Es la ficha **`#426`**, aplazada a V6 el 16-ago. Hoy el emisor lo rechaza a la
cara: *«AOT: tipo 'double' no soportado en thunk native (ficha #426)»*.

**La propuesta.** *«Al código C generado, si hace falta, le añadimos las
funciones C que necesite de las operaciones de double. De momento, esas
funciones internamente lo que harán es llamar a los opCodes que corresponda. No
es muy óptimo en rendimiento pero nos sirve mientras no encontremos una solución
mejor.»*

### El patrón ya está probado: es lo que resolvió `#381`

Con `long` pasó exactamente lo mismo y se resolvió así (idea de Eduardo también,
en su día): el micro no tiene división de 64 bits en hardware, el `/` de C se
convertiría en una llamada a libgcc, **y un `.mdn` no puede resolver símbolos
externos** — es código relocalizable puro, sin enlazador al cargar. La salida
fue `idiv64`/`imod64` en la tabla `aot_helpers`. Su comentario dice el porqué y
también el cuidado que hay que tener:

> *«⚠️ Y hacen EXACTAMENTE lo que hace el intérprete, no lo que sería "más
> correcto": mismo chequeo de divisor cero, mismo mensaje… Si el camino
> compilado fuera más listo que el interpretado, el mismo programa daría dos
> resultados según llevara `.mdn` o no — que es justamente el invariante que no
> se puede romper.»*

Eso vale igual aquí, y con `double` **pica más**: NaN, infinitos, el redondeo,
el `-0.0`, qué pasa al convertir un `double` fuera de rango a `integer`. Cada
uno de esos es una oportunidad de que el `.mdn` y el intérprete difieran en un
bit. La regla es la misma: el helper no implementa la operación, **comparte la
implementación del intérprete**.

### Una precisión que abarata mucho la idea

«Llamar al opcode» no significa meterse en el intérprete: el helper es **C
normal compilado DENTRO del firmware**, y el firmware sí está enlazado contra
libgcc. O sea que `h_dadd(a,b)` es literalmente `return a + b;` — el `__adddf3`
lo resuelve el enlazador del firmware, que es lo que el `.mdn` no puede hacer.
No hay que reimplementar coma flotante por software: hay que **prestarle al
`.mdn` la que el firmware ya tiene**.

### Cuántos helpers, y el detalle de que aquí NO basta con la división

Con `long` sólo hicieron falta dos (`/` y `mod`): sumar y multiplicar los hace
GCC en línea. Con `double` **en las familias de hoy no hay ni una operación
gratis**, porque ninguna tiene FPU de doble precisión:

| familia | FPU | `double` |
|---|---|---|
| RP2350 (Cortex-M33) | simple precisión (FPv5-**SP**-D16) | software |
| ESP32-P4 (RISC-V) | ABI `ilp32f` = float en registros | software |
| STM32U5 (Cortex-M33) | simple precisión | software |

Así que la lista es la de libgcc soft-float: `+ - * /`, negar, las seis
comparaciones y las conversiones (`double`↔`integer`, `double`↔`long`,
`double`↔`float`). Unas 15-20 entradas. La tabla `aot_helpers` **sólo crece por
el final** (prefijo congelado, `#158`), así que añadirlas es aditivo y no rompe
ningún `.mdn` ya grabado.

### Y una simetría que conviene ver antes de llamarlo «poco óptimo»

Eduardo lo da por lento a propósito, pero el número puede sorprender **a favor**:

- **Donde `double` es software** (todas las placas de hoy), una operación cuesta
  ya decenas de ciclos. El coste extra de la llamada al helper —una decena— es
  un recargo del orden del 20-30 %, no un desastre.
- **Donde `double` fuera hardware** (un STM32F7/H7 con FPU de doble, que Eduardo
  mencionó), la operación costaría 1-3 ciclos y la llamada 10-20: ahí sí, un
  factor 5-10.

O sea que **la solución barata es barata justo donde `double` ya es lento, y cara
justo donde sería rápido**. Eso le da a la «solución mejor» un disparador claro y
medible en vez de una intuición: emitir la instrucción nativa en lugar del helper
**sólo si el micro tiene FPU de doble precisión**. Y parte del mecanismo para
saberlo ya existe — el `.mdn` lleva su ABI de coma flotante en la cabecera
(`ilp32f`), precisamente porque una discrepancia de ABI de FP no da error de
enlace sino resultados mal.

### La ganancia esperada, ESTIMADA con datos reales (18-ago)

La pregunta de Eduardo —*«si ya hay una emulación de operaciones double, ¿no
podemos hacer que las native la aprovechen?»*— es exactamente este diseño. Y
`samples/DblBench.bp`, escrito ese día para otra cosa, permite estimar lo que se
ganaría **antes** de invertir el trabajo:

| Metro, 200.000 vueltas × 4 operaciones | ms |
|---|---:|
| bucle de `double` **interpretado** | 4048 |
| bucle de enteros (≈ el coste del INTÉRPRETE) | 2835 |
| **la aritmética de coma flotante en sí** | **~1213** (1,5 µs/op) |

Una `native` quita el despacho del intérprete —esos 2835 ms— pero **la
aritmética se queda igual**, porque acaba en las MISMAS rutinas a través de los
helpers. Sumando la llamada, saldrían unos 1300-1400 ms contra 4048: **≈3×**,
no 10×.

Eso no descarta la feature —3× medido es una ganancia real— pero **cambia cómo
se cuenta**: el suelo lo pone la emulación, no el intérprete, y quien marque
`native` esperando que un cálculo de `double` vuele se llevará una decepción.
Conviene que eso esté escrito en el manual el día que entre.

**Corolario de la decisión de `L14`**: al quedarse con las rutinas optimizadas
del RP2350 (las que aplastan subnormales), los helpers heredan ESA velocidad. Si
se hubiera cambiado a las de libgcc, el `double` en `native` habría nacido un
80 % más lento en su parte cara — la decisión de hoy abarata la feature de
mañana sin que nadie lo pretendiera.

Aun así, la medida de verdad sigue pendiente para el primer día del trabajo:
cronometrar un bucle de `double` en una `native` con helpers contra el mismo
bucle interpretado. La estimación de arriba es aritmética sobre datos reales,
que es mucho mejor que una intuición — pero sigue sin ser una medida.

---

## `run miModulo <arg>` — el argumento, SIEMPRE en el heap (Eduardo, 18-ago)

Es la ficha **`#412`**, movida a V6 por decisión de Eduardo: *«no es nada
urgente ni crítico»*. Pero el diseño queda cerrado aquí, que es lo que cuesta.

**De dónde viene.** `#386` arregló que el argumento de `Main` saliera de su
VALOR POR DEFECTO en vez de ser siempre `""`, y su propio comentario en
`MivmEmitter` dejó dicho lo que faltaba: *«Pasar el runtime el argumento DE
VERDAD (wire + IDE + los 3 firmwares) sigue pendiente aparte»*. Eso es esto.

**La idea de Eduardo, que es la que lo simplifica:** *«si el argumento se sube
al heap y se le pasa la referencia, el caso sin argumentos se convierte en un
caso con argumento `""` también subido al heap; en realidad los dos casos son
casi iguales.»*

**Y por qué importa más de lo que parece.** No es sólo tener un camino en vez de
dos: hoy los dos casos producen **tipos de cadena distintos**. El argumento
horneado es un literal de la ZONA DE DATOS —sin cabecera de bloque y con
dirección por debajo de `heap_start`— y uno de ejecución sería una cadena del
HEAP. Son las dos formas que hubo que reconocer en `#389` para el `CHECKCAST`.
Si `Main` recibe una u otra según cómo se lance el programa, todo lo que haga
con ella (guardarla en un campo, concatenar, dejar que la vea el GC) recorre
caminos distintos — la clase de diferencia que acaba en «funciona desde el IDE y
falla en la placa». Con la propuesta, **siempre es del heap** y la asimetría no
llega a existir.

### La forma

- La VM tiene *el argumento de ejecución*: una cadena, vacía si nadie la dio.
- `__startup` sigue conociendo el valor por defecto (lo dice el fuente) pero ya
  no lo empuja: se lo **pasa a un builtin**, que devuelve la referencia buena —
  el argumento de ejecución si lo hay, y si no una copia del defecto **subida al
  heap igualmente**.
- `Main` recibe siempre una referencia del heap, venga de donde venga.

Lo que CONSERVA, y por eso es seguro: sin argumento y sin defecto el programa
recibe `""` en el heap en vez de `""` en datos — mismo valor y mismo
comportamiento observable. Un programa que hoy funciona no se entera.

### Lo que cuesta, contado

| dónde | qué |
|---|---|
| emisor | una línea distinta en `__startup` (`MivmEmitter`) |
| las 2 VMs | un builtin que resuelve arg-de-ejecución-o-defecto, y lo aloja |
| wire | campo `arg` en `RUN` — **cuatro** implementadores: Pico, ESP32, STM32 y el simulador |
| CLI | los dos hosts |
| IDE | `run X <arg>` en la consola y el «Run on Device» |

**Lo caro no es el mecanismo: es que abre el protocolo.** Por eso encaja en V6 y
no en el cierre de V5 — y encaja además con `#434` (desacoplar los eventos) y
con fundir el `.mdn` en el `.mod`, que ya van a mover ese mismo terreno.

---

## La RAM del código nativo de un pack: ¿stack, heap, o arena aparte? (Eduardo, 21-ago)

Pregunta de Eduardo, planteada mirando a V6/V7: *«cualquier código C que metamos en un
Pack necesitará algo de RAM para trabajar. Al final habrá que hacer como los módulos que
están en un Pack: el código se queda quieto pero se emplea un poco de RAM para cada uno.
La cuestión es de dónde sale esa RAM: del stack, del heap, o una tercera vía — un espacio
reservado con su propio malloc aparte del gestionado por el Heap.»*

### La respuesta corta: la tercera vía YA está elegida, y corriendo

No es una decisión pendiente: es lo que se hizo para SQLite en V5/H3, y el porqué está
escrito en el código.

- **`bpvm_bios.h:74`** separa las dos cosas sin ambigüedad: *«el del pack, NO el heap BP
  (son de formas distintas: `bpvm_heap_alloc` devuelve handles con tipo y sujetos al GC;
  esto quiere punteros crudos). Sale de la arena reservada por el ENV.»*
- **`bios_pico.c:12`** explica por qué NO se apuntó al heap del sistema: *«el pack
  comería del heap de FreeRTOS, justo lo que la arena separada existe para evitar»*.
- **`bpvm_sqlmem.c`** es la REGLA de cuánta se reserva, con los números medidos.
- Y en placa se ve: `bd: reservada (SQLite=2) -> 2048 KB @ 0x11000000`.

**Por qué NO el heap de BP** (y esto ya no hay que volver a discutirlo): el heap reparte
*handles* con generación, sujetos al GC y movibles. El C nativo quiere *punteros crudos*
que no se muevan bajo sus pies. Son formas incompatibles, no una preferencia.

**Por qué NO la pila**: es de tamaño fijo y contado por hilo (`Pila VM: 12 KB sin usar de
16 KB`), y una biblioteca C que reserve por su cuenta la desborda sin aviso. Lo que la
pila NO da es lo que estas bibliotecas piden: vida más larga que la llamada.

### Lo que de verdad está abierto (y esto sí es V6/V7)

1. **La arena de hoy es SINGULAR y se llama `SQLite`** — la clave del ENV y la ranura de
   la BIOS (*«LA ARENA DE LA BD»*). Para N packs hace falta identidad.
2. **SQLite se queda la arena ENTERA** y la gestiona con su propio asignador (MEMSYS5).
   Un segundo pack nativo hoy se pelearía con él.
3. **`malloc`/`free`/`realloc` de la BIOS son CHIVATOS, no implementaciones.** Ese es
   justo el camino que necesita *«cualquier código C»* que no traiga asignador propio, y
   el código ya dice que llegará: *«saldrán de la arena del ENV cuando toque»*. **Ese es
   el trabajo concreto**, y hoy en la Pico devuelve NULL gritando.
4. **Quién decide los tamaños.** Hoy: una clave de ENV por función. Con N packs son N
   claves, y eso choca de frente con *«un microcontrolador no es un barco»*.

### Una propuesta, para discutir — NO decidida

**Una sola arena, con el asignador dentro de la VM y contabilidad POR PACK.**

- Se mantiene la frontera que importa —ni el heap de FreeRTOS ni el de BP—, que es lo que
  la arena existe para proteger.
- Una sola clave de ENV (`nativo=<MB>` o similar) en vez de una por biblioteca.
- La contabilidad por pack da lo que hoy dan los chivatos, pero en régimen permanente:
  **quién se está comiendo la arena**, con nombre. Hoy eso sólo se sabe cuando peta.
- SQLite seguiría pudiendo pedir un bloque grande y usar MEMSYS5 dentro: *un cliente más*
  de la arena, no su dueño.

⚠️ **Lo que hay que mirar antes de comprarla**: un asignador de propósito general en la
VM es código nuevo en el camino crítico, y la fragmentación con clientes de vidas muy
distintas (LVGL pinta y suelta; SQLite retiene páginas) es exactamente donde estos
esquemas se rompen. Merece medirse antes que escribirse.

📌 **Y el contexto de Eduardo**: *«SQLite es un buen modelo pero habrá que mejorarlo»*, y
**LVGL a un pack es V7**. O sea que el segundo cliente de la arena ya tiene nombre y
fecha aproximada: cuando llegue, el punto 2 de arriba deja de ser teórico.

## El REPL común (U3): partir en dispatcher + operaciones de familia (26-ago)

> **Estado: PARA DECIDIR con Eduardo.** U3.0 (la matriz verbo × familia) está medida y en
> `FICHAS.md`. Esto es el boceto de diseño ANTES de escribir código, siguiendo el método:
> las charlas de diseño se escriben aquí y el código llega después de la decisión.

### Lo que la matriz permite afirmar

- **El contrato de facto existe**: 20 verbos idénticos en las tres familias, protocolo
  escrito en `BPVM_WIRE_PROTOCOL.md`, y desde U2 los tres REPL hablan por el MISMO wire
  (builders comunes + 4 funciones de cable por familia).
- Lo que difiere de verdad por familia, verbo a verbo, cae en tres cubos:
  1. **nada** (PING, TIME, DEL, MKDIR, STAT, GET, PUT*, LOG_*…): tocan el wire y la
     fachada de FS, que ya son comunes;
  2. **datos de placa** (INFO, STATE, DF): el flujo es igual y cambian los CAMPOS que
     aporta la familia;
  3. **el flujo entero** (RUN, y los verbos de hardware BOOTSEL/SD_*): enredado con el
     scheduler, el autorun, el poll de KILL, el debug… — el hueso.

### La forma propuesta (espejo del patrón que ya funcionó en U2)

- `src/bpvm_repl.c` — el dispatcher y los verbos de los cubos 1 y 2, UNA vez.
- `bpvm_repl_ops_t` — la cintura de familia: un struct de punteros con lo que el común no
  puede saber (rellenar los campos de INFO, ejecutar un RUN, los verbos extra de hardware).
  Mismo patrón que la cintura del log o el VFS de SQLite.
- Cada familia queda en: su cable (U2) + su `ops` + sus verbos propios. El objetivo de
  tamaño: que `stm32_repl.c` pase de 920 líneas a ~200 de cintura.

### La pregunta que decide el orden (para Eduardo)

**¿Migración 1:1 primero y los verbos que faltan después, o aprovechar la migración para
añadirlos?** — Contestada con la segunda pasada de Eduardo (*«hay que ver si esos comandos
están implementados o no»*, U3.0b en FICHAS): el «gratis» se midió verbo a verbo. Da
funcionalidad REAL en `RENAME` (fachada común) / `FORMAT` (primitiva presente en ESP32) /
`SAVE` (no-op legítimo en STM32); semántica-v1 coherente en `RMDIR` (stub también en la
Pico); y NO resucita `PROMPT`, que está muerto en toda la VM-C (`IO.prompt` sin
implementar — fichado aparte). Así que: **migrar 1:1 el núcleo, los verbos con primitiva
entran solos, y lo muerto no se disfraza**. El orden de familias, el ya escrito en U3.3: STM32 → S3 → Pico, de menos a
más.

### Los riesgos que ya se ven

- **RUN es el hueso** (cubo 3): cada familia lo tiene entretejido con su scheduler/autorun/
  poll. Propuesta: RUN se queda EN LA FAMILIA en la primera pasada (es un verbo del `ops`),
  y sólo migra cuando los otros 19 estén en el común y verificados en placa.
- **El INFO por campos**: la tentación de un INFO común con `#ifdef` por familia. Mejor un
  callback `ops->info_campos(writer)` — el común pone el sobre, la familia mete lo suyo.
- **Verificación**: la de siempre del cordón — el IDE contra cada placa tras cada tanda,
  y `LIST_DIR`/`INFO`/`PACK_LS` byte-comparables antes/después donde no cambie nada.


---

## Hilos de ejecución: ¿cuántos queremos de verdad? (Eduardo, 4-sep)

**El planteamiento de Eduardo:** *«Después de unificar código parece que cada micro va a su
aire. Ahora lo que necesitamos es analizar realmente cuántos hilos de ejecución queremos. A mí
una cosa que se me plantea es que la VM no debería ocuparse nada más que de interpretar los
opcodes. Ponerle muchas tareas ahí supone ralentizar la ejecución de los programas. Pero es
una teoría, deberíamos medirlo.»*

### El censo (leído del código, 4-sep)

| Familia | Tareas nuestras | Qué corre en la tarea de la VM | Del sistema |
|---|---|---|---|
| ESP32-C6 / C3 / S3 (`esp32/common`) | **1**: la `main` de ESP-IDF entra en `repl_esp32_run()` y no vuelve | lector del wire + REPL + VM + bombeo de LVGL (C6) | C6/C3: `IDLE`, `esp_timer`, `Tmr Svc` (≈4 en total); S3: + `IDLE1`, `ipc0`, `ipc1` |
| ESP32-P4 | 1 ó 2: `wire_uart` (o `wire_v1` TCP, nunca las dos) + `tcp_log` opcional | lo mismo, dentro de la tarea del wire | + event loop, `tiT` (lwIP), Ethernet |
| Pico 2 | **1**: `vm_task` | wire + REPL + VM | `IDLE`, `Tmr Svc` (≈3). El camino SMP (workers + comm task) sólo con `-DBPVM_PICO_SMP_WORKERS`, que el build normal no define |
| STM32 Nucleo / Discovery | **0**: bare-metal, superbucle + UART por interrupción | todo | — |

Los hilos de BasicPlus son **verdes**: los turna el scheduler de la VM dentro de esa única
tarea (`scheduler.c`; «sin pthreads adicionales»). `platform_esp32.c` sabe crear tareas de
FreeRTOS (`bpvm-thread`, 4 KB) pero sólo las pide el scheduler SMP y la comm task, y en ESP32
no se usan.

### Qué hace la tarea de la VM además de interpretar, y cada cuánto

Entre cuanto y cuanto (**1024 opcodes** por defecto; `quantum=N` por ENV desde `#462`), el lazo
de `scheduler.c` hace: `events_revive_terminated` → **`poll_cb`** (en ESP32
`usb_serial_jtag_read_bytes(…, 0)`: ¿ha llegado un KILL/HELLO?) → **`taskYIELD()`** (`#462`) →
`wake_expired_sleeps` → `wake_completed_joins` → `pick_next_runnable` → `event_drain_one`. Y
**dentro** del cuanto, cada `print` codifica un mensaje `OUTPUT` en JSON y lo escribe al
transporte **síncronamente** (`usb_serial_jtag_write_bytes` con tope `WIRE_TX_MS`).

### Experimento 1 — lo de ENTRE cuantos (sin tocar código: `quantum` por ENV)

`Bench.mod` (fib(28) interpretado dos veces, puro cálculo, el wire callado) en la C6 con GUI,
imagen 128 KB; el cronómetro es el `elapsedMs` del `EXITED`:

| cuanto (opcodes) | elapsedMs | vs 65 536 |
|---|---|---|
| 65 536 | 23 390 / 23 400 | — |
| 8 192 | 23 430 | +0,2 % |
| **1 024 (defecto)** | **23 640 / 23 640** | **+1,0 %** |
| 128 | 25 420 | +8,6 % |

Lineal con el número de fronteras (128 = 8× fronteras → 8× el coste: 2 030 ms frente a 250).
Con ~30 M de opcodes en la pasada salen ~30 000 fronteras → **unos 8 µs por frontera**
(≈1 300 ciclos a 160 MHz: la llamada al driver USB, el `taskYIELD` y la contabilidad del
scheduler). **Conclusión: lo que la tarea hace entre cuantos cuesta ~1 %.** Sacarlo a otra tarea
no aceleraría los programas de forma medible; y subir el cuanto a 8 192 lo deja en 0,2 % a
cambio de atender un KILL 8× más tarde (que sigue siendo < 1 ms).

### Experimento 2 — la SALIDA dentro de la tarea (`samples/benchmarks/PrintBench.bp`)

Dos bucles iguales de 2 000 vueltas, el segundo con un `print` de ~50 caracteres por vuelta;
el propio programa cronometra con `Pico.uptimeMs`:

```
RESULTADO sin print: 21 ms; con print: 1700 ms; lineas: 2000
mensajes OUTPUT: 11 944 | bytes de data: 122 072 | bytes crudos recibidos: 815 984 | EXITED elapsedMs 1720
```

**0,84 ms por línea impresa** — 80 veces el bucle que la produce. Y el porqué está en los
otros dos números: cada `print "a", i, "b"` salió en **seis** mensajes `OUTPUT` (uno por
argumento y otro por el salto de línea), cada uno con ~60 B de marco JSON, así que **122 KB de
texto viajaron como 816 KB** (6,7×) en 11 944 escrituras al driver USB; a ~480 KB/s, que es lo
que da el USB-Serial-JTAG con fragmentos de ese tamaño, son 1,7 s. El intérprete no gasta ese
tiempo codificando: lo gasta **esperando al transporte**, porque la escritura es síncrona.

### Lo que dicen las dos medidas juntas

1. **La teoría se sostiene en la salida, no en el resto.** Lo que la tarea de la VM hace
   «además de interpretar» cuesta ~1 % cuando el wire calla, y **0,84 ms por línea** cuando el
   programa habla. El freno es la salida, y es un freno de *espera*, no de CPU.
2. **El número de hilos no es la palanca principal; el formato sí.** Un mensaje `OUTPUT` por
   línea (o por cuanto, o por N bytes acumulados) en vez de uno por argumento divide por seis
   las escrituras y por ~5 el tráfico, sin tocar la arquitectura de tareas. Es lo primero que
   habría que hacer, y se mide con el mismo `PrintBench`.
3. **Una comm task con cola** (la que ya existe para el camino SMP de la Pico: `bpvm_oq_*` +
   `comm_task_entry`) desacopla al intérprete del transporte: el `print` deja el texto en la
   cola y sigue; sólo se bloquea cuando la cola se llena, es decir, cuando el programa produce
   más deprisa de lo que el USB traga — y ahí no hay hilo que lo arregle. Vale para la
   latencia del intérprete en programas que imprimen a ráfagas; no cambia el caudal.
4. **El wire de entrada** (poll entre cuantos) es barato como está; lo que `#462` señala —que
   el SO no respira mientras la VM trabaja— ya tiene su `taskYIELD`, y una tarea propia del
   wire a mayor prioridad sólo aportaría atender un KILL en mitad de un cuanto largo.
5. **Unificar el reparto entre familias** es otra cuestión, de forma: hoy Pico (`vm_task`),
   ESP32 (`main`) y P4 (`wire_uart`) hacen lo mismo con nombres y pilas distintas (16 KB, 8 KB,
   32 KB). Con estas medidas, el reparto «una tarea de la VM + una comm task con cola» es el
   único que tiene un porqué medido, y sería el mismo en las tres familias con FreeRTOS.

⏭️ Siguiente paso, si se decide seguir: (a) `OUTPUT` por línea → medir con `PrintBench`;
(b) la comm task con cola en `esp32/common` reutilizando `bpvm_oq_*` → medir otra vez;
(c) sólo entonces decidir el reparto común de tareas. Cada paso con su número antes y después.

### La decisión (Eduardo, 4-sep, tarde)

*«Lo veo bastante anárquico, sin un orden claro. Y todos los micros deberían funcionar más o
menos igual. Creo que el hilo de la VM debería dedicarse solamente a ejecutar los opcodes. Hace
falta al menos un segundo hilo de ejecución. Y la misma arquitectura en todos los micros.»* Y el
rumbo: *«En un futuro tendremos dos VM y una cola de threads BP, y así irían recorriendo la cola
entre las dos. Pero de momento 1 VM, 1 core y 2 hilos a nivel de OS.»*

Queda como ficha `A1` en `FICHAS.md`: `vm` (sólo opcodes) + `io` (todo lo demás), tres colas y
nada más entre ellos, igual en todas las familias; el modelo GUI en `vm` y LVGL en `io`; en dos
núcleos cada hilo en el suyo sin más regla que «sólo cruzan las colas» (visibilidad por
primitivas del RTOS, flash que congela al otro núcleo, interrupciones donde las registra `io`,
bucle del intérprete en RAM). El camino SMP de `scheduler_smp.c` es el futuro de dos VM sobre
una cola: se aparca sin cerrarlo.


---

## EL MODELO FINAL EN LOS MICROS: 1 núcleo y 2 núcleos (Eduardo, 9-sep)

Dicho por él, antes de seguir tocando el SMP, y es el destino contra el que hay que medir
cualquier cambio:

> **1 núcleo** — dos hilos de SO: `IO` y `VM1`, turnándose. *«Pero hay que tener presente que en un
> futuro podría haber más hilos del SO, así que hay que trabajar con una cola de threads SO.»*
>
> **2 núcleos** — tres hilos de SO: `IO`, `VM1` y `VM2`. *«Yo dejaría las 2 VM a piñón, aunque si no
> se ejecuta ningún programa BP estarán ociosas.»*
>
> *«Suponiendo que tenemos una cola de Threads BP, a nivel de SO la IO, la VM1 y la VM2 se van
> turnando. Ahora cada VM simplemente tiene que consultar la cola de Threads BP y ejecutar el que le
> toque. **En principio no interactúan entre ellas**, los intercambios se hacen a través de la
> memoria compartida.»*

📌 **Vocabulario, que evita un malentendido que ya tuvimos**: lo que él llama **VM** es lo que el
código llama **worker**. Una VM = un hilo de SO que ejecuta bytecode BP; toma un thread BP, lo
ejecuta, lo deja y coge otro. Dos VM hacen lo mismo, cada una a su ritmo.

### Lo que ya está así, medido en el código

**La VM-C ya implementa este modelo casi entero** (`src/scheduler_smp.c:6-10`):

```
- bajo vm_lock: parquear si no hay tc RUNNABLE, despertarse con cond_signal
- SOLTAR vm_lock y correr el quantum          ← LOCK-FREE
- re-tomar vm_lock para gestionar la salida del quantum
```

O sea: las VM se tocan **sólo en la cola**, y mientras ejecutan van sin cerrojo. Y la cola tiene el
candado semántico que el modelo exige — `sched_owner`, el worker que tiene asignado ese tc: *«sólo
seleccionable si está RUNNABLE Y nadie lo tiene asignado»*. Eso es literalmente *«un thread en
ejecución no lo puede tomar otro núcleo»*.

Medido el 9-sep, con una carga sin mutex: **`--smp=2` escala ×1,90** (215 → 113 ms) y no falla.

### 🔑 El modelo PREDICE dónde está el bug que queda

*«En principio no interactúan entre ellas»* — y hoy hay **una** interacción que no es la cola: el
**GC stop-the-world**. Las dos VM tienen que pararse a la vez.

Y ahí es justo donde queda el cuelgue tras arreglar el mutex: el residuo de `--smp=2` (1 de 30) y
los fallos de `--smp=4` mueren con las dos últimas líneas **idénticas** —los dos workers anunciando
el mismo «GC por TABLA de handles»—. El modelo no lo adivinó por casualidad: **la única parte que se
sale de él es la única que sigue rota.**

### ⏭️ Dos diferencias con lo que hay, para decidir

1. **«A piñón» vs parquear.** Hoy una VM sin trabajo se **parquea** en un `cond_wait` en vez de girar
   en vacío. Es mejor para consumo y calor —importa en un micro alimentado por batería— y despertar
   cuesta poco porque el que encola hace `cond_broadcast`. Si de verdad se quiere «a piñón», hay que
   decir por qué: la latencia de despertar es lo único que se gana, y no está medida.
2. **La cola de hilos de SO.** Hoy los workers se crean fijos al arrancar (`bpvm_run_smp(vm, n)`).
   Que sean una *cola* —hilos que entran y salen— es del futuro que él anticipa; hoy no hay nada que
   lo impida, pero tampoco está.

⚠️ Y lo que **no** cambia con los núcleos: `A4` sigue mandando en el firmware — *«un solo núcleo
hasta nueva orden»*. Todo lo de arriba se mide hoy en el host.


## Las pruebas finales, con un agente conduciendo las placas (Eduardo, 5-sep)

**La pregunta**, al cerrar el día:

> *«Te he visto ejecutar programas y resetear en placas reales. Si construimos una lista de
> programas a testear, ¿se los podríamos pasar a un agente para que lo haga?»*

**Sí, y hoy ya ha pasado.** En una sesión se han grabado y medido las **cinco familias** sin que
nadie tocara una placa: `idf.py flash` en las cuatro ESP32, la copia del `.uf2` con `BOOTSEL`
pedido por el wire en la Pico, y `STM32_Programmer_CLI` por número de sonda en la Nucleo y la
Discovery. Y sobre eso: subir módulos, ejecutar, matar, leer el log persistente y cronometrar.

Lo que lo hace posible no es nada nuevo: **todo lo que hace falta ya es protocolo de máquina**.
El wire v1 tiene `PUT`/`RUN`/`KILL`/`LIST`/`LOG_DUMP`/`INFO`, `HELLO` declara las capacidades de
cada placa, y los programas se cronometran solos. No hay que inventar un sistema de pruebas: hay
que **decidir tres cosas** que hoy no están decididas.

### 1. El oráculo ya está elegido, y es el invariante del proyecto

La pregunta difícil de un banco de pruebas es «¿cómo sé que el resultado es correcto?». Aquí no
hay que responderla, porque ya está respondida: **el `stdout` de la placa debe ser byte-idéntico
al del host para el mismo `.mod`**. Es exactamente lo que `compat/compat.sh` hace hoy entre las
dos VMs, con un corpus de **38 programas**.

Eso evita el peor error de diseño posible: mantener ficheros de «salida esperada». Se pudren, y
cuando se pudren nadie sabe si falló el programa o el fichero. Con el host de oráculo, el
esperado **se calcula en cada pasada**.

📌 Y trae un beneficio que no es obvio: convierte el banco de placas en una extensión del arnés
de paridad, no en un sistema aparte. Un fallo dice *«la C6 se aparta del host en la línea 12»*,
que es un diagnóstico, no un aviso.

### 2. Lo que la lista tiene que decir de cada programa (y esto es el trabajo)

«Todos los programas en todas las placas» es justo lo que no escala — y es el problema que
`#444` planteó. La lista no es una lista de nombres: es una lista de **programas con sus
condiciones**:

| Campo | Para qué | De dónde sale hoy |
|---|---|---|
| dónde aplica | un sample de SD no se corre en una Pico (no tiene lector) | `HELLO` ya declara capacidades; falta usarlas |
| qué necesita en el FS | `Gui` y `Json` para los del GUI, un `.pack`, un fichero de datos | hoy se sube a mano |
| cuánto tarda como mucho | para no colgar la tanda | los tiempos ya medidos sirven de base |
| cómo se decide | byte-idéntico al host · o «no debe petar» · o **ojos de Eduardo** | hoy, en la cabeza |
| qué hace falta a mano | pulsar un botón, mirar la pantalla, enchufar algo | hoy, en la cabeza |

Las dos últimas filas son las importantes, y llevan a la tercera decisión.

### 3. Lo que un agente NO puede hacer — y que tiene que GRITAR, no saltarse

Hay tres clases de prueba que no se automatizan, y hoy se distinguen sólo porque alguien se
acuerda:

- **La pantalla.** Que el modelo diga `screen [240x240]` no dice que el panel pinte. Los tres
  «✅ visto» de esta semana —los tres botones del C6, las esquinas girando, la paleta de la
  Discovery— los dio Eduardo con los ojos, y no hay forma de que un agente los dé.
- **Lo físico**: un botón, un sensor, quitar y poner la SD.
- **Lo que exige una mano**: la Pico sin botón de reset, una placa que sólo se recupera
  desenchufando.

⚠️ **La regla de diseño que sale de hoy**, y sale escaldado: una prueba que se salta **en
silencio** es una prueba que no existe. Hoy me ha pasado dos veces en un rato — el caso del GUI
del arnés `io` se saltaba sin decir por qué (faltaba LVGL, y luego faltaban las dependencias en
el FS), y un `.elf` que no se reconstruyó dio un «Build Finished» tranquilizador. Así que el
informe tiene que **probar que corrió**: sello del firmware de cada placa, CRC de cada módulo
subido, y el número de programas **ejecutados**, no el de programas de la lista. Con una línea
por cada uno que se saltó y **por qué**.

### 4. Por qué esto sí responde a `#444`, y el reparto de tres pisos no

`#444` descartó el reparto obvio (host / simulador / placa) con un *«sigue sin ser un buen
sistema»*, y con razón: repartir no quita trabajo, sólo lo ordena. Lo que ha cambiado hoy no es
el reparto — es **quién paga la re-verificación**. El problema medido era *«U3 obligó a
re-verificar en placa ocho veces en un día»*. Ocho veces las hace un agente mientras se hace
otra cosa.

El simulador sigue valiendo, pero por otro motivo: es la única forma de probar lo que **no hay**
(una placa que no está conectada, una que aún no existe). No sustituye a la placa, la precede.

### ⏭️ Lo que haría falta, en orden

1. **El manifiesto**: un fichero con la lista y sus condiciones (los cinco campos de arriba).
   Empezar por los **38 del corpus de paridad**, que ya tienen oráculo, y crecer desde ahí hacia
   los 303 samples.
2. **El runner**: lee el manifiesto, pregunta `HELLO` a cada placa conectada, ejecuta lo que
   aplica y compara con el host. Lo que hoy son cuatro guiones sueltos en un scratchpad.
3. **El informe**: por placa y por programa, con los saltados y su motivo, y el sello que prueba
   qué firmware corrió.
4. **La lista de lo que necesita ojos**, aparte y corta, para que Eduardo la despache de una
   sentada en vez de descubrirla a mitad.

📌 Nada de esto es infraestructura nueva: es escribir el manifiesto y juntar los guiones que hoy
ya existen. Lo caro era conducir las placas, y eso ya está resuelto.


---

## La captura de pantalla EN EL MICRO — el testigo que le falta a las pruebas gráficas (Eduardo, 5-sep)

> *«Creo que una vez hablamos de hacer una captura de pantalla en el micro. Mi idea era
> precisamente para esto: para que cuando se probasen los programas gráficos, desde el PC se
> pudiese ver esa pantalla. Al final no lo concretamos pero la idea sigue ahí.»*

Encaja justo en el agujero que la propuesta de arriba deja al descubierto: **«la pantalla necesita
los ojos de Eduardo»**. Los tres «✅ visto» de esta semana —los botones del C6, las esquinas
girando, la paleta de la Discovery— los dio él mirando. Una captura no quita los ojos, pero quita
la **repetición**.

En el host ya existe desde hace tiempo (`BPVM_GUI_SHOT_MS` y el comando `shot` de
`src/gui_display_sdl.c`, con su escritor de PNG sin dependencias). Lo que sigue es qué hace falta
para tenerlo en la placa.

### 1. La pregunta de verdad: ¿de dónde salen los píxeles?

La respuesta intuitiva —«lee el framebuffer»— **no vale**, y eso se mide, no se supone:

| placa | cómo pinta | ¿hay imagen viva en memoria? |
|---|---|---|
| **Discovery U5G9J** (LTDC) | `s_framebuffer[800*480]` estático, el LTDC lo barre por DMA | ✅ **sí**, 768 KB |
| **ESP32-P4** (MIPI-DSI) | LVGL en `RENDER_MODE_PARTIAL`, buffer de `hres*120` en PSRAM | ❌ **no** — el `fb` de 1024×600 que hay en `gui_display_dsi.c:326` es el del **rojo de arranque** (G3), se queda vivo a propósito pero **no contiene la pantalla actual** |
| **ESP32-C6** (ST7789 SPI) | `DRAW_LINES 24`, por bandas | ❌ **no**, y **no cabe**: 240×240×2 = **115 200 B** contra ~**68 KB** libres (el mínimo histórico medido en `chip_cfg.h`) |

Y el camino del host tampoco se puede copiar tal cual: `lv_snapshot_take()` **reserva la pantalla
entera** (115 KB en el C6 — imposible; 768 KB en la Discovery; 1,2 MB en el P4).

📌 O sea: de las tres placas con pantalla, **sólo una** tiene píxeles que leer, y la más pequeña
no tiene sitio para fabricarlos.

### 2. La forma que sí sirve en las tres: engancharse al `flush`

Las tres renderizan **por bandas** — es su naturaleza, no una limitación. Así que:

> armar una bandera → invalidar la pantalla → `lv_refr_now()` → y **cada banda que LVGL manda al
> panel sale también por el cable**.

- **Cero memoria extra**: se reaprovecha el buffer de dibujo que ya existe.
- **Funciona con render parcial**, que es lo que hacen las tres.
- Es **código común** con ~2 líneas de cintura por familia (el `flush_cb` es de familia).
- Y **el host hace lo mismo**, así que el artefacto sale igual desde el PC y desde la placa.

### 3. El transporte ya existe — no hace falta protocolo nuevo, casi

El wire v1 ya lleva bytes crudos: `GET` responde con `"bulk":N` y a continuación N bytes
(`wire_v1_read_exact`). Así que **una banda = un mensaje con su propio `bulk`**, lo que además
evita tener que saber el total por adelantado y evita el buffer grande:

```
PC  →  SHOT {id}
        ← SHOT_BEGIN {w,h,fmt}
        ← SHOT_BAND  {x1,y1,x2,y2,bulk:N} + N bytes      (una por flush)
        ← SHOT_END   {bands,bytes}
```

El PC monta la imagen y escribe el PNG (Python trae `zlib`; y si algún día hiciera falta en el
micro, el escritor de PNG **sin zlib** ya está escrito en `gui_display_sdl.c`).

### 4. Los números del cable, que son los que mandan

| placa | pantalla | crudo RGB565 | cable | tiempo en crudo |
|---|---|---|---|---|
| **C6** | 240×240 | 115 200 B | USB-Serial-JTAG | **< 1 s** |
| **Discovery** | 800×480 | 768 000 B | UART 115 200 (11,5 KB/s) | **67 s** |
| **P4** | 1024×600 | 1 228 800 B | UART 115 200 | **107 s** |

⚠️ **En crudo, las dos pantallas grandes están fuera.** Y no es casualidad que sean justo las dos
que van por UART lenta. Lo natural es **RLE sobre RGB565** (~30 líneas): una pantalla de GUI es
casi toda tiradas planas del mismo color.

📌 Y aquí va la única cifra que **no tengo**, y de la que cuelga el diseño entero: **cuánto
comprime de verdad una pantalla real**. Se mide gratis en el host, antes de tocar la placa. Si no
llega a ~10×, el plan B es **submuestrear** (a 1/4 la pregunta *«¿se ve bien?»* se sigue
contestando, y cuesta cero).

### 5. Quién lo ejecuta — y `A1` ya dejó la costura hecha

LVGL no es reentrante y vive en el hilo `vm`; el hilo `io` **no puede tocarlo**. Así que la
petición entra por la **cola de control** y la sirve la tarea `vm`, en la misma costura donde `A1`
puso la guarda del KILL: `src/builtins.c:970` y `:1015`, dentro de `Gui.run()`. No hay maquinaria
que inventar — es exactamente para esto para lo que sirve la arquitectura de dos hilos.

Dos disparadores, como en el host:
1. **El verbo `SHOT`** desde el PC, para el banco de pruebas.
2. **`Gui.shot()` desde BasicPlus**, para que el programa se retrate **en el instante que importa**
   — que muchas veces es a mitad, no al final.

### 6. Qué resuelve y qué NO (importa más lo segundo)

**Resuelve**: un PNG por programa gráfico. El agente **adjunta prueba** en vez de decir «parece que
va»; Eduardo despacha veinte pantallas de una sentada en vez de presenciar veinte ejecuciones; y
aparece un **oráculo de regresión** que hoy no existe — la misma placa, el mismo programa, antes y
después.

**No resuelve**, y conviene no venderlo: no es el oráculo dual-VM. El host y la placa no comparten
resolución, así que comparar píxel a píxel host↔placa no va a ser. Placa↔placa en el tiempo, sí.

⚠️ **Y la trampa, que ya nos ha mordido tres veces**: la captura es lo que LVGL **dibujó**, no lo
que el panel **muestra**. Un `gap` mal puesto, una rotación al revés, el backlight apagado o una
línea de SPI floja dan un **PNG perfecto y una pantalla negra** — que es literalmente lo que pasó
en `P2`. La captura retira la repetición, no los ojos.

### ⏭️ Orden propuesto

1. **Medir la compresión** de pantallas reales en el host. Gratis, y decide el resto.
2. La captura común + el gancho del `flush` en **una** placa: el **C6** (cable rápido y pantalla
   pequeña — el ciclo más barato).
3. El lado del PC: montar las bandas y escribir el PNG.
4. Las otras dos. Si `A2` dice la verdad, deberían ser dos líneas cada una.
