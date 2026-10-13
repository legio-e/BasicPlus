# Checklist de publicación — BasicPlus v7.0

Doc de mantenedor (no enlazado desde la portada). El repo público
`legio-e/BasicPlus` **ya existe**; publicar es un `push` + una release.

> **Esto se hace una vez cada varias semanas y no se recuerda.** Por eso cada paso
> dice *qué* se hace, *quién* lo hace y **cómo saber que salió bien**. Si un paso
> no tiene forma de comprobarse, es que está mal escrito.

**Lo que se publica en V7 es UN artefacto**: `BasicPlus-7.0-win.zip` — el paquete
completo (IDE + compilador + stdlib + **los cinco packs** + samples + docs + micro
simulado + **las 8 imágenes de firmware dentro**, en `firmware/`: Pico/Metro,
ESP32-S3, ESP32-C3, ESP32-C5, ESP32-C6, ESP32-P4, Nucleo-U575, Discovery U5G9J —
10 placas).

Cambió respecto a V3, que soltaba 7 binarios sueltos. Y cambió **otra vez el
6-ago**, a media publicación: se habían adjuntado además las 5 imágenes sueltas
«para quien sólo quiere reflashear», y Eduardo lo cortó con el argumento que
zanja el asunto —

> *quien necesita el firmware necesita el IDE, y quien ya tiene el IDE ya tiene
> las imágenes en `firmware/`.*

Las copias sueltas no le servían a nadie y duplicaban el artefacto. **Un fichero,
una suma, una cosa que verificar.**

⚠️ Si algún día se vuelven a soltar, hay que revisar `docs/INSTALAR_FIRMWARE.md`
(las dos versiones): dice de dónde se saca la imagen, y esa frase depende de esta
decisión.

---

## ✅ PRE-FLIGHT DE V7 — F1 (pasado el 12 y el 13-oct; V7 publicada el 13-oct)

La tabla de V6, rellena al pasar el `F1` de V7 con el resultado MEDIDO de cada
casilla. El plan del `F1` y su avance, placa a placa, en `FICHAS.md` (§`F1`).

| comprobación | resultado |
|---|---|
| Suite `lexer-java` · suite `miVM` | 13-oct: **281/281** · **57/57**. ⚠️ No van en `compat.sh` y el paso 1 del `F1` no las tenía: las cazó el repaso de este checklist, antes de publicar |
| `samples_sweep.py` (corpus entero) | 12-oct: **399 OK, 0 FALLO** |
| `doc_frags.py` (todos los fragmentos de docs/) | 12-oct: **0 FALLO** en 25 documentos (314 compilados); **1 024 enlaces internos, 0 rotos** |
| `h13-errores.sh` | 12-oct: **50/50** (19 programas) |
| `compat/compat.sh check` | 12-oct, tras `#692`: **verde** — paridad 149/149 en MOD9, MOD8 y con dos workers; C 102/102; sombra 15/15; estrés del heap, `packs-abi`, stdlib embebida, cápsula V6 (59/59), norma y simulado |
| Guarda de las 4 portadas (tamaño + encabezados) | 13-oct: **4 OK** — 18 268 / 18 824 / 24 313 / 24 021 B; 18 / 18 / 7 / 7 encabezados. Y al leerlas, frases desfasadas (abajo) |
| Las **8** imágenes en `dist/firmware/`, del mismo árbol, `SHA256SUMS.txt` | 12-oct: **8**, del árbol de `2842cd70` (`#691`, el último cambio de firmware); `sha256sum -c`, 8 OK. Las STM32, por `objcopy` desde el `.elf`: los `.bin` de al lado eran de agosto |
| Los **5** packs | 12-oct: el `Stdlib.pack`, igual byte a byte a los 30 módulos de la stdlib (`stdlib_pack.sh`); los tres de la GUI, con la forma de `#623`; el `SQLite.pack`, reconstruido con el compilador final (cambian su fecha y su comprobación) |
| `tanda.py` en las placas | 12-oct: **las diez**, cada una grabada desde el ZIP con la imagen final y sus packs según `INSTALAR`; los mismos veredictos que su base, comparados placa a placa (`compat/informes/f1_v7_*.md`); el `imageCrc` de cada imagen, en `FICHAS` §`F1`. Y a mano: `#681`, `#682`, `#686`, `#691`, el Stop con la pantalla limpia, los ADC 4–7 de la Metro, la memoria de cada placa y SQLite en la Metro y en las P4 |
| Micro simulado con LVGL y SQLite | 13-oct: `make sim LVGL=1 SQLITE=1` — **2 024** `lv_`, **305** `sqlite3_`, `SDL2.dll`; el mismo que el del ZIP probado salvo el sello de la hora de compilación (9 bytes) |
| ZIP montado y verificado por dentro | 13-oct: `BasicPlus-7.0-win.zip`, 28 MB, sha256 **`8f843444…`**, 718 ficheros. Frente al que probó Eduardo (`8c54f18a…`), cuatro distintos: las tres correcciones de la ayuda (abajo) y el sello del simulado |
| Desplegado en carpeta limpia y probado allí | 12-oct, Eduardo (el paso 5 del `F1`): el IDE del ZIP, *«todas las pruebas OK»* salvo el Stop en la P4 → `#692`, arreglado; 13-oct, la prueba final del ZIP: *«todo OK»* |

