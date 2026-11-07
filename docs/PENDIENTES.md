# BasicPlus — Limitaciones y pulido conocidos

> **Qué es esto, y qué NO.** Aquí van las **limitaciones y decisiones del lenguaje**
> que alguien que USA BasicPlus debe conocer: cosas que son así y de momento se
> quedan así. Es documentación, y por eso la citan `QUICKSTART`, `PHILOSOPHY` y los
> README.
>
> **Lo que ya NO va aquí: el estado del trabajo.** Qué ficha está abierta, cerrada
> o en curso vive **sólo** en `docs/FICHAS.md`. Decisión de Eduardo (17-ago): *«Estado
> y pendientes son ficheros de trabajo tuyos. Pero el que dice realmente cuál es la
> situación es Fichas.»* **Si este fichero contradice a `FICHAS.md`, manda `FICHAS.md`**
> y esto se corrige.
>
> El motivo es concreto: el 17-ago este fichero daba por abiertos dos bugs cerrados
> ese mismo día, y de las 51 fichas que citaba `ESTADO`, 49 eran una segunda copia de
> las de `FICHAS`. Una lista de bugs más larga que la real es justo lo que este
> documento venía a evitar.
>
> **Mapa de docs:**
> - **`docs/FICHAS.md`** — el registro de fichas. **La fuente de verdad.** Lo que viene
>   después de V7 —la WiFi, el Bluetooth y el bajo consumo, y lo que V7 dejó para
>   entonces— está en su bloque de V8.
> - **`docs/ESTADO.md`** — el traspaso entre sesiones (diario, por fechas).
> - **`V7_BACKLOG.md`** / **`V7_IDEAS.md`** — el plan de V7 y su diseño (el porqué de
>   cada decisión).
> - **`V6_BACKLOG.md`** / **`V5_BACKLOG.md`** / **`V4_BACKLOG.md`** / **`HECHO_V*.md`** —
>   snapshots inmutables de versiones cerradas.
>
> Convención: L=limitación · N=hallazgo · M=mejora. (Los bugs, **B**, se fueron a
> `FICHAS.md`: tienen estado.)

---

## 🟡 Limitaciones / decisiones documentadas del lenguaje

- **L14 — en el RP2350 (Pico 2 / Metro), los `double` SUBNORMALES se aplastan a
  cero.** Es decir: por debajo de `2.2250738585072014e-308`, la Metro da `0`
  donde el PC, el ESP32-P4 y el STM32 dan el valor. Todo lo que esté por encima
  de esa frontera es exacto y byte-idéntico en las cuatro plataformas.

  **De dónde sale.** El SDK de la Pico sustituye las rutinas de `double` de
  libgcc por las suyas optimizadas, y ésas descartan los subnormales *a
  propósito* — está escrito en su ensamblador: `double_sci_m33.S:121`,
  `movs r0,#0  @ flush denormal`. No es un fallo nuestro ni del SDK: es su
  compromiso de velocidad, que heredamos al enlazar.

  **Medido, no supuesto** (17-ago, `samples/SubNorm.bp` y `samples/DblBench.bp`,
  los dos en el repo):
  - la frontera es EXACTAMENTE la del formato: `n1..n4` (normales, incluido el
    menor normal) salen bien; `s1..s5` (subnormales) y las operaciones que caen
    ahí, todos a `0`; el control en magnitudes normales, exacto;
  - el P4 y el host dan las 16 líneas correctas — **la Metro es la excepción, no
    la regla**;
  - cambiarlo se puede (`pico_set_double_implementation(bpvm_pico compiler)`) y
    cuesta **+23 KB de flash y un 24 % de tiempo** en el banco, que es **1,8×**
    en la aritmética de coma flotante una vez descontada la sobrecarga del
    intérprete (control entero idéntico al milisegundo en las dos corridas).

  **Decisión de Eduardo (17-ago): NO se cambia.** *«Prefiero un 25 % más de
  velocidad y perder un poco de compatibilidad que afecta al 0,01 % de los casos,
  en los extremos, no con valores normales. `double` se va a utilizar en la toma
  de medidas que requieran precisión, pero estamos hablando de instrumentación
  donde tenemos 6 u 8 dígitos significativos como mucho.»* Aplica su propio
  criterio: esto lo pagaría TODO programa que use `double` en la Metro, siempre,
  para proteger un rango que no usa nadie.

  ⚠️ **El caso a vigilar no es escribir `5e-324` a mano** —eso no pasa— sino que
  un cálculo DESBORDE POR ABAJO: en la Metro daría `0` y en el P4 un número
  diminuto, en silencio. Si algún día alguien tropieza con eso, la palanca está
  identificada y medida aquí mismo.

