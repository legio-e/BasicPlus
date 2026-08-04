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

  Los dos workers dan `sum=656067456` idéntico, y el planificador de la BP-VM
  reparte igual de acompasado que en las otras placas — primera vez que ese
  planificador corre sobre RISC-V.

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
| 11 | `GuiImageDemo` | Imagen (asset + control) | Con #360 detrás |
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
dicho es que **el KILL de punta a punta (#257) hoy se comporta como se prometió**,
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
| 17 | a1 + a2 | 🔴 **ABIERTO — `use-after-free` sistemático, BIESTABLE y PERSISTENTE.** Muere a ~1 ms del RUN, con la memoria de la VM limpia al final (`0 bloques sin liberar`). **Cuando falla, falla siempre; cuando va, va siempre — y NO cambia con un reset.** **DESCARTADO CON EVIDENCIA**: (1) el tamaño del heap — 32 vueltas verdes en host, {1,2 workers} × {369 KB, 8 MB}; (2) la **PSRAM** — con `psram=0` y el heap en SRAM sigue fallando; (3) una **carrera SMP** — una carrera no sobrevive a un reset; (4) `Core` rancio — el `.mod` de `bpstdlib` y el embebido en el firmware son **byte a byte idénticos**; (5) la **zona de rascar** compartida (#338) — está guardada y diría `scratch OCUPADA`, y esa línea no sale en ningún log; (6) el **contenido de `/app`** — con `/app` vacío sigue fallando; (7) el **número de ficheros** — la teoría del umbral (10 va / 11 rompe) se cayó al vaciar `/app`; (8) `FS_MAX_FILES` — está definido pero **no lo usa nadie**, resto del FS plano de #305. **GIRO (3-ago, tarde)**: añadir ficheros a `/app` (los R2..R4 de la escalera) **lo ARREGLÓ** — y antes añadir el 11º lo había ROTO. O sea que **no es monótono en ninguna dirección**: no es *cuántos*, es **cómo queda colocado**. Los dos estados verificados estables: 5 vueltas verdes con totales correctos en el bueno, 3+ rojas en el malo. Hipótesis viva: desde #305 el `.mod` **se lee por trozos** (`bpvm_fs_read_at`); un fallo en un borde de bloque haría que **el mismo fichero, recolocado, se cargue bien o mal** — explica el sistematismo, la memoria limpia (no hay fuga: hay un módulo mal leído) y la muerte al primer acceso. **NO confirmada.** **LOCALIZADO**: sobrevive al reset ⇒ **está en FLASH**, no en RAM. Sólo quedan FS (littlefs), zona de packs, ENV/particiones y el log. Dato duro: con `/app` VACÍO el FS dice **17 ficheros, 131072 B = 128 KB exactos = 32 bloques**, frente a los **100 KB** de un FS recién formateado. Hay ~28 KB de metadatos/bloques sin compactar: **borrar ficheros NO devuelve el FS a su estado anterior** | ⏳ **En curso, en el estado malo y ANTES de formatear**: (a) `LIST` completo como mapa; (b) la escalera `diag/uaf17/R1..R6` —que se corrió en el estado bueno por error, donde no dice nada—; (c) `MemT1_Oo`, que crea objetos **sin threads**: si también falla, el bug no es de threads y toda la línea de hoy estaba mal orientada. Luego formatear el FS y, si eso lo cura, buscar cuántos bloques hacen falta para reproducirlo a voluntad |
| 6 | a1 Pico | 🔴 **El paquete no compilaba NADA que importara la stdlib.** El ZIP llevaba `bpstdlib/*.mod`, pero el compilador sólo sabe dónde están si encuentra un `BpVM.cfg` con `stdlibDir`, y ese fichero no iba dentro. En el repo funcionaba porque ahí sí existe. Encima, al no resolver `import Core` el compilador **lo omite y sigue**, así que los 4 errores salían en el código del usuario (`ExcCatchTest`) en vez de decir «no encuentro Core» | ✅ el ZIP lleva `BpVM.cfg` con rutas **relativas** (`./bpstdlib`) — `VmConfig` las resuelve contra el directorio del propio `.cfg`, así que la instalación se puede mover. **Guardián**: el montaje extrae el ZIP a un temporal FUERA del repo y compila `ExcCatchTest`; si falla, borra el ZIP. Verificado que sabe fallar. Encontrado por Eduardo («¿algún problema con el módulo Core?») |
| 19 | b1 S3 | `BoardTest` imprime **`variant=B`** en un ESP32-S3. Los otros dos números son correctos (`gpioCount=45`, `GPIO_COUNT=45`), lo que lo empeora: **el dato falso viaja envuelto en dos verdaderos**. Sale de `bpstdlib/Pico.bp:92`, que **no es intrínseco**: cinco líneas de BP puro, `if gpioCount() >= 40 then "B" else "A"` — el umbral separa RP2350A (30 GPIO) de RP2350B (48), y los 45 del S3 caen del lado del 48. Visto por Eduardo en el log de la tanda 7 | 🟡 **AL REGISTRO, sin tocar código — freeze.** **El reencuadre es de Eduardo y cambia el diagnóstico**: `Pico.bp` **es la librería de la familia RP2350**, no una capa común; `variant()` hace lo que documenta, y lo que falla es que **se está usando una librería de un micro en otra familia porque para ésa no hay ninguna**. Alcance real: no está en el manual (sólo en `docs/H7_TASKS.md`) y el **único llamante de todo el árbol es `samples/BoardTest.bp`**. Coste de tocarlo hoy: `Pico.mod` va **embebido** en los firmwares (`MODS=(… Pico …)` en los tres `regen_*_mods.sh`) ⇒ regenerar la stdlib + los 3 `*_mods.c` + las 5 imágenes + reflashear + repetir tandas ya verdes. → **V5: librería de placa genérica** (`docs/V5_IDEAS.md`), con el dato puesto por el micro y la librería haciendo de puente |
| 20 | — (paquete) | **El widget `Chart` es lo NUEVO de V4 en gráficos (H7, #317) y se publica sin demo.** El `ChartDemo.bp` existe y está escrito —caso realista, ventana deslizante de muestras de sensor con dos series— pero vive en `bpgenvm-c/samples/`, que es la carpeta de desarrollo de la VM-C y **no viaja al ZIP** (el copiado es `git ls-files 'samples/*.bp'`). Resultado: la stdlib expone `Gui.Chart` con sus seis intrínsecos y **no hay un solo `.bp` publicado que lo use** — verificado con `grep -rln Chart --include=*.bp`, que en todo el árbol sólo da `bpstdlib/Gui.bp` y ese fichero fuera de sitio. Encontrado al montar la escalera de GUI del P4 | 🟡 **AL REGISTRO, decisión al cerrar la fila.** No toca firmware: es **mover un fichero** y rehacer el ZIP, que ya hay que rehacerlo por los prints del `paralleltest`. Antes de moverlo hay que **compilarlo y correrlo** —lleva desde H7 sin tocarse y el árbol ha cambiado mucho—, y si va, entra en la escalera de GUI como peldaño propio. Mismo patrón que el hallazgo 5: un sample que no llega al usuario es una función que, para él, no existe |
| 24 | b2 P4 | 🔴 **Tras muchas ejecuciones seguidas, la pantalla deja de pintar — y el reset lo cura.** `GuiFontDemo` salió **totalmente negra** (las demás demos dejan fondo blanco). **El modelo estaba PERFECTO**: el volcado enseñaba los 4 labels con `font=12/20/32/48` y sus posiciones, y la consola imprimió todo hasta el final. Tras un **reset de la placa**, el mismo `.mod` sin recompilar **se ve**. Eso descarta el sample, descarta `fontSize` y descarta las fuentes (las cuatro Montserrat que usa están activadas en `include/lv_conf.h`). Lo que queda: **algo se degrada de un RUN al siguiente hasta que deja de pintar**. Lo paró Eduardo antes de que yo siguiera buscando — *«a ver si reseteando se arregla, llevo muchas demos seguidas sin un reset»*—, que es la regla de quitar estado antes de teorizar, y acertó | ⏳ **ABIERTO, y hay que ponerle número antes de decidir.** Precedente que se parece demasiado: **#352**, *«el modelo del GUI es de la VM: se rompía a los 30 RUN»*, cerrado en host — esto huele a su gemelo **en placa**. **NO confirmado**: con un solo dato no sé si es LVGL, el modelo de la VM o el driver de display. **Medida acordada, y sale gratis**: contar los RUN desde el último reset mientras se termina la escalera. **Si vuelve a los 5-10 → es serio para publicar** (cualquiera que desarrolle GUI hace run-retoca-run y se lo come a diario); **si aguanta 30+ como el #352 → se documenta el rodeo y a V5**. Eduardo apunta además que *«esto nos pasó en demos antiguas»*: si aquello también fue tras muchas ejecuciones, lleva tiempo latente. ⚠️ **Cambia cómo se leen los peldaños verdes**: se corrieron en cadena sin reset, así que lo que demuestran es **«funciona en fresco»** |
| 23 | b2 P4 | **La tecla ✓ del teclado en pantalla no hace nada.** Visto por Eduardo en `GuiListKbd`: *«el teclado virtual funciona y se pinta en el textbox, pero el V no sé si hace algo»*. No hace nada, y **no puede**: en LVGL el ✓ dispara `LV_EVENT_READY` (y el ✗, `LV_EVENT_CANCEL`), y la fachada registra **sólo dos** eventos — `gui.c:1012-1014`, `LV_EVENT_VALUE_CHANGED` y `LV_EVENT_CLICKED`. La clase `Keyboard` (`Gui.bp:569`) tiene constructor y `attach()`, nada más. O sea que el ✓ lo **dibuja LVGL** porque viene en su mapa de teclas por defecto, y por debajo no hay quien lo escuche | 🟡 **AL REGISTRO → V5.** **Más suave que el 21 y el 22**, y conviene no meterlo en el mismo saco: aquí **no se informa nada falso**. Lo que el módulo promete —*«las teclas editan ese campo, que dispara su onChange»*— **funciona**; el ✓ es un botón inerte, no un dato mentiroso. Y el arreglo es **añadir un evento** (`onReady`/`onAccept`) ⇒ fachada C + `Gui.mod` + las imágenes con pantalla: eso es **función nueva**, exactamente lo que el freeze aparta. Lo que sí cabría en V5 junto al evento: decidir si el ✓ además **oculta el teclado** por defecto, que es lo que espera cualquiera que haya usado un móvil |
| 21 | b1 S3 (y b2, c1, c2) | 🔴 **`NeoTest` pasa en verde SIN ENCENDER NADA fuera del RP2350 — un instrumento que miente.** `bpvm_neopixel_set_backend()` lo llama **un solo sitio en todo el árbol**: `pico/main.c:1321`. ESP32 (S3 y P4) y STM32 compilan la fachada (`src/bpvm_neopixel.c`, en sus CMakeLists) pero **nadie le registra backend**, y sin backend `bpvm_neopixel_init()` hace `return 0` —o sea **éxito**— y `show()` no hace nada. El programa pide un NeoPixel, le dicen que sí, escribe colores y no pasa nada, sin forma de enterarse. En el S3 lo anoté como verde: **no lo era** | 🟡 **AL REGISTRO.** Son **dos cosas y sólo una es bug**: (a) *que no haya driver* de WS2812 en ESP32/STM32 es un hueco de alcance —H7.4 se hizo por PIO para el RP2350 y nunca se prometió en las otras—, va a **V5**; (b) *que la fachada conteste éxito sin hardware* sí es defecto, y del que más duele según el criterio de esta casa: un instrumento mudo que además se declara verde. Arreglarlo toca **código común** ⇒ las 5 imágenes, así que **al lote**, con el mismo patrón que el 13(a). Mientras tanto, en las tandas de las familias sin driver **`NeoTest` no puntúa** |
| 22 | b2 P4 | **Un programa BP no puede saber el tamaño real de la pantalla: `Gui.Screen()` dice `480x320` en todas las placas.** Visto por Eduardo en el volcado de `GuiClickDemo` sobre un panel de **1024×600** (*«la resolución está a piñón»*) — y no es del sample, que no lleva ningún número: es de `src/gui.c:30`, `GUI_SCREEN_W/H 480/320`. Existe el camino para arreglarlo —`bpvm_gui_set_screen_size()`— pero **no lo llama ningún firmware**: sólo `test/main.c` y `tools/bpvm_sim.c`. Y la costura del P4 descarta lo que le pasan (`void bpvm_gui_disp_init(int w, int h) { (void) w; (void) h; …}`), porque el tamaño físico lo fija el panel. O sea: **modelo lógico 480×320, panel físico 1024×600**, decidido así en su día para que la paridad dual-VM se comprobara por ÁRBOL y no por píxeles. La ironía es que **el micro simulado del IDE sí puede decir «soy una P4 de 1024×600» y la P4 de verdad no** | 🟡 **AL REGISTRO → V5.** Criterio de Eduardo en el momento: *«está a piñón pero bueno, lo importante es que funciona»*. Y el arreglo **no es de freeze aunque sea una línea por familia**: en cuanto el firmware diga la resolución de verdad, **todos los layouts existentes cambian de sitio** — es cambio de comportamiento, no corrección. Emparenta con el 19 y el 21: **tres hallazgos del mismo día y de la misma forma — un dato que la placa contesta sin preguntárselo a la placa** |
