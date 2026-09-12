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

## ⏭️ AL RETOMAR (13-sep) — ✅ **`F1` CERRADA**: V6 está LISTA; falta el gesto de publicar (tag `v6.0` + push + release con el ZIP)

**Lo que pasó el 12-sep por la noche** (la sesión más larga de V6; el detalle está en la sección `F1` de `FICHAS`):
1. **Guardas del PC** en verde — y la suite del frontend estaba ROJA desde `#458` (fixture sin `import Core`).
2. **Siete imágenes** del mismo árbol; la regeneración de la stdlib embebida destapó que iba **sin `I2c.mod`**.
3. **Nueve placas** regrabadas desde `dist` y pasadas por `tanda.py` (Eduardo rotó: Pico, Metro, DK2, Nucleo, C3,
   S3, P4 kit, P4 Waveshare, C6). Todo verde. Aprendido: Windows dio el mismo COM3 a la C3 y la C6; abrir el
   puerto serie reinicia las ESP (DTR/RTS al EN); en el P4 un pack grabado gana a `/app` y `/lib` y el runner
   **para** (correcto: `#493`).
4. **La BD estaba ROTA en placa desde el 23-ago** por dos causas superpuestas — `#503` (guarda muerta: el P4 no
   cargaba ningún `.mdn` desde `#465`) y `#504` (el puente `.mdn` del pack de SQLite era de ABI 4) — y nadie lo
   vio porque nadie ejecutó una consulta. `SqlDemo` == host en los dos P4 (y en la tarjeta), `Bench` nativo 32 ms.
5. **`#502` medida y cerrada** en el cargador de `.mdn` (un blob del P4 REINICIABA la C6; ahora se ignora con
   mensaje); **`#379` cerrada** (8/8 con la tarjeta); **`#505`** (el simulador sin pantalla no drenaba el evento
   del handler — lo cazó Eduardo probando el ZIP).
6. **El ZIP**: montado, verificado por cuatro lentes independientes (workflow), desplegado en `C:	mpp6` y
   probado; **el IDE lo probó Eduardo: OK**. Lo que cazaron las lentes y el empaquetador está corregido (cinco
   drivers MOD6, `Stdlib.pack` MOD6 del 20-ago, cerrar la ventana del simulador, `board.json` de la Metro,
   `packsDir` en `BpVM.cfg`). Decisión tomada por mí, dicha a Eduardo: **el simulador del ZIP lleva SQLite**.

⏭️ **Al retomar — PUBLICAR, con el OK de Eduardo** (es lo único que falta y es de fuera): (1) `git status`
limpio; (2) tag `v6.0` sobre el último commit; (3) `git push origin main --tags` (~480 commits por delante);
(4) `gh release create v6.0 dist/BasicPlus-6.0-win.zip --title "BasicPlus v6.0 — orden en los micros"
--notes-file dist/release-v6.0.md` (el cuerpo ya está escrito, ES + EN, con el sha256 del ZIP `2279292d…`);
(5) comprobar el build de Pages (`gh api repos/legio-e/BasicPlus/pages/builds/latest --jq .status`) y abrir la
web; (6) descargar el ZIP de la release y verificar el sha256; (7) `docs/HECHO_V6.md` (snapshot inmutable) y la
memoria de cierre. ⚠️ Antes de montar el ZIP otra vez por lo que sea: el IDE cerrado, y el host está en sabor
`gui1-lvgl1` (para `compat.sh check` hay que volver a `make GUI=1 LVGL=0`).

📌 **Estado del repo**: todo commiteado, sin push. Placas al escribir esto: Waveshare P4 (COM15), DK2 (COM12),
C6 (COM3). El P4 kit y la Waveshare llevan el `SQLite.pack` nuevo sellado y `SQLite=4` en el ENV.

---

## ⏭️ AL RETOMAR (12-sep, noche) — ✅ **`D1` CERRADA** (`72f9706d`); **`F1` en curso**: las guardas del PC pasadas, faltan las siete imágenes y las nueve placas

**Lo que pasó al retomar** (la sesión de la tarde se cortó con el verificador de `D1` recién terminado):
1. **Los hallazgos del verificador, corregidos** (`72f9706d`). Los tres graves: (a) *«el C3/C6 rechaza el blob
   del P4 por ABI»* era **falso** en cuatro documentos — el guardián de la float-ABI existe en los packs, no en la
   sección `native` del `.mod`, y el IDE compila con los flags del P4 para cualquier RISC-V → **ficha `#502`**
   (nadie lo ha medido; se mide en `F1` con el C6); (b) *«la stdlib no se sube nunca»* → la regla real de `#466`;
   (c) `doc_frags.py` en rojo con 126 «fallos» que eran «falta contexto» → la clase **TROZO** en la herramienta
   (0 FALLO; y con tres roturas inyectadas, tres FALLO). Más las medias y bajas (PrintBench 1,9-5,3×, UNICORE,
   `tempC` en STM32, las dos portadas a V6, nueve placas / verificadas ocho, `CatchSinTipo.bp` de vuelta a
   `samples/`…). El detalle, en la sección `D1` de `FICHAS`.
2. **Eduardo leyó `RELEASES.md` v6.0 y la dio por buena** (*«OK»*) → `D1` cerrada en `FICHAS`, `F1` abierta con su
   plan en cinco pasos (guardas del PC → siete imágenes → nueve placas por `tanda.py` → lo manual: BD en P4,
   `#379`, `#502` → publicar).

⏭️ **Al retomar**: seguir el plan de `F1` en `FICHAS` por donde diga la entrada de abajo de ésta (las guardas del
PC se corren primero y se anota su resultado aquí). Placas al escribir esto: sólo la **Pico (COM22)**; Eduardo va
conectando por el orden del plan (Pico → Metro → C3 → C6 → S3 → P4 kit → P4 Waveshare → Nucleo → DK2).

---

## ⏭️ AL RETOMAR (12-sep, tarde — sesión cortada por el límite de tokens) — 🧊 **V6 CONGELADA** (`ae60e2e9`); `D1` a medias, **commiteada como WIP** (`e9fc30f0`)

**Lo que hay que saber en un minuto:**
1. **El freeze está marcado** (`ae60e2e9`, raya de código en `843e2d0a`). Después han entrado **seis bugs con
   ficha y reproducción**, todos del inventario de `D1`: `#496` (SQLite/Orm sin `import Core`), `#497`
   (`README.es.md` destruido desde el 22-ago, publicado así en v5.0), `#498` (seis samples rotos + el
   diagnóstico que engañaba), `#499` (rutas de 128-255 → «IO error»), y **`#501`** (miVM: un `sleep` como
   última acción de `main` mataba 1 de cada 5 ejecuciones — un yield tardío pisaba el `HALT`; `5d02dfd1`).
   `#500` (V7, Eduardo): la norma de versiones de las dependencias. 59 PASS.
2. **`D1` está escrita pero NO verificada ni leída**: el commit `e9fc30f0` lleva toda la documentación (ES y EN,
   manual/referencia/gui/PENDIENTES/QUICKSTART/INSTALAR/guía IDE/RELEASES v6.0/portal) escrita por nueve
   editores en paralelo, cada uno compilando sus fragmentos, más `tools/doc_frags.py` y `tools/samples_sweep.py`
   y el checklist nuevo de `PUBLICAR.md`. **Un verificador independiente estaba en marcha al cortar**: su informe,
   si terminó, está en
   `C:/Users/Eduardo/AppData/Local/Temp/claude/C--lenguajes-pm-miVM/92dd594f-bfc5-43df-be19-03d8a397bfe5/tasks/waifpc93s.output`
   (clave `result.verificacion`). **Terminó justo al cortar — leído por encima, esto es lo que dice:**
   ✅ las falsedades del inventario han desaparecido de todos los documentos (0 apariciones vivas); ✅ la
   introducción de Eduardo en `RELEASES.md` es byte-idéntica; ✅ `samples_sweep.py` 347/347; ✅ la mayoría de
   las cifras de v6.0 cuadran con FICHAS. ❌ **Tres graves a corregir antes de cerrar `D1`**: (a) manual §8.3,
   referencia y RELEASES afirman que «el C3/C6 rechaza el blob nativo del P4 por ABI de coma flotante y sigue
   interpretando» — **el código no lo respalda** (`loader.c:437 → mdn_load` valida magic/version/abi/arch;
   comprobar qué pasa de verdad y decir eso); (b) `guia-ide.html:311` «la biblioteca estándar no se sube nunca»
   es **falso** desde `#466` (el IDE pregunta por cada dependencia y sube a `/lib` la que falte o sea vieja);
   (c) `doc_frags.py` como gate de `PUBLICAR.md` da **126 fallos**, todos «falta contexto» (los editores
   compilaron con arneses privados y los fragmentos que dependen del anterior no llevan `data-bp="sigue"`):
   o se encadenan los fragmentos en el HTML o el gate no puede exigir código 0. Medias: PrintBench «3,5-5,3×»
   cuando la Pico da 1,9× (rango real 1,9-5,3); «en el S3 y el P4 el otro núcleo atiende el cable» es
   **invención** (`CONFIG_FREERTOS_UNICORE=y`); `Machine.tempC` sí existe en STM32 (ADC1, `BOARD_HAS_ADC_TEMP`);
   el resto de los README sigue anunciando V5 fuera del bloque Estado; el parque baila entre 8 y 9 placas
   (FICHAS dice **nueve**). Bajas: desajustes ES/EN previos a V6, un SVG con `.bpi`, un pie «cierre de V2»,
   `samples/pendientes/CatchSinTipo.bp` ya compila y debe volver a `samples/`.
3. **Eduardo no ha leído aún las notas de versión** (`docs/RELEASES.md`, sección v6.0): su introducción va tal
   cual como primer bloque; el resto lo escribió un editor desde el inventario. **Leerlas con él es lo primero.**
   Y la sección **18.4 de `gui.html`** («Dos formas de poner en marcha la interfaz: `run()` y `start()`») la pidió
   él con dos detalles concretos: `run()` no vuelve nunca en placa, y en las dos formas los handlers corren en el
   hilo del GUI — lo que cambia es el principal (parado en `join()` con `run()`, corriendo a la vez con `start()`,
   y entonces datos compartidos entre dos hilos).

⏭️ **Al retomar, en orden**: (1) leer el informe del verificador (o repetirlo) y arreglar lo que señale; (2) leer
`RELEASES.md` v6.0 con Eduardo; (3) cerrar `D1` en `FICHAS` con un commit que no diga WIP; (4) **`F1`**: el barrido
de `samples/` (`samples_sweep.py`), `doc_frags.py`, `compat.sh check`, una tanda de `tanda.py` por familia
(regrabar la Metro antes: sigue con el firmware del 9-sep), la dist con siete imágenes; (5) publicar v6.0 y el
push (unos 470 commits por delante de `origin`).

📌 **Estado del repo**: todo commiteado, **sin push**. Host en sabor `gui1-lvgl0`. Placas al cortar: DK2 (COM12),
Pico (COM22), Metro (COM4). miVM reconstruida con `#501`; frontend con el diagnóstico de `#498`; los cinco
firmwares compilan con `#499` (grabadas hoy C6, P4 y DK2 con el `INFO` de `screenW/H`, antes de `#499`).

---

## ⏭️ AL RETOMAR (12-sep) — `G2`, `C1` y `T1` cerrados el 11-sep; **lo primero: marcar el 🧊 CODE FREEZE V6** y empezar `D1`

**Decisión de Eduardo al cerrar el día (11-sep):** *«cuando terminemos `T1` congelamos código. Y por hoy
terminaremos, la documentación la empezamos mañana.»* `T1` quedó cerrada esa misma noche (runner arreglado y
tanda de confirmación hecha), así que **el primer commit del 12-sep es el freeze**: a partir de ahí sólo entra lo
que esté roto (el criterio de siempre: no «¿merece la pena?» sino «¿está roto?»). Luego `D1` (documentación) y
`F1` (pruebas finales).

**Lo que pasó después del traspaso anterior, en orden:**
1. **`C1` verificado en las cuatro pantallas** (C6 tras un «sin memoria» que era la DRAM de plataforma; P4 con
   `idf.py -p COM14 flash`, y girado 90°). La `Gui` rancia del pack del P4 sombreaba a `/lib` disfrazada de
   `/app` — `#493`, anotado; Eduardo borró los packs de la placa.
2. **`T1`**: `tools/tanda.py` (bucle por placa, Eduardo rota placas) probado primero contra `bpvm-sim`; la
   **prueba de fuego** con la lista corta en Pico, C6, P4 y DK2, todo verde (`compat/informes/tanda_prueba_fuego.md`).
   Para las gráficas el oráculo va **por placa** con `--screen=WxH`, y el `INFO` de los builds con LVGL publica
   `screenW/screenH` (regrabadas C6, P4 y DK2 con eso).
3. **La revisión adversaria del runner encontró 3 formas de decir verde por motivos equivocados** (artefacto
   rancio, dependencias comprobadas en `/lib` cuando el RUN resuelve `/app`, `EXITED` de otra sesión) y 5 menores;
   arregladas y verificadas con un proxy TCP que inyecta fallos del wire. La tanda de confirmación cazó la
   **Metro con firmware del 9-sep** (`exit=11` vs `1`) por conducta.
4. Fichas: `#495` abierta a V7 (el `BUSY` del wire durante un RUN es de la época de un hilo); `#444` respondida
   por `T1`; `#493` con el matiz del pack del P4.

⚠️ **Para el runner, lo que hay que saber**: el orden es por placa; el informe se añade por tanda; `--puertos auto`
descubre lo conectado (P4 y DK2 tardan > 3 s tras un reset: si sale «ninguna placa contesta», repetir); desde Git
Bash, `MSYS_NO_PATHCONV=1` si se pasan rutas `/app/...`; el host tiene que estar en el sabor `gui1-lvgl0`.

📌 **Estado del repo**: todo commiteado, **sin push**. Paridad 59 PASS. Grabadas hoy: C6, P4, DK2 (con `screenW/H`
en `INFO`) y la Pico (10-sep). **La Metro sigue con el firmware del 9-sep** (regrabar con el `.uf2` de
`pico/build/` + BOOTSEL por el wire; luego reinstalar `/lib`). Placas conectadas al cerrar: DK2 (COM12), Pico (COM22),
Metro (COM4).

⏭️ **Lo primero al retomar**: (1) el commit del **CODE FREEZE V6** (`FICHAS` + memoria, como en V4/V5); (2) `D1`
según su plan en `FICHAS`; (3) si sobra un hueco, regrabar la Metro y pasarle la tanda.

---

## ⏭️ AL RETOMAR (11-sep, noche) — **`C1` CONSTRUIDO** en el mismo día que `G2`; el siguiente es `T1`

**`C1` — la captura de pantalla — hecha de punta a punta y vista en las CUATRO pantallas** (`0791bb3e`, `d9c6e0db`): `Gui.shot(path)` en las dos VMs,
el fichero `.shot` (`docs/SHOT_FORMAT.md`), `tools/shot2png.py`, y **la pantalla de la Discovery vista en
el PC** (800×480, 6 956 B, `GET` en 643 ms). Detalle y lo que corrigió del diseño, en `FICHAS` (`#475`).
`#495` abierta a V7 por decisión de Eduardo (el `BUSY` del wire durante un RUN es de la época de un hilo).

**Cómo se hizo, y por qué importa para lo siguiente:** con `ultracode`. Un mapa previo de seis lectores
tumbó dos premisas del diseño (*el host no va en PARTIAL*, *el LZ4 pide 16 KB de pila*) antes de escribir
nada; dos satélites en paralelo (la herramienta Python y el lado Java) contra una spec escrita primero; y
una revisión adversaria (3 lentes, refutadores por pares) que confirmó dos hallazgos reales y descartó
siete que ya había arreglado en vuelo. **La spec primero** es lo que hizo que tres piezas escritas por tres
manos encajaran a la primera.

⚠️ **Lo que enseñó la placa**: (1) *bombear antes de capturar* — las transiciones del tema de LVGL
(~100 ms) hacen que una captura sin lazo salga con el checkbox vacío y `val=1` en el modelo; (2) **en la DK2
`/lib` se vacía en cada arranque** — lo subido a mano a `/lib` muere al primer reset, los módulos GUI van a
`/app`; (3) tres capturas del mismo programa con dos reflasheos en medio salieron **byte-idénticas**: el
oráculo placa↔placa existe.

📌 **Estado del repo**: todo commiteado, **sin push**. Paridad **59 PASS**. Discovery grabada con el
firmware final (17:30) y con `Gui/Json/Collections/Str` en `/app`. C6 y P4 **compilan, sin probar en placa**.
**C6 grabada y verificada** (2 862 B, 40×; costó un «sin memoria» primero: la DRAM de plataforma, no el tope — arreglado comprimiendo desde la banda sin copiar). **P4 grabada y verificada** (8 885 B, 138×; y girada 90°). Pico compilada. Metro sigue sin regrabar. El host queda en el sabor `gui1-lvgl0` (el del arnés); para ver la
GUI en el PC, `make GUI=1 LVGL=1` (y el arnés se negará hasta volver).

⏭️ **Lo primero al retomar**: **`T1` fase 1** (los
~50 samples puros con las placas conducidas), y en la fase 2 la prueba gráfica es *ejecutar, `toJson()`,
`shot`, `GET`, diff* — todas las piezas existen ya.

---

## ⏭️ AL RETOMAR (11-sep, tarde) — **`G2` CERRADO** en tres commits; el siguiente es `C1`, y antes se PAUSA

**`G2` — Revisión del modelo gráfico — hecho entero el 11-sep** (`50fcbc46` · `65a50f0e` · `65e9f558`),
y de paso una ficha que nadie esperaba, `#494`. Detalle de cada punto en `FICHAS` (sección `G2`).
Eduardo pidió *«el punto 4 y después una pausa»*: **esto es la pausa**.

**Lo que pasó, en orden:**

1. **El modelo BP de la GUI no era un modelo** (salió analizando `C1`): el árbol vivía sólo en C, los
   wrappers de los hijos se tiraban al cargar un `.win`, `destroy()` prometía una cascada que nadie
   hacía, y `OwnerList` **estaba hueca**. Eduardo lo convirtió en hito: *«si un objeto contiene a
   otro, debería poder verse»*. Ahora `Container` tiene `children: OwnerList`, `attach()`/`delete()`
   son el alta y la baja simétricas, la cascada es nuestra (hijos antes que padre, C antes que BP), y
   `Button` es un contenedor con un solo constructor y `text := ""` por defecto.
2. **`toJson()` recursivo en formato `.win`**, en BP y UNA vez (paridad gratis), con cinco
   intrínsecas de una línea para la geometría **autorada** (`align` por nombre + desplazamientos;
   lo auto se omite: el JSON hereda la paridad del `dumpTree`). **`GuiWinJson.bp`** al corpus:
   `main.win → J1 → J2`, `J1 == J2`, árbol idéntico; **igual en la Discovery** (LVGL, 800×480).
