# La RAM de las placas: dónde se va (la C3 y la C6 primero)

> Eduardo, 7-oct-2026: *«Documenta el consumo de memoria. En C3 y C6 es importante ya que vamos muy justos de RAM.»*
>
> **Medido en la C6** el 7-oct con la imagen de ese día (`2609629c`) y `tools/medir_consumo.py`: dos pasadas, las mismas
> cifras al byte. **La C3**, el mismo día y con la misma herramienta (imagen `8ec31ecd`; con `log=1` en su ENV para la
> medida, y `log=0` otra vez después, como estaba). Las fichas: `#651` (la GUI de la C6, al límite), `#666` (lo que no se suelta al acabar un RUN),
> `#667` (los destructores), `#650` (el módulo del pack que no gasta RAM).
>
> **Remedida la C6 el 8-oct** (imagen `7c18cbb7`, de `cd9a8ad1`, con los packs nuevos de `#623`), tras `#651` (los
> nombres del cargador, un bloque por módulo), `#650` y `#623` (la stdlib, XIP del `Stdlib.pack`) y `#666`–`#668` (lo
> que se suelta al acabar). Las tablas dan las dos medidas: la del 8-oct y, entre paréntesis, la del 7-oct. La C3,
> por remedir.
>
> **El 27-oct**, sin placas conectadas: el `INFO` del lote de esa tarde y los `.elf` del día (§7). Lo que se puede
> recortar —sobre todo, las pilas de los hilos BP, del heap: 48 KB más en la C3 y la C6—, en `#740`. **El 29-oct**, las
> pilas, del heap, en el código (las dos VMs); medirlo en las placas, en el lote siguiente.

## 1. Dos memorias que no hay que confundir

| | dónde está | en la C3 y la C6 | quién la gasta | cómo se mira |
|---|---|---|---|---|
| **el heap de BP** | dentro del bloque de la VM | fijo: 63 KB en la C6 (8-oct; 64 hasta el 7-oct: §2), 48 en la C3 (6-oct) | los objetos, strings y arrays del programa; el GC lo recoge | `Heap.getFree()`, `vmHeapBytes` del `INFO` |
| **la RAM del sistema** | el heap del IDF, fuera de la VM | lo que quede: en la C6, ~80 KB en reposo | el sistema, y lo que el núcleo pide en C para el programa: las tablas para cargarlo, los ficheros abiertos, la GUI, las natives, los periféricos | la línea `mem:` del log tras cada RUN (con `log=1`), `rtosHeapMinFreeBytes` del `INFO` (el mínimo histórico), «heap del sistema, mínimo libre» en la tanda |

Un programa puede tener el heap de BP medio vacío y quedarse sin RAM del sistema: «enlace tardío: sin memoria para
construir las clases» en GuiArbolG2 (`#651`) es eso —la tabla de clases del enlace se pide en C, de una vez, y no
encuentra un bloque contiguo—.

## 2. El reparto al arrancar: la C6

| | |
|---|---|
| SRAM | 512 KB, sin PSRAM |
| la imagen (código en IRAM, datos y `.bss`) | ~133 KB (`_heap_start` = `0x408215d0` el 8-oct; `0x408214c0` el 7-oct: 272 B más) |
| el IDF, FreeRTOS y el arranque, antes de los packs | ~21 KB |
| **el bloque de los packs** (`packRam=136` en el ENV) | **136 KB**: los estáticos de los motores de la placa —`DriverSpi` y `GuiMini`, 133 600 B, con el pool de LVGL de 64 KB— y 5 KB de arena |
| **el bloque de la VM** | **127 KB** (8-oct): 63 de heap de BP y 64 de pilas de los hilos (no bajan de 64). El arranque lo pide, en KB enteros, si deja al sistema su margen (`CHIP_MARGEN_SISTEMA`, 103 824 B): el 7-oct, con 128 KB, dejaba 103 864, 40 B por encima; con los 272 B de más de la imagen del 8-oct no llega, y baja 1 KB —que pierde el heap—. El log lo dice: «vm: memoria 127 KB -> heap 63 KB + pilas 64 KB» |
| lo que el arranque reserva después | ~23 KB (entre otras cosas, la tarea del programa: 10 KB de pila) |
| **en reposo, tras un programa vacío** | **80 588 B libres; bloque mayor, 63 488 B** (8-oct; el 7-oct, 80 936 y 65 536) |

