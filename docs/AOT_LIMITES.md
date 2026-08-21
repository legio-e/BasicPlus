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

### 1. `double` (pero `long` YA no: entra desde V5)

⚠️ **Este apartado decía que `long` tampoco cruzaba, y dejó de ser cierto el
16-ago-2026** (tarea #381, verificada en la Metro). Hoy una función `native`
acepta y devuelve `long` con normalidad: se marshalla como 8 bytes big-endian.
Se probaron sumas, anchos mezclados en una misma firma, la división y el módulo
por helper, y las conversiones en los dos sentidos.

Y de aquella tarea salió algo que importa más que el tipo: **dividir por cero
desde código nativo lanza un error de BP atrapable** en vez de reiniciar la
placa.

**Lo que sigue fuera es `double`.**

**Qué hacer**: `float` si la precisión de 32 bits basta; o quitar `native` de
esa función concreta —seguirá funcionando, interpretada— y dejar el resto del
módulo en nativo.

> **Y hay una razón de fondo, no sólo del emisor** (apunte de Eduardo): la FPU
> de estos micros es de **precisión simple**. El Cortex-M33 lleva FPv5-**SP**
> —o sea RP2350 y STM32U5, dos de las tres familias— y ahí un `double` **no
> toca la FPU**: se emula en software, con un coste de otro orden.
>
> Es decir, que marcar `native` una función que usa `double` sería pedir
> velocidad y elegir a la vez el camino lento. El recorte del AOT y lo que
> conviene hacer apuntan al mismo sitio: **si buscas velocidad en el micro, usa
> `float`**. `double` sigue estando disponible en el código interpretado para
> cuando lo que importa es la precisión y no el reloj.

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

### 4. Builtins: sólo un subconjunto

Hoy: `now`, `charAt`, `charCodeAt`, `substring`, `intToString`. El resto se
rechaza con el nombre del builtin en el mensaje.

### 5. Construcciones sueltas

`for`-range no numérico · concatenación de string sólo con `string`/`integer` ·
llamadas cross-module a intrínsecos · acceso a método privado / `super` desde
nativo. Cada una tiene su mensaje propio con la línea.

## Dónde se hace cumplir

Todo el gating vive en `lexer-java/src/main/java/basicplus/frontend/AotCEmitter.java`:

- `cType(...)` — la puerta de los tipos (el caso de 8 bytes sale de aquí).
- `isBridgeI32Type(...)` — la puerta del puente `native→BP`.
- los `throw new UnsupportedAotException(...)` repartidos por los emisores de
  sentencia y expresión, uno por construcción no soportada.

El contexto (nombre de función + línea) lo añaden los dos bucles que recorren
las `native` en `emit()`: los sitios que lanzan no saben en qué función están,
así que el mensaje se enriquece donde sí se sabe.

**Si añades un recorte nuevo**: lánzalo como `UnsupportedAotException` con un
mensaje que diga *qué* no se puede y *qué alternativa* hay. El contexto de
función y línea se pone solo.

---

## 🔬 EL CENSO: cuánto le falta al AOT para tragar CUALQUIER código BP

> Medido el 21-ago, a pregunta de Eduardo. **Este apartado existe porque el resto del
> documento subestima**: nombra cinco límites, y son los cinco que alguien se encontró de
> frente y escribió. La cobertura real la define lo que el emisor tiene implementado, no
> una lista de excepciones — y por eso hay rechazos genéricos (`statement no soportado`,
> `expression no soportada`, `TypeRef no soportado`) esperando a lo que no se contempló.

**Método**: comparar los nodos de AST que `AotCEmitter` sabe emitir contra los que
`Ast.java` define y un usuario puede escribir. **30 soportados frente a ~54.**

### 🔴 La barrera de verdad, y NO es `double`: no hay `this`

`ThisExpr`, `ClassDef`, `PropertyDef`, `GetterDef` y `SetterDef` tienen **cero menciones**
en el emisor. La consecuencia es grande y conviene decirla entera:

> **Un MÉTODO no puede ser `native`. Sólo las funciones sueltas de un módulo.**

En un lenguaje donde todo desciende de `Object` y el código de usuario vive en clases, eso
deja fuera a la mayoría del código BP que se escribe — **independientemente de qué tipos
crucen la frontera**. Por eso `double` (que entra en V6) no es la barrera que más pesa:
aunque cruzaran todos los tipos del mundo, seguiría sin poder marcarse `native` un método.

### 🟡 Lo cotidiano que sorprende (y es barato)

Comprobado ejecutando el compilador, no leyendo: `print "hola"` dentro de una `native`
da `AOT: statement no soportado: PrintStmt`.

| falta | nodo |
|---|---|
| **`print`** | `PrintStmt` |
| **`null`** | `NullLitExpr` |
| literales de array (`[1,2,3]`) | `ArrayLitExpr` |
| **`do … loop`** | `DoLoopStmt` |
| `for … in` sobre colecciones | `ForInRange` / `ForRange` |
| desestructurar `{a, b} := t` | `DestructAssignStmt` |
| tuplas | `TupleExpr`, `TupleTypeRef` |
| `instanceof` | `InstanceOfExpr` |

📌 Varias de estas son **azúcar y se emiten casi solas**: `do…loop` es un `while` al
revés, `null` es un cero, `print` es una llamada al runtime que ya existe. Son las que
mejor relación esfuerzo/cobertura tienen, y ninguna está en el documento de arriba.

### 🟠 Lo profundo

`EventDef` y `RaiseStmt` (eventos) · `ParallelStmt` · `MethodRefExpr` · `SuperCallExpr` ·
`BoundCallExpr` (el `import X:Y`) · `EnumDef`. Aquí no es emitir C: es decidir qué
significan esas construcciones cuando el que corre es código nativo y no la VM.

### El resumen que contesta la pregunta

| | |
|---|---|
| Límites documentados arriba | **5** |
| Construcciones del lenguaje sin soporte, medidas | **~24** |
| La que más pesa | **`this` → no hay métodos `native`** |
| Las más baratas | `print`, `null`, `do…loop`, literales de array |
| Ya con fecha | `double` → V6 · fundir `.mdn` en `.mod` → V6 |

⏭️ **Y la conclusión de método**: mientras el emisor tenga rechazos genéricos, esta lista
se queda rancia sola. Lo que la mantendría honesta es un **test que recorra los nodos del
AST** y compruebe cuáles pasan por el AOT — el mismo censo de arriba, pero automático y
corriendo en la batería.
