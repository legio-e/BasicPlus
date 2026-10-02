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


## Tanda 2026-10-01 09:04:23

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (29 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-30 11:21:15) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-30 21:58:28) - sabor `gui1-lvgl0`
- placas conectadas: 4 (stm32u5g9@COM12, esp32p4@COM15, rp2350a@COM22, esp32c5@COM6)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | b435a8b4 | 290 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 2268f642 | 527 | ok | 0 | 327 | 3 |
| MachineId | MachineId | 2b38cfb7 | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 748c89ef | 4602 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 14 | 13 |
| ArrayFijo | ArrayFijo | 3a36e8cb | 2349 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | f4f81618 | 3600 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | f8904450 | 2164 | ok | 0 | 11 | 8 |
| GuiWinJson | GuiWinJson | 8091ac8b | 2060 | ok | 0 | 45 | 17 |
| GuiShot | GuiShot | 48635085 | 1344 | ok | 0 | 372 | 8 |
| NatNew | NatNew | d1001418 | 2057 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | d9a8086a | 1552 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 03942d44 | 4867 | ok | 0 | 11 | 40 |
| NatCede | NatCede | c32ecc30 | 1494 | ok | 0 | 89 | 7 |
| NatExpropia | NatExpropia | 97ae536a | 900 | ok | 0 | 10 | 4 |
| NatBuiltin | NatBuiltin | a8ae365e | 1587 | ok | 0 | 44 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 10 | 3 |
| NatDirecta | NatDirecta | 47af3b73 | 1018 | ok | 0 | 8 | 9 |
| NatCoste | NatCoste | 01da15a7 | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 9b3bc755 | 1434 | ok | 0 | 8 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| CatCadena | CatCadena | 5edfd98a | 3839 | ok | 0 | 9 | 24 |
| CatCrono | CatCrono | 3b281178 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 8 | 12 |
| SbCrono | SbCrono | c68dbf62 | 819 | ok | 0 | 9 | 2 |
| HeapInfo | HeapInfo | 4397d411 | 1097 | ok | 0 | 12 | 10 |
| HeapCompact | HeapCompact | 40751a0d | 3562 | ok | 0 | 23 | 17 |
| HeapReposo | HeapReposo | 44621731 | 2783 | ok | 0 | 55 | 8 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  1 2026 07:08:37` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=2608195 fs=581632/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 57 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 311 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 111 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 437 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 203 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 480 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 700 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 279 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 380 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 467 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 3640 | 51 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 3809 | 359 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 248 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 320 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 398 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 228 | 89 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 173 | 10 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 234 | 44 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 155 | 10 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 175 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1353 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 205 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 342 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 334 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 296 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 277 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 362 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 243 | 12 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 1425 | 23 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 975 | 55 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=2610597 fsUsed=581632 vmHeapBytes=786432
- tras ThrowSinAtrapar: uptimeMs=2611631 fsUsed=581632 vmHeapBytes=786432
- tras ThreadTrasMain: uptimeMs=2612722 fsUsed=581632 vmHeapBytes=786432
- tras MachineId: uptimeMs=2614199 fsUsed=581632 vmHeapBytes=786432
- tras Destructor: uptimeMs=2616451 fsUsed=581632 vmHeapBytes=786432
- tras TimeDemo: uptimeMs=2618470 fsUsed=581632 vmHeapBytes=786432
- tras ArrayFijo: uptimeMs=2619688 fsUsed=581632 vmHeapBytes=786432
- tras MoveRango: uptimeMs=2621060 fsUsed=581632 vmHeapBytes=786432
- tras HiloUsa: uptimeMs=2622560 fsUsed=581632 vmHeapBytes=786432
- tras GuiWinJson: uptimeMs=2645477 fsUsed=679936 vmHeapBytes=786432
- tras GuiShot: uptimeMs=2652026 fsUsed=679936 vmHeapBytes=786432
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=2655585 fsUsed=679936 vmHeapBytes=786432
- tras NatV7: uptimeMs=2657198 fsUsed=679936 vmHeapBytes=786432
- tras FloatNat: uptimeMs=2660632 fsUsed=679936 vmHeapBytes=786432
- tras NatCede: uptimeMs=2662186 fsUsed=679936 vmHeapBytes=786432
- tras NatExpropia: uptimeMs=2663868 fsUsed=679936 vmHeapBytes=786432
- tras NatBuiltin: uptimeMs=2665518 fsUsed=679936 vmHeapBytes=786432
- tras NatAnida: uptimeMs=2667005 fsUsed=679936 vmHeapBytes=786432
- tras NatDirecta: uptimeMs=2668822 fsUsed=679936 vmHeapBytes=786432
- tras NatCoste: uptimeMs=2671492 fsUsed=679936 vmHeapBytes=786432
- tras NatNombres: uptimeMs=2673077 fsUsed=679936 vmHeapBytes=786432
- tras NatEstrecha: uptimeMs=2674728 fsUsed=679936 vmHeapBytes=786432
- tras CatCadena: uptimeMs=2676096 fsUsed=679936 vmHeapBytes=786432
- tras CatCrono: uptimeMs=2677394 fsUsed=679936 vmHeapBytes=786432
- tras SbPiezas: uptimeMs=2678615 fsUsed=679936 vmHeapBytes=786432
- tras SbCrono: uptimeMs=2679697 fsUsed=679936 vmHeapBytes=786432
- tras HeapInfo: uptimeMs=2681245 fsUsed=679936 vmHeapBytes=786432
- tras HeapCompact: uptimeMs=2684213 fsUsed=679936 vmHeapBytes=786432
- tras HeapReposo: uptimeMs=2685884 fsUsed=679936 vmHeapBytes=786432
- ejecutadas (con EXITED) 29 de 29 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 290 | b435a8b4 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 527 | 2268f642 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4602 | 748c89ef | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2349 | 3a36e8cb | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3600 | f4f81618 | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2164 | f8904450 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45031 | 63045d4d | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22729 | 7d2494c4 | ok (por trozos) |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12295 | d16238b4 | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4597 | b52b92b4 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2060 | 8091ac8b | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4121 | c3b7d4bd | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2290 | afcec746 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8446 | 6f671ea3 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2216 | 1b6831b0 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1121 | 29573c1d | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2581 | 0338a10a | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2159 | 042932ad | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | ee360c06 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2452 | 6fa9e397 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3839 | 5edfd98a | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1097 | 4397d411 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3562 | 40751a0d | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2783 | 44621731 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: el RUN la resuelve a pack:Gui/Gui.mod (fuera de /lib y /app), crc ae0d2a7e: se sube a /lib
- dependencia Json: el RUN la resuelve a pack:Gui/Json.mod (fuera de /lib y /app), crc 578601f2: se sube a /lib
- dependencia Collections: el RUN la resuelve a pack:Gui/Collections.mod (fuera de /lib y /app), crc feacb85e: se sube a /lib
- dependencia Str: el RUN la resuelve a pack:Gui/Str.mod (fuera de /lib y /app), crc 8e56f753: se sube a /lib
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  1 2026 07:09:22` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=2575016 fs=483328/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 75 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 580 | 47 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 500 | 368 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 70 | 89 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 50 | 44 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 550 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatNombres | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| NatEstrecha | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| CatCadena | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 40 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 50 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 410 | 23 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 180 | 55 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=2577652 fsUsed=483328 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=2579105 fsUsed=483328 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=2580546 fsUsed=483328 vmHeapBytes=24637440
- tras MachineId: uptimeMs=2582022 fsUsed=483328 vmHeapBytes=24637440
- tras Destructor: uptimeMs=2584624 fsUsed=483328 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=2587359 fsUsed=483328 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=2589114 fsUsed=483328 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=2590732 fsUsed=483328 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=2592865 fsUsed=483328 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=2619104 fsUsed=581632 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=2628796 fsUsed=581632 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=2632900 fsUsed=581632 vmHeapBytes=24637440
- tras NatV7: uptimeMs=2634884 fsUsed=581632 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=2638488 fsUsed=581632 vmHeapBytes=24637440
- tras NatCede: uptimeMs=2640356 fsUsed=581632 vmHeapBytes=24637440
- tras NatExpropia: uptimeMs=2642024 fsUsed=581632 vmHeapBytes=24637440
- tras NatBuiltin: uptimeMs=2643975 fsUsed=581632 vmHeapBytes=24637440
- tras NatAnida: uptimeMs=2646061 fsUsed=581632 vmHeapBytes=24637440
- tras NatDirecta: uptimeMs=2647829 fsUsed=581632 vmHeapBytes=24637440
- tras NatCoste: uptimeMs=2650014 fsUsed=581632 vmHeapBytes=24637440
- tras NatNombres: uptimeMs=2651832 fsUsed=581632 vmHeapBytes=24637440
- tras NatEstrecha: uptimeMs=2653817 fsUsed=581632 vmHeapBytes=24637440
- tras CatCadena: uptimeMs=2655503 fsUsed=581632 vmHeapBytes=24637440
- tras CatCrono: uptimeMs=2656853 fsUsed=581632 vmHeapBytes=24637440
- tras SbPiezas: uptimeMs=2658338 fsUsed=585728 vmHeapBytes=24637440
- tras SbCrono: uptimeMs=2660138 fsUsed=589824 vmHeapBytes=24637440
- tras HeapInfo: uptimeMs=2661675 fsUsed=593920 vmHeapBytes=24637440
- tras HeapCompact: uptimeMs=2664565 fsUsed=598016 vmHeapBytes=24637440
- tras HeapReposo: uptimeMs=2666493 fsUsed=602112 vmHeapBytes=24637440
- ejecutadas (con EXITED) 29 de 29 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 290 | b435a8b4 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 527 | 2268f642 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4602 | 748c89ef | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2349 | 3a36e8cb | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3600 | f4f81618 | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2164 | f8904450 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45031 | 63045d4d | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22729 | 7d2494c4 | ok (por trozos) |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12295 | d16238b4 | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4597 | b52b92b4 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2060 | 8091ac8b | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4437 | d56c8c12 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2442 | 1f3a6bd3 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9322 | a821aeed | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2320 | 62b0de4c | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1161 | 07a5e0ae | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMAFC_ILP32F\NatBuiltin.mod | 2785 | 667ad14e | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMAFC_ILP32F\NatAnida.mod | 1232 | 3b121406 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMAFC_ILP32F\NatDirecta.mod | 2531 | 670e020b | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMAFC_ILP32F\NatCoste.mod | 1000 | 6d904575 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMAFC_ILP32F\NatNombres.mod | 2696 | 27d4ba10 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMAFC_ILP32F\NatEstrecha.mod | 3131 | 3dac6451 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3839 | 5edfd98a | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1097 | 4397d411 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3562 | 40751a0d | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2783 | 44621731 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: el RUN la resuelve a pack:Gui/Gui.mod (fuera de /lib y /app), crc ae0d2a7e: se sube a /lib
- dependencia Json: el RUN la resuelve a pack:Gui/Json.mod (fuera de /lib y /app), crc 578601f2: se sube a /lib
- dependencia Collections: el RUN la resuelve a pack:Gui/Collections.mod (fuera de /lib y /app), crc feacb85e: se sube a /lib
- dependencia Str: el RUN la resuelve a pack:Gui/Str.mod (fuera de /lib y /app), crc 8e56f753: se sube a /lib
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  1 2026 07:08:27` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=2834188 fs=466944/1048576 B vmHeapBytes=263080
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 66 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 30 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 7 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 306 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 7 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 55 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 48 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 22 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 68 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 35 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 45 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 372 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 26 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 22 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 54 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 61 | 89 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 12 | 10 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 43 | 44 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 16 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 789 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 11 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 68 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 57 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 94 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 43 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 97 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 35 | 12 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 844 | 23 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 369 | 55 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=2836222 fsUsed=466944 vmHeapBytes=263080
- tras ThrowSinAtrapar: uptimeMs=2837289 fsUsed=466944 vmHeapBytes=263080
- tras ThreadTrasMain: uptimeMs=2838379 fsUsed=466944 vmHeapBytes=263080
- tras MachineId: uptimeMs=2839875 fsUsed=466944 vmHeapBytes=263080
- tras Destructor: uptimeMs=2842130 fsUsed=466944 vmHeapBytes=263080
- tras TimeDemo: uptimeMs=2843921 fsUsed=466944 vmHeapBytes=263080
- tras ArrayFijo: uptimeMs=2845133 fsUsed=466944 vmHeapBytes=263080
- tras MoveRango: uptimeMs=2846484 fsUsed=466944 vmHeapBytes=263080
- tras HiloUsa: uptimeMs=2848002 fsUsed=466944 vmHeapBytes=263080
- tras NatNew: uptimeMs=2850008 fsUsed=466944 vmHeapBytes=263080
- tras NatV7: uptimeMs=2851235 fsUsed=466944 vmHeapBytes=263080
- tras FloatNat: uptimeMs=2854008 fsUsed=466944 vmHeapBytes=263080
- tras NatCede: uptimeMs=2855431 fsUsed=466944 vmHeapBytes=263080
- tras NatExpropia: uptimeMs=2856581 fsUsed=466944 vmHeapBytes=263080
- tras NatBuiltin: uptimeMs=2857826 fsUsed=466944 vmHeapBytes=263080
- tras NatAnida: uptimeMs=2858966 fsUsed=466944 vmHeapBytes=263080
- tras NatDirecta: uptimeMs=2860181 fsUsed=466944 vmHeapBytes=263080
- tras NatCoste: uptimeMs=2861717 fsUsed=466944 vmHeapBytes=263080
- tras NatNombres: uptimeMs=2862972 fsUsed=466944 vmHeapBytes=263080
- tras NatEstrecha: uptimeMs=2864231 fsUsed=466944 vmHeapBytes=263080
- tras CatCadena: uptimeMs=2865597 fsUsed=466944 vmHeapBytes=263080
- tras CatCrono: uptimeMs=2866719 fsUsed=466944 vmHeapBytes=263080
- tras SbPiezas: uptimeMs=2868124 fsUsed=466944 vmHeapBytes=263080
- tras SbCrono: uptimeMs=2869222 fsUsed=466944 vmHeapBytes=263080
- tras HeapInfo: uptimeMs=2870755 fsUsed=466944 vmHeapBytes=263080
- tras HeapCompact: uptimeMs=2873306 fsUsed=466944 vmHeapBytes=263080
- tras HeapReposo: uptimeMs=2875179 fsUsed=466944 vmHeapBytes=263080
- ejecutadas (con EXITED) 27 de 29 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 290 | b435a8b4 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 527 | 2268f642 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4602 | 748c89ef | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2349 | 3a36e8cb | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3600 | f4f81618 | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2164 | f8904450 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4121 | c3b7d4bd | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2290 | afcec746 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8446 | 6f671ea3 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2216 | 1b6831b0 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1121 | 29573c1d | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2581 | 0338a10a | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2159 | 042932ad | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | ee360c06 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2452 | 6fa9e397 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3839 | 5edfd98a | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1097 | 4397d411 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3562 | 40751a0d | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2783 | 44621731 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Oct  1 2026 07:08:50` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=software uptimeMs=2773064 fs=327680/16777216 B vmHeapBytes=6288384
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 52 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 20 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 390 | 46 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 500 | 357 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 20 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |
| NatCede | **IDENTICO** | 70 | 89 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatExpropia | **IDENTICO** | 20 | 10 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMC_ILP32) |
| NatBuiltin | **IDENTICO** | 50 | 44 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMC_ILP32) |
| NatAnida | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatDirecta | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMC_ILP32) |
| NatCoste | **IDENTICO** | 640 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatNombres | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| NatEstrecha | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| CatCadena | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 70 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 80 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 20 | 12 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 700 | 23 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 330 | 55 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=2774782 fsUsed=327680 vmHeapBytes=6288384
- tras ThrowSinAtrapar: uptimeMs=2776429 fsUsed=327680 vmHeapBytes=6288384
- tras ThreadTrasMain: uptimeMs=2777468 fsUsed=327680 vmHeapBytes=6288384
- tras MachineId: uptimeMs=2778934 fsUsed=327680 vmHeapBytes=6288384
- tras Destructor: uptimeMs=2780605 fsUsed=327680 vmHeapBytes=6288384
- tras TimeDemo: uptimeMs=2782499 fsUsed=327680 vmHeapBytes=6288384
- tras ArrayFijo: uptimeMs=2783539 fsUsed=327680 vmHeapBytes=6288384
- tras MoveRango: uptimeMs=2784606 fsUsed=327680 vmHeapBytes=6288384
- tras HiloUsa: uptimeMs=2785858 fsUsed=327680 vmHeapBytes=6288384
- tras GuiWinJson: uptimeMs=2801939 fsUsed=425984 vmHeapBytes=6288384
- tras GuiShot: uptimeMs=2809725 fsUsed=425984 vmHeapBytes=6288384
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.shot` (3350 B, crc f0589b1a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=2812285 fsUsed=425984 vmHeapBytes=6288384
- tras NatV7: uptimeMs=2813700 fsUsed=425984 vmHeapBytes=6288384
- tras FloatNat: uptimeMs=2816235 fsUsed=425984 vmHeapBytes=6288384
- tras NatCede: uptimeMs=2817836 fsUsed=430080 vmHeapBytes=6288384
- tras NatExpropia: uptimeMs=2819437 fsUsed=434176 vmHeapBytes=6288384
- tras NatBuiltin: uptimeMs=2821089 fsUsed=438272 vmHeapBytes=6288384
- tras NatAnida: uptimeMs=2823074 fsUsed=442368 vmHeapBytes=6288384
- tras NatDirecta: uptimeMs=2824713 fsUsed=446464 vmHeapBytes=6288384
- tras NatCoste: uptimeMs=2826728 fsUsed=450560 vmHeapBytes=6288384
- tras NatNombres: uptimeMs=2828163 fsUsed=454656 vmHeapBytes=6288384
- tras NatEstrecha: uptimeMs=2829813 fsUsed=458752 vmHeapBytes=6288384
- tras CatCadena: uptimeMs=2831081 fsUsed=462848 vmHeapBytes=6288384
- tras CatCrono: uptimeMs=2832133 fsUsed=466944 vmHeapBytes=6288384
- tras SbPiezas: uptimeMs=2833189 fsUsed=471040 vmHeapBytes=6288384
- tras SbCrono: uptimeMs=2834219 fsUsed=475136 vmHeapBytes=6288384
- tras HeapInfo: uptimeMs=2835501 fsUsed=479232 vmHeapBytes=6288384
- tras HeapCompact: uptimeMs=2838203 fsUsed=483328 vmHeapBytes=6288384
- tras HeapReposo: uptimeMs=2840074 fsUsed=487424 vmHeapBytes=6288384
- ejecutadas (con EXITED) 29 de 29 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 290 | b435a8b4 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 527 | 2268f642 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4602 | 748c89ef | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2349 | 3a36e8cb | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3600 | f4f81618 | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2164 | f8904450 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45031 | 63045d4d | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22729 | 7d2494c4 | ok (por trozos) |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12295 | d16238b4 | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4597 | b52b92b4 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2060 | 8091ac8b | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4453 | 21a595f6 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2442 | a0971c86 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9730 | f6cc997c | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMC_ILP32\NatCede.mod | 2320 | 0a18c7cc | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMC_ILP32\NatExpropia.mod | 1161 | abbbccac | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMC_ILP32\NatBuiltin.mod | 2789 | 7ac18801 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMC_ILP32\NatAnida.mod | 1232 | 359b3eec | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMC_ILP32\NatDirecta.mod | 2527 | b0130698 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMC_ILP32\NatCoste.mod | 1000 | 3f859af0 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMC_ILP32\NatNombres.mod | 2712 | 57a14515 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMC_ILP32\NatEstrecha.mod | 3131 | 01c97fbf | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3839 | 5edfd98a | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1097 | 4397d411 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3562 | 40751a0d | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2783 | 44621731 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: el RUN la resuelve a pack:Gui/Gui.mod (fuera de /lib y /app), crc ae0d2a7e: se sube a /lib
- dependencia Json: el RUN la resuelve a pack:Gui/Json.mod (fuera de /lib y /app), crc 578601f2: se sube a /lib
- dependencia Collections: el RUN la resuelve a pack:Gui/Collections.mod (fuera de /lib y /app), crc feacb85e: se sube a /lib
- dependencia Str: el RUN la resuelve a pack:Gui/Str.mod (fuera de /lib y /app), crc 8e56f753: se sube a /lib
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | stm32u5g9@COM12 | esp32p4@COM15 | rp2350a@COM22 | esp32c5@COM6 |
|---|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA | NO-PETA |
| Destructor | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| TimeDemo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ArrayFijo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MoveRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HiloUsa | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| GuiWinJson | IDENTICO | IDENTICO | SALTADA | IDENTICO |
| GuiShot | NO-PETA | NO-PETA | SALTADA | NO-PETA |
| NatNew | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatV7 | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| FloatNat | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatCede | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatExpropia | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatBuiltin | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatAnida | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatDirecta | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatCoste | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatNombres | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatEstrecha | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| CatCadena | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| CatCrono | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| SbPiezas | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| SbCrono | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HeapInfo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HeapCompact | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HeapReposo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |

NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  
NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
NECESITA OJOS: GuiShot en esp32c5@COM6: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png  


## Tanda 2026-10-02 06:23:21

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (29 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-01 18:45:16) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-01 17:48:47) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 95 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | b435a8b4 | 290 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | 2268f642 | 527 | ok | 0 | 320 | 3 |
| MachineId | MachineId | 2b38cfb7 | 484 | ok | 0 | 10 | 3 |
| Destructor | Destructor | 748c89ef | 4602 | ok | 0 | 13 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 22 | 13 |
| ArrayFijo | ArrayFijo | 3a36e8cb | 2349 | ok | 0 | 18 | 11 |
| MoveRango | MoveRango | f4f81618 | 3600 | ok | 0 | 15 | 26 |
| HiloUsa | HiloUsa | f8904450 | 2164 | ok | 0 | 21 | 8 |
| GuiWinJson | GuiWinJson | 8091ac8b | 2060 | ok | 0 | 63 | 17 |
| GuiShot | GuiShot | 48635085 | 1344 | ok | 0 | 363 | 8 |
| NatNew | NatNew | d1001418 | 2057 | ok | 0 | 14 | 10 |
| NatV7 | NatV7 | d9a8086a | 1552 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 03942d44 | 4867 | ok | 0 | 10 | 40 |
| NatCede | NatCede | c32ecc30 | 1494 | ok | 0 | 97 | 7 |
| NatExpropia | NatExpropia | 97ae536a | 900 | ok | 0 | 9 | 4 |
| NatBuiltin | NatBuiltin | a8ae365e | 1587 | ok | 0 | 51 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 12 | 3 |
| NatDirecta | NatDirecta | 47af3b73 | 1018 | ok | 0 | 12 | 9 |
| NatCoste | NatCoste | 01da15a7 | 772 | ok | 0 | 13 | 2 |
| NatNombres | NatNombres | 9b3bc755 | 1434 | ok | 0 | 12 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 15 | 23 |
| CatCadena | CatCadena | 5edfd98a | 3839 | ok | 0 | 16 | 24 |
| CatCrono | CatCrono | 3b281178 | 886 | ok | 0 | 14 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 14 | 12 |
| SbCrono | SbCrono | c68dbf62 | 819 | ok | 0 | 16 | 2 |
| HeapInfo | HeapInfo | 4397d411 | 1097 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 40751a0d | 3562 | ok | 0 | 27 | 17 |
| HeapReposo | HeapReposo | 44621731 | 2783 | ok | 0 | 73 | 8 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  1 2026 17:54:30` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=261913 fs=241664/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 32 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 95 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 320 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 10 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 22 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 18 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 10 | 15 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 21 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 63 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 363 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 0 | 14 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 0 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |
| NatCede | **IDENTICO** | 60 | 97 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatExpropia | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMC_ILP32) |
| NatBuiltin | **IDENTICO** | 40 | 51 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMC_ILP32) |
| NatAnida | **IDENTICO** | 0 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatDirecta | **IDENTICO** | 0 | 12 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMC_ILP32) |
| NatCoste | **IDENTICO** | 1060 | 13 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatNombres | **IDENTICO** | 0 | 12 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| NatEstrecha | **IDENTICO** | 10 | 15 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| CatCadena | **IDENTICO** | 20 | 16 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 90 | 14 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 20 | 14 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 120 | 16 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **DIFIERE** | 10 | 9 | texto distinto |
| HeapCompact | **DIFIERE** | 290 | 27 | texto distinto; estado no casa (host rc=0, EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: No space in heap)) |
| HeapReposo | **IDENTICO** | 380 | 73 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=262705 fsUsed=241664 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=263213 fsUsed=241664 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=264149 fsUsed=241664 vmHeapBytes=65536
- tras MachineId: uptimeMs=264738 fsUsed=241664 vmHeapBytes=65536
- tras Destructor: uptimeMs=265707 fsUsed=241664 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=266758 fsUsed=241664 vmHeapBytes=65536
- tras ArrayFijo: uptimeMs=267312 fsUsed=241664 vmHeapBytes=65536
- tras MoveRango: uptimeMs=267854 fsUsed=241664 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=268999 fsUsed=241664 vmHeapBytes=65536
- tras NatNew: uptimeMs=270275 fsUsed=241664 vmHeapBytes=65536
- tras NatV7: uptimeMs=271212 fsUsed=241664 vmHeapBytes=65536
- tras FloatNat: uptimeMs=272278 fsUsed=241664 vmHeapBytes=65536
- tras NatCede: uptimeMs=273179 fsUsed=245760 vmHeapBytes=65536
- tras NatExpropia: uptimeMs=274094 fsUsed=249856 vmHeapBytes=65536
- tras NatBuiltin: uptimeMs=275022 fsUsed=253952 vmHeapBytes=65536
- tras NatAnida: uptimeMs=275932 fsUsed=258048 vmHeapBytes=65536
- tras NatDirecta: uptimeMs=277115 fsUsed=262144 vmHeapBytes=65536
- tras NatCoste: uptimeMs=279218 fsUsed=266240 vmHeapBytes=65536
- tras NatNombres: uptimeMs=280136 fsUsed=270336 vmHeapBytes=65536
- tras NatEstrecha: uptimeMs=281069 fsUsed=274432 vmHeapBytes=65536
- tras CatCadena: uptimeMs=281620 fsUsed=278528 vmHeapBytes=65536
- tras CatCrono: uptimeMs=282138 fsUsed=282624 vmHeapBytes=65536
- tras SbPiezas: uptimeMs=282888 fsUsed=286720 vmHeapBytes=65536
- tras SbCrono: uptimeMs=283422 fsUsed=290816 vmHeapBytes=65536
- tras HeapInfo: uptimeMs=283955 fsUsed=294912 vmHeapBytes=65536
<details><summary>HeapInfo: cola del stderr del host</summary>