El log del arranque lo dice (`heap: libre … | mayor …`, `vm: DRAM interna libre … | margen …`, `packs: bloque de
RAM …`). Sin la GUI en pack —sin el bloque de 136 KB—, a la C6 le quedarían otros 136 KB.

## 3. Lo que gasta un programa: la C6

La línea `mem:` se escribe al acabar el RUN y **antes** de liberar el programa: cuenta lo que el programa tenía vivo.
«En uso al acabar» = el libre en reposo (80 588; el 7-oct, 80 936) − el libre de esa línea; «peor momento» = el libre
en reposo − el mínimo histórico (sólo cuando ese programa lo bajó). Todo se compara con un programa vacío (`Nada`: su
módulo y `Core`). En KB de 1000 B, como el 7-oct; el bloque mayor, en KB de 1024.

| programa | en uso al acabar | peor momento | al acabar el RUN |
|---|---|---|---|
| `Importa5`: cinco módulos más de la stdlib (Math, Str, Collections, Json, Time —y Rtc, que trae Time—), sin tocar nada | 21,5 KB (36,0) | 27,9 KB (42,8) | vuelve |
| `Limpia`: importa `IO` | 3,0 KB (5,4) | | vuelve |
| `FicheroAbierto`: importa `IO` y deja un fichero abierto | 5,0 KB (5,6): 2,1 más que `Limpia` —no es el fichero: `FicheroCerrado`, con el fichero cerrado, gasta lo mismo; es la destrucción del final, la fila siguiente— | | vuelve: lo cierra su destructor |
| `ConDestructor`: un objeto con destructor vivo al acabar, sin `IO` | 2,1 KB (—) | | vuelve: es la destrucción del final de `#668`, que corre antes de la línea `mem:`; se suelta con el programa |
| `NatV7` / `FloatNat`: natives (su código, en RAM ejecutable mientras corre el programa) | 12,5 / 2,2 KB (13,5 / 3,2) | 16,8 KB (17,9) (`NatV7`) | vuelve |
| `GuiWinJson` / `GuiArbolG2`: GUI | 38,3 / 40,5 KB (67,3 / 71,5) | 44,6 / 46,0 KB (74,3 / 78,7) | vuelve |
| `NeoUna` + `NeoTres`: dos tiras NeoPixel | 3,1 KB la primera, con el módulo (3,9) | | vuelve casi todo: quedan ~0,1 KB (~1,5), sin crecer al repetir; pero el bloque mayor se queda partido, de 62 a 57 KB (de 64 a 56), hasta el siguiente reset (`#666`) |
| `PwmDos` + `PulsoTres`: un PWM y un contador de pulsos | 3,2 KB el PWM, con el módulo (3,7) | | vuelve casi todo: quedan ~0,3 KB (~0,4), sin crecer al repetir; el bloque mayor, de 62 a 58 KB (de 64 a 62) (`#666`) |

**La GUI en la C6, desde el 8-oct, con holgura**: en su peor momento deja 36 008 B (GuiWinJson) y 34 592 B
(GuiArbolG2); y tras `PinesSistema` —las tiras que la tumbaban—, IDÉNTICAS, con 35 920 y 34 488 B. Lo que se fue: las
cabeceras de los nombres (`#651`) y las copias de la stdlib que se cargaban en RAM desde `/lib` (`#623`, `#650`): todo
corre XIP del `Stdlib.pack`. **El 7-oct iba al límite**: dejaba 6,5 y 2,2 KB, y con algo más ocupado —las tiras que
dejó un programa anterior, sin más— GuiArbolG2 no cargaba (IDÉNTICO tras un `reset` y ERROR tras `PinesSistema`).

### Por qué un módulo cuesta de 3 a 6 KB aunque venga de un pack

Un módulo que se ejecuta desde la zona de packs (XIP) no copia su código a la RAM, pero el cargador guarda en C los
nombres de lo que importa, de sus dependencias y de sus referencias —**una copia por nombre, un bloque cada una**—, y
el enlazador su tabla de símbolos. Censado en el PC (el guardián de `#339` con el programa cargado, antes de
liberarlo; punteros de 8 bytes):

