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
| `bpvm_esp32p4_merged.bin` | P4 Kit (b2) · P4 Waveshare (b3) | `idf.py build` + `merge_bin` en `bpgenvm-c/esp32p4` |
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

## Los `_merged` se flashean en el offset 0

Los dos ESP32 llevan `_merged` en el nombre y no es decorativo: son **bootloader +
tabla de particiones + aplicación en un solo fichero**, listos para grabar de una pieza
en el **offset 0**. El binario suelto de la aplicación (`bpvm_esp32*.bin` del directorio
de compilación) va en 0x10000 y **por sí solo no arranca**; publicar ese sería regalar un
ladrillo a quien no tenga ya el bootloader puesto. El offset del bootloader NO es el
mismo en los dos chips (0x0 en el S3, 0x2000 en el P4), otra razón para no dejar que
nadie lo componga a mano.

Se generan con los parámetros que dice el propio build (`build/flasher_args.json`), no
de memoria:

```bash
esptool --chip esp32s3 merge_bin -o bpvm_esp32_merged.bin   --flash_mode dio --flash_freq 80m --flash_size 16MB   0x0 bootloader/bootloader.bin 0x8000 partition_table/partition-table.bin   0x10000 bpvm_esp32.bin
```

Comprobación barata de que el merge salió bien: `tamaño(merged) - 0x10000` tiene que
dar EXACTAMENTE el tamaño de la aplicación.
