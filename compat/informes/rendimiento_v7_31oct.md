# El rendimiento de hoy frente a V7, en la Metro y en la P4 (31-oct)

Eduardo: *«Me gustaría poder comparar el rendimiento actual con lo que teníamos, ya sea de la versión 7 o incluso
anterior. Por lo menos poder comparar en la Metro y en la P4»*; *«de momento comparamos con la V7»*; y *«Tenemos datos
históricos de comprobaciones que hicimos, no podrías recuperarlos para incluirlos en la comparación?»* (FICHAS `#743`).

## Cómo

- **La herramienta:** `bpgenvm-c/tools/rendimiento_versiones.py`. Cada versión se mide entera, como se publicó: su
  compilador, su stdlib y su firmware salen de su ZIP de `dist/`; «hoy» es el árbol (HEAD, `3a581135` y lo de hoy).
- **Los programas:** los de `samples/benchmarks` que no han cambiado de texto desde V7 (los siete clásicos, igual desde
  V4; `Bench`, `AllocBench`, `PrintBench` y `VivoLargo`, desde V6; `BenchMem`, desde V7). Cada uno, dos veces; las
  dos pasadas difieren un 3 % como mucho (salvo `PrintBench` en la Metro, un 20 %).
- **Las placas:** la Metro RP2350B (`4CE219E14C8F8E1C`) y la P4 Waveshare 4.3" (`E8F60AE0B380`, 32 MB de flash).
  V7 con su imagen publicada (la Metro, 12-oct 16:06; la P4, 12-oct 16:09) y su propia stdlib (el `Stdlib.pack` de
  hoy, quitado mientras); hoy, con la imagen de HEAD (la Metro, 30-oct 20:34; la P4, 30-oct 20:37).
- **Las funciones `native`** corren interpretadas en las dos versiones (no se sube ningún `.mdn`): la columna «native»
  de cada programa es otra pasada del intérprete.
- Los detalles, línea a línea: `rendimiento_rp2350_v7_hoy.md` y `rendimiento_p4_v7_hoy.md`; las salidas enteras, en
  `rendimiento/`.

## V7 → hoy (el RUN entero; negativo = hoy más rápido)

| programa | qué carga | Metro | P4 |
|---|---|---|---|
| `arraybench` | arrays | 1248 → 1248 ms (0 %) | 1008 → 1031 ms (+2 %) |
| `fibobench` | llamadas, enteros | 46.527 → 47.628 (+2 %) | 34.481 → 35.769 (+4 %) |
| `Bench` | `fib(28)` (su línea «interp») | 8735 → 9092 ms (+4 %) | 6621 → 6859 ms (+4 %) |
| `floatbench` | reales | 4897 → 5219 (+7 %) | 3919 → 4086 (+4 %) |
| `intbench` | enteros | 3063 → 2761 (−10 %) | 1952 → 2024 (+4 %) |
| `sortbench` | arrays | 5544 → 5500 (−1 %) | 4399 → 4506 (+2 %) |
| `sumbench` | arrays | 7376 → 7490 (+2 %) | 5752 → 5882 (+2 %) |
| `BenchMem` | memoria | 11.917 → 12.319 (+3 %) | 9340 → 9592 (+3 %) |
| `VivoLargo` | cada vuelta (30 s fijos) | +7 % | +3 % |
| `stringbench` | cadenas | 423.190 → 366.351 (−13 %) | 288.988 → 274.037 (−5 %) |
| `AllocBench` | reservas y GC | 3082 → 1912 (−38 %) | 1152 → 1146 (−1 %) |
| `PrintBench` | salida por el wire | 1456 → 735 (−50 %) | 20.934 → 20.934 (0 %: la UART) |

- **Las cadenas, las reservas y la salida, mejor** (el H5 de las cadenas, `#718` y `#736`, y en la Metro la imagen M,
  con lo caliente en la SRAM).
- **El cálculo puro, de un 2 a un 7 % más lento** en las dos. Lo que más pesa ahí, probablemente, la comprobación de la
  pila antes de cada instrucción (`#740`, la b), que cuesta un 12–14 % del bytecode en la Pico 2 y que se quedó a
  propósito: *«es más importante la seguridad que la velocidad»*. Sin medir aparte en estas dos placas.
- `PrintBench` en la P4 lo marca su UART a 115.200 (21 s en las dos versiones); en la Metro, el USB.

## Lo que había medido antes: `fib(28)` interpretado

`Bench` es el mismo `fib(28)` desde V4 (sólo cambia de dónde sale la hora: `Pico.uptimeMs` en V4 y V5,
`Machine.uptimeMs` después). Las cifras viejas salen de las campañas de placas (`docs/H13_PRUEBAS.md`, V4;
`docs/H13_PRUEBAS_V5.md`, V5) y de `docs/BENCHMARKS.md` (V1): una pasada cada una.

| versión | Metro (heap en la PSRAM) | P4 | Pico 2 |
|---|---|---|---|
| V1 (26-may) | — | — | ~5500 ms |
| V4 (3/4-ago) | 7755 ms | 4543 ms (el KIT) | — |
| V5 (21-ago) | 8819 ms | 4363 ms (¿qué P4?) | 8925 ms |
| V7 (medida hoy) | 8735 ms | 6621 ms (la Waveshare) | — |
| hoy | 9092 ms | 6859 ms (la Waveshare) | — |

- **La Metro es la misma placa, con el heap en la PSRAM en todas** (V4 ya lo tenía ahí): de V4 a hoy, +17 %; casi todo
  entre V4 y V5 (+14 %); de V5 a V7, igual; de V7 a hoy, +4 %.
- **La P4, no:** la cifra de V4 es del kit (b2, 16 MB de flash) y hoy está conectada la Waveshare (b3, 32 MB). Mismo
  chip, mismo reloj (360 MHz), misma imagen, pero el código corre desde la flash por la caché, y cada placa lleva su
  flash. El salto de 4,4 a 6,6 s puede ser de la placa y no de la versión: para saberlo, medir V7 en el kit, o V5 en
  la Waveshare.
- Las tandas funcionales de V7 (12-oct) y V8 (27-oct) traen el tiempo de cada prueba en placa, pero coinciden 3 o 4
  pruebas cortas por placa: dicen lo mismo (−5 a −1 %) y poco más.

## Las placas, al acabar

- **La Metro:** la imagen de HEAD, el `Stdlib.pack` de hoy (`packs/`) regrabado y `/lib` vacío; `SQLite`, sin tocar.
- **La P4:** la imagen de HEAD (con `#718` y la L2 de 256 KB, `#737`), y sus packs regrabados y sellados para la zona de
  esta imagen: `Stdlib`, `Gui` (motor LVGL, rc=0) y `SQLite` (rc=0). Era lo que pedía `#741`.
- De paso: `wire_serie.py pack` con un motor nativo no grababa si se lanzaba con `python -X utf8` (Java escribe en
  cp1252 y la raya «—» rompía la lectura): arreglado. Y `idf.py` lanzado desde Git Bash, a través de PowerShell, no
  graba nada si ve las variables de MSYS: la herramienta las quita, y comprueba con el HELLO que la imagen cambió.
