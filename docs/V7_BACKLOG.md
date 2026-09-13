# V7 — el mapa del hito EN CURSO

> 🚀 **Desde el 13-sep-2026, V7 es la versión en curso** (V6 se publicó el 12-sep, tag `v6.0`,
> «orden en los micros»). Este fichero es la **puerta de entrada** al hito: un **índice**, una
> línea por asunto y un puntero a dónde está el detalle. El estado vive en **`FICHAS.md`
> §ABIERTAS** (bloque V7); los diseños, en **`V7_IDEAS.md`**.
>
> ⚠️ **NO es una fuente de verdad y no debe convertirse en una.** Si algo aquí contradice a
> `FICHAS`, manda `FICHAS`. Existe por la misma razón que `V6_BACKLOG.md`: lo aplazado estaba
> repartido en cuatro sitios (fichas abiertas, restos de fichas cerradas, el diario y los
> cuadernos de ideas) y no había forma de verlo de un vistazo.

**De dónde sale.** Del **censo del 13-sep**, hecho al abrir V7: 26 barridos sobre `FICHAS.md`
entero, `ESTADO.md`, `V6_BACKLOG`, `V6_IDEAS`, `V7_IDEAS` y `PENDIENTES` (448 candidatas en bruto,
261 únicas), y **cada candidata releída en su sitio** y buscada en todo el registro por si se
había cerrado después: 92 cayeron (cerradas, duplicadas o superadas), quedan las
de abajo. Los punteros `FICHAS:Lnnnn` son líneas del `FICHAS.md` del 13-sep.

**Recuento:** **19 con número** (18 fichas + el hito `L1`), **28 restos** de fichas cerradas de V6, **~40 sueltos** sin ficha y **6 fuera del plan** — 90 entradas en este índice.

---

## 🎯 Lo que Eduardo nombró de memoria al abrir V7 (13-sep)

Sin mirar el registro: *«la caracterización de los periféricos de cada micro, limitaciones de
los `.mod` a la hora de guardar dependencias intra-clases cuando estaban en diferentes módulos,
el sistema de archivos a la hora de acceder a una posición concreta dentro de un archivo, así
como la implementación en BP de una clase `File`»*. Es la mejor señal de por dónde empezar, y
los cuatro caen en el mismo sitio del mapa:

| lo que nombró | dónde vive |
|---|---|
| **los periféricos de cada micro** | §D: `#470` (los cuatro números → arrays por micro), `#479` (mapa de pines y analógicos), `#378` (que cada micro DIGA lo que tiene), los restos de `#469` (el ADC del STM32) y de `#480` (el `Pulse` del C3) |
| **las dependencias entre clases de módulos distintos en el `.mod`** | §B: `#492` (la interfaz no exporta campos), `#460` (herencia entre módulos, aparcada hasta tener versiones), `#500` (la norma de versiones) |
| **acceder a una posición concreta de un fichero** | §C: `#491`, el cursor |
| **la clase `File` en BP** | §C: la mitad de arriba de `#491` (`File` + `TextFile`) sobre el destructor de `L1` |

## 🧭 EL PLAN DE V7 — propuesta, por decidir con Eduardo

El criterio con el que se ordena es el suyo al cerrar V6: *«llevamos varias versiones (V4 y V6)
sentando las bases para una VM más robusta; todavía no hemos llegado a meta»*. Lo que **cierra
clases de fallo** va delante de lo que añade; y lo pequeño que desbloquea, delante de lo grande.

