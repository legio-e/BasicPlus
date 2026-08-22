# samples/pendientes — buenos, pero hoy no compilan

**Esta carpeta NO viaja en la distribución**, y esa es toda su razón de ser.

Aquí van los ejemplos que **deberían funcionar y hoy no**, por un fallo nuestro y no
por un error suyo. Son distintos de las dos carpetas vecinas y conviene no mezclarlos:

| carpeta | qué contiene | ¿viaja? |
|---|---|---|
| `samples/` | ejemplos que funcionan | sí |
| `samples/errores/` | programas que **fallan a propósito**: demuestran que el compilador detecta lo que debe | sí |
| **`samples/pendientes/`** | **ejemplos correctos que hoy no compilan por un bug** | **no** |

⚠️ **Por qué no van a `errores/`**: su `LEEME` lo dice — esa carpeta existe para que *«un
sample que deja de compilar por una regresión no se camufle entre los que fallan por
diseño»*. Meter aquí un bug sería justo lo contrario.

⚠️ **Y por qué no se borran**: son la **prueba de regresión**. El día que compilen, el
fallo está arreglado y vuelven a `samples/`.

## Qué hay hoy

### `appv1lsp.bp` y `appv2.bp` — la sustitución de Liskov entre interfaces de módulo

`BufferedLogger` implementa `com.example.LogApiV2`, que **extiende** `com.example.LogApi`.
Un cliente que pide `LogApi` debería aceptarlo, y el propio compilador dice que ese es el
diseño (`Main.java`: *«el impl puede implementar la interfaz pedida directamente o
cualquier descendiente de ella»*). Lo rechaza igual:

```
error: 'BufferedLogger' no implementa 'com.example.LogApi' (directa o
transitivamente; declara com.example.LogApiV2)
```

`appv2.bp` cae por lo mismo visto del otro lado: *«el módulo importado 'LogApiV2' no expone
'log'»* — que son los miembros que **hereda**.

📌 **No es regresión de V5** (nada que ver con `any`→`Object`) y está fichado en
`docs/FICHAS.md` con el diagnóstico: `implSatisfies` es correcto en forma, pero una
`module interface` pura **no genera `.mod`**, así que la cadena sólo se recorre
recompilando la interfaz en modo `INTERFACE_ONLY` — la pasada con el bug ya aparcado.
