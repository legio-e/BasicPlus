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
      ejecutar la app **desde el pack**, con su llamada cross-module (`App` → `Util`)
      resolviéndose dentro. ⚠️ *Corrección: antes esto pedía «leyendo un recurso de
      dentro». Los packs de V4 llevan MÓDULOS, no recursos — los `resources/` se
      suben aparte a `/app/<ruta>`. Servirlos desde el pack es #362, V5.*
- [ ] E9 `resources/` del proyecto: `leeme.txt` aparece en `/app/…` tras el Run

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

**Referencia de la placa (FS recién formateado, antes de nada):** heap 277 KB +
pilas 92 · RTOS libre 15 KB · pila VM 12 de 16 KB sin usar · FS 100 KB / 1,5 MB.
Ojo: `heap 277 KB` es el **reparto configurado**, no el consumo — no se mueve nunca
y no sirve para vigilar fugas. El instrumento es la línea de **fin de RUN**.

**Tandas cerradas (3-ago, ZIP `bff6e337`):**

- ✅ **Tanda 1 · Humo** (6): `OoSmoke` `ExcCatchTest` `StrOps348` `MathOps348` `PathOps348`
  `stacktrace`. Sin hallazgos.
- ✅ **Tanda 2 · Memoria** (10, el tren en orden). Los diez con
  `fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar;
  plataforma: 0 vivos)` — **confirmación en placa de #357**, y no en un test suelto
  sino en toda la batería, con `MemT5_Gc` (~440 KB de churn) y
  `smp_heap_stress_pico` dentro, y sobre la imagen sellada.
  La plataforma no se movió: `RTOS libre` 15 KB y marca de agua 11 de 16 KB
  **idénticos** a los de después de la tanda de humo. 16 RUN acumulados en la misma
  sesión, ninguno deja nada.
- ✅ **Tanda 3 · Ficheros** (8): `FileTest` `FileOpsTest` `FileBytesTest` `FsPowerCut`
  `FsImportTest` `JsonDemo` `LogTest` `CompressFileTest`. Dos hallazgos, ninguno del
  producto: `FsPowerCut` no avisaba de nada (arreglado, 863608d) y `FileOpsTest`
  chocó con `lastModified`, que **destapó documentación rancia** — el manual seguía
  describiendo el FS plano de antes de H2 (hallazgo 13, ef04d10). El torturador de
  corte de corriente llegó a 19.806 de 20.000 sin corromper el contador; el corte
  físico se prueba aparte.
- ✅ **Tanda 4 · Threads** (5): `AsyncDemo` `ThreadTrasMain` `mutextest`
  `synclisttest` `preempttest`. Sin hallazgos.
- ✅ **Tanda 4b · Concurrencia del LENGUAJE** (4 de 5): `synctest` `modpropsync`
  `paralleltest` `paralleltest_sugar`. **Faltaban en la lista** —lo preguntó
  Eduardo— y son dos piezas que nadie estaba mirando: `sync property` (con la
  transformación del compilador de B2 detrás) en dos de sus tres formas, y el
  bloque `parallel/case/default/endpar`. ⏳ Queda `l2app` (la forma
  **cross-module** del `sync property`), bloqueado por el hallazgo 15: se repite
  al reconstruir el ZIP.
- ✅ **Tanda 5 · Eventos** (6): `EvFull` `EvOrder` `EvNest` `EvThrow` `EvFin`
  `h5cevuse`. Sin hallazgos. Cubre el ciclo completo, el orden de despacho, la
  reentrada (un handler que dispara otro), el handler que atrapa lo suyo, #342
  (el evento del thread que muere no se pierde) y el caso cross-module.
- ✅ **Tanda 6 · Lo nuevo del lenguaje** (8): `TupleFirstClass` `TupCrossTest`
  `DefaultParams` `StaticPropTest` `narrowtypes` `Field8Test` `Wrap8Test`
  `PropLongTest`. Sin hallazgos. Aquí entran las tuplas (que en el ensanchado
  4→8B se quedaron olvidadas y hubo que cazar), los campos y properties de 8
  bytes, y los tipos estrechos: todo lo que toca anchos en placa.