| orden | qué | por qué ahí |
|---|---|---|
| **1** | **§B — las dependencias**: `#500` la norma de versiones, `#493` la sombra de `/app` y del pack, `#492` los campos que no cruzan | Es lo que mordió en `F1` (un `Stdlib.pack` MOD6 y un `Core` rancio) y lo que Eduardo nombra segundo. Cierra una clase de fallo **silencioso**. Pequeño: una o dos sesiones |
| **2** | **`L1` — el módulo raíz** | La columna vertebral de V7 (decidido el 5-sep): los tipos y las intrínsecas en un fichero legible, las dos tablas generadas. Encima de él se diseñan los añadidos: destructor, `Map`, `Time`, `#19` (`Object` comodín ya está hecho: el censo lo cazó sin tachar). Y desbloquea la sobrecarga de intrínsecas |
| **3** | **§C — `#491` el cursor y `File`/`TextFile`** | Lo tercero y lo cuarto que nombró. Necesita el destructor (paso 2) y arregla la enfermedad que ha vuelto tres veces (`read_stream` sólo en littlefs, sin escritura en streaming, aperturas duplicadas) |
| **4** | **§D — la HAL BP de periféricos**: `#378` → `#470` + `#479` + restos | Lo primero que nombró. Cambia `Adc.bp`/`Gpio.bp` embebidos en las cinco imágenes (ABI y blobs): mejor con el módulo raíz ya hecho, que es quien decide dónde se declaran |
| **5** | **§E — `#502` + `#441`**: el destino `ilp32` y la cabecera del `.mdn` con float-ABI y huella de flags | Un solo salto de formato para las dos; y la red del AOT (resto de `N1`) |
| entre medias | `#495`, `#452`, los restos de `T1`, las medidas pendientes (§H) | Sesiones cortas para cambiar de aire entre los grandes |
| cuando haya amperímetro | `#490` bajo consumo (estudio) | Sin números no hay estudio |

📌 **Regla de V6 que sigue:** los pendientes de la lista se **cierran o se cancelan**, no se
acumulan; cancelar es un resultado válido. Y **una entrada es una cosa**: los restos que aquí
cuelgan de una ficha cerrada se numeran el día que entren en el plan, no antes.

---

## A · El módulo raíz — lenguaje y compilador (`L1`)

| asunto | dónde |
|---|---|
| **`L1` — el módulo raíz** (la unidad `System`): los tipos y las 226 intrínsecas en un fichero BP legible; las dos tablas de ids (`Builtin.java`/`builtins.c`) **generadas** de él, y `check_builtins.py` se tira (resto de `#476`). *«Un solo archivo, sin posibilidad de diferencias entre compilador y VM»* (Eduardo, 5-sep) | `FICHAS` `L1` (L1055) y la decisión (L6271); `#476` (L4940) |
| **Sobrecargar una intrínseca** — `Intrinsics.REGISTRY` indexa por nombre pelado y el emisor compone la clave con `fs.name`: por eso `sign`/`signF` no se unifican. `factorial` en f64 **no necesita builtin**: `gamma(n+1)` ya lo cubre (`GammaFact.bp`) | `FICHAS` L1086 |
| **Destructor `~Clase()` + `var owner` LOCAL** — diseño hecho (`V5_IDEAS` 9-ago); `var owner` como campo ya existe (`#449`/`#450`). Y la 5.ª capa que pidió `#491`: destructor **nativo** que el GC llama tras el barrido | `FICHAS` L6287 y `#491` (L299) |
| **`Map`**: captadores tipados para claves **y** valores + `put` sobrecargado para clave entera y cadena (decidido 23-ago; `Collections.bp` sigue con `Object`) | `FICHAS` L10435 |
| **`#396` — módulo `Time`** con `Time.Date` sobre un `long` de segundos (no un tipo del lenguaje) | `FICHAS` L11869 |
| **`#19` — array fijo LOCAL inline de verdad** (tarea `#288`; el UAF se cerró en V4): `LEA_LOCAL` existe y el frontend nunca lo emite | `FICHAS` L11887; `V4_REF_AUDIT.md` §307 |
| Mudar **`Exception`** fuera de `Core`, como `Object` (ahorraría el `import Core` a 21 de los 56 ficheros que lo ganaron) — idea, sin ficha | `ESTADO` 30-ago |
| **`intrinsic` declarada sin registro no da error** en el compilador: compila y en runtime devuelve basura (miVM `OutOfMemoryError`, VM-C segfault) — hueco, sin ficha | `FICHAS` L4403 |
| `#460` — cabo suelto: **quitar del compilador** `module interface`/`implements` (paso aparte, con su verificación); la **herencia entre módulos** aparcada hasta tener *«un control de versiones efectivo»* — el caso con el que medirla es `Core` | `FICHAS` `#460` (L8414) |
| 🐛 **`L20` — el compilador ESCRIBE el `.mod` aunque dé error**: `for … step -1` falla (exit 2) y aun así deja un `.mod` que ejecuta sin el bucle. Silencioso: va delante. Y la decisión de lenguaje que `D1` dejó sin zanjar: `step` negativo o variable (hoy sólo literal entero positivo) | `PENDIENTES` L20 |
| `L19` — el cast `Clase(x)` sólo baja desde `Object`: `Perro(a)` con `a: Animal` se toma por constructor | `PENDIENTES` L19 |
| `M4` — prohibir el prefijo `__` en identificadores de usuario (hoy el lexer lo acepta y comparte namespace con los sintéticos `__prop_get_X`…) · `N9` — una clase sintetizada (`Thread`/`Mutex`/`StringBuilder`) redeclarada a medias por el usuario gana sin diagnóstico · `M5` — comprobar que el debugger enseña las properties heredadas | `PENDIENTES` M4, N9, M5 |
| `N2` — documentar como invariante que `JavaMutex.{ownerTid, waiters}` sólo se toca bajo `vmLock` (minutos; el código ya lo cumple) | `PENDIENTES` N2 |

