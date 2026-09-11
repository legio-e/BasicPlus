# El fichero `.shot` — la captura de pantalla de BasicPlus (V6/C1)

> **Qué es.** Lo que escribe `Gui.shot(path)`: la pantalla **tal como LVGL la dibujó**, en el
> formato de píxel de las placas (RGB565), troceada en franjas y comprimida. Se escribe **igual**
> desde el host (VM-C con SDL, también `--no-screen`) y desde las tres placas con pantalla (C6,
> P4, DK2), por el **mismo camino** (el gancho del flush de LVGL), y se baja con el `GET` de siempre.
> El PC lo convierte a PNG con `bpgenvm-c/tools/shot2png.py`.
>
> **Qué NO es.** No es el oráculo automático de la GUI (eso es el árbol: `toJson()` / `dumpTree`,
> con paridad byte a byte entre las dos VMs). La imagen es **la prueba para el ojo** — y para
> comparar la misma placa consigo misma en el tiempo. Host y placa no son comparables píxel a píxel
> (32 vs 16 bpp al dibujar, antialiasing distinto). Y la captura es lo que LVGL **dibujó**, no lo
> que el panel **muestra**: un backlight apagado da una captura perfecta y una pantalla negra.

## Por qué así (las decisiones, con su medida)

- **El gancho es `LV_EVENT_FLUSH_START`**, no los cuatro `flush_cb` de familia: LVGL 9.2.2 lo
  emite justo antes de cada `flush_cb` con el área, y `lv_display_get_buf_active()` es la banda.
  Cero líneas por familia, llega **antes** del `lv_draw_sw_rgb565_swap` in situ del C6, y sirve
  igual en `RENDER_MODE_PARTIAL` (las tres placas, host `--no-screen`) y en `DIRECT` (el host con
  ventana SDL, que **no** va por bandas: un solo flush de pantalla entera, premisa que la ficha
  `#475` daba por falsa).
- **Se invalida la pantalla entera y se fuerza el refresco** (`lv_obj_invalidate(screen)` +
  `lv_refr_now`): sin eso, en PARTIAL sólo llegan las áreas sucias y la captura sale **a trozos**.
  Con la pantalla entera invalidada, LVGL entrega bandas de ancho completo en orden de `y`.
- **Franjas de ≤ 24 filas, independientes**: la banda de cada placa mide distinto (C6 24 líneas,
  DK2 48, P4 120, host DIRECT 320); troceando cada banda en franjas de 24 el fichero tiene la
  **misma estructura** en las cuatro, y el buffer de entrada del compresor es pequeño y acotado.
- **RGB565 en las cuatro**: las placas ya dibujan en 565; el host (XRGB8888) **convierte al
  capturar**. Un solo formato de píxel en el fichero.
- **Comprimido en RAM, escrito de una vez**: la fachada FS no tiene escritura en streaming, y
  escribir por bandas son N commits de littlefs (el patrón que costó 45× en `#398`). Se acumula
  el comprimido y se hace **un** `bpvm_fs_write`. Antes se borra el fichero si existe: en littlefs
  reescribir pide transitoriamente viejo + nuevo (medido en la DK2: `-28`).
- **LZ4 (bloques, el de LVGL)** con `LZ4_MEMORY_USAGE 10`: el estado por defecto (14) son
  **16 416 B en la pila** de la tarea `vm` — 16 KB en la DK2 y 8 KB en el C6, desbordamiento
  seguro. Con 10 son 1 056 B, y además se reserva en el heap con `LZ4_sizeofState()`, así que el
  tamaño real lo dice el propio `lz4.c` compilado, no una macro que pueda desincronizarse. El
  fichero lleva el códec en la cabecera: `0` = crudo (lo escribe miVM, que no lleva LZ4), `1` = LZ4.
- **La imagen es la LÓGICA** (lo que LVGL pinta, antes de cualquier rotación del driver): es lo que
  da `FLUSH_START` en las cuatro. La cabecera lleva la rotación por si el PC quiere girarla.

## El formato (todo little-endian)

```
Cabecera — 16 bytes
  0   char[4]  "BPSH"
  4   u8       version   = 1
  5   u8       fmt       = 1   (RGB565: cada píxel u16 little-endian, R5 G6 B5)
  6   u8       codec     = 0 crudo | 1 LZ4 bloque (sin frame, sin cabecera propia)
  7   u8       rot       = rotación LVGL de la pantalla al capturar: 0=0°, 1=90°, 2=180°, 3=270°
  8   u16      w         ancho lógico en píxeles
  10  u16      h         alto lógico
  12  u32      nblocks   número de bloques que siguen

Bloque — cabecera de 16 bytes + datos
  0   u16 x1, u16 y1, u16 x2, u16 y2   rectángulo en coordenadas de pantalla, INCLUSIVO
  8   u32 rawLen     = (x2-x1+1) * (y2-y1+1) * 2   (bytes del rectángulo en RGB565, filas contiguas)
  12  u32 compLen    bytes que siguen
  16  compLen bytes  (codec 0: rawLen bytes tal cual; codec 1: un bloque LZ4 que descomprime a rawLen)
```

Los bloques no se solapan y, juntos, cubren la pantalla entera. Normalmente son franjas de ancho
completo y ≤ 24 filas, en orden de `y` creciente; el lector **no debe suponerlo**: coloca cada
bloque donde dice su rectángulo.

## Contrato de `Gui.shot(path)`

