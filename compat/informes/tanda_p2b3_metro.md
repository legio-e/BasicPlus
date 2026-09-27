# Tandas T1 — las placas conducidas

## Tanda 2026-09-27 08:41:33

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (20 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-27 08:14:10) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-27 08:25:53) - sabor `gui1-lvgl0`
- placas conectadas: 1 (rp2350b@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 324 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 12 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 9 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 8 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 12 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 52 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 360 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 9 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 8 | 40 |
| NatCede | NatCede | f74a429b | 1485 | ok | 0 | 93 | 7 |
| NatExpropia | NatExpropia | bba4cdbe | 948 | ok | 0 | 8 | 4 |
| NatBuiltin | NatBuiltin | 932a14a5 | 1509 | ok | 0 | 61 | 8 |
| NatAnida | NatAnida | bb4c1fcb | 729 | ok | 0 | 8 | 3 |
| NatDirecta | NatDirecta | 38520c54 | 1071 | ok | 0 | 8 | 9 |
| NatCoste | NatCoste | d9f8a413 | 750 | ok | 0 | 12 | 2 |

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Sep 27 2026 08:18:25` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=23201 fs=233472/10248192 B vmHeapBytes=5767168
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 35 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 40 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 17 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 315 | 324 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 17 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 88 | 10 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 71 | 12 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 32 | 9 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 88 | 8 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 43 | 12 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 52 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 360 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 31 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 32 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 66 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 112 | 93 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 21 | 8 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 57 | 61 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 28 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 860 | 12 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |

- tras MathRango: uptimeMs=25224 fsUsed=233472 vmHeapBytes=5767168
- tras ThrowSinAtrapar: uptimeMs=26294 fsUsed=233472 vmHeapBytes=5767168
- tras ThreadTrasMain: uptimeMs=27371 fsUsed=233472 vmHeapBytes=5767168
- tras MachineId: uptimeMs=28659 fsUsed=233472 vmHeapBytes=5767168
- tras Destructor: uptimeMs=30897 fsUsed=233472 vmHeapBytes=5767168
- tras TimeDemo: uptimeMs=32898 fsUsed=233472 vmHeapBytes=5767168
- tras ArrayFijo: uptimeMs=34127 fsUsed=233472 vmHeapBytes=5767168
- tras MoveRango: uptimeMs=35447 fsUsed=233472 vmHeapBytes=5767168
- tras HiloUsa: uptimeMs=37153 fsUsed=233472 vmHeapBytes=5767168
- tras NatNew: uptimeMs=39752 fsUsed=233472 vmHeapBytes=5767168
- tras NatV7: uptimeMs=41492 fsUsed=233472 vmHeapBytes=5767168
- tras FloatNat: uptimeMs=44857 fsUsed=233472 vmHeapBytes=5767168
- tras NatCede: uptimeMs=46374 fsUsed=237568 vmHeapBytes=5767168
- tras NatExpropia: uptimeMs=47826 fsUsed=241664 vmHeapBytes=5767168
- tras NatBuiltin: uptimeMs=49439 fsUsed=245760 vmHeapBytes=5767168
- tras NatAnida: uptimeMs=51077 fsUsed=249856 vmHeapBytes=5767168
- tras NatDirecta: uptimeMs=52832 fsUsed=253952 vmHeapBytes=5767168
- tras NatCoste: uptimeMs=55118 fsUsed=258048 vmHeapBytes=5767168
- ejecutadas (con EXITED) 18 de 20 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

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
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |

SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  

