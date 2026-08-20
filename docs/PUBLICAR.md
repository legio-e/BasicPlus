# Checklist de publicación — BasicPlus v4.0

Doc de mantenedor (no enlazado desde la portada). El repo público
`legio-e/BasicPlus` **ya existe**; publicar es un `push` + una release.

> **Esto se hace una vez cada varias semanas y no se recuerda.** Por eso cada paso
> dice *qué* se hace, *quién* lo hace y **cómo saber que salió bien**. Si un paso
> no tiene forma de comprobarse, es que está mal escrito.

**Lo que se publica en V4 es UN artefacto**: `BasicPlus-4.0-win.zip` — el paquete
completo (IDE + compilador + stdlib + samples + docs + micro simulado + **las 5
imágenes de firmware dentro**, en `firmware/`).

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

## 1 · Pre-flight — que lo que se publica sea lo que se probó

Estos pasos **ya se hicieron el 6-ago-2026** (H13). Se dejan escritos porque la
próxima vez hay que repetirlos, y porque **cada uno tapa una trampa que ya nos
mordió al menos una vez**.

- [ ] **Suite verde**: `cd lexer-java && mvn test`.
- [ ] **Paridad dual-VM** en host: `bash compat/compat.sh check`.
- [ ] **Stdlib canónica**: si tocaste `bpstdlib/*.bp`, recompílala y luego
      `bash bpgenvm-c/scripts/regen_all_mods.sh` para resincronizar los blobs
      embebidos de las familias.
      ⚠️ *Trampa*: los blobs de stdlib son **generados**. Se tocan regenerando,
      nunca a mano.
- [ ] **Las 5 imágenes, del MISMO árbol**, y en una sola tanda:
  - RP2350: `ninja -C bpgenvm-c/pico/build` → `bpvm_pico.uf2`
  - ESP32-S3: `cd bpgenvm-c/esp32 && idf.py build`
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
    (S3 `0x0/0x8000/0x10000` @80m · P4 `0x2000/0x8000/0x10000` @40m).
  - Regenerar `dist/firmware/SHA256SUMS.txt`.
- [ ] **BpIde**: ⚠️ **con el IDE CERRADO**. `mvn install` en `miVM` y en
      `lexer-java`, luego `mvn package` en `BpIde`. La versión sale del pom
      (`BpIde-5.0.jar`).
- [ ] **Micro simulado con LVGL**: `cd bpgenvm-c && make sim LVGL=1`.
      ⚠️ *Trampa (hallazgo 37)*: **sin `LVGL=1` el simulador ejecuta pero no
      pinta**, y el fallo es mudo. Comprobación: `strings build/bpvm-sim.exe |
      grep -c "^lv_"` debe dar **miles**, no 0.
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

⚠️ Los volúmenes en **inglés** se quedan atrás sin que nadie lo note:
`docs/en/index.html` llegó a estar **dos versiones** por detrás. Búscalo con
`grep -rn -i "\bv3\b" docs/ README*.md` y decide caso por caso qué es
historia legítima y qué es texto caducado.

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

## 4 · Release v4.0

- [ ] Tag `v4.0` sobre el commit publicado.
- [ ] Cuerpo de la release = sección **v4.0** de `docs/RELEASES.md`.
- [ ] Adjuntar **el ZIP y nada más** (ver arriba: las imágenes van dentro).
- [ ] Comprobar desde la propia release: descargar el ZIP y verificar el
      `sha256` contra el que anotaste al montarlo.

## Limitaciones conocidas que van en las notas (no bloquean)

- **El wire se desincroniza tras el Stop** y hay que reconectar — sólo en algunas
  placas (funciona bien en STM32, intermitente en P4). Investigado y acotado:
  no es el KILL ni el drenaje del IDE. → V5 (#379).
- **`loadFont` no puede decir que ha fallado**: devuelve un id válido siempre y
  `setFont` con una fuente que no cargó es un no-op. → V5.
- **Al cambiar del emulador a una placa, el AOT puede saltarse**: el IDE se queda
  con la arquitectura anterior. Ya **avisa** por consola y se cura reconectando
  (o reiniciando el IDE). El arreglo de fondo → V5 (#379/#40).
- **Los contadores de periféricos del INFO son informativos** y no siempre casan
  con los del lenguaje (el mismo core sale en encapsulados distintos). → V5 (#378).
- `Gui.setRotation` no rota en LTDC (STM32 Discovery); el evento se dispara y el
  modelo gira, pero el panel no.
- Sin driver WS2812 fuera del RP2350 — desde V4 **lo dice** en vez de pasar en
  verde sin encender nada.
