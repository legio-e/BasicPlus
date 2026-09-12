# Checklist de publicación — BasicPlus v6.0

Doc de mantenedor (no enlazado desde la portada). El repo público
`legio-e/BasicPlus` **ya existe**; publicar es un `push` + una release.

> **Esto se hace una vez cada varias semanas y no se recuerda.** Por eso cada paso
> dice *qué* se hace, *quién* lo hace y **cómo saber que salió bien**. Si un paso
> no tiene forma de comprobarse, es que está mal escrito.

**Lo que se publica en V6 es UN artefacto**: `BasicPlus-6.0-win.zip` — el paquete
completo (IDE + compilador + stdlib + packs + samples + docs + micro simulado +
**las 7 imágenes de firmware dentro**, en `firmware/`: Pico/Metro, ESP32-S3,
ESP32-C3, ESP32-C6, ESP32-P4, Nucleo-U575, Discovery U5G9J — 9 placas).

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

## ⏳ PRE-FLIGHT DE V6 — F1 (por pasar)

La misma tabla que la de V5, para rellenar al pasar F1 con el resultado MEDIDO de
cada casilla (número, no «OK»):

| comprobación | resultado |
|---|---|
| Suite `lexer-java` · suite `miVM` | 12-sep: **110/110** · **39/39**. ⚠️ La del frontend estaba ROJA desde el 30-ago (`#458`): un fixture sin `import Core`. Trece días sin correrla |
| `samples_sweep.py` (corpus entero) | 12-sep: **348 OK, 0 FALLO** (2 avisos de import omitido, ambos a propósito y anotados) |
| `doc_frags.py` (todos los fragmentos de docs/) | 12-sep: **152 OK, 119 trozo, 2 error-esperado, 0 FALLO** (540 bloques, 25 docs) |
| `compat/compat.sh check` | 12-sep: **59 PASS, 0 FAIL, 0 SKIP**, ninguna línea `!! stderr:` |
| Guarda de las 4 portadas (tamaño + encabezados) | 12-sep: **4 OK** — 14.092 / 14.515 / 20.731 / 20.580 B; 18 / 18 / 7 / 7 encabezados |
| Las **7** imágenes en `dist/firmware/`, del mismo árbol, `SHA256SUMS.txt` | |
| `tanda.py` en una placa por familia (Pico/Metro · S3 · C3/C6 · P4 · Nucleo/DK2) | |
| Micro simulado con LVGL (y decidido si con SQLite) | |
| ZIP montado y verificado por dentro | |
| Desplegado en carpeta limpia y probado allí | |

## 1 · Pre-flight — que lo que se publica sea lo que se probó

Estos pasos **ya se hicieron el 6-ago-2026** (H13) y el 22-ago (V5). Se dejan
escritos porque la próxima vez hay que repetirlos, y porque **cada uno tapa una
trampa que ya nos mordió al menos una vez**.

- [ ] **Suites verdes**: `mvn -f lexer-java/pom.xml test` y `mvn -f miVM/pom.xml test`.
      ⚠️ El 22-ago estaban ROJAS sin que nadie lo supiera (ver arriba): mirar el
      número de fallos, no el color del último build que se recuerda.
