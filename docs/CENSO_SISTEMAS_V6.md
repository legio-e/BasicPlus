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

---

# Paso 2 — cuántos están unificados, cuáles son HAL BP, y qué cuesta cada uno

> Tres preguntas de Eduardo (23-ago): *«de los 32, ¿cuántos están unificados de verdad?
> ¿Cuántos pertenecerían al HAL BP —donde la implementación puede diferir, pero la
> interfaz exportada debería ser común—? Y de los que no lo están, ¿cuáles se unifican
> fácil y cuáles son más complicados?»*
>
> 🔬 **La prueba que se usa, y por qué es mecánica:** un sistema con código por familia
> está bien construido si ese código **implementa un contrato declarado en una cabecera
> común**. Se comprueba mirando qué cabeceras incluye cada `.c` privado. Si no incluye
> ninguna común, no hay HAL BP: hay tres implementaciones sueltas que casualmente hacen
> algo parecido. Los 36 ficheros privados de las tres familias se pasaron por esa criba.

## El resumen, en números

| grupo | qué significa | cuántos |
|---|---|---|
| **A · Unificados de verdad** | todo el código en `src/`, cero por familia | **15** |
| **B · HAL BP correcto** | implementación por familia + **contrato común** | **6** |
| **C · HAL BP con el contrato roto** | debería ser B, pero la interfaz no es común (o falta en alguna familia) | **4** |
| **D · Duplicación pura** | **no son hardware**: deberían ser 100 % comunes y no lo son | **5** |
| **E · Casos aparte** | arranque y red | **2** |

**Respuesta corta:** unificados de verdad, **15 de 32**. Bien resueltos como HAL BP, **6
más** → **21 de 32 están donde deben**. Los **11 restantes** son el trabajo de V6, y sólo
**4** de ellos son de verdad hardware.

## A · Unificados de verdad (15)

Intérprete · Builtins · Excepciones · Enlazado · Cargador · Heap-reserva · Heap-GC ·
Tabla de handles · Planificador · Hilos · Eventos · Fachada de ficheros · FAT ·
`LIST_DIR` · pegamento de SQLite.

Con dos asteriscos que el número esconde:

- 🟡 **La tabla de handles está unificada pero no es un módulo**: no tiene fichero ni
  cabecera, vive repartida por cinco `.c`. Unificada por omisión, no por diseño.
- 🟡 **`LIST_DIR` y FAT son comunes pero no universales**: el STM32 no compila el primero
  y el host no compila el segundo. El código es único; la cobertura, no.

## B · HAL BP correcto — 6 sistemas

Aquí la implementación **debe** diferir, y el contrato ya es común. Es el modelo a imitar:

| sistema | contrato común | lo implementan |
|---|---|---|
| **Plataforma / RTOS** | `bpvm_platform.h` | `platform_freertos.c` · `platform_esp32.c` · `platform_stm32.c` · `platform_pthread.c` |
| **littlefs** | `bpvm_fs_lfs.h` | `fs_lfs_pico.c` · `fs_lfs_esp32.c` · `fs_lfs_stm32.c` |
| **Packs** | `bpvm_pack.h` | los tres `board_mgr_*.c` |
| **Entorno / gestor de placa** | `bpvm_bmgr.h` | los tres `board_mgr_*.c` |
| **Drivers de periférico** | `bpvm_gpio.h`, `bpvm_spi.h`, … | `gpio_esp32.c` · `gpio_stm32.c` |
| **GUI** | `bpvm_gui.h` | `gui_display_ltdc.c` · `gui_display_sdl.c` |

📌 **El dato que más tranquiliza del censo:** las tres familias implementan `littlefs`,
`plataforma`, `packs` y `gestor de placa` **contra la misma cabecera**. Eso es
exactamente lo que Eduardo describe como HAL BP, y ya funciona en cuatro sistemas.

## C · Debería ser HAL BP, pero el contrato no es común — 4 sistemas

| sistema | qué falla | qué falta |
|---|---|---|
| **BIOS** | `bpvm_bios.h` existe y **sólo la Pico lo implementa** | escribir `bios_esp32.c` y `bios_stm32.c` — es **añadir**, no refactorizar |
| **Particiones / flash** | `bpvm_part.h` es común, pero `flash_lock.c` y `stm32_flash.c` **no incluyen ninguna cabecera común** | colgarlos del contrato que ya hay |
| **SD / capa de bloque** | contrato común (`bpvm_sd.h`, `bpvm_blk.h`), pero **el STM32 no lo tiene** | implementarlo — trabajo nuevo, no unificación |
| **Wire / transporte** | 🔴 los tres transportes (`wire_v1.c` ×2, `stm32_wire.c`) **no incluyen `bpvm_comm.h`** | y hay algo peor, abajo |

