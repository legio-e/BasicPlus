# Tandas T1 — las placas conducidas

## Tanda 2026-09-24 15:31:55

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 312 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 10 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 48 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 370 | 8 |

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Sep 24 2026 10:07:46` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=20565 fs=221184/516096 B vmHeapBytes=393216
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 16 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 257 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 66 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 378 | 312 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 137 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 356 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 514 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 188 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 312 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 287 | 10 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 48 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 370 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |

- tras MathRango: uptimeMs=22432 fsUsed=229376 vmHeapBytes=393216
- tras ThrowSinAtrapar: uptimeMs=23715 fsUsed=237568 vmHeapBytes=393216
- tras ThreadTrasMain: uptimeMs=24810 fsUsed=245760 vmHeapBytes=393216
- tras MachineId: uptimeMs=26140 fsUsed=253952 vmHeapBytes=393216
- tras Destructor: uptimeMs=28399 fsUsed=262144 vmHeapBytes=393216
- tras TimeDemo: uptimeMs=30206 fsUsed=270336 vmHeapBytes=393216
- tras ArrayFijo: uptimeMs=31468 fsUsed=278528 vmHeapBytes=393216
- tras MoveRango: uptimeMs=32799 fsUsed=286720 vmHeapBytes=393216
- tras HiloUsa: uptimeMs=34553 fsUsed=303104 vmHeapBytes=393216
- ejecutadas (con EXITED) 9 de 11 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Matriz pruebas x placas

| prueba | stm32u575@COM5 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-24 15:35:01

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 314 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 11 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 12 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 8 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 49 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 366 | 8 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 24 2026 10:08:03` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=18176 fs=557056/2064384 B vmHeapBytes=1179648
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 46 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 320 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 117 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 448 | 314 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 210 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 507 | 11 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 754 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 301 | 12 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 396 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 423 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 4985 | 51 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 5353 | 369 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=20594 fsUsed=557056 vmHeapBytes=1179648
- tras ThrowSinAtrapar: uptimeMs=21829 fsUsed=557056 vmHeapBytes=1179648
- tras ThreadTrasMain: uptimeMs=22912 fsUsed=557056 vmHeapBytes=1179648
- tras MachineId: uptimeMs=24198 fsUsed=557056 vmHeapBytes=1179648
- tras Destructor: uptimeMs=26433 fsUsed=565248 vmHeapBytes=1179648
- tras TimeDemo: uptimeMs=28452 fsUsed=573440 vmHeapBytes=1179648
- tras ArrayFijo: uptimeMs=29669 fsUsed=581632 vmHeapBytes=1179648
- tras MoveRango: uptimeMs=30987 fsUsed=589824 vmHeapBytes=1179648
- tras HiloUsa: uptimeMs=32520 fsUsed=606208 vmHeapBytes=1179648
- tras GuiWinJson: uptimeMs=56025 fsUsed=704512 vmHeapBytes=1179648
- tras GuiShot: uptimeMs=64200 fsUsed=704512 vmHeapBytes=1179648
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (7292 B, crc bd19c7be) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- ejecutadas (con EXITED) 11 de 11 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45038 | ae0d2a7e | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22616 | 578601f2 | ok (por trozos) |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12295 | feacb85e | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4641 | 8e56f753 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2116 | 093521a9 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1391 | a455f18e | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib
- dependencia Json: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib
- dependencia Collections: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib
- dependencia Str: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib

### Matriz pruebas x placas

| prueba | stm32u5g9@COM12 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  