## B · Lo que un `.mod` exporta y cómo se resuelven las dependencias

| asunto | dónde |
|---|---|
| **`#492` — los campos PROTEGIDOS no cruzan módulos**: la interfaz exporta métodos y propiedades, pero no campos (`field <nombre>:<tipo> slot <n>` en `ClassSig`); el rodeo es un getter. Coste: rehacer la cadena entera | `FICHAS` `#492` (L135) |
| **`#500` — la norma de versiones de las dependencias**: el `.mod` de la app lleva la versión de cada dependencia y el cargador **rechaza** una más antigua, venga del pack, de `/app` o de `/lib`. *«`Core.bp` nos sigue dando guerra»* | `FICHAS` `#500` (L160) |
| **`#493` — la sombra de `/app`** (y de un pack): una copia de stdlib en `/app` o un pack de librería tapa a `/lib` sin ruido. Dos mitades: el aviso en `OUTPUT`, y la decisión con `#500`/`#496` (que el pack builder no empaquete stdlib, o que el resolutor no deje que un pack tape a `/lib`). La mitad del runner ya está (`T1`, `843e2d0a`) | `FICHAS` `#493` (L203) |
| **`#441` — el `.mdn` no recuerda su RECETA**: huella de los flags de compilación en la cabecera, comparada al cargar como ya se compara `arch`. Mismo salto de formato que el resto de `#502` (§E): hacerlas juntas | `FICHAS` L11637 |

## C · Ficheros: el cursor y la clase `File`

| asunto | dónde |
|---|---|
| **`#491` — EL CURSOR**: abajo, descriptores en la fachada de FS (abrir / `seek` absoluto y relativo / leer / escribir / cerrar); arriba, **`File`** (bytes) + **`TextFile`** (UTF-8) como objeto en el heap con su buffer dentro, y el destructor nativo que cierra al recoger. Con él caen `read_stream` sólo en littlefs (`#473`), la escritura en streaming (`#475`) y las aperturas duplicadas (`FF_FS_LOCK 0`). Quedan por escribir antes de codificar: endianness de `write(n)`, modo de apertura, origen del `seek`, `readLine` al final, extender más allá del fin | `FICHAS` `#491` (L224) |
| **`/sd` como prefijo RESERVADO**: sin tarjeta, error en vez de caer en silencio a la flash interna (decidido 23-ago «entra en V6»; no se hizo). ⚠️ Cambio de comportamiento: a las notas de versión | `FICHAS` L10417 |
| **`SD_INFO`/`SD_MOUNT` al REPL común** — siguen sólo en `pico/repl_v1.c` (resto de V5/H6) | `FICHAS` L11903 |
| **La SD del STM32**: el motor es común; falta la cintura SDMMC de la U5 y montar `/sd` | `FICHAS` L6310 |
| SD «superfloppy» sin MBR: probar con una tarjeta real reformateada (resto de H2-P5; en PC ya está) | `FICHAS` L1547 |
| **`listDir` en la VM-C** — a medio nacer: id y código en miVM, sin declaración BP y sin C (resto de `#484`; `input()` se queda fuera por decisión del 26-ago, y `PENDIENTES` mete a `listDir` en esa misma decisión: corregirlo) | `FICHAS` L4236 |
| `#456` resto — **el tope de un path es de la VM-C, no del lenguaje**: `BPVM_FS_PATH_MAX` = 256 sólo en C; miVM abre lo que sea. Que el tope sea de la spec, lo compruebe miVM igual, y un sample de paridad lo fuerce | `FICHAS` L9476 |

## D · La caracterización de los periféricos de cada micro — la HAL BP

