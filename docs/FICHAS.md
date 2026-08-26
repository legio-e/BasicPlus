# BasicPlus — Registro de FICHAS

> **Por qué existe este fichero.** Las fichas se numeran (`#384`, `#417`…) y vamos
> por el 400 y pico, pero hasta el 14-ago-2026 **no estaban en ningún fichero**:
> vivían en la lista de tareas de una sesión y en su transcript. Consecuencia
> medida ese mismo día: se dio una lista de 11 pendientes cuando había **36**, se
> dio por pendiente lo que estaba hecho (H7) y lo que se había decidido no hacer
> (la release 4.0.1). Un número de ficha que no se puede buscar no sirve de nada.
>
> **Cómo se mantiene.** Al abrir una ficha, una línea aquí. Al cerrarla, se marca
> con su commit y se queda — el registro de lo cerrado es lo que evita volver a
> darlo por pendiente. Una línea por ficha, empezando por `#NNN`, para que un
> `grep` conteste.
>
> ## 🔒 ESTE FICHERO ES LA FUENTE ÚNICA DE VERDAD
>
> **Decisión de Eduardo (17-ago):** *«Estado y pendientes son ficheros de trabajo
> tuyos. Pero el que dice realmente cuál es la situación es Fichas.»* Si
> `docs/ESTADO.md` o `docs/PENDIENTES.md` dicen otra cosa que este fichero, **manda
> éste** y lo otro se corrige, no al revés.
>
> El motivo no es de orden, es que ya costaba tiempo: el mismo 17-ago, `PENDIENTES`
> daba por abiertos dos bugs cerrados ese día, `ESTADO` daba por pendiente un censo
> hecho, y por la mañana `ESTADO` contradecía a la ficha #417. Medido: de las 51
> fichas que citaba `ESTADO`, **49 eran una segunda copia de las de aquí**. Eduardo:
> *«me estoy volviendo loco con cosas que aparecen y desaparecen»*.
>
> **Dónde vive.** `docs/FICHAS.md`, **versionado en git desde el 17-ago**. Estuvo en
> `notas/` (ignorado por git) mientras era material de la versión en curso, pero una
> fuente de verdad sin historial no tiene red: los commits de esa sesión no pudieron
> incluirla. Sigue sin publicarse hasta que V5 cierre — versionar y publicar son
> cosas distintas.
>
> Reconstruido el 14-ago del último `task_reminder` del transcript de la sesión
> `25fabe6b`, más el `git log`.

---

## 🚀 V5 PUBLICADA — 22-ago-2026

Los cuatro pasos del plan de cierre, hechos. **El congelado queda levantado** y lo que
entre a partir de ahora es V6 (índice en `V6_BACKLOG.md`).

| qué | dónde |
|---|---|
| commit publicado | `ff408a1e` |
| etiqueta | `v5.0` |
| release | https://github.com/legio-e/BasicPlus/releases/tag/v5.0 |
| artefacto **único** | `BasicPlus-5.0-win.zip` — 24.403.893 B, `sha256 093789a6…a5f9` |
| web | https://legio-e.github.io/BasicPlus/ y `/en/`, build `built` sobre `ff408a1e` |

**Verificado, no supuesto:** el ZIP se **descargó desde la propia release** y su sha256
coincide con el que se montó y se probó; las dos webs se consultaron con `curl` y sirven
V5 (no bastaba con que el build dijera `built` — el fallo de V4 fue exactamente ése).

📌 **Tres cosas que se encontraron al publicar y que ya están corregidas en
`PUBLICAR.md`**, para que no vuelvan a morder:

1. **Las cuatro portadas seguían en `v4.0`** —las dos del repo y las dos de Pages— y las
   tarjetas de la web ofrecían «lo diferido a v5», que ya era v6.
2. **El número de versión hay que subirlo ANTES de montar el ZIP**, porque `docs/` viaja
   dentro: se hizo al revés, hubo que rehacer el ZIP *después* de que Eduardo lo hubiera
   probado y volver a verificarlo. Salió barato de milagro (la lista de ficheros quedó
   idéntica porque las sustituciones conservaban la longitud).
3. **El checklist mentía sobre el cuerpo de la release**: decía «= sección de
   `RELEASES.md`», que es sólo español; V4 publicó de verdad un cuerpo **bilingüe** con
   Descarga y Documentación. Se descubrió comparando con lo que V4 hizo, no con lo que
   el doc decía.

📌 **Y una decisión de contenido:** entran al repo público `bp_propuesta_modelo_memoria/`
(el estudio de mayo-julio del modelo de memoria) y `diag/orm-slots/`. Al primero se le
escribió **portada** (`0517af68`) porque no se abría por ninguna parte y citaba 13 veces
unas carpetas `hallazgos/` y `fuentes/` que nunca estuvieron en el repo.

---

## ABIERTAS

> **Desde el 23-ago-2026 TODO lo de esta sección es de V6.** V5 se publicó el 22-ago;
> las secciones ya terminadas bajaron a «CERRADAS EN V5», y **lo que quedó pendiente pasó
> a V6 y dejó de ser de V5** — decisión de Eduardo ese mismo día. Así que aquí no hay dos
> categorías: hay fichas de V6, unas abiertas *durante* V5 y otras heredadas *de* V5.
>
> ⚠️ **Queda una limpieza sin hacer, y conviene saberla:** en el bloque heredado hay **59
> entradas y unas 43 llevan marca de cierre** (✅, «CERRADA», «HECHO» o un commit). O sea
> que buena parte de lo que se lee aquí ya está resuelto y debería estar abajo. No se ha
> movido porque distinguirlas exige leerlas una a una —un clasificador automático ya falló
> en dos— y eso es trabajo aparte.

### ═══ V6 — LA VERSIÓN EN CURSO ═══

El índice de todo lo aplazado está en `V6_BACKLOG.md`; los diseños ya trabajados, en
`V6_IDEAS.md`. Aquí vive el estado.

### 🎯 LOS HITOS DE V6 — unificar primero, arquitectura después

**Decisión de Eduardo (23-ago), y su razón:** *«antes de hacer más cosas deberíamos
empezar unificando, de lo más sencillo a lo más complicado. Una vez tengamos los sistemas
unificados podemos volver a estudiarlos desde el punto de vista de arquitectura, por
niveles. Pero al estar todo unificado, cualquier cambio estructural se puede hacer una vez
y no 3 veces.»*

📐 **De dónde sale el orden:** del censo de sistemas (`CENSO_SISTEMAS_V6.md`), que midió
qué está unificado y qué cuesta cada cosa. No es una lista de deseos: cada tarea dice qué
tocar y cómo se comprueba.

| hito | qué | cerrado |
|---|---|---|
| **U1** | lo que **no exige decidir nada** — 4 tareas, una por sesión corta | abierto 23-ago |
| **U2** | el **transporte** (wire): entender los gemelos falsos y darle contrato | abierto 23-ago |
| **U3** | el **REPL** — el trabajo de verdad: 4.318 líneas sin contrato | abierto 23-ago |
| **U4** | la **stdlib embebida**: un solo formato de blobs | abierto 23-ago |
| **U5** | la **tabla de handles**: darle módulo | abierto 23-ago |
| **A1** | *(después de U1–U5)* la revisión **por niveles**, ya sobre código único | — |
| **N1** | **AOT**: ampliar la cobertura por tandas *(encargo del 21-ago)* | abierto 21-ago |
| **L1** | **lenguaje y compilador** | abierto 23-ago |
| **E1** | **el IDE** y el protocolo wire | abierto 23-ago |
| **G1** | **GUI**: el bucle de LVGL a un **hilo BP propio** | abierto 23-ago |
| **P1** | **placas nuevas**: ESP32-**C3** y ESP32-**C6** | abierto 23-ago |
| **P2** | **pantallas SPI** — *después de P1* | abierto 23-ago |

📌 **U1–U5 no bloquean a los demás.** La unificación es lo que se hace *primero* porque
abarata todo lo que venga detrás, pero N1, L1 y E1 tocan sitios distintos y pueden avanzar
en paralelo si apetece cambiar de aire. Los que **sí** tienen orden son A1 (después de la
unificación), P2 (después de P1) y U3 (después de U2).

⚠️ **Fuera de esta serie, y a propósito**: los packs nativos en S3/STM32, la SD del STM32,
`LIST_DIR` en el STM32 y la red en placa **no son unificación, son funcionalidad que no
existe**. Van al saco de «qué falta» y no bloquean U1–U5.

---

#### 🖼️ G1 — el bucle de LVGL a un hilo BP propio

**Idea de Eduardo (23-ago):** *«de LVGL me gustaría, si podemos, mejorar el bucle,
poniéndolo en un hilo BP sólo para él»*.

📌 **No es un problema nuevo: es una FORMA DE SOLUCIÓN para `#434`**, que ya está fichada.
Hoy un clic tiene que **atravesar el lazo de BP** para llegar a su handler —el upcall lo
encola y sólo se drena entre quanta, y el único punto de quantum es la vuelta de
`Gui.run()`—, así que **el evento no avanza mientras el bombeo duerme**.

⏭️ **Por qué encaja**: BasicPlus tiene **hilos preemptivos de verdad**, no `async`. Un hilo
dedicado al bombeo de LVGL desacopla el ritmo de la GUI del quantum de la aplicación **en
todas las familias a la vez**, en vez de ajustar un número por placa — que es justo lo que
`#434` pedía evitar.

⚠️ **Lo que hay que resolver antes de escribir código**, y no es menor: LVGL **no es
reentrante**. Si el bombeo vive en su hilo y los builtins de `Gui` se llaman desde el hilo
de la app, hay dos hilos tocando LVGL. Hace falta decidir el candado —o que los builtins
encolen y sea el hilo de la GUI quien ejecute— antes de tocar nada.
📐 Y lo que `#434` ya pedía sigue valiendo: **medir primero** el camino clic→handler
aparte del camino invalidar→pintar, e instrumentar el STM32 como está el P4.

#### 🧩 L1 — lenguaje y compilador

Agrupa lo que ya está fichado y suelto por el registro. **No duplica: agrupa** — el texto
de cada una sigue en su sitio.

- 🔴 **La pasada de INTERFAZ no resuelve `Core` implícito** — marcada *V6 OBLIGATORIO*.
- **La sustitución por LSP entre interfaces de módulo** no funciona *(probablemente el
  mismo problema visto dos veces que el anterior)*.
- **`Object` = comodín por referencia** — decidido y diseñado.
- **Liberación de recursos**: destructor `~Clase()` + `var owner` + `FREE_REF`.
- **Ficheros como CLASE** — decidido: dos clases y la segunda hereda.
- **`Map`**: objetos internos para claves **y** valores + `add` sobrecargado *(decidido el
  23-ago; ver arriba)*.
- **`Math`, ampliar** — `fact` sobrecargada y f64, más lo que salió de mirar BASIC256:
  `atan2`, `remap`, `clamp`, `hypot`, `wrap`. Todas son cálculo puro y de pocas líneas.
- **`#19`** array fijo LOCAL: que sea inline de verdad · **`#396`** módulo `Time`.

#### 💻 E1 — el IDE y el wire

- **`#412`** — `run miModulo <arg>`, con el argumento siempre en el heap *(diseño hecho)*.
- **NO copiar dependencias que el dispositivo YA TIENE** — y que lo diga él.
- **Al fallar una dependencia, decir DE DÓNDE salió el módulo**, por CRC *(idea de Eduardo)*.
- **El verbo `RESET` no llega con un RUN vivo** *(era `#452`)*.
- **PROBAR BASES DE DATOS SIN PLACA** — packs en el PC.
- **El árbol de ficheros por COLOR** según el tipo · **enseñar el `durationMs`** que la
  placa ya manda y nadie imprime.

#### 🔌 P1 — las placas nuevas: ESP32-C3 y ESP32-C6

**Encargo de Eduardo (23-ago).** Encaja con la prioridad ya escrita —[[prioridad-arm-riscv-s3-secundario]]:
ARM y RISC-V primero— porque las dos son **RISC-V**.

✅ **Ya hay una estimación hecha, y es buena noticia**: la ficha *«¿cuánto cuesta una familia
nueva si antes unificamos?»* la contesta con el precedente del P4 — el IDF es el mismo para
toda la familia ESP32, así que casi todo lo hecho sirve y **lo nuevo es el arranque y, sobre
todo, trabajo de pruebas**.
⏭️ **Y por eso este hito va DESPUÉS de U1–U5**, no antes: cada sistema sin unificar es una
copia más que escribir para cada micro nuevo. Es el argumento entero de V6, aplicado.

##### 📐 EL COSTE, MEDIDO (26-ago) — a pregunta de Eduardo

No estimado a ojo: contado sobre el precedente del P4, que es la última familia añadida.

| de dónde sale su código | ficheros |
|---|---|
| el común (`src/`) | **53** |
| reutilizados del S3 | **10** |
| **propios de la familia** | **7** |

**Siete ficheros.** Ése es hoy el tamaño de «una familia ESP nueva» — y con U3 terminado
uno de esos siete (su REPL) también se cae.

✅ **La memoria NO es el problema que parecía**, y el precedente vuelve a ser del S3: ya
funciona **sin PSRAM**, reservando la VM en SRAM interna (`MALLOC_CAP_INTERNAL`), 160 KB
con **respaldo automático a 128 KB** si no cabe, y avisando por el log. El C6 (512 KB) está
en el mismo rango que el S3; el C3 (400 KB) es el más justo, pero el mecanismo de respaldo
ya existe y ya habla.

🔴 **LA TRAMPA, y conviene saberla antes de empezar: `riscv` NO ES UN TARGET, SON DOS.** El
catálogo (`NpackReloc.DESTINOS`) tiene una sola entrada RISC-V, `riscv32-esp-p4`, con
`-march=rv32imafc -mabi=ilp32f` — o sea **con FPU**. El C3 y el C6 son **rv32imac, sin
coma flotante**: su ABI es `ilp32`. Un `.mdn` del P4 **no vale** en un C6 ni al revés.
📌 Es exactamente la clase de fallo que costó media tarde el 25-ago con el ARM
softfp/hard — pero ya **no puede pasar callado**: el guardián de `MdnPack` compara la
float-ABI del `.o` y el firmware declara la suya (`bpvm_mdn_host_float_abi`). Fallaría con
mensaje. Lo que hay que hacer es dar de alta un destino nuevo y decidir su sufijo (hoy
`RISCV` a secas se quedaría ambiguo).

⏭️ **Reparto del trabajo**: arranque y bring-up baratos (casi todo reúso del S3; el IDF
amortigua) · decidir cuánta RAM lleva la VM en el C3 · un destino AOT nuevo con sus flags ·
y **lo caro, como siempre, el banco**: dos placas × la batería.

##### 🖼️ La pantalla del C6 — evaluado el 26-ago, y DECIDIDO

**Decisión de Eduardo**: el C6 se hace **como las demás imágenes, con LVGL empotrado**.

📐 **Por qué sale barato**: el patrón está probado tres veces y el driver más pequeño
(STM32/LTDC) son **160 líneas**. La cintura de una pantalla nueva es `flush_cb`,
`lv_tick_set_cb`, el buffer parcial y —si hay táctil— un `read_cb`. Todo lo demás (widgets,
layout, eventos) vive en `src/gui.c`, portable.
Y la RAM juega a favor: la DK2 necesita ~750 KB de framebuffer para su panel de 800×480,
pero **por SPI no hace falta framebuffer completo** — se dibuja en un buffer parcial y se
envía por el bus. Con 20 líneas de 240 son ~10 KB. Los costes reales son otros dos: el
FLASH que ocupa LVGL, y que `flush_cb` sobre SPI es más lento que sobre LTDC/DSI (no hay
DMA de escaneo). Ninguno es un muro.

❌ **Lo que se DESCARTA para V6: LVGL como PACK binario** — *«merece un trabajo separado,
con una evaluación previa a ver cómo se puede diseñar, pero eso está fuera de V6»*
(Eduardo). La evaluación que lo aparca, para no repetirla:
- La BIOS tiene **20 ranuras y ninguna de GUI**. El modelo que funciona (SQLite) es de
  llamadas **hacia dentro**: BP pide, el pack calcula, devuelve.
- LVGL necesita lo contrario: llamadas **hacia fuera y en caliente** — `flush_cb` lo llama
  LVGL muchas veces por frame, el `read_cb` del táctil igual, y los eventos vuelven a BP.
  Es un contrato **bidireccional y con estado**, no una tabla de servicios.
- Y `src/gui.c` (1.201 líneas) tendría que irse al pack con él, porque es quien habla con
  LVGL: código hoy COMÚN convertido en un binario por arquitectura — lo contrario del
  camino de V6.
- 📌 Si el objetivo fuera *no pagar LVGL en las imágenes que no lo usan*, **ya existe la
  vía barata**: `BPVM_LVGL`, sin el cual el driver es una unidad de compilación vacía.

#### 📺 P2 — pantallas SPI *(depende de P1)*

**Encargo de Eduardo (23-ago):** *«hay que mirar el soporte de pantallas SPI, pero eso
cuando hayamos añadido los micros ESP32-C6, que tiene una pantalla muy pequeña»*.

📌 El orden lo pone él y tiene sentido: **la placa primero, el driver después**. Hoy la GUI
va por MIPI-DSI (P4) y LTDC (STM32), las dos interfaces paralelas de gama alta; SPI es la
que abre la GUI a las placas pequeñas, y conviene desarrollarla contra el hardware que la
va a usar.

#### ✅ U1 — CERRADO el 23-ago (con dos tareas, no cuatro)

**Quedó en `U1.1` (json_min) y `U1.2` (el log de la Pico), las dos hechas y verificadas en
las cinco imágenes.** `U1.3` y `U1.4` salieron del hito al mirarlas de cerca, cada una por
un motivo distinto — y las dos las paró un criterio que a mi clasificación le faltaba:

- **`U1.3`**: la fusión `.mod`/`.mdn` **borra** ese código. Lo vio Eduardo. Mi criterio
  miraba si el contrato ya existía, no si el trabajo **seguirá existiendo** después.
- **`U1.4`**: mi premisa era falsa. Agrupé dos ficheros por la señal *«no incluye cabecera
  común»*, que detecta divergencia bien pero **no dice qué hace falta para arreglarla**.

📌 **La lección, que vale para U2–U5**: «¿existe ya el contrato?» es un buen detector de
humo y un mal presupuesto. Antes de meter algo en un hito de *«no exige decidir nada»*, hay
que mirar **qué es cada fichero** y **si el trabajo sobrevive** a lo que ya está planeado.

#### 🟢 U1 — lo que no exige decidir nada

Cuatro tareas independientes entre sí. Ninguna necesita diseño: el contrato ya existe o
las copias ya son idénticas.

- **`U1.1` ✅ HECHO (23-ago) · `json_min` al común.** Las tres copias (`pico/`, `esp32/main/`, `stm32/port/`)
  son **byte-idénticas — 0 % de diferencia medido**, y las tres cabeceras declaran las
  mismas seis funciones. Mover a `src/`, borrar las privadas.
  ⚠️ **El trabajo no es mover, es dar de alta**: un `.c` del común va en **cinco** sitios
  (`Makefile`, `pico/CMakeLists.txt`, `esp32/main/CMakeLists.txt`,
  `esp32p4/main/CMakeLists.txt`, y el `.cproject`+`subdir.mk` del STM32). Ver
  [[core-c-nuevo-alta-en-5-builds]].
  ✅ **Hecho y verificado en las CINCO**: `src/json_min.c` + `include/json_min.h`, las tres
  copias privadas borradas, y alta en `Makefile` (el simulador, que metía mano en `pico/`),
  `pico/CMakeLists.txt`, `esp32/main/CMakeLists.txt`, `esp32p4/main/CMakeLists.txt` y el
  STM32 (que regenera su `subdir.mk` solo). Las cinco construyen. `find -name json_min.c`
  devuelve **uno**.
  📌 De paso quedó anotado un dato que el censo no tenía: **la P4 reutiliza SIETE ficheros
  del S3** (`wire_v1`, `esp32_mods`, `repl_esp32`, `board_mgr_esp32`, `platform_esp32`,
  `gpio_esp32` y el propio `json_min`), así que no es un árbol independiente.

- **`U1.2` ✅ HECHO EN CÓDIGO (23-ago) · El log de la Pico al común** *(cierra `#423`)*. Medido: **la Pico es la única
  familia cuyo `CMakeLists` no nombra `src/bpvm_log.c`** — lleva las 15 funciones
  duplicadas en `pico/log.c` (280 líneas). El STM32 (60) y el ESP32 (103) **ya tienen la
  forma correcta**: sólo cintura (`flash_read`, `flash_write`, `now_ms`).
  ⏭️ O sea que no hay que diseñar nada: **hay dos ejemplos de cómo debe quedar**.
  ✅ **Hecho**: `pico/log.c` pasa de **280 a 101 líneas** (sólo cintura: reloj de FreeRTOS,
  lectura por XIP, `erase`+`program` bajo `flash_lock`), `pico/log.h` queda como fachada
  igual que la del STM32, y **`src/bpvm_log.c` entra por fin en `pico/CMakeLists.txt`**.
  ✅ **Verificado en el PC**: el firmware **compila y enlaza** —y eso ya prueba que no
  quedó ninguna función duplicada del núcleo, que el enlazador lo habría cantado— y
  `make test-logsw` sigue en verde (4 de 4).
  ⏳ **Falta la placa**: que el post-mortem siga sobreviviendo al reinicio en la Pico. Es
  la función que no puede romperse y **no se puede comprobar en host**.
  ⚠️ **Un cambio de comportamiento, a propósito**: `log_clear_flash` borraba SÓLO la flash
  y dejaba el log en RAM —así que el siguiente volcado lo devolvía—; el del núcleo vacía
  las dos. Si vacías el log, esperas que se vaya.

- **`U1.3`** ~~El escaneo de `.mdn` al común~~ — ❌ **RETIRADO de U1 el 23-ago. Lo paró
  Eduardo**: *«si vamos a fusionar `.mod` y `.mdn`, deja de tener sentido, ¿no?»*. Y tiene
  razón: la fusión **elimina justo lo que ese módulo hace**, que es *buscar* el `.mdn` —ni
  en el FS por nombre ni en la zona de packs, porque el bloque nativo llega **dentro** del
  módulo que se carga. Unificar tres bucles para borrarlos después es trabajo perdido.
  📌 **De paso corrige al censo, que dijo menos de lo que pasa.** No es «el STM32 diverge»:
  la cabecera de `bpvm_mdn_scan.h` lo deja escrito desde el 10-ago — *«sólo el Pico llama a
  esto; las otras TRES familias conservan su bucle propio, INTACTO»*. Son tres, y estaba
  documentado.
  ⏭️ **Lo que SÍ sobrevive a la fusión** y hay que llevarse a su hito: la **cintura** de la
  RAM ejecutable cuando hay que copiar el código —arena en Pico y STM32,
  `heap_caps_malloc(MALLOC_CAP_EXEC)` en el ESP32—. Ya está parametrizada como callback, o
  sea que no es trabajo de unificación: es parte del diseño de la fusión.
  ⚠️ **El riesgo de aplazarlo, dicho para que no sorprenda**: mientras la fusión no llegue,
  una mejora del AOT aterriza en una familia y no en las otras tres. Es tolerable porque el
  módulo común ya existe y la migración es *«cambiar su bucle por una llamada, no
  reescribirlo»* — pero deja de serlo si la fusión se aplaza mucho.

- **`U1.4`** ~~Flash y particiones al contrato~~ — ❌ **SACADO de U1 el 23-ago: su premisa
  era falsa.** Yo dije *«cuelgan de `bpvm_part.h`, que ya existe»*. Medido: `bpvm_part.h`
  es el **mapa** de particiones (nombres, layout, tamaños) y **no hace E/S de flash**. Y
  **no existe ninguna cabecera común de flash**. Los dos ficheros tampoco son lo mismo:
  `flash_lock.c` (40) es la **ventana exclusiva de XIP** del RP2350 —concurrencia— y
  `stm32_flash.c` (56) son las **primitivas de borrado/escritura** del U5.

  📐 **Lo que SÍ hay, medido:** cada familia cablea `erase`/`program` para **cuatro
  consumidores** —packs, env, FS y log— y ya existen **dos cinturas distintas** para lo
  mismo: `bpvm_pack_flash_t` (`erase`/`program`, offsets **relativos** a la región) y
  `bpvm_log_cintura_t` (`flash_read`/`flash_write` de la región **entera**). Entre las tres
  familias son ~12 cableados de las mismas dos operaciones.

  ⏭️ **El trabajo real es UNA cintura de flash por familia** usada por los cuatro. Y es
  **diseño**, no mecánica: hay que decidir offsets relativos o absolutos, si la lectura
  entra en el contrato, y quién toma el candado. Buena noticia: `bpvm_pack_flash_t` ya es
  genérica —no tiene nada de packs salvo el nombre— así que probablemente sea ascenderla,
  no inventarla.

  ✅ **Y una sospecha mía que resultó infundada, comprobada antes de fichar**: miré si
  alguna de esas escrituras se saltaba `flash_lock` en la Pico. **No**: packs, env y FS lo
  toman los tres. No hay bug latente; falta la abstracción, nada más.

#### 🔎 Lo que salió al hacer U1.1 — la configuración RELEASE del STM32 está abandonada

Encontrado el 23-ago al intentar compilar las cinco imágenes. **No bloquea nada y no se ha
tocado**, pero conviene saberlo antes de que muerda.

El proyecto de la Discovery tiene dos configuraciones, y **la que se publica es `Debug`**:
su `text` son **887.604 B** y el `bpvm_stm32_dk2.bin` publicado son **888.268 B** — cuadran.

🔴 **`Release` no serviría**, y por tres motivos a la vez:
- le faltan cuatro `-D` que `Debug` sí tiene: **`BPVM_BOARD_DK2`**, `BPVM_GUI`, `BPVM_LVGL`
  y `LV_CONF_INCLUDE_SIMPLE`. Sin el primero, `board.h` **cae a la rama de la Nucleo** y
  pide `stm32u5xx_nucleo.h`;
- le faltan dos rutas de include (`third_party/littlefs` y `third_party/lvgl`);
- excluye **dos** ficheros host-only donde `Debug` excluye **cinco** (`fs_host.c`,
  `fs_lfs_host.c` y `fs_fat.c` se le cuelan, y no compilan para ARM).

⚠️ **La trampa es el NOMBRE**: quien vaya a hacer «la de release» construye, sin saberlo,
una Nucleo sin GUI — o no construye. Me pasó a mí hoy, y perdí un rato creyendo que había
roto algo.
📌 **Y corrige al censo**: `CENSO_FAMILIAS.md` daba el STM32 como `-Os` citando el
`subdir.mk`, y anotaba que lo de los ficheros host-only estaba *«RESUELTO: su `.cproject`
SÍ los excluye»*. Las dos cosas son ciertas **de `Debug`**, que es de donde salía el dato —
y falsas de `Release`. El instrumento decía la verdad de una configuración y se leyó como
si hablara del proyecto.
✅ **DECIDIDO Y HECHO (23-ago). Eduardo: *«de momento borra release»***. Quitada del
`.cproject` de la Discovery —los tres sitios: el `<cconfiguration>`, su
`scannerConfigBuildInfo` y su entrada de `refreshScope`— más el directorio `Release/` que
quedaba huérfano (2,4 MB, no versionado). Verificado después: el proyecto **importa y
construye igual**, 0 errores y 0 avisos, con el mismo `text` de 887.604 B.

✅ **Y la Nucleo, igual (Eduardo, mismo día: *«quita también release de la Nucleo»*).** La
suya estaba aún peor: **ninguna** ruta de include y **ninguna** exclusión de ficheros
host-only, donde `Debug` tiene cuatro de cada. Ni siquiera existía su directorio `Release/`
— **nadie la había construido nunca**, que es la prueba más limpia de que sobraba.
Verificado igual: construye 0 errores, y `text`+`data` = 247.444 B frente a los 247.448 del
`bpvm_stm32_nucleo.bin` publicado.

📌 **Así que las dos placas STM32 tienen ya UNA sola configuración, y es la que se usa.**
Deja de existir la trampa del nombre. Lo que sí queda pendiente, y es harina de otro costal:
esa única configuración se llama `Debug` aunque compile a `-Os` y sea la que se publica.

#### 🟡 U2 — el transporte

> ⚠️ **Criterio de Eduardo (24-ago), y manda sobre el resto del hito**: *«las comunicaciones
> son nuestro cordón umbilical entre el PC y el micro, conviene ir con prudencia. Los
> cambios tienen que hacerse en pasitos pequeños; en vez de 1 gran cambio, mejor 3 o 4
> pequeños.»*

##### ✅ `U2.1` HECHO (24-ago) — y los dos enunciados de este hito eran FALSOS

Decía: *«`pico/wire_v1.c` y `esp32/main/wire_v1.c` se llaman igual, dicen ser la misma
versión del protocolo, y **el 100 % de sus líneas difieren**. No es una copia divergida:
son dos programas distintos con el mismo nombre.»*

**Medido, y es justo al revés.** Las tres familias que comparten la API (`pico`, `esp32/S3`,
`esp32p4`) exponen **las mismas 15 funciones `wire_v1_*`**, y comparadas *palabra por
palabra*:

| | funciones | líneas |
|---|---|---|
| **idénticas en las tres** | **11** | 88 (× 3 copias = **264** donde bastan 88) |
| distintas de verdad | 4 | 47 |

Y las cuatro que difieren son **exactamente** las cuatro que tocan el cable:
`recv_line`, `recv_bulk`, `send_line`, `send_bulk`. Las otras once —`msg_begin`,
`msg_begin_event`, `msg_end`, los cuatro `field_*`, `send_cstr`, `send_error`,
`send_fatal`, `send_reply_empty`— son **construcción de JSON, sin una línea de hardware**.

📌 **O sea que la costura ya existe y está limpia**: el fichero mezcla dos capas, protocolo
y transporte, y la frontera cae en un sitio exacto. No hay nada que adivinar.

📌 **Y los propios ficheros lo dicen**, sólo que nadie lo había leído junto: *«Misma API que
pico/wire_v1.h»* (S3), *«MISMA API (wire_v1.h), de modo que el dispatcher `repl_esp32.c` se
reutiliza TAL CUAL»* (P4). El P4 reutilizando el REPL del S3 sin tocarlo **es la prueba** de
que la API es común de verdad.

⚠️ **La mina, que sigue ahí**: los tres ficheros llevan escrito *«los builders JSON son
COPIA… si cambia el formato del protocolo, mantener las tres copias en sync»*. Hoy están en
sync —las once son idénticas—, o sea que alguien lo ha hecho a mano y le ha salido bien.
**Un comentario no es un mecanismo**: es [[arreglo-que-no-viaja-entre-familias]] esperando
turno.

##### ❌ `U2.2` — su premisa también era falsa, y de otra manera

Decía que los cuatro transportes *«no incluyen `bpvm_comm.h`, que existe y es común»*.
Medido: `bpvm_comm.h` **no es el contrato de los transportes**. Declara tres funciones
(`bpvm_comm_start` / `_stop` / `_output_enqueue`) que son el contrato **VM↔comunicaciones**,
y las implementan **dos de las cinco imágenes**: `src/comm_host.c` y `pico/comm_pico.c`. El
S3, la P4 y el STM32 **no las implementan en absoluto**.

Así que añadir un `#include` ahí no unifica nada. Es el mismo error que paró `U1.4`, y con
la lección ya escrita: *«¿existe ya el contrato?» es un buen detector de humo y un mal
presupuesto*.

##### 🧭 Lo que SÍ hay que hacer, en pasitos

1. ✅ **HECHO Y VERIFICADO EN PLACA (24-ago) · `bb530e3f`** — las 11 + sus 4 helpers a
   `src/wire_v1_proto.c`, con `include/bpvm_wire_v1.h` nombrando las dos capas.
   **Sólo la Pico**: el S3 y la P4 conservan su copia hasta tenerlas en el banco.
   `pico/wire_v1.c` 259 → 108 líneas; su cabecera 133 → 42.

   🎯 **La red fue más fuerte de lo previsto, y conviene repetir el gesto**: antes de
   flashear se comparó la imagen nueva con la vieja y salió que las **15 funciones generan
   código máquina IDÉNTICO**, que hay **1984 símbolos en las dos, el mismo conjunto y todos
   del mismo tamaño**, y que **`.text` es idéntico byte a byte** (303.148). Los 1494 bytes
   que difieren en el `.uf2` (0,25 %) son pura recolocación. Con eso la prueba en placa
   **confirma en vez de descubrir**.
   📌 Por eso el bloque se movió TAL CUAL, sin reescribir una línea: si se hubiera
   "mejorado" de paso, esa comparación no existiría.

   ✅ **En placa (Eduardo)**: el IDE conecta y funciona todo, **incluida la subida de un
   Pack** — que es la prueba fuerte, porque el bulk es binario y ahí un fallo de framing no
   da error, corrompe datos. Ejercita las dos capas: `field_bulk` (común) y
   `send_bulk`/`recv_bulk` (las que se quedan en la familia).

2. ✅ **HECHO Y VERIFICADO EN LA P4 (25-ago) · `f7510d85`** — borradas las dos copias
   (`esp32/main/wire_v1.c` 252→124, `esp32p4/main/wire_v1_tcp.c` 321→192) y alta de
   `src/wire_v1_proto.c` en sus builds. Los 11 builders viven ya en UN sitio para las tres
   familias con wire; la frase que llevaban dentro —*«mantener las tres copias en sync»*—
   ya no describe nada.

   🎯 **Mismo gesto que en la Pico, antes de flashear**: binario del P4 **exactamente igual**
   (`0x13ccc0`), **6713 símbolos con los mismos tamaños salvo uno** — `wire_task_uart`,
   0xc4→0xc2. Y ese uno explicado, no supuesto: desensamblado son **66 instrucciones, las
   mismas y en el mismo orden**; contando codificaciones, 32 comprimidas + 33 de 4 bytes
   pasan a 33 + 32. Una instrucción se comprimió por **relajación del enlazador** al quedar
   su destino más cerca. Cero diferencia semántica.

   ✅ **En placa**: `INFO`, `ls` y `RUN` (los 11 builders construyendo JSON sobre RISC-V), y
   la prueba fuerte — **subida de un Pack de 581.632 B por el camino BULK**, que es binario
   y donde un fallo de framing corrompe en vez de fallar. De paso confirmó la **poda por
   familia** (2 entradas ARMV8 descartadas, 1.130.496 → 581.632 B) y la relocalización del
   motor de SQLite en 5.087 sitios.

   ⚠️ **Aviso de método que este paso deja**: comparar `.text` byte a byte **no vale en
   ESP-IDF** — el código no vive en `.text` y `objcopy --only-section=.text` saca DOS
   FICHEROS VACÍOS, que comparados dan «idéntico». Las secciones son `.flash.text` e
   `.iram0.text`, y ahí la comparación byte a byte tampoco dice nada útil (una función
   encoge y todo lo de detrás se desplaza). **Lo que vale es por símbolo.**

3. ✅ **HECHO Y VERIFICADO EN PLACA (26-ago) · `ba70c9ab`** — el STM32 deja de ser la
   cuarta forma. Las 4 de cable a los nombres del contrato (`recv_bulk` gana el chequeo de
   capacidad que no hacía), `send_error`/`send_cstr` borrados en favor del común —**y con
   ello un bug latente fuera**: el `send_error` propio metía `message` SIN ESCAPAR, una
   comilla rompía el framing—, `send_fatal` como wrapper de 1 línea (LED de la placa + el
   JSON del común), 57 llamantes renombrados.
   📌 `wire_v1_proto.o` **ya se compilaba** en este build (linked folder): el linker lo
   descartaba por no tener llamantes. La unificación fue darle llamantes.
   ✅ **En placa (Eduardo)**: varios programas subidos y ejecutados (PUT = `recv_bulk` con
   la firma nueva; la salida por `send_line` del contrato), un módulo cargado y un **Pack
   grabado** — el camino bulk a lo grande, `s_burn_chunk` incluido.
   ⏭️ **Se queda para U3, a propósito**: los ~22 replies que el REPL arma a mano con
   `snprintf` + `json_escape` (INFO, LIST…). Son del dispatcher, no del cable.
4. ✅ **RESUELTO (26-ago) — y la premisa era FALSA por segunda vez.** Medido de punta a
   punta: `bpvm_comm.h` es el contrato de salida del **modo SMP**, que es *opt-in* (host
   `--smp=N`; Pico `-DBPVM_PICO_SMP_WORKERS`, que el build por defecto NO define). En
   single-worker —todas las placas hoy— la salida va por `output_cb` y este contrato ni se
   toca. Y **las cinco imágenes SÍ lo implementan**: la Pico con `comm_pico.c` y las otras
   cuatro con `comm_host.c` — tres de ellas **como relleno deliberado** (S3 y P4 lo listan
   explícito en su CMakeLists; el STM32 lo arrastra por el linked folder).
   Lo dicho antes aquí («tres familias sin implementar un contrato que existe») medía
   *implementaciones propias* y concluía *implementaciones*. La cadena entera de este hito
   nació de leer mal este fichero — dos veces.
   **La acción**: el MAPA escrito en `bpvm_comm.h` (quién, cuándo corre, y el aviso de que
   un SMP futuro en S3/P4/STM32 debe traer su `comm_<familia>.c` que hable por el wire, no
   heredar el relleno de stdout), y el relleno anotado en los dos CMakeLists. Cero cambio
   de comportamiento; sin reflashear.

##### 🏁 U2, EL TRANSPORTE: CERRADO (26-ago)

Los 11 builders del protocolo en UN fichero para las cuatro familias con wire · el cable
reducido a 4 funciones por familia · el STM32 dentro del contrato (y su `send_error` sin
escapar, muerto) · `bpvm_comm.h` con su mapa. **Verificado en placa en Pico, P4 y STM32**;
el S3 comparte los ficheros del P4. Los ~22 replies a mano del REPL del STM32 → U3.

##### 📐 De propina, una lección de método que costó dos medidas

El enunciado falso («100 % difieren») salió de un diff **por líneas**. Al remedir yo caí en
lo mismo: mi primer script dijo *«0 de 15 idénticas»* — porque comparaba líneas sobre código
**reformateado** (`char* buf` vs `char *buf`, el ajuste de línea de un parámetro), y encima
mi extractor cortaba mal las funciones al contar la llave de dentro del literal `'}'`.
Comparando palabra por palabra salieron 11. **Un diff por líneas sobre C reformateado miente
hacia el lado peor**: dice que hay que investigar donde no hay nada que investigar.

#### 🔴 U3 — el REPL

El 80 % del problema y el 100 % de las asimetrías que nos han mordido (`SD_INFO` sólo en
la Pico, el `INFO` del STM32 incompleto, el aviso de `/lib`, el `preinstall`).

##### ✅ `U3.0` (26-ago) — LA MATRIZ VERBO × FAMILIA, medida

El enunciado de este hito («no tiene contrato») también exageraba hacia el lado malo:
hay un **núcleo de 20 verbos idénticos en las tres**, la Pico es el superconjunto (29),
y el protocolo (`BPVM_WIRE_PROTOCOL.md`) ya documenta la mayoría. Medido con
`strcmp(type, …)` sobre los tres dispatchers:

| faltan en | verbos |
|---|---|
| **esp32 (S3 y P4)** | `FORMAT`, `RENAME`, `RMDIR`, `SD_INFO`, `SD_MOUNT`, `BOOTSEL`* |
| **stm32** | `LIST_DIR`†, `PROMPT_RESPONSE`, `SAVE`, `RENAME`, `RMDIR`, `SD_*`†, `BOOTSEL`* |

\* `BOOTSEL` es hardware del RP2350: exclusión legítima. † Ya fichadas.

##### 🔬 `U3.0b` (26-ago) — Y LA SEGUNDA PASADA, a pregunta de Eduardo

> *«Si añadimos un comando RENAME y detrás no hay un comando que lo implemente de verdad,
> no va a servir de nada. Hay que ver si esos comandos están implementados o no.»*

Tenía razón, y más de lo esperado: la matriz medía el **dispatcher** (`strcmp`), no lo que
hay debajo — y **hasta la Pico tiene verbos-fachada**. Verbo ausente × primitiva real:

| verbo | le falta a | la primitiva debajo | veredicto al compartir dispatcher |
|---|---|---|---|
| `RENAME` | S3/P4, STM32 | `bpvm_fs_rename` — **fachada común**, `lfs_rename` del motor común | ✅ **gratis DE VERDAD** |
| `FORMAT` | S3/P4 | `fs_format_ram` **EXISTE** en `fs_lfs_esp32.c` | ✅ gratis (nombre de familia → va al `ops`) |
| `SAVE` | STM32 | littlefs persiste en cada close (`fs_save` no-op **documentado**) | ✅ gratis como no-op LEGÍTIMO vía `ops` |
| `RMDIR` | S3/P4, STM32 | **ninguna**: el de la Pico ES un stub que contesta OK sin hacer nada (v1: dirs = prefijos) | ⚠️ se hereda el stub — coherente, pero que conste |
| `PROMPT_RESPONSE` | STM32 | **ninguna en NINGUNA familia**: `IO.prompt()` no está implementado en la VM-C; la Pico hace ack cortés | 🔴 letra muerta — heredar = heredar el stub |
| `SD_INFO`/`SD_MOUNT` | S3/P4, STM32 | `bpvm_sd` común, cintura RP2350 (SPI) y ESP32 (SDIO); **STM32 sin cintura** | S3/P4 probablemente gratis; STM32 debe **fallar con mensaje**, no faltar |
| `BOOTSEL` | S3/P4, STM32 | hardware del RP2350 | ✅ verbo de familia (`ops`): correcto que falte |

🔴 **Hallazgo colateral con entidad propia — y AMPLIADO el 26-ago**: **`input()` /
`IO.prompt()` no existe en la VM-C**, ni en placa ni en host (`repl_v1.c:2005`: *«nunca
emitimos PROMPT_REQUEST»*).

📐 **Lo que se creía el 25-ago**: una tubería a medio construir. **Lo que es de verdad**,
tras el apunte de Eduardo (*«en su día sí que lo probamos, se abría una ventana; eso es de
V1 o V2»*) y comprobarlo en el histórico: **funciona en miVM desde antes del wire v1** —
`setPromptSender` / `deliverPromptResponse` ya estaban, y en `5182e396` (25-may) sólo se
MIGRABAN de `promptRequest` a `PROMPT_REQUEST`. El IDE recibe y contesta; los tres REPL
aceptan la respuesta.

| | ¿lo tiene? |
|---|---|
| miVM | ✅ completo, desde V1/V2 |
| IDE (la ventana que Eduardo recuerda) | ✅ |
| protocolo + los tres REPL | ✅ |
| **VM-C** | ❌ **nunca** |

⚠️ O sea que **no es una feature a medias: es una divergencia limpia del invariante
sagrado**, y de las que la paridad **no puede cazar por construcción** — el arnés compara
el `stdout` de dos ejecuciones no interactivas, y un programa que espera respuesta no cabe
en ese corpus. Lleva así desde V1.

📌 Encaja con el patrón que V6 lleva toda la semana destapando: algo que se da por bueno
porque **el instrumento que lo vigilaría no llega hasta ahí**. Antes de implementarlo hay
una pregunta de diseño que contestar (Eduardo, 26-ago, sin decidir): **¿lo queremos?** Un
`input()` bloqueante en un micro sin consola es discutible —lo natural allí es la GUI o el
wire—, y «BasicPlus no tiene entrada por consola, tiene formularios» sería una decisión
legítima. Lo que no vale es el estado de hoy: **funcionando en una VM y no en la otra, sin
decidir**.

##### ✅ `U3.1` — el contrato + GRUPO 1, verificado en placa (26-ago · `0285fb6c`)

`include/bpvm_repl.h` (el contrato: la familia pregunta primero al común) y
`src/bpvm_repl.c` con el grupo 1 —PING, TIME, LOG_DUMP, LOG_CLEAR—, todo sobre contratos ya
comunes (wire U2, log U1.2, RTC, json U1.1). El STM32 delega y borró sus cuatro copias.
Alta en los CUATRO firmwares; en el host CLI **no va a propósito** (es capa-wire, el
Makefile dice por qué). Tres divergencias unificadas y anotadas en el header (TIME sin
parámetro ahora avisa; la marca del CLEAR queda siempre; el sink del STM32 perdía chunks).

✅ **En placa (Nucleo, 26-ago)**: conexión (PING), LOG_DUMP entero por el sink nuevo, y la
prueba visible — la marca `LOG cleared via wire v1` **apareció en el log del STM32 por
primera vez**: esa línea sólo puede venir del común.

##### ✅ `U3.2` — GRUPO 2: el FS por la fachada común (26-ago · `778e6361`)

Seis verbos más al común — DEL, MKDIR, RMDIR, STAT, RENAME, GET — todos resueltos enteros
con `bpvm_fs_*`. El STM32 borra sus copias y **gana dos verbos que nunca tuvo**: RENAME con
primitiva real (fachada → `lfs_rename`, atómico) y RMDIR (el stub v1, a sabiendas). Su STAT
aprende el `crc` bajo demanda (#398): el «contenido idéntico, salto PUT» del IDE se apoya
ya en algo real en esta placa. Divergencias unificadas: códigos de error a la referencia, y
el `fs_save()` tras DEL (no-op documentado) fuera.

✅ **En placa (Nucleo, 26-ago), el grupo COMPLETO**: Delete (DEL común), doble subida con
`salto PUT` (STAT+crc común), Download (GET común, bulk por trozos), PUT de familia y RUN
de control con su AOT 4/4 intacto.

**El marcador de U3**: 10 verbos en el común (meta 4 + FS 6) de los 29 del protocolo. En el
STM32 quedan de familia: HELLO, INFO, LIST, DF, FORMAT, PUT*, RUN, KILL, RESET, STATE — los
de `ops` y los de estado.

##### ✅ `U3.3` — LIST al común, verificado en placa (26-ago · `ecd08f73`)

El verbo del episodio del `/lib`, en streaming sobre la fachada (que inyecta los montajes
hijos: `/sd` entra solo). El STM32 borra su `handle_list` y gana lo que #398 nunca le
llevó: **su LIST viejo calculaba el CRC de CADA fichero en CADA refresco del árbol** — el
común emite `crc:-1` y el CRC va por `STAT{crc}` bajo demanda (grupo 2). El refresco pasa
de leer el FS entero a no leer nada. Y sin CRC anidado, fuera el snapshot por directorio:
streaming puro, con el único tope (directorios pendientes) DECLARADO (`omitted` + log).

✅ **En placa (Nucleo)**: Refresh con el árbol correcto y la línea nueva `ls: N ent` en el
log; Delete y Download de control sobre lo listado (los nombres que emite LIST son los que
DEL/GET consumen, y quedó escrito).

**Marcador U3: 11 de 29 verbos en el común** (meta 4 + FS 7). Quedan en familia: HELLO,
INFO, LIST_DIR, DF, FORMAT, SAVE, PUT*, RUN, KILL, RESET, STATE, DEBUG* y los de hardware.

##### ✅ `U3.4` — LA CINTURA: INFO, DF, FORMAT, SAVE (26-ago · `f2425cb9`)

El primer grupo que **no** se resuelve solo con contratos comunes. Se midió antes de
diseñar: comparando el INFO de las tres, **18 campos son comunes y 11 sólo del RP2350**
(variante, packs/XIP, los cinco de SQLite, `floatAbi`, dos del RTOS). De ahí la forma de
`bpvm_repl_ops_t`: los comunes como **valores** en una struct que la familia rellena (no 18
punteros a función), lo propio por **un** gancho (`info_extra`), y punteros sólo para las
ACCIONES (`fs_format`, `fs_save`) y los contadores del FS, que aún no tienen fachada.

Tres mejoras que no son de copiar:
- **INFO tiene la misma forma y el mismo orden en las tres.** Antes cada una lo armaba con
  su `snprintf` — así fue como el STM32 acabó mandando 7 campos y uno con otro nombre.
- Sin cintura registrada, o sin `fs_format`: **UNSUPPORTED con nombre y motivo**, no el
  «type no implementado» genérico. Un port a medias lo dice en vez de parecer roto.
- `SAVE` sin `fs_save` contesta OK **y es la verdad** (littlefs persiste al cerrar), no un
  no-op disfrazado.

✅ **En placa (Nucleo)**: el diálogo INFO completo y con los valores propios en su sitio
(114 GPIO del LQFP144, 28 salidas PWM, 20 canales ADC, flash leída del registro, reparto de
la VM 384+128 KB). La documentación del STM32 sobre **de dónde sale cada valor** se
conservó entera: es lo único realmente propio de esa familia.

**Marcador U3: 15 de 29 verbos en el común.** Quedan: HELLO, STATE, LIST_DIR, PUT*, RUN,
KILL, RESET, DEBUG* y los de hardware (BOOTSEL, SD_*).

##### 📖 El episodio del `/lib` desaparecido (26-ago) — y las DOS fichas de E1 que mordieron

Tras flashear, el árbol del IDE mostraba `/app` con 3 ficheros y **ningún `/lib`** — con
90 KB usados en el FS. La cadena, medida pieza a pieza y confirmada por Eduardo:

1. El flasheo **borró `/lib`** del littlefs.
2. La **Stdlib está grabada como PACK** en esta placa, y `bpvm_pack.c:126` registra la zona
   como *fallback* de la fachada — **con `stat` y `read`, y NULL en `list`**.
3. El instalador *si-ausente* pregunta por `stat` → el pack contesta «existe» → **no
   repone**. Correcto: ¿para qué duplicar lo que el pack sirve por XIP?
4. El RUN resuelve `Core` por el fallback → todo funciona.
5. **LIST no tiene fallback** → el árbol miente por omisión. Y el IDE, comparando contra
   LIST, subió un `Pico.mod` que el pack ya servía.

📌 **Nada estaba roto en lo funcional: lo roto es la VISIBILIDAD.** Son exactamente dos
fichas de E1 ya escritas, mordiendo a la vez: **«vista de packs en el árbol»** y **«no
copiar deps que el device YA TIENE — y que lo diga él»**. Este episodio es su caso de
prueba natural cuando se hagan.

⚠️ Dos huecos de instrumento anotados de paso: el instalador es MUDO (`fs_put` sin
comprobar ni log — debería decir «N instalados, M ya servidos por el pack») y el boot del
STM32 no dice de dónde salió la tabla de particiones (el TOTAL del FS bailó 516096 ↔
614400 entre arranques del mismo día y aún no está explicado).

📐 **Lo que esto fija del diseño**: heredar el dispatcher común da funcionalidad REAL en
`RENAME`/`FORMAT`/`SAVE`, semántica-v1 coherente en `RMDIR`, y NO resucita `PROMPT` (eso
pide implementar `IO.prompt` en la VM-C, que es otra ficha). Los verbos de hardware
(`BOOTSEL`, `SD_*`) viven en el `ops` de familia, y donde no haya soporte la respuesta es
un error con nombre, nunca «type no implementado».

- **`U3.1` · Escribir `bpvm_repl.h`.** Hoy **no existe** como fichero, pero U3.0 enseña
  que el contrato de facto sí: 20 verbos comunes + el protocolo escrito. El `.h` es
  ponerle nombre a lo que ya converge.
- **`U3.2` · Partir en dos.** El transporte **sí** es hardware; interpretar `RUN`, `DIR`,
  `INFO` o `PACK_BURN` **no**. Depende de U2.
- **`U3.3` · Migrar familia a familia**, de la que menos tiene a la que más:
  `stm32_repl.c` 920 → `repl_esp32.c` 1344 → `repl_v1.c` 2054.
  ✅ **Se comprueba, y es el hito con más red**: el IDE hace lo mismo contra cada placa,
  y `LIST_DIR`/`INFO`/`PACK_LS` devuelven lo mismo que antes.

#### 🟡 U4 — la stdlib embebida

16 ficheros `*_mod.c` en la Pico frente a **uno** en ESP32 (4.845 líneas) y STM32 (4.837).
No es código distinto: es el mismo dato empaquetado de dos maneras.
⚠️ **Es un GENERADO.** Se toca el generador, nunca el resultado — ver
[[generado-parcheado-a-mano]].

#### 🟡 U5 — la tabla de handles

Hoy no tiene fichero ni cabecera: vive repartida por `bpvm.c`, `builtins.c`,
`bpvm_aot_helpers.c`, `bpvm_dbg_wire.c` y `bpvm_util.c`. Está *unificada por omisión*, no
por diseño, y por eso `#432` (dónde debe vivir y de qué tamaño) no se puede ni plantear.

---

### ✅ DECIDIDAS el 23-ago (Eduardo) — entran en V6

- **📁 [FS] `/sd` pasa a ser un PREFIJO RESERVADO.** Decisión de Eduardo: *«en Linux hay
  una carpeta `/dev` donde están los dispositivos y otra donde se montan (`/mnt`). Aquí
  quizás deberíamos reservar `/sd` para que no haya problemas.»*
  🔬 **El mecanismo de hoy, medido en `src/fs_facade.c`**: la tabla de montajes tiene la
  entrada 0 como backend **raíz con prefijo `""`**, y `route()` elige el prefijo
  coincidente más largo. Sin tarjeta, `/sd/x` no coincide con ningún montaje y **cae en la
  raíz**, que actúa de comodín. Por eso escribir en `/sd` con la tarjeta fuera no falla:
  va a la flash interna en silencio.
  ⏭️ **Lo que hay que hacer**: que `/sd` exista en la tabla **aunque no haya nada montado**,
  como punto de montaje reservado, y que `route()` devuelva error en vez de la raíz.
  📌 **Y el matiz que hace buena la idea**: Linux tiene exactamente esta trampa —escribir
  en `/mnt/x` sin montar escribe en el disco de abajo—, y es una molestia clásica. Lo que
  propone Eduardo **no es copiar Linux: es arreglar lo que Linux hace mal**, aprovechando
  que aquí el conjunto de prefijos es finito y conocido.
  ⚠️ **Es un cambio de comportamiento.** Un programa que hoy funciona apoyándose en el
  respaldo dejará de hacerlo — y eso es lo que se busca, pero hay que decirlo en las notas
  de versión. Nos mordió el 21-ago: sin guarda, `SdCard.bp` *parecía colgarse*.

- **🗺️ [lenguaje] `Map`: objetos internos para claves Y valores, más sobrecargas de
  `add`.** Decisión de Eduardo, cerrando la cola que dejaron los captadores de `List` en
  V5: *«objetos internos tanto para las keys como para los values. También sobrecargas
  para `add(key, value)`; aquí podemos hacerlo para los casos más habituales: key integer
  y key string.»*
  📐 **Lo que fija la decisión**: `Map` no hereda los captadores de `Core.List` (`SyncList`
  y `OwnerList` sí, por extenderla), así que hay que dárselos — y **a las dos mitades**,
  no sólo a los valores, que era la duda que quedaba abierta.
  ⏭️ **El alcance está acotado a propósito**: las sobrecargas de `add` se hacen para
  **clave entera y clave cadena**, que son los casos habituales, no para el producto
  cartesiano de todos los tipos. Encaja con [[base-finita-lo-demas-en-packs]] — la
  pregunta no es «¿es útil?» sino «¿lo paga todo el mundo?».
  📌 Sigue la regla de la casa para el azúcar: cuelga de algo que ya existe
  ([[no-gastar-palabras-reservadas]]), y las conversiones van en los envoltorios, como se
  hizo en `List`.

---

### 🔜 Aplazadas a V6 durante el desarrollo de V5

- **🔴 [V6, OBLIGATORIO] la pasada de INTERFAZ no resuelve `Core` implícito** — encargo
  explícito de Eduardo (22-ago): *«de momento hacemos 1 para salir del paso, pero en V6
  esto tiene que estar solucionado definitivamente»*.
  📐 **El hecho**, medido al recorrer el checklist de publicación: un módulo que expone un
  tipo de la stdlib en una **firma pública** sin importar `Core` compila su cuerpo pero
  **pierde el miembro en la interfaz**:
  ```
  -- omitidas en interfaz (1): class Dao.method list: retorno tipo no exportable: <error>
  ```
  Y el error de verdad aparece **en el consumidor**, lejos de la causa:
  `'Dao' no tiene miembro de instancia 'list'`.
  🔍 **La asimetría es entre las DOS PASADAS**: la completa resuelve `Core` implícito —por
  eso `ListGets.bp` usa `List` sin importar nada y corre en las seis placas— y la de
  interfaz no.
  📅 **Desde cuándo**: el 18-ago, con `#450` («el compilador deja de sintetizar List,
  SyncList y OwnerList»). Antes `List` la fabricaba el compilador y existía en todas
  partes; ahora viene de `Core.bp` como cualquier clase. El cambio es correcto; lo que
  faltó fue que la pasada de interfaz lo acompañara.
  ✅ **Alcance real, comprobado — por eso NO bloqueó V5**: la stdlib está limpia
  (`Str.bp` importa `Core` desde `#446`, y el `Orm` lo recibe de ahí; ningún módulo expone
  un tipo de `Core` sin importarlo). Sólo afecta a un módulo **de usuario** que exponga
  tipos de stdlib sin importar nada que arrastre `Core`. Y el compilador **avisa en el
  sitio correcto**, aunque el error salga en otro.
  ⏭️ **Lo que hay que hacer en V6**: que la pasada de interfaz resuelva los tipos
  implícitos igual que la completa. Emparenta con el otro bug de esa misma pasada —el de
  LSP entre interfaces de módulo, `appv1lsp`/`appv2`— que también sale de que
  `INTERFACE_ONLY` ve menos que la pasada entera. **Son la misma raíz y conviene
  arreglarlos juntos.**

- **🧮 [V6] ¿CUÁNTO cuesta una familia nueva (ESP32-C3 / C6) si antes unificamos?** —
  pregunta de Eduardo (22-ago): *«si tenemos en cuenta que el IDF es el mismo para todas
  las familias ESP32, en realidad sale muy poco código: casi todo lo hecho para el P4
  debería servir. Así que meter una familia nueva será el boot y sobre todo trabajo de
  pruebas.»*
  ✅ **Ya hay un experimento hecho que lo contesta: el P4**, que fue la última familia
  añadida. **Reutiliza OCHO ficheros del S3** —incluido el `repl_esp32.c` de 69 KB, el
  `board_mgr`, `platform`, `fs_lfs`, los blobs, el log, `gpio` y `json_min`— y sólo tiene
  **2.558 líneas propias**. Pero lo que decide la respuesta es **en qué** se le van:

  | fichero propio del P4 | líneas | ¿lo necesita un C3/C6? |
  |---|---|---|
  | `gui_display_dsi.c` | 713 | ❌ no tienen pantalla |
  | `main.c` (**el boot**) | 623 | ✅ sí |
  | `pack_p4.c` | 340 | 🔧 lo unifica V6 → un gancho |
  | `wire_v1_tcp.c` | 321 | ❌ irían por UART, como el S3 |
  | `blk_sdmmc_p4.c` | 249 | ❌ normalmente no llevan SD |
  | `bios_p4.c` | 141 | ✅ sí |
  | `p4_board_id.c` | 91 | ✅ sí |
  | `aot_Bench.c` / `aot_funcs_p4.c` | 80 | ✅ el registro (31); el resto es un sample |

  📊 **Cuenta**: **1.283 líneas no aplican** (pantalla + Ethernet + SD). Lo realmente
  necesario son **≈890 líneas**, y **el grueso es `main.c`: el BOOT** — exactamente lo que
  Eduardo predijo. Con la partición en dos boots, la mayor parte de esas 623 se va al común
  y quedan las decenas del arranque de silicio.
  🎁 **Y un regalo que no estaba contado: el C3 y el C6 son RISC-V, como el P4.** Toda la
  cadena AOT de `H4` —`.mdn`, `.npk`, relocalización, `-mcmodel=medany`— **ya funciona para
  ellos**. Nacen con aceleración el primer día, sin escribir una línea de AOT.
  🎯 **Conclusión, que es la de Eduardo con números detrás**: el código de una familia ESP32
  nueva es **el boot y poco más**. El coste real se desplaza a **las pruebas** — y por eso
  el simulador con disfraces y la batería multi-placa dejan de ser comodidad para ser *la*
  inversión que hace sostenible añadir placas.

- **📈 [V6] «Unificar no es una opción, es el único camino» — la tesis económica, medida**
  — cierre de Eduardo (22-ago): *«la parte del compilador es muy pequeña; añadiendo la VM
  crece, pero en el total sigue siendo pequeña. ¿En qué se nos va el tiempo? Cada vez más
  en los sistemas y en las pruebas. Unificar es el único camino viable para crecer de forma
  lineal y no exponencial: no es una opción.»*
  ⚠️ **Primero conté MAL, y Eduardo lo corrigió**: sumé las líneas TOTALES de compilador
  y VMs, y eso mide el artefacto acumulado, no el esfuerzo — *«no los hemos escrito en esta
  versión, los hemos ido escribiendo a lo largo de 5. Si queremos ser justos habría que
  contar las líneas NUEVAS de esta versión. Y lo mismo con todo: el trabajo no es hacerlo
  todo de nuevo, es ampliar y reformar lo que ya hay.»* Tiene razón, y bien contado la
  tesis sale REFORZADA.
  📊 **V5 de verdad: 365 commits desde el tag `v4.0`**, y esto es lo tocado:

  | área | añadidas | borradas |
  |---|---|---|
  | compilador | 6.836 | 403 |
  | VM Java | 398 | 11 |
  | VM-C (núcleo + sistemas) | 3.802 | 110 |
  | **sistemas por familia** | **17.389** | **12.146** |
  | IDE | 2.682 | 255 |
  | stdlib | 6.973 | 4.075 |
  | documentación | 10.250 | 2.032 |

  ✅ **El reparto real del esfuerzo de V5**: lenguaje (compilador + VM Java) **7.234
  líneas**; sistemas (por familia + el `src/` de la VM-C, que es casi todo sistemas)
  **≈21.000**. **Los sistemas son TRES VECES el lenguaje.** Y la documentación sola (10.250)
  pesa más que el compilador.
  🔬 **Y el número que confirma lo de «ampliar y reformar»**: en sistemas por familia se
  añaden 17.389 líneas y se borran **12.146**. Esa proporción no es la de escribir cosas
  nuevas — es la de REFORMAR. Se tira dos tercios de lo que se pone.
  📌 **La derivada, que era el argumento**: al añadir una familia nueva, compilador **+0**,
  VM **+0**, sistemas **≈ +8.000**, y una placa más en cada campaña de pruebas (H13 son dos
  días por CINCO). El crecimiento no es exponencial por el código: lo es por la **MATRIZ**
  — cada familia multiplica las combinaciones y cada característica se multiplica por las
  familias. Hoy 5 imágenes; con el C3 y el C6 previstos, 7.
  🎯 **Formulado así, la tesis es más fuerte**: no es que el lenguaje sea pequeño, es que
  **el lenguaje ya no crece y los sistemas sí**. El esfuerzo se ha desplazado sin que nadie
  lo decidiera. Unificar convierte «añadir una placa» en *una cintura* en vez de *una
  reimplementación*, y «añadir una característica» en *una vez* en vez de *cinco*.
  🔗 Y enlaza con la tesis de fiabilidad de la ficha anterior: lo repartido no sólo cuesta
  más de mantener — **esconde sus bugs hasta que alguien prueba esa placa concreta**. Coste
  y fiabilidad empujan en la misma dirección.

- **🏆 [V6] POR QUÉ UNIFICAR: la tesis de Eduardo, contrastada con H13** — cierre de las
  reflexiones del 22-ago: *«¿cuántos problemas de memoria hemos tenido? ¿Y cuántos de FS?
  Ya nos hemos olvidado: los dos sistemas están funcionando. ¿Y por qué? Primero porque los
  unificamos en un único sistema, y después de unificarlos los fuimos puliendo hasta que
  desaparecieron los bugs. Así que **la unificación es el paso previo a tener sistemas
  fiables**.»*
  🔬 **Y H13 lo confirma sin que nadie lo buscara.** Clasificando TODO lo encontrado el 21
  y 22-ago sobre cinco placas:

  | hallazgo | de dónde nace |
  |---|---|
  | `#414`: builtins de packs dentro del `#ifdef BPVM_GUI` | packs — **por familia** |
  | blobs de `IO`/`Math` rancios (sólo la Pico) | generador — **por familia** |
  | `BAD_ALIGN` grabando packs en STM32 (4 K vs 8 K) | bloque de borrado — **por familia** |
  | `Core.mod` rancio en `/app` tapando a `/lib` | 3 estrategias de `/lib` — **por familia** |
  | el S3 sin vista de packs | packs — **por familia** |
  | 8 samples que no compilaban | lenguaje (`any`→`Object`) |
  | LSP entre interfaces · `native` en método ignorado | compilador |
  | 3 errores de documentación | docs |
  | **memoria** | **CERO** |
  | **sistema de ficheros** | **CERO** |

  📌 **Dos días machacando cinco placas** con SD, SQLite, ORM, GUI, packs, hilos y GC
  —incluido `SdCard` escribiendo 32.000 B en 500 trozos y `AotGcRt` forzando colectas— **y
  ni un fallo de memoria ni de FS**. Los dos subsistemas que en V4 costaron una campaña
  entera no han dicho una palabra.
  🎯 **Y los CINCO fallos estructurales vienen todos del mismo sitio: lo que NO está
  unificado** — packs, arranque, `/lib`. Ninguno del núcleo común.
  ⚠️ **Con una salvedad honesta**: queda un fallo sin explicar —la placa que dejó de
  ejecutar nada y sólo se recuperó reparticionando— y **podría ser de FS o de particiones**.
  Está fichado aparte con lo que hay que medir si repite. No cambia el balance, pero
  contarlo como cero sería hacer trampa.
  💡 **Lo que esto añade a la tesis**: no es sólo que unificar permita pulir. Es que
  **mientras algo está repartido en N copias, los bugs no se manifiestan donde se
  desarrolla** — aparecen en la placa que nadie probó, meses después. Memoria y FS se
  pudieron pulir porque, al ser únicos, **cada bug salía en el PC y en las cinco placas a
  la vez**. Un bug de packs sólo sale en la familia que lo tiene mal, y por eso los cinco de
  arriba llevaban meses ahí sin que nada fallara.

- **🎭 [V6] EL SIMULADOR CON DISFRACES: que `bpvm-sim` pueda vestirse de cada familia** —
  nace de la reflexión de Eduardo (22-ago): *«la VM-C es un hardware completamente
  diferente… podríamos hacer 2 versiones, una más libre, más cercana al PC, y otra más
  integrada con toda la arquitectura de las placas, y con ésta detectar más problemas de
  integración. Antes la VM-C servía para validar la VM y el compilador; esto serviría para
  validar todo el resto. ¿La ganancia? Detectar cosas en los micros es caro en tiempo;
  validar en el PC es mucho más ágil.»*
  ✅ **Las dos versiones YA EXISTEN**: `bpgenvm-c` (la libre, valida VM+compilador) y
  **`bpvm-sim`** (H10), que es *«servidor TCP wire v1 COMPLETO (META + FILES + TERMINAL +
  gestión de placa + packs) con FS littlefs sobre imagen y la VM-C de verdad ejecutando; el
  IDE lo trata como una placa más»*. Enlaza la librería entera y ya tiene dos smokes
  (`boardsim-smoke`, `sim-smoke`) que corren sin placa.
  🔎 **Entonces la pregunta útil no es si hacerlo, sino POR QUÉ NO CAZÓ LO DE HOY.** Y la
  respuesta acota el trabajo: **el sim valida el camino común; los bugs de estos dos días
  estaban en los caminos POR FAMILIA.**

  | hallazgo | ¿lo habría cazado el sim de hoy? |
  |---|---|
  | `#414`: builtins de packs dentro de `#ifdef BPVM_GUI` | ❌ el sim se construye **con** GUI |
  | packs a 4 KB vs los 8 KB del STM32 | ❌ tiene un solo bloque de borrado |
  | el S3 sin vista de packs | ❌ el sim sí la tiene |
  | las tres estrategias de `/lib` | ❌ el sim tiene una |

  📐 **Sus parámetros de hoy** (`--mem --psram --flash --fs`) cubren **memoria y
  almacenamiento, y nada de la personalidad de cada familia**.
  ⏭️ **La propuesta concreta**: un `--familia=<pico|s3|p4|stm32>` que fije lo que de verdad
  distingue a cada una y que ya sabemos enumerar porque lo hemos medido estos dos días —
  **bloque de borrado** (4 K / 8 K), **GUI sí/no**, **estrategia de `/lib`** (instalar si
  falta / si difiere / vaciar y reembeber), **hay vista de packs o no**, y **AOT
  disponible** (arm / riscv / ninguno).
  🎯 **Con eso, los dos bugs de packs de hoy se cazan en el PC en segundos** en vez de en
  dos días de placa — que es exactamente la ganancia que Eduardo busca. Y encaja con la
  lección del `#414`: *«lo que no funciona en C tampoco en la Pico» sólo vale si el C que
  se prueba lleva la MISMA configuración*. El disfraz ES esa configuración.
  📌 **Y una consecuencia de método**: esto convierte la batería multi-placa en algo que se
  puede correr **antes** de tocar hardware. La placa seguiría siendo la última palabra —hay
  cosas que sólo da el silicio— pero dejaría de ser el primer sitio donde se descubren las
  asimetrías.

- **🥾🥾 [V6] ¿UN boot o DOS?** — pregunta de Eduardo (22-ago): *«tenemos 1 boot, y
  dependerá del hardware. Si lo dividimos en 2, podemos tener un boot que dependa del
  hardware pero el 2º, que se ejecuta a continuación, podría ya ser independiente.»*
  ✅ **La división está EMPEZADA, sólo que sin nombre.** `bpvm_boot_climb()` ya es una
  escalera **común** (`KERNEL→PARTITIONS→FS→APP`) cuyos peldaños son **callbacks que pone
  cada familia**. O sea que la SECUENCIA ya es independiente del hardware y lo específico
  son los ganchos: el «boot 2» existe en embrión.
  🩸 **Lo que falta es que la FRONTERA tenga nombre — y se nota en que cada familia la
  dibuja donde le parece:**

  | familia | llama a la escalera desde |
  |---|---|
  | Pico | `main.c:1239` |
  | ESP32 | `board_mgr_esp32.c:313` |
  | STM32 | `board_mgr_stm32.c:141` |

  📌 **Y el ejemplo que lo demuestra, medido**: el `preinstall` de la Pico —el que puebla
  `/lib` desde los blobs y AVISA de módulos rancios, el que cazó el `IO.mod` desfasado el
  21-ago— vive en `pico/main.c:1285`, o sea **DESPUÉS de la escalera pero dentro del código
  de familia**. No tiene nada de hardware: es poblar un FS y comparar tamaños. Por eso sólo
  lo tiene la Pico, por eso el STM32 resolvió lo mismo de otra forma (vaciar y reembeber),
  el ESP32 de una tercera (sólo-si-falta), y **ninguno de los dos avisa**.
  🎯 **Ahí está el valor de la propuesta**: con un «boot 2» común y declarado, poblar `/lib`
  sería un peldaño suyo — y las cinco imágenes lo tendrían, o **dirían que no lo traen**.
  Las tres estrategias distintas de `/lib` y el peldaño de packs que el S3 no tiene son el
  mismo síntoma: **piezas sin hardware dentro viviendo en el boot de hardware**.
  ⏭️ **El reparto que sugiere lo medido**:
  - **Boot 1 (por familia)**: relojes, RAM, RTOS, transporte, acceso a flash. Lo que no
    existe hasta que el silicio arranca.
  - **Boot 2 (común)**: particiones → **packs** (hoy sin peldaño) → FS → poblar `/lib` →
    VM. Con ganchos sólo para *«cómo alcanzo este almacenamiento»*, que es lo único que
    de verdad cambia (ver la ficha de unificar packs: cabe en una función).
  🔗 Emparenta con todo lo anterior de hoy: el criterio de capas, el inventario, y el
  peldaño de packs que falta. **Son la misma reforma vista desde cuatro sitios.**

- **🏛️ [V6] ¿QUÉ INCLUYE el «sistema operativo» común, y dónde encaja cada pieza?** —
  pregunta de Eduardo (22-ago): *«por encima del HAL BP está sobre todo el sistema
  operativo: gestión de memoria, FS, etc. Y este es (debe ser) común. Entonces si es
  común deberíamos saber qué incluye. El sistema de packs es común pero se tiene que
  montar casi antes que todo lo demás, así que ¿va antes del SO o pertenece al SO?»*
  Y su encuadre del hito: *«V6 es un paso necesario, un poner orden. Es como V4, que era
  poner orden en la gestión de RAM y el FS; aquí es más a nivel de arquitectura.»*
  ✅ **Para los packs la respuesta YA está en el código, y es «pertenece»**:
  ```c
  void bpvm_pack_mount(const uint8_t* base, uint32_t size) {
      s_mounted_base = base; s_mounted_size = size;
      bpvm_fs_set_fallback(zone_res_stat, zone_res_read, NULL);   /* ← */
  }
  ```
  Montar un pack **registra un respaldo en la fachada de ficheros**. O sea que los packs ya
  están modelados como **un backend del FS**, igual que littlefs o la SD. No van antes del
  SO: son parte de él, en la misma capa que el FS, y por debajo sólo consumen particiones.
  🔑 **Y de ahí sale el criterio general para colocar cualquier pieza**: *la capa de algo es
  la de aquello que CONSUME*. Los packs consumen particiones (no FS) ⇒ por encima de
  particiones; y ofrecen ficheros ⇒ proveedor del FS, no algo previo.
  🩸 **Lo que falta, y es justo el «poner orden»: la escalera NO lo dice.** `bpvm_boot.h`
  declara `KERNEL(0) → PARTITIONS(1) → FS(2) → APP(3)` y **no hay peldaño para los packs**,
  así que cada familia los monta donde le parece: la Pico en `pack_pico.c`, el STM32 dentro
  de su `board_mgr`, el P4 tras su `mmap`… y el S3 **en ninguna parte**.
  📌 **Esa ausencia es la causa del agujero del S3**, no un descuido de quien lo portó: sin
  peldaño declarado, olvidarlo no rompe nada al compilar ni al arrancar — sólo aparece el
  día que alguien intenta grabar un pack en esa placa. Un peldaño obligatorio convierte el
  olvido en un fallo ruidoso, que es lo que el proyecto ya hace en otros cinco sitios.
  ⏭️ **El trabajo de V6, entonces, es doble**: (a) **declarar** qué capas hay y qué contiene
  cada una —el SO común: memoria, FS+backends, packs, planificador…—, y (b) que la escalera
  de arranque las refleje, de modo que **una capa no provista se DIGA** en vez de faltar en
  silencio. El mecanismo ya existe: `bpvm_boot` distingue *«capa no provista»* de *«capa
  fallida»* — hoy nadie usa esa distinción para los packs.

- **🎯 [V6] EL CRITERIO DE CAPAS, y lo que mide contra el código de hoy** — Eduardo,
  22-ago: *«si dividimos el código por capas, solamente la de hardware, la HAL y la BP HAL
  tiene sentido que sean diferentes; todo lo demás debe ser independiente del hardware y
  por lo tanto común»*. Es un criterio **operativo**: se puede contrastar. Esto es el
  contraste, medido el mismo día.
  🔴 **Lo que más lo incumple, y con diferencia: el REPL está TRIPLICADO en ~220 KB.**

  | fichero | bytes |
  |---|---|
  | `pico/repl_v1.c` | **102.966** |
  | `esp32/main/repl_esp32.c` | 69.052 |
  | `stm32/port/stm32_repl.c` | 47.816 |
  | *común del wire* (`bmgr_wire`+`dbg_wire`+`comm_common`) | *43.159* |

  El **transporte** sí es hardware (UART, USB-CDC); **interpretar `RUN`, `DIR`, `INFO` o
  `PACK_BURN` no lo es** — el protocolo es el mismo en las cinco imágenes.
  🧠 **Y esto explica CUATRO hallazgos del 21 y 22-ago que parecían independientes:**
  `SD_INFO`/`SD_MOUNT` sólo en `pico/repl_v1.c` · el `INFO` del STM32 sin cuatro campos que
  la Pico sí da · el aviso `/lib … NO es el de esta imagen` sólo en la Pico · el
  `preinstall` con comprobación, sólo en la Pico.
  **No son cuatro fallos: son cuatro síntomas del mismo.** Con tres REPL separados, cada
  mejora aterriza en uno y los otros se quedan atrás — y no se descubre hasta que alguien
  prueba esa placa concreta. Es [[arreglo-que-no-viaja-entre-familias]] con una causa
  estructural detrás.
  🔎 **El resto del contraste**, por si sirve para ordenar el trabajo:
  - ✅ **Cumplen el criterio** (son cintura y deben serlo): `bios_*`, `board_mgr_*`,
    `platform_*`, `comm_*`, `fs_lfs_*`, `flash_lock`, `psram`, `neopixel`, `gpio_*`,
    `gui_display_*`, los `main.c`.
  - ❌ **No lo cumplen**: los tres REPL (arriba) · `json_min.c` (**3 copias idénticas**) ·
    el log (núcleo común 8.937 B **+** 11.913 de la Pico, 5.216 del S3, 2.553 del STM32).
  - 🟡 **Ni una cosa ni otra: los blobs.** La Pico tiene **16 ficheros `*_mod.c`** con la
    stdlib embebida; el ESP32 y el STM32 la meten en **UNO** (`esp32_mods.c`,
    `stm32_mods.c`). Mismo dato, tres formas — y de ahí sale que la Pico "tenga 32
    ficheros" frente a 11. No es código: es la misma stdlib empaquetada distinto.
  ⏭️ **Y el orden que sugiere la medida**: `json_min` primero (gratis, los tres ficheros ya
  son idénticos), luego el REPL (donde está el 80 % del problema y el 100 % de las
  asimetrías que nos han mordido), y el log al hilo del REPL, porque buena parte de lo que
  cada familia mete ahí es diagnóstico del propio REPL.

- **📊 [V6] EL INVENTARIO de lo unificado y lo que falta** — medido el 22-ago, a raíz de la
  observación de Eduardo: *«poco a poco vamos unificando: ya tenemos particiones comunes,
  variables de entorno (más o menos), logs, y ahora packs. Y además la gestión de RAM y el
  FS.»* Es cierto; esto lo pone en números para que la unificación de V6 se planifique
  sobre datos y no sobre impresión.
  📐 **El reparto de hoy**: **57 ficheros `.c` en `src/`** (común) frente a 32 en `pico/`,
  11 en `esp32/main`, 9 en `esp32p4/main` y 11 en `stm32/port`.
  ✅ **Ya común de verdad**: particiones (`bpvm_part`), ENV (`bpvm_env`), arranque
  escalonado (`bpvm_boot`), núcleo de packs (`bpvm_pack`), fachada de ficheros
  (`fs_facade`), heap y GC, tabla BIOS.
  🔎 **Lo que sigue duplicado, por orden de facilidad:**
  1. **`json_min.c` — TRES COPIAS BYTE A BYTE IDÉNTICAS** (8.688 B en `pico/`,
     `esp32/main/` y `stm32/port/`). Es el parser JSON del wire y **no toca hardware**.
     Duplicación pura: subirlo a `src/` es la unificación más barata que queda y no tiene
     riesgo, porque los tres ficheros ya son el mismo.
  2. **El log está «más o menos», como el ENV**: hay núcleo común (`src/bpvm_log.c`,
     8.937 B) pero cada familia añade el suyo — y **el de la Pico (11.913 B) es MÁS GRANDE
     que el común**. Merece mirar qué hay ahí que no sea de hardware.
  3. **Packs**: ver la ficha de la unificación — la diferencia real cabe en una función.
  4. `fs_lfs` y `board_mgr`: motor común + cintura por familia. **Esto es lo correcto**, no
     hay nada que unificar; se listan para que no se confundan con los de arriba.
  📌 **Y el criterio que sale de esto**: la pregunta no es *«¿está duplicado?»* sino
  *«¿lo duplicado depende del hardware?»*. `board_mgr` duplicado está bien; `json_min`
  triplicado no. Sin esa distinción, un censo de duplicados manda a rehacer cinturas que
  están bien.

- **🏗️ [V6] UNIFICAR el sistema de packs: implementación común + cintura por hardware** —
  decisión de Eduardo (22-ago), al dejar el S3 sin packs en V5: *«yo unificaría el
  sistema, el mismo para todas las familias, con las particularidades de hardware de cada
  una. O sea: un sistema común, una implementación común, pero soporte a las diferencias
  particulares de cada hardware.»*
  📐 **Y el reparto sale MUY favorable, medido el 22-ago.** Lo único que difiere de verdad
  entre las cuatro es **cómo se consigue un puntero legible a la zona**:

  | familia | cómo obtiene el puntero |
  |---|---|
  | STM32 | `FLASH_BASE + pp->offset` — aritmética; la flash interna ya está mapeada |
  | RP2350 | `(const uint8_t*) base` — aritmética; XIP mapeado por hardware |
  | ESP32-P4 | `s_map_inst` de un **mmap explícito** — *«antes del mapeo no existe»* |
  | ESP32-S3 | **nada**: no existe `pack_s3.c` (ver la ficha del agujero) |

  ✅ **Todo lo demás YA es común y está probado en placa**: `bpvm_pack_mount()`, el
  recorrido de la zona, la búsqueda de módulos y `.mdn`, y el grabado entero por la cintura
  `bpvm_pack_flash_t` (erase/program/erase_block). O sea que **la diferencia cabe en una
  función por familia**: `mapear(offset, size) → const uint8_t*`.
  ⏭️ **La forma que sugiere el propio código**: un paso común que, con el layout del boot
  en la mano, pida el puntero a esa función y llame a `bpvm_pack_mount`. Las dos familias
  de aritmética la implementan en una línea; el ESP32 con su `mmap`; y **quien no la
  implemente lo dice**, en vez de quedarse en silencio como el S3 hoy.
  🎯 **Por qué esto vale más que arreglar el S3 a mano**: es la tercera vez que el mismo
  agujero aparece en una familia distinta (`#327` en la Pico, y hoy el S3). Cablearlo a
  mano una cuarta vez sólo mueve el hueco. Emparenta directamente con `#378` (que cada
  micro DIGA lo que tiene) y con la unificación que dejó el censo `#427`.

- **[VM] al fallar una dependencia, DECIR DE DÓNDE salió el módulo — por CRC** *(idea de
  Eduardo, 22-ago, y él mismo la sitúa en V6)*.
  🩸 **El problema, vivido el 22-ago**: el Nucleo dio
  `exit 11 (lib 'Core' presente pero no exporta 'Core.__cls_new_List'; ¿version vieja?)`
  **con `/lib` recién reembebido**. La causa era un `Core.mod` rancio en **`/app`**, y el
  mensaje no lo decía porque **sólo nombra el módulo, no el fichero**. Eduardo: *«debe
  indicar el path exacto, ya que la mayoría de las veces es porque hay más de un módulo»*.
  Costó media hora con el código delante; a un usuario no le sale.
  📐 **Por qué hoy no puede decirlo**: `bpvm_module_t` guarda `library` y `name` pero
  **no la ruta**. El cargador SÍ la conoce y la registra
  (`bpvm.c:512`, `[bpvm-c] dep 'Core' -> /lib/Core.mod`), pero eso va al log —que hay que
  tener encendido— y no al mensaje que ve el usuario.
  💡 **La idea de Eduardo, y por qué es mejor que guardar la ruta**: en vez de arrastrar
  una ruta por módulo, **guardar su CRC**; y cuando ocurra el error, recorrer los sitios
  donde pudo estar, calcular el CRC de cada candidato y decir cuál coincide.
  ✅ **Cuesta CERO en régimen normal**, que es lo que la hace buena: la búsqueda sólo
  ocurre cuando ya ha fallado algo. Y en memoria son **4 bytes por módulo** en vez de una
  ruta: con `BPVM_MAX_MODULES = 16`, **64 B frente a ~1 KB**. En un micro eso no es un
  detalle.
  🔎 **Comprobado que las piezas están**: `bpvm_crc32` ya existe en la VM-C
  (`include/crc32.h`, y `crc32.c` se enlaza en las cinco imágenes). El `.mod` no lleva CRC
  en su cabecera, así que se calcularía sobre los bytes al cargar — que el cargador ya lee
  enteros.
  ⏭️ **Y el remate que lo hace de verdad útil**: si ADEMÁS encuentra un segundo fichero con
  el mismo nombre y distinto CRC, decirlo — *«hay otro `Core.mod` en `/app` que NO es
  éste»*. Ese es el mensaje que habría resuelto la mañana del 22-ago en un vistazo, porque
  nombra las dos copias y no sólo la que se cargó.

### 🎯 V6/HITO-AOT — ampliar la cobertura del AOT, poco a poco (encargo de Eduardo, 21-ago)

> *«Creamos un hito AOT, donde solucionamos esto, implementamos double y mejoramos el
> soporte de statements que ahora no entran, al menos los más sencillos. Así poco a poco
> vamos ampliando el soporte AOT.»*

El criterio es suyo y conviene respetarlo: **ampliar por tandas**, no de un salto.

**1. `native` en un MÉTODO — arreglarlo, no sólo avisar.** 🟨 **LA MITAD BARATA, HECHA
(23-ago)**: ya **AVISA**. `AotCEmitter.emitModule` recorre ahora también los `ClassDef` y
nombra el método que no se emite —*«el metodo native 'Caja.doble' NO se compila a codigo
nativo todavia: corre interpretado»*—. Va **antes** del `return ""`, que era el camino por
el que se colaba el caso peor: un módulo cuyas únicas `native` son métodos salía diciendo
*«no tiene funciones native»* y punto.
✅ **Con test de regresión** (`AotNativeEnMetodoTest`, la **primera prueba del AOT** que hay
en el repo): tres casos —sólo método, mezcla con una `native` de módulo, y una clase limpia
que **no** debe generar ruido—. Comprobado que **falla sin el arreglo** (2 de 3), que es lo
que distingue un test de un adorno. Batería: 107/0.
✅✅ **Y LA OTRA MITAD TAMBIÉN (23-ago): los métodos `native` YA SE COMPILAN.** Salió tal
como lo planteó Eduardo —*«mimodulo.miclase.mimetodonative(objMiclase, ...)»*— y **no hubo
que forzar nada, porque por debajo ya era así**: `ModWriter.addMethod` hace
`declareParam("this", 8)` antes que los demás parámetros. Lo único que faltaba era
**exportar el nombre**, porque el registro AOT busca el símbolo para sacar su dirección.

📐 **Lo que se midió por el camino, y desmonta dos cosas que yo había escrito:**
- Un método `native` y uno normal producen el **`.mod` idéntico**. El `native` no cambia
  el bytecode — lo dice el propio comentario de `Ast.FuncDef.isNative`.
- El secuestro AOT va **por DIRECCIÓN** (`bpvm_aot_lookup(target_abs)` en `OP_CALL` y
  `CALL_EXT`), no por nombre. El nombre sólo sirve para hallar la dirección al registrar.
- Mi *«el cuerpo del método no tiene símbolo»* era **falso**: tiene dirección, sólo que
  `addMethod` la daba de alta con `exportar=false` y un comentario explicando por qué.

🔩 **Tres cambios**: `ModWriter` acepta `exportar` en `addMethod`/`addPrivateMethod` ·
`MivmEmitter` pasa `fn.isNative` · `AotCEmitter` **aplana** cada método a un `FuncDef` con
`this` delante, así toda la maquinaria (cuerpo, thunk, refs de 8 B, registro) sirve sin
tocarla. Registro: `<Módulo>.<Clase>.<método>`.

🔑 **Y la LECTURA DE PROPERTY, por su getter** — corrección de Eduardo: *«no se accede a un
campo, se accede como una propiedad y eso es llamar a un método get»*. No es una
conveniencia: en BP no hay campos públicos, es como ya lo emite el bytecode
(`emitInvokeVirtualSmart(cls, "get"+Nombre, 0)`), **y es la única ruta que el runtime
soporta** — entre los helpers del AOT está `call_method_i32` y no hay ninguno para leer
campos. El C sale así:
```c
return (vm->aot_helpers->call_method_i32(vm, this, 2, (const int32_t*) 0, 0, 0u, 0) * 2);
```
⚖️ **LIMITACIÓN ACEPTADA (Eduardo, 23-ago):** un `native` **no** puede leer un campo
directamente — *«es razonable, teniendo en cuenta que siempre puede acceder a través de un
getter»*. Se avisa y ese método corre interpretado.

🛡️ **Sin regresión**: un método que no se puede traducir **se cae solo**, con su aviso, en
vez de tumbar el módulo — que es lo que habría pasado con el `throw` de siempre, y habría
roto módulos que hoy emiten bien.
✅ Test reescrito a cuatro casos (emite · property por el getter · campo degrada sin
tumbar · clase limpia sin ruido). Baterías **108/0** y **34/0**.

**1 (continuación).** Hoy se ignora en silencio (ver
la ficha aparte). Son dos cosas y en este orden: que **AVISE** —barato, y convierte una
mentira muda en una línea— y luego **abrir el barrido** de `AotCEmitter.java:259` a los
métodos de las clases, pasando el objeto como primer parámetro. Ojo: `MemberAccessExpr`
ya está soportado, así que la parte que parecía difícil (`this`) puede que no lo sea.

**2. `double`.** Diseño hecho en `docs/V6_IDEAS.md` §double, con la ganancia estimada
sobre datos reales. Es la ficha `#426`.
⚠️ Con el matiz que ya está escrito en `AOT_LIMITES.md` y no hay que perder: la FPU de
Cortex-M33 es de precisión **simple**, así que un `double` no toca la FPU en dos de las
tres familias. Soportarlo es correcto; **prometer velocidad con él, no**.

**2 (estado, 23-ago).** ✅✅ **HECHO — `double` YA CRUZA a una función `native`.** ETAPA 1 — los 19 helpers ya están en la tabla** (lado
C): las 4 aritméticas, `mod`, `neg`, `pow`, las 6 comparaciones y las 6 conversiones,
añadidas **al final** (prefijo congelado, `#158`), así que es aditivo.
📌 **Dos decisiones tomadas al hacerlo:** las seis comparaciones van **separadas** y no como
un `dcmp` de −1/0/1 —con NaN no son negaciones unas de otras, y un comparador único
divergiría del intérprete en cuanto apareciera un NaN—; y **`DPOW` no se duplica**: se movió
a `bpvm_dpow`, UNA implementación que usan el intérprete y el helper. Su algoritmo venía
copiado byte a byte de `VirtualMachine.java`, y dos copias de eso se separan solas.
✅ **Con red nueva**: `powtest` entra en el corpus de paridad, que cubría `doubletest` pero
**ningún caso de `^`**. Paridad **34 → 35 PASS**.

⏭️ **LA COLA DE LA ETAPA 2, para no descubrirla a mitad.** Cuando el emisor empiece a usar
los helpers hay que **subir `MDN_ABI_VERSION` de 4 a 5**. Y eso NO es cosmético: el cargador
compara con **igualdad exacta** y rechaza —bien, con mensaje: *«el `.mdn` es de otra era de
los helpers AOT: hay que REGENERARLO»*—. O sea que ese día hay que regenerar:
- los **cuatro** nativos versionados de SQLite (`SQLite.mdn` y `sqlite.npk`, ARM y RISC-V),
- el **`SQLite.pack`** que los lleva dentro,
- y **reflashear las cinco imágenes**, porque la tabla vive en el firmware.

✅ **ETAPA 2 HECHA (23-ago)** — el emisor ya usa los helpers:
- `cType`/`readHelper`/`writeHelper` aceptan `double` (el marshalling ya estaba, compartido
  con `long`); se añadieron `read_f64_be`/`write_f64_be`, gemelos de los de `float`.
- literal `DoubleLitExpr`, y **toda la aritmética y las comparaciones por helper**:
  `dadd`/`dsub`/`dmul`/`ddiv`/`dmod`/`dpow` y las seis de comparar. Basta con que UN
  operando sea `double`, igual que la regla de `long`.
- **`MDN_ABI_VERSION` 4 → 5.**

✅ **VERIFICADO donde importa, no en el `.c` sino en el `.o`:** el C generado compila con
`-Wall -Wextra`, y en modo `.mdn` **no tiene NI UN símbolo indefinido**. Que era el objetivo
entero: sin helpers, `a + b` de `double` habría dejado un `__adddf3` sin resolver y el
empaquetador lo habría rechazado. Baterías 108/0 y paridad 35 PASS.

⏭️ **PENDIENTE, y decidido por Eduardo (23-ago): las imágenes NO se regeneran ahora,
*«cuando hagamos pruebas las regeneramos»*.** Eso es coherente: el `MDN_ABI_VERSION` subido
no tiene efecto hasta que se reconstruyan las VMs.

❌ **PERO LO DE REGENERAR SQLITE ERA FALSO — medido el 25-ago.** Lo escribí arriba dos veces
(*«ese día hay que regenerar los cuatro nativos de SQLite y su `SQLite.pack`»*) y **el
`.npk` no lleva el ABI de la tabla de helpers**: `bpvm_npack.c` valida por **arquitectura y
float-ABI** (`bpvm_mdn_host_arch` / `_float_abi`), no por `MDN_ABI_VERSION`. Ese gate es del
`.mdn` y sólo del `.mdn`.

La prueba, en el log del P4 ya con la imagen de ABI 6 y el `SQLite.pack` de siempre grabado:

```
pack: sqlite: pack vivo · 3.53.4
pack: sqlite: initialize OK — motor arrancado y vfs 'bp' registrado
pack: sqlite: API publicada como 'SQLI' — 17 simbolos, v1
```

📌 Confirmado **al cargar**. Confirmarlo **en uso** pide correr un sample del ORM, que es
otra cosa y no está hecho. Pero la tarea «regenerar los cuatro nativos» sale de la lista:
no existía.

⏭️ Lo que SÍ queda de aquí: **reflashear las cinco imágenes** (la tabla vive en el
firmware). Hecho ya en Pico, P4 y S3.

⚠️ **Y una medida que falta**: la paridad dual-VM **no cubre el camino AOT** (ejecuta el
intérprete en las dos VMs), así que estos helpers **no tienen red automática todavía**. La
prueba de verdad es cronometrar y comparar un bucle de `double` en una `native` contra el
mismo interpretado — que es justo lo que el diseño pedía para el primer día.

**3. Los statements sencillos.** 🟨 **TRES DE CUATRO HECHOS (23-ago), y la cuarta pata
—el medidor— PRIMERO**, que es lo que evita que las otras se pudran.

✅ **El medidor** (`AotCoberturaTest`): recorre los nodos del AST con un fragmento BP mínimo
por cada uno y **mide** cuáles pasan. La foto es el test: cambiarla es el gesto que registra
que la cobertura se movió; si nadie la tocó y hay diferencia, es regresión.
📌 **Y se validó a sí mismo**: la primera medida decía 17 de 28, pero al hacer que dijera el
**motivo** de cada rechazo salieron **seis fragmentos míos mal escritos** — cuatro nodos
(`ForStmt`, `IndexExpr`, `LongLitExpr`, `SwitchStmt`) **sí estaban soportados** y el censo
los habría dado por rotos. Un medidor sin la columna del porqué acusa a quien no es.

✅ **`null`** — un cero: el handle nulo ES el cero, nada que traducir.
✅ **`do…loop`** — el `do { } while (c)` de C tal cual; sin condición, `while (1)`.
✅ **`print`** — ⚠️ **y NO era «una llamada al runtime que ya existe»**, como decía esta
ficha: el intérprete despacha **por tipo** (seis rutinas distintas más el booleano por
builtin y el `toString` polimórfico), y la tabla de helpers sólo trae `i32`, `f32` y
`string`. Así que va **lo que hay** y se **rechaza el resto nombrando el tipo**: imprimir un
`long` con `print_i32` truncaría en silencio, y un `print` que miente es peor que uno que no
está — sobre todo aquí, que su uso es depurar. El espaciado se copia del intérprete.

⏭️ **Queda el literal de array**, que sí necesita alocar (`newarray_i32` y `array_store_i32`
ya están en los helpers, así que es abordable). Y quedan fuera, con su motivo escrito:
`try`/`catch` dentro de una native, `throw` de algo que no sea `RuntimeError(string)`, y el
destructuring de tuplas.

📊 **Cobertura: 20 → 23 de 27.** Baterías 109/0, paridad 35 PASS.
⚠️ Misma carencia que los `double`: **la paridad no cubre el camino AOT**, así que estos
tres no tienen red automática — sólo el medidor, que dice si *compilan*, no si *coinciden*.

**3c. ✅ CERRADO (24-ago) — CREAR objetos y arrays desde `native`.**
*(Anotado el 23-ago a petición de Eduardo: «déjalo pendiente por ahora pero que conste que
se puede hacer». Hecho el 24-ago: `6ba99f81` + `05a68055`. El detalle de lo que salió al
hacerlo, en **N1.5** más abajo — incluidos tres fallos mudos que estaban ahí desde antes,
y uno de la VM-C ajeno al AOT.)*

📐 **La evidencia, para que nadie lo vuelva a leer como un muro:**
- Las **ranuras ya están** en la tabla de helpers: `newarray_i32`, `newarray_i8`,
  `newarray_i16` y `new_object`.
- Y las cuatro son **muñones que devuelven 0**. Uno lo dice en voz alta:
  ```c
  bpvm_diag_urgente("[aot] newarray_i32 stub — implementar al AOT-ear arrays");
  ```
- El emisor **no las llama nunca** (`grep newarray|new_object` en `AotCEmitter` = 0).

✅ **Y el obstáculo de fondo YA NO EXISTE — se quitó en V5.** Lo difícil de alocar desde
código nativo no es alocar: es que **el GC no ve un handle que vive en una local de C** (o
peor, sólo en un registro). Eso está resuelto en `heap.c` §2d (`#302` paso 3), **con la idea
de Eduardo**: *«que el GC mire donde el native ya tiene sus handles, en vez de obligar al
native a apartarlos a un shadow stack»*. Escanea la pila de C del thunk, y usa el truco de
Boehm —un `setjmp` que vuelca los registros preservados— porque un handle puede vivir sólo
en un registro. Lo destapó `make test-aotgc`: un intermedio de `"valor " + intToString(n)`
se reciclaba a media expresión, **mudo y en host**.

⏭️ **Lo que queda es trabajo, no investigación**: implementar los cuatro helpers llamando al
alocador real —lo mismo que hacen `OP_NEWARRAY`/`OP_NEW` en el intérprete, con la disciplina
de siempre: **el helper hace lo que hace el opcode**— y enseñar al emisor a emitirlos. Con
eso caen `ArrayLitExpr` y la creación de objetos de una vez.

⚠️ **Y por qué se escribe así:** hoy dije cuatro veces *«no se puede»* donde era *«no está
hecho»*, y Eduardo me corrigió las cuatro. Ésta es la peor de las cuatro, porque la barrera
que iba a alegar **la había quitado él mismo una versión antes**. Ver
[[no-se-puede-vs-no-esta-implementado]].

**3b. Los statements sencillos — el enunciado original.** Del censo de `AOT_LIMITES.md`, por relación
esfuerzo/cobertura: **`print`** (llamada al runtime que ya existe), **`null`** (un cero),
**`do…loop`** (un `while` al revés), **literales de array**. Las cuatro son azúcar y
ninguna estaba documentada como límite hasta el censo del 21-ago.

⏭️ **Y una cuarta que propongo, porque si no las otras tres se pudren**: un **test que
recorra los nodos del AST** y compruebe cuáles pasan por el AOT. Mientras el emisor tenga
rechazos genéricos (`statement no soportado`), cualquier lista escrita a mano se queda
rancia sola — el propio `AOT_LIMITES.md` nombraba cinco cuando eran veinticuatro. Con el
test, la lista se mide en cada batería en vez de recordarse.

**4. 🧬 EL `.mod` SE COME AL `.mdn`** — *(metido en este hito el 23-ago por decisión de
Eduardo: «aunque no sea exactamente AOT, cuando terminemos se puede probar todo junto en
placa»)*. El razonamiento es el bueno: **la placa es lo caro**, y los cuatro puntos tocan la
misma superficie —código nativo—, así que una sola tanda de verificación los cubre.

Diseño completo en `docs/V6_IDEAS.md` §.mdn (≈90 líneas). En corto: un `.mod` lleva su
bytecode y **cero o varios bloques nativos, uno por familia**; el IDE poda antes de enviar y
al micro le llega sólo el suyo.

📌 **No es una apuesta: ya funciona en otro sitio.** El `.bpi` se fundió en el `.mod` en
V4/H6.a (71 ficheros borrados), y **el pack de SQLite ya lleva dos familias dentro y el IDE
lo poda al grabar** — 1.122.304 B en disco, 569.344 en la placa. Es el mismo flujo, subido
del NOMBRE del fichero al FORMATO, donde se puede validar.

⚖️ **Lo que justifica hacerlo** no es la comodidad, es que **mata una clase de fallo**: dos
ficheros que deben ir juntos se desparejan, y nos ha pasado de las dos maneras — en el
tiempo (*«el `.mdn` es MÁS VIEJO que su `.mod`»*, hay un guardián porque hace falta) y de
familia ([[artefacto-de-otra-familia-se-cuela]]: un `.mdn` de ARM subido a la P4). Con
bloques etiquetados dentro, elegir el equivocado **deja de ser posible por construcción**.

🔗 **Y ya es carga estructural**: `U1.3` se canceló porque esta fusión borra el escaneo del
`.mdn`. O sea que hay trabajo secuenciado contra ella.

🟨 **PRIMER PASO HECHO (23-ago): el FORMATO está cerrado — `.mod` v7.** A petición de
Eduardo (*«¿podemos hacer algo ahora, el caso más sencillo, una sola plataforma?»*).
- Header de **36 bytes** (9 enteros: se añade `nativeSize`) y sección **`native`** entre
  `interface` y `data`. Calcado de lo que hizo v6 con el `.bpi`.
- **Contenido: N blobs `.mdn` concatenados.** No hace falta tabla de contenidos porque un
  `.mdn` **ya se describe a sí mismo** (`code_size` + `sym_count` dan su tamaño) y **ya dice
  su arquitectura** (`arch`). Por eso **el formato de N familias es el mismo que el de una**:
  hoy N=1, y el multifamilia no pedirá tocar el formato, sólo emitir más blobs. La poda del
  IDE se vuelve *«quédate con el blob cuyo `arch` coincide»* — tirar bytes, no reformatear.
- ✅ **v6 SIGUE EJECUTÁNDOSE, así que NO hubo que regenerar los 32 `.mod` existentes.** El
  gate de `#284` vigila el **ABI** (el ancho de referencia) y v7 no lo toca: sólo añade una
  sección. Demostrado, no supuesto: la stdlib sigue siendo MOD6 y la paridad pasa.

⚠️ **Y una que salvó Eduardo, revisando el formato antes de que hubiera un solo blob: LA
ALINEACIÓN.** Las secciones del `.mod` **no están alineadas** —hoy `data` y `code` empiezan
en offsets impares— y da igual, porque el cargador las **copia** a `memory[]` alineadas. Con
`native` **no da igual**: un `.mod` en un pack se ejecuta por **XIP, en su sitio**, así que
la posición del blob en el fichero **es su dirección de ejecución**, y un blob RISC-V en
offset impar no arranca. La sección se alinea **a sí misma** (0–3 bytes de relleno al
principio, incluidos en `nativeSize`); el lector hace `align4(inicio)`.
📌 Sin esa observación, el fallo habría salido **sólo en placa, sólo con XIP y sólo en una
arquitectura**. De los caros.
✅ Y con test (`ModV7SeccionNativaTest`, 8 tamaños de blob), **que hacía falta porque el
relleno es código que hoy no ejecuta nadie** — la sección se emite vacía. El test cazó de
entrada que mi aserción no decía lo que el diseño promete.

📖 **DOCUMENTADO** —encargo de Eduardo: *«cuando esté hecho hay que documentarlo, para que
no sea necesario buscar en el código cómo se ha hecho»*—. `MOD_FORMAT.md` §1, §4.5, §4.6,
§10, §11 y §12. Y de paso se descubrió que **el doc llevaba DOS versiones de retraso**:
describía v5 cuando el compilador emitía v6, así que la sección `interface` **nunca se había
escrito**. Ahora están las dos.

🟩 **SEGUNDO PASO HECHO (23-ago): el blob ya viaja dentro, y el cargador lo lee.**

- **En memoria, idea de Eduardo** (*«¿no podemos utilizar un streamer en memoria (2 en
  realidad)…?»*) — y tenía razón en que era fácil, **porque `MdnPack` ya construía el `.mdn`
  entero en un buffer** y sólo al final hacía `Files.write`. `MdnPack.empaquetar()` devuelve
  los bytes; `pack()` se queda y delega, así que no hay dos implementaciones.
- **`ModFormat.conBloqueNativo(mod, blob)`** funde los dos en memoria. **Acumulativa**: una
  llamada por familia y el módulo acaba con todas, cada una con su `arch`. Multifamilia sin
  escribir nada más.
- **El IDE** (`AotBuild`) funde al vuelo. ⚠️ **Sigue emitiendo también el `.mdn` suelto** a
  propósito: quitarlo antes de que TODAS las imágenes lean la sección dejaría el AOT sin
  efecto en placa. Se retira cuando la otra mitad esté desplegada — y ése es el día en que
  dejan de poder desparejarse.
- **El cargador C** registra los thunks desde la sección, y va **al final de la carga** a
  propósito: `bpvm_load_mdn` resuelve por NOMBRE contra los símbolos exportados, que se
  registran antes. Ponerlo donde se salta la sección no habría encontrado ninguno.

📌 **Y una decisión que evita una segunda copia:** el gate de arquitectura **no se
reimplementa** en el bucle — se le pasan todos los blobs a `bpvm_load_mdn` y decide él.
Duplicarlo era tentador (ahorra intentos) pero equivocarse ahí significa **ejecutar código
de otra ISA**, y ya teníamos el gate probado. `MDN_ERR_ARCH` no se avisa: en un módulo
multifamilia es lo normal, y un aviso que salta siempre se aprende a ignorar.
*(De hecho mi primera versión SÍ lo duplicaba, y estaba mal: en host habría aceptado un
blob ARM.)*

🔬 **PRIMERA PRUEBA EN PLACA (23-ago) — dos fallos míos, los dos cazados por ella.** Eduardo
la corrió con `Bench` en la Pico y el sample dio `fib(28) AOT = 8518 ms`, **idéntico al
interpretado**. Ningún error. El mismo síntoma mudo que ya tiene comentario en `FrmMain` de
una vez anterior.

1. 🔴 **La fusión reescribía el `.mod` DESPUÉS de escribir el `.mdn`**, así que el `.mod`
   quedaba más nuevo y el guardián del IDE —que existe justamente para que no se
   desparejen— **rechazaba el `.mdn`**. Sin él y con el firmware cargando todavía por ahí,
   no se ejecutaba nada nativo. Arreglado invirtiendo el orden: fundir primero.
2. 🔴 **`MDN_ABI_VERSION` estaba en DOS sitios y sólo subí uno.** El C pasó a 5 y el Java
   seguía estampando 4, así que la placa rechazaba el `.mdn` — con un mensaje correcto que
   **va al log de la placa, no a la consola del IDE**, y por eso no se veía nada.

📌 **Lo que enseñan juntos**: los dos son *desfases mudos entre dos artefactos que deben ir
juntos* — exactamente la clase de fallo que esta fusión existe para eliminar. Que aparezcan
al implementarla es casi poético, pero también dice que **mientras haya `.mod` y `.mdn`
sueltos el riesgo sigue vivo**, y que la única cura es que quede uno.
⏭️ Y una carencia que dejan a la vista: **el IDE no sabe qué ABI habla la placa**. Lo publica
todo menos eso (`arch` sí va en el `INFO`), así que un desfase sólo se descubre ejecutando y
mirando el log del dispositivo. Merece ficha aparte.

✅✅ **VALIDADO EN PLACA (Pico, 23-ago): el nativo viaja DENTRO del `.mod` y se ejecuta.**
Con `/app/Bench.mdn` **borrado**, o sea sin ningún fichero suelto:

```
fib(28) interp =  317811  in  8601  ms
fib(28) AOT    =  317811  in    84  ms      ← 102×
```

Y en el log de la placa, la línea que lo prueba: `[mdn] 1 bloque(s) nativo(s) desde el
propio .mod` — precedida de `MDN: 1/1 thunks registrados, 64 code bytes (zero-copy)`.
**Zero-copy** significa que el código se ejecuta donde está, sin copiarse: para eso servía
toda la disciplina de alineación.

🩸 **LO QUE COSTÓ: CINCO fallos, todos míos, y NINGUNO detectable sin placa.** Vale la pena
listarlos porque el patrón es el mismo en los cinco —un desfase mudo entre dos cosas que
deben casar— y porque explica por qué esta feature existe:

| fallo | por qué no lo vio nadie |
|---|---|
| la fusión reescribía el `.mod` **tras** el `.mdn` | el guardián de frescura rechazaba el `.mdn`; cero errores |
| `MDN_ABI_VERSION` subido **sólo en C** | Java estampaba 4, el firmware hablaba 5; el rechazo va al log de la placa |
| `data + off` con el `.mod` **por trozos** | en la Pico `data` es NULL (H11); en host funcionaba |
| el relleno **copiado** a RAM | el blob caía en `copia+1..3` → acceso desalineado → hard fault |
| `malloc` + `free` con un cargador **zero-copy** | los thunks apuntaban a memoria liberada → cuelgue al ejecutar |
| *(y uno del REPL)* `bpvm_aot_clear()` **tras** la carga | borraba lo que el módulo acababa de registrar |

📌 **Los seis pasaron limpiamente por 109 pruebas de compilador, 39 de VM y la paridad
dual-VM.** No es que las redes fallaran: es que **ninguna toca el camino AOT en placa**. Es
la medida más clara que tenemos de dónde está el hueco de cobertura, y confirma lo que ya
se anotó con los `double`.
📌 **Y el instrumento decisivo fue el log del dispositivo.** Estuve tres iteraciones
teorizando sobre el código; en cuanto Eduardo lo encendió (`log=1`), la línea *«1/1 thunks
registrados»* descartó de golpe formato, subida, ABI, alineación y lectura, y dejó una sola
posibilidad. Encender el log era el primer paso, no el último.

⚠️ **Y el `clear()` mal colocado está en las TRES familias** (`repl_v1.c`, `repl_esp32.c`,
`stm32_repl.c`), porque el REPL está triplicado. Arreglado sólo en la Pico. Es el ejemplo
más limpio que ha dado V6 de por qué existe **U3**: un arreglo que hay que hacer tres veces
y que, si mañana se hace en dos, falla en silencio en la tercera.

⏭️ **Lo que queda para cerrarlo del todo**: desplegar en las cinco imágenes y entonces
retirar el `.mdn` suelto. Va con la tanda de pruebas ya comprometida — el ABI de helpers
(4→5, y **5→6** el 24-ago con N1.5) obliga a reflashear igualmente.

---

#### ✅ N1.5 (24-ago) — una `native` ya CREA arrays y objetos · `6ba99f81` `05a68055` `0f955e67`

**Lo que entró.** Los cuatro helpers de reserva llevaban desde la fase A siendo `return 0`;
ahora hacen lo mismo que sus opcodes, entra `newarray_i64` (`long[]`/`double[]`, que
faltaba), y el emisor sabe emitir `[a,b,c]` y `newXArray(n)`. Un **objeto** no se aloca en
C —el constructor es bytecode—: se cruza a la factoría `__cls_new_<Clase>`, que es la ruta
que `#213` ya había abierto para `throw MiExcepcion(...)`; sólo faltaba dejar de mirarla
únicamente dentro de un `throw`. `new_object` sigue sin implementarse y **ahora lo dice**.

📌 **Esto es sobre todo COBRAR V5.** Lo que lo bloqueaba no estaba en el emisor: un objeto
recién creado vive en un local de C, y hasta V5 el GC no miraba ahí. Fabricarlo era
fabricar algo recolectable en vivo. Lo quitó `#302` paso 3 — idea de Eduardo.

##### ✅ N1.5b (24-ago, la misma tarde) — arrays de REFERENCIAS y de `float`

Escribí que los arrays de referencias no se podían *«porque en esta ABI una ref cruza con
la generación descartada y un `TYPE_ARRAY_REF` las guarda con ella»*. **Falso**, y lo
corrigió Eduardo en el momento: *«desde V5 todas las referencias a objetos, arrays y
strings deberían poder pasarse tal cual»*.

📐 **Comprobado antes de tocar nada**: `bpref_regen(vm, ref)` (`bpvm_internal.h:733`)
reconstruye la generación viva consultando `vm->handle_gen[idx]`. No es pérdida de
información: es un límite de **transporte**, no de **capacidad**. Y las dos líneas ya
estaban escritas dos veces en el propio fichero de helpers — `read_ref`/`write_ref`, la
frontera del thunk desde `#302`.

Hechos esa misma tarde, para que entren en el mismo reflasheo: `newarray_ref`,
`array_load_ref`, `array_store_ref` y, de paso, `array_load_f32`/`array_store_f32`. El ABI
se queda en **6**: nunca se ha flasheado nada con él, así que sumar slots no rompe a nadie.

📌 **`array_store_ref` es la frontera donde se recupera la generación**, y no es un
detalle: guardar la palabra baja a secas dejaría gen 0, que no casa con nada → el objeto se
leería **muerto** al primer acceso. Use-after-free, no error.

✅ **Probado por el peldaño 3**, que es el que puede decir algo aquí: `largoDeDos` crea un
`string[]` y mete en él dos cadenas **recién alocadas**, con `gc_bump_threshold = 1`.
Rellenar la casilla 1 dispara un GC mientras a la 0 sólo la sostiene el array. Con la
generación mal puesta eso no da error: da corrupción. Siete valores exactos.
`AotCoberturaTest` sigue en 28/30 (esto no añade nodos: los quita de la lista de rechazos).

📌 **Y un literal de cadena SÍ puede ir dentro.** El sample lleva uno a propósito
(`["1234", "56", "xy"]`), porque eso lo resolvió `#428` en V5 y conviene que se note si
alguna vez deja de ser verdad.

**Aplazado a V7** (decisiones de Eduardo, 24-ago). Con el planteamiento inicial que dio él
al cerrar la tarde, que en los dos casos es el mismo: **una función AOT es una función BP
con código nativo**, así que lo que funciona en BP debería poder reproducirse dentro.

##### 🧩 V7 — TUPLAS dentro de una native

> Eduardo: *«las tuplas deberían funcionar, a fin de cuentas son objetos»*.

Y es literal: el compilador sintetiza una clase oculta por forma
(`MivmEmitter.synthesizeTupleClasses`) con campos `_0`, `_1`, … El destructuring es *coge
la ref, lee sus campos*. De las dos mitades, **una ya funciona**: la llamada devuelve una
ref y el puente sabe traerla (`ret_is_ref=1`, lo mismo que hace la factoría de un objeto).

⏭️ **Lo que falta es LEER UN CAMPO desde native**, y hoy no hay helper para eso — a
propósito: la regla es *público ⇒ property*, y una property se lee por su getter
(`call_method_i32`). Pero una tupla **no tiene getters**: sus `_i` son campos pelados. Dos
salidas, y conviene elegir a la vista:

- **helpers de campo** (`get_field_i32` / `_i64` / `_ref`), del mismo tamaño que los de
  array de N1.5. El slot es determinista y el compilador lo conoce. Contenido, pero abre
  una puerta que la regla de las properties cerró a propósito;
- **darle getters a la clase sintética**, y entonces el camino es el que ya existe. No
  añade mecanismo, pero mete slots de vtable en todo módulo que use tuplas.

##### 🧩 V7 — `try`/`catch` dentro de una native · ⚠️ LA OBJECIÓN ERA FALSA

Aquí ponía que no se podía *«porque el `.mdn` no puede llamar a `setjmp`: cero
relocalizaciones externas»* (`#213`). Eso es cierto **del `setjmp` de libc** — y no es el
único salto no-local que hay.

📐 **MEDIDO el 24-ago con los dos toolchains reales**: `__builtin_setjmp` /
`__builtin_longjmp` de gcc **se expanden INLINE**. Un `.o` que los usa sale con **cero
símbolos indefinidos**, y empaqueta: **ARM 52 B, RISC-V 60 B**. O sea que el salto no-local
dentro del `.mdn` existe.

⏭️ **Lo que queda por resolver NO es el salto: es a dónde salta un `throw`.** Hoy un helper
que lanza hace `longjmp` al *boundary AOT* del intérprete, saltándose de largo el marco del
native. Para cazar dentro hace falta que el `eh_stack` sepa que hay un manejador NATIVO en
medio — y ahí encaja exactamente el planteamiento de Eduardo: darle al native las mismas
primitivas que tiene el intérprete (empujar y sacar manejador), con la única diferencia de
que el aterrizaje es un `__builtin_longjmp` a su marco en vez de una asignación de PC.

⚠️ **Y lo que NO está medido, para no repetir el error de hoy**: que `__builtin_setjmp`
aguante lo que aquí se le va a pedir. Está pensado para uso interno de gcc, no guarda los
registros callee-saved como el `setjmp` de verdad, y el salto tiene que llegar desde OTRO
marco (el del helper). El experimento de arriba salta dentro de la misma función. **El
siguiente paso de V7 es un caso que salte desde una llamada anidada**, no dar por bueno lo
que este mide.

##### ⏭️ PENDIENTE de N1: verificar CADA FAMILIA por separado

*(Encargo de Eduardo, 24-ago: «esta tarde verificamos en la Pico y las otras dos familias
más adelante».)*

Todo lo de N1 —N1.1 a N1.5— está verificado en host y, de N1.4, **sólo la Pico**. El resto
de familias no ha ejecutado una sola línea de este código. No es una formalidad: N1.4 dejó
**seis fallos que sólo se ven en placa** y ninguna de las 148 pruebas los tocó.

| familia | estado |
|---|---|
| **RP2350** (Pico 2 / Metro) | ✅ **CERRADA (24-ago)**: N1.1…N1.5b en placa, `NatNew` 7/7 y `NatV7` 4/4 thunks. Ver abajo |
| **ESP32-P4** (RISC-V) | ✅ **CERRADA (25-ago)**: `NatV7` **4/4 thunks** y `NatNew` **7/7, 1448 code bytes** — el mismo número que produjo MdnPack, cuadra punta a punta. Los siete valores exactos. Ver abajo: costó **tres** fallos que sólo la placa podía enseñar |
| **ESP32-S3** (Xtensa) | ➖ **NO APLICA**: no hay generador AOT para Xtensa (`NpackReloc.DESTINOS` = ARM + RISC-V), y su `aot_funcs_stub.c` es un no-op explícito. Lo que sí se verificó es el **wire** |
| **STM32** (Nucleo U575) | ✅ **CERRADA (25-ago)**: `NatV7` 4/4 thunks y `NatNew` los 7 valores — **primer AOT ejecutado en un STM32**. `sumaHasta` en 0 ms a 160 MHz = nativo de verdad. Costó DOS fallos propios de esta placa, ver abajo |

📌 Va por familias y no de golpe por [[focus-un-kit-batch-cross-family]]: a fondo en una
placa, las demás juntas y más adelante.

##### 🔴 Y lo que salió al hacerlo: tres fallos mudos, ninguno nuevo

| fallo | desde | por qué no lo vio nadie |
|---|---|---|
| el censo del AOT medía **cinco fragmentos mal escritos**, no el AOT | 21-ago | `pasa()` sólo miraba errores del PARSER; lo que parecía «no soportado» era «no compila». `ThrowStmt` llevaba soportado desde `#186` |
| ~~el puente native→BP metía el nombre en `.rodata`~~ | — | **FALSA ALARMA, ver abajo**: comprobé el `.o` sin el paso de ENLACE, que el pipeline hace siempre |
| indexar un `long[]`/`word[]` en native leía **4 bytes** | siempre | `arrElemKind` devolvía "i32" para todo lo que no fuera `byte`. No fallaba: devolvía otro número |

##### 🔴🔴 CORREGIDO EL MISMO DÍA: **dos de esos «fallos» no existían**

Lo destapó una pregunta de Eduardo: *«lo de soporte de literales strings lo vimos en V5.
Había un problema con el compilador gcc, sé que lo solucionamos pero no sé si se hizo
alguna trampa»*. No hubo trampa: es **`#428`**, cerrado el 16-ago con su idea —*«esos
literales tienen que ir como parte del código nativo»*— y **verificado en la Metro**. Un
guión de enlace compartido (`bpgenvm-c/aot/mdn.ld`) **fusiona `.rodata` dentro de
`.text`**, y sigue siendo relocatable: enlazado a dos direcciones distintas el `.text` sale
byte-idéntico.

**Yo comprobaba el `.o`, y el pipeline real empaqueta el `.elf` ENLAZADO**
(`AotBuild.enlazar` = true en TODAS las familias). Medido las dos formas con el mismo
fichero:

| | `MdnPack` sobre el `.o` | sobre el `.elf` enlazado |
|---|---|---|
| una native con `return "hola"` | ❌ *«1 referencia fuera de .text: .LC0»* | ✅ 60 B, 1 símbolo |
| `print "valor:", n` (el «fallo» del 23-ago) | ❌ | ✅ 116 B |
| el puente `find_function("Mod.func")` | ❌ | ✅ |

Así que **el puente native→BP sí llegaba a una placa**, y el `print` del 23-ago **sí
compilaba para ARM**. Los dos «hallazgos» eran el mismo error de medida, y el cambio que
metí por ellos —materializar el nombre byte a byte— sobraba: **revertido**.

📌 **Lo caro no fue el código de más: fue medir UNA ETAPA QUE EL PRODUCTO NO TIENE y sacar
de ahí una conclusión sobre la placa.** El instrumento estaba bien; lo que estaba mal era
dónde lo puse. Enlaza con [[instrumento-mudo-dudar-de-el.md]] por el otro lado: aquí el
instrumento no callaba, gritaba — y gritaba sobre algo que no era el producto.

⏭️ **La regla que queda**: si `MdnPack` se queja de un `.LC0`, la pregunta es **si estás
enlazando**, no si hay que quitar el literal.

Y uno **ajeno al AOT**, de propina: `OP_ASTORE_I16` de la VM-C escribía cuatro bytes de
ceros en una casilla de dos → guardar `v[0]` ponía a cero `v[1]`, y el último elemento se
salía del array. Sólo en la VM-C, o sea **divergencia entre las dos VMs** (`0f955e67`,
guarda en `samples/WordStore.bp`). Salió leyendo ese opcode para copiarlo en el helper
nuevo; la línea llevaba el comentario «unused (preservado de la versión previa)».

##### 📐 La lección de método: **el censo mide EMISIÓN, y eso no basta**

Que el emisor acepte una construcción no dice que su C compile para el micro ni que dé el
número correcto. Las dos cosas fallaron: `print` pasaba el censo y no compilaba para ARM
(23-ago); `long[]` pasaba el censo, compilaba, empaquetaba **y devolvía otro número**.

De ahí salen los **tres peldaños**, cada uno con su herramienta:

| peldaño | herramienta |
|---|---|
| emite | `AotCoberturaTest` — foto **28 de 30** |
| cabe | `arm-none-eabi-gcc` + `MdnPack`, y `riscv32-esp-elf-gcc` + enlace |
| **acierta** | **`make test-aotnew`** *(nuevo)* — compila el mismo `.c` con el compilador del host y lo EJECUTA con `gc_bump_threshold = 1` |

El tercero es el que faltaba y el que cazó el `long[]`. Y hace la pregunta que sólo se
puede hacer ejecutando: **¿sobrevive al GC lo que la native fabrica?** Colectar en cada
alocación convierte esa ventana de lotería en certeza. Cinco valores exactos: arrays i32,
i64, i16, f64 y un objeto. Gemelo de `test-aotgc`, y regenera el `.c` desde el `.bp` —
nunca un artefacto rancio.

##### ✅ VERIFICADO EN LA PICO (24-ago, tarde)

`NatNew.mod` con su bloque ARM dentro (3.363 → 4.808 B), y el log de la placa:

```
[313579] MDN: 7/7 thunks registrados, 1172 code bytes (zero-copy)
[313579] [mdn] 1 bloque(s) nativo(s) desde el propio .mod
...
[313596] AOT: buscando el .mdn de 2 modulos (FS + pack)
```

📌 **Y esas dos líneas no son ambiguas, que era la duda**: `MDN: 7/7` lo emite
`bpvm_load_mdn(vm, sec + p, tam)` — `sec` apunta DENTRO de la sección del `.mod` — y
`[mdn] 1 bloque(s)` sólo sale si esa misma llamada devolvió OK (`loader.c:521-530`). Los
dos en el mismo tick, y el barrido del `.mdn` suelto no entra hasta 17 ms después sin
emitir un segundo registro. **No hizo falta borrar el `.mdn` suelto para desempatar**: lo
desempata de dónde sale el mensaje.

Los siete valores, exactos e idénticos a miVM y a la VM-C de host. Con eso quedan
ejecutándose en ARM: arrays i32/i8/i16/i64/f64/f32 **y de referencias**, creación de
objetos por la factoría, un literal de cadena fusionado en `.text` por `mdn.ld`, y
`long`/`double` dentro de una native.

⚠️ **Lo que esto NO mide**: el tiempo. Registro + resultado correcto demuestran que el
código nativo se instaló y que coincide con el intérprete —el secuestro es por dirección en
`OP_CALL`, así que registrado implica ejecutado— pero el cronómetro de este sample no
existe. El 102× de ayer (`Bench`) es la medida de velocidad.

##### 🔴 Y la segunda pasada destapó que N1.1 NUNCA se había acelerado

`NatV7` —el sample de lo de ayer: métodos `native`, `double`, `print` desde native— dio
**3 de 4**:

```
MDN: skip 'NatV7.Caja_doble' rc=-2 (symbol no en .mod?)
MDN: 3/4 thunks registrados
```

El `.mod` exporta el método como `Caja.doble` y `MdnPack` pedía `Caja_doble`: reconstruía
el nombre BP partiendo el identificador de C por el prefijo `thunk_<Mod>_`, y `cId()` ya
había convertido el punto en guion bajo. De `Caja_doble` no se recupera `Caja.doble`
—un identificador BP también lleva guiones bajos—, así que **no había forma de acertar**.

📌 **Las funciones de MÓDULO sí acertaban** (no tienen punto que perder), y por eso el
fallo vivía escondido detrás de los casos que iban. Es
[[n-casos-del-mismo-sample-no-son-n-casos]]: 3 de 4 verdes no dicen nada del cuarto.

⚠️ **Y no daba error.** El thunk no se registra, el método corre interpretado y el programa
imprime 42. N1.1 se cerró el 23-ago verificado por EMISIÓN —el C generado era impecable,
con el nombre equivocado— y ésta fue la primera vez que ese caso se ejecutó en una placa.
El único sitio donde se veía era el log, y sólo con `log=1`.

**Arreglo** (`e7b50683`): dejar de adivinar. El emisor pone el nombre verdadero en el
propio símbolo ELF (`__asm__("thunk_<Mod>_<nombre BP>")`) y `MdnPack` lo lee tal cual; su
filtro de alias de gcc pasa de «cualquier nombre con punto» a los sufijos de clonado
concretos, porque ahora el punto es parte del nombre. **Sin tocar C: no hubo que
reflashear**, sólo rehacer el fat-jar del IDE.

✅ **Reverificado**: `MDN: 4/4 thunks registrados`, sin skip.

##### 🕸️ La red que faltaba — `AotSimboloEnModTest`

Compara los nombres que el AOT va a registrar contra los símbolos que el `.mod` exporta de
verdad. **Caza esta clase entera porque compara DOS ARTEFACTOS**, y el fallo era un
desacuerdo entre dos caminos distintos (`AotCEmitter` y `MivmEmitter`): una prueba que mire
uno solo no puede ver nada. Comprobada en rojo reintroduciendo el fallo antes de darla por
buena, y su mensaje dice literalmente que el síntoma es *«el resultado sale bien»*.

##### ✅ LA P4 (25-ago) — y los TRES fallos que hicieron falta para llegar

Ninguno era del AOT. Los tres eran **el `.mdn` cruzando a la placa sin que nadie comprobara
que casaba con ella**, y ninguno se manifestó como un error: uno colgó y dos devolvieron un
número.

| # | qué | síntoma | por qué no lo vio nadie |
|---|---|---|---|
| 1 | el `clear()` del registro AOT iba **después** de cargar | habría dado 0/N | arreglado en la Pico el 23-ago y nunca viajó — [[arreglo-que-no-viaja-entre-familias]] |
| 2 | el `.mdn` se compilaba con **otra ABI y otro repertorio** que el firmware | **CUELGUE** | `AotBuild` no fijaba `-march`/`-mabi` *«porque casan por construcción»*. Falso: el defecto del toolchain trae la extensión `d` (doble en hardware) que el P4 no tiene |
| 3 | las **constantes de coma flotante** se quedaban fuera del blob | `Infinity` y `1.219193E25` | `mdn.ld` fusiona `.rodata*` pero RISC-V las pone en `.srodata*` |

📌 **Y uno más, que fue el que permitió ver los otros**: el log del cargador era un hook
débil que **sólo la Pico implementaba**. La P4 cargaba el bloque y no había forma de saber
si sus thunks habían entrado. Se quitó el hook y ahora el loader escribe en el log común —
las cuatro familias ven lo mismo.

##### 🕸️ Lo que queda de todo esto: tres guardianes donde no había ninguno

1. **ABI de coma flotante** del `.o` RISC-V (`e_flags`) y **repertorio** declarado
   (`.riscv.attributes`, la extensión `d`). Aborta el empaquetado nombrando el remedio.
2. **Nada con contenido fuera de `.text`**. El guardián que ya existía mira las
   *relocalizaciones del `.o`*; el pipeline empaqueta el `.elf` **ya enlazado**, donde
   están resueltas — o sea que una sección que se cae fuera **no dejaba rastro**. Éste vale
   para lo que venga.
3. Y el del día anterior: **el nombre del símbolo** tiene que existir en el `.mod`
   (`AotSimboloEnModTest`).

⚠️ **Los tres se probaron EN ROJO** antes de darlos por buenos, con el artefacto que
fallaba de verdad.

📐 **La lección de método, que es la misma tres veces**: el `.npk` valida arquitectura y
float-ABI desde V5/H4. El `.mdn` no validaba **nada**. Dos formatos hermanos, uno con
contrato y otro sin él — y el que no lo tenía es el que se cargó tres veces.

##### ✅ EL STM32 (25-ago, tarde) — y los DOS fallos que costó

**1. La imagen que llegaba a la placa no era la construida.** El build headless regenera el
`.elf` **pero NO el `.bin`** —trampa ya documentada en `PUBLICAR.md`— y el `.bin` del
`Debug/` era del **5 de agosto** (más un `bpvm_stm32.bin` de **junio** haciendo de señuelo).
Dos flasheos en falso: la placa corría una imagen pre-v7 que rechazaba todo `.mod` de hoy
sin decir palabra. 📌 Lo desatascó **la pregunta de Eduardo** —*«¿por qué el log sólo añade
una línea cuando en la Pico añade 12?»*— que obligó a mirar el instrumento antes que la
teoría: el camino de fallo del RUN **no hablaba**. Ahora habla (status + fallo + missing +
el MAGIC leído), y los `.bin` están regenerados.

**2. La frontera de coma flotante hablaba OTRA ABI.** Primer AOT en un STM32: 4/4 thunks,
`sumaHasta` a velocidad nativa… y `mediaPor2(3,5) = NaN`. Sólo los `double`. Medido en los
dos firmwares:

|  | float-abi | los `double` cruzan por |
|---|---|---|
| Pico (funcionaba) | `softfp` | r0-r3 |
| STM32 (NaN) | `hard` | la FPU (d0) |

El `.mdn` ARM es **uno para las dos** y va en softfp: en el STM32, `h_dadd` leía la FPU
mientras el operando estaba en r0-r1. **En la Pico casaba de chiripa histórica, no por
contrato.** Arreglo (`da9be3af`): `pcs("aapcs")` —macro `BPVM_AOT_FP_ABI`, sólo `__arm__`—
en las **28 entradas** de la tabla que cruzan float/double por valor, en el tipo del
puntero **y** en la definición: si no coinciden, **no compila**, que es la red. Verificado
en el desensamblado (h_dmod recibe en r0-r3 y hace él mismo los `vmov` a su `fmod`). El
`.mdn` no cambia ni un byte; la Pico tampoco. Sin subir `MDN_ABI`.

##### 🎯 N1 EN PLACA: CERRADO EN LAS TRES FAMILIAS CON GENERADOR

| familia | evidencia |
|---|---|
| RP2350 | 7/7 y 4/4, 102× (`Bench`) |
| ESP32-P4 | 7/7 (1448 B) y 4/4, RISC-V |
| STM32 U575 | 4/4 y los 7 valores, 0 ms el bucle |

📐 **El patrón de la semana, cerrado**: seis fallos de integración `.mdn`↔placa en tres
días —clear(), ABI RISC-V, `.srodata`, nombres de símbolo, float-ABI ARM, y el censo que
medía mal— y **ninguno dio un error**: colgaban o devolvían un número. Hoy los seis tienen
guardián o instrumento.

✅ **Y el `.mdn` suelto, RETIRADO (25-ago, `761fd6cc`) — verificado por Eduardo en placa.**
Petición suya: *«si quitas que genere el .mdn me haces un favor, lo que tengo que borrar en
cada prueba»*. Tres piezas: AotBuild no lo escribe (y retira el rancio del outDir), el
Explorer lo retira del DEVICE al subir — que no era cosmética: el barrido del RUN registra
el suelto DESPUÉS del embebido, así que un rancio en `/app` habría PISADO a los thunks
frescos —, y siguen a propósito los de doble extensión del pipeline de packs y el barrido
de los REPL (camino de los `.mdn` de deps versionados, como `SQLite.mdn`).

##### 🏁 N1, EL ALCANCE DE V6: CERRADO (25-ago)

Formato v7 con sección native · cobertura 28/30 nodos (medida, no recordada) · arrays de
todos los anchos y refs, objetos, `double` · verificado en las TRES familias con generador ·
el `.mdn` suelto retirado · seis guardianes/instrumentos donde no había ninguno. **Lo que
queda es de V7 a propósito**: tuplas y `try`/`catch` en native (arriba), y la nota de que la
paridad no cubre el camino AOT (mitigada por `make test-aotnew` en host).

---

⚠️ **Lo caro no es la fusión, es su cola** — está en el diseño y conviene no descubrirlo a
mitad: el gate de ABI sube la versión del `.mod` y eso deja **rancias las cuatro copias de
la stdlib** (incluida `packs/Stdlib.pack`); el IDE compara local-contra-device para no
resubir y **al podar esa comparación empieza a mentir**; y la poda debe ser **UNA** función
llamada desde los dos sitios, o es [[arreglo-que-no-viaja-entre-familias]] otra vez.

> Decisión de Eduardo (16-ago) al sacar `#426`: lo que no es de esta versión no
> debe engordar su lista. Se quedan escritas aquí para no perderlas.

*(Movidas aquí el 17-ago por decisión de Eduardo: la lista de pendientes de V5
se revisa EXCLUYENDO lo de V6. Nada se pierde: está aquí, con su texto.)*


- **[IDE] enseñar el `durationMs` que la placa YA manda** *(salido el 21-ago midiendo
  `#408`; aplazado a V6 porque es mejora, no bug — code freeze)*.
  📐 **El hecho**: `SAVE` se cronometra en el firmware y devuelve `durationMs` en el
  `SAVE_REPLY` (`pico/repl_v1.c:929-943`). El IDE responde «FS guardado en flash» y
  **tira el número**. O sea que la medida que pedía `#408` ya existe, ya viaja por el
  wire, y sólo falta imprimirla.
  ⏭️ Lo barato: que la consola diga «FS guardado en flash (1.234 ms)». Y de paso mirar
  qué otros verbos ya devuelven tiempos que nadie enseña — si `SAVE` lo hacía sin que
  lo supiéramos, puede haber más.

- **[IDE+device] NO copiar dependencias que el dispositivo YA TIENE — y que lo diga él**
  *(idea de Eduardo, 20-ago. Aplazada a V6: es mejora, no bug.)*
  🩸 **El problema, con nombres**: hoy el IDE sube al dispositivo las dependencias del
  programa en cada Run. Entre ellas van `Json` (21 KB) y `Gui` (43 KB), que son las dos
  más grandes de la stdlib. Eduardo: *«que Json y Gui se carguen cuando se ejecuta un
  programa no es del todo correcto porque son grandes y consumirán RAM»*.
  📐 **Y no es sólo transferencia: es RAM.** Un módulo que acaba en el sistema de ficheros
  se carga ENTERO en memoria para ejecutarse. Uno que vive en un pack se ejecuta **en el
  sitio**, desde la flash — a RAM sólo van su ext-table y su bloque de datos
  (`bpvm_loader_load_xip`, y la VM lo canta: *«cargado XIP desde pack (codigo en
  sitio)»*). O sea que **el mismo módulo cuesta RAM desde el FS y casi nada desde el
  pack**. Con `Stdlib.pack` grabado, `Gui` deja de costar 43 KB de RAM.
  ⏭️ **El diseño, afinado por Eduardo (20-ago), y es la clave**: el dispositivo no debe
  contestar con un inventario. Debe **buscar la dependencia EXACTAMENTE COMO LO HACE EL
  CARGADOR DE MÓDULOS** y contestar una sola cosa: hace falta o no hace falta.
  *«Si la dependencia está en cualquier sitio donde el cargador la encuentre, y es igual o
  más antigua que la que hay grabada, no se carga. Así que da igual que el módulo esté en
  `/app`, `/lib`, `/sys` o en un pack.»*
  📐 **Por qué esto es lo correcto y no un detalle**: cualquier otra respuesta obliga al
  IDE a reimplementar la resolución de imports, y entonces hay **dos buscadores** que se
  desincronizan en cuanto uno cambie. Es el patrón que ya ha mordido en este proyecto
  varias veces (una copia privada que no se enteró de que el común creció). Con esto hay
  UN algoritmo, el del cargador, y el IDE sólo pregunta.
  ✅ **Y la mitad ya está construida.** `PicoExplorer.putIfChanged` YA le pide al
  dispositivo el CRC del fichero y **se salta el PUT si coincide** — con su respaldo para
  firmware viejo (`#110`/`#111`) y una consulta por fichero desde `#398`. Lo que le falta
  es justo lo que señala Eduardo: hoy pregunta **por la ruta destino** (`/app/Gui.mod`), no
  *«¿lo encontraría el cargador en algún sitio?»*. Si `Gui` vive en un pack o en `/lib`, la
  pregunta por `/app` dice «no está» y se sube igual.
  📌 **Con qué se compara: CRC, no fecha.** Un `.mod` **no lleva marca de tiempo**, así que
  «más antigua» no se puede medir tal cual; y comparar por tamaño ya falló una vez —el
  *skip-if-same-size* de `#110` servía `.mod` rancios—. El mecanismo que funciona y que ya
  está en pie es el CRC, que detecta un rancio venga de donde venga. En la práctica la
  regla queda: **mismo CRC ⇒ no se sube**; distinto ⇒ se sube, porque el IDE es la fuente
  de verdad de lo que quieres ejecutar.
  ⏭️ **Lo que hay que construir, entonces, es poco:**
  1. un verbo de wire que reciba el nombre del módulo y su CRC y conteste
     **sí/no** resolviendo con el cargador (FS por sus rutas + packs montados);
  2. que `putIfChanged` pregunte eso en vez de preguntar por la ruta destino.
  📌 Encaja con la stdlib preinstalada: si la placa trae `Stdlib.pack`, lo normal pasa a
  ser **no copiar nada** y subir sólo el programa.
  `Stdlib.pack`, lo normal pasa a ser **no copiar nada** y subir sólo el programa.

- **[host] PROBAR BASES DE DATOS SIN PLACA — packs en el PC** *(aplazada a V6 el 19-ago.
  Eduardo: «me parece que se sale de V5, habra que dejarlo para V6»)*.
  🩸 **El problema**: la VM-C que usa la gente no puede correr BD — dice *«falta el
  codigo nativo del pack 'SQLI'»*—, asi que hace falta PLACA para probar la mitad de lo
  que V5 añade. Choca con «depura en el PC, despliega en el micro». Los demos si corren,
  pero con `sqldemo.exe`, un binario de pruebas con SQLite enlazado dentro.
  📐 **Lo que ya esta y lo que falta**, medido: el formato de pack y el relocalizador son
  PORTABLES (`src/bpvm_pack.c`, `src/bpvm_npack.c`, en el nucleo comun). Falta:
  1. un `.npk` de **x86-64** — o sea pasar el AOT y el relocalizador por una TERCERA
     arquitectura, con su ABI y sus banderas (lo que costo H4 y H7 en RISC-V);
  2. y **ejecutar codigo realojado en el PC**: en la placa el pack corre desde flash
     mapeada (XIP); aqui habria que reservar memoria ejecutable y saltar a ella. Es una
     pieza NUEVA y especifica del sistema operativo, no un ajuste.
  💡 **El camino que quiza salga mas barato**: que cargue packs el **micro simulado del
  IDE** (V4/H10, `bpvm-sim`), que ya habla wire v1 completo. El usuario probaria sin
  placa y sin binario especial, y de paso el simulador ganaria en fidelidad.
  📌 Mientras tanto, `docs/BASEDATOS.md` tiene que DECIR que hoy la prueba es en placa.

- **[lenguaje] ¿quiere `Map` captadores tipados para sus VALORES?** — cola de los
  captadores de `List`, que se hicieron en V5 (ver «CERRADAS EN V5»). `SyncList` y
  `OwnerList` los heredan gratis por extender `Core.List`; `Map` no, y su caso es
  distinto porque la clave también podría quererlos. **Sin decidir.**

- **[wire] el verbo `RESET` no llega con un RUN vivo** *(era `#452`; aplazada a V6 el 18-ago. Eduardo: «ahora sabemos apañarnos y a los usuarios no les afecta» — el rodeo es `kill` + `reset`, y está documentado cara al usuario en `PENDIENTES.md` L15.)*
  Salió el 18-ago probando `#439`. Durante una ejecución el firmware sólo atiende
  `HELLO` y `KILL`, y a todo lo demás contesta `BUSY`
  (`esp32/main/repl_esp32.c:915`, y el equivalente en las otras familias); el IDE
  refleja eso apagando el botón (`PicoExplorer.java:2180`). El comentario del código
  dice que la intención era *«que la placa nunca quede sorda»* — y casi lo consigue,
  pero deja fuera justo el verbo que hace falta cuando lo que quieres no es recuperar el
  control, sino **releer lo que acaba de pasar**.
  🩸 **Por qué importa más de lo que parece, y es por `#439`**: con la placa colgada, si
  no puedes mandar `RESET` por el wire, la única salida es el RST físico — que en ESP32
  es `power-on` y **borra la RAM del log**. O sea que el mecanismo funciona y aun así no
  lo tienes disponible en el escenario para el que se escribió. Hoy se sortea con
  `kill` + `reset`, que basta porque el `kill` sí llega.
  📌 **ES DE LA IMAGEN, NO DEL IDE.** El botón apagado es sólo el reflejo: tocar el IDE
  a solas encendería un botón que la placa contesta con `BUSY`. El filtro está en el
  firmware y son **CUATRO** sitios, censados por la primitiva (el mensaje) y no por el
  nombre — `pico/repl_v1.c:1406` · `esp32/main/repl_esp32.c:915` (S3 **y** P4, comparten
  REPL) · `stm32/port/stm32_repl.c:467` · y **`tools/bpvm_sim.c:679`**, que es el que se
  escapa si uno cuenta «familias»: el simulador del IDE. Un doble que se comporte
  distinto del original es una trampa, así que va en el mismo lote.
  ⏭️ Meter `RESET` en la lista blanca de ese mismo `if`, en los cuatro, y quitar el
  `&& enabled` de `btnReset` (`PicoExplorer.java:2180`). El `RESET_REPLY` ya se manda
  antes de reiniciar, así que eso no cambia.
  ⚠️ **El cuidado real está en la Pico**: su `handle_reset` hace `log_flush()` antes de
  reiniciar (`repl_v1.c:1229`), y permitirlo durante un RUN significa **escribir flash
  con la VM en marcha** — el peligro clásico de ejecutar desde XIP. La cintura del log
  post-mortem ya lo resuelve, pero hay que comprobarlo, no suponerlo. Las de ESP32 no
  hacen flush (van directas a `esp_restart()`), así que ahí no aplica.

- **[AOT] el `.mdn` no recuerda su RECETA — la huella de los FLAGS** *(mitad abierta de
  `#441`; aplazada a V6 el 18-ago. Eduardo: «ahora no vamos a modificar formatos». La
  otra mitad, la arquitectura, sí entró en V5: `9fcff33`.)*
  🩸 **El caso real que la motivó, el 17-ago**: añadir `-mcmodel=medany` dejó malos
  **todos** los `.mdn` de RISC-V ya generados. Misma arquitectura, misma fecha, código
  inservible — o sea que ni el gate de `arch` ni la comparación de fechas lo ven. Un
  `.mdn` generado con otra receta se sube tan tranquilo y lo que falla es la placa.
  📐 **Qué haría falta**: sellar en el `.mdn` una huella de la receta (un hash de los
  flags de compilación) y compararla al subirlo, igual que ahora se compara `arch`.
  🔴 **Por qué no es un añadido sino un cambio de FORMATO**: la cabecera no tiene campo
  libre — `magic·version·abi_version·code_size·sym_count·arch`, y es **little-endian**
  (al revés que el `.mod`). Meter la huella obliga a subir `version` y a tocar el lector
  del IDE y el de las cuatro imágenes a la vez. Es la clase de cambio que se hace al
  principio de una versión, no al cerrarla.
  💡 **Mientras tanto, lo que hay**: si se vuelven a cambiar los flags de AOT de una
  familia, hay que regenerar sus `.mdn` A MANO y saberlo — no hay red. Conviene
  mencionarlo en el commit que toque `AotBuild`.

- **[P4] los 32 MB de flash y el XIP de los packs** — aplazado a V6 el 18-ago
  (Eduardo: *«es demasiado arriesgado»*). El diagnóstico está CERRADO, lo que
  queda es la obra:
  📐 **El hecho**: el caché de flash del P4 direcciona a 24 bits, así que
  `spi_flash_mmap` rechaza (`ESP_ERR_INVALID_ARG`) toda dirección o tramo por
  encima de **16 MB**. Y el XIP de los packs vive de ese mapeo. Medido en placa
  con DOS repartos: falla por tamaño (19 MB desde 13,3) y por dirección
  (empezando en 25,6). Antes iba porque con `bpdata` de 10 MB todo caía debajo.
  🚫 Saltárselo exige `BOOTLOADER_CACHE_32BIT_ADDR_QUAD_FLASH`, que Espressif
  marca EXPERIMENTAL (*«can't use on all flash chips stable»*). Descartado.
  🔑 **La pieza que lo hace resoluble**: el FS **no mapea** — lee con
  `esp_partition_read`. O sea que el FS puede vivir arriba y sólo los packs
  necesitan estar abajo.
  ⏭️ **Dos caminos, ninguno barato:**
  **A.** Invertir el orden en el común (`bpvm_part_layout_from_sizes`): packs
  primero con tamaño ajustable, FS al final llevándose el resto. Es el modelo
  correcto —*dices cuánto para packs, el FS se queda lo demás*— pero el orden lo
  comparten las TRES familias: el FS se mueve en todas ⇒ **reformatear las tres**.
  **B.** Dar al P4 una segunda partición (`bppacks` abajo, `bpdata` arriba sólo
  FS). No toca a las otras, pero el firmware busca UNA partición y la reparte él:
  hay que enseñarle a usar dos.
  ✅ **PROBADO EN PLACA el 18-ago y DECIDIDO: se vuelve a 16 MB en V5.** Eduardo:
  *«se prueba y se decide… tampoco cuesta tanto probarlo»* — y con razón, porque yo
  ya había fallado una vez con esto (dije que era el TAMAÑO y su prueba con 6.528 KB
  lo desmintió). Así que en vez de discutir con el fuente del IDF, se instrumentó el
  arranque para que lo dijera la placa, y lo dijo:
  ```
  pack: fisica 0x19a0000..0x2000000 (26240..32768 KB) | limite del cache 24 bits
        = 0x1000000 (16384 KB)  <<< EMPIEZA POR ENCIMA  <<< ACABA POR ENCIMA
  ```
  Los dos extremos fuera. Con `bpdata` a 32 MB los packs **no mapean nunca**, así que
  el P4 se quedaría sin packs ni SQLite (lo que cerró H7) — y por el criterio del
  propio Eduardo (*«si funciona se queda así»*) se revierte.
  📌 Lo que SÍ se queda: la línea de diagnóstico. Cualquiera que mueva las
  particiones verá al arrancar si se ha salido del rango mapeable, en vez de un
  `err=258` que no explica nada.
  ⚠️ Al volver a 16 MB, **el ENV tiene que caber**: si quedó en FS=20.000 KB, el
  arranque se queda DEGRADADO (lo dice, no es un ladrillo). El valor que funcionaba
  era **FS = 7.344 KB**, que deja 2.800 para packs.


- **[P4] el silicio nuevo (ESP32-P4X) pedirá lo suyo** — aviso de Eduardo (20-ago):
  *«las placas que tenemos con P4 son ESP32P4, y hay algún problema eléctrico así que
  funcionan a 360 MHz en vez de los 400 previstos. Hay una versión ESP32P4X que será la
  buena. Pediré una placa con el micro actualizado y tendremos que hacer una imagen para
  él, porque algunas opciones de IDF cambian de un micro a otro.»*
  📐 **Lo que hay hoy, leído del `sdkconfig` y no supuesto:**
  - `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=360`, con el de **400 explícitamente desactivado**.
  - `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` y `CONFIG_ESP32P4_REV_MIN_0=y`, **las dos en
    `sdkconfig.defaults`** — o sea versionadas y deliberadas: la imagen de hoy está
    clavada a silicio *anterior a la v3*.
  - `CONFIG_ESP32P4_REV_MAX_FULL=199`: acepta de la revisión 0.0 a la 1.99.
  ⛔ **Son DOS imágenes por narices, y lo dice el propio IDF en la primera línea del
  Kconfig del P4:** *«Support of ESP32-P4 rev. <3.0 and >=3.0 is mutually exclusive»*, y la
  ayuda del interruptor remata: *«huge hardware difference… not compatible»*. Verificado el
  20-ago en `esp_hw_support/port/esp32p4/Kconfig.hw_support` de la v6.0.1.
  📐 **Qué es de verdad `ESP32P4_SELECTS_REV_LESS_V3`** — no un rango que se pueda
  ensanchar, sino un interruptor que parte el IDF en dos mitades excluyentes:
  - con `=y` el mínimo sólo puede ser **0.0 / 0.1 / 1.0**; con `=n`, sólo **3.0 / 3.1**.
    No hay ajuste que cubra los dos silicios.
  - el `REV_MAX_FULL` **no se elige: sale de él** (199 con `=y`, 399 con `=n`). El 199 que
    tenemos es consecuencia, no decisión — por ahí no se toca.
  - y arrastra código real, no sólo la comprobación de arranque: **el reloj del propio
    bootloader** (90 MHz con `=y`, 100 con `=n`), el Key Manager del cifrado de flash, el
    VBAT y el apagado de CPU en light sleep.
  🔑 **El detalle que ahorra trabajo: el DEFAULT del IDF es `n`** (mínimo 3.1). O sea que
  la imagen del P4X no hay que «configurarla»: se consigue **quitando** de
  `sdkconfig.defaults` las dos líneas que hoy la clavan al silicio viejo y dejando mandar
  al default. Lo que está personalizado es la imagen de AHORA, no la futura.
  ⚠️ **La trampa, que ya mordió:** `sdkconfig.defaults` **sólo siembra el `sdkconfig` la
  primera vez**. Cambiar los defaults sin borrar el `sdkconfig` que ya existe deja el
  firmware EXACTAMENTE IGUAL, y no lo avisa nadie — pasó el 18-ago.
  🔑 **Por qué 360 y no 400 — confirmado por Eduardo (20-ago):** no es una cifra tímida
  ni un resto de nada, es un RODEO a un defecto del silicio. A 400 MHz estas placas dan
  **problemas de consumo y no funcionan bien**; a 360 van finas.
  ⛔ Con lo cual **subirlas a 400 no es una mejora pendiente: es volver a romperlas.**
  Queda escrito aquí porque el `sdkconfig` sólo dice *360* y un 400 desactivado y sin
  explicación al lado invita a que alguien lo «arregle» de buena fe.
  ✅ **Lo que el P4X cambia**: allí está corregido y SÍ puede trabajar a 400. Así que las
  dos imágenes se diferenciarán en TRES cosas, no en una: la revisión de silicio, la
  frecuencia (360 vs 400) y todo lo que el interruptor arrastra por debajo.
  🧭 **Y la política: se MANTIENEN LAS DOS.** Eduardo, 20-ago: *«aunque el P4 queda
  obsoleto, todavía quedan muchas placas que se están comercializando actualmente; en
  cambio del P4X, que será el bueno, hoy apenas hay placas.»* O sea que esto **no es una
  migración** con fecha de caducidad: durante V6 el silicio viejo es el que la gente
  tiene y compra, y el nuevo el que casi nadie ha visto. La imagen del P4X se añade, no
  sustituye.
  📦 **Consecuencia para la distribución**: el ZIP pasará a llevar **dos imágenes de P4**,
  y ahí el nombre es lo único que separa al usuario de flashear la que no es. Hay
  precedente de que un artefacto se cuele en la placa equivocada, así que los nombres
  tienen que decir el silicio, no la familia.
  🛡️ **La red ya existe, y es del propio IDF — verificado el 20-ago** en
  `bootloader_support/src/bootloader_common_loader.c`: el bootloader compara la revisión
  grabada en efuses contra la de la imagen y **rechaza en las DOS direcciones**, con el
  mensaje `chip revision check failed. Required >= vX.Y / <= vX.Y, found vX.Y`. La
  imagen vieja en un P4X falla por el máximo; la del P4X en un P4 viejo, por el mínimo.
  ✅ Con lo cual **la imagen equivocada no arranca a medias ni corrompe nada: se planta y
  dice por qué**. No hay que construir ninguna comprobación propia — basta con nombrarlas
  bien y decirlo en la documentación.
  📌 **Y en la FORMA**, un matiz que conviene no confundir: `esp32/` y `esp32p4/` son
  carpetas distintas porque son *targets* distintos del IDF; el P4 y el P4X, en cambio,
  son **el MISMO target** y lo que los separa es este interruptor. Aun así se resuelve
  igual —otra carpeta de build con su `sdkconfig.defaults`—, nunca con macros repartidas
  por el código: aquí cambia el SILICIO. Lo que cambia por placa sigue yendo al ENV.

- **[IDE] el árbol de ficheros, por COLOR según el tipo** — encargo de Eduardo
  (18-ago). Cada extensión conocida con su color (`.mod`, `.mdn`, `.fon`,
  `.bin`…), y **el ROJO queda RESERVADO** para ficheros con algún problema.
  Esa reserva es lo importante del encargo: si el rojo se gasta en un tipo,
  luego no queda color para lo que de verdad hay que mirar.
  📍 Dónde: `PicoExplorer.java:200`, el `DefaultTreeCellRenderer` del árbol, que
  hoy ya pone el label y el icono y **no usa color para nada** — el rojo está
  libre, así que la reserva se puede respetar desde el primer día.
  📐 Lo que hace falta decidir al hacerlo (no ahora): qué cuenta como
  *«problema»* para ganarse el rojo. Candidatos que el IDE **ya sabe** hoy y hoy
  no enseña en el árbol: un `.mdn` cuya arquitectura no es la de la placa
  (`#441`), un `.mod` de ABI incompatible (el gate de `#284`), un `/lib` rancio
  (el chivato de `#422`) y un listado truncado (`#425`, que hoy avisa aparte).
  Ojo con el daltonismo: el color como ÚNICO canal deja fuera a mucha gente —
  conviene que el rojo lleve además icono o marca.
  ✅ **PRESUPUESTADO el 26-ago** (Eduardo: *«si no es muy complicado lo podemos
  probar a hacer a la vuelta»*). Mirado el sitio: **un solo renderer, un solo
  fichero, sin tocar wire ni firmware**. Ya calcula el label y pone el icono;
  falta un `setForeground(...)` por extensión. `Backend.Entry` trae `name` y
  `size`, o sea que **el color por TIPO sale con lo que ya hay**.
  ✅ **PASO 1 HECHO Y VISTO EN PANTALLA (26-ago · `9cc33ee6` + ajuste)**: cinco
  familias SEMÁNTICAS —`.mod` azul, `.mdn` ciruela, `.pack` ocre, **recursos en
  verde** (fuentes, imágenes, `.win`, binarios), texto/config gris— y
  directorios y desconocidos en negro. **El rojo, sin gastar.**
  📐 **La corrección que enseñó el criterio** (Eduardo, en pantalla): la primera
  paleta salió tan apagada que *«el azul se ve poco, cuesta distinguirlos»*. El
  error de medida fue mío y es instructivo: yo comprobaba el contraste contra el
  FONDO BLANCO (iba sobrado, 10:1) cuando lo que importa es el contraste contra
  el **NEGRO del texto normal**, que es de lo que hay que despegarse para que el
  color signifique algo. El azul viejo estaba a 2,2× del negro; los de ahora,
  entre 3,0 y 4,2× — y siguen por encima de 5:1 sobre blanco. Mate, pero legible.
  ⏭️ **Va en dos pasos, y el primero es el encargo entero**:
  1. colores por extensión, con **el rojo RESERVADO y sin usar** — eso ya
     cumple lo pedido y no espera a nada;
  2. el rojo, después, según el IDE sepa señalar cada problema (los cuatro
     candidatos de arriba necesitan datos que el árbol hoy no pide).


- `#434` — **desacoplar los eventos del lazo de LVGL** (idea de Eduardo, 17-ago,
  al cerrar `#424`). Hoy un clic tiene que ATRAVESAR el lazo de BP para llegar a
  su handler: el upcall lo encola y sólo se drena entre quanta, y el único punto
  de quantum es la vuelta de `Gui.run()`. O sea que **el evento no avanza
  mientras el bombeo duerme**. El mecanismo es IDÉNTICO en las dos familias; lo
  que cambia es el grano del sueño — 10 ms en el P4 (`CONFIG_FREERTOS_HZ=100`)
  contra 1 ms en el STM32 (`__WFI` + SysTick). Un factor diez sobre la misma
  forma. Desacoplarlos quita la dependencia del ritmo del lazo en TODAS las
  familias, en vez de ajustar un número por placa.
  Antes de diseñar nada, dos medidas: **instrumentar el STM32 igual que el P4**
  (sus cifras están leídas del código, no medidas) y cronometrar el camino
  clic → handler por separado del camino invalidar → pintar. Palancas conocidas
  y ya descartadas como parche: el tope del lazo (hecho, 50→10) y el periodo del
  `indev` de LVGL (40→10 ms; costaría pasar de ~3 % a ~12-15 % de un núcleo).
  Emparenta con [[#432]] en lo de fondo: el reparto común/hardware de V6.

- `#432` — **¿dónde debe vivir la tabla de handles, y de qué tamaño?** Las dos
  preguntas que dejó `#430` (Eduardo, 17-ago). **Están acopladas: la segunda
  depende de la primera**, y conviene decidirlas juntas.

  **(a) ¿Se queda en el malloc de plataforma (SRAM) o se muda a la zona del
  heap?** Hoy sale de `bpvm_realloc` → SRAM, mientras los objetos que indexa
  viven en PSRAM: la tabla escala con el número de objetos, pero se paga de un
  presupuesto que no escala con ellos. Mudarla parece lo coherente, pero hay
  tres cosas que mirar antes:
  - ⚠️ **Es la estructura MÁS CALIENTE de la VM**: cada `bpref_deref` toca
    `handle_addr[idx]`. Moverla a PSRAM la mete en la memoria lenta. Esto se
    **mide** (derefs/segundo antes y después) — es exactamente el tipo de
    mejora que sale cara sin avisar. Cabe una tercera vía: `handle_gen` (frío,
    sólo en validación y GC) fuera y `handle_addr` (caliente) en SRAM.
  - ⚠️ **La zona ya tiene otro inquilino**: SQLite reserva de ahí
    (`bd: reservada (SQLite=2) -> 2048 KB @ 0x11000000`). Si la tabla también
    tira de ella, hay que decidir el reparto — y quién cede cuando no cabe.
  - `bpvm_arena_reserve` ya talla de esa región, pero es de **un solo uso** y
    la tabla **crece**. O se preasigna el máximo (y entonces el tamaño hay que
    acertarlo, ver (b)), o la región tiene que poder crecer, y eso toca los
    límites que usa `is_heap_ref`.

  **(b) ¿El tamaño debería salir del heap?** Hoy el arranque (4096) y el tope
  (16384 en la Pico) son constantes, y el tope está puesto **por la SRAM**. Pero
  la NECESIDAD sale del heap: con 5,6 MB y objetos de ~24 B caben ~230.000
  objetos vivos, catorce veces el tope. Consecuencia real: un programa legítimo
  puede recibir OOM **con heap libre**. (Ya no cuelga —eso lo arregló `#430`—
  pero sigue estando mal.) Lo natural sería derivarlo del heap, como ya hace
  `gc_bump_threshold` (`(stack_base - heap_start) / 8`)… y ahí está el nudo:
  **mientras la tabla se pague en SRAM, el tope no PUEDE escalar con el heap**,
  porque el presupuesto no escala. Resolver (a) es lo que desbloquea (b).

  **Lo que hace falta medir antes de decidir** (encaja con el censo funcional de
  V6, eje «memoria y tiempos»): el coste real de un deref en PSRAM vs SRAM, y
  cuántos objetos vivos a la vez llega a tener un programa de verdad — si nadie
  se acerca a 16384, el problema es teórico y la respuesta es «déjala donde
  está»; si un ORM con muchas filas lo roza, es urgente. Ver
  [[tabla-handles-sram-y-presion-430]].

- `#378` — que cada micro **DIGA lo que tiene** (capa HAL BP de capacidades).

- (sin número) — el **tamaño de flash lo dice la placa**: tabla grande + clamp, no
  una imagen por tamaño.

- (sin número) — **la S3 no tiene `bios_s3.c`**: no ofrece tabla BIOS, así que no
  puede alojar un pack nativo. Familia por hacer, no prueba pendiente.

- `[V6]` `Object` = comodín por referencia — decidido y diseñado en
  `docs/OBJECT_COMODIN.md`. **Ojo**: estaba clasificado V6 y su mitad estática se
  hizo en V5 el 14-ago. Falta la **clase contenedora** (nombre sin decidir, si
  distingue vacío de `null`, y cómo se saca un escalar).

- (sin número) — **liberación de recursos**: destructor `~Clase()` + `var owner` +
  bloque. Diseño de Eduardo.

- `#396` — módulo `Time` con clase `Time.Date`, sobre un `long` de segundos de
  época — **no** un tipo del lenguaje.

- (sin número) — librería `Math`: ampliar (`fact` sobrecargada, f64) **y repasar
  lo que ya hay**; strings igual si hace falta.

- (sin número) — **diagnóstico del heap DESDE BP**: las herramientas existen, pero
  están en la VM equivocada.

- (sin número) — **muro de contención** entre el heap y las pilas. Idea de Eduardo.

- `[ISA]` — `CALL_REL`: CALL local PC-relativo, el modelo de Eduardo.

- `#19` — array fijo LOCAL: el UAF ya está cerrado (`b99529e`); queda **sólo el
  inline por eficiencia**.

- `#356` — REBAJADO: la pérdida de bytes no se manifiesta (era colateral de #357);
  queda **el descarte mudo**, latente.

- (sin número) — **librería de placa GENÉRICA**: el micro da el dato, la librería
  hace de puente.

- (sin número) — **batería de rendimiento HW+SW**: medir el REPARTO, no el tiempo.

- (sin número) — **prueba de resistencia larga**: días de carga VARIADA, con marca
  periódica en el log-anillo para que «la muerte deje rastro».

- `[SIN VERSIÓN, V6+]` — Linux: el IDE en Linux + la Raspberry Pi como PLACA.

- (de H6) — `SD_INFO` y `SD_MOUNT` siguen sólo en `pico/repl_v1.c`; no han subido
  a código común. Verificado el 14-ago: siguen ahí.

- `#426` `[V6]` — **`double` en una función `native`. APLAZADO A V6 por decisión
  de Eduardo (16-ago)**, y no por coste sino porque *falta pensar el diseño*:
  *«los micros como los STM32F7 tienen coprocesador que soporta float y double.
  Lo correcto sería: si el micro soporta double por hardware, por hardware; si
  no, por software. Quizás lo mejor sería meter las funciones de coma flotante
  en la BIOS o en opcodes»*.
  Eso reencuadra la ficha entera: **no es «cómo meto libgcc en el .mdn», es
  «quién provee la coma flotante y cómo lo dice cada placa»** — que es la misma
  pregunta que `#378` (que cada micro DIGA lo que tiene). Hacerlo ahora por
  helpers sería resolver el caso pequeño y cerrar la puerta al bueno.
  ⚠️ Ojo al dato que lo motiva: la FPU del Cortex-M33 (RP2350, STM32U5) es de
  **precisión simple**, pero la del **STM32F7 es de doble** — o sea que la
  respuesta correcta DEPENDE DE LA PLACA, y por eso no puede ser una constante
  en el emisor.
  *(Lo demás, tal como estaba.)* **`double` en una función `native`** (sale de `#381`, que los tenía
  juntos). No le falta el marshalling —ése ya está hecho y es el mismo—: le
  falta que la aritmética de coma flotante, que estos micros **emulan por
  software**, sea alcanzable desde un `.mdn`. Hoy deja seis símbolos de libgcc
  sin resolver (`__aeabi_dadd`, `__aeabi_dmul`, `__aeabi_ddiv`, `__aeabi_dcmplt`,
  `__aeabi_i2d`…) y el empaquetador lo rechaza.
  El camino es el mismo que funcionó para la división de `long` —helpers en la
  tabla—, pero aquí serían MUCHAS operaciones y se paga una llamada indirecta
  por cada una: hay que **medir si sale a cuenta** antes de escribirlo.
  ⚠️ Y el aviso de fondo de Eduardo, que sigue en pie (`AOT_LIMITES.md` §1): la
  FPU de estos micros es de **precisión simple**, así que un `double` no toca la
  FPU ni compilado. Marcar `native` una función con `double` es pedir velocidad
  y elegir el camino lento a la vez. Riesgo añadido: la paridad de coma flotante
  (contracción `a*b+c`) — ver `GAP-4`.
  Análisis completo en `docs/AOT_ABI8_IDEAS.md`.

---


### ═══ V6 — lo que viene de V5 y AHORA ES DE V6 ═══

> 🧹 **TRIAJE del 26-ago-2026.** Esta cola tenía **59 fichas y 47 ya estaban cerradas**
> —con su ✅ y su commit— pero seguían aquí dentro: la lista de pendientes **mentía al
> alza**. Las 47 se movieron íntegras al **📦 ARCHIVO** del final de este documento
> (ni una línea borrada: comprobado comparando contra git, 0 líneas de contenido perdidas).
>
> De las **12 supervivientes**, verificadas **contra el código y git, no de memoria**:
> una estaba cerrada sin marcar (el `native` en método, que resolvió N1.1) y otra tenía el
> enunciado equivocado (los packs del S3: la región SÍ existe). **Quedan 9 vivas** (dos H2-P5 más llevaban «CERRADO»/«ya no aplica» en su propio texto sin tachar).
>
> 📌 Es [[no-acumular-pendientes]] aplicado a la propia libreta. En V5 esto costó tiempo
> real: dos bugs cerrados que seguían apareciendo como abiertos.

> **Decisión de Eduardo (23-ago):** *«lo que haya de V5 que quedó pendiente pasa a V6 y
> deja de ser de V5»*. Así que esto **no es un limbo ni una lista de espera**: son fichas
> de V6 como las de arriba. Lo único que las distingue es de dónde vienen, y eso se
> conserva porque el contexto de un bug incluye cuándo salió.
>
> Los títulos de sección siguen nombrando hitos de V5 (`V5/H10`, «la cola de H2») **a
> propósito**: dicen la procedencia, no el hito al que pertenecen.

### IDE — V5/H10 «lo pendiente que no son bugs»

### Módulos y arranque (nuevas del 14-ago, en placa)

- **🐛 [ESP32 y STM32] un módulo rancio sobrevive y NADIE lo dice — y `/app` es el punto ciego de los DOS** — encontrado el
  22-ago comparando las tres familias. **No es regresión de V5**: es así desde que existe
  el mecanismo.
  📐 **El hecho**: `esp32_mods_install` (`esp32/main/esp32_mods.c:4840`) instala un módulo
  **sólo si NO existe** (`if (bpvm_fs_stat(...) != 0)`). Un `Core.mod` de un firmware
  anterior se queda ahí indefinidamente. Y a diferencia del RP2350, el ESP32 **no tiene el
  aviso** `NO es el de esta imagen` — ése vive sólo en `pico/main.c`.
  🩸 **Lo que le llega al usuario**: `exit 11` con *«lib 'Core' presente pero no exporta
  'Core.__cls_new_List'; ¿versión vieja?»* — y ni una pista de que la causa esté en `/lib`
  ni de que borrarlo lo arregle. En la Pico lo dice la placa al arrancar; aquí no.
  📌 **La POLÍTICA no es el problema y no hay que cambiarla.** Su comentario la justifica:
  *«No sobreescribas si ya está (p.ej. el usuario subió una versión)»*, que es razonable —
  respetar lo que el usuario puso a mano. Lo que falta es **decirlo**.
  ⏭️ **El arreglo barato**: comparar el tamaño del fichero con el del blob embebido y
  emitir la misma línea que la Pico. No lee el fichero entero (la Pico ya resolvió eso en
  `H11`: preguntar sólo si EXISTE costaba menos que leerlo). Es el mismo criterio de «que
  el desfase GRITE» que el proyecto aplica en el gate del `.mod`, el `magic` de la BIOS, el
  sello del `.npk` y la marca del punto de encuentro.
  🩸 **AMPLIADO el 22-ago, con el Nucleo delante**: el problema **no es sólo del ESP32 ni
  sólo de `/lib`**. El IDE sube las dependencias del programa a **`/app`**, y lo que hay
  ahí **tapa** a lo de `/lib`. El STM32 vacía `/lib` en cada arranque pero **no toca
  `/app`**, así que un `Core.mod` rancio ahí sobrevive a todo — y eso es exactamente lo que
  dio el `exit 11` en el Nucleo con `/lib` recién reembebido. Lo encontró Eduardo con
  `dir /app`.
  ✅ **El RP2350 es la única familia con la red completa**: su comprobación recorre una
  tabla que incluye las dos carpetas, y por eso en la Metro salía
  `lib: /app/Hello.mod NO es el de esta imagen` (`pico/main.c:1323`).
  🔗 **Y emparenta con la ficha de V6** *«NO copiar dependencias que el dispositivo YA
  TIENE»*: si el IDE no dejara copias en `/app`, no habría nada que envejecer. Esa ficha
  deja de ser sólo una optimización de tiempo de subida.
  ⚖️ **Y hay una TERCERA vía ya escrita en el propio proyecto**: el STM32 vacía `/lib` y lo
  reembebe fresco cada arranque (*«evita stdlib rancia tras actualizar el firmware»*,
  `fs_lfs_stm32.c:196`). Se autocura, a cambio de desgaste de flash. **Tres familias, tres
  estrategias, ninguna decidida como LA buena** — eso es material del eje común/hardware de
  V6 (`#427`), no de un parche suelto.

### Familias — lo que dejó el censo (`#427`)

- **🕳️ [S3] los packs: encaminados pero SIN REGIÓN — el mismo agujero que `#327` en la
  ⚠️ **Matizado en el triaje (26-ago), porque el enunciado despista**: el S3 **sí tiene `bpdata`** (`partitions.csv`: 0x118000, ~15 MB) — la región existe desde que se arregló la tabla que acababa en 2 MB. Lo que falta es el **reparto FS|Packs dentro de bpdata** (el «mando» del env) y el camino de grabación. Medir antes de tocar: no es «no hay región», es «nadie le ha dado su mitad».
  Pico, ahora en el S3** (visto el 22-ago probando H13).
  📐 **El hecho**: no existe `esp32/main/pack_s3.c`. El único que llama a
  `board_mgr_esp32_set_packs_view()` es `esp32p4/main/pack_p4.c`, así que en el S3
  `s_packs_view` es NULL para siempre → `bm.packs_*` se queda a cero → la placa contesta
  *«la placa no expone packs (configura las particiones primero)»*.
  🩸 **Y el mensaje ENGAÑA, igual que engañó en la Pico**: invita a tocar las particiones,
  y en el S3 eso **no puede** arreglarlo. El comentario de `#327` en `board_mgr_pico.c:41`
  ya lo decía con estas palabras: *«el LS contestaba —correctamente— "esta placa no expone
  packs", **hiciera Eduardo lo que hiciera con las particiones**»*.
  📌 **Y la frase de `#327` que define el patrón**: *«Encaminar los comandos y no dar la
  zona es media función»*. Entonces el hueco era la Pico y lo tenían el STM32 y el
  simulado; hoy lo tienen las otras cuatro y el hueco es el S3. Es
  [[arreglo-que-no-viaja-entre-familias]] otra vez: el común creció y una copia privada
  no.
  ✅ **No es regresión de V5 ni bloquea**: la tabla de la Puerta 2 nunca listó packs para
  el S3. Pero conviene DECIRLO, porque hoy sólo se descubre intentándolo y el mensaje
  manda a otro sitio.
  ⏭️ **Dos trabajos, separables**:
  1. **Barato y honesto**: que la placa distinga *«esta familia no tiene zona de packs»* de
     *«hay zona pero no está configurada»*. Hoy las dos dan la misma frase y una de ellas
     es un consejo imposible. Es el mismo criterio que ya se aplicó al `BAD_ALIGN` y al
     `exit 11`: **que el mensaje diga qué encontró, no lo que supone**.
  2. **El de fondo**: escribir `pack_s3.c`. La flash del S3 son 16 MB y la zona caería por
     debajo del límite del caché, así que no tiene el problema del P4 con los 32 MB —
     debería ser un port directo del mapeo del P4.

### Placas y hardware

- **🐛🔴 [placa] un estado persistente deja la Metro SIN PODER EJECUTAR NADA, y sólo lo
  cura REPARTICIONAR** — visto el 21-ago al final de la sesión de H13. **Sin causa
  identificada**; se ficha con la cronología porque el rodeo no es evidente y a un
  usuario le puede pasar.
  📐 **El síntoma**: no corre **ningún** programa. Ni los de la sesión, ni el
  `/app/Hello.mod` que preinstala el propio firmware —que no usa strings, ni builtins,
  ni un solo `import`—. Se cuelga mudo, sin línea de error.
  🔍 **Lo que DESCARTA el firmware, y es el dato que más vale**: se volvió a flashear la
  imagen del 20-ago —la misma que esa mañana había corrido `ListGets`, `SqlDemo`,
  `DaoDemo`, `GenDemo`, `SdDoc`, `SdCard` y `Bench` sin una queja— **y tampoco
  funcionaba**. Con lo cual lo roto sobrevive al flasheo: es FS, ENV o zona de packs.
  ⚠️ **Y formatear NO bastó.** Se formateó (FS + zona de packs) y siguió igual. Lo único
  que lo curó fue **cambiar el tamaño de la partición**, que fuerza a rehacer el reparto
  entero. Es la SEGUNDA vez en el mismo día que reparticionar arregla algo.
  📋 **Cronología, por si la pista está aquí** (todo entre «funcionaba» y «no
  funciona»): formatear FS+packs → regrabar `SQLite.pack` (1,13 MB por streaming) →
  reformatear la SD a exFAT → reiniciar → reformatear la SD a FAT32 → flashear imagen
  nueva → flashear la de ayer.
  🔬 **LO QUE FALTA MEDIR, y no se capturó**: el log **durante el intento de ejecución**.
  Si sale la línea `RUN/v1 /app/X.mod session=N` el programa llega a lanzarse y se
  atasca dentro; si no sale, no llega ni a arrancar. Eso parte el problema en dos y sin
  ello sólo se puede especular. **Si vuelve a pasar, lo PRIMERO es ese log.**
  ⏭️ Sospechoso natural para empezar: qué toca un cambio de tamaño de partición que un
  formateo NO toca. Ahí está la diferencia entre lo que curó y lo que no.

- `#379` — el wire se **desincroniza tras el Stop**, y sólo en unas placas.
  🔎 **HIPÓTESIS FUERTE (17-ago): esto era `#398`, no una desincronización.**
  Eduardo, al proponerle repetir la prueba en el P4: *«lo de la P4 con SD con
  comportamiento extraño era ANTES de que se solucionara el problema del refresco
  del árbol»*. Y con eso encajan TODOS los síntomas sin necesidad de que el wire
  se desordene — bastaba con preguntar mientras el device estaba ocupado:
  antes de `#398`, un refresco con tarjeta tenía al firmware **siete segundos**
  calculando el CRC de cada fichero.
  · «timeout esperando `INFO`» → el device no atendía, estaba listando.
  · «se recupera solo» → en cuanto acaba el listado, contesta.
  · «sólo en unas placas» → las que tienen SD, donde el listado era lento.
  · «el AOT no sabe la arquitectura» → el `arch` viaja en ESE `INFO`.
  📐 **Medido el 17-ago: la Metro, 5 de 5 ciclos `run`→`stop`→`Info` limpios**
  (uptimes 994/1094/1146/1177/1214 s: son cinco de verdad, no la misma respuesta
  repetida). Consistente con la hipótesis.
  ⏭️ **La prueba que decide: los mismos 5 ciclos en el P4 CON la tarjeta**, que
  era la combinación donde peor se veía. 5/5 → la explicación se sostiene y esta
  ficha se cierra como absorbida por `#398`. Un fallo → la hipótesis es mala y hay
  una desincronización de verdad que buscar.
  🔴 **17-ago, en el P4: el Stop CUELGA.** Eduardo, probando esto mismo: *«he
  hecho stop y se cuelga (y como se cuelga no hay log)»*. O sea que la hipótesis
  de absorción **NO se sostiene tal cual**: un Stop que cuelga no es «el firmware
  estaba ocupado listando». La ficha sigue ABIERTA.
  ⏭️ Lo siguiente cuando se retome, y en este orden (no se hizo, se paró aquí):
  1. **¿está colgado el device o el IDE?** Con el cuelgue puesto, pedir `Info` por
     la consola. Contesta → el device está sano y lo colgado es el IDE. Timeout →
     el device está parado. Es la misma pregunta que separa «tarda más que su
     timeout» de «se pierde», y sigue sin hacerse.
  2. **qué estaba corriendo**: con `native`/`.mdn` o interpretado. Importa porque la
     VM sólo mira el Stop **dentro del bucle del intérprete**
     (`interp.c:676`, al principio de cada instrucción): mientras corre código
     nativo del `.mdn` no hay dónde verlo.
  3. si se recupera solo o hay que desenchufar.
  Síntoma conocido: timeout esperando respuesta a `INFO`, y se recupera solo.
  🔗 **Puede ser la raíz de más cosas de las que parece** (Eduardo, 15-ago: *«la
  P4 con la SD funciona raro, yo creo que puede venir de ese comportamiento
  extraño»*):
  - el aviso del AOT *«no sé la arquitectura del dispositivo — desconecta y
    vuelve a conectar»* sale cuando el IDE no logra el `arch`, **y el `arch` se
    pide por el INFO**, que es justo el verbo que esta ficha dice que se pierde.
    `98183ef` dejó de fiarse del valor cacheado *para grabar* por este motivo,
    pero el camino del **Run** sigue cogiéndolo de la caché: con un programa que
    tenga funciones `native`, eso significa ejecutarlas INTERPRETADAS sin que
    nadie lo pida.
  - y lo raro de la SD en el P4, que Eduardo tiene pendiente de revisar.
  ⚠️ La medida que separa las hipótesis sigue siendo la misma y no se ha hecho:
  **cronometrar la respuesta en el FIRMWARE, no en el IDE**. «Tarda más que su
  timeout» y «se pierde» son dos fallos distintos y desde fuera se ven igual.
- `#408` — medir **los dos cuellos** que se ven comparando P4 y Metro (árbol en la
  P4 / formateo en la Metro).
- **`#408` — la mitad de la METRO, MEDIDA el 21-ago: el formateo son ~15 s**, y es el
  de la **zona de packs** (4,2 MB en esta placa) → unos **280 KB/s de borrado de
  flash**, que para un RP2350 es lo esperable. 📌 Anotado como *explicado*, no como
  pendiente: nadie tiene que perseguirlo pensando que es un fallo. Borrar 4,2 MB cuesta
  eso.
  ✅ **Y LA OTRA MITAD, MEDIDA el 22-ago en el P4 — `#408` QUEDA CERRADA.** El árbol
  **no es un cuello**: `ls: 23 ent en 145 ms | app:1/0ms lib:14/61ms sd:8/36ms`. 145 ms
  para 23 entradas, y encima **desglosado por carpeta**, que era justo lo que la ficha
  pedía para poder distinguir quién tarda. El grueso se lo lleva `/lib` (14 entradas en
  61 ms) y la SD (8 en 36 ms); `/app` es instantáneo.
  📌 **Las dos mitades, entonces, tienen respuestas opuestas y las dos son buenas
  noticias**: el formateo tarda lo que cuesta borrar flash (explicado, no patológico) y el
  árbol ya no tarda —lo arregló `#424` el 17-ago— así que no queda nada que optimizar.
  Cerrar una ficha midiendo y descubriendo que no hay problema también es cerrarla.
  🔎 **Y de camino salió esto**: `save` responde «FS guardado en flash» y nada más, pero
  el firmware **sí se cronometra** y manda `durationMs` en el `SAVE_REPLY`
  (`pico/repl_v1.c:929-943`). **El dato viaja por el wire y el IDE lo descarta.**
  Enseñarlo no cuesta nada y es exactamente la medida que esta ficha pedía. No es un
  bug, así que con el freeze va a V6 → ver «Aplazadas».
### Pulido (no urgente) — subidos desde `PENDIENTES` el 17-ago

> Estaban en `PENDIENTES.md`, que es documentación de cara al usuario. Tienen estado de
> trabajo («hay que hacer X»), así que su sitio es éste.

### Lenguaje y VM

- **🐛 [compilador] la sustitución por LSP entre interfaces de módulo NO funciona** —
  encontrado el 21-ago censando los 277 `.bp` del ZIP. **Dos samples publicados que no
  compilan**, y no por V5: nada que ver con `any`→`Object`.
  📐 **El caso**, que el propio sample explica en su cabecera: `appv1lsp.bp` importa
  `com.example.LogApi:BufferedLogger`; `BufferedLogger` declara
  `implements com.example.LogApiV2`; y `logapiv2.bp` dice
  `module interface LogApiV2 extends com.example.LogApi`. Transitivamente lo cumple, y
  el comentario del código lo da por bueno:
  *«El impl puede implementar la interfaz pedida directamente o cualquier descendiente
  de ella (subinterfaz)»* (`Main.java:1958`). Pero el compilador lo rechaza:
  ```
  error: 'BufferedLogger' no implementa 'com.example.LogApi' (directa o
  transitivamente; declara com.example.LogApiV2)
  ```
  Y `appv2.bp` cae por lo mismo visto del otro lado: *«el módulo importado 'LogApiV2'
  no expone 'log' / 'level' / 'VERSION'»* — que son los miembros que HEREDA de `LogApi`.
  🔍 **Hasta dónde llegué (21-ago)**: el algoritmo de `implSatisfies`
  (`Main.java:1357`) es correcto en forma —sube `implements` → `extends` hasta dar con
  la pedida— así que falla el dato, no la lógica. Y el dato falta porque **una
  `module interface` pura no genera `.mod`**: compilado el conjunto como proyecto, sale
  `com.example.BufferedLogger.mod` y **no** `com.example.LogApiV2.mod`. Sin artefacto,
  la cadena sólo puede recorrerse recompilando la interfaz desde el fuente en modo
  `INTERFACE_ONLY`… que es **la pasada con el bug ya aparcado** («la pasada
  interfaz-only tira firmas»). Muy probablemente son el mismo problema visto dos veces.
  📌 **Por qué no se arregló al encontrarlo**: code freeze, y esto **no es regresión de
  V5** — es del subsistema de interfaces de módulo, que es delicado. Se ficha y lo
  decide Eduardo.
  ⚠️ **Mientras tanto**: `appv1lsp.bp` y `appv2.bp` viajan en el ZIP y no compilan. O se
  arregla, o se sacan de la distribución, o se mueven a `samples/errores/` con una nota
  de que hoy no va. **No se publica sin elegir una de las tres.**

### AOT / native

- ~~**🐛 [AOT] `native` en un MÉTODO se ignora en SILENCIO**~~ — ✅ **CERRADA (triaje del 26-ago): la resolvió V6/N1.1** (`631f55fa`), y con test (`AotNativeEnMetodoTest`). El emisor APLANA el método a función con `this` de primer parámetro. Y el 24-ago se cerró además su cola oculta: el nombre del símbolo no casaba con el `.mod` (`Caja_doble` vs `Caja.doble`), así que registraba 3 de 4 thunks — también en silencio. Red: `AotSimboloEnModTest`. — encontrado el 21-ago, y lo
  destapó Eduardo dudando de un diagnóstico mío: *«¿ningún método de clase puede ser
  native? Me parece una limitación tonta, teniendo en cuenta que `miObjeto.miMetodo(...)`
  en realidad internamente es `miMetodo(miObjeto, ...)`»*. Tenía razón.
  📐 **La causa, en una línea** (`AotCEmitter.java:259`): el pre-pass que recolecta las
  `native` recorre `module.defs` y sólo mira los `Ast.FuncDef`. **Un `ClassDef` no entra**,
  así que los métodos ni se abren. No se rechazan: no se miran.
  🩸 **Y por eso el fallo es MUDO, que es lo grave.** Probado con una clase con
  `public native function doble(): integer` devolviendo `this.n * 2`: el compilador no da
  error, `AotMain` dice *«no tiene funciones `native` — sin emisión»*, y el método corre
  **interpretado** mientras el programador cree que va a velocidad AOT. Pedir velocidad y
  que te la nieguen sin avisar.
  ⏭️ **Dos trabajos, y el primero NO espera a V6**: (a) que `native` en un método **avise**
  — es el mismo criterio de «que el desfase grite» que el proyecto ya aplica en el gate del
  `.mod`, el `magic` de la BIOS, el sello del `.npk`, la marca del punto de encuentro y el
  aviso de `/lib` rancio; (b) abrir el barrido a los métodos, pasando el objeto como primer
  parámetro — que es lo que ya ocurre por debajo.
  📌 **Y una corrección de un diagnóstico mío**, para que no se herede el error: escribí que
  la barrera era *«no hay `this`»*. Falso. El emisor **ya sabe emitir `MemberAccessExpr`**,
  que es lo que `this.n` necesita. La barrera era el barrido, no la semántica — y eso hace
  el trabajo bastante más pequeño de lo que yo había dicho.

### Arrastres de V4 y varios

### Cola de H2 (la SD), anotada al cerrarlo el 8-ago

- ~~**`H2-P5` (exFAT / «superfloppy»)**~~ — ✅ **CERRADO el 21-ago con una PRUEBA, no con una
  suposición.** Tarjeta reformateada a exFAT en el PC y metida en la Metro:
  ```
  [ 7809] sd: no hay FAT32 en la particion (exFAT? reformatea a FAT32)
  ```
  ✅ **El resultado es el bueno**: exFAT no está soportado —se sabía— pero el firmware
  **lo detecta y lo dice con la salida incluida**, en vez de callarse o montar basura.
  Y ya estaba documentado en los dos idiomas (`referencia.html` §SD y su gemelo inglés):
  *«Formatea las tarjetas en FAT32… exFAT todavía no está soportado»*. Nada que arreglar.
  📌 **Y de regalo, el pin de detección quedó probado en sus TRES casos**, que no era el
  objetivo de la prueba: `tarjeta RETIRADA — /sd desmontado` al sacarla,
  `zocalo VACIO (pin de deteccion) — no se monta` al arrancar sin ella, y detección **en
  caliente** al meterla (leyó la tarjeta a los 7,8 s del arranque). Los tres correctos.

- ~~`H2-P5` — **variedad de tarjetas**~~ — ✅ **CERRADA (triaje 26-ago): el enunciado ya NO aplica**: era
  *«una sola tarjeta y una sola placa»* y son **dos y dos** (Eduardo, 17-ago): la
  SanDisk de **128 GB en las dos placas** (Metro y P4) y la de **32 GB en la
  Metro** —la medida que documenta `#425`: 38 ficheros, `ls` 99 ms—.
  📌 **Ojo con el nombre**, que ya despistó una vez: la **Pico no tiene lector de
  SD**, así que ninguna prueba de tarjeta puede ser suya. Lo que se llama «Pico»
  es la **imagen** del firmware, que es **única para Pico y Metro** (RP2350, la
  variante se decide en runtime). Placa ≠ imagen.
  📐 **Qué queda cubierto de verdad** — mirando sobre qué se bifurca el driver
  (`bpvm_sd.c:129`), no la etiqueta comercial. Hay **dos caminos**, no tres:
  **CSD v1 = SDSC** (capacidad por tres campos, direcciona **por BYTE**) y
  **CSD v2 = SDHC *y* SDXC juntas** (un solo campo, direcciona **por BLOQUE**).
  La de 32 GB es SDHC y la de 128 GB es SDXC —*la ficha la llamaba «SDHC»: error
  de etiqueta, aunque para el driver den lo mismo*—, o sea que **las dos clases de
  alta capacidad están probadas** y el driver **no está afinado a una tarjeta ni a
  una placa**. Eso era el grueso de la ficha, y está hecho.
  ⏭️ **Lo que queda es sólo esto, y son dos cosas de capas distintas:**
  1. 🟢 **SDSC — la parte que corrompe en silencio, YA PROBADA sin tarjeta**
     (17-ago, `make test-sdsc`). Eduardo: *«no tengo tarjetas de 2G ni voy a
     tener, están obsoletas»* — y tiene razón, pero lo que daba miedo de SDSC no
     era la tarjeta, era **una cuenta**: `arg = alta_cap ? lba : lba*512`
     (`bpvm_sd.c:400` y `:415`). En SDHC/SDXC el argumento de CMD17/CMD24 es el
     BLOQUE y en SDSC el BYTE, y confundirlos no da error: lee o escribe otro
     sitio. Eso es aritmética pura sobre un dato del OCR, y lo único que tocaba
     el hardware eran dos funciones de plataforma — `bpvm_spi_transfer` y
     `bpvm_gpio_write` —, que el test pone él. **No hacía falta la tarjeta.**
     Cada caso va con su gemelo de alta capacidad (lba 2 → 1024 vs 2), y el
     bloque 0 se comprueba aparte porque es el único donde un driver roto
     acierta por casualidad — o sea que arrancar no distingue el fallo.
     **Verificado en las dos direcciones**: invirtiendo la línea del driver, el
     test se pone rojo SÓLO en los casos SDSC y el gemelo sigue verde.
     *(El decodificador del CSD v1 ya estaba cubierto en `test_sd.c` desde H1.)*
     ⚠️ **Lo que sigue sin poderse medir**, y así se queda: las rarezas
     eléctricas y de arranque de una SDSC real (no contesta a CMD8, negociación
     distinta). Sin tarjeta no hay forma, y suponerlo sería peor que decirlo.
     Mismo criterio que L14: si no se puede medir, se dice.
  2. 🟡 **exFAT y «superfloppy» sin MBR** — NO son del driver SD sino de **FatFs y
     del arranque de partición**, o sea otra capa. Se prueban **reformateando
     cualquiera de las dos tarjetas que ya hay**, sin comprar nada: es lo barato
     que queda de esta ficha.
- 🔴 **La VM-C normal NO puede correr bases de datos: hace falta placa** — abierta el
  19-ago al preguntarlo Eduardo (*«¿ahora se puede testear una consulta a una BD en la
  VM-C? Es la que puede probar el usuario sin placa»*).
  🩸 **Choca con la promesa del proyecto.** Los demos de BD SI corren en el PC, pero con
  `bpgenvm-c/build/sqldemo.exe`, un binario de PRUEBAS que produce `make test-sqldemo`
  con SQLite enlazado dentro. La VM-C que usaria un usuario contesta *«sql_open: falta
  el codigo nativo del pack 'SQLI' v1»*. O sea que «depura en el PC, despliega en el
  micro» no se cumple para la mitad de lo que V5 añade.
  ⏭️ **Dos caminos**, y el segundo es el bueno:
  1. enlazar SQLite en la VM-C de host cuando se compile con un flag — rapido, pero
     mete 400 KB de C ajeno en el binario de todo el mundo;
  2. **que la VM-C de host sepa CARGAR un pack**, como hace la placa. Es mas trabajo
     —el pack es nativo por arquitectura, harian falta `.npk` de x86-64— pero es lo
     coherente: el PC dejaria de ser un caso especial.
  📌 Y hay un tercer camino que quiza gane: el **micro simulado del IDE** (V4/H10,
  `bpvm-sim`) ya habla wire v1 completo. Si el simulador carga packs, el usuario prueba
  sin placa Y sin binario especial.
  📌 Mientras tanto, decirlo en `docs/BASEDATOS.md`: hoy la prueba real es en placa.

- 🔴 **`listDir` NO esta en la VM-C: un programa BP no puede listar un directorio en
  placa** — encontrado el 19-ago escribiendo `docs/TARJETA_SD.md`.
  🩸 Se puso un ejemplo de recorrer un directorio, se ejecuto, y la VM-C contesto
  *«builtin 42 no soportado en esta VM (subconjunto C)»*. miVM si lo tiene, o sea que
  **funciona en el PC y no en el micro** — la peor forma de faltar.
  📌 **Acotado**: es el UNICO verbo de fichero que falta. `readFile`, `writeFile`,
  `appendFile`, `fileExists`, `readFileBytes` y `writeFileBytes` estan en las dos
  (comprobado sobre `bpgenvm-c/src/builtins.c`).
  📌 **No confundirlo** con el arbol de ficheros del IDE, que si lista la tarjeta: eso
  lo hace el firmware por el wire (`LIST`/`LIST_DIR`), no el programa del usuario.
  ⏭️ Implementarlo es un builtin en la VM-C sobre la fachada de FS que ya existe. **No
  se hizo: es una FEATURE que falta, no un bug, y estamos en freeze** — decision de
  Eduardo. Documentado como limitacion en `TARJETA_SD.md` §6, con el rodeo mientras
  tanto (llevarse la cuenta uno mismo, o nombres predecibles por fecha).

## CERRADAS EN V5 (con su commit, para no volver a darlas por abiertas)

| ficha | qué | commit |
|---|---|---|
| `#384` | el error de palabra reservada DICE que lo es | `2637a43` |
| `#385` | el tipo de un literal entero lo decide su MAGNITUD | `537dfe3` |
| `#386` | el argumento de `Main` sale de su valor por defecto | `7a2eef2` |
| `#387` | a la 2ª firma le faltaban los TIPOS | `e9c9b5a` |
| `#388` | encadenar sobre lo devuelto por un método importado | `9ed0010` |
| `#390` | visibilidad en 3 niveles (private / protected / public) | `0b258d3` |
| `#391` | ABSORBIDO por #390: `virtual` es todo menos `private` | — |
| `#392` | el importador contaba mal los slots de una hija con sobrecargas | `c4f5053` |
| `#393` | el importador comprueba su tabla de métodos | `b5d2ff0` |
| `#402` | el oráculo pasa también por ARM, con el SQLite entero | `4420746` |
| `#403` | el emisor a `.class` queda marcado OBSOLETO | `c3a8b13` |
| `#406` | un `throw` sin atrapar ya DICE qué pasó, en las 3 familias | `c599095` |
| `#362` | la zona de packs sirve RECURSOS (host) | `d5552ed` |
| `#417` | **verificado EN PLACA (P4, 14-ago)**: los recursos salen de la zona | — |
| `#414` | módulo `Packs` (`list`/`listIn`) — **verificado en el P4** | `3901f1c` |
| `#365` | un módulo con `library` ya puede **arrancar** un pack | `88e75a4` |
| `#411` | el SQLite.pack con carpeta propia, reconstruible de un clon limpio | `5f9e924` |
| `#383` | `PACK_CALL` — **CANCELADA** por alcance (los packs los hace el proyecto) | — |
| `#419` | el arranque con SD — **DESCARTADA POR LA MEDIDA** (965 ms, 266 de la SD) | — |
| `#398` | el refresco del árbol: **6953 ms → 155 ms**, verificado en la P4 | `f4e5c1f` `10b4467` |
| `#430` | el cuelgue de la Metro era **LA TABLA DE HANDLES** — **verificado en DOS familias: Metro y P4 (17-ago)** | `d1c1c1f` |
| `#302`p3 | el GC escanea la pila C del native — **VERIFICADO EN PLACA (17-ago)** | `53a22fa` |
| `#422` | el chivato del `/lib` rancio — **VERIFICADO EN PLACA, los 2 caminos (17-ago)** | — |
| `#418` | `/sys` resuelve (el ULTIMO: rescata sin tapar) — **VERIFICADO EN PLACA (17-ago)** | — |
| `#433` | el log COMUN truncaba por el final y EN SILENCIO — ahora anillo (P4/S3/STM32) | `79a25ce` |
| `#424` | los eventos del GUI: **medido y mejorado** (50→100 Hz); el resto → `#434` en V6 | `f96c957` |
| `#425` | el listado DECLARA lo que deja fuera (4 implementaciones + el simulador) | `a632122` |
| `#437` | la consola llega donde el árbol: `copy` · `get` · `logclr` | `dcb2b7d` |
| `#435` | la ventana de la placa reordenada + entorno a diálogo, también desde la principal | `08de08e` |
| `#436` | editar el `.bpbuild` desde el IDE (guarda y RELEE para validar) | `c88f8b5` |
| `#394` | subir eligiendo destino — ahora se VE y se puede editar | `69adaa9` |
| `IDE-7` | selección múltiple: borrar y subir en lote, con UN refresco | `69adaa9` |
| `#395` | botón `DAO build`, habilitado sólo con proyecto abierto | `1eaf117` |
| `#440` | el `.mdn` de RISC-V direccionaba sus datos en **ABSOLUTO** — **VERIFICADO EN PLACA (P4, 17-ago)** | `9d41562` |

**`#430`, la ficha entera** (abierta y cerrada el 17-ago; se abre aquí para no
perder cómo se acotó, que es lo reutilizable):

- **Síntoma**: la Metro se colgaba muda ejecutando `AotGcRt` (30.000 concats en
  una `native`). Ni log, ni `MALLOC FAIL`: la cola de flash acababa en
  `about to bpvm_run` — el post-mortem **no cubre cuelgues**, sólo crashes y
  puntos fijos de volcado (ficha aparte, ver ABIERTAS).
- **Cómo se acotó** (todo de Eduardo, y en este orden):
  1. *«¿Qué pasa si no es native?»* → sin `native` moría igual, tras el 3000.
     El nativo y el escaneo #302, exonerados de un plumazo.
  2. *`gc()` a mano cada 1000* → **terminó limpio**. El GC de la placa funciona;
     lo que fallaba es que nadie lo llamaba.
  3. *«Cambiar el tamaño de la tabla y ver si se cuelga antes o después»* → el
     gemelo `AotGcRt2` gasta el DOBLE de handles por vuelta y murió tras el
     **1000** en vez del 3000. La muerte sigue a la **cuenta de handles**.
- **Causa**: el disparo del GC contaba **volumen** (#357) y un programa de
  objetos chicos se le escapa: 600 KB (bajo el umbral de 704 KB) pero 30.000
  slots. La tabla sólo doblaba hasta pedir 512 KB **de SRAM** (las dos tablas
  salen del malloc de PLATAFORMA, no del heap de la VM, que está en PSRAM). El
  RP2350 tiene 520 KB. El malloc fallaba → `vApplicationMallocFailedHook` →
  parpadeo eterno: **un cuelgue, no un error**.
- **El arreglo, en las DOS VMs** (las tres ideas, de Eduardo):
  1. **La marca**: repartir un slot de los últimos 64 arma `handle_pressure`;
     la puerta de `heap_alloc` lo consulta y colecta ahí. Si recicla, resuelto;
     si todo está VIVO, crece — donde crecer es una decisión, no un accidente
     en medio de un `register`.
  2. **El tope por puerto** (`BPVM_HANDLE_CAP_MAX`; la Pico: 16384 slots =
     128 KB) convierte el malloc imposible en OOM honesto ANTES de pedirlo. Y
     `handle_register` deja de devolver la **dirección cruda** cuando no puede
     crecer (el «las refs MIENTEN» que #355 dejó a medias): ref nula → los 5
     sitios de `interp.c` la vuelven `No space in heap` atrapable.
  3. **La excepción PREFABRICADA**: el OOM se construye en el prólogo del RUN,
     cuando construir es gratis, y vive como raíz del GC. Lanzarla no aloja
     nada ⇒ muere el «throw: sin memoria para el MENSAJE → el programa NO se
     entera».
- **De regalo**: miVM escribía su diagnóstico de GC por **stdout** — cualquier
  programa que colectara rompía el invariante en Java. A stderr, como
  `bpvm_diag`.
- **Pruebas**: `AotGcRt2` con memoria de Metro mantiene la tabla en 4096 y
  termina; `OomHandles` con `--handlecap` 2048/1024 atrapa el OOM y sigue vivo,
  y el nodo escala con el tope (994 / 482); paridad 28 PASS; `test-aotgc` verde.
  **En placa: `AotGcRt2` llega a `fin` con `malos : 0`** (antes moría al 1000).

**`#302` paso 3, cómo se verificó EN PLACA** (17-ago, con el #430 ya arreglado):
`AotGcRt.bp` en su forma NATIVE, 10.000 vueltas, `.mdn` cargado (1 thunk, 152 B
nativo). Salió `malos : 0` y `ultimo : v9999w9999`. Por qué eso PRUEBA el
escaneo y no sólo "no petó": el intermedio del concat izquierdo vive **sólo en
la pila C** mientras el derecho aloja tres veces más; con la presión de tabla
disparando (#430) hubo ~20 colectas en el recorrido, y 7 de cada 12 reservas de
la vuelta se hacen DENTRO de `eco`. Sin el escaneo conservador de la pila C, ese
intermedio se recicla y `ultimo` sale corrupto — el mismo fallo que el test rojo
`test_aotgc.c` pilló en host antes de arreglarlo. 10.000 comparaciones, cero
desviaciones.

*(Y el 17-ago por la tarde, la MISMA prueba en el **P4** una vez arreglado
#440: 10.000 vueltas, `malos : 0`, `ultimo : v9999w9999`. O sea que el
escaneo conservador de la pila C esta verificado en placa en las **dos
arquitecturas**, ARM y RISC-V, no en una.)*

> Y la lección de método: este sample estuvo DOS intentos sin probar nada —
> primero mudo (parecía colgado cuando trabajaba: le faltaba el latido), y luego
> colgándose de verdad por una causa **ajena a lo que venía a medir** (#430). Un
> instrumento nuevo se valida antes de creerle, también cuando lo que falla es
> el sujeto y no el instrumento.

**`#417`, cómo se verificó** — importa porque el instrumento obvio no valía:

- Pack `test1` grabado en el P4 con `montserrat_26_bold.bin` dentro (3 entradas:
  `mod1.mod`, la fuente y el `manifest.mft`), y `FontLoadDemo` cargándola.
- **El `id` que devuelve `loadFont` NO prueba nada**: el contador es 1-based y se
  asigna SIEMPRE, con o sin fuente, a propósito, para que la VM-C y miVM devuelvan
  ids idénticos (paridad dual-VM) — `gui.c:964`.
- **Lo que lo prueba es una línea que NO aparece.** Si no consigue materializar la
  fuente, `gui.c:981` escribe
  `[gui] loadFont('...'): no se pudo cargar (id N queda sin fuente)`.
  No está en la salida, y en el P4 ese chivato está activo porque lleva LVGL.
- **El `__guiDumpTree` no sirve** para esto: `gui.c:1185` sólo imprime `font=`
  cuando hay `fontSize` (catálogo compilado), nunca para `setFont`. Su silencio no
  significa nada.
- **Y salió del PACK, no del FS**: Eduardo lo probó con la forma **cualificada**,
  `Gui.loadFont("pack:test1/montserrat_26_bold.bin")`, que va a ESE pack y se
  salta el FS entero. Así que no queda el matiz de «cargó, pero no sabemos de
  dónde»: la zona de packs sirvió el recurso, que es exactamente lo que #362
  prometía y lo que esta ficha tenía que demostrar.

Con esto **H11 quedó desbloqueado** (era la ficha que lo trababa) y el 15-ago
**cerró entero**: `#414` y `#365` cerradas con commit, `#411` en su parte de
packs, y `PACK_CALL` (#383) cancelada.


### ~~[lenguaje] `List` con captadores TIPADOS~~ — ✅ CERRADA EN V5 (`20-ago`, adelantada desde V6)

Eduardo la adelantó al ver que las demos de BD no funcionaban: *«las demos han de
funcionar, no vamos a hacer como en C que por sistema las demos nunca funcionan»*.
Entraron `getInteger`, `getLong`, `getDouble`, `getBoolean` y `getString`, con las
conversiones puestas en los cinco envoltorios. **El enunciado y las decisiones de
diseño se conservan enteros abajo, porque la cola de `Map` sigue abierta.**

- **[lenguaje] `List` con captadores TIPADOS** — encargo de Eduardo (19-ago):
  *«en List añadir métodos `getInteger(indice)`, `getLong(indice)`, `getFloat(indice)`,
  `getDouble(indice)` y `getString(indice)`»*.
  🩸 **De dónde sale, y por eso no es azúcar cosmético**: desde `#389` `List.get()`
  devuelve `Object`, así que todo uso tipado necesita un downcast explícito. El coste ya
  se pagó el 19-ago — `Json.bp` llevaba días sin compilar y el arreglo fueron **8 casts,
  los 8 el mismo patrón**: `JsonValue(this.items.get(i))`. Con captadores tipados eso se
  escribe una vez, dentro de `List`, en vez de en cada sitio que la use.
  📌 **Encaja con lo que ya hay**: los envoltorios (`Integer`, `Long`, `Double`, `Float`,
  `Boolean`) se mudaron a `Core` con `#446`, y `add` ya está sobrecargado por tipo. Esto
  es la simetría que falta — se puede meter por tipo pero no sacar por tipo.
  📐 **La semántica la decidió Eduardo (19-ago) y NO es un cast**: *«si no es del tipo
  pedido hay que hacer conversiones. Los envoltorios ya deberían tener las conversiones.
  Y si hay una conversión imposible se dispara un error.»* O sea que `getInteger(i)` no
  exige que el elemento SEA un `Integer`: lo convierte, y sólo revienta si la conversión
  es imposible. Devuelve el primitivo (`integer`), no el envoltorio.
  🔴 **Y ahí está el trabajo de verdad: hoy los envoltorios NO tienen conversiones.**
  Medido en `Core.bp` el 19-ago — `Integer`, `Long`, `Double`, `Float` y `Boolean` tienen
  exactamente cuatro cosas cada uno: constructor desde SU primitivo, `value()`,
  `compareTo(Object)` y `toString()`. Nada más. Así que esto son **dos fichas encadenadas**:
  primero las conversiones en los envoltorios, después los captadores de `List`, que se
  vuelven triviales encima.
  📐 **Y la DIRECCIÓN importa (Eduardo, 19-ago)**: *«integer a string vale, así `"hola"+1`
  se convierte en `"hola1"` sin problemas, pero string a integer no, eso hay que pedirlo
  explícitamente con la función concreta.»* La asimetría no es capricho: hacia `string` la
  conversión **no puede fallar** (todo tiene `toString`), y desde `string` **falla por el
  CONTENIDO**, que es otra clase de cosa.
  🔬 Comprobado el 19-ago, las dos mitades: `"hola" + 1` → `hola1` y `"pi=" + 3.5` →
  `pi=3.5`; y la familia explícita ya existe — `Str.parseInt`, `parseLong`, `parseDouble`
  y `parseHex`, **todas devolviendo `(boolean, valor)`**, o sea que ni siquiera lanzan:
  obligan a mirar el `ok`.
  ✅ **Con eso se cae la contradicción que se había anotado**: `string`→número NO entra en
  los captadores, así que no hay dos contratos compitiendo. Queda repartido y limpio:
  - `getString(i)` — **siempre funciona**, porque todo sabe volverse cadena.
  - `getInteger/getLong/getFloat/getDouble(i)` — convierten **entre numéricos**; si el
    elemento es una cadena, **NO se parsea**: eso se pide con `Str.parse*`.
  - lo imposible lanza, que es lo que Eduardo pidió.
  ✅ **`getBoolean` ENTRA** (Eduardo, 19-ago: *«añade getBoolean, no hay problema»*), y
  con él la conversión booleano→numérico: **`False` = 0, `True` = 1**.
  📐 **De dónde viene la idea, y el matiz que la recorta.** Eduardo la trajo por su
  parecido con `ord()`, *«que también sirve para los elementos de un enumerador y para la
  conversión de char»*. `Ord` es de **Pascal** (en Java es `? 1 : 0`), y eso juega a
  favor: está definido justo para esos tres casos, así que es buen modelo. **Pero en BP
  sólo quedan DOS de los tres**: `char` **no existe como tipo** —no está en la gramática
  y los caracteres ya SON enteros (`sb.appendChar(44)`)—, o sea que esa pata sobra aquí.
  🔬 **Y el tercero tampoco funciona hoy**, comprobado el 19-ago: `var i: integer := c`
  con `c` de un enum da *«valor de tipo 'Color' no asignable a variable de tipo
  'integer'»*, aunque la gramática los respalda con enteros
  (`enum_value ::= name [':=' INTEGER_LIT]`). O sea que **de un enum no se puede sacar su
  número**, y eso es un agujero por sí solo — emparenta con `M6` de `PENDIENTES`
  (`const C := Color.RED` tampoco vale). Hacia `string` sí van los dos, coherente con la
  regla de dirección.
  ⏭️ **Recomendación al abrirlo: NO un `ord()` nuevo.** Con dos casos no compensa gastar
  una palabra reservada —criterio de Eduardo: *«si ya hay algo especial, el azúcar cuelga
  de ahí»*—. Lo natural es que salga de las conversiones que ya se van a escribir:
  `Integer(b)` e `Integer(color)`, y los captadores encima.
  ✅ **`double`→`integer` TRUNCA** (Eduardo, 19-ago): *«debe truncar, si se quiere
  redondear que llame a la función para redondear que para eso está»*.
  🔬 Y esa función existe — comprobado: **no está en `Math`, son builtins globales**:
  `round` (id 33, *half-up*), `floor` (31) y `ceil` (32), con `abs`, `sqrt` y `pow` al
  lado. `Math.bp` sólo tiene trigonometría y logaritmos, así que buscarlo ahí despista.
  ⚠️ **Hay que decir HACIA DÓNDE trunca, y no es un detalle**: truncar es *hacia cero*
  (`-2,7` → `-2`), mientras que `floor` da `-3`. Coinciden en positivos y discrepan en
  negativos, que es justo donde nadie mira. Y **ningún builtin trunca hoy**: `floor` vale
  para positivos y `ceil` para negativos, o sea que el comportamiento de los captadores
  es NUEVO y tiene que quedar escrito en su documentación, con el caso negativo de
  ejemplo.
  ✅ **`long`→`integer` que no cabe: EXCEPCIÓN** (Eduardo, 19-ago: *«pues claro, es una
  exception, es puro sentido común»*). Coherente con `#385`: el recorte silencioso da un
  número plausible y equivocado, que es el peor fallo posible.

  ### El contrato, ya cerrado entero (19-ago)

  | de \ a | `string` | numérico (`integer`/`long`/`float`/`double`) | `boolean` |
  |---|---|---|---|
  | numérico | implícito (`"x=" + 1`) | convierte; `double`→entero **trunca hacia cero**; si no cabe, **excepción** | — |
  | `boolean` | implícito | `False`=0 · `True`=1 | directo |
  | `string`  | directo | **NO** — se pide con `Str.parseInt/parseLong/parseDouble`, que devuelven `(ok, valor)` | **NO** |
  | otro objeto | `toString()` | **excepción** | **excepción** |

  📌 **Los métodos**: `getString`, `getInteger`, `getLong`, `getFloat`, `getDouble` y
  `getBoolean`, todos por índice y devolviendo el **primitivo**.
  📌 **Qué se lanza**: lo mismo que ya lanza un downcast fallido (`#444`), para no
  inventar una segunda familia de errores que diga lo mismo.
  📌 **La regla que lo explica todo en una frase**: hacia `string` es implícito porque no
  puede fallar; desde `string` es explícito porque falla por el CONTENIDO; y entre
  numéricos convierte, pero **perder información es un error, no un redondeo silencioso**.
  📌 Aplica también a `SyncList` y `OwnerList`, que heredan de `Core.List`, y conviene
  mirar si `Map` quiere lo mismo para sus valores.

### Hitos de V5 — la tabla

| hito | qué | cerrado |
|---|---|---|
| H1 | la Metro **lee** la tarjeta SD | 7-ago, en placa |
| H2 | la SD como **sistema de ficheros** (FatFs) | 8-ago, en placa |
| H3 | **SQLite corre en la Metro** (la tabla BIOS presta memoria) | 8-ago, en placa |
| H4 | un programa BP **consulta una BD de verdad** | 10-ago, salida idéntica al host |
| H5 | **el ORM**: DAO a mano → `@BD{...}` → generador → verificador | 11-ago |
| H6 | la SD del P4 por **SDMMC** + `LIST_DIR` a código común | 11-ago, en placa |
| H7 | **SQLite en el P4**: nativo RISC-V ejecutándose, motor arrancado, pack grabado y `SqlDemo` corriendo | **CERRADO**, verificado en placa |
| H8 | *la herramienta antes que el artefacto*: relocalizador que coincide con `ld`, `sources`, un `.mod` y N `.mdn`, botón de grabar que relocaliza | 13-ago, en host |
| H9 | la tanda de **arreglos del compilador** (#384, #385, #386, #387, #388, #392, #393, #406) + `Object` como raíz real (#389, la mitad estática) | **14-ago** |
| H10 | **el IDE**: lo pendiente que no eran bugs | 15-ago (las 9 fichas de su sección, cerradas) |
| H11 | **packs**: cerrar lo que quedó suelto (`#416`, paraguas) | 15-ago |
| H12 | **documentar V5** de cara al usuario | abierto 18-ago |
| H13 | **las pruebas finales** | abierto 18-ago |

*(El nombre de H8/H9 no está en ningún doc: sale de los prefijos de commit. Ojo con
confundir el H9 de V5 con el H9 de V4, que era el kernel por capas.)*

*(Tabla subida desde `ESTADO` el 17-ago: era el único sitio con las fechas
y el enunciado de cada hito. **H10 (IDE) y H11 (packs) cerrados** también.)*

**Cerrados: H1…H11.** Quedan **H12** (documentar) y **H13** (pruebas finales), abiertos
el 18-ago al fijar el plan de cierre; después, publicar.

⚠️ *Aquí decía «Queda H10 (IDE)» dos líneas después de decir que H10 estaba cerrado —
una contradicción dentro de la propia fuente de verdad, corregida el 18-ago. H10 lo
está: sus 9 fichas están todas tachadas.*

---


### 📦 Archivado el 23-ago al abrir V6 — secciones que ya estaban terminadas

> Estaban en «ABIERTAS» pero no lo estaban: o se cerraron durante V5, o eran el propio
> trabajo de cerrar la versión, que se completó al publicar el 22-ago. Se bajan enteras,
> sin tocar su texto.

### 🏁 Packs — V5/H11 «cerrar lo que quedó suelto» (#416, paraguas) — **CERRADO el 15-ago**

> Las cuatro fichas que colgaban de él, resueltas: `#417` y `#414` **verificadas
> en placa**, `#365` verificada en las dos VMs, `#411` cerrada en su parte de
> packs, y `PACK_CALL` (#383) **cancelada** por decisión de alcance. Lo que se
> quitó de en medio para poder cerrarlo —la limpieza de `notas/`— no era trabajo
> de packs: está en «Cierre de V5».
>
> Se queda todo escrito aquí, no se borra: el registro de lo cerrado es lo que
> evita volver a darlo por pendiente.

- ~~`#417`~~ — **CERRADA el 14-ago, verificada en el P4.** Ver abajo.
- ~~`#414`~~ — módulo `Packs`. ✅ **CERRADA: host (`3901f1c`) y VERIFICADA EN EL
  P4 el 15-ago.** API: `Packs.list()` y `Packs.listIn(pack, ext)`, las dos
  devolviendo `List`.
  **En placa** listó los dos packs grabados con su contenido: `SQLite` (7
  entradas) y `test1` (3), y el filtro por extensión dejó 4 módulos y 1
  respectivamente. **Cuadra con lo que el IDE enseña por el wire** (`PACK_ENTRIES`
  → «SQLite … 7 fich», «test1 … 3 fich»): dos caminos independientes contando lo
  mismo, que es la mejor comprobación que se podía pedir sin montar nada.
  **La forma la decidió Eduardo**: cuatro intrínsecos que sólo mueven primitivos
  —dos avanzan (`0` empieza, `-1` termina), dos dicen el texto— y la lista se arma
  en BP. Así ningún builtin construye objetos, que era el coste escondido de la
  ficha: hoy ninguno lo hace. El cursor es un valor que lleva el programa, así que
  no hay estado, es reentrante entre hilos y avanzar es O(1).
  **miVM**: sin zona de packs, `next` devuelve -1 a la primera → lista vacía. Eso
  contesta la duda del diseño del 13-ago y da la paridad **sin un solo `if`**.
  Verificado en host: con `--pack=PackFixA.pack` lista el pack y sus 3 ficheros
  (y cuadra con el `LIST` del propio firmware, que es otro camino); sin zona, las
  dos VMs dicen `0`; stdlib 27, frontend 102/102, miVM 34/34, paridad 28/0/0.
  ⚠️ **Para probarlo en placa hay que REFLASHEAR**: los cuatro builtins son
  código de la VM-C. Con un firmware viejo, `PacksDemo` se encuentra un opcode
  que no conoce. El sample está en `samples/PacksDemo.bp` y en el P4 debería
  listar `SQLite` y `test1` con su contenido.
- ~~`#411`~~ — ✅ **CERRADA el 15-ago** (`5f9e924`) en lo que era de packs: el
  SQLite.pack tiene **carpeta propia**, `bpstdlib/sqlite/`, con fuentes, los
  cuatro nativos versionados (`.npk` + `.mdn` × ARM/RISC-V) y un `LEEME.md` con
  la cadena entera. El pack se reconstruye igual (1.122.304 B) y **ya se puede
  rehacer desde un clon limpio**, que antes no.
  📤 **La limpieza de `notas/` SE SACA DEL HITO** — decisión de Eduardo (15-ago):
  no es trabajo de packs, es de cierre de versión, y tenerla aquí trababa H11 sin
  motivo. Vive ahora en «Cierre de V5» (al final de este fichero).
  *(Lo de abajo es el enunciado original, por si hace falta el contexto.)* Sus palabras: *«todo el tema del SQLite.pack debería tener una
  carpeta propia»*, *«en notas debería haber las notas y nada más»*, *«las demos
  (SqlDemo, SqlDemoSd) SÍ deben estar en samples, que son ejemplos»*. Y en esa
  carpeta va **todo: fuentes, compilados y el pack**.
  Censo: `bpstdlib/SQLite.bp` se va (es librería de pack, no stdlib — `Stdlib.bp`
  no la importa ni entra en `Stdlib.pack`, así que mover es barato) ·
  `samples/Orm.bp` se va · `notas/p4/SQLite.bpbuild` se va · las demos se quedan.
  Y `notas/` tiene CINCO subcarpetas de experimentos con binarios dentro
  (`metro-h4`, `p4`, `v5-salto-crudo`, `v5-sqlite-prueba`, `v5-sqlite_edu`).
  **Por qué importa**: el 13-ago costó tiempo porque con `Orm.bp` en `samples/`
  la fuente local GANA al pack, y la prueba del ORM-desde-el-pack no probaba nada.
  Es el mismo patrón que el `/app` tapando a `/lib` del 15-ago.
  Falta decidir: el NOMBRE de la carpeta, y qué se hace con las cinco de `notas/`
  (llevan los binarios que fueron la evidencia de H4/H7/H8).
  ⚠️ Al ejecutarlo: toca rutas de build y hay que reconstruir el `SQLite.pack`
  al terminar para comprobar que sale igual. En su propia tanda, no a medias.
- ~~`#365`~~ — ✅ **CERRADA el 15-ago (`88e75a4`), verificada en las dos VMs.**
  Un módulo con `library` ya puede **arrancar** un pack.
  **Qué pasaba** (y el enunciado viejo se quedaba corto — no era «`library` +
  `out:pack` es imposible», era el *arranque*): el `.mod` de un módulo con
  `library` se llama `com.example.Demo.mod`, así que su entrada en el pack es
  `com.example.Demo`; el manifest escribía `main=<proj.main>` y `proj.main`
  nombra el FICHERO FUENTE (`Demo.bp`). Quien arranca busca la entrada LITERAL
  (`bpvm.c:643` y `ModuleManager.executeRootPack`, las dos igual) y no la
  encontraba. Poner el cualificado en `main` tampoco valía: ahí se busca el
  fuente. Un pack **biblioteca** con `library` sí funcionaba.
  **El arreglo lo decidió Eduardo** («¿y si ponemos `library` dentro del
  manifest?»). De las dos formas se eligió la que **no toca las VMs**: en vez de
  un campo `library=` que las dos tuvieran que concatenar —dos implementaciones
  haciendo la misma cuenta es donde el invariante se rompe—, el manifest lleva
  ya el nombre CANÓNICO (`main=com.example.Demo`). Las dos VMs siguen buscando
  literal, **sin una línea de cambio**. El manifest es un fichero generado: puede
  llevar el nombre resuelto. El dato viaja en el `Cierre`, que es del compilador.
  🩸 **Y de camino, una trampa muda**: la regla de doble extensión (la de
  `sqlite.npk.RISCV`) miraba el penúltimo componente del nombre. Con
  `com.example.Npk.mod` —un módulo llamado `Npk` dentro de una librería— veía
  `npk` y renombraba la entrada a `com.example.mod` con tipo `npk`, en silencio y
  dentro de un pack ya grabado; con `Mod`, un error falso. Ahora sólo se mira la
  doble extensión si la ÚLTIMA no es ya un tipo.
  **Verificado, no sólo compilado**: `samples/packlib/` (queda en el repo, con el
  cómo-se-prueba dentro) construye el pack y **las dos VMs dan la misma salida**;
  frontend 104/104 con 2 tests nuevos, miVM 34/34, paridad 28/0/0, `test-pack` y
  `test-packres` verdes, y **el `SQLite.pack` real da sus 9 entradas idénticas**
  con el compilador nuevo. Fat-jar del IDE reconstruido.
- ~~`PACK_CALL` (= **#383**)~~ — ❌ **CANCELADA el 15-ago, decisión de Eduardo**:
  *«estos packs los hacemos nosotros, así que el sistema actual está bien»*.
  Era un builtin genérico para llamar a un pack **sin AOT** («reusar el mecanismo
  de los `intrinsic`»), y lo que compraba era que **mantener** un pack nativo no
  exigiera los dos toolchains cruzados: hoy, tocar una línea de `SQLite.bp`
  obliga a regenerar `SQLite.mdn.ARMV8` y `.RISCV`. Como el único que publica
  packs nativos es el propio proyecto —que tiene los toolchains—, esa barrera no
  existe en la práctica.
  **Lo que se aceptó al cancelarla, dicho claro**: en un pack nativo el AOT **no
  es una optimización, es un requisito**. Sin `.mdn` para esa arquitectura, sus
  funciones lanzan. Y el AOT **es mudo por línea de comandos** (ver el LEEME de
  `bpstdlib/sqlite/`): si no puede generar los `.mdn`, el pack sale más pequeño
  sin decir nada. Eso deja de ser «algo que PACK_CALL arreglará algún día» y pasa
  a ser el comportamiento definitivo — por eso conviene que el aviso mudo del AOT
  se mire alguna vez.
  **Y lo que costaría si algún día se reabre** (medido el 15-ago, para no
  repetir el estudio): el `.npk` tiene **UNA sola entrada** (`bp_pack_init`,
  `NpackBuild.java:44`) y **ninguna tabla de símbolos**, así que haría falta
  cambiar su formato, **regenerar los `.npk` con los dos toolchains** (el `.elf`
  intermedio no se guarda), un opcode nuevo en las dos VMs y la llamada genérica
  en C — que esa sí es barata: un `switch` por aridad con casts a punteros de
  función de N `int32_t`, sin ensamblador ni libffi, con el mismo límite de 32
  bits que ya tiene la ABI del AOT.
  🔧 El comentario del parser que la daba por futura está actualizado
  (`Parser.java:699`): ese cuerpo-que-lanza es **definitivo**.

### ~~🐛 `#431`~~ — ✅ CERRADA (`8055248`): miVM busca las deps junto al `.mod`, y un módulo que falta se DICE

Descubierto de rebote el 17-ago preparando la prueba de `/sys` (#418), y
confirmado con un control (`BridgeApp`, un sample viejo, falla igual ⇒ no es del
sample nuevo).

```sh
java -jar miVM/target/bpgenvm-1.0.jar bpgenvm-c/samples/SysUse.mod   # ❌ revienta
cd bpgenvm-c/samples && java -jar ../../miVM/...jar SysUse.mod       # ✅ va
bpgenvm-c/build/bpgenvm-c bpgenvm-c/samples/SysUse.mod               # ✅ va (VM-C)
```

Dos cosas mal, y la segunda es la fea:

1. **La política difiere**: la VM-C resuelve las deps en la carpeta del `.mod`
   (`bpvm_load_mod` → `path_dirname`); miVM las busca en el CWD.
2. **El fallo NO es un error, es un `FileNotFoundException` con stack trace de
   Java.** Aunque la política se decidiera distinta a propósito, quedarse sin
   una dependencia tiene que decirlo como lo dice la VM-C, no volcar la pila.

No afecta al arnés (copia a un WORK dir y ejecuta desde allí) ni al IDE (manda
rutas ya resueltas) — por eso ha vivido tanto tiempo sin verse. Grupo B.

### 🩸 El ORM no funcionaba — encontrado y arreglado el 19-ago al documentarlo

- ~~**El ORM entero, roto**~~ — ✅ **ARREGLADO y VERIFICADO EN EJECUCION el 19-ago**
  (`240b400d` los bugs, `7854b61f` el doble de host).
  🩸 **Ningun demo de BD compilaba**, y `H12` iba a documentar eso. Tres causas:
  1. **el GENERADOR emitia codigo roto** — `DaoGen` escribia `return this.uno(...)` en
     una funcion declarada como que devuelve la entidad, y `Orm.Dao.uno()` devuelve
     `Object` desde `#389`. Ya emite el downcast;
  2. **`List` era ambiguo en TODO programa del ORM** — al importar `Core` el semantico
     aliasa `List` en el modulo, y la interfaz de ese modulo la reexporta **como suya**;
     quien importa `SQLite` y `Orm` veia dos simbolos para UNA clase. Arreglado dando
     prioridad a `Core`, que se importa implicitamente y es donde viven los tipos raiz;
  3. **y lo que mas costo NO era un bug**: `samples/` tenia **seis copias fosiles de la
     stdlib del 10-11 de JUNIO**, ninguna en git, que ganaban porque se busca en
     `sourceDir` antes que en las dependencias. El `Core` de junio no tenia `List`.
  🩸 **Y faltaba el `packglue.c`**, que yo mismo habia borrado con `notas/`: es el doble
  de host de la tabla BIOS, sin el cual no se puede ejecutar nada de BD en el PC.
  Reescrito desde `bios_pico.c`, con `malloc`/`free`/`realloc` que **no funcionan**
  igual que en la Pico — un doble mas amable que el original es una trampa.
  🔬 **PROBADO EN EJECUCION, no solo compilando** (`make test-sqldemo`, host):
  · `SqlDemo` — exec, execInt/Str/Double, query, consulta anidada · `status=OK`
  · `DaoDemo` — dos DAO a mano, una conexion, CRUD entero, `Where` con encadenado y
    `orNext`, el apostrofo escapado · `status=OK`
  · `GenDemo` — los mismos verbos con DAO **generados** · `status=OK`
  Los tres con **0 bloques sin liberar**, y sin una sola linea de los stubs de `malloc`:
  SQLite tira solo de su arena, como en la placa.
  📌 **La leccion, y es de metodo**: esto lo destapo IR A DOCUMENTARLO. Escribir «asi se
  usa el ORM» obliga a ejecutarlo, y ejecutarlo es lo que lo encontro. Documentar antes
  de publicar no es cortesia con el usuario: es una prueba mas.

- 🔴 **La documentación INGLESA arrastra deuda de V4** — visto el 20-ago al repasar el
  bilingüe (lo pidió Eduardo: *«la documentación, la release y la portada son bilingües»*).
  ✅ **Lo de V5 ya está espejado**: `en/referencia.html` (los dos grupos, SD y Packs),
  `en/guia-ide.html` (comandos y las once claves del ENV), `en/index.html` (V5, tarjetas y
  el escaparate de BD) y `en/basedatos.html`, que no existía.
  🔴 **Lo que falta es ANTERIOR a V5:**
  1. **`en/manual.html` se dejó SIETE secciones de V4**: `eventos` con sus cuatro
     subsecciones (`ev-declarar`, `ev-escuchar`, `ev-cuando`, `ev-async`), `sobrecarga` y
     `dospasadas`. Los eventos y la sobrecarga de funciones **son features de V4** que
     nunca se tradujeron: 61 secciones en español contra 54 en inglés.
  2. **Las notas de versión no existen en inglés.** `docs/RELEASES.md` es sólo español, y
     `en/index.html` las enlaza marcadas «(Spanish)» — o sea que está asumido, no roto.
  ⏭️ Decisión de Eduardo: traducir esas siete secciones es trabajo de documentación, no un
  bug, así que cabe o no cabe en el freeze según lo que él quiera. Lo que sí conviene es
  que no se publique creyendo que el manual inglés está completo.

### Cierre de V5 — lo que se hace AL CERRAR, no antes

- **⚖️ DECIDIDO (22-ago): el ZIP pesa 24 MB y SE QUEDA ASÍ.** No es un descuido ni una
  tarea pendiente — es una decisión, y se escribe aquí para que nadie la «arregle».
  📊 **De dónde sale el peso** (V4 9,6 MB → V5 24,4 MB, medido comparando los dos ZIP):
  el 93 % del crecimiento es **el fat-jar del IDE**, que pasa de 4,4 a 18,2 MB. Y dentro,
  `org/sqlite` son **24,9 MB sin comprimir**: el driver JDBC de SQLite, que el IDE necesita
  en el PC para leer el esquema de una base y generar los DAO de `@BD`.
  📐 El artefacto de Maven trae los nativos de **seis plataformas** — Linux 7,2 MB ·
  Android 4,9 · **Windows 3,8** · musl 3,2 · FreeBSD 3,2 · Mac 2,3 — y en una distribución
  `-win` sólo se ejecuta uno. Son ~20,7 MB que no arrancarán jamás.
  🎯 **Y aun así se quedan. Eduardo**: *«no tocamos nada. Es el PC: los recursos son casi
  ilimitados comparados con los de un micro. 10 MB más en un PC no es nada.»*
  📌 **Encaja con la filosofía del proyecto y conviene verlo así**: se diseña para el
  dispositivo pequeño, y el PC **no es la restricción**. Gastar riesgo de empaquetado —una
  exclusión de rutas en el shade que, si se pasa, deja el IDE sin generador de DAO— para
  ahorrar unos megas en un disco de terabytes es cambiar algo caro por algo que no vale
  nada. El sitio donde cada byte cuenta es la flash del micro, y ahí sí se pelea.

Nada de esto bloquea un hito, y por eso está aparte: tenerlo colgando de H11
trababa el hito por trabajo que no era suyo (Eduardo, 15-ago).

- ~~**`H12` — DOCUMENTAR V5**~~ — ✅ **CERRADO el 20-ago.** Los ocho puntos hechos:
  la tarjeta SD (§14.17 de la referencia), los packs (§15), el IDE (consola al día y el
  ENV con sus once claves), `Core` (§13.2 reescrita y §13.3 nueva), la puerta (portada,
  los dos README y su apartado de BD) y las notas de versión (`RELEASES.md`, v5.0 «los
  datos», con su sección «lo que todavía no»). Más `docs/basedatos.html`, volumen propio.
  🩸 **Y lo que documentar destapó, que fue lo caro**: el ORM no compilaba (tres bugs),
  `listDir` no está en la VM-C, la §13.2 daba un ejemplo que ya no compila, la lista de
  comandos del IDE se dejaba seis, el ENV no tenía ni una clave escrita y `AOT_LIMITES`
  negaba el `long` en `native`. Ninguno salía leyendo código: salieron al EJECUTAR los
  ejemplos y al contrastar cada afirmación.
  📌 Queda para `H13`: revisar que la documentación no afirme nada más que haya dejado
  de ser cierto, con la misma pregunta por documento.
- ~~(el enunciado original)~~ **Hito** por decisión de Eduardo
  (18-ago): *«documentar con el número de hito que le corresponda»*. Va DESPUÉS de la
  limpieza. Abierta ese mismo día al repasar el cierre —*«queda hacer limpieza,
  documentar y pruebas finales»*— porque **la parte de documentar no tenía línea aquí**
  y esta lista es la que se mira.
  🩸 **Lo medido, no lo supuesto**: `sqlite` y `@BD` aparecen **CERO veces** en toda la
  documentación de usuario —`manual.html`, `referencia.html`, `cheatsheet.html`,
  `QUICKSTART.md`, `README.md`/`README.es.md`— y sólo salen en ficheros de trabajo
  internos (`FICHAS`, `ESTADO`, `V5_IDEAS`, `V4_BACKLOG`, `CENSO_FAMILIAS`). O sea que
  **los dos hitos con más cara de usuario de toda V5 no existen para quien lea la
  documentación**: el ORM (H5: `@BD`, generador de DAO, verificador, reglas de tipos
  BP↔SQLite) y SQLite dentro de un pack (H3/H4).
  ✅ **Lo que SÍ está bien y no hay que tocar** —comprobado, para no rehacer trabajo—:
  `referencia.html` §13.3/13.4 describe `OwnerList` y `SyncList` sin llamarlas
  sintetizadas, y su frase *«SyncList extiende List»* pasó a ser literalmente cierta
  con `#450`/`#451` en vez de quedarse rancia. `Comparable` tiene entrada (§ tabla de
  stdlib). `Thread` y `Mutex` siguen descritos como sintetizados, que es correcto.
  ✅ **HECHO el 19-ago: `docs/BASEDATOS.md`** (`d614d27f`) — las 13 secciones del indice
  de Eduardo, y todo lo que afirma esta EJECUTADO. Escribirlo fue lo que destapo que el
  ORM no compilaba (ver la ficha de arriba): documentar es una prueba mas.
  ⏭️ **Lo que queda, en el orden acordado con Eduardo (19-ago):**
  1. **La TARJETA SD** — el hueco grande: cero menciones de «tarjeta SD», «/sd» o
     «FatFs» en manual, referencia y QUICKSTART, y son TRES hitos (H1, H2, H6).
     📌 **Encuadre correcto, precision de Eduardo**: la SD es capacidad de la IMAGEN,
     no de la placa. **RP2350 y ESP32** (en STM32 todavia no). Metro y P4 traen lector
     soldado; en una **Pico 2 se cablea uno** y funciona igual — la imagen es la misma
     y los pines salen del ENV. Decir «Metro y P4» donde toca decir la familia
     convierte una cuestion de cableado en un limite inventado.
  2. **Los PACKS** — construir, grabar, listar, formatear; packs ejecutables; los
     **resources dentro del pack**; `import X from "..."` (hay samples: `fromtest.bp`,
     `frompathtest.bp`, `appwithfromimpl.bp`); y los **proyectos tipo pack**, que
     probablemente vienen de V4. Hoy: UNA mencion en cada documento.
  3. **Los cambios del IDE** (H10).
  4. **`Core`** — que `List` ya no la sintetiza el compilador, los envoltorios y sus
     CONVERSIONES, el `add` sobrecargado y los **captadores tipados** (`getInteger` y
     compania: cero menciones hoy).
  5. **Las VARIABLES DE ENTORNO** — ✅ **CENSADAS el 20-ago**, que era el trabajo previo.
     Son **once**, y algunas llevan valor estructurado:

     | clave | qué |
     |---|---|
     | `board` | identidad de la placa |
     | `display` | qué panel lleva (P4: `st7701`…) |
     | `sd` | el lector: **valor estructurado**, distinto por familia — RP2350 (SPI) `sck,mosi,miso,cs,cd`; ESP32 (SDIO) `clk,cmd,d0..d3,pwr,pwralto,slot,khz,ldo` |
     | `SQLite` | MB de arena para la BD (mín. 2, máx. 4095) |
     | `psram` · `psramCsPin` | PSRAM y su pin de selección |
     | `flashSizeBytes` | tamaño de flash declarado |
     | `gpioCount` | número de GPIO |
     | `log` | el log post-mortem, encendido/apagado |
     | `gc` | el recolector (Pico) |
     | `latido` | el latido de vida (Pico) |

     🩸 **Y por qué no las sabía nadie**: **no hay UNA forma de leer el entorno**. Conviven
     `bpvm_env_get*(env, "k", …)`, un envoltorio por familia (`board_env_bool("gc", 1)`) y
     un `#define ENV_KEY_DISPLAY "display"` — y ese último no lo encuentra ningún grep de
     literales en la llamada. Por eso los dos primeros censos se dejaron la mitad.
     ⏭️ **Para V6**: una sola puerta de lectura, o un registro declarado de claves. Mientras
     haya tres idiomas, la lista se vuelve a desincronizar en cuanto alguien añada una.
  6. **Los COMANDOS NUEVOS del IDE** (encargo de Eduardo, 19-ago).
  6b. 🔴 **`native` con `long` — y `docs/AOT_LIMITES.md` MIENTE.** Lo recordo Eduardo el
     19-ago (*«hemos ampliado las funciones native a long... es un pasito»*). Es `#381`,
     cerrada el 16-ago y verificada en la Metro. Pero `AOT_LIMITES.md` sigue diciendo
     que *«`float`, `long`, `double` y `void` quedan fuera»* (linea 64) y titula un
     apartado *«Tipos de 8 bytes: long y double»* (33). **Eso ya no es cierto para
     `long`**, y una documentacion que NIEGA una capacidad que existe cuesta mas que una
     que falta: el usuario ni lo intenta.
     📌 Ojo al corregir: de `double` el documento tiene RAZON y hay que dejarlo — en
     RP2350 y STM32U5 no hay coma flotante de 64 bits en hardware (emparenta con `L14`).
     Lo que cambia es solo `long`.
     📌 Y lo que merece contarse de `#381`, que es lo mejor de la ficha: **dividir por
     cero desde codigo nativo lanza un error de BP ATRAPABLE en vez de reiniciar la
     placa**. Eso es lo que convierte `native` en algo que se puede usar sin miedo.
     🔎 **Y al mirar los limites REALES (19-ago) salio que `native` son DOS CAMINOS con
     limites distintos** — si el documento no lo distingue, cualquier correccion sera
     verdad en un sitio y mentira en el otro:
     · **native normal (AOT)** — `long` SI cruza (es `#381`). `double` no, y por hierro:
       RP2350 y STM32U5 no tienen coma flotante de 64 bits.
     · **puente a un PACK (`AotCEmitter.cTypePack`)** — mas estrecho, y ahi `#381` NO ha
       llegado. Cruzan `integer`, `boolean` (int32), `float`, `string` (`const char*`),
       `long[]`/`double[]` **solo como caja de salida**, y los objetos **como handle**.
       El porque esta en su propio mensaje y es bueno: *«un pack no conoce el GC de BP,
       asi que solo cruzan VALORES»*.
     🔴 **Mensaje de error RANCIO**: el del puente dice *«el rodeo se quitara cuando el
     AOT marshalle 8 bytes - tarea #381»*, y `#381` YA esta cerrada. Quien lo lea buscara
     una ficha resuelta. O se corrige el texto, o se abre la ficha de llevar los 8 bytes
     tambien al puente.
  7. **`QUICKSTART` y los dos README**, que son la puerta.
  8. **Las notas de la version**, con L14 y L15 de `PENDIENTES` enlazadas.
  📌 Y una comprobación barata al terminar, que es la que caza lo rancio: buscar en la
  documentación de usuario los nombres que V5 movió o creó y ver que ninguno describe
  el mundo anterior.

- **`H13` — LAS PRUEBAS FINALES.** **Hito** por decisión de Eduardo (18-ago): *«las
  pruebas finales también con su hito correspondiente»*. Va DESPUÉS de `H12`, y es lo
  último antes de publicar.
  📌 **No duplica fichas: las agrupa.** El texto de cada una sigue en su sitio; aquí
  está sólo la lista de lo que entra en la tanda, para que ninguna se quede fuera por
  vivir en otra sección:
  - **`#379`** — verificar que era `#398` (el refresco del árbol) y no una
    desincronización del wire. Hipótesis fuerte de Eduardo del 17-ago, con los cuatro
    síntomas encajando; el Stop del P4 del 18-ago sobre un bucle cerrado es una piedra
    más. *(sección «Placas y hardware»)*
  - **`#408`** — medir los dos cuellos que se ven comparando P4 y Metro (árbol en la
    P4, formateo en la Metro). Válido desde que el P4 dejó de compilarse a `-Og`.
    *(«Placas y hardware»)*
  - **`H2-P5`** — exFAT y el «superfloppy», reformateando una de las dos tarjetas que
    ya hay. La mitad de SDSC cayó el 18-ago con `test_sdsc`. *(«Cola de H2»)*
  - **la cola de `#439`** — repetir en Metro y STM32 la prueba que cerró la ficha en el
    P4 (`CuelgaLog.bp` → `kill` → `reset` → «RAM SUPERVIVIENTE»). El código está
    verificado en el `.elf` de las cuatro imágenes y probado en placa en una. Está
    escrita DENTRO de una ficha ya cerrada, que es donde las cosas se pierden: por eso
    se nombra aquí. **En la Metro, ojo**: allí el botón físico de reset SÍ conserva la
    RAM (pin de RUN), al revés que en el ESP32 — ver L15 de `PENDIENTES.md`.
  ⚠️ **Estamos en CODE FREEZE** (ver la cabecera): lo que salga de esta tanda y no sea
  un bug se anota para V6, no se arregla de camino.
  📌 **Ojo con el número**: el `H13` de V4 era la batería de pruebas de aquel cierre.
  Mismo problema que ya avisa la tabla con el `H9`. Los hitos se numeran por versión.

- ~~**Borrar las cinco carpetas de experimentos de `notas/`**~~ — ✅ **HECHO el 19-ago**
  (`3636ff0` el rescate, `f2edd80` el borrado). 57 MB fuera; `notas/` se queda con las
  notas y nada más, que era el encargo (*«en notas debería haber las notas y nada más»*).
  🩸 **Y la ficha se equivocaba en lo importante.** Decía que *«lo único NO duplicado son
  los `.elf` de SQLite»*. Falso: había **26 fuentes y scripts que no existían en ningún
  otro sitio del repo**, y entre ellos `vfs_bp.c` —el VFS sin el cual
  `sqlite3_initialize()` falla en silencio— y los dos `build_sqlite*.sh`. Enfrente,
  `bpstdlib/sqlite/nativo/*.npk` y `*.mdn`, **binarios versionados que se publican en
  V5**. Borrar sin mirar los habría dejado sin receta para siempre.
  📌 Rescatado a `bpstdlib/sqlite/nativo/src/` (77 KB), que es donde Eduardo decidió el
  14-ago que viviera todo lo del pack. El `LEEME.md` de esa carpeta ya decía que se creó
  porque *«el pack no se podía reconstruir desde un clon limpio»* — el traslado se había
  hecho a medias.
  🔬 **Probado, no supuesto**: el `.npk` de RISC-V se reconstruye **byte a byte** (618.168 B
  idénticos, y sus números son los que canta el P4 al arrancar). Ese control es lo que
  dice que la receta rescatada es la buena.

- ~~**Los restos del árbol**~~ — ✅ **HECHO el 19-ago** (`f2edd80`). 28 MB más:
  - `docs/390-private-wip.patch` — borrado, comprobando antes que no aplica.
  - `miVM/.claude/worktrees/jolly-blackburn-9ec483/` — quitado con `git worktree remove`,
    no con `rm`, para que git se entere. Y de paso salió **un segundo worktree** que la
    ficha no listaba: un husk de la sesión `25fabe6b` en el scratchpad de Temp, con el
    checkout ya borrado pero aún registrado. Fuera también; queda un único worktree.
  - `docs/V4_SAMPLES_ROJOS.md` — **marcado como HISTÓRICO** y metido en git con esa
    cabecera. Sin fecha visible invitaba a leerse como actual, y ya daba por vivos bugs
    cerrados. El censo vigente es el del 3-ago y lo produce `compat/compat.sh`.
  - artefactos sueltos (`fc.txt`, `ff.txt`, `fileio_test.txt` ×3, `auto.txt` —vacío—,
    `bigfile.bin`) y **27 `.slots`**, que además pasan a `.gitignore` para que no vuelvan.
    Los de `bpstdlib/` y `bpdevices/` **siguen versionados**: son la distribución.

- ~~**Vaciar `C:	mp`**~~ — ✅ **HECHO el 19-ago** (encargo de Eduardo el mismo día).
  **157 MB → 0.** 146 MB eran tres árboles de build sin una sola fuente dentro (el
  `hello_world` de ESP-IDF y dos workspaces de CubeIDE). Los 12 MB restantes NO eran
  temporales, y por eso se miraron uno a uno antes de tirarlos:
  - `Discovery_u5g9j_knowngood_GUI.elf` (7,4 MB) — **superado**: la imagen buena de la
    DK2 es `bpvm_stm32_dk2.bin`, y está en `dist/firmware/`, dentro del ZIP, con su
    SHA256 y publicada. Comprobado, no supuesto.
  - `gpio_stm32_selfconfig.bak.c` — **estrictamente anterior** al del repo: a éste le
    faltan el `BOARD_NAME`, los callbacks board-aware de ADC/PWM y el TIM16 de la DK2.
  - `spike_device/` (par `.uf2` ON/OFF + su `LEEME`) y `bpvm_pico_SUBNORM.uf2` —
    imágenes de experimentos cuyas MEDIDAS ya están escritas (la del SUBNORM es la de
    `L14` en `PENDIENTES`); los binarios se rehacen.
  - **42 sondas `.bp`** que no existen en el repo — ninguna citada por ninguna ficha
    (el único positivo del grep era `JFileChooser`, otra cosa).
  📐 **El criterio es de Eduardo, y vale más que la limpieza**: *«no tiene sentido tener
  algo que queremos mantener en un directorio temporal»*. Un directorio temporal es
  desechable POR DEFINICIÓN; si dentro aparece algo irreemplazable, el fallo es que esté
  ahí, no que se vaya a borrar. Lo contrario de lo que pasó con `notas/` el mismo día —
  y por eso allí se rescató y aquí no.

- ~~**El desfase de `Str` y la stdlib**~~ — ✅ **RESUELTO el 19-ago** (`58ad9d0`).
  Eduardo, al plantearselo: *«si hay que guardarlo se guarda, si hay que compilarlo se
  compila y hay que actualizar el pack se actualiza, no veo el problema»*. Y tenia razon
  en que no lo era — pero al tirar del hilo era **mucho mas grande que `Str`**:
  🩸 **24 de los 26 modulos versionados** no coincidian con lo que emite el compilador de
  hoy. Arrastraban toda la tanda del 18-ago (`#442`..`#451`) y el `Core` nuevo.
  🩸 **Y tapaban un modulo ROTO: `Json.bp` llevaba dias sin compilar.** Desde que `#389`
  dejo que `List.get()` devuelva `Object` en vez de `any`, 8 sitios asignaban el
  resultado a `JsonString`/`JsonValue` sin downcast. Su `.mod` del 15-ago lo escondia —
  el modo de fallo exacto contra el que avisaba la ficha de la stdlib-como-proyecto, otra
  vez. Arreglado con 8 casts explicitos, sin tocar la logica.
  🩸 **Y un build NO limpio MIENTE**: `bpstdlib/out/` tenia modulos del 15-ago, asi que el
  primer pack que genere salio contaminado y el build no se quejo. Con `rm -rf out`
  delante, aborta en `Json` — que es lo que debia haber pasado siempre. **Regla: la
  stdlib se reconstruye SIEMPRE en limpio.**
  🔬 **Verificado con la bateria de `H13`** (`scripts/h13-lista.sh`), antes y despues:
  `48 corren · 18 compilan · 4 NO compilan · 0 fallan`, y el **diff de las dos salidas es
  VACIO**. Los 66 programas se comportan igual, `JsonDemo` (parse + serialize) incluido —
  que es lo que prueba que los casts estan bien y no solo que compilan.
  📌 Regenerados tambien `packs/Stdlib.pack` (163.840 -> 172.032 B) y los blobs embebidos
  de las 3 familias; el firmware de la Pico reconstruido y enlaza. **ESP32 y STM32 se
  reconstruyen en `H13`**, que es cuando se reflashean.

- ~~**La decisión sobre `dist/`**~~ — ✅ **DECIDIDA el 19-ago por Eduardo**: *«dist lo
  reconstruimos antes de publicar. En principio no se sube al repositorio, el zip se
  sube a GitHub aparte.»* O sea que `dist/` es una **salida**, no una fuente: se rehace
  en el cierre de cada versión y viaja como asset de la release.
  📌 Aplicado en `.gitignore`. Con una excepción deliberada: **los tres MANIFIESTOS sí
  siguen versionados** —`BasicPlus-4.0-win.zip.sha256`, `firmware/README.md` y
  `firmware/SHA256SUMS.txt`, 4 KB de texto—. Son el único registro en el repo de qué
  binarios salieron en cada versión, y sin ellos no se puede verificar un artefacto
  descargado. El ZIP no; su huella sí.
  🩸 **Y el motivo práctico está medido**: dentro hay **342 copias** de ficheros del
  árbol, y su `packs/Stdlib.pack` quedó rancio respecto al layout nuevo de
  `Collections.Map` (`#390`). Eso no es teórico — el 19-ago un `grep` de `FileTest.bp`
  salió por duplicado justo por esto, igual que pasaba con el worktree.
  📌 Los 31 MB de la build de V4 **siguen en disco a propósito**: es la distribución de
  una versión ya publicada y se sustituye sola al reconstruir. Ignorarla basta.
- **Este fichero** deja de ser material de la versión en curso y puede subir con
  ella (ver la cabecera).

## 🧊 CODE FREEZE V5 — ✅ LEVANTADO (histórico)

> Se levantó al publicar V5 el 22-ago. Se baja aquí el 23-ago porque, arriba, un
> congelado se lee como una instrucción en vigor. **El criterio que usó sigue
> siendo bueno** y volverá a valer al cerrar V6: ante algo mejorable, la pregunta
> no es «¿merece la pena?» sino «¿está roto?».
>
> *Título original: «CODE FREEZE V5 — desde el 18-ago-2026».*


**Decisión de Eduardo:** *«A partir de ahora, código congelado, solamente se arreglan
bugs.»*

**El criterio, que ya se probó en V4:** la pregunta ante algo que se podría mejorar NO
es *«¿merece la pena?»* sino **«¿está roto?»**. Si no está roto, se anota para V6 y se
sigue. Una mejora que entra en la recta final no viene sola: viene con su tanda de
verificación y con el riesgo de romper algo que ya estaba probado en placa.

**Qué entra**: bugs. **Qué no entra**: features, refactors, mejoras «de paso», y —lo
que más se cuela— arreglar de camino algo que se ve feo mientras se toca otra cosa.

**El plan de cierre, en este orden** (Eduardo, 18-ago):

1. **Limpieza** — el 19-ago, antes de empezar a documentar. Está en «Cierre de V5».
2. **H12 — documentar V5.**
3. **H13 — las pruebas finales.**
4. **Publicar.**

**Y hasta publicar, nada sube a GitHub.** Commitear no es publicar; «ahead of origin»
es lo normal en esta fase. Ver la norma en la cabecera de este fichero.

---


## SIN CATALOGAR

Números que aparecen en el transcript pero cuyo título no he podido recuperar:
**#397, #399, #400, #405, #410, #413**. Si hacen falta:

```
f=~/.claude/projects/C--lenguajes-pm-miVM/25fabe6b-e3ce-428d-b70b-77e2f33c2004.jsonl
grep -oE '#405[^"\\]{0,120}' "$f" | sort -u | head
```

*(#405 suena a los tres arreglos del ESP32 de `5090e9a`, pero no lo doy por bueno
sin verlo: dar por catalogado lo que no se ha leído es exactamente el error que
este fichero viene a evitar.)*

## 📦 ARCHIVO — cerradas de la cola heredada de V5 (triaje del 26-ago-2026)

> **Qué es esto.** Al abrir V6 se trajo la cola de pendientes de V5 tal cual. De sus **59
> fichas, 47 ya estaban resueltas** —con su ✅ y su commit— pero seguían archivadas dentro
> de `ABIERTAS`, así que la lista de pendientes **mentía al alza**: cada vez que había que
> decidir «¿qué hacemos ahora?» había que filtrarlas a mano.
>
> Es el criterio de Eduardo aplicado a la propia libreta ([[no-acumular-pendientes]]):
> *«una lista larga ESCONDE»*. En V5 costó tiempo real — dos bugs cerrados que seguían
> apareciendo como abiertos.
>
> **No se ha borrado ni una línea**: el texto de cada ficha viaja entero, con su sección
> de origen como encabezado. Lo que cambia es dónde vive.


#### IDE — V5/H10 «lo pendiente que no son bugs»  *(archivadas)*

- ~~`#437`~~ — **la consola no llega a donde llega el árbol** (Eduardo, 17-ago:
  *«creo que falta alguno; si ahora se puede copiar un fichero a una carpeta
  determinada, desde la consola también debería poder hacerse»*). Censado sobre
  el dispatch de `PicoExplorer` y la interfaz `Backend`:

  | acción | árbol / botones | consola |
  |---|---|---|
  | listar · borrar · ejecutar · parar · editar | Refresh, Delete, Run, Stop, Edit | `dir` `del` `run` `kill` `edit` |
  | mem · log · save · reset | Info, Log, Save, Reset | `mem` `log` `save` `reset` |
  | **subir un fichero (PUT)** | **Upload** | ❌ **falta** |
  | **bajar un fichero al PC (GET)** | **Get** | ❌ **falta** (`type` vuelca a pantalla, no guarda) |
  | **vaciar el log** | **LogClr** | ❌ **falta** |
  | crear · autorun · SD | — | `new` `autorun` `sd` (sólo consola) |

  **Ninguna de las dos superficies es superconjunto de la otra**, que es como
  estas cosas se pudren: cada mejora entra por un lado y el otro se queda atrás.
  Los tres huecos son de verbos que el `Backend` YA expone (`put`, `get`, y el
  vaciado del log), así que es fontanería, no capacidad nueva.

  **Lo que pide Eduardo, concretado:** un `copy <local> [destino]` — `copy` y no
  `put` porque la consola habla en DOS (`dir`, `type`, `del`, `cls`) y ahí
  `copy C:\x\y.mod /lib` se lee solo. Y como la consola tiene `cd` y su propio
  cwd (`consoleCwd`), el destino puede omitirse y valer el directorio actual. Su
  gemelo sería `get <remoto> [local]`.

  **A mirar al hacerlo:**
  - ¿Hace falta `mkdir`? El `Backend` **no lo tiene**, así que hoy las carpetas
    del device sólo existen porque alguien puso un fichero dentro. Si `copy`
    admite un destino que no existe, hay que decidir si lo crea, falla, o el
    `put` con ruta ya lo resuelve solo.
  - ~~`#435`~~ mete un panel de carpetas y `#394` era «subir eligiendo destino»: las
    tres fichas tocan el mismo gesto desde tres superficies. Conviene abordarlas
    juntas y que compartan el código de resolver rutas — si no, otra vez
    [[arreglo-que-no-viaja-entre-familias]] pero dentro del IDE.
  - Mantener el `help` al día: hoy lista los comandos a mano en un `emitLine`, y
    un comando nuevo que no salga ahí es un comando que no existe.
- ~~`#436`~~ — **editar el fichero de proyecto desde el IDE** (Eduardo, 17-ago):
  *«algo parecido a editar el pom de Maven»* — el tipo de salida, los ficheros
  incluidos, las familias nativas, etc.

  **Lo difícil YA ESTÁ HECHO, y conviene saberlo antes de planificar:**
  - `BpBuild.save()` existe y **re-serializa el mapa JSON crudo conservando lo
    que no se editó** (`b.raw = map`). O sea que guardar NO se lleva por delante
    las claves que el IDE no entienda ni el array `_comentario` con que se
    documenta `SQLite.bpbuild`. Sólo se pierden los comentarios `//`. Ese es
    justo el miedo de «editar el pom», y ya está resuelto: **hay que protegerlo,
    no reinventarlo.**
  - Editar el proyecto **ya se hace, repartido en tres diálogos**: *Project
    Properties…* (que hoy sólo lleva `out:pack`, el check de AOT y UN target en
    un `JTextField`), *VM Endpoint…* y *AOT (toolchain)…*, más *Add File to
    Resources…*.

  **O sea que el trabajo es juntar y completar, no construir.** Lo que hoy NO
  toca ningún diálogo, contado sobre `BpBuild.java`: `sources` (la lista de
  `.bp` del proyecto), `dependencies`, `sourceDir`/`outDir`/`main`,
  `aotTargets` (la LISTA de familias — el diálogo actual sólo edita el
  `aotTarget` singular), y los cuatro de pack: `packName`, `packVersion`,
  `packProvides`, `packNotas` (cero menciones en `FrmMain`), más `database`.

  ⚠️ **La trampa concreta: `aotTarget` (singular) y `aotTargets` (lista) son DOS
  campos distintos.** El singular es el de siempre; la lista llegó con V5/H8
  para los packs multifamilia. Un editor que enseñe las familias tiene que
  dejarlos coherentes o el build hará una cosa y la ventana dirá otra — el tipo
  de fallo que no da error, sólo un `.mdn` de la familia equivocada
  ([[artefacto-de-otra-familia-se-cuela]]).

  **A decidir con Eduardo:** ¿un formulario, el JSON en crudo dentro del editor,
  o los dos (como hace Maven, que tiene formulario y pestaña de XML)? Lo crudo
  es casi gratis —el `.bpbuild` es un fichero y el IDE ya sabe abrir ficheros—
  pero necesita validar al guardar para no dejar el proyecto ilegible; el
  formulario es más trabajo pero es lo que evita las erratas. Y si se hacen los
  dos, quién manda cuando difieren.
- ~~`#435`~~ — **la ventana de la placa, reordenada** (Eduardo, 17-ago; es el
  primero de los cambios que trae para H10). Tres movimientos:
  1. **Las variables de entorno salen a un diálogo propio.** Hoy viven en la
     mitad de arriba de `BoardMgrPanel` (`JSplitPane` vertical: env arriba,
     particiones abajo), con su tabla, el check de `psram` y los botones
     *Añadir/editar…* y *Borrar*. Todo eso se muda tal cual a un diálogo.
  2. **La ventana se queda con particiones y packs**, y en el hueco que deja el
     env entra un **panel pequeño que enseña una carpeta** — por defecto la de
     packs, pero navegable a otras.
  3. **Añadir un pack pasa a ser seleccionar + botón.** Hoy es *«Copiar pack a
     la placa…»* → `JFileChooser` cada vez (`PacksPanel:212`, arrancando en
     `lastBurnDir`). Con el panel, el fichero ya está a la vista.

  **Lo que hay que respetar al moverlo** (son cosas que ya costaron su rato):
  - ⚠️ **El check del `.pack` YA está duplicado en cuatro sitios** y está
    fichado como riesgo en `ESTADO.md` (`bpvm.c:828`, `Main.java:378`,
    `FrmMain.java:2556` y `:3165`, `SimRunner.java:101`). El panel nuevo tendrá
    que decidir qué es un pack para habilitar el botón: **que reuse, no que
    escriba el quinto**.
  - ⚠️ **La clave del env es CANÓNICA en minúsculas.** El comentario de
    `BoardMgrPanel:48` lo dice y por qué: *«el firmware lee "psram" EXACTO
    (`bpvm_env_get` es case-sensitive). Escribir "PSRAM" fue el bug»*. Eso viaja
    con el código al diálogo — es de las cosas que se pierden en una mudanza.
  - **`lastBurnDir` ya existe** en `IdePrefs` y recuerda la última carpeta
    usada. El panel debería reusarlo (recordar dónde te dejaste) en vez de
    estrenar una preferencia nueva al lado.

  **A decidir con Eduardo cuando se aborde:** qué es «la carpeta de packs» por
  defecto —¿la de salida del proyecto abierto, o una global cuando no hay
  proyecto?—; si el panel enseña sólo `.pack` o todo con el botón deshabilitado
  para lo demás (lo segundo suele envejecer mejor: se ve por qué no se puede);
  y desde dónde se abre el diálogo del env (botón en `FrmBoard`, que es quien
  monta los dos paneles y tiene la conexión).
- ~~`#398`~~ — ✅ **CERRADA el 15-ago, VERIFICADA EN LA P4: 6953 ms → 155 ms,
  45×.** *«Pasamos de un sistema incómodo de trabajar a uno bastante cómodo»*
  (Eduardo). Lo que queda de su enunciado original —el árbol perezoso y el
  truncado mudo— **sale a ficha propia, `#425`**: no urge, y esconderlo dentro de
  una cerrada es como se pierden las cosas.

  | | antes | ahora |
  |---|---:|---:|
  | refresco del árbol con SD | 6953 ms | **155 ms** |
  | montaje de la SD | 293 ms | 46 ms |
  | arranque hasta el wire | 965 ms | **717 ms** |

  **La causa no era «el CRC es caro»**: era que `bpvm_fs_crc32` troceaba el
  fichero de 256 en 256 B y cada trozo iba por `read_at`, **que recibe el path**
  — o sea que cada 256 B se ABRÍA el fichero otra vez. En FatFs: `f_open` +
  `f_lseek` + `f_read` + `f_close`, 5432 aperturas para 1,3 MB, con el seek
  recorriendo la cadena de clústeres desde el principio (cuadrático). El chivato
  que lo delató: **el flash interno iba 3× más lento que la tarjeta** (80 KB/s
  contra 255), lo que ya decía que el coste no era leer.
  **Dos arreglos** (`f4e5c1f`, `10b4467`): (B) `crc32` opcional en la interfaz de
  backend — abre UNA vez, implementado en los tres backends; 16,5× medido en el
  PC sobre littlefs. (A) el LISTADO deja de calcular CRC (`crc:-1`) y se pide con
  `STAT {crc:true}` para el fichero que se va a subir.
  🩸 **Por qué la P4 sufría más que la Metro**: el corte que evita calcular el
  CRC de los volúmenes montados estaba **sólo en `pico/repl_v1.c`** desde V5/H2;
  la familia ESP32 nunca lo recibió. Otro arreglo que no viajó entre familias.
  🔸 **De rebote, el `ESP_ERR_TIMEOUT` del montaje no ha vuelto a salir.** ⚠️ NO
  se da por muerto: era intermitente, y una pasada buena es lo que produce un
  fallo probabilístico que sigue vivo. Hipótesis razonable y comprobable: antes
  cada refresco movía 1,3 MB por SDIO, y un reset durante o justo después podía
  dejar la tarjeta ocupada para el `init` del arranque siguiente; ahora son
  36 ms. **Lo confirmaría**: 15-20 arranques en frío y en caliente, con un
  refresco pesado justo antes de resetear.
  🔸 **El tramo más caro del arranque es ahora otro**: 337 ms escaneando la zona
  de packs para encontrar `0 candidatos` — casi la mitad de los 717 ms.
- ~~`#429`~~ — 🩸 **EL IDE COMPILA CON SU PROPIA COPIA DEL COMPILADOR, Y NO AVISA
  CUANDO ESTÁ RANCIA.** El fat-jar `BpIde-4.0.jar` empaqueta el frontend, así que
  tocar `lexer-java` y no reconstruir el IDE deja **dos compiladores distintos**
  en la misma máquina: el de la línea de comandos con los cambios y el del IDE
  sin ellos.
  **Coste medido, hoy mismo (16-ago)**: `long` en `native` funcionaba desde por
  la mañana, y al probarlo en la Metro el IDE dijo *«no puede utilizar long en
  código nativo»*. El fat-jar era de las 18:07 de ayer y el cambio de las 09:34
  de hoy. El aviso está desde hace tiempo en las notas de trabajo — y aun así se
  escapó, después de tres commits al emisor.
  **Por qué es ficha y no un recordatorio**: un aviso que hay que recordar cada
  vez ya ha fallado. Lo que falta es que **el desfase se detecte y se diga**, no
  que se recuerde. Y el modo de fallo es de los malos: no da un error raro, da un
  error PLAUSIBLE —el mensaje correcto de una versión anterior— así que uno se
  pone a buscar el bug en el sitio equivocado.
  **Ideas, de barata a buena**: que el IDE compare la fecha/hash de su frontend
  empaquetado con el de `lexer-java/target` y avise si el de fuera es más nuevo;
  que el banner de compilación (que ya imprime `BpIde-4.0.jar | fecha`) diga
  también la del frontend; o que el IDE no empaquete el compilador y lo invoque.
  ⚠️ Y el segundo filo, que ya mordió el 12-ago (`GuiColorDemo` cian): con el
  compilador rancio no siempre sale un error — a veces sale un **.mod distinto**,
  y eso no lo cuenta nadie.
- ~~`#425`~~ — **el árbol del IDE TRUNCA EN SILENCIO** (lo que queda del enunciado
  original de `#398`, = H2-P3 del backlog). El recorrido plano tiene tope de
  **16 directorios / 96 entradas** y, al pasarse, el árbol enseña menos ficheros
  sin decir nada — que se lee como «no hay más».
  Ya NO es un problema de rendimiento (eso se cerró: 155 ms), es de **verdad**:
  un listado corto silencioso es una mentira, y de las que se creen.
  Lo que hace falta ya existe: **`LIST_DIR` está en las tres familias con su
  contador de `omitidas`**, y el comando `dir` de la consola ya lo usa y ya avisa
  (`⚠ LISTADO INCOMPLETO: N entrada(s) más`). Falta que el árbol pida por
  directorio —y de paso sea perezoso— en vez del recorrido plano.
  ✅ **HECHO el 17-ago (`a632122`) y MEDIDO en la Metro el 17** con una SD de
  32 GB: el listado sale entero (38 ficheros, `ls 99 ms`) y **no aparece aviso**,
  que es el control — el chivato no da falsos positivos. Los topes siguen ahí
  (16 dirs / 96 entradas por dir), pero ahora **cuando muerdan lo dirán**, y con
  ese número se decidirá si basta subirlos o hace falta el árbol perezoso.

  ⚠️ **Corregido lo que decía esta ficha:** afirmaba que «el árbol no puede
  mostrar `/sd` porque el plano no ve los montajes». **Ya no es cierto** —
  `bpvm_fs_list` los emite como hijos (`fs_facade.c`) y en la captura del 17-ago
  se ve `/sd` con su contenido. Ese argumento ya no sostiene el árbol perezoso;
  si se hace algún día, será por los topes o por no aplanar una tarjeta entera
  en cada refresco, no por esto.
  **Por qué sube**: Eduardo (15-ago) *«la lentitud es el refresco del árbol;
  cualquier operación que implique refrescarlo —añadir, borrar— tarda 1-2 s sin
  SD y 5 s o más con SD»*. El arranque ya se descartó midiendo (ver `#419`).
  🔎 **El sospechoso, localizado**: el LS plano **calcula el CRC32 de CADA
  fichero, leyéndolo entero, en CADA listado** (`repl_esp32.c:266`). El CRC está
  ahí para que el IDE se salte una subida cuyo contenido ya está en la placa —
  una optimización de la SUBIDA que se paga en TODOS los listados. Y el recorrido
  baja a los volúmenes montados (`bpvm_fs_list` emite los montajes como
  directorios hijos, `fs_facade.c:325`), así que con tarjeta se le suma. Nótese
  que `LIST_DIR`, el verbo nuevo, **no calcula CRC** — por diseño.
  ⚠️ **Sospechoso, no culpable: está sin medir.** Por eso lo primero es el
  instrumento (`b44f15e`), no el arreglo — y hoy esa disciplina ya ha evitado un
  arreglo inútil.
  📐 **EL INSTRUMENTO YA ESTÁ PUESTO**, en los dos extremos:
  - **firmware** (`handle_list`): una línea por refresco con el total, cuánto de
    eso es CRC, los KB leídos y **el reparto por carpeta raíz** —por raíz y no
    «¿es la SD?», para no asumir la respuesta—:
    `ls: 6 ent en 1636 ms | crc 1636 ms de 1234 KB | app:2/30ms sd:2/1600ms`
  - **IDE** (`onRefresh`): el tiempo que ve el usuario partido en **ls / mem /
    árbol**, en el status. Restando el total del device sale el viaje del wire.
  El IDE mide en CUALQUIER placa, así que **P4 vs Metro** —que dirá si esto es
  del P4 o general— sale sin tocar el firmware del Pico.
  ⏭️ **Falta**: compilar+flashear el P4 y hacer un refresco con y sin tarjeta.
  Con esos dos números se decide: si el CRC es la cara, sacarlo del listado (y
  pedirlo con `STAT` sólo del fichero que se va a subir) puede valer más y costar
  menos que el árbol perezoso — o hacer falta las dos cosas.
- ~~`#394`~~ — subir un fichero **eligiendo destino** (hoy sólo por consola).
- ~~`#395`~~ — botón `DAO build`, sólo habilitado con proyecto abierto.
- ~~`IDE-7`~~ — selección múltiple en el árbol: **borrar y subir**, con UN refresco.

*(La de «rendimiento del GUI» estaba aquí y NO era de H10: es la lentitud de los
eventos EN LA P4. Movida a Placas como `#424`.)*

#### Módulos y arranque (nuevas del 14-ago, en placa)  *(archivadas)*

- ~~`#418`~~ — **los módulos de `/sys` no se encuentran.** `bpvm_entry_resolve`
  (`src/bpvm.c:697`) busca **basedir → tal cual → `/app` → `/lib`**, y `/sys` NO
  está en la lista: en toda la VM, `/sys` sólo se usa para leer `auto.txt` (#345).
  Un `Core.mod` que viva ahí es invisible para un `import`.
  **Síntoma**, y es de los que engañan: el IDE dice `exit 1 (IO error)`, pero eso
  NO es un fallo de entrada/salida — es el guardián del enlace (`bpvm.c:865`,
  *«si algo se quedó sin dueño, se NOMBRA»*). El firmware sí lo nombra
  (`repl_esp32.c:830`: `falta el modulo 'X'`); lo que no lo enseña es el IDE.
  **Decisión de Eduardo (14-ago): tiene que poder encontrarlos.** Así que el
  arreglo va en el resolutor, no en el IDE.
  *(Visto al probar `FontLoadDemo` en el P4: faltaba `Core.mod`, que está en
  `/sys`.)*
  🔧 **ARREGLADO en host** (`c161b73`): `/sys` entra al FINAL de la cadena, así el
  cambio es aditivo y no altera ninguna resolución que ya funcione. Verde: build
  limpio, `test-fsvfs`/`test-fslfs`/`test-fspos`/`test-pack`/`test-packres` y
  paridad 28/0/0.
  ⏳ **FALTA PROBAR EL CASO**, y no es formalismo: en host **no existe `/sys`** —es
  la jerarquía del device—, así que ese camino no se ha ejecutado ni una vez. Un
  camino compilado no es un camino probado. **Prueba en placa**: subir un módulo a
  `/sys` (p. ej. el `Core.mod` que ya está ahí), quitarlo de `/lib` y `/app`, y
  comprobar que un `import` lo encuentra. Con la imagen NUEVA, claro.
- ~~`#419`~~ — ✅ **DESCARTADA POR LA MEDIDA (15-ago).** Dos logs
  de la P4, con tarjeta y sin ella, leyendo los `[ms]` que el log ya trae:

  | tramo | sin SD | con SD |
  |---|---:|---:|
  | init (BD, heap PSRAM, BIOS, flash) | 4 ms | 4 ms |
  | montar el FS interno (19 ficheros) | 75 ms | 75 ms |
  | subir a estado 3 | 54 ms | 54 ms |
  | configurar el SDIO | 100 ms | 100 ms |
  | **montar la SD** | 36 ms *(timeout)* | **293 ms** |
  | **escanear la zona de packs** | **338 ms** | **337 ms** |
  | arrancar el REPL | 35 ms | 44 ms |
  | **hasta que el wire está listo** | **699 ms** | **965 ms** |

  **El arranque entero es de UN SEGUNDO y la tarjeta cuesta 266 ms.** O sea que
  la idea del hilo aparte —que era buena— habría ganado 0,3 s y no habría
  arreglado nada de lo que se nota. **Medir antes de tocar, hoy, ahorró el
  arreglo entero.**
  🔸 **De regalo**: **338 ms escaneando la zona de packs para encontrar `0
  candidatos`**, el tramo más caro después del FS, y se paga siempre.
  🔸 Y la imagen medida es la VIEJA: sigue diciendo `| 0 kHz`, o sea sin el
  arreglo del reloj ni `#420`.
  ➡️ **EL TIEMPO ESTÁ EN OTRO SITIO, y Eduardo lo acotó**: *«la lentitud es el
  refresco del árbol; cualquier operación que lo refresque tarda 1-2 s sin SD y
  5 s o más con SD»*. Eso es `#398`/`#408`, y ahí sigue el trabajo. Lo que
  quedaba de esta ficha (lo del arranque) está cerrado.
  *(Enunciado original, por contexto.)* **Sin la SD, la placa
  arranca sin errores y MÁS RÁPIDO** — y el árbol del IDE también refresca antes.
  **EL HECHO ESTRUCTURAL, que explica el síntoma** (leído el 15-ago, sin placa):
  en `wire_task_uart` —el transporte de esa placa— **el wire se abre DESPUÉS de
  montar la SD**. El orden es `board_mgr_esp32_boot` → `fs_register_bpvm` +
  `esp32_mods_install` → **`p4_montar_sd`** (`main.c:345`) → `pack_p4_cargar` →
  `esp32_hw_register` → **`wire_v1_uart_init`** (`main.c:357`). O sea que todo lo
  que tarde el montaje es tiempo en que **el IDE no puede conectar**. El árbol no
  refresca «más rápido» sin tarjeta: es que el wire abre antes.
  ✅ **LA MEDIDA YA EXISTE, no hay que instrumentar**: `log_printf` prefija
  `[ms]` a cada línea (`bpvm_log.c:67`), así que el reparto del arranque está
  escrito en el log que ya se saca. **Hace falta un log de arranque CON tarjeta y
  otro SIN**, y restar.
  ❌ **Descartado ya**: la espera de hasta 5 s por *Link Up* de Ethernet **no se
  compila** — `BPVM_P4_NETLOG` está a 0 desde V4 (`main.c:126`). Era el
  sospechoso obvio.
  💡 **Idea de Eduardo para DESPUÉS de medir**: la E/S que retrasa a todo lo demás
  es buena candidata a **un hilo aparte**. Encaja sin inventar nada —ya hay
  FreeRTOS, y el arranque escalonado de H9 ya tiene estados
  (`board_boot_status`)—, pero con dos condiciones y una advertencia:
  1. **qué contesta el sistema mientras se monta**: un `/sd` que aún no está
     tiene que decir «montando», no «no existe», o cambiamos una espera visible
     por un fallo intermitente;
  2. **la fachada del FS no tiene un solo mutex** (`fs_facade.c`,
     `bpvm_fs_fat.c`): hoy vale porque el montaje ocurre antes de que exista
     nadie más, pero montar desde otra tarea con el REPL vivo son dos hilos en el
     registro de volúmenes.
  ⚠️ **Y la advertencia, que tiene precedente EN ESTE MISMO TRAMO**: un hilo
  aparte quita el bloqueo, **no el coste**. `esp32_mods.c:4004` cuenta que el
  primer boot tardaba **~46 s** y la causa no era la obvia —cada `fs_put`
  reescribía la partición entera—; se arregló MIDIENDO. Movido a un hilo seguiría
  tardando 46 s, en paralelo y sin que nadie volviera a mirarlo.
  Lo del árbol se junta con `#408` y `#398`.
- ~~`#420`~~ — ✅ **CERRADA el 16-ago, VERIFICADA EN LA P4** (`29da27c`), y la
  prueba fue la de `#423`: para que con `log=1` aparezcan mensajes de EJECUCIÓN
  tiene que estar conectado el sink del diagnóstico de la VM, que es justo lo
  que esta ficha añadía y lo que a esta familia le faltaba. Sin ella, `log=1`
  no habría enseñado nada nuevo.
  *(El enunciado original, abajo.)* **El P4 era la única familia sin log de
  EJECUCIÓN.**
  Tenía `log_init()` y escribía todo el arranque con `log_printf`, pero **no
  conectaba el sink del diagnóstico de la VM** — el S3 lo hace en su `main.c:107`
  y el STM32 en su repl. Así que `bpvm_diag` se iba al `stderr` por defecto, que
  aquí es la consola USB-JTAG: nadie la mira y no sobrevive al reset.
  Lo que se perdía: de dónde sale cada módulo (`dep 'X' -> /lib/X.mod`), qué
  dependencia falta, el veredicto del guardián de fin de RUN.
  **Coste medido**: una mañana de hipótesis sobre un `Core.mod` en `/lib` que
  daba «IO error», con el firmware sabiendo la respuesta desde el primer intento.
  ⏭️ Compilar, flashear y **repetir el caso de `Core`** — es lo que lo cierra.
- ~~(sin número)~~ — ✅ **`read_at` NO MIRABA LA ZONA DE PACKS** (16-ago,
  `1d4ccbf`). La fachada del FS no era coherente consigo misma:
  `stat` y `read` consultaban el fallback de la zona y **`read_at` no**. Un
  módulo del pack **existía** para `stat` —con su tamaño— y no se podía leer por
  trozos; y como cargar un módulo va por `read_at` desde #305, el resultado era
  `IO error`.
  **Cómo se manifestó** (P4, con el `SQLite.pack` grabado): el resolutor probaba
  `/app/SQLite.mod`, el `stat` decía que sí con 8325 B —los del pack, aunque en
  `/app` no hubiera NADA— y la carga moría. De propina, el firmware avisaba de
  que «el FS eclipsa al del pack» sin que hubiera un solo fichero en el FS: el
  que reclamaba el `stat` era el pack mismo.
  **No era una regresión**: `read_at` llegó en #305 y el fallback en V5/H4, y
  nunca se juntaron. Sólo se manifiesta con un pack grabado **y** un módulo suyo
  que no esté también en el FS — la combinación que sólo aparece usándolo de
  verdad. *Un camino compilado no es un camino probado.*
  🛡️ La regla queda fijada en `make test-fsfb`: **si `stat` dice que un fichero
  existe, se tiene que poder leer, entero y por trozos**.
- ~~`#422`~~ — 🟡 **EL CHIVATO, HECHO (17-ago); la política de refresco, pendiente.**
  El arranque ya DICE cuándo un módulo de `/lib` no es el de la imagen
  (`lib: X NO es el de esta imagen (N B en FS, M embebido) - ¿rancio de otro
  firmware, o subido por ti?`) — en las DOS familias con despliegue, mismo
  criterio (tamaño gratis del stat; CRC de una apertura sólo si empatan) y
  mismo mensaje. En la sección del log que se registra SIEMPRE.
  ⏳ Falta placa (reflashear y tocar un `/lib` a propósito) y LA DECISIÓN:
  refrescar automáticamente exige distinguir «rancio» de «subido por el
  usuario», y eso pide estado extra (p.ej. un manifiesto con los CRC de lo que
  el firmware desplegó la última vez: si el fichero coincide con lo que YO puse
  y lo embebido cambió → refrescar; si no coincide → es del usuario, avisar y
  no tocar). Decisión de Eduardo.
  *(El mecanismo y la historia, abajo.)* 🩸 **UN `/lib` RANCIO SOBREVIVE A LOS
  REFLASHEOS.** Los módulos de
  `/lib` **los despliega el firmware**, y **grabar una imagen nueva NO los
  refresca**: Eduardo tuvo que cambiar el tamaño de la partición para que se
  repoblaran (15-ago). O sea que una placa puede tener imagen de hoy y un `/lib`
  de hace semanas.
  **Por qué no se nota, que es lo peor**: (a) el IDE sólo compara el CRC de lo que
  va a subir, y los módulos los sube a `/app`, así que **el de `/lib` no lo mira
  nadie nunca**; y (b) el orden de búsqueda es `/app` antes que `/lib`, de modo
  que mientras haya copia en `/app` el rancio queda tapado. Resultado: el fallo
  aparece cuando **quitas** un fichero que estaba de más, que es el momento más
  confuso posible.
  **Cómo se manifestó**: `Core.mod` de 2576 B en los dos sitios, mismo tamaño y
  distinto contenido; con el de `/app` iba, sin él daba `exit 1 (IO error)` sin
  más. Media mañana.
  🔎 **EL MECANISMO EXACTO, encontrado el 15-ago** — ya no es «parece que»:
  ```c
  if (bpvm_fs_stat(s_mods[i].path, &sz_dummy) != 0)   // esp32_mods.c:4014
      fs_put(...)                                      // SÓLO si no existe
  ```
  El firmware despliega su módulo **únicamente si el fichero no está**. Por eso
  reflashear no refresca `/lib` —el fichero existe, así que no se toca— y por eso
  se repobló al cambiar el tamaño de la partición: eso lo borró. La condición no
  es un descuido (existe para no pisar lo que el usuario haya subido), pero
  **compara EXISTENCIA, no contenido ni versión**, y ahí está el agujero.
  **Ideas de arreglo, por rentabilidad**: mostrar el **CRC en el árbol** del IDE
  (el `LS` ya lo trae — `PicoExplorer.deviceCrcByPath`), que convierte esta
  sospecha en una mirada; que el IDE **compare también `/lib`**; y que el
  firmware diga en el log qué versión desplegó ahí.
- ~~`#421`~~ — ✅ **CERRADA el 16-ago** (`e62a7fc`): los cuatro fallos de carga
  que antes decían lo mismo ahora dicen cosas distintas, **y viajan por el
  wire** —que era la mitad que faltaba: al log ya iban desde `18effeb`—.
  `no encuentro 'X' (buscado en …)` · `'X' mide 0 bytes (subida a medias?)` ·
  `'X' (1581 B) se lee pero no cuadra con su cabecera: truncado o de otra
  version` · `no se dijo que ejecutar`. El tercero es EL caso del 15-ago.
  Mecanismo: `bpvm_entry_t.fallo`, gemelo de `missing` para el camino de E/S —
  lo rellena quien detecta el fallo y el REPL lo reenvía. En los tres lados del
  wire. `make test-loaderr` fija los mensajes y, sobre todo, que NO SEAN EL
  MISMO. Verificado contra el simulador cortando un `.mod` por la mitad.
  ⏭️ Queda fuera, y es otro camino: el CLI del host sigue diciendo «IO error»
  (usa `bpvm_load_mod` directo, y su salida la compara el arnés de paridad).
  *(Enunciado original, abajo.)* **`IO error` era «como no decir nada»**. `18effeb` mejoró **sólo el rastro del log** de la placa; lo que el
  IDE enseña sigue siendo `exit 1 (IO error)`, que es donde mira uno primero. Lo
  que falta es que **ese detalle viaje en el mensaje del wire**
  (`repl_esp32.c:833` manda `bpvm_status_str(ls)` a secas).
  **Y hay un mudo peor, que el caso del 15-ago dejó a la vista**: el gate de ABI
  (#284, `loader.c:121`) valida la VERSIÓN —un `.mod` v5 o con magic malo grita
  con error propio— pero **no la INTEGRIDAD**. Un `.mod` v6 cuyo contenido no
  cuadre con su cabecera (truncado, a medias) pasa el control y muere con un
  `IO error` genérico: en esa función todos los IO son `bc_read_be32` fallando,
  o sea «no pude leer los siguientes 4 bytes».
  Deducción del caso real, por descarte: como el error fue `IO error` y no
  `ABI_MOD_V5`, el `Core.mod` rancio **era v6** —posterior a H6.a— y lo que
  falló fue leerlo entero, no su formato. Sale igual si el
  fichero no existe, si mide 0 bytes, si no se pudo leer o si el path venía
  vacío, y **nunca dice la ruta** — que el firmware tiene en la mano
  (`bpvm_entry_t.resolved`). Con eso, media mañana de conjeturas habría sido una
  línea. Se ve en el IDE como `exit 1 (IO error)` y no hay más.
  *(El mensaje del guardián del enlace sí es bueno —`falta el modulo 'X'`, y el
  IDE lo muestra— así que lo que falta es dar el mismo trato al camino de E/S.)*
- ~~`#423`~~ — ✅ **CERRADA el 16-ago, VERIFICADA EN LA P4** (`49083e3`).
  Eduardo, con la imagen nueva: *«con log=0 no muestra mensajes de ejecución y
  con log=1 sí. Los mensajes de arranque se mantienen siempre»* — que es
  exactamente el contrato de las tres partes.
  **La solución la decidió él**: una variable de entorno `log=0|1`, con el
  arranque fijo y lo posterior gobernado por la variable. El corte se puso al
  TERMINAR el arranque (no en cuanto se lee el ENV, que en el P4 ocurre
  demasiado pronto y habría dejado el log en una línea).
  Detalle abajo, tal como estaba.
  🩸 **EL LOG SE LLENA EN ~26 COLECTAS Y SE CALLA POR EL FINAL.** Salió
  al pie del log de arranque del 15-ago: `[LOG OVERFLOW]`. **Está en las cuatro
  familias**, no es del P4; en el P4 acaba de asomar porque hasta #420 no le
  llegaba nada de la VM.
  **La cuenta, que no admite discusión**: el GC escribe **3 líneas por colecta**
  —`heap.c:558` (`vivo=/liberado=`), `:572` (reservas) y `:584` (lista de
  libres)—, unos **300 B**. La región del log es de **8 KB** en el P4 y el STM32
  y **4 KB** en el S3 (`log_esp32.c:47`). Es decir: **~26 colectas en el P4 y
  ~13 en el S3** y el log está lleno. Un programa que trabaje con cadenas —el
  propio `BusTest`— da cientos.
  **Lo grave no es que se llene: es POR DÓNDE se calla.** `bpvm_log.c:24`
  (`append_raw`) es append-only — cuando no cabe, **deja de escribir** y pone
  `[LOG OVERFLOW]`. Así que el log de una placa que se cuelga contiene el
  arranque y las primeras colectas, y **NO el momento del cuelgue**: justo lo
  contrario de para lo que existe un post-mortem. El propio criterio ya
  aprendido («el log post-mortem es anillo, nunca truncar por el final») **no
  está aplicado aquí**.
  **Tensión real, y por eso no se arregla solo**: en el arranque interesa el
  PRINCIPIO (¿es la imagen nueva?, ¿llegó el env?) y en un cuelgue interesa el
  FINAL. Un anillo a secas se come el arranque.
  **Opciones, de barata a buena**: (a) el GC deja **una** línea por colecta —la
  de `vivo=/liberado=`, que es la que contesta #355— y las otras dos detrás de
  `--trace`: ×3 de historia, 10 minutos, pero sigue llenándose; (b) **anillo con
  cabecera reservada**: el primer tercio se congela al acabar el arranque y el
  resto rota, que da las dos cosas; (c) las dos.
  ⚠️ **No se toca sin hablarlo**: esas tres líneas son el instrumento con el que
  se cazaron #355 y #357, y quien decide qué se le quita es Eduardo.

#### Familias — lo que dejó el censo (`#427`)  *(archivadas)*

- ~~(sin número)~~ — ✅ **CERRADA el 18-ago: borradas.** Eduardo: *«se puede
  borrar, ya no lo utilizo nunca»*. Fuera `hello_mod.c` de ESP32, P4 y STM32, y con
  ellos el `bpvm_app.c/h` del STM32 — el demo de H9.1 que era su único consumidor y
  al que **no llamaba nadie**.
  📐 Lo que gastaban: ESP32 y P4 ni compilaban el suyo (no estaba en `SRCS`); el del
  STM32 **sí** entraba en la imagen, para un demo muerto.
  ✅ Verificado que no los usaba nadie de verdad: Pico y STM32 reconstruidos, **0
  errores** (el `subdir.mk` que los citaba lo regenera CubeIDE solo).
  🔁 Y el generador se redujo al único vivo — si no, la próxima pasada los habría
  vuelto a crear, que es la gracia de tener generador y el peligro de tenerlo mal.
  📌 Queda el del **Pico**, que sí se usa: lo preinstala como `/app/Hello.mod`. Si
  tampoco hace falta ahí, quitarlo es cambiar lo que la placa trae de fábrica —
  decisión aparte.
- ~~`hello_mod.c` del STM32~~ — ✅ **CERRADA el 18-ago**: **las cuatro imágenes
  salen ya del mismo fuente**, y por un generador, no a mano.
  🩸 **La raíz era peor que la divergencia**: la cabecera de esos ficheros decía
  *«GENERADO por `scripts/regen-hello-blob.sh`»* y **ese script no existía** — un
  puntero muerto que hacía pasar por generado algo mantenido a mano. Por eso el
  hello era el único blob fuera de los `regen_*_mods.sh`.
  📐 **Y el censo se quedaba corto**: decía «el STM32 lleva un Hello de otra época»
  (186 líneas vs 347, y en `MOD5`), pero al medirlo salió que **el ÚNICO vivo
  también estaba rancio** — la Pico embebía 4.034 B contra los 3.965 que emite el
  compilador de hoy. O sea que el `.mod` skew que los otros guiones evitan para la
  stdlib, aquí no lo evitaba nadie.
  ✅ `bpgenvm-c/scripts/regen_hello_blob.sh`, enganchado a `regen_all_mods.sh`. Las
  cuatro a 3.965 B del mismo `samples/hello.bp`; **firmware de la Pico reconstruido
  y enlazado** con el Hello nuevo dentro.
  ⏭️ Sale de aquí un hallazgo que NO es de este punto y va aparte (abajo): tres de
  esas cuatro copias son código muerto.

#### Placas y hardware  *(archivadas)*

- ~~(sin número)~~ — ✅ **PASADA (15-ago). La prueba que dice si el bus es SANO**: MB de patrón conocido,
  ida y vuelta, al reloj objetivo. Cola de H6. ⚠️ Lo importante: un bus marginal
  **no falla en el `mount`**, y debajo de SQLite **corrompe la base en silencio**.
  Con pull-ups de 51 K, que es el punto flojo conocido del P4.
  ✅ **PASADA EN LA SD DEL P4 el 15-ago**: `samples/BusTest.bp` (`4552b62`) —
  **2048 KB ida y vuelta, 0 diferencias**. El instrumento se validó antes con un
  control en ROJO (meterle al fichero 5 el contenido del 6): lo detectó por el
  byte 2, que es donde va el número de fichero dentro del patrón — mismo tamaño,
  distinto contenido.
  **A 20 MHz**, contestado el mismo día: el log decía `| 0 kHz` porque imprimía
  lo que PIDE el env, y el env no fija `khz` → el driver aplica su defecto
  (`SDIO_KHZ_POR_DEFECTO`, 20 MHz, el conservador que eligieron los pull-ups de
  51 K). O sea que la prueba corrió al reloj **que esta placa usa de verdad**,
  que es el que importa. El log ya lo dice bien (`0a4e25c`).
  🔓 **Se reabre si se sube el reloj** — un bus marginal aguanta despacio y falla
  arriba, y ése es justo el caso que esta prueba existe para pillar. Y si se
  quiere apretar del todo: repetirla con la placa caliente, el otro caso que
  nombra la ficha.
- ~~`#424`~~ — 🟢 **MEJORADO Y MEDIDO (17-ago); el resto va a V6 como `#434`.** Los
  eventos del GUI iban lentos en la P4. La ficha culpaba al tope de 50 ms del
  lazo; **la medida dijo que no**, y de paso tumbó también mi deducción.

  **Lo que se instrumentó** (`gui_display_dsi.c`, con `log=1`; con `log=0` no
  cuesta ni una línea — se queda en el firmware: la próxima vez que alguien diga
  «va lento», la respuesta es un botón en lugar de una tarde):
  - `gui pump`: vueltas/s del lazo, cuántas topan y el `idle` medio;
  - `gui reparto`: cuánto de cada vuelta es TRABAJO, y cómo se parte entre leer
    el táctil y volcar el frame.

  **Lo que salió, contra lo que se creía:**
  | | se creía | medido |
  |---|---|---|
  | el tope de 50 ms | el culpable | **0 disparos en 60 s** — no entra nunca |
  | el trabajo por vuelta | 10-20 ms (deducción mía) | **0,4 ms** (1,3 pulsando) |
  | el táctil | sospechoso | 1,0 ms × 25/s = 2,5 % |
  | el flush | sospechoso | 0,2 ms, y sólo cuando hay algo que pintar |

  **La causa real: el lazo no estaba ocupado, DORMÍA.** `vTaskDelay(pdMS_TO_TICKS
  (idle_ms))` obedece a LVGL al pie de la letra, y LVGL pide su periodo de
  refresco (33 ms) — que con `CONFIG_FREERTOS_HZ=100` son **3 ticks**. De ahí los
  20 ms de periodo y los 50 Hz clavados. Lo que se pagaba no era detectar el
  toque (LVGL lee el táctil con su propio temporizador, igual en las dos
  familias) sino **esperar hasta 30 ms a que se repintara**.

  **El cambio (`f96c957`): el tope, de 50 a 10 ms.** Verificado en placa:
  50 → 100 Hz, `idle` medio 17 → 8 ms, sin TWDT en 46 s. Coste conocido: ~4 % de
  un núcleo (antes 2 %). Eduardo: *«se nota más ágil»* — y con el Spike, *«algo
  más rápido pero tampoco como en el STM32»*.

  **Lo que NO se arregló** y por qué se va a V6 (`#434`): sigue habiendo un
  factor ~1,5 contra el STM32, y no es LVGL (misma biblioteca, y `lv_conf.h` es
  un único fichero compartido por las cinco familias). ⚠️ Y ojo con un hueco del
  método: **los números del STM32 nunca se midieron, se leyeron del código**. Lo
  primero de `#434` es instrumentarlo igual — el doble sólo vale de oráculo si
  se le pregunta lo mismo.
- ~~(sin número, V5/H7)~~ — ✅ **EL P4 CARGA EL PACK NATIVO EN EL PRIMER `Run`,
  como la Pico** (16-ago, `bd8a916`). **Verificado en placa: arranque 717 ms →
  386 ms** (con la imagen de dos días antes, 965 → 386: dos veces y media).
  **No era una optimización: era una decisión de Eduardo que no había viajado.**
  Está escrita en `pico/pack_pico.c` desde el 7-ago —*«un cuelgue durante un Run
  se arregla desenchufando una vez; un cuelgue en el ARRANQUE se repite en cada
  arranque y obliga a regrabar»*— y el P4 barría la zona y saltaba dentro de
  `wire_task`, antes del REPL. Costaba 338 ms de cada arranque **y ponía el
  único paso que puede colgar justo donde un cuelgue obliga a regrabar**, en una
  placa que sólo se recupera desenchufando.
  **Lo que se movió y lo que no**, que era la parte fina: el log separaba solo
  las dos mitades —`mapear` 0 ms, `barrer` 338 ms—. El MAPEO se queda en el
  arranque (el IDE lo necesita: sin la vista publicada, `PACK_LS` dice «sin zona
  de packs»); se retrasa BUSCAR el ancla y SALTAR. Registro por setter
  explícito, no weak/strong (en ESP-IDF el override débil no se enlaza); el S3
  no registra ninguno y eso es un puntero nulo, no un caso especial.
  **El barrido NO se tocó**: sigue barriendo, que para eso existe el ancla
  («BUSCAR, no acertar la dirección»). `test_npack.c` lo dejó claro — uno de sus
  casos pone el pack en el offset 256 entre basura, así que un atajo del tipo
  «si empieza virgen no busques» contradiría el diseño. *Ese test evitó un bug.*
  ✅ **La línea del primer `Run`, verificada** (16-ago): sale
  `packs: sin pack utilizable (peldano 1)` con `log=1`.
  ✅ **Y el remate** (`2982671`): mover la carga al Run quitó los 338 ms del
  arranque pero **no los eliminó** —sin pack, la carga no se marca como hecha, y
  el barrido volvía en CADA ejecución—. Lo destapó el log de Eduardo al probarlo.
  Ahora no se barre si no hay ningún pack grabado (un `.npk` vive siempre DENTRO
  de un pack, y `bpvm_pack_scan` lo sabe leyendo la primera cabecera).
  **Verificado en placa: del último `ls` a la línea del pack, 354 ms → 17 ms.**
  ⚠️ No confundir con la idea descartada («si la zona empieza virgen, no
  busques» dentro del buscador): eso contradecía el ancla. El buscador no se
  toca; sólo no se le llama cuando se sabe que no hay nada.
  🛡️ Lo que protege `make test-packskip`: **el caso POSITIVO**. Un falso «no
  hay» dejaría un pack grabado sin cargar EN SILENCIO — se comprueba con packs
  reales (PackFixA y el SQLite.pack de 1,1 MB).
  ✅ **CERRADA DEL TODO el 16-ago**: con el `SQLite.pack` grabado, `SqlDemo` se
  ejecuta contra la base de la SD y sale `exit 0 (OK)` — 6 filas insertadas,
  agregados, agrupaciones. El pack carga en el primer Run
  (`packs: cargado, la entrada devolvio 0`), publica su API (`SQLI, 17
  simbolos`) y con pack grabado el barrido tarda **18 ms** (lo encuentra al
  principio de la zona).
- ~~`#427`~~ — ✅ **EL CENSO, HECHO el 16-ago (`e158693`): `docs/CENSO_FAMILIAS.md`.** Todo
  mecánico y con la fuente de cada dato. Lo que encontró, en corto:
  🔴 **el P4 compila a `-Og`** (sdkconfig + 18 hits en el log de build) — la
  lección del STM32-a-`-O0` repetida, y TODAS las medidas de estos días son con
  optimización de depuración; 🔴 **`#421` no llegó al STM32** (cross-family miss
  mío del 16-ago — el censo cazando lo que existe para cazar); 🔴 el
  `json_min.c` del STM32 es una copia VIEJA del parser del wire (220 vs 263
  líneas; pico≡esp32 idénticos); 🔴 el STM32 sin `LIST_DIR` (ni verbo ni .c) y
  con bucle `.mdn` propio; 🔴 el host no compila FatFs (la SD sin oráculo);
  🔴 verbos del wire dispares (SAVE/FORMAT/RENAME/RMDIR faltan según familia —
  y no hay lista escrita de cuáles son CONTRATO); 🟡 la columna STM32 sale del
  Debug/subdir.mk y trae rarezas (compila fs_host/net_host) — contrastar.
  **Los rojos quedan PRIORIZADOS en el doc, decisión ficha a ficha** (de
  Eduardo): **1, 2, 3 y 8 caben en V5**; el resto es unificación → V6.
  ✅ **Y 1, 2 y 3 SE HICIERON EL MISMO DÍA** (`ec81afc`, 16-ago 14:26): el P4 a
  `-Os` —fijado en `sdkconfig.defaults` con su porqué—, el #421 al STM32 y el
  `json_min` resincronizado (los tres md5 idénticos), verificado con el build
  headless. **De este censo sólo queda el 8**, y está abajo con entrada propia:
  enterrado dentro de una ficha CERRADA no lo veía ningún barrido.
  ⚠️ Con esto cae también la alarma de *«todas las medidas llevan optimización de
  depuración»*: sólo afecta a lo medido **hasta el 16-ago a mediodía**.
  *(El enunciado y el método, abajo.)* 🔎 **EL CENSO DE LAS FAMILIAS.** Decisión de Eduardo (16-ago): *«lo mejor
  sería revisar todas las familias e imágenes; eso nos daría un censo real de
  cómo está el código. La unificación y racionalización es la tarea de V6, pero
  lo que vayamos adelantando bienvenido sea»*.
  ⏱️ **Cuándo**: antes de documentar y finalizar V5. El CENSO es de V5; la
  UNIFICACIÓN que salga de él, de V6.

  **Por qué, y no es una intuición**: en dos días salieron CUATRO fallos del
  mismo tipo, y los cuatro persiguiendo otra cosa:
  | | qué no había viajado |
  |---|---|
  | `#398` | el corte del CRC de la SD estaba **sólo en el Pico** → la P4 pagaba 5,3 s por refresco |
  | `#423` | el Pico **nunca migró** al log común → fue la única familia que no compiló |
  | H7 | la decisión de cargar el pack en el `Run` **sólo llegó al Pico** → 338 ms y riesgo de regrabar |
  | (fachada) | `read_at` no miraba la zona de packs → un módulo del pack no se podía cargar |
  Los tres primeros son «esto está en una familia y no en otra». El cuarto es su
  pariente: «dos piezas correctas que nunca se juntaron». Encontrarlos de
  casualidad no escala.

  **MÉTODO — mecánico donde se pueda, que un censo a ojo vale lo que la atención
  del que mira** (y ya hay precedente: [censar por la primitiva, no por el
  nombre] dejó escapar #355 dos veces):
  1. **Qué ficheros del común compila cada imagen**, sacado de los
     `CMakeLists`/`Makefile`, no de la memoria. Punto de partida: pico 49 refs a
     `src/`, P4 52, S3 47 — *esas diferencias son la lista de sospechosos*.
  2. **Qué lleva cada familia por su cuenta**: pico 30 `.c` propios, S3 12, P4
     10, STM32 13. Los nombres gemelos (`log.c`, `pack_*.c`, `board_mgr_*.c`,
     `repl_*.c`) son candidatos a copia divergida.
  3. **Qué verbos del wire implementa cada REPL** (`grep` de los
     `strcmp(type, …)`): el protocolo dice ser UNO, y ya se sabe de al menos dos
     que sólo están en el Pico (`SD_INFO`, `SD_MOUNT`, ficha de la cola de H2).
  4. **Qué símbolos del común usa cada objeto** (`nm` de los `.o`), que es lo que
     distingue «lo compila» de «lo usa».
  5. Y las **imágenes**: qué familia puede alojar un pack nativo (el S3 no tiene
     `bios_s3.c`), qué flags lleva cada build ([flags-de-build-por-familia]:
     el STM32 se publicó a `-O0` toda V4).

  **Entregable**: una tabla en `docs/` — capacidad × familia, con tres estados:
  *del común* / *copia propia* / *no lo tiene*. Lo que salga en rojo se decide
  ficha a ficha; lo que se pueda adelantar en V5, se adelanta.
  ⚠️ Y el censo NO es la unificación: mezclar las dos cosas es como esta tarea
  se convierte en un refactor de tres semanas a las puertas de cerrar una
  versión.
- ~~`#415`~~ — ✅ **CERRADA el 17-ago y VERIFICADA EN LA METRO**: `/lib` pasó de
  14 a 16 módulos, con `Math.mod` (2410 B) e `IO.mod` (2491 B) preinstalados y
  con el tamaño correcto. **La stdlib BASE ya es la misma en las tres.**
  A la Metro le faltaban `Math` e `IO`, así que el mismo `import Math` iba en el
  P4 y fallaba en la Metro hasta subir el módulo a mano — un agujero justo en la
  promesa del lenguaje. Añadidos a su imagen (blobs generados con `xxd -i` desde
  `bpstdlib/*.mod`, como los otros catorce; +10 KB de UF2). Comprobado por
  comparación de las tres tablas: **14 módulos comunes** y la Pico sólo añade
  `Neopixel`, que es suyo.

  ⚠️ **Y NO era tarea de placa**, aunque estuviera en esa lista: se contesta del
  árbol. Sólo la verificación final lo es (flashear y ver los dos en `/lib`).

  📌 **Dos avisos que salieron al hacerla, y el segundo es de método:**
  - Los blobs son GENERADOS: se rehacen con `xxd -i`, nunca a mano. Y hay que
    mirar que el `.mod` de origen esté al día — aquí se comprobó contra el blob
    del ESP32 (`io_mod_len = 2491` = el tamaño del `.mod`), que estaba al día.
  - **El censo que hice primero MINTIÓ**: usé el patrón `[A-Za-z]+\.mod` y eso
    **descarta en silencio todo nombre con un dígito**, o sea `I2c`. Dije que
    faltaba en el ESP32 y el STM32 cuando estaba en las tres. Lo pilló Eduardo
    con la memoria del sensor de humedad delante: *«I2C tiene que estar en todas,
    y me extraña que no esté porque en su día lo estuvimos probando»*. Es la
    misma familia que [[censar-por-la-primitiva-no-por-el-nombre]]: un censo que
    se come casos sin decirlo es peor que no tenerlo, porque da confianza.
- ~~(sin número)~~ — 🟡 **HECHO en código el 18-ago; falta flashear.** La **media
  flash del P4**: 32 MB físicos con el bootloader configurado para 16. Estaba
  aparcado desde el 12-ago *«porque exige reflashear el bootloader»*, y Eduardo:
  *«debería ser razonable de arreglar»*. Lo era — y **el trabajo estaba medio hecho
  de antes**: la tabla `partitions_32m.csv` ya existía. Sólo faltaba apuntar a ella
  y subir el tamaño (`FLASHSIZE_32MB` + `PARTITION_TABLE_CUSTOM_FILENAME`).
  📏 **Lo que gana**: `bpdata` (FS + packs) pasa de **10.144 K a 26.528 K** — 16 MB
  más de datos. La app se queda igual (6 MB, 79 % libre).
  ✅ Verificado en la tabla **generada**, no en el `.csv`.
  ⚠️ **AL FLASHEAR: bootloader + tabla + app, los tres.** El tamaño vive en la
  cabecera del BOOTLOADER, así que reflashear sólo la app deja el límite viejo y la
  flash de arriba **no responde: se escribe y no se guarda**. Es la trampa de #328,
  que se manifestó como «littlefs CORRUPT».
  🛡️ Y si pasa, ahora se ve: el guardián de `board_mgr_esp32.c` compara configurada
  contra física y avisa — *«EL BOOTLOADER USA MENOS FLASH DE LA QUE HAY»*. Va al
  log, que además desde hoy sobrevive al reset.

#### Pulido (no urgente) — subidos desde `PENDIENTES` el 17-ago  *(archivadas)*

- ~~**El «pwm» del arranque y el del INFO no son la misma unidad**~~ — ✅ **CERRADA
  el 18-ago: ahora cada cifra DICE de qué es.**
  El log de boot decía `pwm=12` (SLICES, de `board_desc`) y el INFO respondía `24`
  (SALIDAS: cada slice tiene canales A y B) para la MISMA placa. Las dos correctas,
  pero puestas una al lado de otra parecían contradecirse — pasó el 17-ago.
  ✅ Arreglo, en los dos lados: el banner dice `pwm=12 slices` y el diálogo del IDE
  separa las líneas con su unidad — `PWM: 24 salidas` / `ADC: 8 canales`.
  📐 **Y salió una comprobación que la ficha no pedía**: el campo del wire se llama
  `pwmSlices` por historia, así que había que ver qué mete cada familia. **Las tres
  mandan SALIDAS** (Pico 24 · ESP32 8 · STM32 28): el wire era coherente y el único
  descuadre estaba en el banner. Si alguna hubiera mandado slices, el arreglo
  habría sido otro — por eso se miró antes de escribir la unidad.
  🖼️ Verificado **viendo la salida**, no leyendo el código: el formateador del
  diálogo es estático, así que se le pasaron los datos reales de las tres familias
  y se leyó lo que sale. Firmware y fat-jar reconstruidos.
- ~~`#439`~~ — ✅ **CERRADA el 18-ago: el log SOBREVIVE al reset, PROBADO EN PLACA (P4).**
  🩸 **EL LOG NO SERVÍA CUANDO LA PLACA SE COLGABA**, que es justo cuando más falta
  hace. Vivía en RAM y llegaba a flash sólo en los `log_flush()` de puntos concretos
  (fin de arranque, algunos errores); un `for(;;)` o un bucle infinito dentro del GC
  dejaban la autopsia CIEGA — al resetear, la cola del log era la del arranque anterior.
  **Anotado el 17-ago por la mañana** al no poder ver por qué se colgaba la Metro con
  `#430`… y no se abrió ficha. **Por la tarde volvió a morder** con el cuelgue del P4
  (Eduardo: *«el log no funciona si el programa se cuelga, eso ya lo sabemos de todas
  estas pruebas, así que no sirve»*), y esa vez costó una vuelta entera de hipótesis que
  no se podían comprobar. Un instrumento que falla exactamente en el caso que motiva su
  existencia no es medio instrumento: es una trampa, porque uno cuenta con él.

  ✅ **EL ARREGLO — la región del log vive en RAM QUE NO SE BORRA.**
  📐 **La idea es de Eduardo y cambió el diseño entero**: *«había una zona de RAM que se
  mantenía, igual se puede utilizar de pequeña caché para no tener que grabar todo cada
  vez en la flash»*. Existe, y el propio SDK de la Pico la usa igual (el token mágico
  del doble reset).
  🩸 **Y evitó un destrozo.** El plan era *«flush por línea»*: eso es un `erase+program`
  de 4 KB **por línea** — no «un poco más lento», sino gastar el sector, porque la flash
  aguanta ~100k borrados y un programa que loguee en bucle se los come en minutos. Con
  RAM que no se borra: **cero desgaste, cero coste**. La decisión de coste de Eduardo
  (*«si está activo y va un poco más lento es que estamos haciendo una traza»*) resolvía
  el compromiso por POLÍTICA; el arreglo lo dejó sin compromiso que resolver.
  📌 **Cómo sabe la región que es válida**: su cabecera vive DENTRO
  (`[magic|version|size][datos]`), así que se reconoce sola. Sólo hacía falta mantenerla
  al día en RAM — antes se escribía únicamente en `log_flush`.
  🔬 **Verificado en el `.elf` de cada imagen** (no en el fuente): Pico `bplog_region`
  4 KB en `.uninitialized_data` · S3 4 KB y P4 8 KB en `.noinit` · STM32 8 KB en
  `.noinit` — **las cuatro con ALLOC y SIN LOAD**. Y `s_used`/`s_dropped` siguen en
  `.bss` y sí se borran: por eso el tamaño se recupera de la cabecera y el contador del
  anillo viaja en su campo `reserved` (si no, la autopsia diría que no falta nada cuando
  faltan líneas — la mentira que #433 vino a quitar).

  🧪 **LA PRUEBA EN PLACA (P4, 18-ago)** — `samples/CuelgaLog.bp`: RUN → **4 min 30 s
  girando** en un `while true` → `kill` → `reset` del IDE. Al volver, segunda línea del
  arranque: `log: RAM SUPERVIVIENTE (lineas de ANTES del reset)`, y detrás la sesión
  entera, incluidos los **269 segundos de silencio** entre `[94888]` y `[364487]` que
  son el cuelgue. Vale como prueba porque `bpvm_log_init` mira la cabecera de RAM
  **antes** que el flash y sale por ahí (`bpvm_log.c:99`): da igual que un `ls` haya
  volcado por el camino. En la misma vuelta el pack cargó entero (`sqlite 3.53.4`,
  `vfs 'bp' registrado`, `rc=0`), o sea que el peldaño 5 del P4 quedó sano de paso.

  🗣️ **La línea de origen no era adorno, fue LO QUE HIZO POSIBLE LA PRUEBA.** El
  arranque dice de dónde viene lo cargado («RAM SUPERVIVIENTE» vs «arranque en frío»),
  y sin eso los dos primeros intentos habrían pasado por buenos siendo inútiles.
  🩸 **Costó tres intentos, y la trampa fue la misma dos veces: una medida que no
  desempata.** (1) La línea de origen sólo existía en `pico/main.c` — el P4 ni podía
  contestar; añadida a las cuatro imágenes. (2) Después, dos resets salieron `arranque
  en frío` **sin que eso significara fallo**, porque las lecturas «el mecanismo está
  roto» y «has usado el reset equivocado» explicaban el log igual de bien. Lo desempató
  el `resetReason` del INFO, que YA EXISTÍA: decía `power-on`. Instrumento que ya
  estaba, pregunta que no se le había hecho.

  ⚠️ **SOBREVIVE A UNOS RESETS Y A OTROS NO, Y CAMBIA POR FAMILIA.** En ESP32 aguanta
  `software` (el `esp_restart()` del verbo `RESET`), `panic/exception` y los dos
  watchdogs, pero **no** `power-on` — que incluye desenchufar **y el botón RST de la
  placa**, porque tira del pin EN y corta el dominio digital. En el RP2350 la RAM sí
  aguanta el pin de RUN (de eso vive el doble-tap del SDK), así que en la Metro el botón
  físico sirve. Del STM32 no está comprobado. Cara al usuario en `PENDIENTES.md` (L15).
  ⚠️ El `.ld` del STM32 lo genera CubeIDE: si se regenera el proyecto, la sección
  `.noinit` se pierde **en silencio**. Avisado dentro del fichero.
  ⏭️ **Queda la misma vuelta en Metro y STM32.** El código está verificado en el `.elf`
  de las cuatro imágenes y probado en placa en una; lo que falta es repetirlo.
  💡 **Idea que sobrevive a la ficha** (no hecha): que las líneas de diagnóstico puedan
  salir TAMBIÉN por el wire como eventos `OUTPUT` mientras hay un RUN vivo, reusando el
  camino que ya funciona — el `print` del programa sí llega con la placa colgada. Eso
  daría diagnóstico EN DIRECTO, no autopsia.

#### Lenguaje y VM  *(archivadas)*

- ~~(sin número)~~ — ✅ **CERRADA el 18-ago** (`compat` 37 PASS): **`SyncList` ya está
  en `Collections`**, que es donde Eduardo la quería. Con esto el reparto que pidió
  queda completo: `List` en `Core` (tipo básico, y el `Map` la usa), `SyncList` y
  `OwnerList` en `Collections`, y **el compilador no sintetiza ninguna**.
  🧪 `samples/SyncXMod.bp`, en el corpus. **Lo que prueba no es que compile**: lo que
  se movió fue el sitio de la clase, y lo que podía romperse en silencio era el
  CERROJO — su `super.add(...)` ahora cruza de módulo. Una lista sin candado no falla
  al usarla, falla cuando dos hilos la tocan a la vez y a veces. Por eso el sample
  lanza **4 hilos × 250 vueltas** y comprueba el total: **1000 de 1000**, en las dos
  VMs.
  🩸 Y una lección repetida: los tres samples míos de hoy (`CastExt`, `ListaBp`,
  `ListaHer`) **pasaban contra un `Collections.mod` rancio** que aún tenía los
  envoltorios. Al refrescarlo salieron 3 SKIP de golpe. El artefacto viejo no da
  error: da un verde que no vale.
- ~~`#450`~~ — ✅ **CERRADA el 18-ago** (`compat` 37 PASS): **el compilador YA NO sintetiza `List`, `SyncList` ni `OwnerList`.**
  Encargo de Eduardo (18-ago), hecho: las tres están escritas en BP. `List` y
  `SyncList` en `Core`, `OwnerList` en `Collections`. Un programa las sigue usando
  **sin un solo import**, y `l.add(42)` envuelve solo por la sobrecarga.
  La razón que hubo para sintetizarlas está en el propio emisor —*«a cambio
  cualquier programa puede usarlas sin import explícito»*— y hoy la da el import
  implícito. El precio que se pagaba: **cada módulo llevaba su propia copia**.
  📐 Tres cambios acoplados (a medias no compila): el emisor deja de sintetizar
  (los cuerpos se quedan comentados como referencia), el semántico deja de
  registrar los `ClassSymbol` builtin, y los nombres se aliasan sin cualificar como
  ya se hacía con `Exception`. `Core` pasa a importarse **siempre**: desde que `List` y
  los envoltorios viven ahí, detectarlo exigiría buscar identificadores en las
  expresiones — censar por el NOMBRE, que aquí ya ha salido mal. El `Core.mod` está
  preinstalado en las tres familias, así que el micro no carga nada nuevo.
  📏 **El coste, medido**: `Core.mod` pasa de **2.576 a 8.306 bytes**.
  ✅ Verificado: **compat 36 PASS**, la stdlib entera reconstruida, los blobs
  embebidos regenerados en las tres familias y **el firmware de la Pico enlazado**.
- ~~`#451`~~ — ✅ **CERRADA el 18-ago** (`compat` 37 PASS): **`super.metodo()` ya
  cruza módulos.**
  ```
  public class Sub extends BaseMod.Base
    public function pon(x: integer)
      super.pon(x * 2)     ← RuntimeException: «Funcion no encontrada: Base.pon»
  ```
  📐 **La causa no es el nombre, es la ABI**: un módulo **no exporta sus métodos**
  (sólo `__init` y los `__cls_new_`/`__cls_init_` — comprobado en los EXPORTS del
  `.mod`). A un método se llega por **vtable**, así que un `super` cross-module no
  tiene símbolo al que llamar. Probé a cualificarlo de dos formas y las dos fallan
  más abajo: no es un fallo de nombre.
  ⚠️ **Le pasa a cualquiera** que extienda una clase importada y quiera delegar en
  la base, no sólo a la stdlib. Y revienta con traza de Java en vez de dar un
  diagnóstico.
  📌 Consecuencia inmediata: **`SyncList` está en `Core` y no en `Collections`**, que es
  donde Eduardo la quiere — sus métodos con cerrojo llaman a `super.add(...)`.
  ✅ **ARREGLO — y la pista la dio Eduardo**: *«si declaras una clase que hereda
  de otra, aunque no lo escribas, se hace la llamada al constructor de super»*.
  Esa SÍ cruzaba, porque el constructor tiene una **factoría exportada de nombre
  plano** (`__cls_init_<Cls>`). La respuesta era darles a los métodos la suya:
  `__cls_m_<Cls>_<metodo>`, pública, que hace el CALL local no-virtual. **Mismo
  mecanismo, y ADITIVO** — añade exports, no mueve ninguno, así que ningún `.mod`
  ya compilado cambia. Sólo para métodos **declarados en la clase** (`astNode !=
  null`): generar factoría de los heredados reventaba con «Función no encontrada:
  Base.toString», porque no hay implementación local a la que llamar.
  🧪 `samples/SuperExt.bp` + `SuperExtBase.bp` (el par: sin dos módulos no hay caso),
  en el corpus. El control va dentro: `super` (10), directo (7) y polimórfico (6).
  ⏭️ **Queda mover `SyncList` a `Collections`**, que ya es posible — dos intentos de
  cirugía de texto salieron mal y se revirtieron; se hace con calma, es un
  cortar-pegar de una clase y dos líneas de alias.
- ~~`#449`~~ — ✅ **CERRADA el 18-ago** (absorbida por #450): **`OwnerList` SÍ se puede escribir en BP; NO hace falta sintetizarla.**
  Eduardo, 18-ago: *«SyncList y OwnerList deberían estar en collections. Hacerlas
  sintetizadas me parece raro, no veo la razón»*. Yo había dicho que `OwnerList` era
  la excepción —que exigía `setFieldOwner` y `FREE_REF`, sin sintaxis en BP—. **Era
  falso**, y `samples/OwnerBp.bp` lo prueba:
  · `var owner items: Object[]` **emite `SET_FIELD_OWNER`** (visto en el
    desensamblado, no en que compile): el bit de propietario del descriptor —la
    clave de la cascada— se pone desde BP;
  · liberar UN elemento suelto sale con un `var owner` **local**, que emite `FREE_REF`
    al salir del scope. Misma semántica, escrita de otra forma.
  🧪 Control de que la liberación OCURRE: el guardián de fin de RUN (#339) dice
  **«0 bloques sin liberar»**. Sin él, un `removeAndFree` que no liberase nada saldría
  igual de verde. Paridad byte a byte, en el corpus.
  ⏭️ **Con esto el reparto que pidió Eduardo es alcanzable entero y sin tocar el
  lenguaje**: `List` en `Core` (una clase, no engorda), `SyncList` y `OwnerList` en
  `Collections`, y el compilador deja de sintetizar las tres. Lo que queda es
  quitar la síntesis y que los símbolos vengan de sus módulos (alias sin cualificar
  como ya se hace con `Exception`, + import implícito).
- ~~`#446`~~ — ✅ **CERRADA el 18-ago** (las dos mitades: la segunda la hizo #450): **los envoltorios viven en `Core`.** Primera mitad del
  encargo de Eduardo (*«la list sintetizada debería desaparecer y utilizar la de
  Core»*), hecha y verde el 18-ago: `Comparable` + `Integer/Long/Double/Float/Boolean`
  están en `Core`, y con ellos `formatDouble`/`longToString` (los usa el `toString` de
  `Double`/`Float`, y `Core` no puede importar `Str`: sería circular). `Str` queda de
  **fachada** con los mismos nombres públicos, así que nadie se rompe.
  📐 **Y NO valía el atajo** de poner `"" + x` en vez de `doubleToString`: medido,
  coinciden en lo normal pero dan `1E12` y `1E-9` donde el otro da `1000000000000`
  y `0`. Habría movido la salida.
  🧱 **El muro para la segunda mitad**, medido al intentarlo: en cuanto `Core` define
  su `List`, el emisor deja de sintetizarla (bien) pero **sigue sintetizando
  `OwnerList`/`SyncList`, que la extienden** → *«Clase padre no declarada: List»*. Y
  `OwnerList` **no puede escribirse en BP**: necesita `setFieldOwner("items")` y
  `FREE_REF`, que no tienen sintaxis (el `var owner` es diseño de V6).
  ⏭️ **Los dos caminos que quedan**, los dos de emisor:
  1. que `OwnerList`/`SyncList` sintetizadas extiendan la `List` **externa** de `Core`
     (la maquinaria existe: `ExternalParentLayout`, la que usa una clase de usuario
     que hereda de una importada; hay que dársela a la síntesis);
  2. o sintetizar las tres **sólo al compilar `Core`**, donde `List` es local, y que el
     resto de módulos las tomen de su interfaz.
  El cuerpo de la `List` en BP ya está escrito y probado — es `samples/ListaBp.bp`,
  que corre en las dos VMs.
- ~~(sin número)~~ — ✅ **CERRADA el 18-ago: no era el `Map`, era el SAMPLE.**
  Eduardo: *«el punto 8 es nuevo, ¿qué pasa con Map?»*. Nada — `MapNumTest` (mismo
  `Map`, claves `Integer`) pasaba con paridad. Acotado con un reproductor de dos
  líneas: `"x" + o` con `o: Object` funciona si lleva un OBJETO (despacha `toString`)
  y **lanza si lleva una CADENA** — el hermano documentado de #389, no hay vtable
  que despachar. `Wrap8Test` concatenaba `m.get(...)` a pelo, el modismo de ANTES de
  que `Object` fuera clase real; en V4 esa línea imprimía **el handle en silencio**
  (el `376` medido en `OBJECT_COMODIN.md`), o sea que el sample llevaba mal desde
  siempre y #389 lo hizo VISIBLE. Arreglo: `string(m.get(...))`, el patrón que ya
  usaba `MapNumTest`. Verificado: 26 líneas, paridad byte a byte, el `Map` iterando
  sus claves `Long` en orden numérico de 64 bits.
  📌 Si algún día se quiere que `"x" + objeto-con-cadena` funcione a pelo (las VMs
  PUEDEN distinguir el bloque), es una decisión de LENGUAJE de Eduardo — no un bug.
- ~~`#447`~~ — ✅ **CERRADA el 18-ago** (`compat` 35 PASS): **convertir un `Object` a LA
  PROPIA CLASE, desde dentro de un método suyo, reventaba el compilador.**
  ```
  public function comparar(other: Object): integer
    var o: Cosa := Cosa(other)      ← RuntimeException: «Clase 'Cosa' no declarada»
  ```
  📐 **Causa**: el descriptor de una clase se registra en `endClass()` —su tamaño
  depende del número de métodos—, así que mientras se emiten SUS métodos el
  símbolo todavía no existe.
  🩸 **Lo grave no es el crash, es lo que tapaba**: eso es exactamente lo que hace
  el `compareTo` de los envoltorios (`var o: Integer := Integer(other)`), o sea que
  **`Collections.bp` llevaba sin poder recompilarse desde #389** (16-ago) y nadie se
  había enterado — porque su `.mod` ya estaba hecho. Un artefacto rancio tapando que
  el fuente ya no compila, que es la quinta mordedura de esa familia en el
  proyecto. Se descubrió de rebote, al mover los envoltorios a `Core`.
  ✅ **Arreglo**: aplazar el operando (placeholder 0 + fixup) y parchearlo al
  cerrar el módulo, junto a los saltos, cuando ya están todos los descriptores.
  🧪 `bpgenvm-c/samples/CastSelf.bp`, en el corpus. Lleva el gemelo *desde fuera de
  la clase* como control —ese camino ya funcionaba— y un cast que TIENE que
  lanzar, para que el aplazamiento no se coma la comprobación.
  🔁 Y la verificación que de verdad lo cierra: **la stdlib entera se reconstruye
  sin errores**, cosa que antes de esto era imposible.
  ⚠️ De paso, una trampa de build anotada: el fat-jar del frontend **empaqueta su
  copia de miVM**, así que tocar `ModWriter` y hacer `install` sin `clean` deja el jar
  con la versión vieja — el error seguía saliendo con el arreglo ya escrito, y los
  números de línea de la traza no cuadraban con el fuente. Es la trampa del
  fat-jar del IDE, un piso más abajo.
- ~~`#443`~~ — ✅ **CERRADA el 18-ago** (`compat` 31 PASS): **`newObjArray(n)` y
  `growObjArray(a, n)`**, los allocators públicos de arrays de REFERENCIAS.
  Hasta hoy sólo estaba `__newRefArray`, interno y **mintiendo en su tipo** (declaraba
  `integer[]`), así que un array de objetos sólo se podía crear con un LITERAL — o
  sea con los elementos ya sabidos. Sin constructor por tamaño no hay lista
  dinámica, y eso era lo que impedía sacar `List` del compilador.
  📐 **No son builtins nuevos**: son un **segundo nombre** de `NEW_REF_ARRAY` y
  `GROW_REF_ARRAY`, con tipo `Object[]`. Alias y no entrada de enum **porque el id es
  `ordinal()`**: una constante nueva se habría llevado un id que ninguna VM conoce y
  habría que implementarlo dos veces para no ganar nada. Así el bytecode emitido
  es el de siempre y **las VMs no se tocan**.
  🩸 Un detalle que costó un intento: el registro va **donde `objectCls` ya existe**,
  no con los demás builtins. `Object` es una CLASE de verdad desde #389, y el
  semántico distingue `any[]` de `Object[]` — lo dijo él solo al intentarlo.
  🧪 `bpgenvm-c/samples/ObjArray.bp`, en el corpus (31 PASS). Comprueba que reserva
  por tamaño, que **las casillas arrancan a null** (no con basura, que es lo que
  decide si el GC puede trazarlas) y que el downcast saca lo que se metió.
- ~~`#444`~~ — ✅ **CERRADA el 18-ago** (`compat` 33 PASS): **el downcast a una clase
  de OTRO MÓDULO ya comprueba en vez de reventar el compilador.**
  Encontrado el 18-ago al escribir `List` en BP, que es lo que #443 desbloqueaba.
  Reproductor de seis líneas, y el gemelo que lo acota:
  ```
  var c: Local := Local(o)                            -> compila (clase LOCAL)
  var c: Collections.Integer := Collections.Integer(o) -> RuntimeException:
       «Clase 'Integer' no declarada para CHECKCAST»  (traza de Java, no un error)
  ```
  Es la mitad DINÁMICA de #389 (opcode `CHECKCAST`, cerrada el 16-ago): busca el
  descriptor en la tabla LOCAL, y una clase importada no lo tiene ahí — construirla
  sí funciona porque eso va por el módulo de origen.
  ⚠️ **Y bloquea justo el camino elegido**: con `Object` de comodín, sacar un escalar
  es `Collections.Integer(o).value()` — o sea un downcast cross-module en cada uso.
  📐 **El molde ya existe**: `TRY_BEGIN_EXT` (BUG-2) resuelve una clase de otro módulo
  con el **nombre cualificado y el `clsOff` parcheado en link-time**. Un `CHECKCAST_EXT`
  con esa misma forma es trabajo conocido, pero toca **las dos VMs y el enlace**,
  así que es decisión de alcance.
  ⏳ Sin medir: si `INSTANCEOF` (#52) tiene el mismo hueco — usa la misma búsqueda,
  pero **no lo he comprobado** y no lo doy por sabido.
  🚨 Aparte del alcance: que sea un **crash con traza de Java** y no un diagnóstico
  hay que arreglarlo igual, se implemente o no el `_EXT`.
  📐 **MEDIDO el 18-ago: «los envoltorios al Core» NO esquiva este bug.** Eduardo
  eligió esa salida para evitar el cruce de módulo, así que se probó de verdad
  (movimiento hecho, compilado, y **revertido** al ver el resultado). Lo que arrastra:
  1. Los envoltorios extienden `Comparable` → se va con ellos.
  2. `NaturalComparator` hace `Comparable(a)` — **un downcast**. Al quedarse en
     `Collections` con `Comparable` en `Core`, ese downcast pasa a ser cross-module y
     **revienta el compilador igual**: *«Clase 'Comparable' no declarada para
     CHECKCAST»*. O sea que el bug no se esquiva: **se mete en la stdlib**.
  3. Para evitarlo hay que mover también `NaturalComparator`, y con él su base
     `Comparator`.
  4. Y `StringComparator` usa `Str`, así que ponerlo en `Core` haría que **el módulo
     base dependa de `Str`** — inversión de capas.
  💰 **Y el coste, que toca el criterio de Eduardo** (*«la base es FINITA: no
  ¿es útil? sino ¿lo paga todo el mundo?»*): `Core` se importa implícitamente y viaja
  **embebido en las imágenes de las cinco familias** (`pico/core_mod.c`,
  `esp32/main/esp32_mods.c`, …), así que engordarlo lo paga hasta el micro más
  pequeño, y obliga a regenerar los blobs de todas.
  ✅ **ARREGLO: opcode `CHECKCAST_EXT` (0xB0)**, hermano de `CHECKCAST` con el
  `cls_off` a **i32** y parcheado en link-time por el nombre cualificado.
  🟢 **Lo que lo hizo pequeño**: reusar la subsección de fixups que ya existía
  para `TRY_BEGIN_EXT` (§4.4 del `.mod`, la llamada *eh-class*, que **de excepciones
  no tiene nada**: parchea un i32 en una dirección de código). Resultado: **ni el
  formato del `.mod` ni los dos loaders cambian** — sólo el opcode en las dos VMs y
  una rama en el emisor. Incluye el camino frío de XIP, igual que su hermano.
  📌 Un matiz de diseño: en `CHECKCAST_EXT` el `cls_off == 0` **no** es el centinela
  de cadena. Una cadena no vive en otro módulo, así que `string(o)` sigue por el
  0xAF de siempre.
  🧪 `bpgenvm-c/samples/CastExt.bp` en el corpus. **El control va DENTRO**: el caso 3
  es un downcast que TIENE que fallar (un `Long` bajado a `Integer`), porque un chequeo
  que nunca dice que no no comprueba nada; y el caso 4 es el mismo fallo con una
  clase LOCAL, para que si los dos caen se vea que el roto es el chequeo entero y
  no la variante nueva. El mensaje sale byte a byte igual en las dos VMs.
  🏁 **Y la prueba de que servía para algo**: `samples/ListaBp.bp` — la `List`
  escrita EN BP con el `add` sobrecargado de Eduardo, que era lo que #443 y #444
  bloqueaban entre los dos. Mete integer/long/double envueltos por la sobrecarga y
  cadena/objeto tal cual, crece de 4 a 48 sin perder nada, y sale byte a byte
  idéntica en las dos VMs. **El traslado de `List` a `Core` ya no tiene bloqueo
  técnico** — lo que queda de esa decisión es de alcance.
- ~~`#442`~~ — ✅ **CERRADA el 18-ago** (`compat` 30 PASS): **un literal de array
  guardaba siempre 4 bytes por casilla.**
  Medido el 18-ago al preguntar Eduardo *«no entiendo por qué no podemos declarar
  un array de objects, es una limitación bastante tonta»*. Y tiene razón en que es
  tonta, pero el hueco **no es de los objetos**: es de los literales, y se lleva
  por delante todo elemento de 8 bytes.
  ```
  var i: integer[] := [10, 20, 30]              -> i[1] = 20     ✅ el control
  var l: long[]    := [10000000000L, ...]       -> l[1] = 0      🔴 EN SILENCIO
  var d: double[]  := [1.5d, 2.5d, 3.5d]        -> revienta
  var s: string[]  := ["uno", "dos"]            -> «No space in heap»
  var a: Caja[]    := [Caja(7), Caja(8)]        -> INVOKE_VIRTUAL sobre null
  ```
  **Las dos VMs dan lo mismo** → es del compilador, no divergencia. Y el `long[]`
  devuelve un **0 plausible sin decir nada**, que es la familia de #385.
  📍 **La causa, y el emisor la confiesa** (`MivmEmitter.emitArrayLit`):
  ```
  w.emit(OpCode.NEWARRAY);   // sin ancho de elemento
  // TODO: coerce a tipo del elemento si supieramos el tipo array de contexto.
  w.emit(OpCode.ASTORE);     // SIEMPRE 4 bytes
  ```
  🟢 **Y ese TODO está DESFASADO: el tipo sí se conoce.** `analyzeArrayLit(al, scope,
  expected)` lo calcula y queda en `info.exprTypes`. Además ya existen las dos piezas
  que hacen falta: `astoreOpForElement` (que **sí** mira `occupies8Bytes`) y
  `newarrayOpForElement` (que dice ser su «espejo» pero **le falta esa rama**: sólo
  contempla `long`/`double`, no las referencias).
  ⚠️ **Lo que NO es**, comprobado para no arreglar lo que no está roto:
  · los arrays de referencias **funcionan** si los crea un builtin — `split()` devuelve
    un `string[]` y `samples/SplitTest.bp` sale correcto (control);
  · la carga y el guardado de elementos **ya son width-aware**;
  · el tipo `Caja[]` **se acepta**;
  · los arrays fijos (`tipo[N]`) **rechazan** las referencias con un mensaje claro, así
    que por ahí no entra el fallo.
  ⏭️ Falta además un **`newObjArray(n)`**: hoy sólo existe `__newRefArray`, interno y
  tipado como `integer[]`. Sin él no se puede crear un array de objetos vacío, que es
  lo que impide escribir `List` en BP.
  ✅ **ARREGLO**: `emitArrayLit` usa el tipo del literal para (a) reservar con el
  ancho correcto y (b) coercer + guardar con `astoreOpForElement`, que es justo lo
  que ya hacía una asignación normal a un elemento.
  🩸 **La trampa que casi cuela, y que sólo se vio DESENSAMBLANDO**: el primer
  intento usó *«no es primitivo»* como predicado de referencia. Pero en BP
  `string` **ES** un `PrimitiveType` y a la vez una referencia de heap, así que salía
  `NEWARRAY` (4 B) con `ASTORE_I64` (8 B): el elemento 0 pisaba al 1 y el 1 se
  escribía fuera. El síntoma —`[0]` bien y `[1]` VACÍO— mandaba a mirar el GC y las
  cadenas literales, y las dos pistas eran falsas. El predicado bueno es
  `isRefType`, que ya existía y ya documenta esa excepción.
  ⚠️ Y el otro cuidado: las referencias **no van por opcode**. `NEWARRAY_I64` da un
  `TYPE_ARRAY_I64` de 8 bytes OPACOS que el GC **no traza**; un array de refs tiene
  que ser `TYPE_ARRAY_REF` (builtin `NEW_REF_ARRAY`). Confundirlos no truncaría:
  sería un use-after-free. Por eso `newarrayOpForElement` **no** lleva la rama de
  referencias, y no le falta.
  🧪 `bpgenvm-c/samples/ArrLitAncho.bp`, en el corpus de paridad (30 PASS). Cada
  ancho con su gemelo de 4 bytes como control, el borde de n=1, y presión de GC
  al final para que un array de refs mal reservado se note. **Rojo verificado**:
  sin el arreglo da `long : 0 0 5100273664`.
  🔗 Con esto, mover `List` a `Core` sólo espera a un `newObjArray(n)` público (ver la
  entrada de las listas y `docs/OBJECT_COMODIN.md`).
- ~~(sin número)~~ — ✅ **CERRADA el 18-ago vía #450**: **las listas: de `any` a
  `Object` + `add` SOBRECARGADO.**
  15 `AnyType.INSTANCE` a mano en `SemanticAnalyzer`. ⚠️ Deja a
  `samples/AnyNumGc.bp` sin sujeto.
  📐 **Dirección de Eduardo (18-ago)**: *«sobrecargamos el método add, habrá un
  `add(i:integer)`, `add(l:long)`, `add(f:float)`, etc. Los otros list igual (no sé
  si pueden heredar los add)»*.
  **Su pregunta, contestada leyendo el código** (`SemanticAnalyzer`):
  · `OwnerList` **SÍ hereda** — sólo declara `removeAndFree` propio, el resto viene
    de `List`. Gana las sobrecargas gratis.
  · `SyncList` **NO** — redeclara las cinco con las mismas firmas, **a propósito**
    («overrides explícitos para documentar que se llama la del subtipo, con
    locking»). Ahí hay que replicarlas, o dejar de redeclararlas.
  ✅ **18-ago, MEDIDO: las sobrecargas se escriben UNA sola vez.** Eduardo: *«el
  list ya está y las otras listas heredan de list»*. Cierto, y también para
  `SyncList`, que era el caso dudoso: redeclara las cinco porque las suyas llevan
  el lock, así que parecía necesitar copia de cada sobrecarga. **No la necesita**:
  si la sobrecarga delega con `this.add(o)`, esa llamada es VIRTUAL, así que basta
  con que la subclase tenga su `add(Object)` — que ya lo tiene.
  `samples/ListaHer.bp` lo fuerza: `Sub` reescribe SÓLO `add(Object)` y al llamar a
  `add(7)` (la sobrecarga HEREDADA) ejecuta la de `Sub` — también por referencia a
  la base. En el corpus, paridad byte a byte.
  ⏭️ Con eso, lo que queda de esta ficha es **dónde viven las sobrecargas**:
  · en la `List` sintetizada → el emisor tendría que construir un
    `Collections.Integer` desde código que él genera, y eso **no está probado**;
  · o `List` en `Core` → BP normal, y eso **sí** está probado hoy
    (`samples/ListaBp.bp`). Decisión de alcance, de Eduardo.
  🩸 **Y el obstáculo de fondo, que cancelar `Box` no quita sino que mueve**: una
  casilla de `List` es un **handle** (`items` es array de refs, `ASTORE_I64`, y el GC
  lo traza por el `field_bitmap`). Un `integer` NO cabe ahí, así que `add(i:integer)`
  tiene que **envolver**. Diseño y decisiones abiertas en `docs/OBJECT_COMODIN.md`.
- ~~`GAP-4`~~ — ✅ **CERRADA el 17-ago: medida, acotada y DECIDIDA.** Resultó
  ser DOS cosas distintas, y ninguna era la que decía la ficha.

  **(1) La notación científica NO diverge** — 22 casos byte a byte en host, y el
  P4 los reproduce. La ficha había nacido de leer el «TODO» castellano de un
  comentario como el marcador inglés (ver abajo).

  **(2) Pero SÍ había una divergencia, y la destapó la prueba en placa**: el
  subnormal más pequeño salía `0` en la Metro. Acotado con `SubNorm.bp`: la
  frontera es EXACTAMENTE la del formato IEEE (por debajo de `2.2e-308`), el P4
  y el host dan bien las 16 líneas, y la causa es que el SDK de la Pico
  reemplaza las rutinas de `double` por unas optimizadas que descartan
  subnormales a propósito (`double_sci_m33.S:121`, `@ flush denormal`).

  **Medido el coste de arreglarlo** (`DblBench.bp`, con control entero que salió
  IDÉNTICO al milisegundo en las dos corridas): +23 KB de flash y +24 % de
  tiempo, que es **1,8×** en la aritmética una vez descontado el intérprete.

  **Decisión de Eduardo: NO se cambia**, y documentado en `PENDIENTES.md` (L14)
  y en el manual. *«Prefiero un 25 % más de velocidad y perder un poco de
  compatibilidad que afecta al 0,01 % de los casos… `double` se va a utilizar en
  la toma de medidas que requieran precisión, pero estamos hablando de
  instrumentación donde tenemos 6 u 8 dígitos significativos como mucho.»*

  ---
  **El detalle de (1), que sigue siendo la mejor parte:** medido el 17-ago (`SciPar.bp`, ya en
  el corpus de paridad: 29 PASS). Las dos VMs dan byte-idéntico en los 22 casos,
  incluidos los extremos (`1E300`, `1E-300`, el mayor double finito, el menor
  subnormal) y los redondeos JUSTO en las dos fronteras del rango
  (`|x| >= 1e12` y `0 < |x| < 1e-6`), que es donde estos formateadores se parten.

  **La ficha nació de leer mal una palabra.** El comentario de `interp.c` dice
  *«…→ notación científica. **TODO** en aritmética IEEE determinista (solo *,/,+
  por literales exactos + cast a int64) … → byte-idéntico a
  `VirtualMachine.formatBpDouble` (Java)»*. Ese `TODO` es el **todo castellano**
  —«todo ello»—, no el marcador inglés de tarea pendiente: la frase dice que
  está hecho ASÍ, y por qué. Alguien lo leyó como un pendiente y de ahí salió una
  ficha que tocaba el invariante sagrado y no existía.

  De regalo, dos cosas comprobadas de camino: **hay un solo formateador por VM**
  (`bpvm_format_double` / `formatBpDouble`), usado por print, por el concat y por
  la conversión a cadena — no hay una segunda implementación que se pueda
  desviar; y `Str.doubleToString` **sí** da otra cosa en los extremos, pero A
  PROPÓSITO (su comentario dice «sin sci») y es código BP, así que corre igual en
  las dos VMs por construcción.

  📌 **Lo que NO cubre esta medida**: es host contra host (x86). El formateo está
  escrito para ser determinista en cualquier FPU (sólo `*`, `/`, `+` por
  literales exactos y un cast a int64), pero eso es un argumento, no una medida.
  `SciPar.mod` cuesta un minuto en una sesión de placa — **añadido a la lista de
  cuando haya placa delante**.
- ~~`N-readfile-msg-skew`~~ — ✅ **CERRADA el 17-ago** (`RfSkew.bp` en el repo):
  miVM pegaba `e.getMessage()` de Java — la ruta normalizada POR LA PLATAFORMA
  (Windows: barras invertidas), o sea distinta por SO y distinta de la VM-C.
  Gana el mensaje de la C: `readFile('...'): no se pudo abrir`. Byte-idéntico
  medido, paridad 28/0/0.

#### AOT / native  *(archivadas)*

- ~~`#440`~~ — ✅ **CERRADA el 17-ago, VERIFICADA EN EL P4** (`9d41562`).
  El `.mdn` de RISC-V direccionaba sus datos en **absoluto** → se colgaba toda
  `native` que tocara un literal. Enlazar a `-Ttext=0` deja relativos los SALTOS,
  no los DATOS: con el modelo por defecto (`medlow`) un literal sale como `lui`+`addi`
  con la dirección de enlace de constante, y el `.mdn` se carga donde caiga → puntero
  salvaje, y **cuelgue mudo, no crash**. ARM nunca lo sufrió (va con `-fpic`, remata
  con `add r1, pc`). Arreglo: `-mcmodel=medany` → `auipc`. Medido: 3 refs
  absolutas → 0. En placa, la escalera `NatEsc` pasa los **6 escalones**.
  Y detras la prueba de verdad: `AotGcRt` entero en el P4 — **10.000 vueltas,
  `malos: 0`, exit 0**. Con eso queda verificada tambien la pata del P4 de
  **#430** (la presion por tabla de handles), que estaba tapada por este bug: no
  es solo que no se cuelgue, es que las 10.000 concatenaciones dentro de la
  nativa devolvieron el valor correcto.
  **Lo reutilizable — cómo se acotó**: la escalera. Una `native` por peldaño, cada
  una exigiendo una cosa más por debajo, imprimiendo antes y después. **Una sola
  corrida da el punto de ruptura** sin ir pidiendo variantes de una en una:
  `NatMin` (sumar enteros) iba bien y el escalón 2 (devolver un literal) moría
  → el thunk estaba sano y lo roto era **tocar datos**. Antes de eso, tres teorías
  caídas por medida: el `.mdn` no era de ARM (`arch=243`, leído en su cabecera), no
  era el GC (moría en la PRIMERA llamada — lo vio Eduardo mirando el orden de las
  líneas) y no era la presión de memoria.
  **La guarda**: se cuentan las relocalizaciones absolutas del `.text` del `.o` y el
  build falla si hay alguna. En el `.o` y no en el `.elf` (al enlazar se consumen y
  las dos variantes quedan como bytes igual de plausibles) y no por desensamblado
  (un `lui` de constante grande es legítimo y no lleva reloc). `AotRiscvPicSmoke`
  la comprueba en las **dos** direcciones: una guarda que sólo se ve en verde
  podría estar contando siempre cero.
- ~~`#441`~~ — ✅ **CERRADA en V5 el 18-ago (`9fcff33`): el IDE ya compara la ARQUITECTURA
  del `.mdn`.** *(La otra mitad —los flags— se aplazó a V6 el mismo día: pide cambiar el
  formato del `.mdn`, y esta versión no toca formatos. Su texto está en «Aplazadas a V6».)*
  📐 Idea de Eduardo: *«¿los `.mdn` tienen cabecera? porque si tienen cabecera lo que
  corresponde añadir [es] ARM o RISCV»*. Y en efecto **ya la llevaban**: `arch` =
  `e_machine` del ELF (ARM 40 · RISC-V 243 · Xtensa 94 · 0 = legacy), y la placa dice
  la suya en el INFO. Lo que faltaba era que alguien **las comparara**:
  `mdnIsStale` sólo miraba fechas, así que el IDE subía tan tranquilo un `.mdn` de
  otra ISA y era el gate del loader quien lo rechazaba **ya en la placa**. Ahora un
  fallo remoto se convierte en un «regenéralo» local.
  🗣️ Y el aviso dice el motivo REAL —*«es de otra ARQUITECTURA (arm, y la placa es
  riscv)»*— en vez de *«es más viejo que su .mod»*, que sería mentira y mandaría a
  mirar unas fechas que están bien.
  🔬 **Sólo se ve en el volcado**: la cabecera del `.mdn` es **little-endian** y la del
  `.mod` big-endian. El primer lector usaba `readInt()` y habría devuelto
  `0x28000000` en vez de 40. Se cazó con `xxd` sobre un `.mdn` real.
  🧪 Control sobre ficheros de verdad, para que la prueba DISTINGA: ARM (40),
  RISC-V (243, cabecera forjada a propósito) y legacy (0, que se deja pasar igual
  que hace el loader).
- ~~`#381`~~ — ✅ **CERRADA el 16-ago, VERIFICADA EN LA METRO.** `long` en una
  función `native`. La salida en ARM real es **byte a byte la del PC**, y el IDE
  generó el `.mdn` solo (8 thunks, 560 B). Lo que confirma cada línea:
  números de más de 32 bits (`sumaL`, `cadena`), anchos mezclados en una firma
  (`mezcla`), la división y el módulo POR HELPER (`divL`, `modL`, `divNeg`), las
  conversiones en los dos sentidos (`baja0`, `baja123`, `sube`) y —el que más
  valía— **`div0: atrapado`**: dividir por cero desde código nativo lanza un
  error de BP atrapable en vez de reiniciar la placa.
  Commits: `f599574` (marshalling), `bd5002f` (división por helper), `072c864`
  (conversiones).
  *(El número lo tenía: lo decía el mensaje de error de `AotCEmitter.cTypePack`.
  Estaba archivado aquí como «(sin número) — long, double y float JUNTOS».)*
  **La corrección de Eduardo que ordenó el trabajo**: *«long es una cosa y
  double otra»*. Y la medida le dio la razón — compilando lo que emite el AOT
  con los flags reales: `long` `+ - *` no deja ni un símbolo (GCC lo hace en
  línea), sólo `/` y `mod` llamaban a `__aeabi_ldivmod`; `double` llama a
  libgcc para casi todo. Comparten el marshalling y nada más → `double` es
  `#426`.
  **Salió barato porque tres piezas ya estaban**: la pila BP ya guarda los
  `long` como 8 bytes big-endian (la misma representación que el intérprete),
  el thunk ya sabía mover 8 bytes (lo hace con las refs desde #302), y la tabla
  de helpers está hecha para crecer por el final.
  **Y la división la resolvió una idea de Eduardo**: *«reemplazarla en el emisor
  por una llamada a una función»*. No hizo falta escribir una división por
  software — **el que no puede llamar a libgcc es el `.mdn`, no el runtime**, así
  que `idiv64`/`imod64` viven en la tabla de helpers y el `.mdn` queda limpio.
  Cero cambios en el build, y vale para ARM y RISC-V a la vez. Los helpers son
  espejo EXACTO del intérprete (mismo chequeo de cero, mismo mensaje): si el
  camino compilado fuera más listo, el mismo programa daría dos resultados según
  llevara `.mdn` o no.
  **Verificado**: `make test-longnat` (nuevo) — la salida por los thunks AOT es
  idéntica a la de la VM-Java con 2^40, anchos mezclados, negativos, el máximo
  de 64 bits, llamadas encadenadas, división, módulo y **división por cero
  atrapada con `try/catch`**. Y el objeto ARM real no deja un solo símbolo
  indefinido.
  ✅ **Y el fleco, cerrado el 16-ago** (`072c864`): las CONVERSIONES numéricas
  dentro de una nativa —`integer(v)`, `long(n)`, `float(x)`—. En BP se escriben
  con el nombre del tipo, así que al emisor le llegaban como una llamada y moría
  con «función desconocida». Se emite el cast de C, que **es literalmente lo que
  hacen los opcodes del intérprete** (`OP_I64_TO_I32` es `(int32_t) v`): la
  misma conversión, no una equivalente. `double(x)` se rechaza con su motivo
  (#426) en vez del mensaje genérico.
  ⏭️ **Sólo falta PROBARLO EN PLACA.** En host está entero: marshalling,
  literales, aritmética, división, módulo, conversiones en los dos sentidos y
  división por cero atrapada — todo con salida idéntica a la VM-Java, y el
  objeto ARM sin un símbolo indefinido. Lo que la placa añade es el único paso
  que aquí no se puede dar: que el `.mdn` se cargue de verdad.
- ~~`#428`~~ — ✅ **CERRADA el 16-ago (`7ddbfec`), VERIFICADA EN LA METRO**: una
  `native` con literales de cadena compila a `.mdn` (188 B, 1 thunk) y en placa
  imprime `valor 7` / `negativo`, limpio y con `exit 0`.
  **La solución fue la de Eduardo** —*«esos literales tienen que ir como parte
  del código nativo»*—: un guión de enlace compartido (`bpgenvm-c/aot/mdn.ld`)
  fusiona `.rodata` DENTRO de `.text`; enlazado a dos direcciones distintas el
  código sale byte-idéntico, o sea que sigue siendo relocatable. En los DOS
  pipelines (IDE y `build_mdn.sh` — que además estaba ROTO desde V5 por un
  classpath incompleto y nadie lo notó: el camino de diario es el del IDE).
  `MdnPack` no se tocó: su guardián sigue vigilando `.data`/`.bss`.
  **Sin regresión**: `LongNat.mdn` regenerado con enlace = código byte-idéntico.
  ⚠️ **Matiz de honestidad, y vale también para `#381`**: la salida limpia
  demuestra que *si* el `.mdn` cargó, los literales funcionan (rotos darían
  basura, no texto limpio) — pero la salida por sí sola no distingue nativo de
  interpretado, PORQUE ESA ES LA GRACIA del degrade. La lección de #417. La
  confirmación de 30 segundos, si se quiere: repetir un Run con `log=1` y ver la
  línea del loader registrando los thunks del `.mdn`.
  *(Lo de abajo, el análisis original.)* 🟢 **CAMINO ENCONTRADO Y MEDIDO el
  16-ago.**
  **El problema, comprobado en vivo**: una `native` tan inocente como
  `return "hola" + intToString(n)` genera un `.rodata.str1.1` y `MdnPack` la
  RECHAZA — hoy **una función native no puede llevar ni un literal de cadena**,
  ni una tabla constante, ni una variable estática.
  **La solución la apuntó Eduardo**: *«esos literales tienen que ir como parte
  del código nativo»*. Y así es, con un **paso de ENLACE** (no de compilación):
  un script de `ld` que fusione `.rodata` dentro de `.text`.
  **Medido**: el `.o` en modo `--mdn` deja UNA reloc (`R_ARM_REL32` al literal);
  tras el enlace final con el script, **cero relocs**, y —la prueba que lo
  cierra— enlazado a `0x00000000` y a `0x20001000` el `.text` sale
  **BYTE-IDÉNTICO**: sigue siendo relocatable, que es lo que el `.mdn` exige.
  🔎 **Y hay una simetría que lo explica**: el `.npk` sale de un ELF ENLAZADO y
  por eso sí puede llevar `.rodata`; el `.mdn` sale de un `.o` SIN enlazar y por
  eso no. Es darle al `.mdn` el paso que al `.npk` ya se le da.
  *(Descartado: no hay directiva de compilador que lo haga — `-fmerge-constants`
  y `-fsection-anchors` no son eso. Y el plan B de Eduardo, sacar los literales
  al módulo BP y leerlos con `cs+offset`, funcionaría pero es más caro: con el
  enlace quedan resueltos en compilación y a coste cero en ejecución.)*
  ⏭️ Falta: meterlo en `build_mdn.sh` (y en el pipeline de RISC-V), aflojar el
  guardián de `MdnPack` para lo que ya venga resuelto, y una prueba en placa con
  un literal de verdad.
- ~~`#302`~~ — 🟢 **paso 3 HECHO EN HOST el 17-ago** (`make test-aotgc` de rojo a
  VERDE), **con el diseño de Eduardo**: escaneo conservador de la pila de C, en
  vez del shadow stack del plan original.
  **La implementación cupo en tres sitios**: un campo en el callctx TLS
  (`cstack_hi`, el techo que apunta `aot_call_guarded` al entrar al thunk más
  externo — con anidamiento native→BP→native gana el de fuera), el paso 2d del
  marcado (recorre `[frame del GC .. techo]` palabra a palabra dándoselo a
  `mark_recursive`, que ya validaba basura: es lo mismo que el paso 1 hace con
  la pila BP), y un `setjmp` que vuelca los registros preservados a la pila
  escaneada (el truco de Boehm — un handle puede vivir SOLO en un registro).
  De propina, `tc->sp` se sincroniza al entrar al thunk, como los 19 safepoints
  del intérprete.
  **Lo que compró frente al shadow stack**: cero cambios en el emisor, cero
  subida de ABI (los `.mdn` ya grabados quedan protegidos sin regenerar), cero
  coste sin AOT activo (callctx a NULL → el GC ni mira), y miVM ni se entera.
  **Medido**: el escaneo son ~180 palabras (~760 B) por colecta, y el rastro
  dice `1 refs` en la colecta que antes mataba el intermedio — el objeto exacto,
  protegido. Regresión entera verde (13 targets), paridad 28/0/0, la Metro
  enlaza.
  ⏳ **Falta placa**: el test es de host; en placa el mismo escenario es
  `RoTest`/`LongNat` con `log=1` mirando que el rastro `pila C del native`
  aparezca en las colectas. Va con la tanda de pruebas finales.
  *(La historia de cómo se llegó, abajo: el argumento del aplazamiento refutado
  con test el 16-ago.)*
  🔴 **paso 3 (raíces GC del native COMPILADO): EL ARGUMENTO DEL
  APLAZAMIENTO ESTÁ MUERTO, probado con test en rojo el 16-ago.**
  Se difirió con *«el native corre síncrono sin GC asíncrono y F2 no compacta»*
  — y las dos patas han caducado: el GC corre **dentro de `bpvm_heap_alloc`**
  (#357), también cuando aloca un helper llamado desde código nativo; y el GC de
  V4 **recicla** y mata handles.
  **El experimento** (`make test-aotgc`, HOY ROJO a propósito — es el criterio
  de aceptación): `"valor " + intToString(n)` en una native, con GC forzado por
  alocación. El handle de la primera alocación espera en un TEMPORAL DE C
  mientras la segunda aloca; el marcado no lo ve (ni está en la pila BP, que
  además se escanea con un `tc->sp` RANCIO: el camino AOT no sincroniza como los
  19 safepoints del intérprete) → el objeto se recicla → la concat imprime
  **doce bytes NUL con `status=OK`**. Corrupción MUDA. El control interpretado,
  con el mismo GC agresivo, imprime `valor 7` — la diferencia es exactamente el
  camino compilado. Y cae también el *«el AOT-en-host la tiene gratis»* del
  doc: esto ES host.
  **Gravedad hoy**: ventana estrecha (una colecta cada ~32 KB alocados) y los
  natives existentes apenas encadenan alocaciones… pero `#428` acaba de abrir
  la puerta a cadenas en natives, que es EXACTAMENTE el patrón vulnerable.
  💡 **Y EL ARREGLO CANDIDATO CAMBIÓ esa misma tarde, por una pregunta de
  Eduardo**: *«¿podemos alojar el código nativo en una zona que escanee el
  GC?»*. El código no contiene las referencias —están en la PILA DE C y los
  registros del hilo— pero la idea, reformulada, es **escaneo conservador de la
  pila de C** (la técnica de Boehm), y le gana al shadow stack del diseño en
  casi todo:
  - **cero cambios en el emisor y cero subida de ABI** → los `.mdn` ya grabados
    se vuelven seguros sin regenerarlos;
  - coste sólo AL COLECTAR (recorrer la pila del hilo), no por llamada;
  - **no toca miVM** (no tiene nativo compilado): la paridad ni se entera;
  - cierra LOS DOS agujeros a la vez — los intermedios en temporales de C y los
    argumentos que el `tc->sp` rancio dejaba fuera (el thunk los copió a
    locales de C, que están en la pila escaneada).
  Piezas: límites de pila por familia (FreeRTOS los SABE: es la pila de la
  tarea; en host se apunta el tope al entrar al worker), la validación de
  candidatos con la maquinaria que YA existe (`valid_map` + tabla de handles con
  generación — un falso positivo sólo retiene de más, y este GC no compacta), y
  un `setjmp` al entrar al GC para volcar los registros a la pila.
  A cambio: retención ocasional de más (aceptable) y una cintura pequeña por
  familia. El shadow stack queda como plan B si el conservador encontrara un
  muro. **El criterio de hecho no cambia: `make test-aotgc` en verde.**

#### Arrastres de V4 y varios  *(archivadas)*

- ~~`#412`~~ — **MOVIDA A V6** el 17-ago por decisión de Eduardo (*«puede ir a
  V6, no es nada urgente ni crítico»*). El diseño quedó CERRADO antes de moverla
  y está en `docs/V6_IDEAS.md`: el argumento **siempre en el heap** (idea de
  Eduardo), que además borra una asimetría de fondo — hoy el argumento horneado
  es un literal de la zona de datos y uno de ejecución sería del heap, las dos
  formas de cadena que dieron guerra en `#389`. Lo que la saca de V5 no es el
  mecanismo (una línea en el emisor + un builtin ×2) sino que **abre el
  protocolo del wire**, con cuatro implementadores.

#### Cola de H2 (la SD), anotada al cerrarlo el 8-ago  *(archivadas)*

- ~~`H2-P4`~~ — las seis operaciones que nunca se habían ejecutado.
  ✅ **CERRADA el 15-ago, VERIFICADA EN PLACA (P4) en los DOS volúmenes**:
  littlefs 10 ok + «mtime no soportado» · SD (FatFs) **11 ok con la fecha real**.
  `samples/FsOpsTest.bp` (`f7b430f`, `96af6a2`) las ejerce comprobando **el
  efecto de cada una**, no que no revienten. El volumen se elige en una
  constante (`BASE`).
  Resultado: miVM 11/11 · VM-C sobre el FS del host 11/11 · **VM-C sobre
  LITTLEFS 10/10** — esta última con `--fs=lfs:<img>`, el modo oráculo, que es
  el MISMO MOTOR que el micro. O sea que cinco de las seis ya están ejercidas
  contra el backend bueno, y sin placa.
  🩸 **Y la sexta no era lo que decía la ficha**: `mtime_ms = NULL` en
  `fs_lfs.c` porque **littlefs no guarda timestamps**. No es una operación sin
  probar: en el FS interno **no existe**, por diseño. En la SD sí
  (`fat_mtime_ms`, con fecha real). El FS del host lo tapaba, porque ahí sí
  funciona — otra vez el mismo patrón: el instrumento cómodo no es el que dice
  la verdad sobre la placa.
  ⏳ Falta en placa: el littlefs de host corre sobre una imagen en fichero, no
  sobre flash real. Y probar `mtime` en `/sd`, que es donde debe funcionar.
- ~~(de H6) — la **polaridad de Q1**~~ — ✅ **CERRADA el 18-ago: no había nada abierto.**
  Q1 **es** el MOSFET que conmuta el raíl de la tarjeta por GPIO45, o sea que «la
  polaridad de Q1» y «la polaridad de `pwr`» son lo mismo — y la línea original ya
  declaraba cerrada la segunda entre paréntesis. Error de redacción mío al archivarla,
  no trabajo pendiente. Eduardo, al preguntárselo: *«es de la lectura de la SD y el
  control de alimentación. Tal como está ahora funciona»*.
  🔬 **Y está contestada por triplicado**, que es lo que la hace cerrable sin tocar
  nada: el análisis del transistor lo dijo (canal P, fuente en 3V3 ⇒ conduce con la
  puerta baja), el código lo fija (`esp32p4/main/blk_sdmmc_p4.c:79` — *«la polaridad
  sale del ENV (`pwralto`) y es activo BAJO — cerrado en placa»*), y **cada arranque lo
  repite**: `pwr 45 (activo bajo)` seguido de `sd: montada en /sd`.
  📌 **Ojo con el 45 si alguien amplía el bus**: en esta placa NO es `d4`, es `pwr`
  (aviso dentro de `blk_sdmmc_p4.c:153`). Y no confundirlo con la retroiluminación,
  que es **GPIO26** por LEDC y tiene su propia polaridad por panel (`bl_invert`,
  invertida en la Waveshare) — resuelta aparte.
