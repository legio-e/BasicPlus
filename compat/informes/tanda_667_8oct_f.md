# Tandas T1 — las placas conducidas

## Tanda 2026-10-08 18:35:42

- manifiesto: `bpgenvm-c\tools\tanda_667.json` (13 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-08 14:11:16) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-08 17:57:08) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| DestruyeArbol | DestruyeArbol | 4714aaf7 | 6754 | ok | 0 | 19 | 10 |
| DestruyeLanza | DestruyeLanza | 4aeaac9d | 2930 | ok | 0 | 9 | 4 |
| DestruyeGc | DestruyeGc | 19fe2788 | 2349 | ok | 0 | 9 | 5 |
| DestruyeFinal | DestruyeFinal | 960fb864 | 1430 | ok | 0 | 8 | 4 |
| DestruyeTrasError | DestruyeTrasError | 91c1307f | 1331 | ok | 1 | 11 | 3 |
| KillDestruye | KillDestruye | 56ff2a4e | 1306 | ok | - | - | - |
| SueltaPwmA | SueltaPwmA | 054b49b9 | 2499 | ok | - | - | - |
| SueltaPwmB | SueltaPwmB | f9cdeb41 | 2708 | ok | 0 | 16 | 14 |
| SueltaNpA | SueltaNpA | 6b9560d5 | 1812 | ok | - | - | - |
| SueltaNpB | SueltaNpB | c3db9d9f | 1958 | ok | 0 | 11 | 1 |
| SueltaRedA | SueltaRedA | 6e91e01c | 2967 | ok | 0 | 15 | 9 |
| SueltaPwmB2 | SueltaPwmB | f9cdeb41 | 2708 | ok | 0 | 14 | 14 |
| SueltaNpB2 | SueltaNpB | c3db9d9f | 1958 | ok | 0 | 11 | 1 |

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Oct  8 2026 17:57:20` imageCrc=`7f42e63c` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.elf (08-10 17:57)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=144702 fs=131072/516096 B vmHeapBytes=196608
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 0 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 114 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 37 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 44 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 29 | 8 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 21 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 856 | - | KILL a los 800 ms; el EXITED, 28 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1280 | - | KILL a los 1500 ms; el EXITED, 19 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 47 | 16 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1474 | - | KILL a los 1500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 16 | 11 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 35 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 46 | 14 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 17 | 11 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=146807 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeLanza: uptimeMs=147634 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeGc: uptimeMs=148426 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeFinal: uptimeMs=149131 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeTrasError: uptimeMs=149824 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras KillDestruye: uptimeMs=151111 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmA: uptimeMs=153229 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmB: uptimeMs=154072 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpA: uptimeMs=156180 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpB: uptimeMs=156938 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaRedA: uptimeMs=157796 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmB2: uptimeMs=158621 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpB2: uptimeMs=159364 fsUsed=139264 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
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

### Matriz pruebas x placas

| prueba | stm32u575@COM5 |
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