3. **La ida y vuelta destapó tres fallos** — para eso es la prueba:
   - el cargador **perdía la `y`** del `.win` (`align(…,0,0)` la pisaba: el `"y": 12` de `main.win`
     nunca había llegado al modelo);
   - una **regresión mía de G2-2**: `Keyboard.attach(ta)` resolvía al `Component.attach` nuevo y el
     teclado se quedaba sin textarea, en silencio → `setTextarea(ta)`;
   - 🐛🐛 **`#494`: el GC de la miVM descarrilaba desde el 15-jul** ante cualquier `""` o array
     vacío (12 B reservados, 8 contados). **El guardián lo gritaba por stderr y el arnés lo tiraba con
     `2>/dev/null`**: 57 PASS con el GC de la VM de referencia roto. Arreglado (el mismo clamp que la
     VM-C tenía desde F2) y el arnés ya vuelca esa línea.

⚠️ **LO QUE APRENDÍ HOY, en dos frases:** *(a)* una prueba de ida y vuelta vale más que las tres
piezas que une: encontró un fallo en el cargador, otro en mi propio commit anterior y otro de hace
dos meses en el GC; *(b)* **un guardián que grita a una tubería que nadie mira no es un guardián** —
el `2>/dev/null` del arnés era un `INCONSISTENTE` silenciado durante 57 casos verdes.

🔧 **Herramientas**: `compat.sh` acepta `// recurso: ruta` en la cabecera de un sample (copia el
fichero al `WORK`), y vuelca `HEAP INCONSISTENTE` del stderr como salida. `wire_serie.py`: desde Git
Bash hay que exportar `MSYS_NO_PATHCONV=1` o `/app/X.mod` se convierte en `C:/Program Files/Git/app/…`
(en la DK2 hay dos ficheros con esa ruta de sesiones anteriores).

📌 **Estado del repo**: todo commiteado, **sin push**. Paridad **58 PASS, 0 FAIL** (con el stderr
vigilado). **Discovery grabada** (13:16) y con `/lib` completo (`Gui/Json/Collections/Str` subidos hoy);
Pico compilada (13:21), **no grabada** (el cambio de C es sólo GUI); **la Metro sigue sin regrabar**
desde que cayó del USB (tiene el `Core` viejo: regrabar + reinstalar `/lib` cuando vuelva). Blobs
embebidos sin tocar (no llevan `Gui`).

⏭️ **Lo primero al retomar**: `C1` — la captura de píxeles (`#475`), porque el árbol ya está
(`toJson()` es la mitad «árbol» de `C1`). Antes de tocar nada, decidir lo pequeño que quedó abierto:
si el guardián del heap debe ser **fatal** (`#494`), si `#493` (la sombra de `/app`) entra en V6, y
la nota de `C1` en `FICHAS` sobre el tamaño del panel horneado en el JSON de `Window.load()`.

---

## ⏭️ AL RETOMAR (10-sep, madrugada) — **V6 se queda SIN fichas abiertas**: sólo los cuatro hitos

**`#473` y `#489` cerradas. No queda ni una ficha de V6 abierta** — lo siguiente son los hitos
`C1` → `T1` → `D1` → `F1`, y Eduardo pidió **analizarlos antes de empezarlos**.

**Lo que pasó, en orden:**

1. **`#489`** — la cola del GUI descartaba en silencio. No se agrandó: se le puso voz (el primero en
   el acto, el total al terminar, por `stderr` para no tocar la paridad). Con `make test-guidrop`, que
   **fuerza** el desbordamiento — 600 eventos en 512 huecos.
2. **`#473`** — se triaron los **91** hallazgos de la auditoría `A3`: 21 que rompen el modelo (2
   arreglados, 10 vivos, 9 ya cubiertos) y **70** de baja gravedad. De esos 70, **cuatro estaban rotos
   de verdad**; tres se arreglaron y el cuarto se midió y resultó no estarlo.
   - 🔑 **Dato de método**: de 12 hallazgos que la primera pasada dio por muertos, **8 resucitaron** al
     revisarlos. El patrón siempre igual: *se arregló la mitad que llega al usuario y se declaró
     muerto el hallazgo entero*.
3. **Los tres arreglos, todos medidos en placa**: el `-2` de línea estancada sube al contrato (un
   mensaje truncado se comía el siguiente); la línea del wire es **atómica en las cinco** (el cerrojo
   es del CABLE, no de la VM); y `Uart` tiene por fin **anillo de 512 B** — verificado en loopback en
   la Pico (GP0↔GP1) y en la Discovery (PC10↔PC11, en dos buses).
4. **Se abrieron dos fichas de V7**: `#490` (bajo consumo) y `#491` (el cursor / `File` como objeto),
   las dos con su estudio dentro.

⚠️ **TRES TROPIEZOS MÍOS, que son la parte útil del traspaso:**
- **Diagnostiqué contra el firmware VIEJO.** Compilé la Discovery tres veces y **no la grabé ni una**;
  leí un `available()=-1` como «el anillo no se activa» cuando era código de tres horas antes.
  **Compilar no es grabar, y el `serverBuild` del `HELLO` lo dice en un segundo.**
- **Publiqué una medida inválida y me salvó el control.** El primer `GET` cronometrado daba a los dos
  sistemas de ficheros por igual de lentos; el caso *bueno* salía peor que el sospechoso, que es la
  señal. El fallo era de mi herramienta (troceaba por líneas y se comía el bulk crudo).
- **Y estuve a punto de tomar la AUSENCIA de un aviso como prueba de que algo funcionaba.** No lo es:
  ese aviso sólo salta al fallar, así que falta igual si la función no llegó a ejecutarse.

🔧 **Dos herramientas nuevas en `samples/`, las dos nacidas de equivocarme**: `GpioPuente.bp`
(comprueba que el cable está ANTES de culpar al código) y `ScanCn1.bp` (lo **busca** en los 16 pines
del CN1 — contestó en 200 ms cuando yo lo buscaba en los pines de al lado). Y `tools/wire_serie.py`
tiene ahora verbo `get` con cronómetro.

📌 **Estado del repo**: todo commiteado, **sin push**. Imágenes al día y **grabadas** en Pico
(23:14) y Discovery (23:15); S3, C3, C6 y P4 compilados y sin grabar. Paridad **54 PASS, 0 FAIL**.

⏭️ **Lo primero al retomar**: `C1`. Eduardo dejó ampliado su diseño la madrugada del 11 —**serializar
la ventana a JSON, con el serializador en `Component` y recursivo por los hijos**— y el análisis está
en su ficha (`#475`). Lo que hay que censar primero: **qué propiedades expone hoy `Component`**, porque
de eso depende que el serializador se pueda escribir en BP (una vez) en vez de como builtin (dos veces).


## ⏭️ AL RETOMAR (9-sep, noche) — **de 19 cosas a 2**, y el corpus de 48 a 54

Segunda mitad del día, entera cerrando pendientes. **Nueve fichas cerradas, cuatro fuera de V6 con
su decisión escrita.** Y el hilo que recorre casi todas: *lo que no está en el corpus, se pudre*.

### Lo que se cerró

| | |
|---|---|
| `#486` | los dos dispatchers del GUI, resueltos **una vez por RUN**: 241 ms → **0-1 ms** |
| `#480` | entera: el ADC del ESP32 que fallaba con 0, + el watchdog (mañana) |
| `#485` | el contrato de prioridad de `io`, de **vigilado a construido** |
| `#484` | no era «falta `listDir`»: eran **seis** builtins sólo en miVM, **dos rotas en vivo** |
| `#481` | el código de salida y el informe de una excepción no atrapada |
| `#482` | la vista doble de los packs: **no la necesita nadie** — medido en placa |
| `#471` | tres roturas de paridad en `Machine`, y un `set()` que mentía |
| `#379` | → **`F1`**: lo único que le queda es una prueba de placa |
| `#487`, `#488`, `A4`, `#470`(números), `#480.2` | **fuera de V6**, con el diseño escrito |

### 🔑 Las correcciones de Eduardo, que son lo que hay que retener

1. **«El S3 es Xtensa y el C3 es RISC-V, son casos diferentes.»** Yo había clasificado cuatro micros
   **leyendo un macro que el programa ni consulta** (`SOC_MMU_DI_VADDR_SHARED`); la decisión se toma
   en *runtime*. Medido en las dos placas: **INST y DATA son la misma dirección** en C3 y S3. El
   mecanismo de vista doble **no tiene ni un usuario**. Y de paso corrige el 8-sep: aquel commit
   llevaba dos arreglos y **sólo uno trabajaba** — lo que dejaba al S3 sin packs era que el mapeo
   vivía en el directorio del P4.
   📐 Su explicación **predice** mejor que la medida: vistas separadas es un rasgo **Harvard**, y ARM
   y RISC-V son Von Neumann.
2. **«Hay que distinguir de un exit distinto de 0, de una excepción no atrapada: 2 casos.»** Eran dos
   roturas: miVM salía con **0** para un programa muerto, y la VM-C devolvía **el ordinal del enum**.
3. **«Cuidado con la frecuencia, no es una constante.»** Destapó que `set(200)` devolvía `true` en
   las dos VMs y `cpuFreqHz()` seguía diciendo lo de antes.
4. **«El watchdog, completo o no se implementa.»** El IWDG del U5 no se para por software → el STM32
   **se queda sin watchdog** y los tres verbos lanzan.
5. **«Con probarlo una vez en una placa es suficiente.»** El 91,8 % es común; compilar las cinco
   familias prueba lo mismo cinco veces.
6. **«Un `exit(11)` no me dice nada.»** → el `help error` del IDE.

### 🔴 CINCO roturas del invariante, todas vivas, todas por lo mismo

Ninguna se veía **porque no había sample suyo en el corpus**:

| | qué |
|---|---|
| los stubs del PC | **13 textos** distintos en seis fachadas (`(sim …)` vs `(stub …)`) |
| `src/uart.c:42` | un **`putchar`** en medio de `bpvm_out` → el payload salía **desordenado** |
| el watchdog | `(host, no-op)` vs `(stub, no-op)` |
| `IO.pathAbsolute` / `IO.prompt` | funcionaban en miVM y la VM-C lanzaba «builtin no soportado» |
| `Machine` | `uniqueId`, `tempC` y `cpuFreqHz` |

**Corpus 48 → 54**: `WdtCatch`, `StubParidad`, `IoPrompt`, `IoPathAbs`, `ThrowSinAtrapar`,
`MachineHost`.

### 🔧 Herramienta nueva: `bpgenvm-c/tools/wire_serie.py`

Cliente del wire **por serie que NO es el IDE**. Es lo que faltaba desde agosto para el paso 1 de
`#379` (*¿está colgado el device o el IDE?*). Con él: **16 ciclos `run→stop→INFO` en C3 y S3, 0
fallos**. Y sirve para cualquier placa: `info`, `log`, `list`, `ciclo`.

```
python tools/wire_serie.py COM3 ciclo /app/Bench.mod 8
```
⚠️ Desde Git Bash hay que poner `MSYS_NO_PATHCONV=1`, o convierte `/app/...` en ruta de Windows y la
placa contesta `NOT_FOUND` — me costó media hora.

### ⚠️ Dos trampas y un aviso

- **El `io-smoke` tenía un caso PERMANENTEMENTE ROJO**, y era **su guarda**: buscaba `"gui lista"`
  para detectar que el sim no trae LVGL, y el sim headless también la imprime. Un caso siempre rojo
  enseña a ignorar el arnés.
- **Mi propio script tiraba la respuesta que no encajaba** y decía «sin respuesta» mientras la placa
  contestaba `NOT_FOUND`. El mismo vicio que llevaba el día arreglando en el producto.
- ⚠️ **El arnés NO mira el código de salida**, sólo `stdout`. Los `exit 1` de `#481` están medidos a
  mano.

### ⏭️ Al volver: quedan DOS

- **`#462`** — el **suelo de ~48 ms** de latencia upcall→handler, independiente del quantum. Es la
  única con incógnita de verdad: el sospechoso (`LV_DEF_REFR_PERIOD = 33 ms`) explica buena parte,
  no los 48. Necesita placa con pantalla.
- **`#473`** — los **78 hallazgos** de la auditoría de capas, sin verificar. No necesita placa ni
  decisión: es leer y comprobar. Es la más grande. *(Van ya tres confirmados sin buscarlos: la
  fachada del FS que no enlaza con el backend de host, el `gpioCount` que no significa «cuántos
  GPIO», y el `io` del P4.)*

⚠️ **Placas**: la **C3 se reflasheó** hoy (llevaba firmware anterior al 8-sep y ahora monta packs);
Pico 2, S3 y Metro al día. El P4, el C6 y los STM32 **no**. Y el `bpgenvm-c` del host está construido
**sin LVGL** (`make sim LVGL=1` para el caso del GUI del `io-smoke`).
## ⏭️ AL RETOMAR (9-sep, tarde) — **de 19 cosas a 11**, y la lista dejó de mentir

Sesión de cerrar pendientes, muy conducida por Eduardo: **seis correcciones suyas**, y cinco
hicieron el trabajo más pequeño o lo pusieron en su sitio.

### Lo que se cerró

| | |
|---|---|
| `#456` | el `path` truncado en silencio — **y no se eligió número: se quitó la copia** |
| `#483` | la Metro que no ejecutaba nada — **por NO REPRODUCIBLE**, con la receta guardada |
| `#480.1` | el **watchdog** del STM32: completo o no está |
| `#470` | los nombres, **verificados en placa**: Pico 2 `rp2350a`, Metro `rp2350b` |
| `A4`, `#488` | **fuera del plan de versiones** |
| `#470` (números), `#480.2` | **a V7**, con el diseño escrito |

### 🔑 Las correcciones de Eduardo, que es lo aprovechable

1. **«¿Por qué el path tiene un tope?»** — cuatro palabras que cambiaron la ficha entera. No lo
   imponía nada: ni littlefs ni FatFs (255 **por nombre**), ni el protocolo (línea de 2048), ni el
   parser (in-place). Lo imponía **la copia**. Y no era un tope: eran **seis** números para la misma
   cosa (39, 63, 95, 191, 256, 511). Ahora el path del wire **no se copia** (`json_str_inplace`) y
   queda **un** número, para lo que hay que guardar o construir.
2. **«Quita topes. Si tiene que haber un tope lo hablamos y vemos cuál es la razón.»** Y al mirar
   salieron dos cosas que la ficha no tenía: **el lado BP tenía el mismo bug** (13 sitios que
   ignoraban lo que devuelve `read_bp_string`) y **el resolvedor iba por detrás** — el wire aceptaba
   205 caracteres y el `RUN` los recortaba a **39**.
3. **«El bucle de LVGL, si no hay nada pendiente, hace una pausa y el sistema de Threads pasa el
   testigo al siguiente.»** Yo iba a por «quantum por tiempo». Él tenía razón y era más simple: el
   bombeo **sí** pausaba, pero **a nivel de SO** (`SDL_Delay`, `vTaskDelay`, `__WFI`,
   `Thread.sleep`), y sobre esa única tarea corren **todos** los hilos BP. No dormía el hilo del
   GUI: congelaba la VM. Ahora devuelve el ocio y duerme el lazo BP.
4. **«El watchdog, completo o no se implementa.»** El IWDG del U5 no se para por software —silicio—,
   así que el STM32 **se queda sin watchdog** y los tres verbos lanzan. Y **el código se borra, no
   se comenta**: código muerto son avisos para siempre.
5. **«El S3 es Xtensa y el C3 es RISC-V, son casos diferentes.»** Yo había propuesto probar los
   packs del S3 en el C3 como si fuera equivalente. Lo es **para el MMU** y no **para la ISA**. La
   ficha se partió en dos.
6. **«Muchas de estas entradas son múltiples cosas; las que no tienen número, asígnaselo.»** De ahí
   `#482`–`#488`.

### 🔴 Tres roturas del INVARIANTE SAGRADO, vivas y sin que nadie las viera

Y las tres por el mismo motivo: **lo que no está en el corpus, se pudre**.

- **Los stubs del PC.** Seis fachadas (`gpio`, `pwm`, `pulse`, `uart`, `spi`, `pico`) escriben su
  línea de simulación **dos veces**, y no decían lo mismo: `(sim …)` contra `(stub …)`. **13
  textos.** Mismo `.mod`, `stdout` distinto.
- **Y uno no era cosmético**: `src/uart.c:42` usaba **`putchar`** en medio de una secuencia de
  `bpvm_out`. `putchar` escribe al `FILE` de C y **se sale del sumidero de la VM**: el payload salía
  **desordenado** (al principio de stdout) y el preview quedaba vacío. Dos líneas más abajo hay un
  comentario que dice *«TEXTO = CONTRATO DE PARIDAD: idéntico al de miVM»* — se hizo para `read` y
  se saltó `write`.
- **El watchdog**: `(host, no-op)` contra `(stub, no-op)`.

**Corpus 48 → 50**: entran `WdtCatch.bp` (los tres verbos del camino de error) y `StubParidad.bp`
(cada verbo con stub de las seis fachadas).

### 📐 La cuenta, que es lo que descolocó a Eduardo al final

*«Empezamos con 9 pendientes y terminamos con 11, parece que vayamos para atrás.»* Contando **como
contamos ahora** —una entrada, una cosa— esta mañana había **19**, no 12: `#462` escondía cuatro,
`#480` otras cuatro, `packs del S3` dos. **De 19 a 11.** Lo que subió no fue el trabajo: fue la
resolución del microscopio.

### ⚠️ Y una trampa mía, la misma de siempre

Eduardo no pudo compilar `MachineId.bp` desde el IDE: *«intrínseco no implementado en el emisor
mivm: Machine.getMicro»*. **El IDE empaqueta su propia copia del compilador** y la suya era del
8-sep 18:19; las intrínsecas se registraron el 9-sep 06:11. El síntoma es de los malos: el
**semántico las acepta** (las lee de `Machine.mod`) y sólo revienta el emisor, así que parece un bug
del compilador y es un **artefacto rancio**. Reconstruido y verificado compilando **con el jar del
IDE**, no mirando el log de Maven.

📌 **Regla que hay que aplicar sola: tocar `Intrinsics.java` obliga a reconstruir el fat-jar del
IDE** (con el IDE cerrado).

### ⏭️ Al volver

**11 pendientes.** Los dos que no necesitan nada de Eduardo: **`#486`** (cachear las dos direcciones
del dispatch del GUI, como ya hace miVM) y **`#480`** (el ADC del ESP32 que falla con 0 en vez de
−1 — es el mismo arreglo aplicado hoy dos veces). **`#481`** sigue esperando su criterio: ¿el
informe del error no atrapado es **salida** o **diagnóstico**?

⚠️ **Placas**: Pico 2, S3 y **Metro** llevan firmware al día; el resto no. Y el `bpgenvm-c` del host
está construido **con `LVGL=1`** (hizo falta para medir el GUI) — si algo se compara con builds
anteriores, tenerlo en cuenta.
## ⏭️ AL RETOMAR (9-sep, noche) — **tres carreras muertas** y la identidad del micro sale por la HAL BP

Sesión corta en fichas (una) y larga en lo que hay debajo: **el SMP de la VM-C pasa de inservible
a limpio**, y `getMicro()`/`getBoard()` quedan por el camino correcto, no por el rápido.

### Lo que se cerró