| asunto | dónde |
|---|---|
| **`#378` — que cada micro DIGA lo que tiene**: `GPIO_COUNT`, `ADC_CHANNELS`, `PWM_SLICES`… dejan de ser `#define` y pasan a ser una pregunta al micro, con la **unidad** explícita (el RP2350 dice 24 canales de PWM en `INFO` y 12 *slices* desde BP); `INFO` y lenguaje de la misma fuente; poder preguntar si un pin concreto existe. La madre de las dos siguientes | `FICHAS` L11853; `V5_IDEAS` (6-ago) |
| **`#470` — los CUATRO NÚMEROS → arrays por micro**: *«para cada micro unos arrays, `[0, 1, 3]`, que indican los números válidos de UART, I2C, SPI, ADC, PWM…; el número es la longitud del array»* (Eduardo, 9-sep). Absorbe la pieza 2 de `#480`: el **`Pulse` del C3 cuenta 0 para siempre** (único ESP sin PCNT) | `FICHAS` `#470` (L738); `#480` (L3492) |
| **`#479` — el mapa de pines y los analógicos**: el mapa por familia compilado en la imagen (no en el ENV); el estado del pin en el común; `Pin` con `INPUT/OUTPUT/OPEN_COLLECTOR/ANALOG` y `AnPin extends Pin`. Absorbe el **ADC del STM32** (anuncia 20 canales, 0 backend; resto de `#469`) y los restos de pines de `#473`. Toca `Adc.bp`/`Gpio.bp` embebidos → ABI y blobs | `FICHAS` `#479` (L845); `#469` (L3408) |
| **`#471` restos — `Machine`**: `manufacturer()`, PSRAM y RAM, y `MAX/MIN_CPU_MHZ` **por micro** (hoy los del RP2350 para las cinco familias). Y de la librería genérica: quitar `variant()` («B» si `gpioCount() >= 40`, que contesta «B» en un S3) y definir «este micro no tiene eso» | `FICHAS` L4529, L11893 |
| **`#487` — el breadcrumb mudo en 4 de 5 familias**: implementarlo en RP2350 y ESP32 o declarar que no está; y en cualquier caso la fachada distingue «slot NULL» de «nada que informar» | `FICHAS` `#487` (L684) |
| **NeoPixel de verdad en ESP32 y STM32** (RMT/`led_strip`; hoy sin backend lanzan excepción BP, `3d14e32c`) | `FICHAS` L3402 |
| **El nombre `pico` en la capa C**: `bpvm_pico.h`, `bpvm_pico_set_backend`, 29 `BUILTIN_PICO_*` — `#471` arregló sólo la capa de usuario. Huérfano que `#473` pide con ficha nueva | `FICHAS` L4815 |
| **El tamaño de flash lo dice la placa** (ESP32 y STM32): sobre grande + JEDEC en runtime, como el RP2350 desde `#292`; nadie lee `flashSizeBytes` del ENV | `FICHAS` L11855 |
| El botón BOOT (GPIO9) del C6 como entrada — un «quizá» de `P2` que cayó en silencio | `FICHAS` L8186 |

## E · AOT y código nativo

