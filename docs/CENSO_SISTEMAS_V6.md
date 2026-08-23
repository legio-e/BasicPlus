# Censo de SISTEMAS — paso 1 de V6

> **Qué es esto.** El primer paso del censo funcional de V6: **la lista de sistemas que
> hay hoy en un micro**, entendiendo por sistema lo que definió Eduardo el 23-ago-2026:
> *«una unidad lógica que haga una tarea»*. El `littleFS` es un sistema; la memoria
> quizá sean dos.
>
> **Qué NO es todavía.** No es el censo completo. La especificación de Eduardo (17-ago,
> en `CENSO_FAMILIAS.md`) pide **cuatro ejes por función**: implementado/dónde ·
> específico vs común · **capas respetadas** (la VM no toca el HAL) · memoria y tiempos
> medidos. Aquí está el inventario y el segundo eje a grandes rasgos. Los otros dos son
> el trabajo siguiente.
>
> **Por qué primero esto.** Criterio de Eduardo (23-ago): *«hay que hacerlo paso a paso;
> si hacemos muchas cosas de golpe se presta a confusiones. El primer paso tiene que ser
> censar.»*
>
> 📐 **De dónde salen los datos:** del listado real de fuentes de cada imagen
> (`src/` 57 ficheros · `pico/` 32 · `esp32/main/` 11 · `stm32/port/` 11), del `grep` de
> quién implementa cada interfaz, y del censo por fichero que ya existe en
> `CENSO_FAMILIAS.md`. Nada sale de memoria.

---

## Los 32 sistemas

La columna **común** son ficheros de `src/`; **cintura**, lo que cada familia pone de su
parte. Un sistema sin cintura es enteramente independiente del hardware.

### Núcleo del lenguaje — no debería tener cintura ninguna

| # | sistema | qué hace | común | cintura |
|---|---|---|---|---|
| 1 | **Intérprete** | ejecuta el bytecode | `interp.c` 1986 · `bpvm.c` 1299 | — |
| 2 | **Builtins** | las funciones nativas del lenguaje | `builtins.c` 2980 | — |
| 3 | **Excepciones** | `try`/`catch`, propagación | `exceptions.c` 285 | — |
| 4 | **Enlazado** | resolución entre módulos | `link.c` 222 | — |
| 5 | **Cargador** | lee el `.mod` y lo instala | `loader.c` 447 · `mdn_loader.c` 180 | — |

### Memoria — hoy UN sistema con dos responsabilidades dentro

| # | sistema | qué hace | común | cintura |
|---|---|---|---|---|
| 6 | **Heap: reserva** | `alloc`, `alloc_string`, `free_block`, `release_reserve` | `heap.c` 1121 *(compartido con el 7)* | — |
| 7 | **Heap: GC** | `gc_stw`, `mark`, `sweep`, `table_sweep` | ⟵ el mismo fichero | — |
| 8 | **Tabla de handles** | ref → objeto, con generación | 🔴 **sin fichero propio**: repartida por `bpvm.c`, `builtins.c`, `bpvm_aot_helpers.c`, `bpvm_dbg_wire.c`, `bpvm_util.c` | — |

📌 **Respuesta a la pregunta de Eduardo** (*«la reserva y el GC, ¿es un sistema o dos?»*):
en el **código de hoy son uno solo** — un único `heap.c` de 1.121 líneas con las dos
familias de funciones dentro. Conceptualmente son dos, y hay **una tercera pieza que no
es ninguna de las dos**: la tabla de handles, que no tiene fichero y vive esparcida en
cinco. Es la primera anomalía que saca este censo.

### Concurrencia

| # | sistema | qué hace | común | cintura |
|---|---|---|---|---|
| 9 | **Planificador** | quantos, cambio de contexto | `scheduler.c` 147 · `scheduler_smp.c` 325 | — |
| 10 | **Hilos** | crear, unir, sincronizar | `threading.c` 210 | — |
| 11 | **Eventos** | cola + inyección de frame entre quantos | `events.c` 221 | — |
| 12 | **Plataforma / RTOS** | la capa bajo los hilos | — | `platform_freertos.c` 316 · `platform_esp32.c` 252 · `platform_stm32.c` 102 · `platform_pthread.c` 191 *(host)* |

### Almacenamiento

| # | sistema | qué hace | común | cintura |
|---|---|---|---|---|
| 13 | **Fachada de ficheros** | enruta por prefijo (`/lib`, `/app`, `/sd`) | `fs_facade.c` 405 | — |
| 14 | **littlefs** | FS de la flash interna | `fs_lfs.c` 421 | `fs_lfs_pico.c` 366 · `fs_lfs_esp32.c` 334 · `fs_lfs_stm32.c` 339 |
| 15 | **FAT** | FS de la tarjeta | `fs_fat.c` 630 | — 🔴 *no se compila en host* |
| 16 | **Capa de bloque / SD** | el medio físico bajo FAT | `bpvm_blk.c` 50 · `bpvm_sd.c` 466 · `bpvm_sd_blk.c` 100 · `bpvm_blk_sdmmc_cfg.c` 121 | 🔴 sólo RP2350 (SPI) y ESP32 (SDIO); **STM32 no** |
| 17 | **Listado de directorio** | `LIST_DIR` | `bpvm_listdir.c` 102 | 🔴 **el STM32 no lo tiene** |
| 18 | **Particiones** | mapa de regiones de flash | `bpvm_part.c` 146 | `flash_lock.c`, `stm32_flash.c` |

### Sistemas de la plataforma

