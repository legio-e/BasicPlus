# BasicPlus — Traspaso entre sesiones

> **Qué es esto.** El **diario** de la sesión: qué se cerró, qué quedó a medias y
> qué riesgo acecha, por fechas. Se lee al empezar una sesión y se escribe al
> terminarla.
>
> **Lo que ya NO va aquí: el estado de las fichas.** Qué está abierto, cerrado o en
> curso vive **sólo** en `docs/FICHAS.md`. Decisión de Eduardo (17-ago): *«Estado y
> pendientes son ficheros de trabajo tuyos. Pero el que dice realmente cuál es la
> situación es Fichas.»* **Si este fichero contradice a `FICHAS.md`, manda `FICHAS.md`.**
>
> El 17-ago se midió por qué: de las **51 fichas que citaba este documento, 49 eran
> una segunda copia** de las de `FICHAS`. Y se desincronizaban — ese mismo día daba por
> pendiente un censo ya hecho y contradecía a la ficha #417. Eduardo: *«me estoy
> volviendo loco con cosas que aparecen y desaparecen»*. Las secciones que
> enumeraban fichas (en curso, riesgos, plan de cierre, próximos pasos, al cerrar)
> se borraron ese día **después de subir a `FICHAS` lo que sólo estaba aquí**: la tabla
> de hitos de V5, el detalle de los restos del árbol y la decisión sobre `dist/`.
>
> **Mapa de docs:** `docs/FICHAS.md` (la fuente de verdad) · `docs/PENDIENTES.md`
> (limitaciones del lenguaje de cara al usuario) · `docs/PHILOSOPHY.md` (el porqué).
>
> **Cómo se escribe una entrada:** concreta —ficha, fichero, prueba— y sin repetir
> el estado que ya está en `FICHAS`. Aquí va lo que PASÓ, no lo que HAY.

---

## Última sesión

## ⏭️ AL RETOMAR (29-ago) — la batería queda en 47/48, y lo único que falta es `U6`

**Lo primero de mañana**: decidir `U6`. Es lo único que bloquea el 48/48, está
diagnosticado con números y no hay nada más que investigar — sólo hacerlo. Detalle
en `#451`.

### Lo que se cerró hoy

`#440` y `#449` (los dos síntomas eran **una sola causa**: la tabla de símbolos
desbordando el margen de `malloc` y escribiéndose sobre el código de un módulo —
los bytes `43 6F 72 65` eran la cadena `"Core…"`). El arreglo: un **pool de
nombres**, 59 KB → 20, y el pico del `realloc` de 99 KB → 28. `JsonDemo` corre en
placa con salida **byte-idéntica a la VM-Java y al host**.

`#450` nuevo y cerrado: **`stack=N` (KB) en el ENV**, petición de Eduardo. El
usuario reparte el bloque entre pilas y montón; ausente = lo de siempre. Entró
donde la regla ya estaba unificada, así que fue el núcleo + una línea por familia.
Verificado en placa: `heap 267+89` → `heap 293+64`.

**La batería, de 40/48 a 47/48.** El único rojo real es `synclisttest` (`#451`).

### Lo que hay que saber para mañana

🔻 **De los tres atascos de hoy, NINGUNO fue un programa roto: los tres fueron
instrumentos que mentían.** El MPU armado antes del último enlace, el MPU
heredando regiones de la ejecución anterior, y el aviso de `#430` gastado por un
`static`. Los tres con la misma forma: **estado que sobrevive entre ejecuciones**.
Cuando algo «falla» y el andamio está puesto, sospechar del andamio primero.

📌 **Y el método que sí funcionó, dos veces: contar.** La pista de `loader.c:224`
para `#440` era excelente —explicaba el síntoma *y* la asimetría Metro/Pico 2— y
era falsa; sobrevivió dos días. Lo que la mató fue instrumentar el registro de
símbolos y **contar** (460, no los ~176 que yo había estimado). Igual con
`synclisttest`: la corrección de Eduardo *«se añaden unos cuantos prints y vemos
dónde está el problema»* dio en 10 minutos lo que yo iba a buscar construyendo
instrumentación del alocador.

### Deuda que sigue viva

- **`U6`** — la tabla de handles fuera del bloque de la VM. Ver `#451`: el objeto
  medio mide 25 B y el reparto asume 64, así que todo programa de objetos pequeños
  agota handles con el heap casi vacío.
- **El STM32 sin verificar desde `U3.6`** (`H13_PRUEBAS_V5_REPASO.md`): seis
  gestos, cinco minutos. Sigue sin hacerse.
- **La batería en las otras familias**: hoy sólo Pico 2.
- **`--smp=2` cuelga en el host** con `synclisttest` (`--smp=1` y el camino normal
  pasan). Bug real del paralelismo, sin ficha todavía; **no** afecta a la Pico,
  cuya imagen no define `BPVM_PICO_SMP_WORKERS`.
- `#446` (host con `GUI=0`) y `#441` (seis opcodes) siguen abiertas.

## ⏭️ AL RETOMAR (28-ago, noche) — la batería de V4 sobre la Pico 2, y lo que destapó

**Lo primero de mañana**: `#440` es el único rojo que queda de los ocho, y está acotado a
un offset concreto con una pista de código. Entrar por `src/loader.c:224`. Todo el detalle
—lo medido y lo **descartado con medida**, para no repetirlo— en `FICHAS.md`.

⚠️ **SON DOS PROBLEMAS Y SE MEZCLAN.** Lo vio Eduardo al cerrar: *«creo que tenemos 2
problemas diferentes y están un poco mezclados»*. La trampa es que **en la Metro
desaparecen los dos** (heap en PSRAM ⇒ la SRAM entera para `malloc`), así que su verde no
distingue entre ellos. Hay un cuadro comparándolos al principio de las fichas.

### Lo que pasó, en orden

Se corrió la batería de V4 completa sobre la Pico 2 (48 samples, decisión de Eduardo:
*«es muy raro que solamente falle JsonDemo»* — y tenía razón). **40 verdes, 8 rojos.**
Seis eran cuelgues mudos, uno `exit 6`, uno `exit 11`.

De ahí salieron cinco fichas. La secuencia que funcionó, y conviene recordarla porque es
el método de la casa: **primero hacer hablar al sistema, después arreglar.**

1. `#445` — al probar `AppTest` salió `builtin 211 no soportado`. Los tres builtins de
   `App` vivían dentro del `#ifdef BPVM_GUI`: rotos en TODA placa sin pantalla. Arreglado
   y verificado en placa.
2. `#446` — y eso destapó por qué el arnés no podía verlo: el host lleva `GUI ?= 1`, o sea
   que **siempre** compila con GUI. 535 líneas del core que ninguna placa pequeña tiene.
3. `#447` — los cuelgues eran mudos porque el SDK deja `isr_hardfault` como bucle infinito.
   Se le puso manejador propio + captura del motivo del `panic`. **No arreglaba nada**, y
   fue lo que desatascó todo: `MemT5_Gc` pasó de misterio a `PANIC: Out of memory` en una
   ejecución.
