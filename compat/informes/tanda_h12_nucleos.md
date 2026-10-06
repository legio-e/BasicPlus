# Tandas T1 — las placas conducidas

## Tanda 2026-10-06 15:47:52

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 85 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 320 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 25 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 21 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 68 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 372 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 55 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 134 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 11 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 14 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 28 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 85 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 43 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 58 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 10 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 13 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 12 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 11 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 29 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 12 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 11 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 9 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 10 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 18 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 61 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 22 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 16 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 9 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 10 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 11 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 13 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 156 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 26 | 11 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  6 2026 07:40:34` imageCrc=`16ea1186` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (06-10 15:43)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=103196 fs=159744/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 11 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 192 | 85 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 314 | 320 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 21 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 233 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 99 | 25 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 76 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 213 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 59 | 21 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 705 | 55 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 573 | 365 | EXITED OK exit=0 |
| GuiArbolG2 | **IDENTICO** | 524 | 59 | EXITED OK exit=0; host rc=0 |
| GuiAnidadaGc | **IDENTICO** | 14312 | 141 | EXITED OK exit=0; host rc=0 |
| NatNew | **IDENTICO** | 82 | 11 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 90 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 282 | 28 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 76 | 85 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 28 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatDosRuns | **IDENTICO** | 100 | 43 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 59 | 58 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 22 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 60 | 10 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 999 | 13 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatNombres | **IDENTICO** | 38 | 12 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| NatEstrecha | **IDENTICO** | 202 | 11 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| DblNat | **IDENTICO** | 2101 | 29 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMAFC_ILP32F) |
| KillLoop | **PARADA** | 437 | - | KILL a los 500 ms; el EXITED, 29 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 438 | - | KILL a los 500 ms; el EXITED, 24 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatRecurKill | **PARADA** | 435 | - | KILL a los 500 ms; el EXITED, 23 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| CatCadena | **IDENTICO** | 174 | 12 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 60 | 11 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 97 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 81 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 78 | 10 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 241 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 231 | 61 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1017 | 22 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 272 | 16 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 66 | 10 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 196 | 11 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 319 | 13 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 15 | 156 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 949 | 26 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=104412 fsUsed=163840 vmHeapBytes=24637440 pila libre: run=13564 io=25872 tareaREPL=25872 heap del sistema, minimo libre=428247
- tras ThrowSinAtrapar: uptimeMs=104995 fsUsed=163840 vmHeapBytes=24637440 pila libre: run=13564 io=25872 tareaREPL=25872 heap del sistema, minimo libre=428247
- tras ThreadTrasMain: uptimeMs=105962 fsUsed=163840 vmHeapBytes=24637440 pila libre: run=15508 io=25872 tareaREPL=25872 heap del sistema, minimo libre=428247
- tras MachineId: uptimeMs=106780 fsUsed=163840 vmHeapBytes=24637440 pila libre: run=13468 io=25864 tareaREPL=25864 heap del sistema, minimo libre=428247
- tras Destructor: uptimeMs=108165 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=13428 io=25864 tareaREPL=25864 heap del sistema, minimo libre=428247
- tras TimeDemo: uptimeMs=109333 fsUsed=163840 vmHeapBytes=24637440 pila libre: run=13428 io=25864 tareaREPL=25864 heap del sistema, minimo libre=425667
- tras ArrayFijo: uptimeMs=110285 fsUsed=163840 vmHeapBytes=24637440 pila libre: run=13428 io=25864 tareaREPL=25864 heap del sistema, minimo libre=425667
- tras MoveRango: uptimeMs=111452 fsUsed=163840 vmHeapBytes=24637440 pila libre: run=13048 io=25864 tareaREPL=25864 heap del sistema, minimo libre=425667
- tras HiloUsa: uptimeMs=112872 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=13048 io=24784 tareaREPL=24784 heap del sistema, minimo libre=425667
- tras GuiWinJson: uptimeMs=118957 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=13048 io=24588 tareaREPL=24588 heap del sistema, minimo libre=390523
- tras GuiShot: uptimeMs=124779 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=13048 io=24588 tareaREPL=24588 heap del sistema, minimo libre=389139
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- tras GuiArbolG2: uptimeMs=132405 fsUsed=180224 vmHeapBytes=24637440 pila libre: run=13048 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras GuiAnidadaGc: uptimeMs=151753 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=13048 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatNew: uptimeMs=153774 fsUsed=180224 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatV7: uptimeMs=155275 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras FloatNat: uptimeMs=157627 fsUsed=184320 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatCede: uptimeMs=158944 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatExpropia: uptimeMs=160095 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatDosRuns: uptimeMs=161750 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatBuiltin: uptimeMs=163263 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatAnida: uptimeMs=164347 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatDirecta: uptimeMs=165615 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatCoste: uptimeMs=167700 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatNombres: uptimeMs=169070 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatEstrecha: uptimeMs=170642 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras DblNat: uptimeMs=173989 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras KillLoop: uptimeMs=174860 fsUsed=172032 vmHeapBytes=24637440 pila libre: run=11236 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatKill: uptimeMs=176240 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras NatRecurKill: uptimeMs=177650 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=11236 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras CatCadena: uptimeMs=178794 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras CatCrono: uptimeMs=179529 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras SbPiezas: uptimeMs=180497 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras SbCrono: uptimeMs=181414 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12916 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras HeapInfo: uptimeMs=182248 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12528 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras HeapCompact: uptimeMs=183849 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=11236 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras HeapReposo: uptimeMs=185483 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12392 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras DescarteAncho: uptimeMs=187424 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12392 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras TempRetiene: uptimeMs=188888 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12392 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras FinallyAnidado: uptimeMs=189775 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12392 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras RtcEnHora: uptimeMs=190821 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=12392 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras FicheroOCarpeta: uptimeMs=191822 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=11584 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras GcHondo: uptimeMs=193005 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=11584 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras ImportHondo: uptimeMs=197077 fsUsed=225280 vmHeapBytes=24637440 pila libre: run=11584 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- tras synclisttest: uptimeMs=201531 fsUsed=176128 vmHeapBytes=24637440 pila libre: run=11584 io=24572 tareaREPL=24572 heap del sistema, minimo libre=385103
- ejecutadas (con EXITED) 44 de 44 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | a79a2400 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | e9faef8a | ok |
| /app/GuiArbolG2.mod | bpgenvm-c\build\tanda\GuiArbolG2\GuiArbolG2.mod | 5379 | b33c4dbf | ok (por trozos) |
| /app/GuiAnidadaGc.mod | bpgenvm-c\build\tanda\GuiAnidadaGc\GuiAnidadaGc.mod | 1121 | 82388d9f | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4467 | 183a7978 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2447 | c909fac7 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9332 | 2365095d | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2325 | 6f33192a | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1166 | 0f42a895 | ok |
| /app/NatDosRuns.mod | bpgenvm-c\build\tanda\NatDosRuns\aot-RV32IMAFC_ILP32F\NatDosRuns.mod | 3672 | 3260af8a | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMAFC_ILP32F\NatBuiltin.mod | 2790 | 7d97dff4 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMAFC_ILP32F\NatAnida.mod | 1232 | 3b121406 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMAFC_ILP32F\NatDirecta.mod | 2599 | 39c87e99 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMAFC_ILP32F\NatCoste.mod | 1000 | 75ab9b09 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMAFC_ILP32F\NatNombres.mod | 2706 | e84239c1 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMAFC_ILP32F\NatEstrecha.mod | 3131 | 3dac6451 | ok |
| /app/DblNat.mod | bpgenvm-c\build\tanda\DblNat\aot-RV32IMAFC_ILP32F\DblNat.mod | 2744 | 34de4dac | ok |
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/NatKill.mod | bpgenvm-c\build\tanda\NatKill\aot-RV32IMAFC_ILP32F\NatKill.mod | 513 | ac4ce6b0 | ok |
| /app/NatRecurKill.mod | bpgenvm-c\build\tanda\NatRecurKill\aot-RV32IMAFC_ILP32F\NatRecurKill.mod | 674 | 3437f178 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/GcHondo.mod | bpgenvm-c\build\tanda\GcHondo\GcHondo.mod | 1584 | 38aa99a6 | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |
| /app/SyncListTest.mod | bpgenvm-c\build\tanda\synclisttest\SyncListTest.mod | 3860 | 958dfea6 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Gui/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:Gui/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: ya en /lib/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

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
| GuiArbolG2 | IDENTICO |
| GuiAnidadaGc | IDENTICO |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatDosRuns | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| DblNat | IDENTICO |
| KillLoop | PARADA |
| NatKill | PARADA |
| NatRecurKill | PARADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | IDENTICO |
| ImportHondo | IDENTICO |
| synclisttest | ASERCIONES |

MEMORIA APRETADA: synclisttest: NO ES VERDE: ninguna placa de la tanda que aprieta la ha corrido (sin PSRAM y <= 1 MB de SRAM: la Pico 2, la C6, la C3)  
NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  


## Tanda 2026-10-06 15:51:12

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32s3@COM9)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 332 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 14 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 17 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 28 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 10 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 51 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 377 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 51 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 137 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 13 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 8 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 107 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 42 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 113 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 10 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 11 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 10 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 12 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 29 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 13 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 13 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 10 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 19 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 57 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 23 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 15 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 9 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 13 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 12 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 17 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 41 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 25 | 11 |

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Oct  6 2026 07:40:04` imageCrc=`1c80eb19` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (06-10 15:48)
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=157407 fs=192512/10248192 B vmHeapBytes=7336960
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 23 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 192 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 11 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 332 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 19 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 233 | 14 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 100 | 17 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 77 | 28 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 213 | 10 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 62 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 51 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 377 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 51 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiAnidadaGc | **SALTADA** | - | 137 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **SALTADA** | - | 13 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatV7 | **SALTADA** | - | 10 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| FloatNat | **SALTADA** | - | 8 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatCede | **SALTADA** | - | 107 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatExpropia | **SALTADA** | - | 9 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatDosRuns | **SALTADA** | - | 42 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatBuiltin | **SALTADA** | - | 113 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatAnida | **SALTADA** | - | 10 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatDirecta | **SALTADA** | - | 11 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatCoste | **SALTADA** | - | 12 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatNombres | **SALTADA** | - | 10 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatEstrecha | **SALTADA** | - | 12 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| DblNat | **SALTADA** | - | 29 | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| KillLoop | **PARADA** | 234 | - | KILL a los 500 ms; el EXITED, 22 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **SALTADA** | - | - | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| NatRecurKill | **SALTADA** | - | - | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |
| CatCadena | **IDENTICO** | 172 | 13 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 166 | 13 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 94 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 211 | 10 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 79 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 823 | 19 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 584 | 57 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 2941 | 23 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 827 | 15 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 28 | 9 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 65 | 13 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 133 | 12 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 892 | 17 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 14 | 41 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 2456 | 25 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=158943 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=14820 io=8172 tareaREPL=8172 heap del sistema, minimo libre=248175
- tras ThrowSinAtrapar: uptimeMs=159793 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=14060 io=8108 tareaREPL=8108 heap del sistema, minimo libre=248147
- tras ThreadTrasMain: uptimeMs=161061 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=13324 io=8108 tareaREPL=8108 heap del sistema, minimo libre=247819
- tras MachineId: uptimeMs=162178 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=13284 io=8108 tareaREPL=8108 heap del sistema, minimo libre=247819
- tras Destructor: uptimeMs=164015 fsUsed=200704 vmHeapBytes=7336960 pila libre: run=13188 io=8108 tareaREPL=8108 heap del sistema, minimo libre=247467
- tras TimeDemo: uptimeMs=167184 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=13188 io=8108 tareaREPL=8108 heap del sistema, minimo libre=242767
- tras ArrayFijo: uptimeMs=168535 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=13188 io=8108 tareaREPL=8108 heap del sistema, minimo libre=242767
- tras MoveRango: uptimeMs=170037 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=12516 io=8108 tareaREPL=8108 heap del sistema, minimo libre=242767
- tras HiloUsa: uptimeMs=171921 fsUsed=200704 vmHeapBytes=7336960 pila libre: run=13196 io=8108 tareaREPL=8108 heap del sistema, minimo libre=242767
- tras KillLoop: uptimeMs=172786 fsUsed=192512 vmHeapBytes=7336960 pila libre: run=13196 io=8108 tareaREPL=8108 heap del sistema, minimo libre=242767
- tras CatCadena: uptimeMs=174224 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=12516 io=8108 tareaREPL=8108 heap del sistema, minimo libre=242767
- tras CatCrono: uptimeMs=175494 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=12516 io=8108 tareaREPL=8108 heap del sistema, minimo libre=242767
- tras SbPiezas: uptimeMs=176676 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=12516 io=8108 tareaREPL=8108 heap del sistema, minimo libre=242767
- tras SbCrono: uptimeMs=178077 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=12516 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras HeapInfo: uptimeMs=179228 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=12212 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras HeapCompact: uptimeMs=182516 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=12252 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras HeapReposo: uptimeMs=184800 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=11988 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras DescarteAncho: uptimeMs=188672 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=11988 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras TempRetiene: uptimeMs=190655 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=11988 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras FinallyAnidado: uptimeMs=191673 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=11988 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras RtcEnHora: uptimeMs=193126 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=11988 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras FicheroOCarpeta: uptimeMs=194675 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=11444 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras GcHondo: uptimeMs=196610 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=11444 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242767
- tras ImportHondo: uptimeMs=202199 fsUsed=245760 vmHeapBytes=7336960 pila libre: run=11444 io=8044 tareaREPL=8044 heap del sistema, minimo libre=242587
- tras synclisttest: uptimeMs=209573 fsUsed=196608 vmHeapBytes=7336960 pila libre: run=11444 io=8044 tareaREPL=8044 heap del sistema, minimo libre=237851
- ejecutadas (con EXITED) 25 de 44 listadas; no terminaron 0; saltadas 19; no llegaron a arrancar 0

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
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/GcHondo.mod | bpgenvm-c\build\tanda\GcHondo\GcHondo.mod | 1584 | 38aa99a6 | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |
| /app/SyncListTest.mod | bpgenvm-c\build\tanda\synclisttest\SyncListTest.mod | 3860 | 958dfea6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

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
| GuiArbolG2 | SALTADA |
| GuiAnidadaGc | SALTADA |
| NatNew | SALTADA |
| NatV7 | SALTADA |
| FloatNat | SALTADA |
| NatCede | SALTADA |
| NatExpropia | SALTADA |
| NatDosRuns | SALTADA |
| NatBuiltin | SALTADA |
| NatAnida | SALTADA |
| NatDirecta | SALTADA |
| NatCoste | SALTADA |
| NatNombres | SALTADA |
| NatEstrecha | SALTADA |
| DblNat | SALTADA |
| KillLoop | PARADA |
| NatKill | SALTADA |
| NatRecurKill | SALTADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | IDENTICO |
| ImportHondo | IDENTICO |
| synclisttest | ASERCIONES |

