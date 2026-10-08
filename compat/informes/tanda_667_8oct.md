# Tandas T1 — las placas conducidas

## Tanda 2026-10-08 11:02:12

- manifiesto: `bpgenvm-c\tools\tanda_667.json` (13 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-08 08:43:12) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-08 08:49:04) - sabor `v2-gui1-lvgl0`
- placas conectadas: 4 (rp2350a@COM22, rp2350b@COM4, esp32s3@COM9, esp32c5@COM6)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| DestruyeArbol | DestruyeArbol | 4714aaf7 | 6754 | ok | 0 | 16 | 10 |
| DestruyeLanza | DestruyeLanza | 4aeaac9d | 2930 | ok | 0 | 9 | 4 |
| DestruyeGc | DestruyeGc | 19fe2788 | 2349 | ok | 0 | 11 | 5 |
| DestruyeFinal | DestruyeFinal | 960fb864 | 1430 | ok | 0 | 19 | 4 |
| DestruyeTrasError | DestruyeTrasError | 91c1307f | 1331 | ok | 1 | 8 | 3 |
| KillDestruye | KillDestruye | 56ff2a4e | 1306 | ok | - | - | - |
| SueltaPwmA | SueltaPwmA | 8b472fdb | 1930 | ok | - | - | - |
| SueltaPwmB | SueltaPwmB | 41eaf94a | 1993 | ok | 0 | 16 | 14 |
| SueltaNpA | SueltaNpA | 08f01efa | 1515 | ok | - | - | - |
| SueltaNpB | SueltaNpB | f435f79b | 1662 | ok | 0 | 12 | 1 |
| SueltaRedA | SueltaRedA | 592fd517 | 2165 | ok | 0 | 15 | 9 |
| SueltaPwmB2 | SueltaPwmB | 41eaf94a | 1993 | ok | 0 | 12 | 14 |
| SueltaNpB2 | SueltaNpB | f435f79b | 1662 | ok | 0 | 10 | 1 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  8 2026 08:51:35` imageCrc=`d82f0e3f` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (08-10 10:41)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=517749 fs=229376/1048576 B vmHeapBytes=219344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 28 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 52 | 16 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 181 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 19 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 15 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 173 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 634 | - | KILL a los 800 ms; el EXITED, 10 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 867 | - | KILL a los 1500 ms; el EXITED, 12 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 25 | 16 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1255 | - | KILL a los 1500 ms; el EXITED, 16 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 25 | 12 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 17 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 25 | 12 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 26 | 10 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=519595 fsUsed=237568 vmHeapBytes=219344 pila libre: run=13244 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeLanza: uptimeMs=520549 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeGc: uptimeMs=521083 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12500 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeFinal: uptimeMs=521607 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeTrasError: uptimeMs=522140 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12500 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras KillDestruye: uptimeMs=523129 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaPwmA: uptimeMs=524851 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12500 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaPwmB: uptimeMs=525804 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaNpA: uptimeMs=527656 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12500 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaNpB: uptimeMs=528630 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaRedA: uptimeMs=529979 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12500 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaPwmB2: uptimeMs=530938 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12808 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaNpB2: uptimeMs=531895 fsUsed=233472 vmHeapBytes=219344 pila libre: run=12500 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
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

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Oct  8 2026 08:51:35` imageCrc=`d82f0e3f` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (08-10 10:41)
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=493353 fs=188416/10248192 B vmHeapBytes=5767168
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 24 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 68 | 16 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 160 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 29 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 27 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 154 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 660 | - | KILL a los 800 ms; el EXITED, 14 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 1079 | - | KILL a los 1500 ms; el EXITED, 14 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 37 | 16 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1264 | - | KILL a los 1500 ms; el EXITED, 14 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 34 | 12 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 28 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 38 | 12 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 35 | 10 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=494661 fsUsed=196608 vmHeapBytes=5767168 pila libre: run=13296 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeLanza: uptimeMs=495235 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12812 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeGc: uptimeMs=495777 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12832 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeFinal: uptimeMs=496304 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12832 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras DestruyeTrasError: uptimeMs=496826 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12812 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras KillDestruye: uptimeMs=497783 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12812 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaPwmA: uptimeMs=499697 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12832 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaPwmB: uptimeMs=500646 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12832 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaNpA: uptimeMs=502433 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12832 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaNpB: uptimeMs=503399 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12832 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaRedA: uptimeMs=504346 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12812 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaPwmB2: uptimeMs=505368 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12832 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras SueltaNpB2: uptimeMs=506336 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12812 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
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

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Oct  8 2026 09:00:36` imageCrc=`4aa525f2` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (08-10 10:55)
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=449700 fs=208896/10248192 B vmHeapBytes=7336960
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 28 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 112 | 16 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 42 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 39 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 28 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 19 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 457 | - | KILL a los 800 ms; el EXITED, 36 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 36 | - | KILL a los 1500 ms; el EXITED, 510 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 71 | 16 | 7 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 258 | - | KILL a los 1500 ms; el EXITED, 34 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 41 | 12 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 28 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 71 | 12 | 7 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 42 | 10 | 4 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=454315 fsUsed=217088 vmHeapBytes=7336960 pila libre: run=13124 io=8092 tareaREPL=8092 heap del sistema, minimo libre=236951
- tras DestruyeLanza: uptimeMs=455713 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12492 io=8092 tareaREPL=8092 heap del sistema, minimo libre=236951
- tras DestruyeGc: uptimeMs=457181 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12212 io=8092 tareaREPL=8092 heap del sistema, minimo libre=236951
- tras DestruyeFinal: uptimeMs=458177 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12212 io=8092 tareaREPL=8092 heap del sistema, minimo libre=236951
- tras DestruyeTrasError: uptimeMs=459400 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12212 io=8044 tareaREPL=8044 heap del sistema, minimo libre=236951
- tras KillDestruye: uptimeMs=460685 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=13084 io=8044 tareaREPL=8044 heap del sistema, minimo libre=236951
- tras SueltaPwmA: uptimeMs=463448 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12828 io=8044 tareaREPL=8044 heap del sistema, minimo libre=236951
- tras SueltaPwmB: uptimeMs=466318 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12828 io=8044 tareaREPL=8044 heap del sistema, minimo libre=236951
- tras SueltaNpA: uptimeMs=468304 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12212 io=8044 tareaREPL=8044 heap del sistema, minimo libre=236951
- tras SueltaNpB: uptimeMs=470276 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12212 io=8044 tareaREPL=8044 heap del sistema, minimo libre=236951
- tras SueltaRedA: uptimeMs=473596 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12212 io=8044 tareaREPL=8044 heap del sistema, minimo libre=236775
- tras SueltaPwmB2: uptimeMs=476499 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12212 io=8044 tareaREPL=8044 heap del sistema, minimo libre=236775
- tras SueltaNpB2: uptimeMs=478629 fsUsed=212992 vmHeapBytes=7336960 pila libre: run=12212 io=8044 tareaREPL=8044 heap del sistema, minimo libre=236775
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

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Oct  8 2026 09:02:20` imageCrc=`c43b78af` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c5/build/bpvm_esp32c5.bin (08-10 10:56)
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=unknown uptimeMs=422583 fs=262144/16777216 B vmHeapBytes=6288384
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 23 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| DestruyeArbol | **IDENTICO** | 40 | 16 | EXITED OK exit=0; host rc=0 |
| DestruyeLanza | **IDENTICO** | 19 | 9 | EXITED OK exit=0; host rc=0 |
| DestruyeGc | **IDENTICO** | 18 | 11 | EXITED OK exit=0; host rc=0 |
| DestruyeFinal | **IDENTICO** | 15 | 19 | EXITED OK exit=0; host rc=0 |
| DestruyeTrasError | **IDENTICO** | 15 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: División por cero); host rc=1 |
| KillDestruye | **PARADA** | 650 | - | KILL a los 800 ms; el EXITED, 12 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmA | **PARADA** | 705 | - | KILL a los 1500 ms; el EXITED, 59 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaPwmB | **ASERCIONES** | 39 | 16 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpA | **PARADA** | 1058 | - | KILL a los 1500 ms; el EXITED, 47 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| SueltaNpB | **ASERCIONES** | 23 | 12 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaRedA | **NO-PETA** | 21 | 15 | EXITED OK exit=0 |
| SueltaPwmB2 | **ASERCIONES** | 39 | 12 | 5 comprobaciones OK y ninguna mala (EXITED OK exit=0) |
| SueltaNpB2 | **ASERCIONES** | 23 | 10 | 2 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras DestruyeArbol: uptimeMs=424502 fsUsed=270336 vmHeapBytes=6288384 pila libre: run=13384 io=8928 tareaREPL=8928 heap del sistema, minimo libre=197307
- tras DestruyeLanza: uptimeMs=425053 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12820 io=8928 tareaREPL=8928 heap del sistema, minimo libre=197307
- tras DestruyeGc: uptimeMs=425536 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8928 tareaREPL=8928 heap del sistema, minimo libre=197307
- tras DestruyeFinal: uptimeMs=426005 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8928 tareaREPL=8928 heap del sistema, minimo libre=197307
- tras DestruyeTrasError: uptimeMs=426470 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8928 tareaREPL=8928 heap del sistema, minimo libre=197307
- tras KillDestruye: uptimeMs=427442 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8928 tareaREPL=8928 heap del sistema, minimo libre=197307
- tras SueltaPwmA: uptimeMs=429242 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197307
- tras SueltaPwmB: uptimeMs=430559 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197307
- tras SueltaNpA: uptimeMs=432292 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197307
- tras SueltaNpB: uptimeMs=433176 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197307
- tras SueltaRedA: uptimeMs=434500 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8880 tareaREPL=8880 heap del sistema, minimo libre=197307
- tras SueltaPwmB2: uptimeMs=435819 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8864 tareaREPL=8864 heap del sistema, minimo libre=197307
- tras SueltaNpB2: uptimeMs=436780 fsUsed=266240 vmHeapBytes=6288384 pila libre: run=12352 io=8864 tareaREPL=8864 heap del sistema, minimo libre=197307
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