| | |
|---|---|
| `#474` | `B1` **arreglado en miVM** — y de propina **dos carreras más en la VM-C** (mutex y GC) |
| `#470` | su mitad buena: **`getMicro()` y `getBoard()`**, por la HAL BP, verificados en dos placas |

Los cuatro números que quedan de `#470` (MHz, GPIO, ADC, PWM) siguen esperando tu criterio, así que
la ficha sigue abierta. El arnés pasa de **45 a 48 casos** (`NeoCatch`, `MachineAlias`, `MachineId`).

### 🔑 Las tres carreras, y por qué salieron juntas

La ficha `#474` pedía revivir una reproducción que no compilaba (le faltaba un `import Core`). Al
revivirla, `B1` seguía vivo con su firma exacta. **Pero el arreglo no lo encontré leyendo: lo
encontró un criterio tuyo.**

> *«Aunque esté escrito en C el modelo bueno es el de OOP. Si tratas al Thread como una clase, debe
> haber un sólo método para cambiar el estado, el setter correspondiente. Y es ahí donde tienes que
> proteger el acceso y el cambio de estado.»*

En miVM `ThreadContext.status` era un campo público que se escribía desde **21 sitios**. Encapsularlo
(`setStatus(nuevo, quien, motivo)`, campo `private`, más un `ownerThread` volátil) no fue cosmética:
**el bug apareció solo** en cuanto hubo un sitio donde mirar. `YIELD` devolvía a la cola un hilo que
ya no era suyo. El arreglo son tres líneas, y todas dentro del setter.

⚠️ **Detalle de Java que me costó un intento**: `private` **no** obliga al setter si la clase es
anidada — la envolvente ve sus privados igual. Hubo que **renombrar el campo** (`status` → `estado`)
para que el compilador señalara los 21 sitios.

Con eso verde en miVM preguntaste lo que había que preguntar: *«¿y en VM-C qué ocurre?»*. En VM-C
`--smp=2` **se colgaba 7 de cada 8 veces**. Dos causas independientes, las dos de manual:

| | `--smp=1` | `--smp=2` | `--smp=4` |
|---|---|---|---|
| al empezar | 0/12 | **7 de 8 cuelgues** | 7 de 8 |
| tras el mutex | 0/12 | 1/30 | 5/12 |
| tras el GC | **0/20** | **0/20 y 0/40** | **0/20** |

- **El mutex no era atómico**: apuntarse como esperando y marcarse `BLOCKED_MUTEX` ocurrían en
  secciones críticas distintas → **despertar perdido** clásico. Ahora las dos cosas pasan dentro de
  la misma, en `bpvm_mutex_try_acquire` / `bpvm_mutex_release`, que son los **dos únicos** sitios que
  tocan la propiedad del mutex (mismo criterio de arriba, aplicado a C).
- **El GC podía tener DOS colectores**: si un worker entraba en `gc_stw()` con un
  stop-the-world ya en marcha, se esperaban mutuamente. Ahora el segundo se apunta como parado,
  espera a que acabe el primero y vuelve.

📌 Y el reparto de SMP, que era lo que estaba tapado: **×1,69 con 2 workers y ×1,83 con 4** en miVM,
**×1,90** en la VM-C. Tú ya habías dicho lo importante: *«`smp=4` no sé si tiene interés práctico, el
que realmente interesa es `smp=2`»* — y es justo el que estaba roto.

### 🧭 La corrección de rumbo: la identidad sale de la HAL BP

Iba a hacer que la fachada BP leyera el nombre **del repl** —o sea, invertir las capas— y lo cortaste:

> *«Hay que hacerlo por el camino correcto. Todos han de llamar a una misma función en Hal BP, una
> para el micro y otra para la placa. Lo que es diferente de cada imagen es la implementación.»*

Hecho así: `bpvm_pico_micro_name()` en la fachada, una implementación por familia, y **el wire y el
programa BP leen los dos de la misma función**. Verificado en placa: Pico 2 → `rp2350a` / `generic`;
S3 → `esp32s3` / `generic`.

🔑 Y ahí salió lo que yo había tirado a la basura: yo cableé `"rp2350"` a secas. Tú: *«en la Pico el
micro es RP2350A (61 pins), en la Metro es el RP2350B (81 pins)»*. **El silicio ya lo dice**:
`SYSINFO.PACKAGE_SEL` (1 = QFN-60, 0 = QFN-80), de sólo lectura e independiente del FS. La función
«inteligente» que pedías son dos líneas.

✅ **Verificado también el `rp2350b` de la Metro** el mismo día, cuando la Metro volvió a ejecutar: el wire dice `rp2350b (RP2350B)` y `Machine.getMicro()` dice `rp2350b` — los dos leyendo la misma función de la HAL BP.

### 📐 El modelo de los micros, escrito donde no se pierde

Lo que dictaste sobre el destino —1 núcleo → `IO`+`VM1` turnándose pero con **cola de hilos de SO**
para el futuro; 2 núcleos → `IO`+`VM1`+`VM2` a piñón, cada VM consultando la cola de hilos BP, *«en
principio no interactúan entre ellas, los intercambios se hacen a través de la memoria
compartida»*— está en **`docs/V6_IDEAS.md`**. En términos de código: **VM = worker**.

### 🧮 Y la cuenta que te hizo gracia

Empezamos el día en «12 pendientes» y lo terminamos en «12». **No era una broma del día: era un
error mío de ayer.** El rótulo del censo decía *8 fichas* mientras la lista enumeraba **nueve**, o
sea que ayer eran **13**. Cerrar `#474` corrigió la aritmética además de la lista. Ahora el rótulo y
la lista dicen lo mismo, que era medio problema.

### ⏭️ Mañana

**12 pendientes, 4 hitos.** Sin bloqueo: **`#473`** (verificar los 78 hallazgos de la auditoría —
sólo leer) y **`#481`**, que es el que toca el invariante sagrado.

**Decisiones tuyas que siguen abiertas** (las mismas de ayer, ninguna se ha resuelto): `#481` (¿el
informe del error no atrapado es salida o diagnóstico?), `#480` (qué ve el programador cuando el chip
no puede apagar el perro), `listDir` (¿entra en V6?), `#456` (qué devuelve un path truncado y cuánto
tiene que caber), los cuatro campos de `#470`, y si el SMP se activa por defecto.

⚠️ **Placas**: **Pico 2 y S3 llevan el firmware de hoy**; las otras siete, no. Y siguen en ellas los
`.mod` de prueba que ya te listé ayer — borrarlos sigue necesitando tu visto bueno, placa a placa.

## ⏭️ AL RETOMAR (8-sep, noche) — **de 15 pendientes a 13**, y el S3 estrena 8 MB de PSRAM

Jornada larga y muy conducida por Eduardo: **tres correcciones suyas** cambiaron el trabajo, y en
los tres casos lo hicieron MÁS PEQUEÑO. Al parar: *«en 2 o 3 días los tenemos todos hechos»*.

### Lo que se cerró

| | |
|---|---|
| `#472` | la **TLS emulada** del STM32 — `__emutls_v` con 2 `malloc` y un `abort()` en un micro |
| `#469` | fachadas sin backend — **cerrada por Eduardo**: *«para V6 está completo»* |
| `#468` | la stdlib en flash → **V7** |
| `S3` | **la PSRAM, activada**: heap de la VM de 160 KB a **7 MB**, probado en placa |
| `S3`/`C6` | **zona de packs**: el mapeo sale del directorio del P4 y pasa al común |
| `#471` | `Pico` → **`Machine`**, con alias. Faltan `getMicro()`/`getBoard()` |
| — | el **neopixel** lanza excepción BP en las dos VMs |
| `#481` | **abierta**: las dos VMs discrepan en `stdout` al morir lanzando |

El arnés pasa de **38 a 47 casos**.

### 🔑 Las tres correcciones de Eduardo, que son lo aprovechable

1. **El eje era la FAMILIA, no la fachada.** Yo pregunté «¿extendemos el patrón de `#469` a 3 de 13
   fachadas o a las 13?» y él: *«eso pertenece a la capa HAL BP… no se trata de hacerlo 8 veces,
   sino 3 y una ya está hecha»*. Medido con ese eje: los huecos eran **cinco**, no ocho ni trece, y
   la RP2350 ya estaba completa. La ficha se cerró el mismo día.
2. **«El sistema debería ser idéntico, igual que el environment.»** Yo iba a escribir `pack_s3.c`
   porque el S3 y el C3 no comparten dirección virtual para datos e instrucciones. Al **contar los
   consumidores**: 12 de `packs_base` son lectura, sólo **uno** ejecuta, y nace en **una línea**.
   No hacía falta código de familia — hacía falta dejar de suponer que un puntero sirve para las
   dos cosas. Y de paso apareció que **el C6 no tenía packs por vivir el código en el directorio
   del vecino**, no por silicio.
3. **«No tiene sentido posponerlo todo a V7.»** El renombrado a `Machine` entra en V6.

### ⚠️ Dos trampas nuevas, las dos me costaron una hora

- **Las intrínsecas se enganchan por `"Módulo.función"`.** Busqué `"Pico"` en el compilador, no
  salió nada, y di por hecho que el nombre no estaba cableado. Estaba, como `"Pico.boardName"`.
  `Machine.boardName()` compilaba con **cero errores** y devolvía basura (miVM `OutOfMemoryError`,
  VM-C **segfault**). 📌 Y detrás hay un hueco del compilador: declarar `intrinsic` algo **sin
  registro** no da error — el **duplicado** sí lo detecta. Sin ficha aún.
- **El build de la stdlib cachea por FUENTE, no por versión del compilador.** Tras arreglar el
  registro el alias seguía roto y culpé al compilador; era un artefacto rancio. `rm -rf
  bpstdlib/out` y correcto. **Al tocar el frontend hay que forzar la reconstrucción.**

⚠️ **Y dos cifras que di mal y quedan corregidas**: los «+877 B por módulo» y el «+18,9 KB de
imagen» eran de un build incremental con blobs a medias. Con build limpio, los `.mod` no tocados
salen **byte-idénticos** a los de git y el coste real es **+2,5 KB en ESP, +5,1 KB en la Pico**.

### ⏭️ Mañana

**13 pendientes.** Sin bloqueo y listos: **`#473`** (verificar los 78 hallazgos de la auditoría —
sólo leer) y **`#474`** (revivir la reproducción de `B1`: le falta un `import Core`). Y la
continuación natural de hoy: **`getMicro()`/`getBoard()`**, que son builtins nuevos —tocan las dos
VMs y `make check-builtins`— y son los que quitan el `esp32s3-devkitc` y cierran `#470`.

**Decisiones de Eduardo que siguen abiertas**: `#481` (¿el informe del error no atrapado es salida
o diagnóstico?), `#480` (qué ve el programador cuando el chip no puede apagar el perro), `listDir`
(¿entra en V6?), `#456` (qué devuelve un path truncado y cuánto tiene que caber).

⚠️ **Placas**: el **S3 está al día** (PSRAM + packs + `Machine`). La Discovery, el C6 y las demás
siguen con el firmware de ayer. Y en las placas quedan `/app/P.mod`, `/app/adc.mod`,
`/app/PsramS3.mod` y `/app/MachineAlias.mod` de las pruebas — borrarlos necesita tu visto bueno.

## ⏭️ AL RETOMAR (7-sep, noche) — **`G1` cerrado, la GUI ENTRA en la red, y el registro ya no se contradice**

Sesión de dos mitades: por la mañana trabajo de código, por la tarde poner el registro en orden.
Eduardo al parar: *«Empezaremos mañana, por hoy es suficiente.»*

**Lo primero, y es una corrección mía que conviene leer antes que nada.** Yo venía diciendo —en el
traspaso de ayer, en dos commits y en la ficha `#477`— que *«el `check` de `compat.sh` lleva
desactivado desde V4»*. **Es falso.** Lo que Eduardo desactivó el 17-jul fue la compatibilidad
**BINARIA** con V2/V3; el `check` se ejecuta, y lo que ejecuta es la paridad dual-VM. Lo comprobé
en un comando: **38 PASS, 0 FAIL, 0 SKIP**. El arnés estaba **verde con el corpus corto**, que es
justo lo que la ficha `#440` ya decía bien. Y lo peor: **la prueba estaba en este mismo fichero**,
escrita por mí 900 líneas más abajo — *«`#459` volcaba el heap y `compat.sh` daba 38 PASS»*. Si
daba 38 PASS, se estaba ejecutando. Dos veces di por muerta una herramienta viva sin abrirla.

**Lo que se hizo con eso.** El corpus pasa de **38 a 45 casos** y por primera vez lleva GUI:
entran `BusBug` (el reactivo de `#440`, que llevaba meses delante del arnés), `AdcDemo`, `ArgDemo`,
`MathRango`, `mathtest` y dos samples nuevos de GUI.

🔑 **Y el bloqueo que impedía meter GUI no era del lenguaje: era una ventana sin cerrar.** La ficha
decía que *«miVM NO TERMINA los samples de GUI»* como si fuera un límite de la VM de referencia.
El `JFrame` de `GuiBackend` **no se destruye nunca** y el EDT de AWT no es demonio, así que con una
ventana realizada la JVM no sale aunque no quede un hilo BP vivo. Se vio de la forma limpia:
**misma salida byte a byte en las dos VMs, y la VM-C salía con 0 y miVM con 124**. Un
`gui.shutdown()` al terminar y las dos salen con 0.

📌 **Y una corrección de rumbo de Eduardo a mitad de faena, que mejoró el resultado**: yo estaba
metiendo los seis samples de GUI que ya existen **cortándolos por timeout**. Funcionaba (49 PASS)
pero tardaba 3m37s y era *flaky por diseño*. Él: *«lo que hay que hacer son nuevos test y utilizar
`Gui.Start(); pause(5000); Gui.Stop()`»*. Con esa forma: **45 PASS en 1m00s y determinista**.
Los samples nuevos son `GuiParidad.bp` y `GuiParidad2.bp`.

⚠️ **Tres endurecimientos del arnés, cada uno con su control** (porque un verde sin control no
vale): el arnés **sabe ponerse en rojo** (doble de la VM-C que ensucia una línea → FAIL por sample
y código 1); **un caso mudo en las dos VMs ya no es PASS**; y **`filt()` ya no se traga las líneas
en blanco de en medio** — lo destapó el dropdown del sample nuevo, que vuelca su texto con saltos
crudos, y antes una VM que emitiera un blanco de más donde la otra no emite nada salía **verde**.

---

### La segunda mitad: el registro contradiciéndose

Eduardo pidió la lista de pendientes *«una entrada por línea para que sea más sencillo contar»*, y
al censarla —las 69 fichas leídas **enteras**, no por la marca— salieron **seis contradicciones**,
todas con la misma forma: **el titular decía una cosa y el cuerpo otra**. `E1` y `G1` cerrados con
la tabla diciendo «abierto»; `A1` cerrada con la cabecera diciendo «abierta»; `A3` y `A4` sin fila
en la tabla; `#408` con el enunciado pendiente y la respuesta dos líneas más abajo; el bug del LSP
cerrado por retirada sin decirlo; y las bases de datos en 🔴 mientras `E1` las daba por hechas.
Eduardo: *«Sí, soluciónalo. No debemos tener información contradictoria.»* Corregidas (`49364cb0`).

📌 **Lo aprovechable del método**: el aviso que había en la cabecera de `ABIERTAS` decía que
separar lo cerrado era trabajo aparte porque *«un clasificador automático ya falló en dos»*.
Fallaba porque clasificaba **por la marca**. Leyéndolas enteras salió a la primera — y de paso, la
«cola heredada» que ese aviso cifraba en **59 entradas** son **12**. Un aviso sobre basura sin
limpiar también envejece.

---

### 🎯 Y el plan de cierre de V6, decidido por Eduardo (7-sep)

*«`C1` y `T1` se quedan en V6. La razón es que lo utilizaremos para las pruebas finales. La
documentación y las pruebas finales, un hito cada uno. `C1` y `T1` justo antes de documentar y
pruebas finales.»*

Con eso V6 queda en **15 pendientes y 4 hitos**, y el orden es:

> **los 15 pendientes → `C1` (captura) → `T1` (pruebas) → `D1` (documentación) → `F1` (pruebas finales)**

Lo que hace fuerte esta decisión es la razón: `C1` y `T1` no son un añadido que se cuela al cerrar,
son **la herramienta con la que se cierra**. (`D1` y `F1` son códigos que puse yo siguiendo la
nomenclatura de la tabla; el reparto y el orden son suyos.)

📌 **Y el parque quedó contado, que es la entrada de `F1` y de `T1`: 7 imágenes y 9 placas.**
Eduardo contó 8 (*«Pico y Metro; S3, C3, C6 y P4; Nucleo y Discovery»*); la novena salió de que
**la imagen del P4 sirve a DOS placas** (kit y Waveshare, con el panel elegido por el ENV) —
confirmado por él. Contar placas por imagen se deja una fuera. Y la Waveshare no es redundante:
es la única que prueba el camino `display=st7701`, o sea que la imagen única lo sea de verdad.
Tiene deuda: **le falta el reflasheo**.

⏭️ **Mañana**: *«lo que toca durante unos días es ir resolviendo pendientes»*. La lista, en
`FICHAS.md`, en el censo de la cabecera de `ABIERTAS`.

⚠️ **Y lo que queda a medias, para que no sorprenda**: las placas siguen con el firmware de ayer
(sin `bpvm_out`, sin `#412`, sin el gate del `.mdn`), y en ellas quedan `/app/P.mod` y
`/app/adc.mod` de las pruebas — borrarlos necesita tu visto bueno, placa por placa.

## ⏭️ AL RETOMAR (6-sep, noche) — **`E1` CERRADO, 7 de 7**. Queda `G1` y los pendientes

Jornada larga y con mucho cierre. Eduardo al parar: *«ha sido una buena jornada»*.

### Lo que se cerró hoy

| | |
|---|---|
| `#452` | el `RESET` con un RUN vivo — **verificado en las tres arquitecturas** |
| `#469` | la fachada de ADC ya no inventa lecturas — **la MITAD de ADC; la ficha sigue abierta** por las otras fachadas |
| `#476` | `make check-builtins`, con su **control negativo** |
| `#478` | **miVM no podía tocar un bus** — 8 sitios del 4→8B de V4 |
| `#412` | el **argumento de ejecución**, de punta a punta y en placa |
| `E1` | los **siete** puntos: el tiempo del RUN, el CRC de origen, las BD sin placa |

Y dos piezas de fondo que salieron por el camino: **`bpvm_out`** (la traza de las
fachadas por el camino del `print`, y en placa **ya viaja por el wire**) y el **gate de
arquitectura del host** (`MDN_ARCH_X64`), que hasta hoy aceptaba en silencio un blob de ARM.

### 📌 Lo que hay que retener de la jornada

**Tres de los bugs de verdad los encontró el trabajo, no la red de seguridad**, y siempre por lo
mismo: **el corpus** del `check` de `compat.sh` no toca ni buses, ni GPIO, ni ADC, ni GUI (`#477`).
⛔ **Corrección del 7-sep: donde esto decía «el `check` lleva desactivado desde V4», es FALSO.** Lo
desactivado fue la compatibilidad **binaria** con V2/V3; el `check` se ejecuta y da paridad dual-VM.
Lo corto era el corpus — que es justo lo que decía la otra mitad de la frase. ✅ Y hoy quedan **cinco samples byte-idénticos** esperando a
entrar: `BusBug`, `AdcDemo`, `ArgDemo`, `MathRango`, `MathTest`.

