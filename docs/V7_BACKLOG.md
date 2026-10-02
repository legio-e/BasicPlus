# V7 — el mapa del hito EN CURSO

> 🚀 **Desde el 13-sep-2026, V7 es la versión en curso** (V6 se publicó el 12-sep, tag `v6.0`,
> «orden en los micros»). Este fichero es la **puerta de entrada** al hito: un **índice**, una
> línea por asunto y un puntero a dónde está el detalle. El estado vive en **`FICHAS.md`
> §ABIERTAS** (bloque V7); los diseños, en **`V7_IDEAS.md`**.
>
> ⚠️ **NO es una fuente de verdad y no debe convertirse en una.** Si algo aquí contradice a
> `FICHAS`, manda `FICHAS`. Existe por la misma razón que `V6_BACKLOG.md`: lo aplazado estaba
> repartido en cuatro sitios (fichas abiertas, restos de fichas cerradas, el diario y los
> cuadernos de ideas) y no había forma de verlo de un vistazo.

**De dónde sale.** Del **censo del 13-sep**, hecho al abrir V7: 26 barridos sobre `FICHAS.md`
entero, `ESTADO.md`, `V6_BACKLOG`, `V6_IDEAS`, `V7_IDEAS` y `PENDIENTES` (448 candidatas en bruto,
261 únicas), y **cada candidata releída en su sitio** y buscada en todo el registro por si se
había cerrado después: 92 cayeron (cerradas, duplicadas o superadas), quedan las
de abajo. Los punteros `FICHAS:Lnnnn` son líneas del `FICHAS.md` del 13-sep (hoy, otras: se buscan por la
palabra clave o con `git show` de aquel día). Donde un hito se ha vuelto a mapear —H7, el 29-sep—, sus punteros
van a la fila del mapa en `V7_IDEAS` o al número de ficha, que no caducan.

**Recuento:** **12 hitos** (`H1`–`H12`); dentro, **18 fichas con número**, **28 restos** de fichas cerradas de V6, **~40 sueltos** sin ficha; y **6 fuera del plan** — 100 entradas en este índice.

---

## 🎯 Lo que Eduardo nombró de memoria al abrir V7 (13-sep)

Sin mirar el registro: *«la caracterización de los periféricos de cada micro, limitaciones de
los `.mod` a la hora de guardar dependencias intra-clases cuando estaban en diferentes módulos,
el sistema de archivos a la hora de acceder a una posición concreta dentro de un archivo, así
como la implementación en BP de una clase `File`»*. Es la mejor señal de por dónde empezar, y
los cuatro caen en el mismo sitio del mapa:

| lo que nombró | dónde vive |
|---|---|
| **los periféricos de cada micro** | §H4: `#470` (los cuatro números → arrays por micro), `#479` (mapa de pines y analógicos), `#378` (que cada micro DIGA lo que tiene), los restos de `#469` (el ADC del STM32) y de `#480` (el `Pulse` del C3) |
| **las dependencias entre clases de módulos distintos en el `.mod`** | §H2: `#492` (la interfaz no exporta campos), `#460` (herencia entre módulos, aparcada hasta tener versiones), `#500` (la norma de versiones) |
| **acceder a una posición concreta de un fichero** | §H3: `#491`, el cursor |
| **la clase `File` en BP** | §H3: la mitad de arriba de `#491` (`File` + `TextFile`) sobre el destructor de `H1` |

## 🧭 CÓMO SE TRABAJA V7 — decisión de Eduardo (13-sep)

- **Un hito por tema, y los hitos se llaman `H` + número** (*«prefiero que los hitos empiecen por H y numerados, así es más
  fácil hacer el seguimiento»*). Los grupos de abajo SON los hitos. `L1` pasa a llamarse `H1`.
- **`H6` LVGL en un pack** y **`H11` la ESP32-C5** tienen hito propio por decisión suya.
- **Sin orden prefijado** — *«como hay muchas cosas pendientes hay que ir cerrando algunos pendientes»*.
  La propuesta que había aquí (dependencias → módulo raíz → cursor → HAL BP → AOT) queda como sugerencia
  de por dónde tirar cuando haya que elegir, no como plan.
- **V7 será distinta: «intentaremos no dejar pendientes para V8».** Lo que se abra se cierra o se
  cancela dentro de la versión; aplazar deja de ser una salida. Y sigue la regla de V6: **una entrada es
  una cosa**, y los restos que cuelgan de una ficha cerrada se numeran cuando se les mete mano.
- **`H6` es estratégico**, y el porqué es suyo: *«Meter LVGL en un pack es estratégico, ya que hoy en día cualquier micro puede llevar una pequeña pantalla, así que le corresponde al usuario si quiere una pantalla o no, y no a nosotros»*. Es el criterio «la base es finita, lo demás en packs» aplicado a la pantalla: la
  base no decide si hay pantalla; el usuario la mete como pack, o no.