4. `#448` — y el motivo era gordo: `PICO_MALLOC_PANIC=1` hace que en la RP2350 `malloc`
   **mate en vez de devolver NULL**, anulando el OOM atrapable que V4 construyó (#355).
   Arreglado; dos de los seis cuelgues pasaron a verde y otros dos a error con nombre.
5. `#449` — la causa de fondo: `VM_SRAM_MALLOC_MARGIN` son 64 KB dimensionados en julio
   para `calloc`/`strdup` por import, y en agosto (#430) la tabla de handles pasó a salir
   del mismo `malloc` — 32 KB de golpe. El margen no se tocó. **Arreglo por decidir**, con
   tres opciones y su contrapartida en la ficha.

### El riesgo que esto deja al descubierto

`#430` se diagnosticó y se arregló **en la Metro**, donde el heap se va a la PSRAM y la
SRAM interna entera queda para `malloc`. El consumidor nuevo se estrenó justo en la placa
donde no cuesta nada, y viajó a la Pico 2 en la MISMA imagen.

Es la contrapartida de la imagen única, y no invalida el principio: es que **la Pico 2 y la
Metro tienen realidades de memoria opuestas, y la más estricta es la que menos se prueba**.
Para todo lo que toque memoria, la Pico 2 debería mandar. Aquí pasó al revés.

### Deuda que sigue pendiente

- **El STM32 lleva sin verificar desde `U3.6`** (ver el apartado de deuda en
  `H13_PRUEBAS_V5_REPASO.md`): seis gestos, cinco minutos. No se hizo hoy porque Eduardo
  priorizó, con razón, cazar el bug de la Pico primero.
- **La batería de V4 en las otras familias.** Hoy se corrió sólo en la Pico 2 (y `JsonDemo`
  en la Metro). El S3, el P4 y las dos STM32 sin pasar.
- **El UAF de `MemT5_Gc`**: con el `malloc` arreglado da
  `referencia a objeto eliminado (use-after-free)`. Un OOM no debería producir una
  referencia colgada, así que hay algo más ahí. Con `gc=0` da otra cosa
  (`ALOAD_U8: idx fuera de rango 0 (len=0)`), pero no es control limpio: sin GC el
  programa no llega igual de lejos.
- **El env de la placa cambió a mitad de investigación** (FS 1512→1024 KB, packs
  536 KB→1024 KB) sin que Eduardo lo tocara. Alguna de las imágenes flasheadas hoy
  reescribió el env. No parece afectar a lo perseguido, pero conviene saberlo.

### Herramientas nuevas, que se quedan

- La Pico **dice por qué muere**: fallos con `CFSR`/`HFSR`/`PC` y el motivo del `panic`, al
  log post-mortem.
- El **opcode desconocido** dice módulo, offset y los bytes de alrededor — que parten la
  investigación en dos (PC malo vs código pisado) sin desensamblar a mano.
- `samples/PicoA.bp` y `PicoB.bp`: el caso mínimo de `#440`, idénticos salvo un `import`.


## ⏭️ AL RETOMAR (27-ago, noche) — el repaso de V5, decidido por Eduardo

**Lo primero de mañana**: la batería de V4 completa sobre **V5 puro** (IDE + firmware +
demos del mismo paquete). Lista, condiciones de partida y lo ya sabido en
**`docs/H13_PRUEBAS_V5_REPASO.md`**. Encargo suyo, y la hipótesis es concreta: *«es muy
raro que solamente falle JsonDemo»*.

⚠️ **La placa quedó en un estado mezclado** (IDE de V4 + imagen de V5 + un FS con ~35
ficheros de las pruebas de acotación). Formatear antes de empezar; sobre esa mezcla ningún
resultado sería atribuible.

### Lo que se cerró hoy

**U3 terminado en dos familias.** Ocho pasos, `U3.5` a `U3.12`, cada uno construido en las
cuatro imágenes y **verificado en placa el mismo día** antes de pasar al siguiente. El
STM32 y la Pico quedan con 26 de 29 verbos en el común; los tres que faltan (RUN, KILL,
RESET) se quedan en la familia por diseño. `pico/repl_v1.c` pasa de 2066 a ~1300 líneas.
El detalle, en `FICHAS.md`.

Lo que más valor tuvo no fue mover código, sino **comparar antes de mover**: en cinco de los
ocho pasos apareció una divergencia real entre familias, y **la que lo hacía bien no siempre
era la misma**. Unificar «hacia el común» sin mirar habría degradado una u otra en cada
verbo. Y de camino se cerró #329 y se tapó un agujero (`bpvm_repl_drain_bulk`) que llevaba
en el STM32 desde la mañana y en la Pico lo tapaba una pre-lectura.

### La cacería que se llevó la tarde — dónde quedó

`JsonDemo` falla en la RP2350 (**#440** en `FICHAS.md`, con todo lo medido y lo descartado).
No lo trajo el trabajo de hoy: falla igual con la imagen anterior a la sesión y con la
publicada de V5. Va con la de V4 → **hueco acotado a 6-ago → 22-ago**.

Hay **tres imágenes de bisección ya construidas** (cuartiles 14, 17 y 20 de agosto) en el
scratchpad de la sesión. Si el repaso de mañana no las hace innecesarias, se reconstruyen en
un minuto con `git checkout <sha> -- bpgenvm-c/` + `ninja -C pico/build` + restaurar.
⚠️ Ese ir y venir **deja ficheros de commits viejos** que no existen en HEAD: comprobar con
`git status` y borrarlos tras verificar uno a uno que no están en HEAD.

Dos hallazgos laterales quedan anotados como **#441** (seis opcodes que la VM-Java tiene y
la VM-C no) y **#442** (el mensaje «opcode desconocido» no dice ni módulo ni offset).

### El riesgo que acecha, y no es técnico

La campaña de V4 dejó **2413 líneas** de registro; la de V5, **418**. `JsonDemo` estaba en
la de V4 con su ✅ y no aparece ni una vez en la de V5. No es que no se anote: es que
**cuando una versión hereda de otra, lo que no se re-prueba hay que saber que no se
re-probó**. Ficha **#443**.

Y una lección de método, mía, apuntada también en memoria: tuve los dos anclajes servidos
—el `.uf2` publicado de V4/V5 y la Nucleo en verde con el mismo fichero— y en vez de usarlos
me puse a **generar casos sintéticos para validar una hipótesis propia**. Eduardo lo paró
dos veces. Fabricar casos de prueba **parece** método, pero mide el eje que uno elige; si el
eje está mal, cada peldaño añade precisión sobre nada. El tell: si el siguiente paso que se
me ocurre es *construir* algo en vez de *ejecutar algo que ya existe* en un punto conocido
del pasado, estoy validando, no acotando.


## ⏭️ AL RETOMAR (26-ago, tarde) — decidir entre estas tres

Nada a medias: todo lo tocado hoy está commiteado, construido y verificado en placa.

1. **Seguir U3 — el grupo `ops`** (INFO, DF, FORMAT, SAVE). Pide diseñar
   `bpvm_repl_ops_t`, la struct de cintura: **es la pieza de diseño que le falta al
   contrato** y la que usará el resto de la migración. Eduardo avisó de que la migración
   se le hace *«un poco aburrida»*, así que conviene alternarla, no encadenarla.
2. **E1 — la vista de packs en el árbol**. Es la ficha que mordió hoy con el episodio del
   `/lib` (el fallback del pack contesta `stat`/`read` pero NO `list`), y tiene caso de
   prueba natural montado. Retorno inmediato y visible.
3. **Una de las 9 arrastradas** que el triaje dejó limpias — p.ej. `listDir` ausente en la
   VM-C, o el módulo rancio que sobrevive sin avisar.

### Lo hecho hoy (26-ago)

- **U2 paso 3 y CIERRE del hito**: el STM32 al contrato del wire; cuatro familias, un solo
  transporte. Un bug latente muerto (su `send_error` no escapaba `message`).
- **U3.1/U3.2/U3.3**: nace `bpvm_repl` (contrato + común) y el STM32 delega **11 de 29
  verbos** — meta, FS de fachada y LIST. Ganó RENAME, RMDIR y el `crc` bajo demanda; su
  refresco de árbol pasó de leer el FS entero a no leer nada (#398 nunca le había llegado).
  Cada grupo verificado en placa el mismo día.
- **Triaje de FICHAS**: de 59 arrastres, **9 vivos**. 47 al archivo, íntegros (0 líneas
  perdidas, comprobado contra git).

### Espinas abiertas, sin cerrar

- El **total del FS del STM32 baila** entre arranques (516096 ↔ 614400) y el **recuento de
  KB no cuadra** con lo que se ve. Sin explicar.
- El **P4 configurado a 16 MB** de flash con chip de 32.
- El **instalador de mods es mudo** (`fs_put` sin comprobar ni log).

---

## ⏭️ AL RETOMAR — U3, con el mapa ya hecho

El diseño está decidido y el censo profundo hecho (U3.0 + U3.0b en FICHAS; boceto en
`V6_IDEAS.md` §REPL). Lo que toca al volver, en este orden:

1. **`bpvm_repl.h`** — el contrato: dispatcher común + `bpvm_repl_ops_t` (cintura de
   familia: campos de INFO, RUN, verbos de hardware).
2. **El dispatcher común en `src/`** con los verbos del cubo fácil (PING, TIME, DEL,
   MKDIR, STAT, GET, PUT*, LOG_*, RENAME, RMDIR…), que ya solo tocan wire común + fachada
   de FS común.
3. **Primera familia: el STM32** (920 líneas, la que menos tiene) — con RUN quedándose en
   la familia hasta que los otros 19 estén verificados en placa.
4. Verificación del cordón: el IDE contra la placa tras cada tanda.

**Decisiones ya tomadas** (no reabrir): migración 1:1; los verbos con primitiva entran
gratis al compartir dispatcher; `PROMPT` no se disfraza (muerto en toda la VM-C hasta que
`IO.prompt` se implemente — hallazgo fichado en U3.0b); donde no haya hardware, error con
nombre y no «type no implementado».

---

### 26-ago (3) — U3 arranca por donde debe: midiendo

**U3.0 hecho**: la matriz verbo × familia de los tres REPL. El enunciado del hito («no
tiene contrato») exageraba: hay un **núcleo de 20 verbos idénticos en las tres**, la Pico
es el superconjunto (29), y el protocolo escrito ya documenta la mayoría. Las asimetrías
que muerden y no estaban fichadas: S3/P4/STM32 **incumplen** `RENAME`/`RMDIR`/`FORMAT`
(que están en `BPVM_WIRE_PROTOCOL.md`), y el STM32 además no tiene `PROMPT_RESPONSE`
(⇒ `input()` desde el IDE no puede contestar ahí) ni `SAVE`.

**El diseño del REPL común está BOCETADO en `V6_IDEAS.md`** (dispatcher común + cintura
`bpvm_repl_ops_t` por familia, espejo del patrón de U2), con LA PREGUNTA para Eduardo:
¿migración 1:1 y los verbos que faltan entran gratis al compartir dispatcher, o se
aprovecha para más? Y el riesgo nombrado: RUN es el hueso y se queda en la familia hasta
que los otros 19 estén verificados.

---

### 26-ago (2) — U2 CERRADO: el paso 4 era corregir el registro, no el código

`bpvm_comm.h` medido de punta a punta, y las DOS premisas escritas eran falsas (las dos
mías): no es el contrato de los transportes, y no es que «sólo 2 de 5 lo implementen» — es
el contrato de salida del **modo SMP** (opt-in: host `--smp=N`, Pico por opción de build), y
**las cinco imágenes lo enlazan**: Pico con su backend, las otras cuatro con `comm_host.c`,
tres de ellas como relleno deliberado. En single-worker (todas las placas hoy) la salida va
por `output_cb` y este contrato ni se toca.

Por el camino caí en una TERCERA teoría falsa («enlazan de chiripa por gc-sections») que
también murió por medida: `emit_text` está vivo en todas y el símbolo lo da el relleno.
Tres teorías, tres medidas, y la única que quedó en pie es la que salió de mirar los
CMakeLists y los `.elf`.

**Acción**: el MAPA en `bpvm_comm.h` + el relleno anotado en los CMakeLists de S3/P4. Cero
código, sin reflashear. **U2 queda CERRADO** — protocolo común, cable por familia, STM32
dentro, contrato SMP cartografiado.

**⏭️ Siguiente**: U3, el REPL — 4.318 líneas triplicadas sin contrato, y ya sin la excusa
del transporte: U2 le deja la frontera limpia.

---

### 26-ago — U2 paso 3: el STM32 deja de ser la cuarta forma del wire

Las 4 funciones de cable a los nombres del contrato, `send_error`/`send_cstr` borrados en
favor del común (que YA se compilaba en este build sin llamantes), `send_fatal` como
wrapper con el LED de la placa, 57 llamantes renombrados, y de regalo un bug latente
fuera: el `send_error` propio no escapaba `message`. Verificado por Eduardo en placa:
programas subidos y ejecutados, un módulo cargado y un Pack grabado (todo el camino bulk
con la firma nueva de `recv_bulk`).

**Estado de U2**: pasos 1-3 hechos y en placa (Pico, S3+P4, STM32). Queda el **paso 4**:
decidir qué hacer con `bpvm_comm.h` — el contrato VM↔comunicaciones que sólo implementan
host y Pico. Y los ~22 replies a mano del REPL del STM32 quedan PARA U3, que es su sitio.

---

### 25-ago (noche) — el STM32 cierra N1, y el `.mdn` suelto se retira

**El STM32 ejecuta AOT** — primero de su familia: `NatV7` 4/4 (con `sumaHasta` en 0 ms a
160 MHz) y `NatNew` los siete valores. Costó dos fallos: el **`.bin` del 5-ago** (el
headless regenera el `.elf` pero NO el `.bin` — dos flasheos en falso, y lo desatascó la
pregunta de Eduardo sobre el log, que llevó a dar voz al camino de fallo del RUN), y la
**frontera de coma flotante** hablando dos ABI (Pico softfp / STM32 hard, con el `.mdn`
softfp para las dos: NaN sin un error; en la Pico casaba de chiripa). Arreglo: la
convención fijada en las 28 entradas FP de la tabla (`pcs("aapcs")`, en el tipo Y la
definición — si no coinciden no compila). Verificado en el desensamblado y en placa.

**Y el `.mdn` suelto, RETIRADO** (petición de Eduardo, verificado por él en placa): la app
ya no lo genera, el Explorer limpia el rancio del device — que además habría PISADO a los
thunks embebidos —, y quedan a propósito los de packs y el barrido para deps versionadas.

**🏁 Con esto N1 (alcance V6) queda CERRADO**: formato v7, 28/30 nodos, tres familias en
placa, seis guardianes nuevos. Tuplas y try/catch → V7, como decidió Eduardo.

**⏭️ El siguiente frente natural es U3 (el REPL)** — el trabajo grande de la versión — o
las bolsas L1/E1 para sesiones cortas. U2 tiene pendientes el paso 3 (STM32, cuarta forma)
y `bpvm_comm.h`.

---

### 25-ago (tarde) — LA PARIDAD: era el ARNÉS, no la stdlib — 38/38 ✅

**El diagnóstico escrito era falso, y era mío.** Decía «desfase de slots por stdlib rancia;
regenerarla es decisión de Eduardo». Medido: la stdlib está bien, el compilador está bien y
el gate de #284 hacía su trabajo. Lo que pasaba: el arnés compilaba los 3 samples desde
`bpgenvm-c/samples/`, donde un **`Core.mod` HERMANO del 18-ago** —anterior a que
`Comparable` ganara las conversiones— tapaba a la stdlib (el resolutor mira junto al fuente
antes que en la stdlib). El módulo pedía `Integer#value#2`; luego el arnés copiaba el Core
ACTUAL (`#7`) al lado y ejecutaba. **Compilaba contra una era y ejecutaba contra otra.** El
ERROR DE LINK era la única parte del sistema diciendo la verdad.

**Tres arreglos en el arnés** (`41477504`), y los dos últimos los destapó el primero al
fallar:
1. **Hermético**: la stdlib fresca al outDir ANTES de compilar (outDir gana al hermano).
2. **El raíz por nombre de módulo**: con la stdlib en WORK, «el primer .mod que no sea
   Core» era `Adc.mod` — una librería sin main que las dos VMs fallaban IGUAL: **38/38
   verde sin ejecutar ni un sample**. El falso-PAR de manual.
3. **Cero PASS ya no es verde**: otro fallo intermedio dio 0 PASS / 38 SKIP y el resumen
   decía VERDE.

Verificado con el ARTEFACTO: `CastExt` pide `#7` y su salida es la del sample entero.
**38 PASS / 0 FAIL / 0 SKIP** — primera vez en verde real desde el 22-ago.

**Y la mina de fondo, refrescada pero no resuelta**: 11 módulos de stdlib HERMANOS en
`bpgenvm-c/samples/` (gitignorados, seis eran de MAYO). La VM-C de host los necesita en
ejecución; nadie los refresca. Copiados de `bpstdlib/` a mano; el día que la stdlib cambie
una vtable volverán a mentir. El mecanismo de los tests (`stdlibdep` del Makefile) existe —
los runs manuales no pasan por él.

---


### 25-ago — la P4: AOT y wire, y TRES fallos que sólo la placa podía enseñar

**Qué familia tocaba, y no era una preferencia**: el AOT sólo tiene dos destinos
(`NpackReloc.DESTINOS` = ARM Cortex-M33 y RISC-V32 del P4). **El S3 es Xtensa y no tiene
generador** — su `aot_funcs_stub.c` es un no-op explícito. Así que el ESP32 que toca es
**la P4**.

**Resultado**: `NatV7` **4/4 thunks** y `NatNew` **7/7, 1448 code bytes** —el mismo número
que produjo MdnPack, o sea que cuadra punta a punta— con los siete valores exactos,
ejecutando código RISC-V compilado. Y `U2.1` paso 2 (el S3 y la P4 al protocolo común del wire) verificado de
paso — `INFO` y `ls` se construyen con los 11 builders que se movieron.

**Pero costó tres fallos, y ninguno era del AOT.** Los tres eran lo mismo: **el `.mdn`
cruzaba a la placa sin que nadie comprobara que casaba con ella.** Y ninguno dio un error —
uno colgó y dos devolvieron un número:

1. El `bpvm_aot_clear()` iba **después** de cargar (habría dado 0/N). Arreglado en la Pico
   el 23-ago y nunca viajó al resto.
2. El `.mdn` se compilaba con **otra ABI y otro repertorio**: `AotBuild` no fijaba
   `-march`/`-mabi` *«porque casan por construcción»*, y el defecto del toolchain trae la
   extensión `d` — coma flotante de doble **en hardware, que el P4 no tiene**. Con doubles
   derramados a la pila, `fld`/`fsd` = instrucción ilegal → **cuelgue**.
3. Las **constantes de coma flotante** se quedaban fuera del blob: `mdn.ld` fusiona
   `.rodata*` en `.text`, y RISC-V las pone en `.srodata*`. El `auipc` apuntaba más allá
   del final. `(3.0+5.0)/2.0` = **Infinity**, un `float[]` = **1.219193E25**.

**📌 Y un cuarto que fue el que permitió ver los otros**: el log del cargador de `.mdn` era
un hook débil que **sólo la Pico implementaba**. La P4 cargaba el bloque y no había forma de
saber si sus thunks habían entrado. Se quitó el hook; ahora escribe en el log común y las
cuatro familias ven lo mismo.

**🕸️ Lo que queda: tres guardianes donde no había ninguno** — ABI y repertorio del `.o`; que
no sobreviva nada con contenido fuera de `.text`; y el nombre del símbolo contra el `.mod`
(de ayer). **Los tres probados en rojo** con el artefacto que fallaba de verdad.

📐 **La lección, y es la misma tres veces**: el `.npk` valida arquitectura y float-ABI desde
V5/H4. El `.mdn` **no validaba nada**. Dos formatos hermanos, uno con contrato y otro sin
él, y el que no lo tenía es el que se cargó tres veces en un día.

⚠️ **Dos falsos verdes míos**, los dos por mirar el log de la herramienta en vez del
artefacto: dije «las cuatro imágenes compilan `bpvm_log.c`» a partir de un `grep -l` que dio
positivo por una mención suelta, y canté «host OK» cuando el enlace había fallado y
`bpgenvm-c.exe` ni existía (la paridad se fue a 5/33 y pareció que había roto la VM).
**Comprobar el ARTEFACTO, no el log.**

⏭️ **Queda**: el STM32, y entonces retirar el `.mdn` suelto. Y una cosa que la P4 avisa sola
y no es de hoy: el proyecto está configurado a **16 MB de flash con un chip de 32** — la
zona de packs se queda en 2800 KB. Cambiarlo mueve el mapa de particiones, así que merece su
propio paso verificado.

---

### 24-ago (tarde) — U2.1: los gemelos del wire no lo eran, y el primer paso ya está en placa

Con AOT cerrado en la Pico, Eduardo propuso unificar algo de las imágenes. Salió `U2.1`, y
**los dos enunciados del hito eran falsos**:

- Decía que `pico/wire_v1.c` y `esp32/main/wire_v1.c` *«son dos programas distintos con el
  mismo nombre; el 100 % de sus líneas difieren»*. Medido palabra por palabra: las tres
  familias con wire exponen **las mismas 15 funciones** y **once son idénticas** (88 líneas
  × 3 copias). Las cuatro que difieren son **exactamente** las cuatro que tocan el cable.
  La costura ya existía y estaba limpia.
- Y `U2.2` decía que los transportes no incluyen `bpvm_comm.h`. Ese header **no es su
  contrato**: es el de VM↔comunicaciones, y lo implementan **dos de las cinco** imágenes.

**Paso 1 hecho y verificado en placa** (`bb530e3f`): las 11 + sus 4 helpers a
`src/wire_v1_proto.c`, con `include/bpvm_wire_v1.h` nombrando las dos capas. Sólo la Pico.
Eduardo confirma que el IDE funciona **incluida la subida de un Pack** — la prueba fuerte,
porque el bulk es binario y ahí un fallo de framing corrompe en vez de fallar.

🎯 **Y el gesto que hay que repetir**: antes de flashear se comparó la imagen nueva con la
vieja — **código máquina idéntico en las 15**, mismo conjunto de 1984 símbolos con los
mismos tamaños, `.text` idéntico byte a byte. Por eso el bloque se movió **tal cual**, sin
mejorar nada de paso: si se hubiera "aprovechado" para limpiar, esa comparación no existiría
y la prueba en placa habría sido un descubrimiento en vez de una confirmación.

⚠️ **Criterio de Eduardo que manda sobre todo este hito**: *«las comunicaciones son nuestro
cordón umbilical entre el PC y el micro. Cada cambio lo hemos de verificar en placa; será
un poco pesado pero aquí nos importa más la seguridad que la velocidad. En vez de 1 gran
cambio, mejor 3 o 4 pequeños.»*

---

## ⏭️ PARA MAÑANA — verificar AOT y comunicaciones en OTRA familia

Las dos cosas piden las mismas placas encendidas, así que van en el mismo viaje al banco.

**🔴 Hay un prerrequisito que se hace SIN placa, y conviene hacerlo antes de encenderla:**
el `bpvm_aot_clear()` mal colocado sigue **sin arreglar** en `esp32/main/repl_esp32.c:1001`
y `stm32/port/stm32_repl.c:548`. Está *después* de la carga, así que **borra los thunks que
el módulo acaba de registrar**. Si mañana se enciende el S3 o la P4 sin arreglarlo, el AOT
saldrá 0/N y se perderá la sesión buscando lo que ya sabemos. En la Pico se arregló el
23-ago (`repl_v1.c`).

**Lo que hay que llevar hecho:**
1. El `clear()` en las dos familias que faltan.
2. `U2.1` paso 2: borrar las copias del protocolo en `esp32/main/wire_v1.c` y
   `esp32p4/main/wire_v1_tcp.c`, dar de alta `src/wire_v1_proto.c` en sus builds, y
   **comparar el código generado antes/después** como se hizo con la Pico.
3. ~~Regenerar los cuatro artefactos nativos de SQLite + `SQLite.pack`.~~ ❌ **FALSO, y
   comprobado el 25-ago**: el `.npk` NO lleva el ABI de la tabla de helpers — se valida por
   **arquitectura y float-ABI** (`bpvm_npack.c` usa `bpvm_mdn_host_arch`/`_float_abi`, no
   `MDN_ABI_VERSION`). La prueba está en el log del P4 con la imagen de ABI 6: *«sqlite:
   pack vivo · 3.53.4 · initialize OK — motor arrancado y vfs 'bp' registrado · API
   publicada como 'SQLI', 17 simbolos»*. Confirmado **al cargar**; confirmarlo **en uso**
   pide correr un sample del ORM, que es otra cosa.

**Qué mirar en la placa** (lo mismo que hoy en la Pico, y con `log=1`, que se lee en el
arranque):
- AOT: `MDN: N/N thunks registrados` **sin ningún `skip`**, y `[mdn] 1 bloque(s) nativo(s)
  desde el propio .mod`. Los samples ya están: `samples/NatNew.bp` (7 thunks) y
  `samples/NatV7.bp` (4). ⚠️ La salida correcta **no demuestra nada** sin esas líneas.
- Wire: que el IDE conecte, liste, ejecute… **y suba un Pack**, que es el camino bulk.

📌 **Y una decisión pequeña que ahorra tiempo**: elegir UNA familia y hacerla entera
(AOT + wire), no las dos a medias — [[focus-un-kit-batch-cross-family]].

---

### 24-ago — N1.5: la `native` ya FABRICA, y el censo dejó de mentir

**Lo pedido**: *«a ver si podemos cerrar AOT»*, y a mitad de sesión Eduardo acotó: *«tuplas
las aplazamos a V7. Lo que nos queda en V6 es instanciar arrays y objetos»*.

**Hecho**: arrays de **todos** los tipos de elemento dentro de una `native`
(i32/i8/i16/i64/f64/f32 y **referencias**: `string[]`, `Clase[]`) y creación de objetos. Un
objeto no se aloca en C —el constructor es bytecode—: se cruza a la factoría
`__cls_new_<Clase>`, la ruta que `#213` ya había abierto para `throw`. Detalle completo en
`FICHAS.md` → N1.5 y N1.5b.

**📌 Lo que hay que llevarse de esta sesión no es la feature: es que TRES cosas que
parecían medidas no lo estaban — y de las tres, DOS eran errores míos de medida, no
fallos del código.** Las dos las destapó Eduardo preguntando, no una prueba.

1. **El censo del AOT medía cinco fragmentos mal escritos, no el AOT.** `ThrowStmt`
   llevaba soportado desde `#186` y lo tapaba un `throw "vaya"` que #248 no permite. El
   arnés sólo miraba errores del *parser*: lo que no compilaba salía como «no soportado».
   Arreglado el arnés, la foto pasó de «23 medidos» a **28 de 30 medidos de verdad**.
2. ~~El puente native→BP nunca había llegado a una placa.~~ **FALSO, y corregido el
   mismo día a partir de una pregunta de Eduardo** (*«lo de los literales lo vimos en V5»*).
   Yo comprobaba el `.o`; el pipeline real empaqueta el `.elf` **ENLAZADO**, y el guión
   `bpgenvm-c/aot/mdn.ld` fusiona `.rodata` dentro de `.text` — eso es `#428`, cerrado en
   V5 y verificado en la Metro. El cambio que metí por esto sobraba: revertido.
   ⚠️ El mismo error de medida había producido el «`print` no compila para ARM» del
   23-ago. **Dos límites declarados que no existían**, los dos por medir una etapa que el
   producto no tiene. La regla: si `MdnPack` se queja de un `.LC0`, la pregunta es si
   estás enlazando.
3. **Indexar un `long[]` en native leía 4 bytes donde hay 8.** No fallaba: devolvía otro
   número. El límite estaba escrito en un comentario y no lo hacía cumplir nadie.

Y de propina, uno **ajeno al AOT**: `OP_ASTORE_I16` de la VM-C ponía a cero el elemento
siguiente. Divergencia con miVM, o sea el invariante roto — cazada por un `word[4]` de tres
líneas escrito para comprobar otra cosa.

**⚙️ La herramienta que queda, que vale más que los arreglos**: `make test-aotnew`. Compila
el `.c` generado con el compilador del host y lo **EJECUTA** con `gc_bump_threshold = 1`
(GC en cada alocación). Es el tercer peldaño que faltaba —**emite / cabe / acierta**— y es
el que cazó el `long[]`. Los dos primeros ya habían dado verde.

**✅ VERIFICADO EN LA PICO esa misma tarde.** Imagen reconstruida (ABI 6) + fat-jar del IDE
reconstruido —comprobado a mano que estampa `abi_version=6`, porque un desfase ahí costó un
flasheo el 23—. El log de la placa:

```
[313579] MDN: 7/7 thunks registrados, 1172 code bytes (zero-copy)
[313579] [mdn] 1 bloque(s) nativo(s) desde el propio .mod
```

Los siete valores exactos. **La duda que había que despejar** era si esos thunks venían de
la sección del `.mod` o del `.mdn` suelto que el IDE sigue subiendo al lado: la despeja de
dónde sale el mensaje —`bpvm_load_mdn(vm, sec + p, ...)`, con `sec` dentro de la sección— y
el hecho de que el barrido del suelto entra 17 ms después sin registrar nada. No hizo falta
borrar nada.

**🔴 Y la segunda pasada, con `NatV7`, destapó que N1.1 NUNCA se había acelerado.** Dio
`MDN: skip 'NatV7.Caja_doble' rc=-2` → **3 de 4**. El `.mod` exporta el método como
`Caja.doble` y `MdnPack` pedía `Caja_doble`: reconstruía el nombre partiendo el
identificador de C, y de `Caja_doble` no se recupera `Caja.doble`. Las funciones de MÓDULO
sí acertaban (no tienen punto que perder), así que el fallo vivía detrás de los casos que
iban — 3 verdes no dicen nada del cuarto.

Y no daba error: el método corría interpretado y el programa imprimía 42. **N1.1 se cerró
el 23-ago verificado por EMISIÓN** —el C era impecable con el nombre equivocado— y ésta fue
la primera vez que ese caso se ejecutó en una placa. Arreglado poniendo el nombre verdadero
en el símbolo ELF; reverificado: **4/4, sin skip**. Sin tocar C, o sea sin reflashear.

**⚙️ Queda `AotSimboloEnModTest`**, que compara los nombres que el AOT registra contra los
que el `.mod` exporta. Es la red que faltaba, y caza la clase entera porque compara DOS
ARTEFACTOS — el fallo era un desacuerdo entre `AotCEmitter` y `MivmEmitter`. Puesta en rojo
a propósito antes de darla por buena.

📌 **El patrón del día, tres veces**: N1.1, el `long[]` y el censo estaban «verificados» por
emisión. Lo que los destapó fue **ejecutar** — en host el `long[]`, en placa los otros dos.
Y en placa hizo falta además `log=1`: sin él, `NatV7` imprime los cuatro valores correctos
y no hay nada que mirar.

**⏭️ Riesgo que sigue**: las otras dos familias (S3/P4 y STM32) no han ejecutado una línea
de esto, y el `bpvm_aot_clear()` mal colocado sigue sin arreglar en `repl_esp32.c` y
`stm32_repl.c`. Al desplegar ahí hay que regenerar además los **cuatro artefactos nativos de
SQLite + `SQLite.pack`** (ABI 5 → 6), y sólo entonces retirar el `.mdn` suelto
(`PicoExplorer.java:1106`).

**⚠️ Y sigue en ROJO lo de ayer, sin tocar**: la paridad dual-VM da **35 PASS / 3 FAIL**
(`CastExt`, `ListaBp`, `ListaHer`) por desfase de slots en `Core.mod` — el compilador pide
`Integer#value#2` y la stdlib publicada exporta `#7`. Es **anterior** a estos cambios
(comprobado revirtiendo) y regenerar la stdlib es un cambio de ABI: decisión de Eduardo.

### 23-ago (noche) — N1.3 y N1.4: el `.mdn` ya viaja DENTRO del `.mod`, validado en placa

**N1.3 — los statements sencillos**, y la cuarta pata primero: **`AotCoberturaTest`**, que
**mide** qué construcciones pasan por el AOT en vez de recordarlo. Cobertura **20 → 23 de
27** (`null`, `do…loop`, `print`).
📌 El medidor se validó a sí mismo: la primera medida decía 17 de 28, y al hacer que dijera
el **motivo** de cada rechazo salieron **seis fragmentos míos mal escritos** — cuatro nodos
sí estaban soportados. Un medidor sin la columna del porqué acusa a quien no es.
📌 Y `print` **no era** «una llamada al runtime que ya existe», como decía la ficha: el
intérprete despacha por tipo y la tabla de helpers sólo cubre tres. Va lo que hay y se
rechaza el resto **nombrando el tipo**.

**N1.4 — la fusión `.mod`/`.mdn`.** Formato **v7** (header de 36 B, sección `native` entre
`interface` y `data`), fusión **en memoria** (idea de Eduardo: `MdnPack` ya construía el
`.mdn` en un buffer), **acumulativa** por familia, y el cargador registrando desde ahí.
✅ **VALIDADO EN LA PICO con el `.mdn` BORRADO: 8601 → 84 ms (102×)**, mismo resultado, y
`zero-copy` — el código se ejecuta donde está.
📌 **v6 sigue ejecutándose**, así que no hubo que regenerar los 32 `.mod` existentes: el
gate del `#284` vigila el ABI y v7 no lo toca.
📌 **`MOD_FORMAT.md` llevaba DOS versiones de retraso** (describía v5 con el compilador
emitiendo v6). Ahora documenta v6 y v7.

🩸 **LA LECCIÓN DEL DÍA, y es la más cara que ha dado V6: SEIS fallos, todos míos, y
NINGUNO detectable sin placa** — orden de la fusión · ABI subido sólo en C · `data+off` con
lectura por trozos · relleno copiado (blob desalineado → hard fault) · `malloc`+`free` con
un cargador zero-copy (thunks a memoria liberada) · `bpvm_aot_clear()` tras la carga.
**Los seis pasaron por 109 pruebas de compilador, 39 de VM y la paridad dual-VM.** No es que
las redes fallaran: **ninguna toca el camino AOT en placa**. Ése es el hueco de cobertura, y
ahora está medido.
🔦 **Y el instrumento decisivo fue el log del dispositivo.** Tres iteraciones teorizando
sobre código; en cuanto se encendió (`log=1`), una línea —«1/1 thunks registrados»— descartó
formato, subida, ABI, alineación y lectura de golpe. **Encender el log es el primer paso, no
el último.**
📐 Dos de esos seis los anticipó la pregunta de Eduardo sobre la **alineación**, hecha antes
de que existiera un solo blob.

**⏭️ AL VOLVER:**
1. **Las otras cuatro imágenes** — y ahí hay uno esperando: el `bpvm_aot_clear()` mal
   colocado está también en `repl_esp32.c` y `stm32_repl.c`. Es **U3** en estado puro: un
   arreglo que hay que hacer tres veces y que, si se hace en dos, falla mudo en la tercera.
2. **Retirar el `.mdn` suelto** cuando todas lean la sección. Ése es el día en que los dos
   ficheros dejan de poder desparejarse — el objetivo de la ficha.
3. Y sigue pendiente **la paridad en rojo** (3 fallos por desfase de ranuras en `Core.mod`),
   que no es de hoy y nadie vigila.


### 23-ago (tarde-3) — el hito AOT: `native` en métodos y `double`, los dos cerrados

**N1.1 — los métodos `native` ya se compilan a C** (`e1842bd9`, `631f55fa`). Salió tal como
lo planteó Eduardo —*«mimodulo.miclase.mimetodonative(objMiclase, ...)»*— y **no hubo que
forzar nada**: `ModWriter.addMethod` ya hacía `declareParam("this", 8)` antes que los demás.
Faltaba **exportar el nombre**, porque el registro AOT busca el símbolo para sacar la
dirección. Y la lectura de miembros va **por el getter** (`call_method_i32`), que es como lo
emite el bytecode y la única ruta que el runtime soporta.

**N1.2 — `double` ya cruza** (`a853b9a9`, `e8b1c8f1`). 21 helpers al final de la tabla, el
emisor los usa, `MDN_ABI_VERSION` 4→5. `DPOW` pasa a `bpvm_dpow`: **una** implementación que
comparten intérprete y helper.

🧠 **Y la lección del día, que es la misma tres veces.** Eduardo me corrigió en las tres, y
las tres tenían la misma forma — yo escribía *«no se puede»* donde era *«no está hecho»* o
*«es más caro»*:
1. *«el cuerpo del método no tiene símbolo»* → lo tenía; faltaba un booleano.
2. *«hace falta el layout de la clase»* → hacía falta llamar al getter.
3. *«no se puede llamar al opcode»* → sí se puede (lo hicimos con el getter); lo cierto es
   que para aritmética el helper es más barato.
Ver [[no-se-puede-vs-no-esta-implementado]]. La señal es que la frase salga de **no haber
mirado**, no de haber medido.

**Redes nuevas** (las dos primeras del AOT que hay en el repo): `AotNativeEnMetodoTest`
(4 casos) y **`powtest` en el corpus de paridad**, que cubría `doubletest` pero ningún caso
de `^`. Paridad 34 → **35 PASS**. Baterías 108/0 y 34/0.

🔴 **HALLAZGO QUE NO ES MÍO Y CONVIENE MIRAR PRONTO: el arnés de paridad está en ROJO.**
Tres samples (`CastExt`, `ListaBp`, `ListaHer`) fallan con
*«lib 'Core' presente pero no exporta `Core.Integer#value#2`»*. Es **desfase de ranuras**: el
`Core.mod` publicado tiene `Integer.value` en el **slot 7** y el compilador de hoy lo pide en
el **2**. Lo aislé revirtiendo mis cambios y reconstruyendo: **sale igual**, o sea que es
previo. H13 no lo vio porque su batería es `scripts/h13-lista.sh`, no este arnés — **V5 se
publicó sin que nadie mirara esta red**. Regenerar la stdlib es cambio de ABI, así que no lo
toqué.

**⏭️ AL VOLVER, tres cosas:**
1. **Lo de la paridad en rojo** — es el invariante sagrado el que está sin vigilar.
2. **Las pruebas en placa**, que acumulan ya tres cambios sin verificar: el log de la Pico
   (U1.2), los métodos `native` y los `double`. ⚠️ Ese día hay que **reflashear las cinco
   imágenes Y regenerar los cuatro nativos de SQLite con su pack A LA VEZ** — el ABI subió,
   y hacer sólo una mitad deja el arranque rechazando los packs.
3. Del hito N1 quedan los **statements sencillos** y la **fusión `.mod`/`.mdn`** (punto 4),
   que Eduardo dejó para más adelante.


### 23-ago (tarde-2) — U1 hecho y cerrado, y siete hitos más

**Código, por fin.** Las dos tareas de U1, verificadas **en las cinco imágenes**:

- **`U1.2` — el log de la Pico al núcleo común** (`31ad091e`): de **280 a 101 líneas**. Era
  la única familia que no compilaba `src/bpvm_log.c` —su `CMakeLists` ni lo nombraba—
  mientras STM32 y ESP32 ya sólo aportaban cintura, así que **había dos ejemplos** y no hubo
  que diseñar nada. ⏳ **Falta placa**: que el post-mortem sobreviva al reinicio.
- **`U1.1` — `json_min` al común** (`b506bbde`): de 3 copias byte-idénticas a una. El
  trabajo no era mover el fichero sino el alta en **cinco builds** — y de paso arregla el
  simulador, que metía mano en `pico/`.

**Y U1 se cerró con DOS tareas, no cuatro.** Las otras dos se cayeron al mirarlas:

- **`U1.3`** lo paró Eduardo: *«si vamos a fusionar `.mod` y `.mdn`, deja de tener sentido»*.
  Cierto — la fusión borra ese código.
- **`U1.4`** tenía la **premisa falsa, y era mía**: dije «cuelgan de `bpvm_part.h`» y
  `bpvm_part.h` no hace E/S de flash. Lo real: ~12 cableados de `erase`/`program` y **dos
  cinturas distintas** ya existentes. Es diseño, no mecánica.

🧠 **La lección, que vale para U2–U5**: mi criterio de «fácil» era *«¿existe ya el
contrato?»*. Detecta divergencia muy bien y **no presupuesta nada** — ni mira si el trabajo
sobrevivirá a lo ya planeado.

**Hallazgo gordo, y arreglado** (`27dadba3`, `c7d0bbee`): **los dos proyectos STM32 tenían
una configuración `Release` abandonada**. A la de la Discovery le faltaban cuatro `-D`
—entre ellos `BPVM_BOARD_DK2`, sin el cual `board.h` cae a la rama de la **Nucleo**—, dos
rutas de include y tres exclusiones; la de la Nucleo no tenía **ninguna** ruta ni exclusión,
y ni siquiera se había construido nunca. Lo que se publica sale de `Debug`, comprobado con
números (887.604 B contra los 888.268 del `.bin` publicado). **Eduardo mandó borrar las dos.**
⚠️ Queda dicho lo que es harina de otro costal: esa única configuración se llama `Debug`
aunque compile a `-Os` y sea la de publicar.

**Siete hitos nuevos** (`8a337ad7`): **N1** AOT · **L1** lenguaje y compilador · **E1** IDE
y wire · **G1** el bucle de LVGL a un hilo BP propio · **P1** ESP32-C3 y C6 · **P2**
pantallas SPI (tras P1). Más `A1`, la revisión por niveles, tras la unificación.

**⏭️ AL VOLVER — encargo de Eduardo:** *«antes de empezar U2, que me parece un trabajo
bastante pesado, me gustaría abordar un poco de AOT (1 o 2 puntos, no todo), y así vamos
cambiando un poco de tipo de tareas»*. El hito **N1** tiene **cuatro** puntos —la fusión `.mod`/`.mdn` entró ahí al final del día,
por decisión suya: *«aunque no sea exactamente AOT, cuando terminemos se puede probar todo
junto en placa»*— y el orden que él mismo dejó escrito el 21-ago es **por tandas, no de un
salto**:

1. **`native` en un MÉTODO** — hoy se ignora **en silencio**. Son dos cosas y en este orden:
   que **AVISE** (barato, y `AOT_LIMITES.md` dice que *no espera a V6*) y luego abrir el
   barrido de `AotCEmitter.java:259` a los métodos, pasando el objeto como primer parámetro.
   📌 `MemberAccessExpr` ya está soportado, así que lo del `this` puede que no sea el muro
   que parecía — ver [[no-se-puede-vs-no-esta-implementado]].
2. **`double`** (`#426`) — diseño hecho en `docs/V6_IDEAS.md` §double.
3. **Los statements sencillos**, del censo de `AOT_LIMITES.md`.

4. **La fusión `.mod`/`.mdn`** — diseño hecho, y **el punto más pesado del hito**: sube la
   versión del `.mod`, deja rancias las cuatro copias de la stdlib, y toca la lógica de poda
   y comparación del IDE.

El 1 parece el mejor primer paso: su mitad barata (avisar) cabe en una sesión corta y
convierte una mentira muda en una línea. La progresión natural es barato primero y la
fusión cuando haya rato seguido.


### 23-ago — V6 abierto: el índice, al día, y el primer paso verificado

Sesión corta, de orientación. **No se tocó código.**

- **`V6_BACKLOG.md` no recogía el bloque de arquitectura del 22-ago.** El índice se escribió
  el 21; las nueve reflexiones sobre unificación salieron el 22 y sólo dos llegaron.
  Faltaban ocho — la columna vertebral del hito. Entran en §C como «La fundamentación»,
  separadas de las tareas porque no son tareas: son el porqué y la medida. `666732fd`.
- **Verificado el primer paso que sugiere la medida** (`FICHAS` L2492, «el orden que sugiere
  la medida»): `json_min` es **realmente gratis**. Los tres `.c` son byte-idénticos (8.688 B,
  mismo md5) y las tres cabeceras declaran las mismas seis funciones; sólo cambia el formato.
  Se puede subir a `src/` sin decidir nada.
- ⚠️ **Y una advertencia sobre mí mismo, porque hoy pasó cuatro veces**: varios `grep` míos
  dieron falso rojo —alternancia `\|` con `-E`, clases sin dígitos, un patrón que exigía el
  paréntesis pegado— y llegué a dar por asimétrico un `json_min.h` que estaba bien. Ninguna
  llegó a los docs, pero el patrón es el de [[instrumento-mudo-dudar-de-el]]: **antes de
  declarar algo roto, contrastar contra un caso que se sabe bueno.**

**Y por la tarde, encargo de Eduardo:** *«archivar V5, poner V6 en primer plano; lo de V6
que esté por ahí en diferentes archivos se unifica; las notas de V6 sin guardar ya se pueden
guardar»*. Hecho en `6351b990`:

- **Guardado lo que vivía fuera del repositorio** (`notas/`, ignorada por git): `V6_IDEAS.md`
  (538 líneas, cinco diseños ya trabajados), `V5_BACKLOG.md` (1.116) y `V5_IDEAS.md` (2.977).
  ⚠️ **Y aquí una trampa que casi muerde**: `V5_IDEAS.md` existía en los DOS sitios y no eran
  copias — 25 secciones en `notas/` frente a **una distinta** en `docs/`. Sobrescribir habría
  borrado esa. Se fusionaron: 26 secciones, verificado.
- **`FICHAS` reordenado sin perder una línea** (comprobado por conteo, no por confianza):
  ABIERTAS empieza por V6; debajo, las **heredadas de V5**, que NO se archivan porque no
  están resueltas — les falta decidir si entran en V6. Bajan al archivo cuatro secciones ya
  terminadas y el CODE FREEZE, que arriba se leía como una instrucción en vigor.
- **Unificado lo de V6 que estaba suelto: cinco asuntos vivos** que el índice no recogía —
  los packs del S3 sin región, el módulo rancio de ESP32/STM32, el estado persistente de la
  Metro, `native` en un método, y **el CENSO FUNCIONAL** que Eduardo especificó el 17-ago y
  que sólo estaba dentro de `CENSO_FAMILIAS.md`. De 31 asuntos a 36.
- Las tres copias de `notas/` quedan marcadas **⛔ COPIA MUERTA** para que nadie edite la
  equivocada. Sin borrar: eso lo decide Eduardo.

**Lo siguiente, sin compromiso de fecha:** decidir el ALCANCE de V6 antes de tocar nada. La
medida sugiere `json_min` → REPL (el 80 % del problema) → el log. El REPL es el trabajo de
verdad y no cabe en una sesión corta.

**Y esa decisión la tomó Eduardo en el momento**, así que no quedó esperando: *«lo que haya
de V5 que quedó pendiente pasa a V6 y deja de ser de V5»*. El bloque heredado deja de ser un
limbo (`09640964`). Los títulos de sección siguen diciendo `V5/H10` o «la cola de H2» a
propósito: dicen **de dónde viene** la ficha, no a qué hito pertenece.

⚠️ **Lo que SÍ queda, y es distinto:** de las 59 entradas heredadas, **unas 43 llevan marca
de cierre** y deberían estar en «CERRADAS». No se movieron porque separarlas exige leerlas
una a una — mi clasificador automático falló en dos (`#422` decía «EL CHIVATO, HECHO» y no
lo detectó). Es una limpieza de una sesión corta, y hasta hacerla **ABIERTAS parece mucho
más grande de lo que es**.

**Y la sesión siguió, con el censo funcional de V6** (`CENSO_SISTEMAS_V6.md`, nuevo):

- **Paso 1 — los 32 sistemas.** Salen del listado real de fuentes y del `grep` de quién
  implementa cada interfaz. Contesta la pregunta de Eduardo sobre la memoria: hoy **reserva
  y GC son UN solo sistema** (`heap.c`, 1.121 líneas), y hay una **tercera** pieza que no es
  ninguna de las dos — la tabla de handles, sin fichero, repartida por cinco `.c`.
- **Paso 2 — cuántos están unificados**, con una prueba **mecánica**: un `.c` por familia
  está bien construido si implementa un contrato de una cabecera común. **22 de 32 están
  donde deben**; de los 10 que faltan, sólo 3 son hardware. Dos hallazgos: **no existe
  `bpvm_repl.h`** (el REPL no tiene el contrato roto, no tiene contrato), y los dos
  `wire_v1.c` **difieren al 100 %** pese al nombre.
- 🔴 **Y el censo estuvo MAL y lo cazó Eduardo.** Me dejé fuera `esp32p4/` entero: censé los
  directorios que *recordaba*, no los que hay. Lo destapó una contradicción de mi propio
  documento —«la BIOS sólo la tiene la Pico» cuando SQLite corre en la P4—. La P4 sí tiene
  BIOS, con la misma macro. Corregido en `b0d33ff9`. **Son CUATRO árboles de placa, no
  tres.**
- **Los hitos U1–U5** (`FICHAS` §«LOS HITOS DE V6»), por decisión de Eduardo: unificar antes
  de seguir analizando, de lo fácil a lo difícil, y la revisión por niveles (A1) **después**
  — *«al estar todo unificado, cualquier cambio estructural se hace una vez y no 3 veces»*.
- **Tres decisiones cerradas**: `/sd` pasa a **prefijo reservado** (cambio de comportamiento
  → notas de versión) · `Map` llevará objetos internos para **claves y valores** + `add`
  sobrecargado para clave entera y cadena · y **`notas/` se vacía**: borrado todo lo de V5.

**⏭️ Al volver**: U1 es lo siguiente, y **U1.2 (el log de la Pico) es el mejor primer paso**
— cierra `#423`, hay **dos ejemplos** de cómo debe quedar (STM32 y ESP32 ya sólo tienen
cintura), y se comprueba fácil: que el post-mortem siga sobreviviendo al reinicio. `U1.1`
(`json_min`) es igual de mecánico pero toca **los cinco builds**, así que con calma.


### 22-ago (tarde) — V5 PUBLICADA

Se empujaron los **358 commits** acumulados desde el 6-ago, se etiquetó `v5.0` sobre
`ff408a1e` y se creó la release con el ZIP como artefacto único. Detalle y verificación,
en `FICHAS.md` §«V5 PUBLICADA».

**Lo que pasó por el camino, que es lo que interesa aquí:**

- **La revisión del push encontró dos carpetas que nadie había mirado**:
  `bp_propuesta_modelo_memoria/` (205 KB de mayo-julio) y `diag/orm-slots/`. Eduardo dejó
  la decisión abierta —*«si crees que es interesante se sube… pero es antiguo»*— y suben
  las dos. La primera con portada nueva, porque no tenía raíz y citaba 13 veces unas
  carpetas que nunca estuvieron en el repo.
- **Mi primer recuento de enlaces rotos estuvo mal**: resolví los `../` contra la raíz del
  conjunto en vez de contra la carpeta de cada fichero, y me salieron rotos que no lo
  eran. Rehecho bien antes de escribir nada.
- **Las cuatro portadas seguían anunciando V4.** Al corregirlas hubo que **rehacer el ZIP**
  (la web viaja dentro, es la ayuda del IDE) *después* de que Eduardo ya lo hubiera
  probado. La lista de ficheros quedó idéntica, así que su prueba seguía valiendo — pero
  por suerte, no por método. Corregido el orden en `PUBLICAR.md`.
- **Eduardo avisó del problema de refresco de la portada de V4** y estaba fichado: dos
  despliegues seguidos se pisan y el segundo **falla en silencio**. Esta vez se comprobó
  el `status` del build *y además* qué sirve la web con `curl`. Las dos cosas, verdes.
- **El checklist mentía sobre el cuerpo de la release.** Decía «= sección de
  `RELEASES.md`»; V4 había publicado un cuerpo **bilingüe** con Descarga y Documentación.
  Se siguió el precedente, no el doc, y se corrigió el doc.

**Riesgo que queda:** ninguno bloqueante. El congelado está levantado; lo siguiente es V6
y su índice está en `V6_BACKLOG.md`.

### 22-ago — H13 TERMINADO: seis placas, y ocho bugs por el camino

**Las tres familias cerradas.** `ListGets` da **21 líneas idénticas** al PC en Pico, Metro,
Nucleo, Discovery, S3 y P4 — tres familias, **tres arquitecturas** (ARM, Xtensa, RISC-V).
La Puerta 1 era el gate no negociable y está pasado en todo el parque.

**La matriz de nativo, completa.** `.mdn` generado al vuelo: ARM **105×** (Metro), RISC-V
**112×** (P4). `.npk` precompilado en pack: ARM y **RISC-V**, los dos con `SqlDemo` 25/25.
Las cuatro casillas, que hasta hoy eran dos.

**Y la BD entera en DOS familias**: `SqlDemo` 25/25, `DaoDemo` 25/25, `GenDemo` 10/10 — en
la Metro (ARM) y en el P4 (RISC-V). 60 líneas por placa, idénticas al PC.

**Ocho bugs arreglados, ninguno visible leyendo código:**
- 🩸 **Los packs no se podían grabar en STM32**: el IDE rearma el pack antes de mandarlo y
  lo alineaba a **4 KB**, pero el U5 borra en páginas de **8 KB**. Fallaba la mitad de las
  veces con un `BAD_ALIGN` que **culpaba al tamaño del fichero, lo único correcto**.
- 🩸 **El pack se negaba a grabarse si el micro no tenía motor nativo.** Decisión de
  Eduardo: *«lleve nativo o no, el pack se graba; si el nativo no corresponde, se graba SIN
  nativo»*. Y el mensaje anterior aconsejaba **reconectar**, cosa imposible de arreglar en
  un Xtensa.
- 🩸 **`/app` es el punto ciego**: el STM32 vacía y reembebe `/lib` cada arranque pero no
  toca `/app`, donde el IDE deja las dependencias. Un `Core.mod` rancio ahí dio `exit 11`
  con `/lib` recién puesto. Lo encontró Eduardo con `dir /app`.
- Y: la guía de instalación **sin la Discovery** (que sí viaja en el ZIP), el manual
  **negando el AOT del P4** (lo tiene desde V4, 113×), `ChartDemo` con ruta rancia en la
  batería (que queda **51/19/0/0**, limpia), y los samples y blobs del día anterior.

**Y tres cosas que no son bugs pero faltaban:**
- **`#408` CERRADA**: el árbol del P4 son **145 ms** (`app:1/3 lib:14/64 sd:8/33`) y el
  formateo de la Metro **15 s** — lo que cuesta borrar 4,2 MB. Ninguno es un cuello.
- **La decisión 5 CONTESTADA, y son TRES respuestas**: RP2350 instala si falta *o difiere*
  y **avisa**; STM32 vacía y reembebe `/lib` cada boot; ESP32 instala **sólo si falta y no
  avisa**. La instrucción honesta para la release vale para las tres: **borrar `/lib` y
  `/app`**.
- **El S3 no expone packs** — no existe `pack_s3.c`. Es el agujero de `#327` (que fue el de
  la Pico) migrado de familia. Fichado; no bloquea porque nunca se prometió.

**🧠 Y una mañana entera de REFLEXIONES de Eduardo sobre V6**, todas medidas contra el
código y fichadas: el criterio de capas (sólo hardware/HAL deben diferir) · el inventario
(el REPL **triplicado en 220 KB**, `json_min` en **3 copias idénticas**) · unificar packs
(la diferencia cabe en **una función**) · uno o dos boots (la frontera existe pero no tiene
nombre) · el simulador con **disfraces** por familia · por qué unificar da fiabilidad (cero
bugs de memoria y de FS en dos días; los cinco estructurales, todos de lo repartido) · y el
coste (sistemas **3×** el lenguaje en trabajo real de V5, y el lenguaje ya no crece).
📌 Con una corrección suya que conviene recordar: yo medí líneas TOTALES y eso es el
artefacto, no el esfuerzo. Bien contado —365 commits desde `v4.0`— salen **17.389 líneas
tocadas en sistemas por familia frente a 7.234 de lenguaje**, y con **12.146 borradas**:
eso no es escribir, es **reformar**.

**⏭️ A LA VUELTA: arreglar lo que falte y PUBLICAR.** Lo pendiente ya no es probar:
1. 🔴 `appv1lsp` y `appv2` viajan **rotos** en el ZIP → arreglar, sacar, o a `samples/errores/`.
2. 🟡 El manual inglés se dejó siete secciones de V4.
3. 🟡 Escribir en `RELEASES.md` la instrucción de actualizar (ya la sabemos).
4. 🟡 **Rehacer el ZIP UNA vez** — han cambiado samples, IDE, `PackBurn`, tres imágenes y
   mucha documentación.
📌 Y dos fichas abiertas que **no bloquean**: el bug de LSP entre interfaces de módulo, y
el estado persistente que dejó una placa sin ejecutar nada (sólo lo curó reparticionar).

### 21-ago — H13 día 1: Pico y Metro CERRADAS, y cinco bugs que sólo salieron ejecutando

**Dos placas completas** (Puertas 1, 2 y 3), todo comparado contra la salida del host:
`ListGets` 21/21 en las dos variantes de RP2350 · `SqlDemo` 25/25 · `DaoDemo` 25/25 ·
`GenDemo` 10/10 · `PacksDemo` 32/32 · `SdDoc` 5/5 · `SdCard` los seis · `Bench` 105×.

**Y con eso quedan probadas las DOS cadenas de nativo**, que son independientes: el
`.npk` de ARM *precompilado y empaquetado* (SQLite, que nunca se había ejecutado) y el
`.mdn` *generado al vuelo* por el IDE para esta placa (`Bench`).

**Cinco bugs, y ninguno se veía leyendo código:**
- 🩸 **`#414`**: los cuatro builtins de packs vivían dentro de un `#ifdef BPVM_GUI`.
  Nacieron así, en el propio commit de la feature: **nunca habían funcionado en una
  placa sin pantalla**. Lo destapó `Packs.list()` muriendo en la Pico.
- 🩸 **Los blobs de `IO` y `Math` de la Pico, rancios desde el 17-ago**: `#415` los
  añadió al FIRMWARE y nadie los dio de alta en el GENERADOR. Lo cantó la placa
  (`/lib/IO.mod NO es el de esta imagen`) y lo preguntó Eduardo. Sólo afectaba a la
  Pico: el S3 y el STM32 sí los listaban.
- 🩸 **Ocho samples del ZIP que no compilaban** desde el cambio `any`→`Object`.
- 🩸 Un `test1.pack` de pruebas viajando en la distribución, y el README de `packs/`
  todavía en V4 (decía 26 módulos; son 27).

**El censo que lo destapó**: la batería cubre ~70 samples y en el ZIP viajan **278**.
Compilados los 278: 8 arreglados, 8 artefacto del arnés, 2 con bug real.

**🧠 Y la lección de método, que vale más que los bugs:** la cascada Java→C→placa **no
podía** cazar el `#414`, porque la VM-C de host se construye **con GUI** y los firmwares
sin pantalla no. El doble era más permisivo que el original. *«Lo que no funciona en C
tampoco en la Pico»* sólo se sostiene si el C que se prueba lleva la MISMA configuración.

**Dos cosas fichadas SIN resolver** (las dos con diagnóstico, ninguna con parche):
- La **sustitución por LSP entre interfaces de módulo** no funciona → `appv1lsp.bp` y
  `appv2.bp` viajan rotos en el ZIP. No es regresión de V5.
- Un **estado persistente deja la placa sin ejecutar NADA** —ni el `Hello` embebido—,
  sobrevive al flasheo, no lo cura formatear, y **sólo lo cura reparticionar**. Sin
  causa. Lo que falta medir está escrito en la ficha.

**Imágenes**: las cinco al día. El P4 y la DK2 **exentas por demostración** —compilar
`builtins.c` con `-DBPVM_GUI` antes y después del arreglo da un objeto byte a byte
idéntico—, así que sólo hubo que rehacer Pico, S3 y Nucleo.

**⏭️ MAÑANA: P4 y STM32**, flasheables directos desde `dist/firmware/`. El Nucleo
llegará con `/lib` rancio: es la ocasión —perdida ya dos veces— de medir cuál es el
arreglo MÍNIMO al actualizar de V4, que es la decisión 5 y hoy no existe en la
documentación. Las seis decisiones abiertas están numeradas en `H13_PRUEBAS_V5.md`.

### 20-ago (tarde) — H12 CERRADO, el bilingüe al día, y la tanda de construcción

**`H12` cerrado con sus ocho puntos.** Y lo caro no fue escribir: fue lo que escribir
destapó — la §13.2 de la referencia daba un ejemplo que YA NO COMPILA, la consola del IDE
se dejaba seis comandos, el ENV no tenia ni una clave escrita, `AOT_LIMITES` negaba el
`long` en `native` y el manual seguía diciendo que las colecciones usan `any`.

**Estructura, decidida por Eduardo:** la referencia en dos grupos (§13 lenguaje / §14
dispositivos), §15 Packs aparte, y **todo dentro de `referencia.html` salvo dos volúmenes
propios**: la GUI «porque es demasiado grande» y las BD «porque lo merecen».

**Y el bilingüe**, que yo no había mirado: `en/referencia.html`, `en/guia-ide.html`,
`en/index.html` y `en/basedatos.html` (no existía) puestos al día. Los ids son idénticos
en los dos idiomas, así que el guion de reestructuración valió tal cual.
⏭️ Queda deuda ANTERIOR: el manual inglés se dejó siete secciones de V4 (eventos,
sobrecarga, `dospasadas`).

**Tanda de construcción completa**, del mismo árbol: stdlib, los dos packs, los blobs, las
5 imágenes selladas en `dist/firmware/` y `BasicPlus-5.0-win.zip`. **Versión del IDE a
5.0.** El `.npk` de ARM regenerado desde sus fuentes — era el rancio.
📌 El rebuild dio **bytes idénticos** en todo salvo ese `.npk`: la construcción es
reproducible.

**Coda del día — cuatro notas de V6 sobre el ESP32-P4X.** Ninguna toca código: el freeze
sigue en pie y el ZIP sin rehacer. Eduardo avisa de que las placas que tenemos son P4 y
que por un defecto eléctrico van a **360 MHz** — los 400 dan problemas de consumo — y que
el **P4X** lo corrige y sí llega a 400. Al escribirlo fui al IDF y lo que yo había supuesto
resultó falso: **una sola imagen no puede cubrir los dos silicios**. Lo dice su Kconfig en
la primera línea (*«rev. <3.0 and >=3.0 is mutually exclusive»*) y el interruptor arrastra
hasta el reloj del bootloader. Política de Eduardo: **se mantienen las dos**, porque del P4
se siguen vendiendo placas y del P4X apenas hay. Y una buena noticia medida: el bootloader
ya rechaza la imagen equivocada en las dos direcciones, así que basta con nombrarlas bien.
Todo en `FICHAS.md` → «Aplazadas a V6».

**Coda del día — sesión de DISEÑO, ya sin placa.** Eduardo pide el listado de lo aplazado
y de ahí sale que estaba en **tres sitios**; se consolida en `docs/V6_BACKLOG.md`, que es
un ÍNDICE y lo dice en su cabecera para no crear una cuarta fuente. Al reunirlo aparece
una entrada rancia (los captadores tipados de `List`, hechos en V5) que se mueve a
«CERRADAS» conservando sus 89 líneas de diseño.

**Y tres asuntos nuevos, los tres nacidos de un problema observado y no de imaginar:**
- **La RAM del código nativo de un pack.** La tercera vía ya estaba elegida y corriendo
  (la arena del ENV); lo abierto es que hoy es singular y se llama `SQLite`, y que los
  `malloc` de la BIOS son chivatos que aún no reparten nada.
- **🩸 `native` en un MÉTODO se ignora en SILENCIO** — bug real, fichado. Y salió porque
  **Eduardo dudó de un diagnóstico mío**: yo dije *«no hay `this`, no se puede»* y él
  contestó que `miObjeto.miMetodo(...)` es `miMetodo(miObjeto, ...)`. Tenía razón: el
  emisor ya emite `MemberAccessExpr`; lo que falla es un barrido que no desciende a las
  clases (`AotCEmitter.java:259`). **El trabajo era mucho más pequeño de lo que yo dije.**
- **El lazo de LVGL y los cabos del AOT**, que resultaron ser la misma enfermedad.

📌 **Y el censo que más va a doler**: `AOT_LIMITES.md` nombraba **5** límites y son **~24**
— la lista corta engañaba porque el emisor tiene rechazos genéricos, así que sólo estaba
documentado lo que alguien se encontró de frente. De ahí sale el **hito AOT de V6**
(encargo de Eduardo: arreglar el método, `double`, y los statements sencillos).

⏭️ **Pendiente de decidir**: `notas/` está en el `.gitignore` y ahí viven **473 líneas** de
diseños de V6 sin versionar — y por esa misma vía ya se perdió el `SqlDemo.bpbuild`. Los
documentos nuevos se han creado en `docs/` a propósito.

**⏭️ MAÑANA: H13 en placa**, con `docs/H13_PRUEBAS_V5.md`. Dos días (viernes y sábado),
partido por puertas. La Puerta 1 —el ABI de `Comparable` en las tres familias— es la que
bloquea, y la Metro es obligatoria porque su `.npk` no se ha ejecutado nunca.

### 20-ago — H12 en marcha: tres documentos, y la referencia reordenada

**Escrito y verificado:** `docs/TARJETA_SD` y `docs/BASEDATOS` (este ultimo, del dia
anterior), mas el apartado de packs. Los ejemplos de la SD se validan con
`samples/SdDoc.bp`: las DOS VMs, salida identica.

**🩸 Y otra vez, escribir los ejemplos los rompio** — cuatro hallazgos, el ultimo gordo:
`chr(10)` NO EXISTE (se me habia inventado; son los escapes del lexer); `listDir`
devuelve `string[]`; un array NO tiene `.length()` (eso es de los objetos, los arrays van
con `for..in..next`); y **`listDir` no esta en la VM-C**, o sea que funciona en el PC y NO
en la placa. Es el UNICO verbo de fichero que falta. Documentado como limitacion.

**📐 Estructura, decidida por Eduardo:** la referencia mezclaba librerias del lenguaje con
las de hardware. Ahora son **§13 Biblioteca del lenguaje** y **§14 Librerias de
dispositivos** (con la SD como 14.17 y la «Politica HW = clase OO» encabezando el grupo),
**§15 Packs** aparte —«no es exactamente una libreria»— y 16/17 corridas. La regla final:
**todo dentro de `referencia.html` salvo dos volumenes propios**, la GUI «porque es
demasiado grande» y las BD «porque lo merecen». Verificado por script: 34 numeros
cambiados, referencias en prosa reescritas y contrastadas contra su destino, cero anclas
rotas, indice regenerado — y se corrigio un `<ul>` sin cerrar que ya traia el fichero.

**🩸 Fallo mio del dia:** escribi los dos primeros documentos en Markdown cuando los
manuales del proyecto son HTML. Convertidos reusando la cabecera y la hoja de
`referencia.html` (mismo md5, comprobado).

**🔎 Y una correccion de Eduardo que valia oro:** el apartado de packs contaba flojo su
razon de ser. No es que ahorren flash: **ahorran RAM**, porque el modulo se ejecuta EN EL
SITIO desde la flash. Al verificarlo salio que el comentario de `bpvm.c:472` decia «hoy
con copia — el XIP es la tanda 2» y estaba RANCIO: la tanda 2 ya esta hecha
(`bpvm_loader_load_xip`, «codigo en sitio»). Corregido el comentario y reescrito el texto.

**⏭️ Idea de Eduardo para V6, anotada con su diseño:** que el IDE no suba dependencias que
el dispositivo ya tiene, y que **lo decida el dispositivo resolviendo COMO EL CARGADOR**
(da igual que este en `/app`, `/lib`, `/sys` o en un pack) — asi no hay dos buscadores que
se desincronicen. Y al mirarlo: **la mitad ya existe**, `putIfChanged` ya compara por CRC;
lo que le falta es preguntar por la resolucion en vez de por la ruta destino.

**⏭️ Sigue H12:** los cambios del IDE, `Core` (envoltorios, captadores), **las variables de
entorno** —que hay que CENSAR antes de escribir, hoy nadie puede enumerarlas—, la puerta
(QUICKSTART y los dos README) y las notas de version. Y una pasada por documento
preguntando *¿que afirma esto que ya no sea verdad?*: `AOT_LIMITES.md` niega el `long` en
`native` y eso dejo de ser cierto el 16-ago.


### 19-ago (tarde) — H12 arranca, y documentar destapa que el ORM no funcionaba

**Lo mas importante del dia no es lo que se escribio, es lo que salio al escribirlo.**
Al ir a documentar las BD hubo que ejecutar los demos, y NINGUNO compilaba. Tres
causas, ninguna la que yo suponia:
1. **el GENERADOR de DAO emitia codigo roto** — `return this.uno(...)` sin el downcast,
   y `uno()` devuelve `Object` desde #389;
2. **`List` era ambiguo en TODO programa del ORM** — al importar `Core` el semantico
   aliasa `List`, y la interfaz del modulo la reexporta COMO SUYA; quien importa
   `SQLite` y `Orm` veia dos simbolos para una clase;
3. **y lo que mas costo NO era un bug**: `samples/` tenia SEIS copias fosiles de la
   stdlib del 10-11 de JUNIO, ninguna en git, ganando por orden de busqueda.
Los tres demos **compilan Y CORREN** (`240b400d`, `7854b61f`, `6bb78c6a`).

**🩸 Y dos cosas que borre yo por la manana y hubo que recuperar:**
- los `.bpbuild` de los demos de BD — reconstruidos y ahora EN GIT (`13a78f9`);
- **`packglue.c`**, el doble de host de la tabla BIOS, que no estaba en ningun sitio.
  Reescrito desde `bios_pico.c` (`7854b61f`). Eduardo: *«un source de VM-C que no esta
  en su sitio y tampoco se ha subido al Git no es un fallo, son como minimo 2»*. Son
  TRES: fuera del arbol, fuera de git, y nadie lo detectaba — el censo que pidio
  encontro OTRA ruta muerta en el mismo target (`../bpstdlib/SQLite.bp`, rota desde
  el 14-ago).
🩸 **La leccion, y es de metodo**: al censar lo unico de `notas/` busque `.c`, `.h`,
`.sh` y `.link` — y NO busque `.bpbuild`. Mire las fuentes y me olvide de los ficheros
que las orquestan.

**Adelantado de V6 por decision de Eduardo** (*«nos ahorramos un monton de problemas»*):
**los captadores tipados de `List`** — `getInteger/Long/Float/Double/String/Boolean`
(`4a003aeb`). Como BP no tiene test de tipo, las conversiones son METODOS VIRTUALES en
`Comparable`. La opcion buena (`className()`) NO CABIA: el descriptor de clase no
guarda el nombre, asi que exigia cambiar el formato — a V6. Verificado con
`samples/ListGets.bp`: **las dos VMs, salida identica byte a byte**, y bateria H13 sin
regresion. ⚠️ Anadir 5 metodos a `Comparable` CORRE SUS RANURAS: hubo que reconstruir
la libreria de SQLite, o `DaoDemo`/`GenDemo` fallaban en ejecucion aunque compilaran.

**Escrito: `docs/BASEDATOS.md`** (`d614d27f`), las 13 secciones del indice de Eduardo.

**⏭️ Sigue H12**, con la lista y el orden en `FICHAS`: la **tarjeta SD** primero (el
hueco grande, tres hitos sin una sola mencion), luego **packs**, **IDE**, **Core**, la
puerta y las notas. 📌 Y el encuadre de la SD, precision de Eduardo: es capacidad de la
**IMAGEN** (RP2350 y ESP32, en STM32 no), no de la placa — en una Pico 2 se cablea un
lector y funciona.


### 19-ago — la limpieza, y el rescate que casi no se hace

**85 MB fuera**, en commits separados: `3636ff0` (rescate), `f2edd80` (borrado),
`04a3bd0` y `a250bd3` (registro). Con eso la limpieza queda CERRADA entera y el
cierre de V5 pasa al siguiente paso, `H12` documentar.

**🩸 Lo que importa del dia: la ficha se equivocaba en el punto critico.** Decia que
en `notas/` *«lo unico NO duplicado son los .elf de SQLite»*. Falso. Habia **26
fuentes y scripts que no existian en ningun otro sitio del repo**, y entre ellos
`vfs_bp.c` —el VFS sin el cual `sqlite3_initialize()` falla EN SILENCIO— y los dos
`build_sqlite*.sh` con las banderas que costo sangre averiguar. Enfrente,
`bpstdlib/sqlite/nativo/*.npk`, binarios versionados **que se publican en V5**.
Borrar a ciegas los habria dejado sin receta para siempre.

Rescatado a `bpstdlib/sqlite/nativo/src/` (77 KB), que es donde Eduardo decidio el
14-ago que viviera todo lo del pack — el `LEEME.md` de esa carpeta YA decia que se
creo porque *«el pack no se podia reconstruir desde un clon limpio»*: el traslado se
habia hecho a medias, se movieron los `.bp` y los binarios y el pegamento en C no.

**🔬 Y se probo, no se supuso**: el `.npk` de RISC-V se reconstruye **BYTE A BYTE**
desde su nueva ubicacion (618.168 B, y sus numeros son los que canta el P4 al
arrancar). Ese control es lo que dice que la receta vale.

**Y el control saco un hallazgo**: el `.npk` de **ARM no se reproduce** — 8 B mas y el
punto de entrada corrido (0x114 vs 0x10c). Se genero antes de algun cambio del
pegamento y nadie lo regenero. No se toca en freeze: va a `H13`.

**Lo demas que cayo:** el parche privado (comprobando antes que no aplica), el
worktree de 28 MB —quitado con `git worktree remove`, no con `rm`— y **un segundo
worktree que la ficha no listaba** (husk de la sesion `25fabe6b` en Temp);
`V4_SAMPLES_ROJOS.md` marcado como HISTORICO y metido en git; los artefactos sueltos
y 27 `.slots`, que ademas pasan a `.gitignore` (los de `bpstdlib/` y `bpdevices/`
siguen versionados: son la distribucion).

**`dist/` decidido** (Eduardo): es una salida, se reconstruye antes de publicar y el
zip va a GitHub aparte. Ignorado, salvo los tres MANIFIESTOS de 4 KB, que son el
unico registro en el repo de que binarios salieron.

**⏭️ HALLAZGO PENDIENTE, para H13:** `bpstdlib/Str.{mod,dbg,slots}` llevan desde el
18-ago regenerados y SIN COMMITEAR (los cambio `#446`: Str paso a fachada, por eso
encoge 5.789 -> 5.578 B), y `packs/Stdlib.pack` es del 15-ago, anterior al cambio.
De las 11 copias de `Str.mod` del arbol solo la de `bpstdlib/` esta en git, asi que
esta acotado — pero es el desfase de siempre.

**Y `C:	mp` vaciado**, encargo del mismo dia: **157 MB -> 0**. 146 MB eran tres
arboles de build; los 12 MB restantes NO eran temporales (el ELF known-good de la
Discovery, un `.bak` de `gpio_stm32`, dos experimentos con `.uf2` y 42 sondas `.bp`
unicas) y se miraron uno a uno: superados o ya registrados, ninguno referenciado.
📐 **El criterio de Eduardo**: *«no tiene sentido tener algo que queremos mantener en
un directorio temporal»*. Es lo contrario del caso de `notas/` del mismo dia, y por eso
alli se rescato y aqui no.

**Pendientes de V5: 7.**


### 18-ago (tarde) — #439 probada EN PLACA, y los 32 MB del P4 que no pudieron ser

**Lo gordo: `#439` CERRADA, con la prueba en el P4.** `CuelgaLog.bp` → 4 min 30 s
girando en un `while true` → `kill` → `reset` del IDE → al volver, `log: RAM
SUPERVIVIENTE (lineas de ANTES del reset)` y la sesion entera detras, con los 269
segundos de silencio que son el cuelgue. El arreglo (region del log en RAM que el
arranque no borra) fue idea de Eduardo y evito el plan anterior, que era flush por
linea: 4 KB de erase+program POR LINEA se come el sector en minutos.

**Costo tres intentos y las dos trampas fueron de instrumento, no de codigo:**
1. la linea que dice de donde viene lo cargado solo existia en `pico/main.c`, asi
   que el P4 no podia contestar la pregunta (`be0a86e` la lleva a las 4 imagenes);
2. luego dos resets salieron `arranque en frio` **sin que eso significara fallo** —
   «esta roto» y «has usado el reset equivocado» explicaban el log igual de bien.
   Lo desempato el `resetReason` del INFO, que YA ESTABA: decia `power-on`. En
   ESP32 el boton RST tira del pin EN y cuenta como arranque en frio; en el RP2350
   el pin de RUN si conserva la RAM. Anotado como **L15** en `PENDIENTES.md`.

**Abierta `#452`** de camino —con un RUN vivo la placa solo atiende `HELLO`/`KILL`,
asi que el `RESET` del wire no llega— **y aplazada a V6 el mismo dia** (Eduardo:
*«ahora sabemos apañarnos y a los usuarios no les afecta»*): el rodeo es `kill` +
`reset`, documentado en `PENDIENTES.md` L15. De paso salio que el censo por
familias estaba mal: son CUATRO sitios, no tres — el S3 y el P4 comparten REPL y
el simulador (`tools/bpvm_sim.c:679`) lleva el mismo filtro. Censar por la
primitiva (el mensaje) y no por el nombre, otra vez.

**Los 32 MB del P4: implementado, probado en placa, revertido.** El bootloader
usaba 16 de los 32 MB; ampliarlo funciono para el FS y **rompio los packs**, y no
por tamaño (la hipotesis que Eduardo tumbo con una zona de 6528 KB): el cache de
flash del P4 direcciona a **24 bits**, o sea que nada por encima de 0x1000000 se
puede mapear. El diagnostico quedo cerrado y el rediseño (packs debajo de 16 MB,
FS encima) va a V6 — `9d0589b`. La revision quedo con el pack sano: sqlite 3.53.4,
vfs `bp` registrado, `rc=0`.

**Y lo demas que cayo:** `#427` pto 8 (el `hello_mod.c` ya sale de un GENERADOR y
del mismo fuente; las 3 copias muertas, fuera) · `#441` la mitad de la arquitectura
(el IDE compara el `arch` del `.mdn`, no solo la fecha) · `#451` SyncList a
Collections · el «pwm» del arranque y del INFO ya dicen su unidad.

**`#441` cerrada por la mitad, por decision de Eduardo:** la arquitectura entra en
V5 (`9fcff33`); la huella de los FLAGS se va a V6 porque pide cambiar el formato
del `.mdn` y *«ahora no vamos a modificar formatos»*. La cabecera no tiene campo
libre, asi que meter el hash obliga a subir `version` y a tocar el lector del IDE y
el de las cuatro imagenes a la vez — eso se hace al empezar una version, no al
cerrarla. **Con eso, pendientes de V5: 7** (3 tecnicas + 4 tareas de cierre). La septima cayo
al preguntar Eduardo si «la polaridad de Q1» era la luz de la pantalla: no —es el
MOSFET del rail de la SD por GPIO45— y resulta que no habia nada abierto. Q1 y
`pwr` son el mismo transistor, y la propia linea de la ficha ya declaraba cerrada
la polaridad de `pwr`. Error de redaccion mio al archivarla. Contestada por
triplicado: el analisis del transistor, el codigo (`blk_sdmmc_p4.c:79`) y cada
arranque (`pwr 45 (activo bajo)` + `sd: montada en /sd`).

**⏭️ COMO SE CIERRA LO QUE QUEDA — decision de Eduardo (18-ago):** *«lo que queda
pendiente se puede mirar en la fase final donde haremos mas pruebas»*. O sea que las
tres tecnicas que quedan (`#379` verificar que era `#398`, `#408` medir los dos
cuellos, `H2-P5` exFAT y superfloppy) NO son trabajo suelto: entran en la tanda de
pruebas del cierre. Y ahi cabe tambien la cola de `#439` —repetir la vuelta en Metro
y STM32— que quedo escrita dentro de la ficha ya cerrada; el codigo esta verificado
en el `.elf` de las cuatro imagenes y probado en placa en una.

**Y una que faltaba en el registro:** al resumir Eduardo el cierre como *«limpieza,
documentar y pruebas finales»*, resulta que **documentar no tenia linea en FICHAS**.
Medido: `sqlite` y `@BD` salen CERO veces en toda la documentacion de usuario
(manual, referencia, cheatsheet, QUICKSTART, los dos README) y solo aparecen en
ficheros de trabajo internos. O sea que los dos hitos mas visibles de V5 —el ORM y
SQLite en un pack— no existen para quien lea la documentacion. Ficha abierta en
«Cierre de V5» con lo que hay que escribir y con lo que ya esta bien (referencia.html
§13.3/13.4 no se quedo rancia: su «SyncList extiende List» paso a ser cierto).

**🧊 CODE FREEZE V5, desde hoy.** Eduardo: *«a partir de ahora, codigo congelado,
solamente se arreglan bugs»*. Banner en la cabecera de `FICHAS.md`, con el criterio
de V4: la pregunta no es «¿merece la pena?» sino **«¿esta roto?»**. Lo que no lo
este, a V6.

**El plan de cierre, en este orden** (decision de Eduardo, 18-ago):
1. **Limpieza** — manana 19-ago, ANTES de documentar (las 5 carpetas de `notas/`,
   los restos del arbol, la decision sobre `dist/`).
2. **`H12` — documentar V5.** Con numero de hito, por decision suya.
3. **`H13` — las pruebas finales.** Idem. Agrupa `#379`, `#408`, `H2-P5` y la cola
   de `#439` (Metro y STM32) SIN duplicar su texto: cada ficha sigue en su seccion.
4. **Publicar.**

De paso salio una contradiccion DENTRO de la fuente de verdad: la tabla de hitos
decia «H10 y H11 cerrados» y dos lineas despues «Queda H10 (IDE)». Comprobado
mecanicamente —sus 9 fichas estan todas tachadas— y corregido. La tabla ahora llega
hasta H13.


### 18-ago — el dia de las listas: 8 fichas cerradas en cadena, corpus 29→37

**El hilo del dia** (todo salio de una decision de Eduardo: cancelar `Box`,
sobrecargar `add`, y «la list sintetizada deberia desaparecer»):

- **#442** — un literal de array guardaba SIEMPRE 4 B/casilla (`long[]` daba 0
  EN SILENCIO). La trampa: `string` es PrimitiveType Y referencia — se vio
  desensamblando, no por el sintoma.
- **#443** — `newObjArray`/`growObjArray`: alias publicos de builtins que ya
  existian (el id es ordinal(): entrada nueva = id que ninguna VM conoce).
- **#444** — `CHECKCAST_EXT` (0xB0): el downcast cross-module reventaba el
  compilador. Reuso la subseccion de fixups de `TRY_BEGIN_EXT` → ni el formato
  ni los loaders cambian.
- **#447** — el cast a la PROPIA clase reventaba (descriptor se registra en
  endClass). Tapaba que **Collections.bp no compilaba desde el 16-ago** — otro
  artefacto rancio. Y el fat-jar del frontend empaqueta miVM: `install` sin
  `clean` deja el jar viejo (trazas con lineas que no cuadran = esa señal).
- **#446** — envoltorios+Comparable a Core; `formatDouble`/`longToString` con
  ellos (Str queda de fachada). El atajo `"" + x` NO valia: 1E12 vs 1000000000000.
- **#450** — el compilador YA NO sintetiza List/SyncList/OwnerList: estan en BP.
  Core se importa SIEMPRE (Core.mod 2.576→8.306 B, preinstalado). Firmware Pico
  enlazado con la stdlib nueva.
- **#449** — mi «OwnerList no se puede escribir en BP» era FALSO (Eduardo lo
  olio): `var owner items: Object[]` emite SET_FIELD_OWNER, y el owner local da
  el FREE_REF. Guardian de fin de RUN: 0 bloques sin liberar.
