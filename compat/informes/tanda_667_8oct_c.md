# Tandas T1 — las placas conducidas

## Tanda 2026-10-08 17:02:00

- manifiesto: `bpgenvm-c\tools\tanda_667.json` (13 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-08 14:11:16) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-08 16:29:49) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| DestruyeArbol | DestruyeArbol | 4714aaf7 | 6754 | ok | 0 | 19 | 10 |
| DestruyeLanza | DestruyeLanza | 4aeaac9d | 2930 | ok | 0 | 12 | 4 |
| DestruyeGc | DestruyeGc | 19fe2788 | 2349 | ok | 0 | 9 | 5 |
| DestruyeFinal | DestruyeFinal | 960fb864 | 1430 | ok | 0 | 10 | 4 |
| DestruyeTrasError | DestruyeTrasError | 91c1307f | 1331 | ok | 1 | 12 | 3 |
| KillDestruye | KillDestruye | 56ff2a4e | 1306 | ok | - | - | - |
| SueltaPwmA | SueltaPwmA | ba4876b8 | 2217 | ok | - | - | - |
| SueltaPwmB | SueltaPwmB | 158c02b7 | 2410 | ok | 0 | 16 | 14 |
| SueltaNpA | SueltaNpA | db52d7e0 | 1659 | ok | - | - | - |
| SueltaNpB | SueltaNpB | 30df1f29 | 1804 | ok | 0 | 12 | 1 |
| SueltaRedA | SueltaRedA | fcf6e1af | 2558 | ok | 0 | 17 | 9 |
| SueltaPwmB2 | SueltaPwmB | 158c02b7 | 2410 | ok | 0 | 12 | 14 |
| SueltaNpB2 | SueltaNpB | 30df1f29 | 1804 | ok | 0 | 10 | 1 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  8 2026 14:58:55` imageCrc=`4d69b438` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (08-10 16:57)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=panic/exception uptimeMs=60372 fs=151552/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 11 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 103 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 35 | 12 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 38 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 28 | 10 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 19 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 662 | - | KILL a los 800 ms; el EXITED, 38 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1095 | - | KILL a los 1500 ms; el EXITED, 31 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 26 | 16 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1068 | - | KILL a los 1500 ms; el EXITED, 23 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 17 | 12 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 19 | 17 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 26 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 18 | 10 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=64443 fsUsed=159744 vmHeapBytes=24637440 pila libre: run=11616 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras DestruyeLanza: uptimeMs=65667 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras DestruyeGc: uptimeMs=66679 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras DestruyeFinal: uptimeMs=67563 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11952 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras DestruyeTrasError: uptimeMs=68480 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11952 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras KillDestruye: uptimeMs=69978 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras SueltaPwmA: uptimeMs=72278 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11952 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras SueltaPwmB: uptimeMs=73485 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras SueltaNpA: uptimeMs=75558 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11952 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras SueltaNpB: uptimeMs=76671 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras SueltaRedA: uptimeMs=78101 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras SueltaPwmB2: uptimeMs=79307 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11952 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
- tras SueltaNpB2: uptimeMs=80612 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11616 io=24656 tareaREPL=24656 heap del sistema, minimo libre=426183
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

| prueba | esp32p4@COM15 |
|---|---|
| DestruyeArbol | IDENTICO |
| DestruyeLanza | IDENTICO |
| DestruyeGc | IDENTICO |
| DestruyeFinal | IDENTICO |
| DestruyeTrasError | IDENTICO |
| KillDestruye | PARADA |
| SueltaPwmA | PARADA |
| SueltaPwmB | ASERCIONES |
| SueltaNpA | PARADA |
| SueltaNpB | ASERCIONES |
| SueltaRedA | NO-PETA |
| SueltaPwmB2 | ASERCIONES |
| SueltaNpB2 | ASERCIONES |