⚠️ **Y una advertencia sobre mí mismo, que se repitió cuatro veces**: mi propio instrumento me dio
el hallazgo equivocado —la latencia del arnés como si fuera sesgo del `elapsedMs` (30× de más), un
`cut -c1-100` que «partía» una línea de C sana, un `2>&1` que mezclaba stderr con la paridad, y un
`make sim` sin `LVGL=1` que acusó a `#462`—. Las cuatro las cazó un **control**. Sin control, cada
una habría sido una ficha falsa.

### Decisiones de Eduardo, con su criterio

- **«¿Es útil para el programador? Si la respuesta es sí, se hace.»** Con eso entró la BD en el
  simulador.
- **«El IDE no tiene que determinar el orden de búsqueda, le pregunta al micro.»** Enderezó el CRC de
  origen: yo iba a duplicar el orden en el IDE, que es la enfermedad de `#470` y lo que costó `#463`.
- **«x86 es en realidad x64, los binarios son diferentes.»** De ahí salieron dos valores de
  arquitectura y no uno.
- **El autorun no pasa argumento**: el parámetro es para PROBAR; una vez probado, **el valor por
  defecto ES la configuración de despliegue**.
- **`#479` (mapa de pines y `AnPin`) a V7**, con el modelo cerrado. Y **`#475`/`#444` pasan a ser
  hitos propios `C1` y `T1`** — son implementación nueva, no pendientes.

### ⏭️ Para la próxima

**`G1`**, y con el **diseño ya cerrado** al final del día (ver su ficha): `Gui.run()` sigue
**síncrono** y se añade `Gui.start()` asíncrono más `Gui.stop()` — decisión de Eduardo, y no se
puede hacer de otra forma porque varios samples imprimen DESPUÉS de `run()`. Dentro caen dos
arreglos vistos al mirarlo: el lazo **no duerme** (gira quemando quanta con la ventana abierta) y
`__guiRunOnce` **rescanea la tabla de símbolos entera en cada pasada**. Es lo único que queda de
los hitos de V6 además de `C1`/`T1`,
cuya versión **sigue sin decidir**. De los pendientes, mi orden por daño: **`#477`** (enchufar el
arnés: es lo que impide que lo de hoy vuelva, y hay cinco samples listos), luego **`#480`** —con el
`Wdt.disable()` del STM32 que **no desactiva** a la cabeza— y **`#470`**.

⚠️ **Placas**: la Pico 2 quedó con todo lo de hoy. **La Discovery y el C6 se desconectaron a mitad**
(`No ST-Link detected`), así que llevan firmware de esta mañana: **sin `bpvm_out`, sin `#412` y sin
el gate**. Y siguen teniendo ficheros de prueba (`/app/P.mod`, `/app/adc.mod`).
⚠️ Para `make sim SQLITE=1` hace falta generar una vez `build/sql/aot_SQLite.c` con `AotMain` y
tener la amalgama de SQLite, igual que `test-sqldemo`.

## ⏭️ AL RETOMAR (6-sep, mediodía) — `E1`, y si da tiempo `G1`

**Lo que dijo Eduardo al parar**: *«a la vuelta retomamos `E1` y si nos da tiempo `G1`. `#475` y
`#444` hay que darle un ítem propio ya que es implementación nueva. Del resto, hay que ir cerrando
pendientes.»* → `#475` y `#444` ya son los hitos **`C1`** y **`T1`** en la tabla (versión sin decidir).

### Lo que cayó hoy — cinco arreglos, y tres los encontró el propio trabajo

1. ✅ **`#452`** — el `RESET` con un RUN vivo. **El enunciado de la ficha era falso**: RESET *sí*
   llegaba y se rechazaba a propósito con `BUSY` (lista blanca KILL/HELLO en el poll). Verificado en
   **las tres arquitecturas** —Discovery, Pico 2, C6—, que es todo el código tocado. Y el segundo
   defecto, que no hacía ruido: **el IDE se tragaba el BUSY** e imprimía «reset enviado».
2. ✅ **El tiempo del RUN** (`E1`): el IDE ya enseña `exit 0 (OK) en 94 ms`. El campo llevaba años
   viajando y `git log -S elapsedMs -- BpIde/` salía **vacío**.
3. ✅ **`#476`** — `make check-builtins`, con su **control negativo**.
4. ✅ **`#469`** — la fachada de ADC ya no inventa una lectura. El arreglo es el patrón genérico:
   *quien no tiene hardware registra un backend explícito, y la ausencia pasa a ser error*.
   ⚠️ **Pero la ficha NO queda cerrada**: lo hecho es la mitad de ADC, y `FICHAS.md` la mantiene
   abierta porque *«queda decidir qué se hace con las OTRAS que la auditoría destapó»* (`#480`).
   *(Corregido el 7-sep: esta lista la daba por cerrada entera y `FICHAS.md` decía lo contrario.)*
5. ✅ **`#478`** — **miVM no podía tocar un bus**: los ocho builtins de I2c/Spi/Uart/Neopixel se
   quedaron fuera del 4→8B de V4, y el arreglo estaba escrito 400 líneas más arriba en `case MOVE`.

### 📌 El hilo que une los cinco, y es lo que hay que retener

**Los tres bugs de verdad de hoy los encontró el trabajo, no la red de seguridad** — y siempre por
lo mismo: **el corpus** del `check` de `compat.sh` **no toca ni buses, ni GPIO, ni ADC, ni GUI**
(`#477`). ⛔ *(«el `check` lleva desactivado desde V4» era falso; corregido el 7-sep — ver arriba.)* El sample que reproduce `#478` **ya existía**, escrito
hace tiempo para bisecar otro cuelgue.

✅ **Y hoy quedan cuatro samples listos para entrar en el corpus**, byte-idénticos en stdout:
`BusBug` (7 líneas), `AdcDemo` (16), `MathRango` (29), `MathTest` (17). Los dos primeros lo son
**desde hoy**: hacía falta alinear los textos de los stubs y arreglar el orden.

### 🔧 Y una pieza de fondo que salió de ahí: `bpvm_out`

Las nueve fachadas escribían con **`printf` directo a stdout**; sus **42 llamadas** pasan a
`bpvm_out()`, que entrega al mismo `emit_text` que el `print` de BasicPlus. Arregla el orden en el
host **y** hace que en placa esos mensajes **viajen por el wire** — verificado en la Discovery.
⚠️ Ojo al reparto, que costó un intento: **`bpvm_diag` ya existía** (`#355`) y es OTRO canal —
diagnóstico a stderr/log. `bpvm_out` = salida del programa, sujeta a la paridad. Escrito en
`include/bpvm_out.h`.

### ⏭️ Para la vuelta

**`E1`** (quedan `#412`, el CRC de origen y las BD sin placa — los tres esperan decisión, no código)
y **`G1`**. Y de los pendientes, mi orden por daño: **`#477`** primero (enchufar el arnés: es lo que
impide que lo de hoy vuelva, y hay cuatro samples esperando), luego **`#480`** —lo que destapó la
auditoría de las 13 fachadas, con el `Wdt.disable()` del STM32 que **no desactiva** a la cabeza— y
**`#470`** (el C3 y el C6 se creen un S3 por la vía BP).

⚠️ **Placas**: la Discovery quedó grabada con lo de hoy; la Pico 2 y el C6 con lo del `RESET`, pero
**no con `bpvm_out`**. Y sigue habiendo un `/app/P.mod` y un `/app/adc.mod` de prueba en ellas.

## ⏭️ AL RETOMAR (5-sep, noche) — mañana **el IDE**, y `L1` ya no está en V6

**Lo que dijo Eduardo al parar**: *« lo podemos dejar por hoy. Mañana podemos ver el IDE y a partir
de ahí ir solucionando pendientes.»* O sea: **`E1` primero**, y luego las fichas sueltas.

### 1. La decisión del día: `L1` entera se va a V7

Salió de tirar de un hilo pequeño —*¿por qué no se puede sobrecargar una intrínseca?*— y acabó en
un cambio de fondo. La cadena, por si hay que reconstruirla:

1. El bloqueo es real y hay **tres** sitios, no uno: el camino de las intrínsecas compone su clave con
   `fs.name` **pelado** (`MivmEmitter.java:1069`, `:3926`, `:3972`) mientras el propio emisor tiene
   escrito que *«DEFINICIÓN y LLAMADA deben usar SIEMPRE este mismo helper»* (`:1678`, `emitName`).
2. Al mirar qué se gana, apareció que **ya conviven dos mecanismos** para lo mismo: `Integer` y
   `Comparable` son fuente en `bpstdlib/Core.bp` (545 líneas), pero `Object`, `Thread`, `Mutex` y
   `StringBuilder` se construyen **a mano en Java** (19 `makeMethod`, `SemanticAnalyzer.java:423-529`),
   y `integer` el tipo es un `case` de un `switch` (`resolveType`, `:1431`). La frontera no responde
   a ningún criterio: es lo que se pudo expresar en cada momento.
3. Eduardo lo nombró mejor que yo, y de memoria de Turbo Pascal: **el módulo raíz**, la unidad
   `System` escrita en el propio lenguaje. *«Me da más tranquilidad tener algo que se pueda LEER que
   algo que sólo existe en memoria. Y esto también es importante: un solo archivo, sin posibilidad
   de diferencias entre compilador y VM.»*
4. Su objeción —«nos queda un `Core.mod` enorme»— **se midió y no se sostiene**: declarar no cuesta
   RAM. El cargador **salta** la sección `interface` (`loader.c:235`), y `Math.mod` —que es casi sólo
   declaraciones— es **54,6 % interface y 16 B de exports**. Lo que crece es la flash de las cinco
   imágenes, porque `Core.mod` va embebido.

✅ **Decidido: `L1` a V7**, con su criterio de siempre —*«lo importante es que lo que hay ahora
funcione correctamente»*— y **se va casi entera**, porque lo que queda dentro son añadidos que se
diseñan **encima** del módulo raíz. **V6 queda en `E1` + `G1` + pendientes + la captura.**
✅ **Lo que NO se aplaza**: `#476`, el andamio. Las **226** entradas de las dos tablas de builtins se
mantienen a mano y divergir **no hace ruido** — lo denuncia el propio `builtins.c:355`.

### 2. La captura de pantalla, medida — y tres cosas que yo había escrito mal

`#475` reescrita con números (11 agentes; el diseño largo en `docs/V6_IDEAS.md`). Las correcciones,
que son lo que hay que retener:

- 🔴 **La cola de control de `A1` NO sirve para un verbo `SHOT`**: tiene dos lectores, los dos en el
  `next_cmd` del depurador — sólo se drena con la VM parada en un breakpoint. → **una sola orden**:
  `Gui.shot(path)` + el `GET` de siempre. Cero verbos nuevos.
- 🔴 **El P4 sí tiene framebuffer vivo** (el del driver DPI del IDF). Sólo el **C6** necesita bandas.
- 🔴 **El FS del P4 no está en PSRAM** — `fs_ram.c` ya no existe; las cinco montan littlefs sobre
  flash. Desgasta y sobrevive al reset. *(Había una memoria del proyecto diciendo lo contrario, de
  junio; corregida. Es el riesgo típico: un dato cierto en V3 que sigue en pie en V6.)*
- ✅ **Comprimir ya está escrito**: el LZ4 de LVGL, `.c` ya dado de alta en los tres builds gráficos,
  apagado por **una línea** (`lv_conf.h:853`). 21,8× a 152,6× medido.
- ⚠️ **Y el orden se invierte**: comprimir es el **primer** paso, el fichero es lo opcional — el crudo
  no cabe en RAM ni en el C6 (115 200 B contra 68 264) ni en la Discovery (771 027 B contra 433 416).
- ⚠️ **Se empieza en el HOST, no en el C6.** Mi versión anterior rompía la cascada del proyecto.

### 3. `#477` — lo que más me escuece del día

El oráculo exacto de la GUI **existe desde V4 y nadie lo ejecuta**: `dumpTree` es byte-idéntico en las
dos VMs (medido hoy en 6 samples), **22 samples lo llaman**, y el corpus del `check` de `compat.sh`
no tiene un solo sample de GUI. ⛔ *(decía «el `check` lleva desactivado desde V4»: falso, corregido
el 7-sep.)* Antes de construir la segunda vía
(capturar, comprimir, bajar) hay que enchufar la primera, que es gratis.
⚠️ Con sus tres agujeros medidos: el chart es invisible en el dump; es **ciego a la rotación** (y en
la Discovery `disp_set_rotation` es un **no-op declarado**, así que el oráculo sale VERDE con el panel
sin girar); y color y fuente son render-only por contrato.
⚠️ Y una tesis mía **falsada al medirla**: no es cierto que sólo cambie la línea `screen` al cambiar
de resolución — `Gui.bp:824` lee el tamaño de pantalla y **todo el camino Forms** se mueve con ella.

### ⏭️ Para mañana — `E1`, con el reconocimiento YA HECHO

Dejé corriendo un reconocimiento de los seis puntos de `E1` (un agente por punto + verificación
adversarial). Lo que trajo, y **`FICHAS.md` ya está corregida** con ello:

🟢 **`E1` tiene CUATRO puntos vivos, no seis.** El árbol por color está **hecho desde el 26-ago**
(`9cc33ee6` + `fb66e579`, `PicoExplorer.java:1296`) y seguía listado como pendiente. *La lista mentía
al alza justo en el hito que se iba a abordar* — [[no-acumular-pendientes]] en su forma más barata.

**1. `#452` primero, porque es LO ROTO — y su enunciado era FALSO.** RESET **sí llega**: la placa lo
lee, lo parsea y lo **rechaza a propósito** con `ERROR BUSY`, porque el poll que atiende el cable
durante un RUN tiene **lista blanca** (KILL, HELLO). Reproducido en el simulador. La diferencia con
KILL cabe en una línea: KILL tiene rama en el poll y **delega**; RESET sólo existe en el despachador
de reposo, al que no se vuelve hasta que `bpvm_run()` retorna.
- 🔴 **Es además una divergencia de las dos VMs que no estaba escrita**: miVM **sí** contesta RESET
  con un RUN corriendo — y no porque nadie lo pensara, sino porque su lector es un hilo aparte
  (`DebugServer.java:175`).
- 🔴 **Y el IDE se traga el BUSY**: `BpvmClient.reset()` captura la `IOException` (y `WireError` la
  extiende) y la manda a un `diag()` que puede no ir a ninguna parte; la consola imprime «reset
  enviado». El usuario cree que reinició, **la placa sigue corriendo y encima se ha quedado sin
  conexión**. *Errores sí, silenciosos no*, en estado puro.
- Se parte en dos: **1a sin placa** (que el IDE deje de mentir + el poll del simulador) y **1b con
  placas** (copiar la forma de KILL en los **tres** ficheros de REPL — las cinco familias son tres).

**2. El tiempo del RUN (sesión corta), pero el número de la ficha CADUCÓ.** El `durationMs` del SAVE
vale 0 siempre (los `fs_save` son no-ops desde littlefs; comprobado en el `.map`, no en el comentario:
`.text.fs_save … 0x2`). El bueno es el **`elapsedMs` del `EXITED`**, que mandan los **cinco** emisores
y que el IDE no ha parseado **nunca**.
⚠️ Decisión antes de enseñarlo: **no miden lo mismo** — miVM arranca el cronómetro antes de cargar y
enlazar, la placa justo antes de `bpvm_run`. Sin decidirlo, el PC parecerá siempre más lento.

**3. El `capabilities` del `HELLO` (sesión corta, sin tocar imágenes):** cero ocurrencias en las 39
clases del IDE, y las tres cadenas **ya divergieron** — el ESP32 no declara `DEBUG` **aunque lo tiene
cableado entero**, y el STM32 tampoco, que ahí sí es correcto (`stm32_repl.c` no llama a
`bpvm_dbg_wire_*` ni una vez, confirmado en el `.map` del Nucleo). Síntoma que ve el usuario: «Debug
on Device» contra una Nucleo → `UNSUPPORTED`.

**Lo que NO tocar mañana**, y por qué:
- **`#412`** no está roto, **falta** — y arrastra un **builtin nuevo (id 232)** que obliga a
  reflashear las cinco familias **a la vez** que se publica el compilador. Va con una ronda de flasheo.
- **Probar BD sin placa**: `make test-sqldemo` ya corre el ciclo **entero** en el PC. Lo que queda es
  una decisión de diseño, no una sesión. *(Y hoy `SQLite.pack`, 1 130 496 B, ni monta contra la región
  simulada de 1 MB.)*
- **CRC de origen**: sesión larga, y el mensaje **no cabe** — `link_error` es `char[160]` y el texto
  actual ya gasta 78.

**Dos hallazgos de propina, anotados aquí para que no se pierdan:**
- 🟡 `scheduler_smp.c:114` y `:158` llaman al `poll_cb` **sin la guarda `vm->io == NULL`** que sí
  tienen `scheduler.c:100` y `builtins.c:970`/`:1015`. **Latente, no vivo** (SMP es opt-in y no está
  en el build por defecto), pero es exactamente la clase de fallo que cerró `A1.7`.
- 🟡 El guardián del fat-jar (`#429`) compara la fecha del **compilador** embebido — pero el IDE
  empaqueta **también la VM-Java**, y para eso no avisa nadie.

⚠️ Y la trampa de siempre antes de tocar el IDE: **el fat-jar no se reconstruye con el IDE abierto**.

## ⏭️ AL RETOMAR — `P2` CERRADA (4-sep): la pantalla del C6 entera. Empieza la segunda mitad de V6

**Dónde estamos.** U1–U6 (unificación), `#466`, `P1` (C3 y C6 sin pantalla) y `P2` (la pantalla
SPI del C6: driver, memoria medida, rotación confirmada) están cerrados. El marcador de Eduardo
—*«cuando tengamos la ESP32-C6 habremos llegado al ecuador de V6»*— quedó atrás. La segunda mitad,
según el cuadro de hitos de `FICHAS.md`: **A1** (arquitectura por niveles sobre código único, que
iba después de la unificación), **G1**, **L1** y **E1**. Y las fichas abiertas de siempre: `#462`
(cuanto por opcodes / suelo de 48 ms / carrera del exit-code en KILL), `#456`, `#452`, `#444`,
`U6.F1`. Decisiones que esperan a Eduardo: **`#468`** (la stdlib en pack XIP: zona vs tabla embebida, con
o sin la tabla de símbolos por referencia — analizada con números), `Neopixel` en la familia ESP32,
y si se prueba de cero la subida de `Gui`+`Json` desde el IDE (exige borrar los dos de `/lib` de la
C6). La del tamaño del screen ya está tomada y hecha (`P2.3`: el screen mide lo que mide el panel).