```
0 B en total
[bpvm] enlace: 2 modulos, 1 busquedas, 4 strcmp, 187 us
[bpvm] tabla de handles: 0 -> 747 slots (5 KB dentro del heap) OK — techo del heap 387240, libre 367 KB
[gc] pila t0: rango [393216..393284) = 17 palabras, 1 son refs al heap
[gc] vivo=19308 liberado=244 heap=[10624..30072) freelist=si
[gc] reservas: de_lista=0 de_bump=410 astilla=0
[gc] libres: 1 huecos, 140 B en total, el mayor 140 | altas=1 fusion_izq=0 fusion_der=0
[bpvm] pila: el main llegó a 140 de 16384 B
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 1 vivos)

```
</details>

<details><summary>HeapInfo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -2,7 +2,7 @@
 ok lo libre cabe en el heap
 ok el mayor bloque cabe en lo libre
 ok los huecos se cuentan
-ok doscientos nodos son al menos cuatrocientos objetos mas
+FALLO doscientos nodos son al menos cuatrocientos objetos mas
 ok lo libre baja al reservar
 ok una colecta mas
...
```

oraculo (host):

```
ok el heap mide algo
ok lo libre cabe en el heap
ok el mayor bloque cabe en lo libre
ok los huecos se cuentan
ok doscientos nodos son al menos cuatrocientos objetos mas
ok lo libre baja al reservar
ok una colecta mas
ok tras soltar y colectar, lo libre sube
ok y los objetos bajan
fin
```

placa:

```
ok el heap mide algo
ok lo libre cabe en el heap
ok el mayor bloque cabe en lo libre
ok los huecos se cuentan
FALLO doscientos nodos son al menos cuatrocientos objetos mas
ok lo libre baja al reservar
ok una colecta mas
ok tras soltar y colectar, lo libre sube
ok y los objetos bajan
fin
```
</details>

- tras HeapCompact: uptimeMs=285376 fsUsed=299008 vmHeapBytes=65536
<details><summary>HeapCompact: cola del stderr del host</summary>

```
3216..393372) = 39 palabras, 8 son refs al heap
[gc] pila t1: rango [409600..409604) = 1 palabras, 0 son refs al heap
[gc] vivo=26556 liberado=7508 heap=[15616..49604) freelist=si
[gc] reservas: de_lista=1657 de_bump=4174 astilla=4052
[gc] libres: 3 huecos, 7432 B en total, el mayor 6300 | altas=2714 fusion_izq=0 fusion_der=0
[gc] compactado: bump 49604 -> 42172 (7 bloques movidos, 0 clavados, 0 huecos)
[bpvm] pila: el main llegó a 328 de 16384 B; el hilo más lleno, a 48 de 4096 B
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 1 vivos)