## Tanda 2026-09-24 15:37:41

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c5@COM6)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 327 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 12 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 11 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 8 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 11 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 53 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 368 | 8 |

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Sep 24 2026 10:09:40` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=unknown uptimeMs=22193 fs=217088/1523712 B vmHeapBytes=6812672
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 31 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 30 | 12 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 20 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 30 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 30 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 670 | 47 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 590 | 378 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=23895 fsUsed=217088 vmHeapBytes=6812672
- tras ThrowSinAtrapar: uptimeMs=24963 fsUsed=217088 vmHeapBytes=6812672
- tras ThreadTrasMain: uptimeMs=26030 fsUsed=217088 vmHeapBytes=6812672
- tras MachineId: uptimeMs=27482 fsUsed=217088 vmHeapBytes=6812672
- tras Destructor: uptimeMs=29166 fsUsed=225280 vmHeapBytes=6812672
- tras TimeDemo: uptimeMs=31502 fsUsed=229376 vmHeapBytes=6812672
- tras ArrayFijo: uptimeMs=32569 fsUsed=233472 vmHeapBytes=6812672
- tras MoveRango: uptimeMs=33653 fsUsed=237568 vmHeapBytes=6812672
- tras HiloUsa: uptimeMs=34921 fsUsed=245760 vmHeapBytes=6812672
- tras GuiWinJson: uptimeMs=38608 fsUsed=253952 vmHeapBytes=6812672
- tras GuiShot: uptimeMs=42361 fsUsed=253952 vmHeapBytes=6812672
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.shot` (3350 B, crc f0589b1a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- ejecutadas (con EXITED) 11 de 11 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2116 | 093521a9 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1391 | a455f18e | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:Gui/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Gui/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Gui/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Gui/Str.mod, crc igual): se ejecuta XIP, no se sube

### Matriz pruebas x placas

| prueba | esp32c5@COM6 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en esp32c5@COM6: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png  


