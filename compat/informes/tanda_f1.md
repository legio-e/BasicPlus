# Tandas T1 — las placas conducidas

## Tanda 2026-09-12 17:38:04

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 322 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 11 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 33 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 340 | 8 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Sep 12 2026 12:05:08` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=45218 fs=245760/1048576 B vmHeapBytes=270408
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (supuesto por boardName 'rp2350a')
- inventario /lib+/app: 26 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 26 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 5 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 304 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 11 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 33 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a') |
| GuiShot | **SALTADA** | - | 340 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350a') |

- tras MathRango: uptimeMs=47065 fsUsed=245760 vmHeapBytes=270408
- tras ThrowSinAtrapar: uptimeMs=48251 fsUsed=245760 vmHeapBytes=270408
- tras ThreadTrasMain: uptimeMs=49400 fsUsed=245760 vmHeapBytes=270408
- tras MachineId: uptimeMs=50749 fsUsed=245760 vmHeapBytes=270408
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


## Tanda 2026-09-12 17:38:44

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (rp2350b@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 322 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 35 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 352 | 8 |

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Sep 12 2026 12:05:08` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "BOOTSEL"]
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=11206 fs=327680/10248192 B vmHeapBytes=271926
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (supuesto por boardName 'rp2350b')
- inventario /lib+/app: 30 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 27 | 9 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 4 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 303 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 10 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 35 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b') |
| GuiShot | **SALTADA** | - | 352 | necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b') |

- tras MathRango: uptimeMs=13038 fsUsed=327680 vmHeapBytes=271926
- tras ThrowSinAtrapar: uptimeMs=14231 fsUsed=327680 vmHeapBytes=271926
- tras ThreadTrasMain: uptimeMs=15375 fsUsed=327680 vmHeapBytes=271926
- tras MachineId: uptimeMs=16932 fsUsed=327680 vmHeapBytes=271926
- ejecutadas (con EXITED) 4 de 6 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |

notas de dependencias:

- dependencia Core: ya en /app/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)

### Matriz pruebas x placas

| prueba | rp2350b@COM4 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b')  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (supuesto por boardName 'rp2350b')  


## Tanda 2026-09-12 17:39:25

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u5@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 312 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 37 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 345 | 8 |

### Placa stm32u5 - 20353850553250040015002D

- sello: boardName=`stm32u5` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 12 2026 17:33:07` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=10222 fs=647168/2064384 B vmHeapBytes=1179648
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 49 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 226 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 80 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 340 | 312 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 93 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 1026 | 35 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 1225 | 338 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=12661 fsUsed=647168 vmHeapBytes=1179648
- tras ThrowSinAtrapar: uptimeMs=13857 fsUsed=647168 vmHeapBytes=1179648
- tras ThreadTrasMain: uptimeMs=15178 fsUsed=647168 vmHeapBytes=1179648
- tras MachineId: uptimeMs=16498 fsUsed=647168 vmHeapBytes=1179648
- tras GuiWinJson: uptimeMs=20047 fsUsed=647168 vmHeapBytes=1179648
- tras GuiShot: uptimeMs=24006 fsUsed=647168 vmHeapBytes=1179648
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


## Tanda 2026-09-12 17:42:53

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u5@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 324 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 29 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 334 | 8 |

### Placa stm32u5 - 203436414230500D00350041

- sello: boardName=`stm32u5` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Sep 12 2026 17:32:56` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=16276 fs=344064/614400 B vmHeapBytes=393216
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: si (supuesto por boardName 'stm32u5')
- inventario /lib+/app: 27 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 211 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 45 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 327 | 324 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 60 | 10 | EXITED OK exit=0 |
| GuiWinJson | **DIFIERE** | 450 | 29 | texto distinto; estado no casa (host rc=0, EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: builtin 135 no soportado en esta VM (subconjunto C))) |
| GuiShot | **PETA** | 499 | 334 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: builtin 135 no soportado en esta VM (subconjunto C)) |

- tras MathRango: uptimeMs=18535 fsUsed=352256 vmHeapBytes=393216
- tras ThrowSinAtrapar: uptimeMs=19917 fsUsed=360448 vmHeapBytes=393216
- tras ThreadTrasMain: uptimeMs=21062 fsUsed=368640 vmHeapBytes=393216
- tras MachineId: uptimeMs=22798 fsUsed=376832 vmHeapBytes=393216
- tras GuiWinJson: uptimeMs=43258 fsUsed=483328 vmHeapBytes=393216
<details><summary>GuiWinJson: cola del stderr del host</summary>

