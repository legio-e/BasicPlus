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
| `CtorMal.bp` | un constructor no delega en sí mismo ni en ciclo, ni delega y además llama a `super(...)`; y la visibilidad de un constructor es la de cualquier miembro: `private` sólo su clase, sin marcar = protegido (su clase y sus subclases), `public` cualquiera (#511/#515) | `no puede delegar en sí mismo` · `se delegan en ciclo` · `no puede llamar además a 'super(...)'` · `'Cerrada()' es private` · `'Familiar()' es protegido` · `'Base()' es private` |
| `StepCero.bp` | el `step` de un `for` numérico es integer y no puede ser 0 escrito (#506) | `el 'step' del 'for' no puede ser 0` · `debe ser integer, no 'float'` |
| `TopeClase.bp` | el tope de una clase cerrada: 256 ranuras de método y 256 de campo (8 bits); una clase abierta (extiende a una de otro módulo) no lo tiene (#529) | `la clase 'Ancha' ocupa 258 ranuras de campos…` · `la clase 'Larga' tiene 257 métodos…` |
| `NativeCampo.bp` | un método `native` que el AOT no puede traducir es un **error**, igual que una función native de módulo: `native` promete velocidad, y si no se puede cumplir se dice al compilar (#519). Una native lee el estado de un objeto por una property (su getter), no un campo | `en el metodo native 'Caja.doble' (linea 15): AOT: lee el campo 'n'…` |
| `paralleltest_scope_violate.bp` | dentro de un `parallel`, un `case` no puede tocar una variable global | `no puede acceder a la variable global 'globalVar'; usa una const, una función o un objeto compartido` |

Fíjate en que los mensajes **dicen qué hacer**, no sólo qué está mal: el de `parallel`
sugiere las tres salidas válidas. Eso es deliberado.

## Se verifica solo

`scripts/h13-errores.sh` compila los cinco y exige que **cada uno falle con el mensaje
esperado**. Falla la comprobación en los dos sentidos: si alguno empieza a compilar
(regla que se perdió) o si el mensaje cambia (diagnóstico que empeoró sin querer).

```bash
bash scripts/h13-errores.sh
```
