# Tandas T1 — las placas conducidas

## Tanda 2026-11-03 09:55:29

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 09:10:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 09:10:26) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 15 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 15 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 326 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 16 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 59 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 382 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 11 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 7 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 0 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 13 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 34 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 57 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 8 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 5 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 21 | 14 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Nov  3 2026 09:45:35` imageCrc=`57b6b9c8` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (03-11 09:45)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=39221 fs=212992/1048576 B vmHeapBytes=220616
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 37 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 22 | 15 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 153 | 15 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 309 | 326 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 8 | 16 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 59 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 382 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| StrNulo | **IDENTICO** | 25 | 11 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 31 | 7 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 884 | 0 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 21 | 13 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 256 | 34 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 6008 | 57 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 240 | 8 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 113 | 5 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 254 | 21 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=40154 fsUsed=217088 vmHeapBytes=220616 pila libre: run=13560 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=40752 fsUsed=217088 vmHeapBytes=220616 pila libre: run=14336 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=41694 fsUsed=217088 vmHeapBytes=220616 pila libre: run=14336 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=42257 fsUsed=217088 vmHeapBytes=220616 pila libre: run=13272 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras StrNulo: uptimeMs=42798 fsUsed=217088 vmHeapBytes=220616 pila libre: run=12776 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras SleepCede: uptimeMs=43463 fsUsed=217088 vmHeapBytes=220616 pila libre: run=12776 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras CedeCero: uptimeMs=44796 fsUsed=217088 vmHeapBytes=220616 pila libre: run=12480 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras DatosGrandes: uptimeMs=45995 fsUsed=249856 vmHeapBytes=220616 pila libre: run=12776 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras FileCursor: uptimeMs=47168 fsUsed=221184 vmHeapBytes=220616 pila libre: run=12776 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras StrCursor: uptimeMs=54101 fsUsed=225280 vmHeapBytes=220616 pila libre: run=12776 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras StrDosCaminos: uptimeMs=55010 fsUsed=217088 vmHeapBytes=220616 pila libre: run=12192 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ParseNumero: uptimeMs=55570 fsUsed=217088 vmHeapBytes=220616 pila libre: run=12488 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras JsonErrores: uptimeMs=56796 fsUsed=217088 vmHeapBytes=220616 pila libre: run=12192 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- ejecutadas (con EXITED) 13 de 15 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | e1a4497c | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 54b30c7f | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | b2ff7b99 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 75a34c76 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | adfe9a69 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 64efa84f | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 11d1d1c5 | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | cf48d73e | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 79bb5071 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | a201d639 | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 7b73e622 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 4ef59c61 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | c0054bd7 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
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
| StrNulo | IDENTICO |
| SleepCede | IDENTICO |
| CedeCero | IDENTICO |
| DatosGrandes | IDENTICO |
| FileCursor | IDENTICO |
| StrCursor | IDENTICO |
| StrDosCaminos | IDENTICO |
| ParseNumero | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-11-03 09:58:33

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 09:10:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 09:10:26) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350b@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 0 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 332 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 15 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 67 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 384 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 14 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 14 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 13 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 8 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 69 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 54 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 12 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 0 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 24 | 14 |

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Nov  3 2026 09:45:38` imageCrc=`e0aa82a7` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico_m.elf (03-11 09:45)
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=19471 fs=249856/10248192 B vmHeapBytes=6275072
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 49 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 22 | 0 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 145 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 316 | 332 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 15 | 15 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 67 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 384 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| StrNulo | **IDENTICO** | 21 | 14 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 128 | 14 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 464 | 13 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 23 | 8 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 102 | 69 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 5369 | 54 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 74 | 12 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 48 | 0 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 168 | 24 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=20356 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=13592 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=20980 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=12880 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=21933 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=13440 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=22501 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=12880 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras StrNulo: uptimeMs=23135 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=13040 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras SleepCede: uptimeMs=23721 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=13040 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras CedeCero: uptimeMs=24644 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=13040 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras DatosGrandes: uptimeMs=25934 fsUsed=286720 vmHeapBytes=6275072 pila libre: run=12744 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras FileCursor: uptimeMs=26944 fsUsed=258048 vmHeapBytes=6275072 pila libre: run=12792 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras StrCursor: uptimeMs=33029 fsUsed=262144 vmHeapBytes=6275072 pila libre: run=12792 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras StrDosCaminos: uptimeMs=33681 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=12744 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ParseNumero: uptimeMs=34198 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=12792 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras JsonErrores: uptimeMs=35328 fsUsed=253952 vmHeapBytes=6275072 pila libre: run=12744 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- ejecutadas (con EXITED) 13 de 15 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | e1a4497c | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 54b30c7f | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | b2ff7b99 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 75a34c76 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | adfe9a69 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 64efa84f | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 11d1d1c5 | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | cf48d73e | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 79bb5071 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | a201d639 | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 7b73e622 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 4ef59c61 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | c0054bd7 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
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
| StrNulo | IDENTICO |
| SleepCede | IDENTICO |
| CedeCero | IDENTICO |
| DatosGrandes | IDENTICO |
| FileCursor | IDENTICO |
| StrCursor | IDENTICO |
| StrDosCaminos | IDENTICO |
| ParseNumero | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-11-03 10:00:14

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 09:10:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 09:10:26) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c3@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 15 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 327 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 66 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 396 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 11 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 1 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 10 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 11 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 52 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 49 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 14 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 6 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 22 | 14 |

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Nov  3 2026 09:47:14` imageCrc=`fc3c581d` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c3/build/bpvm_esp32c3.bin (03-11 09:47)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=USB uptimeMs=55628 fs=36864/1523712 B vmHeapBytes=98304
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 1 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 8 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 5 | 15 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 312 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 66 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 396 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| StrNulo | **IDENTICO** | 11 | 11 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 33 | 1 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 952 | 10 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 12 | 11 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 245 | 52 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 11571 | 49 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 129 | 14 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 78 | 6 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 357 | 22 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=56222 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8504 io=9940 tareaREPL=9940 heap del sistema, minimo libre=97604
- tras ThrowSinAtrapar: uptimeMs=56716 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8504 io=9940 tareaREPL=9940 heap del sistema, minimo libre=97604
- tras ThreadTrasMain: uptimeMs=57165 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8504 io=9940 tareaREPL=9940 heap del sistema, minimo libre=97604
- tras MachineId: uptimeMs=57648 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8504 io=9940 tareaREPL=9940 heap del sistema, minimo libre=97604
- tras StrNulo: uptimeMs=58115 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8504 io=9940 tareaREPL=9940 heap del sistema, minimo libre=97604
- tras SleepCede: uptimeMs=58615 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8504 io=9940 tareaREPL=9940 heap del sistema, minimo libre=96056
- tras CedeCero: uptimeMs=59900 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8504 io=9940 tareaREPL=9940 heap del sistema, minimo libre=96056
- tras DatosGrandes: uptimeMs=60601 fsUsed=73728 vmHeapBytes=98304 pila libre: run=8504 io=9940 tareaREPL=9940 heap del sistema, minimo libre=96056
- tras FileCursor: uptimeMs=61554 fsUsed=45056 vmHeapBytes=98304 pila libre: run=8312 io=9940 tareaREPL=9940 heap del sistema, minimo libre=94920
- tras StrCursor: uptimeMs=73731 fsUsed=49152 vmHeapBytes=98304 pila libre: run=8264 io=9940 tareaREPL=9940 heap del sistema, minimo libre=94920
- tras StrDosCaminos: uptimeMs=74215 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8264 io=9940 tareaREPL=9940 heap del sistema, minimo libre=94920
- tras ParseNumero: uptimeMs=74648 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8264 io=9940 tareaREPL=9940 heap del sistema, minimo libre=94920
- tras JsonErrores: uptimeMs=75774 fsUsed=40960 vmHeapBytes=98304 pila libre: run=8264 io=9940 tareaREPL=9940 heap del sistema, minimo libre=86468
- ejecutadas (con EXITED) 13 de 15 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | e1a4497c | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 54b30c7f | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | b2ff7b99 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 75a34c76 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | adfe9a69 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 64efa84f | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 11d1d1c5 | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | cf48d73e | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 79bb5071 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | a201d639 | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 7b73e622 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 4ef59c61 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | c0054bd7 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
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
| StrNulo | IDENTICO |
| SleepCede | IDENTICO |
| CedeCero | IDENTICO |
| DatosGrandes | IDENTICO |
| FileCursor | IDENTICO |
| StrCursor | IDENTICO |
| StrDosCaminos | IDENTICO |
| ParseNumero | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-11-03 10:02:41

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 09:10:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 09:10:26) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 0 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 313 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 17 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 59 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 380 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 14 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 15 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 12 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 9 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 47 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 49 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 16 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 9 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 14 | 14 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Nov  3 2026 09:49:01` imageCrc=`6889de20` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (03-11 09:49)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=66807 fs=266240/16777216 B vmHeapBytes=25145344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 40 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 192 | 0 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 9 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 313 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 19 | 17 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 725 | 60 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 622 | 383 | EXITED OK exit=0 |
| StrNulo | **IDENTICO** | 108 | 14 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 164 | 15 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 340 | 12 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 66 | 9 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 276 | 47 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 4179 | 49 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1081 | 16 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 487 | 9 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 136 | 14 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=68496 fsUsed=270336 vmHeapBytes=25145344 pila libre: run=12516 io=25796 tareaREPL=25796 heap del sistema, minimo libre=239444
- tras ThrowSinAtrapar: uptimeMs=69216 fsUsed=270336 vmHeapBytes=25145344 pila libre: run=12516 io=25796 tareaREPL=25796 heap del sistema, minimo libre=239444
- tras ThreadTrasMain: uptimeMs=70562 fsUsed=270336 vmHeapBytes=25145344 pila libre: run=12516 io=25796 tareaREPL=25796 heap del sistema, minimo libre=239444
- tras MachineId: uptimeMs=71360 fsUsed=270336 vmHeapBytes=25145344 pila libre: run=12516 io=25796 tareaREPL=25796 heap del sistema, minimo libre=239444
- tras GuiWinJson: uptimeMs=74731 fsUsed=274432 vmHeapBytes=25145344 pila libre: run=12500 io=25528 tareaREPL=25528 heap del sistema, minimo libre=232096
- tras GuiShot: uptimeMs=77682 fsUsed=282624 vmHeapBytes=25145344 pila libre: run=11360 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- tras StrNulo: uptimeMs=79767 fsUsed=282624 vmHeapBytes=25145344 pila libre: run=11360 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- tras SleepCede: uptimeMs=80935 fsUsed=282624 vmHeapBytes=25145344 pila libre: run=11360 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- tras CedeCero: uptimeMs=82052 fsUsed=282624 vmHeapBytes=25145344 pila libre: run=13336 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- tras DatosGrandes: uptimeMs=86366 fsUsed=315392 vmHeapBytes=25145344 pila libre: run=11360 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- tras FileCursor: uptimeMs=87892 fsUsed=286720 vmHeapBytes=25145344 pila libre: run=11360 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- tras StrCursor: uptimeMs=94447 fsUsed=290816 vmHeapBytes=25145344 pila libre: run=11360 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- tras StrDosCaminos: uptimeMs=96817 fsUsed=282624 vmHeapBytes=25145344 pila libre: run=11360 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- tras ParseNumero: uptimeMs=98650 fsUsed=282624 vmHeapBytes=25145344 pila libre: run=11360 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- tras JsonErrores: uptimeMs=100010 fsUsed=282624 vmHeapBytes=25145344 pila libre: run=11360 io=25528 tareaREPL=25528 heap del sistema, minimo libre=231880
- ejecutadas (con EXITED) 15 de 15 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | e1a4497c | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 54b30c7f | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | b2ff7b99 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 75a34c76 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | 5a615738 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 001f8834 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | adfe9a69 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 64efa84f | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 11d1d1c5 | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | cf48d73e | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 79bb5071 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | a201d639 | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 7b73e622 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 4ef59c61 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | c0054bd7 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
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
| StrNulo | IDENTICO |
| SleepCede | IDENTICO |
| CedeCero | IDENTICO |
| DatosGrandes | IDENTICO |
| FileCursor | IDENTICO |
| StrCursor | IDENTICO |
| StrDosCaminos | IDENTICO |
| ParseNumero | IDENTICO |
| JsonErrores | IDENTICO |

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  


