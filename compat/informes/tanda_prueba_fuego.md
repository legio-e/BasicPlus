# Tandas T1 — las placas conducidas

## Tanda 2026-09-11 18:23:39

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-11 16:46:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-11 17:40:30) - sabor `gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 10 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 330 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 39 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 347 | 8 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Sep 10 2026 22:51:19` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- transporte: serie COM22
- pantalla: no (supuesto por boardName 'rp2350a')
- inventario /lib+/app: 21 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 27 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 5 | 10 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 304 | 330 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 6 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 39 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a') |
| GuiShot | **SALTADA** | - | 347 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a') |

- ejecutadas 4 de 6 listadas (saltadas 2)

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual)
- dependencia Math: ya en /lib/Math.mod (crc igual)
- dependencia Machine: ya en /lib/Machine.mod (crc igual)

### Matriz pruebas x placas

| prueba | rp2350a@COM22 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a')  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a')  


## Tanda 2026-09-11 18:24:02

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-11 16:46:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-11 17:40:30) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 325 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 8 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 31 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 341 | 8 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Sep 10 2026 22:54:58` capacidades=["META", "FILES", "TERMINAL"]
- transporte: serie COM3
- pantalla: si (supuesto por boardName 'esp32c6')
- inventario /lib+/app: 28 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 325 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 8 | EXITED OK exit=0 |
| GuiWinJson | **DIFIERE** | 560 | 31 | texto distinto |
| GuiShot | **NO-PETA** | 420 | 341 | EXITED OK exit=0 |

<details><summary>GuiWinJson: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -1,14 +1,14 @@
-J1: {"type":"Panel","width":480,"height":320,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}
+J1: {"type":"Panel","width":240,"height":240,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}
 -- arbol tras cargar main.win --
-screen [480x320 align=0 +0,0]
-  panel [480x320 align=0 +0,0]
-    panel [480x320 align=0 +0,0]
+screen [240x240 align=0 +0,0]
...
```

oraculo (host):

```
J1: {"type":"Panel","width":480,"height":320,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}
-- arbol tras cargar main.win --
screen [480x320 align=0 +0,0]
  panel [480x320 align=0 +0,0]
    panel [480x320 align=0 +0,0]
      label "Demo de Forms — Camino A" [-1x-1 align=1 +0,12]
      button [-1x-1 align=4 +0,0]
        label "Saludar" [-1x-1 align=0 +0,0]
      checkbox "Activar" [-1x-1 align=7 +0,-20 val=0]

WIN: {"type":"Window","width":480,"height":320,"children":[{"type":"Panel","width":480,"height":320,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}]}
J2 == J1: true
arbol igual: true
hijos del panel: 3
hijos del boton (la etiqueta): 1
boton en JSON: {"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"}
FIN
```

placa:

```
J1: {"type":"Panel","width":240,"height":240,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}
-- arbol tras cargar main.win --
screen [240x240 align=0 +0,0]
  panel [240x240 align=0 +0,0]
    panel [240x240 align=0 +0,0]
      label "Demo de Forms — Camino A" [-1x-1 align=1 +0,12]
      button [-1x-1 align=4 +0,0]
        label "Saludar" [-1x-1 align=0 +0,0]
      checkbox "Activar" [-1x-1 align=7 +0,-20 val=0]

WIN: {"type":"Window","width":240,"height":240,"children":[{"type":"Panel","width":240,"height":240,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}]}
J2 == J1: true
arbol igual: true
hijos del panel: 3
hijos del boton (la etiqueta): 1
boton en JSON: {"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"}
FIN
```
</details>

- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`
- ejecutadas 6 de 6 listadas (saltadas 0)

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 3627 | c2e655fa | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 2545 | 61c07fa6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual)
- dependencia Math: ya en /lib/Math.mod (crc igual)
- dependencia Machine: ya en /lib/Machine.mod (crc igual)
- GuiWinJson: la placa tiene pantalla pero INFO no dice su tamano; oraculo de 480x320
- dependencia Gui: ya en /lib/Gui.mod (crc igual)
- dependencia Json: ya en /lib/Json.mod (crc igual)
- dependencia Collections: ya en /lib/Collections.mod (crc igual)
- dependencia Str: ya en /lib/Str.mod (crc igual)
- GuiShot: la placa tiene pantalla pero INFO no dice su tamano; oraculo de 480x320

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | DIFIERE |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  


## Tanda 2026-09-11 18:26:22

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-11 16:46:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-11 17:40:30) - sabor `gui1-lvgl0`
- placas conectadas: 0 (ninguna)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 324 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 31 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 349 | 8 |

### Matriz pruebas x placas

| prueba |
|---|
| MathRango |
| ThrowSinAtrapar |
| ThreadTrasMain |
| MachineId |
| GuiWinJson |
| GuiShot |

(sin placas: no se ejecuto nada en dispositivo)  


## Tanda 2026-09-11 18:26:39

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-11 16:46:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-11 17:40:30) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 7 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 320 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 7 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 31 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 338 | 8 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Sep 10 2026 22:54:58` capacidades=["META", "FILES", "TERMINAL"]
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 34 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 7 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 320 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 7 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 560 | 32 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 440 | 336 | EXITED OK exit=0 |

- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`
- ejecutadas 6 de 6 listadas (saltadas 0)

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 3627 | c2e655fa | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 2545 | 61c07fa6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual)
- dependencia Math: ya en /lib/Math.mod (crc igual)
- dependencia Machine: ya en /lib/Machine.mod (crc igual)
- dependencia Gui: ya en /lib/Gui.mod (crc igual)
- dependencia Json: ya en /lib/Json.mod (crc igual)
- dependencia Collections: ya en /lib/Collections.mod (crc igual)
- dependencia Str: ya en /lib/Str.mod (crc igual)

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  


## Tanda 2026-09-11 18:28:22

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-11 16:46:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-11 17:40:30) - sabor `gui1-lvgl0`
- placas conectadas: 0 (ninguna)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 331 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 33 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 348 | 8 |

### Matriz pruebas x placas

| prueba |
|---|
| MathRango |
| ThrowSinAtrapar |
| ThreadTrasMain |
| MachineId |
| GuiWinJson |
| GuiShot |

(sin placas: no se ejecuto nada en dispositivo)  


## Tanda 2026-09-11 18:28:41

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-11 16:46:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-11 17:40:30) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM14)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 13 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 316 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 31 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 348 | 8 |

### Placa esp32p4 - 6055F9FB058D

- sello: boardName=`esp32p4` uniqueId=`6055F9FB058D` serverName=`bpvm-esp32p4` serverBuild=`Sep 10 2026 22:58:18` capacidades=["META", "FILES", "TERMINAL"]
- transporte: serie COM14
- pantalla: si (INFO screenW x screenH = 1024x600)
- inventario /lib+/app: 32 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 316 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 540 | 41 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 560 | 348 | EXITED OK exit=0 |

- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.shot` (9301 B, crc d8b7d24c) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.png`
- ejecutadas 6 de 6 listadas (saltadas 0)

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 3627 | c2e655fa | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 2545 | 61c07fa6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual)
- dependencia Math: ya en /lib/Math.mod (crc igual)
- dependencia Machine: ya en /lib/Machine.mod (crc igual)
- dependencia Gui: ya en /lib/Gui.mod (crc igual)
- dependencia Json: ya en /lib/Json.mod (crc igual)
- dependencia Collections: ya en /lib/Collections.mod (crc igual)
- dependencia Str: ya en /lib/Str.mod (crc igual)

### Matriz pruebas x placas

| prueba | esp32p4@COM14 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en esp32p4@COM14: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.png  


## Tanda 2026-09-11 18:30:11

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-11 16:46:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-11 17:40:30) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u5@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 316 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 7 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 30 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 353 | 8 |

### Placa stm32u5 - 20353850553250040015002D

- sello: boardName=`stm32u5` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 11 2026 18:29:29` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 44 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 226 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 82 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 341 | 316 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 94 | 7 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 1075 | 37 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 1271 | 337 | EXITED OK exit=0 |

- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5_203538505532\GuiShot.shot` (7292 B, crc bd19c7be) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5_203538505532\GuiShot.png`
- ejecutadas 6 de 6 listadas (saltadas 0)

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 3627 | c2e655fa | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 2545 | 61c07fa6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual)
- dependencia Math: ya en /lib/Math.mod (crc igual)
- dependencia Machine: ya en /lib/Machine.mod (crc igual)
- dependencia Gui: ya en /app/Gui.mod (crc igual)
- dependencia Json: ya en /app/Json.mod (crc igual)
- dependencia Collections: ya en /app/Collections.mod (crc igual)
- dependencia Str: ya en /app/Str.mod (crc igual)

### Matriz pruebas x placas

| prueba | stm32u5@COM12 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en stm32u5@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5_203538505532\GuiShot.png  


