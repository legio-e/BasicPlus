# Fundir los tres backends de FreeRTOS — estudio (10-sep-2026)

> **Qué es esto.** El estudio previo de `#473`/R1+R16: hay TRES implementaciones del mismo contrato
> de plataforma sobre FreeRTOS (`src/platform_freertos.c`, que hoy compila SÓLO el STM32;
> `pico/platform_freertos.c`; `esp32/common/platform_esp32.c`). Esto dice qué se puede fundir, en qué
> orden, y qué NO se puede. **No es una decisión tomada**: es el material para tomarla.
>
> Lo hicieron seis lecturas independientes + una pasada de refutación adversarial (52 hallazgos, 33
> sobrevivieron). **Lo que va abajo no está verificado a mano entero** — sí lo están los cuatro
> puntos de más peso, releídos uno a uno:

| comprobado a mano | veredicto |
|---|---|
| **El 32 % de la Pico** — que con el común `PrintBench` pasaba de 4 040 a 5 330 ms | ✅ está escrito en `src/platform_freertos.c:19-24`, con bisección y ±10 ms, y **sin explicar**. Y el `KILL` MEJORÓ de 33 a 2 ms |
| **La trampa de las unidades de pila** | ✅ real. El común dice `BPVM_FR_STACK_IO 1024u` en **palabras** (`:58-59`); el ESP32 usa `BPVM_ESP32_THREAD_STACK_BYTES 4096` en **bytes**, y su propio comentario lo advierte (`platform_esp32.c:13`). Fundir sin el `-D` deja `io` con 1 KB, y el sink declara `char buf[1024]` en pila |
| **El camino de error del SMP que no une** | ✅ real: `src/scheduler_smp.c:303-315` hace `bpvm_free(arg); return -1;` sin unir los workers ya creados |
| **El semáforo del condvar** | ✅ real y en las TRES: `xSemaphoreCreateBinary()` (`src:106`, `pico:77`, `esp32:70`) retiene UN token, y `broadcast` emite N. Sin síntoma observado |

> 📌 **Una corrección al estudio.** Su §4.3 dice que no se sabe si la tarea ociosa recibe turno en la
> Pico y en los ESP. **Ya se sabe, medido el mismo día** (`#473`/R16): en la Pico sí —55 RUNs con la
> marca de agua del heap plana— y en el ESP32 también, porque su lector del wire bloquea cediendo
> CPU. La que NO recibe turno es la del STM32. O sea que la fuga de `vTaskDelete(NULL)` está
> **latente, no viva**, en las dos familias que la conservan.

---

# Fusión de los tres backends de plataforma sobre FreeRTOS — qué se funde, en qué orden, y qué hay que medir

*(Todas las citas se han releído en esta pasada: los tres `platform_*.c` enteros, `include/bpvm_platform.h`, `src/comm_common.c`, `src/bpvm_io.c` y las altas de build. Donde no he podido determinar algo leyendo, lo digo en el §4.)*

---

## 1. Qué se puede fundir y qué NO

### 1.1 Lo genuinamente específico de cada familia (lo único que no se funde)

**Pico / RP2350** — tres símbolos de silicio, y el motivo está escrito y es correcto:
- `bpvm_platform_now_ms` sobre `time_us_64()` (`pico/platform_freertos.c:355-369`) — no puede ser `xTaskGetTickCount()` porque el SysTick deriva de `clk_sys` y `Pico.setCpuFreqMHz()` desajustaría el tiempo en proporción.
- `bpvm_platform_busy_wait_us` (`:371-379`) y `bpvm_platform_random_u32` sobre `get_rand_32()` (`:381-386`).
- Sus dos includes del SDK (`pico/time.h`, `pico/rand.h`, `:23-24`) cubren las tres.
- **Además, el acoplamiento con flash**: `#include "flash_lock.h"` (`:26`), el campo `lockout_victim` (`:191`) y sus usos (`:204-206`, `:303`). Esto ya tiene hueco previsto en el común: el débil `bpvm_platform_thread_arranca(int core_id)` (`src/platform_freertos.c:227-228`, llamado en `:232`).

**ESP32 (S3, C3, C6, P4 — cuatro variantes, una sola copia)** — los mismos tres verbos sobre `esp_timer_get_time()`, `esp_rom_delay_us()` y `esp_random()` (`esp32/common/platform_esp32.c:292-316`), más tres cosas de plataforma que **no** son "cintura de silicio" pero sí son de ESP-IDF:
- el prefijo de includes `freertos/` (`:20-22`) frente al pelado del común (`src/platform_freertos.c:48-50`);
- **la pila en BYTES** (`:171`, `BPVM_ESP32_THREAD_STACK_BYTES 4096`), frente a PALABRAS en vanilla;
- `xTaskCreatePinnedToCore` + `tskNO_AFFINITY` (`:207`), que es el verbo de afinidad del kernel IDF; `xTaskCreateAffinitySet` del común (`src/platform_freertos.c:256`) **no existe** en ese kernel.

