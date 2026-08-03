# H13 — Guión de pruebas finales de V4

> Última verificación antes de publicar. **3 familias · 7 placas**, barrido completo:
> no dos pruebas y ya. Estimación **2-3 días** según las sorpresas (Eduardo, 3-ago).
> **Código congelado**: sólo se corrige lo que esté ROTO; toda mejora va a V5. Un bug
> que salga aquí **bloquea la publicación** — la fecha cede, la calidad no.
> Marca `[x]` según avances; lo que falle va al **Registro de hallazgos** del final.
> Guiones históricos: `H14_TEST_PLAN.md` (V2) y `V3_TEST_BATCH.md` (V3).

## Las 7 placas

| | Placa | Familia | Firmware | Pantalla |
|---|---|---|---|---|
| **a1** | Pico 2 | RP2350A | `pico` | — |
| **a2** | Metro RP2350B | RP2350B | `pico` (misma imagen) | — |
| **b1** | ESP32-S3 DevKit | ESP32 | `esp32` | — |
| **b2** | ESP32-P4 Kit | ESP32 | `esp32p4` | MIPI-DSI 1024×600 + táctil |
| **b3** | ESP32-P4 Waveshare 4.3" | ESP32 | `esp32p4` (**la misma que b2**) | ST7701 480×800 |
| **c1** | STM32 Nucleo-U575ZI-Q | STM32 | `stm32` | — |
| **c2** | STM32 Discovery U5G9J | STM32 | `stm32` | LTDC |

Y dos bancos que **no son placas pero se publican igual**, así que entran al final:
el **micro simulado** y la **VM Java** del PC.

---

## Puerta 0 — en el PC, antes de tocar una placa

Gratis y encuentra regresiones sin gastar un flasheo. Si esto está verde, lo que quede
en placa es plataforma (backend HW, boot, wire), no el núcleo del lenguaje.

- [ ] `compat/compat.sh check` → paridad dual-VM **28/0/0**
- [ ] `make test-env test-part test-boot test-bmgr test-pack test-crc` en `bpgenvm-c`
- [ ] `make sim-smoke` (wire v1 completo contra el simulado)
- [ ] `mvn test` en `miVM` → 34/34
- [ ] Compilar los 5 firmwares y que **no** salga ningún aviso nuevo

---

## Paso 0 — firmware FRESCO, y desde `dist/firmware/`

> **La regla (Eduardo, 3-ago): lo que se prueba y lo que se publica es el MISMO fichero,
> bit a bit.** Cada imagen se genera UNA vez, se copia a `dist/firmware/` y se sella
> (`sha256sum`). A partir de ahí se flashea SIEMPRE desde ahí, y de ahí salen los
> adjuntos de la release. Si hay que recompilar algo, se vuelve a sellar **y se vuelve a
> probar** lo que dependa de ello. Detalle en `dist/firmware/README.md`.