🔴 **El hallazgo feo:** `pico/wire_v1.c` y `esp32/main/wire_v1.c` se llaman igual, dicen
ser la misma versión del protocolo… y **el 100 % de sus líneas difieren**. No son una
copia divergida: son dos programas distintos con el mismo nombre. Antes de unificarlos
hay que entender por qué, porque un nombre que miente es peor que dos nombres distintos.

## D · No son hardware y aun así están duplicados — 5 sistemas

Éste es el grupo que justifica V6. **Nada de esto depende del silicio:**

| sistema | duplicación | lo que dice la medida |
|---|---|---|
| **REPL** | `repl_v1.c` 2054 + `repl_esp32.c` 1344 + `stm32_repl.c` 920 = **4.318 líneas** | 🔴 **no existe `bpvm_repl.h`**: no es que el contrato esté roto, es que no hay contrato |
| **Log** | `pico/log.c` 280 + `log_esp32.c` 103 + `stm32/log.c` 60, sobre 215 comunes | 🟢 **el STM32 SÍ incluye `bpvm_log.h`** — el contrato existe y ya está probado en una familia |
| **Utilidades (JSON)** | `json_min.c` ×3 | 🟢 **0 % de diferencia: byte-idénticos** |
| **AOT / escaneo de `.mdn`** | el STM32 usa un bucle propio en `stm32_repl.c:526` | el común (`bpvm_mdn_scan.c`) ya existe y lo usan las otras |
| **stdlib embebida** | 16 ficheros `*_mod.c` en la Pico frente a **uno** en ESP32 y STM32 | mismo dato, dos empaquetados |

## E · Casos aparte (2)

- **Arranque** — `bpvm_boot.h` común y la escalera ya es única; lo que queda por familia es
  el `main.c`, que es legítimo… salvo que el de la Pico son **1.582 líneas**, y ahí dentro
  hay cosas que no son arranque.
- **Red** — hoy es casi sólo host (`net.c` 41 líneas frente a `net_host.c` 238). No está
  duplicado: está **sin hacer** en placa. Es carencia, no divergencia.

## Y la respuesta a «cuál es fácil y cuál no»

El coste no se estima a ojo: sale de si **el contrato ya existe** y de cuánto se parecen
las copias.

### 🟢 Fáciles — el contrato ya existe o las copias son idénticas

1. **`json_min`** — tres copias byte-idénticas y las tres cabeceras declaran lo mismo.
   Es mover un fichero. **Cero decisiones que tomar.**
2. **Log** — el contrato `bpvm_log.h` existe y **el STM32 ya lo respeta**, así que está
   probado. Falta que Pico y ESP32 dejen de tener el suyo.
3. **Particiones / flash** — `bpvm_part.h` está; falta colgar de él los dos ficheros
   sueltos.
4. **Escaneo de `.mdn`** — sustituir el bucle propio del STM32 por el común, que ya usan
   las otras dos.

### 🟡 Medias — hay que decidir algo

5. **BIOS en ESP32 y STM32** — no es unificar, es **escribir lo que falta** contra una
   cabecera que ya existe. El trabajo se conoce; el volumen, no.
6. **stdlib embebida** — es un **generado**, no código a mano: se toca el generador, nunca
   el resultado. Unificar el formato es barato; equivocarse de sitio, caro.

### 🔴 Difíciles — hay que diseñar

7. **REPL** — 4.318 líneas sin contrato. Y no basta con mover: hay que **partirlo en dos**,
   porque el transporte sí es hardware y la interpretación de `RUN`/`DIR`/`INFO` no. Es el
   80 % del problema y el 100 % de las asimetrías que nos han mordido.
8. **Wire / transporte** — antes de unificar hay que explicar por qué dos ficheros con el
   mismo nombre difieren al 100 %.
9. **SD en el STM32** y **red en placa** — no son unificación: es funcionalidad que no
   existe. Van al saco de «qué falta», no al de «qué está repetido».

📌 **Lo que sugiere el orden:** los cuatro fáciles caben en sesiones cortas y no requieren
decidir nada — son el arranque natural de V6. El REPL es el trabajo de verdad y conviene
abordarlo cuando haya tiempo seguido, no a ratos.