## Tanda 2026-09-24 15:40:49

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 13 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 323 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 8 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 18 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 48 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 378 | 8 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Sep 24 2026 10:10:12` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=17332 fs=360448/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 35 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 13 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **COLGADA** | - | 13 | sin EXITED en 30 s; tras KILL: tampoco EXITED en 5 s |
| ArrayFijo | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 18 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **COLGADA** | - | 46 | sin EXITED en 60 s; tras KILL: tampoco EXITED en 5 s |
| GuiShot | **COLGADA** | - | 371 | sin EXITED en 60 s; tras KILL: tampoco EXITED en 5 s; la placa SE REINICIO durante la prueba (uptimeMs 59462 -> 59269) |

- tras MathRango: uptimeMs=19234 fsUsed=360448 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=20268 fsUsed=360448 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=21336 fsUsed=360448 vmHeapBytes=65536
- tras MachineId: uptimeMs=22618 fsUsed=360448 vmHeapBytes=65536
- tras Destructor: uptimeMs=24323 fsUsed=368640 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=33421 fsUsed=372736 vmHeapBytes=65536
<details><summary>TimeDemo: 20 lineas NO-JSON del wire (panics, hard faults, avisos)</summary>

```
I (595) cpu_start: GPIO 17 and 16 are used as console UART I/O pins
I (596) cpu_start: Pro cpu start user code
I (596) cpu_start: cpu freq: 160000000 Hz
I (598) app_init: Application information:
I (601) app_init: Project name:     bpvm_esp32c6
I (606) app_init: App version:      v6.0-218-g81fc3773-dirty
I (611) app_init: Compile time:     Sep 24 2026 15:40:18
I (616) app_init: ELF file SHA256:  955acc7a7...
I (621) app_init: ESP-IDF:          v6.0.1
I (624) efuse_init: Min chip rev:     v0.0
I (628) efuse_init: Max chip rev:     v0.99
I (632) efuse_init: Chip rev:         v0.2
I (636) heap_init: Initializing. RAM available for dynamic allocation:
I (642) heap_init: At 40836340 len 000462D0 (280 KiB): RAM
I (647) heap_init: At 4087C610 len 00002F54 (11 KiB): RAM
I (653) heap_init: At 50000024 len 00003FC4 (15 KiB): RTCRAM
I (659) spi_flash: detected chip: generic
I (662) spi_flash: flash io: dio
I (665) sleep_gpio: Configure to isolate all GPIO pins in sleep state
I (671) sleep_gpio: Enable automatic switching of GPIO sleep configuration
```
</details>

- tras ArrayFijo: uptimeMs=34472 fsUsed=376832 vmHeapBytes=65536
- tras MoveRango: uptimeMs=35508 fsUsed=380928 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=36772 fsUsed=389120 vmHeapBytes=65536
- tras GuiWinJson: uptimeMs=59462 fsUsed=376832 vmHeapBytes=65536
<details><summary>GuiWinJson: 20 lineas NO-JSON del wire (panics, hard faults, avisos)</summary>

```
I (595) cpu_start: GPIO 17 and 16 are used as console UART I/O pins
I (596) cpu_start: Pro cpu start user code
I (596) cpu_start: cpu freq: 160000000 Hz
I (598) app_init: Application information:
I (602) app_init: Project name:     bpvm_esp32c6
I (606) app_init: App version:      v6.0-218-g81fc3773-dirty
I (611) app_init: Compile time:     Sep 24 2026 15:40:18
I (616) app_init: ELF file SHA256:  955acc7a7...
I (621) app_init: ESP-IDF:          v6.0.1
I (625) efuse_init: Min chip rev:     v0.0
I (628) efuse_init: Max chip rev:     v0.99
I (632) efuse_init: Chip rev:         v0.2
I (636) heap_init: Initializing. RAM available for dynamic allocation:
I (642) heap_init: At 40836340 len 000462D0 (280 KiB): RAM
I (648) heap_init: At 4087C610 len 00002F54 (11 KiB): RAM
I (653) heap_init: At 50000024 len 00003FC4 (15 KiB): RTCRAM
I (659) spi_flash: detected chip: generic
I (662) spi_flash: flash io: dio
I (666) sleep_gpio: Configure to isolate all GPIO pins in sleep state
I (671) sleep_gpio: Enable automatic switching of GPIO sleep configuration
```
</details>

- **la placa SE REINICIO durante GuiShot** (uptimeMs decrecio; ver notas)
- tras GuiShot: uptimeMs=59269 fsUsed=372736 vmHeapBytes=65536
<details><summary>GuiShot: 20 lineas NO-JSON del wire (panics, hard faults, avisos)</summary>

```
I (596) cpu_start: GPIO 17 and 16 are used as console UART I/O pins
I (596) cpu_start: Pro cpu start user code
I (596) cpu_start: cpu freq: 160000000 Hz
I (598) app_init: Application information:
I (602) app_init: Project name:     bpvm_esp32c6
I (606) app_init: App version:      v6.0-218-g81fc3773-dirty
I (611) app_init: Compile time:     Sep 24 2026 15:40:18
I (616) app_init: ELF file SHA256:  955acc7a7...
I (621) app_init: ESP-IDF:          v6.0.1
I (625) efuse_init: Min chip rev:     v0.0
I (628) efuse_init: Max chip rev:     v0.99
I (632) efuse_init: Chip rev:         v0.2
I (636) heap_init: Initializing. RAM available for dynamic allocation:
I (643) heap_init: At 40836340 len 000462D0 (280 KiB): RAM
I (648) heap_init: At 4087C610 len 00002F54 (11 KiB): RAM
I (653) heap_init: At 50000024 len 00003FC4 (15 KiB): RTCRAM
I (659) spi_flash: detected chip: generic
I (662) spi_flash: flash io: dio
I (666) sleep_gpio: Configure to isolate all GPIO pins in sleep state
I (671) sleep_gpio: Enable automatic switching of GPIO sleep configuration
```
</details>

- ejecutadas (con EXITED) 8 de 11 listadas; no terminaron 3; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45038 | ae0d2a7e | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22616 | 578601f2 | ok (por trozos) |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12295 | feacb85e | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4641 | 8e56f753 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2116 | 093521a9 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1391 | a455f18e | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en /lib con crc 1f305f94 != local ae0d2a7e (DESFASE, se sube la de bpstdlib a /lib)
- dependencia Json: en /lib con crc 8d090549 != local 578601f2 (DESFASE, se sube la de bpstdlib a /lib)
- dependencia Collections: en /lib con crc 740b9a71 != local feacb85e (DESFASE, se sube la de bpstdlib a /lib)
- dependencia Str: en /lib con crc c2cb43dd != local 8e56f753 (DESFASE, se sube la de bpstdlib a /lib)
- GuiShot: la placa SE REINICIO durante la prueba (uptimeMs 59462 -> 59269, resetReason=panic/exception)

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | COLGADA |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | COLGADA |
| GuiShot | COLGADA |



## Tanda 2026-09-24 15:53:05

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 0 (ninguna)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 13 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 331 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 10 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 24 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 11 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 17 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 63 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 377 | 8 |

### Matriz pruebas x placas

| prueba |
|---|
| MathRango |
| ThrowSinAtrapar |
| ThreadTrasMain |
| MachineId |
| Destructor |
| TimeDemo |
| ArrayFijo |
| MoveRango |
| HiloUsa |
| GuiWinJson |
| GuiShot |

(sin placas: no se ejecuto nada en dispositivo)  


## Tanda 2026-09-24 15:53:32

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 322 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 8 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 14 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 13 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 47 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 360 | 8 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Sep 24 2026 15:52:33` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=23226 fs=372736/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 41 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 30 | 8 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 30 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 13 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **DIFIERE** | 370 | 49 | texto distinto; estado no casa (host rc=0, EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: No space in heap)) |
| GuiShot | **NO-PETA** | 450 | 373 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=25111 fsUsed=372736 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=26330 fsUsed=372736 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=27381 fsUsed=372736 vmHeapBytes=65536
- tras MachineId: uptimeMs=28866 fsUsed=372736 vmHeapBytes=65536
- tras Destructor: uptimeMs=30550 fsUsed=372736 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=32837 fsUsed=372736 vmHeapBytes=65536
- tras ArrayFijo: uptimeMs=33903 fsUsed=372736 vmHeapBytes=65536
- tras MoveRango: uptimeMs=34938 fsUsed=372736 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=36205 fsUsed=372736 vmHeapBytes=65536
- tras GuiWinJson: uptimeMs=45248 fsUsed=372736 vmHeapBytes=65536
<details><summary>GuiWinJson: cola del stderr del host</summary>

