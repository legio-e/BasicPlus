# V6 — el mapa del hito EN CURSO

> 🚀 **Desde el 23-ago-2026, V6 es la versión en curso** (V5 se publicó el 22). Este
> fichero nació como índice de lo aplazado; ahora es además la puerta de entrada al hito.
> El estado sigue en `FICHAS.md` §ABIERTAS, cuyo primer bloque es ya **V6**; los diseños,
> en `V6_IDEAS.md` (guardado en `docs/` ese mismo día: antes vivía fuera del repositorio).

> **Qué es esto y qué NO es.** Un **índice**, hecho el 21-ago a petición de Eduardo
> («hay muchas cosas aplazadas a V6, dame un listado»). Una línea por asunto y un
> puntero a dónde está el detalle.
>
> ⚠️ **NO es una fuente de verdad y no debe convertirse en una.** El estado sigue
> viviendo en **`FICHAS.md`**, y los diseños ya trabajados en **`docs/V6_IDEAS.md`**.
> Si algo aquí contradice a `FICHAS`, manda `FICHAS`. Este fichero existe porque lo
> aplazado estaba repartido en tres sitios y no había forma de verlo de un vistazo —
> que es la misma enfermedad que nos costó tiempo en V5.

**Recuento:** 36 asuntos vivos + **8 reflexiones de fundamentación** (§C, del 22-ago) +
5 diseños ya cerrados en `V6_IDEAS.md`, y 2 candidatos que salieron el 21-ago probando H13.
Uno de la lista se hizo ya en V5 y se ha movido a «CERRADAS» (ver el final).

📌 **Puesto al día el 23-ago**, al abrir V6: el índice se escribió el 21 y no recogía el
bloque de arquitectura del 22 — que es justamente la columna vertebral del hito.

➕ **Y ese mismo día, por decisión de Eduardo, lo que V5 dejó pendiente pasó a V6 y dejó
de ser de V5.** Eso ensancha el hito por encima de esta lista: en `FICHAS.md` §ABIERTAS
**todo es ya de V6**, incluido el bloque heredado (fichas del IDE, módulos y arranque,
placas, lenguaje, AOT y la cola de la SD). Este índice recoge los asuntos *nombrados*;
la cuenta real de fichas vive en `FICHAS`.

⚠️ **Con una salvedad medida:** de las 59 entradas del bloque heredado, **unas 43 llevan
marca de cierre**. Están donde están porque separarlas exige leerlas una a una. Así que
«pasa a V6» no significa «hay 59 tareas nuevas»: significa que ninguna se queda huérfana
en un hito que ya cerró.

---

## 🎯 EL PLAN DE V6 — primero unificar, luego arquitectura

**Decisión de Eduardo (23-ago):** unificar antes de seguir analizando, de lo más sencillo
a lo más complicado. *«Al estar todo unificado, cualquier cambio estructural se puede hacer
una vez y no 3 veces.»* El detalle de cada tarea está en `FICHAS.md` §«LOS HITOS DE V6».

| hito | qué | por qué ahí |
|---|---|---|
| 🟢 **U1** | `json_min` · el log de la Pico · el `.mdn` del STM32 · flash al contrato | no exige **decidir** nada: el contrato existe o las copias son idénticas |
| 🟡 **U2** | el transporte (wire) | primero **explicar** por qué dos ficheros con el mismo nombre difieren al 100 % |
| 🔴 **U3** | el REPL — 4.318 líneas **sin contrato** | el 80 % del problema; depende de U2 |
| 🟡 **U4** | la stdlib embebida: un formato en vez de dos | es un **generado**: se toca el generador |
| 🟡 **U5** | la tabla de handles: darle módulo | hoy repartida por 5 ficheros; desbloquea `#432` |
| ⬜ **A1** | la revisión **por niveles** | después de U1–U5, y ya sobre código único |

Y **siete hitos más**, que agrupan lo que ya estaba fichado y suelto (23-ago):

| hito | qué | nota |
|---|---|---|
| **N1** | **AOT**: ampliar la cobertura por tandas | ya existía, del 21-ago |
| **L1** | **lenguaje y compilador** | incluye el `Core` implícito, marcado *OBLIGATORIO* |
| **E1** | **el IDE** y el wire | `#412`, el CRC de dependencias, `RESET`, el árbol por color… |
| **G1** | **GUI**: el bucle de LVGL a un **hilo BP propio** | es una **solución** para `#434`, no un problema nuevo |
| **P1** | **ESP32-C3 y ESP32-C6** | las dos RISC-V; va **después** de la unificación, a propósito |
| **P2** | **pantallas SPI** | *después de P1*: la placa primero, el driver después |