🩸 **Lo que se le escapó al `F1` y cazó este checklist** (13-oct), ya con el ZIP probado:
- **Las suites de Maven**: el paso 1 del `F1` copió la lista de guardas de `compat.sh`, que no las corre. Salieron
  verdes; ahora la casilla lo dice (§1).
- **«Nueve placas» en cinco frases**: los dos README, la de arriba de las dos `index.html` y la de la ronda en las
  notas. El `F1` sumó la décima placa (el kit P4 EV) y corrigió `INSTALAR`, el párrafo de las placas de la portada y
  la tabla de las notas, pero no esas cinco. Y en las notas, dos números de antes de `#685`: 148 casos de paridad
  (son 149) y 18 programas de errores (19, con 50 comprobaciones). Tres de esos ficheros viajan en el ZIP: se rehízo,
  y se comparó con el probado.
- **El simulado, en el sabor de la red**: la red lo pide sin LVGL y así se quedó tras el último ZIP; el empaquetador
  paró (*«bpvm-sim.exe esta construido SIN LVGL»*), como debe. Ahora la casilla del simulado dice el orden (§1).

## ✅ PRE-FLIGHT DE V5 — pasado el 22-ago

Queda registrado porque el checklist pide cada casilla y las tres últimas costaron
hallazgos que no estaban previstos.

| comprobación | resultado |
|---|---|
| Suite `lexer-java` | ✅ **104 tests, 0 fallos** |
| Suite `miVM` | ✅ **34 tests, 0 fallos** |
| Batería de samples | ✅ **51 corren · 19 compilan · 0 no compilan · 0 fallan** |
| Los tres demos de BD en el host | ✅ `SqlDemo`, `DaoDemo`, `GenDemo` con `status=OK` |
| Las 5 imágenes, del mismo árbol y selladas | ✅ `SHA256SUMS.txt` verifica |
| Micro simulado **con LVGL** | ✅ y con **reja en `montar-zip.sh`** para que no se repita |
| ZIP montado y verificado por dentro | ✅ **24.403.893 B** |
| **Desplegado en carpeta limpia y probado allí** | ✅ **Eduardo, 22-ago**: el IDE del ZIP funciona *«tanto con placa como con el simulador; la ayuda también»* |

🩸 **Las suites estaban ROJAS y nadie lo sabía** — 3 fallos, ninguno del producto: dos
tests de miVM contaban el GC por `stdout` cuando el diagnóstico se movió a `stderr` (para
no romper la paridad byte a byte, que es el invariante), y uno de `lexer-java` llevaba
rojo desde el 18-ago porque su fixture describía el lenguaje anterior a `#450`.
🩸 **Y el simulador se publicaba SIN gráficos.** El checklist ya decía *«micro simulado con
LVGL»* y aun así se saltó en el ZIP del 20-ago. Ahora el empaquetado lo comprueba: un
humano puede saltarse una casilla, el script no.
📌 **Las dos cosas salieron de recorrer el checklist entero en vez de fiarse.** Y la
última —desplegar en limpio— es la que encontró los dos fallos del simulador. Ninguno se
ve desde el repo.

