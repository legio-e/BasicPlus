# BasicPlus — Traspaso entre sesiones

> **Qué es esto.** El **diario** de la sesión: qué se cerró, qué quedó a medias y
> qué riesgo acecha, por fechas. Se lee al empezar una sesión y se escribe al
> terminarla.
>
> **Lo que ya NO va aquí: el estado de las fichas.** Qué está abierto, cerrado o en
> curso vive **sólo** en `docs/FICHAS.md`. Decisión de Eduardo (17-ago): *«Estado y
> pendientes son ficheros de trabajo tuyos. Pero el que dice realmente cuál es la
> situación es Fichas.»* **Si este fichero contradice a `FICHAS.md`, manda `FICHAS.md`.**
>
> El 17-ago se midió por qué: de las **51 fichas que citaba este documento, 49 eran
> una segunda copia** de las de `FICHAS`. Y se desincronizaban — ese mismo día daba por
> pendiente un censo ya hecho y contradecía a la ficha #417. Eduardo: *«me estoy
> volviendo loco con cosas que aparecen y desaparecen»*. Las secciones que
> enumeraban fichas (en curso, riesgos, plan de cierre, próximos pasos, al cerrar)
> se borraron ese día **después de subir a `FICHAS` lo que sólo estaba aquí**: la tabla
> de hitos de V5, el detalle de los restos del árbol y la decisión sobre `dist/`.
>
> **Mapa de docs:** `docs/FICHAS.md` (la fuente de verdad) · `docs/PENDIENTES.md`
> (limitaciones del lenguaje de cara al usuario) · `docs/PHILOSOPHY.md` (el porqué).
>
> **Cómo se escribe una entrada:** concreta —ficha, fichero, prueba— y sin repetir
> el estado que ya está en `FICHAS`. Aquí va lo que PASÓ, no lo que HAY.

---

## Última sesión

## 🧱 H1 (19-sep) — las seis de V10 en el compilador, cerradas: `#522` `#527` `#528` `#529` `#532` y `#517` de rebote

Sesión corta de créditos (sin agentes): todo a mano, sample por sample, verificando el artefacto en las dos VMs.
Arranque por `#522` como estaba escrito, y de ahí las cuatro de V10 en H1, en orden.

- **`#522`** (`f09401e6`): un enum en un `+` con string es su ENTERO (`"c=" + c` → `c=1`), coherente con `print c`,
  que ya imprimía el entero — la ficha proponía error de compilación, pero eso dejaba al programador sin NINGUNA forma
  de meter un enum en una cadena (`integer(c)` también se rechaza). Y lo que salió al lado: el `+` con string aceptaba
  CUALQUIER tipo (array, tupla, `null`, una llamada void) y todos rompían el heap igual; ahora error con línea.
  `Color(1)` es error semántico; `EnumSymbol.values` en orden de declaración; M6 fuera de PENDIENTES (era falso).
- **`#527` `#528` `#529` `#517`** (`1af6ff6d`): el ctor sintético del `sync` lo registra el semántico con la regla del
  `super()` implícito (y si el padre exige argumentos, error: eso cierra `#517`); una GUARDA (TRY_BEGIN sin clase)
  alrededor de cada accesor `sync` suelta el mutex ante cualquier excepción. `#528` con una lección: la clave de
  sobrecarga cualifica la clase ajena RELATIVA A LA FUNCIÓN DUEÑA, no «lo externo» — el primer intento hizo que el
  consumidor pidiera `__cls_init_Base$LCtorBase/Base;` donde el dueño exporta `$LBase;`, y lo cazó el oráculo
  `CtorHijo` en la pasada MOD8 del check (la red hizo su trabajo). `#529`: la property redeclarada en una subclase
  del mismo módulo es una redefinición (como ya lo era cross-module: medido antes de decidir), y el tope de 256
  ranuras de una clase cerrada lo dice el semántico con línea.
- **`#532`**: `try … finally` sin catch se emite; las dos VMs nombran la clase de una excepción no atrapada por
  la 4.5 (`NoAtrapa.MiError: se escapa`); guardia contra el `throw` de lo que no es Exception. Y dos rancios del
  Makefile de la VM-C: el snapshot `test/aot_ThrowUser.c` (de junio; regenerado con `AotMain`, no a mano) y un
  `samples/Core.mod` gen 0 del 25-ago que mataba `test-throwmsg` (el `fixture` ahora copia de `bpstdlib/`).
- De paso, tres refs de 8 B en local de 4 (hallazgo 34): `__pending_exc` del rethrow de `finally`, el parámetro de
  un setter escrito de una property de MÓDULO, y el `__sync_exc` nuevo ya nace de 8.
- **La stdlib no cambia**: los 28 `.mod` recompilados salen byte-idénticos (sólo la generación difiere si se compila
  sin `version`); ni blobs. El IDE fat-jar sí lleva el compilador y la miVM de hoy. La VM-C sí cambió (`#532`:
  `exceptions.c`, `link.c`): el firmware de la Pico compila limpio con ello y NO se grabó — las imágenes del 17-sep
  no nombran la clase de una excepción no atrapada; la próxima grabación sí.

**Tarde (19-sep): las cuatro que quedaban** — `#507` (`05628196`, con una trampa de build cazada al medirlo: el
jar-plugin reutilizaba el SHADED anterior como «original» y el compilador seguía con el writer viejo aunque `.m2`
tuviera el nuevo — `forceCreation=true`), `#506` (el `step` con signo y con expresión), y `#511`+`#515` (`6dcb67a9`)
con las decisiones de Eduardo de la noche anterior: inicializadores como en Java, delegación `MiClase(args)` como
primera línea del constructor, visibilidad del constructor como la de cualquier miembro. Lo que enseñó el oráculo:
`CtorHijo` dejó de compilar con la primera versión (un solo sintético sin args para la clase sin constructor) —
en BasicPlus una clase sin constructor HEREDA las firmas del padre (`Tarea(4096)` con `Tarea extends Thread`), así
que el sintético es uno por sobrecarga del padre. Eduardo: «los constructores son métodos estáticos; una clase
puede no tener ninguno, uno o varios sobrecargados». H1 no tiene fichas de V10 abiertas.

**H1 sigue ABIERTO** (Eduardo, 19-sep, al cierre: «no vale cerrar en falso; si falta mucho código pendiente lo vamos
haciendo, tardemos las sesiones que haga falta; en V7 cerramos todos los temas pendientes, no los pasamos a V8» —
la regla de V7, y yo la había saltado). **Orden para retomar**: H1, el módulo raíz propiamente: los tipos y las 226
intrínsecas en un fichero legible y las tablas generadas a partir de él (lo de L1 del 5-sep), y después los extras de
la fila del hito (sobrecarga de intrínsecas, destructor `~Clase()`, `Map`, `#396` `Time`, `#19`, `#460`, los huecos).
## 📱 H11 (21-sep, noche) — LA C5 ARRANCA: la 1.ª etapa (la imagen SIN LVGL) hecha en una tarde

Eduardo conectó la C5 y la 1.ª etapa salió por el camino de `P1`/`P2` de V6, tal como el backlog preveía: **la familia
se activa, no se construye**. `esp32c5/` es un clon del C3 (sin pantalla) con la identidad del C5: `chip_cfg.h`
(`esp32c5`, 240 MHz, los números de memoria del C3 hasta medir con U6.4), `c5_board_id.c` (29 GPIO, 6 LEDC, 6 ADC,
384 KB), `partitions.csv` y `sdkconfig.defaults`. Compiló a la primera (`idf.py set-target esp32c5`; 517 KB, 51 %
libre en la factory de 1 MB) y grabó por el USB nativo (VID 303A PID 1001: **USB-Serial-JTAG, COM6** — el camino del
C3/C6: wire por el USB, consola a UART0). Lo que dijo la placa: `esptool` «ESP32-C5 (revision v1.2), flash 32MB»; el
`INFO` `boardName=esp32c5`, `arch 243`, **`aotReceta RV32IMC_ILP32`** (sin FPU: el destino del AOT es el del C3/C6,
`ilp32` — el resto de `#502`), 240 MHz, 32 MB de flash, 384 KB de SRAM, PSRAM 0 (aún no activada).

Dos cosas que hicieron falta y no estaban: (1) **aprovisionar una placa VIRGEN sin el IDE** — arrancaba «estado 0
DEGRADADO: falta algún tamaño»; `wire_serie.py` gana el verbo `part` (`PART_LS`/`PART_DEFAULTS`/`PART_APPLY`): propuso
fs 1,5 MB + packs 1,5 MB, se aplicó, reset → estado 3, FS montado, zona de packs montada; (2) la tabla de 4 MB del C3
hacía cantar al arranque «el bootloader usa menos flash de la que hay» → **flash de 32 MB declarada** y `bpdata` del
tamaño del S3 (acaba en los 16 MB, mismos offsets: el env y el FS no se movieron); la zona de packs de **13,7 MB se
mapea bien** (INST/DATA ok). La segunda mitad de la flash y la PSRAM quedan para la 2.ª etapa (LVGL en un pack).

**Verificado**: `FileCursor` == host en la C5 (el cursor en una tercera placa, littlefs); `tanda.py` con la C5 como
columna (`SIN_PANTALLA`): MathRango, ThrowSinAtrapar, ThreadTrasMain IDÉNTICO, MachineId no-peta, las dos de GUI
saltadas por no tener pantalla (lo correcto); `FileBench`: 400 líneas 11,5 s → 0,2 s. Con 64 KB de heap de VM el
bench se queda sin heap en el caso 3 (32 KB × 2 copias): el reparto es el conservador del C3 (128 KB) hasta medir.

**Y la PSRAM, a petición de Eduardo** (*«¿no probamos de activar la PSRAM? Sería como una S3 más o menos, 8M de
PSRAM»*): `CONFIG_SPIRAM=y` en QUAD a 80 MHz (el C5 sólo tiene ese bus: su `Kconfig.spiram` no ofrece OCT ni HEX),
`IGNORE_NOTFOUND` y `ALWAYSINTERNAL=2048` como el S3. Arrancó a la primera: **8 MB detectados, la VM en PSRAM con
7677 KB** (heap 7,3 MB + pilas 512 KB, por la rama exclusiva de `main.c` de la familia: objetivo 0 = todo lo que deja
el margen de 512 KB). `FileCursor` == host, la `tanda` 4/4 igual, y `FileBench` completa los cuatro casos (antes
se quedaba sin heap): el coste de la PSRAM se ve —`readLine` 69 → 91 ms, `readFile`+`split` 16 → 19— y es el
esperado (el intérprete entierra la latencia). Dos avisos del log que no son de esta placa: la línea «DRAM interna
libre …» imprime los números de la PSRAM con la etiqueta de la SRAM, y «el asignador sólo dio 7677 KB de los 7677
planificados» avisa de nada (redondeo a KB): cosméticos, del común, para H7. **Y nos paramos ahí**, como se dijo: la
2.ª etapa (LVGL en un pack) es H6, y ya tiene la PSRAM que necesita.

## 🧭 H4 (22-sep, mañana) — LA TABLA DE CAPACIDADES: P1 hecho (`#378`, los arrays de `#470`, los restos de `#471`, el nombre `pico`)

Día de análisis primero (`V7_IDEAS.md` §H4, `092db959` + §2b `b4c46854`): BP contra `INFO` en Pico, P4, C5 y host —dos
fuentes que ya no coincidían: la Pico 12 PWM por BP y 24 por el wire; el P4 `variant()` «B» y `MAX_CPU_MHZ` 300 corriendo a
360; el C3/C6/C5 por BP se creían un S3 (`gpio_esp32.c` devolvía 45/20/8 a pelo)—, el diseño (UNA tabla por micro y por
VARIANTE elegida en el arranque) y el apunte de Eduardo: *«un micro puede tener diferente hardware… algún micro tiene más
de un ADC»* → la tabla es por variante, RAM/flash se miden, (unidad, canal) por pin.

**P1**: `bpvm_caps_t` en `bpvm_machine.h` (la capa C ya se llama `machine`: `src/pico.c` → `bpvm_machine.c`, `bpvm_pico_*`
fuera de 18 ficheros, 14 ids sin cambiar de número). De la tabla beben el `INFO` (contadores + `micro`, `paquete`,
`pwmUnidad`, `adcUnidades`, `uartIds/i2cIds/spiIds`) y `Machine` en BP (cuatro primitivas, 250-253: `uarts()/i2cs()/spis()`,
`pins()/analogPins()/pwmPins()/pulsePins()/openDrainPins()`, `hasPin/isAnalog/isInputOnly`, `manufacturer/package/pwmUnit/
adcUnits/hasBreadcrumb`, `MAX/MIN_CPU_MHZ` por micro, `variant()` honesto; los contadores de siempre, azúcar). Por familia:
RP2350 dos tablas por variante (A/B por `PACKAGE_SEL`); **ESP32 CALCULADA del IDF** (`soc_caps.h` + `adc_oneshot_channel_to_io`:
ni un número a mano, vale para S3/C3/C6/C5/P4); STM32 por die con los `#ifdef` del CMSIS (PWM sólo en los 4 pines que el
backend mapea: los «28» del INFO eran ficción; ANALOG ninguno hasta P2). Host = perfil RP2350A en las dos VMs.

**Verificado**: Pico BP == INFO (`rp2350a`, QFN-60, 12 slice, ADC 26..29); P4 BP == INFO (analógicos 16-23 y 49-54 de las
DOS unidades, PWM 8 canal, `uarts [1,2]`, `spis [2,3]`, MAX 360, variant «»). `samples/Caps.bp` en el corpus (91) y
`QuienSoy.bp` de sonda. Las siete imágenes compilan. Verde: check 91/59/9/13 + MOD8; mezcla 194; h13 38; frontend 223;
miVM 52. Referencia es/en (§Machine, la tabla).

**P2 (`#479`), la misma mañana**: **el estado del pin en el común** (`src/gpio.c`: libre/INPUT/OUTPUT/OPEN_COLLECTOR/ANALOG) y
**la validación contra la tabla ANTES del backend, con nombre** —pin que no existe, sólo entrada, colector abierto donde no
lo hay, PWM/pulsos donde no puede, instancias de UART/I2C/SPI, y el invariante «analógico ⇒ no digital» en los dos sentidos—
con el mismo texto en miVM (`gpioReclamar`/`gpioInstanciaOk` sobre el perfil del host). `Pin` gana `OPEN_COLLECTOR`
(ESP32/STM32 al backend; RP2350 lo rechaza con nombre), `ANALOG`, `release()` y `~Pin()`; **`AnPin extends Pin`** (el
(unidad, canal) del mapa; hoy las imágenes atienden la unidad 1 y la 2 avisa); `Adc.Channel(ch)` validado por el rango real
y con `release()`/`~Channel()`; el fin de RUN libera todos los pines. `samples/PinMapa.bp` en el corpus (92). **En placa**:
Pico (GP26 → canal 0, raw real) y P4 (GPIO16 → canal 0 de la unidad 1, raw 3386), los errores con el nombre del micro.
Referencia es/en (§Gpio: modos, `release`, `AnPin`; §Adc). Queda de P2 el ADC externo del STM32 (con la Nucleo/DK2 a mano).

**P3a (`#487`)**: el breadcrumb **sube al común** (`bpvm_breadcrumb_attach`: el algoritmo que vivía en `gpio_stm32.c`,
sin cambiar su contrato) y cada familia pone SÓLO el bloque que su silicio conserva entre resets: el STM32 los BKPxR del
TAMP, el RP2350 una zona `.uninitialized_data` (`__uninitialized_ram`), las ESP32 una `.noinit`. Los cuatro callbacks
desaparecen del backend de `Machine`. Verificado con el `RESET` del wire: Pico «arranque #4, rastro anterior 100 200 300»,
P4 «#3, 100 200 300». Quedan de P3 NeoPixel y el botón BOOT del C6 (con la placa) y P4 (cascada y tanda).

## 🖥️ H6 (22-sep, tarde) — LVGL en un pack: el día de análisis (`V7_IDEAS` §H6), sin tocar código

Encargo de Eduardo: *«Vamos a analizar qué necesitamos para meter LVGL en un pack. De momento nada más analizar.»*
Medido, no supuesto: **la GUI cuesta 664 KB de flash y 107 KB de RAM** en la imagen del C6 (LVGL 644 KB, de los que
418 son las once Montserrat; `gui.c` 11 KB + **106 KB de `.bss`** por el `g_nodes[512]`; los 83 builtins 4,7 KB); **LVGL
necesita 17 símbolos** de la plataforma (`nm -u`: libgcc, assert, malloc, cadenas, `vsnprintf`; nada de `math.h`, ni
ficheros, ni tiempo, **ni coma flotante** —`LV_USE_FLOAT 0`; y la C5 no tiene FPU: soft-float, como C3/C6—) y con su stdlib interna quedaría en libgcc + assert; **el que habla con el SDK es el driver del
panel** (28 símbolos en el C6, 65 en el P4). Lo que falta al pack nativo: la BIOS no presta HARDWARE; ancla y cargador
sólo en Pico y P4 (`#if CONFIG_IDF_TARGET_ESP32P4` en el común); una RAM y un pack (el primero válido, barriendo la
zona); sin destino `.npk` para `RV32IMC_ILP32` (`#502`); la fachada SPI del ESP32 sin DMA (64 B). **La placa, leída**:
ST7789 240×320 por SPI (SCK6/MOSI7/DC9/CS10), táctil CST3530 en I2C 0x58, y **reset y backlight por un CH32V003 en
I2C 0x24** (no son GPIO). **El diseño**: en el pack LVGL + `gui.c` entero (modelo y render: los 106 KB salen de la
imagen) + el driver del panel contra la HAL que presta la BIOS v5 + `Gui.mod`; en la imagen los 83 builtins y ~90
trampolines que resuelven `bpvm_gui.h` por nombre con `busca('LVGL')` (error con nombre sin pack); `'PANT'` como
contrato LVGL↔panel con dos proveedores (pack, o la imagen para DSI/LTDC: así `H6.b` es viable); la RAM por código
nativo con `packRam` y `ram_bytes` en la cabecera; N `npk` por la cadena. Nueve preguntas para Eduardo (el enlace
B1 builtins contra B2 `native` —miVM no ejecuta `native` de pack—, un pack o dos, la RAM, la HAL frente al atajo
`'ESPL'`, las fuentes, el `INFO` antes del primer Run, `Gui.mod` y el Run del IDE, la reja, `H6.b`) y un plan
P1–P4 (P1 = el pack nativo a toda la familia ESP32 + `mini.npk` en la C5; P3 = `H6.a`). Nada de código hoy.

## 🖥️ H6 (22-sep, tarde y noche) — LVGL EN UN PACK: P1, P2 y P3 hechos, y el pack PROBADO EN EL P4

Tras el día de análisis (arriba), Eduardo dio luz verde ranura a ranura. Detalle y números en `V7_IDEAS` §H6.5.

**P1 — el pack nativo, a toda la familia ESP32** (`b6b2ce59`): `bios_p4.c`/`pack_p4.c` suben a
`esp32/common/` (S3, C3, C6, C5 y P4 con el mismo código; fuera el `#if CONFIG_IDF_TARGET_ESP32P4` del
board manager). El bloque de RAM lo reserva el arranque común con `packRam=<KB>` o `SQLite=<MB>`, y **la
arena empieza detrás de lo que el pack se llevó de verdad**, no de la constante de 16 KB — sin eso el
pack de la GUI (392 KB de `.bss`) lo rechazaba el peldaño TAMAÑO. Destino `riscv32-imc` (el resto de
`#502`) y **el mini versionado** (`tools/npack_mini`, diez pasos): en el P4, «mini: TODO OK -> 0»
(`0267b92f`).

**La BIOS v5** (`552f9f26`): diez ranuras de HAL sobre las fachadas que ya existen (cero código por
familia), la tabla **crece por el final** (el pack exige `>=`, no `!=`), `publica()` con **reja** y ocho
marcas. De paso se destapó que **`#19` había roto `SQLite.bp`** el 21-sep sin que ninguna red lo viera
(los arrays de salida `long[1]` eran fijos y se pasaban como referencia); arreglado y los dos `.npk`
regenerados. `SqlDemo` en el P4 == host (`99b6ea04`).

**P2 — la costura, GENERADA** (`2f29542c`): `bpvm_gui.h` es el contrato y `scripts/gen_gui_pack.py` saca
de él los 87 trampolines, la tabla, lo que el pack publica y un doble para el PC. `make test-guipack`:
28 comprobaciones. Sin pack, `Gui.screen()` lanza «GUI: esta placa no tiene el pack 'LVGL' grabado».

**P3 — el pack** (`38525ead`): `bpstdlib/gui/` con LVGL 9.2.2, **el mismo `gui.c`** bajo `BPVM_EN_PACK`,
el pegamento sobre la BIOS y el panel de la C5 (ST7789 + **el CH32V003 que lleva el reset y el
backlight por I2C** + el táctil CST3530, todo leído de su BSP). `lvgl.npk.RV32IMC` = 759 996 B.

**Y LA PRUEBA EN EL P4** (`d28a31fb`), que es idea suya: *«¿se podría probar en la P4 sin desactivar el
LVGL actual? No sé, con un `Gui2.bp` que llame a la LVGL del pack»*. Con un **panel nulo** elegido en el
build y **`Gui2.bp`** (`native … from pack`, sin builtins de por medio), `samples/GuiPack.bp` habla con
las dos: **el pack dice 240×320** (su modelo, su LVGL, su TLSF) **y la imagen 480×800** pintando en el
MIPI-DSI — `GuiShot` sigue sacando su captura con el pack cargado. Dos instancias de LVGL en el mismo
proceso, cada una con sus globales. **Lo que no prueba**: el panel, el táctil y el flush; eso es la C5.
El P4 quedó restaurado con `SQLite.pack` y `SqlDemo` == host.

## 📱 H6.a (22-sep, noche) — LA C5 PINTA Y RESPONDE AL TACTO, con LVGL viniendo de un PACK

Eduardo conectó la C5 y se cerró `H6.a` en una sesión. La imagen de esa placa **no lleva ni `gui.c` ni
LVGL** —sólo los 83 builtins y los ~87 trampolines—, y aun así `GuiShot` pintó su captura EN LA
PANTALLA (confirmado por él mirando la placa) y `C5Touch` contó cinco pulsaciones reales llegando a su
manejador BP. Números y detalle en `V7_IDEAS` §H6.6; commit `92b5c7b8`.

Camino: imagen grabada → **el mini primero** («mini: TODO OK -> 0», que es justo para lo que existe) →
`Gui.pack` podado a `riscv-imc` (827 392 B) → el INFO ya publica 240×320 → a pintar.

**Dos fallos, y el segundo enseña más que el resultado.** (1) `Gui.mod` compilado como proyecto suelto
salía con **generación 0** («'Gui' (gen 0) es mas antiguo…»): el pack lleva ahora los `.mod` canónicos
de `bpstdlib/`, como el de SQLite. (2) La placa se **reiniciaba muda** en el primer `Gui.screen()`, y
con la consola sólo en UART0 no había pánico que leer: dos vueltas de suposiciones —Eduardo y yo, las
dos hacia el driver del panel— hasta activar la copia por USB y **verlo**. No era el panel: era la
recursión de `discover_deps` sobre littlefs y una lectura de flash que en esta placa **no va directa
porque el destino está en PSRAM**. Main task 8 → 16 KB, con el volcado traducido escrito al lado.

**Medido**: pantalla entera **143 ms**, una etiqueta **1 ms**. Una interfaz responde; el peor caso
cuesta, y la palanca es la fachada SPI (DMA), no el driver.

## 🧩 H6.c `G1` (23-sep, mañana) — VARIOS MOTORES NATIVOS A LA VEZ, y `env_get` en la BIOS v6

**Primero, una pregunta de Eduardo** sobre el búfer de 64 B del panel de la C5 (*«¿se puede hacer más
grande y mejorar el refresco?»*). Tocando ese 64, no: la fachada SPI del ESP32 va SIN DMA y 64 B es el
búfer del periférico; lo de ayer no era un resto de la depuración sino otro fallo, arreglado en plena
caza del reinicio. La palanca es la fachada (que trocee ella, y DMA para lo grande): ~143 → ~45 ms
estimados, y es `G4`. Análisis en `V7_IDEAS` §H6.7.11. Eduardo eligió seguir con `G1`. De paso salió que
la VM-C recorta en silencio a 256 B los builtins de I2C/SPI/UART y la Java no —rompe la paridad—, y va
en su propia sesión.

**`G1`, con el diseño corregido antes de programar** (lo aprobó Eduardo: *«me parece perfecto»*): en vez
de `ram_bytes` en la cabecera y un reparto por orden de cadena, **el reparto es el SELLO** —la placa da
el tope al grabar, el cargador sólo comprueba que no se pisan—. La cadena en vez del barrido, gana el
último, la arena común y con dueño, `env_get` entero o nada, y el IDE dice «no cabe» antes de grabar.
Commit `65756e91`; detalle en `V7_IDEAS` §H6.8.

**En placa**: la C5 con Gui + mini («2 motores, 2 cargables», `env_get('packRam') = 512`, `GuiFlushBench`
144/1 ms como ayer; un segundo `Gui.pack` sin borrar el primero → «no cabe: 383 KB, quedan 129» y la
zona intacta), y el P4 con SQLite + Gui («2 motores», la arena de SQLite 3 723 KB encima de los dos,
`SqlDemo` == host, `GuiPack` «las dos vivas»). Lo ya grabado ayer cargó sin regrabar en las dos.

**Tres cosas que salieron por el camino**: (1) `test-bmgr` **no enlazaba desde el 13-ago** (#362) y nadie
lo vio porque no está en ninguna red; (2) **el STM32 no compilaba desde H6 P2**: CubeIDE compila todo
`src/`, y `gui_pack.c`/`gui_pack_publics.c` no son de sus imágenes (guardas en el generador, `0386c5ce`);
(3) **el P4 no se reinicia al abrir su puerto** (CH343): tras grabar un pack hace falta `reset` para que
lo cargue — un log de un solo arranque de tres minutos me hizo creer que no cargaba.

**Redes**: `test-npack`, `test-bios`, `test-env`, `test-bmgr`, `test-guipack`, `test-guidrop`,
`test-sqldemo`, `boardsim-smoke` y `compat check` VERDE (92 PASS + MOD8 + sombra). Compilan las cinco
ESP32, la Pico, las dos STM32, el host y el simulador. La Metro no estaba: su cargador se compiló pero
los varios motores se probaron en la familia ESP32.

## 🖥️ H6.c `G2` (23-sep, mañana) — EL DRIVER DE PANTALLA EN SU PROPIO PACK (falta mirarlo)

Eduardo dio el visto bueno con dos aportaciones antes de programar: el nombre (*«¿cuando hablas de
paneles es a lo que yo llamaba drivers?»* — sí, y todo pasó a llamarse **driver**: `bpstdlib/gui/drivers/`)
y el orden RGB y los bits de menos, que dieron el símbolo opcional **`formato`** de `'PANT'` (RGB565,
RGB565 girado, RGB888; el orden RGB/BGR es un bit de MADCTL y no toca el contrato). Commit `84acbe0c`;
detalle en `V7_IDEAS` §H6.9.

**En placa**: la C5 con `Gui.pack` (sin driver) + `DriverC5.pack` → `GuiFlushBench` **144/1 ms**, igual
que antes, y el log del driver arrancando paso a paso; sin el driver, `Gui.Screen()` dice «GUI: no hay
driver de pantalla ('PANT'): graba el pack del driver de esta placa». El P4 con SQLite + LVGL +
`DriverNulo` a la vez: `GuiPack` «las dos vivas», `SqlDemo` == host. ⚠️ **Falta VERLO**: Eduardo estaba
fuera y la pantalla y el táctil de la C5 no se han mirado.

**El fallo del día, y enseña**: la C5 se reiniciaba en el `init` del driver. Con la consola por USB
(la receta de ayer): «Load access fault» en `di()` leyendo `g_bios`. El driver no tiene NINGUNA variable
inicializada, así que no hay `.data`, `-Tdata` no colocó nada y su `.sbss` quedó en la flash —y
`NpackReloc` lo realojó como flash, EN SILENCIO—. Arreglado por los dos lados (`npk.ld` + la
comprobación que lo rechaza con nombre). El mini no lo había cazado porque sí tiene datos.

**Redes**: `test-guipack` (con y sin `'PANT'`), `compat check` VERDE, IDE reconstruido; las cinco ESP32
siguen como en `G1` (sólo cambió la C5).

## 🖥️ H6.c `G3` (23-sep, media mañana) — EL DRIVER GENÉRICO: la C5 funciona sin una línea de C (falta mirarlo)

Eduardo aprobó el diseño con un cambio de cliente (*«OK, la C5 primero»*): el C6 no puede cargar todavía el
pack de LVGL, y la C5 tenía su `DriverC5` para comparar. Commit `f78cb29b`; detalle en `V7_IDEAS` §H6.10.

**Lo que hay**: `DriverSpi` (`bpstdlib/gui/drivers/spi/`), un driver para cualquier MIPI-DCS por SPI. La
placa va en el ENV, una línea por dispositivo (`lcd=`, `exp=`, `touch=`, con el formato de `sd=`), y el
controlador en un `.seq` de texto dentro del pack (`st7789.seq`: SLPOUT y RAMCTRL; lo de la placa lo pone el
driver). El desfase por giro sale de `ram` y `off`. `'PANT'` gana `estado` (opcional): la VM lo pregunta en
el primer `Gui.Screen()`, y una errata del ENV llega al programa con nombre.

**En placa (la C5, con la imagen de hoy)**: `DriverC5` borrado, `DriverSpi` grabado y las tres líneas en
el ENV → `GuiFlushBench` **144/1 ms** (con `DriverC5`, 145/1); el log lo enseña leyendo el ENV y su `.seq`
desde su propio pack; quitando `dc`, `Core.RuntimeError: GUI: pantalla: ENV lcd: falta 'dc'`; y el táctil
contesta (10 s sin aviso con 0x58, y el control con 0x59 da «el tactil no contesta» a los 178 ms).
⚠️ **Falta VERLO**: la pantalla y pulsar — Eduardo estaba fuera.

**Cómo se sabe que manda lo mismo que `DriverC5`**: `make test-driverspi` ejecuta los dos drivers contra
una BIOS que graba los cables. Volcado, giros y táctil byte a byte iguales; el arranque con los mismos seis
comandos (RAMCTRL cambia de sitio: ahora es del `.seq`); más los desfases del C6 y de las 135×240, el
`.seq` de un usuario y 17 erratas con nombre. Con CONTROL: tres sabotajes, los tres cazados.

**Redes**: `test-driverspi` nueva, `test-guipack` con el caso de `estado`, la imagen de la C5 compilada y
grabada (es la única que compila esa costura: el P4 no lleva `BPVM_GUI_PACK`), `driver_c5.npk` idéntico
tras tocar sólo su comentario.

## 🚌 H6.c `G4.a1` (23-sep, mediodía) — EL BUS, UNA CONFIGURACIÓN: compartido, en las cinco familias

El diseño salió de tres aportaciones de Eduardo (`V7_IDEAS` §H6.11): el bus se COMPARTE y lo exclusivo es el
dispositivo; el árbitro va en la HAL BP; los pines del bus van en el ENV, y quien pida otros recibe un error
(y si el ENV no lo declara, el primero que aparece; y se puede liberar). Commit `05615664`; detalle en §H6.11.7.

**Lo que hay**: `bpvm_bus.c` (la regla y los pines reservados), las fachadas SPI/I2C como puerta de los tres
usuarios, la cintura partida en abrir/ajustar/cerrar en las tres familias, `spiN=`/`i2cN=` leídos al
arrancar (y en el simulador), `release()` en BP con la misma regla en miVM, la forma corta de `sd=`, y
`DriverSpi` con las líneas sin pines de bus.

**En placa (la C5)**: el arranque declara sus dos buses; 145/1 ms con el driver nuevo; `C5Bus.bp`: con la
pantalla viva, un programa se engancha al SPI 2 y la etiqueta sigue a 1 ms; los errores con nombre; y el
programa lee el CH32 por el I2C 0 compartido. **Redes**: `test-bus` nueva, `test-driverspi`, `test-guipack`,
`test-sd`/`test-blk`/`test-sdsc` (las dos primeras no enlazaban desde #478/H4), paridad 93/93 con
`BusCompartido.bp`, y las ocho imágenes compiladas (S3, C3, C5, C6, P4, Pico, Nucleo, Discovery).
⚠️ `sim-smoke` falla, pero ya fallaba en `HEAD` (el simulador no encuentra `Core`): apuntado aparte (→ `#537`, cerrada esa tarde).

## 🧪 H9 `#537` (23-sep, tarde) — EL MICRO SIMULADO, EN LA RED: cuatro smokes podridos y un `Stdlib.pack` rancio

El `sim-smoke` que `G4.a1` vio rojo en `HEAD` llevaba así desde el 20-sep: **`656ec7d5` (H1 P3) hace que el parser
ponga `import Core` en todo módulo**, y el smoke arrancaba el simulador sin librería. Acotado construyendo el
frontend a los dos lados del commit con un repo Maven privado (`656ec7d5^`: el Hello F1 sin importaciones;
`656ec7d5`: con `Core`); el simulador estaba bien. Al mirar los hermanos salió más, todo de la misma forma
(pruebas fuera de las redes): el caso GUI de `io-smoke` no cargaba desde el 11-sep (`Gui` → `Collections`, y el
arnés subía las dependencias con una lista a mano) y su guarda lo contaba como «saltado» con el total en OK; el
control de `test-listtrunc` medía contra el tope del recorredor que el sim tenía antes de U3.24; y, lo gordo,
**`packs/Stdlib.pack` era el del 18-sep**: el micro simulado del IDE daba `LINK_ERROR` con cualquier
`StringBuilder`, y `montar-zip.sh` lo habría metido en el ZIP de V7.

**Decidido con Eduardo**: los smokes arrancan el sim como el IDE (`--pack=packs/Stdlib.pack`); una guarda del
contenido del pack (`compat/stdlib_pack.sh`, con `--regenerar`) en la red y en `montar-zip.sh`; y los cuatro
smokes en `compat.sh check` (frente `check-sim`, con su propio binario del sim). Lo del IDE, a `#538`.

**Red**: `check-sim` 5/5 con un control por arreglo (pack rancio, smoke viejo, pack sin `Collections`, sabor LVGL);
`compat.sh check` entero en verde (93/93, cápsula 59/59, norma 9/9, sim 5/5, sombra 13/13, MOD8 93/93). Commit
`21fbfb28`; detalle en la ficha.

## ⏭️ Traspaso (23-sep, tarde) — `#537` cerrada; lo siguiente sigue siendo `G4.a2` (el árbitro)

**Lo nuevo que hay que saber**: `compat.sh check` tiene un frente más, `check-sim` (~11 s): construye
`bpgenvm-c/build/bpvm-sim-red.exe` (no toca `bpvm-sim.exe`) y exige el sabor `gui1-lvgl0`, como la paridad. **Al
tocar la stdlib, `bash compat/stdlib_pack.sh --regenerar`** además de `regen_all_mods.sh`: si no, la red se pone
roja con la lista de módulos que no cuadran. `#538` (el IDE y el `Core` implícito) queda abierta en H8.

**Lo de antes sigue igual**: la C5 con `C5Touch` para mirar colores y pulsar (cierra `G2` y `G3`), y después
`G4.a2`. Sin push.

## 👆 H6.c (23-sep, tarde) — `G2` y `G3` MIRADOS, y el táctil que «se colgaba»: era la HERRAMIENTA

**Mirados por Eduardo en la C5**: los colores bien y el botón de `C5Touch` responde. Pero al volver tras
horas, el botón no reaccionaba, y al recargar el programa volvió.

**Primero, un diagnóstico equivocado** (`431e2838`): leyendo el chip a mano creí verlo colgado, y `DriverSpi`
ganó lo que hace su fabricante y no hacíamos —el MODO NORMAL al despertarlo— y un vigilante de su latido ESD
que lo reiniciaba. La «prueba larga» que dejé falló a los tres minutos, y el log no tenía ni una línea tras
la primera lectura.

**Lo que era** (`V7_IDEAS` §H6.12): **`wire_serie.py` reiniciaba la C5 al CERRAR el puerto** —medido: 142 s
de uptime y, tras el cierre, 9,5 s—. Cada `run` dejaba el programa en marcha mientras escuchaba y lo mataba al
terminar, con la pantalla aún pintada. Arreglado en la herramienta: DTR/RTS apagados al abrir (97 → 108 →
119 s), `resetcable` para reiniciar por el cable, y un reset explícito tras `pack`/`env`. Con el puerto
cerrado: cinco pulsaciones con su traza. El vigilante, quitado (Eduardo); se quedan el modo normal y la
traza, que ahora lee el latido.

**Y la pregunta de Eduardo que abre otra puerta**: *«¿se despierta tocando la pantalla?»* Depende del modo en
que se le deje dormir (bajo consumo y gesto, sí; sueño profundo, no), y el `INT` del táctil va al GPIO5 de la
C5: anotado en `#490`.

## ✅ H6.c (23-sep, 19 h) — LA PRUEBA LARGA: 2 h 13 min sin tocarla, y el táctil responde

`C5Touch` con la traza, lanzado a las 16:34 con la herramienta ya arreglada y el driver sin vigilante. Eduardo
la dejó 2 h 13 min sin tocar y al volver el botón respondió: sus tres pulsaciones están en la traza. Leído
después con `resetcable` + `log` (la RAM superviviente): un latido cada 5 min, 7 500 lecturas en cada uno
—unas 225 000 en total— sin un solo fallo, y el latido ESD del chip avanzando todo el tiempo (unos 32 000/s).
Con eso se cierra el asunto del táctil de la C5 (la fila H6 de `FICHAS`).

## 🔌 IDE (23-sep, 19 h) — al desconectarse dejaba la C5 en MODO DESCARGA: arreglado

Medido con Eduardo: conectar el IDE a la C5 y desconectarlo la dejaba en el cargador de la ROM (el wire no
contesta; esptool sí, sin reiniciarla) — peor que el reinicio de `wire_serie.py`: el firmware ni arranca y la
pantalla sigue pintada. Un arnés que abre como `BpvmClient` (purejavacomm, DTR=1 y RTS=1) y cierra de tres
maneras, dos veces cada una: cerrar sin más → descarga; DTR y luego RTS → descarga; **RTS y luego DTR → sigue**.
`BpvmClient.cerrarPuerto` lo hace así en sus dos cierres; medido con el `BpvmClient` real (2 de 2) y con el IDE
de Eduardo (el uptime siguió contando). Detalle en `V7_IDEAS` §H6.12.

## 🚦 H6.c `G4.a2` (23-sep, noche) — EL ÁRBITRO DE LOS BUSES: el turno en la HAL BP

Con dos correcciones de Eduardo al diseño (`V7_IDEAS` §H6.11.6b): sin tabla de dispositivos —«todas son
operaciones petición-fin»: cada petición trae su bus, su CS, su velocidad y su modo— y «la velocidad se guarda
la actual; si la nueva es la misma no se cambia». Commit `c5b701a8`; detalle en §H6.11.8.

**Lo que hay**: el turno (`bpvm_bus.c`) con un mutex y una condvar para toda la tabla, plazo de 1 s y el error
con nombre; la fachada SPI con `tomar`/`cs`/`xfer`/`dejar` y la velocidad actual del bus; cada llamada de I2C un
turno y `escribir_leer`; la BIOS v7 (45 ranuras); `DriverSpi` y la TF por turnos.

**La inanición**, que es lo que más enseña: la destapó la propia prueba de dos hilos, una vez y por casualidad.
Arreglada con CORTESÍA (quien llega con otros esperando se pone a la cola), quitarla y correr la prueba seis
veces dio seis verdes: la prueba no la protegía. Ahora tiene su medida (un glotón y la TF, cinco intentos, el
tope en turnos): 8/8 verdes con la cortesía y 8/8 rojos sin ella.

**En placa (la C5)**: la BIOS v7, 148 ms por pantalla (145 antes) y `C5Touch` pinta y responde. Y por el camino:
**el segundo programa de GUI heredaba el fondo del anterior** (el `reset` limpiaba los widgets, no la pantalla);
hasta hoy no se veía porque cada `run` reiniciaba la placa. **Arreglado** en un commit aparte: al terminar un
programa, una pantalla NUEVA (tema, banderas de fábrica, sin callbacks) y la vieja fuera; en la C5, fondo blanco
y el botón responde (Eduardo).

## 🔀 H6.c `G4.a3` (24-sep, mañana) — EL DISPOSITIVO Y LA TRANSACCIÓN, y la microSD hablando con la pantalla en el mismo bus

`92e3e532`. El diseño salió de la conversación de la mañana (`V7_IDEAS` §H6.11.9), en tres correcciones de Eduardo:
el bus **por su número** («lo asigna el fabricante»); **agrupar operaciones como transacciones SQL**, sin
monopolizar el bus; y, una vuelta más, **primero en C** (*«tiene más sentido que se llamen desde ahí»*) y un
**tope de tiempo configurable pero siempre limitado** —su ejemplo: un conversor que tarda 200 ms no puede dejar a
nadie sin bus—. Como los hilos de BP van en un hilo del SO, la transacción se PREPARA y se ejecuta de una vez (un
BEGIN…COMMIT con código en medio bloquearía a la GUI); el tope lo pone el ENV de cada bus (`ms:`), 20 ms si no lo
dice, nunca más de 100, y se calcula antes de empezar. Eduardo aprobó además pasar `DriverSpi` a transacciones
y dejar el turno a mano sólo MEDIDO, y arreglar la TF (que hace justo lo del conversor) en H3.

**Hecho**: la transacción en la HAL BP (SPI e I2C, con repeated start), la BIOS v8, `DriverSpi` por
transacciones, y en BP `Spi.Device`, `Spi.Transaction`, `writeRead`, `execute` e `I2c.Transaction` en las dos VM
(`SpiDevice.bp` en la paridad, 95/95). Las cinturas: repeated start en ESP32 (lista de órdenes del IDF), Pico
(`nostop`) y STM32 (a nivel de registro, sin probar en placa).

**Lo que enseñó** (§H6.11.10): el IDF del ESP32 hace una transacción I2C en como mucho 8 órdenes y no deja
encadenar dos listas sin STOP, así que lo que cabe en una transacción lo marca el ESP32 **en todas las familias**
(hasta 3 mensajes, 8 órdenes); **el ESP32 no aceptaba más de 64 bytes por transferencia SPI sin DMA** —desde BP,
un `Spi.write` de más de 64 fallaba entero— y la HAL del STM32 truncaba a 65 535 en silencio: las dos cinturas
trocean; el formato MOD8 no admite una clase usada antes de declararse (lo cazó la red); y `PACK_LS` sólo lista 16
packs (`#539`, tarea aparte). Al grabar la C5, la imagen creció 64 KB y el pack de LVGL «realojado para OTRA
dirección» no cargó: se regrabaron `Gui` y `DriverSpi`, en ese orden.

**En placa (la C5)**: 47 ranuras y «tope de un turno 20 ms» al arrancar; `GuiFlushBench` 147 ms por pantalla
(148); **`C5Sd.bp`**: un programa habla con la microSD por `Spi.Device` (CS 23, 400 kHz) en el SPI 2 de la pantalla
(40 MHz): CMD0, CMD8, ACMD41 (con el bus libre entre intentos), CMD58, el bloque 0 entero en una transacción de
997 bytes (55 AA) y **30 de 30** comandos intercalados con la pantalla; el tope igual que en el PC; el latido del
táctil con repeated start, igual que con STOP; y el botón de `C5Touch` —el táctil de `DriverSpi` con repeated start— **falta pulsarlo** (la lectura del táctil sí va: el latido avanza y la primera trama llega).

## 🔴 H7 y H4 (24-sep, mañana) — LAS TRES ROJAS: `#535`, `#530` y `#519`

Con la Nucleo para la tarde, Eduardo eligió las tres fichas rojas que quedaban abiertas. Las tres eran del mismo
tipo: algo que fallaba **sin decirlo**.

- **`#535`** (`cebe358d`): el `move` de la VM-C no comprobaba nada y copiaba 4 B por elemento. Se escribió
  como espejo del de miVM —y la referencia tenía su propia omisión: rechazaba `long[]` y `double[]`—.
  `MoveRango.bp` lo prueba con siete tipos de array y catorce errores; con el `move` viejo, el `byte[]` escribía
  FUERA del array (8 B desde el byte 8 de uno de 6).
- **`#530`** (`3476867a`): **reproducido, era peor que la ficha**, que decía que miVM lo ejecutaba: saltaba a
  `cs - 1` y ejecutaba un byte cualquiera. Las dos VMs arrancaban el hilo leyendo la ranura 0 sin la regla de la
  herencia que `INVOKE_VIRTUAL` sí seguía: una copia de la regla que se quedó atrás. Ahora la regla está escrita
  una vez por VM (en C había tres copias) y el arranque la usa. La otra mitad de la ficha ya estaba resuelta
  desde H1 P3 (hay un solo `Thread`, el de `Core`). Por el camino me equivoqué un momento: «`Sumador` no
  asignable a `Object`» parecía un bug y es la regla del manual (un `Thread` no es un `Object`).
- **`#519`** (`2b08cc74`): la ficha dejaba dos salidas y la pregunta fue para Eduardo: **error, como en
  módulo**. Un método `native` que no se puede traducir ya no «corre interpretado» con un aviso —que sin una
  native de módulo al lado ni salía—: es el mismo error que una función, con el método, la línea y el motivo con
  nombre («lee el campo 'n'…»). Antes de proponerlo conté los métodos `native` del árbol, y **el censo dio 0
  cuando había uno** (`NatV7`): mi script cortaba la clase en el `end Caja` del constructor, la misma trampa
  que el reordenado de `Spi.bp` en `G4.a3` (memoria nueva: casar el `end` por la sangría). El de `NatV7` se traduce:
  el error no rompe nada.

**De paso**, al pasar los tests de C de `#530`: `test-aotnew` no compila `NatNew.bp` desde el cambio de los arrays
fijos (`60325e4c`) y `test-debug-mt` carga un `Arith.mod` MOD5 de mayo. Los dos están rotos desde antes y fuera de
toda red; van a una tarea aparte.

## 🧰 H9 (24-sep, mediodía) — PREPARADA LA TARDE DE PLACAS; y la tanda contra el simulado destapa `#540`

Eduardo, de vuelta a V7: *«Tenemos muchas cosas abiertas. Cuando vuelva esta tarde aprovechamos que puedo conectar y
probar placas y cerramos todo lo que podamos.»* El inventario de lo abierto que pide placa salió de leer §ABIERTAS de
`FICHAS` entera (tres lectores en paralelo, una sección cada uno, y lo que dijeron contrastado con las fichas).
Preparado para la tarde:
- `tools/tanda_v7.json`: las seis de siempre + `Destructor`, `TimeDemo` y `ArrayFijo` (lo que H1 P5 pide ver en
  S3/C3/C6/Nucleo/DK2) + `MoveRango` (`#535`) + `HiloUsa` (`#530`). Y `tanda.py` sube ya los **módulos de la app** a
  `/app` (`HiloLib` con `HiloUsa`; antes sólo sabía de la stdlib). Contra el micro simulado: todo IDÉNTICO salvo
  `GuiShot`, sin artefacto porque el simulado de la red no lleva LVGL.
- `samples/Bmp280RepStart_Nucleo.bp` (el repeated start del STM32: cinco pruebas, cada una con su control por STOP)
  y `samples/C6Boot.bp` (el botón BOOT de la C6).
- **`#466` (resto)**: el log de arranque de la Pico decía «WATCHDOG» tras un RESET pedido por el wire; ahora usa la
  misma función que `resetCause` («reinicio pedido»). Una línea, la imagen rehecha; se ve en la placa.

**La tanda contra el simulado destapó `#540`.** `MoveRango` daba por null un local sin inicializar: en el PC la pila
es nueva y vale 0 en las dos VMs; en el simulado, con programas corridos antes, era basura («move: src no es ref a
heap»). Eduardo: *«Las variables locales si no se inicializan su valor es indeterminado. En teoría utilizar una
variable local sin inicializar es un error.»* El sample estaba mal (lleva ya `:= null`); lo que falta es que el
compilador lo detecte: **el PC lo tapa y la placa no**. Ficha abierta en H1, con la propuesta (la asignación
definida, como Java) y la pregunta: error o aviso.

**De paso**, la higiene de `FICHAS`: doce fichas cerradas que el censo no tachaba, `#525`/`#526` (cerradas el 18-sep)
en la fila de H7, y tres entradas de la cola de V5 que se cerraron el 9-sep con número (`#482`–`#484`) sin marca.

## 🧪 H9 `#541` (24-sep, tarde) — CINCO PRUEBAS DE C, EN LA RED: el AOT ejecutado y el núcleo del depurador

La tarea aparte del mediodía (en un worktree, mientras Eduardo estaba con las placas): `test-aotnew` y `test-debug-mt`,
rotos y fuera de toda red. **Al mirar sus hermanos salieron tres más**, y en un checkout limpio ninguna daba rojo por lo
que prueba: `test-debug`, `test-debug-mt` y `test-listen` (el mismo `DBGMOD`) **se saltaban con salida 0**, porque
`samples/Arith.mod` salió de git el 6-ago y nadie lo regenera (en el checkout principal es el MOD5 del 22-may, y la VM lo
rechaza); a `test-listen` le faltaba además el registro del FS de `#344`, y desde el `import Core` implícito no
encontraba `Core`; `test-aotnew` no compilaba `NatNew.bp` desde `#19`; y su gemelo `test-aotgc` estaba rojo porque
`AotMain` no veía `Core` (en el checkout principal lo tapaba un `samples/Core.mod` suelto).

**Lo gordo salió de ahí**: sin la interfaz de `Core` el AOT no da error, genera ranuras fijas para los métodos de una
clase y `areaDe(6,7)` sale **0 con `status=OK`**. Las pruebas ya le dan la stdlib; el AOT queda abierto como `#542` (H5),
con la propuesta de que sin `Core` falle con nombre. **Y un sabotaje enseñó que el caso 3 de `NatNew` nunca vigiló lo
que dice**: con los helpers de 4 bytes sobre el `word[]` (el fallo de `05a68055`), la native se lee bien a sí misma y
`tocaPalabras()` da 6000 igual. Ahora `Main` relee `w` con el intérprete (`1000 0 2000` con el sabotaje).

**Hecho**: `NatNew.bp` con un `word[]` dinámico (`[word(0), word(0), word(0)]`: `word[]` no tiene constructor y el
literal de enteros no se estrecha solo) y la relectura; las tres del depurador compilan su `.mod` en cada ejecución
(`DBGBP`, de `samples/arith.bp` a `build/dbgmod/<prueba>/`; pasar `DBGMOD` es ya un error con nombre); el AOT de las
pruebas con la stdlib en su `outDir` (macro `aotmain`: lo que dice va a `aot.log` y no a `/dev/null`, y «sin interfaz» es
rojo); `debug_listen.c` con su registro del FS; `dbg_client.py` enseña el stderr del servidor al fallar (lo tiraba: el
fallo de `test-listen` era un timeout sin motivo); y un frente `check-testc` en `compat.sh check` con las cinco (verde =
salida 0 y su línea de OK; ~10 s).

**Red**: las cinco en verde en un worktree limpio, con un control por arreglo (los helpers de 16 bits con paso 4, `AotMain`
sin `Core`, el servidor sin el registro, un `DBGBP` que no compila: los cuatro en rojo y con nombre). `compat.sh check`
entero en verde (paridad 97/97, cápsula 59/59, norma 9/9, sim 5/5, pruebas de C 5/5, sombra 13/13, MOD8 97/97; 5 min
42 s). Commit `4987771e`; detalle en la ficha.

**Lo nuevo que hay que saber**: `compat.sh check` tiene un frente más, `check-testc`; las pruebas del depurador se lanzan
con `DBGBP=ruta/X.bp`; y `test-aotnew` reescribe `test/aot_NatNew.c` en cada pasada (el versionado es ya el de MOD9). Del
Makefile, 63 de los 69 objetivos `test-*` siguen sin red. Sin push.

## 🔌 H9 (24-sep, tarde) — LA TARDE DE PLACAS: las nueve grabadas, probadas y SELLADAS

Eduardo conectó las nueve, una detrás de otra; yo grabé la imagen de hoy y pasé la tanda de V7
(`tools/tanda_v7.json`, 11 pruebas). **Verde en las nueve salvo una prueba en la C6**, y selladas como punto de
restauración (`dist/firmware-v7-24sep`, `22f30005`): la precondición de `H6.b`. Por el camino:
- **La Nucleo**: el repeated start del STM32 contra el sensor de Eduardo, que resultó ser un BME280 (el sample sólo
  aceptaba el chip_id del BMP280): cinco pruebas, cada una igual a su control por STOP. Con el botón de `C5Touch`
  (5 pulsaciones, 5 por el puerto), `G4.a3` queda cerrado del todo. El FS que «bailaba» no bailó (tres arranques en
  la Nucleo y tres en la DK2); el botón NRST da «pin (NRST)» y el log de antes vivo (`L15`).
- **La C6 reventaba** (`#543`): `TimeDemo` y las dos de GUI la reiniciaban. La consola por USB dio el «Stack
  protection fault» en `main`, y `addr2line` la cadena: la carga recursiva de dependencias, con littlefs debajo, ya
  no cabe en 8 KB. 16 KB en las cuatro ESP que quedaban, y la C6 pasa, salvo `GuiWinJson`, que en V7 no cabe en su
  heap («No space in heap», anotado en `#468`). Y su botón BOOT no se ve (`#544`).
- **La Metro y SQLite**: su pack llevaba el `.mdn` de la ABI 6, rechazado con nombre; y el `.pack` del repo llevaba el
  motor ARM del 19-ago, que exige la BIOS exacta (`bp_pack_init` → 3). Reconstruido: `SqlDemo` == host.
- **Las herramientas**: la tanda reiniciaba la S3 y el P4 al abrir su CH343 (el DTR/RTS por defecto de pyserial) y no
  les daba tiempo a contestar; `wire_serie` no llegaba a la Pico (sin DTR su CDC no transmite). Arregladas las dos
  (`71e8b11d`).
- `#466` en la Pico: «reinicio pedido» tras un RESET por el wire; `U1.2`: el log de antes sobrevive. `P1.C3`:
  `trytest` y `JsonDemo` idénticos en la C3.
- La SD «superfloppy», **cancelada** en placa (Eduardo): no se puede preparar la tarjeta en Windows sin riesgo, y el
  caso está probado en el PC.

## ✅ H1 (24-sep, tarde) — H1 CERRADO: P4 cancelado, P2b a H5, y `#540` (la asignación definida)

Eduardo pidió repasar H1 y aprobó las tres propuestas («OK, los 3»): **P4** (declarar los primitivos en BP),
cancelado —son palabras reservadas y no ganarían nada—; **P2b** (el AOT y las intrínsecas) a H5, que es su hito; y
**`#540` como error de compilación** (`5b4d7edc`). Antes de encenderlo, el censo: de 582 ficheros sólo 3 leían
una local sin valor, los tres dando por hecho que valía null o 0; arreglados. El caso fino fue `StrNulo`: su «string
sin inicializar es null» sigue siendo cierto para un CAMPO —los objetos nacen a cero— y el sample lo comprueba así
(un `string` no admite `:= null`). Dos restos del backlog de H1 resultaron ya resueltos: la `intrinsic` sin registro
(la palabra ya no existe desde P1) y sacar `Exception` de `Core` (ya no ahorra nada desde P3). **Con esto H1 queda
cerrado.**

## 📋 H8 `#539` y `#545` (24-sep, tarde) — `PACK_LS` ya no esconde los packs vivos; y en la C5, el wire perdía las líneas de más de 2 KB

La tarea aparte de la mañana, en el worktree `friendly-dirac-ac6dfc`. **Reproducido en el PC antes de tocar**: la
cadena de la C5 (17 borrados y 2 vivos al final) por el verbo de verdad listaba `P00`…`P15`, todos borrados, con
`count` 19 y nada más. Y dos consumidores más leían los vivos de esa lista y fallaban igual: `wire_serie.py packdel`
y el aviso de `requires-pack=` del IDE al grabar. El arreglo (`2c2d416e`): `bpvm_pack_listar` (los vivos primero; en
el hueco, los borrados más recientes; el MISMO recorrido que `bpvm_pack_scan`, que no cambia de contrato) y
`PACK_LS` con `omitted`/`activeOmitted`, como el `omitted` del `LIST` (`#425`); `wire_serie.py` y el panel del IDE
lo dicen.

**En la C5, tras la ronda de placas**: el control con la imagen sellada (16 de 19, todos borrados) y, con la de
`#539`, `PACK_LS` MUDO: 60 s sin respuesta, una línea vacía, la placa viva. Era el transporte, `#545` (`1d1e25bd`):
`usb_serial_jtag_write_bytes` es todo o nada y su anillo mide 2048 B; la respuesta nueva, 2.062. Con la línea en
tramos, 0,64 s y los TRES vivos (la ronda grabó un `DriverSpi` nuevo sin borrar el viejo). `Gui` y `DriverSpi`, sin
regrabar; y la tanda de V7 en la C5, 11/11, la misma matriz que la sellada (el `GuiShot.shot`, byte a byte igual).
Detalle en las dos fichas.

**Lo que enseñó:**
- **Un transporte que se rinde en silencio**: la pista fue la línea VACÍA en la basura del cliente —el salto de línea
  sin su cuerpo—. Y el comentario del código decía lo contrario de lo que hace el driver («puede escribir menos de
  lo pedido»): se leyó el fuente del IDF antes de tocar nada.
- **Una respuesta que crece 30 B es una prueba de tamaño gratis**: `#539` no rompió el wire, destapó un tope que
  estaba desde el 31-ago y que nada de lo que se probaba alcanzaba (la salida de programas va en trozos de ≤ 512 B y
  el `GET` en 256).
- **Antes de dar una imagen ESP nueva se sabe, sin placa, si habrá que regrabar los packs sellados**: `nm` de los
  dos `.elf` — `_heap_start` y el fin del texto y del rodata en páginas de 64 KB de la MMU. **Confirmado en la C5**:
  mismas direcciones de zona, de bloque de RAM y de los dos motores. (Y la carga de los motores es perezosa: la
  dispara el primer `INFO` o el primer `RUN`, no el arranque.)
- **En un worktree**, las ESP32 se construyen con `idf.py set-target <chip>` y `idf.py build` (el `sdkconfig` no
  viaja; el de la C5 salió idéntico al de `main`); la Pico, con su `build/` dentro del worktree; del STM32, sólo los
  `.c` tocados con su toolchain y sus flags (sus `.project` son locales y apuntan al `src/` del principal). La C6
  lleva LVGL dentro y `third_party/lvgl` está ignorado: no viaja. Se compiló con un enlace (junction) al del
  principal, QUITADO al acabar, porque un borrado recursivo del worktree lo seguiría y vaciaría el original. Y un
  `set-target` que falla deja un `sdkconfig` con el target POR DEFECTO (`esp32`): el siguiente build sale para
  Xtensa y no enlaza (`usb_serial_jtag_*` no existe allí). Hay que borrar `build/` y `sdkconfig` y repetir.
- **`FICHAS.md` se guarda en CRLF desde el 24-sep por la mañana** (hasta el 23-sep, LF como todo el repo): un CR
  suelto (una línea en blanco escrita CR CR LF) hace que git lo tome por binario y no lo normalice, y así
  `bf72ad9a` lo pasó ENTERO a CRLF; la tarde metió tres más. Quitados todos al fusionar (`git ls-files --eol`:
  `crlf`, ya no `-text`) y añadido sin conversión (`git -c core.autocrlf=false add`). Volver a LF (`git add
  --renormalize`) es un commit aparte, cuando no haya ramas abiertas que toquen `FICHAS`. Y el `sed -i` de Git Bash
  pasa a LF el fichero entero: `FICHAS` se edita sin `sed`.

## ✅ H2 (24-sep, tarde) — H2 CERRADO: `#523` cerrada, y sus dos colaterales, `#546` y `#547`, eran bugs de verdad

Eduardo preguntó cómo estaba H2. Quedaba `#523` sola, y sus seis criterios se cumplían (la mezcla, hoy 194/194 en las
dos VMs, como el 17-sep); su «Queda» estaba desfasado salvo tres cosas, y las tres eran reales. `montar-zip.sh` decía
`MOD_ACTUAL="MOD7"`: habría parado el paquete de V7 (`1e1ba380`). Y los dos «colaterales sin ficha» del 17-sep,
probados con una sonda, resultaron bugs silenciosos e idénticos en las dos VMs: **un `switch` con sujeto de 8 B no casaba
NUNCA** (string, long: siempre al default) y **`field := 7` en una property long no la cambiaba y descuadraba la pila**.
Con Eduardo («OK, las 4»): `switch` admite entero, boolean, enum y string (por contenido); un real o un objeto, error con
nombre. Al arreglarlo salieron dos más en la misma función: un `break` dentro de un case salía del BUCLE (el manual dice
«del switch», y así lo hacían el AOT y `#540`), y sin bucle el emisor fallaba sin línea. Todo en `f39f81d7`, con
el AOT de un switch en una native (sujeto string sobre `string_eq`), el censo con el compilador de antes y el de ahora
(562 de 590 `.mod` idénticos; sólo cambian los tres samples nuevos) y **una red que no lo veía**: la paridad compara las
dos VMs entre sí, y fallaban igual. Desde hoy una línea `FALLO` es rojo en `check_parity` aunque coincidan, y los samples
nuevos se comprueban solos. Y al probar un `continue` dentro de un switch en un `for`, un cuarto, preexistente y más
gordo: **`continue` en un `for` no avanzaba la variable** —bucle infinito, en el intérprete y en el AOT—; nadie lo pisaba
(2 `continue` en el árbol, ninguno en un `for`): `#548`, `132f956f`. **Con esto H2 queda cerrado.**

## 💾 H3 (24-sep, noche) — SD.3 hecho, y SD.4 destapa cinco fichas en fila (`#549`–`#553`)

Eduardo: *«C6 la a. Nos concentramos en ir cerrando H3, H4, H5 y H6; antes de empezar nuevos hitos. Ahora continuamos
con H3, el lector de SD común.»* **SD.3** (`e1843e99`): la Metro registra su SD desde el ENV con el ayudante común
(`bpvm_sd_spi_env.c`) y `pico/repl_v1.c` pierde sus dos verbos y el vigía (−270 líneas); el P4 da la identidad de su
SDMMC y registra `SDMMC`; la familia del `main` común registra por `CHIP_SD_REGISTRAR()` en `chip_cfg.h` —la C5 su TF,
la S3 y la C3 «no tiene ranura», la C6 el porqué de esperar a `H6.b`—. Compilaron las ocho y `compat` verde.

**SD.4, en la Metro y la C5.** El control antes de grabar —la Metro con la imagen de antes, `wire_serie.py sd`— dijo
«el pin 34 no existe en rp2350a», y de ahí salieron cinco, cada una destapada por la anterior:
`#549` (la Metro con la tabla de la A desde H4 P1: `main()` la elegía antes de leer la variante), `#550` (la serie,
negativa), `#551` (SD.2 daba por terminada la escritura con la tarjeta aún grabando: la tarjeta de mentira de
`test-sd-turnos` era más amable que la de verdad), `#552` (la C5 a 25 MHz tuerce bits: `khz:` en la línea `sd`) y
`#553` (los datos de un bloque, de una vez: `SdCard` de 29,7 a 3,8 s en la C5). El camino de `#551` es el que conviene
recordar: la capa de disco callaba el porqué; primero se hizo hablar (el motivo del dispositivo y su traza al log), y
luego la traza de la fase de datos dijo `00 … 00` —la tarjeta seguía grabando—; el control con la TF de antes cerró la
duda. Resultado: `SdCard` «SD OK — los seis» en las dos placas, y la TF y la pantalla de la C5 a la vez
(`C5SdGui`, `607663a3`): 30 ficheros escritos y releídos sin un byte distinto mientras la etiqueta seguía cambiando (63 ms
de peor hueco; sola, 30). Y `#554`, abierta: concatenar en placa es unas 250 veces más lento que en el PC.

## 🖥️ H6 `G4.b` (25-sep, mañana, sesión remota) — el DMA en la cintura SPI del ESP32: la pantalla de la C5, de 147 a 70 ms

Con el P4 hecho, lo siguiente acordado con Eduardo: `G4.b`, que se prueba entero en la C5 —se regraba por USB aunque su
firmware no arrancara; con la Metro, a distancia, mejor no arriesgar—. El diseño, en `V7_IDEAS` §H6.11.11 antes de
programar: `DriverSpi` ya mandaba la franja entera desde `G4.a3`, así que el pack no se toca y todo va en la cintura
(`gpio_esp32.c`): el bus con DMA, lo de hasta 4 bytes en los búferes de la transacción, polling hasta 64 e interrupción
por encima. Medido antes y después en la C5: `GuiFlushBench` 147 → 70 ms (la estimación de §7.11 eran ~45: sin medir,
el resto será pintar en PSRAM y el rebote), `SdCard` 4,2 → 3,8 s, `C5SdGui` y `C5Bus` bien (`c6e0f87a`). **Falta verlo
en el cristal.** ⚠️ Al rodata de la C5 le quedan ~4 KB: la próxima imagen que crezca correrá su zona de packs y habrá que
regrabar `Gui` y `DriverSpi` (como hoy el P4).

## 🧩 H4 `#536` (25-sep, sesión remota) — `__npShow`: el array validado y el tope, la zona de rascar entera

La tercera de la lista de la mañana, de las pequeñas que se prueban en el PC. Eduardo decidió el tope («la zona
entera»: 2048 LEDs en el PC, RP2350 y ESP32; 4096 en STM32) y lo demás salió de `#534`: el array se valida con el
mismo `bus_array`/`busArray`, así que los textos son los de los builtins de bus en las dos VMs. `samples/NpShowTope.bp`
en el corpus (paridad 101/101) y su control: con el código de antes, las dos VMs daban los MISMOS cuatro `FALLO` —la
paridad sola no lo habría visto—. Tras tocar miVM, la cadena de siempre (miVM → `lexer-java` clean install → BpIde):
el IDE lleva la miVM de hoy (`b96c06ca`).

## ⚙️ H5 (25-sep, sesión remota) — el AOT sin FPU, en la C5: dos fallos por delante (`#555`, `#556`) y `sumaHasta` de 124 a 1 ms

Lo acordado con Eduardo: el resto de `#502`, el destino `ilp32` de la C3, la C6 y la C5. Leyendo antes de tocar, el
destino YA estaba (`RISCV32_IMC`, de H6, y el IDE lo elige por la receta del INFO); lo que nunca se había hecho era
EJECUTAR un `.mdn` de un programa en una placa sin FPU. `AotSingle` (la herramienta de línea de comandos del Run del
IDE) compiló `NatV7` para `riscv-imc` a la primera, y la C5 dijo «memoria del buffer insuficiente». Acotado en el PC:
el `.mod` sin AOT enlaza y el fundido no («símbolo no resuelto ''»), para `arm` igual → **`#555`**: `conBloqueNativo`
no conocía MOD9 y escribía una cabecera MOD7; el AOT fundido, roto en todas las placas desde el 17-sep sin que ninguna
red lo viera (`fb652b6a`, con su prueba y la de MOD8 arreglada: comparaba campos corridos). Con eso, la C5 registró
sus 4 thunks… y se reinició al primero → **`#556`**: MEMPROT deja el heap sin ejecución; el P4 lo había resuelto en
V4 y el ajuste no viajó a sus hermanas RISC-V (`810d0a77`: fuera en C5, C3 y C6, regenerados y comparados). Resultado:
`NatV7` nativo en la C5 (124 → 1 ms), en el P4 y en la Metro, con los cuatro valores exactos. La C3 y la C6,
compiladas y sin probar.

## 🧪 H5 (25-sep, tarde, sesión remota) — EL AOT EN LA TANDA, y lo que destapó: el `float` sin FPU (`#559`), la FMA (`#560`), el `print` (`#558`, `#561`), las globales (`#562`) y el EXITED que no salía (`#557`)

Eduardo, a distancia: *«meter AOT en la tanda de placas»* (conectadas la Metro, la C5 y el P4). `42ae4bbf`: una prueba
con «aot» sube su `.mod` con el nativo fundido para la receta de cada placa (`AotSingle`, el camino del Run del IDE), y
el EXITED dice cuántos thunks se usaron (`nativos`) — sin eso, una native que no corre nativa da los mismos números y la
prueba pasaría igual. La primera vuelta cazó dos cosas: el `print` de una native adelantaba al del programa (`#558`,
`7a897d7b`) y **`NatNew` reiniciaba la C5**. Sin el mensaje del pánico (va a la UART0), el rastro se escribió en BP:
`NatPaso`, un paso por native con un `sleep` detrás, se paraba en el sexto, `sumaFloats`. El `.o` pedía `__addsf3` y el
enlace lo ponía en la dirección 0. El censo de catorce natives dio el alcance real —7, 7 y 16 símbolos sin resolver en
ARM, P4 y C5: el `double` mezclado y `long`↔`float`, rotos en TODAS las placas— y el diseño salió del criterio de
Eduardo de `#426` («si el micro lo soporta por hardware, por hardware; si no, por software»): helpers del firmware y
macros que deciden por la receta. Meter libgcc en el `.mdn` se descartó midiendo: la de los toolchains es `medlow`
(`__clz_tab` en absoluto, el cuelgue de `#440`). Diseñándolo apareció `#560` (gcc fundía `a*b+c` en una FMA con FPU;
el intérprete redondea dos veces) y la prueba nueva del PC (`make test-aotfloat`: el C del AOT con la FPU y forzando
los helpers) cazó `#561` (el `print` de un float con `%g`). En placa, `FloatNat` no llegaba a tener nativo: **`#562`**,
la caché `static` del CS del módulo —ningún módulo con globales había tenido nunca código nativo en una placa—. Y
`#557`, de la revisión de la mañana: un `errorMessage` largo se llevaba el EXITED entero por delante.

**Resultado en las tres placas**: `FloatNat` IDÉNTICO con 23/23 thunks (la C5 por los helpers; el P4 y la Metro por la
instrucción, y el caso `fma` da 0), `NatNew` 7/7 en la C5, la tanda de V7 entera verde (42 resultados) y `SqlDemo` en la
Metro y el P4 con el pack regrabado (`SQLite.mdn cargado del pack (3184 B, 16 thunks)`).

## 📦 H6 `#468` (25-sep, tarde, sesión remota) — la stdlib EN FLASH: el instalador cede a la zona de packs

Eduardo: *«¿De H6 queda algo pendiente?»* → H6.b (tras la ronda sellada), la pantalla de la C5 tras `G4.b`, el doble
búfer (a decidir) y `#468`, que es lo que eligió. Leyendo antes de tocar, el `MODULE_QUERY` del plan ya estaba (el
`STAT` por nombre de `#466` y la zona en el resolvedor y en el IDE desde `#493`); lo medido en la C5 decía el resto:
`Gui`, `Json`, `Collections` y `Str` salían del pack XIP y `Core` de `/lib` («el FS eclipsa al del pack»), porque el
INSTALADOR de fábrica repone sus copias en cada arranque y `/lib` va delante. Diseño en `V7_IDEAS` §H6.13; Eduardo
aprobó las tres partes (la zona antes que la stdlib; el instalador no instala lo que la zona sirve igual y quita su
copia; `/lib` al día al tocar un pack) y **canceló** la vía de cargar la stdlib desde la imagen. Y avisó, a mitad del
trabajo: *«si el usuario borra el pack se queda sin la librería»* — se contestó con el diseño (la imagen sigue siendo la
fuente) y se probó en la Metro: `packdel SQLite` → `Core` de vuelta a `/lib` en el acto. Verificarlo destapó dos más,
en el build de packs: el pack de la GUI llevaba un `Core.mod` de generación 0 (**`#563`**: la receta no copiaba el
canónico; ahora el build lo rechaza con nombre) y el cierre transitivo de los packs no leía MOD9 (**`#564`**).

## 🔌 H3/H5/H6 (25-sep, tarde de placas) — LAS NUEVE CON LA ABI 8: la ronda rehecha, y la C3 y el P4 destapan `#565` y `#566`

Eduardo, con las placas delante: *«Dime qué necesitas»*. El plan —grabar las seis que no estaban conectadas con la
imagen de la ABI 8, pasar la tanda de V7 entera en las nueve como punto de restauración nuevo, y lo que pide manos— salió
así, por tandas según se iban conectando:

- **Pico 2, S3 y DK2**, grabadas (BOOTSEL por el wire, `app-flash`, ST-Link) y la tanda: 12/12, 9/9 (la S3 es Xtensa: sin
  AOT) y 14/14.
- **C3 y Nucleo**: la Nucleo, 12/12; la C3, las tres de AOT colgadas y la placa reiniciándose (`panic/exception`) →
  **`#565`**: su SRAM tiene dos direcciones y el cargador saltaba por la de datos. Con la traducción del IDF en la
  cintura, 12/12, con 7/7, 4/4 y 23/23 thunks nativos: era lo que faltaba para dar por probado el destino `ilp32`.
- **C6, C5 y P4**: la C6, grabada, 13/14 (el AOT a la primera; `GuiWinJson`, «No space in heap», la excepción conocida);
  la C5, 14/14; el P4, 14/14. Y con el OK de Eduardo, el `PACK_FORMAT` del P4 y SQLite, Gui (el de `#563`) y DriverNulo
  regrabados: 1,4 MB libres (antes, 8 KB) y `Core` XIP del pack de la GUI (antes, «el FS eclipsa al del pack»); 14/14.
- **La C6 y `#468`**: el `Stdlib.pack` grabado sacó de `/lib` lo de la imagen, pero `Gui`, `Json`, `Collections` y `Str`
  los había subido la tanda y seguían tapando al pack. Borrados (OK de Eduardo), `GuiWinJson` IDÉNTICO y la tanda 14/14:
  el objetivo de `#468`, en la placa que la motivó.
- **La C5, en el cristal** (`G4.b`): `C5Touch` —el contador sube en pantalla al pulsar, 5 de 5 también por el puerto— y
  `GuiFlushBench`, 70 ms por pantalla y 1 ms la etiqueta.
- **SD.4 en el P4**: la tarjeta metida con la placa en marcha no montaba (`NOT_FOUND`) → **`#566`**: un intento fallido no
  soltaba el host. Arreglado y probado con las manos de Eduardo: sin tarjeta, el reintento da `TIMEOUT`; metida, monta
  sin reiniciar; `SdCard` «los seis»; la tanda, 14/14.
- **La Metro**: 12/12, y la tarjeta en caliente: RETIRADA y METIDA por el vigía del común, `SdCard` «los seis». Eduardo
  la sacó una segunda vez, a media escritura: error con nombre (`CMD24 -> FF…`), desmontaje limpio y montaje al volver.
- **La Nucleo**: su zona de packs da «cadena ROTA, 0 libres». Sus primeros bytes (`0x08182000`) son un `Core.mod` MOD7:
  restos de otra época de la flash —el FS de hoy no puede salirse de su partición (`block_count` sale de su tamaño)—. No
  admitía packs hasta un `PACK_FORMAT`: lo hizo Eduardo esa misma tarde, y la zona quedó vacía y sana (504 KB libres).

Las ocho imágenes probadas, bit a bit, en `dist/firmware-v7-25sep` (README + SHA256), y el informe en
`compat/informes/tanda_v7_25sep.md`. Las capturas de `GuiShot` (C5, C6, P4, DK2), miradas: bien; en las pantallas de 240
de ancho el deslizador del ejemplo tapa la casilla, porque su ancho es fijo.

## 🖥️ H6.b en la C6 (25-sep, noche) — LA C6 SIN LVGL EN LA IMAGEN: `GuiMini`, `#568` y, con la TF, `H3` cerrado

Eduardo: *«continuamos»*, con el orden del traspaso. Los tres pasos de `V7_IDEAS` §H6.14.6, cada uno cambiando una cosa:

1. **La línea base** (`f0412363`): `GuiFlushBench` no medía —LVGL sólo repinta al vencer su refresco (33 ms) y el ejemplo
   metía diez `__guiRunOnce` seguidos; en la C5 salía bien por casualidad—. Arreglado: la C6 de antes, 29 ms por pantalla
   y 2,8 por etiqueta, y `GuiRotCycle` bien en el cristal (Eduardo).
2. **El pack pequeño, `GuiMini`** (`b719359a`): cuatro perillas del mismo LVGL + `gui.c` y el pool; el pack escribe
   ahora el pico de su pool al acabar cada programa (13 KB en la tanda, 54 con 255 etiquetas). Por el camino, **`#568`**:
   la tabla de widgets llena no decía nada; ahora «Gui: no caben mas widgets: esta GUI tiene sitio para N», con
   `test-guilleno` en `check-testc` y el sabotaje cazado. Probado primero en la C5 (`packRam` de 512 a 1024 con el OK de
   Eduardo: el `packdel` deja la RAM de hueco hasta formatear), y la C5 vuelve a su `Gui` grande.
3. **La C6** (`7dbbc2a7`): la imagen como la de la C5 (−626 KB; 104 KB más de heap al arrancar), las cuatro líneas del
   ENV y los packs `Stdlib`, `DriverSpi` y `GuiMini` (su zona formateada, OK de Eduardo). Dos cosas no salieron como se
   esperaba: **la RAM** —con 96 KB de pool la VM bajaba a 100 KB y `SdCard` no cabía; el margen de 103 824 B de la C6
   se atribuía a LVGL y, medido sin ella, el sistema sigue pidiendo ≈ 94 KB: el pool baja a 64 y la VM vuelve a 128—
   y **la velocidad**, mejor de lo previsto: 35 ms por pantalla, no 52 (la franja del pack va en SRAM interna, en la C5
   en PSRAM). La tanda 14/14, `GuiRotCycle` bien en el cristal (Eduardo), y **la TF de la C6 por fin en uso**: `SdCard`
   «los seis» y `C5SdGui` RESULT OK (27 ficheros con la pantalla a la vez, 0 distintos) → **`H3` cerrado**.

## ⚙️ H5 (26-sep, mañana) — `#542`, `#567`, tres fichas de paso y P2b: la native se suspende COMO UNA INTERRUPCIÓN (idea de Eduardo), hecho en el PC

- **`#542`** (`99136adb`): el AOT resuelve los imports como el compilador y uno que falta es un error con su nombre.
  Destapó que 6 pruebas del AOT traducían sin `Core`. **Y casi rompe el IDE**: la primera versión dejaba sin AOT el Run
  de un proyecto en cuanto una native importaba otro módulo de la app (el C va a `target/` y el AOT buscaba ahí), con
  `compat` verde: `AotBuild` no está en ninguna red. Salió al comprobar una frase de la ficha («en el IDE, un fallo del
  build»: era falsa) y se probó a mano con un proyecto de dos módulos. Memoria nueva: *endurecer un contrato: cuenta
  los llamadores*.
- **`#567`** (`b9315223`): `mhz_max 62` en `st7789.seq` y el rechazo con nombre; el primer mensaje no cabía en
  `s_motivo` (86 de 79) y salía cortado — la prueba mira ahora hasta el final.
- **El análisis de `P2b`** comparó, uno a uno, los helpers tipados del AOT con sus builtins y destapó **`#569`**
  (`charAt` fuera de rango: `""` con el AOT, `RuntimeError` interpretado) y **`#570`** (un tamaño de array negativo
  MATABA el programa en la VM-C, donde miVM lanza: nueve caminos, censados por la primitiva), en `41adacb8`, con
  `test-aotstr` y `ArrayNegativo` en el corpus.
- **Eduardo decidió cómo se suspende una native**: primero, *«la función native llame a una función opCode y ahí se
  suspende»* y la línea en los bucles; y luego, *«¿y si el código nativo funcionase como una interrupción? Sin un stack
  propio, usa el stack que le ceda el intérprete»*. Descartadas anidar el planificador (los hilos sólo vuelven en orden
  inverso) y la máquina de estados en el emisor. Memoria nueva.
- **P2b.0**, medido en la Pico: la línea del bucle con un contador en registro, +2 ciclos por vuelta; mirar un
  indicador en cada vuelta, +7. El nativo sigue 134 y 96 veces más rápido que interpretado. El primer intento midió
  mal (un byte de la zona reservada que en la Pico no vale 0) y se vio porque el número era absurdo.
- **`#571`**, visto al leer el código para P2b.1: anidar natives (native → BP → native) perdía el límite de fallos; si
  la exterior lanzaba después, el proceso moría en silencio. Reproducido y arreglado aparte (`99b02ddb`,
  `test-aotanida`), porque vale ya en todas las placas.
- **P2b.1, a-d, en el PC** (`1507d734`, `4b1414cc`, `be011deb`, `21bacb85`): el cambio de contexto x86-64 —con los
  límites de pila del TIB: sin ellos, el `longjmp` de MinGW muere con `STATUS_BAD_STACK`—, la pila prestada, suspender
  y reanudar, y la línea del bucle con el helper `punto_control` (**ABI 9**). Cada paso con su prueba en `check-testc` y
  sus sabotajes; el más elocuente, el del GC: sin escanear las natives suspendidas, la cadena de la native se reciclaba
  y su hueco lo reusaba la basura de otro hilo, con `status=OK`.
- De paso, el arnés `fixture` dejaba correr una prueba con el `.mod` RANCIO si el `.bp` no compilaba (`3f71c5b6`).

`compat.sh check` verde en cada paso (al final: paridad 103/103 en MOD9 y MOD8, pruebas de C 17/17); compilan la C6, la
Pico y la Nucleo.

## ⚙️ H5 (26-sep, media mañana) — P2b.2, la lista a mano con UNA implementación (la opción 3 de Eduardo) y `#573`

- **P2b.2** (`5f0cc4f1`): el AOT llama a cualquier intrínseca `CALL_BUILTIN n` por el despachador del intérprete
  (`call_builtin`, ABI 9) —`Math.sin`, `sqrt`, `startsWith`, `sleep`…—. La prueba del `sleep` no cazaba el sabotaje
  «sin suspender»: el hilo se dormía después, en el primer builtin ya interpretado, y la salida salía igual; ahora la
  native lee tras el `sleep` una global que sólo pone el otro hilo. Destapó `#572` (identificadores de BP que chocan en
  el C del AOT).
- **La lista a mano, medida** (5 millones de `charCodeAt` en un bucle nativo, en el PC): 44 ms por el helper tipado, 96
  por el despachador, 257 interpretado. Eduardo: *«la 3»* —se quedan los helpers tipados, pero cada uno llama a la MISMA
  función que su builtin—. Hecho en `4bfdb1a8`: `bpvm_prims.h`, seis primitivas y sus entradas. Un fallo DENTRO de una
  primitiva ya no lo ve `test-aotstr` (el AOT y el intérprete se mueven juntos): lo caza la paridad contra miVM, y
  `test-aotstr` vigila las entradas. Los dos lados, saboteados; uno salió verde cuando debía ser rojo —el builtin
  `SUBSTRING` con `start + 1`— porque `NatStr` no tenía ningún `substring` con inicio positivo: comprobación nueva.
- **La pila, MEDIDA** con la orden del firmware de la Pico y `-fstack-usage` (la nota del switch de builtins, aplicada):
  el primer intento dejaba en cada `case` de `NEWARRAY` un búfer vivo a la vez que el de `BPVM_RT_THROW`, y el marco de
  TODO el intérprete pasó de 248 a 336 B. Con `BPVM_RT_THROW_TEXTO` (`fdfb2398`), 256. El diff y las pruebas no lo veían.
- **`#573`**, abierta y cerrada (`de04b689`): al mirar lo que quedaba fuera de la lista, los helpers de los OPERADORES de
  arrays lanzaban su propio texto («array_load_i32: index out of bounds» donde el intérprete y miVM dicen «ALOAD: índice
  fuera de rango 7 (length=3)») y el concat sin memoria devolvía un `null` callado. La misma receta
  (`bpvm_prim_texto_indice`), `test-aotarr` (`NatArr`, 21: 20 distintas antes, iguales después) y `NatArr` en la
  paridad. Otra vez la pila: con el búfer en `elem_de`, cada `a[i]` de una native bajaba 112 B más y hacía una llamada
  más (a `-Os` gcc no la inlineaba) → el error en una función fría y `elem_de` siempre en línea: el marco de antes.
- Visto de paso, sin tocar: el AOT no traduce las conversiones a tipos estrechos (`byte(x)`, `word(x)`, `int8(x)`,
  `int16(x)`) dentro de una native —error duro de compilación, que es lo que se quiere de una `native`, pero con un texto
  que dice «función desconocida» y lista cinco builtins, desfasado desde P2b.2—.

`compat.sh check` verde en cada paso (al final: paridad 104/104 en MOD9 y MOD8, pruebas de C 19/19); compilan la Pico,
la C6, la P4 y la Nucleo.

## 🔌 H4 (26-sep, tarde) — el NeoPixel de la familia ESP32, `#574` y el ADC del STM32: H4 hecho en el código

- **El análisis antes** (`V7_IDEAS` §H4.5, `1041bf8b`) y las decisiones de Eduardo: el RMT del núcleo del IDF (no
  `led_strip`, que se descarga del registro); validar el pin del NeoPixel en el común y separar «no implementado» de «no
  se pudo iniciar»; los pines del wire, también del sistema. Su idea de un SPI por software, como MicroPython: mirado en
  su documentación, su `neopixel` va por `machine.bitstream`, que en la ESP32 usa el RMT; un SPI por software no da los
  ±150 ns del WS2812, y uno por HARDWARE sí —la forma de dar NeoPixel al STM32—: apuntada.
- **La placa del NeoPixel**: Eduardo, mirando: en la C3 y la C6 no hay LED RGB a la vista (el esquemático de la C6 lo
  pone en el GPIO8); la S3 sí. Hecho en `6a53f91f` y probado en la S3 (grabada con su OK): el primer `RUN` dijo «falta
  el módulo 'Neopixel'» —la stdlib embebida de la familia no lo llevaba, se añadió por el generador—; después, rojo,
  verde y azul, y otra vez tras un `Gpio.Pin`; y con una prueba de color, sólo el 48: la S3 es una DevKitC-1 v1.0.
- **`#574`**, visto al analizar: la tabla de la ESP32 daba por buenos los pines de la flash —en la C6 el GPIO25 es su
  MISO—. Ahora quedan fuera los que el IDF reserva al arrancar y los del cable del IDE, que dice cada transporte; en la S3,
  `gpioCount` de 45 a 36. La C6 no se grabó: lleva `DriverSpi` y `GuiMini` sellados y no sabíamos qué commit tiene.
- **El ADC del STM32** (`422a55dc`): el mapa, sacado con `pypdf` de las hojas de datos que bajó Eduardo; el backend del
  ADC1 a 12 bits; y en la Nucleo, con el divisor al 50 % que montó en A0: **1,646 V** (esperaba 1,65). A0 = PA3 no salió
  de un manual: salió de leer los 17 canales. De paso, `#574` en el STM32 (PA9/PA10) y la DK2 bien en la tabla (LQFP144,
  3 ADC).
- La pila, medida otra vez con los flags de la Pico: sin cambio. Y dos trampas de siempre que volvieron a morder:
  `sed -i` quitó los CRLF de `interp.c` (sin daño: git normaliza) y un heredoc se comió las continuaciones de una receta
  del Makefile (rehecha con un script escrito con Write).

`compat.sh check` verde (paridad 104/104 en MOD9 y MOD8); compilan las cinco ESP, la Pico, la Nucleo y la DK2.

## 🖥️ H6 (26-sep, tarde) — `G5.a`: el P4 sin LVGL en la imagen, probado en la placa

- Eduardo: *«Yo continuaría por H6 y dejamos H5 para el final»*. De H6 quedaba `G5` (el P4 y la DK2). **El análisis**
  (`V7_IDEAS` §H6.15, `5fcea785`): el P4 lo tiene todo —packs nativos, el pack de LVGL de su ISA, un driver que ya hace
  lo de `'PANT'`—; la DK2 no: el STM32 no tiene packs nativos (ni BIOS ni cargador de `npk`) ni LVGL como pack ARM.
  Decidido: el P4 primero; **packs nativos para la Discovery**; el `PACK_FORMAT` del P4 al acabar.
- **`G5.a`** (`1a48870a`): `gui_display_dsi.c` con dos modos por el build; con `BPVM_GUI_PACK`, la tabla `'PANT'`
  publicada desde la imagen con `bpvm_bios_publica`. El giro —un panel DPI no gira por hardware— con las cuentas de
  LVGL 9.2.2 portadas tal cual (la zona, los píxeles, el punto del táctil). La imagen, −644 KB y −368 KB de `.bss`.
- **En la placa**, con el OK de Eduardo a cada paso: la zona de packs se corrió (la app es más pequeña) y los tres
  packs dijeron «realojado para OTRA dirección», como se esperaba; `PACK_FORMAT`, SQLite y la Gui regrabados. La tanda
  de V7 **14/14**, `GuiFlushBench` **25 ms** por pantalla y 2 ms por etiqueta, el giro bien en el cristal (Eduardo) y el
  táctil 5 de 5 y en las cuatro orientaciones (`GuiRotDemo`). Un descuido: no medí la velocidad del LVGL de la imagen
  ANTES de grabar; la imagen de `dist` permite medirla si hace falta.

## 🔩 H6 (26-sep, tarde, 3) — `G5.b0`: los packs nativos en el STM32, hechos y probados en la DK2 (y `#575` por el camino)

- El análisis (`V7_IDEAS` §H6.15.4, `3914352c`) y las decisiones de Eduardo (`4f15beca`): **la A** —las STM32 a
  `softfp`; su pregunta, «¿que funcionen por soft pero si hay hardware lo aproveche?», es justo `softfp`: la convención
  de `soft` y la FPU por dentro—, el bloque de RAM «decide tú», el borrado por banco sí, y la DK2 («en las pruebas
  finales se prueban todas»).
- **Paso 1** (`ab87ece6`): las dos `.cproject` a `softfp`; `floatbench` igual (2,523 → 2,522 s interpretado, 0,04 s
  nativo) y la tanda 14/14 en la DK2.
- **Paso 2** (`ec0a48b2`): la BIOS y el cargador del STM32 (calcados de la Pico), la VM LO PRIMERO de la RAM (la
  sección `.bp_vm` en los dos `.ld`) con el bloque `packRam=` delante —dirección fija, `0x20000000`—, la RAM en
  `PACK_BURN_BEGIN`, la BIOS y el ancla comprobadas en cada arranque y `stm32_flash_erase` partido por banco.
- **`#575`** (`848ded39`), al preparar SQLite: sus `.mdn` y el de Gui2 seguían en la ABI 8 con la VM en la 9 desde
  P2b.1.d (sin natives en toda placa con imagen nueva), y el AOT no podía regenerarlos —`#542` contaba `import v1 from
  pack "SQLI"` como un módulo que falta—. Arreglado, regenerados (cambia un byte, la ABI), los packs reconstruidos y una
  red nueva, `packs-abi`, en `compat.sh check` (verde entera).
- **En la DK2**, con el OK de Eduardo a cada lote: la zona de datos repartida de nuevo (FS 1008 KB, packs 1536 KB: la de
  antes, 528, no llegaba para la Gui), `PACK_FORMAT`, `packRam=512`, el `mini` —sus pasos, todos OK, vistos con
  `log=1`, quitado después— y SQLite con **`SqlDemo` entero**; la tanda 14/14 otra vez.
- **Paso 4** (`17dbf2d2`): LVGL como pack ARM, 660 KB, el oráculo del realojado idéntico a `ld`; el pack de la Gui,
  744 KB podado para ARM.

## 🖥️ H6 (26-sep, tarde, 4) — `G5.b`: la DK2 sin LVGL en la imagen, y con giro; `G5` hecho

- El análisis (`V7_IDEAS` §H6.15.6, `68fddb9c`), con la velocidad de hoy medida ANTES de tocar nada (la lección de
  `G5.a`): 37 ms por pantalla entera y 2 por etiqueta. Eduardo: el giro ya, la memoria medida y las franjas a medir.
- El código (`568e976a`): la DK2 publica su LTDC y el GT911 como `'PANT'` desde la imagen, con el GIRO que no tenía;
  `gui.c` guardado contra `BPVM_GUI_PACK`; LVGL fuera del `.cproject`; −627 KB de flash, −179 KB de `.bss`, la VM a
  1792 KB.
- En la placa, con el OK de Eduardo: la imagen, `packRam=768` y el pack de la Gui para ARM (su RAM, justo encima de
  la de SQLite). La primera tanda saltó las pruebas de GUI —el INFO no decía la pantalla sin el pack cargado—, y se
  arregló como en la ESP32 (`bba97e0b`). Luego: la tanda 14/14, 48 ms por pantalla y 1 por etiqueta, el giro bien en
  el cristal, `C5Touch` 5 de 5 y `GuiRotDemo` en las cuatro orientaciones (Eduardo). **`G5` hecho.**

## 🔗 H7 (27-sep, tarde) — `#554` cerrada: concatenar en placa, de 2122 a 44 ms

- La causa (el `__strconcat` sintetizado copiaba byte a byte en bytecode), la concatenación de N de una vez (idea de
  Eduardo) y el análisis a fondo de los literales como entradas estáticas de la tabla de handles (`V7_IDEAS` §H7);
  Eduardo eligió el camino **C**: el builtin `__strCopy` (263), que acepta la cadena en sus dos formas (`e5761dd1`).
- Con su OK, la Metro, la DK2, el P4 y la Nucleo grabadas con esa imagen; su INFO ya dice `"mdnAbi": 9` (`#576` en
  placa); la ronda de V7 verde en las cuatro, con `CatCadena` y `CatCrono`. En la Metro, el bucle de `#554`, de 2122
  a 44 ms. Del análisis salieron `#587` (el `StringBuilder`), `#588` (las globales del AOT) y `#589` (el depurador).

## 🎯 AOT (27-sep, tarde) — `#588` cerrada: las globales del módulo, desde el compilador

- La native calculaba la posición de cada global por su cuenta y, con una global string, long o array por
  delante, leía y ESCRIBÍA otra palabra (`NatGlobales`: `suma` 705033164 en vez de 23; la escritura se comía las
  cadenas). Ahora se la pregunta al compilador (`MivmEmitter.offsetsDeGlobales`, el mismo reparto del `.mod`).
  `make test-aotglobales` 7/7 (roja antes). El AOT lleva las clases de miVM en su `classpath`.

## 🧵 H7 (27-sep, tarde) — `#587` cerrada: el `StringBuilder`, piezas y una caché de caracteres

- El diseño de Eduardo: las PIEZAS en un `string[]` y `toString()` de una vez (la suma de los bytes, una reserva y
  un `__strCopy` por pieza, como la concatenación de `#554`). Para `appendChar` —`Json` y `Str` construyen carácter
  a carácter— una caché de 64 que se vacía como una pieza más (Eduardo: *«un array de longitud fija […] hasta que
  llega un appendInt o appendStr y entonces aprovechas y vacías la caché»*). `ec5b15c7`.
- Medido en la Metro con el `Core` nuevo en `/app`: 192 `appendStr` de 94 a 19 ms. La primera versión llevaba la
  cuenta de caracteres y el tope de la caché por `len`, y `appendChar` EMPEORABA (93 ms donde el viejo daba 78);
  sin las dos cosas, 74-75. Lo cazó la medida en la placa.
- Por el camino, la trampa conocida: el script que reescribía la clase cortó en el `end StringBuilder` del
  CONSTRUCTOR (se casa por sangría: `'\n  end StringBuilder\n'`); con `git checkout` y rehecho.
- La stdlib regenerada (los 29 módulos: cambia la interfaz de `Core`), `Stdlib.pack` y la stdlib embebida; `compat.sh
  check` verde (paridad 107/107 con `SbPiezas`). **Las imágenes nuevas están compiladas y NO grabadas**: las cuatro
  placas de hoy siguen con la de `e5761dd1`, cuyo `StringBuilder` viejo tiene la misma interfaz pública.

## 🐞 H8 (27-sep, tarde) — `#589` cerrada: el depurador de miVM ya lee las cadenas del heap; y dos fichas nuevas

- Reproducido antes de tocar nada, por el wire (un cliente que para en un breakpoint y pregunta como el IDE): una
  cadena del heap salía `"@40000004"`, un literal `""literal""` (las comillas dos veces), las properties string
  `null` (la vista leía la palabra ALTA de la referencia: la generación) y `READ_STRING` con comillas, al revés que
  la VM-C. `b777f25f`: la indirección (`leeCadenaDeRef`, espejo del `READ_STRING` de la VM-C), UN render de
  locales para la VM-Java y para una placa —había dos copias y la de Java se separó—, la palabra baja en las
  properties; y en el IDE, la cadena vacía de una placa ya no sale `@…`.
- La red: `DepuradorCadenasTest` (lexer-java) ejecuta el depurador de miVM de verdad, por el socket; saboteada
  (el miVM de antes), ROJA. No había ninguna.
- Por el camino, dos fallos anteriores, con ficha: **`#590`** —`public property saludo: string := "hola"` vale
  `null` en las dos VMs; el compilador sólo avisa; decisión de Eduardo— y **`#591`** —un breakpoint en la última
  línea de un módulo para dentro de `__strconcat`, que hereda esa línea en el `.dbg`—. La prueba lo esquiva con
  una sentencia de más, y lo dice.

## 🧵 H7 (27-sep, tarde) — la guarda del SMP: con `io`, el planificador SMP ya no toca el transporte

- `scheduler_smp.c` llamaba al `poll_cb` viejo aunque el hilo `io` estuviera en marcha (la guarda `vm->io == NULL`
  de `scheduler.c` no había viajado: la auditoría de capas, A3); latente, porque el SMP es opt-in y en el PC el
  `poll_cb` es NULL. Primero la prueba: `test_kill.c` con tres fases más —`io` arrancado, el KILL por su
  `ops.poll` como en una placa y el `poll_cb` viejo contando—; ROJA en las dos del SMP (177 921 llamadas entre
  cuantos y 7 durmiendo) y verde el control (`bpvm_run`, 0). Con la guarda, 0 en las seis. `6f249614`.
- `test-kill` no estaba en ninguna red: ahora en `check-testc` (25), con su línea «OK:».

## 🧮 (27-sep, tarde) — `#590`: una global siempre vale lo que dice su declaración (el modelo de Eduardo)

- Las decisiones de Eduardo del lote: `#590` como un `var` de módulo, `#494` UNA EXCEPCIÓN (falta el cómo), `#356`
  cerrada por superada, `#531` a analizar antes de tocar nada. Y al ver el manual, el modelo: *«las variables
  globales van en el espacio de constantes del módulo […] si se indica, el valor viene ya grabado en el módulo»*;
  lo que no se puede grabar, en `__init`; un array literal, grabado y copiado al cargar.
- El censo destapó más: un `var` estático tiraba su valor EN SILENCIO, y `samples/ModInit.bp` llevaba roto desde el
  21-sep (sus arrays de módulo se ignoraban; fuera de toda red). `dd2e06e5`: un criterio para las cuatro clases de
  global, el literal horneado y lo demás en `__globales` (al final del módulo, llamada por `__init` tras los imports
  y antes de la inicializadora); la stdlib, igual byte a byte. `GlobalesIni` (26) y `ModInit`, a la paridad.

## ⏭️ Traspaso (27-sep, tarde) — pausa: `#554` hecho en el PC; falta la placa

**Commits desde el traspaso de mediodía** (sin push): `7f2c60bd` (`#576`) · `617ab23d` (H5 cerrado) · `b377da40` ·
`989c2fc2` (el análisis de `#554`) · `e5761dd1` (`#554`, el camino C) · y los docs.

**`#554`**: la causa, medida —el `__strconcat` sintetizado copiaba byte a byte en bytecode—; la concatenación de N de
una vez (idea de Eduardo); el análisis a fondo de su propuesta de los literales como entradas estáticas de la tabla de
handles (`V7_IDEAS` §H7: con el GC de hoy sólo vale una entrada por literal que dura lo que el RUN, y pide opcode y
formato nuevos); y el camino que eligió, **C**: el builtin `__strCopy` (263), que acepta la cadena en sus dos formas.
Hecho y verde en el PC (`compat.sh check`: paridad 106/106 en MOD9 y MOD8, pruebas de C 23/23); compilan las ocho
imágenes con la stdlib nueva dentro. **Falta la placa**: grabar la imagen y medir `CatCrono` (la Metro, la de la
medida original: con la versión de `move` del mismo diseño, 2122 → 45 ms el bucle).

**⚠️ El riesgo que acecha**: lo que compila el compilador de hoy (el IDE incluido, reconstruido) usa el builtin 263.
En una placa con la imagen de antes, un programa nuevo falla en la primera concatenación de cadenas («builtin no
soportado») hasta que se regraba. **Desde la tarde, la Metro, la DK2, el P4 y la Nucleo ya la tienen** (`#554`
cerrada); la S3, la C3, la C5 y la C6, todavía no (`#581`). Lo compilado ANTES sigue funcionando en cualquier imagen
(su helper copiaba byte a byte, que vale para todo).

**Las placas**, como en el traspaso de mediodía (la Nucleo con `packRam=256`, que se queda).

**Para repasar a la vuelta** (Eduardo: *«a la vuelta repasamos lo que hay que hacer»*): grabar las placas con la
imagen de `e5761dd1` (con OK) y cerrar `#554` en placa; lo demás de H7 (la lista de su fila en FICHAS); las fichas
nuevas del análisis (`#587` el `StringBuilder` con el diseño de Eduardo, `#588` las globales del AOT —silencioso—,
`#589` el depurador de miVM); y las sueltas `#577`–`#586`.

## ⏭️ Traspaso (27-sep, mediodía) — H5 CERRADO

**Commits de hoy** (sin push): `b2a55303` (P2b.3 rediseñado: la native en un hilo de la plataforma) · `0b997502`
`138e8626` (P2b.3 a y b: el ejecutor y el testigo; las natives en ejecutores) · `f8146e4d` (las natives DIRECTAS) ·
`abb21836` `9f7227f5` (c: el HardFault que habla, los fallos fatales al registro) · `b9b3f0ac` (llamar a un pack no
cede; la pila del ejecutor a 8 KB; los packs en `.mdn` v3) · `a76375c6` (`#572`) · `5efb4c76` (las conversiones
estrechas en una native, la opción A de Eduardo) · `7f2c60bd` (`#576`: el INFO con la ABI de helpers) · y los docs
(`1670702d` `9c4d3a8c` `93a605ed` `998d05e7` `7c9a97f5` y el de este traspaso).

**H5, cerrado** (Eduardo: *«Haz la 5, crea una entrada para cada una del resto y las pones como pendientes sueltas. Y
después cerramos H5»*). Lo que quedaba en su fila está en diez fichas SUELTAS, cada una con qué falta y qué decidió
Eduardo: `#577` (tuplas en una native), `#578` (`try`/`catch` en una native), `#579` (la paridad y el camino AOT:
propuesta de cerrarla con la evidencia), `#580` (parar una native recursiva sin bucles; un `KILL` en placa), `#581`
(P2b.3.d en las ESP), `#582` (la línea de un error), `#583` (el debugger), `#584` (la cola de la fusión), `#585`
(`double` medido y en el manual) y `#586` (la documentación del AOT, desfasada tras P2b).

**Las placas**:
- **La DK2 (COM12), la Metro (COM4) y el P4 (COM15)**: la imagen de P2b.3 (los ejecutores de 8 KB y los packs en v3:
  SQLite y Gui, directas), con el log apagado. Su INFO aún no manda `mdnAbi`: la imagen es de antes de `#576`.
- **La Nucleo (COM5)**: la misma imagen (`softfp`), y en su ENV **`packRam=256`** —la VM en 256 KB— con el `mini` y
  SQLite grabados, para la prueba del adaptador de `G5.b0`; el log, como estaba (`log=1`). **Se queda así**
  (Eduardo: *«puedes dejar lo que hay, no importa»*).
- **La S3, la C3, la C5 y la C6**: sin tocar hoy, con la ABI 8 (`#581`).

**⚠️ El riesgo que acecha**: las ESP con la ABI 8 rechazan todo nativo nuevo y corren interpretado hasta `#581`
(desde `#576`, el IDE lo dirá en cuanto conecte una placa con la imagen nueva; con las de antes, no lo sabe); y la
Nucleo tiene la VM en 256 KB por su `packRam` (a propósito: un programa que no quepa, es por eso).

**Queda de V7**: los hitos H7 (memoria, arranque y arquitectura), H8 (IDE y wire), H9 (pruebas y medida), H10
(placas y drivers), H11 (la C5) y H12 (los dos núcleos, al final), y las fichas sueltas `#577`–`#586` (FICHAS).

## 🏁 H5 (27-sep, mediodía) — `#576` hecha y H5 cerrado

- `#576` (`7f2c60bd`): el INFO publica `mdnAbi` —la ABI de helpers de la imagen; 0 sin receta—. El IDE la compara
  con la que genera y, si no casa, lo dice en vez de compilar un nativo que la placa rechazaría (así se tapó `#504`
  tres semanas); `mdnIsStale` tampoco sube un `.mdn` suelto de otra ABI; la tanda la apunta y explica con ella un
  «SIN NATIVO». `sim_smoke` (en `check-sim`) lo vigila. Las ocho imágenes compilan; las placas lo verán con su
  próxima grabación.
- Lo que quedaba en la fila de H5, en diez fichas sueltas (`#577`–`#586`), y **H5 cerrado**.

## 🏷️ H5 (27-sep, mediodía) — `#572`: los nombres de BP con su prefijo en el C del AOT

- `#572` cerrada (`a76375c6`): el emisor pone `bp_` delante de todo identificador de BP (parámetros, locales, el
  iterador de un `for`, el `this` de un método) en UN sitio, `cVar()`. Una native con variables llamadas `vm`,
  `mem`, `cs`, `int`, `static`, `bool`… ya se traduce; antes gcc no compilaba el módulo y corría interpretada.
  De paso, un caso mudo en el puente a un pack (un parámetro `r` se pasaba sin valor). `make test-aotnombres`, en
  `check-testc`, roja con el emisor de antes. El frontend y el IDE, reconstruidos.
- Las conversiones estrechas en una native (`byte(x)`…): de las dos formas —A, la comprobación en el C del módulo
  con los helpers que hay, sin tocar la ABI; B, un helper nuevo con la misma función que el opcode, que sube la ABI
  a 10 y obliga a regrabar todo—, Eduardo elige A: *«nos permite más flexibilidad, si se quiere cambiar algo
  solamente hace falta modificar el compilador. También es verdad que genera más código, pero siempre lo podemos
  cambiar más adelante»*. **Hecho** (`5efb4c76`): la comprobación en el C del módulo, con el texto del
  intérprete; `make test-aotestrecha` 23/23 (roja con el emisor de antes) y `NatEstrecha` en la paridad; y el texto
  rancio de «función desconocida». La ronda de V7 gana `NatNombres` y `NatEstrecha`.
  En placa, sin imagen nueva (la ABI no cambia): `NatNombres` (4/4 thunks nativos) y `NatEstrecha` (7/7) IDÉNTICOS al PC en la DK2, la Metro, el P4 y la Nucleo (`compat/informes/tanda_estrecha_27sep.md`).

## 🔌 H5 (27-sep, tarde) — P2b.3.d: la sesión de placas, primera parte (la DK2, la Metro, el P4 y la Nucleo)

- `P2b.3.c` (`abb21836`, `9f7227f5`): el HardFault habla en las dos familias Cortex-M33 (con STKOF, la tarea y su
  pila); los fallos fatales, al registro aunque el log esté apagado; la ronda de V7 gana las seis pruebas de natives.
- En las placas, con el OK de Eduardo en cada grabación: la DK2, la Metro (por BOOTSEL desde el wire) y el P4, la ronda
  de V7 entera (20/20, 18/18, 20/20), con las natives que ceden IDÉNTICAS al PC. Medido: el testigo, 17–36 µs por
  llamada; la pila del ejecutor, 3,2–3,5 KB en las pruebas y hasta 5,5 KB con SQLite.
- De ahí, decidido por Eduardo (`b9b3f0ac`): llamar a un pack no cede (SQLite y Gui2, directas: sin ejecutor) y la pila
  del ejecutor a 8 KB; los `.mdn` de los packs regenerados en v3, los packs reconstruidos y regrabados (`PACK_FORMAT` en
  el P4 y la DK2), y las tres placas otra vez: la ronda igual, y SQLite ya sin ejecutor.
- De paso: el ADC de la DK2, el NeoPixel de la Metro, SQLite en la Metro y SQLite y la Gui en el P4. Y la pregunta de
  Eduardo por los 32 MB del P4: la caché mapea 16 (el revert `2dc12d5b`); es de H7.
- La Nucleo (COM5), con el OK de Eduardo: la imagen de HEAD (`softfp`) arranca a la primera; la ronda 18/18
  (`FloatNat` 23/23 nativo), el testigo ~31 µs, la pila del ejecutor 3296 B. Y el adaptador de `G5.b0`, que esperaba
  placa: `packRam=256` en su ENV (la VM baja de 512 a 256 KB), el `mini` y SQLite grabados (en la zona quedan 24 KB
  de 504), los dos motores cargados y `SqlDemo` entero sin ejecutor. **`packRam=256` se queda en su ENV** hasta que
  Eduardo diga (quitarlo devuelve la VM a 512 KB; los packs se quedan grabados, sin cargarse) —dicho el mismo día:
  se queda—. De paso, un `put` sin
  `MSYS_NO_PATHCONV` dejó en la placa `/C:/Program Files/Git/app/SqlDemo.mod`: borrado con un `DEL` del wire.

## 🧵 H5 (27-sep) — P2b.3: la native en un HILO de la plataforma (idea de Eduardo), y las natives directas

- El análisis del cambio de contexto en ARM y RISC-V destapó lo que de verdad costaba: cambiar de pila DENTRO de una
  tarea choca con las vigilancias de pila de cada sistema (el chequeo de desborde de FreeRTOS en ARM, el vigilante
  por hardware del IDF en las ESP32 RISC-V —el de `#543`—, el área de la FPU del P4, la PSRAM). Eduardo: *«FreeRtos
  hace un cambio de contexto cuando cambia de Thread»* y *«pensar en la ejecución de código nativo como un Thread BP,
  con su propio stack… Ese es un mecanismo que ya existe»*. Rediseñado (`b2a55303`): la native corre en un EJECUTOR
  (un hilo de la plataforma) y ella y el intérprete se pasan el testigo. Y Eduardo: la pila de un hilo de FreeRTOS
  puede ser pequeña → se mide con la marca de agua.
- `P2b.3.a` (`0b997502`): el semáforo binario y el hilo del ejecutor en las cuatro plataformas; `test-ejecutor`;
  2,3 µs por paso del testigo en el PC. De paso, la copia de la Pico no ponía a cero su estructura de hilo.
- `P2b.3.b` (`138e8626`): las natives en ejecutores, el GC en el worker, fuera el ensamblador x86; `compat.sh check`
  verde; 5,2 µs por llamada a una native diminuta (0,10 antes).
- Las natives DIRECTAS (`f8146e4d`), decididas por Eduardo al ver la medida: sin puntos de cesión (transitivo por
  las del módulo: su pregunta) corren sin ejecutor; `.mdn` v3; la diminuta, 0,05 µs. `test-aotdirecta` y
  `test-mdnv3`, con sus sabotajes.
- Todo compila en la Pico, las dos STM32 y las cinco ESP32; nada grabado todavía.

## ⏭️ Traspaso (26-sep, fin del día) — H4 y H6 CERRADOS (Eduardo); mañana, H5: «Mañana intentamos terminar H5»

**Hoy**: H4 cerrado (el NeoPixel de la ESP32, `#574`, el ADC del STM32); H6 cerrado (`G5.a` el P4, `G5.b0` los packs
nativos en el STM32 con las STM32 a `softfp`, `G5.b` la DK2 con giro); y de paso `#573` y `#575` (los `.mdn` de los
packs en la ABI 8, con red nueva: `packs-abi`). Todo en `main`, sin push; el árbol, limpio.

**H5, lo que queda** (FICHAS, fila de H5), en este orden:
1. **`P2b.3`**: el cambio de contexto de una native en ARM (Cortex-M33) y en RISC-V (el P4, `ilp32f`; la C3, la C5 y la
   C6, `ilp32`) —en el PC está hecho (`P2b.1`, x86-64)— y la cascada: host → placas. Hasta entonces, en placa, un `kill`
   a una native LANZA por el límite de fallos en vez de suspenderla.
2. **La sesión de placas de `P2b.3`**, que recoge lo que se ha ido dejando: grabar las imágenes con la ABI 9 en las que
   siguen en la 8 (la Metro, la S3, la C3, la C5, la C6 y la Nucleo —esta, además, con `softfp` y el adaptador de
   `G5.b0`: `packRam=` en su ENV si se quieren motores—), la C6 con `#574` (`samples/PinesSistema.bp`), el ADC de la DK2,
   el NeoPixel de la Metro con el `__npInit` nuevo, DriverSpi regrabado en la C5 y la C6, y **regrabar SQLite y la Gui
   en el P4** (siguen los de la ABI 8: SQLite y Gui2 no funcionan ahí hasta entonces).
3. **`#572`**: identificadores de BP que chocan en el C del AOT.
4. **Las conversiones estrechas** en una native (`byte(9)`…), y su texto de error rancio.

**Las placas hoy**: la **DK2** con la imagen de `G5.b` (`softfp`, sin LVGL, `'PANT'` desde la imagen), `packRam=768`
y en su zona el `mini`, SQLite y la Gui; el **P4** con la imagen de `G5.a` y los packs viejos; las demás, como las dejó
la tarde de placas del 25-sep (ABI 8).

**Riesgos**: (1) toda placa con imagen de hoy en adelante necesita los packs reconstruidos tras `#575` —la red
`packs-abi` vigila los del PC, no los ya grabados—; (2) la Nucleo no se ha grabado con `softfp` ni con el adaptador:
compila, pero no se ha visto arrancar; (3) el disparo de la carga de packs desde el INFO (`bba97e0b`) es nuevo en el
STM32: un pack que colgara lo haría al primer INFO del IDE, no en el arranque (como en la ESP32).

## ⏭️ Traspaso (26-sep, tarde, 4) — H6: `G5` hecho (el P4 y la DK2 sin LVGL en la imagen)

**Commits** (sin push): `568e976a` (`G5.b`), `bba97e0b` (el INFO carga los packs), y los docs (`68fddb9c` y el de
este traspaso).

**Las placas**: la **DK2** con la imagen de `G5.b` (sin LVGL, `'PANT'` desde la imagen), `packRam=768` y en su zona
el `mini`, SQLite y la Gui (quedan 312 KB libres); en `/app`, las de la tanda y `GuiRotCycle`, `C5Touch`,
`GuiRotDemo`, `SqlDemo` y `MiniRun`. El **P4**, como en `G5.a`, con los packs de SQLite y la Gui de antes de `#575`
(ABI 8): regrabarlos. La **Nucleo**, sin tocar: su imagen (`softfp`, el adaptador) va a las pruebas finales.

**Queda**:
- **H6 CERRADO** (Eduardo, 26-sep), como H4 esta tarde: de sus tres objetivos (H4, H5 y H6) queda H5.
- Lo de antes: P2b.3 y su sesión de placas (la C6 con `#574`, el ADC de la DK2, el NeoPixel de la Metro, DriverSpi
  en la C5/C6, las imágenes con `#569`–`#573` y los packs de SQLite y la Gui en el P4), `#572`, las conversiones
  estrechas y la documentación de las pantallas; y **H5**, que Eduardo dejó para el final.

## ⏭️ Traspaso (26-sep, tarde, 3) — H6: `G5.b0` hecho; queda `G5.b` (la DK2 sin LVGL en la imagen)

**Commits** (sin push): `ab87ece6` (softfp), `ec0a48b2` (paso 2), `848ded39` (`#575`), `17dbf2d2` (paso 4), y los docs
(`3914352c`, `4f15beca`, `898216d7`, `aa174255` y el de este traspaso).

**Las placas**: la **DK2** con la imagen de `G5.b0` (`softfp`, el adaptador; LVGL todavía en la imagen), la zona de
datos repartida (FS 1008 KB, packs 1536 KB), `packRam=512` en el ENV y en la zona el `mini` y SQLite; en `/app`,
`MiniRun`, `SqlDemo` y las de la tanda. El **P4** sigue con los packs de SQLite y la Gui de antes de `#575` (ABI 8):
regrabarlos en la sesión de placas de P2b.3. La **Nucleo**, sin tocar: su imagen (`softfp` y el adaptador) va a las
pruebas finales.

**Queda**, en este orden:
- **`G5.b`**, analizar primero (como `G5.a`): la DK2 —`'PANT'` por LTDC (`stm32/port/gui_display_ltdc.c`) y su táctil
  (el I2C 2), publicados desde la imagen, y la imagen sin LVGL (`BPVM_GUI_PACK` y `gui_pack.c`, como el P4)—. El pack de
  la Gui para ARM ya existe y cabe en la zona; sus 383 KB de estáticos con `packRam=512` dejarían ~122 KB de arena a
  SQLite: el reparto de la RAM se decide ahí.
- Lo de antes: P2b.3 y su sesión de placas (la C6 con `#574`, el ADC de la DK2, el NeoPixel de la Metro, DriverSpi en
  la C5/C6, las imágenes con `#569`–`#573` y ahora los packs de SQLite y la Gui en el P4), `#572`, las conversiones
  estrechas y la documentación de las pantallas.

## ⏭️ Traspaso (26-sep, tarde, 2) — H6: `G5.a` hecho; queda `G5.b` (la DK2) con `G5.b0` delante

**Commits** (sin push): `1a48870a` (`G5.a`) · y los docs (`5fcea785`, `ef290544`, `d439c7fa`…).

**Las placas**: el **P4** con la imagen de `G5.a` (sin LVGL), SQLite y la Gui (`lvgl.npk.RISCV`) regrabados, sin
`DriverNulo`; en `/app` las pruebas de pantalla (`GuiFlushBench`, `GuiRotCycle`, `C5Touch`, `GuiRotDemo`). La **DK2**
conectada (COM12) con su imagen de antes. La S3 y la Nucleo, como las dejó el traspaso anterior.

**Queda**, en este orden:
- **`G5.b0`**: los packs nativos en el STM32 —la BIOS y el cargador de la familia, como los de la RP2350 (`bios_pico.c`,
  `pack_pico.c`), y el sellado en `PACK_BURN_BEGIN`— y LVGL como pack ARM (`build_lvgl.sh`). Se prueba con SQLite en
  una STM32 (el `SqlDemo` de la Metro; el motor `ARMV8` ya está en `packs/SQLite.pack`).
- **`G5.b`**: la DK2 —`'PANT'` por LTDC y su táctil, y la imagen sin LVGL—.
- Lo de antes: P2b.3 y su sesión de placas, `#572`, las conversiones estrechas y la documentación de las pantallas.

## ⏭️ Traspaso (26-sep, tarde) — H4 CERRADO (Eduardo); la sesión de placas de P2b.3 recoge lo que falta probar

**Commits** (sin push): `6a53f91f` (NeoPixel ESP32 y `#574`) · `422a55dc` (el ADC del STM32) · y los docs (`1041bf8b`,
`10d4e5f9`, `08fce286`…).

**Las placas**: la **S3** con la imagen nueva (NeoPixel, `#574`) y, en `/app`, `NeoEsp.mod` y `NeoColor.mod` (éste, una
prueba de color; se quedan: Eduardo, «no hace falta borrar nada»); la **Nucleo** con la imagen nueva (el ADC) y
`/app/AdcNucleo.mod`, con el divisor de Eduardo en A0. La Pico 2 sin `NatLazo`. El resto, como estaban.

**⚠️ El riesgo que acecha**: el mismo de la ABI 9 (las placas con la 8 rechazan todo `.mdn` nuevo hasta P2b.3). Y la
S3 y la Nucleo llevan ya imágenes de la ABI 9.

**Queda**, en este orden:
- **P2b.3** y su sesión de placas, que recoge además lo que H4 no pudo probar: la C6 con `#574`
  (`samples/PinesSistema.bp`), el ADC de la DK2 y el NeoPixel de la Metro con el `__npInit` nuevo; y lo de antes
  (regrabar `DriverSpi` en la C5 y la C6, `H6.b` en el P4 y la DK2, las imágenes con `#569`–`#573`).
- **`#572`** y las conversiones a tipos estrechos en una native (y su texto de error desfasado).
- Sin placas: la documentación de usuario de la pantalla por packs y la guía de instalación desfasada.

## ⏭️ Traspaso (26-sep, media mañana) — P2b.2 hecho, la lista a mano con una implementación, `#573` cerrada

**Commits** (sin push): `5f0cc4f1` (P2b.2) · `4bfdb1a8` `fdfb2398` (la opción 3 y la pila) · `de04b689` (`#573`) · y los
docs.

**Las placas**: sin cambios desde la mañana, salvo que en la Pico ya no están `/app/NatLazo.mod` y `/app/NatLazo.mdn`
(la medida de P2b.0): borrados con el sí de Eduardo por `DEL` del wire, y comprobado con un `LIST`.

**⚠️ El riesgo que acecha**: el mismo — desde la ABI 9, las placas (con la 8) rechazan todo `.mdn` nuevo y la función
corre interpretada, hasta la cascada de P2b.3.

**Queda**, en este orden:
- **P2b.3**: el cambio de contexto de ARMv8-M con FPU, RV32 y RV32 con F; el tamaño de la pila prestada en cada placa
  (ojo a los helpers que reservan: ahora son dos marcos, `string_concat` 144 B) y el GC en la pila del intérprete; la
  cascada de la ABI 9 y la sesión de placas —con lo que ya espera placa: regrabar el `DriverSpi` en la C5 y la C6
  (`#567`), `H6.b` en el P4 y la DK2, y las imágenes con `#569`, `#570`, `#571`, la opción 3 y `#573`—.
- **`#572`**: que el emisor dé nombre propio en C a los identificadores de BP.
- Las conversiones a tipos estrechos en una native (y el texto desfasado de ese error): pequeño, en el emisor.
- Apuntado: una native recursiva sin bucles no pasa por la línea.
- Sin placas: la documentación de usuario de la pantalla por packs (el entregable de §7.9) y la guía de instalación
  desfasada (la C6 ya no trae LVGL en la imagen; la C5 no tiene sección).

## ⏭️ Traspaso (26-sep, mañana) — P2b.1 hecho en el PC: una native se suspende, se expropia y se para con un `kill`

**Commits** (sin push): `99136adb` `b9315223` (`#542`, `#567`) · `41adacb8` (`#569`, `#570`) · `99b02ddb` (`#571`) ·
`1507d734` `4b1414cc` `be011deb` `21bacb85` `c6937b50` (P2b.1) · `3f71c5b6` (el arnés) · y los docs.

**Las placas**: sin cambios, salvo dos ficheros de prueba en la Pico (`/app/NatLazo.mod` y `/app/NatLazo.mdn`, la
medida de P2b.0; se pueden borrar). Todas siguen con las imágenes de la ABI 8.

**⚠️ El riesgo que acecha**: desde `21bacb85`, todo `.mdn` que se genere —el IDE, la tanda con `aot`— es de la **ABI 9**,
y las placas, con la 8, lo **rechazan con nombre** y la función corre interpretada. La tanda con `aot` (`NatNew`,
`NatV7`, `FloatNat`) saldrá SIN NATIVO hasta que se regraben. No es un fallo: es la cascada de P2b.3.

**Queda**, en este orden:
- **P2b.2**: el helper `call_builtin` (el despachador de la VM), la marca de `builtin_throw` y el camino genérico del
  emisor para toda intrínseca `CALL_BUILTIN n`; y la decisión de la lista a mano (medir primero).
- **P2b.3**: el cambio de contexto de ARMv8-M con FPU, RV32 y RV32 con F; el tamaño de la pila prestada en cada placa y
  el GC en la pila del intérprete; la cascada de la ABI 9 (los `.mdn`, los packs SQLite y Gui, las imágenes) y la
  sesión de placas — con lo que ya espera placa: regrabar el `DriverSpi` en la C5 y la C6 (`#567`), `H6.b` en el P4 y
  la DK2, y las imágenes con `#569`, `#570` y `#571`.
- Apuntado: una native recursiva sin bucles no pasa por la línea.
- Sin placas, también: la documentación de usuario de la pantalla por packs (el entregable de §7.9), con la guía de
  instalación desfasada (la C6 ya no trae LVGL en la imagen; la C5 no tiene sección).

## ⏭️ Traspaso (25-sep, noche) — `H6.b` en la C6 hecho y `H3` cerrado

**Commits** (sin push): `f0412363` (la línea base y `GuiFlushBench`) · `b719359a` (`GuiMini` y `#568`) · `7dbbc2a7` (la C6
sin LVGL) · y los docs. `compat.sh check` verde (paridad 102/102 en MOD9 y en MOD8, pruebas de C 11/11 con
`test-guilleno`); compilan la C5, la C6 (ya sin LVGL), el P4 y la DK2.

**Las placas**: la **C6** con la imagen nueva —no está en `dist/`: `dist/firmware-v7-25sep` sigue siendo el punto de antes
de `H6.b`—, su ENV (`spi2`, `lcd`, `sd`, `packRam=136`) y los packs `Stdlib`, `DriverSpi` y `GuiMini`, con una tarjeta
dentro; la **C5** con su imagen nueva (`#568`), `packRam=1024` y su `Gui` grande activo (el viejo y `GuiMini`, borrados: su
RAM queda de hueco hasta un `PACK_FORMAT`).

**Queda**:
- `H6`: `H6.b` en el P4 y la DK2 (con `G5`: `'PANT'` desde la imagen, DSI y LTDC) y `#567` (el tope de `mhz`, en el `.seq`).
- `H5`: hacer `#542`, `P2b`, cronometrar nativo contra interpretado y el manual.
- `H4`: el NeoPixel de la C6 (RMT) y el ADC externo del STM32.
- Medir qué son los ≈ 94 KB de pico del sistema en la C6 (el PUT de 48 KB, las tareas que el IDF crea en marcha…): si
  baja, la VM o el pool pueden crecer.
- La documentación de usuario de `GuiMini` (qué pack grabar según la placa y cuánto `packRam` pide), con el entregable
  de §7.9 (plantilla + script + documentación).

## ⏭️ Traspaso (25-sep, tarde de placas) — la ronda de las nueve con la ABI 8, `#565` y `#566`

**Commits** (sin push): `1f98d61a` (`#565`) · `7798cb61` (`#566`) · y los docs, con el informe de la tanda y el README de
`dist/firmware-v7-25sep`. `compat.sh check` verde tras `#565` (paridad 102/102 en MOD9 y en MOD8, cápsula 59/59, norma
9/9, sim 6/6, pruebas de C 10/10, sombra 13/13); compilan las ocho imágenes.

**Corrección del traspaso de anoche**: decía que `H6.b` iba «tras la ronda sellada de las nueve placas», como si faltara.
Esa ronda se hizo el 24-sep (`22f30005`), y hoy se ha rehecho con la ABI 8: **`H6.b` ya tiene su punto de restauración**,
`dist/firmware-v7-25sep`.

**Las placas quedan**: las nueve con las imágenes de `dist/firmware-v7-25sep` (la C3 con `#565`, el P4 con `#566`); la C6
con el `Stdlib.pack` y su `/lib` sólo con lo de la imagen; el P4 con su zona recién formateada (SQLite, Gui y DriverNulo;
1,4 MB libres) y una SanDisk de 32 GB; la Nucleo con su zona formateada (vacía).

**Pendiente**:
- De Eduardo: ~~el doble búfer asíncrono~~ (decidido esa misma tarde: no urgente, como opción del usuario si hay
  memoria; `V7_IDEAS` §H6.14.8) y ~~los 80 MHz~~ (cancelados esa tarde: nos ceñimos a la hoja del controlador, 62,5 MHz en el ST7789); ~~`#542`~~ (decidida esa tarde: un import sin interfaz en el AOT es un error de carga,
  como en el compilador; queda hacerlo); y ~~una pregunta nueva de `#468`~~ (las copias de la stdlib en `/lib` que no pone la imagen tapan al
  pack): **decidido, nada de momento** —«desde el IDE se puede borrar fácilmente si hace falta… ya veremos si en la
  operación normal aparecen problemas»—; y tapar al pack sólo pasa cuando el principal no está en ese pack, y entonces
  es lo correcto.
- ~~Con manos: el explorador del IDE con `sd` y `sd mount` (H3)~~: hecho por Eduardo esa misma tarde, en la Metro
  (sacar la tarjeta y refrescar, meterla y refrescar: bien; en el log de la placa, el `ls` del IDE da 5 entradas en
  `/sd`, 0 tras «RETIRADA» y 5 otra vez tras «METIDA»). De H3 sólo queda la TF de la C6, detrás de `H6.b`.
- `H6.b` (quitar LVGL de C6, P4 y DK2), ya con el punto de restauración. **En la C6, analizado y decidido esa misma
  tarde** (`V7_IDEAS` §H6.14 y §H6.14.8: un segundo pack de LVGL para pantallas pequeñas —fuentes pequeñas, pool de
  64-96 KB, 256 nodos, un búfer—). **Lo siguiente, en este orden**: (1) la línea base —`GuiFlushBench` no mide la
  pantalla entera en la C6 de hoy («0 ms»): arreglarlo para que mida igual en los dos caminos, y medir—; (2) el pack
  pequeño, probado PRIMERO en la C5, que ya pinta por el pack (si falla, es el pack y no la imagen); (3) la imagen de
  la C6 sin LVGL, sus cuatro líneas de ENV (§14.3) y los packs; la tanda, `GuiRotCycle` en el cristal y la TF. El P4 y
  la DK2, después, con `G5`.
- Sin probar: el `.mdn` suelto del FS en la C3 (`#565` explica por qué debería ir).
- **`#567`, abierta y decidida**: el driver genérico rechaza un `mhz` por encima de la hoja del controlador
  (`mhz_max` en su `.seq`; 62 en el ST7789), con error con nombre y su caso en `test-driverspi`.
- **`#544`, cerrada** esa misma tarde: repetida con la imagen de hoy, `C6Boot` cuenta 5 pulsaciones de 5; lo del 24-sep
  no se reproduce y, por decisión de Eduardo, se cierra sin el control con la imagen de entonces.

## ⏭️ Traspaso (25-sep, noche, sesión remota) — H6: `#468` cerrada, y `#563` y `#564` de paso

**Commits** (sin push): `7398d6e5` (`#468`) · `534c294b` (`#564`) · `e28bedd2` (`#563`) · y los docs. Frontend 249/249; `compat.sh
check` verde con `test-mods` ya en `check-testc` (pruebas de C 10/10); compilan las siete imágenes y el simulador.

**Placas**: las tres conectadas llevan la imagen con `#468` y la tanda (MathRango,
GuiWinJson, GuiShot, NatNew, FloatNat) sale igual. La **Metro**: su `SQLite.pack` regrabado (el mismo), `Core` del pack.
La **C5**: `Core` del pack `DriverSpi`. El **P4**: sin cambios en `/lib` —su imagen no embebe `Gui`, `Json`,
`Collections` ni `Str` (esas copias de `/lib` no son de la imagen: las subieron el IDE o la tanda, y el instalador no
las toca) y el `Core` de su pack de la GUI es el de generación 0 (`#563`)—. **Pendiente del P4**: en su próximo `PACK_FORMAT` (le quedan 8 KB de zona), regrabar la
GUI con el pack de hoy (`bpstdlib/gui/out/Gui.pack`, ya con el `Core` bueno).

**Lo que queda de H6** (`FICHAS`, fila H6): `H6.b` —quitar LVGL de C6, P4 y DK2— tras la ronda sellada de las nueve
placas (necesita todas en la mesa); mirar el cristal de la C5 tras `G4.b`; y decidir si el doble búfer asíncrono y los
80 MHz entran en H6 o se cancelan.

## ⏭️ Traspaso (25-sep, tarde, sesión remota) — H5: la tanda con el AOT y seis fichas cerradas; las tres placas conectadas, al día

**Commits** (sin push): `42ae4bbf` (la tanda con el AOT) · `7a897d7b` (`#558`) · `8dfaea68` (`#561`) · `b78f449f`
(`#559` y `#560`: van juntos porque los `.mdn` regenerados llevan la ABI 8 y la receta nueva a la vez) · `6ac28e97`
(`#557`) · `b2c85ab2` (`#562`) · y los docs. Frontend 246/246; `compat.sh check` verde (paridad 102/102 en MOD9 y en
MOD8 con `FloatNat` en el corpus, cápsula 59/59, norma 9/9, sim 6/6, pruebas de C 9/9 con `test-aotfloat` y
`test-wirerecorte`, sombra 13/13); compilan las siete imágenes.

**Placas** — ⚠️ **con la ABI 8, una placa sin grabar RECHAZA todo `.mdn` nuevo** (con nombre: la native corre
interpretada), y el puente de SQLite de un pack viejo no carga: grabar imagen Y regrabar su `SQLite.pack` van juntos.
- **Metro** (COM4): la imagen de hoy (10:51) y su `SQLite.pack` regrabado; en su zona quedan cinco copias viejas BORRADAS
  (1,6 MB libres).
- **C5** (COM6): la imagen de hoy (10:51); `Gui` y `DriverSpi` sin tocar y cargando (misma página de 64 KB, como dijo
  `nm`: rodata en `0x4208f03c`, ~4 KB de margen). Su `Gui.pack` lleva el `Gui2.mdn` viejo, que en la C5 no se usaba
  (receta del P4).
- **P4** (COM15): la imagen de hoy (10:52), zona en `0x40154000` (rodata en `0x4014f80c`, ~2 KB de margen); `SQLite` y
  `Gui` regrabados. **Le quedan 8 KB libres** (las copias viejas, BORRADAS, ocupan hasta formatear): el próximo pack
  pide `PACK_FORMAT` y regrabar los tres.
- **Pico 2, C3, C6, S3, Nucleo y DK2**: sin grabar (desconectadas). La C3 y la C6 son las que faltan de ver con AOT
  (MEMPROT fuera, `#556`, y el `float` por los helpers, `#559`): con ellas, el destino `ilp32` queda probado entero.

**Queda de H5** (`FICHAS`, fila H5): `P2b` (el despachador genérico de builtins, que vuelve a subir la ABI: conviene
juntarlo con la próxima regeneración), `#542` (decisión de Eduardo), `#468`, cronometrar native frente a interpretado y
el manual. Y lo que pide manos, de H3/H6: la tarjeta en el P4 (SD.4), la tarjeta en caliente en la Metro, el explorador
del IDE con `sd`, y mirar el cristal de la C5 tras `G4.b`.

## ⏭️ Traspaso (25-sep, mañana, sesión remota) — el P4 sin tarjeta, hecho; sus packs, resellados

Eduardo, desde fuera: *«¿por dónde podemos continuar?»*. Conectadas: la Metro (COM4), la C5 (COM6) y **el P4 (COM15)**
—la Pico 2 ya no—. El P4 entra en SD.4 **sin tarjeta** (su arranque viejo decía `ESP_ERR_TIMEOUT` y su ranura no tiene
detector): grabado con la imagen de hoy y `5a3ed071` (registra siempre, monta con el FS), el arranque pasa por el común y
`SD_INFO`/`SD_MOUNT` contestan por primera vez (`medio SDMMC`, el `esp_err_t` como peldaño, sin traza). Al arrancar dio
`TIMEOUT` y en los reintentos `NOT_FOUND`: las dos son «no hay tarjeta», pero el reintento pasa por el host ya
inicializado —eso es justo lo que se probará al meter una y pedir `SD_MOUNT`—.

**Los packs del P4**: la imagen creció 5,6 KB y su rodata cruzó a la página de la zona (`nm`: acaba en `0x4014f69c`), así
que la zona se corrió de `0x40144000` a `0x40154000` y los tres se rechazaron con nombre, como estaba previsto. Sin sitio
para resellarlos (614 KB libres, 1,4 MB hacían falta), con el OK de Eduardo: `PACK_FORMAT` y SQLite, Gui y DriverNulo
regrabados desde los mismos ficheros (podados en seco: los mismos tamaños), todos `rc=0`; `SqlDemo` y `GuiPack` («las dos
vivas») bien.

## ⏭️ Traspaso (24-sep, medianoche) — H3: SD.3 hecho; SD.4 hecho en la Metro y la C5; faltan el P4 y la tarjeta en caliente

**Commits** (sin push): `e1843e99` (SD.3) · `73464cde` (`#549`) · `f82f246b` (`#550`) · `1d484f3c` (`#551`) · `004f7b04`
(`#552`) · `28caacde` (`#553`) · `607663a3` (`C5SdGui`, `sd`/`sdmount`) · y los docs. Sobre el código final, compilan
las ocho imágenes y `compat.sh check` sale verde (paridad 100/100 en MOD9 y en MOD8, cápsula 59/59, norma 9/9, sim 6/6
con `sd_smoke` en 20, pruebas de C 7/7, sombra 13/13).

**Placas**: la **Metro** (COM4) lleva la imagen de hoy entera (su ENV, el de siempre: `sd=sck:34,…,cd:40`); la **Pico 2**
(COM22), la de `#549` (sin `#550`–`#553`, que no le tocan: no tiene SD); la **C5** (COM6), la de hoy, con
**`sd=bus:2,cs:23,khz:10000`** nuevo en su ENV —sus packs `Gui` y `DriverSpi` siguieron cargando sin regrabar, como
predijo `nm`: mismas páginas de 64 KB—. El **P4** y las demás, la sellada.

**Queda de H3** (`FICHAS`, fila H3): **el P4** —conectarlo, grabarlo y pasarle `sd`, `sdmount` y `SdCard`: su
`p4_identidad` no ha corrido nunca; y de paso, que con el FS caído su `SD_INFO` diga eso y no el motivo por defecto
(«su ENV no lleva línea `sd`»): el P4 sólo registra con el FS arriba— y **la tarjeta en caliente en la Metro** (sacarla y meterla con la placa en marcha:
el vigía es ya el del común). Con eso, SD.4 y la fila de la SD se cierran; el explorador del IDE (`sd`, `sd mount`) lee
los mismos campos que antes más `medio`, y conviene mirarlo una vez con Eduardo.

**Lo siguiente**, dentro de H3–H6 (Eduardo, 24-sep): lo que quede de H3, y luego H4 (`#469` resto, NeoPixel C6, `#536`,
`#544`), H5 (`P2b`, `#542`) o H6 (`G4.b`, `H6.b`).

## ⏭️ Traspaso (24-sep, noche) — todo en `main`: H1 y H2 cerrados, y las dos ramas de la tarde, fusionadas

**`main` lo lleva todo** (sin push): H1 y H2 cerrados (`#540`; `#523` y `#546`–`#548`); `#539`/`#545` (`cd8831e2`) y
`#541` (`11996888`) fusionados a mano —sólo chocaban los docs, y ninguna línea de ninguno de los dos lados se perdió— y
verificados juntos: `compat.sh check` verde (paridad 100/100 en MOD9 y en MOD8, cápsula V6 59/59, norma, sim 5/5, pruebas
de C 6/6, sombra 13/13), frontend 246/246, `h13` 46/46, el IDE reconstruido. `test-aotswitch` usa ya el AOT de `#541`
(`aotmain`) y está en `check-testc` (`fd9e179e`). **Las dos ramas y sus worktrees, borrados** (Eduardo, 24-sep: `git
branch -d`, fusionadas); quedan sus dos carpetas VACÍAS en `miVM/.claude/worktrees/`, que Windows no deja quitar mientras
las sesiones que las tienen de directorio sigan abiertas — se borran al cerrarlas. Sin ramas abiertas: si se quiere,
`FICHAS.md` puede volver a LF como el resto del repo (`git add --renormalize`, un commit de sólo finales de línea); hoy
está en CRLF y sin CR sueltos. **FICHAS, limpia** (Eduardo: «si necesitas reordenar o limpiar fichas, hazlo»): en la tabla
de hitos, las cerradas que seguían sin tachar, tachadas (`#494`, `#477`) y los restos de las cerradas llamados «restos»
(`#502`, `#426`, `#473`); y arriba de ABIERTAS, lo abierto de verdad en una línea.

**Placas**: la C5 lleva la imagen con `#539`/`#545` (el código de hoy); las otras ocho, la sellada
(`dist/firmware-v7-24sep`): sin `#539`, y la C3 y la C6 sin el transporte de `#545` (una respuesta de más de 2 KB se les
sigue perdiendo hasta regrabarlas). Lo del compilador de la tarde (`#546`–`#548`) no toca el firmware.

**Abierto de hoy**: `#542` (el AOT sin la interfaz de `Core` genera código equivocado sin error; la propuesta de la ficha
—`--stdlibDir` y fallar con nombre—, decide Eduardo), `#544` (el BOOT de la C6: la prueba cruzada), `#468` (la C6 sin
memoria para `GuiWinJson`) y el NeoPixel de la familia ESP32 (el backend RMT).

**H3, la SD al común** (`V7_IDEAS` §H3.7): **SD.1** hecho (`b254d586`: el registro y `SD_INFO`/`SD_MOUNT` en el común, la
identidad de la tarjeta en el contrato de bloque, FAT por punteros, el micro simulado con ranura y `sd_smoke` en
`check-sim`) y **SD.2** hecho (`2d6dd838`: la TF suelta el bus en sus esperas; `test-sd-turnos` en `check-testc`, con su
control). La SD del STM32 no aplica (la DK2 no tiene ranura). **Quedan SD.3** —la Metro (fuera sus dos handlers de
`repl_v1.c`), el P4 (su `identidad` desde el `sdmmc_card_t`) y la C5 (FAT y el motor SPI en su build, `sd=bus:2,cs:23`)
registran su SD— **y SD.4**, en placa con tarjeta. **La C6, pendiente de Eduardo**: su pantalla va por `esp_lcd` a pelo
—inicializa el SPI2 con DMA y `ESP_ERROR_CHECK`, y el árbitro no la ve—; (a) tras `H6.b`, que la pasa al pack de la C5, o
(b) ya, con el cerrojo del bus del IDF en la cintura del ESP32.

**Lo siguiente**, a elegir con Eduardo: SD.3/SD.4, `G4.b` (el DMA del ESP32) o `H6.b`.

## ⏭️ Traspaso (24-sep, al final de la tarde) — `#539` y `#545` hechos y vistos en la C5; la rama, al día con `main`

**Estado de las placas**: la **C5 lleva la imagen de esta rama** (`#539` + `#545`), NO la sellada; las otras ocho,
las selladas. Para volver al punto de restauración, `dist/firmware-v7-24sep`.

**Lo que queda de `#545`**: la C3 y la C6 llevan el transporte viejo —compilan con el arreglo, sin grabar—: una
respuesta de más de 2 KB se les seguirá perdiendo hasta que se graben.

**La rama** `claude/friendly-dirac-ac6dfc`: al día con `main` (fusionada la tarde de placas), sin push. La de
`#541`/`#542` (`claude/quirky-khayyam-64df44`) sigue aparte: al fusionarla, `FICHAS` chocará en la tabla de hitos
(la fila de H9) y hay que hacerlo a mano.

## ⏭️ Traspaso (24-sep, tarde) — la ronda de V7, sellada; lo que dejó abierto la tarde

**Sellado**: `dist/firmware-v7-24sep` (8 imágenes, 9 placas, SHA256) y `compat/informes/tanda_v7.md`. `H6.b` puede
empezar cuando Eduardo quiera: hay a dónde volver.

**H1 y H2, cerrados** (ver arriba). **Lo que dejó abierto la tarde**: `#544` (el BOOT de la C6: la prueba cruzada, un pin de la cabecera a masa); `#468`,
con su medida (la C6 sin memoria para `GuiWinJson`); el NeoPixel de la familia ESP32 (el
backend RMT).

**Las otras dos sesiones**: `#539` (`PACK_LS`) y `#541`/`#542` (las pruebas de C en la red) están hechas en sus ramas
(`claude/friendly-dirac-ac6dfc`, `claude/quirky-khayyam-64df44`) y sin fusionar. Al fusionar, `FICHAS` chocará —los
tres tocamos el registro— y hay que hacerlo a mano. `#539` falta verla en la C5. Al fusionar `#541`, `make test-aotswitch` (`#546`)
entra en la lista de su `check-testc`.

**Lo siguiente**, a elegir con Eduardo: H3 «SD al común», `G4.b` (el DMA del ESP32) o `H6.b`.

**Estado de las placas**: las nueve con las imágenes selladas. Sin push.

## ⏭️ Traspaso (24-sep, mediodía) — la TARDE DE PLACAS: cerrar todo lo que se pueda con ellas

**El esqueleto: la ronda de V7 en las nueve placas.** Una placa cada vez —Eduardo conecta, yo grabo, «probada /
conecta»—: grabar la imagen de hoy, `python bpgenvm-c/tools/tanda.py --lista bpgenvm-c/tools/tanda_v7.json --puertos
auto --informe compat/informes/tanda_v7.md`, y lo propio de esa placa. Al acabar, **sellarla** como punto de
restauración (commit + informe + los binarios en `dist/firmware/`): es lo que `H6.b` exige antes de tocar nada, y el
cierre casi completo de mitad de V7. Cierra de paso **H1 P5** (grabar S3/C3/C6/Nucleo/DK2 y verlas == host), **H4 P4**
(cascada y tanda), la tanda por familia de **`#523`** y **la C5 en la tanda** como placa con pantalla (resto de H6.a).

**Lo propio de cada placa**, en el orden propuesto:
1. **Nucleo** — con el **BMP280** en el I2C2 (SDA PF0, SCL PF1, 3V3, GND; SDO a GND): `Bmp280RepStart_Nucleo.bp`
   cierra `G4.a3`. Y el **TOTAL del FS que «baila»** entre arranques (H10: tres arranques, mirando `INFO`) y la causa
   del reset con el **botón NRST** (`L15`).
2. **DK2** — la tanda con pantalla (800×480) y el FS que baila.
3. **C5** — grabar y **regrabar `Gui` y `DriverSpi`**, en ese orden (si la imagen cambió de tamaño, el sello los
   rechaza); la tanda con pantalla (240×320); **el botón de `C5Touch`**; y si da tiempo, el bloque de la VM sin PSRAM
   (U6.4, resto de H11).
4. **C6** — la tanda (240×240) y el **botón BOOT** con `C6Boot.bp` (H4 P3). El NeoPixel de la C6 todavía no: falta
   el backend RMT de la familia ESP32, que es código.
5. **C3** — la tanda, y `trytest.bp` y `JsonDemo` (`P1.C3`).
6. **S3** — la tanda.
7. **P4** — la tanda con pantalla (`SqlDemo` ya dio == host el 23-sep, con `G1`).
8. **Pico 2** — la tanda; **`#466`**: un `RESET` por el wire y el log tiene que decir «reinicio pedido»; y `U1.2`, que
   el post-mortem sobrevive al reinicio.
9. **Metro** — la tanda; **regrabar el pack de SQLite** (el `.mdn` pasó a ABI 7 con `#533`) y `SqlDemo` == host; la
   causa del reset con el **botón RUN** (`L15`); y si hay una tarjeta que se pueda borrar, la SD **«superfloppy»**
   (FAT32 sin MBR, resto de H2-P5).

**Lo que hace falta a mano**: el BMP280 y cuatro cables; una tarjeta SD que se pueda reformatear; y los dedos para
`C5Touch`, BOOT, NRST y RUN.

**Lo que NO es de esta tarde** (pide código o una decisión antes): `#540` (¿error o aviso?), `#536` (el tope de
NeoPixel), el NeoPixel de la familia ESP32, el ADC externo del STM32 (`#469`: hay que ver qué falta), `#495`, `#452`,
`#468`, `#531`, `G4.b`, H3 «SD al común» y `H6.b` (después de la ronda sellada).

**El rumbo (Eduardo, 24-sep)**: *«La V8 la dedicaremos a la WiFi y el Bluetooth principalmente. El modo de bajo
consumo también lo movemos a V8, todo lo demás de V7 se mantiene.»* `#490` sale de V7 (H10) y pasa entera al
bloque de V8 de `FICHAS`; nada más cambia de sitio.

**Tareas aparte, lanzadas y sin empezar**: `#539` (`PACK_LS` esconde los packs vivos) y los dos tests de C rotos
fuera de la red (`test-aotnew`, `test-debug-mt`).

**Estado de las placas**: la C5 con la imagen de `G4.a3`; las demás, con imágenes anteriores. Las de hoy (ocho builds:
la Pico y la Metro comparten imagen) están compiladas y sin grabar; la de la Pico, rehecha a las 12:27 con `#466`.
Sin push.

## ⏭️ Traspaso (24-sep, mañana) — `G4.a3` hecho; a la tarde, la Nucleo

**A la tarde (Eduardo)**: la **Nucleo** con el repeated start del STM32 —compilado, sin probar—: un BMP280 por
I2C con `bus.writeRead(0x76, [0xD0], 1, id, 1)` (0x58) y una `I2c.Transaction`. Grabarla con
`STM32_Programmer_CLI` (sn de la Nucleo en la memoria `build-stm32-headless`).

**Después**, a elegir con Eduardo: H3 «SD al común» (la TF montada por el firmware del ESP32, y soltando el bus en
sus esperas: la prueba de la concurrencia en placa, con dos hilos del SO) o `G4.b` (el DMA del ESP32).

**Estado de las placas**: la C5 con la imagen de `G4.a3` (BIOS v8) y en su zona `Gui` + `DriverSpi` v8 regrabados
(ojo: `wire_serie packs` enseña sólo los 16 primeros de 19 —`#539`—; los vivos están); `C5Touch` quedó en marcha.
Las otras siete imágenes, compiladas y sin grabar. Sin push.

## ⏭️ Traspaso (23-sep, noche) — `G4.a2` hecho; lo siguiente: el fondo de la GUI entre programas

**Hecho después, esa misma noche**: la pantalla se estrena NUEVA al terminar un programa (`b2bcd5cb`) y la
ROTACIÓN vuelve a 0° (`4f11b7fb`): los dos, vistos en la C5 por Eduardo. **Fusionadas** las ramas de `#537` y
`#534` (`622390b6`, `f037e050`), con check entero verde (paridad 94/94, el frente `sim`); y la interacción que
trajo `#534` (un turno por tramo) arreglada: la llamada entera de SPI es un turno. **Lo siguiente**, a elegir
con Eduardo: **`G4.a3`** (`Spi.Device` y el repeated start) o **H3 «SD al común»**,
que es lo que permite la prueba de verdad del árbitro: la TF y la pantalla a la vez en la C5 (la microSD ya
está puesta).

⚠️ Sin medir: la S3 y el P4 con el cierre nuevo del IDE. Las dos ramas fusionadas, borradas con sus worktrees
(Eduardo), y también el worktree `vigilant-hopper-d8260a` (la versión de `#534` con `bpvm_malloc` que Eduardo
paró para comparar: sobraba con `#534` fusionada; su trabajo sin commitear, copiado antes fuera del repo). Y las
dos viejas ya fusionadas (`claude/jolly-blackburn-9ec483`, `v5/object-raiz`): hoy sólo queda `main`.

**Estado de las placas**: la C5 con la imagen de `G4.a2` (`c5b701a8`, BIOS v7: SIN `#534` ni `e9824463`; grabar la
de `main` antes de probar buses en ella) y en su zona el `Gui` nuevo (pantalla y rotación limpias) y el `DriverSpi` v7
(en ese orden: el reparto de RAM es el sello, y un pack borrado de DEBAJO no deja su hueco al que se graba
después); el resto, compiladas y sin grabar. Sin push.

## ⏭️ Traspaso (23-sep, 19 h) — el táctil de la C5, cerrado; lo siguiente: `G4.a2` (el árbitro)

**Lo siguiente es `G4.a2`**, como estaba en el traspaso de mediodía: un mutex por bus en la HAL BP, un
dispositivo por CS con su velocidad y su modo en cada transacción, la BIOS v7 con «tomar / transferir /
soltar», y su prueba de dos hilos contra la cinta, con control.

El IDE ya no reinicia la C5 al desconectarse (ver la entrada de arriba). ⚠️ Sin medir: la S3 y el P4 con el
cierre nuevo (razonado: RTS primero no toca el EN de sus puentes).

**Estado de las placas**: la C5 con la imagen de `G4.a1` y el `DriverSpi` sin vigilante en su zona, en el REPL;
el resto sin grabar. El IDE, reconstruido (`BpIde-6.0.jar` de las 19:16). Sin push.

## ⏭️ Traspaso (23-sep, tarde) — el «cuelgue» era la herramienta; **prueba larga en marcha**; lo siguiente: `G4.a2`

**Lo primero al volver Eduardo**: la C5 corre `C5Touch` con la traza desde las 16:34:29, sin puerto.
**Primero pulsar** el botón (¿sube el contador?); **después** `wire_serie.py COM6 resetcable` y `log` —durante
un RUN el wire no atiende, así que para leer hay que pararlo—; lo de antes del reinicio queda en la RAM
superviviente (4 KB: los últimos latidos de 5 min, con el ESD). Si responde, la fila H6 de `FICHAS` se cierra
con eso; si no, el latido dice si el chip escaneaba.

⚠️ **Sin medir**: el IDE abre con DTR=1 y RTS=1 (la Pico necesita DTR) y al desconectar Windows los suelta:
una C5 con un programa lanzado desde el IDE se reiniciaría al cerrarlo. Mirarlo con el `uptime`.

**Después, `G4.a2`** (el árbitro), como estaba en el traspaso de mediodía.

**Estado de las placas**: la C5 con la imagen de `G4.a1` y el `DriverSpi` sin vigilante en su zona; el resto
sin grabar. Sin push.

## ⏭️ Traspaso (23-sep, mediodía) — `G4.a1` hecho; lo siguiente: `G4.a2` (el árbitro)

**Lo primero al volver Eduardo**, lo de antes: la C5 con `C5Touch` en marcha para mirar colores y pulsar
(cierra `G2` y `G3`). Su ENV ya está en el formato de `G4.a1` (`spi2=`, `i2c0=` y las líneas sin pines de
bus); el pack `DriverSpi` grabado es el nuevo.

**Lo siguiente es `G4.a2`**: el árbitro —un mutex por bus en la HAL BP, un dispositivo por CS con su
velocidad y su modo aplicados en cada transacción (hoy hay UN ajuste por bus: manda el último que abrió), la
BIOS v7 con «tomar / transferir / soltar»— y su prueba de dos hilos contra la cinta, con control. Después
`G4.a3` (`Spi.Device`) y `G4.b` (el DMA). ⚠️ Quedan tres drivers de IMAGEN fuera de la HAL BP (táctil del P4,
pantalla del C6, táctil de la Discovery): si un programa abre su bus antes que la GUI, la placa se reinicia
como antes (§H6.11.7).

**Estado de las placas**: la C5 con la imagen de `G4.a1`; el resto sin grabar (sólo compiladas). Sin push.

## ⏭️ Traspaso (23-sep, mañana) — `G3` hecho, **falta mirar la C5** (y con eso `G2`); lo siguiente: `G4`

**Lo primero al volver Eduardo**: la C5 quedó con `C5Touch` en marcha —«toca el boton», el botón «PULSA»
y «pulsaciones: 0»—. Mirar los colores (el azul del botón; si sale verdoso o rojizo, el giro de bytes, que
está en `pant_lvgl.c`) y pulsar: el contador de la pantalla tiene que subir. Sirve sin el PC; abrir el
puerto REINICIA la C5 y se lleva el programa. Si todo va, `G2` y `G3` se marcan ✅ en `FICHAS`. Para volver
a `DriverC5`: `packdel DriverSpi` y grabar `bpstdlib/gui/drivers/c5/out/DriverC5.pack` (con las líneas del
ENV puestas no molesta: `DriverC5` no las lee).

**Lo siguiente es `G4`, REDISEÑADO con Eduardo esa misma noche** (`V7_IDEAS` §H6.11): el bus es
COMPARTIDO y lo exclusivo es el dispositivo (su CS); los pines de SPI e I2C van en el ENV (`spi2=`,
`i2c0=`) y quien pida otra configuración recibe un error con nombre; un árbitro —un mutex por bus— en la
HAL BP; desde BP, una transacción nunca abarca dos llamadas. Se empieza por `G4.a1` (los buses en el ENV,
en las cinco familias); el DMA es `G4.b`. Decidido con Eduardo: un bus que el ENV no declara está libre
y lo establece el primero que aparece, y se puede liberar (`release()`) salvo el del ENV o si lo usa otro. Queda también `G5` y, para cerrar
`H6.c`, la documentación de usuario de las líneas `lcd=`/`exp=`/`touch=` (hoy la referencia está en la
cabecera de `driver_spi.c`).

**Estado de las placas**: la C5 con la imagen de hoy (la que pregunta `estado`) y, en su zona, `Gui` +
`DriverSpi` vivos (`DriverC5` y los viejos, borrados); en su ENV, `packRam=512`, `log=1` y las tres líneas
de la pantalla. El P4 sin cambios desde `G2`. Sin push.

## ⏭️ Traspaso (23-sep, mañana) — `G2` hecho, **falta mirarlo en la C5**; lo siguiente: `G3`

**Lo primero al volver Eduardo**: mirar la C5 —`GuiShot` ya dejó pintada su pantalla (una etiqueta, el
botón «Pulsa», la casilla «Activo» marcada y el deslizador al 70 %)— y correr `C5Touch` pulsando el
botón. Si sale bien, `G2` se marca ✅ en `FICHAS`; si los colores salen cambiados, el sitio es el giro
de bytes, que pasó del driver a `pant_lvgl.c` (`formato` RGB565_BE).

**Lo siguiente es `G3`**: el driver genérico MIPI-DCS por perfil, con la C6 de oráculo (también pide
mirar una pantalla). ⚠️ **Lo que muerde**: un motor nuevo sin variables inicializadas se enlaza con
`npk.ld` (los drivers ya lo hacen; si otro pack se queda sin `.data`, `NpackReloc` lo dirá).

**Estado de las placas**: la C5 con la imagen de hoy (consola otra vez sólo en UART0) y, en su zona,
`Gui` + `DriverC5` vivos (los viejos `Gui` y `Mini` borrados; ENV `packRam=512`, `log=1`); el P4 con
`SQLite` + `Gui` + `DriverNulo` vivos (el `Gui` viejo borrado). Sin push.

## ⏭️ Traspaso (23-sep, primera hora) — `G1` de `H6.c` cerrado; **lo siguiente: `G2` (`'PANT'`)**

`G1` cerrado con el criterio de §7.8 cumplido en las dos placas. **Lo siguiente es `G2`**: el contrato
`'PANT'`, el `busca()` perezoso desde LVGL y `panel_c5.c` a su propio `npk` —ya se puede: dos motores
conviven, cada uno con su RAM—. Luego `G3` (el driver genérico por perfil, la C6 de oráculo) y `G4` (el
bus con dueño + la fachada SPI con DMA: el análisis del búfer de 64 B está hecho, §7.11).

⚠️ **Lo que muerde**: (1) actualizar un pack grabando el nuevo SIN borrar el viejo pide RAM para los dos
—en la C5 no cabe; el IDE lo dice antes de grabar—; (2) en el P4 un pack recién grabado no se carga
hasta un `reset` (su puerto no reinicia la placa); (3) la BIOS es v6: un pack reconstruido contra ella
(como el mini de hoy) no carga en una imagen de ayer.

**Estado de las placas**: la C5 con la imagen de hoy, `Gui.pack` + `Mini.pack` grabados (ENV `packRam=512`,
`log=1`); el P4 con la imagen de hoy, `SQLite.pack` + `Gui.pack` (el del panel nulo), `SqlDemo` == host.
Sin push. En otra sesión corre la tarea del tope de 256 B de los builtins de bus (su worktree está en
`miVM/.claude/worktrees/`; no entra en estos commits).
## 🔌 `#534` (23-sep) — el `count` de los builtins de bus, igual en las dos VMs

Visto de pasada analizando el SPI de la C5, y hecho con el método: **medir el «antes» con un sample, proponer la
semántica, y la cascada Java → C → firmware**. El aviso decía «la VM-C recorta a 256»; medido con `BusCount.bp` y los
binarios de HEAD salieron **cuatro** cosas: I2C recortaba a **64** (no 256), `count` < 0 hacía **segfault** en la VM-C,
el array `null` sólo lo miraba miVM, y la dirección I2C del stub se escribía distinto en cada VM (`0x3c`/`0x3C`).
Eduardo eligió entre tres (ficha `#534`): tramos de 256 en SPI y UART en las DOS VMs —así el bucle se prueba con diff
en el PC—, el plazo de `Uart.read` por tramo, e I2C sin partir con tope de 256.

**Lo que enseñó**: (1) la primera versión metía los buffers en el helper y **se midió** con `-fstack-usage` en ARM:
+568 B de pila en cada llamada SPI/UART sobre los 2072 de `bpvm_call_builtin`; con los buffers en el `case`, como
antes, +8 y 64. Leyendo el diff no se veía. (2) Al censar por la primitiva salieron dos hermanos, en fichas aparte por
decisión de Eduardo: `move` en la VM-C no valida nada (`#535`) y `__npShow` recorta a 256 LEDs (`#536`, donde los
tramos NO valen: el WS2812 hace *latch* entre ráfagas).

**Tres trampas de trabajar en un worktree**, para la próxima: `dist/*.zip` está ignorado y `check-fwd` sale rojo sin
él (se copia del checkout principal verificando su sha256 contra el `.sha256` versionado); el firmware de la Pico
configurado en el scratchpad no compila porque la ruta pasa de 260 caracteres (se configura en
`bpgenvm-c/pico/build` del worktree, con `-Dpicotool_DIR` apuntando al del principal); y `mvn install` escribe en el
`~/.m2` de todos: se instaló la miVM de HEAD antes de tocarla y después sólo `package`.

## ⏭️ Traspaso (23-sep) — `#534` hecha; lo siguiente sigue siendo `H6.c`, por `G1`

`#534` cerrada (`60741fa6`: paridad 93/93 con `BusCount`, `check` entero en verde) y **el firmware de la Pico compila**;
queda el tercer escalón con placa, escrito en la ficha: un loopback SPI de más de 256 bytes y `UartBuf` con N = 300.
Abiertas de pasada: `#535` (`move`) y `#536` (`__npShow`). El fat-jar del IDE lleva su copia de la miVM: el arreglo
entra en él al hacer `install` de la miVM y rehacer `BpIde` (lo normal al publicar). Nada grabado en placas; sin push.

## ⏭️ Traspaso (22-sep, FIN DEL DÍA) — H4 y H6.a cerradas; **lo siguiente: `H6.c`, por `G1`**

Un día largo y con tres tramos: **H4** por la mañana, **H6** (análisis, P1, la BIOS v5, P2, P3 y la
prueba en el P4) por la tarde, y por la noche **la C5**: `H6.a` cerrada con la placa pintando y
respondiendo al tacto con LVGL viniendo de un pack. Y de cierre, una charla de diseño que **corrigió el
análisis** — la de «un driver genérico y N perfiles».

**Lo siguiente es `H6.c`** (generalizar el pack a cualquier placa; `V7_IDEAS` §H6.7-7.10, con las seis
preguntas ya contestadas por Eduardo), y empieza por **`G1`**: `env_get` en la BIOS y el reparto de RAM
entre varios `npk` — hoy sólo se carga uno, y con el driver fuera del pack de LVGL hacen falta dos. El
resto del plan: `G2` `'PANT'` · `G3` el driver genérico por perfil, **con la C6 de oráculo** (pinta hoy
con LVGL dentro: si el pack genérico la pinta igual, el nivel 0 queda demostrado contra un artefacto) ·
`G4` el bus con dueño y DMA en la fachada SPI · `G5` el P4 y la DK2 publicando `'PANT'`, que es `H6.b`.

⚠️ **Tres cosas que muerden y están escritas donde toca**: el bus SPI **no tiene dueño** (en la C5 la TF
comparte bus con el panel: un `Spi.Bus(2, …)` desde BP lo mataría en silencio); construir un `npk` hoy
**sólo podemos hacerlo nosotros** (plantilla + script + docs son entregable de `H6.c`); y con la consola
sólo en UART0 un reinicio de una ESP32 es **mudo** — la receta para verlo quedó en `esp32c5/sdkconfig.defaults`.

**Estado de las placas**: la C5 con la imagen final y `Gui.pack` grabado; el P4 restaurado con
`SQLite.pack` y `SqlDemo` == host. Árbol limpio, sin push.

## ⏭️ Traspaso (22-sep, mediodía-noche) — H4 y H6 el mismo día; la C5 era lo siguiente

Hoy entraron dos hitos: **H4** por la mañana (arriba) y **H6** por la tarde y la noche —análisis,
P1, la BIOS v5, P2 y P3—, y el pack de la GUI quedó **probado en el P4 sin tocar su LVGL**, que era
la idea de Eduardo. Commits: `d66e31b9` `ad6ceef1` `67f7b564` `1bcf1ec8` (H4) · `376933b3` `15dbce59`
(el análisis de H6) · `552f9f26` `99b6ea04` `b6b2ce59` `0267b92f` (P1 + BIOS v5) · `2f29542c` (P2) ·
`38525ead` `0afccb85` (P3) · `d28a31fb` `c2e8c602` (la prueba en el P4 y los docs).

**LO SIGUIENTE, y Eduardo lo dijo al pausar: la C5.** ✅ **HECHO esa misma noche** (arriba): `H6.a`
cerrada. La receta que se siguió, que sigue valiendo para la próxima placa:

1. **Grabar la imagen** (`esp32c5/`, ya compilada con `-DBPVM_GUI -DBPVM_GUI_PACK`: lleva los 83
   builtins y los trampolines, sin `gui.c` ni LVGL). La C5 es USB-Serial-JTAG; el puerto fue **COM6**.
2. **El mini primero** — el control antes del motor: `bash bpgenvm-c/tools/npack_mini/build_mini.sh imc`
   (ya hecho: `mini.npk.RV32IMC`), `Mini.bpbuild` → `Mini.pack`, y grabarlo con
   `wire_serie.py COM6 pack ... riscv-imc`. Tiene que decir «mini: TODO OK -> 0». Si no, el fallo es
   del cargador y no del pack de la GUI — que es justo para lo que existe el mini.
3. **El pack de la GUI**: `bpstdlib/gui/out/Gui.pack` ya está construido con los dos `npk`;
   `wire_serie.py COM6 pack bpstdlib/gui/out/Gui.pack riscv-imc` se queda con el `RV32IMC` (panel C5).
4. **Encenderla**: un `Gui*Demo` de los de siempre —los builtins son los mismos, `Gui.bp` es el mismo—
   y `samples/GuiPack.bp` si se quiere ver también el camino `Gui2`.
5. **Medir** lo que el análisis dejó dicho: ms por flush de 24 líneas y por pantalla completa (la
   fachada SPI del ESP32 va **sin DMA**, 64 B por transacción: es el número que decide si hay que
   tocarla), la RAM del pack en uso, y la latencia de un clic con `GuiClickMeas`.

⚠️ **Dos cosas que morderán en la C5 y están escritas en el código**: el **reset del panel y el
backlight NO son GPIO** (van por I2C al CH32V003 en 0x24: si no contesta, pantalla negra con el SPI
perfecto), y **hoy se carga UN pack nativo** (el primero válido de la zona) — la C5 con la GUI no
podrá tener además SQLite hasta que esté el reparto de RAM entre varios `npk` (`V7_IDEAS` §H6 2.5).

**Después de la C5**: `H6.b` (quitar LVGL de C6/P4/DK2) sólo tras la ronda sellada; y H7 (`#530` 🔴,
`#531` 🟡), que era lo siguiente antes de que H6 se adelantara.

## ⏭️ Traspaso (22-sep, mediodía) — H4 casi entera en una mañana; lo que queda pide placas (C6, Nucleo/DK2)

Hecho hoy: H4 P1 (la tabla de capacidades, `d66e31b9`), P2 (`#479`: el mapa de pines en uso, `ad6ceef1`), P3a (`#487`: el
breadcrumb en el común, `67f7b564`), la cascada (siete imágenes; Pico y P4 grabadas, tanda verde; `1bcf1ec8`). Cerradas
`#378`, `#470`, `#479`, `#487` y los restos de `#471`; el nombre `pico` fuera de la capa C; la flash la dice la placa.
**Queda de H4**, todo con placa: NeoPixel (RMT, GPIO8) y el botón BOOT del C6; el ADC externo del STM32 (`#469` resto) con
la Nucleo/DK2; la máscara de pines por paquete cuando haya dos placas del mismo die. **Pendiente de grabar**: S3/C3/C6/C5/
Nucleo/DK2 (imágenes de las 09:3x, con H1 + H3 + H4). **Lo siguiente**: H7 (`#530` 🔴, `#531` 🟡) o lo que Eduardo diga.
Árbol limpio, sin push. **(Y luego la tarde y la noche fueron H6 entera hasta donde llega sin la C5: ver arriba.)**

## ⏭️ Traspaso (21-sep, al cerrar) — la 1.ª etapa de H11 HECHA esa misma noche (con PSRAM); lo siguiente: H7 y H4

Eduardo, al parar: *«cuando vuelva podemos probar la 1.ª etapa a ver qué tal. Y nos pararemos ahí. Después H7 y H4 o
lo que toque, pero ya para mañana y siguientes días.»* Así que el orden es: **(1) la imagen de la C5 SIN LVGL**
(H11, 1.ª etapa: clonar `esp32c6/` → `esp32c5/`, `chip_cfg.h`, board id, particiones, `sdkconfig.defaults`, el
reparto de memoria con los 8 MB de PSRAM, serie, la TF; `tanda.py` con la C5 de columna; ver `V7_BACKLOG` §H11) —
comprobado ya que el IDF instalado (`C:\esp\v6.0.1`) trae el target `esp32c5` (`idf.py --list-targets`, y
`components/soc/esp32c5`); **(2) H7** (`#530` 🔴 y `#531` 🟡, paridad rota) y **(3) H4** (la HAL). Pendiente de
grabar cuando se conecten: S3/C3/C6/Nucleo/DK2 (las imágenes de las 14:57-14:58 llevan H1 + H3 + el `readLine`
nativo). Árbol limpio en `fa8b9874`; sin push (V7 en curso).

## 🧬 H3 (21-sep, tarde) — EL CURSOR: `IO.File`/`IO.TextFile` con todo el estado dentro del objeto; `#491` cerrada; `listDir` en la VM-C; `/sd` reservado; `#456`

Día de análisis primero (`V7_IDEAS.md` §H3, `289672e8`): el mapa medido (sin descriptor abajo, dos buffers estáticos
en littlefs, 340/552 B por fichero abierto en el micro, el hueco entre marca y barrido del GC) y las ocho preguntas
del 17-ago contestadas. Eduardo: OK a todo, y *«`TextFile` extiende `File` y le añade un encoder/decoder de UTF-8 ↔
bytes»* — que es `toBytes`/`fromBytes` sobre la capa de bytes.

**P1** (`b0e15592`): la fachada C con descriptor sobre un BLOQUE que pone el llamador (cabecera + estado del motor
alineado a 8 + ruta), siete ops nuevas por backend; littlefs con su caché dentro del bloque (cae el tope de dos),
FatFs con el `FIL`, host con `FILE*`; packs y motores sin cursor por `read_at`; `fat_read_stream` de regalo (`#473`
punto 4). `test-fscursor`: el mismo guion en host y littlefs, 153/153. **P2+P3**: nueve primitivas (240-248) en las dos
VMs, el registro de abiertos enhebrado por los bloques (`bpvm_files_gc` entre marca y barrido; `ficherosGc()` en
miVM; fin de RUN cierra y dice «fin de ejecución con N fichero(s) abierto(s) sin cerrar: ruta (modo)» — mismo texto),
la apertura duplicada con escritura es error; `IO.File`/`IO.TextFile` en `IO.bp` con `~File()`; `samples/FileCursor.bp`
en el corpus (bytes, texto con `\r\n` y sin salto final, `readStr` en caracteres, owner/try var, la red del GC);
referencia es/en §13.11b; `PENDIENTES` sin la limitación. **P4**: `listDir` por fin en la VM-C (zona de rascar con
guardián + raíz temporal como `split`; ORDENADO por bytes en las dos, y error si no se puede abrir — miVM devolvía
lista vacía); `/sd` prefijo reservado (sin montaje falla limpio; `test-fscursor`); `#456` el tope de path también en
miVM (`sandboxPath`, «path demasiado largo»); `samples/PathLargo.bp` en el corpus.

🐛 **Lo que cazó el frente MOD8 del arnés**: el importador de interfaces decidía override/sobrecarga sólo contra los
hermanos del stub → `TextFile.write(string)` sobre `File.write(byte[])` salía como override y corría las ranuras
(«no exporta `IO.TextFile#writeLine#20`»: el dueño decía 21). `reclavarContraLaBase` en `sembrarSlotsDeVtable`
(la regla de `assignKeysInChain`), test rojo→verde en `SobrecargaCrossModuleTest`. En MOD9 no se veía. Y dos tests C
que no enlazaban (`test-fspos`, `test-listdir`: `fs_facade.c` usa `bpvm_log` desde U3), ahora con `LIBOBJ`.

Verde: check 89/59/9/13 + MOD8; mezcla 194; h13 38; frontend 223; miVM 52; tests C. **La cascada** (P5): blobs,
las siete imágenes construidas (Pico, S3, C3, C6, P4, Nucleo, DK2), IDE fat-jar; **Pico y P4 grabadas y verificadas**
(`placas_v9` verde; `FileCursor` y `PathLargo` == host en las dos; y en el P4 un `TextFile` sobre `/sd` —FAT por SDMMC,
el `FIL` en el heap—: 200 líneas escritas, leídas, un byte parcheado con `r+`). Queda grabar S3/C3/C6/Nucleo/DK2
(las mismas cinco de H1). Quedan de la fila H3: `SD_INFO`/`SD_MOUNT` al REPL común y la SD del STM32 — las dos de
placa con tarjeta (Metro, DK2).

**La medida** (Eduardo: *«¿has hecho una comparación de rendimiento?»* — no; hecha, `samples/FileBench.bp`, tabla en
`V7_IDEAS` §H3.6): escribir 400 líneas en littlefs pasa de **19,9 s** (`appendFile`, abrir+commit+cerrar cada una) a
**0,3 s** con `writeLine` (Pico; 15,3 → 0,18 en el P4; 0,9 → 0,1 en la SD). Leer sigue ganándolo el fichero entero
(nativo de punta a punta) pero a costa de tenerlo ENTERO en el heap. Y destapó que `readLine` en BP era 15× más
lento que `readFile`+`split` (354 ms/400 líneas en la Pico): **`__freadLine` nativo** (id 249), 87 ms. Pico y P4
regrabadas con ello; `FileCursor` == host en las dos.

## 🧬 H1 (21-sep, tarde) — `#19` cerrada como la redefinió Eduardo; la fila H1, hecha salvo grabar cinco placas

**`#19`** (`60325e4c`): *«mantenemos los array fijos pero los limitamos a declaraciones locales y dentro de una clase como
private. No utilizamos referencias con ellos»*. Un `tipo[N]` no es una referencia: vive en el marco (`[len][N elems]`,
`LEA_LOCAL`, que existía desde H1.2a sin emitirse) o dentro del objeto (`private var`; `LEA_FIELD`/`LEA_FIELD_REF`,
0xBC/0xBD, nuevos en las dos VMs), y sólo se indexa, se recorre y se mide; lo demás es error con línea. A nivel de
módulo, fuera (se horneaba en el data block: se pierde la tabla en flash — `ModInit` pasa a `tipo[]`). Un campo fijo
ocupa `1 + ⌈N·ancho/4⌉` ranuras (hasta 255; el cargador C lo admite; `FieldInfo.slots`). Tres tropiezos por el camino:
`bpref_field` devuelve un PUNTERO, no un offset (segfault en la VM-C); la cabecera del campo se escribía con un store
de 8 bytes (el tipo del símbolo es ArrayType = ref); y el bitmap del layout marcaba el campo como ref. Verde: check
88/59/9/13/88, mezcla 194, frontend 222, miVM 52, errores 38, tests C. **La cascada otra vez** (la VM-C cambió): blobs,
siete imágenes, Pico y P4 grabadas y `ArrayFijo` == host en las dos; IDE fat-jar.

**H1, estado**: todo lo de la fila hecho — el módulo raíz (P1-P3), P2c, Map, destructor, Time, #19, N9/N2/M4/L19;
M5 a H8; #460 en ideas; P4 no. **Queda sólo grabar S3/C3/C6/Nucleo/DK2** (construidas). Y `#530`/`#531` son de H7.

## 🧬 H1 (21-sep, mediodía) — Map, el DESTRUCTOR, Time; la cascada rehecha; Pico y P4 grabadas

**Map** (`5fa31e28`): las sobrecargas que pidió Eduardo (clave integer/string/Object; captadores tipados del valor), en BP.
**Destructor** (`29e737c1` + `e5d9425b`): `function ~Clase()`, virtual y encadenado al padre; `var owner` local lo llama al
morir (orden inverso, también con excepción: la función con owners va dentro de un TRY que destruye y relanza); `try var`
(capa 1); owner no escapa; sin cambios en las VMs. Consecuencia: Object gana la ranura 2 (`__dtor()V`) → en MOD8 los
marcadores corren una y una app MOD8 contra el Core MOD7 de V6 ya no enlaza (la norma lo dice así). **Time** (`d3af4dde`,
#396): `Time.Date` sobre un long, BP puro. Verde: check 87/59/9/13/87, mezcla 194, frontend 222, errores 35.
**La cascada, rehecha entera** (Core cambió tres veces desde ayer): blobs con `Time` en las tres listas, las siete imágenes
construidas, **Pico y P4 grabadas y verificadas** (placas_v9 verde; Destructor y TimeDemo dan lo mismo que el host en las
dos; en el P4, Gui/Json/Collections/Str/Time de hoy subidos a /lib). Quedan S3/C3/C6/Nucleo/DK2 por grabar.
**Queda en H1**: `#19` (array fijo local inline) — mi propuesta es cerrarla como NO (abajo, para Eduardo).

## 🧹 H1 (21-sep, mañana) — los huecos baratos de la fila: N9, N2, M4, L19

Cuatro commits, cada uno cerrado y verde: **N9** (`760e6a2a`) redeclarar Object/Thread/Mutex/StringBuilder fuera de
Core es error con línea (con P3 una copia local a medias ganaba en silencio; `samples/errores/ClaseDelLenguaje.bp`,
también `root class` fuera de Core); **N2** (`5836c6ac`) el invariante de `JavaMutex` (ownerTid/waiters sólo bajo
vmLock) escrito en el código tras comprobar los cuatro sitios; **M4** (`fa778c9c`) el prefijo `__` es del sistema:
fuera de Core sólo lo lleva una intrínseca o una función pública (el puente `Gui.__guiDispatch` que la VM llama
por nombre), lo demás es error con línea (`samples/errores/PrefijoSistema.bp`; el relleno de `--barajar` pasa a
`relleno__N`); **L19** (`977f3ba2`) `Perro(a)` con `a: Animal` baja directo (`esAntepasadoDe`; el emisor ya hacía
CHECKCAST contra cualquier clase; `samples/CastBaja.bp` en el corpus, 84). h13 30/30, check 84/59/9/13/84, mezcla
194/194, frontend 221/221. **Quedan en la fila H1**: `M5` (comprobar que el debugger enseña las properties
heredadas: es una prueba con el debug server, no código), `#460` (herencia entre módulos: decisión de Eduardo —
con #523 el «control de versiones efectivo» que pedía ya existe), destructor `~Clase()`, `Map`, `#396` `Time`,
`#19`; y grabar S3/C3/C6/Nucleo/DK2.

## 🧬 H1 (20-sep, tarde) — P2c y P3: las intrínsecas se sobrecargan, y las CUATRO clases del lenguaje viven en BP

**P2c** (`4dee6cf4`, `231fa322`): `Math.sign(x: integer)` / `sign(x: float)` (fuera `signF`); `len(s: string)` /
`len(a: any[])` en `Core.bp` con `any[]` = «cualquier array» (una línea en `ArrayType.isAssignableFrom`, sólo la raíz
escribe `any`), y el caso especial «dirigido por tipo» fuera del semántico y del emisor. Bug cazado: la segunda `len`
se perdía en silencio (`tryDefine`); ahora las globales de la raíz encadenan sobrecargas como el importador. Se
pierde `len(obj)` sobre una clase con `length()`/`size()` (azúcar que se saltaba la visibilidad): es `obj.length()`.

**P3** (este commit): `Object`, `Thread`, `Mutex` y `StringBuilder` escritas en BasicPlus al principio de `Core.bp`,
compiladas UNA vez. Las dos decisiones de Eduardo: *«'root' o algo similar para indicar que no desciende de nada»*
→ `public root class Object` / `public root class Thread` (palabra contextual, sólo en Core; sin `root` y sin
`extends`, la base es `Core.Object`); *«import Core se añade al árbol AST si no está incluido ya en el módulo, así
que se puede indicar o omitir, pero en el AST se incluye siempre salvo en Core»* → lo pone el PARSER (al final de los
imports), y con eso todo lo público de Core se ve sin cualificar y `cargarRaiz`/`tipoDeRaiz` (P2) desaparecen: las
globales entran por analizador desde el namespace del import (`registrarRaiz`), con los stubs de ESTE módulo.
Fuera: los 20 `makeMethod`, las cuatro síntesis del emisor (+ las de List/OwnerList/SyncList que estaban muertas:
~1000 líneas de Java escribiendo bytecode a mano), `objectBaseLayout`/`threadBaseLayout`/`builtinBaseMethodNames`,
`sembrarRaizBuiltin`, `claseBuiltin`/`resolverComoBuiltin`, la inyección de toString/compareTo en cada clase y
`ClassSymbol.isBuiltin`. Las claves canónicas de las cuatro siguen SIMPLES (`LObject;`, `esDelLenguaje()`), así que
la ABI de claves no cambió; el nombre de la clase como REF sí va cualificado (`Core.Object`), que es lo que el
cargador necesita (el primer intento las dejó simples en los dos sitios: «'Object' (no cargado)»). Las subclases
sintetizadas de `parallel` y `Thread(obj::m())` se abren contra `Core.Thread` (`abrirClaseSintetizada`); el `Mutex`
de `sync` y su `lock/unlock`, por la clase del lenguaje (`construirDelLenguaje`/`invocarDelLenguaje`); las tuplas
son root (no tienen métodos). **Consecuencias medidas**: toda clase sin `extends` es ABIERTA en MOD9 (el cargador
la construye contra `Core.Object`); todo módulo depende de Core al cargar (`Core.__init`, como la unidad System); la
stdlib adelgaza **25,5 KB** (Str.mod 5715→4641; Gui 46,3→45,0 KB); un módulo MOD8 con clases necesita un Core MOD8
(los tests —`StdlibDeTest`— y `mezcla.py` —`core_para`— se lo fabrican; en mezcla, «app MOD9 contra Lib MOD8 g8»
predice ahora «otra interfaz»: la Lib vieja arrastra su Core viejo y la app nueva lo rechaza con nombre). Un bug
del importador salió con esto: copiaba las ranuras de una base LOCAL todavía diferida (la interfaz no va en orden
de declaración; desde que toda cadena acaba en Object el caso se da) → tabla vacía → «no exporta
'Core.Integer#value#0'» en MOD8; ahora se difiere también. El tope #529 (256 ranuras) se aplica a lo que el
compilador sabe (lo propio): `TopeClase` pasa a 257 métodos propios. Manual §9.4 (la raíz, `root class`) y §10.0
(`import Core` implícito, las cuatro en BP); gramática. Tests del frontend que compilan clases: dependen de
`../bpstdlib` como cualquier proyecto (`StdlibDeTest.paraFormato`). Verde todo: check 83/59/9/13/83, mezcla
194/194, frontend 220/220, errores 22/22, miVM 52/52, tests C.

**Riesgo que acecha**: las placas llevan el firmware del 17-sep con la stdlib vieja; los `.mod` de hoy (interfaz de
Core cambiada, clases abiertas contra `Core.Object`) NO enlazan allí hasta la cascada (P5). Y el coste de carga en
la Pico de «todas las clases abiertas» no está medido: se mide en P5 con una app grande (Gui).

**P5, primer tramo (16:00)**: blobs de las tres familias regenerados (`regen_all_mods.sh`) con la stdlib de P3 y
el firmware de la Pico compilado (`bpvm_pico.uf2`, 657 KB; sin grabar). **Tramo 2** (`471f7865`): `bpdevices/*.mod`
recompilados y los `.dbg` de la stdlib al día. **Tramo 3** (16:10): las siete imágenes construidas con los blobs
nuevos — S3 (0x732e0), P4 (0x142cd0), C3, C6, Nucleo (.elf 4,0 MB) y DK2 (.elf 10,2 MB) — **ninguna grabada**. El pack
de la stdlib no se versiona (`bpstdlib/out/Stdlib.pack` sale del build; `montar-zip.sh` lo recoge). ⚠️ **Falso**
(corregido el 23-sep, `#537`): `packs/Stdlib.pack` SÍ está versionado —es lo que el IDE graba en el micro simulado y
lo que `montar-zip.sh` copia al ZIP—, se quedó en el del 18-sep y ahora lo vigila la red. Queda: grabar y
verificar en placa cuando Eduardo diga (Pico con `placas_v9.py --embebida`; medir la carga de una app con clases,
que ahora son todas abiertas).

**Tramo 4 (17:00) — Pico y P4 GRABADAS y verificadas** (Eduardo las conectó). Pico (`flash_pico.py` por BOOTSEL del
wire): `placas_v9.py --embebida` verde — CoreRefs MOD9 contra el Core embebido (1 clase construida, 10 refs, 45 B de
arena, enlace 1573 µs) y CoreRefs MOD8 rechazado con nombre; CtorInit (4 clases): 4 construidas, 23 refs, 181 B,
1959 µs. P4 (`idf.py -p COM15 flash`; el CH343 ahora es COM15, no COM14): CtorInit OK; GuiParidad OK con la Gui
vieja de su /lib (sellada contra el Core del 17-sep: «otra interfaz» → enlace tardío, 2 clases) y, tras subir a
/lib Gui/Json/Collections/Str de hoy (el blob ESP no las lleva), **la medida que faltaba**: 43 clases construidas,
315 refs, **6 217 B de arena y 23,9 ms de enlace** (6 módulos; antes 9,9 ms) en el P4 a 400 MHz. Ése es el coste
de «todas las clases abiertas» en una app con Gui; en la Pico (150 MHz, sin Gui) las apps son mucho más pequeñas.
Quedan por grabar S3/C3/C6/Nucleo/DK2 (construidas). Informe: `compat/informes/placas_v9_2026-09-20.tsv`.

**Hueco cerrado (16:30)**: asignar a un método (`c.m := 5`) lo rechaza el semántico con línea («'m' es un método,
no un campo»); ya no queda ningún error que sólo dé el emisor desde un fuente válido, así que `#521`
(EmisorConErroresTest) lo fuerza con `MivmEmitter.PRUEBA_ERROR_DE_EMISOR` (andamio, sólo tests). Frontend 221/221.

**H1 NO está cerrado** (pregunta de Eduardo al cerrar el día): el módulo raíz sí (P1-P3; P4 propuesto como no),
pero la fila lleva encima lo que sigue. **Orden para retomar** (propuesto, Eduardo no lo ha vetado): (1) lo barato
que P3 pudo resolver — `N9` (ya no hay clases sintetizadas que redeclarar) y `#460` (herencia entre módulos, con
#523): COMPROBAR y cerrar o reabrir, no asumir; (2) `N2` (documentar el invariante de JavaMutex) y `M4` (prohibir
`__` en identificadores de usuario); (3) `L19` (`Perro(a)` con `a: Animal` se toma por constructor); (4) los de
diseño con Eduardo: destructor `~Clase()` + `var owner` local, `Map` (captadores tipados, `put` sobrecargado),
`#396` `Time`; (5) `#19` (array fijo local inline). Además: grabar S3/C3/C6/Nucleo/DK2 cuando estén conectadas;
y sin ficha: los mensajes de tipos no cualifican, dos módulos homónimos de librerías distintas no se importan
juntos. P2b es de H4.

## 🧬 H1 (20-sep, domingo por la mañana) — el cuerpo `inline`: la stdlib entera declara sus intrínsecas en BP

La decisión de Eduardo a las 8: *«una función intrínseca se declara como una función normal; la diferencia es el
cuerpo: `inline opCode`, y al final `end`. Es la sintaxis de Turbo Pascal»*. Eso cambió P1 de forma (ya no hay que
«atar» nada) y salió entero en la mañana (`b27ab09d` + el commit de hoy): `inline` entra en el lenguaje y `intrinsic`
sale; el bytecode viaja en la interfaz (`func … inline CALL_BUILTIN 24`) y el consumidor lo inlina; la stdlib
entera migrada (177 funciones, 17 módulos, guion de una vez); `Intrinsics.java` borrado (162 registros a mano);
`scripts/gen_builtins.py` comprueba los ids contra `Builtin.java` y será el generador cuando las 65 globales de
`addBuiltin` pasen a `Core.bp` (P2). 20 `.mod` de la stdlib cambian sólo en la interfaz: las imágenes del 17-sep
llevan los viejos → un programa compilado hoy que importe uno de esos 20 no enlaza en placa hasta la cascada de P5
(las intrínsecas no llaman al módulo: es el sello `ifaceCrc`). Lección de la mañana: un test del frontend (`EmisorCon
ErroresTest`) usaba la `intrinsic` sin registro como «error del emisor»; el caso que queda a mano es `c.m := 5`
(asignar a un método), que el semántico debería rechazar con línea y hoy no.

**P2, el mismo domingo (mediodía).** `Core.bp` es la raíz: la sección «LAS PRIMITIVAS DE LA VM» (240 funciones,
una por id, con su firma y su bytecode; generada UNA vez de lo que había con `scripts/gen_core_raiz.py`), las 64
globales (`strlen`, `abs`, `parseInt`, `newIntArray`, las `__` del compilador…) `public` y visibles sin cualificar en
todo programa (`Main.cargarRaiz` lee Core.mod como un import y las mete en el scope global; sin Core.mod, aviso
ruidoso), las 176 restantes privadas: sólo la tabla. Fuera los 65 `addBuiltin` (quedan 4: `byte()`/`int8()`/`word()`/
`int16()`, palabras reservadas que BP no puede declarar). `scripts/gen_builtins.py --escribir` GENERA `Builtin.java` y
`bpgenvm-c/src/builtin_ids.h` (que `builtins.c` incluye; sus notas históricas, en un `#if 0`); las 5 constantes que
no seguían la regla, renombradas (`NP_INIT`, `GUI_IMAGE_VIEW_*`); `check_builtins.py` borrado. Core.mod: 14,3 → 19,3 KB
(sólo interfaz); 12 `.mod` de la stdlib cambian (los sellos). Firmware de la Pico compilado (sin grabar). Manual §10.0.
Lo que pide cuidado mañana: `any` sólo lo puede escribir la raíz (por nombre `Core`), y una `inline` privada vale
(antes «intrinsic implica public»).

El mapa está hecho (20-sep): `docs/V7_IDEAS.md` §H1 — son 240 intrínsecas en CINCO sitios a mano (los ids en
`Builtin.java` y `builtins.c`, las firmas en 65 `addBuiltin` y en las `intrinsic` de los `.bp`, el enlace en
162 `register` de `Intrinsics.java`), y el plan en cinco pasos (P1 el generador de las dos tablas desde `Core.bp`;
P2 las 65 globales de `addBuiltin` a `Core.bp` como raíz (y entonces `gen_builtins.py` ESCRIBE `Builtin.java` y el enum de C); P3 `Object`/`Thread`/`Mutex`/`StringBuilder` en BP; P4 primitivos; P5 la
cascada). P1 y P2 hechos el 20-sep. **Orden para retomar**: P2c (sobrecargar intrínsecas: `sign`/`signF` → `sign` ×2, y la
idea de Eduardo con `len`) y P3 (las clases del lenguaje en BP); P2b es de H4. Abiertas además: `#519` (H4/AOT), `#530` y `#531` (H7). En H7 esperan `#530` (Thread.run heredada
cross-module) y `#531` (orden de los eventos). Quedan escritos sin ficha: los mensajes de tipos no cualifican
(«'Base' no asignable a 'Base'»), y dos módulos homónimos de librerías distintas no se pueden importar en el mismo
fichero (`#528`); un inicializador de campo no puede usar `this` ni otros campos (manual §9.1).

## 🧬 `#523` V11 (17-sep, noche) — LA CADENA EN MOD9: el compilador, la stdlib (gen 7), los blobs, las seis imágenes, el pack, el IDE

Decisión de Eduardo: «pasamos a MOD9 y vamos arreglando lo que surja». Hecho y verificado en placa (Pico y DK2 grabadas:
un MOD9 enlaza contra el Core MOD9 embebido; un MOD8 se rechaza con nombre). `--stdlibDir=DIR` nuevo en el frontend; el
`.slots` fuera; `check` con la paridad principal en MOD9 y `check-mod8` como foto «antes»; mezcla 194/194; tests C con el
guardián de fixtures que grita. Cierra `#514` y `#500`; abre `#533` (AOT fase 2: método de clase externa desde el `.mdn`).

**El pack de SQLite ya está en MOD9** (noche): `.mdn` ARM/RISC-V regenerados, `Core`/`Str` de dentro gen 7 (copiarlos a
`out/` antes de construir, o salen gen 0 y E0 los rechaza), `test-sqldemo` OK en host con todo MOD9; y **en la Metro
(con PSRAM, regrabado el pack porque el sello del motor es de la RAM que la placa da en ese momento): `SqlDemo` MOD9
byte-idéntico al host**. Metro y Nucleo grabadas con la imagen de hoy y verificadas (`CoreRefs` MOD9 OK, MOD8 rechazado);
`/app` de la Metro limpio; el pack queda grabado y sellado; ENV `SQLite=2` como estaba.

**El P4 también** (noche): grabado con la imagen de hoy; E9 en vivo por un `Core` MOD7 rancio en `/app` (borrado; `/lib`
repuesto por la imagen y `Gui`/`Json`/`Collections`/`Str` MOD9 repuestos a mano en `/lib`); `CoreRefs` MOD9 OK; el pack de
SQLite RISC-V grabado y sellado, **`SqlDemo` MOD9 byte-idéntico al host**. Cinco placas con la imagen de hoy.

S3/C6/C3 no se graban a propósito (Eduardo: «ya está bastante probado»; las imágenes están construidas — ojo al usarlas:
un `/app` con `.mod` MOD7/8 deja de enlazar, se recompila desde el IDE).

## 🧬 `#533` (18-sep) — el AOT en MOD9, fase 2: el `.mdn` pide la ranura por clave

Helper `slot_por_clave` en la tabla del `.mdn` (`MDN_ABI_VERSION` 7 en `mdn_format.h` y `MdnPack`), `aot_<Mod>_ranura`
en el C generado para clases externas/abiertas (método y getter de property), `bpvm_link_slot_por_clave` en `link.c`
(la búsqueda exacta, privados ajenos no cuentan). Verificado en host y en la Pico 2 (con control: un `.mdn` marcado
da 110). Regenerados: `test/aot_XMethodNat.c`, los `.mdn` del pack de SQLite (ABI 7), `packs/SQLite.pack`; las seis
imágenes reconstruidas (Pico grabada; las demás cuando toquen — y al grabarlas, el pack de SQLite de la Metro y del P4
hay que regrabarlo: su `.mdn` es ABI 6 y el cargador lo rechazará con nombre). Frontend 220/220, tests C OK.

## 🐛 `#525` y `#526` (18-sep, tarde) — las dos graves de V10, cerradas

`#525`: un receptor null en los opcodes de campo y en INVOKE_VIRTUAL es un `RuntimeError` atrapable con el mismo texto en
las dos VMs (miVM leía `memory[4+slot*4]`; la VM-C paraba el RUN); `samples/NullRecv.bp` en el corpus. `#526`: `print obj`
despachaba la ranura 0 (en un Thread, `run`); ahora `Thread` tiene `toString` (la 3) y el emisor despacha por nombre/clave;
por el camino la tercera tabla de Thread (el importador) se desincronizó y `CtorHijo` la cazó → fuente única
(`builtinBaseMethodNames`); `samples/ThreadPrint.bp` en el corpus (71). La cadena rehecha (stdlib, packs, blobs; Pico
grabada; STM32/ESP32 reconstruidas), el IDE fat-jar también. check verde 71/59/9/12/71, mezcla 194/194.

## 🧹 `#516` y `#520` (18-sep, noche) — H2 sin fichas abiertas

`#516`: el enum cruza en las firmas (`enum:Color` / `enum:Lib.Color` en la interfaz, `UnresolvedEnumRef` de 4 B, resuelto
contra `ns.enums`; un enum de un tercer módulo no importado es error con nombre); samples `EnumFirmaLib`/`EnumFirma` en
el corpus (72). `#520`: `pack:<Pack>/x` en miVM como en la VM-C (caso 13 de sombra), `rootModuleDir` del pack, orden
raíz→workdir, exit 2 al no existir el fichero, buffers de `discover_deps` a `BPVM_FS_PATH_MAX`. check verde
72/59/9/13/72. **H2 queda sin fichas abiertas** (sólo `#522`, que es de H1); `#523` sigue abierta con lo de siempre:
`montar-zip.sh` al publicar y la decisión de quitar `--formato=8` al cerrar V7.

**Orden para retomar** (decisión de Eduardo, 18-sep, noche): **H1, el módulo raíz**. Arrancar por `#522` (enum:
`"c=" + c` revienta el heap y `Color(1)` devuelve basura, dentro del propio módulo — hermana de `#516`), luego las de
V10 en H1 (`#527` `#528` `#529` `#532`) y de ahí el módulo raíz propiamente (los tipos y las intrínsecas en un fichero
legible, las tablas generadas). El IDE fat-jar lleva el compilador y la miVM de hoy. De H2 quedaba
`#516` (enum en firmas) y `#520` (rincones de la resolución por CLI) detrás. Ojo: cualquier `.mod` MOD7/8 que
haya en `/app` de una placa con el firmware de hoy deja de enlazar — se recompila desde el IDE (que ya emite MOD9).

## 🧪 `#523` V5 (17-sep) — el corpus entero en MOD9 entra en la red diaria

Sesión corta (créditos justos: nada de workflows). `compat/compat.sh check-mod9`, y como sexto frente de `check`: la
stdlib compilada como MOD9 en el temporal + los 69 samples del corpus con `--formato=9`, exigiendo magic `MOD9` en el
`.mod` (control hecho: con el flag neutralizado caen los 69), paridad Java == C y el mismo stdout que la pasada MOD8.
**69/69**; `check` verde en 4m15 (69/59/9/12/69). Ver `FICHAS` `#523` §17-sep.

**V4, las filas que faltaban** (bloque V de `mezcla.py`, 31 filas: override huérfano → E2, el abuelo en tres módulos ×
21 variantes, dos averías, MOD8×MOD9 en las dos direcciones): **167/167** en MOD9. Hallazgo por el camino: E9
(«recompila 'Lib'») no llegaba a hablar nunca porque el import 3.1 del constructor con firma caía antes; los dos
cargadores lo comprueban ahora antes de los imports (mismo texto, mismo momento). miVM 51/51, tests C OK, `check` verde.

**Orden para retomar**: (1) `#525`/`#526` (graves, preexistentes); (2) lo que queda de `#523`: V6 (GC con bitmaps
reconstruidos como fila fija), V9 (medidas), V8 (placas), la cadena en MOD9 (V11: `build_mdn.sh`/`AotBuild` con el
formato, `.slots` fuera) y los docs (MOD_FORMAT con la ref SUPER y la 4.5, OPCODES, HEAP_LAYOUT `+6`, WIRE E0-E10).

**V6 también** (misma tarde): el GC con los bitmaps reconstruidos como filas fijas de la mezcla (app `Gc`: clase abierta
con campos de referencia propios, 300 vivos, `gc()` forzado; Base con un campo de referencia delante, con 33 campos
—segunda palabra del bitmap— y barajada en B): 17 filas, todas ok; mezcla MOD9 **194/194**.

**Los docs de MOD9** (tarde): `MOD_FORMAT.md` §13 (v9 entero, E0-E10 con texto), `OPCODES.md` (`0xB1..0xBB`),
`HEAP_LAYOUT.md` (`+6`), `BPVM_WIRE_PROTOCOL.md` (`LINK_ERROR`). Al escribir el wire salió que la miVM mandaba el fallo
de enlace como `RUNTIME_ERROR` con prefijo «link: » y el device como `LINK_ERROR` pelado: igualado (test).

**V9 en el host** (`compat/v9_medidas.py`, 69 samples): `.mod` +32 % — y no por las refs: son los 484 B de la 4.5 con
las cuatro clases builtin que cada módulo lleva copiadas (el módulo raíz, L1, es el arreglo); enlace VM-C 80 → 98 µs
(mediana), `strcmp` 10× menos en los samples con GUI, los GUI más pequeños en MOD9.

**V8/V9 en placa** (tarde, Pico y DK2 con el firmware de hoy; `compat/placas_v9.py`): `CoreRefs` MOD8 y MOD9 con el stdout
esperado en las dos placas (Core MOD9 a `/app` por `puts`); `GuiTableDemo` en la DK2 con toda la stdlib MOD9 en `/app`:
**mismo stdout que MOD8 y enlaza más rápido** (30,4 → 27,2 ms; 91.200 → 8.623 strcmp; `Gui.mod` 61 → 44 KB). Salió el
desfase de stdlib: el Core embebido de las dos imágenes es MOD7 y la `Collections` MOD8 de hoy no enlaza contra él
(«no exporta List#add$LObject;»); en MOD9 corre. Y el compilador sella `Core` desde `bpstdlib/` (BpVM.cfg) y no desde
el `.mod` de al lado: los arneses compilan una copia fuera del repo. **Queda de `#523`**: V8 en ESP32-S3/C6/P4 (cuando
estén conectadas; cargador común) y V11 (la cadena en MOD9: `bpstdlib/*.mod`, los blobs de las cinco imágenes, packs,
IDE, `build_mdn.sh`/`AotBuild` con el formato, `.slots` fuera) — V11 es cambiar el formato por defecto: lo decide Eduardo.

## 🔬 `#523` V10 (16-sep, tarde-noche) — la revisión adversarial del enlace tardío: 85 hallazgos, 25 arreglados, 8 fichas, 1 decisión

**Hecho** (ver `FICHAS` `#523` §V10): dos workflows de revisión (9 buscadores; la verificación por agente murió dos veces por el
límite de uso → triaje a mano con las reproducciones). Arreglado el lote grave de programas legítimos en el compilador y en las
dos VMs (`4d075d65`), EC-2 (`8fd40d91`: un try con dos catch dejaba vivo el segundo, preexistente, las dos VMs), y el
«la interfaz cambió entre la pasada y la emisión» de la stdlib (un .mod versionado con CRLF fuera de outDir se tomaba como
entrada). `compat/v10_casos.py` reproduce lo arreglado (13/13). Redes: frontend 220/220, miVM 51/51, C tests OK, compat
check verde (69/59/9/12), mezcla MOD9 120/120, la stdlib como MOD8 y MOD9 sin errores, Pico enlaza.

**J-3 decidido y hecho** (Eduardo: «salvo los private todos son virtuales»; `super.m()` = el m EFECTIVO del padre): ref
SUPER (kind 6) resuelta por la cadena en los dos cargadores; mezcla MOD9 130/130 con `ExtD`.

**La Pico carga MOD9** (noche, firmware de hoy grabado con `flash_pico.py` sin BOOTSEL): `Ext` MOD8 y MOD9 con el stdout
esperado; enlace 995 µs (MOD8) → 1.655 µs (MOD9: 1 clase construida, 11 refs, 66 B de arena, 4× menos strcmp). Ver `FICHAS`
`#523` §16-sep noche. Quedan en `/app` de la Pico `Lib.mod`/`Ext.mod` (inofensivos; el IDE los pisa en cada Run).

**Orden para retomar**: (1) las 8 fichas nuevas `#525`-`#532` son preexistentes: `#525` (null en campos, miVM en silencio)
y `#526` (print de un Thread ejecuta run) son las graves; (2) el plan de `#523`: filas de V4, V6, V5, V9, V8 (la Pico), la
cadena en MOD9 (V11: MOD_FORMAT con la ref SUPER y la 4.5, OPCODES, HEAP_LAYOUT `+6`, WIRE E0-E10) y los docs.

## 🧬 `#523` EN LAS DOS VMs (16-sep) — el enlace tardío corre: MOD9 se compila, se carga en miVM y en la VM-C, y la mezcla da 84/84 con el criterio de Java

**Hecho hoy, por la cascada** (commits `c0f116b0` → `eb13a8bf`, ver `FICHAS` `#523` §16-sep): V0 línea base (cronómetro del enlace; la Pico no
estaba conectada — sus ciclos quedan para cuando lo esté), F1 clave canónica, F2 formato MOD9 (escritor + `TablasMod9`), F3a el emisor
(`--formato=9`), F4 miVM carga MOD9 (`EnlaceTardio.java`), F3b nombres R4 con firma, la VM-C carga MOD9 (`link.c`), la reflexión de
Forms en C. **84/84 en las dos VMs, byte-idénticas**; MOD8 (el formato por defecto) intacto con todas las redes en verde;
Pico, DK2, P4 y sim compilan (S3/C6 no reconstruidos hoy: comparten el mismo loader).

**Trampas del día**: el lector de cabecera del compilador (`Main.java:1690`) rechazaba MOD9 en silencio («no se localizó .bpi») — el
comentario de `#457` lo predecía; la discovery de miVM leía mal la cabecera MOD9 (v8a); `gen` regenera los goldens V2 versionados
(NO ejecutarlo: `check` basta); los heredocs de bash siguen comiéndose `\n` y `'` (Write tool para los scripts, siempre).

**El AOT en MOD9 (fase 1), a pregunta de Eduardo** (*«en AOT se llamaba a un método de clase por la Vtable, ¿funcionará?»*): estaba
en el diseño y no en el código. Hecho: en MOD9 una `native` sólo hornea ranuras de clases CERRADAS del propio módulo (externa o
abierta → rechazo nombrando la clase); los nombres que pide al intérprete son los R4 con firma; si un símbolo no está, el C
generado lo DICE; y `cEscapeBytes` pasa de hex a octal (bug preexistente: `\x` glotón se comía una letra). Ver `FICHAS` `#523`.

**Orden para retomar**: (1) el guardián `--barajar` (V1) y las refs SUBTIPO (A1); (2) `instanceof Mod.Cls` (D7), `bpvm_mods` por `gen`,
bpi 9; (3) las filas nuevas de la matriz (V4) y los fixtures del cargador (V3); (4) **la revisión adversarial del código** (V10)
antes de tocar la cadena; (5) V9 medidas y V8 placas (la Pico); (6) la cadena en MOD9 (V11), con `build_mdn.sh`/`AotBuild`
pasando el formato al AOT. `#524` abierta (intrínseca desconocida → builtin 3 en silencio).

## 🧬 `#523` DECIDIDA Y DISEÑADA (15-sep, noche) — enlace tardío: la clase se adapta al cargar; «validado al 100x100» escrito antes del código

**Cómo se llegó**: el mapa de `#514`/`#516` (`3fa48e69`) → Eduardo: *«llevamos como 5 versiones dando vueltas a los slots… cerrar el tema de
forma definitiva»* → tres rondas de diseño en chat (§H2.6) → *«tú defiendes que nada de esto es posible, pero yo no termino de creer»* →
**`compat/mezcla.py`** (`59558202`): 84 filas en las dos VMs, línea base ANTES de tocar nada — la norma cumple lo que dice (75), el
compilador falla con la misma clase (5, `#514`) y **el camino tolerante tiene cuatro agujeros** (firma de cabeza, retorno, tipo de
property, firma de función de módulo: el marcador conserva el nombre en la ranura y pasa; yo había afirmado lo contrario) → Eduardo:
*«en Java hay compatibilidad hacia atrás… lo ideal es que pudiésemos hacer algo similar»* → **la norma**: *«lo que puede resolver el
compilador es estático y lo que puede resolver el cargador es dinámico»* → `#523` (`dd541e7c`) → el diseño en `V7_IDEAS.md` §H2.7
(`8bab1fd3`: 6 lectores + 3 diseños + 2 jueces + síntesis + crítico; las 11 correcciones del crítico incorporadas).

**Estado**: diseño escrito, código sin tocar. **Esperan las decisiones D1-D10** (final de §H2.7). Tamaño: 17-23 sesiones en cascada. **El criterio de Eduardo al cerrar el día** (15-sep, noche): *«Me da igual lo que tardemos, lo importante es que sea una
buena solución y que no tengamos que estar revisando en cada versión.»* — el tiempo no es la restricción; la calidad y la
validación al 100 % sí. Mañana se empieza por las decisiones D1-D10, y después V0 (la línea base, antes de tocar nada).
Fichas abiertas hoy: `#521` (el `.mod` se escribe con errores del emisor), `#522` (enum: `"c=" + c` revienta el heap).

**Orden para retomar H2 (cambia)**: (1) las decisiones de Eduardo; (2) **`#523`** por la cascada — V0 línea base con cronómetro en las
dos VMs y ciclos en la Pico (ANTES de tocar nada) → frontend (`claveCanonica`, bpi 9, refs 3.3, CLASES 4.5, 11 sitios `_REF`,
`--barajar`) → miVM (cargador + 11 opcodes + tests JUnit V3) → VM-C host → Pico → stdlib MOD9 + docs → V4-V10; (3) **la cadena una sola
vez, en MOD9** (con `bpvm_mods_sincronizar` comparando `gen`); (4) `#514`/`#516` se cierran «por construcción» al final, no antes;
`#520`/`#521`/`#522` cuando toque. **La Pico está conectada** (Eduardo, tarde): sirve para V0 (ciclos del enlace hoy) y para V8.

## 🌗 `#493` CERRADA (15-sep, tarde-noche) — la zona de packs deja de disfrazarse de `/app`: un orden de módulos en las dos VMs, el aviso «tapa al de /lib», el XIP vuelve, un pack es un docker

**Commits**: `a4cec0a3` (el grueso), `be7dc131` (STAT por nombre mira la zona), `6442b839` (los arreglos de la revisión
adversarial). Método: mapa verificado (6 lectores + 5 refutadores + crítico, 3,2 M tokens) → las decisiones de Eduardo
→ implementación → revisión adversarial (5 lentes + crítico, 1,6 M tokens) → 8 arreglos. **Lo que la revisión cazó y
habría llegado a placa**: el principal de la zona daba `NOT_FOUND` por el wire (las cinco imágenes), un overlay sin
apagar (use-after-free) que hacía mentir al STAT, `fileExists` ciego a la zona, un aviso falso al ejecutar desde `/lib`
(paridad), y **miVM no ejecutaba ningún módulo con `library` que importara la stdlib** — preexistente, la VM de
referencia derivaba la library al revés que el compilador y la VM-C, y ninguna red lo veía (ahora `libimport.bp` en
el corpus). Detalle en la ficha.

**Red al cierre**: paridad 68/68, cápsula 59/59, norma 9/9, **sombra 12/12** (nuevo `check-sombra`, con el wire contra
el sim por `compat/sim_sombra.py`), miVM 40/40, frontend 193/193, test-fsfb/packres/packskip/pack (rotos desde el
31-ago, enlazan otra vez), test-modhdr/mdnreceta/npack/loaderr/sim-smoke; los cinco firmwares compilan. Fichas:
`#493` ✅, `#520` abierta (los rincones preexistentes de la resolución, todos de línea de comandos).

**Dos preguntas para Eduardo** (en la ficha): el pack en ejecución reclama recursos por basename en cualquier ruta
mientras la zona sólo contesta a `pack:` (asimetría de `#310`); y el paso 0 no avisa cuando un pack autocontenido
lleva su propio `Core` (es «docker», pero es la forma de `#496`).

**Orden para retomar H2**: (1) `#514` `#516`; (2) **la cadena**: `Stdlib.bpbuild` con `"version": 7`, `rm -rf
bpstdlib/out` y la stdlib rehecha (MOD8, LF), samples, fat-jar del IDE (⚠️ con el IDE cerrado; `compileFile` del Run
debe pasar la `version`), blobs de las cinco imágenes, `Stdlib.pack`/`SQLite.pack` (`#496`: su puente `.mdn` saldrá v2;
y con `#493` un pack de librería ya no tapa a `/lib`), `montar-zip.sh` a MOD8, el `exitCode` de carga de la Pico por la
tabla `#481`; (3) el cierre pequeño (redes con número + tanda corta en una placa por familia — con el XIP de verdad,
el aviso «sin huella» de un `.mdn` v1, el rechazo de un v2 ajeno, la Pico con `Stdlib.pack` y `STAT name=` antes del
primer RUN, y el sync → mount del STM32 en la lista); `#520` cuando toque limpiar.

## 🔏 `#441` CERRADA (15-sep, tarde) — el `.mdn` lleva la huella de su receta y las recetas viven en UN `.h`, por juego de instrucciones

**Commit**: `7e722ef8`. La decisión de Eduardo que lo moldeó: las recetas se nombran **por ISA/ABI, no por placa**
(«¿`RISCV32_ESP_P4`? No debería ser por juego de instrucciones nada más») → `ARMV8M_MAIN_SOFTFP` (Pico 2 y STM32U5
comparten `.mdn`), `RV32IMAFC_ILP32F` (P4), `RV32IMC_ILP32` (C3/C6, sin destino en el IDE aún). El `.h`
(`bpgenvm-c/include/bpvm_aot_recetas.h`) es la única fuente: el firmware la elige por las macros de su ISA y la hashea,
el IDE la parsea y compila con ella, `MdnPack` estampa el CRC en la cabecera v2 (24 B). El cargador rechaza un v2 de
otra receta (`MDN_ERR_RECETA`) y carga un v1 «a ciegas» con aviso. INFO publica `aotReceta`/`aotFlagsCrc`; el IDE
elige el destino por la receta de la placa y no sube un `.mdn` de otra. Detalle en la ficha (FICHAS §V7).

**Red al cierre**: frontend 193/193 (+`AotRecetasTest`), miVM 40/40, paridad 65/65, cápsula 59/59, fwd rechaza, norma
9/9, `test-mdnreceta` 10/10, `test-modhdr` 7 fixtures (con `NatNew8.mod`: MOD8 + blob v2, huella igual en Java y C),
`test-npack` revivido, `sim-smoke` OK; los cinco firmwares compilan y cada uno lleva su receta.

⚠️ **Trampa de sesión**: los heredocs de Bash me mangaron `\0`, `\n` y tabs en tres scripts seguidos → todo script
por el Write tool (memoria `escribir-ficheros-sin-destruirlos`). Y `sed -i` en MSYS convierte CRLF→LF: para tocar un
fichero CRLF, Python con `newline=''`. Un `git stash` + `pop` renormaliza a CRLF los ficheros con LF (inocuo: el
índice es LF).

**Orden para retomar H2**: (1) `#493` (la sombra: fallback sólo en el propio namespace, un orden de búsqueda, `fromPath`
en la VM-C, aviso por OUTPUT «cargado desde /app», `PackStep` sin stdlib, orden de arranque STM32, `test_fsfallback`
invertido, prueba de sombra/XIP en el sim); (2) `#514` `#516`; (3) **la cadena**: `Stdlib.bpbuild` con `"version": 7`,
`rm -rf bpstdlib/out` y la stdlib rehecha (MOD8), samples, fat-jar del IDE (⚠️ con el IDE cerrado; y `compileFile` del
Run del IDE debe pasar la `version` del proyecto), blobs de las cinco imágenes, `Stdlib.pack`/`SQLite.pack` (`#496`: su
puente `.mdn` saldrá v2), `montar-zip.sh` a MOD8, el `exitCode` de carga de la Pico por la tabla `#481`; (4) el cierre
pequeño (redes con número + tanda corta en una placa por familia — con el aviso «sin huella» de un v1 y el rechazo de
un v2 ajeno en la lista). **Eduardo (15-sep, mediodía): «ahora no puedo pruebas con placas, lo aplazamos»** → las
comprobaciones en placa de `#441` se quedan en esa lista; nada de lo que sigue en H2 depende de ellas.

## 🧱 MOD8 HECHO (15-sep) — la norma de versiones de `#500` en las dos VMs con el mismo texto, `#518` cerrada; queda la cadena, `#441` (`.mdn` v2) y `#493`

**Commits**: `5fc59ae4` (MOD8 + la norma + check-norma 9/9 + `#518b`) · `8d7e2674` (oráculos de `#518` en el corpus,
65/65). Método: mapa de 7 lectores + 7 refutadores antes de tocar (3,1 M tokens; sacó, entre otras cosas, que
`check_fwd` estaba en verde sin ejecutar el módulo compilado, que `Math` no deja imports —por eso el sello va por
módulo—, y que la pasada interfaz-only pierde las const sin tipo). Red al cierre: frontend 189/189, miVM 40/40,
paridad **65/65** (apps MOD8 sobre stdlib MOD7), cápsula V6 59/59, fwd RECHAZA, norma 9/9, samples 348/348,
test-modhdr y los test-* con fixture OK.

**Las decisiones de Eduardo hoy**: la norma de `extends` («los hijos pueden ser más recientes que los padres; los
padres no pueden ser más recientes que los hijos») en vez del marcador de layout — pidió el marco y el detalle caso
por caso antes de decidir (usar ≠ extender). Pendiente de su respuesta: **la huella del `.mdn`** (pieza 4, `#441`):
propuesta `bpgenvm-c/include/bpvm_aot_recetas.h` como única fuente (un literal por destino que el firmware hashea y
`AotBuild` parsea), cabecera v2 de 24 B, INFO publica `aotFlagsCrc`; el mapa tiene el detalle (14 sitios).

**Orden para retomar**: (1) `#441` (`.mdn` v2) si Eduardo da el visto bueno a las recetas; (2) `#493`; (3) `#514`
`#516`; (4) **la cadena**: `Stdlib.bpbuild` con `"version": 7`, `rm -rf bpstdlib/out` y la stdlib rehecha (MOD8),
samples, fat-jar del IDE (⚠️ con el IDE cerrado; y `compileFile` del Run del IDE debe pasar la `version` del proyecto),
blobs de las cinco imágenes, `Stdlib.pack`/SQLite.pack, `montar-zip.sh` a MOD8, el `exitCode` de carga de la Pico
por la tabla `#481`; (5) el cierre pequeño (redes con número + tanda corta en una placa por familia).

## ⏭️ AL RETOMAR (14-sep, tarde) — B de `#492` casi entera: campos, `final`/`private`, ESTÁTICOS y CONSTRUCTORES ×N cruzan el módulo; `#509` `#510` `#513` cerradas. Falta la subida de formato (MOD8/`#500`) y su cadena

**Commits del día, en orden**: `4a6ea0f4` (`#508`) · `160e849b` (campos, `bpi 8`) · `7680998a` (`#512`) ·
`f4a3ca7e` (`final`/`private` en la interfaz, `raise` heredado por ranura, campos privados fuera) · `024f5b24`
(**estáticos**: `staticmethod`/`staticprop` + alias plano `__cls_s_`/`__cls_sg_`/`__cls_ss_`; **`#510`**) ·
`9381c607` (**ctor ×N**: `ctor+(...)`, `__cls_init_/__cls_new_<Cls>$clave`, `super(args)` por firma; **`#509`** y
**`#513`**). Estado de la red al último commit: frontend **189/189**, miVM 39/39, paridad **63/63** (tres oráculos
nuevos: `CamposHijo`, `EstaticosHijo`, `CtorHijo`), cápsula V6 59/59, samples 348/348, test-modhdr OK. **La stdlib
es byte-idéntica** (no usa estáticos de clase ni hereda constructores): sus `.mod` y blobs NO cambian por esto.

**Lo que Eduardo dijo hoy y manda**: *«sólo preguntaba, no hace falta que modifiques algo que ya está hecho»* (los
estáticos siguen emitiéndose y exportándose como `Cls.m`; se AÑADE el alias y la línea de interfaz) y *«los métodos
estáticos siempre van precedidos por el nombre de la clase»* — justo el nudo: el punto en el nombre es lo que los
cargadores parten mal (penúltimo punto = módulo).

**Método del día**: un mapa de 7 lectores + 7 refutadores ANTES de tocar (todo verificado línea a línea; sacó `#513`,
la trampa del flag `static` en `method` —un lector viejo daría ranura en silencio— y la causa exacta de `#510`), y
al final una **revisión adversarial** (5 lentes + 5 refutadores con reproducción) de los tres commits: **34
hallazgos, 30 reales** — seis regresiones del día (la factoría por ctor efectivo sin que la interfaz lo publicara,
la clave posicional de los estáticos, el separador `_` del alias, la colisión con funciones de usuario que rompía
la paridad 7/7 vs 999/999, `super()` ambiguo, el rótulo del Disasm), cinco preexistentes del mismo camino, y seis
preexistentes con ficha (`#514`–`#519`). Todo lo arreglable el mismo día: `720392a6` (+ `e439d310` y el lote de
docs). **Lección del día**: el mapa previo (7 lectores + 7 refutadores) evitó errores de diseño; la revisión
posterior cazó lo que el diseño no ve — los casos que nadie escribe (cabeza privada, `A_b.c`, una función llamada
como el alias, `extends Thread` sin ctor). Las dos juntas, ~4,3 M tokens de agentes; ningún test de la red los
habría visto (compilan limpio y ejecutan con paridad lo que no es).

**La norma de Eduardo (14-sep, tarde)**: *«tanto las constantes como las variables estáticas están pensadas para
acceder a ellas a través del CS … si queremos acceder desde fuera de un módulo debería ser a través de getters y
setters del módulo»*; *«no hay nada que impida que se acceda desde otros módulos o AOT siempre que se cumplan las
normas de visibilidad y se utilicen getters y setters»*; *«el CS es válido para el código del módulo, no para
código externo»*. Consecuencia escrita: la `var` estática no cruza; cruza la property.

**Orden para retomar (acordado con Eduardo al cerrar el 14-sep — «¿H2 queda cerrado?» No: cerrada la pieza 3)**:
(1) MOD8 + marcador de layout + `#441` en UNA subida de formato; (2) `#493`; (3) `#514` `#516` `#518` con la
interfaz y los cargadores aún abiertos; (4) la cadena (stdlib, samples, fat-jar del IDE con el IDE cerrado, blobs
de las cinco imágenes, `Stdlib.pack`, `MOD_FORMAT.md` a v8) y el cierre pequeño. `#515` `#517` `#519` son de H1/AOT.

**Queda de H2** (`V7_IDEAS.md` §H2): la subida de formato **MOD8** (`gen` + `ifaceCrc`, `#500`), el marcador de
layout `Clase#layout#nF#nM` al enlazar, `#441` (huella del `.mdn`), `#493` (fallback de packs, `fromPath` en la
VM-C, ruido en OUTPUT, pack sin stdlib, orden de arranque STM32), y con ello **la cadena**: stdlib final, samples,
fat-jar del IDE (⚠️ con el IDE cerrado), blobs de las cinco imágenes, `Stdlib.pack`, docs (`MOD_FORMAT.md` §4.1:
varios nombres por offset y los `__cls_*`; el manual ×3 dice *«los estáticos no cruzan»* y `PENDIENTES` L18 *«sólo
la primera sobrecarga del constructor cruza»*: ya es falso). Tests pendientes de H2: la norma (4 casos), el
marcador de layout, sombra/XIP en el sim. De H1 siguen `#506`, `#507`, `#511` (decisión de Eduardo).

## 📐 H2 ANALIZADA (14-sep) — la matriz de `#492`, B decidido, el mapa de las dependencias y el diseño de H2 en `V7_IDEAS`

**Eduardo abrió el día con «Empezamos V7» y, a mi primer desvío (`L20`), «¿Empezábamos con H2, no?».** Día de análisis:
ni una línea de código de producto.

1. **`#506` abierta** de paso (L20 reproducido: el compilador devuelve exit 2 y escribe `.mod`/`.dbg`/`.slots`; las dos VMs
   ejecutan el programa sin su bucle; causa en `MivmEmitter.emitTo:364-380` y `Main.emitMivmMod:975`; `step -1` falla sólo
   porque `-1` llega como `UnaryExpr`).
2. **La pregunta de Eduardo sobre `#492`** — *«si es el único caso o hay más»* — contestada con una **matriz** de 24 casos
   (`notas/v7-apertura/matriz492.py`: una base con un miembro de cada clase, `Der extends Basemod.Base` en otro módulo,
   y el control en el mismo módulo). **No cruzan**: el campo protegido, el campo heredado leído desde una `native` (el
   caso que él recordaba: mismo agujero), y el `raise` de un evento de la base (el emisor peta). **Cruza lo que no
   debería**: `final`. **No cruzan por «v1»**: método y property estáticos. Y cuatro bugs del compilador que no son de
   la frontera, fichados: `#507` (construir la propia clase → excepción del emisor), `#508` (`private` perdido en el
   parser para `var`/`property`: una subclase lee un campo privado), `#509` (`super(5)` con constructor sobrecargado),
   `#510` (`Base.MAX` dentro de una derivada: *«el tipo 'void' no tiene miembros»*).
3. **Las dos opciones de `#492`, medidas**: campo → property cuesta **×2,25** por acceso (86 → 193 ms en VM-C, 2 M
   accesos) y **+91 B** por campo (≈74 de RAM); exportar el campo en la interfaz, ≈25 B de flash en una sección que la
   VM salta. **Eduardo decidió B** (*«OK B, me parece bien»*), generalizada: la interfaz exporta la superficie completa
   que un descendiente puede tocar (campos, `final`, estáticos) + el emisor de eventos + un marcador de layout en el
   enlace. `#508` va antes (sin `private` de verdad, B exportaría los privados).
4. **El mapa de H2** (workflow: seis lectores + un verificador, 1,3 M tokens, `notas/v7-apertura/mapa_h2.json`) — tres
   hallazgos que cambian el diseño: el `.mod` **no lleva ningún dato de versión** (la única «versión» del ecosistema es
   el dígito del `MAGIC`) y el enlace **no compara el layout del padre** horneado en el hijo; la zona de packs contesta
   al `stat` de **cualquier ruta** por basename (por eso tapa `/app` y `/lib`), y **la rama XIP de `discover_deps` es
   inalcanzable** — los módulos de un pack se cargan a RAM por stream; el compilador resuelve la stdlib PRIMERO y las
   dos VMs la ÚLTIMA; miVM honra `fromPath` y la VM-C lo tira.
5. **El diseño de H2, escrito en `V7_IDEAS.md` §H2** (cuatro piezas, una sola subida de formato `MOD8`/`bpi 8`/`.mdn`):
   `#500` con `gen` (versión declarada del proyecto) + `ifaceCrc` por módulo y por import, la norma en el cargador
   (más antiguo → rechazo; misma `gen` y otra interfaz → rechazo; más nuevo → si enlaza, con marcador de layout) y el
   rechazo por `OUTPUT`; `#493` con el respaldo contestando sólo en su espacio (vuelve el XIP), orden único, ruido por
   `OUTPUT`, builder sin stdlib, arranque de STM32, `test_fsfallback.c` del revés; `#492` = B; `#441` huella de flags en
   el `.mdn`. Orden de trabajo y el cierre pequeño de H2 (la matriz como test de la suite, test de la norma, `compat`,
   `tanda` corta en P4 y Pico) al final del apartado.

6. **Y la condición de Eduardo para la subida de formato**: *«puede ser fuente de problemas de compatibilidad; hay que
   probarlo bien, con los ejemplos de siempre más tests hechos a medida»*. Escrita como §H2 punto 4: las redes de
   siempre con número + siete tests a medida (el gate en las dos direcciones — incluido **`MOD8` en las VMs del ZIP de
   V6**, que debe rechazar con mensaje —, la cabecera con todas las secciones, los cuatro casos de la norma, el marcador
   de layout, la matriz entera, la sombra y el XIP en el sim, y byte-idéntico con V6 en lo que no cambia). **Primero los
   tests, luego el formato.** Los constructores sobrecargados (`#509`) viajan en la misma subida.

7. **Y empezó el código de H2** (Eduardo: *«cuando quieras podemos empezar»*): **`#508` cerrada** (`4a6ea0f4`) — el `private`
   de `var`/`property` ya vive en el AST, los símbolos y `checkVisibility`; stdlib byte-idéntica (28/28, sin regenerar blobs),
   `samples_sweep` 348/348; y **la matriz de `#492` es un test de la suite** (`HerenciaCrossModuleTest`, 50 casos de
   caracterización con los agujeros anotados; la suite del frontend pasa de 110 a **160**, todos verdes). El fat-jar del IDE
   NO se ha rehecho (se rehace con la subida de formato; hasta entonces el IDE no aplica `#508`).

8. **Decisiones de Eduardo sobre la subida de formato** (14-sep, tarde): *«el compilador compila una versión concreta, no
   varias; claramente V8, es un cambio importante; compatibilidad hacia atrás sí — es aditivo, los V7 tienen que poderse
   ejecutar sin problemas; la stdlib se rehace cuando terminemos todos los cambios del formato, mientras tanto se recompila lo
   necesario»*. → El compilador emite sólo `MOD8`; **las VMs cargan `MOD7` y `MOD8`** (sin `gen`/`ifaceCrc` la norma no se
   aplica y el módulo corre como hoy); `MOD8` en las VMs de V6 se rechaza con mensaje; la stdlib `MOD7` de hoy sigue cargando
   durante todo el trabajo (la prueba de compatibilidad, de gratis). Escrito en `V7_IDEAS.md` §H2 (pieza 1 y apartado 4).

9. **Los tests que no dependen de las decisiones, hechos** (Eduardo: *«adelante con los tests»*): (1) la **cápsula V6**
   en `compat/compat.sh` — `gen-v6`/`check-mod7`/`check-fwd`, con el ZIP publicado como oráculo; **59/59 hacia atrás**,
   hacia delante «V6 ejecuta lo de hoy» (`50f73178`); (2) **`make test-modhdr`** — la cabecera y las secciones del `.mod`
   contra el cargador, cinco fixtures con native ARM/RISC-V (relleno 0/1/3 B) y `library`; y **`Disasm` aprende `MOD7`**
   (se desplazaba: no leía `nativeSize`) (`050739bf`); (3) **`InterfazIdaYVueltaTest`** — la interfaz embebida de los 28
   módulos de la stdlib, byte a byte (`e86f9f3f`). Suite del frontend **188/188**; `compat.sh check` = paridad 59 +
   mod7 59 + fwd, todo verde. `.gitattributes` nuevo: goldens LF, binarios binarios.

⏭️ **Al retomar: cuatro decisiones de Eduardo** (`V7_IDEAS.md` §H2 punto 5) y a programar H2 en el orden del punto 3:
(1) `gen` = versión del proyecto (`.bpbuild`) ¿o por módulo?; (2) con `gen` mayor, ¿basta que enlace?; (3) `fromPath`
en las dos VMs o fuera; (4) el pack de librería sin stdlib por defecto; (5) una sola subida de formato para H2 y `#441`.
Primer código: `#508` (parser) → la subida de formato. ⚠️ Toca el frontend: rehacer stdlib (`rm -rf bpstdlib/out`),
samples, fat-jar del IDE (cerrado), blobs de las cinco imágenes. Host sigue en sabor `gui1-lvgl1`.

📌 **Estado del repo**: todo commiteado, sin push. Temporales de hoy en `C:/tmp/v7l20`, `C:/tmp/v7_492`, `C:/tmp/v7_self`,
`C:/tmp/v7_coste` (desechables).

---

## 🧬 V7 ABIERTA (13-sep) — V6 comprobada cerrada, el censo de V7 hecho ficha a ficha, y `FICHAS` con V7 en el centro

**Encargo de Eduardo:** *«Comenzamos, V7. Quizás lo primero es asegurar que V6 está cerrado y poner V7 en
el centro del trabajo.»*

**1. V6, cerrada — medido, no supuesto.** `main` == `origin/main`; tag `v6.0` sobre `9cb1a7bd`; la release con su
único artefacto; Pages `built` sobre `84dce0d0`; `FICHAS` con 0 fichas y 0 hitos de V6. Lo que aún decía «V6 en
curso» ya no lo dice: `V6_BACKLOG.md` marcado **ARCHIVO** (molde de V5), `V6_IDEAS.md` remite a `V7_IDEAS.md`,
`PENDIENTES.md` apunta a `V7_*`, `CLAUDE.md` dice «V7, en curso desde el 13-sep», el pre-flight de `PUBLICAR.md`
deja de decir «por pasar». El paso (7) del plan de publicar (`HECHO_V6.md`) queda cumplido como se hizo en V5: el
`V6_BACKLOG.md` archivado ES el snapshot; `HECHO_V*` era la convención de V1/V2. De paso: `C:\tmp\bp6` había
llegado a `ESTADO`, `PUBLICAR` y `FICHAS` como `C:<TAB>mp<BS>p6` (el heredoc de anoche se comió las barras) —
reparado, y la memoria del heredoc anotada porque volvió a morder hoy al repararlo. Y la raíz del repo limpia de
temporales (con el OK de Eduardo): `TestPacks/`, `boardsim.flash.fs`, los `.log`/`.txt`/`.bin` de samples y los
compilados rancios de `samples/benchmarks/` (el `VivoLargo.mod` estaba versionado: restaurado).

**2. El censo de V7** — con un workflow (288 agentes, ~26 M tokens; se cortó a mitad por el límite de sesión de
Eduardo y se reanudó desde la caché): 26 barridos sobre `FICHAS.md` entero, `ESTADO.md`, los dos backlogs,
`V6_IDEAS`/`V7_IDEAS` y `PENDIENTES` → 448 candidatas en bruto, 261 únicas; **cada una releída en su sitio y
buscada en todo el registro** por si se había cerrado después; un crítico de completitud hizo el `grep V7` contra
la lista (1 hueco). Resultado: **92 cayeron** (cerradas sin tachar —`Object` comodín hecho desde el 18-ago—,
duplicadas, superadas) y quedan **19 con número** (18 fichas + `L1`), **28 restos** de fichas cerradas de V6,
**~40 sueltos** y **6 fuera del plan**. Todo en **`docs/V7_BACKLOG.md`** (nuevo, 90 entradas por temas A–J, con
qué queda de cada una y su puntero a línea del `FICHAS` nuevo), y la cuenta en la cabecera de `FICHAS` §ABIERTAS.
Un hallazgo que no es una ficha: el triaje de `#473` pedía ficha nueva para 10 «rompe-el-modelo» vivos y los
lotes D/E de `473_TRIAJE_COLA.md`, y nunca se abrieron.

**3. `FICHAS.md` reestructurado sin borrar una línea** (verificado por multiconjunto: las únicas 6 líneas cuyo
texto cambia son 3 rótulos, la del freeze V6 —ahora LEVANTADO— y las dos viñetas de §Aplazadas que el censo
encontró sin tachar: `Object` comodín y `#426`): arriba el bloque **🚀 V6 PUBLICADA** (molde del de V5, que baja
junto a «CERRADAS EN V5»); §ABIERTAS es V7 con el censo; las **12 fichas abiertas** (`#452` `#468` `#470` `#479`
`#487` `#490` `#491` `#492` `#493` `#495` `#500` y `L1`) **movidas enteras** al bloque «V7 — lo que viene de V6»;
la cola heredada de V5 (que V6 no abrió) también, con su rótulo puesto al día; y el resto de V6 bajo
«📦 CERRADAS EN V6», encabezado por el censo del 12-sep tal como estaba. Script y piezas en `notas/v7-apertura/`.

**4. Lo que Eduardo nombró de memoria al abrir V7** (la mejor señal de por dónde empezar): los periféricos de cada
micro (`#378`/`#470`/`#479`), las dependencias entre clases de módulos distintos en el `.mod` (`#492`/`#500`/`#460`),
la posición dentro de un fichero (`#491`) y la clase `File` (`#491` + el destructor de `L1`). Está escrito en
`FICHAS` y en `V7_BACKLOG.md` §🎯.

**5. Las decisiones de Eduardo, la misma tarde**, tras ver los pendientes agrupados por temas:
- **Un hito por tema, y los hitos se llaman `H` + número** (*«así es más fácil hacer el seguimiento»*): `H1` el
  módulo raíz (era `L1`), `H2` dependencias del `.mod`, `H3` ficheros y `File`, `H4` la HAL BP de periféricos, `H5`
  AOT, **`H6` LVGL en un pack** (hito propio por decisión suya), `H7` memoria y arranque, `H8` IDE y wire, `H9`
  pruebas, `H10` placas y drivers, **`H11` la ESP32-C5** — Waveshare **ESP32-C5-Touch-LCD-2.8** (2,8", 240×320, táctil), 8 MB PSRAM, 32 MB flash, TF, RTC; comprada el 5-sep.
- **El P4X no tiene fecha** (*«lo pedí pero me enviaron la versión P4»*): sale del plan; la C5 lo sustituye.
- **Sin orden prefijado**: *«como hay muchas cosas pendientes hay que ir cerrando algunos pendientes»*.
- **La regla que hace distinta a V7**: *«intentaremos no dejar pendientes para V8»* — lo que se abra se cierra o se
  cancela dentro de la versión; aplazar deja de ser una salida. Y la norma de numerar (una entrada es una cosa)
  se aplica al meter mano a un resto, no al censar.
Todo escrito en `FICHAS` §ABIERTAS (la tabla «LOS HITOS DE V7») y en `V7_BACKLOG.md` (§🧭 y los rótulos `H1`–`H11`;
`H6` y `H11` con sección propia; el P4X en «fuera del plan»).

**6. Y dos cosas de fondo, de Eduardo, la misma tarde** (*«hoy no implementamos, solamente analizar; llevamos 3 semanas sin
parar»*): **`H6` es estratégico** — *«Meter LVGL en un pack es estratégico, ya que hoy en día cualquier micro puede llevar una pequeña pantalla, así que le corresponde al usuario si quiere una pantalla o no, y no a nosotros»* — y **`A4` vuelve como `H12`**: *«Al final de V7 podemos hacer una prueba a ver si se puede trabajar con 2 núcleos reales: si funciona se mantiene, pero si da problemas y no somos capaces de solucionarlos, lo echamos para atrás»*. Escritas en `FICHAS` (tabla de hitos) y en `V7_BACKLOG.md` (§🧭, `H6`, `H12`).

**7. Lo que añadí yo, con su permiso** (*«eres libre de hacerlo, a fin de cuentas somos socios»*), como PROPUESTAS en
`V7_BACKLOG.md` §🤝 y resumidas en `FICHAS`: cerrar por hito y no al final (definición de «cerrado»: fichas + prueba que
lo ejercita + docs + índice tachado); lo silencioso salta la cola (`L20`, los diez de `#473`); `H2` es la red de `H1`;
`H6` se demuestra en la C5 y nace segura para `H12`; un repaso del índice a los seis hitos. **Contestadas la misma
tarde**: *«de acuerdo en todo salvo en el punto 1»* — no un `F1` por hito, sino **un cierre pequeño por hito** (ronda corta
de tests) **y uno casi completo a mitad de V7**. Y la forma de `H6`: `H6.a` el pack en la C5 (*«inofensivo»*), `H6.b`
quitar LVGL de C6/P4/DK2 (*«más delicado»*) sólo tras **una prueba completa sellada como punto de restauración**.

**8. Y el cierre del día** (Eduardo): *«con esto V7 queda bastante completo. En principio no hace falta añadir nada
más, salvo los bugs que vayan apareciendo»*. **El alcance de V7 está cerrado el mismo día que se abre**: doce hitos y su
índice; a partir de aquí sólo entran bugs, con su ficha. Día de análisis, sin una línea de código.

⏭️ **Al retomar**: elegir por dónde empezar a cerrar — la sugerencia sigue siendo `H2` (`#500`/`#493`/`#492`, pequeño
y cierra lo que mordió en `F1`) y `H1` después; pero es de Eduardo. Sin decidir nada se pueden cerrar ya `N2`
(minutos) y la limpieza de `H9` (`samples/out` MOD6; los `.mod` de prueba en las placas, con OK placa a placa).
Cuando llegue la C5: `H11` por el camino de `P1`/`P2`, y comprobar en la hoja del chip si lleva FPU (decide el
destino AOT, `#502`).

📌 **Estado del repo**: todo commiteado, **sin push** (V7 en curso: push sólo al cerrar versión). Host en sabor
`gui1-lvgl1`: `make GUI=1 LVGL=0` antes de `compat.sh check`. El IDE del ZIP sigue desplegado en `C:\tmp\bp6`.

---

## 🚀 V6 PUBLICADA (12-sep, noche) — tag `v6.0`, release con el ZIP, Pages en V6; **V7 arranca**

Con el «OK» de Eduardo: tag `v6.0` sobre `9cb1a7bd`; `git push origin main --tags` (477 commits; salió también
el tag local `v1.0`, histórico); `gh release create v6.0 dist/BasicPlus-6.0-win.zip --notes-file
dist/release-v6.0.md`; el build de Pages **no arrancó solo** con el push (el último era del 22-ago) y se relanzó
con `gh api -X POST repos/legio-e/BasicPlus/pages/builds` → `built` sobre `9cb1a7bd`; portada ES y EN, `gui.html`
y la captura comprobados en el navegador; el ZIP descargado de la propia release y su sha256 `2279292d…`
contrastado con el montado. `CLAUDE.md` dice ya «V6 publicada; lo siguiente es V7». Memorias: `cierre-v6`,
`abrir-puerto-reinicia-esp`, `dos-esp-mismo-com`; el freeze marcado como histórico.

⏭️ **Al retomar (V7)**: leer `FICHAS` — lo que quedó dicho para V7: `#493`/`#500` (un pack de librería no tapa la
stdlib; la norma de versiones de las dependencias), `#502` (destino `ilp32` y la float-ABI en la cabecera del
`.mdn`), `#495` (`GET` durante un RUN), `#487`, el INFO de las ESP/STM32 sin `packsXipBase`/`sqlite*`/`floatAbi`,
y `L1` (el módulo raíz). Y el host está en sabor `gui1-lvgl1`: `make GUI=1 LVGL=0` antes de `compat.sh check`.

---

## ⏭️ AL RETOMAR (13-sep) — ✅ **`F1` CERRADA**: V6 está LISTA; falta el gesto de publicar (tag `v6.0` + push + release con el ZIP)

**Lo que pasó el 12-sep por la noche** (la sesión más larga de V6; el detalle está en la sección `F1` de `FICHAS`):
1. **Guardas del PC** en verde — y la suite del frontend estaba ROJA desde `#458` (fixture sin `import Core`).
2. **Siete imágenes** del mismo árbol; la regeneración de la stdlib embebida destapó que iba **sin `I2c.mod`**.
3. **Nueve placas** regrabadas desde `dist` y pasadas por `tanda.py` (Eduardo rotó: Pico, Metro, DK2, Nucleo, C3,
   S3, P4 kit, P4 Waveshare, C6). Todo verde. Aprendido: Windows dio el mismo COM3 a la C3 y la C6; abrir el
   puerto serie reinicia las ESP (DTR/RTS al EN); en el P4 un pack grabado gana a `/app` y `/lib` y el runner
   **para** (correcto: `#493`).
4. **La BD estaba ROTA en placa desde el 23-ago** por dos causas superpuestas — `#503` (guarda muerta: el P4 no
   cargaba ningún `.mdn` desde `#465`) y `#504` (el puente `.mdn` del pack de SQLite era de ABI 4) — y nadie lo
   vio porque nadie ejecutó una consulta. `SqlDemo` == host en los dos P4 (y en la tarjeta), `Bench` nativo 32 ms.
5. **`#502` medida y cerrada** en el cargador de `.mdn` (un blob del P4 REINICIABA la C6; ahora se ignora con
   mensaje); **`#379` cerrada** (8/8 con la tarjeta); **`#505`** (el simulador sin pantalla no drenaba el evento
   del handler — lo cazó Eduardo probando el ZIP).
6. **El ZIP**: montado, verificado por cuatro lentes independientes (workflow), desplegado en `C:\tmp\bp6` y
   probado; **el IDE lo probó Eduardo: OK**. Lo que cazaron las lentes y el empaquetador está corregido (cinco
   drivers MOD6, `Stdlib.pack` MOD6 del 20-ago, cerrar la ventana del simulador, `board.json` de la Metro,
   `packsDir` en `BpVM.cfg`). Decisión tomada por mí, dicha a Eduardo: **el simulador del ZIP lleva SQLite**.

⏭️ **Al retomar — PUBLICAR, con el OK de Eduardo** (es lo único que falta y es de fuera): (1) `git status`
limpio; (2) tag `v6.0` sobre el último commit; (3) `git push origin main --tags` (~480 commits por delante);
(4) `gh release create v6.0 dist/BasicPlus-6.0-win.zip --title "BasicPlus v6.0 — orden en los micros"
--notes-file dist/release-v6.0.md` (el cuerpo ya está escrito, ES + EN, con el sha256 del ZIP `2279292d…`);
(5) comprobar el build de Pages (`gh api repos/legio-e/BasicPlus/pages/builds/latest --jq .status`) y abrir la
web; (6) descargar el ZIP de la release y verificar el sha256; (7) `docs/HECHO_V6.md` (snapshot inmutable) y la
memoria de cierre. ⚠️ Antes de montar el ZIP otra vez por lo que sea: el IDE cerrado, y el host está en sabor
`gui1-lvgl1` (para `compat.sh check` hay que volver a `make GUI=1 LVGL=0`).

📌 **Estado del repo**: todo commiteado, sin push. Placas al escribir esto: Waveshare P4 (COM15), DK2 (COM12),
C6 (COM3). El P4 kit y la Waveshare llevan el `SQLite.pack` nuevo sellado y `SQLite=4` en el ENV.

---

## ⏭️ AL RETOMAR (12-sep, noche) — ✅ **`D1` CERRADA** (`72f9706d`); **`F1` en curso**: las guardas del PC pasadas, faltan las siete imágenes y las nueve placas

**Lo que pasó al retomar** (la sesión de la tarde se cortó con el verificador de `D1` recién terminado):
1. **Los hallazgos del verificador, corregidos** (`72f9706d`). Los tres graves: (a) *«el C3/C6 rechaza el blob
   del P4 por ABI»* era **falso** en cuatro documentos — el guardián de la float-ABI existe en los packs, no en la
   sección `native` del `.mod`, y el IDE compila con los flags del P4 para cualquier RISC-V → **ficha `#502`**
   (nadie lo ha medido; se mide en `F1` con el C6); (b) *«la stdlib no se sube nunca»* → la regla real de `#466`;
   (c) `doc_frags.py` en rojo con 126 «fallos» que eran «falta contexto» → la clase **TROZO** en la herramienta
   (0 FALLO; y con tres roturas inyectadas, tres FALLO). Más las medias y bajas (PrintBench 1,9-5,3×, UNICORE,
   `tempC` en STM32, las dos portadas a V6, nueve placas / verificadas ocho, `CatchSinTipo.bp` de vuelta a
   `samples/`…). El detalle, en la sección `D1` de `FICHAS`.
2. **Eduardo leyó `RELEASES.md` v6.0 y la dio por buena** (*«OK»*) → `D1` cerrada en `FICHAS`, `F1` abierta con su
   plan en cinco pasos (guardas del PC → siete imágenes → nueve placas por `tanda.py` → lo manual: BD en P4,
   `#379`, `#502` → publicar).

⏭️ **Al retomar**: seguir el plan de `F1` en `FICHAS` por donde diga la entrada de abajo de ésta (las guardas del
PC se corren primero y se anota su resultado aquí). Placas al escribir esto: sólo la **Pico (COM22)**; Eduardo va
conectando por el orden del plan (Pico → Metro → C3 → C6 → S3 → P4 kit → P4 Waveshare → Nucleo → DK2).

---

## ⏭️ AL RETOMAR (12-sep, tarde — sesión cortada por el límite de tokens) — 🧊 **V6 CONGELADA** (`ae60e2e9`); `D1` a medias, **commiteada como WIP** (`e9fc30f0`)

**Lo que hay que saber en un minuto:**
1. **El freeze está marcado** (`ae60e2e9`, raya de código en `843e2d0a`). Después han entrado **seis bugs con
   ficha y reproducción**, todos del inventario de `D1`: `#496` (SQLite/Orm sin `import Core`), `#497`
   (`README.es.md` destruido desde el 22-ago, publicado así en v5.0), `#498` (seis samples rotos + el
   diagnóstico que engañaba), `#499` (rutas de 128-255 → «IO error»), y **`#501`** (miVM: un `sleep` como
   última acción de `main` mataba 1 de cada 5 ejecuciones — un yield tardío pisaba el `HALT`; `5d02dfd1`).
   `#500` (V7, Eduardo): la norma de versiones de las dependencias. 59 PASS.
2. **`D1` está escrita pero NO verificada ni leída**: el commit `e9fc30f0` lleva toda la documentación (ES y EN,
   manual/referencia/gui/PENDIENTES/QUICKSTART/INSTALAR/guía IDE/RELEASES v6.0/portal) escrita por nueve
   editores en paralelo, cada uno compilando sus fragmentos, más `tools/doc_frags.py` y `tools/samples_sweep.py`
   y el checklist nuevo de `PUBLICAR.md`. **Un verificador independiente estaba en marcha al cortar**: su informe,
   si terminó, está en
   `C:/Users/Eduardo/AppData/Local/Temp/claude/C--lenguajes-pm-miVM/92dd594f-bfc5-43df-be19-03d8a397bfe5/tasks/waifpc93s.output`
   (clave `result.verificacion`). **Terminó justo al cortar — leído por encima, esto es lo que dice:**
   ✅ las falsedades del inventario han desaparecido de todos los documentos (0 apariciones vivas); ✅ la
   introducción de Eduardo en `RELEASES.md` es byte-idéntica; ✅ `samples_sweep.py` 347/347; ✅ la mayoría de
   las cifras de v6.0 cuadran con FICHAS. ❌ **Tres graves a corregir antes de cerrar `D1`**: (a) manual §8.3,
   referencia y RELEASES afirman que «el C3/C6 rechaza el blob nativo del P4 por ABI de coma flotante y sigue
   interpretando» — **el código no lo respalda** (`loader.c:437 → mdn_load` valida magic/version/abi/arch;
   comprobar qué pasa de verdad y decir eso); (b) `guia-ide.html:311` «la biblioteca estándar no se sube nunca»
   es **falso** desde `#466` (el IDE pregunta por cada dependencia y sube a `/lib` la que falte o sea vieja);
   (c) `doc_frags.py` como gate de `PUBLICAR.md` da **126 fallos**, todos «falta contexto» (los editores
   compilaron con arneses privados y los fragmentos que dependen del anterior no llevan `data-bp="sigue"`):
   o se encadenan los fragmentos en el HTML o el gate no puede exigir código 0. Medias: PrintBench «3,5-5,3×»
   cuando la Pico da 1,9× (rango real 1,9-5,3); «en el S3 y el P4 el otro núcleo atiende el cable» es
   **invención** (`CONFIG_FREERTOS_UNICORE=y`); `Machine.tempC` sí existe en STM32 (ADC1, `BOARD_HAS_ADC_TEMP`);
   el resto de los README sigue anunciando V5 fuera del bloque Estado; el parque baila entre 8 y 9 placas
   (FICHAS dice **nueve**). Bajas: desajustes ES/EN previos a V6, un SVG con `.bpi`, un pie «cierre de V2»,
   `samples/pendientes/CatchSinTipo.bp` ya compila y debe volver a `samples/`.
3. **Eduardo no ha leído aún las notas de versión** (`docs/RELEASES.md`, sección v6.0): su introducción va tal
   cual como primer bloque; el resto lo escribió un editor desde el inventario. **Leerlas con él es lo primero.**
   Y la sección **18.4 de `gui.html`** («Dos formas de poner en marcha la interfaz: `run()` y `start()`») la pidió
   él con dos detalles concretos: `run()` no vuelve nunca en placa, y en las dos formas los handlers corren en el
   hilo del GUI — lo que cambia es el principal (parado en `join()` con `run()`, corriendo a la vez con `start()`,
   y entonces datos compartidos entre dos hilos).

⏭️ **Al retomar, en orden**: (1) leer el informe del verificador (o repetirlo) y arreglar lo que señale; (2) leer
`RELEASES.md` v6.0 con Eduardo; (3) cerrar `D1` en `FICHAS` con un commit que no diga WIP; (4) **`F1`**: el barrido
de `samples/` (`samples_sweep.py`), `doc_frags.py`, `compat.sh check`, una tanda de `tanda.py` por familia
(regrabar la Metro antes: sigue con el firmware del 9-sep), la dist con siete imágenes; (5) publicar v6.0 y el
push (unos 470 commits por delante de `origin`).

📌 **Estado del repo**: todo commiteado, **sin push**. Host en sabor `gui1-lvgl0`. Placas al cortar: DK2 (COM12),
Pico (COM22), Metro (COM4). miVM reconstruida con `#501`; frontend con el diagnóstico de `#498`; los cinco
firmwares compilan con `#499` (grabadas hoy C6, P4 y DK2 con el `INFO` de `screenW/H`, antes de `#499`).

---

## ⏭️ AL RETOMAR (12-sep) — `G2`, `C1` y `T1` cerrados el 11-sep; **lo primero: marcar el 🧊 CODE FREEZE V6** y empezar `D1`

**Decisión de Eduardo al cerrar el día (11-sep):** *«cuando terminemos `T1` congelamos código. Y por hoy
terminaremos, la documentación la empezamos mañana.»* `T1` quedó cerrada esa misma noche (runner arreglado y
tanda de confirmación hecha), así que **el primer commit del 12-sep es el freeze**: a partir de ahí sólo entra lo
que esté roto (el criterio de siempre: no «¿merece la pena?» sino «¿está roto?»). Luego `D1` (documentación) y
`F1` (pruebas finales).

**Lo que pasó después del traspaso anterior, en orden:**
1. **`C1` verificado en las cuatro pantallas** (C6 tras un «sin memoria» que era la DRAM de plataforma; P4 con
   `idf.py -p COM14 flash`, y girado 90°). La `Gui` rancia del pack del P4 sombreaba a `/lib` disfrazada de
   `/app` — `#493`, anotado; Eduardo borró los packs de la placa.
2. **`T1`**: `tools/tanda.py` (bucle por placa, Eduardo rota placas) probado primero contra `bpvm-sim`; la
   **prueba de fuego** con la lista corta en Pico, C6, P4 y DK2, todo verde (`compat/informes/tanda_prueba_fuego.md`).
   Para las gráficas el oráculo va **por placa** con `--screen=WxH`, y el `INFO` de los builds con LVGL publica
   `screenW/screenH` (regrabadas C6, P4 y DK2 con eso).
3. **La revisión adversaria del runner encontró 3 formas de decir verde por motivos equivocados** (artefacto
   rancio, dependencias comprobadas en `/lib` cuando el RUN resuelve `/app`, `EXITED` de otra sesión) y 5 menores;
   arregladas y verificadas con un proxy TCP que inyecta fallos del wire. La tanda de confirmación cazó la
   **Metro con firmware del 9-sep** (`exit=11` vs `1`) por conducta.
4. Fichas: `#495` abierta a V7 (el `BUSY` del wire durante un RUN es de la época de un hilo); `#444` respondida
   por `T1`; `#493` con el matiz del pack del P4.

⚠️ **Para el runner, lo que hay que saber**: el orden es por placa; el informe se añade por tanda; `--puertos auto`
descubre lo conectado (P4 y DK2 tardan > 3 s tras un reset: si sale «ninguna placa contesta», repetir); desde Git
Bash, `MSYS_NO_PATHCONV=1` si se pasan rutas `/app/...`; el host tiene que estar en el sabor `gui1-lvgl0`.

📌 **Estado del repo**: todo commiteado, **sin push**. Paridad 59 PASS. Grabadas hoy: C6, P4, DK2 (con `screenW/H`
en `INFO`) y la Pico (10-sep). **La Metro sigue con el firmware del 9-sep** (regrabar con el `.uf2` de
`pico/build/` + BOOTSEL por el wire; luego reinstalar `/lib`). Placas conectadas al cerrar: DK2 (COM12), Pico (COM22),
Metro (COM4).

⏭️ **Lo primero al retomar**: (1) el commit del **CODE FREEZE V6** (`FICHAS` + memoria, como en V4/V5); (2) `D1`
según su plan en `FICHAS`; (3) si sobra un hueco, regrabar la Metro y pasarle la tanda.

---

## ⏭️ AL RETOMAR (11-sep, noche) — **`C1` CONSTRUIDO** en el mismo día que `G2`; el siguiente es `T1`

**`C1` — la captura de pantalla — hecha de punta a punta y vista en las CUATRO pantallas** (`0791bb3e`, `d9c6e0db`): `Gui.shot(path)` en las dos VMs,
el fichero `.shot` (`docs/SHOT_FORMAT.md`), `tools/shot2png.py`, y **la pantalla de la Discovery vista en
el PC** (800×480, 6 956 B, `GET` en 643 ms). Detalle y lo que corrigió del diseño, en `FICHAS` (`#475`).
`#495` abierta a V7 por decisión de Eduardo (el `BUSY` del wire durante un RUN es de la época de un hilo).

**Cómo se hizo, y por qué importa para lo siguiente:** con `ultracode`. Un mapa previo de seis lectores
tumbó dos premisas del diseño (*el host no va en PARTIAL*, *el LZ4 pide 16 KB de pila*) antes de escribir
nada; dos satélites en paralelo (la herramienta Python y el lado Java) contra una spec escrita primero; y
una revisión adversaria (3 lentes, refutadores por pares) que confirmó dos hallazgos reales y descartó
siete que ya había arreglado en vuelo. **La spec primero** es lo que hizo que tres piezas escritas por tres
manos encajaran a la primera.

⚠️ **Lo que enseñó la placa**: (1) *bombear antes de capturar* — las transiciones del tema de LVGL
(~100 ms) hacen que una captura sin lazo salga con el checkbox vacío y `val=1` en el modelo; (2) **en la DK2
`/lib` se vacía en cada arranque** — lo subido a mano a `/lib` muere al primer reset, los módulos GUI van a
`/app`; (3) tres capturas del mismo programa con dos reflasheos en medio salieron **byte-idénticas**: el
oráculo placa↔placa existe.

📌 **Estado del repo**: todo commiteado, **sin push**. Paridad **59 PASS**. Discovery grabada con el
firmware final (17:30) y con `Gui/Json/Collections/Str` en `/app`. C6 y P4 **compilan, sin probar en placa**.
**C6 grabada y verificada** (2 862 B, 40×; costó un «sin memoria» primero: la DRAM de plataforma, no el tope — arreglado comprimiendo desde la banda sin copiar). **P4 grabada y verificada** (8 885 B, 138×; y girada 90°). Pico compilada. Metro sigue sin regrabar. El host queda en el sabor `gui1-lvgl0` (el del arnés); para ver la
GUI en el PC, `make GUI=1 LVGL=1` (y el arnés se negará hasta volver).

⏭️ **Lo primero al retomar**: **`T1` fase 1** (los
~50 samples puros con las placas conducidas), y en la fase 2 la prueba gráfica es *ejecutar, `toJson()`,
`shot`, `GET`, diff* — todas las piezas existen ya.

---

## ⏭️ AL RETOMAR (11-sep, tarde) — **`G2` CERRADO** en tres commits; el siguiente es `C1`, y antes se PAUSA

**`G2` — Revisión del modelo gráfico — hecho entero el 11-sep** (`50fcbc46` · `65a50f0e` · `65e9f558`),
y de paso una ficha que nadie esperaba, `#494`. Detalle de cada punto en `FICHAS` (sección `G2`).
Eduardo pidió *«el punto 4 y después una pausa»*: **esto es la pausa**.

**Lo que pasó, en orden:**

1. **El modelo BP de la GUI no era un modelo** (salió analizando `C1`): el árbol vivía sólo en C, los
   wrappers de los hijos se tiraban al cargar un `.win`, `destroy()` prometía una cascada que nadie
   hacía, y `OwnerList` **estaba hueca**. Eduardo lo convirtió en hito: *«si un objeto contiene a
   otro, debería poder verse»*. Ahora `Container` tiene `children: OwnerList`, `attach()`/`delete()`
   son el alta y la baja simétricas, la cascada es nuestra (hijos antes que padre, C antes que BP), y
   `Button` es un contenedor con un solo constructor y `text := ""` por defecto.
2. **`toJson()` recursivo en formato `.win`**, en BP y UNA vez (paridad gratis), con cinco
   intrínsecas de una línea para la geometría **autorada** (`align` por nombre + desplazamientos;
   lo auto se omite: el JSON hereda la paridad del `dumpTree`). **`GuiWinJson.bp`** al corpus:
   `main.win → J1 → J2`, `J1 == J2`, árbol idéntico; **igual en la Discovery** (LVGL, 800×480).
3. **La ida y vuelta destapó tres fallos** — para eso es la prueba:
   - el cargador **perdía la `y`** del `.win` (`align(…,0,0)` la pisaba: el `"y": 12` de `main.win`
     nunca había llegado al modelo);
   - una **regresión mía de G2-2**: `Keyboard.attach(ta)` resolvía al `Component.attach` nuevo y el
     teclado se quedaba sin textarea, en silencio → `setTextarea(ta)`;
   - 🐛🐛 **`#494`: el GC de la miVM descarrilaba desde el 15-jul** ante cualquier `""` o array
     vacío (12 B reservados, 8 contados). **El guardián lo gritaba por stderr y el arnés lo tiraba con
     `2>/dev/null`**: 57 PASS con el GC de la VM de referencia roto. Arreglado (el mismo clamp que la
     VM-C tenía desde F2) y el arnés ya vuelca esa línea.

⚠️ **LO QUE APRENDÍ HOY, en dos frases:** *(a)* una prueba de ida y vuelta vale más que las tres
piezas que une: encontró un fallo en el cargador, otro en mi propio commit anterior y otro de hace
dos meses en el GC; *(b)* **un guardián que grita a una tubería que nadie mira no es un guardián** —
el `2>/dev/null` del arnés era un `INCONSISTENTE` silenciado durante 57 casos verdes.

🔧 **Herramientas**: `compat.sh` acepta `// recurso: ruta` en la cabecera de un sample (copia el
fichero al `WORK`), y vuelca `HEAP INCONSISTENTE` del stderr como salida. `wire_serie.py`: desde Git
Bash hay que exportar `MSYS_NO_PATHCONV=1` o `/app/X.mod` se convierte en `C:/Program Files/Git/app/…`
(en la DK2 hay dos ficheros con esa ruta de sesiones anteriores).

📌 **Estado del repo**: todo commiteado, **sin push**. Paridad **58 PASS, 0 FAIL** (con el stderr
vigilado). **Discovery grabada** (13:16) y con `/lib` completo (`Gui/Json/Collections/Str` subidos hoy);
Pico compilada (13:21), **no grabada** (el cambio de C es sólo GUI); **la Metro sigue sin regrabar**
desde que cayó del USB (tiene el `Core` viejo: regrabar + reinstalar `/lib` cuando vuelva). Blobs
embebidos sin tocar (no llevan `Gui`).

⏭️ **Lo primero al retomar**: `C1` — la captura de píxeles (`#475`), porque el árbol ya está
(`toJson()` es la mitad «árbol» de `C1`). Antes de tocar nada, decidir lo pequeño que quedó abierto:
si el guardián del heap debe ser **fatal** (`#494`), si `#493` (la sombra de `/app`) entra en V6, y
la nota de `C1` en `FICHAS` sobre el tamaño del panel horneado en el JSON de `Window.load()`.

---

## ⏭️ AL RETOMAR (10-sep, madrugada) — **V6 se queda SIN fichas abiertas**: sólo los cuatro hitos

**`#473` y `#489` cerradas. No queda ni una ficha de V6 abierta** — lo siguiente son los hitos
`C1` → `T1` → `D1` → `F1`, y Eduardo pidió **analizarlos antes de empezarlos**.

**Lo que pasó, en orden:**

1. **`#489`** — la cola del GUI descartaba en silencio. No se agrandó: se le puso voz (el primero en
   el acto, el total al terminar, por `stderr` para no tocar la paridad). Con `make test-guidrop`, que
   **fuerza** el desbordamiento — 600 eventos en 512 huecos.
2. **`#473`** — se triaron los **91** hallazgos de la auditoría `A3`: 21 que rompen el modelo (2
   arreglados, 10 vivos, 9 ya cubiertos) y **70** de baja gravedad. De esos 70, **cuatro estaban rotos
   de verdad**; tres se arreglaron y el cuarto se midió y resultó no estarlo.
   - 🔑 **Dato de método**: de 12 hallazgos que la primera pasada dio por muertos, **8 resucitaron** al
     revisarlos. El patrón siempre igual: *se arregló la mitad que llega al usuario y se declaró
     muerto el hallazgo entero*.
3. **Los tres arreglos, todos medidos en placa**: el `-2` de línea estancada sube al contrato (un
   mensaje truncado se comía el siguiente); la línea del wire es **atómica en las cinco** (el cerrojo
   es del CABLE, no de la VM); y `Uart` tiene por fin **anillo de 512 B** — verificado en loopback en
   la Pico (GP0↔GP1) y en la Discovery (PC10↔PC11, en dos buses).
4. **Se abrieron dos fichas de V7**: `#490` (bajo consumo) y `#491` (el cursor / `File` como objeto),
   las dos con su estudio dentro.

⚠️ **TRES TROPIEZOS MÍOS, que son la parte útil del traspaso:**
- **Diagnostiqué contra el firmware VIEJO.** Compilé la Discovery tres veces y **no la grabé ni una**;
  leí un `available()=-1` como «el anillo no se activa» cuando era código de tres horas antes.
  **Compilar no es grabar, y el `serverBuild` del `HELLO` lo dice en un segundo.**
- **Publiqué una medida inválida y me salvó el control.** El primer `GET` cronometrado daba a los dos
  sistemas de ficheros por igual de lentos; el caso *bueno* salía peor que el sospechoso, que es la
  señal. El fallo era de mi herramienta (troceaba por líneas y se comía el bulk crudo).
- **Y estuve a punto de tomar la AUSENCIA de un aviso como prueba de que algo funcionaba.** No lo es:
  ese aviso sólo salta al fallar, así que falta igual si la función no llegó a ejecutarse.

🔧 **Dos herramientas nuevas en `samples/`, las dos nacidas de equivocarme**: `GpioPuente.bp`
(comprueba que el cable está ANTES de culpar al código) y `ScanCn1.bp` (lo **busca** en los 16 pines
del CN1 — contestó en 200 ms cuando yo lo buscaba en los pines de al lado). Y `tools/wire_serie.py`
tiene ahora verbo `get` con cronómetro.

📌 **Estado del repo**: todo commiteado, **sin push**. Imágenes al día y **grabadas** en Pico
(23:14) y Discovery (23:15); S3, C3, C6 y P4 compilados y sin grabar. Paridad **54 PASS, 0 FAIL**.

⏭️ **Lo primero al retomar**: `C1`. Eduardo dejó ampliado su diseño la madrugada del 11 —**serializar
la ventana a JSON, con el serializador en `Component` y recursivo por los hijos**— y el análisis está
en su ficha (`#475`). Lo que hay que censar primero: **qué propiedades expone hoy `Component`**, porque
de eso depende que el serializador se pueda escribir en BP (una vez) en vez de como builtin (dos veces).


## ⏭️ AL RETOMAR (9-sep, noche) — **de 19 cosas a 2**, y el corpus de 48 a 54

Segunda mitad del día, entera cerrando pendientes. **Nueve fichas cerradas, cuatro fuera de V6 con
su decisión escrita.** Y el hilo que recorre casi todas: *lo que no está en el corpus, se pudre*.

### Lo que se cerró

| | |
|---|---|
| `#486` | los dos dispatchers del GUI, resueltos **una vez por RUN**: 241 ms → **0-1 ms** |
| `#480` | entera: el ADC del ESP32 que fallaba con 0, + el watchdog (mañana) |
| `#485` | el contrato de prioridad de `io`, de **vigilado a construido** |
| `#484` | no era «falta `listDir`»: eran **seis** builtins sólo en miVM, **dos rotas en vivo** |
| `#481` | el código de salida y el informe de una excepción no atrapada |
| `#482` | la vista doble de los packs: **no la necesita nadie** — medido en placa |
| `#471` | tres roturas de paridad en `Machine`, y un `set()` que mentía |
| `#379` | → **`F1`**: lo único que le queda es una prueba de placa |
| `#487`, `#488`, `A4`, `#470`(números), `#480.2` | **fuera de V6**, con el diseño escrito |

### 🔑 Las correcciones de Eduardo, que son lo que hay que retener

1. **«El S3 es Xtensa y el C3 es RISC-V, son casos diferentes.»** Yo había clasificado cuatro micros
   **leyendo un macro que el programa ni consulta** (`SOC_MMU_DI_VADDR_SHARED`); la decisión se toma
   en *runtime*. Medido en las dos placas: **INST y DATA son la misma dirección** en C3 y S3. El
   mecanismo de vista doble **no tiene ni un usuario**. Y de paso corrige el 8-sep: aquel commit
   llevaba dos arreglos y **sólo uno trabajaba** — lo que dejaba al S3 sin packs era que el mapeo
   vivía en el directorio del P4.
   📐 Su explicación **predice** mejor que la medida: vistas separadas es un rasgo **Harvard**, y ARM
   y RISC-V son Von Neumann.
2. **«Hay que distinguir de un exit distinto de 0, de una excepción no atrapada: 2 casos.»** Eran dos
   roturas: miVM salía con **0** para un programa muerto, y la VM-C devolvía **el ordinal del enum**.
3. **«Cuidado con la frecuencia, no es una constante.»** Destapó que `set(200)` devolvía `true` en
   las dos VMs y `cpuFreqHz()` seguía diciendo lo de antes.
4. **«El watchdog, completo o no se implementa.»** El IWDG del U5 no se para por software → el STM32
   **se queda sin watchdog** y los tres verbos lanzan.
5. **«Con probarlo una vez en una placa es suficiente.»** El 91,8 % es común; compilar las cinco
   familias prueba lo mismo cinco veces.
6. **«Un `exit(11)` no me dice nada.»** → el `help error` del IDE.

### 🔴 CINCO roturas del invariante, todas vivas, todas por lo mismo

Ninguna se veía **porque no había sample suyo en el corpus**:

| | qué |
|---|---|
| los stubs del PC | **13 textos** distintos en seis fachadas (`(sim …)` vs `(stub …)`) |
| `src/uart.c:42` | un **`putchar`** en medio de `bpvm_out` → el payload salía **desordenado** |
| el watchdog | `(host, no-op)` vs `(stub, no-op)` |
| `IO.pathAbsolute` / `IO.prompt` | funcionaban en miVM y la VM-C lanzaba «builtin no soportado» |
| `Machine` | `uniqueId`, `tempC` y `cpuFreqHz` |

**Corpus 48 → 54**: `WdtCatch`, `StubParidad`, `IoPrompt`, `IoPathAbs`, `ThrowSinAtrapar`,
`MachineHost`.

### 🔧 Herramienta nueva: `bpgenvm-c/tools/wire_serie.py`

Cliente del wire **por serie que NO es el IDE**. Es lo que faltaba desde agosto para el paso 1 de
`#379` (*¿está colgado el device o el IDE?*). Con él: **16 ciclos `run→stop→INFO` en C3 y S3, 0
fallos**. Y sirve para cualquier placa: `info`, `log`, `list`, `ciclo`.

```
python tools/wire_serie.py COM3 ciclo /app/Bench.mod 8
```
⚠️ Desde Git Bash hay que poner `MSYS_NO_PATHCONV=1`, o convierte `/app/...` en ruta de Windows y la
placa contesta `NOT_FOUND` — me costó media hora.

### ⚠️ Dos trampas y un aviso

- **El `io-smoke` tenía un caso PERMANENTEMENTE ROJO**, y era **su guarda**: buscaba `"gui lista"`
  para detectar que el sim no trae LVGL, y el sim headless también la imprime. Un caso siempre rojo
  enseña a ignorar el arnés.
- **Mi propio script tiraba la respuesta que no encajaba** y decía «sin respuesta» mientras la placa
  contestaba `NOT_FOUND`. El mismo vicio que llevaba el día arreglando en el producto.
- ⚠️ **El arnés NO mira el código de salida**, sólo `stdout`. Los `exit 1` de `#481` están medidos a
  mano.

### ⏭️ Al volver: quedan DOS

- **`#462`** — el **suelo de ~48 ms** de latencia upcall→handler, independiente del quantum. Es la
  única con incógnita de verdad: el sospechoso (`LV_DEF_REFR_PERIOD = 33 ms`) explica buena parte,
  no los 48. Necesita placa con pantalla.
- **`#473`** — los **78 hallazgos** de la auditoría de capas, sin verificar. No necesita placa ni
  decisión: es leer y comprobar. Es la más grande. *(Van ya tres confirmados sin buscarlos: la
  fachada del FS que no enlaza con el backend de host, el `gpioCount` que no significa «cuántos
  GPIO», y el `io` del P4.)*

⚠️ **Placas**: la **C3 se reflasheó** hoy (llevaba firmware anterior al 8-sep y ahora monta packs);
Pico 2, S3 y Metro al día. El P4, el C6 y los STM32 **no**. Y el `bpgenvm-c` del host está construido
**sin LVGL** (`make sim LVGL=1` para el caso del GUI del `io-smoke`).
## ⏭️ AL RETOMAR (9-sep, tarde) — **de 19 cosas a 11**, y la lista dejó de mentir

Sesión de cerrar pendientes, muy conducida por Eduardo: **seis correcciones suyas**, y cinco
hicieron el trabajo más pequeño o lo pusieron en su sitio.

### Lo que se cerró

| | |
|---|---|
| `#456` | el `path` truncado en silencio — **y no se eligió número: se quitó la copia** |
| `#483` | la Metro que no ejecutaba nada — **por NO REPRODUCIBLE**, con la receta guardada |
| `#480.1` | el **watchdog** del STM32: completo o no está |
| `#470` | los nombres, **verificados en placa**: Pico 2 `rp2350a`, Metro `rp2350b` |
| `A4`, `#488` | **fuera del plan de versiones** |
| `#470` (números), `#480.2` | **a V7**, con el diseño escrito |

### 🔑 Las correcciones de Eduardo, que es lo aprovechable

1. **«¿Por qué el path tiene un tope?»** — cuatro palabras que cambiaron la ficha entera. No lo
   imponía nada: ni littlefs ni FatFs (255 **por nombre**), ni el protocolo (línea de 2048), ni el
   parser (in-place). Lo imponía **la copia**. Y no era un tope: eran **seis** números para la misma
   cosa (39, 63, 95, 191, 256, 511). Ahora el path del wire **no se copia** (`json_str_inplace`) y
   queda **un** número, para lo que hay que guardar o construir.
2. **«Quita topes. Si tiene que haber un tope lo hablamos y vemos cuál es la razón.»** Y al mirar
   salieron dos cosas que la ficha no tenía: **el lado BP tenía el mismo bug** (13 sitios que
   ignoraban lo que devuelve `read_bp_string`) y **el resolvedor iba por detrás** — el wire aceptaba
   205 caracteres y el `RUN` los recortaba a **39**.
3. **«El bucle de LVGL, si no hay nada pendiente, hace una pausa y el sistema de Threads pasa el
   testigo al siguiente.»** Yo iba a por «quantum por tiempo». Él tenía razón y era más simple: el
   bombeo **sí** pausaba, pero **a nivel de SO** (`SDL_Delay`, `vTaskDelay`, `__WFI`,
   `Thread.sleep`), y sobre esa única tarea corren **todos** los hilos BP. No dormía el hilo del
   GUI: congelaba la VM. Ahora devuelve el ocio y duerme el lazo BP.
4. **«El watchdog, completo o no se implementa.»** El IWDG del U5 no se para por software —silicio—,
   así que el STM32 **se queda sin watchdog** y los tres verbos lanzan. Y **el código se borra, no
   se comenta**: código muerto son avisos para siempre.
5. **«El S3 es Xtensa y el C3 es RISC-V, son casos diferentes.»** Yo había propuesto probar los
   packs del S3 en el C3 como si fuera equivalente. Lo es **para el MMU** y no **para la ISA**. La
   ficha se partió en dos.
6. **«Muchas de estas entradas son múltiples cosas; las que no tienen número, asígnaselo.»** De ahí
   `#482`–`#488`.

### 🔴 Tres roturas del INVARIANTE SAGRADO, vivas y sin que nadie las viera

Y las tres por el mismo motivo: **lo que no está en el corpus, se pudre**.

- **Los stubs del PC.** Seis fachadas (`gpio`, `pwm`, `pulse`, `uart`, `spi`, `pico`) escriben su
  línea de simulación **dos veces**, y no decían lo mismo: `(sim …)` contra `(stub …)`. **13
  textos.** Mismo `.mod`, `stdout` distinto.
- **Y uno no era cosmético**: `src/uart.c:42` usaba **`putchar`** en medio de una secuencia de
  `bpvm_out`. `putchar` escribe al `FILE` de C y **se sale del sumidero de la VM**: el payload salía
  **desordenado** (al principio de stdout) y el preview quedaba vacío. Dos líneas más abajo hay un
  comentario que dice *«TEXTO = CONTRATO DE PARIDAD: idéntico al de miVM»* — se hizo para `read` y
  se saltó `write`.
- **El watchdog**: `(host, no-op)` contra `(stub, no-op)`.

**Corpus 48 → 50**: entran `WdtCatch.bp` (los tres verbos del camino de error) y `StubParidad.bp`
(cada verbo con stub de las seis fachadas).

### 📐 La cuenta, que es lo que descolocó a Eduardo al final

*«Empezamos con 9 pendientes y terminamos con 11, parece que vayamos para atrás.»* Contando **como
contamos ahora** —una entrada, una cosa— esta mañana había **19**, no 12: `#462` escondía cuatro,
`#480` otras cuatro, `packs del S3` dos. **De 19 a 11.** Lo que subió no fue el trabajo: fue la
resolución del microscopio.

### ⚠️ Y una trampa mía, la misma de siempre

Eduardo no pudo compilar `MachineId.bp` desde el IDE: *«intrínseco no implementado en el emisor
mivm: Machine.getMicro»*. **El IDE empaqueta su propia copia del compilador** y la suya era del
8-sep 18:19; las intrínsecas se registraron el 9-sep 06:11. El síntoma es de los malos: el
**semántico las acepta** (las lee de `Machine.mod`) y sólo revienta el emisor, así que parece un bug
del compilador y es un **artefacto rancio**. Reconstruido y verificado compilando **con el jar del
IDE**, no mirando el log de Maven.

📌 **Regla que hay que aplicar sola: tocar `Intrinsics.java` obliga a reconstruir el fat-jar del
IDE** (con el IDE cerrado).

### ⏭️ Al volver

**11 pendientes.** Los dos que no necesitan nada de Eduardo: **`#486`** (cachear las dos direcciones
del dispatch del GUI, como ya hace miVM) y **`#480`** (el ADC del ESP32 que falla con 0 en vez de
−1 — es el mismo arreglo aplicado hoy dos veces). **`#481`** sigue esperando su criterio: ¿el
informe del error no atrapado es **salida** o **diagnóstico**?

⚠️ **Placas**: Pico 2, S3 y **Metro** llevan firmware al día; el resto no. Y el `bpgenvm-c` del host
está construido **con `LVGL=1`** (hizo falta para medir el GUI) — si algo se compara con builds
anteriores, tenerlo en cuenta.
## ⏭️ AL RETOMAR (9-sep, noche) — **tres carreras muertas** y la identidad del micro sale por la HAL BP

Sesión corta en fichas (una) y larga en lo que hay debajo: **el SMP de la VM-C pasa de inservible
a limpio**, y `getMicro()`/`getBoard()` quedan por el camino correcto, no por el rápido.

### Lo que se cerró

| | |
|---|---|
| `#474` | `B1` **arreglado en miVM** — y de propina **dos carreras más en la VM-C** (mutex y GC) |
| `#470` | su mitad buena: **`getMicro()` y `getBoard()`**, por la HAL BP, verificados en dos placas |

Los cuatro números que quedan de `#470` (MHz, GPIO, ADC, PWM) siguen esperando tu criterio, así que
la ficha sigue abierta. El arnés pasa de **45 a 48 casos** (`NeoCatch`, `MachineAlias`, `MachineId`).

### 🔑 Las tres carreras, y por qué salieron juntas

La ficha `#474` pedía revivir una reproducción que no compilaba (le faltaba un `import Core`). Al
revivirla, `B1` seguía vivo con su firma exacta. **Pero el arreglo no lo encontré leyendo: lo
encontró un criterio tuyo.**

> *«Aunque esté escrito en C el modelo bueno es el de OOP. Si tratas al Thread como una clase, debe
> haber un sólo método para cambiar el estado, el setter correspondiente. Y es ahí donde tienes que
> proteger el acceso y el cambio de estado.»*

En miVM `ThreadContext.status` era un campo público que se escribía desde **21 sitios**. Encapsularlo
(`setStatus(nuevo, quien, motivo)`, campo `private`, más un `ownerThread` volátil) no fue cosmética:
**el bug apareció solo** en cuanto hubo un sitio donde mirar. `YIELD` devolvía a la cola un hilo que
ya no era suyo. El arreglo son tres líneas, y todas dentro del setter.

⚠️ **Detalle de Java que me costó un intento**: `private` **no** obliga al setter si la clase es
anidada — la envolvente ve sus privados igual. Hubo que **renombrar el campo** (`status` → `estado`)
para que el compilador señalara los 21 sitios.

Con eso verde en miVM preguntaste lo que había que preguntar: *«¿y en VM-C qué ocurre?»*. En VM-C
`--smp=2` **se colgaba 7 de cada 8 veces**. Dos causas independientes, las dos de manual:

| | `--smp=1` | `--smp=2` | `--smp=4` |
|---|---|---|---|
| al empezar | 0/12 | **7 de 8 cuelgues** | 7 de 8 |
| tras el mutex | 0/12 | 1/30 | 5/12 |
| tras el GC | **0/20** | **0/20 y 0/40** | **0/20** |

- **El mutex no era atómico**: apuntarse como esperando y marcarse `BLOCKED_MUTEX` ocurrían en
  secciones críticas distintas → **despertar perdido** clásico. Ahora las dos cosas pasan dentro de
  la misma, en `bpvm_mutex_try_acquire` / `bpvm_mutex_release`, que son los **dos únicos** sitios que
  tocan la propiedad del mutex (mismo criterio de arriba, aplicado a C).
- **El GC podía tener DOS colectores**: si un worker entraba en `gc_stw()` con un
  stop-the-world ya en marcha, se esperaban mutuamente. Ahora el segundo se apunta como parado,
  espera a que acabe el primero y vuelve.

📌 Y el reparto de SMP, que era lo que estaba tapado: **×1,69 con 2 workers y ×1,83 con 4** en miVM,
**×1,90** en la VM-C. Tú ya habías dicho lo importante: *«`smp=4` no sé si tiene interés práctico, el
que realmente interesa es `smp=2`»* — y es justo el que estaba roto.

### 🧭 La corrección de rumbo: la identidad sale de la HAL BP

Iba a hacer que la fachada BP leyera el nombre **del repl** —o sea, invertir las capas— y lo cortaste:

> *«Hay que hacerlo por el camino correcto. Todos han de llamar a una misma función en Hal BP, una
> para el micro y otra para la placa. Lo que es diferente de cada imagen es la implementación.»*

Hecho así: `bpvm_pico_micro_name()` en la fachada, una implementación por familia, y **el wire y el
programa BP leen los dos de la misma función**. Verificado en placa: Pico 2 → `rp2350a` / `generic`;
S3 → `esp32s3` / `generic`.

🔑 Y ahí salió lo que yo había tirado a la basura: yo cableé `"rp2350"` a secas. Tú: *«en la Pico el
micro es RP2350A (61 pins), en la Metro es el RP2350B (81 pins)»*. **El silicio ya lo dice**:
`SYSINFO.PACKAGE_SEL` (1 = QFN-60, 0 = QFN-80), de sólo lectura e independiente del FS. La función
«inteligente» que pedías son dos líneas.

✅ **Verificado también el `rp2350b` de la Metro** el mismo día, cuando la Metro volvió a ejecutar: el wire dice `rp2350b (RP2350B)` y `Machine.getMicro()` dice `rp2350b` — los dos leyendo la misma función de la HAL BP.

### 📐 El modelo de los micros, escrito donde no se pierde

Lo que dictaste sobre el destino —1 núcleo → `IO`+`VM1` turnándose pero con **cola de hilos de SO**
para el futuro; 2 núcleos → `IO`+`VM1`+`VM2` a piñón, cada VM consultando la cola de hilos BP, *«en
principio no interactúan entre ellas, los intercambios se hacen a través de la memoria
compartida»*— está en **`docs/V6_IDEAS.md`**. En términos de código: **VM = worker**.

### 🧮 Y la cuenta que te hizo gracia

Empezamos el día en «12 pendientes» y lo terminamos en «12». **No era una broma del día: era un
error mío de ayer.** El rótulo del censo decía *8 fichas* mientras la lista enumeraba **nueve**, o
sea que ayer eran **13**. Cerrar `#474` corrigió la aritmética además de la lista. Ahora el rótulo y
la lista dicen lo mismo, que era medio problema.

### ⏭️ Mañana

**12 pendientes, 4 hitos.** Sin bloqueo: **`#473`** (verificar los 78 hallazgos de la auditoría —
sólo leer) y **`#481`**, que es el que toca el invariante sagrado.

**Decisiones tuyas que siguen abiertas** (las mismas de ayer, ninguna se ha resuelto): `#481` (¿el
informe del error no atrapado es salida o diagnóstico?), `#480` (qué ve el programador cuando el chip
no puede apagar el perro), `listDir` (¿entra en V6?), `#456` (qué devuelve un path truncado y cuánto
tiene que caber), los cuatro campos de `#470`, y si el SMP se activa por defecto.

⚠️ **Placas**: **Pico 2 y S3 llevan el firmware de hoy**; las otras siete, no. Y siguen en ellas los
`.mod` de prueba que ya te listé ayer — borrarlos sigue necesitando tu visto bueno, placa a placa.

## ⏭️ AL RETOMAR (8-sep, noche) — **de 15 pendientes a 13**, y el S3 estrena 8 MB de PSRAM

Jornada larga y muy conducida por Eduardo: **tres correcciones suyas** cambiaron el trabajo, y en
los tres casos lo hicieron MÁS PEQUEÑO. Al parar: *«en 2 o 3 días los tenemos todos hechos»*.

### Lo que se cerró

| | |
|---|---|
| `#472` | la **TLS emulada** del STM32 — `__emutls_v` con 2 `malloc` y un `abort()` en un micro |
| `#469` | fachadas sin backend — **cerrada por Eduardo**: *«para V6 está completo»* |
| `#468` | la stdlib en flash → **V7** |
| `S3` | **la PSRAM, activada**: heap de la VM de 160 KB a **7 MB**, probado en placa |
| `S3`/`C6` | **zona de packs**: el mapeo sale del directorio del P4 y pasa al común |
| `#471` | `Pico` → **`Machine`**, con alias. Faltan `getMicro()`/`getBoard()` |
| — | el **neopixel** lanza excepción BP en las dos VMs |
| `#481` | **abierta**: las dos VMs discrepan en `stdout` al morir lanzando |

El arnés pasa de **38 a 47 casos**.

### 🔑 Las tres correcciones de Eduardo, que son lo aprovechable

1. **El eje era la FAMILIA, no la fachada.** Yo pregunté «¿extendemos el patrón de `#469` a 3 de 13
   fachadas o a las 13?» y él: *«eso pertenece a la capa HAL BP… no se trata de hacerlo 8 veces,
   sino 3 y una ya está hecha»*. Medido con ese eje: los huecos eran **cinco**, no ocho ni trece, y
   la RP2350 ya estaba completa. La ficha se cerró el mismo día.
2. **«El sistema debería ser idéntico, igual que el environment.»** Yo iba a escribir `pack_s3.c`
   porque el S3 y el C3 no comparten dirección virtual para datos e instrucciones. Al **contar los
   consumidores**: 12 de `packs_base` son lectura, sólo **uno** ejecuta, y nace en **una línea**.
   No hacía falta código de familia — hacía falta dejar de suponer que un puntero sirve para las
   dos cosas. Y de paso apareció que **el C6 no tenía packs por vivir el código en el directorio
   del vecino**, no por silicio.
3. **«No tiene sentido posponerlo todo a V7.»** El renombrado a `Machine` entra en V6.

### ⚠️ Dos trampas nuevas, las dos me costaron una hora

- **Las intrínsecas se enganchan por `"Módulo.función"`.** Busqué `"Pico"` en el compilador, no
  salió nada, y di por hecho que el nombre no estaba cableado. Estaba, como `"Pico.boardName"`.
  `Machine.boardName()` compilaba con **cero errores** y devolvía basura (miVM `OutOfMemoryError`,
  VM-C **segfault**). 📌 Y detrás hay un hueco del compilador: declarar `intrinsic` algo **sin
  registro** no da error — el **duplicado** sí lo detecta. Sin ficha aún.
- **El build de la stdlib cachea por FUENTE, no por versión del compilador.** Tras arreglar el
  registro el alias seguía roto y culpé al compilador; era un artefacto rancio. `rm -rf
  bpstdlib/out` y correcto. **Al tocar el frontend hay que forzar la reconstrucción.**

⚠️ **Y dos cifras que di mal y quedan corregidas**: los «+877 B por módulo» y el «+18,9 KB de
imagen» eran de un build incremental con blobs a medias. Con build limpio, los `.mod` no tocados
salen **byte-idénticos** a los de git y el coste real es **+2,5 KB en ESP, +5,1 KB en la Pico**.

### ⏭️ Mañana

**13 pendientes.** Sin bloqueo y listos: **`#473`** (verificar los 78 hallazgos de la auditoría —
sólo leer) y **`#474`** (revivir la reproducción de `B1`: le falta un `import Core`). Y la
continuación natural de hoy: **`getMicro()`/`getBoard()`**, que son builtins nuevos —tocan las dos
VMs y `make check-builtins`— y son los que quitan el `esp32s3-devkitc` y cierran `#470`.

**Decisiones de Eduardo que siguen abiertas**: `#481` (¿el informe del error no atrapado es salida
o diagnóstico?), `#480` (qué ve el programador cuando el chip no puede apagar el perro), `listDir`
(¿entra en V6?), `#456` (qué devuelve un path truncado y cuánto tiene que caber).

⚠️ **Placas**: el **S3 está al día** (PSRAM + packs + `Machine`). La Discovery, el C6 y las demás
siguen con el firmware de ayer. Y en las placas quedan `/app/P.mod`, `/app/adc.mod`,
`/app/PsramS3.mod` y `/app/MachineAlias.mod` de las pruebas — borrarlos necesita tu visto bueno.

## ⏭️ AL RETOMAR (7-sep, noche) — **`G1` cerrado, la GUI ENTRA en la red, y el registro ya no se contradice**

Sesión de dos mitades: por la mañana trabajo de código, por la tarde poner el registro en orden.
Eduardo al parar: *«Empezaremos mañana, por hoy es suficiente.»*

**Lo primero, y es una corrección mía que conviene leer antes que nada.** Yo venía diciendo —en el
traspaso de ayer, en dos commits y en la ficha `#477`— que *«el `check` de `compat.sh` lleva
desactivado desde V4»*. **Es falso.** Lo que Eduardo desactivó el 17-jul fue la compatibilidad
**BINARIA** con V2/V3; el `check` se ejecuta, y lo que ejecuta es la paridad dual-VM. Lo comprobé
en un comando: **38 PASS, 0 FAIL, 0 SKIP**. El arnés estaba **verde con el corpus corto**, que es
justo lo que la ficha `#440` ya decía bien. Y lo peor: **la prueba estaba en este mismo fichero**,
escrita por mí 900 líneas más abajo — *«`#459` volcaba el heap y `compat.sh` daba 38 PASS»*. Si
daba 38 PASS, se estaba ejecutando. Dos veces di por muerta una herramienta viva sin abrirla.

**Lo que se hizo con eso.** El corpus pasa de **38 a 45 casos** y por primera vez lleva GUI:
entran `BusBug` (el reactivo de `#440`, que llevaba meses delante del arnés), `AdcDemo`, `ArgDemo`,
`MathRango`, `mathtest` y dos samples nuevos de GUI.

🔑 **Y el bloqueo que impedía meter GUI no era del lenguaje: era una ventana sin cerrar.** La ficha
decía que *«miVM NO TERMINA los samples de GUI»* como si fuera un límite de la VM de referencia.
El `JFrame` de `GuiBackend` **no se destruye nunca** y el EDT de AWT no es demonio, así que con una
ventana realizada la JVM no sale aunque no quede un hilo BP vivo. Se vio de la forma limpia:
**misma salida byte a byte en las dos VMs, y la VM-C salía con 0 y miVM con 124**. Un
`gui.shutdown()` al terminar y las dos salen con 0.

📌 **Y una corrección de rumbo de Eduardo a mitad de faena, que mejoró el resultado**: yo estaba
metiendo los seis samples de GUI que ya existen **cortándolos por timeout**. Funcionaba (49 PASS)
pero tardaba 3m37s y era *flaky por diseño*. Él: *«lo que hay que hacer son nuevos test y utilizar
`Gui.Start(); pause(5000); Gui.Stop()`»*. Con esa forma: **45 PASS en 1m00s y determinista**.
Los samples nuevos son `GuiParidad.bp` y `GuiParidad2.bp`.

⚠️ **Tres endurecimientos del arnés, cada uno con su control** (porque un verde sin control no
vale): el arnés **sabe ponerse en rojo** (doble de la VM-C que ensucia una línea → FAIL por sample
y código 1); **un caso mudo en las dos VMs ya no es PASS**; y **`filt()` ya no se traga las líneas
en blanco de en medio** — lo destapó el dropdown del sample nuevo, que vuelca su texto con saltos
crudos, y antes una VM que emitiera un blanco de más donde la otra no emite nada salía **verde**.

---

### La segunda mitad: el registro contradiciéndose

Eduardo pidió la lista de pendientes *«una entrada por línea para que sea más sencillo contar»*, y
al censarla —las 69 fichas leídas **enteras**, no por la marca— salieron **seis contradicciones**,
todas con la misma forma: **el titular decía una cosa y el cuerpo otra**. `E1` y `G1` cerrados con
la tabla diciendo «abierto»; `A1` cerrada con la cabecera diciendo «abierta»; `A3` y `A4` sin fila
en la tabla; `#408` con el enunciado pendiente y la respuesta dos líneas más abajo; el bug del LSP
cerrado por retirada sin decirlo; y las bases de datos en 🔴 mientras `E1` las daba por hechas.
Eduardo: *«Sí, soluciónalo. No debemos tener información contradictoria.»* Corregidas (`49364cb0`).

📌 **Lo aprovechable del método**: el aviso que había en la cabecera de `ABIERTAS` decía que
separar lo cerrado era trabajo aparte porque *«un clasificador automático ya falló en dos»*.
Fallaba porque clasificaba **por la marca**. Leyéndolas enteras salió a la primera — y de paso, la
«cola heredada» que ese aviso cifraba en **59 entradas** son **12**. Un aviso sobre basura sin
limpiar también envejece.

---

### 🎯 Y el plan de cierre de V6, decidido por Eduardo (7-sep)

*«`C1` y `T1` se quedan en V6. La razón es que lo utilizaremos para las pruebas finales. La
documentación y las pruebas finales, un hito cada uno. `C1` y `T1` justo antes de documentar y
pruebas finales.»*

Con eso V6 queda en **15 pendientes y 4 hitos**, y el orden es:

> **los 15 pendientes → `C1` (captura) → `T1` (pruebas) → `D1` (documentación) → `F1` (pruebas finales)**

Lo que hace fuerte esta decisión es la razón: `C1` y `T1` no son un añadido que se cuela al cerrar,
son **la herramienta con la que se cierra**. (`D1` y `F1` son códigos que puse yo siguiendo la
nomenclatura de la tabla; el reparto y el orden son suyos.)

📌 **Y el parque quedó contado, que es la entrada de `F1` y de `T1`: 7 imágenes y 9 placas.**
Eduardo contó 8 (*«Pico y Metro; S3, C3, C6 y P4; Nucleo y Discovery»*); la novena salió de que
**la imagen del P4 sirve a DOS placas** (kit y Waveshare, con el panel elegido por el ENV) —
confirmado por él. Contar placas por imagen se deja una fuera. Y la Waveshare no es redundante:
es la única que prueba el camino `display=st7701`, o sea que la imagen única lo sea de verdad.
Tiene deuda: **le falta el reflasheo**.

⏭️ **Mañana**: *«lo que toca durante unos días es ir resolviendo pendientes»*. La lista, en
`FICHAS.md`, en el censo de la cabecera de `ABIERTAS`.

⚠️ **Y lo que queda a medias, para que no sorprenda**: las placas siguen con el firmware de ayer
(sin `bpvm_out`, sin `#412`, sin el gate del `.mdn`), y en ellas quedan `/app/P.mod` y
`/app/adc.mod` de las pruebas — borrarlos necesita tu visto bueno, placa por placa.

## ⏭️ AL RETOMAR (6-sep, noche) — **`E1` CERRADO, 7 de 7**. Queda `G1` y los pendientes

Jornada larga y con mucho cierre. Eduardo al parar: *«ha sido una buena jornada»*.

### Lo que se cerró hoy

| | |
|---|---|
| `#452` | el `RESET` con un RUN vivo — **verificado en las tres arquitecturas** |
| `#469` | la fachada de ADC ya no inventa lecturas — **la MITAD de ADC; la ficha sigue abierta** por las otras fachadas |
| `#476` | `make check-builtins`, con su **control negativo** |
| `#478` | **miVM no podía tocar un bus** — 8 sitios del 4→8B de V4 |
| `#412` | el **argumento de ejecución**, de punta a punta y en placa |
| `E1` | los **siete** puntos: el tiempo del RUN, el CRC de origen, las BD sin placa |

Y dos piezas de fondo que salieron por el camino: **`bpvm_out`** (la traza de las
fachadas por el camino del `print`, y en placa **ya viaja por el wire**) y el **gate de
arquitectura del host** (`MDN_ARCH_X64`), que hasta hoy aceptaba en silencio un blob de ARM.

### 📌 Lo que hay que retener de la jornada

**Tres de los bugs de verdad los encontró el trabajo, no la red de seguridad**, y siempre por lo
mismo: **el corpus** del `check` de `compat.sh` no toca ni buses, ni GPIO, ni ADC, ni GUI (`#477`).
⛔ **Corrección del 7-sep: donde esto decía «el `check` lleva desactivado desde V4», es FALSO.** Lo
desactivado fue la compatibilidad **binaria** con V2/V3; el `check` se ejecuta y da paridad dual-VM.
Lo corto era el corpus — que es justo lo que decía la otra mitad de la frase. ✅ Y hoy quedan **cinco samples byte-idénticos** esperando a
entrar: `BusBug`, `AdcDemo`, `ArgDemo`, `MathRango`, `MathTest`.

⚠️ **Y una advertencia sobre mí mismo, que se repitió cuatro veces**: mi propio instrumento me dio
el hallazgo equivocado —la latencia del arnés como si fuera sesgo del `elapsedMs` (30× de más), un
`cut -c1-100` que «partía» una línea de C sana, un `2>&1` que mezclaba stderr con la paridad, y un
`make sim` sin `LVGL=1` que acusó a `#462`—. Las cuatro las cazó un **control**. Sin control, cada
una habría sido una ficha falsa.

### Decisiones de Eduardo, con su criterio

- **«¿Es útil para el programador? Si la respuesta es sí, se hace.»** Con eso entró la BD en el
  simulador.
- **«El IDE no tiene que determinar el orden de búsqueda, le pregunta al micro.»** Enderezó el CRC de
  origen: yo iba a duplicar el orden en el IDE, que es la enfermedad de `#470` y lo que costó `#463`.
- **«x86 es en realidad x64, los binarios son diferentes.»** De ahí salieron dos valores de
  arquitectura y no uno.
- **El autorun no pasa argumento**: el parámetro es para PROBAR; una vez probado, **el valor por
  defecto ES la configuración de despliegue**.
- **`#479` (mapa de pines y `AnPin`) a V7**, con el modelo cerrado. Y **`#475`/`#444` pasan a ser
  hitos propios `C1` y `T1`** — son implementación nueva, no pendientes.

### ⏭️ Para la próxima

**`G1`**, y con el **diseño ya cerrado** al final del día (ver su ficha): `Gui.run()` sigue
**síncrono** y se añade `Gui.start()` asíncrono más `Gui.stop()` — decisión de Eduardo, y no se
puede hacer de otra forma porque varios samples imprimen DESPUÉS de `run()`. Dentro caen dos
arreglos vistos al mirarlo: el lazo **no duerme** (gira quemando quanta con la ventana abierta) y
`__guiRunOnce` **rescanea la tabla de símbolos entera en cada pasada**. Es lo único que queda de
los hitos de V6 además de `C1`/`T1`,
cuya versión **sigue sin decidir**. De los pendientes, mi orden por daño: **`#477`** (enchufar el
arnés: es lo que impide que lo de hoy vuelva, y hay cinco samples listos), luego **`#480`** —con el
`Wdt.disable()` del STM32 que **no desactiva** a la cabeza— y **`#470`**.

⚠️ **Placas**: la Pico 2 quedó con todo lo de hoy. **La Discovery y el C6 se desconectaron a mitad**
(`No ST-Link detected`), así que llevan firmware de esta mañana: **sin `bpvm_out`, sin `#412` y sin
el gate**. Y siguen teniendo ficheros de prueba (`/app/P.mod`, `/app/adc.mod`).
⚠️ Para `make sim SQLITE=1` hace falta generar una vez `build/sql/aot_SQLite.c` con `AotMain` y
tener la amalgama de SQLite, igual que `test-sqldemo`.

## ⏭️ AL RETOMAR (6-sep, mediodía) — `E1`, y si da tiempo `G1`

**Lo que dijo Eduardo al parar**: *«a la vuelta retomamos `E1` y si nos da tiempo `G1`. `#475` y
`#444` hay que darle un ítem propio ya que es implementación nueva. Del resto, hay que ir cerrando
pendientes.»* → `#475` y `#444` ya son los hitos **`C1`** y **`T1`** en la tabla (versión sin decidir).

### Lo que cayó hoy — cinco arreglos, y tres los encontró el propio trabajo

1. ✅ **`#452`** — el `RESET` con un RUN vivo. **El enunciado de la ficha era falso**: RESET *sí*
   llegaba y se rechazaba a propósito con `BUSY` (lista blanca KILL/HELLO en el poll). Verificado en
   **las tres arquitecturas** —Discovery, Pico 2, C6—, que es todo el código tocado. Y el segundo
   defecto, que no hacía ruido: **el IDE se tragaba el BUSY** e imprimía «reset enviado».
2. ✅ **El tiempo del RUN** (`E1`): el IDE ya enseña `exit 0 (OK) en 94 ms`. El campo llevaba años
   viajando y `git log -S elapsedMs -- BpIde/` salía **vacío**.
3. ✅ **`#476`** — `make check-builtins`, con su **control negativo**.
4. ✅ **`#469`** — la fachada de ADC ya no inventa una lectura. El arreglo es el patrón genérico:
   *quien no tiene hardware registra un backend explícito, y la ausencia pasa a ser error*.
   ⚠️ **Pero la ficha NO queda cerrada**: lo hecho es la mitad de ADC, y `FICHAS.md` la mantiene
   abierta porque *«queda decidir qué se hace con las OTRAS que la auditoría destapó»* (`#480`).
   *(Corregido el 7-sep: esta lista la daba por cerrada entera y `FICHAS.md` decía lo contrario.)*
5. ✅ **`#478`** — **miVM no podía tocar un bus**: los ocho builtins de I2c/Spi/Uart/Neopixel se
   quedaron fuera del 4→8B de V4, y el arreglo estaba escrito 400 líneas más arriba en `case MOVE`.

### 📌 El hilo que une los cinco, y es lo que hay que retener

**Los tres bugs de verdad de hoy los encontró el trabajo, no la red de seguridad** — y siempre por
lo mismo: **el corpus** del `check` de `compat.sh` **no toca ni buses, ni GPIO, ni ADC, ni GUI**
(`#477`). ⛔ *(«el `check` lleva desactivado desde V4» era falso; corregido el 7-sep — ver arriba.)* El sample que reproduce `#478` **ya existía**, escrito
hace tiempo para bisecar otro cuelgue.

✅ **Y hoy quedan cuatro samples listos para entrar en el corpus**, byte-idénticos en stdout:
`BusBug` (7 líneas), `AdcDemo` (16), `MathRango` (29), `MathTest` (17). Los dos primeros lo son
**desde hoy**: hacía falta alinear los textos de los stubs y arreglar el orden.

### 🔧 Y una pieza de fondo que salió de ahí: `bpvm_out`

Las nueve fachadas escribían con **`printf` directo a stdout**; sus **42 llamadas** pasan a
`bpvm_out()`, que entrega al mismo `emit_text` que el `print` de BasicPlus. Arregla el orden en el
host **y** hace que en placa esos mensajes **viajen por el wire** — verificado en la Discovery.
⚠️ Ojo al reparto, que costó un intento: **`bpvm_diag` ya existía** (`#355`) y es OTRO canal —
diagnóstico a stderr/log. `bpvm_out` = salida del programa, sujeta a la paridad. Escrito en
`include/bpvm_out.h`.

### ⏭️ Para la vuelta

**`E1`** (quedan `#412`, el CRC de origen y las BD sin placa — los tres esperan decisión, no código)
y **`G1`**. Y de los pendientes, mi orden por daño: **`#477`** primero (enchufar el arnés: es lo que
impide que lo de hoy vuelva, y hay cuatro samples esperando), luego **`#480`** —lo que destapó la
auditoría de las 13 fachadas, con el `Wdt.disable()` del STM32 que **no desactiva** a la cabeza— y
**`#470`** (el C3 y el C6 se creen un S3 por la vía BP).

⚠️ **Placas**: la Discovery quedó grabada con lo de hoy; la Pico 2 y el C6 con lo del `RESET`, pero
**no con `bpvm_out`**. Y sigue habiendo un `/app/P.mod` y un `/app/adc.mod` de prueba en ellas.

## ⏭️ AL RETOMAR (5-sep, noche) — mañana **el IDE**, y `L1` ya no está en V6

**Lo que dijo Eduardo al parar**: *« lo podemos dejar por hoy. Mañana podemos ver el IDE y a partir
de ahí ir solucionando pendientes.»* O sea: **`E1` primero**, y luego las fichas sueltas.

### 1. La decisión del día: `L1` entera se va a V7

Salió de tirar de un hilo pequeño —*¿por qué no se puede sobrecargar una intrínseca?*— y acabó en
un cambio de fondo. La cadena, por si hay que reconstruirla:

1. El bloqueo es real y hay **tres** sitios, no uno: el camino de las intrínsecas compone su clave con
   `fs.name` **pelado** (`MivmEmitter.java:1069`, `:3926`, `:3972`) mientras el propio emisor tiene
   escrito que *«DEFINICIÓN y LLAMADA deben usar SIEMPRE este mismo helper»* (`:1678`, `emitName`).
2. Al mirar qué se gana, apareció que **ya conviven dos mecanismos** para lo mismo: `Integer` y
   `Comparable` son fuente en `bpstdlib/Core.bp` (545 líneas), pero `Object`, `Thread`, `Mutex` y
   `StringBuilder` se construyen **a mano en Java** (19 `makeMethod`, `SemanticAnalyzer.java:423-529`),
   y `integer` el tipo es un `case` de un `switch` (`resolveType`, `:1431`). La frontera no responde
   a ningún criterio: es lo que se pudo expresar en cada momento.
3. Eduardo lo nombró mejor que yo, y de memoria de Turbo Pascal: **el módulo raíz**, la unidad
   `System` escrita en el propio lenguaje. *«Me da más tranquilidad tener algo que se pueda LEER que
   algo que sólo existe en memoria. Y esto también es importante: un solo archivo, sin posibilidad
   de diferencias entre compilador y VM.»*
4. Su objeción —«nos queda un `Core.mod` enorme»— **se midió y no se sostiene**: declarar no cuesta
   RAM. El cargador **salta** la sección `interface` (`loader.c:235`), y `Math.mod` —que es casi sólo
   declaraciones— es **54,6 % interface y 16 B de exports**. Lo que crece es la flash de las cinco
   imágenes, porque `Core.mod` va embebido.

✅ **Decidido: `L1` a V7**, con su criterio de siempre —*«lo importante es que lo que hay ahora
funcione correctamente»*— y **se va casi entera**, porque lo que queda dentro son añadidos que se
diseñan **encima** del módulo raíz. **V6 queda en `E1` + `G1` + pendientes + la captura.**
✅ **Lo que NO se aplaza**: `#476`, el andamio. Las **226** entradas de las dos tablas de builtins se
mantienen a mano y divergir **no hace ruido** — lo denuncia el propio `builtins.c:355`.

### 2. La captura de pantalla, medida — y tres cosas que yo había escrito mal

`#475` reescrita con números (11 agentes; el diseño largo en `docs/V6_IDEAS.md`). Las correcciones,
que son lo que hay que retener:

- 🔴 **La cola de control de `A1` NO sirve para un verbo `SHOT`**: tiene dos lectores, los dos en el
  `next_cmd` del depurador — sólo se drena con la VM parada en un breakpoint. → **una sola orden**:
  `Gui.shot(path)` + el `GET` de siempre. Cero verbos nuevos.
- 🔴 **El P4 sí tiene framebuffer vivo** (el del driver DPI del IDF). Sólo el **C6** necesita bandas.
- 🔴 **El FS del P4 no está en PSRAM** — `fs_ram.c` ya no existe; las cinco montan littlefs sobre
  flash. Desgasta y sobrevive al reset. *(Había una memoria del proyecto diciendo lo contrario, de
  junio; corregida. Es el riesgo típico: un dato cierto en V3 que sigue en pie en V6.)*
- ✅ **Comprimir ya está escrito**: el LZ4 de LVGL, `.c` ya dado de alta en los tres builds gráficos,
  apagado por **una línea** (`lv_conf.h:853`). 21,8× a 152,6× medido.
- ⚠️ **Y el orden se invierte**: comprimir es el **primer** paso, el fichero es lo opcional — el crudo
  no cabe en RAM ni en el C6 (115 200 B contra 68 264) ni en la Discovery (771 027 B contra 433 416).
- ⚠️ **Se empieza en el HOST, no en el C6.** Mi versión anterior rompía la cascada del proyecto.

### 3. `#477` — lo que más me escuece del día

El oráculo exacto de la GUI **existe desde V4 y nadie lo ejecuta**: `dumpTree` es byte-idéntico en las
dos VMs (medido hoy en 6 samples), **22 samples lo llaman**, y el corpus del `check` de `compat.sh`
no tiene un solo sample de GUI. ⛔ *(decía «el `check` lleva desactivado desde V4»: falso, corregido
el 7-sep.)* Antes de construir la segunda vía
(capturar, comprimir, bajar) hay que enchufar la primera, que es gratis.
⚠️ Con sus tres agujeros medidos: el chart es invisible en el dump; es **ciego a la rotación** (y en
la Discovery `disp_set_rotation` es un **no-op declarado**, así que el oráculo sale VERDE con el panel
sin girar); y color y fuente son render-only por contrato.
⚠️ Y una tesis mía **falsada al medirla**: no es cierto que sólo cambie la línea `screen` al cambiar
de resolución — `Gui.bp:824` lee el tamaño de pantalla y **todo el camino Forms** se mueve con ella.

### ⏭️ Para mañana — `E1`, con el reconocimiento YA HECHO

Dejé corriendo un reconocimiento de los seis puntos de `E1` (un agente por punto + verificación
adversarial). Lo que trajo, y **`FICHAS.md` ya está corregida** con ello:

🟢 **`E1` tiene CUATRO puntos vivos, no seis.** El árbol por color está **hecho desde el 26-ago**
(`9cc33ee6` + `fb66e579`, `PicoExplorer.java:1296`) y seguía listado como pendiente. *La lista mentía
al alza justo en el hito que se iba a abordar* — [[no-acumular-pendientes]] en su forma más barata.

**1. `#452` primero, porque es LO ROTO — y su enunciado era FALSO.** RESET **sí llega**: la placa lo
lee, lo parsea y lo **rechaza a propósito** con `ERROR BUSY`, porque el poll que atiende el cable
durante un RUN tiene **lista blanca** (KILL, HELLO). Reproducido en el simulador. La diferencia con
KILL cabe en una línea: KILL tiene rama en el poll y **delega**; RESET sólo existe en el despachador
de reposo, al que no se vuelve hasta que `bpvm_run()` retorna.
- 🔴 **Es además una divergencia de las dos VMs que no estaba escrita**: miVM **sí** contesta RESET
  con un RUN corriendo — y no porque nadie lo pensara, sino porque su lector es un hilo aparte
  (`DebugServer.java:175`).
- 🔴 **Y el IDE se traga el BUSY**: `BpvmClient.reset()` captura la `IOException` (y `WireError` la
  extiende) y la manda a un `diag()` que puede no ir a ninguna parte; la consola imprime «reset
  enviado». El usuario cree que reinició, **la placa sigue corriendo y encima se ha quedado sin
  conexión**. *Errores sí, silenciosos no*, en estado puro.
- Se parte en dos: **1a sin placa** (que el IDE deje de mentir + el poll del simulador) y **1b con
  placas** (copiar la forma de KILL en los **tres** ficheros de REPL — las cinco familias son tres).

**2. El tiempo del RUN (sesión corta), pero el número de la ficha CADUCÓ.** El `durationMs` del SAVE
vale 0 siempre (los `fs_save` son no-ops desde littlefs; comprobado en el `.map`, no en el comentario:
`.text.fs_save … 0x2`). El bueno es el **`elapsedMs` del `EXITED`**, que mandan los **cinco** emisores
y que el IDE no ha parseado **nunca**.
⚠️ Decisión antes de enseñarlo: **no miden lo mismo** — miVM arranca el cronómetro antes de cargar y
enlazar, la placa justo antes de `bpvm_run`. Sin decidirlo, el PC parecerá siempre más lento.

**3. El `capabilities` del `HELLO` (sesión corta, sin tocar imágenes):** cero ocurrencias en las 39
clases del IDE, y las tres cadenas **ya divergieron** — el ESP32 no declara `DEBUG` **aunque lo tiene
cableado entero**, y el STM32 tampoco, que ahí sí es correcto (`stm32_repl.c` no llama a
`bpvm_dbg_wire_*` ni una vez, confirmado en el `.map` del Nucleo). Síntoma que ve el usuario: «Debug
on Device» contra una Nucleo → `UNSUPPORTED`.

**Lo que NO tocar mañana**, y por qué:
- **`#412`** no está roto, **falta** — y arrastra un **builtin nuevo (id 232)** que obliga a
  reflashear las cinco familias **a la vez** que se publica el compilador. Va con una ronda de flasheo.
- **Probar BD sin placa**: `make test-sqldemo` ya corre el ciclo **entero** en el PC. Lo que queda es
  una decisión de diseño, no una sesión. *(Y hoy `SQLite.pack`, 1 130 496 B, ni monta contra la región
  simulada de 1 MB.)*
- **CRC de origen**: sesión larga, y el mensaje **no cabe** — `link_error` es `char[160]` y el texto
  actual ya gasta 78.

**Dos hallazgos de propina, anotados aquí para que no se pierdan:**
- 🟡 `scheduler_smp.c:114` y `:158` llaman al `poll_cb` **sin la guarda `vm->io == NULL`** que sí
  tienen `scheduler.c:100` y `builtins.c:970`/`:1015`. **Latente, no vivo** (SMP es opt-in y no está
  en el build por defecto), pero es exactamente la clase de fallo que cerró `A1.7`.
- 🟡 El guardián del fat-jar (`#429`) compara la fecha del **compilador** embebido — pero el IDE
  empaqueta **también la VM-Java**, y para eso no avisa nadie.

⚠️ Y la trampa de siempre antes de tocar el IDE: **el fat-jar no se reconstruye con el IDE abierto**.

## ⏭️ AL RETOMAR — `P2` CERRADA (4-sep): la pantalla del C6 entera. Empieza la segunda mitad de V6

**Dónde estamos.** U1–U6 (unificación), `#466`, `P1` (C3 y C6 sin pantalla) y `P2` (la pantalla
SPI del C6: driver, memoria medida, rotación confirmada) están cerrados. El marcador de Eduardo
—*«cuando tengamos la ESP32-C6 habremos llegado al ecuador de V6»*— quedó atrás. La segunda mitad,
según el cuadro de hitos de `FICHAS.md`: **A1** (arquitectura por niveles sobre código único, que
iba después de la unificación), **G1**, **L1** y **E1**. Y las fichas abiertas de siempre: `#462`
(cuanto por opcodes / suelo de 48 ms / carrera del exit-code en KILL), `#456`, `#452`, `#444`,
`U6.F1`. Decisiones que esperan a Eduardo: **`#468`** (la stdlib en pack XIP: zona vs tabla embebida, con
o sin la tabla de símbolos por referencia — analizada con números), `Neopixel` en la familia ESP32,
y si se prueba de cero la subida de `Gui`+`Json` desde el IDE (exige borrar los dos de `/lib` de la
C6). La del tamaño del screen ya está tomada y hecha (`P2.3`: el screen mide lo que mide el panel).

**Placas.** La C6 (COM3) lleva la imagen con GUI de 128 KB (4-sep 00:15) con `/lib/Gui.mod`,
`/lib/Json.mod`, `/app/GuiColorDemo.mod` y `/app/GuiRotCycle.mod`, `log=0`. Pico 2, S3, Nucleo y
Discovery llevan las imágenes del 3-sep. Metro, C3 y P4 siguen con imágenes anteriores a `#466`/U4/U5
(sus números ya están fijados; se actualizan cuando se graben).

### El orden que venía (para contexto)

**El orden lo fijó Eduardo al cerrar el 2-sep:** *«Terminaremos U6, U4 y U5. Después podemos hacer
P1 (sin pantalla) y P2 (añadir la pantalla a ESP32-C6).»*

### Lo que quedó hecho el 2-sep (segunda mitad, tras la pausa por tokens)
- ✅ **P4 repasado**: `/lib` limpio de por sí (15/15), `/app/Core.mod` fuera con su OK.
- ✅ **`U6.11`**: el STM32 al planificador — Nucleo y Discovery compilan headless, `test_mem`
  22/22. **Las cinco familias deciden su memoria con `bpvm_mem_plan()`.**
- ✅ **`#466` en tres pasos**, con la regla de Eduardo (*el SO repone `/lib` por versión y CRC; el
  IDE pregunta a la placa por cada dependencia y sólo sube lo que falta o es más viejo*): `STAT`
  por nombre + `magic`; `src/bpvm_mods.c` llamado por los tres instaladores (`test-mods` 16/16);
  el IDE con `statModule`, sin `EMBEDDED_CORE_MODS` (`StatModuleSmoke` 5/5). Y de regalo: **el
  CRC del wire salía negativo** (bit 31, `long` de 32 bits) y el IDE re-subía la mitad de los
  ficheros idénticos — arreglado en los dos lados. Seis builds verdes, paridad 38/0/0,
  `sim-smoke` 45/45, fat-jar del IDE reconstruido. Commits `1a7c1924`, `f1a9c23b`, `0e1a425b`.

### En este orden
1. ✅ **`#466` CERRADA (3-sep)**: en la Pico 2, el SO repuso solo un `Math.mod` MOD6 plantado a
   propósito (4a), y el Run de `MathRango` desde `BpIde-6.0.jar` dijo `Math.mod: ya en la placa,
   idéntico — no se sube` con las 29 líneas iguales al host (4b). De camino salió y se cerró
   **`#467`**: el compilador resolvía `import Math` contra `samples/out` (la stdlib entera de V5,
   MOD6) antes que contra `bpstdlib`; ahora la stdlib va PRIMERO en los tres buscadores del
   frontend, el `BpVM.cfg` del fichero manda sobre el del cwd, y el IDE es `BpIde-6.0.jar`
   (`bpide.bat` arranca desde la raíz del repo).
2. ✅ **S3 (3-sep)**: regrabado con la imagen de las 19:01 (`#466` + `U4.1`); su `/lib` era todo de
   V5 (13 MOD6 — ayer lo di por limpio porque el ESP32 no avisaba) y el primer arranque los repuso
   los 13 él solo; memoria idéntica (160 KB, techo 272, margen 26564); `STAT` por nombre OK.
3. ✅ **Nucleo y Discovery (3-sep, grabadas por Eduardo)**: las dos dicen `vm: 512 KB en SRAM
   estática (todo lo que deja la región)` e INFO 393216/131072; sus `/lib` estaban vacíos y la
   imagen instaló los 14; `STAT` por nombre OK. **`U6` CERRADO en las cinco familias.**
   ⏭️ `U6.12` (observación de Eduardo): la Discovery pasa de 512 a **1536 KB** de VM
   (`BOARD_VM_BYTES` por placa; medido: quedan 457 KB libres). ✅ Regrabada por Eduardo (sello
   19:26): INFO 1179648/393216, el panel del IDE `heap 1.1 MB + stack 384 KB`.
4. ✅ **`U4` HECHO (3-sep, `U4.1`)**: un generador (`scripts/regen_mods.sh`), tres ficheros de
   sólo datos, el bucle en `src/bpvm_mods.c`; blobs byte-idénticos, seis builds, Pico 2 verificada.
   Las demás placas lo reciben al regrabarse.
5. ✅ **`U5` HECHO (3-sep, `U5.1`)**: `bpvm_handles.[ch]` con la tabla (tipo, constantes, API)
   sacada de `heap.c` y de diez campos sueltos de `bpvm_t`; el ABI del `.mdn` intacto (los helpers
   no leen esos campos por offset); paridad 38/0/0, `test_smp_handles` limpio, seis builds, Pico 2
   byte-idéntica. **Con esto la serie de unificación U1–U6 está cerrada.**
6. ✅ **`P1.C6` HECHO (3-sep, tarde)**: `esp32c6/` clonado del C3 (siete ficheros), un arreglo en la
   cintura común (`SOC_UART_HP_NUM`, el C6 cuenta su UART LP), aprovisionado por el wire, las tres
   constantes MEDIDAS (bloque contiguo 376 KB, margen 16932 → objetivo 192 KB), `fib(28)` 11823 ms
   (C3 11315), `MathRango` 29 líneas byte-idénticas al host. **El ecuador de V6.**
7. ✅ **`P2.1` HECHA (3-sep noche → 4-sep madrugada)** — la pantalla del C6 **funciona a la
   primera** (Eduardo vio los tres botones con sus colores); la imagen con LVGL cabe en la
   `factory` de 1,5 MB. Medido con la GUI en marcha (tres pasadas), LVGL cuesta ~108 KB estáticos
   y hasta 103 824 B en marcha (va sobre el heap de C), así que la imagen con pantalla lleva
   **objetivo 128 KB, margen 103 824**; la de 128 grabada y confirmada (`vm: 128 KB … techo 194
   por el margen del sistema`, INFO 65536/65536, mínimo histórico 68 264 B con el demo; `log=0`).
   `test_mem` 30/30. El C6 (COM3) tiene `/lib/Gui.mod`, `/lib/Json.mod` y `/app/GuiColorDemo.mod`.
   Siguiente: **`P2.2`** (PWM del backlight, rotación con gaps, LED RGB como `Neopixel`, el IDE
   subiendo `Gui`+`Json` solos; detalle en la ficha).
8. ✅ **`P2.2` HECHA (4-sep)** — la rotación va por MADCTL del panel con el gap de cada orientación
   (tabla ST7789 240×240: 180 → y+80, 270 → x+80) y repintado entero; el aviso a consola, no a
   stdout. Sample nuevo `GuiRotCycle.bp` (gira solo cada 3 s, marcas en las esquinas; paridad host
   25/25). **Eduardo lo vio: «la demo funciona perfectamente, y sí las esquinas coinciden»** — las
   cuatro orientaciones, sin franja. La lista de P2.2 se acortó con hechos: el IDE YA resuelve deps
   transitivas (Gui→Json), el PWM del backlight no tiene API de brillo a la que servir (se queda el
   GPIO), y `Neopixel` sólo tiene backend en la Pico (en ESP32 es no-op: adaptador nuevo, ficha
   aparte). Punto de diseño anotado: el screen lógico dice 480×320 en todas las placas (paridad del
   dump) mientras LVGL alinea contra el panel real. **Con esto `P2` queda cerrada.**
9. ✅ **`P2.3` HECHA (4-sep)** — decisión de Eduardo: *«la resolución ha de ser la que tenga la
   pantalla, no hay otra»*. Contrato nuevo `bpvm_gui_disp_native_size` (C6 240×240, DK2 800×480,
   P4 según el ENV, host 0) y el modelo lo pregunta al crear el screen; `--screen=` y el simulador
   siguen ganando. La C6 dice `screen [240x240]`; el host con `--screen=240x240` da el mismo dump
   byte a byte; sin él, paridad con miVM intacta; sim-smoke OK; compilan host, C6, P4 y Discovery.
10. 🟡 **`#468` ABIERTA (4-sep): la stdlib en un pack XIP** — dirección de Eduardo (*«subir el pack
   de la librería estándar+Json+Gui, ahorrar 40 o 50 K»*), **analizada con números en la C6**: los
   tres módulos ocupan 29 224 B del bloque de la VM y un pack XIP devuelve 22 268 B al heap (de 31
   a ~53 KB libres); la otra mitad es la tabla de símbolos (49 152 B en el malloc de plataforma),
   que sólo baja con nombres por referencia desde flash. El nudo: el FS eclipsa a la zona y `#466`
   repone `/lib` en cada arranque → instalador + `STAT` tienen que ver la zona. Falta su decisión
   (pack en la zona vs tabla embebida XIP; con o sin la tabla de símbolos). Detalle en la ficha.
11. 🟡 **`A1` ABIERTA (4-sep, tarde): la arquitectura de ejecución común** — Eduardo preguntó
   cuántas tareas de FreeRTOS corren (censo: UNA nuestra en ESP32 y Pico con todo dentro, cero en
   STM32, `vm`/`main`/`wire_uart` según la familia) y se midió en la C6: lo de entre cuantos
   cuesta 1 %, la salida 0,84 ms por `print` (seis mensajes OUTPUT por línea, 122 KB → 816 KB),
   `AllocBench` 0,66 ms por vuelta con asignación. **Decisión: `vm` sólo opcodes + `io` todo lo
   demás, tres colas, igual en todos los micros; de momento 1 VM, 1 core, 2 hilos de SO; el
   futuro son dos VM sobre una cola de threads BP (el camino SMP, aparcado sin cerrarlo).**
   Decidido después: **FreeRTOS en todas las familias**, el PC y la **C6** como modelo, y **todo
   en código común** (las familias sólo ponen transporte, primitivas y la creación de las dos
   tareas). Tareas concretas `A1.1`–`A1.7` en la ficha. Samples nuevos: `PrintBench.bp`,
   `AllocBench.bp` (en `samples/benchmarks/`).
12. ✅ **`A1.1` HECHA (5-sep): el PC ya corre con DOS hilos.** `include/bpvm_io.h` +
   `src/bpvm_io.c` (común): hilo `io`, cola con espera acotada, líneas armadas y entregadas de
   una pieza, apagado que drena antes de unir. `emit_text` encola; el scheduler deja de llamar al
   `poll_cb` cuando hay `io`. **Paridad 38/38 byte-idéntica con los dos hilos en cada sample**,
   más `test-mem`, `test-mods`, `test-smphandles`, `sim-smoke` 45/45 y la prueba nueva
   `make io-smoke`: KILL en 3 ms calculando y 17 ms imprimiendo a chorro, **un `OUTPUT` por
   línea** (antes cuatro), salida exacta. Enlaza en las seis imágenes (ninguna lo usa aún).
13. ✅ **`A1.2` HECHA (5-sep): la C6 con las dos tareas** — 20 líneas en `esp32/common`
   (un adaptador de `poll` y el arranque de `io` alrededor de `bpvm_run`), así que **toda la
   familia ESP32 lo hereda**. Medido en placa: cálculo IGUAL (fib(28) 23 580 vs 23 640 ms;
   `AllocBench` 13 188 vs 13 129), salida **3,6× más rápida** (2 000 líneas: 1 700 → 470 ms),
   **6× menos mensajes** `OUTPUT` (11 944 → 1 988) y **3,4× menos bytes** por el wire (816 → 238
   KB); KILL en 46 ms imprimiendo y 18 ms calculando. La pantalla sigue (`GuiRotCycle`).
   ⚠️ Con el depurador armado `io` no arranca (su `pause_cb` lee el wire desde la VM): se
   resuelve en `A1.7`.
   🔬 **Por el camino, un bug de plataforma que sólo se ve en placa**: `cond_timed_wait` con menos
   de un tick daba 0 y no esperaba, así que `io` giraba en vacío y fib(28) tardaba 140 600 ms.
   Arreglado en ESP32 y Pico (redondeo hacia arriba). Pasó los 38 samples del PC con el fallo
   dentro — la cascada haciendo su trabajo.
14. ✅ **`A1.4` HECHA (5-sep): la Pico 2 con las dos tareas.** Mismo gesto que en el ESP32.
   Medido: cálculo igual (fib(28) 17 091 vs 17 157 ms), `AllocBench` +3,6 %, **2 000 líneas de
   7 694 → 3 958 ms (1,9×)**, 6× menos mensajes, 3,4× menos bytes, KILL en 33 ms.
   🔬 **Y el segundo bug que sólo se ve en placa: LA PRIORIDAD ES CONTRATO.** El hilo `io` nacía
   por DEBAJO de `vm_task` en la Pico y no se ejecutaba mientras el programa calculaba: un KILL
   no llegaba nunca (9,9 s y `EXITED OK`). El contrato gana `bpvm_platform_thread_create_io` y la
   regla, medida en las dos placas: **`io` a la MISMA prioridad que la VM** — por debajo no
   funciona, por encima cuesta caudal (C6 486 → 848 ms; Pico 3 958 → 4 832).
15. ✅ **`A1.5` HECHA (5-sep): FreeRTOS en el STM32, las dos placas con las dos tareas.**
   Kernel: el MISMO que la Pico (`FreeRTOS-LTS` V11.3.0, puerto `ARM_CM33_NTZ`) — **de ST no
   sale**: el paquete U5 trae ThreadX y una capa que emula la API de FreeRTOS encima, y CubeMX no
   ofrece FREERTOS para U5 aunque sí para el L552, que es el mismo núcleo. La plataforma pasa a
   `src/platform_freertos.c`, **común** (de las 363 líneas de la Pico sólo 4 eran suyas); migrar
   la Pico es ficha aparte, exige placa. Medido: `PrintBench` **71 357 → 20 464 ms (3,5×)** en las
   dos placas (aquí la UART a 115 200 ES el cuello), cálculo igual en la Nucleo y **11 % más
   rápido** en la Discovery (la VM ya no sondea el wire entre cuantos), KILL en 15 ms.
   🔬 Tercer fallo de plataforma que sólo se ve en placa: se paraba a la quinta ejecución porque
   `vTaskDelete(NULL)` deja la pila en manos de la tarea ociosa, que nunca corría. Lo dijo el
   gancho de malloc fallido en el log; ahora borra el `join`. ⏭️ **Falta que Eduardo mire la
   pantalla de la Discovery** (el modelo de 800×480 se arma bien).
   ✅ **La pantalla de la Discovery, vista por Eduardo con las dos tareas: «se ve bien, todo OK».**
16. 🔄 **`A1.6` APLAZADA Y REORIENTADA (Eduardo, 5-sep)**: LVGL **no** se mueve a `io`. Se trazó
   dónde corre hoy su lazo —lo conduce el programa BP dentro de `vm`, vía `Gui.run()` →
   `__guiRunOnce` → `lv_timer_handler`, y nadie más lo bombea— y el coste ya estaba medido en
   `#424`: 0,4 ms por vuelta cada 33 ms, ~1 % de CPU. No hay caso por rendimiento, y LVGL no es
   reentrante. Lo que Eduardo quiere es que **`Gui.run()` tenga su propio hilo BP** (verde, luego
   sin problema de reentrancia) para no bloquear la aplicación. ⚠️ El obstáculo, ya localizado:
   el bombeo **duerme el hilo del SO** (`vTaskDelay`/`__WFI`/`SDL_Delay`), o sea todos los hilos
   BP; tiene que volver en vez de dormir y dejar turno al scheduler de la VM. Detalle en la ficha.
   **Siguiente: `A1.3`** (S3, C3 y P4: el código ya está, falta verificar EN PLACA) y `A1.7` (el
   depurador por la cola de control). Fichas nuevas propuestas: migrar la Pico a
   `src/platform_freertos.c`; y el `Neopixel` de la familia ESP32.

## 🏁 LA UNIFICACIÓN, CONCLUIDA (Eduardo, 5-sep)

*«Con eso creo que el proceso de unificación lo podemos dar por concluido. Ahora cualquier cosa
que implementemos nueva o reformemos prácticamente hay que hacer 1 vez. Eso nos permite crecer de
forma lineal, sin que el número de placas/micros afecte apenas.»*

Cierran `U1`–`U6`, `P1`–`P2` y `A1`, con el número de `A2` detrás: **91,8 % del código de cada
firmware es común**, 0,7 % de placa, y lo único específico que queda es la pantalla del P4 y del
C6. La evidencia práctica del crecimiento lineal está en las tres últimas fichas: el C6 entró con
siete ficheros, `A1.2` llevó las dos tareas a toda la familia ESP32 con veinte líneas, y `A1.3` se
cerró **sin escribir código**.

⚠️ Lo barato es la **placa dentro de una familia**, no la familia: un silicio con otro SDK sigue
costando su cintura. Y quedan, con ficha y medida, `A1.6`, la migración de la plataforma de la
Pico y `#468` — features y una duda medida, no unificación pendiente.

### Lo que queda en la mesa

**Del modelo de capas (`A3`, 5-sep), abiertas para ir arreglando poco a poco** — decisión de
Eduardo: *«Registralo como pendiente. Lo iremos arreglando poco a poco.»*

| Ficha | Qué | Tamaño |
|---|---|---|
| 🔴 `#469` | La fachada de ADC **miente en el STM32**: sin backend registrado devuelve una rampa inventada y el pinout de la Pico. La paridad dual-VM no puede verlo. Hay que mirar las otras 16 fachadas por si tienen el mismo stub complaciente | mediana |
| 🟠 `#470` | La identidad de placa se contesta por **dos caminos** y ya divergieron; un programa BP en un C3 **se cree un S3** | mediana, y hay que decidir antes qué significa cada campo |
| 🟡 `#471` | El nombre **`Pico`** llega hasta el usuario (`Pico.uptimeMs()` en una STM32). Toca la stdlib: **cambio de lenguaje, decide Eduardo** | pequeña + decisión |
| 🟡 `#472` | El común nombra **dos familias** donde quería decir «micro»; el STM32 acaba con TLS emulada | media hora |
| 📋 `#473` | El **resto de la auditoría, sin verificar** (12 de 90). Dentro, cosas que pintan serias: el `EXITED` del STM32 interpola cadenas del usuario sin escapar; `RUN` escrito cuatro veces | verificar antes de tocar |

### Lo demás que queda (por si se retoma)

## ⏭️ (anterior) — el orden que dio Eduardo al parar (5-sep)

*«Rematamos todo lo que hay pendiente de A1. Luego, me gustaría conocer las proporciones de
código específico del micro, el específico de la familia y el común a todos los micros. Después
de A1 debería quedar muy poco código específico.»*

### 1. Rematar `A1`

✅ **`A1.3` hecha para C3 y P4 (5-sep)**: sin código nuevo, sólo recompilar y grabar. `PrintBench`
**1 610 → 305 ms** en el C3 (5,3×, la mejor de todas las placas) y **71 075 → 20 365** en el P4
(3,5×; sale por puente UART, como el STM32). La pantalla del P4 sigue, a 1 024×600. ⚠️ El
`AllocBench` del C3 sube un 10 %, repetible, pero **no es `A1`**: la C6 —mismo silicio, misma VM—
salió plana; el «antes» del C3 era del 1-sep e incluye `U4`, `U5`, `U6` y `#466`, y el sospechoso
es el planificador de memoria de `U6`. Anotado como pregunta de `U6`.

🔴 **La migración de la Pico al fichero común NO se subió, y por un motivo medido**: con
`src/platform_freertos.c`, `PrintBench` en la Pico pasa de **4 040 a 5 330 ms** (32 % peor,
repetible a ±10 ms, aislado por bisección). Curiosamente el KILL MEJORA (33 → 2 ms), o sea que la
diferencia está en el reparto de turno, no en trabajo de más. No se sube lo que no se entiende.

✅ **`A1.3` COMPLETA (5-sep): C3, P4 y S3 verificados en placa.** El S3 da `PrintBench`
**71 075 → 20 343 ms** (3,5×) y `AllocBench` **plano** (20 244 → 20 384) — y ese número zanja lo
del C3: con dos controles (C6 y S3) queda claro que `A1` no cuesta nada en asignación y que el
10 % del C3 pasó entre el 1 y el 3 de septiembre, o sea en `U6`.

| Paso | Qué falta | Qué hace falta tener delante |
|---|---|---|
| `A1.7` | El depurador por la cola de control: hoy su `pause_cb` lee el wire desde la tarea `vm`, así que **con el depurador armado `io` NO arranca** (interbloqueo puesto a propósito en las tres familias) | una placa y el IDE |
| — | Migrar la Pico a `src/platform_freertos.c` (es un BORRADO, no una fusión: de sus 363 líneas sólo 4 eran suyas) | la **Pico 2** conectada, para verificar en placa lo que hoy funciona |
| `A1.6` | 🔄 **Aplazada y reorientada** por Eduardo: no se mueve LVGL, se le da a `Gui.run()` su propio hilo BP. El obstáculo ya está localizado (el bombeo duerme el hilo del SO) | — |

### 2. El censo de proporciones: común / familia / micro — ✅ HECHO (5-sep)

**El 91,8 % del código de cada firmware es COMÚN**; 7,6 % de familia y **0,7 % de placa**. La
hipótesis de Eduardo se sostiene con margen. Medido por el **artefacto** (el `.map` del enlazador,
no las líneas por carpeta: en la Nucleo `gui_display_sdl.o` está en el build y aporta 0 bytes) y
repetible con `bpgenvm-c/scripts/censo_reparto.py`. Detalle e interpretación en la ficha `A2`.

Lo más útil no es el total: la **Pico es la familia menos unificada** (15,4 %, por ser la más
antigua) y las **dos STM32 las más comunes** (95 %), y lo único que queda como código de placa es
**la pantalla** del P4 y del C6 — que es donde debe estar.

<sub>El enunciado original, por si hay que rehacerlo:</sub>

**La pregunta de Eduardo, tal cual**: cuánto código es específico de un micro, cuánto de una
familia y cuánto común — y su hipótesis, que después de `A1` debería quedar muy poco específico.

📌 Es un censo, así que vale la norma del proyecto: **censar por lo que el código HACE, no por la
carpeta en que está**. Un fichero en `src/` que sólo compila una familia no es común; y
`esp32/common/` es específico de familia aunque se llame «common». Propuesta de método, a
validar con él antes de contar:

- **Común** = lo que compilan TODAS las imágenes (`src/` menos lo que cada build excluye).
- **De familia** = `esp32/common/`, `pico/` sin el `board_desc`, `stm32/port/`… lo que sirve a
  varias placas del mismo silicio.
- **De micro/placa** = `esp32c6/main/`, `esp32p4/main/`, los `Core/` de CubeIDE, `pico/boards/`.
- Y las tres trampas conocidas: los **generados** (blobs de stdlib, `.mdn`) no cuentan como
  código escrito; los `Core/` de CubeMX son **generados por ST**, no nuestros, y hay que contarlos
  aparte o el número miente; y `src/platform_freertos.c` es común de nacimiento pero **hoy sólo lo
  compila el STM32** — o sea, el censo tiene que decir «común» y «común EN USO».
- Lo interesante no es sólo el total: es **la tendencia** (qué había antes de `A1` y qué hay
  después) y **el desglose por eje** (transporte, memoria, FS, GUI, plataforma), que es lo que
  dice dónde queda trabajo de unificación.

### Riesgos que acechan
- **IDE viejo (dist V5) contra firmware nuevo**: sigue subiendo stdlib a `/lib` por CRC, y el
  instalador nuevo la REPONE al siguiente arranque (misma versión, CRC distinto). Sólo se pelean si
  se mezclan versiones; el dist no se toca.
- **IDE nuevo contra firmware viejo** (sin `STAT` por nombre): `statModule` devuelve null y el IDE
  sube como siempre; el CRC con signo lo normaliza `crcSinSigno`.
- **El S3 y las dos STM32 llevan imágenes anteriores a todo esto** hasta que se graben.

## ⏭️ AL RETOMAR (2-sep, 2ª pausa — por límite de tokens) — el RESTO de placas

El repaso general quedó **a medias por la mitad buena**: cuatro de cinco familias verificadas con
la imagen final `U6.10` en su placa (P4, Metro, Pico 2, C3) y tres `/lib` limpiados a mano
(Metro 14, Pico 2 16, S3 2). Falta, en este orden:

1. ✅ **P4** (2-sep, al retomar): `/lib` limpio de por sí (15/15, 0 avisos); `/app/Core.mod`
   borrado con el OK de Eduardo, reset por el wire, 21 ficheros y los mismos números de memoria.
   Trae un matiz a `#466`: pasó por el IDE (tiene `/app/Json.mod`, `/app/JsonDemo.mod`) y su
   `/lib` no se ensució — a dónde escribe el IDE se mira en su código antes del arreglo de raíz.
2. **S3** (COM9): regrabar con la `U6.10` **final** (lleva la previa a la regla unificada; mismos
   números) y comprobar que el `/lib` sigue limpio — se ensucia en cuanto el IDE de V5 sube
   dependencias.
3. ✅ **STM32 Nucleo y Discovery, en código (`U6.11`)**: enumerador = el array estático de 512 KB
   como región exclusiva sin margen, objetivo 0 → los 512 KB de siempre; INFO y RUN sobre lo
   planificado; `test_mem` 22/22; los dos `.elf` compilados headless con 0 errores. **Falta
   grabarlas** (Eduardo, desde CubeIDE) y leer su línea `vm:` — y revisar su `/lib`, que su
   instalador no avisa del rancio.
4. **`#466` de raíz — ANALIZADO en el código; decisión de Eduardo pendiente** (mecanismo, tres
   opciones y recomendación, en la ficha): el IDE sube a `/lib` las deps que están en
   `EMBEDDED_CORE_MODS` (+Gui) con content-check por CRC — a propósito, por un bug de vtables del
   13-jun; los tres instaladores copian «si falta»; y el aviso de `#422` sólo vive en la Pico: en
   el ESP32 se perdió al regenerar los blobs (`#446`: iba en un fichero GENERADO) y el STM32 nunca
   lo tuvo. Recomendación **A**: el IDE deja `/lib` en paz (sus deps de stdlib a `/app/<proj>`, que
   gana) y un helper común refresca `/lib` cuando no coincide con lo embebido. Mientras no esté,
   cada Run desde el IDE vuelve a ensuciar las placas limpiadas hoy. 🛠️ **Al retomar: la regla
   definitiva de Eduardo está en la ficha, y los pasos (1) `STAT` por nombre + `magic` y (2) el
   helper común del instalador ya están hechos y verificados (host + seis builds). (3) el IDE también:
   pregunta a la placa por cada dependencia, fuera `EMBEDDED_CORE_MODS`, verificado con el cliente
   real contra el simulador y fat-jar reconstruido. **Falta (4) las placas**: Metro y Pico 2 con el
   `uf2` nuevo (`BOOTSEL` por el wire) y ver `lib: … repuesto` en su log; luego un Run desde el IDE
   nuevo sin subir stdlib. De paso: el CRC del wire salía NEGATIVO (bit 31, `long` de 32 bits) y
   el IDE re-subía la mitad de los ficheros idénticos — arreglado en los dos lados.**

La matriz completa, con lo hecho, está justo debajo.

## ⏭️ (2-sep, 1ª pausa) — REPASO GENERAL de imágenes y placas

Eduardo, al parar: *«A la vuelta repasamos el estado general de todas las imágenes y placas.»*
Ésta es la matriz de partida, escrita en caliente el 2-sep a las 14:30, no reconstruida:

| placa | imagen que LLEVA | frente al repo (`U6.10` final, regla unificada) | pendientes en la placa |
|---|---|---|---|
| **P4** (COM14) | `U6.10` final, grabada 14:20 | ✅ **es la del repo** — verificada byte a byte (28 668 KiB @0x48000a7c), y otra vez tras el reset del repaso | ✅ `/lib` **limpio de por sí** (15/15, 0 avisos, aunque pasó por el IDE); ✅ `/app/Core.mod` borrado (2-sep, al retomar) |
| **S3** (COM9) | ✅ **`U4.1` + `#466`**, regrabada el 3-sep (19:01) | ✅ verificada: mismos números (160 KB, techo 272, margen 26564) | ✅ `/lib` **repuesto por la imagen** (13 MOD6 de V5 → MOD7 al primer arranque; ayer parecía limpio porque el ESP32 no avisaba) |
| **C3** (COM3) | ✅ **`U6.10` final**, regrabada en el repaso (2-sep) | ✅ verificada: 128 KB, techo 136 por el contiguo, 64/64 — idéntico | ✅ `/lib` **limpio de origen** (lo aprovisionó el instalador; el IDE de V5 nunca le subió nada: 0 avisos), ✅ `log` devuelto a 0 |
| **Metro** (COM4) | ✅ **`U6.10` final**, regrabada en el repaso (2-sep) | ✅ verificada en los dos modos, byte a byte | ✅ `/lib` **limpio** (los 14 de V5 borrados y repuestos por la imagen, `#466`); NO había `/app/Core.mod`; ENV `psram=0`, `SQLite=2`, `log=1` — **volverá a ensuciarse mientras el IDE de V5 suba stdlib** |
| **Pico 2** (COM22) | ✅ **`U6.10` final**, regrabada en el repaso (2-sep; llevaba la del 29-ago) | ✅ **rama SRAM en variante A verificada**: `357 KB @ 0x20024ab8`, y su `stack=90` del ENV respetado (heap 267 + pilas 90; INFO 273736/92160, igual que antes) | ✅ `/lib` **limpio** (16 de V5 y de una imagen vieja borrados y repuestos por la imagen, `#466`); ENV `stack=90`, `gc=1`, `mpu=1`, `log=1`; FS de 1 MB al 24 % — **volverá a ensuciarse mientras el IDE de V5 suba stdlib** |
| **STM32 Nucleo / Discovery** | ✅ las dos con **`U4.1` + `#466`** (Eduardo, 3-sep, sellos 19:08 y 19:12) | ✅ verificadas: `vm: 512 KB en SRAM estática`, INFO 393216/131072 | `/lib` instalado entero por la imagen en las dos (estaban vacíos); en la Nucleo un `/app/Json.mod` MOD6 que repondrá el IDE |
| host / simulador | — | ✅ repo, `test-mem` 19/19, `sim-smoke` 40/40, paridad 38/0/0 | — |

**Los seis builds compilan** con el repo actual (host, Pico, S3, C3, P4, Nucleo, Discovery).

### Lo que ese repaso debería decidir

1. **Regrabar C3, S3 y Metro con la imagen final** — no por necesidad (sus números están fijados por
   `test-mem` y no cambian) sino para que «lo que lleva la placa» sea «lo que hay en el repo» en las
   cuatro que ya deciden con el planificador. La Metro se graba por el wire (`BOOTSEL`); el C3 y el
   S3 con `idf.py flash` (ojo al DTR/RTS de cada uno, está en `tools/medir_margen.ps1`).
2. ✅ El `/app/Core.mod` del P4 ya está fuera (el `log` del C3 ya está a 0). Y **`#466` de raíz**
   (la Metro ya está limpia a mano): que el IDE no suba stdlib a `/lib`, y que el instalador
   refresque `/lib` cuando no coincida con lo embebido. Mientras tanto, cada Run desde el IDE de
   V5 contra una placa de V6 vuelve a ensuciar `/lib`.
3. ✅ **El STM32**: migrado en código (`U6.11`, 2-sep al retomar); falta grabar las dos placas y
   leer su línea `vm:`.

## ⏭️ (anterior) — `U6` tiene diseño y medidas; falta escribir código

La sesión del 31-ago acabó siendo **de análisis y diseño**, y con eso `U6` (la organización de
la memoria) pasa de ficha roja sin empezar a tener censo, hallazgos, forma y las medidas que la
sostienen. Nada de lo de abajo es estimación: todo está medido en placa o leído del código.

### Por dónde empezar, en orden de menos riesgo

1. ✅ **`CHIP_MARGEN_SISTEMA` en el S3 y el C3 — HECHO en código (`U6.7`, 2-sep).** Tres
   constantes por chip (objetivo / margen medido / suelo) y `vm_buffer_init()` mide, comprueba
   las dos restricciones y baja diciéndolo. **Visto en el C3** (2-sep): la línea sale exactamente
   como se predijo —`objetivo 128, techo 136 por el bloque contiguo, margen 17588`— y el reparto
   no se mueve (64/64). **El S3 queda por contrastar** al reflashear; la predicción es 160 KB con
   techo 264 por el bloque contiguo.
2. **🟡 El enumerador de regiones** (`U6.5`/`U6.6`), familia a familia. ✅ **Pico HECHA (`U6.8`,
   2-sep)**: `bpvm_mem` común + `test-mem` (14/14 contra las placas medidas) + la Pico
   enumerando; verificado en la Metro en los dos modos, byte a byte con las líneas de ayer.
   ✅ **S3 y C3 HECHOS (`U6.9`, 2-sep)**: `vm_buffer_init` queda en enumerar (`heap_caps_get_info`)
   y tomar (malloc + escalera); verificado en el C3 **y en el S3** con los números esperados.
   ✅ **P4 HECHO (`U6.10`)**: el margen es **de la región** y va contra el TOTAL, no contra el
   contiguo — lo cazó el P4 con 508 KiB de diferencia; corregido, verificado byte a byte con su
   imagen anterior (28 668 KiB @0x48000a7c) y su caso medido en `test_mem` (19/19). **La familia
   ESP32 entera y la Pico deciden ya con la misma función.** **Queda el STM32, el último**: su
   techo lo comprueba el ENLAZADOR y eso es más fuerte que cualquier comprobación en marcha — su
   enumerador devuelve el array estático y el aserto se queda.
3. **🔴 El cosido de dos regiones.** Es lo que toca el GC, y necesita las cuatro comprobaciones
   inventariadas en `V6_IDEAS`. Poner `coser=0|1` en el ENV **desde el primer día**: un mecanismo
   que no se puede apagar no se puede medir.

### Lo que quedó a medias

- **El margen de la Pico y el del STM32 sin MEDIR.** El protocolo existe
  (`bpgenvm-c/tools/medir_margen.ps1`) pero habla el wire del ESP32; para esas dos hay que
  adaptarlo.
- **El AOT del C3.** Hoy se descubrió que su ausencia era un hueco del port, no del silicio: le
  faltaba `esp_mm` en `REQUIRES` y ya compila con el camino del `.mdn` dentro. Falta que el IDE
  compile el `.mdn` con la ISA del C3 y probarlo en placa (`U6.4`).
  ⚠️ Con un aviso: el P4 y el C3 comparten el tag `MDN_ARCH_RISCV` y se separan por la ABI de
  coma flotante, **pero eso no separa la extensión de atómicos**, que el P4 tiene y el C3 no.
- **`U6.F1` aparcada** a petición de Eduardo: con PSRAM, qué buffers NO-BP agrandar con la SRAM
  ociosa. Dentro está el método, que importa más que la lista.

### Riesgos que acechan

- 🔴 **Las cuatro comprobaciones del hueco.** Si se implementa el cosido sin ellas, el fallo no
  es un error: es la VM escribiendo o ejecutando memoria del IDF. Están listadas en `V6_IDEAS`.
- 🟡 **El S3 corre una imagen del 30-ago.** Su margen se midió con ella; si se reflashea conviene
  repetir la medida, que ahora cuesta un minuto.
- ✅ **Imagen final (`U6.10`, regla unificada) en P4, Metro, Pico 2 y C3** (2-sep, todas
  verificadas en placa). El **S3** lleva la `U6.10` previa a la regla unificada (mismos números,
  fixture); regrabar cuando se conecte. El S3 subió a 346 064 B libres / 278 528 contiguos con la imagen nueva (+7,7 KB:
  lo que liberó la unificación); si se remide su margen, añadir el caso a `test_mem`, no
  sustituir el del 30-ago.
- ℹ️ **Grabar la Pico/Metro sin dedo**: `BOOTSEL` por el wire la abre como disco `RP2350` y se
  copia el UF2 (`bpgenvm-c/pico/build/bpvm_pico.uf2`). El `RESET` por el wire corta el puerto
  antes de contestar — la excepción al leer la respuesta es normal.

### 🧹 Pendientes manuales

- El **C3** se quedó con `log=1` en su ENV (se puso para medir). Devolverlo a `0`.
- ✅ **El S3 ya está limpio** (2-sep, por el wire): fuera `/app/Core.mod` y el `/lib/Pico.mod` de
  V5; el arranque reinstala el `Pico.mod` de la imagen (⚠️ corregido 2-sep: el ESP32 NO tiene el
  aviso de `#422` — se perdió en `#446`; ver `#466`).
- ✅ El `/app/Core.mod` del resto de placas: la Metro y la Pico 2 no lo tenían; el del P4, fuera
  el 2-sep al retomar.

### `U6` — la organización de la memoria: censo, medidas y forma (sesión de análisis)

Sesión sin apenas código y con mucho medido. `U6` estaba en rojo desde el 28-ago, abierta con una
pregunta de Eduardo (*«¿es por micro o por familia? ¿está previsto unificarlo?»*), y sale de hoy
con censo, cuatro hallazgos, tres medidas en placa y una forma escrita.

**El censo (`U6.0`)** — los cinco puertos **y los dos entornos de PC**, que estaban fuera del
recuento. Cuatro cosas:

1. «Cuánto» eran **dos filosofías mezcladas sin decirlo**: calcular (Pico, P4) contra una
   constante a mano (STM32, S3, C3).
2. El **reparto** estaba unificado en las placas y **no** en el host ni en el simulador.
3. El **techo** tiene tres regímenes, y el del ESP32 sólo se conoce preguntando — y es el
   **bloque contiguo**, no el total.
4. El **margen** ⚠️ *(corregido el mismo día: dije que sólo la Pico lo nombraba y el STM32 también
   lo hace, en el `.ld`, donde no miré)*.

**Lo cerrado**

- `U6.1` — el **simulador y el host** reparten como las placas y leen `stack=N`. Importa desde
  `U3.24`: el sim es el arnés del REPL común, y un doble más amable que el original no caza nada.
- `U6.3` — **la SRAM ociosa no compensa**, medido en la Metro con `psram=0/1`: la PSRAM cuesta
  entre 4,3 % y 7,0 % en tres cargas distintas, a cambio de 17× más memoria. Y el porqué es lo
  interesante: **el intérprete entierra la latencia de memoria** (la misma dispersión cuesta 15 %
  en el host y 5 % en la placa). De ahí salió `BenchMem.bp`, con su control.
- `U6.4` — **el margen del S3 estaba mal**: 26 564 B, no 86 256. El viejo se midió con una carga
  que nadie anotó. Ahora hay protocolo escrito (`tools/medir_margen.ps1`) y las dos placas se
  llevan 1,5×, no 4,9×.

**Lo diseñado** — `U6.5` y `U6.6`: tres capas (constantes de la imagen, ENV, enumeración en
marcha) y siete pasos de arranque de los que **sólo el primero es de la familia**. La clave es
`exclusiva`: con ella, «todo lo que quede» y «una constante» dejan de ser rivales y son una
regla con dos clases de memoria.

**Y dos correcciones mías que conviene recordar**, las dos a preguntas de Eduardo:

- *«la C3 es RISC-V, debería soportar AOT»* — y tenía razón. Su ausencia era un hueco del port
  (faltaba `esp_mm` en `REQUIRES`) y yo había escrito en el código que era del silicio.
- *«¿quién reserva esa memoria?»* — nadie: son **37 reservas del IDF** que dejan el espacio libre
  en 7 trozos. Yo había dicho que las regiones eran contiguas —cierto en direcciones— y salté de
  ahí a que el espacio libre lo fuera.

**Y una cosa que se midió y salió mejor de lo esperado**: coser dos regiones **se puede**. Los dos
trozos se piden a la vez, salen en orden de dirección y el hueco entre ellos son **8 024 B**. El
heap del C3 pasaría de 64 KB a ~165 con el margen puesto: **2,6×**.

### `P1.C3.3` — el C3 ejecuta BasicPlus, por su único cable

🧹 **Pendiente manual**: el C3 se quedó con `log=1` en su ENV (se puso para medir la marca
de agua). Devolverlo a `0` cuando ya no haga falta — el log de ejecución llena la región de
8 KB en ~26 colectas del GC (`#423`).

`fib(28) = 317811`, `exit 0`, subido y ejecutado desde el host por el **USB-Serial-JTAG**, sin
adaptador. La placa saluda como `bpvm-esp32c3` y el INFO da sus GPIOs, su ADC y su SRAM.

Lo desatascó un dato de la placa, de Eduardo: *«hay 2 pulsadores; para grabar se pulsan los dos
y se suelta reset. En un arranque normal el USB es NUESTRO.»* Si el USB no hace falta para
grabar, lo ocupa el wire y la consola se va a UART0.

Antes se descartó su otra idea —multiplexar con DTR— **midiendo**: el C3 no tiene USB-OTG y el
periférico USB-Serial-JTAG no expone ninguna línea de control al software (los ~70 campos de sus
registros, ni `dtr` ni `rts` ni `line_state`). Ese par lo consume el hardware para reset y boot.

Cinco cosas rotas por el camino, todas de la misma familia —**heredar del S3 sin preguntar al
silicio**—: la tabla de particiones de 16 MB (el chip tiene 4), los pines del wire en GPIO43/44
(que en el C3 no existen), una declaración duplicada del init del transporte, un banner que
decía «UART0» fuera cual fuera el cable, y la identidad de placa del S3.

⚠️ **Y un diagnóstico mío que era falso.** Leí `throw ... No space in heap` en el log y concluí
que el heap se agotaba; llegué a cambiar por eso el tamaño del bloque de la VM. Era la
**prefabricación** del OOM de `#430`, que no puede hacerse en un módulo que no importa `Core` —
un no-evento que sonaba idéntico a un fallo. Arreglado el aviso para que distinga las dos cosas.
El 128 KB se queda, pero por su motivo real: dobla el heap (32 → 64 KB) sin tocar los hilos y
sobrando 131 KB de margen.

💡 Y de ahí salió una idea de Eduardo que quedó anotada en `V6_IDEAS`: **coser las dos regiones
de RAM en un solo espacio de direcciones**, marcando el hueco como bloque ocupado igual que se
hace con el de SQLite. Las regiones del C3 resultaron ser **contiguas** (lo parte ESP-IDF por
capacidades, no por direcciones), así que valdría ~250 KB en vez de 136. Falta medir si los dos
bloques caen pegados.


### `P1.C3.2` — la familia ESP32 ya tiene un común de verdad

Pregunta de Eduardo antes de empezar: *«¿no hace falta el FS y la gestión de la memoria
también?»*. Medido, la respuesta es distinta para cada uno:

- **El FS: nada que hacer.** `fs_lfs_esp32.c` busca la partición **por nombre** (`bpdata`) —
  cero código por chip. Lo único particular es `partitions.csv`, y el C3 ya tiene el suyo.
- **La memoria: sí, pero no es un problema de reestructurar.** El *código* ya era común; lo
  particular es **un número**, y tiene que ser constante de compilación porque
  `vm_buffer_init()` corre **antes** que `board_mgr_esp32_boot()`, o sea antes del ENV.

El censo cambió el tamaño del trabajo: **9 de los 10 `.c` de `esp32/main/` los usan los tres
chips** — ya era el común sin decirlo. Y mover los proyectos costaba **193 rutas relativas**,
cuyo fallo típico no es «no compila» sino «resuelve a otra cosa».

Así que el mismo reparto con otra forma: las **fuentes** salen a `esp32/common/` y cada
proyecto conserva su componente `main/` (que el IDF exige) con un `chip_cfg.h` dentro. Ni una
profundidad cambia — unas 30 líneas en tres `CMakeLists`.

**Lo que de verdad se arregló**: el `main.c` del C3 era una copia de 176 líneas que se
diferenciaba en **dos números y un nombre**, y los arreglos no viajaban — `#464` y `#465` se
hicieron en uno y no en el otro, esta misma semana. Ahora hay uno.

Verificado en el artefacto: el mismo `main.c` produce `ESP32-S3` en una imagen y `ESP32-C3` en
la otra (`strings` de los `.bin`), y las tres conservan el tamaño exacto de antes.

⏭️ Queda `P1.C3.3`: **medir** el heap del C3 en placa. Los 96 KB de ensayo están puestos para
enlazar, no medidos, y el fichero lo dice en mayúsculas.


### `U3.25` — y el arnés nuevo cazó dos verbos que mentían

Al ampliar `sim_smoke.py` con lo que el común tiene y el arnés no miraba, dos rojos, y de
verdad: `MKDIR` y `RMDIR` eran **stubs que contestaban OK sin tocar nada**. El común creaba
directorios en cada PUT (`crear_dirs_padre`, `#455`) y decía «hecho, nada» cuando se le pedía
uno explícitamente, dos funciones más abajo.

Y el sim **tenía un `MKDIR` de verdad**: migrarlo sin mirar fue unificar hacia abajo — la
regresión de `#455` otra vez. La diferencia con la vez anterior es el tiempo: **diez minutos
en el host**, no una placa tres días después.

Arreglado. `MKDIR` crea o da error con nombre; `RMDIR` distingue ahora «no existe»
(`NOT_FOUND`) de «no está vacío» (`NOT_EMPTY`), que el stub juntaba en un OK.

⏭️ **Falta verlo en placa**: en el host el backend es littlefs sobre una imagen y en el micro
es flash de verdad. Gesto: `MKDIR /app/x`, `LIST_DIR /app`, `RMDIR /app/x`.


### `U3.24` — el simulador al REPL común: el arnés que faltaba

Cerrado `U3` (las comunicaciones), quedaba una deuda que el propio bug del `LIST` había dejado
por escrito: **`bpvm-sim` era el quinto consumidor del REPL y no había migrado**. 21 verbos
propios, ejercitados por el wire desde `sim_smoke.py`… contra su propio código.

Migrado. El sim baja a **4 verbos propios** (`RUN`, `KILL`, `RESET` y la gestión de placa) y
`tools/bpvm_sim.c` pierde 124 líneas. De regalo gana `LIST_DIR` y `RMDIR`, que no tenía.

**Y con control, que es lo que lo hace valer:** se volvió a meter el bug de ayer (el `memset`)
y `make sim-smoke` lo cazó — `FAIL` en las dos comprobaciones de `LIST` con contenido. Doce
segundos en el host contra el viaje a la P4 de ayer.

Detalle fino que conviene recordar: la comprobación *«LIST tras formatear → vacío»* **pasa con
el bug vivo**. Cubre el verbo pero no el camino. La que caza es la que tiene ficheros dentro.

Verificado: `sim-smoke` 25/25, `boardsim-smoke` verde, paridad 38 PASS / 0 FAIL / 0 SKIP.


## ⏭️ AL RETOMAR (31-ago, cierre 2) — dos cosas del IDE

### ✅ `#463` — `Core.mod` se subía a `/app` y creaba un override sin querer

**Lo vio Eduardo mirando el árbol de la placa**, sin que fallara nada: `Core.mod` aparecía en
`/app` **y** en `/lib` con el mismo tamaño. La lista `EMBEDDED_CORE_MODS` del IDE tenía **13**
nombres donde el firmware embebe **14**; faltaba `Core`. Arreglado, fat-jar rehecho y
verificado en la Discovery (`Core.mod → /lib`, CRC idéntico, salta el PUT).

📌 **Dos correcciones de Eduardo que cambiaron el diagnóstico**: que `/app` gane a `/lib` **está
bien y es a propósito** —*«si tienes un módulo más moderno lo puedes probar; de la otra forma no
se podría sin borrarlo de /lib»*—, así que el fallo no era la precedencia sino **crear el
override sin pedirlo**. Un override deliberado se recuerda; uno accidental te espera.

🧹 **Pendiente manual**: borrar el `/app/Core.mod` que dejaron las ejecuciones anteriores. El
arreglo evita crear el próximo, no limpia el que hay — y mientras esté, sigue ganando.

⚠️ **Lo de fondo, abierto**: esa lista es un **gemelo escrito a mano** de lo que embebe el
firmware, y se habían separado por uno. Lo robusto es que **lo diga el dispositivo** (el `LIST`
de `/lib` ya existe y el IDE ya compara por CRC). Mientras siga siendo una constante en Java,
volverá a desincronizarse.

### ⏸️ EN SUSPENSO — «faltan módulos» al ejecutar desde `run`

**Síntoma de Eduardo**: ejecutar desde el IDE (que copia los módulos) y a continuación desde el
`run` de la consola (que no copia) → *«me dice que faltan módulos»*. **Decisión: se deja en
suspenso a ver si vuelve a ocurrir.**

**Lo que se averiguó por el camino y sirve si reaparece** — hay una asimetría real:

| quién lanza | `path` que manda | basedir que se deduce | dónde busca las DEPENDENCIAS |
|---|---|---|---|
| IDE, con proyecto | `/app/<proj>/X.mod` | `/app/<proj>` | el proyecto → `/app` → `/lib` → `/sys` |
| IDE, sin proyecto | `/app/X.mod` | *(plano)* | `/app` → `/lib` → `/sys` |
| consola `run X` | `consoleCwd + "/X.mod"` | según dónde esté el `cd` | lo que salga de ahí |

📐 La regla está en `bpvm_entry_resolve` (`bpvm.c:779`) y el basedir en
`bpvm_fs_set_basedir_from_module` (`fs_facade.c:98`), que **sólo lo activa si la ruta empieza
por `/app/<proj>/`**. O sea: **el mismo programa puede buscar sus dependencias en sitios
distintos según por dónde lo arranques**.

🔴 **Pero NO está confirmado que sea la causa de lo que vio Eduardo.** Faltan dos datos que sólo
da la reproducción: **el texto exacto del error** (*«faltan módulos»* y *«no encuentro el
módulo»* son mensajes distintos y apuntan a sitios distintos) y **dónde estaba el `cd`**. Sin
eso es una teoría que encaja, no una causa. *(Y cabe que fuera parte de lo de `#463` y ya esté
resuelto.)*

---

## ⏭️ AL RETOMAR (31-ago, cierre) — **la matriz de placas, al día**

| familia | estado | qué ha corrido con la imagen de hoy |
|---|---|---|
| **ESP32-P4** | ✅ **probada** | `JsonDemo` · `trytest` · `GuiEvSpike` · `stop` |
| **STM32 Discovery** | ✅ **probada** | `JsonDemo` · `GuiColorDemo` · `GuiEvSpike` · `GuiEvLat` ×3 · `trytest` · `stop` ×5 |
| **Pico / Metro** | 🔴 **la stdlib nueva SIN ESTRENAR** | — |
| **ESP32-S3** | 🔴 **la stdlib nueva SIN ESTRENAR** | — |
| **STM32 Nucleo** | 🔴 **la stdlib nueva SIN ESTRENAR** | — |

El gesto que cierra cada una son dos programas con **salida conocida e idéntica en las tres
que ya pasaron**, o sea que valen de oráculo:

- **`trytest.bp`** → `atrapado: n era negativo` + los otros dos mensajes. Es **el camino más
  tocado de estos dos días**: `#459` cambió cómo se enlaza la variable de un `catch` sin tipo,
  `Exception` ganó `toString()`, y de los 56 ficheros que ganaron `import Core`, **23 fue por
  `RuntimeError`**.
- **`JsonDemo`** → la stdlib nueva corriendo (`Core.mod` regenerado).

⚠️ **Y una precisión sobre el `/lib`, para no dar de más por probado**: el IDE refresca por CRC
**sólo los módulos que cada programa pide**. La Discovery tiene al día `Core`, `Json`, `Gui` y
`Pico` — **no los 27**. Los que ningún programa haya pedido pueden seguir siendo los viejos, y
eso morderá el día que se use uno. Para dejar una placa al día ENTERA: **formatear el FS** y que
la imagen reinstale.

---

## ⏭️ AL RETOMAR (31-ago, tarde) — **la latencia de los eventos del GUI, medida y partida en dos**

### 🔬 Dónde retomarlo: `#462`, y está listo para atacarse

De una sensación de Eduardo —*«en las ESP32, cuando se ejecuta un programa, se queda todo
bloqueado; los eventos pasan pero mucho más lentos»*— salieron **cuatro cosas distintas**:

| | qué es | estado |
|---|---|---|
| 1 | el planificador **nunca cedía el turno al SO** — el gancho `bpvm_platform_thread_yield()` existía en las 4 plataformas y **no lo llamaba nadie** | ✅ **arreglado**; el `stop` del P4 volvió a funcionar |
| 2 | el **quantum se mide en OPCODES** (1024) y `Gui.run()` gasta uno bombeando LVGL entero | 🟢 **medido**: `1,21 ms × vueltas`. Sin arreglar |
| 3 | un **suelo de 48 ms** independiente del quantum | 🔴 **sin investigar**. Sospechoso: `LV_DEF_REFR_PERIOD = 33 ms` (`include/lv_conf.h:82`) |
| 4 | el `exit` de un `stop` es **una CARRERA**: dice `OK` o `KILLED` según dónde caiga el KILL | 🐛 hallazgo colateral, sin arreglar |

**El modelo, validado con tres puntos en la STM32 Discovery** (`samples/GuiEvLat.bp`, escrito
para esto):

```
lat = 1,21 ms/vuelta × vueltas + 48 ms de suelo      (error máx. 5 ms en un rango de 32×)

quantum 1024 → 403 ms     quantum 128 → 87 ms     quantum 32 → 63 ms
```

📌 **Se predijo antes de medir**: para `quantum=128` el modelo decía ~95 ms y salieron **87**.
Con dos puntos un ajuste encaja por construcción; con la predicción cumplida, ya no.

⏭️ **Lo que queda, y son decisiones de Eduardo, no trabajo a medias:**
- **El quantum por TIEMPO**, como ya hace miVM (`bpvm_internal.h:280` lo tenía anotado como
  diferencia entre VMs, sin ver que aquí valía 400 ms). Toca el planificador → con placa.
- **El suelo de 48 ms**: averiguar QUÉ es antes de tocar. Bajar el refresco de LVGL cuesta CPU
  y sería adivinar.
- **El `exit` inestable**: pequeño, aislado, y de los silenciosos.

🔧 **Herramienta nueva y reutilizable**: `quantum=N` en el ENV, en las cuatro familias (mismo
patrón que `stack=N`). ⚠️ Se lee **una sola vez al arrancar** → hay que **resetear** tras
cambiarla. Y **no es configuración para dejar puesta**: con 32 los eventos van finos y el resto
paga más cambios de contexto.

⚠️ **Si una placa se comporta rara, mira primero si tiene `quantum` puesto en el ENV.**

### 🧭 Lo que enseñó la tarde, y es de método

**La medida tumbó DOS diagnósticos míos seguidos.** Primero *«es la prioridad 5 del P4»* — falso:
la Discovery, sin RTOS y sin prioridades, tiene la misma latencia. Después *«la latencia bajará
proporcionalmente al quantum»* — falso: bajó 6,3× de 32, porque había un suelo.

📌 **Y la clave la dio una frase de Eduardo que resultó ser un artefacto**: la Discovery *«funciona
muy bien»*. No funcionaba bien — **tardaba 400 ms igual**. Lo que pasaba es que los clics iban a
800-2200 ms y nunca se acumulaban. **Sin cronómetro, dos diagnósticos se apoyaron en una
impresión que dependía del ritmo del dedo.** De ahí salió `GuiEvLat.bp`: el spike sólo daba el
ORDEN de las líneas, y con el orden no se distingue «drena lento» de «cliqué despacio».

---

## ⏭️ AL RETOMAR (31-ago) — `#441` cerrada y **LAS CINCO IMÁGENES, del mismo árbol**

### Lo primero: el estado de los artefactos, comprobado por fecha (no de memoria)

| eslabón | estado |
|---|---|
| `bpstdlib/*.mod` | **27/27 al día**, ninguno más viejo que su `.bp`, **todos MOD7** |
| blobs embebidos (`core_mod.c`, `esp32_mods.c`, `stm32_mods.c`) | ✅ posteriores al `.mod` más nuevo |
| **las CINCO imágenes** | ✅ posteriores al último cambio de núcleo (`interp.c`, `#441`) |
| fat-jar del IDE | ✅ posterior al frontend |

```
bpvm_pico.uf2            31-ago 07:37     600.064 B
bpvm_esp32_merged.bin    31-ago 07:38     505.760 B
bpvm_esp32p4_merged.bin  31-ago 07:39   1.361.200 B
Nucleo_u575b.elf         31-ago 07:39   3.701.456 B
Discovery_u5g9j.bin      31-ago 08:05     889.172 B
```

🔴 **El Discovery llevaba sin construirse desde el 29-ago** — se le habían pasado los tres
bugs del compilador, la stdlib nueva y `#441`. Construido y **con la trampa de
`PUBLICAR.md` confirmada en vivo**: el `cleanBuild` headless regenera el `.elf` **pero no
el `.bin`**, y el que había al lado era **del 5 de agosto**. Veintiséis días. Regenerado
con `arm-none-eabi-objcopy`.

⚠️ **`dist/firmware/` sigue siendo del 20-21 de agosto, y ESO ESTÁ BIEN**: es la V5
publicada. No se toca hasta publicar — y hoy mismo ha servido de **oráculo** para demostrar
que `#459` no lo habíamos roto nosotros. Sobrescribirla habría destruido esa capacidad.

### Lo cerrado el 31-ago

**`#441`** — los seis opcodes de globales estrechas (`GET/SET_GLOBAL_I8/U8/I16/U16`), ya en
la VM-C. El arreglo era trivial; lo que costó fue el test, **y resultó que ya existía**:
`miVM/MainNarrow.java` los ejercita los seis fabricando un `.mod` a mano con `ModWriter`, y
nadie lo había pasado nunca por la VM-C. Con control en rojo (`opcode 0x40 desconocido`) y
después **19 valores byte a byte iguales**.

### Decisiones de Eduardo

- **`#452` APLAZADA al script de test.** *«Con los comandos tenemos 3 casos de uso: el
  usuario, el IDE internamente, y el futuro script.»* El tercero no existe todavía, así que
  juzgar los verbos ahora sería decidir con un consumidor de menos a la vista. Se repasarán
  **los 29**, no cinco. 📌 De la tabla que queda hecha, lo más útil: **`FORMAT` hace falta
  HOY** — tras flashear hay que formatear el FS y no hay forma desde el IDE.
- **`#461` ABIERTA**: *«los path no deberían reservar espacios fijos… la longitud ha de ser
  variable»*. Medido: los paths reales miden **13,3 de media**, y el listado del ESP32
  reserva **7,5 KB de `.bss`** para ~1,5 KB de contenido. El remedio es el pool de `#440`,
  aplicado hoy mismo en la tabla de símbolos. Empalma con `LIST`, el último verbo de `U3`.
- **`#456`** replanteada: FatFs y littlefs admiten **255**; el 64 es un tope NUESTRO.

⏭️ **Lo primero al retomar sigue siendo lo mismo**: Pico, S3 y STM32 **con la stdlib nueva
sin estrenar** — y ahora las cinco imágenes están listas y del mismo árbol. Formatear el FS
antes, que hoy cambiaron los 27 módulos.

## ⏭️ AL RETOMAR (30-ago, tarde-noche) — tres bugs del COMPILADOR, y uno venía publicado en V5

### Lo que hay que saber antes de tocar nada

🔴 **Al flashear cualquier placa, FORMATEA EL FS.** Hoy han cambiado **los 27 módulos de
la stdlib** (todos pasan a `.mod` v7 y `Core` además gana un método). El ESP32 y el STM32
instalan un módulo embebido **sólo si no existe** (`esp32_mods.c:4862`), así que una placa
con `/lib` de ayer se queda con el viejo **y no lo dice** — la Pico sí lo canta en el log
(`main.c:1330`). Es la ficha *«un módulo rancio sobrevive y NADIE lo dice»*, latente hasta
hoy porque ninguna imagen había traído la stdlib cambiada.
*(El camino del IDE lo esquiva solo: compara por CRC y sube a `/app`, que tiene precedencia.)*

### Lo cerrado

| ficha | qué |
|---|---|
| **`#457`** | el compilador escribía `.mod` **v7** y su propio lector de interfaces sólo aceptaba **v6** → **la stdlib no se podía regenerar**. Un `return null` mudo, con el error saliendo en otro fichero |
| **`#458`** | **norma nueva**: si usas un tipo de `Core`, importas `Core`. Y la pasada de interfaz **deja de tirar miembros en silencio** |
| **`#459`** | un `catch` sin tipo entregaba un valor roto que **volcaba la tabla de símbolos** por pantalla. **Publicado en V5** |
| `U3.21` + **`#455`** | el grupo `PUT` del ESP32 al común; el `mkdir` de los directorios padre, recuperado antes de que la migración borrara la última copia |
| `Math` | entran `clamp` `wrap` `hypot` `remap` (`atan2` ya estaba, y la ficha la seguía pidiendo) |
| **`#443`** | cerrada **por decisión**: la hipótesis está contestada y el cierre de V6 trae su propia tanda |

**Abiertas nuevas**: `#456` (un `path` >63 chars se trunca en silencio y la operación dice OK).

### 🔑 Las tres son la MISMA enfermedad

Una suposición que dejó de ser cierta y a la que nadie volvió:

- el `import Core` implícito, inyectado **en una de las dos pasadas**;
- el lector de interfaces, congelado en `MOD6` cuando el escritor pasó a `MOD7`;
- el emisor tratando una excepción como string porque *«en BP los throws son típicamente
  strings»* — cierto **dos versiones atrás**, y el propio comentario lo dejaba escrito.

📌 **Al subir un formato o cambiar una regla, censar los LECTORES.** Con `.mod` eran tres,
no dos: las dos VMs lo aceptaban y el que se quedó atrás fue el del compilador, que nadie
recuerda que existe porque casi siempre trabaja en memoria.

### ⚠️ Y el arnés estuvo VERDE con un bug de memoria dentro

`#459` volcaba el heap por pantalla y `compat.sh` daba **38 PASS**: las dos VMs producían
**la misma basura, byte a byte**. *Un oráculo que sólo compara dos implementaciones no
puede ver un fallo que ambas comparten.* Es la trampa del falso-PAR en su forma pura, y
refuerza `#444`: hace falta una red que compare contra lo ESPERADO, no sólo entre VMs.

### Estado de verificación

| familia | qué se ha visto |
|---|---|
| **ESP32-P4** | **la mejor**: `JsonDemo` (con `Core.mod` regenerado corriendo en RISC-V) y `trytest.bp` con la salida idéntica al host |
| **Pico · S3 · STM32** | 🔴 **la stdlib nueva SIN ESTRENAR**. Compilan, pero ninguna la ha ejecutado |

⏭️ **Primer gesto al retomar**: `trytest.bp` y `JsonDemo` en la Pico y en el S3 —los dos
ya tienen salida conocida, o sea que sirven de oráculo— **después de formatear el FS**.

### Lo que queda del tema «lenguaje»

- 🟡 **LSP entre interfaces de módulo** (`appv1lsp`/`appv2`, en `samples/pendientes/`).
  ⚠️ Y la explicación escrita en la ficha **está en duda**: dice que *«falla el dato»*
  porque una `module interface` pura no genera `.mod`, pero la traza del 30-ago imprime
  `extends=com.example.LogApi` — el dato está. Primer gesto: mirar si `implSatisfies`
  recibe ese `extends` o una copia sin él, en vez de dar por buena la explicación.
- 🟡 **`factorial` en f64** y **unificar `sign`/`signF`**: bloqueados por lo mismo, **no se
  puede sobrecargar una intrínseca** (`Intrinsics.REGISTRY` es un `Map` por nombre pelado
  y el call-site compone la clave con `fs.name`, no con el mangleado de `H5.a`).
- 🟡 **Mudar `Exception` fuera de `Core`**, como está `Object` — idea de Eduardo. Ahorraría
  el `import` a 21 de 56 ficheros. **No se hizo a propósito**: mete identidad de clases
  entre módulos en medio de un arreglo de bug, y un `catch` que deja de atrapar falla en
  silencio. Va en su propio paso.

### Lo que enseñó la tarde, y no es del código

**El «no sé» de Eduardo ante una salida rara.** Propuse `trytest.bp` como trámite para
cerrar la verificación; él miró la salida, dijo *«no sé»*, y eso destapó `#459`. Una salida
que no se entiende merece pararse a mirarla **aunque el `exit` sea 0 y el arnés esté verde**.

**Y un método que funcionó**: para decidir *«¿lo he roto yo hoy?»* se usó la
**distribución publicada** (`dist/BasicPlus-5.0-win/`) como oráculo — su compilador, su VM
y su stdlib, todo desde el mismo fat-jar. Contestó en un minuto lo que rebuildeando el
árbol viejo habría costado varios, y descartó la sospecha razonable (y falsa) de que la
tanda del día tuviera la culpa.

## ⏭️ AL RETOMAR (30-ago, tarde) — `U3.21` cerrado y **el P4 al día**; la deuda es ahora del STM32

### La matriz, al cierre de la tarde

| familia | última verificación | qué le falta por ver |
|---|---|---|
| **ESP32-P4** | **30-ago (tarde)**, en placa | al día — ver abajo lo que cubrió |
| **ESP32-S3** | **30-ago**, gesto a gesto | `U3.21` sólo por herencia (comparte `repl_esp32.c` con el P4, que sí se probó) |
| **Pico / Metro** | 29-ago (batería 48/48) | `#453` · el `SAVE` de `U3.17` · `#455` |
| **STM32 Nucleo** | 27-ago (`U3.6`) | `#449` `#450` `#451` `#453` `#455` · `U3.17` · los 6 gestos de la deuda vieja |
| **STM32 Discovery** | ninguna esta serie | lo del Nucleo, y además es la de PANTALLA |

🟢 **El P4 pasó de ser la deuda gorda a estar al día en una tanda.** Y lo primero que
se resolvió no costó ningún gesto: **el `server_name` lo demuestra el propio `INFO`**.
`Micro : esp32p4` sale de `bid->board_name`, y `board_name` y `server_name` son campos
**contiguos de la misma struct**, rellenados en el mismo inicializador de
`p4_board_id.c`; `p4_install_board_id()` corre en `main.c:322` (y `:370` en el camino
TCP) **antes** de `repl_esp32_run()`, que copia el nombre en la línea 1201 antes de
registrar la cintura. No hay ventana. La pregunta se contestó **con lo que ya había**.

Lo que la tanda de Eduardo cubrió, gesto a gesto:

| gesto | qué prueba |
|---|---|
| subir y bajar `Fichas.md` (~300 KB) | **`U3.21`**: >8 KB ⇒ `PUT_BEGIN` + ~37×`PUT_DATA` + `PUT_END`, los tres por el común. Y **`#453`/`#454`** en el mismo viaje: en el S3 un fichero de 120 KB daba timeout antes del arreglo |
| grabar `Stdlib.pack` | 🎯 **el control que hacía falta**: `PACK_BURN_*` es el camino que **sigue pre-leyendo el bulk** en `handle_request`. Si `lo_lee_el_comun` se hubiera colado a `1` de más, esto se corrompe — y en binario no da error, corrompe. Verde ⇒ la bandera separa bien los dos caminos |
| `fib(28)` interp 4410 ms / **AOT 32 ms** | el `.mdn` RISC-V vivo (137×) y `RUN` + resolución de módulo |
| `/lib/Pico.mod ya en FS (contenido idéntico), salto PUT` | el `STAT`+CRC bajo demanda (`#398`) migrado, decidiendo bien |
| el arranque (`heap 27.5 MB + stack 512 KB`) | **`#450`** en su rama por defecto: sin `stack` en el ENV, el reparto de siempre |

⚠️ **Lo que esa tanda NO prueba, y conviene no apuntárselo:** `#455` sólo quedó
verificado en que **no rompe el caso normal** — el `PUT` fue a `/app`, que existe desde
el montaje, así que el `mkdir` nuevo fue un no-op. La capacidad que devuelve (subir a
una carpeta que no existe) **sigue sin ejercitarse**, y no hay test de host que la cubra
(el simulador trae su propio `handle_put`). Es el hueco de `#444`.

⏭️ **Lo que queda de `U3`**: sólo `LIST`. Sigue parado por lo mismo — el ESP32 imprime
un desglose por raíz (`ls: 25 ent en 160 ms | app:11/50ms lib:14/64ms`) que el común no
tiene, y migrarlo tal cual sería **unificar hacia abajo**, que es exactamente el error
que `#455` acaba de demostrar que se comete solo. El camino bueno es subir el desglose
al común, y entonces la Pico y el STM32 lo GANAN. Diseño: el común ya recorre con una
cola desde `/`, así que cronometrar cada entrada de profundidad 1 da el mismo desglose.

## ⏭️ AL RETOMAR (30-ago) — `U3` casi cerrado en ESP32, y **el P4 es la deuda gorda**

### Lo primero: LA MATRIZ DE VERIFICACIÓN

Pregunta de Eduardo al cerrar: *«¿cómo están las otras familias?»*. Medido con
`git log` sobre el código compartido desde la última prueba de cada una:

| familia | última verificación | qué le falta por ver |
|---|---|---|
| **ESP32-S3** | **30-ago**, gesto a gesto | al día |
| **Pico / Metro** | 29-ago (batería 48/48) | `#453` · el `SAVE` de `U3.17` |
| **STM32 Nucleo** | 27-ago (`U3.6`) | `#449` `#450` `#451` `#453` · `U3.17` · los 6 gestos de la deuda vieja |
| **STM32 Discovery** | ninguna esta serie | lo del Nucleo, y además es la de PANTALLA |
| **ESP32-P4** | **25-ago** (`U2` paso 2) | **`U3` ENTERO (12 verbos)** · `server_name` · `#449` `#450` `#451` `#453` |

🔴 **El P4 es el peor caso y conviene entender por qué.** Comparte
`repl_esp32.c` con el S3, así que ha recibido **toda** la migración del 30-ago sin
flashearse. Y —esto es lo importante— **lo único que el verde del S3 NO puede
exonerar es justo lo suyo**: el `server_name`, que es por placa (`bpvm-esp32p4`
contra `bpvm-esp32`). Fue el bug que puse en `U3.13` y quité en `U3.19`, y **sólo
se manifiesta en el P4**.

📌 La regla que sale de ahí, y sirve para toda la serie U: *el código común
verificado en una familia dice algo de las demás; **lo que sale de la cintura,
no**.* Por eso la matriz no se puede rellenar por inferencia.

⏭️ **Orden propuesto y el gesto que prueba cada cosa:**

1. **P4** — conectar el IDE y **mirar el nombre del servidor** (cubre lo único no
   inferible), luego `INFO`, `ls`, subir y bajar un fichero.
2. **Pico** — abrir un fichero grande (prueba `#453`) y `save` desde la consola.
3. **STM32** — los seis gestos de `H13_PRUEBAS_V5_REPASO.md` + abrir un fichero
   grande.

### Lo que se hizo el 30-ago

**`U3` en el ESP32, de 23 verbos propios a 6.** `repl_esp32.c` 1360 → 1242 líneas.
Migrados y **verificados en placa**: `DEL` `STAT` `MKDIR` `GET` `PING` `TIME`
`LOG_DUMP` `LOG_CLEAR` `SAVE` `DF` `INFO` `HELLO`.

Quedan dos cosas, y las dos son decisión de Eduardo más que trabajo:

- 🟡 **`LIST` está PARADO a propósito.** Migrarlo tal cual perdería el desglose por
  raíz que el ESP32 sí imprime (`ls: 25 ent en 160 ms | app:11/50ms lib:14/64ms`) y
  el común no — sería unificar HACIA ABAJO. Lo correcto es subir ese desglose al
  común, y entonces la Pico y el STM32 lo GANAN. Además el común recorre
  directorios (`bpvm_fs_list` con cola desde `/`) y el ESP32 lista plano: ninguno
  de los dos es «el bueno» entero.
- ⏭️ **El grupo `PUT`**, con el terreno ya preparado: `U3.20` subió el `type` por
  encima de la pre-lectura del bulk y dejó la bandera `lo_lee_el_comun` en 0.

**Cuatro fichas nuevas**, tres de ellas encontradas persiguiendo otra cosa:
`#452` (cinco verbos que ningún cliente manda), `#453` (el `GET` abría el fichero
472 veces para 120 KB), `#454` (el timeout medía tiempo total en vez de
inactividad) y `#446` cerrada con `make check-nogui`.

### Lo que conviene llevarse

🔁 **Tres veces el mismo patrón en una sesión: el arreglo existe en un camino y
falta en su gemelo.** `crc32` arreglado y el `GET` no (`#453`); el corte del CRC en
la Pico y no en el ESP32 (`#424`); el troceado en `PUT` y no en `GET` (`#454`).
Ninguno se buscó — los tres salieron de perseguir otra cosa. Merece la pena
buscarlos a propósito: `grep` de la primitiva que se deja de usar.

⚠️ **Y un diagnóstico mío que era falso.** Calculé que 120 KB a 115200 baud son
10,5 s contra un plazo de 10,0 y concluí «no cabe por el cable». La aritmética
estaba bien y la conclusión mal: lo tumbó que Eduardo observara que **un pack de
~580 KB sí se graba**, tardando ~50 s. No miré lo que ya funcionaba antes de
declarar algo imposible.

## ⏭️ AL RETOMAR (29-ago, cierre) — **se sigue por `U3` con el ESP32**

**Decisión de Eduardo al cerrar**: *«cuando retomemos continuamos justo por U3 y vamos
cerrando cosas»*. Con eso **termina la etapa de bugs de V5** y se vuelve a V6.

### Por dónde exactamente

`U3` (el REPL) tiene **STM32 y Pico completos** y verificados en placa: 26 de los 29 verbos
al común (`RUN`/`KILL`/`RESET` se quedan por diseño). **Falta la tercera familia: el
ESP32** — el S3 y la P4 conservan su `repl_esp32.c` de 1344 líneas.

📌 **Y hay un motivo nuevo para que sea lo siguiente.** Eduardo fijó ese día la prioridad:
*«a mí la S3 me importa relativamente, es el paso; el futuro son la P4, la C6 y la S31»*.
Las tres son ESP32 y **las tres comparten `esp32/main/*`**: cada verbo que siga en la copia
privada es un verbo que habrá que replicar tres veces. Es el argumento de V6, aplicado.

⏭️ Y de paso, en el mismo arranque, **saldar la deuda del STM32 desde `U3.6`** (seis
gestos, cinco minutos, en `H13_PRUEBAS_V5_REPASO.md`): lleva desde el 27 esperando.

### El estado de la serie, ya limpio

| | | |
|---|---|---|
| U1 · U2 | — · el transporte | ✅ cerrados |
| **U3** | **el REPL** | 🔵 **STM32 y Pico hechos; falta ESP32** ← *aquí* |
| U4 | la stdlib embebida | 🟡 sin empezar (es un GENERADO: tocar el generador) |
| U5 | la tabla de handles | 🟡 sin fichero propio todavía |
| U6 | la organización de memoria | 🔴 abierta, **sin la urgencia** que tuvo el 29 por la mañana |

Abiertas además: `#446` (host con `GUI=0`, barato y sin placa), `#441` (seis opcodes) y
`#444` (el sistema de pruebas no escala — su diseño ya está escrito en `V7_IDEAS.md`).

### Lo que se cerró en la sesión del 29

**La batería pasó de 40/48 a 48/48.** Cerradas `#440`, `#442`, `#445`, `#447`, `#448`,
`#449`, `#450` y `#451`. La causa de fondo era **una sola**: las tablas de símbolos y de
handles vivían en el margen de `malloc` de la plataforma, separadas del heap, y no se
prestaban nada. Arreglado en dos movimientos —un pool de nombres y la tabla de handles
dentro del bloque de la VM— y verificado en **dos familias** (Pico 2 y S3).

⚠️ **Lo que conviene llevarse, y no es sobre el bug**: de los cinco atascos del día, **cuatro
no eran del proyecto** — tres eran instrumentos de diagnóstico míos mintiendo y uno era una
cifra mía mal calculada. Lo que resolvió las dos veces que hubo atasco de verdad fue
**medir**: contar símbolos, contar handles, `idf.py size-components`, y la tabla de memoria
de Espressif. Nunca razonar. Ver [[contar-los-consumidores-no-leer-el-codigo]] y
[[instrumento-mudo-dudar-de-el]].

## ⏭️ AL RETOMAR (29-ago) — la batería queda en 47/48, y lo único que falta es `U6`

**Lo primero de mañana**: decidir `U6`. Es lo único que bloquea el 48/48, está
diagnosticado con números y no hay nada más que investigar — sólo hacerlo. Detalle
en `#451`.

### Lo que se cerró hoy

`#440` y `#449` (los dos síntomas eran **una sola causa**: la tabla de símbolos
desbordando el margen de `malloc` y escribiéndose sobre el código de un módulo —
los bytes `43 6F 72 65` eran la cadena `"Core…"`). El arreglo: un **pool de
nombres**, 59 KB → 20, y el pico del `realloc` de 99 KB → 28. `JsonDemo` corre en
placa con salida **byte-idéntica a la VM-Java y al host**.

`#450` nuevo y cerrado: **`stack=N` (KB) en el ENV**, petición de Eduardo. El
usuario reparte el bloque entre pilas y montón; ausente = lo de siempre. Entró
donde la regla ya estaba unificada, así que fue el núcleo + una línea por familia.
Verificado en placa: `heap 267+89` → `heap 293+64`.

**La batería, de 40/48 a 47/48.** El único rojo real es `synclisttest` (`#451`).

### Lo que hay que saber para mañana

🔻 **De los tres atascos de hoy, NINGUNO fue un programa roto: los tres fueron
instrumentos que mentían.** El MPU armado antes del último enlace, el MPU
heredando regiones de la ejecución anterior, y el aviso de `#430` gastado por un
`static`. Los tres con la misma forma: **estado que sobrevive entre ejecuciones**.
Cuando algo «falla» y el andamio está puesto, sospechar del andamio primero.

📌 **Y el método que sí funcionó, dos veces: contar.** La pista de `loader.c:224`
para `#440` era excelente —explicaba el síntoma *y* la asimetría Metro/Pico 2— y
era falsa; sobrevivió dos días. Lo que la mató fue instrumentar el registro de
símbolos y **contar** (460, no los ~176 que yo había estimado). Igual con
`synclisttest`: la corrección de Eduardo *«se añaden unos cuantos prints y vemos
dónde está el problema»* dio en 10 minutos lo que yo iba a buscar construyendo
instrumentación del alocador.

### Deuda que sigue viva

- **`U6`** — la tabla de handles fuera del bloque de la VM. Ver `#451`: el objeto
  medio mide 25 B y el reparto asume 64, así que todo programa de objetos pequeños
  agota handles con el heap casi vacío.
- **El STM32 sin verificar desde `U3.6`** (`H13_PRUEBAS_V5_REPASO.md`): seis
  gestos, cinco minutos. Sigue sin hacerse.
- **La batería en las otras familias**: hoy sólo Pico 2.
- **`--smp=2` cuelga en el host** con `synclisttest` (`--smp=1` y el camino normal
  pasan). Bug real del paralelismo, sin ficha todavía; **no** afecta a la Pico,
  cuya imagen no define `BPVM_PICO_SMP_WORKERS`.
- `#446` (host con `GUI=0`) y `#441` (seis opcodes) siguen abiertas.

## ⏭️ AL RETOMAR (28-ago, noche) — la batería de V4 sobre la Pico 2, y lo que destapó

**Lo primero de mañana**: `#440` es el único rojo que queda de los ocho, y está acotado a
un offset concreto con una pista de código. Entrar por `src/loader.c:224`. Todo el detalle
—lo medido y lo **descartado con medida**, para no repetirlo— en `FICHAS.md`.

⚠️ **SON DOS PROBLEMAS Y SE MEZCLAN.** Lo vio Eduardo al cerrar: *«creo que tenemos 2
problemas diferentes y están un poco mezclados»*. La trampa es que **en la Metro
desaparecen los dos** (heap en PSRAM ⇒ la SRAM entera para `malloc`), así que su verde no
distingue entre ellos. Hay un cuadro comparándolos al principio de las fichas.

### Lo que pasó, en orden

Se corrió la batería de V4 completa sobre la Pico 2 (48 samples, decisión de Eduardo:
*«es muy raro que solamente falle JsonDemo»* — y tenía razón). **40 verdes, 8 rojos.**
Seis eran cuelgues mudos, uno `exit 6`, uno `exit 11`.

De ahí salieron cinco fichas. La secuencia que funcionó, y conviene recordarla porque es
el método de la casa: **primero hacer hablar al sistema, después arreglar.**

1. `#445` — al probar `AppTest` salió `builtin 211 no soportado`. Los tres builtins de
   `App` vivían dentro del `#ifdef BPVM_GUI`: rotos en TODA placa sin pantalla. Arreglado
   y verificado en placa.
2. `#446` — y eso destapó por qué el arnés no podía verlo: el host lleva `GUI ?= 1`, o sea
   que **siempre** compila con GUI. 535 líneas del core que ninguna placa pequeña tiene.
3. `#447` — los cuelgues eran mudos porque el SDK deja `isr_hardfault` como bucle infinito.
   Se le puso manejador propio + captura del motivo del `panic`. **No arreglaba nada**, y
   fue lo que desatascó todo: `MemT5_Gc` pasó de misterio a `PANIC: Out of memory` en una
   ejecución.
4. `#448` — y el motivo era gordo: `PICO_MALLOC_PANIC=1` hace que en la RP2350 `malloc`
   **mate en vez de devolver NULL**, anulando el OOM atrapable que V4 construyó (#355).
   Arreglado; dos de los seis cuelgues pasaron a verde y otros dos a error con nombre.
5. `#449` — la causa de fondo: `VM_SRAM_MALLOC_MARGIN` son 64 KB dimensionados en julio
   para `calloc`/`strdup` por import, y en agosto (#430) la tabla de handles pasó a salir
   del mismo `malloc` — 32 KB de golpe. El margen no se tocó. **Arreglo por decidir**, con
   tres opciones y su contrapartida en la ficha.

### El riesgo que esto deja al descubierto

`#430` se diagnosticó y se arregló **en la Metro**, donde el heap se va a la PSRAM y la
SRAM interna entera queda para `malloc`. El consumidor nuevo se estrenó justo en la placa
donde no cuesta nada, y viajó a la Pico 2 en la MISMA imagen.

Es la contrapartida de la imagen única, y no invalida el principio: es que **la Pico 2 y la
Metro tienen realidades de memoria opuestas, y la más estricta es la que menos se prueba**.
Para todo lo que toque memoria, la Pico 2 debería mandar. Aquí pasó al revés.

### Deuda que sigue pendiente

- **El STM32 lleva sin verificar desde `U3.6`** (ver el apartado de deuda en
  `H13_PRUEBAS_V5_REPASO.md`): seis gestos, cinco minutos. No se hizo hoy porque Eduardo
  priorizó, con razón, cazar el bug de la Pico primero.
- **La batería de V4 en las otras familias.** Hoy se corrió sólo en la Pico 2 (y `JsonDemo`
  en la Metro). El S3, el P4 y las dos STM32 sin pasar.
- **El UAF de `MemT5_Gc`**: con el `malloc` arreglado da
  `referencia a objeto eliminado (use-after-free)`. Un OOM no debería producir una
  referencia colgada, así que hay algo más ahí. Con `gc=0` da otra cosa
  (`ALOAD_U8: idx fuera de rango 0 (len=0)`), pero no es control limpio: sin GC el
  programa no llega igual de lejos.
- **El env de la placa cambió a mitad de investigación** (FS 1512→1024 KB, packs
  536 KB→1024 KB) sin que Eduardo lo tocara. Alguna de las imágenes flasheadas hoy
  reescribió el env. No parece afectar a lo perseguido, pero conviene saberlo.

### Herramientas nuevas, que se quedan

- La Pico **dice por qué muere**: fallos con `CFSR`/`HFSR`/`PC` y el motivo del `panic`, al
  log post-mortem.
- El **opcode desconocido** dice módulo, offset y los bytes de alrededor — que parten la
  investigación en dos (PC malo vs código pisado) sin desensamblar a mano.
- `samples/PicoA.bp` y `PicoB.bp`: el caso mínimo de `#440`, idénticos salvo un `import`.


## ⏭️ AL RETOMAR (27-ago, noche) — el repaso de V5, decidido por Eduardo

**Lo primero de mañana**: la batería de V4 completa sobre **V5 puro** (IDE + firmware +
demos del mismo paquete). Lista, condiciones de partida y lo ya sabido en
**`docs/H13_PRUEBAS_V5_REPASO.md`**. Encargo suyo, y la hipótesis es concreta: *«es muy
raro que solamente falle JsonDemo»*.

⚠️ **La placa quedó en un estado mezclado** (IDE de V4 + imagen de V5 + un FS con ~35
ficheros de las pruebas de acotación). Formatear antes de empezar; sobre esa mezcla ningún
resultado sería atribuible.

### Lo que se cerró hoy

**U3 terminado en dos familias.** Ocho pasos, `U3.5` a `U3.12`, cada uno construido en las
cuatro imágenes y **verificado en placa el mismo día** antes de pasar al siguiente. El
STM32 y la Pico quedan con 26 de 29 verbos en el común; los tres que faltan (RUN, KILL,
RESET) se quedan en la familia por diseño. `pico/repl_v1.c` pasa de 2066 a ~1300 líneas.
El detalle, en `FICHAS.md`.

Lo que más valor tuvo no fue mover código, sino **comparar antes de mover**: en cinco de los
ocho pasos apareció una divergencia real entre familias, y **la que lo hacía bien no siempre
era la misma**. Unificar «hacia el común» sin mirar habría degradado una u otra en cada
verbo. Y de camino se cerró #329 y se tapó un agujero (`bpvm_repl_drain_bulk`) que llevaba
en el STM32 desde la mañana y en la Pico lo tapaba una pre-lectura.

### La cacería que se llevó la tarde — dónde quedó

`JsonDemo` falla en la RP2350 (**#440** en `FICHAS.md`, con todo lo medido y lo descartado).
No lo trajo el trabajo de hoy: falla igual con la imagen anterior a la sesión y con la
publicada de V5. Va con la de V4 → **hueco acotado a 6-ago → 22-ago**.

Hay **tres imágenes de bisección ya construidas** (cuartiles 14, 17 y 20 de agosto) en el
scratchpad de la sesión. Si el repaso de mañana no las hace innecesarias, se reconstruyen en
un minuto con `git checkout <sha> -- bpgenvm-c/` + `ninja -C pico/build` + restaurar.
⚠️ Ese ir y venir **deja ficheros de commits viejos** que no existen en HEAD: comprobar con
`git status` y borrarlos tras verificar uno a uno que no están en HEAD.

Dos hallazgos laterales quedan anotados como **#441** (seis opcodes que la VM-Java tiene y
la VM-C no) y **#442** (el mensaje «opcode desconocido» no dice ni módulo ni offset).

### El riesgo que acecha, y no es técnico

La campaña de V4 dejó **2413 líneas** de registro; la de V5, **418**. `JsonDemo` estaba en
la de V4 con su ✅ y no aparece ni una vez en la de V5. No es que no se anote: es que
**cuando una versión hereda de otra, lo que no se re-prueba hay que saber que no se
re-probó**. Ficha **#443**.

Y una lección de método, mía, apuntada también en memoria: tuve los dos anclajes servidos
—el `.uf2` publicado de V4/V5 y la Nucleo en verde con el mismo fichero— y en vez de usarlos
me puse a **generar casos sintéticos para validar una hipótesis propia**. Eduardo lo paró
dos veces. Fabricar casos de prueba **parece** método, pero mide el eje que uno elige; si el
eje está mal, cada peldaño añade precisión sobre nada. El tell: si el siguiente paso que se
me ocurre es *construir* algo en vez de *ejecutar algo que ya existe* en un punto conocido
del pasado, estoy validando, no acotando.


## ⏭️ AL RETOMAR (26-ago, tarde) — decidir entre estas tres

Nada a medias: todo lo tocado hoy está commiteado, construido y verificado en placa.

1. **Seguir U3 — el grupo `ops`** (INFO, DF, FORMAT, SAVE). Pide diseñar
   `bpvm_repl_ops_t`, la struct de cintura: **es la pieza de diseño que le falta al
   contrato** y la que usará el resto de la migración. Eduardo avisó de que la migración
   se le hace *«un poco aburrida»*, así que conviene alternarla, no encadenarla.
2. **E1 — la vista de packs en el árbol**. Es la ficha que mordió hoy con el episodio del
   `/lib` (el fallback del pack contesta `stat`/`read` pero NO `list`), y tiene caso de
   prueba natural montado. Retorno inmediato y visible.
3. **Una de las 9 arrastradas** que el triaje dejó limpias — p.ej. `listDir` ausente en la
   VM-C, o el módulo rancio que sobrevive sin avisar.

### Lo hecho hoy (26-ago)

- **U2 paso 3 y CIERRE del hito**: el STM32 al contrato del wire; cuatro familias, un solo
  transporte. Un bug latente muerto (su `send_error` no escapaba `message`).
- **U3.1/U3.2/U3.3**: nace `bpvm_repl` (contrato + común) y el STM32 delega **11 de 29
  verbos** — meta, FS de fachada y LIST. Ganó RENAME, RMDIR y el `crc` bajo demanda; su
  refresco de árbol pasó de leer el FS entero a no leer nada (#398 nunca le había llegado).
  Cada grupo verificado en placa el mismo día.
- **Triaje de FICHAS**: de 59 arrastres, **9 vivos**. 47 al archivo, íntegros (0 líneas
  perdidas, comprobado contra git).

### Espinas abiertas, sin cerrar

- El **total del FS del STM32 baila** entre arranques (516096 ↔ 614400) y el **recuento de
  KB no cuadra** con lo que se ve. Sin explicar.
- El **P4 configurado a 16 MB** de flash con chip de 32.
- El **instalador de mods es mudo** (`fs_put` sin comprobar ni log).

---

## ⏭️ AL RETOMAR — U3, con el mapa ya hecho

El diseño está decidido y el censo profundo hecho (U3.0 + U3.0b en FICHAS; boceto en
`V6_IDEAS.md` §REPL). Lo que toca al volver, en este orden:

1. **`bpvm_repl.h`** — el contrato: dispatcher común + `bpvm_repl_ops_t` (cintura de
   familia: campos de INFO, RUN, verbos de hardware).
2. **El dispatcher común en `src/`** con los verbos del cubo fácil (PING, TIME, DEL,
   MKDIR, STAT, GET, PUT*, LOG_*, RENAME, RMDIR…), que ya solo tocan wire común + fachada
   de FS común.
3. **Primera familia: el STM32** (920 líneas, la que menos tiene) — con RUN quedándose en
   la familia hasta que los otros 19 estén verificados en placa.
4. Verificación del cordón: el IDE contra la placa tras cada tanda.

**Decisiones ya tomadas** (no reabrir): migración 1:1; los verbos con primitiva entran
gratis al compartir dispatcher; `PROMPT` no se disfraza (muerto en toda la VM-C hasta que
`IO.prompt` se implemente — hallazgo fichado en U3.0b); donde no haya hardware, error con
nombre y no «type no implementado».

---

### 26-ago (3) — U3 arranca por donde debe: midiendo

**U3.0 hecho**: la matriz verbo × familia de los tres REPL. El enunciado del hito («no
tiene contrato») exageraba: hay un **núcleo de 20 verbos idénticos en las tres**, la Pico
es el superconjunto (29), y el protocolo escrito ya documenta la mayoría. Las asimetrías
que muerden y no estaban fichadas: S3/P4/STM32 **incumplen** `RENAME`/`RMDIR`/`FORMAT`
(que están en `BPVM_WIRE_PROTOCOL.md`), y el STM32 además no tiene `PROMPT_RESPONSE`
(⇒ `input()` desde el IDE no puede contestar ahí) ni `SAVE`.

**El diseño del REPL común está BOCETADO en `V6_IDEAS.md`** (dispatcher común + cintura
`bpvm_repl_ops_t` por familia, espejo del patrón de U2), con LA PREGUNTA para Eduardo:
¿migración 1:1 y los verbos que faltan entran gratis al compartir dispatcher, o se
aprovecha para más? Y el riesgo nombrado: RUN es el hueso y se queda en la familia hasta
que los otros 19 estén verificados.

---

### 26-ago (2) — U2 CERRADO: el paso 4 era corregir el registro, no el código

`bpvm_comm.h` medido de punta a punta, y las DOS premisas escritas eran falsas (las dos
mías): no es el contrato de los transportes, y no es que «sólo 2 de 5 lo implementen» — es
el contrato de salida del **modo SMP** (opt-in: host `--smp=N`, Pico por opción de build), y
**las cinco imágenes lo enlazan**: Pico con su backend, las otras cuatro con `comm_host.c`,
tres de ellas como relleno deliberado. En single-worker (todas las placas hoy) la salida va
por `output_cb` y este contrato ni se toca.

Por el camino caí en una TERCERA teoría falsa («enlazan de chiripa por gc-sections») que
también murió por medida: `emit_text` está vivo en todas y el símbolo lo da el relleno.
Tres teorías, tres medidas, y la única que quedó en pie es la que salió de mirar los
CMakeLists y los `.elf`.

**Acción**: el MAPA en `bpvm_comm.h` + el relleno anotado en los CMakeLists de S3/P4. Cero
código, sin reflashear. **U2 queda CERRADO** — protocolo común, cable por familia, STM32
dentro, contrato SMP cartografiado.

**⏭️ Siguiente**: U3, el REPL — 4.318 líneas triplicadas sin contrato, y ya sin la excusa
del transporte: U2 le deja la frontera limpia.

---

### 26-ago — U2 paso 3: el STM32 deja de ser la cuarta forma del wire

Las 4 funciones de cable a los nombres del contrato, `send_error`/`send_cstr` borrados en
favor del común (que YA se compilaba en este build sin llamantes), `send_fatal` como
wrapper con el LED de la placa, 57 llamantes renombrados, y de regalo un bug latente
fuera: el `send_error` propio no escapaba `message`. Verificado por Eduardo en placa:
programas subidos y ejecutados, un módulo cargado y un Pack grabado (todo el camino bulk
con la firma nueva de `recv_bulk`).

**Estado de U2**: pasos 1-3 hechos y en placa (Pico, S3+P4, STM32). Queda el **paso 4**:
decidir qué hacer con `bpvm_comm.h` — el contrato VM↔comunicaciones que sólo implementan
host y Pico. Y los ~22 replies a mano del REPL del STM32 quedan PARA U3, que es su sitio.

---

### 25-ago (noche) — el STM32 cierra N1, y el `.mdn` suelto se retira

**El STM32 ejecuta AOT** — primero de su familia: `NatV7` 4/4 (con `sumaHasta` en 0 ms a
160 MHz) y `NatNew` los siete valores. Costó dos fallos: el **`.bin` del 5-ago** (el
headless regenera el `.elf` pero NO el `.bin` — dos flasheos en falso, y lo desatascó la
pregunta de Eduardo sobre el log, que llevó a dar voz al camino de fallo del RUN), y la
**frontera de coma flotante** hablando dos ABI (Pico softfp / STM32 hard, con el `.mdn`
softfp para las dos: NaN sin un error; en la Pico casaba de chiripa). Arreglo: la
convención fijada en las 28 entradas FP de la tabla (`pcs("aapcs")`, en el tipo Y la
definición — si no coinciden no compila). Verificado en el desensamblado y en placa.

**Y el `.mdn` suelto, RETIRADO** (petición de Eduardo, verificado por él en placa): la app
ya no lo genera, el Explorer limpia el rancio del device — que además habría PISADO a los
thunks embebidos —, y quedan a propósito los de packs y el barrido para deps versionadas.

**🏁 Con esto N1 (alcance V6) queda CERRADO**: formato v7, 28/30 nodos, tres familias en
placa, seis guardianes nuevos. Tuplas y try/catch → V7, como decidió Eduardo.

**⏭️ El siguiente frente natural es U3 (el REPL)** — el trabajo grande de la versión — o
las bolsas L1/E1 para sesiones cortas. U2 tiene pendientes el paso 3 (STM32, cuarta forma)
y `bpvm_comm.h`.

---

### 25-ago (tarde) — LA PARIDAD: era el ARNÉS, no la stdlib — 38/38 ✅

**El diagnóstico escrito era falso, y era mío.** Decía «desfase de slots por stdlib rancia;
regenerarla es decisión de Eduardo». Medido: la stdlib está bien, el compilador está bien y
el gate de #284 hacía su trabajo. Lo que pasaba: el arnés compilaba los 3 samples desde
`bpgenvm-c/samples/`, donde un **`Core.mod` HERMANO del 18-ago** —anterior a que
`Comparable` ganara las conversiones— tapaba a la stdlib (el resolutor mira junto al fuente
antes que en la stdlib). El módulo pedía `Integer#value#2`; luego el arnés copiaba el Core
ACTUAL (`#7`) al lado y ejecutaba. **Compilaba contra una era y ejecutaba contra otra.** El
ERROR DE LINK era la única parte del sistema diciendo la verdad.

**Tres arreglos en el arnés** (`41477504`), y los dos últimos los destapó el primero al
fallar:
1. **Hermético**: la stdlib fresca al outDir ANTES de compilar (outDir gana al hermano).
2. **El raíz por nombre de módulo**: con la stdlib en WORK, «el primer .mod que no sea
   Core» era `Adc.mod` — una librería sin main que las dos VMs fallaban IGUAL: **38/38
   verde sin ejecutar ni un sample**. El falso-PAR de manual.
3. **Cero PASS ya no es verde**: otro fallo intermedio dio 0 PASS / 38 SKIP y el resumen
   decía VERDE.

Verificado con el ARTEFACTO: `CastExt` pide `#7` y su salida es la del sample entero.
**38 PASS / 0 FAIL / 0 SKIP** — primera vez en verde real desde el 22-ago.

**Y la mina de fondo, refrescada pero no resuelta**: 11 módulos de stdlib HERMANOS en
`bpgenvm-c/samples/` (gitignorados, seis eran de MAYO). La VM-C de host los necesita en
ejecución; nadie los refresca. Copiados de `bpstdlib/` a mano; el día que la stdlib cambie
una vtable volverán a mentir. El mecanismo de los tests (`stdlibdep` del Makefile) existe —
los runs manuales no pasan por él.

---


### 25-ago — la P4: AOT y wire, y TRES fallos que sólo la placa podía enseñar

**Qué familia tocaba, y no era una preferencia**: el AOT sólo tiene dos destinos
(`NpackReloc.DESTINOS` = ARM Cortex-M33 y RISC-V32 del P4). **El S3 es Xtensa y no tiene
generador** — su `aot_funcs_stub.c` es un no-op explícito. Así que el ESP32 que toca es
**la P4**.

**Resultado**: `NatV7` **4/4 thunks** y `NatNew` **7/7, 1448 code bytes** —el mismo número
que produjo MdnPack, o sea que cuadra punta a punta— con los siete valores exactos,
ejecutando código RISC-V compilado. Y `U2.1` paso 2 (el S3 y la P4 al protocolo común del wire) verificado de
paso — `INFO` y `ls` se construyen con los 11 builders que se movieron.

**Pero costó tres fallos, y ninguno era del AOT.** Los tres eran lo mismo: **el `.mdn`
cruzaba a la placa sin que nadie comprobara que casaba con ella.** Y ninguno dio un error —
uno colgó y dos devolvieron un número:

1. El `bpvm_aot_clear()` iba **después** de cargar (habría dado 0/N). Arreglado en la Pico
   el 23-ago y nunca viajó al resto.
2. El `.mdn` se compilaba con **otra ABI y otro repertorio**: `AotBuild` no fijaba
   `-march`/`-mabi` *«porque casan por construcción»*, y el defecto del toolchain trae la
   extensión `d` — coma flotante de doble **en hardware, que el P4 no tiene**. Con doubles
   derramados a la pila, `fld`/`fsd` = instrucción ilegal → **cuelgue**.
3. Las **constantes de coma flotante** se quedaban fuera del blob: `mdn.ld` fusiona
   `.rodata*` en `.text`, y RISC-V las pone en `.srodata*`. El `auipc` apuntaba más allá
   del final. `(3.0+5.0)/2.0` = **Infinity**, un `float[]` = **1.219193E25**.

**📌 Y un cuarto que fue el que permitió ver los otros**: el log del cargador de `.mdn` era
un hook débil que **sólo la Pico implementaba**. La P4 cargaba el bloque y no había forma de
saber si sus thunks habían entrado. Se quitó el hook; ahora escribe en el log común y las
cuatro familias ven lo mismo.

**🕸️ Lo que queda: tres guardianes donde no había ninguno** — ABI y repertorio del `.o`; que
no sobreviva nada con contenido fuera de `.text`; y el nombre del símbolo contra el `.mod`
(de ayer). **Los tres probados en rojo** con el artefacto que fallaba de verdad.

📐 **La lección, y es la misma tres veces**: el `.npk` valida arquitectura y float-ABI desde
V5/H4. El `.mdn` **no validaba nada**. Dos formatos hermanos, uno con contrato y otro sin
él, y el que no lo tenía es el que se cargó tres veces en un día.

⚠️ **Dos falsos verdes míos**, los dos por mirar el log de la herramienta en vez del
artefacto: dije «las cuatro imágenes compilan `bpvm_log.c`» a partir de un `grep -l` que dio
positivo por una mención suelta, y canté «host OK» cuando el enlace había fallado y
`bpgenvm-c.exe` ni existía (la paridad se fue a 5/33 y pareció que había roto la VM).
**Comprobar el ARTEFACTO, no el log.**

⏭️ **Queda**: el STM32, y entonces retirar el `.mdn` suelto. Y una cosa que la P4 avisa sola
y no es de hoy: el proyecto está configurado a **16 MB de flash con un chip de 32** — la
zona de packs se queda en 2800 KB. Cambiarlo mueve el mapa de particiones, así que merece su
propio paso verificado.

---

### 24-ago (tarde) — U2.1: los gemelos del wire no lo eran, y el primer paso ya está en placa

Con AOT cerrado en la Pico, Eduardo propuso unificar algo de las imágenes. Salió `U2.1`, y
**los dos enunciados del hito eran falsos**:

- Decía que `pico/wire_v1.c` y `esp32/main/wire_v1.c` *«son dos programas distintos con el
  mismo nombre; el 100 % de sus líneas difieren»*. Medido palabra por palabra: las tres
  familias con wire exponen **las mismas 15 funciones** y **once son idénticas** (88 líneas
  × 3 copias). Las cuatro que difieren son **exactamente** las cuatro que tocan el cable.
  La costura ya existía y estaba limpia.
- Y `U2.2` decía que los transportes no incluyen `bpvm_comm.h`. Ese header **no es su
  contrato**: es el de VM↔comunicaciones, y lo implementan **dos de las cinco** imágenes.

**Paso 1 hecho y verificado en placa** (`bb530e3f`): las 11 + sus 4 helpers a
`src/wire_v1_proto.c`, con `include/bpvm_wire_v1.h` nombrando las dos capas. Sólo la Pico.
Eduardo confirma que el IDE funciona **incluida la subida de un Pack** — la prueba fuerte,
porque el bulk es binario y ahí un fallo de framing corrompe en vez de fallar.

🎯 **Y el gesto que hay que repetir**: antes de flashear se comparó la imagen nueva con la
vieja — **código máquina idéntico en las 15**, mismo conjunto de 1984 símbolos con los
mismos tamaños, `.text` idéntico byte a byte. Por eso el bloque se movió **tal cual**, sin
mejorar nada de paso: si se hubiera "aprovechado" para limpiar, esa comparación no existiría
y la prueba en placa habría sido un descubrimiento en vez de una confirmación.

⚠️ **Criterio de Eduardo que manda sobre todo este hito**: *«las comunicaciones son nuestro
cordón umbilical entre el PC y el micro. Cada cambio lo hemos de verificar en placa; será
un poco pesado pero aquí nos importa más la seguridad que la velocidad. En vez de 1 gran
cambio, mejor 3 o 4 pequeños.»*

---

## ⏭️ PARA MAÑANA — verificar AOT y comunicaciones en OTRA familia

Las dos cosas piden las mismas placas encendidas, así que van en el mismo viaje al banco.

**🔴 Hay un prerrequisito que se hace SIN placa, y conviene hacerlo antes de encenderla:**
el `bpvm_aot_clear()` mal colocado sigue **sin arreglar** en `esp32/main/repl_esp32.c:1001`
y `stm32/port/stm32_repl.c:548`. Está *después* de la carga, así que **borra los thunks que
el módulo acaba de registrar**. Si mañana se enciende el S3 o la P4 sin arreglarlo, el AOT
saldrá 0/N y se perderá la sesión buscando lo que ya sabemos. En la Pico se arregló el
23-ago (`repl_v1.c`).

**Lo que hay que llevar hecho:**
1. El `clear()` en las dos familias que faltan.
2. `U2.1` paso 2: borrar las copias del protocolo en `esp32/main/wire_v1.c` y
   `esp32p4/main/wire_v1_tcp.c`, dar de alta `src/wire_v1_proto.c` en sus builds, y
   **comparar el código generado antes/después** como se hizo con la Pico.
3. ~~Regenerar los cuatro artefactos nativos de SQLite + `SQLite.pack`.~~ ❌ **FALSO, y
   comprobado el 25-ago**: el `.npk` NO lleva el ABI de la tabla de helpers — se valida por
   **arquitectura y float-ABI** (`bpvm_npack.c` usa `bpvm_mdn_host_arch`/`_float_abi`, no
   `MDN_ABI_VERSION`). La prueba está en el log del P4 con la imagen de ABI 6: *«sqlite:
   pack vivo · 3.53.4 · initialize OK — motor arrancado y vfs 'bp' registrado · API
   publicada como 'SQLI', 17 simbolos»*. Confirmado **al cargar**; confirmarlo **en uso**
   pide correr un sample del ORM, que es otra cosa.

**Qué mirar en la placa** (lo mismo que hoy en la Pico, y con `log=1`, que se lee en el
arranque):
- AOT: `MDN: N/N thunks registrados` **sin ningún `skip`**, y `[mdn] 1 bloque(s) nativo(s)
  desde el propio .mod`. Los samples ya están: `samples/NatNew.bp` (7 thunks) y
  `samples/NatV7.bp` (4). ⚠️ La salida correcta **no demuestra nada** sin esas líneas.
- Wire: que el IDE conecte, liste, ejecute… **y suba un Pack**, que es el camino bulk.

📌 **Y una decisión pequeña que ahorra tiempo**: elegir UNA familia y hacerla entera
(AOT + wire), no las dos a medias — [[focus-un-kit-batch-cross-family]].

---

### 24-ago — N1.5: la `native` ya FABRICA, y el censo dejó de mentir

**Lo pedido**: *«a ver si podemos cerrar AOT»*, y a mitad de sesión Eduardo acotó: *«tuplas
las aplazamos a V7. Lo que nos queda en V6 es instanciar arrays y objetos»*.

**Hecho**: arrays de **todos** los tipos de elemento dentro de una `native`
(i32/i8/i16/i64/f64/f32 y **referencias**: `string[]`, `Clase[]`) y creación de objetos. Un
objeto no se aloca en C —el constructor es bytecode—: se cruza a la factoría
`__cls_new_<Clase>`, la ruta que `#213` ya había abierto para `throw`. Detalle completo en
`FICHAS.md` → N1.5 y N1.5b.

**📌 Lo que hay que llevarse de esta sesión no es la feature: es que TRES cosas que
parecían medidas no lo estaban — y de las tres, DOS eran errores míos de medida, no
fallos del código.** Las dos las destapó Eduardo preguntando, no una prueba.

1. **El censo del AOT medía cinco fragmentos mal escritos, no el AOT.** `ThrowStmt`
   llevaba soportado desde `#186` y lo tapaba un `throw "vaya"` que #248 no permite. El
   arnés sólo miraba errores del *parser*: lo que no compilaba salía como «no soportado».
   Arreglado el arnés, la foto pasó de «23 medidos» a **28 de 30 medidos de verdad**.
2. ~~El puente native→BP nunca había llegado a una placa.~~ **FALSO, y corregido el
   mismo día a partir de una pregunta de Eduardo** (*«lo de los literales lo vimos en V5»*).
   Yo comprobaba el `.o`; el pipeline real empaqueta el `.elf` **ENLAZADO**, y el guión
   `bpgenvm-c/aot/mdn.ld` fusiona `.rodata` dentro de `.text` — eso es `#428`, cerrado en
   V5 y verificado en la Metro. El cambio que metí por esto sobraba: revertido.
   ⚠️ El mismo error de medida había producido el «`print` no compila para ARM» del
   23-ago. **Dos límites declarados que no existían**, los dos por medir una etapa que el
   producto no tiene. La regla: si `MdnPack` se queja de un `.LC0`, la pregunta es si
   estás enlazando.
3. **Indexar un `long[]` en native leía 4 bytes donde hay 8.** No fallaba: devolvía otro
   número. El límite estaba escrito en un comentario y no lo hacía cumplir nadie.

Y de propina, uno **ajeno al AOT**: `OP_ASTORE_I16` de la VM-C ponía a cero el elemento
siguiente. Divergencia con miVM, o sea el invariante roto — cazada por un `word[4]` de tres
líneas escrito para comprobar otra cosa.

**⚙️ La herramienta que queda, que vale más que los arreglos**: `make test-aotnew`. Compila
el `.c` generado con el compilador del host y lo **EJECUTA** con `gc_bump_threshold = 1`
(GC en cada alocación). Es el tercer peldaño que faltaba —**emite / cabe / acierta**— y es
el que cazó el `long[]`. Los dos primeros ya habían dado verde.

**✅ VERIFICADO EN LA PICO esa misma tarde.** Imagen reconstruida (ABI 6) + fat-jar del IDE
reconstruido —comprobado a mano que estampa `abi_version=6`, porque un desfase ahí costó un
flasheo el 23—. El log de la placa:

```
[313579] MDN: 7/7 thunks registrados, 1172 code bytes (zero-copy)
[313579] [mdn] 1 bloque(s) nativo(s) desde el propio .mod
```

Los siete valores exactos. **La duda que había que despejar** era si esos thunks venían de
la sección del `.mod` o del `.mdn` suelto que el IDE sigue subiendo al lado: la despeja de
dónde sale el mensaje —`bpvm_load_mdn(vm, sec + p, ...)`, con `sec` dentro de la sección— y
el hecho de que el barrido del suelto entra 17 ms después sin registrar nada. No hizo falta
borrar nada.

**🔴 Y la segunda pasada, con `NatV7`, destapó que N1.1 NUNCA se había acelerado.** Dio
`MDN: skip 'NatV7.Caja_doble' rc=-2` → **3 de 4**. El `.mod` exporta el método como
`Caja.doble` y `MdnPack` pedía `Caja_doble`: reconstruía el nombre partiendo el
identificador de C, y de `Caja_doble` no se recupera `Caja.doble`. Las funciones de MÓDULO
sí acertaban (no tienen punto que perder), así que el fallo vivía detrás de los casos que
iban — 3 verdes no dicen nada del cuarto.

Y no daba error: el método corría interpretado y el programa imprimía 42. **N1.1 se cerró
el 23-ago verificado por EMISIÓN** —el C era impecable con el nombre equivocado— y ésta fue
la primera vez que ese caso se ejecutó en una placa. Arreglado poniendo el nombre verdadero
en el símbolo ELF; reverificado: **4/4, sin skip**. Sin tocar C, o sea sin reflashear.

**⚙️ Queda `AotSimboloEnModTest`**, que compara los nombres que el AOT registra contra los
que el `.mod` exporta. Es la red que faltaba, y caza la clase entera porque compara DOS
ARTEFACTOS — el fallo era un desacuerdo entre `AotCEmitter` y `MivmEmitter`. Puesta en rojo
a propósito antes de darla por buena.

📌 **El patrón del día, tres veces**: N1.1, el `long[]` y el censo estaban «verificados» por
emisión. Lo que los destapó fue **ejecutar** — en host el `long[]`, en placa los otros dos.
Y en placa hizo falta además `log=1`: sin él, `NatV7` imprime los cuatro valores correctos
y no hay nada que mirar.

**⏭️ Riesgo que sigue**: las otras dos familias (S3/P4 y STM32) no han ejecutado una línea
de esto, y el `bpvm_aot_clear()` mal colocado sigue sin arreglar en `repl_esp32.c` y
`stm32_repl.c`. Al desplegar ahí hay que regenerar además los **cuatro artefactos nativos de
SQLite + `SQLite.pack`** (ABI 5 → 6), y sólo entonces retirar el `.mdn` suelto
(`PicoExplorer.java:1106`).

**⚠️ Y sigue en ROJO lo de ayer, sin tocar**: la paridad dual-VM da **35 PASS / 3 FAIL**
(`CastExt`, `ListaBp`, `ListaHer`) por desfase de slots en `Core.mod` — el compilador pide
`Integer#value#2` y la stdlib publicada exporta `#7`. Es **anterior** a estos cambios
(comprobado revirtiendo) y regenerar la stdlib es un cambio de ABI: decisión de Eduardo.

### 23-ago (noche) — N1.3 y N1.4: el `.mdn` ya viaja DENTRO del `.mod`, validado en placa

**N1.3 — los statements sencillos**, y la cuarta pata primero: **`AotCoberturaTest`**, que
**mide** qué construcciones pasan por el AOT en vez de recordarlo. Cobertura **20 → 23 de
27** (`null`, `do…loop`, `print`).
📌 El medidor se validó a sí mismo: la primera medida decía 17 de 28, y al hacer que dijera
el **motivo** de cada rechazo salieron **seis fragmentos míos mal escritos** — cuatro nodos
sí estaban soportados. Un medidor sin la columna del porqué acusa a quien no es.
📌 Y `print` **no era** «una llamada al runtime que ya existe», como decía la ficha: el
intérprete despacha por tipo y la tabla de helpers sólo cubre tres. Va lo que hay y se
rechaza el resto **nombrando el tipo**.

**N1.4 — la fusión `.mod`/`.mdn`.** Formato **v7** (header de 36 B, sección `native` entre
`interface` y `data`), fusión **en memoria** (idea de Eduardo: `MdnPack` ya construía el
`.mdn` en un buffer), **acumulativa** por familia, y el cargador registrando desde ahí.
✅ **VALIDADO EN LA PICO con el `.mdn` BORRADO: 8601 → 84 ms (102×)**, mismo resultado, y
`zero-copy` — el código se ejecuta donde está.
📌 **v6 sigue ejecutándose**, así que no hubo que regenerar los 32 `.mod` existentes: el
gate del `#284` vigila el ABI y v7 no lo toca.
📌 **`MOD_FORMAT.md` llevaba DOS versiones de retraso** (describía v5 con el compilador
emitiendo v6). Ahora documenta v6 y v7.

🩸 **LA LECCIÓN DEL DÍA, y es la más cara que ha dado V6: SEIS fallos, todos míos, y
NINGUNO detectable sin placa** — orden de la fusión · ABI subido sólo en C · `data+off` con
lectura por trozos · relleno copiado (blob desalineado → hard fault) · `malloc`+`free` con
un cargador zero-copy (thunks a memoria liberada) · `bpvm_aot_clear()` tras la carga.
**Los seis pasaron por 109 pruebas de compilador, 39 de VM y la paridad dual-VM.** No es que
las redes fallaran: **ninguna toca el camino AOT en placa**. Ése es el hueco de cobertura, y
ahora está medido.
🔦 **Y el instrumento decisivo fue el log del dispositivo.** Tres iteraciones teorizando
sobre código; en cuanto se encendió (`log=1`), una línea —«1/1 thunks registrados»— descartó
formato, subida, ABI, alineación y lectura de golpe. **Encender el log es el primer paso, no
el último.**
📐 Dos de esos seis los anticipó la pregunta de Eduardo sobre la **alineación**, hecha antes
de que existiera un solo blob.

**⏭️ AL VOLVER:**
1. **Las otras cuatro imágenes** — y ahí hay uno esperando: el `bpvm_aot_clear()` mal
   colocado está también en `repl_esp32.c` y `stm32_repl.c`. Es **U3** en estado puro: un
   arreglo que hay que hacer tres veces y que, si se hace en dos, falla mudo en la tercera.
2. **Retirar el `.mdn` suelto** cuando todas lean la sección. Ése es el día en que los dos
   ficheros dejan de poder desparejarse — el objetivo de la ficha.
3. Y sigue pendiente **la paridad en rojo** (3 fallos por desfase de ranuras en `Core.mod`),
   que no es de hoy y nadie vigila.


### 23-ago (tarde-3) — el hito AOT: `native` en métodos y `double`, los dos cerrados

**N1.1 — los métodos `native` ya se compilan a C** (`e1842bd9`, `631f55fa`). Salió tal como
lo planteó Eduardo —*«mimodulo.miclase.mimetodonative(objMiclase, ...)»*— y **no hubo que
forzar nada**: `ModWriter.addMethod` ya hacía `declareParam("this", 8)` antes que los demás.
Faltaba **exportar el nombre**, porque el registro AOT busca el símbolo para sacar la
dirección. Y la lectura de miembros va **por el getter** (`call_method_i32`), que es como lo
emite el bytecode y la única ruta que el runtime soporta.

**N1.2 — `double` ya cruza** (`a853b9a9`, `e8b1c8f1`). 21 helpers al final de la tabla, el
emisor los usa, `MDN_ABI_VERSION` 4→5. `DPOW` pasa a `bpvm_dpow`: **una** implementación que
comparten intérprete y helper.

🧠 **Y la lección del día, que es la misma tres veces.** Eduardo me corrigió en las tres, y
las tres tenían la misma forma — yo escribía *«no se puede»* donde era *«no está hecho»* o
*«es más caro»*:
1. *«el cuerpo del método no tiene símbolo»* → lo tenía; faltaba un booleano.
2. *«hace falta el layout de la clase»* → hacía falta llamar al getter.
3. *«no se puede llamar al opcode»* → sí se puede (lo hicimos con el getter); lo cierto es
   que para aritmética el helper es más barato.
Ver [[no-se-puede-vs-no-esta-implementado]]. La señal es que la frase salga de **no haber
mirado**, no de haber medido.

**Redes nuevas** (las dos primeras del AOT que hay en el repo): `AotNativeEnMetodoTest`
(4 casos) y **`powtest` en el corpus de paridad**, que cubría `doubletest` pero ningún caso
de `^`. Paridad 34 → **35 PASS**. Baterías 108/0 y 34/0.

🔴 **HALLAZGO QUE NO ES MÍO Y CONVIENE MIRAR PRONTO: el arnés de paridad está en ROJO.**
Tres samples (`CastExt`, `ListaBp`, `ListaHer`) fallan con
*«lib 'Core' presente pero no exporta `Core.Integer#value#2`»*. Es **desfase de ranuras**: el
`Core.mod` publicado tiene `Integer.value` en el **slot 7** y el compilador de hoy lo pide en
el **2**. Lo aislé revirtiendo mis cambios y reconstruyendo: **sale igual**, o sea que es
previo. H13 no lo vio porque su batería es `scripts/h13-lista.sh`, no este arnés — **V5 se
publicó sin que nadie mirara esta red**. Regenerar la stdlib es cambio de ABI, así que no lo
toqué.

**⏭️ AL VOLVER, tres cosas:**
1. **Lo de la paridad en rojo** — es el invariante sagrado el que está sin vigilar.
2. **Las pruebas en placa**, que acumulan ya tres cambios sin verificar: el log de la Pico
   (U1.2), los métodos `native` y los `double`. ⚠️ Ese día hay que **reflashear las cinco
   imágenes Y regenerar los cuatro nativos de SQLite con su pack A LA VEZ** — el ABI subió,
   y hacer sólo una mitad deja el arranque rechazando los packs.
3. Del hito N1 quedan los **statements sencillos** y la **fusión `.mod`/`.mdn`** (punto 4),
   que Eduardo dejó para más adelante.


### 23-ago (tarde-2) — U1 hecho y cerrado, y siete hitos más

**Código, por fin.** Las dos tareas de U1, verificadas **en las cinco imágenes**:

- **`U1.2` — el log de la Pico al núcleo común** (`31ad091e`): de **280 a 101 líneas**. Era
  la única familia que no compilaba `src/bpvm_log.c` —su `CMakeLists` ni lo nombraba—
  mientras STM32 y ESP32 ya sólo aportaban cintura, así que **había dos ejemplos** y no hubo
  que diseñar nada. ⏳ **Falta placa**: que el post-mortem sobreviva al reinicio.
- **`U1.1` — `json_min` al común** (`b506bbde`): de 3 copias byte-idénticas a una. El
  trabajo no era mover el fichero sino el alta en **cinco builds** — y de paso arregla el
  simulador, que metía mano en `pico/`.

**Y U1 se cerró con DOS tareas, no cuatro.** Las otras dos se cayeron al mirarlas:

- **`U1.3`** lo paró Eduardo: *«si vamos a fusionar `.mod` y `.mdn`, deja de tener sentido»*.
  Cierto — la fusión borra ese código.
- **`U1.4`** tenía la **premisa falsa, y era mía**: dije «cuelgan de `bpvm_part.h`» y
  `bpvm_part.h` no hace E/S de flash. Lo real: ~12 cableados de `erase`/`program` y **dos
  cinturas distintas** ya existentes. Es diseño, no mecánica.

🧠 **La lección, que vale para U2–U5**: mi criterio de «fácil» era *«¿existe ya el
contrato?»*. Detecta divergencia muy bien y **no presupuesta nada** — ni mira si el trabajo
sobrevivirá a lo ya planeado.

**Hallazgo gordo, y arreglado** (`27dadba3`, `c7d0bbee`): **los dos proyectos STM32 tenían
una configuración `Release` abandonada**. A la de la Discovery le faltaban cuatro `-D`
—entre ellos `BPVM_BOARD_DK2`, sin el cual `board.h` cae a la rama de la **Nucleo**—, dos
rutas de include y tres exclusiones; la de la Nucleo no tenía **ninguna** ruta ni exclusión,
y ni siquiera se había construido nunca. Lo que se publica sale de `Debug`, comprobado con
números (887.604 B contra los 888.268 del `.bin` publicado). **Eduardo mandó borrar las dos.**
⚠️ Queda dicho lo que es harina de otro costal: esa única configuración se llama `Debug`
aunque compile a `-Os` y sea la de publicar.

**Siete hitos nuevos** (`8a337ad7`): **N1** AOT · **L1** lenguaje y compilador · **E1** IDE
y wire · **G1** el bucle de LVGL a un hilo BP propio · **P1** ESP32-C3 y C6 · **P2**
pantallas SPI (tras P1). Más `A1`, la revisión por niveles, tras la unificación.

**⏭️ AL VOLVER — encargo de Eduardo:** *«antes de empezar U2, que me parece un trabajo
bastante pesado, me gustaría abordar un poco de AOT (1 o 2 puntos, no todo), y así vamos
cambiando un poco de tipo de tareas»*. El hito **N1** tiene **cuatro** puntos —la fusión `.mod`/`.mdn` entró ahí al final del día,
por decisión suya: *«aunque no sea exactamente AOT, cuando terminemos se puede probar todo
junto en placa»*— y el orden que él mismo dejó escrito el 21-ago es **por tandas, no de un
salto**:

1. **`native` en un MÉTODO** — hoy se ignora **en silencio**. Son dos cosas y en este orden:
   que **AVISE** (barato, y `AOT_LIMITES.md` dice que *no espera a V6*) y luego abrir el
   barrido de `AotCEmitter.java:259` a los métodos, pasando el objeto como primer parámetro.
   📌 `MemberAccessExpr` ya está soportado, así que lo del `this` puede que no sea el muro
   que parecía — ver [[no-se-puede-vs-no-esta-implementado]].
2. **`double`** (`#426`) — diseño hecho en `docs/V6_IDEAS.md` §double.
3. **Los statements sencillos**, del censo de `AOT_LIMITES.md`.

4. **La fusión `.mod`/`.mdn`** — diseño hecho, y **el punto más pesado del hito**: sube la
   versión del `.mod`, deja rancias las cuatro copias de la stdlib, y toca la lógica de poda
   y comparación del IDE.

El 1 parece el mejor primer paso: su mitad barata (avisar) cabe en una sesión corta y
convierte una mentira muda en una línea. La progresión natural es barato primero y la
fusión cuando haya rato seguido.


### 23-ago — V6 abierto: el índice, al día, y el primer paso verificado

Sesión corta, de orientación. **No se tocó código.**

- **`V6_BACKLOG.md` no recogía el bloque de arquitectura del 22-ago.** El índice se escribió
  el 21; las nueve reflexiones sobre unificación salieron el 22 y sólo dos llegaron.
  Faltaban ocho — la columna vertebral del hito. Entran en §C como «La fundamentación»,
  separadas de las tareas porque no son tareas: son el porqué y la medida. `666732fd`.
- **Verificado el primer paso que sugiere la medida** (`FICHAS` L2492, «el orden que sugiere
  la medida»): `json_min` es **realmente gratis**. Los tres `.c` son byte-idénticos (8.688 B,
  mismo md5) y las tres cabeceras declaran las mismas seis funciones; sólo cambia el formato.
  Se puede subir a `src/` sin decidir nada.
- ⚠️ **Y una advertencia sobre mí mismo, porque hoy pasó cuatro veces**: varios `grep` míos
  dieron falso rojo —alternancia `\|` con `-E`, clases sin dígitos, un patrón que exigía el
  paréntesis pegado— y llegué a dar por asimétrico un `json_min.h` que estaba bien. Ninguna
  llegó a los docs, pero el patrón es el de [[instrumento-mudo-dudar-de-el]]: **antes de
  declarar algo roto, contrastar contra un caso que se sabe bueno.**

**Y por la tarde, encargo de Eduardo:** *«archivar V5, poner V6 en primer plano; lo de V6
que esté por ahí en diferentes archivos se unifica; las notas de V6 sin guardar ya se pueden
guardar»*. Hecho en `6351b990`:

- **Guardado lo que vivía fuera del repositorio** (`notas/`, ignorada por git): `V6_IDEAS.md`
  (538 líneas, cinco diseños ya trabajados), `V5_BACKLOG.md` (1.116) y `V5_IDEAS.md` (2.977).
  ⚠️ **Y aquí una trampa que casi muerde**: `V5_IDEAS.md` existía en los DOS sitios y no eran
  copias — 25 secciones en `notas/` frente a **una distinta** en `docs/`. Sobrescribir habría
  borrado esa. Se fusionaron: 26 secciones, verificado.
- **`FICHAS` reordenado sin perder una línea** (comprobado por conteo, no por confianza):
  ABIERTAS empieza por V6; debajo, las **heredadas de V5**, que NO se archivan porque no
  están resueltas — les falta decidir si entran en V6. Bajan al archivo cuatro secciones ya
  terminadas y el CODE FREEZE, que arriba se leía como una instrucción en vigor.
- **Unificado lo de V6 que estaba suelto: cinco asuntos vivos** que el índice no recogía —
  los packs del S3 sin región, el módulo rancio de ESP32/STM32, el estado persistente de la
  Metro, `native` en un método, y **el CENSO FUNCIONAL** que Eduardo especificó el 17-ago y
  que sólo estaba dentro de `CENSO_FAMILIAS.md`. De 31 asuntos a 36.
- Las tres copias de `notas/` quedan marcadas **⛔ COPIA MUERTA** para que nadie edite la
  equivocada. Sin borrar: eso lo decide Eduardo.

**Lo siguiente, sin compromiso de fecha:** decidir el ALCANCE de V6 antes de tocar nada. La
medida sugiere `json_min` → REPL (el 80 % del problema) → el log. El REPL es el trabajo de
verdad y no cabe en una sesión corta.

**Y esa decisión la tomó Eduardo en el momento**, así que no quedó esperando: *«lo que haya
de V5 que quedó pendiente pasa a V6 y deja de ser de V5»*. El bloque heredado deja de ser un
limbo (`09640964`). Los títulos de sección siguen diciendo `V5/H10` o «la cola de H2» a
propósito: dicen **de dónde viene** la ficha, no a qué hito pertenece.

⚠️ **Lo que SÍ queda, y es distinto:** de las 59 entradas heredadas, **unas 43 llevan marca
de cierre** y deberían estar en «CERRADAS». No se movieron porque separarlas exige leerlas
una a una — mi clasificador automático falló en dos (`#422` decía «EL CHIVATO, HECHO» y no
lo detectó). Es una limpieza de una sesión corta, y hasta hacerla **ABIERTAS parece mucho
más grande de lo que es**.

**Y la sesión siguió, con el censo funcional de V6** (`CENSO_SISTEMAS_V6.md`, nuevo):

- **Paso 1 — los 32 sistemas.** Salen del listado real de fuentes y del `grep` de quién
  implementa cada interfaz. Contesta la pregunta de Eduardo sobre la memoria: hoy **reserva
  y GC son UN solo sistema** (`heap.c`, 1.121 líneas), y hay una **tercera** pieza que no es
  ninguna de las dos — la tabla de handles, sin fichero, repartida por cinco `.c`.
- **Paso 2 — cuántos están unificados**, con una prueba **mecánica**: un `.c` por familia
  está bien construido si implementa un contrato de una cabecera común. **22 de 32 están
  donde deben**; de los 10 que faltan, sólo 3 son hardware. Dos hallazgos: **no existe
  `bpvm_repl.h`** (el REPL no tiene el contrato roto, no tiene contrato), y los dos
  `wire_v1.c` **difieren al 100 %** pese al nombre.
- 🔴 **Y el censo estuvo MAL y lo cazó Eduardo.** Me dejé fuera `esp32p4/` entero: censé los
  directorios que *recordaba*, no los que hay. Lo destapó una contradicción de mi propio
  documento —«la BIOS sólo la tiene la Pico» cuando SQLite corre en la P4—. La P4 sí tiene
  BIOS, con la misma macro. Corregido en `b0d33ff9`. **Son CUATRO árboles de placa, no
  tres.**
- **Los hitos U1–U5** (`FICHAS` §«LOS HITOS DE V6»), por decisión de Eduardo: unificar antes
  de seguir analizando, de lo fácil a lo difícil, y la revisión por niveles (A1) **después**
  — *«al estar todo unificado, cualquier cambio estructural se hace una vez y no 3 veces»*.
- **Tres decisiones cerradas**: `/sd` pasa a **prefijo reservado** (cambio de comportamiento
  → notas de versión) · `Map` llevará objetos internos para **claves y valores** + `add`
  sobrecargado para clave entera y cadena · y **`notas/` se vacía**: borrado todo lo de V5.

**⏭️ Al volver**: U1 es lo siguiente, y **U1.2 (el log de la Pico) es el mejor primer paso**
— cierra `#423`, hay **dos ejemplos** de cómo debe quedar (STM32 y ESP32 ya sólo tienen
cintura), y se comprueba fácil: que el post-mortem siga sobreviviendo al reinicio. `U1.1`
(`json_min`) es igual de mecánico pero toca **los cinco builds**, así que con calma.


### 22-ago (tarde) — V5 PUBLICADA

Se empujaron los **358 commits** acumulados desde el 6-ago, se etiquetó `v5.0` sobre
`ff408a1e` y se creó la release con el ZIP como artefacto único. Detalle y verificación,
en `FICHAS.md` §«V5 PUBLICADA».

**Lo que pasó por el camino, que es lo que interesa aquí:**

- **La revisión del push encontró dos carpetas que nadie había mirado**:
  `bp_propuesta_modelo_memoria/` (205 KB de mayo-julio) y `diag/orm-slots/`. Eduardo dejó
  la decisión abierta —*«si crees que es interesante se sube… pero es antiguo»*— y suben
  las dos. La primera con portada nueva, porque no tenía raíz y citaba 13 veces unas
  carpetas que nunca estuvieron en el repo.
- **Mi primer recuento de enlaces rotos estuvo mal**: resolví los `../` contra la raíz del
  conjunto en vez de contra la carpeta de cada fichero, y me salieron rotos que no lo
  eran. Rehecho bien antes de escribir nada.
- **Las cuatro portadas seguían anunciando V4.** Al corregirlas hubo que **rehacer el ZIP**
  (la web viaja dentro, es la ayuda del IDE) *después* de que Eduardo ya lo hubiera
  probado. La lista de ficheros quedó idéntica, así que su prueba seguía valiendo — pero
  por suerte, no por método. Corregido el orden en `PUBLICAR.md`.
- **Eduardo avisó del problema de refresco de la portada de V4** y estaba fichado: dos
  despliegues seguidos se pisan y el segundo **falla en silencio**. Esta vez se comprobó
  el `status` del build *y además* qué sirve la web con `curl`. Las dos cosas, verdes.
- **El checklist mentía sobre el cuerpo de la release.** Decía «= sección de
  `RELEASES.md`»; V4 había publicado un cuerpo **bilingüe** con Descarga y Documentación.
  Se siguió el precedente, no el doc, y se corrigió el doc.

**Riesgo que queda:** ninguno bloqueante. El congelado está levantado; lo siguiente es V6
y su índice está en `V6_BACKLOG.md`.

### 22-ago — H13 TERMINADO: seis placas, y ocho bugs por el camino

**Las tres familias cerradas.** `ListGets` da **21 líneas idénticas** al PC en Pico, Metro,
Nucleo, Discovery, S3 y P4 — tres familias, **tres arquitecturas** (ARM, Xtensa, RISC-V).
La Puerta 1 era el gate no negociable y está pasado en todo el parque.

**La matriz de nativo, completa.** `.mdn` generado al vuelo: ARM **105×** (Metro), RISC-V
**112×** (P4). `.npk` precompilado en pack: ARM y **RISC-V**, los dos con `SqlDemo` 25/25.
Las cuatro casillas, que hasta hoy eran dos.

**Y la BD entera en DOS familias**: `SqlDemo` 25/25, `DaoDemo` 25/25, `GenDemo` 10/10 — en
la Metro (ARM) y en el P4 (RISC-V). 60 líneas por placa, idénticas al PC.

**Ocho bugs arreglados, ninguno visible leyendo código:**
- 🩸 **Los packs no se podían grabar en STM32**: el IDE rearma el pack antes de mandarlo y
  lo alineaba a **4 KB**, pero el U5 borra en páginas de **8 KB**. Fallaba la mitad de las
  veces con un `BAD_ALIGN` que **culpaba al tamaño del fichero, lo único correcto**.
- 🩸 **El pack se negaba a grabarse si el micro no tenía motor nativo.** Decisión de
  Eduardo: *«lleve nativo o no, el pack se graba; si el nativo no corresponde, se graba SIN
  nativo»*. Y el mensaje anterior aconsejaba **reconectar**, cosa imposible de arreglar en
  un Xtensa.
- 🩸 **`/app` es el punto ciego**: el STM32 vacía y reembebe `/lib` cada arranque pero no
  toca `/app`, donde el IDE deja las dependencias. Un `Core.mod` rancio ahí dio `exit 11`
  con `/lib` recién puesto. Lo encontró Eduardo con `dir /app`.
- Y: la guía de instalación **sin la Discovery** (que sí viaja en el ZIP), el manual
  **negando el AOT del P4** (lo tiene desde V4, 113×), `ChartDemo` con ruta rancia en la
  batería (que queda **51/19/0/0**, limpia), y los samples y blobs del día anterior.

**Y tres cosas que no son bugs pero faltaban:**
- **`#408` CERRADA**: el árbol del P4 son **145 ms** (`app:1/3 lib:14/64 sd:8/33`) y el
  formateo de la Metro **15 s** — lo que cuesta borrar 4,2 MB. Ninguno es un cuello.
- **La decisión 5 CONTESTADA, y son TRES respuestas**: RP2350 instala si falta *o difiere*
  y **avisa**; STM32 vacía y reembebe `/lib` cada boot; ESP32 instala **sólo si falta y no
  avisa**. La instrucción honesta para la release vale para las tres: **borrar `/lib` y
  `/app`**.
- **El S3 no expone packs** — no existe `pack_s3.c`. Es el agujero de `#327` (que fue el de
  la Pico) migrado de familia. Fichado; no bloquea porque nunca se prometió.

**🧠 Y una mañana entera de REFLEXIONES de Eduardo sobre V6**, todas medidas contra el
código y fichadas: el criterio de capas (sólo hardware/HAL deben diferir) · el inventario
(el REPL **triplicado en 220 KB**, `json_min` en **3 copias idénticas**) · unificar packs
(la diferencia cabe en **una función**) · uno o dos boots (la frontera existe pero no tiene
nombre) · el simulador con **disfraces** por familia · por qué unificar da fiabilidad (cero
bugs de memoria y de FS en dos días; los cinco estructurales, todos de lo repartido) · y el
coste (sistemas **3×** el lenguaje en trabajo real de V5, y el lenguaje ya no crece).
📌 Con una corrección suya que conviene recordar: yo medí líneas TOTALES y eso es el
artefacto, no el esfuerzo. Bien contado —365 commits desde `v4.0`— salen **17.389 líneas
tocadas en sistemas por familia frente a 7.234 de lenguaje**, y con **12.146 borradas**:
eso no es escribir, es **reformar**.

**⏭️ A LA VUELTA: arreglar lo que falte y PUBLICAR.** Lo pendiente ya no es probar:
1. 🔴 `appv1lsp` y `appv2` viajan **rotos** en el ZIP → arreglar, sacar, o a `samples/errores/`.
2. 🟡 El manual inglés se dejó siete secciones de V4.
3. 🟡 Escribir en `RELEASES.md` la instrucción de actualizar (ya la sabemos).
4. 🟡 **Rehacer el ZIP UNA vez** — han cambiado samples, IDE, `PackBurn`, tres imágenes y
   mucha documentación.
📌 Y dos fichas abiertas que **no bloquean**: el bug de LSP entre interfaces de módulo, y
el estado persistente que dejó una placa sin ejecutar nada (sólo lo curó reparticionar).

### 21-ago — H13 día 1: Pico y Metro CERRADAS, y cinco bugs que sólo salieron ejecutando

**Dos placas completas** (Puertas 1, 2 y 3), todo comparado contra la salida del host:
`ListGets` 21/21 en las dos variantes de RP2350 · `SqlDemo` 25/25 · `DaoDemo` 25/25 ·
`GenDemo` 10/10 · `PacksDemo` 32/32 · `SdDoc` 5/5 · `SdCard` los seis · `Bench` 105×.

**Y con eso quedan probadas las DOS cadenas de nativo**, que son independientes: el
`.npk` de ARM *precompilado y empaquetado* (SQLite, que nunca se había ejecutado) y el
`.mdn` *generado al vuelo* por el IDE para esta placa (`Bench`).

**Cinco bugs, y ninguno se veía leyendo código:**
- 🩸 **`#414`**: los cuatro builtins de packs vivían dentro de un `#ifdef BPVM_GUI`.
  Nacieron así, en el propio commit de la feature: **nunca habían funcionado en una
  placa sin pantalla**. Lo destapó `Packs.list()` muriendo en la Pico.
- 🩸 **Los blobs de `IO` y `Math` de la Pico, rancios desde el 17-ago**: `#415` los
  añadió al FIRMWARE y nadie los dio de alta en el GENERADOR. Lo cantó la placa
  (`/lib/IO.mod NO es el de esta imagen`) y lo preguntó Eduardo. Sólo afectaba a la
  Pico: el S3 y el STM32 sí los listaban.
- 🩸 **Ocho samples del ZIP que no compilaban** desde el cambio `any`→`Object`.
- 🩸 Un `test1.pack` de pruebas viajando en la distribución, y el README de `packs/`
  todavía en V4 (decía 26 módulos; son 27).

**El censo que lo destapó**: la batería cubre ~70 samples y en el ZIP viajan **278**.
Compilados los 278: 8 arreglados, 8 artefacto del arnés, 2 con bug real.

**🧠 Y la lección de método, que vale más que los bugs:** la cascada Java→C→placa **no
podía** cazar el `#414`, porque la VM-C de host se construye **con GUI** y los firmwares
sin pantalla no. El doble era más permisivo que el original. *«Lo que no funciona en C
tampoco en la Pico»* sólo se sostiene si el C que se prueba lleva la MISMA configuración.

**Dos cosas fichadas SIN resolver** (las dos con diagnóstico, ninguna con parche):
- La **sustitución por LSP entre interfaces de módulo** no funciona → `appv1lsp.bp` y
  `appv2.bp` viajan rotos en el ZIP. No es regresión de V5.
- Un **estado persistente deja la placa sin ejecutar NADA** —ni el `Hello` embebido—,
  sobrevive al flasheo, no lo cura formatear, y **sólo lo cura reparticionar**. Sin
  causa. Lo que falta medir está escrito en la ficha.

**Imágenes**: las cinco al día. El P4 y la DK2 **exentas por demostración** —compilar
`builtins.c` con `-DBPVM_GUI` antes y después del arreglo da un objeto byte a byte
idéntico—, así que sólo hubo que rehacer Pico, S3 y Nucleo.

**⏭️ MAÑANA: P4 y STM32**, flasheables directos desde `dist/firmware/`. El Nucleo
llegará con `/lib` rancio: es la ocasión —perdida ya dos veces— de medir cuál es el
arreglo MÍNIMO al actualizar de V4, que es la decisión 5 y hoy no existe en la
documentación. Las seis decisiones abiertas están numeradas en `H13_PRUEBAS_V5.md`.

### 20-ago (tarde) — H12 CERRADO, el bilingüe al día, y la tanda de construcción

**`H12` cerrado con sus ocho puntos.** Y lo caro no fue escribir: fue lo que escribir
destapó — la §13.2 de la referencia daba un ejemplo que YA NO COMPILA, la consola del IDE
se dejaba seis comandos, el ENV no tenia ni una clave escrita, `AOT_LIMITES` negaba el
`long` en `native` y el manual seguía diciendo que las colecciones usan `any`.

**Estructura, decidida por Eduardo:** la referencia en dos grupos (§13 lenguaje / §14
dispositivos), §15 Packs aparte, y **todo dentro de `referencia.html` salvo dos volúmenes
propios**: la GUI «porque es demasiado grande» y las BD «porque lo merecen».

**Y el bilingüe**, que yo no había mirado: `en/referencia.html`, `en/guia-ide.html`,
`en/index.html` y `en/basedatos.html` (no existía) puestos al día. Los ids son idénticos
en los dos idiomas, así que el guion de reestructuración valió tal cual.
⏭️ Queda deuda ANTERIOR: el manual inglés se dejó siete secciones de V4 (eventos,
sobrecarga, `dospasadas`).

**Tanda de construcción completa**, del mismo árbol: stdlib, los dos packs, los blobs, las
5 imágenes selladas en `dist/firmware/` y `BasicPlus-5.0-win.zip`. **Versión del IDE a
5.0.** El `.npk` de ARM regenerado desde sus fuentes — era el rancio.
📌 El rebuild dio **bytes idénticos** en todo salvo ese `.npk`: la construcción es
reproducible.

**Coda del día — cuatro notas de V6 sobre el ESP32-P4X.** Ninguna toca código: el freeze
sigue en pie y el ZIP sin rehacer. Eduardo avisa de que las placas que tenemos son P4 y
que por un defecto eléctrico van a **360 MHz** — los 400 dan problemas de consumo — y que
el **P4X** lo corrige y sí llega a 400. Al escribirlo fui al IDF y lo que yo había supuesto
resultó falso: **una sola imagen no puede cubrir los dos silicios**. Lo dice su Kconfig en
la primera línea (*«rev. <3.0 and >=3.0 is mutually exclusive»*) y el interruptor arrastra
hasta el reloj del bootloader. Política de Eduardo: **se mantienen las dos**, porque del P4
se siguen vendiendo placas y del P4X apenas hay. Y una buena noticia medida: el bootloader
ya rechaza la imagen equivocada en las dos direcciones, así que basta con nombrarlas bien.
Todo en `FICHAS.md` → «Aplazadas a V6».

**Coda del día — sesión de DISEÑO, ya sin placa.** Eduardo pide el listado de lo aplazado
y de ahí sale que estaba en **tres sitios**; se consolida en `docs/V6_BACKLOG.md`, que es
un ÍNDICE y lo dice en su cabecera para no crear una cuarta fuente. Al reunirlo aparece
una entrada rancia (los captadores tipados de `List`, hechos en V5) que se mueve a
«CERRADAS» conservando sus 89 líneas de diseño.

**Y tres asuntos nuevos, los tres nacidos de un problema observado y no de imaginar:**
- **La RAM del código nativo de un pack.** La tercera vía ya estaba elegida y corriendo
  (la arena del ENV); lo abierto es que hoy es singular y se llama `SQLite`, y que los
  `malloc` de la BIOS son chivatos que aún no reparten nada.
- **🩸 `native` en un MÉTODO se ignora en SILENCIO** — bug real, fichado. Y salió porque
  **Eduardo dudó de un diagnóstico mío**: yo dije *«no hay `this`, no se puede»* y él
  contestó que `miObjeto.miMetodo(...)` es `miMetodo(miObjeto, ...)`. Tenía razón: el
  emisor ya emite `MemberAccessExpr`; lo que falla es un barrido que no desciende a las
  clases (`AotCEmitter.java:259`). **El trabajo era mucho más pequeño de lo que yo dije.**
- **El lazo de LVGL y los cabos del AOT**, que resultaron ser la misma enfermedad.

📌 **Y el censo que más va a doler**: `AOT_LIMITES.md` nombraba **5** límites y son **~24**
— la lista corta engañaba porque el emisor tiene rechazos genéricos, así que sólo estaba
documentado lo que alguien se encontró de frente. De ahí sale el **hito AOT de V6**
(encargo de Eduardo: arreglar el método, `double`, y los statements sencillos).

⏭️ **Pendiente de decidir**: `notas/` está en el `.gitignore` y ahí viven **473 líneas** de
diseños de V6 sin versionar — y por esa misma vía ya se perdió el `SqlDemo.bpbuild`. Los
documentos nuevos se han creado en `docs/` a propósito.

**⏭️ MAÑANA: H13 en placa**, con `docs/H13_PRUEBAS_V5.md`. Dos días (viernes y sábado),
partido por puertas. La Puerta 1 —el ABI de `Comparable` en las tres familias— es la que
bloquea, y la Metro es obligatoria porque su `.npk` no se ha ejecutado nunca.

### 20-ago — H12 en marcha: tres documentos, y la referencia reordenada

**Escrito y verificado:** `docs/TARJETA_SD` y `docs/BASEDATOS` (este ultimo, del dia
anterior), mas el apartado de packs. Los ejemplos de la SD se validan con
`samples/SdDoc.bp`: las DOS VMs, salida identica.

**🩸 Y otra vez, escribir los ejemplos los rompio** — cuatro hallazgos, el ultimo gordo:
`chr(10)` NO EXISTE (se me habia inventado; son los escapes del lexer); `listDir`
devuelve `string[]`; un array NO tiene `.length()` (eso es de los objetos, los arrays van
con `for..in..next`); y **`listDir` no esta en la VM-C**, o sea que funciona en el PC y NO
en la placa. Es el UNICO verbo de fichero que falta. Documentado como limitacion.

**📐 Estructura, decidida por Eduardo:** la referencia mezclaba librerias del lenguaje con
las de hardware. Ahora son **§13 Biblioteca del lenguaje** y **§14 Librerias de
dispositivos** (con la SD como 14.17 y la «Politica HW = clase OO» encabezando el grupo),
**§15 Packs** aparte —«no es exactamente una libreria»— y 16/17 corridas. La regla final:
**todo dentro de `referencia.html` salvo dos volumenes propios**, la GUI «porque es
demasiado grande» y las BD «porque lo merecen». Verificado por script: 34 numeros
cambiados, referencias en prosa reescritas y contrastadas contra su destino, cero anclas
rotas, indice regenerado — y se corrigio un `<ul>` sin cerrar que ya traia el fichero.

**🩸 Fallo mio del dia:** escribi los dos primeros documentos en Markdown cuando los
manuales del proyecto son HTML. Convertidos reusando la cabecera y la hoja de
`referencia.html` (mismo md5, comprobado).

**🔎 Y una correccion de Eduardo que valia oro:** el apartado de packs contaba flojo su
razon de ser. No es que ahorren flash: **ahorran RAM**, porque el modulo se ejecuta EN EL
SITIO desde la flash. Al verificarlo salio que el comentario de `bpvm.c:472` decia «hoy
con copia — el XIP es la tanda 2» y estaba RANCIO: la tanda 2 ya esta hecha
(`bpvm_loader_load_xip`, «codigo en sitio»). Corregido el comentario y reescrito el texto.

**⏭️ Idea de Eduardo para V6, anotada con su diseño:** que el IDE no suba dependencias que
el dispositivo ya tiene, y que **lo decida el dispositivo resolviendo COMO EL CARGADOR**
(da igual que este en `/app`, `/lib`, `/sys` o en un pack) — asi no hay dos buscadores que
se desincronicen. Y al mirarlo: **la mitad ya existe**, `putIfChanged` ya compara por CRC;
lo que le falta es preguntar por la resolucion en vez de por la ruta destino.

**⏭️ Sigue H12:** los cambios del IDE, `Core` (envoltorios, captadores), **las variables de
entorno** —que hay que CENSAR antes de escribir, hoy nadie puede enumerarlas—, la puerta
(QUICKSTART y los dos README) y las notas de version. Y una pasada por documento
preguntando *¿que afirma esto que ya no sea verdad?*: `AOT_LIMITES.md` niega el `long` en
`native` y eso dejo de ser cierto el 16-ago.


### 19-ago (tarde) — H12 arranca, y documentar destapa que el ORM no funcionaba

**Lo mas importante del dia no es lo que se escribio, es lo que salio al escribirlo.**
Al ir a documentar las BD hubo que ejecutar los demos, y NINGUNO compilaba. Tres
causas, ninguna la que yo suponia:
1. **el GENERADOR de DAO emitia codigo roto** — `return this.uno(...)` sin el downcast,
   y `uno()` devuelve `Object` desde #389;
2. **`List` era ambiguo en TODO programa del ORM** — al importar `Core` el semantico
   aliasa `List`, y la interfaz del modulo la reexporta COMO SUYA; quien importa
   `SQLite` y `Orm` veia dos simbolos para una clase;
3. **y lo que mas costo NO era un bug**: `samples/` tenia SEIS copias fosiles de la
   stdlib del 10-11 de JUNIO, ninguna en git, ganando por orden de busqueda.
Los tres demos **compilan Y CORREN** (`240b400d`, `7854b61f`, `6bb78c6a`).

**🩸 Y dos cosas que borre yo por la manana y hubo que recuperar:**
- los `.bpbuild` de los demos de BD — reconstruidos y ahora EN GIT (`13a78f9`);
- **`packglue.c`**, el doble de host de la tabla BIOS, que no estaba en ningun sitio.
  Reescrito desde `bios_pico.c` (`7854b61f`). Eduardo: *«un source de VM-C que no esta
  en su sitio y tampoco se ha subido al Git no es un fallo, son como minimo 2»*. Son
  TRES: fuera del arbol, fuera de git, y nadie lo detectaba — el censo que pidio
  encontro OTRA ruta muerta en el mismo target (`../bpstdlib/SQLite.bp`, rota desde
  el 14-ago).
🩸 **La leccion, y es de metodo**: al censar lo unico de `notas/` busque `.c`, `.h`,
`.sh` y `.link` — y NO busque `.bpbuild`. Mire las fuentes y me olvide de los ficheros
que las orquestan.

**Adelantado de V6 por decision de Eduardo** (*«nos ahorramos un monton de problemas»*):
**los captadores tipados de `List`** — `getInteger/Long/Float/Double/String/Boolean`
(`4a003aeb`). Como BP no tiene test de tipo, las conversiones son METODOS VIRTUALES en
`Comparable`. La opcion buena (`className()`) NO CABIA: el descriptor de clase no
guarda el nombre, asi que exigia cambiar el formato — a V6. Verificado con
`samples/ListGets.bp`: **las dos VMs, salida identica byte a byte**, y bateria H13 sin
regresion. ⚠️ Anadir 5 metodos a `Comparable` CORRE SUS RANURAS: hubo que reconstruir
la libreria de SQLite, o `DaoDemo`/`GenDemo` fallaban en ejecucion aunque compilaran.

**Escrito: `docs/BASEDATOS.md`** (`d614d27f`), las 13 secciones del indice de Eduardo.

**⏭️ Sigue H12**, con la lista y el orden en `FICHAS`: la **tarjeta SD** primero (el
hueco grande, tres hitos sin una sola mencion), luego **packs**, **IDE**, **Core**, la
puerta y las notas. 📌 Y el encuadre de la SD, precision de Eduardo: es capacidad de la
**IMAGEN** (RP2350 y ESP32, en STM32 no), no de la placa — en una Pico 2 se cablea un
lector y funciona.


### 19-ago — la limpieza, y el rescate que casi no se hace

**85 MB fuera**, en commits separados: `3636ff0` (rescate), `f2edd80` (borrado),
`04a3bd0` y `a250bd3` (registro). Con eso la limpieza queda CERRADA entera y el
cierre de V5 pasa al siguiente paso, `H12` documentar.

**🩸 Lo que importa del dia: la ficha se equivocaba en el punto critico.** Decia que
en `notas/` *«lo unico NO duplicado son los .elf de SQLite»*. Falso. Habia **26
fuentes y scripts que no existian en ningun otro sitio del repo**, y entre ellos
`vfs_bp.c` —el VFS sin el cual `sqlite3_initialize()` falla EN SILENCIO— y los dos
`build_sqlite*.sh` con las banderas que costo sangre averiguar. Enfrente,
`bpstdlib/sqlite/nativo/*.npk`, binarios versionados **que se publican en V5**.
Borrar a ciegas los habria dejado sin receta para siempre.

Rescatado a `bpstdlib/sqlite/nativo/src/` (77 KB), que es donde Eduardo decidio el
14-ago que viviera todo lo del pack — el `LEEME.md` de esa carpeta YA decia que se
creo porque *«el pack no se podia reconstruir desde un clon limpio»*: el traslado se
habia hecho a medias, se movieron los `.bp` y los binarios y el pegamento en C no.

**🔬 Y se probo, no se supuso**: el `.npk` de RISC-V se reconstruye **BYTE A BYTE**
desde su nueva ubicacion (618.168 B, y sus numeros son los que canta el P4 al
arrancar). Ese control es lo que dice que la receta vale.

**Y el control saco un hallazgo**: el `.npk` de **ARM no se reproduce** — 8 B mas y el
punto de entrada corrido (0x114 vs 0x10c). Se genero antes de algun cambio del
pegamento y nadie lo regenero. No se toca en freeze: va a `H13`.

**Lo demas que cayo:** el parche privado (comprobando antes que no aplica), el
worktree de 28 MB —quitado con `git worktree remove`, no con `rm`— y **un segundo
worktree que la ficha no listaba** (husk de la sesion `25fabe6b` en Temp);
`V4_SAMPLES_ROJOS.md` marcado como HISTORICO y metido en git; los artefactos sueltos
y 27 `.slots`, que ademas pasan a `.gitignore` (los de `bpstdlib/` y `bpdevices/`
siguen versionados: son la distribucion).

**`dist/` decidido** (Eduardo): es una salida, se reconstruye antes de publicar y el
zip va a GitHub aparte. Ignorado, salvo los tres MANIFIESTOS de 4 KB, que son el
unico registro en el repo de que binarios salieron.

**⏭️ HALLAZGO PENDIENTE, para H13:** `bpstdlib/Str.{mod,dbg,slots}` llevan desde el
18-ago regenerados y SIN COMMITEAR (los cambio `#446`: Str paso a fachada, por eso
encoge 5.789 -> 5.578 B), y `packs/Stdlib.pack` es del 15-ago, anterior al cambio.
De las 11 copias de `Str.mod` del arbol solo la de `bpstdlib/` esta en git, asi que
esta acotado — pero es el desfase de siempre.

**Y `C:\tmp` vaciado**, encargo del mismo dia: **157 MB -> 0**. 146 MB eran tres
arboles de build; los 12 MB restantes NO eran temporales (el ELF known-good de la
Discovery, un `.bak` de `gpio_stm32`, dos experimentos con `.uf2` y 42 sondas `.bp`
unicas) y se miraron uno a uno: superados o ya registrados, ninguno referenciado.
📐 **El criterio de Eduardo**: *«no tiene sentido tener algo que queremos mantener en
un directorio temporal»*. Es lo contrario del caso de `notas/` del mismo dia, y por eso
alli se rescato y aqui no.

**Pendientes de V5: 7.**


### 18-ago (tarde) — #439 probada EN PLACA, y los 32 MB del P4 que no pudieron ser

**Lo gordo: `#439` CERRADA, con la prueba en el P4.** `CuelgaLog.bp` → 4 min 30 s
girando en un `while true` → `kill` → `reset` del IDE → al volver, `log: RAM
SUPERVIVIENTE (lineas de ANTES del reset)` y la sesion entera detras, con los 269
segundos de silencio que son el cuelgue. El arreglo (region del log en RAM que el
arranque no borra) fue idea de Eduardo y evito el plan anterior, que era flush por
linea: 4 KB de erase+program POR LINEA se come el sector en minutos.

**Costo tres intentos y las dos trampas fueron de instrumento, no de codigo:**
1. la linea que dice de donde viene lo cargado solo existia en `pico/main.c`, asi
   que el P4 no podia contestar la pregunta (`be0a86e` la lleva a las 4 imagenes);
2. luego dos resets salieron `arranque en frio` **sin que eso significara fallo** —
   «esta roto» y «has usado el reset equivocado» explicaban el log igual de bien.
   Lo desempato el `resetReason` del INFO, que YA ESTABA: decia `power-on`. En
   ESP32 el boton RST tira del pin EN y cuenta como arranque en frio; en el RP2350
   el pin de RUN si conserva la RAM. Anotado como **L15** en `PENDIENTES.md`.

**Abierta `#452`** de camino —con un RUN vivo la placa solo atiende `HELLO`/`KILL`,
asi que el `RESET` del wire no llega— **y aplazada a V6 el mismo dia** (Eduardo:
*«ahora sabemos apañarnos y a los usuarios no les afecta»*): el rodeo es `kill` +
`reset`, documentado en `PENDIENTES.md` L15. De paso salio que el censo por
familias estaba mal: son CUATRO sitios, no tres — el S3 y el P4 comparten REPL y
el simulador (`tools/bpvm_sim.c:679`) lleva el mismo filtro. Censar por la
primitiva (el mensaje) y no por el nombre, otra vez.

**Los 32 MB del P4: implementado, probado en placa, revertido.** El bootloader
usaba 16 de los 32 MB; ampliarlo funciono para el FS y **rompio los packs**, y no
por tamaño (la hipotesis que Eduardo tumbo con una zona de 6528 KB): el cache de
flash del P4 direcciona a **24 bits**, o sea que nada por encima de 0x1000000 se
puede mapear. El diagnostico quedo cerrado y el rediseño (packs debajo de 16 MB,
FS encima) va a V6 — `9d0589b`. La revision quedo con el pack sano: sqlite 3.53.4,
vfs `bp` registrado, `rc=0`.

**Y lo demas que cayo:** `#427` pto 8 (el `hello_mod.c` ya sale de un GENERADOR y
del mismo fuente; las 3 copias muertas, fuera) · `#441` la mitad de la arquitectura
(el IDE compara el `arch` del `.mdn`, no solo la fecha) · `#451` SyncList a
Collections · el «pwm» del arranque y del INFO ya dicen su unidad.

**`#441` cerrada por la mitad, por decision de Eduardo:** la arquitectura entra en
V5 (`9fcff33`); la huella de los FLAGS se va a V6 porque pide cambiar el formato
del `.mdn` y *«ahora no vamos a modificar formatos»*. La cabecera no tiene campo
libre, asi que meter el hash obliga a subir `version` y a tocar el lector del IDE y
el de las cuatro imagenes a la vez — eso se hace al empezar una version, no al
cerrarla. **Con eso, pendientes de V5: 7** (3 tecnicas + 4 tareas de cierre). La septima cayo
al preguntar Eduardo si «la polaridad de Q1» era la luz de la pantalla: no —es el
MOSFET del rail de la SD por GPIO45— y resulta que no habia nada abierto. Q1 y
`pwr` son el mismo transistor, y la propia linea de la ficha ya declaraba cerrada
la polaridad de `pwr`. Error de redaccion mio al archivarla. Contestada por
triplicado: el analisis del transistor, el codigo (`blk_sdmmc_p4.c:79`) y cada
arranque (`pwr 45 (activo bajo)` + `sd: montada en /sd`).

**⏭️ COMO SE CIERRA LO QUE QUEDA — decision de Eduardo (18-ago):** *«lo que queda
pendiente se puede mirar en la fase final donde haremos mas pruebas»*. O sea que las
tres tecnicas que quedan (`#379` verificar que era `#398`, `#408` medir los dos
cuellos, `H2-P5` exFAT y superfloppy) NO son trabajo suelto: entran en la tanda de
pruebas del cierre. Y ahi cabe tambien la cola de `#439` —repetir la vuelta en Metro
y STM32— que quedo escrita dentro de la ficha ya cerrada; el codigo esta verificado
en el `.elf` de las cuatro imagenes y probado en placa en una.

**Y una que faltaba en el registro:** al resumir Eduardo el cierre como *«limpieza,
documentar y pruebas finales»*, resulta que **documentar no tenia linea en FICHAS**.
Medido: `sqlite` y `@BD` salen CERO veces en toda la documentacion de usuario
(manual, referencia, cheatsheet, QUICKSTART, los dos README) y solo aparecen en
ficheros de trabajo internos. O sea que los dos hitos mas visibles de V5 —el ORM y
SQLite en un pack— no existen para quien lea la documentacion. Ficha abierta en
«Cierre de V5» con lo que hay que escribir y con lo que ya esta bien (referencia.html
§13.3/13.4 no se quedo rancia: su «SyncList extiende List» paso a ser cierto).

**🧊 CODE FREEZE V5, desde hoy.** Eduardo: *«a partir de ahora, codigo congelado,
solamente se arreglan bugs»*. Banner en la cabecera de `FICHAS.md`, con el criterio
de V4: la pregunta no es «¿merece la pena?» sino **«¿esta roto?»**. Lo que no lo
este, a V6.

**El plan de cierre, en este orden** (decision de Eduardo, 18-ago):
1. **Limpieza** — manana 19-ago, ANTES de documentar (las 5 carpetas de `notas/`,
   los restos del arbol, la decision sobre `dist/`).
2. **`H12` — documentar V5.** Con numero de hito, por decision suya.
3. **`H13` — las pruebas finales.** Idem. Agrupa `#379`, `#408`, `H2-P5` y la cola
   de `#439` (Metro y STM32) SIN duplicar su texto: cada ficha sigue en su seccion.
4. **Publicar.**

De paso salio una contradiccion DENTRO de la fuente de verdad: la tabla de hitos
decia «H10 y H11 cerrados» y dos lineas despues «Queda H10 (IDE)». Comprobado
mecanicamente —sus 9 fichas estan todas tachadas— y corregido. La tabla ahora llega
hasta H13.


### 18-ago — el dia de las listas: 8 fichas cerradas en cadena, corpus 29→37

**El hilo del dia** (todo salio de una decision de Eduardo: cancelar `Box`,
sobrecargar `add`, y «la list sintetizada deberia desaparecer»):

- **#442** — un literal de array guardaba SIEMPRE 4 B/casilla (`long[]` daba 0
  EN SILENCIO). La trampa: `string` es PrimitiveType Y referencia — se vio
  desensamblando, no por el sintoma.
- **#443** — `newObjArray`/`growObjArray`: alias publicos de builtins que ya
  existian (el id es ordinal(): entrada nueva = id que ninguna VM conoce).
- **#444** — `CHECKCAST_EXT` (0xB0): el downcast cross-module reventaba el
  compilador. Reuso la subseccion de fixups de `TRY_BEGIN_EXT` → ni el formato
  ni los loaders cambian.
- **#447** — el cast a la PROPIA clase reventaba (descriptor se registra en
  endClass). Tapaba que **Collections.bp no compilaba desde el 16-ago** — otro
  artefacto rancio. Y el fat-jar del frontend empaqueta miVM: `install` sin
  `clean` deja el jar viejo (trazas con lineas que no cuadran = esa señal).
- **#446** — envoltorios+Comparable a Core; `formatDouble`/`longToString` con
  ellos (Str queda de fachada). El atajo `"" + x` NO valia: 1E12 vs 1000000000000.
- **#450** — el compilador YA NO sintetiza List/SyncList/OwnerList: estan en BP.
  Core se importa SIEMPRE (Core.mod 2.576→8.306 B, preinstalado). Firmware Pico
  enlazado con la stdlib nueva.
- **#449** — mi «OwnerList no se puede escribir en BP» era FALSO (Eduardo lo
  olio): `var owner items: Object[]` emite SET_FIELD_OWNER, y el owner local da
  el FREE_REF. Guardian de fin de RUN: 0 bloques sin liberar.
- **#451** — `super.metodo()` cross-module: la pista la dio Eduardo («el ctor de
  super SI se llama solo») → factorias `__cls_m_<Cls>_<metodo>`, aditivo.
- **Wrap8Test** — no era el Map: el sample concatenaba un Object-con-cadena a
  pelo (en V4 imprimia el HANDLE en silencio; #389 lo hizo visible).

**⏭️ A LA VUELTA DEL DESCANSO (lo dijo Eduardo):** el punto 1 (`hello_mod.c`
del STM32, mecanico) y **#439** con su hipotesis ya registrada en la ficha:
los logs se quedan en RAM (confirmado), y «si queremos el log como mecanismo
de depuracion de verdad, directos a la flash — falta un flush()». Primero
medir el coste del flush por linea en cada familia (la Pico borra sectores de
4K), luego decidir el modo.

**Pendientes de V5: 10.** El mas barato despues: mover SyncList a Collections
(desbloqueado; dos intentos de cirugia de texto fallaron, hacerlo con calma).

### 17-ago (tarde) — el cuelgue del P4, y UNA fuente de verdad

**Lo gordo: `#440`, verificada en el P4.** Toda `native` que tocara un literal
de cadena colgaba la placa. No era el GC ni la memoria, como parecía: era el
**modo de direccionamiento**. RISC-V compilaba con `-fno-pic` y el modelo por
defecto, que llega a sus datos metiendo la dirección de ENLACE como constante
(`lui`+`addi`); enlazar a `-Ttext=0` deja relativos los SALTOS pero no los
DATOS, y como el `.mdn` se carga donde caiga, eso es un puntero salvaje →
cuelgue mudo. ARM nunca lo sufrió (`-fpic`, remata con `add r1, pc`). Arreglo:
`-mcmodel=medany`. Con él cayeron de paso **la pata del P4 de `#430`** y
**`#302` paso 3 en la segunda arquitectura** (`AotGcRt`: 10.000 vueltas,
`malos: 0`).

**Cómo se localizó, que es lo reutilizable:** la ESCALERA (`NatEsc.bp`, en el
repo). Una `native` por peldaño, cada una exigiendo una cosa más por debajo,
con un print antes y después. Una sola corrida da el punto de ruptura sin ir
pidiendo variantes de una en una. `NatMin` (sumar enteros) pasaba y el escalón
2 (devolver un literal) moría → el thunk estaba sano y lo roto era tocar datos.

**`H2-P5`: el camino SDSC, probado sin tarjeta SDSC.** Eduardo: *«no tengo
tarjetas de 2G ni voy a tener»*. Pero lo que daba miedo era una cuenta
(`arg = alta_cap ? lba : lba*512`), y eso es aritmética pura → `make test-sdsc`,
con control por caso y rojo verificado.

**Y el cambio de fondo: `docs/FICHAS.md` es LA FUENTE ÚNICA**, y ya está en
git. Eduardo: *«me estoy volviendo loco con cosas que aparecen y desaparecen»*.
Estaba medido: de las 51 fichas que citaba este documento, **49 eran una
segunda copia**. Ahora `ESTADO` es sólo este diario, `PENDIENTES` sólo
limitaciones de cara al usuario, y `CLAUDE.md` apunta a `FICHAS`.

⚠️ **Tres errores míos de registro, todos del mismo tipo**, y conviene tenerlos
presentes porque volverán: (1) moví «el árbol trunca mudo» de `PENDIENTES` a
`FICHAS` sin contrastarlo — era `#425`, cerrada ese mismo día; (2) di los
cuatro rojos del censo como pendientes cuando 1, 2 y 3 se cerraron el 16-ago
(`ec81afc`) — no los vi porque viven DENTRO de una ficha cerrada, invisible a
un barrido; (3) repetí que «todas las medidas llevan `-Og`» cuando eso sólo
vale hasta el 16-ago a mediodía. Los tres los cazó Eduardo leyendo la lista.
**La lección: al mover algo de sitio, contrastarlo con lo cerrado; y lo que sea
trabajo de V5 no puede vivir dentro de una ficha cerrada.**

**Sin cerrar:** `#379` — probando los 5 ciclos en el P4, **el Stop cuelga**. La
hipótesis de que era `#398` disfrazado NO se sostiene. La pregunta que parte el
problema sigue sin hacerse: **¿está colgado el device o el IDE?** (pedir `Info`
por la consola con el cuelgue puesto). Se paró ahí a propósito.

⏭️ **MAÑANA: LA TIJERA.** Decisión de Eduardo al ver la lista: *«esto sigue
enrevesado, demasiadas cosas; mañana metemos la tijera a ver si podemos cerrar
unos cuantos»*. O sea que el marco del día siguiente es **cerrar y descartar,
no abrir** — y cancelar sigue siendo un resultado válido. Quedan **11** de V5
(barrido mecánico de `FICHAS`, sin V4 ni V6) + 4 tareas de cierre. De los 11,
sólo dos son desarrollo nuevo (`#438` `Box` y `List`→`Object`, que son la misma
conversación); el resto es cerrar cosas, y `hello_mod` es el más mecánico.

### 17-ago — H10 ENTERO, el grupo B mecánico, y una ficha que no existía

**H10 cerrado, las siete** (`#425`, `#437`, `#435`, `#436`, `#394`, `IDE-7`,
`#395`) — detalle en el plan de cierre, arriba. Y del **grupo B** lo que se podía
hacer sin conversación previa: `#431`, `#429`, `#412` (a V6 con su diseño) y
`GAP-4`.

**Lo más aprovechable del día, por si sirve de aviso:**

- **`GAP-4` no existía.** Decía que la notación científica de `double` en la VM-C
  estaba pendiente y que el invariante sagrado podía estar roto en magnitudes
  extremas. La medida dice que **no**: 22 casos byte a byte, incluidos los dos
  lados de cada frontera y los extremos del tipo. La ficha **nació de leer mal
  una palabra** — el comentario dice *«TODO en aritmética IEEE determinista»* y
  ese `TODO` es el **castellano** («todo ello»), no el marcador inglés de
  pendiente. `SciPar.bp` queda en el corpus de paridad: **29 PASS**.
- **`#425` era el mismo mal que `#433`**, y estaba en CUATRO sitios, no tres: el
  cuarto era el micro simulado, que es con quien habla el IDE en modo Sim. Lo
  cazó el grep de quién-más-lo-hace; de memoria se habría escapado.
- **`#429` enseñó algo al probarlo**: la primera versión ponía las fechas con
  precisión de minuto, y como el desfase típico es de segundos las dos salían
  IGUALES — un aviso cuya evidencia no se ve se lee como falsa alarma. Con
  segundos.

**Herramientas nuevas que se quedan:** `FrmBoardShot` y `EnvDialogShot` (pintan
una ventana o un diálogo del IDE a PNG sin display ni placa; la primera cazó un
botón sobre un `GridLayout(1,1)` que compilaba y rompía el layout), y
`make test-listtrunc`.

**⏭️ AL VOLVER — orden decidido por Eduardo:** *«prefiero hacer las pruebas en
placa y después meternos con `List` y `Box`, que es desarrollo nuevo.»*

1. **La sesión de placa B**, con su guión ya escrito en
   `notas/SESION_PLACA_B.md`: `SciPar` (la pata de placa de GAP-4, 5 min y sin
   reflashear), `#379` (el wire tras el Stop — primero SABER en qué placas),
   `#362` (recursos del pack, verde en host y nunca en placa), `#408` (los dos
   cuellos, con las fotos ya cambiadas) y `#415`.
2. **`List`/`SyncList`/`OwnerList` → `Object` y `#438` (`Box`)**, que son la
   MISMA conversación y tienen decisiones que son de Eduardo: el nombre, si
   distingue «vacío» de `null`, y cómo se saca un escalar — la asimetría de los N
   getters que ya salió al diseñar `File`/`TextFile` para V6. Conviene
   resolverla igual en los dos sitios.

Estado del repo: todo committeado, **sin push**. Imágenes al día (Metro 17-ago
15:57 con `#425`; el P4 se construye y sale a `-Os`). Toolchain reconstruida:
frontend, miVM y el fat-jar del IDE.

### 17-ago (tarde-noche) — LA SESIÓN DE PLACA COMPLETA, y un bug de memoria de V4

**Ocho fichas resueltas o verificadas en placa**: `#389`, `#381`/`#428`, `#430`,
`#302`p3, `#422`, `#418`, `#433` y `#424`. Grupo A cerrado. Pendientes de V5:
**16** (veníamos de 21 esa mañana, y de 45).

**Lo más importante, en la lectura de Eduardo: `#430` es un bug de V4 y seguimos
CONSOLIDANDO las VMs.** La tabla de handles nació en la migración de V4/H1 y el
disparo del GC por volumen es de `#357`: el eje de presión que faltaba —los
SLOTS— llevaba ahí desde entonces, latente. Solo se manifestó al coincidir las
tres condiciones (muchos objetos chicos + heap grande + SRAM pequeña), y se
manifestó como lo peor posible: un cuelgue mudo. Ahora es una colecta a tiempo
y, si de verdad no hay sitio, un OOM atrapable. Matiza
`v4-es-la-base-lo-siguiente-es-aditivo`: la base de V4 sigue asentándose.

**Lo demás de la tarde**, por si hace falta el hilo: `#430` se acotó con el test
de desplazamiento (el gemelo que gasta el doble murió a la mitad de camino);
`#302`p3 quedó verificado en ARM en cuanto `#430` dejó de estorbar; `#424` se
midió en vez de suponerse —el tope de 50 ms no disparaba nunca y el lazo no
estaba ocupado sino dormido— y de intentar leer esa medida salió `#433`, el log
común que truncaba en silencio mientras la Pico llevaba anillo desde `#326`.

**⏭️ PRÓXIMOS DÍAS: H10 ENTERO, hasta terminarlo** (decisión de Eduardo). Es el
grupo C: `#425` (el árbol del IDE trunca en silencio), `#394` (subir eligiendo
destino — ojo, el Upload ya respeta la carpeta del árbol desde hoy, así que la
ficha puede haber encogido), `#395` (botón `DAO build`), `IDE-7` y la clase
`Box`. ⚠️ **Eduardo trae cambios que añadir a esas fichas: escucharlos ANTES de
planificar el bloque**, que el enunciado puede crecer. Después de H10 queda muy
poco.

Estado del repo: todo committeado, **sin push** (norma: nada a GitHub hasta
cerrar la versión). Imágenes al día: Metro `bpvm_pico.uf2` (17:53) y P4 a `-Os`
con el instrumento del lazo dentro (gated por `log=1`). El IDE, fat-jar de las
17:54. Y desde hoy **ESP-IDF se usa desde aquí** (`C:\esp6.0.1\esp-idf`): el
P4 y el S3 se compilan antes de pedir un flasheo.

### 17-ago (tarde) — LA SESIÓN DE PLACA: la Metro entera, y un cuelgue cazado

**Seis fichas verificadas EN PLACA en una tarde**: `#389` (CastRt, 9 líneas byte
a byte con el opcode nuevo), `#381`/`#428` (LongNat: 8/8 thunks, el `.mdn` del
pipeline enlazado corriendo en ARM), `#430`, `#302` paso 3, `#422` (los dos
caminos) y `#418`. **La Metro queda COMPLETA.**

**Lo gordo fue `#430`**, y no era lo que parecía. La Metro se colgaba muda
ejecutando el sample que venía a probar el escaneo de la pila C — o sea, el
instrumento moría antes de medir. Eduardo lo acotó en tres pasos, ninguno
teórico: quitar el `native` (murió igual ⇒ el nativo, exonerado), llamar al
`gc()` a mano (terminó limpio ⇒ el GC va bien, nadie lo llamaba) y **el test de
desplazamiento**: un gemelo que gasta el doble de handles por vuelta murió a la
mitad de camino. La causa: el disparo del GC contaba VOLUMEN y no SLOTS, la
tabla de handles sólo doblaba, y su salto a 65536 pide 512 KB **de SRAM** (las
tablas salen del malloc de plataforma, no del heap de la VM, que está en PSRAM).
El malloc fallaba y el hook de FreeRTOS parpadea para siempre: un cuelgue, no un
error. Arreglado en las DOS VMs con las tres ideas de Eduardo — la marca al
final de la tabla, el tope por puerto (Pico: 16384 slots) y la **excepción
prefabricada** en el prólogo del RUN, que hace que quedarse sin memoria para
contar el error deje de ser una muerte muda. Ficha completa en `docs/FICHAS.md`.

Herramientas arregladas por el camino: el **Upload del Explorer** subía siempre a
`/app` (sin eso, `#422` y `#418` no se podían ni probar), y **miVM escribía su
diagnóstico de GC por stdout** — cualquier programa que colectara rompía el
invariante en Java.

**📊 LA CUENTA (17-ago, contada del fichero): 17 pendientes de V5** —
12 fichas numeradas + 2 encargos sin número (`List`→`Object` y `Box`) + 3 tareas
de cierre. Veníamos de **45**. Hoy: cerradas 4 (`#430` nació y murió el mismo
día), abierta 1 (`#431`).

**⏭️ AL VOLVER — plan de Eduardo:**

- **Hoy se cierra con `#424`** (los eventos del GUI del P4) y con eso basta:
  cierra el grupo A entero.
- **Otro día, el bloque del IDE** (grupo C / H10: `#425`, `#394`, `#395`,
  `IDE-7` y `Box`). ⚠️ Eduardo trae cambios que añadir a esas fichas — leer lo
  que diga ANTES de planificar el bloque, que el enunciado puede crecer.

Detalle de los dos primeros pasos:

1. **El P4** (lo único que queda del grupo A): remedir `#424` (los eventos del
   GUI, **sin tocar nada primero** — la foto pudo cambiar sola con `#398` y
   `-Os`; si siguen lentos, la prueba de una línea está localizada en
   `gui_display_dsi.c:546`) y anotar las **nuevas líneas base a `-Os`**
   (arranque, árbol, SD: las de estos días eran a `-Og` y ya no valen). El
   guión, en `notas/SESION_PLACA_A.md`.
2. **Grupo B**, ya de escritorio. Estaba empezando `#429` (que el IDE detecte su
   propio compilador rancio: nos costó tiempo dos veces el 16 y el 17-ago). El
   sitio está localizado — `lexer-java/.../Version.java`, que ya sabe de dónde
   salió y de cuándo es; falta la comparación contra el `basicplus-frontend.jar`
   del árbol y el aviso. Lo demás de B: `List`/`SyncList`/`OwnerList` de `any` a
   `Object`, `#412`, `GAP-4` y el **`#431` nuevo** (miVM busca las deps en el
   CWD en vez de junto al `.mod`, y revienta con stack trace de Java).

Estado del repo: todo committeado (6 commits, `1ed8ebc`..`17df39a`), **sin push**
(norma: nada a GitHub hasta cerrar la versión). Imagen de la Metro al día
(`bpvm_pico.uf2`, 17:53) con la anterior guardada como known-good; fat-jar del
IDE de las 17:54 y verificado por conducta.

<!-- Fecha — quién — resumen del traspaso. La entrada más reciente arriba. -->

- **2026-08-17 (2) — ✅ `#389` CERRADA EN HOST: el downcast de `Object` LANZA**
  (`05acc0d`) — el último bug conocido del lenguaje. Opcode nuevo `CHECKCAST`
  (0xAF, las dos VMs): mira sin consumir, null pasa, y el error NOMBRA el tipo
  esperado (el nombre viaja como literal internado — el descriptor no lo
  lleva). De hacerlo salieron dos arreglos más: INSTANCEOF de la VM-C leía el
  class_ptr a ciegas (paridad latente con miVM, que ya validaba) y el despacho
  virtual sobre un Object-con-cadena daba el 504 disfrazado — ahora lanza
  atrapable, mismo mensaje byte a byte. Y una trampa cazada por el reproductor:
  los literales de cadena viven en la región de datos SIN cabecera, y el primer
  intento los rechazaba en `string(o)`.
  Verificado con `CastRt.bp` (9 casos, salida idéntica en las dos VMs), toda la
  batería, y el IDE reconstruido. Falta placa (reflashear: opcode nuevo).
- **2026-08-17 — 🟢 `#302` paso 3 HECHO EN HOST: el test rojo del día anterior,
  VERDE con el diseño de Eduardo.** Escaneo conservador de la pila de C en vez
  del shadow stack: el GC recorre `[su frame .. el techo que apuntó el guard del
  thunk]` con el mismo `mark_recursive` conservador de la pila BP, y un `setjmp`
  vuelca los registros (Boehm). Tres sitios: un campo TLS en el callctx, el
  paso 2d del marcado, y la sincronización de `tc->sp` al entrar al thunk.
  **Por qué esta forma gana**: cero emisor, cero ABI (los `.mdn` grabados quedan
  protegidos sin regenerarlos), cero coste sin AOT, y miVM ni se entera — la
  paridad sigue 28/0/0. Medido: ~180 palabras por colecta, y el rastro marca
  `1 refs` justo en la colecta que antes reciclaba el intermedio.
  El test queda de guardián permanente (`make test-aotgc`); en placa falta ver
  el rastro `pila C del native` con `log=1` — va con las pruebas finales.
  **Con esto, el bug conocido que bloqueaba el cierre de V5 está arreglado.**
- **2026-08-16 — 🔴 `#302` paso 3: EL ARGUMENTO DEL APLAZAMIENTO, REFUTADO CON
  TEST.** Se difirió con «el native corre síncrono sin GC asíncrono y F2 no
  compacta», y las dos patas caducaron en V4: el GC corre DENTRO de la
  alocación (#357) —también desde un helper llamado por código nativo— y
  recicla. `make test-aotgc` (HOY ROJO a propósito: es el criterio de
  aceptación) lo demuestra: `"valor " + intToString(n)` en una native, con GC
  por alocación, imprime DOCE BYTES NUL con status=OK — el intermedio, cuyo
  único handle vive en un temporal de C, se recicla en mitad de la expresión.
  El control interpretado con el mismo GC imprime bien. Corrupción MUDA, y en
  HOST — cae también el «el AOT-en-host la tiene gratis» del doc de diseño
  (anotado allí, conservando el texto original).
  Gravedad hoy: ventana estrecha y natives que apenas encadenan alocaciones —
  pero `#428` acaba de abrir la puerta a cadenas en natives, que es justo el
  patrón vulnerable. El arreglo sigue siendo el diseñado (shadow stack), más
  dos piezas que el experimento añade: sincronizar `tc->sp` al entrar al thunk
  y enraizar los intermedios de expresiones con ≥2 alocaciones.
- **2026-08-16 — ✅ `#428` CERRADA, VERIFICADA EN LA METRO** (`7ddbfec`): una
  `native` puede llevar LITERALES. `RoTest` imprime `valor 7` / `negativo` con
  las cadenas viajando dentro del `.mdn` (188 B). La solución fue la de Eduardo
  —los literales como parte del código— vía un guión de enlace compartido que
  fusiona `.rodata` en `.text` (relocatable comprobado: byte-idéntico a dos
  direcciones). En los dos pipelines; el manual estaba además roto desde V5
  (classpath) y nadie lo notó. AOT en V5 queda: sólo `#302` (raíces GC).
  ⚠️ Matiz apuntado en la ficha: la salida limpia no distingue nativo de
  interpretado (esa es la gracia del degrade); la confirmación de 30 s es un
  Run con `log=1` mirando la línea del loader. Vale también para `#381`.
- **2026-08-16 — ✅ `#381` CERRADA, VERIFICADA EN LA METRO.** `long` en una
  función `native`, con la salida en ARM **byte a byte la del PC** y el `.mdn`
  generado por el propio IDE (8 thunks, 560 B). Lo que confirma: números de más
  de 32 bits, anchos mezclados en una firma, la división y el módulo por helper
  —la idea de Eduardo que evitó enlazar libgcc— y `div0: atrapado`, que es
  dividir por cero desde código nativo sin reiniciar la placa.
  🩸 **Y de camino, ficha nueva `#429`**: el IDE compila con SU copia del
  compilador (el fat-jar lo empaqueta) y **no avisa cuando está rancia**. Costó
  el primer intento de esta prueba: el IDE decía «no puede utilizar long en
  código nativo» con un fat-jar de ayer y el cambio de esta mañana. El aviso
  lleva tiempo en las notas y aun así se escapó — un aviso que hay que recordar
  cada vez ya ha fallado; lo que falta es que el desfase **se detecte y se
  diga**. Modo de fallo malo: no da un error raro, da uno PLAUSIBLE (el mensaje
  correcto de una versión anterior).
  📤 **`#426` (`double` en AOT) sale de los pendientes de V5** y pasa a una
  sección propia de V6, por decisión de Eduardo: lo que no es de esta versión no
  debe engordar su lista.
- **2026-08-16 (noche) — 🏁 `SqlDemo` CORRIENDO CONTRA LA SD, con todo lo de hoy
  dentro.** `exit 0 (OK)`: el pack de SQLite se carga en el primer `Run`,
  publica su API (17 símbolos), el módulo `SQLite.mod` se resuelve **desde la
  zona de packs**, y la demo inserta 6 filas en `/sd/medidas.db`, hace
  agregados y agrupa. Es la prueba que valida la cadena entera.
  🩸 **Y llegar ahí destapó un bug de los buenos** (`1d4ccbf`): la fachada del
  FS **no era coherente consigo misma**. `stat` y `read` consultaban el fallback
  de la zona de packs y **`read_at` no**, así que un módulo del pack existía
  para `stat` y no se podía leer por trozos — y cargar un módulo va por `read_at`
  desde #305. El síntoma era `IO error` sobre un `/app/SQLite.mod` que **no
  existe**, con el firmware avisando de que «el FS eclipsa al del pack» sin que
  hubiera ningún fichero en el FS.
  No era regresión: `read_at` llegó en #305 y el fallback en H4, y nunca se
  juntaron. Sólo aparece con un pack grabado **y** un módulo suyo que no esté
  además en el FS — la combinación que sólo se da usándolo de verdad. La regla
  queda fijada en un test: *si `stat` dice que existe, se tiene que poder leer,
  entero y por trozos*.
  Y `#421` estaba a medias por mi parte: el detalle salía del módulo principal y
  **el fallo real siempre es una dependencia** (el `Core.mod` del 15 y este
  `SQLite.mod`). Ahora vive en `vm->load_error`, junto a `link_error` y
  `runtime_error`, que ya existían para lo mismo.
- **2026-08-16 (tarde) — ⚡ EL ARRANQUE DEL P4: 717 ms → 386 ms, verificado en
  placa** (`bd8a916`). Con la imagen del 15-ago eran 965: **dos veces y media**.
  Y no se optimizó nada — se quitó del arranque lo que no debía estar ahí.
  **Era una decisión de Eduardo que no había viajado entre familias**: el pack
  nativo se carga en el primer `Run` y no al arrancar, *«porque un cuelgue
  durante un Run se arregla desenchufando una vez y uno en el arranque obliga a
  regrabar»*. Estaba escrita en `pico/pack_pico.c` desde el 7-ago y el P4 hacía
  lo contrario: 338 ms de cada arranque, y el único paso que puede colgar puesto
  justo donde no se sale sin regrabar.
  La parte fina fue **qué se mueve**: el log ya separaba las dos mitades
  (`mapear` 0 ms, `barrer` 338 ms). El mapeo se queda —el IDE necesita ver la
  zona desde el arranque— y se retrasa el barrido y el salto.
  🔍 **Y un test evitó un bug**: la idea inicial era «si la zona empieza virgen,
  no busques». `test_npack.c` tiene un caso que pone el pack en el offset 256
  entre basura — el ancla existe precisamente para no depender de dónde esté.
  ⏳ Falta ejercitar la línea del primer `Run` y que `PACK_LS` siga viendo la
  zona sin Run previo.
  **Van tres arreglos en dos días del mismo tipo**: algo que se decidió o se
  arregló en una familia y no llegó a otra (el corte del CRC de la SD, el log
  propio del Pico, y esto). Empieza a merecer una revisión sistemática, no
  seguir cazándolos de uno en uno.
- **2026-08-16 — Eduardo + Claude. Tres cerradas, y el grupo de «módulos y
  arranque» baja de cinco a dos.**
  ✅ **`#423` y `#420`, VERIFICADAS EN LA P4** con la imagen nueva. Eduardo:
  *«con log=0 no muestra mensajes de ejecución y con log=1 sí; los mensajes de
  arranque se mantienen siempre»* — el contrato de las tres partes, cumplido. Y
  esa misma prueba cierra `#420`: para que con `log=1` aparezca rastro de
  EJECUCIÓN tiene que estar conectado el sink del diagnóstico de la VM, que es
  lo que a esa familia le faltaba.
  ✅ **`#421`** (`e62a7fc`): los cuatro fallos de carga que antes decían
  `IO error` ahora dicen cosas distintas **y viajan por el wire** — al log ya
  iban desde ayer; lo que faltaba era que llegaran al IDE. El caso que costó la
  mañana del 15-ago («se lee pero no cuadra con su cabecera: truncado o de otra
  versión») se probó cortando un `.mod` por la mitad contra el simulador.
  ✅ **`#381` completa en host** (`072c864`): las conversiones numéricas dentro
  de una nativa. Sólo le falta la placa.
  Lo que enseñó el día: **el patrón que se repite es «el sistema lo sabe y no lo
  dice»** — el log que no existía en el P4, el motivo de carga que se quedaba en
  el firmware, el CRC que nadie pedía y todos pagaban. Tres fichas distintas, un
  solo tipo de bug.
- **2026-08-15 (noche, 2) — Eduardo + Claude. 🟢 `long` YA CRUZA A UNA FUNCIÓN
  `native`** (`f599574`, `bd5002f`). Hasta hoy el AOT sólo marshallaba 4 bytes.
  **Dos decisiones de Eduardo hicieron el trabajo, y las dos ahorraron camino:**
  1. *«long es una cosa y double otra; empecemos por long»*. La ficha decía
     «long, double y float JUNTOS» y la medida le dio la razón: compilando lo
     que emite el AOT con los flags reales, `long` `+ - *` no deja **ni un**
     símbolo sin resolver (GCC lo hace en línea) y `double` llama a libgcc para
     casi todo. Comparten el marshalling y nada más. `double` → ficha `#426`.
  2. Para la división —lo único de `long` que llamaba a `__aeabi_ldivmod`—:
     *«¿y si la reemplazamos en el emisor por una llamada a una función?»*. Y
     resultó que **ni siquiera hay que escribir una división por software**: el
     que no puede llamar a libgcc es el `.mdn`, no el runtime. Así que
     `idiv64`/`imod64` van en la tabla de helpers y el módulo nativo queda
     limpio — sin tocar el pipeline de ninguna arquitectura.
  Salió barato porque tres piezas ya estaban puestas: la pila BP ya guarda los
  `long` como 8 bytes big-endian (misma representación que el intérprete), el
  thunk ya movía 8 bytes con las refs (#302), y la tabla de helpers está hecha
  para crecer por el final.
  **Verificado** con `make test-longnat` (nuevo): la salida por los thunks AOT
  es idéntica a la de la VM-Java con 2^40, anchos mezclados en una firma,
  negativos, el máximo de 64 bits, llamadas encadenadas, división, módulo y
  división por cero **atrapada con `try/catch`** desde código nativo. El objeto
  ARM real no deja un solo símbolo indefinido. Más `test-bytenat`,
  `test-compressnat`, `test-callbp`, `test-throwmsg`, paridad 28/0/0,
  frontend 104/104, miVM 34/34.
  ⏭️ **Falta**: el cast `integer(x)` dentro de una `native`, y **probarlo en
  placa** — el `.mdn` sólo se carga de verdad allí. Análisis y plan en
  `docs/AOT_ABI8_IDEAS.md`.
- **2026-08-15 (noche) — Eduardo + Claude. ⚡ EL ÁRBOL DEL IDE: 6953 ms → 155 ms
  (45×), verificado en la P4.** Y el arranque, de paso, 965 → 717 ms.
  **Cómo se llegó, que es lo que hay que repetir**: Eduardo dijo *«no hace falta
  especular, lo podemos medir; lo que hace falta es que el log lo registre»*. Se
  instrumentó el refresco en los dos extremos (`b44f15e`) y el instrumento
  contestó a la primera: **el CRC era el 98-99 % del tiempo**. Antes de eso, la
  hipótesis en la mesa era el arranque —y la medida la había descartado ya: 965
  ms hasta el wire, 266 de ellos por la tarjeta—. **Dos arreglos evitados por
  medir, uno acertado por medir.**
  **La causa no era «el CRC es caro»**, y ahí está la lección: `bpvm_fs_crc32`
  troceaba el fichero de 256 en 256 B y cada trozo iba por `read_at`, que recibe
  el PATH — o sea que **cada 256 B se abría el fichero otra vez**. 5432 aperturas
  para 1,3 MB, con un `f_lseek` que recorre la FAT desde el principio: cuadrático
  con el tamaño. El dato que lo delató fue una rareza en los números: **el flash
  interno iba tres veces más lento que la SD**, lo que ya decía que el cuello no
  era leer.
  Arreglado en dos mitades: `f4e5c1f` (el backend calcula el CRC con UNA
  apertura; 16,5× medido en el PC sobre littlefs, los tres backends) y `10b4467`
  (el listado no calcula CRC; se pide con `STAT {crc:true}` justo antes de subir
  ese fichero). Verificado contra el **simulador** —LIST, STAT y el valor
  idéntico a `java.util.zip.CRC32`—, con `sim-smoke`/`boardsim-smoke`, la
  batería del FS, paridad 28/0/0, y **el firmware de la Metro construido con el
  toolchain ARM**.
  🩸 **Y por qué la P4 sufría más que la Metro**: el corte que evitaba calcular
  el CRC de los volúmenes montados estaba **sólo en el Pico** desde V5/H2. La
  familia ESP32 nunca lo recibió. *Un arreglo que no viaja entre familias es
  medio arreglo* — van ya unos cuantos.
  🔸 De rebote, el `ESP_ERR_TIMEOUT` del montaje no ha vuelto a aparecer. **No se
  da por muerto**: era intermitente y una pasada buena no prueba nada; la
  hipótesis (y cómo confirmarla) está en la ficha.
  🔸 **El tramo más caro del arranque es ahora otro**: 337 ms escaneando la zona
  de packs para encontrar `0 candidatos`, casi la mitad de los 717 ms.
- **2026-08-15 (tarde, 3) — Eduardo + Claude. 🏁 H11 (PACKS) CERRADO.** Las
  cuatro fichas que colgaban de él, resueltas: `#417` y `#414` verificadas en
  placa, `#365` verificada en las dos VMs, `#411` en su parte de packs — y
  **`PACK_CALL` (#383) CANCELADA**.
  **Dos decisiones de alcance de Eduardo, y las dos son la misma idea**: sacar
  del hito lo que no era suyo. La **limpieza de `notas/`** (5 carpetas, ~56 MB)
  no es trabajo de packs sino de cierre de versión, y estaba trabando el hito;
  se movió a su sitio. Y **`PACK_CALL`** —llamar a un pack sin AOT— se cancela:
  *«estos packs los hacemos nosotros, así que el sistema actual está bien»*. Lo
  que compraba era que **mantener** un pack nativo no exigiera los dos toolchains
  cruzados, y como el único que publica packs nativos es el proyecto, esa barrera
  no existe en la práctica.
  ⚠️ **Lo que eso deja aceptado, y conviene tenerlo escrito**: en un pack nativo
  el AOT **no es una optimización, es un requisito** — sin `.mdn` para esa
  arquitectura, sus funciones lanzan. Y el AOT **es mudo por línea de comandos**:
  si no puede generarlos, el pack sale más pequeño sin decir nada. Eso deja de
  ser «algo que PACK_CALL arreglará» y pasa a ser definitivo. El comentario del
  parser que lo daba por futuro está corregido.
  **Queda H10 (IDE) como único hito abierto de V5.**
- **2026-08-15 (tarde, 2) — Eduardo + Claude. ✅ `#365` CERRADA** (`88e75a4`): un
  módulo con `library` ya puede **arrancar** un pack. No era «`library` +
  `out:pack` es imposible», como decía la ficha: era el arranque. El `.mod` de un
  módulo con `library` se llama `com.example.Demo.mod` —así se llama su entrada—
  y el manifest escribía `main=Demo`, que es el nombre del FICHERO FUENTE; quien
  arranca busca literal y no lo encontraba.
  **La solución es de Eduardo** («¿y si ponemos `library` dentro del manifest?»),
  y entre las dos formas se eligió la que **no toca las VMs**: el manifest lleva
  ya el nombre canónico en `main=`, en vez de un campo `library=` que las dos VMs
  tuvieran que concatenar — *dos implementaciones haciendo la misma cuenta es
  donde el invariante se rompe*. Coste en runtime: **cero líneas**. El manifest
  es un artefacto generado, y puede llevar el nombre resuelto.
  De camino apareció una trampa muda: la regla de la doble extensión
  (`sqlite.npk.RISCV`) se comía los nombres cualificados que acaban en un tipo
  (`com.example.Npk.mod` → `com.example.mod`), en silencio y dentro de un pack ya
  grabado. Arreglada con la condición que separa los dos casos.
  Verificado de punta a punta: `samples/packlib/` corre igual en **las dos VMs**,
  104/104 frontend (2 tests nuevos), 34/34 miVM, paridad 28/0/0, y el
  `SQLite.pack` real reconstruye sus 9 entradas idénticas.
  Con ésta, **H11 se quedó a una ficha** — y se cerró esa misma tarde (arriba).
- **2026-08-15 (tarde) — Eduardo + Claude. 🏁 EL BUS DE LA SD DEL P4 ES SANO.**
  `samples/BusTest.bp` en la tarjeta: **2048 KB de patrón conocido, ida y vuelta,
  0 diferencias**, 4 bits, **20 MHz**. Era la ficha que más pesaba de la tanda —
  la condición previa para poner SQLite encima de esa tarjeta— y queda cerrada.
  El instrumento se validó antes con un control **en rojo** (meterle al fichero 5
  el contenido del 6: lo cazó por el byte 2, que es donde va el número dentro del
  patrón). *Se reabre si se sube el reloj*: un bus marginal aguanta despacio y
  falla arriba.
  **Y el instrumento tenía un fallo que casi deja la medida sin valor** (`0a4e25c`):
  el log de arranque imprimía `pines.khz` —lo que PIDE el env—, y con el env
  vacío eso sale `| 0 kHz`. La prueba estaba hecha y no se podía decir a qué
  velocidad. El driver resuelve `khz > 0 ? khz : SDIO_KHZ_POR_DEFECTO`, así que
  fueron 20 MHz; ahora el log hace la misma cuenta y dice de dónde sale el
  número, y la constante vive en `blk_sdmmc_p4.h` en vez de escondida en el `.c`.
  ⏳ **sin compilar** (no hay ESP-IDF en esta máquina).
  Lo que enseñó la tarde: **un número imposible en un log no es cosmética, es
  el log diciendo que no sabe de qué habla**. `0 kHz` no es «no lo sé», es un
  dato falso — y estuvo ahí, leído varias veces, hasta que hizo falta anotarlo.
  Un chivato que anuncia un valor que no existe es peor que no tener chivato.
  **Abierta `#423`**, que salió del pie de ese mismo log (`[LOG OVERFLOW]`): el
  GC escribe **3 líneas por colecta** (~300 B) y el log mide **8 KB** en el P4 y
  **4 KB** en el S3 → **~26 colectas y está lleno**. Está en las cuatro familias.
  Lo grave no es que se llene: `append_raw` es append-only y **se calla por el
  final**, o sea que el log de una placa colgada contiene el arranque y no el
  cuelgue. Decisión de Eduardo pendiente — esas líneas son el instrumento con el
  que se cazaron #355 y #357.
- **2026-08-15 (mañana) — Eduardo + Claude.** Tanda de placa y de packs. Cerradas
  **`#414`** (módulo `Packs`: `list()` / `listIn()`, verificado en el P4 el mismo
  día que se escribió), **`H2-P4`** (las seis operaciones del FS, en los DOS
  volúmenes) y **`#411`** en su parte de carpeta: `bpstdlib/sqlite/` con fuentes,
  los cuatro nativos versionados y un `LEEME` con la cadena — **el pack ya se
  puede reconstruir desde un clon limpio**, que antes no. Arregladas además
  `#418` (el resolutor mira `/sys`) y **`#420`, el P4 era la única familia sin
  log de EJECUCIÓN**.
  Lo que enseñó la mañana, y es una sola cosa dicha de cuatro maneras:
  - **el instrumento cómodo no dice la verdad sobre la placa**. El FS del host
    daba 11/11 y tapaba que en littlefs `mtime` no existe; lo destapó
    `--fs=lfs:`, que usa el mismo motor que el micro;
  - **lo que no se compara, se pudre**: un `/lib/Core.mod` rancio del mismo
    tamaño que el bueno costó media mañana, y el IDE nunca mira el CRC de `/lib`
    porque sube a `/app` (#422);
  - **la red de seguridad hizo su trabajo**: el pack de SQLite no salió igual dos
    veces seguidas, y por eso no se borró nada de `notas/`;
  - y las tres fichas que se cerraron salieron de **predecir desde el código y
    usar la placa para confirmar**. Las horas se fueron en lo contrario.
  Pendiente de la tanda: `#418` en placa y `H2-P5` (otras tarjetas). **La prueba
  del bus sano — la que más pesaba — se cerró al día siguiente** (ver la entrada
  del 15-ago).
- **2026-08-14 (noche) — Eduardo + Claude. 🏁 H9 CERRADO.** `Object` deja de ser un
  alias de `any` y pasa a ser la raíz REAL del modelo de objetos (existía desde
  H5.1.a, pero sólo en el emisor: al semántico nadie se la había presentado).
  Subir es implícito, bajar se escribe con el nombre del tipo —`Cosa(o)`,
  `string(o)`— que es la regla que ya regía entre primitivos (`byte(someInt)`).
  **#389 queda pendiente**: falta que la conversión COMPRUEBE en ejecución.
  Antes se guardó el trabajo suelto en 6 commits (el WIP del P4 del 12-ago, la
  documentación) y se arregló `test-pack`, que no enlazaba desde #362.
  Lo que enseñó la tanda, y conviene no olvidar:
  - **el desfase de `.mod` ANESTESIA los cambios**: con la stdlib vieja (interfaz
    `any`, que traga cualquier cosa) la suite y la paridad daban verde con
    `string → Object` sin implementar. No se vio hasta regenerar la stdlib;
  - **un SKIP no es un PASS**: el arnés dijo "VERDE" con 3 SKIP mientras tres
    samples no compilaban;
  - **el censo sólo vale con el directorio de salida BORRADO** (el compilador es
    incremental): mintió tres veces en el mismo día, una de ellas diciendo "2 de
    26 módulos" cuando eran 26;
  - **prueba fuerte que sí sirvió**: 24 de 26 `.mod` byte-idénticos, y los 2 que
    cambian sólo en la interfaz — `+69 = 23×3` y `+15 = 5×3`, exactamente lo que
    crece `"any"` al pasar a `"Object"`. Cero `any` en las interfaces publicadas.
- **2026-08-14 (tarde) — Eduardo + Claude.** Reescrito este documento con datos del
  repo en vez del andamiaje inicial. Dos cosas que decía y **eran falsas**:
  (a) **#310 no está abierto** — se cerró en V4 y está verificado en las tres
  familias (`16c7970`, `237c963`, `677bcf2` *"batería estándar del P4 al completo —
  8/8, y #310 en las tres familias"*); (b) el **check de `.pack` ya está duplicado**
  en cuatro sitios, no vive sólo en el CLI de la VM-C. Añadido lo que faltaba de
  los días 12-14: H8, la tanda H9, #390, #362 y #403.
- **2026-08-14 — Eduardo.** V4 dada por cerrada. En V5 se añadió soporte de SQLite
  y ha quedado bastante bien.
- **2026-08-14 — (andamiaje inicial)** — este ESTADO.md se creó a partir de
  `README.es.md`, `PHILOSOPHY.md` y `PENDIENTES.md`. Sus secciones "En curso" y
  "Próximos pasos" eran semilla sin verificar; corregidas arriba.
