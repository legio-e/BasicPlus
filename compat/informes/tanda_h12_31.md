# Tandas T1 — las placas conducidas

## Tanda 2026-10-05 16:00:09

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 14:27:28) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 315 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 14 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 12 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 13 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 58 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 384 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 69 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 75 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 14 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 17 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 101 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 10 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 63 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 15 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 14 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 14 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 14 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 35 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 21 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 17 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 15 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 19 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 14 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 21 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 59 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 24 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 22 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 17 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 13 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 34 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 17 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 34 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 29 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  5 2026 15:59:02` imageCrc=`a12064fa` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (05-10 15:59)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=58767 fs=217088/1048576 B vmHeapBytes=239920
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 28 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 13 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 73 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 306 | 315 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 7 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 26 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 31 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 13 | 12 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 32 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 25 | 13 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 58 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 384 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 69 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 13 | 75 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 13 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 24 | 17 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 50 | 101 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 12 | 10 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 42 | 63 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 8 | 15 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 12 | 14 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 862 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 29 | 14 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 1983 | 35 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 425 | - | KILL a los 500 ms; el EXITED, 6 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 420 | - | KILL a los 500 ms; el EXITED, 8 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 422 | - | KILL a los 500 ms; el EXITED, 6 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 32 | 21 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 84 | 17 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 25 | 15 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 79 | 19 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 26 | 14 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 323 | 21 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 327 | 59 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1189 | 24 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 312 | 22 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 8 | 17 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 13 | 13 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 122 | 34 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 349 | 17 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 7 | 34 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1520 | 29 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=59466 fsUsed=221184 vmHeapBytes=239920 pila libre: run=13440 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6752
- tras ThrowSinAtrapar: uptimeMs=60029 fsUsed=221184 vmHeapBytes=239920 pila libre: run=13440 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras ThreadTrasMain: uptimeMs=60969 fsUsed=221184 vmHeapBytes=239920 pila libre: run=13376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras MachineId: uptimeMs=61489 fsUsed=221184 vmHeapBytes=239920 pila libre: run=13376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras Destructor: uptimeMs=62108 fsUsed=225280 vmHeapBytes=239920 pila libre: run=13272 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras TimeDemo: uptimeMs=63090 fsUsed=221184 vmHeapBytes=239920 pila libre: run=13272 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras ArrayFijo: uptimeMs=63661 fsUsed=221184 vmHeapBytes=239920 pila libre: run=13272 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras MoveRango: uptimeMs=64190 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras HiloUsa: uptimeMs=64784 fsUsed=225280 vmHeapBytes=239920 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras NatNew: uptimeMs=65950 fsUsed=225280 vmHeapBytes=239920 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatV7: uptimeMs=66860 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras FloatNat: uptimeMs=68034 fsUsed=229376 vmHeapBytes=239920 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatCede: uptimeMs=68942 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatExpropia: uptimeMs=69851 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatBuiltin: uptimeMs=70833 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatAnida: uptimeMs=71744 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatDirecta: uptimeMs=72666 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatCoste: uptimeMs=74369 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatNombres: uptimeMs=75370 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatEstrecha: uptimeMs=76346 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras DblNat: uptimeMs=79329 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras KillLoop: uptimeMs=79963 fsUsed=217088 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatKill: uptimeMs=81132 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatRecurKill: uptimeMs=82248 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras CatCadena: uptimeMs=82779 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras CatCrono: uptimeMs=83293 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras SbPiezas: uptimeMs=83829 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras SbCrono: uptimeMs=84391 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras HeapInfo: uptimeMs=84922 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12384 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras HeapCompact: uptimeMs=85883 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras HeapReposo: uptimeMs=86835 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras DescarteAncho: uptimeMs=88687 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras TempRetiene: uptimeMs=89639 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FinallyAnidado: uptimeMs=90170 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras RtcEnHora: uptimeMs=90707 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FicheroOCarpeta: uptimeMs=91653 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras GcHondo: uptimeMs=92637 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras ImportHondo: uptimeMs=94739 fsUsed=270336 vmHeapBytes=239920 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras synclisttest: uptimeMs=97730 fsUsed=221184 vmHeapBytes=239920 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- ejecutadas (con EXITED) 39 de 42 listadas; no terminaron 0; saltadas 3; no llegaron a arrancar 0

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

MEMORIA APRETADA: synclisttest: verde en las que aprietan: rp2350a@COM22  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiArbolG2 en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-05 16:32:48

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 14:27:28) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 322 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 16 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 18 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 52 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 372 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 52 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 8 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 12 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 8 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 110 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 12 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 48 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 8 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 8 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 11 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 29 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 10 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 10 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 13 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 18 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 69 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 23 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 15 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 12 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 11 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 13 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 12 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 56 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 24 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  5 2026 16:26:13` imageCrc=`e3a0573f` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: OTRA: la compilada en el PC (bpgenvm-c/pico/build/bpvm_pico.elf (05-10 16:30)) da 525a1982
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=watchdog uptimeMs=35176 fs=278528/1048576 B vmHeapBytes=236240
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 30 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 19 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 79 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 38 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 40 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 17 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 43 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 30 | 18 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 52 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 372 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 52 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 17 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 15 | 12 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 29 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 62 | 110 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 15 | 12 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 47 | 48 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 11 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 18 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 2098 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 13 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 38 | 11 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2148 | 29 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 419 | - | KILL a los 500 ms; el EXITED, 6 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 418 | - | KILL a los 500 ms; el EXITED, 7 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 418 | - | KILL a los 500 ms; el EXITED, 9 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 44 | 10 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 109 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 32 | 10 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 88 | 13 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 35 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 384 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 419 | 69 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1393 | 23 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 347 | 15 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 12 | 12 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 17 | 11 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 155 | 13 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 407 | 12 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 8 | 56 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1893 | 24 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=35905 fsUsed=282624 vmHeapBytes=236240 pila libre: run=13568 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4480
- tras ThrowSinAtrapar: uptimeMs=36430 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13568 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras ThreadTrasMain: uptimeMs=37378 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13408 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras MachineId: uptimeMs=37940 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13408 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras Destructor: uptimeMs=38609 fsUsed=282624 vmHeapBytes=236240 pila libre: run=13296 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras TimeDemo: uptimeMs=39561 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13184 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras ArrayFijo: uptimeMs=40115 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13184 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras MoveRango: uptimeMs=40662 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras HiloUsa: uptimeMs=41353 fsUsed=282624 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatNew: uptimeMs=42405 fsUsed=282624 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatV7: uptimeMs=43278 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras FloatNat: uptimeMs=44412 fsUsed=286720 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatCede: uptimeMs=45389 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatExpropia: uptimeMs=46237 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatBuiltin: uptimeMs=47131 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatAnida: uptimeMs=48071 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatDirecta: uptimeMs=48989 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatCoste: uptimeMs=51955 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatNombres: uptimeMs=52845 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatEstrecha: uptimeMs=53760 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras DblNat: uptimeMs=56761 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras KillLoop: uptimeMs=57448 fsUsed=274432 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatKill: uptimeMs=58533 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras NatRecurKill: uptimeMs=59636 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras CatCadena: uptimeMs=60178 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras CatCrono: uptimeMs=60695 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras SbPiezas: uptimeMs=61279 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras SbCrono: uptimeMs=61803 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras HeapInfo: uptimeMs=62353 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras HeapCompact: uptimeMs=63736 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras HeapReposo: uptimeMs=65171 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12280 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras DescarteAncho: uptimeMs=66940 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12280 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras TempRetiene: uptimeMs=67884 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12280 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras FinallyAnidado: uptimeMs=68421 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12280 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras RtcEnHora: uptimeMs=68956 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12280 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras FicheroOCarpeta: uptimeMs=69937 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras GcHondo: uptimeMs=70867 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras ImportHondo: uptimeMs=73095 fsUsed=327680 vmHeapBytes=236240 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- tras synclisttest: uptimeMs=76479 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=4368
- ejecutadas (con EXITED) 39 de 42 listadas; no terminaron 0; saltadas 3; no llegaron a arrancar 0

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