🩸 **Y aun así v5.0 salió con dos cosas rotas que ninguna casilla miraba** (vistas el
12-sep, al documentar V6): `README.es.md` **destruido** —7,8 MB, un párrafo repetido
11.983 veces, cero encabezados, así en el tag y en GitHub (#497)— y **6 de los 326
samples** publicados que no compilaban con el compilador congelado (#498; la casilla
decía «0 no compilan» porque medía una LISTA de 70, no el corpus). De ahí las cuatro
casillas nuevas de abajo: los dos barridos, la guarda de las portadas y la de las
siete imágenes. **La herramienta, no el artefacto**: lo que se comprobó a mano una
vez no se vuelve a comprobar; lo que comprueba un script, sí.

## ✅ PRE-FLIGHT DE V6 — F1 (pasado el 12-sep; V6 publicada esa noche)

La misma tabla que la de V5, rellena al pasar F1 con el resultado MEDIDO de
cada casilla (número, no «OK»):

| comprobación | resultado |
|---|---|
| Suite `lexer-java` · suite `miVM` | 12-sep: **110/110** · **39/39**. ⚠️ La del frontend estaba ROJA desde el 30-ago (`#458`): un fixture sin `import Core`. Trece días sin correrla |
| `samples_sweep.py` (corpus entero) | 12-sep: **348 OK, 0 FALLO** (2 avisos de import omitido, ambos a propósito y anotados) |
| `doc_frags.py` (todos los fragmentos de docs/) | 12-sep: **152 OK, 119 trozo, 2 error-esperado, 0 FALLO** (540 bloques, 25 docs) |
| `compat/compat.sh check` | 12-sep: **59 PASS, 0 FAIL, 0 SKIP**, ninguna línea `!! stderr:` |
| Guarda de las 4 portadas (tamaño + encabezados) | 12-sep: **4 OK** — 14.092 / 14.515 / 20.731 / 20.580 B; 18 / 18 / 7 / 7 encabezados |
| Las **7** imágenes en `dist/firmware/`, del mismo árbol, `SHA256SUMS.txt` | 12-sep: **7**, selladas; C6/P4/DK2 del árbol `97d61d7e~` (tras `#505`), S3/C3/Pico/Nucleo byte-idénticas a las probadas salvo sello de hora; todas con `I2c.mod` dentro (el 11-sep se regeneró sin él) |
| `tanda.py` en una placa por familia (Pico/Metro · S3 · C3/C6 · P4 · Nucleo/DK2) | 12-sep: **las nueve placas**, cada una regrabada desde `dist` — `compat/informes/tanda_f1.md`. Además `SqlDemo` == host en los dos P4 (y `SqlDemoSd` en la tarjeta), `Bench` nativo 32 ms en el P4, `#379` 8/8 |
| Micro simulado con LVGL (y decidido si con SQLite) | 12-sep: `make sim LVGL=1 SQLITE=1` — **con SQLite** (`SqlDemo` == host en el sim); lv_ 2026 símbolos, sqlite3_ 305; cerrar la ventana apaga el sim |
| ZIP montado y verificado por dentro | 12-sep: `BasicPlus-6.0-win.zip`, 25 MB; el empaquetador cazó 5 drivers MOD6 y un `Stdlib.pack` MOD6; cuatro verificadores independientes (samples 345/345, docs, firmware, sim) → 4 medios corregidos |
| Desplegado en carpeta limpia y probado allí | 12-sep: `C:\tmp\bp6`: sellos 7/7, `ChartDemo`/`FontLoadDemo` capturados, `Bench`, `SqlDemo` == host, `GuiEvSpike --no-screen` (`#505`); **IDE probado por Eduardo: OK** |

## ✅ Al publicar V7 — lo que quedó escrito para ese día

- ✅ **`--formato=8`: se queda como está** (Eduardo, 13-oct: *«la salida normal es MOD9 […] así que también puedan
  generar MOD8 no es problema»*). El compilador emite MOD9 sin flag desde el 17-sep (`#523` V11), y `--formato=8`
  sigue como salida de emergencia, como ya dice la referencia (ES y EN: la tabla de opciones del compilador y
  §17.1); las notas no lo nombran. Lo usan el frente `check-mod8` de `compat.sh` (la foto «antes»: el corpus en MOD8
  da el stdout de MOD9), las filas MOD8 del bloque V de `compat/mezcla.py` y los dos tests de marcadores del
  frontend. Una diferencia con MOD9, por si alguien lo usa (vista el 7-oct, con `#664`): en MOD8 la ranura por
  nombre (`Gui.__guiSlotOf`, la que usa `bindMember`) sale de los MARCADORES `Clase#metodo#slot`, que sólo exporta
  una clase `public` —en MOD9, de la tabla de la clase, sea cual sea—: con una clase no pública da -1 y el manejador
  se ignora en silencio. Las ventanas de Forms son `public class`.
- ℹ️ `montar-zip.sh` ya habla MOD9 (`MOD_ACTUAL`, y los formatos viejos que se buscan dentro de los packs en
  `MOD_VIEJOS`): lo arregló el cierre de H2 (24-sep) — decía MOD7 y habría parado el paquete. Al subir de formato,
  las dos líneas.

## 1 · Pre-flight — que lo que se publica sea lo que se probó

Estos pasos se hicieron el 6-ago-2026 (H13 de V4), el 22-ago (V5), el 12-sep (V6) y el 12 y 13-oct (V7). Se dejan
escritos porque la próxima vez hay que repetirlos, y porque **cada uno tapa una trampa que ya nos mordió al menos una
vez**.

- [ ] **Suites verdes**: `mvn -f lexer-java/pom.xml test` y `mvn -f miVM/pom.xml test`.
      ⚠️ El 22-ago estaban ROJAS sin que nadie lo supiera (ver arriba): mirar el
      número de fallos, no el color del último build que se recuerda. Con `-q`, Maven
      no lo dice: se cuenta en `target/surefire-reports/TEST-*.xml`.
      ⚠️ **No van en `compat.sh check`**: hay que correrlas aparte. En V7 el `F1` se
      las saltó, y las cazó este checklist (13-oct).
- [ ] **Paridad dual-VM** en host: `bash compat/compat.sh check`. Debe decir cuántos
      PASS y **ninguna línea `!! stderr:`**: desde el 11-sep el arnés ya no tira el
      stderr (#494: el GC de miVM descarrilaba dos meses con 57 PASS en verde).
      Desde el 28-sep corre el corpus también con **dos workers** en las dos VMs
      (`check-smp`, #599): si sólo esa pasada sale roja, el fallo es del modo de
      dos núcleos, y lo que no vería nadie con un worker.
- [ ] **Barrido de TODOS los samples**: `python bpgenvm-c/tools/samples_sweep.py`
      → código de salida **0** y `N OK, 0 FALLO`. Compila cada `samples/**/*.bp`
      con el frontend actual, `bpstdlib/*.mod` y `packs/*.pack` al lado (la misma
      receta que `compat.sh`); salta `errores/` (fallan a propósito: los vigila
      `scripts/h13-errores.sh`, córrelo también: `N comprobaciones OK · 0 mal`) y
      `holes/`, e informa aparte `pendientes/`. Un `AVISO:` de import omitido no es
      fallo pero léelo.
      ⚠️ No sustituye a `scripts/h13-lista.sh`: aquél además EJECUTA su lista.
- [ ] **Ningún fragmento de la documentación falla**:
      `python bpgenvm-c/tools/doc_frags.py` → código de salida **0** y `0 FALLO`.
      Extrae los `<pre>` de `docs/*.html` y `docs/en/*.html` y los ```` ```basic ````
      de los .md de usuario, envuelve los trozos sueltos en `module/main` (con
      `import Core` y el import de cada módulo de la stdlib que nombren) y los
      compila uno a uno. Un trozo cuyos ÚNICOS errores son nombres sin cualificar
      que no declara (`edad`, `scr`, `db`: el contexto que el lector ya tiene) sale
      como **TROZO**, no como fallo, con la lista de lo que le falta — **léela**: un
      `Excepcion` por `Exception` saldría ahí y no en rojo. Todo lo demás (un
      método que no existe, un `Gui.Foo`, un error de sintaxis, un módulo entero
      que no compila) es FALLO. Lo que a propósito no es un programa se marca en
      el HTML: `<pre data-bp="skip">` (no compilar), `data-bp="error"` (DEBE fallar:
      ejemplo de error del compilador), `data-bp="sigue"` (continúa el trozo
      anterior). Un fragmento con `...` es un ESQUEMA y no cuenta. **Un ejemplo que
      debería compilar y no compila se arregla, no se marca.** `--solo docs/gui.html`
      para uno; `-v` enseña el andamio y los errores de cada trozo. 12-sep: 152 OK,
      119 trozo, 2 error-esperado, 0 FALLO; y con tres roturas inyectadas, tres FALLO.
      12-oct: 0 FALLO en 25 documentos (314 compilados), y los enlaces internos.
- [ ] **Stdlib canónica**: si tocaste `bpstdlib/*.bp`, recompílala y luego
      `bash bpgenvm-c/scripts/regen_all_mods.sh` para resincronizar los blobs
      embebidos de las familias.
      ⚠️ *Trampa*: los blobs de stdlib son **generados**. Se tocan regenerando,
      nunca a mano.
- [ ] **Las 8 imágenes, del MISMO árbol**, y en una sola tanda:
  - RP2350: `ninja -C bpgenvm-c/pico/build` → `bpvm_pico.uf2` (Pico 2 y Metro:
    una imagen, la variante se detecta en la placa)
  - ESP32: `idf.py build` en `bpgenvm-c/esp32` (la S3), `esp32c3`, `esp32c5`,
    `esp32c6` y `esp32p4`. El ESP-IDF v6.0.1 está en `C:\esp\v6.0.1\esp-idf`
    (en PowerShell, `& 'C:\esp\v6.0.1\esp-idf\export.ps1'` antes)
  - STM32 (las dos): CubeIDE headless —
    `stm32cubeidec --launcher.suppressErrors -nosplash -consoleLog -application
    org.eclipse.cdt.managedbuilder.core.headlessbuild -data <ws-temp>
    -importAll <dir-del-.project> -cleanBuild "<Proyecto>/Debug"`
    ⚠️ La configuración se llama `Debug` pero **es la de publicar**: es la única de
    los dos proyectos y compila a `-Os`. Se queda con ese nombre (Eduardo, 2-oct,
    H10): lo usan esta receta, `tanda.py` y `censo_reparto.py`, y renombrarla
    movería sus rutas sin cambiar la imagen.
- [ ] **Regenerar `dist/firmware/` a partir de esos builds** — ⚠️ **la trampa más
      cara del 6-ago (hallazgo 39)**: `dist/firmware` es lo que copia el
      empaquetador, y llevaba imágenes de **tres días antes** sin que nada lo
      dijera.
  - Pico: copia directa del `.uf2`.
  - STM32: **`arm-none-eabi-objcopy -O binary` desde el `.elf`**
    (`bpgenvm-c/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.elf` →
    `bpvm_stm32_nucleo.bin`; `Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf`
    → `bpvm_stm32_dk2.bin`) — ⚠️ el `cleanBuild` headless regenera el `.elf`
    **pero NO el `.bin`**; si copias el `.bin` que hay al lado, publicas el build
    anterior (en V7, los de al lado eran de agosto).
  - ESP32: en el `build/` de cada una,
    `esptool --chip <chip> merge-bin -o <dist>\bpvm_<x>_merged.bin '@flash_args'`
    (`esp32s3` → `bpvm_esp32_merged.bin`; `esp32c3`, `esp32c5`, `esp32c6` y
    `esp32p4`, con su nombre). Los offsets salen de `flash_args`: S3, C3 y C6
    `0x0/0x8000/0x10000` @80m · C5 `0x2000/0x8000/0x10000` @80m · P4
    `0x2000/0x8000/0x10000` @40m.
    ⚠️ **`idf.py merge-bin` escribe `build/merged-binary.bin`**, no
    `bpvm_esp32*_merged.bin`, que es el nombre que `INSTALAR_FIRMWARE.md` le da
    al usuario. O sea que el atajo `idf.py merge-bin` deja **intacta la imagen
    con el nombre bueno** — y ahí sigue la vieja, con su fecha vieja, sin que
    nada avise. Medido el 30-ago: al reconstruir el P4, la `*_merged.bin` que
    había al lado era **del 4-ago**. Pásale `-o` el nombre bueno, y comprueba la
    FECHA del fichero, no la salida del comando.
  - **Contar OCHO** ficheros de imagen en `dist/firmware/`, con la fecha de HOY:
    `ls -l dist/firmware/*.uf2 dist/firmware/*.bin | wc -l` → **8**. El
    empaquetador copia `*.uf2` y `*.bin` sin contar: si falta una, el ZIP sale con
    siete y nadie avisa. Y `dist/firmware/README.md` al día.
  - Regenerar `dist/firmware/SHA256SUMS.txt` (`sha256sum *.uf2 *.bin`) y
    comprobarlo: `sha256sum -c SHA256SUMS.txt` → **8 OK**.
- [ ] **Los cinco packs, de su compilación** (`packs/`): el `Stdlib.pack`, igual byte
      a byte a `bpstdlib/*.mod` (`stdlib_pack.sh`); los de la GUI y el de SQLite, de
      sus fuentes y con el compilador de la release. La ABI de sus natives la vigila
      la red (`packs-abi`), y el empaquetador para si un pack lleva un `.mod` de un
      formato viejo (`MOD_VIEJOS`).
- [ ] **BpIde**: ⚠️ **con el IDE CERRADO**. `mvn install` en `miVM` y en
      `lexer-java`, luego `mvn package` en `BpIde`. La versión sale del pom
      (`BpIde-7.0.jar`; el `<version>` del pom es la versión del producto en curso).
- [ ] **Micro simulado con LVGL y SQLite**: `cd bpgenvm-c && make sim LVGL=1 SQLITE=1`
      (con SQLite desde V6: los demos de BD corren sin placa).
      ⚠️ *Trampa (hallazgo 37)*: **sin `LVGL=1` el simulador ejecuta pero no
      pinta**, y el fallo es mudo. Comprobación: `strings build/bpvm-sim.exe |
      grep -c "^lv_"` debe dar **miles** (V7: 2 024), y `grep -c sqlite3_`,
      **centenares** (V7: 305).
      ⚠️ **La red deja el simulado SIN LVGL**: `compat.sh` lo pide en su sabor
      (`gui1-lvgl0`). El orden: la red → `make sim LVGL=1 SQLITE=1` → montar el ZIP
      → `make` y `make sim`, que devuelven el host y el simulado al sabor de la red.
      Si se monta sin rehacerlo, el empaquetador para (13-oct).
- [ ] **Las placas con `tanda.py`** — la prueba de que la imagen que se publica
      CORRE, no solo que compila: con la placa conectada,
      `python bpgenvm-c/tools/tanda.py --lista bpgenvm-c/tools/tanda_v7.json
      --puertos COMx --informe compat/informes/f1_v7_<placa>.md`, cada una con el
      firmware del ZIP recién grabado y sus packs según `INSTALAR_FIRMWARE.md`. En
      V6, una por familia; en V7, **las diez**. Lo que no sea IDENTICO/NO-PETA lleva
      su evidencia, y los veredictos se comparan con los de la base **placa a placa**:
      el 12-oct, un comparador que casaba por el nombre de la prueba mezcló las placas
      de un informe de varias y vio en la DK2 21 diferencias que no había. El sello de la imagen,
      el `imageCrc` del `HELLO`: el de la fecha no la identifica. ⚠️ Lo que cazó la
      prueba de fuego de T1 (11-sep): la Metro tenía firmware del 9-sep — el sello lo
      dijo, el ojo no.
- [ ] **Montar el paquete**: `bash scripts/montar-zip.sh`.
      ⚠️ **Con su carpeta libre**: el script empieza borrando `dist/BasicPlus-<v>-win`,
      y si un proceso la tiene de carpeta de trabajo (el IDE lanzado desde allí, o la
      ventana `cmd` que lo lanzó), se lleva el contenido y no puede quitar la carpeta
      (12-oct). Antes: ningún Java en marcha, y la carpeta se deja renombrar (`mv` y
      vuelta).
      Falla a propósito si: aparece una subcarpeta de `samples/` sin decidir
      (hallazgo 35) · hay un `.mod` sin su `.bp` en `bpstdlib/` · la ayuda tiene
      enlaces rotos · el paquete no compila con su propia stdlib · **algún
      fichero de proyecto (`.bpbuild`/`.bpproject`) no abre** (hallazgo 42) · el
      simulado no lleva LVGL · un pack lleva un `.mod` viejo.
      ℹ️ El de los proyectos mira **sólo la sintaxis**: que al proyecto le falte el
      `.mod` por compilar es normal en un paquete recién montado y NO es fallo.
      📌 **Si se rehace después de probarlo**, se compara con el probado: los dos
      descomprimidos y `diff -rq`, y cada diferencia, explicada (13-oct: 718
      ficheros, 4 distintos).
- [ ] **Desplegar en carpeta LIMPIA y probar allí** — no sobre una instalación
      vieja, que es donde se esconden los ficheros que ya no se generan.
      La puerta final de H13 fueron 4 pruebas: `reset+MemInfo+paralleltest_sugar`
      en placa · `ChartDemo` en el emulador · `FontLoadDemo` en el emulador ·
      `Bench` en placa **y** en el emulador. En V7, el IDE del ZIP, a mano, por
      Eduardo (el paso 5 del `F1`).

## 2 · Repositorio

- [ ] `git status` limpio de lo que no debe subir. ⚠️ Ojo con `bpgenvm-c/pico/_deps/`
      y `build*/`: son salida de build y un `git add -A` los arrastra.
- [ ] `git push origin main`.

## 3 · Las portadas — que son DOS, y no se enteran la una de la otra

⚠️ **Trampa del 6-ago**: «la portada sale en V3» y no estaba en `docs/`.
El proyecto tiene **dos portadas distintas**, y hay que tocar las dos:

| dónde | fichero | cuándo se ve |
|---|---|---|
| página del **repo** (github.com/legio-e/BasicPlus) | `README.md` + `README.es.md` | al instante con el `push` |
| sitio de **Pages** (legio-e.github.io/BasicPlus) | `docs/index.html` + `docs/en/index.html` | tras construir Pages |

Y la de Pages viaja **además** dentro del ZIP: es la documentación que abre
el IDE. Si cambias `docs/`, **hay que rehacer el ZIP**; el `README.md` no
viaja, así que ése no obliga.

⚠️ **Por eso el número de versión de las portadas se sube ANTES de montar el
ZIP.** El 22-ago se hizo al revés: las cuatro portadas seguían en `v4.0`, se
corrigieron después de que Eduardo hubiera probado el ZIP, y hubo que rehacerlo
y volver a verificarlo. Salió barato porque la lista de ficheros quedó idéntica
—las sustituciones conservaban la longitud— pero es suerte, no método.

⚠️ Los volúmenes en **inglés** se quedan atrás sin que nadie lo note:
`docs/en/index.html` llegó a estar **dos versiones** por detrás. Búscalo con
`grep -rn -i "\bv6\b" docs/ README*.md` (la versión anterior) y decide caso por
caso qué es historia legítima y qué es texto caducado.

⚠️ **Y los números**: las placas probadas, los casos de la red, los programas de
error. En V7 el `F1` sumó una placa y la red un caso después de escribirlos, y cinco
frases y dos números se quedaron atrás (13-oct; arriba). Se buscan al final, en las
portadas, en las notas y en `INSTALAR`: `grep -rn -i -E "nueve|nine|<N> casos"`.

🩸 **Y una portada puede estar DESTRUIDA y pasar todas las casillas de arriba.**
`README.es.md` se publicó en v5.0 con 7,8 MB: el banner «V5 — los datos» repetido
11.983 veces, sin un solo `#`, sin el blink ni los comandos (#497; la herramienta
que escribió las cuatro portadas el 22-ago lo dejó así y el commit entró). Nadie
abrió el fichero: se miró que «anunciara V5», y lo anunciaba. Por eso:

- [ ] **Guarda de las cuatro portadas** — tamaño y encabezados, ANTES del push y
      antes de montar el ZIP:
      ```sh
      for f in README.md README.es.md docs/index.html docs/en/index.html; do
        b=$(wc -c < "$f"); h=$(grep -c -E '^#{1,3} |<h[1-3][ >]' "$f")
        case "$f" in *.md) min=10 ;; *) min=5 ;; esac
        printf '%-22s %8d B  %3d encabezados  %s\n' "$f" "$b" "$h" \
               "$( [ "$b" -lt 102400 ] && [ "$h" -ge "$min" ] && echo OK || echo MAL )"
      done
      ```
      Las cuatro deben decir **OK**: menos de 100 KB y al menos 10 encabezados
      (`#`) en los README, 5 (`<h1>`–`<h3>`) en los `index.html` (hoy: 18/18 y 7/7).
      El README destruido medía 7.825.108 B con 0 encabezados; el sano, 18 KB con 18.
      Y **abrirlas las cuatro** y leer la primera pantalla: la guarda caza el
      desastre, no el texto caducado.
- [ ] Settings → Pages → Source: rama `main`, carpeta **`/docs`** (ya configurado).
- [ ] `docs/.nojekyll` presente.
- [ ] **Comprobar que el build de Pages terminó BIEN** — no basta con el push:
      `gh api repos/legio-e/BasicPlus/pages/builds/latest --jq .status`
      ⚠️ Dos despliegues seguidos se pisan («*due to in progress deployment*»)
      y el segundo **falla en silencio**: la web sigue sirviendo la anterior.
      Se relanza con `gh api -X POST repos/legio-e/BasicPlus/pages/builds`.
- [ ] Abrir la URL y comprobar: portada, que **carga la captura**
      (`img/guicolordemo.png`), y que los volúmenes (`manual`, `referencia`,
      `guia-ide`, `gui`, `basedatos`, `creditos`) abren en ES y EN; y
      `bp-desde-dentro`, que sólo está en español (la portada inglesa enlaza ése:
      `/en/bp-desde-dentro.html` da 404 y no es un fallo).
- [ ] ℹ️ `web/cheatsheet.html` **no** se publica automáticamente: vive fuera de
      `docs/` a propósito (para que no viaje en el ZIP). Si lo quieres en la web,
      hay que copiarlo o enlazarlo a mano.

## 4 · Release v7.0

- [ ] Tag `v7.0` sobre el commit publicado, y `git push origin v7.0`.
- [ ] Cuerpo de la release: **bilingüe, español y luego inglés**, con el molde de la
      anterior (`dist/release-v6.0.md`; el de V7, `dist/release-v7.0.md`): intro ·
      Lo nuevo · **Al actualizar** · Descarga (tabla de las **ocho** imágenes +
      `sha256` del ZIP) · Documentación · Limitaciones. El título, «BasicPlus v7.0»:
      sin lema (Eduardo, 13-oct; las anteriores lo llevaban).
      ⚠️ **No es un copia-pega de `RELEASES.md`**: aquel es sólo español y va por
      temas, no por release. Esto se escribió mal en el checklist y se descubrió el
      22-ago comparando con lo que V4 publicó de verdad.
      ℹ️ El enlace «English below ↓» de la primera línea no salta: GitHub no pone
      anclas a los encabezados de una release (visto en v7.0; en v6.0, igual).
- [ ] `gh release create v7.0 dist/BasicPlus-7.0-win.zip --repo legio-e/BasicPlus
      --title "BasicPlus v7.0" --notes-file dist/release-v7.0.md` — **el ZIP y nada
      más** (ver arriba: las imágenes van dentro).
- [ ] Comprobar desde la propia release: descargar el ZIP y verificar el
      `sha256` contra el que anotaste al montarlo.

## Limitaciones conocidas que van en las notas (no bloquean)

Aquí había una lista (la de V4, con sus «→ V5»). Se quita: **se desincronizaba**,
que es lo que pasa con toda lista que vive en dos sitios. La fuente es
`docs/RELEASES.md` («Lo que todavía no» de la versión) y `docs/PENDIENTES.md`; el
cuerpo de la release se escribe desde ahí. Lo que sí es de este checklist:
comprobar que esas dos hablan de la MISMA versión que las portadas antes del push
(`grep -n "todavía no" docs/RELEASES.md`).