```
</details>

<details><summary>HeapCompact: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -6,12 +6,3 @@
 ok compactado: con su colecta
 ok compactado: los objetos no aumentan
 ok lo vivo sigue intacto
-ok tras compactar se reserva
-ok y se colecta
-ok y se vuelve a compactar
-ok un fichero abierto sobrevive a la compactacion
...
```

oraculo (host):

```
ok fragmentado: hay huecos
ok fragmentado: el mayor libre es menor que lo libre
ok compactado: ni un hueco
ok compactado: todo lo libre en un bloque
ok compactado: no se pierde memoria
ok compactado: con su colecta
ok compactado: los objetos no aumentan
ok lo vivo sigue intacto
ok tras compactar se reserva
ok y se colecta
ok y se vuelve a compactar
ok un fichero abierto sobrevive a la compactacion
ok y lo vivo tambien
rechazo: Heap.compact: sólo se puede llamar desde Main
rechazo: Heap.compact: no se puede compactar con otros hilos vivos
ok con el hilo acabado, se compacta
fin
```

placa:

```
ok fragmentado: hay huecos
ok fragmentado: el mayor libre es menor que lo libre
ok compactado: ni un hueco
ok compactado: todo lo libre en un bloque
ok compactado: no se pierde memoria
ok compactado: con su colecta
ok compactado: los objetos no aumentan
ok lo vivo sigue intacto
```
</details>

- tras HeapReposo: uptimeMs=286751 fsUsed=303104 vmHeapBytes=65536
- ejecutadas (con EXITED) 27 de 29 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 290 | b435a8b4 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 527 | 2268f642 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4602 | 748c89ef | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2349 | 3a36e8cb | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3600 | f4f81618 | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2164 | f8904450 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4453 | 21a595f6 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2442 | a0971c86 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9730 | f6cc997c | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMC_ILP32\NatCede.mod | 2320 | 0a18c7cc | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMC_ILP32\NatExpropia.mod | 1161 | abbbccac | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMC_ILP32\NatBuiltin.mod | 2789 | 7ac18801 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMC_ILP32\NatAnida.mod | 1232 | 359b3eec | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMC_ILP32\NatDirecta.mod | 2527 | b0130698 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMC_ILP32\NatCoste.mod | 1000 | 3f859af0 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMC_ILP32\NatNombres.mod | 2712 | 57a14515 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMC_ILP32\NatEstrecha.mod | 3131 | 01c97fbf | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3839 | 5edfd98a | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1097 | 4397d411 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3562 | 40751a0d | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2783 | 44621731 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube

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
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| CatCadena | IDENTICO |
| CatCrono | IDENTICO |
| SbPiezas | IDENTICO |
| SbCrono | IDENTICO |
| HeapInfo | DIFIERE |
| HeapCompact | DIFIERE |
| HeapReposo | IDENTICO |

SALTADA: GuiWinJson en esp32c6@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32c6@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-02 08:13:32

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (32 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-02 08:04:41) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-02 07:40:34) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 336 | 3 |
| MachineId | MachineId | 2b38cfb7 | 484 | ok | 0 | 11 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 8 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 15 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 8 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 10 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 13 | 8 |
| GuiWinJson | GuiWinJson | f74e3271 | 2070 | ok | 0 | 62 | 17 |
| GuiShot | GuiShot | 48635085 | 1344 | ok | 0 | 362 | 8 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | d0a08075 | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 8 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 81 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 8 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 44 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 47af3b73 | 1018 | ok | 0 | 13 | 9 |
| NatCoste | NatCoste | 01da15a7 | 772 | ok | 0 | 14 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 8 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 12 | 23 |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 8 | 24 |
| CatCrono | CatCrono | 3b281178 | 886 | ok | 0 | 12 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | c68dbf62 | 819 | ok | 0 | 10 | 2 |
| HeapInfo | HeapInfo | 93b25231 | 1107 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | c45827d4 | 3577 | ok | 0 | 20 | 17 |
| HeapReposo | HeapReposo | 4a87e0d1 | 2803 | ok | 0 | 57 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 24 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 19 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 8 | 5 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  1 2026 17:54:30` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=118687 fs=307200/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 48 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 336 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 11 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 30 | 15 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 13 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **DIFIERE** | 260 | 50 | texto distinto; estado no casa (host rc=0, EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: No space in heap)) |
| GuiShot | **NO-PETA** | 540 | 367 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |
| NatCede | **IDENTICO** | 60 | 81 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatExpropia | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMC_ILP32) |
| NatBuiltin | **IDENTICO** | 40 | 44 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMC_ILP32) |
| NatAnida | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatDirecta | **IDENTICO** | 0 | 13 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMC_ILP32) |
| NatCoste | **IDENTICO** | 1060 | 14 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatNombres | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| NatEstrecha | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| CatCadena | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 100 | 12 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 120 | 10 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **DIFIERE** | 20 | 9 | texto distinto |
| HeapCompact | **IDENTICO** | 370 | 20 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 330 | 57 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1650 | 24 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 470 | 19 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=119957 fsUsed=307200 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=120472 fsUsed=307200 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=121402 fsUsed=307200 vmHeapBytes=65536
- tras MachineId: uptimeMs=121962 fsUsed=307200 vmHeapBytes=65536
- tras Destructor: uptimeMs=122960 fsUsed=307200 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=124020 fsUsed=307200 vmHeapBytes=65536
- tras ArrayFijo: uptimeMs=124978 fsUsed=307200 vmHeapBytes=65536
- tras MoveRango: uptimeMs=125547 fsUsed=307200 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=126979 fsUsed=307200 vmHeapBytes=65536
- tras GuiWinJson: uptimeMs=138288 fsUsed=405504 vmHeapBytes=65536
<details><summary>GuiWinJson: cola del stderr del host</summary>

```
ap 387984, libre 321 KB
[bpvm] GC por TABLA de handles: 591/654 slots (marca cruzada)
[gc] pila t0: rango [393216..393888) = 168 palabras, 26 son refs al heap
[gc] vivo=2388 liberado=28400 heap=[58368..89156) freelist=si
[gc] reservas: de_lista=0 de_bump=590 astilla=0
[gc] libres: 25 huecos, 28400 B en total, el mayor 13808 | altas=25 fusion_izq=0 fusion_der=0
[bpvm] GC: bump 89156 -> 89156 (recupera 0 B, tope 42700, heap 58368..387984)
[bpvm] pila: el main llegó a 956 de 16384 B
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

- tras GuiShot: uptimeMs=147809 fsUsed=405504 vmHeapBytes=65536
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=148900 fsUsed=405504 vmHeapBytes=65536
- tras NatV7: uptimeMs=149806 fsUsed=405504 vmHeapBytes=65536
- tras FloatNat: uptimeMs=150888 fsUsed=405504 vmHeapBytes=65536
- tras NatCede: uptimeMs=151828 fsUsed=405504 vmHeapBytes=65536
- tras NatExpropia: uptimeMs=152751 fsUsed=405504 vmHeapBytes=65536
- tras NatBuiltin: uptimeMs=153708 fsUsed=405504 vmHeapBytes=65536
- tras NatAnida: uptimeMs=154623 fsUsed=405504 vmHeapBytes=65536
- tras NatDirecta: uptimeMs=155593 fsUsed=405504 vmHeapBytes=65536
- tras NatCoste: uptimeMs=157424 fsUsed=405504 vmHeapBytes=65536
- tras NatNombres: uptimeMs=158765 fsUsed=405504 vmHeapBytes=65536
- tras NatEstrecha: uptimeMs=159678 fsUsed=405504 vmHeapBytes=65536
- tras CatCadena: uptimeMs=160210 fsUsed=405504 vmHeapBytes=65536
- tras CatCrono: uptimeMs=160742 fsUsed=405504 vmHeapBytes=65536
- tras SbPiezas: uptimeMs=161293 fsUsed=405504 vmHeapBytes=65536
- tras SbCrono: uptimeMs=161797 fsUsed=405504 vmHeapBytes=65536
- tras HeapInfo: uptimeMs=162330 fsUsed=405504 vmHeapBytes=65536
<details><summary>HeapInfo: cola del stderr del host</summary>