**STM32** — ya está bien: los tres verbos viven fuera, en `stm32/port/platform_stm32.c:38,40,72`. Es el patrón a imitar.

### 1.2 Lo que hoy diverge y NO es de familia (esto es lo que la fusión viene a matar)

Todo lo demás de los tres ficheros es FreeRTOS puro: mutex (idéntico línea a línea en los tres), condvar (idéntico salvo `broadcast`), hilos, join, yield, sleep. Las divergencias vivas:

| Divergencia | Común | Pico | ESP32 |
|---|---|---|---|
| Quién borra la tarea | `vTaskSuspend(NULL)` + `vTaskDelete(ft->task)` en join (`src:234-236`, `:318`) | `vTaskDelete(NULL)` (`pico:211`), join no toca la tarea (`:324-331`) | `vTaskDelete(NULL)` (`esp32:184`), ídem (`:261-268`) |
| Prioridad de nacimiento de los hilos BP | `BPVM_FR_PRIO_VM` = IDLE+2 (`src:55-56`, usada en `:277`, `:307`) | IDLE+1 (`pico:226`) | IDLE+1 (`esp32:194`) |
| `broadcast` | bucle: toma el guard N+1 veces, un `give` por vuelta (`src:186-193`) | toma el guard una vez, pone `waiters=0`, N `gives` (`pico:170-178`) | igual que Pico (`esp32:157-165`) |
| Nombres de tarea | `bpvm-thr` / `bpvm-io` / `bpvm-thr-pin` | `bpvm-thread` / `bpvm-io` | **todas** `bpvm-thread` (`esp32:207`) |
| `lockout_victim` en `create_io` | n/a (`fr_spawn` fija los cuatro campos, `src:245-249`) | **sin inicializar** (`pico:257-262`), leído en `:204` | n/a |

El redondeo de tick ya convergió (`a257be8b`, 10-sep) — y es el mejor argumento de la fusión: fue la **cuarta** vez que ese fallo apareció con la misma forma, y el comentario de `pico/platform_freertos.c:345-347` lo dice: *"el arreglo estaba desde el 5-sep en las TRES copias y en el sleep del común; a este no viajó"*.

### 1.3 Lo que NO cambia con la fusión (para no mandar a nadie a mirar donde no hay problema)

- **Quién crea la tarea de la VM**: lo pone cada familia fuera del backend (Pico `main.c:1590-1591`; P4 `main.c:652`/`:657`; STM32 `main.c:191`; S3/C3/C6 corren en la `main` de ESP-IDF, `esp32/common/main.c:192`). Como `io` hereda con `uxTaskPriorityGet(NULL)` desde #485 (`src:295-296`, `pico:275-276`, `esp32:241-242`), el contrato de prioridad de `io` sigue correcto en las cinco después de fundir.
- **Un `Thread` de BasicPlus no crea tarea del RTOS**: el planificador de hilos BP es cooperativo dentro de la tarea de la VM (`src/scheduler.c:117-123`, `:145`). Por eso la prioridad de nacimiento de los hilos BP es hoy latente.
- **El núcleo no entra al backend por debajo del contrato**: el único fichero de `src/` que incluye FreeRTOS es `src/platform_freertos.c:48-50`.
- **El único consumidor vivo del contrato de hilos en firmware es `io`**: `bpvm_platform_thread_create` pelado no lo llama nadie fuera de los propios backends, y `_pinned` sólo se usa en el camino SMP (`src/scheduler_smp.c:309`, `pico/comm_pico.c:108`), apagado por defecto.

---

## 2. El plan, en pasos pequeños y verificables

**Aviso transversal**: la paridad dual-VM (Java vs C-host) **no cubre nada de esto** — el host enlaza `src/platform_pthread.c` (`Makefile:34`). Aquí la verificación es *compilar las cinco imágenes* + *probar en placa*. Y el común hoy sólo lo compila CubeIDE (los dos proyectos STM32, por barrido: `.cproject:91` excluye cuatro ficheros y `platform_freertos.c` no está entre ellos), así que **cualquier cambio en el común hay que construirlo con el STM32 headless** o no se sabe si compila.

