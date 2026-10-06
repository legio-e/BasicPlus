# Tandas T1 — las placas conducidas

## Tanda 2026-10-06 16:41:09

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (44 pruebas listadas, filtro --prueba GuiWinJson,GuiShot,GuiArbolG2,GuiAnidadaGc)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-03 18:35:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-06 08:06:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| GuiWinJson | GuiWinJson | a79a2400 | 2070 | ok | 0 | 51 | 17 |
| GuiShot | GuiShot | e9faef8a | 1344 | ok | 0 | 373 | 8 |
| GuiArbolG2 | GuiArbolG2 | b33c4dbf | 5379 | ok | 0 | 56 | 39 |
| GuiAnidadaGc | GuiAnidadaGc | 82388d9f | 1121 | ok | 0 | 131 | 1 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Oct  6 2026 07:41:23` imageCrc=`c2d3bb43` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c6/build/bpvm_esp32c6.bin (06-10 16:37)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=15448 fs=32768/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 0 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| GuiWinJson | **IDENTICO** | 922 | 50 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 496 | 368 | EXITED OK exit=0 |
| GuiArbolG2 | **IDENTICO** | 887 | 51 | EXITED OK exit=0; host rc=0 |
| GuiAnidadaGc | **IDENTICO** | 33572 | 139 | EXITED OK exit=0; host rc=0 |

- tras GuiWinJson: uptimeMs=17469 fsUsed=40960 vmHeapBytes=65536 pila libre: run=6160 io=5588 tareaREPL=5588 heap del sistema, minimo libre=6696
- tras GuiShot: uptimeMs=19070 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=5588 tareaREPL=5588 heap del sistema, minimo libre=2368
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: no existia
- tras GuiArbolG2: uptimeMs=21310 fsUsed=45056 vmHeapBytes=65536 pila libre: run=5284 io=5588 tareaREPL=5588 heap del sistema, minimo libre=2196
- tras GuiAnidadaGc: uptimeMs=55753 fsUsed=40960 vmHeapBytes=65536 pila libre: run=5284 io=5588 tareaREPL=5588 heap del sistema, minimo libre=2052
- ejecutadas (con EXITED) 4 de 4 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2070 | a79a2400 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1344 | e9faef8a | ok |
| /app/GuiArbolG2.mod | bpgenvm-c\build\tanda\GuiArbolG2\GuiArbolG2.mod | 5379 | b33c4dbf | ok (por trozos) |
| /app/GuiAnidadaGc.mod | bpgenvm-c\build\tanda\GuiAnidadaGc\GuiAnidadaGc.mod | 1121 | 82388d9f | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:GuiMini/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:GuiMini/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:GuiMini/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:GuiMini/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:GuiMini/Str.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |
| GuiArbolG2 | IDENTICO |
| GuiAnidadaGc | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  