- ✅ **Tanda 8 · Pack ejecutable, EN EL PC** (#310, camino de host): `sampleproject`
  con `out: pack` → se sube **un solo `.pack` de 8 KB**, la app arranca desde dentro
  y la llamada cross-module `App` → `Util` se resuelve ahí mismo. Corrió contra el
  backend **VM Java**, no contra la placa (`handshake bpvm-java 1.0`, socket en
  `localhost`). ⏳ **Falta el mismo proyecto EN LA PICO**, que es donde #310 se
  demuestra, y con él el **AOT ARM** (`aot: {target: arm}` no pinta nada en el PC).
- ✅ **Tanda 7 · Dispositivo** (2): `blink` — GPIO real en GP25 — y `BoardTest`,
  que devuelve `gpioCount=30 · variant=A`: la placa **se identifica sola** y
  coincide con lo que declara la stdlib board-aware (`GPIO_COUNT=30`). Es el
  mecanismo del que depende que a1 y a2 compartan imagen. El resto de dispositivos
  no se ha tocado en V4 y probarlos es lo más caro (decisión de Eduardo al montar
  la lista).

## 🔁 Lote 1 — el rebuild entre la a1 y la a2 (decidido 3-ago)

**El firmware NO se toca.** La imagen de la Pico se queda como está, bit a bit, y es
la misma que va a la Metro. Así los 49 verdes de la a1 siguen valiendo y la a2
arranca con un binario ya barrido. Se rehacen **sólo el jar del IDE y el ZIP**.

1. ⚠️ **Eduardo cierra el BpIde** — el jar no se puede reconstruir con él abierto.
2. Hallazgo 15: borrar los 24 intrusos de `bpstdlib/` (`.mod` sin `.bp` al lado) y
   los `.mod`/`.dbg` de `samples/`, + regla en `.gitignore`.
3. Hallazgo 16: en el camino de placa, `buildProject` cuando hay proyecto y la misma
   rama `packRun` que ya usa el host para elegir `.pack` en vez de `.mod`.
4. `mvn package` del IDE + `montar-zip.sh`. El guardián MOD6 tiene que pasar en
   verde **sin** los intrusos (hoy los caza, que es la prueba de que funciona).
5. Eduardo reinstala desde el ZIP nuevo.

**Re-prueba en la a1: sólo lo que el lote desbloquea** — `l2app` y `sampleproject`
contra la placa. Los otros 47 no se repiten: el criterio de trabajo por lotes dice
re-probar lo que falló, no la fila entera.

Lo del firmware que queda pendiente (mensaje de `lastModified`, retirar `Bench.mdn`,
y el hallazgo 9 cuando Eduardo decida el tamaño) viaja con la reconstrucción del
ESP32, que hace falta de todos modos.

### a2 · Metro RP2350B
Variante B: 48 GPIO, 16 MB flash, **PSRAM 8 MB**, NeoPixel. **Misma imagen que a1.**

**Referencia de la placa (FS recién formateado):** heap **7,5 MB** + pilas 512 KB —
*el heap está en PSRAM* — · RTOS libre 15 KB · pila VM 12 de 16 KB sin usar · FS
100 KB / 9,8 MB · flash 16 MB · 48 GPIO / 8 ADC · `Reset: reinicio pedido`.
**Cuatro de los cinco puntos propios de la a2 quedan cerrados con este solo INFO**:
variante B detectada sola, heap en PSRAM, clamp #292 (la flash que dice es la real)
y PSRAM vista. Y todo **con el mismo `.uf2`** que la a1 — que es la tesis de la
imagen única, demostrada.

**Tandas cerradas (3-ago, ZIP `34ea3b0d`):**

- ✅ **Tanda 1 · Humo** (7): las 6 de la a1 + **`RandomTest`, 8 de 8**. Primera vez
  que los aleatorios se verifican **en placa**: la cintura por plataforma del
  RP2350 cumple el contrato —límite superior excluido incluido, que es donde se
  rompen los ports.

**Tandas cerradas sobre la imagen del lote 2 (3-ago, `.uf2` `7b4fde29`):**

- 🏁 **Tanda 2 · Memoria** (10, repetida ENTERA a propósito porque el firmware es
  otro). Verde. Y con ella **#369 queda verificado en placa**: el nº 10
  (`smp_heap_stress_pico`) daba `exit 11 (use-after-free)` de forma **sistemática**
  en esta placa, y ahora pasa. Era el frame inicial del thread, que se había
  quedado en el layout de refs de 4 bytes; el `this` de `run()` se leía 4 bytes por
  debajo de su propia pila y la generación salía de memoria ajena.
  Vale la pena anotar por qué costó: **no lo arreglaba un reset**, porque lo que
  decidía si fallaba era el estado del heap (si el GC había reciclado ya ese hueco),
  no el arranque. De ahí que pareciera ir y no ir sin tocar nada.

  **Pasada en las DOS configuraciones de memoria, y sin querer.** La primera vuelta
  fue con `PSRAM=0` —la placa se había quedado así del diagnóstico de #369— o sea
  con **277 KB de heap en SRAM**. Al darnos cuenta se activó la PSRAM y se repitió
  entera: **7,5 MB de heap en PSRAM**. Verde las dos veces. No son la misma prueba:
  cambia el camino de código del heap y cambia lo que aprieta —con 277 KB el GC
  trabaja de verdad; con 7,5 MB sobra sitio para tapar una fuga, y por eso ahí el
  instrumento **no** es que el heap no crezca sino el contador de `fin de RUN`
  (`0 bloques sin liberar`, que es lo que salió).

- ⚡ **`FsPowerCut` con CORTE DE CORRIENTE REAL** — lo que en la a1 quedó pendiente
  como «se prueba aparte». Eduardo desenchufó a lo bestia con el contador por
  debajo de 1000 (no había llegado a imprimir el primer hito). Al rearrancar:
  `contador recuperado = 812`. Un valor **coherente**, ni basura ni cero, y el FS
  montó sin quejarse. Después siguió acumulando bien: 812 + 2000 = **2812**, exit 0.

  Es el contrato del journal cumplido de punta a punta: puedes perder la última
  escritura **sin confirmar**, pero nunca te quedas con un estado corrupto. Hasta
  ahora sólo lo teníamos con el torturador simulado (19.806/20.000 en la a1), que
  demuestra que no se rompe escribiendo mucho, **no** que sobreviva a un corte.
  Son dos cosas distintas y ahora están las dos.

- ✅ **Tanda 3 · Ficheros** (8), repetida entera. Verde, sin hallazgos nuevos. Los
  dos de la a1 (`FsPowerCut` mudo y el `lastModified` de `FileOpsTest`) ya venían
  arreglados y no reaparecen. Dentro va el corte de corriente real de arriba.

- ✅ **Tanda 4 · Threads** (6 — uno nuevo). Verde. El nuevo es **`ThreadFieldTest`**,
  la red de regresión de #369, y esta es la **primera vez que corre en placa**:
  `huecos reciclados: OK` · `cada thread escribe en SU campo entero: OK` ·
  `campo de referencia escrito en run(): OK` · `=== ThreadFieldTest: 3 de 3 ===`.
  Las tres líneas, con el heap en PSRAM (20.000 objetos de churn ahí dentro).
  O sea que el guardián **no es sólo verde en el PC: sirve como instrumento en la
  placa**, que es donde hará falta si esto se vuelve a mover.

- ✅ **Tanda 4b · Concurrencia del lenguaje** (5, ahora sí las cinco). Verde.
  Con ella se cierra el cabo suelto de la a1: **`l2app` — el `sync property`
  cross-module— se ejecuta por fin EN PLACA**. Allí quedó bloqueado por el
  hallazgo 15 (el `.mod` rancio que ganaba la resolución stdlib-first), no por el
  lenguaje; el lote 1 lo desbloqueó y aquí lo confirma el hardware.

- ✅ **Tanda 5 · Eventos** (6). Verde, sin hallazgos. Interesaba especialmente
  `EvFin` (#342 — el evento del thread que muere no se pierde) porque toca justo
  la zona removida hoy: threads que terminan. Sale limpio.

- ✅ **Tanda 7 · Dispositivo** y **Tanda 8 · Excepciones y builtins**. Verdes.
  (`NeoTest` **no puntúa** aquí — hallazgo 21: la fachada contesta éxito sin driver.)
- ✅ **`BlinkStm32`** (LED verde PC7). Verde.
- ✅✅ **AOT ARM en placa — y la medida es HONESTA por dos horas de margen.**
  `fib(28)`: **interpretado 7.755 ms → AOT 78 ms = 99×**, mismo resultado
  (`317811`) por los dos caminos. `Bench.mdn` = **1 thunk, 68 B de nativo**, y
  el `.mdn` que sube son 124 B.

  **Y aquí se cobra el hallazgo 33 por segunda vez.** Con el firmware a `-O0`
  el intérprete habría tardado ~3,65× más (≈28.300 ms) mientras que el AOT
  habría dado los mismos ~78 ms —el `.mdn` **siempre** se compiló a `-Os`—, o
  sea que esta tanda habría anunciado **≈360×**. Un número precioso y **falso**:
  no por medir mal el nativo, sino por comparar contra un intérprete lastrado.
  Se salvó por horas, porque la tanda de AOT estaba pendiente.
- ✅ **Tanda 6 · Lo nuevo del lenguaje** (8). Verde. Esta tanda pesaba más hoy que
  esta mañana: es **la que prueba anchos en placa** (tuplas, campos y properties de
  8 bytes, tipos estrechos), y #369 resultó ser **otro** sitio que el ensanchado
  4→8B se dejó atrás — el segundo, después de las tuplas. Si quedara algún resto
  vivo de esa familia, aquí es donde tenía más probabilidades de asomar. No asomó.

- ✅ **Tanda 7 · Dispositivo** (2). `NeoDemo` —WS2812 por PIO, que en la Metro
  sustituye a `blink` porque aquí el GP25 es el NeoPixel— y `BoardTest`:
  `gpioCount=48 · variant=B · GPIO_COUNT=48`. En la a1, **con el mismo binario**,
  decía `30 · A`. Con esto la **imagen única** queda demostrada por los dos lados:
  no es que el `.uf2` funcione en las dos placas, es que **se distingue solo** en
  runtime y la stdlib board-aware coincide con lo que dice el silicio.

- ✅ **Tanda 8 · Pack ejecutable EN LA PLACA** (#310). El pack arranca desde dentro
  y la llamada cross-module `App` → `Util` se resuelve ahí mismo: `hello,
  BasicPlus!` / `5 squared = 25`. Con esto **#310 queda demostrado en device**, que
  es lo que faltaba de la a1 (allí sólo corrió contra la VM Java del PC).

  🐛 **Hallazgo 18, visto por Eduardo en este mismo log:** el pack ejecuta bien,
  pero el IDE **sube además su contenido suelto** — crea `/app/sampleproject`, sube
  `Util.mod` y sube `leeme.txt`, y las tres cosas ya van DENTRO del `.pack`
  (verificado: `PackStep` mete outDir + `resources/`, y `strings App.pack` enseña
  los símbolos de `Util` y el texto completo del léeme). `packRun` se calcula y se
  usa para elegir la entrada, pero el bloque de subida lo ignora. Es el hermano del
  hallazgo 16 —aquél arregló *construir* el pack y dejó sin arreglar *subirlo*— y
  afecta también a la ruta de host. Importa más que el desperdicio: esas copias
  quedan en `/app`, que **está en el camino de búsqueda**, o sea la forma exacta
  del hallazgo 15. ✅ **ARREGLADO Y REVERIFICADO EN PLACA** (4-ago, 16c7970): la
  condición se lee ahora de la extensión del artefacto, no de la config. La tanda 8
  repetida enseña `[pack] App.pack viaja cerrado: sus módulos y resources van
  dentro, no se suben por separado`, sube **un solo fichero**, y en el árbol del
  dispositivo `/app/sampleproject/` queda **sólo `App.pack`** — la limpieza de
  huérfanos (F2) se llevó sola el `Util.mod` y el `leeme.txt` de la ejecución
  anterior. La app sigue dando `hello, BasicPlus!` / `5 squared = 25`, exit 0.

  Alcance acotado: de las cuatro rutas de subida, la del device era **la única**
  rota. La de host ya era correcta desde #310; las de debug-en-placa y micro
  simulado ejecutan `<base>.mod` explícitamente, nunca un pack, así que sus deps y
  recursos sí hacen falta.

- ⚡ **AOT ARM demostrado EN PLACA** (4-ago, `Bench`). Era el hueco que la tanda 8
  NO cubría: `sampleproject` no tiene ninguna `native function`, así que su
  `aot: {target: arm}` no ejercía nada. `Bench` sí la tiene (`native function fib`),
  y el ciclo completo se ve de punta a punta — compilar → `[aot] Bench.mdn ✓
  (1 thunk, 68 B nativo)` → subir → cargar el thunk → ejecutar código máquina:

  | placa | interpretado | AOT | ratio |
  |---|---|---|---|
  | **a2** Metro (heap en PSRAM) | 8797 ms | 84 ms | 104,7× |
  | **a1** Pico (heap en SRAM) | 8486 ms | 84 ms | 101,0× |

  **~100×, y el mismo número (317811) en las dos.** Coherente con el 113× medido en
  H4 sobre RISC-V. Que el resultado coincida importa tanto como la velocidad: el
  nativo corre *bien*.

  **Y de regalo, el peaje de la PSRAM — con su letra pequeña.** La misma prueba en
  las dos placas da 311 ms de diferencia: **3,5 %**, mucho menos de lo esperado. El
  motivo es que la PSRAM del RP2350 va por la **caché XIP** y `fib` recursivo tiene
  un conjunto de trabajo minúsculo, así que cabe en caché y la latencia queda
  absorbida. Lo medido es «la PSRAM cuesta 3,5 % **cuando el programa cabe en
  caché**», NO en general: un churn de GC sobre los 7,5 MB fallaría en caché sin
  parar y ahí el peaje sería otro — ese número no lo tenemos.
  El AOT sale **84 ms clavados en ambas** (mismo silicio, mismo reloj, código nativo
  en SRAM en los dos casos): buena señal de que no estamos midiendo ruido.

  Dos cosas que salieron de aquí y no son fallos del producto: (a) el programa está
  **mudo 9 segundos** —el primer `print` va después del `fib` interpretado— y eso
  parece un cuelgue; es del sample. (b) el `MDN: RECHAZADO — ABI 1, esta VM habla 2`
  del arranque es **el gate funcionando** (#284) contra el blob embebido de mayo:
  argumento extra para el hallazgo 12, que ya pedía quitarlo — no sólo cuesta
  105 ms de cada boot, es que además imprime lo que parece un error en todos.

**Con la tanda 8 la a2 queda barrida entera, y con ella la familia RP2350 completa
(a1 + a2).** Ocho tandas, cero hallazgos del producto desde el arreglo de #369; el
único hallazgo es del IDE y no afecta a lo que ejecuta la placa.

> ⚠️ **Antes de la 1ª tanda: formatear el FS de la Metro** (cambiando el tamaño de
> partición, como en la Pico). Si la placa trae un FS de una imagen anterior, el
> preinstalado **no lo pisa** (hallazgo 11) y arrastrarías un `/app/Hello.mod` v5 que
> el gate de ABI rechazará — el mismo susto que costó media mañana en la a1.
- [ ] La variante se detecta sola (48 GPIO, 8 ADC en `INFO`)
- [ ] **El heap va a PSRAM** y se ve en `INFO`
- [ ] Clamp #292: la flash que dice `INFO` es la real
- [ ] `NeoDemo` — WS2812 por PIO
- [ ] Con PSRAM, subir un fichero **grande de verdad** (100 KB+)

### b1 · ESP32-S3 DevKit
Wire por UART0; consola por USB nativo. Xtensa: `native` **interpretada**, sin AOT.
- [x] ❌ **#331 — hipótesis REFUTADA (4-ago), y el riesgo de publicación con ella.**
      El anillo del log **sobrevive al reflasheo** (la imagen fusionada acaba mucho
      antes de `bpdata @0x118000`), así que conserva arranques del firmware ANTERIOR.
      Se distinguen por el `buffer de bulk`, que es constante de compilación (#334):
      los de **20480 B** son de antes de #338 (2-ago) — el firmware que llevaba la
      placa hasta hoy —, los de **8192 B** son los de hoy. **Y los cuatro dicen
      `flash: configurada 16384 KB | chip fisico 16384 KB`.**
      O sea: el bootloader viejo **NO** estaba a 2 MB. La causa de #328 sigue sin
      conocerse, pero el escenario que daba miedo —«le pasará a cualquiera con un S3
      así»— **no existe**: ni la imagen vieja ni la nueva configuran mal la flash.
      Demostrado además en la misma sesión que **repartir + formatear funciona**
      (región FS 7632 → 10000 KB, 0 ficheros), que es la operación que murió en #328.
      *Nota de método: di por perdida la evidencia al ver el binario viejo
      sobrescrito. Estaba en el log post-mortem. El anillo en flash pagó otra vez.*
- [ ] DRAM: el bloque de 160 KB (#336) sigue dando de sí con carga

**Referencia de la placa (4-ago, imagen nueva `35b70432`):** `esp32s3` · 240 MHz ·
45 GPIO · 8 PWM / 20 ADC · flash 16 MB · SRAM 512 KB · **sin PSRAM** · VM heap
**96 KB** + stack 64 KB · FS 9,8 MB (repartido hoy de 7,6 → 10000 KB).
Ojo al leer resultados aquí: el heap es **la tercera parte** del de la Pico y
**1/78** del de la Metro, porque de los 512 KB del chip sólo llegan ~312 KB libres
y el IDF se come otros ~84 KB EN MARCHA (razonamiento medido en `esp32/main/main.c`).
Un rojo por memoria en esta placa es información sobre **la plataforma**, no sobre
el producto.

**Tandas (4-ago, ZIP `14ebff58` descomprimido — se prueba el paquete, no el repo):**

- ✅ **Tanda 1 · Humo** (7). Verde.
- ✅ **Tanda 2 · Memoria** (10). Verde con 96 KB de heap. `smp_heap_stress_pico` da
  `20100/20100`: es la **tercera arquitectura** que confirma el arreglo de #369
  —está en código común (`threading.c`), así que Cortex-M33 y Xtensa lo prueban por
  caminos distintos—. Y el aviso del AOT sale como debe: `AOT target 'xtensa' no
  soportado (arm = Cortex-M33 RP2350/STM32 · riscv = ESP32-P4). Los módulos se
  ejecutarán interpretados` — dice qué SÍ hay y qué va a hacer, no sólo que no puede.
- ✅ **Tanda 6 · Lo nuevo del lenguaje** (8). Verde. **Tercera arquitectura** para
  todo lo que ensanchó V4 —tuplas locales y cross-module, `long`/`double`,
  enteros estrechos, campos y envoltorios de 8 bytes, parámetros por defecto y
  static property—, y la única de las tres que **no** es la que se usó para
  desarrollarlo. Sin hallazgos: el 4→8B aguanta también en Cortex-M33.
- ✅ **Tanda 5 · Eventos** (6). Verde. Impresión de Eduardo, **sin cronometrar**:
  los eventos también van más rápido. Tiene mecanismo —el `-Os` alcanzó al
  despacho de eventos, que es núcleo portable: la cola y la inyección del frame
  entre quanta— así que encaja, pero **sigue siendo impresión**.

  ⚠️ **Aviso para la comparación que quiere hacer con la c2**: *«con la Discovery
  se verá más claro si los eventos van mejor que en la P4»* — y esa comparación
  **va a estar contaminada por los PÍXELES**. En cuanto entra la pantalla, el
  coste de LVGL escala con el **área**, y el panel del P4 es de **1024×600**
  (hallazgo 22). Si la Discovery parece más ágil, una parte será que tiene menos
  píxeles que empujar, no que despache eventos mejor.

  **La comparación limpia es la que acaba de hacer**: los 6 samples de esta
  tanda **no tocan pantalla**, así que c1 contra b1/b2 mide el mecanismo de
  eventos y nada más. Lo que falta para convertirlo en dato es que **algún
  sample de eventos imprima un tiempo**, que hoy ninguno hace → anotado para V5,
  no se toca en freeze.
- ✅ **Tanda 4 · Threads y concurrencia** (11). Completa y verde **salvo el
  hallazgo 34** (`paralleltest_sugar`), que resultó ser el 24 y se curó con un
  reset. `paralleltest` dio **56.781 ms**, el 3,65× del hallazgo 33.
- ✅ **Tanda 3 · Ficheros** (8). Verde. No es una repetición de la Pico: aquí la
  cintura es `fs_lfs_esp32.c` sobre `esp_partition`, no `flash_range_*`. Mismo
  littlefs y misma fachada, **trozo de abajo distinto** — o sea que la abstracción
  de H2 queda probada por su segundo camino.
- ✅ **Tanda 4 + 4b · Threads y concurrencia** (11). Verde. `paralleltest` pareció
  colgarse y **no era cuelgue: son 129 segundos** (4M de iteraciones interpretadas)
  en los que el sample no decía nada. Segunda vez en el mismo día que un programa
  mudo se confunde con uno colgado (la otra fue `Bench`, 9 s), así que **se le han
  puesto `print` de progreso** — decisión de Eduardo: *"así el usuario verá lo que
  está haciendo en vez de pensar que se ha colgado"*.

  **Y lo que enseña el patrón importa más que el tiempo.** Los dos workers avanzan
  ACOMPASADOS (`w1 @100000` / `w2 @100000` / …) sin un solo `yield` en el programa.
  Ojo con la lectura: **ninguna placa ejecuta dos workers de intérprete** — en el
  RP2350 el segundo core lleva la comunicación, no ejecuta BP. O sea que ese
  reparto lo hace **el planificador de la propia BP-VM** entre quanta, no el
  sistema operativo. Es H2 funcionando, y aquí sobre Xtensa.

  ⏳ **Abierto (rendimiento, no corrección):** 129.262 ms en el S3 contra 215 ms en
  el host son **601×**, cuando el `fib(28)` de `Bench` da 50× entre host y Pico.
  Pero la comparación es coja —cargas distintas— y **falta el control bueno**:
  `paralleltest` contra `paralleltest` en RP2350. Si el S3 resultara varias veces
  más lento con la misma carga, la sospecha clásica en ESP32 es el intérprete
  corriendo desde flash por caché en vez de IRAM. → **V5**, no se toca en freeze.

  🔎 **Al día 5-ago: la duda sigue abierta pero MUCHO más estrecha, y ahora el S3
  está solo.** El hallazgo 33 destapó que el STM32 se compilaba a `-O0`; con eso
  arreglado hay **tres familias con build optimizado** y los ciclos por iteración
  quedan **2.271** (STM32 `-Os`) · **2.976** (P4 `-Os`) · **7.756** (S3 `-Os`). Los
  dos primeros se parecen; el S3 va **3,4× peor que el mejor**, y **no es el build**,
  porque es el mismo `-Os` que el P4. O sea que lo que quedaba de sospecha —dónde
  ejecuta el intérprete— **sobrevive y ya no tiene alternativas cómodas**.
- ✅ **Tanda 5 · Eventos** (6). Verde, sin hallazgos — igual que en la Metro.
- ✅ **Tanda 6 · Lo nuevo del lenguaje** (8). Verde. Tuplas, `long`/`double`,
  enteros estrechos, sobrecarga: todo lo que ensanchó V4 pasa también en la
  tercera arquitectura, y con **96 KB de heap**, que es el peor caso del parque.
- ⚠️ **Tanda 7 · Dispositivo** (2). `BoardTest` verde, con **el hallazgo 19**
  dentro: `variant=B` en un ESP32-S3 (ver el registro). **`NeoTest` NO cuenta como
  verde**: en el ESP32 no hay backend de WS2812 —sólo lo registra la Pico— y la
  fachada devuelve éxito sin hacer nada, así que el sample pasa **sin encender
  nada**. Es el **hallazgo 21**. *Lo di por bueno al anotar la fila; lo corrijo
  aquí.*
- ✅ **Tanda 8 · Pack ejecutable EN LA PLACA** (#310). Verde a la primera, y con
  ella **la fila b1 cerrada**. Vale doble: es la **segunda familia** que ejecuta
  un pack desde `Run on Device` —o sea que los hallazgos **16** (construirlo) y
  **18** (subirlo cerrado) quedan confirmados en un micro que no es RP2350—, y
  es el camino de pack sobre `fs_lfs_esp32.c`, distinto del de la Metro.

**🏁 b1 CERRADA (4-ago): 8 tandas, 8 verdes.** Un solo hallazgo (el 19,
informativo, a V5) y **cero rojos**. Con ella son **dos familias completas** —
RP2350 (a1+a2) y Xtensa (b1)— y las tres arquitecturas donde el arreglo de #369
está probado por caminos distintos.

### b2 · ESP32-P4 Kit
PSRAM 32 MB, MIPI-DSI **EK79007 1024×600**, táctil **GT911 (I2C 0x14)**, Ethernet IP101.
**AOT RISC-V con `.mdn` dinámico** (#H4, 113×).

**Referencia de la placa (4-ago, imagen `8e1c3535`):** `esp32p4` · **360 MHz** ·
55 GPIO · 14 PWM / 14 ADC · flash 16 MB · SRAM 768 KB · **PSRAM 32 MB** · VM heap
**27,5 MB** + stack 512 KB · FS 100 KB / 6,8 MB (recién formateado — los 100 KB
son la línea base de un FS vacío, el mismo número que salió en la Pico).

**La otra punta del parque.** El heap es **286×** el del S3 (96 KB) y **110×** el
de la Pico (257 KB). Aquí un rojo por memoria no es información sobre la
plataforma: si algo se queda sin memoria con 27 MB, es un bug. Y es la **única
placa con AOT en marcha**, así que es la tanda que ejercita lo que en el S3 no se
pudo probar (Xtensa no tiene AOT).

**La placa venía sin particiones y el IDE lo dijo** antes de intentar nada. Es
H9 (#303) haciendo su trabajo: el arranque por capas se para en el estado que
toca y lo cuenta, en vez de fallar mudo más adelante.

⚠️ `BoardTest` dirá `variant=B` también aquí (55 GPIO ≥ 40). **Es el hallazgo 19,
ya registrado** — no hace falta volver a anotarlo.

**Tandas (4-ago):**

- ✅ **Tanda 1 · Humo** (7). Verde.
- ✅ **Tanda 2 · Memoria** (10). Verde con **27,5 MB de heap**. Aquí el criterio se
  da la vuelta respecto al S3: con este heap, que los samples de estrés pasen no
  demuestra gran cosa — lo que cuenta es que **la memoria vuelva a su sitio al
  acabar el RUN**, que es lo que mide el guardián de #339.
- ✅ **Tanda 3 · Ficheros** (8). Verde, y con **una medida de Eduardo que merece
  quedar escrita**: el FS va **igual de rápido que en las demás placas**. *«Aquí
  la CPU no aporta diferencia, el FS es el más lento y es el que manda.»*

  No es una anécdota, es la confirmación de que **el FS está limitado por la
  flash, no por el micro**: un borrado de bloque son milisegundos, y a esa escala
  da igual ir a 150 MHz o a 360. Dos consecuencias prácticas: (a) el rendimiento
  del FS es una propiedad **de la placa, no de la familia**, así que no hay que
  perfilarlo por micro; y (b) el mismo motor (littlefs + fachada) da el mismo
  número por dos cinturas distintas y en tres ISA — que es exactamente lo que se
  buscaba al diseñarlo con la cintura desde el día uno.
- ✅ **Tanda 4 · Threads y concurrencia** (11). Verde, y **notablemente más
  rápida**. `paralleltest` (4M de iteraciones interpretadas, dos threads):

  | | reloj | tiempo | por reloj |
  |---|---|---|---|
  | ESP32-S3 (Xtensa) | 240 MHz | 129.262 ms | — |
  | **ESP32-P4 (RISC-V)** | **360 MHz** | **33.079 ms** | **2,6× mejor** |

  **3,9× más rápido con 1,5× de reloj**, o sea **2,6× por ciclo**. Es el control
  que faltaba en la duda de rendimiento que dejó abierta la b1: la lentitud del
  S3 **no es «lo que cuesta ser un micro»** — es del S3. Misma carga, mismo
  binario del ZIP, y en los dos casos **interpretado** (`paralleltest` no tiene
  funciones `native`, así que el AOT no entra en esta medida: es intérprete
  contra intérprete en dos ISA).

  Los dos threads BP dan `sum=656067456` idéntico, y el planificador de la BP-VM
  reparte igual de acompasado que en las otras placas — primera vez que ese
  planificador corre sobre RISC-V.

  ⚠️ **Y conviene dejar escrito qué NO mide esto, porque induce a error**: un
  **thread de BP no es una task del RTOS ni un núcleo**. Los threads de BP son
  verdes: los reparte **el planificador de la propia VM entre quanta**, y en
  placa la VM corre en **una sola task** en las tres familias — verificado en el
  código: `esp32/main/repl_esp32.c:807` y `stm32/port/stm32_repl.c:561` llaman a
  `bpvm_run(vm)` a secas (el comentario del ESP32 lo dice: *«el SMP en ESP32 es
  H4.2+»*), y el Pico sólo entra por `bpvm_run_smp` si se define
  `BPVM_PICO_SMP_WORKERS` al configurar, que **no está definido** en la imagen
  que se publica. O sea que **el segundo núcleo del P4 no ejecuta BP**, y esta
  medida es intérprete contra intérprete **en un solo núcleo a cada lado**: la
  comparación entre placas es de **reloj y arquitectura, nada más**. (Corrección
  de Eduardo, 5-ago: *«los BPThreads no son los RTOS_Threads»*.)

  Sigue sin cerrarse la pregunta de **por qué** el S3 va lento por ciclo (la
  sospecha del intérprete desde flash por caché en vez de IRAM), pero ya no es
  una anomalía sin referencia: hay dos puntos y el reparto entre ellos es
  arquitectura, no reloj. → **V5**.
- ✅ **Tanda 5 · Eventos** (6). Verde. Tercera arquitectura para H5.c, y **la que
  más importa de las tres**: la escalera de GUI que viene detrás se apoya entera
  en este mecanismo —el lazo de #324 son eventos— así que verlos verdes *antes*
  de tocar la pantalla separa dos causas que si no vendrían juntas.
- ✅ **Tanda 6 · Lo nuevo del lenguaje** (8). Verde. Tercera arquitectura, sin
  sorpresas.
- ✅ **Tanda 7 · Dispositivo**. `BoardTest` verde (con el `variant=B` del hallazgo
  19, esperado). **`NeoTest` no puntúa aquí tampoco**: el P4 tampoco registra
  backend de WS2812 — hallazgo 21.
- ✅ **Tanda 8 · Pack ejecutable EN LA PLACA** (#310). Verde. **Tercera familia**
  con el pack ejecutándose desde `Run on Device`: con RP2350 y Xtensa detrás, los
  hallazgos 16 y 18 quedan confirmados en las tres, y #310 con ellos.

**Batería estándar del P4: 8/8.** Lo que queda de la fila b2 es lo propio de esta
placa —la escalera de GUI de 16 peldaños y el AOT—, o sea que es la placa a la
que **más le queda por delante**, no la que va más adelantada.
#### Tanda GUI del P4 — orden de dependencia (4-ago)

**En escalera: cada peldaño supone el anterior.** Si uno falla, no sigas hacia
abajo — el siguiente hereda el fallo y no dice nada nuevo.

| # | Qué se corre | Qué demuestra | Ojo con |
|---|---|---|---|
| 1 | `GuiDemo` | Que pinta algo. El primer programa GUI de BP (V3) | Si esto falla, salta a la escalera de bisección de abajo |
| 2 | `GuiColorDemo` | Color de fondo y de texto | **Es el del cian del P4** (#285). Quedó cerrado: era el compilador rancio que empaquetaba el IDE, no el render. Aquí se confirma con el IDE nuevo |
| 3 | `GuiGeomDemo` | Geometría explícita (x/y/ancho/alto) | El P4 es 1024×600: es la resolución más grande del parque |
| 4 | `GuiClickDemo` | **Táctil** + upcall de eventos | GT911 en I2C **0x14** (no la dirección de catálogo) |
| 5 | `GuiCheckDemo` | Widget con `onChange` | Primer widget con evento propio |
| 6 | `GuiEvSpike` | Que el lazo de eventos **no duplica** (#324) | No es demo, es una MEDIDA. Lo que importa es el conteo |
| 7 | `GuiValueDemo`, `GuiLedSpin` | switch / slider / bar / spinbox / led | — |
| 8 | `GuiInputDemo`, `GuiListKbd` | dropdown, textarea, list, keyboard | Entrada de texto = el táctil trabajando de verdad |
| 9 | `GuiTabDemo`, `GuiTableDemo`, `GuiMsgDemo` | tabview, table, msgbox modal | — |
| 10 | `GuiFontDemo`, `FontLoadDemo` | Tamaños de fuente y carga de una `.bin` de LVGL | `FontLoadDemo` lee del FS: cruza GUI × ficheros |
| 11 | **`samples/imageproject`** (proyecto) | Imagen (asset + control) | **NO el `GuiImageDemo` suelto**: ése no puede subir el `.png` a la placa y encima no viaja en el ZIP → hallazgo 25. El proyecto sí, por su `resources/` |
| 12 | `GuiRotDemo` | `Gui.setRotation` en runtime | En MIPI-DSI puede no comportarse como en SPI |
| 13 | `GuiAsyncDemo` | Trabajo largo desde un handler **sin congelar la GUI** | El cruce eventos × threads. Aquí es donde se nota si el planificador y el lazo de GUI se estorban |
| 14 | `GuiGcRoot` / `GuiGcRootQ` | Que el objeto BP colgado de un widget **no lo barre el GC** | Guardián de #302 paso 1. Cadena de dos eslabones: el widget lo sostiene el `objptr` y el widget sostiene al oyente por `recv` |
| 15 | Proyecto `formdemo` | **Forms desde `.win`** (`resources/main.win`) | Es lo que colgaba en el P4 y se arregló (super() implícito + widget-sin-contenedor + FS→PSRAM) |
| 16 | **AOT desde el IDE** | Generar el `.mdn`, cargarlo, medir | El plato fuerte de esta placa. Se compara contra el **propio P4 interpretado**, no contra otra placa |

**Resultado (4-ago) — peldaños 1 a 7, todos verdes, sin sacar la escalera de
bisección:**

| # | Resultado |
|---|---|
| 1 `GuiDemo` | ✅ Pinta |
| 2 `GuiColorDemo` | ✅ **Cierra #285 por medida**: el cian era el compilador rancio que empaquetaba el IDE, no el render. Ahora, con el IDE nuevo y en la placa donde apareció, sale bien |
| 3 `GuiGeomDemo` | ✅ |
| 4 `GuiClickDemo` | ✅ **Táctil**, GT911 en I2C 0x14. De aquí sale el **hallazgo 22** (el `screen` dice 480×320 sobre un panel de 1024×600) |
| 5 `GuiCheckDemo` | ✅ Widget con `onChange` |
| 6 `GuiEvSpike` | ✅ **#324 confirmado en RISC-V**, y la lectura tuvo su momento: con el log a medias salían **5 upcalls y 2 handlers** y canté que no cuadraba. Con el log completo son **7 y 7**. Los eventos iban con retraso, que es justo lo que dice el diseño —`raise` **encola** y se drena entre quanta, no es una llamada síncrona—, así que en placa salen a rachas. **El invariante no es la alternancia, es el total.** El control de host, corrido en la VM-C, da la referencia limpia: 1 clic → 1 upcall → 1 handler, y el `3 handler` **antes** del `4 tras run()`, que es el criterio que el propio sample define para el caso bueno |
| 7 `GuiValueDemo`, `GuiLedSpin` | ✅ switch/slider/bar/spinbox/led, con valores coherentes en el volcado |
| 8 `GuiInputDemo`, `GuiListKbd` | ✅ Teclado en pantalla escribiendo en el textarea. Visto por Eduardo: **la tecla ✓ no hace nada** → **hallazgo 23** |
| 9 `GuiTabDemo`, `GuiTableDemo`, `GuiMsgDemo` | ✅ tabview, table y el modal devolviendo su índice de botón (`val=0` en el volcado) |
| 11 `samples/imageproject` | ✅ Imagen en placa **y de paso el ciclo completo de `resources/`** (#260): el IDE sube el `.png` al `/app` del device y el firmware lo encuentra. Cubre más que el sample suelto, que ni siquiera podía → **hallazgo 25** |
| 12 `GuiRotDemo` | ✅ `setRotation` en caliente sobre MIPI-DSI, que no es rotar por software un SPI. Eduardo: *«va un pelín lento pero creo que son los eventos»* → **tarea #373 a V5**, y ahí lo primero es **medir**, no optimizar: hay tres candidatos y el suyo es uno, no necesariamente el mayor — (a) el lazo de bombeo está **en BP** desde #324, o sea interpretado en cada frame; (b) la inyección de un frame BP por evento; (c) el repintado de **1024×600, 4× los píxeles del modelo lógico de 480×320** (hallazgo 22) y sin acelerador 2D. La (c) no tiene nada que ver con los eventos y puede ser la gorda |
| 13 `GuiAsyncDemo` | ✅ **La arquitectura entera a la vez**: el lazo de GUI sigue bombeando y refrescando el progreso mientras un worker hace trabajo largo. Son H2 (threads) + H5.c (eventos) + #325 (`Thread(obj::metodo(args))`) trabajando juntos, en placa y sobre RISC-V. Dato para el #373: si la GUI va fluida **compitiendo** con un worker por el mismo intérprete, el planificador no es el cuello |
| 14 `GuiGcRoot`, `GuiGcRootQ` | ✅ **Y el verde es FUERTE, no débil.** Avisé de lo contrario —«con 27 MB el GC pasa poco»— y me equivoqué: los dos samples llaman a **`gc()` explícitamente** y comprueban el clic *después*, así que **no dependen de que el heap se llene** y el tamaño aquí da igual. Lo que aguanta una recogida real es la cadena de dos eslabones: el `objptr` sostiene el widget y el widget sostiene al oyente por el campo `recv` del evento. Si a ese campo le faltara el bit de referencia en el layout, el oyente moriría vivo y el clic sería un use-after-free. Guardián de #302 paso 1 |
| 15 `samples/formdemo` | ✅ **Reverificación de verdad**: Forms desde `.win` es **lo que colgaba en esta placa**, y se arregló con tres cosas a la vez (el `super()` implícito del modelo Java, el widget-sin-contenedor y el FS en PSRAM). El log enseña el ciclo completo: `subido resource /app/formdemo/main.win (390 bytes)` y luego los handlers disparando (`onSaludar`, `checkbox cambiado`). Segundo paso por `resources/`, esta vez con un `.win` en vez de un `.png` |
| 16 **AOT** (`Bench`) | ✅✅ **116×, y es lo único de toda la campaña que no se puede probar en ninguna otra placa.** `fib(28)`: **4.543 ms interpretado → 39 ms en AOT**, mismo micro, mismo reloj, misma ejecución. Los tres controles pasan: (a) **compiló de verdad** —`AOT: compilando funciones native (target riscv — lo dice la placa)`, `[aot] Bench.mdn ✓ (1 thunk, 130 B nativo)`, `subido AOT /app/Bench.mdn (186 bytes)`—, no el `sin funciones native que compilar` de los samples de GUI; (b) **ningún RECHAZADO** de ABI, que es lo que sí pasa con el `Bench.mdn` embebido y rancio de la Pico (hallazgo 12) — aquí el `.mdn` se genera fresco y habla ABI 2; (c) **y el resultado es el mismo por los dos caminos**, `317811`: no es sólo más rápido, es que **calcula lo mismo**. Por encima de la referencia guardada de la placa (113×). Comparado con esta mañana en ARM (8.486 → 84 ms), el P4 es ~1,9× más rápido interpretando y ~2,2× en nativo |
| 16 **AOT** (`Bench`) | ✅✅ **116×, y es lo único de toda la campaña que no se puede probar en ninguna otra placa.** `fib(28)`: **4.543 ms interpretado → 39 ms en AOT**, mismo micro, mismo reloj, misma ejecución. Los tres controles pasan: (a) **compiló de verdad** —`AOT: compilando funciones native (target riscv — lo dice la placa)`, `[aot] Bench.mdn ✓ (1 thunk, 130 B nativo)`, `subido AOT /app/Bench.mdn (186 bytes)`—, no el `sin funciones native que compilar` de los samples de GUI; (b) **ningún RECHAZADO** de ABI, que es lo que sí pasa con el `Bench.mdn` embebido y rancio de la Pico (hallazgo 12) — aquí el `.mdn` se genera fresco y habla ABI 2; (c) **y el resultado es el mismo por los dos caminos**, `317811`: no es sólo más rápido, es que **calcula lo mismo**. Por encima de la referencia guardada de la placa (113×). Comparado con esta mañana en ARM (8.486 → 84 ms), el P4 es ~1,9× más rápido interpretando y ~2,2× en nativo |
| 10 `GuiFontDemo`, `FontLoadDemo` | ⚠️ `GuiFontDemo` **salió negra a la primera** → **hallazgo 24** (no era del sample: estado acumulado; con la placa reseteada, verde). `FontLoadDemo` ✅: `loadFont(...) -> id 1` leyendo el `.bin` del FS de la placa y **la fuente cargada se pinta**, confirmado por Eduardo a ojo. Éste es el primer peldaño que cruza **GUI × ficheros**, y hay que saber leerlo: el volcado **no muestra** la fuente cargada —sólo refleja `fontSize` en px, y `setFont(id)` va por otro camino—, así que aquí **el instrumento es ciego y sólo decide el ojo** |

**Si algo se rompe, la escalera de bisección ya está escrita** y no hay que
improvisar: `GuiLblMin` (lo mínimo que debería ir) → `GuiWinMin` (sólo construir
la Window) → `GuiWinLbl` (Window + Label hijo) → `GuiWinPanel` (Window + Panel) →
`GuiWinChk` (Window + Checkbox). Es la que cazó el cuelgue de Forms. `GuiRotProbe`
es la sonda del cian, del mismo estilo.

**Hueco honesto, que conviene decir antes de correr nada:** ningún sample junta
**AOT y GUI**. El `.mdn` del P4 es de `Bench`, que no pinta. La combinación
«código nativo compilado que toca objetos BP sostenidos por widgets» es
justamente el terreno del **paso 3 de #302** (shadow stack / raíces GC del native
compilado), que quedó diferido *a AOT-en-placa* — o sea, a esta placa. No es un
rojo esperado: es que **no hay quien lo pruebe**.

**🏁 ESCALERA DE GUI COMPLETA: 16/16.** Con la batería estándar (8/8) delante,
**la fila b2 queda cerrada**. Un rojo real —el **hallazgo 24**, la pantalla que
deja de pintar tras muchos RUN— y cinco hallazgos informativos (19, 20, 22, 23,
25). Es la placa que más cubre del parque: la única con **GUI completa + táctil +
AOT en placa**, y por tanto la única donde V4 se ve entera funcionando a la vez.

### b3 · ESP32-P4 Waveshare 4.3"
Panel **ST7701 480×800**, elegido por el **ENV** (`display=st7701`, #311), backlight invertido.
- [ ] El panel sale del ENV: cambiar la variable y ver que cambia el panel
- [ ] El backlight enciende (el `bl_invert` viaja con la entrada del catálogo)
- [ ] Mismo GUI que b2, a otra resolución: comprobar que la interfaz no se sale

**Referencia de la placa (4-ago, MISMA imagen `8e1c3535` que la b2):** `esp32p4` ·
360 MHz · 55 GPIO · **flash 32 MB** · SRAM 768 KB · PSRAM 32 MB · VM heap 27,5 MB
+ stack 512 KB · **FS 5,0 MB** de una zona de datos de ~10 MB → **hallazgo 27**
(el límite lo pone la tabla del IDF de la imagen, no el chip; se deja así en V4).

**Esta fila NO repite las 8 tandas, y el motivo importa**: es **la misma imagen**
que la b2, ya verde de punta a punta. Lo que aquí prueba algo que no esté probado
ya es lo que **cambia entre las dos placas** — el panel, el táctil y el arranque.
Repetir el resto mediría el mismo binario dos veces.

**Tandas (4-ago):**

- ✅ **Tanda 1 · Humo** + `RandomTest` (8/8). Verde. Su valor aquí no es el ISA
  —eso ya lo cubrió la b2— sino que **el mismo binario arranca y opera en una
  placa distinta**, que es la afirmación que sostiene la imagen única.
- ✅ **El panel sale del ENV** (#311) y **el backlight invertido enciende**.
  `GuiDemo` y `GuiColorDemo` se ven bien con el `ST7701`, lo que ya demuestra que
  el ENV **se leyó**: sin él habría arrancado con el perfil de la EV a 1024×600.
- ✅ **Táctil** (`GuiClickDemo`). Es lo único de esta placa que no comparte
  silicio con lo ya probado en la b2.
- ✅ **Geometría y tabla** (`GuiGeomDemo`, `GuiTableDemo`). El primero dibuja su
  panel de `100x50` en `pos=10,20` — correcto, y **no puede enseñar el hallazgo
  22 porque no usa `align`**: al ser geometría absoluta, el modelo lógico no
  entra en juego. *(Predije que se vería el efecto y me equivoqué de sample.)*
  El segundo ocupa sólo la parte superior, pero **eso tampoco lo demuestra**:
  como observó Eduardo, **la tabla se ajusta a su contenido** (cabecera + una
  fila), y eso se ve pequeño en cualquier pantalla. **El 22 ya está probado sin
  necesidad de la vista**: el volcado imprime `screen [480x320]` en una placa de
  480×**800**. Lo que sí discriminaría, el día que haga falta: un widget con
  **align BOTTOM** — con el modelo real saldría abajo del todo; con el de 320
  sale a media pantalla, y eso no admite otra lectura.

**🏁 b3 CERRADA.** No repite las 8 tandas por diseño (misma imagen que la b2): se
ha probado **lo que cambia entre las dos placas** —arranque, panel por ENV,
backlight, táctil y layout— y todo verde. Lo que sí ha aportado, y vale más que
la fila entera, son **dos hallazgos que sólo se ven en una placa recién
particionada**: el **28** (🔴 crítico, en virgen no arranca ninguna demo gráfica)
y el **27** (el límite de flash lo pone la imagen). **Cinco placas de siete.**

### c1 · STM32 Nucleo-U575ZI-Q
Sin pantalla. Wire por el VCP del ST-LINK. **AOT ARM**. Página de borrado de **8 KB**.
- [ ] `BlinkStm32` — LED verde PC7
- [ ] AOT: cómputo `native` y ganancia
- [ ] #338 aquí deja el buffer en **12 KB** (no 8): es su sector, no una excepción

**Referencia de la placa (5-ago):** `nucleo-u575zi` · 160 MHz · **114 GPIO** (el
INFO; el lenguaje dice 128 → **hallazgo 32**) · 28 PWM / 20 ADC · flash 2 MB ·
SRAM 768 KB · **VM: heap 372 KB medidos** tras subir el bloque a 512 KB
(hallazgo 31; antes 64) · **partición FS 704 KB** (el primer número del panel
es lo USADO —`fsUsedBytes`—, no una propiedad de la placa: sube según lo que
haya subido; 192 KB al empezar la fila, 328 KB tras las tandas 1-3) · página de borrado **8 KB**, la
única de las tres familias que no es de 4 (**hallazgo 29**).

**Tandas (5-ago):**

- ✅ **Tanda 1 · Humo** (7). Verde **dos veces**: una con la imagen de 128 KB de
  bloque y otra tras subirlo a 512 (hallazgo 31). La segunda es la que cuenta.
- ✅ **Tanda 2 · Memoria** (10) **+ `MemInfo`**. Verde, y es la tanda que
  **reverifica el hallazgo 31 en placa**, que era lo único que le faltaba:

  | medida | valor |
  |---|--:|
  | mayor bloque reservable | **244 KB** |
  | total en trozos de 4 KB | **372 KB** |
  | total en trozos de 1 KB | 369 KB |

  Los 372 KB medidos **son** los 384 que predice la regla de reparto menos lo
  que el propio runtime tiene ya en pie: el modelo no se ha estimado, se ha
  comprobado. Y el hueco entre 244 y 372 es **fragmentación**, exactamente lo
  que `MemInfo` existe para separar.

  🔁 **Repetida sobre la imagen `-Os`**: **372 / 372 / 369**. Los totales no se
  mueven ni un KB —el bloque de 512 KB del hallazgo 31 sigue entero, como decía
  el `bss`— pero el **mayor bloque pasó de 244 a 372**, y eso pedía explicación
  porque un flag del compilador no tiene por qué tocar el alocador.

  ✅ **RESUELTO, y no era ni el `-Os` ni el alocador: eran DOS VERSIONES DEL
  SAMPLE.** La medida de la mañana se tomó con el `MemInfo` **anterior al arreglo
  `b9fceb8`** —el que **biseca**, y que precisamente por bisecar medía mal—.
  Comprobado ejecutando esa versión recuperada de git en el host con
  `--mem=786432`: da **244 KB (250.000 B)**, el número exacto de la placa. Y hay
  una segunda huella que no admite discusión: **el orden de las líneas**. La
  versión vieja imprime «Mayor bloque» ANTES que «Total en trozos de 4 KB», y la
  nueva al revés — y los dos logs de Eduardo salen cada uno en su orden.

  ⚠️ **Corrección de una frase mía que estaba mal**: aquí decía *«el mismo
  binario en el host da los mismos tres números: paridad host↔placa byte a
  byte»*. **No era cierto** — con el sample arreglado el host daba 372 donde la
  placa daba 244, y llamé paridad a una comparación que no había hecho número a
  número. La paridad **sí existe**, pero se demuestra hoy: comparando la misma
  versión del sample, host y placa dan **372 / 372 / 369** los dos.

  📌 Y la lección es la de siempre, sólo que esta vez la pagué yo: **antes de
  explicar un número raro, comprobar que los dos lados corren el MISMO código**.
  Estuve a punto de escribir que el `-Os` había desfragmentado el heap.
  Criterio de Eduardo al cerrarlo: *«podría ser un bug o puede ser inofensivo;
  si es un bug los test deberían destaparlo»* — y de hecho ya no hay nada que
  destapar.
- ✅ **Tanda 3 · Ficheros** (8). Verde. No es una repetición de la Pico: aquí la
  cintura es `fs_lfs_esp32.c` sobre `esp_partition`, no `flash_range_*`. Mismo
  littlefs y misma fachada, **trozo de abajo distinto** — o sea que la abstracción
  de H2 queda probada por su segundo camino.
- ✅ **Tanda 4 + 4b · Threads y concurrencia** (11). Verde. `paralleltest` pareció
  colgarse y **no era cuelgue: son 129 segundos** (4M de iteraciones interpretadas)
  en los que el sample no decía nada. Segunda vez en el mismo día que un programa
  mudo se confunde con uno colgado (la otra fue `Bench`, 9 s), así que **se le han
  puesto `print` de progreso** — decisión de Eduardo: *"así el usuario verá lo que
  está haciendo en vez de pensar que se ha colgado"*.

  **Y lo que enseña el patrón importa más que el tiempo.** Los dos workers avanzan
  ACOMPASADOS (`w1 @100000` / `w2 @100000` / …) sin un solo `yield` en el programa.
  Ojo con la lectura: **ninguna placa ejecuta dos workers de intérprete** — en el
  RP2350 el segundo core lleva la comunicación, no ejecuta BP. O sea que ese
  reparto lo hace **el planificador de la propia BP-VM** entre quanta, no el
  sistema operativo. Es H2 funcionando, y aquí sobre Xtensa.

  ⏳ **Abierto (rendimiento, no corrección):** 129.262 ms en el S3 contra 215 ms en
  el host son **601×**, cuando el `fib(28)` de `Bench` da 50× entre host y Pico.
  Pero la comparación es coja —cargas distintas— y **falta el control bueno**:
  `paralleltest` contra `paralleltest` en RP2350. Si el S3 resultara varias veces
  más lento con la misma carga, la sospecha clásica en ESP32 es el intérprete
  corriendo desde flash por caché en vez de IRAM. → **V5**, no se toca en freeze.

  🔎 **Al día 5-ago: la duda sigue abierta pero MUCHO más estrecha, y ahora el S3
  está solo.** El hallazgo 33 destapó que el STM32 se compilaba a `-O0`; con eso
  arreglado hay **tres familias con build optimizado** y los ciclos por iteración
  quedan **2.271** (STM32 `-Os`) · **2.976** (P4 `-Os`) · **7.756** (S3 `-Os`). Los
  dos primeros se parecen; el S3 va **3,4× peor que el mejor**, y **no es el build**,
  porque es el mismo `-Os` que el P4. O sea que lo que quedaba de sospecha —dónde
  ejecuta el intérprete— **sobrevive y ya no tiene alternativas cómodas**.
- ✅ **Tanda 5 · Eventos** (6). Verde, sin hallazgos — igual que en la Metro.
- ✅ **Tanda 6 · Lo nuevo del lenguaje** (8). Verde. Tuplas, `long`/`double`,
  enteros estrechos, sobrecarga: todo lo que ensanchó V4 pasa también en la
  tercera arquitectura, y con **96 KB de heap**, que es el peor caso del parque.
- ⚠️ **Tanda 7 · Dispositivo** (2). `BoardTest` verde, con **el hallazgo 19**
  dentro: `variant=B` en un ESP32-S3 (ver el registro). **`NeoTest` NO cuenta como
  verde**: en el ESP32 no hay backend de WS2812 —sólo lo registra la Pico— y la
  fachada devuelve éxito sin hacer nada, así que el sample pasa **sin encender
  nada**. Es el **hallazgo 21**. *Lo di por bueno al anotar la fila; lo corrijo
  aquí.*
- ✅ **Tanda 8 · Pack ejecutable EN LA PLACA** (#310). Verde a la primera, y con
  ella **la fila b1 cerrada**. Vale doble: es la **segunda familia** que ejecuta
  un pack desde `Run on Device` —o sea que los hallazgos **16** (construirlo) y
  **18** (subirlo cerrado) quedan confirmados en un micro que no es RP2350—, y
  es el camino de pack sobre `fs_lfs_esp32.c`, distinto del de la Metro.

**🏁 b1 CERRADA (4-ago): 8 tandas, 8 verdes.** Un solo hallazgo (el 19,
informativo, a V5) y **cero rojos**. Con ella son **dos familias completas** —
RP2350 (a1+a2) y Xtensa (b1)— y las tres arquitecturas donde el arreglo de #369
está probado por caminos distintos.

### b2 · ESP32-P4 Kit
PSRAM 32 MB, MIPI-DSI **EK79007 1024×600**, táctil **GT911 (I2C 0x14)**, Ethernet IP101.
**AOT RISC-V con `.mdn` dinámico** (#H4, 113×).

**Referencia de la placa (4-ago, imagen `8e1c3535`):** `esp32p4` · **360 MHz** ·
55 GPIO · 14 PWM / 14 ADC · flash 16 MB · SRAM 768 KB · **PSRAM 32 MB** · VM heap
**27,5 MB** + stack 512 KB · FS 100 KB / 6,8 MB (recién formateado — los 100 KB
son la línea base de un FS vacío, el mismo número que salió en la Pico).

**La otra punta del parque.** El heap es **286×** el del S3 (96 KB) y **110×** el
de la Pico (257 KB). Aquí un rojo por memoria no es información sobre la
plataforma: si algo se queda sin memoria con 27 MB, es un bug. Y es la **única
placa con AOT en marcha**, así que es la tanda que ejercita lo que en el S3 no se
pudo probar (Xtensa no tiene AOT).

**La placa venía sin particiones y el IDE lo dijo** antes de intentar nada. Es
H9 (#303) haciendo su trabajo: el arranque por capas se para en el estado que
toca y lo cuenta, en vez de fallar mudo más adelante.

⚠️ `BoardTest` dirá `variant=B` también aquí (55 GPIO ≥ 40). **Es el hallazgo 19,
ya registrado** — no hace falta volver a anotarlo.

**Tandas (4-ago):**

- ✅ **Tanda 1 · Humo** (7). Verde.
- ✅ **Tanda 2 · Memoria** (10). Verde con **27,5 MB de heap**. Aquí el criterio se
  da la vuelta respecto al S3: con este heap, que los samples de estrés pasen no
  demuestra gran cosa — lo que cuenta es que **la memoria vuelva a su sitio al
  acabar el RUN**, que es lo que mide el guardián de #339.
- ✅ **Tanda 3 · Ficheros** (8). Verde, y con **una medida de Eduardo que merece
  quedar escrita**: el FS va **igual de rápido que en las demás placas**. *«Aquí
  la CPU no aporta diferencia, el FS es el más lento y es el que manda.»*

  No es una anécdota, es la confirmación de que **el FS está limitado por la
  flash, no por el micro**: un borrado de bloque son milisegundos, y a esa escala
  da igual ir a 150 MHz o a 360. Dos consecuencias prácticas: (a) el rendimiento
  del FS es una propiedad **de la placa, no de la familia**, así que no hay que
  perfilarlo por micro; y (b) el mismo motor (littlefs + fachada) da el mismo
  número por dos cinturas distintas y en tres ISA — que es exactamente lo que se
  buscaba al diseñarlo con la cintura desde el día uno.
- ✅ **Tanda 4 · Threads y concurrencia** (11). Verde, y **notablemente más
  rápida**. `paralleltest` (4M de iteraciones interpretadas, dos threads):

  | | reloj | tiempo | por reloj |
  |---|---|---|---|
  | ESP32-S3 (Xtensa) | 240 MHz | 129.262 ms | — |
  | **ESP32-P4 (RISC-V)** | **360 MHz** | **33.079 ms** | **2,6× mejor** |

  **3,9× más rápido con 1,5× de reloj**, o sea **2,6× por ciclo**. Es el control
  que faltaba en la duda de rendimiento que dejó abierta la b1: la lentitud del
  S3 **no es «lo que cuesta ser un micro»** — es del S3. Misma carga, mismo
  binario del ZIP, y en los dos casos **interpretado** (`paralleltest` no tiene
  funciones `native`, así que el AOT no entra en esta medida: es intérprete
  contra intérprete en dos ISA).

  Los dos threads BP dan `sum=656067456` idéntico, y el planificador de la BP-VM
  reparte igual de acompasado que en las otras placas — primera vez que ese
  planificador corre sobre RISC-V.

  ⚠️ **Y conviene dejar escrito qué NO mide esto, porque induce a error**: un
  **thread de BP no es una task del RTOS ni un núcleo**. Los threads de BP son
  verdes: los reparte **el planificador de la propia VM entre quanta**, y en
  placa la VM corre en **una sola task** en las tres familias — verificado en el
  código: `esp32/main/repl_esp32.c:807` y `stm32/port/stm32_repl.c:561` llaman a
  `bpvm_run(vm)` a secas (el comentario del ESP32 lo dice: *«el SMP en ESP32 es
  H4.2+»*), y el Pico sólo entra por `bpvm_run_smp` si se define
  `BPVM_PICO_SMP_WORKERS` al configurar, que **no está definido** en la imagen
  que se publica. O sea que **el segundo núcleo del P4 no ejecuta BP**, y esta
  medida es intérprete contra intérprete **en un solo núcleo a cada lado**: la
  comparación entre placas es de **reloj y arquitectura, nada más**. (Corrección
  de Eduardo, 5-ago: *«los BPThreads no son los RTOS_Threads»*.)

  Sigue sin cerrarse la pregunta de **por qué** el S3 va lento por ciclo (la
  sospecha del intérprete desde flash por caché en vez de IRAM), pero ya no es
  una anomalía sin referencia: hay dos puntos y el reparto entre ellos es
  arquitectura, no reloj. → **V5**.
- ✅ **Tanda 5 · Eventos** (6). Verde. Tercera arquitectura para H5.c, y **la que
  más importa de las tres**: la escalera de GUI que viene detrás se apoya entera
  en este mecanismo —el lazo de #324 son eventos— así que verlos verdes *antes*
  de tocar la pantalla separa dos causas que si no vendrían juntas.
- ✅ **Tanda 6 · Lo nuevo del lenguaje** (8). Verde. Tercera arquitectura, sin
  sorpresas.
- ✅ **Tanda 7 · Dispositivo**. `BoardTest` verde (con el `variant=B` del hallazgo
  19, esperado). **`NeoTest` no puntúa aquí tampoco**: el P4 tampoco registra
  backend de WS2812 — hallazgo 21.
- ✅ **Tanda 8 · Pack ejecutable EN LA PLACA** (#310). Verde. **Tercera familia**
  con el pack ejecutándose desde `Run on Device`: con RP2350 y Xtensa detrás, los
  hallazgos 16 y 18 quedan confirmados en las tres, y #310 con ellos.

**Batería estándar del P4: 8/8.** Lo que queda de la fila b2 es lo propio de esta
placa —la escalera de GUI de 16 peldaños y el AOT—, o sea que es la placa a la
que **más le queda por delante**, no la que va más adelantada.
#### Tanda GUI del P4 — orden de dependencia (4-ago)

**En escalera: cada peldaño supone el anterior.** Si uno falla, no sigas hacia
abajo — el siguiente hereda el fallo y no dice nada nuevo.

| # | Qué se corre | Qué demuestra | Ojo con |
|---|---|---|---|
| 1 | `GuiDemo` | Que pinta algo. El primer programa GUI de BP (V3) | Si esto falla, salta a la escalera de bisección de abajo |
| 2 | `GuiColorDemo` | Color de fondo y de texto | **Es el del cian del P4** (#285). Quedó cerrado: era el compilador rancio que empaquetaba el IDE, no el render. Aquí se confirma con el IDE nuevo |
| 3 | `GuiGeomDemo` | Geometría explícita (x/y/ancho/alto) | El P4 es 1024×600: es la resolución más grande del parque |
| 4 | `GuiClickDemo` | **Táctil** + upcall de eventos | GT911 en I2C **0x14** (no la dirección de catálogo) |
| 5 | `GuiCheckDemo` | Widget con `onChange` | Primer widget con evento propio |
| 6 | `GuiEvSpike` | Que el lazo de eventos **no duplica** (#324) | No es demo, es una MEDIDA. Lo que importa es el conteo |
| 7 | `GuiValueDemo`, `GuiLedSpin` | switch / slider / bar / spinbox / led | — |
| 8 | `GuiInputDemo`, `GuiListKbd` | dropdown, textarea, list, keyboard | Entrada de texto = el táctil trabajando de verdad |
| 9 | `GuiTabDemo`, `GuiTableDemo`, `GuiMsgDemo` | tabview, table, msgbox modal | — |
| 10 | `GuiFontDemo`, `FontLoadDemo` | Tamaños de fuente y carga de una `.bin` de LVGL | `FontLoadDemo` lee del FS: cruza GUI × ficheros |
| 11 | **`samples/imageproject`** (proyecto) | Imagen (asset + control) | **NO el `GuiImageDemo` suelto**: ése no puede subir el `.png` a la placa y encima no viaja en el ZIP → hallazgo 25. El proyecto sí, por su `resources/` |
| 12 | `GuiRotDemo` | `Gui.setRotation` en runtime | En MIPI-DSI puede no comportarse como en SPI |
| 13 | `GuiAsyncDemo` | Trabajo largo desde un handler **sin congelar la GUI** | El cruce eventos × threads. Aquí es donde se nota si el planificador y el lazo de GUI se estorban |
| 14 | `GuiGcRoot` / `GuiGcRootQ` | Que el objeto BP colgado de un widget **no lo barre el GC** | Guardián de #302 paso 1. Cadena de dos eslabones: el widget lo sostiene el `objptr` y el widget sostiene al oyente por `recv` |
| 15 | Proyecto `formdemo` | **Forms desde `.win`** (`resources/main.win`) | Es lo que colgaba en el P4 y se arregló (super() implícito + widget-sin-contenedor + FS→PSRAM) |
| 16 | **AOT desde el IDE** | Generar el `.mdn`, cargarlo, medir | El plato fuerte de esta placa. Se compara contra el **propio P4 interpretado**, no contra otra placa |

**Resultado (4-ago) — peldaños 1 a 7, todos verdes, sin sacar la escalera de
bisección:**

| # | Resultado |
|---|---|
| 1 `GuiDemo` | ✅ Pinta |
| 2 `GuiColorDemo` | ✅ **Cierra #285 por medida**: el cian era el compilador rancio que empaquetaba el IDE, no el render. Ahora, con el IDE nuevo y en la placa donde apareció, sale bien |
| 3 `GuiGeomDemo` | ✅ |
| 4 `GuiClickDemo` | ✅ **Táctil**, GT911 en I2C 0x14. De aquí sale el **hallazgo 22** (el `screen` dice 480×320 sobre un panel de 1024×600) |
| 5 `GuiCheckDemo` | ✅ Widget con `onChange` |
| 6 `GuiEvSpike` | ✅ **#324 confirmado en RISC-V**, y la lectura tuvo su momento: con el log a medias salían **5 upcalls y 2 handlers** y canté que no cuadraba. Con el log completo son **7 y 7**. Los eventos iban con retraso, que es justo lo que dice el diseño —`raise` **encola** y se drena entre quanta, no es una llamada síncrona—, así que en placa salen a rachas. **El invariante no es la alternancia, es el total.** El control de host, corrido en la VM-C, da la referencia limpia: 1 clic → 1 upcall → 1 handler, y el `3 handler` **antes** del `4 tras run()`, que es el criterio que el propio sample define para el caso bueno |
| 7 `GuiValueDemo`, `GuiLedSpin` | ✅ switch/slider/bar/spinbox/led, con valores coherentes en el volcado |
| 8 `GuiInputDemo`, `GuiListKbd` | ✅ Teclado en pantalla escribiendo en el textarea. Visto por Eduardo: **la tecla ✓ no hace nada** → **hallazgo 23** |
| 9 `GuiTabDemo`, `GuiTableDemo`, `GuiMsgDemo` | ✅ tabview, table y el modal devolviendo su índice de botón (`val=0` en el volcado) |
| 11 `samples/imageproject` | ✅ Imagen en placa **y de paso el ciclo completo de `resources/`** (#260): el IDE sube el `.png` al `/app` del device y el firmware lo encuentra. Cubre más que el sample suelto, que ni siquiera podía → **hallazgo 25** |
| 12 `GuiRotDemo` | ✅ `setRotation` en caliente sobre MIPI-DSI, que no es rotar por software un SPI. Eduardo: *«va un pelín lento pero creo que son los eventos»* → **tarea #373 a V5**, y ahí lo primero es **medir**, no optimizar: hay tres candidatos y el suyo es uno, no necesariamente el mayor — (a) el lazo de bombeo está **en BP** desde #324, o sea interpretado en cada frame; (b) la inyección de un frame BP por evento; (c) el repintado de **1024×600, 4× los píxeles del modelo lógico de 480×320** (hallazgo 22) y sin acelerador 2D. La (c) no tiene nada que ver con los eventos y puede ser la gorda |
| 13 `GuiAsyncDemo` | ✅ **La arquitectura entera a la vez**: el lazo de GUI sigue bombeando y refrescando el progreso mientras un worker hace trabajo largo. Son H2 (threads) + H5.c (eventos) + #325 (`Thread(obj::metodo(args))`) trabajando juntos, en placa y sobre RISC-V. Dato para el #373: si la GUI va fluida **compitiendo** con un worker por el mismo intérprete, el planificador no es el cuello |
| 14 `GuiGcRoot`, `GuiGcRootQ` | ✅ **Y el verde es FUERTE, no débil.** Avisé de lo contrario —«con 27 MB el GC pasa poco»— y me equivoqué: los dos samples llaman a **`gc()` explícitamente** y comprueban el clic *después*, así que **no dependen de que el heap se llene** y el tamaño aquí da igual. Lo que aguanta una recogida real es la cadena de dos eslabones: el `objptr` sostiene el widget y el widget sostiene al oyente por el campo `recv` del evento. Si a ese campo le faltara el bit de referencia en el layout, el oyente moriría vivo y el clic sería un use-after-free. Guardián de #302 paso 1 |
| 15 `samples/formdemo` | ✅ **Reverificación de verdad**: Forms desde `.win` es **lo que colgaba en esta placa**, y se arregló con tres cosas a la vez (el `super()` implícito del modelo Java, el widget-sin-contenedor y el FS en PSRAM). El log enseña el ciclo completo: `subido resource /app/formdemo/main.win (390 bytes)` y luego los handlers disparando (`onSaludar`, `checkbox cambiado`). Segundo paso por `resources/`, esta vez con un `.win` en vez de un `.png` |
| 16 **AOT** (`Bench`) | ✅✅ **116×, y es lo único de toda la campaña que no se puede probar en ninguna otra placa.** `fib(28)`: **4.543 ms interpretado → 39 ms en AOT**, mismo micro, mismo reloj, misma ejecución. Los tres controles pasan: (a) **compiló de verdad** —`AOT: compilando funciones native (target riscv — lo dice la placa)`, `[aot] Bench.mdn ✓ (1 thunk, 130 B nativo)`, `subido AOT /app/Bench.mdn (186 bytes)`—, no el `sin funciones native que compilar` de los samples de GUI; (b) **ningún RECHAZADO** de ABI, que es lo que sí pasa con el `Bench.mdn` embebido y rancio de la Pico (hallazgo 12) — aquí el `.mdn` se genera fresco y habla ABI 2; (c) **y el resultado es el mismo por los dos caminos**, `317811`: no es sólo más rápido, es que **calcula lo mismo**. Por encima de la referencia guardada de la placa (113×). Comparado con esta mañana en ARM (8.486 → 84 ms), el P4 es ~1,9× más rápido interpretando y ~2,2× en nativo |
| 16 **AOT** (`Bench`) | ✅✅ **116×, y es lo único de toda la campaña que no se puede probar en ninguna otra placa.** `fib(28)`: **4.543 ms interpretado → 39 ms en AOT**, mismo micro, mismo reloj, misma ejecución. Los tres controles pasan: (a) **compiló de verdad** —`AOT: compilando funciones native (target riscv — lo dice la placa)`, `[aot] Bench.mdn ✓ (1 thunk, 130 B nativo)`, `subido AOT /app/Bench.mdn (186 bytes)`—, no el `sin funciones native que compilar` de los samples de GUI; (b) **ningún RECHAZADO** de ABI, que es lo que sí pasa con el `Bench.mdn` embebido y rancio de la Pico (hallazgo 12) — aquí el `.mdn` se genera fresco y habla ABI 2; (c) **y el resultado es el mismo por los dos caminos**, `317811`: no es sólo más rápido, es que **calcula lo mismo**. Por encima de la referencia guardada de la placa (113×). Comparado con esta mañana en ARM (8.486 → 84 ms), el P4 es ~1,9× más rápido interpretando y ~2,2× en nativo |
| 10 `GuiFontDemo`, `FontLoadDemo` | ⚠️ `GuiFontDemo` **salió negra a la primera** → **hallazgo 24** (no era del sample: estado acumulado; con la placa reseteada, verde). `FontLoadDemo` ✅: `loadFont(...) -> id 1` leyendo el `.bin` del FS de la placa y **la fuente cargada se pinta**, confirmado por Eduardo a ojo. Éste es el primer peldaño que cruza **GUI × ficheros**, y hay que saber leerlo: el volcado **no muestra** la fuente cargada —sólo refleja `fontSize` en px, y `setFont(id)` va por otro camino—, así que aquí **el instrumento es ciego y sólo decide el ojo** |

**Si algo se rompe, la escalera de bisección ya está escrita** y no hay que
improvisar: `GuiLblMin` (lo mínimo que debería ir) → `GuiWinMin` (sólo construir
la Window) → `GuiWinLbl` (Window + Label hijo) → `GuiWinPanel` (Window + Panel) →
`GuiWinChk` (Window + Checkbox). Es la que cazó el cuelgue de Forms. `GuiRotProbe`
es la sonda del cian, del mismo estilo.

**Hueco honesto, que conviene decir antes de correr nada:** ningún sample junta
**AOT y GUI**. El `.mdn` del P4 es de `Bench`, que no pinta. La combinación
«código nativo compilado que toca objetos BP sostenidos por widgets» es
justamente el terreno del **paso 3 de #302** (shadow stack / raíces GC del native
compilado), que quedó diferido *a AOT-en-placa* — o sea, a esta placa. No es un
rojo esperado: es que **no hay quien lo pruebe**.

**🏁 ESCALERA DE GUI COMPLETA: 16/16.** Con la batería estándar (8/8) delante,
**la fila b2 queda cerrada**. Un rojo real —el **hallazgo 24**, la pantalla que
deja de pintar tras muchos RUN— y cinco hallazgos informativos (19, 20, 22, 23,
25). Es la placa que más cubre del parque: la única con **GUI completa + táctil +
AOT en placa**, y por tanto la única donde V4 se ve entera funcionando a la vez.

### b3 · ESP32-P4 Waveshare 4.3"
Panel **ST7701 480×800**, elegido por el **ENV** (`display=st7701`, #311), backlight invertido.
- [ ] El panel sale del ENV: cambiar la variable y ver que cambia el panel
- [ ] El backlight enciende (el `bl_invert` viaja con la entrada del catálogo)
- [ ] Mismo GUI que b2, a otra resolución: comprobar que la interfaz no se sale

**Referencia de la placa (4-ago, MISMA imagen `8e1c3535` que la b2):** `esp32p4` ·
360 MHz · 55 GPIO · **flash 32 MB** · SRAM 768 KB · PSRAM 32 MB · VM heap 27,5 MB
+ stack 512 KB · **FS 5,0 MB** de una zona de datos de ~10 MB → **hallazgo 27**
(el límite lo pone la tabla del IDF de la imagen, no el chip; se deja así en V4).

**Esta fila NO repite las 8 tandas, y el motivo importa**: es **la misma imagen**
que la b2, ya verde de punta a punta. Lo que aquí prueba algo que no esté probado
ya es lo que **cambia entre las dos placas** — el panel, el táctil y el arranque.
Repetir el resto mediría el mismo binario dos veces.

**Tandas (4-ago):**

- ✅ **Tanda 1 · Humo** + `RandomTest` (8/8). Verde. Su valor aquí no es el ISA
  —eso ya lo cubrió la b2— sino que **el mismo binario arranca y opera en una
  placa distinta**, que es la afirmación que sostiene la imagen única.
- ✅ **El panel sale del ENV** (#311) y **el backlight invertido enciende**.
  `GuiDemo` y `GuiColorDemo` se ven bien con el `ST7701`, lo que ya demuestra que
  el ENV **se leyó**: sin él habría arrancado con el perfil de la EV a 1024×600.
- ✅ **Táctil** (`GuiClickDemo`). Es lo único de esta placa que no comparte
  silicio con lo ya probado en la b2.
- ✅ **Geometría y tabla** (`GuiGeomDemo`, `GuiTableDemo`). El primero dibuja su
  panel de `100x50` en `pos=10,20` — correcto, y **no puede enseñar el hallazgo
  22 porque no usa `align`**: al ser geometría absoluta, el modelo lógico no
  entra en juego. *(Predije que se vería el efecto y me equivoqué de sample.)*
  El segundo ocupa sólo la parte superior, pero **eso tampoco lo demuestra**:
  como observó Eduardo, **la tabla se ajusta a su contenido** (cabecera + una
  fila), y eso se ve pequeño en cualquier pantalla. **El 22 ya está probado sin
  necesidad de la vista**: el volcado imprime `screen [480x320]` en una placa de
  480×**800**. Lo que sí discriminaría, el día que haga falta: un widget con
  **align BOTTOM** — con el modelo real saldría abajo del todo; con el de 320
  sale a media pantalla, y eso no admite otra lectura.

**🏁 b3 CERRADA.** No repite las 8 tandas por diseño (misma imagen que la b2): se
ha probado **lo que cambia entre las dos placas** —arranque, panel por ENV,
backlight, táctil y layout— y todo verde. Lo que sí ha aportado, y vale más que
la fila entera, son **dos hallazgos que sólo se ven en una placa recién
particionada**: el **28** (🔴 crítico, en virgen no arranca ninguna demo gráfica)
y el **27** (el límite de flash lo pone la imagen). **Cinco placas de siete.**

### c1 · STM32 Nucleo-U575ZI-Q
Sin pantalla. Wire por el VCP del ST-LINK. **AOT ARM**. Página de borrado de **8 KB**.
- [ ] `BlinkStm32` — LED verde PC7
- [ ] AOT: cómputo `native` y ganancia
- [ ] #338 aquí deja el buffer en **12 KB** (no 8): es su sector, no una excepción

**Referencia de la placa (5-ago):** `nucleo-u575zi` · 160 MHz · **114 GPIO** (el
INFO; el lenguaje dice 128 → **hallazgo 32**) · 28 PWM / 20 ADC · flash 2 MB ·
SRAM 768 KB · **VM: heap 372 KB medidos** tras subir el bloque a 512 KB
(hallazgo 31; antes 64) · **partición FS 704 KB** (el primer número del panel
es lo USADO —`fsUsedBytes`—, no una propiedad de la placa: sube según lo que
haya subido; 192 KB al empezar la fila, 328 KB tras las tandas 1-3) · página de borrado **8 KB**, la
única de las tres familias que no es de 4 (**hallazgo 29**).

**Tandas (5-ago):**

- ✅ **Tanda 1 · Humo** (7). Verde **dos veces**: una con la imagen de 128 KB de
  bloque y otra tras subirlo a 512 (hallazgo 31). La segunda es la que cuenta.
- ✅ **Tanda 2 · Memoria** (10) **+ `MemInfo`**. Verde, y es la tanda que
  **reverifica el hallazgo 31 en placa**, que era lo único que le faltaba:

  | medida | valor |
  |---|--:|
  | mayor bloque reservable | **244 KB** |
  | total en trozos de 4 KB | **372 KB** |
  | total en trozos de 1 KB | 369 KB |

  Los 372 KB medidos **son** los 384 que predice la regla de reparto menos lo
  que el propio runtime tiene ya en pie: el modelo no se ha estimado, se ha
  comprobado. Y el hueco entre 244 y 372 es **fragmentación**, exactamente lo
  que `MemInfo` existe para separar. El mismo binario en el host con
  `--mem=786432` da los mismos tres números: **paridad host↔placa byte a byte**.

  🔁 **Repetida sobre la imagen `-Os` (hallazgo 33) — y un número se movió:**

  | medida | con `-O0` | con `-Os` |
  |---|--:|--:|
  | total en trozos de 4 KB | 372 KB | **372 KB** (380.928 B) |
  | total en trozos de 1 KB | 369 KB | **369 KB** |
  | **mayor bloque** | **244 KB** | **372 KB** ← igual al total |

  Lo importante primero: **los totales no se han movido ni un KB**, que es lo que
  había que comprobar tras cambiar la imagen — el `bss` decía que el bloque de
  512 KB seguía entero y la placa lo confirma. El modelo de reparto aguanta.

  ⚠️ **Pero el mayor bloque pasó de 244 a 372 KB, o sea que el hueco de
  fragmentación se cerró a cero — y eso NO lo predije ni lo sé explicar todavía.**
  Un flag del compilador no tiene por qué cambiar el comportamiento del alocador:
  el código es el mismo, la secuencia es determinista y el heap mide igual. Así
  que antes de contarlo como mejora hay que entenderlo, que es justo lo contrario
  de lo que apetece cuando un número sale bonito.

  **La sospecha más barata es la HISTORIA**: el «mayor bloque» no mide capacidad,
  mide **fragmentación**, y por tanto depende de lo que corriera antes. La medida
  del `-O0` se tomó al final de una tanda 2 entera; ésta, sobre una placa recién
  reflasheada y arrancada. **El experimento que lo decide cuesta 30 segundos:
  correr `MemInfo` dos veces seguidas.** Si la segunda vuelve a dar 244, la
  fragmentación **sobrevive al RUN** —y eso sí sería un hallazgo, porque el heap
  debería reinicializarse en cada ejecución (es lo que vigila #339)—. Si da 372
  las dos, la explicación está en otro sitio y habrá que buscarla.
- ✅ **Tanda 3 · Ficheros** (8). Verde. No es una repetición de las otras dos
  familias: aquí corre la **tercera cintura** del mismo motor
  (`fs_lfs_stm32.c` sobre la HAL — ni `flash_range_*` ni `esp_partition`) y
  además con la **página de borrado de 8 KB**, la única del parque que no es de
  4. Que littlefs dé el mismo resultado con el doble de sector es la prueba de
  que el tamaño está donde tiene que estar —en la cintura— y no horneado en el
  motor.

  🔁 **Repetida sobre la imagen `-Os`**: verde otra vez, sin hallazgos. Con esto
  **las tandas 1, 2 y 3 quedan reverificadas sobre el firmware que se publica**,
  y la c1 vuelve al punto donde estaba antes del hallazgo 33 — pero ya sin
  deuda: lo que se pruebe de aquí en adelante se prueba **una sola vez**.

**🔬 Observación de Eduardo, sin medir todavía: la flash INTERNA parece bastante
más rápida.** Cargar y borrar ficheros va notablemente más ágil que en el P4, y
su hipótesis es el tipo de flash — la Nucleo la tiene **interna**, el P4
**externa SPI**.

Lo que la refuerza es que **explica también la observación de ayer**: en la fila
b2 quedó anotado que el FS iba igual de rápido en Pico, S3 y P4 y que *«el FS es
el más lento y es el que manda»*. Las tres tienen **flash externa SPI**. La
Nucleo es la primera con flash interna, y es la primera que rompe el patrón —
una hipótesis que explica a la vez la regularidad y su excepción vale más que
una corazonada. Los órdenes de magnitud cuadran: una NOR SPI borra un sector de
4 KB en decenas de ms y programa de 256 en 256 B; la flash del U5 está en el bus,
borra páginas de 8 KB y programa de 128 bits.

⚠️ **Pero hay un confusor, y conviene dejarlo escrito**: *cargar* pasa por el
wire **y** por la flash, y los enlaces no son comparables (la Nucleo va por el
**VCP del ST-LINK**, el P4 por un **bridge USB-UART**). *Borrar* no transfiere
nada: es flash pura. Como Eduardo observa que **borrar también va más rápido**,
esa mitad no la explica el enlace. **Sigue siendo hipótesis** hasta que se mida
con un cronómetro; queda anotada como caracterización, no como hallazgo — aquí
no hay nada roto.

**Ampliación tras la tanda 3 — y aquí el experimento se cierra solo.** Eduardo:
*«es curioso porque la Nucleo, que es 3 veces más lenta en CPU, sin embargo
parece más ágil por el sistema de archivos»*. Puesto al lado de la observación
de la b2, lo que hay es un **experimento controlado por accidente**, hecho en
dos mitades y sin querer:

| | CPU | flash | ¿FS? |
|---|---|---|---|
| Pico · S3 · P4 (b2) | 150 → 350 MHz | **externa SPI** en las tres | **igual** |
| Nucleo (c1) | **160 MHz**, la más lenta | **interna** | **más rápida** |

La primera mitad **varió la CPU dejando la flash igual**: no cambió nada. La
segunda **cambia la flash con la CPU yendo en contra** —la Nucleo pierde por
2,2× de reloj— **y aun así gana**. Esa
es la forma fuerte del argumento: cuando la máquina más lenta gana, la velocidad
del micro no puede ser la explicación. La causa queda en la flash, que es donde
apuntaba la hipótesis.

**Y la conclusión de Eduardo es la que vale**: *«no es algo que dependa de
nosotros ya que es hardware, pero a la hora de diseñar un equipo puede ser
importante»*. Es un **criterio de elección de placa**, no un pendiente nuestro:
para una aplicación que toca ficheros —cargar, listar, borrar, o sea lo que el
usuario hace todo el día— **el tipo de flash manda más que los MHz**. Un micro
de gama alta con NOR SPI externa se sentirá menos ágil que uno modesto con flash
interna. 📝 Candidato a una línea en la guía de elegir placa; no es V4.

⚠️ **Lo que sigue sin medirse**: cuánto. «Más ágil» no es un número, y los dos
enlaces siguen sin ser comparables. Para convertirlo en dato haría falta
cronometrar **borrar N ficheros** (flash pura, sin wire) en las dos placas.

**Y el hallazgo 33 obliga a releer esta fila — pero la refuerza, no la tumba.**
La comparación de la mañana se hizo con la Nucleo compilada a **`-O0`** y el P4
a `-Os`: o sea que **la Nucleo iba lastrada y aun así ganaba**. Tras reflashear
con `-Os`, Eduardo la ve *«todo parece más ágil»* — impresión, no medida, pero
con mecanismo: el `-Os` no llegó sólo al intérprete, llegó a **todo el firmware**
(el wire, la cintura de littlefs, el JSON del protocolo, los caminos de
PUT/DEL/LIST).

Dos consecuencias, y conviene no confundirlas:

1. La conclusión de la **b2** —*«la CPU no aporta, el FS es el que manda»*— queda
   **intacta**: allí se compararon Pico, S3 y P4, y **las tres iban optimizadas**
   (`-O3`, `-Os`, `-Os`). El `-O0` sólo estaba en el STM32.
2. Que quitarlo se note en cargar y borrar dice que **nuestro código no era
   despreciable** en esas operaciones. No contradice que la flash mande, pero sí
   avisa de que el reparto flash/código no está medido — y ahora que las cinco
   imágenes van optimizadas, es el momento bueno para medirlo si algún día se
   quiere.

**🏁 c1 CERRADA — y es la fila que MÁS ha aportado de las seis.** Ocho tandas +
`BlinkStm32` + `MemInfo` + AOT, todo verde, con un único fallo (**34**) que se
curó con un reset y resultó ser el 24. **Sexta placa de siete.**

Lo que ha dejado, que no es poco para una placa sin pantalla:

- 🔴 **Hallazgo 33** — el firmware STM32 se publicaba a **`-O0`**. Arreglado y
  verificado: **3,65×** y **−40 %** de imagen. Es el hallazgo más grande de
  H13 después del 28, y **salió de un número que no cuadraba con el silicio**.
- 🔴 **Hallazgo 31** — medio mega de RAM parado. Arreglado: heap 64 → **372 KB
  medidos en placa**.
- 🔴 **Hallazgo 35** — el AOT se publica **sin un solo ejemplo**. Al lote del ZIP.
- 🟡 **Hallazgos 29, 30, 32** — mensajes e INFO. Al lote de firmware.
- 🔬 **Hallazgo 34** — segunda vista del 24, **en otra familia y sin LVGL**:
  eso tacha la pila gráfica y deja la degradación en el núcleo portable. La
  gravedad se revisó a **condicional** y el método de la investigación quedó
  escrito en `docs/V5_IDEAS.md`.
- 🛠️ **`samples/MemInfo.bp`** — herramienta nueva: mide el heap desde BP.
- 📐 **Paridad host↔placa demostrada con números**: 372 / 372 / 369 en los dos.
- 📋 **Cuatro tareas de V5** nacidas aquí (#374, #376, #377 y la ampliación de
  #372), más la hipótesis de la flash interna y la regla del 5-ago.

### c2 · STM32 Discovery U5G9J
Pantalla **LTDC**. Es la placa gráfica de la familia.

**Referencia de la placa (5-ago, tras reparticionar):** `u5g9j-dk2` · 160 MHz ·
flash **4 MB** · **SRAM 2,9 MB** —la mayor del parque sin contar PSRAM— ·
partición FS **2 MB** · página de borrado **8 KB** · imagen `-Os` del hallazgo 33
(`bpvm_stm32_dk2.bin`, `49c39824`).

📐 **Lo que la placa NO usa, y por qué no se toca.** El estático de esta imagen
son ≈**1,52 MB** (`text` 868.940 · `bss` 1.594.312, del enlace de hoy), o sea
que quedan ≈**1,36 MB de SRAM parados**. Y el heap de la VM es **el mismo que en
la Nucleo** —372 KB— porque `s_vm_mem` es **un solo array de 512 KB compartido
por las dos placas de la familia** (`stm32_repl.c:66`). Es exactamente la forma
del hallazgo 31, pero **ya no es un descuido: es el precio conocido de *una
familia, una imagen***. Con la regla del 5-ago encima, **no se toca en V4** —
y no está roto: 372 KB es más heap que el S3 y que la Pico. → **V5**, junto con
el 27 y el 374 (que el recurso lo diga la placa, no una constante).

**Tandas (5-ago):**

- ✅ **Tanda 1 · Humo** (7). Verde. **Séptima placa que arranca y ejecuta**, y la
  cuarta imagen distinta del día tras el `-Os`: el binario del U5G9 —otro chip,
  otro mapa de memoria, 2,4× de estático y LVGL dentro— carga y corre igual.

### Plan de la c2: primera parte ABREVIADA, y el peso en el GUI

Propuesta de Eduardo: *«esta placa tiene un micro casi idéntico al anterior, no
podemos hacer una primera parte de test abreviados y nos centramos más en la 2ª
parte con todos los test del GUI»*. **Sí — pero abreviando por lo que DE VERDAD
cambia, no por parecido.** Y conviene precisar una cosa antes, porque el caso
**no es el de la b3**: allí era **la misma imagen** en otra placa, y por eso
repetir las 8 tandas no probaba nada. Aquí el binario **es otro**
(`bpvm_stm32_dk2.bin` ≠ `bpvm_stm32_nucleo.bin`), el chip es otro (U5G9 vs
U575), el mapa de memoria es otro y el estático es **2,4× mayor**. Lo idéntico
son las **fuentes**, no lo que corre.

Así que se queda lo que cubre una diferencia real, y se cae lo que sólo
repetiría núcleo ya verificado en tres arquitecturas:

| tanda | ¿se hace? | por qué |
|---|---|---|
| **1 · Humo** (7) | ✅ **sí** | es la puerta: *¿este binario arranca y ejecuta?* Barata y no negociable |
| **2 · Memoria** → sólo **`MemInfo`** | ✅ **abreviada a 1** | primera vez que el bloque de VM de 512 KB **convive con los buffers de LVGL** en la misma SRAM. Un solo RUN contesta |
| **3 · Ficheros** (8) | ✅ **sí** | geometría distinta: flash **4 MB**, partición FS **2 MB** (la Nucleo: 2 MB / 704 KB). Aquí es donde asoma un mal cruce partición↔enlazador |
| 4 · Threads (11) | ❌ no | núcleo portable puro, mismas fuentes, verde en **tres** arquitecturas |
| 5 · Eventos (6) | ⚠️ **la cubre el GUI** | el lazo de #324 **son eventos**: si la escalera de GUI va, la 5 está probada por dentro |
| 6 · Lenguaje (8) | ❌ no | ídem 4 |
| **7 · Dispositivo** → sólo **`BoardTest`** | ✅ **abreviada a 1** | es donde vive el hallazgo 32, y esta placa lo **agrava** |
| 8 · Excepciones (7) | ❌ no | ídem 4 |

**≈17 samples en vez de 60**, y cada uno con un motivo escrito.

🎯 **Y hay un premio que Eduardo no ha pedido pero que cae solo**: acaba de
**reparticionar, «todo limpio»** — que es **exactamente el estado en el que
apareció el hallazgo 28**, el crítico (en un dispositivo virgen no arrancaba
ninguna demo gráfica porque `Gui.mod` no viajaba en el ZIP). O sea que el primer
`GuiDemo` de esta placa **reverifica el 28 en las condiciones donde falló, y en
otra familia**. Si sube `Gui.mod` solo, el arreglo queda confirmado dos veces.

⚠️ **Y el INFO trae de serie los hallazgos ya conocidos de la familia**: no dice
el heap de la VM (**30**) y da `GPIO 114` (**32**) — pero aquí el 32 se agrava,
ver abajo.

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

**✅ El Stop dejó de ser una lotería** (Eduardo, 4-ago, sobre el P4): *«Antes los
stop funcionaban alguna vez, ahora están funcionando. Eso me permite hacer una
demo tras otra sin necesidad de resetear.»*

Vale la pena escribirlo porque es **un resultado, no una impresión**, y de los
que se pierden si nadie los anota: *«funciona a veces»* es justo lo que vuelve a
romperse sin que salte ningún test. Lo que lo hace creíble es que el síntoma
—encadenar RUN sin reiniciar la placa— es el de #352 (el modelo del GUI se
rompía a los 30 RUN) y el de #344 (el verbo RUN escrito una sola vez), y la
intermitencia es la firma de la campaña de memoria. **No lo atribuyo a un commit
concreto**: no hay medida que lo reparta, y aquí eso importa. Lo que sí queda
⚠️ **CORREGIDO DOS VECES, y la segunda es la buena** (b3, misma tarde). Primero
anoté que el Stop «volvía a fallar». Luego Eduardo lo caracterizó de verdad, y
resulta que **son DOS cosas y sólo falla una**:

- ✅ **El KILL del programa BP FUNCIONA** — la aplicación para, que es lo que
  #257 prometía y lo que de verdad importa.
- ❌ **Lo que queda mal es el WIRE**: tras el Stop hay que cerrar la comunicación
  y reconectar, *«y a veces hay que hacerlo una segunda vez»*.

Eso cambia dónde hay que mirar: no es el KILL, es el **estado del transporte
DESPUÉS del KILL** — la VM muere con un frame a medias y el siguiente comando
entra desincronizado. Que hagan falta **dos** reconexiones apunta a un buffer que
se drena por tandas, el mismo terreno que el drenaje adaptativo del `Connect` en
`SerialBackend`. Criterio de Eduardo: *«eso es mejor que tener que resetear»* →
**molesto, no bloqueante**: va **al lote**, no a la cabeza.

*Nota de método: mi «vuelve a fallar» era demasiado grueso y habría mandado a
buscar al KILL, que está bien. La caracterización del usuario valía más que mi
observación.*

Lo que sí queda dicho es que **el KILL de punta a punta (#257) funciona cuando funciona**,
y que la forma de trabajar que habilita —demo tras demo sin reset— es la que ha
hecho posible correr las tandas de hoy al ritmo que se han corrido.

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

## 🛑 Regla del 5-ago (media mañana): SE ACABAN LOS CAMBIOS DE FIRMWARE

Dictada por Eduardo tras repetir las mismas tandas **tres veces en una mañana**
(hallazgo 31 → reflasheo; hallazgo 33 → reflasheo): *«salvo que salga algo
terrible, no hacemos más cambios de firmware»*. Y tiene razón: los dos cambios
valían la pena por separado, pero **el coste no lo pago yo, lo paga él**, y tres
repeticiones de la misma fila es exactamente lo que el método por lotes existe
para evitar. Esto no es una regla nueva: es **hacer cumplir la que ya había**.

**Qué cuenta como «terrible»** (y por tanto interrumpe), decidido de antemano
para no tener que discutirlo cada vez:

- **Se pierden o se corrompen datos** — FS, packs, flash.
- **La placa no arranca, se cuelga o hay que desenchufarla** para recuperarla.
- **La VM da resultados incorrectos** o corrompe memoria.
- **Algo publicado no se puede usar recién instalado**, como el hallazgo 28.

**Qué NO cuenta**, por evidente que parezca la mejora: rendimiento, mensajes
poco claros, campos que faltan en el INFO, cosméticos, y cualquier cosa que ya
esté en el lote. Todo eso **se sigue anotando** —el registro no se cierra— pero
va al lote y no toca la placa.

⚠️ **Lo que sí queda pendiente y hay que decir ahora para que no sorprenda**: el
lote (12, 13a, 21b, 29, 30, 32 + Stop/wire) **es de firmware**, así que al
cerrarlo habrá **un reflasheo por familia, UNA vez, al final de las campañas**.
Eso estaba en el plan desde el principio; lo que se acaba es reflashear **en
mitad de una fila**.

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

> No hay un hallazgo 14: es un salto de numeración mío, no un apunte perdido.

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
| 9 | — (VM-C) | 🟡 **CORREGIDO 3-ago — yo lo anoté mal.** Escribí que «la VM-C no imprime traza NI el mensaje» de una excepción no atrapada. **El mensaje SÍ lo imprime**, y desde hace tiempo: `exceptions.c:97` saca `EXCEPCION NO ATRAPADA en el thread N` y `:111` saca `el mensaje que traia: "..."`. Salió al hacer el control del `RandomTest` nuevo, forzándolo a rojo. Lo que de verdad falta es la **TRAZA**: la cadena de funciones con `fichero:línea` que sí da la VM-Java | ✅ el hueco es **más pequeño** de lo que dije: no es «no dice nada», es «dice qué pasó pero no dónde». De los tres tamaños que te ofrecí, **el (1) —sólo el mensaje— ya está hecho**. Queda decidir sólo entre dejarlo así o invertir en (2) la cadena de funciones por nombre / (3) la traza completa, que necesita el mapa PC→línea y hoy no viaja a la placa. Para V4 mi recomendación es dejarlo: con el mensaje y el código de salida se diagnostica |
| 13 | a1 Pico | `FileOpsTest` moría en placa con `lastModified('fops_orig.txt'): no se pudo leer`. **No es un fallo: littlefs no guarda fechas** (`fs_lfs.c:275`, `.mtime_ms = NULL` → la fachada devuelve −1 → el builtin lanza). Dos cosas alrededor sí estaban mal: (a) **el mensaje miente sobre la causa** — dice «no se pudo leer», que suena a fallo de E/S, y mandó a Eduardo a buscar un problema de tiempos; (b) **la documentación estaba rancia**: decía «el FS del firmware es plano (sin directorios)» y listaba `mkdir`/`rmdir`/`copyFile`/`isDirectory` como no disponibles en placa — desde H2 (littlefs) los cuatro **funcionan**, y la propia captura de Eduardo lo enseña (`fops_dir/copia.txt` en el árbol) | ✅ (b) corregido en los dos idiomas + nota en la tabla de `lastModified`; ✅ el sample atrapa y dice «mtime = no disponible (este FS no guarda fechas)» — en el PC sigue imprimiendo `mtime>0 = true`, paridad intacta. 🟡 **(a) EN EL LOTE**: cambiar el texto a la causa real es una línea de `builtins.c`, pero obliga a recompilar las 3 familias. A favor de hacerlo: el mensaje engañó al autor del lenguaje |
| 15 | a1 Pico | 🔴 **CONFIRMADO EN PLACA: el IDE sube un `.mod` v5 y el sample muere (exit 3).** Al correr `l2app`, el IDE resolvió `L2Lib` a un `.mod` **v5** y lo subió; la placa lo rechazó. La cadena: `bpstdlib/` —lo que `BpVM.cfg` declara como `stdlibDir`— tiene 26 módulos legítimos (v6) y **24 intrusos, todos v5** (23 demos de Gui y `L2Lib`), restos de una compilación vieja, ninguno con `.bp` que lo respalde. Y `resolveDeviceDeps` da prioridad de stdlib a **cualquier** módulo presente en `stdlibDir` (`FrmMain.java:2777`, la cláusula `|| isRegularFile(stdlibDir/imp + ".mod")`, ampliada el 26-jun para cubrir `Json`). O sea que el intruso **manda sobre el módulo recién compilado**. El frontend sí genera un `L2Lib.mod` MOD6 en `outDir`: nunca llega a mirarse. Hay tres copias rancias del mismo módulo (`bpstdlib/`, `samples/`, `samples/out/`), y sólo la de `bpstdlib` hace daño | ✅ **guardián puesto**: `montar-zip.sh` recorre el paquete montado y si un `.mod` no es MOD6, **no sella**. Una regla, sin lista que mantener, y vale igual para la próxima subida de formato. Verificado que dispara: hoy encuentra los 24. ✅ **CERRADO** (`f22d8be`, 3-ago): fuera **72 artefactos rancios** — los 24 `.mod` de `bpstdlib/` sin `.bp` que los respalde (los que sí viajaban en el ZIP) y 48 generados de `samples/` (24 `.mod` v5 + 24 `.dbg`, ruido del repo). Y **segundo guardián** en `montar-zip.sh`: el invariante de `bpstdlib/` — un módulo de la librería estándar tiene que tener su `.bp` al lado. Verificado hoy: **0 intrusos**. |
| 16 | a1 Pico | 🔴 **Run on Device IGNORA `out: pack`.** Con el proyecto `sampleproject` la placa recibió tres ficheros sueltos (`App.mod`, `Util.mod`, `leeme.txt`) y **del pack ni rastro**. No es del firmware: `bpvm_load_entry` despacha `.mod`/`.pack` (#344) y el REPL registra `run: pack '<x>' (main=<y>)`. Es el IDE. Dos cosas en el camino de placa (`FrmMain.java:2447`): (a) `modPath = outDir.resolve(moduleName + ".mod")` está **fijo**, sin rama de pack —la lógica `packRun` existe sólo en el camino de la VM del PC, líneas 2368-2370—; y (b) compila el `.bp` suelto con `compileFile`, no el proyecto con `buildProject`, así que el `.pack` **ni se construye**. Y `packBurn` sólo se llama desde `PacksPanel`, que no tiene «ejecutar». O sea: la función existe en la placa y **no hay botón que llegue a ella desde un proyecto**. Los 3 ficheros son correctos para el camino sin pack (el `leeme.txt` es #260 y está previsto) | ✅ **ARREGLADO** (`acd6718`): `Run on Device` respeta `out:pack` — compila con `buildProject` y sube el `.pack`. **Verificado en placa en dos familias**: Metro (tanda 8) y S3 (tanda 8), las dos verdes a la primera. Le faltaba un hermano, el hallazgo 18 (subía además el contenido suelto), arreglado aparte en `16c7970`. ⏩ *Rodeo que se usó antes de tenerlo*: `Build Project` → `Upload…` el `App.pack` a `/app` → consola `autorun /app/App.pack` → `reset`. Eso ejerce formato del pack + despacho + `main:` del manifest en placa. Lo que **no** cubre es ejecutar desde la zona de packs (XIP); el burn y la persistencia ahí ya se verificaron en #327. **Segundo caso del mismo patrón**: el camino de host se lleva la función y el de placa se queda atrás — como la lista `EMBEDDED_CORE_MODS` |
| 17 | a1 + a2 | 🔴 **ABIERTO — `use-after-free` sistemático, BIESTABLE y PERSISTENTE.** Muere a ~1 ms del RUN, con la memoria de la VM limpia al final (`0 bloques sin liberar`). **Cuando falla, falla siempre; cuando va, va siempre — y NO cambia con un reset.** **DESCARTADO CON EVIDENCIA**: (1) el tamaño del heap — 32 vueltas verdes en host, {1,2 workers} × {369 KB, 8 MB}; (2) la **PSRAM** — con `psram=0` y el heap en SRAM sigue fallando; (3) una **carrera SMP** — una carrera no sobrevive a un reset; (4) `Core` rancio — el `.mod` de `bpstdlib` y el embebido en el firmware son **byte a byte idénticos**; (5) la **zona de rascar** compartida (#338) — está guardada y diría `scratch OCUPADA`, y esa línea no sale en ningún log; (6) el **contenido de `/app`** — con `/app` vacío sigue fallando; (7) el **número de ficheros** — la teoría del umbral (10 va / 11 rompe) se cayó al vaciar `/app`; (8) `FS_MAX_FILES` — está definido pero **no lo usa nadie**, resto del FS plano de #305. **GIRO (3-ago, tarde)**: añadir ficheros a `/app` (los R2..R4 de la escalera) **lo ARREGLÓ** — y antes añadir el 11º lo había ROTO. O sea que **no es monótono en ninguna dirección**: no es *cuántos*, es **cómo queda colocado**. Los dos estados verificados estables: 5 vueltas verdes con totales correctos en el bueno, 3+ rojas en el malo. Hipótesis viva: desde #305 el `.mod` **se lee por trozos** (`bpvm_fs_read_at`); un fallo en un borde de bloque haría que **el mismo fichero, recolocado, se cargue bien o mal** — explica el sistematismo, la memoria limpia (no hay fuga: hay un módulo mal leído) y la muerte al primer acceso. **NO confirmada.** **LOCALIZADO**: sobrevive al reset ⇒ **está en FLASH**, no en RAM. Sólo quedan FS (littlefs), zona de packs, ENV/particiones y el log. Dato duro: con `/app` VACÍO el FS dice **17 ficheros, 131072 B = 128 KB exactos = 32 bloques**, frente a los **100 KB** de un FS recién formateado. Hay ~28 KB de metadatos/bloques sin compactar: **borrar ficheros NO devuelve el FS a su estado anterior** | ✅✅ **CERRADO Y VERIFICADO EN PLACA** (4-ago, #369): **no era el FS ni la colocación de ficheros — era el frame inicial del thread, que se quedó en 4 bytes tras el ensanchado 4→8B.** El spawn construía a mano el primer marco de llamada con el layout PRE-4→8B: escribía `this` en 4 bytes con `bp = sb+16`, y `run()` leía sus 8 bytes en `sb-4` → la mitad baja (idx) correcta y **la generación traída de memoria por debajo de su propia pila**. Un handle con gen 0 acierta por casualidad hasta que el GC recicla ese slot: de ahí la biestabilidad y que el reset no cambiara nada (decide el estado del heap, no el arranque). Cerrado en las DOS VMs y verificado en **tres arquitecturas** (Cortex-M33 ×2 y Xtensa), porque el arreglo vive en código común. Guardián de regresión nuevo: `samples/ThreadFieldTest.bp`, **verificado que sabe fallar** (reintroduciendo el marco de 16 bytes reproduce la firma exacta de placa). *Todo lo tachado de la izquierda queda como registro de lo que se descartó — y de que la línea del FS estaba mal orientada.* ~~En curso, en el estado malo y ANTES de formatear~~: (a) `LIST` completo como mapa; (b) la escalera `diag/uaf17/R1..R6` —que se corrió en el estado bueno por error, donde no dice nada—; (c) `MemT1_Oo`, que crea objetos **sin threads**: si también falla, el bug no es de threads y toda la línea de hoy estaba mal orientada. Luego formatear el FS y, si eso lo cura, buscar cuántos bloques hacen falta para reproducirlo a voluntad |
| 6 | a1 Pico | 🔴 **El paquete no compilaba NADA que importara la stdlib.** El ZIP llevaba `bpstdlib/*.mod`, pero el compilador sólo sabe dónde están si encuentra un `BpVM.cfg` con `stdlibDir`, y ese fichero no iba dentro. En el repo funcionaba porque ahí sí existe. Encima, al no resolver `import Core` el compilador **lo omite y sigue**, así que los 4 errores salían en el código del usuario (`ExcCatchTest`) en vez de decir «no encuentro Core» | ✅ el ZIP lleva `BpVM.cfg` con rutas **relativas** (`./bpstdlib`) — `VmConfig` las resuelve contra el directorio del propio `.cfg`, así que la instalación se puede mover. **Guardián**: el montaje extrae el ZIP a un temporal FUERA del repo y compila `ExcCatchTest`; si falla, borra el ZIP. Verificado que sabe fallar. Encontrado por Eduardo («¿algún problema con el módulo Core?») |
| 19 | b1 S3 | `BoardTest` imprime **`variant=B`** en un ESP32-S3. Los otros dos números son correctos (`gpioCount=45`, `GPIO_COUNT=45`), lo que lo empeora: **el dato falso viaja envuelto en dos verdaderos**. Sale de `bpstdlib/Pico.bp:92`, que **no es intrínseco**: cinco líneas de BP puro, `if gpioCount() >= 40 then "B" else "A"` — el umbral separa RP2350A (30 GPIO) de RP2350B (48), y los 45 del S3 caen del lado del 48. Visto por Eduardo en el log de la tanda 7 | 🟡 **AL REGISTRO, sin tocar código — freeze.** **El reencuadre es de Eduardo y cambia el diagnóstico**: `Pico.bp` **es la librería de la familia RP2350**, no una capa común; `variant()` hace lo que documenta, y lo que falla es que **se está usando una librería de un micro en otra familia porque para ésa no hay ninguna**. Alcance real: no está en el manual (sólo en `docs/H7_TASKS.md`) y el **único llamante de todo el árbol es `samples/BoardTest.bp`**. Coste de tocarlo hoy: `Pico.mod` va **embebido** en los firmwares (`MODS=(… Pico …)` en los tres `regen_*_mods.sh`) ⇒ regenerar la stdlib + los 3 `*_mods.c` + las 5 imágenes + reflashear + repetir tandas ya verdes. → **V5: librería de placa genérica** (`docs/V5_IDEAS.md`), con el dato puesto por el micro y la librería haciendo de puente |
| 20 | — (paquete) | **El widget `Chart` es lo NUEVO de V4 en gráficos (H7, #317) y se publica sin demo.** El `ChartDemo.bp` existe y está escrito —caso realista, ventana deslizante de muestras de sensor con dos series— pero vive en `bpgenvm-c/samples/`, que es la carpeta de desarrollo de la VM-C y **no viaja al ZIP** (el copiado es `git ls-files 'samples/*.bp'`). Resultado: la stdlib expone `Gui.Chart` con sus seis intrínsecos y **no hay un solo `.bp` publicado que lo use** — verificado con `grep -rln Chart --include=*.bp`, que en todo el árbol sólo da `bpstdlib/Gui.bp` y ese fichero fuera de sitio. Encontrado al montar la escalera de GUI del P4 | 🟡 **AL REGISTRO, decisión al cerrar la fila.** No toca firmware: es **mover un fichero** y rehacer el ZIP, que ya hay que rehacerlo por los prints del `paralleltest`. Antes de moverlo hay que **compilarlo y correrlo** —lleva desde H7 sin tocarse y el árbol ha cambiado mucho—, y si va, entra en la escalera de GUI como peldaño propio. Mismo patrón que el hallazgo 5: un sample que no llega al usuario es una función que, para él, no existe |
| 35 | — (paquete) | 🔴 **El AOT —el hito grande de V4— se publica SIN UN SOLO EJEMPLO.** Visto por Eduardo al correr el bench en la c1: *«el Bench no está en las samples del zip pero sí donde siempre»*. Verificado: `scripts/montar-zip.sh:114` hace `case "${f#samples/}" in */*) continue ;; esac`, o sea **descarta todo lo que viva en un subdirectorio** de `samples/`, y luego copia a mano sólo tres proyectos (`sampleproject`, `formdemo`, `imageproject`) más `errores/`. Resultado: **`samples/benchmarks/` entero —8 ficheros— se queda fuera**, y con él `Bench.bp`, que es el que enseña `native` + el gemelo interpretado para medir la ganancia en la misma placa. **Comprobado que el agujero es total**: `grep` de `native function` sobre los samples de la RAÍZ —los únicos que viajan— da **cero**; los cinco que lo usan (`Bench` `intbench` `floatbench` `arraybench` `fibobench` `sortbench`, más `aottest/`) están todos en subcarpetas. O sea que **un usuario que instala V4 lee `native` en el manual y no tiene un solo `.bp` que ejecutar para verlo** | 🟡 **AL REGISTRO → al lote del ZIP.** Es **hallazgo 20 otra vez y peor**: allí era un widget sin demo (`ChartDemo`), aquí es **H4 entero** —113× en RISC-V, 116× en el P4, **99× medido hoy en la Nucleo**— sin nada que enseñar. **No toca firmware**, así que no choca con la regla del 5-ago: entra en el mismo resellado de ZIP que el 20 y el 25. Y el arreglo debería ser **de mecanismo, no de lista**: hoy publicar un sample nuevo depende de acordarse de añadirlo a mano al script, y ya se ha escapado dos veces. Antes de moverlos hay que **compilarlos y correrlos** —igual que se decidió con `ChartDemo`— porque llevan tiempo sin tocarse |
| 34 | c1 STM32 | ✅ **`paralleltest_sugar` muere con `exit 11 (RUNTIME_ERROR)` en la Nucleo — y sin decir por qué.** El programa arranca, imprime `[main] arrancando 2 ramas + default` y `[default] localDefault = 12345`, y **muere antes de que t1 y t2 impriman su resultado**: o sea que **el `default` corre y las dos ramas `case` no**. Lo que ya está comprobado:

- **En el host pasa.** Compilado y ejecutado con `--mem=786432` (el mismo bloque de VM que la placa ⇒ mismo heap de 372 KB): salida completa y `status=OK`, con `sumaRango` dando `1250025000` y `-544942296` (el segundo desborda `integer`, y es correcto que desborde).
- **En el RP2350 pasa.** Verde en la **tanda 4b de la a1**, donde este sample y `paralleltest` entraron precisamente porque faltaban de la lista.
- Es la **primera vez que la fila c corre la tanda 4**, así que **NO hay línea base con `-O0`** — y por tanto **no se puede atribuir al hallazgo 33 todavía**. Puede ser un hueco de la familia STM32 que nadie había mirado, o puede ser que optimizar destapara algo. **Las dos hipótesis siguen vivas y decirlo importa.**

⚠️ **Y hay un segundo defecto encima del primero: el error no dice nada.** La consola da `exit 11` a secas, sin mensaje. Un `RuntimeError` en BP lleva su texto (#280 lo propagó hasta el wire y el IDE), así que o se está perdiendo por el camino en esta familia, o el fallo no es una excepción de BP sino algo que mata la ejecución por debajo. Es la doctrina de la casa incumplida —**un instrumento mudo**— y ahora mismo es lo que bloquea el diagnóstico | ✅ **RESUELTO EN 10 MINUTOS Y SIN TOCAR NADA: es el hallazgo 24 otra vez.** Eduardo lo repitió —**falló las dos veces**, o sea que dentro de un estado dado es determinista y no es una carrera— **hizo un reset y desapareció**: salida completa, `exit 0`, y los dos números **idénticos a los del host** (`1250025000` / `-544942296`). O sea que **cuando corre, corre bien**; lo que falla es el estado acumulado, no el programa.

**Y esta segunda vista vale más que la primera, porque descarta al sospechoso principal.** El hallazgo 24 se vio en el P4 **con LVGL de por medio**, y la sospecha natural era la pila gráfica. Aquí no hay pantalla, no hay LVGL y no hay un solo píxel: es la **familia STM32 y el camino de threads**. Misma firma —*se degrada de un RUN al siguiente hasta que rompe, y el reset lo cura*— en **dos familias, dos subsistemas sin nada en común**. Luego lo que se degrada está en la **VM/firmware**, no en los gráficos. Eso estrecha muchísimo la búsqueda de V5.

🟡 **NO bloquea V4 — se fusiona con el 24 y va a V5, pero con la gravedad REVISADA.** Eduardo, al cuantificar el daño: *«pueden aparecer al cabo de 2 días de ejecución sin resetear, mortal para una máquina de estados sin atención humana… inofensivo en pruebas, mortal en explotación»*. El criterio del 24 —*encadenar RUN es actividad de desarrollo*— **sólo protege si lo que se degrada se degrada POR RUN**; si fuera **por operación**, un programa que corre para siempre lo alcanza igual, sólo que más tarde. **Hoy no sabemos cuál de las dos es**, así que la gravedad queda **condicional** y el detalle está en `docs/V5_IDEAS.md`. A favor de la lectura tranquila: **#357 dejó 10.000 vueltas en UN solo programa** con el heap en una banda de ~130 B, «no hay techo» — el escenario de explotación en pequeño, y salió limpio, aplicando el criterio que ya fijó Eduardo allí (*«de los programas no se sale nunca; encadenar RUN es actividad de desarrollo»*) y la regla del 5-ago: el reset lo cura, los resultados son correctos, no se pierde nada ⇒ **no es «terrible», no se toca el firmware**. Queda escrito por dónde se tira cuando toque: el guardián de fin de RUN de **#339** dice *quién* se queda la memoria, y aquí habría hecho falta —lo que estorbó fue el **hallazgo 30**, que el INFO del STM32 no dice el heap, o sea justo el instrumento que hacía falta.

📌 **Lo que sí queda vivo del 34 es el instrumento mudo**: `exit 11` sin mensaje. Sigue sin saberse si el `RuntimeError` pierde su texto en esta familia o si lo que mata no es una excepción de BP. Se anota **junto al 24** para V5: el día que se persiga la degradación, **el mensaje es la primera pista y hoy no la hay** |
| 33 | c1/c2 STM32 | ✅✅ **El firmware STM32 que publicamos está compilado SIN OPTIMIZAR — `-O0`, y eso incluye el intérprete de la VM.** Salió de una medida de Eduardo, no de una sospecha: `paralleltest` en la Nucleo da **207.017 ms** contra 33.079 del P4 y 129.262 del S3, y su lectura fue *«a simple vista esta CPU es más lenta»*. Normalizando por reloj —4M de iteraciones interpretadas, un solo núcleo en las tres— la Nucleo sale **peor por ciclo que el S3**, que va a 240 MHz y ya era el lento del parque:

| placa | reloj | tiempo | ciclos/iteración |
|---|--:|--:|--:|
| ESP32-P4 (RISC-V) | 360 MHz | 33.064 ms | **2.976** |
| ESP32-S3 (Xtensa) | 240 MHz | 129.262 ms | 7.756 |
| **STM32U575 (Cortex-M33)** | **160 MHz** | **207.017 ms** | **8.281** |

**La medida del P4 está REPETIDA, y ese es el control que hacía falta**: Eduardo la volvió a correr el 5-ago y dio **33.064 ms** contra los **33.079** del 4-ago — **15 ms de diferencia en 33 segundos, un 0,05 %**. Otro día, otra sesión, misma imagen. Así que el 6,3× contra la Nucleo no es ruido de una pasada suelta ni un mal momento de la placa: el instrumento repite, y lo que mide es real. (El reloj exacto del P4 mueve un poco la tercera columna —a 350 MHz saldrían 2.894 ciclos— pero no mueve la conclusión.)

Eso no cuadra con el silicio: un M33 con flash interna y caché no tiene por qué ir **peor por ciclo** que un Xtensa. Y la causa está en el build, verificada leyendo los flags reales que usa cada familia, no la documentación:

| familia | configuración | optimización |
|---|---|---|
| RP2350 | CMake **Release** (`CMakeCache.txt`) | `-g -O3 -DNDEBUG` |
| ESP32 | IDF `CONFIG_COMPILER_OPTIMIZATION_SIZE=y` | `-Os` |
| **STM32** | CubeIDE **Debug** (la única que existe) | **`-O0 -g3`** |

Y no es que se escape sólo la capa vendor: el `-O0` está en **`Debug/src/subdir.mk`**, que es donde se compilan los 24 ficheros del core heredado — o sea **`interp.c`, el bucle de despacho**. En `Nucleo_u575b/` sólo hay carpeta `Debug/`, con el `.bin` del 5-ago: nunca se ha construido otra cosa. El `.cproject` sí trae una config con `-Os`, pero **jamás se ha usado**. Nadie eligió esto: es el valor por defecto de CubeIDE para Debug, y arrastramos la config con la que nació el port en H9 | 🔴 **DECISIÓN DE EDUARDO — es el mismo animal que el 31**: no es una mejora que se nos ocurra ahora, es **un descuido de configuración**; publicar el build de depuración no es una decisión de diseño que nadie tomara. **Arreglo propuesto: cambiar el nivel de optimización DENTRO de la config Debug** (a `-Os`, la misma que ESP32), **no** cambiarse a la config Release — porque los include paths, las carpetas enlazadas y el script de enlace sólo se han mantenido en Debug y la otra config podría diferir en cualquier cosa. Un ajuste, todo lo demás probado. `-g3` se queda: la info de depuración no ocupa flash. **Riesgo**: el core nunca se ha compilado optimizado *para este target*, y optimizar destapa UB latente (aliasing, `volatile` que falta) — pero **los mismos fuentes ya se compilan a `-O3` en RP2350 y a `-Os` en ESP32**, así que el riesgo real se concentra en nuestros ~6 ficheros de la capa STM32 y en la HAL de ST, que el mundo entero compila optimizada. **Coste**: reconstruir (~7 s headless) + reflashear + repetir las tandas 1-3 de la c1; la c2 aún no ha empezado. — ✅ **HECHO (5-ago), autorizado por Eduardo**: *«es un riesgo pequeño que siempre podemos revertir»*. Cambiado el nivel de optimización a `-Os` **dentro de la config Debug** en las **dos** placas de la familia (`Nucleo_u575b/.cproject` y `Discovery_u5g9j/.cproject`), una sola opción por proyecto y nada más tocado. Reconstruidas headless: **0 errores** las dos, y el único aviso es un `bpvm_resolve_handler defined but not used` que ya estaba antes — o sea que **optimizar no ha destapado nada**. Verificado en los flags reales del build que el `-Os` llega al core (`interp.c` incluido) y no sólo a la capa vendor.

| imagen | antes (`-O0`) | ahora (`-Os`) | |
|---|--:|--:|--:|
| `bpvm_stm32_nucleo.bin` | 384.144 B | **229.292 B** | **−40 %** |
| `bpvm_stm32_dk2.bin` | 1.243.092 B | **869.608 B** | **−30 %** |

El `bss` se queda en **645.536** contra los 645.544 de antes: el bloque de VM de 512 KB del hallazgo 31 sigue entero, que era lo que había que comprobar de paso. Las dos imágenes publicadas en `dist/firmware/` con `SHA256SUMS.txt` rehecho (`01f8d2b5` nucleo · `49c39824` dk2). **Sin tocar el ZIP** — el firmware no viaja dentro. ✅✅ **VERIFICADO EN PLACA (5-ago)**: `paralleltest` pasa de **207.017 ms a 56.781 ms** — **3,65× más rápido** con el mismo silicio, el mismo reloj y el mismo binario de BP. Sólo cambió cómo se compiló la VM. (Yo esperaba «entre 2× y 4×»; cayó dentro, pero el número lo puso la placa.)

**Y el parque entero cambia de orden — la Nucleo pasa de última a PRIMERA por ciclo:**

| placa | reloj | tiempo | ciclos/iteración |
|---|--:|--:|--:|
| **STM32U575 `-Os`** | 160 MHz | **56.781 ms** | **2.271** ← la mejor |
| ESP32-P4 (RISC-V) | 360 MHz | 33.064 ms | 2.976 |
| ESP32-S3 (Xtensa) | 240 MHz | 129.262 ms | 7.756 |
| ~~STM32U575 `-O0`~~ | 160 MHz | ~~207.017 ms~~ | ~~8.281~~ |

**La Nucleo nunca fue lenta: cargaba el `-O0`.** Con 2,25× menos reloj que el P4 sólo tarda 1,72× más, y por ciclo le saca un 31 %. La lección de método es la de siempre en esta casa: **el instrumento estaba mintiendo y la lectura «esta CPU es más lenta» era correcta como observación y falsa como diagnóstico** — se salvó porque el número no cuadraba con el silicio y se fue a mirar el build en vez de aceptarlo. ⏳ Queda repetir las tandas 1-3 de la c1 sobre la imagen nueva.

**❓ «¿Y los nativos que compilamos, están optimizados?» (Eduardo, 5-ago) — SÍ, y siempre lo estuvieron.** Comprobado en las dos rutas que generan `.mdn`: `AotBuild.java:54` (target `arm`, Cortex-M33 = RP2350 **y STM32**) y `:64` (target `riscv`, P4) llevan **`-Os`** las dos, y `pico/build_mdn.sh:81` usa exactamente los mismos flags —tienen que casar o el `.mdn` no es cargable, y el comentario del código lo dice—. O sea que el `-O0` era **sólo del build del firmware STM32**, no del AOT.

Y de ahí sale una consecuencia con suerte: **en el STM32, el código AOT era lo ÚNICO optimizado de la placa**. Si la tanda de AOT de la c1 se hubiera corrido antes de hoy, la ganancia habría salido **inflada** —no porque el nativo fuera más rápido, sino porque el intérprete contra el que se compara iba lastrado 3,65×—. La tanda estaba pendiente, así que la medida que se tome ahora será honesta. En la a1/a2 y en el P4 no hubo distorsión: sus firmwares ya iban a `-O3` y `-Os` |
| 32 | c1 STM32 | **El INFO dice `GPIO: 114` y `Pico.gpioCount()` dice `128` — en la misma placa.** Visto por Eduardo comparando el panel del IDE con la salida de `PicoInfo`. **Son dos fuentes distintas para el mismo dato**: el INFO lo lleva **escrito a mano en una cadena** (`stm32_repl.c:132`, `"gpioCount":114` — las I/O del encapsulado LQFP144) y el intrínseco va al **backend** (`gpio_stm32.c:140`, `STM32_PIN_COUNT` = 128 = 8 puertos × 16, el espacio de direccionamiento). Cada número es defendible por separado, pero **no pueden convivir**: el panel dice una cosa y el lenguaje otra. Y el propio código ya sabía del problema — el comentario de `gpio_stm32.c:144` cuenta que **`ADC_CHANNELS` y `PWM_SLICES` tuvieron EXACTAMENTE este bug** y se arreglaron con callbacks board-aware porque *«el INFO ya daba 20/28 desde un string aparte»*: **se arreglaron dos de tres y el GPIO se quedó** | 🟡 **AL REGISTRO → al lote.** Es la doctrina de la casa incumplida —**una función, una fuente**— la misma de #299 (layout de clase) y #315 (slots de vtable). El arreglo es que el INFO pregunte al backend en vez de llevar el número horneado, como ya hacen ADC y PWM: **una familia, una imagen**. ⚠️ **Y antes hay que DECIDIR cuál es el número bueno**, que no es obvio: 114 es lo que el usuario puede pinchar, 128 es lo que el driver direcciona. Yo publicaría **114** —el usuario cuenta pines, no registros— y dejaría 128 como límite interno de validación. **Barrido pendiente**: comprobar si en las otras dos familias el INFO también hornea algún número que el backend ya sabe.

🔻 **AMPLIACIÓN (5-ago, al arrancar la c2) — es peor de lo que parecía: NO es sólo el GPIO, y NO es sólo el INFO.** Los tres contadores de periféricos del backend son **constantes horneadas del U575**, y el propio código lo dice: `gpio_stm32.c:139` devuelve `STM32_PIN_COUNT` *«128: puertos A..H **del U575**»*, `:147` devuelve `20` *«ADC1 14-bit… **del STM32U575**»* y `:151` devuelve `28` *«coincide con el INFO»*. El comentario de encima los llama **«board-aware»** y **no lo son**: no miran `BPVM_BOARD_DK2` para nada, aunque `board.h:21` distingue perfectamente las dos placas. Resultado: **la Discovery, que es un STM32U5G9 y no un U575, reporta los números del U575** — 114 GPIO / 28 PWM / 20 ADC, idénticos a los de la Nucleo. O sea que el arreglo no es «que el INFO pregunte al backend»: **es que el backend pregunte a la PLACA**, que es justo lo que ya se hizo para `boardName` (`:175`, sale de `board.h`) y no para éstos. 🟡 Sigue **al lote**, sin subir de prioridad: son datos informativos y límites de validación, no corrupción — pero la fila del hallazgo queda corregida, porque «el INFO miente» se quedaba corto |
| 31 | c1 STM32 | **La placa con el heap MÁS PEQUEÑO del parque es la que más RAM libre tiene: ~520 KB parados.** Pregunta de Eduardo al ver el INFO —*«partimos de 768K de RAM, ¿dónde se ha ido la memoria?»*— y la respuesta medida sobre el ELF es que **no se ha ido a ninguna parte**. El linker ve **768 K contiguos** en `0x20000000` (no hay fragmentación de bancos), y el estático total (`bss`+`data`) son **≈247 KB**: `s_vm_mem` **128** · `g_scratch` 16 · `s_snap_names` 12 · `s_put_buf` 12 · `s_env_a`+`s_env_b` 16 · `s_region` 8 · buffers de littlefs 13 · resto ~42. **Sobran ~520 KB.** El bloque de 128 KB se fijó cuando el port era nuevo y nadie volvió a mirarlo; como la regla de reparto es común, de ahí salen los **64/64**. Contraste que lo hace sangrante: en el S3 se peleó KB a KB para subir el bloque de 128 a 160 (#336) **con medidas en la mano**, porque de 512 KB sólo llegaban ~312 libres y el IDF se comía otros 84 en marcha | ✅ **ARREGLADO EN V4** (5-ago) — **y la decisión la corrigió Eduardo**. Yo apliqué la regla del freeze mecánicamente («es mejora, no corrección») y él la rebatió con lo que la regla protege de verdad: *«esto es más bien un descuido nuestro, es sencillo de arreglar y estoy al principio de las pruebas, no me cuesta nada volver a empezar»*. **El coste de repetir era casi cero justo en ese momento** — sólo llevaba la tanda 1 — y un bloque que nadie revisó desde que nació el port no es una decisión de diseño. `stm32_repl.c:66`: **128 → 512 KB** ⇒ la regla común reparte **heap 384 + pila 128**, seis veces el heap anterior y el segundo del parque tras el P4. Verificado al enlazar: `bss` 252.328 → **645.544** (exactamente +384 KB), estático total ~631 KB de 768, **~137 KB libres** con el linker exigiendo sólo 20. Imagen nueva sellada (`3ee901c2`), **sin tocar el ZIP** — el firmware no viaja dentro. ✅ **Reverificado en placa** (5-ago): tanda 2 verde y `MemInfo` mide **372 KB de heap** (mayor bloque 244), los mismos tres números que el host con `--mem=786432`. ~~NO SE TOCA EN V4 → V5. Subir el bloque es mejora, no corrección, y además obligaría a repetir la tanda de memoria de las dos placas de la familia.~~ Pero la medida ya está hecha y es lo único que hacía falta para decidirlo: hay margen para **multiplicar el heap por 4** sin acercarse al límite. Emparenta con el 30 (el INFO no dice el heap) y con el 27 (el tamaño lo pone una constante y no la placa): **la misma familia — un recurso decidido en tiempo de compilación que nadie vuelve a mirar** |
| 30 | c1/c2 STM32 | **El INFO del STM32 no trae la línea de la VM** (`heap … + stack …`), y las otras dos familias sí. Verificado en el código: el campo `vmHeapBytes` lo mandan `pico/repl_v1.c:737` y `esp32/main/repl_esp32.c:224`, y **el STM32 no lo manda**. No es mentira, es una **omisión** — pero cuesta justo donde duele: el panel de INFO es el instrumento con el que se lee la tanda de memoria, y en esta familia hay que ir a buscar el número al fuente. El dato real: `stm32_repl.c:66` reserva **128 KB** de bloque de VM y `:473` reparte con la regla común ⇒ **64 KB de heap + 64 KB de pila**, el heap **más pequeño del parque** (S3 96 · Pico 257 · Metro/P4 en MB) | 🟡 **AL REGISTRO → al lote.** Es el más barato de los de firmware: **una familia, una imagen** —no las cinco— y es añadir un campo que ya existe en las otras dos, con el valor a mano en el mismo fichero. Mientras tanto queda escrito aquí el número, que es lo que hacía falta para leer la fila c |
| 29 | c1 STM32 | **El error de alineación de particiones no dice A QUÉ hay que alinear.** Al cambiar el tamaño del FS en la Nucleo: `VM error [INVALID_PARAM]: un tamaño no está alineado al sector de borrado (partición 0: fs)`. La validación es **correcta** —una partición que no cae en frontera de página haría que borrar una arrastrase parte de la vecina— pero el mensaje **no dice cuál es el sector ni cuál sería el valor válido más cercano**. Y aquí es donde más falta hace: el **STM32U5 borra en páginas de 8 KB** (`flash_layout_stm32.h:28`, `BP_ENV_SECTOR 0x2000`) mientras que **ESP32 y RP2350 usan 4 KB**. O sea que **quien venga de probar las otras dos familias mete un número que allí valía y aquí no**, y el mensaje no le da la pista. Visto por Eduardo: *«ni idea de lo que significa este mensaje»* — y lo dice el autor del lenguaje | 🟡 **AL REGISTRO → al lote.** Misma familia que el 13(a), el 21 y el 22: **dice la verdad y no sirve**. El arreglo es meter el sector y el valor sugerido en el texto ⇒ **código común ⇒ las 5 imágenes**, así que entra en el mismo reflasheo que el 13(a) y el 21(b). ⏩ **Mientras tanto**: en STM32, múltiplos de **8 KB** (96, 104, 112, 128, 256…); y si sólo se quiere que funcione, **pedir los valores por defecto** — `bpvm_part_defaults()` alinea a la baja al sector, así que su propuesta es válida en cualquier familia |
| 28 | — (paquete) | 🔴🔴 **EN UN DISPOSITIVO VIRGEN NO ARRANCA NINGUNA DEMO GRÁFICA: el IDE no sube `Json`, del que depende `Gui`.** Visto por Eduardo en la Waveshare recién particionada, y diagnosticado bien de un vistazo: *«parece que el IDE no está leyendo el árbol entero de dependencias, se queda con la lista del principal»*. **La causa es peor que eso**: la resolución transitiva **SÍ está implementada** —hay un BFS en `resolveDeviceDeps` puesto justo por este caso, y el comentario del código lo dice— pero descubre los imports de cada módulo **leyendo su `.bp`**: `dir.resolve(imp + ".bp")` → `parseImports`. Y **el ZIP lleva 26 `.mod` de `bpstdlib/` y CERO `.bp`**. Sin el fuente de `Gui`, `Json` no se encola, no se sube, y la app muere. **La recursión funciona en el repo y está MUERTA en la instalación.** Rodeo que encontró Eduardo: ejecutar `JsonDemo` primero deja `Json.mod` en `/lib` y a partir de ahí las gráficas van | ✅✅ **CERRADO Y VERIFICADO EN PLACA** (4-ago, ZIP `a5b28fef`). Eduardo lo probó **en el estado exacto en que apareció**: FS de la Waveshare formateado, `GuiDemo` directo y sin `JsonDemo` delante. **Suben los DOS módulos, `Gui` y `Json`, y el programa funciona** — antes sólo salía `Gui.mod`. Ésa es la señal, no que arranque. El ZIP lleva ahora **26 `.bp` de `bpstdlib/`** además de los 26 `.mod`, y `Gui.bp` delata su `import Json` en la línea 14, que es lo que el BFS necesita leer. **Y hay guardián nuevo**: el montaje comprueba el invariante *sobre el PAQUETE MONTADO* (el que ya había miraba el repo, que es donde nunca falló). Verificado que **sabe fallar**: con un `.mod` sin su `.bp` aborta y no sella. ⏩ **Cómo verificarlo en placa, en el estado exacto en que apareció**: formatear el FS de la Waveshare y ejecutar `GuiDemo` **directamente**, sin `JsonDemo` delante. 🔴 **CRÍTICO — bloqueaba publicación, y es el hallazgo más importante del día.** Pega a **un usuario nuevo con una placa nueva** justo en lo que más vende V4: la GUI. **Por qué no salió antes — y NO es lo que parecía.** Mi primera lectura fue «nuestras placas estaban sucias»; **falso, y lo corrigió Eduardo**: la mayoría se probaron **vírgenes**, y él además **cambió el tamaño de la partición a propósito para forzar que se recargara la stdlib**. Lo que tapaba el bug era **el ORDEN de la batería**: `JsonDemo` corría antes que las tandas gráficas y dejaba `Json.mod` en `/lib`, así que cuando llegaban las demos de GUI el terreno ya estaba abonado. **No lo escondía el estado de la placa: lo escondía una prueba anterior.** Salió en la b3 sólo porque ahí se saltaron pruebas. **Lección, y generaliza mal:** en una batería que comparte dispositivo **cada prueba deja estado para la siguiente**, así que el orden puede esconder fallos en cualquier punto — no basta con partir de una placa limpia si la prueba nº 3 prepara el terreno de la nº 9. Criterio de Eduardo al respecto: *«tampoco hay que descartar que se nos haya pasado algún otro problema. Pero al menos lo más común lo tenemos probado.»* ⏩ **El arreglo es de EMPAQUETADO, no de código: llevar `bpstdlib/*.bp` en el ZIP** — una línea en `montar-zip.sh`, cero riesgo, y de paso el usuario puede leer la fuente de la stdlib. Encaja con el invariante que el guardián del hallazgo 15 ya exige **en el repo** («un `.mod` de la stdlib tiene su `.bp` al lado»): esto lo hace cierto también en la instalación. Alternativa más robusta pero que es código, y por tanto V5: **leer los imports del `.mod`**, que desde H6.a es autodescriptivo. **Tercera vez hoy del mismo meta-patrón** (6 `BpVM.cfg`, 25 `testimg.png`, 28): *funciona en el repo, roto desde el ZIP*. Y el guardián del montaje **compila `ExcCatchTest`, que no importa `Gui`**, así que no lo caza — el guardián necesitaba un caso con GUI |
| 27 | b3 P4 Waveshare | **La Waveshare tiene 32 MB de flash y sólo se pueden usar ~16: el límite lo pone la IMAGEN, no la placa.** El INFO dice `Flash: 32.0 MB` —o sea que el chip se detecta bien— pero la zona de datos sale de **`bpdata` de la tabla del IDF**, y el firmware se construye con `partitions.csv` (perfil de 16 MB): `bpdata = 0x9E8000` ≈ **10,16 MB**, que repartido al 50 % da los **5,0 MB de FS** que muestra el INFO. Cuadra exacto. En el ESP32 nuestro gestor de particiones **no mira el flash del chip a propósito** — `board_mgr_esp32.c:235`, *«la tabla vendor ES el límite; sin clamp #292»*—, así que repartir desde el IDE reparte dentro de esos 10 MB y no puede pasar de ahí. Existe ya `partitions_32m.csv` (`bpdata = 0x19E8000` ≈ **26,16 MB**), pero usarlo son **dos imágenes del P4**. Criterio de Eduardo: *«el tamaño de la flash es lo que dice la placa o si prefieres el IDF»* | 🟡 **DECIDIDO NO TOCAR EN V4 → V5.** Se deja la imagen única (10 MB de datos en las dos P4) y se documenta: un segundo binario del P4 en pleno freeze añade superficie de sellado, prueba y documentación justo al cerrar, y 10 MB sobran para lo que V4 hace. **La dirección para V5 tiene una simetría bonita con #292**: aquel puso un **clamp** para cuando la tabla promete MÁS flash del que hay; aquí hace falta lo contrario — **declarar la tabla grande (32 MB) y dejar que el clamp existente la recorte** en las placas de 16. Eso daría **una sola imagen que se adapta hacia abajo**, que es justo el principio de Eduardo. ⚠️ **Pregunta abierta que decide el diseño**: si el bootloader del IDF acepta arrancar con una tabla que se sale del chip físico. Si no lo acepta, la alternativa es que nuestra zona **empiece** en `bpdata` y **acabe en el fin del flash real** (en la tabla de 16 MB ya acaba justo ahí: `0x618000 + 0x9E8000 = 0x1000000`), lo que exigiría acceso crudo `esp_flash_*` para el excedente en vez de `esp_partition_*`. **Cuarta aparición del mismo patrón hoy** (19, 21, 22, 27): un dato que debería venir del hardware y viene de una suposición de compilación |
| 26 | — (docs) | **`docs/INSTALAR_FIRMWARE.md` acumula CUATRO cosas caducadas, y es el documento que un usuario nuevo lee el primero.** (1) **El panel del P4**: dice que sale de `/sys/board.json` y manda ejecutar `samples/SetDisplay.bp` tras flashear una placa que no sea la EV (líneas 96-99) — **ya no**: sale del ENV (#311), y ese paso sobra. Corregido por Eduardo, 4-ago. (2) **El fichero del P4 no existe**: llama a la imagen `bpvm_esp32p4_generic.bin` y la sellada es **`bpvm_esp32p4_merged.bin`**. (3) **La sintaxis de esptool es de la v4**: `esptool.py`, `write_flash`, `merge_bin`, `--flash_mode/--flash_freq/--flash_size` — con el IDF v6 es **esptool v5**, que quiere `esptool` y guiones (`write-flash`, `merge-bin`, `--flash-size`). (4) **`--flash_size 2MB`** en el ejemplo de fusión del S3, cuando el build real usa **16MB** (`flash_args`) | ✅ **ARREGLADO** (4-ago, ZIP `a5b28fef`): las cuatro corregidas y selladas — el panel por ENV (fuera `board.json` y fuera `SetDisplay`), `bpvm_esp32p4_merged.bin`, sintaxis esptool v5 (`esptool` + subcomandos con guión) y `--flash-size 16MB` en el ejemplo del S3. 🟡 *Queda como sugerencia*: releer el resto del documento con el mismo criterio, porque cuatro en una página sugiere que no se ha revisado desde que cambiaron las cosas. ~~AL REGISTRO, y es el hallazgo MÁS BARATO de todos: sólo texto.~~ Cero código, cero firmware; entra en el mismo rebuild del ZIP que ya hay que hacer. Y es de los que más daño hacen por su coste: quien siga (2) busca un fichero que no está, y quien siga (3) se come un error de sintaxis en el primer comando que teclea. Verificar de paso el resto del documento con el mismo criterio, porque cuatro en una página sugiere que no se ha releído desde que cambiaron las cosas |
| 25 | — (paquete) | **`samples/GuiImageDemo.bp` se publica y NO puede funcionar desde la instalación.** El sample hace `loadFile("testimg.png")`, y **el `.png` suelto no viaja en el ZIP**: el copiado de `samples/` se lleva sólo los `.bp` (`git ls-files 'samples/*.bp'`), más los árboles de proyecto completos. Así que falla **en placa** (nadie sube el asset) **y en el PC desde el ZIP** (no está el fichero al lado). En el repo funciona porque ahí sí está `samples/testimg.png` — el mismo patrón que el hallazgo 6 con `BpVM.cfg`. Al usuario le sale `cargada = false`, `dims = 0x0` y la pantalla en blanco. **El camino bueno existe y sí viaja entero**: `samples/imageproject/` (`.bpbuild` + `src/` + `resources/testimg.png`, 6.321 B), y su cabecera lo explica — el suelto es la versión de PC, el proyecto la de placa. *Salió porque puse el suelto en la escalera de GUI del P4: error mío al montar la lista, no del producto* | 🟡 **AL REGISTRO.** No es código: o **se empaqueta `samples/testimg.png`** (una línea en `montar-zip.sh`, y entonces el suelto vale en PC desde la instalación), o **la cabecera del sample dice** que en placa —y desde el ZIP— hay que usar `imageproject`. La segunda es más honesta: un sample suelto no puede subir recursos a un device **por diseño**, así que ningún empaquetado lo arregla ahí. Escalera corregida: el peldaño de imagen es **`imageproject`**, no `GuiImageDemo` |
| 24 | b2 P4 | 🔴 **Tras muchas ejecuciones seguidas, la pantalla deja de pintar — y el reset lo cura.** `GuiFontDemo` salió **totalmente negra** (las demás demos dejan fondo blanco). **El modelo estaba PERFECTO**: el volcado enseñaba los 4 labels con `font=12/20/32/48` y sus posiciones, y la consola imprimió todo hasta el final. Tras un **reset de la placa**, el mismo `.mod` sin recompilar **se ve**. Eso descarta el sample, descarta `fontSize` y descarta las fuentes (las cuatro Montserrat que usa están activadas en `include/lv_conf.h`). Lo que queda: **algo se degrada de un RUN al siguiente hasta que deja de pintar**. Lo paró Eduardo antes de que yo siguiera buscando — *«a ver si reseteando se arregla, llevo muchas demos seguidas sin un reset»*—, que es la regla de quitar estado antes de teorizar, y acertó | ✅ **DECIDIDO — NO bloquea, se documenta → V5.** Criterio de Eduardo, y es de dominio: *«si la pantalla deja de pintar porque has salido del programa… esto no es un SO con un prompt, de los programas no se sale nunca. Nosotros lo hacemos porque estamos haciendo pruebas y el programador lo hará porque estará desarrollando pantallas, pero si después de 15 salidas se le cuelga no es ningún problema hacer un reset.»* **En un micro la placa arranca UN programa y lo corre para siempre**: encadenar RUN es una actividad de desarrollo, no de funcionamiento, así que el fallo no vive en el modo de operación del producto. ⚠️ **La condición bajo la que habría que revisarlo**, para quien lea esto dentro de un año: el razonamiento vale **porque la degradación es por RUN**, que es lo único medido. Si apareciera que se acumula por algo que un programa largo hace **en marcha** —crear y destruir ventanas, por ejemplo—, sí llegaría al usuario final. Hoy no hay ni un indicio de eso. ~~ABIERTO, y hay que ponerle número antes de decidir.~~ Precedente que se parece demasiado: **#352**, *«el modelo del GUI es de la VM: se rompía a los 30 RUN»*, cerrado en host — esto huele a su gemelo **en placa**. **NO confirmado**: con un solo dato no sé si es LVGL, el modelo de la VM o el driver de display. **Medida acordada, y sale gratis**: contar los RUN desde el último reset mientras se termina la escalera. **Si vuelve a los 5-10 → es serio para publicar** (cualquiera que desarrolle GUI hace run-retoca-run y se lo come a diario); **si aguanta 30+ como el #352 → se documenta el rodeo y a V5**. Eduardo apunta además que *«esto nos pasó en demos antiguas»*: si aquello también fue tras muchas ejecuciones, lleva tiempo latente. ⚠️ **Cambia cómo se leen los peldaños verdes**: se corrieron en cadena sin reset, así que lo que demuestran es **«funciona en fresco»** |
| 23 | b2 P4 | **La tecla ✓ del teclado en pantalla no hace nada.** Visto por Eduardo en `GuiListKbd`: *«el teclado virtual funciona y se pinta en el textbox, pero el V no sé si hace algo»*. No hace nada, y **no puede**: en LVGL el ✓ dispara `LV_EVENT_READY` (y el ✗, `LV_EVENT_CANCEL`), y la fachada registra **sólo dos** eventos — `gui.c:1012-1014`, `LV_EVENT_VALUE_CHANGED` y `LV_EVENT_CLICKED`. La clase `Keyboard` (`Gui.bp:569`) tiene constructor y `attach()`, nada más. O sea que el ✓ lo **dibuja LVGL** porque viene en su mapa de teclas por defecto, y por debajo no hay quien lo escuche | 🟡 **AL REGISTRO → V5.** **Más suave que el 21 y el 22**, y conviene no meterlo en el mismo saco: aquí **no se informa nada falso**. Lo que el módulo promete —*«las teclas editan ese campo, que dispara su onChange»*— **funciona**; el ✓ es un botón inerte, no un dato mentiroso. Y el arreglo es **añadir un evento** (`onReady`/`onAccept`) ⇒ fachada C + `Gui.mod` + las imágenes con pantalla: eso es **función nueva**, exactamente lo que el freeze aparta. Lo que sí cabría en V5 junto al evento: decidir si el ✓ además **oculta el teclado** por defecto, que es lo que espera cualquiera que haya usado un móvil |
| 21 | b1 S3 (y b2, c1, c2) | 🔴 **`NeoTest` pasa en verde SIN ENCENDER NADA fuera del RP2350 — un instrumento que miente.** `bpvm_neopixel_set_backend()` lo llama **un solo sitio en todo el árbol**: `pico/main.c:1321`. ESP32 (S3 y P4) y STM32 compilan la fachada (`src/bpvm_neopixel.c`, en sus CMakeLists) pero **nadie le registra backend**, y sin backend `bpvm_neopixel_init()` hace `return 0` —o sea **éxito**— y `show()` no hace nada. El programa pide un NeoPixel, le dicen que sí, escribe colores y no pasa nada, sin forma de enterarse. En el S3 lo anoté como verde: **no lo era** | 🟡 **AL REGISTRO.** Son **dos cosas y sólo una es bug**: (a) *que no haya driver* de WS2812 en ESP32/STM32 es un hueco de alcance —H7.4 se hizo por PIO para el RP2350 y nunca se prometió en las otras—, va a **V5**; (b) *que la fachada conteste éxito sin hardware* sí es defecto, y del que más duele según el criterio de esta casa: un instrumento mudo que además se declara verde. Arreglarlo toca **código común** ⇒ las 5 imágenes, así que **al lote**, con el mismo patrón que el 13(a). Mientras tanto, en las tandas de las familias sin driver **`NeoTest` no puntúa** |
| 22 | b2 P4 | **Un programa BP no puede saber el tamaño real de la pantalla: `Gui.Screen()` dice `480x320` en todas las placas.** Visto por Eduardo en el volcado de `GuiClickDemo` sobre un panel de **1024×600** (*«la resolución está a piñón»*) — y no es del sample, que no lleva ningún número: es de `src/gui.c:30`, `GUI_SCREEN_W/H 480/320`. Existe el camino para arreglarlo —`bpvm_gui_set_screen_size()`— pero **no lo llama ningún firmware**: sólo `test/main.c` y `tools/bpvm_sim.c`. Y la costura del P4 descarta lo que le pasan (`void bpvm_gui_disp_init(int w, int h) { (void) w; (void) h; …}`), porque el tamaño físico lo fija el panel. O sea: **modelo lógico 480×320, panel físico 1024×600**, decidido así en su día para que la paridad dual-VM se comprobara por ÁRBOL y no por píxeles. La ironía es que **el micro simulado del IDE sí puede decir «soy una P4 de 1024×600» y la P4 de verdad no** | 🟡 **AL REGISTRO → V5.** Criterio de Eduardo en el momento: *«está a piñón pero bueno, lo importante es que funciona»*. Y el arreglo **no es de freeze aunque sea una línea por familia**: en cuanto el firmware diga la resolución de verdad, **todos los layouts existentes cambian de sitio** — es cambio de comportamiento, no corrección. Emparenta con el 19 y el 21: **tres hallazgos del mismo día y de la misma forma — un dato que la placa contesta sin preguntárselo a la placa** |
