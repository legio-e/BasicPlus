# Límites de `native` (AOT v1)

> Referencia viva. Lo que el emisor AOT **acepta y rechaza** hoy, y qué hacer
> en cada caso. Se aplica a las funciones marcadas `native function ...`; el
> resto del módulo no se ve afectado.

## La regla que hace todo esto seguro

**Todo recorte es un error de COMPILACIÓN, nunca un fallo en ejecución.** Si una
función `native` usa algo que el emisor no sabe traducir:

- la compilación **aborta** (código de salida `2`),
- **no se genera el `.mod`** — no queda un artefacto a medias,
- el mensaje **nombra la función y la línea**, y dice qué alternativas hay.

Ejemplo real:

```
error AOT en función native:
  en la firma de la funcion native 'media' (linea 2): el tipo 'double' ocupa
  8 bytes y el AOT v1 sólo maneja valores de 4 (parámetros, retorno y variables
  locales). Opciones: usar 'float' si la precisión de 32 bits basta, o quitar
  'native' de esta función para que corra interpretada (el resto del módulo
  sigue yendo a nativo).
```

Esto importa más de lo que parece: un recorte **mudo** —que compilara y luego
fallara raro en la placa— sería mucho peor que uno anunciado. La política es
que el compilador se plante.

## Lo que NO soporta el AOT v1

> ⚠️ **Ojo: `native` no es un solo camino, y los límites NO son los mismos.**
> Lo de aquí abajo es el **AOT normal** (una función `native` de tu módulo).
> El **puente a un PACK** es más estrecho y `long` todavía NO ha llegado ahí:
> cruzan `integer`, `boolean`, `float`, `string`, los objetos como *handle*, y
> `long[]`/`double[]` **sólo como caja de salida**. Y las **llamadas de vuelta
> de `native` a BP** son otra cosa distinta — ver §3.

### 1. ~~`double`~~ — entró en V6/N1.2 (queda un resto)

⚠️ **Este apartado ha caducado DOS veces**, y las dos por lo mismo: describía un
límite del emisor como si fuera del hardware.

