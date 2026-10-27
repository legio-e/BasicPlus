# Tandas T1 — las placas conducidas

## Tanda 2026-10-27 16:15:46

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote27.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-24 18:27:11) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-23 12:02:19) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 25795d08 | 1611 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 6e8d156c | 295 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 94260145 | 532 | ok | 0 | 323 | 3 |
| MachineId | MachineId | 7b5c3078 | 452 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | f14b60af | 2041 | ok | 0 | 56 | 17 |
| GuiShot | GuiShot | 56ce6117 | 1347 | ok | 0 | 386 | 8 |
| MoveSoloLectura | MoveSoloLectura | e7c0ebd1 | 2822 | ok | 0 | 14 | 27 |
| StrDosCaminos | StrDosCaminos | f38b316e | 3760 | ok | 0 | 9 | 155 |
| Utf8Descodifica | Utf8Descodifica | 71a9c7b6 | 8597 | ok | 0 | 28 | 74 |
| ArrayEnorme | ArrayEnorme | 69f48e33 | 1992 | ok | 0 | 15 | 11 |
| TcpCloseSuelto | TcpCloseSuelto | a0e69546 | 992 | ok | 0 | 12 | 3 |
| ParseNumero | ParseNumero | 8b7bac2b | 3868 | ok | 0 | 9 | 52 |
| CharCodeAtFuera | CharCodeAtFuera | 79d2a84f | 1605 | ok | 0 | 9 | 12 |
| JsonErrores | JsonErrores | d308222b | 3607 | ok | 0 | 22 | 14 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct 23 2026 11:23:53` imageCrc=`a049ea15` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (27-10 16:09)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=80757 fs=200704/1048576 B vmHeapBytes=219016
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 32 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 23 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 155 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 8 | 10 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 56 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 386 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| MoveSoloLectura | **IDENTICO** | 143 | 14 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 234 | 9 | EXITED OK exit=0; host rc=0 |
| Utf8Descodifica | **IDENTICO** | 401 | 28 | EXITED OK exit=0; host rc=0 |
| ArrayEnorme | **IDENTICO** | 28 | 15 | EXITED OK exit=0; host rc=0 |
| TcpCloseSuelto | **IDENTICO** | 9 | 12 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 109 | 9 | EXITED OK exit=0; host rc=0 |
| CharCodeAtFuera | **IDENTICO** | 31 | 9 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 221 | 22 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=81680 fsUsed=204800 vmHeapBytes=219016 pila libre: run=13432 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=82209 fsUsed=204800 vmHeapBytes=219016 pila libre: run=12744 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=83230 fsUsed=204800 vmHeapBytes=219016 pila libre: run=13432 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=83793 fsUsed=204800 vmHeapBytes=219016 pila libre: run=12744 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras MoveSoloLectura: uptimeMs=84418 fsUsed=200704 vmHeapBytes=219016 pila libre: run=12168 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras StrDosCaminos: uptimeMs=85346 fsUsed=196608 vmHeapBytes=219016 pila libre: run=12360 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras Utf8Descodifica: uptimeMs=86723 fsUsed=192512 vmHeapBytes=219016 pila libre: run=12360 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras ArrayEnorme: uptimeMs=87266 fsUsed=184320 vmHeapBytes=219016 pila libre: run=12360 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras TcpCloseSuelto: uptimeMs=87832 fsUsed=184320 vmHeapBytes=219016 pila libre: run=12056 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras ParseNumero: uptimeMs=88401 fsUsed=184320 vmHeapBytes=219016 pila libre: run=12360 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras CharCodeAtFuera: uptimeMs=89050 fsUsed=184320 vmHeapBytes=219016 pila libre: run=12360 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras JsonErrores: uptimeMs=90007 fsUsed=184320 vmHeapBytes=219016 pila libre: run=12056 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- ejecutadas (con EXITED) 12 de 14 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 25795d08 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 6e8d156c | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 94260145 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 7b5c3078 | ok |
| /app/MoveSoloLectura.mod | bpgenvm-c\build\tanda\MoveSoloLectura\MoveSoloLectura.mod | 2822 | e7c0ebd1 | ok |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | f38b316e | ok |
| /app/Utf8Descodifica.mod | bpgenvm-c\build\tanda\Utf8Descodifica\Utf8Descodifica.mod | 8597 | 71a9c7b6 | ok (por trozos) |
| /app/ArrayEnorme.mod | bpgenvm-c\build\tanda\ArrayEnorme\ArrayEnorme.mod | 1992 | 69f48e33 | ok |
| /app/TcpCloseSuelto.mod | bpgenvm-c\build\tanda\TcpCloseSuelto\TcpCloseSuelto.mod | 992 | a0e69546 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 8b7bac2b | ok |
| /app/CharCodeAtFuera.mod | bpgenvm-c\build\tanda\CharCodeAtFuera\CharCodeAtFuera.mod | 1605 | 79d2a84f | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | d308222b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Net: en la zona de packs (pack:Stdlib/Net.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | rp2350a@COM22 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |
| MoveSoloLectura | IDENTICO |
| StrDosCaminos | IDENTICO |
| Utf8Descodifica | IDENTICO |
| ArrayEnorme | IDENTICO |
| TcpCloseSuelto | IDENTICO |
| ParseNumero | IDENTICO |
| CharCodeAtFuera | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-27 16:22:28

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote27.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-24 18:27:11) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-23 12:02:19) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350b@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 25795d08 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 6e8d156c | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 94260145 | 532 | ok | 0 | 315 | 3 |
| MachineId | MachineId | 7b5c3078 | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | f14b60af | 2041 | ok | 0 | 59 | 17 |
| GuiShot | GuiShot | 56ce6117 | 1347 | ok | 0 | 378 | 8 |
| MoveSoloLectura | MoveSoloLectura | e7c0ebd1 | 2822 | ok | 0 | 25 | 27 |
| StrDosCaminos | StrDosCaminos | f38b316e | 3760 | ok | 0 | 9 | 155 |
| Utf8Descodifica | Utf8Descodifica | 71a9c7b6 | 8597 | ok | 0 | 31 | 74 |
| ArrayEnorme | ArrayEnorme | 69f48e33 | 1992 | ok | 0 | 9 | 11 |
| TcpCloseSuelto | TcpCloseSuelto | a0e69546 | 992 | ok | 0 | 9 | 3 |
| ParseNumero | ParseNumero | 8b7bac2b | 3868 | ok | 0 | 9 | 52 |
| CharCodeAtFuera | CharCodeAtFuera | 79d2a84f | 1605 | ok | 0 | 11 | 12 |
| JsonErrores | JsonErrores | d308222b | 3607 | ok | 0 | 19 | 14 |

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Oct 23 2026 11:23:54` imageCrc=`5f25c83d` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico_m.elf (27-10 16:09)
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=55739 fs=192512/10248192 B vmHeapBytes=5767168
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 33 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 23 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 140 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 315 | 315 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 14 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 59 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 378 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| MoveSoloLectura | **IDENTICO** | 106 | 25 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 74 | 9 | EXITED OK exit=0; host rc=0 |
| Utf8Descodifica | **IDENTICO** | 176 | 31 | EXITED OK exit=0; host rc=0 |
| ArrayEnorme | **IDENTICO** | 21 | 9 | EXITED OK exit=0; host rc=0 |
| TcpCloseSuelto | **IDENTICO** | 14 | 9 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 50 | 9 | EXITED OK exit=0; host rc=0 |
| CharCodeAtFuera | **IDENTICO** | 23 | 11 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 163 | 19 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=56486 fsUsed=196608 vmHeapBytes=5767168 pila libre: run=13592 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=57004 fsUsed=196608 vmHeapBytes=5767168 pila libre: run=10880 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=57927 fsUsed=196608 vmHeapBytes=5767168 pila libre: run=13432 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=58555 fsUsed=196608 vmHeapBytes=5767168 pila libre: run=10880 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras MoveSoloLectura: uptimeMs=59159 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=12168 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras StrDosCaminos: uptimeMs=59692 fsUsed=188416 vmHeapBytes=5767168 pila libre: run=10880 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras Utf8Descodifica: uptimeMs=60975 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=10880 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras ArrayEnorme: uptimeMs=61570 fsUsed=176128 vmHeapBytes=5767168 pila libre: run=12144 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras TcpCloseSuelto: uptimeMs=62128 fsUsed=176128 vmHeapBytes=5767168 pila libre: run=10880 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras ParseNumero: uptimeMs=62642 fsUsed=176128 vmHeapBytes=5767168 pila libre: run=12144 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras CharCodeAtFuera: uptimeMs=63162 fsUsed=176128 vmHeapBytes=5767168 pila libre: run=10880 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- tras JsonErrores: uptimeMs=63833 fsUsed=176128 vmHeapBytes=5767168 pila libre: run=12144 io=9736 tareaREPL=9736 heap del sistema, minimo libre=6632
- ejecutadas (con EXITED) 12 de 14 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 25795d08 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 6e8d156c | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 94260145 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 7b5c3078 | ok |
| /app/MoveSoloLectura.mod | bpgenvm-c\build\tanda\MoveSoloLectura\MoveSoloLectura.mod | 2822 | e7c0ebd1 | ok |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | f38b316e | ok |
| /app/Utf8Descodifica.mod | bpgenvm-c\build\tanda\Utf8Descodifica\Utf8Descodifica.mod | 8597 | 71a9c7b6 | ok (por trozos) |
| /app/ArrayEnorme.mod | bpgenvm-c\build\tanda\ArrayEnorme\ArrayEnorme.mod | 1992 | 69f48e33 | ok |
| /app/TcpCloseSuelto.mod | bpgenvm-c\build\tanda\TcpCloseSuelto\TcpCloseSuelto.mod | 992 | a0e69546 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 8b7bac2b | ok |
| /app/CharCodeAtFuera.mod | bpgenvm-c\build\tanda\CharCodeAtFuera\CharCodeAtFuera.mod | 1605 | 79d2a84f | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | d308222b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Net: en la zona de packs (pack:Stdlib/Net.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | rp2350b@COM4 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |
| MoveSoloLectura | IDENTICO |
| StrDosCaminos | IDENTICO |
| Utf8Descodifica | IDENTICO |
| ArrayEnorme | IDENTICO |
| TcpCloseSuelto | IDENTICO |
| ParseNumero | IDENTICO |
| CharCodeAtFuera | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-27 16:24:13

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote27.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-24 18:27:11) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-23 12:02:19) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c3@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 25795d08 | 1611 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 6e8d156c | 295 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 94260145 | 532 | ok | 0 | 326 | 3 |
| MachineId | MachineId | 7b5c3078 | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | f14b60af | 2041 | ok | 0 | 62 | 17 |
| GuiShot | GuiShot | 56ce6117 | 1347 | ok | 0 | 382 | 8 |
| MoveSoloLectura | MoveSoloLectura | e7c0ebd1 | 2822 | ok | 0 | 14 | 27 |
| StrDosCaminos | StrDosCaminos | f38b316e | 3760 | ok | 0 | 10 | 155 |
| Utf8Descodifica | Utf8Descodifica | 71a9c7b6 | 8597 | ok | 0 | 27 | 74 |
| ArrayEnorme | ArrayEnorme | 69f48e33 | 1992 | ok | 0 | 10 | 11 |
| TcpCloseSuelto | TcpCloseSuelto | a0e69546 | 992 | ok | 0 | 11 | 3 |
| ParseNumero | ParseNumero | 8b7bac2b | 3868 | ok | 0 | 9 | 52 |
| CharCodeAtFuera | CharCodeAtFuera | 79d2a84f | 1605 | ok | 0 | 8 | 12 |
| JsonErrores | JsonErrores | d308222b | 3607 | ok | 0 | 19 | 14 |

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Oct 27 2026 16:13:05` imageCrc=`a2c24147` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c3/build/bpvm_esp32c3.bin (27-10 16:13)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=48867 fs=36864/1523712 B vmHeapBytes=49152
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 1 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 7 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 5 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 309 | 326 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 62 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 382 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| MoveSoloLectura | **IDENTICO** | 121 | 14 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 106 | 10 | EXITED OK exit=0; host rc=0 |
| Utf8Descodifica | **IDENTICO** | 377 | 27 | EXITED OK exit=0; host rc=0 |
| ArrayEnorme | **IDENTICO** | 15 | 10 | EXITED OK exit=0; host rc=0 |
| TcpCloseSuelto | **IDENTICO** | 6 | 11 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 64 | 9 | EXITED OK exit=0; host rc=0 |
| CharCodeAtFuera | **IDENTICO** | 19 | 8 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 345 | 19 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=49527 fsUsed=40960 vmHeapBytes=49152 pila libre: run=8540 io=9940 tareaREPL=9940 heap del sistema, minimo libre=91716
- tras ThrowSinAtrapar: uptimeMs=50003 fsUsed=40960 vmHeapBytes=49152 pila libre: run=8540 io=9940 tareaREPL=9940 heap del sistema, minimo libre=91716
- tras ThreadTrasMain: uptimeMs=50550 fsUsed=40960 vmHeapBytes=49152 pila libre: run=8540 io=9940 tareaREPL=9940 heap del sistema, minimo libre=91716
- tras MachineId: uptimeMs=51069 fsUsed=40960 vmHeapBytes=49152 pila libre: run=8540 io=9940 tareaREPL=9940 heap del sistema, minimo libre=91716
- tras MoveSoloLectura: uptimeMs=51637 fsUsed=40960 vmHeapBytes=49152 pila libre: run=7756 io=9900 tareaREPL=9900 heap del sistema, minimo libre=89864
- tras StrDosCaminos: uptimeMs=52152 fsUsed=40960 vmHeapBytes=49152 pila libre: run=7756 io=9900 tareaREPL=9900 heap del sistema, minimo libre=89864
- tras Utf8Descodifica: uptimeMs=53330 fsUsed=49152 vmHeapBytes=49152 pila libre: run=7756 io=9900 tareaREPL=9900 heap del sistema, minimo libre=78864
- tras ArrayEnorme: uptimeMs=53822 fsUsed=40960 vmHeapBytes=49152 pila libre: run=7756 io=9900 tareaREPL=9900 heap del sistema, minimo libre=78864
- tras TcpCloseSuelto: uptimeMs=54356 fsUsed=40960 vmHeapBytes=49152 pila libre: run=7756 io=9900 tareaREPL=9900 heap del sistema, minimo libre=78864
- tras ParseNumero: uptimeMs=54758 fsUsed=40960 vmHeapBytes=49152 pila libre: run=7756 io=9900 tareaREPL=9900 heap del sistema, minimo libre=78864
- tras CharCodeAtFuera: uptimeMs=55241 fsUsed=40960 vmHeapBytes=49152 pila libre: run=7756 io=9900 tareaREPL=9900 heap del sistema, minimo libre=78864
- tras JsonErrores: uptimeMs=56142 fsUsed=40960 vmHeapBytes=49152 pila libre: run=7756 io=9900 tareaREPL=9900 heap del sistema, minimo libre=78864
- ejecutadas (con EXITED) 12 de 14 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 25795d08 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 6e8d156c | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 94260145 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 7b5c3078 | ok |
| /app/MoveSoloLectura.mod | bpgenvm-c\build\tanda\MoveSoloLectura\MoveSoloLectura.mod | 2822 | e7c0ebd1 | ok |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | f38b316e | ok |
| /app/Utf8Descodifica.mod | bpgenvm-c\build\tanda\Utf8Descodifica\Utf8Descodifica.mod | 8597 | 71a9c7b6 | ok (por trozos) |
| /app/ArrayEnorme.mod | bpgenvm-c\build\tanda\ArrayEnorme\ArrayEnorme.mod | 1992 | 69f48e33 | ok |
| /app/TcpCloseSuelto.mod | bpgenvm-c\build\tanda\TcpCloseSuelto\TcpCloseSuelto.mod | 992 | a0e69546 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 8b7bac2b | ok |
| /app/CharCodeAtFuera.mod | bpgenvm-c\build\tanda\CharCodeAtFuera\CharCodeAtFuera.mod | 1605 | 79d2a84f | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | d308222b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Net: en la zona de packs (pack:Stdlib/Net.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | esp32c3@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |
| MoveSoloLectura | IDENTICO |
| StrDosCaminos | IDENTICO |
| Utf8Descodifica | IDENTICO |
| ArrayEnorme | IDENTICO |
| TcpCloseSuelto | IDENTICO |
| ParseNumero | IDENTICO |
| CharCodeAtFuera | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-27 16:26:20

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote27.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-24 18:27:11) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-23 12:02:19) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 25795d08 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 6e8d156c | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 94260145 | 532 | ok | 0 | 327 | 3 |
| MachineId | MachineId | 7b5c3078 | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | f14b60af | 2041 | ok | 0 | 59 | 17 |
| GuiShot | GuiShot | 56ce6117 | 1347 | ok | 0 | 396 | 8 |
| MoveSoloLectura | MoveSoloLectura | e7c0ebd1 | 2822 | ok | 0 | 15 | 27 |
| StrDosCaminos | StrDosCaminos | f38b316e | 3760 | ok | 0 | 10 | 155 |
| Utf8Descodifica | Utf8Descodifica | 71a9c7b6 | 8597 | ok | 0 | 40 | 74 |
| ArrayEnorme | ArrayEnorme | 69f48e33 | 1992 | ok | 0 | 13 | 11 |
| TcpCloseSuelto | TcpCloseSuelto | a0e69546 | 992 | ok | 0 | 9 | 3 |
| ParseNumero | ParseNumero | 8b7bac2b | 3868 | ok | 0 | 11 | 52 |
| CharCodeAtFuera | CharCodeAtFuera | 79d2a84f | 1605 | ok | 0 | 10 | 12 |
| JsonErrores | JsonErrores | d308222b | 3607 | ok | 0 | 21 | 14 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct 27 2026 16:10:29` imageCrc=`db6793d7` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (27-10 16:10)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=51992 fs=114688/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 198 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 9 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 25 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 570 | 63 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 683 | 394 | EXITED OK exit=0 |
| MoveSoloLectura | **IDENTICO** | 279 | 15 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1084 | 10 | EXITED OK exit=0; host rc=0 |
| Utf8Descodifica | **IDENTICO** | 811 | 40 | EXITED OK exit=0; host rc=0 |
| ArrayEnorme | **IDENTICO** | 100 | 13 | EXITED OK exit=0; host rc=0 |
| TcpCloseSuelto | **IDENTICO** | 29 | 9 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 498 | 11 | EXITED OK exit=0; host rc=0 |
| CharCodeAtFuera | **IDENTICO** | 121 | 10 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 223 | 21 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=52995 fsUsed=122880 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=53575 fsUsed=122880 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=54172 fsUsed=122880 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=54809 fsUsed=122880 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiWinJson: uptimeMs=57496 fsUsed=131072 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiShot: uptimeMs=59778 fsUsed=139264 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: no existia
- tras MoveSoloLectura: uptimeMs=62027 fsUsed=139264 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras StrDosCaminos: uptimeMs=63765 fsUsed=139264 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras Utf8Descodifica: uptimeMs=66010 fsUsed=147456 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayEnorme: uptimeMs=66746 fsUsed=139264 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras TcpCloseSuelto: uptimeMs=67446 fsUsed=139264 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ParseNumero: uptimeMs=68780 fsUsed=139264 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CharCodeAtFuera: uptimeMs=69481 fsUsed=139264 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras JsonErrores: uptimeMs=70782 fsUsed=139264 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 25795d08 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 6e8d156c | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 94260145 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 7b5c3078 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | f14b60af | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 56ce6117 | ok |
| /app/MoveSoloLectura.mod | bpgenvm-c\build\tanda\MoveSoloLectura\MoveSoloLectura.mod | 2822 | e7c0ebd1 | ok |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | f38b316e | ok |
| /app/Utf8Descodifica.mod | bpgenvm-c\build\tanda\Utf8Descodifica\Utf8Descodifica.mod | 8597 | 71a9c7b6 | ok (por trozos) |
| /app/ArrayEnorme.mod | bpgenvm-c\build\tanda\ArrayEnorme\ArrayEnorme.mod | 1992 | 69f48e33 | ok |
| /app/TcpCloseSuelto.mod | bpgenvm-c\build\tanda\TcpCloseSuelto\TcpCloseSuelto.mod | 992 | a0e69546 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 8b7bac2b | ok |
| /app/CharCodeAtFuera.mod | bpgenvm-c\build\tanda\CharCodeAtFuera\CharCodeAtFuera.mod | 1605 | 79d2a84f | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | d308222b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Net: en la zona de packs (pack:Stdlib/Net.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | stm32u5g9@COM12 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |
| MoveSoloLectura | IDENTICO |
| StrDosCaminos | IDENTICO |
| Utf8Descodifica | IDENTICO |
| ArrayEnorme | IDENTICO |
| TcpCloseSuelto | IDENTICO |
| ParseNumero | IDENTICO |
| CharCodeAtFuera | IDENTICO |
| JsonErrores | IDENTICO |

NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  


## Tanda 2026-10-27 16:32:19

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote27.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-24 18:27:11) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-23 12:02:19) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 25795d08 | 1611 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 6e8d156c | 295 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 94260145 | 532 | ok | 0 | 327 | 3 |
| MachineId | MachineId | 7b5c3078 | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | f14b60af | 2041 | ok | 0 | 60 | 17 |
| GuiShot | GuiShot | 56ce6117 | 1347 | ok | 0 | 397 | 8 |
| MoveSoloLectura | MoveSoloLectura | e7c0ebd1 | 2822 | ok | 0 | 18 | 27 |
| StrDosCaminos | StrDosCaminos | f38b316e | 3760 | ok | 0 | 11 | 155 |
| Utf8Descodifica | Utf8Descodifica | 71a9c7b6 | 8597 | ok | 0 | 33 | 74 |
| ArrayEnorme | ArrayEnorme | 69f48e33 | 1992 | ok | 0 | 9 | 11 |
| TcpCloseSuelto | TcpCloseSuelto | a0e69546 | 992 | ok | 0 | 10 | 3 |
| ParseNumero | ParseNumero | 8b7bac2b | 3868 | ok | 0 | 10 | 52 |
| CharCodeAtFuera | CharCodeAtFuera | 79d2a84f | 1605 | ok | 0 | 11 | 12 |
| JsonErrores | JsonErrores | d308222b | 3607 | ok | 0 | 20 | 14 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct 27 2026 16:10:00` imageCrc=`a1f1564d` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (27-10 16:10)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=22527 fs=184320/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 21 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 192 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 11 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 21 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 728 | 65 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 618 | 373 | EXITED OK exit=0 |
| MoveSoloLectura | **IDENTICO** | 281 | 18 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1069 | 11 | EXITED OK exit=0; host rc=0 |
| Utf8Descodifica | **IDENTICO** | 750 | 33 | EXITED OK exit=0; host rc=0 |
| ArrayEnorme | **IDENTICO** | 94 | 9 | EXITED OK exit=0; host rc=0 |
| TcpCloseSuelto | **IDENTICO** | 25 | 10 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 489 | 10 | EXITED OK exit=0; host rc=0 |
| CharCodeAtFuera | **IDENTICO** | 116 | 11 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 137 | 20 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=23941 fsUsed=188416 vmHeapBytes=24637440 pila libre: run=12524 io=25796 tareaREPL=25796 heap del sistema, minimo libre=369092
- tras ThrowSinAtrapar: uptimeMs=24586 fsUsed=188416 vmHeapBytes=24637440 pila libre: run=12524 io=25796 tareaREPL=25796 heap del sistema, minimo libre=369092
- tras ThreadTrasMain: uptimeMs=25622 fsUsed=188416 vmHeapBytes=24637440 pila libre: run=12524 io=25796 tareaREPL=25796 heap del sistema, minimo libre=369092
- tras MachineId: uptimeMs=26454 fsUsed=188416 vmHeapBytes=24637440 pila libre: run=12524 io=25796 tareaREPL=25796 heap del sistema, minimo libre=369092
- tras GuiWinJson: uptimeMs=29775 fsUsed=192512 vmHeapBytes=24637440 pila libre: run=12524 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361732
- tras GuiShot: uptimeMs=32411 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11368 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361608
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- tras MoveSoloLectura: uptimeMs=34882 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11368 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361608
- tras StrDosCaminos: uptimeMs=37253 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11748 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361608
- tras Utf8Descodifica: uptimeMs=39832 fsUsed=208896 vmHeapBytes=24637440 pila libre: run=11748 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361608
- tras ArrayEnorme: uptimeMs=40768 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11748 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361608
- tras TcpCloseSuelto: uptimeMs=41605 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11748 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361608
- tras ParseNumero: uptimeMs=43120 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11748 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361608
- tras CharCodeAtFuera: uptimeMs=44247 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11748 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361608
- tras JsonErrores: uptimeMs=45523 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11748 io=25460 tareaREPL=25460 heap del sistema, minimo libre=361608
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 25795d08 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 6e8d156c | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 94260145 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 7b5c3078 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | f14b60af | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 56ce6117 | ok |
| /app/MoveSoloLectura.mod | bpgenvm-c\build\tanda\MoveSoloLectura\MoveSoloLectura.mod | 2822 | e7c0ebd1 | ok |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | f38b316e | ok |
| /app/Utf8Descodifica.mod | bpgenvm-c\build\tanda\Utf8Descodifica\Utf8Descodifica.mod | 8597 | 71a9c7b6 | ok (por trozos) |
| /app/ArrayEnorme.mod | bpgenvm-c\build\tanda\ArrayEnorme\ArrayEnorme.mod | 1992 | 69f48e33 | ok |
| /app/TcpCloseSuelto.mod | bpgenvm-c\build\tanda\TcpCloseSuelto\TcpCloseSuelto.mod | 992 | a0e69546 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 8b7bac2b | ok |
| /app/CharCodeAtFuera.mod | bpgenvm-c\build\tanda\CharCodeAtFuera\CharCodeAtFuera.mod | 1605 | 79d2a84f | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | d308222b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Net: en la zona de packs (pack:Stdlib/Net.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | esp32p4@COM15 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |
| MoveSoloLectura | IDENTICO |
| StrDosCaminos | IDENTICO |
| Utf8Descodifica | IDENTICO |
| ArrayEnorme | IDENTICO |
| TcpCloseSuelto | IDENTICO |
| ParseNumero | IDENTICO |
| CharCodeAtFuera | IDENTICO |
| JsonErrores | IDENTICO |

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  


## Tanda 2026-10-27 16:33:59

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote27.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-24 18:27:11) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-23 12:02:19) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32s3@COM9)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 25795d08 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 6e8d156c | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 94260145 | 532 | ok | 0 | 315 | 3 |
| MachineId | MachineId | 7b5c3078 | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | f14b60af | 2041 | ok | 0 | 74 | 17 |
| GuiShot | GuiShot | 56ce6117 | 1347 | ok | 0 | 394 | 8 |
| MoveSoloLectura | MoveSoloLectura | e7c0ebd1 | 2822 | ok | 0 | 17 | 27 |
| StrDosCaminos | StrDosCaminos | f38b316e | 3760 | ok | 0 | 9 | 155 |
| Utf8Descodifica | Utf8Descodifica | 71a9c7b6 | 8597 | ok | 0 | 29 | 74 |
| ArrayEnorme | ArrayEnorme | 69f48e33 | 1992 | ok | 0 | 12 | 11 |
| TcpCloseSuelto | TcpCloseSuelto | a0e69546 | 992 | ok | 0 | 12 | 3 |
| ParseNumero | ParseNumero | 8b7bac2b | 3868 | ok | 0 | 9 | 52 |
| CharCodeAtFuera | CharCodeAtFuera | 79d2a84f | 1605 | ok | 0 | 9 | 12 |
| JsonErrores | JsonErrores | d308222b | 3607 | ok | 0 | 19 | 14 |

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Oct 27 2026 16:11:15` imageCrc=`29e72bba` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (27-10 16:11)
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=51503 fs=69632/10248192 B vmHeapBytes=7336960
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 11 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 191 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 11 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 313 | 315 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 19 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 74 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 394 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| MoveSoloLectura | **IDENTICO** | 270 | 17 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1067 | 9 | EXITED OK exit=0; host rc=0 |
| Utf8Descodifica | **IDENTICO** | 754 | 29 | EXITED OK exit=0; host rc=0 |
| ArrayEnorme | **IDENTICO** | 90 | 12 | EXITED OK exit=0; host rc=0 |
| TcpCloseSuelto | **IDENTICO** | 23 | 12 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 486 | 9 | EXITED OK exit=0; host rc=0 |
| CharCodeAtFuera | **IDENTICO** | 114 | 9 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 391 | 19 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=52806 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=12140 io=9348 tareaREPL=9348 heap del sistema, minimo libre=208632
- tras ThrowSinAtrapar: uptimeMs=53418 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=12140 io=9348 tareaREPL=9348 heap del sistema, minimo libre=208632
- tras ThreadTrasMain: uptimeMs=54656 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=12140 io=9348 tareaREPL=9348 heap del sistema, minimo libre=208632
- tras MachineId: uptimeMs=55458 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=12140 io=9348 tareaREPL=9348 heap del sistema, minimo libre=208632
- tras MoveSoloLectura: uptimeMs=57110 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=11452 io=9348 tareaREPL=9348 heap del sistema, minimo libre=206160
- tras StrDosCaminos: uptimeMs=59247 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=11452 io=9348 tareaREPL=9348 heap del sistema, minimo libre=206160
- tras Utf8Descodifica: uptimeMs=61983 fsUsed=81920 vmHeapBytes=7336960 pila libre: run=11452 io=9348 tareaREPL=9348 heap del sistema, minimo libre=205232
- tras ArrayEnorme: uptimeMs=62893 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=11452 io=9348 tareaREPL=9348 heap del sistema, minimo libre=205232
- tras TcpCloseSuelto: uptimeMs=63783 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=11452 io=9348 tareaREPL=9348 heap del sistema, minimo libre=205232
- tras ParseNumero: uptimeMs=65200 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=11452 io=9348 tareaREPL=9348 heap del sistema, minimo libre=205232
- tras CharCodeAtFuera: uptimeMs=66052 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=11452 io=9348 tareaREPL=9348 heap del sistema, minimo libre=205232
- tras JsonErrores: uptimeMs=67620 fsUsed=73728 vmHeapBytes=7336960 pila libre: run=11452 io=9316 tareaREPL=9316 heap del sistema, minimo libre=205232
- ejecutadas (con EXITED) 12 de 14 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 25795d08 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 6e8d156c | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 94260145 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 7b5c3078 | ok |
| /app/MoveSoloLectura.mod | bpgenvm-c\build\tanda\MoveSoloLectura\MoveSoloLectura.mod | 2822 | e7c0ebd1 | ok |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | f38b316e | ok |
| /app/Utf8Descodifica.mod | bpgenvm-c\build\tanda\Utf8Descodifica\Utf8Descodifica.mod | 8597 | 71a9c7b6 | ok (por trozos) |
| /app/ArrayEnorme.mod | bpgenvm-c\build\tanda\ArrayEnorme\ArrayEnorme.mod | 1992 | 69f48e33 | ok |
| /app/TcpCloseSuelto.mod | bpgenvm-c\build\tanda\TcpCloseSuelto\TcpCloseSuelto.mod | 992 | a0e69546 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 8b7bac2b | ok |
| /app/CharCodeAtFuera.mod | bpgenvm-c\build\tanda\CharCodeAtFuera\CharCodeAtFuera.mod | 1605 | 79d2a84f | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | d308222b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Net: en la zona de packs (pack:Stdlib/Net.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | esp32s3@COM9 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |
| MoveSoloLectura | IDENTICO |
| StrDosCaminos | IDENTICO |
| Utf8Descodifica | IDENTICO |
| ArrayEnorme | IDENTICO |
| TcpCloseSuelto | IDENTICO |
| ParseNumero | IDENTICO |
| CharCodeAtFuera | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-27 16:35:28

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote27.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-24 18:27:11) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-23 12:02:19) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 25795d08 | 1611 | ok | 0 | 16 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 6e8d156c | 295 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 94260145 | 532 | ok | 0 | 317 | 3 |
| MachineId | MachineId | 7b5c3078 | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | f14b60af | 2041 | ok | 0 | 62 | 17 |
| GuiShot | GuiShot | 56ce6117 | 1347 | ok | 0 | 384 | 8 |
| MoveSoloLectura | MoveSoloLectura | e7c0ebd1 | 2822 | ok | 0 | 15 | 27 |
| StrDosCaminos | StrDosCaminos | f38b316e | 3760 | ok | 0 | 10 | 155 |
| Utf8Descodifica | Utf8Descodifica | 71a9c7b6 | 8597 | ok | 0 | 32 | 74 |
| ArrayEnorme | ArrayEnorme | 69f48e33 | 1992 | ok | 0 | 10 | 11 |
| TcpCloseSuelto | TcpCloseSuelto | a0e69546 | 992 | ok | 0 | 14 | 3 |
| ParseNumero | ParseNumero | 8b7bac2b | 3868 | ok | 0 | 9 | 52 |
| CharCodeAtFuera | CharCodeAtFuera | 79d2a84f | 1605 | ok | 0 | 9 | 12 |
| JsonErrores | JsonErrores | d308222b | 3607 | ok | 0 | 24 | 14 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct 27 2026 16:12:32` imageCrc=`767c501d` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c6/build/bpvm_esp32c6.bin (27-10 16:12)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=30811 fs=36864/1261568 B vmHeapBytes=62464
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 1 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 7 | 16 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 4 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 312 | 317 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 4 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 982 | 72 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 569 | 422 | EXITED OK exit=0 |
| MoveSoloLectura | **IDENTICO** | 112 | 15 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 72 | 10 | EXITED OK exit=0; host rc=0 |
| Utf8Descodifica | **IDENTICO** | 309 | 32 | EXITED OK exit=0; host rc=0 |
| ArrayEnorme | **IDENTICO** | 12 | 10 | EXITED OK exit=0; host rc=0 |
| TcpCloseSuelto | **IDENTICO** | 5 | 14 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 41 | 9 | EXITED OK exit=0; host rc=0 |
| CharCodeAtFuera | **IDENTICO** | 13 | 9 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 275 | 24 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=31496 fsUsed=40960 vmHeapBytes=62464 pila libre: run=6504 io=5828 tareaREPL=5828 heap del sistema, minimo libre=71912
- tras ThrowSinAtrapar: uptimeMs=31962 fsUsed=40960 vmHeapBytes=62464 pila libre: run=6504 io=5828 tareaREPL=5828 heap del sistema, minimo libre=71912
- tras ThreadTrasMain: uptimeMs=32854 fsUsed=40960 vmHeapBytes=62464 pila libre: run=6504 io=5828 tareaREPL=5828 heap del sistema, minimo libre=71912
- tras MachineId: uptimeMs=33358 fsUsed=40960 vmHeapBytes=62464 pila libre: run=6504 io=5828 tareaREPL=5828 heap del sistema, minimo libre=71912
- tras GuiWinJson: uptimeMs=36033 fsUsed=45056 vmHeapBytes=62464 pila libre: run=6160 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- tras GuiShot: uptimeMs=38134 fsUsed=45056 vmHeapBytes=62464 pila libre: run=5284 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: no existia
- tras MoveSoloLectura: uptimeMs=38835 fsUsed=45056 vmHeapBytes=62464 pila libre: run=5284 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- tras StrDosCaminos: uptimeMs=39321 fsUsed=45056 vmHeapBytes=62464 pila libre: run=5284 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- tras Utf8Descodifica: uptimeMs=40654 fsUsed=53248 vmHeapBytes=62464 pila libre: run=5284 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- tras ArrayEnorme: uptimeMs=41105 fsUsed=45056 vmHeapBytes=62464 pila libre: run=5284 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- tras TcpCloseSuelto: uptimeMs=41671 fsUsed=45056 vmHeapBytes=62464 pila libre: run=5284 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- tras ParseNumero: uptimeMs=42240 fsUsed=45056 vmHeapBytes=62464 pila libre: run=5284 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- tras CharCodeAtFuera: uptimeMs=42705 fsUsed=45056 vmHeapBytes=62464 pila libre: run=5284 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- tras JsonErrores: uptimeMs=43602 fsUsed=45056 vmHeapBytes=62464 pila libre: run=5284 io=5648 tareaREPL=5648 heap del sistema, minimo libre=32596
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 25795d08 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 6e8d156c | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 94260145 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 7b5c3078 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | f14b60af | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 56ce6117 | ok |
| /app/MoveSoloLectura.mod | bpgenvm-c\build\tanda\MoveSoloLectura\MoveSoloLectura.mod | 2822 | e7c0ebd1 | ok |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | f38b316e | ok |
| /app/Utf8Descodifica.mod | bpgenvm-c\build\tanda\Utf8Descodifica\Utf8Descodifica.mod | 8597 | 71a9c7b6 | ok (por trozos) |
| /app/ArrayEnorme.mod | bpgenvm-c\build\tanda\ArrayEnorme\ArrayEnorme.mod | 1992 | 69f48e33 | ok |
| /app/TcpCloseSuelto.mod | bpgenvm-c\build\tanda\TcpCloseSuelto\TcpCloseSuelto.mod | 992 | a0e69546 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 8b7bac2b | ok |
| /app/CharCodeAtFuera.mod | bpgenvm-c\build\tanda\CharCodeAtFuera\CharCodeAtFuera.mod | 1605 | 79d2a84f | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | d308222b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Net: en la zona de packs (pack:Stdlib/Net.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |
| MoveSoloLectura | IDENTICO |
| StrDosCaminos | IDENTICO |
| Utf8Descodifica | IDENTICO |
| ArrayEnorme | IDENTICO |
| TcpCloseSuelto | IDENTICO |
| ParseNumero | IDENTICO |
| CharCodeAtFuera | IDENTICO |
| JsonErrores | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  


## Tanda 2026-10-27 16:37:07

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote27.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-24 18:27:11) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-23 12:02:19) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 25795d08 | 1611 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 6e8d156c | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 94260145 | 532 | ok | 0 | 329 | 3 |
| MachineId | MachineId | 7b5c3078 | 452 | ok | 0 | 8 | 3 |
| GuiWinJson | GuiWinJson | f14b60af | 2041 | ok | 0 | 65 | 17 |
| GuiShot | GuiShot | 56ce6117 | 1347 | ok | 0 | 381 | 8 |
| MoveSoloLectura | MoveSoloLectura | e7c0ebd1 | 2822 | ok | 0 | 16 | 27 |
| StrDosCaminos | StrDosCaminos | f38b316e | 3760 | ok | 0 | 11 | 155 |
| Utf8Descodifica | Utf8Descodifica | 71a9c7b6 | 8597 | ok | 0 | 28 | 74 |
| ArrayEnorme | ArrayEnorme | 69f48e33 | 1992 | ok | 0 | 9 | 11 |
| TcpCloseSuelto | TcpCloseSuelto | a0e69546 | 992 | ok | 0 | 10 | 3 |
| ParseNumero | ParseNumero | 8b7bac2b | 3868 | ok | 0 | 12 | 52 |
| CharCodeAtFuera | CharCodeAtFuera | 79d2a84f | 1605 | ok | 0 | 11 | 12 |
| JsonErrores | JsonErrores | d308222b | 3607 | ok | 0 | 22 | 14 |

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Oct 27 2026 16:09:45` imageCrc=`17555d19` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.elf (27-10 16:09)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=64236 fs=139264/516096 B vmHeapBytes=393216
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 2 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 197 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 8 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 329 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 24 | 8 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 65 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 381 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| MoveSoloLectura | **IDENTICO** | 274 | 16 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1082 | 11 | EXITED OK exit=0; host rc=0 |
| Utf8Descodifica | **IDENTICO** | 789 | 28 | EXITED OK exit=0; host rc=0 |
| ArrayEnorme | **IDENTICO** | 97 | 9 | EXITED OK exit=0; host rc=0 |
| TcpCloseSuelto | **IDENTICO** | 28 | 10 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 496 | 12 | EXITED OK exit=0; host rc=0 |
| CharCodeAtFuera | **IDENTICO** | 121 | 11 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 172 | 22 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=65145 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=65702 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=66276 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=66895 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MoveSoloLectura: uptimeMs=67774 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras StrDosCaminos: uptimeMs=69493 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras Utf8Descodifica: uptimeMs=71868 fsUsed=155648 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayEnorme: uptimeMs=72620 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras TcpCloseSuelto: uptimeMs=73301 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ParseNumero: uptimeMs=74631 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CharCodeAtFuera: uptimeMs=75324 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras JsonErrores: uptimeMs=76232 fsUsed=147456 vmHeapBytes=393216 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 12 de 14 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 25795d08 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 6e8d156c | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 94260145 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 7b5c3078 | ok |
| /app/MoveSoloLectura.mod | bpgenvm-c\build\tanda\MoveSoloLectura\MoveSoloLectura.mod | 2822 | e7c0ebd1 | ok |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | f38b316e | ok |
| /app/Utf8Descodifica.mod | bpgenvm-c\build\tanda\Utf8Descodifica\Utf8Descodifica.mod | 8597 | 71a9c7b6 | ok (por trozos) |
| /app/ArrayEnorme.mod | bpgenvm-c\build\tanda\ArrayEnorme\ArrayEnorme.mod | 1992 | 69f48e33 | ok |
| /app/TcpCloseSuelto.mod | bpgenvm-c\build\tanda\TcpCloseSuelto\TcpCloseSuelto.mod | 992 | a0e69546 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 8b7bac2b | ok |
| /app/CharCodeAtFuera.mod | bpgenvm-c\build\tanda\CharCodeAtFuera\CharCodeAtFuera.mod | 1605 | 79d2a84f | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | d308222b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Net: en la zona de packs (pack:Stdlib/Net.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | stm32u575@COM5 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |
| MoveSoloLectura | IDENTICO |
| StrDosCaminos | IDENTICO |
| Utf8Descodifica | IDENTICO |
| ArrayEnorme | IDENTICO |
| TcpCloseSuelto | IDENTICO |
| ParseNumero | IDENTICO |
| CharCodeAtFuera | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-27 16:39:30

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote27.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-24 18:27:11) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-23 12:02:19) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c5@COM6)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 25795d08 | 1611 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 6e8d156c | 295 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 94260145 | 532 | ok | 0 | 335 | 3 |
| MachineId | MachineId | 7b5c3078 | 452 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | f14b60af | 2041 | ok | 0 | 72 | 17 |
| GuiShot | GuiShot | 56ce6117 | 1347 | ok | 0 | 390 | 8 |
| MoveSoloLectura | MoveSoloLectura | e7c0ebd1 | 2822 | ok | 0 | 15 | 27 |
| StrDosCaminos | StrDosCaminos | f38b316e | 3760 | ok | 0 | 10 | 155 |
| Utf8Descodifica | Utf8Descodifica | 71a9c7b6 | 8597 | ok | 0 | 29 | 74 |
| ArrayEnorme | ArrayEnorme | 69f48e33 | 1992 | ok | 0 | 12 | 11 |
| TcpCloseSuelto | TcpCloseSuelto | a0e69546 | 992 | ok | 0 | 9 | 3 |
| ParseNumero | ParseNumero | 8b7bac2b | 3868 | ok | 0 | 10 | 52 |
| CharCodeAtFuera | CharCodeAtFuera | 79d2a84f | 1605 | ok | 0 | 9 | 12 |
| JsonErrores | JsonErrores | d308222b | 3607 | ok | 0 | 20 | 14 |

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Oct 27 2026 16:11:53` imageCrc=`bfcbd723` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c5/build/bpvm_esp32c5.bin (27-10 16:12)
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=software uptimeMs=21070 fs=53248/16777216 B vmHeapBytes=6812672
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 14 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 13 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 317 | 335 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 12 | 10 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 862 | 59 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 602 | 384 | EXITED OK exit=0 |
| MoveSoloLectura | **IDENTICO** | 150 | 15 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 62 | 10 | EXITED OK exit=0; host rc=0 |
| Utf8Descodifica | **IDENTICO** | 238 | 29 | EXITED OK exit=0; host rc=0 |
| ArrayEnorme | **IDENTICO** | 20 | 12 | EXITED OK exit=0; host rc=0 |
| TcpCloseSuelto | **IDENTICO** | 14 | 9 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 47 | 10 | EXITED OK exit=0; host rc=0 |
| CharCodeAtFuera | **IDENTICO** | 22 | 9 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 210 | 20 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=21846 fsUsed=57344 vmHeapBytes=6812672 pila libre: run=12488 io=9908 tareaREPL=9908 heap del sistema, minimo libre=172864
- tras ThrowSinAtrapar: uptimeMs=22378 fsUsed=57344 vmHeapBytes=6812672 pila libre: run=12488 io=9908 tareaREPL=9908 heap del sistema, minimo libre=172864
- tras ThreadTrasMain: uptimeMs=23262 fsUsed=57344 vmHeapBytes=6812672 pila libre: run=12488 io=9908 tareaREPL=9908 heap del sistema, minimo libre=172864
- tras MachineId: uptimeMs=23780 fsUsed=57344 vmHeapBytes=6812672 pila libre: run=12488 io=9908 tareaREPL=9908 heap del sistema, minimo libre=172864
- tras GuiWinJson: uptimeMs=26533 fsUsed=61440 vmHeapBytes=6812672 pila libre: run=12376 io=9728 tareaREPL=9728 heap del sistema, minimo libre=160116
- tras GuiShot: uptimeMs=28574 fsUsed=61440 vmHeapBytes=6812672 pila libre: run=11428 io=9728 tareaREPL=9728 heap del sistema, minimo libre=155160
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.shot` (3350 B, crc f0589b1a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png`; antes del RUN: no existia
- tras MoveSoloLectura: uptimeMs=29373 fsUsed=61440 vmHeapBytes=6812672 pila libre: run=11428 io=9728 tareaREPL=9728 heap del sistema, minimo libre=155160
- tras StrDosCaminos: uptimeMs=29784 fsUsed=61440 vmHeapBytes=6812672 pila libre: run=11428 io=9728 tareaREPL=9728 heap del sistema, minimo libre=155160
- tras Utf8Descodifica: uptimeMs=30585 fsUsed=69632 vmHeapBytes=6812672 pila libre: run=11428 io=9728 tareaREPL=9728 heap del sistema, minimo libre=155160
- tras ArrayEnorme: uptimeMs=31069 fsUsed=61440 vmHeapBytes=6812672 pila libre: run=11428 io=9728 tareaREPL=9728 heap del sistema, minimo libre=155160
- tras TcpCloseSuelto: uptimeMs=31601 fsUsed=61440 vmHeapBytes=6812672 pila libre: run=11428 io=9728 tareaREPL=9728 heap del sistema, minimo libre=155160
- tras ParseNumero: uptimeMs=32093 fsUsed=61440 vmHeapBytes=6812672 pila libre: run=11428 io=9728 tareaREPL=9728 heap del sistema, minimo libre=155160
- tras CharCodeAtFuera: uptimeMs=32562 fsUsed=61440 vmHeapBytes=6812672 pila libre: run=11428 io=9728 tareaREPL=9728 heap del sistema, minimo libre=155160
- tras JsonErrores: uptimeMs=33456 fsUsed=61440 vmHeapBytes=6812672 pila libre: run=11428 io=9728 tareaREPL=9728 heap del sistema, minimo libre=155160
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 25795d08 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 6e8d156c | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 94260145 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 7b5c3078 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | f14b60af | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 56ce6117 | ok |
| /app/MoveSoloLectura.mod | bpgenvm-c\build\tanda\MoveSoloLectura\MoveSoloLectura.mod | 2822 | e7c0ebd1 | ok |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | f38b316e | ok |
| /app/Utf8Descodifica.mod | bpgenvm-c\build\tanda\Utf8Descodifica\Utf8Descodifica.mod | 8597 | 71a9c7b6 | ok (por trozos) |
| /app/ArrayEnorme.mod | bpgenvm-c\build\tanda\ArrayEnorme\ArrayEnorme.mod | 1992 | 69f48e33 | ok |
| /app/TcpCloseSuelto.mod | bpgenvm-c\build\tanda\TcpCloseSuelto\TcpCloseSuelto.mod | 992 | a0e69546 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 8b7bac2b | ok |
| /app/CharCodeAtFuera.mod | bpgenvm-c\build\tanda\CharCodeAtFuera\CharCodeAtFuera.mod | 1605 | 79d2a84f | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | d308222b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Net: en la zona de packs (pack:Stdlib/Net.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | esp32c5@COM6 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |
| MoveSoloLectura | IDENTICO |
| StrDosCaminos | IDENTICO |
| Utf8Descodifica | IDENTICO |
| ArrayEnorme | IDENTICO |
| TcpCloseSuelto | IDENTICO |
| ParseNumero | IDENTICO |
| CharCodeAtFuera | IDENTICO |
| JsonErrores | IDENTICO |

NECESITA OJOS: GuiShot en esp32c5@COM6: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png  

