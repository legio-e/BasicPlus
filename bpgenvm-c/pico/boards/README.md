# Ficheros de placa (`/sys/board.json`) — H7.3

El firmware RP2350 es **genérico**: el mismo binario vale para Pico 2 (RP2350A,
30 GPIO) y Metro (RP2350B, 48 GPIO). Lo que cambia entre placas vive en **datos**,
no en el código: un fichero `board.json` que se sube al FS del dispositivo como
`/sys/board.json`. El firmware lo lee en boot (`board_desc_init`, ver
`../board_desc.c`).

Estos ficheros son **plantillas**: súbelas a `/sys/board.json` en la placa
correspondiente (vía PicoExplorer/PUT). Sin `/sys/board.json` el firmware usa los
defaults por variante (hoy: variante `B` permisiva — ver nota abajo).

## Dos capas

| Capa  | Quién la fija            | Campos                                            |
|-------|--------------------------|---------------------------------------------------|
| CHIP  | tabla built-in (variante)| `gpioCount`, nº PIO/PWM/ADC, default `psramCsPin` |
| PLACA | este `board.json`        | `name`, `ledPin`, `neopixelPin`, `psramCsPin`     |

El core del firmware NO conoce "Metro"/"Pico": sólo variantes RP2350A/B (tabla de
caps) + lo que diga este fichero. Una placa nueva = otro `board.json`, sin tocar
el firmware.

## Esquema

| Campo         | Tipo   | Significado                                                        |
|---------------|--------|-------------------------------------------------------------------|
| `name`        | string | Nombre legible (lo devuelve `Pico.board` en BP).                  |
| `variant`     | "A"/"B"| Variante del chip → selecciona la tabla de caps (30 vs 48 GPIO).  |
| `ledPin`      | int    | GPIO del LED onboard (−1 si no hay / es NeoPixel).                |
| `neopixelPin` | int    | GPIO del NeoPixel WS2812 (−1 si no hay). Peculiar de cada placa.  |
| `psramCsPin`  | int    | Chip-Select de la PSRAM en el bus QSPI (−1 / omitir → default).   |
| `gpioCount`   | int    | (opcional) override explícito de la tabla, para placas atípicas.  |

Campos omitidos → default por variante (`apply_variant_caps` en `board_desc.c`).

## Pendiente de confirmar (esquemático del Metro)

`metro-rp2350b.json` va **minimal** a propósito (sólo `name` + `variant`). Los
pines específicos del Metro hay que rellenarlos desde el esquemático de Adafruit
en sus tareas:

- `neopixelPin` — NeoPixel onboard del Metro → **H7.4** (driver WS2812).
- `psramCsPin`  — CS de la PSRAM en el QSPI. Default por variante: GPIO0 (A) /
  GPIO47 (B), a confirmar → **H7.2.a** (init PSRAM).
- `ledPin`      — el Metro no lleva LED GPIO simple (usa el NeoPixel); confirmar.
