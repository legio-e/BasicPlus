# Tandas T1 — las placas conducidas

## Tanda 2026-10-05 13:39:05

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 13:15:23) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 315 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 18 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 11 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 10 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 53 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 379 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 53 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 11 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 9 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 92 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 61 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 9 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 8 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 13 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 10 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 28 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 10 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 13 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 9 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 18 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 74 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 25 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 14 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 12 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 12 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 12 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 14 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 33 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 26 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  5 2026 13:15:00` imageCrc=`d766e80c` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (05-10 13:15)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=52138 fs=212992/1048576 B vmHeapBytes=240088
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 26 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 14 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 66 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 307 | 315 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 6 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 27 | 10 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 30 | 18 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 13 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 25 | 10 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 53 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 379 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 53 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 12 | 11 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 12 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 22 | 9 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 52 | 92 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 13 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 42 | 61 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 8 | 9 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 11 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 867 | 13 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 27 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 1976 | 28 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 417 | - | KILL a los 500 ms; el EXITED, 5 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 425 | - | KILL a los 500 ms; el EXITED, 7 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 422 | - | KILL a los 500 ms; el EXITED, 4 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 31 | 10 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 73 | 13 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 24 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 79 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 26 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 301 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 280 | 74 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1151 | 25 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 314 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 8 | 12 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 12 | 12 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 122 | 12 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 355 | 14 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 7 | 33 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1593 | 26 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=52824 fsUsed=217088 vmHeapBytes=240088 pila libre: run=13368 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6752
- tras ThrowSinAtrapar: uptimeMs=53335 fsUsed=217088 vmHeapBytes=240088 pila libre: run=13368 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6752
- tras ThreadTrasMain: uptimeMs=54256 fsUsed=217088 vmHeapBytes=240088 pila libre: run=13368 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6752
- tras MachineId: uptimeMs=54785 fsUsed=217088 vmHeapBytes=240088 pila libre: run=13368 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6752
- tras Destructor: uptimeMs=55456 fsUsed=221184 vmHeapBytes=240088 pila libre: run=13264 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6752
- tras TimeDemo: uptimeMs=56428 fsUsed=217088 vmHeapBytes=240088 pila libre: run=13264 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6752
- tras ArrayFijo: uptimeMs=56956 fsUsed=217088 vmHeapBytes=240088 pila libre: run=13264 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6752
- tras MoveRango: uptimeMs=57489 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6752
- tras HiloUsa: uptimeMs=58126 fsUsed=221184 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6752
- tras NatNew: uptimeMs=59178 fsUsed=221184 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatV7: uptimeMs=60061 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FloatNat: uptimeMs=61193 fsUsed=225280 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatCede: uptimeMs=62057 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatExpropia: uptimeMs=62999 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatBuiltin: uptimeMs=63899 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatAnida: uptimeMs=64792 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatDirecta: uptimeMs=65696 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatCoste: uptimeMs=67426 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatNombres: uptimeMs=68350 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatEstrecha: uptimeMs=69272 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras DblNat: uptimeMs=72257 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras KillLoop: uptimeMs=72871 fsUsed=212992 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatKill: uptimeMs=73991 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatRecurKill: uptimeMs=75057 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras CatCadena: uptimeMs=75580 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras CatCrono: uptimeMs=76078 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras SbPiezas: uptimeMs=76662 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras SbCrono: uptimeMs=77174 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12792 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras HeapInfo: uptimeMs=77706 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12384 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras HeapCompact: uptimeMs=78674 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras HeapReposo: uptimeMs=79614 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras DescarteAncho: uptimeMs=81461 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras TempRetiene: uptimeMs=82396 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FinallyAnidado: uptimeMs=82911 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras RtcEnHora: uptimeMs=83426 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FicheroOCarpeta: uptimeMs=84363 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras GcHondo: uptimeMs=85364 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras ImportHondo: uptimeMs=87502 fsUsed=266240 vmHeapBytes=240088 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras synclisttest: uptimeMs=90384 fsUsed=217088 vmHeapBytes=240088 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
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


## Tanda 2026-10-05 13:41:02

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 13:15:23) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 331 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 19 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 12 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 8 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 11 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 57 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 370 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 57 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 11 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 12 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 100 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 8 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 48 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 11 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 10 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 28 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 9 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 11 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 10 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 10 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 11 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 20 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 61 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 22 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 14 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 9 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 10 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 12 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 12 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 30 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 24 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  5 2026 13:15:00` imageCrc=`d766e80c` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (05-10 13:15)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=45239 fs=212992/1048576 B vmHeapBytes=223704
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 26 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 16 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 133 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 305 | 331 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 7 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 27 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 34 | 19 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 14 | 12 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 34 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 26 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 57 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 370 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 57 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 13 | 11 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 13 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 23 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 50 | 100 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 12 | 8 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 43 | 48 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 8 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 12 | 10 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 867 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 26 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 1977 | 28 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 421 | - | KILL a los 500 ms; el EXITED, 8 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 436 | - | KILL a los 500 ms; el EXITED, 8 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 416 | - | KILL a los 500 ms; el EXITED, 5 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 34 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 96 | 11 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 25 | 10 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 81 | 10 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 32 | 11 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 357 | 20 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 372 | 61 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1440 | 22 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 323 | 14 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 13 | 10 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 129 | 12 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 427 | 12 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 7 | 30 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1769 | 24 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=45907 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13368 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras ThrowSinAtrapar: uptimeMs=46425 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13368 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras ThreadTrasMain: uptimeMs=47361 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13368 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras MachineId: uptimeMs=47889 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13368 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras Destructor: uptimeMs=48579 fsUsed=221184 vmHeapBytes=223704 pila libre: run=13368 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras TimeDemo: uptimeMs=49539 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13304 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras ArrayFijo: uptimeMs=50057 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13368 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras MoveRango: uptimeMs=50572 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras HiloUsa: uptimeMs=51193 fsUsed=221184 vmHeapBytes=223704 pila libre: run=13280 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras NatNew: uptimeMs=52243 fsUsed=221184 vmHeapBytes=223704 pila libre: run=13280 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatV7: uptimeMs=53115 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras FloatNat: uptimeMs=54219 fsUsed=225280 vmHeapBytes=223704 pila libre: run=13280 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatCede: uptimeMs=55079 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatExpropia: uptimeMs=55976 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13280 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatBuiltin: uptimeMs=56865 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatAnida: uptimeMs=57729 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13280 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatDirecta: uptimeMs=58668 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13272 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatCoste: uptimeMs=60374 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatNombres: uptimeMs=61249 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatEstrecha: uptimeMs=62121 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13152 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras DblNat: uptimeMs=65060 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras KillLoop: uptimeMs=65677 fsUsed=212992 vmHeapBytes=223704 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatKill: uptimeMs=66779 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13152 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatRecurKill: uptimeMs=67861 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13152 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras CatCadena: uptimeMs=68393 fsUsed=217088 vmHeapBytes=223704 pila libre: run=13136 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras CatCrono: uptimeMs=68914 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras SbPiezas: uptimeMs=69477 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12792 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras SbCrono: uptimeMs=69995 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras HeapInfo: uptimeMs=70524 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12384 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras HeapCompact: uptimeMs=71498 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras HeapReposo: uptimeMs=72848 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras DescarteAncho: uptimeMs=74665 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras TempRetiene: uptimeMs=75603 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras FinallyAnidado: uptimeMs=76120 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras RtcEnHora: uptimeMs=76636 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12376 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras FicheroOCarpeta: uptimeMs=77569 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12176 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras GcHondo: uptimeMs=78566 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras ImportHondo: uptimeMs=80685 fsUsed=266240 vmHeapBytes=223704 pila libre: run=12376 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras synclisttest: uptimeMs=84009 fsUsed=217088 vmHeapBytes=223704 pila libre: run=12176 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
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