- **`H12` — los dos núcleos, al final de V7 y reversible**: *«Al final de V7 podemos hacer una prueba a ver si se puede trabajar con 2 núcleos reales: si funciona se mantiene, pero si da problemas y no somos capaces de solucionarlos, lo echamos para atrás»*. `A4` deja de estar fuera del plan; el criterio de salida queda escrito antes de empezar.
- El **P4X no tiene fecha** (pedido; llegó un P4): fuera del plan hasta que haya placa. En su lugar, la
  **ESP32-C5** — la Waveshare **ESP32-C5-Touch-LCD-2.8** (2,8", 240×320, táctil), con 8 MB de PSRAM y 32 MB de flash, comprada el 5-sep — es `H11`.

### 🤝 Lo que añado yo (13-sep) — Eduardo: *«si quieres añadir algo que consideres importante eres libre de hacerlo, a fin de cuentas somos socios»*

Cinco cosas, todas pequeñas de escribir y grandes de olvidar. Propuestas el 13-sep y **contestadas el mismo día**:
cuatro aceptadas tal cual y la primera rebajada a su tamaño — está anotado en cada una.

1. **Un cierre PEQUEÑO por hito, y uno casi completo a mitad de V7** — ✅ así lo dejó Eduardo (*«cerrar después
   de cada hito es mucho trabajo, pero se puede hacer un pequeño cierre, una ronda pequeña de test; y a mitad
   podemos hacer un cierre casi completo»*). Lo que yo proponía —`F1` entero por hito— no; lo que queda:
   - **al cerrar un hito, la ronda pequeña**: la prueba de fuego de `tanda.py` (el corpus corto) en las placas que
     el hito tocó, `compat.sh` en host, y **una prueba que EJERCITE lo nuevo** (no que lo cargue: *«confirmado al
     cargar no es confirmado en uso»*, la lección de `F1`); las fichas cerradas o canceladas con motivo; su bloque
     de este índice tachado. Los docs, cuando cambie lo que ven los usuarios, no como ceremonia.
   - **a mitad de V7, el cierre casi completo**: la ronda de `F1` entera (las nueve placas con lo construido, el
     ZIP montado y probado, las guardas) más el repaso del índice del punto 5. Es también el **punto de
     restauración** que pide `H6` antes de quitar LVGL de ninguna imagen (ver abajo).
2. **Lo silencioso salta la cola.** Sin orden prefijado, una excepción: lo que **falla sin decirlo** se mira
   antes que cualquier hito. Hoy son dos: `L20` (el compilador escribe el `.mod` tras dar error: un programa
   ejecuta sin su bucle) y los diez «rompe-el-modelo» de `#473` (la puerta de arranque que deja pasar
   `LIST_DIR`/`SAVE` sin FS, el `RUN` copiado cuatro veces…). Es el criterio de siempre —*errores sí, silenciosos
   no*— convertido en regla de cola.
3. **`H2` es la red de `H1`.** El módulo raíz regenera `Core.mod`, que va **embebido en las cinco imágenes**; el
   riesgo de `H1` es justo un `Core` rancio en algún sitio que pase en silencio — y `#500` (una dependencia más
   antigua que el módulo principal NO vale) es la reja que lo cazaría. No es «H2 es pequeño»: es que sin `H2`
   hecho, `H1` se prueba sin red. Sugiero `H2` antes de tocar `H1`.
4. **`H6` se demuestra en la C5, y nace pensando en `H12`.** La tesis de `H6` es «cualquier micro pequeño puede
   llevar una pantalla si su dueño quiere»; la prueba de la tesis no es el P4 (que ya lleva LVGL en la imagen),
   es la **C5**: una placa pequeña cuya pantalla llega **como pack**. Propongo que `H11` entre sin pantalla por el
   camino de `P1`, y que su pantalla sea el primer pack de `H6` (si hace falta antes, por el camino de `P2`, es su
   decisión; pero el pack tiene que llegar a probarse ahí). Y como `H12` va al final: la API de `H6`
   (`publica()`/`busca()`, la arena por código nativo) se diseña **segura con dos núcleos desde el primer día**
   — un candado hoy es barato; ponerlo en `H12` sobre un pack ya en uso, no.
5. **Un repaso a mitad de V7** — ✅ aceptado, y va junto al cierre casi completo del punto 1. «Nada a V8» tiene
   un riesgo simétrico: que V7 no cierre nunca. En V6 el repaso del 7-sep (*«15 pendientes → 2»*) fue lo que la
   hizo cerrable. Cuando se cierren **seis hitos**: un día de triaje de este índice con la pregunta de siempre
   —«¿está roto?»— y cancelar como resultado válido. Y el rótulo de `FICHAS` (cuántas quedan) **se cuenta, no se
   escribe**: en V6 el desfase del 8-sep salió de escribirlo a mano.

📌 **Los puntos 2, 3 y 4, aceptados tal cual** (Eduardo, 13-sep: *«de acuerdo en todo salvo en el punto 1»*).


---

## H1 · El módulo raíz — lenguaje y compilador (era `L1`)

| asunto | dónde |
|---|---|
| **`L1` — el módulo raíz** (la unidad `System`): los tipos y las 226 intrínsecas en un fichero BP legible; las dos tablas de ids (`Builtin.java`/`builtins.c`) **generadas** de él, y `check_builtins.py` se tira (resto de `#476`). *«Un solo archivo, sin posibilidad de diferencias entre compilador y VM»* (Eduardo, 5-sep) | `FICHAS` `L1` (L1055) y la decisión (L6271); `#476` (L4940) |
| **Sobrecargar una intrínseca** — `Intrinsics.REGISTRY` indexa por nombre pelado y el emisor compone la clave con `fs.name`: por eso `sign`/`signF` no se unifican. `factorial` en f64 **no necesita builtin**: `gamma(n+1)` ya lo cubre (`GammaFact.bp`) | `FICHAS` L1086 |
| **Destructor `~Clase()` + `var owner` LOCAL** — diseño hecho (`V5_IDEAS` 9-ago); `var owner` como campo ya existe (`#449`/`#450`). Y la 5.ª capa que pidió `#491`: destructor **nativo** que el GC llama tras el barrido | `FICHAS` L6287 y `#491` (L299) |
| **`Map`**: captadores tipados para claves **y** valores + `put` sobrecargado para clave entera y cadena (decidido 23-ago; `Collections.bp` sigue con `Object`) | `FICHAS` L10435 |
| **`#396` — módulo `Time`** con `Time.Date` sobre un `long` de segundos (no un tipo del lenguaje) | `FICHAS` L11869 |
| **`#19` — array fijo LOCAL inline de verdad** (tarea `#288`; el UAF se cerró en V4): `LEA_LOCAL` existe y el frontend nunca lo emite | `FICHAS` L11887; `V4_REF_AUDIT.md` §307 |
| ~~Mudar **`Exception`** fuera de `Core`~~ — **ya no ahorra nada** (24-sep): desde H1 P3 el `import Core` lo pone el compilador en todo módulo | `ESTADO` 30-ago |
| ~~**`intrinsic` declarada sin registro no da error**~~ — **resuelto por H1 P1** (24-sep, comprobado): la palabra `intrinsic` ya no existe —el cuerpo es `inline`— y escribirla es un error de sintaxis | `FICHAS` L4403 |
| `#460` — cabo suelto: **quitar del compilador** `module interface`/`implements` (paso aparte, con su verificación); la **herencia entre módulos** aparcada hasta tener *«un control de versiones efectivo»* — el caso con el que medirla es `Core` | `FICHAS` `#460` (L8414) |
| 🐛 **`L20` — el compilador ESCRIBE el `.mod` aunque dé error**: `for … step -1` falla (exit 2) y aun así deja un `.mod` que ejecuta sin el bucle. Silencioso: va delante. Y la decisión de lenguaje que `D1` dejó sin zanjar: `step` negativo o variable (hoy sólo literal entero positivo) | `PENDIENTES` L20 |
| `L19` — el cast `Clase(x)` sólo baja desde `Object`: `Perro(a)` con `a: Animal` se toma por constructor | `PENDIENTES` L19 |
| ✅ ~~`#508` — `private var`/`property` perdido en el parser~~ — **cerrada el 14-sep** (`4a6ea0f4`); y la matriz de `#492` es ya `HerenciaCrossModuleTest` (50 casos) | `FICHAS` `#508` |
| `M4` — prohibir el prefijo `__` en identificadores de usuario (hoy el lexer lo acepta y comparte namespace con los sintéticos `__prop_get_X`…) · `N9` — una clase sintetizada (`Thread`/`Mutex`/`StringBuilder`) redeclarada a medias por el usuario gana sin diagnóstico · `M5` — comprobar que el debugger enseña las properties heredadas | `PENDIENTES` M4, N9, M5 |
| `N2` — documentar como invariante que `JavaMutex.{ownerTid, waiters}` sólo se toca bajo `vmLock` (minutos; el código ya lo cumple) | `PENDIENTES` N2 |

## H2 · Lo que un `.mod` exporta y cómo se resuelven las dependencias

> 📐 **Diseño hecho el 14-sep** (`V7_IDEAS.md` §H2), sobre un mapa verificado y una matriz medida: `#492` decidido **B**
> (exportar en la interfaz: campos, `final`, estáticos; ×2,25 y +74 B por campo era el precio de la alternativa); `#500`
> con `gen` + `ifaceCrc` y `MOD8`; `#493` con el respaldo en su espacio (y de paso vuelve el XIP de los packs, hoy
> inalcanzable). Salieron cuatro bugs del compilador (`#507`–`#510`, en H1). **Decidido**: la subida es ADITIVA (las
> VMs cargan `MOD7` y `MOD8`; la stdlib se rehace al final). **Tests hechos antes del formato** (14-sep): la cápsula V6
> en `compat.sh` (59/59), `make test-modhdr`, la matriz (50) y la ida y vuelta de la interfaz (28) en la suite del
> frontend (188/188). Esperan cuatro decisiones de Eduardo (`V7_IDEAS.md` §H2 punto 5).

| asunto | dónde |
|---|---|
| **`#492` — B, casi entera (14-sep)**: la interfaz publica campos no privados, `final`/`private`, estáticos (alias plano `__cls_s_<Cls>#m$firma`) y todas las sobrecargas del constructor (cadena efectiva); `#509` `#510` `#513` cerradas con ella. Queda el marcador de layout, que va con MOD8. De la revisión adversarial salieron `#514`–`#519` (preexistentes) | `FICHAS` `#492` |
| **`#500` — la norma de versiones de las dependencias**: el `.mod` de la app lleva la versión de cada dependencia y el cargador **rechaza** una más antigua, venga del pack, de `/app` o de `/lib`. *«`Core.bp` nos sigue dando guerra»* | `FICHAS` `#500` (L160) |
| **`#493` — la sombra de `/app`** (y de un pack): una copia de stdlib en `/app` o un pack de librería tapa a `/lib` sin ruido. Dos mitades: el aviso en `OUTPUT`, y la decisión con `#500`/`#496` (que el pack builder no empaquete stdlib, o que el resolutor no deje que un pack tape a `/lib`). La mitad del runner ya está (`T1`, `843e2d0a`) | `FICHAS` `#493` (L203) |
| **`#441` — el `.mdn` no recuerda su RECETA**: huella de los flags de compilación en la cabecera, comparada al cargar como ya se compara `arch`. Mismo salto de formato que el resto de `#502` (§H5): hacerlas juntas | `FICHAS` L11637 |

## H3 · Ficheros: el cursor y la clase `File`

| asunto | dónde |
|---|---|
| **`#491` — EL CURSOR**: abajo, descriptores en la fachada de FS (abrir / `seek` absoluto y relativo / leer / escribir / cerrar); arriba, **`File`** (bytes) + **`TextFile`** (UTF-8) como objeto en el heap con su buffer dentro, y el destructor nativo que cierra al recoger. Con él caen `read_stream` sólo en littlefs (`#473`), la escritura en streaming (`#475`) y las aperturas duplicadas (`FF_FS_LOCK 0`). Quedan por escribir antes de codificar: endianness de `write(n)`, modo de apertura, origen del `seek`, `readLine` al final, extender más allá del fin | `FICHAS` `#491` (L224) |
| **`/sd` como prefijo RESERVADO**: sin tarjeta, error en vez de caer en silencio a la flash interna (decidido 23-ago «entra en V6»; no se hizo). ⚠️ Cambio de comportamiento: a las notas de versión | `FICHAS` L10417 |
| **`SD_INFO`/`SD_MOUNT` al REPL común** — siguen sólo en `pico/repl_v1.c` (resto de V5/H6) | `FICHAS` L11903 |
| ~~**La SD del STM32**: el motor es común; falta la cintura SDMMC de la U5 y montar `/sd`~~ — **no aplica** (Eduardo, 24-sep: la DK2 no tiene ranura microSD, y la Nucleo tampoco; ninguna STM32 del parque la tiene) | `FICHAS`, fila de H3 |
| ~~SD «superfloppy» sin MBR: probar con una tarjeta real reformateada~~ — **cancelada en placa** (Eduardo, 24-sep: el firmware no formatea y Windows no la prepara sin escribir el disco en crudo; en PC ya está) | `FICHAS`, `H2-P5` |
| **`listDir` en la VM-C** — a medio nacer: id y código en miVM, sin declaración BP y sin C (resto de `#484`; `input()` se queda fuera por decisión del 26-ago, y `PENDIENTES` mete a `listDir` en esa misma decisión: corregirlo) | `FICHAS` L4236 |
| `#456` resto — **el tope de un path es de la VM-C, no del lenguaje**: `BPVM_FS_PATH_MAX` = 256 sólo en C; miVM abre lo que sea. Que el tope sea de la spec, lo compruebe miVM igual, y un sample de paridad lo fuerce | `FICHAS` L9476 |

## H4 · La caracterización de los periféricos de cada micro — la HAL BP

| asunto | dónde |
|---|---|
| **`#378` — que cada micro DIGA lo que tiene**: `GPIO_COUNT`, `ADC_CHANNELS`, `PWM_SLICES`… dejan de ser `#define` y pasan a ser una pregunta al micro, con la **unidad** explícita (el RP2350 dice 24 canales de PWM en `INFO` y 12 *slices* desde BP); `INFO` y lenguaje de la misma fuente; poder preguntar si un pin concreto existe. La madre de las dos siguientes | `FICHAS` L11853; `V5_IDEAS` (6-ago) |
| **`#470` — los CUATRO NÚMEROS → arrays por micro**: *«para cada micro unos arrays, `[0, 1, 3]`, que indican los números válidos de UART, I2C, SPI, ADC, PWM…; el número es la longitud del array»* (Eduardo, 9-sep). Absorbe la pieza 2 de `#480`: el **`Pulse` del C3 cuenta 0 para siempre** (único ESP sin PCNT) | `FICHAS` `#470` (L738); `#480` (L3492) |
| **`#479` — el mapa de pines y los analógicos**: el mapa por familia compilado en la imagen (no en el ENV); el estado del pin en el común; `Pin` con `INPUT/OUTPUT/OPEN_COLLECTOR/ANALOG` y `AnPin extends Pin`. Absorbe el **ADC del STM32** (anuncia 20 canales, 0 backend; resto de `#469`) y los restos de pines de `#473`. Toca `Adc.bp`/`Gpio.bp` embebidos → ABI y blobs | `FICHAS` `#479` (L845); `#469` (L3408) |
| **`#471` restos — `Machine`**: `manufacturer()`, PSRAM y RAM, y `MAX/MIN_CPU_MHZ` **por micro** (hoy los del RP2350 para las cinco familias). Y de la librería genérica: quitar `variant()` («B» si `gpioCount() >= 40`, que contesta «B» en un S3) y definir «este micro no tiene eso» | `FICHAS` L4529, L11893 |
| **`#487` — el breadcrumb mudo en 4 de 5 familias**: implementarlo en RP2350 y ESP32 o declarar que no está; y en cualquier caso la fachada distingue «slot NULL» de «nada que informar» | `FICHAS` `#487` (L684) |
| **NeoPixel de verdad en ESP32 y STM32** (RMT/`led_strip`; hoy sin backend lanzan excepción BP, `3d14e32c`) | `FICHAS` L3402 |
| **El nombre `pico` en la capa C**: `bpvm_pico.h`, `bpvm_pico_set_backend`, 29 `BUILTIN_PICO_*` — `#471` arregló sólo la capa de usuario. Huérfano que `#473` pide con ficha nueva | `FICHAS` L4815 |
| **El tamaño de flash lo dice la placa** (ESP32 y STM32): sobre grande + JEDEC en runtime, como el RP2350 desde `#292`; nadie lee `flashSizeBytes` del ENV | `FICHAS` L11855 |
| El botón BOOT (GPIO9) del C6 como entrada — un «quizá» de `P2` que cayó en silencio | `FICHAS` L8186 |

## H5 · AOT y código nativo

| asunto | dónde |
|---|---|
| **`#502` resto**: el destino **`riscv32-esp-c`** (`rv32imc`, `ilp32`) en `NpackReloc.DESTINOS`, elegido por el `floatAbi` del `INFO`; y la **float-ABI en la cabecera del `.mdn`** (sección `native` del `.mod`). El cargador ya rechaza (12-sep); el IDE sigue compilando con la ABI del P4 | `FICHAS` `#502` (L3868) |
| **`N1` restos**: leer un campo de **tupla** desde native; **`try`/`catch`** dentro de una native (el `eh_stack` no sabe de un manejador nativo); y **la paridad dual-VM no cubre el camino AOT** — sólo `make test-aotnew` en host (seis fallos de `N1.4` pasaron por 109+39 pruebas) | `FICHAS` L11509, L11223 |
| **`#379` resto — una `native` larga no atiende el Stop**: `kill_requested` sólo se mira entre instrucciones del intérprete. Diseño en `V7_IDEAS` §«¿Podemos parar un código AOT?» | `FICHAS` L1287; `V7_IDEAS.md` |
| **`#426` restos** — lo que la ficha pedía (que `double` cruce a una `native`) **se hizo en V6** por helpers y la viñeta de §Aplazadas no lleva su ✅. Quedan: la conversión `double(x)` escrita DENTRO de una native sigue rechazada; cronometrar `double` native contra interpretado (≈3× estimado, nunca medido); decirlo en el manual; y la instrucción nativa donde haya FPU doble (hoy ninguna placa del sobre) | `FICHAS` L11906, L10896 |
| **`#468` — la stdlib en flash (XIP) en vez de en RAM** (la C6): decidir si se prueba el XIP del blob embebido salvo que `/lib` la sustituya; la tabla de símbolos por referencia; *«si LVGL sube a un pack, ese pack lleva `Gui.mod`»* | `FICHAS` `#468` (L932) |
| **Los cabos sueltos del AOT** — en qué línea falló el nativo (`bpvm_aot_fault_t` viaja sin origen), **parar/expropiar un thunk** (hoy un bucle infinito nativo sólo se recupera desenchufando), y el debugger que se pierde dentro. Propuesta de Eduardo (24-ago): **una marca (safepoint) que sirve para las tres cosas**, y el primer paso es MEDIR su coste en los saltos hacia atrás con `samples/NatV7.bp` (1 ms en la Pico). Sin fecha: *«todo eso para el futuro»* | `V7_IDEAS.md` §2 |
| La cola de la **fusión `.mod`/`.mdn`**: el `.mod` multifamilia (hoy N=1: el IDE funde sólo el blob de la placa conectada), la poda como UNA función (Run y packs), `putIfChanged` comparando lo podado | `FICHAS` L11519; `V6_IDEAS.md` §.mdn |
| **El `INFO` no publica la ABI de helpers** (`MDN_ABI_VERSION`): un `.mdn` de ABI vieja se rechaza en el log de la placa y el programa cae a interpretado sin ruido — así se tapó `#504` tres semanas | `FICHAS` L11129 |

## H6 · LVGL en un pack — y el código nativo empotrable (hito propio por decisión de Eduardo, 13-sep)

> *«El encapsulado de LVGL en un pack tiene que tener su propio hito.»* Y el porqué, del mismo día: *«Meter LVGL en un pack es estratégico, ya que hoy en día cualquier micro puede llevar una pequeña pantalla, así que le corresponde al usuario si quiere una pantalla o no, y no a nosotros»*. Es el primer cliente de verdad del
> diseño del código nativo empotrable (el encargo del 21-ago), así que las dos cosas van juntas: la disciplina
> alrededor de `publica()`/`busca()` se construye con LVGL delante.
>
> **La forma del hito, en dos mitades de riesgo distinto** (Eduardo, 13-sep): *«La migración de LVGL es un hito, y
> si lo hacemos sobre C5 es inofensivo. Ahora, quitar LVGL de las imágenes que la lleven es otro tema, más
> delicado. Por eso una prueba completa antes de quitar nada nos dará un punto para restaurar en caso de
> problemas.»*
>
> 1. **`H6.a` — LVGL como pack, en la C5.** Inofensivo: la C5 no lleva LVGL en la imagen, así que el pack sólo puede
>    añadir. Aquí se construye y se prueba todo: el contrato bidireccional, el driver del panel en el pack,
>    `Gui.mod` dentro, la arena por código nativo, la reja de `publica()`. Las tres imágenes con LVGL (C6, P4, DK2)
>    **no se tocan**.
> 2. **`H6.b` — quitar LVGL de las imágenes que la llevan** (C6, P4, DK2). Delicado, y por eso con precondición:
>    **una prueba completa antes de quitar nada** —la ronda de `F1`: las nueve placas con lo construido, el ZIP,
>    las guardas— cuyo resultado se sella como **punto de restauración** (commit + informe de tanda + los binarios
>    conocidos-buenos de cada placa en `dist/firmware/`, como manda la memoria «redes de seguridad y recuperabilidad»). Si al
>    quitarlo algo se rompe y no se arregla, se vuelve a ese punto y `H6.b` se cancela sin que `H6.a` se pierda.
>    Encaja con el cierre casi completo de mitad de V7 (§🤝 punto 1): puede ser el mismo día.

| asunto | dónde |
|---|---|
| **LVGL a un pack — el primer cliente**: contrato bidireccional y con estado (`flush_cb`/`read_cb`/eventos), qué pasa con `src/gui.c`, el pack lleva `Gui.mod` (`#468`), y el driver del pack pone la pantalla del usuario (`P2.3`); se mira junto a `#434`. *«Merece un trabajo separado, con una evaluación previa»* (26-ago) | `V7_IDEAS.md` §1; `FICHAS` L8243 |
| **El código nativo empotrable — el diseño** (encargo del 21-ago: *«código independiente que ya sabemos recolocar; lo que hay que mejorar es cómo publica su API y cómo interactúa con el resto»*), en cuatro puntos: (1) **reja en `publica()`** — leer marca y versión de la tabla en vez de aceptar un `const void*` (una tarde, con test); (2) dependencias entre códigos nativos con **`busca()` perezoso** en el primer uso (propuesta, sin decidir); (3) si las **cuatro ranuras** (`BPVM_PACK_MAX 4`) valen cuando crezca el catálogo — medir antes; (4) **la arena de RAM por código nativo**, no por pack: hoy es singular y se llama `SQLite`, y los `malloc` de la BIOS son chivatos | `V7_IDEAS.md` §1; `V6_IDEAS.md` §arena |

## H7 · Memoria, arranque y arquitectura

> ✅ **H7 CERRADO el 1-oct** —lo último que quedaba, el resto de `U6` que pedía la S3: remedirla con la imagen de hoy y añadir su caso a `test_mem` (`b0bc3164`: la S3 N16R8 con la imagen de `ae3ca338`, 7677 KB de la PSRAM para la VM; el caso sin PSRAM, del 30-ago, se queda). Eduardo lo había dejado abierto hasta que la S3 estuviera en la mesa, y la conectó esa tarde. Lo que salió de H7 sigue en su sitio: `#615` (SUELTA: la Pico al común) y el modelo de los núcleos (H12)—.

| asunto | dónde |
|---|---|
| ✅ 29-sep — **el mapa verificado de lo que quedaba** y lo que salió de él: `#603` (soltar un owner con `--smp`), `#604` y `#605` (los hilos y las pilas), `#606` (dos latentes del XIP), `#593` (la reserva en un método), `#608` (una excepción sin atrapar acaba su hilo) y, de la prueba de estrés del heap, `#610`, `#611` y `#612` — todas cerradas; ~~`#607`~~ (30-sep, `85de188f`: el setter de Forms con el opcode `INVOKE_SLOT`, y el hilo que muere sin atrapar lo dice en la salida) y ~~`#613`~~ (30-sep, `d6d72384`: B1 por `sleep(0)` en miVM, del «encontrado de paso») | `V7_IDEAS` §H7, el mapa del 29-sep; `FICHAS` |
| ✅ 1-oct, con la S3 (`b0bc3164`) — **`U6` restos** (30-sep, Eduardo: la C3 —coserla, su objetivo y el +10 %— a **V8**; `vmheap=N`, el bloque entero —hecho, `ee89f444`—; el nativo sobre PSRAM, a estudiar sin versión): ~~**coser las dos regiones del C3**~~ (136 → ~250 KB útiles; las cuatro comprobaciones del hueco y `coser=0|1` en el ENV), ~~el **margen de la Pico como MEDIDA** (64 KB a mano desde julio) y el del STM32~~ (30-sep, `a2bdff6e`: medidos; la propuesta, en `V7_IDEAS` §H7), ~~**remedir el S3 y añadir su caso a `test_mem`**~~ (1-oct, `b0bc3164`: la S3 N16R8 con la imagen de `ae3ca338`, 7677 KB de la PSRAM para la VM; el caso sin PSRAM, del 30-ago, se queda), ~~la clave `vmheap=N` que no entró~~ (30-sep, `ee89f444`), ~~el +10 % del `AllocBench` del C3~~ (a V8, con la C3) y ~~medir un nativo sobre PSRAM~~ (a estudiar sin versión) | `V7_IDEAS` §H7, el mapa del 29-sep (fila «Los restos de `U6`») |
| ~~**`U6.F1` — con PSRAM, qué buffers NO-BP agrandar**~~ con la SRAM ociosa (421 KB en la Metro): estudio aparcado (30-sep, Eduardo: a estudiar en un futuro, sin versión) — *«me sigue chirriando tener RAM ociosa»* | `FICHAS`: buscar `U6.F1` |
| ~~**`U1.4` — UNA cintura de flash por familia**~~ (30-sep, Eduardo: la pequeña, un `pico_flash.c` — hecha, `47fe6fef`, y probada en la Pico) para los cuatro consumidores (packs, env, FS, log); hoy ~12 cableados de las mismas dos operaciones | `V7_IDEAS` §H7, el mapa del 29-sep (fila «`U1.4`») |
| ✅ 1-oct (`e9ca7c30`: la fase 4 de Boot 2, el arranque común; las capas, en `H9_KERNEL_CAPAS.md`; sin peldaño de packs: la zona es una capa AL LADO, Eduardo, 30-sep) — ~~**Boot 1 / Boot 2**: dar nombre a la frontera familia/común, **peldaño de packs** en `bpvm_boot.h` (hoy 4 peldaños y cada familia monta los packs por su cuenta), y **declarar las capas del SO común** (los packs pertenecen al SO: son backend del FS)~~ | `V7_IDEAS` §H7, el mapa del 29-sep (fila «Boot 1 / Boot 2…») |
| ✅ 30-sep y 1-oct (las cuatro fases de Boot 2: `da19145e`, `ef55fc9d`, `72c3ab6f` y `e9ca7c30`) — ~~**Unificar el montaje de packs**: paso común + `mapear()` por familia (el S3 sigue sin `pack_s3.c`)~~ | `V7_IDEAS` §H7, el mapa del 29-sep (la misma fila) |
| ✅ 30-sep (`d5b828b1`, `a91b8dad`: el FS en los 16 MB de arriba y `bpdata` para los packs, en la Waveshare y en la C5; un FS de más de 16 MB, más adelante) — ~~**[P4] los 32 MB de flash y el XIP de los packs**~~: el caché mapea 24 bits; dos caminos (invertir el orden común, o segunda partición sólo en el P4), sin elegir; hoy 16 MB y packs en 2800 KB. **Paso 0 hecho** (30-sep, `cc503a54`: la dirección física en el log de la zona y sin el falso «reflashear bootloader»; visto en la Waveshare). Y Eduardo (30-sep): el techo es del código que se ejecuta, el FS no debería tenerlo, y cada placa dice su tamaño | `V7_IDEAS` §H7, el mapa del 29-sep (fila «Los 32 MB del P4») y «Los 32 MB del P4 — dónde está de verdad cada techo» |
| ✅ 30-sep, CERRADA por Eduardo (medido el 30-sep: quitar la base `cb` del `CALL` da del 1,0 al 1,7 % en el peor caso, `fib(32)`, y la caché en línea nada; no compensa subir el formato con toda su cascada) — ~~**`[ISA]` `CALL_REL`**~~ — el CALL local PC-relativo, que retira la base `cb` «transitoria» del XIP (desde `#307`, jul) | `V7_IDEAS` §H7, el mapa del 29-sep (fila «`CALL_REL`») |
| → **`#615`, SUELTA** (Eduardo, 1-oct: aparcada sin fecha ni previsión; se verá si en V7 o en V8) — ~~**Migrar la Pico a `src/platform_freertos.c`**~~ — parada por un 32 % de `PrintBench` sin explicar; ~~**paso 0: remedir**~~ (30-sep, `d8db24b7`: +10,8 %, fuera del ±3 %; la Pico se queda con su fichero; el porqué, sin encontrar; lo que queda, fusionar por mitades, puede esperar) (la medida es anterior a `#485`). Es el paso 0 de fundir los tres backends de FreeRTOS (`#473` R1+R16) | `V7_IDEAS` §H7, el mapa del 29-sep (fila «La Pico sobre `platform_freertos.c`»); `docs/FUSION_BACKENDS_FREERTOS.md` |
| ✅ 29-sep (`98c640e9`) — ~~**Muro de contención entre el heap y las pilas**~~ (idea de V4): faltaba donde nadie miraba, en las PILAS —`#605`, el desbordamiento de pila es una excepción atrapable, en las dos VMs—; con `#604`, los hilos de la VM-C reutilizan hueco y pila y respetan `Thread(stackSize)` | `FICHAS` `#604`, `#605` |
| ✅ 28-sep (`ac0f3d71`, `#594`: el módulo `Heap`; y compactar, `#595`, `6b265162`) — ~~**Diagnóstico del heap DESDE BP**~~: `heapFrag`/`heapMap` y los contadores del GC como **números**, en las dos VMs (hoy sólo miVM y como cadena); intrínsecas nuevas → van con el módulo raíz | `FICHAS` `#594`, `#595`; `V5_IDEAS` (5-ago) |
| ~~`#356`~~ ✅ 27-sep, cerrada por superada (Eduardo: el único descarte no pierde bytes y se cuenta desde `#355`) — el descarte mudo del alocador (resto rebajado de `#357`): el cuerpo de la ficha no está en el repo; reconstruir qué se descarta o cerrar por no reproducible | `FICHAS` `#356` |
| ~~`#494` resto — el guardián del heap del GC (`HEAP INCONSISTENTE` / `DESCARRILADO`): ¿fatal o aviso?~~ ✅ 27-sep (`7a1a9374`): ni lo uno ni lo otro, una excepción (Eduardo); su punto ciego, `#593` | `FICHAS` `#494`, `#593` |
| ~~`scheduler_smp.c` llama al `poll_cb` sin la guarda `vm->io == NULL` (latente: SMP es opt-in)~~ ✅ 27-sep (`6f249614`: la guarda y `test-kill` en la red); ~~**decidir si `--smp=2` va por defecto** en el host~~ (×1,7–1,9 medido, 0/40 cuelgues tras `#474`): **de momento no** (Eduardo, 28-sep: se decide con las pruebas del final de V7, y hay micros de 1 y de 2 núcleos; el corpus con dos workers, medido ese día, sacó ~~`#599`~~ —cerrada, `c539c27c`: desde entonces corre en la red, `check-smp`— y `#600`, la línea partida con dos workers); y ~~el **modelo final del SMP**~~ → **H12**, con el modelo de Eduardo del 1-oct (`V7_IDEAS` §H12): VMs ociosas «a piñón» o parqueadas (medir la latencia de despertar antes) y los workers como cola de hilos de SO en vez de N fijos — rumbo de Eduardo del 9-sep | `ESTADO` 5-sep; `FICHAS` `#599`, `#600`; `V6_IDEAS.md` §SMP |

## H8 · IDE y protocolo wire

> ✅ **H8 CERRADO el 1-oct** (Eduardo: *«Si. cierra H8»*) —lo último, ese día: el paso 4 de `#495` (`847873f5`, `be628ee3`; visto en las cuatro placas), `#617` (`b33794df`) y `#538`, visto por Eduardo en el IDE con una placa; `M5`, a V8—. El detalle, en `FICHAS` (la fila de H8) y `V7_IDEAS` §H8. Esta tabla estaba atrasada (daba por
> abiertos `#495`, los restos de `E1`, el `INFO` y el guardián del fat-jar): puesta al día al cerrar.

| asunto | dónde |
|---|---|
| ✅ 1-oct, cerrada (`1c19eabb`, `ae3ca338`, `e97b4c38`, `847873f5`, `be628ee3`) — ~~**`#495` — `GET`/`LS` durante un RUN**~~ → el cable troceado: la subida con ventana y el segundo intento del IDE (pasos 1–3) y el `GET` y el `LIST` troceados y mezclables (paso 4); durante un RUN, `STAT` y los dos troceados | `FICHAS` `#495`; `V7_IDEAS` §H8 |
| ✅ 1-oct, cerrada (Eduardo, 28-sep: los cinco se quedan como están, probados en `check-sim` desde `#537`) — ~~**`#452` — repasar los 29 verbos del wire**~~ con «¿a cuál de los tres consumidores sirve?»: **`FORMAT` falta hoy desde el IDE**, `RENAME`, `RMDIR`, `PING`, `DF` duplicado. La condición («cuando exista el script») se cumplió con `T1` | `FICHAS` `#452` (L1102) |
| ✅ 28-sep y 1-oct — ~~**`E1` restos**~~: el `EXITED` de un `RESET` (28-sep, `10910242`); la vista de packs en el árbol (Eduardo: no, el árbol es el FS); el **rojo** del árbol (1-oct, `d7251bd5`: el `LIST` examina la cabecera de cada `.mod`, `.mdn` y `.pack`) | `FICHAS` (la fila de H8); `V7_IDEAS` §H8 `E1` |
| ✅ 28-sep (`801e840e`) y 1-oct (`#602`, `a6f7ff7f`) — ~~**El `INFO` de las ESP32 y STM32**~~: `floatAbi` y la zona de packs, del común; y el bloque de RAM de los motores (`packRam*`), el mismo en las cuatro familias | `FICHAS` `#602` |
| ✅ 28-sep (`3c9d197c`) — ~~El guardián del fat-jar (`#429`) no vigila la **miVM embebida**~~: ya la vigila | `FICHAS` (la fila de H8) |
| ✅ 1-oct, visto por Eduardo (`#538`) — ~~Confirmar desde el IDE que `Run on Device` sube sólo lo que falta (con `/lib` vacío)~~: `JsonDemo` en la P4 sin `/lib`: `Json` y `Core`, del `Stdlib.pack`, idénticos, no se suben; exit 0 | `FICHAS` `#538` |
| ✅ 1-oct (`b33794df`) — `#617`, salida del paso 4: la VM-Java sube por trozos (con la regla de las placas) y vuelve a escuchar cuando el cliente se va | `FICHAS` `#617` |

## H9 · Pruebas y medida

| asunto | dónde |
|---|---|
| **`T1` restos** del runner: «dónde aplica» por capacidad (`necesita: ["sd"]`); **el sello miente** (`serverBuild` es la hora de `main.c`: `imageCrc`/git-rev en `HELLO_REPLY`); el descubrimiento necesita > 3 s tras un reset en P4/DK2; crecer el corpus «cuando haga falta». Y el aviso del 7-sep: el runner compara host contra placa, **no contra lo esperado** (✅ 2-oct: una línea `FALLO` de la placa da el veredicto FALLO aunque salga igual en el PC, como en la red) | `FICHAS` L5934 |
| **Lo que `V7_IDEAS` pedía del arnés y `T1` no hizo**: `memoria: APRETADA` en el manifiesto (la matriz no es verde sin la celda de la placa que más aprieta: Pico 2 / C6); el criterio «no determinista» por aserciones (para `synclisttest` y similares, hoy en ningún corpus); **recuperar la placa colgada** con `RESET` por el wire (o DTR/RTS donde lo haya) y pedir el **log post-mortem** después — hoy `KILL` y abortar la placa; y comprobar que la C6 y el STM32 se resetean **sin manos** | `V7_IDEAS.md` §4 |
| `L15` — la tabla de resets del log post-mortem está rancia (el STM32 sí se comprobó en H13); lo no medido es el **botón físico** de reset en Metro (`RUN`) y STM32 (`NRST`) | `PENDIENTES` L15 |
| ✅ (2-oct: la paridad exige el mismo código en las dos VMs; censo previo, 0 de 125) ~~**`compat.sh` no vigila el código de salida**~~ (resto de `#481`): los `1` de `ThrowSinAtrapar` están medidos a mano; y el camino «el módulo no exporta `RuntimeError`» sin comparar | `FICHAS` L3679 |
| ✅ (2-oct: P4 23,4 ms, del KILL al `EXITED`; `FICHAS` `#485`) ~~`#485` resto~~ — **medir** en el P4 el KILL mientras calcula (`io` a la prioridad de la VM); el único número (17 ms) es anterior al arreglo | `FICHAS` L4290 |
| ✅ (U3.25 en la C6 el 2-oct; U1.2 y P1.C3, en placa el 24/25-sep, anotados el 2-oct) ~~`U3.25`~~ — `MKDIR`/`RMDIR` reales **en placa** (el «verificado» del 30-ago fue con el stub) · `U1.2` — que el post-mortem de la Pico **sobrevive al reinicio**, anotado · `P1.C3` — `trytest.bp` y `JsonDemo` en el C3 | `FICHAS` L10209, L8315, L7830 |
| ✅ (`#537`, 23-sep) ~~**`make test-listtrunc` con el CONTROL en rojo**~~ (13-sep, comprobado): 16 carpetas → 15 entradas, `omitted=1` por debajo del tope | `FICHAS` L9490 |
| ➡️ (Eduardo, 2-oct: *«el rendimiento lo podemos medir en H12 cuando trabajemos con 2 núcleos. La prueba de resistencia de días lo dejamos para V8, cuando lo trabajemos un poco más»*: la batería, a H12; la resistencia, a V8) ~~**Batería de rendimiento HW+SW**~~ — medir el REPARTO (VM / HAL / hardware) por placa; ya hay datos sueltos (Nucleo vs Discovery). Es la misma cosa que **el eje 4 del censo funcional** (memoria y tiempos por sistema, *«nada medido a este nivel»*), el único de los cuatro ejes del 17-ago que V6 no hizo: una entrada, no dos · **Prueba de resistencia larga** — días de carga variada con marca periódica en el log-anillo (RUN largo Y muchos RUN) | `FICHAS` L11896, L11898; `CENSO_SISTEMAS_V6.md` §eje 4 |
| **El simulador con disfraces** — `bpvm-sim --familia=` (bloque de borrado, vista de packs, AOT); parte de la motivación cayó con la unificación | `FICHAS` L10599 |
| **Inyección de eventos en el device** (resto de `#475`/`C1`): la captura en placa sólo ve la pantalla inicial; el host tiene guion, la placa sólo el dedo · `Window.load()` **hornea el tamaño del panel** en el `.win` (no portable entre pantallas) | `FICHAS` L5506, L5179 |
| Limpieza: los 11 `.mod` de stdlib **hermanos** en `bpgenvm-c/samples/` que nadie refresca (un run manual carga stdlib rancia) · `samples/out` con la stdlib MOD6 de V5 (resto de `#467`, sólo `rm`) · los `.mod` de prueba que quedaron en las placas (`/app/P.mod`, `adc.mod`…; con OK placa a placa) | `ESTADO` 25-ago; `FICHAS` L3336 |

## H10 · Placas y drivers

| asunto | dónde |
|---|---|
| ~~**`#490` — bajo consumo, el ESTUDIO**~~ → **V8** (Eduardo, 24-sep: la V8 es la WiFi, el Bluetooth y el bajo consumo; la ficha está en el bloque de V8 de `FICHAS`). Lo que decía: censo hecho (`BAJO_CONSUMO_CENSO.md`: ninguna placa duerme); faltan la línea base por placa, los mA (sin amperímetro no hay números), la tabla reloj/modo por micro y por qué el *tickless* no sirve. Punto de partida decidido: despertador por temporizador + escalera progresiva + dos regímenes. Hallazgos anexos dentro de la ficha: STM32 sin tarea ociosa, `sleepUs()` en espera activa, `MIN_CPU_MHZ` declarado y no implementado, 19 periféricos siempre encendidos en la DK2 | `FICHAS` `#490` (L414) |
| **`Rtc` por software en Pico y ESP**: con autorun nadie calibra y `epochSec()` devuelve segundos-desde-boot como si fuera la hora, sin forma de distinguirlo desde BP | `FICHAS` L671 |
| ✅ (24-sep, en la Pico 2: «reinicio pedido» tras un `RESET` por el wire) ~~`#466` resto~~ — la Pico dice `causa del reset = WATCHDOG` tras un `RESET` pedido por el wire (una línea en `main.c:1596`; `resetCause` ya distingue) | `FICHAS` L3194 |
| **STM32**: la única configuración se llama `Debug` aunque compile a `-Os` y sea la de publicar (renombrar o no; tocar `PUBLICAR.md` y el script) · **el TOTAL del FS baila** entre arranques (516096 ↔ 614400, 26-ago) y el boot no dice de dónde sale la tabla de particiones | `FICHAS` L8410, L10270 |
| ✅ (2-oct: los defaults lo reproducen, 0 diferencias; fuera del índice y con su `.gitignore`) ~~El `sdkconfig` generado del C3 está **trackeado**~~ (S3, C6 y P4 lo ignoran): `.gitignore` + `git rm --cached` | `FICHAS` L7969 |
| Cosmético: el log del bootloader sale por el USB nativo **antes del `HELLO`** (C6/C3); el IDE lo salta | `FICHAS` L8241 |
| El ENV de la Pico 2 **cambió solo** al flashear (28-ago; FS 1512→1024 KB): sin causa ni reproducción — si vuelve, capturar el log del arranque | `ESTADO` 28-ago |
| **ESP32-S31** (RISC-V, sucesora de la S3) cuando haya placa, por el camino de `P1.C3` | `FICHAS` L7641 |
| **GUI**: `#477` resto — los tres agujeros del oráculo `__guiDumpTree` (el chart no vuelca datos, ciego a la rotación en la DK2, color/fuente sólo de render) · `G2` resto — `owner` en las properties de la ventana y un `~Component` que libere el nodo (necesita el destructor); `toJson()` no emite min/max, options, colores, ni Chart/Table/ImageView | `FICHAS` L5036, L5103 |
| **GUI — una cola de tareas hacia el hilo que bombea** (el `invokeLater` de Swing) y la vía intermedia de Eduardo: una clase base **`Command`** en la stdlib (`ejecutar()`/`habilitado`, encolable). La reentrada de LVGL que lo motivaba ya la cerró `G1`; queda la utilidad de «ejecutar esto en aquel hilo» — y si entra, entera de una vez (una base nueva corre las ranuras de la vtable) | `V7_IDEAS.md` §3 |
| **Los 10 «rompe-el-modelo» vivos de `#473`** que el triaje pedía con ficha nueva y no la tienen: `RUN` ×4, sink de `OUTPUT` ×5, puerta de arranque ×3 con agujeros, fachada FS↔littlefs, clasificación de fallo en 1/3, `board_mgr_*` ×3, `lv_conf.h` duplicado, `setRotation` en la DK2, el camino de error de `scheduler_smp.c` que no une workers, y los lotes D (FS, 15) y E (GUI, 10) de `473_TRIAJE_COLA.md` | `FICHAS` `#473` (L4597); `docs/473_TRIAJE_COLA.md` §3 |
| El DMA de `Uart` donde la familia lo tenga (V6 dejó el anillo por IRQ) | `FICHAS` L4798 |

## H11 · La ESP32-C5 — Waveshare ESP32-C5-Touch-LCD-2.8 (comprada el 5-sep; la placa nueva de V7)

> ✅ **H11 CERRADO el 1-oct** (Eduardo) —las dos etapas, hechas: la imagen sin LVGL (21-sep) y LVGL en un pack (`H6.a`, 22-sep; H6 cerrado el 26-sep); el AOT sin FPU (H5, 25-sep); la TF (H3); los 32 MB, con el FS arriba (30-sep, `a91b8dad`); y la tanda de V7 entera, 29 de 29 (1-oct). Lo que la fila daba por pendiente, medir el bloque de la VM sin PSRAM, no aplica: su imagen lleva siempre PSRAM (`CONFIG_SPIRAM`) y esa rama ni se compila; su RTC y el bajo consumo van a V8 con `#490`, y la memoria apretada, a H9—.
>
> Sustituye al P4X en el plan. Entra por el camino que V6 dejó hecho para el C3 y el C6 (`P1`/`P2`): la
> familia ESP32 se **activa**, no se construye — y lo que costó en V6 fue la placa, no la familia.
>
> **La placa ya está en la mesa (Eduardo, 21-sep), y el plan es en DOS ETAPAS** — idea suya, no para ahora:
> **1.ª la imagen SIN LVGL** (la C5 como una placa no gráfica más: familia activada, `boardName`, memoria con
> sus 8 MB de PSRAM, serie, la TF; por `tanda.py` como el C3/C6); **2.ª, sobre esa imagen, montar LVGL en un
> PACK e ir probando** — o sea que la C5 es el **primer cliente real de H6** (LVGL en un pack): la pantalla y el
> táctil no entran en la imagen, llegan por el pack. Así H11 y H6 se prueban una contra otra, en la placa
> pensada para ello (pantalla pequeña + PSRAM), sin tocar las imágenes que ya funcionan.

**La placa, según su ficha** (Amazon, 13-sep; *«5-point capacitive touch, no batt»*): módulo **ESP32-C5-WROOM-1**,
RISC-V a 240 MHz, **384 KB de SRAM, 320 KB de ROM, 8 MB de PSRAM, 32 MB de flash**; Wi-Fi **2,4 y 5 GHz**, BT 5 LE
y **802.15.4** (Zigbee/Thread); USB-C con USB *full-speed*; pantalla IPS **2,8" de 240×320, 262K colores, táctil
capacitiva de 5 puntos por I2C con interrupción**; **ranura TF**; IMU QMI8658 (6 ejes), sensor RTC, SHTC3
(temperatura y humedad); códec de audio ES8311 + amplificador NS4150B, micrófono y altavoz; gestión de batería
con conector MX1.25 (sin batería); *«flexible clock and multiple power modes to realize low power consumption»*.

| asunto | dónde |
|---|---|
| **La placa**: carpeta `esp32c5/` clonando `esp32c6/` — `chip_cfg.h`, `<chip>_board_id.c`, `partitions.csv`, `sdkconfig.defaults` (comprobar que el IDF instalado, `C:\esp6.0.1`, ya trae el target `esp32c5`), y el enumerador de memoria para `bpvm_mem_plan` **con los 8 MB de PSRAM** (el reparto del S3/P4 —heap de la VM y FS en PSRAM—, no el del C6) | `FICHAS` `P1` (V6); `ESTADO` 31-ago/3-sep |
| **El AOT**: el destino se decide por la float-ABI que publique el `INFO` — si el C5 lleva FPU de simple precisión va como el P4 (`ilp32f`); si no, como el C3/C6 (`ilp32`, que es justo el resto de `#502`, H5). Comprobar en la hoja del chip, no suponer | `#502` |
| **La pantalla de 2,8" 240×320**: el panel por el ENV (`display=…`) como en `P2`; el controlador exacto lo dice el esquema de Waveshare (si es un ST7789 por SPI, el driver del C6 vale con otra geometría; si es otro, es un driver más, no un modelo nuevo). **El táctil de 5 puntos por I2C es nuevo en una placa pequeña**: hasta hoy sólo el P4 (GT911) y la DK2 tienen táctil — el modelo de eventos de la GUI se prueba por primera vez en un micro con 384 KB | `FICHAS` `P2` (V6); memoria `gui-screen-es-el-panel` |
| **Lo que trae y ya tiene sitio en otros hitos**: la **ranura TF** (la SD por SPI del motor común → H3, y una placa más para `SdDemo`); el **sensor RTC** y los **modos de bajo consumo** que anuncia (→ `#490`, que desde el 24-sep es de **V8**; antes H10: candidata al estudio junto al STM32, que hoy es la única con RTC de hardware); los **32 MB de flash** (¿mapea XIP más allá de 16 MB, o tiene el mismo techo que el P4? → H7); Wi-Fi 6 dual y 802.15.4 (→ la red en placa, V8) | H3, H10, H7 |
| **El banco de pruebas**: `tanda.py` con la C5 como una columna más; con PSRAM y pantalla es la primera placa pequeña que puede correr los `Gui*Demo`, y candidata a **juez de memoria apretada** (lo que `V7_IDEAS` pedía del C6, H9) | H9 |
| Lo que se decide cuando esté en la mesa: `boardName`, qué expone (`getMicro()`/`getBoard()`, H4), y si su USB es el serial-JTAG nativo del chip (la trampa del COM3 compartido con C3/C6; un solo USB → la consola por UART0 como en el C3) | memorias `dos-esp-mismo-com`, `c3-un-solo-usb-sin-dtr` |
| ✅ **1.ª etapa hecha (21-sep, noche)** — lo que dijo la placa al llegar: `esptool` «ESP32-C5 (revision v1.2), 32MB»; USB **serial-JTAG nativo** (303A:1001, COM6): wire por el USB, consola a UART0, como C3/C6; **sin FPU** → `aotReceta RV32IMC_ILP32` (el destino del AOT es el del C3/C6: `#502`); `boardName=esp32c5`, 29 GPIO, 6 LEDC, 6 ADC, 384 KB; **zona de packs de 13,7 MB mapeada** (bpdata hasta los 16 MB; la otra mitad de la flash sin tocar); el bloque de la VM sin PSRAM con los números del C3 (128 KB) hasta medir con U6.4. **PSRAM: 8 MB en QUAD a 80 MHz (el único bus del C5), la VM en PSRAM con 7677 KB** por la rama exclusiva de `main.c`, `FileBench` completo, coste visible y esperado (`readLine` 69 → 91 ms). Placa virgen: `wire_serie.py COM6 part apply` | `ESTADO` 21-sep noche |

## H12 · La prueba de los dos núcleos — al FINAL de V7, y reversible (era `A4`)

> Eduardo, 13-sep: *«Al final de V7 podemos hacer una prueba a ver si se puede trabajar con 2 núcleos reales: si funciona se mantiene, pero si da problemas y no somos capaces de solucionarlos, lo echamos para atrás»*. `A4` estaba
> 🧊 fuera del plan desde el 9-sep (*«no es V6 y probablemente tampoco V7»*); hoy vuelve, con dos condiciones
> que son la forma del hito: **va al final** (cuando todo lo demás esté cerrado y probado en un núcleo) y
> **tiene criterio de salida escrito antes de empezar** — revertir es un resultado válido, no un fracaso.

| asunto | dónde |
|---|---|
| **El modelo, de Eduardo (1-oct)**: con un núcleo, dos hilos de SO —`io` y la VM—; con dos (la Pico, la Metro, las dos P4 y la S3), tres —`io` y dos de la VM—, que los núcleos van tomando sin anclar; los hilos BP, en una cola que recorren las VM; una VM sin nada en la cola cede. Lo que hay y lo que cambia —el REPL pasa a `io`, que vive siempre—, y lo que falta por medir (cómo cede una VM ociosa), en `V7_IDEAS` §H12. La S3, que también tiene dos núcleos, entra con tres hilos (Eduardo, 1-oct) | `V7_IDEAS` §H12 |
| **Lo que `A4` dejó hecho y medido**: el inventario del estado compartido; la carrera de `s_line_buf` arreglada (`64b6f2fb`); `CONFIG_FREERTOS_UNICORE=y` en S3 y P4 (`c080e1f0`); las tres carreras del SMP de host cerradas en `#474` (0/40 cuelgues) | `FICHAS` `A4` (L6915) |
| **Antes de activar el segundo núcleo**, en este orden: (0) la variable de condición de FreeRTOS con un semáforo CONTADOR, en las tres plataformas —con el binario de hoy, un aviso a todos con dos runs esperando despertaría a uno solo (R8 del estudio de la fusión; añadido el 1-oct, `V7_IDEAS` §H12)—; (1) `vm->kill_requested` como atómico con orden explícito (hoy `volatile int`); (2) en la Pico, el `printf` de consola comparte el USB con el wire y no toma `wire_v1_tx_lock`; (3) lo que el inventario marcó como compartido sin dueño | `FICHAS` `A4` |
| **La forma** (afinada por el modelo del 1-oct: sin anclar, los núcleos toman el siguiente hilo listo): `io` en un núcleo y `vm` en el otro (la arquitectura de `A1`: tres colas y nada más); después, si aguanta, **las dos VM sobre una cola de threads BP** (el camino SMP de host, `scheduler_smp.c`, rehecho sobre `io` — y entonces `comm_pico.c`/`comm_host.c` se borran, `A1.4`); el modelo «a piñón» vs parquear y la cola de hilos de SO (H7) se deciden midiendo la latencia de despertar | `FICHAS` `A1` (L6371), H7 |
| **La batería de rendimiento HW+SW** (venida de H9, Eduardo, 2-oct): se mide aquí, con dos núcleos —el REPARTO VM / HAL / hardware por placa, el eje 4 del censo funcional (`CENSO_SISTEMAS_V6.md`)— | `FICHAS` L11896 |
| **Dónde se prueba**: las placas con dos núcleos son la Pico 2 (dos Cortex-M33 / Hazard3), el S3 y el P4 (dual RISC-V); el C3, C6, C5 y los STM32 son de un núcleo y se quedan como están. El banco es `tanda.py` con la misma tanda de `F1` en las dos configuraciones, y `Bench`/`PrintBench`/`AllocBench` con cronómetro: **si no es medible, no es mejora** | H9; memoria `impresion-no-es-medida-cronometro` |
| **El criterio de salida**, literal: *«si funciona se mantiene, pero si da problemas y no somos capaces de solucionarlos, lo echamos para atrás»*. Lo que se conserva en cualquier caso: los atómicos y los candados que la prueba obligue a poner, que son correctos también con un núcleo | — |

## Fuera del plan de versiones (decidido; se lista para no perderlo)

- 🧊 **`#488` — el AOT no genera Xtensa** (*«de momento no»*): el S3 corre todo interpretado. Con él, `bios_s3.c` (sin consumidor) y los **packs nativos en S3/STM32**. → `FICHAS` `#488` (L4063)
- **La red en placa** (backend WiFi/Ethernet de `Net`): **V8**, rumbo del 10-ago. Y el 24-sep Eduardo fija la
  V8: *«la WiFi y el Bluetooth principalmente»*, y el bajo consumo (`#490`) con ellas. → `FICHAS`, el bloque de V8
- **Linux**: el IDE en Linux + la Raspberry Pi como placa — `[SIN VERSIÓN, V6+]`. → `FICHAS` L11901
- `Gui.setBrightness` / PWM del backlight — «no tiene a quién servir» (4-sep).
- **[P4] la segunda imagen para el silicio nuevo (P4X)** — **sin fecha** (Eduardo, 13-sep: *«lo pedí pero me enviaron la versión P4, así que de momento no lo tengo»*). Cuando haya placa: otra carpeta de build sin las dos líneas que clavan al silicio viejo; se mantienen las dos imágenes. → `FICHAS` L11696
- 📌 **EL PARQUE DE PLACAS, dicho por Eduardo (21-sep, tras la C5):** de la familia ESP32 faltan DOS y no habrá más por ahora: el **P4X** (*«en principio es recompilar la imagen ajustando los parámetros de IDF a la v3»* del silicio: otra carpeta de build sin `CONFIG_ESP32P4_SELECTS_REV_LESS_V3`) y la **ESP32-S31** (*«totalmente nueva y que sólo está en kit»*). **La familia Raspberry está terminada.** Lo que queda es **el STM32** — donde una placa nueva es hoy un proyecto CubeIDE, y por eso el coste por placa es el que hay que bajar (H4/H10)— **o alguna familia nueva**.
- «Faltan módulos» al ejecutar desde `run` de consola tras el IDE — en suspenso, sin reproducción (la asimetría del `basedir` sigue: `fs_facade.c:98`).
