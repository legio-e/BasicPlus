# H9 — Kernel por capas / arranque escalonado (charla de diseño, 18-jul)

> Charla de diseño (método *analizar-antes-de-programar*). Fija el modelo de un
> **hito NUEVO H9** propuesto por Eduardo (18-jul): re-estratificar el arranque
> del firmware en **capas recuperables** sobre un **núcleo mínimo siempre-vivo**.
> Da a las **particiones** la importancia que merecen —pasan a ser el *pivote*
> del boot, no un detalle enterrado— y es el **cimiento que H3 (Packs) necesita**.

## La motivación (Eduardo)

**Feedback cuando las cosas NO funcionan** — en placas nuevas o durante cambios
profundos (como el modelo de memoria de ahora). Hoy el boot es **monolítico**:
init flash/PSRAM → mount FS → init heap de la VM → levantar el wire → autorun,
todo acoplado. Si una etapa falla o se cuelga, el aparato se queda **mudo** (los
"cuelgues mudos en placa" que arrastramos desde H1). La idea: **un sistema mínimo
que SIEMPRE responde**, y el resto montado encima de forma que un fallo arriba te
deje caer a la **última capa buena DICIENDO por qué**, en vez de brickear en
silencio. Es el *fail-fast* de H1 subido al nivel del arranque.

Eduardo: *"si algo no funciona todavía, que mantengamos el sistema básico; pero
también poder manejar el sistema de particiones y a partir de ahí arrancar el
resto."*

> **✅ Núcleo host IMPLEMENTADO (18-jul):** 3 ladrillos verdes sin placa —
> `bpvm_env` (bloque de identidad, `1041fd1`), `bpvm_part` (tabla de particiones +
> validación + clamp #292, `e0e98c3`) y `bpvm_boot` (esta máquina de estados +
> STATE, `4660e8b`). `make test-env`/`test-part`/`test-boot` = 24/18/20 verde.
> Falta enchufarlo a placa (cintura de flash por-micro + transporte kernel-comm).

## El modelo: estados apilados y recuperables

Cada estado es **independientemente útil** e **independientemente recuperable**.
El arranque normal SUBE 0→1→2→3; un fallo BAJA al último estado bueno y **reporta**.

| Estado | Da | Necesita | Al fallar la capa de arriba |
|--------|----|----------|-----------------------------|
| **0 — Kernel** | comunicación básica | reloj/core (SDK) + 1 transporte + buffer **estático** | — (es el suelo) |
| **1 — Particiones** | gestionar la tabla de particiones | descriptor válido en sitio fijo de flash | te quedas en 0, "sin tabla de particiones" |
| **2 — FS** | copiar/listar/borrar archivos | mount de littlefs sobre su partición | te quedas en 1, "FS no montable" (host formatea) |
| **3 — VM/App** | ejecutar programas | heap (SRAM/PSRAM) + stdlib + autorun | caes a 2 (comms+FS vivos), "app/heap falló" |

- **Estado 0 (kernel).** Sin heap, sin FS, sin particiones. Un lazo de comandos
  diminuto con buffer **estático**. Su set mínimo: `PING`, `INFO/STATE` y —lo que
  lo hace todo posible— **lectura/escritura/borrado de flash por dirección
  ABSOLUTA**. Eso es lo que permite *manejar el sistema de particiones desde el
  suelo*. Superficie de fallo casi nula → más robusto que el boot de hoy.
- **Estado 1 (particiones).** Lee y valida el descriptor de particiones (sitio
  fijo, fuera del FS — ya diseñado en `particiones-flash-estado-pre-h3`). Corrupto
  o ausente → te quedas en 0 y el host lo escribe. **Aquí las particiones dejan de
  ser una precondición enterrada y pasan a ser algo que se gestiona desde el
  suelo** — que es el objetivo de "darles la importancia que merecen".
- **Estado 2 (FS).** Monta littlefs en su partición. Falla → te quedas en 1 y el
  host formatea. (El FS ya existe; es el *mount* el que hoy puede colgar.)
- **Estado 3 (VM + app).** Init del heap, stdlib, autorun. Crash → caes a 2 con
  comms + FS vivos y reportas. Aquí vive el *"la app petó pero el aparato sigue
  alcanzable"*.

