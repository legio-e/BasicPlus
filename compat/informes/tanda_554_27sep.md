# Tandas T1 — las placas conducidas

## Tanda 2026-09-27 14:43:35

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (24 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-27 11:56:14) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-27 11:56:17) - sabor `gui1-lvgl0`
- placas conectadas: 4 (rp2350b@COM4, stm32u5g9@COM12, esp32p4@COM15, stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 320 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 10 | 3 |
| Destructor | Destructor | 9a7aecca | 4622 | ok | 0 | 17 | 33 |
| TimeDemo | TimeDemo | cb7de6ba | 1508 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | 47d23d19 | 2351 | ok | 0 | 10 | 11 |
| MoveRango | MoveRango | 579ed639 | 3640 | ok | 0 | 9 | 26 |
| HiloUsa | HiloUsa | daaf5323 | 2168 | ok | 0 | 11 | 8 |
| GuiWinJson | GuiWinJson | 7bef06c8 | 2072 | ok | 0 | 54 | 17 |
| GuiShot | GuiShot | 87c018c8 | 1347 | ok | 0 | 362 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 12 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 16 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 8 | 40 |
| NatCede | NatCede | 401002ad | 1500 | ok | 0 | 108 | 7 |
| NatExpropia | NatExpropia | b1dc8b58 | 904 | ok | 0 | 9 | 4 |
| NatBuiltin | NatBuiltin | 2d83de41 | 1596 | ok | 0 | 42 | 8 |
| NatAnida | NatAnida | dd68fcc3 | 739 | ok | 0 | 8 | 3 |
| NatDirecta | NatDirecta | a6467f1a | 1027 | ok | 0 | 8 | 9 |
| NatCoste | NatCoste | d0ec0c6b | 776 | ok | 0 | 11 | 2 |
| NatNombres | NatNombres | 1fdb4187 | 1440 | ok | 0 | 8 | 6 |
| NatEstrecha | NatEstrecha | e571ad32 | 1781 | ok | 0 | 9 | 23 |
| CatCadena | CatCadena | 73adf7c4 | 3842 | ok | 0 | 12 | 24 |
| CatCrono | CatCrono | 7c1aca27 | 890 | ok | 0 | 10 | 2 |

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Sep 27 2026 10:28:28` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=30396 fs=307200/10248192 B vmHeapBytes=5767168
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 48 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 39 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 16 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 314 | 320 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 16 | 10 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 55 | 17 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 60 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 30 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 63 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 44 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 54 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 362 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 32 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 30 | 16 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 62 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 69 | 108 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 21 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 52 | 42 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 18 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 26 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 912 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 20 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 70 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 68 | 12 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 121 | 10 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=32418 fsUsed=307200 vmHeapBytes=5767168
- tras ThrowSinAtrapar: uptimeMs=33871 fsUsed=307200 vmHeapBytes=5767168
- tras ThreadTrasMain: uptimeMs=34954 fsUsed=307200 vmHeapBytes=5767168
- tras MachineId: uptimeMs=36433 fsUsed=307200 vmHeapBytes=5767168
- tras Destructor: uptimeMs=38653 fsUsed=307200 vmHeapBytes=5767168
- tras TimeDemo: uptimeMs=40274 fsUsed=307200 vmHeapBytes=5767168
- tras ArrayFijo: uptimeMs=41488 fsUsed=307200 vmHeapBytes=5767168
- tras MoveRango: uptimeMs=42828 fsUsed=307200 vmHeapBytes=5767168
- tras HiloUsa: uptimeMs=44371 fsUsed=307200 vmHeapBytes=5767168
- tras NatNew: uptimeMs=46983 fsUsed=307200 vmHeapBytes=5767168
- tras NatV7: uptimeMs=48598 fsUsed=307200 vmHeapBytes=5767168
- tras FloatNat: uptimeMs=52179 fsUsed=307200 vmHeapBytes=5767168
- tras NatCede: uptimeMs=53771 fsUsed=307200 vmHeapBytes=5767168
- tras NatExpropia: uptimeMs=55453 fsUsed=307200 vmHeapBytes=5767168
- tras NatBuiltin: uptimeMs=57049 fsUsed=307200 vmHeapBytes=5767168
- tras NatAnida: uptimeMs=58542 fsUsed=307200 vmHeapBytes=5767168
- tras NatDirecta: uptimeMs=60139 fsUsed=307200 vmHeapBytes=5767168
- tras NatCoste: uptimeMs=62439 fsUsed=307200 vmHeapBytes=5767168
- tras NatNombres: uptimeMs=64243 fsUsed=307200 vmHeapBytes=5767168
- tras NatEstrecha: uptimeMs=66078 fsUsed=307200 vmHeapBytes=5767168
- tras CatCadena: uptimeMs=67444 fsUsed=311296 vmHeapBytes=5767168
- tras CatCrono: uptimeMs=68546 fsUsed=311296 vmHeapBytes=5767168
- ejecutadas (con EXITED) 22 de 24 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4622 | 9a7aecca | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1508 | cb7de6ba | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2351 | 47d23d19 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3640 | 579ed639 | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1075 | f3a9569a | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2168 | daaf5323 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4123 | 5310b8e4 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2296 | 27b863a9 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8449 | 79aa73d4 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2222 | 48164171 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1125 | dc8bc021 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2590 | 7b1c87e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1169 | 39c761e9 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2168 | eb972d40 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 936 | b3e11350 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2458 | 3144773c | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2909 | 8c8ce537 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3842 | 73adf7c4 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 890 | 7c1aca27 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 27 2026 12:01:09` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=151332 fs=499712/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 47 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 369 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 164 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 501 | 320 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 295 | 10 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 589 | 17 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 1046 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 361 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 410 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 522 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 4441 | 38 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 4796 | 369 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 209 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 268 | 16 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 373 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 186 | 108 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 141 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 196 | 42 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 126 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 150 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1311 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 171 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 313 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 304 | 12 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 254 | 10 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=153336 fsUsed=499712 vmHeapBytes=786432
- tras ThrowSinAtrapar: uptimeMs=154385 fsUsed=499712 vmHeapBytes=786432
- tras ThreadTrasMain: uptimeMs=155486 fsUsed=499712 vmHeapBytes=786432
- tras MachineId: uptimeMs=156975 fsUsed=499712 vmHeapBytes=786432
- tras Destructor: uptimeMs=159224 fsUsed=499712 vmHeapBytes=786432
- tras TimeDemo: uptimeMs=161675 fsUsed=499712 vmHeapBytes=786432
- tras ArrayFijo: uptimeMs=162878 fsUsed=499712 vmHeapBytes=786432
- tras MoveRango: uptimeMs=164232 fsUsed=499712 vmHeapBytes=786432
- tras HiloUsa: uptimeMs=165762 fsUsed=499712 vmHeapBytes=786432
- tras GuiWinJson: uptimeMs=185813 fsUsed=581632 vmHeapBytes=786432
- tras GuiShot: uptimeMs=193042 fsUsed=581632 vmHeapBytes=786432
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=195965 fsUsed=581632 vmHeapBytes=786432
- tras NatV7: uptimeMs=197387 fsUsed=581632 vmHeapBytes=786432
- tras FloatNat: uptimeMs=199983 fsUsed=581632 vmHeapBytes=786432
- tras NatCede: uptimeMs=201207 fsUsed=581632 vmHeapBytes=786432
- tras NatExpropia: uptimeMs=202547 fsUsed=581632 vmHeapBytes=786432
- tras NatBuiltin: uptimeMs=204016 fsUsed=581632 vmHeapBytes=786432
- tras NatAnida: uptimeMs=205356 fsUsed=581632 vmHeapBytes=786432
- tras NatDirecta: uptimeMs=206572 fsUsed=581632 vmHeapBytes=786432
- tras NatCoste: uptimeMs=208513 fsUsed=581632 vmHeapBytes=786432
- tras NatNombres: uptimeMs=209938 fsUsed=581632 vmHeapBytes=786432
- tras NatEstrecha: uptimeMs=211210 fsUsed=581632 vmHeapBytes=786432
- tras CatCadena: uptimeMs=212561 fsUsed=589824 vmHeapBytes=786432
- tras CatCrono: uptimeMs=213643 fsUsed=598016 vmHeapBytes=786432
- ejecutadas (con EXITED) 24 de 24 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4622 | 9a7aecca | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1508 | cb7de6ba | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2351 | 47d23d19 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3640 | 579ed639 | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1075 | f3a9569a | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2168 | daaf5323 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45033 | a4ea2cda | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22729 | c101a51f | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4597 | cf859cfa | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2072 | 7bef06c8 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 87c018c8 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4123 | 5310b8e4 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2296 | 27b863a9 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8449 | 79aa73d4 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2222 | 48164171 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1125 | dc8bc021 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2590 | 7b1c87e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1169 | 39c761e9 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2168 | eb972d40 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 936 | b3e11350 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2458 | 3144773c | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2909 | 8c8ce537 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3842 | 73adf7c4 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 890 | 7c1aca27 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: el RUN la resuelve a pack:Gui/Gui.mod (fuera de /lib y /app), crc ae0d2a7e: se sube a /lib
- dependencia Json: el RUN la resuelve a pack:Gui/Json.mod (fuera de /lib y /app), crc 578601f2: se sube a /lib
- dependencia Collections: en la zona de packs (pack:Gui/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: el RUN la resuelve a pack:Gui/Str.mod (fuera de /lib y /app), crc 8e56f753: se sube a /lib

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 27 2026 10:31:24` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=154044 fs=569344/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 76 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 320 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 10 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 10 | 17 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 10 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 570 | 48 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 520 | 366 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| NatV7 | **IDENTICO** | 10 | 16 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| FloatNat | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMAFC_ILP32F) |
| NatCede | **IDENTICO** | 60 | 108 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatExpropia | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (RV32IMAFC_ILP32F) |
| NatBuiltin | **IDENTICO** | 50 | 42 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (RV32IMAFC_ILP32F) |
| NatAnida | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatDirecta | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (RV32IMAFC_ILP32F) |
| NatCoste | **IDENTICO** | 520 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (RV32IMAFC_ILP32F) |
| NatNombres | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| NatEstrecha | **IDENTICO** | 10 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |
| CatCadena | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 40 | 10 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=157048 fsUsed=569344 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=158527 fsUsed=569344 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=159937 fsUsed=569344 vmHeapBytes=24637440
- tras MachineId: uptimeMs=161600 fsUsed=569344 vmHeapBytes=24637440
- tras Destructor: uptimeMs=164402 fsUsed=569344 vmHeapBytes=24637440
- tras TimeDemo: uptimeMs=166811 fsUsed=569344 vmHeapBytes=24637440
- tras ArrayFijo: uptimeMs=168230 fsUsed=569344 vmHeapBytes=24637440
- tras MoveRango: uptimeMs=169839 fsUsed=569344 vmHeapBytes=24637440
- tras HiloUsa: uptimeMs=171915 fsUsed=569344 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=195593 fsUsed=569344 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=206075 fsUsed=569344 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=209674 fsUsed=569344 vmHeapBytes=24637440
- tras NatV7: uptimeMs=211654 fsUsed=569344 vmHeapBytes=24637440
- tras FloatNat: uptimeMs=215263 fsUsed=569344 vmHeapBytes=24637440
- tras NatCede: uptimeMs=217095 fsUsed=569344 vmHeapBytes=24637440
- tras NatExpropia: uptimeMs=219176 fsUsed=569344 vmHeapBytes=24637440
- tras NatBuiltin: uptimeMs=221023 fsUsed=569344 vmHeapBytes=24637440
- tras NatAnida: uptimeMs=222767 fsUsed=569344 vmHeapBytes=24637440
- tras NatDirecta: uptimeMs=224686 fsUsed=569344 vmHeapBytes=24637440
- tras NatCoste: uptimeMs=226869 fsUsed=569344 vmHeapBytes=24637440
- tras NatNombres: uptimeMs=228693 fsUsed=569344 vmHeapBytes=24637440
- tras NatEstrecha: uptimeMs=230644 fsUsed=569344 vmHeapBytes=24637440
- tras CatCadena: uptimeMs=232340 fsUsed=573440 vmHeapBytes=24637440
- tras CatCrono: uptimeMs=233937 fsUsed=577536 vmHeapBytes=24637440
- ejecutadas (con EXITED) 24 de 24 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4622 | 9a7aecca | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1508 | cb7de6ba | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2351 | 47d23d19 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3640 | 579ed639 | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1075 | f3a9569a | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2168 | daaf5323 | ok |
| /lib/Gui.mod | bpstdlib\Gui.mod | 45033 | a4ea2cda | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22729 | c101a51f | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4597 | cf859cfa | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2072 | 7bef06c8 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 87c018c8 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMAFC_ILP32F\NatNew.mod | 4439 | 1bc7ea38 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMAFC_ILP32F\NatV7.mod | 2448 | 0a332d2d | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMAFC_ILP32F\FloatNat.mod | 9325 | f98b1edf | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-RV32IMAFC_ILP32F\NatCede.mod | 2326 | 9f1acace | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-RV32IMAFC_ILP32F\NatExpropia.mod | 1165 | 494d7a7c | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-RV32IMAFC_ILP32F\NatBuiltin.mod | 2794 | 118fd946 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-RV32IMAFC_ILP32F\NatAnida.mod | 1237 | b0d74640 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-RV32IMAFC_ILP32F\NatDirecta.mod | 2540 | a64eb206 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-RV32IMAFC_ILP32F\NatCoste.mod | 1004 | 810da08f | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMAFC_ILP32F\NatNombres.mod | 2702 | 165e3c8c | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMAFC_ILP32F\NatEstrecha.mod | 3145 | 673644b2 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3842 | 73adf7c4 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 890 | 7c1aca27 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en /lib con crc ae0d2a7e != local a4ea2cda (DESFASE, se sube la de bpstdlib a /lib)
- dependencia Json: en /lib con crc 578601f2 != local c101a51f (DESFASE, se sube la de bpstdlib a /lib)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: en /lib con crc 8e56f753 != local cf859cfa (DESFASE, se sube la de bpstdlib a /lib)

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Sep 27 2026 12:00:58` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=304504 fs=491520/516096 B vmHeapBytes=196608
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 39 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 307 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 105 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 432 | 320 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 194 | 10 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 472 | 17 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 696 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 272 | 10 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 372 | 9 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 458 | 11 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **SALTADA** | - | 54 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 362 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| NatNew | **IDENTICO** | 273 | 12 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 330 | 16 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 381 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCede | **IDENTICO** | 200 | 108 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatExpropia | **IDENTICO** | 151 | 9 | EXITED OK exit=0; host rc=0; nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |
| NatBuiltin | **IDENTICO** | 208 | 42 | EXITED OK exit=0; host rc=0; nativo 6/6 thunks (ARMV8M_MAIN_SOFTFP) |
| NatAnida | **IDENTICO** | 135 | 8 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatDirecta | **IDENTICO** | 158 | 8 | EXITED OK exit=0; host rc=0; nativo 9/9 thunks (ARMV8M_MAIN_SOFTFP) |
| NatCoste | **IDENTICO** | 1140 | 11 | EXITED OK exit=0; host rc=0; nativo 2/2 thunks (ARMV8M_MAIN_SOFTFP) |
| NatNombres | **IDENTICO** | 181 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 322 | 9 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| CatCadena | **IDENTICO** | 314 | 12 | EXITED OK exit=0; host rc=0 |
| CatCrono | **IDENTICO** | 276 | 10 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=306544 fsUsed=491520 vmHeapBytes=196608
- tras ThrowSinAtrapar: uptimeMs=307798 fsUsed=491520 vmHeapBytes=196608
- tras ThreadTrasMain: uptimeMs=308905 fsUsed=491520 vmHeapBytes=196608
- tras MachineId: uptimeMs=310215 fsUsed=491520 vmHeapBytes=196608
- tras Destructor: uptimeMs=312510 fsUsed=491520 vmHeapBytes=196608
- tras TimeDemo: uptimeMs=314555 fsUsed=491520 vmHeapBytes=196608
- tras ArrayFijo: uptimeMs=315794 fsUsed=491520 vmHeapBytes=196608
- tras MoveRango: uptimeMs=317176 fsUsed=491520 vmHeapBytes=196608
- tras HiloUsa: uptimeMs=318717 fsUsed=491520 vmHeapBytes=196608
- tras NatNew: uptimeMs=320967 fsUsed=491520 vmHeapBytes=196608
- tras NatV7: uptimeMs=322205 fsUsed=491520 vmHeapBytes=196608
- tras FloatNat: uptimeMs=325027 fsUsed=491520 vmHeapBytes=196608
- tras NatCede: uptimeMs=326271 fsUsed=491520 vmHeapBytes=196608
- tras NatExpropia: uptimeMs=327633 fsUsed=491520 vmHeapBytes=196608
- tras NatBuiltin: uptimeMs=328912 fsUsed=491520 vmHeapBytes=196608
- tras NatAnida: uptimeMs=330070 fsUsed=491520 vmHeapBytes=196608
- tras NatDirecta: uptimeMs=331292 fsUsed=491520 vmHeapBytes=196608
- tras NatCoste: uptimeMs=333240 fsUsed=491520 vmHeapBytes=196608
- tras NatNombres: uptimeMs=334476 fsUsed=491520 vmHeapBytes=196608
- tras NatEstrecha: uptimeMs=335761 fsUsed=491520 vmHeapBytes=196608
- tras CatCadena: uptimeMs=337138 fsUsed=499712 vmHeapBytes=196608
- tras CatCrono: uptimeMs=338245 fsUsed=507904 vmHeapBytes=196608
- ejecutadas (con EXITED) 22 de 24 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4622 | 9a7aecca | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1508 | cb7de6ba | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2351 | 47d23d19 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3640 | 579ed639 | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1075 | f3a9569a | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2168 | daaf5323 | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4123 | 5310b8e4 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2296 | 27b863a9 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8449 | 79aa73d4 | ok (por trozos) |
| /app/NatCede.mod | bpgenvm-c\build\tanda\NatCede\aot-ARMV8M_MAIN_SOFTFP\NatCede.mod | 2222 | 48164171 | ok |
| /app/NatExpropia.mod | bpgenvm-c\build\tanda\NatExpropia\aot-ARMV8M_MAIN_SOFTFP\NatExpropia.mod | 1125 | dc8bc021 | ok |
| /app/NatBuiltin.mod | bpgenvm-c\build\tanda\NatBuiltin\aot-ARMV8M_MAIN_SOFTFP\NatBuiltin.mod | 2590 | 7b1c87e9 | ok |
| /app/NatAnida.mod | bpgenvm-c\build\tanda\NatAnida\aot-ARMV8M_MAIN_SOFTFP\NatAnida.mod | 1169 | 39c761e9 | ok |
| /app/NatDirecta.mod | bpgenvm-c\build\tanda\NatDirecta\aot-ARMV8M_MAIN_SOFTFP\NatDirecta.mod | 2168 | eb972d40 | ok |
| /app/NatCoste.mod | bpgenvm-c\build\tanda\NatCoste\aot-ARMV8M_MAIN_SOFTFP\NatCoste.mod | 936 | b3e11350 | ok |
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2458 | 3144773c | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2909 | 8c8ce537 | ok |
| /app/CatCadena.mod | bpgenvm-c\build\tanda\CatCadena\CatCadena.mod | 3842 | 73adf7c4 | ok |
| /app/CatCrono.mod | bpgenvm-c\build\tanda\CatCrono\CatCrono.mod | 890 | 7c1aca27 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa

### Matriz pruebas x placas

| prueba | rp2350b@COM4 | stm32u5g9@COM12 | esp32p4@COM15 | stm32u575@COM5 |
|---|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA | NO-PETA |
| Destructor | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| TimeDemo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ArrayFijo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MoveRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| HiloUsa | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| GuiWinJson | SALTADA | IDENTICO | IDENTICO | SALTADA |
| GuiShot | SALTADA | NO-PETA | NO-PETA | SALTADA |
| NatNew | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatV7 | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| FloatNat | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatCede | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatExpropia | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatBuiltin | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatAnida | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatDirecta | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatCoste | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatNombres | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatEstrecha | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| CatCadena | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| CatCrono | IDENTICO | IDENTICO | IDENTICO | IDENTICO |

SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  
NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  
SALTADA: GuiWinJson en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  

