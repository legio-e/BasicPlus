# Tandas T1 — las placas conducidas

## Tanda 2026-10-05 23:48:28

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 22:43:11) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 327 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 51 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 360 | 8 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  5 2026 22:45:55` imageCrc=`d5ffbb19` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (05-10 23:47)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=62736 fs=135168/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 6 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 192 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 327 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 21 | 10 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 696 | 49 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 577 | 375 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=63957 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13560 io=25796 tareaREPL=25796 heap del sistema, minimo libre=441299
- tras ThrowSinAtrapar: uptimeMs=64555 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13560 io=25796 tareaREPL=25796 heap del sistema, minimo libre=441299
- tras ThreadTrasMain: uptimeMs=65723 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13528 io=25796 tareaREPL=25796 heap del sistema, minimo libre=440951
- tras MachineId: uptimeMs=66407 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13528 io=25748 tareaREPL=25748 heap del sistema, minimo libre=440951
- tras GuiWinJson: uptimeMs=72313 fsUsed=143360 vmHeapBytes=24637440 pila libre: run=12528 io=24604 tareaREPL=24604 heap del sistema, minimo libre=418775
- tras GuiShot: uptimeMs=78050 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- ejecutadas (con EXITED) 6 de 6 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | ba378a10 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 2766e6a8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 283bde27 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 6670f12a | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | aab92b31 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 1b3a4360 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: ya en /lib/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)
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

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  


## Tanda 2026-10-05 23:49:16

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 22:43:11) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 367 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 23 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 18 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 50 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 363 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 57 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 57 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 91 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 14 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 58 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 9 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 29 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 53 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 13 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 17 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 67 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 23 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 14 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 12 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 13 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 12 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 15 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 48 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 24 | 11 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  5 2026 22:45:55` imageCrc=`d5ffbb19` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (05-10 23:47)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=120467 fs=135168/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 6 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 193 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 11 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 314 | 367 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 20 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 238 | 10 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 100 | 23 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 78 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 217 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 60 | 18 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 342 | 52 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 651 | 381 | EXITED OK exit=0 |
| GuiArbolG2 | **IDENTICO** | 507 | 52 | EXITED OK exit=0; host rc=0 |
| NatNew | **IDENTICO** | 83 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 93 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 283 | 57 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 74 | 91 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 27 | 14 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 59 | 58 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 22 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 60 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 604 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatNombres | **IDENTICO** | 37 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| NatEstrecha | **IDENTICO** | 206 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| DblNat | **IDENTICO** | 2097 | 29 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMAFC_ILP32F) |
| KillLoop | **PARADA** | 627 | - | KILL a los 500 ms; el EXITED, 21 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 635 | - | KILL a los 500 ms; el EXITED, 29 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatRecurKill | **PARADA** | 657 | - | KILL a los 500 ms; el EXITED, 26 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| CatCadena | **IDENTICO** | 175 | 53 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 55 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 94 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 80 | 13 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 78 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 216 | 17 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 245 | 67 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 905 | 23 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 269 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 31 | 12 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 67 | 13 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 112 | 12 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 299 | 15 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 15 | 48 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 980 | 24 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=121754 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras ThrowSinAtrapar: uptimeMs=122339 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras ThreadTrasMain: uptimeMs=123289 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras MachineId: uptimeMs=124024 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras Destructor: uptimeMs=125557 fsUsed=143360 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras TimeDemo: uptimeMs=126810 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras ArrayFijo: uptimeMs=127777 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras MoveRango: uptimeMs=128945 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras HiloUsa: uptimeMs=130430 fsUsed=143360 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras GuiWinJson: uptimeMs=136319 fsUsed=143360 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- tras GuiShot: uptimeMs=142306 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=417279
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- tras GuiArbolG2: uptimeMs=147796 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11344 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatNew: uptimeMs=149595 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatV7: uptimeMs=150881 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras FloatNat: uptimeMs=152983 fsUsed=159744 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatCede: uptimeMs=154200 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatExpropia: uptimeMs=155200 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatBuiltin: uptimeMs=156450 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatAnida: uptimeMs=157716 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatDirecta: uptimeMs=158920 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatCoste: uptimeMs=160473 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatNombres: uptimeMs=161639 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatEstrecha: uptimeMs=163105 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras DblNat: uptimeMs=166375 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras KillLoop: uptimeMs=167324 fsUsed=147456 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatKill: uptimeMs=168735 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras NatRecurKill: uptimeMs=170235 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras CatCadena: uptimeMs=171277 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras CatCrono: uptimeMs=171946 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras SbPiezas: uptimeMs=172699 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras SbCrono: uptimeMs=173382 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras HeapInfo: uptimeMs=174112 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras HeapCompact: uptimeMs=175305 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras HeapReposo: uptimeMs=176499 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras DescarteAncho: uptimeMs=178156 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras TempRetiene: uptimeMs=179370 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras FinallyAnidado: uptimeMs=180155 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras RtcEnHora: uptimeMs=181023 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras FicheroOCarpeta: uptimeMs=181757 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras GcHondo: uptimeMs=182825 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras ImportHondo: uptimeMs=186953 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- tras synclisttest: uptimeMs=191635 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11212 io=24604 tareaREPL=24604 heap del sistema, minimo libre=413247
- ejecutadas (con EXITED) 42 de 42 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | aab92b31 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 1b3a4360 | ok |
| /app/GuiArbolG2.mod | bpgenvm-c\build\tanda\GuiArbolG2\GuiArbolG2.mod | 5379 | b6471624 | ok (por trozos) |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4467 | 183a7978 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2447 | c909fac7 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9332 | 2365095d | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2325 | 6f33192a | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1166 | 0f42a895 | ok |
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

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
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


