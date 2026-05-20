# BasicPlus stdlib

Directorio donde viven los módulos estándar de BasicPlus en formato
compilado (`.mod`). La VM los carga cuando un programa hace
`import <Module>` y el `.mod` no está en el `cwd`.

## Módulos actuales

| Módulo | Fichero | Qué expone |
|---|---|---|
| `Json` | `Json.mod` | Parse + write JSON. Hoy sólo exporta `isDigitCode` y `hexDigit` en la `.bpi` por la limitación L2 (no se pueden exportar clases). |

## Cómo apuntar la VM aquí

Pon un `BpVM.cfg` en el `cwd` (o pasa `--config <ruta>`) con:

```json
{
  "stdlibDir": "C:/lenguajes/pm/bpstdlib"
}
```

Ver `docs/manual.html §15.2` para todos los campos disponibles.

## Cómo regenerar un módulo stdlib

Desde la raíz del repo:

```
cd lexer-java
java -jar target/basicplus-frontend.jar samples/Json.bp --compile ../bpstdlib --backend=mivm
```

Esto produce `Json.mod`, `Json.bpi` y `Json.dbg` en `bpstdlib/`.
