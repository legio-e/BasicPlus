# `#473` — triaje de los 70 hallazgos `incomodo`/`cosmetico` (10-sep-2026)

> **Qué es esto.** El resto de la auditoría `A3`, verificado contra el código de HOY y no contra el
> texto del auditor. Siete lotes de diez, más una ronda de escépticos **sobre los declarados
> muertos** — porque al cerrar una versión el error caro es dar por resuelto lo que sigue roto: eso
> apaga la búsqueda.
>
> **Y esa ronda se ganó el sueldo: de 12 dados por muertos, 8 resucitaron.** El patrón es siempre el
> mismo — se arregló la mitad que llega al usuario y se declaró muerto el hallazgo entero.
>
> ⚠️ **Los cuatro de la lista corta están releídos a mano, uno a uno.** El resto no: es material de
> trabajo, no veredicto firme.

# Triaje de los 70 hallazgos "incómodo/cosmético" de #473

Revisión hecha sobre el código de HOY (10-sep), no sobre el texto del auditor. Todo lo que afirmo lleva fichero:línea; lo que no pude determinar leyendo está en el §5.

---

## 1. EL REPARTO

| Desenlace | N | Detalle |
|---|---:|---|
| **Arreglados de verdad** (muertos confirmados) | **4** | ver §5: no tengo sus líneas en el material que se me pasó, sólo el recuento |
| **Cubiertos por una ficha abierta** | **7** | #470 ×4 (líneas 110, 126, 130, 356) · #473 ×2 (22, 486) · #487 ×1 (134) |
| **Vivos** | **59** | 51 que llegaron marcados "vivo" + **8 que llegaron marcados "arreglado/falso" y resucitaron** |
| — de ellos, **rotos hoy** (lista corta §2) | **4** | 114, 240, 434, 438 |
| — de ellos, **a V7** (§3) | **55** | |
| **TOTAL** | **70** | |

**Trabajo real que queda antes de cerrar V6: cuatro arreglos**, tres de ellos locales a un fichero y con el patrón ya escrito y probado en otra familia. Todo lo demás son duplicaciones, nombres feos, comentarios rancios y contratos mal repartidos: ninguno da un resultado incorrecto hoy.

Dato incómodo del recuento: de los 12 que la primera pasada dio por muertos, **8 resucitaron** (dos tercios). El patrón es siempre el mismo — se arregla la mitad que llega al usuario y se declara muerto el hallazgo entero. Detalle en §4.

---

## 2. LA LISTA CORTA — lo que merece tocarse antes de cerrar V6

Cuatro. Ordenados por daño. Los tres primeros los he verificado a mano en el código hoy (no me fío del texto del auditor ni del triaje).

**1. El `recv_line` de la Pico y de las tres imágenes ESP gira PARA SIEMPRE si la línea se queda a medias** — `pico/wire_v1.c:38-48` (`if (c<0) { vTaskDelay(5); continue; }`) y `esp32/common/wire_v1.c:112-119` (`if (c<0) continue;`), frente al STM32 que sí topa: `stm32/port/stm32_wire.c:107-115`, `-2` a los 300 ms con el comentario «Anti-cuelgue… en vez de girar para siempre». **Se ve desde fuera:** un byte suelto sin `\n` durante un RUN (alguien tecleando en un terminal sobre el USB-Serial-JTAG del C3/C6) y el hilo `io` no vuelve → la cola de salida se llena → la VM se para por contrapresión → **el KILL no llega nunca**. En la Pico se sale desenchufando. El arreglo está escrito y probado: cuatro líneas por transporte, copiadas del STM32.