| asunto | dónde |
|---|---|
| **`#502` resto**: el destino **`riscv32-esp-c`** (`rv32imc`, `ilp32`) en `NpackReloc.DESTINOS`, elegido por el `floatAbi` del `INFO`; y la **float-ABI en la cabecera del `.mdn`** (sección `native` del `.mod`). El cargador ya rechaza (12-sep); el IDE sigue compilando con la ABI del P4 | `FICHAS` `#502` (L3868) |
| **`N1` restos**: leer un campo de **tupla** desde native; **`try`/`catch`** dentro de una native (el `eh_stack` no sabe de un manejador nativo); y **la paridad dual-VM no cubre el camino AOT** — sólo `make test-aotnew` en host (seis fallos de `N1.4` pasaron por 109+39 pruebas) | `FICHAS` L11509, L11223 |
| **`#379` resto — una `native` larga no atiende el Stop**: `kill_requested` sólo se mira entre instrucciones del intérprete. Diseño en `V7_IDEAS` §«¿Podemos parar un código AOT?» | `FICHAS` L1287; `V7_IDEAS.md` |
| **`#426` restos** — lo que la ficha pedía (que `double` cruce a una `native`) **se hizo en V6** por helpers y la viñeta de §Aplazadas no lleva su ✅. Quedan: la conversión `double(x)` escrita DENTRO de una native sigue rechazada; cronometrar `double` native contra interpretado (≈3× estimado, nunca medido); decirlo en el manual; y la instrucción nativa donde haya FPU doble (hoy ninguna placa del sobre) | `FICHAS` L11906, L10896 |
| **`#468` — la stdlib en flash (XIP) en vez de en RAM** (la C6): decidir si se prueba el XIP del blob embebido salvo que `/lib` la sustituya; la tabla de símbolos por referencia; *«si LVGL sube a un pack, ese pack lleva `Gui.mod`»* | `FICHAS` `#468` (L932) |
| **El código nativo empotrable — el diseño** (encargo del 21-ago: *«código independiente que ya sabemos recolocar; lo que hay que mejorar es cómo publica su API y cómo interactúa con el resto»*), en cuatro puntos: (1) **reja en `publica()`** — leer marca y versión de la tabla en vez de aceptar un `const void*` (una tarde, con test); (2) dependencias entre códigos nativos con **`busca()` perezoso** en el primer uso (propuesta, sin decidir); (3) si las **cuatro ranuras** (`BPVM_PACK_MAX 4`) valen cuando crezca el catálogo — medir antes; (4) **la arena de RAM por código nativo**, no por pack: hoy es singular y se llama `SQLite`, y los `malloc` de la BIOS son chivatos | `V7_IDEAS.md` §1; `V6_IDEAS.md` §arena |
| **LVGL a un pack — el primer cliente**: contrato bidireccional y con estado (`flush_cb`/`read_cb`/eventos), qué pasa con `src/gui.c`, el pack lleva `Gui.mod` (`#468`), y el driver del pack pone la pantalla del usuario (`P2.3`); se mira junto a `#434`. *«Merece un trabajo separado, con una evaluación previa»* (26-ago) | `V7_IDEAS.md` §1; `FICHAS` L8243 |
| **Los cabos sueltos del AOT** — en qué línea falló el nativo (`bpvm_aot_fault_t` viaja sin origen), **parar/expropiar un thunk** (hoy un bucle infinito nativo sólo se recupera desenchufando), y el debugger que se pierde dentro. Propuesta de Eduardo (24-ago): **una marca (safepoint) que sirve para las tres cosas**, y el primer paso es MEDIR su coste en los saltos hacia atrás con `samples/NatV7.bp` (1 ms en la Pico). Sin fecha: *«todo eso para el futuro»* | `V7_IDEAS.md` §2 |
| La cola de la **fusión `.mod`/`.mdn`**: el `.mod` multifamilia (hoy N=1: el IDE funde sólo el blob de la placa conectada), la poda como UNA función (Run y packs), `putIfChanged` comparando lo podado | `FICHAS` L11519; `V6_IDEAS.md` §.mdn |
| **El `INFO` no publica la ABI de helpers** (`MDN_ABI_VERSION`): un `.mdn` de ABI vieja se rechaza en el log de la placa y el programa cae a interpretado sin ruido — así se tapó `#504` tres semanas | `FICHAS` L11129 |

## F · Memoria, arranque y arquitectura

