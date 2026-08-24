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

**Lo que queda fuera es sólo la conversión `double(x)` escrita dentro de una
native.** El mensaje lo dice con su ficha.

📌 El apunte de Eduardo que sigue valiendo, y que no era sobre el emisor: **si
buscas velocidad en el micro, usa `float`**. El Cortex-M33 lleva FPv5-**SP** y el
P4 va con ABI `ilp32f`: un `double` no toca la FPU en ninguna de las tres
familias. Que ahora *se pueda* no lo vuelve rápido.

### 2. `try` / `catch` DENTRO de una función `native`

El `.mdn` no puede usar `setjmp` (#213). Lanzar (`throw`) sí se puede.

**Qué hacer**: envolver la llamada a la función `native` en un `try` desde
código BP normal. Las excepciones que lance la nativa se propagan y se atrapan
ahí.

### 3. Llamadas de `native` a funciones BP: sólo firmas de 4 bytes

El puente `native→BP` marshalla `integer`/`boolean`/`string`/arrays/refs, que
ocupan 4 bytes. `float`, `long`, `double` y `void` quedan fuera.

Ojo: llamar a BP desde `native` **pierde la velocidad AOT** (esa función corre
interpretada). El compilador lo avisa como `-- aviso AOT: ...` sin abortar.

> 🐛 **Y hasta el 24-ago-2026 este puente NO LLEGABA A UNA PLACA.** Emitía
> `find_function(vm, "Mod.func")`, y ese literal se va a `.rodata`; un `.mdn` se
> lleva `.text` y nada más, sin aplicar relocalizaciones, así que `MdnPack`
> rechazaba el `.o`. O sea que el puente estaba verificado por EMISIÓN, no por
> empaquetado — desde que existe (#211, V5). Afectaba a las llamadas del mismo
> módulo, a las cross-module y al `throw MiExcepcion(...)` de #213. El nombre va
> ahora byte a byte en la pila, como ya hacía el puente a un pack.

### 4. Builtins: sólo un subconjunto

Hoy: `now` · `charAt`, `charCodeAt`, `substring`, `intToString` · `len` (sobre
array o string; sobre una clase es una llamada a su `length()`/`size()` y va por
otro camino) · `newIntArray`, `newByteArray`, `newLongArray`, `newDoubleArray`
*(V6/N1.5)*. El resto se rechaza con el nombre del builtin en el mensaje.

### 5. Arrays: qué elemento sí y cuál no *(V6/N1.5)*

Crear e indexar arrays dentro de una `native` va para **integer, boolean, byte,
word, short, long y double** — cada ancho con su helper.

Fuera quedan dos, y por motivos distintos:

- **Arrays de REFERENCIAS** (`string[]`, `Clase[]`): un `TYPE_ARRAY_REF` guarda
  handles de 64 bits **con generación**, y en la ABI del AOT una referencia cruza
  con la generación descartada. Hay que decidir dónde se recupera la viva, y esa
  decisión no se toma de paso.
- **`float[]`**: no tiene helper de store (el valor tendría que cruzar como
  patrón de bits).

> 🐛 **Ojo con la lección de este apartado.** Hasta el 24-ago el emisor elegía el
> helper por el ancho **sólo para `byte`**, y para todo lo demás usaba el de 4
> bytes: un `long[]` se leía de cuatro en cuatro y un `word[]` también. No
> fallaba — devolvía otro número. La limitación estaba escrita en un comentario
> («v1: solo integer[]») y no la hacía cumplir nadie. **Un límite que no falla no
> es un límite**, y por eso ahora cada caso sin helper es un rechazo con motivo.

### 6. Construcciones sueltas

`for … in` sobre colecciones (el `for` numérico sí) · concatenación de string
sólo con `string`/`integer` · llamadas cross-module a intrínsecos · acceso a
método privado / `super` desde nativo · `instanceof` · eventos (`EventDef`,
`RaiseStmt`) · `ParallelStmt` · referencias a método. Cada una tiene su mensaje
propio con la línea.

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
demuestra nada: `print` los pasó el 23-ago y no compilaba para ARM.

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

- **`print`** pasaba el censo y **no compilaba para ARM** (23-ago): metía un
  literal en `.rodata` y `MdnPack` lo rechazaba.
- **`long[]`** pasaba el censo, compilaba, empaquetaba… y devolvía **otro
  número** (24-ago): el emisor elegía el helper de 4 bytes.

Por eso la verificación de una construcción nueva son **tres peldaños**, no uno:

| peldaño | qué demuestra | con qué |
|---|---|---|
| 1. emite | el emisor la acepta | `AotCoberturaTest` |
| 2. cabe | compila y empaqueta para el micro | `arm-none-eabi-gcc` + `MdnPack` (y el `riscv32-esp-elf-gcc` + enlace, para el P4) |
| 3. acierta | ejecutada da el valor exacto | `make test-aotnew` — compila el mismo `.c` con el compilador del host y lo corre con `gc_bump_threshold = 1` |

El peldaño 3 es el que faltaba y el que cazó el `long[]`. Y hace la pregunta que
sólo se puede hacer ejecutando: **¿sobrevive al GC lo que la native fabrica?** El
handle recién creado vive en un local de C, y quien lo mira es el escaneo de la
pila de C del native (`heap.c` §2d, #302 paso 3, idea de Eduardo). Colectar en
cada alocación convierte esa ventana de lotería en certeza.

### Lo que queda fuera, y por qué

| falta | nodo | motivo |
|---|---|---|
| `try`/`catch` dentro de native | `TryStmt` | el `.mdn` no puede usar `setjmp` (§2). Lanzar sí |
| desestructurar `{a, b} := t` | `DestructAssignStmt`, `TupleExpr` | **aplazado a V7** (decisión de Eduardo, 24-ago). Una tupla es un objeto sintético y sus elementos son CAMPOS; leerlos desde native pediría un helper de campo que hoy no existe |
| `for … in` sobre colecciones | `ForInRange` | el `for` numérico sí |
| `instanceof` | `InstanceOfExpr` | sin medir siquiera: el fragmento que tenía no producía el nodo |
| eventos, `parallel`, referencias a método, `enum` | varios | aquí no es emitir C: es decidir qué significan esas construcciones cuando el que corre es código nativo y no la VM |

