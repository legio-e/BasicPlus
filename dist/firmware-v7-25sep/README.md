# dist/firmware-v7-25sep — el punto de restauración con la ABI 8

**Qué es.** Las imágenes que se grabaron y se probaron en las nueve placas la tarde del
25-sep-2026, **bit a bit**: las de ESP32 son el `merge-bin` de los mismos `build/` que se
grabaron (con `esptool merge-bin @flash_args`, que no recompila), las de STM32 el `objcopy -O
binary` del mismo `.elf` grabado, y el `.uf2` es el que se copió por BOOTSEL a la Pico 2 y a la
Metro. Sustituye como punto de partida a `dist/firmware-v7-24sep`, que se queda: aquel es de
antes de la ABI 8 del `.mdn` (`#559`), y una imagen de entonces rechaza todo `.mdn` de hoy.

**Para qué.** Sigue siendo la precondición de `H6.b` (quitar LVGL de C6, P4 y DK2): *«una prueba
completa antes de quitar nada nos dará un punto para restaurar»* (Eduardo, 13-sep). Si `H6.b`
rompe algo y no se arregla, se vuelve a esto.

## Las 8 imágenes (9 placas)

| Fichero | Placas | Tanda de V7 (`tools/tanda_v7.json`, 14 pruebas) |
|---|---|---|
| `bpvm_pico.uf2` | Pico 2 · Metro RP2350B | 11 IDÉNTICO + 1 NO-PETA en las dos (sin pantalla), el AOT de ARM incluido |
| `bpvm_esp32_merged.bin` | ESP32-S3 | 8 + 1 (sin pantalla; sin AOT: Xtensa no tiene receta) |
| `bpvm_esp32c3_merged.bin` | ESP32-C3 | 11 + 1 (sin pantalla), el AOT `ilp32` incluido — con `#565` |
| `bpvm_esp32c5_merged.bin` | ESP32-C5 (Waveshare 2,8") | 12 + 2 con pantalla (LVGL de un pack) y AOT |
| `bpvm_esp32c6_merged.bin` | ESP32-C6-LCD-1.3 | 12 + 2 con pantalla y AOT — con el `Stdlib.pack` grabado (ver abajo) |
| `bpvm_esp32p4_merged.bin` | ESP32-P4 | 12 + 2 con pantalla (480×800) y AOT `ilp32f` — con `#566` |
| `bpvm_stm32_nucleo.bin` | Nucleo-U575 | 11 + 1 (sin pantalla), el AOT de ARM incluido |
| `bpvm_stm32_dk2.bin` | Discovery U5G9J | 12 + 2 con pantalla (800×480) y AOT |

El informe entero, placa a placa, en `compat/informes/tanda_v7_25sep.md` (y `.json`): seis
tandas, en el orden en que se conectaron las placas. La C3 sale dos veces —la primera con las
tres pruebas de AOT colgadas, que es `#565`— y el P4 tres —antes y después de regrabar sus packs,
y con `#566`—; vale la última de cada una.

## El estado de las placas que acompaña a las imágenes

- **C6**: su zona lleva el `Stdlib.pack` (`packs/`), y su `/lib`, sólo lo que pone la imagen.
  Así `GuiWinJson` cabe (la stdlib corre desde la flash: `#468`); sin el pack, con los 64 KB de
  heap que le deja LVGL dentro de la imagen, termina con «No space in heap».
- **P4**: zona formateada el 25-sep y regrabada con `SQLite.pack` (`packs/`), `Gui.pack` y
  `DriverNulo.pack` (`bpstdlib/gui/…/out/`), por ese orden, sellados para `0x40154000`. El rodata
  de su imagen acaba en `0x4014f96c`: una imagen que lo pase de `0x40150000` corre la zona una
  página y obliga a regrabarlos.
- **C5**: sus packs, sin tocar hoy (tres vivos; `Gui` y `DriverSpi` entre ellos).
- **Metro**: su `SQLite.pack`.
- **Nucleo**: su zona tenía restos de un `Core.mod` MOD7 de otra época de la flash («cadena
  ROTA»); Eduardo la formateó el 25-sep y quedó vacía.

## Grabarlas

```bash
# ESP32 (imagen fusionada, desde 0x0)
python -m esptool --chip <esp32s3|esp32c3|esp32c5|esp32c6|esp32p4> -p COMx write-flash 0x0 bpvm_<...>_merged.bin
# STM32
STM32_Programmer_CLI -c port=SWD sn=<SN> mode=UR -w bpvm_stm32_<nucleo|dk2>.bin 0x08000000 -v -rst
# RP2350: BOOTSEL (python bpgenvm-c/tools/flash_pico.py COMx dist/firmware-v7-25sep/bpvm_pico.uf2)
```

Y comprobar que nadie las ha tocado: `cd dist/firmware-v7-25sep && sha256sum -c SHA256SUMS.txt`.