MEMORIA APRETADA: synclisttest: NO ES VERDE: ninguna placa de la tanda que aprieta la ha corrido (sin PSRAM y <= 1 MB de SRAM: la Pico 2, la C6, la C3)  
SALTADA: GuiWinJson en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiArbolG2 en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiAnidadaGc en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: NatNew en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatV7 en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: FloatNat en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatCede en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatExpropia en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatDosRuns en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatBuiltin en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatAnida en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatDirecta en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatCoste en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatNombres en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatEstrecha en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: DblNat en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatKill en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  
SALTADA: NatRecurKill en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  


## Tanda 2026-10-06 16:10:18

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 326 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 13 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 27 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 12 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 48 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 383 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 53 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 196 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 9 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 91 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 43 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 47 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 9 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 12 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 33 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 11 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 10 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 25 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 82 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 23 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 15 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 9 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 10 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 12 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 78 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 23 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 26 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  6 2026 07:40:01` imageCrc=`33e4fac5` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (06-10 15:49)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=1242257 fs=225280/1048576 B vmHeapBytes=219440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 27 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 23 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 155 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 308 | 326 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 9 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 40 | 13 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 39 | 27 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 17 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 45 | 12 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 32 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 48 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 383 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 53 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiAnidadaGc | **SALTADA** | - | 196 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 16 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 15 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 34 | 9 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 61 | 91 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 11 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDosRuns | **IDENTICO** | 285 | 43 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 48 | 47 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 9 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 17 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 2958 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 13 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 42 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2178 | 33 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 437 | - | KILL a los 500 ms; el EXITED, 7 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 431 | - | KILL a los 500 ms; el EXITED, 6 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 427 | - | KILL a los 500 ms; el EXITED, 10 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 46 | 11 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 97 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 34 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 89 | 10 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 35 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 395 | 25 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 479 | 82 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1579 | 23 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 347 | 15 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 13 | 9 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 17 | 10 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 85 | 12 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 448 | 78 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 8 | 23 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 3549 | 26 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=1243058 fsUsed=229376 vmHeapBytes=219440 pila libre: run=13500 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=1243669 fsUsed=229376 vmHeapBytes=219440 pila libre: run=14336 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=1244608 fsUsed=229376 vmHeapBytes=219440 pila libre: run=13444 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=1245146 fsUsed=229376 vmHeapBytes=219440 pila libre: run=13300 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras Destructor: uptimeMs=1245780 fsUsed=233472 vmHeapBytes=219440 pila libre: run=13368 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras TimeDemo: uptimeMs=1246789 fsUsed=229376 vmHeapBytes=219440 pila libre: run=13268 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras ArrayFijo: uptimeMs=1247423 fsUsed=229376 vmHeapBytes=219440 pila libre: run=13368 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras MoveRango: uptimeMs=1247962 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras HiloUsa: uptimeMs=1248559 fsUsed=233472 vmHeapBytes=219440 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras NatNew: uptimeMs=1249632 fsUsed=233472 vmHeapBytes=219440 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatV7: uptimeMs=1250643 fsUsed=229376 vmHeapBytes=219440 pila libre: run=13272 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras FloatNat: uptimeMs=1251796 fsUsed=237568 vmHeapBytes=219440 pila libre: run=12796 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras NatCede: uptimeMs=1252718 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12796 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras NatExpropia: uptimeMs=1253601 fsUsed=229376 vmHeapBytes=219440 pila libre: run=13272 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras NatDosRuns: uptimeMs=1254930 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras NatBuiltin: uptimeMs=1256018 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras NatAnida: uptimeMs=1256906 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras NatDirecta: uptimeMs=1257853 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras NatCoste: uptimeMs=1261672 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras NatNombres: uptimeMs=1262608 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras NatEstrecha: uptimeMs=1263619 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras DblNat: uptimeMs=1266597 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras KillLoop: uptimeMs=1267266 fsUsed=225280 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras NatKill: uptimeMs=1268486 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras NatRecurKill: uptimeMs=1269624 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras CatCadena: uptimeMs=1270203 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras CatCrono: uptimeMs=1270734 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras SbPiezas: uptimeMs=1271271 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras SbCrono: uptimeMs=1271803 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras HeapInfo: uptimeMs=1272398 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras HeapCompact: uptimeMs=1273778 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras HeapReposo: uptimeMs=1275152 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras DescarteAncho: uptimeMs=1277353 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras TempRetiene: uptimeMs=1278306 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras FinallyAnidado: uptimeMs=1278920 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12372 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras RtcEnHora: uptimeMs=1279446 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras FicheroOCarpeta: uptimeMs=1279984 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12180 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras GcHondo: uptimeMs=1280943 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12368 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras ImportHondo: uptimeMs=1283166 fsUsed=278528 vmHeapBytes=219440 pila libre: run=12180 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- tras synclisttest: uptimeMs=1288286 fsUsed=229376 vmHeapBytes=219440 pila libre: run=12180 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6072
- ejecutadas (con EXITED) 40 de 44 listadas; no terminaron 0; saltadas 4; no llegaron a arrancar 0

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
| /app/NatDosRuns.mod | bpgenvm-c\build\tanda\NatDosRuns\aot-ARMV8M_MAIN_SOFTFP\NatDosRuns.mod | 3540 | db2b330a | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2215 | 18ea246e | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | 6205214f | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/DblNat.mod | bpgenvm-c\build\tanda\DblNat\aot-ARMV8M_MAIN_SOFTFP\DblNat.mod | 2540 | ac5be2ec | ok |
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/NatKill.mod | bpgenvm-c\build\tanda\NatKill\aot-ARMV8M_MAIN_SOFTFP\NatKill.mod | 485 | 228703af | ok |
| /app/NatRecurKill.mod | bpgenvm-c\build\tanda\NatRecurKill\aot-ARMV8M_MAIN_SOFTFP\NatRecurKill.mod | 606 | efa300c8 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/GcHondo.mod | bpgenvm-c\build\tanda\GcHondo\GcHondo.mod | 1584 | 38aa99a6 | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |
| /app/SyncListTest.mod | bpgenvm-c\build\tanda\synclisttest\SyncListTest.mod | 3860 | 958dfea6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

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
| GuiArbolG2 | SALTADA |
| GuiAnidadaGc | SALTADA |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatDosRuns | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| DblNat | IDENTICO |
| KillLoop | PARADA |
| NatKill | PARADA |
| NatRecurKill | PARADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | IDENTICO |
| ImportHondo | IDENTICO |
| synclisttest | ASERCIONES |

MEMORIA APRETADA: synclisttest: verde en las que aprietan: rp2350a@COM22  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiArbolG2 en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiAnidadaGc en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-06 16:11:31

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350b@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 321 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 12 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 17 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 10 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 120 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 374 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 54 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 130 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 9 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 99 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 12 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 44 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 68 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 9 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 14 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 14 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 13 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 30 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 9 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 11 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 11 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 11 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 11 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 18 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 75 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 22 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 15 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 9 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 76 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 12 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 12 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 24 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 25 | 11 |

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Oct  6 2026 07:40:01` imageCrc=`33e4fac5` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (06-10 15:49)
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=79083 fs=180224/10248192 B vmHeapBytes=5767168
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 22 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 34 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 144 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 317 | 321 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 19 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 53 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 56 | 17 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 29 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 57 | 10 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 45 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 120 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 374 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 54 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiAnidadaGc | **SALTADA** | - | 130 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 28 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 26 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 47 | 9 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 72 | 99 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 21 | 12 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDosRuns | **IDENTICO** | 334 | 44 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 58 | 68 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 28 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 2977 | 14 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 23 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 56 | 13 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2243 | 30 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 432 | - | KILL a los 500 ms; el EXITED, 6 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 423 | - | KILL a los 500 ms; el EXITED, 8 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 453 | - | KILL a los 500 ms; el EXITED, 9 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 63 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 143 | 11 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 47 | 11 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 105 | 11 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 50 | 11 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 525 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 491 | 75 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1693 | 22 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 401 | 15 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 23 | 9 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 28 | 76 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 104 | 12 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 544 | 12 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 18 | 24 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 3958 | 25 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=79798 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13604 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=80316 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=14352 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=81254 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13604 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=81854 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13440 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6632
- tras Destructor: uptimeMs=82495 fsUsed=188416 vmHeapBytes=5767168 pila libre: run=13156 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6632
- tras TimeDemo: uptimeMs=83462 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13264 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6632
- tras ArrayFijo: uptimeMs=84002 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13156 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6632
- tras MoveRango: uptimeMs=84572 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12976 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6632
- tras HiloUsa: uptimeMs=85239 fsUsed=188416 vmHeapBytes=5767168 pila libre: run=12976 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6632
- tras NatNew: uptimeMs=86312 fsUsed=188416 vmHeapBytes=5767168 pila libre: run=12976 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6352
- tras NatV7: uptimeMs=87280 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13156 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6352
- tras FloatNat: uptimeMs=88424 fsUsed=192512 vmHeapBytes=5767168 pila libre: run=13156 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6352
- tras NatCede: uptimeMs=89355 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13156 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6352
- tras NatExpropia: uptimeMs=90221 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12976 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6352
- tras NatDosRuns: uptimeMs=91581 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13140 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6072
- tras NatBuiltin: uptimeMs=92477 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13140 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6072
- tras NatAnida: uptimeMs=93329 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12976 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6072
- tras NatDirecta: uptimeMs=94304 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13140 io=8976 tareaREPL=8976 heap del sistema, minimo libre=6072
- tras NatCoste: uptimeMs=98071 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12976 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras NatNombres: uptimeMs=98963 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13140 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras NatEstrecha: uptimeMs=99888 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12976 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras DblNat: uptimeMs=103256 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13140 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras KillLoop: uptimeMs=103944 fsUsed=180224 vmHeapBytes=5767168 pila libre: run=13140 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras NatKill: uptimeMs=105030 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13140 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras NatRecurKill: uptimeMs=106209 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12976 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras CatCadena: uptimeMs=106751 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13140 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras CatCrono: uptimeMs=107311 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12976 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras SbPiezas: uptimeMs=107841 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13140 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras SbCrono: uptimeMs=108373 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=13140 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras HeapInfo: uptimeMs=108925 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12832 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras HeapCompact: uptimeMs=110332 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12676 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras HeapReposo: uptimeMs=111333 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12676 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras DescarteAncho: uptimeMs=113498 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12672 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras TempRetiene: uptimeMs=114463 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12672 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras FinallyAnidado: uptimeMs=114981 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12676 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras RtcEnHora: uptimeMs=115500 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12672 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras FicheroOCarpeta: uptimeMs=116086 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12180 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras GcHondo: uptimeMs=117050 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12672 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras ImportHondo: uptimeMs=119401 fsUsed=233472 vmHeapBytes=5767168 pila libre: run=12180 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- tras synclisttest: uptimeMs=124906 fsUsed=184320 vmHeapBytes=5767168 pila libre: run=12672 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6072
- ejecutadas (con EXITED) 40 de 44 listadas; no terminaron 0; saltadas 4; no llegaron a arrancar 0

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
| /app/NatDosRuns.mod | bpgenvm-c\build\tanda\NatDosRuns\aot-ARMV8M_MAIN_SOFTFP\NatDosRuns.mod | 3540 | db2b330a | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2215 | 18ea246e | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | 6205214f | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/DblNat.mod | bpgenvm-c\build\tanda\DblNat\aot-ARMV8M_MAIN_SOFTFP\DblNat.mod | 2540 | ac5be2ec | ok |
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/NatKill.mod | bpgenvm-c\build\tanda\NatKill\aot-ARMV8M_MAIN_SOFTFP\NatKill.mod | 485 | 228703af | ok |
| /app/NatRecurKill.mod | bpgenvm-c\build\tanda\NatRecurKill\aot-ARMV8M_MAIN_SOFTFP\NatRecurKill.mod | 606 | efa300c8 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/GcHondo.mod | bpgenvm-c\build\tanda\GcHondo\GcHondo.mod | 1584 | 38aa99a6 | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |
| /app/SyncListTest.mod | bpgenvm-c\build\tanda\synclisttest\SyncListTest.mod | 3860 | 958dfea6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: DESFASE de compilación —la placa tiene en pack:SQLite/Str.mod la versión 7 (>= la mínima 7), crc 8e56f753 != el de hoy 9765bb28: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

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
| GuiArbolG2 | SALTADA |
| GuiAnidadaGc | SALTADA |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatDosRuns | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| DblNat | IDENTICO |
| KillLoop | PARADA |
| NatKill | PARADA |
| NatRecurKill | PARADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | IDENTICO |
| ImportHondo | IDENTICO |
| synclisttest | ASERCIONES |

