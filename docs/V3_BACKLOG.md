# BasicPlus — Backlog de V3

> **Fuente única del backlog de V3.** Las *tasks* del IDE/sesión son efímeras
> (el trabajo en curso de cada sesión); **este doc manda**. Visión y principios:
> `V3_ROADMAP.md`. Charlas de diseño: `V3_IDEAS.md`. Al cerrar un ítem se anota
> su resultado aquí y, cuando toque, pasa a un futuro `HECHO_V3.md`.
>
> Estado: `pendiente` / `en curso` / `cerrado`. (`#NNN` = id de task histórica.)

---

## 🎨 GUI (objetivo cabecera)

El camino crítico —upcall C→BP → `Gui.*` en miVM → VM-C host (LVGL+SDL) →
portabilidad → micro → cross-family— se detallará en los hitos H1+ (ver
`V3_ROADMAP.md` §6 y la decisión consolidada en `V3_IDEAS.md` §1). Las tareas
concretas se crean al arrancar cada hito.

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
- **Compresión `deflate`-lite** (LZSS → LZ77+Huffman) y/o formato Archive
  multi-fichero. **Aditivo**: `Compress.deflate` nuevo, sin tocar `decompress`
  (LZSS) — hay código V2 que depende (principio 7).

## ⚙️ Compilador / lenguaje (ampliación selectiva — "eventos y poco más")

### H7 — 2 cambios de lenguaje (Eduardo, 20-jun; para esa tarde)

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

- **Continuación de línea** (Eduardo, 20-jun; **decisión pendiente — su duda era "qué
  operador"**). La continuación IMPLÍCITA dentro de `()`/`[]` YA existe (#249/L5) →
  hoy una expresión larga se parte envolviéndola en paréntesis. Falta el caso fuera de
  corchetes. Opciones: **(A) operador colgante** — si la línea acaba en operador
  binario / coma / `:=` / `.` / `and`/`or`, continúa, SIN carácter especial (extiende
  la misma supresión de newline de #249; lo más ergonómico, y responde literalmente a
  "qué operador": ninguno). **(B) `\` final** explícito (limpio: no choca con el `_`
  separador ni con el escape de strings, que solo aplica dentro de comillas). **NO usar
  `_` final** (estilo VB) — se solaparía visualmente con el separador nuevo. Recomendado:
  (A); (B) si se prefiere marcador explícito.

- **#169 — AOT cross-module sin puente del intérprete** (MEJORA de rendimiento;
  hoy FUNCIONA vía `call_bp` + warning). Diseño en `AOT_CROSS_MODULE.md`.
- **Layout compacto de narrow** (L10 follow-up): `byte[]`/`int16[]` y globales
  narrow con storage real (hoy viven como i32; la maquinaria de la VM ya existe).
- **Fixed arrays `tipo[N]`** en campos de clase y en `native` (hoy error honesto;
  locales y de módulo ✅ en V2).
- **N-pubvar-warn**: `public var` de módulo se ignora en silencio → avisar
  ("usa property/const"). La gramática ya documenta la regla.
- **N5 v2 / condvar real** en `SyncList` (hoy spin-poll de 1 ms).

## 🛠️ IDE / herramientas

- **N-ide-aot-button — AOT integrado en el IDE**: hoy el `.mdn` se compila por CLI
  aparte; que el Run/Build lo haga, con detección del toolchain ARM y flags por
  target. Liga con #258.
- **IDE multiplataforma (Windows + Linux)**: migrar el puerto serie
  `purejavacomm → jSerialComm`, lanzador `.sh`, rutas/asunciones de toolchain.
  Workstream propio, independiente del lenguaje/VM.
- **describePc remoto** (el call-stack del debugger muestra `PC=<n>`; falta
  `req: describePc(pc)` en el wire) · **multi-run en el daemon** (hoy 1
  ejecución/proceso) · **aprovisionamiento del device** (subir stdlib/`BpVM.cfg`).

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
