# Tandas T1 — las placas conducidas

## Tanda 2026-09-27 09:24:23

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (20 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-27 09:05:26) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-27 09:05:31) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 328 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 46 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 364 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 12 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 8 | 40 |
| NatCede | NatCede | f74a429b | 1485 | ok | 0 | 107 | 7 |
| NatExpropia | NatExpropia | bba4cdbe | 948 | ok | 0 | 9 | 4 |
| NatBuiltin | NatBuiltin | 932a14a5 | 1509 | ok | 0 | 53 | 8 |
| NatAnida | NatAnida | bb4c1fcb | 729 | ok | 0 | 8 | 3 |
| NatDirecta | NatDirecta | 38520c54 | 1071 | ok | 0 | 10 | 9 |
| NatCoste | NatCoste | d9f8a413 | 750 | ok | 0 | 12 | 2 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 27 2026 09:09:32` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=16926 fs=458752/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 44 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 305 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 100 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 435 | 328 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 221 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 512 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 905 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 293 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 384 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 544 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 984 | 51 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 1003 | 372 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 264 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 358 | 12 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 390 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 293 | 107 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 176 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 248 | 53 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 153 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 165 | 10 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1375 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |

- tras MathRango: uptimeMs=18913 fsUsed=458752 vmHeapBytes=786432
- tras ThrowSinAtrapar: uptimeMs=19963 fsUsed=458752 vmHeapBytes=786432
- tras ThreadTrasMain: uptimeMs=21063 fsUsed=458752 vmHeapBytes=786432
- tras MachineId: uptimeMs=22363 fsUsed=458752 vmHeapBytes=786432
- tras Destructor: uptimeMs=24601 fsUsed=458752 vmHeapBytes=786432
- tras TimeDemo: uptimeMs=26821 fsUsed=458752 vmHeapBytes=786432
- tras ArrayFijo: uptimeMs=28034 fsUsed=458752 vmHeapBytes=786432
- tras MoveRango: uptimeMs=29350 fsUsed=458752 vmHeapBytes=786432
- tras HiloUsa: uptimeMs=30869 fsUsed=458752 vmHeapBytes=786432
- tras GuiWinJson: uptimeMs=34286 fsUsed=458752 vmHeapBytes=786432
- tras GuiShot: uptimeMs=37722 fsUsed=458752 vmHeapBytes=786432
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=41226 fsUsed=458752 vmHeapBytes=786432
- tras NatV7: uptimeMs=42843 fsUsed=458752 vmHeapBytes=786432
- tras FloatNat: uptimeMs=45896 fsUsed=458752 vmHeapBytes=786432
- tras NatCede: uptimeMs=47498 fsUsed=458752 vmHeapBytes=786432
- tras NatExpropia: uptimeMs=48999 fsUsed=458752 vmHeapBytes=786432
- tras NatBuiltin: uptimeMs=50853 fsUsed=458752 vmHeapBytes=786432
- tras NatAnida: uptimeMs=52335 fsUsed=458752 vmHeapBytes=786432
- tras NatDirecta: uptimeMs=53920 fsUsed=458752 vmHeapBytes=786432
- tras NatCoste: uptimeMs=56389 fsUsed=458752 vmHeapBytes=786432
- ejecutadas (con EXITED) 20 de 20 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4123 | 5310b8e4 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2296 | 27b863a9 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8449 | 79aa73d4 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2207 | 46d764ab | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1169 | 5d4c0544 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2503 | 4d6fadde | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1159 | 06ab1673 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2212 | 749bdbba | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 910 | 26f80162 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Gui/Core.mod, crc igual): se ejecuta XIP, no se sube
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
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |

NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  