```
rado=140 heap=[10688..10796) freelist=vacia
[gc] reservas: de_lista=0 de_bump=8 astilla=0
[gc] libres: 0 huecos, 0 B en total, el mayor 0 | altas=0 fusion_izq=0 fusion_der=0
[gc] pila t0: rango [393216..393284) = 17 palabras, 0 son refs al heap
[gc] vivo=108 liberado=19304 heap=[10688..10796) freelist=vacia
[gc] reservas: de_lista=0 de_bump=410 astilla=0
[gc] libres: 0 huecos, 0 B en total, el mayor 0 | altas=0 fusion_izq=0 fusion_der=0
[bpvm] pila: el main llegó a 140 de 16384 B
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 1 vivos)

```
</details>

<details><summary>HeapInfo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -2,7 +2,7 @@
 ok lo libre cabe en el heap
 ok el mayor bloque cabe en lo libre
 ok los huecos se cuentan
-ok doscientos nodos son al menos cuatrocientos objetos mas
+FALLO doscientos nodos son al menos cuatrocientos objetos mas
 ok lo libre baja al reservar
 ok una colecta mas
...
```

oraculo (host):

```
ok el heap mide algo
ok lo libre cabe en el heap
ok el mayor bloque cabe en lo libre
ok los huecos se cuentan
ok doscientos nodos son al menos cuatrocientos objetos mas
ok lo libre baja al reservar
ok una colecta mas
ok tras soltar y colectar, lo libre sube
ok y los objetos bajan
fin
```

placa:

```
ok el heap mide algo
ok lo libre cabe en el heap
ok el mayor bloque cabe en lo libre
ok los huecos se cuentan
FALLO doscientos nodos son al menos cuatrocientos objetos mas
ok lo libre baja al reservar
ok una colecta mas
ok tras soltar y colectar, lo libre sube
ok y los objetos bajan
fin
```
</details>

- tras HeapCompact: uptimeMs=163744 fsUsed=405504 vmHeapBytes=65536
- tras HeapReposo: uptimeMs=165117 fsUsed=405504 vmHeapBytes=65536
- tras DescarteAncho: uptimeMs=167334 fsUsed=409600 vmHeapBytes=65536
- tras TempRetiene: uptimeMs=168673 fsUsed=413696 vmHeapBytes=65536
- tras FinallyAnidado: uptimeMs=169203 fsUsed=417792 vmHeapBytes=65536
- ejecutadas (con EXITED) 32 de 32 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45112 | f99660c9 | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22749 | 620b3c39 | ok (por trozos) |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12305 | 263868cb | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4607 | 9765bb28 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4483 | a4d0e489 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2447 | a7571111 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9740 | 7ad95292 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMC_ILP32\NatCede.mod | 2325 | 898b5cda | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMC_ILP32\NatExpropia.mod | 1166 | 8b6c96ec | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMC_ILP32\NatBuiltin.mod | 2794 | f745c5e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMC_ILP32\NatAnida.mod | 1232 | 359b3eec | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMC_ILP32\NatDirecta.mod | 2527 | b0130698 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMC_ILP32\NatCoste.mod | 1000 | 3f859af0 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMC_ILP32\NatNombres.mod | 2722 | 975650c8 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMC_ILP32\NatEstrecha.mod | 3131 | 01c97fbf | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1107 | 93b25231 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: el RUN la resuelve a pack:GuiMini/Gui.mod (fuera de /lib y /app), crc ae0d2a7e: se sube a /lib
- dependencia Json: el RUN la resuelve a pack:GuiMini/Json.mod (fuera de /lib y /app), crc 578601f2: se sube a /lib
- dependencia Collections: el RUN la resuelve a pack:GuiMini/Collections.mod (fuera de /lib y /app), crc feacb85e: se sube a /lib
- dependencia Str: el RUN la resuelve a pack:GuiMini/Str.mod (fuera de /lib y /app), crc 8e56f753: se sube a /lib
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube

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
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| CatCadena | IDENTICO |
| CatCrono | IDENTICO |
| SbPiezas | IDENTICO |
| SbCrono | IDENTICO |
| HeapInfo | DIFIERE |
| HeapCompact | IDENTICO |
| HeapReposo | IDENTICO |
| DescarteAncho | IDENTICO |
| TempRetiene | IDENTICO |
| FinallyAnidado | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  


## Tanda 2026-10-02 08:25:21

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (32 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-02 08:04:41) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-02 07:40:34) - sabor `gui1-lvgl0`
- placas conectadas: 3 (rp2350a@COM22, esp32p4@COM15, stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 321 | 3 |
| MachineId | MachineId | 2b38cfb7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 16 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 8 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | f74e3271 | 2070 | ok | 0 | 48 | 17 |
| GuiShot | GuiShot | 48635085 | 1344 | ok | 0 | 379 | 8 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 11 | 10 |
| NatV7 | NatV7 | d0a08075 | 1557 | ok | 0 | 14 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 8 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 108 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 52 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 8 | 3 |
| NatDirecta | NatDirecta | 47af3b73 | 1018 | ok | 0 | 10 | 9 |
| NatCoste | NatCoste | 01da15a7 | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 8 | 24 |
| CatCrono | CatCrono | 3b281178 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 8 | 12 |
| SbCrono | SbCrono | c68dbf62 | 819 | ok | 0 | 9 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 8 | 10 |
| HeapCompact | HeapCompact | c45827d4 | 3577 | ok | 0 | 17 | 17 |
| HeapReposo | HeapReposo | 4a87e0d1 | 2803 | ok | 0 | 62 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 21 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 16 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 8 | 5 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  1 2026 17:50:45` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=678145 fs=475136/1048576 B vmHeapBytes=251032
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 66 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 39 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 306 | 321 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 8 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 59 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 50 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 23 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 75 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 33 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 48 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 379 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 28 | 11 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 26 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 68 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 61 | 108 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 13 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 45 | 52 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 19 | 10 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 791 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 11 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 72 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 61 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 97 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 49 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 98 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 38 | 8 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 384 | 17 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 358 | 62 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1502 | 21 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 388 | 16 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 9 | 8 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=678962 fsUsed=475136 vmHeapBytes=251032
- tras ThrowSinAtrapar: uptimeMs=679463 fsUsed=475136 vmHeapBytes=251032
- tras ThreadTrasMain: uptimeMs=680372 fsUsed=475136 vmHeapBytes=251032
- tras MachineId: uptimeMs=680885 fsUsed=475136 vmHeapBytes=251032
- tras Destructor: uptimeMs=681489 fsUsed=475136 vmHeapBytes=251032
- tras TimeDemo: uptimeMs=682454 fsUsed=475136 vmHeapBytes=251032
- tras ArrayFijo: uptimeMs=682972 fsUsed=475136 vmHeapBytes=251032
- tras MoveRango: uptimeMs=683487 fsUsed=475136 vmHeapBytes=251032
- tras HiloUsa: uptimeMs=684054 fsUsed=475136 vmHeapBytes=251032
- tras NatNew: uptimeMs=685082 fsUsed=475136 vmHeapBytes=251032
- tras NatV7: uptimeMs=686036 fsUsed=475136 vmHeapBytes=251032
- tras FloatNat: uptimeMs=687156 fsUsed=475136 vmHeapBytes=251032
- tras NatCede: uptimeMs=688054 fsUsed=475136 vmHeapBytes=251032
- tras NatExpropia: uptimeMs=688906 fsUsed=475136 vmHeapBytes=251032
- tras NatBuiltin: uptimeMs=689859 fsUsed=475136 vmHeapBytes=251032
- tras NatAnida: uptimeMs=690711 fsUsed=475136 vmHeapBytes=251032
- tras NatDirecta: uptimeMs=691582 fsUsed=475136 vmHeapBytes=251032
- tras NatCoste: uptimeMs=693248 fsUsed=475136 vmHeapBytes=251032
- tras NatNombres: uptimeMs=694126 fsUsed=475136 vmHeapBytes=251032
- tras NatEstrecha: uptimeMs=695017 fsUsed=475136 vmHeapBytes=251032
- tras CatCadena: uptimeMs=695531 fsUsed=475136 vmHeapBytes=251032
- tras CatCrono: uptimeMs=696112 fsUsed=475136 vmHeapBytes=251032
- tras SbPiezas: uptimeMs=696616 fsUsed=475136 vmHeapBytes=251032
- tras SbCrono: uptimeMs=697103 fsUsed=475136 vmHeapBytes=251032
- tras HeapInfo: uptimeMs=697634 fsUsed=475136 vmHeapBytes=251032
- tras HeapCompact: uptimeMs=698576 fsUsed=475136 vmHeapBytes=251032
- tras HeapReposo: uptimeMs=699915 fsUsed=475136 vmHeapBytes=251032
- tras DescarteAncho: uptimeMs=702083 fsUsed=479232 vmHeapBytes=251032
- tras TempRetiene: uptimeMs=703008 fsUsed=483328 vmHeapBytes=251032
- tras FinallyAnidado: uptimeMs=703588 fsUsed=487424 vmHeapBytes=251032
- ejecutadas (con EXITED) 30 de 32 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4151 | 0b2e4308 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2295 | 1ca83bfb | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8456 | 3612384a | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2221 | 63be81f6 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1126 | dfa8ab63 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2159 | 042932ad | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | ee360c06 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  1 2026 17:55:04` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=664073 fs=159744/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 321 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 560 | 44 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 520 | 369 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 60 | 108 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 50 | 52 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 540 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatNombres | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| NatEstrecha | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| CatCadena | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 30 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 50 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 140 | 17 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 250 | 62 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 620 | 21 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 190 | 16 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=665126 fsUsed=163840 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=665693 fsUsed=167936 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=666596 fsUsed=172032 vmHeapBytes=24637440
- tras MachineId: uptimeMs=667294 fsUsed=176128 vmHeapBytes=24637440
- tras Destructor: uptimeMs=668629 fsUsed=184320 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=669912 fsUsed=188416 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=670798 fsUsed=192512 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=671898 fsUsed=196608 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=673182 fsUsed=204800 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=689130 fsUsed=311296 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=696352 fsUsed=315392 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=698821 fsUsed=323584 vmHeapBytes=24637440
- tras NatV7: uptimeMs=700190 fsUsed=327680 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=702427 fsUsed=339968 vmHeapBytes=24637440
- tras NatCede: uptimeMs=703726 fsUsed=344064 vmHeapBytes=24637440
- tras NatExpropia: uptimeMs=704883 fsUsed=348160 vmHeapBytes=24637440
- tras NatBuiltin: uptimeMs=706278 fsUsed=352256 vmHeapBytes=24637440
- tras NatAnida: uptimeMs=707532 fsUsed=356352 vmHeapBytes=24637440
- tras NatDirecta: uptimeMs=708797 fsUsed=360448 vmHeapBytes=24637440
- tras NatCoste: uptimeMs=710398 fsUsed=364544 vmHeapBytes=24637440
- tras NatNombres: uptimeMs=711731 fsUsed=368640 vmHeapBytes=24637440
- tras NatEstrecha: uptimeMs=713236 fsUsed=372736 vmHeapBytes=24637440
- tras CatCadena: uptimeMs=714419 fsUsed=376832 vmHeapBytes=24637440
- tras CatCrono: uptimeMs=715204 fsUsed=380928 vmHeapBytes=24637440
- tras SbPiezas: uptimeMs=716302 fsUsed=385024 vmHeapBytes=24637440
- tras SbCrono: uptimeMs=717087 fsUsed=389120 vmHeapBytes=24637440
- tras HeapInfo: uptimeMs=717955 fsUsed=393216 vmHeapBytes=24637440
- tras HeapCompact: uptimeMs=719457 fsUsed=397312 vmHeapBytes=24637440
- tras HeapReposo: uptimeMs=721091 fsUsed=401408 vmHeapBytes=24637440
- tras DescarteAncho: uptimeMs=722709 fsUsed=405504 vmHeapBytes=24637440
- tras TempRetiene: uptimeMs=723860 fsUsed=409600 vmHeapBytes=24637440
- tras FinallyAnidado: uptimeMs=724895 fsUsed=413696 vmHeapBytes=24637440
- ejecutadas (con EXITED) 32 de 32 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45112 | f99660c9 | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22749 | 620b3c39 | ok (por trozos) |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12305 | 263868cb | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4607 | 9765bb28 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4467 | 183a7978 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2447 | 13b1b5cb | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9332 | 2365095d | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2325 | 6f33192a | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1166 | 0f42a895 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMAFC_ILP32F\NatBuiltin.mod | 2790 | 7d97dff4 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMAFC_ILP32F\NatAnida.mod | 1232 | 3b121406 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMAFC_ILP32F\NatDirecta.mod | 2531 | 670e020b | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMAFC_ILP32F\NatCoste.mod | 1000 | 6d904575 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMAFC_ILP32F\NatNombres.mod | 2706 | e84239c1 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMAFC_ILP32F\NatEstrecha.mod | 3131 | 3dac6451 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: el RUN la resuelve a pack:Stdlib/Gui.mod (fuera de /lib y /app), crc 63045d4d: se sube a /lib
- dependencia Json: el RUN la resuelve a pack:Stdlib/Json.mod (fuera de /lib y /app), crc 7d2494c4: se sube a /lib
- dependencia Collections: el RUN la resuelve a pack:Stdlib/Collections.mod (fuera de /lib y /app), crc d16238b4: se sube a /lib
- dependencia Str: el RUN la resuelve a pack:Stdlib/Str.mod (fuera de /lib y /app), crc b52b92b4: se sube a /lib
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  2 2026 07:57:19` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=138553 fs=598016/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 57 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 317 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 115 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 444 | 321 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 206 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 490 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 714 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 286 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 383 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 488 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 4040 | 51 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 4365 | 366 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 315 | 11 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 396 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 431 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 277 | 108 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 221 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 294 | 52 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 195 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 209 | 10 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1402 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 253 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 352 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 345 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 312 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 289 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 380 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 256 | 8 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 988 | 17 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 1024 | 62 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1628 | 21 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 767 | 16 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 238 | 8 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=139819 fsUsed=598016 vmHeapBytes=786432
- tras ThrowSinAtrapar: uptimeMs=140383 fsUsed=598016 vmHeapBytes=786432
- tras ThreadTrasMain: uptimeMs=141387 fsUsed=598016 vmHeapBytes=786432
- tras MachineId: uptimeMs=142000 fsUsed=598016 vmHeapBytes=786432
- tras Destructor: uptimeMs=143402 fsUsed=598016 vmHeapBytes=786432
- tras TimeDemo: uptimeMs=144553 fsUsed=598016 vmHeapBytes=786432
- tras ArrayFijo: uptimeMs=145320 fsUsed=598016 vmHeapBytes=786432
- tras MoveRango: uptimeMs=146220 fsUsed=598016 vmHeapBytes=786432
- tras HiloUsa: uptimeMs=147504 fsUsed=598016 vmHeapBytes=786432
- tras GuiWinJson: uptimeMs=161457 fsUsed=696320 vmHeapBytes=786432
- tras GuiShot: uptimeMs=166964 fsUsed=696320 vmHeapBytes=786432
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=168848 fsUsed=696320 vmHeapBytes=786432
- tras NatV7: uptimeMs=169618 fsUsed=696320 vmHeapBytes=786432
- tras FloatNat: uptimeMs=171418 fsUsed=696320 vmHeapBytes=786432
- tras NatCede: uptimeMs=172183 fsUsed=696320 vmHeapBytes=786432
- tras NatExpropia: uptimeMs=172852 fsUsed=696320 vmHeapBytes=786432
- tras NatBuiltin: uptimeMs=173656 fsUsed=696320 vmHeapBytes=786432
- tras NatAnida: uptimeMs=174334 fsUsed=696320 vmHeapBytes=786432
- tras NatDirecta: uptimeMs=175101 fsUsed=696320 vmHeapBytes=786432
- tras NatCoste: uptimeMs=176998 fsUsed=696320 vmHeapBytes=786432
- tras NatNombres: uptimeMs=177787 fsUsed=696320 vmHeapBytes=786432
- tras NatEstrecha: uptimeMs=178722 fsUsed=696320 vmHeapBytes=786432
- tras CatCadena: uptimeMs=179625 fsUsed=696320 vmHeapBytes=786432
- tras CatCrono: uptimeMs=180256 fsUsed=696320 vmHeapBytes=786432
- tras SbPiezas: uptimeMs=180989 fsUsed=696320 vmHeapBytes=786432
- tras SbCrono: uptimeMs=181621 fsUsed=696320 vmHeapBytes=786432
- tras HeapInfo: uptimeMs=182315 fsUsed=696320 vmHeapBytes=786432
- tras HeapCompact: uptimeMs=184078 fsUsed=696320 vmHeapBytes=786432
- tras HeapReposo: uptimeMs=185717 fsUsed=696320 vmHeapBytes=786432
- tras DescarteAncho: uptimeMs=187666 fsUsed=704512 vmHeapBytes=786432
- tras TempRetiene: uptimeMs=188932 fsUsed=712704 vmHeapBytes=786432
- tras FinallyAnidado: uptimeMs=189684 fsUsed=720896 vmHeapBytes=786432
- ejecutadas (con EXITED) 32 de 32 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45112 | f99660c9 | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22749 | 620b3c39 | ok (por trozos) |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12305 | 263868cb | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4607 | 9765bb28 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4151 | 0b2e4308 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2295 | 1ca83bfb | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8456 | 3612384a | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2221 | 63be81f6 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1126 | dfa8ab63 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2159 | 042932ad | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | ee360c06 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: el RUN la resuelve a pack:Gui/Gui.mod (fuera de /lib y /app), crc ae0d2a7e: se sube a /lib
- dependencia Json: el RUN la resuelve a pack:Gui/Json.mod (fuera de /lib y /app), crc 578601f2: se sube a /lib
- dependencia Collections: el RUN la resuelve a pack:Gui/Collections.mod (fuera de /lib y /app), crc feacb85e: se sube a /lib
- dependencia Str: el RUN la resuelve a pack:Gui/Str.mod (fuera de /lib y /app), crc 8e56f753: se sube a /lib
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | rp2350a@COM22 | esp32p4@COM15 | stm32u5g9@COM12 |
|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA |
| Destructor | IDENTICO | IDENTICO | IDENTICO |
| TimeDemo | IDENTICO | IDENTICO | IDENTICO |
| ArrayFijo | IDENTICO | IDENTICO | IDENTICO |
| MoveRango | IDENTICO | IDENTICO | IDENTICO |
| HiloUsa | IDENTICO | IDENTICO | IDENTICO |
| GuiWinJson | SALTADA | IDENTICO | IDENTICO |
| GuiShot | SALTADA | NO-PETA | NO-PETA |
| NatNew | IDENTICO | IDENTICO | IDENTICO |
| NatV7 | IDENTICO | IDENTICO | IDENTICO |
| FloatNat | IDENTICO | IDENTICO | IDENTICO |
| NatCede | IDENTICO | IDENTICO | IDENTICO |
| NatExpropia | IDENTICO | IDENTICO | IDENTICO |
| NatBuiltin | IDENTICO | IDENTICO | IDENTICO |
| NatAnida | IDENTICO | IDENTICO | IDENTICO |
| NatDirecta | IDENTICO | IDENTICO | IDENTICO |
| NatCoste | IDENTICO | IDENTICO | IDENTICO |
| NatNombres | IDENTICO | IDENTICO | IDENTICO |
| NatEstrecha | IDENTICO | IDENTICO | IDENTICO |
| CatCadena | IDENTICO | IDENTICO | IDENTICO |
| CatCrono | IDENTICO | IDENTICO | IDENTICO |
| SbPiezas | IDENTICO | IDENTICO | IDENTICO |
| SbCrono | IDENTICO | IDENTICO | IDENTICO |
| HeapInfo | IDENTICO | IDENTICO | IDENTICO |
| HeapCompact | IDENTICO | IDENTICO | IDENTICO |
| HeapReposo | IDENTICO | IDENTICO | IDENTICO |
| DescarteAncho | IDENTICO | IDENTICO | IDENTICO |
| TempRetiene | IDENTICO | IDENTICO | IDENTICO |
| FinallyAnidado | IDENTICO | IDENTICO | IDENTICO |

SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  


## Tanda 2026-10-02 08:30:41

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (32 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-02 08:04:41) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-02 07:40:34) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 327 | 3 |
| MachineId | MachineId | 2b38cfb7 | 484 | ok | 0 | 12 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 11 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 15 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 8 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 10 | 8 |
| GuiWinJson | GuiWinJson | f74e3271 | 2070 | ok | 0 | 47 | 17 |
| GuiShot | GuiShot | 48635085 | 1344 | ok | 0 | 353 | 8 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 8 | 10 |
| NatV7 | NatV7 | d0a08075 | 1557 | ok | 0 | 13 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 8 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 95 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 47 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 8 | 3 |
| NatDirecta | NatDirecta | 47af3b73 | 1018 | ok | 0 | 8 | 9 |
| NatCoste | NatCoste | 01da15a7 | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 8 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 10 | 23 |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 8 | 24 |
| CatCrono | CatCrono | 3b281178 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | c68dbf62 | 819 | ok | 0 | 9 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | c45827d4 | 3577 | ok | 0 | 17 | 17 |
| HeapReposo | HeapReposo | 4a87e0d1 | 2803 | ok | 0 | 60 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 23 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 17 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 8 | 5 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  1 2026 17:54:30` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=194422 fs=294912/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 50 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 15 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 10 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 770 | 46 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 490 | 360 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |
| NatCede | **IDENTICO** | 60 | 95 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatExpropia | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMC_ILP32) |
| NatBuiltin | **IDENTICO** | 40 | 47 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMC_ILP32) |
| NatAnida | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatDirecta | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMC_ILP32) |
| NatCoste | **IDENTICO** | 1050 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatNombres | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| NatEstrecha | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| CatCadena | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 90 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 120 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 370 | 17 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 410 | 60 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1680 | 23 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 510 | 17 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=195527 fsUsed=294912 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=196076 fsUsed=294912 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=197044 fsUsed=294912 vmHeapBytes=65536
- tras MachineId: uptimeMs=197644 fsUsed=294912 vmHeapBytes=65536
- tras Destructor: uptimeMs=198663 fsUsed=294912 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=199715 fsUsed=294912 vmHeapBytes=65536
- tras ArrayFijo: uptimeMs=200547 fsUsed=294912 vmHeapBytes=65536
- tras MoveRango: uptimeMs=201066 fsUsed=294912 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=202032 fsUsed=294912 vmHeapBytes=65536
- tras GuiWinJson: uptimeMs=204717 fsUsed=294912 vmHeapBytes=65536
- tras GuiShot: uptimeMs=206751 fsUsed=294912 vmHeapBytes=65536
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=207854 fsUsed=294912 vmHeapBytes=65536
- tras NatV7: uptimeMs=208755 fsUsed=294912 vmHeapBytes=65536
- tras FloatNat: uptimeMs=209839 fsUsed=294912 vmHeapBytes=65536
- tras NatCede: uptimeMs=210754 fsUsed=294912 vmHeapBytes=65536
- tras NatExpropia: uptimeMs=211891 fsUsed=294912 vmHeapBytes=65536
- tras NatBuiltin: uptimeMs=212777 fsUsed=294912 vmHeapBytes=65536
- tras NatAnida: uptimeMs=213643 fsUsed=294912 vmHeapBytes=65536
- tras NatDirecta: uptimeMs=214577 fsUsed=294912 vmHeapBytes=65536
- tras NatCoste: uptimeMs=216677 fsUsed=294912 vmHeapBytes=65536
- tras NatNombres: uptimeMs=217612 fsUsed=294912 vmHeapBytes=65536
- tras NatEstrecha: uptimeMs=218530 fsUsed=294912 vmHeapBytes=65536
- tras CatCadena: uptimeMs=219114 fsUsed=294912 vmHeapBytes=65536
- tras CatCrono: uptimeMs=219663 fsUsed=294912 vmHeapBytes=65536
- tras SbPiezas: uptimeMs=220215 fsUsed=294912 vmHeapBytes=65536
- tras SbCrono: uptimeMs=220833 fsUsed=294912 vmHeapBytes=65536
- tras HeapInfo: uptimeMs=221666 fsUsed=294912 vmHeapBytes=65536
- tras HeapCompact: uptimeMs=223068 fsUsed=294912 vmHeapBytes=65536
- tras HeapReposo: uptimeMs=224450 fsUsed=294912 vmHeapBytes=65536
- tras DescarteAncho: uptimeMs=226620 fsUsed=294912 vmHeapBytes=65536
- tras TempRetiene: uptimeMs=227572 fsUsed=294912 vmHeapBytes=65536
- tras FinallyAnidado: uptimeMs=228089 fsUsed=294912 vmHeapBytes=65536
- ejecutadas (con EXITED) 32 de 32 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4483 | a4d0e489 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2447 | a7571111 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9740 | 7ad95292 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMC_ILP32\NatCede.mod | 2325 | 898b5cda | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMC_ILP32\NatExpropia.mod | 1166 | 8b6c96ec | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMC_ILP32\NatBuiltin.mod | 2794 | f745c5e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMC_ILP32\NatAnida.mod | 1232 | 359b3eec | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMC_ILP32\NatDirecta.mod | 2527 | b0130698 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMC_ILP32\NatCoste.mod | 1000 | 3f859af0 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMC_ILP32\NatNombres.mod | 2722 | 975650c8 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMC_ILP32\NatEstrecha.mod | 3131 | 01c97fbf | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:GuiMini/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:GuiMini/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:GuiMini/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:GuiMini/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:GuiMini/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube

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
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| CatCadena | IDENTICO |
| CatCrono | IDENTICO |
| SbPiezas | IDENTICO |
| SbCrono | IDENTICO |
| HeapInfo | IDENTICO |
| HeapCompact | IDENTICO |
| HeapReposo | IDENTICO |
| DescarteAncho | IDENTICO |
| TempRetiene | IDENTICO |
| FinallyAnidado | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  


## Tanda 2026-10-02 10:37:48

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (32 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-02 10:04:54) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-02 07:40:34) - sabor `gui1-lvgl0`
- placas conectadas: 4 (esp32c6@COM3, esp32p4@COM15, rp2350a@COM22, stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 13 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 326 | 3 |
| MachineId | MachineId | 2b38cfb7 | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 11 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 8 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 23 | 8 |
| GuiWinJson | GuiWinJson | f74e3271 | 2070 | ok | 0 | 52 | 17 |
| GuiShot | GuiShot | 48635085 | 1344 | ok | 0 | 376 | 8 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | d0a08075 | 1557 | ok | 0 | 9 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 11 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 86 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 8 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 49 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 8 | 3 |
| NatDirecta | NatDirecta | 47af3b73 | 1018 | ok | 0 | 9 | 9 |
| NatCoste | NatCoste | 01da15a7 | 772 | ok | 0 | 11 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 8 | 24 |
| CatCrono | CatCrono | 3b281178 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | c68dbf62 | 819 | ok | 0 | 9 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | c45827d4 | 3577 | ok | 0 | 18 | 17 |
| HeapReposo | HeapReposo | 4a87e0d1 | 2803 | ok | 0 | 66 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 21 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 14 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 8 | 5 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  1 2026 17:54:30` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=132698 fs=294912/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 50 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 13 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 326 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 30 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 23 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 750 | 47 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 480 | 372 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 0 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |
| NatCede | **IDENTICO** | 60 | 86 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatExpropia | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMC_ILP32) |
| NatBuiltin | **IDENTICO** | 40 | 49 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMC_ILP32) |
| NatAnida | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatDirecta | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMC_ILP32) |
| NatCoste | **IDENTICO** | 1060 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatNombres | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| NatEstrecha | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| CatCadena | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 90 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 120 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 360 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 360 | 66 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1710 | 21 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 490 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=133666 fsUsed=294912 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=134199 fsUsed=294912 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=135182 fsUsed=294912 vmHeapBytes=65536
- tras MachineId: uptimeMs=135767 fsUsed=294912 vmHeapBytes=65536
- tras Destructor: uptimeMs=136602 fsUsed=294912 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=137183 fsUsed=294912 vmHeapBytes=65536
- tras ArrayFijo: uptimeMs=137702 fsUsed=294912 vmHeapBytes=65536
- tras MoveRango: uptimeMs=138253 fsUsed=294912 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=139220 fsUsed=294912 vmHeapBytes=65536
- tras GuiWinJson: uptimeMs=141348 fsUsed=294912 vmHeapBytes=65536
- tras GuiShot: uptimeMs=143375 fsUsed=294912 vmHeapBytes=65536
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=144524 fsUsed=294912 vmHeapBytes=65536
- tras NatV7: uptimeMs=145426 fsUsed=294912 vmHeapBytes=65536
- tras FloatNat: uptimeMs=146761 fsUsed=294912 vmHeapBytes=65536
- tras NatCede: uptimeMs=147629 fsUsed=294912 vmHeapBytes=65536
- tras NatExpropia: uptimeMs=148480 fsUsed=294912 vmHeapBytes=65536
- tras NatBuiltin: uptimeMs=149396 fsUsed=294912 vmHeapBytes=65536
- tras NatAnida: uptimeMs=150264 fsUsed=294912 vmHeapBytes=65536
- tras NatDirecta: uptimeMs=151215 fsUsed=294912 vmHeapBytes=65536
- tras NatCoste: uptimeMs=152974 fsUsed=294912 vmHeapBytes=65536
- tras NatNombres: uptimeMs=153885 fsUsed=294912 vmHeapBytes=65536
- tras NatEstrecha: uptimeMs=154836 fsUsed=294912 vmHeapBytes=65536
- tras CatCadena: uptimeMs=155417 fsUsed=294912 vmHeapBytes=65536
- tras CatCrono: uptimeMs=155969 fsUsed=294912 vmHeapBytes=65536
- tras SbPiezas: uptimeMs=156755 fsUsed=294912 vmHeapBytes=65536
- tras SbCrono: uptimeMs=157237 fsUsed=294912 vmHeapBytes=65536
- tras HeapInfo: uptimeMs=157787 fsUsed=294912 vmHeapBytes=65536
- tras HeapCompact: uptimeMs=159173 fsUsed=294912 vmHeapBytes=65536
- tras HeapReposo: uptimeMs=160539 fsUsed=294912 vmHeapBytes=65536
- tras DescarteAncho: uptimeMs=162742 fsUsed=294912 vmHeapBytes=65536
- tras TempRetiene: uptimeMs=164143 fsUsed=294912 vmHeapBytes=65536
- tras FinallyAnidado: uptimeMs=164694 fsUsed=294912 vmHeapBytes=65536
- ejecutadas (con EXITED) 32 de 32 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4483 | a4d0e489 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2447 | a7571111 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9740 | 7ad95292 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMC_ILP32\NatCede.mod | 2325 | 898b5cda | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMC_ILP32\NatExpropia.mod | 1166 | 8b6c96ec | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMC_ILP32\NatBuiltin.mod | 2794 | f745c5e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMC_ILP32\NatAnida.mod | 1232 | 359b3eec | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMC_ILP32\NatDirecta.mod | 2527 | b0130698 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMC_ILP32\NatCoste.mod | 1000 | 3f859af0 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMC_ILP32\NatNombres.mod | 2722 | 975650c8 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMC_ILP32\NatEstrecha.mod | 3131 | 01c97fbf | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:GuiMini/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:GuiMini/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:GuiMini/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:GuiMini/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:GuiMini/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  1 2026 17:55:04` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=116843 fs=413696/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 43 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 326 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 23 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 570 | 47 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 500 | 368 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 70 | 86 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 50 | 49 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 550 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatNombres | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| NatEstrecha | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| CatCadena | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 40 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 60 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 150 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 180 | 66 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 620 | 21 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 180 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=118243 fsUsed=413696 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=118825 fsUsed=413696 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=119760 fsUsed=413696 vmHeapBytes=24637440
- tras MachineId: uptimeMs=120460 fsUsed=413696 vmHeapBytes=24637440
- tras Destructor: uptimeMs=121880 fsUsed=413696 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=123396 fsUsed=413696 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=124380 fsUsed=413696 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=125562 fsUsed=413696 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=126914 fsUsed=413696 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=133072 fsUsed=413696 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=139560 fsUsed=413696 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=142010 fsUsed=413696 vmHeapBytes=24637440
- tras NatV7: uptimeMs=143378 fsUsed=413696 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=145613 fsUsed=413696 vmHeapBytes=24637440
- tras NatCede: uptimeMs=146914 fsUsed=413696 vmHeapBytes=24637440
- tras NatExpropia: uptimeMs=148015 fsUsed=413696 vmHeapBytes=24637440
- tras NatBuiltin: uptimeMs=149400 fsUsed=413696 vmHeapBytes=24637440
- tras NatAnida: uptimeMs=150517 fsUsed=413696 vmHeapBytes=24637440
- tras NatDirecta: uptimeMs=151968 fsUsed=413696 vmHeapBytes=24637440
- tras NatCoste: uptimeMs=153552 fsUsed=413696 vmHeapBytes=24637440
- tras NatNombres: uptimeMs=154838 fsUsed=413696 vmHeapBytes=24637440
- tras NatEstrecha: uptimeMs=156289 fsUsed=413696 vmHeapBytes=24637440
- tras CatCadena: uptimeMs=157423 fsUsed=413696 vmHeapBytes=24637440
- tras CatCrono: uptimeMs=158140 fsUsed=413696 vmHeapBytes=24637440
- tras SbPiezas: uptimeMs=159075 fsUsed=413696 vmHeapBytes=24637440
- tras SbCrono: uptimeMs=159875 fsUsed=413696 vmHeapBytes=24637440
- tras HeapInfo: uptimeMs=160743 fsUsed=413696 vmHeapBytes=24637440
- tras HeapCompact: uptimeMs=162244 fsUsed=413696 vmHeapBytes=24637440
- tras HeapReposo: uptimeMs=163805 fsUsed=413696 vmHeapBytes=24637440
- tras DescarteAncho: uptimeMs=165583 fsUsed=413696 vmHeapBytes=24637440
- tras TempRetiene: uptimeMs=167062 fsUsed=413696 vmHeapBytes=24637440
- tras FinallyAnidado: uptimeMs=167949 fsUsed=413696 vmHeapBytes=24637440
- ejecutadas (con EXITED) 32 de 32 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4467 | 183a7978 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2447 | 13b1b5cb | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9332 | 2365095d | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2325 | 6f33192a | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1166 | 0f42a895 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMAFC_ILP32F\NatBuiltin.mod | 2790 | 7d97dff4 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMAFC_ILP32F\NatAnida.mod | 1232 | 3b121406 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMAFC_ILP32F\NatDirecta.mod | 2531 | 670e020b | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMAFC_ILP32F\NatCoste.mod | 1000 | 6d904575 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMAFC_ILP32F\NatNombres.mod | 2706 | e84239c1 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMAFC_ILP32F\NatEstrecha.mod | 3131 | 3dac6451 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: ya en /lib/Gui.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Json: ya en /lib/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  1 2026 17:50:45` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=154002 fs=487424/1048576 B vmHeapBytes=251032
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 69 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 40 | 13 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 8 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 307 | 326 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 9 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 60 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 47 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 22 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 73 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 33 | 23 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 52 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 376 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 28 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 28 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 65 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 63 | 86 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 12 | 8 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 43 | 49 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 18 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 791 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 13 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 72 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 59 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 101 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 48 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 100 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 40 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 408 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 368 | 66 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1507 | 21 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 397 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=154805 fsUsed=487424 vmHeapBytes=251032
- tras ThrowSinAtrapar: uptimeMs=155327 fsUsed=487424 vmHeapBytes=251032
- tras ThreadTrasMain: uptimeMs=156270 fsUsed=487424 vmHeapBytes=251032
- tras MachineId: uptimeMs=156821 fsUsed=487424 vmHeapBytes=251032
- tras Destructor: uptimeMs=157437 fsUsed=487424 vmHeapBytes=251032
- tras TimeDemo: uptimeMs=158402 fsUsed=487424 vmHeapBytes=251032
- tras ArrayFijo: uptimeMs=158918 fsUsed=487424 vmHeapBytes=251032
- tras MoveRango: uptimeMs=159536 fsUsed=487424 vmHeapBytes=251032
- tras HiloUsa: uptimeMs=160123 fsUsed=487424 vmHeapBytes=251032
- tras NatNew: uptimeMs=161130 fsUsed=487424 vmHeapBytes=251032
- tras NatV7: uptimeMs=161987 fsUsed=487424 vmHeapBytes=251032
- tras FloatNat: uptimeMs=163092 fsUsed=487424 vmHeapBytes=251032
- tras NatCede: uptimeMs=163945 fsUsed=487424 vmHeapBytes=251032
- tras NatExpropia: uptimeMs=164792 fsUsed=487424 vmHeapBytes=251032
- tras NatBuiltin: uptimeMs=165681 fsUsed=487424 vmHeapBytes=251032
- tras NatAnida: uptimeMs=166541 fsUsed=487424 vmHeapBytes=251032
- tras NatDirecta: uptimeMs=167460 fsUsed=487424 vmHeapBytes=251032
- tras NatCoste: uptimeMs=169246 fsUsed=487424 vmHeapBytes=251032
- tras NatNombres: uptimeMs=170157 fsUsed=487424 vmHeapBytes=251032
- tras NatEstrecha: uptimeMs=171037 fsUsed=487424 vmHeapBytes=251032
- tras CatCadena: uptimeMs=171575 fsUsed=487424 vmHeapBytes=251032
- tras CatCrono: uptimeMs=172099 fsUsed=487424 vmHeapBytes=251032
- tras SbPiezas: uptimeMs=172635 fsUsed=487424 vmHeapBytes=251032
- tras SbCrono: uptimeMs=173152 fsUsed=487424 vmHeapBytes=251032
- tras HeapInfo: uptimeMs=173684 fsUsed=487424 vmHeapBytes=251032
- tras HeapCompact: uptimeMs=175069 fsUsed=487424 vmHeapBytes=251032
- tras HeapReposo: uptimeMs=176415 fsUsed=487424 vmHeapBytes=251032
- tras DescarteAncho: uptimeMs=178590 fsUsed=487424 vmHeapBytes=251032
- tras TempRetiene: uptimeMs=179592 fsUsed=487424 vmHeapBytes=251032
- tras FinallyAnidado: uptimeMs=180091 fsUsed=487424 vmHeapBytes=251032
- ejecutadas (con EXITED) 30 de 32 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4151 | 0b2e4308 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2295 | 1ca83bfb | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8456 | 3612384a | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2221 | 63be81f6 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1126 | dfa8ab63 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2159 | 042932ad | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | ee360c06 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  2 2026 10:21:20` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=174644 fs=622592/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 60 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 363 | 13 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 157 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 497 | 326 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 274 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 594 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 945 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 369 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 446 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 640 | 23 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 997 | 55 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 1109 | 376 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 334 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 421 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 442 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 298 | 86 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 236 | 8 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 313 | 49 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 209 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 220 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1421 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 276 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 397 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 399 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 363 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 352 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 449 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 309 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 1177 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 748 | 66 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1566 | 21 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 691 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 177 | 8 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=176044 fsUsed=622592 vmHeapBytes=786432
- tras ThrowSinAtrapar: uptimeMs=176629 fsUsed=622592 vmHeapBytes=786432
- tras ThreadTrasMain: uptimeMs=177646 fsUsed=622592 vmHeapBytes=786432
- tras MachineId: uptimeMs=178261 fsUsed=622592 vmHeapBytes=786432
- tras Destructor: uptimeMs=179691 fsUsed=622592 vmHeapBytes=786432
- tras TimeDemo: uptimeMs=181279 fsUsed=622592 vmHeapBytes=786432
- tras ArrayFijo: uptimeMs=182077 fsUsed=622592 vmHeapBytes=786432
- tras MoveRango: uptimeMs=183395 fsUsed=622592 vmHeapBytes=786432
- tras HiloUsa: uptimeMs=184697 fsUsed=622592 vmHeapBytes=786432
- tras GuiWinJson: uptimeMs=187478 fsUsed=622592 vmHeapBytes=786432
- tras GuiShot: uptimeMs=189714 fsUsed=622592 vmHeapBytes=786432
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=191594 fsUsed=622592 vmHeapBytes=786432
- tras NatV7: uptimeMs=192776 fsUsed=622592 vmHeapBytes=786432
- tras FloatNat: uptimeMs=194565 fsUsed=622592 vmHeapBytes=786432
- tras NatCede: uptimeMs=195313 fsUsed=622592 vmHeapBytes=786432
- tras NatExpropia: uptimeMs=195976 fsUsed=622592 vmHeapBytes=786432
- tras NatBuiltin: uptimeMs=196777 fsUsed=622592 vmHeapBytes=786432
- tras NatAnida: uptimeMs=197431 fsUsed=622592 vmHeapBytes=786432
- tras NatDirecta: uptimeMs=198181 fsUsed=622592 vmHeapBytes=786432
- tras NatCoste: uptimeMs=200064 fsUsed=622592 vmHeapBytes=786432
- tras NatNombres: uptimeMs=200848 fsUsed=622592 vmHeapBytes=786432
- tras NatEstrecha: uptimeMs=201694 fsUsed=622592 vmHeapBytes=786432
- tras CatCadena: uptimeMs=202615 fsUsed=622592 vmHeapBytes=786432
- tras CatCrono: uptimeMs=203248 fsUsed=622592 vmHeapBytes=786432
- tras SbPiezas: uptimeMs=203994 fsUsed=622592 vmHeapBytes=786432
- tras SbCrono: uptimeMs=205052 fsUsed=622592 vmHeapBytes=786432
- tras HeapInfo: uptimeMs=205751 fsUsed=622592 vmHeapBytes=786432
- tras HeapCompact: uptimeMs=207535 fsUsed=622592 vmHeapBytes=786432
- tras HeapReposo: uptimeMs=208755 fsUsed=622592 vmHeapBytes=786432
- tras DescarteAncho: uptimeMs=210705 fsUsed=622592 vmHeapBytes=786432
- tras TempRetiene: uptimeMs=211956 fsUsed=622592 vmHeapBytes=786432
- tras FinallyAnidado: uptimeMs=212691 fsUsed=622592 vmHeapBytes=786432
- ejecutadas (con EXITED) 32 de 32 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 2b38cfb7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4151 | 0b2e4308 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2295 | 1ca83bfb | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8456 | 3612384a | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2221 | 63be81f6 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1126 | dfa8ab63 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2159 | 042932ad | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | ee360c06 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 3b281178 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | c68dbf62 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: DESFASE de compilación —la placa tiene en pack:Gui/Gui.mod la versión 7 (>= la mínima 7), crc ae0d2a7e != el de hoy f99660c9: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Json: DESFASE de compilación —la placa tiene en pack:Gui/Json.mod la versión 7 (>= la mínima 7), crc 578601f2 != el de hoy 620b3c39: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Collections: DESFASE de compilación —la placa tiene en pack:Gui/Collections.mod la versión 7 (>= la mínima 7), crc feacb85e != el de hoy 263868cb: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Str: DESFASE de compilación —la placa tiene en pack:Gui/Str.mod la versión 7 (>= la mínima 7), crc 8e56f753 != el de hoy 9765bb28: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | esp32c6@COM3 | esp32p4@COM15 | rp2350a@COM22 | stm32u5g9@COM12 |
|---|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA | NO-PETA |
| Destructor | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| TimeDemo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ArrayFijo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MoveRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HiloUsa | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| GuiWinJson | IDENTICO | IDENTICO | SALTADA | IDENTICO |
| GuiShot | NO-PETA | NO-PETA | SALTADA | NO-PETA |
| NatNew | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatV7 | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| FloatNat | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatCede | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatExpropia | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatBuiltin | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatAnida | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatDirecta | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatCoste | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatNombres | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatEstrecha | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| CatCadena | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| CatCrono | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| SbPiezas | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| SbCrono | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HeapInfo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HeapCompact | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HeapReposo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| DescarteAncho | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| TempRetiene | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| FinallyAnidado | IDENTICO | IDENTICO | IDENTICO | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  
NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  


## Tanda 2026-10-02 12:37:40

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (33 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-02 11:45:49) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-02 12:02:45) - sabor `gui1-lvgl0`
- placas conectadas: 4 (esp32c6@COM3, esp32p4@COM15, rp2350a@COM22, stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 318 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 14 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 18 | 8 |
| GuiWinJson | GuiWinJson | f74e3271 | 2070 | ok | 0 | 60 | 17 |
| GuiShot | GuiShot | 48635085 | 1344 | ok | 0 | 364 | 8 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 8 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 8 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 92 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 55 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 47af3b73 | 1018 | ok | 0 | 14 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 8 | 23 |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 8 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 8 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 9 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 10 | 10 |
| HeapCompact | HeapCompact | c45827d4 | 3577 | ok | 0 | 19 | 17 |
| HeapReposo | HeapReposo | 4a87e0d1 | 2803 | ok | 0 | 63 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 28 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 14 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 8 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 9 | 7 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  2 2026 11:59:24` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=80472 fs=102400/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 318 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 18 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 770 | 48 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 540 | 366 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 0 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |
| NatCede | **IDENTICO** | 60 | 92 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatExpropia | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMC_ILP32) |
| NatBuiltin | **IDENTICO** | 50 | 55 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMC_ILP32) |
| NatAnida | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatDirecta | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMC_ILP32) |
| NatCoste | **IDENTICO** | 1060 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatNombres | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| NatEstrecha | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| CatCadena | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 90 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 110 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 20 | 10 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 370 | 19 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 320 | 63 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1700 | 28 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 490 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=81295 fsUsed=106496 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=81779 fsUsed=110592 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=82728 fsUsed=114688 vmHeapBytes=65536
- tras MachineId: uptimeMs=83230 fsUsed=118784 vmHeapBytes=65536
- tras Destructor: uptimeMs=83762 fsUsed=126976 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=85532 fsUsed=131072 vmHeapBytes=65536
- tras ArrayFijo: uptimeMs=86033 fsUsed=135168 vmHeapBytes=65536
- tras MoveRango: uptimeMs=86534 fsUsed=139264 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=87517 fsUsed=147456 vmHeapBytes=65536
- tras GuiWinJson: uptimeMs=90038 fsUsed=155648 vmHeapBytes=65536
- tras GuiShot: uptimeMs=92054 fsUsed=159744 vmHeapBytes=65536
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=93057 fsUsed=167936 vmHeapBytes=65536
- tras NatV7: uptimeMs=93939 fsUsed=172032 vmHeapBytes=65536
- tras FloatNat: uptimeMs=94973 fsUsed=184320 vmHeapBytes=65536
- tras NatCede: uptimeMs=95861 fsUsed=188416 vmHeapBytes=65536
- tras NatExpropia: uptimeMs=96708 fsUsed=192512 vmHeapBytes=65536
- tras NatBuiltin: uptimeMs=97725 fsUsed=196608 vmHeapBytes=65536
- tras NatAnida: uptimeMs=98591 fsUsed=200704 vmHeapBytes=65536
- tras NatDirecta: uptimeMs=99462 fsUsed=204800 vmHeapBytes=65536
- tras NatCoste: uptimeMs=101184 fsUsed=208896 vmHeapBytes=65536
- tras NatNombres: uptimeMs=102094 fsUsed=212992 vmHeapBytes=65536
- tras NatEstrecha: uptimeMs=103014 fsUsed=217088 vmHeapBytes=65536
- tras CatCadena: uptimeMs=103531 fsUsed=221184 vmHeapBytes=65536
- tras CatCrono: uptimeMs=104182 fsUsed=225280 vmHeapBytes=65536
- tras SbPiezas: uptimeMs=104682 fsUsed=229376 vmHeapBytes=65536
- tras SbCrono: uptimeMs=105164 fsUsed=233472 vmHeapBytes=65536
- tras HeapInfo: uptimeMs=105666 fsUsed=237568 vmHeapBytes=65536
- tras HeapCompact: uptimeMs=106635 fsUsed=241664 vmHeapBytes=65536
- tras HeapReposo: uptimeMs=107644 fsUsed=245760 vmHeapBytes=65536
- tras DescarteAncho: uptimeMs=109870 fsUsed=249856 vmHeapBytes=65536
- tras TempRetiene: uptimeMs=111072 fsUsed=253952 vmHeapBytes=65536
- tras FinallyAnidado: uptimeMs=111572 fsUsed=258048 vmHeapBytes=65536
- tras RtcEnHora: uptimeMs=112492 fsUsed=262144 vmHeapBytes=65536
- ejecutadas (con EXITED) 33 de 33 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 6670f12a | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4483 | a4d0e489 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2447 | 7def5e1d | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9740 | 7ad95292 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMC_ILP32\NatCede.mod | 2325 | 898b5cda | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMC_ILP32\NatExpropia.mod | 1166 | 8b6c96ec | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMC_ILP32\NatBuiltin.mod | 2794 | f745c5e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMC_ILP32\NatAnida.mod | 1232 | 359b3eec | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMC_ILP32\NatDirecta.mod | 2527 | b0130698 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMC_ILP32\NatCoste.mod | 1000 | 27be448c | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMC_ILP32\NatNombres.mod | 2722 | 975650c8 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMC_ILP32\NatEstrecha.mod | 3131 | 01c97fbf | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:GuiMini/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:GuiMini/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:GuiMini/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:GuiMini/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:GuiMini/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  2 2026 11:59:56` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=111733 fs=266240/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 12 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 318 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 18 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 580 | 52 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 580 | 369 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 60 | 92 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 50 | 55 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 560 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatNombres | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| NatEstrecha | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| CatCadena | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 40 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 60 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 150 | 19 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 180 | 63 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 630 | 28 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 180 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=112961 fsUsed=270336 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=113646 fsUsed=274432 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=114800 fsUsed=278528 vmHeapBytes=24637440
- tras MachineId: uptimeMs=115582 fsUsed=282624 vmHeapBytes=24637440
- tras Destructor: uptimeMs=117081 fsUsed=290816 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=119100 fsUsed=294912 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=120135 fsUsed=299008 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=121352 fsUsed=303104 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=122804 fsUsed=311296 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=131215 fsUsed=319488 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=139684 fsUsed=323584 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=142021 fsUsed=331776 vmHeapBytes=24637440
- tras NatV7: uptimeMs=143305 fsUsed=335872 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=145440 fsUsed=348160 vmHeapBytes=24637440
- tras NatCede: uptimeMs=146640 fsUsed=352256 vmHeapBytes=24637440
- tras NatExpropia: uptimeMs=147758 fsUsed=356352 vmHeapBytes=24637440
- tras NatBuiltin: uptimeMs=148942 fsUsed=360448 vmHeapBytes=24637440
- tras NatAnida: uptimeMs=149927 fsUsed=364544 vmHeapBytes=24637440
- tras NatDirecta: uptimeMs=151095 fsUsed=368640 vmHeapBytes=24637440
- tras NatCoste: uptimeMs=152646 fsUsed=372736 vmHeapBytes=24637440
- tras NatNombres: uptimeMs=153861 fsUsed=376832 vmHeapBytes=24637440
- tras NatEstrecha: uptimeMs=155264 fsUsed=380928 vmHeapBytes=24637440
- tras CatCadena: uptimeMs=156433 fsUsed=385024 vmHeapBytes=24637440
- tras CatCrono: uptimeMs=157051 fsUsed=389120 vmHeapBytes=24637440
- tras SbPiezas: uptimeMs=157834 fsUsed=393216 vmHeapBytes=24637440
- tras SbCrono: uptimeMs=158535 fsUsed=397312 vmHeapBytes=24637440
- tras HeapInfo: uptimeMs=159268 fsUsed=401408 vmHeapBytes=24637440
- tras HeapCompact: uptimeMs=160536 fsUsed=405504 vmHeapBytes=24637440
- tras HeapReposo: uptimeMs=161871 fsUsed=409600 vmHeapBytes=24637440
- tras DescarteAncho: uptimeMs=163674 fsUsed=413696 vmHeapBytes=24637440
- tras TempRetiene: uptimeMs=164824 fsUsed=417792 vmHeapBytes=24637440
- tras FinallyAnidado: uptimeMs=165639 fsUsed=421888 vmHeapBytes=24637440
- tras RtcEnHora: uptimeMs=166724 fsUsed=425984 vmHeapBytes=24637440
- ejecutadas (con EXITED) 33 de 33 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 6670f12a | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4467 | 183a7978 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2447 | c909fac7 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9332 | 2365095d | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2325 | 6f33192a | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1166 | 0f42a895 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMAFC_ILP32F\NatBuiltin.mod | 2790 | 7d97dff4 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMAFC_ILP32F\NatAnida.mod | 1232 | 3b121406 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMAFC_ILP32F\NatDirecta.mod | 2531 | 670e020b | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMAFC_ILP32F\NatCoste.mod | 1000 | 75ab9b09 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMAFC_ILP32F\NatNombres.mod | 2706 | e84239c1 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMAFC_ILP32F\NatEstrecha.mod | 3131 | 3dac6451 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: ya en /lib/Gui.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Json: ya en /lib/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  2 2026 11:57:27` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=169692 fs=176128/1048576 B vmHeapBytes=251024
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 19 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 39 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 9 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 306 | 318 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 8 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 60 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 48 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 23 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 73 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 35 | 18 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 60 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 364 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 28 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 28 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 66 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 62 | 92 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 13 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 45 | 55 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 11 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 18 | 14 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 789 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 12 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 74 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 62 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 79 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 50 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 101 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 41 | 10 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 382 | 19 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 365 | 63 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1425 | 28 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 392 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 8 | 8 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 29 | 9 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=170308 fsUsed=180224 vmHeapBytes=251024
- tras ThrowSinAtrapar: uptimeMs=170824 fsUsed=184320 vmHeapBytes=251024
- tras ThreadTrasMain: uptimeMs=171718 fsUsed=188416 vmHeapBytes=251024
- tras MachineId: uptimeMs=172211 fsUsed=192512 vmHeapBytes=251024
- tras Destructor: uptimeMs=172789 fsUsed=200704 vmHeapBytes=251024
- tras TimeDemo: uptimeMs=173743 fsUsed=204800 vmHeapBytes=251024
- tras ArrayFijo: uptimeMs=174340 fsUsed=208896 vmHeapBytes=251024
- tras MoveRango: uptimeMs=174841 fsUsed=212992 vmHeapBytes=251024
- tras HiloUsa: uptimeMs=175395 fsUsed=221184 vmHeapBytes=251024
- tras NatNew: uptimeMs=176412 fsUsed=229376 vmHeapBytes=251024
- tras NatV7: uptimeMs=177265 fsUsed=233472 vmHeapBytes=251024
- tras FloatNat: uptimeMs=178358 fsUsed=245760 vmHeapBytes=251024
- tras NatCede: uptimeMs=179199 fsUsed=249856 vmHeapBytes=251024
- tras NatExpropia: uptimeMs=180142 fsUsed=253952 vmHeapBytes=251024
- tras NatBuiltin: uptimeMs=181044 fsUsed=258048 vmHeapBytes=251024
- tras NatAnida: uptimeMs=181886 fsUsed=262144 vmHeapBytes=251024
- tras NatDirecta: uptimeMs=182766 fsUsed=266240 vmHeapBytes=251024
- tras NatCoste: uptimeMs=184431 fsUsed=270336 vmHeapBytes=251024
- tras NatNombres: uptimeMs=185287 fsUsed=274432 vmHeapBytes=251024
- tras NatEstrecha: uptimeMs=186235 fsUsed=278528 vmHeapBytes=251024
- tras CatCadena: uptimeMs=186754 fsUsed=282624 vmHeapBytes=251024
- tras CatCrono: uptimeMs=187339 fsUsed=286720 vmHeapBytes=251024
- tras SbPiezas: uptimeMs=187842 fsUsed=290816 vmHeapBytes=251024
- tras SbCrono: uptimeMs=188341 fsUsed=294912 vmHeapBytes=251024
- tras HeapInfo: uptimeMs=188857 fsUsed=299008 vmHeapBytes=251024
- tras HeapCompact: uptimeMs=189821 fsUsed=303104 vmHeapBytes=251024
- tras HeapReposo: uptimeMs=190743 fsUsed=307200 vmHeapBytes=251024
- tras DescarteAncho: uptimeMs=192507 fsUsed=311296 vmHeapBytes=251024
- tras TempRetiene: uptimeMs=193506 fsUsed=315392 vmHeapBytes=251024
- tras FinallyAnidado: uptimeMs=194011 fsUsed=319488 vmHeapBytes=251024
- tras RtcEnHora: uptimeMs=194507 fsUsed=323584 vmHeapBytes=251024
- ejecutadas (con EXITED) 31 de 33 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 6670f12a | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4151 | 0b2e4308 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2295 | 6d5eecb8 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8456 | 3612384a | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2221 | 63be81f6 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1126 | dfa8ab63 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2159 | 042932ad | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | 6205214f | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  2 2026 12:00:39` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=192752 fs=253952/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 17 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 287 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 92 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 416 | 318 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 172 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 438 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 674 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 251 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 306 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 278 | 18 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 784 | 61 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 880 | 364 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 225 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 289 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 382 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 205 | 92 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 156 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 217 | 55 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 143 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 165 | 14 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1332 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 191 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 305 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 292 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 249 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 233 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 308 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 208 | 10 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 818 | 19 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 849 | 63 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1583 | 28 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 711 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 196 | 8 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 454 | 9 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=193653 fsUsed=262144 vmHeapBytes=786432
- tras ThrowSinAtrapar: uptimeMs=194203 fsUsed=270336 vmHeapBytes=786432
- tras ThreadTrasMain: uptimeMs=194802 fsUsed=278528 vmHeapBytes=786432
- tras MachineId: uptimeMs=195403 fsUsed=286720 vmHeapBytes=786432
- tras Destructor: uptimeMs=196804 fsUsed=294912 vmHeapBytes=786432
- tras TimeDemo: uptimeMs=197938 fsUsed=303104 vmHeapBytes=786432
- tras ArrayFijo: uptimeMs=198688 fsUsed=311296 vmHeapBytes=786432
- tras MoveRango: uptimeMs=199588 fsUsed=319488 vmHeapBytes=786432
- tras HiloUsa: uptimeMs=200454 fsUsed=335872 vmHeapBytes=786432
- tras GuiWinJson: uptimeMs=202440 fsUsed=352256 vmHeapBytes=786432
- tras GuiShot: uptimeMs=204589 fsUsed=360448 vmHeapBytes=786432
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=206438 fsUsed=368640 vmHeapBytes=786432
- tras NatV7: uptimeMs=207186 fsUsed=376832 vmHeapBytes=786432
- tras FloatNat: uptimeMs=208553 fsUsed=393216 vmHeapBytes=786432
- tras NatCede: uptimeMs=209302 fsUsed=401408 vmHeapBytes=786432
- tras NatExpropia: uptimeMs=209951 fsUsed=409600 vmHeapBytes=786432
- tras NatBuiltin: uptimeMs=210735 fsUsed=417792 vmHeapBytes=786432
- tras NatAnida: uptimeMs=211402 fsUsed=425984 vmHeapBytes=786432
- tras NatDirecta: uptimeMs=212152 fsUsed=434176 vmHeapBytes=786432
- tras NatCoste: uptimeMs=214003 fsUsed=442368 vmHeapBytes=786432
- tras NatNombres: uptimeMs=214785 fsUsed=450560 vmHeapBytes=786432
- tras NatEstrecha: uptimeMs=215655 fsUsed=458752 vmHeapBytes=786432
- tras CatCadena: uptimeMs=216552 fsUsed=466944 vmHeapBytes=786432
- tras CatCrono: uptimeMs=217170 fsUsed=475136 vmHeapBytes=786432
- tras SbPiezas: uptimeMs=217886 fsUsed=483328 vmHeapBytes=786432
- tras SbCrono: uptimeMs=218503 fsUsed=491520 vmHeapBytes=786432
- tras HeapInfo: uptimeMs=219186 fsUsed=499712 vmHeapBytes=786432
- tras HeapCompact: uptimeMs=220520 fsUsed=507904 vmHeapBytes=786432
- tras HeapReposo: uptimeMs=222155 fsUsed=516096 vmHeapBytes=786432
- tras DescarteAncho: uptimeMs=224105 fsUsed=524288 vmHeapBytes=786432
- tras TempRetiene: uptimeMs=225374 fsUsed=532480 vmHeapBytes=786432
- tras FinallyAnidado: uptimeMs=226121 fsUsed=540672 vmHeapBytes=786432
- tras RtcEnHora: uptimeMs=227239 fsUsed=548864 vmHeapBytes=786432
- ejecutadas (con EXITED) 33 de 33 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 6670f12a | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4667 | 1fd37c26 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1504 | 7b5397fe | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2359 | cd20e12c | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3670 | d9f3513b | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1074 | 4a7229f8 | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2189 | 0c1797a2 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | f74e3271 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 48635085 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4151 | 0b2e4308 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2295 | 6d5eecb8 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8456 | 3612384a | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2221 | 63be81f6 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1126 | dfa8ab63 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2159 | 042932ad | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | 6205214f | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | c45827d4 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 4a87e0d1 | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: DESFASE de compilación —la placa tiene en pack:Gui/Gui.mod la versión 7 (>= la mínima 7), crc ae0d2a7e != el de hoy f99660c9: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Json: DESFASE de compilación —la placa tiene en pack:Gui/Json.mod la versión 7 (>= la mínima 7), crc 578601f2 != el de hoy 620b3c39: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Collections: DESFASE de compilación —la placa tiene en pack:Gui/Collections.mod la versión 7 (>= la mínima 7), crc feacb85e != el de hoy 263868cb: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Str: DESFASE de compilación —la placa tiene en pack:Gui/Str.mod la versión 7 (>= la mínima 7), crc 8e56f753 != el de hoy 9765bb28: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | esp32c6@COM3 | esp32p4@COM15 | rp2350a@COM22 | stm32u5g9@COM12 |
|---|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA | NO-PETA |
| Destructor | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| TimeDemo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ArrayFijo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MoveRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HiloUsa | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| GuiWinJson | IDENTICO | IDENTICO | SALTADA | IDENTICO |
| GuiShot | NO-PETA | NO-PETA | SALTADA | NO-PETA |
| NatNew | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatV7 | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| FloatNat | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatCede | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatExpropia | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatBuiltin | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatAnida | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatDirecta | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatCoste | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatNombres | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatEstrecha | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| CatCadena | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| CatCrono | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| SbPiezas | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| SbCrono | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HeapInfo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HeapCompact | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HeapReposo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| DescarteAncho | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| TempRetiene | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| FinallyAnidado | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| RtcEnHora | IDENTICO | IDENTICO | IDENTICO | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  
NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  

