# Tandas T1 — las placas conducidas

## Tanda 2026-10-05 09:17:58

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 08:51:28) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 326 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 36 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 17 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 11 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 13 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 49 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 406 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 52 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 13 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 13 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 38 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 103 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 10 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 58 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 10 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 13 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 42 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 15 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 12 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 34 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 27 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 14 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 12 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 9 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 10 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 20 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 57 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 23 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 19 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 10 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 10 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 13 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 14 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 43 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 28 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  5 2026 08:53:53` imageCrc=`8f96a773` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (05-10 08:54)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=74747 fs=204800/1048576 B vmHeapBytes=240112
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 25 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 13 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 68 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 307 | 326 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 7 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 25 | 36 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 33 | 17 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 14 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 34 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 26 | 13 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 49 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 406 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 52 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 12 | 13 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 12 | 13 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 22 | 38 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 51 | 103 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 13 | 10 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 42 | 58 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 8 | 10 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 12 | 13 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 821 | 42 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 9 | 15 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 30 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2409 | 34 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 416 | - | KILL a los 500 ms; el EXITED, 5 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 420 | - | KILL a los 500 ms; el EXITED, 7 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 406 | - | KILL a los 500 ms; el EXITED, 6 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 32 | 27 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 89 | 14 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 28 | 12 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 92 | 9 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 28 | 10 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 341 | 20 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 374 | 57 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1618 | 23 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 368 | 19 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 13 | 10 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 124 | 13 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 474 | 14 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 7 | 43 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 1560 | 28 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=75432 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13828 io=8992 tareaREPL=8992 heap del sistema, minimo libre=6752
- tras ThrowSinAtrapar: uptimeMs=75942 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13828 io=8992 tareaREPL=8992 heap del sistema, minimo libre=6752
- tras ThreadTrasMain: uptimeMs=76846 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13708 io=8992 tareaREPL=8992 heap del sistema, minimo libre=6752
- tras MachineId: uptimeMs=77466 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13708 io=8992 tareaREPL=8992 heap del sistema, minimo libre=6752
- tras Destructor: uptimeMs=78076 fsUsed=212992 vmHeapBytes=240112 pila libre: run=13380 io=8992 tareaREPL=8992 heap del sistema, minimo libre=6752
- tras TimeDemo: uptimeMs=79034 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13364 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras ArrayFijo: uptimeMs=79548 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13364 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras MoveRango: uptimeMs=80082 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras HiloUsa: uptimeMs=80696 fsUsed=212992 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras NatNew: uptimeMs=81736 fsUsed=212992 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatV7: uptimeMs=82648 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras FloatNat: uptimeMs=83888 fsUsed=217088 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatCede: uptimeMs=84771 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatExpropia: uptimeMs=85692 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatBuiltin: uptimeMs=86585 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatAnida: uptimeMs=87508 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatDirecta: uptimeMs=88458 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatCoste: uptimeMs=90193 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatNombres: uptimeMs=91089 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatEstrecha: uptimeMs=92009 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras DblNat: uptimeMs=95477 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras KillLoop: uptimeMs=96099 fsUsed=204800 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatKill: uptimeMs=97203 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatRecurKill: uptimeMs=98295 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras CatCadena: uptimeMs=98871 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras CatCrono: uptimeMs=99383 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras SbPiezas: uptimeMs=99914 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras SbCrono: uptimeMs=100421 fsUsed=208896 vmHeapBytes=240112 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras HeapInfo: uptimeMs=100973 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12604 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras HeapCompact: uptimeMs=102007 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12596 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras HeapReposo: uptimeMs=102967 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras DescarteAncho: uptimeMs=105188 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras TempRetiene: uptimeMs=106151 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FinallyAnidado: uptimeMs=106791 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras RtcEnHora: uptimeMs=107297 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FicheroOCarpeta: uptimeMs=108253 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12404 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras GcHondo: uptimeMs=109193 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12404 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras ImportHondo: uptimeMs=111445 fsUsed=258048 vmHeapBytes=240112 pila libre: run=12404 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras synclisttest: uptimeMs=114444 fsUsed=208896 vmHeapBytes=240112 pila libre: run=12404 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
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


## Tanda 2026-10-05 09:20:21

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (42 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-05 08:51:28) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | ba378a10 | 1611 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 2766e6a8 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 283bde27 | 532 | ok | 0 | 329 | 3 |
| MachineId | MachineId | 6670f12a | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 1fd37c26 | 4667 | ok | 0 | 12 | 33 |
| TimeDemo | TimeDemo | 7b5397fe | 1504 | ok | 0 | 16 | 13 |
| ArrayFijo | ArrayFijo | cd20e12c | 2359 | ok | 0 | 11 | 11 |
| MoveRango | MoveRango | d9f3513b | 3670 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 0c1797a2 | 2189 | ok | 0 | 11 | 8 |
| GuiWinJson | GuiWinJson | aab92b31 | 2070 | ok | 0 | 53 | 17 |
| GuiShot | GuiShot | 1b3a4360 | 1344 | ok | 0 | 374 | 8 |
| GuiArbolG2 | GuiArbolG2 | b6471624 | 5379 | ok | 0 | 52 | 39 |
| NatNew | NatNew | ff6ac50b | 2087 | ok | 0 | 12 | 10 |
| NatV7 | NatV7 | 0f03da1a | 1557 | ok | 0 | 13 | 11 |
| FloatNat | FloatNat | 8cff9966 | 4877 | ok | 0 | 12 | 40 |
| NatCede | NatCede | eeb16b3b | 1499 | ok | 0 | 118 | 7 |
| NatExpropia | NatExpropia | 560d1f57 | 905 | ok | 0 | 11 | 4 |
| NatBuiltin | NatBuiltin | c83c0cb5 | 1592 | ok | 0 | 51 | 8 |
| NatAnida | NatAnida | f2ccf6d0 | 734 | ok | 0 | 12 | 3 |
| NatDirecta | NatDirecta | 33179300 | 1018 | ok | 0 | 11 | 9 |
| NatCoste | NatCoste | 881d927c | 772 | ok | 0 | 15 | 2 |
| NatNombres | NatNombres | 15055910 | 1444 | ok | 0 | 10 | 6 |
| NatEstrecha | NatEstrecha | 49b2633d | 1767 | ok | 0 | 9 | 23 |
| DblNat | DblNat | 96142621 | 1859 | ok | 0 | 37 | 4 |
| KillLoop | KillLoop | 1be38af8 | 233 | ok | - | - | - |
| NatKill | NatKill | 9f6cf8de | 401 | ok | - | - | - |
| NatRecurKill | NatRecurKill | 536db6bb | 420 | ok | - | - | - |
| CatCadena | CatCadena | 30413235 | 3879 | ok | 0 | 9 | 24 |
| CatCrono | CatCrono | 6ae56af1 | 886 | ok | 0 | 11 | 2 |
| SbPiezas | SbPiezas | 635b7c22 | 1979 | ok | 0 | 9 | 12 |
| SbCrono | SbCrono | 0e798ff0 | 819 | ok | 0 | 12 | 2 |
| HeapInfo | HeapInfo | 34f97321 | 1225 | ok | 0 | 12 | 10 |
| HeapCompact | HeapCompact | 9adcb888 | 3577 | ok | 0 | 21 | 17 |
| HeapReposo | HeapReposo | 839f347d | 2803 | ok | 0 | 72 | 8 |
| DescarteAncho | DescarteAncho | 4c807172 | 1730 | ok | 0 | 24 | 5 |
| TempRetiene | TempRetiene | e04b9177 | 3177 | ok | 0 | 19 | 13 |
| FinallyAnidado | FinallyAnidado | 8fe62074 | 1951 | ok | 0 | 11 | 5 |
| RtcEnHora | RtcEnHora | 3991790b | 1301 | ok | 0 | 41 | 7 |
| FicheroOCarpeta | FicheroOCarpeta | 497ca2db | 792 | ok | 0 | 14 | 7 |
| GcHondo | GcHondo | 38aa99a6 | 1584 | ok | 0 | 12 | 4 |
| ImportHondo | ImportHondo | 127b7202 | 399 | ok | 0 | 23 | 2 |
| synclisttest | SyncListTest | 958dfea6 | 3860 | ok | 0 | 28 | 11 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  5 2026 08:53:53` imageCrc=`8f96a773` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (05-10 08:54)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=46569 fs=204800/1048576 B vmHeapBytes=223728
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 24 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 14 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 134 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 306 | 329 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 7 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 27 | 12 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 36 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 14 | 11 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 37 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 28 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 53 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 374 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiArbolG2 | **SALTADA** | - | 52 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 12 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 12 | 13 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 22 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 50 | 118 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 12 | 11 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 42 | 51 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 9 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 13 | 11 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 821 | 15 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 28 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| DblNat | **IDENTICO** | 2408 | 37 | EXITED OK exit=0; host rc=0; nativo 3/3 thunks (ARMV8M_MAIN_SOFTFP) |
| KillLoop | **PARADA** | 417 | - | KILL a los 500 ms; el EXITED, 25 ms despues: EXITED KILLED exit=130 (terminado por KILL) |
| NatKill | **PARADA** | 422 | - | KILL a los 500 ms; el EXITED, 6 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatRecurKill | **PARADA** | 437 | - | KILL a los 500 ms; el EXITED, 4 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 37 | 9 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 122 | 11 | EXITED OK exit=0; host rc=0 |
| SbPiezas | **IDENTICO** | 30 | 9 | EXITED OK exit=0; host rc=0 |
| SbCrono | **IDENTICO** | 99 | 12 | EXITED OK exit=0; host rc=0 |
| HeapInfo | **IDENTICO** | 36 | 12 | EXITED OK exit=0; host rc=0 |
| HeapCompact | **IDENTICO** | 451 | 21 | EXITED OK exit=0; host rc=0 |
| HeapReposo | **IDENTICO** | 505 | 72 | EXITED OK exit=0; host rc=0 |
| DescarteAncho | **IDENTICO** | 1949 | 24 | EXITED OK exit=0; host rc=0 |
| TempRetiene | **IDENTICO** | 400 | 19 | EXITED OK exit=0; host rc=0 |
| FinallyAnidado | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0 |
| RtcEnHora | **IDENTICO** | 14 | 41 | EXITED OK exit=0; host rc=0 |
| FicheroOCarpeta | **IDENTICO** | 123 | 14 | EXITED OK exit=0; host rc=0 |
| GcHondo | **IDENTICO** | 537 | 12 | EXITED OK exit=0; host rc=0 |
| ImportHondo | **IDENTICO** | 7 | 23 | EXITED OK exit=0; host rc=0 |
| synclisttest | **ASERCIONES** | 2035 | 28 | 3 comprobaciones OK y ninguna mala (EXITED OK exit=0) |