MEMORIA APRETADA: synclisttest: NO ES VERDE: ninguna placa de la tanda que aprieta la ha corrido (sin PSRAM y <= 1 MB de SRAM: la Pico 2, la C6, la C3)  
SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiArbolG2 en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiAnidadaGc en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-06 16:29:30

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c5@COM6)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 333 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 18 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 51 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 372 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 61 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 154 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 10 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 11 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 8 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 87 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 14 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 43 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 57 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 11 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 16 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 11 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 94 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 10 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 15 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 15 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 10 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 10 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 19 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 61 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 25 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 21 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 9 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 11 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 12 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 12 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 43 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 26 | 11 |

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Oct  6 2026 07:41:06` imageCrc=`c2d175a4` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c5/build/bpvm_esp32c5.bin (06-10 16:28)
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=software uptimeMs=38217 fs=233472/16777216 B vmHeapBytes=6288384
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 21 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 12 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 12 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 315 | 333 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 11 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 19 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 30 | 18 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 15 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 22 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 28 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 759 | 510 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 508 | 365 | EXITED OK exit=0 |
| GuiArbolG2 | **IDENTICO** | 626 | 55 | EXITED OK exit=0; host rc=0 |
| GuiAnidadaGc | **IDENTICO** | 21601 | 134 | EXITED OK exit=0; host rc=0 |
| NatNew | **IDENTICO** | 15 | 10 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 15 | 11 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 17 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |
| NatCede | **IDENTICO** | 65 | 87 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatExpropia | **IDENTICO** | 18 | 14 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMC_ILP32) |
| NatDosRuns | **IDENTICO** | 278 | 43 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMC_ILP32) |
| NatBuiltin | **IDENTICO** | 50 | 57 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMC_ILP32) |
| NatAnida | **IDENTICO** | 12 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatDirecta | **IDENTICO** | 13 | 11 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMC_ILP32) |
| NatCoste | **IDENTICO** | 747 | 16 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatNombres | **IDENTICO** | 13 | 11 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| NatEstrecha | **IDENTICO** | 21 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| DblNat | **IDENTICO** | 3397 | 94 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMC_ILP32) |
| KillLoop | **PARADA** | 429 | - | KILL a los 500 ms; el EXITED, 50 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 430 | - | KILL a los 500 ms; el EXITED, 41 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMC_ILP32) |
| NatRecurKill | **PARADA** | 448 | - | KILL a los 500 ms; el EXITED, 3 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMC_ILP32) |
| CatCadena | **IDENTICO** | 26 | 10 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 94 | 15 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 28 | 15 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 115 | 10 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 27 | 10 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 397 | 19 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 353 | 61 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1581 | 25 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 449 | 21 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 12 | 9 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 15 | 11 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 65 | 12 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 509 | 12 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 12 | 43 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1466 | 26 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=39052 fsUsed=237568 vmHeapBytes=6288384 pila libre: run=13764 io=10032 tareaREPL=10032 heap del sistema, minimo libre=208487
- tras ThrowSinAtrapar: uptimeMs=39539 fsUsed=237568 vmHeapBytes=6288384 pila libre: run=13764 io=10032 tareaREPL=10032 heap del sistema, minimo libre=208487
- tras ThreadTrasMain: uptimeMs=40471 fsUsed=237568 vmHeapBytes=6288384 pila libre: run=13560 io=10032 tareaREPL=10032 heap del sistema, minimo libre=208163
- tras MachineId: uptimeMs=40972 fsUsed=237568 vmHeapBytes=6288384 pila libre: run=13560 io=8880 tareaREPL=8880 heap del sistema, minimo libre=208163
- tras Destructor: uptimeMs=41488 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=13400 io=8880 tareaREPL=8880 heap del sistema, minimo libre=207795
- tras TimeDemo: uptimeMs=42872 fsUsed=237568 vmHeapBytes=6288384 pila libre: run=13400 io=8880 tareaREPL=8880 heap del sistema, minimo libre=203107
- tras ArrayFijo: uptimeMs=43373 fsUsed=237568 vmHeapBytes=6288384 pila libre: run=13400 io=8880 tareaREPL=8880 heap del sistema, minimo libre=203107
- tras MoveRango: uptimeMs=43860 fsUsed=237568 vmHeapBytes=6288384 pila libre: run=12864 io=8880 tareaREPL=8880 heap del sistema, minimo libre=203107
- tras HiloUsa: uptimeMs=44792 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=12864 io=8880 tareaREPL=8880 heap del sistema, minimo libre=203107
- tras GuiWinJson: uptimeMs=54997 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=12400 io=8880 tareaREPL=8880 heap del sistema, minimo libre=169159
- tras GuiShot: uptimeMs=64427 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.shot` (3350 B, crc f0589b1a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png`; antes del RUN: no existia
- tras GuiArbolG2: uptimeMs=74487 fsUsed=245760 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras GuiAnidadaGc: uptimeMs=104826 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatNew: uptimeMs=105782 fsUsed=245760 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatV7: uptimeMs=106782 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras FloatNat: uptimeMs=107801 fsUsed=249856 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatCede: uptimeMs=108653 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatExpropia: uptimeMs=109585 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatDosRuns: uptimeMs=110886 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatBuiltin: uptimeMs=111786 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatAnida: uptimeMs=112638 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatDirecta: uptimeMs=113555 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatCoste: uptimeMs=115271 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatNombres: uptimeMs=116241 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatEstrecha: uptimeMs=117142 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras DblNat: uptimeMs=121362 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras KillLoop: uptimeMs=122053 fsUsed=237568 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatKill: uptimeMs=123103 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras NatRecurKill: uptimeMs=124153 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras CatCadena: uptimeMs=124682 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras CatCrono: uptimeMs=125182 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras SbPiezas: uptimeMs=125699 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras SbCrono: uptimeMs=126602 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras HeapInfo: uptimeMs=127150 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras HeapCompact: uptimeMs=128902 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras HeapReposo: uptimeMs=130219 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras DescarteAncho: uptimeMs=132374 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras TempRetiene: uptimeMs=133306 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras FinallyAnidado: uptimeMs=133839 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras RtcEnHora: uptimeMs=134326 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras FicheroOCarpeta: uptimeMs=135208 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras GcHondo: uptimeMs=136140 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras ImportHondo: uptimeMs=139011 fsUsed=290816 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- tras synclisttest: uptimeMs=143147 fsUsed=241664 vmHeapBytes=6288384 pila libre: run=11428 io=8880 tareaREPL=8880 heap del sistema, minimo libre=156271
- ejecutadas (con EXITED) 44 de 44 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | a79a2400 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | e9faef8a | ok |
| /app/GuiArbolG2.mod | bpgenvm-c\build\tanda\GuiArbolG2\GuiArbolG2.mod | 5379 | b33c4dbf | ok (por trozos) |
| /app/GuiAnidadaGc.mod | bpgenvm-c\build\tanda\GuiAnidadaGc\GuiAnidadaGc.mod | 1121 | 82388d9f | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4483 | a4d0e489 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2447 | 7def5e1d | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9740 | 7ad95292 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMC_ILP32\NatCede.mod | 2325 | 898b5cda | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMC_ILP32\NatExpropia.mod | 1166 | 8b6c96ec | ok |
| /app/NatDosRuns.mod | bpgenvm-c\build\tanda\NatDosRuns\aot-RV32IMC_ILP32\NatDosRuns.mod | 3672 | dde0972e | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMC_ILP32\NatBuiltin.mod | 2794 | f745c5e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMC_ILP32\NatAnida.mod | 1232 | 359b3eec | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMC_ILP32\NatDirecta.mod | 2591 | 0a7357b1 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMC_ILP32\NatCoste.mod | 1000 | 27be448c | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMC_ILP32\NatNombres.mod | 2722 | 975650c8 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMC_ILP32\NatEstrecha.mod | 3131 | 01c97fbf | ok |
| /app/DblNat.mod | bpgenvm-c\build\tanda\DblNat\aot-RV32IMC_ILP32\DblNat.mod | 2768 | 86917e14 | ok |
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/NatKill.mod | bpgenvm-c\build\tanda\NatKill\aot-RV32IMC_ILP32\NatKill.mod | 513 | fb6da01d | ok |
| /app/NatRecurKill.mod | bpgenvm-c\build\tanda\NatRecurKill\aot-RV32IMC_ILP32\NatRecurKill.mod | 674 | c20f1991 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/GcHondo.mod | bpgenvm-c\build\tanda\GcHondo\GcHondo.mod | 1584 | 38aa99a6 | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |
| /app/SyncListTest.mod | bpgenvm-c\build\tanda\synclisttest\SyncListTest.mod | 3860 | 958dfea6 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:DriverSpi/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: DESFASE de compilación —la placa tiene en /lib/Gui.mod la versión 7 (>= la mínima 7), crc 7b6dd517 != el de hoy c0bede1a: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba la imagen—
- dependencia Json: ya en /lib/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

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
| GuiArbolG2 | IDENTICO |
| GuiAnidadaGc | IDENTICO |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatDosRuns | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| DblNat | IDENTICO |
| KillLoop | PARADA |
| NatKill | PARADA |
| NatRecurKill | PARADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | IDENTICO |
| ImportHondo | IDENTICO |
| synclisttest | ASERCIONES |

MEMORIA APRETADA: synclisttest: NO ES VERDE: ninguna placa de la tanda que aprieta la ha corrido (sin PSRAM y <= 1 MB de SRAM: la Pico 2, la C6, la C3)  
NECESITA OJOS: GuiShot en esp32c5@COM6: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png  


## Tanda 2026-10-06 16:31:38

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 316 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 12 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 15 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 12 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 10 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 13 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 58 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 370 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 55 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 128 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 11 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 8 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 101 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 42 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 56 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 11 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 8 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 15 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 10 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 10 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 28 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 9 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 11 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 13 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 10 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 19 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 59 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 22 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 15 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 12 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 10 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 11 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 14 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 55 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 26 | 11 |

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Oct  6 2026 16:29:54` imageCrc=`96cc76b4` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.elf (06-10 16:29)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=104040 fs=294912/516096 B vmHeapBytes=196608
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 17 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 197 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 8 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 316 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 24 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 239 | 10 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 104 | 15 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 80 | 12 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 220 | 10 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 62 | 13 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 58 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 370 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 55 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiAnidadaGc | **SALTADA** | - | 128 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 86 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 95 | 11 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 286 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 63 | 101 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDosRuns | **IDENTICO** | 281 | 42 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 62 | 56 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 25 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 62 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 940 | 15 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 42 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 207 | 10 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2327 | 28 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 629 | - | KILL a los 500 ms; el EXITED, 16 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 655 | - | KILL a los 500 ms; el EXITED, 19 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 647 | - | KILL a los 500 ms; el EXITED, 18 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 179 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 94 | 11 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 99 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 87 | 13 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 83 | 10 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 312 | 19 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 272 | 59 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1030 | 22 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 340 | 15 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 32 | 12 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 70 | 10 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 77 | 11 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 360 | 14 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 17 | 55 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1070 | 26 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=105029 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=105620 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=106229 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=106836 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras Destructor: uptimeMs=107849 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras TimeDemo: uptimeMs=109010 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayFijo: uptimeMs=109786 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MoveRango: uptimeMs=110679 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HiloUsa: uptimeMs=111553 fsUsed=311296 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatNew: uptimeMs=112953 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatV7: uptimeMs=114065 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FloatNat: uptimeMs=115886 fsUsed=311296 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatCede: uptimeMs=117045 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatExpropia: uptimeMs=118025 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatDosRuns: uptimeMs=119256 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatBuiltin: uptimeMs=120402 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatAnida: uptimeMs=121395 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatDirecta: uptimeMs=122524 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatCoste: uptimeMs=124344 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatNombres: uptimeMs=125480 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatEstrecha: uptimeMs=126715 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DblNat: uptimeMs=129968 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras KillLoop: uptimeMs=130816 fsUsed=294912 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatKill: uptimeMs=132032 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatRecurKill: uptimeMs=133311 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CatCadena: uptimeMs=134217 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CatCrono: uptimeMs=134843 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SbPiezas: uptimeMs=135593 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SbCrono: uptimeMs=136225 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HeapInfo: uptimeMs=136915 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HeapCompact: uptimeMs=138247 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HeapReposo: uptimeMs=139478 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras DescarteAncho: uptimeMs=141044 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras TempRetiene: uptimeMs=142341 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FinallyAnidado: uptimeMs=143083 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras RtcEnHora: uptimeMs=143774 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FicheroOCarpeta: uptimeMs=144394 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GcHondo: uptimeMs=145509 fsUsed=303104 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ImportHondo: uptimeMs=147591 fsUsed=401408 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras synclisttest: uptimeMs=151746 fsUsed=319488 vmHeapBytes=196608 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 40 de 44 listadas; no terminaron 0; saltadas 4; no llegaron a arrancar 0

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
| /app/NatDosRuns.mod | bpgenvm-c\build\tanda\NatDosRuns\aot-ARMV8M_MAIN_SOFTFP\NatDosRuns.mod | 3540 | db2b330a | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2215 | 18ea246e | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | 6205214f | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/DblNat.mod | bpgenvm-c\build\tanda\DblNat\aot-ARMV8M_MAIN_SOFTFP\DblNat.mod | 2540 | ac5be2ec | ok |
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/NatKill.mod | bpgenvm-c\build\tanda\NatKill\aot-ARMV8M_MAIN_SOFTFP\NatKill.mod | 485 | 228703af | ok |
| /app/NatRecurKill.mod | bpgenvm-c\build\tanda\NatRecurKill\aot-ARMV8M_MAIN_SOFTFP\NatRecurKill.mod | 606 | efa300c8 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/GcHondo.mod | bpgenvm-c\build\tanda\GcHondo\GcHondo.mod | 1584 | 38aa99a6 | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12305 | 263868cb | ok (por trozos) |
| /app/SyncListTest.mod | bpgenvm-c\build\tanda\synclisttest\SyncListTest.mod | 3860 | 958dfea6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Collections: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib
- dependencia Str: DESFASE de compilación —la placa tiene en pack:SQLite/Str.mod la versión 7 (>= la mínima 7), crc 8e56f753 != el de hoy 9765bb28: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot); quedan en /lib, subidos por la tanda: /lib/Collections.mod

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
| GuiArbolG2 | SALTADA |
| GuiAnidadaGc | SALTADA |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatDosRuns | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| DblNat | IDENTICO |
| KillLoop | PARADA |
| NatKill | PARADA |
| NatRecurKill | PARADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | IDENTICO |
| ImportHondo | IDENTICO |
| synclisttest | ASERCIONES |