- **L15 — el log post-mortem sobrevive a unos resets y a otros no, y depende de la
  familia.** Desde el 18-ago el log vive en una región de RAM que el arranque no
  borra, así que una autopsia enseña las líneas de ANTES del reinicio — incluidas
  las de un cuelgue, que es para lo que se hizo. Pero esa RAM sólo sobrevive si el
  reset **no corta la alimentación del núcleo**, y ahí las placas no se comportan
  igual:

  | | conserva el log | lo pierde |
  |---|---|---|
  | **ESP32-S3 / P4** | reset por software (el `reset` del IDE), panic/excepción, watchdog | **el botón RST de la placa** y desenchufar |
  | **RP2350 (Pico 2 / Metro)** | reset por software (el `reset` del IDE) y, con la imagen de dos núcleos (la de V7), un fallo fatal: la reinicia el perro guardián (ver «Para V8») | **el botón de reset de la Metro** (el pin de RUN: medido el 3-oct, arranque en frío) y desenchufar |
  | **STM32** | el botón NRST (medido en la Nucleo el 24-sep: «pin (NRST)» y las líneas de antes) | desenchufar |

  La C3, la C5 y la C6 no están en la tabla porque no se ha medido: llevan el mismo log que la
  S3 y la P4 —el mismo código, la misma región que el arranque no borra—, y lo esperable es
  que se porten igual, pero nadie lo ha visto.

  Lo que sorprende es el ESP32 —su botón RST tira del pin EN y corta el dominio
  digital entero, así que cuenta como arranque en frío— y la Metro: aquí ponía que su
  botón (el pin de RUN) conservaba el log, sin haberlo medido, y no lo conserva. Medido
  el 3-oct: tras el botón, el arranque dice `arranque en frío (cargado de flash)` y la
  causa `power-on/run`; de antes sólo queda lo que ya estaba en la flash, hasta el
  arranque del último programa, y las líneas de ese programa se pierden. **Si persigues
  un cuelgue en un P4, un S3 o una Metro, reinicia desde el IDE, no con el botón.** El arranque dice siempre
  de cuándo es lo que ha cargado (`RAM SUPERVIVIENTE` frente a `arranque en frío`),
  así que no hay que adivinarlo; y el `Reset` del diálogo de Info dice qué tipo de
  reinicio hubo. El volcado a flash sigue existiendo como red para los cortes de
  corriente, pero sólo llega hasta el último punto de guardado.

  Con un programa en ejecución la placa atiende `HELLO`, `KILL` y —desde V6
  (`#452`)— también `RESET`: el `reset` del IDE llega con un RUN vivo en las cinco
  familias, mata el programa y reinicia después. `kill` + `reset` sigue valiendo. El
  `kill` llega aunque el programa esté colgado —está probado sobre un bucle cerrado
  que no cede el turno— y no borra el log, así que la autopsia sale entera igual.
  Desde V7 (`#495`) atiende además `STAT` y el `GET` y el `LIST` **troceados**, que son
  los que pide el IDE: el explorador lista y baja ficheros con el programa en marcha
  (un log, un `.shot`), y sus trozos van entre la salida del programa. Lo demás —subir,
  borrar, crear una carpeta, un `GET` entero como el de `wire_serie.py`— contesta
  `BUSY` hasta que el programa acaba.

- **L17 — un `catch` sin tipo ES `catch e: Exception`** (V6, 30-ago). No es un caso
  aparte: el compilador le pone ese tipo al AST, así que `catch e` y
  `catch e: Core.Exception` compilan a lo mismo. Consecuencias prácticas:
  - `e.msg` funciona sin escribir el tipo;
  - **imprimir la excepción muestra su mensaje** (`Exception.toString()` devuelve `msg`),
    no `object@1234`;
  - `Exception` es un tipo de `Core`, que todo módulo importa sin escribirlo (**L16**).

  No se pierde ninguna captura: `throw` sólo acepta instancias de `Exception`, y el
  compilador lo rechaza si no lo es.