**Placas.** La C6 (COM3) lleva la imagen con GUI de 128 KB (4-sep 00:15) con `/lib/Gui.mod`,
`/lib/Json.mod`, `/app/GuiColorDemo.mod` y `/app/GuiRotCycle.mod`, `log=0`. Pico 2, S3, Nucleo y
Discovery llevan las imágenes del 3-sep. Metro, C3 y P4 siguen con imágenes anteriores a `#466`/U4/U5
(sus números ya están fijados; se actualizan cuando se graben).

### El orden que venía (para contexto)

**El orden lo fijó Eduardo al cerrar el 2-sep:** *«Terminaremos U6, U4 y U5. Después podemos hacer
P1 (sin pantalla) y P2 (añadir la pantalla a ESP32-C6).»*

### Lo que quedó hecho el 2-sep (segunda mitad, tras la pausa por tokens)
- ✅ **P4 repasado**: `/lib` limpio de por sí (15/15), `/app/Core.mod` fuera con su OK.
- ✅ **`U6.11`**: el STM32 al planificador — Nucleo y Discovery compilan headless, `test_mem`
  22/22. **Las cinco familias deciden su memoria con `bpvm_mem_plan()`.**
- ✅ **`#466` en tres pasos**, con la regla de Eduardo (*el SO repone `/lib` por versión y CRC; el
  IDE pregunta a la placa por cada dependencia y sólo sube lo que falta o es más viejo*): `STAT`
  por nombre + `magic`; `src/bpvm_mods.c` llamado por los tres instaladores (`test-mods` 16/16);
  el IDE con `statModule`, sin `EMBEDDED_CORE_MODS` (`StatModuleSmoke` 5/5). Y de regalo: **el
  CRC del wire salía negativo** (bit 31, `long` de 32 bits) y el IDE re-subía la mitad de los
  ficheros idénticos — arreglado en los dos lados. Seis builds verdes, paridad 38/0/0,
  `sim-smoke` 45/45, fat-jar del IDE reconstruido. Commits `1a7c1924`, `f1a9c23b`, `0e1a425b`.

### En este orden
1. ✅ **`#466` CERRADA (3-sep)**: en la Pico 2, el SO repuso solo un `Math.mod` MOD6 plantado a
   propósito (4a), y el Run de `MathRango` desde `BpIde-6.0.jar` dijo `Math.mod: ya en la placa,
   idéntico — no se sube` con las 29 líneas iguales al host (4b). De camino salió y se cerró
   **`#467`**: el compilador resolvía `import Math` contra `samples/out` (la stdlib entera de V5,
   MOD6) antes que contra `bpstdlib`; ahora la stdlib va PRIMERO en los tres buscadores del
   frontend, el `BpVM.cfg` del fichero manda sobre el del cwd, y el IDE es `BpIde-6.0.jar`
   (`bpide.bat` arranca desde la raíz del repo).
2. ✅ **S3 (3-sep)**: regrabado con la imagen de las 19:01 (`#466` + `U4.1`); su `/lib` era todo de
   V5 (13 MOD6 — ayer lo di por limpio porque el ESP32 no avisaba) y el primer arranque los repuso
   los 13 él solo; memoria idéntica (160 KB, techo 272, margen 26564); `STAT` por nombre OK.
3. ✅ **Nucleo y Discovery (3-sep, grabadas por Eduardo)**: las dos dicen `vm: 512 KB en SRAM
   estática (todo lo que deja la región)` e INFO 393216/131072; sus `/lib` estaban vacíos y la
   imagen instaló los 14; `STAT` por nombre OK. **`U6` CERRADO en las cinco familias.**
   ⏭️ `U6.12` (observación de Eduardo): la Discovery pasa de 512 a **1536 KB** de VM
   (`BOARD_VM_BYTES` por placa; medido: quedan 457 KB libres). ✅ Regrabada por Eduardo (sello
   19:26): INFO 1179648/393216, el panel del IDE `heap 1.1 MB + stack 384 KB`.
4. ✅ **`U4` HECHO (3-sep, `U4.1`)**: un generador (`scripts/regen_mods.sh`), tres ficheros de
   sólo datos, el bucle en `src/bpvm_mods.c`; blobs byte-idénticos, seis builds, Pico 2 verificada.
   Las demás placas lo reciben al regrabarse.
5. ✅ **`U5` HECHO (3-sep, `U5.1`)**: `bpvm_handles.[ch]` con la tabla (tipo, constantes, API)
   sacada de `heap.c` y de diez campos sueltos de `bpvm_t`; el ABI del `.mdn` intacto (los helpers
   no leen esos campos por offset); paridad 38/0/0, `test_smp_handles` limpio, seis builds, Pico 2
   byte-idéntica. **Con esto la serie de unificación U1–U6 está cerrada.**
6. ✅ **`P1.C6` HECHO (3-sep, tarde)**: `esp32c6/` clonado del C3 (siete ficheros), un arreglo en la
   cintura común (`SOC_UART_HP_NUM`, el C6 cuenta su UART LP), aprovisionado por el wire, las tres
   constantes MEDIDAS (bloque contiguo 376 KB, margen 16932 → objetivo 192 KB), `fib(28)` 11823 ms
   (C3 11315), `MathRango` 29 líneas byte-idénticas al host. **El ecuador de V6.**
7. ✅ **`P2.1` HECHA (3-sep noche → 4-sep madrugada)** — la pantalla del C6 **funciona a la
   primera** (Eduardo vio los tres botones con sus colores); la imagen con LVGL cabe en la
   `factory` de 1,5 MB. Medido con la GUI en marcha (tres pasadas), LVGL cuesta ~108 KB estáticos
   y hasta 103 824 B en marcha (va sobre el heap de C), así que la imagen con pantalla lleva
   **objetivo 128 KB, margen 103 824**; la de 128 grabada y confirmada (`vm: 128 KB … techo 194
   por el margen del sistema`, INFO 65536/65536, mínimo histórico 68 264 B con el demo; `log=0`).
   `test_mem` 30/30. El C6 (COM3) tiene `/lib/Gui.mod`, `/lib/Json.mod` y `/app/GuiColorDemo.mod`.
   Siguiente: **`P2.2`** (PWM del backlight, rotación con gaps, LED RGB como `Neopixel`, el IDE
   subiendo `Gui`+`Json` solos; detalle en la ficha).
8. ✅ **`P2.2` HECHA (4-sep)** — la rotación va por MADCTL del panel con el gap de cada orientación
   (tabla ST7789 240×240: 180 → y+80, 270 → x+80) y repintado entero; el aviso a consola, no a
   stdout. Sample nuevo `GuiRotCycle.bp` (gira solo cada 3 s, marcas en las esquinas; paridad host
   25/25). **Eduardo lo vio: «la demo funciona perfectamente, y sí las esquinas coinciden»** — las
   cuatro orientaciones, sin franja. La lista de P2.2 se acortó con hechos: el IDE YA resuelve deps
   transitivas (Gui→Json), el PWM del backlight no tiene API de brillo a la que servir (se queda el
   GPIO), y `Neopixel` sólo tiene backend en la Pico (en ESP32 es no-op: adaptador nuevo, ficha
   aparte). Punto de diseño anotado: el screen lógico dice 480×320 en todas las placas (paridad del
   dump) mientras LVGL alinea contra el panel real. **Con esto `P2` queda cerrada.**
9. ✅ **`P2.3` HECHA (4-sep)** — decisión de Eduardo: *«la resolución ha de ser la que tenga la
   pantalla, no hay otra»*. Contrato nuevo `bpvm_gui_disp_native_size` (C6 240×240, DK2 800×480,
   P4 según el ENV, host 0) y el modelo lo pregunta al crear el screen; `--screen=` y el simulador
   siguen ganando. La C6 dice `screen [240x240]`; el host con `--screen=240x240` da el mismo dump
   byte a byte; sin él, paridad con miVM intacta; sim-smoke OK; compilan host, C6, P4 y Discovery.
10. 🟡 **`#468` ABIERTA (4-sep): la stdlib en un pack XIP** — dirección de Eduardo (*«subir el pack
   de la librería estándar+Json+Gui, ahorrar 40 o 50 K»*), **analizada con números en la C6**: los
   tres módulos ocupan 29 224 B del bloque de la VM y un pack XIP devuelve 22 268 B al heap (de 31
   a ~53 KB libres); la otra mitad es la tabla de símbolos (49 152 B en el malloc de plataforma),
   que sólo baja con nombres por referencia desde flash. El nudo: el FS eclipsa a la zona y `#466`
   repone `/lib` en cada arranque → instalador + `STAT` tienen que ver la zona. Falta su decisión
   (pack en la zona vs tabla embebida XIP; con o sin la tabla de símbolos). Detalle en la ficha.
11. 🟡 **`A1` ABIERTA (4-sep, tarde): la arquitectura de ejecución común** — Eduardo preguntó
   cuántas tareas de FreeRTOS corren (censo: UNA nuestra en ESP32 y Pico con todo dentro, cero en
   STM32, `vm`/`main`/`wire_uart` según la familia) y se midió en la C6: lo de entre cuantos
   cuesta 1 %, la salida 0,84 ms por `print` (seis mensajes OUTPUT por línea, 122 KB → 816 KB),
   `AllocBench` 0,66 ms por vuelta con asignación. **Decisión: `vm` sólo opcodes + `io` todo lo
   demás, tres colas, igual en todos los micros; de momento 1 VM, 1 core, 2 hilos de SO; el
   futuro son dos VM sobre una cola de threads BP (el camino SMP, aparcado sin cerrarlo).**
   Decidido después: **FreeRTOS en todas las familias**, el PC y la **C6** como modelo, y **todo
   en código común** (las familias sólo ponen transporte, primitivas y la creación de las dos
   tareas). Tareas concretas `A1.1`–`A1.7` en la ficha. Samples nuevos: `PrintBench.bp`,
   `AllocBench.bp` (en `samples/benchmarks/`).
12. ✅ **`A1.1` HECHA (5-sep): el PC ya corre con DOS hilos.** `include/bpvm_io.h` +
   `src/bpvm_io.c` (común): hilo `io`, cola con espera acotada, líneas armadas y entregadas de
   una pieza, apagado que drena antes de unir. `emit_text` encola; el scheduler deja de llamar al
   `poll_cb` cuando hay `io`. **Paridad 38/38 byte-idéntica con los dos hilos en cada sample**,
   más `test-mem`, `test-mods`, `test-smphandles`, `sim-smoke` 45/45 y la prueba nueva
   `make io-smoke`: KILL en 3 ms calculando y 17 ms imprimiendo a chorro, **un `OUTPUT` por
   línea** (antes cuatro), salida exacta. Enlaza en las seis imágenes (ninguna lo usa aún).
13. ✅ **`A1.2` HECHA (5-sep): la C6 con las dos tareas** — 20 líneas en `esp32/common`
   (un adaptador de `poll` y el arranque de `io` alrededor de `bpvm_run`), así que **toda la
   familia ESP32 lo hereda**. Medido en placa: cálculo IGUAL (fib(28) 23 580 vs 23 640 ms;
   `AllocBench` 13 188 vs 13 129), salida **3,6× más rápida** (2 000 líneas: 1 700 → 470 ms),
   **6× menos mensajes** `OUTPUT` (11 944 → 1 988) y **3,4× menos bytes** por el wire (816 → 238
   KB); KILL en 46 ms imprimiendo y 18 ms calculando. La pantalla sigue (`GuiRotCycle`).
   ⚠️ Con el depurador armado `io` no arranca (su `pause_cb` lee el wire desde la VM): se
   resuelve en `A1.7`.
   🔬 **Por el camino, un bug de plataforma que sólo se ve en placa**: `cond_timed_wait` con menos
   de un tick daba 0 y no esperaba, así que `io` giraba en vacío y fib(28) tardaba 140 600 ms.
   Arreglado en ESP32 y Pico (redondeo hacia arriba). Pasó los 38 samples del PC con el fallo
   dentro — la cascada haciendo su trabajo.
14. ✅ **`A1.4` HECHA (5-sep): la Pico 2 con las dos tareas.** Mismo gesto que en el ESP32.
   Medido: cálculo igual (fib(28) 17 091 vs 17 157 ms), `AllocBench` +3,6 %, **2 000 líneas de
   7 694 → 3 958 ms (1,9×)**, 6× menos mensajes, 3,4× menos bytes, KILL en 33 ms.
   🔬 **Y el segundo bug que sólo se ve en placa: LA PRIORIDAD ES CONTRATO.** El hilo `io` nacía
   por DEBAJO de `vm_task` en la Pico y no se ejecutaba mientras el programa calculaba: un KILL
   no llegaba nunca (9,9 s y `EXITED OK`). El contrato gana `bpvm_platform_thread_create_io` y la
   regla, medida en las dos placas: **`io` a la MISMA prioridad que la VM** — por debajo no
   funciona, por encima cuesta caudal (C6 486 → 848 ms; Pico 3 958 → 4 832).
15. ✅ **`A1.5` HECHA (5-sep): FreeRTOS en el STM32, las dos placas con las dos tareas.**
   Kernel: el MISMO que la Pico (`FreeRTOS-LTS` V11.3.0, puerto `ARM_CM33_NTZ`) — **de ST no
   sale**: el paquete U5 trae ThreadX y una capa que emula la API de FreeRTOS encima, y CubeMX no
   ofrece FREERTOS para U5 aunque sí para el L552, que es el mismo núcleo. La plataforma pasa a
   `src/platform_freertos.c`, **común** (de las 363 líneas de la Pico sólo 4 eran suyas); migrar
   la Pico es ficha aparte, exige placa. Medido: `PrintBench` **71 357 → 20 464 ms (3,5×)** en las
   dos placas (aquí la UART a 115 200 ES el cuello), cálculo igual en la Nucleo y **11 % más
   rápido** en la Discovery (la VM ya no sondea el wire entre cuantos), KILL en 15 ms.
   🔬 Tercer fallo de plataforma que sólo se ve en placa: se paraba a la quinta ejecución porque
   `vTaskDelete(NULL)` deja la pila en manos de la tarea ociosa, que nunca corría. Lo dijo el
   gancho de malloc fallido en el log; ahora borra el `join`. ⏭️ **Falta que Eduardo mire la
   pantalla de la Discovery** (el modelo de 800×480 se arma bien).
   ✅ **La pantalla de la Discovery, vista por Eduardo con las dos tareas: «se ve bien, todo OK».**
16. 🔄 **`A1.6` APLAZADA Y REORIENTADA (Eduardo, 5-sep)**: LVGL **no** se mueve a `io`. Se trazó
   dónde corre hoy su lazo —lo conduce el programa BP dentro de `vm`, vía `Gui.run()` →
   `__guiRunOnce` → `lv_timer_handler`, y nadie más lo bombea— y el coste ya estaba medido en
   `#424`: 0,4 ms por vuelta cada 33 ms, ~1 % de CPU. No hay caso por rendimiento, y LVGL no es
   reentrante. Lo que Eduardo quiere es que **`Gui.run()` tenga su propio hilo BP** (verde, luego
   sin problema de reentrancia) para no bloquear la aplicación. ⚠️ El obstáculo, ya localizado:
   el bombeo **duerme el hilo del SO** (`vTaskDelay`/`__WFI`/`SDL_Delay`), o sea todos los hilos
   BP; tiene que volver en vez de dormir y dejar turno al scheduler de la VM. Detalle en la ficha.
   **Siguiente: `A1.3`** (S3, C3 y P4: el código ya está, falta verificar EN PLACA) y `A1.7` (el
   depurador por la cola de control). Fichas nuevas propuestas: migrar la Pico a
   `src/platform_freertos.c`; y el `Neopixel` de la familia ESP32.

## 🏁 LA UNIFICACIÓN, CONCLUIDA (Eduardo, 5-sep)

*«Con eso creo que el proceso de unificación lo podemos dar por concluido. Ahora cualquier cosa
que implementemos nueva o reformemos prácticamente hay que hacer 1 vez. Eso nos permite crecer de
forma lineal, sin que el número de placas/micros afecte apenas.»*

Cierran `U1`–`U6`, `P1`–`P2` y `A1`, con el número de `A2` detrás: **91,8 % del código de cada
firmware es común**, 0,7 % de placa, y lo único específico que queda es la pantalla del P4 y del
C6. La evidencia práctica del crecimiento lineal está en las tres últimas fichas: el C6 entró con
siete ficheros, `A1.2` llevó las dos tareas a toda la familia ESP32 con veinte líneas, y `A1.3` se
cerró **sin escribir código**.

⚠️ Lo barato es la **placa dentro de una familia**, no la familia: un silicio con otro SDK sigue
costando su cintura. Y quedan, con ficha y medida, `A1.6`, la migración de la plataforma de la
Pico y `#468` — features y una duda medida, no unificación pendiente.

### Lo que queda en la mesa

**Del modelo de capas (`A3`, 5-sep), abiertas para ir arreglando poco a poco** — decisión de
Eduardo: *«Registralo como pendiente. Lo iremos arreglando poco a poco.»*

| Ficha | Qué | Tamaño |
|---|---|---|
| 🔴 `#469` | La fachada de ADC **miente en el STM32**: sin backend registrado devuelve una rampa inventada y el pinout de la Pico. La paridad dual-VM no puede verlo. Hay que mirar las otras 16 fachadas por si tienen el mismo stub complaciente | mediana |
| 🟠 `#470` | La identidad de placa se contesta por **dos caminos** y ya divergieron; un programa BP en un C3 **se cree un S3** | mediana, y hay que decidir antes qué significa cada campo |
| 🟡 `#471` | El nombre **`Pico`** llega hasta el usuario (`Pico.uptimeMs()` en una STM32). Toca la stdlib: **cambio de lenguaje, decide Eduardo** | pequeña + decisión |
| 🟡 `#472` | El común nombra **dos familias** donde quería decir «micro»; el STM32 acaba con TLS emulada | media hora |
| 📋 `#473` | El **resto de la auditoría, sin verificar** (12 de 90). Dentro, cosas que pintan serias: el `EXITED` del STM32 interpola cadenas del usuario sin escapar; `RUN` escrito cuatro veces | verificar antes de tocar |

### Lo demás que queda (por si se retoma)

## ⏭️ (anterior) — el orden que dio Eduardo al parar (5-sep)

*«Rematamos todo lo que hay pendiente de A1. Luego, me gustaría conocer las proporciones de
código específico del micro, el específico de la familia y el común a todos los micros. Después
de A1 debería quedar muy poco código específico.»*

### 1. Rematar `A1`

✅ **`A1.3` hecha para C3 y P4 (5-sep)**: sin código nuevo, sólo recompilar y grabar. `PrintBench`
**1 610 → 305 ms** en el C3 (5,3×, la mejor de todas las placas) y **71 075 → 20 365** en el P4
(3,5×; sale por puente UART, como el STM32). La pantalla del P4 sigue, a 1 024×600. ⚠️ El
`AllocBench` del C3 sube un 10 %, repetible, pero **no es `A1`**: la C6 —mismo silicio, misma VM—
salió plana; el «antes» del C3 era del 1-sep e incluye `U4`, `U5`, `U6` y `#466`, y el sospechoso
es el planificador de memoria de `U6`. Anotado como pregunta de `U6`.

