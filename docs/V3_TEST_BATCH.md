# V3 — Batch de pruebas finales (checklist)

> Verificación pre-publicación de V3. Se organiza en **2 grupos** (Eduardo 4-jul):
> Grupo 1 = **regresión ligera** de las 4 placas NO gráficas; Grupo 2 =
> verificación **completa** (no gráfico + GUI) de las 3 placas CON pantalla.
> Marca `[x]` según avances; los fallos van al **Registro de hallazgos** del final.
> Bugs que salgan → se arreglan (código descongelado solo para eso). Publicar es
> DESPUÉS de este batch. Escalera de peldaños y matriz históricas: `H14_TEST_PLAN.md`.

---

## Puerta 0 — Paridad en el PC (gratis, ANTES de tocar placa)

- [x] `cd compat && ./compat.sh check` → **TODO VERDE** (16 PASS / 0 FAIL · opcodes OK ·
      emisión OK, 4-jul). Verifica que las VMs V3 y el frontend siguen byte-idénticos a
      los goldens de V2 (comportamiento + ids de opcode + emisión). Un rojo aquí =
      regresión real, y se ve sin gastar un flash.

Si la Puerta 0 está verde, el riesgo de las placas baja mucho: lo que quede es
plataforma (backend HW, boot, wire), no el núcleo.

---

## GRUPO 1 — No gráfico · regresión ligera

**Objetivo:** confirmar que V3 no rompió nada (que "todo sigue funcionando sin
cambios importantes"). NO es re-test exhaustivo de sensores — es un pase smoke por
placa. 3 firmwares · 4 placas.

**Paso 0 (Eduardo, por placa):** recompilar el firmware con las fuentes V3 y flashear.
**⚠ NO reutilizar imágenes viejas** (p.ej. el `.uf2` del 15/06): son anteriores a `^`,
`eval`, reset-cause-en-INFO, CRC skip-PUT y base-dir #268 del núcleo → falsas regresiones.
**Blobs stdlib embebidos regenerados** (eran rancios, pre-ADC/PWM del 26/06): Pico
`pico_mod.c` (1edb72c) y STM32 `stm32_mods.c` (9feee77); ESP32 ya estaba fresco. El
rebuild los recoge — no hace falta nada más por tu parte.

**Peldaños por placa** (de H14; marcar los que apliquen a lo que tengas cableado):

| Placa | Firmware | Boot+INFO | Ejec/OO (paridad) | GPIO (blink) | I2C | SPI | UART | reset-cause | autorun+Stop |
|---|---|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| **Pico 2** (RP2350A) | `pico` | [x] | [x] | [x] | —¹ | —¹ | —¹ | [x] | [x] |
| **Metro RP2350B** | `pico` (misma img) | [x] | [x] | [x]³ | —¹ | —¹ | —¹ | [x] | [x] |
| **ESP32-S3 DevKit** | `esp32` | [x] | [ ] | [ ] | [ ] | [ ] | [ ] | [x] | [ ] |
| **STM32 Nucleo-U575** | `stm32` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |

¹ **I2C/SPI/UART en Pico 2 y Metro (RP2350) = diferidos** a la placa donde el cableado sea cómodo (los
buses son código compartido; basta validarlos en una placa con los cacharros a mano).
**Pico 2 CERRADA (4-jul):** Boot+INFO · paridad `^`/`eval` · AOT (~99×) · GPIO · reset-cause
· CRC/deps/stdlib-fresca — todo ✅.

³ **GPIO de la Metro = NeoPixel** (`NeoDemo` ✅): su "LED" es el WS2812 onboard (GP25). El
`blink.bp` de LED plano hardcodea GP25 (LED de la Pico 2) → no aplica a la Metro. Ver registro.

**Notas por placa (lo que V3 cambió y conviene mirar de reojo):**
- **Pico 2**: INFO variante A (30 GPIO). `native` acelerada (AOT ARM) → probar `^`/`eval`
  en un sample compute y comparar con host.
- **Metro RP2350B**: subir `/sys/board.json` (variante B) → INFO debe mostrar 48 GPIO +
  **PSRAM 8 MB usada como heap** + NeoPixel. Es la placa donde el heap PSRAM importa.
  **✅ 4-jul (INFO):** RP2350B · 48 GPIO · 8 ADC · 16 MB flash · **PSRAM 8 MB** — **imagen
  única CONFIRMADA** (el MISMO `bpvm_pico.uf2` que la Pico 2; la variante la elige
  `/sys/board.json` vía `SetBoardMetro.bp`). PSRAM sondeada+habilitada en GP47.
  **Metro CERRADA (4-jul):** + paridad `^`/`eval` + AOT ~102× (heap PSRAM) + GPIO vía NeoPixel
  + **ADC fix verificado** (PicoInfo → `ADC channels: 8`). Buses I2C/SPI/UART diferidos; Stop =
  mismo binario que Pico 2 (allí ✅). Hallazgo abierto: el cuelgue con `/app` lleno (registro).
- **ESP32-S3**: wire por UART0 (bridge); consola/logs por USB nativo. `native`
  interpretada (sin AOT en Xtensa) — corre igual, sin ganancia.
- **STM32 Nucleo**: wire por VCP del ST-LINK; AOT ARM (probar `^`/`eval`). reset-cause
  vía `RCC->CSR`.

**Muestras sugeridas (no gráficas):** `Blink*`, `PicoInfo`, `GpioLoop`, `UartEcho_*`,
`SpiLoop`, `I2cScanOO`, `Bme280*`/`Bme688*` (si hay sensor), `RtcDemo`, `PwmCount`,
`AdcDemo`. Para paridad: cualquier sample de cómputo/OO corrido en host **y** en placa,
misma salida.

---

## GRUPO 2 — Gráfico · verificación completa

**Objetivo:** las DOS caras — lo no gráfico (como el grupo 1) **Y** lo gráfico
(GUI + táctil). 2 firmwares · 3 placas.

**Paso 0:** flashear la imagen gráfica correspondiente (P4 = `esp32p4`; DK2 = build
STM32 con LTDC). **La Waveshare hay que re-flashearla con la imagen FINAL (P6)** —
ahora tiene la de P5, sin la stdlib unificada.

**Parte NO gráfica** (mismo smoke que grupo 1):

| Placa | Firmware | Boot+INFO | Ejec/OO | GPIO | I2C¹ | UART | autorun+Stop |
|---|---|:--:|:--:|:--:|:--:|:--:|:--:|
| **P4 Function-EV** | `esp32p4` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| **Waveshare P4 4.3"** | `esp32p4` (misma) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| **STM32 Discovery DK2** | `stm32-dk2` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |

¹ En las P4, periféricos I2C **siempre en bus 1** (el bus 0 es del táctil).

**Parte GRÁFICA:**

| Placa | Pantalla enciende | Catálogo widgets | Color/fuentes | Formulario `.win` | Táctil | Rotación | Imagen única (board.json) |
|---|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| **P4 Function-EV** (ek79007, default) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] (sin json = EV) |
| **Waveshare P4** (st7701) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] (`SetDisplay.bp` → st7701) |
| **STM32 DK2** (LTDC) | [ ] | [ ] | [ ] | [ ] | [ ] | n/a² | n/a |