**Son 5 imágenes para 7 placas** — dos sirven a dos placas cada una, y no es un atajo:
la variante se decide en runtime (el RP2350 se identifica solo; el panel del P4 sale del
ENV, #311).

> ⚠️ **No reutilizar imágenes viejas.** V4 ha cambiado el modelo de memoria (handles),
> el arranque (kernel por capas), el FS (littlefs en las 3 familias), el formato `.mod`
> (v6, sin `.bpi`) y los blobs de stdlib embebidos. Una imagen anterior da **falsas
> regresiones** — y ayer ya nos costó una cacería de un bug que no existía.

- [ ] **a1/a2** RP2350 — imagen única, la genero yo (`ninja -C pico/build`)
- [ ] **b1** ESP32-S3 y **b2/b3** ESP32-P4 (una sola imagen) — **las compilas tú**
      (`idf.py build`): es el único toolchain que no tengo. Bloquea el grupo b entero,
      conviene lanzarlo antes de empezar
- [ ] **c1/c2** STM32 — las genero yo (CubeIDE headless). Son DOS imágenes distintas
- [ ] Con las 5 en su sitio: `cd dist/firmware && sha256sum *.uf2 *.bin > SHA256SUMS.txt`

---

## Batería común (aplica a las 7)

### A · Arranque y conexión
- [ ] A1 Arranca a **estado 3 (app)**; el log no trae nada raro
- [ ] A2 `INFO` con los datos correctos de ESA placa (micro, GPIO, flash, PSRAM)
- [ ] A3 Causa del reset correcta (power-on / pin / watchdog / software)
- [ ] A4 Conectar y desconectar 3 veces seguidas sin quedarse colgado
- [ ] A5 `RESET` desde el IDE → vuelve y reconecta

### B · Sistema de ficheros
- [ ] B1 `LIST` del árbol completo (`/app`, `/lib`, `/sys`)
- [ ] B2 Subir un fichero pequeño (< 8 KB, un solo viaje) y releerlo idéntico
- [ ] B3 Subir uno **grande** (> 8 KB → streaming) y releerlo idéntico ← chunk nuevo de #338
- [ ] B4 Sobrescribir un fichero existente
- [ ] B5 Borrar, renombrar, crear y borrar directorio
- [ ] B6 `DF` coherente con lo que hay
- [ ] B7 Apagar y encender: **todo sigue ahí** (persistencia real)
- [ ] B8 Formatear el FS y comprobar que se rehace solo

### C · Ejecución
- [ ] C1 `T` — hilos (2-3, que es el caso real)
- [ ] C2 `JsonDemo` — strings, objetos, parsing
- [ ] C3 `FileTest` — FS desde BP
- [ ] C4 `AsyncDemo` — `Thread(obj::metodo(args))` (#325)
- [ ] C5 `Ev*` — eventos (#H5.c): `EvFull`, `EvOrder`, `EvMulti`, `EvThrow`
- [ ] C6 Excepciones: `try/catch` de un `RuntimeError` nativo, y uno de usuario
- [ ] C7 **OOM atrapable**: forzar sin memoria y que salga `RuntimeError` (no un reset)
- [ ] C8 Ejecutar el mismo programa **3 veces seguidas**: misma salida, sin arrastre
- [ ] C9 `Stop` (Ctrl+F2) de un programa en marcha → corta y la placa sigue viva

### D · Gestión de placa (H9 — nuevo en V4)
- [ ] D1 Panel abre y muestra el estado del arranque
- [ ] D2 Leer variables de entorno
- [ ] D3 Editar una y **reiniciar**: persiste
- [ ] D4 Borrar una
- [ ] D5 `Proponer defaults` de particiones
- [ ] D6 Aplicar tamaños válidos → reinicia y arranca con el reparto nuevo
- [ ] D7 Aplicar un tamaño **inválido** → error claro y el entorno queda **intacto**
- [ ] D8 Con la placa a medio configurar, el IDE ofrece abrir el panel al conectar

### E · Packs (H3 + #310 — nuevo en V4)
- [ ] E1 Grabar un pack de librería en la zona de packs
- [ ] E2 Listarlo: nombre, tamaño, **fecha de compilación**, ficheros que trae
- [ ] E3 `import` de un módulo del pack desde un programa
- [ ] E4 Un `.mod` suelto en `/lib` **eclipsa** al del pack
- [ ] E5 Retirar un pack: deja de usarse al momento
- [ ] E6 Formatear la zona
- [ ] E7 Persistencia tras apagar
- [ ] E8 **Pack EJECUTABLE (#310)**: `samples/sampleproject` (`main: App`) → grabar y
      ejecutar desde el pack, leyendo un recurso de dentro

### F · Depuración
- [ ] F1 Poner un breakpoint y que pare ahí
- [ ] F2 Step (línea a línea)
- [ ] F3 Panel de variables con valores correctos (#341)
- [ ] F4 Call stack
- [ ] F5 Continuar hasta el final

### G · Autónomo
- [ ] G1 Fijar `autorun` y reiniciar → arranca solo
- [ ] G2 La **ventana de rescate** (#345) responde a HELLO/Stop antes de lanzar
- [ ] G3 Quitar el autorun

### H · Memoria (lo más nuevo de V4)
- [ ] H1 `INFO` recién arrancada: apuntar heap / pila / RTOS libre
- [ ] H2 Correr carga (T + JsonDemo + un GUI si la placa tiene) y **repetir INFO**
- [ ] H3 **El heap no crece entre RUN sucesivos** (#357) — 20-30 vueltas
- [ ] H4 El guardián de fin de RUN (#339) no grita
- [ ] H5 #338: el buffer de 8 KB no estorba ni a la subida ni al panel de gestión

### I · Recuperación
- [ ] I1 Desenchufar a media escritura y volver: el FS monta
- [ ] I2 `LOG_DUMP` tras un reset: el log **sobrevive**
- [ ] I3 `LOG_CLEAR` y comprobar que se limpia

---

## J · La instalación desde cero — UNA vez, no por placa

> La prueba más valiosa de todas, porque es la única que ve **lo que ve alguien que
> acaba de llegar**. Y porque los arreglos de distribución (encontrar `packs/`,
> `bpgenvm-c/`, el ejecutable del simulado, resolver rutas del `.cfg`) **nunca se han
> ejecutado desde una instalación de verdad** — sólo desde el árbol de fuentes, que es
> donde todo está en su sitio por casualidad.
>
> Se hace **desde el ZIP sellado**, no desde `BpIde/target/`.

- [ ] J1 Descomprimir el ZIP en una carpeta cualquiera. **Con espacios en la ruta**
      (p.ej. `C:\Mis Programas\BasicPlus 4.0\`): ahí es donde se rompen las cosas
- [ ] J2 Arrancar con `bpide.bat` **sin configurar nada**
- [ ] J3 El título de la ventana dice **`BpIde 4.0`** (#367) — ni `[H9]` ni `dev`
- [ ] J4 **F1** abre el ÍNDICE, y desde ahí se navega a los **cinco** volúmenes y se
      vuelve. Las imágenes se ven (#363)
- [ ] J5 Compilar y ejecutar un sample en la VM del PC
- [ ] J6 Arrancar el **micro simulado** y ejecutar algo con GUI — esto prueba que
      encontró `bin/` y `packs/`
- [ ] J6b **Compilar un programa que importe la stdlib** (`samples/ExcCatchTest.bp`:
      `import Core` + un módulo propio con clases y excepciones). Este paso es el que
      faltaba: J2 comprobaba que el IDE *encuentra* sus carpetas, no que *compile*
      algo de verdad, y por ahí se coló que el ZIP no llevaba `BpVM.cfg` — ver el
      hallazgo 6
- [ ] J7 Conectar una placa y hacer un Run
- [ ] J8 **Intentar un AOT SIN compilador instalado.** Resultado esperado: aviso claro
      y el programa corre **interpretado**. Si sale un error feo, es un fallo: ahí se
      espanta al que acaba de llegar
- [ ] J9 Instalar el toolchain de Arm y reintentar → lo **detecta solo** (§5.1)
- [ ] J10 Si no lo detecta, configurarlo a mano en **Project → AOT (toolchain)…**
- [ ] J11 Mover la carpeta ENTERA a otro sitio y arrancar: sigue funcionando
- [ ] J12 Mover **sólo el jar** fuera: debe quejarse con sentido (la guía avisa de esto)

**Lo que más me interesa de este bloque son J2 y J8**: el primero ejercita los cuatro
arreglos de distribución de golpe, y el segundo es el único punto donde un mensaje malo
tiene coste real — alguien que instala por primera vez y se encuentra una traza.

### Lo ya adelantado sin GUI (3-ago)

El ZIP se descomprimió en una ruta **con espacios** y se ejercitó desde ahí, con el
directorio de trabajo **en otro sitio** (`C:\Windows`) para que nada colara por el cwd:

| Comprobado | Cómo | Resultado |
|---|---|---|
| J1 | descomprimir en `…\Mis Programas\BasicPlus 4.0\` | ✅ |
| J2 (la parte de rutas) | sonda que llama a `installDir`, `docsDir`, `packsDirEffective`, `SimRunner.locateExe`, `AotBuild.autodetectBpgenvm` | ✅ los cinco resuelven a la instalación |
| J3 (el valor) | `Implementation-Version` del manifest del jar instalado | ✅ `4.0` |
| J4 (los ficheros) | índice + los cinco volúmenes + comprobador de enlaces del montaje | ✅ cero enlaces rotos dentro del paquete |
| J6 (el arranque) | `bin\bpvm-sim.exe` lanzado desde la instalación | ✅ arranca, resuelve su `SDL2.dll` y escucha (0,5 s) |

**Sigue pendiente de ojos**, porque es GUI o necesita hardware: J2 arrancando de
verdad, J3 leyendo el título, J4 navegando y viendo las imágenes, J5, J7, J8–J12.

Una advertencia del camino: la sonda dio `Implementation-Version = null` la primera vez.
No era el jar — era **la sonda**: al vivir en un directorio del classpath, era ella
quien definía el paquete `com.mycompany.bpide`, y un paquete definido desde un
directorio no tiene manifest. Metida DENTRO del jar, `4.0`. Instrumento mudo, dudar de
él primero.

---

## Matriz placa × bloque

Estado: `[x]` pasa · `[ ]` pendiente · `—` no aplica

| Bloque | a1 Pico | a2 Metro | b1 S3 | b2 P4 Kit | b3 P4 ws | c1 Nucleo | c2 Discovery |
|---|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| A arranque/conexión | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| B ficheros | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| C ejecución | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| D gestión de placa | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| E packs | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| F depuración | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| G autónomo | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| H memoria | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| I recuperación | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| **GUI** | — | — | — | [ ] | [ ] | — | [ ] |
| **AOT nativo** | [ ] | [ ] | — | [ ] | [ ] | [ ] | [ ] |

---

## Lo específico de cada placa

### a1 · Pico 2 (RP2350A)
Variante A: 30 GPIO, 4 MB flash, sin PSRAM. **AOT ARM**.
- [ ] `PicoInfo` — ADC/PWM/temperatura reales
- [ ] GPIO: `blink` en GP25
- [ ] AOT: un cómputo `native` y comparar con el host (misma salida, mucho más rápido)
- [ ] #338 ya ✅ verificado 2-ago; se repite dentro del barrido
> ⚠️ **Sin botón de reset**: sólo se recupera desenchufando. Antes de una prueba que
> pueda colgarla, ten a mano la imagen buena.

### a2 · Metro RP2350B
Variante B: 48 GPIO, 16 MB flash, **PSRAM 8 MB**, NeoPixel. **Misma imagen que a1.**
- [ ] La variante se detecta sola (48 GPIO, 8 ADC en `INFO`)
- [ ] **El heap va a PSRAM** y se ve en `INFO`
- [ ] Clamp #292: la flash que dice `INFO` es la real
- [ ] `NeoDemo` — WS2812 por PIO
- [ ] Con PSRAM, subir un fichero **grande de verdad** (100 KB+)

### b1 · ESP32-S3 DevKit
Wire por UART0; consola por USB nativo. Xtensa: `native` **interpretada**, sin AOT.
- [ ] **#331 — la causa sin probar.** La placa escribe tras `erase-flash`, pero nunca
      se demostró por qué. Hipótesis: bootloader viejo de 2 MB. Es el único pendiente
      con riesgo de publicación: si es eso, le pasará a cualquiera con un S3 así.
      Mirar en el log la línea `flash: configurada N KB | chip físico M KB`; si no
      cuadran, **reflashear bootloader** y confirmar
- [ ] DRAM: el bloque de 160 KB (#336) sigue dando de sí con carga

### b2 · ESP32-P4 Kit
PSRAM 32 MB, MIPI-DSI **EK79007 1024×600**, táctil **GT911 (I2C 0x14)**, Ethernet IP101.
**AOT RISC-V con `.mdn` dinámico** (#H4, 113×).
- [ ] `GuiColorDemo` — se ve, con sus colores
- [ ] `GuiClickDemo` / `GuiTableDemo` — **táctil** respondiendo
- [ ] Eventos del GUI (#324): el lazo en BP, sin duplicados
- [ ] **AOT desde el IDE**: generar el `.mdn`, cargarlo y medir la ganancia
- [ ] `FontLoadDemo`, `GuiImageDemo` — recursos (fuente e imagen) desde el pack

### b3 · ESP32-P4 Waveshare 4.3"
Panel **ST7701 480×800**, elegido por el **ENV** (`display=st7701`, #311), backlight invertido.
- [ ] El panel sale del ENV: cambiar la variable y ver que cambia el panel
- [ ] El backlight enciende (el `bl_invert` viaja con la entrada del catálogo)
- [ ] Mismo GUI que b2, a otra resolución: comprobar que la interfaz no se sale

### c1 · STM32 Nucleo-U575ZI-Q
Sin pantalla. Wire por el VCP del ST-LINK. **AOT ARM**. Página de borrado de **8 KB**.
- [ ] `BlinkStm32` — LED verde PC7
- [ ] AOT: cómputo `native` y ganancia
- [ ] #338 aquí deja el buffer en **12 KB** (no 8): es su sector, no una excepción

### c2 · STM32 Discovery U5G9J
Pantalla **LTDC**. Es la placa gráfica de la familia.
- [ ] `GuiColorDemo` y compañía
- [ ] El buffer de PUT de 64 KB del GUI (el que se arregló en H9)

---

## Novedades de V4 a confirmar (transversal — marcar donde aplique)

Lo que V4 ha cambiado por debajo y sólo se ve de verdad en placa:

- [ ] **Handles** (H1): el modelo de memoria nuevo, en las 7. Un UAF aquí sale como
      basura o reset, no como excepción
- [ ] **Kernel por capas** (H9): estados 0→3, y que un fallo **baje al último estado
      bueno reportando** en vez de morir mudo
- [ ] **littlefs** en las 3 familias (H2)
- [ ] **Packs** (H3) + **packs ejecutables** (#310)
- [ ] **`.mod` v6 sin `.bpi`** (H6.a): la interfaz viaja dentro del módulo
- [ ] **Sobrecarga de funciones** (H5.a) y **eventos** (H5.c) corriendo en placa
- [ ] **GUI por eventos** (#324) en las 3 con pantalla
- [ ] **AOT** — ARM en a1/a2/c1/c2, **RISC-V dinámico** en b2/b3, interpretado en b1
- [ ] **Subida por streaming** (#294) con el trozo nuevo de 8 KB (#338)

---

## Huecos vistos al montar el guión

- **El widget `Chart` (H7, #317) no tiene ningún sample.** Está en `bpstdlib/Gui.bp`
  pero ningún `.bp` de `samples/` lo usa, así que se publicaría sin haberse ejecutado
  nunca desde BP. Escribir un sample para probarlo **no es una mejora del producto**:
  es cerrar un agujero de la batería. Hacerlo antes del grupo gráfico.

---

## Reparto

**Yo:** compilo firmwares (RP2350 y STM32), preparo samples y proyectos de prueba,
analizo lo que falle, arreglo, y llevo este registro al día.
**Tú:** flasheas, cableas lo que haga falta, compilas los ESP32 (`idf.py`), y me pasas
lo que veas — el log persistente es el mejor testigo que tenemos.

---

## Cómo se trabaja: por lotes

Criterio de Eduardo (3-ago, a mitad de la Pico): **no se rehace el paquete por cada
hallazgo**. Con 7 placas, cada reconstrucción obliga a reinstalar y reflashear, y así
la campaña no converge. Además, cambiar el paquete a mitad **invalida lo ya probado**.

Tres cajones, y lo que los separa es qué interrumpe:

| | Qué se hace |
|---|---|
| **Crítico** | Parar y arreglar. Sólo lo que impide seguir probando, o lo que haría inválido lo que venga detrás (ejemplo: faltaba `BpVM.cfg` → no compilaba nada con stdlib) |
| **Error del test en sí** | Corregir y seguir. Sample rancio, expectativa mal escrita, fallo del arnés. **No se reconstruye el paquete**: no es del producto |
| **Todo lo demás** | Al registro y seguir. Se decide al **terminar la fila** |

Al cerrar cada fila: se agrupan los arreglos, **un solo rebuild, un solo reflasheo**, y
se **vuelve a probar lo que falló** — no la fila entera.

## Registro de hallazgos

| # | Placa | Qué pasó | Estado |
|---|---|---|---|
| 1 | — (paquete) | La ayuda enlazaba 10 anexos `.md` que **no iban dentro del ZIP**: F1 abría el índice y de ahí a PHILOSOPHY, RELEASES, OPCODES… todo 404 en la instalación | ✅ 43dd9b2 — y guardián: si un enlace de la ayuda no resuelve dentro del paquete, el ZIP no se monta |
| 2 | — (paquete) | `samples/` se llevaba **27 ficheros sueltos** de mi copia de trabajo (`NB2P0.bp`, `FsMinC30.bp`, `Hola.bp`…) | ✅ 43dd9b2 — van los del repo, que son los revisados |
| 3 | — (paquete) | El ZIP salía con **`\` como separador** (`Compress-Archive` de PS 5.1). Windows lo tolera; quien lo abra en Linux o Mac se encuentra ficheros con barras en el nombre, todos en un montón | ✅ se monta con `jar` + guardián que rechaza cualquier `\` en los nombres |
| 4 | — (compilador) | **`SyncList` abortaba el compilador** con una traza Java. Repro de 6 líneas. Era el cross-check #174b pidiendo el slot de vtable a una clase built-in; `SyncList` es la única cuya base es otra built-in | ✅ 9dd7d10 — el chequeo no corre para built-in. Paridad dual-VM 28/0/0 |
| 5 | — (paquete) | De los 262 samples publicados, **15 no compilaban**: 5 fallan a propósito, 6 se quedaron rancios (5 con `throw "cadena"` que #248 prohibió, incl. **`hello.bp`**; 1 con la API de I2c anterior a `Bus`), 4 son de `library` y necesitan proyecto | ✅ los 5 a `samples/errores/` con su diagnóstico esperado; los 6 rancios arreglados; los 4 de `library`, pendientes de mirar con su proyecto |
| 10 | a1 Pico | 🔴 El `Hello.mod` **embebido en la imagen** era un `.mod` v5, anterior al ensanchado 4→8B: la placa lo rechaza al ejecutarlo. Se quedó fuera cuando en #271 se regeneraron los blobs de la stdlib —Hello no es stdlib, es un sample— y el script generador que citaba su cabecera **no existía**. Estaba rancio en las **tres** familias, y los tres ficheros eran distintos entre sí (12124/12818/16638 B, tres fechas) | ✅ `scripts/regen-hello-blob.sh` los rehace los tres de la misma fuente y a la vez; ahora los tres son MOD6, 4034 B, idénticos. Pico recompilada y resellada. **El gate de ABI (#284) hizo su trabajo**: paró la ejecución y dijo exactamente qué pasaba. ⚠️ S3 y P4 tienen el blob arreglado en fuente pero sus imágenes **están sin recompilar** |
| 7 | a1 Pico | `Reset: watchdog` en el INFO tras un reinicio **pedido** (cambiar particiones). El RESET del wire usa `watchdog_reboot()`, y `watchdog_caused_reboot()` no distingue eso de un watchdog que disparó → el instrumento decía «cuelgue» donde no lo había. Y con el mismo texto para ambos, `WdtTest`/`WdtDemo` no demostraban nada | ✅ `watchdog_enable_caused_reboot()` separa los dos casos: **watchdog** (disparó) · **reinicio pedido** (RESET) · **power-on/run**. Firmware Pico recompilado y resellado. **✅ CONFIRMADO EN PLACA**: `power-on/run` al flashear y `reinicio pedido` al cambiar particiones (el caso que nos engañó). Falta la 3ª rama, el watchdog disparando de verdad → llega con `WdtTest` |
| 12 | a1 Pico | En **cada** RUN: `MDN: RECHAZADO — ABI 1, esta VM habla 2`. El `Bench.mdn` embebido (`pico/embedded_bench_mdn.c`, 26-may) es anterior a que los helpers AOT pasaran a v2 (#302 paso 2). Inofensivo —el gate lo rechaza y se queda el linked-in— pero asusta en cada ejecución. **Tercer artefacto generado embebido que se queda atrás**, tras el Hello .mod en 3 familias | 🟡 EN EL LOTE. **Recomiendo RETIRARLO, no regenerarlo**: es andamio de #159, el micro-bench que servía para decidir si valía la pena el AOT — decisión tomada hace mucho (H4 cerró con 113×). Mismo criterio que #340. Sólo lo lleva la Pico |
| 11 | a1 Pico | Una imagen nueva **no actualiza lo que ya está en el FS**: el preinstalado hace `if (fs_exists(...)) continue`, así que el `/app/Hello.mod` v5 sobrevivió al reflasheo. Coherente —no queremos pisar ficheros del usuario— pero un blob preinstalado que cambia de versión se queda rancio para siempre en placas con FS previo | 🟡 EN EL LOTE. No es peligroso: el gate de ABI lo caza. Lo suyo sería reemplazar cuando el fichero de la imagen es **distinto** del que hay. Rodeo mientras tanto: formatear el FS |
| 8 | — (ejemplos) | `stacktrace` y `EvThrow` morían con código de salida y sin decir nada — y la traza que prometía `stacktrace` sólo la imprime la VM del PC | ✅ los dos atrapan y terminan de forma regular; salida idéntica en las 2 VMs. Criterio de Eduardo: «un ejemplo que muere callado no enseña nada». El hueco de la traza queda abierto y anotado |
| 9 | — (VM-C) | 🟡 **La VM-C no imprime traza NI el mensaje** de una excepción no atrapada; la VM-Java da la pila completa con `fichero:línea`. En placa te quedas con el código de salida y nada más. Hueco de paridad, nunca implementado (el comentario de `exceptions.c:14` promete lo que no hay) | 🔴 ABIERTO. Tres tamaños: (1) sólo el mensaje, barato; (2) + cadena de funciones por nombre; (3) traza completa, necesita el mapa PC→línea que hoy no viaja a la placa. Decisión de Eduardo |
| 6 | a1 Pico | 🔴 **El paquete no compilaba NADA que importara la stdlib.** El ZIP llevaba `bpstdlib/*.mod`, pero el compilador sólo sabe dónde están si encuentra un `BpVM.cfg` con `stdlibDir`, y ese fichero no iba dentro. En el repo funcionaba porque ahí sí existe. Encima, al no resolver `import Core` el compilador **lo omite y sigue**, así que los 4 errores salían en el código del usuario (`ExcCatchTest`) en vez de decir «no encuentro Core» | ✅ el ZIP lleva `BpVM.cfg` con rutas **relativas** (`./bpstdlib`) — `VmConfig` las resuelve contra el directorio del propio `.cfg`, así que la instalación se puede mover. **Guardián**: el montaje extrae el ZIP a un temporal FUERA del repo y compila `ExcCatchTest`; si falla, borra el ZIP. Verificado que sabe fallar. Encontrado por Eduardo («¿algún problema con el módulo Core?») |