```
[bpvm-c] dep 'Gui' -> Gui.mod
[bpvm-c] dep 'Json' -> Json.mod
[bpvm-c] dep 'Core' -> Core.mod
[bpvm-c] dep 'Collections' -> Collections.mod
[bpvm-c] dep 'Str' -> Str.mod
[bpvm] tabla de simbolos: 1784 simbolos, 43703 B de nombres, 81920 B en total
[bpvm] tabla de handles: 0 -> 681 slots (5 KB dentro del heap) OK — techo del heap 387768, libre 335 KB
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 1 vivos)

```
</details>

<details><summary>GuiWinJson: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -1,17 +1 @@
-J1: {"type":"Panel","width":480,"height":320,"children":[{"type":"Label","text":"Demo de Forms — Camino A","align":"TOP_MID","y":12},{"type":"Button","text":"Saludar","align":"CENTER","clic":"onSaludar"},{"type":"Checkbox","text":"Activar","align":"BOTTOM_MID","y":-20,"change":"onActivar"}]}
--- arbol tras cargar main.win --
-screen [480x320 align=0 +0,0]
-  panel [480x320 align=0 +0,0]
-    panel [480x320 align=0 +0,0]
-      label "Demo de Forms — Camino A" [-1x-1 align=1 +0,12]
-      button [-1x-1 align=4 +0,0]
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

```
</details>

- tras GuiShot: uptimeMs=46194 fsUsed=491520 vmHeapBytes=393216
<details><summary>GuiShot: cola del stderr del host</summary>

```
[bpvm-c] dep 'Gui' -> Gui.mod
[bpvm-c] dep 'Json' -> Json.mod
[bpvm-c] dep 'Core' -> Core.mod
[bpvm-c] dep 'Collections' -> Collections.mod
[bpvm-c] dep 'Str' -> Str.mod
[bpvm] tabla de simbolos: 1734 simbolos, 42149 B de nombres, 81920 B en total
[bpvm] tabla de handles: 0 -> 682 slots (5 KB dentro del heap) OK — techo del heap 387760, libre 335 KB
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 1 vivos)

```
</details>

- artefacto GuiShot.shot de GuiShot: NO bajado: GET /app/GuiShot.shot: NOT_FOUND no existe; GET /GuiShot.shot: NOT_FOUND no existe; GET GuiShot.shot: NOT_FOUND no existe; antes del RUN: no existia
- ejecutadas (con EXITED) 6 de 6 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |
| /app/Gui.mod | bpstdlib\Gui.mod | 62016 | 1f305f94 | ok (por trozos) |
| /app/Json.mod | bpstdlib\Json.mod | 24059 | 8d090549 | ok (por trozos) |
| /app/Collections.mod | bpstdlib\Collections.mod | 8890 | 740b9a71 | ok (por trozos) |
| /app/Str.mod | bpstdlib\Str.mod | 5487 | c2cb43dd | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 3627 | c2e655fa | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 2545 | 61c07fa6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- GuiWinJson: la placa tiene pantalla pero INFO no dice su tamano; oraculo de 480x320
- dependencia Gui: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /app
- dependencia Json: en /app con crc 9efcca31 != local 8d090549 (rancio, se vuelve a subir)
- dependencia Collections: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /app
- dependencia Str: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /app
- GuiShot: la placa tiene pantalla pero INFO no dice su tamano; oraculo de 480x320

### Matriz pruebas x placas

| prueba | stm32u5@COM5 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | DIFIERE |
| GuiShot | PETA |



## Tanda 2026-09-12 17:44:09

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas, filtro --prueba GuiWinJson,GuiShot)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u5@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 35 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 347 | 8 |

### Placa stm32u5 - 203436414230500D00350041

- sello: boardName=`stm32u5` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Sep 12 2026 17:32:56` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=91512 fs=491520/614400 B vmHeapBytes=393216
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 37 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| GuiWinJson | **SALTADA** | - | 35 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 347 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |

- ejecutadas (con EXITED) 0 de 2 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

(nada)

### Matriz pruebas x placas

| prueba | stm32u5@COM5 |
|---|---|
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en stm32u5@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u5@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-12 17:44:52

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c3@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 321 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 11 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 33 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 334 | 8 |

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Sep 12 2026 17:33:47` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=unknown uptimeMs=3068 fs=126976/1523712 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 19 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 321 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 11 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 33 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 334 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |

- tras MathRango: uptimeMs=4954 fsUsed=131072 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=6003 fsUsed=135168 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=7223 fsUsed=139264 vmHeapBytes=65536
- tras MachineId: uptimeMs=8492 fsUsed=143360 vmHeapBytes=65536
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

### Matriz pruebas x placas

| prueba | esp32c3@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-12 17:49:11

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32s3@COM9)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 311 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 8 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 45 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 352 | 8 |

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Sep 12 2026 17:33:10` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=94 variant=None cpuFreqHz=240000000 resetReason=power-on uptimeMs=12391 fs=323584/10248192 B vmHeapBytes=7336960
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 32 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 311 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 8 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 45 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 352 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |

- tras MathRango: uptimeMs=14879 fsUsed=327680 vmHeapBytes=7336960
- tras ThrowSinAtrapar: uptimeMs=16295 fsUsed=331776 vmHeapBytes=7336960
- tras ThreadTrasMain: uptimeMs=17463 fsUsed=335872 vmHeapBytes=7336960
- tras MachineId: uptimeMs=19082 fsUsed=335872 vmHeapBytes=7336960
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

### Matriz pruebas x placas

| prueba | esp32s3@COM9 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |

SALTADA: GuiWinJson en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-09-12 17:51:19

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM14)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 328 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 11 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 32 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 343 | 8 |

### Placa esp32p4 - 6055F9FB058D

- sello: boardName=`esp32p4` uniqueId=`6055F9FB058D` serverName=`bpvm-esp32p4` serverBuild=`Sep 12 2026 12:08:51` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=7026 fs=376832/7168000 B vmHeapBytes=28831744
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM14
- pantalla: si (INFO screenW x screenH = 1024x600)
- inventario /lib+/app: 38 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 10 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 328 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 11 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 540 | 32 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 560 | 352 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=9063 fsUsed=376832 vmHeapBytes=28831744
- tras ThrowSinAtrapar: uptimeMs=10213 fsUsed=376832 vmHeapBytes=28831744
- tras ThreadTrasMain: uptimeMs=11299 fsUsed=376832 vmHeapBytes=28831744
- tras MachineId: uptimeMs=12602 fsUsed=376832 vmHeapBytes=28831744
- tras GuiWinJson: uptimeMs=20788 fsUsed=376832 vmHeapBytes=28831744
- tras GuiShot: uptimeMs=25298 fsUsed=376832 vmHeapBytes=28831744
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.shot` (9301 B, crc d8b7d24c) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- ejecutadas (con EXITED) 6 de 6 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |
| /app/Json.mod | bpstdlib\Json.mod | 24059 | 8d090549 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 3627 | c2e655fa | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 2545 | 61c07fa6 | ok |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Gui: ya en /lib/Gui.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Json: en /app con crc 73c9a02c != local 8d090549 (rancio, se vuelve a subir)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)

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


## Tanda 2026-09-12 18:34:19

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 0 (ninguna)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 13 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 315 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 8 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 32 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 347 | 8 |

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


## Tanda 2026-09-12 18:34:28

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM14)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 317 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 11 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 38 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 337 | 8 |

### Placa esp32p4 - 6055F9FB058D

- sello: boardName=`esp32p4` uniqueId=`6055F9FB058D` serverName=`bpvm-esp32p4` serverBuild=`Sep 12 2026 18:20:05` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=5746 fs=393216/7168000 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM14
- pantalla: si (INFO screenW x screenH = 1024x600)
- inventario /lib+/app: 39 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **ERROR** | - | 12 | dependencia Core sombreada por Core.mod |
| ThrowSinAtrapar | **ERROR** | - | 8 | dependencia Core sombreada por Core.mod |
| ThreadTrasMain | **ERROR** | - | 317 | dependencia Core sombreada por Core.mod |
| MachineId | **ERROR** | - | 11 | dependencia Core sombreada por Core.mod |
| GuiWinJson | **ERROR** | - | 38 | dependencia Core sombreada por Core.mod |
| GuiShot | **ERROR** | - | 344 | dependencia Core sombreada por Core.mod |

- ejecutadas (con EXITED) 0 de 6 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 6

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |

notas de dependencias:

- dependencia Core: el RUN la resuelve a Core.mod (fuera de /lib y /app), crc -1: se sube a /app
- dependencia Core: SOMBREADA por Core.mod (crc -1) aunque se acaba de subir /app/Core.mod con crc e6006d29

### Matriz pruebas x placas

| prueba | esp32p4@COM14 |
|---|---|
| MathRango | ERROR |
| ThrowSinAtrapar | ERROR |
| ThreadTrasMain | ERROR |
| MachineId | ERROR |
| GuiWinJson | ERROR |
| GuiShot | ERROR |



## Tanda 2026-09-12 18:37:38

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM14)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 311 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 11 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 35 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 346 | 8 |

### Placa esp32p4 - 6055F9FB058D

- sello: boardName=`esp32p4` uniqueId=`6055F9FB058D` serverName=`bpvm-esp32p4` serverBuild=`Sep 12 2026 18:20:05` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=7130 fs=409600/7168000 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM14
- pantalla: si (INFO screenW x screenH = 1024x600)
- inventario /lib+/app: 40 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 311 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 11 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 550 | 31 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 550 | 355 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=9146 fsUsed=409600 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=10486 fsUsed=409600 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=11769 fsUsed=409600 vmHeapBytes=24637440
- tras MachineId: uptimeMs=13267 fsUsed=409600 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=17588 fsUsed=409600 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=21968 fsUsed=409600 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.shot` (9301 B, crc d8b7d24c) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
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

