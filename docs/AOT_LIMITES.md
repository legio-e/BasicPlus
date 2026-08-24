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

> ⚠️ **El 24-ago-2026 escribí aquí que este puente «nunca había llegado a una
> placa» porque su `find_function(vm, "Mod.func")` mete el nombre en `.rodata`.
> Era FALSO**, y conviene que se quede escrito por qué: comprobé el `.o` **sin el
> paso de ENLACE**, que el pipeline real hace siempre. El guión
> `bpgenvm-c/aot/mdn.ld` fusiona `.rodata` dentro de `.text` — eso es `#428`,
> cerrado en V5 y verificado en la Metro. Con el enlace, empaqueta.
>
> Lo caro del error no fue el código que sobraba: fue **medir una etapa que el
> producto no tiene** y sacar de ahí una conclusión sobre la placa.

### 4. Builtins: sólo un subconjunto

Hoy: `now` · `charAt`, `charCodeAt`, `substring`, `intToString` · `len` (sobre
array o string; sobre una clase es una llamada a su `length()`/`size()` y va por
otro camino) · `newIntArray`, `newByteArray`, `newLongArray`, `newDoubleArray`
*(V6/N1.5)*. El resto se rechaza con el nombre del builtin en el mensaje.

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
| desestructurar `{a, b} := t` | `DestructAssignStmt`, `TupleExpr` | **aplazado a V7** (decisión de Eduardo, 24-ago). Una tupla es un objeto sintético y sus elementos son CAMPOS; leerlos desde native pediría un helper de campo que hoy no existe |
| `for … in` sobre colecciones | `ForInRange` | el `for` numérico sí |
| `instanceof` | `InstanceOfExpr` | sin medir siquiera: el fragmento que tenía no producía el nodo |
| eventos, `parallel`, referencias a método, `enum` | varios | aquí no es emitir C: es decidir qué significan esas construcciones cuando el que corre es código nativo y no la VM |