- **L16 — `import Core` es implícito** (V7/H1, 20-sep; en V6 había que escribirlo). El
  compilador lo añade a todo módulo que no lo escriba —salvo al propio `Core`—: se puede
  poner o no, y da lo mismo. `List`, los envoltorios de los primitivos (`Integer`, `Long`,
  `Float`, `Double`, `Boolean`), `Comparable`, `Exception` y `RuntimeError` se escriben sin
  más, igual que las cuatro clases del lenguaje —`Object`, `Thread`, `Mutex` y
  `StringBuilder`—, que desde V7 son clases BasicPlus de `Core.bp` (manual §10.0).

  En V6 la norma era la contraria (`#458`, 30-ago: *«si se utiliza un tipo de Core, se ha
  de importar Core»*). Hasta entonces el compilador ya lo inyectaba, pero en una sola de
  sus dos pasadas, y un módulo que exponía un tipo de `Core` en una firma pública
  **perdía ese miembro** al importarlo otro. Ahora lo pone el parser, y las dos pasadas
  ven el mismo árbol. Los demás imports siguen sin ser transitivos (**L18**). Este
  programa compila y ejecuta tal cual —imprime `2` en las dos VMs— sin importar nada:

```basicplus
module L16
  // sin `import Core`: lo pone el compilador

  function leer(): List
    var l: List := List()
    try
      l.add("uno")
      throw RuntimeError("fallo de prueba")
    catch e: RuntimeError
      l.add(e.msg)
    endtry
    return l
  end leer

  public function Main()
    var l: List := leer()
    print l.length()
  end Main
end L16
```

- **L18 — lo que cruza la frontera de un módulo, y lo que no** (V6, `#492` → ✅ V7,
  14-sep-2026). Desde V7 la interfaz de un módulo exporta **todo lo que un descendiente
  puede tocar**: métodos, propiedades, **los campos protegidos**, **todas las
  sobrecargas del constructor**, **los métodos y properties estáticos** y las
  constantes — con `final` y `private` respetados. Los dos primeros límites de V6 ya
  no existen (`class Gato extends Base.Animal` en otro módulo ve `var nombre`;
  `Base.Animal()` y `Base.Animal("Rex")` valen las dos, y `super(...)` elige por
  firma). Lo que sigue dentro:
  - **una `native` no lee campos ni llama a estáticos** (ni propios ni heredados): el
    compilador lo rechaza con un error claro, en una native de módulo y en un método
    `native` de clase (`#519`, V7). Rodeo: un getter.
  - **una `var` estática no cruza** (ni de módulo ni de clase): vive en el segmento
    de datos del módulo, que sólo vale para su propio código. El estado sale del
    módulo por getters y setters: declárala `property` (protegida) y cruza.
  - **`Clase(o)` con `o: Object` decide cast-o-construcción mirando sólo la primera
    sobrecarga del constructor** (`#389`): si hay un `Animal(o: Object)` que no es la
    cabeza, `Animal(x)` se toma como cast.
  - **los imports no son transitivos.** Si `Mid` importa `Base` y te devuelve un
    `Base.Animal`, para nombrar ese tipo —o llamar a sus métodos— tu módulo tiene que
    escribir `import Base` él mismo; si no, `tipo cualificado 'Base.Animal' no
    encontrado` (o `el tipo 'Base.Animal' no tiene miembros`). Lo mismo si usas una
    clase de `Mid` que extiende `Base.Animal`: sin `import Base`, el compilador avisa
    (`base cross-module 'Base.Animal' de la clase importada 'Mid.Gato' no resuelta
    (¿falta el import de su módulo?)`) y no ve lo que hereda. La excepción es `Core`,
    que se importa siempre (**L16**).

