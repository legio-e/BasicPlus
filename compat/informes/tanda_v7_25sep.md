# Tandas T1 — las placas conducidas

## Tanda 2026-09-25 16:04:57

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-25 11:56:58) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-25 11:42:15) - sabor `gui1-lvgl0`
- placas conectadas: 3 (rp2350a@COM22, esp32s3@COM9, stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 14 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 327 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 12 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 16 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 13 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 12 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 14 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 50 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 366 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 12 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 14 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 12 | 40 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Sep 25 2026 11:44:55` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=108854 fs=356352/1048576 B vmHeapBytes=263208
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 43 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 29 | 14 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 6 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 306 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 66 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 52 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 21 | 13 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 78 | 12 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 34 | 14 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 50 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 366 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 19 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 20 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 50 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |

- tras MathRango: uptimeMs=110672 fsUsed=356352 vmHeapBytes=263208
- tras ThrowSinAtrapar: uptimeMs=111954 fsUsed=356352 vmHeapBytes=263208
- tras ThreadTrasMain: uptimeMs=113067 fsUsed=356352 vmHeapBytes=263208
- tras MachineId: uptimeMs=114368 fsUsed=356352 vmHeapBytes=263208
- tras Destructor: uptimeMs=116803 fsUsed=356352 vmHeapBytes=263208
- tras TimeDemo: uptimeMs=118638 fsUsed=356352 vmHeapBytes=263208
- tras ArrayFijo: uptimeMs=120074 fsUsed=356352 vmHeapBytes=263208
- tras MoveRango: uptimeMs=121574 fsUsed=356352 vmHeapBytes=263208
- tras HiloUsa: uptimeMs=123095 fsUsed=356352 vmHeapBytes=263208
- tras NatNew: uptimeMs=125542 fsUsed=364544 vmHeapBytes=263208
- tras NatV7: uptimeMs=127530 fsUsed=368640 vmHeapBytes=263208
- tras FloatNat: uptimeMs=131120 fsUsed=380928 vmHeapBytes=263208
- ejecutadas (con EXITED) 12 de 14 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4099 | 6eda00af | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2252 | 4362b914 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8449 | 82c18de9 | ok (por trozos) |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Sep 25 2026 11:47:22` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=74719 fs=401408/10248192 B vmHeapBytes=7336960
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 42 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 14 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 50 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 50 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 50 | 12 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 40 | 14 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 50 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 366 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **SALTADA** | - | 12 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatV7 | **SALTADA** | - | 14 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| FloatNat | **SALTADA** | - | 12 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |

- tras MathRango: uptimeMs=77356 fsUsed=401408 vmHeapBytes=7336960
- tras ThrowSinAtrapar: uptimeMs=78742 fsUsed=401408 vmHeapBytes=7336960
- tras ThreadTrasMain: uptimeMs=80226 fsUsed=401408 vmHeapBytes=7336960
- tras MachineId: uptimeMs=81726 fsUsed=401408 vmHeapBytes=7336960
- tras Destructor: uptimeMs=84112 fsUsed=401408 vmHeapBytes=7336960
- tras TimeDemo: uptimeMs=86365 fsUsed=401408 vmHeapBytes=7336960
- tras ArrayFijo: uptimeMs=87683 fsUsed=401408 vmHeapBytes=7336960
- tras MoveRango: uptimeMs=89583 fsUsed=401408 vmHeapBytes=7336960
- tras HiloUsa: uptimeMs=91452 fsUsed=401408 vmHeapBytes=7336960
- ejecutadas (con EXITED) 9 de 14 listadas; no terminaron 0; saltadas 5; no llegaron a arrancar 0

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

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 25 2026 11:48:01` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=132761 fs=606208/2064384 B vmHeapBytes=1179648
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 52 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 331 | 14 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 129 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 459 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 238 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 514 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 857 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 307 | 13 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 402 | 12 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 517 | 14 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 4588 | 53 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 4951 | 368 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 328 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 419 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 419 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |

- tras MathRango: uptimeMs=134962 fsUsed=606208 vmHeapBytes=1179648
- tras ThrowSinAtrapar: uptimeMs=136013 fsUsed=606208 vmHeapBytes=1179648
- tras ThreadTrasMain: uptimeMs=137115 fsUsed=606208 vmHeapBytes=1179648
- tras MachineId: uptimeMs=138600 fsUsed=606208 vmHeapBytes=1179648
- tras Destructor: uptimeMs=140618 fsUsed=606208 vmHeapBytes=1179648
- tras TimeDemo: uptimeMs=143003 fsUsed=606208 vmHeapBytes=1179648
- tras ArrayFijo: uptimeMs=144237 fsUsed=606208 vmHeapBytes=1179648
- tras MoveRango: uptimeMs=145572 fsUsed=606208 vmHeapBytes=1179648
- tras HiloUsa: uptimeMs=147088 fsUsed=606208 vmHeapBytes=1179648
- tras GuiWinJson: uptimeMs=169859 fsUsed=704512 vmHeapBytes=1179648
- tras GuiShot: uptimeMs=177632 fsUsed=704512 vmHeapBytes=1179648
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (7292 B, crc bd19c7be) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=180453 fsUsed=712704 vmHeapBytes=1179648
- tras NatV7: uptimeMs=181655 fsUsed=720896 vmHeapBytes=1179648
- tras FloatNat: uptimeMs=184639 fsUsed=737280 vmHeapBytes=1179648
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4099 | 6eda00af | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2252 | 4362b914 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8449 | 82c18de9 | ok (por trozos) |

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

| prueba | rp2350a@COM22 | esp32s3@COM9 | stm32u5g9@COM12 |
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
| GuiWinJson | SALTADA | SALTADA | IDENTICO |
| GuiShot | SALTADA | SALTADA | NO-PETA |
| NatNew | IDENTICO | SALTADA | IDENTICO |
| NatV7 | IDENTICO | SALTADA | IDENTICO |
| FloatNat | IDENTICO | SALTADA | IDENTICO |

SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiWinJson en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: NatNew en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatV7 en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: FloatNat en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  


## Tanda 2026-09-25 16:09:48

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-25 11:56:58) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-25 11:42:15) - sabor `gui1-lvgl0`
- placas conectadas: 2 (esp32c3@COM3, stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 16 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 328 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 12 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 14 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 21 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 18 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 14 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 15 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 49 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 390 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 13 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 13 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 12 | 40 |

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Sep 25 2026 11:46:25` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=30904 fs=241664/1523712 B vmHeapBytes=57344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 33 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 16 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 328 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 40 | 14 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 40 | 21 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 18 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 30 | 14 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 15 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 49 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 390 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **COLGADA** | - | 13 | sin EXITED en 30 s; tras KILL: tampoco EXITED en 5 s; la placa SE REINICIO durante la prueba (uptimeMs 43233 -> 35009) |
| NatV7 | **COLGADA** | - | 13 | sin EXITED en 30 s; tras KILL: tampoco EXITED en 5 s; la placa SE REINICIO durante la prueba (uptimeMs 35009 -> 34933) |
| FloatNat | **COLGADA** | - | 12 | sin EXITED en 30 s; tras KILL: tampoco EXITED en 5 s |

- tras MathRango: uptimeMs=32757 fsUsed=241664 vmHeapBytes=57344
- tras ThrowSinAtrapar: uptimeMs=34007 fsUsed=241664 vmHeapBytes=57344
- tras ThreadTrasMain: uptimeMs=35043 fsUsed=241664 vmHeapBytes=57344
- tras MachineId: uptimeMs=36325 fsUsed=241664 vmHeapBytes=57344
- tras Destructor: uptimeMs=38228 fsUsed=241664 vmHeapBytes=57344
- tras TimeDemo: uptimeMs=39698 fsUsed=241664 vmHeapBytes=57344
- tras ArrayFijo: uptimeMs=40934 fsUsed=241664 vmHeapBytes=57344
- tras MoveRango: uptimeMs=41984 fsUsed=241664 vmHeapBytes=57344
- tras HiloUsa: uptimeMs=43233 fsUsed=241664 vmHeapBytes=57344
- **la placa SE REINICIO durante NatNew** (uptimeMs decrecio; ver notas)
- tras NatNew: uptimeMs=35009 fsUsed=249856 vmHeapBytes=57344
<details><summary>NatNew: 20 lineas NO-JSON del wire (panics, hard faults, avisos)</summary>

```
I (216) cpu_start: Pro cpu start user code
I (216) cpu_start: cpu freq: 160000000 Hz
I (218) app_init: Application information:
I (222) app_init: Project name:     bpvm_esp32c3
I (226) app_init: App version:      v6.0-273-gda52d942
I (231) app_init: Compile time:     Sep 25 2026 16:09:10
I (236) app_init: ELF file SHA256:  54f9c75b2...
I (240) app_init: ESP-IDF:          v6.0.1
I (244) efuse_init: Min chip rev:     v0.3
I (248) efuse_init: Max chip rev:     v1.99
I (252) efuse_init: Chip rev:         v0.4
I (256) heap_init: Initializing. RAM available for dynamic allocation:
I (262) heap_init: At 3FC9C0C0 len 00023F40 (143 KiB): RAM
I (267) heap_init: At 3FCC0000 len 0001C710 (113 KiB): Retention RAM
I (273) heap_init: At 3FCDC710 len 00002950 (10 KiB): Retention RAM
I (279) heap_init: At 50000024 len 00001FC4 (7 KiB): RTCRAM
I (285) spi_flash: detected chip: generic
I (288) spi_flash: flash io: dio
I (291) sleep_gpio: Configure to isolate all GPIO pins in sleep state
I (297) sleep_gpio: Enable automatic switching of GPIO sleep configuration
```
</details>

- **la placa SE REINICIO durante NatV7** (uptimeMs decrecio; ver notas)
- tras NatV7: uptimeMs=34933 fsUsed=253952 vmHeapBytes=57344
<details><summary>NatV7: 20 lineas NO-JSON del wire (panics, hard faults, avisos)</summary>

```
I (216) cpu_start: Pro cpu start user code
I (216) cpu_start: cpu freq: 160000000 Hz
I (218) app_init: Application information:
I (221) app_init: Project name:     bpvm_esp32c3
I (226) app_init: App version:      v6.0-273-gda52d942
I (231) app_init: Compile time:     Sep 25 2026 16:09:10
I (236) app_init: ELF file SHA256:  54f9c75b2...
I (240) app_init: ESP-IDF:          v6.0.1
I (244) efuse_init: Min chip rev:     v0.3
I (248) efuse_init: Max chip rev:     v1.99
I (252) efuse_init: Chip rev:         v0.4
I (256) heap_init: Initializing. RAM available for dynamic allocation:
I (262) heap_init: At 3FC9C0C0 len 00023F40 (143 KiB): RAM
I (267) heap_init: At 3FCC0000 len 0001C710 (113 KiB): Retention RAM
I (273) heap_init: At 3FCDC710 len 00002950 (10 KiB): Retention RAM
I (279) heap_init: At 50000024 len 00001FC4 (7 KiB): RTCRAM
I (285) spi_flash: detected chip: generic
I (288) spi_flash: flash io: dio
I (291) sleep_gpio: Configure to isolate all GPIO pins in sleep state
I (297) sleep_gpio: Enable automatic switching of GPIO sleep configuration
```
</details>

- tras FloatNat: uptimeMs=35066 fsUsed=266240 vmHeapBytes=57344
<details><summary>FloatNat: 20 lineas NO-JSON del wire (panics, hard faults, avisos)</summary>

```
I (216) cpu_start: Pro cpu start user code
I (216) cpu_start: cpu freq: 160000000 Hz
I (218) app_init: Application information:
I (221) app_init: Project name:     bpvm_esp32c3
I (226) app_init: App version:      v6.0-273-gda52d942
I (231) app_init: Compile time:     Sep 25 2026 16:09:10
I (236) app_init: ELF file SHA256:  54f9c75b2...
I (240) app_init: ESP-IDF:          v6.0.1
I (244) efuse_init: Min chip rev:     v0.3
I (248) efuse_init: Max chip rev:     v1.99
I (252) efuse_init: Chip rev:         v0.4
I (256) heap_init: Initializing. RAM available for dynamic allocation:
I (262) heap_init: At 3FC9C0C0 len 00023F40 (143 KiB): RAM
I (267) heap_init: At 3FCC0000 len 0001C710 (113 KiB): Retention RAM
I (273) heap_init: At 3FCDC710 len 00002950 (10 KiB): Retention RAM
I (279) heap_init: At 50000024 len 00001FC4 (7 KiB): RTCRAM
I (285) spi_flash: detected chip: generic
I (288) spi_flash: flash io: dio
I (291) sleep_gpio: Configure to isolate all GPIO pins in sleep state
I (297) sleep_gpio: Enable automatic switching of GPIO sleep configuration
```
</details>

- ejecutadas (con EXITED) 9 de 14 listadas; no terminaron 3; saltadas 2; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4423 | 2d06f373 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2408 | 9e722734 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9733 | 7a8312b9 | ok (por trozos) |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- NatNew: la placa SE REINICIO durante la prueba (uptimeMs 43233 -> 35009, resetReason=panic/exception)
- NatV7: la placa SE REINICIO durante la prueba (uptimeMs 35009 -> 34933, resetReason=panic/exception)

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Sep 25 2026 11:47:41` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=192588 fs=311296/516096 B vmHeapBytes=393216
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 27 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 282 | 16 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 86 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 406 | 328 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 162 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 401 | 14 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 488 | 21 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 170 | 18 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 298 | 14 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 231 | 15 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 49 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 390 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 182 | 13 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 239 | 13 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 362 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |

- tras MathRango: uptimeMs=194612 fsUsed=311296 vmHeapBytes=393216
- tras ThrowSinAtrapar: uptimeMs=195876 fsUsed=311296 vmHeapBytes=393216
- tras ThreadTrasMain: uptimeMs=196971 fsUsed=311296 vmHeapBytes=393216
- tras MachineId: uptimeMs=198674 fsUsed=311296 vmHeapBytes=393216
- tras Destructor: uptimeMs=200715 fsUsed=311296 vmHeapBytes=393216
- tras TimeDemo: uptimeMs=202551 fsUsed=311296 vmHeapBytes=393216
- tras ArrayFijo: uptimeMs=203765 fsUsed=311296 vmHeapBytes=393216
- tras MoveRango: uptimeMs=205266 fsUsed=311296 vmHeapBytes=393216
- tras HiloUsa: uptimeMs=206785 fsUsed=311296 vmHeapBytes=393216
- tras NatNew: uptimeMs=209278 fsUsed=319488 vmHeapBytes=393216
- tras NatV7: uptimeMs=210880 fsUsed=327680 vmHeapBytes=393216
- tras FloatNat: uptimeMs=213963 fsUsed=344064 vmHeapBytes=393216
- ejecutadas (con EXITED) 12 de 14 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4099 | 6eda00af | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2252 | 4362b914 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8449 | 82c18de9 | ok (por trozos) |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Matriz pruebas x placas

| prueba | esp32c3@COM3 | stm32u575@COM5 |
|---|---|---|
| MathRango | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA |
| Destructor | IDENTICO | IDENTICO |
| TimeDemo | IDENTICO | IDENTICO |
| ArrayFijo | IDENTICO | IDENTICO |
| MoveRango | IDENTICO | IDENTICO |
| HiloUsa | IDENTICO | IDENTICO |
| GuiWinJson | SALTADA | SALTADA |
| GuiShot | SALTADA | SALTADA |
| NatNew | COLGADA | IDENTICO |
| NatV7 | COLGADA | IDENTICO |
| FloatNat | COLGADA | IDENTICO |

SALTADA: GuiWinJson en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiWinJson en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-25 16:20:00

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-25 11:56:58) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-25 11:42:15) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c3@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 329 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 15 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 243 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 17 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 14 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 12 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 15 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 51 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 374 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 12 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 13 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 12 | 40 |

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Sep 25 2026 16:18:28` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=82387 fs=266240/1523712 B vmHeapBytes=57344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 36 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 329 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 15 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 30 | 243 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 40 | 17 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 0 | 14 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 40 | 12 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 15 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 51 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 374 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 0 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 0 | 13 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |

- tras MathRango: uptimeMs=84049 fsUsed=266240 vmHeapBytes=57344
- tras ThrowSinAtrapar: uptimeMs=85084 fsUsed=266240 vmHeapBytes=57344
- tras ThreadTrasMain: uptimeMs=86130 fsUsed=266240 vmHeapBytes=57344
- tras MachineId: uptimeMs=87392 fsUsed=266240 vmHeapBytes=57344
- tras Destructor: uptimeMs=89263 fsUsed=266240 vmHeapBytes=57344
- tras TimeDemo: uptimeMs=90714 fsUsed=266240 vmHeapBytes=57344
- tras ArrayFijo: uptimeMs=91977 fsUsed=266240 vmHeapBytes=57344
- tras MoveRango: uptimeMs=93214 fsUsed=266240 vmHeapBytes=57344
- tras HiloUsa: uptimeMs=94653 fsUsed=266240 vmHeapBytes=57344
- tras NatNew: uptimeMs=97140 fsUsed=266240 vmHeapBytes=57344
- tras NatV7: uptimeMs=98768 fsUsed=266240 vmHeapBytes=57344
- tras FloatNat: uptimeMs=101288 fsUsed=266240 vmHeapBytes=57344
- ejecutadas (con EXITED) 12 de 14 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4423 | 2d06f373 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2408 | 9e722734 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9733 | 7a8312b9 | ok (por trozos) |

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
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |

SALTADA: GuiWinJson en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-25 16:26:59

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-25 11:56:58) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-25 16:20:01) - sabor `gui1-lvgl0`
- placas conectadas: 3 (esp32c6@COM3, esp32c5@COM6, esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 14 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 14 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 330 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 13 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 17 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 18 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 13 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 13 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 15 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 50 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 378 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 12 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 14 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 14 | 40 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Sep 25 2026 16:21:34` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=24300 fs=389120/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 44 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 14 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 330 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 13 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 30 | 17 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 30 | 18 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 13 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 15 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **DIFIERE** | 370 | 56 | texto distinto; estado no casa (host rc=0, EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: No space in heap)) |
| GuiShot | **NO-PETA** | 450 | 367 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 0 | 14 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |

- tras MathRango: uptimeMs=25983 fsUsed=389120 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=27218 fsUsed=389120 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=28668 fsUsed=389120 vmHeapBytes=65536
- tras MachineId: uptimeMs=30307 fsUsed=389120 vmHeapBytes=65536
- tras Destructor: uptimeMs=32203 fsUsed=389120 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=34916 fsUsed=389120 vmHeapBytes=65536
- tras ArrayFijo: uptimeMs=35973 fsUsed=389120 vmHeapBytes=65536
- tras MoveRango: uptimeMs=37024 fsUsed=389120 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=38693 fsUsed=389120 vmHeapBytes=65536
- tras GuiWinJson: uptimeMs=48729 fsUsed=389120 vmHeapBytes=65536
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

- tras GuiShot: uptimeMs=59498 fsUsed=389120 vmHeapBytes=65536
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=61914 fsUsed=397312 vmHeapBytes=65536
- tras NatV7: uptimeMs=63320 fsUsed=401408 vmHeapBytes=65536
- tras FloatNat: uptimeMs=66072 fsUsed=413696 vmHeapBytes=65536
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4423 | 2d06f373 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2408 | 9e722734 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9733 | 7a8312b9 | ok (por trozos) |

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

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Sep 25 2026 11:45:24` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=173116 fs=278528/1523712 B vmHeapBytes=6812672
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 46 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 14 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 330 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 13 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 30 | 17 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 40 | 18 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 20 | 13 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 30 | 13 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 15 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 670 | 52 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 520 | 367 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |

- tras MathRango: uptimeMs=174882 fsUsed=278528 vmHeapBytes=6812672
- tras ThrowSinAtrapar: uptimeMs=176105 fsUsed=278528 vmHeapBytes=6812672
- tras ThreadTrasMain: uptimeMs=177339 fsUsed=278528 vmHeapBytes=6812672
- tras MachineId: uptimeMs=178606 fsUsed=278528 vmHeapBytes=6812672
- tras Destructor: uptimeMs=180462 fsUsed=278528 vmHeapBytes=6812672
- tras TimeDemo: uptimeMs=182531 fsUsed=278528 vmHeapBytes=6812672
- tras ArrayFijo: uptimeMs=183580 fsUsed=278528 vmHeapBytes=6812672
- tras MoveRango: uptimeMs=184627 fsUsed=278528 vmHeapBytes=6812672
- tras HiloUsa: uptimeMs=186078 fsUsed=278528 vmHeapBytes=6812672
- tras GuiWinJson: uptimeMs=189766 fsUsed=278528 vmHeapBytes=6812672
- tras GuiShot: uptimeMs=193284 fsUsed=278528 vmHeapBytes=6812672
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.shot` (3350 B, crc f0589b1a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=195070 fsUsed=278528 vmHeapBytes=6812672
- tras NatV7: uptimeMs=196316 fsUsed=278528 vmHeapBytes=6812672
- tras FloatNat: uptimeMs=198416 fsUsed=278528 vmHeapBytes=6812672
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4423 | 2d06f373 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2408 | 9e722734 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9733 | 7a8312b9 | ok (por trozos) |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:DriverSpi/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:Gui/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Gui/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Gui/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Gui/Str.mod, crc igual): se ejecuta XIP, no se sube

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 25 2026 11:45:58` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=181975 fs=512000/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 63 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 14 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 330 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 13 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 20 | 17 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 18 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 13 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 10 | 15 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 560 | 58 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 600 | 366 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |

- tras MathRango: uptimeMs=184631 fsUsed=512000 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=185995 fsUsed=512000 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=187500 fsUsed=512000 vmHeapBytes=24637440
- tras MachineId: uptimeMs=189236 fsUsed=512000 vmHeapBytes=24637440
- tras Destructor: uptimeMs=191871 fsUsed=512000 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=194773 fsUsed=512000 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=196285 fsUsed=512000 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=197958 fsUsed=512000 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=200208 fsUsed=512000 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=211105 fsUsed=512000 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=222380 fsUsed=512000 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (6989 B, crc 8192c86e) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=226085 fsUsed=512000 vmHeapBytes=24637440
- tras NatV7: uptimeMs=227819 fsUsed=512000 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=231718 fsUsed=512000 vmHeapBytes=24637440
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4415 | 0e68c0ac | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2408 | e4e11589 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9325 | 906757c1 | ok (por trozos) |

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

| prueba | esp32c6@COM3 | esp32c5@COM6 | esp32p4@COM15 |
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
| GuiWinJson | DIFIERE | IDENTICO | IDENTICO |
| GuiShot | NO-PETA | NO-PETA | NO-PETA |
| NatNew | IDENTICO | IDENTICO | IDENTICO |
| NatV7 | IDENTICO | IDENTICO | IDENTICO |
| FloatNat | IDENTICO | IDENTICO | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  
NECESITA OJOS: GuiShot en esp32c5@COM6: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png  
NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  


## Tanda 2026-09-25 16:35:12

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-25 11:56:58) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-25 16:20:01) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 317 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 14 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 14 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 17 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 12 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 12 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 15 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 52 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 371 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 12 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 16 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 12 | 40 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 25 2026 11:45:58` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=114951 fs=487424/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 62 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 317 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 14 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 20 | 14 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 17 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 12 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 15 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 560 | 49 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 500 | 370 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 0 | 16 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |

- tras MathRango: uptimeMs=117852 fsUsed=487424 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=119272 fsUsed=487424 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=120789 fsUsed=487424 vmHeapBytes=24637440
- tras MachineId: uptimeMs=122456 fsUsed=487424 vmHeapBytes=24637440
- tras Destructor: uptimeMs=125226 fsUsed=487424 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=127428 fsUsed=487424 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=129146 fsUsed=487424 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=131034 fsUsed=487424 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=132865 fsUsed=487424 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=140221 fsUsed=487424 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=147863 fsUsed=487424 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (6989 B, crc 8192c86e) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=151632 fsUsed=487424 vmHeapBytes=24637440
- tras NatV7: uptimeMs=153400 fsUsed=487424 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=157291 fsUsed=487424 vmHeapBytes=24637440
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4415 | 0e68c0ac | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2408 | e4e11589 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9325 | 906757c1 | ok (por trozos) |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Gui/Core.mod, crc igual): se ejecuta XIP, no se sube
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
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  


## Tanda 2026-09-25 16:46:13

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-25 11:56:58) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-25 16:20:01) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 17 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 327 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 13 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 19 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 17 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 13 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 15 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 51 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 377 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 12 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 13 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 14 | 40 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 25 2026 16:43:39` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=65734 fs=487424/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 62 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 17 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 13 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 20 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 19 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 17 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 13 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 15 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 560 | 60 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 550 | 661 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |

- tras MathRango: uptimeMs=68914 fsUsed=487424 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=70264 fsUsed=487424 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=71547 fsUsed=487424 vmHeapBytes=24637440
- tras MachineId: uptimeMs=73265 fsUsed=487424 vmHeapBytes=24637440
- tras Destructor: uptimeMs=75686 fsUsed=487424 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=78020 fsUsed=487424 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=79372 fsUsed=487424 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=81308 fsUsed=487424 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=83341 fsUsed=487424 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=91354 fsUsed=487424 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=100654 fsUsed=487424 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (6989 B, crc 8192c86e) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=104309 fsUsed=487424 vmHeapBytes=24637440
- tras NatV7: uptimeMs=106312 fsUsed=487424 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=110635 fsUsed=487424 vmHeapBytes=24637440
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4415 | 0e68c0ac | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2408 | e4e11589 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9325 | 906757c1 | ok (por trozos) |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Gui/Core.mod, crc igual): se ejecuta XIP, no se sube
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
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  

## Tanda 2026-09-25 16:49:44

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-25 11:56:58) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-25 16:20:01) - sabor `gui1-lvgl0`
- placas conectadas: 1 (rp2350b@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 17 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 321 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 12 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 15 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 16 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 16 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 16 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 19 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 54 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 373 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 15 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 13 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 11 | 40 |

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Sep 25 2026 11:44:55` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=40091 fs=233472/10248192 B vmHeapBytes=5767168
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 35 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 38 | 17 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 16 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 315 | 321 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 16 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 78 | 15 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 73 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 32 | 16 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 90 | 16 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 49 | 19 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 54 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 373 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 30 | 15 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 30 | 13 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 59 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |

- tras MathRango: uptimeMs=42110 fsUsed=233472 vmHeapBytes=5767168
- tras ThrowSinAtrapar: uptimeMs=43386 fsUsed=233472 vmHeapBytes=5767168
- tras ThreadTrasMain: uptimeMs=44482 fsUsed=233472 vmHeapBytes=5767168
- tras MachineId: uptimeMs=45988 fsUsed=233472 vmHeapBytes=5767168
- tras Destructor: uptimeMs=48620 fsUsed=233472 vmHeapBytes=5767168
- tras TimeDemo: uptimeMs=50404 fsUsed=233472 vmHeapBytes=5767168
- tras ArrayFijo: uptimeMs=51621 fsUsed=233472 vmHeapBytes=5767168
- tras MoveRango: uptimeMs=52919 fsUsed=233472 vmHeapBytes=5767168
- tras HiloUsa: uptimeMs=54641 fsUsed=233472 vmHeapBytes=5767168
- tras NatNew: uptimeMs=57451 fsUsed=233472 vmHeapBytes=5767168
- tras NatV7: uptimeMs=59049 fsUsed=233472 vmHeapBytes=5767168
- tras FloatNat: uptimeMs=62198 fsUsed=233472 vmHeapBytes=5767168
- ejecutadas (con EXITED) 12 de 14 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4099 | 6eda00af | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2252 | 4362b914 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8449 | 82c18de9 | ok (por trozos) |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:SQLite/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Matriz pruebas x placas

| prueba | rp2350b@COM4 |
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

SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-25 16:50:27

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-25 11:56:58) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-25 16:20:01) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 313 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 15 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 13 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 19 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 13 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 16 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 15 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 57 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 392 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 13 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 15 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 12 | 40 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Sep 25 2026 16:21:34` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=1431635 fs=200704/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 27 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 313 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 15 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 30 | 13 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 40 | 19 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 0 | 13 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 30 | 16 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 30 | 15 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 520 | 53 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 470 | 366 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 0 | 13 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 10 | 15 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |

- tras MathRango: uptimeMs=1433320 fsUsed=200704 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=1434361 fsUsed=200704 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=1435437 fsUsed=200704 vmHeapBytes=65536
- tras MachineId: uptimeMs=1436906 fsUsed=200704 vmHeapBytes=65536
- tras Destructor: uptimeMs=1438591 fsUsed=200704 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=1440277 fsUsed=200704 vmHeapBytes=65536
- tras ArrayFijo: uptimeMs=1441313 fsUsed=200704 vmHeapBytes=65536
- tras MoveRango: uptimeMs=1442362 fsUsed=200704 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=1444030 fsUsed=200704 vmHeapBytes=65536
- tras GuiWinJson: uptimeMs=1447083 fsUsed=200704 vmHeapBytes=65536
- tras GuiShot: uptimeMs=1450454 fsUsed=200704 vmHeapBytes=65536
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=1452872 fsUsed=200704 vmHeapBytes=65536
- tras NatV7: uptimeMs=1454310 fsUsed=200704 vmHeapBytes=65536
- tras FloatNat: uptimeMs=1456658 fsUsed=200704 vmHeapBytes=65536
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4423 | 2d06f373 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2408 | 9e722734 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9733 | 7a8312b9 | ok (por trozos) |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube

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

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  