- La ejecuta el hilo `vm` (todo hilo BP corre sobre esa tarea, que es donde vive LVGL): no reentra
  LVGL ni compite con `Gui.run()`. Con `--smp=N` (varios workers, sólo host) **no está soportada**.
- Devuelve los **bytes escritos** (> 0). Falla con `RuntimeError` atrapable y mensaje fijo:
  `Gui.shot('p'): sin pantalla` (build sin LVGL, o miVM sin ventana) ·
  `Gui.shot('p'): error de escritura` · `Gui.shot('p'): no cabe en el tope de captura` (la comprimida
  supera el tope `BPVM_SHOT_MAX`) · `Gui.shot('p'): sin memoria`.
- El `path` se usa **tal cual**, como `writeFile`: en placa, relativo a la raíz del littlefs; en el
  host VM-C, relativo al cwd del proceso (o dentro de la imagen con `--fs=lfs:`); en miVM, dentro
  del workdir. No crea directorios.
- Los mismos **mensajes** en las dos VMs; el **valor** (bytes escritos) no coincide ni puede: miVM
  escribe crudo (códec 0, 307 440 B a 480×320) y la VM-C comprime (LZ4, ~4-5 KB), y cada pantalla
  mide lo suyo. Un programa que quiera paridad de `stdout` **no imprime el valor** (así lo hace
  `samples/GuiShot.bp`). miVM pinta su árbol Swing a un `BufferedImage`. Bajo `--workdir` (el IDE),
  miVM aplica al `path` el mismo sandbox que `writeFile`.
- `BPVM_SHOT_MAX` se compara con lo que la captura **ocupa**: el compresor escribe con salida
  limitada a lo que queda libre y el buffer crece (×2) hasta el tope; sólo si el fichero final no
  cabe sale «no cabe». Medido: 480×320 ruidosa → 74 KB con el tope por defecto; con 20 KB, «no
  cabe» y sin fichero; la normal (4,4 KB) cabe en 20 KB.
- El fichero **sólo se puede bajar cuando el programa ha terminado** (o tras `KILL`): el wire
  contesta `BUSY` a `GET` durante un RUN en las cinco implementaciones. Flujo de prueba:
  `RUN → (shot dentro) → EXITED → GET → shot2png.py`.

## Lo que la primera captura enseñó (11-sep, host + Discovery)

- **Bombear ANTES de capturar.** El tema de LVGL anima las transiciones de estado (el check de un
  checkbox tarda ~100 ms en aparecer). Una captura justo después de `cb.checked := true`, sin una
  vuelta de lazo, sale con el checkbox **vacío** aunque el modelo diga `val=1` — medido. Es lo que
  se ve en ese instante, no el modelo. Receta: `Gui.start()` · `sleep(300)` · `Gui.shot(...)` ·
  `Gui.stop()` · `Gui.join()` (`samples/GuiShot.bp`).
- **Host `--no-screen`** entrega bandas de 10 filas, así que ahí los bloques son de 10, no de 24;
  con ventana (DIRECT) son 14 franjas de 24. El lector no supone ninguna de las dos.
- **En la Discovery `/lib` se VACÍA en cada arranque** (`fs_lfs_stm32.c:clear_lib`) y se reinstala
  sólo lo embebido — un `Gui.mod` subido a mano a `/lib` muere al primer reset. Los módulos de
  stdlib que no van en la imagen (`Gui`, `Json`, `Collections`, `Str`) van a **`/app`**, que es
  donde los deja el IDE. (Ojo `#493`: `/app` tapa a `/lib`.)
- Números reales: DK2 800×480 → **20 bloques, 6 956 B (110×), `GET` en 643 ms** a 115 200; **C6 240×240 →
  10 bloques, 2 862 B (40×), `GET` en 8 ms** por USB-JTAG; **P4 1024×600 → 25 bloques, 8 885 B (138×),
  `GET` en 820 ms**, y girado 90° la lógica 600×1024 con `rot=1`; host 480×320 → 4,4 KB (70×); miVM
  (Swing, códec 0) → 307 440 B.
- **En el C6 lo que manda es la DRAM de plataforma, no el tope**: tras LVGL quedan ~26 KB (bloque mayor
  14 KB). Por eso en placa se comprime **desde la banda, sin copiar** (ya es RGB565 y contigua) y el buffer
  de salida arranca en 4 KB; el `tmp` de conversión sólo existe en el host. La captura cuesta ~5 KB de DRAM.
- Bajar por COM sin el IDE: `python tools/wire_serie.py COM12 get /GuiShot.shot local.shot`
  (desde Git Bash con `MSYS_NO_PATHCONV=1`), y `python tools/shot2png.py local.shot`.

## Tamaños de referencia (crudo; la medida del 5-sep dio 21,8×–152,6× con LZ4 en pantallas reales)

| | pantalla | crudo RGB565 | banda LVGL | franjas de 24 |
|---|---|---|---|---|
| C6 | 240×240 | 115 200 B | 24 filas (11 520 B) | 10 |
| DK2 | 800×480 | 768 000 B | 48 filas | 20 |
| P4 | 1024×600 | 1 228 800 B | 120 filas | 25 |
| host | 480×320 (`--screen=`) | 307 200 B | DIRECT: 1 flush entero (`--no-screen`: 10 filas) | 14 |

`BPVM_SHOT_MAX` (tope del comprimido en RAM): 256 KB por defecto; 40 KB en el C6 (68 KB libres).
