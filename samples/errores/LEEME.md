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
