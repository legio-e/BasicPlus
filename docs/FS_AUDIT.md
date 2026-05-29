# FS Audit — estado actual del filesystem de la Pico

Documento para cerrar la tarea #132 (P-fs-audit). Mezcla **estado
documentado por el código** (extraído de `bpgenvm-c/pico/fs.{h,c}`) +
**tests prácticos pendientes** que solo se pueden hacer con hardware
en la mano.

Estado a fecha: tras cierre de H3 (AOT). Versión del FS: **v4**
(`FS_VERSION = 4` en `fs.c`).

## 1. Diseño actual

### Capacidad
| Parámetro | Valor | Definido en |
|---|---:|---|
| Slots máximos | 64 ficheros | `FS_MAX_FILES` |
| Tamaño de cada nombre | 40 bytes (incluye NUL) | `FS_NAME_LEN` |
| Espacio total de datos | 128 KB | `FS_DATA_SIZE` |
| Tamaño de header en flash | 4 KB (1 sector) | `FS_HEADER_BYTES` |
| Región total en flash | 132 KB | `FS_REGION_SIZE` |
| Offset en flash | últimos 132 KB del chip de 4 MB | `FS_FLASH_OFFSET` |

### Modelo
- Los ficheros viven en RAM (mirror del contenido de flash).
- Datos alineados a 4 bytes (necesario para que `.mdn` Thumb-2 ejecute
  zero-copy desde el buffer FS).
- `fs_save_to_flash()` persiste todo el estado: erase + program de los
  33 sectores. Bloquea ~50 ms con IRQs OFF.
- `fs_init()` al boot intenta cargar de flash; si magic inválido o
  versión distinta de v4 → FS vacío.

### Códigos de error
| Código | Significado |
|---|---|
| `FS_OK` | OK |
| `FS_ERR_NOT_FOUND` | Lookup falló |
| `FS_ERR_EXISTS` | Nombre ya existe (no usado por `fs_put`, sí por código de cliente) |
| `FS_ERR_NO_SPACE` | No cabe en los 128 KB |
| `FS_ERR_NAME_TOO_LONG` | Nombre > 39 chars |
| `FS_ERR_TOO_BIG` | Single file > 128 KB |
| `FS_ERR_TABLE_FULL` | 64 slots ocupados |
| `FS_ERR_BAD_FLASH` | Magic/versión inválidos al cargar |
| `FS_ERR_INVALID` | Argumentos inválidos |

## 2. Comportamiento esperado (según código)

| Escenario | Comportamiento esperado |
|---|---|
| **Subir N ficheros sin SAVE + power-cycle** | Desaparecen todos. RAM no persiste sin `fs_save_to_flash`. |
| **Subir N ficheros + SAVE + power-cycle** | Los N ficheros aparecen tras boot. `fs_init` los carga del flash. |
| **Overwrite del mismo nombre** | El `fs_put` libera el slot antiguo y reusa el nombre. Tras el fix de #111, no debería corromper USB CDC. |
| **FS lleno (>128 KB)** | `fs_put` devuelve `FS_ERR_NO_SPACE`. Sin compactación automática — habría que llamar a `fs_delete` primero. |
| **>64 ficheros** | El 65º `fs_put` devuelve `FS_ERR_TABLE_FULL`. |
| **Reboot por watchdog** | Si hubo `fs_save_to_flash` previo, recupera el último snapshot. Cambios desde el último SAVE se pierden. |
| **Fichero mayor que 128 KB** | `fs_put` devuelve `FS_ERR_TOO_BIG`. |
| **Versión vieja en flash (v3 o anterior)** | `fs_init` invalida → arranca con FS vacío (v3→v4 cambió alineación; ver comentario en `FS_VERSION`). |

## 3. Tests prácticos a realizar

Marca `[x]` cuando lo verifiques en hardware. Si el comportamiento real
difiere de la columna "Esperado", anótalo en "Notas".

| # | Test | Esperado | Verificado | Notas |
|---|---|---|---|---|
| T1 | Subir 5 ficheros (sin SAVE) → power-cycle → `fs_list` | FS vacío | `[ ]` | |
| T2 | Subir 5 ficheros → SAVE manual → power-cycle → `fs_list` | 5 ficheros, mismo contenido | `[ ]` | |
| T3 | Subir A.mod 2 KB → SAVE → sobreescribir A.mod 3 KB → SAVE → power-cycle → leer A.mod | A.mod de 3 KB, contenido correcto | `[ ]` | Cierra el bug #111 |
| T4 | Subir ficheros hasta llenar (~ 128 KB) → próximo PUT | `FS_ERR_NO_SPACE`, no corrupción | `[ ]` | |
| T5 | Subir 65 ficheros pequeños (de 100 bytes) | El 65º devuelve `FS_ERR_TABLE_FULL` | `[ ]` | |
| T6 | Trigger watchdog tras subir 3 ficheros sin SAVE | Tras reboot solo aparecen los que estaban en el último SAVE | `[ ]` | |
| T7 | Reboot por hard fault (asserción) tras subir + SAVE | FS coherente al recuperar | `[ ]` | |
| T8 | Borrar todos los ficheros + SAVE + power-cycle | FS vacío persiste | `[ ]` | |
| T9 | Ejecutar `.mdn` directamente desde FS (zero-copy) | Funciona — alineación a 4 garantizada por v4 | `[x]` | Validado al cerrar H3 (SortBench.mdn) |
| T10 | Llamadas concurrentes a `fs_put` durante un `fs_save_to_flash` | No definido en v1; documentar si se puede o no | `[ ]` | Probable: bloquea con IRQs OFF, no hay race. |

## 4. Limitaciones conocidas (decisión consciente para v1)

- **No hay compactación automática**: borrar un fichero deja un slot
  con `size = 0` pero su offset en `s_data` no se libera hasta que
  algún `fs_put` reordene. Para v1 esto es aceptable porque la mayoría
  de los usos son "subir N veces y olvidar".
- **No hay versionado en disco**: si se actualiza el firmware con un
  layout nuevo y el chip tiene FS viejo, se pierde el FS (no se
  intenta migrar). Mitigación: bump de `FS_VERSION` invalida limpio.
- **Mirror completo en SRAM**: 128 KB de mirror + 128 KB en flash =
  256 KB de la RAM de la Pico consumidos por el FS. Si en el futuro
  hace falta FS más grande (varios MB), habría que rediseñar leyendo
  páginas de XIP directamente — pendiente, fuera del scope de v1.
- **No hay directorios reales**: la jerarquía `/lib/`, `/app/`, `/sys/`
  se implementa como **prefijos de nombre** (ver #133 P-fs-hierarchy
  completada). No hay `mkdir`/`rmdir`.

## 5. Veredicto para v1

El FS actual es **suficiente para v1**: cubre el caso de uso
(subir N módulos, persistirlos, ejecutarlos) con un código simple
(~300 LOC) y semántica clara. Las limitaciones de §4 son aceptables.

Los tests T1-T8 quedan pendientes de verificar en hardware. Si todos
pasan, esta tarea se cierra. Si T3 falla (bug #111 recae) o T4/T5
muestran comportamiento inesperado, abrir issue.

Para v2 (si llega): pensar en allocator con compactación incremental
y FS sin mirror completo en SRAM.