- [ ] **Paridad dual-VM** en host: `bash compat/compat.sh check`. Debe decir cuántos
      PASS y **ninguna línea `!! stderr:`**: desde el 11-sep el arnés ya no tira el
      stderr (#494: el GC de miVM descarrilaba dos meses con 57 PASS en verde).
- [ ] **Barrido de TODOS los samples**: `python bpgenvm-c/tools/samples_sweep.py`
      → código de salida **0** y `N OK, 0 FALLO`. Compila cada `samples/**/*.bp`
      con el frontend actual, `bpstdlib/*.mod` y `packs/*.pack` al lado (la misma
      receta que `compat.sh`); salta `errores/` (fallan a propósito: los vigila
      `scripts/h13-errores.sh`, córrelo también) y `holes/`, e informa aparte
      `pendientes/`. Un `AVISO:` de import omitido no es fallo pero léelo.
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
- [ ] **Stdlib canónica**: si tocaste `bpstdlib/*.bp`, recompílala y luego
      `bash bpgenvm-c/scripts/regen_all_mods.sh` para resincronizar los blobs
      embebidos de las familias.
      ⚠️ *Trampa*: los blobs de stdlib son **generados**. Se tocan regenerando,
      nunca a mano.
- [ ] **Las 7 imágenes, del MISMO árbol**, y en una sola tanda:
  - RP2350: `ninja -C bpgenvm-c/pico/build` → `bpvm_pico.uf2` (Pico 2 y Metro:
    una imagen, la variante se detecta en la placa)
  - ESP32-S3: `cd bpgenvm-c/esp32 && idf.py build`
  - ESP32-C3 y ESP32-C6: `cd bpgenvm-c/esp32c3 && idf.py build` · ídem `esp32c6`
    (nuevas en V6; el ESP-IDF está en `C:\esp6.0.1`)
  - ESP32-P4: `cd bpgenvm-c/esp32p4 && idf.py build`
  - STM32 (las dos): CubeIDE headless —
    `stm32cubeidec --launcher.suppressErrors -nosplash -consoleLog -application
    org.eclipse.cdt.managedbuilder.core.headlessbuild -data <ws-temp>
    -importAll <dir-del-.project> -cleanBuild "<Proyecto>/Debug"`
- [ ] **Regenerar `dist/firmware/` a partir de esos builds** — ⚠️ **la trampa más
      cara del 6-ago (hallazgo 39)**: `dist/firmware` es lo que copia el
      empaquetador, y llevaba imágenes de **tres días antes** sin que nada lo
      dijera.
  - Pico: copia directa del `.uf2`.
  - STM32: **`arm-none-eabi-objcopy -O binary` desde el `.elf`** —
    ⚠️ el `cleanBuild` headless regenera el `.elf` **pero NO el `.bin`**; si
    copias el `.bin` que hay al lado, publicas el build anterior.
  - ESP32: `esptool merge-bin` con los offsets de `build/flash_args`
    (S3, C3 y C6 `0x0/0x8000/0x10000` @80m · P4 `0x2000/0x8000/0x10000` @40m).
    ⚠️ **`idf.py merge-bin` escribe `build/merged-binary.bin`**, no
    `bpvm_esp32*_merged.bin`, que es el nombre que `INSTALAR_FIRMWARE.md` le da
    al usuario. O sea que el atajo `idf.py merge-bin` deja **intacta la imagen
    con el nombre bueno** — y ahí sigue la vieja, con su fecha vieja, sin que
    nada avise. Medido el 30-ago: al reconstruir el P4, la `*_merged.bin` que
    había al lado era **del 4-ago**. Renómbrala o pásale `-o` el nombre bueno, y
    comprueba la FECHA del fichero, no la salida del comando.
  - **Contar SIETE** ficheros de imagen en `dist/firmware/` (hoy hay cinco: faltan
    `bpvm_esp32c3_merged.bin` y `bpvm_esp32c6_merged.bin`), con la fecha de HOY:
    `ls -l dist/firmware/*.uf2 dist/firmware/*.bin | wc -l` → **7**. El empaquetador
    copia `*.uf2` y `*.bin` sin contar: si falta una, el ZIP sale con seis y nadie
    avisa. Y actualizar `dist/firmware/README.md` (sigue diciendo «las 5 imágenes»).
  - Regenerar `dist/firmware/SHA256SUMS.txt` (con las siete).
- [ ] **BpIde**: ⚠️ **con el IDE CERRADO**. `mvn install` en `miVM` y en
      `lexer-java`, luego `mvn package` en `BpIde`. La versión sale del pom
      (`BpIde-6.0.jar`; el `<version>` del pom es la versión del producto en curso).
- [ ] **Micro simulado con LVGL**: `cd bpgenvm-c && make sim LVGL=1`.
      ⚠️ *Trampa (hallazgo 37)*: **sin `LVGL=1` el simulador ejecuta pero no
      pinta**, y el fallo es mudo. Comprobación: `strings build/bpvm-sim.exe |
      grep -c "^lv_"` debe dar **miles**, no 0.
      ❓ **DECISIÓN PENDIENTE (Eduardo): ¿el simulador del dist lleva también
      `SQLITE=1`?** Desde V6/E1 `make sim LVGL=1 SQLITE=1` enlaza SQLite en el
      micro simulado y los demos de BD corren sin placa. De esto dependen dos
      frases: la de `docs/basedatos.html` («hoy hace falta placa») y la de las
      notas de v6.0. Se decide aquí, ANTES de montar el ZIP, y se escribe en las
      dos. Comprobación si entra: `strings build/bpvm-sim.exe | grep -c sqlite3_`
      debe dar decenas, no 0.
- [ ] **Una placa por familia con `tanda.py`** — la prueba de que la imagen que se
      publica CORRE, no solo que compila: con la placa conectada,
      `python bpgenvm-c/tools/tanda.py --puertos auto --informe compat/informes/f1.md`
      en al menos **Pico 2 o Metro · ESP32-S3 · ESP32-C3 o C6 · ESP32-P4 · Nucleo o
      DK2**, cada una con el firmware de `dist/firmware/` recién grabado. El informe
      se ABRE EN MODO AÑADIR (una sección por placa, Eduardo va rotándolas) y lleva
      el sello de la imagen (INFO) y la matriz pruebas × placas; lo que no sea
      IDENTICO/NO-PETA lleva su evidencia. ⚠️ Lo que cazó la prueba de fuego de T1
      (11-sep): la Metro tenía firmware del 9-sep — el sello del INFO lo dijo, el
      ojo no.
- [ ] **Montar el paquete**: `bash scripts/montar-zip.sh`.
      Falla a propósito si: aparece una subcarpeta de `samples/` sin decidir
      (hallazgo 35) · hay un `.mod` sin su `.bp` en `bpstdlib/` · la ayuda tiene
      enlaces rotos · el paquete no compila con su propia stdlib · **algún
      fichero de proyecto (`.bpbuild`/`.bpproject`) no abre** (hallazgo 42).
      ℹ️ El último mira **sólo la sintaxis**: que al proyecto le falte el `.mod`
      por compilar es normal en un paquete recién montado y NO es fallo.
- [ ] **Desplegar en carpeta LIMPIA y probar allí** — no sobre una instalación
      vieja, que es donde se esconden los ficheros que ya no se generan.
      La puerta final de H13 fueron 4 pruebas: `reset+MemInfo+paralleltest_sugar`
      en placa · `ChartDemo` en el emulador · `FontLoadDemo` en el emulador ·
      `Bench` en placa **y** en el emulador.

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
`grep -rn -i "\bv5\b" docs/ README*.md` y decide caso por caso qué es
historia legítima y qué es texto caducado.

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
      El README destruido medía 7.825.108 B con 0 encabezados; el sano, 12 KB con 18.
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
      `guia-ide`, `gui`, `bp-desde-dentro`, `creditos`) abren en ES y EN.
