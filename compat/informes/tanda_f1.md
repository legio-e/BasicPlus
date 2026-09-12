# Tandas T1 — las placas conducidas

## Tanda 2026-09-12 17:38:04

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 322 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 11 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 33 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 340 | 8 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Sep 12 2026 12:05:08` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=45218 fs=245760/1048576 B vmHeapBytes=270408
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (supuesto por boardName 'rp2350a')
- inventario /lib+/app: 26 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 26 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 5 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 304 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 11 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 33 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a') |
| GuiShot | **SALTADA** | - | 340 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a') |

- tras MathRango: uptimeMs=47065 fsUsed=245760 vmHeapBytes=270408
- tras ThrowSinAtrapar: uptimeMs=48251 fsUsed=245760 vmHeapBytes=270408
- tras ThreadTrasMain: uptimeMs=49400 fsUsed=245760 vmHeapBytes=270408
- tras MachineId: uptimeMs=50749 fsUsed=245760 vmHeapBytes=270408
- ejecutadas (con EXITED) 4 de 6 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | rp2350a@COM22 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a')  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a')  


## Tanda 2026-09-12 17:38:44

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (rp2350b@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 322 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 35 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 352 | 8 |

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Sep 12 2026 12:05:08` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=11206 fs=327680/10248192 B vmHeapBytes=271926
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (supuesto por boardName 'rp2350b')
- inventario /lib+/app: 30 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 27 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 4 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 303 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 10 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 35 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b') |
| GuiShot | **SALTADA** | - | 352 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b') |

- tras MathRango: uptimeMs=13038 fsUsed=327680 vmHeapBytes=271926
- tras ThrowSinAtrapar: uptimeMs=14231 fsUsed=327680 vmHeapBytes=271926
- tras ThreadTrasMain: uptimeMs=15375 fsUsed=327680 vmHeapBytes=271926
- tras MachineId: uptimeMs=16932 fsUsed=327680 vmHeapBytes=271926
- ejecutadas (con EXITED) 4 de 6 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |

notas de dependencias:

- dependencia Core: ya en /app/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | rp2350b@COM4 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b')  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b')  


## Tanda 2026-09-12 17:39:25

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u5@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 312 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 37 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 345 | 8 |

### Placa stm32u5 - 20353850553250040015002D

- sello: boardName=`stm32u5` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 12 2026 17:33:07` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=10222 fs=647168/2064384 B vmHeapBytes=1179648
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 49 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 226 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 80 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 340 | 312 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 93 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 1026 | 35 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 1225 | 338 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=12661 fsUsed=647168 vmHeapBytes=1179648
- tras ThrowSinAtrapar: uptimeMs=13857 fsUsed=647168 vmHeapBytes=1179648
- tras ThreadTrasMain: uptimeMs=15178 fsUsed=647168 vmHeapBytes=1179648
- tras MachineId: uptimeMs=16498 fsUsed=647168 vmHeapBytes=1179648
- tras GuiWinJson: uptimeMs=20047 fsUsed=647168 vmHeapBytes=1179648
- tras GuiShot: uptimeMs=24006 fsUsed=647168 vmHeapBytes=1179648
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5_203538505532\GuiShot.shot` (7292 B, crc bd19c7be) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- ejecutadas (con EXITED) 6 de 6 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 3627 | c2e655fa | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 2545 | 61c07fa6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Gui: ya en /app/Gui.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Json: ya en /app/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /app/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /app/Str.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | stm32u5@COM12 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en stm32u5@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5_203538505532\GuiShot.png  

