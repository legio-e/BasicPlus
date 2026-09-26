# Tandas T1 — las placas conducidas

## Tanda 2026-09-26 16:19:00

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-26 09:53:37) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-26 15:25:04) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 321 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 22 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 11 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 18 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 53 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 364 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 8 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 9 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 8 | 40 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 26 2026 16:09:31` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=47439 fs=491520/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 63 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 321 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 20 | 22 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 18 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 560 | 51 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 520 | 371 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |

- tras MathRango: uptimeMs=50426 fsUsed=491520 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=51844 fsUsed=491520 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=53179 fsUsed=491520 vmHeapBytes=24637440
- tras MachineId: uptimeMs=54780 fsUsed=491520 vmHeapBytes=24637440
- tras Destructor: uptimeMs=57250 fsUsed=491520 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=59935 fsUsed=491520 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=61354 fsUsed=491520 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=63338 fsUsed=491520 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=65473 fsUsed=491520 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=75432 fsUsed=491520 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=85841 fsUsed=495616 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=89478 fsUsed=495616 vmHeapBytes=24637440
- tras NatV7: uptimeMs=91429 fsUsed=495616 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=95567 fsUsed=495616 vmHeapBytes=24637440
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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4439 | d807be92 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2448 | a9f2094d | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9325 | 68a3b505 | ok (por trozos) |

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