```
 slots (5 KB dentro del heap) OK — techo del heap 387984, libre 322 KB
[bpvm] GC por TABLA de handles: 591/654 slots (marca cruzada)
[gc] pila t0: rango [393216..393568) = 88 palabras, 24 son refs al heap
[gc] vivo=4452 liberado=37376 heap=[57984..99812) freelist=si
[gc] reservas: de_lista=0 de_bump=590 astilla=0
[gc] libres: 42 huecos, 37376 B en total, el mayor 7328 | altas=42 fusion_izq=0 fusion_der=0
[bpvm] GC: bump 99812 -> 99812 (recupera 0 B, tope 42753, heap 57984..387984)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 1 vivos)

```
</details>

<details><summary>GuiWinJson: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -1,17 +1 @@
-J1: {"type":"Panel","width":240,"height":240,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}
--- arbol tras cargar main.win --
-screen [240x240 align=0 +0,0]
-  panel [240x240 align=0 +0,0]
-    panel [240x240 align=0 +0,0]
-      label "Demo de Forms — Camino A" [-1x-1 align=1 +0,12]
-      button [-1x-1 align=4 +0,0]
...
```

oraculo (host):

```
J1: {"type":"Panel","width":480,"height":320,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}
-- arbol tras cargar main.win --
screen [480x320 align=0 +0,0]
  panel [480x320 align=0 +0,0]
    panel [480x320 align=0 +0,0]
      label "Demo de Forms — Camino A" [-1x-1 align=1 +0,12]
      button [-1x-1 align=4 +0,0]
        label "Saludar" [-1x-1 align=0 +0,0]
      checkbox "Activar" [-1x-1 align=7 +0,-20 val=0]

WIN: {"type":"Window","width":480,"height":320,"children":[{"type":"Panel","width":480,"height":320,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}]}
J2 == J1: true
arbol igual: true
hijos del panel: 3
hijos del boton (la etiqueta): 1
boton en JSON: {"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"}
FIN
```

placa:

```

```
</details>

- tras GuiShot: uptimeMs=55189 fsUsed=376832 vmHeapBytes=65536
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: no existia
- ejecutadas (con EXITED) 11 de 11 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2116 | 093521a9 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1391 | a455f18e | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: ya en /lib/Gui.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Json: ya en /lib/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | DIFIERE |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  


## Tanda 2026-09-24 16:04:08

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 16 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 318 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 13 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 27 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 11 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 11 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 28 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 84 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 371 | 8 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Sep 24 2026 10:07:37` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=48935 fs=339968/1048576 B vmHeapBytes=264256
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 40 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 30 | 16 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 7 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 304 | 318 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 7 | 13 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 68 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 52 | 27 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 20 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 75 | 11 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 31 | 28 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 84 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 371 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |

- tras MathRango: uptimeMs=50721 fsUsed=339968 vmHeapBytes=264256
- tras ThrowSinAtrapar: uptimeMs=51990 fsUsed=339968 vmHeapBytes=264256
- tras ThreadTrasMain: uptimeMs=53242 fsUsed=339968 vmHeapBytes=264256
- tras MachineId: uptimeMs=54708 fsUsed=339968 vmHeapBytes=264256
- tras Destructor: uptimeMs=56889 fsUsed=344064 vmHeapBytes=264256
- tras TimeDemo: uptimeMs=59278 fsUsed=344064 vmHeapBytes=264256
- tras ArrayFijo: uptimeMs=60693 fsUsed=344064 vmHeapBytes=264256
- tras MoveRango: uptimeMs=61964 fsUsed=348160 vmHeapBytes=264256
- tras HiloUsa: uptimeMs=63646 fsUsed=356352 vmHeapBytes=264256
- ejecutadas (con EXITED) 9 de 11 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Matriz pruebas x placas

| prueba | rp2350a@COM22 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-24 16:09:40

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c3@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 13 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 312 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 13 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 21 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 47 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 379 | 8 |

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Sep 24 2026 16:05:01` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=20126 fs=180224/1523712 B vmHeapBytes=61440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 24 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 13 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 312 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 30 | 13 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 40 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 21 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 47 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 379 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |

- tras MathRango: uptimeMs=22229 fsUsed=180224 vmHeapBytes=61440
- tras ThrowSinAtrapar: uptimeMs=23280 fsUsed=180224 vmHeapBytes=61440
- tras ThreadTrasMain: uptimeMs=24349 fsUsed=180224 vmHeapBytes=61440
- tras MachineId: uptimeMs=25632 fsUsed=180224 vmHeapBytes=61440
- tras Destructor: uptimeMs=27533 fsUsed=188416 vmHeapBytes=61440
- tras TimeDemo: uptimeMs=29372 fsUsed=192512 vmHeapBytes=61440
- tras ArrayFijo: uptimeMs=30420 fsUsed=196608 vmHeapBytes=61440
- tras MoveRango: uptimeMs=31472 fsUsed=200704 vmHeapBytes=61440
- tras HiloUsa: uptimeMs=32922 fsUsed=208896 vmHeapBytes=61440
- ejecutadas (con EXITED) 9 de 11 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Matriz pruebas x placas

| prueba | esp32c3@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-24 16:10:10

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\tanda_c3.json` (2 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c3@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| trytest | TryTest | d7b17849 | 1309 | ok | 0 | 9 | 7 |
| JsonDemo | JsonDemo | fcb7d66d | 2436 | ok | 0 | 19 | 30 |

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Sep 24 2026 16:05:01` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=12978 fs=208896/1523712 B vmHeapBytes=61440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 30 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| trytest | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0 |
| JsonDemo | **IDENTICO** | 110 | 19 | EXITED OK exit=0; host rc=0 |

- tras trytest: uptimeMs=14413 fsUsed=212992 vmHeapBytes=61440
- tras JsonDemo: uptimeMs=18599 fsUsed=241664 vmHeapBytes=61440
- ejecutadas (con EXITED) 2 de 2 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/TryTest.mod | bpgenvm-c\build\tanda\trytest\TryTest.mod | 1309 | d7b17849 | ok |
| /lib/Json.mod | bpstdlib\Json.mod | 22616 | 578601f2 | ok (por trozos) |
| /app/JsonDemo.mod | bpgenvm-c\build\tanda\JsonDemo\JsonDemo.mod | 2436 | fcb7d66d | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Json: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib

### Matriz pruebas x placas

| prueba | esp32c3@COM3 |
|---|---|
| trytest | IDENTICO |
| JsonDemo | IDENTICO |



## Tanda 2026-09-24 16:12:26

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 0 (ninguna)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 322 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 13 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 11 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 12 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 20 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 49 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 369 | 8 |

### Matriz pruebas x placas

| prueba |
|---|
| MathRango |
| ThrowSinAtrapar |
| ThreadTrasMain |
| MachineId |
| Destructor |
| TimeDemo |
| ArrayFijo |
| MoveRango |
| HiloUsa |
| GuiWinJson |
| GuiShot |

(sin placas: no se ejecuto nada en dispositivo)  


## Tanda 2026-09-24 16:12:57

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 18 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 324 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 19 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 12 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 11 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 48 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 364 | 8 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 24 2026 16:06:35` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=31291 fs=483328/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 59 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 18 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 324 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 19 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 12 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 560 | 49 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 490 | 378 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=33832 fsUsed=483328 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=35331 fsUsed=483328 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=36751 fsUsed=483328 vmHeapBytes=24637440
- tras MachineId: uptimeMs=38368 fsUsed=483328 vmHeapBytes=24637440
- tras Destructor: uptimeMs=40856 fsUsed=487424 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=43476 fsUsed=487424 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=44923 fsUsed=487424 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=46527 fsUsed=491520 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=48694 fsUsed=499712 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=57934 fsUsed=499712 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=67659 fsUsed=499712 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (6989 B, crc 8192c86e) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- ejecutadas (con EXITED) 11 de 11 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2116 | 093521a9 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1391 | a455f18e | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: ya en /lib/Gui.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Json: ya en /lib/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | esp32p4@COM15 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  


## Tanda 2026-09-24 16:15:46

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 0 (ninguna)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 326 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 15 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 10 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 49 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 371 | 8 |

### Matriz pruebas x placas

| prueba |
|---|
| MathRango |
| ThrowSinAtrapar |
| ThreadTrasMain |
| MachineId |
| Destructor |
| TimeDemo |
| ArrayFijo |
| MoveRango |
| HiloUsa |
| GuiWinJson |
| GuiShot |

(sin placas: no se ejecuto nada en dispositivo)  


## Tanda 2026-09-24 16:16:50

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32s3@COM9)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 317 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 17 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 12 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 14 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 47 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 387 | 8 |

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Sep 24 2026 16:04:00` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=64467 fs=372736/10248192 B vmHeapBytes=7336960
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 36 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 317 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 40 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 50 | 17 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 20 | 12 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 50 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 30 | 14 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 47 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 387 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |

- tras MathRango: uptimeMs=67154 fsUsed=372736 vmHeapBytes=7336960
- tras ThrowSinAtrapar: uptimeMs=68622 fsUsed=372736 vmHeapBytes=7336960
- tras ThreadTrasMain: uptimeMs=69873 fsUsed=372736 vmHeapBytes=7336960
- tras MachineId: uptimeMs=71592 fsUsed=372736 vmHeapBytes=7336960
- tras Destructor: uptimeMs=73978 fsUsed=380928 vmHeapBytes=7336960
- tras TimeDemo: uptimeMs=76179 fsUsed=385024 vmHeapBytes=7336960
- tras ArrayFijo: uptimeMs=77498 fsUsed=389120 vmHeapBytes=7336960
- tras MoveRango: uptimeMs=79415 fsUsed=393216 vmHeapBytes=7336960
- tras HiloUsa: uptimeMs=81350 fsUsed=401408 vmHeapBytes=7336960
- ejecutadas (con EXITED) 9 de 11 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Matriz pruebas x placas

| prueba | esp32s3@COM9 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-24 16:18:34

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (11 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-24 10:01:19) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-24 09:46:07) - sabor `gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 18 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 324 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 16 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 46 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 364 | 8 |