## Corolario: el descriptor de PLACA es un artefacto del kernel, no un fichero del FS (Eduardo, 18-jul)

Hoy `board.json` (qué micro, tamaño de flash, si hay PSRAM y cuánta/en qué pin,
GPIO…) vive en `/sys/board.json` = **el FS**, una capa de ARRIBA (estado 2). Pero:

- **Las particiones (estado 1) NECESITAN el tamaño de flash** para trazar regiones.
- **La memoria (estado 3) NECESITA la PSRAM** (presencia/tamaño/pin) para el heap.

Ambos son **hechos de placa** → el descriptor debe ser legible **en el suelo**, ANTES
de esos estados. Montar memoria/particiones con defaults y *corregirlos luego* con un
fichero del FS es tarde: es exactamente lo que nos mordió (clamp del tamaño de flash de
la Metro; PSRAM apagada hasta escribir `board.json` — [[esp32-p4-memoria-psram]] /
[[placa-p4-pantalla-flexibilidad-hw]] / [[firmware-rp2350-imagen-unica]]).

**Por tanto: el descriptor de placa es de la MISMA categoría que el descriptor de
particiones** — identidad "quién soy / cómo estoy hecho", en sitio fijo de flash, leído
por el kernel (estado 0/1) con flash raw, escribible por el host vía kernel-comm. No hay
bootstrap nuevo: leer un bloque en un offset conocido no requiere saber nada de la placa;
el transporte del estado 0 es fijo por familia de firmware. Bonus:

1. **Sobrevive a las operaciones de FS** (está fuera de la partición del FS, como la
   tabla) → un formateo o un burn de pack ya no borran la identidad de la placa (se acaba
   el "re-ejecutar `SetBoardMetro` tras tocar el FS").
2. **Poner en marcha una placa nueva pasa a ser una operación del SUELO:** el host escribe
   "Metro, 16 MB flash, 8 MB PSRAM CS47" por kernel-comm ANTES de FS/memoria → reinicio →
   el kernel lo lee → estados 1 y 3 se montan BIEN a la primera. Es el "feedback en placas
   nuevas" que motiva H9.
3. **Refuerza la imagen única:** el mismo `.uf2` sigue eligiendo variante en runtime, pero
   lee su identidad de la flash en el suelo, no como fichero corrector.

**Decisiones (18-jul):** (a) ¿descriptor de placa y tabla de particiones JUNTOS o
separados? → **JUNTOS**, un env único (la tabla de particiones son entradas más). (b)
¿`board.json`-el-fichero desaparece o queda como espejo? → ✅ **DESAPARECE.** Una sola
fuente de verdad = el bloque de env en flash; ni espejo ni copia en `/sys`. El **IDE es el
EDITOR** (escribe el env por kernel-comm, A/B); el device solo LEE. Si el usuario quiere
cambiar tamaños de particiones lo hace desde el IDE → **escribe env → reinicia → el device
sube con el layout nuevo** (no se reparticiona en vivo con el FS montado sobre el layout
viejo); si algo falla, se queda en estado 0/1, recuperable. Es justo el "cambio profundo"
que hoy podría brickear y que H9 hace seguro.

### Formato del bloque de env (DECIDIDO 18-jul — Eduardo delegó el formato) — ✅ IMPLEMENTADO (host, `1041fd1`)

> **Primer ladrillo de H9 HECHO (host):** `bpgenvm-c/{include/bpvm_env.h,src/bpvm_env.c}` +
> `test-env` (24/24 verde). Formato + parse + A/B (`bpvm_env_pick`/`next_seq`) + get/get_bool/
> get_long + serialize, dependencia mínima (solo crc32, sin VM/heap). La **cintura de flash
> por-micro** (leer/borrar/escribir el sector) NO está — llega en su fase (placa).


Zona pequeña de flash **a offset fijo, a piñón** (propuesta de Eduardo). Es una **zona de
"variables de environment"**: no JSON, sino **líneas `clave=valor\n`** (modelo *env* de
U-Boot). Razón: el kernel lo parsea **en el estado 0, SIN heap y robusto ante corrupción**;
un parser JSON ahí es código y superficie de fallo de más, mientras que `clave=valor` se
lee con un bucle trivial sin alocar y es **abierto por construcción** (clave desconocida →
se ignora; añadir entradas nunca rompe a un kernel viejo). Todo cabe como string:
`flashSizeBytes=16777216`, `psram=1`, `psramCsPin=47`, `gpioCount=48`.