| asunto | dónde |
|---|---|
| **`U6` restos**: **coser las dos regiones del C3** (136 → ~250 KB útiles; las cuatro comprobaciones del hueco y `coser=0|1` en el ENV), el **margen de la Pico como MEDIDA** (64 KB a mano desde julio) y el del STM32, remedir el S3 y añadir su caso a `test_mem`, la clave `vmheap=N` que no entró, el +10 % del `AllocBench` del C3, y medir un nativo sobre PSRAM | `FICHAS` L2225, L2520, L2587, L2820 |
| **`U6.F1` — con PSRAM, qué buffers NO-BP agrandar** con la SRAM ociosa (421 KB en la Metro): estudio aparcado — *«me sigue chirriando tener RAM ociosa»* | `FICHAS` L2306 |
| **`U1.4` — UNA cintura de flash por familia** para los cuatro consumidores (packs, env, FS, log); hoy ~12 cableados de las mismas dos operaciones | `FICHAS` L8350 |
| **Boot 1 / Boot 2**: dar nombre a la frontera familia/común, **peldaño de packs** en `bpvm_boot.h` (hoy 4 peldaños y cada familia monta los packs por su cuenta), y **declarar las capas del SO común** (los packs pertenecen al SO: son backend del FS) | `FICHAS` L10638, L10673 |
| **Unificar el montaje de packs**: paso común + `mapear()` por familia (el S3 sigue sin `pack_s3.c`) | `FICHAS` L10772 |
| **[P4] los 32 MB de flash y el XIP de los packs**: el caché mapea 24 bits; dos caminos (invertir el orden común, o segunda partición sólo en el P4), sin elegir; hoy 16 MB y packs en 2800 KB | `FICHAS` L11655 |
| **`[ISA]` `CALL_REL`** — el CALL local PC-relativo, que retira la base `cb` «transitoria» del XIP (desde `#307`, jul) | `FICHAS` L11885 |
| **Migrar la Pico a `src/platform_freertos.c`** — parada por un 32 % de `PrintBench` sin explicar; **paso 0: remedir** (la medida es anterior a `#485`). Es el paso 0 de fundir los tres backends de FreeRTOS (`#473` R1+R16) | `FICHAS` L4682; `docs/FUSION_BACKENDS_FREERTOS.md` |
| **Muro de contención entre el heap y las pilas** (idea de V4; desde `#451` la tabla de handles vive entre los dos) | `FICHAS` L11883 |
| **Diagnóstico del heap DESDE BP**: `heapFrag`/`heapMap` y los contadores del GC como **números**, en las dos VMs (hoy sólo miVM y como cadena); intrínsecas nuevas → van con el módulo raíz | `FICHAS` L11880; `V5_IDEAS` (5-ago) |
| `#356` — el descarte mudo del alocador (resto rebajado de `#357`): el cuerpo de la ficha no está en el repo; reconstruir qué se descarta o cerrar por no reproducible | `FICHAS` L11890 |
| `#494` resto — el guardián del heap del GC (`HEAP INCONSISTENTE` / `DESCARRILADO`): ¿fatal o aviso? | `FICHAS` L4000 |
| `scheduler_smp.c` llama al `poll_cb` sin la guarda `vm->io == NULL` (latente: SMP es opt-in); **decidir si `--smp=2` va por defecto** en el host (×1,7–1,9 medido, 0/40 cuelgues tras `#474`); y el **modelo final del SMP**: VMs ociosas «a piñón» o parqueadas (medir la latencia de despertar antes) y los workers como cola de hilos de SO en vez de N fijos — rumbo de Eduardo del 9-sep | `ESTADO` 5-sep; `FICHAS` L6850; `V6_IDEAS.md` §SMP |

## G · IDE y protocolo wire

| asunto | dónde |
|---|---|
| **`#495` — `GET`/`LS` durante un RUN**: `GET_REPLY` entero bajo `tx_mtx`, decidir el cerrojo del `read_stream`, levantar el `BUSY` en los cinco `poll` sólo para `LS`/`GET` | `FICHAS` `#495` (L176) |
| **`#452` — repasar los 29 verbos del wire** con «¿a cuál de los tres consumidores sirve?»: **`FORMAT` falta hoy desde el IDE**, `RENAME`, `RMDIR`, `PING`, `DF` duplicado. La condición («cuando exista el script») se cumplió con `T1` | `FICHAS` `#452` (L1102) |
| **`E1` restos**: el **rojo** del árbol por color (ficheros con problema — decidir qué cuenta); el `EXITED` de un `RESET` dice «terminado por KILL» (una línea); la **vista de packs en el árbol** (`LIST` sin fallback al pack: `/lib` no aparece aunque el RUN lo resuelva) | `FICHAS` L7427, L11760, L10266 |
| **El `INFO` de las ESP32 y STM32** sin `packsXipBase`/`sqlite*`/`floatAbi` (`info_extra` sólo en la Pico); el IDE ya los pinta si llegan | `ESTADO` 12-sep |
| El guardián del fat-jar (`#429`) no vigila la **miVM embebida**, sólo el compilador | `ESTADO` 5-sep |
| Confirmar desde el IDE 6.0 que `Run on Device` sube `Gui`+`Json` solo (C6 con `/lib` vacío) — una comprobación | `FICHAS` L8238 |

## H · Pruebas y medida

