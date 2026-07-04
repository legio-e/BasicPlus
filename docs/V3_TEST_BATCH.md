# V3 — Batch de pruebas finales (checklist)

> Verificación pre-publicación de V3. Se organiza en **2 grupos** (Eduardo 4-jul):
> Grupo 1 = **regresión ligera** de las 4 placas NO gráficas; Grupo 2 =
> verificación **completa** (no gráfico + GUI) de las 3 placas CON pantalla.
> Marca `[x]` según avances; los fallos van al **Registro de hallazgos** del final.
> Bugs que salgan → se arreglan (código descongelado solo para eso). Publicar es
> DESPUÉS de este batch. Escalera de peldaños y matriz históricas: `H14_TEST_PLAN.md`.

---

## Puerta 0 — Paridad en el PC (gratis, ANTES de tocar placa)

- [ ] `cd compat && ./compat.sh check` → **TODO VERDE**. Verifica que las VMs V3 y el
      frontend siguen byte-idénticos a los goldens de V2 (comportamiento + ids de
      opcode + emisión). Un rojo aquí = regresión real, y se ve sin gastar un flash.

Si la Puerta 0 está verde, el riesgo de las placas baja mucho: lo que quede es
plataforma (backend HW, boot, wire), no el núcleo.

---

## GRUPO 1 — No gráfico · regresión ligera

**Objetivo:** confirmar que V3 no rompió nada (que "todo sigue funcionando sin
cambios importantes"). NO es re-test exhaustivo de sensores — es un pase smoke por
placa. 3 firmwares · 4 placas.

**Paso 0 (Eduardo, por placa):** recompilar el firmware con las fuentes V3 y flashear.

**Peldaños por placa** (de H14; marcar los que apliquen a lo que tengas cableado):

| Placa | Firmware | Boot+INFO | Ejec/OO (paridad) | GPIO (blink) | I2C | SPI | UART | reset-cause | autorun+Stop |
|---|---|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| **Pico 2** (RP2350A) | `pico` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| **Metro RP2350B** | `pico` (misma img) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| **ESP32-S3 DevKit** | `esp32` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| **STM32 Nucleo-U575** | `stm32` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |

**Notas por placa (lo que V3 cambió y conviene mirar de reojo):**
- **Pico 2**: INFO variante A (30 GPIO). `native` acelerada (AOT ARM) → probar `^`/`eval`
  en un sample compute y comparar con host.
- **Metro RP2350B**: subir `/sys/board.json` (variante B) → INFO debe mostrar 48 GPIO +
  **PSRAM 8 MB usada como heap** + NeoPixel. Es la placa donde el heap PSRAM importa.
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
- [ ] **`^` (potencia) y `eval`** en placa: correr en las ARM (Pico/Metro/Nucleo — AOT) y
      en ESP32 (interpretado) — misma salida que host.
- [ ] **CRC skip-PUT**: subir un `.mod` ya presente e idéntico → el IDE dice "contenido
      idéntico, salto PUT" (no re-sube).
- [ ] **missing-lib al wire**: correr algo cuya dep NO esté → el IDE resuelve/sube la lib,
      o da error claro con el nombre.
- [ ] **stdlib fresca en placa** (P4): tras primer boot, `Pico.mod` etc. byte-idénticos a
      bpstdlib (skip-PUT lo confirma).

---

## Registro de hallazgos

_(Vacío. Cada fallo: placa · peldaño · síntoma · ¿bug de V3 o de cableado/entorno? ·
commit del fix si lo hay.)_

- …
