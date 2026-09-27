# Tandas T1 — las placas conducidas

## Tanda 2026-09-27 09:32:44

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (20 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-27 09:05:26) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-27 09:05:31) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 333 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 8 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 14 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 12 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 10 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 61 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 351 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 10 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 10 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 7 | 40 |
| NatCede | NatCede | f74a429b | 1485 | ok | 0 | 101 | 7 |
| NatExpropia | NatExpropia | bba4cdbe | 948 | ok | 0 | 8 | 4 |
| NatBuiltin | NatBuiltin | 932a14a5 | 1509 | ok | 0 | 59 | 8 |
| NatAnida | NatAnida | bb4c1fcb | 729 | ok | 0 | 11 | 3 |
| NatDirecta | NatDirecta | 38520c54 | 1071 | ok | 0 | 11 | 9 |
| NatCoste | NatCoste | d9f8a413 | 750 | ok | 0 | 11 | 2 |

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Sep 27 2026 09:30:41` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=31413 fs=229376/516096 B vmHeapBytes=393216
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 17 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 255 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 64 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 376 | 333 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 133 | 8 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 351 | 10 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 503 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 184 | 12 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 309 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 278 | 10 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 61 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 351 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 199 | 10 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 257 | 10 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 369 | 7 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 232 | 101 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 103 | 8 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 149 | 59 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 98 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 129 | 11 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1086 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |

- tras MathRango: uptimeMs=33250 fsUsed=237568 vmHeapBytes=393216
- tras ThrowSinAtrapar: uptimeMs=34329 fsUsed=245760 vmHeapBytes=393216
- tras ThreadTrasMain: uptimeMs=35644 fsUsed=253952 vmHeapBytes=393216
- tras MachineId: uptimeMs=36942 fsUsed=262144 vmHeapBytes=393216
- tras Destructor: uptimeMs=39016 fsUsed=270336 vmHeapBytes=393216
- tras TimeDemo: uptimeMs=40652 fsUsed=278528 vmHeapBytes=393216
- tras ArrayFijo: uptimeMs=41885 fsUsed=286720 vmHeapBytes=393216
- tras MoveRango: uptimeMs=43215 fsUsed=294912 vmHeapBytes=393216
- tras HiloUsa: uptimeMs=44969 fsUsed=311296 vmHeapBytes=393216
- tras NatNew: uptimeMs=47615 fsUsed=319488 vmHeapBytes=393216
- tras NatV7: uptimeMs=49218 fsUsed=327680 vmHeapBytes=393216
- tras FloatNat: uptimeMs=52489 fsUsed=344064 vmHeapBytes=393216
- tras NatCede: uptimeMs=54075 fsUsed=352256 vmHeapBytes=393216
- tras NatExpropia: uptimeMs=55760 fsUsed=360448 vmHeapBytes=393216
- tras NatBuiltin: uptimeMs=57379 fsUsed=368640 vmHeapBytes=393216
- tras NatAnida: uptimeMs=58847 fsUsed=376832 vmHeapBytes=393216
- tras NatDirecta: uptimeMs=60465 fsUsed=385024 vmHeapBytes=393216
- tras NatCoste: uptimeMs=62925 fsUsed=393216 vmHeapBytes=393216
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

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

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
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |
| NatCede | IDENTICO |
| NatExpropia | IDENTICO |
| NatBuiltin | IDENTICO |
| NatAnida | IDENTICO |
| NatDirecta | IDENTICO |
| NatCoste | IDENTICO |

SALTADA: GuiWinJson en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  