| asunto | dónde |
|---|---|
| **`T1` restos** del runner: «dónde aplica» por capacidad (`necesita: ["sd"]`); **el sello miente** (`serverBuild` es la hora de `main.c`: `imageCrc`/git-rev en `HELLO_REPLY`); el descubrimiento necesita > 3 s tras un reset en P4/DK2; crecer el corpus «cuando haga falta». Y el aviso del 7-sep: el runner compara host contra placa, **no contra lo esperado** | `FICHAS` L5934 |
| **Lo que `V7_IDEAS` pedía del arnés y `T1` no hizo**: `memoria: APRETADA` en el manifiesto (la matriz no es verde sin la celda de la placa que más aprieta: Pico 2 / C6); el criterio «no determinista» por aserciones (para `synclisttest` y similares, hoy en ningún corpus); **recuperar la placa colgada** con `RESET` por el wire (o DTR/RTS donde lo haya) y pedir el **log post-mortem** después — hoy `KILL` y abortar la placa; y comprobar que la C6 y el STM32 se resetean **sin manos** | `V7_IDEAS.md` §4 |
| `L15` — la tabla de resets del log post-mortem está rancia (el STM32 sí se comprobó en H13); lo no medido es el **botón físico** de reset en Metro (`RUN`) y STM32 (`NRST`) | `PENDIENTES` L15 |
| **`compat.sh` no vigila el código de salida** (resto de `#481`): los `1` de `ThrowSinAtrapar` están medidos a mano; y el camino «el módulo no exporta `RuntimeError`» sin comparar | `FICHAS` L3679 |
| `#485` resto — **medir** en el P4 el KILL mientras calcula (`io` a la prioridad de la VM); el único número (17 ms) es anterior al arreglo | `FICHAS` L4290 |
| `U3.25` — `MKDIR`/`RMDIR` reales **en placa** (el «verificado» del 30-ago fue con el stub) · `U1.2` — que el post-mortem de la Pico **sobrevive al reinicio**, anotado · `P1.C3` — `trytest.bp` y `JsonDemo` en el C3 | `FICHAS` L10209, L8315, L7830 |
| **`make test-listtrunc` con el CONTROL en rojo** (13-sep, comprobado): 16 carpetas → 15 entradas, `omitted=1` por debajo del tope | `FICHAS` L9490 |
| **Batería de rendimiento HW+SW** — medir el REPARTO (VM / HAL / hardware) por placa; ya hay datos sueltos (Nucleo vs Discovery). Es la misma cosa que **el eje 4 del censo funcional** (memoria y tiempos por sistema, *«nada medido a este nivel»*), el único de los cuatro ejes del 17-ago que V6 no hizo: una entrada, no dos · **Prueba de resistencia larga** — días de carga variada con marca periódica en el log-anillo (RUN largo Y muchos RUN) | `FICHAS` L11896, L11898; `CENSO_SISTEMAS_V6.md` §eje 4 |
| **El simulador con disfraces** — `bpvm-sim --familia=` (bloque de borrado, vista de packs, AOT); parte de la motivación cayó con la unificación | `FICHAS` L10599 |
| **Inyección de eventos en el device** (resto de `#475`/`C1`): la captura en placa sólo ve la pantalla inicial; el host tiene guion, la placa sólo el dedo · `Window.load()` **hornea el tamaño del panel** en el `.win` (no portable entre pantallas) | `FICHAS` L5506, L5179 |
| Limpieza: los 11 `.mod` de stdlib **hermanos** en `bpgenvm-c/samples/` que nadie refresca (un run manual carga stdlib rancia) · `samples/out` con la stdlib MOD6 de V5 (resto de `#467`, sólo `rm`) · los `.mod` de prueba que quedaron en las placas (`/app/P.mod`, `adc.mod`…; con OK placa a placa) | `ESTADO` 25-ago; `FICHAS` L3336 |

## I · Placas y drivers

