# dist/firmware — las imágenes buenas

**La regla:** lo que se prueba antes de publicar (la ronda final en las placas) y lo que
se publica es **el mismo fichero, bit a bit**. No se recompila entre probar y publicar.
Si hay que recompilar algo, se vuelve a sellar y **se vuelve a probar** lo que dependa
de ello.

Esta carpeta es el único sitio del que se flashea durante esa ronda y el único del que
el empaquetador (`scripts/montar-zip.sh`) copia las imágenes al `firmware/` del ZIP.
Los directorios de compilación (`pico/build`, `esp32*/build`, los `Debug/` de CubeIDE)
son de trabajo: de ahí sale la imagen **una vez**, se copia aquí, y a partir de ese
momento manda esta copia.

## Las 8 imágenes (10 placas)

| Fichero | Placas | Cómo se genera |
|---|---|---|
| `bpvm_pico.uf2` | Pico 2 (a1) · Metro RP2350B (a2) | `ninja -C bpgenvm-c/pico/build` |
| `bpvm_esp32_merged.bin` | ESP32-S3 (b1) | `idf.py build` + `merge-bin` en `bpgenvm-c/esp32` |
| `bpvm_esp32c3_merged.bin` | ESP32-C3 (b4, V6) | `idf.py build` + `merge-bin` en `bpgenvm-c/esp32c3` |
| `bpvm_esp32c5_merged.bin` | ESP32-C5 Waveshare 2,8" (V7) | `idf.py build` + `merge-bin` en `bpgenvm-c/esp32c5` |
| `bpvm_esp32c6_merged.bin` | ESP32-C6 / C6-LCD-1.3 (b5, V6) | `idf.py build` + `merge-bin` en `bpgenvm-c/esp32c6` |
| `bpvm_esp32p4_merged.bin` | P4 Kit (b2) · P4 Waveshare (b3) | `idf.py build` + `merge-bin` en `bpgenvm-c/esp32p4` |
| `bpvm_stm32_nucleo.bin` | Nucleo-U575 (c1) | STM32CubeIDE |
| `bpvm_stm32_dk2.bin` | Discovery U5G9J (c2) | STM32CubeIDE |

**Una imagen sirve a dos placas en dos casos**, y no es un atajo: la variante se decide
en runtime. En RP2350 el micro se identifica solo (A/B: 30 o 48 GPIO, PSRAM); en el P4
el panel sale del **ENV** (`display=st7701`), no de la imagen (#311). En V7 son **ocho
ficheros**: el empaquetador copia `*.uf2` y `*.bin` sin contar, así que se cuentan a
mano (`ls *.uf2 *.bin | wc -l` → 8).

Desde V7 la pantalla de la C5, la C6, las P4 y la DK2 no va en su imagen: llega en los
packs de la GUI, que no se sellan aquí. Viven en `packs/` y los vigila el empaquetador
(`#679`: exige los cinco y que cada uno sea, byte a byte, el de su compilación).

## Sellado

Al dejar una imagen aquí se regenera el manifiesto:

```bash
cd dist/firmware && sha256sum *.uf2 *.bin > SHA256SUMS.txt
```

Y antes de publicar se comprueba que nadie la ha tocado:

```bash
cd dist/firmware && sha256sum -c SHA256SUMS.txt
```

`SHA256SUMS.txt` **sí va al repo**; los binarios **no** (son unos 6 MB y van dentro del
ZIP, en `firmware/`). Así queda escrito en el historial qué se probó exactamente, sin
engordar el repo. Y el informe de `tanda.py` de cada placa (`compat/informes/`) anota el
sello de la imagen que corría (`imageCrc`, del `HELLO`).

## Por qué

En V3 los binarios se adjuntaban directamente desde sus directorios de compilación. Eso
funciona mientras nadie recompile en medio — y basta un `ninja` de más para publicar
algo que nadie ha probado, sin que se note. Ayer mismo (2-ago) una imagen recompilada
mientras se probaba nos costó una cacería de un bug inexistente: el mecanismo importa.

## Los `_merged` se flashean en el offset 0

Las cinco imágenes ESP32 llevan `_merged` en el nombre y no es decorativo: son
**bootloader + tabla de particiones + aplicación en un solo fichero**, listos para grabar
de una pieza en el **offset 0**. El binario suelto de la aplicación (`bpvm_esp32*.bin`
del directorio de compilación) va en 0x10000 y **por sí solo no arranca**; publicar ese
sería regalar un ladrillo a quien no tenga ya el bootloader puesto. El offset del
bootloader NO es el mismo en todos los chips (0x0 en la S3, la C3 y la C6; 0x2000 en la
C5 y el P4), ni la flash (16 MB la S3 y el P4, 4 MB la C3 y la C6, 32 MB la C5): otra
razón para no dejar que nadie lo componga a mano.

Se generan con los parámetros que dice el propio build (`build/flash_args`), no de
memoria: esptool (v5, la del IDF v6) los lee tal cual con `@`.

```bash
cd bpgenvm-c/esp32c5/build
esptool --chip esp32c5 merge-bin -o bpvm_esp32c5_merged.bin @flash_args
```

(Así se hicieron las de `dist/firmware-v7-25sep`; en cada imagen, su `--chip` y su
nombre.) Comprobación barata de que el merge salió bien: `tamaño(merged) - 0x10000` tiene
que dar EXACTAMENTE el tamaño de la aplicación.
