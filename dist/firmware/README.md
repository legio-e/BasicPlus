# dist/firmware — las imágenes buenas

**La regla:** lo que se prueba en H13 y lo que se publica es **el mismo fichero, bit a
bit**. No se recompila entre probar y publicar. Si hay que recompilar algo, se vuelve a
sellar y **se vuelve a probar** lo que dependa de ello.

Esta carpeta es el único sitio del que se flashea durante H13 y el único del que se
suben los adjuntos de la release. Los directorios de compilación (`pico/build`,
`esp32*/build`, los `Debug/` de CubeIDE) son de trabajo: de ahí sale la imagen **una
vez**, se copia aquí, y a partir de ese momento manda esta copia.

## Las 5 imágenes (7 placas)

| Fichero | Placas | Cómo se genera |
|---|---|---|
| `bpvm_pico.uf2` | Pico 2 (a1) · Metro RP2350B (a2) | `ninja -C bpgenvm-c/pico/build` |
| `bpvm_esp32_merged.bin` | ESP32-S3 (b1) | `idf.py build` + `merge_bin` en `bpgenvm-c/esp32` |
| `bpvm_esp32p4.bin` | P4 Kit (b2) · P4 Waveshare (b3) | `idf.py build` en `bpgenvm-c/esp32p4` |
| `bpvm_stm32_nucleo.bin` | Nucleo-U575 (c1) | STM32CubeIDE |
| `bpvm_stm32_dk2.bin` | Discovery U5G9J (c2) | STM32CubeIDE |

**Una imagen sirve a dos placas en dos casos**, y no es un atajo: la variante se decide
en runtime. En RP2350 el micro se identifica solo (A/B: 30 o 48 GPIO, PSRAM); en el P4
el panel sale del **ENV** (`display=st7701`), no de la imagen (#311).

## Sellado

Al dejar una imagen aquí se regenera el manifiesto:

```bash
cd dist/firmware && sha256sum *.uf2 *.bin > SHA256SUMS.txt
```

Y antes de publicar se comprueba que nadie la ha tocado:

```bash
cd dist/firmware && sha256sum -c SHA256SUMS.txt
```

`SHA256SUMS.txt` **sí va al repo**; los binarios **no** (son 4 MB y se adjuntan a la
release de GitHub). Así queda escrito en el historial qué se probó exactamente, sin
engordar el repo. El manifiesto también se copia al registro de H13
(`docs/H13_PRUEBAS.md`) cuando cada placa se da por cerrada.

## Por qué

En V3 los binarios se adjuntaban directamente desde sus directorios de compilación. Eso
funciona mientras nadie recompile en medio — y basta un `ninja` de más para publicar
algo que nadie ha probado, sin que se note. Ayer mismo (2-ago) una imagen recompilada
mientras se probaba nos costó una cacería de un bug inexistente: el mecanismo importa.