- **#451** — `super.metodo()` cross-module: la pista la dio Eduardo («el ctor de
  super SI se llama solo») → factorias `__cls_m_<Cls>_<metodo>`, aditivo.
- **Wrap8Test** — no era el Map: el sample concatenaba un Object-con-cadena a
  pelo (en V4 imprimia el HANDLE en silencio; #389 lo hizo visible).

**⏭️ A LA VUELTA DEL DESCANSO (lo dijo Eduardo):** el punto 1 (`hello_mod.c`
del STM32, mecanico) y **#439** con su hipotesis ya registrada en la ficha:
los logs se quedan en RAM (confirmado), y «si queremos el log como mecanismo
de depuracion de verdad, directos a la flash — falta un flush()». Primero
medir el coste del flush por linea en cada familia (la Pico borra sectores de
4K), luego decidir el modo.

**Pendientes de V5: 10.** El mas barato despues: mover SyncList a Collections
(desbloqueado; dos intentos de cirugia de texto fallaron, hacerlo con calma).

### 17-ago (tarde) — el cuelgue del P4, y UNA fuente de verdad

**Lo gordo: `#440`, verificada en el P4.** Toda `native` que tocara un literal
de cadena colgaba la placa. No era el GC ni la memoria, como parecía: era el
**modo de direccionamiento**. RISC-V compilaba con `-fno-pic` y el modelo por
defecto, que llega a sus datos metiendo la dirección de ENLACE como constante
(`lui`+`addi`); enlazar a `-Ttext=0` deja relativos los SALTOS pero no los
DATOS, y como el `.mdn` se carga donde caiga, eso es un puntero salvaje →
cuelgue mudo. ARM nunca lo sufrió (`-fpic`, remata con `add r1, pc`). Arreglo:
`-mcmodel=medany`. Con él cayeron de paso **la pata del P4 de `#430`** y
**`#302` paso 3 en la segunda arquitectura** (`AotGcRt`: 10.000 vueltas,
`malos: 0`).

**Cómo se localizó, que es lo reutilizable:** la ESCALERA (`NatEsc.bp`, en el
repo). Una `native` por peldaño, cada una exigiendo una cosa más por debajo,
con un print antes y después. Una sola corrida da el punto de ruptura sin ir
pidiendo variantes de una en una. `NatMin` (sumar enteros) pasaba y el escalón
2 (devolver un literal) moría → el thunk estaba sano y lo roto era tocar datos.

**`H2-P5`: el camino SDSC, probado sin tarjeta SDSC.** Eduardo: *«no tengo
tarjetas de 2G ni voy a tener»*. Pero lo que daba miedo era una cuenta
(`arg = alta_cap ? lba : lba*512`), y eso es aritmética pura → `make test-sdsc`,
con control por caso y rojo verificado.

**Y el cambio de fondo: `docs/FICHAS.md` es LA FUENTE ÚNICA**, y ya está en
git. Eduardo: *«me estoy volviendo loco con cosas que aparecen y desaparecen»*.
Estaba medido: de las 51 fichas que citaba este documento, **49 eran una
segunda copia**. Ahora `ESTADO` es sólo este diario, `PENDIENTES` sólo
limitaciones de cara al usuario, y `CLAUDE.md` apunta a `FICHAS`.

⚠️ **Tres errores míos de registro, todos del mismo tipo**, y conviene tenerlos
presentes porque volverán: (1) moví «el árbol trunca mudo» de `PENDIENTES` a
`FICHAS` sin contrastarlo — era `#425`, cerrada ese mismo día; (2) di los
cuatro rojos del censo como pendientes cuando 1, 2 y 3 se cerraron el 16-ago
(`ec81afc`) — no los vi porque viven DENTRO de una ficha cerrada, invisible a
un barrido; (3) repetí que «todas las medidas llevan `-Og`» cuando eso sólo
vale hasta el 16-ago a mediodía. Los tres los cazó Eduardo leyendo la lista.
**La lección: al mover algo de sitio, contrastarlo con lo cerrado; y lo que sea
trabajo de V5 no puede vivir dentro de una ficha cerrada.**

**Sin cerrar:** `#379` — probando los 5 ciclos en el P4, **el Stop cuelga**. La
hipótesis de que era `#398` disfrazado NO se sostiene. La pregunta que parte el
problema sigue sin hacerse: **¿está colgado el device o el IDE?** (pedir `Info`
por la consola con el cuelgue puesto). Se paró ahí a propósito.

⏭️ **MAÑANA: LA TIJERA.** Decisión de Eduardo al ver la lista: *«esto sigue
enrevesado, demasiadas cosas; mañana metemos la tijera a ver si podemos cerrar
unos cuantos»*. O sea que el marco del día siguiente es **cerrar y descartar,
no abrir** — y cancelar sigue siendo un resultado válido. Quedan **11** de V5
(barrido mecánico de `FICHAS`, sin V4 ni V6) + 4 tareas de cierre. De los 11,
sólo dos son desarrollo nuevo (`#438` `Box` y `List`→`Object`, que son la misma
conversación); el resto es cerrar cosas, y `hello_mod` es el más mecánico.

### 17-ago — H10 ENTERO, el grupo B mecánico, y una ficha que no existía

**H10 cerrado, las siete** (`#425`, `#437`, `#435`, `#436`, `#394`, `IDE-7`,
`#395`) — detalle en el plan de cierre, arriba. Y del **grupo B** lo que se podía
hacer sin conversación previa: `#431`, `#429`, `#412` (a V6 con su diseño) y
`GAP-4`.

**Lo más aprovechable del día, por si sirve de aviso:**

- **`GAP-4` no existía.** Decía que la notación científica de `double` en la VM-C
  estaba pendiente y que el invariante sagrado podía estar roto en magnitudes
  extremas. La medida dice que **no**: 22 casos byte a byte, incluidos los dos
  lados de cada frontera y los extremos del tipo. La ficha **nació de leer mal
  una palabra** — el comentario dice *«TODO en aritmética IEEE determinista»* y
  ese `TODO` es el **castellano** («todo ello»), no el marcador inglés de
  pendiente. `SciPar.bp` queda en el corpus de paridad: **29 PASS**.
- **`#425` era el mismo mal que `#433`**, y estaba en CUATRO sitios, no tres: el
  cuarto era el micro simulado, que es con quien habla el IDE en modo Sim. Lo
  cazó el grep de quién-más-lo-hace; de memoria se habría escapado.
- **`#429` enseñó algo al probarlo**: la primera versión ponía las fechas con
  precisión de minuto, y como el desfase típico es de segundos las dos salían
  IGUALES — un aviso cuya evidencia no se ve se lee como falsa alarma. Con
  segundos.

**Herramientas nuevas que se quedan:** `FrmBoardShot` y `EnvDialogShot` (pintan
una ventana o un diálogo del IDE a PNG sin display ni placa; la primera cazó un
botón sobre un `GridLayout(1,1)` que compilaba y rompía el layout), y
`make test-listtrunc`.

**⏭️ AL VOLVER — orden decidido por Eduardo:** *«prefiero hacer las pruebas en
placa y después meternos con `List` y `Box`, que es desarrollo nuevo.»*

1. **La sesión de placa B**, con su guión ya escrito en
   `notas/SESION_PLACA_B.md`: `SciPar` (la pata de placa de GAP-4, 5 min y sin
   reflashear), `#379` (el wire tras el Stop — primero SABER en qué placas),
   `#362` (recursos del pack, verde en host y nunca en placa), `#408` (los dos
   cuellos, con las fotos ya cambiadas) y `#415`.
2. **`List`/`SyncList`/`OwnerList` → `Object` y `#438` (`Box`)**, que son la
   MISMA conversación y tienen decisiones que son de Eduardo: el nombre, si
   distingue «vacío» de `null`, y cómo se saca un escalar — la asimetría de los N
   getters que ya salió al diseñar `File`/`TextFile` para V6. Conviene
   resolverla igual en los dos sitios.

Estado del repo: todo committeado, **sin push**. Imágenes al día (Metro 17-ago
15:57 con `#425`; el P4 se construye y sale a `-Os`). Toolchain reconstruida:
frontend, miVM y el fat-jar del IDE.

### 17-ago (tarde-noche) — LA SESIÓN DE PLACA COMPLETA, y un bug de memoria de V4

**Ocho fichas resueltas o verificadas en placa**: `#389`, `#381`/`#428`, `#430`,
`#302`p3, `#422`, `#418`, `#433` y `#424`. Grupo A cerrado. Pendientes de V5:
**16** (veníamos de 21 esa mañana, y de 45).

**Lo más importante, en la lectura de Eduardo: `#430` es un bug de V4 y seguimos
CONSOLIDANDO las VMs.** La tabla de handles nació en la migración de V4/H1 y el
disparo del GC por volumen es de `#357`: el eje de presión que faltaba —los
SLOTS— llevaba ahí desde entonces, latente. Solo se manifestó al coincidir las
tres condiciones (muchos objetos chicos + heap grande + SRAM pequeña), y se
manifestó como lo peor posible: un cuelgue mudo. Ahora es una colecta a tiempo
y, si de verdad no hay sitio, un OOM atrapable. Matiza
`v4-es-la-base-lo-siguiente-es-aditivo`: la base de V4 sigue asentándose.

**Lo demás de la tarde**, por si hace falta el hilo: `#430` se acotó con el test
de desplazamiento (el gemelo que gasta el doble murió a la mitad de camino);
`#302`p3 quedó verificado en ARM en cuanto `#430` dejó de estorbar; `#424` se
midió en vez de suponerse —el tope de 50 ms no disparaba nunca y el lazo no
estaba ocupado sino dormido— y de intentar leer esa medida salió `#433`, el log
común que truncaba en silencio mientras la Pico llevaba anillo desde `#326`.

**⏭️ PRÓXIMOS DÍAS: H10 ENTERO, hasta terminarlo** (decisión de Eduardo). Es el
grupo C: `#425` (el árbol del IDE trunca en silencio), `#394` (subir eligiendo
destino — ojo, el Upload ya respeta la carpeta del árbol desde hoy, así que la
ficha puede haber encogido), `#395` (botón `DAO build`), `IDE-7` y la clase
`Box`. ⚠️ **Eduardo trae cambios que añadir a esas fichas: escucharlos ANTES de
planificar el bloque**, que el enunciado puede crecer. Después de H10 queda muy
poco.

Estado del repo: todo committeado, **sin push** (norma: nada a GitHub hasta
cerrar la versión). Imágenes al día: Metro `bpvm_pico.uf2` (17:53) y P4 a `-Os`
con el instrumento del lazo dentro (gated por `log=1`). El IDE, fat-jar de las
17:54. Y desde hoy **ESP-IDF se usa desde aquí** (`C:\esp6.0.1\esp-idf`): el
P4 y el S3 se compilan antes de pedir un flasheo.

### 17-ago (tarde) — LA SESIÓN DE PLACA: la Metro entera, y un cuelgue cazado

**Seis fichas verificadas EN PLACA en una tarde**: `#389` (CastRt, 9 líneas byte
a byte con el opcode nuevo), `#381`/`#428` (LongNat: 8/8 thunks, el `.mdn` del
pipeline enlazado corriendo en ARM), `#430`, `#302` paso 3, `#422` (los dos
caminos) y `#418`. **La Metro queda COMPLETA.**

**Lo gordo fue `#430`**, y no era lo que parecía. La Metro se colgaba muda
ejecutando el sample que venía a probar el escaneo de la pila C — o sea, el
instrumento moría antes de medir. Eduardo lo acotó en tres pasos, ninguno
teórico: quitar el `native` (murió igual ⇒ el nativo, exonerado), llamar al
`gc()` a mano (terminó limpio ⇒ el GC va bien, nadie lo llamaba) y **el test de
desplazamiento**: un gemelo que gasta el doble de handles por vuelta murió a la
mitad de camino. La causa: el disparo del GC contaba VOLUMEN y no SLOTS, la
tabla de handles sólo doblaba, y su salto a 65536 pide 512 KB **de SRAM** (las
tablas salen del malloc de plataforma, no del heap de la VM, que está en PSRAM).
El malloc fallaba y el hook de FreeRTOS parpadea para siempre: un cuelgue, no un
error. Arreglado en las DOS VMs con las tres ideas de Eduardo — la marca al
final de la tabla, el tope por puerto (Pico: 16384 slots) y la **excepción
prefabricada** en el prólogo del RUN, que hace que quedarse sin memoria para
contar el error deje de ser una muerte muda. Ficha completa en `docs/FICHAS.md`.

Herramientas arregladas por el camino: el **Upload del Explorer** subía siempre a
`/app` (sin eso, `#422` y `#418` no se podían ni probar), y **miVM escribía su
diagnóstico de GC por stdout** — cualquier programa que colectara rompía el
invariante en Java.

**📊 LA CUENTA (17-ago, contada del fichero): 17 pendientes de V5** —
12 fichas numeradas + 2 encargos sin número (`List`→`Object` y `Box`) + 3 tareas
de cierre. Veníamos de **45**. Hoy: cerradas 4 (`#430` nació y murió el mismo
día), abierta 1 (`#431`).

**⏭️ AL VOLVER — plan de Eduardo:**

- **Hoy se cierra con `#424`** (los eventos del GUI del P4) y con eso basta:
  cierra el grupo A entero.
- **Otro día, el bloque del IDE** (grupo C / H10: `#425`, `#394`, `#395`,
  `IDE-7` y `Box`). ⚠️ Eduardo trae cambios que añadir a esas fichas — leer lo
  que diga ANTES de planificar el bloque, que el enunciado puede crecer.

Detalle de los dos primeros pasos:

1. **El P4** (lo único que queda del grupo A): remedir `#424` (los eventos del
   GUI, **sin tocar nada primero** — la foto pudo cambiar sola con `#398` y
   `-Os`; si siguen lentos, la prueba de una línea está localizada en
   `gui_display_dsi.c:546`) y anotar las **nuevas líneas base a `-Os`**
   (arranque, árbol, SD: las de estos días eran a `-Og` y ya no valen). El
   guión, en `notas/SESION_PLACA_A.md`.
2. **Grupo B**, ya de escritorio. Estaba empezando `#429` (que el IDE detecte su
   propio compilador rancio: nos costó tiempo dos veces el 16 y el 17-ago). El
   sitio está localizado — `lexer-java/.../Version.java`, que ya sabe de dónde
   salió y de cuándo es; falta la comparación contra el `basicplus-frontend.jar`
   del árbol y el aviso. Lo demás de B: `List`/`SyncList`/`OwnerList` de `any` a
   `Object`, `#412`, `GAP-4` y el **`#431` nuevo** (miVM busca las deps en el
   CWD en vez de junto al `.mod`, y revienta con stack trace de Java).

Estado del repo: todo committeado (6 commits, `1ed8ebc`..`17df39a`), **sin push**
(norma: nada a GitHub hasta cerrar la versión). Imagen de la Metro al día
(`bpvm_pico.uf2`, 17:53) con la anterior guardada como known-good; fat-jar del
IDE de las 17:54 y verificado por conducta.

<!-- Fecha — quién — resumen del traspaso. La entrada más reciente arriba. -->

- **2026-08-17 (2) — ✅ `#389` CERRADA EN HOST: el downcast de `Object` LANZA**
  (`05acc0d`) — el último bug conocido del lenguaje. Opcode nuevo `CHECKCAST`
  (0xAF, las dos VMs): mira sin consumir, null pasa, y el error NOMBRA el tipo
  esperado (el nombre viaja como literal internado — el descriptor no lo
  lleva). De hacerlo salieron dos arreglos más: INSTANCEOF de la VM-C leía el
  class_ptr a ciegas (paridad latente con miVM, que ya validaba) y el despacho
  virtual sobre un Object-con-cadena daba el 504 disfrazado — ahora lanza
  atrapable, mismo mensaje byte a byte. Y una trampa cazada por el reproductor:
  los literales de cadena viven en la región de datos SIN cabecera, y el primer
  intento los rechazaba en `string(o)`.
  Verificado con `CastRt.bp` (9 casos, salida idéntica en las dos VMs), toda la
  batería, y el IDE reconstruido. Falta placa (reflashear: opcode nuevo).
- **2026-08-17 — 🟢 `#302` paso 3 HECHO EN HOST: el test rojo del día anterior,
  VERDE con el diseño de Eduardo.** Escaneo conservador de la pila de C en vez
  del shadow stack: el GC recorre `[su frame .. el techo que apuntó el guard del
  thunk]` con el mismo `mark_recursive` conservador de la pila BP, y un `setjmp`
  vuelca los registros (Boehm). Tres sitios: un campo TLS en el callctx, el
  paso 2d del marcado, y la sincronización de `tc->sp` al entrar al thunk.
  **Por qué esta forma gana**: cero emisor, cero ABI (los `.mdn` grabados quedan
  protegidos sin regenerarlos), cero coste sin AOT, y miVM ni se entera — la
  paridad sigue 28/0/0. Medido: ~180 palabras por colecta, y el rastro marca
  `1 refs` justo en la colecta que antes reciclaba el intermedio.
  El test queda de guardián permanente (`make test-aotgc`); en placa falta ver
  el rastro `pila C del native` con `log=1` — va con las pruebas finales.
  **Con esto, el bug conocido que bloqueaba el cierre de V5 está arreglado.**
- **2026-08-16 — 🔴 `#302` paso 3: EL ARGUMENTO DEL APLAZAMIENTO, REFUTADO CON
  TEST.** Se difirió con «el native corre síncrono sin GC asíncrono y F2 no
  compacta», y las dos patas caducaron en V4: el GC corre DENTRO de la
  alocación (#357) —también desde un helper llamado por código nativo— y
  recicla. `make test-aotgc` (HOY ROJO a propósito: es el criterio de
  aceptación) lo demuestra: `"valor " + intToString(n)` en una native, con GC
  por alocación, imprime DOCE BYTES NUL con status=OK — el intermedio, cuyo
  único handle vive en un temporal de C, se recicla en mitad de la expresión.
  El control interpretado con el mismo GC imprime bien. Corrupción MUDA, y en
  HOST — cae también el «el AOT-en-host la tiene gratis» del doc de diseño
  (anotado allí, conservando el texto original).
  Gravedad hoy: ventana estrecha y natives que apenas encadenan alocaciones —
  pero `#428` acaba de abrir la puerta a cadenas en natives, que es justo el
  patrón vulnerable. El arreglo sigue siendo el diseñado (shadow stack), más
  dos piezas que el experimento añade: sincronizar `tc->sp` al entrar al thunk
  y enraizar los intermedios de expresiones con ≥2 alocaciones.
- **2026-08-16 — ✅ `#428` CERRADA, VERIFICADA EN LA METRO** (`7ddbfec`): una
  `native` puede llevar LITERALES. `RoTest` imprime `valor 7` / `negativo` con
  las cadenas viajando dentro del `.mdn` (188 B). La solución fue la de Eduardo
  —los literales como parte del código— vía un guión de enlace compartido que
  fusiona `.rodata` en `.text` (relocatable comprobado: byte-idéntico a dos
  direcciones). En los dos pipelines; el manual estaba además roto desde V5
  (classpath) y nadie lo notó. AOT en V5 queda: sólo `#302` (raíces GC).
  ⚠️ Matiz apuntado en la ficha: la salida limpia no distingue nativo de
  interpretado (esa es la gracia del degrade); la confirmación de 30 s es un
  Run con `log=1` mirando la línea del loader. Vale también para `#381`.
- **2026-08-16 — ✅ `#381` CERRADA, VERIFICADA EN LA METRO.** `long` en una
  función `native`, con la salida en ARM **byte a byte la del PC** y el `.mdn`
  generado por el propio IDE (8 thunks, 560 B). Lo que confirma: números de más
  de 32 bits, anchos mezclados en una firma, la división y el módulo por helper
  —la idea de Eduardo que evitó enlazar libgcc— y `div0: atrapado`, que es
  dividir por cero desde código nativo sin reiniciar la placa.
  🩸 **Y de camino, ficha nueva `#429`**: el IDE compila con SU copia del
  compilador (el fat-jar lo empaqueta) y **no avisa cuando está rancia**. Costó
  el primer intento de esta prueba: el IDE decía «no puede utilizar long en
  código nativo» con un fat-jar de ayer y el cambio de esta mañana. El aviso
  lleva tiempo en las notas y aun así se escapó — un aviso que hay que recordar
  cada vez ya ha fallado; lo que falta es que el desfase **se detecte y se
  diga**. Modo de fallo malo: no da un error raro, da uno PLAUSIBLE (el mensaje
  correcto de una versión anterior).
  📤 **`#426` (`double` en AOT) sale de los pendientes de V5** y pasa a una
  sección propia de V6, por decisión de Eduardo: lo que no es de esta versión no
  debe engordar su lista.
- **2026-08-16 (noche) — 🏁 `SqlDemo` CORRIENDO CONTRA LA SD, con todo lo de hoy
  dentro.** `exit 0 (OK)`: el pack de SQLite se carga en el primer `Run`,
  publica su API (17 símbolos), el módulo `SQLite.mod` se resuelve **desde la
  zona de packs**, y la demo inserta 6 filas en `/sd/medidas.db`, hace
  agregados y agrupa. Es la prueba que valida la cadena entera.
  🩸 **Y llegar ahí destapó un bug de los buenos** (`1d4ccbf`): la fachada del
  FS **no era coherente consigo misma**. `stat` y `read` consultaban el fallback
  de la zona de packs y **`read_at` no**, así que un módulo del pack existía
  para `stat` y no se podía leer por trozos — y cargar un módulo va por `read_at`
  desde #305. El síntoma era `IO error` sobre un `/app/SQLite.mod` que **no
  existe**, con el firmware avisando de que «el FS eclipsa al del pack» sin que
  hubiera ningún fichero en el FS.
  No era regresión: `read_at` llegó en #305 y el fallback en H4, y nunca se
  juntaron. Sólo aparece con un pack grabado **y** un módulo suyo que no esté
  además en el FS — la combinación que sólo se da usándolo de verdad. La regla
  queda fijada en un test: *si `stat` dice que existe, se tiene que poder leer,
  entero y por trozos*.
  Y `#421` estaba a medias por mi parte: el detalle salía del módulo principal y
  **el fallo real siempre es una dependencia** (el `Core.mod` del 15 y este
  `SQLite.mod`). Ahora vive en `vm->load_error`, junto a `link_error` y
  `runtime_error`, que ya existían para lo mismo.
- **2026-08-16 (tarde) — ⚡ EL ARRANQUE DEL P4: 717 ms → 386 ms, verificado en
  placa** (`bd8a916`). Con la imagen del 15-ago eran 965: **dos veces y media**.
  Y no se optimizó nada — se quitó del arranque lo que no debía estar ahí.
  **Era una decisión de Eduardo que no había viajado entre familias**: el pack
  nativo se carga en el primer `Run` y no al arrancar, *«porque un cuelgue
  durante un Run se arregla desenchufando una vez y uno en el arranque obliga a
  regrabar»*. Estaba escrita en `pico/pack_pico.c` desde el 7-ago y el P4 hacía
  lo contrario: 338 ms de cada arranque, y el único paso que puede colgar puesto
  justo donde no se sale sin regrabar.
  La parte fina fue **qué se mueve**: el log ya separaba las dos mitades
  (`mapear` 0 ms, `barrer` 338 ms). El mapeo se queda —el IDE necesita ver la
  zona desde el arranque— y se retrasa el barrido y el salto.
  🔍 **Y un test evitó un bug**: la idea inicial era «si la zona empieza virgen,
  no busques». `test_npack.c` tiene un caso que pone el pack en el offset 256
  entre basura — el ancla existe precisamente para no depender de dónde esté.
  ⏳ Falta ejercitar la línea del primer `Run` y que `PACK_LS` siga viendo la
  zona sin Run previo.
  **Van tres arreglos en dos días del mismo tipo**: algo que se decidió o se
  arregló en una familia y no llegó a otra (el corte del CRC de la SD, el log
  propio del Pico, y esto). Empieza a merecer una revisión sistemática, no
  seguir cazándolos de uno en uno.
- **2026-08-16 — Eduardo + Claude. Tres cerradas, y el grupo de «módulos y
  arranque» baja de cinco a dos.**
  ✅ **`#423` y `#420`, VERIFICADAS EN LA P4** con la imagen nueva. Eduardo:
  *«con log=0 no muestra mensajes de ejecución y con log=1 sí; los mensajes de
  arranque se mantienen siempre»* — el contrato de las tres partes, cumplido. Y
  esa misma prueba cierra `#420`: para que con `log=1` aparezca rastro de
  EJECUCIÓN tiene que estar conectado el sink del diagnóstico de la VM, que es
  lo que a esa familia le faltaba.
  ✅ **`#421`** (`e62a7fc`): los cuatro fallos de carga que antes decían
  `IO error` ahora dicen cosas distintas **y viajan por el wire** — al log ya
  iban desde ayer; lo que faltaba era que llegaran al IDE. El caso que costó la
  mañana del 15-ago («se lee pero no cuadra con su cabecera: truncado o de otra
  versión») se probó cortando un `.mod` por la mitad contra el simulador.
  ✅ **`#381` completa en host** (`072c864`): las conversiones numéricas dentro
  de una nativa. Sólo le falta la placa.
  Lo que enseñó el día: **el patrón que se repite es «el sistema lo sabe y no lo
  dice»** — el log que no existía en el P4, el motivo de carga que se quedaba en
  el firmware, el CRC que nadie pedía y todos pagaban. Tres fichas distintas, un
  solo tipo de bug.
- **2026-08-15 (noche, 2) — Eduardo + Claude. 🟢 `long` YA CRUZA A UNA FUNCIÓN
  `native`** (`f599574`, `bd5002f`). Hasta hoy el AOT sólo marshallaba 4 bytes.
  **Dos decisiones de Eduardo hicieron el trabajo, y las dos ahorraron camino:**
  1. *«long es una cosa y double otra; empecemos por long»*. La ficha decía
     «long, double y float JUNTOS» y la medida le dio la razón: compilando lo
     que emite el AOT con los flags reales, `long` `+ - *` no deja **ni un**
     símbolo sin resolver (GCC lo hace en línea) y `double` llama a libgcc para
     casi todo. Comparten el marshalling y nada más. `double` → ficha `#426`.
  2. Para la división —lo único de `long` que llamaba a `__aeabi_ldivmod`—:
     *«¿y si la reemplazamos en el emisor por una llamada a una función?»*. Y
     resultó que **ni siquiera hay que escribir una división por software**: el
     que no puede llamar a libgcc es el `.mdn`, no el runtime. Así que
     `idiv64`/`imod64` van en la tabla de helpers y el módulo nativo queda
     limpio — sin tocar el pipeline de ninguna arquitectura.
  Salió barato porque tres piezas ya estaban puestas: la pila BP ya guarda los
  `long` como 8 bytes big-endian (misma representación que el intérprete), el
  thunk ya movía 8 bytes con las refs (#302), y la tabla de helpers está hecha
  para crecer por el final.
  **Verificado** con `make test-longnat` (nuevo): la salida por los thunks AOT
  es idéntica a la de la VM-Java con 2^40, anchos mezclados en una firma,
  negativos, el máximo de 64 bits, llamadas encadenadas, división, módulo y
  división por cero **atrapada con `try/catch`** desde código nativo. El objeto
  ARM real no deja un solo símbolo indefinido. Más `test-bytenat`,
  `test-compressnat`, `test-callbp`, `test-throwmsg`, paridad 28/0/0,
  frontend 104/104, miVM 34/34.
  ⏭️ **Falta**: el cast `integer(x)` dentro de una `native`, y **probarlo en
  placa** — el `.mdn` sólo se carga de verdad allí. Análisis y plan en
  `docs/AOT_ABI8_IDEAS.md`.
- **2026-08-15 (noche) — Eduardo + Claude. ⚡ EL ÁRBOL DEL IDE: 6953 ms → 155 ms
  (45×), verificado en la P4.** Y el arranque, de paso, 965 → 717 ms.
  **Cómo se llegó, que es lo que hay que repetir**: Eduardo dijo *«no hace falta
  especular, lo podemos medir; lo que hace falta es que el log lo registre»*. Se
  instrumentó el refresco en los dos extremos (`b44f15e`) y el instrumento
  contestó a la primera: **el CRC era el 98-99 % del tiempo**. Antes de eso, la
  hipótesis en la mesa era el arranque —y la medida la había descartado ya: 965
  ms hasta el wire, 266 de ellos por la tarjeta—. **Dos arreglos evitados por
  medir, uno acertado por medir.**
  **La causa no era «el CRC es caro»**, y ahí está la lección: `bpvm_fs_crc32`
  troceaba el fichero de 256 en 256 B y cada trozo iba por `read_at`, que recibe
  el PATH — o sea que **cada 256 B se abría el fichero otra vez**. 5432 aperturas
  para 1,3 MB, con un `f_lseek` que recorre la FAT desde el principio: cuadrático
  con el tamaño. El dato que lo delató fue una rareza en los números: **el flash
  interno iba tres veces más lento que la SD**, lo que ya decía que el cuello no
  era leer.
  Arreglado en dos mitades: `f4e5c1f` (el backend calcula el CRC con UNA
  apertura; 16,5× medido en el PC sobre littlefs, los tres backends) y `10b4467`
  (el listado no calcula CRC; se pide con `STAT {crc:true}` justo antes de subir
  ese fichero). Verificado contra el **simulador** —LIST, STAT y el valor
  idéntico a `java.util.zip.CRC32`—, con `sim-smoke`/`boardsim-smoke`, la
  batería del FS, paridad 28/0/0, y **el firmware de la Metro construido con el
  toolchain ARM**.
  🩸 **Y por qué la P4 sufría más que la Metro**: el corte que evitaba calcular
  el CRC de los volúmenes montados estaba **sólo en el Pico** desde V5/H2. La
  familia ESP32 nunca lo recibió. *Un arreglo que no viaja entre familias es
  medio arreglo* — van ya unos cuantos.
  🔸 De rebote, el `ESP_ERR_TIMEOUT` del montaje no ha vuelto a aparecer. **No se
  da por muerto**: era intermitente y una pasada buena no prueba nada; la
  hipótesis (y cómo confirmarla) está en la ficha.
  🔸 **El tramo más caro del arranque es ahora otro**: 337 ms escaneando la zona
  de packs para encontrar `0 candidatos`, casi la mitad de los 717 ms.
- **2026-08-15 (tarde, 3) — Eduardo + Claude. 🏁 H11 (PACKS) CERRADO.** Las
  cuatro fichas que colgaban de él, resueltas: `#417` y `#414` verificadas en
  placa, `#365` verificada en las dos VMs, `#411` en su parte de packs — y
  **`PACK_CALL` (#383) CANCELADA**.
  **Dos decisiones de alcance de Eduardo, y las dos son la misma idea**: sacar
  del hito lo que no era suyo. La **limpieza de `notas/`** (5 carpetas, ~56 MB)
  no es trabajo de packs sino de cierre de versión, y estaba trabando el hito;
  se movió a su sitio. Y **`PACK_CALL`** —llamar a un pack sin AOT— se cancela:
  *«estos packs los hacemos nosotros, así que el sistema actual está bien»*. Lo
  que compraba era que **mantener** un pack nativo no exigiera los dos toolchains
  cruzados, y como el único que publica packs nativos es el proyecto, esa barrera
  no existe en la práctica.
  ⚠️ **Lo que eso deja aceptado, y conviene tenerlo escrito**: en un pack nativo
  el AOT **no es una optimización, es un requisito** — sin `.mdn` para esa
  arquitectura, sus funciones lanzan. Y el AOT **es mudo por línea de comandos**:
  si no puede generarlos, el pack sale más pequeño sin decir nada. Eso deja de
  ser «algo que PACK_CALL arreglará» y pasa a ser definitivo. El comentario del
  parser que lo daba por futuro está corregido.
  **Queda H10 (IDE) como único hito abierto de V5.**
- **2026-08-15 (tarde, 2) — Eduardo + Claude. ✅ `#365` CERRADA** (`88e75a4`): un
  módulo con `library` ya puede **arrancar** un pack. No era «`library` +
  `out:pack` es imposible», como decía la ficha: era el arranque. El `.mod` de un
  módulo con `library` se llama `com.example.Demo.mod` —así se llama su entrada—
  y el manifest escribía `main=Demo`, que es el nombre del FICHERO FUENTE; quien
  arranca busca literal y no lo encontraba.
  **La solución es de Eduardo** («¿y si ponemos `library` dentro del manifest?»),
  y entre las dos formas se eligió la que **no toca las VMs**: el manifest lleva
  ya el nombre canónico en `main=`, en vez de un campo `library=` que las dos VMs
  tuvieran que concatenar — *dos implementaciones haciendo la misma cuenta es
  donde el invariante se rompe*. Coste en runtime: **cero líneas**. El manifest
  es un artefacto generado, y puede llevar el nombre resuelto.
  De camino apareció una trampa muda: la regla de la doble extensión
  (`sqlite.npk.RISCV`) se comía los nombres cualificados que acaban en un tipo
  (`com.example.Npk.mod` → `com.example.mod`), en silencio y dentro de un pack ya
  grabado. Arreglada con la condición que separa los dos casos.
  Verificado de punta a punta: `samples/packlib/` corre igual en **las dos VMs**,
  104/104 frontend (2 tests nuevos), 34/34 miVM, paridad 28/0/0, y el
  `SQLite.pack` real reconstruye sus 9 entradas idénticas.
  Con ésta, **H11 se quedó a una ficha** — y se cerró esa misma tarde (arriba).
- **2026-08-15 (tarde) — Eduardo + Claude. 🏁 EL BUS DE LA SD DEL P4 ES SANO.**
  `samples/BusTest.bp` en la tarjeta: **2048 KB de patrón conocido, ida y vuelta,
  0 diferencias**, 4 bits, **20 MHz**. Era la ficha que más pesaba de la tanda —
  la condición previa para poner SQLite encima de esa tarjeta— y queda cerrada.
  El instrumento se validó antes con un control **en rojo** (meterle al fichero 5
  el contenido del 6: lo cazó por el byte 2, que es donde va el número dentro del
  patrón). *Se reabre si se sube el reloj*: un bus marginal aguanta despacio y
  falla arriba.
  **Y el instrumento tenía un fallo que casi deja la medida sin valor** (`0a4e25c`):
  el log de arranque imprimía `pines.khz` —lo que PIDE el env—, y con el env
  vacío eso sale `| 0 kHz`. La prueba estaba hecha y no se podía decir a qué
  velocidad. El driver resuelve `khz > 0 ? khz : SDIO_KHZ_POR_DEFECTO`, así que
  fueron 20 MHz; ahora el log hace la misma cuenta y dice de dónde sale el
  número, y la constante vive en `blk_sdmmc_p4.h` en vez de escondida en el `.c`.
  ⏳ **sin compilar** (no hay ESP-IDF en esta máquina).
  Lo que enseñó la tarde: **un número imposible en un log no es cosmética, es
  el log diciendo que no sabe de qué habla**. `0 kHz` no es «no lo sé», es un
  dato falso — y estuvo ahí, leído varias veces, hasta que hizo falta anotarlo.
  Un chivato que anuncia un valor que no existe es peor que no tener chivato.
  **Abierta `#423`**, que salió del pie de ese mismo log (`[LOG OVERFLOW]`): el
  GC escribe **3 líneas por colecta** (~300 B) y el log mide **8 KB** en el P4 y
  **4 KB** en el S3 → **~26 colectas y está lleno**. Está en las cuatro familias.
  Lo grave no es que se llene: `append_raw` es append-only y **se calla por el
  final**, o sea que el log de una placa colgada contiene el arranque y no el
  cuelgue. Decisión de Eduardo pendiente — esas líneas son el instrumento con el
  que se cazaron #355 y #357.
- **2026-08-15 (mañana) — Eduardo + Claude.** Tanda de placa y de packs. Cerradas
  **`#414`** (módulo `Packs`: `list()` / `listIn()`, verificado en el P4 el mismo
  día que se escribió), **`H2-P4`** (las seis operaciones del FS, en los DOS
  volúmenes) y **`#411`** en su parte de carpeta: `bpstdlib/sqlite/` con fuentes,
  los cuatro nativos versionados y un `LEEME` con la cadena — **el pack ya se
  puede reconstruir desde un clon limpio**, que antes no. Arregladas además
  `#418` (el resolutor mira `/sys`) y **`#420`, el P4 era la única familia sin
  log de EJECUCIÓN**.
  Lo que enseñó la mañana, y es una sola cosa dicha de cuatro maneras:
  - **el instrumento cómodo no dice la verdad sobre la placa**. El FS del host
    daba 11/11 y tapaba que en littlefs `mtime` no existe; lo destapó
    `--fs=lfs:`, que usa el mismo motor que el micro;
  - **lo que no se compara, se pudre**: un `/lib/Core.mod` rancio del mismo
    tamaño que el bueno costó media mañana, y el IDE nunca mira el CRC de `/lib`
    porque sube a `/app` (#422);
  - **la red de seguridad hizo su trabajo**: el pack de SQLite no salió igual dos
    veces seguidas, y por eso no se borró nada de `notas/`;
  - y las tres fichas que se cerraron salieron de **predecir desde el código y
    usar la placa para confirmar**. Las horas se fueron en lo contrario.
  Pendiente de la tanda: `#418` en placa y `H2-P5` (otras tarjetas). **La prueba
  del bus sano — la que más pesaba — se cerró al día siguiente** (ver la entrada
  del 15-ago).
- **2026-08-14 (noche) — Eduardo + Claude. 🏁 H9 CERRADO.** `Object` deja de ser un
  alias de `any` y pasa a ser la raíz REAL del modelo de objetos (existía desde
  H5.1.a, pero sólo en el emisor: al semántico nadie se la había presentado).
  Subir es implícito, bajar se escribe con el nombre del tipo —`Cosa(o)`,
  `string(o)`— que es la regla que ya regía entre primitivos (`byte(someInt)`).
  **#389 queda pendiente**: falta que la conversión COMPRUEBE en ejecución.
  Antes se guardó el trabajo suelto en 6 commits (el WIP del P4 del 12-ago, la
  documentación) y se arregló `test-pack`, que no enlazaba desde #362.
  Lo que enseñó la tanda, y conviene no olvidar:
  - **el desfase de `.mod` ANESTESIA los cambios**: con la stdlib vieja (interfaz
    `any`, que traga cualquier cosa) la suite y la paridad daban verde con
    `string → Object` sin implementar. No se vio hasta regenerar la stdlib;
  - **un SKIP no es un PASS**: el arnés dijo "VERDE" con 3 SKIP mientras tres
    samples no compilaban;
  - **el censo sólo vale con el directorio de salida BORRADO** (el compilador es
    incremental): mintió tres veces en el mismo día, una de ellas diciendo "2 de
    26 módulos" cuando eran 26;
  - **prueba fuerte que sí sirvió**: 24 de 26 `.mod` byte-idénticos, y los 2 que
    cambian sólo en la interfaz — `+69 = 23×3` y `+15 = 5×3`, exactamente lo que
    crece `"any"` al pasar a `"Object"`. Cero `any` en las interfaces publicadas.
- **2026-08-14 (tarde) — Eduardo + Claude.** Reescrito este documento con datos del
  repo en vez del andamiaje inicial. Dos cosas que decía y **eran falsas**:
  (a) **#310 no está abierto** — se cerró en V4 y está verificado en las tres
  familias (`16c7970`, `237c963`, `677bcf2` *"batería estándar del P4 al completo —
  8/8, y #310 en las tres familias"*); (b) el **check de `.pack` ya está duplicado**
  en cuatro sitios, no vive sólo en el CLI de la VM-C. Añadido lo que faltaba de
  los días 12-14: H8, la tanda H9, #390, #362 y #403.
- **2026-08-14 — Eduardo.** V4 dada por cerrada. En V5 se añadió soporte de SQLite
  y ha quedado bastante bien.
- **2026-08-14 — (andamiaje inicial)** — este ESTADO.md se creó a partir de
  `README.es.md`, `PHILOSOPHY.md` y `PENDIENTES.md`. Sus secciones "En curso" y
  "Próximos pasos" eran semilla sin verificar; corregidas arriba.
