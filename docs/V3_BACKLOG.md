# BasicPlus — Backlog de V3

> **Fuente única del backlog de V3.** Las *tasks* del IDE/sesión son efímeras
> (el trabajo en curso de cada sesión); **este doc manda**. Visión y principios:
> `V3_ROADMAP.md`. Charlas de diseño: `V3_IDEAS.md`. Al cerrar un ítem se anota
> su resultado aquí y, cuando toque, pasa a un futuro `HECHO_V3.md`.
>
> Estado: `pendiente` / `en curso` / `cerrado`. (`#NNN` = id de task histórica.)

---

## 🗺️ Mapa de hitos (post-H7, Eduardo 20-jun)

H6 (widgets GUI, en la DK2) ✅ · H7 (lenguaje: `^`, `eval`, separador `_`, continuación) ✅.
Orden siguiente:
- **H8 — librería estándar** (revisión/ampliación). *Anotado; SE SALTA por ahora.*
- **H9 — revisión del IDE** (cosillas a mejorar). *Anotado; SE SALTA por ahora.*
- **H10 — paridad HW del STM32 + reset-cause (ACTIVO):**
  - (a) Implementar en STM32 (HAL U5) las clases de control de HW que existen para Pico/ESP32
    pero **NO** para STM32: **I2C** (`I2c.Bus`), **ADC** (`Adc.Channel`), **PWM** (`Pwm.Slice`),
    **Timer** (`Timer.Alarm`), **WDT** (`Wdt.Timer`), **RTC** (`Rtc.Clock`), **pulse-counter**,
    **Neopixel**… (UART + SPI ya hechos en H15). **Primero en la Nucleo-U575** (placa STM32 de V2).
  - (b) **Reset-cause vía RAM retenida**: usar la backup SRAM / registros que sobreviven al
    reset para guardar una "miga de pan" (causa del último reset: watchdog / hardfault / power /
    KILL…) y reportarla en el boot. Liga con la observabilidad de bring-up (V3_IDEAS §6).
  - (c) Al terminar, **portar el desarrollo a la Discovery DK2** (misma familia U5).

(GUI en pausa ≥1 semana tras cerrar H6 en la DK2.)

## ✅ Estado y decisiones de cierre de V3 (24-jun)

**Buque insignia HECHO:** GUI de BasicPlus, MISMO bytecode, en 3 silicios (Java host ·
ARM/STM32-DK2 · RISC-V/ESP32-P4). Las dos columnas (DK2 + confirmación P4) en pie → V3 en
**fase de cierre** (rematar + tapar agujeros + documentar). Cerrado además: H10 (HW STM32
Nucleo+DK2), stdlib Compress/Json/Freq (core), P4 completa (VM + G1-G6 gráficos + Ethernet/wire).

**Decisiones (Eduardo, 24-jun):**
- **P4 = placa BP COMPLETA (HW al 100 %) → DENTRO de V3.** Motivo de peso: hay **pantallas
  con P4 integrado, baratas → candidatas reales para proyectos**; por eso la P4 debe hacerlo
  TODO, no solo gráficos. Portar los backends HW del firmware P4 (GPIO/I2C/SPI/ADC/PWM/UART…
  hoy **stubs**), al estilo S3 (ESP-IDF). **NO ahora** (atracón de flashes) — más adelante.
- **TCP server (por la Ethernet del P4): APLAZADO** (sin decidir; aplazar no es grave). La
  Ethernet del P4 ya funciona → desbloqueado técnicamente, pero no prioritario.
- **Rollout de gráficos a otros kits: solo equipos con recursos de sobra (flash/RAM/pines);
  NO prioritario.** La pantalla con S3 "podría caber pero me da igual" → no se persigue.
  DK2 + P4 son las plataformas gráficas de V3.
- **Pausa de flasheo** tras el maratón del P4 → el trabajo inmediato debería evitar reflashear
  (docs / IDE host-side / AOT host).

### Plan de cierre de V3 — ORDEN (24-jun, visto bueno de Eduardo)

**NO se añaden más features a V3.** Solo queda ejecutar, en este orden (lo de PC primero —
venimos de un atracón de flashes; el P4-HW, flash-intensivo, cuando se recarguen pilas):