- [ ] ℹ️ `web/cheatsheet.html` **no** se publica automáticamente: vive fuera de
      `docs/` a propósito (para que no viaje en el ZIP). Si lo quieres en la web,
      hay que copiarlo o enlazarlo a mano.

## 4 · Release v6.0

- [ ] Tag `v6.0` sobre el commit publicado.
- [ ] Cuerpo de la release: **bilingüe, español y luego inglés**, con el molde de
      V5 (`gh release view v5.0 --json body`): intro · Lo nuevo · **Al actualizar**
      (en V6 es la sección que más pesa: `import Core`, `parent: Container`,
      `Keyboard.setTextarea`, `print e`, códigos de salida, reflashear y limpiar
      `/app`) · Descarga (tabla de las **siete** imágenes + `sha256` del ZIP) ·
      Documentación · Limitaciones.
      ⚠️ **No es un copia-pega de `RELEASES.md`**: aquel es sólo español y va por
      temas, no por release. Esto se escribió mal en el checklist y se descubrió el
      22-ago comparando con lo que V4 publicó de verdad.
- [ ] Adjuntar **el ZIP y nada más** (ver arriba: las imágenes van dentro).
- [ ] Comprobar desde la propia release: descargar el ZIP y verificar el
      `sha256` contra el que anotaste al montarlo.

## Limitaciones conocidas que van en las notas (no bloquean)

Aquí había una lista (la de V4, con sus «→ V5»). Se quita: **se desincronizaba**,
que es lo que pasa con toda lista que vive en dos sitios. La fuente es
`docs/RELEASES.md` («Lo que todavía no» de v6.0) y `docs/PENDIENTES.md`; el cuerpo
de la release se escribe desde ahí. Lo que sí es de este checklist: comprobar que
esas dos hablan de la MISMA versión que las portadas antes del push
(`grep -n "todavía no" docs/RELEASES.md`).