## Tanda 2026-10-05 23:51:47

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 22:43:11) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 323 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 11 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 15 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 11 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 11 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 50 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 368 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 52 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 12 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 103 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 59 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 8 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 52 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 13 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 13 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 28 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 10 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 8 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 9 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 17 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 62 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 25 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 15 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 8 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 10 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 12 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 15 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 22 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 25 | 11 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  5 2026 22:45:55` imageCrc=`d5ffbb19` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (05-10 23:47)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=34753 fs=135168/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 192 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 9 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 309 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 20 | 11 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 238 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 99 | 15 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 79 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 214 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 59 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 688 | 49 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 580 | 406 | EXITED OK exit=0 |
| GuiArbolG2 | **IDENTICO** | 509 | 56 | EXITED OK exit=0; host rc=0 |
| NatNew | **IDENTICO** | 83 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 93 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 282 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 69 | 103 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 27 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 59 | 59 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 22 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 60 | 52 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 604 | 13 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatNombres | **IDENTICO** | 38 | 13 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| NatEstrecha | **IDENTICO** | 206 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| DblNat | **IDENTICO** | 2097 | 28 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (RV32IMAFC_ILP32F) |
| KillLoop | **PARADA** | 652 | - | KILL a los 500 ms; el EXITED, 20 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 653 | - | KILL a los 500 ms; el EXITED, 23 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatRecurKill | **PARADA** | 445 | - | KILL a los 500 ms; el EXITED, 21 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| CatCadena | **IDENTICO** | 175 | 10 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 57 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 96 | 8 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 81 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 80 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 222 | 17 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 243 | 62 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 937 | 25 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 267 | 15 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 30 | 8 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 68 | 10 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 175 | 12 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 303 | 15 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 16 | 22 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1103 | 25 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=35806 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=15536 io=25796 tareaREPL=25796 heap del sistema, minimo libre=439551
- tras ThrowSinAtrapar: uptimeMs=36504 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13560 io=25756 tareaREPL=25756 heap del sistema, minimo libre=439551
- tras ThreadTrasMain: uptimeMs=37323 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13732 io=25756 tareaREPL=25756 heap del sistema, minimo libre=439551
- tras MachineId: uptimeMs=37890 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13596 io=25676 tareaREPL=25676 heap del sistema, minimo libre=439551
- tras Destructor: uptimeMs=39192 fsUsed=143360 vmHeapBytes=24637440 pila libre: run=13448 io=25676 tareaREPL=25676 heap del sistema, minimo libre=439551
- tras TimeDemo: uptimeMs=40088 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13448 io=25676 tareaREPL=25676 heap del sistema, minimo libre=437011
- tras ArrayFijo: uptimeMs=40877 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13448 io=25676 tareaREPL=25676 heap del sistema, minimo libre=437011
- tras MoveRango: uptimeMs=41911 fsUsed=139264 vmHeapBytes=24637440 pila libre: run=13076 io=25676 tareaREPL=25676 heap del sistema, minimo libre=437011
- tras HiloUsa: uptimeMs=42996 fsUsed=143360 vmHeapBytes=24637440 pila libre: run=13076 io=24668 tareaREPL=24668 heap del sistema, minimo libre=437011
- tras GuiWinJson: uptimeMs=47165 fsUsed=143360 vmHeapBytes=24637440 pila libre: run=12452 io=24588 tareaREPL=24588 heap del sistema, minimo libre=402403
- tras GuiShot: uptimeMs=51286 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=12944 io=24588 tareaREPL=24588 heap del sistema, minimo libre=400875
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- tras GuiArbolG2: uptimeMs=56990 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatNew: uptimeMs=58492 fsUsed=155648 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatV7: uptimeMs=59710 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras FloatNat: uptimeMs=61861 fsUsed=159744 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatCede: uptimeMs=62979 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatExpropia: uptimeMs=63939 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatBuiltin: uptimeMs=65212 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatAnida: uptimeMs=66213 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatDirecta: uptimeMs=67333 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatCoste: uptimeMs=68917 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatNombres: uptimeMs=70097 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatEstrecha: uptimeMs=71470 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras DblNat: uptimeMs=74738 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras KillLoop: uptimeMs=75714 fsUsed=147456 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatKill: uptimeMs=77095 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras NatRecurKill: uptimeMs=78295 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras CatCadena: uptimeMs=79344 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras CatCrono: uptimeMs=80012 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=12944 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras SbPiezas: uptimeMs=80778 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=12944 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras SbCrono: uptimeMs=81456 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=12944 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras HeapInfo: uptimeMs=82196 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=12556 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras HeapCompact: uptimeMs=83513 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras HeapReposo: uptimeMs=84747 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=12428 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras DescarteAncho: uptimeMs=86415 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras TempRetiene: uptimeMs=87627 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras FinallyAnidado: uptimeMs=88417 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=12428 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras RtcEnHora: uptimeMs=89302 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras FicheroOCarpeta: uptimeMs=90152 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras GcHondo: uptimeMs=91067 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=12428 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras ImportHondo: uptimeMs=93789 fsUsed=200704 vmHeapBytes=24637440 pila libre: run=11256 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- tras synclisttest: uptimeMs=97154 fsUsed=151552 vmHeapBytes=24637440 pila libre: run=12428 io=24588 tareaREPL=24588 heap del sistema, minimo libre=396851
- ejecutadas (con EXITED) 42 de 42 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

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
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | aab92b31 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | 1b3a4360 | ok |
| /app/GuiArbolG2.mod | bpgenvm-c\build\tanda\GuiArbolG2\GuiArbolG2.mod | 5379 | b6471624 | ok (por trozos) |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4467 | 183a7978 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2447 | c909fac7 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9332 | 2365095d | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2325 | 6f33192a | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1166 | 0f42a895 | ok |
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

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
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