**Numeración de hitos** — V3 reinicia su propia serie. Hechos: H1–H7; **H8** (stdlib) = núcleo
✅ (Compress/Json/Freq), resto→V4; **H10** (HW STM32) ✅; **H11 = ESP32-P4 / familia RISC-V** ✅
(VM + gráficos G1-G6 — etiqueta retro para anclar la serie). **H8 y H9 eran los diferidos** del
mapa post-H7; **H9 (revisión del IDE) se materializa AHORA** dentro del cierre. Los 6 pasos del
cierre = **H12…H17** (1=H12 · 2=H13 · 3=H14 · 4=H15 · 5=H16 · 6=H17):

1. **(H12) IDE Fase 1** *(PC, sin flasheo)* — (a) comodidad del editor: comentar/indentar + barra de
   botones + fix del árbol remoto cortado; (b) AOT automático (ARM: RP2350+STM32): config
   por-proyecto + detección toolchain + pipeline detect→C→gcc→.mdn→subir + degrade a interpretado
   con warning.
2. **Forms (la "ventana")** *(PC)* — (a) cargador `Gui.Form.load` (JSON→árbol Component, binding
   por nombre) + esquema del form; (b) IDE Fase 2: editar forms desde el IDE (+ candidato: repaso
   AOT-casts).
3. **P4 HW completo** *(flasheo intensivo → cuando se recarguen pilas)* — backends GPIO/I2C/SPI/
   ADC/PWM/UART en el firmware del P4 (estilo S3).
4. **Tapar huecos** — bugs conocidos + INFO/causa-de-reset en el IDE + pulidos sueltos del backlog.
5. **Documentación** — documentar (usuario + interno).
6. **Finalización (estilo V2, = H14)** — batería de PRUEBAS en dispositivos reales para asegurar
   que todo funciona; los bugs que aparezcan, se arreglan; + la **publicación del cierre** (V3 ha
   ido en LOCAL todo el tiempo, repo público congelado en V2 → publicar V3 es la decisión del final).

Orden flexible donde las piezas son independientes (cargador ↔ AOT intercambiables; el P4-HW puede
adelantarse en cuanto haya ganas de flashear).

### V4 — fuera de V3 (Eduardo, 24-jun)