### Paso 0 — REMEDIR el 32 % de la Pico (bloquea todo lo demás para esa familia)

La cabecera del común (`src/platform_freertos.c:18-29`) dice que con este fichero la Pico hacía PrintBench en 5 330 ms en vez de 4 040. **Esa medida es del 5-sep (`f80c541e`) y desde entonces `#485` (9-sep, `8d0ebee1`) y `#473 R2` (10-sep, `a257be8b`) han tocado los tres ficheros.** Antes de tratar el 32 % como bloqueante, hay que repetir la medida con el árbol de hoy.

*Comprobación*: sustituir `pico/CMakeLists.txt:149` por el común + un `pico/platform_pico_hw.c` mínimo, construir, y correr PrintBench cuatro veces contra las cuatro del fichero privado. Si el 32 % ha desaparecido, la fusión de la Pico deja de estar bloqueada; si sigue, ver §4.1.

### Paso 1 — Los arreglos que hay que hacer IGUAL, se funda o no (baratos, y quitan ruido del experimento)

1. `ft->lockout_victim = 0;` en `pico/platform_freertos.c:257-262`. Hoy es inocuo porque `bpvm_flash_lock_init_victim()` tiene el cuerpo entero bajo `#if configNUMBER_OF_CORES > 1` (`pico/flash_lock.c:15-23`) y el default es 1 core (`pico/CMakeLists.txt:219-220`), pero es una lectura de valor indeterminado en cada RUN.
2. Joinear los workers ya creados en el camino de error de `src/scheduler_smp.c:303-315` (hoy `bpvm_free(arg); return -1;` sin join). Es el único de los cuatro call sites que no está emparejado, y es precisamente el camino de OOM.
3. Arreglar la lista de perillas del común (`src/platform_freertos.c:36`): nombra `BPVM_FR_STACK_VM`, que **no existe en el árbol**, y omite `BPVM_FR_PRIO_VM`, que sí (`:55`). Es la lista que leerá quien migre.
4. Declarar `bpvm_platform_thread_arranca` en `include/bpvm_platform.h` con su comentario: hoy sólo existe dentro del `.c` del común, y quien lea el contrato no lo encuentra.

*Comprobación*: build STM32 headless + build Pico (~30 s) verdes; `nm` sobre los objetos para confirmar que no aparece ni desaparece ningún símbolo.

### Paso 2 — Llevar el ciclo de vida `suspend + join` a Pico y ESP32 (todavía SIN mover el fichero)

Es el arreglo medido en la Nucleo, y las dos plataformas tienen encendido lo que hace falta (`INCLUDE_vTaskSuspend` e `INCLUDE_vTaskDelete`: `pico/FreeRTOSConfig.h:78-79`, IDF `:210-211`). En el ESP hay un detalle de orden: su join descarta `et->task` antes del `vPortFree` (`esp32:261-268`), así que hay que borrar la tarea **antes** de liberar la estructura.