MEMORIA APRETADA: synclisttest: verde en las que aprietan: stm32u575@COM5  
SALTADA: GuiWinJson en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiArbolG2 en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiAnidadaGc en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-06 16:33:56

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c3@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 329 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 11 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 16 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 54 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 381 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 53 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 133 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 49 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 14 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 10 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 97 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 12 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 43 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 60 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 9 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 14 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 31 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 12 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 16 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 10 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 13 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 14 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 25 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 59 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 23 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 21 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 14 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 19 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 16 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 32 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 119 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 283 | 11 |

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Oct  6 2026 07:40:52` imageCrc=`b4b58a9b` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c3/build/bpvm_esp32c3.bin (06-10 16:33)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=29738 fs=180224/1523712 B vmHeapBytes=49152
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 19 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 7 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 4 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 312 | 329 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 4 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 17 | 11 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 38 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 11 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 20 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 36 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 54 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 381 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 53 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiAnidadaGc | **SALTADA** | - | 133 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 9 | 49 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 8 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 13 | 10 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |
| NatCede | **IDENTICO** | 75 | 97 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatExpropia | **IDENTICO** | 11 | 12 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMC_ILP32) |
| NatDosRuns | **IDENTICO** | 573 | 43 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMC_ILP32) |
| NatBuiltin | **IDENTICO** | 41 | 60 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMC_ILP32) |
| NatAnida | **IDENTICO** | 7 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatDirecta | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMC_ILP32) |
| NatCoste | **IDENTICO** | 1209 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatNombres | **IDENTICO** | 7 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| NatEstrecha | **IDENTICO** | 19 | 14 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| DblNat | **IDENTICO** | 5670 | 31 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMC_ILP32) |
| KillLoop | **PARADA** | 433 | - | KILL a los 500 ms; el EXITED, 34 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 427 | - | KILL a los 500 ms; el EXITED, 26 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMC_ILP32) |
| NatRecurKill | **PARADA** | 416 | - | KILL a los 500 ms; el EXITED, 33 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMC_ILP32) |
| CatCadena | **IDENTICO** | 32 | 12 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 136 | 16 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 33 | 10 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 170 | 13 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 37 | 14 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 684 | 25 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 538 | 59 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 2362 | 23 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 671 | 21 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 6 | 14 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 10 | 19 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 96 | 16 | EXITED OK exit=0; host rc=0 |
| GcHondo | **SALTADA** | - | 32 | necesita 180 KB de heap y la VM de esta placa da 48 KB (vmHeapBytes del INFO) |
| ImportHondo | **IDENTICO** | 5 | 119 | EXITED OK exit=0; host rc=0 |
| synclisttest | **SALTADA** | - | 283 | necesita 80 KB de heap y la VM de esta placa da 48 KB (vmHeapBytes del INFO) |

- tras MathRango: uptimeMs=30307 fsUsed=184320 vmHeapBytes=49152 pila libre: run=9668 io=9072 tareaREPL=9072 heap del sistema, minimo libre=103156
- tras ThrowSinAtrapar: uptimeMs=30774 fsUsed=184320 vmHeapBytes=49152 pila libre: run=9668 io=9072 tareaREPL=9072 heap del sistema, minimo libre=103156
- tras ThreadTrasMain: uptimeMs=31279 fsUsed=184320 vmHeapBytes=49152 pila libre: run=9440 io=9056 tareaREPL=9056 heap del sistema, minimo libre=102808
- tras MachineId: uptimeMs=31774 fsUsed=184320 vmHeapBytes=49152 pila libre: run=9440 io=9008 tareaREPL=9008 heap del sistema, minimo libre=99468
- tras Destructor: uptimeMs=32291 fsUsed=188416 vmHeapBytes=49152 pila libre: run=9304 io=8960 tareaREPL=8960 heap del sistema, minimo libre=98096
- tras TimeDemo: uptimeMs=33259 fsUsed=184320 vmHeapBytes=49152 pila libre: run=9304 io=8960 tareaREPL=8960 heap del sistema, minimo libre=93392
- tras ArrayFijo: uptimeMs=33729 fsUsed=184320 vmHeapBytes=49152 pila libre: run=9304 io=8960 tareaREPL=8960 heap del sistema, minimo libre=93392
- tras MoveRango: uptimeMs=34209 fsUsed=184320 vmHeapBytes=49152 pila libre: run=9012 io=8960 tareaREPL=8960 heap del sistema, minimo libre=93392
- tras HiloUsa: uptimeMs=34727 fsUsed=188416 vmHeapBytes=49152 pila libre: run=9012 io=8960 tareaREPL=8960 heap del sistema, minimo libre=93392
- tras NatNew: uptimeMs=35878 fsUsed=188416 vmHeapBytes=49152 pila libre: run=9012 io=8960 tareaREPL=8960 heap del sistema, minimo libre=92936
- tras NatV7: uptimeMs=36881 fsUsed=184320 vmHeapBytes=49152 pila libre: run=9012 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras FloatNat: uptimeMs=37913 fsUsed=192512 vmHeapBytes=49152 pila libre: run=9012 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatCede: uptimeMs=38797 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatExpropia: uptimeMs=39669 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatDosRuns: uptimeMs=40968 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatBuiltin: uptimeMs=41902 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatAnida: uptimeMs=42706 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatDirecta: uptimeMs=43587 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatCoste: uptimeMs=45670 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatNombres: uptimeMs=46554 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatEstrecha: uptimeMs=47478 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras DblNat: uptimeMs=54156 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras KillLoop: uptimeMs=54757 fsUsed=180224 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatKill: uptimeMs=55790 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras NatRecurKill: uptimeMs=56731 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras CatCadena: uptimeMs=57217 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras CatCrono: uptimeMs=57716 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras SbPiezas: uptimeMs=58183 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras SbCrono: uptimeMs=58651 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras HeapInfo: uptimeMs=59134 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8528 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras HeapCompact: uptimeMs=60466 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8488 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras HeapReposo: uptimeMs=61821 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8488 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras DescarteAncho: uptimeMs=64806 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8488 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras TempRetiene: uptimeMs=66155 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8488 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras FinallyAnidado: uptimeMs=66657 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8488 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras RtcEnHora: uptimeMs=67560 fsUsed=184320 vmHeapBytes=49152 pila libre: run=8488 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras FicheroOCarpeta: uptimeMs=68477 fsUsed=184320 vmHeapBytes=49152 pila libre: run=7756 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- tras ImportHondo: uptimeMs=70446 fsUsed=233472 vmHeapBytes=49152 pila libre: run=7756 io=8960 tareaREPL=8960 heap del sistema, minimo libre=90504
- ejecutadas (con EXITED) 38 de 44 listadas; no terminaron 0; saltadas 6; no llegaron a arrancar 0

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
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4483 | a4d0e489 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2447 | 7def5e1d | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9740 | 7ad95292 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMC_ILP32\NatCede.mod | 2325 | 898b5cda | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMC_ILP32\NatExpropia.mod | 1166 | 8b6c96ec | ok |
| /app/NatDosRuns.mod | bpgenvm-c\build\tanda\NatDosRuns\aot-RV32IMC_ILP32\NatDosRuns.mod | 3672 | dde0972e | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMC_ILP32\NatBuiltin.mod | 2794 | f745c5e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMC_ILP32\NatAnida.mod | 1232 | 359b3eec | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMC_ILP32\NatDirecta.mod | 2591 | 0a7357b1 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMC_ILP32\NatCoste.mod | 1000 | 27be448c | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMC_ILP32\NatNombres.mod | 2722 | 975650c8 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMC_ILP32\NatEstrecha.mod | 3131 | 01c97fbf | ok |
| /app/DblNat.mod | bpgenvm-c\build\tanda\DblNat\aot-RV32IMC_ILP32\DblNat.mod | 2768 | 86917e14 | ok |
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/NatKill.mod | bpgenvm-c\build\tanda\NatKill\aot-RV32IMC_ILP32\NatKill.mod | 513 | fb6da01d | ok |
| /app/NatRecurKill.mod | bpgenvm-c\build\tanda\NatRecurKill\aot-RV32IMC_ILP32\NatRecurKill.mod | 674 | c20f1991 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

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
| GuiArbolG2 | SALTADA |
| GuiAnidadaGc | SALTADA |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatDosRuns | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| DblNat | IDENTICO |
| KillLoop | PARADA |
| NatKill | PARADA |
| NatRecurKill | PARADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | SALTADA |
| ImportHondo | IDENTICO |
| synclisttest | SALTADA |

MEMORIA APRETADA: synclisttest: NO ES VERDE: ninguna placa de la tanda que aprieta la ha corrido (sin PSRAM y <= 1 MB de SRAM: la Pico 2, la C6, la C3)  
SALTADA: GuiWinJson en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiArbolG2 en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiAnidadaGc en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GcHondo en esp32c3@COM3: necesita 180 KB de heap y la VM de esta placa da 48 KB (vmHeapBytes del INFO)  
SALTADA: synclisttest en esp32c3@COM3: necesita 80 KB de heap y la VM de esta placa da 48 KB (vmHeapBytes del INFO)  


## Tanda 2026-10-06 16:38:17

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 324 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 19 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 10 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 19 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 60 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 373 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 54 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 130 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 8 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 100 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 71 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 45 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 61 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 10 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 9 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 12 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 28 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 9 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 10 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 18 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 71 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 23 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 14 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 9 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 11 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 11 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 14 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 58 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 27 | 11 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  6 2026 07:41:23` imageCrc=`c2d3bb43` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c6/build/bpvm_esp32c6.bin (06-10 16:37)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=45506 fs=32768/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 0 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 6 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 4 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 324 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 4 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 12 | 10 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 28 | 19 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 9 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 16 | 10 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 29 | 19 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 948 | 51 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 563 | 375 | EXITED OK exit=0 |
| GuiArbolG2 | **IDENTICO** | 869 | 52 | EXITED OK exit=0; host rc=0 |
| GuiAnidadaGc | **IDENTICO** | 33571 | 131 | EXITED OK exit=0; host rc=0 |
| NatNew | **IDENTICO** | 7 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 8 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 11 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |
| NatCede | **IDENTICO** | 70 | 100 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatExpropia | **IDENTICO** | 10 | 71 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMC_ILP32) |
| NatDosRuns | **IDENTICO** | 451 | 45 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMC_ILP32) |
| NatBuiltin | **IDENTICO** | 41 | 61 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMC_ILP32) |
| NatAnida | **IDENTICO** | 5 | 10 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatDirecta | **IDENTICO** | 7 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMC_ILP32) |
| NatCoste | **IDENTICO** | 1218 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMC_ILP32) |
| NatNombres | **IDENTICO** | 6 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| NatEstrecha | **IDENTICO** | 12 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| DblNat | **IDENTICO** | 5417 | 28 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMC_ILP32) |
| KillLoop | **PARADA** | 624 | - | KILL a los 500 ms; el EXITED, 31 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 628 | - | KILL a los 500 ms; el EXITED, 6 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMC_ILP32) |
| NatRecurKill | **PARADA** | 595 | - | KILL a los 500 ms; el EXITED, 28 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMC_ILP32) |
| CatCadena | **IDENTICO** | 26 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 120 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 27 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 167 | 10 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 31 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 521 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 421 | 71 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 2425 | 23 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 680 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 6 | 9 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 9 | 11 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 86 | 11 | EXITED OK exit=0; host rc=0 |
| GcHondo | **SALTADA** | - | 14 | necesita 180 KB de heap y la VM de esta placa da 64 KB (vmHeapBytes del INFO) |
| ImportHondo | **IDENTICO** | 5 | 58 | EXITED OK exit=0; host rc=0 |
| synclisttest | **SALTADA** | - | 27 | necesita 80 KB de heap y la VM de esta placa da 64 KB (vmHeapBytes del INFO) |