## Tanda 2026-09-11 19:18:43

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-11 16:46:21) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-11 17:40:30) - sabor `gui1-lvgl0`
- placas conectadas: 3 (stm32u5@COM12, rp2350a@COM22, rp2350b@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 325 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 7 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 33 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 344 | 8 |

### Placa stm32u5 - 20353850553250040015002D

- sello: boardName=`stm32u5` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 11 2026 18:29:29` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=2914897 fs=638976/2064384 B vmHeapBytes=1179648
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 48 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 234 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 96 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 348 | 325 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 104 | 7 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 944 | 28 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 1111 | 345 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=2917161 fsUsed=638976 vmHeapBytes=1179648
- tras ThrowSinAtrapar: uptimeMs=2918364 fsUsed=638976 vmHeapBytes=1179648
- tras ThreadTrasMain: uptimeMs=2919497 fsUsed=638976 vmHeapBytes=1179648
- tras MachineId: uptimeMs=2920848 fsUsed=638976 vmHeapBytes=1179648
- tras GuiWinJson: uptimeMs=2924152 fsUsed=638976 vmHeapBytes=1179648
- tras GuiShot: uptimeMs=2928320 fsUsed=638976 vmHeapBytes=1179648
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5_203538505532\GuiShot.shot` (7292 B, crc bd19c7be) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- ejecutadas (con EXITED) 6 de 6 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 3627 | c2e655fa | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 2545 | 61c07fa6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Gui: ya en /app/Gui.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Json: ya en /app/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /app/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /app/Str.mod (crc igual; resuelto por nombre, como el RUN)

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Sep 10 2026 22:51:19` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=1880938 fs=245760/1048576 B vmHeapBytes=270408
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (supuesto por boardName 'rp2350a')
- inventario /lib+/app: 26 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 28 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 5 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 303 | 325 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 6 | 7 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 33 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a') |
| GuiShot | **SALTADA** | - | 344 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a') |

- tras MathRango: uptimeMs=1882782 fsUsed=245760 vmHeapBytes=270408
- tras ThrowSinAtrapar: uptimeMs=1883966 fsUsed=245760 vmHeapBytes=270408
- tras ThreadTrasMain: uptimeMs=1885101 fsUsed=245760 vmHeapBytes=270408
- tras MachineId: uptimeMs=1886628 fsUsed=245760 vmHeapBytes=270408
- ejecutadas (con EXITED) 4 de 6 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Sep  9 2026 10:04:55` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=2946095 fs=299008/10248192 B vmHeapBytes=272724
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (supuesto por boardName 'rp2350b')
- inventario /lib+/app: 26 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 25 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **DIFIERE** | 5 | 8 | estado no casa (host rc=1, EXITED RUNTIME_ERROR exit=11 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3))) |
| ThreadTrasMain | **IDENTICO** | 303 | 325 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 7 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 33 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b') |
| GuiShot | **SALTADA** | - | 344 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b') |

- tras MathRango: uptimeMs=2950687 fsUsed=319488 vmHeapBytes=272724
- tras ThrowSinAtrapar: uptimeMs=2951864 fsUsed=323584 vmHeapBytes=272724
<details><summary>ThrowSinAtrapar: salida recibida de la placa (con EXITED)</summary>

```
antes
```
</details>

<details><summary>ThrowSinAtrapar: cola del stderr del host</summary>

```
 KB
[bpvm] throw (interp): "ALOAD: índice fuera de rango 99 (length=3)"
[bpvm] EXCEPCION NO ATRAPADA en el thread 0: el thread TERMINA aqui. Si esto sale dentro de un catch, el manejador se ha quedado sin memoria al construir su propio mensaje.
[bpvm]   el mensaje que traia: "ALOAD: índice fuera de rango 99 (length=3)"
[bpvm] throw (interp): construida pero SIN handler -> el RUN termina en error
=== RuntimeError: excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3) ===
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 1 vivos)

```
</details>

- tras ThreadTrasMain: uptimeMs=2952974 fsUsed=327680 vmHeapBytes=272724
- tras MachineId: uptimeMs=2954542 fsUsed=327680 vmHeapBytes=272724
- ejecutadas (con EXITED) 4 de 6 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |

notas de dependencias:

- dependencia Core: en /lib con crc 553263eb != local e6006d29 (DESFASE, se sube a /app por delante)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | stm32u5@COM12 | rp2350a@COM22 | rp2350b@COM4 |
|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | DIFIERE |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA |
| GuiWinJson | IDENTICO | SALTADA | SALTADA |
| GuiShot | NO-PETA | SALTADA | SALTADA |

NECESITA OJOS: GuiShot en stm32u5@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5_203538505532\GuiShot.png  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a')  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a')  
SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b')  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b')  