- **L19 — las interfaces, como las de Java antes de Java 8** (V8/H7.3, `#756`; manual §9.8).
  Una interfaz lleva métodos (sólo su firma) y properties; no lleva:
  - **métodos con cuerpo** (los `default` de Java, o los de C# 8): todo lo pone la clase,
    propio o heredado de su padre;
  - **implementación explícita** (`void I.M()` de C#): una clase tiene un solo método con
    cada firma, y si dos interfaces lo piden, ese método vale para las dos;
  - **parámetros de tipo** (`Comparable<T>`), ni clases abstractas.
  Y además: un `Thread` y sus hijas no implementan interfaces; las interfaces son del
  formato 9 (el de por defecto); y una `native` no llama por una interfaz (rodeo: llamar
  desde BP y pasarle a la native lo que haga falta).

- **L7 — `owner`/`final` no aplican a property de módulo.** Por diseño: `owner`
  pide FREE_REF en cascada (solo campos de instancia); `final` aplica a herencia
  (los módulos no la tienen). Reabrible si surge caso de uso.
- **L9 — `Mutex` no reentrante.** Por diseño (documentado en el manual). Para
  re-entrada, usar otro patrón (flag + condvar).
- **Con varios hilos, un `print` sale entero; entre dos `print`, no** (decidido el 28-sep,
  `#600`). `print "Resultado=", r` nunca se mezcla con la línea de otro hilo; entre `print a`
  y `print b` sí puede caer. Para que varias líneas salgan juntas: un `StringBuilder` y un
  solo `print` (manual §14.5). El límite: una línea de más de 1 KB —lo que cabe de una vez
  en la cola de salida de la Pico— puede salir en trozos.
- **Con dos runs (`vm.runs=2`, lo de por defecto en las placas de dos núcleos —la Pico 2, la
  Metro, la S3 y la P4— desde el 6-oct), un programa que se DEPURA va con uno: el primero** (decidido por
  Eduardo el 5-oct, V7/H12). El depurador para el programa entero y sigue a un hilo; con dos
  runs ejecutando a la vez sería muy difícil de seguir (*«Alguna vez he tenido que depurar en Java
  con varios hilos y es muy difícil de seguir»*). Los hilos BP siguen siendo varios —se turnan en
  ese run, como con `vm.runs=1`—; lo que no hay mientras se depura es el segundo run. Un programa
  que usa SQLite también va con uno (el motor está compilado sin hilos).
- **Los hilos VIVOS a la vez, los que quepan en la memoria, cada uno con la pila que pide**
  (V7/H12, etapa 2: la cola de hilos, sin tope; hasta entonces, 32 a la vez —`#604`/`#605`—).
  Un hilo terminado deja su sitio; pedir una pila que no cabe es un `RuntimeError` en `start()`.
  Una función que no cabe en la pila de su hilo lanza «desbordamiento de pila» (manual §14.1).
  El límite: una recursión DENTRO de una `native` corre, en la VM-C, en la pila de C de su
  ejecutor, y allí sólo la vigila miVM (que ejecuta la native como bytecode).
- **Dentro de una `native`, casi todo; no todo** (V7/H5). Desde V7 caben `try`/`catch`/`finally`
  (`#578`) y las tuplas (`#577`). Siguen fuera, cada uno con un error de compilación que dice
  el remedio: un `try var` (declara el recurso antes con `var` y suéltalo en un `finally`) y
  una tupla con un campo `float`, `long` o `double` (desestructúrala en BP y pásale los
  campos). Lo demás que el AOT no traduce —leer campos o llamar a estáticos (L18), un
  `for … in` sobre una colección, `instanceof`, los eventos…— también es un error de
  compilación con su línea (`#519`): `native` es una promesa. La lista, en `AOT_LIMITES.md`.
  - **En la placa, el depurador no para dentro de una `native`** (decidido el 9-oct, `#583`):
    allí corre como código máquina; en el PC sí para, porque la VM Java la interpreta. El IDE
    lo avisa al poner el breakpoint; para seguirla paso a paso, quítale `native` mientras la
    depuras.
  - **En el ESP32-S3 todo se ejecuta interpretado** (`#488`, fuera del plan). El AOT
    genera ARM Thumb-2 y RISC-V; **no genera Xtensa**, y de momento no se va a hacer. Las
    `native function` compilan y corren, pero a la velocidad del intérprete. Aceleran:
    RP2350, STM32, ESP32-P4 y, desde V7, **la C5, la C3 y la C6** (RISC-V sin FPU, destino
    `ilp32`; probado en placa en la C5, la C3 y la C6 el 25-sep). En ellas los `float` van
    por las rutinas del firmware (`#559`): dan lo mismo que interpretados, pero aceleran
    menos que el código entero. En el PC —VM Java y micro simulado— todo es interpretado,
    y así se comparan los tiempos.
- **La GUI, sólo en las placas con pantalla, y por packs** (V7/H6). La llevan las imágenes de
  la C5, la C6, las P4 y la DK2; las de la Pico 2 y la Metro, la S3, la C3 y la Nucleo, no:
  allí `Gui.Screen()` lanza `builtin 135 no soportado en esta VM (subconjunto C)`. Y donde
  la hay, LVGL ya no va dentro de la imagen: llega en el pack de la GUI (`Gui.pack`, o
  `GuiMini.pack` en la C6) y, en la C5 y la C6, con el del driver de pantalla
  (`DriverSpi.pack`, que lee la placa de su ENV). Se graban una vez desde el IDE, con un
  bloque de RAM para los motores en el ENV (`packRam=<KB>`). Si falta algo, el programa lo
  sabe al crear su pantalla: «GUI: esta placa no tiene el pack 'LVGL' grabado (la GUI llega
  por el pack)», «GUI: el pack de la GUI esta, pero su motor no cargo: falta packRam= en el
  ENV» o «GUI: no hay driver de pantalla ('PANT'): graba el pack del driver de esta placa».
  Qué pide cada placa, en `gui.html` §23.4. En el PC no hace falta nada.


### `input()` sólo funciona en el PC, no en el micro (decidido el 26-ago; `listDir` YA está en las dos desde V7/H3)

`input()` / `IO.prompt()` abre su ventana en el IDE cuando el programa corre en la **VM
Java**. Es una primitiva de la raíz (`Core`: se llama sin `import` ni cualificar) y **compila
igual para todo el mundo**, pero en la **VM-C —o sea en cualquier placa y en el micro simulado— lanza un
`RuntimeError` atrapable**:

```
builtin 37 no soportado en esta VM (subconjunto C)      // input()
```

Si no lo atrapas, el programa **termina con código de salida 1** (`RuntimeError BP no
atrapado`), igual que cualquier otra excepción sin `catch`. `IO.prompt()` en la VM-C lanza
`prompt: no hay IDE conectado`. Es el único builtin del lenguaje que no está en la VM-C
(además de `heapFrag`/`heapMap`, que son diagnóstico de la VM Java): el resto — ficheros,
rutas, matemáticas, cadenas — corre igual en las dos. `listDir(path)` **también** desde
V7/H3 (21-sep): en las dos VMs devuelve los nombres **ordenados por bytes** (el orden de un
listado es del FS, y el mismo programa tiene que imprimir lo mismo en el PC y en la placa)
y lanza `RuntimeError` si el directorio no se puede abrir (antes miVM devolvía una lista
vacía en silencio).

**Se queda así a propósito.** El razonamiento de Eduardo: `print` e `input` no son entrada
y salida de verdad en un micro —son **herramientas de depuración**, anteriores al
debugger—, y un `input()` era el «breakpoint del pobre» para parar un programa. Con el
debugger de verdad funcionando, eso ya está mejor resuelto.

⚠️ **Lo que hay que saber al escribir un programa**: si va a correr en placa, no uses
`input()` para pedir datos. Para interactuar con el usuario en un micro está la GUI
(formularios), y para pausar y mirar, el debugger. Un programa que quiera correr en los dos
sitios puede envolver la llamada en `try … catch e: RuntimeError`.

📌 El coste de igualarlo no era pequeño: `input()` bloquea el hilo BP (estado
`BLOCKED_PROMPT` y salida de la cola de ejecución hasta que llega la respuesta), así que en
la VM-C tocaría el planificador, no la tabla de builtins.

### Una tupla no entra en una colección (decidido el 21-ago)

`lst.add(pair())` **no compila**. Hasta V4 colaba porque `any` era asignable en los
dos sentidos; desde `#389` la raíz del modelo de objetos es `Object`, y una tupla no
es un `Object`.

**Es una decisión, no un olvido.** La tupla está para **devolver varios valores de
una función**, que es donde gana:

```basic
{ n, s } := pair()          // esto sigue igual de bien
```

Para **guardar** pares hay dos formas mejores, y las dos le ponen nombre a los campos:

```basic
class Par                    // 1) una clase con sus dos propiedades
  public property numero: integer
  public property texto: string
  public function Par(n: integer, t: string)
    this.numero := n
    this.texto  := t
  end Par
end Par

var lst: List := List()
lst.add(Par(42, "hi"))
var p: Par := Par(lst.get(0))
```

```basic
var m: Collections.Map := Collections.Map()   // 2) un Map, si lo que quieres es buscar por clave
m.put("hi", Integer(42))                      //    (import Collections; Core va siempre, L16)
```

El ejemplo `samples/TupleFirstClass.bp` lo enseña de las dos maneras.

📌 **Lo mismo vale para los arrays**: un `byte[]` tampoco es un `Object`, así que
`lst.add(newByteArray(n))` no compila. Se envuelve igual, en una clase de una sola
propiedad — es lo que hace `samples/MemInfo.bp`, donde la lista sólo sirve de ancla
contra el recolector.

### Los parámetros `var` sólo reciben la property de un objeto (V8/H7.4, decidido el 7-nov)

`function suma(var n: integer, m: integer)` y `suma(c::valor, 5)` (manual §8.1b). De momento no se pasan: las
properties de un módulo y las estáticas (no tienen objeto: llamarlas por su dirección pediría un opcode nuevo), las de
un objeto declarado como interfaz (en la llamada no se sabe su ranura), las `owner` y nada en MOD8. Tampoco en un
constructor, un evento ni una `native`. Y el depurador no enseña el valor de un parámetro `var`: son dos valores
ocultos, y verlo pediría llamar al getter.

### Los overlays: lo que no hacen todavía (V8/H9, cerrado el 7-nov)

`Overlay` carga un programa con sus módulos, lo ejecuta y lo descarga (manual §10.7, «Overlays: un programa que carga a otro»). Para
qué: un programa deja en memoria sólo lo que usa siempre, y lo que se usa poco —la configuración, exportar o importar
datos— va aparte y se carga cuando hace falta, como los overlays de MS-DOS con la barrera de los 640 K (Eduardo). Y otros
usos (Eduardo): una línea de comandos que ejecuta otros programas, y un programa extensible, con una interfaz para
intercambiar datos. Lo que no hace todavía, con su error o en el manual, y se hará cuando el uso lo pida (*«H9 necesita rodaje, que se utilice
y que vayan apareciendo los problemas»*):
- un error en un hilo del programa cargado no llega a `tryExe`: el programa entero acaba con código 1 (y los hilos que
  deje vivos son un error de `tryExe`: tiene que cerrarlos antes de salir);
- si usó la GUI, sus widgets se quedan, y sus manejadores impiden descargarlo; los eventos en cola para sus objetos,
  también;
- el depurador no pone puntos de ruptura en sus módulos (las trazas sí los nombran), y sus `native` corren
  interpretadas;
- `load` sólo desde `Main` (no desde un manejador de la GUI), y uno a la vez: dentro del programa cargado no valen otro
  `load` ni `Heap.compact`;
- una global de un módulo que se queda con un literal del que se va no se detecta; y el tope de 16 módulos cuenta los
  del lanzador y los del programa;
- intercambiar datos con el programa cargado, con una interfaz, para un programa extensible: hoy, de ida, un `string`
  (`tryExe(arg)`), y de vuelta, si acabó bien o su error (`getError()`, `getErrorClass()`, `getTrace()`).

### Los errores de la VM con su tipo: lo que no lo lleva (V8/H7.5, 7-nov)

`ArithmeticError`, `NullReferenceError`, `IndexError`, `CastError` e `IOError` (con su hija `EncodingError`), todas
hijas de `RuntimeError` (manual §11). Siguen siendo un `RuntimeError` a secas, a propósito: los pines y los buses, la
red, el heap lleno y la pila desbordada (en Java, `Error`, que no conviene atrapar), `parseInt` de algo que no es un
número, un array de tamaño negativo, y un `path demasiado largo` de `pathAbsolute` (no toca ficheros). Con un `Core`
de antes (una placa sin la imagen de V8/H7.5), todo sale como `RuntimeError`, con el mismo mensaje. Y `throw` de una
excepción `null` no se atrapa, ni con un `catch` sin tipo (`#806`, por decidir).

## ⏭️ Para V8 — lo que conviene saber al programar hoy

Lo que V7 deja para V8: el tema de V8 —la WiFi, el Bluetooth y el bajo consumo— y lo que
se aplazó al cerrar V7. No son bugs: son piezas que no están, o que están a medias. El
estado de cada una vive en su ficha (el bloque de V8 de `docs/FICHAS.md`); aquí, lo que
cambia al escribir un programa.

- **No hay bajo consumo** (`#490`). El micro nunca duerme: sin nada que hacer sigue
  girando (y en el STM32 la tarea ociosa ni siquiera llega a correr). No existe un
  `sleep` de placa ni fuentes de despertar en BP: un programa a batería no puede
  ahorrar hoy.
- **No hay red en la placa.** `Net` (el cliente TCP) sólo tiene el backend del PC; en una
  placa lanza `RuntimeError` («Net: sin red en esta plataforma»). La WiFi y el Bluetooth
  son el tema de V8.
- **En la RP2350, la imagen de dos núcleos —la de V7— hace más lento un programa de un
  solo hilo** (`#665`). En la Metro, `fib(26)` tarda 3 908 ms frente a 2 892 con la imagen
  de un núcleo (+35 %), y con `vm.cores=1` lo mismo (3 951): es la imagen, no el otro
  núcleo, y pega cuando los datos de la VM están en la PSRAM (que sea por la PSRAM es la
  sospecha, sin medir: falta probar la Metro con `psram=0`). En la Pico 2 el cálculo
  apenas lo nota (+1,8 % con un run, +8,4 % con dos), pero lo que imprime mucho sí:
  PrintBench, 1 270 ms frente a 817. Y dos hilos que trabajan con cadenas no ganan con el
  segundo núcleo (×0,83 en la Pico 2, ×0,75 en la Metro). `vm.runs=1` quita lo que cuesta el
  segundo run; lo de la imagen sólo se recupera con la de un núcleo, que se compila aparte
  (`cmake .. -DBPVM_PICO_NUM_CORES=1`). Se investiga en V8; lo que se gana con dos hilos, en
  el manual §14.6.
- **Un fallo fatal tumba la placa entera** (que el fallo de un run no la tumbe es de V8). Un
  error del programa es una excepción y, como mucho, acaba su hilo; un fallo *fatal* —un
  HardFault, la memoria del sistema agotada, una comprobación interna que falla— es del
  sistema. En la Pico 2 y la Metro, con la imagen de dos núcleos, la placa deja el informe en
  el log —en una RAM que sobrevive al reinicio—, el perro guardián la reinicia a los 250 ms,
  lo dice al arrancar («fallo fatal» en `Machine.resetCause()` y en el INFO del IDE) y **no
  lanza el autorun**: un programa que fallara nada más arrancar sería un bucle de reinicios;
  desde el IDE se arranca a mano (manual §14.6). **En las ESP32, lo mismo**: un fallo así es
  un *panic* del IDF —o su perro de interrupciones—, la placa se reinicia y **no lanza el
  autorun**; lo dicen el log («autorun: … NO arranca: el arranque anterior acabo en un fallo
  fatal (panic/exception; ver el log)») y la consola, y la causa del reinicio es la del chip
  («panic/exception» o «int-watchdog»). El perro de tareas del módulo `Wdt` no es un fallo
  fatal: tras él, el autorun sí se lanza (y si el programa vuelve a caer, la salida es la
  ventana del autorun: con el IDE conectado, al arrancar da unos segundos para pararlo con
  *Stop*). **En las STM32** (la Nucleo y la DK2), un HardFault deja su informe en el log y la
  placa **parada**: no se reinicia sola. Para V8: terminar sólo el run que falla y que `io`
  siga, el MPU protegiendo la memoria del sistema y un contador de reinicios contra el bucle
  del autorun.
- **En la placa, un error no dice en qué línea pasó** (`#582`). Al acabar, la placa da el
  mensaje —`excepcion no atrapada: Core.RuntimeError: División por cero`— y nada más, en
  código interpretado y en una `native`; la traza con la línea sólo la da la VM Java del
  PC. La placa no tiene el `.dbg` (da direcciones, y el PC pone los nombres): en V8, la
  placa dirá dónde y el IDE lo traducirá a su línea.
- **Un `.mod` lleva el código nativo de UNA familia** (`#584`). El IDE compila en cada Run
  para la placa que tiene delante y funde en el `.mod` el bloque nativo de su familia;
  llevado a mano a una placa de otra familia, el programa corre igual, pero sus `native` van
  interpretadas (la placa tolera el bloque ajeno). El `.mod` multifamilia —el mismo fichero
  para cualquier placa— es de V8, con la WiFi.
- **El depurador no despliega los objetos** (`M5`). Un local que es un objeto sale como
  `@<valor>`, sin sus campos ni sus properties (heredadas o no). Pide el layout de cada clase
  en el `.dbg` y una vista en árbol en el IDE: V8.
- **Con un run, un `read` de `Uart` que espera para a los demás hilos.**
  `Uart.Port.read(datos, n, timeoutMs)` espera a nivel del sistema —el run duerme de
  milisegundo en milisegundo hasta que llegan los bytes o vence el plazo—: con UN run (las
  placas de un núcleo, o `vm.runs=1`), mientras un hilo espera ahí, los demás hilos BP no
  avanzan. `sleep` no lo hace (duerme sólo su hilo), y con dos runs la espera retiene sólo
  el suyo. Lo que funciona con un run: mirar `available()` y, si no hay nada, `sleep`. En
  V8, esas esperas cederán el hilo, como `sleep`.
- **La C3 va justa de memoria.** Su VM se queda en 112 KB —48 de heap de BP, medido el
  6-oct— y lo que no
  quepa ahí no corre en ella (`docs/MEMORIA.md`). Optimizarla —coser sus dos regiones de
  memoria y devolverle los 128 KB de VM— es de V8.
- **Lo que el silicio tiene y la HAL aún no da**: la PIO del RP2350, programable desde BP, y
  más UART en las ESP32 (hoy un programa tiene, como mucho, la 1 y la 2: las de más de la
  P4 y la de bajo consumo de la C5, la C6 y la P4, todavía no). Las dos, para V8.

## 🔬 Prueba de resistencia larga (idea de Eduardo, 3-ago; para V8)

Dejar una placa corriendo **días** y ver hasta dónde aguanta. No entró en V4 (en plena
campaña de publicación no era razonable, y una prueba así no se improvisa); decidido el
2-oct, se hace en V8, con calma.

Lo que la haría útil, y no sólo larga:

- **Que la muerte deje rastro.** Si se cuelga a las 30 horas y no queda registro, se
  han perdido 30 horas. El log post-mortem en flash ya es un anillo que sobrevive al
  reset — hay que apoyarse en él y grabar una marca periódica (vuelta, heap, RTOS
  libre, marca de agua de pila), no sólo los errores.
- **Carga VARIADA, no un bucle.** Repetir una sola operación ejercita un único patrón
  de asignación. Lo que caza fugas y fragmentación es alternar: cadenas, objetos,
  ficheros, hilos, eventos. La batería de H13 ya es ese repertorio.
- **RUN largo Y muchos RUN.** Son dos fallos distintos: lo que se acumula dentro de
  una ejecución y lo que no se devuelve entre ejecuciones. El guardián de fin de RUN
  (#339) sólo ve el segundo.
- **Antes, la versión barata en el PC.** El mismo repertorio en host corre órdenes de
  magnitud más rápido y encuentra gratis lo que sea de memoria pura. No sustituye a
  la placa —no hay flash real, ni dos núcleos de verdad, ni IRQs— pero se paga solo.

Precedente de que el método funciona: #357 se cerró con 10.000 vueltas en la Pico, y
el bug que quedaba sólo se manifestaba al estrechar el heap.