- tras MathRango: uptimeMs=46239 fsUsed=36864 vmHeapBytes=65536 pila libre: run=7620 io=5960 tareaREPL=5960 heap del sistema, minimo libre=75600
- tras ThrowSinAtrapar: uptimeMs=46752 fsUsed=36864 vmHeapBytes=65536 pila libre: run=7620 io=5944 tareaREPL=5944 heap del sistema, minimo libre=75584
- tras ThreadTrasMain: uptimeMs=47672 fsUsed=36864 vmHeapBytes=65536 pila libre: run=7416 io=5944 tareaREPL=5944 heap del sistema, minimo libre=75276
- tras MachineId: uptimeMs=48241 fsUsed=36864 vmHeapBytes=65536 pila libre: run=7416 io=5912 tareaREPL=5912 heap del sistema, minimo libre=72092
- tras Destructor: uptimeMs=48807 fsUsed=40960 vmHeapBytes=65536 pila libre: run=7344 io=5912 tareaREPL=5912 heap del sistema, minimo libre=70740
- tras TimeDemo: uptimeMs=49826 fsUsed=36864 vmHeapBytes=65536 pila libre: run=7344 io=5912 tareaREPL=5912 heap del sistema, minimo libre=66036
- tras ArrayFijo: uptimeMs=50352 fsUsed=36864 vmHeapBytes=65536 pila libre: run=7344 io=5912 tareaREPL=5912 heap del sistema, minimo libre=66036
- tras MoveRango: uptimeMs=50877 fsUsed=36864 vmHeapBytes=65536 pila libre: run=6720 io=5912 tareaREPL=5912 heap del sistema, minimo libre=66036
- tras HiloUsa: uptimeMs=51894 fsUsed=40960 vmHeapBytes=65536 pila libre: run=6720 io=4872 tareaREPL=4872 heap del sistema, minimo libre=66036
- tras GuiWinJson: uptimeMs=54697 fsUsed=40960 vmHeapBytes=65536 pila libre: run=6160 io=4872 tareaREPL=4872 heap del sistema, minimo libre=6980
- tras GuiShot: uptimeMs=56765 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2644
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: no existia
- tras GuiArbolG2: uptimeMs=58962 fsUsed=45056 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2288
- tras GuiAnidadaGc: uptimeMs=93365 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatNew: uptimeMs=94266 fsUsed=45056 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatV7: uptimeMs=95098 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras FloatNat: uptimeMs=96102 fsUsed=49152 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatCede: uptimeMs=96936 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatExpropia: uptimeMs=97802 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatDosRuns: uptimeMs=99037 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatBuiltin: uptimeMs=99905 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatAnida: uptimeMs=100723 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatDirecta: uptimeMs=101672 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatCoste: uptimeMs=103756 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatNombres: uptimeMs=104642 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatEstrecha: uptimeMs=105509 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras DblNat: uptimeMs=111765 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras KillLoop: uptimeMs=112563 fsUsed=36864 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatKill: uptimeMs=113764 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras NatRecurKill: uptimeMs=114914 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras CatCadena: uptimeMs=115402 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras CatCrono: uptimeMs=115885 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras SbPiezas: uptimeMs=116354 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras SbCrono: uptimeMs=116837 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras HeapInfo: uptimeMs=117387 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras HeapCompact: uptimeMs=118338 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras HeapReposo: uptimeMs=119254 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras DescarteAncho: uptimeMs=122225 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras TempRetiene: uptimeMs=123526 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras FinallyAnidado: uptimeMs=124026 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras RtcEnHora: uptimeMs=124544 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras FicheroOCarpeta: uptimeMs=125041 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=4872 tareaREPL=4872 heap del sistema, minimo libre=2144
- tras ImportHondo: uptimeMs=128130 fsUsed=90112 vmHeapBytes=65536 pila libre: run=5284 io=4808 tareaREPL=4808 heap del sistema, minimo libre=2144
- ejecutadas (con EXITED) 42 de 44 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

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
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | a79a2400 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | e9faef8a | ok |
| /app/GuiArbolG2.mod | bpgenvm-c\build\tanda\GuiArbolG2\GuiArbolG2.mod | 5379 | b33c4dbf | ok (por trozos) |
| /app/GuiAnidadaGc.mod | bpgenvm-c\build\tanda\GuiAnidadaGc\GuiAnidadaGc.mod | 1121 | 82388d9f | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4483 | a4d0e489 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2447 | 7def5e1d | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9740 | 7ad95292 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMC_ILP32\NatCede.mod | 2325 | 898b5cda | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMC_ILP32\NatExpropia.mod | 1166 | 8b6c96ec | ok |
| /app/NatDosRuns.mod | bpgenvm-c\build\tanda\NatDosRuns\aot-RV32IMC_ILP32\NatDosRuns.mod | 3672 | dde0972e | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMC_ILP32\NatBuiltin.mod | 2794 | f745c5e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMC_ILP32\NatAnida.mod | 1232 | 359b3eec | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMC_ILP32\NatDirecta.mod | 2591 | 0a7357b1 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMC_ILP32\NatCoste.mod | 1000 | 27be448c | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMC_ILP32\NatNombres.mod | 2722 | 975650c8 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMC_ILP32\NatEstrecha.mod | 3131 | 01c97fbf | ok |
| /app/DblNat.mod | bpgenvm-c\build\tanda\DblNat\aot-RV32IMC_ILP32\DblNat.mod | 2768 | 86917e14 | ok |
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/NatKill.mod | bpgenvm-c\build\tanda\NatKill\aot-RV32IMC_ILP32\NatKill.mod | 513 | fb6da01d | ok |
| /app/NatRecurKill.mod | bpgenvm-c\build\tanda\NatRecurKill\aot-RV32IMC_ILP32\NatRecurKill.mod | 674 | c20f1991 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:GuiMini/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: DESFASE de compilación —la placa tiene en pack:GuiMini/Gui.mod la versión 7 (>= la mínima 7), crc 7b6dd517 != el de hoy c0bede1a: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Json: en la zona de packs (pack:GuiMini/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:GuiMini/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:GuiMini/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: en la zona de packs (pack:Stdlib/Heap.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

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
| GuiArbolG2 | IDENTICO |
| GuiAnidadaGc | IDENTICO |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatDosRuns | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| DblNat | IDENTICO |
| KillLoop | PARADA |
| NatKill | PARADA |
| NatRecurKill | PARADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | SALTADA |
| ImportHondo | IDENTICO |
| synclisttest | SALTADA |

MEMORIA APRETADA: synclisttest: NO ES VERDE: ninguna placa de la tanda que aprieta la ha corrido (sin PSRAM y <= 1 MB de SRAM: la Pico 2, la C6, la C3)  
SALTADA: GcHondo en esp32c6@COM3: necesita 180 KB de heap y la VM de esta placa da 64 KB (vmHeapBytes del INFO)  
SALTADA: synclisttest en esp32c6@COM3: necesita 80 KB de heap y la VM de esta placa da 64 KB (vmHeapBytes del INFO)  
NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  


## Tanda 2026-10-06 16:46:25

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 314 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 10 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 15 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 14 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 48 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 385 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 52 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 136 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 11 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 9 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 96 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 43 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 68 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 9 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 28 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 9 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 11 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 11 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 10 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 18 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 61 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 24 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 15 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 8 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 12 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 15 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 12 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 22 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 24 | 11 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  6 2026 16:34:13` imageCrc=`4d7adfe2` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (06-10 16:34)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=34300 fs=212992/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 16 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 198 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 9 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 312 | 314 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 25 | 10 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 240 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 106 | 15 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 81 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 221 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 65 | 14 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 543 | 52 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 642 | 374 | EXITED OK exit=0 |
| GuiArbolG2 | **IDENTICO** | 799 | 56 | EXITED OK exit=0; host rc=0 |
| GuiAnidadaGc | **IDENTICO** | 18122 | 134 | EXITED OK exit=0; host rc=0 |
| NatNew | **IDENTICO** | 87 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 97 | 11 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 287 | 9 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 69 | 96 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 32 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDosRuns | **IDENTICO** | 285 | 43 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 63 | 68 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 26 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 62 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1124 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 43 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 207 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2933 | 28 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 434 | - | KILL a los 500 ms; el EXITED, 18 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 618 | - | KILL a los 500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 429 | - | KILL a los 500 ms; el EXITED, 19 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 182 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 91 | 11 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 102 | 11 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 106 | 10 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 85 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 296 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 268 | 61 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1191 | 24 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 439 | 15 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 33 | 8 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 71 | 12 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 100 | 15 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 378 | 12 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 19 | 22 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1310 | 24 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=35418 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=36015 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=36633 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=37267 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras Destructor: uptimeMs=38685 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras TimeDemo: uptimeMs=39859 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayFijo: uptimeMs=40637 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MoveRango: uptimeMs=41538 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HiloUsa: uptimeMs=42836 fsUsed=229376 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiWinJson: uptimeMs=45456 fsUsed=229376 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiShot: uptimeMs=47693 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: no existia
- tras GuiArbolG2: uptimeMs=51260 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiAnidadaGc: uptimeMs=70513 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatNew: uptimeMs=71880 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatV7: uptimeMs=72982 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FloatNat: uptimeMs=74798 fsUsed=245760 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatCede: uptimeMs=75908 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatExpropia: uptimeMs=76917 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatDosRuns: uptimeMs=78553 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatBuiltin: uptimeMs=79770 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatAnida: uptimeMs=80771 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatDirecta: uptimeMs=81906 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatCoste: uptimeMs=84123 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatNombres: uptimeMs=85274 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatEstrecha: uptimeMs=86459 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DblNat: uptimeMs=90497 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras KillLoop: uptimeMs=91181 fsUsed=229376 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatKill: uptimeMs=92417 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatRecurKill: uptimeMs=93478 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CatCadena: uptimeMs=94399 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CatCrono: uptimeMs=95033 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SbPiezas: uptimeMs=95767 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SbCrono: uptimeMs=96412 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HeapInfo: uptimeMs=97100 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HeapCompact: uptimeMs=98418 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HeapReposo: uptimeMs=100070 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DescarteAncho: uptimeMs=102005 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras TempRetiene: uptimeMs=103289 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FinallyAnidado: uptimeMs=104023 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras RtcEnHora: uptimeMs=105124 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FicheroOCarpeta: uptimeMs=106158 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GcHondo: uptimeMs=107275 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ImportHondo: uptimeMs=109326 fsUsed=335872 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras synclisttest: uptimeMs=111725 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 44 de 44 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | a79a2400 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | e9faef8a | ok |
| /app/GuiArbolG2.mod | bpgenvm-c\build\tanda\GuiArbolG2\GuiArbolG2.mod | 5379 | b33c4dbf | ok (por trozos) |
| /app/GuiAnidadaGc.mod | bpgenvm-c\build\tanda\GuiAnidadaGc\GuiAnidadaGc.mod | 1121 | 82388d9f | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4151 | 0b2e4308 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2295 | 6d5eecb8 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8456 | 3612384a | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2221 | 63be81f6 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1126 | dfa8ab63 | ok |
| /app/NatDosRuns.mod | bpgenvm-c\build\tanda\NatDosRuns\aot-ARMV8M_MAIN_SOFTFP\NatDosRuns.mod | 3540 | db2b330a | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2215 | 18ea246e | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | 6205214f | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/DblNat.mod | bpgenvm-c\build\tanda\DblNat\aot-ARMV8M_MAIN_SOFTFP\DblNat.mod | 2540 | ac5be2ec | ok |
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/NatKill.mod | bpgenvm-c\build\tanda\NatKill\aot-ARMV8M_MAIN_SOFTFP\NatKill.mod | 485 | 228703af | ok |
| /app/NatRecurKill.mod | bpgenvm-c\build\tanda\NatRecurKill\aot-ARMV8M_MAIN_SOFTFP\NatRecurKill.mod | 606 | efa300c8 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/GcHondo.mod | bpgenvm-c\build\tanda\GcHondo\GcHondo.mod | 1584 | 38aa99a6 | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |
| /app/SyncListTest.mod | bpgenvm-c\build\tanda\synclisttest\SyncListTest.mod | 3860 | 958dfea6 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Gui/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: DESFASE de compilación —la placa tiene en pack:Gui/Gui.mod la versión 7 (>= la mínima 7), crc 7b6dd517 != el de hoy c0bede1a: vale y NO se sube, pero la prueba corre con la suya; para probar la de hoy, regraba el pack—
- dependencia Json: en la zona de packs (pack:Gui/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Gui/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Gui/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

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
| GuiArbolG2 | IDENTICO |
| GuiAnidadaGc | IDENTICO |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatDosRuns | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| DblNat | IDENTICO |
| KillLoop | PARADA |
| NatKill | PARADA |
| NatRecurKill | PARADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | IDENTICO |
| ImportHondo | IDENTICO |
| synclisttest | ASERCIONES |

MEMORIA APRETADA: synclisttest: NO ES VERDE: ninguna placa de la tanda que aprieta la ha corrido (sin PSRAM y <= 1 MB de SRAM: la Pico 2, la C6, la C3)  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  


## Tanda 2026-10-06 18:07:31

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 332 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 19 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 60 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 373 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 55 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 137 | 1 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 10 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 12 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 12 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 98 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatDosRuns | NatDosRuns | 3e2e992b | 3016 | ok | 0 | 42 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 49 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 68 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 10 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 28 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 9 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 11 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 13 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 10 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 11 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 19 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 60 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 21 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 15 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 9 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 10 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 12 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 12 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 23 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 24 | 11 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  6 2026 16:34:13` imageCrc=`4d7adfe2` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (06-10 16:34)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=31576 fs=212992/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 16 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 198 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 9 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 332 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 24 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 240 | 10 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 106 | 19 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 81 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 221 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 64 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 544 | 49 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 640 | 383 | EXITED OK exit=0 |
| GuiArbolG2 | **IDENTICO** | 800 | 54 | EXITED OK exit=0; host rc=0 |
| GuiAnidadaGc | **IDENTICO** | 18124 | 142 | EXITED OK exit=0; host rc=0 |
| NatNew | **IDENTICO** | 87 | 10 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 96 | 12 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 288 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 69 | 98 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 32 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDosRuns | **IDENTICO** | 286 | 42 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 62 | 49 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 26 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 62 | 68 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1125 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 43 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 208 | 10 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2931 | 28 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 644 | - | KILL a los 500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 648 | - | KILL a los 500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 647 | - | KILL a los 500 ms; el EXITED, 17 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 181 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 91 | 11 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 102 | 13 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 106 | 10 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 85 | 11 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 295 | 19 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 263 | 60 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1189 | 21 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 433 | 15 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 32 | 9 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 71 | 10 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 81 | 12 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 380 | 12 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 19 | 23 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1309 | 24 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=32658 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=33243 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=33830 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=34461 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras Destructor: uptimeMs=35478 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras TimeDemo: uptimeMs=36640 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayFijo: uptimeMs=37413 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras MoveRango: uptimeMs=38295 fsUsed=221184 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HiloUsa: uptimeMs=39163 fsUsed=229376 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiWinJson: uptimeMs=41165 fsUsed=229376 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiShot: uptimeMs=43418 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: no existia
- tras GuiArbolG2: uptimeMs=46955 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiAnidadaGc: uptimeMs=66088 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatNew: uptimeMs=67539 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatV7: uptimeMs=68642 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FloatNat: uptimeMs=70441 fsUsed=245760 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatCede: uptimeMs=71543 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatExpropia: uptimeMs=72544 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatDosRuns: uptimeMs=73778 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatBuiltin: uptimeMs=74929 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatAnida: uptimeMs=75981 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatDirecta: uptimeMs=77116 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatCoste: uptimeMs=79317 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatNombres: uptimeMs=80466 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatEstrecha: uptimeMs=81651 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DblNat: uptimeMs=85638 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras KillLoop: uptimeMs=86488 fsUsed=229376 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatKill: uptimeMs=87705 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras NatRecurKill: uptimeMs=88940 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CatCadena: uptimeMs=89855 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras CatCrono: uptimeMs=90487 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SbPiezas: uptimeMs=91220 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras SbCrono: uptimeMs=91837 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HeapInfo: uptimeMs=92536 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HeapCompact: uptimeMs=93872 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras HeapReposo: uptimeMs=95089 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras DescarteAncho: uptimeMs=97070 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras TempRetiene: uptimeMs=98337 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FinallyAnidado: uptimeMs=99060 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras RtcEnHora: uptimeMs=99740 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras FicheroOCarpeta: uptimeMs=100389 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras GcHondo: uptimeMs=101491 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras ImportHondo: uptimeMs=104006 fsUsed=335872 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- tras synclisttest: uptimeMs=106826 fsUsed=237568 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 44 de 44 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | a79a2400 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | e9faef8a | ok |
| /app/GuiArbolG2.mod | bpgenvm-c\build\tanda\GuiArbolG2\GuiArbolG2.mod | 5379 | b33c4dbf | ok (por trozos) |
| /app/GuiAnidadaGc.mod | bpgenvm-c\build\tanda\GuiAnidadaGc\GuiAnidadaGc.mod | 1121 | 82388d9f | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4151 | 0b2e4308 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2295 | 6d5eecb8 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8456 | 3612384a | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2221 | 63be81f6 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1126 | dfa8ab63 | ok |
| /app/NatDosRuns.mod | bpgenvm-c\build\tanda\NatDosRuns\aot-ARMV8M_MAIN_SOFTFP\NatDosRuns.mod | 3540 | db2b330a | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2586 | 7c4d5b08 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1164 | e0fc87f8 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2215 | 18ea246e | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 932 | 6205214f | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2462 | d9a3367a | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2895 | 1ae05cb1 | ok |
| /app/DblNat.mod | bpgenvm-c\build\tanda\DblNat\aot-ARMV8M_MAIN_SOFTFP\DblNat.mod | 2540 | ac5be2ec | ok |
| /app/KillLoop.mod | bpgenvm-c\build\tanda\KillLoop\KillLoop.mod | 233 | 1be38af8 | ok |
| /app/NatKill.mod | bpgenvm-c\build\tanda\NatKill\aot-ARMV8M_MAIN_SOFTFP\NatKill.mod | 485 | 228703af | ok |
| /app/NatRecurKill.mod | bpgenvm-c\build\tanda\NatRecurKill\aot-ARMV8M_MAIN_SOFTFP\NatRecurKill.mod | 606 | efa300c8 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3879 | 30413235 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 886 | 6ae56af1 | ok |
| /app/SbPiezas.mod | bpgenvm-c\build\tanda\SbPiezas\SbPiezas.mod | 1979 | 635b7c22 | ok |
| /app/SbCrono.mod | bpgenvm-c\build\tanda\SbCrono\SbCrono.mod | 819 | 0e798ff0 | ok |
| /app/HeapInfo.mod | bpgenvm-c\build\tanda\HeapInfo\HeapInfo.mod | 1225 | 34f97321 | ok |
| /app/HeapCompact.mod | bpgenvm-c\build\tanda\HeapCompact\HeapCompact.mod | 3577 | 9adcb888 | ok |
| /app/HeapReposo.mod | bpgenvm-c\build\tanda\HeapReposo\HeapReposo.mod | 2803 | 839f347d | ok |
| /app/DescarteAncho.mod | bpgenvm-c\build\tanda\DescarteAncho\DescarteAncho.mod | 1730 | 4c807172 | ok |
| /app/TempRetiene.mod | bpgenvm-c\build\tanda\TempRetiene\TempRetiene.mod | 3177 | e04b9177 | ok |
| /app/FinallyAnidado.mod | bpgenvm-c\build\tanda\FinallyAnidado\FinallyAnidado.mod | 1951 | 8fe62074 | ok |
| /app/RtcEnHora.mod | bpgenvm-c\build\tanda\RtcEnHora\RtcEnHora.mod | 1301 | 3991790b | ok |
| /app/FicheroOCarpeta.mod | bpgenvm-c\build\tanda\FicheroOCarpeta\FicheroOCarpeta.mod | 792 | 497ca2db | ok |
| /app/GcHondo.mod | bpgenvm-c\build\tanda\GcHondo\GcHondo.mod | 1584 | 38aa99a6 | ok |
| /app/Eslabon01.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon01.mod | 319 | 1356c3d7 | ok |
| /app/Eslabon02.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon02.mod | 319 | 59acc834 | ok |
| /app/Eslabon03.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon03.mod | 319 | c44464fa | ok |
| /app/Eslabon04.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon04.mod | 319 | 9de149dc | ok |
| /app/Eslabon05.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon05.mod | 323 | 31aa5b74 | ok |
| /app/Eslabon06.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon06.mod | 323 | cdc0744e | ok |
| /app/Eslabon07.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon07.mod | 323 | fec0338d | ok |
| /app/Eslabon08.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon08.mod | 323 | cb32ff8b | ok |
| /app/Eslabon09.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon09.mod | 323 | d505b221 | ok |
| /app/Eslabon10.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon10.mod | 323 | 62d4b243 | ok |
| /app/Eslabon11.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon11.mod | 323 | f6069ee5 | ok |
| /app/Eslabon12.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon12.mod | 323 | de5dfd58 | ok |
| /app/Eslabon13.mod | bpgenvm-c\build\tanda\ImportHondo\Eslabon13.mod | 247 | 9e425c80 | ok |
| /app/ImportHondo.mod | bpgenvm-c\build\tanda\ImportHondo\ImportHondo.mod | 399 | 127b7202 | ok |
| /app/SyncListTest.mod | bpgenvm-c\build\tanda\synclisttest\SyncListTest.mod | 3860 | 958dfea6 | ok |

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
- dependencia Heap: ya en /lib/Heap.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia IO: ya en /lib/IO.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Eslabon01: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon02: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon03: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon04: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon05: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon06: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon07: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon08: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon09: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon10: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon11: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon12: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Eslabon13: no es de bpstdlib: modulo de la app, va a /app con el programa
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

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
| GuiArbolG2 | IDENTICO |
| GuiAnidadaGc | IDENTICO |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatDosRuns | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |
| NatNombres | IDENTICO |
| NatEstrecha | IDENTICO |
| DblNat | IDENTICO |
| KillLoop | PARADA |
| NatKill | PARADA |
| NatRecurKill | PARADA |
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
| RtcEnHora | IDENTICO |
| FicheroOCarpeta | IDENTICO |
| GcHondo | IDENTICO |
| ImportHondo | IDENTICO |
| synclisttest | ASERCIONES |

MEMORIA APRETADA: synclisttest: NO ES VERDE: ninguna placa de la tanda que aprieta la ha corrido (sin PSRAM y <= 1 MB de SRAM: la Pico 2, la C6, la C3)  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  

