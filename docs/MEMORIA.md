# La RAM de las placas: dónde se va (la C3 y la C6 primero)

> Eduardo, 7-oct-2026: *«Documenta el consumo de memoria. En C3 y C6 es importante ya que vamos muy justos de RAM.»*
>
> **Medido en la C6** el 7-oct con la imagen de ese día (`2609629c`) y `tools/medir_consumo.py`: dos pasadas, las mismas
> cifras al byte. **La C3**, el mismo día y con la misma herramienta (imagen `8ec31ecd`; con `log=1` en su ENV para la
> medida, y `log=0` otra vez después, como estaba). Las fichas: `#651` (la GUI de la C6, al límite), `#666` (lo que no se suelta al acabar un RUN),
> `#667` (los destructores), `#650` (el módulo del pack que no gasta RAM).

## 1. Dos memorias que no hay que confundir

| | dónde está | en la C3 y la C6 | quién la gasta | cómo se mira |
|---|---|---|---|---|
| **el heap de BP** | dentro del bloque de la VM | fijo: 64 KB en la C6, 48 en la C3 (6-oct) | los objetos, strings y arrays del programa; el GC lo recoge | `Heap.getFree()`, `vmHeapBytes` del `INFO` |
| **la RAM del sistema** | el heap del IDF, fuera de la VM | lo que quede: en la C6, ~80 KB en reposo | el sistema, y lo que el núcleo pide en C para el programa: las tablas para cargarlo, los ficheros abiertos, la GUI, las natives, los periféricos | la línea `mem:` del log tras cada RUN (con `log=1`), `rtosHeapMinFreeBytes` del `INFO` (el mínimo histórico), «heap del sistema, mínimo libre» en la tanda |

Un programa puede tener el heap de BP medio vacío y quedarse sin RAM del sistema: «enlace tardío: sin memoria para
construir las clases» en GuiArbolG2 (`#651`) es eso —la tabla de clases del enlace se pide en C, de una vez, y no
encuentra un bloque contiguo—.

## 2. El reparto al arrancar: la C6

| | |
|---|---|
| SRAM | 512 KB, sin PSRAM |
| la imagen (código en IRAM, datos y `.bss`) | ~133 KB (`_heap_start` = `0x408214c0`) |
| el IDF, FreeRTOS y el arranque, antes de los packs | ~21 KB |
| **el bloque de los packs** (`packRam=136` en el ENV) | **136 KB**: los estáticos de los motores de la placa —`DriverSpi` y `GuiMini`, 133 600 B, con el pool de LVGL de 64 KB— y 5 KB de arena |
| **el bloque de la VM** | **128 KB**: 64 de heap de BP y 64 de pilas de los hilos. El arranque lo pide si deja al sistema su margen (`CHIP_MARGEN_SISTEMA`, 103 824 B): deja 103 864, 40 B por encima |
| lo que el arranque reserva después | ~23 KB (entre otras cosas, la tarea del programa: 10 KB de pila) |
| **en reposo, tras un programa vacío** | **80 936 B libres; bloque mayor, 65 536 B** |

El log del arranque lo dice (`heap: libre … | mayor …`, `vm: DRAM interna libre … | margen …`, `packs: bloque de
RAM …`). Sin la GUI en pack —sin el bloque de 136 KB—, a la C6 le quedarían otros 136 KB.

## 3. Lo que gasta un programa: la C6

La línea `mem:` se escribe al acabar el RUN y **antes** de liberar el programa: cuenta lo que el programa tenía vivo.
«En uso al acabar» = 80 936 − el libre de esa línea; «peor momento» = 80 936 − el mínimo histórico (sólo cuando ese
programa lo bajó). Todo se compara con un programa vacío (`Nada`: su módulo y `Core`).

| programa | en uso al acabar | peor momento | al acabar el RUN |
|---|---|---|---|
| `Importa5`: cinco módulos más de la stdlib (Math, Str, Collections, Json, Time —y Rtc, que trae Time—), sin tocar nada | 36,0 KB | 42,8 KB | vuelve |
| `Limpia`: importa `IO` | 5,4 KB | | vuelve |
| `FicheroAbierto`: importa `IO` y deja un fichero abierto | 5,6 KB (el fichero, ~0,2 KB) | | vuelve: lo cierra el fin de RUN |
| `NatV7` / `FloatNat`: natives (su código, en RAM ejecutable mientras corre el programa) | 13,5 / 3,2 KB | 17,9 KB (`NatV7`) | vuelve |
| `GuiWinJson` / `GuiArbolG2`: GUI | 67,3 / 71,5 KB | 74,3 / 78,7 KB | vuelve |
| `NeoUna` + `NeoTres`: dos tiras NeoPixel | 3,9 KB la primera (con el módulo) | | **no vuelve: ~1,5 KB, y parte el heap —el bloque mayor, de 64 a 56 KB—** (`#666`; el 7-oct, antes de `#667`: por remedir) |
| `PwmDos` + `PulsoTres`: un PWM y un contador de pulsos | 3,7 KB el PWM (con el módulo) | | **no vuelve: ~0,4 KB; el bloque mayor, de 64 a 62 KB** (`#666`; el 7-oct, antes de `#667`: por remedir) |

**La GUI en la C6 va al límite**: en su peor momento deja 6,5 KB (GuiWinJson) y 2,2 KB (GuiArbolG2) de los 80. Con
algo más ocupado —las tiras que dejó un programa anterior, sin más— GuiArbolG2 no carga: medido el 7-oct, con la
misma imagen, IDÉNTICO tras un `reset` y ERROR tras `PinesSistema` (`#666`).

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

En la placa, ~760 cabeceras menos con la GUI (a medir en la C6). Lo que queda, por tamaño: la estructura de la VM
(9 KB), los trozos de 2 KB de la tabla de símbolos (18 KB con la GUI), las tablas de referencias (8,6 KB) y la tabla de
símbolos (8 KB). `test-censo`, en la red, no deja pasar de 100 bloques cargado.

## 4. Para programar en la C3 y la C6

- **Importa sólo lo que uses**: cada módulo, de 3 a 6 KB de RAM del sistema mientras corre el programa (medido:
  `IO`, 5,4 KB; cinco de la stdlib, 36 KB), aunque se ejecute desde un pack: el pack ahorra el código, no las tablas.
- **La GUI en la C6 deja 2–7 KB** en su peor momento: un programa de GUI que importe más módulos, o que corra tras uno
  que dejó periféricos tomados, puede no caber.
- **Las natives** se llevan su código a la RAM ejecutable mientras corre el programa (`NatV7`: 18 KB en el peor
  momento); al acabar, vuelve.
- **NeoPixel, PWM y contador de pulsos**: desde V7 (`#667`, 8-oct) se SUELTAN al acabar el RUN —bien, con un
  error o con un STOP—: sus objetos los destruyen y lo que no, la red del final; el siguiente programa los vuelve
  a tener (probado en placa: las ranuras del LEDC, los canales RMT y las máquinas de la PIO). Lo que vuelve de la
  RAM, por remedir en la C3 y la C6 (las filas de arriba son del 7-oct, de antes).
- **Los ficheros** cuestan poco (~0,2 KB abiertos); el módulo `IO`, 5,4 KB.

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