**Tema de V4: consolidar + mejorar rendimiento** (un poco como fue V2), **además de lo diferido**:
AOT en ESP32 (Xtensa/RISC-V), servidor TCP (`Net.Listener`), **diseñador visual de forms drag&drop**,
preview de forms en PC (si no se cuela antes), rollout de gráficos a más kits, dual-core RP2350 (#153),
`deflate`-lite, multi-fichero/Archive, IDE multiplataforma (jSerialComm), strings multilínea + interpolación,
AOT cross-module (#169). **El alcance de V3 queda CERRADO** ("y ya nada más").

## 🎨 GUI (objetivo cabecera)

El camino crítico —upcall C→BP → `Gui.*` en miVM → VM-C host (LVGL+SDL) →
portabilidad → micro → cross-family— se detallará en los hitos H1+ (ver
`V3_ROADMAP.md` §6 y la decisión consolidada en `V3_IDEAS.md` §1). Las tareas
concretas se crean al arrancar cada hito.

- **Ventana = módulo + form JSON (diseño de UI por JSON) — concepto clave de V3 (charla 24-jun).**
  Una "ventana" se define con DOS ficheros: el **módulo** (`.bp`, el código) + un **form**
  (NUEVO: fichero **JSON** con el árbol de Componentes). El módulo, en su init, **carga el
  JSON y construye la ventana dinámicamente** → diseñar la UI desde el PC sin reprobar en el
  device. Encaja con lo hecho: modelo `Component` (H6) = el árbol; `Json.bp` = el parser;
  GuiBackend Swing de miVM = preview en el PC casi de regalo. **Alcance (Eduardo):**
  - **FUNDAMENTAL (V3): el cargador runtime** `Gui.Form.load(json)` — BP portable,
    byte-idéntico en los 3 silicios; recorre el árbol y crea los `Gui.*`. Implementación:
    dispatch tipo→constructor + propiedad→setter por **switch** (BP no tiene reflexión; el
    set de widgets es acotado → manejable).
  - **MEJOR (V3 si cabe): editar los forms desde el IDE** (editor de form / JSON asistido).
  - **PARA NOTA (puede ir a V4): preview en el PC** — y OJO: es **más barato de lo que parece**,
    porque el cargador es BP y miVM ya pinta en Swing → "preview" ≈ *correr el form en miVM
    dentro del IDE*. El diseñador visual drag&drop completo = V4.
  - **Binding evento↔código: por NOMBRE** (el form nombra el widget; el código hace
    `form.get("btnOk")` + handler/subclase). `onClick:"handler"` por-nombre = azúcar futuro.
    Modelo "diseñador + code-behind" (Qt/Android/NetBeans), conviviendo con el modo imperativo.
  - Form = **JSON soberano** (NO el XML privado de LVGL); mapea a nuestro árbol Component.
    Diseñar el esquema con anidación desde el principio.

- **GUI-blocking-from-event — ejecutar trabajo largo / diálogo modal con respuesta
  desde un handler de evento sin congelar el GUI** (Eduardo, 20-jun). Raíz: el
  upcall (`onClick`/`onChange`) corre en el worker que TAMBIÉN bombea el GUI
  (modelo ISR); si el handler bloquea, en LVGL/micro (un solo hilo) es **deadlock**
  (nadie procesa el toque que cerraría el modal / nadie repinta). Dos casos:
  (a) **proceso largo** desde un evento → seguramente `Thread` aparte + marshalling
  del resultado/repintado al pump del GUI (el GUI es single-thread en LVGL); (b)
  **diálogo modal síncrono** (`if confirmar("¿Borrar?") then …` en línea) → o el
  mismo patrón async-con-callback (ya cubierto por Msgbox+botones+`onChange`), o un
  **bombeo anidado** re-entrante en la llamada (más complejo, re-entrancia). Diseñar
  con calma; NO bloquea H6 (Msgbox-aviso es asíncrono y suficiente para H6).
  **Refinamiento (Eduardo, 21-jun) — la ERGONOMÍA es la clave:** el caso (a) debe
  resolverse con una **llamada sencilla async** (estilo Swing `invokeLater`/`SwingWorker`):
  el usuario NO crea ni arranca el Thread — escribe algo como `async(tarea)` dentro del
  `onClick` y **BP lo gestiona por debajo** (crea/arranca/limpia el hilo). Y un
  `invokeLater(closure)` que marshale de vuelta al hilo del GUI para repintar/actualizar
  widgets con seguridad (reusa la cola de eventos del upcall). Objetivo: que programar la
  GUI sea AMIGABLE, no un infierno de threads manuales. **Decisión de diseño clave:** ¿tiene
  BP valores-función/closures? Si NO, la API usaría objetos estilo Runnable (clase con `run()`
  override-able, como `onClick`); si se añaden closures, `async(() -> …)`. → **REPESCA del
  GUI** (no pertenece a ninguna H).

## 🛡️ Arnés de no-regresión V2→V3 (montar PRIMERO)

Instrumental del principio 7 (`V3_ROADMAP.md` §4): la red antes del trapecio.
- Oráculo **VM-Java V2 congelada** (jar del tag `v2.0`) para diff de comportamiento.
- **OpCodes congelados** (golden `nombre→id`; V3 = superconjunto, ids V2 intactos).
- **Guardián del `.mod`**: lectura (`.mod` V2 + salida esperada → VM-V3 byte-idéntico)
  y escritura (snapshot del emisor + disassembler #163 vs `MOD_FORMAT.md`).
- **Regresión V2 verde** en las VMs de V3.

## 🧩 Infraestructura (la arrastra el GUI)

- **#153 — Dual-core RP2350** (single-core SMP validado en placa; dual-core con
  SWD). **Incluye el fix de B1** (la race solo asoma con paralelismo real; en V2
  mitigada a 1 worker, el device single-core es inmune). Un núcleo a lo gráfico.
- **#225 — Mapa de memoria configurable por PSRAM** (`memorySize` de `BpVM.cfg`);
  verificar en ambas placas RP2350. Para framebuffers/recursos del GUI.
- **#229 — FS grande aprovechando la flash del Metro (16 MB)**: detección
  runtime + FS mayor.

## 🌐 Stdlib

- **#145 — WiFi en placa (Pico 2 W / ESP32-S3) + servidor TCP.** H11.a (cliente
  TCP host) ✅ en V2; queda **H11.b** (WiFi al boot con `/sys/wifi.json`) +
  **H11.c** (backend `Net` sobre lwIP), y de premio el wire v1 sobre TCP ("Run on
  Device" sin cable). Diseño en `HECHO_V2.md` (H11) + `WIFI_TCP_REFLECTION.md`.
- **Servidor TCP (`listen`/`accept`) — APLAZADO; se retoma con la Ethernet del ESP32-P4**
  (decisión Eduardo, 22-jun). El cliente `Net.Tcp` ✅ V2 (H11.a); falta el lado servidor
  (builtins listen/accept + `Net.Listener` + thread-por-conexión + server de prueba en PC).
  **Estrategia: desacoplar TCP/IP de WiFi.** Al arrancar el ESP32-P4 se valida primero su
  **Ethernet** (puerto físico, IP fija — liga con la idea del log-Ethernet del P4); si va
  bien, se retoma la librería TCP sobre ese transporte cableado. WiFi (#145) queda aparte
  (otras connotaciones: lwIP-sobre-WiFi, credenciales…). Cierra la tanda de stdlib de
  esta sesión (Compress ✅ / Json ✅ / Freq ✅ en placa).
- **Compresión (CORE hecho, 22-jun).** `Compress.decompress` (LZSS) ✅ V2 +
  **`Compress.compress` (LZSS) ✅** (mismo framing, round-trip verificado en host,
  commit `53bbd7b`). Pendiente:
  - **`compress` native — PENDIENTE, depende de "AOT — casts"** (sección Compilador).
    Hoy corre INTERPRETADO (necesita `byte()` chequeado y el AOT no emite casts). Es la
    dirección rara (comprimir para guardar/enviar) → interpretado vale; `decompress` sí
    es native-able (solo copia byte→byte).
  - **Multi-fichero / Archive — DIFERIDO por complejidad** (Eduardo gateó "si no es muy
    complejo"). El códec es buffer-a-buffer (dst + capacidad); un packer multi-entrada en
    BP puro pide o un builder OO con estado o colecciones (List&lt;byte[]&gt;) → cruza el
    "thin container". Retomar si surge necesidad real (p.ej. empaquetar recursos GUI).
  - **Futuro `deflate`-lite** (LZSS → +Huffman). Aditivo, sin tocar `decompress`
    (hay código V2 que depende — principio 7).
- **Json (CORE hecho, 22-jun).** `samples/Json.bp` (librería completa: jerarquía
  JsonValue, parser recursivo, serializador, escapes) promovido a `bpstdlib/Json.bp`
  (commit `48220e4`) + ergonomía: `writeJsonPretty` (indentado) + getters
  `getString/getInt/getFloat/getBool/getObject/getArray` y `getXxxOr(key, def)` en
  JsonObject. Smoke test → `samples/JsonDemo.bp`. **Paridad dual-VM byte-idéntica
  verificada en host** — al hacerlo se cerró un gap: `parseFloat` (builtin 2) portado
  a la VM-C (`strtod`→f32; antes era "portable diferido" de H10). Pendiente: embeber
  en firmware (tanda de placa); futuro UI-via-Json (mapear al árbol Component, NO V3).
- **Freq (HECHO + verificado en placa 22-jun).** `bpstdlib/Freq.bp`:
  `Freq.Meter` (frecuencímetro por conteo de flancos sobre `Pulse.Counter`) —
  `measureHz(windowMs)` / `measureHzAvg(windowMs, n)` / `maxHz(windowMs)` (techo por
  contador 16-bit). Sample `samples/FreqDemo_Dk2.bp` (puente PB8→PB7). **Verificado en
  la DK2** (sin reflash — el firmware ya tenía el pulse backend; el IDE subió las deps):
  1 kHz→1000, 500 Hz→500 (avg 501), 2 kHz/100 ms→2000. **Mejoras futuras (NO V3, idea de
  Eduardo 22-jun):** (a) combinar **dos contadores de 16 bits → 32 bits** (cascada de
  timers) → techo mucho mayor sin acortar la ventana, menos overflow; (b) **frecuencias
  < 1 Hz** y más precisión vía **input-capture** (medir el periodo entre flancos,
  freq = 1/periodo) — el conteo por ventana ya cubre bien la ALTA frecuencia, el periodo
  cubre el extremo BAJO/sub-Hz; necesita backend HW (timer en modo IC, firmware).

## ⚙️ Compilador / lenguaje (ampliación selectiva — "eventos y poco más")

### H7 — HECHO (20-jun): 4 cambios de lenguaje, dual-VM byte-idéntico

**Cerrado**: operador `^` (`7a426af`), `eval` (`e73b329`), separador `_` (`7caaea4`),
continuación de línea por operador colgante (`dcc95bd`). compat GATE + suites verdes;
IDE reempaquetado. **`^` y `eval` necesitan reflash** para la DK2 (tocan las VMs); el
separador y la continuación son solo frontend. **`^` en native/AOT: diferido** (ver
nota en el bullet del operador). Detalle de diseño original abajo (se conserva):

- **Operador `^` de potencia** — `x^2` = x al cuadrado. Token `^` **LIBRE** (en BP
  el XOR es la keyword `xor`, no `^` → sin colisión). **Toca:** lexer (token nuevo),
  parser (binario, **asociativo por la DERECHA** y de **precedencia ALTA** — por
  encima de `*` y del unario menos: `-x^2` ⇒ `-(x^2)`, `2^3^2` ⇒ `2^(3^2)`),
  semántico (tipos/resultado), emisores (MivmEmitter + AotCEmitter + JvmEmitter) y
  **las 2 VMs byte-idéntico**.
  - **Cómputo por tipos (encargo de Eduardo):** `float ^ integer` → **multiplicación
    repetida** / exponenciación por cuadrados (exacto y **parity-safe**; exponente
    negativo ⇒ `1/x^|n|`). `float ^ float` → **`exp(n·ln(x))`** (dominio x>0; `ln`
    indefinido para x≤0 → decidir error vs NaN). `int^int` → decidir (mult repetida;
    ¿overflow a long?). Resultado: `float^*` ⇒ float.
  - **RIESGO DE PARIDAD (el grande):** el camino `float^float` usa transcendentales —
    `Math.exp/log` (Java) vs `exp/log` (C) pueden diferir en el último ULP ⇒ resultado
    NO byte-idéntico. El camino `float^integer` (mult) SÍ es byte-idéntico. Mitigación
    a probar: computar en **double y estrechar a float32** (las diferencias de ULP en
    double suelen desaparecer al redondear a float) y **verificar con el arnés de
    paridad**. **Ya existe el builtin `POW`** (expuesto vía `Math.pow`): revisar su
    impl/paridad actual; `^` float^float puede **bajar a él** si es byte-idéntico en
    ambas VMs. Aditivo (principio 7): opcode nuevo (p.ej. `POW_F`/`POW_I`) o reuso de POW.
  - **HECHO (20-jun, commit `7a426af`):** `^` en el INTÉRPRETE (ambas VMs, dual-VM
    byte-idéntico) con IPOW/LPOW/DPOW (0xAC/0xAD/0xAE). **PENDIENTE — `^` en AOT/native**
    (Eduardo 20-jun: "dejarlo y anotar", retomar con el AOT): `AotCEmitter` no emite `^`
    → una `native function` con `^` **ABORTA la compilación** (el `native` exige AOT-able).
    El camino .mod (interpretado) funciona perfecto. Fix futuro: helpers
    `aot_ipow/aot_lpow/aot_dpow` (misma lógica que los opcodes) + emisión en AotCEmitter,
    O degradar AOT no-soportado a interpretado (best-effort) en vez de abortar. Mismo
    hueco para el backend JVM (JvmEmitter).

- **`eval("expresión")` — evaluador de expresiones en runtime, estilo BASIC
  (LIMITADO y SEGURO).** NO es el `eval` de Python (sin código arbitrario, sin acceso
  a variables/funciones) → **seguro por construcción** (gramática cerrada). Ampliable
  poco a poco en versiones futuras.
  - **Alcance inicial:** solo operaciones básicas (`+ - * /`, paréntesis, números;
    y `^` cuando exista). Nada sofisticado.
  - **Toca:** builtin `eval(s)` en las 2 VMs → cada VM lleva un **mini-parser de
    expresiones** (descenso recursivo) con resultado **byte-idéntico** (misma
    aritmética y mismo tipo numérico). Superficie de paridad NUEVA (un evaluador en
    Java + otro en C) → mantenerlos alineados; el grueso (aritmética) es parity-safe.
  - **Decidir:** tipo de retorno (¿double/float?), error de sintaxis ⇒ ¿RuntimeError
    BP cazable? **Futuro (NO ahora):** variables/funciones/llamadas, siempre dentro de
    una gramática cerrada (jamás el modelo Python ilimitado).

- **Separador `_` en literales numéricos** (Eduardo, 20-jun) — `100_000`,
  `0xDEAD_BEEF`, `0b1111_0000`, `1_000.5`, `2_000L`. **SOLO el scanner**
  (`Lexer.scanNumber`): consumir `_` únicamente si va **entre dígitos** (dígito antes
  y después) y descartarlo antes de `parseLong`/`parseDouble` → así `_100`/`100_` no
  chocan con identificadores. Aplica a decimal/hex/binario/float (todos pasan por
  `scanNumber`). No afecta a nada más (ni .mod, ni VMs).

- **Continuación de línea — DECIDIDO: (A) operador colgante** (Eduardo, 20-jun). La
  continuación IMPLÍCITA dentro de `()`/`[]` YA existe (#249/L5); para el resto, **si la
  línea termina en un operador binario / coma / `:=` / `.` / `and`/`or` (un "token
  continuador"), la sentencia continúa en la siguiente — SIN carácter especial**
  (estilo Kotlin/Go). **Implementación:** extiende la MISMA supresión de newline de
  #249 — al fin de línea, mirar el último token significativo; si es un continuador, no
  emitir el terminador de sentencia. Definir el conjunto exacto de tokens-continuadores
  (operadores binarios + `,` + `.` + `:=`/`=` + `and`/`or`/`xor`/`mod`/`shl`/`shr` + el
  `^` nuevo). Descartados `\` final y `_` final (este chocaría con el separador nuevo).

- **AOT — casts (`byte()`/`int()`/`float()`/`long()`/`double()`) — PENDIENTE; repaso
  del AOT en la fase IDE** (Eduardo, 22-jun). El `AotCEmitter` **no maneja NINGÚN nodo
  cast** → cualquier función con un cast cae a interpretado (verificado: 0 coincidencias
  de cast/conversión en `AotCEmitter.java`). **Bloquea `compress` native** (`byte()`
  chequeado 0..255) y todo `native` con conversiones. Fix: emitir el narrowing/conversión
  en C (para `byte()`, i32→u8 con la semántica chequeada de `I32_TO_U8`; el helper de
  store a `byte[]` ya existe, #193). Mismo paraguas que `^` en AOT (arriba) → cuando
  toque el IDE, **repaso del AOT de una pasada**.
- **#169 — AOT cross-module sin puente del intérprete** (MEJORA de rendimiento;
  hoy FUNCIONA vía `call_bp` + warning). Diseño en `AOT_CROSS_MODULE.md`.
- **Layout compacto de narrow** (L10 follow-up): `byte[]`/`int16[]` y globales
  narrow con storage real (hoy viven como i32; la maquinaria de la VM ya existe).
- **Fixed arrays `tipo[N]`** en campos de clase y en `native` (hoy error honesto;
  locales y de módulo ✅ en V2).
- **N-pubvar-warn**: `public var` de módulo se ignora en silencio → avisar
  ("usa property/const"). La gramática ya documenta la regla.
- **N5 v2 / condvar real** en `SyncList` (hoy spin-poll de 1 ms).
- **Strings multilínea** (Eduardo, 20-jun: *"estaría bien, pero no urgente"*) — **NO
  para H7**; futura mejora de lenguaje. Literal de cadena que abarca varias líneas
  (triple comilla u otra sintaxis a decidir), útil para JSON/plantillas embebidas.
  Junto con interpolación de strings, candidatas a una tanda de lenguaje posterior.

## 🛠️ IDE / herramientas

### Fase 1 del IDE — comodidad + AOT automático (decidida 24-jun; host-Java, SIN reflasheo de firmware)

El usuario es quien más toca el IDE → comodidad + que el AOT "just works".

**AOT automático desde el IDE:**
- Targets Fase 1 = **ARM: RP2350/Pico + STM32**. **ESP32 (S3/P4) → V4** (port del loader `.mdn` a Xtensa/RISC-V).
- **Elección de target POR PROYECTO** (sección `aot` en `.bpbuild`: `enabled`/`target`/`optLevel`/`flags`).
- Rutas de toolchain **por máquina** (IdePrefs: auto-detect PATH + carpetas estándar + override) + **panel Settings→AOT** (extensible para "lo que vaya surgiendo").
- Pipeline (FrmMain `doRunOnDevice` ~1765): detectar `function native` → `AotCEmitter` (Java in-IDE) → gcc del target (ProcessBuilder) → `MdnPack` (Java) → subir `.mdn` con el `.mod`.
- **Native NO AOT-able → WARNING + continúa interpretado (NO aborta).** "Para eso el `.mod` siempre se genera" (Eduardo).

**Comodidad del editor/IDE (peticiones de Eduardo, usuario diario):**
- **Comentar/descomentar** (toggle del prefijo de comentario BP sobre línea/selección).
- **Indentar/des-indentar** (Tab / Shift-Tab sobre selección).
- **Árbol de archivos remotos se CORTA con muchos módulos** → fix de scroll/layout (PicoExplorer).
- **Barra de botones superior (toolbar)** con las funciones más habituales.

**Fase 2 del IDE (más adelante, "algún añadido más"):** TBD. Candidato fuerte: **repaso del AOT de una pasada** (casts `byte()/int()…` + `^` en native → más funciones AOT-ables). (AOT ESP32 = V4.)

---

- **N-ide-aot-button — AOT integrado en el IDE**: hoy el `.mdn` se compila por CLI
  aparte; que el Run/Build lo haga, con detección del toolchain ARM y flags por
  target. Liga con #258.
- **IDE multiplataforma (Windows + Linux)**: migrar el puerto serie
  `purejavacomm → jSerialComm`, lanzador `.sh`, rutas/asunciones de toolchain.
  Workstream propio, independiente del lenguaje/VM.
- **describePc remoto** (el call-stack del debugger muestra `PC=<n>`; falta
  `req: describePc(pc)` en el wire) · **multi-run en el daemon** (hoy 1
  ejecución/proceso) · **aprovisionamiento del device** (subir stdlib/`BpVM.cfg`).
- **Ver el diagnóstico de reset desde el IDE (H10 → IDE)** *(idea de Eduardo, 21-jun)*:
  mostrar en el botón **INFO** (o un comando/panel "Diagnóstico") la **causa del último
  reset** + **boot count** + el **rastro de migas** (`markAt(0)` = causa original).
  La fontanería del runtime YA está hecha en H10 (builtins `Pico.resetCause`/`bootCount`/
  `markCount`/`markAt`, ids 201-205; verificados en placa). Falta solo el lado IDE: que
  el reply de `INFO` del wire (`BpvmClient.requestInfo` → Map; el handler INFO del
  firmware) incluya esos campos y el panel los pinte → diagnosticar un cuelgue de campo
  **sin escribir un .bp**. Encaja con la "observabilidad de bring-up" (más abajo).

## 🧱 Portabilidad / multi-micro

- **#258 — Árbol familia/micro/placa + imagen por familia + target por proyecto**
  (`board_desc` data-driven, detección por wire). Base en `micros/`. **Habilita el
  rollout de kits gráficos** (incluido el ESP32-P4, que es RISC-V → port nuevo).
- **Capa de port + observabilidad de bring-up** (miga de pan en RAM de backup,
  boot por etapas con checkpoint, self-test de port, `PORTING.md`, "encoger el
  rojo"). Diseño completo en `V3_IDEAS.md` §6. Madura justo antes de bajar el GUI
  al micro.
- **#138 — Multiplexar USB CDC** en streams distinguibles.
- **P-adc-8ch**: `Adc.bp` valida `0..3`; exponer el nº de canales por placa
  (`Pico.adcChannels()`, como `gpioCount()`) para los 8 del RP2350B. Liga con #258.