- tras MathRango: uptimeMs=47284 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13824 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6752
- tras ThrowSinAtrapar: uptimeMs=47795 fsUsed=208896 vmHeapBytes=223728 pila libre: run=14556 io=9000 tareaREPL=9000 heap del sistema, minimo libre=6752
- tras ThreadTrasMain: uptimeMs=48719 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13704 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras MachineId: uptimeMs=49265 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13716 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras Destructor: uptimeMs=49864 fsUsed=212992 vmHeapBytes=223728 pila libre: run=13596 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras TimeDemo: uptimeMs=50903 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13596 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras ArrayFijo: uptimeMs=51414 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13488 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras MoveRango: uptimeMs=51947 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras HiloUsa: uptimeMs=52553 fsUsed=212992 vmHeapBytes=223728 pila libre: run=13020 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6752
- tras NatNew: uptimeMs=53725 fsUsed=212992 vmHeapBytes=223728 pila libre: run=13488 io=8968 tareaREPL=8968 heap del sistema, minimo libre=6472
- tras NatV7: uptimeMs=54606 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13020 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FloatNat: uptimeMs=55724 fsUsed=217088 vmHeapBytes=223728 pila libre: run=13488 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatCede: uptimeMs=56610 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13020 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatExpropia: uptimeMs=57460 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13488 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatBuiltin: uptimeMs=58472 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13020 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatAnida: uptimeMs=59341 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13488 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatDirecta: uptimeMs=60242 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13020 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatCoste: uptimeMs=61924 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13020 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatNombres: uptimeMs=62827 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13020 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatEstrecha: uptimeMs=63793 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13360 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras DblNat: uptimeMs=67170 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13360 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras KillLoop: uptimeMs=67822 fsUsed=204800 vmHeapBytes=223728 pila libre: run=13360 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatKill: uptimeMs=68871 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13020 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras NatRecurKill: uptimeMs=69970 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13020 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras CatCadena: uptimeMs=70538 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13360 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras CatCrono: uptimeMs=71042 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras SbPiezas: uptimeMs=71565 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras SbCrono: uptimeMs=72108 fsUsed=208896 vmHeapBytes=223728 pila libre: run=13360 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras HeapInfo: uptimeMs=72686 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras HeapCompact: uptimeMs=74038 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12592 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras HeapReposo: uptimeMs=75401 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras DescarteAncho: uptimeMs=77975 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12592 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras TempRetiene: uptimeMs=78946 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12592 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FinallyAnidado: uptimeMs=79543 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12596 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras RtcEnHora: uptimeMs=80069 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12592 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras FicheroOCarpeta: uptimeMs=81013 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12404 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras GcHondo: uptimeMs=81949 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12592 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras ImportHondo: uptimeMs=84046 fsUsed=258048 vmHeapBytes=223728 pila libre: run=12404 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
- tras synclisttest: uptimeMs=87532 fsUsed=208896 vmHeapBytes=223728 pila libre: run=12592 io=8960 tareaREPL=8960 heap del sistema, minimo libre=6472
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

