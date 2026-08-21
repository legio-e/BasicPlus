# V6 — el mapa de lo aplazado

> **Qué es esto y qué NO es.** Un **índice**, hecho el 21-ago a petición de Eduardo
> («hay muchas cosas aplazadas a V6, dame un listado»). Una línea por asunto y un
> puntero a dónde está el detalle.
>
> ⚠️ **NO es una fuente de verdad y no debe convertirse en una.** El estado sigue
> viviendo en **`FICHAS.md`**, y los diseños ya trabajados en **`notas/V6_IDEAS.md`**.
> Si algo aquí contradice a `FICHAS`, manda `FICHAS`. Este fichero existe porque lo
> aplazado estaba repartido en tres sitios y no había forma de verlo de un vistazo —
> que es la misma enfermedad que nos costó tiempo en V5.

**Recuento:** 29 asuntos vivos + 4 diseños ya cerrados en `V6_IDEAS.md`, y 2 candidatos
que salieron el 21-ago probando H13. Uno de la lista se hizo ya en V5 y se ha movido a «CERRADAS» (ver el final).

---

## A · Lenguaje y modelo de objetos

| asunto | dónde está el detalle |
|---|---|
| **`Object` = comodín por referencia** — decidido y diseñado | `FICHAS` §Aplazadas |
| **Liberación de recursos**: destructor `~Clase()` + `var owner` + `FREE_REF` (hoy sin sintaxis) | `FICHAS` §Aplazadas |
| **Ficheros como CLASE** — no hay `seek` porque no hay `open`. **DECIDIDO**: dos clases y la segunda hereda | `V6_IDEAS.md` §Ficheros (≈180 líneas, con las preguntas abiertas) |
| **`#19`** — array fijo LOCAL: el UAF ya está cerrado, queda que sea inline de verdad | `FICHAS` §Aplazadas |
| **`#396`** — módulo `Time` con `Time.Date` sobre un `long` de segundos | `FICHAS` §Aplazadas |
| **`Math`** — ampliar (`fact` sobrecargada, f64) y repasar | `FICHAS` §Aplazadas |
| 🆕 **La sustitución por LSP entre interfaces de módulo NO funciona** | `FICHAS` §Lenguaje y VM (fichado el 21-ago) |

## B · AOT y código nativo

| asunto | dónde |
|---|---|
| **`#426` — `double` en funciones `native`** | `FICHAS` + **diseño hecho** en `V6_IDEAS.md` §double (con ganancia estimada sobre datos reales) |
| **El `.mdn` se funde en el `.mod`**, con un bloque nativo por familia | `V6_IDEAS.md` §.mdn (≈90 líneas: por qué no es una apuesta, qué mata, la trampa) |
| **El `.mdn` no recuerda su RECETA** — huella de los flags de compilación | `FICHAS` §Aplazadas (mitad abierta del `#441`) |
| **`[ISA]` `CALL_REL`** — CALL local PC-relativo | `FICHAS` §Aplazadas |

## C · Arquitectura — el reparto común/hardware

> Es **el eje grande de V6**: partir común y hardware para poder meter pruebas en medio.
> El modelo declarado es el VFS de SQLite.

| asunto | dónde |
|---|---|
| **`#378`** — que cada micro **DIGA** lo que tiene (capa HAL BP de capacidades) | `FICHAS` §Aplazadas |
| **`#432`** — dónde debe vivir la tabla de handles, y de qué tamaño | `FICHAS` §Aplazadas |
| **Librería de placa GENÉRICA** — el micro da el dato, la librería no lo sabe | `FICHAS` §Aplazadas |
| **El tamaño de flash lo dice la placa** — tabla grande + clamp | `FICHAS` §Aplazadas |
| **La S3 no tiene `bios_s3.c`** — no ofrece tabla BIOS | `FICHAS` §Aplazadas |
| **`SD_INFO`/`SD_MOUNT` siguen sólo en `pico/repl_v1.c`** — no han subido al común | `FICHAS` §Aplazadas (de H6) |
| **La unificación que dejó el censo `#427`** — lo que no cupo en V5 | `FICHAS` L805 |

## D · Memoria y GC