| programa | módulos | bloques vivos | bytes |
|---|---|---|---|
| `Nada` | 2 | 14 | 14 590 (9 192, la estructura de la VM) |
| `Importa5` | 8 | 437 | 36 929 |
| `GuiWinJson` | 6 | 818 | 54 549 |
| `GuiArbolG2` | 6 | 884 | 56 457 |

En la placa, cada bloque lleva además **32 B de cabecera del guardián de `#339`** (`bpvm_blk_t`: quién lo pidió, para
señalarlo si se fuga) y la del heap del IDF: con ~850 bloques, unos 30 KB sólo de cabeceras (**estimación**: el
desglose en la placa está por medir). Es la mayor parte de lo que se lleva la GUI en la C6 —el pool de LVGL está en el
bloque de los packs, fuera de esta cuenta—. Los caminos para recortarlo, en `#651`.

**Desde el 8-oct (`#651`), un bloque por módulo para sus nombres.** El censo por quién pide cada bloque (`make
censo-carga`) decía que de los 818 de `GuiWinJson`, 770 eran nombres sueltos del cargador. Ahora, los mismos bytes en
muchos menos bloques:

| programa | módulos | bloques vivos | bytes |
|---|---|---|---|
| `Nada` | 2 | 14 | 15 751 |
| `Importa5` | 8 | 67 | 37 048 |
| `GuiWinJson` | 6 | 54 | 54 696 |
| `GuiArbolG2` | 6 | 54 | 56 543 |

En la placa, ~760 cabeceras menos con la GUI (medido en la C6 el 8-oct, junto con `#623` y `#650`: la GUI, 29–31 KB
menos en uso; `IO`, de 5,4 a 3,0 KB). Lo que queda, por tamaño: la estructura de la VM
(9 KB), los trozos de 2 KB de la tabla de símbolos (18 KB con la GUI), las tablas de referencias (8,6 KB) y la tabla de
símbolos (8 KB). `test-censo`, en la red, no deja pasar de 100 bloques cargado.

## 4. Para programar en la C3 y la C6

- **Importa sólo lo que uses**: cada módulo, unos 3–4 KB de RAM del sistema mientras corre el programa (medido en la
  C6 el 8-oct: `IO`, 3,0 KB; cinco de la stdlib, 21,5 KB; el 7-oct, 5,4 y 36), aunque se ejecute desde un pack: el
  pack ahorra el código, no las tablas.
- **La GUI en la C6 deja ~35 KB** en su peor momento (8-oct; el 7-oct, 2–7 KB), también tras un programa que tomó
  periféricos: se sueltan al acabar.
- **Las natives** se llevan su código a la RAM ejecutable mientras corre el programa (`NatV7`: 16,8 KB en el peor
  momento); al acabar, vuelve.
- **NeoPixel, PWM y contador de pulsos**: desde V7 (`#667`, 8-oct) se SUELTAN al acabar el RUN —bien, con un
  error o con un STOP—: sus objetos los destruyen y lo que no, la red del final; el siguiente programa los vuelve
  a tener (probado en placa: las ranuras del LEDC, los canales RMT y las máquinas de la PIO). La RAM, medida en la C6
  el 8-oct: vuelve toda menos ~0,1 KB tras las tiras y ~0,3 KB tras el PWM y el contador, sin crecer al repetir;
  pero el bloque mayor se queda partido (4–5 KB menos) hasta el siguiente reset. La C3, por remedir.
- **Los ficheros** cuestan poco (el 7-oct, ~0,2 KB abiertos; desde `#668` su destructor los cierra antes de la línea
  `mem:`); el módulo `IO`, 3,0 KB. **Un objeto con destructor vivo al acabar** —un fichero, un periférico, un
  componente de la GUI— cuesta ~2,1 KB de RAM del sistema en la destrucción del final (`#668`; medido con
  `ConDestructor`, sin `IO`: la serie `destructor` de la herramienta), que vuelven al liberar el programa.

## 5. La C3

- SRAM: 400 KB, sin PSRAM. **Sin bloque de packs** (`packRamStatus off`: no tiene pantalla).
- **V6** (2-sep, U6.4; `esp32c3/main/chip_cfg.h`): 280 KB libres al arrancar, pero el bloque mayor era de 136 KB —el
  techo de la VM es el bloque contiguo, no la RAM—; la VM, 128 KB (64 + 64); quedaban 148 956 B, y el sistema llegó a
  gastar 17 588 B más (su margen).