² La rotación (`Gui.setRotation`) no está en la DK2/LTDC en esta versión (avisa y sigue).

**Muestras gráficas:** `GuiColorDemo`, `GuiClickDemo`, `GuiCheckDemo`, `GuiValueDemo`,
`GuiTableDemo`, `GuiTabDemo`, `GuiImageDemo`, `GuiMsgDemo`, `GuiListKbd`, `FontLoadDemo`,
`GuiRotDemo` (rotación, solo P4), `samples/formdemo` (formulario `.win`), `SetDisplay.bp`
(elegir panel en la Waveshare).

---

## Cosas NUEVAS de V3 a confirmar (transversal, marcar donde aplique)

- [ ] **eth_init no-fatal** (P4): la imagen del EV arranca en una placa SIN PHY (la
      Waveshare) — ya visto en P1, reconfirmar tras el rebuild final.
- [ ] **Imagen única P4**: la MISMA imagen sirve a las 2 placas P4 vía `/sys/board.json`.
- [ ] **reset-cause e2e** (3 familias no-P4 + P4): forzar un reset (power-on, watchdog,
      botón) → INFO muestra la causa correcta.
- [~] **`^` (potencia) y `eval`** en placa: correr en las ARM (Pico/Metro/Nucleo — AOT) y
      en ESP32 (interpretado) — misma salida que host. **Pico 2 ✅ (4-jul):** H7Pow/H7Eval
      byte-idénticos al host; **AOT** confirmado con `fibobench` (native==interpretado=4160200,
      **~99×**: 108.4 s → 1.095 s, `.mdn` auto-subido por el IDE). **Metro ✅ (4-jul):** ídem
      byte-idéntico + AOT ~102× (112.1 s → 1.095 s); el tramo interpretado ~3% más lento que
      la Pico 2 = firma del **heap en PSRAM** (el nativo, idéntico, apenas toca heap).
      Pendiente Nucleo (AOT) + ESP32 (interpretado).
