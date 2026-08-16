# Censo de las familias — qué tiene cada una, qué le falta, qué divergió

> **`#427`, 17-ago-2026.** Decisión de Eduardo: *«revisar todas las familias e
> imágenes; eso nos daría un censo real de cómo está el código. La unificación
> y racionalización es tarea de V6, pero lo que vayamos adelantando bienvenido
> sea»*. **Esto es el CENSO, no la unificación**: fotografía + hallazgos. Lo que
> salga en rojo se decide ficha a ficha.
>
> **Método**: mecánico, no de memoria. Cada dato dice de dónde sale. Lo motivó
> una racha medida: CUATRO fallos del tipo «esto está en una familia y no en
> otra» en dos días (el corte del CRC sólo en el Pico, el log propio del Pico,
> la carga del pack en el arranque del P4, `read_at` sin el fallback de packs)
> — los cuatro encontrados de casualidad, persiguiendo otra cosa.
>
> ⚠️ **Caveat del STM32**: su lista de fuentes sale de `Debug/…/subdir.mk` (el
> último build de CubeIDE), no de un fichero de proyecto legible. Los datos de
> SU columna que resulten raros están marcados 🟡 y hay que contrastarlos
> abriendo el `.cproject` — de hecho el propio censo encontró rarezas ahí
> (compila `fs_host.c`/`net_host.c`, que son de host).

## La foto

| imagen | build | nivel de opt. | fuente del dato |
|---|---|---|---|
| host | `bpgenvm-c/Makefile` | `-O2` | Makefile |
| Pico (RP2350) | CMake + ninja | **`-Os` (Release)** ✔ | CMakeCache |
| S3 (ESP32-S3) | idf.py | **`-Os`** ✔ | `sdkconfig: OPTIMIZATION_SIZE=y` |
| **P4 (ESP32-P4)** | idf.py | 🔴 **`-Og` (DEBUG)** | `sdkconfig: OPTIMIZATION_DEBUG=y` **+ 18 × `-Og` en el log de build** |
| STM32 (U5G9J) | CubeIDE | `-Os` ✔ (era `-O0` toda V4) | `subdir.mk` |
| sim (bpvm-sim) | Makefile | `-O2` | Makefile |

🔴 **EL HALLAZGO GORDO: el P4 —la placa de diario— compila a `-Og`.** Es la
lección de V4 repetida (el STM32 se publicó a `-O0` y se vio de casualidad).
Todas las medidas de estos días (árbol 149 ms, arranque 386 ms, bus a 20 MHz)
se hicieron con optimización de depuración: **una imagen release será más
rápida gratis**. Cambiarlo es una línea del `sdkconfig` — y REPETIR las medidas
de referencia, que ésa es la parte que no es gratis.

## 1 · El común (`src/*.c`, 57 ficheros): quién compila qué

40 de 57 están en las cinco imágenes. Los 17 con huecos, con veredicto:

| fichero | host | pico | s3 | p4 | stm32 | veredicto |
|---|---|---|---|---|---|---|
| `platform_pthread.c`, `gui_display_sdl.c` | x | – | – | – | – | ✔ host-only por diseño |
| `fs_host.c`, `fs_lfs_host.c`, `net_host.c`, `comm_host.c` | x | – | (†) | (†) | ✔ – | ✔ host-only. **El 🟡 del STM32, RESUELTO el mismo día**: su `.cproject` SÍ los excluye — era el `subdir.mk` rancio del Debug mintiendo (el instrumento, no el código). †S3/P4 sí llevan `comm_host.c`: verificar |
| `bpvm_sd.c`, `bpvm_sd_blk.c` | x | x | – | – | – | ✔ SD por SPI = sólo Pico |
| `bpvm_blk.c`, `bpvm_blk_sdmmc_cfg.c` | x | x/– | – | x | – | ✔ capa de bloque = donde hay SD |
| `fs_fat.c` | 🔴 **–** | x | – | x | – | 🔴 **el host NO compila FatFs** → no hay oráculo host del backend de la SD; `fat_crc32` (#398) sólo se ha compilado en ARM, nunca ejecutado en host |
| `gui.c` | x | – | – | x | x | ✔ GUI = P4 + STM32 (+host SDL) |
| `bpvm_log.c` | x | 🔴 **–** | x | x | x | 🔴 **el Pico lleva copia propia** (`pico/log.c`, 208 líneas) — confirmado en #423, ahora censado |
| `bpvm_listdir.c` | x | x | x | x | 🔴 **–** | 🔴 **el STM32 no tiene `LIST_DIR`** (ni el .c ni el verbo) → el árbol perezoso (#425) no podrá funcionar allí |
| `bpvm_mdn_scan.c` | x | x | x | x | 🔴 **–** | 🔴 **el STM32 escanea los `.mdn` con SU PROPIO bucle** (`stm32_repl.c:526`) en vez del común — copia divergida |
| `bpvm_bios_fs.c`, `bpvm_npack.c` | x | x | x/– | x | – | el STM32 **no puede alojar un pack nativo** (como la S3, pero lo de la S3 estaba escrito y lo del STM32 NO) |

## 2 · Los verbos del wire (el protocolo dice ser UNO)

18 de 29 verbos están en los cuatro despachadores. Los dispares:

| verbo | pico | esp32 (s3+p4) | stm32 | sim | lectura |
|---|---|---|---|---|---|
| `LIST_DIR` | x | x | 🔴 – | 🔴 – | el STM32 y el sim no listan por directorio |
| `SAVE` | x | x | 🔴 – | x | el botón de guardar del IDE **no hace nada en el STM32** |
| `FORMAT` | x | 🔴 – | x | x | **no se puede formatear un S3/P4 desde el IDE** |
| `RENAME` | x | 🔴 – | 🔴 – | x | renombrar desde el IDE sólo funciona en Pico y sim |
| `RMDIR` | x | 🔴 – | 🔴 – | 🔴 – | sólo el Pico |
| `PROMPT_RESPONSE` | x | x | 🔴 – | 🔴 – | `input()` interactivo no funciona en STM32 ni sim |
| `LOG_DUMP` / `LOG_CLEAR` | x | x | x | 🟡 – | el botón Log del IDE contra el sim |
| `SD_INFO` / `SD_MOUNT` | x | 🔴 – | – | – | conocido (cola de H2): nunca subieron a común — y el P4 **tiene SD** y no los ofrece |
| `BOOTSEL` | x | – | – | – | ✔ por diseño (RP2350) |

## 3 · Los gemelos de nombre (candidatos a copia divergida)

md5 y líneas, medidos:

| rol | estado |
|---|---|
| `json_min.c` | pico ≡ esp32 (md5 idéntico) · 🔴 **STM32 divergido**: 220 líneas frente a 263 — una copia vieja del parser del wire |
| `hello_mod.c` | esp32 ≡ p4 · pico ~igual · 🔴 **STM32: 186 líneas frente a 347** — el Hello embebido es de otra época |
| `wire_v1.c` | 🟡 pico ≠ esp32 (259 vs 252 líneas, md5 distinto) — diff pendiente: ¿deriva o diferencia legítima de transporte? |
| `log.c` | STM32 = cintura fina de 53 líneas sobre el común ✔ (el modelo a seguir) · 🔴 Pico = implementación completa propia (208) |
| `board_mgr_*`, `fs_lfs_*`, `gpio_*`, `platform_*`, `bios_*`, `pack_*`, `aot_funcs*` | ✔ cinturas por diseño (el patrón de la casa: motor único + cintura por micro) |

## 4 · Lo que el censo cazó ya cazado (validación)

Estas ya se conocían y el censo las reencuentra por método — señal de que el
método ve lo que hay: el log propio del Pico (#423), `SD_INFO`/`SD_MOUNT` sólo
en el Pico (cola de H2), la S3 sin `bios_s3.c` (escrito en ESTADO).

## 5 · Y una de ESTA MISMA SEMANA que el censo destapó

🔴 **El `#421` no llegó al STM32.** El detalle del fallo de carga
(`entry.fallo`) se cablea en el REPL del ESP32, en el del Pico y en el sim — y
el del STM32 usa `bpvm_load_entry` pero **nunca lee `entry.fallo`**
(`stm32_repl.c:526`): allí sigue saliendo el «IO error» pelado. Cross-family
miss del 16-ago, mío. *Exactamente la clase de agujero que este censo existe
para pillar.*

## Los rojos, priorizados (decisión ficha a ficha — de Eduardo)

1. **El P4 a `-Og`** — una línea de `sdkconfig` + repetir las medidas de
   referencia. La más barata y la que más devuelve.
2. **`#421` al STM32** — cuatro líneas, el patrón ya existe en tres sitios.
3. **`json_min.c` del STM32 divergido** — el parser del WIRE con una copia
   vieja es un bug de protocolo esperando; igualarlo al de pico/esp32 (que son
   idénticos entre sí) o, mejor, subirlo a `src/`.
4. **Verbos del wire**: decidir cuáles son CONTRATO (y entonces faltan en
   STM32/ESP32: `SAVE`, `FORMAT`, `RENAME`, `RMDIR`, `LIST_DIR`) y cuáles son
   por-placa (`BOOTSEL`, `SD_*`). Hoy esa lista no está escrita en ningún sitio.
5. **`fs_fat.c` sin oráculo host** — dar de alta el backend FAT en el host
   (sobre imagen, como `--fs=lfs:`) para que la SD tenga paridad comprobable.
6. **El escaneo `.mdn` propio del STM32** → migrar a `bpvm_mdn_scan.c` común.
7. **El log propio del Pico** → migrar a `bpvm_log.c` (el STM32 enseña cómo:
   cintura de 53 líneas).
8. **`hello_mod.c` del STM32** → regenerar del mismo fuente que las demás.
9. ~~🟡 Verificar la columna STM32~~ — **HECHO el mismo día, y con premio**: el
   `.cproject` ya excluía los de host (el `subdir.mk` rancio mentía), pero al
   compilar de verdad salió que **el `cleanBuild` del STM32 estaba ROTO**:
   `fs_fat.c` entró en `src/` en V5/H2 y nadie lo excluyó. Curado en los dos
   proyectos (`ec81afc`), build headless 0 errores. Queda el † del ESP32
   (`comm_host.c` en S3/P4).

**Alcance**: 1, 2, 3 y 8 caben en V5 — **y 1, 2 y 3 se hicieron el mismo día**
(`ec81afc`): el P4 a `-Os` (fijado en `sdkconfig.defaults` con su porqué), el
`#421` al STM32 y el `json_min` resincronizado (los tres md5 idénticos), todo
verificado con el build headless. Queda el 8 (`hello_mod`, necesita el pipeline
de embebido). 4, 5, 6, 7 son unificación → V6.

## Por qué el STM32 está donde está (Eduardo, 17-ago — no estaba escrito)

*«El STM32 está atrasado por muchas razones; la principal es que la única placa
que tengo con SD en esa familia es de un micro que no tenemos soportado, así
que implica hacer el desarrollo para el micro, añadir soporte de SD, packs
nativos y SQLite. Demasiado para esta versión y por eso lo aplazamos a V6.»*

O sea: el atraso no es descuido, es **alcance decidido**. Lo razonable y chico
se hace en V5 (el `#421`, el `json_min`); la puesta al día de la familia es V6.

## El censo de V6 — la ampliación (especificación de Eduardo, 17-ago)

Este documento es la **base**; en V6 se amplía a un censo FUNCIONAL:

*«Haremos una lista con todo lo que existe actualmente a nivel funcional: Boot,
SD, memoria, FS, Fat32, GC, Packs, VM, drivers de hardware, etc. Lo que
revisaremos es si están implementados, si es código específico o común, si
respetan las relaciones entre módulos (por ejemplo que la VM no llame al HAL
directamente, para evitar que no sea hardware-independiente). Consumos de
memoria y tiempos. Con eso podremos detectar anomalías y carencias. Es mucho
trabajo pero tenemos una versión completa dedicada a ello, la V6.»*

Es decir, cuatro ejes por FUNCIÓN (no por fichero, que es lo que mira este
censo): **implementado/dónde** · **específico vs común** · **capas respetadas**
(la VM no toca el HAL: la regla de independencia del hardware) · **memoria y
tiempos medidos**. Este doc aporta el primer eje y medio; los otros dos son el
trabajo de V6.

---
*Generado con: los `CMakeLists`/`Makefile`/`sdkconfig`/`subdir.mk` de cada
imagen, `md5sum` de los gemelos, `grep strcmp(type,…)` de los despachadores, y
el log de build del P4. Nada de este documento sale de la memoria de nadie.*