📌 **U1–U5 no bloquean a los demás** — abaratan lo que venga detrás, pero N1, L1 y E1 tocan
sitios distintos. Con orden obligado sólo: A1 (tras la unificación), P2 (tras P1) y U3
(tras U2).

⚠️ **No entran aquí, a propósito:** packs nativos en S3/STM32, la SD del STM32, `LIST_DIR`
en el STM32 y la red en placa. **No son unificación: es funcionalidad que no existe.**

📐 La base de todo esto es `CENSO_SISTEMAS_V6.md`: 32 sistemas, de los que **22 ya están
donde deben** y sólo **3** de los que faltan son hardware de verdad.

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
| **El `.mdn` se funde en el `.mod`** — 🎯 **punto 4 del hito N1** *(23-ago: va con el AOT para probarlo todo junto en placa)* | `V6_IDEAS.md` §.mdn · `FICHAS` §HITO-AOT |
| **El `.mdn` no recuerda su RECETA** — huella de los flags de compilación | `FICHAS` §Aplazadas (mitad abierta del `#441`) |
| **`[ISA]` `CALL_REL`** — CALL local PC-relativo | `FICHAS` §Aplazadas |
| 🆕 **`native` en un MÉTODO se ignora en SILENCIO** — y el método corre interpretado mientras el programador cree que va a velocidad AOT. El AVISO *no espera a V6* (`AOT_LIMITES.md` L157): es barato | `FICHAS` L2244 |

## C · Arquitectura — el reparto común/hardware

> Es **el eje grande de V6**: partir común y hardware para poder meter pruebas en medio.
> El modelo declarado es el VFS de SQLite.

### 🧠 La fundamentación — el bloque de reflexiones del 22-ago

No son tareas: son **el porqué y la medida** sobre los que se planifica el eje. Salieron
todas de una misma mañana, al cerrar V5, y están en `FICHAS.md` con su desarrollo.

| reflexión | lo que ya contesta | `FICHAS` |
|---|---|---|
| **POR QUÉ UNIFICAR** — la tesis de Eduardo | memoria y FS dejaron de dar guerra **después** de unificarlos: la unificación es el paso previo a la fiabilidad | L2345 |
| **La tesis económica** — «no es una opción, es el único camino» | el tiempo no se va en el compilador sino en **sistemas y pruebas**; unificar es lo que hace el crecimiento lineal y no exponencial | L2302 |
| **EL CRITERIO DE CAPAS** | sólo hardware / HAL / BP HAL deben diferir. Contrastado contra el código: **el REPL está TRIPLICADO, ~220 KB** | L2492 |
| **EL INVENTARIO** de lo unificado y lo que falta | el reparto de hoy en números (57 `.c` comunes frente a los privados por familia), para planificar sobre datos | L2531 |
| **¿QUÉ INCLUYE el «sistema operativo» común?** | y dónde encaja cada pieza — en particular los packs, que se montan casi antes que todo lo demás | L2458 |
| **¿UN boot o DOS?** | la división **ya está empezada sin nombre**: `bpvm_boot_climb()` es una escalera común cuyos peldaños los pone cada familia | L2423 |
| **¿CUÁNTO cuesta una familia nueva** (C3/C6)? | el P4 es el experimento que ya lo responde: casi todo lo hecho sirve; lo nuevo es el boot y **sobre todo pruebas** | L2269 |
| **EL SIMULADOR CON DISFRACES** | que `bpvm-sim` pueda vestirse de cada familia y cazar problemas de integración en el PC, que en placa cuestan caro | L2384 |