🔴 **La migración de la Pico al fichero común NO se subió, y por un motivo medido**: con
`src/platform_freertos.c`, `PrintBench` en la Pico pasa de **4 040 a 5 330 ms** (32 % peor,
repetible a ±10 ms, aislado por bisección). Curiosamente el KILL MEJORA (33 → 2 ms), o sea que la
diferencia está en el reparto de turno, no en trabajo de más. No se sube lo que no se entiende.

✅ **`A1.3` COMPLETA (5-sep): C3, P4 y S3 verificados en placa.** El S3 da `PrintBench`
**71 075 → 20 343 ms** (3,5×) y `AllocBench` **plano** (20 244 → 20 384) — y ese número zanja lo
del C3: con dos controles (C6 y S3) queda claro que `A1` no cuesta nada en asignación y que el
10 % del C3 pasó entre el 1 y el 3 de septiembre, o sea en `U6`.

| Paso | Qué falta | Qué hace falta tener delante |
|---|---|---|
| `A1.7` | El depurador por la cola de control: hoy su `pause_cb` lee el wire desde la tarea `vm`, así que **con el depurador armado `io` NO arranca** (interbloqueo puesto a propósito en las tres familias) | una placa y el IDE |
| — | Migrar la Pico a `src/platform_freertos.c` (es un BORRADO, no una fusión: de sus 363 líneas sólo 4 eran suyas) | la **Pico 2** conectada, para verificar en placa lo que hoy funciona |
| `A1.6` | 🔄 **Aplazada y reorientada** por Eduardo: no se mueve LVGL, se le da a `Gui.run()` su propio hilo BP. El obstáculo ya está localizado (el bombeo duerme el hilo del SO) | — |

### 2. El censo de proporciones: común / familia / micro — ✅ HECHO (5-sep)

**El 91,8 % del código de cada firmware es COMÚN**; 7,6 % de familia y **0,7 % de placa**. La
hipótesis de Eduardo se sostiene con margen. Medido por el **artefacto** (el `.map` del enlazador,
no las líneas por carpeta: en la Nucleo `gui_display_sdl.o` está en el build y aporta 0 bytes) y
repetible con `bpgenvm-c/scripts/censo_reparto.py`. Detalle e interpretación en la ficha `A2`.

Lo más útil no es el total: la **Pico es la familia menos unificada** (15,4 %, por ser la más
antigua) y las **dos STM32 las más comunes** (95 %), y lo único que queda como código de placa es
**la pantalla** del P4 y del C6 — que es donde debe estar.

<sub>El enunciado original, por si hay que rehacerlo:</sub>

**La pregunta de Eduardo, tal cual**: cuánto código es específico de un micro, cuánto de una
familia y cuánto común — y su hipótesis, que después de `A1` debería quedar muy poco específico.

📌 Es un censo, así que vale la norma del proyecto: **censar por lo que el código HACE, no por la
carpeta en que está**. Un fichero en `src/` que sólo compila una familia no es común; y
`esp32/common/` es específico de familia aunque se llame «common». Propuesta de método, a
validar con él antes de contar:

- **Común** = lo que compilan TODAS las imágenes (`src/` menos lo que cada build excluye).
- **De familia** = `esp32/common/`, `pico/` sin el `board_desc`, `stm32/port/`… lo que sirve a
  varias placas del mismo silicio.
- **De micro/placa** = `esp32c6/main/`, `esp32p4/main/`, los `Core/` de CubeIDE, `pico/boards/`.
- Y las tres trampas conocidas: los **generados** (blobs de stdlib, `.mdn`) no cuentan como
  código escrito; los `Core/` de CubeMX son **generados por ST**, no nuestros, y hay que contarlos
  aparte o el número miente; y `src/platform_freertos.c` es común de nacimiento pero **hoy sólo lo
  compila el STM32** — o sea, el censo tiene que decir «común» y «común EN USO».
- Lo interesante no es sólo el total: es **la tendencia** (qué había antes de `A1` y qué hay
  después) y **el desglose por eje** (transporte, memoria, FS, GUI, plataforma), que es lo que
  dice dónde queda trabajo de unificación.

### Riesgos que acechan
- **IDE viejo (dist V5) contra firmware nuevo**: sigue subiendo stdlib a `/lib` por CRC, y el
  instalador nuevo la REPONE al siguiente arranque (misma versión, CRC distinto). Sólo se pelean si
  se mezclan versiones; el dist no se toca.
- **IDE nuevo contra firmware viejo** (sin `STAT` por nombre): `statModule` devuelve null y el IDE
  sube como siempre; el CRC con signo lo normaliza `crcSinSigno`.
- **El S3 y las dos STM32 llevan imágenes anteriores a todo esto** hasta que se graben.

## ⏭️ AL RETOMAR (2-sep, 2ª pausa — por límite de tokens) — el RESTO de placas

El repaso general quedó **a medias por la mitad buena**: cuatro de cinco familias verificadas con
la imagen final `U6.10` en su placa (P4, Metro, Pico 2, C3) y tres `/lib` limpiados a mano
(Metro 14, Pico 2 16, S3 2). Falta, en este orden:

1. ✅ **P4** (2-sep, al retomar): `/lib` limpio de por sí (15/15, 0 avisos); `/app/Core.mod`
   borrado con el OK de Eduardo, reset por el wire, 21 ficheros y los mismos números de memoria.
   Trae un matiz a `#466`: pasó por el IDE (tiene `/app/Json.mod`, `/app/JsonDemo.mod`) y su
   `/lib` no se ensució — a dónde escribe el IDE se mira en su código antes del arreglo de raíz.
2. **S3** (COM9): regrabar con la `U6.10` **final** (lleva la previa a la regla unificada; mismos
   números) y comprobar que el `/lib` sigue limpio — se ensucia en cuanto el IDE de V5 sube
   dependencias.
3. ✅ **STM32 Nucleo y Discovery, en código (`U6.11`)**: enumerador = el array estático de 512 KB
   como región exclusiva sin margen, objetivo 0 → los 512 KB de siempre; INFO y RUN sobre lo
   planificado; `test_mem` 22/22; los dos `.elf` compilados headless con 0 errores. **Falta
   grabarlas** (Eduardo, desde CubeIDE) y leer su línea `vm:` — y revisar su `/lib`, que su
   instalador no avisa del rancio.
4. **`#466` de raíz — ANALIZADO en el código; decisión de Eduardo pendiente** (mecanismo, tres
   opciones y recomendación, en la ficha): el IDE sube a `/lib` las deps que están en
   `EMBEDDED_CORE_MODS` (+Gui) con content-check por CRC — a propósito, por un bug de vtables del
   13-jun; los tres instaladores copian «si falta»; y el aviso de `#422` sólo vive en la Pico: en
   el ESP32 se perdió al regenerar los blobs (`#446`: iba en un fichero GENERADO) y el STM32 nunca
   lo tuvo. Recomendación **A**: el IDE deja `/lib` en paz (sus deps de stdlib a `/app/<proj>`, que
   gana) y un helper común refresca `/lib` cuando no coincide con lo embebido. Mientras no esté,
   cada Run desde el IDE vuelve a ensuciar las placas limpiadas hoy. 🛠️ **Al retomar: la regla
   definitiva de Eduardo está en la ficha, y los pasos (1) `STAT` por nombre + `magic` y (2) el
   helper común del instalador ya están hechos y verificados (host + seis builds). (3) el IDE también:
   pregunta a la placa por cada dependencia, fuera `EMBEDDED_CORE_MODS`, verificado con el cliente
   real contra el simulador y fat-jar reconstruido. **Falta (4) las placas**: Metro y Pico 2 con el
   `uf2` nuevo (`BOOTSEL` por el wire) y ver `lib: … repuesto` en su log; luego un Run desde el IDE
   nuevo sin subir stdlib. De paso: el CRC del wire salía NEGATIVO (bit 31, `long` de 32 bits) y
   el IDE re-subía la mitad de los ficheros idénticos — arreglado en los dos lados.**

La matriz completa, con lo hecho, está justo debajo.

## ⏭️ (2-sep, 1ª pausa) — REPASO GENERAL de imágenes y placas

Eduardo, al parar: *«A la vuelta repasamos el estado general de todas las imágenes y placas.»*
Ésta es la matriz de partida, escrita en caliente el 2-sep a las 14:30, no reconstruida:

| placa | imagen que LLEVA | frente al repo (`U6.10` final, regla unificada) | pendientes en la placa |
|---|---|---|---|
| **P4** (COM14) | `U6.10` final, grabada 14:20 | ✅ **es la del repo** — verificada byte a byte (28 668 KiB @0x48000a7c), y otra vez tras el reset del repaso | ✅ `/lib` **limpio de por sí** (15/15, 0 avisos, aunque pasó por el IDE); ✅ `/app/Core.mod` borrado (2-sep, al retomar) |
| **S3** (COM9) | ✅ **`U4.1` + `#466`**, regrabada el 3-sep (19:01) | ✅ verificada: mismos números (160 KB, techo 272, margen 26564) | ✅ `/lib` **repuesto por la imagen** (13 MOD6 de V5 → MOD7 al primer arranque; ayer parecía limpio porque el ESP32 no avisaba) |
| **C3** (COM3) | ✅ **`U6.10` final**, regrabada en el repaso (2-sep) | ✅ verificada: 128 KB, techo 136 por el contiguo, 64/64 — idéntico | ✅ `/lib` **limpio de origen** (lo aprovisionó el instalador; el IDE de V5 nunca le subió nada: 0 avisos), ✅ `log` devuelto a 0 |
| **Metro** (COM4) | ✅ **`U6.10` final**, regrabada en el repaso (2-sep) | ✅ verificada en los dos modos, byte a byte | ✅ `/lib` **limpio** (los 14 de V5 borrados y repuestos por la imagen, `#466`); NO había `/app/Core.mod`; ENV `psram=0`, `SQLite=2`, `log=1` — **volverá a ensuciarse mientras el IDE de V5 suba stdlib** |
| **Pico 2** (COM22) | ✅ **`U6.10` final**, regrabada en el repaso (2-sep; llevaba la del 29-ago) | ✅ **rama SRAM en variante A verificada**: `357 KB @ 0x20024ab8`, y su `stack=90` del ENV respetado (heap 267 + pilas 90; INFO 273736/92160, igual que antes) | ✅ `/lib` **limpio** (16 de V5 y de una imagen vieja borrados y repuestos por la imagen, `#466`); ENV `stack=90`, `gc=1`, `mpu=1`, `log=1`; FS de 1 MB al 24 % — **volverá a ensuciarse mientras el IDE de V5 suba stdlib** |
| **STM32 Nucleo / Discovery** | ✅ las dos con **`U4.1` + `#466`** (Eduardo, 3-sep, sellos 19:08 y 19:12) | ✅ verificadas: `vm: 512 KB en SRAM estática`, INFO 393216/131072 | `/lib` instalado entero por la imagen en las dos (estaban vacíos); en la Nucleo un `/app/Json.mod` MOD6 que repondrá el IDE |
| host / simulador | — | ✅ repo, `test-mem` 19/19, `sim-smoke` 40/40, paridad 38/0/0 | — |

**Los seis builds compilan** con el repo actual (host, Pico, S3, C3, P4, Nucleo, Discovery).

### Lo que ese repaso debería decidir

1. **Regrabar C3, S3 y Metro con la imagen final** — no por necesidad (sus números están fijados por
   `test-mem` y no cambian) sino para que «lo que lleva la placa» sea «lo que hay en el repo» en las
   cuatro que ya deciden con el planificador. La Metro se graba por el wire (`BOOTSEL`); el C3 y el
   S3 con `idf.py flash` (ojo al DTR/RTS de cada uno, está en `tools/medir_margen.ps1`).
2. ✅ El `/app/Core.mod` del P4 ya está fuera (el `log` del C3 ya está a 0). Y **`#466` de raíz**
   (la Metro ya está limpia a mano): que el IDE no suba stdlib a `/lib`, y que el instalador
   refresque `/lib` cuando no coincida con lo embebido. Mientras tanto, cada Run desde el IDE de
   V5 contra una placa de V6 vuelve a ensuciar `/lib`.
3. ✅ **El STM32**: migrado en código (`U6.11`, 2-sep al retomar); falta grabar las dos placas y
   leer su línea `vm:`.

## ⏭️ (anterior) — `U6` tiene diseño y medidas; falta escribir código

La sesión del 31-ago acabó siendo **de análisis y diseño**, y con eso `U6` (la organización de
la memoria) pasa de ficha roja sin empezar a tener censo, hallazgos, forma y las medidas que la
sostienen. Nada de lo de abajo es estimación: todo está medido en placa o leído del código.

### Por dónde empezar, en orden de menos riesgo

1. ✅ **`CHIP_MARGEN_SISTEMA` en el S3 y el C3 — HECHO en código (`U6.7`, 2-sep).** Tres
   constantes por chip (objetivo / margen medido / suelo) y `vm_buffer_init()` mide, comprueba
   las dos restricciones y baja diciéndolo. **Visto en el C3** (2-sep): la línea sale exactamente
   como se predijo —`objetivo 128, techo 136 por el bloque contiguo, margen 17588`— y el reparto
   no se mueve (64/64). **El S3 queda por contrastar** al reflashear; la predicción es 160 KB con
   techo 264 por el bloque contiguo.
2. **🟡 El enumerador de regiones** (`U6.5`/`U6.6`), familia a familia. ✅ **Pico HECHA (`U6.8`,
   2-sep)**: `bpvm_mem` común + `test-mem` (14/14 contra las placas medidas) + la Pico
   enumerando; verificado en la Metro en los dos modos, byte a byte con las líneas de ayer.
   ✅ **S3 y C3 HECHOS (`U6.9`, 2-sep)**: `vm_buffer_init` queda en enumerar (`heap_caps_get_info`)
   y tomar (malloc + escalera); verificado en el C3 **y en el S3** con los números esperados.
   ✅ **P4 HECHO (`U6.10`)**: el margen es **de la región** y va contra el TOTAL, no contra el
   contiguo — lo cazó el P4 con 508 KiB de diferencia; corregido, verificado byte a byte con su
   imagen anterior (28 668 KiB @0x48000a7c) y su caso medido en `test_mem` (19/19). **La familia
   ESP32 entera y la Pico deciden ya con la misma función.** **Queda el STM32, el último**: su
   techo lo comprueba el ENLAZADOR y eso es más fuerte que cualquier comprobación en marcha — su
   enumerador devuelve el array estático y el aserto se queda.
3. **🔴 El cosido de dos regiones.** Es lo que toca el GC, y necesita las cuatro comprobaciones
   inventariadas en `V6_IDEAS`. Poner `coser=0|1` en el ENV **desde el primer día**: un mecanismo
   que no se puede apagar no se puede medir.

### Lo que quedó a medias

- **El margen de la Pico y el del STM32 sin MEDIR.** El protocolo existe
  (`bpgenvm-c/tools/medir_margen.ps1`) pero habla el wire del ESP32; para esas dos hay que
  adaptarlo.
- **El AOT del C3.** Hoy se descubrió que su ausencia era un hueco del port, no del silicio: le
  faltaba `esp_mm` en `REQUIRES` y ya compila con el camino del `.mdn` dentro. Falta que el IDE
  compile el `.mdn` con la ISA del C3 y probarlo en placa (`U6.4`).
  ⚠️ Con un aviso: el P4 y el C3 comparten el tag `MDN_ARCH_RISCV` y se separan por la ABI de
  coma flotante, **pero eso no separa la extensión de atómicos**, que el P4 tiene y el C3 no.
- **`U6.F1` aparcada** a petición de Eduardo: con PSRAM, qué buffers NO-BP agrandar con la SRAM
  ociosa. Dentro está el método, que importa más que la lista.

### Riesgos que acechan

- 🔴 **Las cuatro comprobaciones del hueco.** Si se implementa el cosido sin ellas, el fallo no
  es un error: es la VM escribiendo o ejecutando memoria del IDF. Están listadas en `V6_IDEAS`.
- 🟡 **El S3 corre una imagen del 30-ago.** Su margen se midió con ella; si se reflashea conviene
  repetir la medida, que ahora cuesta un minuto.
- ✅ **Imagen final (`U6.10`, regla unificada) en P4, Metro, Pico 2 y C3** (2-sep, todas
  verificadas en placa). El **S3** lleva la `U6.10` previa a la regla unificada (mismos números,
  fixture); regrabar cuando se conecte. El S3 subió a 346 064 B libres / 278 528 contiguos con la imagen nueva (+7,7 KB:
  lo que liberó la unificación); si se remide su margen, añadir el caso a `test_mem`, no
  sustituir el del 30-ago.
- ℹ️ **Grabar la Pico/Metro sin dedo**: `BOOTSEL` por el wire la abre como disco `RP2350` y se
  copia el UF2 (`bpgenvm-c/pico/build/bpvm_pico.uf2`). El `RESET` por el wire corta el puerto
  antes de contestar — la excepción al leer la respuesta es normal.

### 🧹 Pendientes manuales

- El **C3** se quedó con `log=1` en su ENV (se puso para medir). Devolverlo a `0`.
- ✅ **El S3 ya está limpio** (2-sep, por el wire): fuera `/app/Core.mod` y el `/lib/Pico.mod` de
  V5; el arranque reinstala el `Pico.mod` de la imagen (⚠️ corregido 2-sep: el ESP32 NO tiene el
  aviso de `#422` — se perdió en `#446`; ver `#466`).
- ✅ El `/app/Core.mod` del resto de placas: la Metro y la Pico 2 no lo tenían; el del P4, fuera
  el 2-sep al retomar.

### `U6` — la organización de la memoria: censo, medidas y forma (sesión de análisis)

Sesión sin apenas código y con mucho medido. `U6` estaba en rojo desde el 28-ago, abierta con una
pregunta de Eduardo (*«¿es por micro o por familia? ¿está previsto unificarlo?»*), y sale de hoy
con censo, cuatro hallazgos, tres medidas en placa y una forma escrita.

**El censo (`U6.0`)** — los cinco puertos **y los dos entornos de PC**, que estaban fuera del
recuento. Cuatro cosas:

1. «Cuánto» eran **dos filosofías mezcladas sin decirlo**: calcular (Pico, P4) contra una
   constante a mano (STM32, S3, C3).
2. El **reparto** estaba unificado en las placas y **no** en el host ni en el simulador.
3. El **techo** tiene tres regímenes, y el del ESP32 sólo se conoce preguntando — y es el
   **bloque contiguo**, no el total.
4. El **margen** ⚠️ *(corregido el mismo día: dije que sólo la Pico lo nombraba y el STM32 también
   lo hace, en el `.ld`, donde no miré)*.

**Lo cerrado**

- `U6.1` — el **simulador y el host** reparten como las placas y leen `stack=N`. Importa desde
  `U3.24`: el sim es el arnés del REPL común, y un doble más amable que el original no caza nada.
- `U6.3` — **la SRAM ociosa no compensa**, medido en la Metro con `psram=0/1`: la PSRAM cuesta
  entre 4,3 % y 7,0 % en tres cargas distintas, a cambio de 17× más memoria. Y el porqué es lo
  interesante: **el intérprete entierra la latencia de memoria** (la misma dispersión cuesta 15 %
  en el host y 5 % en la placa). De ahí salió `BenchMem.bp`, con su control.