- **El 6-oct** (el lote de H12, su `INFO`): el bloque de la VM era de 112 KB —**48 de heap de BP** y 64 de pilas—, no
  los 128 de su objetivo (el arranque lo baja de 4 en 4 KB si no cabe con su margen: el porqué, en su log de arranque),
  y el mínimo histórico del heap del sistema, 122 984 B.
- **Medido el 7-oct** (`tools/medir_consumo.py`; la serie de GUI no aplica: no tiene pantalla). **En reposo, tras un
  programa vacío: 108 684 B libres, bloque mayor 86 016 B** —28 KB más que la C6: no tiene el bloque de los packs—.
  Lo que gasta un programa es lo de la C6, al centenar de bytes: el mismo núcleo y los mismos drivers del IDF.

| programa | en uso al acabar | peor momento | al acabar el RUN |
|---|---|---|---|
| `Importa5`: cinco módulos más de la stdlib | 36,7 KB | 43,8 KB | vuelve |
| `Limpia`: importa `IO` | 5,6 KB | | vuelve |
| `FicheroAbierto`: `IO` y un fichero abierto | 5,8 KB (el fichero, ~0,2 KB) | | vuelve |
| `NatV7` / `FloatNat`: natives | 13,7 / 3,4 KB | 18,2 KB (`NatV7`) | vuelve |
| `NeoUna` + `NeoTres`: dos tiras NeoPixel | 4,0 KB la primera (con el módulo) | | **no vuelve: ~1,5 KB, y el bloque mayor, de 86 a 78 KB** (`#666`; el 7-oct, antes de `#667`: por remedir) |
| `PwmDos`: un PWM | 3,8 KB (con el módulo) | | **no vuelve: ~0,2 KB** (`#666`; el 7-oct, antes de `#667`: por remedir); la C3 no tiene contador de pulsos (`PulsoTres` lo atrapa) |

  En la C3 lo justo es el **heap de BP** (48 KB), no la RAM del sistema: GcHondo, OwnerHondo y synclisttest se saltan
  en su ronda por memoria.

## 6. Cómo medir

    python bpgenvm-c/tools/medir_consumo.py COMx [serie …] [--informe ruta.md]

Por cada serie, un `reset` (suelta todo) y sus programas en orden (`tools/consumo/` y los de la ronda de V7), uno por
tanda; tras cada uno lee su línea `mem:` del log. Ni graba ni toca el ENV. Sólo las ESP (la línea es suya), con
`log=1`. Y el margen del sistema —lo que el sistema gasta fuera de la VM—, con `tools/medir_margen.py`.

## 7. El 27-oct: las placas sin PSRAM, remedidas por la imagen

> Eduardo, 27-oct: *«Antes de cerrar la VM cuanta memoria RAM consume y si hay posibilidades de reducir el consumo. Ya lo miramos en su día pero no se si con los cambios que ha habido se puede hacer algo. Solamente para los micros que no tengan PSRAM, los que tienen PSRAM no hay problema.»* Sin placas conectadas: el `INFO` del lote de esa tarde
> (`compat/informes/lote_v8_27oct.md`, las imágenes de ese día) y los `.elf` (`_heap_start` y los símbolos por
> tamaño, con el `nm` de cada cadena). Lo que se puede hacer, en `#740`.

| | C3 | C6 | Pico 2 |
|---|---|---|---|
| SRAM | 400 KB | 512 KB | 520 KB |
| la imagen (código en IRAM, datos y búferes) | 126 KB | 135 KB | 123 KB (con los 24 KB del heap de FreeRTOS) |
| el bloque de la VM | 112 KB | 125 KB | ~285 KB |
| · el heap de BP | 48 KB | 61 KB | 214 KB |
| · las pilas de los hilos | 64 KB | 64 KB | ~71 KB |
| el bloque de los packs (la GUI) | — | 136 KB | — |
| la RAM del sistema, mínimo libre en el lote | 79 KB | 72 KB (33 con la GUI) | 64 KB para malloc; FreeRTOS, 6,6 KB libres de 24 |