- El 16-ago-2026 dejó de ser cierto para **`long`** (#381, verificado en la Metro).
- El 23-ago-2026 dejó de serlo para **`double`** (#426): parámetros, retorno,
  variables locales, aritmética y comparaciones. La FPU sigue siendo de precisión
  simple en las tres familias, así que la emulación la hace el runtime — pero por
  la tabla de helpers, no por libgcc, que en un `.mdn` no existe.

~~**Lo que queda fuera es sólo la conversión `double(x)` escrita dentro de una
native.**~~ **Tampoco eso desde el 25-sep (`#559`)**: la conversión va por su helper,
como las demás. Aquel día se vio además que mezclar `double` con otro tipo
(`x: double := i`, `d * f`) y pasar entre `long` y `float` dejaban una llamada a
libgcc sin resolver en TODAS las placas —el enlace la ponía en la dirección 0—: el
emisor ya calca las conversiones del intérprete, y la guarda de `AotBuild` rechaza,
con el nombre de la native, cualquier símbolo que el `.o` no defina.

📌 **`float` sin FPU (la C5, la C3, la C6, desde el 25-sep, `#559`)**: cada operación y
cada conversión de `float` va por un helper del firmware —la misma rutina que usa el
intérprete—, así que da lo mismo que interpretado y acelera menos que el código
entero. Con FPU (Cortex-M33, P4) sigue siendo la instrucción. Y en las dos, sin
fundir `a * b + c` en una FMA (`-ffp-contract=off` en las recetas, `#560`): el
intérprete redondea dos veces, y el nativo también.

📌 El apunte de Eduardo que sigue valiendo, y que no era sobre el emisor: **si
buscas velocidad en el micro, usa `float`**. El Cortex-M33 lleva FPv5-**SP** y el
P4 va con ABI `ilp32f`: un `double` no toca la FPU en ninguna de las tres
familias. Que ahora *se pueda* no lo vuelve rápido.

📏 **Medido** (`#585`, 3-oct, la C3 —RV32IMC a 160 MHz, sin FPU—; `samples/DblNat.bp`, en
la ronda de V7: el mismo bucle de 50 000 vueltas × 4 operaciones, nativo e interpretado,
con el resultado igual al bit; dos pasadas iguales al ms):

| | nativo | interpretado | |
|---|---:|---:|---:|
| `double` | 307 ms | 1609 ms | ×5,2 |
| `float` (sin FPU: también emulado) | 200 ms | 1141 ms | ×5,7 |
| enteros (el control) | 9 ms | 943 ms | ×105 |

La estimación de V6 (×3 en la Metro, con las rutinas del ROM) se queda corta, pero el
fondo se confirma: la native se ahorra el intérprete y no la emulación —1,5 µs por
operación de `double`—. En una placa con FPU de simple precisión, `float` es la
instrucción: lo dirá `DblNat` en la ronda de cada placa.

### 2. `try` / `catch` DENTRO de una función `native`

Lo rechaza el compilador. Lanzar (`throw`) sí se puede.

**Qué hacer**: envolver la llamada a la función `native` en un `try` desde
código BP normal. Las excepciones que lance la nativa se propagan y se atrapan
ahí.

> ⚠️ Aquí ponía que «el `.mdn` no puede usar `setjmp` (#213)». **Era falso**: `__builtin_setjmp` y
> `__builtin_longjmp` se compilan en línea (medido el 24-ago: 52 B en ARM, 60 en RISC-V), sin símbolos
> externos. Lo que falta de verdad es DÓNDE aterriza un `throw`: el `eh_stack` del intérprete no sabe de
> un manejador nativo. Es `#578` (V7), con el planteamiento de Eduardo: darle a la native las primitivas de
> manejadores del intérprete y aterrizar con `__builtin_longjmp`.

### 3. Llamadas de `native` a funciones BP: sólo firmas de 4 bytes

El puente `native→BP` marshalla `integer`/`boolean`/`string`/arrays/refs, que
ocupan 4 bytes. `float`, `long`, `double` y `void` quedan fuera.

Ojo: llamar a BP desde `native` **pierde la velocidad AOT** (esa función corre
interpretada). El compilador lo avisa como `-- aviso AOT: ...` sin abortar. Y desde
V7 (P2b), si esa función BP se bloquea o agota su turno, la native **se suspende**
con ella y los demás hilos siguen (§7).

> ⚠️ **El 24-ago-2026 escribí aquí que este puente «nunca había llegado a una
> placa» porque su `find_function(vm, "Mod.func")` mete el nombre en `.rodata`.
> Era FALSO**, y conviene que se quede escrito por qué: comprobé el `.o` **sin el
> paso de ENLACE**, que el pipeline real hace siempre. El guión
> `bpgenvm-c/aot/mdn.ld` fusiona `.rodata` dentro de `.text` — eso es `#428`,
> cerrado en V5 y verificado en la Metro. Con el enlace, empaqueta.
>
> Lo caro del error no fue el código que sobraba: fue **medir una etapa que el
> producto no tiene** y sacar de ahí una conclusión sobre la placa.

### 4. ~~Builtins: sólo un subconjunto~~ — desde V7, CUALQUIER intrínseca (P2b.2)

Desde el 26-sep (`P2b.2`, ABI 9 del `.mdn`), una native llama a **cualquier función intrínseca** —las que
tienen el cuerpo `inline CALL_BUILTIN n`: `Math.sin`, `sqrt`, `startsWith`, `sleep`…— por el **mismo
despachador de builtins que el intérprete** (el helper `call_builtin`): ejecuta su código y no una copia, así
que la paridad sale por construcción. La excepción de un builtin sale por el límite de fallos (se atrapa en el
`try` BP de fuera), y un builtin que bloquea suspende la native (§7). La lista de antes —`now` · `charAt`,
`charCodeAt`, `substring`, `intToString` · `len` · `newIntArray`, `newByteArray`, `newLongArray`,
`newDoubleArray`— sigue por sus helpers tipados, más rápidos (5 millones de `charCodeAt`: 44 ms tipado, 96 por
el despachador, 257 interpretado), pero cada uno llama a LA MISMA primitiva que su builtin (`bpvm_prims.h`).

Y las **conversiones estrechas** (`byte(x)`, `int8`, `word`, `int16`) van en el C del módulo con el rango y el
mensaje del intérprete, sin tocar la ABI (27-sep).

La única intrínseca que se rechaza: **`Heap.compact()`** (`#595`, 28-sep): mueve los objetos del heap, y una
native guarda sus direcciones en variables de C.

### 5. Arrays: todos los elementos *(V6/N1.5)*

Crear e indexar arrays dentro de una `native` va para **todos** los tipos de
elemento: `integer`, `boolean`, `byte`, `word`, `short`, `long`, `double`,
`float`, y los de **referencias** (`string[]`, `Clase[]`, arrays anidados).
Cada ancho con su helper.

> 🐛 **La lección de este apartado, que valió por dos.** Hasta el 24-ago el
> emisor elegía el helper por el ancho **sólo para `byte`**, y para todo lo demás
> usaba el de 4 bytes: un `long[]` se leía de cuatro en cuatro y un `word[]`
> también. No fallaba — devolvía otro número. La limitación estaba escrita en un
> comentario («v1: solo integer[]») y no la hacía cumplir nadie. **Un límite que
> no falla no es un límite.**
>
> ⚠️ **Y el mismo día, el otro error, que es el de siempre**: aquí llegó a poner
> que los arrays de referencias *no se podían* porque «en esta ABI una ref cruza
> con la generación descartada». **Falso.** `bpref_regen(vm, ref)` la reconstruye
> desde la tabla de handles, y `write_ref` ya hacía justo eso en la frontera del
> thunk desde `#302`. Lo corrigió Eduardo: *«desde V5 todas las referencias a
> objetos, arrays y strings deberían poder pasarse tal cual»* — y se hicieron esa
> misma tarde, tres helpers de la misma forma que los demás. Leer un límite de
> **transporte** («no se lleva la generación») como uno de **capacidad** («no se
> puede recuperar») es el mismo error que convirtió cinco veces un «no está
> hecho» en un «no se puede».
>
> 📌 Donde SÍ se recupera la generación es en `array_store_ref`, y no es un
> detalle: guardar la palabra baja a secas dejaría el handle con generación 0, y
> un 0 no casa con nada — el objeto se leería como muerto al primer acceso. Un
> use-after-free, no un error.

📌 **Y un literal de cadena SÍ puede ir dentro** — `["a", intToString(n)]` compila
y empaqueta. Lo resuelve `#428` (V5): el paso de enlace fusiona `.rodata` dentro
de `.text`. El sample `NatNew.bp` lleva uno a propósito, para que se note si
alguna vez deja de ser verdad. Se fabrican con `intToString` y demás.

### 6. Construcciones sueltas

`for … in` sobre colecciones (el `for` numérico sí) · concatenación de string
sólo con `string`/`integer` · llamadas cross-module a intrínsecos · acceso a
método privado / `super` desde nativo · **campos** (leer o escribir `this.x`, propio
o heredado: «lee el campo 'x'…», «escribe en 'x'…»; una property se LEE, por su
getter) · **métodos y properties estáticos** («privado/super/estático pendientes»,
«property de modulo o estatica») · `instanceof` · eventos (`EventDef`, `RaiseStmt`) ·
`ParallelStmt` · referencias a método. Cada una tiene su mensaje propio con la línea,
y es **el mismo error en una native de módulo que en un método `native` de una
clase** («en el metodo native 'Caja.lee' (linea 7): …»). Ficha `#519` (V7, 24-sep,
decidido por Eduardo: «error, como en módulo»): hasta entonces un método se
degradaba a un aviso «corre interpretado» (V6/N1.1), y si el módulo no tenía además
una native de módulo, ni eso —el validador sólo miraba `module.defs`—.

Lo que SÍ cabe desde el 14-sep-2026: construir una clase con **constructor
sobrecargado** — la factoría lleva el sufijo de la sobrecarga elegida
(`__cls_new_<Cls>$s`); antes llamaba siempre a la cabeza con los argumentos de
la otra, sin aviso.

Y los identificadores de BP que chocan con los de C (`int`, `char`…) ya compilan: van
al C con su prefijo (`#572`, 27-sep).

### 7. Parar, ceder y suspenderse: una native es un hilo más *(V7/H5, P2b)*

Hasta V6 una native corría de un tirón: el Stop esperaba a que volviera y ningún otro hilo avanzaba. Desde V7
(idea de Eduardo: *«pensar en la ejecución de código nativo como un Thread BP, con su propio stack»*):

- **Se suspende** si llama a algo que bloquea —`sleep`, una función BP que espera un `Mutex`, un builtin que
  bloquea—, y los demás hilos siguen.
- **Cede el turno** en un cálculo largo: la *línea del bucle* —`punto_control` cada 1024 vueltas— la expropia a
  los 5 ms. Medido en la Pico 2 (P2b.0): +33 % en el bucle más apretado, +15 % en uno menos; el nativo sigue 134 y
  96 veces más rápido que interpretado.
- **Y desde `#592` (9-oct), el 2º temporizador** (idea de Eduardo): un bucle cuyo cuerpo LLAMA a algo (un helper, un
  builtin, una función BP, otra native) mira en CADA vuelta una marca que el reloj de la plataforma sube cada 5 ms —y
  el KILL también— (`vm->memory + BPVM_MARCA_TIEMPO_ADDR`, la región reservada: la ABI no cambia), así que cede a
  tiempo aunque cada vuelta sea lenta. En placa, `NatPesadaPlaca` (cada vuelta, un `indexOf` de 32 KB): con la imagen
  de antes colgaba la placa —ni el KILL, ni el cable—; ahora, PARADA a los 10–30 ms. En el PC, `make test-aotpesada`:
  350 ms en vez de 5,2 s. Cuesta algo más que el contador en esos bucles (+1,9 % en la Pico 2 y +0,5 % en la P4, en
  uno que recorre un array). Uno SIN llamadas sigue con el contador: sus vueltas son cortas por fuerza, y ahí la marca
  costaría mucho más (en la Pico 2, el bucle más apretado: 1875 frente a 1329 ms).
- **Una recursión cuenta como un bucle** (`#580`, 3-oct; Eduardo: *«como un bucle»*): una native que puede volver a
  entrar en sí misma por llamadas entre natives del módulo —`fib` → `fib`, `a` → `b` → `a`— lleva la marca a la
  ENTRADA: cada 1024 entradas, `punto_control`. Sin ella, una recursiva sin bucles ni cedía ni se podía parar (en la
  C3, `fib(40)` eran 32 s con el Stop sin efecto). El contador viaja con la llamada; la ABI no cambia. Cuesta
  +52 % en `fib(30)` en la C3, que es el peor caso (toda ella es llamar), y va a un ejecutor como una con bucle.
- **Se para**: un Stop (`KILL`) la detiene en esa misma línea (`make test-aotkill`: una en bucle infinito, en 99 ms,
  y una recursiva sin bucles). Y en placa, la tanda la para (`kill_tras_ms`): en la C3, `NatKill` y `NatRecurKill`
  (una recursiva sin bucles), PARADA a los 2 y 8 ms del KILL con el nativo en uso.

Cómo: la native corre en un **ejecutor** —un hilo de la plataforma con su pila: una tarea de FreeRTOS, un pthread
en el PC— y ella y el intérprete se pasan el testigo; sólo corre uno de los dos. Una native **directa** —sin puntos
de cesión, transitivamente— corre sin ejecutor (de 5,2 a 0,05 µs por llamada). Llamar a un pack no cede: SQLite y
Gui2 son directas. Las pruebas: `test-aotcede`, `test-aotexpropia`, `test-aotkill` y `test-aotdirecta` en
`check-testc`, y `NatCede`/`NatExpropia`/`NatBuiltin` en la ronda de placas.

Las TUPLAS, desde `#577` (7-oct): una native desestructura (`{ a, b } := f()`, también la tupla de otro módulo) y
construye (`return (a, b)`) por funciones ocultas de su módulo que sintetiza el compilador (`__tup_get_<forma>_<i>`,
`__tup_new_<forma>`), llamadas por el puente: una llamada al intérprete por campo. Sólo formas de valores de 4 bytes
(integer y estrechos, boolean, string, referencias); con `float`, `long` o `double`, error al compilar. Prueba:
`make test-aottuplas`.

### 8. Depurar: no dentro de una native *(V7, `#583`)*

Un breakpoint dentro de una native no para en la placa: el `CALL` salta al código máquina sin pasar por el bytecode
de la función. En el PC sí para (allí corre interpretada). **Decidido por Eduardo (9-oct)**: *«No depuramos las
natives. Si quieres ejecutar paso a paso le quitas native, para verificar el algoritmo vale. La haces native cuando
estás seguro que funciona. ¿Qué ocurre si la compilación de natives tiene un bug? Ese es nuestro trabajo,
asegurarnos que el compilador funciona correctamente. Se puede detectar con test de caja negra»* —las `test-aot*`,
cada native contra su interpretada, y la tanda, cada placa contra el PC—. El IDE lo dice al poner el breakpoint
(`NativaEnLinea`, en el compilador: por el texto, la `function` más cercana por encima y su `end`).

Los cabos del AOT que quedan, cada uno en su ficha: `try`/`catch` dentro (`#578`, en V7) y la línea de un error
dentro de una native (`#582`, en V8: la placa dirá dónde y el PC lo traducirá).

## Dónde se hace cumplir

Todo el gating vive en `lexer-java/src/main/java/basicplus/frontend/AotCEmitter.java`:

- `cType(...)` — la puerta de los tipos.
- `isBridgeI32Type(...)` — la puerta del puente `native→BP`.
- `arrElemKind(...)` — la puerta del ANCHO de elemento de un array. Devolver el
  helper equivocado aquí no da error: lee de más o de menos *(V6/N1.5)*.
- los `throw new UnsupportedAotException(...)` repartidos por los emisores de
  sentencia y expresión, uno por construcción no soportada.

El contexto (nombre de función + línea) lo añaden los dos bucles que recorren
las `native` en `emit()`: los sitios que lanzan no saben en qué función están,
así que el mensaje se enriquece donde sí se sabe.

**Si añades un recorte nuevo**: lánzalo como `UnsupportedAotException` con un
mensaje que diga *qué* no se puede y *qué alternativa* hay. El contexto de
función y línea se pone solo.

**Y si añades SOPORTE nuevo**: mueve el nodo en `AotCoberturaTest.SOPORTADOS`
—ése es el registro— y súbelo por los tres peldaños de abajo. El primero solo no
demuestra nada.

---

## 🔬 EL CENSO: qué construcciones pasan por el AOT

> **Ya no se escribe a mano.** Lo mide
> `lexer-java/src/test/java/basicplus/frontend/AotCoberturaTest.java`, que recorre
> los nodos del AST con un fragmento BP mínimo por cada uno, pregunta al emisor, y
> compara contra una foto esperada. Si alguien añade soporte —o lo rompe— **el
> test lo dice**, y la foto se mueve a propósito en vez de pudrirse sola.
>
> Este apartado existía porque el resto del documento **subestimaba**: nombraba
> cinco límites, y el censo a mano del 21-ago encontró veinticuatro. Una lista
> escrita a mano vuelve a desfasarse; ejecutar el compilador, no.

**Foto del 24-ago-2026: 28 de 30.** Faltan `TryStmt` y `TupleExpr`, los dos
aplazados a V7 a propósito (§2 y abajo).
**Foto del 7-oct-2026: 29 de 30.** `TupleExpr` entró con `#577`; falta `TryStmt` (`#578`).

### ⚠️ Y la lección de este censo no es el número: es que MEDÍA MAL

De los cuatro nodos que la foto anterior daba por no soportados, **`ThrowStmt` lo
estaba desde #186/#213**. Lo tapaba un fragmento mal escrito —`throw "vaya"`, que
#248 no permite—: el caso se moría en el análisis semántico y nunca llegaba a
preguntarle nada al emisor. Los otros cuatro casos estaban igual de mal, cada uno
por su motivo (una variable duplicada con un parámetro, dos sin declarar, un
`null` sobre un `string`, y `Core.Exception` sin Core cargado).

**Cinco de veintisiete, y la causa era del ARNÉS**: `pasa()` sólo miraba los
errores del *parser*. Un fragmento que parsea y no compila caía por el mismo
camino que uno no soportado, y los dos salían como «RECHAZA». Hoy:

- se rechaza aparte lo que no pasa el análisis semántico, con el motivo delante
  (*«NO COMPILA (el fragmento del test está mal)»*), que es lo que hizo visibles
  los cinco de golpe;
- los imports se resuelven con **el mismo código que el compilador**
  (`Main.loadImportsForAnalyzer` contra la stdlib de este checkout) en vez de con
  un analizador pelado sin `Core`.

Y tres nodos que N1 toca de lleno —`CallExpr`, la creación de objetos y
`MemberAccessExpr`— **no estaban en la lista**. Que un nodo no esté censado no es
«no soportado»: es *no medido*, que cuenta como nada.

### El censo mide EMISIÓN, no ejecución — y esa distancia muerde

Que el emisor acepte una construcción no dice que su C compile para el micro, ni
que dé el número correcto. Las dos cosas han fallado ya:

- **`long[]`** pasaba el censo, compilaba, empaquetaba… y devolvía **otro
  número** (24-ago): el emisor elegía el helper de 4 bytes. Sólo lo ve quien lo
  ejecuta.

Por eso la verificación de una construcción nueva son **tres peldaños**, no uno:

| peldaño | qué demuestra | con qué |
|---|---|---|
| 1. emite | el emisor la acepta | `AotCoberturaTest` |
| 2. cabe | compila y empaqueta para el micro | gcc de la familia → **ENLACE con `bpgenvm-c/aot/mdn.ld`** → `MdnPack` |
| 3. acierta | ejecutada da el valor exacto | `make test-aotnew` — compila el mismo `.c` con el compilador del host y lo corre con `gc_bump_threshold = 1` |

⚠️ **El enlace del peldaño 2 no es opcional y saltárselo miente hacia el lado
malo.** `MdnPack` sobre el `.o` rechaza cualquier literal (`.LC0`); sobre el
`.elf` enlazado lo acepta, porque `mdn.ld` fusiona `.rodata` en `.text` (`#428`).
Medir el `.o` me hizo declarar **tres límites que no existen** en dos días — el
`print` del 23-ago y, el 24, los literales de cadena y el puente native→BP.

El peldaño 3 es el que faltaba y el que cazó el `long[]`. Y hace la pregunta que
sólo se puede hacer ejecutando: **¿sobrevive al GC lo que la native fabrica?** El
handle recién creado vive en un local de C, y quien lo mira es el escaneo de la
pila de C del native (`heap.c` §2d, #302 paso 3, idea de Eduardo). Colectar en
cada alocación convierte esa ventana de lotería en certeza.

### Lo que queda fuera, y por qué

| falta | nodo | motivo |
|---|---|---|
| `try`/`catch` dentro de native | `TryStmt` | el `.mdn` no puede usar `setjmp` (§2). Lanzar sí. **A V7 «por lo menos ver si es posible»** (Eduardo, 24-ago): la frase de arriba dice qué NO se puede usar, no que no haya otro camino — y ese enunciado ya ha fallado cinco veces en este mismo documento |
| una tupla con `float`, `long` o `double` | `DestructAssignStmt`, `TupleExpr` | el puente native→BP lleva valores de 4 bytes: desestructúrala en BP y pásale los campos a la native. Las demás formas, hechas (`#577`, 7-oct) |
| `for … in` sobre colecciones | `ForInRange` | el `for` numérico sí |
| `instanceof` | `InstanceOfExpr` | sin medir siquiera: el fragmento que tenía no producía el nodo |
| eventos, `parallel`, referencias a método, `enum` | varios | aquí no es emitir C: es decidir qué significan esas construcciones cuando el que corre es código nativo y no la VM |

