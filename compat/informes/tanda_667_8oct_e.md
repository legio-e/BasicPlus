# Tandas T1 — las placas conducidas

## Tanda 2026-10-08 18:29:40

- manifiesto: `bpgenvm-c\tools\tanda_667.json` (13 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-08 14:11:16) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-08 17:57:08) - sabor `v2-gui1-lvgl0`
- placas conectadas: 3 (esp32c6@COM3, esp32c5@COM6, stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| DestruyeArbol | DestruyeArbol | 4714aaf7 | 6754 | ok | 0 | 20 | 10 |
| DestruyeLanza | DestruyeLanza | 4aeaac9d | 2930 | ok | 0 | 9 | 4 |
| DestruyeGc | DestruyeGc | 19fe2788 | 2349 | ok | 0 | 11 | 5 |
| DestruyeFinal | DestruyeFinal | 960fb864 | 1430 | ok | 0 | 10 | 4 |
| DestruyeTrasError | DestruyeTrasError | 91c1307f | 1331 | ok | 1 | 10 | 3 |
| KillDestruye | KillDestruye | 56ff2a4e | 1306 | ok | - | - | - |
| SueltaPwmA | SueltaPwmA | 054b49b9 | 2499 | ok | - | - | - |
| SueltaPwmB | SueltaPwmB | f9cdeb41 | 2708 | ok | 0 | 14 | 14 |
| SueltaNpA | SueltaNpA | 6b9560d5 | 1812 | ok | - | - | - |
| SueltaNpB | SueltaNpB | c3db9d9f | 1958 | ok | 0 | 18 | 1 |
| SueltaRedA | SueltaRedA | 6e91e01c | 2967 | ok | 0 | 16 | 9 |
| SueltaPwmB2 | SueltaPwmB | f9cdeb41 | 2708 | ok | 0 | 13 | 14 |
| SueltaNpB2 | SueltaNpB | c3db9d9f | 1958 | ok | 0 | 12 | 1 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  8 2026 15:00:22` imageCrc=`7c18cbb7` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c6/build/bpvm_esp32c6.bin (08-10 18:22)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=415744 fs=32768/1261568 B vmHeapBytes=64512
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 0 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 36 | 20 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 11 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 11 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 8 | 10 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 8 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 642 | - | KILL a los 800 ms; el EXITED, 32 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1277 | - | KILL a los 1500 ms; el EXITED, 36 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 22 | 14 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1254 | - | KILL a los 1500 ms; el EXITED, 12 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 15 | 18 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 17 | 16 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 22 | 13 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 15 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=417111 fsUsed=40960 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras DestruyeLanza: uptimeMs=417605 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras DestruyeGc: uptimeMs=418103 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras DestruyeFinal: uptimeMs=418611 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras DestruyeTrasError: uptimeMs=419122 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras KillDestruye: uptimeMs=420144 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras SueltaPwmA: uptimeMs=422064 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras SueltaPwmB: uptimeMs=422594 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras SueltaNpA: uptimeMs=424334 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras SueltaNpB: uptimeMs=424824 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras SueltaRedA: uptimeMs=425345 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras SueltaPwmB2: uptimeMs=425856 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- tras SueltaNpB2: uptimeMs=426338 fsUsed=36864 vmHeapBytes=64512 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=34592
- ejecutadas (con EXITED) 13 de 13 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/DestruyeArbol.mod | bpgenvm-c\build\tanda\DestruyeArbol\DestruyeArbol.mod | 6754 | 4714aaf7 | ok (por trozos) |
| /app/DestruyeLanza.mod | bpgenvm-c\build\tanda\DestruyeLanza\DestruyeLanza.mod | 2930 | 4aeaac9d | ok |
| /app/DestruyeGc.mod | bpgenvm-c\build\tanda\DestruyeGc\DestruyeGc.mod | 2349 | 19fe2788 | ok |
| /app/DestruyeFinal.mod | bpgenvm-c\build\tanda\DestruyeFinal\DestruyeFinal.mod | 1430 | 960fb864 | ok |
| /app/DestruyeTrasError.mod | bpgenvm-c\build\tanda\DestruyeTrasError\DestruyeTrasError.mod | 1331 | 91c1307f | ok |
| /app/KillDestruye.mod | bpgenvm-c\build\tanda\KillDestruye\KillDestruye.mod | 1306 | 56ff2a4e | ok |
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 2499 | 054b49b9 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 2708 | f9cdeb41 | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1812 | 6b9560d5 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 1958 | c3db9d9f | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 2967 | 6e91e01c | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 2708 | f9cdeb41 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 1958 | c3db9d9f | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Pwm: en la zona de packs (pack:Stdlib/Pwm.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Uart: en la zona de packs (pack:Stdlib/Uart.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Neopixel: en la zona de packs (pack:Stdlib/Neopixel.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Oct  8 2026 14:59:52` imageCrc=`e2995df7` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c5/build/bpvm_esp32c5.bin (08-10 18:24)
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=software uptimeMs=318293 fs=139264/16777216 B vmHeapBytes=6288384
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 39 | 20 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 18 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 17 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 15 | 10 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 14 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 826 | - | KILL a los 800 ms; el EXITED, 24 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1525 | - | KILL a los 1500 ms; el EXITED, 57 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 41 | 14 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1458 | - | KILL a los 1500 ms; el EXITED, 45 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 23 | 18 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 22 | 16 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 41 | 13 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 22 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=320240 fsUsed=147456 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras DestruyeLanza: uptimeMs=320761 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras DestruyeGc: uptimeMs=321226 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras DestruyeFinal: uptimeMs=321693 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras DestruyeTrasError: uptimeMs=322161 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras KillDestruye: uptimeMs=323263 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras SueltaPwmA: uptimeMs=325223 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras SueltaPwmB: uptimeMs=325694 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras SueltaNpA: uptimeMs=327483 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras SueltaNpB: uptimeMs=327967 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras SueltaRedA: uptimeMs=328451 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras SueltaPwmB2: uptimeMs=328936 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- tras SueltaNpB2: uptimeMs=329435 fsUsed=143360 vmHeapBytes=6288384 pila libre: run=11320 io=8816 tareaREPL=8816 heap del sistema, minimo libre=189155
- ejecutadas (con EXITED) 13 de 13 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/DestruyeArbol.mod | bpgenvm-c\build\tanda\DestruyeArbol\DestruyeArbol.mod | 6754 | 4714aaf7 | ok (por trozos) |
| /app/DestruyeLanza.mod | bpgenvm-c\build\tanda\DestruyeLanza\DestruyeLanza.mod | 2930 | 4aeaac9d | ok |
| /app/DestruyeGc.mod | bpgenvm-c\build\tanda\DestruyeGc\DestruyeGc.mod | 2349 | 19fe2788 | ok |
| /app/DestruyeFinal.mod | bpgenvm-c\build\tanda\DestruyeFinal\DestruyeFinal.mod | 1430 | 960fb864 | ok |
| /app/DestruyeTrasError.mod | bpgenvm-c\build\tanda\DestruyeTrasError\DestruyeTrasError.mod | 1331 | 91c1307f | ok |
| /app/KillDestruye.mod | bpgenvm-c\build\tanda\KillDestruye\KillDestruye.mod | 1306 | 56ff2a4e | ok |
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 2499 | 054b49b9 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 2708 | f9cdeb41 | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1812 | 6b9560d5 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 1958 | c3db9d9f | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 2967 | 6e91e01c | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 2708 | f9cdeb41 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 1958 | c3db9d9f | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: DESFASE de compilación —la placa tiene en /lib/Collections.mod la versión 7 (>= la mínima 7), crc 263868cb != el de hoy 738b2934: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Str: DESFASE de compilación —la placa tiene en /lib/Str.mod la versión 7 (>= la mínima 7), crc 9765bb28 != el de hoy 4320a024: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Pwm: en la zona de packs (pack:Stdlib/Pwm.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Uart: en la zona de packs (pack:Stdlib/Uart.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Neopixel: en la zona de packs (pack:Stdlib/Neopixel.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Oct  8 2026 17:57:20` imageCrc=`7f42e63c` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.elf (08-10 17:57)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=286264 fs=311296/516096 B vmHeapBytes=196608
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 18 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 114 | 20 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 37 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 43 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 29 | 10 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 23 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 846 | - | KILL a los 800 ms; el EXITED, 30 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1056 | - | KILL a los 1500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 47 | 14 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1259 | - | KILL a los 1500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 17 | 18 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 35 | 16 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 46 | 13 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 17 | 12 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=288658 fsUsed=319488 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeLanza: uptimeMs=289476 fsUsed=319488 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeGc: uptimeMs=290259 fsUsed=319488 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeFinal: uptimeMs=290952 fsUsed=319488 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeTrasError: uptimeMs=291625 fsUsed=319488 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras KillDestruye: uptimeMs=292887 fsUsed=319488 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmA: uptimeMs=295049 fsUsed=319488 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmB: uptimeMs=296283 fsUsed=319488 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpA: uptimeMs=298555 fsUsed=327680 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpB: uptimeMs=299294 fsUsed=327680 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaRedA: uptimeMs=300944 fsUsed=327680 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmB2: uptimeMs=302176 fsUsed=327680 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpB2: uptimeMs=303339 fsUsed=327680 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 13 de 13 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/DestruyeArbol.mod | bpgenvm-c\build\tanda\DestruyeArbol\DestruyeArbol.mod | 6754 | 4714aaf7 | ok (por trozos) |
| /app/DestruyeLanza.mod | bpgenvm-c\build\tanda\DestruyeLanza\DestruyeLanza.mod | 2930 | 4aeaac9d | ok |
| /app/DestruyeGc.mod | bpgenvm-c\build\tanda\DestruyeGc\DestruyeGc.mod | 2349 | 19fe2788 | ok |
| /app/DestruyeFinal.mod | bpgenvm-c\build\tanda\DestruyeFinal\DestruyeFinal.mod | 1430 | 960fb864 | ok |
| /app/DestruyeTrasError.mod | bpgenvm-c\build\tanda\DestruyeTrasError\DestruyeTrasError.mod | 1331 | 91c1307f | ok |
| /app/KillDestruye.mod | bpgenvm-c\build\tanda\KillDestruye\KillDestruye.mod | 1306 | 56ff2a4e | ok |
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 2499 | 054b49b9 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 2708 | f9cdeb41 | ok |
| /lib/Neopixel.mod | bpstdlib\Neopixel.mod | 2768 | a4d3c2d8 | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1812 | 6b9560d5 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 1958 | c3db9d9f | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 2967 | 6e91e01c | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 2708 | f9cdeb41 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 1958 | c3db9d9f | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: DESFASE de compilación —la placa tiene en pack:SQLite/Str.mod la versión 7 (>= la mínima 7), crc 8e56f753 != el de hoy 4320a024: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Pwm: ya en /lib/Pwm.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Uart: ya en /lib/Uart.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Neopixel: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib
- limpieza: 0 fichero(s) de /app borrados; quedan en /lib, subidos por la tanda: /lib/Neopixel.mod

### Matriz pruebas x placas

| prueba | esp32c6@COM3 | esp32c5@COM6 | stm32u575@COM5 |
|---|---|---|---|
| DestruyeArbol | IDENTICO | IDENTICO | IDENTICO |
| DestruyeLanza | IDENTICO | IDENTICO | IDENTICO |
| DestruyeGc | IDENTICO | IDENTICO | IDENTICO |
| DestruyeFinal | IDENTICO | IDENTICO | IDENTICO |
| DestruyeTrasError | IDENTICO | IDENTICO | IDENTICO |
| KillDestruye | PARADA | PARADA | PARADA |
| SueltaPwmA | PARADA | PARADA | PARADA |
| SueltaPwmB | ASERCIONES | ASERCIONES | ASERCIONES |
| SueltaNpA | PARADA | PARADA | PARADA |
| SueltaNpB | ASERCIONES | ASERCIONES | ASERCIONES |
| SueltaRedA | NO-PETA | NO-PETA | NO-PETA |
| SueltaPwmB2 | ASERCIONES | ASERCIONES | ASERCIONES |
| SueltaNpB2 | ASERCIONES | ASERCIONES | ASERCIONES |