La Nucleo (384 KB de heap de BP) y la DK2 (768) van holgadas: las justas siguen siendo la C3 y la C6.

- **Lo que cambió desde el 7–8 de octubre: poco.** La imagen de la C6, +1,6 KB (`_heap_start`, de `0x408215d0` a
  `0x40821c30`): su bloque de la VM baja de 127 a 125 KB —el heap, de 63 a 61— y la GUI tiene ~3 KB menos de holgura
  en su peor momento (de 36 008 a 32 596 B libres). La C3, igual: 48 KB de heap. El sonido (V8/H4), ~0,4 KB.
- **Adónde va la imagen** (la C6; la C3, casi igual): el código en IRAM, 43 KB (38 en la C3); unos 70 KB de búferes
  estáticos nuestros —la pila de la tarea `run` (10 KB), el log (8 KB, en `.noinit`), el espejo del ENV (8 KB), el
  búfer del PUT (8 KB), el de trabajo (`g_scratch`, 8 KB), el de ficheros (4 KB), los tres de líneas del wire
  (6 KB), los tres `tiene` del enlace (4,2 KB) y otros menores—; y el resto, el IDF y FreeRTOS.
- **Las pilas de los hilos BP se llevan, en la C3 y la C6, más que el heap:** el reparto da el 25 % del bloque, pero
  nunca menos de 64 KB —el main, 16, y 24 hilos de 2 KB—, y se reservan aunque el programa no cree ningún hilo. Con
  las pilas sacadas del heap (`#740`), el heap de la C3 pasaría de 48 a 96 KB y el de la C6, de 61 a 109.
- **Hecho en el código el 29-oct (`#740`):** la región de pilas es la pila del main, 16 KB, y la de cada hilo, un
  bloque del heap que pide al arrancar y barre el GC cuando acaba. Las cifras de la tabla, las del 27-oct: las nuevas,
  medidas en el lote (la línea `vm: memoria … -> heap … + pilas 16 KB` del log, y el `INFO`).
- **Y lo barato (`#740`, 2), el mismo día:** el log de la C3 y la C6, de 8 a 4 KB, y los tres `tiene` del enlace en
  uno. En el ELF: 6.896 B menos de RAM estática (`.noinit` −4.096, `.bss` −2.800); con los ~460 B del sonido, que
  entró después, `_heap_start` baja 6.432 B en las dos. En la C6, su bloque de la VM puede volver a sus 128 KB (el
  tope) y el resto queda para el sistema; en la C3, lo que dé su bloque contiguo mayor: en la placa.

## El Bluetooth en todas las imágenes (8-nov)

Lo común del Bluetooth (`bpvm_bt.c`, V8/H2) entra en todas las imágenes, también en las que no tienen radio: **156 B
de RAM fija** (medido en la Pico: 96 de datos, el texto del motivo, y 60 de `.bss`) y 3 KB de flash. Lo grande no está
fijo: las listas de lo encontrado y lo conectado (~1,4 KB) se reservan al encender el Bluetooth la primera vez, en una
placa con radio, y el búfer de `/sys/bluetooth.json` (1,5 KB), sólo mientras se lee.

Y el teclado del SO (`bpvm_teclado.c`, V8/H2, paso 2, 9-nov), igual: **12 B de RAM fija** (medido en la Pico) y 1,5 KB
de flash; su estado (~0,7 KB: el búfer, los avisos y los teclados apuntados), la primera vez que se usa. Y el ratón del
SO (`bpvm_raton.c`, el paso 3, el mismo día), lo mismo: **12 B de RAM fija** (medido en la Pico) y 1,2 KB de flash; su
estado (528 B: la posición, los avisos de los botones y la rueda, y los ratones apuntados), la primera vez que se usa.
Con el paso 4 (10-nov), los lectores de C de la GUI: 16 B más en el estado del ratón y 32 en el del teclado; la RAM
fija, igual (medido en la Pico: 12 B cada uno).

**La radio del Bluetooth en la C5** (NimBLE, `esp32/common/bt_esp32.c`; V8/H2, paso 5, 11-nov), medida en la placa con la
WiFi conectada y la línea `mem:` del log (el final de cada RUN, con un programa vacío, `Nada`):