| asunto | dónde |
|---|---|
| **`#378`** — que cada micro **DIGA** lo que tiene (capa HAL BP de capacidades) | `FICHAS` §Aplazadas |
| **`#432`** — dónde debe vivir la tabla de handles, y de qué tamaño | `FICHAS` §Aplazadas |
| **Librería de placa GENÉRICA** — el micro da el dato, la librería no lo sabe | `FICHAS` §Aplazadas |
| **El tamaño de flash lo dice la placa** — tabla grande + clamp | `FICHAS` §Aplazadas |
| **La S3 no tiene `bios_s3.c`** — no ofrece tabla BIOS | `FICHAS` §Aplazadas |
| **`SD_INFO`/`SD_MOUNT` siguen sólo en `pico/repl_v1.c`** — no han subido al común | `FICHAS` §Aplazadas (de H6) |
| **La unificación que dejó el censo `#427`** — lo que no cupo en V5 | `FICHAS` L805 |
| 🆕 **UNIFICAR los packs**: implementación común + cintura por hardware (la diferencia real cabe en UNA función) | `FICHAS` §Aplazadas (22-ago, decisión de Eduardo) |
| 🆕 **[S3] los packs: encaminados pero SIN REGIÓN** — el mismo agujero que `#327`; la S3 es la única familia que no los expone | `FICHAS` L1384 |
| 🆕 **Un módulo rancio sobrevive y NADIE lo dice** (ESP32 y STM32) — y `/app` es el punto ciego: tiene preferencia sobre `/lib` | `FICHAS` L1119 |
| 🆕 **El CENSO FUNCIONAL** — el de V5 es por fichero; Eduardo especificó el 17-ago ampliarlo a cuatro ejes (función · específico-vs-común · capas · memoria y tiempos) | `CENSO_FAMILIAS.md` §«El censo de V6» |

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
| ✅ ~~**NO copiar dependencias que el dispositivo YA TIENE** — y que lo diga él~~ — HECHO el 2-sep (`#466` paso 3) | `FICHAS` `#466` |
| **El verbo `RESET` no llega con un RUN vivo** (era `#452`) | `FICHAS` §Aplazadas |
| **PROBAR BASES DE DATOS SIN PLACA** — packs en el PC | `FICHAS` §Aplazadas |
| **El árbol de ficheros por COLOR** según el tipo (rojo RESERVADO) | `FICHAS` §Aplazadas |
| 🆕 **Enseñar el `durationMs` que la placa YA manda** y nadie imprime | `FICHAS` §Aplazadas (21-ago) |
| 🆕 **Al fallar una dependencia, decir DE DÓNDE salió el módulo — por CRC** | `FICHAS` §Aplazadas (22-ago, idea de Eduardo) |

## F · Placas

| asunto | dónde |
|---|---|
| **El ESP32-P4X** — silicio nuevo, imagen aparte, 400 MHz | `FICHAS` §Aplazadas (20-ago) |
| **Los 32 MB de flash del P4 y el XIP de los packs** — dos caminos, ninguno barato | `FICHAS` §Aplazadas |
| **`#434`** — desacoplar los eventos del lazo de LVGL | `FICHAS` §Aplazadas |
| 🆕 **Un estado persistente dejó la Metro SIN PODER EJECUTAR NADA** — sólo se curó reparticionando. Sin causa identificada: es el riesgo abierto más feo que deja V5 | `FICHAS` L1446 |

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

### ✅ Y otra de la misma sesión — DECIDIDA el 23-ago

La fachada de ficheros enruta por prefijo más largo **con la raíz de respaldo**. Con la
tarjeta fuera, escribir en `/sd/...` **no da error: cae en silencio al FS interno**, así
que un programa escribe en la flash de la placa creyendo que escribe en la tarjeta.

**Decisión de Eduardo: `/sd` pasa a ser un PREFIJO RESERVADO** — existe en la tabla de
montajes aunque no haya nada montado, y sin tarjeta **da error en vez de desviar**. La
idea viene de `/dev` y `/mnt` de Linux, y de paso arregla lo que Linux hace mal ahí.
Detalle y qué tocar, en `FICHAS` §«DECIDIDAS el 23-ago». ⚠️ Es cambio de comportamiento:
va en las notas de versión.

## H · Fuera de versión (V6+)

- **Linux**: el IDE en Linux + la Raspberry Pi como PLACA. Marcado `[SIN VERSIÓN, V6+]`.
- **V7 — LVGL a un pack, y el diseño del código nativo empotrable**: cuántos caben, cómo
  publican su API y si hablan entre ellos. Encargo de Eduardo (21-ago) → **`docs/V7_IDEAS.md`**.

---

## Ya NO aplica

- ~~**`List` con captadores TIPADOS**~~ — estaba en la lista de V6 y **se hizo en V5**
  el 20-ago, adelantada por Eduardo (*«las demos han de funcionar»*). `getInteger`,
  `getLong`, `getDouble`, `getBoolean` y `getString`, con las conversiones en los
  envoltorios. ✅ **Ya movida** en `FICHAS.md` a «CERRADAS EN V5», con sus 89 líneas de
  diseño intactas — porque su cola sigue viva: **¿quiere `Map` lo mismo para sus
  valores?** `SyncList` y `OwnerList` los heredan gratis; `Map` no.
