# Tandas T1 — las placas conducidas

## Tanda 2026-10-05 19:17:18

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 14:27:28) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 329 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 9 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 18 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 11 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 56 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 373 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 53 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 11 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 12 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 95 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 9 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 55 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 8 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 47 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 12 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 12 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 12 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 28 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 8 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 10 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 12 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 10 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 18 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 66 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 23 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 15 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 9 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 11 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 11 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 15 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 68 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 25 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  5 2026 17:02:16` imageCrc=`b7b1ade6` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: OTRA: la compilada en el PC (bpgenvm-c/pico/build/bpvm_pico.elf (05-10 17:00)) da 01011cd0
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=173903 fs=237568/1048576 B vmHeapBytes=219848
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 31 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 19 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 160 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 308 | 329 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 8 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 40 | 9 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 45 | 18 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 18 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 49 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 32 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 56 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 373 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 53 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 14 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 13 | 11 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 30 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 61 | 95 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 14 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 45 | 55 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 9 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 16 | 47 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1875 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 13 | 12 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 40 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2050 | 28 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 421 | - | KILL a los 500 ms; el EXITED, 10 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 411 | - | KILL a los 500 ms; el EXITED, 7 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 416 | - | KILL a los 500 ms; el EXITED, 7 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 53 | 8 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 154 | 10 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 36 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 89 | 12 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 45 | 10 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 536 | 18 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 565 | 66 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 2032 | 23 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 356 | 15 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 13 | 9 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 17 | 11 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 86 | 11 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 547 | 15 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 8 | 68 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 3448 | 25 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=174642 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13356 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6736
- tras ThrowSinAtrapar: uptimeMs=175163 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13352 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6736
- tras ThreadTrasMain: uptimeMs=176123 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13356 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6736
- tras MachineId: uptimeMs=176676 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13356 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6736
- tras Destructor: uptimeMs=177379 fsUsed=245760 vmHeapBytes=219848 pila libre: run=13256 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6736
- tras TimeDemo: uptimeMs=178379 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13356 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6736
- tras ArrayFijo: uptimeMs=178912 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13256 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6736
- tras MoveRango: uptimeMs=179456 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12796 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6736
- tras HiloUsa: uptimeMs=180528 fsUsed=245760 vmHeapBytes=219848 pila libre: run=13256 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6736
- tras NatNew: uptimeMs=181586 fsUsed=245760 vmHeapBytes=219848 pila libre: run=12796 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatV7: uptimeMs=182516 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13256 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras FloatNat: uptimeMs=183720 fsUsed=249856 vmHeapBytes=219848 pila libre: run=12796 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatCede: uptimeMs=184598 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13256 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatExpropia: uptimeMs=185474 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12796 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatBuiltin: uptimeMs=186402 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13256 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatAnida: uptimeMs=187388 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12796 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatDirecta: uptimeMs=188315 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13256 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatCoste: uptimeMs=191243 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12796 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatNombres: uptimeMs=192159 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13256 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatEstrecha: uptimeMs=193140 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13184 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras DblNat: uptimeMs=196098 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12796 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras KillLoop: uptimeMs=196770 fsUsed=237568 vmHeapBytes=219848 pila libre: run=12796 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatKill: uptimeMs=197957 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13184 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras NatRecurKill: uptimeMs=199069 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12796 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras CatCadena: uptimeMs=199615 fsUsed=241664 vmHeapBytes=219848 pila libre: run=13184 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras CatCrono: uptimeMs=200561 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12376 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras SbPiezas: uptimeMs=201153 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12380 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras SbCrono: uptimeMs=201695 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12376 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras HeapInfo: uptimeMs=202236 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12380 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras HeapCompact: uptimeMs=203620 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12380 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras HeapReposo: uptimeMs=205019 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12376 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras DescarteAncho: uptimeMs=207572 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12376 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras TempRetiene: uptimeMs=208518 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12268 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras FinallyAnidado: uptimeMs=209022 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12376 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras RtcEnHora: uptimeMs=209606 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12268 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras FicheroOCarpeta: uptimeMs=210135 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12176 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras GcHondo: uptimeMs=211073 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12176 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras ImportHondo: uptimeMs=213357 fsUsed=290816 vmHeapBytes=219848 pila libre: run=12268 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
- tras synclisttest: uptimeMs=218368 fsUsed=241664 vmHeapBytes=219848 pila libre: run=12176 io=9064 tareaREPL=9064 heap del sistema, minimo libre=6456
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

- la placa NO lleva la imagen compilada en el PC: imageCrc b7b1ade6, bpgenvm-c/pico/build/bpvm_pico.elf (05-10 17:00) da 01011cd0
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


## Tanda 2026-10-05 21:33:10

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 19:27:12) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 332 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 12 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 15 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 11 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 52 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 370 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 58 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 15 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 11 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 89 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 10 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 58 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 12 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 8 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 13 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 9 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 28 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 10 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 11 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 10 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 51 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 9 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 17 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 68 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 24 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 16 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 8 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 10 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 16 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 12 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 31 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 27 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  5 2026 19:33:37` imageCrc=`de4a033f` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: OTRA: la compilada en el PC (bpgenvm-c/pico/build/bpvm_pico.elf (05-10 19:33)) da 7a61cd3b
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=179061 fs=225280/1048576 B vmHeapBytes=219840
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 28 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 19 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 153 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 308 | 332 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 8 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 39 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 44 | 15 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 18 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 48 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 33 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 52 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 370 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 58 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 16 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 13 | 15 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 33 | 11 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 61 | 89 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 13 | 10 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 46 | 58 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 9 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 14 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1807 | 13 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 12 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 38 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2043 | 28 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 412 | - | KILL a los 500 ms; el EXITED, 6 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 421 | - | KILL a los 500 ms; el EXITED, 7 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 436 | - | KILL a los 500 ms; el EXITED, 7 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 53 | 10 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 147 | 11 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 35 | 10 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 89 | 51 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 41 | 9 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 480 | 17 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 634 | 68 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1957 | 24 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 350 | 16 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 13 | 8 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 17 | 10 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 86 | 16 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 521 | 12 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 8 | 31 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 3349 | 27 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=180031 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13296 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=180600 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13300 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=181558 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13296 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=182099 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13300 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras Destructor: uptimeMs=182734 fsUsed=233472 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras TimeDemo: uptimeMs=183802 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13300 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras ArrayFijo: uptimeMs=184343 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras MoveRango: uptimeMs=184900 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras HiloUsa: uptimeMs=185895 fsUsed=233472 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6632
- tras NatNew: uptimeMs=188718 fsUsed=233472 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatV7: uptimeMs=189640 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras FloatNat: uptimeMs=190768 fsUsed=237568 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatCede: uptimeMs=191666 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatExpropia: uptimeMs=192675 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatBuiltin: uptimeMs=193573 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatAnida: uptimeMs=194457 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatDirecta: uptimeMs=195394 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatCoste: uptimeMs=198326 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatNombres: uptimeMs=199297 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatEstrecha: uptimeMs=200249 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras DblNat: uptimeMs=203195 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras KillLoop: uptimeMs=203850 fsUsed=225280 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatKill: uptimeMs=205020 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras NatRecurKill: uptimeMs=206138 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12796 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras CatCadena: uptimeMs=206685 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras CatCrono: uptimeMs=207215 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12380 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras SbPiezas: uptimeMs=207848 fsUsed=229376 vmHeapBytes=219840 pila libre: run=13240 io=9084 tareaREPL=9084 heap del sistema, minimo libre=6352
- tras SbCrono: uptimeMs=208358 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12380 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras HeapInfo: uptimeMs=208923 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12380 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras HeapCompact: uptimeMs=210341 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12380 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras HeapReposo: uptimeMs=211691 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12380 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras DescarteAncho: uptimeMs=214339 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12304 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras TempRetiene: uptimeMs=215296 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12380 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras FinallyAnidado: uptimeMs=215826 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12304 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras RtcEnHora: uptimeMs=216362 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12380 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras FicheroOCarpeta: uptimeMs=216976 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12176 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras GcHondo: uptimeMs=217903 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12380 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras ImportHondo: uptimeMs=220145 fsUsed=278528 vmHeapBytes=219840 pila libre: run=12176 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
- tras synclisttest: uptimeMs=225233 fsUsed=229376 vmHeapBytes=219840 pila libre: run=12176 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6352
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

- la placa NO lleva la imagen compilada en el PC: imageCrc de4a033f, bpgenvm-c/pico/build/bpvm_pico.elf (05-10 19:33) da 7a61cd3b
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

