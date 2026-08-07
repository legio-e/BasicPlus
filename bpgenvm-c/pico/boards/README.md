# Ficheros de placa (`/sys/board.json`) — H7.3

El firmware RP2350 es **genérico**: el mismo binario vale para Pico 2 (RP2350A,
30 GPIO) y Metro (RP2350B, 48 GPIO). Lo que cambia entre placas vive en **datos**,
no en el código: un fichero `board.json` que se sube al FS del dispositivo como
`/sys/board.json`. El firmware lo lee en boot (`board_desc_init`, ver
`../board_desc.c`).

Estos ficheros son **plantillas**: súbelas a `/sys/board.json` en la placa
correspondiente (vía PicoExplorer/PUT). Sin `/sys/board.json` el firmware
defaultea a variante **`A` (30 GPIO)** — el default seguro mientras el build
sea para el target `pico2` (el SDK sólo conoce 30 GPIO). Una placa B declara su
variante en su `board.json`. H7.2 hará el default dinámico (sondeo de PSRAM).

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
| `sdBus`       | string | Bus del lector microSD (`spi0`) o ausente = la placa no lleva.     |
| `sdSckPin`    | int    | SD: reloj. En 4 bits es `SDIO_CLK`.                               |
| `sdMosiPin`   | int    | SD: MOSI. En 4 bits es `SDIO_CMD`.                                |
| `sdMisoPin`   | int    | SD: MISO. En 4 bits es **`SDIO_DAT0`** (ver aviso abajo).         |
| `sdCsPin`     | int    | SD: chip-select. En 4 bits es **`SDIO_DAT3`** (ver aviso abajo).  |
| `sdDat1Pin`   | int    | SD: `SDIO_DAT1`. Sin uso en modo SPI.                             |
| `sdDat2Pin`   | int    | SD: `SDIO_DAT2`. Sin uso en modo SPI.                             |
| `sdDetectPin` | int    | SD: detección de tarjeta. Permite decir "no hay tarjeta" YA.      |

Campos omitidos → default por variante (`apply_variant_caps` en `board_desc.c`).

## El lector microSD del Metro (V5) — confirmado 7-ago-2026

Del pinout de Adafruit (learn.adafruit.com/adafruit-metro-rp2350/pinouts):

| señal            | GPIO | función del mux RP2350 |
|------------------|------|------------------------|
| SD_SCK / CLK     | 34   | `SPI0_SCLK`            |
| SD_MOSI / CMD    | 35   | `SPI0_TX`              |
| SD_MISO / DAT0   | 36   | `SPI0_RX`              |
| SDIO_DAT1        | 37   | —                      |
| SDIO_DAT2        | 38   | —                      |
| SD_CS / DAT3     | 39   | GPIO por software      |
| SD_CARD_DETECT   | 40   | —                      |

**El cableado permite los DOS modos y no estropea ninguno:**

- **SPI0 por hardware**: las tres señales que exigen función de mux caen justo
  donde deben (34/35/36); el CS en 39 no la necesita porque lo mueve el software.
- **SDIO 4 bits**: `DAT0..DAT3 = 36,37,38,39` son **cuatro GPIO CONSECUTIVOS**,
  que es lo que una máquina PIO necesita para tratarlos como un grupo con una
  sola instrucción.

⚠️ **AVISO al escribir el driver de 4 bits.** Los campos se llaman por su papel en
SPI, que es el modo con el que se empieza, pero **en 4 bits significan otra cosa**:

    sdMisoPin (36) ES DAT0        sdCsPin (39) ES DAT3

Un driver de 4 bits que lea `sdDat1Pin`/`sdDat2Pin` y busque "dat0"/"dat3" no los
encontrará. **DAT0 = `sdMisoPin`, DAT3 = `sdCsPin`.**

⚠️ **Y el PIO no llega gratis a estos pines.** En RP2350B cada instancia de PIO
direcciona 32 pines y hay que mover su ventana con `pio_set_gpio_base(pio, 16)`
para alcanzar el 34-40 — con lo que ESE bloque deja de ver los GPIO 0-15. Hay 3
bloques; dedicarle uno. Si `PICO_PIO_USE_GPIO_BASE` no valiera 1, el 5º bit del
número de pin se **ignora en silencio** y GPIO34 se convertiría en GPIO2: no da
error, da el pin equivocado. Nuestro build ya va contra `bp_rp2350b`
(`PICO_RP2350A=0`), así que sale a 1 — pero es de las cosas que hay que mirar
ANTES de depurar un driver que "no responde".

**Estado:** los pines están DECLARADOS aquí; `board_desc.c` todavía NO los lee.
El lector llega con el bring-up de la SD.