| asunto | dónde |
|---|---|
| **`#490` — bajo consumo, el ESTUDIO**: censo hecho (`BAJO_CONSUMO_CENSO.md`: ninguna placa duerme); faltan la línea base por placa, los mA (sin amperímetro no hay números), la tabla reloj/modo por micro y por qué el *tickless* no sirve. Punto de partida decidido: despertador por temporizador + escalera progresiva + dos regímenes. Hallazgos anexos dentro de la ficha: STM32 sin tarea ociosa, `sleepUs()` en espera activa, `MIN_CPU_MHZ` declarado y no implementado, 19 periféricos siempre encendidos en la DK2 | `FICHAS` `#490` (L414) |
| **`Rtc` por software en Pico y ESP**: con autorun nadie calibra y `epochSec()` devuelve segundos-desde-boot como si fuera la hora, sin forma de distinguirlo desde BP | `FICHAS` L671 |
| `#466` resto — la Pico dice `causa del reset = WATCHDOG` tras un `RESET` pedido por el wire (una línea en `main.c:1596`; `resetCause` ya distingue) | `FICHAS` L3194 |
| **STM32**: la única configuración se llama `Debug` aunque compile a `-Os` y sea la de publicar (renombrar o no; tocar `PUBLICAR.md` y el script) · **el TOTAL del FS baila** entre arranques (516096 ↔ 614400, 26-ago) y el boot no dice de dónde sale la tabla de particiones | `FICHAS` L8410, L10270 |
| El `sdkconfig` generado del C3 está **trackeado** (S3, C6 y P4 lo ignoran): `.gitignore` + `git rm --cached` | `FICHAS` L7969 |
| Cosmético: el log del bootloader sale por el USB nativo **antes del `HELLO`** (C6/C3); el IDE lo salta | `FICHAS` L8241 |
| El ENV de la Pico 2 **cambió solo** al flashear (28-ago; FS 1512→1024 KB): sin causa ni reproducción — si vuelve, capturar el log del arranque | `ESTADO` 28-ago |
| **[P4] segunda imagen para el silicio nuevo (P4X)** manteniendo las dos · **ESP32-S31** (RISC-V, sucesora de la S3) cuando haya placa, por el camino de `P1.C3` | `FICHAS` L11696, L7641 |
| **GUI**: `#477` resto — los tres agujeros del oráculo `__guiDumpTree` (el chart no vuelca datos, ciego a la rotación en la DK2, color/fuente sólo de render) · `G2` resto — `owner` en las properties de la ventana y un `~Component` que libere el nodo (necesita el destructor); `toJson()` no emite min/max, options, colores, ni Chart/Table/ImageView | `FICHAS` L5036, L5103 |
| **GUI — una cola de tareas hacia el hilo que bombea** (el `invokeLater` de Swing) y la vía intermedia de Eduardo: una clase base **`Command`** en la stdlib (`ejecutar()`/`habilitado`, encolable). La reentrada de LVGL que lo motivaba ya la cerró `G1`; queda la utilidad de «ejecutar esto en aquel hilo» — y si entra, entera de una vez (una base nueva corre las ranuras de la vtable) | `V7_IDEAS.md` §3 |
| **Los 10 «rompe-el-modelo» vivos de `#473`** que el triaje pedía con ficha nueva y no la tienen: `RUN` ×4, sink de `OUTPUT` ×5, puerta de arranque ×3 con agujeros, fachada FS↔littlefs, clasificación de fallo en 1/3, `board_mgr_*` ×3, `lv_conf.h` duplicado, `setRotation` en la DK2, el camino de error de `scheduler_smp.c` que no une workers, y los lotes D (FS, 15) y E (GUI, 10) de `473_TRIAJE_COLA.md` | `FICHAS` `#473` (L4597); `docs/473_TRIAJE_COLA.md` §3 |
| El DMA de `Uart` donde la familia lo tenga (V6 dejó el anillo por IRQ) | `FICHAS` L4798 |

## J · Fuera del plan de versiones (decidido; se lista para no perderlo)

- 🧊 **`A4` — los dos núcleos** (*«no es V6 y probablemente tampoco V7; hay muchas cosas antes»*, 9-sep). Inventario hecho; antes de activar: `kill_requested` atómico, el `printf` de la Pico comparte USB con el wire. Con él van **las dos VM sobre una cola de threads BP** (el camino SMP rehecho sobre `io`) y **borrar `comm_pico.c`/`comm_host.c`** (`A1.4`, inertes). → `FICHAS` `A4` (L6915)
- 🧊 **`#488` — el AOT no genera Xtensa** (*«de momento no»*): el S3 corre todo interpretado. Con él, `bios_s3.c` (sin consumidor) y los **packs nativos en S3/STM32**. → `FICHAS` `#488` (L4063)
- **La red en placa** (backend WiFi/Ethernet de `Net`): **V8**, rumbo del 10-ago. → `FICHAS` L6310
- **Linux**: el IDE en Linux + la Raspberry Pi como placa — `[SIN VERSIÓN, V6+]`. → `FICHAS` L11901
- `Gui.setBrightness` / PWM del backlight — «no tiene a quién servir» (4-sep).
- «Faltan módulos» al ejecutar desde `run` de consola tras el IDE — en suspenso, sin reproducción (la asimetría del `basedir` sigue: `fs_facade.c:98`).
