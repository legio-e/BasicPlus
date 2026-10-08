# Tandas T1 — las placas conducidas

## Tanda 2026-10-08 18:38:34

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\C--lenguajes-pm-miVM\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\tanda_c6_extra.json` (3 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-08 14:11:16) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-08 17:57:08) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| PinesSistema | PinesSistema | 3d458dc2 | 1056 | ok | 0 | 16 | 39 |
| GuiWinJson | GuiWinJson | 5f8ff7d5 | 2070 | ok | 0 | 54 | 17 |
| GuiArbolG2 | GuiArbolG2 | dee22fe8 | 5331 | ok | 0 | 57 | 39 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  8 2026 15:00:22` imageCrc=`7c18cbb7` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c6/build/bpvm_esp32c6.bin (08-10 18:22)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=75332 fs=32768/1261568 B vmHeapBytes=64512
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 0 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| PinesSistema | **NO-PETA** | 29 | 16 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 1036 | 50 | EXITED OK exit=0; host rc=0 |
| GuiArbolG2 | **IDENTICO** | 1046 | 56 | EXITED OK exit=0; host rc=0 |

- tras PinesSistema: uptimeMs=76216 fsUsed=36864 vmHeapBytes=64512 pila libre: run=6720 io=5864 tareaREPL=5864 heap del sistema, minimo libre=63772
- tras GuiWinJson: uptimeMs=79069 fsUsed=40960 vmHeapBytes=64512 pila libre: run=6160 io=5652 tareaREPL=5652 heap del sistema, minimo libre=35920
- tras GuiArbolG2: uptimeMs=82005 fsUsed=40960 vmHeapBytes=64512 pila libre: run=6160 io=5652 tareaREPL=5652 heap del sistema, minimo libre=34488
- ejecutadas (con EXITED) 3 de 3 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/PinesSistema.mod | bpgenvm-c\build\tanda\PinesSistema\PinesSistema.mod | 1056 | 3d458dc2 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | 5f8ff7d5 | ok |
| /app/GuiArbolG2.mod | bpgenvm-c\build\tanda\GuiArbolG2\GuiArbolG2.mod | 5331 | dee22fe8 | ok (por trozos) |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gpio: en la zona de packs (pack:Stdlib/Gpio.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Neopixel: en la zona de packs (pack:Stdlib/Neopixel.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| PinesSistema | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiArbolG2 | IDENTICO |