*Comprobación*: N RUNs seguidos en cada placa (≥10) mirando la memoria libre de FreeRTOS con el guardián de fin de RUN (#339) — la curva tiene que quedar plana. Es la cadencia exacta con la que murió la Nucleo al quinto RUN. **Este paso cambia una fuga por otra: ver R4.**

### Paso 3 — El semáforo del condvar: contador, no binario (las tres a la vez)

`xSemaphoreCreateBinary()` en las tres (`src:106`, `pico:77`, `esp32:70`) sólo retiene UN token, y `broadcast` emite N `gives` (`src:186-193`, `pico:177`, `esp32:164`). Los sobrantes se pierden. Los consumidores están en el camino de la salida (`src/comm_common.c:94`, `:119`, `:129-130`) y en el SMP/GC (`src/heap.c:699-715`, `src/scheduler_smp.c`), y varios esperan **sin plazo**, así que un wakeup perdido no se recupera solo. Elegir una de las dos formas de `broadcast` al fundir **no** lo arregla: hay que cambiar el semáforo o dar con reintento.

*Comprobación*: hoy no hay síntoma conocido que sirva de test. Lo honesto es hacerlo con el arnés de host (donde el camino SMP sí se ejecuta: `test/main.c:400`, `test/test_kill.c:58`) y comprobar que no cambia nada en placa. No lo vendas como arreglo de un bug observado.

### Paso 4 — Extraer lo de silicio, dejando la copia privada sólo con FreeRTOS

Crear `pico/platform_pico_hw.c` (los tres verbos + la definición fuerte del gancho) y su equivalente en `esp32/common/`. **El gancho se escribe `if (core_id > 0) bpvm_flash_lock_init_victim();`** — copiar el predicado de la Pico (`core_id != 0`, `pico:303`) dispararía para `io` y para los no-pinned, porque el común les pasa `core_id = -1` (`src:277`, `:295`).

*Comprobación*: la imagen sigue enlazando y `nm` muestra los tres símbolos en el nuevo objeto. Los tres son load-bearing: sin ellos el enlace se queda a oscuras.

### Paso 5 — Fundir la Pico (sólo si el Paso 0 salió verde)

**Sustituir**, no añadir, `pico/CMakeLists.txt:149`. Es la única colisión de basename entre `src/` y una carpeta de familia: añadir sin quitar da *multiple definition* de los dieciséis símbolos del backend.
Perillas: no hace falta `-D` de pila (`StackType_t` de 4 bytes), pero **sube los hilos BP de IDLE+1 a IDLE+2**; si se quiere conservar exactamente el comportamiento de hoy, `-DBPVM_FR_PRIO_VM=(tskIDLE_PRIORITY+1)`.

*Comprobación*: PrintBench (tiempo), KILL a mitad de cálculo (latencia), y la tanda de RUNs del Paso 2.

### Paso 6 — Fundir el ESP (los CUATRO, porque son cuatro listas)

a) **El include**: `#include "FreeRTOS.h"` no resuelve bajo IDF — el kernel registra `FreeRTOS-Kernel/include/freertos` como `PRIV_INCLUDE_DIRS` (`components/freertos/CMakeLists.txt:146`, `:178`). Lo barato y estable es hacer el include condicional por `ESP_PLATFORM` en el común; exportar la ruta privada del IDF ata el proyecto a un detalle interno de la versión.
b) `../../src/platform_freertos.c` a SRCS y **fuera** la copia privada, en las cuatro listas (`esp32/main:11`, `esp32c3/main:14`, `esp32c6/main:15`, `esp32p4/main:33`).
c) **`-DBPVM_FR_STACK_IO=4096 -DBPVM_FR_STACK_THR=4096` en los cuatro**, o mejor: expresar la perilla en bytes en el común y convertir en el punto de uso, que quita la clase de error.
d) El camino de pin: añadir una rama `xTaskCreatePinnedToCore` bajo `ESP_PLATFORM`, o dejar escrito que la afinidad queda muerta en ESP.

*Comprobación*: `idf.py build` de las cuatro (ESP-IDF está en `C:\esp6.0.1`; nunca entregar ESP sin compilar) y, en al menos una placa, un `print` — el primero, que es donde se manifiesta el fallo de pila.

### Paso 7 — Cierre

Borrar las copias privadas, cerrar el contrato en el header (gancho + obligación de join) y anotar en `docs/FICHAS.md`. No hay hoy ningún verificador que compare las listas de altas entre builds (`scripts/`, `tools/`: nada); si se quiere red, éste es el momento.

---

## 3. Riesgos, el más importante primero

**R1 — El 32 % de PrintBench en la Pico, sin explicar.** Es el único riesgo que bloquea de verdad, y no por su tamaño sino por la regla de la casa: *no se publica lo que no se entiende*. Está medido, repetible a ±10 ms y aislado por bisección (`src/platform_freertos.c:19-24`).

**R2 — La pila de `io` en el ESP: 4096 bytes pasarían a 1024.** No es "la cuarta parte": el sink que corre en `io` declara `char buf[1024]` en pila (`esp32/common/repl_esp32.c:436`, es el `io_ops.line` de `:774`) y el lazo añade `char tmp[256]` (`src/bpvm_io.c:107`). Es desbordamiento en la primera línea impresa. Con el canario encendido en los cuatro sdkconfig será un abort, no un silencio — pero el build sale verde. El común ya delega la unidad por escrito (`src:52-54`) y **ninguna familia pone el `-D`**.

**R3 — El include no resuelve en ESP-IDF.** Es el primer fallo que aparece y el más barato de arreglar; lo pongo alto porque es obligatorio, no porque sea grave.

**R4 — Cambio de fuga por fuga (Paso 2), y hay que decirlo entero.** Hoy, en Pico/ESP, un hilo que nadie joinee fuga la struct y el semáforo (`pvPortMalloc`, `pico:216-219`, `esp32:201-204`) y su pila+TCB **sólo vuelven si la ociosa recibe turno** — que es justo lo que no pasó en la Nucleo. Con `suspend+join`, la pila+TCB de un hilo no joineado **no vuelven nunca**, con certeza. O sea: se cambia una fuga probabilística grande por una determinista, y a cambio el join se vuelve obligatorio de verdad. Hoy es seguro (los cuatro call sites joinean, y `io` siempre: `src/bpvm_io.c:147`, `:164`, sin `return`/`goto`/`longjmp` en medio en ninguno de los repl) **salvo el camino de error de `src/scheduler_smp.c:303-315`** — de ahí el Paso 1.2. Nota: esto no inventa una obligación nueva, converge con el host, donde `platform_pthread.c:109` crea joinable y nadie hace `pthread_detach`.

