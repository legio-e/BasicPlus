# Fix VM: variables locales aliasing con la pila

## Síntoma

`Cliente.main()` corre un bucle `while (i < 4)` que llama a `Aritmetica.cuadrado(i)` (en realidad `x + x`, o sea `2*x`) y lo imprime. La salida esperada es:

```
VM [PRINT]: 2
VM [PRINT]: 4
VM [PRINT]: 6
=== FIN DE LA EJECUCIÓN ===
```

La salida observada en tu corrida fue:

```
VM [PRINT]: 2
... (varios ciclos PC ...)
VM [PRINT]: 2
... (y se sigue repitiendo)
```

Es decir, `cuadrado` siempre recibe `x = 1` y devuelve `2`.

## Causa raíz

En `main()` la VM arranca con `BP = SP = 32768`. La variable local `i` se almacena en `BP + 0 = 32768` (offset 0 calculado por `emitGetLocal` / `emitSetLocal`). **Pero la pila empieza también en `SP = 32768`**, así que el primer `push` de la función escribe en `memory[32768]` — exactamente el slot de `i`.

Trazado de iteración 2 (la primera ya imprimió `2` correctamente):

1. Tras `PRINT 2`, `SP = 32768` y `memory[32768] = 2` (residuo del valor de retorno).
2. `GET_LOCAL i` lee `memory[32768]` → empuja **2** (pero `i` debería seguir siendo `1`).
3. `PUSH 1`, `ADD` → 3. `SET_LOCAL i` → ahora `i = 3` (¡un salto de 2!).
4. Vuelta al inicio. `GET_LOCAL i` empuja `3`. `PUSH 4`. `LT` empuja `1` (resultado booleano), **sobrescribiendo otra vez `memory[32768]`** — ahora `i = 1`.
5. `GET_LOCAL i` lee `1` y se lo pasa a `cuadrado`.
6. `cuadrado(1) = 2`. `PRINT 2`. Bucle.

El problema general: **las variables locales y la pila ocupan la misma región de memoria**, y la pila no respeta el espacio reservado para los locales porque no hay un prólogo que ajuste `SP`.

## Solución

Se introduce un nuevo opcode `OP_ENTER = 0x0F` con un operando de 2 bytes (unsigned short) que indica el tamaño total de las locales en bytes. Cada función emite `OP_ENTER N` como primera instrucción de su prólogo, lo que sube `SP` por `N` bytes y deja la pila por **encima** del bloque de locales. Estructura del frame tras `OP_ENTER`:

```
   memory[BP - 20]   ... param[0] (x)              \
   memory[BP - 16]   ... param[1] (y)              | (depende del nº de params)
   memory[BP - 12]   ... saved PC                  \
   memory[BP -  8]   ... saved BP                  | contexto fijo de 12 bytes
   memory[BP -  4]   ... saved CS                  /
   memory[BP +  0]   ... local[0]      ← protegido por OP_ENTER
   memory[BP +  4]   ... local[1]
       ...
   memory[BP +  N]   ← SP justo tras OP_ENTER (pila operacional crece aquí)
```

### Cambios en `ModWriter.java`

- Nueva constante `public static final byte OP_ENTER = 0x0F;`
- Nuevas tablas internas `functionEnterOperandPos` y `functionLocalsBytes` para parchear el operando del `OP_ENTER` al cierre de cada función (cuando ya conocemos cuántas locales declaró).
- `addFunction(name, isPublic)` emite el opcode `OP_ENTER` + placeholder `short` de 2 bytes inmediatamente, registrando la posición del operando.
- `writeToFile(...)` cierra la última función abierta y parchea todos los `OP_ENTER` con el tamaño correcto (`currentLocals.size() * 4`).
- `addModulo(...)` limpia también las dos nuevas tablas y `callFixups` (que tampoco se limpiaba antes — un bug latente si reutilizabas un `ModWriter`).

### Cambios en `VirtualMachine.java`

- Nuevo case `0x0F` en `run()`:

```java
case 0x0F: { // OP_ENTER
    int localsBytes = readInt16(PC); // unsigned 0..65535
    PC += 2;
    SP += localsBytes;
    break;
}
```

`Main.java` y `ModuleManager.java` no necesitan cambios: las firmas del fichero `.mod` no cambian (sigue habiendo cabecera fija de 24 bytes y secciones imports/exports/code), solo aparece una instrucción nueva al inicio del bytecode de cada función.

## Compatibilidad con módulos antiguos

Los `.mod` generados con la versión anterior de `ModWriter` **no son compatibles**: les falta el prólogo. Hay que regenerar `Aritmetica.mod` y `Cliente.mod` con la nueva versión (basta con volver a ejecutar `Main.java`).

## Comprobación tras el fix

Tracé la ejecución con los nuevos opcodes y el resultado es:

| Iteración | i antes | x pasado | cuadrado(x) | PRINT |
|-----------|---------|----------|-------------|-------|
| 1         | 1       | 1        | 2           | 2     |
| 2         | 2       | 2        | 4           | 4     |
| 3         | 3       | 3        | 6           | 6     |
| 4         | 4       | —        | —           | (salto a FIN_WHILE, HALT) |

`i` ya nunca se corrompe porque vive en `BP+0` y la pila opera estrictamente desde `BP+4` en adelante.

## Otras observaciones menores (no críticas)

Mientras revisaba el código vi un par de detalles que no son bugs activos pero que conviene tener en mente:

1. `addModulo` no estaba limpiando `callFixups`. Si reutilizas el mismo `ModWriter` para varios módulos sin recrearlo, los fixups de llamadas se mezclan. Ya lo añadí al limpiado.
2. `emitGetParam` calcula offsets como `-12 - (4 * (size - index))`. Para 1 parámetro da `-16`, para 2 da `-20` y `-16` (en ese orden), etc. Funciona correctamente con el orden de push de `emitGetLocal` previo a la llamada.
3. La VM no comprueba desbordamiento de pila ni colisión con código. En memoria de 64 KB con stack en la mitad alta no es problema todavía, pero a futuro convendrá un check.
