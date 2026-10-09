# Tandas T1 — las placas conducidas

## Tanda 2026-10-09 09:15:04

- manifiesto: `bpgenvm-c\tools\tanda_667.json` (13 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-09 08:24:01) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-09 08:13:46) - sabor `v2-gui1-lvgl0`
- placas conectadas: 4 (rp2350a@COM22, esp32p4@COM15, stm32u5g9@COM12, esp32s3@COM9)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| DestruyeArbol | DestruyeArbol | 4714aaf7 | 6754 | ok | 0 | 17 | 10 |
| DestruyeLanza | DestruyeLanza | 4aeaac9d | 2930 | ok | 0 | 11 | 4 |
| DestruyeGc | DestruyeGc | 19fe2788 | 2349 | ok | 0 | 10 | 5 |
| DestruyeFinal | DestruyeFinal | 960fb864 | 1430 | ok | 0 | 8 | 4 |
| DestruyeTrasError | DestruyeTrasError | 91c1307f | 1331 | ok | 1 | 8 | 3 |
| KillDestruye | KillDestruye | 56ff2a4e | 1306 | ok | - | - | - |
| SueltaPwmA | SueltaPwmA | 054b49b9 | 2499 | ok | - | - | - |
| SueltaPwmB | SueltaPwmB | f9cdeb41 | 2708 | ok | 0 | 17 | 14 |
| SueltaNpA | SueltaNpA | 6b9560d5 | 1812 | ok | - | - | - |
| SueltaNpB | SueltaNpB | c3db9d9f | 1958 | ok | 0 | 11 | 1 |
| SueltaRedA | SueltaRedA | 6e91e01c | 2967 | ok | 0 | 17 | 9 |
| SueltaPwmB2 | SueltaPwmB | f9cdeb41 | 2708 | ok | 0 | 12 | 14 |
| SueltaNpB2 | SueltaNpB | c3db9d9f | 1958 | ok | 0 | 9 | 1 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  9 2026 08:19:47` imageCrc=`219d7796` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (09-10 08:19)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=486461 fs=114688/1048576 B vmHeapBytes=219104
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 12 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 53 | 17 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 169 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 19 | 10 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 16 | 8 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 162 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 654 | - | KILL a los 800 ms; el EXITED, 14 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1273 | - | KILL a los 1500 ms; el EXITED, 11 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 27 | 17 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1446 | - | KILL a los 1500 ms; el EXITED, 16 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 26 | 11 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 19 | 17 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 27 | 12 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 27 | 9 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=487870 fsUsed=122880 vmHeapBytes=219104 pila libre: run=12176 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras DestruyeLanza: uptimeMs=488406 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12372 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras DestruyeGc: uptimeMs=488939 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12176 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras DestruyeFinal: uptimeMs=489466 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12176 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras DestruyeTrasError: uptimeMs=490002 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12372 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras KillDestruye: uptimeMs=490981 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12176 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras SueltaPwmA: uptimeMs=492727 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12372 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras SueltaPwmB: uptimeMs=493267 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12372 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras SueltaNpA: uptimeMs=495107 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12176 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras SueltaNpB: uptimeMs=495742 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12372 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras SueltaRedA: uptimeMs=496276 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12176 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras SueltaPwmB2: uptimeMs=496810 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12176 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
- tras SueltaNpB2: uptimeMs=497347 fsUsed=118784 vmHeapBytes=219104 pila libre: run=12176 io=8952 tareaREPL=8952 heap del sistema, minimo libre=6072
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

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  9 2026 08:21:58` imageCrc=`aaedab10` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (09-10 09:08)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=410602 fs=163840/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 14 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 103 | 17 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 36 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 39 | 10 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 29 | 8 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 18 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 660 | - | KILL a los 800 ms; el EXITED, 32 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1314 | - | KILL a los 1500 ms; el EXITED, 30 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 27 | 17 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1291 | - | KILL a los 1500 ms; el EXITED, 29 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 18 | 11 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 20 | 17 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 27 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 18 | 9 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=413757 fsUsed=172032 vmHeapBytes=24637440 pila libre: run=12400 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras DestruyeLanza: uptimeMs=414892 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras DestruyeGc: uptimeMs=415759 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras DestruyeFinal: uptimeMs=416513 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras DestruyeTrasError: uptimeMs=417298 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras KillDestruye: uptimeMs=418701 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras SueltaPwmA: uptimeMs=421011 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras SueltaPwmB: uptimeMs=422062 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras SueltaNpA: uptimeMs=424221 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras SueltaNpB: uptimeMs=425232 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras SueltaRedA: uptimeMs=426212 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras SueltaPwmB2: uptimeMs=427254 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
- tras SueltaNpB2: uptimeMs=428221 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=11236 io=24548 tareaREPL=24548 heap del sistema, minimo libre=404135
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

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  9 2026 08:20:23` imageCrc=`8a51ed36` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (09-10 08:20)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=375218 fs=73728/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 0 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 119 | 17 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 38 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 44 | 10 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 30 | 8 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 23 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 836 | - | KILL a los 800 ms; el EXITED, 31 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1468 | - | KILL a los 1500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 33 | 17 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1467 | - | KILL a los 1500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 18 | 11 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 37 | 17 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 32 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 18 | 9 | 1 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=376833 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeLanza: uptimeMs=377652 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeGc: uptimeMs=378419 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeFinal: uptimeMs=379103 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DestruyeTrasError: uptimeMs=379784 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras KillDestruye: uptimeMs=381004 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmA: uptimeMs=383220 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmB: uptimeMs=384036 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpA: uptimeMs=385961 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpB: uptimeMs=386693 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaRedA: uptimeMs=387513 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaPwmB2: uptimeMs=388327 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SueltaNpB2: uptimeMs=389062 fsUsed=81920 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
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

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Oct  9 2026 08:21:22` imageCrc=`436cc8ec` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (09-10 09:09)
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=401647 fs=212992/10248192 B vmHeapBytes=7336960
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 29 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 111 | 17 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 35 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 39 | 10 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 25 | 8 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 17 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 670 | - | KILL a los 800 ms; el EXITED, 40 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 699 | - | KILL a los 1500 ms; el EXITED, 31 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 63 | 17 | 7 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 880 | - | KILL a los 1500 ms; el EXITED, 35 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 40 | 11 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 29 | 17 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 63 | 12 | 7 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 40 | 9 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=404807 fsUsed=221184 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras DestruyeLanza: uptimeMs=405802 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras DestruyeGc: uptimeMs=406726 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras DestruyeFinal: uptimeMs=407553 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras DestruyeTrasError: uptimeMs=408380 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras KillDestruye: uptimeMs=409724 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras SueltaPwmA: uptimeMs=412017 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras SueltaPwmB: uptimeMs=413809 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras SueltaNpA: uptimeMs=415813 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras SueltaNpB: uptimeMs=417195 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=238295
- tras SueltaRedA: uptimeMs=419340 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=236503
- tras SueltaPwmB2: uptimeMs=421114 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=236503
- tras SueltaNpB2: uptimeMs=422417 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=11292 io=8024 tareaREPL=8024 heap del sistema, minimo libre=236503
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

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: DESFASE de compilación —la placa tiene en /lib/Collections.mod la versión 7 (>= la mínima 7), crc 263868cb != el de hoy 738b2934: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Str: DESFASE de compilación —la placa tiene en /lib/Str.mod la versión 7 (>= la mínima 7), crc 9765bb28 != el de hoy 4320a024: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Pwm: ya en /lib/Pwm.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Uart: ya en /lib/Uart.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Neopixel: ya en /lib/Neopixel.mod (crc igual; resuelto por nombre, como el RUN)
- limpieza: 0 fichero(s) de /app borrados

### Matriz pruebas x placas

| prueba | rp2350a@COM22 | esp32p4@COM15 | stm32u5g9@COM12 | esp32s3@COM9 |
|---|---|---|---|---|
| DestruyeArbol | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| DestruyeLanza | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| DestruyeGc | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| DestruyeFinal | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| DestruyeTrasError | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| KillDestruye | PARADA | PARADA | PARADA | PARADA |
| SueltaPwmA | PARADA | PARADA | PARADA | PARADA |
| SueltaPwmB | ASERCIONES | ASERCIONES | ASERCIONES | ASERCIONES |
| SueltaNpA | PARADA | PARADA | PARADA | PARADA |
| SueltaNpB | ASERCIONES | ASERCIONES | ASERCIONES | ASERCIONES |
| SueltaRedA | NO-PETA | NO-PETA | NO-PETA | NO-PETA |
| SueltaPwmB2 | ASERCIONES | ASERCIONES | ASERCIONES | ASERCIONES |
| SueltaNpB2 | ASERCIONES | ASERCIONES | ASERCIONES | ASERCIONES |


