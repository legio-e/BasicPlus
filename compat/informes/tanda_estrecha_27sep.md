# Tandas T1 — las placas conducidas

## Tanda 2026-09-27 10:08:34

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (22 pruebas listadas, filtro --prueba NatNombres,NatEstrecha)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-27 10:00:39) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-27 09:05:31) - sabor `gui1-lvgl0`
- placas conectadas: 4 (stm32u5g9@COM12, rp2350b@COM4, esp32p4@COM15, stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| NatNombres | NatNombres | 59da5179 | 1484 | ok | 0 | 8 | 6 |
| NatEstrecha | NatEstrecha | eea1b76e | 1849 | ok | 0 | 8 | 23 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 27 2026 09:09:32` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=2544530 fs=458752/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 44 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatNombres | **IDENTICO** | 205 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 343 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |

- tras NatNombres: uptimeMs=2546789 fsUsed=466944 vmHeapBytes=786432
- tras NatEstrecha: uptimeMs=2548474 fsUsed=475136 vmHeapBytes=786432
- ejecutadas (con EXITED) 2 de 2 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2502 | f4660c04 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2977 | 3968968e | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Gui/Core.mod, crc igual): se ejecuta XIP, no se sube

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Sep 27 2026 08:18:25` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=3363970 fs=266240/10248192 B vmHeapBytes=5767168
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 43 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatNombres | **IDENTICO** | 23 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 118 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |

- tras NatNombres: uptimeMs=3365844 fsUsed=270336 vmHeapBytes=5767168
- tras NatEstrecha: uptimeMs=3367323 fsUsed=274432 vmHeapBytes=5767168
- ejecutadas (con EXITED) 2 de 2 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2502 | f4660c04 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2977 | 3968968e | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:SQLite/Core.mod, crc igual): se ejecuta XIP, no se sube

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 27 2026 08:20:53` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=2927347 fs=536576/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 73 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatNombres | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMAFC_ILP32F) |
| NatEstrecha | **IDENTICO** | 30 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMAFC_ILP32F) |

- tras NatNombres: uptimeMs=2930592 fsUsed=540672 vmHeapBytes=24637440
- tras NatEstrecha: uptimeMs=2932895 fsUsed=544768 vmHeapBytes=24637440
- ejecutadas (con EXITED) 2 de 2 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-RV32IMAFC_ILP32F\NatNombres.mod | 2746 | 6fe4de7d | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-RV32IMAFC_ILP32F\NatEstrecha.mod | 3213 | 4731c315 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Gui/Core.mod, crc igual): se ejecuta XIP, no se sube

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Sep 27 2026 09:30:41` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=1834057 fs=450560/516096 B vmHeapBytes=196608
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 36 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatNombres | **IDENTICO** | 146 | 8 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| NatEstrecha | **IDENTICO** | 290 | 8 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |

- tras NatNombres: uptimeMs=1836400 fsUsed=458752 vmHeapBytes=196608
- tras NatEstrecha: uptimeMs=1837666 fsUsed=466944 vmHeapBytes=196608
- ejecutadas (con EXITED) 2 de 2 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatNombres.mod | bpgenvm-c\build\tanda\NatNombres\aot-ARMV8M_MAIN_SOFTFP\NatNombres.mod | 2502 | f4660c04 | ok |
| /app/NatEstrecha.mod | bpgenvm-c\build\tanda\NatEstrecha\aot-ARMV8M_MAIN_SOFTFP\NatEstrecha.mod | 2977 | 3968968e | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:SQLite/Core.mod, crc igual): se ejecuta XIP, no se sube

### Matriz pruebas x placas

| prueba | stm32u5g9@COM12 | rp2350b@COM4 | esp32p4@COM15 | stm32u575@COM5 |
|---|---|---|---|---|
| NatNombres | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| NatEstrecha | IDENTICO | IDENTICO | IDENTICO | IDENTICO |


