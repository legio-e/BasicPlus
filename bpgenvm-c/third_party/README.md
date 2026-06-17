# third_party/ — dependencias vendorizadas (V3 / H4.2 GUI)

Estas dependencias **NO se versionan** (ver `.gitignore`): se descargan localmente.
Las usa el backend GUI con render real (LVGL + SDL2) de la VM-C en host — la
variante "con GUI" de las dos imágenes (la "sin GUI" / `GUI=0` no las necesita).

## Qué hay aquí

- **`lvgl/`** — LVGL **v9.2.2** (trae los drivers SDL `lv_sdl_window`/`lv_sdl_mouse`
  en `src/drivers/sdl/`, así que no escribimos driver). Config en
  `../include/lv_conf.h` (basado en `lv_conf_template.h`: `#if 1`, `LV_COLOR_DEPTH 32`,
  `LV_USE_SDL 1`, stdlib = CLIB).
- **`SDL2/`** — SDL2 **2.30.9** dev para MinGW (`x86_64-w64-mingw32/{include,lib,bin}`).
  `SDL2.dll` (de `bin/`) debe ir junto al ejecutable.

Toolchain de referencia: **w64devkit** (gcc 16.x), sin gestor de paquetes → por eso
se vendorizan a mano.

## Cómo regenerarlas (fresh checkout)

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
```