**2. La atomicidad de una línea en el cable sólo la garantiza la Pico** — `pico/wire_v1.c:92-98` toma `wire_v1_tx_lock()` dentro de `send_line`; `esp32/common/wire_v1.c:135-138` son dos `uart_write_bytes` pelados (igual en `wire_v1_usbjtag.c:132-135`, `esp32p4/main/wire_v1_tcp.c:180-183`, `stm32/port/stm32_wire.c:124-127`). El común entrega la salida bajo `tx_mtx` (`src/bpvm_io.c:84-87`) pero llama al poll FUERA (`:114`), mientras el `dbgw_send` del hilo vm sí lo toma. **Se ve desde fuera:** depurando en placa, el IDE recibe un JSON entrelazado (el poll contesta BUSY/HELLO justo cuando el hilo vm manda un BP_HIT). Corrupción de framing, no estética, y no existe en el PC. ⚠️ **Orden obligatorio:** arreglar el 1 antes, y NO envolver el poll en el `tx_lock` — la salida limpia es que `wire_v1_send_line` tome el cerrojo por dentro, como ya hace la Pico.

**3. `Uart.available()` contesta un BOOLEANO en la Pico donde su propio contrato dice «cuántos bytes»** — contrato en `include/bpvm_uart.h:16-17` («cuántos bytes hay en el buffer RX… -1 si el backend no lo soporta»); la Pico devuelve `uart_is_readable(inst) ? 1 : 0` (`pico/main.c:376-379`); el ESP32 cumple (`esp32/common/gpio_esp32.c:249-254`, `uart_get_buffered_data_len`); el STM32 cumple el "no soportado" (`stm32/port/gpio_stm32.c:545-548`, `return -1` y lo dice). **Se ve desde fuera:** `if Uart.available() >= 4` no se cumple NUNCA en la Pico con 40 bytes esperando, y sí se cumple en el ESP32 con el mismo programa. Silencioso, y en la familia de referencia. Arreglo local al backend de la Pico. **No tocar el stub del host** (`src/uart.c:57`, «TEXTO = CONTRATO DE PARIDAD»): eso toca las dos VMs a la vez.