| # | sistema | qué hace | común | cintura |
|---|---|---|---|---|
| 19 | **Packs** | contenedor grabable + XIP | `bpvm_pack.c` 760 · `bpvm_npack.c` 153 · `bpvm_bios_fs.c` 131 | dentro de `board_mgr_*.c` en las **tres** familias |
| 20 | **BIOS** | punto de encuentro para el código nativo | `bpvm_bios.c` 157 | `bios_pico.c` — 🔴 **sólo la Pico** |
| 21 | **AOT / nativo** | `.mdn`, `.npk`, registro de funciones | `bpvm_aot_helpers.c` 645 · `aot_registry.c` 66 · `bpvm_mdn_scan.c` 153 | `aot_funcs.c` (Pico) · `aot_funcs_stub.c` (ESP32) — 🔴 **el STM32 escanea con su propio bucle** |
| 22 | **Arranque** | la escalera `KERNEL→PARTITIONS→FS→APP` | `bpvm_boot.c` 70 | `main.c` de cada familia (1582 en la Pico) |
| 23 | **Entorno / gestor de placa** | variables, descripción de la placa | `bpvm_bmgr.c` 132 · `bpvm_env.c` 275 · `bpvm_bmgr_wire.c` 431 | `board_mgr_*.c` ×3 · `board_desc.c` |
| 24 | **Log** | diagnóstico, post-mortem en RAM | `bpvm_log.c` 215 | 🔴 `pico/log.c` 280 · `log_esp32.c` 103 · `stm32 log.c` 60 |

### Comunicación e interfaz

| # | sistema | qué hace | común | cintura |
|---|---|---|---|---|
| 25 | **Wire / transporte** | el canal con el IDE | `comm_common.c` 106 · `bpvm_dbg_wire.c` 302 | `wire_v1.c` ×2 · `stm32_wire.c` · `comm_pico.c` |
| 26 | **REPL** | interpreta `RUN`, `DIR`, `INFO`, `PACK_BURN`… | 🔴 **nada** | 🔴 `repl_v1.c` 2054 · `repl_esp32.c` 1344 · `stm32_repl.c` 920 |
| 27 | **GUI** | LVGL y sus widgets | `gui.c` 1201 | `gui_display_sdl.c` 399 *(host)* · `gui_display_ltdc.c` 160 *(STM32)* |
| 28 | **Red** | sockets | `net.c` 41 · `net_host.c` 238 | — *(hoy casi sólo host)* |

### Periféricos y datos

| # | sistema | qué hace | común | cintura |
|---|---|---|---|---|
| 29 | **Drivers de periférico** | gpio, spi, i2c, uart, pwm, adc, rtc, wdt, pulse, neopixel | 10 ficheros de `src/`, 25–73 líneas cada uno | `gpio_esp32.c` 610 · `gpio_stm32.c` 1049 · `neopixel.c` · `psram.c` |
| 30 | **stdlib embebida** | los módulos que la imagen trae dentro | — | 🟡 **16 ficheros `*_mod.c`** en la Pico frente a **UNO** en ESP32 (4845) y STM32 (4837) |
| 31 | **SQLite (pegamento)** | la regla del bloque de la BD | `bpvm_sqlmem.c` 60 | *(el motor va en un pack)* |
| 32 | **Utilidades** | JSON, CRC, varios | `bpvm_util.c` 161 · `crc32.c` 30 | 🔴 `json_min.c` **×3 copias byte-idénticas** |

---

## Lo que el inventario deja a la vista

Sin entrar todavía en los ejes que faltan, hay siete anomalías que se ven sólo de listar:

1. 🔴 **El REPL no tiene NADA en común** — 4.318 líneas repartidas en tres ficheros que
   hacen lo mismo. Es la mayor violación del criterio de capas y ya explica cuatro
   asimetrías conocidas (ver `FICHAS` §«EL CRITERIO DE CAPAS»).
2. 🔴 **La tabla de handles no es un sistema, es un rastro** — repartida por cinco
   ficheros sin dueño. Casa con la ficha `#432` (dónde debe vivir y de qué tamaño).
3. 🔴 **El log está en las tres familias además del común** — 443 líneas privadas frente
   a 215 comunes.
4. 🔴 **La BIOS sólo la tiene la Pico.** ESP32 y STM32 no publican tabla.
5. 🔴 **Tres sistemas no llegan al STM32**: SD/bloque, `LIST_DIR` y el escaneo común de
   `.mdn` (usa un bucle propio).
6. 🟡 **`json_min.c` está tres veces, byte-idéntico.** Es la unificación más barata que
   existe hoy: cero decisiones que tomar.
7. 🟡 **La stdlib embebida usa dos formatos**: 16 ficheros en la Pico, uno en las otras.
   No es código distinto — es el mismo dato empaquetado de dos maneras, y es de ahí de
   donde sale que «la Pico tenga 32 ficheros».

## Lo que falta para cerrar el censo

- **Eje 3 — capas respetadas.** La regla es de Eduardo: *«que la VM no llame al HAL
  directamente»*. Se puede comprobar mecánicamente (qué incluye cada `.c`), y no está hecho.
- **Eje 4 — memoria y tiempos por sistema.** Nada medido todavía a este nivel.
- **Verificar la presencia real por familia.** Esta tabla dice qué ficheros existen; falta
  contrastar contra lo que **cada build compila de verdad**, que es dato distinto — el
  `#414` salió justo de esa diferencia.

📌 **Un nombre que confunde y conviene arreglar cuando se toque:** `src/pico.c` no es de
la placa Pico — es la fachada del **módulo `Pico` de la stdlib**, el mismo patrón que
`pulse.c` o `pwm.c`. En un directorio común, un fichero con nombre de familia se lee mal.