## Tanda 2026-11-03 10:06:09

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 09:10:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 09:10:26) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 330 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 69 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 392 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 13 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 13 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 10 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 12 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 51 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 40 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 10 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 9 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 22 | 14 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Nov  3 2026 09:50:04` imageCrc=`ef452299` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (03-11 09:50)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=60117 fs=114688/1032192 B vmHeapBytes=1032192
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 199 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 313 | 330 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 25 | 10 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 580 | 61 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 685 | 386 | EXITED OK exit=0 |
| StrNulo | **IDENTICO** | 114 | 13 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 29 | 13 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 493 | 10 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 73 | 12 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 290 | 51 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 5790 | 40 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1097 | 10 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 500 | 9 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 226 | 22 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=61071 fsUsed=122880 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=61634 fsUsed=122880 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=62219 fsUsed=122880 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=62834 fsUsed=122880 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiWinJson: uptimeMs=64976 fsUsed=131072 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiShot: uptimeMs=67219 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: no existia
- tras StrNulo: uptimeMs=68906 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SleepCede: uptimeMs=69569 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CedeCero: uptimeMs=70619 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3240 tareaREPL=None heap del sistema, minimo libre=None
- tras DatosGrandes: uptimeMs=74521 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FileCursor: uptimeMs=75889 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras StrCursor: uptimeMs=83326 fsUsed=147456 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras StrDosCaminos: uptimeMs=85046 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ParseNumero: uptimeMs=86368 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras JsonErrores: uptimeMs=87278 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 15 de 15 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | e1a4497c | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 54b30c7f | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | b2ff7b99 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 75a34c76 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | 5a615738 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 001f8834 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | adfe9a69 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 64efa84f | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 11d1d1c5 | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | cf48d73e | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 79bb5071 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | a201d639 | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 7b73e622 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 4ef59c61 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | c0054bd7 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
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
| StrNulo | IDENTICO |
| SleepCede | IDENTICO |
| CedeCero | IDENTICO |
| DatosGrandes | IDENTICO |
| FileCursor | IDENTICO |
| StrCursor | IDENTICO |
| StrDosCaminos | IDENTICO |
| ParseNumero | IDENTICO |
| JsonErrores | IDENTICO |

NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  


## Tanda 2026-11-03 10:09:46

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 09:10:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 09:10:26) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32s3@COM9)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 38 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 330 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 11 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 57 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 389 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 12 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 26 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 18 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 17 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 51 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 33 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 18 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 11 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 25 | 14 |

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Nov  3 2026 09:46:30` imageCrc=`cebeccdb` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (03-11 09:46)
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=60276 fs=69632/10248192 B vmHeapBytes=7844864
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 11 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 194 | 38 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 12 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 319 | 330 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 20 | 11 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 57 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 389 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| StrNulo | **IDENTICO** | 109 | 12 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 48 | 26 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 760 | 18 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 67 | 17 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 284 | 51 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 13938 | 33 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1083 | 18 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 486 | 11 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 400 | 25 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=61495 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=12028 io=9380 tareaREPL=9380 heap del sistema, minimo libre=210972
- tras ThrowSinAtrapar: uptimeMs=62077 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=14060 io=9380 tareaREPL=9380 heap del sistema, minimo libre=210972
- tras ThreadTrasMain: uptimeMs=63263 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=13300 io=9380 tareaREPL=9380 heap del sistema, minimo libre=210972
- tras MachineId: uptimeMs=64030 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=13284 io=9380 tareaREPL=9380 heap del sistema, minimo libre=210972
- tras StrNulo: uptimeMs=65247 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=12356 io=9380 tareaREPL=9380 heap del sistema, minimo libre=210896
- tras SleepCede: uptimeMs=66125 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=12180 io=9380 tareaREPL=9380 heap del sistema, minimo libre=208304
- tras CedeCero: uptimeMs=67800 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=12180 io=9380 tareaREPL=9380 heap del sistema, minimo libre=208304
- tras DatosGrandes: uptimeMs=71854 fsUsed=106496 vmHeapBytes=7844864 pila libre: run=12180 io=9380 tareaREPL=9380 heap del sistema, minimo libre=208304
- tras FileCursor: uptimeMs=73989 fsUsed=77824 vmHeapBytes=7844864 pila libre: run=11988 io=9380 tareaREPL=9380 heap del sistema, minimo libre=207164
- tras StrCursor: uptimeMs=90114 fsUsed=81920 vmHeapBytes=7844864 pila libre: run=12028 io=9348 tareaREPL=9348 heap del sistema, minimo libre=207164
- tras StrDosCaminos: uptimeMs=92329 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=11988 io=9348 tareaREPL=9348 heap del sistema, minimo libre=207164
- tras ParseNumero: uptimeMs=93776 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=11988 io=9348 tareaREPL=9348 heap del sistema, minimo libre=207164
- tras JsonErrores: uptimeMs=95673 fsUsed=73728 vmHeapBytes=7844864 pila libre: run=11988 io=9348 tareaREPL=9348 heap del sistema, minimo libre=207164
- ejecutadas (con EXITED) 13 de 15 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | e1a4497c | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 54b30c7f | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | b2ff7b99 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 75a34c76 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | adfe9a69 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 64efa84f | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 11d1d1c5 | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | cf48d73e | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 79bb5071 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | a201d639 | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 7b73e622 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 4ef59c61 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | c0054bd7 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
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
| StrNulo | IDENTICO |
| SleepCede | IDENTICO |
| CedeCero | IDENTICO |
| DatosGrandes | IDENTICO |
| FileCursor | IDENTICO |
| StrCursor | IDENTICO |
| StrDosCaminos | IDENTICO |
| ParseNumero | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-11-03 10:12:06

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 09:10:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 09:10:26) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c5@COM6)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 7 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 329 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 11 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 60 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 401 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 18 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 15 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 23 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 7 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 32 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 55 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 14 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 14 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 24 | 14 |

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Nov  3 2026 09:47:48` imageCrc=`f1ebc468` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c5/build/bpvm_esp32c5.bin (03-11 09:48)
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=software uptimeMs=39102 fs=53248/16777216 B vmHeapBytes=7320576
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 14 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 13 | 7 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 314 | 329 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 13 | 11 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 840 | 63 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 626 | 377 | EXITED OK exit=0 |
| StrNulo | **IDENTICO** | 18 | 18 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 87 | 15 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 561 | 23 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 19 | 7 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 147 | 32 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 7215 | 55 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 67 | 14 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 49 | 14 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 213 | 24 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=39890 fsUsed=57344 vmHeapBytes=7320576 pila libre: run=12600 io=9908 tareaREPL=9908 heap del sistema, minimo libre=175152
- tras ThrowSinAtrapar: uptimeMs=40370 fsUsed=57344 vmHeapBytes=7320576 pila libre: run=12600 io=9908 tareaREPL=9908 heap del sistema, minimo libre=175152
- tras ThreadTrasMain: uptimeMs=41254 fsUsed=57344 vmHeapBytes=7320576 pila libre: run=12600 io=9908 tareaREPL=9908 heap del sistema, minimo libre=175152
- tras MachineId: uptimeMs=41807 fsUsed=57344 vmHeapBytes=7320576 pila libre: run=12600 io=9908 tareaREPL=9908 heap del sistema, minimo libre=175152
- tras GuiWinJson: uptimeMs=44238 fsUsed=61440 vmHeapBytes=7320576 pila libre: run=12280 io=9664 tareaREPL=9664 heap del sistema, minimo libre=162600
- tras GuiShot: uptimeMs=46259 fsUsed=61440 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.shot` (3350 B, crc f0589b1a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png`; antes del RUN: no existia
- tras StrNulo: uptimeMs=46859 fsUsed=61440 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- tras SleepCede: uptimeMs=47472 fsUsed=61440 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- tras CedeCero: uptimeMs=48344 fsUsed=61440 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- tras DatosGrandes: uptimeMs=49110 fsUsed=94208 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- tras FileCursor: uptimeMs=50029 fsUsed=65536 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- tras StrCursor: uptimeMs=58033 fsUsed=69632 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- tras StrDosCaminos: uptimeMs=58558 fsUsed=61440 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- tras ParseNumero: uptimeMs=59087 fsUsed=61440 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- tras JsonErrores: uptimeMs=60021 fsUsed=61440 vmHeapBytes=7320576 pila libre: run=11428 io=9664 tareaREPL=9664 heap del sistema, minimo libre=157432
- ejecutadas (con EXITED) 15 de 15 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | e1a4497c | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 54b30c7f | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | b2ff7b99 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 75a34c76 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | 5a615738 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 001f8834 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | adfe9a69 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 64efa84f | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 11d1d1c5 | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | cf48d73e | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 79bb5071 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | a201d639 | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 7b73e622 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 4ef59c61 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | c0054bd7 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
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
| StrNulo | IDENTICO |
| SleepCede | IDENTICO |
| CedeCero | IDENTICO |
| DatosGrandes | IDENTICO |
| FileCursor | IDENTICO |
| StrCursor | IDENTICO |
| StrDosCaminos | IDENTICO |
| ParseNumero | IDENTICO |
| JsonErrores | IDENTICO |

NECESITA OJOS: GuiShot en esp32c5@COM6: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png  


## Tanda 2026-11-03 10:14:07

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 09:10:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 09:10:26) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 342 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 12 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 58 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 387 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 0 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 14 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 14 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 11 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 28 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 53 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 6 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 6 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 37 | 14 |

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Nov  3 2026 09:49:47` imageCrc=`ffee40be` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.elf (03-11 09:49)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=433729 fs=139264/516096 B vmHeapBytes=507904
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 2 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 197 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 8 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 312 | 342 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 24 | 12 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 58 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 387 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| StrNulo | **IDENTICO** | 113 | 0 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 29 | 14 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 419 | 14 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 71 | 11 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 290 | 28 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 4742 | 53 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1097 | 6 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 497 | 6 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 176 | 37 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=434661 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=435226 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=435838 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=436462 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras StrNulo: uptimeMs=437271 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SleepCede: uptimeMs=437944 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CedeCero: uptimeMs=439023 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DatosGrandes: uptimeMs=442948 fsUsed=180224 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FileCursor: uptimeMs=444336 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras StrCursor: uptimeMs=451054 fsUsed=155648 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras StrDosCaminos: uptimeMs=452795 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ParseNumero: uptimeMs=454110 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras JsonErrores: uptimeMs=455615 fsUsed=147456 vmHeapBytes=507904 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 13 de 15 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | e1a4497c | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 54b30c7f | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | b2ff7b99 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 75a34c76 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | adfe9a69 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 64efa84f | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 11d1d1c5 | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | cf48d73e | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 79bb5071 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | a201d639 | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 7b73e622 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 4ef59c61 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | c0054bd7 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
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
| StrNulo | IDENTICO |
| SleepCede | IDENTICO |
| CedeCero | IDENTICO |
| DatosGrandes | IDENTICO |
| FileCursor | IDENTICO |
| StrCursor | IDENTICO |
| StrDosCaminos | IDENTICO |
| ParseNumero | IDENTICO |
| JsonErrores | IDENTICO |

SALTADA: GuiWinJson en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-11-03 10:16:10

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 09:10:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 09:10:26) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 16 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 6 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 330 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 13 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 60 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 380 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 14 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 22 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 13 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 13 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 46 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 39 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 11 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 9 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 24 | 14 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Nov  3 2026 09:48:23` imageCrc=`3942ac24` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c6/build/bpvm_esp32c6.bin (03-11 09:48)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=USB uptimeMs=496957 fs=36864/1261568 B vmHeapBytes=114688
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 1 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 7 | 16 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 4 | 6 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 313 | 330 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 4 | 13 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 983 | 61 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 569 | 378 | EXITED OK exit=0 |
| StrNulo | **IDENTICO** | 9 | 14 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 28 | 22 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 812 | 13 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 214 | 46 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 10645 | 39 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 73 | 11 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 42 | 9 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 289 | 24 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=497591 fsUsed=40960 vmHeapBytes=114688 pila libre: run=6472 io=5828 tareaREPL=5828 heap del sistema, minimo libre=74844
- tras ThrowSinAtrapar: uptimeMs=498059 fsUsed=40960 vmHeapBytes=114688 pila libre: run=6472 io=5828 tareaREPL=5828 heap del sistema, minimo libre=74844
- tras ThreadTrasMain: uptimeMs=498576 fsUsed=40960 vmHeapBytes=114688 pila libre: run=6472 io=5828 tareaREPL=5828 heap del sistema, minimo libre=74844
- tras MachineId: uptimeMs=499092 fsUsed=40960 vmHeapBytes=114688 pila libre: run=6472 io=5780 tareaREPL=5780 heap del sistema, minimo libre=74844
- tras GuiWinJson: uptimeMs=501728 fsUsed=45056 vmHeapBytes=114688 pila libre: run=6080 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- tras GuiShot: uptimeMs=503746 fsUsed=45056 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: no existia
- tras StrNulo: uptimeMs=504397 fsUsed=45056 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- tras SleepCede: uptimeMs=504931 fsUsed=45056 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- tras CedeCero: uptimeMs=506242 fsUsed=45056 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- tras DatosGrandes: uptimeMs=507067 fsUsed=77824 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- tras FileCursor: uptimeMs=508047 fsUsed=49152 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- tras StrCursor: uptimeMs=519729 fsUsed=53248 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- tras StrDosCaminos: uptimeMs=520227 fsUsed=45056 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- tras ParseNumero: uptimeMs=520650 fsUsed=45056 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- tras JsonErrores: uptimeMs=521563 fsUsed=45056 vmHeapBytes=114688 pila libre: run=5284 io=5584 tareaREPL=5584 heap del sistema, minimo libre=35376
- ejecutadas (con EXITED) 15 de 15 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | e1a4497c | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 54b30c7f | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | b2ff7b99 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 75a34c76 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | 5a615738 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 001f8834 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | adfe9a69 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 64efa84f | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 11d1d1c5 | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | cf48d73e | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 79bb5071 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | a201d639 | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 7b73e622 | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | 4ef59c61 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | c0054bd7 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
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
| StrNulo | IDENTICO |
| SleepCede | IDENTICO |
| CedeCero | IDENTICO |
| DatosGrandes | IDENTICO |
| FileCursor | IDENTICO |
| StrCursor | IDENTICO |
| StrDosCaminos | IDENTICO |
| ParseNumero | IDENTICO |
| JsonErrores | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  