Marco del bloque (fiabilidad, no solo texto):
```
[magic "BPEV" 4B][version u16][len u16][crc u32][seq u32]
[payload: clave=valor\n ...][relleno 0xFF hasta fin de sector]
```
- **magic** distingue env válida de flash borrada (0xFF); **crc** (reusar el CRC-16/CCITT
  del formato de packs, por consistencia) sobre el payload → bloque a medio escribir se
  rechaza y se cae a defaults de compilación + kernel-comm lo reporta; **seq** para el A/B.
- **A/B desde el principio** (dos sectores): seguro barato contra corte de corriente a mitad
  de escritura (= "nunca brickear"). El kernel elige la copia con `seq` mayor que pase CRC;
  actualizar = escribir en la copia rancia. **Nota física:** el borrado es por **sector de
  4K**, así que aunque el contenido sea ~2K cada copia ocupa un sector; A/B ⇒ 8K totales
  (minúsculo).
- **⚠️ Posición: offset fijo BAJO, justo tras la región reservada de la imagen — NUNCA
  relativo al FINAL de la flash.** El tamaño de flash está DENTRO del env → la posición del
  env no puede depender de él (huevo-y-gallina). El offset es una constante por familia,
  compartida kernel↔host, y es **una entrada más del `FLASH_LAYOUT` por micro** (el "solo
  falta tamaños por micro" de [[particiones-flash-estado-pre-h3]]).
- **Un solo mecanismo en el suelo:** como es un env abierto, la **tabla de particiones vive
  como entradas del mismo bloque** — pero **SOLO tamaños** (`part.fs.size=…`,
  `part.packs.size=…`), nunca offsets. Cierra hacia "junto y simple" de la decisión (a). Solo
  se gradúa a un descriptor binario aparte si crece.
- **⚠️ Modelo de particiones (CORREGIDO 18-jul, Eduardo):** las particiones son un **conjunto
  FIJO y ORDENADO** — las que son, en su orden. El usuario **NO las crea/borra**; lo único
  editable es el **TAMAÑO**. Por eso los **offsets se DERIVAN** (contiguos desde la base, en
  orden) y NO se guardan: no pueden contradecir al orden y **no hay solapes por
  construcción** → una sola fuente de verdad. La 1ª vez (env sin tamaños) el asistente
  **propone defaults**, el usuario ajusta y confirma. **✅ IMPLEMENTADO (host, `e0e98c3` →
  reescrito `9e59895`):** `bpvm_part.{h,c}` + `test-part` (21/21) — enum fijo `BPVM_PART_FS/
  PACKS`, `bpvm_part_defaults`, offsets derivados, fachada kind→región, validación reducida a
  lo que aún puede fallar (ZERO/UNALIGNED/OVERFLOW) + clamp #292; falta un tamaño →
  `BPVM_PART_ERR_MISSING` (virgen, no error). Subsumirá el `bp_ptable_t` binario de
  `fs_lfs_pico.c` (B2.b). Cintura de flash por-micro aparte (fase de placa).

### Acceso desde los programas BP (DECIDIDO 18-jul) — las dos APIs, en capas (fachada, no duplicación)

- **Base: API genérica de env** — `Env.get("psramCsPin")`, `Env.has(...)`, `Env.list()`.
  Abierta por construcción, igual que el bloque: una entrada nueva es legible desde BP sin
  tocar nada (el `getenv()` de toda la vida). Valores string.
- **Encima: funciones de sistema TIPADAS** para el puñado de hechos universales —
  `Board.flashSize(): long`, `Board.hasPsram(): boolean`, `Board.psramSize(): long`,
  `Board.gpioCount(): integer`, `Board.name(): string`. **Delegan** en la genérica
  (`hasPsram()` = `Env.get("psram")=="1"`), no recalculan → NO pueden divergir de la fuente.
- **Una sola verdad, sin skew:** el runtime lee de una **copia en RAM que el kernel rellena
  UNA vez al arrancar** desde el env; la flash es la única verdad, la RAM es caché derivada
  de solo-lectura. Desde BP es **SOLO LECTURA** (escribir el env es privilegio del IDE).
- **Re-hospeda lo existente:** las funciones de identificación del módulo `Pico`/MCU
  ([[esp32-p4-memoria-psram]] / P-mcu-module) y el botón **INFO del IDE** (micro/flash/RAM/
  PSRAM, #230) pasan a leer del MISMO env → mueren como fuentes separadas que podrían
  discrepar. Reparto: tipadas para lo universal (nombre, flash, PSRAM, GPIO); `Env.get` para
  la cola larga board-específica (dirección I2C de un sensor horneada en la placa, un pin
  raro). *(Nombre exacto de los módulos `Env`/`Board` = detalle a alinear con el módulo MCU
  existente para no fragmentar.)*

## Layout de flash en 3 ZONAS (DECIDIDO 19-jul, charla Eduardo) — el env al PRINCIPIO, en hueco no-grabado

Eduardo: la imagen tiene **dos zonas de código** con una **zona en blanco** entre medias;
el env va **al principio, a dirección FIJA**, reservando el espacio en la imagen, de modo
que NUNCA haya que moverlo aunque la imagen crezca.

| Zona | Rango (RP2350) | Qué es |
|---|---|---|
| **1 arranque** | `0x000000–0x00FFFF` (64K **FIJOS**) | vectores + cabecera picobin + arranque; "salta" a la zona 3 |
| **2 kernel** | `0x010000–0x013FFF` (16K) | **env A** (4K) + **env B** (4K) + reserva (8K: flag modo-seguro, futuro) — **HUECO del UF2, no se graba NUNCA** |
| **3 firmware** | `0x014000–0x1FFFFF` (~1,9 MB) | el firmware real (hoy ~0,5 MB) |
| FS | `0x200000–…` | littlefs (como hoy; luego conducido por las particiones H9) |

- **Por qué funciona:** (a) la zona 1 es una **región del linker con LENGTH fijo** → si el
  código de arranque crece, el build **FALLA gritando** (region overflow); la dirección del
  env es eterna. (b) la zona 2 **no tiene secciones** → el UF2 no contiene bloques ahí → el
  flasheo BOOTSEL no borra esos sectores → **el env sobrevive a cada reflasheo del firmware**.
- **Es el modelo nativo de ESP-IDF** (bootloader | tabla | app) → estandariza el MISMO
  layout en las 3 familias. Portabilidad.
- **64K de zona 1, deliberadamente generosos** ("el tamaño del boot no es un problema,
  64K de 4M" — Eduardo): hoy solo arranca y salta; mañana es el sitio del **kernel-comm
  REAL del estado 0** sin mover ninguna dirección.
- **Variante A primero (decidido, "poco a poco"):** UN solo build partido por el linker
  (el "salto" es el vector de reset apuntando a código de la zona 3). La evolución a B
  (dos programas: la zona 1 **valida** la zona 3 con magic/CRC y, si está corrupta, se
  queda en el suelo respondiendo) NO mueve direcciones — se hace cuando toque.
- Migración: el env deja `0x3FD000/0x3FE000` (posición provisional del primer adaptador);
  el contenido actual se pierde (fase de desarrollo, asumido).

### Los 2 huecos de H9 que quedan (Eduardo, 19-jul)

1. **Identidad en el SUELO — "que se lea el procesador y el tamaño de la flash de verdad".**
   La detección ya existe (SYSINFO.PACKAGE_SEL + JEDEC, board_desc.c) pero corre DENTRO de
   `board_desc_init`, DESPUÉS de montar el FS (por board.json). Hay que bajarla a
   **antes-del-FS** y que sea LA fuente de identidad (env/bmgr). Abre el cierre bueno de
   #292: subir `PICO_FLASH_SIZE_BYTES` a 16 MB y que el guardián pase a ser el **clamp
   RUNTIME por JEDEC** (la verdad detectada sustituye a la constante mentirosa).
2. **PSRAM por env.** Hoy NADIE lee el env al arrancar (solo al llegar un comando del IDE)
   y el sondeo PSRAM (H7.2.a) cuelga de `board.json:psramCsPin`. Decidido: el arranque lee
   el env por XIP (memcpy, sin FS); **`psram=1` + micro RP2350B → CS = GPIO47 derivado**
   (el último pin). **Solo RP2350B de momento** — técnicamente posible en la A pero no se
   conoce placa que lo lleve; si aparece, se añade la posibilidad. Init TEMPRANO pre-FS →
   la distribución de memoria se decide antes de montar nada. Primer clavo del ataúd de
   board.json.

## Las DOS capas de comunicación (refinamiento de Eduardo)

La comunicación no es monolítica: son **dos juegos de comandos sobre UN solo
transporte físico** (mismo framing/CRC/chunking = `comm_common`, ya abstraído).

1. **kernel-comm** — mínima, **presente desde el estado 0 y SIEMPRE**. Solo lo
   imprescindible para traer el sistema y recuperarlo: `PING`, `STATE` (¿en qué
   estado estoy y por qué?), flash raw (read/write/erase por dirección), y las
   transiciones (escribir tabla de particiones, "quédate en el suelo").
2. **full-comm** — **completa, se registra cuando memoria + FS están arriba**
   (estado 2/3): copiar archivos (PUT/GET/LS), ejecutar programas (RUN/KILL/
   autorun), debugger, upload de módulos… Es esencialmente el wire rico de hoy.

**Mecanismo:** un solo dispatcher físico con **dos tablas**: la del kernel
(diminuta, siempre) y la completa (registrada al llegar a estado 2/3). Un comando
que necesita una capa aún no disponible **devuelve un error limpio "no disponible
en el estado actual"** — nunca cuelga ni corrompe. `STATE` es el latido del bucle
de feedback: el host **siempre** sabe con qué está hablando.

### Comandos de gestión de placa (kernel-comm) — ✅ NÚCLEO IMPLEMENTADO (host, `a104961`)

El puñado de verbos que el IDE (FrmBoard) usa para ver/editar entorno y particiones.
Van en **kernel-comm** (deben funcionar en el suelo, es donde se recupera una placa
virgen). Framing = wire v1 de siempre (`{"type":..,"id":N,..}` + reply `_REPLY` /
`ERROR`). El firmware es un **adaptador fino**: parsea el JSON, llama a la función
`bpvm_bmgr_*` correspondiente, formatea el reply. Toda la lógica (y sus pruebas) vive
en `bpvm_bmgr` (host-testable, `test-bmgr` 30/30).

| Verbo | Argumentos | Reply | `bpvm_bmgr_*` |
|---|---|---|---|
| `STATE` | — | `{state,name,degraded,reason}` | (bpvm_boot_state_report) |
| `ENV_LS` | — | lista `{key,value}` | `env_count` + `env_pair_at` |
| `ENV_GET` | `key` | `{value}` o ERROR NOT_FOUND | `env_get` |
| `ENV_SET` | `key,value` | OK (+ vuelca slot) | `env_set` |
| `ENV_DEL` | `key` | OK (+ vuelca slot) | `env_set(value=NULL)` |
| `PART_LS` | — | tamaños+offsets derivados, o `MISSING` (virgen) | `part_layout` |
| `PART_DEFAULTS` | — | tamaños sugeridos (1ª vez) | `part_defaults` |
| `PART_APPLY` | `sizes[]` | OK (+ vuelca slot) o error de validación | `part_apply` |

**Notas de contrato** (cerradas por el modelo de particiones y el bloque A/B):
- **No hay `PART_ADD`/`PART_DEL`.** Las particiones son un conjunto fijo; "crear" =
  `PART_APPLY` con los tamaños (defaults la 1ª vez). Los offsets NUNCA viajan (derivados).
- `PART_APPLY` es **transaccional**: valida primero; si falla, **no toca el env** y
  devuelve el error + índice culpable. No reparticiona en vivo → el device se reinicia y
  sube con el layout nuevo (si algo va mal, se queda en estado 0/1, recuperable).
- `ENV_SET`/`PART_APPLY` re-serializan a la copia A/B **rancia** con `seq+1`; el device
  solo tiene que volcar el sector `wrote_slot` a flash (la cintura por-micro). Corte de
  corriente a mitad = seguro (la copia actual sigue válida hasta que la nueva pasa CRC).

## Flujo de bring-up (IDE ↔ FrmBoard) — placa virgen (charla 18-jul)

Es la máquina de estados de H9 asomando en el IDE. Una **placa recién flasheada = estado 0**
(kernel, sin particiones → sin FS → sin VM).

1. **Al conectar, el IDE manda `STATE`** (el "escrutinio"; NO inferir "árbol vacío ⇒ virgen":
   un árbol vacío también es una placa sana sin ficheros — `STATE` desambigua).
   - **estado < 3** → abre/propone **FrmBoard** en modo setup/recuperación.
   - **estado 3** → operación normal; FrmBoard se abre a mano con el botón **INFO**.
2. **Auto-abrir si VIRGEN** (nada que perder); **PROPONER si DEGRADADA** ("FS no se pudo montar,
   ¿reparar?") — puede haber datos/intención, o el usuario solo quiere reflashear.
3. En FrmBoard **la conexión ya existe → sin conectar/desconectar** (comparte el puerto). INFO
   evoluciona de popup a "abre la ventana de placa" (y su info la lee del env).

### El asistente de primera conexión — orden OBLIGATORIO: entorno ANTES que particiones

No es preferencia: es el corolario. Las **particiones necesitan el tamaño de flash** y la
**memoria la PSRAM** → ambos hechos de placa que hay que fijar primero.
1. **Identidad/entorno** (flash size, PSRAM sí/no + tamaño/pin, GPIO…). Sin esto el paso 2 no
   puede sugerir tamaños sensatos.
2. **Particiones**: sugiere layout por defecto *ya sabiendo el tamaño de flash*; el usuario ajusta;
   valida (caben, alineadas al sector de borrado, sin solapes).
3. **Formatea el FS** en su partición → estado 2.
4. Escribe env (A/B) + tabla + **reinicia** → sube a estado 3, aparece el árbol normal.

**Afinados:**
- **Auto-detectar lo detectable, preguntar el resto.** El kernel (estado 0) lee el **JEDEC de la
  flash** (tamaño real) y **sondea la PSRAM** → pre-rellena el asistente. Es lo que habría evitado
  el mordisco Pico/Metro (declarábamos el tamaño a mano y mentía). Lo no detectable lo pregunta
  apoyándose en el **catálogo de placas del IDE** (eliges "Metro RP2350B" → pre-rellena
  flash/PSRAM/GPIO/layout); si el JEDEC contradice la elección, avisa.
- **Validar antes de escribir + red de seguridad:** rechaza un layout imposible *antes* de tocar
  flash; si algo sale mal tras el reinicio, vuelve a estado 0/1 y FrmBoard se reabre → recuperable.
  El asistente es seguro de experimentar porque H9 lo hace irrompible.
- **El mismo asistente reconfigura** (no solo el 1er arranque); los botones crear/borrar partición
  son la versión "a mano". Aviso: cambiar el tamaño de la partición del FS ⇒ reformatear (pérdida
  de datos).
- **Botones de FrmBoard** (sin cerrar, "ya veremos"): crear/borrar partición, formatear FS,
  editar/escribir entrada de env, botón del asistente; más adelante Burn/borrar pack (H3). **No**
  hay conectar/desconectar.

## Invariantes (lo que hace que esto funcione)

1. **El estado 0 es un suelo SIEMPRE alcanzable.** Ninguna capa de arriba puede
   pisarlo (ver decisión #1).
2. **Todo `init` es FALIBLE y NUNCA cuelga:** devuelve error → se reporta → se
   queda abajo. (La disciplina de H1: gritar, no colgar.)
3. **Ante fallo, se cae al último estado bueno reportando el motivo** — no se
   reintenta a ciegas ni se brickea.
4. **El host CONDUCE las transiciones** (escribe la tabla desde 0, formatea desde
   1, arranca la app desde 2). El device ofrece el mecanismo; el host decide.

## Decisiones load-bearing (a cerrar ANTES de programar)

1. **Supervivencia del estado 0.** Para que "mantener el sistema básico" sea
   real, arriba no debe poder pisar el 0. La buena noticia: **es para lo que
   sirven las particiones.** Si el gestor garantiza que un formateo de FS o un
   burn de pack solo escriben *en su partición* (nunca en la región de código),
   entonces una **app mala o un FS corrupto NO pueden brickear** — solo un reflash
   completo, y ese ya se recupera por BOOTSEL/DFU. Historia limpia, sin bootloader
   aparte. *(Alternativa más dura: estado 0 en región de flash protegida tipo
   mini-bootloader → abre "¿quién actualiza el estado 0?". Empezar por la ligera.)*
2. **Robustez del descriptor de particiones.** Sitio fijo + **CRC** + **copia A/B**
   → una escritura a medias no te deja tirado. Pequeño pero es el pivote del boot.
3. **Detección de cuelgue + máquina de estados + flag de "modo seguro".**
   - Si el estado 3 se cuelga (bucle infinito de un programa), kernel-comm debe
     seguir vivo para atender un `KILL` y volver a 2 (ya medio hecho: KILL #257 +
     comm task en core/prioridad aparte). El **watchdog** (Wdt) es el respaldo del
     cuelgue duro.
   - **Flag persistente de "no subas, quédate en el suelo"** que el host pueda
     poner: si un autorun brickea el ascenso, se recupera forzando arranque a
     estado 0. Es el "modo seguro" clásico, pero por flash en vez de por botón.
   - El **autorun (#256) se re-encuadra**: "estado 3 sube solo si está sano"; un
     fallo previo o el flag de modo seguro lo inhiben.

## Descomposición / secuencia sugerida

- **H9.0 — Kernel (estado 0).** Transporte + buffer estático + lazo de comandos +
  flash raw. **Entregable POR SÍ SOLO** (canal de recuperación imposible de
  brickear), no depende de nada. Es el mejor primer escalón.
- **H9.1 — Gestor de particiones (estado 1)** sobre la fachada ya acordada:
  leer/validar/escribir el descriptor (A/B + CRC), gestionar regiones.
- **H9.2 — Mount de FS falible (estado 2)** + formateo conducido por el host.
- **H9.3 — Init de VM/heap/app falible (estado 3)** + autorun gateado por salud +
  flag de modo seguro.
- **H9.4 — Dispatcher full-comm** registrado al llegar a 2/3; kernel-comm siempre
  presente; comando `STATE`; **el IDE aprende a hablar con un device degradado**
  (modo recuperación: escribir tabla / formatear / reflashear) — pieza vecina de H8.
  **La ventana IDE de esto es `FrmBoard` = H8.a** (esqueleto ya creado 18-jul: 3×2 con
  pestañas env/packs, comms con la tabla de particiones, consola; comparte conexión con
  la ventana principal). Registrada, **UI sin cablear hasta que existan estos backends**.
- **H9.5 — Watchdog/hang-detection** como respaldo + KILL desde full-comm.
- **Transversal — cintura por micro:** transporte (ya abstraído #137), flash raw
  (ya hay `flash_range_*` RP2350 / `esp_flash`-`esp_partition` ESP32 / HAL STM32),
  fachada de particiones (ya diseñada). El trozo por-familia es **pequeño y casi
  todo existe**.

## Relación con lo existente

- **Subsume/re-encuadra** `particiones-flash-estado-pre-h3` (la fachada
  `bpvm_part.h` pasa a ser el gestor del estado 1) y es **prerequisito de H3
  (Packs)** — un pack es una región gestionada por particiones; no hay packs en
  serio sin gestor de particiones, y H9 *es* ese gestor manejable desde el suelo.
- Es el **hogar sistémico del fail-fast** de H1: convierte "brickeo silencioso en
  boot" en imposible-por-construcción.
- Reusa: transporte abstraído (#137), KILL end-to-end (#257), Wdt, autorun (#256),
  el wire v1 rico (pasa a ser full-comm).

## Verificación

- **Oráculo de inyección de fallos EN HOST:** el host puede simular las capas
  (modo "sin particiones", "FS no montable", "heap no inicializa") y probar la
  máquina de estados + la retirada + los mensajes **sin placa** — misma filosofía
  host-como-oráculo del dual-VM. La verificación en silicio (por etapas y por
  familia) va en tandas de placa, que Eduardo agrupa por caras en tiempo.
- **El estado 0 es un hito entregable y verificable por sí mismo.**

## Estado

Propuesto y ACORDADO 18-jul (Eduardo prefiere este modelo al diseño actual: da
feedback ante fallos, en placas nuevas y en cambios profundos).

**Núcleo portable EN HOST — 4 ladrillos verdes (host-como-oráculo, sin placa):**
1. `bpvm_env` — bloque de env A/B (formato + CRC + pick + get + serialize +
   payload_set + enumeración + apply). `test-env` 34/34. (`1041fd1` + `9e59895` + `a104961`)
2. `bpvm_part` — particiones = conjunto FIJO y ORDENADO, offsets DERIVADOS, el
   usuario solo edita tamaños; defaults la 1ª vez; validación + clamp #292.
   `test-part` 21/21. (`e0e98c3` → reescrito `9e59895`)
3. `bpvm_boot` — máquina de estados 0→3, subida/retirada a último-bueno + `STATE`.
   `test-boot` 20/20. (`4660e8b`)
4. `bpvm_bmgr` — board manager: núcleo del protocolo de gestión de placa (ENV_*/
   PART_*), compone env+part, transaccional. `test-bmgr` 30/30. (`a104961`)

**IDE SIN PLACA — end-to-end verde (Eduardo escogió "mini-server C que reusa bmgr"):**
5. `boardsim` (`tools/boardsim.c`, `a8a35b2`) — servidor TCP wire v1 que despacha
   STATE/ENV_*/PART_* al `bpvm_bmgr` real; env respaldado por un fichero A/B = la
   "flash" (lo único simulado). `make boardsim` + `boardsim-smoke` (python, 21/21).
6. Cliente Java en `BpvmClient` (`82f4320`) — `boardState/envList/envGet/envSet/
   envDel/partLayout/partDefaults/partApply` + modelos. `BoardMgrSmoke` 8/8 vs el sim.
7. `BoardMgrPanel` + `FrmBoard` (`cfea355`) + menú Run→«Gestión de placa…» (`4cdd083`).
   Comparte la conexión de la ventana principal (`backend.debugClient()`).

**Cómo probarlo sin placa:** `make boardsim && build/bpvm-boardsim.exe 5099 board.flash`
→ en el IDE, conectar el explorador de dispositivo a `127.0.0.1:5099` (TCP) → Run →
«Gestión de placa…» → tablas de entorno/particiones en vivo (placa virgen: entorno
0 vars, particiones "missing" → Proponer defaults → editar → Aplicar).

**Adaptador de firmware YA EN CÓDIGO (18-jul, re-verificado 19-jul):** la cintura de
flash A/B + el adaptador JSON existen y están cableados — `pico/board_mgr_pico.c`
(lee A/B por XIP, trocea `s_put_buf`, despacha a `bpvm_bmgr_wire_dispatch`, vuelca
`wrote_slot` con erase+program bajo `bpvm_flash_lock`) enganchado en `repl_v1.c`
(STATE/ENV_*/PART_* → `board_mgr_pico_handle`) y compilado en el UF2 (`786eb66` +
`a262537`). Env provisional en `0x3FD/0x3FE000` → se muda al layout de 3 zonas.

**Pendiente (ladrillos acordados 19-jul, en orden, "poco a poco"):**
1. **Layout 3 zonas** (linker partido + env a `0x010000/0x011000` + hueco del UF2).
2. **Identidad en el suelo** (PACKAGE_SEL + JEDEC pre-FS → fuente de env/bmgr).
3. **PSRAM por env** (leer env al arrancar + `psram=1` ∧ RP2350B → CS=GPIO47 + init temprano).
4. **Verificación interactiva de Eduardo** (Pico + Metro, una tanda): FrmBoard contra
   placa real + persistencia del env tras reinicio Y tras reflasheo del firmware.
Después: máquina de estados conduciendo el boot real de main.c; unificación
particiones-H9 ↔ FS real (subsumir `bp_ptable_t`); refinar FrmBoard (rejilla 3×2 +
asistente de 1ª conexión). Opcional host: descriptor tipado `bpvm_board`.
