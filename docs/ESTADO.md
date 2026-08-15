# BasicPlus — Estado vivo

> **Documento vivo de traspaso entre sesiones.** Se lee al empezar y se actualiza
> al terminar. Aquí va lo que está EN CURSO y lo que ACECHA; los bugs formales
> viven en `PENDIENTES.md` y las decisiones de fondo en `PHILOSOPHY.md`.
>
> Convención de traspaso: al cerrar una tanda, anota en "Última sesión" qué quedó
> cerrado, qué a medias y qué riesgo hay abierto. Sé concreto (issue, fichero, prueba).

---

## Versión y foco actual

- **V4 — CERRADA y publicada** (release v4.0, 6-ago). Lo caro fue reestructurar la
  **gestión de memoria** (referencias por handle con contador de generación) y el
  **sistema de ficheros** (littlefs en las tres familias).
- **V5 — EN CURSO**, y corta por decisión: SQLite + ORM, el traslado al P4, los
  pendientes y cositas del IDE. Lo grande (partir común/hardware) es V6.
- 📋 **LAS FICHAS ESTÁN EN `notas/FICHAS.md`** (creado el 14-ago). Es el registro
  numerado —abiertas, cerradas con su commit, y las que no se han podido
  catalogar— y se mantiene a mano: una línea al abrir, la marca y el commit al
  cerrar. **Antes no existía**: las fichas vivían en la lista de tareas de una
  sesión, y por eso el 14-ago se dio una lista de 11 pendientes cuando había 36,
  se dio por pendiente lo ya hecho (H7) y lo que se decidió no hacer (la release
  4.0.1). Un número de ficha que no se puede buscar no sirve de nada.
- ⚠️ **El resto del registro vivo de V5 está FUERA del repo**: `notas/V5_BACKLOG.md`
  y `notas/V5_IDEAS.md` (commit `62d4b3b`: de la versión EN CURSO no se publica
  nada). Ojo: el backlog está **parado el 12-ago** y sus "pendientes" pueden estar
  hechos o anulados — contrastar con `FICHAS.md` y el `git log` antes de creerlo.
  Y ese backlog **se quedó el 12-ago**: H8 entero, la tanda H9 y todo lo del 13 y
  14-ago sólo constan en el `git log`. Si hace falta el detalle de esos dos días,
  está en los mensajes de commit, que son largos a propósito.

### Hitos de V5 cerrados

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

*(El nombre de H8/H9 no está en ningún doc: sale de los prefijos de commit. Ojo con
confundir el H9 de V5 con el H9 de V4, que era el kernel por capas.)*

## En curso / a medias

- **🟠 #389 — falta la COMPROBACIÓN EN EJECUCIÓN.** Lo hecho el 14-ago es la mitad
  estática: `Object` es la raíz de verdad, subir es implícito y **bajar hay que
  escribirlo** (`Cosa(o)`, `string(o)`, misma forma que `byte(someInt)`). Pero esa
  conversión **sólo fija el tipo**: si el DAO dice `Cosa` y devuelve `Otra`, sigue
  saliendo el **504**. Lo que falta es que lance, sobre `instanceof` (#52, ya
  existe), y eso toca **las dos VMs y el AOT** — por eso salió de H9.
  Con ella van dos hermanos medidos: `toString()`/`compareTo()` sobre un `Object`
  que lleva una **cadena** no tienen vtable que despachar (una cadena es
  referencia pero no desciende de `Object`); las dos VMs *pueden* detectarlo,
  porque el tag del handle y el tipo del bloque distinguen array de objeto.
  Diseño de fondo en `docs/OBJECT_COMODIN.md`.