| | DRAM interna libre al acabar | mínima desde el arranque |
|---|---|---|
| la imagen de antes, sin el Bluetooth | 87 KB | 52 KB |
| con el Bluetooth en la imagen, apagado | 61,8 KB | 26,1 KB (el pico de conectarse la WiFi) |
| con el Bluetooth encendido (`autoStart`) | **35,6 KB** | **15,9 KB** (las dos radios arrancando) |

- **Enlazarlo cuesta 26 KB de RAM interna fija**, aunque no se encienda: tras arrancar la VM quedan 118.879 B en vez de
  144.923. Es casi todo código del controlador que corre desde la IRAM (`libble_app.a`, 20,7 KB; `libbt.a`, 4,2 KB), y en
  la C5 la IRAM y la DRAM salen de la misma SRAM.
- **Encendido, otros ~26 KB**, del controlador; al apagarlo vuelven todos menos 1,6 KB. Recortar a la mitad los búferes de
  NimBLE (las dos bolsas de mensajes, los paquetes de datos y los eventos) no movió nada: van a la PSRAM
  (`CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL`).
- En flash, +339 KB: la imagen, 1,82 MB de una partición de 2 (quedan 277 KB).
- **Decidido, seguir así** (Eduardo, 11-nov: *«la C5 tiene PSRAM, la SRAM es RAM de trabajo pero no nos importa mucho
  porque la VM con quien realmente trabaja es con la PSRAM. Así que la opción 1 está bien»*). Lo otro, por si un día
  hiciera falta: `CONFIG_BT_CTRL_RUN_IN_FLASH_ONLY`, que pasa a la flash la mayor parte del código del controlador
  (~20 KB fijos), y que Espressif marca como experimental (la radio, más lenta; y una conexión puede caerse mientras se
  escribe en la flash).
- **Y después, conectando** (el segundo tramo, el mismo día), salió que **no es sólo cuánta, sino que sea contigua**: con la
  pila de la tarea de NimBLE a 8 KB, el controlador no pudo empezar una búsqueda (el error 0x07 del HCI, «Memory Capacity
  Exceeded»; con 4, sí); y con la WiFi conectada y un aparato conectado (18 KB libres), no puede empezar la conexión del
  segundo (con la WiFi apagada, 46 KB, sí). La tarea del Bluetooth de lo común, 6 KB más. **Decidido** (Eduardo): para
  la C5, recortar los búferes de la WiFi en la RAM interna (*«c»*); se hará después de dejar el Bluetooth funcionando en
  la P4, que no tiene este problema.
- **Hecho el 13-nov** (`esp32c5/sdkconfig.defaults`; `V8_IDEAS` §E1, «El paso 5 en la C5»). Con la imagen de ese día y la
  WiFi conectada, ni el primer aparato emparejaba (el cifrado, sin memoria). La *«c»* sola —6 búferes de recepción en vez
  de 10, 6,4 KB menos con la WiFi encendida, y 16 tramas de envío en vez de 32— no bastó: emparejaban y volvían los dos,
  pero al usarlos el driver de la WiFi tiraba las balizas por falta de memoria (`wifi:m f null`) y hubo un pánico. Con
  ella, el código de la WiFi fuera de la IRAM (tres de sus optimizaciones de velocidad; la del sueño se queda):
  **13,4 KB fijos más**, aunque la WiFi no se encienda (`_heap_start`, de `0x4083f660` a `0x4083c0e0`).

  | la C5, con la WiFi conectada | la mínima tras arrancar (`INFO`) | libre al acabar, con los dos aparatos en uso |
  |---|---|---|
  | la imagen del 13-nov, sin recortes | 28,6 KB | — (no empareja) |
  | con la *«c»* | 35,7 KB | 18,7 KB, y la WiFi sin sitio para las balizas |
  | con la *«c»* y el código de la WiFi fuera de la IRAM | **49,3 KB** | **31,4 KB** (27 KB con la GUI) |

  Subir y bajar 1 MB por la WiFi tarda lo mismo con los recortes que sin ellos (tres veces cada uno, el mismo rato).
- **La pantalla SPI, sin RAM interna** (`#818`, el mismo día): cada refresco pedía un búfer de rebote interno de hasta
  11,5 KB, y con el Bluetooth y la WiFi no lo había. Ahora el DMA lee de la PSRAM (en la S3, la C5 y la P4).
