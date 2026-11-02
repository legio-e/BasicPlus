# samples/errores — los que NO compilan, y está bien

**Todos los programas de esta carpeta fallan a propósito.** No son ejemplos rotos: son
la demostración de que el compilador **detecta** lo que tiene que detectar, y cada uno
apunta a una regla concreta del lenguaje.

Si abres uno en el IDE y le das a compilar, verás errores. Eso es el resultado
correcto. Lo que sería un fallo es que **compilara**.

La razón de tenerlos aparte es que en `samples/` no se distinguían de un ejemplo que se
hubiera quedado rancio, y eso confunde a quien abre el paquete y nos quita la señal a
nosotros: un sample que deja de compilar por una regresión se camuflaba entre los que
fallan por diseño.

## Qué prueba cada uno

| Programa | La regla que demuestra | El diagnóstico que debe salir |
|---|---|---|
| `BadSyntax.bp` | el parser **se recupera** de un error y sigue analizando en vez de rendirse en el primero | varios errores sintácticos en líneas distintas (16, 22, 31…), no uno solo |
| `DefaultParamsBad.bp` | las tres reglas de los parámetros por defecto (H8.1) | `debe ser una constante literal` · `sin valor por defecto no puede ir tras uno con valor por defecto` · `no asignable a` |
| `classleak.bp` | una clase **no** puede llamar a una función privada de su módulo | `la clase 'Indiscreta' no puede llamar a 'helperPrivado' del módulo (no es public)` |
| `narrowtypes_errors.bp` | los rangos de los enteros estrechos se comprueban en compilación (L10) | `literal 300 fuera del rango de byte (0..255)` y tres más: `int8`, `word`… |
| `EnumMal.bp` | un enum es nominal y cerrado (`Color(1)` no es una conversión), y en un `+` con string el otro operando tiene que tener conversión a string: primitivos, enums (su entero) y objetos; un array, una tupla, `null` o una llamada void, no (#522) | `'Color' es un enum: no se llama ni se convierte desde un entero` · `'+' incompatible: 'string' y 'integer[]' (no hay conversión a string)` y tres más |
| `CtorMal.bp` | un constructor no delega en sí mismo ni en ciclo, ni delega y además llama a `super(...)`; y la visibilidad de un constructor es la de cualquier miembro: `private` sólo su clase, sin marcar = protegido (su clase y sus subclases), `public` cualquiera (#511/#515); y `super(...)` con argumentos a un padre sin constructor —sin ellos vale, y no hace nada— (#753) | `no puede delegar en sí mismo` · `se delegan en ciclo` · `no puede llamar además a 'super(...)'` · `'Cerrada()' es private` · `'Familiar()' es protegido` · `'Base()' es private` · `no define constructor` |
| `StepCero.bp` | el `step` de un `for` numérico es integer y no puede ser 0 escrito (#506) | `el 'step' del 'for' no puede ser 0` · `debe ser integer, no 'float'` |
| `TopeClase.bp` | el tope de una clase cerrada: 256 ranuras de método y 256 de campo (8 bits); una clase abierta (extiende a una de otro módulo) no lo tiene (#529) | `la clase 'Ancha' ocupa 258 ranuras de campos…` · `la clase 'Larga' tiene 257 métodos…` |
| `NativeCampo.bp` | un método `native` que el AOT no puede traducir es un **error**, igual que una función native de módulo: `native` promete velocidad, y si no se puede cumplir se dice al compilar (#519). Una native lee el estado de un objeto por una property (su getter), no un campo | `en el metodo native 'Caja.doble' (linea 15): AOT: lee el campo 'n'…` |
| `ProtegidoMal.bp` | lo **protegido** es de la familia (la clase que lo declara y sus descendientes) y, sobre otro objeto, sólo si es de la clase que lo usa o de una hija suya, la regla de Java y C#: desde `B`, sobre un objeto que es sólo un `A` —podría ser un hermano de `B`—, no; el campo se lee por una property pública. Y desde fuera de la familia, nada (#760) | `desde 'B' se usa sobre un 'B' o una hija suya, y este objeto es un 'A'…` · `'campo' es protegido de 'A': lo ven esa clase y sus descendientes…` |
| `NativeConversion.bp` | una **conversión de referencia** dentro de una `native` (`Perro(a)`, con `a` un `Animal`) es un error: su comprobación —que el objeto sea de esa clase, o lanza— vive en el intérprete. Antes el AOT la tomaba por una construcción, sin un error: un `Perro` nuevo con el handle de `a` por argumento (#757) | `en la funcion native 'patasDe' (linea 21): AOT: convertir a 'Perro' (line 22) no se traduce en una native…` |
| `LocalSinValor.bp` | usar una variable **local** antes de darle valor es un error: sin inicializador su valor es indeterminado, y hay que asignarla en todos los caminos antes de leerla (#540). Las globales sí nacen con valor | `la variable local 'total' se usa sin tener valor…` · `'nombre' se usa sin tener valor…` |
| `SwitchMal.bp` | el sujeto de un `switch` es un entero, un boolean, un enum o un string (comparado por su contenido): un **real** no —la igualdad exacta de reales es una trampa—, ni un `case` real contra un entero, y un **objeto** tampoco (comparar referencias es cosa de `if`) (#546) | `switch sobre 'double': un real no se compara…` · `valor de 'case' de tipo 'float'…` · `…para comparar referencias, usa if` |
| `PropFieldTipo.bp` | dentro de un `get`/`set`, `field` tiene el tipo de su property: asignarle otro es un error, como a cualquier variable (#547) | `no se puede asignar 'string' a 'integer'` |
| `FinallySale.bp` | un `finally` no puede salir con `return`, `break` ni `continue`: taparía la excepción que fuera en vuelo. Un `throw` dentro sí vale, y un `break` a un bucle de DENTRO del `finally`, también (#674) | `un 'finally' no puede salir con 'return'` · `… con 'break'` · `… con 'continue'` (y ninguno en `vale`) |
| `FirmaLarga.bp` | la firma de una función —su nombre con los tipos de sus parámetros— tiene un tope de 160 caracteres, también en las funciones de módulo: aquí, tres parámetros de una clase de 63 dan 240 (#685) | `la firma 'demasiado(…' de 'demasiado' tiene 240 caracteres (maximo 160)` |
| `UnaLineaConFin.bp` | el `if`, `while` y `for` de una línea no llevan terminador: escribirlo en su misma línea (`if E then STMT endif`) es mezclar las dos formas. Uno en la línea siguiente es del bloque de fuera, y vale (#701: `bpgenvm-c/samples/UnaLineaAlFinal.bp`, en el corpus) | `el 'if' de una línea no lleva 'endif'` · `'endwh'` · `'next'` |
| `paralleltest_scope_violate.bp` | dentro de un `parallel`, un `case` no puede tocar una variable global | `no puede acceder a la variable global 'globalVar'; usa una const, una función o un objeto compartido` |
| `CompuestoMal.bp` | una asignación compuesta, `x op= e`, es `x := x op e` con la regla de C#: si el resultado no se guarda solo en el tipo de `x`, sólo vale cuando `e` sí se guardaría sola en `x` (y entonces se recorta al tamaño de `x`, como `b += 1` con `b` byte); el operador, el de siempre: `-=` con números, `&=` con enteros (V8/H7.1, #747) | `'*=': 'integer' * 'float' da 'float', que no se guarda solo en 'integer'` · `'-=' requiere numéricos` · `'&=' requiere integer/long` · `…que no se guarda solo en 'byte'` |
| `GenericosMal.bp` | los genéricos de lo que ya hay son `List<Object>` y `Collections.Map<integer, Object>` o `<string, Object>` (lo demás, de momento no); la llave de un Map tipado se comprueba al compilar, y su fábrica casa con ella (`numericMap()` para la integer, `Map()` para la string); `for x in` recorre un array o una List (V8/H7.2, #749) | `una lista es List<Object>` · `la llave de un Map es integer o string` · `el valor de un Map es Object` · `'Cosa' no es genérica` · `una llave integer necesita Collections.numericMap()` · `la llave de este Map es integer, no 'string'` · `se esperaba un array o una List en 'for in'` |
| `InterfacesMal.bp` | las interfaces de clase (V8/H7.3, #756): una interfaz sólo tiene métodos y properties y no hereda en ciclo; quien la implementa tiene, propio o heredado, cada método con su firma y su retorno, público, y lo dice con `@implement{ … }` nombrando una interfaz suya; `@override{}` sólo en lo que redefine; una anotación, una vez; una interfaz no se extiende ni se construye, y una clase no se implementa; un `Thread` no implementa interfaces (no es un `Object`) | `no tiene campos` · `ciclo de herencia entre interfaces` · `y no tiene 'area(): double' (tiene 'area(): integer')` · `tiene que ser public` · `márcalo con @implement{ Forma }` · `no es una interfaz de` · `no redefine nada` · `repetida` · `no se extiende` · `va en 'extends'` · `un Thread no es un Object` · `no se construye` |
| `ConstanteNula.bp` | una **constante** no puede ser `null`: es un valor, no una variable, y no referencia nada; una **variable** `string` sí puede valer `null` (V8, #777 y #774) | `la constante 'NADA' no puede ser null: una constante es un valor, no una variable…` (y `ETIQUETA`, de clase, y `LOCAL`, en una función) |
| `ArgLargo.bp` | el **valor por defecto del argumento de ejecución** (`Main(arg: string := …)`) tiene que caber en él: el tope de las dos VMs, 127 bytes de UTF-8. Antes la VM-C lo cortaba sin decirlo y la VM-Java no. Y el que se da al ejecutar, lo mismo: la línea de órdenes de las dos VMs y el `RUN` del wire lo rechazan con su texto (#764) | `el valor por defecto de 'arg' no cabe: 130 bytes (el tope del argumento es 127)` · y al ejecutar, `el argumento no cabe: 128 bytes (el tope es 127)` |
| `IifMal.bp` | el ternario, `iif(condición, a, b)`: la condición es boolean; las dos ramas dan el mismo tipo (o números, o `null` y una referencia, o una clase y su hija); un `var owner` local no es una rama (el ternario no mueve un owner); es un valor, no una sentencia; y el nombre es del lenguaje (V8/H7.1, #747) | `'iif' es el ternario de BP` · `se esperaba boolean` · `las dos ramas tienen que dar el mismo tipo` · `se esperaban 3 argumentos` · `iif no mueve un owner` · `es un valor, no una sentencia` |

Fíjate en que los mensajes **dicen qué hacer**, no sólo qué está mal: el de `parallel`
sugiere las tres salidas válidas. Eso es deliberado.

## Se verifica solo

`scripts/h13-errores.sh` compila los cinco y exige que **cada uno falle con el mensaje
esperado**. Falla la comprobación en los dos sentidos: si alguno empieza a compilar
(regla que se perdió) o si el mensaje cambia (diagnóstico que empeoró sin querer).

```bash
bash scripts/h13-errores.sh
```
