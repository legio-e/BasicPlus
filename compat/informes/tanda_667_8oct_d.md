# Tandas T1 — las placas conducidas

## Tanda 2026-10-08 17:11:40

- manifiesto: `bpgenvm-c\tools\tanda_667.json` (13 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-08 14:11:16) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-08 16:29:49) - sabor `v2-gui1-lvgl0`
- placas conectadas: 2 (stm32u5g9@COM12, esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| DestruyeArbol | DestruyeArbol | 4714aaf7 | 6754 | ok | 0 | 17 | 10 |
| DestruyeLanza | DestruyeLanza | 4aeaac9d | 2930 | ok | 0 | 9 | 4 |
| DestruyeGc | DestruyeGc | 19fe2788 | 2349 | ok | 0 | 9 | 5 |
| DestruyeFinal | DestruyeFinal | 960fb864 | 1430 | ok | 0 | 9 | 4 |
| DestruyeTrasError | DestruyeTrasError | 91c1307f | 1331 | ok | 1 | 9 | 3 |
| KillDestruye | KillDestruye | 56ff2a4e | 1306 | ok | - | - | - |
| SueltaPwmA | SueltaPwmA | ba4876b8 | 2217 | ok | - | - | - |
| SueltaPwmB | SueltaPwmB | 158c02b7 | 2410 | ok | 0 | 14 | 14 |
| SueltaNpA | SueltaNpA | db52d7e0 | 1659 | ok | - | - | - |
| SueltaNpB | SueltaNpB | 30df1f29 | 1804 | ok | 0 | 12 | 1 |
| SueltaRedA | SueltaRedA | fcf6e1af | 2558 | ok | 0 | 15 | 9 |
| SueltaPwmB2 | SueltaPwmB | 158c02b7 | 2410 | ok | 0 | 12 | 14 |
| SueltaNpB2 | SueltaNpB | 30df1f29 | 1804 | ok | 0 | 10 | 1 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  8 2026 16:20:34` imageCrc=`b2891a18` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (08-10 16:20)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=493240 fs=73728/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 0 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 117 | 17 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 38 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 44 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 22 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 861 | - | KILL a los 800 ms; el EXITED, 31 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1432 | - | KILL a los 1500 ms; el EXITED, 18 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 33 | 14 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1457 | - | KILL a los 1500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 18 | 12 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 36 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 32 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 18 | 10 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=494889 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeLanza: uptimeMs=495710 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeGc: uptimeMs=496459 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeFinal: uptimeMs=497152 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeTrasError: uptimeMs=497827 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras KillDestruye: uptimeMs=499074 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmA: uptimeMs=501247 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmB: uptimeMs=502029 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpA: uptimeMs=504040 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpB: uptimeMs=504764 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaRedA: uptimeMs=505564 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmB2: uptimeMs=506336 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpB2: uptimeMs=507054 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
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
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 2217 | ba4876b8 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 2410 | 158c02b7 | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1659 | db52d7e0 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 1804 | 30df1f29 | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 2558 | fcf6e1af | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 2410 | 158c02b7 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 1804 | 30df1f29 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Pwm: en la zona de packs (pack:Stdlib/Pwm.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Uart: en la zona de packs (pack:Stdlib/Uart.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Neopixel: en la zona de packs (pack:Stdlib/Neopixel.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  8 2026 14:58:55` imageCrc=`4d69b438` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (08-10 16:57)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=panic/exception uptimeMs=646598 fs=151552/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 11 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 103 | 17 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 36 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 39 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 28 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 20 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 648 | - | KILL a los 800 ms; el EXITED, 35 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1292 | - | KILL a los 1500 ms; el EXITED, 22 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 27 | 14 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1275 | - | KILL a los 1500 ms; el EXITED, 22 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 18 | 12 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 20 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 30 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 18 | 10 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=649913 fsUsed=159744 vmHeapBytes=24637440 pila libre: run=11616 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras DestruyeLanza: uptimeMs=651003 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11236 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras DestruyeGc: uptimeMs=651968 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11236 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras DestruyeFinal: uptimeMs=652838 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras DestruyeTrasError: uptimeMs=653619 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras KillDestruye: uptimeMs=654941 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras SueltaPwmA: uptimeMs=657361 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras SueltaPwmB: uptimeMs=658481 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11236 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras SueltaNpA: uptimeMs=660571 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11236 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras SueltaNpB: uptimeMs=661509 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras SueltaRedA: uptimeMs=662563 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras SueltaPwmB2: uptimeMs=663880 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11236 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
- tras SueltaNpB2: uptimeMs=664897 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24524 tareaREPL=24524 heap del sistema, minimo libre=406507
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
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 2217 | ba4876b8 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 2410 | 158c02b7 | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1659 | db52d7e0 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 1804 | 30df1f29 | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 2558 | fcf6e1af | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 2410 | 158c02b7 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 1804 | 30df1f29 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: DESFASE de compilación —la placa tiene en /lib/Collections.mod la versión 7 (>= la mínima 7), crc 263868cb != el de hoy 738b2934: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Str: DESFASE de compilación —la placa tiene en /lib/Str.mod la versión 7 (>= la mínima 7), crc 9765bb28 != el de hoy 4320a024: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Pwm: en la zona de packs (pack:Stdlib/Pwm.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Uart: en la zona de packs (pack:Stdlib/Uart.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Neopixel: en la zona de packs (pack:Stdlib/Neopixel.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados

### Matriz pruebas x placas

| prueba | stm32u5g9@COM12 | esp32p4@COM15 |
|---|---|---|
| DestruyeArbol | IDENTICO | IDENTICO |
| DestruyeLanza | IDENTICO | IDENTICO |
| DestruyeGc | IDENTICO | IDENTICO |
| DestruyeFinal | IDENTICO | IDENTICO |
| DestruyeTrasError | IDENTICO | IDENTICO |
| KillDestruye | PARADA | PARADA |
| SueltaPwmA | PARADA | PARADA |
| SueltaPwmB | ASERCIONES | ASERCIONES |
| SueltaNpA | PARADA | PARADA |
| SueltaNpB | ASERCIONES | ASERCIONES |
| SueltaRedA | NO-PETA | NO-PETA |
| SueltaPwmB2 | ASERCIONES | ASERCIONES |
| SueltaNpB2 | ASERCIONES | ASERCIONES |


