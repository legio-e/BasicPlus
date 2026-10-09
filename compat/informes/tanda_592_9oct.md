# Tandas T1 — las placas conducidas

## Tanda 2026-10-09 09:09:43

- manifiesto: `bpgenvm-c\tools\tanda_592.json` (1 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-09 08:24:01) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-09 08:13:46) - sabor `v2-gui1-lvgl0`
- placas conectadas: 4 (rp2350a@COM22, esp32p4@COM15, stm32u5g9@COM12, esp32s3@COM9)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| NatPesadaPlaca | NatPesadaPlaca | b4115764 | 449 | ok | - | - | - |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  9 2026 08:19:47` imageCrc=`219d7796` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (09-10 08:19)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=162057 fs=118784/1048576 B vmHeapBytes=219104
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 13 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatPesadaPlaca | **PARADA** | 432 | - | KILL a los 500 ms; el EXITED, 10 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |

- tras NatPesadaPlaca: uptimeMs=163423 fsUsed=118784 vmHeapBytes=219104 pila libre: run=15344 io=9776 tareaREPL=9776 heap del sistema, minimo libre=6352
- ejecutadas (con EXITED) 1 de 1 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatPesadaPlaca.mod | bpgenvm-c\build\tanda\NatPesadaPlaca\aot-ARMV8M_MAIN_SOFTFP\NatPesadaPlaca.mod | 630 | 44d28f70 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  9 2026 08:21:58` imageCrc=`aaedab10` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (09-10 09:08)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=76549 fs=167936/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 15 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatPesadaPlaca | **PARADA** | 451 | - | KILL a los 500 ms; el EXITED, 30 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (RV32IMAFC_ILP32F) |

- tras NatPesadaPlaca: uptimeMs=78269 fsUsed=172032 vmHeapBytes=24637440 pila libre: run=15508 io=25692 tareaREPL=25692 heap del sistema, minimo libre=414799
- ejecutadas (con EXITED) 1 de 1 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatPesadaPlaca.mod | bpgenvm-c\build\tanda\NatPesadaPlaca\aot-RV32IMAFC_ILP32F\NatPesadaPlaca.mod | 658 | 7a447ac4 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  9 2026 08:20:23` imageCrc=`8a51ed36` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (09-10 08:20)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=25102 fs=90112/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 2 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatPesadaPlaca | **PARADA** | 657 | - | KILL a los 500 ms; el EXITED, 20 ms despues: EXITED KILLED exit=130 (terminado por KILL); nativo 1/1 thunks (ARMV8M_MAIN_SOFTFP) |

- tras NatPesadaPlaca: uptimeMs=26299 fsUsed=90112 vmHeapBytes=786432 pila libre: run=-1 io=3272 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 1 de 1 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatPesadaPlaca.mod | bpgenvm-c\build\tanda\NatPesadaPlaca\aot-ARMV8M_MAIN_SOFTFP\NatPesadaPlaca.mod | 630 | 44d28f70 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Oct  9 2026 08:21:22` imageCrc=`436cc8ec` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (09-10 09:09)
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=38761 fs=212992/10248192 B vmHeapBytes=7336960
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 29 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatPesadaPlaca | **SALTADA** | - | - | prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar |

- ejecutadas (con EXITED) 0 de 1 listadas; no terminaron 0; saltadas 1; no llegaron a arrancar 0

subido a /app:

(nada)

notas de dependencias:

- limpieza: 0 fichero(s) de /app borrados

### Matriz pruebas x placas

| prueba | rp2350a@COM22 | esp32p4@COM15 | stm32u5g9@COM12 | esp32s3@COM9 |
|---|---|---|---|---|
| NatPesadaPlaca | PARADA | PARADA | PARADA | SALTADA |

SALTADA: NatPesadaPlaca en esp32s3@COM9: prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): aqui el codigo nativo no se puede probar  