### Placa rp2350a - 4CE219E14C8F8E1C

- sello: boardName=`rp2350a` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Sep 24 2026 10:07:37` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=16901 fs=167936/10248192 B vmHeapBytes=5767168
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 18 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 41 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 16 | 18 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 315 | 324 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 17 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 77 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 72 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 33 | 16 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 90 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 46 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 46 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 364 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |

- tras MathRango: uptimeMs=18919 fsUsed=172032 vmHeapBytes=5767168
- tras ThrowSinAtrapar: uptimeMs=20002 fsUsed=176128 vmHeapBytes=5767168
- tras ThreadTrasMain: uptimeMs=21111 fsUsed=180224 vmHeapBytes=5767168
- tras MachineId: uptimeMs=22408 fsUsed=184320 vmHeapBytes=5767168
- tras Destructor: uptimeMs=24640 fsUsed=192512 vmHeapBytes=5767168
- tras TimeDemo: uptimeMs=26244 fsUsed=196608 vmHeapBytes=5767168
- tras ArrayFijo: uptimeMs=27498 fsUsed=200704 vmHeapBytes=5767168
- tras MoveRango: uptimeMs=28791 fsUsed=204800 vmHeapBytes=5767168
- tras HiloUsa: uptimeMs=30518 fsUsed=212992 vmHeapBytes=5767168
- ejecutadas (con EXITED) 9 de 11 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Matriz pruebas x placas

| prueba | rp2350a@COM4 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en rp2350a@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  

