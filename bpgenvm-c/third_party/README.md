# third_party/ — dependencias vendorizadas

Dos categorías, según sean núcleo o backend opcional:

- **In-tree (committeadas)** — núcleo **no-opcional** que va a host **y a los 3 firmwares**.
  Build hermético y reproducible **sin fetch**; versión clavada en git. Hoy: **`littlefs/`**
  y **`fatfs/`**.
- **Descargadas localmente (gitignored, ver `.gitignore`)** — backend **GUI opcional**
  (render real LVGL + SDL2) de la variante "con GUI" de la VM-C en host; la variante
  "sin GUI" / `GUI=0` no las necesita. Hoy: **`lvgl/`**, **`SDL2/`**.

## In-tree (committeadas)

- **`littlefs/`** — littlefs **v2.11.3** — el **motor del FS de flash** (H2 fase A). Recortado
  a lo que compilamos: núcleo (`lfs.{c,h}`, `lfs_util.{c,h}`) + `bd/` (block-devices:
  `lfs_rambd` en RAM, `lfs_filebd` sobre fichero, `lfs_emubd` con inyección de fallos) +
  `LICENSE.md` (BSD-3) + docs upstream (`README.md`, `DESIGN.md`, `SPEC.md`). **Fuera** el
  harness de `tests/`/`scripts/`/`runners/`/`benches/` de upstream. Se versiona in-tree
  (soberanía: build reproducible aunque upstream desaparezca). Smoke: `make test-lfs`.

  **Actualizar a otra versión** (`vX.Y.Z`):
  ```sh
  cd bpgenvm-c/third_party
  curl -fsSL -o lfs.tgz https://github.com/littlefs-project/littlefs/archive/refs/tags/vX.Y.Z.tar.gz
  tar xzf lfs.tgz && rm -f lfs.tgz && rm -rf littlefs && mv littlefs-X.Y.Z littlefs
  cd littlefs && rm -rf .github tests scripts runners benches Makefile .gitattributes .gitignore
  ```

- **`fatfs/`** — FatFs **R0.16** (ChaN) — el **motor FAT de la tarjeta SD** (V5/H2). Es el
  formato que entiende el PC, que es justo para lo que sirve una SD: sacarla y leerla.
  Licencia BSD de 1 cláusula, y **la redistribución en binario no exige crédito en la
  documentación** (sí conservar el aviso en el fuente, que va intacto en `ff.c`).
  Compilamos `ff.c` + `ffunicode.c`; el resto del paquete son ejemplos y documentación.
  Coste medido en la Pico: **12,7 KB de flash y 1,7 KB de RAM** (con `fs_fat.c` incluido).

  > ⚠️ **AL ACTUALIZAR, BORRAR SU `ffconf.h`.** El paquete trae uno de plantilla y el
  > nuestro vive en `../include/ffconf.h`, que es el que lleva escritas las decisiones
  > (CP850, nombres largos en UTF-8, sin exFAT, sin `f_mkfs`, el lock lo pone `fs_fat.c`).
  > Si se queda el suyo, **manda el que primero encuentre el `-I`** — y entonces la
  > configuración cambia sin que nadie lo haya pedido y sin un solo mensaje. Borrarlo
  > convierte ese fallo silencioso en un error de compilación.

  **Actualizar a otra versión** (`ffNN.zip`, p. ej. `ff16.zip`):
  ```sh
  cd bpgenvm-c/third_party
  curl -fsSL -o ff.zip http://elm-chan.org/fsw/ff/arc/ffNN.zip
  rm -rf fatfs && mkdir fatfs && cd fatfs && unzip -j ../ff.zip && cd .. && rm -f ff.zip
  rm -f fatfs/ffconf.h          # <-- el nuestro manda: ../include/ffconf.h
  ```
  Y después **comparar** el `ffconf.h` nuevo del paquete con el nuestro antes de tirarlo:
  una versión nueva puede traer opciones que no existían, y con el suyo borrado no habría
  quien lo cuente.

## Descargadas localmente (GUI, NO versionadas)

Estas dependencias **NO se versionan** (ver `.gitignore`): se descargan localmente. Las usa
el backend GUI con render real (LVGL + SDL2) de la VM-C en host — la variante "con GUI" de
las dos imágenes (la "sin GUI" / `GUI=0` no las necesita).

- **`lvgl/`** — LVGL **v9.2.2** (trae los drivers SDL `lv_sdl_window`/`lv_sdl_mouse`
  en `src/drivers/sdl/`, así que no escribimos driver). Config en
  `../include/lv_conf.h` (basado en `lv_conf_template.h`: `#if 1`, `LV_COLOR_DEPTH 32`,
  `LV_USE_SDL 1`, stdlib = CLIB).
- **`SDL2/`** — SDL2 **2.30.9** dev para MinGW (`x86_64-w64-mingw32/{include,lib,bin}`).
  `SDL2.dll` (de `bin/`) debe ir junto al ejecutable.

Toolchain de referencia: **w64devkit** (gcc 16.x), sin gestor de paquetes → por eso
se vendorizan a mano.

### Cómo regenerarlas (fresh checkout)

```sh
cd bpgenvm-c/third_party
curl -L -o sdl2.tgz https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-devel-2.30.9-mingw.tar.gz
curl -L -o lvgl.tgz https://github.com/lvgl/lvgl/archive/refs/tags/v9.2.2.tar.gz
tar xzf sdl2.tgz && tar xzf lvgl.tgz && rm -f sdl2.tgz lvgl.tgz
mv SDL2-2.30.9 SDL2 && mv lvgl-9.2.2 lvgl
```

## Smoke de toolchain

```sh
cd bpgenvm-c && make gui-smoke   # compila LVGL+SDL, abre una ventana, imprime "LVGL+SDL smoke OK"
cd bpgenvm-c && make test-lfs    # compila littlefs, smoke del FS en host, imprime "littlefs smoke OK ..."
```
