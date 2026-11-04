# Tandas T1 — las placas conducidas

## Tanda 2026-11-04 06:35:10

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\placas\tanda_lote3nov.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-03 11:40:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-03 19:41:49) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | e1a4497c | 1611 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 54b30c7f | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | b2ff7b99 | 532 | ok | 0 | 329 | 3 |
| MachineId | MachineId | 75a34c76 | 452 | ok | 0 | 16 | 3 |
| GuiWinJson | GuiWinJson | 5a615738 | 2041 | ok | 0 | 78 | 17 |
| GuiShot | GuiShot | 001f8834 | 1347 | ok | 0 | 389 | 8 |
| StrNulo | StrNulo | adfe9a69 | 2784 | ok | 0 | 11 | 14 |
| SleepCede | SleepCede | 64efa84f | 656 | ok | 0 | 17 | 2 |
| CedeCero | CedeCero | 11d1d1c5 | 1157 | ok | 0 | 14 | 1 |
| DatosGrandes | DatosGrandes | cf48d73e | 34291 | ok | 0 | 10 | 11 |
| FileCursor | FileCursor | 79bb5071 | 4208 | ok | 0 | 98 | 33 |
| StrCursor | StrCursor | a201d639 | 11397 | ok | 0 | 48 | 12 |
| StrDosCaminos | StrDosCaminos | 7b73e622 | 3760 | ok | 0 | 11 | 155 |
| ParseNumero | ParseNumero | 4ef59c61 | 3868 | ok | 0 | 10 | 52 |
| JsonErrores | JsonErrores | c0054bd7 | 3607 | ok | 0 | 22 | 14 |

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Nov  4 2026 06:31:14` imageCrc=`973b1d93` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.elf (04-11 06:31)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=242501 fs=212992/516096 B vmHeapBytes=507904
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 10 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 199 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 7 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 329 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 23 | 16 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 78 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 389 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| StrNulo | **IDENTICO** | 113 | 11 | EXITED OK exit=0; host rc=0 |
| SleepCede | **IDENTICO** | 37 | 17 | EXITED OK exit=0; host rc=0 |
| CedeCero | **IDENTICO** | 410 | 14 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 70 | 10 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 278 | 98 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 4511 | 48 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1094 | 11 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 496 | 10 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 170 | 22 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=243589 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=244164 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=244769 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=245414 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras StrNulo: uptimeMs=246251 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SleepCede: uptimeMs=246909 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras CedeCero: uptimeMs=247973 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras DatosGrandes: uptimeMs=251895 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras FileCursor: uptimeMs=253276 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras StrCursor: uptimeMs=259457 fsUsed=229376 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras StrDosCaminos: uptimeMs=261184 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ParseNumero: uptimeMs=262504 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras JsonErrores: uptimeMs=263520 fsUsed=221184 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
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

