# Tandas T1 — las placas conducidas

## Tanda 2026-11-06 11:55:14

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\wifi\tanda_h3.json` (15 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-05 16:34:26) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-06 11:21:29) - sabor `v2-gui1-lvgl0`
- placas conectadas: 2 (esp32p4@COM15, esp32s3@COM9)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 5c58295b | 1611 | ok | 0 | 33 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 53cd5336 | 295 | ok | 1 | 35 | 1 |
| ThreadTrasMain | ThreadTrasMain | 89cb057e | 532 | ok | 0 | 346 | 3 |
| MachineId | MachineId | 0d1bac8c | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | 722bfc87 | 2041 | ok | 0 | 83 | 17 |
| GuiShot | GuiShot | 84d42b22 | 1347 | ok | 0 | 375 | 8 |
| StrNulo | StrNulo | d28013f0 | 2784 | ok | 0 | 10 | 14 |
| SleepCede | SleepCede | 3a7985fa | 656 | ok | 0 | 13 | 2 |
| CedeCero | CedeCero | 3059648e | 1157 | ok | 0 | 16 | 1 |
| DatosGrandes | DatosGrandes | 3bb51971 | 34291 | ok | 0 | 16 | 11 |
| FileCursor | FileCursor | 1c717705 | 4208 | ok | 0 | 94 | 33 |
| StrCursor | StrCursor | 3e3e811c | 11397 | ok | 0 | 45 | 12 |
| StrDosCaminos | StrDosCaminos | 305a692b | 3760 | ok | 0 | 15 | 155 |
| ParseNumero | ParseNumero | a6fd93e1 | 3868 | ok | 0 | 0 | 52 |
| JsonErrores | JsonErrores | 6efa6966 | 3607 | ok | 0 | 22 | 14 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Nov  6 2026 11:14:02` imageCrc=`9a5fe4fa` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (06-11 11:14)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=1320007 fs=352256/16777216 B vmHeapBytes=25145344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 50 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 192 | 33 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 35 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 313 | 346 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 20 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 729 | 60 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 615 | 384 | EXITED OK exit=0 |
| StrNulo | **IDENTICO** | 111 | 10 | EXITED OK exit=0; host rc=0 |
| SleepCede | **DIFIERE** | 54 | 13 | texto distinto; estado no casa (host rc=0, EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: removeFile('sleepcede_test.bin'): ya está abierto) en SleepCede.bp:27) |
| CedeCero | **IDENTICO** | 303 | 16 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 66 | 16 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 295 | 94 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 4106 | 45 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1084 | 15 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 487 | 0 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 136 | 22 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=1321718 fsUsed=356352 vmHeapBytes=25145344 pila libre: run=12056 io=25664 tareaREPL=25664 heap del sistema, minimo libre=151276
- tras ThrowSinAtrapar: uptimeMs=1322664 fsUsed=356352 vmHeapBytes=25145344 pila libre: run=12056 io=25664 tareaREPL=25664 heap del sistema, minimo libre=151276
- tras ThreadTrasMain: uptimeMs=1324144 fsUsed=356352 vmHeapBytes=25145344 pila libre: run=12056 io=25664 tareaREPL=25664 heap del sistema, minimo libre=151276
- tras MachineId: uptimeMs=1324860 fsUsed=356352 vmHeapBytes=25145344 pila libre: run=12056 io=25664 tareaREPL=25664 heap del sistema, minimo libre=151276
- tras GuiWinJson: uptimeMs=1327663 fsUsed=360448 vmHeapBytes=25145344 pila libre: run=10620 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- tras GuiShot: uptimeMs=1330215 fsUsed=368640 vmHeapBytes=25145344 pila libre: run=10620 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- tras StrNulo: uptimeMs=1332417 fsUsed=368640 vmHeapBytes=25145344 pila libre: run=12056 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- tras SleepCede: uptimeMs=1333805 fsUsed=368640 vmHeapBytes=25145344 pila libre: run=11972 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
<details><summary>SleepCede: cola del stderr del host</summary>