- `U6.4` — **el margen del S3 estaba mal**: 26 564 B, no 86 256. El viejo se midió con una carga
  que nadie anotó. Ahora hay protocolo escrito (`tools/medir_margen.ps1`) y las dos placas se
  llevan 1,5×, no 4,9×.

**Lo diseñado** — `U6.5` y `U6.6`: tres capas (constantes de la imagen, ENV, enumeración en
marcha) y siete pasos de arranque de los que **sólo el primero es de la familia**. La clave es
`exclusiva`: con ella, «todo lo que quede» y «una constante» dejan de ser rivales y son una
regla con dos clases de memoria.

**Y dos correcciones mías que conviene recordar**, las dos a preguntas de Eduardo:

- *«la C3 es RISC-V, debería soportar AOT»* — y tenía razón. Su ausencia era un hueco del port
  (faltaba `esp_mm` en `REQUIRES`) y yo había escrito en el código que era del silicio.
- *«¿quién reserva esa memoria?»* — nadie: son **37 reservas del IDF** que dejan el espacio libre
  en 7 trozos. Yo había dicho que las regiones eran contiguas —cierto en direcciones— y salté de
  ahí a que el espacio libre lo fuera.

**Y una cosa que se midió y salió mejor de lo esperado**: coser dos regiones **se puede**. Los dos
trozos se piden a la vez, salen en orden de dirección y el hueco entre ellos son **8 024 B**. El
heap del C3 pasaría de 64 KB a ~165 con el margen puesto: **2,6×**.

### `P1.C3.3` — el C3 ejecuta BasicPlus, por su único cable

🧹 **Pendiente manual**: el C3 se quedó con `log=1` en su ENV (se puso para medir la marca
de agua). Devolverlo a `0` cuando ya no haga falta — el log de ejecución llena la región de
8 KB en ~26 colectas del GC (`#423`).

`fib(28) = 317811`, `exit 0`, subido y ejecutado desde el host por el **USB-Serial-JTAG**, sin
adaptador. La placa saluda como `bpvm-esp32c3` y el INFO da sus GPIOs, su ADC y su SRAM.

Lo desatascó un dato de la placa, de Eduardo: *«hay 2 pulsadores; para grabar se pulsan los dos
y se suelta reset. En un arranque normal el USB es NUESTRO.»* Si el USB no hace falta para
grabar, lo ocupa el wire y la consola se va a UART0.

Antes se descartó su otra idea —multiplexar con DTR— **midiendo**: el C3 no tiene USB-OTG y el
periférico USB-Serial-JTAG no expone ninguna línea de control al software (los ~70 campos de sus
registros, ni `dtr` ni `rts` ni `line_state`). Ese par lo consume el hardware para reset y boot.

Cinco cosas rotas por el camino, todas de la misma familia —**heredar del S3 sin preguntar al
silicio**—: la tabla de particiones de 16 MB (el chip tiene 4), los pines del wire en GPIO43/44
(que en el C3 no existen), una declaración duplicada del init del transporte, un banner que
decía «UART0» fuera cual fuera el cable, y la identidad de placa del S3.

⚠️ **Y un diagnóstico mío que era falso.** Leí `throw ... No space in heap` en el log y concluí
que el heap se agotaba; llegué a cambiar por eso el tamaño del bloque de la VM. Era la
**prefabricación** del OOM de `#430`, que no puede hacerse en un módulo que no importa `Core` —
un no-evento que sonaba idéntico a un fallo. Arreglado el aviso para que distinga las dos cosas.
El 128 KB se queda, pero por su motivo real: dobla el heap (32 → 64 KB) sin tocar los hilos y
sobrando 131 KB de margen.

💡 Y de ahí salió una idea de Eduardo que quedó anotada en `V6_IDEAS`: **coser las dos regiones
de RAM en un solo espacio de direcciones**, marcando el hueco como bloque ocupado igual que se
hace con el de SQLite. Las regiones del C3 resultaron ser **contiguas** (lo parte ESP-IDF por
capacidades, no por direcciones), así que valdría ~250 KB en vez de 136. Falta medir si los dos
bloques caen pegados.


### `P1.C3.2` — la familia ESP32 ya tiene un común de verdad

Pregunta de Eduardo antes de empezar: *«¿no hace falta el FS y la gestión de la memoria
también?»*. Medido, la respuesta es distinta para cada uno:

- **El FS: nada que hacer.** `fs_lfs_esp32.c` busca la partición **por nombre** (`bpdata`) —
  cero código por chip. Lo único particular es `partitions.csv`, y el C3 ya tiene el suyo.
- **La memoria: sí, pero no es un problema de reestructurar.** El *código* ya era común; lo
  particular es **un número**, y tiene que ser constante de compilación porque
  `vm_buffer_init()` corre **antes** que `board_mgr_esp32_boot()`, o sea antes del ENV.

El censo cambió el tamaño del trabajo: **9 de los 10 `.c` de `esp32/main/` los usan los tres
chips** — ya era el común sin decirlo. Y mover los proyectos costaba **193 rutas relativas**,
cuyo fallo típico no es «no compila» sino «resuelve a otra cosa».

Así que el mismo reparto con otra forma: las **fuentes** salen a `esp32/common/` y cada
proyecto conserva su componente `main/` (que el IDF exige) con un `chip_cfg.h` dentro. Ni una
profundidad cambia — unas 30 líneas en tres `CMakeLists`.

**Lo que de verdad se arregló**: el `main.c` del C3 era una copia de 176 líneas que se
diferenciaba en **dos números y un nombre**, y los arreglos no viajaban — `#464` y `#465` se
hicieron en uno y no en el otro, esta misma semana. Ahora hay uno.

Verificado en el artefacto: el mismo `main.c` produce `ESP32-S3` en una imagen y `ESP32-C3` en
la otra (`strings` de los `.bin`), y las tres conservan el tamaño exacto de antes.

⏭️ Queda `P1.C3.3`: **medir** el heap del C3 en placa. Los 96 KB de ensayo están puestos para
enlazar, no medidos, y el fichero lo dice en mayúsculas.


### `U3.25` — y el arnés nuevo cazó dos verbos que mentían

Al ampliar `sim_smoke.py` con lo que el común tiene y el arnés no miraba, dos rojos, y de
verdad: `MKDIR` y `RMDIR` eran **stubs que contestaban OK sin tocar nada**. El común creaba
directorios en cada PUT (`crear_dirs_padre`, `#455`) y decía «hecho, nada» cuando se le pedía
uno explícitamente, dos funciones más abajo.

Y el sim **tenía un `MKDIR` de verdad**: migrarlo sin mirar fue unificar hacia abajo — la
regresión de `#455` otra vez. La diferencia con la vez anterior es el tiempo: **diez minutos
en el host**, no una placa tres días después.

Arreglado. `MKDIR` crea o da error con nombre; `RMDIR` distingue ahora «no existe»
(`NOT_FOUND`) de «no está vacío» (`NOT_EMPTY`), que el stub juntaba en un OK.

⏭️ **Falta verlo en placa**: en el host el backend es littlefs sobre una imagen y en el micro
es flash de verdad. Gesto: `MKDIR /app/x`, `LIST_DIR /app`, `RMDIR /app/x`.


### `U3.24` — el simulador al REPL común: el arnés que faltaba

Cerrado `U3` (las comunicaciones), quedaba una deuda que el propio bug del `LIST` había dejado
por escrito: **`bpvm-sim` era el quinto consumidor del REPL y no había migrado**. 21 verbos
propios, ejercitados por el wire desde `sim_smoke.py`… contra su propio código.

Migrado. El sim baja a **4 verbos propios** (`RUN`, `KILL`, `RESET` y la gestión de placa) y
`tools/bpvm_sim.c` pierde 124 líneas. De regalo gana `LIST_DIR` y `RMDIR`, que no tenía.

**Y con control, que es lo que lo hace valer:** se volvió a meter el bug de ayer (el `memset`)
y `make sim-smoke` lo cazó — `FAIL` en las dos comprobaciones de `LIST` con contenido. Doce
segundos en el host contra el viaje a la P4 de ayer.

Detalle fino que conviene recordar: la comprobación *«LIST tras formatear → vacío»* **pasa con
el bug vivo**. Cubre el verbo pero no el camino. La que caza es la que tiene ficheros dentro.

Verificado: `sim-smoke` 25/25, `boardsim-smoke` verde, paridad 38 PASS / 0 FAIL / 0 SKIP.


## ⏭️ AL RETOMAR (31-ago, cierre 2) — dos cosas del IDE

### ✅ `#463` — `Core.mod` se subía a `/app` y creaba un override sin querer

**Lo vio Eduardo mirando el árbol de la placa**, sin que fallara nada: `Core.mod` aparecía en
`/app` **y** en `/lib` con el mismo tamaño. La lista `EMBEDDED_CORE_MODS` del IDE tenía **13**
nombres donde el firmware embebe **14**; faltaba `Core`. Arreglado, fat-jar rehecho y
verificado en la Discovery (`Core.mod → /lib`, CRC idéntico, salta el PUT).

📌 **Dos correcciones de Eduardo que cambiaron el diagnóstico**: que `/app` gane a `/lib` **está
bien y es a propósito** —*«si tienes un módulo más moderno lo puedes probar; de la otra forma no
se podría sin borrarlo de /lib»*—, así que el fallo no era la precedencia sino **crear el
override sin pedirlo**. Un override deliberado se recuerda; uno accidental te espera.

🧹 **Pendiente manual**: borrar el `/app/Core.mod` que dejaron las ejecuciones anteriores. El
arreglo evita crear el próximo, no limpia el que hay — y mientras esté, sigue ganando.

⚠️ **Lo de fondo, abierto**: esa lista es un **gemelo escrito a mano** de lo que embebe el
firmware, y se habían separado por uno. Lo robusto es que **lo diga el dispositivo** (el `LIST`
de `/lib` ya existe y el IDE ya compara por CRC). Mientras siga siendo una constante en Java,
volverá a desincronizarse.

### ⏸️ EN SUSPENSO — «faltan módulos» al ejecutar desde `run`

**Síntoma de Eduardo**: ejecutar desde el IDE (que copia los módulos) y a continuación desde el
`run` de la consola (que no copia) → *«me dice que faltan módulos»*. **Decisión: se deja en
suspenso a ver si vuelve a ocurrir.**

**Lo que se averiguó por el camino y sirve si reaparece** — hay una asimetría real:

| quién lanza | `path` que manda | basedir que se deduce | dónde busca las DEPENDENCIAS |
|---|---|---|---|
| IDE, con proyecto | `/app/<proj>/X.mod` | `/app/<proj>` | el proyecto → `/app` → `/lib` → `/sys` |
| IDE, sin proyecto | `/app/X.mod` | *(plano)* | `/app` → `/lib` → `/sys` |
| consola `run X` | `consoleCwd + "/X.mod"` | según dónde esté el `cd` | lo que salga de ahí |

📐 La regla está en `bpvm_entry_resolve` (`bpvm.c:779`) y el basedir en
`bpvm_fs_set_basedir_from_module` (`fs_facade.c:98`), que **sólo lo activa si la ruta empieza
por `/app/<proj>/`**. O sea: **el mismo programa puede buscar sus dependencias en sitios
distintos según por dónde lo arranques**.

🔴 **Pero NO está confirmado que sea la causa de lo que vio Eduardo.** Faltan dos datos que sólo
da la reproducción: **el texto exacto del error** (*«faltan módulos»* y *«no encuentro el
módulo»* son mensajes distintos y apuntan a sitios distintos) y **dónde estaba el `cd`**. Sin
eso es una teoría que encaja, no una causa. *(Y cabe que fuera parte de lo de `#463` y ya esté
resuelto.)*

---

## ⏭️ AL RETOMAR (31-ago, cierre) — **la matriz de placas, al día**

| familia | estado | qué ha corrido con la imagen de hoy |
|---|---|---|
| **ESP32-P4** | ✅ **probada** | `JsonDemo` · `trytest` · `GuiEvSpike` · `stop` |
| **STM32 Discovery** | ✅ **probada** | `JsonDemo` · `GuiColorDemo` · `GuiEvSpike` · `GuiEvLat` ×3 · `trytest` · `stop` ×5 |
| **Pico / Metro** | 🔴 **la stdlib nueva SIN ESTRENAR** | — |
| **ESP32-S3** | 🔴 **la stdlib nueva SIN ESTRENAR** | — |
| **STM32 Nucleo** | 🔴 **la stdlib nueva SIN ESTRENAR** | — |

El gesto que cierra cada una son dos programas con **salida conocida e idéntica en las tres
que ya pasaron**, o sea que valen de oráculo:

- **`trytest.bp`** → `atrapado: n era negativo` + los otros dos mensajes. Es **el camino más
  tocado de estos dos días**: `#459` cambió cómo se enlaza la variable de un `catch` sin tipo,
  `Exception` ganó `toString()`, y de los 56 ficheros que ganaron `import Core`, **23 fue por
  `RuntimeError`**.
- **`JsonDemo`** → la stdlib nueva corriendo (`Core.mod` regenerado).

⚠️ **Y una precisión sobre el `/lib`, para no dar de más por probado**: el IDE refresca por CRC
**sólo los módulos que cada programa pide**. La Discovery tiene al día `Core`, `Json`, `Gui` y
`Pico` — **no los 27**. Los que ningún programa haya pedido pueden seguir siendo los viejos, y
eso morderá el día que se use uno. Para dejar una placa al día ENTERA: **formatear el FS** y que
la imagen reinstale.

---

## ⏭️ AL RETOMAR (31-ago, tarde) — **la latencia de los eventos del GUI, medida y partida en dos**

### 🔬 Dónde retomarlo: `#462`, y está listo para atacarse

De una sensación de Eduardo —*«en las ESP32, cuando se ejecuta un programa, se queda todo
bloqueado; los eventos pasan pero mucho más lentos»*— salieron **cuatro cosas distintas**:

| | qué es | estado |
|---|---|---|
| 1 | el planificador **nunca cedía el turno al SO** — el gancho `bpvm_platform_thread_yield()` existía en las 4 plataformas y **no lo llamaba nadie** | ✅ **arreglado**; el `stop` del P4 volvió a funcionar |
| 2 | el **quantum se mide en OPCODES** (1024) y `Gui.run()` gasta uno bombeando LVGL entero | 🟢 **medido**: `1,21 ms × vueltas`. Sin arreglar |
| 3 | un **suelo de 48 ms** independiente del quantum | 🔴 **sin investigar**. Sospechoso: `LV_DEF_REFR_PERIOD = 33 ms` (`include/lv_conf.h:82`) |
| 4 | el `exit` de un `stop` es **una CARRERA**: dice `OK` o `KILLED` según dónde caiga el KILL | 🐛 hallazgo colateral, sin arreglar |

**El modelo, validado con tres puntos en la STM32 Discovery** (`samples/GuiEvLat.bp`, escrito
para esto):

```
lat = 1,21 ms/vuelta × vueltas + 48 ms de suelo      (error máx. 5 ms en un rango de 32×)

quantum 1024 → 403 ms     quantum 128 → 87 ms     quantum 32 → 63 ms
```

📌 **Se predijo antes de medir**: para `quantum=128` el modelo decía ~95 ms y salieron **87**.
Con dos puntos un ajuste encaja por construcción; con la predicción cumplida, ya no.

⏭️ **Lo que queda, y son decisiones de Eduardo, no trabajo a medias:**
- **El quantum por TIEMPO**, como ya hace miVM (`bpvm_internal.h:280` lo tenía anotado como
  diferencia entre VMs, sin ver que aquí valía 400 ms). Toca el planificador → con placa.
- **El suelo de 48 ms**: averiguar QUÉ es antes de tocar. Bajar el refresco de LVGL cuesta CPU
  y sería adivinar.
- **El `exit` inestable**: pequeño, aislado, y de los silenciosos.

🔧 **Herramienta nueva y reutilizable**: `quantum=N` en el ENV, en las cuatro familias (mismo
patrón que `stack=N`). ⚠️ Se lee **una sola vez al arrancar** → hay que **resetear** tras
cambiarla. Y **no es configuración para dejar puesta**: con 32 los eventos van finos y el resto
paga más cambios de contexto.

⚠️ **Si una placa se comporta rara, mira primero si tiene `quantum` puesto en el ENV.**

### 🧭 Lo que enseñó la tarde, y es de método

**La medida tumbó DOS diagnósticos míos seguidos.** Primero *«es la prioridad 5 del P4»* — falso:
la Discovery, sin RTOS y sin prioridades, tiene la misma latencia. Después *«la latencia bajará
proporcionalmente al quantum»* — falso: bajó 6,3× de 32, porque había un suelo.

📌 **Y la clave la dio una frase de Eduardo que resultó ser un artefacto**: la Discovery *«funciona
muy bien»*. No funcionaba bien — **tardaba 400 ms igual**. Lo que pasaba es que los clics iban a
800-2200 ms y nunca se acumulaban. **Sin cronómetro, dos diagnósticos se apoyaron en una
impresión que dependía del ritmo del dedo.** De ahí salió `GuiEvLat.bp`: el spike sólo daba el
ORDEN de las líneas, y con el orden no se distingue «drena lento» de «cliqué despacio».

---

## ⏭️ AL RETOMAR (31-ago) — `#441` cerrada y **LAS CINCO IMÁGENES, del mismo árbol**

### Lo primero: el estado de los artefactos, comprobado por fecha (no de memoria)

| eslabón | estado |
|---|---|
| `bpstdlib/*.mod` | **27/27 al día**, ninguno más viejo que su `.bp`, **todos MOD7** |
| blobs embebidos (`core_mod.c`, `esp32_mods.c`, `stm32_mods.c`) | ✅ posteriores al `.mod` más nuevo |
| **las CINCO imágenes** | ✅ posteriores al último cambio de núcleo (`interp.c`, `#441`) |
| fat-jar del IDE | ✅ posterior al frontend |

```
bpvm_pico.uf2            31-ago 07:37     600.064 B
bpvm_esp32_merged.bin    31-ago 07:38     505.760 B
bpvm_esp32p4_merged.bin  31-ago 07:39   1.361.200 B
Nucleo_u575b.elf         31-ago 07:39   3.701.456 B
Discovery_u5g9j.bin      31-ago 08:05     889.172 B
```

🔴 **El Discovery llevaba sin construirse desde el 29-ago** — se le habían pasado los tres
bugs del compilador, la stdlib nueva y `#441`. Construido y **con la trampa de
`PUBLICAR.md` confirmada en vivo**: el `cleanBuild` headless regenera el `.elf` **pero no
el `.bin`**, y el que había al lado era **del 5 de agosto**. Veintiséis días. Regenerado
con `arm-none-eabi-objcopy`.

⚠️ **`dist/firmware/` sigue siendo del 20-21 de agosto, y ESO ESTÁ BIEN**: es la V5
publicada. No se toca hasta publicar — y hoy mismo ha servido de **oráculo** para demostrar
que `#459` no lo habíamos roto nosotros. Sobrescribirla habría destruido esa capacidad.

### Lo cerrado el 31-ago

