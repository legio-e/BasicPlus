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

## 🧊 CODE FREEZE V5 — desde el 18-ago-2026

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

## ABIERTAS

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

### IDE — V5/H10 «lo pendiente que no son bugs»

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

### Familias — lo que dejó el censo (`#427`)

- **🕳️ [S3] los packs: encaminados pero SIN REGIÓN — el mismo agujero que `#327` en la
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
  eso. La otra mitad de la ficha —el árbol en la P4— sigue sin medir.
  🔎 **Y de camino salió esto**: `save` responde «FS guardado en flash» y nada más, pero
  el firmware **sí se cronometra** y manda `durationMs` en el `SAVE_REPLY`
  (`pico/repl_v1.c:929-943`). **El dato viaja por el wire y el IDE lo descarta.**
  Enseñarlo no cuesta nada y es exactamente la medida que esta ficha pedía. No es un
  bug, así que con el freeze va a V6 → ver «Aplazadas».
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
### Pulido (no urgente) — subidos desde `PENDIENTES` el 17-ago

> Estaban en `PENDIENTES.md`, que es documentación de cara al usuario. Tienen estado de
> trabajo («hay que hacer X»), así que su sitio es éste.

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

### AOT / native

- **🐛 [AOT] `native` en un MÉTODO se ignora en SILENCIO** — encontrado el 21-ago, y lo
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

### Arrastres de V4 y varios

- ~~`#412`~~ — **MOVIDA A V6** el 17-ago por decisión de Eduardo (*«puede ir a
  V6, no es nada urgente ni crítico»*). El diseño quedó CERRADO antes de moverla
  y está en `notas/V6_IDEAS.md`: el argumento **siempre en el heap** (idea de
  Eduardo), que además borra una asimetría de fondo — hoy el argumento horneado
  es un literal de la zona de datos y uno de ejecución sería del heap, las dos
  formas de cadena que dieron guerra en `#389`. Lo que la saca de V5 no es el
  mecanismo (una línea en el emisor + un builtin ×2) sino que **abre el
  protocolo del wire**, con cuatro implementadores.
### Cola de H2 (la SD), anotada al cerrarlo el 8-ago

- **`H2-P5` (exFAT / «superfloppy») — CERRADO el 21-ago con una PRUEBA, no con una
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
- `H2-P5` — **variedad de tarjetas.** 🟢 **El enunciado original ya NO aplica**: era
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

### 🔜 Aplazadas a V6 — NO cuentan como pendientes de V5

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

**1. `native` en un MÉTODO — arreglarlo, no sólo avisar.** Hoy se ignora en silencio (ver
la ficha aparte). Son dos cosas y en este orden: que **AVISE** —barato, y convierte una
mentira muda en una línea— y luego **abrir el barrido** de `AotCEmitter.java:259` a los
métodos de las clases, pasando el objeto como primer parámetro. Ojo: `MemberAccessExpr`
ya está soportado, así que la parte que parecía difícil (`this`) puede que no lo sea.

**2. `double`.** Diseño hecho en `notas/V6_IDEAS.md` §double, con la ganancia estimada
sobre datos reales. Es la ficha `#426`.
⚠️ Con el matiz que ya está escrito en `AOT_LIMITES.md` y no hay que perder: la FPU de
Cortex-M33 es de precisión **simple**, así que un `double` no toca la FPU en dos de las
tres familias. Soportarlo es correcto; **prometer velocidad con él, no**.

**3. Los statements sencillos.** Del censo de `AOT_LIMITES.md`, por relación
esfuerzo/cobertura: **`print`** (llamada al runtime que ya existe), **`null`** (un cero),
**`do…loop`** (un `while` al revés), **literales de array**. Las cuatro son azúcar y
ninguna estaba documentada como límite hasta el censo del 21-ago.

⏭️ **Y una cuarta que propongo, porque si no las otras tres se pudren**: un **test que
recorra los nodos del AST** y compruebe cuáles pasan por el AOT. Mientras el emisor tenga
rechazos genéricos (`statement no soportado`), cualquier lista escrita a mano se queda
rancia sola — el propio `AOT_LIMITES.md` nombraba cinco cuando eran veinticuatro. Con el
test, la lista se mide en cada batería en vez de recordarse.

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