**R5 — La afinidad se pierde EN SILENCIO en ESP.** El gate del común (`src:250`) exige `configUSE_CORE_AFFINITY`, que el IDF sólo define con el kernel Amazon SMP. Y ojo con la causa: **no es `UNICORE`**. Quitar `CONFIG_FREERTOS_UNICORE` del S3 o del P4 (`esp32/sdkconfig.defaults:78`, `esp32p4/sdkconfig.defaults:79`) no lo reactiva; y forzarlo a mano no enlazaría, porque `xTaskCreateAffinitySet` no existe en ese kernel. Eso rompe la promesa escrita en `include/bpvm_platform.h:67-69`.

**R6 — El gancho de flash de la Pico se queda sin dueño.** Nadie define la versión fuerte (el símbolo aparece tres veces, las tres en `src/platform_freertos.c`). Hoy no cambia el binario que se envía (single-core: la función es un cuerpo vacío), pero si algún día se enciende el dual-core sin definirlo, el síntoma no será mudo: `bpvm_flash_lock_begin` (`pico/flash_lock.c:25-33`) llama a `multicore_lockout_start_blocking()` esperando una víctima que nadie registró.

**R7 — La prioridad de nacimiento sube de IDLE+1 a IDLE+2 en Pico y ESP.** Hoy es latente (nadie llama a `thread_create` pelado en firmware), pero es el mismo par de números que #485 quitó de `io`, todavía vivo en `create` y `_pinned`. Si algún día tienen consumidor, el arreglo ya está escrito al lado: `uxTaskPriorityGet(NULL)`.

**R8 — El semáforo binario del condvar es un defecto de las tres, y la fusión no lo arregla.** No hay fallo observado atribuido a esto.

**R9 — Ningún `make`/`cmake`/`idf.py` compila el común hoy.** Sólo CubeIDE. Un cambio en el común no lo ve nadie hasta que se construye el STM32.

**R10 — Basename colisionante**: añadir sin quitar (Paso 5) da *multiple definition*.

**R11 — El STM32 barre `src/` con lista negra**: cualquier `.c` nuevo host-only que aterrice en `src/` entra en los dos builds. La vía preferida y ya documentada es la guarda de plataforma dentro del propio fichero (`src/net_host.c:21-27`), no ampliar las exclusiones.

---

## 4. Lo que NO he podido determinar leyendo — hay que medirlo

1. **El mecanismo del 32 %.** Lo que sí sé por leer: el camino de un `print` es `bpvm_oq_push` → `bpvm_oq_pop_timed` en `src/comm_common.c` (mutex + `signal(not_empty)` `:73` + `timed_wait` `:108` + `broadcast(not_full)` `:119`), y en ese camino el **único** trozo donde el común y la copia de la Pico difieren funcionalmente es la forma de `broadcast` (`src:186-193` toma el guard N+1 veces; `pico:170-178`, una). Con un esperador eso es *un* par take/give de mutex extra por `pop`: repartido entre 2 000 líneas, 1 290 ms serían ~650 µs por línea, que no cuadra con el coste de un mutex. **Así que no lo doy por explicado**: es una hipótesis a falsar con un experimento de una variable (copiar sólo el `broadcast` de la Pico al común y remedir), no una conclusión.
2. **Por qué el KILL mejoró de 33 ms a 2 ms con el común** (misma cabecera). Es la otra mitad de la pista y tampoco está explicada.
3. **Si la tarea ociosa recibe turno durante un RUN en la Pico y en los ESP.** Eso decide si la fuga de `vTaskDelete(NULL)` se manifiesta ahí o sólo está latente. En la Nucleo se manifestó; en las otras dos, no determinado.
4. **Si el común compila bajo ESP-IDF más allá del include.** Por lectura sólo está probado que el include no resuelve; el resto no se ha compilado.
5. **Si un `.c` nuevo sin guarda rompería el enlace del STM32 por símbolo duplicado**: no hay hoy ningún caso en el árbol que lo demuestre.
6. **El coste en RAM de retener pila+TCB hasta el join** durante un RUN largo con varios hilos: es la misma memoria que hoy, pero la ventana cambia; no hay medida.