**`#441`** — los seis opcodes de globales estrechas (`GET/SET_GLOBAL_I8/U8/I16/U16`), ya en
la VM-C. El arreglo era trivial; lo que costó fue el test, **y resultó que ya existía**:
`miVM/MainNarrow.java` los ejercita los seis fabricando un `.mod` a mano con `ModWriter`, y
nadie lo había pasado nunca por la VM-C. Con control en rojo (`opcode 0x40 desconocido`) y
después **19 valores byte a byte iguales**.

### Decisiones de Eduardo

- **`#452` APLAZADA al script de test.** *«Con los comandos tenemos 3 casos de uso: el
  usuario, el IDE internamente, y el futuro script.»* El tercero no existe todavía, así que
  juzgar los verbos ahora sería decidir con un consumidor de menos a la vista. Se repasarán
  **los 29**, no cinco. 📌 De la tabla que queda hecha, lo más útil: **`FORMAT` hace falta
  HOY** — tras flashear hay que formatear el FS y no hay forma desde el IDE.
- **`#461` ABIERTA**: *«los path no deberían reservar espacios fijos… la longitud ha de ser
  variable»*. Medido: los paths reales miden **13,3 de media**, y el listado del ESP32
  reserva **7,5 KB de `.bss`** para ~1,5 KB de contenido. El remedio es el pool de `#440`,
  aplicado hoy mismo en la tabla de símbolos. Empalma con `LIST`, el último verbo de `U3`.
- **`#456`** replanteada: FatFs y littlefs admiten **255**; el 64 es un tope NUESTRO.

⏭️ **Lo primero al retomar sigue siendo lo mismo**: Pico, S3 y STM32 **con la stdlib nueva
sin estrenar** — y ahora las cinco imágenes están listas y del mismo árbol. Formatear el FS
antes, que hoy cambiaron los 27 módulos.

## ⏭️ AL RETOMAR (30-ago, tarde-noche) — tres bugs del COMPILADOR, y uno venía publicado en V5

### Lo que hay que saber antes de tocar nada

🔴 **Al flashear cualquier placa, FORMATEA EL FS.** Hoy han cambiado **los 27 módulos de
la stdlib** (todos pasan a `.mod` v7 y `Core` además gana un método). El ESP32 y el STM32
instalan un módulo embebido **sólo si no existe** (`esp32_mods.c:4862`), así que una placa
con `/lib` de ayer se queda con el viejo **y no lo dice** — la Pico sí lo canta en el log
(`main.c:1330`). Es la ficha *«un módulo rancio sobrevive y NADIE lo dice»*, latente hasta
hoy porque ninguna imagen había traído la stdlib cambiada.
*(El camino del IDE lo esquiva solo: compara por CRC y sube a `/app`, que tiene precedencia.)*

### Lo cerrado

| ficha | qué |
|---|---|
| **`#457`** | el compilador escribía `.mod` **v7** y su propio lector de interfaces sólo aceptaba **v6** → **la stdlib no se podía regenerar**. Un `return null` mudo, con el error saliendo en otro fichero |
| **`#458`** | **norma nueva**: si usas un tipo de `Core`, importas `Core`. Y la pasada de interfaz **deja de tirar miembros en silencio** |
| **`#459`** | un `catch` sin tipo entregaba un valor roto que **volcaba la tabla de símbolos** por pantalla. **Publicado en V5** |
| `U3.21` + **`#455`** | el grupo `PUT` del ESP32 al común; el `mkdir` de los directorios padre, recuperado antes de que la migración borrara la última copia |
| `Math` | entran `clamp` `wrap` `hypot` `remap` (`atan2` ya estaba, y la ficha la seguía pidiendo) |
| **`#443`** | cerrada **por decisión**: la hipótesis está contestada y el cierre de V6 trae su propia tanda |

**Abiertas nuevas**: `#456` (un `path` >63 chars se trunca en silencio y la operación dice OK).

### 🔑 Las tres son la MISMA enfermedad

Una suposición que dejó de ser cierta y a la que nadie volvió:

- el `import Core` implícito, inyectado **en una de las dos pasadas**;
- el lector de interfaces, congelado en `MOD6` cuando el escritor pasó a `MOD7`;
- el emisor tratando una excepción como string porque *«en BP los throws son típicamente
  strings»* — cierto **dos versiones atrás**, y el propio comentario lo dejaba escrito.

📌 **Al subir un formato o cambiar una regla, censar los LECTORES.** Con `.mod` eran tres,
no dos: las dos VMs lo aceptaban y el que se quedó atrás fue el del compilador, que nadie
recuerda que existe porque casi siempre trabaja en memoria.

### ⚠️ Y el arnés estuvo VERDE con un bug de memoria dentro

`#459` volcaba el heap por pantalla y `compat.sh` daba **38 PASS**: las dos VMs producían
**la misma basura, byte a byte**. *Un oráculo que sólo compara dos implementaciones no
puede ver un fallo que ambas comparten.* Es la trampa del falso-PAR en su forma pura, y
refuerza `#444`: hace falta una red que compare contra lo ESPERADO, no sólo entre VMs.

### Estado de verificación

| familia | qué se ha visto |
|---|---|
| **ESP32-P4** | **la mejor**: `JsonDemo` (con `Core.mod` regenerado corriendo en RISC-V) y `trytest.bp` con la salida idéntica al host |
| **Pico · S3 · STM32** | 🔴 **la stdlib nueva SIN ESTRENAR**. Compilan, pero ninguna la ha ejecutado |

⏭️ **Primer gesto al retomar**: `trytest.bp` y `JsonDemo` en la Pico y en el S3 —los dos
ya tienen salida conocida, o sea que sirven de oráculo— **después de formatear el FS**.

### Lo que queda del tema «lenguaje»

- 🟡 **LSP entre interfaces de módulo** (`appv1lsp`/`appv2`, en `samples/pendientes/`).
  ⚠️ Y la explicación escrita en la ficha **está en duda**: dice que *«falla el dato»*
  porque una `module interface` pura no genera `.mod`, pero la traza del 30-ago imprime
  `extends=com.example.LogApi` — el dato está. Primer gesto: mirar si `implSatisfies`
  recibe ese `extends` o una copia sin él, en vez de dar por buena la explicación.
- 🟡 **`factorial` en f64** y **unificar `sign`/`signF`**: bloqueados por lo mismo, **no se
  puede sobrecargar una intrínseca** (`Intrinsics.REGISTRY` es un `Map` por nombre pelado
  y el call-site compone la clave con `fs.name`, no con el mangleado de `H5.a`).
- 🟡 **Mudar `Exception` fuera de `Core`**, como está `Object` — idea de Eduardo. Ahorraría
  el `import` a 21 de 56 ficheros. **No se hizo a propósito**: mete identidad de clases
  entre módulos en medio de un arreglo de bug, y un `catch` que deja de atrapar falla en
  silencio. Va en su propio paso.

### Lo que enseñó la tarde, y no es del código

**El «no sé» de Eduardo ante una salida rara.** Propuse `trytest.bp` como trámite para
cerrar la verificación; él miró la salida, dijo *«no sé»*, y eso destapó `#459`. Una salida
que no se entiende merece pararse a mirarla **aunque el `exit` sea 0 y el arnés esté verde**.

**Y un método que funcionó**: para decidir *«¿lo he roto yo hoy?»* se usó la
**distribución publicada** (`dist/BasicPlus-5.0-win/`) como oráculo — su compilador, su VM
y su stdlib, todo desde el mismo fat-jar. Contestó en un minuto lo que rebuildeando el
árbol viejo habría costado varios, y descartó la sospecha razonable (y falsa) de que la
tanda del día tuviera la culpa.

## ⏭️ AL RETOMAR (30-ago, tarde) — `U3.21` cerrado y **el P4 al día**; la deuda es ahora del STM32

### La matriz, al cierre de la tarde

| familia | última verificación | qué le falta por ver |
|---|---|---|
| **ESP32-P4** | **30-ago (tarde)**, en placa | al día — ver abajo lo que cubrió |
| **ESP32-S3** | **30-ago**, gesto a gesto | `U3.21` sólo por herencia (comparte `repl_esp32.c` con el P4, que sí se probó) |
| **Pico / Metro** | 29-ago (batería 48/48) | `#453` · el `SAVE` de `U3.17` · `#455` |
| **STM32 Nucleo** | 27-ago (`U3.6`) | `#449` `#450` `#451` `#453` `#455` · `U3.17` · los 6 gestos de la deuda vieja |
| **STM32 Discovery** | ninguna esta serie | lo del Nucleo, y además es la de PANTALLA |

🟢 **El P4 pasó de ser la deuda gorda a estar al día en una tanda.** Y lo primero que
se resolvió no costó ningún gesto: **el `server_name` lo demuestra el propio `INFO`**.
`Micro : esp32p4` sale de `bid->board_name`, y `board_name` y `server_name` son campos
**contiguos de la misma struct**, rellenados en el mismo inicializador de
`p4_board_id.c`; `p4_install_board_id()` corre en `main.c:322` (y `:370` en el camino
TCP) **antes** de `repl_esp32_run()`, que copia el nombre en la línea 1201 antes de
registrar la cintura. No hay ventana. La pregunta se contestó **con lo que ya había**.

Lo que la tanda de Eduardo cubrió, gesto a gesto:

| gesto | qué prueba |
|---|---|
| subir y bajar `Fichas.md` (~300 KB) | **`U3.21`**: >8 KB ⇒ `PUT_BEGIN` + ~37×`PUT_DATA` + `PUT_END`, los tres por el común. Y **`#453`/`#454`** en el mismo viaje: en el S3 un fichero de 120 KB daba timeout antes del arreglo |
| grabar `Stdlib.pack` | 🎯 **el control que hacía falta**: `PACK_BURN_*` es el camino que **sigue pre-leyendo el bulk** en `handle_request`. Si `lo_lee_el_comun` se hubiera colado a `1` de más, esto se corrompe — y en binario no da error, corrompe. Verde ⇒ la bandera separa bien los dos caminos |
| `fib(28)` interp 4410 ms / **AOT 32 ms** | el `.mdn` RISC-V vivo (137×) y `RUN` + resolución de módulo |
| `/lib/Pico.mod ya en FS (contenido idéntico), salto PUT` | el `STAT`+CRC bajo demanda (`#398`) migrado, decidiendo bien |
| el arranque (`heap 27.5 MB + stack 512 KB`) | **`#450`** en su rama por defecto: sin `stack` en el ENV, el reparto de siempre |

⚠️ **Lo que esa tanda NO prueba, y conviene no apuntárselo:** `#455` sólo quedó
verificado en que **no rompe el caso normal** — el `PUT` fue a `/app`, que existe desde
el montaje, así que el `mkdir` nuevo fue un no-op. La capacidad que devuelve (subir a
una carpeta que no existe) **sigue sin ejercitarse**, y no hay test de host que la cubra
(el simulador trae su propio `handle_put`). Es el hueco de `#444`.

⏭️ **Lo que queda de `U3`**: sólo `LIST`. Sigue parado por lo mismo — el ESP32 imprime
un desglose por raíz (`ls: 25 ent en 160 ms | app:11/50ms lib:14/64ms`) que el común no
tiene, y migrarlo tal cual sería **unificar hacia abajo**, que es exactamente el error
que `#455` acaba de demostrar que se comete solo. El camino bueno es subir el desglose
al común, y entonces la Pico y el STM32 lo GANAN. Diseño: el común ya recorre con una
cola desde `/`, así que cronometrar cada entrada de profundidad 1 da el mismo desglose.

## ⏭️ AL RETOMAR (30-ago) — `U3` casi cerrado en ESP32, y **el P4 es la deuda gorda**

### Lo primero: LA MATRIZ DE VERIFICACIÓN

Pregunta de Eduardo al cerrar: *«¿cómo están las otras familias?»*. Medido con
`git log` sobre el código compartido desde la última prueba de cada una:

| familia | última verificación | qué le falta por ver |
|---|---|---|
| **ESP32-S3** | **30-ago**, gesto a gesto | al día |
| **Pico / Metro** | 29-ago (batería 48/48) | `#453` · el `SAVE` de `U3.17` |
| **STM32 Nucleo** | 27-ago (`U3.6`) | `#449` `#450` `#451` `#453` · `U3.17` · los 6 gestos de la deuda vieja |
| **STM32 Discovery** | ninguna esta serie | lo del Nucleo, y además es la de PANTALLA |
| **ESP32-P4** | **25-ago** (`U2` paso 2) | **`U3` ENTERO (12 verbos)** · `server_name` · `#449` `#450` `#451` `#453` |

🔴 **El P4 es el peor caso y conviene entender por qué.** Comparte
`repl_esp32.c` con el S3, así que ha recibido **toda** la migración del 30-ago sin
flashearse. Y —esto es lo importante— **lo único que el verde del S3 NO puede
exonerar es justo lo suyo**: el `server_name`, que es por placa (`bpvm-esp32p4`
contra `bpvm-esp32`). Fue el bug que puse en `U3.13` y quité en `U3.19`, y **sólo
se manifiesta en el P4**.

📌 La regla que sale de ahí, y sirve para toda la serie U: *el código común
verificado en una familia dice algo de las demás; **lo que sale de la cintura,
no**.* Por eso la matriz no se puede rellenar por inferencia.

⏭️ **Orden propuesto y el gesto que prueba cada cosa:**

1. **P4** — conectar el IDE y **mirar el nombre del servidor** (cubre lo único no
   inferible), luego `INFO`, `ls`, subir y bajar un fichero.
2. **Pico** — abrir un fichero grande (prueba `#453`) y `save` desde la consola.
3. **STM32** — los seis gestos de `H13_PRUEBAS_V5_REPASO.md` + abrir un fichero
   grande.

### Lo que se hizo el 30-ago

**`U3` en el ESP32, de 23 verbos propios a 6.** `repl_esp32.c` 1360 → 1242 líneas.
Migrados y **verificados en placa**: `DEL` `STAT` `MKDIR` `GET` `PING` `TIME`
`LOG_DUMP` `LOG_CLEAR` `SAVE` `DF` `INFO` `HELLO`.

Quedan dos cosas, y las dos son decisión de Eduardo más que trabajo:

- 🟡 **`LIST` está PARADO a propósito.** Migrarlo tal cual perdería el desglose por
  raíz que el ESP32 sí imprime (`ls: 25 ent en 160 ms | app:11/50ms lib:14/64ms`) y
  el común no — sería unificar HACIA ABAJO. Lo correcto es subir ese desglose al
  común, y entonces la Pico y el STM32 lo GANAN. Además el común recorre
  directorios (`bpvm_fs_list` con cola desde `/`) y el ESP32 lista plano: ninguno
  de los dos es «el bueno» entero.
- ⏭️ **El grupo `PUT`**, con el terreno ya preparado: `U3.20` subió el `type` por
  encima de la pre-lectura del bulk y dejó la bandera `lo_lee_el_comun` en 0.

**Cuatro fichas nuevas**, tres de ellas encontradas persiguiendo otra cosa:
`#452` (cinco verbos que ningún cliente manda), `#453` (el `GET` abría el fichero
472 veces para 120 KB), `#454` (el timeout medía tiempo total en vez de
inactividad) y `#446` cerrada con `make check-nogui`.

### Lo que conviene llevarse

🔁 **Tres veces el mismo patrón en una sesión: el arreglo existe en un camino y
falta en su gemelo.** `crc32` arreglado y el `GET` no (`#453`); el corte del CRC en
la Pico y no en el ESP32 (`#424`); el troceado en `PUT` y no en `GET` (`#454`).
Ninguno se buscó — los tres salieron de perseguir otra cosa. Merece la pena
buscarlos a propósito: `grep` de la primitiva que se deja de usar.

⚠️ **Y un diagnóstico mío que era falso.** Calculé que 120 KB a 115200 baud son
10,5 s contra un plazo de 10,0 y concluí «no cabe por el cable». La aritmética
estaba bien y la conclusión mal: lo tumbó que Eduardo observara que **un pack de
~580 KB sí se graba**, tardando ~50 s. No miré lo que ya funcionaba antes de
declarar algo imposible.

## ⏭️ AL RETOMAR (29-ago, cierre) — **se sigue por `U3` con el ESP32**

**Decisión de Eduardo al cerrar**: *«cuando retomemos continuamos justo por U3 y vamos
cerrando cosas»*. Con eso **termina la etapa de bugs de V5** y se vuelve a V6.

### Por dónde exactamente

`U3` (el REPL) tiene **STM32 y Pico completos** y verificados en placa: 26 de los 29 verbos
al común (`RUN`/`KILL`/`RESET` se quedan por diseño). **Falta la tercera familia: el
ESP32** — el S3 y la P4 conservan su `repl_esp32.c` de 1344 líneas.

📌 **Y hay un motivo nuevo para que sea lo siguiente.** Eduardo fijó ese día la prioridad:
*«a mí la S3 me importa relativamente, es el paso; el futuro son la P4, la C6 y la S31»*.
Las tres son ESP32 y **las tres comparten `esp32/main/*`**: cada verbo que siga en la copia
privada es un verbo que habrá que replicar tres veces. Es el argumento de V6, aplicado.

⏭️ Y de paso, en el mismo arranque, **saldar la deuda del STM32 desde `U3.6`** (seis
gestos, cinco minutos, en `H13_PRUEBAS_V5_REPASO.md`): lleva desde el 27 esperando.

### El estado de la serie, ya limpio

| | | |
|---|---|---|
| U1 · U2 | — · el transporte | ✅ cerrados |
| **U3** | **el REPL** | 🔵 **STM32 y Pico hechos; falta ESP32** ← *aquí* |
| U4 | la stdlib embebida | 🟡 sin empezar (es un GENERADO: tocar el generador) |
| U5 | la tabla de handles | 🟡 sin fichero propio todavía |
| U6 | la organización de memoria | 🔴 abierta, **sin la urgencia** que tuvo el 29 por la mañana |

Abiertas además: `#446` (host con `GUI=0`, barato y sin placa), `#441` (seis opcodes) y
`#444` (el sistema de pruebas no escala — su diseño ya está escrito en `V7_IDEAS.md`).

### Lo que se cerró en la sesión del 29

**La batería pasó de 40/48 a 48/48.** Cerradas `#440`, `#442`, `#445`, `#447`, `#448`,
`#449`, `#450` y `#451`. La causa de fondo era **una sola**: las tablas de símbolos y de
handles vivían en el margen de `malloc` de la plataforma, separadas del heap, y no se
prestaban nada. Arreglado en dos movimientos —un pool de nombres y la tabla de handles
dentro del bloque de la VM— y verificado en **dos familias** (Pico 2 y S3).

⚠️ **Lo que conviene llevarse, y no es sobre el bug**: de los cinco atascos del día, **cuatro
no eran del proyecto** — tres eran instrumentos de diagnóstico míos mintiendo y uno era una
cifra mía mal calculada. Lo que resolvió las dos veces que hubo atasco de verdad fue
**medir**: contar símbolos, contar handles, `idf.py size-components`, y la tabla de memoria
de Espressif. Nunca razonar. Ver [[contar-los-consumidores-no-leer-el-codigo]] y
[[instrumento-mudo-dudar-de-el]].

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
