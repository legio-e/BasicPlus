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

### Formato del bloque de env (DECIDIDO 18-jul — Eduardo delegó el formato)

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
- **Un solo mecanismo en el suelo:** como es un env abierto, la **tabla de particiones puede
  vivir como entradas del mismo bloque** (`part.fs.offset=…`, `part.fs.size=…`); solo se
  gradúa a un descriptor binario aparte si crece. Cierra hacia "junto y simple" de la
  decisión (a).

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
feedback ante fallos, en placas nuevas y en cambios profundos). **Sin implementar
— registrado como charla de diseño.** Antes de programar: cerrar las 3 decisiones
load-bearing. Empezar por **H9.0 (kernel/estado 0)**, que no depende de nada.
