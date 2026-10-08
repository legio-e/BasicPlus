# Tandas T1 — las placas conducidas

## Tanda 2026-10-08 18:40:51

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\tanda_c6_650.json` (2 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-08 14:11:16) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-08 17:57:08) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| Importa5 | Importa5 | e75e09bb | 956 | ok | 0 | 32 | 2 |
| Importa5Copias | Importa5 | e75e09bb | 956 | ok | 0 | 24 | 2 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  8 2026 15:00:22` imageCrc=`7c18cbb7` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c6/build/bpvm_esp32c6.bin (08-10 18:22)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=211641 fs=32768/1261568 B vmHeapBytes=64512
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 0 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| Importa5 | **NO-PETA** | 36 | 32 | EXITED OK exit=0 |
| Importa5Copias | **NO-PETA** | 37 | 24 | EXITED OK exit=0 |

- tras Importa5: uptimeMs=213258 fsUsed=36864 vmHeapBytes=64512 pila libre: run=6160 io=5652 tareaREPL=5652 heap del sistema, minimo libre=34488
- tras Importa5Copias: uptimeMs=214700 fsUsed=86016 vmHeapBytes=64512 pila libre: run=6160 io=5652 tareaREPL=5652 heap del sistema, minimo libre=34488
- ejecutadas (con EXITED) 2 de 2 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/Importa5.mod | bpgenvm-c\build\tanda\Importa5\Importa5.mod | 956 | e75e09bb | ok |
| /app/Str.mod | bpstdlib\Str.mod | 4607 | 4320a024 | ok (por trozos) |
| /app/Json.mod | bpstdlib\Json.mod | 22749 | dc348e4c | ok (por trozos) |
| /app/Collections.mod | bpstdlib\Collections.mod | 12439 | 738b2934 | ok (por trozos) |
| /app/Importa5.mod | bpgenvm-c\build\tanda\Importa5Copias\Importa5.mod | 956 | e75e09bb | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| Importa5 | NO-PETA |
| Importa5Copias | NO-PETA |