- **La clase contenedora (`Box`) sigue SIN escribir.** Es la otra mitad del reparto
  de Eduardo —**envolver = librería** (una clase aparte con `set` sobrecargado,
  posible desde que #387 arregló la sobrecarga cross-module); **desenvolver con
  seguridad = lenguaje**—. Sin ella, meter un escalar en un `Object` obliga a
  escribir `Integer(5)` a mano. Decisiones abiertas: el nombre, si distingue
  «vacío» de `null`, y cómo se saca un escalar (BP no sobrecarga por retorno, así
  que o N getters con nombre o `get(porDefecto: integer)`).
- **`List` / `SyncList` / `OwnerList` siguen con firmas `any`.** Eduardo pidió
  pasarlas a `Object` (`SemanticAnalyzer`, 15 apariciones de `AnyType.INSTANCE`
  escritas a mano). `Map` ya está en `Object` y no se toca. ⚠️ El día que se haga,
  `samples/AnyNumGc.bp` se queda sin sujeto: hoy prueba que un escalar crudo en un
  slot trazado no descarrila el GC, y ese camino sólo existe ya por `List.add`.
- **El WIP del P4 estaba sin commitear desde el 12-ago**; se guardó el 14
  (`c917bd6`). Ojo al leer ese commit: dice "en placa, nada" porque en ESA tanda
  no se tocó placa, **no porque el trabajo estuviera sin probar**.
- **La S3 (Xtensa) es la única familia que no corre esto.** Está sólo *compilada*:
  se le dio de alta `bpvm_bios_fs.c` en su build, pero **no tiene `bios_s3.c`**,
  así que no ofrece tabla BIOS y no puede alojar un pack nativo. Es una familia
  por hacer, no una prueba pendiente. *(Y el firmware ya responde bien a eso:
  `ramBase == 0` significa «esta placa no da RAM a packs nativos», y el IDE lo
  dice en vez de grabar un motor que no arrancaría.)*
- **#362 (recursos desde la zona de packs) no se ha probado en placa.** Verde en
  host: `make test-packres` 12/12, end-to-end con un PNG real y control en rojo,
  paridad dual-VM 28/28. Su propio commit lo dice: las cinco cinturas montan zona
  por el mismo `bpvm_pack_mount`, *"pero eso es un argumento, no una medida"*.

### 📦 El pack de SQLite: UNO, con las dos familias dentro

Eduardo, 14-ago — está cerrado y conviene que no se vuelva a dar por pendiente:

- **`SQLite.pack` lleva ya las DOS familias** (ARM y RISC-V) en el mismo pack.
- **El IDE lo REHACE al grabarlo** en el micro: poda y deja sólo el código BP y el
  nativo que le toca a esa placa. Por eso el log de grabado dice el tamaño podado
  y no el del fichero (`ddb3ddc`): 1.122.304 B en disco → 569.344 en la placa.
- **Probado y verificado en la Metro y en el P4**, y `SqlDemo` ejecutada en las
  dos familias.

Esto cierra H7 y, con él, la cadena entera: un mismo pack con dos arquitecturas,
relocalizado al grabar, corriendo en dos silicios distintos.

## Riesgos / decisiones pendientes (atacar ANTES de seguir)

- **El check de `.pack` YA está duplicado** (esto corrige lo que decía la versión
  anterior de este documento: no vive sólo en el CLI de la VM-C). Comprobado el
  14-ago, la misma regla escrita a mano en cuatro sitios:
  `bpgenvm-c/src/bpvm.c:828`, `miVM/src/main/java/edu/bpgenvm/Main.java:378`,
  `BpIde/.../FrmMain.java:2556` y `:3165`, `BpIde/.../SimRunner.java:101`.
  Si el criterio cambia (mayúsculas, un pack sin extensión), hay que tocar los
  cuatro y el que se olvide no fallará al compilar. *(Cosa distinta, no confundir:
  la construcción del nombre en el frontend — `lexer-java/.../Main.java:1586,1682`
  y `PackStep.java:235`.)*
- **`dist/BasicPlus-4.0-win/packs/Stdlib.pack` queda rancio** respecto al layout
  nuevo de `Collections.Map` (#390: `layout 8 10` → `8 12`). Se dejó a propósito
  —es una distribución construida de V4, no una fuente— pero es exactamente el
  desfase de `.mod`/pack que ya ha costado tiempo otras veces. `packs/Stdlib.pack`
  sí se regeneró (`9f95e92`).
- **Sobrante que confunde:** `docs/390-private-wip.patch` (14-ago) es un borrador
  de #390 superado por el commit `0b258d3`; comprobado que **no aplica ni hacia
  delante ni al revés**. Borrarlo.
- **Ruido en las búsquedas:** `miVM/.claude/worktrees/jolly-blackburn-9ec483/`
  (29-jul) es una copia completa del repo — todo `grep` global sale por duplicado.
- **`docs/V4_SAMPLES_ROJOS.md`** (sin commitear) es un censo del **15-jul** (343
  samples: 225 verdes, 80 skip-HW, 36 rojos). O se rehace o se marca como
  histórico: tal cual, no dice el estado de hoy de nada.

## Bugs abiertos que rozan el invariante (resumen — detalle en PENDIENTES.md)

- **GAP-4** — `bpvm_format_double` (VM-C, `interp.c:321`) es byte-idéntico a
  `formatBpDouble` (Java) en punto fijo, pero la **notación científica** para
  `|x| >= 1e12` o `0 < |x| < 1e-6` es un TODO → las dos VMs podrían NO ser
  byte-idénticas en doubles extremos. Raro de disparar pero toca el invariante sagrado.
  *(Ojo: `%lld` no va en newlib-nano del STM32; hoy mitigado con `u64_dec`.)*
- **N-readfile-msg-skew** — el **texto** del `RuntimeError` de `readFile(ausente)`
  difiere entre VMs (`x` vs `no se pudo abrir`). Si un programa atrapa e imprime
  `e.msg`, la salida no es byte-idéntica. Alinear el wording en miVM y/o
  `bpgenvm-c/src/builtins.c`.

## ⏭️ SIGUIENTE SESIÓN: tanda de verificación en placa

Decisión de Eduardo (14-ago): *«la siguiente sesión verificamos en la Metro y en
la P4 lo que esté pendiente, y así cerramos entradas y desbloqueamos lo
siguiente»*. Es lo que más desbloquea por hora invertida: casi todo lo abierto de
packs y de la SD espera una medida, no código.

🆕 **15-ago (sábado)** — dos cosas nuevas que se prueban en esta misma tanda:
- **`#418`** (el resolutor mira `/sys`): subir un módulo SÓLO a `/sys` y ver que
  un `import` lo encuentra. En host no se pudo probar — `/sys` es del device.
- ✅ **`#414` CERRADA** — verificada en el P4 el mismo 15-ago: `PacksDemo` listó
  `SQLite` (7 entradas) y `test1` (3), y el filtro por extensión dio 4 módulos y
  1. Los conteos **cuadran con lo que el IDE lee por el wire**, que es un camino
  distinto. *(Recordatorio para la próxima: los builtins son código de la VM-C, o
  sea que probar esto exige reflashear.)*

**Antes de tocar placa** — reconstruir los firmwares desde el árbol limpio. El
trabajo del P4 se commiteó el 14-ago (`c917bd6`) y **nadie lo ha compilado desde
cero**: lo que se probó vivía en el árbol de Eduardo sin commitear. Y ojo con la
stdlib, que se regeneró ese día (`Collections.mod` y `Gui.mod` cambiaron): el
grupo embebido en los firmwares es Core y los drivers, así que *no debería*
afectar — pero eso es un argumento, no una medida.

✅ **`#417` YA ESTÁ HECHA** (14-ago, en el P4): un pack con una fuente `.bin`
dentro, y `FontLoadDemo` cargándola. Los recursos de la zona de packs funcionan en
placa, y **H11 queda desbloqueado**. Detalle de cómo se verificó —y por qué el
instrumento obvio no valía— en `notas/FICHAS.md`.

**En la Metro (RP2350)**
1. **`H2-P4` — las seis operaciones del FS que nunca se han ejecutado**: `remove`,
   `rename`, `mkdir`, `rmdir`, `mtime_ms` y el `write` en modo *append*.
3. **`H2-P5` — una segunda tarjeta**: SDSC (direcciona por BYTE, camino distinto
   en `bpvm_sd_leer_bloque`) y/o sin MBR.
4. **`#415`** — comprobar si el grupo 1 de la stdlib es el que debe ser (la Pico
   perdió `Math` e `IO`).

**En el P4**
1. ~~**La prueba del bus SANO**~~ — ✅ **CERRADA el 15-ago**. `samples/BusTest.bp`:
   **2048 KB de patrón conocido, ida y vuelta, 0 diferencias**, en 4 bits y **a
   20 MHz** (el env no fija `khz`, así que el driver aplica su defecto — el
   conservador que eligieron los pull-ups de 51 K). El instrumento se validó
   antes con un control **en rojo**. Con esto ya se puede poner SQLite encima de
   esa tarjeta: era la condición previa.
   *Se reabre si se sube el reloj* — un bus marginal aguanta despacio y falla
   arriba, y ese es el caso que esta prueba existe para pillar.
2. **`#379`** — mirar si el `INFO` sigue perdiendo la respuesta, y cronometrar la
   respuesta **en el firmware**, no en el IDE: son dos fallos distintos (tarda más
   que su timeout / se pierde) y sólo esa medida los separa.
3. **`#419`** — con y sin tarjeta: **sin SD arranca sin errores y más rápido**, y
   el árbol del IDE también refresca antes (observado el 14-ago). Medir dónde se
   va el tiempo antes de tocar nada — se junta con `#408`.

**Y una ficha que salió de rebote y no necesita placa: `#418`.** Los módulos que
viven en `/sys` **no se encuentran**: el resolutor mira basedir → tal cual →
`/app` → `/lib`, y `/sys` sólo se usa para `auto.txt`. Un `Core.mod` ahí es
invisible para un `import`, y el síntoma engaña —el IDE dice `exit 1 (IO error)`,
que en realidad es el guardián del enlace diciendo que falta un módulo—. Decisión
de Eduardo: **tiene que poder encontrarlos**, o sea que el arreglo va en el
resolutor. Es trabajo de escritorio y desatasca cualquier prueba en placa.

Todo lo demás (fichas de compilador, IDE, AOT) puede esperar: no depende de tener
la placa delante. Detalle de cada ficha en `notas/FICHAS.md`.

## Próximos pasos

1. **La clase contenedora (`Box`)** — es librería, no toca el compilador, y es lo
   que hace llevadero tener que escribir `Integer(5)`. Empezar por ahí.
2. **#389, la comprobación en ejecución**: que el downcast LANCE. Toca las dos VMs
   y el AOT; verificar con `diag/orm-slots/ProbeMal.bp`, que es el reproductor y
   hoy da error de compilación (antes se lo tragaba mudo).
3. **`List`/`SyncList`/`OwnerList` de `any` a `Object`** (lo pidió Eduardo). Ojo a
   lo que eso le hace a `samples/AnyNumGc.bp`.
4. **Cerrar H7**: confirmar si falta el pack con las DOS arquitecturas dentro; el
   resto está verificado en el P4.
5. **Probar #362 en placa** (la zona de packs sirviendo recursos).
6. (Backlog) árbol perezoso del IDE con `LIST_DIR`; **#379** (timeout de INFO en el
   P4, arrastrado y se recupera solo); la media flash del P4 (aparcada a propósito:
   32 MB físicos, bootloader configurado para 16).

## 🧹 Al CERRAR V5 (van con el push, no antes)

Por la norma: nada sube a GitHub hasta terminar la versión, así que la limpieza
se hace en ese mismo momento y no a trozos por el camino.

- **Limpiar `notas/`** — cinco carpetas de experimentos, ~56 MB (`metro-h4`, `p4`,
  `v5-salto-crudo`, `v5-sqlite-prueba`, `v5-sqlite_edu`). Decisión de Eduardo
  (15-ago): borrarlas, pero al cerrar. Lo único no duplicado son los `.elf` de
  SQLite, regenerables del amalgama con los toolchains. Ver `notas/FICHAS.md`
  (#411), donde está el inventario de qué se va.
- **Los sobrantes del árbol**: `docs/390-private-wip.patch` (borrador superado),
  `miVM/.claude/worktrees/` (copia completa del repo que duplica todo `grep`),
  `docs/V4_SAMPLES_ROJOS.md` (censo del 15-jul), y los artefactos sueltos
  (`.slots`, `fc.txt`, `ff.txt`, `fileio_test.txt`, `auto.txt`, `bigfile.bin`).
- **`dist/BasicPlus-4.0-win/`** lleva la distribución construida de V4 —
  compilador y `Stdlib.pack` viejos. Correcto que esté congelada, pero conviene
  decidir si se regenera o se retira al publicar.
- **Y el registro**: `notas/FICHAS.md` y el backlog dejan de ser «promesa» al
  cerrar la versión y pueden subir con ella.

## Ideas aparcadas (no urgente)

- **Prueba de resistencia larga** (post-V4): dejar una placa corriendo días con
  carga VARIADA (no un bucle), con marca periódica en el log-anillo de flash para
  que "la muerte deje rastro". Antes, la versión barata en host. (Ver PENDIENTES.md.)

---

## Última sesión

<!-- Fecha — quién — resumen del traspaso. La entrada más reciente arriba. -->

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
