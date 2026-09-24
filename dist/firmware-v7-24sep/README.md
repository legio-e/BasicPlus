# dist/firmware-v7-24sep — el punto de restauración de mitad de V7

**Qué es.** Las imágenes que se grabaron y se probaron en las nueve placas la tarde del
24-sep-2026 (la «tarde de placas»), **bit a bit**: las de ESP32 son el `merge-bin` de los
mismos `build/` que se grabaron (se comprobó que la aplicación no se recompiló entre grabar y
fusionar), las de STM32 el `objcopy -O binary` del mismo `.elf` grabado, y el `.uf2` es el que
se copió por BOOTSEL a la Pico 2 y a la Metro. `dist/firmware/` sigue siendo V6: no se toca.

**Para qué.** Es la precondición de `H6.b` (quitar LVGL de C6, P4 y DK2): *«una prueba
completa antes de quitar nada nos dará un punto para restaurar»* (Eduardo, 13-sep). Si
`H6.b` rompe algo y no se arregla, se vuelve a esto.

## Las 8 imágenes (9 placas)

| Fichero | Placas | Tanda de V7 (`tools/tanda_v7.json`) |
|---|---|---|
| `bpvm_pico.uf2` | Pico 2 · Metro RP2350B | 8 IDÉNTICO + 1 NO-PETA en las dos (sin pantalla) |
| `bpvm_esp32_merged.bin` | ESP32-S3 | 8 + 1 (sin pantalla) |
| `bpvm_esp32c3_merged.bin` | ESP32-C3 | 8 + 1 (sin pantalla); y `trytest`, `JsonDemo` |
| `bpvm_esp32c5_merged.bin` | ESP32-C5 (Waveshare 2,8") | 9 + 2 con pantalla (LVGL de un pack) |
| `bpvm_esp32c6_merged.bin` | ESP32-C6-LCD-1.3 | 8 + 2 con pantalla; **`GuiWinJson`: «No space in heap»** (ver abajo) |
| `bpvm_esp32p4_merged.bin` | ESP32-P4 | 9 + 2 con pantalla (480×800) |
| `bpvm_stm32_nucleo.bin` | Nucleo-U575 | 8 + 1 (sin pantalla) |
| `bpvm_stm32_dk2.bin` | Discovery U5G9J | 9 + 2 con pantalla (800×480) |

El informe entero, placa a placa, en `compat/informes/tanda_v7.md` (y `.json`).

## Lo que se sabe y queda anotado

- **La C6 no tiene memoria para `GuiWinJson`** en V7 (termina con «No space in heap», con
  nombre); en V6 sí cabía. Es `#468` (la stdlib en RAM en la C6).
- **El botón BOOT de la C6 (GPIO9) no se detecta**: lee 1 siempre. `#544`.
- Las ESP32 llevan la pila de la tarea `main` a **16 KB** (`#543`): con 8 KB la C6
  reventaba al cargar un programa con dependencias hondas.

## Grabarlas

```bash
# ESP32 (imagen fusionada, desde 0x0)
python -m esptool --chip <esp32s3|esp32c3|esp32c5|esp32c6|esp32p4> -p COMx write-flash 0x0 bpvm_<...>_merged.bin
# STM32
STM32_Programmer_CLI -c port=SWD sn=<SN> mode=UR -w bpvm_stm32_<nucleo|dk2>.bin 0x08000000 -v -rst
# RP2350: BOOTSEL (python bpgenvm-c/tools/flash_pico.py COMx dist/firmware-v7-24sep/bpvm_pico.uf2)
```

Y comprobar que nadie las ha tocado: `cd dist/firmware-v7-24sep && sha256sum -c SHA256SUMS.txt`.