- **Ojo con el «bloque mayor» de la línea `mem:`:** en la C5 es la RTC RAM (15.360 B, siempre el mismo número), que está
  en el montón como interna pero no vale para DMA. Para saber si cabe un búfer de DMA, ese número no sirve.

**La radio del Bluetooth en la P4** (el mismo `bt_esp32.c`, con el controlador en su C6, por `esp_hosted`; el paso 5,
11-nov), medida en la placa con la WiFi conectada: la mínima desde el arranque (`rtosHeapMinFreeBytes`, del `INFO`) y la
línea `mem:` del log.
- **Enlazarlo cuesta 3 KB de RAM interna**: la mínima, de 173,4 KB a 170,3, con el Bluetooth apagado. El controlador está
  en la C6, y lo de NimBLE va a la PSRAM.
- **Encendido, con el teclado y el ratón conectados, otros 22 KB en el peor momento**: la mínima, 148,1 KB; al acabar el
  programa, 203,1 KB libres (con la radio apagada, 209,2). Los búferes de NimBLE, los de por defecto.
- **Lo que mandan, al teclado y al ratón del SO** (`bpvm_hid.c`, la pieza 2, el mismo día): 3,8 KB de código en la P4 y
  4 B de RAM fija (los bloqueos). Con cada aparato, lo que dice su descriptor y lo que tenía apretado (unos 430 B), en
  la tabla de los aparatos de la radio, que va a la PSRAM (8 sitios: 3,4 KB); y el descriptor entero (hasta 1 KB, en la
  PSRAM), sólo mientras se lee. El estado del ratón del SO, 8 B más (552 B: la pantalla). Con la WiFi y los dos
  conectados, la mínima de la RAM interna, 144,9 KB (con `HidPrueba`; antes, con `BtMira`, 148,1).
- **Y en la GUI** (la pieza 3, 12-nov): la tabla `'ENTR'` de la imagen, sólo constantes en la flash; el pack de la GUI,
  6 KB más de código y 1 KB más de RAM fija, en su bloque de la PSRAM (la flecha del ratón, 912 B, y el estado de la
  entrada). Con la WiFi, los dos aparatos y una pantalla con controles, la mínima de la RAM interna, 139,5 KB.
- **Y el arranque y el apagado por falta de uso** (la pieza 5, 12-nov): 16 B más de RAM fija en lo común del Bluetooth,
  en todas las imágenes (medido en la P4, `_heap_start`).

**La radio del Bluetooth en la S3** (el mismo `bt_esp32.c`, con el controlador del chip, como la C5; 14-nov), medida con el
`INFO` y la línea `mem:` del log, con la WiFi conectada:
- **Enlazarlo cuesta 24,8 KB de RAM interna fija** (`_heap_start`, de `0x3fcbe790` a `0x3fcc4a90`; el código del
  controlador en la IRAM, +21,6 KB): la mínima tras arrancar, de 63,7 a 37,4 KB. Y 206 KB de flash.
- **Emparejados el teclado y el ratón, la mínima bajó a 7,8 KB**, y a la pila de NimBLE le quedaban 1,4 KB de 4. Con los
  recortes de la C5 (6 búferes de recepción y 16 de envío, el código de la WiFi fuera de la IRAM: 18 KB fijos) y la pila
  de NimBLE a 6 KB: la mínima tras arrancar, 61–62,6 KB; y con los dos conectados, 35,7 KB libres al acabar.
- **Pero mientras se conectan, la mínima vuelve a bajar a 7,9 KB** (`#821`): un pico de unos 28 KB por encima de lo que
  queda después, sin fallos. Sin la consola de la S3 delante (va por su otro USB), no se sabe de quién es.

## La WiFi en la C3 y la C6 (6-nov)

Sus imágenes no la llevan, y no la llevarán (Eduardo: *«C3 y C6 sin wifi»*). Enlazarla costaría, aunque no se encendiera nunca, +54 KB de RAM fija en la C3 y +88 KB en la C6; con todas las opciones de ahorro, +33 y +60 KB. La S3 pone sus búferes en la PSRAM, y estas dos no tienen. Las
cifras, en `V8_IDEAS.md` §E3, «La C3 y la C6, sin PSRAM».