**4. `read_stream` no existe en FAT: el GET de un fichero grande desde /sd** — la tabla del backend FAT no tiene el campo (`src/fs_fat.c:436-452`, verificado: están `crc32`, `read_at`, `write_at`… y no `read_stream`), littlefs sí (`src/fs_lfs.c:407`, «#453 — idem para el GET: 472 aperturas por 120 KB»). El respaldo es un bucle de 256 B (`src/fs_facade.c:466-482`) sobre `fat_read_at`, que hace f_open+f_lseek+f_read+f_close en CADA llamada, y el propio fichero documenta que el coste es **cuadrático** (`src/fs_fat.c:225-234`, 5432 aperturas medidas para 1,3 MB). **Se ve desde fuera:** el explorador del IDE enseña /sd y la descarga revienta el timeout de 10 s, mientras el mismo gesto sobre /flash va fino desde #453. El arreglo no tiene diseño que inventar: es el gemelo exacto de `fat_crc32` (`src/fs_fat.c:235-256`) cambiando el consumidor del buffer. **Condición:** la medida de los 10 s es prestada (#453, littlefs en un S3). Cronometrar un GET de ~120 KB desde la SD de la Metro **antes** de tocar nada; si no revienta, se va a V7.

### Los dos de frontera, y por qué los dejo fuera

- **Un FORMAT que falla contesta `FORMAT_REPLY` OK** (resucitado de la línea 466). La rama buena está escrita en el común (`src/bpvm_repl.c:616-619`) y es **inalcanzable** porque las tres cinturas devuelven 0 fijo (`pico/repl_v1.c:1488`, `esp32/common/repl_esp32.c:1007`, `stm32/port/fs_lfs_stm32.c:269-275` es `void`). Es un OK falso — pero sólo se alcanza si `lfs_format`/`lfs_mount` fallan por hardware, y a partir de ahí `src/fs_lfs.c:468-469` pone el backend a NULL y todo lo siguiente falla a gritos. Es *una respuesta de más antes del ruido*, y arreglarlo bien significa cambiar la firma en las tres familias durante un cierre. **Voto: V7.** Lo que sí haría de propina, riesgo cero: corregir `src/bpvm_repl.c:641-642`, que afirma «el STM32, que sí escribe flash» y es FALSO desde que `fs_lfs_stm32.c:277-280` es un no-op documentado — un comentario del común que miente sobre una familia manda una depuración al sitio equivocado.
- **`ensure_parent_dirs` puede rendirse en silencio en el STM32** (resucitado de la 268): `stm32/port/fs_lfs_stm32.c:161` con `char dir[128]` y sin la comprobación de `NAME_TOO_LONG` que sí tienen pico y esp32. Haría falta un path de /lib de más de 127 caracteres; nadie ha contado si existe (§5). **Voto: V7** con el resto de la cuadruplicación.

---

## 3. LO QUE VA A V7 (55 vivos + los 7 ya cubiertos)

**A. Identidad y números del micro → se pega a #470** (ya en V7, con el diseño decidido: arrays de valores válidos por micro, y el count = su length).
Líneas 110 (I2C/SPI/UART_BUSES son `return 2` en BP y no llegan a C), 126 (PWM_SLICES: slices en Pico / salidas en STM32 / canales LEDC en ESP32), 130 (los counts no acotan lo que la fachada acepta), 356 (`pwm_slices`, vocabulario del RP2350) — **ya adoptadas**. Y **sin adoptar, hay que meterlas**: 348 (los stubs de `src/pico.c` devuelven el perfil RP2350 — ojo, son **contrato de paridad** con `VirtualMachine.java:5665-5666`, se tocan las dos VMs o ninguna), 352 (`pio_count` en `include/bpvm_repl.h:86`, huérfano: #470 se llevó cuatro números y éste no estaba), 138 y 364 (las cabeceras de ADC/Pulse/PWM definen el contrato con los pines del RP2350), 118 (baudrate del I2C ignorado en STM32 — degrada al lado seguro; y el `pin` de `setDuty`) → estas dos últimas comparten terreno con **#479** (mapa de pines).

**B. El nombre `pico` en la capa C → HUÉRFANO, necesita ficha nueva.** #471 se cerró el 9-sep habiendo arreglado sólo la capa de usuario (`Machine.bp`), y su lista de "lo que va a V7" no incluye el renombrado. Vivos: `include/bpvm_pico.h` + `bpvm_pico_set_backend`, que registran **las cinco familias** (`pico/main.c:1566`, `esp32/common/gpio_esp32.c:666`, `esp32p4/main/p4_board_id.c:97`, `stm32/port/gpio_stm32.c:1000`); 29 `BUILTIN_PICO_*` en `src/builtins.c`; y una capa que el auditor no vio: **`miVM/.../bytecode/Builtin.java:195-428` declara `PICO_UNIQUE_ID("__picoUniqueId")`, `PICO_BOARD_NAME`…** — o sea, `Machine.bp` llama por debajo a intrínsecas `__pico*` en las DOS VMs. Suma aquí la 284 (`bpvm_flash_lock_*`, prefijo del común viviendo sólo en `pico/`).

**C. Los tres REPL y el wire → el frente U3, con #473 como material de entrada.** 15 hallazgos, todos duplicación o simetría, ninguno con consecuencia: 422 (poll triplicado + `try_getchar` con tres nombres), 426 (la cintura del depurador, 41 líneas idénticas carácter a carácter entre Pico y ESP32), 454 (traducción `fs_status_t`, idéntica salvo un espacio), 458 (tres códigos distintos para el bulk que no cabe), 462 (autorun triplicado), 470 (la cabecera dice cuatro verbos, el simulador implementa tres y su comentario dice dos), 478 (orden de consulta del REPL común: **cero intersección** de verbos, contado uno a uno), 482 (PROMPT_RESPONSE en 2 de 3 — ver §4), 494 (la cuantificación: 1.909 líneas hoy, +2,3 % en cinco días), 430 y 252 (`capabilities` miente — **nadie lo consume**: 0 aciertos en `BpIde/src`), 442 (el campo `user` de `bpvm_io_ops_t` significa cosa distinta en cada implementación), 474 (la cola de control trunca a 255 B y devuelve éxito — hoy **inalcanzable**, medido: sólo entran los 12 verbos del depurador y el único con texto es SET_BP con el nombre pelado, ~70 B), 446 (la guarda `vm->io == NULL` está en `scheduler.c:100` y NO en `scheduler_smp.c:114` — dos palabras, para la sesión que vuelva a tocar SMP), 490 (`comm_host.c` de relleno: código muerto, `bpvm_run_smp` sólo lo llama la Pico).

**D. FS y almacenamiento → ficha nueva de reparto común/familia.** 15: 236 (`copy` sin el respaldo read+write que promete su comentario), 244 (`hay_medio()` significa cosas distintas; el único llamador vivo es la Metro, que sí detecta), 248 (DF mezcla volúmenes: bytes de la raíz, `fileCount` incluye /sd), 256 (no hay fachada de flash: cuatro formas y el ENV sin contrato — **es la pieza que habilita subir `board_mgr_*`**), 260 (SD_INFO/SD_MOUNT sólo en la Pico; el P4 monta desde su `main`), 264 (el shim heredado del FS triplicado y con interfaz distinta — y buena parte es **código muerto**: `fs_put_append`, `fs_delete/fs_del`, `fs_get` sin llamador fuera de su `.c`), 268 (`ensure_parent_dirs` ×4: se arregló **1 de 4**, ver §4), 276 (el STM32 compila la pila de SD entera sin llamadores), 280 (`docs/CENSO_FAMILIAS.md` contradice al código — **trampa: quien planifique el reparto de V7 con ese censo reparte mal**; el arreglo correcto es generarlo de las listas de fuentes reales), 324 y 328 (la fachada de espacio y sus tres envoltorios de una línea), 360 (`ldo`, concepto exclusivo del P4, en `include/bpvm_blk_sdmmc.h:47`), 450 (el escaneo de `.mdn` común lo llaman 2 de 4: en STM32 y simulador un `.mdn` dentro de un pack corre interpretado — mismo resultado, más lento), 466 (canal de error para `fs_save`/`fs_format`), y 240 si la medida del §2 sale limpia.

**E. GUI y la costura de display → ficha nueva.** 10: 176 (`disp_init(w,h)`: 1 de 4 respeta los parámetros — hoy son redundantes, `resolve_screen_size()` pregunta antes), 180 (el fallo de arranque del display no llega a BP: el C6 queda mudo, el P4 bombea sin buffers), 184 (el enganche de LVGL copiado ×4), 188 (el GT911 con dos drivers enteros distintos), 192 (la política del pump escrita a mano ×4 + una copia rancia viva, ver §4), 196 (el display es la única de las 13 fachadas de HW sin `set_backend`, ver §4), 200 (dos de los siete verbos de la cabecera común son sólo del host), 204 (96 KB de SRAM fijos: medido HOY en `esp32c6/build/bpvm_esp32c6.map`, `.bss.g_nodes` = 0x18000 + 2×0x800 de la cola de #489 ≈ 102 KB en un chip de 512 KB), 368 (la costura es de enlace, no registrable), 340 (`lv_conf.h` decide con una lista enumerada de macros de placa y **falla abierto** — hoy las tres placas con panel están en las dos listas).

**F. RTOS y plataforma → #473** (la fusión de los tres backends de FreeRTOS, **bloqueada por la medida**: PrintBench en la Pico 4040→5330 ms, 32 % peor). 9: 22 (dos ciclos de vida del handle de hilo — **medido en R16: no filtra**), 26 (arrancar la tarea de la VM no tiene fachada), 30 (los ganchos de fallo del RTOS escritos dos veces), 34 (`bpvm_platform_thread_arranca` declarado en el `.c`, no en el header), 46 (la cabecera del contrato dice «(futuro, F5+)» de un fichero en producción), 486 (la Pico con copia privada divergida — contenido a mano con éxito: los dos últimos parches, 8d0ebee1 y a257be8b, entraron en las dos), 18 (resucitado: `BPVM_FR_PRIO_VM` y los workers de la Pico a +1, ver §4), 332 y 42 (`BPVM_SCRATCH_BYTES` mirando el macro de CubeMX: **aceptado y documentado**, tiene aserto de compilación).

**G. Huecos documentados a medias → #487** (breadcrumb mudo en 4 de 5) y, la mitad del RTC, **#490** (bajo consumo): línea 134. `bpvm_rtc_set_backend` se llama en UN sitio de todo el árbol (`stm32/port/gpio_stm32.c:1009`); lo que ha mejorado es que `src/rtc.c:1-22` ya confiesa el hueco con todas las letras.

---

## 4. LO QUE EL AUDITOR SE INVENTÓ O EXAGERÓ — y lo que exageró la primera pasada

**Sobre la auditoría del 5-sep: los HECHOS aguantan, las CONSECUENCIAS no.** En cinco días de trabajo intenso el código citado seguía donde decía (a veces con la línea corrida), y la cuantificación se sostuvo al recontarla (1.866 → 1.909 líneas). Lo que falla sistemáticamente es el «y esto produce X»:

- **Daño inventado, línea 482 (PROMPT_RESPONSE):** decía «deja al IDE esperando un reply que nunca llega». Falso por dos vías: el IDE lo manda con `sendOneShot` (`BpvmClient.java:93`, `:476-492`) — fire-and-forget, no registra pendiente; y el mensaje **ni siquiera puede llegar**, porque la VM-C todavía no emite `PROMPT_REQUEST`. Además el STM32 sí contesta (UNSUPPORTED con el mismo id), no calla.
- **Daño real pero inofensivo, línea 422:** «en el STM32 la cola de control de A1.7 no se alimenta» — cierto, y da igual: el STM32 no tiene depurador cableado (`grep -c bpvm_dbg_wire stm32/port/stm32_repl.c` → 0), nadie saca de esa cola.
- **Prueba caducada, línea 348:** la traza «[pico] tempC (stub → 25.0)» que citaba como evidencia **ya no existe** — la quitó #471 por ser rotura de paridad. Y los números que denunciaba son contrato de paridad con miVM, no un descuido.
- **Prueba caducada, línea 138:** su ejemplo fuerte (la fuga del ADC) está arreglado desde #469; media prueba se le cayó.
- **Cuantificador falso con fondo cierto, línea 196:** «la ÚNICA costura de la VM que no es fachada de punteros» es falso (`bpvm_platform.h` son 22 símbolos sueltos). Pero el fondo es cierto y **la primera pasada se pasó de frenada cerrándolo como FALSO**: el display es la única de las **13 fachadas de periférico** sin `set_backend`, y por eso es el único que no puede tener el camino «sin backend → error» que #480 impuso al watchdog. Se reescribe el cuantificador, no se tira el hallazgo.
- **Enunciado equivocado con fondo vivo, línea 466:** dos de sus tres patas se caen (`fs_save` es no-op en las tres familias; «la Pico sí lo propaga» es decorativo). La tercera —el FORMAT fallido que contesta OK— sigue viva y es **universal, no del STM32**, así que el hallazgo no es falso: es más grande y apunta a otro sitio.
- **Se quedó CORTO, línea 264:** no vio que buena parte de los tres shims es código muerto.
- **Su propio plan caducó, línea 494:** el punto (3) manda a arreglar el escape del EXITED del STM32, hecho hoy en 80d6ad0d. Si alguien usa esa lista como plan, trabaja en balde.

**Sobre la primera pasada de triaje (la que dio 12 por muertos): 8 de 12 mal.** Y el error es de una sola forma — *se arregla la mitad que llega al usuario y se firma el acta entera*:

- **38 y 316:** «arreglado, ya existe `Machine.bp`». Arreglada la capa de usuario, vivas la fachada C (las cinco familias registran en `bpvm_pico_*`), los 29 ordinales `BUILTIN_PICO_` y las intrínsecas `__pico*` **en las dos VMs**. Y el alcance de #471 lo decía: «renombrar `bpvm_pico_*` **Y** el módulo `Pico`» (`docs/FICHAS.md:5249`) — se hizo el segundo y se cerró.
- **268:** «lo mató #456». Mató el síntoma citado (`dir[64]` en el común), no el hallazgo, que se titula «×4»: siguen las cuatro copias y la asimetría de topes no se cerró, **se invirtió** (256 en el común contra 127 útiles en las familias).
- **192:** «arreglado por #462». Muerta la mitad peligrosa (ningún pump vivo duerme), viva la duplicación (cuatro copias del mismo clamp) y viva una copia con la política VIEJA entera en `esp32p4/main/gui_display_dsi.c:721-727` — dentro de `p4_gfx_lvgl_test`, que **no es código muerto declarado**: está exportada en el `.h` y conservada a propósito como diagnóstico.
- **42:** «arreglado». Uno de los dos `#if` murió (#472), el otro sigue en `include/bpvm.h:346`. El desenlace honesto es «uno arreglado, el otro **aceptado y documentado**».
- **18:** «ya no tiene con quién discrepar». Falso en dos sitios: `src/platform_freertos.c:56` sigue teniendo `BPVM_FR_PRIO_VM (tskIDLE_PRIORITY+2)`, que coincide **por casualidad** con los dos `main.c` del STM32; y en la Pico `platform_freertos.c:226,308` crea los hilos que ejecutan la VM a **+1** mientras `vm_task` va a **+2**, o sea que en el build SMP `io` queda POR ENCIMA del intérprete — justo lo que `include/bpvm_platform.h:95-100` declara prohibido y midió.

**Conclusión de fiabilidad:** de los ~91 hallazgos originales, ni uno resultó **enteramente** inventado; lo inventado o exagerado fue el daño, en al menos cinco. Y un "arreglado" de revisión rápida vale bastante menos que un "vivo": dos tercios se cayeron al comprobarlos. Regla para la sesión que herede esto — **verificar los cierres, no los hallazgos**.

---

## 5. LO QUE NO SE HA PODIDO DETERMINAR

1. **Los 4 "muertos confirmados"**: sé que son cuatro, no sé cuáles. En el material que recibí vienen sólo como recuento, sin línea ni título. Hace falta la lista de la revisión que los confirmó.
2. **El timeout real del GET sobre /sd** (§2, punto 4). La medida de los 10 s es la de #453 (littlefs, S3). Lo que sí está medido y escrito es que en FAT el mecanismo es el mismo y además cuadrático. **Para saberlo:** un GET desde el IDE de un fichero de ~120 KB en la SD de la Metro, con cronómetro. Media hora.
3. **Los dos bugs del wire no se han reproducido**, sólo leído. Para el cuelgue (§2.1) es barato: teclear un carácter suelto en un terminal sobre el USB-Serial-JTAG del C3/C6 durante un RUN y ver si el KILL responde. Para el entrelazado (§2.2) hace falta una sesión de depuración en placa con salida abundante mientras se pisa un breakpoint, mirando si llega un JSON partido — o instrumentar el receptor para contar líneas mal formadas.
4. **Si el enlazador descarta `p4_gfx_lvgl_test`** (la copia rancia del pump). Nadie ha mirado el mapa. Determinable en dos minutos con un grep sobre `esp32p4/build/*.map`.
5. **Si el build SMP de la Pico se usa hoy** fuera de `pico/h2-test/RUNBOOK.md:89-90`. Nadie define `BPVM_PICO_SMP_WORKERS` en el árbol, pero no he comprobado si algún script o CI lo activa.
6. **Si existe algún path de /lib de la stdlib por encima de 127 caracteres** (lo que haría morder el `ensure_parent_dirs` del STM32). Se contesta contando la tabla de `bpvm_mods_instalar_tabla`; no lo he hecho.
7. **El arreglo exacto de `Uart.available()` en la Pico**: no he determinado si el SDK del RP2350 expone el nivel del FIFO RX (`uart_is_readable` es booleano). Si no lo expone, la respuesta honesta del contrato es `-1`, que ya es una de las dos opciones válidas — pero conviene mirarlo antes de prometer el número.
8. **Nada de esto se ha compilado ni ejecutado.** El triaje entero es lectura del código de hoy más las medidas ya escritas en el repo (R16 en la Pico, el mapa del C6, `fs_fat.c`). Los cuatro de la lista corta se apoyan en lectura verificada línea a línea, no en reproducción.