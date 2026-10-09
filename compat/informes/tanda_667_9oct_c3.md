# Tandas T1 — las placas conducidas

## Tanda 2026-10-09 20:18:44

- manifiesto: `bpgenvm-c\tools\tanda_667.json` (13 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-09 11:20:44) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-09 19:05:28) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c3@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| DestruyeArbol | DestruyeArbol | 4714aaf7 | 6754 | ok | 0 | 19 | 10 |
| DestruyeLanza | DestruyeLanza | 4aeaac9d | 2930 | ok | 0 | 12 | 4 |
| DestruyeGc | DestruyeGc | 19fe2788 | 2349 | ok | 0 | 10 | 5 |
| DestruyeFinal | DestruyeFinal | 960fb864 | 1430 | ok | 0 | 9 | 4 |
| DestruyeTrasError | DestruyeTrasError | 91c1307f | 1331 | ok | 1 | 11 | 3 |
| KillDestruye | KillDestruye | 56ff2a4e | 1306 | ok | - | - | - |
| SueltaPwmA | SueltaPwmA | c65bec37 | 2650 | ok | - | - | - |
| SueltaPwmB | SueltaPwmB | 3dc9efcd | 2871 | ok | 0 | 16 | 14 |
| SueltaNpA | SueltaNpA | 4edb10e5 | 1888 | ok | - | - | - |
| SueltaNpB | SueltaNpB | 04bf67eb | 2037 | ok | 0 | 12 | 1 |
| SueltaRedA | SueltaRedA | 2f35566d | 3181 | ok | 0 | 17 | 9 |
| SueltaPwmB2 | SueltaPwmB | 3dc9efcd | 2871 | ok | 0 | 12 | 14 |
| SueltaNpB2 | SueltaNpB | 04bf67eb | 2037 | ok | 0 | 12 | 1 |

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Oct  9 2026 19:09:16` imageCrc=`236350f9` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c3/build/bpvm_esp32c3.bin (09-10 19:09)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=4127153 fs=81920/1523712 B vmHeapBytes=49152
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 3 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 49 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 16 | 12 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 14 | 10 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 12 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 11 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 862 | - | KILL a los 800 ms; el EXITED, 36 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1487 | - | KILL a los 1500 ms; el EXITED, 12 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 31 | 16 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1474 | - | KILL a los 1500 ms; el EXITED, 33 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 22 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 19 | 17 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 31 | 12 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 22 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=4128638 fsUsed=90112 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras DestruyeLanza: uptimeMs=4129104 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras DestruyeGc: uptimeMs=4129583 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras DestruyeFinal: uptimeMs=4130053 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras DestruyeTrasError: uptimeMs=4130540 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras KillDestruye: uptimeMs=4131650 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras SueltaPwmA: uptimeMs=4133580 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras SueltaPwmB: uptimeMs=4134065 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras SueltaNpA: uptimeMs=4135900 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras SueltaNpB: uptimeMs=4136393 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras SueltaRedA: uptimeMs=4136928 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras SueltaPwmB2: uptimeMs=4137401 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
- tras SueltaNpB2: uptimeMs=4137855 fsUsed=86016 vmHeapBytes=49152 pila libre: run=7720 io=8848 tareaREPL=8848 heap del sistema, minimo libre=83304
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
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 2650 | c65bec37 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 2871 | 3dc9efcd | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1888 | 4edb10e5 | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 2037 | 04bf67eb | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 3181 | 2f35566d | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 2871 | 3dc9efcd | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 2037 | 04bf67eb | ok |

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

| prueba | esp32c3@COM3 |
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