- [x] **CRC skip-PUT**: subir un `.mod` ya presente e idéntico → el IDE dice "contenido
      idéntico, salto PUT" (no re-sube). ✅ Pico 2 (4-jul): `Pico.mod ya en FS (2415 bytes,
      contenido idéntico), salto PUT`.
- [x] **missing-lib al wire**: correr algo cuya dep NO esté → el IDE resuelve/sube la lib,
      o da error claro con el nombre. ✅ Pico 2: Blink → resolvió y subió `Gpio.mod` a /lib.
- [x] **stdlib fresca en placa**: tras primer boot, `Pico.mod` etc. byte-idénticos a
      bpstdlib (skip-PUT lo confirma). ✅ Pico 2: `Pico.mod` = 2415 B = el fresco (el blob
      rancio regenerado se propagó al device; el skip-PUT por CRC lo confirma).

---

## Registro de hallazgos

_(Cada fallo: placa · peldaño · síntoma · ¿bug de V3 o de cableado/entorno? ·
commit del fix si lo hay.)_

- **Paso 0 · blobs stdlib embebidos rancios** (Pico + STM32) — al comprobar si el `.uf2`
  del 15/06 servía, se detectó que el `Pico.mod` horneado en los firmwares Pico y STM32
  era anterior al cambio ADC/PWM board-aware (26/06): `pico_mod` embebido ≠ `bpstdlib`.
  Mismo skew [[stdlib-mod-version-skew-oo-device]] que mordió en S3/P4. Regenerados con
  `regen_*_mods.sh` (Pico 1edb72c, STM32 9feee77); ESP32 ya fresco. El build no regenera
  el blob solo → hay que correr el regen antes de recompilar. Detectado en PC, sin gastar flash.

- **Pico 2 · INFO "PWM 24" = canales, no slices** (cosmético → V4, NO regresión) — el botón
  INFO muestra `PWM 24` (= 12 slices × 2 canales; `repl_v1.c:712` manda `pwm_slices*2` a
  propósito) mientras el sample PicoInfo y el descriptor dicen 12 slices. Ambos correctos,
  magnitudes distintas; la etiqueta "PWM" del IDE (`PicoExplorer.java:1255`) es ambigua.
  No es mis-detección de placa: la Pico 2 muestra GPIO 30 + ADC 4 = variante A correcta (la
  B daría 48/8). V4: armonizar la etiqueta ("PWM ch" o mostrar slices).

- **Metro · Blink no enciende nada** (NO bug, → V4 board-aware LED) — `samples/blink.bp`
  hardcodea GP25 (LED onboard de la **Pico 2**); en la **Metro GP25 es el NeoPixel**, no un
  LED plano (su LED sería GP23, `ledPin` del board.json, "a confirmar con esquemático"). Blink
  hace on/off del pin del NeoPixel → nada visible. Hueco de fondo: board.json declara `ledPin`
  pero NADIE lo lee (sin `Pico.ledPin()` board-aware; el heartbeat del firmware también
  hardcodea `PICO_DEFAULT_LED_PIN=25`) → unificación board-aware = V4. En la Metro el "LED" =
  NeoPixel, así que **`NeoDemo` cubre el peldaño GPIO** (³ en la tabla).

- **Metro · CUELGUE con `/app` lleno de mods** — 🔴 **BUG REAL CONFIRMADO, ABIERTO.** Síntoma
  (Eduardo, 4-jul): con "muchas demos cargadas" en `/app`, correr `NeoDemo` **colgó la placa
  SIN mensaje** → hubo que resetear. NO es el tope limpio del FS (`fs_put` ya devuelve
  `FS_ERR_NO_SPACE`/`TABLE_FULL`); un cuelgue mudo = overflow/loop/corrupción no guardada.
  Tras el reset `/app` quedó vacío (uploads RAM) → ahora NeoDemo va (FS 35/128 KB). Territorio
  **delicado (carga de módulos / memoria bajo presión)** = por [[v4-consolidacion-v3-bugs-obvios]]
  se trata con reproducción+tests, NO shotgun. **Plan:** reproducir en frío (con `/app` vacío,
  subir mods 1 a 1 sin resetear + correr NeoDemo entre medias → hallar el nº y el punto exactos
  de cuelgue: ¿upload/`compact()`? ¿link con N módulos?), LOCALIZAR, y solo entonces tocar.
  Candidato a **sesión dedicada** (no bloquea el resto del batch). ¿Bloquea publicar? decisión
  de Eduardo (en uso normal `/app` se limpia en el reset y el IDE lo gestiona → poco frecuente,
  pero un cuelgue-con-reset no es ideal).

- **Metro · `Pico.ADC_CHANNELS()` reportaba 4, no 8** (bug de V3, ✅ FIXED + VERIFICADO en placa)
  — el intrínseco (builtin 208 → `bpvm_pico_adc_channels`) caía al **fallback fijo 4** de
  `src/pico.c` porque el backend del Pico (`pico/main.c`) registraba `.gpioCount` (board-aware,
  48 ✅) pero se dejó `.adcChannels`/`.pwmSlices` — hueco de la tanda ADC/PWM board-aware
  (953d7d9, 26/06). El firmware INFO (lee `board_desc` directo) ya daba 8 → de ahí la
  discrepancia INFO=8 vs PicoInfo=4. **Severidad baja:** la Pico 2 (variante A=4) salía bien;
  los canales ADC 4-7 de la Metro **funcionan** a nivel firmware (bounds-check contra
  `board_desc`→8), solo la API BP los ocultaba. **Fix (5º commit del batch):**
  `.adcChannels`/`.pwmSlices = board_desc()->…` (calcado de `gpioCount`, commit 88bd7d4).
  ✅ **Verificado 4-jul:** Metro recompilada+reflasheada → PicoInfo da `ADC channels: 8` (era 4).

- **S3/Nucleo · MISMO bug ADC/PWM en los backends esp32 + stm32** — el fix de la Metro solo
  tocó el backend pico; los backends **esp32** (`gpio_esp32.c`) y **stm32** (`gpio_stm32.c`)
  tenían el MISMO hueco (`.gpioCount` sí, `.adcChannels`/`.pwmSlices` no) → fallback 4/12.
  **CONFIRMADO en la S3:** PicoInfo dio `ADC channels: 4` / `PWM slices: 12` (debían 20/8).
  Fix **f15b7a5** (esp32 → 20/8; stm32 → 20/28; casan con lo que ya reporta el INFO del wire).
  ✅ **esp32 VERIFICADO en la S3** (reflasheada → PicoInfo da `ADC 20 / PWM 8`, casan con el INFO).
  Pendiente: stm32 (Nucleo). **Menor S3 → V4:** `Pico.tempC()` devuelve 0 (el backend esp32 no
  cablea el sensor de temperatura interno). **Nota FS (relevante al cuelgue):** en la S3 el `/app`
  **PERSISTE** al reflash (partición esp; no RAM-only como el pico — el skip-PUT confirmó
  `/app/PicoInfo.mod` idéntico tras reflashear) → allí `/app` se acumula, el cuelgue podría
  alcanzarse antes.

- **Puerta 0 · TryCatch** — el harness daba 1 rojo (V3-Java salía vacío, "paridad
  rota"). **Causa: hueco del arnés, NO regresión del producto.** `try/catch` en V3
  depende de `Core` (#248) y el golden `TryCatch.mod` lo referencia; la VM-C resuelve
  `Core.mod` junto al `.mod` raíz, pero el CLI de la VM-Java lo resuelve relativo al
  CWD, y `run_vm` lanzaba el JAR desde `compat/` (no desde el dir del `.mod` en WORK)
  → `FileNotFoundException: Core.mod` tragado por `2>/dev/null` → stdout vacío.
  Ejecutados directamente sobre el mismo `.mod` con Core presente, los tres (golden,
  V3-C, V3-Java) dan salida byte-idéntica. **Fix (utillaje):** `run_vm` ejecuta ambas
  VMs desde el dir del `.mod` (subshell). Puerta 0 → 16/16 verde. _Matiz de fondo
  (no bloquea, → V4): las dos CLI host resuelven la ruta de las deps desde bases
  distintas (VM-C = dir del `.mod`; VM-Java = CWD). No afecta al producto — el daemon
  del IDE y el firmware montan las deps en su sitio y la salida es idéntica._
