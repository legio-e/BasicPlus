# Tandas T1 — las placas conducidas

## Tanda 2026-09-27 08:53:44

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (20 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-27 08:14:10) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-27 08:25:53) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 141 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 329 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 21 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 10 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 17 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 52 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 364 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 12 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 8 | 40 |
| NatCede | NatCede | f74a429b | 1485 | ok | 0 | 93 | 7 |
| NatExpropia | NatExpropia | bba4cdbe | 948 | ok | 0 | 8 | 4 |
| NatBuiltin | NatBuiltin | 932a14a5 | 1509 | ok | 0 | 41 | 8 |
| NatAnida | NatAnida | bb4c1fcb | 729 | ok | 0 | 10 | 3 |
| NatDirecta | NatDirecta | 38520c54 | 1071 | ok | 0 | 9 | 9 |
| NatCoste | NatCoste | d9f8a413 | 750 | ok | 0 | 13 | 2 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 27 2026 08:20:53` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=28223 fs=512000/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 67 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 141 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 329 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 21 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 10 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 17 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 620 | 48 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 530 | 372 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 80 | 93 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 50 | 41 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 530 | 13 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |

- tras MathRango: uptimeMs=31372 fsUsed=512000 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=33056 fsUsed=512000 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=34575 fsUsed=512000 vmHeapBytes=24637440
- tras MachineId: uptimeMs=36159 fsUsed=512000 vmHeapBytes=24637440
- tras Destructor: uptimeMs=38663 fsUsed=512000 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=41133 fsUsed=512000 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=42549 fsUsed=512000 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=44117 fsUsed=512000 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=46318 fsUsed=512000 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=54742 fsUsed=512000 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=63483 fsUsed=512000 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=67419 fsUsed=512000 vmHeapBytes=24637440
- tras NatV7: uptimeMs=69355 fsUsed=512000 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=73492 fsUsed=512000 vmHeapBytes=24637440
- tras NatCede: uptimeMs=75209 fsUsed=516096 vmHeapBytes=24637440
- tras NatExpropia: uptimeMs=76962 fsUsed=520192 vmHeapBytes=24637440
- tras NatBuiltin: uptimeMs=78763 fsUsed=524288 vmHeapBytes=24637440
- tras NatAnida: uptimeMs=80701 fsUsed=528384 vmHeapBytes=24637440
- tras NatDirecta: uptimeMs=83102 fsUsed=532480 vmHeapBytes=24637440
- tras NatCoste: uptimeMs=85201 fsUsed=536576 vmHeapBytes=24637440
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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4439 | 1bc7ea38 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2448 | 0a332d2d | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9325 | f98b1edf | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2311 | 0ad873d2 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1209 | 76defffe | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMAFC_ILP32F\NatBuiltin.mod | 2707 | fb0f4c21 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMAFC_ILP32F\NatAnida.mod | 1227 | d381be1d | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMAFC_ILP32F\NatDirecta.mod | 2584 | 553a1e3c | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMAFC_ILP32F\NatCoste.mod | 978 | 9aae5926 | ok |

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
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  


## Tanda 2026-09-27 09:18:12

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (20 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-27 09:05:26) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-27 09:05:31) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 323 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 11 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 11 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 21 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 57 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 360 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 8 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 8 | 40 |
| NatCede | NatCede | f74a429b | 1485 | ok | 0 | 93 | 7 |
| NatExpropia | NatExpropia | bba4cdbe | 948 | ok | 0 | 10 | 4 |
| NatBuiltin | NatBuiltin | 932a14a5 | 1509 | ok | 0 | 53 | 8 |
| NatAnida | NatAnida | bb4c1fcb | 729 | ok | 0 | 10 | 3 |
| NatDirecta | NatDirecta | 38520c54 | 1071 | ok | 0 | 8 | 9 |
| NatCoste | NatCoste | d9f8a413 | 750 | ok | 0 | 11 | 2 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 27 2026 08:20:53` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=18372 fs=536576/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 73 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 11 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 11 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 21 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 570 | 47 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 520 | 368 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 80 | 93 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 50 | 53 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 20 | 10 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 530 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |

- tras MathRango: uptimeMs=21482 fsUsed=536576 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=22916 fsUsed=536576 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=24285 fsUsed=536576 vmHeapBytes=24637440
- tras MachineId: uptimeMs=25884 fsUsed=536576 vmHeapBytes=24637440
- tras Destructor: uptimeMs=28459 fsUsed=536576 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=31291 fsUsed=536576 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=32659 fsUsed=536576 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=34578 fsUsed=536576 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=36595 fsUsed=536576 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=45603 fsUsed=536576 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=55095 fsUsed=536576 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=58914 fsUsed=536576 vmHeapBytes=24637440
- tras NatV7: uptimeMs=60783 fsUsed=536576 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=64736 fsUsed=536576 vmHeapBytes=24637440
- tras NatCede: uptimeMs=66505 fsUsed=536576 vmHeapBytes=24637440
- tras NatExpropia: uptimeMs=68274 fsUsed=536576 vmHeapBytes=24637440
- tras NatBuiltin: uptimeMs=70108 fsUsed=536576 vmHeapBytes=24637440
- tras NatAnida: uptimeMs=72057 fsUsed=536576 vmHeapBytes=24637440
- tras NatDirecta: uptimeMs=74411 fsUsed=536576 vmHeapBytes=24637440
- tras NatCoste: uptimeMs=76497 fsUsed=536576 vmHeapBytes=24637440
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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4439 | 1bc7ea38 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2448 | 0a332d2d | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9325 | f98b1edf | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2311 | 0ad873d2 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1209 | 76defffe | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMAFC_ILP32F\NatBuiltin.mod | 2707 | fb0f4c21 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMAFC_ILP32F\NatAnida.mod | 1227 | d381be1d | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMAFC_ILP32F\NatDirecta.mod | 2584 | 553a1e3c | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMAFC_ILP32F\NatCoste.mod | 978 | 9aae5926 | ok |

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
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  

