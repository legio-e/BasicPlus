# Tandas T1 — las placas conducidas

## Tanda 2026-10-08 12:16:56

- manifiesto: `bpgenvm-c\tools\tanda_667.json` (13 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-08 11:41:26) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-08 11:40:49) - sabor `v2-gui1-lvgl0`
- placas conectadas: 4 (rp2350a@COM22, rp2350b@COM4, esp32s3@COM9, esp32c5@COM6)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| DestruyeArbol | DestruyeArbol | 4714aaf7 | 6754 | ok | 0 | 19 | 10 |
| DestruyeLanza | DestruyeLanza | 4aeaac9d | 2930 | ok | 0 | 9 | 4 |
| DestruyeGc | DestruyeGc | 19fe2788 | 2349 | ok | 0 | 9 | 5 |
| DestruyeFinal | DestruyeFinal | 960fb864 | 1430 | ok | 0 | 9 | 4 |
| DestruyeTrasError | DestruyeTrasError | 91c1307f | 1331 | ok | 1 | 9 | 3 |
| KillDestruye | KillDestruye | 56ff2a4e | 1306 | ok | - | - | - |
| SueltaPwmA | SueltaPwmA | 8b472fdb | 1930 | ok | - | - | - |
| SueltaPwmB | SueltaPwmB | 41eaf94a | 1993 | ok | 0 | 14 | 14 |
| SueltaNpA | SueltaNpA | 08f01efa | 1515 | ok | - | - | - |
| SueltaNpB | SueltaNpB | f435f79b | 1662 | ok | 0 | 12 | 1 |
| SueltaRedA | SueltaRedA | 592fd517 | 2165 | ok | 0 | 15 | 9 |
| SueltaPwmB2 | SueltaPwmB | 41eaf94a | 1993 | ok | 0 | 12 | 14 |
| SueltaNpB2 | SueltaNpB | f435f79b | 1662 | ok | 0 | 10 | 1 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  8 2026 08:51:35` imageCrc=`24a0ed2e` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (08-10 12:02)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=171791 fs=237568/1048576 B vmHeapBytes=219344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 30 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 51 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 183 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 18 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 16 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 171 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 830 | - | KILL a los 800 ms; el EXITED, 11 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1047 | - | KILL a los 1500 ms; el EXITED, 13 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 25 | 14 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1240 | - | KILL a los 1500 ms; el EXITED, 16 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 25 | 12 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 17 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 26 | 12 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 25 | 10 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=173478 fsUsed=245760 vmHeapBytes=219344 pila libre: run=13240 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras DestruyeLanza: uptimeMs=174021 fsUsed=241664 vmHeapBytes=219344 pila libre: run=12708 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras DestruyeGc: uptimeMs=174563 fsUsed=241664 vmHeapBytes=219344 pila libre: run=12496 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras DestruyeFinal: uptimeMs=175152 fsUsed=241664 vmHeapBytes=219344 pila libre: run=12496 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras DestruyeTrasError: uptimeMs=175676 fsUsed=241664 vmHeapBytes=219344 pila libre: run=12708 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras KillDestruye: uptimeMs=176797 fsUsed=237568 vmHeapBytes=219344 pila libre: run=12708 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras SueltaPwmA: uptimeMs=178640 fsUsed=237568 vmHeapBytes=219344 pila libre: run=12708 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras SueltaPwmB: uptimeMs=179582 fsUsed=237568 vmHeapBytes=219344 pila libre: run=12496 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras SueltaNpA: uptimeMs=181451 fsUsed=237568 vmHeapBytes=219344 pila libre: run=12708 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras SueltaNpB: uptimeMs=182389 fsUsed=237568 vmHeapBytes=219344 pila libre: run=12496 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras SueltaRedA: uptimeMs=183353 fsUsed=237568 vmHeapBytes=219344 pila libre: run=12708 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras SueltaPwmB2: uptimeMs=184319 fsUsed=237568 vmHeapBytes=219344 pila libre: run=12708 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras SueltaNpB2: uptimeMs=185269 fsUsed=237568 vmHeapBytes=219344 pila libre: run=12708 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
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
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 1930 | 8b472fdb | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 1993 | 41eaf94a | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1515 | 08f01efa | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 1662 | f435f79b | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 2165 | 592fd517 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 1993 | 41eaf94a | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 1662 | f435f79b | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: DESFASE de compilación —la placa tiene en /lib/Collections.mod la versión 7 (>= la mínima 7), crc 263868cb != el de hoy 738b2934: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Str: DESFASE de compilación —la placa tiene en /lib/Str.mod la versión 7 (>= la mínima 7), crc 9765bb28 != el de hoy 4320a024: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Pwm: ya en /lib/Pwm.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Uart: ya en /lib/Uart.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Neopixel: ya en /lib/Neopixel.mod (crc igual; resuelto por nombre, como el RUN)
- limpieza: 0 fichero(s) de /app borrados

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Oct  8 2026 08:51:35` imageCrc=`24a0ed2e` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (08-10 12:02)
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=169509 fs=188416/10248192 B vmHeapBytes=5767168
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 24 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 67 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 156 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 28 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 27 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 155 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 850 | - | KILL a los 800 ms; el EXITED, 15 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1068 | - | KILL a los 1500 ms; el EXITED, 13 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 38 | 14 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1310 | - | KILL a los 1500 ms; el EXITED, 13 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 34 | 12 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 29 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 38 | 12 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 34 | 10 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=170768 fsUsed=196608 vmHeapBytes=5767168 pila libre: run=13252 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeLanza: uptimeMs=171363 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=13252 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeGc: uptimeMs=171899 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeFinal: uptimeMs=172442 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=13164 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeTrasError: uptimeMs=172982 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras KillDestruye: uptimeMs=174100 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=13164 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaPwmA: uptimeMs=175861 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=13164 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaPwmB: uptimeMs=176797 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaNpA: uptimeMs=178558 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=13164 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaNpB: uptimeMs=179071 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaRedA: uptimeMs=180004 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=13164 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaPwmB2: uptimeMs=181006 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaNpB2: uptimeMs=181533 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=13164 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
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
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 1930 | 8b472fdb | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 1993 | 41eaf94a | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1515 | 08f01efa | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 1662 | f435f79b | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 2165 | 592fd517 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 1993 | 41eaf94a | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 1662 | f435f79b | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: DESFASE de compilación —la placa tiene en /lib/Collections.mod la versión 7 (>= la mínima 7), crc 263868cb != el de hoy 738b2934: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Str: DESFASE de compilación —la placa tiene en pack:SQLite/Str.mod la versión 7 (>= la mínima 7), crc 8e56f753 != el de hoy 4320a024: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Pwm: ya en /lib/Pwm.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Uart: ya en /lib/Uart.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Neopixel: ya en /lib/Neopixel.mod (crc igual; resuelto por nombre, como el RUN)
- limpieza: 0 fichero(s) de /app borrados

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Oct  8 2026 12:03:33` imageCrc=`7d294a40` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (08-10 12:15)
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=125484 fs=221184/10248192 B vmHeapBytes=7336960
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 31 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 117 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 41 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 41 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 27 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 20 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 650 | - | KILL a los 800 ms; el EXITED, 35 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 478 | - | KILL a los 1500 ms; el EXITED, 35 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 72 | 14 | 7 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 918 | - | KILL a los 1500 ms; el EXITED, 41 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 43 | 12 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 28 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 65 | 12 | 7 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 41 | 10 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=128911 fsUsed=229376 vmHeapBytes=7336960 pila libre: run=13124 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras DestruyeLanza: uptimeMs=130107 fsUsed=225280 vmHeapBytes=7336960 pila libre: run=12492 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras DestruyeGc: uptimeMs=131090 fsUsed=225280 vmHeapBytes=7336960 pila libre: run=12212 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras DestruyeFinal: uptimeMs=131966 fsUsed=225280 vmHeapBytes=7336960 pila libre: run=12524 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras DestruyeTrasError: uptimeMs=132837 fsUsed=225280 vmHeapBytes=7336960 pila libre: run=12212 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras KillDestruye: uptimeMs=134225 fsUsed=225280 vmHeapBytes=7336960 pila libre: run=12524 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras SueltaPwmA: uptimeMs=136442 fsUsed=221184 vmHeapBytes=7336960 pila libre: run=12524 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras SueltaPwmB: uptimeMs=138587 fsUsed=221184 vmHeapBytes=7336960 pila libre: run=12524 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras SueltaNpA: uptimeMs=140734 fsUsed=221184 vmHeapBytes=7336960 pila libre: run=12212 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras SueltaNpB: uptimeMs=142250 fsUsed=221184 vmHeapBytes=7336960 pila libre: run=12212 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236935
- tras SueltaRedA: uptimeMs=144350 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=12212 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236775
- tras SueltaPwmB2: uptimeMs=146238 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=12524 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236775
- tras SueltaNpB2: uptimeMs=147656 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=12212 io=8076 tareaREPL=8076 heap del sistema, minimo libre=236775
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
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 1930 | 8b472fdb | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 1993 | 41eaf94a | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1515 | 08f01efa | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 1662 | f435f79b | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 2165 | 592fd517 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 1993 | 41eaf94a | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 1662 | f435f79b | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: DESFASE de compilación —la placa tiene en /lib/Collections.mod la versión 7 (>= la mínima 7), crc 263868cb != el de hoy 738b2934: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Str: DESFASE de compilación —la placa tiene en /lib/Str.mod la versión 7 (>= la mínima 7), crc 9765bb28 != el de hoy 4320a024: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Pwm: ya en /lib/Pwm.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Uart: ya en /lib/Uart.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Neopixel: ya en /lib/Neopixel.mod (crc igual; resuelto por nombre, como el RUN)
- limpieza: 0 fichero(s) de /app borrados

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Oct  8 2026 09:02:20` imageCrc=`921f7fe0` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c5/build/bpvm_esp32c5.bin (08-10 12:15)
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=unknown uptimeMs=110280 fs=266240/16777216 B vmHeapBytes=6288384
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 24 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 39 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 19 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 17 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 15 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 15 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 849 | - | KILL a los 800 ms; el EXITED, 24 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1116 | - | KILL a los 1500 ms; el EXITED, 79 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 39 | 14 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1279 | - | KILL a los 1500 ms; el EXITED, 33 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 22 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 22 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 39 | 12 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 23 | 10 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=111752 fsUsed=274432 vmHeapBytes=6288384 pila libre: run=13384 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197291
- tras DestruyeLanza: uptimeMs=112283 fsUsed=270336 vmHeapBytes=6288384 pila libre: run=12820 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197291
- tras DestruyeGc: uptimeMs=112735 fsUsed=270336 vmHeapBytes=6288384 pila libre: run=12352 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197291
- tras DestruyeFinal: uptimeMs=113202 fsUsed=270336 vmHeapBytes=6288384 pila libre: run=12352 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197291
- tras DestruyeTrasError: uptimeMs=113647 fsUsed=270336 vmHeapBytes=6288384 pila libre: run=12352 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197291
- tras KillDestruye: uptimeMs=114742 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197291
- tras SueltaPwmA: uptimeMs=116572 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8864 tareaREPL=8864 heap del sistema, minimo libre=197291
- tras SueltaPwmB: uptimeMs=117453 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8864 tareaREPL=8864 heap del sistema, minimo libre=197291
- tras SueltaNpA: uptimeMs=119212 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8864 tareaREPL=8864 heap del sistema, minimo libre=197291
- tras SueltaNpB: uptimeMs=119712 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8864 tareaREPL=8864 heap del sistema, minimo libre=197291
- tras SueltaRedA: uptimeMs=120640 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8864 tareaREPL=8864 heap del sistema, minimo libre=197291
- tras SueltaPwmB2: uptimeMs=121525 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8864 tareaREPL=8864 heap del sistema, minimo libre=197291
- tras SueltaNpB2: uptimeMs=122025 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8864 tareaREPL=8864 heap del sistema, minimo libre=197291
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
| /app/SueltaPwmA.mod | bpgenvm-c\build\tanda\SueltaPwmA\SueltaPwmA.mod | 1930 | 8b472fdb | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB\SueltaPwmB.mod | 1993 | 41eaf94a | ok |
| /app/SueltaNpA.mod | bpgenvm-c\build\tanda\SueltaNpA\SueltaNpA.mod | 1515 | 08f01efa | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB\SueltaNpB.mod | 1662 | f435f79b | ok |
| /app/SueltaRedA.mod | bpgenvm-c\build\tanda\SueltaRedA\SueltaRedA.mod | 2165 | 592fd517 | ok |
| /app/SueltaPwmB.mod | bpgenvm-c\build\tanda\SueltaPwmB2\SueltaPwmB.mod | 1993 | 41eaf94a | ok |
| /app/SueltaNpB.mod | bpgenvm-c\build\tanda\SueltaNpB2\SueltaNpB.mod | 1662 | f435f79b | ok |

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

| prueba | rp2350a@COM22 | rp2350b@COM4 | esp32s3@COM9 | esp32c5@COM6 |
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