- la placa NO lleva la imagen compilada en el PC: imageCrc e3a0573f, bpgenvm-c/pico/build/bpvm_pico.elf (05-10 16:30) da 525a1982
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

MEMORIA APRETADA: synclisttest: verde en las que aprietan: rp2350a@COM22  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiArbolG2 en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-10-05 16:37:24

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 14:27:28) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 328 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 11 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 11 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 18 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 10 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 14 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 54 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 390 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 59 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 13 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 51 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 103 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 23 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 47 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 13 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 9 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 14 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 11 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 31 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 9 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 11 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 11 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 12 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 11 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 21 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 73 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 29 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 17 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 12 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 12 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 16 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 14 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 25 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 27 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  5 2026 16:26:13` imageCrc=`e3a0573f` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: OTRA: la compilada en el PC (bpgenvm-c/pico/build/bpvm_pico.elf (05-10 16:30)) da 525a1982
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=watchdog uptimeMs=64265 fs=274432/1048576 B vmHeapBytes=236240
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 29 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 20 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 71 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 308 | 328 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 9 | 11 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 37 | 11 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 41 | 18 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 17 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 45 | 10 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 30 | 14 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 54 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 390 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 59 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 16 | 13 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 15 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 31 | 51 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 62 | 103 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 15 | 23 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 47 | 47 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 17 | 9 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1844 | 14 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 13 | 11 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 41 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2123 | 31 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 418 | - | KILL a los 500 ms; el EXITED, 10 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 428 | - | KILL a los 500 ms; el EXITED, 8 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 417 | - | KILL a los 500 ms; el EXITED, 7 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 47 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 108 | 11 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 34 | 11 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 87 | 12 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 33 | 11 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 381 | 21 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 420 | 73 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1387 | 29 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 342 | 17 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 13 | 12 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 18 | 12 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 193 | 16 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 404 | 14 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 8 | 25 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1885 | 27 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=65020 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13440 io=9064 tareaREPL=9064 heap del sistema, minimo libre=4368
- tras ThrowSinAtrapar: uptimeMs=65533 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13440 io=9064 tareaREPL=9064 heap del sistema, minimo libre=4368
- tras ThreadTrasMain: uptimeMs=66468 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13440 io=9064 tareaREPL=9064 heap del sistema, minimo libre=4368
- tras MachineId: uptimeMs=67009 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13440 io=9064 tareaREPL=9064 heap del sistema, minimo libre=4368
- tras Destructor: uptimeMs=67692 fsUsed=282624 vmHeapBytes=236240 pila libre: run=13272 io=9064 tareaREPL=9064 heap del sistema, minimo libre=4368
- tras TimeDemo: uptimeMs=68675 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13272 io=9064 tareaREPL=9064 heap del sistema, minimo libre=4368
- tras ArrayFijo: uptimeMs=69204 fsUsed=278528 vmHeapBytes=236240 pila libre: run=13272 io=9064 tareaREPL=9064 heap del sistema, minimo libre=4368
- tras MoveRango: uptimeMs=69751 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9064 tareaREPL=9064 heap del sistema, minimo libre=4368
- tras HiloUsa: uptimeMs=70393 fsUsed=282624 vmHeapBytes=236240 pila libre: run=12792 io=9064 tareaREPL=9064 heap del sistema, minimo libre=4368
- tras NatNew: uptimeMs=71440 fsUsed=282624 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatV7: uptimeMs=72342 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras FloatNat: uptimeMs=73550 fsUsed=286720 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatCede: uptimeMs=74513 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatExpropia: uptimeMs=75372 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatBuiltin: uptimeMs=76311 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatAnida: uptimeMs=77217 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatDirecta: uptimeMs=78211 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatCoste: uptimeMs=81174 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatNombres: uptimeMs=82090 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatEstrecha: uptimeMs=83011 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras DblNat: uptimeMs=85946 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras KillLoop: uptimeMs=86635 fsUsed=274432 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatKill: uptimeMs=87792 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras NatRecurKill: uptimeMs=88896 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras CatCadena: uptimeMs=89434 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12792 io=9000 tareaREPL=9000 heap del sistema, minimo libre=4368
- tras CatCrono: uptimeMs=90055 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras SbPiezas: uptimeMs=90582 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras SbCrono: uptimeMs=91104 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras HeapInfo: uptimeMs=91646 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras HeapCompact: uptimeMs=93018 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras HeapReposo: uptimeMs=94428 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras DescarteAncho: uptimeMs=96183 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras TempRetiene: uptimeMs=97125 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras FinallyAnidado: uptimeMs=97654 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras RtcEnHora: uptimeMs=98232 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12376 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras FicheroOCarpeta: uptimeMs=99168 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12176 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras GcHondo: uptimeMs=100111 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12176 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras ImportHondo: uptimeMs=102321 fsUsed=327680 vmHeapBytes=236240 pila libre: run=12176 io=8976 tareaREPL=8976 heap del sistema, minimo libre=4368
- tras synclisttest: uptimeMs=105720 fsUsed=278528 vmHeapBytes=236240 pila libre: run=12176 io=8968 tareaREPL=8968 heap del sistema, minimo libre=4368
- ejecutadas (con EXITED) 39 de 42 listadas; no terminaron 0; saltadas 3; no llegaron a arrancar 0

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

- la placa NO lleva la imagen compilada en el PC: imageCrc e3a0573f, bpgenvm-c/pico/build/bpvm_pico.elf (05-10 16:30) da 525a1982
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

MEMORIA APRETADA: synclisttest: verde en las que aprietan: rp2350a@COM22  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiArbolG2 en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  