| asunto | dónde |
|---|---|
| **Muro de contención entre el heap y las pilas** | `FICHAS` §Aplazadas |
| **Diagnóstico del heap DESDE BP** — las herramientas existen, falta exponerlas | `FICHAS` §Aplazadas |
| **`#356`** — REBAJADO: la pérdida de bytes no se manifiesta | `FICHAS` §Aplazadas |

## E · IDE y protocolo wire

| asunto | dónde |
|---|---|
| **`#412` — `run miModulo <arg>`**, el argumento SIEMPRE en el heap | **diseño hecho** en `V6_IDEAS.md` §run |
| **NO copiar dependencias que el dispositivo YA TIENE** — y que lo diga él | `FICHAS` §Aplazadas |
| **El verbo `RESET` no llega con un RUN vivo** (era `#452`) | `FICHAS` §Aplazadas |
| **PROBAR BASES DE DATOS SIN PLACA** — packs en el PC | `FICHAS` §Aplazadas |
| **El árbol de ficheros por COLOR** según el tipo (rojo RESERVADO) | `FICHAS` §Aplazadas |
| 🆕 **Enseñar el `durationMs` que la placa YA manda** y nadie imprime | `FICHAS` §Aplazadas (21-ago) |

## F · Placas

| asunto | dónde |
|---|---|
| **El ESP32-P4X** — silicio nuevo, imagen aparte, 400 MHz | `FICHAS` §Aplazadas (20-ago) |
| **Los 32 MB de flash del P4 y el XIP de los packs** — dos caminos, ninguno barato | `FICHAS` §Aplazadas |
| **`#434`** — desacoplar los eventos del lazo de LVGL | `FICHAS` §Aplazadas |

## G · Pruebas y medida

| asunto | dónde |
|---|---|
| **Batería de rendimiento HW+SW** — medir el REPARTO, no el tiempo | `FICHAS` §Aplazadas |
| **Prueba de resistencia larga** — días de carga VARIADA | `FICHAS` §Aplazadas |
| 🆕 **Que la batería de host corra TAMBIÉN sin GUI** | ver abajo — es la lección de H13 |

### 🧠 La que sale de H13 y conviene no perder

El 21-ago se encontró el `#414` **en placa**, no en la cascada, y el porqué importa más
que el bug: **la VM-C de host se construye con `GUI=1` por defecto y los firmwares sin
pantalla no**. Así que había builtins que en el host existían y en la placa no, y la
regla *«lo que no funciona en C tampoco funcionará en la Pico»* **no se sostenía**: sólo
vale si el C que se prueba lleva la MISMA configuración que la placa.

⏭️ Lo barato: que el arnés corra también en la configuración sin GUI. Lo más barato aún:
un censo que compare los builtins **compilados** de cada imagen contra los que emite el
compilador — habría cazado esto sin ejecutar nada.

### 🆕 Y otra de la misma sesión, sin decidir

La fachada de ficheros enruta por prefijo más largo **con la raíz de respaldo**. Con la
tarjeta fuera, escribir en `/sd/...` **no da error: cae en silencio al FS interno**. Es
deliberado, pero para un prefijo de dispositivo extraíble significa que un programa
escribe en la flash de la placa creyendo que escribe en la tarjeta. ¿Debería `/sd` sin
montar dar error en vez de desviar? **Sin decidir.**

## H · Fuera de versión (V6+)

- **Linux**: el IDE en Linux + la Raspberry Pi como PLACA. Marcado `[SIN VERSIÓN, V6+]`.

---

## Ya NO aplica

- ~~**`List` con captadores TIPADOS**~~ — estaba en la lista de V6 y **se hizo en V5**
  el 20-ago, adelantada por Eduardo (*«las demos han de funcionar»*). `getInteger`,
  `getLong`, `getDouble`, `getBoolean` y `getString`, con las conversiones en los
  envoltorios. ✅ **Ya movida** en `FICHAS.md` a «CERRADAS EN V5», con sus 89 líneas de
  diseño intactas — porque su cola sigue viva: **¿quiere `Map` lo mismo para sus
  valores?** `SyncList` y `OwnerList` los heredan gratis; `Map` no.