```
 5 busquedas, 125 strcmp, 187 us
[bpvm] tabla de handles: 0 -> 964 slots (7 KB dentro del heap) OK — techo del heap 500192, libre 474 KB
[gc] pila t0: rango [507904..507940) = 9 palabras, 0 son refs al heap
[gc] vivo=236 liberado=12 heap=[14016..14252) freelist=vacia
[gc] reservas: de_lista=0 de_bump=8 astilla=0
[gc] libres: 0 huecos, 0 B en total, el mayor 0 | altas=0
[bpvm] pila: el main llegó a 200 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>SleepCede: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -1,2 +1 @@
-tras sleep(0): true
-fin
+tras sleep(0): File('sleepcede_test.bin', "w"): ya está abierto
```

oraculo (host):

```
tras sleep(0): true
fin
```

placa:

```
tras sleep(0): File('sleepcede_test.bin', "w"): ya está abierto
```
</details>

- tras CedeCero: uptimeMs=1335137 fsUsed=368640 vmHeapBytes=25145344 pila libre: run=11972 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- tras DatosGrandes: uptimeMs=1339545 fsUsed=401408 vmHeapBytes=25145344 pila libre: run=11972 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- tras FileCursor: uptimeMs=1341475 fsUsed=372736 vmHeapBytes=25145344 pila libre: run=10620 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- tras StrCursor: uptimeMs=1348220 fsUsed=376832 vmHeapBytes=25145344 pila libre: run=11972 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- tras StrDosCaminos: uptimeMs=1350535 fsUsed=368640 vmHeapBytes=25145344 pila libre: run=11972 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- tras ParseNumero: uptimeMs=1352081 fsUsed=368640 vmHeapBytes=25145344 pila libre: run=11972 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- tras JsonErrores: uptimeMs=1353419 fsUsed=368640 vmHeapBytes=25145344 pila libre: run=11972 io=25396 tareaREPL=25396 heap del sistema, minimo libre=151276
- ejecutadas (con EXITED) 15 de 15 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 5c58295b | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 53cd5336 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 89cb057e | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 0d1bac8c | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | 722bfc87 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 84d42b22 | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | d28013f0 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 3a7985fa | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 3059648e | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | 3bb51971 | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 1c717705 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | 3e3e811c | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 305a692b | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | a6fd93e1 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | 6efa6966 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Nov  6 2026 11:50:58` imageCrc=`ce33e434` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (06-11 11:51)
- imagen: arch=94 variant=None cpuFreqHz=160000000 resetReason=power-on uptimeMs=261068 fs=45056/9199616 B vmHeapBytes=7844864
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 3 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 191 | 33 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 11 | 35 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 319 | 346 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 18 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 83 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 375 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| StrNulo | **IDENTICO** | 107 | 10 | EXITED OK exit=0; host rc=0 |
| SleepCede | **DIFIERE** | 42 | 13 | texto distinto; estado no casa (host rc=0, EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: removeFile('sleepcede_test.bin'): ya está abierto) en SleepCede.bp:27) |
| CedeCero | **IDENTICO** | 649 | 16 | EXITED OK exit=0; host rc=0 |
| DatosGrandes | **IDENTICO** | 66 | 16 | EXITED OK exit=0; host rc=0 |
| FileCursor | **IDENTICO** | 286 | 94 | EXITED OK exit=0; host rc=0 |
| StrCursor | **IDENTICO** | 12795 | 45 | EXITED OK exit=0; host rc=0 |
| StrDosCaminos | **IDENTICO** | 1081 | 15 | EXITED OK exit=0; host rc=0 |
| ParseNumero | **IDENTICO** | 485 | 0 | EXITED OK exit=0; host rc=0 |
| JsonErrores | **IDENTICO** | 324 | 22 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=262102 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=12980 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56972
- tras ThrowSinAtrapar: uptimeMs=262785 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=12988 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56972
- tras ThreadTrasMain: uptimeMs=263637 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=12988 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56972
- tras MachineId: uptimeMs=264255 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=12988 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56972
- tras StrNulo: uptimeMs=265241 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=12364 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56972
- tras SleepCede: uptimeMs=266038 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=11732 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56972
<details><summary>SleepCede: cola del stderr del host</summary>

```
 5 busquedas, 125 strcmp, 187 us
