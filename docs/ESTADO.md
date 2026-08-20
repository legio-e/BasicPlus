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
