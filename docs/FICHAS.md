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
> ### 📊 EL CENSO, al 9-sep-2026 — leído ficha a ficha, no por la marca
>
> **Lo que queda de V6 son 2 pendientes y 4 hitos** — y cada uno es UNA cosa; con las entradas
> agrupadas de antes la lista decía 9. *(De `#482` salió `#488` —el S3 es Xtensa y el
> C3 RISC-V, casos distintos— y `#488` salió acto seguido del plan de versiones: «de momento no».)* *(Eran 15 el 7-sep, cuenta de Eduardo. El
> 8-sep se cerraron `#472` y `#469`, `#468` se fue a V7 y se abrió `#481`; el 9-sep se cerraron
> `#474` y `#456`, `#470` se fue a V7 y `A4` salió del plan de versiones.)*
>
> ✅ **9-sep — TODO PENDIENTE TIENE NÚMERO, Y UNA ENTRADA ES UNA COSA.** Eduardo: *«muchas de estas
> entradas en realidad son múltiples cosas; las que no tienen número asignado hay que asignarle un
> número»*. Los tres de la cola heredada pasan a `#482` (packs del S3), `#483` (la Metro) y `#484`
> (`listDir`); y se parten las que escondían varias: de `#462` salen `#485` (el `io` del P4 por
> debajo de su VM) y `#486` (el `strcmp` del bucle GUI), y de `#480` sale `#487` (el breadcrumb).
> **La cuenta sube de 9 a 12 y eso es lo correcto**: el trabajo era exactamente el mismo, lo que
> pasaba es que la lista mentía a la baja. Ya no queda ninguna viñeta sin número, y **el rótulo se
> saca contando la lista, no a mano** — que es como se coló el desfase del 8-sep.
>
> ✅ **`#456` cerrada del todo el 9-sep**: Eduardo confirmó el único número que queda,
> `BPVM_FS_PATH_MAX` = 256 (*«256 está bien»*).
>
> | dónde | cuántas | cuáles |
> |---|---|---|
> | **fichas de V6** | **0** | — (`#473` cerrada el 10-sep) |
> | **hitos** | **3** | ~~`G2`~~ (✅ 11-sep) → ~~`C1`~~ (✅ 11-sep; P4 por ver en placa) → `T1` (pruebas) → 🧊 **CODE FREEZE V6** → `D1` (documentación) → `F1` (pruebas finales) |
>
> ✅ **De los hitos de unificación y arquitectura no queda ninguno abierto**: U1–U6, A1–A3, N1,
> E1, G1, P1 y P2, todos cerrados; `L1` se fue a V7. 🧊 **`A4` ya no cuenta**: el 9-sep salió del
> plan de versiones — *«no es V6 y probablemente tampoco V7; hay muchas cosas antes»*.
>
> 📌 **El orden de cierre**: los 15 pendientes primero; después `C1` y `T1`, que son **la
> herramienta con la que se hacen las pruebas finales**; y al final documentar y probar.
>
> 📌 **Cómo se hizo, porque el aviso que había aquí decía otra cosa.** Este bloque avisaba de
> que en el bloque heredado *«hay 59 entradas y unas 43 llevan marca de cierre»* y que separarlas
> era trabajo aparte porque *«un clasificador automático ya falló en dos»*. Se hizo el 7-sep, y la
> forma que funcionó fue **no clasificar por la marca**: 24 agentes leyeron las 69 fichas enteras,
> y cada candidata a abierta se releyó por segunda vez comprobando además que sus commits existen.
> La cola heredada son **12 viñetas, no 59**, y **8 están cerradas**.
>
> ⚠️ **Y salieron seis contradicciones**, todas de la misma forma —el titular decía una cosa y el
> cuerpo otra—, corregidas el mismo día: `E1` y `G1` cerrados con la tabla diciendo «abierto»;
> `A1` cerrada con la cabecera diciendo «abierta»; `A3` y `A4` sin fila en la tabla de hitos;
> `#408` con el enunciado pendiente y la respuesta dos líneas más abajo; el bug del LSP cerrado
> por retirada y la viñeta sin decirlo; y la de las bases de datos en 🔴 mientras `E1` la daba por
> hecha. **Una lista larga esconde**: seis de las que parecían pendientes no lo eran.

### ═══ V6 — LA VERSIÓN EN CURSO ═══

### 🏁 LA UNIFICACIÓN, CONCLUIDA (Eduardo, 5-sep-2026)

> *«Con eso creo que el proceso de unificación lo podemos dar por concluido. Ahora cualquier cosa
> que implementemos nueva o reformemos prácticamente hay que hacer 1 vez. Eso nos permite crecer
> de forma lineal, sin que el número de placas/micros afecte apenas.»*

**Qué se cierra**: la serie `U1`–`U6` (transporte, REPL, stdlib embebida, tabla de handles,
organización de la memoria), `P1`–`P2` (dos placas nuevas y la primera pantalla SPI) y `A1` (la
arquitectura de ejecución: dos hilos de SO, `vm` e `io`, iguales en las cinco familias, con
FreeRTOS en todas).

**Con qué número se cierra**: el censo `A2` — **91,8 % del código de cada firmware es común**,
7,6 % de familia y **0,7 % de placa**, medido por el `.map` del enlazador y no por las carpetas.
Y lo único que queda como código de placa es **la pantalla** del P4 y del C6, que es donde debe
estar.

**Lo que eso significa en la práctica, y es lo que Eduardo señala**: el coste de una feature deja
de multiplicarse por el número de placas. Cuando el C6 entró (ficha `P1.C6`) fueron **siete
ficheros y una línea de cintura**; cuando `A1.2` llevó las dos tareas a la familia ESP32 fueron
**veinte líneas en `esp32/common`** y las otras tres placas lo heredaron sin tocar nada; y `A1.3`
se cerró **sin escribir código**, sólo grabando y midiendo. Ése es el crecimiento lineal.

⚠️ **Y lo que NO cubre esta declaración**, porque conviene no confundir «concluido» con «no queda
nada»: una **familia** nueva (un silicio con otro SDK) sigue costando su cintura entera —lo barato
es la placa dentro de una familia, no la familia—; y quedan abiertos, con su ficha y su medida,
`A1.6` (el hilo BP para `Gui.run()`), la migración de la plataforma de la Pico (parada por un 32 %
sin explicar) y `#468` (la stdlib en pack XIP). Nada de eso es unificación pendiente: son features
y una duda medida.


El índice de todo lo aplazado está en `V6_BACKLOG.md`; los diseños ya trabajados, en
`V6_IDEAS.md`. Aquí vive el estado.


#### 🟢 LA BATERÍA DE V4 SOBRE LA PICO 2: de 40/48 a **48/48** (29-ago)

Ayer, 8 rojos. Hoy **ninguno**, y las causas reales eran menos de las que
parecían:

| sample | ayer | hoy | causa real |
|---|---|---|---|
| `JsonDemo` | `exit 6`, código pisado | ✅ | tabla de símbolos desbordando el margen (#440/#449) |
| `stacktrace`, `MemT4b` | cuelgue mudo | ✅ | `PICO_MALLOC_PANIC` (#448) |
| `MemT5_Gc`, `synctest`, `PropLongTest`, `ThreadFieldTest` | cuelgue / error | ✅ | presión de memoria, resuelta por #448 + #449 |
| `synclisttest` | cuelgue mudo | ✅ | **la tabla de handles no cabía en el margen** → #451, cerrada esa misma tarde |

⚠️ **Y DOS DE LOS FALLOS DE HOY NO ERAN DE NINGÚN PROGRAMA: eran del andamio del
MPU.** `StackTrace` y `ThreadFieldTest` están sanos; los mató el testigo. Está
contado en #440, y la lección en el bloque de abajo.

#### ✅ `#451` — la tabla de handles no puede crecer: el margen de 64 KB otra vez (**CERRADA 29-ago** · `1ebd034f`+`87e9ae20`)

> ✅ **VERDE EN LA PICO 2**, `exit 0`, el programa entero. **La batería queda 48/48.**
>
> **El arreglo**: la tabla se muda DENTRO del bloque de la VM, al FINAL del heap
> —`[módulos][heap →][TABLA][pilas]`—. Criterio de Eduardo para el sitio: al principio
> no, que ahí va la arena de SQLite, y este espacio *«nunca será ejecutable»*.
>
> | | heap | antes | ahora |
> |---|---|---|---|
> | Pico 2 | 257 KB | ❌ moría a 1800 items | ✅ tabla a 3848 slots |
> | S3 · C6 · C3 | 96 KB | ❌ tope 1536, imposible | ✅ tabla a 4096 slots |
> | respaldo S3 | 64 KB | ❌ | ❌ pero por el **heap de verdad**, no por la tabla |
>
> 🎁 **Y el crecimiento dejó de costar el doble.** Un `realloc` necesita el array viejo y
> el nuevo a la vez —doblar de 256 a 512 pedía 99 KB en un margen de 64, que era
> exactamente lo que fallaba—. Dentro del bloque la tabla crece **hacia abajo** y las
> regiones se solapan: un `memmove` y ya. El pico es el tamaño final, no 2×.
>
> 📌 **Se retiró el tope proporcional (`heap/64`)**: era una política que hacía falta
> mientras la tabla salía de otra bolsa y había que adivinar cuánto gastar de ella — y
> adivinaba mal, porque asumía 64 B por objeto y los reales son 25. Ahora el límite es
> físico: la tabla baja hasta tocar el heap, y quedarse sin handles ya *es* quedarse sin
> memoria.
>
> ⚠️ **Dos cosas se rompieron por el camino, y las dos enseñan:**
> 1. `test-smphandles` pasó de 0 corrupciones a **200.000 (todas)**. No era una carrera:
>    `bpvm_init` deja `heap_start = heap_next = stack_base` como marcador de «aún no hay
>    módulos», así que la tabla caía por debajo y **nunca llegaba a existir**. Un heap
>    vacío se desplaza gratis; ahora lo hace.
> 2. **El operador estaba mal justo donde la frase estaba mal.** Yo había escrito «el heap
>    empieza donde acaba la tabla» —al revés— y la comprobación decía
>    `nuevo_top <= heap_next + reserva`, que rechaza la igualdad. Lo cazó Eduardo leyendo
>    el comentario: si la tabla va al FINAL, **«el heap ACABA donde EMPIEZA la tabla»**, y
>    entonces que coincidan es el caso normal, no una colisión. Una frase mal dicha y un
>    `<=` de más eran el mismo error.

#### 🔵 `#451`(histórico) — el diagnóstico, tal como se escribió (29-ago)

`synclisttest` muere con `No space in heap` **teniendo el heap al 20 %**. El log
de la placa lo dice entero:

```
[gc] vivo=39796  heap=[16512..65492)                      <- 49 KB usados
[bpvm] GC: bump 65492 -> 65492 (... heap 16512..273720)   <- de 251 KB
[bpvm] tabla de handles: 2008 -> 4016 slots (31 KB)
[bpvm] throw: "No space in heap"                          <- con 200 KB libres
```

Y la línea de después del throw dice `1946/**2008**`: ese crecimiento **no
ocurrió**. El `realloc` falló en el margen de plataforma:

```
tabla de símbolos ......... 12,0 KB
tabla de handles vieja .... 15,7 KB
tabla de handles nueva .... 31,4 KB   (el realloc necesita las dos)
                            -------
                             59,1 KB   en un margen de 64
```

📌 **Por qué no es un caso raro:** el objeto medio de este programa mide **25
bytes**, y el reparto proporcional de #449 (`heap/64`) asume 64. Todo programa de
objetos pequeños agota los handles teniendo heap de sobra — aquí, 200 KB de sobra.
No es un tope mal puesto: es que **las dos memorias están separadas y no se
prestan nada**.

⏩ **Lo cierra `U6`** (la tabla dentro del bloque de la VM): entonces 30 KB de
tabla salen de 251 KB de heap y el problema desaparece por construcción. Mientras
sigan siendo dos bolsas, esto reaparecerá.

#### ✅ `#450` — `stack=N` (KB): el reparto pilas/montón lo decide el usuario (cerrada 29-ago)

Petición de Eduardo mientras se arreglaba #449: *«podemos añadir una variable de entorno
`stack=100` y que el usuario configure el tamaño del stack. Si no existe, el tamaño como
hasta ahora»* — y en **kilobytes**, *«más sencillo para el usuario»*.

El reparto era 25 % pilas / 75 % montón, y esa proporción **es una apuesta sobre el
programa**: uno con muchos hilos se queda sin pilas teniendo montón de sobra; uno de un
solo hilo que mueve datos desperdicia el 25 %. En la Pico 2 son 89 KB de los 358 del
bloque, y no había forma de moverlos sin recompilar.

**Dónde entró, que es lo que hace que sea barato:** la regla ya estaba unificada en
`bpvm_stack_region_bytes()` (`src/bpvm.c`) desde antes; las familias sólo la consultan. Así
que el ENV **alimenta** la regla, no la duplica: un `bpvm_set_stack_kb()` en el núcleo y
**una línea por familia** para leer la clave. Cinco imágenes, cinco líneas idénticas.

- Ausente o `0` ⇒ **exactamente** lo de siempre. Verificado: sin la opción, el `stdout` del
  host es byte-idéntico al de antes del cambio.
- Los topes (16 KB … la mitad del bloque) **no son paternalismo**: por debajo del stack del
  `main` la VM no arranca (`bpvm_init` devuelve NULL) y por encima de la mitad no queda
  montón. Un valor del env no puede dejar la placa inútil, así que se ajusta **y se dice**:

  ```
  [bpvm] env stack=9999 KB fuera de rango: se usan 256 KB (permitido 16..256 KB con un bloque de 512 KB)
  ```

**Probado en el host antes que en placa** (la cascada), con `--stack=N`, que es el espejo
de la clave igual que `--nogc` lo es de `gc=0`. El testigo de que el reparto se mueve de
verdad no es el mensaje: son los slots de la tabla de handles, que son proporcionales al
montón y salen de otro sitio —

| | `<default>` | `--stack=16` | `--stack=100` | `--stack=200` |
|---|---|---|---|---|
| slots | 474 | 954 | 786 | 586 |

Documentada para el usuario en `guia-ide.html` §9.3. Las cinco imágenes construidas y
**verificado en el binario** (no en el log del build) que la llamada existe: 1 sitio en
cada una.

✅ **VERIFICADO EN PLACA** (Pico 2, 29-ago), y el testigo es el propio INFO:

```
sin la clave    VM : heap 267 KB + stack 89 KB
stack=64        VM : heap 293 KB + stack 64 KB
```

Los 25 KB que se le quitan a la pila aparecen en el montón y el bloque total no se mueve
(356/357 KB, la diferencia es el redondeo a KB del INFO). O sea que no es sólo que la clave
se lea: es que **el reparto cambia de verdad y no se pierde memoria por el camino**.

#### ✅ ERAN DOS SÍNTOMAS DE UNA SOLA CAUSA (medido y CERRADO 29-ago) — leer esto antes que #440 y #449

> **Actualiza el cuadro de abajo, que se deja como está por lo que enseña.** El 28-ago
> #440 y #449 parecían dos problemas: uno determinista y mudo, otro variable y ruidoso.
> El 29-ago la medida dice que **es el mismo**, y por eso el cuadro no conseguía separarlos.

**LA CAUSA, medida en el caso real** (`JsonDemo` + `Json` + `Core` en el host, contando
los símbolos según se registran):

```
460 simbolos  ·  132 B cada uno (char name[128] + addr)  =  59 KB vivos
la tabla crece DUPLICANDO, y el realloc necesita el viejo Y el nuevo:
    256 entradas (33 KB)  ->  512 (66 KB)   =   99 KB A LA VEZ
... dentro de un margen de malloc de 64 KB.
```

Y ahí se parten los dos síntomas, que es lo que despistaba:

- **Antes del arreglo de la frontera** (`40a34b24`): `malloc` no tenía tope, así que los
  99 KB **entraban en el bloque de la VM** y la tabla se escribía encima del código de
  `Json`. De ahí `43 6F 72 65` en `Json+3754`: **no era un opcode, era el nombre de un
  símbolo** — la cadena `Core.…`. Eso es #440, y por eso era determinista y no dejaba
  rastro: no fallaba nada, se escribía en el sitio equivocado.
- **Después del arreglo**: `_sbrk` corta, el `realloc` devuelve NULL y `Core` se queda sin
  registrar → `exit 11 (lib 'Core' presente pero no exporta 'Core.__init')`. Eso es #449.

O sea que **el arreglo de ayer no cambió el problema, cambió el síntoma**: de corrupción
silenciosa a error honesto. Que era exactamente lo que se buscaba.

⏩ **El arreglo** (29-ago): el nombre sale de la struct y va a un **pool de cadenas** —
8 B por entrada más el nombre exacto. Los mismos 460 símbolos:

| | vivos | pico del `realloc` |
|---|---|---|
| antes | 59,3 KB | **99,0 KB** |
| ahora | 20,0 KB | **28,0 KB** |

📌 **Y por qué un pool y no un `name[40]`**, que también habría entrado: la medida dice que
el nombre más largo son 35 caracteres, pero un array fijo **trunca**, y dos nombres que
compartan prefijo y se pasen del tamaño pasan a ser **el mismo símbolo** — el enlace
resolvería a la función equivocada, en silencio. Eso es peor que quedarse sin memoria, que
al menos se ve. El pool no trunca nunca *y* cuesta menos: no había que elegir.

🔦 **Lo que faltaba era un instrumento, no una idea.** El consumidor más grande del margen
era invisible: la tabla de handles decía su tamaño desde #430, la de símbolos no decía
nada. Ahora lo dice, con la misma forma:

```
[bpvm] tabla de simbolos: 460 simbolos, 11564 B de nombres, 20480 B en total
```

---

*(El cuadro del 28-ago, tal como se escribió, porque enseña cómo se ve una causa única
desde dentro:)*

| | **#449 — presión de memoria** | **#440 — código pisado** |
|---|---|---|
| síntoma | cuelgue mudo → ahora error con nombre | `opcode 0x43` en `Json+3754` |
| determinista | no (depende de cuánto gaste) | **sí**, mismo PC en todas las imágenes |
| en el log | `PANIC: Out of memory`, tabla agotada | **nada**: ni panic, ni OOM, ni tabla |
| cuántos samples | 7 de los 8 rojos | 1 |
| ventana | #430, 16-ago | por determinar |
| estado | causa localizada, **arreglo por decidir** | **abierto**, con pista concreta |

La fila que estaba mal era «en el log: **nada**». No era que #440 no dejara rastro: es que
el rastro *era* el propio dato corrupto, y no había quien contase lo que ocupaba la tabla.

**La batería de V4 sobre la Pico 2, 28-ago: 40 verdes de 48.**

#### ✅ `#440` — `JsonDemo`: el código de un módulo aparece PISADO en la RP2350 (abierta 27-ago · **CERRADA 29-ago** · `e4957d7c`)

> ✅ **VERDE EN LA PICO 2**, `exit 0`. Y no sólo verde: la salida es **byte-idéntica a la de
> la VM-Java y a la del host** — el invariante de las dos VMs se cumple sobre el programa
> que llevaba dos días roto.
>
> **La causa**: los bytes `43 6F 72 65` de `Json+3754` **eran el nombre de un símbolo**
> (`Core.…`), no un opcode. La tabla de símbolos desbordaba el margen de `malloc` y se
> escribía sobre el código. Arreglado por dos vías: la frontera de `_sbrk` (`40a34b24`) y
> el pool de nombres (`e4957d7c`).
>
> ⚠️ **La pista de `loader.c:224` que hay abajo era FALSA** — el scratch de exports no tenía
> nada que ver. Se deja escrita a propósito: era la única escritura grande fuera de sitio
> que se veía leyendo el código, encajaba con la asimetría Metro/Pico 2, y aun así no era.
> Lo que la descartó no fue leer más código, fue **contar**.
>
> 🔻 **Y EL TESTIGO DEL MPU MATÓ DOS PROGRAMAS SANOS** (29-ago). Cazó al culpable
> —`_malloc_r`— y luego dio **dos falsos positivos**, los dos por la misma clase de
> defecto: *estado que sobrevive cuando no debería*.
>
> 1. **Se armaba antes del último enlace.** Mi comentario decía que era seguro «porque
>    `bpvm_link_all` es idempotente». Idempotente significa que escribe el **mismo
>    valor** — pero sigue siendo una **escritura**, y al MPU el valor le da igual. Cayó
>    `StackTrace` sobre su propio fixup de `eh_class` (`code_off=461`). Arreglado con un
>    gancho post-enlace en el núcleo (`bpvm_set_after_link_hook`).
> 2. **El desarmado no apagaba las regiones**, sólo `CTRL`. Un programa de 4 módulos
>    dejaba 4 regiones vivas; el siguiente, de 2, reconfiguraba dos y heredaba las otras
>    dos apuntando a lo que ya era **heap**. Cayó `ThreadFieldTest` en la quinta ejecución,
>    con un `MMFAR` **fuera** de las regiones que el propio log listaba — que es lo que
>    delató el asunto.
>
> 📌 **La lección, y es la del día entero:** *un andamio de diagnóstico es código de
> producción.* Las tres pérdidas de tiempo del 29-ago no fueron programas rotos, fueron
> **instrumentos que mentían** (estos dos y el aviso con `static` de #449). Y las tres
> tenían la misma forma: algo que se guardaba de una ejecución a la siguiente.

**Síntoma**: `exit 6 (opcode 0x43 desconocido)`, siempre en el mismo sitio. Con el mensaje
mejorado (#442) el sitio ya tiene nombre:

```
opcode 0x43 desconocido en PC 7784 = Json+3754
  bytes @7780: FF 32 00 00 [43] 6F 72 65        <- 43 6F 72 65 = "Core" en ASCII
```

Y en el `.mod` de disco ese offset tiene **código**, no datos:

```
+3752  08 04     RET   4
+3754  0F 00 04  ENTER 4
```

O sea: **el código está pisado**, no es un salto a sitio equivocado. Esa distinción costó
una tarde el 27-ago y ahora la contesta el propio mensaje.

**Lo MEDIDO** (no interpretado):
- El CRC del `.mod` en la placa coincide con el del PC → los bytes llegan bien.
- Los 27 `.mod` de la stdlib son byte a byte idénticos en V4, V5 y hoy; el fuente de
  `JsonDemo.bp` también; y los compiladores de V4 y V5 producen el **mismo**
  `JsonDemo.mod`. La diferencia **no** está en el lado del PC.
- `PicoA` (sin imports) va; `PicoB` (sólo añade `import Json`, sin usarlo) falla.
- **Verde en la Metro** con la misma imagen y ficheros — pero ver el cuadro de arriba:
  allí tampoco hay presión de memoria, así que ese verde no separa las causas.
- **No lo cura #448**: con el `malloc` arreglado sigue dando `exit 6`, y el log no trae ni
  un evento de memoria. Es lo que lo separa de #449.
- No depende del camino de lanzamiento (Run desde el PC y doble clic dan lo mismo).

**Descartado con medida**: la migración del REPL del 27-ago (falla igual con la imagen
anterior), `Core.mod` rancio en `/app` (falla igual viniendo de `/lib`), desfase de la
stdlib (el `Core.mod` embebido, el de `bpstdlib/out` y el que sube el IDE son **el mismo
fichero**), el escaneo de `.mdn` (no-op en la ejecución que falló), el parcheo AOT (escribe
`0xAA`), el tamaño del `.mod` (rojos 1861-4730 B, verdes 1808-4372: solapan), y el
desalineamiento (`CFSR=0`).

**⏩ LA PISTA, y es por donde entrar.** `src/loader.c:224`:

```c
bc_read(&c, vm->memory + end_addr, exports_size);   /* scratch DETRAS del modulo */
exp_buf = vm->memory + end_addr;
```

La sección de **exports se lee a la memoria de la VM justo detrás del módulo**, como zona
de trabajo — y para `Json` son **7773 bytes**. Es el primer sitio donde el cargador escribe
kilobytes en una zona que no es suya. En la Metro da igual (heap en PSRAM); en la Pico 2
todo comparte el mismo bloque de 357 KB.

⚠️ **No está cerrado**: por la aritmética de las bases ese scratch NO debería caer en 7784
(con el orden JsonDemo→Json→Core las direcciones sólo suben). Falta leer el flujo del
cargador. Pero es la única escritura grande fuera de sitio que hay, y encaja con la
asimetría Metro/Pico 2.

Bases del caso, para no recalcularlas: `JsonDemo cb=0x474 (1140)` · `Json cb=0xFBE (4030)`,
code 8877 · `Core cb=0x368B (13963)`, code 4998 · heap desde `end_addr+64`.

#### ✅ `U6` — la ORGANIZACIÓN DE LA MEMORIA no está unificada, y no estaba prevista (abierta 28-ago · **CERRADA 3-sep**: `U6.8`–`U6.11`, las cinco familias con `bpvm_mem_plan()` y verificadas en placa)

Pregunta de Eduardo al cerrar el día: *«La organización de memoria no sé cómo se está
haciendo hoy en día. ¿Es por micro o por familia? ¿Está previsto unificarlo?»*

**Es por MICRO**, y son cuatro mecanismos distintos inventados por separado:

| | cómo decide su memoria de VM |
|---|---|
| Pico / Metro | runtime, desde símbolos del linker (`end`, base de la RAM de packs); o la PSRAM entera si `board_desc()` la declara |
| STM32 | array estático: `static uint8_t s_vm_mem[512*1024]` (`stm32_repl.c:81`) |
| ESP32-S3 | `heap_caps_malloc(160 KB)` con escalón de respaldo a 128 (`esp32/main/main.c:49`) |
| ESP32-P4 | de la PSRAM, restando la reserva del display |

**Y no estaba previsto**: la serie U cubre el wire (U2), el REPL (U3), los blobs de la
stdlib (U4) y darle módulo a la tabla de handles (U5). La organización de la memoria **no
está en la lista**.

**Lo que costó no tenerlo** (28-ago, #440 + #449): tres decisiones de memoria que no se
hablan entre sí —
- el margen de `malloc` (64 KB) es una constante de la Pico que nadie más conoce
  (`pico/main.c`),
- el arranque de la tabla de handles (4096 slots = 32 KB) lo decide `src/heap.c` **sin
  saber en qué placa está**,
- y la frontera entre el heap de `malloc` y el bloque de la VM **vivía sólo en un
  comentario**: `_sbrk` se limita en `__StackLimit`, así que `malloc` la cruzaba en
  silencio y escribía dentro del código de un módulo.

Dos días de investigación, y el síntoma era un opcode imposible.

⏩ **La forma, que es la misma que U2/U3**: un contrato con cintura. La familia dice **dónde
y cuánto** —lo único genuinamente suyo: PSRAM, SRAM, símbolos del linker, `heap_caps`— y el
común decide el **reparto**: heap, pilas, tabla de handles y margen, con una regla
proporcional en un solo sitio. Hoy el reparto está replicado y divergente en cuatro
puertos.

📌 **Y el criterio que lo hace urgente** (Eduardo, 28-ago): *«estamos poniendo la misma
tabla para un micro de 520K y otro de 8M+520K. El tamaño de la tabla debe ser proporcional
al tamaño del heap.»* Mientras el reparto sea constantes sueltas, cada placa nueva es otra
oportunidad de poner mal el número — y la placa más estricta es la que menos se prueba
(ver #449).

##### 📊 `U6.0` — EL CENSO (31-ago). *«Medir dos veces antes de cortar»* (Eduardo)

Leído del código, sin placas. Las cuatro preguntas de la ficha, para los cinco puertos **y los
dos entornos de PC**, que también son consumidores y estaban fuera del recuento.

| | **DÓNDE** vive el bloque | **CUÁNTO** mide | **REPARTO** | **TECHO**, y quién lo sabe |
|---|---|---|---|---|
| **Pico/Metro** (SRAM) | `vm_sram_region()`: de `&end + 64 KB` a `PACK_RAM_SRAM_BASE` | **lo que quede** | `bpvm_stack_region_bytes` | símbolos del **linker** — se sabe **antes** |
| **Pico/Metro** (PSRAM) | `PSRAM_XIP_BASE` | `board_desc()->psram_bytes` | idem | el descriptor de placa |
| **STM32** | array estático en `.bss` | **constante 512 KB** | idem | el **enlazador**: si no cabe, no enlaza |
| **ESP32-S3** | `heap_caps_malloc(INTERNAL)` | **constante 160 KB** (+respaldo 128) | idem | bloque contiguo — **sólo pidiéndolo** |
| **ESP32-C3** | idem | **constante 128 KB** (+respaldo 96) | idem | idem |
| **ESP32-P4** | `heap_caps_malloc(SPIRAM)` | **lo que quede** − 4 MB de display, reintentando −1 MiB hasta un piso de 2 MB | idem | PSRAM libre, y se ajusta reintentando |
| **host** (`test/main.c`) | `malloc` | `--mem` | ⚠️ **mitad y mitad** salvo `--stack` | — |
| **simulador** | `calloc` | `--mem` (512 KB por defecto) | ⚠️ **mitad y mitad SIEMPRE** | — |

### Lo que el censo destapa

**1️⃣ «Cuánto» son DOS filosofías mezcladas sin decirlo.**

- *«todo lo que quede»* — Pico y P4: **calculan**, y se adaptan solos a la placa.
- *«una constante»* — STM32 (512 KB), S3 (160), C3 (128): un número escrito a mano.

Las constantes son justo las que se ponen mal en cada placa nueva, y **hoy hemos añadido la
tercera** (`P1.C3.3`). El P4 además **reintenta bajando 1 MiB**; el S3 y el C3 tienen **un solo
escalón de respaldo**. Dos maneras distintas de tratar el mismo caso —«no cabe lo que pedí»—
inventadas por separado.

**2️⃣ El REPARTO está unificado en las placas… y NO en los dos entornos de PC.**

Las tres familias llaman a `bpvm_stack_region_bytes` (25 %, suelo de 64 KB). El host va a
**mitad y mitad** salvo que le pasen `--stack`, y **el simulador siempre**. Y hay más: las tres
familias leen `stack=N` del ENV (`bpvm_set_stack_kb`) y **el simulador no lo lee**, teniendo
gestor de placa y ENV.

📌 Esto ya mordió una vez, y está escrito en `repl_esp32.c`: *«Esta función tenía una COPIA de la
regla, y se había quedado en /2 mientras el Pico ya iba por /4 — dos placas repartiendo distinto
sin que nadie lo hubiera decidido.»* La copia del ESP32 se arregló; **las dos del PC siguen
ahí**.

⚠️ Y es peor de lo que parece por lo que acabamos de hacer en `U3.24`: el simulador es ahora el
arnés del REPL común. Un doble **más amable que el original** —reparte distinto e ignora el
mando— es exactamente lo que no queremos de un arnés ([[doble-mas-amable-que-el-original]]).

**3️⃣ El TECHO tiene tres regímenes, y sólo uno se conoce por adelantado.**

- **Se sabe antes** (Pico): sale de símbolos del linker; el cálculo *es* el techo.
- **Lo comprueba el enlazador** (STM32): el array no cabe ⇒ no enlaza. Falla pronto y fuerte.
- **Sólo se sabe pidiéndolo** (ESP32): `heap_caps_get_largest_free_block`, y **después** de
  arrancar el IDF.

El tercero es el que sorprende, y hoy nos sorprendió: en el C3 hay 280 KB libres y el mayor
bloque son 136 KB. **Ningún contrato de hoy expresa eso** — se descubrió leyendo un log.

**4️⃣ El margen tiene nombre en DOS puertos, y en los otros dos vive en un comentario.**

⚠️ *Corregido el 31-ago: esta entrada decía «sólo existe en la Pico» y era **falso**. Al ir a
trabajar los casos sin PSRAM salió que el STM32 también lo tiene, sólo que en el sitio donde no
lo busqué — el script del enlazador.*

| puerto | el margen | ¿nombrado? | ¿comprobado? |
|---|---|---|---|
| **Pico** | `VM_SRAM_MALLOC_MARGIN` = 64 KB | ✅ en C | el cálculo de `vm_sram_region()` lo respeta |
| **STM32** | `_Min_Heap_Size` 16K + `_Min_Stack_Size` 4K | ✅ en el `.ld` | ✅ **por el ENLAZADOR**: la sección `._user_heap_stack` los reserva dentro de `>RAM`, así que si no cabe **no compila** |
| **ESP32-S3** | 86 KB (medidos, `#336`) | ❌ sólo en el comentario que justifica la constante | ❌ |
| **ESP32-C3** | 17,5 KB (medidos, `P1.C3.3`) | ❌ ídem | ❌ |

Y la constante del STM32 tampoco es un número a ojo: lleva su presupuesto escrito —*768 total −
512 aquí − ~119 del resto del estático = ~137 KB libres, y el linker sólo exige 20; margen de
6×*— y nació corrigiendo un descuido real (`H13` hallazgo 31: eran 128 KB porque *«se fijó al
nacer el port y nadie volvió a mirarlo»*, con ~520 KB parados).

📌 **Así que el hueco es MÁS PEQUEÑO de lo que decía este censo: son los dos ESP32.** Y en los
dos el número **ya está medido** — existe como razonamiento en un comentario, pero no como
cantidad que el código use. Eso es lo que hay que cambiar: no medir de nuevo, sino **darle
nombre a lo medido y dejar que el tamaño se calcule**, como hace la Pico.

Lo que costó no tenerlo sigue en pie: `#440`/`#449` fueron dos días y el síntoma era un opcode
imposible.

### ✅ Lo que YA está bien y no hay que tocar

- La **tabla de handles** es proporcional al heap desde `#449` (`handle_slots_por_heap`:
  heap/512, piso 256) y vive **dentro** del bloque desde el 29-ago. Es el único de los cuatro
  repartos que ya se decide en un solo sitio y con una regla, no con una constante.
- La **regla del reparto** existe y es única (`bpvm_stack_region_bytes`). El problema no es que
  falte: es que **hay tres consumidores que no la llaman**.

### ⏭️ La forma que sugiere el censo

La misma que `U2`/`U3`: **un contrato con cintura**.

| | |
|---|---|
| **la familia dice** | *dónde* puede vivir, *cuánto* hay como mucho (el techo, en su régimen) y *qué pasa si no cabe* |
| **el común decide** | *cuánto toma* (la política «todo lo que quede» o «esta constante», elegida UNA vez), *cómo reparte* (heap / pilas / handles / margen) y *cómo baja* si no cabe |

Y el primer paso que cierra algo por sí solo, siguiendo `U3.24`: **que el simulador y el host
usen la regla común y lean `stack=N`**. Son los dos consumidores que ya tenemos en el PC, se
verifica sin placa, y convierte el arnés en un doble fiel antes de empezar a mover nada de las
placas.


##### ✅ `U6.1` — el simulador y el host reparten como las placas (31-ago)

El primero de los cuatro hallazgos del censo, y el que se cierra sin placa. Los tres
consumidores que no llamaban a la regla común eran el **host** y el **simulador** (la copia del
ESP32 ya se había arreglado en su día).

| | antes | ahora |
|---|---|---|
| host (`test/main.c`) | mitad y mitad **salvo `--stack`** | `bpvm_stack_region_bytes` siempre; `--stack` sigue mandando |
| simulador | mitad y mitad **siempre** | idem |
| simulador: `stack=N` del ENV | **no lo leía** | lo lee **en el arranque**, como las tres placas |
| simulador: `vmHeapBytes`/`vmStackBytes` del `INFO` | **cero** | el reparto de verdad |

Con `--mem=512K`: pilas 131072, heap 393216 — exactamente `max(512K/4, 64K)`. Y con `stack=96`
en el ENV: 98304 / 425984.

📌 **Se lee en el ARRANQUE y no en cada `Run`, a propósito.** En la placa el valor se recoge en
el boot y hace falta un reset para que entre. Aplicarlo por `Run` sería más cómodo en el
simulador y por eso mismo **infiel**: el doble tiene que doler donde duele la placa
([[doble-mas-amable-que-el-original]]).

⚠️ **Y por qué importaba más de lo que parecía**: desde `U3.24` el simulador es el **arnés** del
REPL común. Un arnés que reparte su memoria distinto de la placa e ignora el mando que la placa
sí honra es un doble más amable que el original — y esos no cazan nada. El `INFO` a cero era la
misma familia de problema: un cero **parece un dato** y no lo es.

🐛 **Un fallo mío por el camino, y lo caza la verificación, no la lectura.** Escribí
`if (bpvm_bmgr_env(&g_bm, &env) == 0)` y esa función devuelve **1 cuando SÍ hay env**: la
condición estaba invertida, así que sólo aplicaba `stack=N` cuando no había ninguno. Compilaba y
se leía bien. Salió al comprobar el número contra el esperado, que es el gesto que no me puedo
saltar.

✅ **Verificado**: paridad **38 PASS / 0 FAIL / 0 SKIP** (el host cambia de reparto y la salida
no se mueve), `sim-smoke` **40/40**, `boardsim-smoke` verde, y el reparto comprobado contra la
regla con y sin `stack=N`.

⏭️ Quedan los otros tres hallazgos del censo: las **dos filosofías** de «cuánto» (constante vs
lo-que-quede) con sus dos formas distintas de bajar cuando no cabe, los **tres regímenes de
techo**, y el **margen sin nombre** en cuatro de los cinco puertos.


##### ✅ `U6.2` — el DÓNDE va antes que el CUÁNTO (Eduardo, 31-ago) — *absorbida por `U6.8`–`U6.11`; U6 cerrada el 3-sep*

> *«Quizás antes que el cuánto, hay que plantearse el dónde. Así que el orden sería ¿hay PSRAM?
> y si hay el reparto se hace de una manera, y si no hay se hace de otra.»*

Puesto contra el censo `U6.0`, **explica el hallazgo 1** — que había dos filosofías de «cuánto»
mezcladas sin decirlo. No eran dos filosofías rivales: era **la misma regla aplicada a dos sitios
distintos**, y faltaba nombrar la diferencia.

| | **de quién es esa memoria** | cómo decide hoy el bloque |
|---|---|---|
| Pico/Metro **con** PSRAM | **exclusiva** de la VM | toda la PSRAM − la región de SQLite |
| ESP32-P4 | **exclusiva** | toda la PSRAM libre − 4 MB de display, reintentando −1 MiB |
| Pico **sin** PSRAM | **compartida** (`malloc`, RAM de packs) | el hueco entero − 64 KB de margen **con nombre** − la RAM de packs |
| ESP32-S3 / C3 | **compartida** (IDF, littlefs, wire, tareas) | ⚠️ **una constante** (160 / 128 KB) |
| STM32 | **compartida** (`malloc`) | ⚠️ **una constante** (array estático de 512 KB) |

**Los tres puertos con memoria exclusiva CALCULAN. De los tres con memoria compartida, sólo
calcula el que le puso nombre a su margen.**

### La regla que sale de ahí

```
¿de quién es la memoria?
├── EXCLUSIVA (PSRAM)  → todo, menos las reservas CON NOMBRE (display, SQLite)
└── COMPARTIDA (SRAM)  → todo, menos un margen CON NOMBRE para los demás
                          inquilinos (malloc, RTOS, FS, wire)
```

Es **una** regla, no dos, y la rama sólo cambia *qué hay que descontar* y *a quién se le
pregunta cuánto hay*.

📌 **Y da criterio para el hallazgo 4.** El margen sin nombre no es higiene: es **lo que bloquea
el «cuánto»**. Mientras un puerto no tenga un número que diga *«esto no es de la VM»*, no le
queda más remedio que adivinar el bloque entero — y adivinar es exactamente lo que se pone mal
en cada placa nueva (tres veces ya: STM32, S3 y hoy el C3).

⚠️ **El caso difícil es el ESP32**, y conviene decirlo antes de empezar: allí «cuánto hay» sólo
se sabe **preguntando** (`heap_caps_get_largest_free_block`) y **después** de que el IDF haya
arrancado; además lo que devuelve es el **bloque contiguo**, no el total (`P1.C3.3`). O sea que
la rama compartida necesita dos cosas de la cintura, no una: *cuánto hay* y *cuánto de eso es
contiguo*.

⏭️ **Lo que falta para poder implementarlo** — y es medir, no escribir:

1. **Nombrar el margen en los tres puertos que no lo tienen** (S3, C3, STM32). No inventarlo:
   el S3 y el C3 ya tienen la marca de agua medida (`#336`, `P1.C3.3`), que es justo el número
   —*lo que el sistema consume en marcha*— del que sale el margen. El STM32 no lo tiene medido.
2. **Comprobar que el orden aguanta en el STM32**, que es el raro: su bloque es `.bss`, así que
   el techo lo verifica el ENLAZADOR y el fallo es en compilación, no en arranque. Eso es una
   garantía **más fuerte** que la de los otros dos, y unificar hacia abajo sería perderla
   ([[arreglo-que-no-viaja-entre-familias]] al revés).


##### ✅ `U6.3` — ¿la SRAM ociosa sirve para algo? **Medido: no compensa** (31-ago)

> Pregunta de Eduardo: *«En los casos que hay PSRAM el stack y el heap los subimos
> automáticamente a la PSRAM, OK. Ahora, nos quedamos con memoria SRAM sin utilizar. ¿Se puede
> aprovechar para algo útil?»*

La pregunta es buena y la memoria ociosa es real. **Lo que no compensa es el motivo**.

### Cuánta SRAM se queda sin usar

Contado desde el ELF y confirmado por la placa:

```
estático (.data+.bss) ..........  83 KB
hueco de `end` a la RAM de packs  421 KB
```

- **Pico sin PSRAM**: la VM toma ese hueco menos los 64 KB de margen → **357 KB**, y la placa lo
  dice igual: `vm: SRAM interna 357 KB -> heap 267 KB + stacks 89 KB | libre para malloc: 64 KB`.
- **Metro con PSRAM**: `vm: heap en PSRAM 6 MB @ 0x11200000 (SRAM interna SIN RESERVAR)` — lo
  dice el propio firmware. Esos 421 KB se quedan para `malloc`, que aquí gasta muy poco.
- **P4**: 241 KB de DIRAM usados de 576 → **327 KB libres**, y ahí sí los usa el IDF.

### El experimento: una placa, una imagen, UNA variable

La PSRAM de la Pico la conduce el ENV (`psram=1`, decisión de Eduardo del 19-jul), así que la
misma imagen corre desde SRAM o desde PSRAM con un reset por medio. Silicio, reloj y binario
idénticos.

| Metro RP2350 | secuencial | disperso | coste de la dispersión |
|---|---|---|---|
| **SRAM** (357 KB) | 5694 ms | 5861 ms | +2,9 % |
| **PSRAM** (6 MB) | 5946 ms | 6269 ms | +5,4 % |
| **coste de la PSRAM** | **+4,4 %** | **+7,0 %** | |

Y `Bench.bp` (`fib(28)`, recursivo, conjunto de trabajo = la pila):

| | intérprete | AOT |
|---|---|---|
| SRAM | 8590 ms | 84 ms |
| PSRAM | 8961 ms (**+4,3 %**) | 84 ms (**idéntico**) |

**Tres cargas distintas —recursión, heap secuencial, heap hostil a la caché— y la PSRAM cuesta
entre 4,3 % y 7,0 %.** A cambio la VM pasa de 357 KB a 6 MB: **17× más memoria por 7 % de
velocidad en el peor caso que he sabido construir.**

### Por qué sale tan poco, que es lo interesante

`BenchMem.bp` trae su control, y comparado con el host el resultado se explica solo:

| la misma dispersión cuesta | |
|---|---|
| en el **host** (VM-C, x86) | **+15 %** |
| en la **Metro con PSRAM** | +5,4 % |
| en la **Metro con SRAM** | +2,9 % |

📌 **El intérprete entierra la latencia de memoria.** Donde el intérprete es rápido (host, 190×
más que la placa en `fib`) la memoria asoma; donde es lento, no. Un ciclo de RAM lenta se pierde
dentro de los ~15 opcodes que la VM gasta por iteración. Esto no es una peculiaridad de la
Metro: es una propiedad de *ejecutar bytecode interpretado*, y por tanto vale para todo el sobre.

⚠️ **Lo que este resultado NO dice**: que el AOT no se moviera (84 ms en los dos) es de este
`fib`, que es compute-bound y toca poca memoria. **No** demuestra que el código nativo sea
inmune a la PSRAM — un nativo que recorra estructuras grandes está sin medir.

### Conclusión, y lo que cambia

🔴 **No se pelea por la SRAM ociosa.** Partir el espacio de direcciones, o mover pilas y tabla
de handles a la interna, costaría lo que ya sabemos —el modelo de referencias, el GC, el
`sp`/`bp` que el debugger lee crudos— y compraría **como mucho un 7 %**. No sale.

🟢 **Pero la idea de [[coser dos regiones]] (`V6_IDEAS`) SIGUE VIVA — por otra razón.** En el C3
**no hay PSRAM**: allí coser las dos regiones no compra velocidad, compra **capacidad** (136 KB →
~250 KB), que es lo que a ese micro le falta. El argumento de velocidad ha muerto; el de
capacidad no estaba en duda.

🎁 **Y sale un instrumento**: `samples/benchmarks/BenchMem.bp`, con su control. `Bench.bp` medía
el intérprete con una carga de pila; faltaba una de heap. Ahora las dos existen y las dos son
portables.


##### 🔵 `U6.F1` — con PSRAM, ¿qué buffers NO-BP se pueden agrandar con la SRAM ociosa? (aparcada 31-ago)

> Eduardo, al cerrar `U6.3`: *«Me sigue chirriando el tener memoria RAM ociosa. Apunta para
> estudiar en un futuro: en el caso de PSRAM mirar qué buffer o heaps no BP se pueden agrandar
> para mejorar rendimiento. De momento no lo tocamos.»*

**Es un ángulo DISTINTO del que se descartó**, y conviene no confundirlos. `U6.3` midió mover
*la memoria de la VM* a la SRAM y salió que no compensa (4–7 %). Esto es al revés: dejar la VM
donde está y dar la SRAM ociosa a **lo que no es la VM** — que además es lo único que sigue
viviendo en la interna cuando el heap se va a la PSRAM.

**Cuánto hay**: 421 KB en la Metro (`vm: ... SRAM interna SIN RESERVAR`), 327 KB de DIRAM libre
en el P4 al enlazar.

**Candidatos, sin orden de mérito** (todos por medir):

| | dónde vive hoy | por qué podría importar |
|---|---|---|
| cachés de littlefs (`read`/`prog`/`lookahead`) | dimensionadas para RAM escasa | tocan **cada** operación de FS |
| el buffer del bulk del wire (`V1_PUT_BUF_SIZE`, 8 KB) | SRAM | trocea las subidas; menos trozos = menos vueltas |
| el pool de la tabla de símbolos | dentro del bloque de la VM | 20 KB tras `#449`; ¿se beneficia de estar en la rápida? |
| la RAM ejecutable del AOT | SRAM (ya) | ya está en la buena; confirmar |
| el margen de `malloc` (64 KB en la Pico) | SRAM | está ocioso en la Metro y nadie lo usa |

⚠️ **El método, antes que la lista**: agrandar un buffer que no es el cuello de botella no
compra nada. Primero hay que **medir cuál lo es** — cronometrar un `PUT` grande, un `LIST` de un
FS lleno y una carga de módulo, que son los tres gestos que el usuario espera de la placa. Y con
control, como en `BenchMem`: la misma operación con el buffer de hoy y con uno mayor.

📌 **Y ojo con el sesgo que ya nos ha salido hoy**: en `U6.3` el intérprete enterraba la latencia
de memoria. Aquí puede pasar lo mismo con el FS — si el cuello es la **flash**, agrandar una
caché en RAM no se nota. La medida tiene que separar las dos cosas.


##### ✅ `U6.4` — el margen del S3 estaba MAL: 26,5 KB, no 86 (31-ago)

Antes de convertir el margen en fórmula había que entender por qué el S3 pedía **86 KB** y el C3
**17,5**. La sospecha era el método, y era el método.

Los dos números salían del mismo instrumento (`heap_caps_get_minimum_free_size`) pero **no de la
misma prueba**: el del S3 dice sólo *«tras varios RUN»*, sin decir cuáles. Dos medidas del mismo
instrumento **no se pueden restar si no se sabe qué se ejecutó**.

### La medida, con protocolo escrito

`bpgenvm-c/tools/medir_margen.ps1`: `LOG_CLEAR` → `RESET` → `PUT` → `RUN ×2` → `LIST` →
`LOG_DUMP`. La misma secuencia en las dos placas.

| | **ESP32-S3** | **ESP32-C3** |
|---|---|---|
| bloque de la VM | 160 KB | 128 KB |
| libre al arrancar | 338 368 B | 280 032 B |
| **bloque contiguo mayor** | **270 336 B** | **139 264 B** |
| libre tras reservar | 174 524 B | 148 956 B |
| mínimo histórico | 147 960 B | 131 368 B |
| **consumo del sistema en marcha** | **26 564 B** | **17 588 B** |
| lo que decía la ficha | ~~86 256 B~~ | 17 560 B |

🎯 **El 86 KB era 3,3× el valor real.** Con la misma carga las dos placas se llevan **1,5×** —
creíble para dos núcleos contra uno— y no el 4,9× que sugerían los números viejos. El C3 es
reproducible al 0,16 % (17 560 suelto, 17 588 con protocolo).

📌 **Y el bloque contiguo separa a las dos mucho más que el consumo**: 264 KB en el S3 contra 136
en el C3. Ahí está la diferencia de verdad entre estos dos silicios, no en lo que gasta el
sistema. Es el dato que `P1.C3.3` destapó y que ningún contrato expresa todavía.

### 🐉 De propina: **el C3 es 1,57× MÁS RÁPIDO que el S3**

Ninguna de las dos placas tiene AOT **en la imagen de hoy** (las dos enlazan `aot_funcs_stub.c`),
así que en `Bench.bp` las dos mitades corren interpretadas y `elapsed/2` es el tiempo del
intérprete, exacto:

| | reloj | núcleos | `fib(28)` interpretado | ciclos para el mismo trabajo |
|---|---|---|---|---|
| **C3** (RISC-V) | 160 MHz | 1 | **11 315 ms** | 1,81 × 10¹² |
| **S3** (Xtensa) | 240 MHz | 2 | **17 785 ms** | 4,27 × 10¹² |

**Con un 33 % menos de reloj y un solo núcleo, el C3 gana por 1,57×** — o sea que el S3 necesita
**2,36× más ciclos** para el mismo trabajo, en el bucle del intérprete, que es donde vive el
100 % del tiempo de un programa BP.

⚠️ **Es UNA carga** (recursión con aritmética entera y llamadas), no una comparación general de
CPUs. Pero refuerza lo ya decidido: [[prioridad-arm-riscv-s3-secundario]].

### 🐛 Y una corrección mía, a pregunta de Eduardo

> *«la C3 es RISC-V, debería soportar AOT»*

Tiene razón, y la primera versión de esta ficha daba a entender que el C3 no lo soportaba. **No
es del silicio: es un hueco del port, y encima con un diagnóstico falso escrito en el código.**

Esta mañana, al crear el proyecto del C3 copiando la lista de fuentes del S3, el gate
`BPVM_ESP_AOT_MDN` salió a 0 porque no encontraba `esp_cache.h`, y anoté en `repl_esp32.c` que
*«viene de `esp_mm`, que ese silicio no tiene»*. **Falso**: el `CMakeLists` de `esp_mm` sólo
excluye `linux` y compila `esp_cache_msync.c` para cualquier target. La cabecera faltaba porque
el componente del C3 **no pedía `esp_mm` en `REQUIRES`** — herencia de copiar la lista del S3,
que es Xtensa y no lo necesita.

**Añadida esa línea, el C3 compila con el camino del `.mdn` DENTRO** (las cadenas `[mdn] …` ya
están en la imagen, y antes no). Un «no se puede» que era un «no está pedido» —
[[no-se-puede-vs-no-esta-implementado]] otra vez, y esta vez lo escribí en un comentario donde
va a engañar al siguiente.

La puerta por capacidad sigue siendo lo correcto (y sigue haciendo falta para el S3, que es
Xtensa de verdad); lo que estaba mal era el motivo.

⏭️ **Lo que falta para que el C3 ejecute nativo**, y ahora es un trabajo acotado:

1. Que el IDE compile el `.mdn` con la ISA del C3. La placa ya publica lo que hace falta —
   `bpvm_mdn_host_arch()` y `bpvm_mdn_host_float_abi()`—, y ahí está el matiz: el P4 y el C3
   **comparten el tag `MDN_ARCH_RISCV`** y se distinguen por la **ABI de coma flotante**
   (`ilp32f` con FPU contra `ilp32` sin ella). Ese mecanismo existe justo por esto, y el
   comentario de `mdn_loader.h` lo dice: *«`MDN_ARCH_ARM` NO distingue hard de softfp, y esa
   discrepancia no da error de enlace — da números mal en silencio»*.
2. ⚠️ Lo que la ABI **no** separa es la extensión `A` (atómicos): el P4 la tiene y el C3 no. Un
   `.mdn` del P4 que use atómicos pasaría el gate del C3 y ejecutaría instrucción ilegal. Hoy
   el riesgo es bajo porque el IDE compila para la placa con la que habla, pero un `.mdn` rancio
   en el FS es exactamente [[artefacto-de-otra-familia-se-cuela]].
3. Probarlo en placa con `Bench.bp`, que ya trae su gemelo interpretado como línea base.


##### ✅ `U6.5` — LA FORMA: una gestión de memoria unificada (propuesta, 31-ago) — *hecha en `U6.8`–`U6.11` (`bpvm_mem_plan`, las cinco familias); U6 cerrada el 3-sep*

> Pregunta de Eduardo: *«¿se puede hacer una gestión de memoria unificada, respetando las
> particularidades de cada placa? Y hay que tener en cuenta que una misma placa puede ir con
> PSRAM y sin ella.»*

Sí, y con lo medido hoy se puede describir sin inventar nada. La segunda frase es la que fija la
forma: **si una misma imagen va con PSRAM y sin ella, la placa no puede DECLARAR su memoria —
tiene que ENUMERARLA en marcha.** Hoy eso es un `if` en `pico/main.c`; ahí está la pista de que
la pregunta estaba mal planteada.

### El contrato

La familia contesta **una** pregunta —*«¿qué piezas de memoria tienes ahora mismo?»*— y el común
decide todo lo demás.

```c
typedef struct {
    uint8_t*    base;
    size_t      bytes;
    int         exclusiva;   /* 1 = nadie más asigna aquí (PSRAM) */
    const char* nombre;      /* "PSRAM", "SRAM interna", "trozo 1"… */
} bpvm_mem_region_t;

typedef struct {
    /* Enumera lo que HAY, ahora. Devuelve cuántas. Se llama en el arranque, y
     * por eso la misma imagen sirve con PSRAM y sin ella: el Pico devuelve UNA
     * región (SRAM) o DOS (PSRAM + SRAM) según lo que el env y el sondeo digan. */
    int (*regiones)(bpvm_mem_region_t* out, int max);

    /* Lo que NO es de la VM en la memoria COMPARTIDA: malloc, RTOS, FS, wire.
     * 0 en la exclusiva. Es el único número que la familia tiene que MEDIR. */
    size_t margen_sistema;
} bpvm_mem_ops_t;
```

**Y con eso el común decide**: qué región usar, cuánto tomar, si merece la pena coser dos
adyacentes, cómo repartir entre heap / tabla / pilas, y qué decir en el log.

### Por qué esta es la división y no otra — lo medido

| lo que el censo `U6.0` encontró disperso | dónde queda |
|---|---|
| **dónde** vive el bloque: símbolos del linker / `.bss` / `heap_caps` interna / PSRAM | **la familia**, y es lo único genuinamente suyo |
| **cuánto**: «todo lo que quede» vs «una constante» (`U6.2`) | **el común**: no son dos filosofías, es una regla y dos clases de memoria |
| el **margen**: nombrado en Pico y STM32, medido pero anónimo en S3 y C3 (`U6.0` corregido) | **la familia**, como UN número — y ya está medido: 26 564 B el S3, 17 588 el C3 (`U6.4`) |
| el **techo**: sabido antes / lo comprueba el enlazador / sólo preguntando | **la familia**, dentro del enumerador: devuelve lo que HAY, cada una a su manera |
| el **reparto** heap/pilas | **el común**, y ya lo era (`bpvm_stack_region_bytes`) — desde `U6.1` también en host y simulador |
| la **tabla de handles** | **el común**, y ya estaba bien: proporcional al heap y dentro del bloque |

📌 **La clave está en `exclusiva`.** Es lo que hace que «todo lo que quede» y «una constante»
dejen de ser dos filosofías rivales (`U6.2`):

```
exclusiva  → tómalo todo, menos las reservas CON NOMBRE (display, SQLite)
compartida → tómalo todo, menos `margen_sistema`
```

Una regla. Lo que cambia es **de quién es la memoria**, no cómo se decide.

### Lo que el común hace con las regiones

1. **Elegir.** Prefiere la exclusiva grande si la hay (PSRAM). Si no, la compartida mayor.
2. **Coser, si sale a cuenta.** Dos regiones ADYACENTES se unen con un bloque reservado
   permanente (medido en el C3: hueco de 8 024 B, útil 253 952 contra 131 072 — `V6_IDEAS`). El
   común aplica un umbral: se cose si el hueco es pequeño frente a lo que se gana. **PSRAM y SRAM
   NO se cosen**: están a 250 MB de distancia en el mapa, y además `U6.3` midió que la PSRAM sólo
   cuesta un 4–7 %, así que no hay nada que ganar.
3. **Restar el margen**, y decirlo si no queda sitio.
4. **Repartir** con la regla de siempre.
5. **Contarlo en UNA línea**, con el mismo formato en las cinco placas.

### Lo que esto arregla, en concreto

- **La misma imagen con y sin PSRAM deja de ser un `if`**: es cuántas regiones devuelve el
  enumerador. Y sirve igual para una placa futura con dos bancos, o con PSRAM opcional.
- **Se acaban las constantes a mano** (`160 KB` el S3, `128` el C3, `512` el STM32). Cada una fue
  puesta una vez y revisada nunca: el STM32 arrastró 128 KB con *«~520 KB parados»* durante toda
  una versión (`H13` hallazgo 31), y el C3 estrenó la suya hoy.
- **El margen pasa de comentario a número**, que es lo que `U6.2` identificó como el bloqueo real
  del «cuánto».
- **El bloque reservado queda disponible para lo que venga**, que es lo que Eduardo le veía:
  *«nos da juego, para este micro pero también para otros futuros»*.

### ⚠️ Lo que NO resuelve, y conviene decirlo antes de empezar

- **El STM32 pierde algo si se unifica mal.** Su bloque es `.bss`, así que el techo lo comprueba
  **el enlazador**: si no cabe, no compila. Eso es más fuerte que cualquier comprobación en
  marcha, y un enumerador que devuelva «lo que hay» en runtime lo perdería. Su enumerador debe
  seguir devolviendo el array estático, y el aserto de enlace quedarse donde está.
- **Cuatro comprobaciones tienen que aprender qué es un hueco** antes de que coser sea seguro: el
  barrido del GC, la guarda del PC, el debugger y —si algún día el hueco no está entre heap y
  pilas— la tabla. Están inventariadas en `V6_IDEAS`.
- **Sigue faltando el margen del STM32 y el de la Pico como MEDIDA**, no como estimación. El
  protocolo existe (`tools/medir_margen.ps1`) pero es del wire del ESP32; para las otras dos hay
  que adaptarlo.


##### 🎯 POR QUE U6 IMPORTA MAS ALLA DE HOY (Eduardo, 31-ago)

> *«Lo que estamos haciendo no solamente sirve para ahora, tambien para las placas futuras (C6,
> P4X, S31, etc). En las proximas placas no hara falta construir un gestor de memoria: es ver su
> mapa y activar el gestor generico. Menos trabajo y un codigo que sabemos que funciona.»*

Y hoy se puede poner en numeros. Lo que cuesta una familia ESP nueva **despues de esta semana**:
`chip_cfg.h` (3 constantes), `<chip>_board_id.c` (13 lineas), y `partitions.csv` +
`sdkconfig.defaults` + `CMakeLists.txt`, que son configuracion y no codigo. **La gestion de
memoria era la ultima pieza que seguia siendo artesanal**; con `U6` pasa a ser dos constantes mas
en ese mismo fichero y un enumerador que le pregunta al chip.

📌 **Y hay algo mas que ahorro de trabajo.** De las tres constantes de memoria puestas a mano,
**dos estaban mal**: el STM32 arrastro 128 KB con ~520 KB parados durante una version entera
(`H13` hallazgo 31) y el margen del S3 estaba por 3,3x (`U6.4`). Ninguna se puso mal por
descuido — se pusieron una vez, con criterio, y nunca se volvieron a mirar. Eso es lo que un
numero escrito a mano hace siempre.

El gestor generico no solo ahorra el trabajo: **quita la clase de error**, porque el enumerador
no hereda el numero de la placa anterior — lo pregunta.

##### ✅ `U6.6` — TRES CAPAS: constantes de la imagen, ENV, y enumeración en marcha (Eduardo, 31-ago) — *hecha: `chip_cfg.h`/`board.h` + ENV + enumeración en `bpvm_mem_plan`; U6 cerrada el 3-sep*

> *«Lo mejor es que cada imagen defina unas constantes que le indiquen al gestor de memoria las
> particularidades de cada familia. Y por encima de todo el tema de la PSRAM, que cambia el mapa
> de memoria completamente. O sea que el reparto de memoria tiene que ser dinámico pero que se
> puede orientar un poco con algunos valores estáticos (el firmware) y del environment.»*

Refina `U6.5`, donde la placa sólo ponía «las regiones + un número». Son **tres capas**, y cada
una contesta lo que sólo ella puede saber:

| capa | quién | qué sabe que las otras no | cuándo |
|---|---|---|---|
| **constantes de la imagen** | `chip_cfg.h` | las propiedades del SILICIO y las medidas hechas sobre él | compilación |
| **el ENV** | `bpenv` de la placa | lo que varía de UNA placa a otra con la misma imagen | sin recompilar |
| **enumeración** | el arranque | **lo que de verdad hay ahí ahora mismo** | cada arranque |

📌 **Y el orden de precedencia ya está inventado en este proyecto.** `bpvm_stack_region_bytes`
lo hace con `stack=N` y lleva escrito el criterio: *«Los topes NO son paternalismo… Un valor del
env no puede dejar la placa inútil — se ajusta y SE DICE.»* La enumeración marca lo posible, las
constantes ponen los suelos y la política, el ENV orienta dentro de eso, y **si hay que corregir
un valor del ENV se corrige y se dice**. Generalizar ese patrón a toda la memoria es exactamente
lo que se propone.

### Las constantes, y de dónde sale cada una

`chip_cfg.h` ya existe (`P1.C3.2`) y ya es el sitio de «lo que no se puede preguntar en marcha».

| constante | qué es | hoy |
|---|---|---|
| `CHIP_MARGEN_SISTEMA` | lo que el sistema consume EN MARCHA fuera de la VM | **medido**: 26 564 B (S3), 17 588 B (C3) — `U6.4` |
| `CHIP_VM_MIN` | por debajo de esto la VM no da para nada útil: no se arranca a medias, **se dice** | la Pico ya lo tiene (`VM_SRAM_MIN`, 64 KB) |
| `CHIP_COSER_HUECO_MAX` | hasta qué hueco compensa coser dos regiones adyacentes | medido en el C3: 8 024 B para ganar 122 880 |
| `CHIP_NOMBRE` | ya está | — |

### Las claves del ENV, y por qué esta lista

| clave | para qué | estado |
|---|---|---|
| `psram=0\|1` | **cambia el mapa entero**: la misma imagen con y sin | ✅ existe (Pico) |
| `stack=N` | repartir pilas/heap sin recompilar | ✅ existe (las 3 familias + el sim desde `U6.1`) |
| `SQLite=N` | reserva con nombre dentro de la memoria exclusiva | ✅ existe |
| `vmheap=N` | forzar el tamaño del bloque | ⏳ nuevo — para acotar sin recompilar |
| `coser=0\|1` | **encender y apagar el cosido** | ⏳ nuevo, y no es un lujo (abajo) |

🎯 **`coser=0|1` se pone DESDE EL PRIMER DÍA, y la razón es de hoy.** Toda la medida de la PSRAM
(`U6.3`) fue posible porque `psram=` existía como mando del ENV: la misma imagen, un reset por
medio, **una sola variable**. Sin ese mando habría hecho falta compilar dos firmwares y comparar
binarios distintos, que es justo lo que no desempata. Un mecanismo nuevo que no se pueda apagar
no se puede medir — y éste toca el GC, la guarda del PC y el debugger.

### Cómo queda el arranque

```
1. enumerar        ¿qué regiones hay?          → la familia, en marcha
2. elegir          exclusiva grande, o la compartida mayor
3. coser           si son adyacentes y el hueco < CHIP_COSER_HUECO_MAX  (y `coser` no lo apaga)
4. restar          las reservas con nombre (display, SQLite) o CHIP_MARGEN_SISTEMA
5. comprobar       ¿≥ CHIP_VM_MIN? si no: no hay VM, y el climb LO DICE
6. repartir        la regla común, con `stack=N` si lo hay (ajustando y diciéndolo)
7. contarlo        UNA línea, mismo formato en las cinco placas
```

El paso 1 es lo único que cambia por familia. Los siete pasos son del común.

### Lo que esta forma resuelve del censo

- **«Con PSRAM o sin ella» deja de ser un `if`**: es cuántas regiones devolvió el enumerador.
- **Las constantes a mano desaparecen** (160 el S3, 128 el C3, 512 el STM32) y las que quedan son
  de otra clase: no *«cuánto tomo»* sino *«qué sé de este silicio»*.
- **El margen pasa de comentario a número**, que `U6.2` identificó como el bloqueo del «cuánto».
- **Y todo lo nuevo nace medible**, porque cada palanca tiene su interruptor.


##### ✅ `U6.7` — el margen pasa de comentario a NÚMERO: el bloque de la VM ya no se pide a ciegas (2-sep)

El paso 1 del traspaso, el de menos riesgo. Sólo los dos ESP32 sin PSRAM (S3 y C3); el P4 va
por su rama exclusiva y la Pico y el STM32 ya tenían el margen con nombre.

### Lo que cambia

Antes, `vm_buffer_init()` hacía `malloc(VM_BUFFER_SIZE)` y si fallaba `malloc(VM_BUFFER_FALLBACK)`:
dos números por chip, puestos una vez y revisados nunca. Ahora **mide** y aplica las dos
restricciones que salieron del C3:

```
libre, contiguo  ← heap_caps_get_info            (lo que HAY, ahora)
techo            = min(contiguo, libre − CHIP_MARGEN_SISTEMA)
bloque           = min(CHIP_VM_OBJETIVO, techo)
si bloque < CHIP_VM_MIN → no hay VM, y se dice con los números
malloc(bloque); si falla, −4 KB y otra vez, hasta el suelo
```

Y en `chip_cfg.h` de cada silicio quedan **tres constantes de tres clases distintas**:

| | S3 | C3 | qué es |
|---|---|---|---|
| `CHIP_VM_OBJETIVO` | 160 KB | 128 KB | **política**: cuánto quiere la VM |
| `CHIP_MARGEN_SISTEMA` | 26 564 B | 17 588 B | **medida** (`U6.4`, protocolo escrito) |
| `CHIP_VM_MIN` | 64 KB | 64 KB | **suelo**, el mismo que `VM_SRAM_MIN` de la Pico |

📌 **El objetivo se queda en los 160/128 de siempre A PROPÓSITO.** Cabría más —en el S3 el techo
son 264 KB—, pero Eduardo fue explícito: *«no tocaría el heap del RTOS, la idea es explotarlo más
si es necesario en el futuro; no tiene sentido reducirlo para volver a agrandarlo»*. El objetivo
protege ese heap. Lo que cambia no es cuánto se coge sino que **ya no se coge sin mirar**: si la
realidad es peor que el objetivo, se baja **y se dice por qué** («manda el bloque contiguo» o
«manda el margen del sistema»), en vez de fallar el malloc en silencio o —peor— caber hoy y
ahogar al IDF dentro de un rato.

### Lo que desaparece

- Las **dos constantes a mano** (`VM_BUFFER_SIZE` / `VM_BUFFER_FALLBACK`) en los dos chips.
- La **escalera de respaldo de dos peldaños**, sustituida por un bucle de −4 KB hasta el suelo:
  la misma idea que el P4 en PSRAM (−1 MiB). Hallazgo 1 del censo: dos maneras de «no caber»
  inventadas por separado, ahora una.
- El comentario del S3 que seguía citando los **86 KB** de `#336` como medida: era 3,3× el valor
  real y ya está corregido en el fichero.

### 📐 Predicho antes de medir

Con los números de `U6.4`:

```
C3  libre 280032  contiguo 139264  margen 17588 → techo 139264 (bloque contiguo) → 128 KB = objetivo
S3  libre 338368  contiguo 270336  margen 26564 → techo 270336 (bloque contiguo) → 160 KB = objetivo
```

En las dos manda el **bloque contiguo**, no el margen — coherente con `U6.4`: lo que separa a
estos silicios es la fragmentación, no lo que gasta el sistema. Y en las dos el bloque queda
**igual que hoy**, que es lo que se pretendía: sin cambio de comportamiento, con la comprobación
puesta. La línea de arranque que hay que ver en placa:

```
vm: heap 128 KB reservado (objetivo 128, techo 136 por el bloque contiguo, margen 17588) | ...
```

✅ **Verificado**: S3, C3 y P4 compilan; las cadenas nuevas están **en los `.bin`** y la del
«respaldo» viejo ya no. **Y en el C3, la predicción palabra por palabra** (2-sep, imagen `U6.7`
reflasheada, arranque limpio con `LOG_CLEAR` + `RESET` por el wire):

```
heap: libre 280032 | mayor 139264 | bloques: 7 libres, 37 usados | usado 13424
vm: heap 128 KB reservado (objetivo 128, techo 136 por el bloque contiguo, margen 17588) | DRAM libre 280032->148956 B
```

Misma foto del heap que el 31-ago, y el `INFO` da el mismo reparto de antes (`vmHeapBytes` 65536 /
`vmStackBytes` 65536): **sin cambio de comportamiento, con la comprobación puesta**. El S3 queda
por contrastar cuando se reflashee.


##### ✅ `U6.8` — el planificador común (`bpvm_mem`) y la Pico enumerando: el `if (psram)` desaparece (2-sep)

El paso 2 del traspaso, primera familia. Y con el patrón de `U3.24`: **el arnés antes que el
artefacto** — el planificador nació con un test de host que lo contrasta con los números
MEDIDOS de las cuatro placas, y sólo después se tocó la Pico.

### El común: `include/bpvm_mem.h` + `src/bpvm_mem.c`

Sin dependencias (sólo `<stddef.h>`/`<stdio.h>`), como `bpvm_sqlmem`. La familia describe piezas
y el común decide:

```c
bpvm_mem_region_t  { base, bytes /*contiguo*/, libre /*total de la bolsa*/, exclusiva, nombre }
bpvm_mem_cfg_t     { objetivo, margen, vm_min, reserva_bytes, reserva_nombre }
bpvm_mem_plan()    → { idx, bytes, techo, limita, reserva_bytes }
bpvm_mem_plan_str()→ la MISMA línea de log en las cinco placas
```

La regla es la de `U6.2`: **la exclusiva mayor si la hay, si no la compartida mayor**; en la
exclusiva se aparta la reserva con nombre delante; en la compartida mandan las dos restricciones
(`bloque contiguo` / `libre − margen`) y se dice cuál; nunca más que `objetivo` aunque quepa
(Eduardo: no se reduce el heap del RTOS para volver a agrandarlo); por debajo de `vm_min` no hay
VM y se dice con los números.

### El test: `test/test_mem.c` (`make test-mem`, 14/14)

Cada caso es una placa real con sus números del log:

| caso | entrada | debe salir |
|---|---|---|
| **C3** | contiguo 139 264, libre 280 032, margen 17 588, objetivo 128 KB | 131 072, techo 136 KB por **el bloque contiguo** |
| **S3** | 270 336 / 338 368 / 26 564 / 160 KB | 163 840 — *cabría más y no se coge* |
| **Metro** | SRAM 431 432 **y** PSRAM 8 MB, SQLite 2 MB | elige la PSRAM aunque vaya segunda, aparta 2 MB, **6 MB** |
| **Pico sin PSRAM** | región 431 432, margen 65 536, sin objetivo | **365 896 = los 357 KB del log** |
| suelo | 90 KB con 40 de margen | `NO CABE … suelo 64 KB: sin VM` |

Si alguien toca el planificador y una placa saldría distinta, esto lo dice en el host en un
segundo. Es lo que a los cuatro mecanismos anteriores les faltó durante meses.

### La Pico: enumera, y el `if` se va

`vm_sram_region()` —que decidía cuánto y desde dónde— pasa a ser `pico_mem_regiones()`, que
sólo dice **qué hay**: la SRAM de `align8(end)` a la RAM del pack siempre, y la PSRAM además si
`board_desc()` la tiene. El `if (psram) { … } else { … }` de 50 líneas del arranque se sustituye
por *enumerar → planificar → tomar*. La reserva de SQLite sigue decidiéndola `bpvm_sqlmem`; el
plan sólo la aparta delante. El margen de malloc (64 KB) es ahora `cfg.margen`, y el plan lo
deja **abajo**, pegado a `end`, que es desde donde crece `sbrk` — por eso la SRAM se toma **por
arriba** de la región.

📌 **Detalle que garantiza «los mismos números»**: el `align8` va sobre `end`, no sobre
`end+margen`, para que `base_vm = align8(end) + margen` sea exactamente lo que daba
`vm_sram_region`. El test lo fija en 365 896.

### ✅ Verificado en la Metro, las dos ramas, byte a byte

Grabada por el wire (`BOOTSEL` es verbo propio de la Pico: la placa se abre como disco `RP2350`
y se copia el UF2, sin dedo), arranque limpio (`LOG_CLEAR` + `RESET`) en cada modo:

```
psram=0  vm: 357 KB en SRAM interna (todo lo que deja el margen del sistema)
         vm: SRAM interna 357 KB @ 0x20024ab8 -> heap 267 KB + stacks 89 KB | libre para malloc: 64 KB
         INFO vmHeapBytes 274422 / vmStackBytes 91474
psram=1  vm: 6144 KB en PSRAM (todo lo que deja la región) | SQLite 2048 KB delante
         vm: heap en PSRAM 6 MB @ 0x11200000 (SRAM interna sin reservar)
         bd: reservada (SQLite=2) -> 2048 KB @ 0x11000000
         INFO vmHeapBytes 5767168 / vmStackBytes 524288
```

Las líneas de la familia son **idénticas** a las del 31-ago (misma dirección `0x20024ab8`, mismo
reparto, misma BD delante); la línea nueva es la común. ENV devuelto a `psram=0`.

Y el `.c` nuevo está **de alta en los cinco builds** (host, Pico, S3, C3, P4; los dos STM32 lo
recogen por la carpeta enlazada): compilan los seis. Paridad **38 PASS / 0 FAIL / 0 SKIP**.

⏭️ Siguientes familias, en este orden: **S3 y C3** (su `vm_buffer_init` de `U6.7` *es* la rama
compartida del planificador: enumerador = `heap_caps_get_info`, y la escalera de −4 KB queda en
el «tomar»), luego el **P4** (exclusiva con reserva del display), y el **STM32 el último** — su
enumerador devuelve el array estático y el aserto del enlazador se queda donde está.


##### ✅ `U6.9` — el S3 y el C3 al planificador: `vm_buffer_init` queda en enumerar y tomar (2-sep)

Segunda familia del paso 2, y la más barata: el `vm_buffer_init` de `U6.7` **ya era** la rama
compartida del planificador (techo = min(contiguo, libre − margen), nunca más que el objetivo,
suelo, escalera). Ahora esa regla vive en `src/bpvm_mem.c` —la misma que la Pico usa desde
`U6.8`— y en `esp32/common/main.c` queda sólo lo que es del silicio:

- **enumerar**: `esp32_mem_regiones()` → una región compartida, la DRAM interna, con `bytes` = el
  bloque contiguo mayor y `libre` = el total (`heap_caps_get_info`);
- **tomar**: `heap_caps_malloc` de lo planificado, en KB enteros, con la escalera de −4 KB hasta
  el suelo si el asignador no sirve lo que sus contadores prometían.

Las tres constantes de `chip_cfg.h` (`CHIP_VM_OBJETIVO` / `CHIP_MARGEN_SISTEMA` / `CHIP_VM_MIN`)
pasan tal cual a `bpvm_mem_cfg_t`. El P4 no cambia: su PSRAM es exclusiva y va por su `main.c`.

### ✅ Verificado en el C3 — los mismos números que `U6.7`, con la línea común

```
heap: libre 280032 | mayor 139264 | bloques: 7 libres, 37 usados | usado 13424
vm: 128 KB en SRAM interna (objetivo 128, techo 136 por el bloque contiguo)
vm: DRAM interna libre 280032->148956 B (bloque mayor 139264->114688 B) | margen 17588
INFO vmHeapBytes 65536 / vmStackBytes 65536
```

Es exactamente lo que `test_mem` fija para el caso C3, y lo que `U6.7` había verificado ayer con
su propia aritmética: **sin cambio de comportamiento, y una regla menos** (la copia de la rama
compartida que vivía aquí). S3, C3 y P4 compilan.

### ✅ Y en el S3 (reflasheado el 2-sep), con una sorpresa buena

```
heap: libre 346064 | mayor 278528 | bloques: 7 libres, 42 usados | usado 17756
vm: 160 KB en SRAM interna (objetivo 160, techo 272 por el bloque contiguo)
vm: DRAM interna libre 346064->182220 B (bloque mayor 278528->120832 B) | margen 26564
INFO vmHeapBytes 98304 / vmStackBytes 65536
```

La **estructura** de la predicción de `U6.7` se cumple exacta —manda el bloque contiguo, 160 KB =
objetivo, reparto 96/64 sin moverse— pero el **nivel** subió respecto a la imagen del 30-ago con la
que se midió `U6.4`: **+7 696 B libres (338 368 → 346 064) y +8 192 B contiguos (270 336 →
278 528)**, techo 272 KB en vez de 264. Es la RAM que la unificación liberó en el S3 —`U3.23` quitó
el `dir_snapshot_t` de 6 KB, `#461` el resto— **medida por primera vez en su placa**. Y el S3
también tiene el espacio libre en **7 trozos**, partidos por 42 reservas del IDF (17 756 B): la
misma forma que el C3, con más reservas por los dos núcleos.

📌 El fixture del S3 en `test_mem` conserva los números del 30-ago a propósito: son medidos y
válidos, y el planificador no depende del nivel — cuando se remida el margen con la imagen nueva
(`tools/medir_margen.ps1`) se añade el caso, no se sustituye.

📌 Con esto, de las cinco placas **tres deciden su memoria con la misma función** (Pico, S3, C3)
y las otras dos (P4, STM32) tienen su hueco descrito en `U6.8`.


##### ✅ `U6.10` — el margen es de la REGIÓN, y el P4 al planificador: la familia ESP32 entera con la misma regla (2-sep)

Al ir a meter el P4 salió que el modelo de `U6.8` tenía el margen en el sitio equivocado. El P4
tiene **dos** cosas distintas en su PSRAM: una **reserva** (SQLite: asignada aparte, alineada a
página por el sello del pack) y un **margen** (los 4 MiB que se dejan *libres* para que LVGL los
pida después). Mi planificador sólo entendía el margen en la memoria compartida.

📌 **Lo correcto es que el margen sea de la región** — *lo que en ESA memoria se deja a los
demás inquilinos*: `malloc`/RTOS en la SRAM (medido), el display en la PSRAM del P4, **nada** en
la PSRAM de la Metro. La Metro lo demuestra sola: deja 64 KB en su SRAM y 0 en su PSRAM, y con
un margen global eso no se puede expresar. Un commit de antigüedad: mejor ahora que arrastrarlo.

```c
bpvm_mem_region_t { base, bytes, libre, exclusiva, margen, nombre }   /* margen: aquí */
bpvm_mem_cfg_t    { objetivo, vm_min, reserva_bytes, reserva_nombre } /* ya sin margen */
```

La regla queda: **exclusiva** → todo, menos la reserva con nombre delante y el margen de la
región; **compartida** → todo lo contiguo, menos el margen de la región (y se dice cuál de las
dos restricciones manda).

### `test-mem`: 16/16, con la forma del P4

Los cinco casos medidos siguen dando lo mismo (el margen viajó a las regiones sin cambiar ningún
número), y entra un sexto: **exclusiva con margen** — 30 MiB con 4 MiB para el display → 26 MiB,
y la línea lo dice (*«todo lo que deja el margen de la región»*). Números redondos a propósito:
**el fixture medido se añade cuando el P4 dé los suyos**; los medidos se suman, no se sustituyen.

### El P4, en código

`vm_buffer_init_psram()` pasa a enumerar (`p4_mem_regiones`: la PSRAM, exclusiva, `bytes` = el
bloque contiguo mayor, `margen` = `VM_PSRAM_DISPLAY_RESERVE`) → planificar (objetivo 0, suelo
`VM_MEM_MIN`) → tomar (alineado a página, con la escalera de −1 MiB si el asignador discrepa).
SQLite **no** cambia: se asigna antes y aparte (`vm_sqlite_init_psram`), y el enumerador ya la ve
gastada.

⚠️ **Un cambio de comportamiento posible, y a favor**: antes el bloque salía de la PSRAM *total*
libre menos el display, y la escalera de −1 MiB corregía si el mayor bloque no llegaba. Ahora
sale del **contiguo** menos el display, así que la escalera casi nunca tendrá que bajar — pero si
la PSRAM del P4 arranca fragmentada, el bloque puede ser algo menor que antes. **Por eso hace
falta la placa**: para ver su `psram: libre | mayor` y fijar el fixture con lo medido.

### ✅ Verificado hoy, sin el P4

- **S3 reflasheado con el refactor**: las mismas líneas que `U6.9` — `160 KB … techo 272 por el
  bloque contiguo`, DRAM 346 064→182 220, reparto 96/64. El margen cambió de sitio y no de valor.
- Los **cinco builds** compilan (host, Pico, S3, C3, P4; los dos STM32 lo recogen solos).
- **Limpieza del S3** (pedida por Eduardo): `DEL /app/Core.mod` y `DEL /lib/Pico.mod` por el wire;
  al rearrancar, la stdlib embebida reinstala **el `Pico.mod` de la imagen** (2 070 B —el mismo
  tamaño que el de V5, por eso el `LIST` engaña; ⚠️ *corregido el 2-sep*: aquí NO pudo haber
  aviso —el ESP32 perdió el de `#422` al regenerar los blobs, ver `#466`—,
  y `bpstdlib/Pico.mod` mide 2 070). 27 → 26 ficheros.

### 🐛 Y el P4, en la mesa, cazó un fallo del modelo — con números

Referencia, con su imagen del 31-ago: `PSRAM libre 32765 KB`, `vm: heap 28668 KB en PSRAM
@0x48000a7c (PSRAM libre 4093 KB)`. Con la primera versión del planificador:

```
psram: libre 32765 KB | mayor 32256 KB | bloques: 1 libres, 3 usados
vm: 28160 KB en PSRAM (todo lo que deja el margen de la región)        ← 508 KiB MENOS
```

La rama exclusiva restaba el margen del display al **bloque contiguo** (32256 − 4096) y la imagen
vieja lo restaba al **total** (32765 − 4096). Y la vieja tenía razón: los 4 MiB del display los
pide LVGL en trozos, no necesitan ser contiguos con nada — el margen va contra el *total*, y lo
contiguo es sólo el tope de lo que un `malloc` puede dar. **Que es exactamente la forma de la
rama compartida.** Así que las dos ramas se funden en una sola cuenta:

```
techo = min( contiguo − reserva ,  libre − reserva − margen )
```

Lo único que cambia entre clases de memoria es **qué inquilinos hay**, no cómo se decide. El
caso P4 entra en `test_mem` con sus números medidos (**19/19**), y los seis anteriores no se
mueven.

### ✅ Verificado en el P4, byte a byte con su imagen anterior

```
psram: libre 32765 KB | mayor 32256 KB | bloques: 1 libres, 3 usados
vm: 28669 KB en PSRAM (todo lo que deja el margen de la región)
vm: heap 28668 KB en PSRAM @0x48000a7c (PSRAM libre 4093 KB) | DRAM interna libre 430315 B
INFO vmHeapBytes 28831744 / vmStackBytes 524288
```

Mismo bloque (28 668 KiB tras alinear a página), misma dirección, mismos 4093 KB libres. Los
**seis builds** compilan con la regla final; la Pico, el S3 y el C3 no se han vuelto a grabar con
ella pero sus fixtures están en el test y no se mueven — y sus enumeradores no han cambiado.

📌 **Con esto la familia ESP32 entera —S3, C3 y P4— y la Pico deciden su memoria con la misma
función.** Queda el STM32, que es el que devuelve un array estático y conserva el aserto del
enlazador.


##### ✅ `#466` — `/lib` se queda RANCIO: el instalador de la stdlib sólo copia «si falta» (abierta 2-sep · **CERRADA 3-sep** · `1a7c1924` `f1a9c23b` `0e1a425b` + la Pico 2)

Salió en el repaso de la Metro con la imagen final. La memoria, byte a byte; pero el arranque:

```
lib: /lib/I2c.mod NO es el de esta imagen (4153 B en FS, 3276 embebido) - ¿rancio de otro firmware, o subido por ti?
lib: /lib/Math.mod NO es el de esta imagen (2320 B en FS, 1708 embebido) …
      … y Gpio, Spi, Uart, Pulse, Pwm, Rtc, Adc, Wdt, Timer, IO, Neopixel — 13 módulos — y /app/Hello.mod
```

**Qué pasa.** El instalador (`esp32_mods_install` y su gemelo de la Pico) copia la stdlib embebida
a `/lib` **sólo si el fichero no existe**. Una placa que pobló `/lib` con una imagen vieja —o con
el IDE subiendo el dist de V5— se queda con esa stdlib **para siempre**, imagen tras imagen, y el
aviso es la única señal. Es [[stdlib-mod-version-skew-oo-device]] en su forma más silenciosa: el
skew no es entre repo y dist, es entre **la imagen y su propio `/lib`**.

**Consecuencia práctica, hoy**: un programa que importe `Math` en esta Metro recibe el `Math`
**sin** `clamp`/`wrap`/`hypot`/`remap`; el `Rtc`, el `Wdt`, el `IO` que reciben son los de hace
semanas. `MathRango.bp` fallaría en ella y funcionaría en el host: **rompe el invariante**.

**Por qué no se vio antes**: en el S3 apareció con UN módulo (`Pico.mod`, `U6.9`) y se arregló a
mano; en la Metro son 13 porque su `/lib` es de antes de que la stdlib se recompilara con el
compilador nuevo. Y mi filtro del log de esta mañana (`psram|vm|bd`) me lo escondió dos veces.

**Y la Pico 2 sola, revisada después, tiene 16**: los 13 de V5, **más `/lib/Core.mod` de V5**
(12 999 B contra 13 111 embebidos — el IDE también sube `Core` a `/lib` desde `#463`, y sube el de
V5), más un `/lib/Pico.mod` de **2 947 B** que no es ni de V5 ni del repo: es el embebido de una
imagen de finales de agosto. Tres generaciones en el mismo `/lib`. La Pico 2 va además con
`stack=90` en el ENV, que el planificador respeta (pilas 90 KB en vez de 89): el mando del ENV
sigue mandando sobre la regla, que es lo que `U6.6` pedía.

**Los tamaños dan la CAUSA, no una pista.** Comparados los 13 con el dist de V5 y con el repo:

| módulo | en el FS de la Metro | `BasicPlus-5.0-win/bpstdlib` | repo |
|---|---|---|---|
| I2c | 4153 | **4153** | 3276 |
| Math | 2320 | **2320** | 1708 |
| Gpio | 4031 | **4031** | 4035 |
| … los 13 | = | **= byte a byte** | ≠ |

**Todos coinciden con el dist de V5 y ninguno con el repo.** Quien pone la stdlib rancia en `/lib`
no es una imagen vieja: es **el IDE de V5** (`C:\temp\BasicPlus-5.0-win`, el que Eduardo usa
contra el firmware de V6) subiendo *su* stdlib como dependencias — y como compara CRC y sube
«si difiere», **corrige la placa hacia V5**. `#463` sólo movió `Core.mod` de `/app` a `/lib`; el
resto de la stdlib siguió subiéndose igual. Y el instalador de la imagen, que sólo copia «si
falta», no lo deshace nunca.

📌 Es justo lo que `CLAUDE.md` dice que NO debe pasar: *«Módulos stdlib pre-instalados en el
device (vía `stdlibDir`); módulos de la app al workdir en cada Run. El IDE NO retransmite la
stdlib en cada ejecución.»* La retransmite, y encima la de otra versión.

### La forma del arreglo (por decidir con Eduardo)

- ✅ **A mano, hoy (2-sep, Metro)**: los 14 borrados por el wire y rearrancada; el instalador repuso
  los 14 con los tamaños de la imagen (`preinstall: /lib/Math.mod (1708 bytes)`…), **0 avisos** y el
  `LIST` coincide 14/14 con lo embebido. El FS bajó de 172 032 a 163 840 B: las copias de V5 eran
  más grandes. Igual que en el S3. Y la **Pico 2** después: 16 borrados, 16 repuestos, 0 avisos,
  16/16 tamaños. **No escala**: hay que acordarse, placa a placa — y en cuanto el IDE de V5 vuelva a
  subir dependencias, vuelve el problema. Por eso el arreglo de raíz sigue abierto. Placas limpias
  hoy: S3, Metro, Pico 2 **y P4** (repasado al retomar la 2ª pausa: su `/lib` ya estaba limpio —
  15/15 tamaños iguales a los embebidos, 0 avisos—; sólo sobraba el `/app/Core.mod`, borrado con
  su OK: 22 → 21 ficheros, y tras el reset los mismos números de memoria, 28 668 KiB @0x48000a7c).

📌 **Y el C3 confirma la causa por el lado contrario**: su `/lib` está **limpio de origen** — los
14 módulos coinciden con los embebidos y no hay ni un aviso — porque lo aprovisionó el instalador
de la imagen el 31-ago y **el IDE de V5 nunca le ha subido nada**. Las placas que pasaron por el
IDE (Metro, Pico 2, S3) estaban sucias; la que no, no.

⚠️ **Matiz que trae el P4** (al retomar): también pasó por el IDE —en su `/app` está lo que el
IDE sube en cada Run: `JsonDemo.mod`, `Json.mod`, `Core.mod`— y sin embargo su `/lib` estaba
limpio, 15/15. Luego «el IDE de V5 ensucia `/lib`» no es *siempre*: o las dependencias van a
`/app` (el workdir) y lo rancio de la Metro y la Pico 2 lo dejó **una imagen anterior** cuyo
instalador pobló `/lib` con los blobs de V5 (y el «sólo si falta» los conservó), o depende de
algo que no he mirado (el `crc:-1` del `LIST`, qué acción del IDE). Se contesta en el código del
IDE y del instalador —a dónde escribe cada uno—, no adivinando; es el primer paso del arreglo de
raíz. Lo que no cambia: el `/lib` rancio existe y ninguna placa lo repone.

### 🔍 El mecanismo, leído en el código (2-sep, al retomar) — ya no hay que adivinar

**Quién escribe en `/lib`: el IDE, en cada Run, y a propósito.** `FrmMain` resuelve las
dependencias del programa y clasifica como «stdlib core» las que están en `EMBEDDED_CORE_MODS`
(Math, IO, Gpio, I2c, Spi, Uart, Pulse, Pwm, Pico, Rtc, Adc, Wdt, Timer; `Core` entró en `#463`)
más Gui: ésas van a `/lib/<X>.mod`; el resto (Json, Collections, Str…) a `/app/<proj>`. Y
`PicoExplorer.putIfChanged` las sube **si el CRC del device no coincide con el local** (DEL +
PUT). El mecanismo es el mismo en el tag `v5.0` y en el repo de hoy. La razón está escrita al
lado (13-jun): un blob embebido de un frontend ANTERIOR no casaba en las vtables con la app
recién compilada → `INVOKE_VIRTUAL slot N no resoluble`; desde entonces `/lib` pasa por el mismo
content-check que la app, «auto-curando blobs embebidos rancios sin reflashear».

Eso explica **la matriz entera**: la Metro y la Pico 2 corrieron samples de hardware desde el IDE
de V5 → sus deps (I2c, Spi, Uart, Rtc…) aterrizaron en `/lib` con los blobs de V5, y el instalador
(«sólo si falta») los conservó imagen tras imagen. El P4 corrió `JsonDemo` con el IDE de V5, cuyas
deps (Json, y entonces Core) no eran «core» → a `/app`; su `/lib` no lo tocó nadie desde que su FS
se repobló. El C3 nunca pasó por el IDE. **Dos escritores de `/lib` con criterios opuestos**: la
imagen dice «`/lib` es lo embebido» y el IDE dice «`/lib` es contra lo que compiló la app». Quien
escribe el último, gana — y ninguno de los dos se entera del otro.

**Quién avisa: sólo la Pico.** Los tres instaladores copian si falta (`pico/main.c`,
`esp32_mods.c`, `stm32_mods.c`), pero el chivato de `#422` («NO es el de esta imagen», tamaño y
luego CRC) hoy sólo existe en `pico/main.c`. 🐛 **En el ESP32 lo hubo y se PERDIÓ**: `#422`
(16-ago) lo añadió editando a mano `esp32_mods.c`, que es GENERADO, y dos días después `#446`
(18-ago) regeneró los blobs desde `regen_esp32_mods.sh` —que nunca lo tuvo— y se lo llevó por
delante sin que nadie lo viera (`git log -S` lo cuenta en un segundo; [[generado-parcheado-a-mano]]
en carne propia). El STM32 nunca lo tuvo. **Corrección** a lo dicho más arriba y en `U6.10`: el S3
no pudo avisar de su `Pico.mod` de V5 — en las ESP32 y las STM32 el `/lib` rancio es hoy MUDO.

### La decisión (Eduardo) — con recomendación

- **A (recomendada)**: el IDE **deja de escribir en `/lib`**: las deps de stdlib contra las que
  compiló van a `/app/<proj>` como cualquier otra dep. Conserva la protección del 13-jun (la app
  corre contra lo que compiló, porque `/app` gana a `/lib` — la precedencia que fijó Eduardo el
  31-ago) y cumple `CLAUDE.md` («el IDE NO retransmite la stdlib»). Y el instalador **refresca
  `/lib` cuando no coincide con lo embebido** (tamaño, luego CRC), en un helper COMÚN de `src/`
  que llaman los tres generados — no dentro de los generados, que es donde `#422` murió. Coste:
  módulos de stdlib duplicados bajo `/app` por proyecto (Gui, 44 KB, es el grande; el FS lo
  aguanta: P4 7 MB, Pico 1 MB al 24 %).
- **B**: el IDE sigue en `/lib` y el instalador refresca → se pelean: cada arranque repone, cada
  Run vuelve a subir (~50 KB por el wire). No.
- **C**: dejarlo y sólo avisar (reponer `#422` en las tres, en el helper común) → el invariante
  sigue rompiéndose, ahora con ruido. Es el mínimo si A se aplaza, y A lo incluye.

### ✅ Decidido: LA REGLA de Eduardo (2-sep) — *«las cosas complicadas o no funcionan o funcionan de formas extrañas»*

Yo iba a montar más de lo necesario (duplicar stdlib bajo `/app/<proj>`, un verbo nuevo). Eduardo
lo dejó en dos frases, y **la versión de la stdlib la resuelve el SO, no la comunicación**:

- **SO (la imagen, al arrancar):** por cada módulo embebido, si en `/lib` **falta**, o el que hay es
  de **versión anterior**, o de la **misma versión con CRC distinto**, se reemplaza. Si el de `/lib`
  es más nuevo que el embebido, se deja. `/app` no se toca nunca (es del usuario; el `Hello.mod`
  de muestra de la Pico sigue siendo «sólo si falta»).
- **Comunicación, el principal:** se sube si no está o su CRC es distinto (como hoy).
- **Comunicación, las dependencias:** por cada una **se pregunta al micro si ya la tiene, donde
  sea**; si la tiene, no se sube — salvo que la suya sea de versión anterior, o de la misma versión
  con CRC distinto. Si el micro la tiene más nueva, no se sube.

**Con lo que hay, sin inventar** (los módulos del FS no tienen fecha: tienen **versión y CRC**):

- La **versión** es el MAGIC del formato (`MOD6` < `MOD7`), que ya discrimina el caso real: la
  stdlib del dist de V5 es `MOD6` (Math 2320 B, I2c 4153 B) y la del repo `MOD7` (1708, 3276).
- La **pregunta al micro** ya existe a medias: `STAT` con `crc:true` (`fileCrc` en el IDE) da
  tamaño y CRC — por ruta. Le faltan dos cosas aditivas: aceptar un **`name` de módulo** y
  resolverlo con **el resolvedor del propio device** (`bpvm_entry_resolve`: proyecto → literal →
  `/app` → `/lib` → `/sys`, así el IDE no lleva un gemelo del orden de búsqueda, que es como se
  desincronizó `#463`), devolviendo la **ruta** donde está y su **`magic`** (4 bytes leídos con
  `bpvm_fs_read_at`). Verbo nuevo, ninguno.
- **Destino** de una dependencia que sí se sube: donde el micro la tenía; si no la tenía, `/lib`
  si es de la stdlib y `/app/<proj>` si es de la app. La lista `EMBEDDED_CORE_MODS` desaparece.
- **El instalador** de las tres imágenes aplica la regla del SO desde un **helper común de `src/`**
  (la fachada `bpvm_fs_stat/read_at/crc32/write` es la misma en las tres familias) que llaman los
  generados; los generadores emiten la llamada. Lo que iba dentro de un generado se lo llevó la
  siguiente regeneración (`#422` en el ESP32): no se repite.

Orden de trabajo, cada paso con su verificación: (1) el device — `STAT` por nombre + `magic`,
protocolo documentado, `sim_smoke`; (2) el helper del instalador — test en host, generadores,
tabla de la Pico, seis builds; (3) el IDE — contra el simulador, fat-jar con el IDE cerrado; (4)
las placas: Metro y Pico 2, que son las que tienen `/lib` con historia.

### 🛠️ Hecho al retomar (2-sep): los pasos (1) y (2), verificados en host y en los seis builds

- **(1) el device pregunta-y-contesta**: `STAT` acepta `name` (+`base` opcional) y lo resuelve
  `bpvm_entry_resolve` —el del RUN—; la respuesta trae `path` y `magic` (también en el STAT por
  ruta). Documentado en `BPVM_WIRE_PROTOCOL.md`; `sim_smoke` **45/45** (5 checks nuevos: lo
  encuentra en `/lib`, el proyecto primero con `base`, NOT_FOUND, sin `magic` para lo que no es
  módulo). Commit `1a7c1924`.
- **(2) la regla del SO, una vez**: `src/bpvm_mods.c` (`bpvm_mods_sincronizar`): falta → instala;
  versión anterior, o misma versión con tamaño/CRC distinto → repone y lo dice; más nuevo → se
  deja y lo dice; fuera de `/lib` → sólo si falta. Lee por la fachada (`stat/read_at/crc32`) y
  escribe por el `put` de la familia (el ESP32 agrupa las escrituras). `make test-mods` **16/16**
  sobre un littlefs real en fichero, con los casos que las placas han tenido de verdad (el MOD6
  de V5 bajo imagen MOD7, el recompilado con el mismo MOD7, basura, el Hello del usuario). Los
  dos generadores emiten la llamada —los generados sólo cambian en la cola: **los blobs salieron
  idénticos**, la stdlib embebida no se ha movido— y la tabla de la Pico llama al mismo helper;
  el chivato de `#422` desaparece porque ya no hace falta: reflashear REPONE.
- **Seis builds verdes** con el `.c` nuevo dado de alta en los cinco sitios: host (`make`,
  `test-mem`, `sim-smoke`), Pico (`uf2` de las 19:27), S3/C3/P4 (`Project build complete`, 19:28),
  Nucleo (text 250 200 → 251 248: el helper) y Discovery (0 avisos).

### ✅ (3) el IDE pregunta a la placa (2-sep) — y de paso, un CRC que llevaba meses NEGATIVO

- `BpvmClient.statModule(name, base)`: el STAT por nombre; devuelve `ModStat{path, size, crc,
  magic}` con `version()`. `null` = no lo tiene, o firmware anterior a `#466` (INVALID_PARAM): se
  sube, como siempre.
- `PicoExplorer.uploadAndRun`: por cada dependencia pregunta, y aplica la regla: no lo tiene →
  sube a donde le toca (`/lib` si es de la stdlib, `/app/<proj>` si es de la app); lo tiene MÁS
  NUEVO → no sube y lo dice; anterior, o misma versión con CRC distinto → sube DONDE la placa lo
  tenía. Ya no «corrige» `/lib` por CRC ni consulta listas.
- `FrmMain`: fuera `EMBEDDED_CORE_MODS` (los dos caminos, Run y Debug, y el filtro de
  `resolveDeviceDeps`, que ya miraba `stdlibDir`); «de la stdlib» = `stdlibDepNames`: vive en
  `stdlibDir`. Gui y Json entran solos, sin excepciones a mano.
- Verificado con el cliente REAL contra el simulador: `StatModuleSmoke` (Java, 5/5: `/lib`, el
  proyecto primero con `base`, el crc, `null`, `versionOf`) y `sim_smoke` 45/45. Fat-jar
  reconstruido con el IDE cerrado.

🐛 **Lo que cazó el smoke Java: el CRC del wire salía NEGATIVO.** `long` es de 32 bits en las
placas y en MinGW, y `(long) crc` con el bit 31 puesto —la mitad de los ficheros— viajaba como
`-1671101502`; el IDE compara contra `java.util.zip.CRC32`, que es sin signo, así que la mitad de
los «idénticos» le parecían distintos y **los volvía a subir**. Llevaba ahí desde que el CRC entró
en el wire (`#398` y antes). Arreglo en los dos lados: el device emite el CRC sin signo
(`wire_v1_field_ulong`; el protocolo lo dice) y el IDE normaliza lo que venga con signo de un
firmware anterior (`crcSinSigno`). El smoke Python lo comprueba ahora con el VALOR, no con
«distinto de 0» — que es por donde se coló: una medida que no desempata no es una medida
([[medida-ambigua-no-desempata]]).

### ✅ (4a) En la Pico 2 (3-sep): el SO repone solo, y sólo lo que toca

Como su `/lib` estaba limpio, se **forzó el caso** (un camino ejecutado no es un camino probado):
por el wire se plantó el `Math.mod` del dist de V5 (MOD6, 2320 B) en `/lib/Math.mod`, y se grabó
el `uf2` nuevo por `BOOTSEL`. El arranque:

```
[  150] lib: /lib/Math.mod repuesto: versión anterior (MOD6 2320 B -> MOD7 1708 B)
[  185] fs: 32 ficheros, 253952/1048576 bytes usados
```

**Una línea**: los otros 14 embebidos, idénticos, callan; el `/app/Hello.mod` del usuario no se
toca; el número de ficheros y los bytes no cambian. Y el `STAT` por nombre desde el wire:
`{"path":"/lib/Math.mod","size":1708,"magic":"MOD7","crc":3686083642}` — el CRC **sin signo, con
el bit 31 puesto**, igual al `zlib.crc32` del `bpstdlib/Math.mod` del repo. Control: la imagen
anterior contestaba `INVALID_PARAM falta path` a la misma pregunta (el camino «firmware viejo»
del IDE).

### ✅ (4b) El IDE en placa (3-sep, Eduardo, desde `BpIde-6.0.jar` sobre la Pico 2)

```
[deps] 1 módulo(s) a subir:
  - Math.mod (stdlib: se le pregunta a la placa)
[Explorer] Math.mod: ya en la placa, idéntico — no se sube
-- clamp(x, lo, hi) --
dentro    5
…
FIN
[Explorer] VM finished: exit 0 (OK)
```

Las 29 líneas de `MathRango` iguales al oráculo del host, y **ni un byte de stdlib por el wire**.
Antes de esto, con el jar 6.0, salieron 25 errores de compilación — que no eran de `#466`: eran
`#467`, el compilador tapado por la stdlib de V5 de `samples/out`. Con eso, **cerrada**: el SO
repone `/lib` (4a) y el IDE pregunta y no retransmite (4b). La Metro y el resto de placas reciben
lo mismo al regrabarse; no hace falta repetir la prueba en cada una.

### ✅ Y el S3 (3-sep), sin plantar nada: el caso real, 13 de golpe

Al leerlo antes de regrabar, su `/lib` era **todo de V5** menos el `Pico.mod` repuesto a mano el
2-sep: 13 módulos MOD6 con los tamaños exactos del dist (`Core` 12999, `Math` 2320, `I2c` 4153…).
⚠️ Corrección a lo dicho el 2-sep: **el S3 no estaba limpio, estaba mudo** — su firmware no tenía
el chivato de `#422` (se perdió en `#446`), así que quité los dos que se veían por el `LIST` y di
por buenos los otros doce. El instrumento sin control, otra vez.

Primer arranque de la imagen nueva (`idf.py flash`, 19:01):

```
[  483] lib: /lib/Core.mod repuesto: versión anterior (MOD6 12999 B -> MOD7 13111 B)
[  508] lib: /lib/Math.mod repuesto: versión anterior (MOD6 2320 B -> MOD7 1708 B)
   … (13 líneas, una por módulo rancio; Pico.mod, ya el bueno, calla)
[  827] REPL entry (wire v1)
```

y el siguiente arranque, silencio: `fs: 26 ficheros, 299008 bytes` (antes 307200: las copias de
V5 eran más grandes). Un solo guardado de la partición para las 13 escrituras (el lote del ESP32).
Los números de memoria, idénticos a los de `U6.9`/`U6.10` (`160 KB`, techo 272, margen 26564); el
`STAT` por nombre, igual que en la Pico. **Regrabar ya refresca el `/lib`: era lo que `#422`
prometía y no cumplía.**
- **De raíz, y son DOS lados**: (1) el **IDE no debe subir stdlib a `/lib`** — la placa ya la
  tiene, y la suya es la buena para su imagen; sólo módulos de la app, a `/app`. (2) El
  **instalador debe refrescar `/lib` cuando no coincide con lo embebido**, en vez de avisar, para
  que una placa que ya esté rancia se cure sola en el primer arranque de la imagen nueva. La duda del aviso —*«¿rancio de otro firmware, o subido por ti?»*— la resolvió Eduardo
  el 31-ago al fijar la precedencia: **los overrides del usuario van a `/app`, que gana a `/lib`**.
  Luego un `/lib` distinto del embebido es siempre rancio, y se puede reponer sin miedo. El coste
  es un `PUT` de cada módulo desactualizado en el primer arranque de una imagen nueva (~50 KB una
  vez), y se dice en el log: *«lib: X.mod actualizado (4153 → 3276 B)»*.

⚠️ Y una segunda cosa del mismo log: **`boot: causa del reset = WATCHDOG` sale también tras un
`RESET` por el wire** (la Pico reinicia vía watchdog). El log no distingue un cuelgue de un
reinicio pedido — la misma clase que [[aviso-que-no-distingue-no-evento-de-fallo]]. Menor, pero
anotado.


##### ✅ `U6.11` — el STM32 al planificador: LAS CINCO FAMILIAS deciden su memoria con la misma función (2-sep)

La última del paso 2 de `U6`, y la más simple: la memoria de la VM en el STM32 es un **array
estático de 512 KB** (`s_vm_mem`, `stm32_repl.c`), así que no hay nada que medir en marcha. El
enumerador ofrece **una región**: el array entero, **exclusiva** (nadie más asigna en él) y **sin
margen** — lo que se deja a `malloc` y a la pila del MSP no se lo deja la VM: lo exige el
**enlazador** (`._user_heap_stack`: `_Min_Heap_Size + _Min_Stack_Size` detrás del estático, o no
enlaza), que es más fuerte que cualquier comprobación en marcha. Objetivo 0, suelo 64 KB.

```
vm: 512 KB en SRAM estática (todo lo que deja la región)
```

Es la misma línea que en las otras cuatro placas, al abrir el boot (tras `log:`), y el número es
el de siempre: 512 KB → 384 de heap + 128 de pilas por la regla común. Lo que cambia:

- **INFO y RUN van sobre lo PLANIFICADO** (`s_vm_bytes`), no sobre el `sizeof`: hoy coinciden, y
  si un día dejan de coincidir el INFO dirá la verdad y el RUN sin plan no arranca a medias
  (`run: sin memoria planificada para la VM — no se ejecuta`).
- `test_mem` fija el caso (**22/22**): 512 KB enteros, manda «la región», y la línea.

### ✅ Verificado sin placa (lo que se puede sin ella)

Los dos proyectos CubeIDE compilan **headless** con `cleanBuild` (receta de [[build-stm32-headless]]):
Nucleo `0 errors` (3 avisos preexistentes de `builtins.c`, funciones sin usar), Discovery
`0 errors, 0 warnings`; los `.elf` son de ahora y llevan las cadenas nuevas dentro (`strings`).
Tamaños: Nucleo text 250 200 / bss 624 354; Discovery text 890 908 / bss 1 573 548 — el bss no
se mueve, el array es el mismo.

⚠️ **Gotcha del headless** que me costó dos builds vacíos: `-data`/`-importAll` con `C:/…`
(barras) hace que Eclipse lea `C` como ESQUEMA de URI («No file system is defined for scheme:
C») y no importa nada — y el launcher sale con 0. Con `cygpath -w` (barras invertidas) va. Y el
`.elf` de las 14:24 seguía ahí para engañar: verificar el ARTEFACTO, no el exit code.

### ✅ En la Nucleo (3-sep, grabada por Eduardo desde CubeIDE, sello 19:08)

```
[    4] vm: 512 KB en SRAM estática (todo lo que deja la región)
INFO: vmHeapBytes 393216 / vmStackBytes 131072 · sramBytes 786432 · boardName nucleo-u575zi
```

La línea de las cinco placas, y el reparto de siempre (384 + 128). Y de `#466`: su `/lib` estaba
**vacío** (el log viejo resolvía `Core` y `Json` desde `/app`, «eclipsa al del pack») y el primer
arranque instaló los 14 (`preinstall: …`); en `/app` queda un `Json.mod` MOD6 (24055 B) que la
imagen no embebe, así que es el IDE quien lo repondrá en el próximo Run que lo necesite (versión
anterior → sube donde está).

### ✅ Y la Discovery (3-sep, sello 19:12): la quinta familia, en placa

```
[   11] vm: 512 KB en SRAM estática (todo lo que deja la región)
INFO: vmHeapBytes 393216 / vmStackBytes 131072 · sramBytes 3080192 · boardName u5g9j-dk2
```

Su `/lib` estaba vacío también: 14 `preinstall:`; `STAT` por nombre igual que en las otras cuatro
(`/lib/Math.mod`, MOD7, crc 3686083642). **Con esto las cinco familias —Pico/Metro, S3, C3, P4,
Nucleo y Discovery— deciden su memoria con la misma función Y lo han demostrado en su placa.
`U6` cerrado.**

##### ✅ `U6.12` — la Discovery deja de heredar el techo de la Nucleo: 1536 KB (3-sep, observación de Eduardo)

*«La Discovery tiene más de 512 K de RAM»* — tiene **3008 KB**, y el port del STM32 usaba un único
array de 512 KB para las dos placas porque «manda la Nucleo». Con `U6` cada imagen pone sus
constantes, así que ya no hay razón: **`BOARD_VM_BYTES` en `board.h`**, por placa, y el array la
toma (`s_vm_mem[BOARD_VM_BYTES]`). Medido antes de elegir el número (`arm-none-eabi-nm --size-sort`
sobre el `.elf` del 3-sep):

| Discovery (3008 KB) | bytes |
|---|---|
| `s_vm_mem` | 524 288 |
| `s_framebuffer` 800×480×2 | 768 000 |
| `g_nodes` 96 KB · `s_drawbuf` 75 KB · `g_scratch` 16 · `s_put_buf` 12 · resto | ~257 000 |
| libre para malloc y pila (con 512 KB de VM) | ~1 470 000 |

La Nucleo vive con ~158 KB libres (768 KB − 624 354 de estático). Con el mismo margen la Discovery
admitiría ~1,75 MB; se pone **1536 KB** (tres veces la Nucleo) y quedan **457 404 B** libres (bss
2 622 124 de 3 080 192). El enlazador sigue vigilando (`._user_heap_stack`). `test_mem` fija el
caso (**24/24**): `vm: 1536 KB en SRAM estática (todo lo que deja la región)`, reparto 1152 + 384.
Los dos `.elf` compilan headless: la Nucleo idéntica (bss 624 354), la Discovery con bss 2 622 124.
✅ **En placa (3-sep, Eduardo la regrabó desde CubeIDE, sello 19:26)**: INFO `vmHeapBytes 1179648 /
vmStackBytes 393216` (1152 + 384 = 1536 KB), y el panel del IDE lo dice en sus unidades:
`VM: heap 1.1 MB + stack 384 KB`. Tres veces la VM de antes en la misma placa, sin tocar nada
más que una constante.

📌 **Con esto, las cinco familias —Pico/Metro, S3, C3, P4, STM32— deciden su memoria con
`bpvm_mem_plan()`** y la cuentan con la misma línea. El paso 2 de `U6` queda cerrado en código;
una placa futura (C6, P4X, S31…) entra escribiendo un enumerador de diez líneas y sus tres
constantes.

#### ✅ `#467` — el IDE del repo compilaba contra la stdlib de V5: el `BpVM.cfg` del CWD mandaba sobre el del fichero (3-sep, cerrada el mismo día)

**Eduardo, con la captura**: 25 errores *«el módulo importado 'Math' no expone 'clamp'»* al abrir
`samples/MathRango.bp` — y el jar era el de ayer. Lo primero, con hechos: el frontend del repo
(30-ago 17:16) y la copia embebida en el jar (el mismo `MivmEmitter.class`) compilan
`MathRango.bp` con `--stdlibDir bpstdlib` sin un error. Compilador y stdlib estaban al día.

**La causa**: `FrmMain.resolveStdlibDir` miraba **primero el cwd del proceso** («compatibilidad
histórica»), después el `outDir` del fichero y luego el proyecto. Lanzado el jar del repo desde
`C:\temp\BasicPlus-5.0-win`, el cfg que encontraba era el del dist (`"stdlibDir": "./bpstdlib"`
= la stdlib de V5, `Math` MOD6 sin las cuatro funciones), y con ése compilaba un fichero del repo.
Jar nuevo, stdlib vieja; y el nombre `BpIde-5.0.jar` en los dos sitios terminó de confundir.

**Arreglo**: (1) el orden — el cfg junto al fichero (walk-up desde `outDir`) manda, luego el del
proyecto, y el cwd queda **el último**, como respaldo para quien no tenga cfg; (2) el `pom` de
BpIde pasa a **`6.0`** (era la etiqueta de V5 sin subir): el jar del repo es `BpIde-6.0.jar` y el del
dist sigue siendo `BpIde-5.0.jar`, que ya no se confunden; `bpide.bat` arranca desde la raíz del
repo; `CLAUDE.md`, `README` y `PUBLICAR` al día. El `BpIde/target/BpIde-5.0.jar` viejo, borrado.

### Segunda vuelta: con el jar 6.0 y el fichero del repo, el MISMO error — la causa de verdad estaba en el COMPILADOR

La captura siguiente lo dejó claro: `BpIde 6.0 — C:/lenguajes/pm/samples/MathRango.bp`, 25 errores.
El cfg ya no podía ser. **Reproducido en línea de comandos** con el outDir que usa el IDE para un
fichero suelto (`<dir del .bp>/out`):

```
basicplus-frontend.jar samples/MathRango.bp --compile samples/out   → 25 «no expone»
basicplus-frontend.jar samples/MathRango.bp --compile <tmp>         → compila
```

**`samples/out` guardaba la stdlib ENTERA de V5**: 26 `.mod` MOD6 del 21-ago (`Math.mod` 2320 B,
`I2c.mod` 4153 B… los mismos tamaños que aparecieron en el `/lib` de la Metro). Y el compilador
resuelve un `import` buscando el `.mod` en **outDir → dir del fuente → dependencyPaths** (donde va
`stdlibDir`): la copia rancia de `samples/out` ganaba a la buena de `bpstdlib`. `carriesInterface()`
sólo descarta los v5 SIN sección de interfaz; un MOD6 la lleva, y pasaba. Es la misma clase de
fallo que el IDE ya arregló para las dependencias del device (26-jun, «stdlib primero»): el
compilador no lo tenía.

**Arreglo de raíz (frontend)**: `Ctx.stdlibDir` (el del cfg más específico) y **la stdlib PRIMERO**
en los tres buscadores —`locateImportMod`, `locateImportBpi` y `loadContractInterface`—: un
módulo que vive en `stdlibDir` ES la stdlib, y ninguna copia por el camino corta la búsqueda.
Verificado: con `--compile samples/out` **0 errores**; `samples/out/Math.mod` sigue ahí (2320 B) y
ya no manda; el `MathRango.mod` compilado ahí corre **byte-idéntico en las dos VMs** (29 líneas);
el jar del IDE reconstruido con el frontend nuevo (`frontend/Main.class` del 3-sep 18:59). **Y
verificado por Eduardo en el IDE**: `== compilación OK: MathRango.mod ==` con `samples/out` delante,
y el Run en la Pico 2 byte-idéntico al host.

📌 Lo del cfg (cwd primero) era una trampa real, pero **no la de hoy**; queda arreglada igual. Y
`samples/out` sigue lleno de MOD6 de V5: ya no hacen daño, pero son un fósil (gitignorado) que
conviene vaciar cuando toque.

#### ✅ `#469` — ~~una fachada SIN BACKEND devuelve un número inventado~~ (abierta 5-sep, de `A3` · **CERRADA el 8-sep por Eduardo**)

**El síntoma, en una placa real**: en una Nucleo o una Discovery, `Adc.read()` devuelve un número
que se mueve, parece una lectura y **es falso**; y `Adc.Channel(0)` imprime `→ GP26`, que es el
pinout del RP2350, desde código común.

**La causa, comprobada**: sólo dos familias registran backend de ADC —
```
grep bpvm_adc_set_backend  →  pico/main.c:1543  ·  esp32/common/gpio_esp32.c:629
```
El STM32 no. Y `src/adc.c` no falla cuando no hay backend: `initChannel` devuelve `26 + ch` con un
`printf` del pinout de la Pico, y `readChannel` devuelve una **rampa** (un contador que avanza de
73 en 73 con vuelta a 4095).

⚠️ **Y la red de seguridad no puede verlo por construcción**: miVM hace lo mismo
(`VirtualMachine.java:5495` imprime `→ GP26`), así que la **paridad dual-VM sale VERDE**. Es el
caso exacto de dos normas del proyecto: *«errores sí, silenciosos no»* y *«un aviso que no
distingue no-evento de fallo»*.

⏭️ **El arreglo tiene dos mitades y la segunda importa más**:
1. Que el STM32 registre su backend de ADC (o declare que no tiene).
2. Que **la fachada distinga** «no hay hardware» (el host, legítimo) de «nadie registró backend»
   (un olvido) y en el segundo caso **grite**. Eso protege a todas las familias futuras, no sólo a
   ésta — y hay que mirar las OTRAS 16 fachadas por si tienen el mismo stub complaciente.

✅ **CERRADA el 8-sep. Palabras de Eduardo:** *«`#469` está cerrado. En V7 podemos hacer alguna
cosa y habrá que hacer algún ajuste, pero para V6 está completo.»*

Lo que la cierra es el reparto de abajo: con el eje de **familia** —el suyo— la RP2350 ya estaba
completa, el `neopixel` de ESP32/STM32 quedó decidido (pendiente de implementar de verdad, y
mientras tanto **lanza excepción BP**, `3d14e32c`) y el `adc` del STM32 se fue a **V7** con
`#479`. No queda ningún hueco de esta clase. Lo que sigue vivo es de OTRA clase y tiene ficha
propia: los cuatro drivers que **tienen backend y mienten**, en `#480`.

##### ✅ EL ALCANCE, DECIDIDO POR EDUARDO (8-sep) — y el eje bueno era otro

Yo planteé la pregunta **por fachada** («¿lo extendemos a 3 de 13, a las 13, o a ninguna?») y
Eduardo la corrigió: *«eso pertenece a la capa HAL BP, depende del hardware pero sobre todo depende
de la FAMILIA. Así que no se trata de hacerlo 8 veces, sino 3 y una ya está hecha.»*

📐 **Medido con ese eje** — 11 fachadas con backend (`fs` y `net` no usan `set_backend`, quedan
fuera) × 3 familias. Los huecos son **cinco**, no ocho ni trece:

| | RP2350 | ESP32 | STM32 |
|---|:---:|:---:|:---:|
| `adc` | sí | sí | **no** |
| `neopixel` | sí | **no** | **no** |
| `rtc` | **no** | **no** | sí |
| gpio · i2c · pico · pulse · pwm · spi · uart · wdt | sí | sí | sí |

✅ **Y Eduardo tenía razón: la RP2350 ya está hecha.** Su único «hueco» es `rtc`, y **no es de esta
clase**: `src/rtc.c` sin backend **no se inventa nada** —mantiene un offset de epoch sobre el reloj
monotónico, que es una implementación de verdad— y la ausencia está **declarada en el sitio del
registro** (`pico/main.c:1547`: *«Rtc en Pico usa el stub portable»*). El ESP32 lo declara aún mejor
(`esp32/common/gpio_esp32.c:552-556`). Así que `rtc` no es un hueco en ninguna de las tres.

🔑 **La norma sale de ahí, y es la de la familia de referencia:** *cada fachada **o** registra
backend, **o** el camino común es una implementación de verdad **y** la ausencia está declarada
donde se registra.*

Con esa norma quedaban dos celdas, y Eduardo decidió las dos:

- **`neopixel` en ESP32 y STM32** — *«se implementó en la Pico gracias al PIO que las otras no
  tienen. Yo lo dejaría pendiente en las otras 2 familias hasta que lo implementemos de verdad. Si
  alguien lo pide y no está que salte una excepción»* — y el matiz que fija el contrato: *«tienen
  que lanzar una excepción BP»*. ✅ **HECHO el 8-sep (`3d14e32c`)**, y el culpable no era el que yo
  señalaba: la VM-C ya lanzaba desde H13; **la que fingía éxito era miVM**. Detalle en la nota de
  abajo. ⏭️ Implementarlo **de verdad** en esas dos familias queda pendiente y no tiene ficha aún.
- **`adc` en el STM32** — *«se ha hecho una parte, creo que falta indicar qué pines soportan ADC y
  cuáles no. Eso hay que definir unos vectores con los números de pines. Creo que lo aplazamos a
  V7.»* ⏩ **A V7, con `#479`**, que ya se llevó allí el mapa de pines. Ojo al síntoma que queda
  mientras tanto, porque es una contradicción visible: `gpio_stm32.c:318` anuncia **20 canales**
  (`adcChannels`) y `bpvm_adc_set_backend` aparece **0 veces** en todo el port — la placa dice tener
  20 y al usarlos contesta *«canal no válido (0..3)»*.

⚠️ **Y una cosa que esta ficha mezclaba y conviene separar**, porque cambia quién arregla qué:
- **Clase 1 — no hay backend**: la fachada se inventa un valor porque no hay a quién preguntar. Es
  HAL BP, es por familia, y es lo que resuelve la norma de arriba.
- **Clase 2 — hay backend y miente**: el registro existe pero el driver está mal. Son los cuatro de
  `#480` (el `Wdt` del STM32, el `Pulse` del C3, el ADC del ESP32 que falla con 0, el breadcrumb con
  los slots a NULL). **El patrón de esta ficha no los arregla**: son bugs de driver, uno a uno.

#### ✅ `#480` — LO QUE DESTAPÓ LA AUDITORÍA DE LAS 13 FACHADAS, verificado (abierta 6-sep · **CERRADA el 9-sep**: dos piezas arregladas, una a V7 y una separada como `#487`)

Tirando de `#469` se auditaron **las trece fachadas de periférico** (quién registra backend en cada
familia, y qué hace la fachada común cuando nadie lo hizo), con una pasada de verificación
adversarial encima. Lo que **sobrevivió**, en orden de daño, y que **no está cubierto por otra
ficha**:

1. 🔴 **`Wdt.disable()` no desactiva en el STM32.** `stm32_wdt_disable_impl` (`gpio_stm32.c:736-739`)
   no puede parar el IWDG y hace «mejor esfuerzo»: lo **reprograma a ~131 s** y lo refresca, con
   `s_wdt_on` todavía a 1. Consecuencia: **la placa se resetea sola** si un `sleep` pasa de ahí —
   haciendo justo lo que recomienda `bpstdlib/Wdt.bp:41`. Y **desde BP no hay forma de saberlo**: el
   builtin devuelve 0 en las cinco familias. Contraste que prueba que no es «así son los
   watchdogs»: la Pico llama a `watchdog_disable()` **de verdad** (`pico/main.c:865-874`).
   ⚠️ Y la doc de usuario culpa al chip equivocado: `Wdt.bp:82-84` dice *«en RP2350 esto se simula
   con un timeout muy grande»* — falso desde que se arregló el RP2350, y **no nombra al único chip
   donde hoy es cierto**.

2. 🟠 **`Pulse` en el ESP32-C3 cuenta 0 para siempre.** El C3 no tiene PCNT y **la decisión de no
   registrar backend es correcta y está documentada** (`#465`, `gpio_esp32.c:630-635`). Lo que está
   mal es lo que hace la fachada común entonces: `src/pulse.c:36-39` devuelve un `counterId` **0
   válido** (el wrapper BP sólo lanza si es `< 0`), y `value()` devuelve `s_stub_value`, que **nadie
   incrementa jamás**. Un frecuencímetro que lee 0 es indistinguible de «no llegan pulsos»: se va a
   buscar al cableado.

3. 🟠 **El backend de ADC del ESP32 falla con 0, no con −1.** `gpio_esp32.c:477,482,486,488`
   devuelven **0** cuando no se pudo abrir la unidad o falló la lectura. El contrato del header pide
   −1 (`bpvm_adc.h:12-14`) y `Adc.bp:60-63` sólo lanza si `< 0`. O sea: ADC roto → **0 V constante y
   mudo** en las cuatro familias ESP32. Mismo vicio que `#469`, esta vez **con backend registrado**.

4. 🟡 **El breadcrumb está mudo en 4 de 5 familias.** El cuarteto `setMark`/`markCount`/`markAt`/
   `bootCount` sólo lo rellena el STM32 (`gpio_stm32.c:310-325`); Pico, ESP32 y P4 lo dejan **NULL**,
   y `src/pico.c:121-140` responde no-op **mudo**, `0`, `0` y `1`. En placa,
   `samples/BreadcrumbDemo.bp` imprime «Arranque #1» y «Migas: 0» para siempre, y su línea final
   **promete en voz alta algo falso**. Es el «instrumento mudo» en la herramienta que existe justo
   para cuando ya estás en problemas.
   📌 **Es una clase de fallo distinta a `#469`**: no es «nadie registró backend», es **backend
   registrado con la mitad de los slots a NULL**. El arreglo de la ADC no la cubre.

✅ **Lo que el censo EXAGERABA y se tachó al verificar** (para no mandar a nadie a mirar donde no
está el problema): **PWM no tiene hallazgo** —las cinco familias registran, el stub sólo corre en el
PC—, y **RTC no es este vicio**: su modelo de offset es deliberado y está documentado en cuatro
sitios, incluida la doc de usuario. *(El residuo real del RTC sí es de otra caja: con autorun no hay
IDE que calibre, y una placa desplegada presenta ms-desde-boot como epoch Unix.)*

⏭️ El arreglo genérico ya está escrito y probado en la ADC (`#469`): **el que no tiene hardware
registra un backend explícito, y la ausencia pasa a ser un error**. Falta extenderlo — y la pieza 4
necesita además que la fachada distinga «slot NULL» de «backend ausente».

### ✅ 9-sep — LA PIEZA 1 (el watchdog) CERRADA, y la 2 va a V7 (`78320969`, `c6dfa022`)

**Criterio de Eduardo, que zanja la pieza 1 sin discutir el caso concreto:** *«El watchdog se ha de
implementar completo o no se implementa. Lo del STM32 o se arregla o se desactiva completamente, no
podemos tener medio watchdog.»*

**Arreglarlo NO SE PUEDE**: el IWDG del U5 no se para por software —arrancado con `KR=0xCCCC`, sólo
lo apaga un reset—, y eso es silicio. Medido además que **nada lo arranca en el boot**: sólo lo
enciende `Wdt.enable()` desde BP.

| | enable | feed | disable | |
|---|---|---|---|---|
| **Pico** | real | real | `watchdog_disable()` **de verdad** | ✅ se queda |
| **ESP32** | Task WDT → reset | real | des-suscribe la task | ✅ se queda |
| **STM32 U5** | IWDG real | real | ❌ imposible | 🚫 **sin backend** |
| **PC** | — | — | — | 🚫 **sin backend** |

Sin backend, los tres verbos lanzan un `RuntimeError` BP atrapable, igual que el NeoPixel desde el
8-sep. **El código del IWDG se borra, no se comenta**: git lo guarda y el código muerto son avisos
del compilador para siempre (el build del Nucleo pasa de 5 avisos a 3). Y se corrigió la doc de
usuario, que **culpaba al chip equivocado**.

### 🧬 La pieza 2 (el `Pulse` del C3) → **V7**, con `#470`

**Eduardo:** *«El contador lo tiene P4. Si los otros micros no tienen contador, lo que hay que hacer
es lo mismo que con las UARTs, I2C, etc. Va con los ficheros del micro.»* Medido: el PCNT lo tienen
**S3, C6 y P4**; el **C3 es el único sin él**. Así que no es un parche de `Pulse`: es la misma
declaración por micro que `#470` y `#479`.

### 🔴 Y tirando de ahí salió una ROTURA DEL INVARIANTE, viva desde siempre (`c6dfa022`)

**Seis fachadas sin hardware en el PC** —`gpio`, `pwm`, `pulse`, `uart`, `spi`, `pico`— escriben su
línea de simulación **dos veces**, una en cada VM, y **no decían lo mismo**: `(sim …)` contra
`(stub …)`. **Trece textos.** Mismo `.mod`, `stdout` distinto.

🕳️ **Por qué nunca saltó: ninguna de las seis tenía un sample en el corpus.** Es el mismo agujero
que `#481` señala para el camino de error, y con la misma forma — *lo que no se compara, se pudre*.

🐛 **Y uno de los trece NO era cosmético.** `src/uart.c:42` usaba **`putchar`** en medio de una
secuencia de `bpvm_out`: `putchar` escribe al `FILE` de C y **se sale del sumidero de la VM**, así
que el payload salía **desordenado** y el preview quedaba vacío.

```
miVM   [uart] write bus=0 bytes=[41 42 43] ("ABC")
VM-C   ABC…                   bytes=[41 42 43] ("")
```

📌 Y dos líneas más abajo, en el mismo fichero, hay un comentario de `#478` que dice *«TEXTO =
CONTRATO DE PARIDAD: idéntico al de miVM»*. **Se hizo para `read` y se saltó `write`.**

La VM-C se alinea con miVM (esa misma convención: miVM es la referencia), y **`samples/StubParidad.bp`
entra en el corpus** tocando **cada** verbo con stub de las seis fachadas. Corpus **48 → 50**, 50 PASS.

### ⏭️ Lo que queda de `#480`

| | | |
|---|---|---|
| 1 | el watchdog del STM32 | ✅ 9-sep |
| 2 | el `Pulse` del C3 | 🧬 **V7**, con `#470` |
| 3 | el **ADC del ESP32 falla con 0, no con −1** → ADC roto = 0 V constante y mudo, **con backend registrado** | 🔴 abierto |
| 4 | el **breadcrumb mudo en 4 de 5 familias** | ➡️ **SEPARADO el 9-sep como `#487`** |


### ✅ CERRADA el 9-sep — la pieza 3, y con ella la ficha entera (`e875f7e2`)

**El backend del ESP32 devolvía `0` en sus CUATRO caminos de fallo**, y `0` es una lectura
perfectamente válida (0 V) y un pin plausible. Con `Adc.bp` lanzando sólo si el retorno es `< 0`,
salía el peor de los tres desenlaces: **ADC roto = 0 V constante y mudo**, en las cuatro familias
ESP32. Y **con backend registrado**, o sea que el arreglo de `#469` —la ausencia de backend hace
ruido— no lo cubría: es una clase de fallo distinta.

Ahora los cuatro devuelven **−1**, que es lo que pide `bpvm_adc.h` y lo que ya hacía la familia de
referencia (`pico/main.c`).

**Y al mirarlo salieron dos cosas más:**

1. **`initChannel` devolvía `1` al ir BIEN**, cuando el contrato dice *«el número de pin físico»*.
   `Adc.Channel` lo publica en la propiedad `pin_`, así que en un ESP32 un programa que la leyera
   veía `1`. Ahora sale el **GPIO de verdad** (`adc_oneshot_channel_to_io`), como en la Pico — y el
   canal se valida contra `SOC_ADC_CHANNEL_NUM` en vez de aceptar cualquiera.
2. **Con el backend arreglado, el error se mudaba un piso arriba**: un fallo de LECTURA devolvería
   −1 a BP y nadie lo mira — `read()` daría −1 y `readVolts()` **voltios negativos**. `Adc.bp` gana
   `leer()`, que lanza un `RuntimeError` atrapable: el mismo idioma que ya usaba el constructor.

⚠️ `Adc` va **embebida** en las imágenes, así que se regeneraron los blobs de las tres familias.
Sólo cambia `Adc.mod`; los otros 27 salen byte-idénticos.

⚠️ **Lo que NO queda probado, y conviene que esté escrito**: el lanzamiento de `leer()`. En el host
el stub **no falla nunca**, así que esa rama sólo se ejerce en placa con un ADC que dé error — no
hay forma de forzarla desde el arnés sin un inyector de fallos que hoy no existe.

### 📋 La ficha, cerrada por piezas

| | | |
|---|---|---|
| 1 | el watchdog del STM32 | ✅ 9-sep (`78320969`) |
| 2 | el `Pulse` del C3 | 🧬 **V7**, con `#470` — cada micro declara lo que tiene |
| 3 | el ADC del ESP32 | ✅ 9-sep (`e875f7e2`) |
| 4 | el breadcrumb mudo | ➡️ separado como **`#487`** |

📌 **Y el hilo que las une**, que es lo que hay que llevarse: las cuatro eran **la misma forma** —
fallar con un valor que el llamante no distingue del éxito. El arreglo genérico ya estaba escrito en
`#469` y aquí se aplicó tres veces seguidas.

#### ✅ `#481` — las dos VMs discrepan en `stdout` cuando el programa MUERE con una excepción sin atrapar (abierta 8-sep · **CERRADA el 9-sep**, `9e7f4a59`)

**Rotura del invariante sagrado, y de radio ancho**: para **cualquier** programa que termine con una
excepción BP no atrapada, las dos VMs escriben cosas distintas en `stdout`.

```
module ThrowTest        // un índice fuera de rango, nada exótico
  function Main()
    print "antes"
    var a: integer[] := [1, 2, 3]
    print a[99]
    print "despues"
  end Main
end ThrowTest
```

```
miVM   →  antes
VM-C   →  antes
          === RuntimeError: ALOAD: índice fuera de rango 99 (length=3) ===
```

miVM manda el informe **sólo por `stderr`** (`[bpgenvm worker 0, tid=0] <mensaje>`); la VM-C pone
una línea `=== RuntimeError: … ===` en **`stdout`** (y sus diagnósticos, además, por `stderr`).

📌 **No es de una fachada ni de un builtin.** Salió midiendo el NeoPixel, pero se reprodujo con un
índice fuera de rango y con una división por cero: es el **camino de salida del error**, común a
todo. El arreglo del NeoPixel (`3d14e32c`) no lo toca ni lo necesita — allí la excepción **se
atrapa**, y en ese caso las dos VMs ya salen byte a byte idénticas.

🕳️ **Y explica un agujero del corpus que llevaba ahí desde siempre: no hay ni un sample que muera
lanzando.** No es casualidad — entrarían **todos** en rojo. O sea que la red de paridad no vigila
hoy el camino de error, que es justo por donde se sale cuando algo va mal.

⏭️ **Lo que hay que decidir antes de tocar nada** (es de Eduardo): ¿el informe del error no atrapado
es **salida del programa** o **diagnóstico**?
- Si es salida → **miVM** tiene que imprimirlo en `stdout` con el mismo formato que la VM-C.
- Si es diagnóstico → **la VM-C** tiene que moverlo a `bpvm_diag`/`stderr`.

⚖️ **Lo que pesa a favor de `stdout`**: en el micro el usuario ve el `stdout` **por el cable**, y si
el motivo de la muerte se va por `stderr` se lo traga el firmware. Ojo también a que hoy la VM-C, si
el módulo **no exporta `RuntimeError`**, no puede construir el objeto y avisa de que *«el programa NO
se entera»* — ese camino hay que mirarlo a la vez.

⏭️ Y cuando se cierre: **meter un sample que muera** en el corpus de `compat/compat.sh`. Mientras no
lo haya, esto se puede volver a torcer sin que suene nada.


### ✅ CERRADA el 9-sep (`9e7f4a59`) — y eran DOS roturas, no una

**El marco lo puso Eduardo, y es lo que ordenó el trabajo:** *«Aquí hay que distinguir de un exit
diferente de 0, de una excepción no atrapada. 2 casos.»* Al medirlo, cada caso estaba roto de su
propia manera:

| | `stdout` | `exit` |
|---|---|---|
| **miVM** | `antes` | **0** ← decía que había ido BIEN |
| **VM-C** | `antes` + `=== RuntimeError: … ===` | **11** ← el ordinal del enum |

### Caso 1 — el informe: es DIAGNÓSTICO, y lo dice quién lo escribe

No lo imprime el programa: lo imprime **el runner**, explicando por qué murió. Mientras estuvo en
`stdout`, **cualquier** programa que muriese rompía la paridad por construcción. Ahora va por
`stderr` en las dos.

🔑 **Y el argumento que esta ficha daba a favor de `stdout` —*«en el micro el usuario ve el stdout
por el cable»*— NO APLICA**, y eso sólo se vio mirando quién escribe qué: en placa el informe **no
pasa por ahí**. Las cuatro cinturas leen `bpvm_runtime_error()` y lo meten en el `errorMessage` del
`EXITED`, que es otro canal. El `=== RuntimeError: … ===` lo escribía **sólo el CLI del host**
(`test/main.c:410`). O sea que la «rotura entre las dos VMs» era, en realidad, entre **dos CLI**.

### Caso 2 — el código de salida: UNA tabla donde había cinco copias crudas

miVM salía con **0** (sistemático, no una carrera como la del `stop` de `#462`) y la VM-C devolvía
`(int) status` — el ordinal del enum. Y ese mapeo estaba escrito **cinco veces**: el CLI del host y
los `map_vm_status` de Pico, ESP32, STM32 y el sim, todas igual de crudas.

```
  0  termino bien               3  fallo interno de la VM
  1  excepcion BP no atrapada   4  sin memoria
  2  no se pudo cargar        130  parado con Stop/KILL     131  parado por el depurador
```

`bpvm_exit_code()` / `bpvm_exit_code_str()` en el común (`src/bpvm.c`), y miVM aplicando la misma.
**El número dice QUÉ pasó; el detalle viaja aparte.**

📌 **El `3` tiene número propio a propósito**: un opcode o un PC inválidos **no son culpa del
programa** — son un bug nuestro o un `.mod` corrupto. Confundirlo con el `1` manda a mirar donde no
está el problema.

### 💡 Y de aquí salió una mejora que no estaba pedida

> **Eduardo:** *«Ahora que estamos, no estaría mal que en la línea de comandos del IDE se pudiera
> preguntar `help error` y que devuelva lo que significa cada código, porque a mí un `exit(11)` no
> me dice nada.»*

Hecho, y es lo que hace la tabla **útil**: el número deja de ser un número. La ayuda vive **junto a
la función que la aplica**, para que no puedan desfasarse. ⚠️ Al tocar el IDE hay que reconstruir su
fat-jar **con el IDE cerrado** — hecho.

### El agujero del corpus, tapado

La ficha lo decía: *«no hay ni un sample que muera lanzando. No es casualidad — entrarían TODOS en
rojo»*. Ahora hay uno: **`samples/ThrowSinAtrapar.bp`**. Corpus **52 → 53**, 53 PASS.

⚠️ **Lo que el arnés NO vigila**: el código de salida. `run_vm` compara `stdout` y no mira el
`exit`. Los dos 1 están medidos a mano. Si algún día alguien devuelve el ordinal otra vez, la
paridad seguiría verde.

📌 Nadie dependía de los códigos viejos — comprobado antes de tocarlos: en todo el repo sólo se
mira `== 0`.

#### ✅ `#482` — la VISTA DOBLE de los packs: NO la necesita nadie (abierta 9-sep, de la cola heredada · **CERRADA el 9-sep**, medido en C3 y S3)

**Qué es.** El 8-sep se arregló el mapeo de packs del ESP32 con dos bases —
`bpvm_pack_mount(base_lectura, base_ejecucion, size)` y `bpvm_pack_exec_ptr()` en el **único** sitio
donde un puntero deja de ser dato y pasa a ser código (`mdn_loader.c:201`). La parte de **lectura**
está verificada en placa (`PACK_LS_REPLY regionSize=5382144, chainOk=true`, donde antes decía «sin
zona de packs»); la de **ejecución** no se ha ejercitado nunca.

📐 **Su condición es del MMU, no de la ISA**: hace falta un chip donde las direcciones virtuales de
instrucción y dato **NO se compartan**, que es cuando el puntero de lectura no vale para ejecutar.

| | ISA | `SOC_MMU_DI_VADDR_SHARED` | ¿tiene `.mdn`? |
|---|---|---|---|
| **ESP32-S3** | **Xtensa** | no lo declara → **separadas** | ❌ no existe generador |
| **ESP32-C3** | RISC-V | no lo declara → **separadas** | ✅ RISC-V |
| ESP32-C6 | RISC-V | 1 → compartidas | ✅ |
| ESP32-P4 | RISC-V | 1 → compartidas | ✅ |

⏭️ **Se prueba en el C3**, que cumple la condición del MMU **y** tiene `.mdn`. Eso ejercita el
código común: la vista doble, `bpvm_pack_exec_ptr` y el montaje.

⚠️ **Y lo que ese test NO prueba, que es de lo que va la ficha de al lado.** Corrección de Eduardo
(9-sep): *«el S3 es Xtensa y el C3 es RISC-V, son casos diferentes.»* Tiene razón y yo había
propuesto el C3 como si fuera equivalente: lo es **para el MMU** y no lo es **para la ISA**. Que el
S3 ejecute desde un pack necesita código Xtensa, y eso es `#488`. Lo verde en el C3 dirá «la vista
doble funciona», no «el S3 ejecuta packs».

📌 Es la lección del `-mcmodel=medany` otra vez: la ISA impuso un requisito real que sólo se vio en
el destino. Dar por buena una arquitectura probando otra es justo el error que aquella costó.


### 🔴 9-sep — MEDIDO EN LA C3, Y LA CLASIFICACIÓN DE ARRIBA ERA FALSA

**Escepticismo de Eduardo, y tenía razón**: *«¿Seguro que el C3 hace eso? Me cuesta entender que se
comporte diferente que el C6 o el P4. Antes de nada hay que verificar.»*

Flasheada la C3 con el firmware de hoy y leído su log de arranque por el wire:

```
pack: zona 1488 KB en bpdata+0x174000 | INST ok @0x4207c000 | DATA ok @0x4207c000
pack: zona montada en 0x4207c000 (1488 KB) — modulos y .mdn visibles
```

**Las dos vistas son LA MISMA DIRECCIÓN.** La C3 se comporta como el C6 y el P4.

🔑 **De dónde salió el error, porque es la lección**: yo clasifiqué los chips leyendo si
`soc_caps.h` declara `SOC_MMU_DI_VADDR_SHARED`. **Pero el código no mira ese macro**: decide en
*runtime* comparando lo que devuelve `esp_partition_mmap` (`board_mgr_esp32.c:192`,
`s_map_data != s_map_inst`). O sea que la tabla que había aquí no era una medida, era una lectura —
y de un sitio que el programa ni consulta.

### ⏭️ Y eso deja una pregunta MAYOR que la que la ficha tenía

Si la C3 no tiene vistas separadas, **¿las tiene alguien?** El único candidato que queda es el
**S3**, y esa afirmación se apoya exactamente en la misma lectura del macro que acaba de fallar.

⚠️ **Si el S3 también diera una sola dirección, el mecanismo de vista doble no tendría NI UN
USUARIO** — y lo que arregló los packs del S3 el 8-sep no habría sido la vista doble, sino la otra
mitad de aquel cambio: **mover el mapeo al fichero común** (que es lo que dejaba al C6 sin packs por
vivir el código en el directorio del P4). Serían dos arreglos en un commit y sólo uno haciendo el
trabajo.

📐 **La comprobación es de dos minutos y no necesita código**: el S3 ya lleva el firmware de hoy;
basta conectarlo y leerle el log por el wire, buscando esa misma línea `pack: … INST @… | DATA @…`.
Hasta entonces, `#482` no es «falta ejercitar el camino de ejecución»: es **«hay que averiguar si
ese camino existe»**.

📌 De paso, la C3 quedó al día: llevaba firmware **anterior al 8-sep** —su log tenía once arranques
y ni una línea `pack:`, que es el bug del directorio del vecino— y ahora monta la zona y trae
`Machine.mod` y el `Adc.mod` nuevo.


### ✅ CERRADA el 9-sep — el caso de vistas separadas NO EXISTE en nuestros micros

**Medido en las dos placas, no leído:**

```
C3   pack: zona 1488 KB en bpdata+0x174000 | INST ok @0x4207c000 | DATA ok @0x4207c000
S3   pack: zona 5256 KB en bpdata+0x9c6000 | INST ok @0x4287e000 | DATA ok @0x4287e000
```

Las dos vistas son **la misma dirección** en los dos chips que se suponían el caso raro. La Pico
(XIP), el STM32 (flash mapeada), el P4 y el C6 tampoco las separan. **El mecanismo de vista doble no
tiene ni un usuario.**

📐 **Y la explicación de fondo es de Eduardo, y vale más que la medida porque además PREDICE**:
*«esto de las vistas separadas me recuerda a micros sin arquitectura Von Neumann, y eso con ARM y
RISC-V lo veo muy raro»*. Exacto: vistas separadas para instrucciones y datos es un rasgo
**Harvard**, y ARM Cortex-M y RISC-V tienen el espacio de direcciones **unificado**. Que un
`soc_caps.h` no declare `SOC_MMU_DI_VADDR_SHARED` no implica que la MMU las separe.

### 🔴 Y lo que esto corrige del 8-sep

Aquel commit llevaba **dos arreglos** y sólo uno estaba trabajando. Lo que dejaba al S3 «sin zona de
packs» **no eran las vistas**: era que `board_mgr_esp32_mapear_packs()` vivía en el directorio del
P4 y el proyecto del S3 no lo llamaba nunca. Lo confirma la C3 de hoy: su firmware anterior al 8-sep
**no tenía ni una línea `pack:`** — el mapeo sencillamente no corría.

### Qué se hace con el mecanismo (decisión de Eduardo)

> *«Posible es, pero no sé si algún día nos vamos a meter en eso. Si prefieres B, pues B.»*

**Se queda** (opción B): cuesta un parámetro y una función identidad, y el día que aparezca un micro
que sí las separe está resuelto. **Lo que NO se queda es la afirmación falsa**, corregida en los
tres sitios donde estaba repetida (`bpvm_pack.h`, `src/mdn_loader.c`,
`esp32/common/board_mgr_esp32.c`) — porque era ella la que hacía daño: se lee y se clasifican chips
con ella, que es exactamente lo que me pasó a mí.

📌 **La lección, y es la que hay que llevarse**: clasifiqué cuatro micros **leyendo un macro que el
programa ni consulta** — el código decide en *runtime* comparando lo que devuelve
`esp_partition_mmap`. No era una medida, era una lectura, y del sitio equivocado. La corrección la
disparó el escepticismo de Eduardo (*«¿seguro? Antes de nada hay que verificar»*), no una prueba.

📌 De paso, **la C3 quedó al día**: llevaba firmware anterior al 8-sep, y ahora monta la zona de
packs y trae `Machine.mod` y el `Adc.mod` nuevo.

#### 🧬 `#492` — los campos PROTEGIDOS no cruzan módulos: la interfaz exporta métodos y propiedades, pero no campos → **V7** (abierta 11-sep, de `G2`)

**La regla, de Eduardo (11-sep):** *«Una variable es `protected` si no se indica nada, `private` si se
declara como tal, y no puede ser pública salvo que sea una propiedad.»* Protegido = lo ven la clase y
sus descendientes.

**Lo que hace el compilador**: un descendiente que vive en **otro módulo** no ve los campos protegidos
de la base — `OwnerList extends Core.List` recibía *«'OwnerList' no tiene miembro de instancia
'items'»*. La causa está en el contrato de módulos: `ClassSig` (`ModuleInterface.java`) exporta
**métodos, propiedades, constructor, constantes estáticas y el layout binario** (cuántos campos, para
que el descendiente coloque los suyos), **pero no los nombres ni los tipos de los campos**. Su propio
comentario lo dice: *«Sin fields públicos (se exponen vía property)»* — correcto, pero se dejó fuera
también a los protegidos. `lookupInstance` sí sube por `baseClass`; lo que no encuentra es lo que
nunca se dio de alta.

**El arreglo, y su forma**: la interfaz es texto (`method …`, `prop …`, `staticconst …`, `event …`);
falta una línea **`field <nombre>:<tipo> slot <n>`** por cada campo **no privado**, y que el
importador los dé de alta como protegidos. `checkVisibility` ya aplica la regla. Las VMs no cambian:
el acceso es por slot. **Coste**: rehacer la cadena entera (jar, stdlib como proyecto, samples, fat-jar
del IDE, blobs de las cinco imágenes) — el de siempre al tocar el frontend.

⏭️ **Aplazado por Eduardo**: *«utiliza un get para acceder al campo y de momento salimos del paso»*.
Mientras, `List.backing()`. Y ojo: **`G2` lo va a pisar** — un `MainWin extends Gui.Window` de usuario
que toque un campo protegido de `Window` se estrella igual.

#### 🧵 `#495` — `GET`/`LS` DURANTE UN RUN: el `BUSY` es de la época de un solo hilo → **V7** (abierta 11-sep, de `C1`)

**De dónde sale.** Al cerrar el flujo de la captura (`RUN → Gui.shot dentro → EXITED → GET`) apareció
que el wire contesta `BUSY` a todo lo que no sea `HELLO`/`KILL`/`RESET` mientras corre un programa, en
las cinco implementaciones (`repl_esp32.c:593`, `bpvm_sim.c:634`, `repl_v1.c:967`, `stm32_repl.c:334`
y el host). Eduardo: *«ese `BUSY` es algo artificial: ahora, con dos hilos del SO corriendo, no hay
nada que impida hacer un listado o copiar un archivo»*. Y es verdad en lo esencial: la regla es de
cuando el REPL y la VM eran el mismo hilo (Pico, `#256`); desde `A1` el hilo `io` lee el cable
durante todo el RUN.

**Lo que SÍ queda por medio, comprobado en el código (ninguna de arquitectura):**
1. **El cerrojo del cable es por LÍNEA, no por respuesta.** Un `GET_REPLY` son una línea y N bytes
   crudos en dos llamadas (`bpvm_repl.c:251`); el `tx_mtx` de `#473` hace atómica la línea. Durante un
   RUN, `io` saca los `OUTPUT` por el mismo cable y un `print` puede colarse **entre la cabecera y su
   bulk**, o dentro del bulk: el IDE leería basura como fichero. Arreglo: la respuesta entera (línea +
   bulk) bajo el mismo cerrojo, en el común.
2. **El cerrojo del FS abarca el `read_stream` entero** (`fs_lfs.c:320`): mientras `GET` manda 60 KB a
   11 KB/s (~5 s en la DK2), un `writeFile`/`readFile` del programa se queda parado. No es un
   bloqueo, es una parada — pero en `T1` cambia los tiempos que se miden. Salir de ahí es trocear con
   huecos (las N aperturas de `#453`) o aceptar la parada y decirlo.
3. Después, levantar el `BUSY` en los cinco `poll` **sólo para `LS`/`GET`**: `PUT` sobre `/app` con el
   programa leyendo ahí es otra conversación.

⏩ **Decisión de Eduardo (11-sep): a V7** — *«lo estudiamos en V7, no es para V6»*. Para `C1` no hace
falta: la captura se escribe cuando el programa quiere y se baja al terminar (o tras `KILL`). El
runner de `T1` lo sabe: `RUN → EXITED → GET`.

#### ✅ `#494` — EL GC DE LA miVM DESCARRILABA ANTE UN OBJETO DE PAYLOAD 0 — desde el 15-jul (abierta y CERRADA el 11-sep, `65e9f558`, de `G2`)

**Qué era.** `heapAlloc` reserva `max(MIN_FREE_BLOCK=12, align4(8 + payload))` desde `5ea01557`
(15-jul, *«el alocador regalaba la astilla»*) — pero **`objectTotalSize()` no clampaba**: para un
`""` o un array vacío contaba 8 B donde había 12. El recorrido de `buildValidObjectsSet` avanzaba 8,
aterrizaba en el relleno, leía **relleno como cabecera** y de ahí basura: el set de válidos salía
incompleto → objetos vivos sin marcar → barridos → `use-after-free` **mucho después y sin rastro**.
La VM-C lo tenía bien desde F2 (`block_total_size` clampa). **Casi dos meses con el GC de la VM de
referencia roto para cualquier programa que crease una cadena vacía.**

**Por qué no se vio.** El guardián del invariante (`[gc] !! HEAP INCONSISTENTE`) **lo cantaba por
stderr desde el primer día** — y el arnés de paridad hacía `2>/dev/null`: 57 PASS con el GC roto.
El caso exacto de *«errores sí, silenciosos no»* y de *«el instrumento necesita control»*: el
guardián estaba, la tubería lo tiraba. Se vio porque `GuiWinJson` murió con un UAF en miVM y no en
la VM-C, y la bisección llegó a un `var basura: string := ""` + 3000 concatenaciones.

**Arreglo.** El mismo clamp en `objectTotalSize` (objetos y arrays). Y el arnés vuelca esa línea de
stderr como salida (rompe la paridad y se ve en el diff). ⏭️ Queda por decidir si el guardián debe
ser **fatal** en vez de un aviso: si el recorrido se desincroniza, seguir es corromper.

#### 🔵 `#493` — un módulo de stdlib en `/app` TAPA al de `/lib`, y el usuario no se entera (abierta 11-sep, de `G2`)

**Medido en la Pico (11-sep)**: había `Collections.mod`, `Json.mod` y `Str.mod` **viejos** en `/app`
—restos de subidas a mano de otras sesiones— y `/app` (el directorio de trabajo) **se resuelve antes
que `/lib`**. `OwnerCascada` moría con `INVOKE_VIRTUAL sobre null` y **el arreglo no tenía nada que
ver**: cargaba la `Collections` vieja. Sólo se vio en el log: `dep 'Str' -> /app/Str.mod`.

📌 **Es la misma familia que la `Gui` rancia en la zona de packs de la P4**: una copia que sombrea a la
buena, y un síntoma que apunta a otro sitio. Y el linker **sí** grita en el desfase de métodos
(*«no exporta `Core.List#length#16`; ¿versión vieja?»*); **lo que no vigila es la sombra**.

⏭️ **Lo que falta es ruido, no lógica**: una línea por el `OUTPUT` cuando un módulo con **nombre de
stdlib** se resuelve desde `/app` en vez de `/lib` (o del embebido). El log ya lo sabe; que lo diga
donde el usuario mira. ⚠️ Y para el runner de `T1`: **comprobar que `/app` no tapa a `/lib` antes de
correr nada**, o los resultados en placa no significan lo que parecen. ¿V6 o V7? Es pequeño y es un
fallo silencioso — a decidir.

#### 🧬 `#491` — EL CURSOR: la fachada de FS no tiene descriptores, y por eso la misma enfermedad ha vuelto TRES veces → **V7** (abierta 10-sep, de `#473`)

🔎 **Y NO ES NUEVO: EL DISEÑO YA ESTABA DECIDIDO, Y SE PERDIÓ POR NO SER FICHA.** Eduardo lo recordaba
(*«se habló que los ficheros de BP fuesen objetos y que open/read/write/close trabajasen con
handles»*) y estaba: **`docs/V6_IDEAS.md:253`, del 17-ago**, bajo el título *«Ficheros: no hay `seek`
porque no hay `open`»* — y arrancó de una pregunta suya, la del `fseek`.

Allí hay **dos decisiones suyas, marcadas como DECIDIDO**:
1. *«Un fichero para BP debe ser una clase, con los métodos habituales: open, close, read, write,
   seek. Read tendrán que ser varios (readByte, readStr…) mientras que write se puede sobrecargar.»*
   Y **la variante SIN handles quedó explícitamente descartada** (`readFileAt(path, off, n)`).
2. *«Podemos tener un tipo `File` con las funciones básicas que trabaja a nivel de bytes. Después un
   2º tipo que herede del anterior y que trabaje con strings (UTF-8).»* — con la regla que lo deja
   limpio: **`TextFile` AÑADE sobrecargas, nunca redefine**, y lo formateado va por `print`.

📌 **El documento ya había visto lo que se volvió a descubrir el 10-sep**, y con estas palabras:
*«la trampa de `#398` desaparece sola cuando el fichero se abre UNA vez… **la apertura persistente ES
el handle**»*. Y sus dos preguntas abiertas son exactamente las que hacían falta: **¿qué pasa si no se
cierra?** (BP no tiene destructores; la red que ya existe es el guardián de fin de RUN de `#339`) y
**¿cuántos ficheros a la vez?** — la misma que se planteó de cero al abrir esta ficha.

⚠️ **CORRECCIÓN — dónde se quedó: dentro de `L1`, que se fue a V7 el 5-sep.** `docs/FICHAS.md:4572`
lo lista explícitamente entre lo que `L1` se lleva —*«destructores + `var owner`, **ficheros como
clase**, `Object` comodín, `Map` con objetos, el módulo `Time`»*—, con el criterio de que son
*«añadidos, no arreglos»*. Estaba pensado para V6 (`docs/ESTADO.md:3008`), pero **nunca tuvo ficha
propia**. Y no hay ni una línea de código: `grep TextFile` en stdlib, miVM, frontend y VM-C da cero,
ni un id de builtin reservado.

📌 **La lección es más pequeña de lo que yo dije, pero sigue siendo lección**: no se perdió —está en
`L1`— pero **una decisión sin ficha propia es invisible en la práctica**, porque *«¿qué hay abierto?»*
se contesta con la lista de fichas, no releyendo los hitos uno por uno. Por eso esta ficha existe.

🧱 **CÓMO SE CIERRAN LAS DOS PREGUNTAS ABIERTAS — propuesta de Eduardo (10-sep):** *«Si hacemos que un
fichero BP sea un objeto, éste se alojaría en el heap. Dentro del objeto puede haber un array de bytes
que sirva de buffer. Así no haría falta limitar el número de handlers de ficheros, y en caso de cuelgue
o error el GC acabaría recolectando el objeto.»*

Contesta de una vez a las dos que `V6_IDEAS.md:404-431` dejó sin cerrar, y **el terreno lo aguanta**:

- ✅ **El GC NO COMPACTA** (`src/heap.c:119`: *«F2 v1: no compacta (no mueve objetos)»*; `:364`:
  *«este GC no compacta, así que retener nunca corrompe»*). Es justo la condición que hacía falta: el
  driver del FS puede quedarse con el puntero al buffer entre llamadas sin que se le mueva debajo.
- ✅ **Y no rompe la regla de la casa.** `src/fs_lfs.c:86` exige *«CERO malloc en las ops de fichero
  (obligatorio en micro)»*, y por eso hoy hay **dos** buffers estáticos de 256 B — o sea que el límite
  real de littlefs son **DOS ficheros abiertos**, no el `FIL` de FatFs. Un array en el heap de la VM
  **no es malloc**: la regla sobrevive y el límite arbitrario desaparece.
- ✅ **El límite deja de ser un número inventado y pasa a ser el honesto**: la memoria — y visible,
  porque el guardián de fin de RUN (`#339`) ya dice quién se quedó qué.

🔓 **Y LO QUE CIERRA EL CÍRCULO: el destructor ya está diseñado, y esta propuesta DESARMA la única
objeción que tenía.** Eduardo, el mismo día: *«si el GC va a liberar el objeto y ejecuta el
destructor, éste puede encargarse de cerrar el fichero.»*

El diseño de destructores es suyo y es de **cuatro capas** (`docs/V5_IDEAS.md:2496`, 9-ago), con
`~Clase()` disparado por **fin de ámbito** —desazucarado a `try/finally`, **cero cambios en las VMs**—
y con las capas 3 y 4 (el agotamiento GRITA, y el fin de RUN suelta y CUENTA) como red. Sintaxis ya
decidida: `function ~Db()`, y verificado que `~` no se usa en ninguna fuente del frontend.

Dejó **una sola cosa abierta**: *«¿corre `~Clase()` si el objeto muere SIN `owner` ni bloque, recogido
por el GC?»*. Y la recomendación escrita era **que no**, con este argumento:

> *«el GC corre por presión de MEMORIA, y un descriptor o un statement son otro recurso escaso. Con 8
> descriptores agotados y el heap al 3 %, el GC no tiene motivo para pasar y el destructor no correría
> nunca.»*

🔑 **Ese argumento deja de aplicar si el buffer vive DENTRO del objeto**, y ahí está lo elegante de la
propuesta: si el recurso escaso **es** el heap, quedarse sin ficheros abiertos **es** presión de
memoria — así que el GC sí tiene motivo para pasar y el destructor sí corre. La objeción no se rebate:
**se disuelve**, porque desaparece la premisa.

📐 **De ahí sale una regla de diseño, y conviene fijarla**: **todo el estado por fichero va en el
objeto**, no sólo el buffer. Si una parte se queda en un `FIL` estático del driver, para esa parte la
objeción original sigue viva y se vuelve al mismo sitio. Con todo dentro, la contabilidad es honesta:
un fichero abierto cuesta heap, se ve, y se recupera por el mismo camino que todo lo demás.

⚠️ **Lo que la propuesta NO resuelve sola:**
1. **El GC hoy no avisa a nadie al recoger**: no hay finalizador ni gancho (`grep finaliz|destructor|
   on_collect` en `heap.c` e `bpvm_internal.h` → cero). Liberar la memoria **no cierra el fichero**:
   littlefs y FatFs guardan su propio estado. Hace falta que el GC llame al destructor al recoger —
   maquinaria nueva, pequeña y acotada, y **es la quinta capa** que el diseño de 9-ago dejó fuera a
   propósito.

   🔬 **«¿Y si el destructor lo hacemos como función NATIVA?» (Eduardo, 10-sep) — analizado, y sí:
   cambia el problema.** Las cuatro razones por las que un destructor en BP no puede correr dentro
   del GC se caen casi todas:

   | razón | ¿sobrevive siendo nativo? |
   |---|---|
   | hace falta el intérprete y un marco de pila | **no**: una función C/Java no lo necesita |
   | puede ASIGNAR memoria → reentra en el GC | **no**, si se prohíbe; y un `close()` no asigna |
   | puede RESUCITAR el objeto guardando `this` | **no**: un nativo no lo hace salvo que se escriba aposta |
   | puede reentrar el cerrojo de la VM | manejable: es una llamada hoja |

   ✅ **Y el sitio ya existe, con la información ya puesta.** `gc_sweep_phase` (`src/heap.c:564`) lee
   la cabecera de cada bloque muerto, y para un objeto **la segunda palabra ES el `class_ptr`**
   (`heap.c:7`, `:55`). Así que saber «este muerto es un `File`» es UNA lectura, no una búsqueda:
   basta un bit en el descriptor de clase. El tag además ya distingue objeto de array
   (`BPVM_TYPE_OBJECT`), así que los arrays ni se miran.

   📐 **Pero NO dentro del barrido: justo DESPUÉS.** El barrido recorre el heap linealmente
   fusionando huecos (`heap.c:564-600`), y cualquier cosa que asigne o mueva en mitad de ese paseo lo
   corrompe. Recoger las direcciones en una lista y vaciarla al terminar cuesta casi nada y elimina
   la clase entera de errores **sin tener que razonar sobre qué hace cada nativo** — que es
   precisamente lo que no se quiere estar comprobando en cada destructor nuevo.

   📌 **Tres condiciones para un destructor nativo**, y conviene que sean del contrato: **no asigna**,
   **no llama a código BP**, y **es IDEMPOTENTE** — porque las capas 1 y 2 pueden haberlo cerrado ya,
   y lo normal es que así sea.

   ⚠️ **Lo que ser nativo NO arregla, y es lo que hay que decidir: la PARIDAD.** Los dos GC corren en
   momentos distintos, así que *cuándo* se dispara el destructor **difiere entre miVM y la VM-C**. Para
   un `close()` eso es invisible en `stdout`… **salvo que el cierre vuelque escrituras pendientes**:
   entonces un programa que relea su propio fichero puede ver cosas distintas en el PC y en la placa.
   De ahí la regla que cierra el asunto, y que ya estaba en el diseño de las cuatro capas: **todo lo
   observable va por `try`/`owner`; el camino del GC es sólo la red para lo anormal.** Y miVM no puede
   apoyarse en `finalize()` de Java —deprecado en la 9, quitado en la 18—, así que el mecanismo se
   escribe explícito en las dos.
2. **La red no sustituye a `close()`**: el GC recoge cuando recoge, así que un fichero puede quedar
   abierto mucho después de ser inalcanzable — y en un micro eso importa (estado del driver, y en la SD
   la caché de escritura). Que es como Eduardo lo planteó: *«en caso de cuelgue o error»*.
3. **Aperturas duplicadas**: FatFs está con `FF_FS_LOCK 0`, sin control de abrir dos veces el mismo
   fichero. Sin límite de handles, N escritores sobre el mismo fichero no se quejan. Decisión aparte, y
   de corrección, no de memoria.

📏 **PUNTO 4 DE `#473`: MEDIDO Y RESUELTO** (10-sep, en la Metro con su SD de 32 GB), con su control:

| | 120 KB | 480 KB | caudal |
|---|---|---|---|
| **littlefs** (`/app`, **con** `read_stream`) — el CONTROL | **212 ms** | — | 566 KB/s |
| **FAT** (`/sd`, **sin** `read_stream`) | **1368 ms** | **6040 ms** | ~80-88 KB/s |

**Es 6,4× más lento, pero NO está roto**: 1,4 s para 120 KB, lejos de los 10 s del timeout del IDE. Y
**no es cuadrático** — 4× el tamaño da **4,4×** el tiempo, no 16×. La medida prestada de `#453` (que sí
reventaba los 10 s) era de **littlefs en un S3**: otro backend y otro chip. ⏭️ **El punto 4 no entra en
V6: se queda aquí**, y desaparece solo el día que exista el cursor.

⚠️ **Y la primera medida fue INVÁLIDA — lo dijo el control**: el `GET` de littlefs, que es el caso
bueno, salía peor que el sospechoso. El fallo era mío: `cmd_get` usaba `esperar()`, que trocea por
líneas y **se comía el bulk crudo**. Sin control, habría publicado que las dos zonas son igual de
lentas. Arreglado leyendo el payload a pelo, y el porqué queda en la docstring de la herramienta.

🩹 **Hay una CUARTA recaída, y es de ESCRITURA**: la ficha `#475` dice *«no existe escritura en
streaming: sólo hay `read_stream`. Escribir por bandas con la fachada de hoy son N commits, no uno —
el patrón que costó 45× en `#398`»*. La enfermedad ya ha salido por los dos lados.

⏭️ **Así que esta ficha ABSORBE aquel diseño** y son la misma cosa por los dos lados: `File`/`TextFile`
es la mitad de arriba (superficie BP) y el cursor de la fachada es la de abajo. La de abajo hace falta
igual: hoy las 17 ops del backend reciben ruta, así que un `File.seek()` no tendría dónde apoyarse.

**Sale del punto 4 de `#473`** —que `read_stream` no existe en el backend FAT, así que bajar un
fichero grande de `/sd` cuesta cuadrático— y lo reencuadra **Eduardo**: *«Esto ya se habló cuando te
pedí un comando `fseek`. La idea es tener un cursor y poder hacer desplazamientos relativos.»*

**Y con el historial delante, tiene la forma de una enfermedad que ya ha recaído dos veces:**

| | quién sufría | qué se hizo |
|---|---|---|
| `#398`/`#424` | `crc32` reabría el fichero cada 256 B — **5432 aperturas para 1,3 MB** | se añadió `crc32` al interfaz de backend: abre una vez. 16,5× medido |
| `#453` | el `GET` reabría cada 256 B — **472 aperturas para 120 KB** | se añadió `read_stream`: abre una vez. Sólo en littlefs |
| **hoy** | el `GET` desde `/sd` | se propone añadir `read_stream` **también** a FAT |

Tres veces el mismo síntoma y tres arreglos **por llamador**, no por causa. Y la causa estaba escrita
desde `#453`, en la cabecera del propio arreglo: *«el backend es el único que puede hacerlo, **la
fachada no tiene descriptores**»*.

📐 **Medido, no supuesto**: de las 17 operaciones del interfaz de backend
(`include/bpvm_fs.h`), **16 reciben un `path`**. No hay handle, ni posición, ni nada abierto — ni en
la fachada, ni en el backend, ni en BP. Y **tres de esas 17 existen sólo para esquivar la ausencia de
cursor**: `read_at`, `crc32` y `read_stream`. Cada una se añadió el día que un llamador concreto se
puso demasiado lento.

🔑 **Y lo que de verdad falta no es velocidad, es una CAPACIDAD.** `readFile` hace `stat` y se trae el
fichero **entero** al heap (`builtins.c`, `BUILTIN_READ_FILE`). O sea que hoy, desde BP, **un fichero
más grande que la RAM no se puede leer**: un CSV de 1 MB en la SD es ilegible en una placa con 271 KB
de heap. Eso no lo arregla ningún `read_stream` más — lo arregla el cursor.

⏭️ **Lo que hay que decidir en V7** (aquí no se diseña nada todavía):
- El cursor en la **fachada**: abrir / desplazar (absoluto y **relativo**, que es lo que pide
  Eduardo) / leer / escribir / cerrar. Con eso, `read_at`, `crc32` y `read_stream` dejan de ser
  operaciones especiales y pasan a ser bucles sobre el cursor.
- El cursor en **BP**: el `fseek` que pidió Eduardo, o sea un fichero abierto como objeto. Eso **es
  superficie de lenguaje**, así que es V7 por definición.
- ⚠️ Y una pregunta que hay que contestar antes de escribir código: **cuántos descriptores caben**.
  Un handle abierto es RAM en el micro, y FatFs quiere su `FIL` (~550 B) por fichero. Que el modelo
  sea «uno por programa», «N con límite declarado» o «uno por hilo BP» cambia el diseño entero.

📌 **Relación con el punto 4 de `#473`**: son la solución estrecha y la ancha del mismo problema.
La estrecha —`read_stream` en FAT, gemelo exacto de `fat_crc32`— sigue siendo válida para V6 **si la
medida la justifica**, y no estorba al cursor: el día que exista, desaparece con las otras dos.

#### 🧬 `#490` — BAJO CONSUMO: el estudio preliminar → **V7** (abierta 10-sep, decidida a V7 el mismo día)

**La pregunta es de Eduardo, y el encuadre también**: *«cuando un micro no hace nada, porque no tiene
nada que hacer, en un micro con alimentación da igual lo que haga, pero si fuese un micro que
funciona con baterías lo ideal es que se durmiese. ¿Cómo está ahora?»*

Y acto seguido, la frase que decide el diseño entero: 🔑 ***«casi todos los micros tienen un modo de
bajo consumo; lo difícil no es entrar en él, lo difícil es programar qué es lo que lo hace salir»***.

**Por qué eso lo cambia todo.** Entrar es una instrucción (`WFI`, `esp_light_sleep_start`, el Stop
del U5). Salir es un **contrato**: hay que declarar *de antemano* qué evento despierta, y ese
contrato es lo que no existe hoy en BP. Un `Machine.sleep()` a secas sería un cuelgue con otro
nombre. Las preguntas reales son de lenguaje, no de silicio:
- ¿Quién declara las fuentes de despertar — el programa, o la placa?
- ¿Qué pasa con los hilos BP dormidos: el `sleep(200)` de un hilo, ¿es una fuente de despertar?
- ¿Y el wire? Si el IDE está conectado, la placa **no puede** dormirse sin perder la conexión; si no
  lo está, sí. Eso hace que el modo dependa de si hay alguien mirando.
- ¿Y la GUI? Un panel encendido consume más que el core; dormir el micro con la pantalla viva no
  ahorra gran cosa.

📌 **Es la misma forma que `#479` (el mapa de pines) y que los arrays de buses de `#470`**: qué pines
y qué periféricos pueden despertar, y qué se conserva en cada modo, es **verdad de silicio** — va en
la cintura de cada micro y viaja compilada en su imagen, no en el ENV. Conviene resolverlas juntas.

**Lo que se sabe ya, y es poco**: no hay ni una línea escrita sobre consumo en todo `docs/`. Y hay un
dato duro, medido el 10-sep por otro motivo (`#473`/R16): **en el STM32 la tarea ociosa no corre
nunca** —su lazo del wire gira con `continue` sin ceder y la tarea `vm` va a `tskIDLE_PRIORITY+2`—,
así que esa familia ni siquiera llega al escalón previo de poder dormir. Y es la familia de bajo
consumo de ST, o sea el peor sitio donde tener eso.

⚠️ **Los tres escalones que se confunden**, y hay que mantenerlos separados en todo el estudio:
(a) el hilo cede la CPU · (b) la tarea ociosa corre · (c) el **micro** entra en bajo consumo.
Sólo (c) se nota en el amperímetro. Hoy la Pico y el ESP32 llegan a (a) con seguridad; (b) y (c)
están por censar.

🎯 **POR DÓNDE EMPEZAR — decidido por Eduardo el 10-sep:** *«No creo que haya una solución común
para todos los casos, pero podemos empezar por algo sencillo e ir creciendo. Empezar por el timer o
el RTC y programar un sleep que despierte cada cierto tiempo, y si no hay nada se vuelva a dormir.»*

**Por qué es el punto de partida correcto, y no un atajo.** Convierte el problema difícil —*declarar
qué despierta*— en uno fácil: *mirar cada cierto tiempo*. Con una sola fuente de despertar (el
temporizador) no hay contrato que diseñar, no hay mapa de pines que resolver antes, y no hay que
tocar el lenguaje. La parte difícil no se resuelve: **se aplaza a propósito**, y eso permite que
haya algo funcionando antes de decidir lo demás.

⚠️ **El precio, y choca de frente con `#462`.** Despertar cada N ms significa que un evento puede
llegar hasta N ms tarde. Esta misma sesión bajamos el suelo de descubrimiento de un evento de 48 ms
a **7 ms**, medido en la C6; un periodo de dormir de 100 ms lo devolvería a ~100 ms. Así que **el
periodo no es una constante: es una política**, y quién la elige (el programa, la placa, o el modo
en que esté) es ya una pregunta de lenguaje. La forma sana de decirlo: *dormir es cambiar latencia
por consumo*, y el que sabe cuánta latencia se puede permitir es el programa.

📌 **«Si no hay nada» hay que definirlo, y la lista es reveladora.** Al despertar hay que mirar:
¿hay bytes en el wire? ¿hay algún hilo BP listo para correr? ¿ha vencido algún `sleep` de hilo?
¿hay un evento en la cola? **Es exactamente la misma lista que las fuentes de despertar**, sólo que
sondeada en vez de por interrupción — que es justo por lo que este camino es más barato de empezar,
y también por lo que crece de forma natural: cada elemento de esa lista que se convierta en
interrupción real es un paso más, sin rehacer lo anterior.

📌 **Timer y RTC NO son intercambiables**, y ésta es de las primeras cosas que el estudio tiene que
poner en tabla: qué reloj sobrevive a qué modo de bajo consumo es **verdad de silicio y distinta por
micro**. Un temporizador general suele morir en los modos profundos; el RTC suele sobrevivir, con
menos resolución. Elegir uno u otro fija de rebote **hasta dónde se puede dormir**.

📌 **Y hay que mirar lo que FreeRTOS ya trae hecho**: `configUSE_TICKLESS_IDLE` es literalmente este
patrón —«ninguna tarea estará lista en N ticks → paro el tick, bajo consumo, programo el despertar»—
implementado por el RTOS. La primera pregunta del estudio no es *cómo lo hacemos*, sino **si eso
está encendido y por qué hoy no sirve**. La sospecha, con lo medido el 10-sep: no llega a ejecutarse,
porque la capa de encima (el lazo del wire, y el scheduler de la VM) no cede el turno a la ociosa.

🪜 **LA ESCALERA — Eduardo, 10-sep:** *«Lo de dormir se puede hacer progresivo: primero pausas
cortas y, según se va avanzando sin actividad, pausas más largas.»*

**Esto deshace la tensión con la latencia, y la deshace en el sitio correcto.** El problema del
periodo fijo era elegir N: corto gasta, largo tarda. Con la escalera **no hay N que elegir** — el
sistema se calibra solo, y el coste cae donde menos se paga: justo después de actividad (que es
cuando es más probable que venga más) la pausa es corta y la latencia sigue siendo la de hoy; tras
un rato largo de silencio (cuando es menos probable que venga nada) la pausa crece. Se paga latencia
sólo en el caso en que casi nunca hay nadie esperándola.

⚠️ **El caso malo, dicho sin adornos: el PRIMER evento tras un silencio largo** — que es exactamente
la persona que deja el aparato quieto y vuelve a tocar un botón. La escalera empeora justo ése. De
ahí que **el tope de la escalera sea una decisión de producto, no técnica**: un cacharro con pantalla
no debería pasar de unas decenas de ms porque un toque se notaría lento; un registrador de datos
puede irse a segundos. Y el escape es limpio: **una fuente que sea interrupción de verdad se salta el
tope entero** (un táctil con línea de IRQ despierta al instante, esté la escalera donde esté). O sea
que subir el tope y convertir fuentes en interrupciones son la misma mejora vista desde dos lados.

🔑 **Y lo importante: media escalera YA ESTÁ CONSTRUIDA, y no la habíamos visto así.** Al cerrar
`#462` el bombeo de la GUI dejó de dormir por su cuenta y pasó a **devolver cuánto tiempo dice LVGL
que puede estar ocioso** (`lv_timer_handler()`), topado a `BPVM_GUI_OCIO_MAX_MS` = 10 ms
(`src/gui_display_sdl.c:340,382`), y es el lazo BP quien duerme eso (`bpstdlib/Gui.bp:850-853`). Eso
no es una pausa fija: es **preguntarle al que sabe**.

Con eso a la vista, las dos formas de decidir cuánto dormir se ven claras, y **se componen**:
- **Preguntar** — el que sabe, contesta. LVGL ya lo hace. `configUSE_TICKLESS_IDLE` de FreeRTOS hace
  lo mismo con las tareas: sabe cuándo vence el próximo `vTaskDelay`.
- **La escalera** — el respaldo para lo que **nadie puede contestar**: cuándo llegará el próximo byte
  del wire, o el próximo toque. Ahí no hay a quién preguntar, y la progresión es la respuesta.

⏭️ Así que la pausa de cada vuelta sale de un `min()`: lo que digan todos los que saben, contra el
peldaño actual de la escalera. La escalera no sustituye a nada de lo que ya hay — **rellena el hueco
que queda**.

🧭 **LOS DOS CASOS — Eduardo, 10-sep:** *«Tal como yo lo veo hay 2 casos: 1 — el usuario indica
expresamente que no va a hacer nada; 2 — el usuario no está prestando atención y la tarea se ha
terminado.»*

**Y no son dos formas de detectar lo mismo: son dos regímenes distintos, porque cambia QUIÉN ACEPTA
LAS CONSECUENCIAS.** Ahí está la utilidad de partirlo así:

| | **caso 1 — declarado** | **caso 2 — inferido** |
|---|---|---|
| quién decide | el programa, con un verbo | el sistema, solo |
| quién acepta el coste | **el programador** | **nadie** |
| qué se puede apagar | mucho: periféricos, relojes, incluso perder el wire — porque alguien lo pidió | sólo lo que se pueda recuperar **como si no hubiera pasado nada** |
| profundidad | hasta donde diga el verbo | topada por la transparencia |
| latencia al volver | asumida | no se puede notar |

🔑 **La consecuencia práctica, y es la que más ordena el diseño: el caso 2 tiene que ser
TRANSPARENTE.** Si nadie ha pedido dormir, nadie ha aceptado perder nada — así que el caso 2 **no
puede tirar la conexión del wire** (el IDE podría estar enganchado), ni perder un evento, ni
retrasar un `sleep` de un hilo BP. Es una optimización invisible o no es. El caso 1 sí puede tirar el
wire, **porque el programa lo ha pedido**.

📌 **No son alternativas, se componen**: el caso 2 es el **suelo** —siempre puesto, automático,
invisible, y es exactamente donde encaja la escalera— y el caso 1 es la **salida explícita** para
cuando el programa sabe más de lo que el sistema puede inferir («voy a estar 10 minutos sin hacer
nada, despiértame con el botón»).

⚠️ **La trampa del caso 2, dicha entera: «el usuario no está prestando atención» NO es observable
por el micro.** Lo que el micro puede observar es *no hay nada pendiente ni nada que venza pronto*.
No es lo mismo: un registrador de datos sin ningún usuario delante está *siempre* desatendido y no
debe dormirse a través de su siguiente muestreo. Así que el disparador del caso 2 no es «nadie mira»
sino **«nada pendiente Y nada que venza antes de que yo despierte»** — que es, otra vez, la misma
lista de antes (wire, hilos BP listos, `sleep` que vence, eventos en cola), ahora usada para decidir
*cuánto* dormir y no sólo *si* dormir.

📊 **EL CENSO, HECHO (10-sep) → `docs/BAJO_CONSUMO_CENSO.md`.** Seis lecturas + refutación
adversarial (**57 de 73 hallazgos vivos**, 16 tumbados) + crítico de completitud. **Sin amperímetro:
ni un mA en él, a propósito.** El titular:

> 🔴 **Ninguna de las siete placas llega a dormir. Ni una. No hay una sola línea en el árbol que meta
> a un micro en un modo de bajo consumo.**

Con los cuatro escalones separados, que es como hay que leerlo:

| familia | el hilo cede | la ociosa corre | el **núcleo** se para (`WFI`) | el **micro** baja consumo |
|---|---|---|---|---|
| **RP2350** (Pico 2 / Metro) | sí | sí | **no** | **no** |
| **ESP32** (S3, C3, C6, P4) | sí | sí | **sí** (`waiti`/`wfi`) | **no** |
| **STM32U5** (DK2 y Nucleo) | **no** | **no** | **no** | **no** |

Verificado **contra el artefacto**, no leyendo: `wfi` = 0 en el desensamblado de la Pico
(82 898 líneas) y en los `.list` de la DK2 y la Nucleo. En los ESP el núcleo sí se para porque el
kernel de IDF llama **siempre** a su idle hook.

🔑 **Y el hallazgo que va contra el punto de partida elegido: el RTC de hardware SÓLO tiene backend
en el STM32** (`stm32/port/gpio_stm32.c:790`, `:1009`). El ESP usa a propósito el stub portable
(`esp32/common/gpio_esp32.c:595`) y la Pico no registra ninguno. **El reloj que se propone como
despertador existe justo en la familia que hoy no llega ni a ejecutar su tarea ociosa, y no existe en
las otras cuatro.** Empezar por el RTC significa construirlo en tres familias antes de poder probar
nada; empezar por el timer, no.

🔑 **Y el que cambia el encuadre entero: miVM YA HACE LO CORRECTO.** Calcula
`earliestSleepWakeMs()` y espera **el delta completo** en `vmLock.wait(delta)`, sin tope, porque
quien llega lo despierta con un `notify` (`VirtualMachine.java:543`, `:2305-2317`). O sea que el
patrón *«duerme hasta el próximo vencimiento y que un evento te despierte»* **ya está escrito en la
implementación de referencia**, y el tope de 50 ms de `src/scheduler.c:144` es una limitación de la
VM-C, no del diseño. ⚠️ Y eso significa que tocarlo **roza el invariante dual-VM**.

**Lo demás que salió y que conviene tener a mano** (el detalle, en el documento):
- ⏰ **El techo del sueño lo pone el reloj de sondeo más rápido**, y hoy con un programa cargado son
  el hilo `io` a 5 ms y el tope de 50 ms del planificador — que además **ya no lo usa nadie** durante
  un RUN con `io`. Aunque todo lo demás estuviera encendido, **ningún micro podría dormir más de un
  tick seguido**.
- ⏰ **Los ESP tienen un límite duro que nadie había censado como tal**: el Interrupt WDT por
  hardware a **300 ms** en las cuatro imágenes. La Pico sí para su watchdog de verdad; el STM32 hoy
  no registra backend a propósito.
- 🔋 **El caso real de una placa a pilas —autorun sin IDE— no estaba censado.** Existe en las tres
  familias, y es el modo en que vive un aparato con batería: arranca solo y **no hay nadie al otro
  lado del cable**. Los 200 sondeos/s del hilo `io` se pagan íntegros para atender un `KILL` que no
  puede llegar.
- 🔌 **Cargas estáticas que no dependen de la CPU y que no apaga nadie**: en la DK2 se inicializan
  **19 periféricos incondicionalmente** en el arranque (cinco UART, tres SPI, el PHY del USB HS…) y
  16 en la Nucleo; el **LTDC escanea el panel desde el arranque** corra o no una GUI; el NeoPixel de
  la Metro se enciende verde tenue en el arranque *(un `test H7.4.a` que se quedó)* y no lo apaga
  nadie. Contraste que prueba que no es inevitable: en la Pico el ADC es perezoso.
- 🐛 **`sleepUs()` es espera activa pura** y su nombre no lo delata: `sleepUs(500000)` son 0,5 s de
  micro a plena carga sin ceder ni el hilo BP ni la CPU.
- 🐛 **`Machine.MIN_CPU_MHZ()` declara 18 MHz para las cinco familias y sólo la Pico lo implementa**
  (y bien: escala Vdd_core por tabla). El STM32 y el ESP devuelven 0. Es la palanca que Eduardo
  recordaba de las pruebas de overclocking, y está declarada donde no existe.
- ⚠️ **Un consumo invisible en el parque**: `Gui.mod` no viaja en el blob embebido y el IDE no
  reinstala la stdlib en cada Run, así que **cualquier placa cuya stdlib no se haya reinstalado tras
  `a5a00daa` tiene el lazo de GUI viejo, que no tiene pausa ninguna y gira a tope**. El síntoma no es
  un fallo: es consumo.

🔧 **EL DATO DE HARDWARE QUE CORRIGE EL VOCABULARIO (Eduardo, 10-sep).** *«El RP2350 no tiene un
RTC dedicado en su hardware, a diferencia del RP2040. Lo que incluye es un temporizador AON
(Always-On): un contador de 64 bits en el dominio de energía siempre encendido que mide
milisegundos, y sirve para programar eventos de encendido. Si una placa RP2350 comercial trae RTC,
es por un chip externo añadido en la placa (tipo PCF85063), no por una función nativa del micro.»*

**Esto explica el hallazgo del censo en vez de contradecirlo**: que la Pico no registre backend de
RTC no es un olvido — **es que no hay nada nativo que registrar**.

🔑 **Y cambia cómo hay que llamar a la pieza.** Lo que hace falta no es un *reloj de calendario*: es
**un contador en el dominio siempre encendido capaz de generar un evento de despertar**. Eso lo
tienen todas las familias, con nombres distintos:

| familia | la pieza | ¿la alcanzamos hoy? |
|---|---|---|
| **RP2350** | **AON Timer**, 64 bits en ms, dentro de POWMAN | **no**: no enlazamos `hardware_powman` ni `pico_sleep` (`pico/CMakeLists.txt:195-213`, 12 librerías y ninguna es ésa) |
| **STM32U5** | RTC de hardware con dominio de respaldo (y además LPTIM) | **sí**, es el único con backend (`gpio_stm32.c:790`, `:1009`) |
| **ESP32-S3 / C3 / C6** | el temporizador del dominio RTC (`esp_sleep_enable_timer_wakeup`) | **no**: usa el stub portable a propósito (`gpio_esp32.c:595`), y PM está apagado |
| **ESP32-P4** | **igual, pero con más silicio**: controlador RTC con lógica de wake-up para deep-sleep **y dominio VBAT** con respaldo por batería y cristal externo de 32,768 kHz | **no**, y con matiz — ver abajo |

🔬 **EL P4 NO ES «UN ESP32» MÁS EN ESTO (dato de Eduardo, 10-sep, comprobado en nuestras imágenes).**
El P4 integra controlador RTC con lógica de activación para deep-sleep **y un dominio VBAT** con
entrada para cristal de 32,768 kHz y conmutación de energía, de modo que la hora sobrevive a un corte
de la alimentación principal. *(Su aviso, y hay que mantenerlo escrito: el soporte de ESP-IDF para el
mantenimiento avanzado con VBAT sigue en pruebas.)*

Lo que dicen **nuestros** `sdkconfig`, que es la pregunta útil:

| | S3 | C3 | C6 | **P4** |
|---|---|---|---|---|
| `CONFIG_SOC_VBAT_SUPPORTED` | — | — | — | **`y`** |
| `CONFIG_ESP_VBAT_INIT_AUTO` | — | — | — | **not set** |
| fuente del reloj RTC | `INT_RC` | `INT_RC` | `INT_RC` | **`INT_RC`** |

O sea: **el silicio del P4 lo soporta, IDF lo sabe (`SOC_VBAT_SUPPORTED=y`) y nuestra imagen no lo
usa** — y las cuatro, el P4 incluido, corren el RTC sobre el **oscilador RC interno**, no sobre un
cristal.

🔑 **Y eso separa limpiamente los dos usos, que hasta ahora iban juntos:**
- **Como DESPERTADOR** («despiértame dentro de N ms»): el RC interno **basta**. Que derive un 5 % con
  la temperatura da igual cuando lo que se pide es volver a mirar dentro de un rato. Para el caso 2
  —la escalera— no hace falta cristal ni VBAT.
- **Como CALENDARIO** («qué hora es tras un corte»): ahí **sí** hacen falta cristal y VBAT, y eso es
  del P4 y del STM32, no de los otros tres.

Así que el punto de partida no se mueve: **empezar por el despertador, que se puede en todas**, y
dejar el calendario como una capacidad que unas placas tienen y otras no —y que, como todo lo de
`#479`, hay que poder **preguntar** en vez de suponer.

❓ **No determinado, y es de placa, no de micro**: si la Waveshare P4 que usamos tiene realmente
cableados el cristal de 32,768 kHz y la entrada VBAT. Eso no sale del código: sale del esquemático o
del multímetro.

⚠️ **Un RTC externo NO es un despertador.** Un PCF85063 por I2C es un **driver de placa**, no silicio:
da la hora, pero **sólo despierta al micro si su línea INT está cableada a un pin capaz de
despertar**. Tenerlo no da la fuente de despertar; es una pregunta de la placa concreta, no del
micro. Es exactamente la distinción de `#479`: hay verdades de silicio y verdades de placa, y viven
en sitios distintos.

📌 **Y una consecuencia que no es de consumo pero está aquí al lado**: en la Pico y el ESP, `Rtc` es
hoy **un offset por software sobre el reloj monotónico** (`src/rtc.c:28`). Muere en cada reset, y el
programa **no tiene forma de saber si la hora es real o inventada** — la misma forma que `#469`.
Anotado en la cabecera de `src/rtc.c`, que además decía que el backend lo registraba la Pico: era
falso, lo registra el STM32.

⏭️ **V7, y es un ESTUDIO, no una implementación.** Lo que falta: **sin amperímetro no hay números** —
decir qué hace el código es gratis, decir cuántos mA necesita instrumento. Y hay una medida que
**no** necesita amperímetro y tampoco se tiene: **cuánto cuesta hoy una vuelta en reposo**. El único
número que había (0,4 ms/vuelta) es del P4, de antes de `#462`, medía sólo `lv_timer_handler`, y su
propia revisión dice que no se puede extender a otra placa. Sin línea base, la escalera no tiene
contra qué compararse.

#### ✅ `#489` — ~~la cola del GUI de la VM-C descarta EN SILENCIO cuando se llena~~ (abierta 10-sep · **CERRADA el 10-sep**)

**Salió contestando a Eduardo** sobre cuánto tarda una tecla del teclado virtual en procesarse:

```c
int nt = (g_ev_tail + 1) % GUI_MAX_NODES;
if (nt == g_ev_head) return;      /* cola llena: descarta (como offer() de miVM) */
```
`src/gui.c:1129-1130` — **512 huecos** (`GUI_MAX_NODES`), y al llenarse el evento **se tira sin log,
sin contador y sin nada**. Un clic o una tecla perdidos, en el **camino de entrada**, que es el peor
sitio posible para un fallo mudo.

⚠️ **Y hay asimetría entre las dos VMs**: miVM usa una `LinkedBlockingQueue` **sin límite**
(`GuiBackend.java:101`), así que ahí no se pierde nada. El mismo programa **no pierde eventos en el
PC y sí en la placa** — y en silencio. Ojo: el comentario del código dice *«como offer() de miVM»*,
o sea que la intención era imitarla; lo que imita es la llamada, no el comportamiento.

📐 **Es difícil de provocar pero no imposible**, y se sabe por qué: el techo de drenaje del lazo del
GUI es **una vuelta cada ~10 ms** (`BPVM_GUI_OCIO_MAX_MS`), así que algo que dispare rápido —un
slider arrastrado, un `onChange` por cada tecla, un sensor a 200 Hz— puede meter eventos más rápido
de lo que salen. 512 da mucho margen, pero el margen no es el punto: **el silencio lo es**.

⏭️ **El arreglo es el que ya está escrito en la casa**: contar los descartes y DECIRLO, igual que el
`omitted` del listado (`#425`) o el aviso de la cola de eventos BP, que miVM ya emite
(*«evento descartado (tid=…)»*, `VirtualMachine.java:4318`). No hace falta agrandar la cola: hace
falta que se entere alguien.

📌 De la misma familia que las cinco roturas del 9-sep: **el fallo existe, lo que falta es el ruido.**

✅ **HECHO (10-sep).** No se agrandó la cola: **se le puso voz**, que es lo que faltaba.

- El **primero se dice en el acto** (para enterarse ya) y el **total al terminar el programa**, en
  `bpvm_gui_reset` — así un slider arrastrado no inunda el diagnóstico con 500 líneas.
- Va por **`bpvm_diag`**, que sale por **stderr**: el `stdout` no se toca y **la paridad dual-VM
  queda intacta**. Comprobado: `compat.sh check` → **54 PASS, 0 FAIL**.
- Copia el patrón que ya estaba escrito al lado, en la cola de eventos BP (`src/events.c:34-40`),
  incluido su porqué: *«un evento perdido en silencio es de los que cuesta una tarde encontrar»*.

🧪 **Y se FUERZA el caso, porque un camino ejecutado no es un camino probado.** El nuevo
`test/test_guidrop.c` (`make test-guidrop`) inyecta **600 eventos en 512 huecos sin drenar** — que
con un programa real pide un slider arrastrado o un sensor rápido, y por eso el fallo llevaba ahí sin
que nadie lo viera. Salida:

```
[gui] cola de eventos llena (512): evento descartado (kind=0) — se cuentan los demas
[gui] 89 eventos de entrada descartados en esta ejecucion (cola de 512)
inyectados=600  recuperados=511  perdidos=89
PASS
```
El test comprueba además que **el segundo `reset` calla**: el contador se pone a cero, no se arrastra
de una ejecución a la siguiente.

📌 **Verificado en el artefacto, no en el log**: build headless de la Discovery, **0 errores 0
avisos**, y las dos cadenas nuevas aparecen en el `.elf`. `gui.c` es común, así que con una familia
basta (criterio de Eduardo, 8-sep).

📌 **La asimetría con miVM se queda, y a propósito**: su cola no tiene límite, así que en el PC no se
pierde nada. Igualar hacia abajo —ponerle tope a miVM— sería perder eventos donde hoy no se pierden.
Lo que se ha arreglado es que la placa **lo diga**.

#### 🧊 `#488` — el AOT no genera código XTENSA, así que el S3 no puede ejecutar nada nativo (abierta 9-sep, sale de `#482` · 🧊 **FUERA DEL PLAN DE VERSIONES el mismo día**: *«de momento no»*)

**El hueco, con los números delante**: el `.mdn` **ya tiene etiqueta** para Xtensa
(`MDN_ARCH_XTENSA = 94`, `mdn_format.h:76`) y el cargador la comprobaría; lo que no hay es
**generador**. El AOT emite ARM Thumb-2 (`MDN_ARCH_ARM`) y RISC-V (`MDN_ARCH_RISCV`), y nada más.

**Consecuencia**: en el ESP32-S3 —la única familia Xtensa del sobre— **todo se ejecuta
interpretado**, y su `esp32/README.md` ya lo dice: *«AOT no aplica aquí»*. No es un bug: es una
pieza que no está.

⏭️ **Versión sin decidir, y no parece de V6**: un backend AOT nuevo es del tamaño de `H4` (el de
RISC-V), no de un pendiente de cierre. Además el S3 está *«por popularidad, no por prioridad»*, y
Espressif también va a RISC-V — así que puede que la respuesta correcta sea no hacerlo nunca y
**declararlo** en la documentación, que es trabajo de `D1`.

### 🧊 9-sep — DECIDIDO: de momento NO, y no por falta de herramienta (Eduardo)

> *«Lo de generar código Xtensa depende de varias cosas. Lo primero es que gcc lo soporte (parece
> que sí) pero lo segundo es nuestra prioridad, y no es prioritario. Pendiente de decidir si se hace
> o no en un futuro, de momento no.»*

✅ **La primera mitad queda comprobada, para que no haya que volver a mirarla**: el compilador
**está instalado en esta máquina**, no hay que buscarlo ni construirlo —

```
C:\Users\Eduardo\.espressif\tools\xtensa-esp-elf\esp-13.2.0_20240530\xtensa-esp-elf\bin\
    xtensa-esp-elf-gcc.exe   (crosstool-NG esp-13.2.0_20240530) 13.2.0
```

Viene con el ESP-IDF, que ya se usa para compilar el firmware del S3. O sea que **la herramienta no
es el obstáculo**: el obstáculo es la prioridad.

🧊 **Y por tanto esto sale del plan de versiones**, como `A4`: no es un aplazamiento con fecha, es
que hay cola por delante. **De momento no**, y si algún día se decide que sí, el punto de partida
está escrito arriba: hay etiqueta (`MDN_ARCH_XTENSA = 94`), hay cargador que la comprobaría, hay
toolchain, y falta el generador.

⏭️ **Lo que SÍ hay que hacer mientras tanto, y es de `D1`**: que la documentación diga en voz alta
que en el ESP32-S3 **todo se ejecuta interpretado**. Hoy sólo lo dice `esp32/README.md`, que es un
fichero interno — el usuario no lo lee.

#### ✅ `#483` — un estado persistente deja la METRO sin poder ejecutar NADA, y sólo lo cura reparticionar (abierta 9-sep · **visto el 21-ago** · **CERRADA el 9-sep por NO REPRODUCIBLE**)

**Sin causa identificada.** Se ficha con la cronología porque el rodeo no es evidente y a un usuario
le puede pasar. El detalle completo está en el archivo de la cola heredada de V5, en esta misma
libreta; lo esencial:

📐 **El síntoma**: no corre **ningún** programa. Ni los de la sesión, ni el `/app/Hello.mod` que
preinstala el propio firmware —que no usa strings, ni builtins, ni un solo `import`—. Se cuelga
**mudo**, sin línea de error.

🔍 **Lo que DESCARTA el firmware, y es el dato que más vale**: se reflasheó la imagen del día
anterior —la misma que esa mañana había corrido `ListGets`, `SqlDemo`, `DaoDemo`, `SdCard` y `Bench`
sin una queja— **y tampoco funcionaba**. Lo roto **sobrevive al flasheo**: es FS, ENV o zona de
packs, no la imagen.

⚠️ **Y formatear NO bastó** (FS + zona de packs). Lo único que lo curó fue **cambiar el tamaño de la
partición**, que fuerza a rehacer el reparto entero.

🔬 **LO QUE FALTA MEDIR, y no se capturó**: el log **durante el intento de ejecución**. Si sale la
línea `RUN/v1 /app/X.mod session=N`, el programa llega a lanzarse y se atasca dentro; si no sale, no
llega ni a arrancar. Eso parte el problema en dos y sin ello sólo se puede especular. **Si vuelve a
pasar, lo PRIMERO es ese log.**


### 🟢 9-sep — NO SE REPRODUCE con la imagen actual (Eduardo, en placa)

> *«Estoy probando la Metro con la última imagen y a mí me funciona.»*

```
[Explorer] la placa ejecuta nativo arm (arch=40)
--- RUN /app/T.mod on Placa (serial v1) ---
Hola mundo
--- VM finished: exit 0 (OK) en 2 ms ---
--- RUN /app/Bench.mod on Placa (serial v1) ---
fib(28) interp =  317811  in  8635  ms
fib(28) AOT    =  317811  in  86  ms
--- VM finished: exit 0 (OK) en 8724 ms ---
```

**Y no es sólo «arranca»**: corre un `Bench` con AOT nativo. De propina, esto verifica en la Metro
algo que no lo estaba — **el AOT ARM en esa placa**: `arch=40` y **×100** (8635 ms → 86 ms).

⚠️ **Lo que esto NO dice, y conviene dejarlo escrito antes de cerrar la ficha.** El modo de fallo del
21-ago **sobrevivía al flasheo** y **no lo curaba formatear**: lo único que lo curó fue **cambiar el
tamaño de la partición**. O sea que «funciona hoy» es compatible con dos historias distintas:

1. el estado que lo rompía se deshizo al reparticionar aquel día y **nunca se ha vuelto a
   provocar** — la causa sigue viva, esperando;
2. algo de lo hecho entre medias lo arregló de verdad.

**No hay forma de distinguirlas con este dato**, porque nunca se supo la causa. Así que la ficha se
cierra por **no reproducible**, no por arreglada, y se conserva la receta de diagnóstico: si vuelve
a pasar, **lo PRIMERO es el log durante el intento de ejecución** — si sale `RUN/v1 /app/X.mod
session=N` el programa llega a lanzarse y se atasca dentro; si no sale, no llega ni a arrancar. Eso
parte el problema en dos y sin ello sólo se puede especular.


### ✅ CERRADA el 9-sep por NO REPRODUCIBLE — decisión de Eduardo, con su criterio

> *«Si vuelve a producirse ya la estudiaremos. A veces ocurren cosas raras, pero si es algo que
> ocurre muy de tanto en tanto cuando se están haciendo pruebas, pues bueno, lo anotamos pero
> seguimos.»*

**Se cierra por no reproducible, NO por arreglada** — la diferencia importa y por eso queda escrita:
nunca se identificó la causa, y el modo de fallo sobrevivía al flasheo y no lo curaba formatear. Lo
que hoy sabemos es que **la Metro ejecuta**, incluido un `Bench` con AOT nativo.

🔬 **Y la receta se conserva, que es lo único que hay que retener**: si vuelve a pasar, **lo PRIMERO
es el log durante el intento de ejecución**. Si sale `RUN/v1 /app/X.mod session=N`, el programa
llega a lanzarse y se atasca dentro; si no sale, no llega ni a arrancar. Eso parte el problema en
dos, y sin ello sólo se puede especular — que es exactamente lo que pasó el 21-ago.

#### ✅ `#484` — `listDir` no existe en la VM-C (abierta 9-sep, de la cola heredada · **CERRADA el 9-sep**: eran SEIS, dos rotas en vivo y dos a V7)

El verbo del wire `LIST_DIR` sí está en el común (`bpvm_repl.c`, `repl_list_dir`), pero la función
**del lenguaje** para que un programa BP liste un directorio no está en la VM-C. Asimetría entre las
dos VMs en la superficie del lenguaje, no en el transporte.

⏭️ **Decisión pendiente de Eduardo**: ¿entra en V6? Si entra, el trabajo es el de siempre —builtin
nuevo en las dos VMs, `make check-builtins` como puerta— y hay que decidir qué devuelve (lista de
nombres, y si distingue fichero de directorio).


### ✅ CERRADA el 9-sep — y no era «falta `listDir`»: eran SEIS, y dos estaban ROTAS EN VIVO

**El enunciado de la ficha se quedaba corto.** `make check-builtins` lo dice desde siempre y nadie
lo había leído como una lista de trabajo:

```
6 builtins solo en miVM, sin numero en la VM-C:
  HEAP_FRAG, HEAP_MAP, INPUT, LIST_DIR, PATH_ABSOLUTE, PROMPT
```

Las seis no son lo mismo. La pregunta que las separa es **¿puede un programa BP llamarlas hoy?**, y
se contesta mirando quién las registra en `Intrinsics.java`:

| grupo | cuáles | qué pasaba |
|---|---|---|
| 🔴 **alcanzables y ROTAS** | `IO.pathAbsolute`, `IO.prompt` | funcionaban en miVM y la VM-C lanzaba `builtin N no soportado en esta VM (subconjunto C)`. **Rotura del invariante, viva.** |
| 🧬 **a medio nacer** | `listDir`, `input` | id en el enum + implementación en miVM, **sin declaración BP y sin C**. Nadie las puede llamar |
| ⚪ **diagnóstico** | `HEAP_FRAG`, `HEAP_MAP` | inalcanzables también |

🕳️ **Y por qué llevaba ahí sin verse: ninguna de las seis tenía un sample en el corpus.** Otra vez.

### `IO.prompt` — existe, y lo que hace es DECIR QUE NO PUEDE (`38ca8ca3`)

El diálogo lo pinta el IDE (verbo `PROMPT_REQUEST` del wire, que **sólo existe en un comentario** de
`json_min.h`), así que en la VM-C nunca hay quien conteste. Eso ya estaba bien decidido y hasta
**documentado en `IO.bp`** —*«si NO hay IDE conectado lanza RuntimeError atrapable»*—; lo que fallaba
es que lanzaba **otro mensaje**. Ahora lanza el de miVM, byte a byte: `prompt: no hay IDE conectado`.

### `IO.pathAbsolute` — cambia de SIGNIFICADO, y por decisión de Eduardo (`18a150cd`)

Devolvía la ruta del **sistema operativo** cuando no había proyecto: un programa BP sacaba
`C:\…\x.txt` por su `stdout`. Dos cosas malas — filtra el host (justo lo que el sandbox de miVM dice
querer evitar) y **no existe en un micro**, donde «absoluto» es `/app/x.txt`. La VM-C no podía
igualarlo sin inventarse un segundo significado.

> **Eduardo:** *«Trabajamos con paths relativos siempre que podamos, eso lo hace más transportable.»*

```
empieza por '/'        -> tal cual (ya es absoluto)
tiene esquema "x:..."  -> tal cual (direccion exacta, #362)
hay proyecto           -> <projectPath>/<path>
si no                  -> tal cual (lo relativo SE QUEDA relativo)
```

🔑 **Y la clave de que las dos VMs no puedan discrepar: es una función PURA de (path,
projectPath)** — no mira el disco y no normaliza `..`. Se **descartó** copiar `bpvm_fs_resolve`, que
sí sondea el FS: con eso, `pathAbsolute` contestaría distinto según existiera o no el fichero, y la
paridad dependería del contenido del disco.

### Lo que va a V7

`listDir` e `input`: tienen id y código en miVM pero **no están declaradas en ningún módulo BP**, así
que hoy no se pueden llamar. **No son un agujero, son una obra parada** — y terminarlas es una
feature nueva, que es lo que en V6 no se hace. Allí se decide qué devuelven (lista de nombres, y si
distingue fichero de directorio).

`HEAP_FRAG` y `HEAP_MAP` se quedan como están: diagnóstico interno, inalcanzable desde BP.

**Corpus 50 → 52**: `IoPrompt.bp` e `IoPathAbs.bp` (las cinco ramas de la regla). `check-builtins`:
230 entradas en la VM-C, todas casando con su ordinal.

#### ✅ `#485` — el hilo `io` del P4 nace POR DEBAJO de su VM: el contrato de `A1`, incumplido (abierta 9-sep, de `#462` · **CERRADA el 9-sep**, `8d0ebee1`)

El contrato lo declara la cabecera con medidas (`include/bpvm_platform.h:75-94`): `io` va a la
**misma** prioridad que la tarea que ejecuta la VM, y **nunca por debajo**. El backend ESP fija `io`
a `tskIDLE_PRIORITY + 1` = **1**, asumiendo que la VM corre en `app_main`. Cierto en S3/C3/C6;
**falso en el P4**, donde la VM corre en `wire_task` a prioridad **5**.

⚠️ **9-sep, Eduardo**: *«que el P4 tenga 2 núcleos o no ahora mismo da igual, solamente trabajamos
con 1»* — así que el atenuante que había («el `io` del P4 va sin afinidad y puede correr en el otro
núcleo») **no cuenta**, y lo que queda es un contrato incumplido a secas.

⏭️ **El arreglo, y es de fondo**: que la prioridad de `io` **se lea de quien lo arranca**
(`uxTaskPriorityGet(NULL)` en `bpvm_io_start`) en vez de ser un literal. Es la única forma de que
«la misma que la VM» signifique lo que dice. Debajo hay una fachada que falta —**arrancar la tarea
que ejecuta la VM no la tiene**—, y es de lo que este contrato depende: hoy son cinco decisiones en
cinco sitios (`pico/main.c`, los dos `main.c` de CubeMX del STM32, `esp32p4/main/main.c`, y ninguna
en S3/C3/C6, donde la VM corre en `app_main`).

📌 Es la misma forma del fallo que en la Pico dejó un KILL sin llegar durante 9,9 s.


### ✅ CERRADA el 9-sep (`8d0ebee1`) — el contrato pasa de VIGILADO a CONSTRUIDO

**El arreglo no es cambiar el número del P4: es que `io` deje de llevar número.** Hasta hoy había un
literal en cada backend y otro donde cada familia crea su tarea de VM, y tenían que **coincidir a
mano**:

| | prioridad de `io` | prioridad de la VM | |
|---|---|---|---|
| `src/platform_freertos.c:283` (STM32) | `BPVM_FR_PRIO_VM` = IDLE+2 | IDLE+2 | coincidía |
| `pico/platform_freertos.c:264` | IDLE+2 | IDLE+2 | coincidía |
| `esp32/common/platform_esp32.c:229` (S3/C3/C6) | IDLE+1 = **1** | `app_main` = 1 | coincidía |
| **el mismo literal, en el P4** | **1** | **`wire_task` = 5** | 🔴 **NO**, y nada lo decía |

Ahora los tres sacan la prioridad de **`uxTaskPriorityGet(NULL)`**. Y es exacto, no aproximado:
`bpvm_io_start()` lo llama **siempre** la tarea que ejecuta la VM — comprobados los cuatro
llamadores (`pico/repl_v1.c:1291`, `esp32/common/repl_esp32.c:778`, `stm32/port/stm32_repl.c:474` y
el sim), todos desde el camino del RUN. Con esto **no hay dos números que puedan divergir**.

`INCLUDE_uxTaskPriorityGet` ya estaba a 1 en las tres `FreeRTOSConfig`. Compiladas Pico, P4, S3 y
Nucleo; paridad 50 PASS.

⚠️ **Lo que NO se puede verificar aquí**: el efecto es del RTOS y sólo se ve en placa. La prueba, si
se quiere cerrar del todo, es **en el P4**: un KILL mientras un programa CALCULA. Con `io` por
debajo debería tardar; a la misma prioridad, decenas de ms — que es lo medido en la Pico y la C6 el
5-sep (9,9 s → 33 ms).

### 🐛 Y de paso: el `io-smoke` tenía un caso PERMANENTEMENTE ROJO, y era la guarda (`dac7fae5`)

Al correrlo para verificar esto salieron dos `FAIL`. **No eran del producto.**

`make io-smoke` construye el sim **sin LVGL**, y sin LVGL `Gui.run()` **no bloquea**: el programa
termina solo antes de que llegue el KILL, el `EXITED` dice `OK` y el caso de `#462` fallaba
**siempre**. Con `make sim LVGL=1` sale `KILLED` y todo verde.

🔑 **Y el script YA tenía guarda para eso** —*«saltada: el simulador no trae LVGL»*— **pero miraba
lo que no era**: buscaba `"gui lista"` en la salida, y el sim headless **también la imprime**. La
guarda no saltaba nunca. La señal de headless es otra y es inequívoca: **si el `EXITED` llega ANTES
de que mandemos el KILL, no hay nada que medir**.

📌 **Un caso permanentemente rojo enseña a ignorar el arnés**, que es peor que no tenerlo — y este
llevaba días así sin que nadie lo mirara. Es [[aviso-que-no-distingue-no-evento-de-fallo]] otra vez,
y esta vez dentro del propio instrumento.

#### ✅ `#486` — `GUI_RUN_ONCE` recorre TODA la tabla de símbolos con `strcmp` en cada pasada (abierta 9-sep, de `#462` · **CERRADA el 9-sep**, `c8d5eda4`)

En el bucle más caliente de la GUI, cada vuelta busca por nombre las **dos** funciones de dispatch
(`Gui.__guiDispatch` y `Gui.__guiDispatchChange`) recorriendo la tabla entera: son ~460 símbolos en
un programa como `JsonDemo`, y la vuelta se repite cientos de veces por quantum
(`src/builtins.c`, `case BUILTIN_GUI_RUN_ONCE`; miVM ya lo cachea en `guiDispatchPc`).

⏭️ Se resuelve cacheando las dos direcciones la primera vez, exactamente como hace miVM. Es barato y
no cambia nada observable — pero **cámbialo con el cronómetro puesto**, que si no es una impresión.


### ✅ CERRADA el 9-sep (`c8d5eda4`) — y las DOS preguntas de Eduardo mandaron el trabajo

**Medido antes de tocar**, con `samples/GuiDispatchBench.bp` (build **sin LVGL**, para que el
barrido sea lo único que se cronometre), 1420 símbolos:

| | 20 000 pasadas |
|---|---|
| antes | **241 ms** (12 µs por vuelta ≈ 8,5 ns por símbolo) |
| ahora | **0-1 ms** |

**1ª pregunta de Eduardo: *«¿esto lo hace cada vez que ejecuta una función, o sólo una vez?»***
Ni una cosa ni la otra: era **una vez por VUELTA DEL BOMBEO**. Con la pausa de 10 ms de `#462`, eso
son **~100 barridos por segundo** mientras la GUI esté viva. Ahora es **una vez por RUN**.

**El arreglo**: `gui_dispatchers()` resuelve los dos y los cachea **en la VM**, no en un `static` —
y eso no es un detalle: la dirección depende del programa **cargado**, y un mismo proceso ejecuta
muchos RUN seguidos (el wire), así que un `static` devolvería la dirección del programa anterior.
Centinela `1` = «buscado y no está», que hace falta o el caso **sin handlers** —el que más vueltas
da— seguiría barriendo. Los campos van **al final** de `bpvm_t`: el prefijo congelado del `.mdn` no
se toca y sus dos asertos siguen en verde (comprobado además con los cuatro tests de nativas).

⚠️ **El corpus NO cubre el dispatch**: ningún caso dispara un handler, así que la paridad en verde no
probaba lo que se acababa de cambiar. Se forzó a mano con `samples/GuiEvSpike.bp` — «3 handler (el
evento se ha drenado)» sale en las dos VMs.

### 🔬 **2ª pregunta de Eduardo: *«¿y la lista está ordenada?»*** — no, y se midió qué implica

**No lo está**: los símbolos se añaden en orden de enlazado (`link.c:130`) y **no hay un `qsort` ni
un `bsearch` en todo el código**. Hay **siete** barridos lineales de la tabla. La pregunta natural
era si alguno más está en un camino caliente, y la respuesta salió midiendo, no leyendo:

| dónde | cuándo se paga |
|---|---|
| `builtins.c:566` | **era cada vuelta del bombeo** → arreglado aquí |
| `builtins.c:685/689` (`bpvm_resolve_handler`) | por handler de Forms invocado = **por clic** |
| `builtins.c:1142` (`GUI_SLOT_OF`) | una vez por nombre, **al cargar el form** |
| `aot_registry.c:52` | registro de thunks = **carga** |
| `bpvm_aot_helpers.c:655` | el thunk lo llama **una vez y cachea** (lo dice su comentario) |
| `link.c:139` (`bpvm_link_lookup`) | carga, y el camino de **ERROR** de la VM |

📌 **Y el único sospechoso serio se descartó con una medida, no con una lectura.** `bpvm_link_lookup`
lo llama `bpvm_throw_runtime_error`, así que parecía «un barrido por excepción». Se probó con dos
programas idénticos salvo en el número de símbolos:

```
169 simbolos   ->  12 ms  / 20.000 lanzamientos
1420 simbolos  ->  13-15 ms
```

Con 8,4× más símbolos el tiempo no se mueve ⇒ **el barrido no se paga por lanzamiento**. La razón:
ese camino es el de los errores que levanta **la VM** (un índice fuera de rango, el `Wdt` sin
backend); un `throw` de un programa BP construye el objeto con opcodes normales y no pasa por ahí.

**Conclusión: tras `#486` no queda ningún barrido de la tabla en un camino caliente**, así que
ordenarla no compra nada hoy. **No se abre ficha** — pero queda escrito por si algún día un camino
nuevo empieza a buscar por nombre en un bucle.

#### 🧬 `#487` — el BREADCRUMB está mudo en 4 de 5 familias (abierta 9-sep, de `#480` · **→ V7** el mismo día)

El cuarteto `setMark`/`markCount`/`markAt`/`bootCount` sólo lo rellena el STM32
(`gpio_stm32.c:310-325`); Pico, ESP32 y P4 dejan esos slots a **NULL**, y `src/pico.c:121-140`
responde no-op **mudo**, `0`, `0` y `1`. En placa, `samples/BreadcrumbDemo.bp` imprime «Arranque #1»
y «Migas: 0» para siempre, y su línea final **promete en voz alta algo falso**.

🔑 Es el «instrumento mudo» **en la herramienta que existe justo para cuando ya estás en problemas**.

📌 **Y es una clase de fallo distinta a `#469`**: no es «nadie registró backend», es **backend
registrado con la mitad de los slots a NULL**. Por eso el arreglo genérico de `#469` no la cubre: la
fachada tiene que distinguir «slot NULL» de «backend ausente».


### 🧬 9-sep — A **V7** (Eduardo)

> *«Lo pasamos a V7. Ahora mismo, con las nuevas placas ya hechas, casi nos da igual.»*

**El motivo es de oportunidad, no de dificultad**: el breadcrumb es la herramienta de las traídas de
placa —dejar marcas por fases y leer el rastro tras un reset inesperado—, y las placas nuevas ya
están arrancadas. Su momento de máximo valor ya pasó; volverá cuando haya silicio nuevo o una
campaña de fallos en campo.

### Lo que queda medido para cuando se retome, que es la mitad del trabajo

📌 **NO es un caso «el hardware no puede», como el watchdog del STM32.** Comprobado el 9-sep:

| | memoria que sobrevive al reset | |
|---|---|---|
| **STM32 U5** | registros de backup | ✅ **es el único que lo implementa** |
| **RP2350** | *scratch* del watchdog | el propio SDK usa el registro 4 para marcar el motivo del reboot |
| **ESP32** | `RTC_NOINIT_ATTR` (memoria RTC sin inicializar) | ⚠️ hay chips donde el IDF lo rechaza (`static_assert`) — mirar uno a uno |

Así que en V7 la decisión no es «se puede o no»: es **implementarlo en RP2350 y ESP32, o declarar
que no está**.

### Y la mitad barata, que va con ello

Sea cual sea la decisión, **la fachada tiene que aprender a distinguir «slot NULL» de «nada que
informar»**. Hoy no puede:

```c
void bpvm_pico_set_mark(int code) {
    if (g_backend && g_backend->setMark) g_backend->setMark(code);
    /* Host: no-op (sin RAM retenida). */
}
```

`markCount()` devuelve **0**, que en BP se lee como *«no hay marcas»* cuando la verdad es *«yo no
guardo marcas»*. Las dos frases se parecen y llevan a sitios opuestos: la primera dice que el reset
fue limpio, la segunda que no tienes instrumento. 🔑 **Y es el instrumento mudo dentro del aparato
que existe para cuando ya estás en problemas** — por eso, aunque se decida no implementarlo,
`BreadcrumbDemo.bp` no puede seguir prometiendo *«si reseteas ahora, el rastro será 10..50»*.

#### 🧬 `#470` — la identidad de la placa se contesta por DOS caminos (abierta 5-sep, de `A3` · **los NOMBRES unificados el 9-sep**, `38e1d143` · **los cuatro números → V7** el 9-sep)

✅ **LOS NOMBRES, RESUELTOS el 9-sep — y no «arreglados»: DISUELTOS.**

**La corrección es de Eduardo, y llegó antes de que yo escribiera nada:** *«hay que hacerlo por el
camino correcto. Todos han de llamar a una misma función en HAL BP, una para el micro y otra para
la placa. Lo que es diferente de cada imagen es la IMPLEMENTACIÓN de estas funciones.»* Yo iba a
hacer que la fachada leyera del **repl**, que invierte las capas.

📐 **Y al medirlo salió que el patrón correcto YA EXISTÍA en la familia de referencia:** el wire de
la Pico llama a `bpvm_pico_board_name()` (`pico/repl_v1.c:625`), o sea a HAL BP. La ESP32 estaba
**medio migrada** —lee `reset_cause` de la fachada pero el nombre de su propia tabla— y el STM32
tenía `BOARD_NAME` y hasta un `gpio_count = 114` **literal en el wire**. Es el mismo patrón que en
`#469` y en los packs: **la RP2350 lo hacía bien y las otras derivaron.**

```
HAL BP   bpvm_pico_micro_name()  +  bpvm_pico_board_name()
           ↑ implementadas por las CUATRO cinturas (Pico, ESP32, P4, STM32)
           ↓ leídas por LOS DOS consumidores
BP       Machine.getMicro() / getBoard()   (builtin 233; el gate, en verde)
wire     el INFO de las tres familias      (ya no tienen tabla propia)
```

🔑 **Y DOS nombres en vez de uno mal definido**, que es lo que hace que la pregunta desaparezca en
vez de contestarse. El micro sale de `CHIP_MICRO`, una entrada **por micro** en el `chip_cfg.h` de
cada uno; la placa dice `generic` salvo que la imagen conozca el modelo.

✅ **VERIFICADO EN LAS DOS PLACAS RP2350 Y POR LOS DOS CAMINOS (9-sep, en placa, Eduardo):**

| | por el WIRE (INFO) | por el PROGRAMA BP (`Machine.getMicro`) |
|---|---|---|
| **Pico 2** | `rp2350a` | `rp2350a` |
| **Metro** | `rp2350b (RP2350B)` | `rp2350b` / placa `generic` |

📌 **Y esto es lo que se gana haciéndolo por el camino correcto**: los dos consumidores leen la
MISMA función de la HAL BP (`bpvm_pico_micro_name()`), así que no hay dos verdades que puedan
divergir — que es exactamente como se desincronizaron el C3 y el C6. La función «inteligente» que
pidió Eduardo queda probada: la distinción sale de `SYSINFO.PACKAGE_SEL` (QFN-60 = RP2350A de 61
pines, QFN-80 = RP2350B de 81), que es de sólo lectura y **no depende del FS**.

✅ **Verificado en dos placas**, no en el log:

| | por el wire | por el programa BP |
|---|---|---|
| **Pico 2** | `rp2350` | `micro: rp2350` · `placa: generic` |
| **ESP32-S3** | `esp32s3` | `micro: esp32s3` · `placa: generic` |

📌 Dos cosas del camino: **el P4 no tenía `chip_cfg.h`** y el include entró en el fichero compartido
→ no compilaba (ahora la tiene, que es lo que significa «una entrada por micro»); y **la Pico ya
decía `rp2350-generic`**, o sea que el reparto estaba medio inventado en la placa de referencia, con
micro y placa mezclados en un solo campo.

⏭️ **LO QUE QUEDA, y necesita criterio de Eduardo**: los otros **cuatro** campos siguen con dos
fuentes, y unificarlos exige decidir qué significan. La tabla de abajo sigue vigente para ellos —
`pwm_slices` son *slices* en la Pico, *salidas* en el STM32 y *canales LEDC* en el ESP32.


Los mismos seis datos (nombre, MHz, GPIO, ADC, PWM, causa de reset) se responden **dos veces por
familia**: una para BasicPlus (la fachada `bpvm_pico_*`) y otra para el wire (`bpvm_repl_info_t`,
rellenada a mano). Ya no coinciden:

| | por el wire | por la fachada (lo que ve el programa BP) |
|---|---|---|
| **ESP32-C3** | «ESP32-C3», 160 MHz, 22 GPIO | **«esp32s3-devkitc», 240 MHz, 45 GPIO** |
| **STM32** | 114 GPIO (`stm32_repl.c:146`) | 128 (`gpio_stm32.c:139`) |
| **Pico** | 24 PWM (`repl_v1.c:632`) | 12 (`main.c:679`) |

**El caso grave es el C3 y el C6**: su identidad se arregló *sólo* en la vía del wire
(`c3_board_id.c` llama a `repl_set_board_id`, pero **nadie llama a `bpvm_pico_set_backend`**), así
que un programa BasicPlus corriendo en un C3 **se cree un S3** — cinco de seis campos falsos,
incluido el nombre de la placa. Es el mismo bug que la cabecera de `c3_board_id.c` dice haber
arreglado: corregido en un camino y vivo en el otro.

📌 Antes de arreglarlo hay que **decidir qué significa cada campo**, porque hoy no está claro: el
114 del STM32 son las I/O del encapsulado y el 128 el rango direccionable del driver; y
`pwm_slices` significa *slices* en la Pico (12), *salidas* en el STM32 (28) y *canales LEDC* en el
ESP32 (8). Unificar sin decidir eso sólo cambiaría de sitio la mentira.

### 🧬 LOS CUATRO NÚMEROS → **V7**, y con el diseño ya decidido (Eduardo, 9-sep)

*«Para cada micro creamos unos arrays, por ejemplo `[0, 1, 3]`, que indican los números válidos, ya
sean UARTs, I2C, SPI, etc. **El número es la longitud del array.** Pero esto va a V7, que es donde
concretaremos los arrays de cada micro.»*

🔑 **Y eso no aplaza la pregunta: la disuelve.** El problema de arriba era «¿qué significa cada
campo?», y la respuesta es que **no hay campo**: lo que hay es el CONJUNTO válido, y el número es su
`length`. Con eso el contador deja de ser un dato aparte que puede contradecir al conjunto — que es
exactamente cómo miente hoy:

| lo que dice hoy | lo que pasa de verdad |
|---|---|
| el S3 declara **45 GPIO** | `Gpio.Pin` **rechaza el GPIO48** (el LED RGB de casi toda placa S3) y **acepta el GPIO23**, que no existe |
| la fachada de ADC deja usar **`0..3`** | son los cuatro pines analógicos del **RP2350** congelados en la API común; el S3 tiene 20 canales y el STM32 otros 20 |
| `pwm_slices` | *slices* en la Pico (12), *salidas* en el STM32 (28), *canales LEDC* en el ESP32 (8) |

Con arrays, «cuántos» y «cuáles» **no pueden discrepar**, porque son la misma cosa preguntada de dos
maneras. Es el mismo movimiento que `#456`: quitar el número en vez de elegirlo.

🔗 **Va con `#479`** (el mapa de pines y los pines analógicos), que ya se fue a V7 el 6-sep por
decisión de Eduardo —*«prefiero no hacer parches»*— y es la misma forma: **tablas por micro que
viajan compiladas en su imagen**, en la HAL BP. Conviene hacerlas de una vez y no tocar ese camino
dos veces.

⏭️ **Lo que queda para V7**, entonces: concretar los arrays de cada micro (UART, I2C, SPI, ADC, PWM,
GPIO) y que `gpioCount()` y compañía pasen a ser su longitud. **En V6 no se toca**, y por tanto esta
ficha sale de la lista de pendientes de V6.

#### ✅ `#471` — el nombre `Pico` atraviesa todas las capas y llega al usuario (abierta 5-sep, de `A3` · **RENOMBRADO el 8-sep** `a9b6cb8b` · **CERRADA el 9-sep** `b79a6c1b`; los métodos NUEVOS, a V7)

##### ✅ PASO 1 HECHO el 8-sep (`a9b6cb8b`): el módulo ya se llama `Machine`

Decisión de Eduardo sobre la versión: *«creo que podemos arreglarlo en V6. No tiene sentido
posponerlo todo a V7. Es un poco de trabajo pero más que nada es ordenar.»* Y sobre la convivencia,
al plantearle alias-o-ruptura: **alias**, *«no hay problema»*.

- `bpstdlib/Machine.bp` es el módulo de verdad, con las mismas 22 funciones.
- `bpstdlib/Pico.bp` es un **alias que reenvía**, y está **generado** de las firmas de `Machine`
  para que no puedan divergir. Se queda porque V4 y V5 están publicadas.
- Coste real de imagen, medido con build limpio: **+2,5 KB en ESP, +5,1 KB en la Pico**.
- `samples/MachineAlias.bp` entra en el corpus → **47 PASS**. Verificado además en los **siete
  builds** y **en la placa** (S3, por el wire): `Machine` y `Pico` contestan lo mismo.

⚠️ **DOS TROPIEZOS QUE HAY QUE RETENER, porque los dos son de repetición:**

1. **El grep que no bastaba.** Comprobé que `"Pico"` no estuviera cableado en el compilador, no
   salió nada, y lo di por bueno. Estaba cableado como **`"Pico.boardName"`**: las intrínsecas se
   registran por `"Módulo.función"` (`Intrinsics.java`). Consecuencia:
   `Machine.boardName()` compilaba con **CERO errores** y devolvía una referencia **basura** —
   miVM con `OutOfMemoryError` en `readVmString`, la VM-C con **segfault**. Es *censar por la
   primitiva, no por el nombre*, con la nota ya escrita.
   📌 Y de ahí sale un hueco del compilador que merece mirarse: declarar `intrinsic` algo **sin
   registro** no da error. El caso contrario sí lo detecta (`intrinsic duplicado`).
2. **Culpé al compilador y era un ARTEFACTO RANCIO.** Tras arreglar el registro, el alias seguía
   roto. No era un bug: el **fuente** de `Pico.bp` no había cambiado, así que el build de la stdlib
   lo dio por bueno — mientras el **compilador** sí había cambiado debajo. Con `rm -rf bpstdlib/out`
   y reconstruir, correcto. **El build de la stdlib cachea por fuente, no por versión del
   compilador**, y eso muerde en cuanto se toca el frontend.

🔧 Y el registro de intrínsecas queda **de una sola lista** (bucle sobre el nombre de módulo). Sólo
`Machine`: registrar también `Pico` pone **dos mecanismos** sobre la misma llamada —el alias declara
funciones BP de verdad— y desincroniza la pila.

⏭️ **Lo que falta de este diseño**: `getMicro()` y `getBoard()` (y los demás que quiera Eduardo:
fabricante, número de serie, PSRAM, RAM…). Son **builtins nuevos**, o sea que tocan las dos VMs y su
gate `make check-builtins`. Eso es lo que de verdad quita el `esp32s3-devkitc` y arregla que el C3 y
el C6 se crean un S3 (`#470`).

##### 🎨 EL DISEÑO, DADO POR EDUARDO (8-sep) — y absorbe también a `#470`

> *«`Pico` no ha de ser "Pico": es un módulo para todos los micros. Se le puede cambiar el nombre a
> "machine" o "system" o algo parecido. El mismo para todos. Dentro, un método `getBoard()` y otro
> `getMicro()` o algo similar. **Nosotros devolvemos los 2 nombres, que sea el usuario el que decida
> qué es lo que le interesa.** Puede haber más métodos: fabricante, número de serie, frecuencia de
> reloj, psram, RAM, etc.»*

🔑 **Y eso disuelve `#470` en vez de arreglarlo.** La ficha vecina decía que había que *«decidir qué
significa cada campo»* porque los dos caminos daban valores distintos para lo mismo. Con este
diseño la pregunta desaparece: **son DOS campos, no uno mal definido.** Hoy `boardName` es una
cosa por el wire y otra por la fachada, y ni los consumidores se ponen de acuerdo —

```
BpIde/.../PicoExplorer.java:2152   sb.append("Micro       : ").append(istr(m, "boardName"))
samples/PicoInfo.bp:12             print "Board:        ", Pico.boardName()
```

— el mismo campo, rotulado **Micro** por el IDE y **Board** desde BP, y devolviendo por ese segundo
camino `"esp32s3-devkitc"`, un devkit concreto, para toda la familia ESP32. Con `getMicro()` y
`getBoard()` cada uno dice lo suyo y no hay nada que sincronizar.

📌 **Y la regla del valor, también suya:** *«nosotros construimos imágenes para el micro, e
intentamos que sea genérica. Así que si podemos tener una entrada por placa que diga "generic" y
será igual para todas las placas que lleven esa imagen. Si alguien se hace su propia imagen, que le
ponga el nombre que quiera.»* O sea: `getMicro()` es real y específico (`esp32s3`, `esp32c3`,
`rp2350`…) y `getBoard()` es **`generic`** salvo que alguien construya su propia imagen.

⚠️ **`system` NO, `machine`.** El nombre `System` ya está pedido: el **módulo raíz de V7** se planteó
*«al estilo de la unidad `System` de Turbo Pascal»*. Gastarlo aquí lo deja ocupado para lo otro —
es *no gastar palabras reservadas*, y la colisión no se ve hasta que duele.

📐 **El alcance, medido antes de decidir la versión**: el módulo expone **22 funciones** y lo usan
**14 samples, 4 módulos de la stdlib, 25 sitios de la documentación y 3 ficheros del IDE**. Rompe
todo programa que escriba `Pico.*`, y `Pico.mod` va **embebido en las cinco imágenes** → cambio de
ABI y blobs regenerados.

⏭️ **Lo que falta decidir: en qué versión.** Por el criterio del propio Eduardo —*«para V6 no
inventamos cosas nuevas»* (8-sep, `#468`)— esto suena a **V7**, y encaja con `L1` y el módulo raíz,
que ya están allí. Pero es su decisión.

✅ **Y lo que NO depende de eso y puede hacerse en V6**: que el C3 y el C6 dejen de creerse un S3.
Hoy su hook por micro (`CHIP_INSTALAR_BOARD_ID()` → `c3_install_board_id()`) instala **sólo** la
identidad del wire (`repl_set_board_id`), y la fachada BP se queda con la de la familia
(`esp32/common/gpio_esp32.c:624`, la del S3). Los valores buenos ya están escritos en `C3_ID`: sólo
van a un sitio en vez de a dos. Eso quita la mentira sin renombrar nada.


`include/bpvm_pico.h` **no es la fachada de una familia**: es la de «información del MCU», y las
cinco la implementan. Pero se llama `pico`, y el nombre sube hasta arriba del todo: la stdlib
expone un módulo **`Pico`**, así que un programa en una STM32 escribe `Pico.uptimeMs()` y
`Pico.cpuFreqHz()` (los bancos del 5-sep lo hacen en las siete placas).

La interfaz es común y correcta; lo que está mal es **el nombre**, y no es inocuo: nadie busca la
identidad de un STM32 en un fichero llamado «pico», y por eso `#470` pasó desapercibido. Mismo
vicio, más pequeño: `pio_count` y `pwm_slices` son vocabulario del RP2350 en un struct común
(`bpvm_repl.h:86`), y el PIO sólo existe en el RP2350.

⏭️ Renombrar a algo agnóstico (`Board`, `Mcu`…). **Toca la stdlib, así que es un cambio de
lenguaje y lo decide Eduardo**: rompe programas que ya usan `Pico.*`, y este proyecto no gasta
palabras reservadas ni cambia el lenguaje a la ligera. Alternativa barata mientras tanto: un alias
y dejar `Pico` como nombre viejo documentado.


### ✅ 9-sep — LO QUE ESTABA ROTO, ARREGLADO; LO QUE ES NUEVO, A V7 (`b79a6c1b`)

**Reparto de Eduardo:** *«No sé, yo arreglaría algo ahora y el resto para V7.»* Aplicado con su
propio criterio — **se arregla lo que está roto, se aplaza lo que es nuevo**.

📌 **Y lo primero que salió al ir a añadir métodos es que DOS de los cuatro que pedía la ficha YA
ESTABAN**: `uniqueId()` (el nº de serie, como recordaba Eduardo) y `tempC()`. Lo que no estaba era
que funcionaran igual en las dos VMs.

### Lo arreglado — tres roturas del invariante, vivas

Ninguna se veía porque **`Machine` sólo tenía `MachineId` en el corpus** (micro/placa):

| | miVM | VM-C |
|---|---|---|
| `uniqueId()` | `host-pc` | `0000000000000000` |
| `tempC()` | callaba | escribía una traza en `stdout` |
| `cpuFreqHz()` | **0** | **150000000**, y otra traza |

### Y debajo, algo peor — lo destapó un aviso de Eduardo

> *«Cuidado con la frecuencia, no es una constante. En su día hicimos pruebas en la Pico: la
> velocidad y la tensión se podían ajustar para hacer overclocking.»*

```
set(200) -> true        y acto seguido    cpuFreqHz() -> el valor de antes
```

**En las DOS VMs.** El stub contestaba «hecho» a un cambio que no hacía y la lectura lo desmentía —
el mismo vicio que el ADC de `#480`: fallar con un valor plausible.

**Ahora el host SIMULA el cambio y lo recuerda.** No es fingir más: el host es el **micro simulado**
(`H10`), y simular el cambio de frecuencia es exactamente su trabajo — así `set` y `get` no se
contradicen y un programa que ajusta la frecuencia **se puede probar en el PC**, que es de lo que va
la cascada.

🔌 **La TENSIÓN no hace falta tocarla, y conviene saber por qué**: en placa la escala **sola**
`pico_pico_set_cpu_freq_mhz_impl` — 1,10 V hasta 200 MHz, 1,15 hasta 250, 1,20 hasta 280, 1,30 por
encima — y con el orden correcto en cada sentido: **subiendo, primero el voltaje; bajando, primero
la frecuencia**, porque al revés *«cuelgue garantizado a 300 MHz con 1,10 V»*. En el PC no hay nada
que alimentar. *(Eduardo la recordaba en `Timer`; está en `pico/main.c`, dentro del propio
`setCpuFreqMHz`.)*

**`samples/MachineHost.bp` al corpus**, con el par `set`/`get` y el clamp. Corpus **53 → 54**.

### ⏭️ Lo que va a V7

- **`manufacturer()`** — la pedía Eduardo: *«una constante similar a micro»*, o sea un `CHIP_...` por
  micro en su `chip_cfg.h`, como `CHIP_MICRO`. Es función nueva.
- **PSRAM y RAM** — ídem.
- 🔴 **`MAX_CPU_MHZ = 300` y `MIN_CPU_MHZ = 18` están en BP e IGUALES PARA TODOS LOS MICROS**, y son
  números del RP2350: en un C3 (160 MHz) o un STM32U5 (160 MHz) el 300 es **falso**. Es exactamente
  el vicio del `0..3` del ADC —la forma de la primera placa convertida en contrato de todas— y va
  con la decisión de `#470`: **cada micro declara lo suyo**.
- **Qué hacen las otras familias con la frecuencia y la tensión**: el escalado de voltaje sólo lo
  tiene la Pico. Falta mirar si las demás escalan, limitan, o aceptan cualquier número.

#### ✅ `#472` — ~~el común nombra DOS familias donde quería decir «micro»~~ (abierta 5-sep, de `A3` · **CERRADA el 8-sep**)

`src/bpvm_aot_helpers.c:25`: `#if defined(BPVM_PICO_NUM_CORES) || defined(ESP_PLATFORM)`. Su
propio comentario dice *«en MCU un global plano basta»* — pero nombra dos familias en vez de la
capacidad, así que **el STM32, que llegó después, cae en la rama del PC**.

Comprobado en el artefacto, no en el código: el `.elf` del Nucleo tiene `__emutls_v.g_aot_fault`,
o sea **TLS emulada —con un `malloc` detrás— en un microcontrolador**, donde se quería un global
plano.

⏭️ Un macro de capacidad (`BPVM_SIN_TLS`, o al revés `BPVM_TIENE_TLS`) que ponga cada familia. Es
media hora, y **quita la clase de error entera**: hoy cualquier familia nueva cae por defecto en
la rama equivocada y en silencio, que es exactamente lo que pasó.

✅ **CERRADA el 8-sep.** La polaridad se invierte: el macro pasa a nombrar la **capacidad** y el
**defecto es la rama del micro**. `bpvm_aot_helpers.c` dice ahora `#if defined(BPVM_TIENE_TLS)` →
`__thread`, `#else` → global plano; y `BPVM_TIENE_TLS` lo declara **un solo sitio**, el
`bpgenvm-c/Makefile`, que es el único build de host que compila esto (el mismo Makefile hace la VM
de host, la batería de tests y el simulador). Una familia nueva que no defina nada cae donde debe.

🔴 **Y era PEOR de lo que decía la ficha, medido en el artefacto y no leído.** El síntoma no era
sólo «el STM32 podría caer en la rama del PC»: estaba **en las dos imágenes STM32 publicadas**, no
sólo en el Nucleo. `arm-none-eabi-nm` sobre los `.elf` del 6-sep daba `__emutls_v.g_aot_fault`,
`__emutls_v.g_aot_callctx` y `__emutls_get_address`; y el desensamblado de `emutls_alloc` son **dos
`malloc` en el primer acceso y un `abort()` si fallan**. En un micro.

⚠️ **Y el comentario que lo justificaba ya era falso**, que es lo que convierte el coste en real:
decía que *«en MCU el AOT no corre, así que el fault-slot NUNCA se arma»*. Hoy el STM32 **sí** carga
`.mdn` Thumb-2 (`stm32/port/stm32_repl.c`), `aot_call_guarded` (`interp.c`) arma el slot en **cada
nativa**, y `heap.c` pide el callctx en **cada GC**. O sea que la TLS emulada se pagaba de verdad.

✅ **Verificado en los cinco artefactos, que es la única prueba que sirve aquí** — y conviene decir
por qué: **`compat.sh check` NO puede ver este cambio**. Es una decisión de compilación que no altera
el `stdout`; un verde ahí sería un falso verde. La prueba es de **símbolos**:

| build | antes | después |
|---|---|---|
| host (MinGW x64) | símbolos en `.tls` | **siguen en `.tls`** — el host conserva sus N workers |
| **Nucleo U575** | `__emutls_v` + 2×`malloc` + `abort()` | **`b g_aot_fault` plano en `.bss`** |
| **Discovery U5G9J** | `__emutls_v` | **plano en `.bss`** |
| Pico 2 (ARM) | plano | plano — sin cambio |
| ESP32-C6 (RISC-V) | plano | plano — sin cambio |

📌 Los cuatro `.elf` de ESP comparten la misma rama; se comprobó el **C6** como representante del
toolchain RISC-V y la **Pico** como representante del ARM de micro. Y de propina, una trampa que se
coló por el camino: el primer `nm` que usé para el C6 **no existía**, así que su grep salía vacío y
parecía «0 ocurrencias». Se repitió con el `nm` bueno y con un **control** (6.363 símbolos) para que
el cero significara algo. Instrumento mudo, otra vez.

⚠️ **Dónde está el riesgo de este cambio, por si algún día se toca**: no en el micro, **en el host**.
Si el host se quedara sin `__thread`, sus N workers pthread compartirían un `g_aot_fault` con su
`jmp_buf` dentro (`interp.c`, `setjmp`), y un `longjmp` iría a un frame muerto — sin error de
compilación, sin diff de paridad, sólo bajo carga. Por eso el `-D` va en el Makefile y no se quita.

✅ Y el arnés, después de reconstruir el host de cero: **45 PASS, 0 FAIL, 0 SKIP**.

#### ✅ `#473` — ~~el resto de la auditoría de capas, sin verificar~~ (abierta 5-sep, de `A3` · **CERRADA el 10-sep**)

> **Cerrada con los 91 hallazgos triados enteros y los CUATRO que estaban rotos resueltos**: tres
> arreglados y medidos en placa (`97f7fb36`, `66c13317`, `6388dc31`, `c659d94d`) y el cuarto **medido**
> —resultó no estar roto— y pegado a `#491`. Lo demás (55 vivos de baja gravedad) va a V7, repartido
> entre `#470`, `#479`, `#487` y `#491`.

La auditoría de `A3` la hicieron seis auditores y terminaron los seis, pero **la fase de
verificación adversarial se cortó por límite de sesión: 12 de 90 hallazgos quedaron
comprobados**. Lo que entró en `A3` y en `#469`–`#472` está verificado a mano; el resto vive en
`docs/A3_AUDITORIA_CAPAS_BRUTO.md` y **no se da por bueno**.

Entre lo que hay ahí sin verificar, y que pinta serio:
- El **`EXITED` del STM32 interpola cadenas del usuario SIN ESCAPAR** en el JSON (`missing`,
  `entry.fallo`): un mensaje de error con una comilla rompería el protocolo.
- El verbo **`RUN` está implementado entero cuatro veces**, con la misma secuencia de pasos.
- El **sink de `OUTPUT` y su escapador JSON, escritos cuatro veces** además del que ya existe en
  el común.
- `fs_total_bytes` y compañía, **copiados carácter por carácter** en las tres familias.
- El STM32 **no desactiva de verdad** con `Wdt.disable()` (reprograma el IWDG a 131 s) y el
  programa no puede enterarse.

⏭️ Verificar uno a uno antes de tocar nada. La regla del proyecto vale aquí más que nunca: un
hallazgo falso cuesta más que uno que falta, porque manda a mirar donde no está el problema.

---

✅ **LOS 21 `rompe-el-modelo`, TRIADOS ENTEROS (10-sep).** Se verificó uno a uno, contando
consumidores y —cuando se podía— ejecutándolo en placa. Reparto:

| estado | cuántos | cuáles |
|---|---|---|
| **arreglados** | 2 | la redondeo de tick que faltaba en dos backends (`a257be8b`) · el `EXITED` del STM32 sin escapar (`80d6ad0d`) |
| **vivos y verificados**, sin arreglar | 10 | los tres backends de FreeRTOS · `lv_conf.h` · `setRotation` · la fachada de FS que depende de littlefs · la clasificación de fallo en 1 de 3 backends · los tres `board_mgr_*` · el hilo que se borra a sí mismo · `RUN` cuatro veces · el sink de `OUTPUT` cinco veces · la puerta de arranque con tres listas distintas |
| **ya cubiertos por otra ficha** | 9 | `#469` (ADC sin backend, el pinout del RP2350 en el común) · `#480` (`Pulse` del C3, `initChannel` que falla con 0) · `#470` (identidad, descriptor de placa, `gpioCount`, y los arrays de buses → V7) |

**Dos hallazgos venían EXAGERADOS**, y esto es justo lo que la fase de verificación existe para
pillar:
- *«tres copias casi línea a línea»* de `board_mgr_*`: medido, el solapamiento es 56 % / 44 % / 61 %
  y el del ESP32 es el doble de grande (294 líneas frente a 137). Hay duplicación, pero no es un
  copia-pega.
- *«`lv_conf.h` falla por defecto y en silencio»*: la duplicación es real (dos listas idénticas de
  nombres de placa, `:33` y `:976`), pero el fallo **no sería silencioso**: sin su `-D` la placa
  nueva se queda con `LV_USE_SDL 1`, y los builds ESP sí compilan `lv_sdl_*.c` —sus `.obj` están en
  `esp32c6/build/`—, que acaba en `#include <SDL2/SDL.h>`. Eso es un error de compilación.

**Y uno era PEOR de lo descrito.** La puerta de arranque no sólo está copiada tres veces con listas
distintas: lo que no está en la lista **pasa de largo al despachador común**. Los agujeros, hoy:
la Pico no gatea `LIST_DIR`; el ESP32 no gatea `MKDIR`/`RMDIR`/`RENAME`/`FORMAT`; el STM32 no gatea
`LIST_DIR` ni `SAVE`. Y el común no calla: sin FS, `LIST_DIR` contesta `NOT_FOUND «no se puede
listar»` —como si el directorio no existiera, en vez de «no hay FS»— y `SAVE` contesta **OK**.

**Medido en placa** (Discovery, 10-sep): `setRotation(90)` deja el modelo en 480×800 mientras el
LTDC sigue escaneando 800×480 —su `disp_set_rotation` es literalmente `(void) deg;`— y los `Form`
se dimensionan con el ancho del **modelo** (`Gui.bp:932`). El programa de prueba es
`samples/RotaDk2.bp`.

🔬 **LA MEDIDA PENDIENTE, HECHA (10-sep) — y el hallazgo se cae por la mitad.** La pregunta era si
el hilo que se borra a sí mismo (`vTaskDelete(NULL)`, vivo en la Pico y el ESP32) filtra memoria como
le pasó a la Nucleo. **En la Pico NO filtra**, y no es una impresión:

| | |
|---|---|
| control | 5 `INFO` seguidos sin ejecutar nada: `rtosHeapMinFree` clavado en 16104 |
| 40 × `RUN /app/Hello.mod` | 16104 → **10920 en el primer RUN, y ahí se queda los 40** |
| 15 × `RUN /app/Bench.mod` (la VM ocupada calculando, que es la condición que mató a la Nucleo) | **10920, plano** |

La marca de agua es *mínimo histórico libre*: sólo baja. Una fuga de un solo byte por RUN la habría
hecho bajar 55 veces. Y el camino **se ejecuta**: cada RUN crea y une el hilo `io`
(`bpvm_io_start` en `pico/repl_v1.c:1295`) por ese mismo trampolín de 4 KB.

**El porqué, comprobado en los dos extremos** — el mecanismo es quién cede el turno a la tarea
ociosa, que es quien recicla la pila de una tarea borrada:

| familia | su lazo del wire | ¿la ociosa recibe turno? |
|---|---|---|
| Pico | `vTaskDelay(10 ms)` cuando no hay nada que leer (`pico/repl_v1.c:1723`) | **sí** |
| ESP32 | `wire_read_byte(100)` — bloquea ≤100 ms cediendo CPU (`esp32/common/wire_v1.c:112`) | **sí** |
| STM32 | `stm32_wire_getchar()` devuelve −1 y el lazo gira con `continue`; el de arriba (`stm32_repl.c:865`) tampoco cede, y la tarea `vm` va a `tskIDLE_PRIORITY + 2` (`Discovery_u5g9j/Core/Src/main.c:173`) | **no** |

Así que la Nucleo no murió por `vTaskDelete(NULL)`: murió porque **su lazo del wire no cede nunca**.
`vTaskDelete(NULL)` sólo era la parte que se apoyaba en la ociosa. El arreglo del común (suspender y
que borre `join`) es correcto igual —no depende de que nadie tenga turno—, pero **la Pico y el ESP32
no estaban expuestas**: el hallazgo acertaba en la duplicación y erraba en la consecuencia.

📄 **El arreglo de fondo de R1+R16 tiene estudio: `docs/FUSION_BACKENDS_FREERTOS.md`** (10-sep).
Qué se puede fundir de los tres backends de FreeRTOS y qué no, en pasos verificables. Lo que hay que
saber sin abrir el documento:

- 🔴 **Lo que bloquea es un 32 % sin explicar.** Ya se intentó y se midió el 5-sep: con el común, la
  Pico hacía `PrintBench` en **5 330 ms** en vez de 4 040, repetible a ±10 ms y aislado por
  bisección — y el `KILL` **mejoró** de 33 ms a 2. No se subió porque no se supo explicar. Esa medida
  es de antes de `#485` y de `a257be8b`, así que **el paso 0 es remedirla**, no discutirla.
- ⚠️ **Una trampa de unidades que el build no ve.** El común expresa las pilas en **palabras**
  (`BPVM_FR_STACK_IO 1024u`), el ESP32 en **bytes** (`4096`). Fundir sin pasar el `-D` deja `io` con
  1 KB cuando su sink declara `char buf[1024]` en pila: desbordamiento en la **primera línea
  impresa**, con el build en verde.
- ⚠️ **La fusión cambia una fuga por otra, y hay que decirlo entero.** Con `suspend+join`, un hilo
  que nadie una **no devuelve su pila nunca, con certeza** (hoy es probabilístico y depende de la
  ociosa). Hoy sería seguro —los cuatro sitios unen— **salvo el camino de error de
  `src/scheduler_smp.c:303-315`**, que libera y se va sin unir los workers ya creados. Ése sí es un
  defecto de hoy.
- 🐛 **Un defecto latente en las TRES copias, que la fusión NO arregla**: el condvar usa
  `xSemaphoreCreateBinary()` (`src:106`, `pico:77`, `esp32:70`), que retiene UN token, y `broadcast`
  emite N. Los sobrantes se pierden, y hay esperadores sin plazo. **Sin síntoma observado** — se
  anota, no se persigue.
- 📌 **Nadie compila el común salvo CubeIDE**: un cambio ahí no lo ve ningún `make` ni `idf.py`
  hasta que se construye el STM32.

✅ **LA COLA, TRIADA ENTERA (10-sep) → `docs/473_TRIAJE_COLA.md`.** No eran ~57: son **70**
(48 `incomodo` + 22 `cosmetico`; la auditoría tiene 91 en total). Siete lotes de diez, verificados
contra el código de HOY, más una ronda de escépticos **sobre los declarados muertos** — porque al
cerrar una versión el error caro es dar por resuelto lo que sigue roto: eso apaga la búsqueda.

🔑 **Y esa ronda se ganó el sueldo: de 12 dados por muertos, 8 RESUCITARON.** El patrón, siempre el
mismo: *se arregló la mitad que llega al usuario y se declaró muerto el hallazgo entero*. Si me
llego a fiar del primer triaje, ocho cosas vivas se habrían dado por cerradas.

| desenlace | n |
|---|---|
| **vivos** | **59** |
| ya **cubiertos** por otra ficha | 7 (`#470` ×4 · `#473` ×2 · `#487` ×1) |
| **arreglados** de verdad desde el 5-sep | 4 |
| **de los vivos, ROTOS HOY** | **4** |
| de los vivos, a **V7** | 55 |

🔴 **LA LISTA CORTA — los cuatro, releídos a mano uno a uno** (no me fío del texto del auditor ni del
triaje):

1. **`recv_line` no abandona una línea a medias en 4 de las 5 imágenes.** La Pico
   (`pico/wire_v1.c:38-45`) y las tres ESP (`esp32/common/wire_v1.c:112-113`) giran sin plazo; el
   STM32 sí topa a 300 ms con su motivo escrito (`stm32/port/stm32_wire.c:110-115`: *«Anti-cuelgue…
   en vez de girar para siempre»*). ⚠️ **La consecuencia real, comprobada, NO es la que decía el
   triaje** (*«la VM se para y hay que desenchufar»*): es que **un mensaje truncado se COME el
   siguiente**. El mensaje que llega después se concatena a la línea estancada, su `
` la termina,
   y el resultado no empieza por `{` → se descarta entero. Si el que se traga es un `KILL`, el
   síntoma es *«le he dado a parar y no ha parado»*, que no se diagnostica nunca.
2. **La atomicidad de una línea en el cable sólo la garantiza la Pico** (`pico/wire_v1.c:93-97`,
   `tx_lock` dentro de `send_line`). Las otras cuatro escriben sin cerrojo. Depurando en placa, el
   IDE puede recibir dos JSON entrelazados: corrupción de framing, no estética, y **no existe en el
   PC**.
3. **`Uart` pierde bytes en la Pico y en el STM32 — y `available()` sólo era el síntoma.**
   El hallazgo del auditor era que la Pico contesta un booleano donde su contrato dice *«cuántos
   bytes»* (`include/bpvm_uart.h:16-17` vs `uart_is_readable(inst) ? 1 : 0`, `pico/main.c:376-378`),
   con el ESP32 cumpliendo (`gpio_esp32.c:249-254`) y el STM32 devolviendo `-1` honestamente. Cierto:
   `if Uart.available() >= 4` **no se cumple NUNCA en la Pico** con 40 bytes esperando, y sí en el
   ESP32 con el mismo programa.

   🔑 **Pero Eduardo va al fondo (10-sep)**: *«las UARTs a veces tienen un pequeño buffer que siempre
   se queda corto, así que hay que añadir un buffer externo que lo amplíe. Eso es necesario yo diría
   que siempre, así que no se trata de adaptarse al hardware sino que el hardware y el software se
   adapten a nuestras necesidades.»*

   Y con eso a la vista, el censo dice que el problema es **mayor** que el `available()`:

   | familia | buffer RX de `Uart` | `available()` |
   |---|---|---|
   | **ESP32** | **512 B por software**, alimentado por la ISR del driver de IDF (`ESP32_UART_RX_BUF`, `gpio_esp32.c:190,232`) | la cuenta real |
   | **Pico** | **ninguno** — sólo el FIFO de 32 B del PL011; **cero IRQ, cero anillo** | `1` ó `0` |
   | **STM32** | **ninguno** — `HAL_UART_Receive` bloqueante sobre el registro (`gpio_stm32.c:539`) | `−1` |

   O sea: **en la Pico y en el STM32 un programa BP PIERDE BYTES** en cuanto llegan más de los que
   caben en el FIFO antes de que él lea. El ESP32 ya hace lo que Eduardo describe. Así que no es
   «arreglar el contador»: es **dar el buffer que el contrato ya prometía**.

   ➕ **Y su segundo apunte, que decide el CÓMO**: *«muchas familias soportan DMA para los
   buffers.»* Comprobado: **hoy no usamos DMA en ninguna parte** — ni siquiera está enlazada
   `hardware_dma` en la Pico; en el STM32 lo único que aparece es config de trigger que generó
   CubeMX. Sería infraestructura nueva entera. Eso parte el trabajo limpiamente:

   - **V6 — el buffer, por IRQ.** Es lo que quita la pérdida de bytes y hace verdad el contrato. El
     patrón ya está escrito en casa: `stm32/port/stm32_wire.c:71-78` es exactamente un anillo
     alimentado por IRQ, para el wire.

   ✅ **HECHO Y MEDIDO EN LAS DOS FAMILIAS (10-sep).** El anillo (512 B) vive en la fachada común
   (`src/uart.c`) y cada familia sólo enciende su IRQ y empuja bytes:

   | placa | prueba | resultado |
   |---|---|---|
   | **Pico 2** | loopback GP0↔GP1, 200 B, 300 ms sin leer | `available`=200, leídos=200, **0 distintos** — 3 pasadas |
   | **Discovery** | loopback PC10↔PC11, 200 B, 300 ms sin leer | **PASS en UART4 (AF8) y en USART3 (AF7)** |

   El FIFO de hardware del STM32 son **8 bytes**: escribir 200 y esperar 300 ms sin leer habría
   perdido 192. No se pierde ninguno.

   🐛 **Y de camino se cerró un cuelgue que nadie había visto.** El `read` viejo del STM32 hacía
   `timeout <= 0 → HAL_MAX_DELAY` (`gpio_stm32.c:539`), o sea **esperar para siempre**: un programa
   que sondee con `read(..., 0)` colgaba la VM y hubo que resetear la placa por la sonda. Con el
   anillo, `timeout 0` devuelve al instante lo que haya. Es el gemelo de lo que `available()` hacía en
   la Pico: **la fachada prometía sondeo y ninguna de las dos familias lo daba.**

   📌 **Dos herramientas salen de aquí, y las dos por haberme equivocado**: `samples/GpioPuente.bp`
   comprueba que el cable está ANTES de culpar al código, y `samples/ScanCn1.bp` **busca** el par
   unido en el conector cuando no se sabe dónde está — contestó «PD7↔PE2» en 200 ms mientras yo
   probaba en PC10/PC11.

   ⚠️ **Y una torpeza que conviene no repetir**: el primer diagnóstico en la Discovery lo hice contra
   el firmware **viejo**. Había compilado la placa tres veces y **no la había grabado ni una**;
   `available()` contestaba `-1` y lo leí como «el anillo no se activa» cuando era código de tres
   horas antes. Compilar no es grabar, y **el `serverBuild` del `HELLO` lo dice en un segundo**.
   - **V7 — DMA donde la familia lo tenga.** A velocidades altas una IRQ por byte cuesta CPU, y
     además **despierta al micro por cada byte**, que enlaza directo con `#490`. Con DMA circular
     los bytes entran sin CPU y `available()` sale del contador del DMA.

   🔑 **Y lo que hace que las dos cosas quepan sin tocar el lenguaje**: la fachada no dice *«usa
   IRQ»* ni *«usa DMA»* — dice **«hay un buffer de N bytes y `available()` te dice cuántos hay
   dentro»**. Cómo se llena es asunto de la cintura de cada micro. Es la forma de HAL BP de siempre.
4. **`read_stream` no existe en el backend FAT** — está en littlefs (`src/fs_lfs.c:407`, *«#453 —
   472 aperturas por 120 KB»*) y no en `src/fs_fat.c`. El respaldo de `fs_facade.c:466-482` es un
   bucle de 256 B con `f_open+f_lseek+f_read+f_close` **por trozo**, y el propio fichero documenta
   que el coste es **cuadrático**. O sea: bajar un fichero grande de `/sd` desde el explorador del
   IDE. ⚠️ **Condicionado**: la medida del timeout es prestada de `#453` (littlefs en un S3). **Hay
   que cronometrar un GET de ~120 KB desde la SD de la Metro ANTES de tocar nada**; si no revienta,
   se va a V7.

⏭️ **Los 55 restantes van a V7**, agrupados por tema y pegados a la ficha que ya los posee: la
identidad y los números del micro a `#470`; el mapa de pines a `#479`; el breadcrumb a `#487`. Y
sale un **huérfano que necesita ficha nueva**: el nombre `pico` en la **capa C** (`bpvm_pico.h`,
`bpvm_pico_set_backend` y 29 `BUILTIN_PICO_*`, que registran las **cinco** familias) — `#471` se
cerró habiendo arreglado sólo la capa de usuario. Y una nota que
no es ficha porque no tiene consecuencia conocida hoy: en el STM32 **la tarea ociosa no corre nunca**,
así que cualquier cosa que FreeRTOS difiera a la ociosa allí no ocurre. El único que se apoyaba en
ella era el borrado de tareas, y ya no.

#### 🧬 `#479` — EL MAPA DE PINES Y LOS PINES ANALÓGICOS → **V7** (abierta 6-sep, decidida a V7 el mismo día)

**Decisión de Eduardo, y con su motivo**: *«Lo desarrollamos en V7. Ahora en V6 no lo desarrollamos,
prefiero no hacer parches.»* Incluye rechazar el trozo intermedio que le ofrecí (hacer que
`initChannel(ch)` significara «el canal ch de esta placa» sin tocar el lenguaje): media reforma es
un parche.

**De dónde sale.** Tirando de `#469` apareció que el `0..3` de la fachada de ADC **son los cuatro
pines analógicos del RP2350 congelados en la API común** — la forma de la primera placa convertida
en contrato de todas. Con los números delante:

| familia | canales ADC que declara | los que la fachada deja usar |
|---|---|---|
| RP2350 | 4 (GP26–29) | 0..3 |
| ESP32-S3 | 20 | 0..3 |
| ESP32-C3 | 6 | 0..3 |
| ESP32-C6 | 7 | 0..3 |
| STM32U5 | 20 | 0..3 |

**El planteamiento es de Eduardo**: *«seguro que hay más de 4 pines. Hay pines que solamente pueden
ser digitales, pero hay pines que pueden ser o analógicos o digitales. Eso se tiene que soportar en
todas las familias.»* Y luego: *«esto es hardware, así que afecta a la capa HAL BP. El mapa de pines
es algo propio de cada micro y se tiene que construir con la imagen del micro. Lo que afecta al
lenguaje BP: o hacemos una clase nueva `AdcPin` o extendemos la clase `Pin` (o las dos cosas). Lo
que está claro es que si un pin es analógico no puede ser digital al mismo tiempo.»*

**Las tres piezas, y en qué capa cae cada una** (comprobado qué hay hoy de cada una):

1. **El mapa de pines — NO EXISTE.** Sólo hay **contadores**: `gpioCount()`, `adcChannels()`,
   `pwmSlices()` (`include/bpvm_pico.h:48,62,63`). El runtime sabe *cuántos*, nunca *cuáles*. Va en
   la cintura de cada familia y viaja compilado en su imagen. ⚠️ **El ENV NO es el sitio** —yo lo
   propuse y me corrigió—: el ENV es config de placa (panel, tamaño de FS); esto es verdad de silicio.
2. **El estado del pin — TAMPOCO, y sin él el invariante no es comprobable.** `src/gpio.c` sólo
   reenvía al backend y no guarda nada; el único que lleva cuenta es el STM32, y en privado
   (`s_mode[128]`, `s_pull[128]`, `gpio_stm32.c:50-51`). Puesto en el común, la regla «analógico ⇒
   no digital» se escribe **una vez** y vale para las cinco familias.
3. **El lenguaje — DECIDIDO por Eduardo el 6-sep.** `Gpio.Pin` ya existe (`Gpio.bp:92`:
   `Pin(num, mode)` con `on/off/toggle/value/isHigh`).

   ❌ **`Adc.Pin` NO**: *«estamos llamando a 2 clases diferentes `Pin` y eso en la práctica va a
   crear confusión»*. El nombre es **`AnPin`**.
   ✅ **`AnPin` DESCIENDE de `Pin`**, y el modelo no es artificial: *«como un pin físico puede ser
   digital y/o analógico, tener una clase que lo permita es un buen modelo»*. Yo objeté que un
   `AnPin` heredaría `on()`/`off()` —lo que su propio invariante prohíbe— y propuse partir `Pin`;
   la respuesta cierra la objeción sin partir nada: *«en BP los métodos pueden ser virtuales y los
   valores se exportan como propiedades, así que no hay ningún problema en extender la clase `Pin`,
   sobrescribiendo lo que haga falta.»*

   **El modelo, entero:**
   - **`Pin`** — el pin FÍSICO: `num` y `mode`. Modos digitales **`INPUT` / `OUTPUT` /
     `OPEN_COLLECTOR`**; modo analógico **`ANALOG`**. El `pull` (none/up/down) aplica **sólo en
     digital**.
   - **`AnPin extends Pin`** — sobrescribe lo que haga falta y añade `read()` / `readVolts()` /
     `readAvg()`.
   - 🔑 **La exclusividad no hay que vigilarla: es el mismo campo `mode`.** Pedir analógico sobre un
     pin en modo digital falla **por construcción**, no porque alguien se acuerde de comprobarlo.

   **`OPEN_COLLECTOR` es de Eduardo y es nuevo entero** — *«importante para cuando se conectan
   varias salidas entre sí»*. Comprobado: **no existe en ninguna parte** (ni en BP, ni en la fachada
   —`init(pin,mode)` sólo conoce `0=INPUT, 1=OUTPUT`, `bpvm_gpio.h:31`—, ni en las cinco cinturas).
   Y no cuesta lo mismo en todas, que es el argumento de que esto vive en la HAL BP:

   | familia | colector abierto |
   |---|---|
   | **ESP32** | nativo — `GPIO_MODE_OUTPUT_OD` (`hal/gpio_types.h:112` del IDF) |
   | **STM32** | nativo — `GPIO_MODE_OUTPUT_OD` (`stm32u5xx_hal_gpio.h`) |
   | **RP2350** | ❌ **el pad NO lo tiene**: hay que emularlo — conducir a bajo / soltar a alta impedancia |

   ⚠️ **Y la semántica que hay que fijar en el contrato, no dejarla a quien implemente**: en modo
   colector abierto **`write(1)` no significa «pon a alto», significa «suelta»** — el alto lo pone
   el pull-up (externo, o el interno si se activa). Es el modo en que `write` cambia de significado;
   sin escribirlo acabaría significando cinco cosas distintas.

   ✅ **De las tres cosas del modo digital, `pull` YA EXISTE** en la fachada común
   (`pull(pin, 0=none/1=up/2=down)`, `bpvm_gpio.h:32`). Lo que falta es `OPEN_COLLECTOR`.

📌 **Y el dato que explica por qué esto es de lenguaje**: `Adc` es **la única fachada que no
habla de pines**. Sus vecinas ya hacen lo que Eduardo pide — `Gpio.init(pin, mode)`,
`Pwm.initSlice(pin, freqHz)`, `Pulse.initSlice(pin, edgeKind)`. La rara es ésta.

⚠️ **Coste**: toca `Adc.bp`/`Gpio.bp`, que van **embebidos en las cinco imágenes** → ABI y blobs
regenerados. Encaja con `L1` y el módulo raíz, que ya están en V7 por el mismo motivo.

✅ **Lo que SÍ se queda de V6**: el arreglo de `#469`. No es un parche hacia este diseño —no añade
media funcionalidad—: **quita una mentira**. Hasta V7, en una placa sin backend de ADC la fachada
falla en vez de devolver una rampa que parece una lectura.

#### ✅ `#478` — ~~miVM NO PUEDE TOCAR UN BUS~~: los builtins de I2c/Spi/Uart se quedaron fuera del 4→8B (abierta y **CERRADA el 6-sep**, `c27a85fe` + `163be329`)

**El invariante sagrado, roto en duro, y con la REFERENCIA en el lado equivocado.** Reproducido con
`samples/BusBug.bp`, el mismo `.mod` en las dos VMs:

```
miVM :  1: inicio · [i2c] init… · 2: ctor ok · 3: array ok  buf[0]= 7
        [bpgenvm worker 0, tid=0] 1073741830      ← muere aqui
VM-C :  … 4: read ok  rc= 1  buf[0]= 0 · 5: fin      (status=OK)
```

`1073741830` = `0x40000006`: **es un handle, no una dirección**.

**La causa, verificada en tres líneas del mismo fichero:**
- `VirtualMachine.java:1867` → `REF_SIZE = 8`.
- `VirtualMachine.java:5747` → `popTc()` hace `tc.sp -= 4`.
- `VirtualMachine.java:5140` (I2C_WRITE) → `int dataRef = popTc(tc);` y acto seguido lo usa como
  **dirección física**: `readI32(memory, dataRef + 4 + i*4)`. Dos fallos a la vez: lee **medio
  handle** y **desincroniza la pila** 4 bytes por cada ref.

📌 **Y el arreglo está escrito 400 líneas más arriba, en el mismo fichero.** `case MOVE`
(`:4774-4783`) usa `popTcRef` + `refDeref`, con un comentario que nombra el bug exacto:
*«#6 (censo V4): array ref = 8B (era popTc 4B + el handle se usaba como dirección física SIN
refDeref)»*. **A `MOVE` se lo arreglaron; a los buses no.** Es la campaña de refs 4→8B de V4 con
siete sitios detrás.

**Los siete**: `:5140`,`:5148` (I2C_WRITE) · `:5159`,`:5164` (I2C_READ) · `:5204`,`:5209`
(SPI_WRITE) · `:5220`,`:5223` (SPI_READ) · `:5232`,`:5238`,`:5241` (SPI_TRANSFER) · `:5268`,`:5274`
(UART_WRITE) · `:5287`,`:5290` (UART_READ). Y de propina `NEOPIXEL_SHOW` (`:5452-5458`) y
`NEOPIXEL_INIT` (`:5446-5451`), que **finge éxito** donde la VM-C lanza.

⚠️ **Por qué no lo vio nadie**, y es la misma historia que `#477`: el corpus de `compat.sh:51-53`
tiene 16 casos y **ninguno toca i2c, spi, uart, gpio, adc, pwm, pulse ni neopixel**. Y el sample que
lo reproduce —`samples/BusBug.bp`— existía ya: se escribió para bisecar un cuelgue **en la Pico**.
El arnés tenía el reactivo delante y no lo metió en la red.

✅ **Y eso se cerró el 7-sep (`1e46f346`): `BusBug` YA ESTÁ en el corpus**, junto con `AdcDemo`,
`ArgDemo`, `MathRango` y `mathtest`. El corpus pasa de 38 a 45 casos y por primera vez lleva GUI.
Ojo al matiz que corrige este párrafo: **el arnés nunca estuvo desactivado** —el `check` ejecuta
`check_parity` y estaba en verde—; lo corto era el corpus, que es exactamente lo que esta ficha
diagnosticó bien y `#477` decía mal. Ver `#477`.

✅ **ARREGLADO el 6-sep.** Los **ocho** sitios pasan a `popTcRef` + `refDeref` (el patrón de
`MOVE`), con su comprobación de null: I2C write/read, SPI write/read/transfer (éste con **dos**
arrays), UART write/read, y `NEOPIXEL_SHOW` — que no desreferencia, pero **tenía que sacar 8 B o
descuadraba la pila igual**. Medido antes y después con el mismo `.mod`:

```
antes:  … 3: array ok  buf[0]= 7 · [bpgenvm worker 0, tid=0] 1073741830   ← muere
ahora:  … 3: array ok  buf[0]= 7 · [i2c] read … · 4: read ok  rc= 1 · 5: fin
```

Y de paso se alinearon los textos de los tres stubs de bus de la VM-C con los de miVM
(`count=`/`(sim → ceros)` en vez de `n=`/`(stub → ceros)`) — misma clase que `#469`. Comprobado que
no se rompió nada: `MathRango`, `MathTest` y `AdcDemo` siguen dando el mismo contenido en las dos VMs.

✅ **Y el orden, también arreglado el 6-sep** — era el último obstáculo para meter estos samples
en el corpus. Las nueve fachadas (`adc`, `gpio`, `i2c`, `spi`, `uart`, `pwm`, `pulse`, `wdt`,
`pico`) escribían con **`printf` directo a stdout**: otro buffer, otro orden. Sus **42 llamadas**
pasan a `bpvm_out()`, que entrega al mismo `emit_text` que usa el `print` de BasicPlus.

**Resultado, comparando sólo stdout, que es lo que dice el contrato:**

| sample | antes | ahora |
|---|---|---|
| `BusBug` | miVM moría | ✅ **7 líneas byte-idénticas** |
| `AdcDemo` | texto Y orden distintos | ✅ **16 líneas byte-idénticas** |
| `MathRango` · `MathTest` | ok | ✅ 29 y 17, sin cambio |

📌 **Y en placa hace lo que faltaba**: esos mensajes ahora **viajan por el wire**. Verificado en la
Discovery — el aviso de `#469` llega al PC, cuando antes se quedaba en la consola de la placa:

```
--- Adc.Channel demo ---
[adc] initChannel: esta placa NO REGISTRA BACKEND DE ADC.
      No es que la lectura sea 0: es que no hay de donde leerla.
```

⚠️ **Dos cosas que costaron un intento cada una y quedan escritas:**
- **`bpvm_diag` YA EXISTÍA** (`bpvm_util.c`, `#355`) y es **otro canal**: diagnóstico a stderr o al
  log persistente, con sink enchufable. El choque de nombres lo cazó el enlazador, y fue la pista de
  que son dos cosas distintas — por eso el nuevo se llama **`bpvm_out`**. El reparto queda escrito
  en `include/bpvm_out.h`: **`bpvm_diag` = diagnóstico; `bpvm_out` = salida del programa**, sujeta a
  la paridad byte-idéntica.
- El mensaje del ADC llegaba **truncado a media palabra** (el buffer de `bpvm_out` son 256 B).
  Acortado. Un aviso cortado es peor que uno corto.

✅ Compilan las cinco familias con el camino nuevo: Pico (ninja), C6 (`idf.py`, y con él toda la
familia ESP32) y Discovery (CubeIDE) — más el host y el simulador. `io_smoke` sigue en `[status=OK]`.

#### ✅ `#476` — ~~las DOS tablas de builtins se mantienen A MANO, y divergir no hace ruido~~ (abierta 5-sep · **CERRADA 6-sep**, `15e9b170`: `make check-builtins`)

**El código lo denuncia por escrito**, `bpgenvm-c/src/builtins.c:355`:

> *«⚠️ Estos números se escriben **A MANO** aquí y tienen que ser el `ordinal()` del enum
> `Builtin` de miVM. Si divergen, la VM-C ejecuta **OTRO builtin** y el síntoma no apunta a esto.»*

Y otra vez ocho líneas más abajo, para los cuatro de `Math`: *«si aquí y allí no coinciden, un
`.mod` ejecuta otra función y **no lo dice nadie**»*.

📐 **El tamaño**: **226 builtins** en dos tablas paralelas, acopladas por el `ordinal()` de un
enum de Java. La defensa es una **convención** — «añadir al final», escrito en un comentario — no
un mecanismo. No hay generador: `bpgenvm-c/scripts/` sólo tiene los censos y los `regen_mods`.

✅ **HECHO el 6-sep** — `bpgenvm-c/scripts/check_builtins.py`, y `make check-builtins`. Lee el
orden del enum `Builtin.java` (el id **es** el `ordinal()`) y los números escritos a mano de
`builtins.c`, y **falla nombrando la entrada** si no coinciden. Texto contra texto, sin build ni
placa. Convierte un fallo silencioso con el síntoma desplazado en un error de construcción.

**Y con su control negativo, que es lo que lo hace un instrumento y no un adorno**: metiendo a
propósito `HYPOT_F = 229` donde miVM dice 230, el guion falla y lo dice con nombre y apellidos —
*«BUILTIN_HYPOT_F = 229 pero en miVM su ordinal es 230»*, salida 1. Restaurado, vuelve a verde.

📊 Estado hoy: **226 entradas de la VM-C, las 226 casan**. Y salen a la luz **6 builtins que sólo
existen en miVM** y que la VM-C no implementa (`HEAP_FRAG`, `HEAP_MAP`, `INPUT`, `LIST_DIR`,
`PATH_ABSOLUTE`, `PROMPT`) — comprobado que no están con otro nombre: cero menciones en
`builtins.c`. No es un error (hay builtins de sólo-host), pero **hasta hoy nadie tenía la lista**.

📌 **La reforma de verdad es de V7** (el módulo raíz: que las dos tablas se **generen** del
fichero único). Cuando llegue, este guion se tira: habrá sido el andamio.

#### ✅ `#477` — el oráculo EXACTO del GUI existe desde V4 y NADIE lo ejecutaba (abierta 5-sep · **CERRADA el 7-sep**, 1e46f346 + cd03ded7)

`Gui.__guiDumpTree()` vuelca el árbol de widgets y **está implementado en las dos VMs con paridad
declarada byte a byte** (`bpgenvm-c/src/gui.c:1156`: *«byte-idéntico a `GuiBackend.dumpTree` de
miVM»*). Comprobado midiendo hoy: **idéntico en 6 samples** (GuiGeomDemo, GuiDemo, ChartDemo,
GuiTableDemo, GuiValueDemo, GuiCheckDemo).

🔴 **Y sin embargo: 22 samples lo llaman y el corpus del arnés no incluía ni un sample de GUI.**

⛔ **Corrección mía, y es la parte importante de esta ficha: yo escribí aquí que «el `check` de
`compat.sh` lleva DESACTIVADO desde V4». Es FALSO, y en dos capas.** Lo que Eduardo desactivó el
17-jul fue la compatibilidad **BINARIA** con V2/V3 —imposible de sostener cuando el formato del
`.mod` cambia a propósito— y lo dejó escrito en la cabecera del propio fichero: *«La red pasa a
ser: PARIDAD VM-Java <-> VM-C + la batería de tests»*. El `check` **se ejecuta**, y lo que ejecuta
es exactamente `check_parity`. Medido antes de tocar nada: **38 PASS, 0 FAIL, 0 SKIP**. El arnés
no estaba muerto: estaba **verde y con el corpus corto**. El diagnóstico bueno ya estaba escrito
en `#440` —*«el corpus tiene 16 casos y ninguno toca i2c, spi, uart, gpio, adc, pwm, pulse ni
neopixel»*—: el problema es el **corpus**, no el arnés. Dos veces di por muerta una herramienta
viva sin abrirla; es *dudar del instrumento* aplicado al revés.

O sea: la verificación automática de la GUI estaba construida, era exacta, era gratis — y no
entraba en la red. Antes de construir la segunda vía (capturar, comprimir, bajar, comparar
imágenes: `#475`) había que enchufar la primera.

⚠️ **Tres agujeros del oráculo, medidos, que hay que conocer antes de fiarse de él:**
- **El chart es invisible**: `ChartDemo` crea 2 series y N puntos, y el árbol saca `chart [200x120
  align=0 +0,0]` y nada más. `bpvm_gui_create_chart` no pone `has_value` y `dump_node` no tiene
  rama para chart — aunque el modelo SÍ guarda los datos en `n->cdata`.
- **Es ciego a la rotación**, y hay una prueba incómoda: en la Discovery
  `bpvm_gui_disp_set_rotation` es un **no-op declarado**. Tras `setRotation(90)` el dump dice
  `screen [480x800]` en host y en placa — **oráculo VERDE** — y el panel sigue en 800x480.
- **Color y fuente son render-only por contrato**: la regresión del `GuiColorDemo` cian no la caza.

⚠️ **Y una tesis mía que quedó FALSADA al medirla**: dije que el árbol sobrevive al cambio de
resolución salvo el `WxH` de la línea `screen`. **Falso**: `bpstdlib/Gui.bp:824-825` — `Window()`
lee el tamaño de la pantalla, así que **todo el camino Forms cambia tres líneas o más**. Medido con
`samples/formev/out/FormEv.mod` a `--screen=240x240` y a `800x480`. Es *censar por la primitiva, no
por el nombre*, literal: el grep sobre los samples daba cero porque el mecanismo vive un piso arriba.

⚠️ Y miVM tiene la pantalla **clavada a 480x320** sin forma de cambiarla (no hay `--screen` en su
parser). La VM-C host sí puede disfrazarse de la placa. Cualquier normalización tiene que contar con eso.

✅ **HECHO el 7-sep** (`1e46f346`, `cd03ded7`). El corpus pasa de **38 a 45 casos** y por primera
vez hay GUI dentro. La `--screen` no hizo falta: las dos VMs de host arrancan en **480x320**, así
que el volcado se compara directo.

🔑 **Y el bloqueo que impedía meter GUI no era del lenguaje: era una ventana sin cerrar.** Esta
ficha decía —lo escribí yo— que los samples de GUI no podían entrar *«porque miVM NO TERMINA los
samples de GUI»*, y lo daba por un límite de la VM de referencia. La causa real: el `JFrame` de
`GuiBackend` se crea y **no se destruye nunca**, y el EDT de AWT no es demonio, así que con una
ventana realizada la JVM no sale aunque no quede un solo hilo BP. Por el camino viejo no se veía
—`Gui.run()` sólo vuelve al cerrar la ventana, y el `DISPOSE_ON_CLOSE` ya la había destruido—; en
cuanto `G1` sacó el lazo a su propio hilo BP y apareció `Gui.stop()`, quedó a la vista.

📐 **Cómo se vio, que es la forma limpia:** `GuiParidad.bp` daba la **misma salida byte a byte** en
las dos VMs y sin embargo la VM-C salía con código **0** y miVM con **124** (cortada por timeout).
Misma salida, distinto final — el diff decía verde y el proceso decía otra cosa. Arreglo:
`gui.shutdown()` en el fin de ejecución de la VM, que es la paridad con la VM-C (allí la ventana
SDL muere con el proceso). Las dos salen con 0.

🧪 **Los samples nuevos, con la forma que pidió Eduardo** —*«hay que hacer nuevos test y utilizar
`Gui.start()`… `Gui.Start(); pause(5000); Gui.Stop()`»*—: construir, arrancar el bombeo en su
hilo BP, dejarlo asentar, pararlo y **entonces** volcar. Terminan solos, sin que nadie cierre una
ventana. `GuiParidad.bp` (panel/label/button/checkbox/toggle/slider/bar/led/table) y
`GuiParidad2.bp` (dropdown/textarea/listbox/spinbox/tabview con dos páginas/chart).

↩️ **La vía que se descartó, y por qué importa:** primero probé a meter los **seis** samples de GUI
que ya existen cortándolos por `timeout`. Funcionaba —49 PASS— pero tardaba **3m37s** y era *flaky
por diseño*: con el corte a 1 s se vio que `GuiValueDemo` imprime `[slider] cambió` en una VM y en
la otra no, según quién llegue antes. Un rojo aleatorio es peor que no tener red. La vía de Eduardo
da **45 PASS en 1m00s** y es determinista.

🕳️ **Un agujero del arnés que destapó el sample nuevo**, y es el que la ⏭️ vieja anticipaba a
medias: `filt()` borraba **todas** las líneas en blanco, no sólo las del banner. Como el volcado
mete el texto del widget crudo (un dropdown de tres opciones ocupa tres líneas), una VM que
emitiera una línea vacía de más donde la otra no emite nada salía **VERDE**. Ahora sólo se quitan
las de los **bordes**. Comprobado con un control: con un doble de la VM-C que cuela un blanco en la
posición 5, el check saca FAIL con el diff enseñándolo; colándolo en la 2 sale verde, y está bien
—esa cae en el borde—.

➕ **Y de paso, tres endurecimientos del arnés:**
- **Un caso mudo en las DOS VMs ya no es PASS, es FAIL.** Dos salidas vacías coinciden, pero eso no
  es paridad: es un caso que no ejecutó nada. Es el falso-PAR contra el que ya avisaba el arnés
  (*«0 PASS no es verde»*) un piso más abajo: por caso en vez de por tanda.
- `VM_TIMEOUT` (25 s) en `run_vm`, para que un sample colgado por un bug no wedgee la tanda entera.
- Entran cinco casos no-GUI que ya existían y estaban fuera de la red: **`BusBug`** (el reactivo de
  los 8 builtins de bus de `#440` — el arnés tenía el reactivo delante y no lo usaba), `AdcDemo`,
  `ArgDemo` (ejercita `RUN_ARG` de `#412`), `MathRango` y `mathtest`.

✅ **Y el arnés sabe ponerse en ROJO**, que es lo único que hace útil a un verde: con un doble de la
VM-C que ensucia una línea (`s/^0/X/`), el check saca FAIL por sample con su diff y **sale con
código 1**; sin sabotaje, código 0.

⏭️ **Lo que queda**, y no es de esta ficha: los tres agujeros del oráculo siguen abiertos (el chart
no vuelca sus datos, es ciego a la rotación, y color/fuente son render-only). Para eso está la
segunda vía, `#475`.

#### 🧱 `G2` — REVISIÓN DEL MODELO GRÁFICO — ✅ **CERRADO el 11-sep** (`50fcbc46` · `65a50f0e` · `65e9f558`)

**De dónde sale.** Analizando `C1` (serializar la ventana a JSON) apareció que **el modelo BP de la
GUI no era un modelo**: el árbol vivía sólo en C, los wrappers de los hijos se tiraban al cargar un
`.win`, y `destroy()` prometía una cascada que el backend no hacía. Eduardo, 11-sep: *«esto no es
`C1`, es un hito en sí mismo, algo como "Revisión del modelo gráfico"»*.

**Los dos planos** (Eduardo): *«el plano LVGL y el plano GUI de BP. Lo que estamos definiendo es que
cuando se destruya un componente BP se destruya él mismo, sus hijos si los hay, y en el plano LVGL los
widgets.»* Cada plano tiene su árbol; **el nuestro manda** y el de LVGL lo sigue.

### El diseño, DECIDIDO (Eduardo, 11-sep)

**El principio que ordena todo lo demás** (Eduardo): *«El modelo LVGL es su modelo, pero el modelo de
ventanas y componentes BP es NUESTRO modelo, y tiene que ser lógico y coherente. **Si un objeto
contiene a otro, debería poder verse.**»* Lo que había —el árbol sólo en C, los wrappers de los
hijos tirados al cargar, `destroy()` prometiendo una cascada que el backend no hace— no lo cumplía.

**1. Cada contenedor tiene a sus hijos** (no una lista plana en `Window`): modela la contención,
permite el `toJson()` recursivo tal como lo describió, la destrucción en cascada **en BP**, y converge
con miVM, cuyo `Node` ya tiene `children`.

**2. Una clase `Container` entre `Component` y los que contienen.** *«Tenemos componentes simples,
que no contienen otros y descienden de `Component`. Luego los que pueden contener a otros —en Swing
sería un `Panel`—. Serían éste y los que lo hereden los que deberían tener una `OwnerList` de sus
hijos.»* Las **16 hojas** no pagan nada; los contenedores llevan la lista.

**3. `Button` es un `Container`, con dos constructores.** Salió de un problema real: *«el botón es un
caso especial: acostumbra a tener un texto nada más, pero también hay botones con iconos, con icono y
texto…»*. La primera idea —`Button` hoja y un `ButtonEx` que herede y contenga— **no cabe en BP**:
sin interfaces de clase (la gramática las lista como *«IDEAS… no en el lenguaje»*) un `ButtonEx extends
Button` nunca sería un `Container`, y el `parent` de los constructores no tendría tipo. Decisión:
*«todos los botones pueden tener varios componentes; un constructor normal y otro para el caso más
frecuente, el botón con un texto»*. Y el `Button` de hoy **ya era eso**: su constructor hace
`Label(this, text)` y tira el wrapper.

```
Component                          hoja: geometría, eventos, delete(), parent: Container
 └─ Container extends Component    var owner children: OwnerList · destroy() en cascada · toJson() recursivo
      ├─ Screen · Panel · Window · TabPage · Tabview
      └─ Button                    Button(parent)  ·  Button(parent, text)
```

**4. Los constructores pasan a `parent: Container`.** Es una **garantía en compilación**: meter un
`Label` en un `Checkbox` deja de compilar. ⚠️ Toca **API publicada** (`Gui.bp` está en V4 y V5); sólo
rompe programas que ya estaban mal según nuestro modelo, o que declararon el padre con tipo estático
`Component`. Va implícito en la decisión, pero queda dicho.

**5. Alta y baja simétricas, o el modelo miente:** cada widget se registra en `parent.children` al
crearse (un solo punto en la base) y **`delete()` lo quita de la lista del padre**. `children` es
**`OwnerList`**, no `List`: es el que existe para que liberar el contenedor libere los elementos.

**6. El serializador vuelve a BP (A), y esta vez por la razón correcta**: el árbol existe en BP.
`toJson()` en `Component` emite lo suyo; `Container` lo sobrescribe añadiendo `"children": [...]`;
cada hoja añade sus campos. Formato: **el de los `.win`**. Prueba: **ida y vuelta con `main.win`**,
al corpus de paridad. Detalle fijado para que la ida y vuelta no sorprenda: `Button(parent, text)`
guarda su `Label` en un campo `owner` propio y `toJson()` lo emite como **`"text"`**, no como hijo —
así el formato de fichero sigue siendo el que ya existe.

**7. Lo que se arregla en C igualmente**, porque es defecto de V6 hoy: `create_node` **reutiliza
ranuras** y `bpvm_gui_delete` **cascadea en el modelo** (por padre, como `dump_node`). Aunque BP lleve
la cascada, un `delete()` directo no debe dejar huérfanos en la tabla de 512.

**8. Lo que espera al destructor (`#491`, V7)**: `owner` en las properties declaradas de la ventana.
Sin destructor, liberar el wrapper no libera el nodo, así que hoy no aporta.

**Y la otra mitad de `C1` sigue en pie**: la captura de píxeles. El árbol dice qué modelo hay; el

### ⚠️ Destruir algo YA destruido — los cuatro sitios (Eduardo: *«hay que preverlo»*)

| | dónde | hoy | regla |
|---|---|---|---|
| **1** | BP→BP: segundo `delete()` sobre el mismo handle | `node_for` no lo encuentra → no-op **silencioso**. Seguro | ✅ **DECIDIDO (Eduardo, 11-sep): idempotente y silencioso, por contrato.** *«Un delete de algo que ya no existe no tiene consecuencias, ni destruye nada ni genera errores. Digamos que es un pequeño error que no debería suceder pero que toleramos.»* Es lo que ya hace el código; ahora es el contrato, no una casualidad |
| **2** | C→LVGL: `lv_obj_delete` sobre un `lv_obj` que LVGL **ya liberó** por su cascada | 🔴 **ROTO HOY**: `clean()` marca `used=0` sólo a los hijos directos (`gui.c:1051`); los **nietos** quedan `used=1` con `lv` colgando → un `delete()` posterior es **use-after-free en LVGL** | la cascada en C recorre TODO el subárbol; **nunca queda un `lv` colgando** |
| **3** | reutilizar ranuras: un handle viejo que alias a un widget nuevo (ABA) | ✅ **seguro por construcción**: `node_for` **busca por handle** (`gui.c:157-162`), no indexa, y `g_next_handle++` no se repite | 🔒 **INVARIANTE**: se reutilizan **ranuras**, **jamás números de handle**. Identidad ≠ posición — el principio de los handles con generación de V4. Que nadie «optimice» `node_for` a un índice |
| **4** | wrapper BP muerto con nodo C vivo: el `objptr` de los eventos (`bind_click`) | un evento LVGL despacharía a memoria BP liberada | **orden**: el nodo C muere **antes** que su wrapper. `destroy()` lo garantiza; soltar la ventana sin `destroy()` no — es el hueco del destructor (`#491`), hoy cubierto sólo por el reset de fin de RUN |

🔑 **Una sola regla de orden resuelve 2 y 4**: la cascada baja **hijos antes que padre**, y en cada
nodo **C antes que BP**. Cada hijo borra su propio `lv_obj` (LVGL no cascadea por nosotros → no hay
`lv` colgando) y el wrapper sólo se libera cuando su nodo ya no existe.

### Los defectos de HOY que este hito arregla de paso (son de V6 aunque el hito no existiera)
- `create_node` **no reutiliza ranuras** (`gui.c:174`): la tabla de 512 sólo crece dentro de un RUN.
- `bpvm_gui_delete` **no cascadea en el modelo** (`gui.c:1073`): descendientes huérfanos.
- `bpvm_gui_clean` deja **nietos con `lv` colgando** (el punto 2 de arriba): crash posible.

### Orden de construcción
1. Los tres arreglos de C (son defectos de hoy y todo lo demás los pisa).
2. `Container` + `OwnerList` + alta/baja simétricas + `parent: Container` (el cimiento).
3. `destroy()` en cascada con la regla de orden.
4. `toJson()` recursivo, formato `.win`, **ida y vuelta con `main.win`** al corpus.
Y después, `C1` (la captura), que se apoya en esto.

### ✅ CONSTRUIDO (11-sep) — los cuatro puntos, y lo que cada uno destapó

**1. Los tres arreglos de C** (`50fcbc46`): `create_node` reutiliza ranuras (`memset` al reusar;
**se reusan ranuras, nunca handles**: `g_next_handle` sigue monótono), `bpvm_gui_delete` cascadea
por padre (`delete_hijos`, recursivo) y `bpvm_gui_clean` borra los hijos por el modelo antes del
`lv_obj_clean`. miVM (`GuiBackend.clean/delete`) igual: hijos antes que padre, baja del padre.
`GuiChurn.bp` al corpus (600× panel + 5 labels + delete: la tabla ya no crece).

**2. `Container`** (`65a50f0e`): `Component.attach(padre)` es el único punto de alta;
`Component.delete()` la baja simétrica; `Container` con `var owner children: OwnerList`,
`register/unregister/childCount/childAt`, `delete()`/`clean()` de atrás hacia delante (cada
`delete()` del hijo lo quita de la lista). `Screen/Panel/Window/TabPage/Tabview/Button` extienden
`Container`; `Button(parent, text := "")` con un solo constructor (decisión de Eduardo: el parámetro
por defecto, porque la interfaz de módulo no exporta sobrecargas de constructor). **`OwnerList`
estaba HUECA** (no era owner de nada) y se arregló con `backing()` en `List` + `grow()` virtual
(`#492`). `GuiArbol.bp` y `OwnerCascada.bp` al corpus; verificado en la Pico (tras vaciar `/app`,
`#493`) y en la Discovery.

**3. La cascada** vive en `Container.delete()`: no hizo falta más.

**4. `toJson()`** (`65e9f558`): `Component.toJson(): Json.JsonObject` emite `type`, `name`, lo
propio de la hoja (`jsonProps()`), `align` **por nombre** + `x`/`y` como desplazamientos, o `x`/`y`
explícitos si mandan ellos, `width`/`height` **sólo si no son auto**, `fontSize`, y los `clic`/`change`
**explícitos** del `.win` (los derivados de `name` no se guardan: se derivan). `Container.toJson()`
añade `children`; `Button` emite `text` desde su etiqueta y **no la lista como hijo** (`jsonChild()`).
`toJsonText()` da el texto sin que el programa importe `Json`. Cinco intrínsecas de una línea ×2 VMs
(`234..238`): `__guiGetAlign/AlignDx/AlignDy` (`align = -1` si mandan `x,y`) y
`__guiGetAuthWidth/Height` (`-1` = auto) — la geometría **autorada**, la del `dump_node`; los `get`
de siempre devuelven el píxel computado y eso no se serializa. **`samples/GuiWinJson.bp`** al corpus:
`main.win → árbol → J1 → árbol → J2`, `J1 == J2` y `dumpTree` idéntico; en la Discovery (LVGL,
800×480) el mismo JSON salvo el tamaño del panel. **58 PASS.**

**Lo que la ida y vuelta destapó** (por esto existe la prueba):
- 🐛 **El cargador perdía la `y` del `.win`**: `applyCommon` aplicaba `x`/`y` como posición y luego
  `align(…, 0, 0)` los pisaba. El `"y": 12` de `main.win` **nunca había llegado al modelo** (medido:
  `align=1 +0,0`). Ahora con `align`, `x`/`y` son desplazamientos del ancla (lo que LVGL llama offset).
- 🐛 **Regresión de G2-2, silenciosa**: `Keyboard.attach(ta)` pasó a resolver al nuevo
  `Component.attach(padre: Container)` — el teclado se quedaba **sin textarea** y con un `Textarea`
  por `parent`. El arnés no lo veía (el stdout es igual). Ahora `Keyboard.setTextarea(ta)`, y el
  teclado se engancha al árbol como todos (`GuiListKbd.bp` actualizado).
- 🐛🐛 **`#494`: el GC de la miVM descarrilaba desde el 15-jul** — ver la ficha. Salió porque el
  sample creaba un `""` temprano y el guardián del heap gritó por stderr.

**Lo que `toJson()` NO serializa todavía** (no hay getter en el backend; se añaden cuando `C1`/`T1`
los pidan, son una línea cada uno): `min`/`max` de Slider/Bar/Spinbox, `options`/`items` de
Dropdown/ListBox, `buttons` de Msgbox, `bgColor`/`textColor` (el nodo C ni los guarda), el título de
las pestañas, y nada de Chart/Table/ImageView. 📌 Y una nota para `C1`: `Window.load()` fija
`width`/`height` del raíz al tamaño de la pantalla cuando el `.win` no los trae, así que el JSON de
una ventana cargada **lleva el tamaño del panel** (480×320 en el host, 800×480 en la DK2): un `.win`
guardado desde `toJson()` no es portable entre pantallas hasta que eso se decida.

**Y dos cosas del arnés** (`compat.sh`): un sample declara los ficheros que necesita con
`// recurso: ruta/desde/la/raíz` en su cabecera (así `GuiWinJson` carga **el** `main.win` de
formdemo, no una copia); y el stderr **ya no se tira entero**: una línea `HEAP INCONSISTENTE` sale
como salida y rompe la paridad.

#### 📸 `#475` — CAPTURA DE PANTALLA EN EL MICRO: el testigo de las pruebas gráficas — ✅ **CONSTRUIDA el 11-sep** (`0791bb3e`; C6 verificada; P4 por ver en placa)

### ✅ LO CONSTRUIDO (11-sep) — `C1`, de punta a punta en un día

**`Gui.shot(path)`** escribe la pantalla tal como LVGL la dibujó a un fichero **`.shot`** — formato y
todas las decisiones en **`docs/SHOT_FORMAT.md`** — y el PC lo pasa a PNG con **`tools/shot2png.py`**
(sólo stdlib de Python, descompresor LZ4 de bloque propio). Se baja con el `GET` de siempre (`wire_serie.py
get <remoto> <local>` ya guarda). Cero verbos nuevos.

**Lo que el mapa previo corrigió del diseño** (6 lectores + crítico, antes de escribir una línea):
- 🔴 *«Las cuatro pantallas van en PARTIAL»* era **falso**: el host con ventana SDL va en **`RENDER_MODE_DIRECT`**
  (framebuffer entero, XRGB8888). Por eso el gancho no son los cuatro `flush_cb` de familia sino
  **`LV_EVENT_FLUSH_START`** en `gui.c`: cero líneas por familia, llega antes del swap in situ del C6, y
  vale igual en PARTIAL (placas, host `--no-screen`) y en DIRECT.
- 🔴 *«LZ4: 1 056 B de RAM»* sólo con `LZ4_MEMORY_USAGE 10`; el defecto (14) son **16 416 B en la pila** de
  la tarea `vm` — 16 KB en la DK2 y 8 KB en el C6. Puesto a 10 en `lv_conf.h`, y el estado se reserva en
  el heap con `LZ4_sizeofState()` (el tamaño lo dice el `lz4.c` compilado, no una macro).
- El host ya capturaba (`BPVM_GUI_SHOT_MS`, F12) pero por `lv_snapshot_take` + PNG en el cwd con numeración
  que reempieza: **no es el camino de placa** y no se reutilizó. Se queda como estaba (útil para el ojo).
- `build/liblvgl.a` **no dependía de `lv_conf.h`**: encender el LZ4 enlazaba la lib rancia en silencio.
  Ahora depende (regla en el Makefile).

**Verificado**: host ventana (DIRECT: 14 franjas de 24, 4,4 KB, 70×) · host `--no-screen` (PARTIAL: 32
bloques de 10 filas) · miVM (Swing, códec 0) · **la Discovery**: 800×480, 20 bloques, **6 956 B (110×),
`GET` en 643 ms** a 115 200 — y tres capturas del mismo programa con dos reflasheos en medio,
**byte-idénticas**: el oráculo de regresión placa↔placa del diseño existe de facto. Compila en Pico, C6, P4
y DK2. Arnés **59 PASS** con `samples/GuiShot.bp` (en el arnés las dos VMs dicen «sin pantalla»; miVM corre
ahora con `-Djava.awt.headless`, y el arnés se niega con nombre si el host está compilado con LVGL).

**Lo que la primera captura enseñó**: *bombear antes de capturar* — el tema de LVGL anima el check del
checkbox (~100 ms) y sin una vuelta de lazo sale vacío con `val=1` en el modelo (es lo que se **ve**, no el
modelo); y **en la DK2 `/lib` se vacía en cada arranque** (`clear_lib`): los módulos GUI que no van en la
imagen viven en `/app`, que es donde los deja el IDE.

**La revisión adversaria** (3 lentes → 11 hallazgos → 2×2 refutadores → 2 confirmados, 7 ya arreglados en
vuelo): el tope `BPVM_SHOT_MAX` se comparaba con el `compressBound` de la franja, no con lo que ocupa —
en el C6 se comía 12 de los 40 KB y la misma pantalla cabía o no según cómo troceara el driver. Ahora el
compresor escribe con salida limitada y el buffer crece hasta el tope: medido con 74 KB ruidosos, y con un
tope de 20 KB «no cabe» y sin fichero. También: 0 bloques ya no es éxito; `remove()` no borra directorios;
miVM crea la ventana si `shot()` llega antes de la primera vuelta del hilo (como la VM-C).

**El C6, la placa que el diseño temía (misma tarde)**: la primera captura dijo **«sin memoria»** — no por
el tope de 40 KB sino por la **DRAM de plataforma**: tras LVGL quedan ~26 KB libres con un bloque mayor de
14 KB, y el `tmp` de 11,5 KB + 16 KB iniciales de salida no cabían. Arreglo en el común: **en placa la banda ya
es RGB565 y contigua, se comprime desde ahí sin copiar** (el `tmp` sólo existe en el host, que convierte
de XRGB8888, y se reserva la primera vez que hace falta), y el buffer de salida arranca en 4 KB. Resultado:
**240×240, 10 bloques, 2 862 B (40×), `GET` en 8 ms** por USB-JTAG, colores correctos (o sea, el gancho llega
**antes** del swap in situ del C6: si llegara después el azul saldría otro color), y el mínimo histórico de
DRAM durante el RUN subió de 9 072 a 15 824 B (la captura cuesta ~5 KB). Host y DK2 siguen dando lo mismo.

⏭️ **Queda**: probar en **P4** (rotación por software en el flush: la captura es la lógica) cuando se
conecte; y, si `T1`/`D1` lo piden, un visor de `.shot` en el IDE (hoy `shot2png.py`). De aquí salió
**`#495`** (V7).

---


🧩 **AMPLIACIÓN DE EDUARDO (11-sep, 0:40 — sólo análisis): SERIALIZAR LA VENTANA A JSON.**
*«Si tenemos una ventana LVGL montada con todos sus componentes dentro, lo que necesitamos es
serializar la ventana a Json. En realidad solamente necesitamos que `Component` tenga su
serializador, y como todos los objetos LVGL heredan de él ya lo tendríamos resuelto. Para serializar,
un componente se serializa y después en un bucle pide que se serialicen sus componentes, y así
recursivamente.»*

📐 **Está más construido de lo que parece.** La cabecera de `src/gui.c` lo dice: *«`dump_tree` produce
el MISMO texto byte-a-byte que miVM → **paridad (sobre el árbol, no píxeles)**… el modelo sigue siendo
la fuente de verdad del `dump_tree`, así que **la paridad NO depende de LVGL**»*. O sea que ya existe
el modelo completo (tipo, geometría, valor, rango, celdas de tabla, series de chart, asset de imagen),
ya se serializa, y **su salida ya es un artefacto con paridad demostrada**. Lo que falta es el
**formato** (JSON) y la **forma** (recursiva por componente en vez de plana).

Tres cosas del análisis:

1. **El nodo guarda `parent`, no lista de hijos** (`gui.c`, `gui_node.parent`). «Pide a sus hijos que
   se serialicen» se resuelve recorriendo la tabla filtrando por padre: O(n) por nodo, O(n²) total —
   con el tope de 512 nodos son 262 000 comparaciones, nada. **No hace falta añadir listas de hijos.**
2. 🔑 **La decisión de verdad es dónde vive el serializador: en BP o como builtin.** El planteamiento
   («que `Component` tenga su serializador») apunta a BP, y creo que es lo correcto por un motivo que
   va más allá de lo elegante: **escrito en BP se escribe UNA vez**. `dumpTree` hoy está escrito DOS
   (C y Java) y hay que mantenerlo byte-idéntico a mano — la duplicación que `#473` lleva todo el día
   persiguiendo. Y ya existe el módulo `Json`. ⏭️ **El precio, y es lo primero que hay que censar**:
   `Component` tendría que exponer como propiedades todo lo que hoy sólo ve el volcado en C.
3. ⚠️ **Y esto NO sustituye a la captura de píxeles: la complementa.** El árbol dice qué *modelo* hay,
   no qué se *ve*. El fallo que tiene hoy la P4 —`GuiColorDemo` termina en 480 ms sin una sola línea
   de LVGL— **sería invisible en un JSON perfecto**. Así que el JSON es el instrumento **automático y
   comparable** (diffeable, y con paridad ya demostrada) y la captura es la única que caza *«el modelo
   está bien y no se dibuja nada»*.

📋 **EL CENSO, HECHO (11-sep).** Lo que emite `dump_node` (`gui.c:1192`, la verdad del modelo) contra
lo que `Component` expone hoy en BP:

| campo del modelo | ¿lo ve BP? | dónde |
|---|---|---|
| `w`, `h`, `x`, `y`, `scroll`, `font_size` | ✅ | `width`/`height`/`x`/`y`/`scrollDir`/`fontSize` en `Component` |
| `text`, `value`, celdas de `Table`, asset de `ImageView` | ✅ pero **en las subclases**, no en la base | correcto para un serializador OO: cada subclase añade lo suyo |
| `type` | ❌ como propiedad; BP lo sabe **por la clase** | `typeName()` por subclase, puro BP |
| `align`, `dx`, `dy`, `pos_set` | ❌ **sin getter** — `align()` sólo escribe | falta |
| `readonly` | ❌ | falta |
| **los HIJOS** | ❌ **no hay forma de enumerarlos desde BP** | **el único bloqueo real** |
| `name` | al revés: BP lo tiene y el modelo C **no lo emite** | ventaja del serializador en BP |

Las intrínsecas GET que existen hoy: `Checked, FontSize, Height, ScrollDir, Text, Value, Width, X, Y`.

🔑 **Y `dump_node` YA RECURRE EXACTAMENTE como Eduardo lo describe** (`gui.c:1245-1247`: emite el
nodo y luego `for (i in nodos) if parent == handle → dump_node(hijo, depth+1)`). La forma no hay que
inventarla: hay que **exponerla**.

🎯 **EL FORMATO YA ESTÁ DECIDIDO: es el de los `.win`** (`samples/formdemo/resources/main.win`):
```json
{ "type": "Panel", "children": [
    { "type": "Label",  "text": "…", "align": "TOP_MID", "y": 12 },
    { "type": "Button", "text": "Saludar", "align": "CENTER", "clic": "onSaludar" } ] }
```
Ojo al detalle: **`align` va por NOMBRE** (`"TOP_MID"`), no por número. BP tiene las constantes, así
que emitir el nombre es puro BP. Y usar ese formato regala **la prueba**: cargar `main.win` →
`toJson()` → comparar con el original. **Ida y vuelta**, que verifica cargador y serializador a la vez.

**Las dos opciones, con su coste:**

| | **A — en BP** (`Component.toJson()` + una sobrescritura por subclase) | **B — builtin** (`dumpJson`, gemelo de `dumpTree`) |
|---|---|---|
| intrínsecas nuevas | **~7, todas de una línea**, ×2 VMs: `childCount/childAt`, `getAlign/getDx/getDy/isPosSet`, `isReadonly` | 0 |
| el serializador | **UNA vez**, en BP, con el módulo `Json` que ya existe | **DOS veces** (C y Java), byte-idénticas a mano — como `dumpTree` hoy |
| `name` y `align` por nombre | sí, sin más | el modelo C no tiene `name` |
| paridad | gratis: mismo código BP en las dos | la que se vigile a mano |
| lo que aporta de rebote | las 7 intrínsecas **sirven solas** (hoy un programa no puede preguntar la alineación de un widget) | nada |

🧩 **LA PREGUNTA DE EDUARDO QUE LO REENCUADRA (11-sep)**: *«Si un componente no guarda una lista de sus
hijos, cuando se destruye una ventana ¿cómo sabe que también se han de liberar todos sus componentes?
Esto lo hablamos en su día: los componentes deberían declararse con `owner` y al destruir la ventana
deberían destruirse todos.»*

**Respuesta, comprobada: NO LO SABE.** Tres niveles y tres conductas:

| nivel | `delete()` de la ventana | `clean()` |
|---|---|---|
| **LVGL** | cascadea (lista de hijos propia) ✅ | cascadea ✅ |
| **modelo C** (`gui.c:1073`) | `used=0` **sólo el nodo**; los descendientes quedan **huérfanos ocupando ranura** ❌ | `used=0` sólo los hijos **directos**, sin nietos y **sin `node_release`** (fugan `text`/`cells`) ❌ |
| **objetos BP** | siguen vivos con un `lvglId` muerto; toda operación **no hace nada, en silencio** ❌ | ídem |

En pantalla se ve bien porque LVGL limpia — por eso nadie lo notó. Y se junta con que **`create_node`
NUNCA reutiliza ranuras** (`gui.c:174`: `g_nodes[g_node_count++]`, sin buscar hueco): la tabla de 512
**sólo crece dentro de un RUN**. Un programa que abra y cierre pantallas muere a las ~500 creaciones
con *«no se puede crear un widget sin un contenedor válido»* — la muerte de `#352`, pero **dentro de
una ejecución**. ⚠️ miVM tiene la misma fuga (su `delete` quita el nodo del mapa pero no a sus hijos,
`GuiBackend.java:583`), sólo que sin muro de 512 — y **su `Node` SÍ tiene lista de hijos**, el de C no:
divergencia estructural entre las dos VMs.

**Sobre `owner`, lo que Eduardo recordaba es MITAD Y MITAD:**
- ✅ **`var owner` SÍ está implementado** (18-ago, `#449`/`#450`): como campo emite `SET_FIELD_OWNER` y
  `FREE_REF` cascadea por los bits de propietario del descriptor, **recursivamente**. Probado con
  `samples/OwnerBp.bp` y el guardián de `#339` («0 bloques sin liberar»).
- ❌ **La GUI no lo usa, y aunque lo usara no bastaría**, por dos razones: (1) `Component` **no guarda a
  sus hijos en BP** —el enlace vive sólo en `gui_node.parent`—, así que no hay campo `owner` que
  cascadear; (2) `owner` libera **memoria del heap**, no el recurso nativo: liberar el wrapper de un
  `Label` no llama a `__guiDelete`, el nodo C y el `lv_obj` seguirían vivos. Es la otra mitad del
  problema de `#491`: **el destructor `~Clase()`**, diseñado y en `L1`/V7.

🔑 **Para que «destruir la ventana destruye sus componentes» sea verdad hacen falta las DOS piezas**:
`Component` con sus hijos como `owner` (la cascada del heap los alcanza) **y** un destructor que llame
a `__guiDelete` (liberar el wrapper libera el nodo). Hoy: la primera herramienta existe sin aplicar, la
segunda no existe.

🎯 **Y la convergencia con el serializador**: la pieza que le falta a `toJson()` —que `Component`
conozca a sus hijos— **es la misma que le falta a la destrucción**. Un solo cimiento sirve a los dos.
Eso mueve la decisión: no es sólo «7 intrínsecas para serializar», es **dar a `Component` la lista de
hijos** (en BP, como `owner`), y de ahí salen la serialización, la destrucción en cascada, y la
paridad estructural con miVM, que ya la tiene.

📜 **LO QUE SE DECIDIÓ EN SU DÍA, ENCONTRADO (Eduardo, 11-sep: *«los componentes se debían declarar en
la ventana, y así debe estar ahora»*).** Está, y es `#324` tanda 2b, en `Gui.bp` (`buildForm`):

> *«Un nombre, todo lo demás se deriva: `"name": "boton1"` → **property `boton1` de la ventana**
> (`setBoton1`) → handlers `boton1_onClick` / `boton1_onChange`. […] una ventana no tiene por qué
> declarar la property de cada widget ni atender todos sus eventos.»*

O sea: **implementado y OPCIONAL por diseño**. Si la ventana declara `property boton1`, el cargador la
rellena (`bindMember`) y la ventana tiene el componente como miembro; si no, el wrapper se crea y **se
tira** (`buildForm(child, w, win)` sin guardar el resultado) y sólo sobrevive el nodo C.

Y la cascada al destruir **se diseñó como trabajo del BACKEND**, no de BP — `Window.destroy()` lo dice
literalmente: *«Destruye la ventana → **el backend cascadea** y borra todo el subárbol»*. El backend C
no lo cumple (ver arriba). **Eso no es un hueco de diseño: es un bug de `gui.c`**, y se arregla ahí —
`bpvm_gui_delete` recurriendo por padre como ya hace `dump_node`, `clean` con `node_release`, y
`create_node` reutilizando ranuras. Unas líneas, sin tocar el lenguaje, y `destroy()` cumple lo que
promete. ⚠️ **Es un defecto de V6 hoy**, independiente de `C1`.

🔄 **Y ESTO CAMBIA LA RECOMENDACIÓN: de A a B.** Los hijos no declarados sólo existen en C, así que un
`toJson()` en BP no puede recorrerlos sin intrínsecas que **fabriquen** wrappers por tipo (`childAt`
devolvería un handle, no un objeto; para llamar `.toJson()` polimórfico haría falta una factoría como
la de `makeWidget`). Con eso, el polimorfismo de A no compra nada — los datos están en C igualmente —
y suma piezas. **El serializador tiene que vivir donde vive el árbol: en el backend**, con la forma de
`dumpTree`. Coste: escribirlo en C y en Java, byte-idéntico — el que `dumpTree` ya paga — y la ida y
vuelta con `main.win` lo vigila. Lo único que el modelo C no tiene y el `.win` sí es **`name`**: un
setter (`__guiSetName`) y una cadena por nodo, y el cargador lo rellena al construir.

~~⏭️ Recomendación revisada: B~~ — **superada por la decisión de abajo, que devuelve el árbol a BP.**

---

### 📐 `C1` CON `G2` DEBAJO — el análisis (Eduardo, 11-sep: *«si analizamos `C1` tenemos que tener en cuenta `G2`. Ahora queda claro que desde un componente podemos ir hacia todos sus hijos, así que una serialización recursiva es viable»*)

**Dos mitades, preguntas distintas, mismo camino de salida:**

| | **el árbol** (JSON) | **los píxeles** (lo medido el 5-sep, abajo) |
|---|---|---|
| contesta | qué **modelo** hay | qué **se ve** |
| se apoya en | `G2` (los hijos) | el framebuffer de cada placa |
| quién lo dispara | **el programa**: `win.toJson()` | **el programa**: `Gui.shot(path)` |
| cómo sale del micro | escribe un fichero → `GET` de siempre | ídem |
| verbos nuevos del wire | **cero** | **cero** |
| comparable en automático | **sí**: texto, y con paridad PC↔placa | no: para ojos |

**Lo que el árbol necesita ADEMÁS de `G2`**: cinco getters de una línea (`align`, `dx`, `dy`, `posSet`,
`readonly`, ×2 VMs) y un `typeName()` por clase (puro BP). El resto es BP sobre `children`.

**Dos reglas para que el JSON tenga paridad de verdad, no sólo de aspecto:**
- **No resolver el layout.** Un widget por `align` emite `align` + desplazamientos, **no** un `x,y`
  calculado — como `dump_node` y como el `.win`. Resolver posiciones metería la fuente y el panel en
  el número, y PC y placa divergirían. Sin resolver, el JSON **hereda la paridad que `dumpTree` ya
  tiene**.
- **Omitir lo «auto»**: `width`/`height` a −1 no se emiten, como no van en el `.win`. Ida y vuelta limpia.

**Lo que esto le da a `T1`, que es para lo que existen las dos mitades**: la prueba de un programa
gráfico en placa es *ejecutarlo, que escriba su `shot.json`, `GET`, y **diff contra el que salió en
el PC***. Iguales ⇒ el mismo modelo en las dos VMs, sin que nadie mire una pantalla. El PNG se mira
sólo cuando el JSON dice que todo está bien y aun así no se ve nada — el caso de la P4 hoy.

⏭️ **Orden**: `G2` entero → `C1`-árbol (barato una vez existe `G2`) → `C1`-píxeles (el plan de abajo).

### ➡️ El diseño del modelo de componentes SE FUE A SU PROPIO HITO: **`G2` — Revisión del modelo gráfico**

Eduardo, 11-sep: *«A nivel organizativo esto no es `C1`, es un hito en sí mismo.»* `C1` se queda con lo
que era: **la captura de píxeles**. El árbol, los contenedores, la cascada y el serializador viven en
`G2` (abajo, en la tabla de hitos y su sección).
fallo de la P4 sería invisible en un JSON perfecto. Si se aprueba, el orden es la cascada de siempre: las 7
intrínsecas en miVM y en la VM-C (host), `Component.toJson()` en BP, y la prueba de ida y vuelta con
`main.win` — que entra al corpus de paridad como caso nuevo.

**La idea es de Eduardo**: *«una vez hablamos de hacer una captura de pantalla en el micro… para
que cuando se probasen los programas gráficos, desde el PC se pudiese ver esa pantalla»*. Y el
refinamiento que lo cambia todo, suyo también: **capturar a un FICHERO** en vez de mandar píxeles
por el cable mientras el programa corre — *«2 o 3 órdenes: captura, comprimir, load»*.

📐 **Está medido** (11 agentes, seis barridos y cuatro lentes; el diseño largo, en
`docs/V6_IDEAS.md`). Lo que sigue son las conclusiones **con los números que las sostienen**, y
tres de ellas corrigen lo que esta ficha decía antes.

### 1. Lo que la medida CORRIGIÓ de la primera versión de esta ficha

- 🔴 **La costura de `A1` NO sirve para un verbo `SHOT` del wire.** Esta ficha decía que la
  petición entraría por la cola de control y la serviría `Gui.run()`. **Falso**: esa cola tiene
  **exactamente dos lectores en todo el repo, los dos dentro del `next_cmd` del depurador** — sólo
  se drena con la VM **parada en un breakpoint**, nunca durante un RUN. Y el STM32 no tiene
  depurador integrado. ✅ **La consecuencia es buena**: **una sola orden nueva, no tres** —
  `Gui.shot(path)` ejecutado por el hilo `vm`, y el `GET` de siempre. **Cero verbos nuevos.**
- 🔴 **El P4 SÍ tiene framebuffer completo y vivo**: el del driver DPI del IDF (`.num_fbs=1`,
  `esp_lcd_dpi_panel_get_frame_buffer`). El `fb` rojo de `gui_display_dsi.c:326` era el smoke test,
  pero no era el único. Así que **dos de las tres placas tienen framebuffer** y sólo el **C6**
  necesita captura por bandas. *(Deducido de dos lecturas; falta comprobarlo en placa — es la
  bisagra de que el P4 salga gratis.)*
- 🔴 **El FS del P4 NO está en PSRAM**: está en la partición de flash `bpdata` (`fs_ram.c` ya
  no existe). Desgasta y sobrevive al reset. *(La memoria del proyecto decía lo contrario.)*

### 2. Comprimir ya está escrito — y hay que ponerlo el PRIMERO

✅ **LVGL trae dentro del repo un LZ4 completo —comprime Y descomprime— y su `.c` YA está dado de
alta en los builds del C6, el P4 y la Discovery.** Está apagado por **una línea**,
`bpgenvm-c/include/lv_conf.h:853`. <4 KB de código, 1 056 B de RAM. Medido sobre 10 pantallas
reales: **21,8× a 152,6×**. *(Nadie ha hecho aún el build real con el interruptor a 1.)*

📌 **Y el orden de las tres órdenes se invierte: comprimir es el PRIMER paso, el fichero es lo
opcional.** Porque el crudo no cabe donde importa:

| | crudo | ¿cabe en RAM? |
|---|---|---|
| **C6** | 115 200 B | ❌ **68 264 B libres** (1,69× por encima). Con `lv_snapshot_take`, 230 400 B |
| **Discovery** | 768 000 B | ❌ comprimir «el fichero» de una llamada pide **771 027 B** contra **433 416 B de heap** (del `.map`) |
| **P4** | 1 228 800 B | ✅ la única donde la propuesta cabe tal cual |

Comprimido, el cable pasa de **107 s a 1,6 s** (RLE16) o a 7 315 B (deflate) en el P4.

### 3. El fichero: cabe, pero con una trampa medida

El FS se midió **ejecutando el `bpvm_part_defaults` del propio proyecto** con la geometría de cada
placa: C6 1 261 568 B · DK2 1 302 528 B · P4 5 193 728 (aprovisionado a 7 520 256). Los tres
ficheros caben. Pero:

- ⚠️ **En la Discovery una captura es el 74 % del FS y REESCRIBIR el mismo fichero falla con `-28`**
  — littlefs es copy-on-write y pide el doble transitoriamente. Borrar antes sí funciona. En un
  bucle de pruebas eso muerde a la segunda vuelta. *(Y si la DK2 llevara el FS de la Nucleo, la
  captura muere tras 323 584 bytes.)*
- ⚠️ **No existe escritura en streaming**: sólo hay `read_stream` (de `#453`). Escribir por bandas
  con la fachada de hoy son **N commits**, no uno — el patrón que costó 45× en `#398`.
- ⚠️ El cerrojo del FS es **grueso**: mientras `vm` escribe la captura, `io` no puede atender el
  cable. En la DK2 son 48 080 `HAL_FLASH_Program` con la ICACHE apagada.
- ✅ **El desgaste NO es problema**: ~0,75 borrados por bloque y captura, del orden de 10⁵ capturas
  (8-53 años a 50/día).

### 4. Qué NO puede ser la imagen

🔴 **La imagen no puede ser el oráculo automático.** El host rasteriza a **32 bpp** y las tres
placas a **16** (un gate de `lv_conf.h`), así que el antialiasing del texto y las mezclas salen
distintos aunque el modelo sea idéntico. El oráculo exacto ya existe y es el `dumpTree` → `#477`.
**La imagen es LA PRUEBA PARA EL OJO, nunca un diff automático.**

⚠️ Y lo que **ninguna** de las dos vías prueba: que el panel **muestre** algo. Backlight, gap,
rotación, inversión, timing, el cable de la pantalla — todo eso da captura perfecta y pantalla
negra. La captura sube un peldaño que hoy no existe; el último sigue siendo el ojo.

⚠️ **Y un límite de alcance que nadie había mirado**: en placa **no hay forma de conducir la UI**.
El host tiene guion determinista (`wait/click/shot/quit`); las placas sólo tienen un dedo sobre el
táctil. Así que **la comparación por imagen sólo cubre la pantalla inicial** hasta que exista
inyección de eventos en el device.

### ⏭️ Lo primero — y NO es lo que decía esta ficha

**Construir el camino de captura ENTERO en el host**: leer el `fb1` del display, escribir por la
fachada `bpvm_fs`, bajarlo con el `GET` del simulador. **Medio día, cero placas.**

📌 La versión anterior decía «empezar por el C6», y eso **viola la cascada del propio proyecto**
(Java → C → placa). Peor: el host captura hoy por un camino (`lv_snapshot_take` + `fopen`) que **no
es el que van a usar las placas**; si se queda así, los dos ficheros no son comparables por
construcción y la comparación se rompe antes de escribirse. Ese mismo prototipo paga de una pasada
los tres huecos que quedan: el 32-vs-16 bpp, el `--screen` sacado del propio dump, y —bajando el
draw buffer a 24 líneas— qué áreas entrega de verdad el flush del C6.

⚠️ **Precondición que nadie había nombrado**: LVGL en PARTIAL sólo llama al flush por las áreas
**invalidadas**. Sin forzar el invalidado total, la captura del C6 sale **a trozos** — plausible y
equivocada, que es el peor fallo posible. Y sólo con el invalidado total las áreas son bandas de
ancho completo en orden.
⚠️ Y en el C6 hay que copiar **ANTES** de `lv_draw_sw_rgb565_swap`, que machaca el buffer in situ.
⚠️ Trampa de artefacto en el propio host: `bp_shot_%04d` reempieza en 0001 en cada ejecución y
escribe en el cwd — una captura rancia es **indistinguible** de la nueva. Verde falso de manual.

#### ⏩ `#468` — la stdlib en flash en vez de en RAM (memoria de la C6) → **V7** (abierta 4-sep · **APLAZADA el 8-sep por Eduardo**)

⏩ **APLAZADA A V7 el 8-sep. Decisión de Eduardo, con su criterio de siempre:** *«para V6 no
inventamos cosas nuevas. Se puede plantear para V7, pero en V6 seguimos como hasta ahora.»*

📌 **Y la salida para quien la necesite HOY, que es suya y ya funciona:** *«si alguien quiere
ahorrar memoria puede montar su propio pack y subirlo»*. Eso dejó de ser teórico esta misma tarde:
hasta el 8-sep la zona de packs sólo la mapeaba el P4, y desde `630a221e` la tienen también el S3,
el C3 y el C6. O sea que el usuario que quiera sacar `Gui`/`Json` de la RAM puede hacerlo por el
camino normal, sin que nosotros toquemos la imagen.

🔮 **Y el apunte que le da forma a la ficha en V7**, también suyo: *«si en V7 LVGL lo subimos a un
pack, éste puede incluir `Gui.mod`»*. Es la pieza que faltaba — el ahorro deja de ser un mecanismo
inventado para la stdlib y pasa a ser **una consecuencia** de empaquetar LVGL, que es algo que se
quiere hacer por su cuenta. El `Gui.mod` viaja con el motor al que envuelve, que es donde debe ir.

⚠️ Lo que sigue valiendo de la medida, para cuando se retome: los **22 268 B** son reales y salen de
que el `code` se quede en flash; la **otra mitad** de los «40-50 K» es la tabla de símbolos por
referencia, que es un cambio del enlazador y va aparte. Lo de abajo se conserva porque es el
análisis que sostiene las dos cosas.

⛔ **8-sep — EL PACK NO PUEDE SER EL MECANISMO, y la objeción es de Eduardo:** *«los micros, cuando
arrancan por primera vez, no tienen la partición definida, así que no se puede usar para subir
ningún pack. La librería estándar de fábrica viene con la imagen del sistema y se copia a `/lib`.»*

📐 **Comprobado en el código, las tres patas:**
- El layout de particiones sale del **ENV** (`esp32/common/board_mgr_esp32.c:327`,
  `bpvm_part_layout(&s_env, …)`). Una placa virgen no tiene ENV escrito.
- Sin él no hay zona: *«sin zona de packs (el arranque no llegó a particiones)»*.
- Y la stdlib de fábrica son blobs **dentro de la imagen** (`esp32_mods.c`:
  `static const unsigned char core_mod[]`), que `esp32_mods_install()` copia a `/lib`.

O sea: un pack sirve para lo que el USUARIO grabe, no para lo que la placa necesita **antes** de
tener particiones. La stdlib de fábrica es justo eso.

🔑 **Pero el ahorro medido NO necesita un pack**, y esto es lo que queda por decidir. Los 22 268 B
salen de que el `code` se quede en **flash** en vez de copiarse al bloque de la VM — y los blobs de
fábrica **ya están en flash**: son `static const`, o sea `.rodata`, mapeada y direccionable. Y el
cargador ya lo contempla, no hay que construirlo:

```
loader.c:26    «base != NULL → blob ya en RAM/flash (embebido en la imagen, XIP, host)»
loader.c:105   ... const char* name_hint, int xip)      ← XIP es un PARAMETRO, no una politica
loader.c:109   if (xip && !data) return BPVM_ERR_IO;    /* exige blob mapeado */
```

⚠️ **Lo que hoy lo impide es el rodeo**: blob → se copia a `/lib` → se carga **desde el FS**, y
desde el FS no hay puntero, así que no se puede XIP. La forma sería: cargar el módulo **desde el
blob embebido** salvo que el usuario lo haya sustituido en `/lib`, que es el «shadow de desarrollo»
de `#310` pero al revés.

⏭️ **Lo que queda por decidir (Eduardo):** ¿se prueba esa vía —cargar la stdlib de fábrica en sitio
desde la imagen— o se deja la memoria de la C6 como está? **No está probada**: lo medido es el
ahorro (22 KB) y que el cargador acepta XIP con un blob; que funcione con estos blobs concretos hay
que verlo. La otra mitad de los «40-50 K» sigue siendo la tabla de símbolos por referencia, que es
un cambio del enlazador y va aparte.

📌 Y lo que la objeción **no** tumba: llevar el mapeo de la zona a `esp32/common` seguía valiendo por
sí solo, y se hizo el 8-sep (`630a221e`) — el S3 y el C6 ya tienen zona de packs para lo que el
usuario quiera grabar.

**La dirección la dio Eduardo (4-sep):** *«En cuanto al consumo de memoria es cierto que vamos
justos. Aquí lo suyo es subir el pack de la librería estándar+Json+Gui, y con eso podemos ahorrar
40 o 50 Ks.»* Antes de escribir código, lo que cuesta hoy y lo que daría, medido en la C6 con
`GuiColorDemo` (imagen 128 KB, `log=1`):

```
[bpvm-c] dep 'Gui' -> /lib/Gui.mod · dep 'Json' -> /lib/Json.mod · dep 'Core' -> /lib/Core.mod
[bpvm] tabla de simbolos: 1294 simbolos, 30554 B de nombres, 49152 B en total
[bpvm] tabla de handles: 0 -> 256 slots (2 KB dentro del heap) OK — techo del heap 63488, libre 31 KB
```

**Dónde viven los módulos.** El cargador los pone DENTRO del bloque de la VM, delante del heap
(`memory[]`: ext-table + data + code, desde `next_free_address`; `heap_start` empieza donde acaban).
Con XIP (H3.c, ya implementado para la zona de packs montada) el `code` se queda en flash y a RAM
sólo van ext-table + data. Cabeceras de los `.mod` de hoy:

| módulo | fichero | data | code | en el bloque de la VM | con XIP |
|---|---|---|---|---|---|
| Core | 13 111 | 992 | 5 039 | 6 031 | 992 |
| Json | 24 059 | 1 788 | 8 877 | 10 665 | 1 788 |
| Gui | 44 003 | 4 176 | 8 352 | 12 528 | 4 176 |
| **suma** | 81 173 | | | **29 224** | **6 956** |

→ un pack XIP con los tres devuelve **22 268 B (21,7 KB) al heap de la VM**: de 31 KB libres a ~53
tras cargar el demo. Eso es la mitad de la cifra de Eduardo. **La otra mitad es la tabla de
símbolos**: 49 152 B (1 294 entradas × 8 B, redondeadas a 16 KB, + un pool de nombres de 32 KB con
30 554 B usados) que sale del `malloc` de PLATAFORMA — está dentro de los 103 824 B de margen del
sistema, no en el bloque de la VM. Un pack por sí solo no la toca: para ahorrarla haría falta que
los nombres se leyeran POR REFERENCIA desde la flash mapeada (el pool desaparece: ~32 KB de DRAM)
— un cambio de diseño del enlazador (`bpvm_symbol_name` devuelve hoy punteros a un pool que se
realoja; los nombres del `.mod` van con longitud, no terminados en 0). Las dos cosas juntas son
los «40 o 50 K».

**Lo que ya hay y lo que falta:**
- El cargador ya carga XIP desde la zona montada y el orden de búsqueda es pack-en-ejecución → FS
  → zona (`#310`, spec §4). **El FS ECLIPSA a la zona** («shadow de desarrollo, con aviso»).
- En la familia ESP32 la zona sólo la mapea el P4 (`pack_p4.c`: `esp_partition_mmap` INST+DATA y
  `board_mgr_esp32_set_packs_view`, ~40 líneas). Llevarlo a `esp32/common` la activa en S3, C3 y
  C6 sin construir nada nuevo. La C6 ya tiene partición `packs` (la propuso `PART_DEFAULTS`).
- El frontend construye packs (`PackStep`, con el cierre de deps externas): un «pack de la
  stdlib» es un pack sin app; falta la forma de pedirlo y el botón del IDE para grabarlo.

**El nudo, que es el que decide el diseño:** con el orden actual, `/lib/*.mod` tapa al pack. Y en
`/lib` los pone (a) el instalador de `#466` en cada arranque, desde la tabla embebida, y (b) el
IDE cuando `STAT` dice que falta (Json y Gui hoy). Para que el pack cuente hacen falta tres cosas
pequeñas y coherentes: el instalador no repone en `/lib` lo que la zona ya sirve con el mismo
MAGIC+CRC; `STAT`/`bpvm_entry_resolve` miran también la zona (el IDE dirá «ya en la placa»); y la
regla de versión de `#466` se aplica igual (un módulo más nuevo en `/app` o `/lib` sigue ganando,
que para eso el FS eclipsa).

**Alternativa sin subir nada:** la tabla embebida de la imagen (`esp32_mods`: 14 módulos, 48 KB)
YA está en flash mapeada (`.rodata` es XIP en ESP32, RP2350 y STM32). Cargar XIP desde ahí —una
fuente más en el orden— ahorra lo mismo que el pack para los módulos embebidos, sin partición ni
grabación; a cambio Gui y Json tendrían que entrar en la imagen del C6 (+68 KB de flash de los 440
libres) y el instalador dejaría de copiarlos a `/lib` (con `STAT` contestando desde la tabla). El
mismo nudo del párrafo anterior, resuelto en el mismo sitio.

⏭️ Decisión de Eduardo: (1) pack en la zona (lo que él propone; sirve también para módulos que no
van en la imagen) o tabla embebida XIP; (2) si la tabla de símbolos por referencia entra en el
mismo lote. Con eso se abre la implementación, en este orden: mapear la zona en `esp32/common`
(activar), el nudo del eclipse (instalador + `STAT`), el pack de la stdlib desde el IDE, medir.

#### ✅ `#449` — la reserva de `malloc` de la Pico 2 se dimensionó para otra cosa (abierta 28-ago · **CERRADA 29-ago** · `e4957d7c`)

> ✅ **Verde en placa** (`JsonDemo`, `exit 0`, salida byte-idéntica a las dos VMs).
>
> ⚠️ **El arreglo NO fue ninguna de las tres opciones de abajo.** La 3 («que el margen se
> calcule») era la buena por instinto, pero la medida cambió la pregunta: el problema no era
> el tamaño del margen sino **quién lo llenaba**. Los dos consumidores, atacados por
> separado —
>
> - la **tabla de handles** ahora es proporcional al heap (`40a34b24`), y
> - la **tabla de símbolos** pasó de 59 KB a 20 con el pool de nombres (`e4957d7c`).
>
> El margen de 64 KB **no se ha tocado**: ya no hace falta. Y esto es lo que hay que
> recordar del episodio — *ninguna de las tres opciones que parecían el trabajo lo era*.
> Las tres discutían cómo repartir 64 KB; la medida dijo que sobraban en cuanto se dejara
> de malgastarlos.

**El número, y son dos ficheros que no se conocen:**

```
26-jul (V4, #309)   VM_SRAM_MALLOC_MARGIN = 64 KB   (pico/main.c)
                    dimensionado para "calloc/strdup por cada import"

16-ago (V5, #430)   la tabla de handles pasa a salir del MISMO malloc
                    -> 32 KB de golpe, y crece x2      (src/heap.c)
                    <- el margen NO se toco
```

Y el tope lo remata: `BPVM_HANDLE_CAP_MAX=16384` ⇒ las dos tablas a tope son **128 KB** en
un hueco de **64**. La tabla **no puede alcanzar ni de lejos su propio tope**: muere en el
primer crecimiento, porque pasar de 4096 a 8192 slots necesita 64 KB nuevos con los 32
viejos aún vivos (el `realloc` necesita ambos) — 96 KB en 64.

**Por qué nadie lo vio, y es la lección:** #430 se diagnosticó y se arregló **en la Metro**,
donde el heap se va a la PSRAM y la SRAM interna entera queda para `malloc`. El consumidor
nuevo se estrenó en la placa donde no cuesta nada y viajó a la Pico 2 en la MISMA imagen.
Es la contrapartida de la imagen única: dos placas con realidades de memoria opuestas, y
**la más estricta es la que menos se prueba**.

**Explica 7 de los 8 rojos**, y por qué morían MUDOS (era #448).

**⏩ 28-ago, tras arreglar la frontera (`_sbrk`) y la tabla proporcional**: la corrupción
desaparece y el fallo se vuelve honrado — `lib 'Core' presente pero no exporta
'Core.__init'`, o sea un `strdup`/`calloc` del cargador devolviendo NULL. **La escasez es
real y ahora se ve.** Y el consumidor gordo está medido:

```
bpvm_symbol_t = char name[128] + uint32_t = 132 BYTES POR SIMBOLO
   Json ~101 simbolos -> 13 KB      Core ~75 -> 9 KB      total ~24 KB
```

...de un margen de 64, y creciendo con `realloc`, que necesita el viejo y el nuevo A LA
VEZ (pasar de 16 a 32 KB pide 48 de pico). El `name[128]` fijo es lo caro: los nombres
cualificados reales rondan los 20-30 caracteres. Un pool de cadenas, o simplemente un
tamaño realista, dividiría eso por cuatro. **Candidato nº 1 para mañana**, y no toca
diseño: es una struct.

⏩ **Arreglo por decidir (de Eduardo, tiene contrapartida real):**
1. **Subir el margen** → menos heap. Hoy 267 KB heap + 89 pilas; 96 KB más para `malloc`
   los deja en ~200.
2. **Bajar `handle_cap_max`** → menos objetos vivos a la vez, sin tocar el heap.
3. **Que el margen se CALCULE** desde el tope de handles en vez de ser una constante
   suelta. Más trabajo, pero es lo único que evita que se repita: hoy los dos números viven
   en ficheros distintos y no saben el uno del otro.

⚠️ Aunque un programa salga verde hoy, está verde **por poco**: lo que lo salva es no llegar
a necesitar el crecimiento. Eso no es estar arreglado, es tener suerte.

#### ✅ `#448` — en la RP2350 `malloc` no fallaba: MATABA (cerrada 28-ago · `defcaae4`)

El wrapper del SDK trae `PICO_MALLOC_PANIC=1` y hace `panic("Out of memory")` **antes** de
devolver NULL. O sea que el OOM ATRAPABLE que V4 construyó a propósito (#355) —
`bpvm_alloc_raw` maneja el NULL y lanza `RuntimeError`— **no se ejecutaba nunca en esa
placa**. En el host y en las otras familias sí, y por eso el mismo programa daba error
limpio en el PC y mataba la placa.

Quitarlo no deja hueco: el guardián de la pila vive en `_sbrk` (devuelve -1 al llegar a
`__StackLimit`) y `PICO_USE_OPTIMISTIC_SBRK` no está definido.

Verificado que es **sólo de esta familia**: ESP32/P4 traen
`CONFIG_HEAP_ABORT_WHEN_ALLOCATION_FAILS` sin activar y el STM32 usa newlib.

Efecto: `stacktrace` y `MemT4b_ReadOnly` pasan de colgarse a **verde**; `MemT5_Gc` y
`synclisttest` pasan de cuelgue mudo a error con nombre.

#### ✅ `#447` — la Pico ya no muere muda (cerrada 28-ago · `defcaae4`)

El SDK deja `isr_hardfault` como bucle infinito: un fallo paraba la placa en seco, sin
print, sin exit code y sin log. Y el silencio no es neutral — **un HardFault y un bucle
infinito de BP se veían EXACTAMENTE IGUAL desde fuera**.

Ahora un manejador propio escribe `CFSR`/`HFSR`, `PC`, `LR` y el desglose en palabras
(incluido `UNALIGNED`) al log post-mortem antes de parar. Y con `PICO_PANIC_FUNCTION` el
`panic()` del SDK cuenta su MOTIVO al log — hacía falta porque lo escribía por `stdout`,
que en esta placa **es el cable del wire**: el IDE recibía esos bytes en medio del protocolo
y los tiraba.

**Rindió en una ejecución**: `MemT5_Gc` pasó de misterio a `PANIC: Out of memory`, y el
`CFSR=00000000` descartó la hipótesis del acceso desalineado — la dijo el hardware.

#### ✅ `#445` — `App.mainModule()` sólo iba en placas CON PANTALLA (cerrada 28-ago · `53396c39`)

Los tres builtins de introspección de H19 (211/212/213) vivían **dentro del bloque
`#ifdef BPVM_GUI`** de `builtins.c`. No tienen nada que ver con la pantalla: es donde se
metieron. Funcionaban sólo donde se define `BPVM_GUI` (host, ESP32-P4, STM32 Discovery) y
**no** en la Pico, el S3 ni la Nucleo.

**Lo que engaña**: el `case` SÍ está escrito. Quien lea el fuente lo ve implementado.

Verificado en el preprocesado con control (ANTES: 0 sin GUI / 3 con GUI; AHORA: 3 y 3) y
ejecutando `AppTest` en un host con `GUI=0`. ✅ Verificado en placa.

#### ✅ `#446` — el arnés no puede ver lo que le falta a una placa pequeña (**CERRADA 30-ago**)

> ⏩ **El arreglo es un control, `make check-nogui`**, y lo que mide no es lo que decía el
> enunciado. Construir el host con `GUI=0` y pasarle el arnés sale **38/38 verde** — pero
> eso no prueba nada: esos 38 son el núcleo determinista y ninguno toca la GUI. **El hueco
> no era el flag, era el corpus.**
>
> Lo que sí cierra el agujero de #445 es preprocesar con y sin el flag y comparar:
>
> ```
> con GUI: 222 builtins  |  sin GUI: 145  |  sólo-GUI: 77
> ...y los 77 se llaman TODOS `GUI_*`  ->  no queda ningún colado
> ```
>
> El control falla si alguna vez un builtin que **no** sea `GUI_*` acaba dentro del
> `#ifdef` — que fue exactamente #445 (los tres de `App`, rotos en toda placa sin pantalla
> y con el fuente enseñándolos implementados).
>
> ✅ **Y se comprobó que SABE VER ROJO**, que es lo que le faltaba al arnés viejo: se
> reinyectó el bug de #445 a propósito y el control lo detectó, lo nombró (`APP_MAIN_MODULE`)
> y salió con error. Un control que sólo sabe decir verde no es un control.
>
> 📌 **Y una lección del propio arreglo**: el primer censo lo hice con un `awk` que contaba
> `#ifdef`/`#endif` a mano, y dijo que los tres `APP_*` **seguían dentro** — se le había
> colado el `#ifdef` de la línea 33. Un instrumento mintiendo sobre el mismo bug que
> perseguía. Por eso el control **preprocesa de verdad**: el preprocesador no opina.

Salió al cerrar #445 y es estructural. El Makefile del host lleva **`GUI ?= 1`**: el PC
**siempre** compila con `BPVM_GUI`. Las **535 líneas** del core bajo ese flag están siempre
presentes en el host, así que el arnés **no puede detectar que a una placa pequeña le falte
algo de ahí**.

⏩ **Arreglo barato y sin placa**: construir el host **también con `GUI=0`** y pasarle el
mismo arnés. Habría cazado #445 en el PC. Es la respuesta concreta a #444: no es que falte
una herramienta, es que **la que hay mide una configuración que ninguna placa pequeña
tiene**.

#### ✅ `#442` — el opcode desconocido ya dice DÓNDE (cerrada 28-ago · `ce748e16`)

Decía el opcode y un PC absoluto; con varios módulos en un espacio común eso obliga a hacer
a mano la aritmética de las bases. Ahora trae **módulo+offset** y los **8 bytes de
alrededor**, que parten la investigación en dos: coinciden con el `.mod` → el PC llegó mal;
no coinciden → el código está pisado. Cerró el diagnóstico de #440 en una ejecución.

#### ✅ `#441` — seis opcodes que la VM-Java tenía y la VM-C no (cerrada 31-ago)

`GET_GLOBAL_I8/U8/I16/U16` y `SET_GLOBAL_I8/I16` (0x40–0x45) están **declarados** en
`bpvm_opcodes.h` y **no tienen `case`** en `src/interp.c`. `VirtualMachine.java` los
implementa. Es una violación del invariante sagrado que el arnés no ve porque **ningún
sample tiene una global `byte` o `short`**, y el compilador de hoy tampoco los emite: la
familia está viva en el formato y muerta en una de las dos VMs.

Salió al perseguir #440 (el `0x43` del síntoma es `GET_GLOBAL_U16`). **No es su causa** —el
mismo `.mod` corre en el host—, pero es real y conviene cerrarlo: son seis casos triviales.

### ✅ Cerrada — y el test YA EXISTÍA, sólo que nunca se había corrido contra la VM-C

Implementados en `interp.c` calcados de `VirtualMachine.java` (offset `i16` big-endian,
extensión de signo en los `I*`, relleno de ceros en los `U*`, truncado a los bytes bajos al
escribir).

📌 **Lo que costó encontrar no fue el arreglo, fue el test.** El compilador **no emite**
estos opcodes, así que ningún programa BP los alcanza y el arnés no puede verlos. Pero
`miVM/.../MainNarrow.java` los ejercita **los seis** con el `ModWriter`, fabricando
`NarrowDemo.mod` a mano. Existía desde antes y nadie lo había pasado por la VM-C.

🔴 **Con control en rojo, y salió limpio.** Antes de reconstruir, la VM-C con el `.mod`:

```
[bpvm-c] opcode 0x40 desconocido en PC 295 = NarrowDemo+3
[bpvm-c]   bytes @291: 00 0F 00 04 [40] FF FC 03
```

Después: **los 19 valores byte a byte iguales** en las dos VMs (`-1`, `255`, `-1`, `65535`,
`-56`, `-32000`, `32767`…). Paridad **38 PASS / 0 FAIL / 0 SKIP** y las cuatro imágenes
reconstruidas (`interp.c` es núcleo: entra en las cuatro).

⚠️ **Un detalle menor que salió de paso**: con el `PRINT` **crudo** la VM-Java antepone
`VM [PRINT]: ` y la VM-C no. Los VALORES coinciden; el adorno no. No afecta al invariante
en la práctica porque el compilador nunca emite ese opcode —sólo lo usan los `.mod`
fabricados a mano—, pero conviene saberlo si algún día se compara uno de ésos byte a byte.

#### ✅ `#442`(enunciado) — «opcode desconocido» manda a buscar donde no es · CERRADA, ver arriba

> Esta era la ENTRADA ORIGINAL; el cierre está más arriba (28-ago · `ce748e16`). Se deja
> el enunciado porque explica el porqué, pero **no cuenta como pendiente**.

Dice el opcode y un **PC absoluto**, y no dice el módulo ni el offset dentro de él. Con
varios módulos enlazados en un espacio común, ese número no sirve sin hacer la aritmética a
mano (`cb` de cada módulo + tamaños). El 27-ago costó buena parte de una tarde. Debería
decir: **módulo, offset dentro del módulo, y de dónde salió ese módulo** (FS o pack).
Encaja con la norma de Eduardo sobre mensajes que mienten o callan.

#### ✅ `#443` — repaso de V5 con la batería de V4 · **CERRADA POR DECISIÓN (30-ago)**

> **Eduardo:** *«la 443 la podemos dejar, creo que ya está claro el problema. De todas
> formas al final de V6 toca volver a testear así que los bugs no se van a escapar.»*
>
> La hipótesis que la abrió —*«es muy raro que solamente falle `JsonDemo`»*— **quedó
> contestada, y con medida**: la batería de la Pico pasó de 40/48 a **48/48**, y las
> ocho rojas resultaron ser tres causas (`#440`/`#449`, `#448`, `#451`) más dos falsos
> positivos del andamio del MPU. No había una cola de bugs distintos escondida: había
> presión de memoria y un instrumento que mentía.
>
> Y el repaso completo **no se pierde, se traslada**: el cierre de V6 lo lleva de todas
> formas. Repetirlo ahora sería pasar dos veces la misma batería sobre un árbol que va
> a seguir moviéndose.
>
> 📌 **Lo que SÍ hay que conservar es la lección, que es de método y no de esta ficha**:
> *cuando una versión hereda de otra, lo que no se re-prueba hay que saber que no se
> re-probó*. V4 dejó 2413 líneas de registro y V5 **418**; si en V5 hubiera puesto «de
> la lista de V4, estas N no se repiten», el 27-ago habría sido una consulta y no una
> tarde. `docs/H13_PRUEBAS_V5_REPASO.md` se queda como **la lista de arranque de la
> tanda de cierre de V6** — sus «condiciones de partida» son lo que hace atribuible el
> resultado.
>
> ⚠️ **Lo que esto NO cierra**: la deuda de verificación del **STM32** (Nucleo desde el
> 27-ago, Discovery en toda la serie). Eso no es «repasar V5», es que hay código común
> nuevo que esas dos placas no han ejecutado nunca — y cuanto más tarde salte, más
> commits hay que bisecar.

<details><summary>El enunciado original</summary>


*«Yo repetiría todas las pruebas de V4 sobre IDE + Firmware + Demos de V5. A ver qué sale,
es muy raro que solamente falle JsonDemo.»* La hipótesis a comprobar es que **#440 no está
solo**. La lista, las condiciones de partida y lo ya sabido, en
`docs/H13_PRUEBAS_V5_REPASO.md`.

Lo que destapó la necesidad: la campaña de V4 dejó **2413 líneas** de registro y la de V5
**418**. No es que no se anote — es que **cuando una versión hereda de otra, lo que no se
re-prueba hay que saber que no se re-probó**. Si en V5 hubiera puesto «de la lista de V4,
estas N no se repiten», el 27-ago habría sido una consulta y no una tarde.

</details>


#### 🧊 CODE FREEZE V6 — antes de `D1` (decidido por Eduardo, 11-sep)

*«Antes de empezar `D1` marcamos congelación de código, para prevenir que se intente implementar
código nuevo y centrarse en arreglar posibles bugs.»*

Es la tercera vez que se cierra una versión así (V4 el 17-jul, V5 en agosto), y el criterio es el
mismo que las dos anteriores: a partir de la congelación **no se pregunta «¿merece la pena?» sino
«¿está roto?»**. Lo que no está roto, a V7 — y cancelar o aplazar es un resultado válido.

**Qué implica, concretado:**
- Se marca **al terminar `T1`** (las dos fases) y **antes de tocar `D1`**. `G2`, `C1` y `T1` son
  código nuevo y van **antes** de la raya, a propósito: son la herramienta con la que se hacen las
  pruebas finales (`F1`).
- Después de la raya sólo entran **arreglos de bugs**, cada uno con su ficha y su reproducción.
- `D1` documenta **lo que hay**, no lo que se quiere; y `F1` prueba **lo congelado**.

⏭️ Cuando llegue, se abre como entrada propia aquí y en `ESTADO.md`, con la fecha y el commit de la
raya, igual que las anteriores.

#### 🧪 `T1` — EL PLAN, INCREMENTAL (Eduardo, 11-sep)

*«Yo plantearía `T1` de una forma incremental. Poder ejecutar las pruebas más sencillas (no gráficas)
y ver qué infraestructura tenemos y qué nos falta. Una vez comprobado que eso va, podemos pasar a la
fase 2, que serían las pruebas gráficas. Y me pararía ahí: en V7 se pueden mejorar las pruebas según lo
que veamos, pero no intentemos hacerlo todo en V6 porque igual se alarga demasiado.»*

El diseño de fondo (oráculo = `stdout` del host · la lista con cinco condiciones por programa · lo
que un agente no puede hacer y tiene que GRITAR) está en `docs/V6_IDEAS.md` §«Las pruebas finales,
con un agente conduciendo las placas» (5-sep). Esto es el reparto en fases, **medido contra lo que ya
existe el 11-sep**.

**Las cinco piezas del sistema, y su estado:**

| pieza | estado |
|---|---|
| **el oráculo** — el `stdout` del host por cada `.mod` | ✅ es lo que `compat.sh check` calcula ya (54 PASS) |
| **conducir la placa** — subir, ejecutar, capturar `stdout`, cronometrar, leer el log | ✅ `tools/wire_serie.py`: `put`/`puts`/`run`/`get`/`log`/`info`/`ciclo` |
| **grabar sin manos** | ✅ `idf.py` · `BOOTSEL` por el wire + `.uf2` · `STM32_Programmer_CLI` por sonda (las cinco, el 10-sep) |
| **el bucle y el informe** — placas × programas, comparar, y **probar que corrió** | ❌ **no existe: es el runner** |
| **las dependencias** — qué `.mod` necesita cada programa | ❌ lo sabe el IDE, no el runner — pero ver abajo: para la fase 1 casi no hace falta |

**El corpus, clasificado** (57 entradas):

| | cuántos | fase |
|---|---|---|
| **puros** — sin hardware, sin GUI | **50** | **1** |
| tocan una fachada de hardware (`AdcDemo`, `WdtCatch`, `NeoCatch`, `StubParidad`, `BusBug`) | 5 | son pruebas de **paridad de stubs**: en placa dan otra cosa a propósito → fuera |
| GUI (`GuiParidad`, `GuiParidad2`) | 2 | **2** |

**Y dentro de los 50, dos hallazgos que simplifican la fase 1:**
- Sólo **3** imprimen algo dependiente de la placa (`MachineHost`, `MachineAlias`, `MachineId`). Para
  ésos el criterio es **«no debe petar»** (exit 0, sin excepción) en vez de byte-idéntico — una de las
  tres opciones del campo *cómo se decide*. **Los otros 47 van con oráculo byte a byte.**
- Los módulos que importan los 50 son los **preinstalados en `/lib`** de cualquier placa (`Core`, `IO`,
  `Machine`, `Math`, `Str`, `Pico`…). La única dependencia de verdad es el par `SuperExt` +
  `SuperExtBase`, que se suben juntos. **La resolución de dependencias puede esperar a la fase 2.**

### Fase 1 — los 50 puros en las placas conectadas
**Construir el runner** sobre lo que existe (un script sobre `wire_serie.py`, unas decenas de líneas):
para cada puerto con `HELLO` → sello del firmware y capacidades; para cada programa → oráculo del host,
`put`, `run`, `stdout` hasta el `EXITED`, comparar según el criterio, apuntar. **El informe prueba que
corrió**: sello de cada placa, CRC de cada módulo subido, **ejecutados vs listados**, y una línea por
cada salto **con su porqué** (regla del 5-sep: *una prueba que se salta en silencio no existe*).
La lista con cinco campos nace aquí con **defectos por omisión y 3 excepciones** — no hay que
escribir 50 filas a mano.
✅ **Criterio de salida de la fase 1**: los 50 corridos en al menos tres familias, con informe, y la
lista de lo que faltó.

### Fase 2 — las gráficas, sobre `G2` + `C1`
Los 2 del corpus más los `Gui*Demo` que sean deterministas. El oráculo es **el `shot.json`** (`C1`):
`GET` y diff contra el del PC. Añade al runner: subir dependencias (`Gui.mod`, `Json.mod` no van en
`/lib`), decidir por capacidades de `HELLO` qué placas tienen pantalla, y el `GET` del fichero.
✅ **Criterio de salida**: las gráficas del corpus con árbol idéntico PC↔placa en las placas con pantalla.

### Y AQUÍ SE PARA (decisión de Eduardo)
Lo que se vea en las dos fases —qué programas faltan, qué falla, qué se salta— es material de V7. **No
se intenta hacer todo en V6.**

#### 🔵 `#444` — el sistema de pruebas no escala a una docena de placas (ABIERTA, 27-ago)

Planteado por Eduardo al cerrar el día: *«hay que mejorar el sistema de test, porque si todo
hay que verificarlo en todas las plataformas nos vamos a volver locos cuando tengamos una
docena de placas»*.

**El problema, medido hoy mismo**: U3 obligó a re-verificar en placa ocho veces en un día, y
aun así quedó deuda (ver el apartado de deuda en `H13_PRUEBAS_V5_REPASO.md`). Con 4 familias
ya duele; con 12 es inviable.

**Descartado como respuesta suficiente** (propuesto el 27-ago, Eduardo: *«sigue sin ser un
buen sistema»*): el reparto obvio en tres pisos — host/unit con cintura falsa, batería
completa contra `bpvm-sim`, y en placa sólo lo que el simulador no puede tener — apoyado en
que `HELLO` ya declara capacidades para que el runner se adapte. **No basta**, y conviene
entender por qué antes de volver a diseñar: queda pendiente esa conversación.

⏭️ **Charla de diseño pendiente**, no tarea. Cuando se retome, va a `docs/*_IDEAS.md`.

### 💡 Retomada el 5-sep: lo que cambia es QUIÉN paga la re-verificación

Eduardo, al ver la sesión de hoy: *«Te he visto ejecutar programas y resetear en placas reales.
Si construimos una lista de programas a testear, ¿se los podríamos pasar a un agente para que lo
haga?»*

**Sí, y hoy ya ha pasado**: las cinco familias grabadas y medidas sin que nadie tocara una placa
(`idf.py flash`, el `.uf2` con BOOTSEL pedido por el wire, y `STM32_Programmer_CLI` por número de
sonda). Lo que lo hace posible ya estaba: el wire es protocolo de máquina y `HELLO` declara
capacidades.

Y por eso el reparto de tres pisos no era la respuesta: **repartir no quita trabajo, sólo lo
ordena**. El problema medido era *«U3 obligó a re-verificar en placa ocho veces en un día»* — y
ocho veces las hace un agente mientras se hace otra cosa.

📐 La propuesta completa —el oráculo (el `stdout` del host, que evita mantener ficheros de salida
esperada), los cinco campos que necesita cada entrada de la lista, lo que un agente NO puede
hacer y tiene que gritar en vez de saltarse, y el orden de trabajo— está en
**`docs/V6_IDEAS.md`**, sección *«Las pruebas finales, con un agente conduciendo las placas»*.

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
| **U1** | lo que **no exige decidir nada** — 4 tareas, una por sesión corta | ✅ 23-ago |
| **U2** | el **transporte** (wire): entender los gemelos falsos y darle contrato | ✅ 26-ago |
| **U3** | el **REPL** — el trabajo de verdad: 4.318 líneas sin contrato | ✅ 31-ago (y el simulador, `U3.24`) |
| **U4** | la **stdlib embebida**: un solo formato de blobs | ✅ 3-sep (`U4.1`) |
| **U5** | la **tabla de handles**: darle módulo | ✅ 3-sep (`U5.1`) |
| **U6** | la **organización de la memoria**: hoy son 4 mecanismos por micro | ✅ 3-sep (`U6.11`: las 5 familias, verificadas en placa) |
| **A1** | *(después de U1–U5)* la revisión **por niveles**, ya sobre código único | ✅ 5-sep: **dos hilos de SO, `vm` + `io`**, en las cinco familias y verificados en placa (`A1.6` aplazada y reorientada por Eduardo) |
| **A2** | el **censo de proporciones** común / familia / placa | ✅ 5-sep: **91,8 % común**, 7,6 % familia, 0,7 % placa |
| **A3** | el **modelo de capas**, auditado contra el código | ✅ 5-sep: la auditoría está hecha; sus fugas salieron como fichas (`#469`–`#473`) |
| **N1** | **AOT**: ampliar la cobertura por tandas *(encargo del 21-ago)* | ✅ 25-ago (el alcance de V6) |
| **L1** | **lenguaje y compilador** | ⏩ **MOVIDA A V7 el 5-sep** — ver la nota bajo la tabla |
| **E1** | **el IDE** y el protocolo wire | ✅ **6-sep**: sus **7 puntos** cerrados — `#412` (argumento de ejecución), `#452` (`RESET` con un RUN vivo), las deps que el device ya tiene, el CRC de procedencia, las BD en el simulador, el árbol por color y el tiempo de la placa |
| **G1** | **GUI**: el bucle de LVGL a un **hilo BP propio** | ✅ **7-sep** (`a4c28062`): `Gui.start()` / `stop()` / `join()`; `Gui.run()` sigue síncrono por compatibilidad |
| **P1** | **placas nuevas**: ESP32-**C3** y ESP32-**C6** | ✅ C3 (31-ago) y C6 sin pantalla (3-sep): **el ecuador de V6**; la pantalla es P2 |
| **C1** | **la CAPTURA DE PANTALLA en el micro** — ver `#475` | ✅ **11-sep** (`0791bb3e`): `Gui.shot(path)` → `.shot` (`docs/SHOT_FORMAT.md`) + `tools/shot2png.py`; visto en host (ventana y `--no-screen`), miVM y **la Discovery** (800×480, 6 956 B, 110×). **C6 verificada** (240×240, 2 862 B, 40×; `GET` en 8 ms por USB-JTAG). P4 compila, sin probar en placa |
| **G2** | **Revisión del modelo gráfico**: contenedores con sus hijos, cascada nuestra, serializador | ✅ **11-sep**, en tres commits: `50fcbc46` (los 3 arreglos de C), `65a50f0e` (`Container` + `OwnerList` + cascada BP), `65e9f558` (`toJson()` + ida y vuelta con `main.win`, 58 PASS, Discovery). Y de paso `#494` |
| **T1** | **el SISTEMA DE PRUEBAS** con las placas conducidas — ver `#444` | ⬜ **ABIERTO · V6** (Eduardo, 7-sep). **Plan en dos fases (11-sep)**: los 50 puros primero, las gráficas sobre `G2`+`C1` después, y ahí se para. Va **antes** de `D1` y `F1` |
| **P2** | **pantallas SPI** — *después de P1* | ✅ HECHA (4-sep): la pantalla del C6 (ST7789 por SPI), vista y girada en placa |
| **D1** | **la DOCUMENTACIÓN** de V6 | ⬜ **ABIERTO · V6** (Eduardo, 7-sep). Penúltimo: se documenta cuando ya no se mueve nada |
| **F1** | **las PRUEBAS FINALES** de V6 | ⬜ **ABIERTO · V6** (Eduardo, 7-sep). **El último.** Se apoya en `C1` y `T1`, que es la razón de que esos dos se queden en V6. 🧪 **Lleva dentro `#379`** (9-sep): lo único que le queda es una prueba de placa —el P4 **con la tarjeta**, `tools/wire_serie.py ciclo`— y ésta es la tanda donde las placas se conducen |

📌 **`#475` Y `#444` PASAN A SER HITOS PROPIOS (`C1` y `T1`), decidido el 6-sep.** Eduardo:
*«`#475` y `#444` hay que darle un ítem propio ya que es implementación nueva. Del resto, hay que ir
cerrando pendientes.»* O sea: la lista de fichas es para **cerrar lo que está roto o a medias**, y lo
que es **construir algo que no existe** no cabe ahí — se le pone nombre y se planifica aparte. Los
dos están **diseñados y con números** (`#475` midió transporte y compresión; `#444` tiene la
propuesta del agente conduciendo las placas), así que lo que falta es hacerlos.
✅ **Y el 7-sep Eduardo decidió la versión: `C1` y `T1` SE QUEDAN EN V6.** Su razón, que es la que
manda sobre el criterio de «esto es añadir y V6 está cerrando» con el que yo los mandaba a V7:
*«C1 y T1 se quedan en V6. La razón es que lo utilizaremos para las pruebas finales.»* O sea que
no son un añadido que se cuela al cerrar: son **la herramienta con la que se cierra**. Es
*la herramienta antes que el artefacto* aplicado al cierre de una versión.

📌 **Y con eso aparecen los dos últimos hitos, uno cada uno** (Eduardo, 7-sep): *«La documentación
y las pruebas finales, un hito cada uno. C1 y T1 justo antes de documentar y pruebas finales.»*
Los códigos `D1` y `F1` los puse yo siguiendo la nomenclatura de la tabla; el reparto y el orden
son suyos. **El orden de cierre de V6 queda así:**

> **los 15 pendientes → `C1` → `T1` → `D1` (documentación) → `F1` (pruebas finales)**

Y la consecuencia práctica, dicha por él: *«lo que toca durante unos días es ir resolviendo
pendientes»*.

##### 🔢 El PARQUE que tiene que probar `F1` — **7 imágenes, 9 placas**

**La cuenta la hizo Eduardo el 7-sep** —*«a mí me salen 8 placas a probar: Pico y Metro; S3, C3,
C6 y P4; Nucleo y Discovery»*— y al contrastarla con el repo apareció una novena: la **segunda
P4**. ✅ **Confirmado por él el mismo día: son 9.**

| imagen | placas que sirve |
|---|---|
| `bpvm_pico.uf2` | **Pico 2** · **Metro RP2350B** |
| `bpvm_esp32_merged.bin` | **ESP32-S3** |
| `esp32c3` | **ESP32-C3** *(nueva en V6)* |
| `esp32c6` | **ESP32-C6** *(nueva en V6)* |
| `bpvm_esp32p4_merged.bin` | **P4 kit** · **P4 Waveshare** |
| `bpvm_stm32_nucleo.bin` | **Nucleo U575** |
| `bpvm_stm32_dk2.bin` | **Discovery U5G9J** |

📐 **De dónde salió la novena**, porque el criterio sirve para la próxima familia: el
`CMakeLists.txt:12` del P4 dice que la imagen *«sirve a las dos P4 (kit y Waveshare; el panel
sale del ENV, `#311`)»*. **Contar placas por imagen se deja una fuera**; hay que contarlas por
lo que la imagen declara servir.

⚠️ **Y no es una placa redundante, que es lo que la hace obligatoria en `F1`:** es la única que
ejercita el camino **panel elegido por el ENV** (`display=st7701` frente al EK79007 del kit) —
o sea, la única prueba real de que la imagen única de esa familia lo es de verdad. Probando sólo
el kit, ese camino se queda sin cubrir y no nos enteraríamos.

🔴 **Deuda conocida sobre ella**: **le falta el reflasheo**, así que hoy va por detrás de las
demás. Es lo primero que hay que resolver de esa placa antes de `F1`.

📌 Nota para `T1`: **7 imágenes y 9 placas** es el tamaño que el sistema de pruebas tiene que
conducir. `#444` decía *«no escala a una docena de placas»* — con nueve ya estamos ahí.

##### 💭 Por qué `T1` va ANTES de `F1` — la reflexión de Eduardo (7-sep)

> *«Las pruebas finales normalmente nos llevaban, en el mejor de los casos, 2 días completos.
> Ahora, con las pruebas en gran parte automatizadas, podría llevarnos la mitad. Pero no sólo es
> la reducción de tiempo: al ser sistemáticas podemos probar todas las placas completamente.»*

**Lo importante de esa frase es la segunda mitad, y conviene no dejarla en intuición.** Tres cosas
que la sostienen con números y con lo que pasó hoy mismo:

**1. El parque creció y el presupuesto no.** V5 se publicó con **5 imágenes y 7 placas**; V6 va por
**7 y 9** (entran C3 y C6). Son **+40 % de imágenes y +29 % de placas** desde la última entrega. Si
2 días eran «el mejor de los casos» con 7, con 9 ese presupuesto ya estaba roto. **Automatizar no
es una comodidad: es lo que hace que nueve placas quepan.**

**2. Un proceso manual cubre lo que alguien se acuerda de cubrir** — y de eso hoy hubo tres
demostraciones seguidas, todas con la misma forma:
- **`BusBug.bp`** existía desde hacía meses, escrito para bisecar otro cuelgue, y era el reactivo
  exacto de `#440`. El arnés lo tenía delante y no lo metió en la red.
- **El oráculo del GUI** (`__guiDumpTree`) está implementado con paridad byte a byte **desde V4**,
  lo llaman 22 samples, es gratis — y nunca se había ejecutado automáticamente.
- **El censo de fichas**: seis de las que parecían pendientes no lo eran.

Los tres son lo mismo: lo que se revisa a mano se revisa **por memoria**, y la memoria muestrea.
Sistemático no significa «más rápido»: significa que **la cobertura deja de depender de quién
esté cansado a las siete de la tarde**.

##### 🗂️ Los TRES TIPOS de prueba, y a qué placa le toca cada uno (Eduardo, 7-sep)

> *«Las pruebas, de 3 tipos: las comunes a todas las placas, las pruebas gráficas para las que
> tengan pantalla, y las pruebas de BD para las que tengan PSRAM.»*
> Y la corrección que añadió acto seguido: *«La S3 no puede ser para SQLite ya que no tenemos
> código binario para ella.»*

| placa | comunes | gráficas | BD |
|---|:---:|:---:|:---:|
| **Pico 2** | ✅ | — | — *(sin PSRAM)* |
| **Metro RP2350B** | ✅ | — | ✅ *(**si lleva** PSRAM: la decide el ENV, `psramCsPin`)* |
| **ESP32-S3** | ✅ | — | ❌ **no hay binario Xtensa** |
| **ESP32-C3** | ✅ | — | — *(sin PSRAM)* |
| **ESP32-C6** | ✅ | ✅ *(ST7789 por SPI, `P2`)* | — *(sin PSRAM)* |
| **P4 kit** | ✅ | ✅ *(EK79007 MIPI-DSI)* | ✅ *(32 MB HEX@200)* |
| **P4 Waveshare** | ✅ | ✅ *(ST7701, elegido por el ENV)* | ✅ *(32 MB)* |
| **Nucleo U575** | ✅ | — | — *(sin RAM externa)* |
| **Discovery U5G9J** | ✅ | ✅ | — *(sin RAM externa)* |
| | **9** | **4** | **3** |

**La BD tiene DOS puertas, no una, y por eso la S3 se cae.** Comprobado en
`bpstdlib/sqlite/nativo/`: los packs nativos existen para **`ARMV8`** y **`RISCV`** —
`SQLite.mdn.ARMV8`, `sqlite.npk.ARMV8`, y sus gemelos RISC-V— y **no hay XTENSA**. O sea:

1. **que haya binario para esa arquitectura** — la contestamos NOSOTROS, mirando qué hemos
   compilado; y
2. **que la placa tenga RAM suficiente (PSRAM)** — la contesta LA PLACA.

La S3 falla por la primera, no por la segunda, y eso cambia lo que costaría arreglarlo: no es
hardware, es **compilar el pack para Xtensa**. (El formato ya tiene el hueco reservado —
`MDN_ARCH_XTENSA 94` en `mdn_format.h:76`—, así que hoy el gate del `.mdn` lo rechaza limpio en
vez de colgarse.) El criterio de Eduardo para la BD viene de V5 y sigue valiendo: *«La Pico no
tiene PSRAM y la Metro puede tenerla o no. El micro y la imagen son la misma, pero en la que no
lleva PSRAM no voy a meter una BD.»*

🔑 **Y aquí está lo que `T1` tiene que hacer distinto, porque dos de los tres criterios NO se
pueden leer de una tabla.** «Tiene pantalla» y «tiene PSRAM» **no son propiedades de la imagen**:
la P4 elige su panel por el ENV (`display=st7701`) y la Metro lleva PSRAM o no según
`psramCsPin`. Dos placas con **la misma imagen** caen en cubos distintos. Así que `T1` **le
pregunta a la placa qué tiene** y reparte con la respuesta — que es la norma que Eduardo ya fijó
para el orden de búsqueda de módulos: *«El IDE no tiene que determinar el orden de búsqueda, le
pregunta al micro y éste se encarga.»* La tercera puerta —si hay binario para esa arquitectura—
es la única que se contesta desde nuestro lado.

⚠️ Y los cubos **no son disjuntos**: las dos P4 entran en los tres.

⚠️ **3. Y el aviso que hay que meter en el diseño de `T1`, porque ya nos mordió**: sistemático no
es lo mismo que correcto. `#459` volcaba el heap por pantalla y `compat.sh` daba **38 PASS**: las
dos VMs producían **la misma basura, byte a byte**. Un oráculo que sólo compara dos
implementaciones **no puede ver un fallo que ambas comparten** — y a nivel de placa el riesgo se
multiplica por nueve: te dirá muy convencido que las nueve coinciden. `T1` necesita comparar
contra **lo esperado**, no sólo entre sí. Es literalmente lo que pedía `#444`.

⏩ **`L1` SE VA A V7, decidido el 5-sep.** Salió de tirar del hilo de las intrínsecas y
acabó en un cambio de fondo: **el módulo raíz**, al estilo de la unidad `System` de Turbo Pascal
— *«me da más tranquilidad tener algo que se pueda leer que tener algo que sólo existe en
memoria. Y esto también es importante: un solo archivo, sin posibilidad de diferencias entre
compilador y VM»* (Eduardo). El criterio del aplazamiento es suyo y es el de siempre: *«lo
importante es que lo que hay ahora funcione correctamente»*.

**Por qué V7 y no V6**, en tres:
1. **No hay nada roto.** El cableado del compilador es frágil, pero funciona.
2. **Toca `Core.mod`, que va EMBEBIDO en las cinco imágenes** (`esp32_mods.c:4165` y gemelos):
   engordarlo obliga a regenerar los blobs de las cinco familias, con su riesgo de ABI y de copias
   rancias. Es justo la clase de cambio que no se mete cerrando una versión.
3. **Se hace mejor con tiempo**: el módulo raíz decide dónde viven los tipos, dónde se declaran
   las 226 intrínsecas y quién genera las dos tablas de ids.

📌 **Y se va casi entero.** Aplicando el mismo criterio, lo que queda dentro de `L1` son
**añadidos, no arreglos** — destructores + `var owner`, ficheros como clase, `Object` comodín,
`Map` con objetos, el módulo `Time` — y todos se diseñan **encima** del módulo raíz, no debajo.
Con eso **V6 quedaba en `E1` + `G1` + los pendientes sueltos + la captura de pantalla**.
📌 **Al día 7-sep: `E1` y `G1` están CERRADOS, así que no queda ni un hito de V6 abierto.**
Lo que queda de V6 son **pendientes sueltos** (`#456`, `#462`, `#468`, `#469`, `#470`, `#472`,
`#480`, `A4`) más los cuatro de la cola heredada. `C1` y `T1` siguen **sin versión decidida**.

⚠️ **Lo que NO se aplaza es la red de seguridad**: ver `#476`. El motivo de querer el módulo
raíz es que una divergencia entre las dos tablas de builtins **no hace ruido**; si la espera dura
una versión entera, durante esa espera tiene que gritar.

📌 **Orden acordado el 2-sep, al cerrar** (Eduardo): *«Terminaremos U6, U4 y U5. Después podemos
hacer P1 (sin pantalla) y P2 (añadir la pantalla a ESP32-C6).»* Estado ese día: U1, U2, U3 y N1
cerrados; P1.C3 hecho; U6 en código en las cinco familias (faltan las STM32 en placa); `#466`
hecho salvo la comprobación en placa. El C6 entra **sin pantalla** por el camino del C3, y la
pantalla es P2. Y el marcador: *«cuando tengamos la ESP32-C6 yo creo que ya habremos llegado al
ecuador de V6»* — la segunda mitad es la arquitectura sobre código único (A1) y G1, L1, E1.

📌 **U1–U5 no bloquean a los demás.** La unificación es lo que se hace *primero* porque
abarata todo lo que venga detrás, pero N1, L1 y E1 tocan sitios distintos y pueden avanzar
en paralelo si apetece cambiar de aire. Los que **sí** tienen orden son A1 (después de la
unificación), P2 (después de P1) y U3 (después de U2).

⚠️ **Fuera de esta serie, y a propósito**: los packs nativos en S3/STM32, la SD del STM32,
`LIST_DIR` en el STM32 y la red en placa **no son unificación, son funcionalidad que no
existe**. Van al saco de «qué falta» y no bloquean U1–U5.

---

#### ✅ `A1` — la arquitectura de ejecución común: `vm` + `io` (abierta 4-sep · **CERRADA el 5-sep**: los dos hilos de SO en las cinco familias, verificados en placa; `A1.6` aplazada y reorientada por Eduardo)

**La decisión de Eduardo (4-sep), tras el censo y las medidas** (`docs/V6_IDEAS.md`, «Hilos de
ejecución»): *«Lo veo bastante anárquico, sin un orden claro. Y todos los micros deberían
funcionar más o menos igual. Creo que el hilo de la VM debería dedicarse solamente a ejecutar
los opcodes. ¿Y entonces qué pasa con todo lo demás? Hace falta al menos un segundo hilo de
ejecución. Y la misma arquitectura en todos los micros (o al menos lo más parecido posible).»*

**Y el rumbo, para que quede claro hacia dónde vamos:** *«En un futuro tendremos dos VM y una
cola de threads BP: las VM irían tomando un thread y ejecutando, la otra VM igual, y así irían
recorriendo la cola entre las dos. Pero de momento 1 VM, 1 core y 2 hilos a nivel de OS.»*
Ese futuro es, en forma, el camino SMP que ya existe en `scheduler_smp.c` (N workers sobre una
cola de runnables, GC stop-the-world entre workers, workers en el núcleo 1+): se queda aparcado y
**el hilo `vm` de hoy es el worker único de mañana** — nada de lo que se haga en A1 puede cerrar
esa puerta.

##### El reparto

- **`vm`**: ejecuta opcodes. Dentro: intérprete, scheduler de hilos verdes, GC (es parte de
  asignar; con un solo mutador no hay baile). No toca ningún transporte: `print` deja bytes en
  una cola y sigue; entre cuantos mira una bandera («parar», «pausar»), sin llamar a drivers.
- **`io`**: todo lo demás. Lee el wire en todo momento (KILL/HELLO al instante, no entre
  cuantos), vacía la cola de salida en `OUTPUT` **por línea** (hoy un `print` sale en seis
  mensajes: 122 KB de texto → 816 KB por el wire), escribe el log, sirve el REPL, bombea LVGL.
- **Entre los dos, tres colas y nada más**: salida (vm → io, bytes copiados), control (io →
  vm: kill, pausa, paso del depurador), eventos (io → vm: clics del GUI, y en el futuro timers
  e interrupciones — la cola de H5.c ya tiene esa forma). **Nada cruza fuera de las colas.**
- **GUI**: `vm` es dueña del modelo y de la copia de los valores; `io` es dueña de LVGL. Los
  cambios de `vm` viajan como encargos; los del usuario vuelven como eventos con el valor
  dentro; las lecturas (`slider.value`) leen la copia y nunca cruzan. Esto **absorbe `G1`** y es
  la respuesta a `#434`.

##### Por familia

| Familia | Hoy | Con A1 |
|---|---|---|
| ESP32 S3/C3/C6/P4 | 1 tarea (`main` o `wire_uart`) con todo dentro | 2 tareas creadas en `esp32/common`, iguales en las cuatro; en S3/P4 `io` en el núcleo 0 y `vm` en el 1 |
| Pico 2 | `vm_task` con todo dentro; comm task + cola de salida existen tras la opción SMP | activar ese camino con un worker y hacerlo el único |
| Host | 1 hilo (o el camino SMP con pthreads) | 2 pthreads: el host prueba la arquitectura ANTES que ninguna placa (la cascada) |
| STM32 | bare-metal, sin RTOS | **decisión pendiente**: FreeRTOS (recomendado: Cortex-M33 con RAM de sobra, `platform_stm32.c` devuelve «sin threads»), o interrupciones + bucle cooperativo, que es lo más parecido posible sin RTOS |

##### Lo que dicen las medidas (C6, 4-sep) sobre el beneficio

Lo que se saca del hilo de la VM costaba un **1 %** en cálculo puro (cuanto 1024 vs 65 536: 23 640
vs 23 400 ms): A1 no acelera el intérprete. Lo que cambia es la **salida** (0,84 ms por `print`
esperando al USB → un empujón a la cola) y la **GUI** (el volcado por SPI deja de costarle tiempo
al intérprete). Y `#462` desaparece por construcción: `vm` ya no puede matar de hambre a `io`.

##### Con dos núcleos (pregunta de Eduardo): qué puede morder

Nada, si sólo cruzan las colas. Lo que hay que vigilar: la **visibilidad entre núcleos** (todo lo
que cruza va por primitivas del RTOS o atómicos con orden; nada de `volatile` suelto); la
**propiedad del modelo GUI** (arriba); las **escrituras en flash** congelan al otro núcleo (XIP;
ya pasa hoy, en la misma tarea); las **interrupciones caen en el núcleo que las registra** (que
`io` inicialice USB, SPI y DMA → viven en el 0); la **caché de flash compartida** (el bucle del
intérprete en RAM; se mide con `Bench`). En un núcleo, las mismas dos tareas repartiéndose el
tiempo, `io` con más prioridad.

##### Las decisiones que cierran el diseño (Eduardo, 4-sep, tarde)

- *«FreeRTOS en todas las familias.»* El STM32 deja de ser bare-metal.
- *«Intentar organizar todo en 2 hilos del SO: un hilo la VM y el otro IO. Empezamos con el PC y
  un micro, y lo utilizamos de modelo para el resto.»* El micro modelo es la **C6**: está en la
  mesa, es de un núcleo (obliga a acertar las prioridades) y tiene pantalla (obliga a resolver
  LVGL). El PC es el simulador (`tools/bpvm_sim.c`), que es el `io` del PC, y el CLI.
- *«Esto debería ser código común, así que una vez hecho para 1 micro debería funcionar en el
  resto.»* Y el inventario lo confirma: la cola de salida (`comm_common.c`, `bpvm_oq_*`), el
  contrato de plataforma (mutex, cond, hilos: implementado en las cuatro), el REPL común de U3
  (`bpvm_repl.c` + `ops` de familia) y la cola de eventos (H5.c) ya están en `src/`. Lo que
  sobra son las **copias**: `comm_host.c` y `comm_pico.c` son el mismo lazo dos veces.

**Regla de reparto.** En `src/` (común): el hilo `io` entero — lazo del REPL, lectura del wire,
drenaje de la cola de salida con `OUTPUT` **por línea**, log, control del depurador —, las tres
colas, y `bpvm_run` sin ningún transporte dentro. En cada familia, SÓLO: (a) el transporte (leer
un byte con timeout, escribir bytes: los `wire_v1_*` de hoy), (b) las primitivas del RTOS que ya
implementa, (c) la llamada que crea las dos tareas con el nombre, la pila y la prioridad que fija
el común, y (d) el orden de arranque del hardware (la máquina de estados de H9).

##### Las tareas de V6

- **✅ `A1.1` — HECHA (5-sep): el hilo `io` en común, y el PC ya corre con dos hilos.**

  **Lo que hay ahora** (`include/bpvm_io.h` + `src/bpvm_io.c`, 100 % común): `bpvm_io_start` crea
  el hilo, `bpvm_io_stop` cierra la cola, espera a que se drene ENTERA y une. El lazo de `io`
  alterna dos gestos — sacar bytes de la cola con **tope de espera** (5 ms: un programa mudo no
  puede dejarlo dormido) y llamar al `poll` de la familia — y **arma las líneas**: pega los
  trozos y entrega una línea completa con su `\n`. Cada familia sólo pone su `bpvm_io_ops_t`
  (`poll` = atiende mi transporte, `line` = enmarca una línea); con `line` a NULL va a `stdout`,
  que es el `io` del CLI.

  **Lo que cambió fuera del módulo, y es poco**: `emit_text` encola si hay `io`
  (`interp.c`); el scheduler **ya no llama al `poll_cb`** cuando hay `io` (`scheduler.c`) — ésa
  es la línea que corta el trato de la VM con el transporte; un campo `io` en `bpvm_t`, detrás
  del prefijo congelado; `bpvm_oq_pop_timed` en la cola (`comm_common.c`); y el camino SMP no
  arranca su comm task si `io` está en marcha (serían dos consumidores de una cola de uno).

  **Verificado en el PC:** `compat.sh check` **38/38 byte-idéntico** con los dos hilos activos en
  cada sample (el CLI arranca `io` alrededor del run, así que el arnés de paridad ejercita la
  arquitectura nueva en cada pasada); `test-mem` 30/30, `test-mods`, `test-smphandles` (0
  corrupciones), `sim-smoke` 45/45. Y el simulador es ya el `io` del PC **con transporte**: `vm`
  ejecuta en el hilo principal mientras `io` lee el socket.

  **Y lo que `io` viene a arreglar, con su prueba propia** (`tools/io_smoke.py`, `make io-smoke`,
  contra el simulador):

  | Lo que se prueba | Resultado |
  |---|---|
  | KILL con el programa CALCULANDO sin imprimir | EXITED KILLED en **3 ms** |
  | KILL con el programa IMPRIMIENDO A CHORRO | EXITED KILLED en **17 ms** (el caso que antes esperaba a que drenase) |
  | Un `print` de tres argumentos → mensajes OUTPUT | **1 por línea** (6 líneas = 6 mensajes; antes 4 por línea) |
  | La salida entregada | **exacta**, byte a byte, sin perder la última línea al parar |

  ⚠️ **Lo que NO hace y hay que decirlo:** el KILL lo *detecta* `io` al instante, pero la VM para
  en la siguiente frontera de cuanto (1024 opcodes: microsegundos). Meter la comprobación dentro
  del intérprete costaría una lectura volátil por opcode y no compensa. Y `comm_host.c` /
  `comm_pico.c` siguen ahí, ahora inertes: se van en `A1.4`, cuando la Pico entre — borrarlos hoy
  dejaría al camino SMP puro sin drenaje.

  **Enlaza en las seis imágenes** (la trampa del `.c` nuevo del núcleo): alta en los cinco
  CMakeLists + el Makefile; el STM32 lo recoge solo por su carpeta enlazada. Compilan C6, C3, S3,
  P4, Pico 2 (`.uf2` con el símbolo dentro) y Nucleo (`bpvm_io.o` y `bpvm_io_write` en el `.elf`).
  Ninguna lo *usa* todavía: sin `bpvm_io_start`, el camino es exactamente el de antes.
- **✅ `A1.2` — HECHA (5-sep): la C6 con las dos tareas, y con ella toda la familia ESP32.**

  **El cambio son 20 líneas en `esp32/common/repl_esp32.c`** y ninguna es de lazo ni de drenaje:
  un adaptador `esp32_io_poll` (que llama al `esp32_run_poll_cb` de siempre) y el arranque de
  `io` alrededor de `bpvm_run`, con `v1_output_sink` como `line`. El sink no se toca: lo que
  cambia es que ahora lo llama `io` con una línea entera en vez de la VM con cada trozo. Es
  literalmente el mismo gesto que en `test/main.c` y en el simulador — que era el encargo.

  ⚠️ **Con el depurador armado, `io` NO arranca.** Su `pause_cb` lee el wire desde la tarea de la
  VM, y dos lectores del mismo transporte es una carrera. Hasta `A1.7` (el depurador por la cola
  de control), un RUN con breakpoints sigue por el camino de un hilo — el mismo interbloqueo que
  la Pico ya hace en su camino SMP.

  **Medido en la C6** (imagen con GUI, 128 KB de VM; cada fila, el mismo `.mod`):

  | Medida | 1 hilo | 2 hilos | |
  |---|---|---|---|
  | `Bench`: fib(28) interpretado ×2 | 23 640 ms | **23 580 ms** | igual (era el 1 %) |
  | `AllocBench`: 20 000 asignaciones + GC | 13 129 ms | **13 188 ms** | igual |
  | `PrintBench`: bucle SIN imprimir | 21 ms | **20 ms** | igual |
  | `PrintBench`: 2 000 líneas | 1 700 ms | **467–482 ms** | **3,6×** |
  | · mensajes `OUTPUT` | 11 944 | **1 988** | **6×** menos |
  | · bytes por el wire | 816 KB | **238 KB** | **3,4×** menos |
  | KILL mientras imprime a chorro | — | **46 ms** | 183 líneas ya encoladas salen antes |
  | KILL mientras calcula | — | **18 ms** | |

  O sea, exactamente lo que las medidas del 4-sep predecían: el cálculo no se toca y la salida
  baja de 0,84 a 0,24 ms por línea. Lo que queda es el transporte, que es físico.

  **La pantalla sigue**: `GuiRotCycle` pinta y gira con `io` en marcha (LVGL aún se bombea desde
  `vm`, dentro de `Gui.run()`; eso se mueve en `A1.6`).

  ### 🔬 Y por el camino, un bug de la capa de plataforma que sólo se ve en placa

  La primera medida con dos hilos dio **fib(28) en 140 600 ms — SEIS VECES más lento**. No era el
  diseño: era una división entera.

  ```c
  TickType_t ticks = (ms > 0) ? pdMS_TO_TICKS(ms) : 0;   /* 5 ms a 100 Hz -> 0 ticks */
  ```

  `bpvm_platform_cond_timed_wait` con menos de un tick (10 ms con `FREERTOS_HZ=100`) daba **cero**,
  y `xSemaphoreTake(sem, 0)` no espera: sondea. El lazo de `io` pide 5 ms entre trago y trago, se
  los daban a cero, y el hilo se comía el núcleo que compartía con la VM. Arreglado en las dos
  plataformas con FreeRTOS (ESP32 y Pico) redondeando **hacia arriba, nunca a cero**: un tick es
  lo mínimo que ese reloj sabe esperar, así que *«hasta N ms»* no puede significar *«nada»*.

  📌 La lección, que es de método: el hilo `io` pasó los 38 samples de paridad y las cuatro
  baterías **en el PC** con este fallo dentro, porque el `pthread` del PC sí espera 5 ms. Lo
  destapó el primer número de la placa. Es la cascada funcionando: el PC caza la lógica, el micro
  caza lo que depende del reloj del silicio.
- **✅ `A1.3` — HECHA: C3, P4 y S3 verificados en placa (5-sep). La familia ESP32 entera.**

  Sin una línea de código nuevo: `A1.2` vive entero en `esp32/common`, así que bastó recompilar y
  grabar. Las dos placas llevaban firmware del **1-sep**, o sea anterior a toda la serie, y ahí
  está el matiz honesto: **su «antes» no es sólo `A1`**, incluye `U4`, `U5`, `U6` y `#466`.

  | Medida | C3 antes | C3 después | P4 antes | P4 después |
  |---|---|---|---|---|
  | `PrintBench` 2 000 líneas | 1 610 ms | **305 ms** (5,3×) | 71 075 ms | **20 365 ms** (3,5×) |
  | · mensajes `OUTPUT` | 11 900 | **1 997** | 12 016 | **2 001** |
  | · bytes por el wire | 813 KB | **239 KB** | 820 KB | **239 KB** |
  | `AllocBench` + GC | 12 831 ms | 14 174 ms | 4 936 ms | **4 900 ms** |
  | `Bench` fib(28) ×2 | — | 22 740 ms | — | — |
  | KILL calculando / imprimiendo | — | **50 / 49 ms** | — | **17 / 837 ms** |

  El P4 sale por un puente USB-UART a 115 200, así que sufre los mismos 71 s que el STM32 y gana
  lo mismo; el C3 va por USB nativo y por eso su cifra de partida ya era baja — y aun así es la
  mejora **más grande de todas las placas**, 5,3×.

  🖥️ **La pantalla del P4 sigue**: `GuiColorDemo` arma su screen de **1 024×600** (el tamaño real
  del panel, o sea que `P2.3` también funciona aquí) y mata limpio.

  ⚠️ **El `AllocBench` del C3 sube un 10 % y NO es `A1`.** Es repetible (14 168 / 14 174), así que
  no es ruido; pero el control lo desmiente: la **C6** —mismo silicio, misma VM de 128 KB, misma
  arquitectura de dos hilos— midió 13 129 → 13 188 antes y después de `A1`, o sea plano. Como el
  «antes» del C3 es del 1-sep, el sospechoso natural es el planificador de memoria de `U6`, que
  fija el tamaño del heap y por tanto el ritmo del GC. Queda anotado como pregunta de `U6`, no de
  `A1`, y se contesta comparando el `vmHeapBytes` de antes y después.

  ### El S3, y la medida que zanja la duda del C3

  El S3 llegó con firmware del **3-sep**, o sea posterior a `U4`, `U5`, `U6` y `#466`: su «antes»
  es casi sólo `A1`, y por eso su número vale como control.

  | Medida | S3 antes | S3 después |
  |---|---|---|
  | `PrintBench` 2 000 líneas | 71 075 ms | **20 343 ms** (3,5×) |
  | · mensajes `OUTPUT` | 12 016 | **2 001** |
  | · bytes por el wire | 820 KB | **239 KB** |
  | `AllocBench` + GC | 20 244 ms | **20 384 ms** (+0,7 %) |
  | `Bench` fib(28) ×2 | — | 34 980 ms |
  | KILL calculando / imprimiendo | — | **31 / 535 ms** |

  **Y con esto la duda del C3 queda contestada del todo**: el `AllocBench` del S3, con una
  referencia posterior a `U6`, sale **plano**; el del C3, con referencia del 1-sep, subía un 10 %.
  Dos controles independientes (la C6 y ahora el S3) dicen lo mismo: **`A1` no cuesta nada en
  asignación**, y lo del C3 pasó entre el 1 y el 3 de septiembre.

  El S3 sale por un puente USB-UART a 115 200, como el P4 y el STM32 — de ahí los mismos 71 s de
  partida y la misma ganancia de 3,5×. 📌 Detalle de método: su puente exige **DTR y RTS afirmados
  a la vez**, así que los scripts de medida llevan ahora un `-Rts`; con sólo DTR la placa se queda
  muda y parece rota. En S3 y P4, `io` fijada al núcleo 0 y `vm` al 1. El P4 pierde `wire_uart`/`wire_v1`
  como tareas propias: pasan a ser el transporte de `io`. Si hace falta tocar algo de la familia
  para que arranquen, es que `A1.2` dejó algo en la C6 que era común.
- **✅ `A1.4` — HECHA (5-sep): la Pico 2 con las dos tareas.**

  Mismo gesto que en el ESP32 (un adaptador de `poll` y el arranque de `io` alrededor de
  `bpvm_run`, con el `v1_output_sink` de siempre), y el mismo interbloqueo con el depurador. Sólo
  hizo falta un detalle propio: el `poll` de esta placa necesita la VM (el latido del LED) y el
  contrato de `io` pasa un puntero de usuario, que aquí es el contexto del sink — así que la VM
  del RUN en curso se guarda en un estático, que es honesto porque `s_active_session` ya
  garantiza un RUN a la vez.

  **Medido en la Pico 2** (mismo `.mod`, firmware de un hilo del 3-sep → el de hoy):

  | Medida | 1 hilo | 2 hilos | |
  |---|---|---|---|
  | `Bench`: fib(28) ×2 | 17 157 ms | **17 091 ms** | igual |
  | `AllocBench`: 20 000 asignaciones + GC | 10 394 ms | **10 766 ms** | +3,6 % |
  | `PrintBench`: 2 000 líneas | 7 694 ms | **3 958 ms** | **1,9×** |
  | · mensajes `OUTPUT` | 12 016 | **2 001** | **6×** menos |
  | · bytes por el wire | 820 KB | **239 KB** | **3,4×** menos |
  | KILL calculando / imprimiendo | — | **33 / 85 ms** | |

  `comm_pico.c` (y `comm_host.c`) quedan inertes: sólo los referencia el camino SMP, que ya no
  arranca su comm task si hay `io`. Se borran cuando ese camino se rehaga sobre `io`, que es el
  trabajo de las dos VM sobre una cola — no antes, porque hoy son los únicos que dan esos
  símbolos.

  ### 🔬 Y la segunda cosa que sólo se ve en placa: LA PRIORIDAD ES CONTRATO

  Con el hilo `io` creado como un hilo cualquiera, en la Pico nacía a `tskIDLE_PRIORITY+1` y
  `vm_task` corre a `+2`: **por debajo**. Resultado: `io` no se ejecutaba mientras el programa
  calculaba, y un KILL enviado a mitad de un cálculo **no llegaba nunca** — el programa terminaba
  solo, 9,9 s después, con `EXITED OK` en vez de `KILLED`. En el ESP32 no se veía porque allí la
  tarea `main` y el hilo corriente van las dos a prioridad 1.

  Así que el contrato de plataforma gana una función, `bpvm_platform_thread_create_io`, y una
  regla con su número: **`io` a la MISMA prioridad que la tarea que ejecuta la VM, nunca por
  debajo — y tampoco por encima.** Las tres opciones, medidas:

  | `io` respecto a la VM | KILL calculando | `PrintBench` C6 | `PrintBench` Pico |
  |---|---|---|---|
  | por debajo (Pico, accidental) | **no llega** (9,9 s, `OK`) | — | — |
  | por encima (+1) | 36 ms | 848 ms | 4 832 ms |
  | **igual** | **15–33 ms** | **486 ms** | **3 958 ms** |

  Por encima funciona pero cuesta caudal: cada `print` despierta a `io` y expulsa a la VM, así
  que se drena en tragos pequeños. A la misma prioridad, el reparto por tiempo del RTOS le da
  turno de sobra y drena en tragos grandes. Donde no hay prioridades (el PC) es la creación de
  siempre; donde no hay hilos (STM32 hasta `A1.5`) devuelve -1 y `bpvm_io_start` se lo traga: la
  VM sigue por el camino de un hilo, como antes.

  📌 Dos bugs en dos días con la misma forma —**el PC verde y la placa no**— y los dos en la capa
  de plataforma, no en el diseño: la espera que no esperaba y la prioridad que no era. Es lo que
  la cascada promete y lo que un arnés de host solo no puede dar.
- **✅ `A1.5` — HECHA (5-sep): FreeRTOS en el STM32, y las dos placas con las dos tareas.**

  ### De dónde sale el kernel — y por qué no de ST

  Eduardo lo planteó: *«en teoría STM soporta FreeRTOS en su código»*. **En general sí; para el
  U5 no.** Comprobado, no supuesto: el paquete `STM32Cube_FW_U5_V1.8.0` no tiene ni un `port.c`,
  ni un `portmacro.h`, ni un `heap_*.c` — lo único que ahí se llama FreeRTOS es una **capa de
  emulación de su API sobre Azure RTOS ThreadX** (copyright Microsoft, `FreeRTOS.h` incluye
  `tx_api.h`). Y la base de datos de CubeMX **no ofrece FREERTOS para U5**, aunque sí para el
  **L552, que es también Cortex-M33** — o sea que es decisión de producto de ST, no límite del
  silicio (el grep negativo del U5 vale porque el mismo grep da positivo en F4 y L5: instrumento
  con control). El plugin `mcu.freertos` de CubeIDE es Java: vistas de tareas y colas en el
  depurador, cero fuentes — aunque eso, gratis, es justo la instrumentación que A1 necesitaba.

  Así que el kernel es **el mismo que ya compila la Pico**: `FreeRTOS-LTS`, kernel **V11.3.0**,
  puerto `ARM_CM33_NTZ`. Comprobado en su caché de build, no en un comentario:
  `pico/build/CMakeCache.txt` dice `FREERTOS_KERNEL_PATH:PATH=…/FreeRTOS-LTS/…`. ⚠️ En el disco
  hay **otra** copia (`C:/lenguajes/pm/FreeRTOS-Kernel`, V11.0.1+) y **no son la misma**: usar
  cada familia la suya sería tener dos kernels sin que nada lo dijera.

  ### La plataforma pasa a ser COMÚN, y ése es el cambio de fondo

  De las 363 líneas de `pico/platform_freertos.c`, **sólo cuatro** eran de la Pico. Copiarlo
  habría repetido el fallo que este proyecto ya tiene documentado tres veces —*el común crece y
  la copia privada no*—, así que la implementación vive en **`src/platform_freertos.c`** y cada
  familia pone sólo su reloj, su espera activa y su azar. Hoy lo compila el STM32; **migrar la
  Pico es ficha aparte** porque exige verificarla en placa, y mover código que funciona sin poder
  ejecutarlo es el «verde falso» que aquí se persigue.

  ### Lo que hizo falta en cada placa

  | | Nucleo U575 | Discovery U5G9J |
  |---|---|---|
  | `SysTick` para FreeRTOS | ya libre (el HAL latía en TIM17) | **hubo que mover el HAL al TIM17** (copiado tal cual de la Nucleo; TIM17 estaba libre) |
  | `SVC`/`PendSV`/`SysTick` en `stm32u5xx_it.c` | fuera (el puerto CM33 los define con esos nombres, y no son weak ni renombrables) | fuera |
  | Kernel en el build | tres carpetas enlazadas (patrón LVGL: sólo lo que se compila) + dos include paths | igual |
  | `main.c` | `while(1){repl}` → tarea `vm` (16 KB de pila, el mismo `_Min_Stack_Size` que era el MSP) + `vTaskStartScheduler` | igual |

  ### Medido en placa (bare-metal → dos tareas, mismo `.mod`)

  | Medida | Nucleo antes | Nucleo después | Discovery antes | Discovery después |
  |---|---|---|---|---|
  | `Bench` fib(28) ×2 | 15 570 ms | **15 602** | 19 440 ms | **17 317** (−11 %) |
  | `AllocBench` + GC | 8 946 ms | **9 050** (+1 %) | 10 192 ms | **9 555** (−6 %) |
  | `PrintBench` 2 000 líneas | 71 357 ms | **20 464** | 71 377 ms | **20 462** |
  | · mensajes `OUTPUT` | 12 016 | **2 001** | 12 016 | **2 001** |
  | · bytes por el wire | 820 KB | **239 KB** | 820 KB | **239 KB** |
  | KILL calculando | — | **15 ms** | — | **15 ms** |

  **Aquí la UART ES el cuello, más que en ninguna otra placa**: 820 018 B a 115 200 baud son
  exactamente esos 71 s, y por eso el ahorro de bytes vale 3,5× — el doble que en la Pico. Y el
  cálculo **mejora** en la Discovery porque la VM ya no sondea el wire entre cuantos: lo que en
  la C6 era el 1 % aquí costaba el 11 %.

  ### ✅ La GUI, vista (Eduardo, 5-sep): *«La pantalla de la Discovery con el GuiColorDemo se ve
  bien, todo OK»*

  `GuiColorDemo` arma su pantalla de 800×480 con los ocho botones y se queda viva bombeando LVGL
  desde `vm`, **con `io` corriendo en paralelo**. Es la comprobación que faltaba y la que más
  importaba de esta placa: meter un planificador debajo de un firmware que ya pintaba por LTDC
  era el paso con más cosas juntas (kernel nuevo, el reloj del HAL movido de periférico, y dos
  tareas compartiendo una UART). No se rompió nada.

  Queda para `A1.6` lo que aún no se ha hecho aquí: mover el bombeo de LVGL de `vm` a `io`. Hoy
  la pantalla se refresca desde el hilo que interpreta, que es lo que `A1` quiere cambiar.

  ### 🔬 El tercer fallo que sólo se ve en placa — y que el instrumento cazó

  La primera versión se paraba **a la quinta ejecución**. No fue un cuelgue mudo: el gancho de
  `configUSE_MALLOC_FAILED_HOOK` lo escribió en el log persistente —
  `RTOS: sin heap de FreeRTOS (configTOTAL_HEAP_SIZE:0) - parado` — y con esa línea el
  diagnóstico fue directo. **`vTaskDelete(NULL)` no libera la pila ni el TCB**: los deja en manos
  de la tarea OCIOSA, que con `vm` sondeando el wire a prioridad 2 no llega a correr nunca. Cada
  RUN creaba su `io` (4 KB) y ninguno se reciclaba. Ahora el hilo avisa y se **duerme**, y es
  `join` quien lo borra — borrar una tarea que no es la actual sí libera en el acto, sin depender
  de nadie. Tres pasadas seguidas con el mismo tiempo, en las dos placas.

  📌 Tres fallos en dos días con la misma forma —**el PC verde y la placa no**— y los tres en la
  capa de plataforma, no en el diseño: la espera que no esperaba, la prioridad que no era, y la
  memoria que nadie reciclaba. Es lo que la cascada promete.

  ⚠️ **Lo que hay que saber para reproducir el build**: el kernel vive FUERA de git
  (`.gitignore`), así que otra máquina necesita `FreeRTOS-LTS` en la misma ruta. Los dos
  `.project` (que son donde viven los enlaces) entran ahora en git con `add -f`; el del Nucleo
  no estaba trackeado hasta hoy.
- **🔄 `A1.6` — APLAZADA Y REORIENTADA (Eduardo, 5-sep): LVGL NO se mueve a `io`. Lo que quiere
  su propio hilo es `Gui.run()`, y un hilo BP.**

  *«Creo que podemos posponer esta decisión, de momento no lo movería a IO. El `Gui.run()`
  debería tener su propio hilo, pero hilo BP, para que pueda interactuar con el resto de la
  aplicación y que no la bloquee.»*

  ### Dónde corre hoy el lazo de LVGL, y qué es ese lazo

  La pregunta de Eduardo era esa, y la respuesta sale del código, no de la memoria:

  ```
  Gui.run()  →  while __guiRunOnce() do endwh          (bpstdlib/Gui.bp:789)
     →  builtin GUI_RUN_ONCE                            (src/builtins.c)
        →  bpvm_gui_lvgl_pump() → bpvm_gui_disp_pump()  (src/gui.c:289)
           →  lv_timer_handler()                        (el port de cada placa)
  ```

  **Lo conduce el programa BasicPlus, dentro de la tarea `vm`.** No lo bombea nadie más:
  comprobado que no hay ni una llamada en el REPL, ni en el arranque, ni en el `main` de ninguna
  familia. **Si el programa no está dentro de `Gui.run()`, LVGL no se ejecuta.**

  Y «el lazo de LVGL» son tres temporizadores: el **refresco** cada 33 ms
  (`LV_DEF_REFR_PERIOD`), que redibuja lo invalidado y llama al `flush_cb`; la **lectura del
  táctil** cada 30 ms, de donde salen los clics; y las **animaciones**. Se autorregula:
  `lv_timer_handler()` devuelve cuántos ms faltan para el siguiente, y el bombeo duerme ese rato.

  ### Por qué NO se mueve a `io`: el número ya estaba medido

  El comentario de `#424` en el P4 lo dejó escrito: el trabajo son **0,4 ms por vuelta** y el tope
  de 50 ms no saltó **ni una vez en 60 s** — *«el lazo no estaba ocupado, estaba DURMIENDO»*. Con
  una vuelta cada 33 ms eso es ~1 % de CPU. **LVGL no le está quitando tiempo al intérprete**, así
  que moverlo no se justifica por rendimiento; y sí costaría caro, porque LVGL **no es reentrante**
  (`LV_USE_OS = LV_OS_NONE`) y los `Gui.*` de BasicPlus crean widgets desde `vm`: tenerlo en `io`
  obligaría a un cerrojo en cada builtin o al reparto completo por colas.

  ### Por qué un hilo BP es mejor que un hilo de SO, y esto es lo que lo decide

  Los hilos de BasicPlus son **verdes**: `bpvm_thread_spawn` crea un `tc` en `vm->threads[]` y los
  turna el scheduler de la VM **dentro de la tarea `vm`** (`src/threading.c`). Dos hilos BP no
  corren nunca a la vez. O sea: **`Gui.run()` en un hilo BP no tiene problema de reentrancia con
  LVGL**, que es justo lo que hace caro moverlo a `io`. Y resuelve lo que de verdad molesta hoy:
  que `Gui.run()` no vuelve nunca, así que el programa no puede hacer nada más mientras la
  pantalla vive.

  ### ⚠️ El obstáculo concreto, para que la ficha futura no empiece de cero

  **El bombeo duerme el HILO DEL SO, no el hilo BP.** Los cuatro puertos acaban igual:

  | Placa | cómo espera |
  |---|---|
  | C6 | `vTaskDelay(ticks)` |
  | P4 | `vTaskDelay(...)` |
  | Discovery | `__WFI()` |
  | host | `SDL_Delay(16)` |

  Eso bloquea la tarea `vm` entera, es decir **todos** los hilos BP, no sólo el del GUI. Con
  `Gui.run()` en su propio hilo BP, el resto de la aplicación se congelaría hasta 10 ms por vuelta
  igualmente. Así que el trabajo de esta ficha es preciso: **el bombeo tiene que volver, no
  dormir**, y dejar que el scheduler de la VM le dé el turno a otro hilo BP; dormir de verdad sólo
  cuando no haya ningún hilo BP ejecutable, que es algo que el scheduler YA hace
  (`bpvm_platform_thread_sleep_ms` cuando nadie está RUNNABLE). Toca `src/gui.c` y los cuatro
  bombeos, y se mide con la latencia de un clic (`#434`) y con un programa que calcule mientras
  la pantalla vive.

  📌 `G1` sigue absorbida aquí, pero con este enunciado y no con el de mover LVGL de hilo.
- **✅ `A1.7` — HECHA (5-sep): el depurador por la cola de control. Se acaba la excepción.**

  Hasta hoy, un RUN con breakpoints arrancaba **sin `io`**: el `next_cmd` del depurador LEE el
  wire, y desde la tarea `vm`, así que con `io` en marcha habría dos lectores del mismo cable.
  Era un interbloqueo deliberado, pero dejaba el caso «depurando» fuera de la arquitectura de A1.

  **Lo que hay ahora**: `io` sigue siendo el único lector, y cuando la línea es del ramo de
  depuración la **deposita** en una cola de control (io → vm) de donde el `next_cmd` la saca. Y
  como durante una pausa quien contesta es la tarea `vm` mientras `io` puede estar drenando lo
  último que imprimió el programa, el cable gana un **cerrojo de escritura**: sin él, dos líneas
  se entrelazan y el IDE ve un JSON roto.

  En el común, cuatro funciones (`bpvm_io_ctrl_push/pop`, `bpvm_io_tx_lock/unlock`). La cola es
  de **líneas** y no de comandos ya parseados, a propósito: el JSON es de la familia, y subir su
  tipo al común sería meter aquí algo que no es de aquí. Cuatro líneas de capacidad, y si se
  llena **se descarta en vez de bloquear a `io`**, que también tiene que atender el KILL. En cada
  familia, tres costuras pequeñas. El STM32 no anuncia `DEBUG` y no tiene esa costura.

  **Verificado con una sesión de depuración completa por el wire, sin IDE** (`PAUSE` → `RUN` →
  `BP_HIT` → `LOCALS` → `STEP` → `CONTINUE` → `EXITED`) en **Pico 2, C3 y S3**. Y la prueba de
  que `io` corre de verdad mientras se depura no es que la sesión termine: es que **la salida del
  programa llega en 1 mensaje `OUTPUT` en vez de 16** — el troceado por líneas es cosa de `io`.
  Esa comprobación pilló que el S3 estaba corriendo todavía el firmware viejo.

  🔬 Y un fallo que sólo la placa podía enseñar: en el ESP32 el adaptador del `poll` pasa
  `vm = NULL` —el contrato de `io` da un puntero de usuario, no la VM— así que no encolaba nada, y
  el C3 paraba en el breakpoint y después no contestaba a nada. La VM del RUN en curso va en un
  estático, como ya hacía la Pico.

**Orden:** `A1.1` → `A1.2` → `A1.3` y `A1.4` en bloque → `A1.5` → `A1.6` → `A1.7`. Cada una con
los tres samples medidos antes y después en la placa que toque, y commit por paso. Si `OUTPUT`
por línea cambia algo observable del wire, `docs/BPVM_WIRE_PROTOCOL.md` lo dice (no debería: el
texto concatenado es el mismo).

Instrumentos: `samples/benchmarks/Bench.bp` (cálculo puro, cuanto por ENV), `PrintBench.bp`
(salida), `AllocBench.bp` (asignación y GC: 20 000 vueltas con dos concatenaciones = 13 129 ms
en la C6, 0,66 ms por vuelta; el reparto GC/concatenación está por separar con `log=1`).

#### ✅ `#474` — ~~`B1` sigue vivo, y su reproducción YA NO COMPILA~~ (abierta 5-sep · **CERRADA el 9-sep**)

✅ **CERRADA. Los tres pasos que pedía, hechos — y salieron DOS bugs más, en la otra VM.**

**1. La reproducción, revivida.** Esta ficha decía que `synclisttest.bp` choca con el bug aparcado
del compilador. **Es falso**: le faltaba un `import Core` y compila. Con eso se pudo medir por
primera vez desde junio.

**2. `B1` seguía vivo**, con su firma exacta —«HALT en thread no-main (tid=2 en PC 9166)» desde el
worker 2 y «(tid=2 en PC 9164)» desde el 3, dos workers sobre el mismo contexto— pero mucho menos
frecuente que en V2: `w2` de ~25 % a 0 %, `w4` de 100 % a ~5 %.

**3. Arreglado, y el método es lo aprovechable.** Iba a auditar los 11 sitios que cambian el estado
leyéndolos. **Criterio de Eduardo (9-sep):** *«aunque esté escrito en C el modelo bueno es el de
OOP. Si tratas al Thread como una clase, debe haber un SOLO método para cambiar el estado, el
setter, y es ahí donde proteges. Si luego se llama de 11 sitios diferentes eso ya es otro tema.»*
Con el estado privado y un setter que **denuncia** la transición ilegal, el culpable se nombró solo:

```
[B1] tid=1 lo está EJECUTANDO worker-3 y worker-1 le cambia el estado a RUNNABLE desde el case YIELD
```

⚠️ **Y un detalle de Java que casi lo tapa**: `private` NO bastaba. `ThreadContext` es una clase
**anidada** dentro de `VirtualMachine`, y en Java la externa ve los privados de la anidada — que es
justo quien escribía los 11 sitios. Hubo que **renombrar el campo** para que el compilador obligase
a pasar por el setter.

**La causa**: mientras el worker salía de `runOnContext`, el thread se volvía RUNNABLE por su cuenta
(un `MUTEX_UNLOCK` le entregaba el mutex) y **otro worker lo reclamaba**. Entonces `getStatus()`
volvía a decir RUNNING —pero de otro— y re-encolarlo metía el mismo contexto dos veces.
**El arreglo**: sólo decide sobre el thread quien sigue siendo su dueño. Dos líneas.

### 🔴 Y al preguntar Eduardo «¿y en la VM-C qué ocurre?», salieron dos más

La cascada del proyecto es Java → C → placa, y faltaba la segunda. La VM-C **no** tenía `B1` —su
planificador hace el claim bien, con `sched_owner`, que es justo el arreglo que se le puso a miVM—
pero se colgaba igual, **7 de cada 8 veces con `--smp=2`**. La bisección lo acotó sin tocar nada:
sin mutex (`smp_fib_bench`) `--smp=2` iba limpio y escalaba **×1,90**; con mutex, cuelgue.

- **El mutex no era atómico.** El check-and-set de la propiedad se hacía a pelo en el builtin y el
  intérprete corre **sin `vm_lock`**. Y peor: un **lost wakeup** — el thread se apuntaba como waiter,
  aún sin ponerse `BLOCKED_MUTEX`; otro hacía `unlock`, le daba la propiedad y lo ponía `RUNNABLE`;
  y entonces el primero ejecutaba `status = BLOCKED_MUTEX` y **pisaba su propio despertar**.
- **El GC podía tener DOS colectores esperándose.** `gc_stw` aguardaba a que `running_workers`
  bajara de 1, y nada impedía que dos entraran: los dos son workers en ejecución. Lo **predijo el
  modelo** que dio Eduardo ese día —*«en principio las 2 VM no interactúan entre ellas»*—: la única
  interacción que no es la cola de threads BP era el GC, y era la única parte que seguía rota.

📊 **Medido, de punta a punta:**

| | antes | tras el mutex | tras el GC |
|---|---|---|---|
| `--smp=1` | 0/12 | 0/12 | **0/20** |
| `--smp=2` | 7 de cada 8 | 1/30 | **0/20 y 0/40** |
| `--smp=4` | 7 de cada 8 | 5/12 | **0/20** |

Y en miVM: `w2`, `w4` y `w8` a **0 fallos en 40 pasadas cada uno**, con el detector mudo.

💰 **El pago**, que es para lo que se arregla — `smp_fib_bench` con su propio reloj en miVM:
`workers=1` 4406 ms · `workers=2` **2589 ms (×1,69)** · `workers=4` **2393 ms (×1,83)**. El SMP
vuelve a servir, y correcto. Estaba bloqueado desde junio.

⚠️ **Lo que esto NO demuestra**: que `B1` no tuviera otras caras. Lo medido es que la carrera
observada desaparece y que el detector queda mudo. Se caracterizó en V2 y pudo tener más
manifestaciones.

📌 **Y no se cambia ningún defecto**: miVM sigue arrancando con 1 worker y el SMP sigue opt-in.
Encenderlo es decisión de Eduardo, y en firmware manda `A4` (*«un solo núcleo hasta nueva orden»*).
El modelo final de los micros quedó escrito en `docs/V6_IDEAS.md`.


**El rumbo de Eduardo (5-sep)**, y por qué esto importa ahora:

> *«Los 2 núcleos no hacen un aporte significativo si uno ejecuta IO y el otro la VM. La
> diferencia será importante cuando tengamos 2 VM y varios hilos BP en marcha, ahí sí que podremos
> casi duplicar la velocidad. Ya lo intentamos y tuvimos que desactivarlo pero pudimos medir un
> rendimiento de 193 %.»*

**Tiene razón en las dos mitades, y las dos están documentadas.**

**La velocidad, medida** (`docs/SMP_ARCH.md`): `--smp=1` 203 ms vs `--smp=2` 103 ms → **×1,97**, y
en `SmpFibBench` **×1,9**, «≈ ideal lineal». Ése es el 193 % que recuerda.

**Y por qué se desactivó: `B1`** (`docs/HECHO_V2.md`, caracterizada el 6-jun-2026). Una **carrera
de datos en el `mem[]` compartido** que sólo muerde con paralelismo REAL:

| workers | fallo |
|---|---|
| 1 | **0 %** (el modelo del device) |
| 2 | ~25 % |
| 4 | **100 %** |

- **No es el GC**: con heap de 64 MB y **0 GC medidos**, w2 seguía fallando ~25 %.
- **Firma**: corrupción del `pc`/`bp` de un thread — «HALT en PC basura», «INVOKE_VIRTUAL null
  receiver». Un volcado sin GC mostró `mutex owner=2` con tid=2 RUNNABLE y tid=1 RUNNING petando:
  carrera worker↔worker.
- **Los sospechosos obvios se descartaron leyendo el código** (el claim del scheduler es atómico,
  el hand-off del mutex es correcto, el guardado de `pc/sp/bp/cs` está en el `finally` de toda
  salida). O sea que es sutil, y por eso sigue ahí.

**Decisión de entonces**: miVM pasa a 1 worker por defecto, el SMP queda opt-in y experimental, y
**el arreglo de `B1` se acopla a encender el dual-core**. Eso nunca llegó.

### 🔴 Y lo que se ha visto hoy al ir a comprobarlo

**La reproducción de `B1` ya no compila.** `samples/synclisttest.bp` usaba `SyncList` como
builtin, y `#450`/`#451` la movieron a `Collections` (donde extiende `Core.List`). Le falta el
`import` — se lo he puesto — pero aun así choca con el bug del compilador ya aparcado
(*«la pasada interfaz-only tira firmas»*): `clase base 'Core.List' no existe`.

📌 O sea: **el único instrumento que caracterizó una carrera bloqueante lleva meses sin funcionar
y nadie lo notó, porque nadie lo ejecutaba.** Es la forma exacta del *«instrumento mudo»* que este
proyecto ya tiene fichada, aplicada al peor sitio posible: el día que se quiera el futuro de dos
VM, se empezaría sin la herramienta que lo diagnosticó.

### ⏭️ Lo que hay que hacer, en orden

1. **Revivir la reproducción**: que `synclisttest.bp` compile y corra con `--workers=2/4`. Choca
   con un bug del compilador aparcado, así que puede que haya que desatascar ése primero — o
   escribir una reproducción nueva que no dependa de herencia entre módulos.
2. **Comprobar si `B1` sigue estando**: han pasado V4 (handles, GC preciso), V5 y V6 por encima.
   Puede seguir igual, puede haber cambiado de firma, o puede haberse ido por accidente. **Sin la
   medida no se sabe**, y esta ficha existe para que no se dé por supuesto ninguno de los tres.
3. Sólo entonces, arreglarlo — y entonces sí, las dos VM sobre una cola de hilos BP.

⚠️ **Y el aviso que engancha con `A4`**: el reparto de hoy (`vm` + `io`) **no toca `B1`**, porque
`io` no ejecuta opcodes ni toca `mem[]`. Los dos núcleos en la configuración actual no lo
despiertan — pero tampoco aportan nada, como Eduardo dice y como midió el S3 (un núcleo: mismo
tiempo, y el KILL incluso mejor). El paralelismo que vale es el otro, y ése está bloqueado por
`B1`.

#### 🧊 `A4` — LOS DOS NÚCLEOS: inventario del estado compartido antes de activarlos (abierta 5-sep · **el inventario está HECHO** · 🧊 **FUERA DEL PLAN DE VERSIONES el 9-sep**: ni V6 ni previsiblemente V7)

### 🧊 9-sep — FUERA DEL PLAN DE VERSIONES (Eduardo)

> *«Los 2 núcleos no sé a dónde van, pero no es V6 (y probablemente tampoco V7). Hay muchas cosas
> antes de meternos con los 2 núcleos.»*

Así que `A4` **deja de ser un pendiente de V6** y **no se apunta a V7**: queda sin versión asignada.
No es un aplazamiento con fecha, es una prioridad — hay cola por delante.

📌 **Lo que sí queda ganado, y conviene no perderlo de vista**: el terreno está más limpio que
cuando se aparcó. El 9-sep se mataron **tres carreras** que hacían el SMP inservible (`#474`) —
`--smp=2` pasó de colgarse 7 de cada 8 veces a **0 de 40**—, y el modelo final de los micros quedó
escrito en `docs/V6_IDEAS.md`. Cuando esto se retome, se retoma sobre algo que funciona, no sobre un
SMP roto.

⚠️ **Y por tanto el defecto no cambia**: un worker, en las dos VMs y en firmware. El SMP sigue
**opt-in** (`--smp=N` / `workers=N`), que es justo lo que ya decía esta ficha.

**El encargo de Eduardo**, y el orden que fijó:

> *«La utilización de 2 núcleos debe hacerse de forma controlada. Antes de hacerla hay que mirar
> si tiene consecuencias, no queremos corrupciones de memoria de forma aleatoria. Lo ideal es
> utilizar los 2 núcleos pero paso a paso.»* — y la orden: **un solo núcleo hasta nueva orden.**

### 0. Lo primero que apareció: el riesgo YA estaba abierto

El S3 y el P4 tienen dos núcleos y **ESP-IDF los usaba por defecto**. Nadie lo había decidido, y
`sdkconfig` está fuera de git, así que ni siquiera constaba. Con `A1` eso dejó de ser inocuo: la
tarea `vm` y el hilo `io` pueden solaparse **de verdad** sobre estado que nunca se había mirado.

✅ **Hecho (`c080e1f0`)**: `CONFIG_FREERTOS_UNICORE=y` en los `sdkconfig.defaults` —versionados—
del S3 y el P4. Las cinco familias corren ahora en un núcleo, y las cinco lo dicen en un fichero
que viaja. **Y no cuesta nada**, medido en el S3:

| | 2 núcleos | 1 núcleo |
|---|---|---|
| 2 000 líneas | 20 343 ms | 20 363 ms |
| `AllocBench` + GC | 20 384 ms | **20 130 ms** |
| KILL calculando | 31 ms | **18 ms** |

De regalo, las imágenes adelgazan (~11 KB el S3). El segundo núcleo no estaba comprando nada
porque el cuello es la UART.

### 1. 🔴 Y el repaso encontró una carrera a la primera — puesta ese mismo día

`s_line_buf` es el buffer del hilo `io`: ahí mete la línea que lee del cable. `A1.7` hizo que el
`next_cmd` del depurador sacara su comando de la cola de control **escribiendo en ese mismo
buffer**, y ése lo lee la tarea `vm`. Dos tareas escribiendo el mismo array, en la familia ESP32 y
en la Pico.

En un núcleo la ventana es un cambio de tarea; en dos, solapamiento real, y el síntoma sería un
JSON del depurador cortado a media línea — la clase de fallo que sale una vez de cada cien y se le
echa la culpa al cable. ✅ Arreglado (`64b6f2fb`): **un buffer por rol**.

📌 Es la respuesta empírica al encargo: *mirar antes* no era formalismo, la primera consecuencia
estaba ahí.

### 2. El inventario, completo

| Estado compartido | Escribe | Lee | Protección | Veredicto |
|---|---|---|---|---|
| `vm->kill_requested` | `io` | `vm` | `volatile int`, una palabra alineada | 🟡 vale hoy; con dos núcleos debería ser atómico con orden explícito |
| `vm->io` (el puntero) | `vm` (start/stop) | `vm` | se publica **antes** de crear el hilo y se anula **tras** el join | ✅ |
| Cola de salida | `vm` empuja · `io` saca | | mutex + condvar | ✅ |
| Cola de control | `io` empuja · `vm` saca | | mutex + condvar | ✅ |
| El cable (dos escritores) | `io` y `vm` (depurador) | | `bpvm_io_tx_lock` | ✅ |
| `s_line_buf` (familia) | `io` | `io` | un solo rol **desde hoy** | ✅ (era el fallo del punto 1) |
| `s_dbg_buf` (familia) | `vm` | `vm` | un solo rol | ✅ |
| `s_out_esc` / `s_out_msg` (STM32) | `io` (el sink) | `io` | sólo los usa el sink | ✅ |
| El sink del ESP32 y de la Pico | pila (`char buf[1024]`) | | por construcción | ✅ |
| `wire_v1_send_error` / `..._reply_empty` | pila (`buf[512]`, `buf[64]`) | | por construcción | ✅ |
| `s_kill_ack_id` | `io` (poll) | `vm` (tras el run) | ordenado por el join de `bpvm_io_stop` | ✅ |
| El heap de la VM | sólo `vm` | | `io` no reserva nada en su lazo | ✅ |

### 3. Lo que falta ANTES de activar el segundo núcleo

1. 🟡 **`kill_requested` con barrera.** `volatile` impide que el compilador lo cachee, pero no
   ordena nada entre núcleos. En estos chips la caché es coherente y funciona, pero «funciona»
   no es «está garantizado»: con dos núcleos debe ser un atómico con orden explícito.
2. 🟡 **Sólo la Pico tiene cerrojo en su transporte** (`wire_v1_tx_lock`). El ESP32 y el STM32 se
   apoyan en el cerrojo común, que cubre los dos escritores que conocemos — pero en la Pico el
   **`printf` de consola y el wire comparten el mismo USB**, y el `printf` no toma ese cerrojo.
3. 🟢 **Y entonces sí**: fijar `io` a un núcleo y `vm` al otro (el reparto que la Pico ya tiene
   escrito para cuando cierre `#153`), y medir. No antes.

📌 **Por qué el orden importa**, y es la lección de esta ficha: activar los dos núcleos no habría
dado un fallo el primer día. Habría dado un JSON roto cada varios cientos de sesiones de
depuración, meses después, sin forma de atarlo al cambio. Mirar primero costó una tarde y encontró
la carrera con el código delante.

#### ✅ `A3` — EL MODELO DE CAPAS, AUDITADO CONTRA EL CÓDIGO (**AUDITORÍA HECHA el 5-sep**; sus fugas viven como fichas propias: `#469`–`#473`, y `#473` guarda las 78 sin verificar)

**El enunciado de Eduardo**, que es lo que se audita:

> *«El hardware de cada fabricante, el HAL de cada fabricante, la HAL BP es nuestra pero depende
> sobre todo de cada familia. La interfaz de HAL BP debería ser común y todo lo que hay por encima
> también. Eso nos indica cómo deben ser las cosas, prácticamente todo es independiente del micro,
> con algunas particularidades que se pueden generalizar.»*

```
   hardware del fabricante          RP2350 · ESP32 S3/C3/C6/P4 · STM32U5
   HAL del fabricante               Pico SDK · ESP-IDF · STM32 HAL          (ajeno)
   ─────────────────────────────────────────────────────────────────────
   HAL BP (implementación)          pico/ · esp32/common/ + esp32*/main/ · stm32/port/
   ══ la INTERFAZ de la HAL BP ══   include/bpvm_*.h        ← debe ser COMÚN
   todo lo de encima                src/ (intérprete, GC, REPL, io, GUI, packs)
   y encima del todo                bpstdlib/*.bp — lo que ve el usuario
```

### 1. Dónde SE CUMPLE — y se cumple mucho mejor de lo que yo esperaba

Lo comprobé con greps sobre las 31 672 líneas del común (`src/` + `include/`):

- **Cero includes de un SDK.** Ni un `esp_*.h`, `pico/*.h`, `stm32*.h` ni `hardware/*` en todo el
  común. La única excepción es `src/platform_freertos.c`, que incluye `FreeRTOS.h` — y no es una
  fuga: ese fichero **es** una implementación de la HAL BP, vive por debajo de la línea aunque
  esté guardado en `src/`.
- **Cero llamadas a funciones de familia.** Ni un `pico_*()`, `esp32_*()`, `stm32_*()` ni `HAL_*()`
  desde `src/` o `include/`.
- **Y lo más revelador: los `#ifdef` del común son por CAPACIDAD, no por silicio.** De 321
  directivas de preprocesador, las que mandan son `BPVM_LVGL` (72) y `BPVM_GUI` (10). Sólo **4**
  miran a una familia o placa, un 1,25 %. El común varía según *qué sabe hacer* la placa, no según
  *cuál* es. Eso es exactamente el modelo funcionando.
- Y hay **17 fachadas** con la forma correcta (un `..._backend_t` que la familia registra), con las
  16 de periférico sumando 642 líneas de común: fino, que es como debe ser una interfaz.

**Conclusión: el modelo se cumple, y donde falla es casi siempre por debajo de la línea o por un
descuido concreto, no por diseño.** Pero hay tres fugas que sí importan.

### 2. 🔴 La fuga grave: una fachada sin backend miente, y no lo dice

**En una STM32, `Adc.read()` devuelve un número inventado por el stub del PC.** Comprobado:

```
grep bpvm_adc_set_backend  →  sólo pico/main.c:1543 y esp32/common/gpio_esp32.c:629
```

El STM32 **no registra backend de ADC**. Y `src/adc.c`, cuando no hay backend, no falla: imprime
`[adc] initChannel(0) → GP26 (stub)` —**el pinout del RP2350, escrito en código común y ejecutado
en una placa ST**— y `readChannel` devuelve una **rampa**: un contador que avanza de 73 en 73 y da
la vuelta a 4095. Un número que se mueve, que parece una lectura y que es falso.

Y la red de seguridad **no puede verlo por construcción**: la VM-Java hace lo mismo
(`VirtualMachine.java:5495`), así que la paridad dual-VM sale verde. Es el caso exacto de la norma
del proyecto *«errores sí, silenciosos no»* y del *«aviso que no distingue no-evento de fallo»*.

📌 **Lo que enseña sobre el modelo**: un stub que finge convierte «esta familia no lo implementa»
en «esto funciona mal». La fachada debería distinguir **no hay hardware** (el host, legítimo) de
**nadie registró backend** (un olvido), y en el segundo caso fallar con ruido. Eso protege a todas
las familias futuras, no sólo al STM32.

### 3. 🟠 La identidad de la placa se contesta por dos caminos, y ya divergieron

Los mismos seis datos (nombre, MHz, GPIO, ADC, PWM, causa de reset) se responden **dos veces por
familia**: una para BasicPlus (la fachada `bpvm_pico_*`) y otra para el wire (`bpvm_repl_info_t`,
rellenada a mano). Verificado que ya no coinciden:

| | por el wire | por la fachada (lo que ve el programa BP) |
|---|---|---|
| **STM32** | 114 GPIO (`stm32_repl.c:146`) | 128 (`gpio_stm32.c:139`) |
| **ESP32-C3** | «ESP32-C3», 160 MHz, 22 GPIO | **«esp32s3-devkitc», 240 MHz, 45 GPIO** |
| **Pico** | 24 PWM (`repl_v1.c:632`) | 12 (`main.c:679`) |

El caso del **C3 y el C6 es el peor**: su identidad se arregló *sólo* en la vía del wire
(`c3_board_id.c` llama a `repl_set_board_id`, pero nadie llama a `bpvm_pico_set_backend`), así que
**un programa BasicPlus corriendo en un C3 se cree un S3**, con cinco de seis campos falsos
incluido el nombre. Es exactamente el bug que la cabecera de `c3_board_id.c` dice haber arreglado
— corregido en un camino y vivo en el otro.

### 4. 🟡 El nombre `Pico` atraviesa todas las capas hasta el usuario

`include/bpvm_pico.h` **no es la fachada de una familia**: es la de «información del MCU», y las
cinco la implementan. Pero se llama `pico`, y el nombre sube hasta arriba: la stdlib expone un
módulo **`Pico`**, así que un programa en una STM32 escribe `Pico.uptimeMs()` y
`Pico.cpuFreqHz()`. Lo confirman mis propios bancos de hoy, que llamaban a `Pico.uptimeMs()` en la
Nucleo, la Discovery, el S3, el C3, el C6 y el P4.

La interfaz es común y correcta; lo que está mal es **el nombre**, y no es inocuo: nadie busca la
identidad de un STM32 en un fichero que se llama «pico», y por eso el duplicado del punto 3 pasó
desapercibido. Mismo vicio, más pequeño: `pio_count` y `pwm_slices` son vocabulario del RP2350 en
un struct común (`bpvm_repl.h:86`), y `pwm_slices` además significa **cosas distintas** en cada
familia — slices en la Pico (12), salidas en el STM32 (28), canales LEDC en el ESP32 (8).

### 5. 🟡 Y una fuga pequeña que ya mordió: nombrar familias en vez de capacidades

`src/bpvm_aot_helpers.c:25` decide si usar TLS con `#if defined(BPVM_PICO_NUM_CORES) ||
defined(ESP_PLATFORM)`. Su propio comentario dice «en MCU un global plano basta» — pero nombra dos
familias en vez de la capacidad. **El STM32, que llegó después, cae en la rama del PC**, y se
comprueba en el artefacto: en el `.elf` del Nucleo está `__emutls_v.g_aot_fault`, o sea **TLS
emulada, con un `malloc` detrás, en un microcontrolador**, donde se quería un global.

### 6. La regla práctica que sale de todo esto

1. **En el común no se nombra un silicio: se nombra una capacidad.** `#ifdef BPVM_GUI` sí;
   `#ifdef ESP_PLATFORM` no. El aviso de que algo está mal es tener que añadir una familia a una
   lista: la siguiente que llegue caerá en la rama equivocada, en silencio.
2. **Una fachada sin backend registrado falla con ruido, no devuelve un valor plausible.** Un
   stub que finge no es una red de seguridad: es un fallo silencioso con retraso.
3. **Un dato se contesta desde UN sitio.** Si el wire y BasicPlus responden lo mismo por caminos
   distintos, divergen — y aquí ya lo han hecho tres veces.
4. **Las particularidades del silicio bajan, no suben.** Un driver de pantalla depende del panel y
   eso está bien; lo que no puede es que su vocabulario aparezca en la interfaz de arriba.

### ⏭️ Lo que queda propuesto

- **`#469`** — la fachada de ADC miente en el STM32: registrar backend o hacer que el stub grite.
- **`#470`** — la identidad de placa, de una sola fuente; y arreglar la vía BP del C3 y el C6, que
  hoy se presentan como un S3.
- **`#471`** — renombrar `bpvm_pico_*` y el módulo `Pico` de la stdlib a algo agnóstico (`Board`,
  `Mcu`…). Toca la stdlib, así que es cambio de lenguaje y decide Eduardo.
- **`#472`** — `bpvm_aot_helpers.c`: cambiar los dos macros de familia por uno de capacidad.

⚠️ **Honestidad sobre el alcance de esta auditoría**: los seis auditores terminaron, pero la fase
de verificación adversarial se cortó por límite de sesión — **12 de 90 fugas quedaron
verificadas**. Todo lo que se afirma arriba está comprobado por mí en el código o en el artefacto;
el resto de hallazgos (duplicación del verbo RUN escrito cuatro veces, el sink de OUTPUT cinco
veces, el `EXITED` del STM32 que interpola cadenas del usuario **sin escapar** en el JSON…) están
en el registro de la auditoría **sin verificar**, y no se dan por buenos hasta comprobarlos.

#### ✅ `A2` — EL CENSO DE PROPORCIONES: cuánto código es común, de familia y de placa (**HECHO el 5-sep**: 91,8 % común, 7,6 % familia, 0,7 % placa)

**La pregunta de Eduardo**, al parar tras `A1`: *«me gustaría conocer las proporciones de código
específico del micro, el específico de la familia y el común a todos los micros. Después de A1
debería quedar muy poco código específico.»*

### Cómo se mide, y por qué así

Contar líneas por carpeta mide **lo que está escrito**. La pregunta es otra: cuánto de cada
**firmware** viene de cada sitio. Así que el censo se hace por el **artefacto** — el `.map` del
enlazador, que dice qué metió en la imagen y cuánto ocupa.

No es una sutileza. En la Nucleo, `gui_display_sdl.o` **está en el build y aporta 0 bytes**: el
fichero entero es un `#ifdef` que no se cumple. Contando por carpeta sumaría 270 líneas de
«común»; contando por artefacto, cero. Y el `.map` tiene otra ventaja: ya está hecho, así que no
hace falta el toolchain de cada arquitectura.

**Las tres trampas** que el traspaso dejó anotadas, y cómo se respetan:
1. Los **generados** (los blobs de la stdlib, `*_mods.c`: 13 016 líneas) no son código escrito.
   Fuera — y además el censo sólo mira secciones de **código**, no de datos, así que un blob de
   48 KB en `.rodata` no puede falsear el reparto.
2. Los **`Core/` de CubeMX** los genera ST: se cuentan **aparte**, no como código nuestro.
3. **«Común» no es «común en uso»**: por eso se mide por imagen y no en el árbol.

El guion queda en el repo (`bpgenvm-c/scripts/censo_reparto.py`), así que esto se puede repetir
después de cada hito y ver la tendencia, que es lo que de verdad dice si la unificación avanza.

### El reparto, por imagen (bytes de código de NUESTRO código)

| Imagen | común | familia | placa | total | reparto |
|---|---|---|---|---|---|
| **Discovery U5G9J** | 223 134 | 11 050 | 0 | 234 184 | **95,3 %** / 4,7 % / 0 % |
| **Nucleo U575** | 190 462 | 10 604 | 0 | 201 066 | **94,7 %** / 5,3 % / 0 % |
| **ESP32-S3** | 109 882 | 9 282 | 0 | 119 164 | **92,2 %** / 7,8 % / 0 % |
| **ESP32-C6** | 143 168 | 12 472 | 1 100 | 156 740 | **91,3 %** / 8,0 % / 0,7 % |
| **ESP32-C3** | 127 514 | 12 068 | 12 | 139 594 | **91,3 %** / 8,6 % / 0,0 % |
| **ESP32-P4** | 151 550 | 13 320 | 6 450 | 171 320 | **88,5 %** / 7,8 % / 3,8 % |
| **Pico 2 (RP2350)** | 90 018 | 16 430 | 0 | 106 448 | **84,6 %** / 15,4 % / 0 % |
| **Las siete juntas** | 1 035 728 | 85 226 | 7 562 | 1 128 516 | **91,8 % / 7,6 % / 0,7 %** |

Aparte, de ST y no nuestro: **29 844 B** de `Core/` en la Discovery y **102 B** en la Nucleo (la
diferencia es la BSP de la pantalla, que la Discovery sí usa).

### Lo que dicen los números

**La hipótesis de Eduardo se sostiene, y con margen: el 92 % del código de cada firmware es
común.** Lo específico de un micro concreto es **0,7 %**.

Y hay tres cosas que el número solo no cuenta:

- **La placa casi no existe como categoría.** Sólo el P4 (3,8 %) y el C6 (0,7 %) tienen código
  propio, y en los dos casos es **la pantalla**: `gui_display_dsi.c` y `gui_display_st7789.c`. Es
  exactamente donde debe estar lo específico — el silicio de un panel no se puede abstraer— y
  todo lo demás de esas placas ya vive en su familia o en el común.
- **La Pico es la familia menos unificada** (15,4 %), y no por casualidad: es la más antigua, y
  cosas que las otras familias tienen en `esp32/common` o en `src/` ella las tiene en `pico/`
  (su REPL son 840 líneas, su `main.c` 1 036). Es el sitio con más recorrido si se quiere subir
  el 92 %. La migración de su plataforma a `src/platform_freertos.c` iba en esa dirección y se
  paró por una medida (ver `A1.3`).
- **Las dos STM32 son las más comunes de todas** (95 %), lo cual es llamativo porque son las que
  más tarde llegaron y las únicas que necesitaron meter un RTOS entero. Dice algo bueno del
  reparto: la familia sólo pone la cintura (`stm32/port`), y el kernel es de fuera.

📌 **Y un aviso sobre qué NO mide esto**: bytes de código, no esfuerzo. Los 6 450 B del P4 son un
driver MIPI-DSI que costó varias sesiones; los 223 134 B comunes de la Discovery incluyen el
intérprete entero, que ya estaba escrito. El censo dice dónde vive el código, no dónde está el
trabajo.

#### ✅ G1 — ~~el bucle de LVGL a un hilo BP propio~~ (**CERRADO el 7-sep**)

### ✅ La forma, decidida por Eduardo el 6-sep

> *«Dejar `Gui.run()` síncrono y `Gui.start()` asíncrono. Pero no lo hacemos hoy, en otra sesión.»*

- **`Gui.run()` SIGUE BLOQUEANDO**, pero por dentro pasa a ser *«lanza el hilo del GUI y espéralo»*.
- **`Gui.start()`** — lanza el hilo y vuelve.
- **`Gui.stop()`** — termina el lazo desde otro hilo. Esto es lo que hace posible **interrumpirlo**,
  que es una de las tres cosas que Eduardo pedía: *«ajustar el bucle, interrumpirlo o salir del
  programa»*.

**Por qué `run()` no puede dejar de bloquear** — la restricción que fija la forma, comprobada: varios
samples tienen código **después** de `Gui.run()` y dependen de ello. `GuiCheckDemo` imprime
«== despues ==», `GuiClickDemo` «== despues del clic ==», `GuiEvLat` «fin». Cambiar el significado
les cambia la salida, y son justo los que quieren entrar en el corpus (`#477`). Aditivo o nada.

**Lo que YA está hecho y no hay que rehacer** (`#324`): el lazo **ya vive en BP** —
`Gui.run()` es literalmente `while __guiRunOnce() do endwh`—, no dentro del builtin. `G1` no es sacar
el lazo: es **de qué hilo cuelga**.

**Por qué hilo BP y no `io`** (decisión de `A1.6`, y se sostiene sola): todos los hilos BP corren
sobre la MISMA tarea de SO (`vm`), interleavados por el scheduler entre quanta — así que **LVGL se
sigue llamando desde un solo hilo de SO**. Con `io` habría un segundo hilo de SO tocando algo que no
es reentrante.

**Y salir del programa sale gratis**: el scheduler para cuando **no queda ningún hilo vivo**
(`scheduler.c:91`, `if (!any_alive(vm)) break;`), no cuando `Main` vuelve. El hilo del GUI mantiene
vivo el programa por sí solo.

### ✅ HECHO el 7-sep

`Gui.start()` / `Gui.stop()` / `Gui.join()`, con el lazo en un `HiloGui extends Thread`. Y
`Gui.run()` **sigue bloqueando** —decisión de Eduardo: *«bloquea el hilo de ejecución, eso siempre ha
sido así; personalmente no me gusta pero se puede mantener por compatibilidad»*—, implementado como
`start()` + `join()`.

**Comprobado**, VM-C headless, tres pasadas por sample:

```
1: antes de start
2: start() VOLVIO — el lazo corre en su hilo
3: el hilo principal sigue vivo, vuelta 0 / 1 / 2
4: pido la parada
5: el lazo ha terminado
```

⚠️ **CAMBIA EL ORDEN DE LOS HANDLERS, y Eduardo lo dio por bueno.** Con el lazo en su hilo, el
scheduler puede inyectar el frame del handler en un hilo que no es el que espera, así que
`GuiCheckDemo` y `GuiClickDemo` despachan el `onChange`/`onClick` **dentro** de `run()` en vez de
después. **Es lo que `#324` perseguía** —el propio sample dice llamar a `run()` «para drenar el
evento»— y en `GuiClickDemo` el árbol de después ya refleja lo que hizo el handler. `GuiColorDemo` y
`GuiDemo` salen idénticos.

🔴 **RETRACTADA una de las dos «mejoras» que yo había apuntado aquí: «el lazo NO duerme» era FALSO.**
Duermen **todos** los bombeos, y cada uno como le toca a su plataforma: `SDL_Delay(16)` en el host
(`gui_display_sdl.c:378`), `vTaskDelay` con el idle que sugiere LVGL en el C6 y el P4, `__WFI()` en
el STM32 (`gui_display_ltdc.c:143`). Y miVM argumenta dónde va esa espera: *«puesto donde el bombeo
puede pagarlo sin que el lazo BP tenga que saber de tiempos»* (`VirtualMachine.java`, `guiEventLoopOnce`).
Mi `sleep` en el lazo BP no ajustaba el ritmo: **sumaba una segunda espera**. Quitado.

🟡 **Sigue en pie el otro**: `__guiRunOnce` recorre la tabla de símbolos **entera en cada pasada**
(`builtins.c:1030`) para localizar `Gui.__guiDispatch` y `Gui.__guiDispatchChange`. No se ha tocado.

🔴 **Y aparece un bloqueo que explica `#477` mejor de lo que estaba escrito: miVM NO TERMINA los
samples de GUI.** Cuatro de cuatro pasadas se cuelgan esperando un cierre de ventana que el programa
nunca hace — **con este cambio y sin él**, comprobado con el `.mod` de antes. O sea que los samples
de GUI no están en el corpus **no porque nadie los metiera, sino porque no pueden correr
desatendidos en la VM de referencia**. Sin resolver eso, la paridad de GUI no se puede verificar.

✅ **RESUELTO el 7-sep (`1e46f346`), y la causa era mucho más pequeña de lo que este párrafo
sugiere.** No es que miVM no sepa terminar un programa de GUI: es que el `JFrame` de `GuiBackend`
se crea y **nunca se destruye**, y el EDT de AWT no es demonio — con una ventana realizada la JVM
no sale aunque no quede un solo hilo BP. Por el camino viejo no se notaba porque `Gui.run()` sólo
vuelve al cerrar la ventana y el `DISPOSE_ON_CLOSE` ya la había destruido; fue `Gui.stop()` —o sea,
esta misma ficha— lo que dejó el agujero a la vista. Arreglo: `gui.shutdown()` al terminar la
ejecución. Un programa con `start()/stop()` ahora sale con **código 0** en las dos VMs, y con eso
la GUI entra en la red de paridad. Detalle y medida, en `#477`.

⚠️ **Y una trampa de método que me costó tres intentos**: el compilador resuelve los imports desde el
**`stdlibDir` de `BpVM.cfg`**, no desde el directorio de salida. Compilar `Gui.bp` a un temporal y
creer que el sample lo usa es un error — decía «el módulo Gui no expone 'start'» mientras el `.mod`
nuevo estaba al lado sin instalar. **Para probar un cambio en la stdlib hay que instalar el `.mod`.**


#### 🧩 L1 — lenguaje y compilador

Agrupa lo que ya está fichado y suelto por el registro. **No duplica: agrupa** — el texto
de cada una sigue en su sitio.

- ✅ ~~**La pasada de INTERFAZ no resuelve `Core` implícito**~~ — **CERRADA el 30-ago**
  (`#458`), y no como estaba previsto: en vez de enseñar a esa pasada a inyectar el
  `import`, se quitó el implícito y la norma pasó a ser explícita. Con ella cayó también
  la mitad que nadie había visto: la pasada de interfaz **tiraba miembros en silencio**
  cuando un tipo no resolvía.
- ~~**La sustitución por LSP entre interfaces de módulo**~~ — 🧊 **DEJA DE SER UN BUG el
  30-ago**: las interfaces de módulo se retiran del lenguaje (`#460`), así que era la
  prueba de una función que ya no existe. No se arregla: se va con ella.
- **`Object` = comodín por referencia** — decidido y diseñado.
- **Liberación de recursos**: destructor `~Clase()` + `var owner` + `FREE_REF`.
- **Ficheros como CLASE** — decidido: dos clases y la segunda hereda.
- **`Map`**: objetos internos para claves **y** valores + `add` sobrecargado *(decidido el
  23-ago; ver arriba)*.
- ✅ **`Math`, ampliar — HECHO el 30-ago**: entran `clamp`, `wrap`, `hypot` y `remap`
  (ids 228-231, al final del enum porque el id es `ordinal()`). Oráculo en
  `samples/MathRango.bp`: **29 líneas byte-idénticas en las dos VMs**, con las esquinas
  fijadas a propósito (rango del revés, `hi` excluido en `wrap`, extrapolación de
  `remap`, `hypot(3e18,4e18)`). `hypot` NO llama al de libm ni al `Math.hypot()` de
  Java —son algoritmos distintos y podrían separarse—: las dos VMs hacen
  `sqrt(x*x+y*y)` en `double`, que con entradas f32 no puede desbordar.
  ⚠️ Y los mensajes de error **no llevan números en coma flotante**: un `%g` de C y un
  `String.valueOf(double)` de Java no dan la misma cadena, y ese texto sale por stdout.
  ℹ️ De la lista original eran cinco: **`atan2` ya estaba hecha** y esta ficha la pidió
  hasta el 30-ago (existe en `Math.bp`, la registra `Intrinsics.java` y la implementan las
  dos VMs — comprobado corriendo `samples/mathtest.bp`, 17 líneas byte-idénticas).

- 🟡 **Lo que queda de `Math` NO es matemática, es el COMPILADOR.** Los dos deseos que
  siguen abiertos son el mismo problema:
  - *«`factorial` sobrecargada y f64»*. Observación de Eduardo: **nuestro factorial es
    `Γ(x+1)`**, y `gamma` ya existe — comprobado con `samples/GammaFact.bp`, que fija
    `gamma(n+1) == factorial(n)` para n=0..12 en las dos VMs, y sigue contestando donde el
    entero lanza (`13! = 6227020800`, `20! = 2.432902E18`). O sea que **no hace falta
    builtin nuevo: el cálculo ya está**.
  - `sign(integer)` y `signF(float)`: dos nombres para una idea, herencia de cuando no
    había sobrecarga.
  🔒 **Y lo que bloquea a los dos es lo mismo: una intrínseca NO se puede sobrecargar.**
  `Intrinsics.REGISTRY` es un `Map` con clave el nombre cualificado y `register()` revienta
  si se repite; y el call-site (`MivmEmitter.java:3924`) compone la clave con **`fs.name`,
  el nombre PELADO** — no el mangleado de `H5.a`. Así que las dos firmas caerían en la
  misma entrada. Arreglarlo es la tarea de verdad, y de paso desbloquea las dos.
- **`#19`** array fijo LOCAL: que sea inline de verdad · **`#396`** módulo `Time`.

#### ✅ E1 — ~~el IDE y el wire~~ (**CERRADO el 6-sep**, 7/7)

- ✅ ~~**`#412`** — `run miModulo <arg>`, con el argumento siempre en el heap~~ — **HECHO el 6-sep**.
  La declaración es la que manda, como pidió Eduardo:
  `public function Main(arg: string := "mi valor por defecto")`.
  Builtin **`__runArg` (id 232)** en las dos VMs: recibe el defecto que declara el fuente y devuelve
  el argumento de ejecución si lo hay, y si no **una copia del defecto, alojada en el heap igual**.
  La idea de Eduardo es la que lo simplifica —el caso sin argumento pasa a ser el caso con `""`
  también en el heap—, y **no era sólo elegancia**: hoy los dos casos producían **tipos de cadena
  distintos** (literal de la zona de datos vs cadena del heap), las dos formas que `#389` tuvo que
  reconocer en el `CHECKCAST`. Con esto la asimetría no llega a existir.

  **Camino completo**: emisor (`__startup`) · las dos VMs · los **dos CLI** (antes rechazaban el
  segundo posicional, así que la receta de paridad no podía ni ejercitar la feature) · el campo
  **escalar `arg`** del `RUN` en los **cuatro** implementadores · y el IDE, **por la línea de
  comandos**: `run <fichero> <argumento>`, con comillas o sin ellas (`partirDos`, ya probado en
  `#437`).

  📌 **El autorun NO pasa ninguno, a propósito** (Eduardo): *«el parámetro es para poder testear el
  programa con diferentes opciones; una vez probado, configuramos el valor del argumento por defecto
  y la misión del auto es simplemente que arranque al arrancar el micro»*. O sea: **el valor por
  defecto ES la configuración de despliegue**, y por eso vive en el fuente y no en `auto.txt`.

  ⚠️ **Y de paso, una promesa del doc que NUNCA se pudo cumplir**: `BPVM_WIRE_PROTOCOL.md`
  documentaba `args:[]` desde siempre, **ninguna línea de código lo parseó jamás**, y no era un
  olvido — el mini-parser de las placas **no sabe leer arrays anidados** (`json_min.h:14`), así que
  implementarlo obligaría a tocar el parser de las cinco familias. El diseño eligió un `arg`
  **escalar**, que es la única barata. Doc corregido.

  **Verificado**: paridad byte-idéntica en los dos casos (host), por el wire contra el simulador, y
  **en placa en la Pico 2** — `arg = [ desde el wire! ]`. Y por el camino del IDE, `exit 0 (OK) en
  17 ms`. Regresión: `io_smoke` `[status=OK]`, paridad 5/5, y el `RESET` de `#452` sigue bien (toca
  los mismos REPL).
- ✅ ~~**NO copiar dependencias que el dispositivo YA TIENE** — y que lo diga él.~~ **HECHO el 2-sep** (`#466`, paso 3): el IDE pregunta por cada dependencia con `STAT` por nombre y sólo sube lo que falta o es más viejo.
- ✅ ~~**Al fallar una dependencia, decir DE DÓNDE salió el módulo**, por CRC~~ *(idea de Eduardo)*
  — **HECHO el 6-sep**, y salió **mucho más pequeño** de lo que yo había estimado (dije «sesión
  larga, y el mensaje no cabe en `char[160]`»): **las dos mitades ya estaban construidas**.
  - El **device** ya contesta dónde tiene un módulo: `STAT{name, base, crc:true}` lo resuelve con
    **el mismo resolvedor que usa el RUN** y devuelve la ruta elegida. Su comentario
    (`bpvm_repl.c:150`) dice por qué se hizo así: *«el IDE no lleva un gemelo del orden de búsqueda,
    que es como se desincronizó `#463`»*.
  - El **IDE** ya lo pregunta por nombre en cada despliegue (`statModule`, `PicoExplorer.java:1070`)
    y ya recibe `path`, `crc` y `magic`… **y no los enseñaba**.
  Ahora el mensaje de cada dependencia lleva **dónde la tiene la placa y con qué CRC**, y —lo que de
  verdad contesta la pregunta— **avisa cuando esa ruta NO es donde este proyecto la pondría**:
  «⚠ OJO: no es donde este proyecto lo pondría (/app/proj/X.mod) — la placa carga esa OTRA copia».
  Comprobado contra el simulador: con `Core.mod` en `/lib`, el device contesta `/lib/Core.mod`
  aunque el IDE lo pondría en `/app/miproyecto/`, y el aviso salta.
  📌 **Y llega ANTES del fallo**, no como post-mortem: se ve en cada Run, que es cuando la copia
  rancia todavía no ha mordido.
  ⚠️ **Corrección de Eduardo que enderezó esto**: yo iba a que el IDE enumerase los candidatos
  (proyecto → /app → /lib → /sys). *«El IDE no tiene que determinar el orden de búsqueda, le
  pregunta al micro y éste se encarga.»* Con mi versión, el orden habría quedado en **dos sitios** —
  la enfermedad de `#470`, y justo lo que `#463` ya costó una vez.
- ✅ ~~**El verbo `RESET` no llega con un RUN vivo**~~ *(era `#452`)* — **ARREGLADO el 6-sep
  (`ee250df6`) y VERIFICADO EN PLACA en la Discovery U5G9J**, con la placa contando su propio
  reinicio:

  ```
  control: el programa esta VIVO (2 OUTPUT, ningun EXITED)
  [+  51 ms] {"type":"EXITED","session":2,"status":"KILLED","exitCode":130,"elapsedMs":3038}
  [+ 102 ms] {"type":"RESET_REPLY","id":4}
  [+2189 ms] === bpvm-stm32 REPL (wire v1) listo ===
  [+2189 ms] reset cause: software
  ```

  Y los dos controles en la misma placa: `KILL` sigue dando `KILL_REPLY`+`EXITED` en 51 ms, y
  `STATE` **sigue dando BUSY** — la lista blanca no se abrió de más.

  ✅ **VERIFICADO EN LAS TRES ARQUITECTURAS**, y con eso queda cubierto **todo el código tocado**:
  los cinco firmwares son **tres ficheros de REPL**, y hay una placa de cada uno.

  | placa | `EXITED KILLED` | `RESET_REPLY` | la prueba del reinicio |
  |---|---|---|---|
  | **Discovery U5G9J** (STM32) | +51 ms | +102 ms | banner + **`reset cause: software`** a +2,19 s |
  | **Pico 2** (RP2350) | +51 ms | +102 ms | el **puerto USB desaparece** a +204 ms y **vuelve a 1,8 s** |
  | **ESP32-C6** | +51 ms | +102 ms | log de arranque del IDF y **vuelve a 16,6 s** |

  Las tres con su control de vida delante (OUTPUT llegando, ningún EXITED) y con `KILL` y `STATE`
  comprobados en la misma sesión. Cubren por herencia al Nucleo (comparte `stm32_repl.c`) y al
  S3/C3/P4 (comparten `esp32/common/repl_esp32.c`).

  🟡 **Cabo suelto menor, visto en las tres**: el `EXITED` de un RESET dice
  `errorMessage: "terminado por KILL"`. Es verdad a medias —muere por el camino del KILL— pero lo
  que el usuario pidió fue un RESET. Una línea, cuando se toque otra cosa por ahí. Rama `RESET` gemela de la de `KILL` en los
  cuatro polls (pico, `esp32/common`, stm32, sim): marcar el id, devolver 1 para que el RUN muera
  por el camino de siempre, y reiniciar DESPUÉS desde la tarea del REPL. El STM32 tenía el RESET
  en línea en el despachador → extraído a `stm32_hacer_reset()`. Verificado en el sim con control:
  con el programa VIVO (352 OUTPUT, ningún EXITED) sale `EXITED KILLED` → `RESET_REPLY` → cierre,
  en 0,4 ms; y `STATE` sigue dando BUSY, lo que prueba que la lista blanca no se abrió de más.
  🔴 **Y el segundo defecto, que no estaba escrito y es peor porque no hacía ruido: el IDE se
  tragaba el BUSY.** `BpvmClient.reset()` capturaba TODA `IOException` —y `WireError` la extiende—
  y la mandaba a un `diag()` que sin sink no va a ninguna parte; la consola imprimía «reset
  enviado» sobre una placa que seguía corriendo, y encima el backend ya había soltado el cliente:
  **sin reset Y sin conexión**. Ahora una reply de error se propaga (*una reply no es una reply
  perdida*) y el cliente se conserva. La maquinaria buena ya existía: `runAsync` tiene desde `#256`
  el mensaje accionable «placa ocupada — 'kill' para abortarlo». Comprobado en el **artefacto**: el
  fat-jar del 3-sep vuelve sin excepción, el recién construido lanza `WireError code=BUSY`.
  📌 El arnés está en `bpgenvm-c/tools/reset_smoke.py`, **y con control**: la primera versión salió
  VERDE con el bug dentro porque mandaba el RESET «un segundo después» y contra el sim los bancos
  terminan en 80 ms — o sea con la VM en reposo. Ahora comprueba que el programa sigue vivo antes
  de mandar el verbo y declara la prueba INVÁLIDA si no. Con `samples/benchmarks/VivoLargo.bp`.
  ⚠️ **El enunciado original era FALSO**,
  comprobado el 5-sep: RESET **sí llega** — la placa lo lee, lo parsea y lo **rechaza a propósito**
  con `ERROR BUSY`, porque el poll que atiende el cable durante un RUN tiene **lista blanca**
  (KILL, HELLO). Reproducido en el simulador. La diferencia con KILL, en una línea: KILL tiene rama
  en el poll y **delega** el trabajo (`repl_v1.c:931`); RESET sólo existe en el despachador de
  reposo (`:1533`), al que no se vuelve hasta que `bpvm_run()` retorna.
- ✅ ~~**PROBAR BASES DE DATOS SIN PLACA**~~ — **HECHO el 6-sep**: `make sim SQLITE=1`, y un
  programa BasicPlus consulta una BD **en el simulador**, por el wire, `status: OK` en 181 ms.

  **El criterio fue de Eduardo**, y conviene que quede: *«¿es útil para el programador? Si la
  respuesta es sí, se hace. ¿Es útil poder probar la ejecución de un programa que consulta una BD en
  el simulador? La respuesta es sí, así que lo hacemos.»*

  📐 **Por qué ENLAZADO y no por pack nativo, que era mi plan A.** El pack de una placa se reubica
  con `NpackReloc`, que es **ELF32 de arriba abajo** — `Elf32.java:169` lo dice sin ambigüedad:
  *«solo ELF32 soportado»*—, y x86-64 es **ELF64**. Traer eso al PC es **un lector nuevo más el
  juego de reubicaciones de x86-64**: un proyecto, no una tarde. Y además el `.mdn` se carga
  **zero-copy** (`mdn_loader.c:173`: *«el código nativo ya está en RAM (ejecutable)»*), suposición
  que es cierta en un micro y **falsa en un PC** (DEP/NX).

  🔑 **Y no cambia nada de lo que el programador ve**, que es lo que lo hace legítimo: `SQLite.mod`
  y `Orm.mod` **viajan igual que a una placa** —son bytecode, con **2 bytes** de sección nativa— y
  sus 16 `native` se casan **POR NOMBRE** (`bpvm_aot_register_by_name`), venga el thunk de un `.npk`
  reubicado o de aquí. El `.c` del puente es el **mismo** que genera `AotMain` para las placas.

  ⚠️ **Lo que hay que saber**: así el simulador tiene SQLite **siempre** y una placa sólo si le
  grabas el pack. Un programa que va aquí y no allí falla **ruidosamente** («falta el código nativo
  del pack 'SQLI'»), que es el propio cuerpo de aviso que el compilador pone a cada `native`
  (`Parser.java:711`) — no en silencio.

  📌 **Y arregló de paso el sample que motivó `#412`.** `SqlDemo.bp` decía en su cabecera *«POR QUÉ
  EL CAMINO NO ES UN ARGUMENTO: porque HOY NO SE PUEDE (tarea `#386`)»* —cierto hasta esta misma
  mañana— y además **llamaba a `/sd/medidas.db` siendo el demo de FLASH**, contradiciendo su propia
  tabla. Ahora el camino es el argumento, con el defecto en la declaración: `run SqlDemo` usa
  `medidas.db` (flash) y `run SqlDemo <ruta>` la que digas. *(La BD puede estar en flash o en la
  tarjeta, y hay un demo para cada cosa — `SqlDemoSd` es el de la SD.)*

  ⏭️ Queda **el trozo viejo de esta ficha**, que sigue siendo verdad: el
  ciclo entero YA corre en el PC (`make test-sqldemo` → `sqldemo.exe SqlDemo.mod`, con `[status=OK]`).
  Lo que falta no es la pieza que decía la ficha, es que ese camino vive en un binario de pruebas y
  no en los que usa la gente.
- ✅ ~~**El árbol de ficheros por COLOR** según el tipo~~ — **HECHO el 26-ago** (`9cc33ee6` el color
  y `fb66e579` la corrección de paleta que pidió Eduardo en pantalla): `PicoExplorer.java:1296`,
  cinco familias semánticas con el **rojo reservado** a propósito. Se quedó sin tachar en esta lista
  hasta el 5-sep. *(Queda el «paso 2», el rojo para ficheros con problema, que la propia ficha aparca.)*
- ✅ ~~**Enseñar el tiempo que la placa ya manda y nadie imprime.**~~ — **HECHO el 6-sep**
  (`ab46384f` + `0fbe6cb2`). El IDE muestra ahora `exit 0 (OK) en 94 ms`; a partir de 10 s, en
  segundos; y si el peer **no manda** el campo, **no escribe nada** — no se inventa un «0 ms», que
  sería una medida falsa. Un solo punto (`AbstractBpvmBackend`) cubre todos los sitios que lo pintan.
  Verificado en el **artefacto** (el fat-jar): los cuatro casos contra un servidor de pega, y de
  punta a punta contra el simulador con un módulo real.
  📐 **Y de paso se arregló el significado**: miVM arrancaba su reloj al RECIBIR el RUN y las cinco
  placas con el módulo ya cargado. Ahora las seis miden **la ejecución**. ⚠️ Pero la cifra que yo
  anuncié era 30 veces mayor de lo que resultó: instrumentando el método, el sesgo son **7 ms**.
  Los ~230 ms que medí primero eran **la latencia de mi propio arnés** contestando al breakpoint de
  entrada (sondeaba cada 200 ms); y el 42× entre las dos VMs con `Bench.mod` (3,8 s en Java contra
  90 ms en C) **no es sesgo, es velocidad de interpretación**. Lo que separó una cosa de otra fue el
  cronómetro DEL PROGRAMA, que no depende ni del arnés ni del campo medido.
  ⚠️ Trampa cara del camino, anotada en `rebuild-bpide-fatjar-tras-frontend`: `mvn install` en
  `lexer-java` **no re-empaqueta** si sus fuentes no cambiaron, su copia sombreada de `edu.bpgenvm`
  se queda rancia, gana en el classpath, y el `NoSuchMethodError` **se lo traga el hilo lector**: el
  IDE no falla, se **cuelga**. Hace falta `clean install` del frontend.

- ⛔️ *(el enunciado viejo de este punto, para el que busque el porqué)* ⚠️ **El número que decía esta ficha
  caducó**: el `durationMs` del SAVE vale **0 siempre** — los tres `fs_save` son no-ops desde
  littlefs, comprobado en el ARTEFACTO (el `.map` de la Discovery: `.text.fs_save … 0x2`, dos bytes).
  El bueno es el **`elapsedMs` del `EXITED`**, que mandan **los cinco emisores** (las cuatro C, el
  sim y miVM) y que el IDE **no ha parseado nunca** (`git log -S elapsedMs -- BpIde/` sale vacío).
  ~15 líneas en tres ficheros. ⚠️ Decisión previa: **no miden lo mismo** — miVM arranca el
  cronómetro al asignar la sesión, **antes de cargar y enlazar** (`DebugServer.java:110`); la placa
  justo antes de `bpvm_run` (`repl_v1.c:1264`). Sin decidirlo, el PC parecerá siempre más lento.

📌 **E1 tiene CUATRO puntos vivos, no seis** (5-sep): dos ya estaban hechos y sin tachar.

#### 🔌 P1 — las placas nuevas: ESP32-C3 y ESP32-C6

**Encargo de Eduardo (23-ago).** Encaja con la prioridad ya escrita —[[prioridad-arm-riscv-s3-secundario]]:
ARM y RISC-V primero— porque las dos son **RISC-V**.

✅ **Ya hay una estimación hecha, y es buena noticia**: la ficha *«¿cuánto cuesta una familia
nueva si antes unificamos?»* la contesta con el precedente del P4 — el IDF es el mismo para
toda la familia ESP32, así que casi todo lo hecho sirve y **lo nuevo es el arranque y, sobre
todo, trabajo de pruebas**.
⏭️ **Y por eso este hito va DESPUÉS de U1–U5**, no antes: cada sistema sin unificar es una
copia más que escribir para cada micro nuevo. Es el argumento entero de V6, aplicado.

---

### 🧪 P1.C3 — MIGRACIÓN DEL ESP32-C3 (abierto 31-ago)

**Encargo de Eduardo**: *«dijimos de no empezar la imagen del C3 hasta terminar la
unificación, pero como es un proceso largo me gustaría adelantar un poco. Sería más bien un
ensayo, no hace falta que sea una imagen completa. Lo que me interesa más que nada es ver
cuál es el coste de desarrollo.»*

#### ✅ P1.C3.1 — EL ENSAYO: compila y enlaza para `esp32c3` (31-ago)

**Resultado: `bpvm_esp32c3.bin`, 431 KB, enlaza limpio.** No se ha probado en placa.

📐 **El coste, medido y no estimado.** Todo el port vive en `bpgenvm-c/esp32c3/`, 5 ficheros:

| fichero | líneas | nuevas de verdad |
|---|---|---|
| `main/main.c` | 176 | **13** (el banner y el tamaño del bloque de la VM) |
| `main/CMakeLists.txt` | 108 | **5** de 66 entradas — el resto es la lista del S3 con un prefijo de ruta |
| `CMakeLists.txt` · `sdkconfig.defaults` · `partitions.csv` | 57 | copia literal, 3 sustituciones |

**≈20 líneas escritas de verdad.** La cintura ESP32 entera se toma del S3 por ruta relativa.

🔑 **Y EL COSTE REAL NO ESTABA AHÍ.** Estaba en **tres suposiciones del S3 metidas en el
código compartido**, que el C3 destapó una a una — ninguna se habría visto sin intentar el
port:

| lo que se daba por hecho | el C3 | cuándo saltó |
|---|---|---|
| 3 UART y 3 SPI | tiene **2 y 2** | al compilar |
| *«es RISC-V»* ⇒ tiene API de caché | es RISC-V y **no trae `esp_cache.h`** | al compilar |
| todos los ESP32 tienen PCNT | **no lo tiene** | al **enlazar** |

Las tres arregladas preguntando al silicio (`SOC_UART_NUM`, `SOC_SPI_PERIPH_NUM`,
`SOC_PCNT_SUPPORTED`, `__has_include`) en vez de dar por hecho la familia — **y mejoran a
todas las placas**: el S3 y el P4 compilan exactamente igual que antes (ver `#465`).

📌 La segunda es la que más enseña: *«es RISC-V»* funcionaba como atajo de *«soporta cargar
un `.mdn` en RAM ejecutable»* **mientras el único RISC-V fuera el P4**. Una familia nueva no
sólo se añade: **audita el código compartido**.

💾 **Y cabe, con margen medido:**

| | C3 | (S3, de referencia) |
|---|---|---|
| DRAM total | 321.296 B | ~319.632 B libres al arrancar |
| estáticos | 108.538 B (33,8 %) | ~101 KB |
| **libre para el runtime** | **212.758 B** | — |

Con **96 KB** para la VM quedan ~116 KB, por encima de los **86 KB** que `#336` midió que
consume el sistema en marcha. **Los 160 KB del S3 NO cabrían** (dejarían 52). ⚠️ Ese 96 está
**sin medir en placa** y así lo dice el propio `main.c`: el definitivo sale de repetir la
medida de `#336` aquí.

#### ✅ P1.C3.2 — REESTRUCTURAR `esp32/`: separar la familia de la placa (HECHO 31-ago)

**Propuesta de Eduardo (31-ago)**: *«ya tenemos una carpeta para el S3 y otra para el P4,
habría que crear una tercera para el C3. No sé si iría bien renombrar `ESP32` a `ESP32S3` y
crear una carpeta `ESP32` que contenga a las otras 3. Entonces lo común de la familia en
`ESP32` y lo particular de cada micro en su carpeta.»*

✅ **El reparto YA EXISTE en la práctica, sólo que sin decirlo** — contado:

| | ficheros |
|---|---|
| **común de la familia** (lo usan P4 y C3 por `../../esp32/main/`) | **8**: `repl_esp32` `board_mgr` `platform` `fs_lfs` `gpio` `wire_v1` `log` `esp32_mods` |
| común de los que no llevan AOT propio | `aot_funcs_stub.c` (S3 y C3; el P4 tiene el suyo) |
| **propio de la placa** | **`main.c`. Y sólo eso.** |

**Nueve de diez `.c` son de familia.** Hoy `esp32/` hace dos papeles a la vez, y eso es lo
que la propuesta separa:

```
esp32/
  common/   ← los 9 + las cabeceras
  s3/       ← main.c + CMakeLists + sdkconfig + partitions
  p4/  c3/  ← igual
```

📊 **Coste**: 53 referencias a `esp32/main` en 12 ficheros, pero **sólo 3 son funcionales**
—los dos `CMakeLists` y `regen_esp32_mods.sh`—; el resto son comentarios y docs (algunas en
snapshots inmutables, que no se tocan).

⚠️ **Va en su propio paso y con las tres imágenes reconstruidas**: mueve 10 ficheros y toca
el build de las tres familias a la vez. No mezclarlo con nada.

##### ✅ `P1.C3.2` — la familia ESP32 tiene un común de verdad (31-ago)

La idea de Eduardo era *«renombrar `esp32/` a `esp32s3/` y crear un `esp32/` que contenga a
las otras 3: lo común de la familia en `esp32/` y lo particular de cada micro en su carpeta»*.
**El censo cambió el tamaño del trabajo**, así que se hizo el mismo reparto con otra forma.

### Lo que el censo dijo antes de tocar nada

| pregunta | medida |
|---|---|
| `.c` de `esp32/main/` que usan **los tres** chips | **9 de 10** |
| el único no compartido | `main.c` |
| `main.c` del C3 **vs** el del S3 | copia con **17 líneas** distintas, en **3 sitios** |
| `main.c` del P4 vs el del S3 | 628 vs 167 líneas — **otro fichero** |
| el FS (`fs_lfs_esp32.c`) | busca la partición **por nombre** (`bpdata`): cero código por chip |
| rutas relativas si se mueven los proyectos | **193** en tres `CMakeLists` |

Dos conclusiones. Una: `esp32/main/` **ya era el común** en todo menos en el nombre — no había
que construirlo, había que decirlo. Otra: mover los tres proyectos costaba 193 rutas relativas
cuyo fallo típico no es «no compila» sino «resuelve a otra cosa», que es el peor.

### El reparto que se hizo

El ESP-IDF exige que cada proyecto tenga su componente `main/`. Así que se dejó el componente
donde estaba y salieron las **fuentes**:

```
esp32/
  common/          ← los 9 .c de la familia + main.c + sus .h   (era esp32/main/)
  main/
    CMakeLists.txt ← el componente `main` del proyecto S3
    chip_cfg.h     ← LO DEL S3: nombre + los dos tamaños del heap
esp32c3/main/
    chip_cfg.h     ← LO DEL C3
esp32p4/           ← su main.c propio (es otro arranque de verdad), fuentes del común
```

**Ni una profundidad cambia**: sólo `esp32/main/X.c` → `esp32/common/X.c`, unas 30 líneas en
tres `CMakeLists` en vez de 193.

### Lo que de verdad se arregló: el `main.c` duplicado

176 líneas copiadas que se diferenciaban en **dos números y un nombre**. Es la trampa de
[[arreglo-que-no-viaja-entre-familias]] — y **ya había mordido dos veces esta semana**: el `if`
sin llaves (`#464`) y las puertas por capacidad del silicio (`#465`) se arreglaron en un
`main.c` y no en el otro. Ahora hay uno.

Lo particular baja a `chip_cfg.h`, y el criterio de qué entra ahí es estrecho: **sólo lo que no
se puede preguntar en marcha**. El tamaño del heap de la VM lo es, y por una razón concreta —
`vm_buffer_init()` corre **antes** que `board_mgr_esp32_boot()` (`layer_app` comprueba
`s_vm_buffer`), o sea antes de que exista el ENV. Todo lo demás sigue yendo al ENV
([[config-de-placa-en-el-env]]).

📌 **La resolución del `#include` es lo que hace que esto sea seguro.** Como en `esp32/common/`
**no** hay ningún `chip_cfg.h`, el `#include "chip_cfg.h"` de `main.c` no puede caer en el del
vecino por accidente: no encuentra nada al lado del fichero que incluye y cae en el `-I` del
proyecto que se está compilando. Si se pusiera un `chip_cfg.h` en `common/`, ganaría siempre y
el fallo sería **silencioso**.

### ✅ Verificado — en el artefacto, no en el log

Las tres imágenes compilan, y **el control es el nombre del chip**: el mismo `main.c` produce

```
  S3   -> BasicPlus VM en ESP32-S3
  C3   -> BasicPlus VM en ESP32-C3
```

leído con `strings` **de los `.bin`**, que es la única forma de saber que cada proyecto cogió su
`chip_cfg.h`. Y las tres conservan el tamaño exacto que tenían antes del cambio (440000 /
431472 / 1294000), que es lo que se espera de un movimiento sin cambio de comportamiento.

⏭️ Queda `P1.C3.3`: **medir** el heap del C3 en placa y sustituir el 96 KB de ensayo. El fichero
lo dice de sí mismo, en mayúsculas.


#### 🎯 La conclusión de Eduardo al ver el ensayo (31-ago)

*«Me vale como prueba. Realmente las ESP32 escalan bien y eso es bueno. Detrás de la C3 y
la C6, en un futuro vendrá la S31 —que es como una actualización de la S3 pero con hardware
más moderno y RISC-V—. Lo que necesitamos es terminar la unificación de las comunicaciones,
y la migración de C3 y C6 nos sale casi gratis.»*

📊 **Y el ensayo lo respalda con un número.** De dónde salen las 63 fuentes del firmware C3:

| origen | fuentes | |
|---|---|---|
| **común de las 5 placas** (`src/`) | **49** | **77 %** |
| común de la familia ESP32 (`esp32/main/`) | 10 | 15 % |
| terceros (littlefs) | 3 | 4 % |
| **propio del C3** | **1** | **1 %** |

**Una fuente de 63 es del C3.** Y las 10 de familia incluyen `repl_esp32.c` — o sea que el
ensayo **ya se benefició de lo que `U3` lleva hecho**: usa el REPL común. Lo que queda de
`U3` (`LIST` y el desglose por raíz) es literalmente lo que separa ese 15 % de bajar más.

🔗 **Por eso `P1` va detrás de la unificación y no al revés**, y ahora está medido en vez de
argumentado: *cada sistema sin unificar es una copia más por micro*. La S31 —RISC-V, como el
C3 y el C6— entra por el mismo camino.

#### ✅ `P1.C3.3` (1/2) — EN PLACA: arranca, y la RAM no era la que parecía (31-ago)

**El C3 arranca y la VM está viva.** Placa: chip `v0.4`, mononúcleo a 160 MHz, **flash embebida
de 4 MB (XMC)**, MAC `e0:72:a1:21:40:78`, un solo puerto USB (`VID_303A/PID_1001` = el
USB-Serial-JTAG nativo, sin puente UART).

### Tres cosas que había que arreglar ANTES de poder medir

**1. La tabla de particiones declaraba 16 MB** — copia literal de la del S3, cuyo módulo de
referencia los tiene. `esptool flash-id` dice **4 MB**: la tabla de 16 se sale de la flash y no
arranca. Bajada al suelo (4 MB) **con los mismos offsets**, así que ensanchar el día que haya
una placa mayor no mueve el env ni el volumen littlefs. Al revés sí funciona —declarar 4 en un
chip de 16 usa menos pero va—, así que el suelo vale para cualquier C3.

📌 **Medido, no supuesto.** La pregunta «¿cuánta flash tiene?» tiene respuesta de una línea
(`esptool flash-id`) y se contestó antes de grabar. Es el mismo modo de fallo que cazó Eduardo
el 27-jul en el S3, al revés: la constante mentía y el chip no.

**2. Los pines del wire eran los del S3, y en el C3 no existen.** Esto:

```c
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define WIRE_UART_TX_PIN 37   /* P4 */
#else
#define WIRE_UART_TX_PIN 43   /* ← «el resto» = el S3 DevKitC */
#endif
```

El ESP32-C3 tiene **GPIO0..21**: el 43 y el 44 **no existen en ese silicio**. `uart_set_pin`
devuelve error y **no aborta**, así que el wire se quedaba con RX/TX muertos **en silencio** —
el peor modo de fallo. Es `#465` con otra cara: **dar por hecha la familia en vez de preguntar
al silicio**. Arreglado al revés de como estaba: por defecto los pines IO_MUX que el SoC asigna
a UART0 (los que ya usan la ROM y el bootloader, o sea los de verdad), y se REENRUTA sólo donde
la **placa** cablea el bridge a otro sitio — que es dato de placa, no de familia, y por eso va
nombrada una a una.

**3. La foto de RAM del arranque no salía por consola.** `log_printf` escribe **sólo** al log de
flash, que además está apagado por defecto (`#423`): para ver la RAM había que conectar el IDE y
pedir un `LOG_DUMP`. El Pico lo dice en su banner *desde siempre* —lo señalaba el propio
comentario del código como algo que al ESP32 le faltaba—, así que la línea ahora también sale
por `printf`. Una asimetría hacia abajo menos, y sin ella este hito no se puede medir.

### 📏 El primer número — y no es el que se esperaba

```
[boot] vm: heap 96 KB reservado | DRAM interna libre 280076->181768 B (bloque mayor 139264->114688 B)
```

**Hay 280 KB libres y el bloque contiguo mayor es 139264 B (136 KB).** La DRAM del C3 sale
troceada en regiones —`heap_init` las lista: 157 KiB + 113 KiB de retención + 10 KiB + 7 KiB de
RTCRAM— y el heap de la VM tiene que ser **un bloque**.

🎯 **Conclusión que ningún cálculo sobre el total habría dado: los 160 KB del S3 NO CABEN en el
C3, aunque sobre RAM.** Habría caído al escalón de respaldo — y el respaldo del S3 (128 KB)
tampoco deja margen sano contra un techo de 136. Es literalmente el caso que el comentario de
`vm_buffer_init` predecía: *«se puede tener RAM de sobra y aun así no caber»*.

Los 96 KB del ensayo caben con holgura. Que sean los definitivos depende del segundo número.

### ⏭️ Lo que falta, y por qué está parado

El segundo número de `#336` es la **marca de agua** (`heap_caps_get_minimum_free_size`), que el
firmware registra **tras cada RUN** — o sea que hay que ejecutar programas. Y para subir un
programa hace falta el wire… que va por UART0, y **esta placa no tiene puerto para UART0**: su
único USB es el nativo, que la consola ya ocupa.

Además el arranque dice `boot: estado 0 (kernel) DEGRADADO: falta algun tamano (placa virgen)`:
sin env no hay particiones, sin particiones no hay FS y sin FS no hay stdlib. Aprovisionar es lo
primero que hace el IDE… por el wire.

**Es una decisión, no un bug** — y es justo el tipo de coste que el ensayo existía para
descubrir:

| opción | quién necesita un adaptador |
|---|---|
| **(a)** dejarlo como está | el **wire**: un USB-serie a GPIO20/21 + GND |
| **(b)** invertir el reparto: wire por USB-Serial-JTAG, consola por UART0 | la **consola** |
| (c) los dos por el USB-JTAG | nadie, pero se entremezclan → descartada |

La (b) es la que deja al **usuario** con un solo cable —que es quien importa— y deja el
adaptador para depurar, que es cosa nuestra. Pero cambia el reparto de las tres imágenes ESP32,
así que se decide antes de tocar.


#### ✅ `P1.C3.3` — CERRADO: el C3 ejecuta BasicPlus por su único cable (31-ago)

```
{"type":"HELLO_REPLY","serverName":"bpvm-esp32c3", ...}
fib(28) interp = 317811 in 11317 ms          ← por el USB-Serial-JTAG, sin adaptador
{"type":"EXITED","status":"OK","exitCode":0}
```

### El muro, y cómo lo tiró Eduardo

La placa tiene **un solo conector USB** y el reparto heredado del S3 —consola por el
USB-Serial-JTAG, wire por UART0— dejaba el wire en pines pelados. Lo desatascó un dato de la
**placa**, no del software:

> *«Tenemos un solo conector USB pero a cambio tenemos 2 pulsadores, uno para reset y otro para
> boot. Si queremos grabar la imagen hay que pulsar los 2 y soltar reset. En un arranque normal
> el USB es NUESTRO.»*

O sea: el USB no hace falta para grabar. Si está libre, lo ocupa **el wire**, que es lo que
necesita el usuario. La consola se va a UART0, para cuando haya que depurar.

Antes se descartó la alternativa que él propuso —multiplexar con DTR— **midiendo, no opinando**:
el C3 declara `SOC_USB_SERIAL_JTAG_SUPPORTED` y **no** `SOC_USB_OTG_SUPPORTED`, y enumerando los
~70 campos del periférico en los registros del C3 no hay **ninguno** de línea de control: ni
`dtr`, ni `rts`, ni `line_state`. Ese par lo consume el hardware para reset y boot — por eso un
pulso de RTS metía el chip en modo descarga. En un chip con USB-OTG (S3, P4) sí se podría, vía
TinyUSB; pero ésos tienen dos puertos y no lo necesitan.

### Lo que hubo que arreglar para llegar

| | |
|---|---|
| **Tabla de particiones de 16 MB** | copia literal de la del S3. `esptool flash-id` dice **4 MB**: no arranca. Bajada al suelo, **mismos offsets** (ensanchar no moverá el env ni el volumen) |
| **Pines del wire = los del S3** | el `#else` fijaba GPIO43/44 y el C3 tiene GPIO0..21: **no existen**. `uart_set_pin` da error y no aborta ⇒ RX/TX muertos EN SILENCIO. `#465` con otra cara. Ahora el defecto es «lo que el SoC asigne a UART0» y se reenruta sólo con **el nombre de la placa** |
| **Declaración duplicada** | `wire_v1_uart_init` estaba declarada en `wire_v1.h` **y** en `repl_esp32.h`. Nunca mordió porque decían lo mismo; al renombrar el hueco del transporte, `main.c` compiló contra la copia vieja. Es la mina que el propio `wire_v1.h` describe para su guarda |
| **El banner mentía** | `[boot] ... wire v1 = UART0` era cadena FIJA: lo decía igual con el cable por USB. Ahora lo dice el transporte, y **avisa si el driver no instaló** |
| **Consola secundaria** | ESP-IDF, con la primaria en UART, manda ADEMÁS una copia por el USB-Serial-JTAG. Texto por el mismo cable que el wire binario — se vio **inyectado entre las respuestas** |
| **Identidad de placa** | sin `c3_board_id.c` saludaba como `bpvm-esp32` y anunciaba GPIOs, ADC y SRAM del S3. El mismo bug que `U3.19` cazó en el P4. Se engancha por el `chip_cfg.h` de `P1.C3.2` |

### 📏 El número, medido

```
[boot] vm: heap 128 KB reservado | DRAM interna libre 280032->148956 B (bloque mayor 139264->114688 B)
       mem: DRAM interna libre 131396 B | MINIMO HISTORICO 131396 B
```

| | |
|---|---|
| bloque contiguo mayor (**el techo real**) | 139 264 B |
| consumo del sistema en marcha | **17 560 B** — una quinta parte de los 86 KB del S3 |
| margen con 128 KB tomados | 131 KB |

Y el reparto: `bpvm_stack_region_bytes` da 25 % a pilas **pero nunca menos de 64 KB**, así que
con 96 KB el suelo dejaba **32 KB de heap** y con 128 KB deja **64**. Doblar el heap sin tocar
los hilos, y sin acercarse al margen. La alternativa —bajar el suelo a 32 KB— daba el mismo heap
pero dejaba la placa en **8 hilos** (main 16 KB + 2 KB por hilo) y tocaba una regla de las cinco
plataformas.

### 🐛 Y un diagnóstico mío que era falso, con su arreglo

Al medir con 96 KB leí en el log:

```
[bpvm] throw: SIN CLASE RuntimeError exportada, no hay con que construir la excepcion: No space in heap
```

y concluí que el heap se había agotado. **Era falso**: es la **prefabricación** del OOM de
`#430` —la idea de Eduardo de fabricar la excepción cuando fabricarla es gratis—, que corre en
todo arranque y que en un módulo que no importa `Core` no se puede hacer. El código lo contempla
(`.v == 0 si ni esto se pudo`). Con 96 KB el `Bench` también terminaba bien.

📌 **El aviso sonaba EXACTAMENTE igual que un fallo real**: misma frase, mismo texto «No space in
heap», en el log de una placa. Llegué a cambiar el tamaño del bloque de la VM por eso. Arreglado:
la VM marca cuándo está prefabricando y el aviso lo dice —*«OOM sin prefabricar: este módulo no
exporta RuntimeError. No es un fallo»*— en el canal normal, no en el urgente. **Un aviso que no
distingue un no-evento de un fallo es peor que no tenerlo** — el reverso de
[[errores-si-silenciosos-no]].

*(El 128 KB se queda: su justificación no era ésa, sino que dobla el heap gratis.)*

✅ **Verificado**: `HELLO`/`INFO`/`DF`/`PUT`/`RUN`/`LOG_DUMP` contra la placa por el USB; `Bench`
dos veces con salida correcta; paridad **38 PASS / 0 FAIL / 0 SKIP**; `sim-smoke` **40/40**; y
las **seis** imágenes reconstruidas (Pico, S3, P4, C3, Nucleo, Discovery).

⏭️ Queda de `P1.C3`: `trytest.bp` + `JsonDemo` en placa, y el AOT (el C3 es RISC-V, así que el
`.mdn` del P4 debería servir de plantilla).


#### ✅ P1.C3.3 — lo que se pedia (hecho arriba)

- **Medir el bloque de la VM en placa** (repetir `#336` en el C3) y fijar el número.
- **Un `c3_board_id.c`** — hoy el ensayo usa el `s_default_board` del S3, así que se
  anunciaría como `bpvm-esp32`. Es exactamente el bug que `U3.19` cazó en el P4.
- **Verificar en placa**: `trytest.bp` + `JsonDemo`, que ya tienen salida conocida.

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

⚠️ **CORREGIDO POR LA MEDIDA EN PLACA (31-ago, `P1.C3.3` arriba).** Este párrafo razonaba sobre
la RAM **total**, y el límite real es el **bloque contiguo**: en el C3 hay 280 KB libres y el
mayor bloque son **136 KB**. O sea que en el C3 no cabe ni el número del S3 (160) **ni su
respaldo con margen sano** (128 contra un techo de 136). «El mecanismo de respaldo ya existe»
era cierto y **no bastaba**: habría arrancado con un aviso y un heap al borde en vez de con un
número elegido. Que eso se viera es exactamente para lo que existía el ensayo.

🔴 **CORRECCIÓN (29-ago): esa conclusión se queda corta, y ahora hay un número.** *Caber*
no es el problema; el problema es **cuántos objetos vivos** admite ese reparto. Con la
regla de #449 (la tabla de handles es el 12,5 % del heap, y sale de OTRA bolsa):

| | VM | heap | tope de handles |
|---|---|---|---|
| Pico 2 | 357 KB | 267 KB | **4284** |
| S3 · C6 · C3 | 160 KB | 96 KB | **1536** |

`synclisttest` necesita **3848** handles (medido en el host). En la Pico 2 cabe por poco;
en el S3, el C6 y el C3 **no cabe** — y no por el tamaño de los objetos, sino porque hay
96 KB de heap parados mientras la tabla, que vive en otra bolsa, se queda sin sitio.

✅ **RESUELTO ESA MISMA TARDE (#451), y esta correccion se queda por lo que ensena.** Por la
mañana escribí aquí que `U6` era *prerrequisito* de P1, con estos números como argumento.
Por la tarde se movió la tabla dentro del bloque de la VM y **el argumento dejó de valer**:
con 96 KB de heap la tabla llega a 4096 slots y `synclisttest` pasa. La tabla y el heap son
ya una sola bolsa, así que el tope proporcional —y con él estas dos filas— no existe.

📌 Lo que sigue en pie, y es más flojo de lo que escribí: `U6` sigue mereciendo la pena
porque el reparto vive replicado en cuatro puertos y cada familia nueva es otra copia. Pero
**no bloquea P1**: las placas nuevas ya no nacen rojas por esto. Conviene no dejar en las
fichas una urgencia que se apagó — la cifra que la justificaba está arreglada.

🔴 **LA TRAMPA, y conviene saberla antes de empezar: `riscv` NO ES UN TARGET, SON DOS.** El
catálogo (`NpackReloc.DESTINOS`) tiene una sola entrada RISC-V, `riscv32-esp-p4`, con
`-march=rv32imafc -mabi=ilp32f` — o sea **con FPU**. El C3 y el C6 son **rv32imac, sin
coma flotante**: su ABI es `ilp32`. Un `.mdn` del P4 **no vale** en un C6 ni al revés.
📌 Es exactamente la clase de fallo que costó media tarde el 25-ago con el ARM
softfp/hard — pero ya **no puede pasar callado**: el guardián de `MdnPack` compara la
float-ABI del `.o` y el firmware declara la suya (`bpvm_mdn_host_float_abi`). Fallaría con
mensaje. Lo que hay que hacer es dar de alta un destino nuevo y decidir su sufijo (hoy
`RISCV` a secas se quedaría ambiguo).

##### 📏 LA MEMORIA DE CADA CHIP, contra el suelo del FABRICANTE (29-ago)

Eduardo trajo la tabla de Espressif *«Memory Usage Comparison»* (memoria libre corriendo
ejemplos basicos). Es el suelo de referencia que faltaba: permite separar **lo que cuesta el
chip** de **lo que cuesta BasicPlus**, en vez de discutirlo.

```
proyecto VACIO en S3 (Espressif) ....... 392,0 KB libres
nuestra imagen (medido en placa) ....... 331,6 KB libres
                                         ---------------
lo que cuesta BasicPlus ................  60,4 KB
   libmain.a (nuestro codigo) ..........  43,1 KB   (medido con idf.py size-components)
   drivers, spi_flash, littlefs, fatfs ..  17,3 KB
```

60 KB para un runtime con FS, drivers y wire de depuracion. **No es anormal**, y ahora esta
medido contra el suelo del propio fabricante.

⏩ **Y la tabla corrige la premisa de este hito.** Aplicando esos 60,4 KB:

| chip | vacio (Espressif) | con BasicPlus |
|---|---|---|
| ESP32-**C3** | 332 KB | **~272 KB** |
| ESP32-S3 | 392 KB | ~332 KB |
| ESP32-**C6** | **474 KB** | **~414 KB** |
| ESP32-S31 | 478 KB | ~418 KB |
| ESP32-P4 | 608 KB | ~548 KB |

🔴 **El C6 tiene MAS memoria que el S3, no menos** — unos 82 KB mas, probablemente por ser
de un solo nucleo frente a los dos del S3. La frase de arriba *«son RISC-V pero tienen poca
memoria»* vale para el **C3**; el C6 esta entre las holgadas.

🔴 **Y el S3 es estrecho POR DECISION NUESTRA, no por el chip**: tiene 332 KB libres y le
damos 160 al bloque de la VM. Al mirar el mapa parecia que el IDF se comia la RAM; el
`size-components` dice que WiFi, BT, lwip y mbedTLS **ni se enlazan** (5-29 bytes cada uno).

El orden real de estrechez es **C3 → S3 → C6 → S31 → P4**.

⚠️ Dos avisos: la tabla es de ejemplos **vacios y sin optimizar** (un suelo, no una
promesa), y los 60,4 KB estan medidos en el S3 — en otro chip cambia el juego de drivers.

##### 🧭 PRIORIDAD (Eduardo, 29-ago)

*«A mi la S3 me importa relativamente, es el paso. El futuro son la P4, la C6 y la S31.»*

Encaja con [[prioridad-arm-riscv-s3-secundario]] y con la tabla de arriba: **las tres del
futuro son las tres mas holgadas** (~414, ~418 y ~548 KB), asi que el trabajo de memoria de
estos dias se hizo contra el caso peor y no contra el caso que viene. El C3 queda como la
placa que apretara de verdad.

⏭️ **Reparto del trabajo**: arranque y bring-up baratos (casi todo reúso del S3; el IDF
amortigua) · decidir cuánta RAM lleva la VM en el C3 · un destino AOT nuevo con sus flags ·
y **lo caro, como siempre, el banco**: dos placas × la batería.

### 🏁 `P1.C6` — EL ESP32-C6 SIN PANTALLA, HECHO (3-sep): el ecuador de V6

*«Cuando tengamos la ESP32-C6 yo creo que ya habremos llegado al ecuador de V6»* (Eduardo, 2-sep).
Placa en la mesa el 3-sep por la tarde; en una tarde, por el camino del C3.

##### ✅ `P1.C6.1` — compila: el proyecto son siete ficheros y UNA línea de cintura

`esptool` dijo quién es: ESP32-C6, 160 MHz un núcleo + LP, **4 MB de flash embebida**, USB-Serial-JTAG
nativo (COM3), Wi-Fi 6 / BLE / 802.15.4. El gemelo del C3 en lo que importa al firmware (un USB,
4 MB, RISC-V sin FPU: el gate del `.mdn` cae solo en RISC-V/softfp por `__riscv`), con más SRAM.
`esp32c6/` es el clon de `esp32c3/`: `CMakeLists.txt`, `main/CMakeLists.txt` (los mismos fuentes
comunes; `c6_board_id.c` en vez de `c3_`), `partitions.csv` (la de 4 MB, mismos offsets),
`sdkconfig.defaults` (wire por USB, consola a UART0 —GPIO16/17 aquí—, sin secundaria), `chip_cfg.h`
y `c6_board_id.[ch]` (31 GPIO, 6 LEDC, 7 ADC, 512 KB SRAM). Y su `.gitignore` como el del S3: el
`sdkconfig` generado no entra (el del C3 sí está trackeado: inconsistencia anotada).

🐛 **Lo único que no compiló, y es de la familia**: `gpio_esp32.c` guardaba `UART_NUM_2` con
`#if SOC_UART_NUM > 2` (`#465`, que ya había salido del C3). El C6 cuenta **tres** UART —dos HP y
la LP del núcleo de baja potencia— pero la tercera es `LP_UART_NUM_0`: la cuenta correcta es
`SOC_UART_HP_NUM` (S3 3, C3 2, C6 2). El silicio nuevo hizo verdadera una condición que los dos
anteriores no distinguían. Arreglado en la cintura común y **recompilados S3, C3 y P4**: verdes.

##### ✅ `P1.C6.2` — en placa: arranca, se aprovisiona por el wire y ejecuta

```
{"serverName":"bpvm-esp32c6", "boardName":"ESP32-C6", "gpioCount":31, "adcChannels":7,
 "flashBytes":4194304, "sramBytes":524288}
[   44] heap: libre 410988 | mayor 385024 | bloques: 6 libres, 42 usados | usado 17760
[   66] boot: estado 0 (kernel) DEGRADADO: falta algun tamano (placa virgen: proponer defaults)
```

Virgen, como el C3: `PART_DEFAULTS` propone 1523712 + 1523712 (la `bpdata` de 2976 KB a medias),
`PART_APPLY` con eso, `RESET` → `boot: estado 3 (app)`, FS de 1488 KB, **los 14 módulos
preinstalados** y `/lib` completo (el instalador nuevo de `U4.1`, en su primera placa nueva).
`Bench`: **`fib(28) interp = 317811 in 11823 ms`** — el C3 dio 11315: misma clase, 4 % más lento
(la línea «AOT» sin `.mdn` cae al intérprete, por eso el RUN tarda 23,6 s: dos veces fib(28)).

📌 Observación: al abrir el puerto justo tras un reset llega un trozo del log del bootloader del
IDF antes del `HELLO_REPLY` (el ROM y el 2nd stage escriben por el USB-JTAG antes de que
`app_main` lo entregue al wire). El C3 hace lo mismo y el IDE conecta igual (el handshake se
salta lo que no es JSON); anotado por si algún día un cliente estricto se queja.

##### ✅ `P1.C6.3` — las tres constantes, MEDIDAS (protocolo `U6.4`)

```
vm: DRAM interna libre 410988->279912 B (bloque mayor 385024->253952 B)    ← con 128 KB heredados
mem: ... MINIMO HISTORICO 262980 B   (LOG_CLEAR → RESET → PUT → RUN×2 → LIST, con log=1)
margen = 279912 − 262980 = 16932 B   (C3 17588, S3 26564: un núcleo, sin radio activa)
```

El bloque contiguo son **376 KB** (el C3 tenía 136: aquí el IDF deja la SRAM casi de una pieza),
así que el techo no aprieta y el objetivo lo pone el criterio de Eduardo: no vaciar el heap del
RTOS para devolvérselo después — y este silicio trae tres radios que pedirán su parte. **192 KB**
(heap 128 + pilas 64, el doble de heap que el C3) deja 184 KB contiguos y ~219 KB libres al
sistema; 256 aún dejaría 120 contiguos: es un número, no una obra. `chip_cfg.h`: objetivo 192,
margen 16932, suelo 64, con la medida al lado. `test_mem` fija el caso (**27/27**). Regrabado:

```
[   43] vm: 192 KB en SRAM interna (objetivo 192, techo 376 por el bloque contiguo)
[   43] vm: DRAM interna libre 410988->214376 B (bloque mayor 385024->188416 B) | margen 16932
INFO: vmHeapBytes 131072 / vmStackBytes 65536
```

Y la paridad: `MathRango` subido por el wire y ejecutado en el C6 → **29 líneas byte-idénticas al
host**, `EXITED OK` en 10 ms. El `log` del ENV, devuelto a 0.

📌 **Coste real de una familia nueva, medido**: siete ficheros pequeños, una línea en la cintura
común, tres números medidos con el protocolo de siempre, ~2 horas con la placa en la mesa. Es lo
que la unificación prometía (`P1` iba después de U1–U5 por esto), y **el ecuador de V6**. La
pantalla del C6 es `P2`.

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

#### 📺 P2 — pantallas SPI *(depende de P1)* — ✅ HECHA (4-sep-2026)

##### 📐 `P2.0` — la placa y la forma (3-sep, tarde): ESP32-C6-LCD-1.3, ST7789 por SPI, sin táctil

**La placa** (Waveshare ESP32-C6-LCD-1.3, la que tiene Eduardo): ESP32-C6FH4 (4 MB, sin PSRAM),
panel IPS 1,3" **240×240, ST7789V2, SPI de 4 hilos, sin táctil**, ranura microSD, LED RGB
(WS2812B), pulsadores RST y BOOT, cabecera de 12 pines (GP1/2/3 y GP12/13/16/17/20/23 fuera).
La wiki vieja está vacía y la nueva pone los pines en una imagen; **el esquemático** (su tabla
«PIN-OUT», leída con `pypdf` por coordenadas) los da sin ambigüedad:

| señal | GPIO | | señal | GPIO |
|---|---|---|---|---|
| LCD MOSI | 6 | | SD MISO | 5 |
| LCD SCLK | 7 | | SD CS | 4 |
| LCD CS | 14 | | SD MOSI/CLK | 6 / 7 (el mismo bus) |
| LCD DC | 15 | | LED WS2812B | 8 |
| LCD RES | 21 | | BOOT | 9 |
| LCD BL (MOSFET) | 22 | | consola UART0 | 16 / 17 |

**La forma**: el cuarto backend del contrato `bpvm_gui_disp_*` (host SDL, STM32 LTDC, P4 DSI),
y el primero por SPI — lo que abre la GUI a las placas chicas. `esp32c6/main/gui_display_st7789.c`
con el `esp_lcd` del IDF (ST7789 de serie: `esp_lcd_new_panel_io_spi` + `esp_lcd_new_panel_st7789`),
LVGL 9.2.2 vendorizada como componente (`EXTRA_COMPONENT_DIRS`, igual que el P4) y `src/gui.c`
tal cual. Sin framebuffer completo: dos buffers parciales de 24 líneas (2 × 11 520 B, RAM interna
con DMA), flush con `esp_lcd_panel_draw_bitmap` y `flush_ready` cuando termina el DMA; el RGB565
va big-endian por SPI (`lv_draw_sw_rgb565_swap` en el flush); bombeo como el P4 (`lv_timer_handler`
+ ≥1 tick, tope 10 ms, `#424`). Backlight por GPIO (PWM después); rotación por el MADCTL del
panel (los gaps de 90/180/270 por comprobar). Flags globales `BPVM_GUI`/`BPVM_LVGL`/`LV_CONF_PATH`
y `BPVM_BOARD_C6` → RGB565 en `lv_conf.h`.

**El presupuesto**: RAM — el pool de LVGL son 64 KB estáticos (`LV_MEM_SIZE`) + 23 KB de buffers
+ el DMA de `esp_lcd`, unos 100 KB de los ~219 que deja el bloque de 192 KB de la VM: cabe, y el
margen se vuelve a medir con la GUI activa (`medir_margen.ps1`; el planificador se protege solo).
Flash — la imagen sin GUI son 466 KB de una `factory` de 1 MB; LVGL con las fuentes de
`lv_conf.h` (12–48) pesa: si no cabe, la `factory` crece a 1,5 MB y `bpdata` se encoge (la placa
es nueva: reaprovisionar no cuesta nada).

**Cómo se comprueba**: compila; `GuiColorDemo` desde el IDE sobre el C6 (`Gui.mod` no va
embebido: lo sube el IDE, y con `#466` sólo la primera vez), y los ojos de Eduardo son el
oráculo de la pantalla; después `GuiEvLat`/`GuiEvSpike` para el ritmo, y la medida del margen.

##### ✅ `P2.1` — HECHA (3-sep noche → 4-sep): el primer backend SPI, visto en la pantalla y con sus constantes medidas

- `esp32c6/main/gui_display_st7789.c`: el contrato `bpvm_gui_disp_*` sobre `esp_lcd` (bus SPI2,
  panel ST7789 con los pines del esquemático, invert on, gap 0/0, backlight GPIO22), dos buffers
  parciales de 24 líneas en RAM interna con DMA, swap RGB565 en el flush, `flush_ready` por
  `on_color_trans_done`, bombeo como el P4, rotación por MADCTL (gaps por comprobar).
- Build: `EXTRA_COMPONENT_DIRS` → LVGL vendorizada; flags `BPVM_BOARD_C6`/`BPVM_GUI`/`BPVM_LVGL`/
  `LV_CONF_PATH`; `gui.c` y `esp_lcd`/`lvgl` en el `main`; `lv_conf.h` conoce al C6 (RGB565 y sin
  SDL — el primer build cayó por compilar el driver SDL del host, que la condición no apagaba).
- **Compila y enlaza**: la imagen mide **0x1142C0 = 1 131 200 B**, y la `factory` era de 1 MB →
  `partitions.csv` del C6 con `factory` de 1,5 MB (`bpenv` 0x190000, `bpdata` 0x198000 de 2464 KB).
  Con eso la placa vuelve a virgen (el ENV se mueve).

✅ **Al retomar (3-sep, noche), hasta el humo por el wire**: el build cabe (`0x1142C0`, un 28 %
libre en la `factory` de 1,5 MB); grabado; reaprovisionado (`PART_DEFAULTS` propone 1 261 568 +
1 261 568 sobre la `bpdata` de 2464 KB; FS de 1232 KB; 14 módulos preinstalados; `log=0`). El
arranque con LVGL dentro:

```
heap: libre 303164 | mayor 278528 | bloques: 6 libres, 42 usados     (sin LVGL: 410988 | 385024)
vm: 192 KB en SRAM interna (objetivo 192, techo 272 por el bloque contiguo)
vm: DRAM interna libre 303164->106552 B (bloque mayor 278528->81920 B) | margen 16932
```

LVGL se lleva ~108 KB de DRAM estática (pool de 64 KB + fuentes/tablas) y el bloque de 192 KB
sigue cabiendo; quedan 106 KB al sistema, de los que saldrán los 23 KB de buffers y el DMA:
**el margen hay que remedirlo con la GUI activa** (paso 6). `Gui.mod` (44 KB) y su dependencia
`Json.mod` (24 KB) —ninguna embebida— subidos por el wire con el PUT por trozos (`putstream.ps1`;
el primer RUN dijo `falta el modulo 'Json'`, que es lo que el IDE resuelve solo). Y el humo:

```
RUN /app/GuiColorDemo.mod → "== widgets ==" + el árbol (screen 480x320, 8 botones) → Gui.run()
KILL a los 12 s → EXITED OK; la placa sigue viva (uptime continuo)
```

O sea: `bpvm_gui_disp_init` (bus SPI, ST7789, LVGL, buffers) y el bombeo corren sin fallo.

### ✅ La pantalla, vista (Eduardo, 3-sep noche): *«Veo 3 botones: Rojo, Azul y Amarillo (más bien naranja)»*

Justo lo que tocaba: el demo dibuja una pantalla lógica de 480×320 sobre un panel de 240×240, así
que se ve la columna izquierda (Rojo, Azul, Amarillo) y el título, centrado a 480, cae fuera. Los
colores son los del demo (`0xFFD000` es un amarillo cálido: si el orden RGB estuviera al revés el
rojo saldría azul, y no), la orientación es la buena y no hay bandas ni basura. **El primer backend
por SPI funciona a la primera con los pines del esquemático, `invert_color` y gap 0/0.**

### 🔬 Y la medida que el humo escondía: con la GUI, 93 KB en marcha

`log=1` → reset → `GuiColorDemo` 20 s → `KILL` → la línea `mem:`:

```
vm: DRAM interna libre 303164->106552 B (bloque mayor 278528->81920 B) | margen 16932   ← 192 KB
mem: DRAM interna libre 15308 B | MINIMO HISTORICO 13624 B (bloque mayor 14336 B)
```

LVGL pesa DOS veces: ~108 KB de DRAM **estática** al arrancar (410988 → 303164: fuentes, tablas,
`gui.c`) y **~93 KB en marcha**, porque va sobre el heap de C (`LV_USE_STDLIB_MALLOC = CLIB`, el
pool `LV_MEM_SIZE` no se usa): objetos, estilos, los dos draw buffers (23 KB con DMA) y el SPI. Con
192 KB de VM el sistema se quedó en **13 624 B** en el peor momento — al límite, y sin radios.
Segunda pasada, ya con 160 KB de VM (regrabada y verificada: `vm: 160 KB … techo 205 por el margen
del sistema`, INFO 98304/65536): mínimo **35 496 B** de 139 320 → **103 824 B** de uso en marcha,
más que en la primera (92 928): LVGL sobre el heap de C varía de pasada a pasada. Así que el
**margen es el máximo medido, 103 824**, y la imagen con pantalla pone **objetivo 128 KB** (heap 64 +
pilas 64, el reparto del C3): el planificador deja 172 KB al sistema, ~68 de holgura sobre el peor
pico. `test_mem` fija los dos casos del C6, sin y con GUI (**30/30**): con GUI manda «el margen del
sistema» (techo 194 KB), no el contiguo. Es la primera imagen en la que el margen lo pone la
pantalla y no el RTOS — y `U6` lo absorbe con un número.

### ✅ La imagen final, grabada y confirmada (4-sep, madrugada): 128 KB y el margen clava

Tercera pasada, ya con la imagen de 128 KB en la placa (mismo protocolo: `log=1` → `LOG_CLEAR` →
`RESET` → `INFO` → `GuiColorDemo` 20 s → `KILL` → `LOG_DUMP`):

```
heap: libre 303164 | mayor 278528 | bloques: 6 libres, 42 usados | usado 17760
vm: 128 KB en SRAM interna (objetivo 128, techo 194 por el margen del sistema)
vm: DRAM interna libre 303164->172088 B (bloque mayor 278528->147456 B) | margen 103824
INFO: vmHeapBytes 65536 / vmStackBytes 65536
mem: DRAM interna libre 69324 B | MINIMO HISTORICO 68264 B (bloque mayor 50176 B)
```

172 088 − 68 264 = **103 824 B**, el margen exacto: la tercera pasada repite la peor de las dos
anteriores al byte, así que el número no era un pico raro sino lo que la GUI cuesta en marcha. Al
sistema le quedan **68 KB** en el peor momento con el demo corriendo, y `log=0` otra vez.

**Lo que `P2.1` deja hecho:** `gui_display_st7789.c` (el cuarto backend del contrato
`bpvm_gui_disp_*` y el primero por SPI), LVGL vendorizada en el build del C6, `lv_conf.h`
consciente de la placa (RGB565, sin SDL), la `factory` a 1,5 MB, el planificador limitado por el
margen (y no por el contiguo) por primera vez, y los dos casos del C6 en `test_mem` (30/30).

⏭️ **`P2.2`** (lo que el driver aún no hace): PWM del backlight por LEDC (hoy GPIO a tope),
rotación 90/180/270 con sus gaps (el ST7789 de 240×240 vive en una RAM de 240×320), el LED RGB
WS2812B (GPIO8) como `Neopixel`, quizá el botón BOOT (GPIO9) como entrada, y comprobar desde el
IDE 6.0 que `Run on Device` sube `Gui`+`Json` solos (por el wire a mano hicieron falta los dos).
Cosmético: por el USB nativo asoma un trozo del log del bootloader antes del HELLO.

##### ✅ `P2.2` — HECHA (4-sep): la rotación con sus gaps, confirmada en placa; y lo que P2.2 NO fue

Antes de escribir nada, los hechos que cambian la lista (cada uno costó un viaje al código):

- **El IDE ya sube `Gui`+`Json` solo.** `resolveDeviceDeps` hace BFS sobre el grafo de imports
  desde junio (Gui → Json para el loader de Forms); por el wire a mano hicieron falta los dos porque
  el wire no resuelve nada. Queda sólo confirmarlo con un `Run on Device` desde el IDE 6.0 en COM3.
- **El PWM del backlight no tiene a quién servir.** En BasicPlus no hay API de brillo (el P4 también
  va al 100 % por LEDC). Un PWM sin API es el mismo «a tope» con más código: se queda el GPIO. Si
  algún día hay `Gui.setBrightness`, es una decisión de lenguaje (las dos VMs + stdlib), no de driver.
- **`Neopixel` es de la Pico.** El backend `bpvm_neopixel_backend_t` sólo lo registra `pico/main.c`
  (PIO); en TODA la familia ESP32 `Neopixel.init` devuelve 0 y `show` es no-op. Darle el WS2812B del
  C6 (GPIO8) es un adaptador nuevo para la familia (RMT/led_strip, valdría también para el LED del
  S3 y del C3), no «activar lo genérico». Se propone como ficha aparte, decisión de Eduardo.
- **El screen lógico del modelo dice 480×320 en todas las placas.** `bpvm_gui_set_screen_size` sólo
  la llaman el host (`--screen`) y el simulador; en firmware el modelo se queda con el `#define` y
  LVGL alinea contra el panel real (240×240 en el C6, 1024×600 en el P4). Por eso el `align` sale
  bien en la placa y el `__guiDumpTree` dice 480×320. Si el device dijera su tamaño, el dump (que es
  stdout) dejaría de ser idéntico al del host: es una decisión de paridad, no un bug del driver.

**Hecho esta noche — la rotación por el panel, con su gap por orientación.** El ST7789 de 240×240
vive en una RAM de 240×320: sobran 80 filas, al final sin `MY` (por eso 0° va con gap 0/0, visto) y
al principio con `MY`; con `MV` (90/270) el offset pasa al eje x. Es la tabla de Adafruit_ST7789
para 240×240 pasada a bits MADCTL: 0° → (0,0); 90° `MV|MX` → (0,0); 180° `MX|MY` → (0,80); 270°
`MV|MY` → (80,0). Tras el `set_gap` se invalida la pantalla entera (la RAM del panel guarda lo viejo
en la orientación vieja), y el aviso va a `ESP_LOGI`, no a stdout (paridad del OUTPUT: la versión
anterior hacía `printf`). El driver del IDF suma el gap a la ventana en `draw_bitmap` y no lo toca
al hacer `swap_xy`, así que el gap se da ya en términos del panel girado — lo que hace la tabla.

**El instrumento: `samples/GuiRotCycle.bp`.** `GuiRotDemo` gira con un toque y el C6 no tiene
táctil, así que este gira solo: un hilo (`extends Thread`, como `GuiAsyncDemo`) duerme 3 s y rota
+90, dos vueltas; el label del centro dice los grados y cuatro marcas `TL/TR/BL/BR` dicen dónde ha
ido cada esquina. Con el giro bien hecho, a 90 la `TL` está arriba a la derecha, a 180 abajo a la
derecha, a 270 abajo a la izquierda; una franja negra o un desplazamiento = el gap de esa
orientación está mal. Paridad host: **25 líneas byte-idénticas** miVM/VM-C. En la placa (imagen
regrabada 4-sep 00:15, `/app/GuiRotCycle.mod` subido): `RUN` → dump → `rotacion: 90` a los 3 s.

### ✅ Visto (Eduardo, 4-sep): *«La demo funciona perfectamente, y sí las esquinas coinciden»*

Las cuatro orientaciones, a la primera: la tabla de gaps era la buena, el sentido del giro es el
horario (90 y 270 no están cruzadas) y no hay franja en ninguna. **`P2.2` cerrada, y con ella
`P2`**: la pantalla del C6 está entera — bus, panel, LVGL, memoria medida, rotación.

Quedan fuera de `P2`, como decisiones y no como trabajo a medias:
- **`Neopixel` en la familia ESP32** (adaptador RMT; valdría para el LED del C6, del S3 y del C3).
  Hoy en ESP32 es no-op; la Pico lo tiene por PIO. Ficha aparte si Eduardo la quiere.
- **El tamaño del screen lógico en device** (hoy 480×320 en todas las placas; LVGL alinea contra
  el panel real; si el device dijera el suyo, el dump dejaría de ser idéntico al del host).
- **Confirmar desde el IDE que `Run on Device` sube `Gui`+`Json` solo.** El código es transitivo
  desde junio, pero la C6 ya tiene los dos en `/lib` (los subí por el wire), así que hoy el IDE los
  vería idénticos y no subiría nada: probarlo de cero exige borrarlos de `/lib`, y eso se decide.
- Cosmético: el trozo de log del bootloader que asoma por el USB nativo antes del HELLO.

##### ✅ `P2.3` — HECHA (4-sep): el screen mide lo que mide el panel

**Decisión de Eduardo (4-sep):** *«Lo de la resolución, ha de ser la que tenga la pantalla, no hay
otra. No tiene sentido que tenga más ni menos. Para más adelante, cuando hagamos LVGL en un pack, el
sistema tendrá que adaptarse a lo que el usuario le ponga, diferentes pantallas.»*

Hasta hoy el modelo se quedaba con el `#define` (480×320) en TODAS las placas y LVGL alineaba contra
el panel real: el `align` salía bien, pero `__guiDumpTree` y `scr.width` mentían (480 en un panel de
240). Lo que cambia, y es poco:

- **Contrato** (`bpvm_gui.h`): `int bpvm_gui_disp_native_size(int* w, int* h)` — el display dice su
  tamaño físico si lo sabe. Micro → 1 y el panel (C6 240×240 fijo; DK2 800×480 fijo; **P4 lo que
  diga el ENV**, `p4_panel_select()` es idempotente y se resuelve ahí mismo); host SDL → 0 (una
  ventana no tiene tamaño propio).
- **Modelo** (`gui.c`): la primera vez que hace falta el tamaño (crear el screen, o rotar antes de
  crearlo) se le pregunta al display. Un tamaño fijado con `bpvm_gui_set_screen_size` (host
  `--screen=`, simulador del IDE) **gana siempre**: así el host puede decir «soy una C6».
- Ese es también el camino para lo de más adelante: cuando LVGL vaya en un pack y la pantalla la
  ponga el usuario, el driver del pack contesta a esta misma pregunta y el modelo se adapta solo.

**Verificado:** la C6 regrabada dice `screen [240x240 align=0 +0,0]` en el dump de `GuiRotCycle`;
el host con `--screen=240x240` da el **mismo dump byte a byte** (8 líneas); sin `--screen` el host
sigue en 480×320 y su salida es idéntica a la de miVM (25 líneas: la paridad de siempre no se
toca); `sim-smoke` OK (el simulador fija el tamaño explícito y gana). Compilan las cuatro imágenes
con GUI: host, C6 (0x114360 B), P4 (0x13cb10 B) y Discovery (headless, 0 errores).



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

#### 🧊 `#460` — **BasicPlus NO tiene interfaces**: se retiran las de módulo, y la HERENCIA entre módulos queda en suspenso (decidido 30-ago)

**Eduardo, al preguntar qué era el «LSP entre interfaces de módulo»:** *«No entiendo qué es
`module interface`; nosotros no tenemos interfaces, ni para módulos ni para clases,
utilizamos herencia entre módulos y entre clases.»* Y al ver el censo: *«No hay interfaces
para módulos. No las tenemos para clases, que sería más defendible, no las tenemos para
módulos. […] Las demos de interface van fuera.»*

### Lo que el censo encontró, y por qué le daba la razón

| pregunta | medido en el árbol |
|---|---|
| ¿hay `module X extends Y` (herencia entre módulos)? | **CERO casos**, ni en el repo ni en la distribución |
| ¿herencia entre **clases**? | **38 casos sólo en `bpstdlib`** (`SyncList extends Core.List`, `RuntimeError extends Exception`…) |
| ¿está `module interface` en la gramática? | **NO** — `basicplus_grammar.ebnf.txt` no menciona `interface` ni una vez |
| ¿quién lo usaba? | 4 ficheros declaraban `module interface`, 9 `implements`, 6 imports con binding — **todos demos**. Nada de la stdlib, nada de producción |
| ¿estaba en el manual? | **SÍ**, tres secciones (10.4/10.5/10.6) en ES y EN. Al usuario sí se le prometía |

🔴 **Y además no funcionaba.** `samples/holes/modcompat/` era una guarda de regresión con
su `README`, y **estaba en rojo sin que nadie la corriera**. Su `UseOK.bp` —que el propio
README dice que *debe* compilar— falla, y con el mismo mensaje que el caso que *debe*
fallar:

```
UseOK  (pido IBase, doy ImplExt):  'ImplExt'  no implementa 'IBase' … declara IExt
UseBad (pido IExt,  doy ImplBase): 'ImplBase' no implementa 'IExt'  … declara IBase
```

Rechaza **las dos direcciones igual** ⇒ no recorre el `extends` en absoluto. O sea que lo
único que el mecanismo añadía sobre un emparejamiento directo —aceptar una implementación
**más nueva**— era exactamente lo roto. *(Y el `UseBad` «pasaba» sin probar nada: habría
fallado igual con el recorrido bien hecho.)*

### La decisión, en dos mitades

1. ❌ **Las interfaces de módulo se retiran.** Fuera los 16 `.bp` de demo, el
   `samples/holes/modcompat/` entero y `samples/plugins/`. Fuera también las tres secciones
   del manual (ES y EN), sustituidas por una nota que dice qué pasó y a dónde mirar —el
   ancla `#interfaces` se conserva porque estaba enlazada, y a quien conocía la función hay
   que decirle algo, no hacerla desaparecer.
2. 🧊 **La herencia entre módulos se aparca, no se cancela.** *«La idea es que sí, pero hay
   que aplicarlo bien.»* Su ejemplo es de hoy mismo: **`Math` se amplió editando el código
   en sitio**, y se podría haber hecho dejando el `Math` original y añadiendo uno nuevo que
   heredara de él. Lo que falta para eso: *«un control de versiones efectivo, que creo que
   hoy no tenemos»*. Se retoma cuando lo haya.

📌 **Y ahí está el caso de uso real, que conviene no perder**: el módulo que más ha
evolucionado es **`Core`**, y ha evolucionado **en sitio** — por eso sus desfases se pagan
con el `.mod skew` (los cuatro `Core.mod` que se quedan rancios) en vez de con versiones
que convivan. Cuando se retome, ése es el ejemplo con el que medirlo.

⏭️ **Cabos sueltos, a propósito:**
- El **compilador sigue aceptando** `module interface`, `implements` y el binding. Quitar
  ese código es un paso aparte y con su propia verificación; hoy sólo se retira del
  lenguaje *de cara al usuario* y se le quitan los usuarios.
- `referencia.html` §17.2 sigue describiendo los campos `interface`/`implements` **del
  formato `.mod`**, y hace bien: eso es el artefacto, no el lenguaje.
- Con esto **`appv1lsp`/`appv2` dejan de ser un bug**: eran la prueba de una función que ya
  no existe. La ficha del LSP se cierra por ahí, no por arreglarla.

#### ✅ `#459` — un `catch` SIN tipo entregaba un valor roto, y **eso se publicó en V5** (cerrada 30-ago)

**Síntoma**, visto en la ESP32-P4 corriendo `samples/trytest.bp` (sample publicado):

```
Probar(-3) => atrapado:       ▒      	vatrapado:       ▒      	vatrapado: ...
inner catch:       ▒ger#toFloat#4▒▒▒ Integer#toDouble#5▒▒▒ ... List#remove#17▒▒▒t
```

Eso que sale por pantalla **es la tabla de símbolos del programa**. `exit 0`, sin un
solo aviso.

### Aislado a una línea

| forma | resultado |
|---|---|
| `catch e: Core.RuntimeError` → `e.msg` | ✅ `[con tipo]` |
| `catch e2` (sin tipo) → `e2.msg` | ❌ bytes del heap |

Caso mínimo en `samples/pendientes/CatchSinTipo.bp`.

### La causa, y está escrita en el código

`MivmEmitter.java:3353` — el analizador tipa la variable de un `catch` genérico como
`ErrorType`, y el emisor hace:

```java
} else if (t == null || t instanceof BpType.ErrorType) {
    // Sucede con variables de 'catch' genérico ...
    // En BP los throws son típicamente strings.
    w.emit(OpCode.PRINT_STR_NONL);
}
```

O sea: emite *«imprime el string que hay en esta dirección»* sobre lo que es una
**referencia a objeto**. La suposición del comentario —*«los throws son típicamente
strings»*— dejó de ser cierta cuando las excepciones pasaron a ser objetos, y nadie
volvió a este `else if`.

⚠️ **No es sólo salida fea: es leer memoria ajena y enseñarla.** Un programa BP puede
volcar el heap de la VM sin querer.

### 🔴 Por qué NO lo cazó nada, que es lo más importante de esta ficha

1. **El arnés de paridad da VERDE.** Compara VM-Java contra VM-C, y **las dos producen
   la MISMA basura** — byte a byte. Es la *trampa del falso-PAR*: un oráculo que sólo
   compara dos implementaciones **no puede ver un fallo que ambas comparten**.
2. **`trytest.bp` no está en el corpus de paridad** (los 38 viven en `bpgenvm-c/samples/`).
3. **La stdlib usa la forma tipada** (`catch e: Core.Exception`), que funciona. El
   camino roto sólo lo pisa un sample.

📅 **Desde cuándo**: al menos desde **V5 publicada**. Comprobado con el artefacto, no
deducido: compilando y ejecutando `trytest.bp` con el compilador, la VM y la stdlib de
`dist/BasicPlus-5.0-win/` — misma basura. **No es una regresión de la tanda del 30-ago**,
que era la sospecha razonable y resultó falsa.

### ✅ El arreglo, en dos mitades — y las dos las decidió Eduardo

**1. El `catch` sin tipo es AZÚCAR.** *«Catch sin tipo debería ser como un `catch _e:
Exception` — no está en BP, lo añade el compilador al AST.»* Una línea en el `Parser`: si
hay variable y no hay tipo, el tipo es `Exception`. A partir de ahí todo va por el camino
tipado, que ya funcionaba.

📐 **Comprobado que no pierde capturas, en vez de suponerlo**: desde `#248` el compilador
**rechaza** `throw` de cualquier cosa que no sea una instancia de `Exception`
(*«solo se puede lanzar una instancia de Exception»*). O sea que el comentario del emisor
—*«en BP los throws son típicamente strings»*— describía un lenguaje que ya no existía, y
todo lo lanzable cae dentro del tipo que se pone.

**2. Y una excepción se imprime como su MENSAJE.** Pregunta de Eduardo al ver el arreglo a
medias: *«¿el `toString()` de una exception no debería imprimir el msg?»*. Sin eso el
agujero de memoria estaba tapado pero `trytest.bp` decía `atrapado: object@1073741827`.
Con el `toString()` en `Exception` (`Core.bp`) dice lo que su propio comentario anunciaba:

```
Probar(-3) => atrapado: n era negativo
inner catch: inner
outer atrapa relanzado: relanzado desde catch
```

📌 Es un **override del slot 0** (el `toString` de `Object`), así que no corre ningún slot
—la trampa de tocar una base de stdlib— y el layout de campos no cambia, que importa
porque las dos VMs asumen `campo 0 = msg` al fabricar un `RuntimeError`.

✅ **Verificado**: `trytest.bp` byte-idéntico en las dos VMs, paridad **38 PASS / 0 FAIL /
0 SKIP**, censo de 313 samples sin roturas nuevas, stdlib 27/27 y las cuatro imágenes.

✅ **Y EN PLACA (P4, 30-ago)**, con la salida **idéntica a la del host** — o sea que la
paridad llega hasta el silicio:

```
Probar(-3) => atrapado: n era negativo
inner catch: inner
outer atrapa relanzado: relanzado desde catch
fin
[Explorer] VM finished: exit 0 (OK)
```

📌 **Cómo salió, que es lo que hay que recordar.** Yo propuse correr `trytest.bp` como
trámite —*«ejercita el camino que más ficheros cambió»*— y Eduardo miró la salida y dijo
**«no sé»**. Ese «no sé» ante algo raro fue lo que destapó un fallo de seguridad de
memoria publicado en V5. La lección no es del bug: es que **una salida que no se entiende
merece pararse a mirarla**, aunque el `exit` sea 0 y aunque el arnés esté verde.

#### ✅ `#458` — `Core` implícito: la norma pasa a ser EXPLÍCITA, y la pasada de interfaz deja de tirar miembros en silencio (cerrada 30-ago)

**Norma de Eduardo (30-ago):** *«la norma tiene que ser sencilla: si se utiliza un tipo
de Core, se ha de importar Core»*. Y sobre el coste: *«estamos haciendo una nueva versión,
si hay que recompilar las librerías, se vuelven a compilar. Lo importante es resolver el
bug.»*

🔑 **Por qué la norma en vez de arreglar la pasada.** El bug era que el `import Core`
sintético lo inyectaba la pasada COMPLETA y no la de INTERFAZ, así que las dos veían
cosas distintas del mismo fichero. Se podía enseñar a la de interfaz a inyectarlo
también; la norma es mejor porque **no hay dos sitios que tengan que adivinar igual: no
hay nada que adivinar**. De propina, un módulo que no usa `Core` deja de arrastrarlo.

`Object` queda fuera, y no por excepción: **no es un tipo de `Core`**. Es un tipo real
—con `toString()` y `compareTo()` en los slots 0 y 1 de toda instancia— pero vive en el
compilador (`SemanticAnalyzer.java:385`), junto a los primitivos. *(Corrección de Eduardo:
yo lo había contado como si no fuera un tipo de verdad.)*

### ⚠️ La mitad que no vi hasta el tercer intento

Quitar la inyección hace saltar el fallo en la pasada COMPLETA… **pero un módulo que se
consume como dependencia se compila antes en modo INTERFAZ, que es TOLERANTE a propósito
y no reporta errores.** O sea que seguía tirando el miembro en silencio y el consumidor
seguía diciendo *«no tiene miembro de instancia 'todos'»*, lejos de la causa. La norma
sola NO arreglaba ese camino, que es justo el del bug original.

La distinción que faltaba, y es la que cierra la ficha:

| omitir un miembro porque… | qué es |
|---|---|
| su tipo **no debe** exportarse | correcto, informativo |
| su tipo **no resolvió** (`<error>`) | **un fallo** — y encima sólo visible en OTRO fichero |

Ahora el segundo caso es un error de verdad, y lo dice donde ocurre:

```
>>> compile Repo.bp (mode=INTERFACE)
error: la interfaz de 'Repo' PIERDE 'class Almacen.method todos: retorno tipo no
exportable: <error>' porque su tipo no resuelve. El módulo compila, pero quien lo
importe no verá ese miembro. Si el tipo es de `Core`, este módulo necesita `import Core`.
```

### La pista, y el falso positivo que se coló

El mensaje de «tipo no encontrado» sugiere `import Core`, pero **sólo si el nombre es de
verdad una clase de Core**, y los nombres se leen de la interfaz de Core al cargarla —
nunca de una lista escrita a mano, que se quedaría rancia. La primera versión los pegaba
a CUALQUIER identificador sin resolver y **el censo de samples la pilló diciéndole a
`SQLite` que importara Core**: mandando a mirar donde no es, que es el pecado que la
pista venía a evitar.

### Alcance, medido

**56 ficheros `.bp`** ganan `import Core` (32 de `samples/`+`bpstdlib/`, 23 de
`bpgenvm-c/samples/` y `diag/`, más `Stdlib.bp`). Dos cosas que el censo por regex NO vio
y sí vio **compilar todo**:
- `Collections.bp` usa `Core.List` **cualificado** — mi regex excluía los usos con punto;
  ya lo importaba, pero el caso es real: cualificar exige importar.
- **`bpgenvm-c/samples/` se me quedó fuera del censo** (censé los directorios que
  recordaba) y ahí vive el corpus de paridad: el arnés cayó a **33 PASS + 5 SKIP** y sólo
  se vio porque *un SKIP no es un PASS*.

✅ **Verificado**: stdlib **27/27 desde cero, 0 errores**, paridad **38 PASS / 0 FAIL /
0 SKIP**, censo de **313 samples sin ninguna rotura nueva** (los 2 que disparan el error
nuevo ya fallaban: les falta el pack de SQLite).

✅ **VERIFICADO EN PLACA (P4, 30-ago)**: `JsonDemo` corre entero, `exit 0`. Y lo que ese
gesto prueba de verdad no es el JSON:

| lo que subió el IDE | por qué importa |
|---|---|
| `Core.mod` | es EL módulo que hasta hoy no se podía regenerar (`#457`). Reconstruido desde fuente, MOD7, ejecutándose en RISC-V |
| `Json.mod` | uno de los 8 de la stdlib que ganaron `import Core`: compila bajo la norma nueva y corre |

Y `JsonDemo` es el sample con el que arrancó toda la campaña — el del `exit 6` con el
código pisado de `#440`.

⚠️ **Lo que NO cubre**, para no apuntárselo: sólo el **P4**, y sólo **2 de los 27** módulos.
No se ha ejercitado ningún `catch` (el grupo `RuntimeError` es el que más ficheros movió:
23 de 56) ni las cuatro funciones nuevas de `Math`. Pico, S3 y STM32 llevan la stdlib
nueva sin estrenar.

📌 **Lo que enseña**: *reproducir hasta que el arreglo se vea funcionar, no hasta que
parezca correcto*. Di el bug por arreglado dos veces —al quitar la inyección y al añadir
la pista— y las dos veces el caso original seguía fallando igual. Sólo el tercer intento,
mirando la SECUENCIA de pasadas en vez de los errores sueltos, dio con la mitad que
faltaba.

#### ✅ `#457` — el compilador ESCRIBE `.mod` v7 y su lector de interfaces sólo aceptaba v6 (cerrada 30-ago)

🔴 **La stdlib no se podía regenerar.** Recompilar `bpstdlib/Math.mod` desde su propio
fuente, sin tocar una línea, dejaba de resolver: cualquier consumidor daba
`identificador no resuelto: 'Math'`.

**Cómo se acotó** — cambiando UNA cosa cada vez, que es lo que evitó culpar a un inocente:

| experimento | resultado |
|---|---|
| `Math.mod` del repo + compilador de hoy | ✅ compila |
| `Math.mod` regenerado **con mis 4 funciones nuevas** | ❌ |
| `Math.mod` regenerado **desde el `Math.bp` SIN tocar** | ❌ ← *aquí se cayó la hipótesis* |
| ... regenerado **por proyecto** en vez de suelto | ❌ (no era la receta) |

Lo tercero es lo que lo resolvió: si el fuente sin cambios también rompe, **no son los
cambios**. Un `hexdump` de los dos `.mod` lo dijo en el primer byte: `MOD6` → **`MOD7`**.

📐 **La causa**: `N1.4` subió el formato a v7 (sección `native` embebida, el `.mdn` deja
de ser fichero aparte) y `extractInterfaceSection` (`lexer-java/.../Main.java:1533`) se
quedó en `if (magic != MAGIC_NUMBER_V6) return null;`. O sea que **el compilador escribía
un formato que él mismo no sabía leer**.

⚠️ **Y es un fallo MUDO doble.** El `return null` no dice nada, y el error aparece
larguísimo después y en otro fichero, como un identificador sin resolver. Encima estaba
tapado por dos casualidades: los 27 `.mod` de `bpstdlib/` seguían siendo v6 (se generaron
antes del salto) y los módulos que se compilan **juntos** resuelven su interfaz en
memoria, sin pasar por disco. Sólo muerde al regenerar uno — que es justo lo que
`PUBLICAR.md` manda hacer al tocar la stdlib.

✅ **Arreglado**: se acepta v7 y se consume su entero extra (`nativeSize`, el 9º del
header — `HEADER_SIZE_V6=32` → `V7=36`); sin eso las secciones se leen 4 bytes corridas.
Verificado: un `Math.mod` v7 resuelve, paridad dual-VM **38 PASS / 0 FAIL / 0 SKIP**.

📌 **Lo que enseña**: *un formato tiene dos lados, y el que escribe no prueba al que lee*.
Las VM-Java y VM-C ya aceptaban v7 (`ModFormat.isKnownMagic`, `loader.c:125`); el que se
quedó atrás fue el **tercer lector**, el del propio compilador, que nadie recuerda que
existe porque casi siempre trabaja en memoria. Enlaza con
[[contar-los-consumidores-no-leer-el-codigo]]: al subir un formato hay que **censar los
lectores**, y son tres, no dos.

#### ✅ `#465` — tres suposiciones del S3 en el código COMPARTIDO de la familia ESP32 (cerrada 31-ago)

Las destapó **el ensayo del C3** (`P1.C3.1`), una a una, y ninguna se habría visto sin
intentar el port. Las tres viven en ficheros que usan **las tres placas**.

| # | lo que se daba por hecho | el C3 | cuándo saltó | arreglo |
|---|---|---|---|---|
| 1 | 3 UART y 3 SPI (`gpio_esp32.c`) | **2 y 2** | al compilar | `#if SOC_UART_NUM > 2` · `SOC_SPI_PERIPH_NUM > 2` |
| 2 | *«es RISC-V»* ⇒ hay API de caché (`repl_esp32.c`, **5 guardas**) | RISC-V **sin `esp_cache.h`** | al compilar | `__has_include`, capacidad en vez de arquitectura |
| 3 | todos los ESP32 tienen PCNT (`gpio_esp32.c`) | **no lo tiene** | al **enlazar** | `#if SOC_PCNT_SUPPORTED`, y sin backend se queda el stub portable |

📌 **La segunda es la que más enseña.** *«Es RISC-V»* funcionaba como atajo de *«soporta
cargar un `.mdn` en RAM ejecutable»* **mientras el único RISC-V fuera el P4**. En cuanto hay
un segundo, el atajo es falso. Un proxy que acierta con una sola muestra no es una regla: es
una coincidencia con suerte.

✅ **Las tres mejoran a TODAS las placas**: el S3 (Xtensa) y el P4 evalúan las guardas
exactamente igual que antes, así que su binario no cambia de comportamiento. Verificado
reconstruyendo las dos.

📌 **Y lo que enseña del método**: *una familia nueva no sólo se añade — AUDITA el código
compartido*. Las tres estaban en ficheros que ya pasaban por tres placas y ningún arnés podía
verlas, porque sólo se manifiestan al compilar para un silicio que no las cumple.

#### ✅ `#464` — un `if` sin llaves en el arranque del S3: guardaba la línea equivocada (cerrada 31-ago)

Salió **preparando el ensayo del C3**, leyendo el `main.c` del S3 como plantilla. No lo buscaba
nadie: el código llevaba así desde `#450`.

```c
if (board_boot_status()->state == BPVM_BOOT_APP && !board_boot_status()->degraded)
    /* …catorce líneas de comentario… */
    bpvm_set_stack_kb(...);      // ← ESTO era el cuerpo del if
    bpvm_set_quantum_ops(...);   // incondicional
    bpvm_log_set_enabled(...);   // incondicional
        repl_esp32_autorun();    // incondicional, e indentado como si estuviera dentro
```

**Los dos efectos, exactamente cambiados:**

| | debería | estaba |
|---|---|---|
| `bpvm_set_stack_kb` (`stack=N`) | siempre | **sólo con la placa en estado APP y no degradada** |
| `repl_esp32_autorun()` | sólo con la placa sana | **siempre** — al revés de lo que dice su propio comentario |

🔎 **Y el P4 lo tiene BIEN**, en una sola línea: `if (bs->state == BPVM_BOOT_APP && !bs->degraded)
repl_esp32_autorun();`. La misma intención escrita en dos formas: la de una línea aguanta, la
repartida con catorce de comentario en medio se rompió sin que nadie lo viera. Se copia la del P4.

⚠️ **Por qué no había mordido**: las dos consecuencias sólo aparecen con la placa **degradada o
por debajo del estado 3**, que es el caso raro. `stack=N` se probó en una placa sana y funcionó.

📌 **Lo que enseña**: *un `if` sin llaves con un comentario largo detrás es una trampa que ni
compila mal ni avisa*. Y salió de leer el fichero **para copiarlo**, no de depurar — preparar un
port hace leer código que llevaba meses sin leerse con atención.

#### ✅ `#463` — el IDE no sabía que `Core` va embebido: lo subía a `/app` y creaba un override sin querer (cerrada 31-ago)

**Lo vio Eduardo mirando el árbol de la placa**: *«parece como si el Core no se detectara en la
stdlib y lo copia casi siempre»*. Y el árbol lo decía sin ambigüedad — `Core.mod` en `/app` **y**
en `/lib`, con el mismo tamaño exacto (13111 B).

### La causa: una lista de trece donde había catorce

`FrmMain.EMBEDDED_CORE_MODS` decide **la carpeta destino** de cada dependencia. Comparada con
lo que el firmware embebe de verdad (leído del generado `esp32_mods.c`, no de memoria):

```
firmware embebe (14): Adc Core Gpio I2c IO Math Pico Pulse Pwm Rtc Spi Timer Uart Wdt
IDE creía       (13):     Adc Gpio I2c IO Math Pico Pulse Pwm Rtc Spi Timer Uart Wdt
                          ^^^^ falta Core
```

Un solo nombre. Arreglado añadiéndolo.

### 📌 Y la corrección de Eduardo, que cambia cuál es el fallo

Escribí que el problema era que `/app` gane a `/lib`. **Falso**: *«la precedencia está bien, el
de /app debe ir antes que /lib. Así si tienes un módulo más moderno lo puedes probar; de la otra
forma no se podría probar sin borrarlo de /lib.»*

O sea que `/app` **es el mecanismo de override**, y está bien puesto. El fallo es otro y más
fino: **el IDE creaba ese override sin que nadie lo pidiera**. Un override deliberado se
recuerda; uno accidental te espera — flasheas una imagen con un `Core` nuevo y la copia vieja de
`/app` lo sigue tapando, en silencio. Es la misma familia que el módulo rancio de `/lib`, pero
por el otro lado.

### ✅ Verificado en placa (Discovery, 31-ago)

```
[deps] 4 módulo(s) a subir:
  - Gui.mod  → /lib
  - Pico.mod → /lib
  - Json.mod → /app      ← correcto: Json NO va embebido en la imagen
  - Core.mod → /lib      ← antes iba a /app
[Explorer] /lib/Core.mod ya en FS (13111 bytes, contenido idéntico), salto PUT
```

El CRC lo encuentra idéntico en `/lib` y se salta la subida, que es justo lo que tenía que
pasar. *(El `/app/Core.mod` que dejaron las ejecuciones anteriores hay que borrarlo a mano: el
arreglo evita crear el próximo, no limpia el que había.)*

### ⚠️ Lo que queda abierto detrás, y es lo de fondo

**Esa lista es un GEMELO escrito a mano de lo que el firmware embebe**, y se habían separado por
uno. Lo robusto es que **lo diga el dispositivo** en vez de que el IDE lo recuerde: el `LIST` de
`/lib` ya existe y el IDE ya compara por CRC fichero a fichero. Mientras siga siendo una
constante en Java, volverá a desincronizarse — el día que el firmware embeba uno más.

🔎 **De paso, una inconsistencia entre los dos caminos del IDE**: el de ejecución usa
`EMBEDDED_CORE_MODS.contains(mod) || "Gui".equals(mod)` y el de **depuración** (`FrmMain:2778`)
sólo `EMBEDDED_CORE_MODS.contains(mod)` — sin el `Gui`. Así que `Gui.mod` va a `/lib` al ejecutar
y a `/app` al depurar. Sin verificar en placa; anotado.

🧹 **Al aplicar esto hay que limpiar a mano** el `/app/Core.mod` que las ejecuciones anteriores
ya dejaron: el arreglo evita crearlo, no borra el que hay.

#### ✅ `#462` — la VM **nunca cede el turno al SO**: en el ESP32 un programa bloquea el resto del sistema (abierta 31-ago · **CERRADA el 10-sep**: el suelo, 48 → 7 ms en placa)

**Eduardo, como usuario:** *«cuando se ejecuta un programa en las STM32, el resto del
sistema sigue vivo, incluidas las comunicaciones; en cambio en las ESP32 se utilizan todos
los recursos y se queda todo bloqueado. Es cierto que pasan los eventos pero mucho más
lentos. Y como programador, creo que la diferencia no es tanto a nivel de código nuestro
sino más a nivel de SO.»*

**Tenía razón, y la causa está en nuestro planificador.**

### El hallazgo

`scheduler.c` sólo llama a `bpvm_platform_thread_sleep_ms` **cuando NO hay ningún thread
ejecutable**. Mientras el programa BP tenga trabajo, el bucle gira sin bloquearse nunca:

```c
if (idx < 0) { ... bpvm_platform_thread_sleep_ms(dt); continue; }   /* único punto que cede */
/* con trabajo: quantum, y vuelta a empezar — sin ceder */
```

En **bare-metal** (STM32) da igual: no hay a quién matar de hambre, y por eso *«funciona muy
bien»*. Bajo **FreeRTOS** la tarea de la VM se come todo lo que esté a su prioridad o por
debajo — incluido lo que bombea LVGL, que es el *«los eventos pasan pero mucho más lentos»*.

🔑 **Y lo que lo hace sangrante: `bpvm_platform_thread_yield()` YA EXISTÍA** en el contrato
(`bpvm_platform.h:57`) y lo implementaban **las cuatro** plataformas — `taskYIELD()` en
ESP32 y Pico, no-op en el STM32 porque allí no hace falta, `sched_yield()` en el host.
**Nadie lo llamaba.** Cero llamadas en todo `src/`. Un gancho que existe y no se usa no es
una abstracción: es una promesa sin cumplir.

### Y hay una segunda diferencia, de configuración, entre las dos ESP32

| | quién ejecuta la VM | prioridad |
|---|---|---|
| **STM32** | bare-metal, sin RTOS — el REPL *es* el bucle principal | — |
| **ESP32-S3** | `app_main` | **1** (`ESP_TASK_PRIO_MIN + 1`, del IDF) |
| **ESP32-P4** | `xTaskCreate(wire_task_uart, …, 5, NULL)` | **5**, y sin fijar a un núcleo |

El mismo código corre a prioridad 1 en el S3 y a **5** en el P4. A 1, casi cualquier tarea
de servicio del IDF (5, 18, 22…) desaloja a la VM y el sistema respira; a 5 la VM gana. Eso
explica que el P4 —*«más rápido y con más de todo»*— se comporte peor.

### Medido

Ceder una vez por quantum (**1024 opcodes**) cuesta en el host **~3,6 %**: 658 ms → 682 ms
en un bucle entero de 20 M de iteraciones, 5 medidas estables cada uno. Paridad **38 PASS /
0 FAIL / 0 SKIP** con el yield puesto.

⚠️ *Y una medida mía que era falsa antes de esa*: 30 ms «con yield» contra 658 sin él. El
número imposible lo delató — ese comando corrió con el shell dentro de `bpgenvm-c/`, la
ruta relativa no resolvía, y el cronómetro midió **un exec fallido**. Instrumento mudo, otra
vez, y otra vez lo cazó que el resultado fuera demasiado bueno.

### ✅ Verificado en el P4 (31-ago): **el `stop` vuelve a funcionar**

Eduardo, con la imagen del yield flasheada: *«Sí, es la última imagen de P4. Antes el stop
ni funcionaba.»*

```
/> stop
[Explorer] Stop: KILL enviado a la placa
4 tras run()
[Explorer] VM finished: exit 0 (OK)
```

🎯 **Y de paso, `GuiEvSpike.bp` sale en su caso BUENO.** Su cabecera fija el criterio —
*«`3 handler` ANTES de `4 tras run` → el drenaje ocurre DENTRO de `run()`»*— y los seis `3`
salen antes del `4`. O sea que el evento **anidado** (el que un handler levanta desde dentro
de otro handler) se drena dentro del bombeo. `#324` aguanta en el P4.

⚖️ **La fuerza de la atribución, dicha con precisión.** Entre la imagen anterior del P4 y
ésta el ÚNICO cambio de código es `scheduler.c` — el yield (`interp.c` de `#441` ya iba en
la de las 07:39; su commit llegó un minuto después de construirla). Pero **no consta qué
imagen tenía la placa cuando el `stop` falló**, así que esto es una correlación muy
apretada, no un test de desplazamiento.

📌 **Y el mecanismo exacto sigue sin estar probado**: el poll corre DENTRO de la tarea de la
VM y los bytes del UART los mete una ISR, que desaloja a cualquier tarea — o sea que por pura
lógica el KILL debería haberse visto igual sin el yield. Que funcione no explica por qué.

⏭️ **El control limpio, si se quiere cerrar del todo**: quitar el yield, reflashear y ver si
el `stop` vuelve a fallar. Un ciclo de flasheo. Merece la pena por una razón concreta: si lo
arregló otra cosa, estaríamos apuntándole el mérito al cambio equivocado y la causa real
seguiría escondida — que es exactamente lo que pasó con el andamio del MPU en `#440`,
acusando a dos programas inocentes.

### ⏭️ Lo que queda: el DESFASE, que es la otra mitad

Los handlers van **hasta tres clics por detrás** de sus upcalls (`2 2 2 → 3 3`). No se
pierde ninguno, pero llegan tarde: es el *«los eventos pasan pero mucho más lentos»* de
Eduardo, ya con forma medible. El yield no lo ha quitado.

La otra mitad es la **prioridad**: bajar el `wire_task` del P4 de **5** a **1** para
igualarlo al S3. 📌 Criterio de Eduardo para esto: *«ahora que estamos unificando, sería
recomendable quedarnos con lo bueno y no unificar a lo peor»*.

### 🔬 Y al MEDIRLO en ms, la explicación cambió (31-ago)

`GuiEvSpike` sólo dice el ORDEN, y con el orden **no se puede distinguir «el P4 drena
lento» de «en la Discovery cliqué más despacio»**. Por eso se escribió
`samples/GuiEvLat.bp`, que cronometra el hueco upcall→handler y la profundidad de la cola.

**Discovery** (la placa que *«funciona muy bien»*):

```
clic 1  upcall t=267636      handler lat=393 ms  cola=0
clic 2  upcall t=269852      handler lat=393 ms  cola=0
clic 3  upcall t=271712      handler lat=420 ms  cola=0
clic 4  upcall t=272519      handler lat=416 ms  cola=0
clic 5  upcall t=273320      handler lat=393 ms  cola=0
```

🔴 **La Discovery TAMPOCO es rápida: cada evento tarda ~400 ms.** Lo que pasa es que los
clics van a 800-2200 ms de distancia, así que nunca se acumula y el patrón sale en pares
limpios. **El «funciona bien» era un artefacto del ritmo del dedo.** Sin cronómetro esto no
se veía — y llevábamos dos diagnósticos apoyados en ello.

📌 **Y la constancia es la pista**: 393, 393, 420, 416, 393. Tan estable no puede ser carga
ni contención; es un **número fijo de vueltas**.

### La causa que encaja: el quantum se mide en OPCODES

`Gui.run()` es `while __guiRunOnce() do endwh` — unos **3-4 opcodes por vuelta**, de los que
**uno bombea LVGL entero** (~1,4 ms). El planificador sólo drena los eventos encolados
**entre quanta**, y un quantum son **1024 opcodes**:

```
1024 opcodes / ~3,5 por vuelta  ≈  290 vueltas de bombeo
290 vueltas × ~1,4 ms           ≈  400 ms   ← lo medido
```

Un quantum en opcodes es barato y predecible **mientras cada opcode cueste lo mismo**. Deja
de serlo cuando uno cuesta milisegundos. Y esto ya estaba escrito y no se había conectado:
`bpvm_internal.h:280` anota que *«la VM-Java mide su quantum por TIEMPO, y no la C, con
quantum por opcodes»* — como una diferencia de comportamiento entre VMs, sin ver que aquí
se convierte en 400 ms de latencia.

🔧 **Construido el mando para PROBARLO en vez de creérselo** (`quantum=N` en el ENV, mismo
patrón que el `stack=N` de `#450`, en las cuatro familias).

### ✅ Probado en la Discovery (31-ago): la hipótesis acierta **a medias**, y eso vale más

| `quantum` | vueltas | latencia medida |
|---|---|---|
| 1024 (por defecto) | ~293 | **393-420 ms** |
| **32** | ~9 | **63-64 ms** |

**Baja, pero NO proporcionalmente**: 32× menos quantum da sólo **6,3×** menos latencia. Ajuste
sobre los dos puntos:

```
lat = 1,19 ms/vuelta × vueltas  +  52 ms de SUELO
```

✅ El **1,19 ms por vuelta** confirma el mecanismo del quantum (estimado ~1,4 antes de medir).
🔴 Pero hay **un suelo de ~52 ms que el quantum no toca**: una SEGUNDA causa, independiente.

### ✅ El modelo PREDIJO y acertó (tercer punto, 31-ago)

Con dos medidas el ajuste encajaba por construcción, así que se hizo una **predicción antes de
medir**: `quantum=128` → **~95 ms**. Medido: **87 ms**.

| quantum | vueltas | medido | modelo (3 puntos) |
|---|---|---|---|
| 1024 | 293 | 403 ms | 403 ms |
| 128 | 37 | **87 ms** | 92 ms |
| 32 | 9 | 63 ms | 59 ms |

```
lat = 1,21 ms/vuelta × vueltas  +  48 ms de SUELO
```

Error máximo **5 ms** en un rango de **32×**. Las dos causas quedan separadas y medidas:

1. 🟢 **El quantum en opcodes** — `1,21 ms × vueltas`. Es el término que crece, y el que hoy
   pone los 400 ms. **Confirmado.**
2. 🔴 **Un suelo de 48 ms**, independiente del quantum. Sospechoso: `LV_DEF_REFR_PERIOD = 33 ms`
   (`include/lv_conf.h:82`). Explica buena parte, no todo. Sin investigar.

🔍 **Sospechoso del suelo**: `LV_DEF_REFR_PERIOD = 33 ms` (`include/lv_conf.h:82`), el periodo
de refresco de LVGL. Explica buena parte de 52, no los 52. *(Ojo: la latencia se mide desde el
upcall, o sea que la detección del clic queda FUERA — el suelo está entre el `raise` y el
handler.)*

### 🐛 Y un error SILENCIOSO que el quantum destapó — y NO es «mal», es INESTABLE

| quantum | qué reporta el `stop` |
|---|---|
| 1024 | `exit 0 (OK)` |
| **32** | **`exit 130 (KILLED)`** |
| 128 | `exit 0 (OK)` |

Con el quantum grande el KILL llega en un punto donde `Gui.run()` devuelve por su cuenta y el
programa termina «normal»; con el pequeño lo ve el planificador y sale como matado. O sea que
**el estado de salida de un `stop` depende de DÓNDE caiga el KILL**: un programa matado puede
decir que salió con éxito, y no siempre el mismo. Eso no es un valor equivocado, es una
**carrera** — peor de diagnosticar y peor de fiarse.

📌 Y no lo buscábamos: salió de cambiar un número para medir otra cosa.

### ✅ La carrera del estado de salida, ARREGLADA (5-sep, `83080c02`)

`bpvm_scheduler_run` salía con `BPVM_OK` aunque hubiera un KILL pendiente. La bandera es la
verdad: **si se pidió parar, se paró**, aunque el programa terminara por su cuenta antes de que
el planificador volviera a mirarla.

Lo volví a medir sin buscarlo, el 5-sep, en **dos placas**: un KILL sobre `GuiColorDemo` daba
`status OK, exitCode 0` en el P4 y en la Discovery.

📌 **Y lo que lo cierra del todo: la VM-Java YA lo hacía bien.** `miVM/Main.java:361` comprueba
`isKillRequested()` tras `run()` y pone 130. Era la VM-C la que se salía del contrato, no las dos
— así que esto no era una decisión de diseño pendiente, era una diferencia entre VMs.

El camino SMP no hacía falta tocarlo (`bpvm.c:1193` ya comprobaba la bandera): lo miré, lo cambié
y lo revertí, porque habría traducido a `ERR_RUNTIME` una salida que ya salía bien.

**Regresión con su CONTROL**, que es lo que hace que la prueba valga: `io_smoke.py` gana el caso
del GUI — arrancar `Gui.run()`, matarlo, exigir `KILLED`. Con el arreglo dice `KILLED`;
quitándolo dice `OK`. Y por el camino, dos cosas del arnés que lo habrían dejado pasar en
silencio: compilaba desde un directorio temporal —donde el frontend no encuentra la stdlib, así
que un caso con `import Gui` **ni compilaba**— y no subía `Core`/`Json`/`Gui` al FS del
simulador. Ahora la prueba **dice en voz alta cuándo se salta**.


### ✅ 9-sep — LA CAUSA DE FONDO, ARREGLADA: la pausa estaba en el sitio equivocado (`805bdd87`)

**Y el diagnóstico que había aquí arriba —«el quantum en opcodes»— era el SÍNTOMA.** Lo cortó
Eduardo con el modelo: *«el bucle de LVGL tiene que mirar si hay algo pendiente, si no hay nada
pendiente lo que tiene que hacer es una pausa y el sistema de Threads le ha de pasar el testigo al
siguiente.»*

📌 **Y antes de eso, otra corrección suya que ahorró el viaje**: *«ahora el bucle se ejecuta en su
propio Thread BP, no debería afectar al resto»*. Cierto: los 400 ms de arriba son del **31-ago**, y
`G1` metió el lazo en su hilo BP el **6-sep**. Había que volver a medir, no seguir citando.

### Lo que se midió (`samples/GuiHambre.bp`, con su control)

Un hilo testigo que sólo mira el reloj mientras el lazo bombea al lado; y la misma medida **sin
GUI**, que es lo que hace que el número signifique algo.

| | antes | **después** | control (sin GUI) |
|---|---|---|---|
| **VM-C** peor hueco | **2136 ms** | **1 ms** | 2 ms |
| **VM-C** vueltas del testigo en 3 s | **85** | 25 941 803 | 12 985 899 |
| **miVM** peor hueco | 64 ms | **16 ms** | 17 ms |

Las dos VMs **empatan ya con su propio control**: el lazo del GUI ha dejado de costarle nada a los
demás hilos BP.

### La causa, y es la misma en las cinco implementaciones

El bombeo **sí** hacía la pausa. La hacía **a nivel de SO**:

```
host (SDL)   lv_timer_handler();  SDL_Delay(16);      ← 16 ms fijos, y tirando lo que LVGL contestó
P4  (ESP)    idle = lv_timer_handler();  vTaskDelay(…)  ← SÍ preguntaba… y se comía la respuesta
C6  (ESP)    ídem
STM32        lv_timer_handler();  __WFI();
miVM         Thread.sleep(15) DENTRO del builtin
```

🔑 Y todos los hilos BP corren sobre **una sola tarea de SO** — decisión de `A1`, y correcta, porque
LVGL no es reentrante. Así que esa espera no dormía el hilo del GUI: **congelaba la VM entera**. El
planificador de threads BP ni se enteraba de que podía pasar el testigo. Por eso el testigo corría
85 veces en tres segundos: no perdía la competición, **nadie repartía**.

### El cambio

`__guiRunOnce()` hace el trabajo y **devuelve el ocio** que dice el propio LVGL — `-1` = «no queda
nada, sal»; `>= 0` = «vuelve dentro de N ms». Quien duerme es el lazo BP de `Gui.bp`, con un `sleep`
**de BP**, que bloquea sólo ese hilo BP. El tope (10 ms, el que midió `#424`) queda en **un sitio**
para las cuatro cinturas y para miVM, en vez de repetido en cinco.

📌 Esto **da la vuelta a una decisión escrita**: `Gui.bp` decía *«SIN `sleep` AQUÍ, y es a propósito.
La espera la pone el BOMBEO»*. Era justo eso lo que había que cambiar.

⚠️ **Y una madriguera mía, que conviene no repetir.** Perseguí durante media hora un fallo
intermitente del arnés (1 de cada 5) creyéndolo mío. Lo provocaba **yo**, lanzando dos programas de
GUI a la vez para medir. Eduardo: *«no puede haber 2 LVGL a la vez, punto.»* Ejecutado como toca:
**8 pasadas seguidas del arnés a 48 PASS** y 15 de `GuiParidad2` con rc=0 y las 23 líneas. El
instrumento estaba midiendo una configuración que no existe — [[instrumento-mudo-dudar-de-el]] por
el otro lado: no un doble más amable, un doble IMPOSIBLE.

### ⏭️ Lo que queda de esta ficha

| | Qué | Estado |
|---|---|---|
| 1 | El **yield** entre cuantos | ✅ 31-ago, y `A1` lo mantiene |
| 2 | El wire atendido **mientras la VM calcula** | ✅ `A1`: es el hilo `io`, en las cinco familias |
| 3 | La **carrera del estado de salida** | ✅ 5-sep (arriba) |
| 4 | El **quantum en opcodes** | 🟡 **el caso del GUI, resuelto el 9-sep sin tocarlo** (arriba): el hilo del GUI se aparta solo. El quantum en opcodes sigue siendo desigual para CUALQUIER hilo con opcodes caros, pero ya no hay nada que lo esté sufriendo |
| 5 | Un **suelo de latencia** independiente del quantum | 🟡 **10-sep**: el sospechoso (`LV_DEF_REFR_PERIOD`) **DESCARTADO** con prueba de desplazamiento, y los 48 ms estaban caducados. Sólo medible EN PLACA — ver abajo |
| 6 | La **prioridad del `wire_task` del P4** | ➡️ **SEPARADA el 9-sep como `#485`** (Eduardo: *«muchas de estas entradas en realidad son múltiples cosas»*) |

📌 El 6 es ahora más concreto que en agosto: ya no es «el P4 va a otra prioridad», es que
**incumple el contrato de `A1`** (`io` a la MISMA prioridad que la VM). En el P4 no se nota
porque es de dos núcleos —`io` corre en el otro— pero es la misma forma del fallo que en la Pico
dejó un KILL sin llegar durante 9,9 s.

🐛 **La ineficiencia del bucle más caliente del GUI** —`GUI_RUN_ONCE` recorriendo toda la tabla de
símbolos con `strcmp` en cada pasada— ➡️ **SEPARADA el 9-sep como `#486`**.

⚠️ **Lo que esto le hace al diagnóstico anterior**: la prioridad 5 del P4 sigue siendo una
diferencia real, pero **ya no es la explicación del retraso** — la Discovery, sin RTOS y sin
prioridades, tiene la misma latencia. Lo que el P4 añade es que además se le acumulan.


### 🔬 10-sep — EL SOSPECHOSO DEL SUELO, DESCARTADO. Y el suelo del PC es el PC.

**Lo primero, y es la lección de ayer otra vez: los 48 ms estaban CADUCADOS.** Se midieron el
31-ago, o sea **antes de `G1`** (el lazo al hilo BP, 6-sep) y **antes del arreglo del 9-sep** (el
bombeo dejó de dormir a nivel de SO). La ficha describía un sistema que ya no existe.

### La herramienta, primero — porque por eso llevaba diez días sin tocarse

Medir esto exigía **que alguien pulsara un botón y mirase**. Ahora no: `samples/GuiLatSuelo.bp` +
el guión de clics del host (`BPVM_GUI_SCRIPT`, con `wait`/`click x y`) + `BPVM_GUI_SHOT_MS` para que
termine solo. Sin manos, repetible.

```
BPVM_GUI_SCRIPT=clics.txt BPVM_GUI_SHOT_MS=8000 build/bpgenvm-c.exe GuiLatSuelo.mod
```

### ❌ `LV_DEF_REFR_PERIOD` NO es la causa — prueba de desplazamiento

La ficha lo señalaba desde el 31-ago (*«explica buena parte, no todo»*). Se movió el número y la
latencia **no se enteró**:

| `LV_DEF_REFR_PERIOD` | latencias medidas |
|---|---|
| 33 (por defecto) | 17 · 21 · 16 · 16 |
| **66** | 13 · 17 · 15 · 16 · 15 |
| **10** | 18 · 19 · 17 · 16 · 15 |

Y el segundo candidato, **nuestro** tope de ocio (`BPVM_GUI_OCIO_MAX_MS`), tampoco: con **2** y con
**40** salen los mismos 15-18 ms.

### 🔑 Y cuando NINGUNA palanca mueve la medida, el sospechoso es el INSTRUMENTO

```
sleep(1) real = 16-17 ms          <- en este PC
sleep(5) real = 16-17 ms
```

Son los **15,6 ms del tick del planificador de Windows**. O sea que **el «suelo de 17 ms» del host
es el PC, no el producto** — y por eso los dos experimentos salían planos: estaban midiendo el tick
de Windows, no LVGL ni nuestro lazo. Los dos eran válidos de método y **ciegos de resolución**.

⚠️ **Consecuencia práctica, y es la que hay que retener: en el PC NO se puede medir latencia por
debajo de ~20 ms.** Cualquier medida de este tipo en el host, por encima de ese suelo o no vale.
`samples/GuiLatSuelo.bp` sigue sirviendo —para latencias grandes, como los 400 ms de antes de
`G1`—, pero lleva ese límite escrito.

### ⏭️ Lo que queda, y dónde

El suelo **sólo se puede medir en placa**, donde el tick es de 1 ms (SysTick del STM32) en vez de
15,6. Y hay una hipótesis concreta que probar allí, con lo que sabemos hoy:

📐 **Puede que los 48 ms ya no existan.** El camino de hoy es: `raise` → el hilo del GUI termina su
vuelta → `sleep(ocio)` → **eso ES la frontera de quantum** y el planificador inyecta el handler. Con
un tick de 1 ms eso deberían ser milisegundos, no 48. Los 48 se midieron con el lazo **dentro del
hilo principal** y con el bombeo durmiendo a nivel de SO — las dos cosas cambiadas desde entonces.

⏭️ **La medida**: `GuiLatSuelo.bp` en la **Discovery** (o el P4), pulsando de verdad. Necesita mano
humana porque el guión de clics es del host (SDL). Si sale del orden de milisegundos, esta pieza se
cierra sola; si sigue en decenas, entonces sí hay algo que buscar y el terreno ya está despejado de
los dos sospechosos falsos.


### ✅ 10-sep — EL SUELO, MEDIDO EN PLACA: **48 ms → 7 ms**. FICHA CERRADA.

```
Discovery, 31-ago   suelo = 48 ms     (lazo en el hilo principal; el bombeo dormia a nivel de SO)
Discovery, 10-sep   suelo =  7 ms     (misma placa, firmware de hoy)   lat= 7 · 7 · 7 · 7 · 7, cola=0
```

**Y no hizo falta arreglar nada más: se lo llevó por delante el cambio del 9-sep.** El bombeo dejó
de dormir y devuelve el ocio; el lazo BP lo duerme con `sleep(ocio)`, y **ese `sleep` ES la frontera
de quantum** donde el planificador entrega el evento. Con `BPVM_GUI_OCIO_MAX_MS = 10`, la espera
media son ~5-7 ms. Medido, no deducido.

### 🔑 El modelo, que es lo que hay que llevarse (y no estaba escrito)

**La latencia de un evento BP no es un coste del sistema de eventos: es el tiempo hasta la siguiente
frontera del hilo que lo atiende.** Medido en la C6, con un programa sin GUI:

| lo que hace el hilo tras el `raise` | latencia |
|---|---|
| `sleep(200)` | **200 ms** |
| 20 × `sleep(10)` | **10 ms** |

Así que el «suelo de 48 ms» nunca fue una constante misteriosa: era **el pulso del propio lazo del
GUI**, que antes iba al ritmo de LVGL (~33 ms) y ahora va a 10.

📌 **Y de ahí sale la cifra que faltaba: el techo de eventos por segundo de un hilo es su número de
fronteras por segundo.** El lazo del GUI drena **uno por vuelta**, o sea ~100/s. Si algo dispara más
rápido —un slider arrastrado, un sensor—, la cola crece; `cola=` en el sample es el número que lo
delata. *(Y si se desborda, hoy se pierde en silencio: `#489`.)*

### ❌ Dos sospechosos descartados por el camino

- **`LV_DEF_REFR_PERIOD`**: prueba de desplazamiento en host con 33 / 66 / 10 → **la latencia no se
  entera**. Llevaba desde el 31-ago señalado en esta ficha.
- **Nuestro `BPVM_GUI_OCIO_MAX_MS`** en host: con 2 y con 40, lo mismo.

⚠️ **Los dos salían planos por la misma razón, y es la lección**: en el PC, `sleep(1)` **dura 16-17
ms** (el tick del planificador de Windows, 15,6). **En el host NO se puede medir latencia por debajo
de ~20 ms.** Los dos experimentos eran válidos de método y **ciegos de resolución**. La medida
buena sale en placa, donde el tick es de 1 ms.

### 📋 La ficha, entera

| | | |
|---|---|---|
| 1 | el **yield** entre cuantos | ✅ 31-ago |
| 2 | el wire atendido mientras la VM calcula | ✅ `A1` (el hilo `io`) |
| 3 | la carrera del estado de salida | ✅ 5-sep |
| 4 | el **quantum en opcodes** | ✅ resuelto de hecho el 9-sep: el hilo del GUI se aparta solo |
| 5 | el **suelo** | ✅ **10-sep: 48 → 7 ms**, medido en placa |
| 6 | la prioridad del `io` del P4 | ➡️ separada como `#485`, **cerrada** el 9-sep |
| — | el `strcmp` del bucle del GUI | ➡️ separado como `#486`, **cerrado** el 9-sep |

**Cierra la queja original de Eduardo** (*«en las ESP32 se utilizan todos los recursos y se queda
todo bloqueado; los eventos pasan pero mucho más lentos»*), con los números de las dos mitades: el
hambre de los demás hilos **2136 ms → 1 ms** (9-sep) y la latencia de un evento **48 → 7 ms**.

### 🧪 Y la herramienta, que es lo que faltaba para poder cerrarla

- **`samples/GuiLatSuelo.bp`** — mide el suelo. En placa con un dedo; en host, sin manos, con el
  guión de clics (`BPVM_GUI_SCRIPT`) y `BPVM_GUI_SHOT_MS`.
- **`bpgenvm-c/tools/wire_serie.py`** — gana `put`, `puts` (por trozos, `#294`) y `run`. Sin esto no
  se puede llevar un `.mod` a una placa sin abrir el IDE.

### ⚠️ Dos cosas que salieron de las placas y NO son de esta ficha

- **La P4 no tiene pantalla operativa**: `GuiColorDemo` construye el árbol y termina en 480 ms, y en
  el log **no hay ni una línea** de panel, DSI ni LVGL. Que no haya pantalla **no lo dice nadie** —
  el usuario ve un programa que corre y no pinta. El panel sale del ENV y esta placa no lo tiene.
- **La zona de packs de la P4 lleva un `Gui` RANCIO que tapa a `/lib/Gui.mod`**: el programa correcto
  no enlaza (*«lib 'Gui' presente pero no exporta 'Gui.start'»*). Es el desfase de stdlib de siempre,
  ahora en un pack. Ese error, al menos, **sí habla**.

#### ✅ `#461` — los paths reservaban tamaño FIJO: **20 KB recuperados en el STM32 y 7,5 en el ESP32** (cerrada 31-ago)

**Eduardo, al hilo de `#456`:** *«Los path no deberían reservar espacios fijos. Si la
mayoría de las entradas son de 16 caracteres y puede haber una de 128, ponerlas todas al
tamaño mayor es un derroche de RAM que no tenemos. Claramente, la longitud ha de ser
variable.»*

### La medida

Longitud **real** de los paths de una placa (`/lib/*.mod`, 27 ficheros): **media 13,3 ·
máximo 20**. Y lo que se reserva para guardarlos:

| buffer | dónde | tamaño |
|---|---|---|
| `names[96][64]` en `static dir_snapshot_t snap` (`fs_lfs_esp32.c:236`) | **.bss del ESP32** | **6144 B** |
| `pending[16][64]` del mismo fichero | .bss | 1024 B |
| `sizes[96]` + `isdir[96]` | .bss | 480 B |
| `pending[16][64]` del común (`bpvm_repl.c:317`) | .bss | 1024 B |

**7,5 KB de RAM estática en el listado del ESP32 para ~1,5 KB de contenido real: 80% de
desperdicio** — y en la familia donde más escasea, la misma bolsa de la que sale la tabla
de handles (`#430`).

### Y ya sabemos cómo se arregla, porque se hizo hoy mismo

Es **exactamente** el patrón de `#440`/`#449`: `bpvm_symbol_t` tenía un `char name[128]`
por símbolo —59 KB vivos, 99 KB de pico al crecer— y pasó a un **pool de cadenas con
offsets** (`name_off` en `vm->sym_pool`): **20 KB vivos, 28 KB de pico**. Mismo remedio:

```c
static char     pool[N];                 /* nombres pegados, terminados en 0 */
static uint16_t off[LIST_MAX_ENTRIES];   /* dónde empieza cada uno */
```

Con 13,3 de media (+1 del NUL) 96 entradas caben en ~1,4 KB. Un pool de **2 KB** las
guarda **y además admite un nombre de 200 caracteres**, que hoy no cabría: se gastan ~4 KB
menos *y* sube el techo. Es la parte bonita — el tamaño fijo pagaba de más y daba de menos
a la vez.

⚠️ **Lo que hay que hacer bien**: con pool, quedarse corto tiene **dos motivos** (sin
ranuras / sin pool) y **los dos tienen que contar en el `omitted`** del reply, que ya
existe desde `#425`. Un listado incompleto que no lo diga es peor que un tope bajo.

🔗 **Empalma con `LIST`, el último verbo de `U3`.** Hoy el ESP32 tiene su propio listado y
el común otro; unificarlos ya estaba pendiente por el desglose por raíz, y ahora hay una
segunda razón: **la copia del ESP32 arrastra 7,5 KB de desperdicio que el común no tiene**.
Conviene hacer las dos cosas de una vez y no tocar ese camino dos veces.

📌 **Lo que NO alcanza esta ficha, para no exagerarla**: los seis `char path[64]` de la pila
del REPL. Sólo vive uno a la vez —un comando cada vez—, así que ahí no hay ×N que ahorrar;
su problema es el TOPE, y ése es `#456`.

### ✅ CERRADA — y el arreglo fue BORRAR, no encoger (31-ago)

Al migrar `LIST` (`U3.23`) se destapó lo que de verdad pasaba: **esos buffers ya no servían
para listar**. Censados los llamadores, a los tres recorridos planos de familia les quedaba
**uno solo: `fs_file_count`**. O sea que guardaban los nombres de todos los ficheros **para
devolver un número**.

Y contar no necesita los nombres: el callback corre bajo el cerrojo del FS y ahí **incrementar
es seguro** — lo que no lo era, y por eso existía el snapshot, era *emitir al wire*. Lo único
que hay que recordar son los **directorios** por visitar, porque descender desde dentro del
callback reentraría el cerrojo.

🔧 **Una implementación en la fachada, tres borrados**: `bpvm_fs_count_files()`
(`src/fs_facade.c`), con la cola de directorios **en un pool** —los nombres pegados, no en
ranuras de 64— y **en la pila**, 352 B que viven lo que dura la cuenta.

| familia | qué tenía | medido |
|---|---|---|
| **STM32** | `s_snap_names[192][64]` + tamaños ≈ **13 KB** de `.bss`, un índice persistente | **`.bss` 645.066 → 624.346 = −20.720 B**, y `text` −5.664 |
| **ESP32-C3** | `names[96][64]` en un `static dir_snapshot_t` | **DRAM 108.538 → 100.882 = −7.656 B** |
| **ESP32-S3 / P4** | el mismo fichero que el C3 | mismo ahorro (no medido aparte) |
| **Pico** | igual, pero sobre `bpvm_scratch_take` | ya no costaba `.bss`; se simplifica igual |

*(Las cifras del STM32 incluyen también lo que se llevaron `U3.22` y `U3.23`, que entraron en
la misma tanda.)*

⚠️ **Un borrado mío que se llevó algo vivo, y cómo se cazó**: el primer script eliminó
`dirlist_t`/`dirlist_cb` del STM32 porque parecían parte del listado — y los usa `clear_lib()`,
que sigue vivo. **Se revirtió el fichero con `git checkout` y se rehízo quirúrgico**, borrando
sólo lo censado. Es la segunda vez hoy que un borrado por patrón se pasa de largo; la regla
que sale: *revertir y rehacer sale más barato que remendar lo que el patrón se llevó*.

✅ **Verificado**: paridad **38 PASS / 0 FAIL / 0 SKIP**, y las **cinco imágenes** construidas
(Pico, S3, P4, C3, STM32 Nucleo).

#### ✅ `#456` — un `path` largo se TRUNCA en silencio y la operación dice OK (abierta 30-ago · **RESUELTA 9-sep**)

Salió mirando los límites de longitud al migrar el `PUT` (`U3.21`). **Medido**, no
deducido — programita en el host contra `json_min.c`:

```
pedido : 70 caracteres
copiado: 63  -> '/app/xxxxxxxx…xxx' (63)
¿el llamador se entera? json_get_str<0 = NO
```

`json_get_str` **trunca por contrato** (está escrito en `json_min.h`: *«Si dst_size es
insuficiente, trunca»*), y devuelve la longitud copiada. Los seis sitios del REPL común
que leen un path comprueban `< 0`, que sólo detecta *«no viene»* — nunca *«no cabe»*.

🔴 **El modo de fallo es el peor de los tres**: no es un error, ni un cuelgue. Es
**operar sobre OTRO fichero y contestar que todo fue bien**. Un `PUT` escribe en el
nombre recortado; un `DEL` borraría el recortado si existiera.

Alcance: `PUT`, `PUT_BEGIN`, `GET`, `DEL`, `STAT`, `MKDIR` (`path`) y `RENAME`
(`from`/`to`). El `confirm` del `FORMAT` **falla seguro** y no cuenta: truncado deja de
coincidir con el valor esperado.

Probabilidad hoy: baja — el IDE manda nombres planos y el tope son 63 caracteres. Pero
es exactamente la categoría que aquí no se acepta: **error silencioso**.

### 📐 La SD cambia la pregunta (Eduardo, 31-ago)

*«Aquí hay que pensar en las SD: 64 es razonable para la flash interna, pero para las SD se
puede quedar pequeño.»* Medidos los topes reales de los dos backends:

| | tope de nombre |
|---|---|
| **FatFs** (la SD) — `ffconf.h` | `FF_MAX_LFN` = **255** |
| **littlefs** (la flash interna) — `lfs.h` | `LFS_NAME_MAX` = **255** |
| **nuestro `char path[64]`** | **63 útiles** |

📌 **O sea que el 64 no lo impone ningún sistema de ficheros: es un tope NUESTRO**, cuatro
veces por debajo de lo que ambos backends admiten. Y en una SD los nombres **no los
ponemos nosotros** — fotos, música o logs de otro aparato ya vienen largos. La pregunta
deja de ser sólo *«¿avisamos al truncar?»* y pasa a ser también *«¿cuánto tiene que caber?»*.

⚖️ **Subirlo NO es gratis, y no todo cuesta lo mismo:**

| buffer | dónde vive | hoy | a 128 | a 256 |
|---|---|---|---|---|
| los seis `char path[64]` | **pila**, y sólo uno vivo a la vez (un comando cada vez) | 64 B | 128 B | 256 B |
| `pending[16][REPL_LIST_NAME_MAX]` del listado | **estático (.bss)**, ×16 | **1 KB** | 2 KB | **4 KB** |
| `esc[REPL_LIST_NAME_MAX * 2]` | pila | 128 B | 256 B | 512 B |

El de la pila es calderilla; **el del listado es ×16 y es RAM estática**, que en el S3/C6/C3
es justo lo que escasea (`#430`: la tabla de handles sale de esa misma bolsa). Lo razonable
parece **desacoplar los dos números** —el path de UN comando no tiene por qué medir lo
mismo que cada entrada de la cola del listado— en vez de subir uno y pagarlo ×16.

🔑 **Y esto no depende del número que se elija**: cualquiera que se ponga, alguien lo
pasará. Lo que hay que arreglar es que **al pasarlo se entere**. El tope es una política;
el silencio es el bug.

⏩ **Lo que hay que decidir**: lo barato es que `json_get_str` devuelva `-2` al truncar.
Como los seis sitios ya comprueban `< 0`, **todos empiezan a rechazar sin tocarlos** — a
cambio de contestar *«falta path»* cuando lo correcto sería *«path demasiado largo»*. Si
se quiere el mensaje bueno, hay que distinguir `-1` de `-2` en cada uno. Toca las cinco
implementaciones del wire, así que va en su propio paso y con placa.

### ✅ RESUELTO EL 9-sep — no se eligió un número: se quitó la copia (`fc8641f4`, `1057ba14`, `8daa01eb`)

**La pregunta la reformuló Eduardo en cuatro palabras**: *«¿Por qué el path tiene un tope?»* Y
medido, la respuesta es que **no lo imponía nada**:

| quién podría imponerlo | qué dice |
|---|---|
| littlefs (flash interna) | `LFS_NAME_MAX` = 255 **por nombre**, sin tope de path completo |
| FatFs (la SD) | `FF_MAX_LFN` = 255, ídem |
| la línea del wire | **2048** (`bpvm_wire_v1.h:45`) |
| el parser JSON | **no copia** — es in-place, el path ya está en RAM |
| `char path[64]` | **63 útiles** ← el único que cortaba |

🔑 **Y no era «un tope»: eran SEIS números distintos para la misma cosa**, ninguno con motivo.

```
desde un programa BP    511    builtins.c, 13 sitios
por el wire comun        63    bpvm_repl.c, 11 buffers de 64 (y dos de 96)
el RUN de Pico y ESP32   39    FS_NAME_LEN
el RUN del STM32         63
el simulador            191    PATH_MAX_SIM
cada entrada del listado 63    REPL_LIST_NAME_MAX
```

Un fichero que el programa abría sin problema no se podía subir ni borrar desde el IDE.

**Eduardo: *«Quita topes. Si tiene que haber un tope lo hablamos y vemos cuál es la razón.»***

### Lo que se hizo

- **`json_str_inplace()`** — desescapa **en sitio**, dentro de la línea que ya trae el path, y
  devuelve un puntero. Sin copia ⇒ **sin tope**. Es lo que usan ahora `DEL`, `STAT` (+`name`/`base`),
  `RENAME`, `GET`, `PUT`, `PUT_BEGIN`, `LIST_DIR`, `MKDIR` y `RMDIR` del común, y el `RUN` de las
  cuatro familias — con su `arg`, que tenía otro tope callado de 127.
- **`json_get_str()` devuelve `-2` al truncar.** Los llamadores ya comprobaban `< 0`, así que los
  que siguen copiando **pasan a rechazar sin tocarlos**. El desescape, además, queda en **un solo
  sitio** (`json_desescapa`) en vez de dos tablas que se podían desincronizar.
- **El lado BP tenía el mismo bug y no estaba en la ficha**: `read_bp_string` trunca igual y **los
  13 sitios ignoraban lo que devuelve**. `read_bp_path()` mide antes de copiar y el builtin lanza un
  `RuntimeError` atrapable.
- **El resolvedor iba por detrás**: `main_path[FS_NAME_LEN]` = 40. El wire aceptaba 205 y el `RUN`
  los recortaba a 39. Al día.
- **El listado DECLARA lo que no le cabe.** Su tope sí tiene motivo —es RAM estática ×16 (`#461`)—
  así que no se sube: se cuenta en `omitted` (`#425`) y se anota en el log. Emitir un nombre
  recortado es el peor de los tres desenlaces: el IDE se lo cree y luego pide algo que no existe.

### El único tope que queda, y su razón

**`BPVM_FS_PATH_MAX` = 256**, en `include/bpvm_fs.h`, y **sólo** para los paths que hay que
**guardar** (la sesión de `PUT` por trozos, que sobrevive entre mensajes) o **construir** (la salida
del resolvedor, el basedir que se salva y se restaura, el path que se copia del heap de la VM).

✅ **El número, confirmado por Eduardo el 9-sep**: *«256 está bien.»*

1. **Por qué hay número.** No se puede quitar del todo: la cadena BP vive en el heap **sin terminador** y las
   dos APIs de FS piden un `const char*` terminado en NUL, o sea que ahí hay que copiar. 256 sale de
   los 255 del FS; el techo lo pone la **pila de 8 KB de la tarea `main`** del S3/C3/C6, donde
   `repl_stat` tiene dos de estos vivos. Con 512 serían 1 KB de esa pila.
2. ⏭️ **Lo único que queda vivo de aquí: la paridad.** miVM **no tiene tope** (en Java es un
   `String`), así que por encima de 256 las dos VMs discrepan: miVM abre el fichero y la VM-C
   lanza. Antes discrepaban igual, a partir de 512 y **en silencio**, así que esto ya es mejor;
   pero para cerrarlo de verdad el tope tiene que ser **del lenguaje** y comprobarlo miVM también.
   **Es la misma discusión de `#481`** y se decide allí, no aquí.

### La prueba, con su control

En `tools/sim_smoke.py`: `PUT` de un path de 205 caracteres, `STAT`/`GET` del path entero, `RENAME`
entre dos largos, `DEL`, el `LIST` que lo declara, y el `PUT_BEGIN` por encima del tope que queda.
**Contra el código de ayer da 3 FAIL**, y el que importa es *«el nombre RECORTADO no existe»*: o sea
que el `PUT` escribía en otro fichero y contestaba OK, que es exactamente el modo de fallo de la
ficha. Compilan las cinco familias.

⚠️ **Dos rojos ANTERIORES encontrados de paso** (comprobado que salen igual sin este cambio):
- **`make test-listtrunc` tiene el CONTROL en rojo**: *«16 carpetas → 15 entradas»* y
  *«omitted=0 (dice 1)»*. El instrumento de `#425` está midiendo mal por debajo del tope. Sin ficha.
- **`make test-fspos`, `test-listdir` y `test-sd` NO ENLAZAN** (`bpvm_fs_log_fail`, `bpvm_out`). Esto
  **confirma por ejecución** el hallazgo de `#473` que decía *«un build con fachada + backend host no
  enlaza; hoy hay 5 objetivos de make rojos por esto»*.

#### ✅ `#455` — el `PUT` del común dejó de crear las carpetas que faltaban (cerrada 30-ago)

**Cómo salió**: comparando línea a línea el `handle_put` del ESP32 contra el del común
**antes de borrarlo** — que es el gesto que ya salvó el `server_name` en `U3.19`. Salió
UNA diferencia, y no estaba en el ESP32: estaba en el común.

| quién escribe | qué hacía |
|---|---|
| `fs_put` de cada familia (`fs_lfs_pico.c`, `fs_lfs_stm32.c`, `fs_lfs_esp32.c`) | `ensure_parent_dirs` → `mkdir -p` del directorio del destino |
| `repl_put` del común (desde `U3.12`) | `bpvm_fs_write` a pelo |

Y `bpvm_fs_write` **hace bien** en no crearlas: se comporta como `fopen`, y ése es el
contrato que el lenguaje expone — lo fija `test_fs_lfs` con un assert explícito
(*«write sin padres → -1»*). El que cambió de significado fue el **PUT del wire**, que
no es un `fopen` sino «guarda este fichero ahí».

🔴 **O sea que la Pico y el STM32 lo perdieron el 27-ago** (`467a3ced`, `U3.12`), en
silencio y sin que nadie lo notara. Y no se notó por una razón concreta: `/sys`, `/lib`
y `/app` **se crean al montar**, así que subir a un directorio que ya existe —que es
todo lo que el IDE hace hoy— sigue funcionando. Sólo muerde subiendo a una carpeta
nueva.

✅ **Arreglado en el común** (`crear_dirs_padre` en `bpvm_repl.c`, llamado por `PUT` y
por `PUT_BEGIN`), donde **lo recuperan las tres familias a la vez**. No es una mejora de
paso: es devolver lo que la familia de referencia tenía, que es justo lo que manda la
regla de la casa del fichero.

📌 **Lo que enseña, y van cuatro veces en dos días**: *el arreglo existe en un camino y
falta en su gemelo*. Aquí con un matiz nuevo y peor — el gemelo sano era el **de la
familia que aún no había migrado**, o sea que la migración iba a BORRAR la última copia
buena. La comparación antes de borrar es lo único que lo caza; el verde de la placa no,
porque el camino roto no se ejecuta nunca.

✅ **Verificado en el P4 el 30-ago** (subir y bajar un fichero de ~300 KB) — pero sólo en que **no rompe el caso normal**: el destino fue `/app`, que existe desde el montaje, o sea que el `mkdir` nuevo fue un no-op. **La capacidad que devuelve sigue sin ejercitarse.**

⚠️ **Sin test de host**: `bpvm_repl.c` no lo tiene (el simulador `bpvm-sim` trae su
**propio** `handle_put` y no enlaza el común, así que `sim-smoke` da verde sin tocar
este código). Es exactamente el hueco que el arnés de V7 (`#444`) tiene que tapar, y de
momento queda anotado en vez de disimulado.

#### ✅ `#454` — el timeout del wire medía lo que no debía (cerrada 30-ago · `01d3eb22`)

**Síntoma**: abrir `ESTADO.md` (120.763 B) desde el árbol del IDE daba
`timeout esperando respuesta a 'GET'`. Un fichero de 9 KB sí se abría.

**Y mi primer diagnóstico fue FALSO.** Calculé que 120.763 B a 115200 baud son
10,5 s contra un plazo de 10,0 y concluí «no cabe por el cable». La aritmética
estaba bien; la conclusión, mal. **Lo tumbó una observación de Eduardo**: *«cuando
grabamos un pack, éste se graba aunque tarde bastante»* — un pack de ~580 KB
tarda ~50 s y **funciona**.

O sea que el cable mueve 580 KB sin problema. Lo que cambia es **la forma**:

```
SUBIR   PUT_BEGIN / PUT_DATA / PUT_END   ->  N peticiones, cada una con SU plazo
BAJAR   GET                              ->  UNA peticion, UN plazo para todo
```

📌 **El criterio, de Eduardo, y es el arreglo**: *«el timeout se suele configurar
por trama, no por tiempo total del traspaso de un archivo; sirve para detectar no
conexiones o conexiones que se han caído.»* Un plazo fijo para una transferencia
sin cota está mal por construcción: no es que 10 s sea poco, es que **no puede
haber un número**.

⏩ `sendRequest` esperaba con `future.get(timeoutMs)`. Ahora espera a rodajas y
sólo se rinde tras `timeoutMs` **sin recibir un byte**. Quita la clase entera:
cualquier verbo, cualquier tamaño, cualquier velocidad — y detecta mejor el cable
muerto, que es para lo que sirve.

⚠️ **El latido se sella en el STREAM, no en el bucle lector.** El cuerpo de un
`bulk` se lee dentro de `recvBulk`, que no vuelve hasta tenerlo entero: sellando
por frame, los 120 KB seguirían sin dar señales de vida y saltaría igual.

#### ✅ `#453` — el `GET` abría el fichero una vez por cada 256 B (cerrada 30-ago · `18c0c42f`)

Salió persiguiendo `#454` y es un problema **distinto y real**, aunque no fuera la
causa de aquel síntoma. `repl_get` troceaba de 256 en 256 B con `bpvm_fs_read_at`,
y `read_at` recibe el **path**: cada trozo abría el fichero, hacía `seek` desde el
principio, leía y cerraba. 472 aperturas para 120 KB.

🔁 **Es LA MISMA ENFERMEDAD que #398/#424, en el otro llamador.** Entonces se
añadió `crc32` al interfaz de backend porque era el caso cronometrado (refresco
del árbol 6953 → 155 ms) y **el bucle del `GET` se quedó como estaba**. La
cabecera incluso llevaba escrito el principio general: *«con el fichero abierto
UNA vez y leído en secuencia, eso desaparece; el backend es el único que puede
hacerlo, la fachada no tiene descriptores»*.

⏩ Así que no hubo que diseñar nada: `read_stream(path, cb, user)` como op
**opcional al final** del backend, igual que `crc32` — el que no la traiga la deja
a `NULL` y la fachada cae al bucle de siempre. Implementada en littlefs con una
apertura y el lock tomado **una vez** en lugar de 472.

✅ **Con su oráculo, que es lo que #398 hizo bien**: `test_fs_lfs` comprueba que
`read_stream` entrega los **mismos bytes** que el bucle viejo, en los tamaños donde
estos bucles se rompen (255/256/257, 511/512/513, 0 y 5001). Aquí lo que no puede
cambiar no es un número sino el **contenido**: unos bytes distintos darían un
fichero corrupto **sin error**, porque el `bulk` anunciado seguiría cuadrando.
Comprobado además que **sabe ver rojo** (se corrompió un byte a propósito). 58 →
119 asserts.

#### ⏸️ `#452` — CINCO verbos del wire que ningún cliente manda (APLAZADA al script de test, 31-ago)

Salió de una pregunta de Eduardo al probar `U3.17` —*«df / mem ?»*— y de tirar del hilo.
En la línea de comandos del IDE `df` y `mem` son **el mismo comando**, y no manda `DF`:
`SerialBackend.mem()` se lo sintetiza con `INFO` + `LIST`.

Censados los 40 verbos que el IDE emite (`sendRequest`) contra los 20 del REPL común:

| verbo | ¿lo manda algún cliente? |
|---|---|
| `PING` · `DF` · `FORMAT` · `RENAME` · `RMDIR` | ❌ **cero ocurrencias en todo el IDE** |
| los otros 15 | sí |

⚠️ **Por qué importa, y no es "código de más":**
1. **No se pueden probar.** Ningún gesto del IDE los alcanza — se vio migrándolos: `U3.13`
   dio al ESP32 `FORMAT`/`RENAME`/`RMDIR` y **no hubo forma de verificarlo por uso**, hubo
   que cambiar el plan y verificar con el paso siguiente.
2. **Y por eso se pudren en silencio**: si uno se rompe, nadie se entera. Es superficie
   viva del protocolo que nadie ejercita.
3. **El arnés de V7 tampoco los cubrirá**, porque su modelo es «lo que hace el IDE». Van a
   necesitar un cliente de pruebas que hable el wire a pelo — lo cual, de paso, es la
   respuesta a *cómo* se prueban.

### 🧭 EL CRITERIO, y por qué la ficha se APLAZA (Eduardo, 31-ago)

*«Con los comandos tenemos 3 casos de uso: 1 — el usuario, 2 — el IDE internamente, 3 — el
futuro script para automatizar los test en placa. Creo que podemos dejar para cuando
abordemos el script la revisión de TODOS los verbos.»*

📌 **El cambio de enfoque importa**: la pregunta deja de ser «¿sobra este verbo?» y pasa a
ser «¿a cuál de los tres consumidores sirve?». Y el tercero **todavía no existe**, así que
juzgarlos ahora sería decidir con un consumidor de menos a la vista — justo el error de
censar sin tener delante a todos los que llaman.

⏸️ **Se aparca hasta el script (`#444`)**, y entonces se repasan **los 29 verbos**, no sólo
estos cinco. Lo medido el 30/31-ago se conserva porque será el punto de partida:

| verbo | ¿usuario? | ¿IDE? | ¿script? |
|---|---|---|---|
| **`FORMAT`** | **sí, y hoy falta**: tras flashear hay que formatear el FS y **no hay forma desde el IDE** (ni botón ni comando de consola) | no lo manda | seguro |
| **`RENAME`** | sí — renombrar en el árbol es algo que al usuario le falta | no lo manda | probable |
| **`RMDIR`** | sí — el IDE ya manda `MKDIR`, falta el par | no lo manda | probable |
| **`PING`** | no | no lo manda | **sí**: saber si la placa sigue viva entre test y test |
| **`DF`** | sí, pero **duplicado**: la consola tiene `df` y `SerialBackend.mem()` lo sintetiza con `INFO` + `LIST` — dos viajes y una cuenta para lo que el firmware da en uno | no lo manda | probable |

⏩ **Lo que decía la ficha antes de aplazarse**: o el IDE los usa, o se declaran
explícitamente «del protocolo, sin cliente todavía» y el arnés los cubre aparte. Lo que no
vale es dejarlos como están: implementados, documentados y sin tocar.

#### ✅ U2 — el transporte (CERRADO 26-ago; sus cuatro pasos, verificados en placa)

> Los cuatro pasos están ✅ y verificados en las tres familias con wire. Lo único que
> quedó vivo se movió a `U3` a propósito (los ~22 replies que el REPL arma a mano).

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

#### 🏁 U3 — el REPL (CERRADO 31-ago)

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
porque **el instrumento que lo vigilaría no llega hasta ahí**.

### ✅ DECIDIDO (Eduardo, 26-ago): SE QUEDA COMO ESTÁ. No se implementa en la VM-C.

Su razonamiento, que reencuadra la ficha entera: *«lo normal es que un programa en un micro
no pida entradas por input, igual que los prints tampoco sirven de nada si el programa no
está conectado a una consola. ¿Entonces para qué sirven el print y los inputs? Son
herramientas de DEPURACIÓN básicamente. Fíjate que están antes que el debugger, así que si
quieres que un programa se pare le pones un input y te hace la función de un break. Yo lo
dejaría, no molesta.»*

📐 **Y el coste, medido, apoya la decisión**: `input()` no es «un builtin más». En miVM es
un **estado de hilo** — el `tc` pasa a `BLOCKED_PROMPT`, sale de la cola de ejecución, y
otro hilo deposita el resultado en su pila cuando llega la respuesta
(`VirtualMachine.java:5023`). Portarlo a la VM-C significa un estado nuevo en el
planificador más un despertar desde el wire, con el `poll` entre quanta de por medio: toca
el scheduler, no la tabla de builtins.

⚠️ **La asimetría que queda, dicha para que conste**: en placa `print` SÍ llega por el wire
(sale como evento OUTPUT), así que las dos herramientas de depuración están en la misma
situación —sólo sirven conectado— pero sólo una funciona. Se acepta a sabiendas: el
debugger de verdad ya existe y hace mejor lo que el `input()` hacía de apaño.

📌 **Lo que esto cambia en el registro**: deja de ser una divergencia *pendiente* y pasa a
ser una **limitación conocida y decidida**. Su sitio, por tanto, es `docs/PENDIENTES.md`
(limitaciones de cara al usuario), no la lista de trabajo.

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

##### ✅ `U3.5`–`U3.6` — el STM32 COMPLETO: HELLO y el grupo PUT (27-ago · `c926ce1a`, `e8d2a129`)

**HELLO.** La forma del saludo al común; lo propio son tres cadenas de cintura
(`server_name`, `server_build`, `capabilities`). `server_build` se queda FUERA del común a
propósito: un `__DATE__` en `src/bpvm_repl.c` sería la fecha de un fichero que casi nunca se
recompila — o sea una fecha que no identifica la imagen, que ya costó un diagnóstico falso.

Dos cosas aparecieron al medir y no estaban en el plan:
- El STM32 tenía **TRES llamantes** de HELLO, no uno: los otros dos son los *poll* de
  «ocupado ejecutando» (durante un RUN sólo se atienden HELLO y KILL).
- Meter tres campos **corrió un puesto** el initializer de la cintura. Lo cazó el
  compilador, pero es el bug que ya nos comimos al quitar un método de una base de la
  stdlib → la cintura pasa a **inicializadores designados**.

**PUT** (de un tirón) y **PUT_BEGIN/DATA/END** (streaming). El scratch y la política de
persistencia van en la cintura porque NO son iguales: el buffer tiene restricciones propias
por familia, y el `after_put` del STM32 persiste salvo bajo `/lib/` mientras la Pico no hace
nada (littlefs ya commitea al cerrar).

El orden de `PUT_DATA` se conserva y ahora está EXPLICADO, porque es lo contrario de lo que
parece natural: **primero se lee el bulk, después se valida la sesión**. Validar antes es lo
que uno escribiría, y el síntoma de hacerlo es malísimo — la respuesta sale correcta
(`NO_SESSION`) y lo único que pasa es que los bytes anunciados se quedan en el cable, así
que el mensaje siguiente se lee desde la mitad de los datos.

**Y se termina #329.** El motivo real del fallo de FS ya se guardaba, pero el STM32 lo
tiraba: respondiera lo que respondiera littlefs, contestaba SIEMPRE «NO_SPACE / FS lleno».
La clasificación y la línea de log bajan a la capa de littlefs (estaban DUPLICADAS en dos
familias, idénticas carácter por carácter) y salen como enum para que quien contesta al IDE
no arrastre `lfs.h`.

✅ **En placa (Nucleo)**: conexión + INFO; subida y ejecución de JsonDemo (dos ficheros a
`/app`); y `Stdlib.pack` de **180 KB por streaming**, que ejercita BEGIN/DATA/END de verdad.

**Marcador U3: 26 de 29.** Al STM32 sólo le quedan RUN/KILL/RESET, que se quedan en la
familia **por diseño**.

##### ✅ `U3.7`–`U3.12` — la Pico entera, en seis pasos (27-ago · `97050ef9`…`467a3ced`)

La Pico se migra **al revés que el STM32**: el dispatch común va al FINAL de su cadena, no
al principio. Así lo que la placa siga implementando gana y lo que ya no, cae al común — se
puede ir grupo a grupo con la placa funcionando entre paso y paso.

| paso | qué | lo que apareció al comparar |
|---|---|---|
| `U3.7` | PING/TIME/LOG_DUMP/LOG_CLEAR | el sink del log del común sustituía los caracteres de control por un **espacio** (byte perdido); la Pico emitía `\uXXXX`. Gana el rico |
| `U3.8` | DEL/STAT/GET/MKDIR/RMDIR/RENAME | el `DEL` común juntaba «no está» y «está y no se puede borrar» en *«no existe»*, que en el segundo caso es falso |
| `U3.9` | LIST | la Pico **gana** (no pide la zona de scratch para todo el listado, y desaparece el tope de entradas por directorio) y **pierde** una: emite dentro del callback de la fachada, o sea con el lock del FS cogido. Si molesta, el arreglo es del COMÚN y para las tres |
| `U3.10` | cintura + HELLO/DF/FORMAT/SAVE | el `SAVE` de la Pico daba `durationMs` y el común no. Allí mide un no-op (sale 0), pero en el STM32 —que sí escribe flash— significa algo y no existía. Ahora lo emite el común con `bpvm_platform_now_ms`, que ya era primitiva común |
| `U3.11` | INFO | 18 comunes + **11 propios** por `info_extra`. El buffer del común sube de 900 a 1024 (medido en banco: 679 B, 345 libres) |
| `U3.12` | grupo PUT | ver abajo |

**Lo de `U3.12` merece su párrafo.** El despachador de la Pico pre-leía el bulk ANTES de
saber qué verbo era, y el común lo lee dentro del handler: dejarlo así sería leerlo dos
veces. La salida no es quitar la pre-lectura (`PACK_BURN_DATA` la necesita) sino **adelantar
la extracción del `type`**, que no toca el cable, y saltársela sólo para los verbos PUT.

Y al reordenar salió a la luz **un agujero de verdad**: la puerta de arranque de H9 —«sin FS
montado, nada de ficheros»— contesta `NOT_READY` y **vuelve sin tragarse el bulk**. En la
Pico lo tapaba la pre-lectura; en el STM32 no lo tapaba nada y llevaba ahí desde `U3.5`.
Arreglado una vez, en el común: `bpvm_repl_drain_bulk()` público, y las dos puertas lo
llaman antes de contestar.

**La cintura de la Pico se registró A MEDIAS a propósito** (`info` y `put_buf` llegaban en
sus pasos). Para que eso fuera seguro **por construcción** y no por el orden de la cadena, el
común dejó de fiarse: `falta_pieza()` hace que cada verbo compruebe lo que necesita y
conteste UNSUPPORTED diciendo **cuál** falta.

✅ **Cada paso verificado en placa el mismo día**, uno a uno (norma de Eduardo: *«cada cambio
lo hemos de verificar en placa; aquí nos importa más la seguridad que la velocidad»*).

**Marcador U3: 26 de 29 en dos familias.** A la Pico le quedan RUN/KILL/RESET (por diseño) y
lo suyo propio: BOOTSEL, SD_INFO, SD_MOUNT, LIST_DIR, PROMPT_RESPONSE. Su `repl_v1.c` pasa
de **2066 a ~1300 líneas**.

⏭️ **Queda de U3**: la familia ESP32 (el S3 y el P4 **comparten** `repl_esp32.c`, así que van
juntas). Y ojo al migrar: su despachador también pre-lee el bulk.

##### ✅ `U3.13`–`U3.21` — el ESP32 entero, en nueve pasos (30-31-ago)

`repl_esp32.c` de 1360 a ~1210 líneas. Migrados y **verificados en placa** (S3 gesto a gesto
el 30-ago, P4 el 31): `DEL` `STAT` `MKDIR` `GET` `PING` `TIME` `LOG_DUMP` `LOG_CLEAR` `SAVE`
`DF` `INFO` `HELLO` `FORMAT` `RENAME` `RMDIR`, y el **grupo `PUT`** completo (`U3.21`).

Dos hallazgos que salieron de comparar línea a línea **antes de borrar**, y que se habrían
perdido migrando a lo bruto: el `server_name` del P4 (`U3.19`) y el `mkdir` de los directorios
padre (`#455`), que la Pico y el STM32 ya habían perdido sin ruido en `U3.12`.

##### 🎯 `U3` — LO QUE QUEDA, medido el 31-ago

Contados los verbos que cada familia **despacha a un handler propio** (no las listas de la
puerta del boot, que no son handlers):

| familia | propios | cuáles |
|---|---|---|
| **STM32** | **0** | — migrado del todo |
| **ESP32** (S3+P4) | 4 | `LIST` `LIST_DIR` · *`RUN` `RESET`* |
| **Pico** | 6 | `LIST_DIR` · *`RUN` `RESET` `BOOTSEL` `SD_INFO` `SD_MOUNT`* |

*En cursiva, los que se quedan **por diseño**: `RUN`/`RESET` tocan la sesión de la VM y la
placa, y `BOOTSEL`/`SD_*` son hardware que sólo tiene el RP2350.*

##### ✅ `U3.22` — `LIST_DIR` al común, y el STM32 lo GANA (31-ago)

El envoltorio que quedaba en la Pico (21 líneas) y en el ESP32 (17), al común. **Comparados
línea a línea antes de borrar** —el gesto que hoy ya salvó el `server_name` y el `mkdir` de
`#455`— y salieron **idénticos salvo el transporte**:

| | sink | cierre de línea |
|---|---|---|
| Pico | `fwrite(stdout)` | `fputc('
')` + `fflush` |
| ESP32 | `wire_v1_send_bulk` | `wire_v1_send_line("", 0)` |

Y en el Pico **esas dos cosas SON** `wire_v1_send_bulk` y `wire_v1_send_line` (el mismo
`fwrite`+`fflush`), así que usar el contrato deja **los mismos bytes en el cable** — y de
paso el Pico gana el `tx_lock` que su atajo se saltaba.

🎁 **El STM32 gana `LIST_DIR`, y a coste CERO de build**: `bpvm_listdir.o` ya estaba en su
binario —su proyecto compila `src/` por carpetas— compilado y sin que nadie lo llamara. Es
el mismo patrón que `bpvm_pack_mount` en el P4 (ver [[arreglo-que-no-viaja-entre-familias]]):
código que viaja en el binario y al que nadie encaminó el verbo.

📖 **Y la documentación se mudó con el código**, que es donde sirve: el bloque que explica
por qué `LIST` y `LIST_DIR` son verbos DISTINTOS (y por qué el listado se hace en dos
tiempos, para no retener el cerrojo del FS mientras el host lee) vivía en el Pico y ahora
está en `bpvm_repl.c`.

⚠️ **Un susto propio, y la lección**: la expresión con la que quité la línea del despachador
era **demasiado glotona** y se llevó también una línea de la condición multilínea de la
puerta del boot (`is_fs`), que contiene el mismo `strcmp`. Lo cazó el compilador al instante
(`'is_fs' undeclared`), pero el aviso es real: *borrar por patrón en un fichero grande
necesita mirar el `git diff` después, no antes*.

✅ **Verificado**: paridad **38 PASS / 0 FAIL / 0 SKIP** y **las cinco imágenes**
reconstruidas — Pico, S3, P4, **C3** y STM32 Nucleo, todas posteriores al cambio.

##### ✅ `U3.23` — `LIST` al común, **subiendo el desglose** (31-ago). **`U3` CERRADO.**

El último verbo. Y se migró **al revés de como habría sido cómodo**: en vez de dejar caer el
desglose por raíz que sólo el ESP32 imprimía, **subió al común** — que era la condición que
lo tenía parado desde el 30-ago.

```
antes (común):  ls: 25 ent (0 dirs omitidos)
antes (ESP32):  ls: 25 ent en 160 ms | app:11/50ms lib:14/64ms
ahora (TODAS):  ls: 25 ent en 160 ms | app:11/50ms lib:14/64ms (0 dirs omitidos)
```

📌 **Cambia el instrumento, no el dato.** El ESP32 cronometraba **por entrada** (dos lecturas
de reloj cada una, y lo decía en su comentario); el común mide **por directorio**, que es la
unidad que su recorrido ya tiene. Sale la misma línea, más barata — y además **atribuye mejor
los subdirectorios**, porque el común desciende y el recorrido plano del ESP32 no.

🎁 **La Pico y el STM32 GANAN el desglose.** Ese es el sentido de la regla que salió de
`#455`: *unificar hacia abajo es una regresión disfrazada de limpieza*.

⚠️ **Y otra vez el heredoc**: al escribir el código, `'\0'` llegó al fichero como un byte NUL
de verdad — tres veces, y el `.c` pasó a ser binario para `grep`. Es la trampa que ya está
fichada en [[escribir-ficheros-sin-destruirlos]] y hoy ha vuelto a morder. Reparado con un
script en fichero, no con un heredoc.

✅ **Verificado**: paridad **38 PASS / 0 FAIL / 0 SKIP** y las **cinco imágenes** (Pico, S3,
P4, C3 y STM32 Nucleo) reconstruidas.

### 🐛 Y en placa NO funcionó — `[Placa ERROR] timeout esperando respuesta a 'LIST'`

Bug mío, y de manual. El contexto del listado se inicializaba **campo a campo**:

```c
repl_list_ctx_t c;
c.id = id; c.first = 1; c.omitidas = 0; c.emitidas = 0;
c.pending = pending; c.tail = 0;      /* ← `nraices` NO */
```

Al añadir los casilleros del desglose (`nraices`, `raiz*`) **la inicialización parcial los
dejó con basura de pila**, y `repl_list_apunta` usa `nraices` como tope de bucle: recorría
memoria ajena y el verbo no contestaba. Arreglado con `memset`, que además protege del
próximo campo que se añada.

📌 **Y lo que más duele: el código que sustituí llevaba la lección escrita.** El
`handle_list` del ESP32 hacía `memset` con este comentario: *«memset y no `= { 1 }`: con la
struct ya no de un solo campo, la inicialización parcial saca
`-Wmissing-field-initializers`»*. Estaba dicho, en el fichero que borré, y no lo traje.
Comparar línea a línea sirve para no perder FUNCIONALIDAD; esto enseña que también hay que
mirar **por qué** el código de origen estaba escrito como estaba.

#### ✅ Verificado en la P4 (31-ago)

`JsonDemo` corre entero y sale `exit 0`. **El log demuestra el `LIST` aunque no lo nombre**:

```
[Explorer] /app/Json.mod ya en FS (24059 bytes, contenido idéntico), salto PUT
[Explorer] /lib/Core.mod ya en FS (13111 bytes, contenido idéntico), salto PUT
[Explorer] /app/JsonDemo.mod ya en FS (2505 bytes, contenido idéntico), salto PUT
```

Esos tres «salto PUT» son **comparaciones contra el listado del Explorer** — el mismo `LIST`
que ayer daba `[Placa ERROR] timeout`. Si no hubiera contestado, no habría con qué comparar.

🎁 Y de paso confirma `#463` en placa: `Core.mod` va a **`/lib`** y no a `/app`, o sea que ya
no se crea el override que no había pedido nadie.


### 🔴 Por qué llegó a la placa: **el simulador es el quinto consumidor y no ha migrado**

El arnés de paridad corre PROGRAMAS; `LIST` es un verbo del wire y ahí no llega. Pero existe
la herramienta que lo habría cazado en segundos y en el host: **`bpvm-sim` + `sim_smoke.py`,
que ejercita `LIST` por el wire** … con **su propio `handle_list`** (`tools/bpvm_sim.c:939`),
no con el común.

Y estaba anotado: el `Makefile` dice *«los CUATRO firmwares (+ el simulador cuando migre)»*.
**Migrar el simulador al REPL común convierte un viaje a la placa en un `make sim-smoke`** —
y es la misma forma del hallazgo de `#455` (el sim trae su propio `handle_put`). Es lo que
`#444` necesita para no ser sólo un deseo.

##### ✅ `U3.24` — **el simulador migra al REPL común** (31-ago): un viaje a la placa se convierte en `make sim-smoke`

Consecuencia directa del bug de arriba, y hecho el mismo día. `bpvm-sim` tenía **21 verbos
propios**: un REPL entero paralelo, en el host, que `sim_smoke.py` ejercita por el wire. O sea
que la herramienta capaz de cazar un fallo del REPL común en segundos **existía… y miraba otro
código**.

| | antes | ahora |
|---|---|---|
| verbos propios del sim | **21** | **4** (`RUN` `KILL` `RESET` `STATE`+`ENV_*`/`PART_*`/`PACK_*`) |
| `tools/bpvm_sim.c` | 964 líneas | **840** (−124) |
| verbos que el sim **no tenía** | — | **gana `LIST_DIR` y `RMDIR`** |

La cintura del sim son **13 campos** (`bpvm_repl_ops_t`), de los cuales sólo dos son de verdad
suyos: el silicio de mentira que se pide por línea de comandos (`--mem`, `--psram`, `--flash`,
`--screen`) y el formateo, que aquí es *cerrar la imagen, borrar el fichero y volver a montar*.

🎯 **El control — y esto es lo que da valor al paso.** Con el arnés ya migrado, **se volvió a
meter el bug de `U3.23`** (el `memset` por la inicialización campo a campo) y se corrió:

```
$ make sim-smoke                       # con el bug DENTRO
  ok  : LIST tras formatear → vacío                     ← pasa igual (FS vacío = 0 entradas)
  FAIL: LIST → ruta COMPLETA y tamaño correcto
  FAIL: LIST → trae crc (el IDE lo usa para saltarse PUTs)
  [status=FAIL]
```

Lo que ayer costó **flashear una P4 y verlo dar timeout**, hoy son **12 segundos en el host**.
Y de paso enseña algo del arnés: la comprobación que *parece* cubrir `LIST` (la del FS recién
formateado) **pasa con el bug vivo**; la que lo caza es la que tiene contenido. Un camino
ejecutado no es un camino probado — [[test-fuerza-el-caso-que-el-programa-real-no-da]].

📌 **Se borran los handlers propios, no se sombrean.** Un handler de la familia que gane al
común deja el común **sin ejercitar**, que es exactamente el agujero por el que se coló esto.

⚠️ **Un cambio de comportamiento, a propósito**: `TIME` sin `epochSec` contestaba `OK` en el
sim y ahora dice `INVALID_PARAM`. Es la unificación ya acordada en `U3.1` («gana la Pico: el
error se DICE»), que las cuatro placas llevan desde el 26-ago; el sim era el que se había
quedado atrás.

✅ **Verificado**: `sim-smoke` **25/25**, `boardsim-smoke` verde, paridad **38 PASS / 0 FAIL /
0 SKIP**, y `LIST_DIR`/`RMDIR` probados a mano contra el sim en marcha.

##### ✅ `U3.25` — **`MKDIR` y `RMDIR` dejaban de mentir** (31-ago). *Lo encontró el arnés recién migrado, a la primera.*

Al ampliar `sim_smoke.py` con los verbos que el común tiene y el arnés no miraba, dos
comprobaciones salieron en rojo — **y eran de verdad**:

```
FAIL: LIST_DIR → nombres SUELTOS y marca los directorios   ← MKDIR /app/sub decía OK
                                                              y el directorio NO estaba
FAIL: RMDIR repetido → NOT_FOUND                            ← RMDIR decía OK SIEMPRE:
                                                              sobre lo que no existe,
                                                              y sobre un dir CON cosas dentro
```

Los dos verbos eran **stubs que contestaban OK sin tocar nada**, con este argumento escrito:
*«en un FS plano con `/` como namespace no hay nodos de directorio»*. Eso fue cierto —del FS
en RAM del STM32— y **dejó de serlo hace tiempo**: los cuatro backends de la fachada
(`fs_lfs.c`, `fs_host.c`, `fs_fat.c`) tienen `mkdir` recursivo, `rmdir` y `isdir` de verdad.

📌 **Y el común ya lo sabía.** `crear_dirs_padre()` —el arreglo de `#455`— llama a
`bpvm_fs_mkdir` en **cada PUT**. O sea: el REPL creaba directorios al subir un fichero, y
contestaba «hecho, nada» cuando se le pedía uno explícitamente. Dos funciones de distancia.

🎯 **Cómo salió: la regresión de `#455`, otra vez.** El simulador **tenía un `MKDIR` de
verdad** (llamaba a `bpvm_fs_mkdir` y reportaba el fallo); el común, el stub. Migrarlo sin
mirar fue *unificar hacia abajo* — exactamente lo que Eduardo señaló el 30-ago: **«quedarnos
con lo bueno y no unificar a lo peor»**. La diferencia con la vez anterior es que esta vez
**el arnés lo dijo a los diez minutos**, no una placa tres días después.

Ahora:

| | antes | ahora |
|---|---|---|
| `MKDIR` | OK sin hacer nada | crea (recursivo, ok-si-existe) o error con nombre |
| `RMDIR` sobre lo que no está | **OK** | `NOT_FOUND` |
| `RMDIR` de un dir con contenido | **OK** (y no borraba) | `NOT_EMPTY` |
| `RMDIR` de un dir vacío | OK (y no borraba) | OK, y **borrado** |

`RMDIR` mira con `bpvm_fs_isdir` + un conteo antes de borrar, porque `bpvm_fs_rmdir` devuelve
`-1` para los dos fallos y **no son el mismo** — el mismo gesto que `repl_del` ya hacía.

⚠️ **Riesgo mirado antes de tocar**: `BpvmClient.mkdir()` existe en el IDE y **no lo llama
nadie** (`grep`), así que nada dependía del OK falso.

✅ **Verificado**: `sim-smoke` **40/40** (15 comprobaciones nuevas), `boardsim-smoke` verde,
paridad **38 PASS / 0 FAIL / 0 SKIP**, y las **seis** imágenes reconstruidas y frescas —
Pico, S3, P4, C3, STM32 Nucleo y STM32 Discovery.

⏭️ Falta **probarlo en placa**: en el host el backend es `fs_lfs` sobre imagen, y en el micro
es el mismo fichero sobre flash real, pero eso hay que verlo. Gesto: `MKDIR /app/x`, `LIST_DIR
/app`, `RMDIR /app/x`.

---

### 🏁 `U3` — CERRADO (31-ago)

Verbos que cada familia sigue despachando por su cuenta, y **todos por diseño**:

| familia | propios | por qué |
|---|---|---|
| **STM32** | **0** | — |
| **ESP32** | `RUN` `RESET` | tocan la sesión de la VM y la placa |
| **Pico** | `RUN` `RESET` `BOOTSEL` `SD_INFO` `SD_MOUNT` | + hardware que sólo tiene el RP2350 |
| **sim** | `RUN` `RESET` `KILL` `STATE` | + la gestión de placa, que ya es núcleo compartido (`U3.24`) |

**Con esto la parte de comunicaciones de la unificación queda cerrada** — que es lo que
Eduardo señaló como condición para que el C3 y el C6 salgan «casi gratis».

⏭️ Queda `#461` (los buffers de tamaño fijo del listado), que toca este mismo camino y por
eso se hace **a continuación**: migrar `LIST` deja el `dir_snapshot_t` de 6 KB del ESP32
sirviendo **sólo para contar ficheros** (`fs_file_count`), que es su único llamador restante.

<details><summary>Lo que quedaba antes de U3.22/U3.23</summary>

**Así que lo pendiente de verdad es UN verbo:**

1. 🟡 **`LIST`** — sólo le queda al ESP32 (la Pico y el STM32 ya usan el común). **Parado a
   propósito**: el ESP32 imprime un desglose por raíz (`ls: 25 ent en 160 ms | app:11/50ms
   lib:14/64ms`) que el común no tiene, y migrarlo tal cual sería **unificar hacia abajo** —
   el error que `#455` demostró que se comete solo. Lo correcto es subir el desglose, y
   entonces la Pico y el STM32 lo **ganan**.
2. ✅ ~~**`LIST_DIR`**~~ — **hecho en `U3.22`** (arriba). Y el STM32, que no lo tenía, lo
   ganó de regalo.

🔗 **Y conviene hacerlo junto con `#461`**: los buffers de tamaño fijo del listado (7,5 KB de
`.bss` en el ESP32 para ~1,5 KB de nombres) están **en ese mismo camino**. Tocarlo dos veces
sería trabajar de más.

</details>

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

#### ✅ U4 — la stdlib embebida (CERRADO el 3-sep con `U4.1`)

16 ficheros `*_mod.c` en la Pico frente a **uno** en ESP32 (4.845 líneas) y STM32 (4.837).
No es código distinto: es el mismo dato empaquetado de dos maneras.
⚠️ **Es un GENERADO.** Se toca el generador, nunca el resultado — ver
[[generado-parcheado-a-mano]].

##### 📐 `U4.0` — el censo y la forma (3-sep): UN generador, ficheros de sólo DATOS, y el bucle en el común

**Medido.** Tres generadores casi iguales (`regen_pico_mods.sh` 47 líneas, `regen_stm32_mods.sh`
81, `regen_esp32_mods.sh` 92) que emiten el mismo dato en dos formatos:

| familia | fichero(s) generado(s) | líneas | bytes | módulos |
|---|---|---|---|---|
| ESP32 (S3, C3, P4) | `esp32/common/esp32_mods.c` | 4 223 | 307 622 | 14 |
| STM32 | `stm32/port/stm32_mods.c` | 4 213 | 307 033 | 14 |
| Pico/Metro | 16 × `pico/*_mod.c` + `hello_mod.c` + `embedded_mods.h` | 4 775 | 346 235 | 15 + `Hello` |

Lo que difiere de verdad entre familias es **la lista** (la Pico embebe además `Neopixel`, y el
`Hello.mod` de muestra en `/app`) y **el `put`** (el ESP32 agrupa las escrituras). Todo lo demás
—el tipo de la tabla, el bucle, la regla— ya es común desde `#466`: la regla vive en
`src/bpvm_mods.c`, pero el bucle sigue copiado en cada generado y en el `main.c` de la Pico.

**La forma:**
1. **Un generador**, `scripts/regen_mods.sh <familia> <salida> MOD… [--extra ruta=fichero]`, que
   emite UN fichero de **sólo datos**: los blobs (`static const`), la tabla
   `const bpvm_mod_embebido_t <fam>_mods[]` y `<fam>_mods_n`. **Ni una línea de código dentro
   del generado**: lo que se puso a mano en uno murió en la siguiente regeneración (`#422`).
2. **El bucle, una vez, en `src/bpvm_mods.c`**: `bpvm_mods_instalar_tabla(tabla, n, put, log)`
   → escritos. Cada familia lo llama con su `put` y su `log`; el ESP32 lo envuelve en su lote.
3. **La Pico pasa al mismo formato**: `pico/pico_mods.c` (15 módulos + `/app/Hello.mod` como
   entrada `--extra`); fuera los 17 ficheros y `embedded_mods.h`, y con ellos la tabla
   `PREINSTALL` a mano, que guardaba la longitud por dirección porque los `_len` eran `extern`.
4. `regen_all_mods.sh` llama al generador tres veces con cada lista; `regen_hello_blob.sh` se
   queda en compilar `hello.bp` y pasárselo a la Pico como `--extra`.

**Cómo se comprueba:** los blobs regenerados son byte-idénticos a los de hoy (mismo `bpstdlib`:
se diffea la parte `xxd`), `test-mods` más un caso del bucle en host, los seis builds, y en placa
el arranque dice lo mismo que hoy (la Pico 2 acaba de verificar la regla con esta misma tabla).

##### ✅ `U4.1` — HECHO (3-sep): un generador, tres ficheros de sólo datos, el bucle en el común

- `scripts/regen_mods.sh <familia> <dir> MOD… [--extra ruta=fichero]` emite `<fam>_mods.c` (blobs
  `static const` + la tabla `bpvm_mod_embebido_t` con `sizeof`, no la variable `_len` de xxd) y
  `<fam>_mods.h` (los `extern`). Los tres `regen_*_mods.sh` quedan en su lista; el maestro los
  llama; `regen_hello_blob.sh` desaparece (el `Hello.mod` de la Pico lo compila su wrapper y entra
  como `--extra /app/Hello.mod=…`).
- `bpvm_mods_instalar_tabla(tabla, n, put, log)` en `src/bpvm_mods.c`: el bucle, una vez. Cada
  familia sólo pone su `put` y su `log`: el ESP32 en `board_mgr_esp32.c` (con su lote de autosave;
  los tres `main.c` siguen llamando a `esp32_mods_install()`), el STM32 en su punto de llamada, la
  Pico en el suyo — fuera la tabla `PREINSTALL` a mano y sus 16 ficheros + `hello_mod.c` +
  `embedded_mods.h` (`pico_mods.c` los sustituye).
- **Los blobs salieron byte-idénticos** (huella de las líneas hex: ESP32 y STM32 `0ebdfd5cf77d`
  antes y después; la Pico `c1418a9a2f1f` en sus 15; el `Hello` recompilado da las mismas 323
  líneas). `test-mods` 18/18 con el caso del bucle; `make`, `sim-smoke` 45/45.
- **Seis builds** con el `pico_mods.c` nuevo en CMake (los CMake del ESP32 y el linked folder del
  STM32 no cambian): Pico 18:54, S3/C3/P4 19:02, Nucleo 19:01 (text 251 432), Discovery 19:02.
- **En la Pico 2**: grabada la imagen de `U4.1`, el arranque no toca nada (`/lib` idéntico: ni una
  línea `lib:`), `fs: 33 ficheros` (el 33.º es el `MathRango.mod` del Run de Eduardo), y el `STAT`
  por nombre contesta lo mismo (`/lib/Math.mod`, MOD7, crc 3686083642).

📌 Ya no queda código en ningún generado: lo que se ponga a mano ahí muere en la siguiente
regeneración, y ahora no hay nada que poner. **`U4` cerrado.**

#### ✅ U5 — la tabla de handles (CERRADO el 3-sep con `U5.1`)

Hoy no tiene fichero ni cabecera: vive repartida por `bpvm.c`, `builtins.c`,
`bpvm_aot_helpers.c`, `bpvm_dbg_wire.c` y `bpvm_util.c`. Está *unificada por omisión*, no
por diseño, y por eso `#432` (dónde debe vivir y de qué tamaño) no se puede ni plantear.

##### 📐 `U5.0` — el censo (3-sep): la tabla ya está en un sitio; le falta el módulo, no la unificación

**Contando consumidores, no leyendo código** — quién toca los campos `handle_*` de `bpvm_t`:

| fichero | toques | qué |
|---|---|---|
| `src/heap.c` | 45 | registro, baja, crecimiento (dentro del bloque, hacia abajo, `#451`), barrido del GC, la presión (`#430`) |
| `include/bpvm_internal.h` | 20 | los diez campos (detrás del prefijo congelado) + los inline `bpvm_ref_dead` / deref |
| `src/bpvm.c` | 11 | init a cero, y la free-list en `bpvm_free` |
| `test/main.c` | 6 | `--handlecap` (`#430`) |
| `bpvm_util.c` · `bpvm_aot_helpers.c` · `test_smp_handles.c` | 1–2 | un diagnóstico, un comentario, la carrera SMP |

Los que la ficha de agosto citaba (`builtins.c`, `bpvm_dbg_wire.c`) **no la tocan**: usan refs a
través de los inline. Y lo que `#432` preguntaba —dónde vive, de qué tamaño— **lo contestó `#451`**:
dentro del bloque de la VM, entre heap y pilas, creciendo hacia abajo hasta chocar con el heap
(límite físico, sin tope adivinado). `#432` se cierra por referencia.

**Lo que sí falta, y es `U5`**: un módulo con nombre. `src/bpvm_handles.c` + `include/bpvm_handles.h`
con lo que hoy es un tramo de `heap.c` (`handle_slots_por_heap`, `handle_table_grow`,
`bpvm_handle_register`, `handle_kill_idx`, `bpvm_handle_kill`, `gc_table_sweep_phase`) y los inline
de deref; los diez campos, en un `bpvm_handles_t` dentro de `bpvm_t` **detrás del prefijo** — y
esto es lo que había que comprobar antes de moverlos: el código AOT de los `.mdn` no lee esos
campos por offset, lo hace todo por `helpers->…` (`AotCEmitter`: `newarray_`, `array_store_`,
`string_length`, `call_bp_i`… ninguno `handle_*`), así que moverlos **no cambia el ABI del `.mdn`**;
el guardián del prefijo (`memory` en 0, `aot_helpers` detrás) sigue siendo el único contrato.
Verificación: paridad 38/0/0 y `test_smp_handles` en host, los seis builds, y una placa (la Pico
2, la del margen más justo).

##### ✅ `U5.1` — HECHO (3-sep): la tabla de handles tiene módulo

- **`include/bpvm_handles.h`**: el tipo `bpvm_handles_t` (los diez campos, con la historia de cada
  uno junto al campo), las constantes (`BPVM_HANDLE_TAG`, `BPVM_HANDLE_CAP_MAX`) y la API:
  `bpvm_handles_init/destroy`, `bpvm_handle_register/kill`, `bpvm_handles_grow` (era el
  `handle_table_grow` estático), `bpvm_handles_gc_sweep` (era `gc_table_sweep_phase`),
  `bpvm_set_handle_cap_max`, `bpvm_uaf_report`.
- **`src/bpvm_handles.c`** (310 líneas): el tramo de `heap.c` tal cual, con sus comentarios de
  `#430`, `#449` y `#451` —que son la historia de por qué la tabla está dentro del bloque—, más
  el init y el destroy que hacía `bpvm.c` a mano. `heap.c` pasa de 1 301 a 1 001 líneas y se queda
  con sus dos llamadas (la fase 6 del GC y la puerta de `heap_alloc`).
- **`bpvm_t`**: los diez `handle_*` → un `bpvm_handles_t handles;` detrás del prefijo congelado;
  todos los accesos (`vm->handle_x` → `vm->handles.x`) renombrados en `heap.c`, `bpvm.c` y los
  inline del header (`bpref_deref`, `bpvm_ref_dead` y `bpref_regen` se quedan en
  `bpvm_internal.h`: son el modelo de refs, no la tabla). Ni `builtins.c` ni `bpvm_dbg_wire.c`
  cambian: no la tocaban.
- **Verificado**: host `make`; `test_smp_handles` a 4 y 8 hilos, 0 corrupciones; `test-mem`,
  `test-mods`, `sim-smoke` 45/45; **paridad 38/0/0**; los **seis builds** con el `.c` nuevo dado de
  alta en los cinco sitios (Nucleo text +160 B, bss idéntico; Discovery bss idéntico); y en la
  **Pico 2** (grabada, `MathRango` por el wire): **29 líneas byte-idénticas al host**, INFO
  273736/92160 como antes, sin reinicio.

📌 **`U5` cerrado, y con él la serie de unificación (U1–U6).** `#432` se cierra por referencia.

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

- ✅ *(cerrada el 3-sep por referencia: la contestó `#451` — dentro del bloque, hacia abajo, límite físico; ver `U5.0`)* `#432` — **¿dónde debe vivir la tabla de handles, y de qué tamaño?** Las dos
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

- (sin número) — librería `Math`: ampliar **y repasar lo que ya hay**; strings igual
  si hace falta. Ver el detalle en `L1` — de la lista original sólo quedan cuatro
  funciones, y `atan2` ya estaba hecha. 📌 Un ejemplo de lo que significa *repasar lo
  que ya hay*: `sign(integer)` y `signF(float)` son **dos nombres para una idea**,
  herencia de cuando no había sobrecarga (existe desde V4/H5.a). Antes de tocarlo hay
  que comprobar si el registro de intrínsecas sabe despachar por tipo de argumento, y
  que renombrar rompe código de usuario.

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

- ✅ ~~**🐛 [ESP32 y STM32] un módulo rancio sobrevive y NADIE lo dice — y `/app` es el punto ciego de los DOS**~~ — **CERRADA el 3-sep por `#466`** (nota al final). Encontrado el
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
  ✅ **CÓMO SE CERRÓ (3-sep, `#466`), y con la regla de Eduardo, no con el parche barato de
  arriba.** Las tres estrategias se fueron: ahora las tres familias corren el MISMO
  instalador (`bpvm_mods_sincronizar`, sobre la tabla embebida de `U4`): por cada módulo de
  la imagen, si falta en `/lib` se instala; si el de `/lib` es de MAGIC anterior, o del mismo
  MAGIC con otro tamaño/CRC, **se repone y lo dice en el log** («repuesto»); si es más nuevo,
  se respeta. Y el punto ciego de `/app` lo cubre el IDE por el otro lado: antes de subir una
  dependencia pregunta a la placa con `STAT` por nombre (resuelve proyecto → `/app` → `/lib`)
  y, si la copia que encuentra es de versión anterior o de CRC distinto, **la reemplaza donde
  está** — un `Core.mod` rancio en `/app` cae en el siguiente `Run`. Verificado en Pico 2, S3
  (13 repuestos), Nucleo, Discovery y C6. Lo único que no toca nadie: un módulo rancio en
  `/app` que ningún programa importe, que tampoco carga nadie.

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

- 🧪 `#379` — el wire se **desincroniza tras el Stop**, y sólo en unas placas.
  **→ ASIGNADA A `F1` (pruebas finales) el 9-sep, decisión de Eduardo**: *«la podemos dejar
  pendiente para las pruebas finales»*. Deja de contar como pendiente suelto — no porque se
  aplace, sino porque **lo único que le queda ES una prueba de placa**, y `F1` es la tanda
  donde las placas se conducen. La medida exacta y su herramienta están abajo.
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

  ### 🔬 9-sep — EL PASO 1, HECHO POR FIN (y con herramienta, que era lo que faltaba)

  **Idea de Eduardo:** *«A ver si se desincronizan, se puede enviar un código de sincronización tipo
  UUUUUUUU.»* Buena en general — y al mirarlo resulta que **el wire ya la tiene, y es el `\n`**:

  - **camino de líneas** (todo salvo el bulk): es `{...}\n`, y cada familia **descarta lo que no
    empiece por `{`** y reintenta al desbordar (`repl_esp32.c:1223`, `repl_v1.c:1727`). Cualquier
    basura se pierde hasta el siguiente salto de línea, o sea que **ya resincroniza en el siguiente
    mensaje**. Un patrón `UUUU` no añadiría nada aquí.
  - **camino bulk** (`PUT` + N bytes crudos): éste **sí** es donde se puede perder el framing de
    verdad — si los dos lados no coinciden en N, el receptor se come bytes crudos creyendo que son
    líneas. Es el único sitio donde un código de sincronización tendría trabajo, y hoy se cubre con
    `drain_bulk` y un caso del arnés («…y el wire sigue en sincronía después»).

  ### Lo que faltaba de verdad: ¿el device o el IDE?

  Era el **paso 1** de esta ficha y llevaba desde agosto sin hacerse, porque no había forma de
  preguntarle a la placa **sin el IDE por medio**. Ahora la hay: `bpgenvm-c/tools/wire_serie.py`, un
  cliente del wire por serie que no es el IDE. Con él, `run → stop → INFO`:

  | | ciclos | fallos | KILL | INFO |
  |---|---|---|---|---|
  | **ESP32-C3** | 8 | **0** | ~215 ms | contesta siempre |
  | **ESP32-S3** | 8 | **0** | ~233 ms | contesta siempre |

  Los `uptime` avanzan en cada ciclo, así que son **dieciséis ciclos de verdad**, no la misma
  respuesta repetida — el control que faltaba en la medida de la Metro del 17-ago.

  ⚠️ **Lo que esto NO prueba, y hay que decirlo**: ninguna de las dos placas tiene **SD**, y el caso
  de la ficha era el **P4 con la tarjeta**, donde además el Stop *colgaba*. Así que esto **no cierra
  `#379`: lo estrecha**. Descarta que haya una desincronización de base en el wire —que era la
  hipótesis del título— y deja la sospecha donde ya la puso Eduardo el 17-ago: el árbol/CRC con
  tarjeta, o sea `#398`.

  ⏭️ **La medida que queda es la misma de siempre y ahora es de dos minutos**: el P4 con la tarjeta,
  `python tools/wire_serie.py COM<n> ciclo /app/Bench.mod 8`. Si sale 8/8, la ficha se cierra como
  absorbida por `#398`; si falla, hay desincronización de verdad y entonces sí toca hablar del
  código de sincronización — en el camino bulk, que es donde vive el problema.

- ~~`#408` — medir **los dos cuellos** que se ven comparando P4 y Metro (árbol en la
  P4 / formateo en la Metro).~~ ✅ **CERRADA**: las dos mitades medidas (21 y 22-ago) —
  el detalle, en la viñeta siguiente. *(Este enunciado se leía como pendiente mientras
  la respuesta estaba dos líneas más abajo; corregido el 7-sep.)*
- ✅ **`#408` — CERRADA. La mitad de la METRO, MEDIDA el 21-ago: el formateo son ~15 s**, y es el
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

> 🧊 **CERRADA POR RETIRADA (30-ago, `#460`)**: las interfaces de módulo salen del
> lenguaje, así que este bug se va con ellas. El texto se conserva porque documenta bien
> el mecanismo y **el diagnóstico que dejó escrito resultó dudoso** — la traza del 30-ago
> imprime el `extends` que aquí se daba por ausente. Sirve de recordatorio: una explicación
> que no se vuelve a comprobar envejece igual que el código.

- ~~**🐛 [compilador] la sustitución por LSP entre interfaces de módulo NO funciona**~~ —
  ✅ **CERRADA POR RETIRADA (30-ago, `#460`)**, como dice la nota de arriba: las interfaces
  de módulo salen del lenguaje. *(La viñeta se leía como pendiente; corregido el 7-sep.)*
  El enunciado original, que se conserva porque documenta el mecanismo:
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
  ✅ **La decisión de publicación YA SE TOMÓ** (y este párrafo decía lo contrario hasta el
  30-ago): `appv1lsp.bp` y `appv2.bp` **no viajan** — viven en `samples/pendientes/`, una
  tercera carpeta creada para esto, con su `LEEME` explicando por qué no van a
  `samples/errores/` (esa es para los que fallan **a propósito**; meter aquí un bug sería
  camuflarlo) y por qué no se borran (**son la prueba de regresión**: el día que compilen,
  está arreglado). En `dist/BasicPlus-5.0-win/samples/` sólo quedan `logapi.bp` y
  `logapiv2.bp`, que compilan solos.

  🔁 **Sigue vivo — reproducido el 30-ago** con el compilador actual:
  ```
  error: 'BufferedLogger' no implementa 'com.example.LogApi' (directa o transitivamente;
  declara com.example.LogApiV2)
  ```
  📌 **Y la traza de hoy discute la hipótesis de arriba, así que conviene comprobarlo antes
  de arreglar.** Se dijo que *«falla el dato»* porque una `module interface` pura no genera
  `.mod` y la cadena no se puede recorrer. Pero en esta corrida el dato **sí está**: la
  pasada imprime `interfaz : com.example.LogApiV2 (… interface=true,
  extends=com.example.LogApi)` — o sea que el `extends` se conoce y aun así el impl se
  rechaza. Puede que el dato exista y no llegue a donde mira `implSatisfies`. **Primer
  gesto al retomarlo: mirar si `implSatisfies` recibe ese `extends` o una copia sin él**,
  en vez de dar por buena la explicación escrita.

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
- ~~🔴 **La VM-C normal NO puede correr bases de datos: hace falta placa**~~ — ✅ **CERRADA el
  6-sep**: `make sim SQLITE=1`, y un programa BasicPlus consulta una BD **en el simulador**, por
  el wire, `status: OK` en 181 ms. Ganó **el tercer camino** de los que esta misma viñeta
  proponía —el micro simulado del IDE—, y por la razón que aquí se anticipaba: el usuario prueba
  sin placa y sin binario especial. El detalle y el porqué de enlazar en vez de cargar un pack
  nativo (x86-64 es ELF64 y `NpackReloc` sólo lee ELF32), en el punto de `E1`.
  *(Se leía como 🔴 abierta mientras `E1` la daba por hecha; corregido el 7-sep.)*
  El enunciado original, que se conserva porque su análisis es el que llevó a la solución —
  abierta el
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