[bpvm] tabla de handles: 0 -> 964 slots (7 KB dentro del heap) OK — techo del heap 500192, libre 474 KB
[gc] pila t0: rango [507904..507940) = 9 palabras, 0 son refs al heap
[gc] vivo=236 liberado=12 heap=[14016..14252) freelist=vacia
[gc] reservas: de_lista=0 de_bump=8 astilla=0
[gc] libres: 0 huecos, 0 B en total, el mayor 0 | altas=0
[bpvm] pila: el main llegó a 200 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>SleepCede: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -1,2 +1 @@
-tras sleep(0): true
-fin
+tras sleep(0): File('sleepcede_test.bin', "w"): ya está abierto
```

oraculo (host):

```
tras sleep(0): true
fin
```

placa:

```
tras sleep(0): File('sleepcede_test.bin', "w"): ya está abierto
```
</details>

- tras CedeCero: uptimeMs=267408 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=11732 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56972
- tras DatosGrandes: uptimeMs=271211 fsUsed=81920 vmHeapBytes=7844864 pila libre: run=11732 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56972
- tras FileCursor: uptimeMs=272831 fsUsed=53248 vmHeapBytes=7844864 pila libre: run=11732 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56732
- tras StrCursor: uptimeMs=287611 fsUsed=57344 vmHeapBytes=7844864 pila libre: run=12980 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56732
- tras StrDosCaminos: uptimeMs=289789 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=11732 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56732
- tras ParseNumero: uptimeMs=291180 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=11732 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56732
- tras JsonErrores: uptimeMs=292614 fsUsed=49152 vmHeapBytes=7844864 pila libre: run=11732 io=9248 tareaREPL=9248 heap del sistema, minimo libre=56732
- ejecutadas (con EXITED) 13 de 15 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 5c58295b | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 53cd5336 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 89cb057e | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 0d1bac8c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | d28013f0 | ok |
| /app/SleepCede.mod | bpgenvm-c\build\tanda\SleepCede\SleepCede.mod | 656 | 3a7985fa | ok |
| /app/CedeCero.mod | bpgenvm-c\build\tanda\CedeCero\CedeCero.mod | 1157 | 3059648e | ok |
| /app/DatosGrandes.mod | bpgenvm-c\build\tanda\DatosGrandes\DatosGrandes.mod | 34291 | 3bb51971 | ok (por trozos) |
| /app/FileCursor.mod | bpgenvm-c\build\tanda\FileCursor\FileCursor.mod | 4208 | 1c717705 | ok (por trozos) |
| /app/StrCursor.mod | bpgenvm-c\build\tanda\StrCursor\StrCursor.mod | 11397 | 3e3e811c | ok (por trozos) |
| /app/StrDosCaminos.mod | bpgenvm-c\build\tanda\StrDosCaminos\StrDosCaminos.mod | 3760 | 305a692b | ok |
| /app/ParseNumero.mod | bpgenvm-c\build\tanda\ParseNumero\ParseNumero.mod | 3868 | a6fd93e1 | ok |
| /app/JsonErrores.mod | bpgenvm-c\build\tanda\JsonErrores\JsonErrores.mod | 3607 | 6efa6966 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | esp32p4@COM15 | esp32s3@COM9 |
|---|---|---|
| MathRango | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA |
| GuiWinJson | IDENTICO | SALTADA |
| GuiShot | NO-PETA | SALTADA |
| StrNulo | IDENTICO | IDENTICO |
| SleepCede | DIFIERE | DIFIERE |
| CedeCero | IDENTICO | IDENTICO |
| DatosGrandes | IDENTICO | IDENTICO |
| FileCursor | IDENTICO | IDENTICO |
| StrCursor | IDENTICO | IDENTICO |
| StrDosCaminos | IDENTICO | IDENTICO |
| ParseNumero | IDENTICO | IDENTICO |
| JsonErrores | IDENTICO | IDENTICO |

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  
SALTADA: GuiWinJson en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  