- dependencia Core: ya en /app/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Gui: ya en /lib/Gui.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Json: ya en /app/Json.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Collections: ya en /lib/Collections.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Str: ya en /lib/Str.mod (crc igual; resuelto por nombre, como el RUN)

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


## Tanda 2026-09-12 18:45:07

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 9 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 324 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 30 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 357 | 8 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 12 2026 18:20:05` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=11543 fs=327680/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 36 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **ERROR** | - | 9 | dependencia Core sombreada por Core.mod |
| ThrowSinAtrapar | **ERROR** | - | 8 | dependencia Core sombreada por Core.mod |
| ThreadTrasMain | **ERROR** | - | 324 | dependencia Core sombreada por Core.mod |
| MachineId | **ERROR** | - | 10 | dependencia Core sombreada por Core.mod |
| GuiWinJson | **ERROR** | - | 31 | dependencia Core sombreada por Core.mod |
| GuiShot | **ERROR** | - | 346 | dependencia Core sombreada por Core.mod |

- ejecutadas (con EXITED) 0 de 6 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 6

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |
| /app/Core.mod | bpstdlib\Core.mod | 13340 | e6006d29 | ok (por trozos) |

notas de dependencias:

- dependencia Core: el RUN la resuelve a Core.mod (fuera de /lib y /app), crc -1: se sube a /app
- dependencia Core: SOMBREADA por Core.mod (crc -1) aunque se acaba de subir /app/Core.mod con crc e6006d29

### Matriz pruebas x placas

| prueba | esp32p4@COM15 |
|---|---|
| MathRango | ERROR |
| ThrowSinAtrapar | ERROR |
| ThreadTrasMain | ERROR |
| MachineId | ERROR |
| GuiWinJson | ERROR |
| GuiShot | ERROR |



## Tanda 2026-09-12 18:47:35

- manifiesto: `bpgenvm-c\tools\tanda_prueba.json` (6 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-12 12:04:08) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-12 12:04:44) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32p4@COM15)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 05e0dfd0 | 2227 | ok | 0 | 8 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 422260e8 | 1800 | ok | 1 | 8 | 1 |
| ThreadTrasMain | ThreadTrasMain | 5138ea38 | 1054 | ok | 0 | 313 | 3 |
| MachineId | MachineId | 22792db1 | 1089 | ok | 0 | 7 | 3 |
| GuiWinJson | GuiWinJson | c2e655fa | 3627 | ok | 0 | 30 | 17 |
| GuiShot | GuiShot | 61c07fa6 | 2545 | ok | 0 | 336 | 8 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Sep 12 2026 18:20:05` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=160357 fs=327680/7520256 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 36 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 8 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 8 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 320 | 313 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 10 | 7 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 530 | 47 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 490 | 341 | EXITED OK exit=0 |

- tras MathRango: uptimeMs=163576 fsUsed=331776 vmHeapBytes=24637440
- tras ThrowSinAtrapar: uptimeMs=165044 fsUsed=335872 vmHeapBytes=24637440
- tras ThreadTrasMain: uptimeMs=166310 fsUsed=339968 vmHeapBytes=24637440
- tras MachineId: uptimeMs=168028 fsUsed=344064 vmHeapBytes=24637440
- tras GuiWinJson: uptimeMs=191817 fsUsed=430080 vmHeapBytes=24637440
- tras GuiShot: uptimeMs=196371 fsUsed=442368 vmHeapBytes=24637440
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (6989 B, crc 8192c86e) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- ejecutadas (con EXITED) 6 de 6 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 2227 | 05e0dfd0 | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 1800 | 422260e8 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 1054 | 5138ea38 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 1089 | 22792db1 | ok |
| /app/Gui.mod | bpstdlib\Gui.mod | 62016 | 1f305f94 | ok (por trozos) |
| /app/Json.mod | bpstdlib\Json.mod | 24059 | 8d090549 | ok (por trozos) |
| /app/Collections.mod | bpstdlib\Collections.mod | 8890 | 740b9a71 | ok (por trozos) |
| /app/Str.mod | bpstdlib\Str.mod | 5487 | c2cb43dd | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 3627 | c2e655fa | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 2545 | 61c07fa6 | ok |

notas de dependencias:

- dependencia Core: ya en /app/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Gui: en /lib con crc 717d9632 != local 1f305f94 (DESFASE, se sube a /app por delante)
- dependencia Json: en /app con crc 9efcca31 != local 8d090549 (rancio, se vuelve a subir)
- dependencia Collections: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /app
- dependencia Str: en /app con crc f5809f1a != local c2cb43dd (rancio, se vuelve a subir)

### Matriz pruebas x placas

| prueba | esp32p4@COM15 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  

