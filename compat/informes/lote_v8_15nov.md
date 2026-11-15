# Tandas T1 — las placas conducidas

## Tanda 2026-11-15 10:26:37

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\pf\tanda_lote15nov.json` (24 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-14 13:33:40) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-14 13:33:59) - sabor `v2-gui1-lvgl0`
- placas conectadas: 3 (esp32p4@COM15, esp32c5@COM6, esp32s3@COM9)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 2270c82e | 1611 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 9b053410 | 295 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | 97dd1bb5 | 532 | ok | 0 | 336 | 3 |
| MachineId | MachineId | 42a7e155 | 452 | ok | 0 | 11 | 3 |
| GuiWinJson | GuiWinJson | 59428ce2 | 2041 | ok | 0 | 66 | 17 |
| GuiShot | GuiShot | 9c8d42ed | 1347 | ok | 0 | 388 | 8 |
| BorraCarpetaFichero | BorraCarpetaFichero | be28edf2 | 2271 | ok | 0 | 16 | 9 |
| RutaFueraFlash | RutaFueraFlash | 79cbf8d4 | 3213 | ok | 0 | 15 | 39 |
| FlashCarpetas | FlashCarpetas | c21b2696 | 733 | ok | 0 | 29 | 7 |
| GcHuecoPila | GcHuecoPila | 3304c6f0 | 1190 | ok | 0 | 17 | 3 |
| RefCaducadaBuiltin | RefCaducadaBuiltin | 845498b8 | 1688 | ok | 0 | 5 | 6 |
| DivBordes | DivBordes | 70f63a2f | 798 | ok | 0 | 6 | 10 |
| ListaBorde | ListaBorde | e6ddaaf1 | 2918 | ok | 0 | 33 | 26 |
| IndiceObjeto | IndiceObjeto | 80c280cb | 12474 | ok | 0 | 18 | 36 |
| GenericosOrden | GenericosOrden | 52134d69 | 8456 | ok | 0 | 26 | 30 |
| SobrecargaEspecifica | SobrecargaEspecifica | 555c6ff1 | 1358 | ok | 0 | 15 | 9 |
| SobrecargaVisible | SobrecargaVisible | d3797df1 | 1999 | ok | 0 | 18 | 4 |
| BoolBits | BoolBits | 7b87929a | 876 | ok | 0 | 22 | 15 |
| ArrayNuevo | ArrayNuevo | 2feee85e | 2393 | ok | 0 | 27 | 17 |
| BucleConNombre | BucleConNombre | 6c35f20c | 1734 | ok | 0 | 14 | 12 |
| ErroresConTipo | ErroresConTipo | 7d9c7755 | 11493 | ok | 0 | 34 | 48 |
| IgualdadRefs | IgualdadRefs | 4a34e551 | 883 | ok | 0 | 20 | 8 |
| VarParam | VarParam | 1024398c | 3481 | ok | 0 | 23 | 12 |
| StrNulo | StrNulo | 4572ae05 | 2784 | ok | 0 | 0 | 14 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Nov 13 2026 12:42:05` imageCrc=`579b84c9` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (15-11 10:14)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=198007 fs=450560/16777216 B vmHeapBytes=25145344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 71 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 193 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 336 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 21 | 11 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 742 | 59 | EXITED OK exit=0; host rc=0 |
| GuiShot | **COLGADA** | - | 387 | sin EXITED en 60 s; tras KILL: tampoco EXITED en 5 s; recuperacion: reinicio por el cable (RTS; el wire no contesto al RESET): vuelve (resetReason=power-on, uptimeMs=10461) |
| BorraCarpetaFichero | **IDENTICO** | 242 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 431 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 172 | 29 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 217 | 17 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 54 | 5 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 59 | 6 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 220 | 33 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 249 | 18 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 212 | 26 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 55 | 15 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 27 | 18 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 85 | 22 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 97 | 27 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 66 | 14 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 632 | 34 | texto distinto |
| IgualdadRefs | **IDENTICO** | 61 | 20 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 95 | 23 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 111 | 0 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=200542 fsUsed=454656 vmHeapBytes=25145344 pila libre: run=13552 io=25796 tareaREPL=25796 heap del sistema, minimo libre=156552
- tras ThrowSinAtrapar: uptimeMs=201360 fsUsed=454656 vmHeapBytes=25145344 pila libre: run=13552 io=25796 tareaREPL=25796 heap del sistema, minimo libre=156552
- tras ThreadTrasMain: uptimeMs=202611 fsUsed=454656 vmHeapBytes=25145344 pila libre: run=13480 io=25796 tareaREPL=25796 heap del sistema, minimo libre=156412
- tras MachineId: uptimeMs=203577 fsUsed=454656 vmHeapBytes=25145344 pila libre: run=13552 io=25796 tareaREPL=25796 heap del sistema, minimo libre=156236
- tras GuiWinJson: uptimeMs=207598 fsUsed=458752 vmHeapBytes=25145344 pila libre: run=12440 io=25528 tareaREPL=25528 heap del sistema, minimo libre=146868
- tras GuiShot: uptimeMs=11449 fsUsed=454656 vmHeapBytes=25145344 pila libre: run=-1 io=-1 tareaREPL=28164 heap del sistema, minimo libre=158492
<details><summary>GuiShot: log post-mortem de la placa, tras recuperarla</summary>

```
[  532] packs: zona lista, 9 pack(s) (3 vivo(s)), 5544 KB libres de 10144
[  645] pantalla: el P4 publica 'PANT' desde la imagen (MIPI-DSI, el panel del ENV, tactil GT911)
[  645] pantalla: 2 pines suyos (la luz y el reset): fuera de Machine.pins() (#669)
[  646] pantalla: el I2C 0 del tactil (sda 7, scl 8, 100 kHz) es de la placa y se comparte (#669)
[  648] sonido: ES8311 en el I2C 0 (0x18); I2S mclk 13, bclk 12, ws 10, dout 9; amplificador en el GPIO 53
[  648] radio: la C6 por SDIO (ranura 1), en reset hasta que haga falta
[  648] sd: SDIO slot 0, 4 bit(s), clk 43 cmd 44 d0 39 | pwr 45 (activo bajo) | ldo 4 | 20000 kHz (por defecto)
[  690] sd: montada en /sd por SDMMC (particion en el bloque 2048)
[  690] boot: imagen de DOS nucleos
[  691] run: la tarea del programa, creada (16 KB de pila, prioridad 5; espera en la cola)
[  691] run: el 2o run, creado (vm.runs=2; 16 KB de pila, RAM interna): dos runs en la cola
[  704] wifi: se enciende (autoStart), para conectarse a '(tapado)'
[ 2561] radio: el enlace con la C6, abierto en 1851 ms
[ 2564] radio: la C6 lleva esp_hosted 3.0.9
[ 2820] radio: el Bluetooth de la C6, encendido
[ 2832] bluetooth: se enciende (bt=1)
[ 2882] wifi: encendida en 2137 ms (de la pila de su tarea quedan 6436 B)
[ 2888] red: IP por DHCP, con el nombre basicplus-esp32p4
[ 2966] REPL entry (wire v1) — buffer de bulk 8192 B
[ 2966] bt: NimBLE en marcha (la propia, 58:E6:C5:F5:CC:0E, pública; 0 alias; claves: 280 B)
[ 3021] bt: esperando a 1 recordado(s) por su dirección
[ 3070] bt: esperando a 2 recordado(s) por su dirección
[ 7877] wifi: conectada a '(tapado)'
[ 8911] red: IP 192.168.1.53
[ 8982] wire: la red escucha en el puerto 3333
[10216] pack: 2 motor(es) en la zona, 2 cargable(s) | bloque 4096 KB @0x48001000: estaticos 793184 B, arena 3321 KB
[10217] pack: [0] 'SQLite' (motor 'sqlite') valido @0x403E80B0 | .data 6916 B a 0x48060B00, .bss 1368 B a cero | saltando a 0x403E8218
[10329] pack: sqlite: pack vivo
[10329] pack: 3.53.4
[10330] pack: sqlite: arena 3321 KB en 480C2A60
[10330] pack: sqlite: initialize OK — motor arrancado y vfs 'bp' registrado
[10331] pack: sqlite: API publicada como 'SQLI' — 17 simbolos, v1
[10331] pack: [0] 'SQLite' volvio, rc=0
[10334] pack: [1] 'Gui' (motor 'lvgl') valido @0x405240B0 | .data 208 B a 0x48062B60, .bss 392740 B a cero | saltando a 0x40524102
[10460] pack: gui: pack vivo (LVGL 9.2.2, pool 256 KB)
[10461] pack: gui: lv_init hecho (TLSF sobre el pool estatico del pack)
[10461] pack: gui: API publicada como 'LVGL'
[10461] pack: [1] 'Gui' volvio, rc=0
[10461] packs: 2 motor(es) cargado(s)
[10619] GUI: pack 'LVGL' resuelto (88 funciones, API v1)
```
</details>

- tras BorraCarpetaFichero: uptimeMs=13492 fsUsed=454656 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=155292
- tras RutaFueraFlash: uptimeMs=15443 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11744 io=25796 tareaREPL=25796 heap del sistema, minimo libre=154376
<details><summary>RutaFueraFlash: cola del stderr del host</summary>

```
throw: ATRAPADA por un catch del programa
[bpvm] throw: "appendFile('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "readFileBytes('/../x810'): no se pudo abrir"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "writeFileBytes('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] pila: el main llegó a 244 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>RutaFueraFlash: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -13,7 +13,7 @@
 rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
 copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
 fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
-lastModified -> IOError: lastModified('/../x810'): no se pudo leer
+lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
 File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
 File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
...
```

oraculo (host):

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no se pudo leer
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no se pudo leer
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```

placa:

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```
</details>

- tras FlashCarpetas: uptimeMs=16494 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=154376
- tras GcHuecoPila: uptimeMs=17778 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11744 io=25796 tareaREPL=25796 heap del sistema, minimo libre=152168
- tras RefCaducadaBuiltin: uptimeMs=19198 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=152168
- tras DivBordes: uptimeMs=20147 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11744 io=25796 tareaREPL=25796 heap del sistema, minimo libre=152168
- tras ListaBorde: uptimeMs=22332 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras IndiceObjeto: uptimeMs=25952 fsUsed=475136 vmHeapBytes=25145344 pila libre: run=11744 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras GenericosOrden: uptimeMs=28922 fsUsed=471040 vmHeapBytes=25145344 pila libre: run=11744 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras SobrecargaEspecifica: uptimeMs=30340 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras SobrecargaVisible: uptimeMs=32076 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras BoolBits: uptimeMs=33042 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras ArrayNuevo: uptimeMs=34459 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras BucleConNombre: uptimeMs=35328 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11744 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras ErroresConTipo: uptimeMs=38783 fsUsed=471040 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
 o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>ErroresConTipo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -19,7 +19,7 @@
 ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
 ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
 ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
-ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
+ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
 ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
 ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
...
```

oraculo (host):

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```

placa:

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```
</details>

- tras IgualdadRefs: uptimeMs=39704 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11744 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras VarParam: uptimeMs=41707 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11736 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- tras StrNulo: uptimeMs=43282 fsUsed=462848 vmHeapBytes=25145344 pila libre: run=11744 io=25796 tareaREPL=25796 heap del sistema, minimo libre=148216
- ejecutadas (con EXITED) 23 de 24 listadas; no terminaron 1; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 2270c82e | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 9b053410 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 97dd1bb5 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 42a7e155 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | 59428ce2 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 9c8d42ed | ok |
| /app/BorraCarpetaFichero.mod | bpgenvm-c\build\tanda\BorraCarpetaFichero\BorraCarpetaFichero.mod | 2271 | be28edf2 | ok |
| /app/RutaFueraFlash.mod | bpgenvm-c\build\tanda\RutaFueraFlash\RutaFueraFlash.mod | 3213 | 79cbf8d4 | ok |
| /app/FlashCarpetas.mod | bpgenvm-c\build\tanda\FlashCarpetas\FlashCarpetas.mod | 733 | c21b2696 | ok |
| /app/GcHuecoPila.mod | bpgenvm-c\build\tanda\GcHuecoPila\GcHuecoPila.mod | 1190 | 3304c6f0 | ok |
| /app/RefCaducadaBuiltin.mod | bpgenvm-c\build\tanda\RefCaducadaBuiltin\RefCaducadaBuiltin.mod | 1688 | 845498b8 | ok |
| /app/DivBordes.mod | bpgenvm-c\build\tanda\DivBordes\DivBordes.mod | 798 | 70f63a2f | ok |
| /app/ListaBorde.mod | bpgenvm-c\build\tanda\ListaBorde\ListaBorde.mod | 2918 | e6ddaaf1 | ok |
| /app/IndiceObjeto.mod | bpgenvm-c\build\tanda\IndiceObjeto\IndiceObjeto.mod | 12474 | 80c280cb | ok (por trozos) |
| /app/GenericosOrden.mod | bpgenvm-c\build\tanda\GenericosOrden\GenericosOrden.mod | 8456 | 52134d69 | ok (por trozos) |
| /app/SobrecargaEspecifica.mod | bpgenvm-c\build\tanda\SobrecargaEspecifica\SobrecargaEspecifica.mod | 1358 | 555c6ff1 | ok |
| /app/SobrecargaVisible.mod | bpgenvm-c\build\tanda\SobrecargaVisible\SobrecargaVisible.mod | 1999 | d3797df1 | ok |
| /app/BoolBits.mod | bpgenvm-c\build\tanda\BoolBits\BoolBits.mod | 876 | 7b87929a | ok |
| /app/ArrayNuevo.mod | bpgenvm-c\build\tanda\ArrayNuevo\ArrayNuevo.mod | 2393 | 2feee85e | ok |
| /app/BucleConNombre.mod | bpgenvm-c\build\tanda\BucleConNombre\BucleConNombre.mod | 1734 | 6c35f20c | ok |
| /app/ErroresConTipo.mod | bpgenvm-c\build\tanda\ErroresConTipo\ErroresConTipo.mod | 11493 | 7d9c7755 | ok (por trozos) |
| /app/IgualdadRefs.mod | bpgenvm-c\build\tanda\IgualdadRefs\IgualdadRefs.mod | 883 | 4a34e551 | ok |
| /app/VarParam.mod | bpgenvm-c\build\tanda\VarParam\VarParam.mod | 3481 | 1024398c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | 4572ae05 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Gui: en la zona de packs (pack:Stdlib/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:Stdlib/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- GuiShot: COLGADA; reinicio por el cable (RTS; el wire no contesto al RESET): vuelve (resetReason=power-on, uptimeMs=10461)
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Nov 13 2026 19:32:19` imageCrc=`18c88b8e` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c5/build/bpvm_esp32c5.bin (15-11 10:13)
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=USB uptimeMs=397620 fs=159744/16777216 B vmHeapBytes=7320576
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 28 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 14 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 15 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 321 | 336 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 13 | 11 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 867 | 80 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 595 | 401 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 128 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 98 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 112 | 29 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 97 | 17 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 20 | 5 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 15 | 6 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 50 | 33 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 70 | 18 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 60 | 26 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 16 | 15 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 16 | 18 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 15 | 22 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 37 | 27 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 15 | 14 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 234 | 34 | texto distinto |
| IgualdadRefs | **IDENTICO** | 15 | 20 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 20 | 23 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 21 | 0 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=398683 fsUsed=163840 vmHeapBytes=7320576 pila libre: run=13764 io=9912 tareaREPL=9912 heap del sistema, minimo libre=45496
- tras ThrowSinAtrapar: uptimeMs=399148 fsUsed=163840 vmHeapBytes=7320576 pila libre: run=13764 io=9912 tareaREPL=9912 heap del sistema, minimo libre=45496
- tras ThreadTrasMain: uptimeMs=400034 fsUsed=163840 vmHeapBytes=7320576 pila libre: run=13576 io=9912 tareaREPL=9912 heap del sistema, minimo libre=45328
- tras MachineId: uptimeMs=400561 fsUsed=163840 vmHeapBytes=7320576 pila libre: run=13576 io=9912 tareaREPL=9912 heap del sistema, minimo libre=45172
- tras GuiWinJson: uptimeMs=403030 fsUsed=167936 vmHeapBytes=7320576 pila libre: run=12400 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36596
- tras GuiShot: uptimeMs=405076 fsUsed=167936 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.shot` (3350 B, crc f0589b1a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=405760 fsUsed=167936 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras RutaFueraFlash: uptimeMs=406429 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
<details><summary>RutaFueraFlash: cola del stderr del host</summary>

```
throw: ATRAPADA por un catch del programa
[bpvm] throw: "appendFile('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "readFileBytes('/../x810'): no se pudo abrir"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "writeFileBytes('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] pila: el main llegó a 244 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>RutaFueraFlash: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -13,7 +13,7 @@
 rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
 copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
 fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
-lastModified -> IOError: lastModified('/../x810'): no se pudo leer
+lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
 File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
 File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
...
```

oraculo (host):

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no se pudo leer
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no se pudo leer
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```

placa:

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```
</details>

- tras FlashCarpetas: uptimeMs=406927 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras GcHuecoPila: uptimeMs=407431 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras RefCaducadaBuiltin: uptimeMs=407961 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras DivBordes: uptimeMs=408661 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras ListaBorde: uptimeMs=409582 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras IndiceObjeto: uptimeMs=411420 fsUsed=188416 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras GenericosOrden: uptimeMs=413254 fsUsed=184320 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras SobrecargaEspecifica: uptimeMs=413938 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras SobrecargaVisible: uptimeMs=414438 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras BoolBits: uptimeMs=414955 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras ArrayNuevo: uptimeMs=415474 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras BucleConNombre: uptimeMs=416157 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras ErroresConTipo: uptimeMs=417575 fsUsed=184320 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
 o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>ErroresConTipo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -19,7 +19,7 @@
 ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
 ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
 ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
-ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
+ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
 ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
 ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
...
```

oraculo (host):

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```

placa:

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```
</details>

- tras IgualdadRefs: uptimeMs=418075 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras VarParam: uptimeMs=418633 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- tras StrNulo: uptimeMs=419327 fsUsed=176128 vmHeapBytes=7320576 pila libre: run=11384 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36472
- ejecutadas (con EXITED) 24 de 24 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 2270c82e | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 9b053410 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 97dd1bb5 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 42a7e155 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | 59428ce2 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 9c8d42ed | ok |
| /app/BorraCarpetaFichero.mod | bpgenvm-c\build\tanda\BorraCarpetaFichero\BorraCarpetaFichero.mod | 2271 | be28edf2 | ok |
| /app/RutaFueraFlash.mod | bpgenvm-c\build\tanda\RutaFueraFlash\RutaFueraFlash.mod | 3213 | 79cbf8d4 | ok |
| /app/FlashCarpetas.mod | bpgenvm-c\build\tanda\FlashCarpetas\FlashCarpetas.mod | 733 | c21b2696 | ok |
| /app/GcHuecoPila.mod | bpgenvm-c\build\tanda\GcHuecoPila\GcHuecoPila.mod | 1190 | 3304c6f0 | ok |
| /app/RefCaducadaBuiltin.mod | bpgenvm-c\build\tanda\RefCaducadaBuiltin\RefCaducadaBuiltin.mod | 1688 | 845498b8 | ok |
| /app/DivBordes.mod | bpgenvm-c\build\tanda\DivBordes\DivBordes.mod | 798 | 70f63a2f | ok |
| /app/ListaBorde.mod | bpgenvm-c\build\tanda\ListaBorde\ListaBorde.mod | 2918 | e6ddaaf1 | ok |
| /app/IndiceObjeto.mod | bpgenvm-c\build\tanda\IndiceObjeto\IndiceObjeto.mod | 12474 | 80c280cb | ok (por trozos) |
| /app/GenericosOrden.mod | bpgenvm-c\build\tanda\GenericosOrden\GenericosOrden.mod | 8456 | 52134d69 | ok (por trozos) |
| /app/SobrecargaEspecifica.mod | bpgenvm-c\build\tanda\SobrecargaEspecifica\SobrecargaEspecifica.mod | 1358 | 555c6ff1 | ok |
| /app/SobrecargaVisible.mod | bpgenvm-c\build\tanda\SobrecargaVisible\SobrecargaVisible.mod | 1999 | d3797df1 | ok |
| /app/BoolBits.mod | bpgenvm-c\build\tanda\BoolBits\BoolBits.mod | 876 | 7b87929a | ok |
| /app/ArrayNuevo.mod | bpgenvm-c\build\tanda\ArrayNuevo\ArrayNuevo.mod | 2393 | 2feee85e | ok |
| /app/BucleConNombre.mod | bpgenvm-c\build\tanda\BucleConNombre\BucleConNombre.mod | 1734 | 6c35f20c | ok |
| /app/ErroresConTipo.mod | bpgenvm-c\build\tanda\ErroresConTipo\ErroresConTipo.mod | 11493 | 7d9c7755 | ok (por trozos) |
| /app/IgualdadRefs.mod | bpgenvm-c\build\tanda\IgualdadRefs\IgualdadRefs.mod | 883 | 4a34e551 | ok |
| /app/VarParam.mod | bpgenvm-c\build\tanda\VarParam\VarParam.mod | 3481 | 1024398c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | 4572ae05 | ok |

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

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Nov 14 2026 07:43:04` imageCrc=`1b8e8d34` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (15-11 10:12)
- imagen: arch=94 variant=None cpuFreqHz=160000000 resetReason=power-on uptimeMs=467864 fs=102400/9199616 B vmHeapBytes=7844864
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 13 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 191 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 11 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 313 | 336 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 18 | 11 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 66 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 388 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 223 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 423 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 170 | 29 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 184 | 17 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 50 | 5 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 61 | 6 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 230 | 33 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 266 | 18 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 226 | 26 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 54 | 15 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 26 | 18 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 84 | 22 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 95 | 27 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 62 | 14 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 646 | 34 | texto distinto |
| IgualdadRefs | **IDENTICO** | 59 | 20 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 95 | 23 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 107 | 0 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=469198 fsUsed=106496 vmHeapBytes=7844864 pila libre: run=14828 io=9440 tareaREPL=9440 heap del sistema, minimo libre=58168
- tras ThrowSinAtrapar: uptimeMs=469817 fsUsed=106496 vmHeapBytes=7844864 pila libre: run=13876 io=9360 tareaREPL=9360 heap del sistema, minimo libre=58168
- tras ThreadTrasMain: uptimeMs=471034 fsUsed=106496 vmHeapBytes=7844864 pila libre: run=13316 io=9360 tareaREPL=9360 heap del sistema, minimo libre=58020
- tras MachineId: uptimeMs=471917 fsUsed=106496 vmHeapBytes=7844864 pila libre: run=13292 io=9360 tareaREPL=9360 heap del sistema, minimo libre=57844
- tras BorraCarpetaFichero: uptimeMs=473085 fsUsed=106496 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=56912
- tras RutaFueraFlash: uptimeMs=474720 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=55984
<details><summary>RutaFueraFlash: cola del stderr del host</summary>

```
throw: ATRAPADA por un catch del programa
[bpvm] throw: "appendFile('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "readFileBytes('/../x810'): no se pudo abrir"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "writeFileBytes('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] pila: el main llegó a 244 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>RutaFueraFlash: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -13,7 +13,7 @@
 rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
 copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
 fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
-lastModified -> IOError: lastModified('/../x810'): no se pudo leer
+lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
 File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
 File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
...
```

oraculo (host):

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no se pudo leer
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no se pudo leer
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```

placa:

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```
</details>

- tras FlashCarpetas: uptimeMs=475604 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=55984
- tras GcHuecoPila: uptimeMs=476671 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=54924
- tras RefCaducadaBuiltin: uptimeMs=477589 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=54924
- tras DivBordes: uptimeMs=478206 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=54924
- tras ListaBorde: uptimeMs=479591 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=53364
- tras IndiceObjeto: uptimeMs=482594 fsUsed=126976 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=52544
- tras GenericosOrden: uptimeMs=485029 fsUsed=122880 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=52544
- tras SobrecargaEspecifica: uptimeMs=485881 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=52544
- tras SobrecargaVisible: uptimeMs=486631 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=52544
- tras BoolBits: uptimeMs=487315 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=52544
- tras ArrayNuevo: uptimeMs=488232 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=52544
- tras BucleConNombre: uptimeMs=488983 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=52544
- tras ErroresConTipo: uptimeMs=491769 fsUsed=122880 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=51944
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
 o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>ErroresConTipo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -19,7 +19,7 @@
 ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
 ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
 ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
-ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
+ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
 ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
 ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
...
```

oraculo (host):

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```

placa:

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```
</details>

- tras IgualdadRefs: uptimeMs=492453 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=51944
- tras VarParam: uptimeMs=493788 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=51944
- tras StrNulo: uptimeMs=494889 fsUsed=114688 vmHeapBytes=7844864 pila libre: run=11452 io=9344 tareaREPL=9344 heap del sistema, minimo libre=51944
- ejecutadas (con EXITED) 22 de 24 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 2270c82e | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 9b053410 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 97dd1bb5 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 42a7e155 | ok |
| /app/BorraCarpetaFichero.mod | bpgenvm-c\build\tanda\BorraCarpetaFichero\BorraCarpetaFichero.mod | 2271 | be28edf2 | ok |
| /app/RutaFueraFlash.mod | bpgenvm-c\build\tanda\RutaFueraFlash\RutaFueraFlash.mod | 3213 | 79cbf8d4 | ok |
| /app/FlashCarpetas.mod | bpgenvm-c\build\tanda\FlashCarpetas\FlashCarpetas.mod | 733 | c21b2696 | ok |
| /app/GcHuecoPila.mod | bpgenvm-c\build\tanda\GcHuecoPila\GcHuecoPila.mod | 1190 | 3304c6f0 | ok |
| /app/RefCaducadaBuiltin.mod | bpgenvm-c\build\tanda\RefCaducadaBuiltin\RefCaducadaBuiltin.mod | 1688 | 845498b8 | ok |
| /app/DivBordes.mod | bpgenvm-c\build\tanda\DivBordes\DivBordes.mod | 798 | 70f63a2f | ok |
| /app/ListaBorde.mod | bpgenvm-c\build\tanda\ListaBorde\ListaBorde.mod | 2918 | e6ddaaf1 | ok |
| /app/IndiceObjeto.mod | bpgenvm-c\build\tanda\IndiceObjeto\IndiceObjeto.mod | 12474 | 80c280cb | ok (por trozos) |
| /app/GenericosOrden.mod | bpgenvm-c\build\tanda\GenericosOrden\GenericosOrden.mod | 8456 | 52134d69 | ok (por trozos) |
| /app/SobrecargaEspecifica.mod | bpgenvm-c\build\tanda\SobrecargaEspecifica\SobrecargaEspecifica.mod | 1358 | 555c6ff1 | ok |
| /app/SobrecargaVisible.mod | bpgenvm-c\build\tanda\SobrecargaVisible\SobrecargaVisible.mod | 1999 | d3797df1 | ok |
| /app/BoolBits.mod | bpgenvm-c\build\tanda\BoolBits\BoolBits.mod | 876 | 7b87929a | ok |
| /app/ArrayNuevo.mod | bpgenvm-c\build\tanda\ArrayNuevo\ArrayNuevo.mod | 2393 | 2feee85e | ok |
| /app/BucleConNombre.mod | bpgenvm-c\build\tanda\BucleConNombre\BucleConNombre.mod | 1734 | 6c35f20c | ok |
| /app/ErroresConTipo.mod | bpgenvm-c\build\tanda\ErroresConTipo\ErroresConTipo.mod | 11493 | 7d9c7755 | ok (por trozos) |
| /app/IgualdadRefs.mod | bpgenvm-c\build\tanda\IgualdadRefs\IgualdadRefs.mod | 883 | 4a34e551 | ok |
| /app/VarParam.mod | bpgenvm-c\build\tanda\VarParam\VarParam.mod | 3481 | 1024398c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | 4572ae05 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | esp32p4@COM15 | esp32c5@COM6 | esp32s3@COM9 |
|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA |
| GuiWinJson | IDENTICO | IDENTICO | SALTADA |
| GuiShot | COLGADA | NO-PETA | SALTADA |
| BorraCarpetaFichero | IDENTICO | IDENTICO | IDENTICO |
| RutaFueraFlash | DIFIERE | DIFIERE | DIFIERE |
| FlashCarpetas | IDENTICO | IDENTICO | IDENTICO |
| GcHuecoPila | IDENTICO | IDENTICO | IDENTICO |
| RefCaducadaBuiltin | IDENTICO | IDENTICO | IDENTICO |
| DivBordes | IDENTICO | IDENTICO | IDENTICO |
| ListaBorde | IDENTICO | IDENTICO | IDENTICO |
| IndiceObjeto | IDENTICO | IDENTICO | IDENTICO |
| GenericosOrden | IDENTICO | IDENTICO | IDENTICO |
| SobrecargaEspecifica | IDENTICO | IDENTICO | IDENTICO |
| SobrecargaVisible | IDENTICO | IDENTICO | IDENTICO |
| BoolBits | IDENTICO | IDENTICO | IDENTICO |
| ArrayNuevo | IDENTICO | IDENTICO | IDENTICO |
| BucleConNombre | IDENTICO | IDENTICO | IDENTICO |
| ErroresConTipo | DIFIERE | DIFIERE | DIFIERE |
| IgualdadRefs | IDENTICO | IDENTICO | IDENTICO |
| VarParam | IDENTICO | IDENTICO | IDENTICO |
| StrNulo | IDENTICO | IDENTICO | IDENTICO |

NECESITA OJOS: GuiShot en esp32c5@COM6: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png  
SALTADA: GuiWinJson en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-11-15 10:33:23

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\pf\tanda_lote15nov.json` (24 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-14 13:33:40) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-14 13:33:59) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 2270c82e | 1611 | ok | 0 | 22 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 9b053410 | 295 | ok | 1 | 13 | 1 |
| ThreadTrasMain | ThreadTrasMain | 97dd1bb5 | 532 | ok | 0 | 336 | 3 |
| MachineId | MachineId | 42a7e155 | 452 | ok | 0 | 10 | 3 |
| GuiWinJson | GuiWinJson | 59428ce2 | 2041 | ok | 0 | 58 | 17 |
| GuiShot | GuiShot | 9c8d42ed | 1347 | ok | 0 | 397 | 8 |
| BorraCarpetaFichero | BorraCarpetaFichero | be28edf2 | 2271 | ok | 0 | 27 | 9 |
| RutaFueraFlash | RutaFueraFlash | 79cbf8d4 | 3213 | ok | 0 | 14 | 39 |
| FlashCarpetas | FlashCarpetas | c21b2696 | 733 | ok | 0 | 21 | 7 |
| GcHuecoPila | GcHuecoPila | 3304c6f0 | 1190 | ok | 0 | 12 | 3 |
| RefCaducadaBuiltin | RefCaducadaBuiltin | 845498b8 | 1688 | ok | 0 | 12 | 6 |
| DivBordes | DivBordes | 70f63a2f | 798 | ok | 0 | 10 | 10 |
| ListaBorde | ListaBorde | e6ddaaf1 | 2918 | ok | 0 | 23 | 26 |
| IndiceObjeto | IndiceObjeto | 80c280cb | 12474 | ok | 0 | 23 | 36 |
| GenericosOrden | GenericosOrden | 52134d69 | 8456 | ok | 0 | 33 | 30 |
| SobrecargaEspecifica | SobrecargaEspecifica | 555c6ff1 | 1358 | ok | 0 | 9 | 9 |
| SobrecargaVisible | SobrecargaVisible | d3797df1 | 1999 | ok | 0 | 7 | 4 |
| BoolBits | BoolBits | 7b87929a | 876 | ok | 0 | 20 | 15 |
| ArrayNuevo | ArrayNuevo | 2feee85e | 2393 | ok | 0 | 33 | 17 |
| BucleConNombre | BucleConNombre | 6c35f20c | 1734 | ok | 0 | 12 | 12 |
| ErroresConTipo | ErroresConTipo | 7d9c7755 | 11493 | ok | 0 | 32 | 48 |
| IgualdadRefs | IgualdadRefs | 4a34e551 | 883 | ok | 0 | 23 | 8 |
| VarParam | VarParam | 1024398c | 3481 | ok | 0 | 13 | 12 |
| StrNulo | StrNulo | 4572ae05 | 2784 | ok | 0 | 6 | 14 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Nov 13 2026 12:41:21` imageCrc=`10e3902d` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c6/build/bpvm_esp32c6.bin (15-11 10:14)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=USB uptimeMs=452144 fs=45056/1261568 B vmHeapBytes=114688
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 1 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 7 | 22 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 4 | 13 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 336 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 10 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 1057 | 68 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 574 | 381 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 201 | 27 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 108 | 14 | texto distinto |
| FlashCarpetas | **IDENTICO** | 145 | 21 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 119 | 12 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 9 | 12 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 6 | 10 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 39 | 23 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 65 | 23 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 54 | 33 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 8 | 9 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 7 | 7 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 6 | 20 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 10 | 33 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 6 | 12 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 173 | 32 | texto distinto |
| IgualdadRefs | **IDENTICO** | 6 | 23 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 13 | 13 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 11 | 6 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=452877 fsUsed=49152 vmHeapBytes=114688 pila libre: run=7620 io=5824 tareaREPL=5824 heap del sistema, minimo libre=73048
- tras ThrowSinAtrapar: uptimeMs=453349 fsUsed=49152 vmHeapBytes=114688 pila libre: run=7620 io=5824 tareaREPL=5824 heap del sistema, minimo libre=73048
- tras ThreadTrasMain: uptimeMs=454303 fsUsed=49152 vmHeapBytes=114688 pila libre: run=7432 io=5824 tareaREPL=5824 heap del sistema, minimo libre=72880
- tras MachineId: uptimeMs=454831 fsUsed=49152 vmHeapBytes=114688 pila libre: run=7432 io=5824 tareaREPL=5824 heap del sistema, minimo libre=72748
- tras GuiWinJson: uptimeMs=457731 fsUsed=53248 vmHeapBytes=114688 pila libre: run=6064 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras GuiShot: uptimeMs=459784 fsUsed=53248 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=460866 fsUsed=53248 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras RutaFueraFlash: uptimeMs=461452 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
<details><summary>RutaFueraFlash: cola del stderr del host</summary>

```
throw: ATRAPADA por un catch del programa
[bpvm] throw: "appendFile('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "readFileBytes('/../x810'): no se pudo abrir"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "writeFileBytes('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] pila: el main llegó a 244 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>RutaFueraFlash: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -13,7 +13,7 @@
 rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
 copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
 fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
-lastModified -> IOError: lastModified('/../x810'): no se pudo leer
+lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
 File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
 File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
...
```

oraculo (host):

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no se pudo leer
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no se pudo leer
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```

placa:

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```
</details>

- tras FlashCarpetas: uptimeMs=461935 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras GcHuecoPila: uptimeMs=462435 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras RefCaducadaBuiltin: uptimeMs=462920 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras DivBordes: uptimeMs=463404 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras ListaBorde: uptimeMs=463986 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras IndiceObjeto: uptimeMs=465011 fsUsed=73728 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras GenericosOrden: uptimeMs=465973 fsUsed=69632 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras SobrecargaEspecifica: uptimeMs=466455 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras SobrecargaVisible: uptimeMs=466941 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras BoolBits: uptimeMs=467423 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras ArrayNuevo: uptimeMs=467942 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras BucleConNombre: uptimeMs=468427 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras ErroresConTipo: uptimeMs=469760 fsUsed=69632 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
 o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>ErroresConTipo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -19,7 +19,7 @@
 ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
 ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
 ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
-ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
+ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
 ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
 ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
...
```

oraculo (host):

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```

placa:

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```
</details>

- tras IgualdadRefs: uptimeMs=470242 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras VarParam: uptimeMs=471161 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- tras StrNulo: uptimeMs=471738 fsUsed=61440 vmHeapBytes=114688 pila libre: run=5284 io=5580 tareaREPL=5580 heap del sistema, minimo libre=31276
- ejecutadas (con EXITED) 24 de 24 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 2270c82e | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 9b053410 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 97dd1bb5 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 42a7e155 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | 59428ce2 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 9c8d42ed | ok |
| /app/BorraCarpetaFichero.mod | bpgenvm-c\build\tanda\BorraCarpetaFichero\BorraCarpetaFichero.mod | 2271 | be28edf2 | ok |
| /app/RutaFueraFlash.mod | bpgenvm-c\build\tanda\RutaFueraFlash\RutaFueraFlash.mod | 3213 | 79cbf8d4 | ok |
| /app/FlashCarpetas.mod | bpgenvm-c\build\tanda\FlashCarpetas\FlashCarpetas.mod | 733 | c21b2696 | ok |
| /app/GcHuecoPila.mod | bpgenvm-c\build\tanda\GcHuecoPila\GcHuecoPila.mod | 1190 | 3304c6f0 | ok |
| /app/RefCaducadaBuiltin.mod | bpgenvm-c\build\tanda\RefCaducadaBuiltin\RefCaducadaBuiltin.mod | 1688 | 845498b8 | ok |
| /app/DivBordes.mod | bpgenvm-c\build\tanda\DivBordes\DivBordes.mod | 798 | 70f63a2f | ok |
| /app/ListaBorde.mod | bpgenvm-c\build\tanda\ListaBorde\ListaBorde.mod | 2918 | e6ddaaf1 | ok |
| /app/IndiceObjeto.mod | bpgenvm-c\build\tanda\IndiceObjeto\IndiceObjeto.mod | 12474 | 80c280cb | ok (por trozos) |
| /app/GenericosOrden.mod | bpgenvm-c\build\tanda\GenericosOrden\GenericosOrden.mod | 8456 | 52134d69 | ok (por trozos) |
| /app/SobrecargaEspecifica.mod | bpgenvm-c\build\tanda\SobrecargaEspecifica\SobrecargaEspecifica.mod | 1358 | 555c6ff1 | ok |
| /app/SobrecargaVisible.mod | bpgenvm-c\build\tanda\SobrecargaVisible\SobrecargaVisible.mod | 1999 | d3797df1 | ok |
| /app/BoolBits.mod | bpgenvm-c\build\tanda\BoolBits\BoolBits.mod | 876 | 7b87929a | ok |
| /app/ArrayNuevo.mod | bpgenvm-c\build\tanda\ArrayNuevo\ArrayNuevo.mod | 2393 | 2feee85e | ok |
| /app/BucleConNombre.mod | bpgenvm-c\build\tanda\BucleConNombre\BucleConNombre.mod | 1734 | 6c35f20c | ok |
| /app/ErroresConTipo.mod | bpgenvm-c\build\tanda\ErroresConTipo\ErroresConTipo.mod | 11493 | 7d9c7755 | ok (por trozos) |
| /app/IgualdadRefs.mod | bpgenvm-c\build\tanda\IgualdadRefs\IgualdadRefs.mod | 883 | 4a34e551 | ok |
| /app/VarParam.mod | bpgenvm-c\build\tanda\VarParam\VarParam.mod | 3481 | 1024398c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | 4572ae05 | ok |

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

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |
| BorraCarpetaFichero | IDENTICO |
| RutaFueraFlash | DIFIERE |
| FlashCarpetas | IDENTICO |
| GcHuecoPila | IDENTICO |
| RefCaducadaBuiltin | IDENTICO |
| DivBordes | IDENTICO |
| ListaBorde | IDENTICO |
| IndiceObjeto | IDENTICO |
| GenericosOrden | IDENTICO |
| SobrecargaEspecifica | IDENTICO |
| SobrecargaVisible | IDENTICO |
| BoolBits | IDENTICO |
| ArrayNuevo | IDENTICO |
| BucleConNombre | IDENTICO |
| ErroresConTipo | DIFIERE |
| IgualdadRefs | IDENTICO |
| VarParam | IDENTICO |
| StrNulo | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  


## Tanda 2026-11-15 10:59:34

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\pf\tanda_lote15nov.json` (24 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-14 13:33:40) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-14 13:33:59) - sabor `v2-gui1-lvgl0`
- placas conectadas: 4 (rp2350a@COM22, rp2350b@COM4, stm32u5g9@COM12, esp32c3@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 2270c82e | 1611 | ok | 0 | 12 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 9b053410 | 295 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | 97dd1bb5 | 532 | ok | 0 | 323 | 3 |
| MachineId | MachineId | 42a7e155 | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | 59428ce2 | 2041 | ok | 0 | 65 | 17 |
| GuiShot | GuiShot | 9c8d42ed | 1347 | ok | 0 | 393 | 8 |
| BorraCarpetaFichero | BorraCarpetaFichero | be28edf2 | 2271 | ok | 0 | 16 | 9 |
| RutaFueraFlash | RutaFueraFlash | 79cbf8d4 | 3213 | ok | 0 | 22 | 39 |
| FlashCarpetas | FlashCarpetas | c21b2696 | 733 | ok | 0 | 20 | 7 |
| GcHuecoPila | GcHuecoPila | 3304c6f0 | 1190 | ok | 0 | 15 | 3 |
| RefCaducadaBuiltin | RefCaducadaBuiltin | 845498b8 | 1688 | ok | 0 | 15 | 6 |
| DivBordes | DivBordes | 70f63a2f | 798 | ok | 0 | 5 | 10 |
| ListaBorde | ListaBorde | e6ddaaf1 | 2918 | ok | 0 | 26 | 26 |
| IndiceObjeto | IndiceObjeto | 80c280cb | 12474 | ok | 0 | 33 | 36 |
| GenericosOrden | GenericosOrden | 52134d69 | 8456 | ok | 0 | 20 | 30 |
| SobrecargaEspecifica | SobrecargaEspecifica | 555c6ff1 | 1358 | ok | 0 | 11 | 9 |
| SobrecargaVisible | SobrecargaVisible | d3797df1 | 1999 | ok | 0 | 14 | 4 |
| BoolBits | BoolBits | 7b87929a | 876 | ok | 0 | 14 | 15 |
| ArrayNuevo | ArrayNuevo | 2feee85e | 2393 | ok | 0 | 42 | 17 |
| BucleConNombre | BucleConNombre | 6c35f20c | 1734 | ok | 0 | 9 | 12 |
| ErroresConTipo | ErroresConTipo | 7d9c7755 | 11493 | ok | 0 | 33 | 48 |
| IgualdadRefs | IgualdadRefs | 4a34e551 | 883 | ok | 0 | 13 | 8 |
| VarParam | VarParam | 1024398c | 3481 | ok | 0 | 12 | 12 |
| StrNulo | StrNulo | 4572ae05 | 2784 | ok | 0 | 13 | 14 |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Nov 13 2026 12:42:24` imageCrc=`aed8e4bf` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (15-11 10:14)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=61242 fs=262144/1048576 B vmHeapBytes=219536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 39 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 24 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 214 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 9 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 65 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 393 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 173 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 150 | 22 | texto distinto |
| FlashCarpetas | **IDENTICO** | 115 | 20 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 148 | 15 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 571 | 15 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 14 | 5 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 72 | 26 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 102 | 33 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 88 | 20 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 17 | 11 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 12 | 14 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 18 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 28 | 42 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 16 | 9 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 2048 | 33 | texto distinto |
| IgualdadRefs | **IDENTICO** | 14 | 13 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 27 | 12 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 25 | 13 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=62188 fsUsed=266240 vmHeapBytes=219536 pila libre: run=13432 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=62796 fsUsed=266240 vmHeapBytes=219536 pila libre: run=14136 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=63720 fsUsed=266240 vmHeapBytes=219536 pila libre: run=13432 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=64293 fsUsed=266240 vmHeapBytes=219536 pila libre: run=13432 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras BorraCarpetaFichero: uptimeMs=65030 fsUsed=266240 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras RutaFueraFlash: uptimeMs=65893 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
<details><summary>RutaFueraFlash: cola del stderr del host</summary>

```
throw: ATRAPADA por un catch del programa
[bpvm] throw: "appendFile('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "readFileBytes('/../x810'): no se pudo abrir"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "writeFileBytes('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] pila: el main llegó a 244 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>RutaFueraFlash: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -13,7 +13,7 @@
 rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
 copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
 fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
-lastModified -> IOError: lastModified('/../x810'): no se pudo leer
+lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
 File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
 File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
...
```

oraculo (host):

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no se pudo leer
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no se pudo leer
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```

placa:

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```
</details>

- tras FlashCarpetas: uptimeMs=66414 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras GcHuecoPila: uptimeMs=66930 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras RefCaducadaBuiltin: uptimeMs=67944 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras DivBordes: uptimeMs=68471 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ListaBorde: uptimeMs=69546 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras IndiceObjeto: uptimeMs=71276 fsUsed=286720 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras GenericosOrden: uptimeMs=72381 fsUsed=282624 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras SobrecargaEspecifica: uptimeMs=72921 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras SobrecargaVisible: uptimeMs=73467 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras BoolBits: uptimeMs=74072 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ArrayNuevo: uptimeMs=74622 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras BucleConNombre: uptimeMs=75155 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ErroresConTipo: uptimeMs=78509 fsUsed=282624 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
 o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>ErroresConTipo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -19,7 +19,7 @@
 ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
 ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
 ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
-ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
+ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
 ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
 ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
...
```

oraculo (host):

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```

placa:

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```
</details>

- tras IgualdadRefs: uptimeMs=79054 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras VarParam: uptimeMs=79610 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras StrNulo: uptimeMs=80173 fsUsed=274432 vmHeapBytes=219536 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- ejecutadas (con EXITED) 22 de 24 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 2270c82e | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 9b053410 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 97dd1bb5 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 42a7e155 | ok |
| /app/BorraCarpetaFichero.mod | bpgenvm-c\build\tanda\BorraCarpetaFichero\BorraCarpetaFichero.mod | 2271 | be28edf2 | ok |
| /app/RutaFueraFlash.mod | bpgenvm-c\build\tanda\RutaFueraFlash\RutaFueraFlash.mod | 3213 | 79cbf8d4 | ok |
| /app/FlashCarpetas.mod | bpgenvm-c\build\tanda\FlashCarpetas\FlashCarpetas.mod | 733 | c21b2696 | ok |
| /app/GcHuecoPila.mod | bpgenvm-c\build\tanda\GcHuecoPila\GcHuecoPila.mod | 1190 | 3304c6f0 | ok |
| /app/RefCaducadaBuiltin.mod | bpgenvm-c\build\tanda\RefCaducadaBuiltin\RefCaducadaBuiltin.mod | 1688 | 845498b8 | ok |
| /app/DivBordes.mod | bpgenvm-c\build\tanda\DivBordes\DivBordes.mod | 798 | 70f63a2f | ok |
| /app/ListaBorde.mod | bpgenvm-c\build\tanda\ListaBorde\ListaBorde.mod | 2918 | e6ddaaf1 | ok |
| /app/IndiceObjeto.mod | bpgenvm-c\build\tanda\IndiceObjeto\IndiceObjeto.mod | 12474 | 80c280cb | ok (por trozos) |
| /app/GenericosOrden.mod | bpgenvm-c\build\tanda\GenericosOrden\GenericosOrden.mod | 8456 | 52134d69 | ok (por trozos) |
| /app/SobrecargaEspecifica.mod | bpgenvm-c\build\tanda\SobrecargaEspecifica\SobrecargaEspecifica.mod | 1358 | 555c6ff1 | ok |
| /app/SobrecargaVisible.mod | bpgenvm-c\build\tanda\SobrecargaVisible\SobrecargaVisible.mod | 1999 | d3797df1 | ok |
| /app/BoolBits.mod | bpgenvm-c\build\tanda\BoolBits\BoolBits.mod | 876 | 7b87929a | ok |
| /app/ArrayNuevo.mod | bpgenvm-c\build\tanda\ArrayNuevo\ArrayNuevo.mod | 2393 | 2feee85e | ok |
| /app/BucleConNombre.mod | bpgenvm-c\build\tanda\BucleConNombre\BucleConNombre.mod | 1734 | 6c35f20c | ok |
| /app/ErroresConTipo.mod | bpgenvm-c\build\tanda\ErroresConTipo\ErroresConTipo.mod | 11493 | 7d9c7755 | ok (por trozos) |
| /app/IgualdadRefs.mod | bpgenvm-c\build\tanda\IgualdadRefs\IgualdadRefs.mod | 883 | 4a34e551 | ok |
| /app/VarParam.mod | bpgenvm-c\build\tanda\VarParam\VarParam.mod | 3481 | 1024398c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | 4572ae05 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Nov 13 2026 12:42:26` imageCrc=`c69e835d` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico_m.elf (15-11 10:14)
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=342745 fs=344064/10248192 B vmHeapBytes=6275072
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 57 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 22 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 208 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 317 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 16 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 65 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 393 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 160 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 105 | 22 | texto distinto |
| FlashCarpetas | **IDENTICO** | 105 | 20 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 49 | 15 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 539 | 15 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 18 | 5 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 46 | 26 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 70 | 33 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 60 | 20 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 20 | 11 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 18 | 14 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 18 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 23 | 42 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 18 | 9 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 1922 | 33 | texto distinto |
| IgualdadRefs | **IDENTICO** | 17 | 13 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 22 | 12 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 21 | 13 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=343573 fsUsed=348160 vmHeapBytes=6275072 pila libre: run=13592 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=344087 fsUsed=348160 vmHeapBytes=6275072 pila libre: run=14144 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=345020 fsUsed=348160 vmHeapBytes=6275072 pila libre: run=13296 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=345574 fsUsed=348160 vmHeapBytes=6275072 pila libre: run=13472 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras BorraCarpetaFichero: uptimeMs=346158 fsUsed=348160 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras RutaFueraFlash: uptimeMs=347009 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
<details><summary>RutaFueraFlash: cola del stderr del host</summary>

```
throw: ATRAPADA por un catch del programa
[bpvm] throw: "appendFile('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "readFileBytes('/../x810'): no se pudo abrir"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "writeFileBytes('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] pila: el main llegó a 244 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>RutaFueraFlash: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -13,7 +13,7 @@
 rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
 copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
 fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
-lastModified -> IOError: lastModified('/../x810'): no se pudo leer
+lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
 File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
 File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
...
```

oraculo (host):

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no se pudo leer
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no se pudo leer
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```

placa:

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```
</details>

- tras FlashCarpetas: uptimeMs=347542 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras GcHuecoPila: uptimeMs=348075 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras RefCaducadaBuiltin: uptimeMs=349023 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras DivBordes: uptimeMs=349746 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ListaBorde: uptimeMs=350417 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras IndiceObjeto: uptimeMs=351629 fsUsed=368640 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras GenericosOrden: uptimeMs=352745 fsUsed=364544 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras SobrecargaEspecifica: uptimeMs=353281 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras SobrecargaVisible: uptimeMs=353833 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras BoolBits: uptimeMs=354444 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ArrayNuevo: uptimeMs=354998 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras BucleConNombre: uptimeMs=355530 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras ErroresConTipo: uptimeMs=358337 fsUsed=364544 vmHeapBytes=6275072 pila libre: run=12152 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
 o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>ErroresConTipo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -19,7 +19,7 @@
 ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
 ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
 ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
-ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
+ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
 ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
 ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
...
```

oraculo (host):

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```

placa:

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```
</details>

- tras IgualdadRefs: uptimeMs=358916 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras VarParam: uptimeMs=359454 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12152 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- tras StrNulo: uptimeMs=359999 fsUsed=356352 vmHeapBytes=6275072 pila libre: run=12168 io=9744 tareaREPL=9744 heap del sistema, minimo libre=6632
- ejecutadas (con EXITED) 22 de 24 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 2270c82e | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 9b053410 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 97dd1bb5 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 42a7e155 | ok |
| /app/BorraCarpetaFichero.mod | bpgenvm-c\build\tanda\BorraCarpetaFichero\BorraCarpetaFichero.mod | 2271 | be28edf2 | ok |
| /app/RutaFueraFlash.mod | bpgenvm-c\build\tanda\RutaFueraFlash\RutaFueraFlash.mod | 3213 | 79cbf8d4 | ok |
| /app/FlashCarpetas.mod | bpgenvm-c\build\tanda\FlashCarpetas\FlashCarpetas.mod | 733 | c21b2696 | ok |
| /app/GcHuecoPila.mod | bpgenvm-c\build\tanda\GcHuecoPila\GcHuecoPila.mod | 1190 | 3304c6f0 | ok |
| /app/RefCaducadaBuiltin.mod | bpgenvm-c\build\tanda\RefCaducadaBuiltin\RefCaducadaBuiltin.mod | 1688 | 845498b8 | ok |
| /app/DivBordes.mod | bpgenvm-c\build\tanda\DivBordes\DivBordes.mod | 798 | 70f63a2f | ok |
| /app/ListaBorde.mod | bpgenvm-c\build\tanda\ListaBorde\ListaBorde.mod | 2918 | e6ddaaf1 | ok |
| /app/IndiceObjeto.mod | bpgenvm-c\build\tanda\IndiceObjeto\IndiceObjeto.mod | 12474 | 80c280cb | ok (por trozos) |
| /app/GenericosOrden.mod | bpgenvm-c\build\tanda\GenericosOrden\GenericosOrden.mod | 8456 | 52134d69 | ok (por trozos) |
| /app/SobrecargaEspecifica.mod | bpgenvm-c\build\tanda\SobrecargaEspecifica\SobrecargaEspecifica.mod | 1358 | 555c6ff1 | ok |
| /app/SobrecargaVisible.mod | bpgenvm-c\build\tanda\SobrecargaVisible\SobrecargaVisible.mod | 1999 | d3797df1 | ok |
| /app/BoolBits.mod | bpgenvm-c\build\tanda\BoolBits\BoolBits.mod | 876 | 7b87929a | ok |
| /app/ArrayNuevo.mod | bpgenvm-c\build\tanda\ArrayNuevo\ArrayNuevo.mod | 2393 | 2feee85e | ok |
| /app/BucleConNombre.mod | bpgenvm-c\build\tanda\BucleConNombre\BucleConNombre.mod | 1734 | 6c35f20c | ok |
| /app/ErroresConTipo.mod | bpgenvm-c\build\tanda\ErroresConTipo\ErroresConTipo.mod | 11493 | 7d9c7755 | ok (por trozos) |
| /app/IgualdadRefs.mod | bpgenvm-c\build\tanda\IgualdadRefs\IgualdadRefs.mod | 883 | 4a34e551 | ok |
| /app/VarParam.mod | bpgenvm-c\build\tanda\VarParam\VarParam.mod | 3481 | 1024398c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | 4572ae05 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Nov 15 2026 10:15:55` imageCrc=`35427bd7` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (15-11 10:16)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=321309 fs=131072/1032192 B vmHeapBytes=1032192
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 197 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 8 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 24 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 610 | 596 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 681 | 1027 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 139 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 430 | 22 | texto distinto |
| FlashCarpetas | **IDENTICO** | 86 | 20 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 61 | 15 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 56 | 15 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 64 | 5 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 242 | 26 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 288 | 33 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 241 | 20 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 59 | 11 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 31 | 14 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 88 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 100 | 42 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 68 | 9 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 662 | 33 | texto distinto |
| IgualdadRefs | **IDENTICO** | 64 | 13 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 99 | 12 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 114 | 13 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=322277 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=322859 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=323441 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=324060 fsUsed=139264 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiWinJson: uptimeMs=327216 fsUsed=147456 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiShot: uptimeMs=330548 fsUsed=155648 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=332366 fsUsed=155648 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RutaFueraFlash: uptimeMs=333621 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
<details><summary>RutaFueraFlash: cola del stderr del host</summary>

```
throw: ATRAPADA por un catch del programa
[bpvm] throw: "appendFile('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "readFileBytes('/../x810'): no se pudo abrir"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "writeFileBytes('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] pila: el main llegó a 244 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>RutaFueraFlash: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -13,7 +13,7 @@
 rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
 copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
 fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
-lastModified -> IOError: lastModified('/../x810'): no se pudo leer
+lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
 File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
 File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
...
```

oraculo (host):

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no se pudo leer
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no se pudo leer
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```

placa:

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```
</details>

- tras FlashCarpetas: uptimeMs=334235 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GcHuecoPila: uptimeMs=334901 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RefCaducadaBuiltin: uptimeMs=335585 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras DivBordes: uptimeMs=336199 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ListaBorde: uptimeMs=337003 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras IndiceObjeto: uptimeMs=339174 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GenericosOrden: uptimeMs=340952 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaEspecifica: uptimeMs=341607 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaVisible: uptimeMs=342323 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BoolBits: uptimeMs=342943 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayNuevo: uptimeMs=343709 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BucleConNombre: uptimeMs=344411 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ErroresConTipo: uptimeMs=346494 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
 o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>ErroresConTipo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -19,7 +19,7 @@
 ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
 ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
 ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
-ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
+ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
 ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
 ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
...
```

oraculo (host):

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```

placa:

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```
</details>

- tras IgualdadRefs: uptimeMs=347113 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras VarParam: uptimeMs=347979 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3216 tareaREPL=None heap del sistema, minimo libre=None
- tras StrNulo: uptimeMs=348779 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 24 de 24 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 2270c82e | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 9b053410 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 97dd1bb5 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 42a7e155 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2041 | 59428ce2 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1347 | 9c8d42ed | ok |
| /app/BorraCarpetaFichero.mod | bpgenvm-c\build\tanda\BorraCarpetaFichero\BorraCarpetaFichero.mod | 2271 | be28edf2 | ok |
| /app/RutaFueraFlash.mod | bpgenvm-c\build\tanda\RutaFueraFlash\RutaFueraFlash.mod | 3213 | 79cbf8d4 | ok |
| /app/FlashCarpetas.mod | bpgenvm-c\build\tanda\FlashCarpetas\FlashCarpetas.mod | 733 | c21b2696 | ok |
| /app/GcHuecoPila.mod | bpgenvm-c\build\tanda\GcHuecoPila\GcHuecoPila.mod | 1190 | 3304c6f0 | ok |
| /app/RefCaducadaBuiltin.mod | bpgenvm-c\build\tanda\RefCaducadaBuiltin\RefCaducadaBuiltin.mod | 1688 | 845498b8 | ok |
| /app/DivBordes.mod | bpgenvm-c\build\tanda\DivBordes\DivBordes.mod | 798 | 70f63a2f | ok |
| /app/ListaBorde.mod | bpgenvm-c\build\tanda\ListaBorde\ListaBorde.mod | 2918 | e6ddaaf1 | ok |
| /app/IndiceObjeto.mod | bpgenvm-c\build\tanda\IndiceObjeto\IndiceObjeto.mod | 12474 | 80c280cb | ok (por trozos) |
| /app/GenericosOrden.mod | bpgenvm-c\build\tanda\GenericosOrden\GenericosOrden.mod | 8456 | 52134d69 | ok (por trozos) |
| /app/SobrecargaEspecifica.mod | bpgenvm-c\build\tanda\SobrecargaEspecifica\SobrecargaEspecifica.mod | 1358 | 555c6ff1 | ok |
| /app/SobrecargaVisible.mod | bpgenvm-c\build\tanda\SobrecargaVisible\SobrecargaVisible.mod | 1999 | d3797df1 | ok |
| /app/BoolBits.mod | bpgenvm-c\build\tanda\BoolBits\BoolBits.mod | 876 | 7b87929a | ok |
| /app/ArrayNuevo.mod | bpgenvm-c\build\tanda\ArrayNuevo\ArrayNuevo.mod | 2393 | 2feee85e | ok |
| /app/BucleConNombre.mod | bpgenvm-c\build\tanda\BucleConNombre\BucleConNombre.mod | 1734 | 6c35f20c | ok |
| /app/ErroresConTipo.mod | bpgenvm-c\build\tanda\ErroresConTipo\ErroresConTipo.mod | 11493 | 7d9c7755 | ok (por trozos) |
| /app/IgualdadRefs.mod | bpgenvm-c\build\tanda\IgualdadRefs\IgualdadRefs.mod | 883 | 4a34e551 | ok |
| /app/VarParam.mod | bpgenvm-c\build\tanda\VarParam\VarParam.mod | 3481 | 1024398c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | 4572ae05 | ok |

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

### Placa esp32c3 - E072A1214078

- sello: boardName=`esp32c3` uniqueId=`E072A1214078` serverName=`bpvm-esp32c3` serverBuild=`Nov 13 2026 12:40:07` imageCrc=`372b630d` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c3/build/bpvm_esp32c3.bin (15-11 10:13)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=USB uptimeMs=184709 fs=45056/1523712 B vmHeapBytes=98304
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 1 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 7 | 12 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 5 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 307 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 65 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 393 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 158 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 105 | 22 | texto distinto |
| FlashCarpetas | **IDENTICO** | 115 | 20 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 71 | 15 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 12 | 15 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 6 | 5 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 50 | 26 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 79 | 33 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 69 | 20 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 9 | 11 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 8 | 14 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 8 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 13 | 42 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 8 | 9 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 266 | 33 | texto distinto |
| IgualdadRefs | **IDENTICO** | 8 | 13 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 18 | 12 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 14 | 13 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=185361 fsUsed=49152 vmHeapBytes=98304 pila libre: run=9668 io=9936 tareaREPL=9936 heap del sistema, minimo libre=95836
- tras ThrowSinAtrapar: uptimeMs=185831 fsUsed=49152 vmHeapBytes=98304 pila libre: run=9668 io=9936 tareaREPL=9936 heap del sistema, minimo libre=95836
- tras ThreadTrasMain: uptimeMs=186317 fsUsed=49152 vmHeapBytes=98304 pila libre: run=9456 io=9936 tareaREPL=9936 heap del sistema, minimo libre=95700
- tras MachineId: uptimeMs=186810 fsUsed=49152 vmHeapBytes=98304 pila libre: run=9456 io=9936 tareaREPL=9936 heap del sistema, minimo libre=95524
- tras BorraCarpetaFichero: uptimeMs=187325 fsUsed=49152 vmHeapBytes=98304 pila libre: run=7756 io=9936 tareaREPL=9936 heap del sistema, minimo libre=94592
- tras RutaFueraFlash: uptimeMs=187796 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9936 tareaREPL=9936 heap del sistema, minimo libre=93664
<details><summary>RutaFueraFlash: cola del stderr del host</summary>

```
throw: ATRAPADA por un catch del programa
[bpvm] throw: "appendFile('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "readFileBytes('/../x810'): no se pudo abrir"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "writeFileBytes('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] pila: el main llegó a 244 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>RutaFueraFlash: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -13,7 +13,7 @@
 rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
 copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
 fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
-lastModified -> IOError: lastModified('/../x810'): no se pudo leer
+lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
 File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
 File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
...
```

oraculo (host):

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no se pudo leer
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no se pudo leer
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```

placa:

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```
</details>

- tras FlashCarpetas: uptimeMs=188246 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9936 tareaREPL=9936 heap del sistema, minimo libre=93664
- tras GcHuecoPila: uptimeMs=188723 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=90744
- tras RefCaducadaBuiltin: uptimeMs=189212 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=90744
- tras DivBordes: uptimeMs=189697 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=90744
- tras ListaBorde: uptimeMs=190427 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=88864
- tras IndiceObjeto: uptimeMs=191849 fsUsed=69632 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80764
- tras GenericosOrden: uptimeMs=193234 fsUsed=65536 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80764
- tras SobrecargaEspecifica: uptimeMs=193717 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80764
- tras SobrecargaVisible: uptimeMs=194215 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80764
- tras BoolBits: uptimeMs=194700 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80764
- tras ArrayNuevo: uptimeMs=195236 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80764
- tras BucleConNombre: uptimeMs=195719 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80764
- tras ErroresConTipo: uptimeMs=197062 fsUsed=65536 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80412
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
 o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>ErroresConTipo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -19,7 +19,7 @@
 ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
 ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
 ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
-ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
+ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
 ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
 ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
...
```

oraculo (host):

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```

placa:

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```
</details>

- tras IgualdadRefs: uptimeMs=197537 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80412
- tras VarParam: uptimeMs=198024 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80412
- tras StrNulo: uptimeMs=198572 fsUsed=57344 vmHeapBytes=98304 pila libre: run=7756 io=9896 tareaREPL=9896 heap del sistema, minimo libre=80412
- ejecutadas (con EXITED) 22 de 24 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 2270c82e | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 9b053410 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 97dd1bb5 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 42a7e155 | ok |
| /app/BorraCarpetaFichero.mod | bpgenvm-c\build\tanda\BorraCarpetaFichero\BorraCarpetaFichero.mod | 2271 | be28edf2 | ok |
| /app/RutaFueraFlash.mod | bpgenvm-c\build\tanda\RutaFueraFlash\RutaFueraFlash.mod | 3213 | 79cbf8d4 | ok |
| /app/FlashCarpetas.mod | bpgenvm-c\build\tanda\FlashCarpetas\FlashCarpetas.mod | 733 | c21b2696 | ok |
| /app/GcHuecoPila.mod | bpgenvm-c\build\tanda\GcHuecoPila\GcHuecoPila.mod | 1190 | 3304c6f0 | ok |
| /app/RefCaducadaBuiltin.mod | bpgenvm-c\build\tanda\RefCaducadaBuiltin\RefCaducadaBuiltin.mod | 1688 | 845498b8 | ok |
| /app/DivBordes.mod | bpgenvm-c\build\tanda\DivBordes\DivBordes.mod | 798 | 70f63a2f | ok |
| /app/ListaBorde.mod | bpgenvm-c\build\tanda\ListaBorde\ListaBorde.mod | 2918 | e6ddaaf1 | ok |
| /app/IndiceObjeto.mod | bpgenvm-c\build\tanda\IndiceObjeto\IndiceObjeto.mod | 12474 | 80c280cb | ok (por trozos) |
| /app/GenericosOrden.mod | bpgenvm-c\build\tanda\GenericosOrden\GenericosOrden.mod | 8456 | 52134d69 | ok (por trozos) |
| /app/SobrecargaEspecifica.mod | bpgenvm-c\build\tanda\SobrecargaEspecifica\SobrecargaEspecifica.mod | 1358 | 555c6ff1 | ok |
| /app/SobrecargaVisible.mod | bpgenvm-c\build\tanda\SobrecargaVisible\SobrecargaVisible.mod | 1999 | d3797df1 | ok |
| /app/BoolBits.mod | bpgenvm-c\build\tanda\BoolBits\BoolBits.mod | 876 | 7b87929a | ok |
| /app/ArrayNuevo.mod | bpgenvm-c\build\tanda\ArrayNuevo\ArrayNuevo.mod | 2393 | 2feee85e | ok |
| /app/BucleConNombre.mod | bpgenvm-c\build\tanda\BucleConNombre\BucleConNombre.mod | 1734 | 6c35f20c | ok |
| /app/ErroresConTipo.mod | bpgenvm-c\build\tanda\ErroresConTipo\ErroresConTipo.mod | 11493 | 7d9c7755 | ok (por trozos) |
| /app/IgualdadRefs.mod | bpgenvm-c\build\tanda\IgualdadRefs\IgualdadRefs.mod | 883 | 4a34e551 | ok |
| /app/VarParam.mod | bpgenvm-c\build\tanda\VarParam\VarParam.mod | 3481 | 1024398c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | 4572ae05 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | rp2350a@COM22 | rp2350b@COM4 | stm32u5g9@COM12 | esp32c3@COM3 |
|---|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA | NO-PETA |
| GuiWinJson | SALTADA | SALTADA | IDENTICO | SALTADA |
| GuiShot | SALTADA | SALTADA | NO-PETA | SALTADA |
| BorraCarpetaFichero | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| RutaFueraFlash | DIFIERE | DIFIERE | DIFIERE | DIFIERE |
| FlashCarpetas | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| GcHuecoPila | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| RefCaducadaBuiltin | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| DivBordes | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ListaBorde | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| IndiceObjeto | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| GenericosOrden | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| SobrecargaEspecifica | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| SobrecargaVisible | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| BoolBits | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ArrayNuevo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| BucleConNombre | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ErroresConTipo | DIFIERE | DIFIERE | DIFIERE | DIFIERE |
| IgualdadRefs | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| VarParam | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| StrNulo | IDENTICO | IDENTICO | IDENTICO | IDENTICO |

SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  
SALTADA: GuiWinJson en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32c3@COM3: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-11-15 11:01:41

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\pf\tanda_sql.json` (2 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-14 13:33:40) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-14 13:33:59) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (rp2350b@COM4)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| SqlTexto | SqlTexto | - | - | FALLO | - | - | - |

  - **SqlTexto**: no compila. suelto:
| SqlDemo | SqlDemo | - | - | FALLO | - | - | - |

  - **SqlDemo**: no compila. suelto:

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Nov 13 2026 12:42:26` imageCrc=`c69e835d` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico_m.elf (15-11 10:14)
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=reinicio pedido uptimeMs=441096 fs=352256/10248192 B vmHeapBytes=6275072
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 57 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| SqlTexto | **ERROR** | - | - | no hay .mod: no compila. suelto: mpile SqlTexto.bp (mode=FULL) === Procesando: C:\lenguajes\pm\bpgenvm-c\samples\SqlTexto.bp (5373 caracteres) ===    -- interfaz de 'Core' desde Core.mod (embebida) --   -- no se localiz� .bp ni .bpi para import 'SQLite'; se omitir� --   -- cargado import 'Core' desde cach� en memoria (funcs=78, consts=0, enums=0, props=0, classes=19) --   -- aviso: sin interfaz para import 'SQLite' (ni 'SQLite.bpi' ni su .mod) -- -- Diagn�sticos sem�nticos (3 errores, 0 avisos) -- [69:20] error sem�ntico: tipo cualificado 'SQLite.Db' no encontrado [70:12] error sem�ntico: tipo cualificado 'SQLite.Query' no encontrado [88:26] error sem�ntico: identificador no resuelto: 'SQLite' -- Resumen m�dulo 'SqlTexto' --   classes=19  enums=0  funcs=7  vars=2  consts=0  props=0   initializer=no  Main=s� compilaci�n abortada por errores sem�nticos en SqlTexto.bp  por proyecto:  (mode=FULL) === Procesando: C:\lenguajes\pm\bpgenvm-c\build\tanda\SqlTexto\src\SqlTexto.bp (5373 caracteres) ===    -- interfaz de 'Core' desde Core.mod (embebida) --   -- no se localiz� .bp ni .bpi para import 'SQLite'; se omitir� --   -- cargado import 'Core' desde cach� en memoria (funcs=78, consts=0, enums=0, props=0, classes=19) --   -- aviso: sin interfaz para import 'SQLite' (ni 'SQLite.bpi' ni su .mod) -- -- Diagn�sticos sem�nticos (3 errores, 0 avisos) -- [69:20] error sem�ntico: tipo cualificado 'SQLite.Db' no encontrado [70:12] error sem�ntico: tipo cualificado 'SQLite.Query' no encontrado [88:26] error sem�ntico: identificador no resuelto: 'SQLite' -- Resumen m�dulo 'SqlTexto' --   classes=19  enums=0  funcs=7  vars=2  consts=0  props=0   initializer=no  Main=s� compilaci�n abortada por errores sem�nticos en SqlTexto.bp  |
| SqlDemo | **ERROR** | - | - | no hay .mod: no compila. suelto: import 'SQLite'; se omitir� --   -- interfaz de 'Str' desde Str.mod (embebida) --   -- interfaz de 'Core' desde Core.mod (embebida) --   -- aviso: sin interfaz para import 'SQLite' (ni 'SQLite.bpi' ni su .mod) --   -- cargado import 'Str' desde cach� en memoria (funcs=13, consts=0, enums=0, props=0, classes=0) --   -- cargado import 'Core' desde cach� en memoria (funcs=78, consts=0, enums=0, props=0, classes=19) -- -- Diagn�sticos sem�nticos (3 errores, 0 avisos) -- [50:13] error sem�ntico: tipo cualificado 'SQLite.Db' no encontrado [50:26] error sem�ntico: identificador no resuelto: 'SQLite' [112:12] error sem�ntico: tipo cualificado 'SQLite.Query' no encontrado -- Resumen m�dulo 'SqlDemo' --   classes=19  enums=0  funcs=2  vars=0  consts=0  props=0   initializer=no  Main=s� compilaci�n abortada por errores sem�nticos en SqlDemo.bp  por proyecto: import 'SQLite'; se omitir� --   -- interfaz de 'Str' desde Str.mod (embebida) --   -- interfaz de 'Core' desde Core.mod (embebida) --   -- aviso: sin interfaz para import 'SQLite' (ni 'SQLite.bpi' ni su .mod) --   -- cargado import 'Str' desde cach� en memoria (funcs=13, consts=0, enums=0, props=0, classes=0) --   -- cargado import 'Core' desde cach� en memoria (funcs=78, consts=0, enums=0, props=0, classes=19) -- -- Diagn�sticos sem�nticos (3 errores, 0 avisos) -- [50:13] error sem�ntico: tipo cualificado 'SQLite.Db' no encontrado [50:26] error sem�ntico: identificador no resuelto: 'SQLite' [112:12] error sem�ntico: tipo cualificado 'SQLite.Query' no encontrado -- Resumen m�dulo 'SqlDemo' --   classes=19  enums=0  funcs=2  vars=0  consts=0  props=0   initializer=no  Main=s� compilaci�n abortada por errores sem�nticos en SqlDemo.bp  |

- ejecutadas (con EXITED) 0 de 2 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 2

subido a /app:

(nada)

notas de dependencias:

- limpieza: 0 fichero(s) de /app borrados

### Matriz pruebas x placas

| prueba | rp2350b@COM4 |
|---|---|
| SqlTexto | ERROR |
| SqlDemo | ERROR |



## Tanda 2026-11-15 11:16:32

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\pf\tanda_lote15nov.json` (24 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-14 13:33:40) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-14 13:33:59) - sabor `v2-gui1-lvgl0`
- placas conectadas: 1 (stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 2270c82e | 1611 | ok | 0 | 16 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 9b053410 | 295 | ok | 1 | 9 | 1 |
| ThreadTrasMain | ThreadTrasMain | 97dd1bb5 | 532 | ok | 0 | 321 | 3 |
| MachineId | MachineId | 42a7e155 | 452 | ok | 0 | 12 | 3 |
| GuiWinJson | GuiWinJson | 59428ce2 | 2041 | ok | 0 | 60 | 17 |
| GuiShot | GuiShot | 9c8d42ed | 1347 | ok | 0 | 398 | 8 |
| BorraCarpetaFichero | BorraCarpetaFichero | be28edf2 | 2271 | ok | 0 | 20 | 9 |
| RutaFueraFlash | RutaFueraFlash | 79cbf8d4 | 3213 | ok | 0 | 15 | 39 |
| FlashCarpetas | FlashCarpetas | c21b2696 | 733 | ok | 0 | 24 | 7 |
| GcHuecoPila | GcHuecoPila | 3304c6f0 | 1190 | ok | 0 | 18 | 3 |
| RefCaducadaBuiltin | RefCaducadaBuiltin | 845498b8 | 1688 | ok | 0 | 12 | 6 |
| DivBordes | DivBordes | 70f63a2f | 798 | ok | 0 | 12 | 10 |
| ListaBorde | ListaBorde | e6ddaaf1 | 2918 | ok | 0 | 22 | 26 |
| IndiceObjeto | IndiceObjeto | 80c280cb | 12474 | ok | 0 | 26 | 36 |
| GenericosOrden | GenericosOrden | 52134d69 | 8456 | ok | 0 | 32 | 30 |
| SobrecargaEspecifica | SobrecargaEspecifica | 555c6ff1 | 1358 | ok | 0 | 13 | 9 |
| SobrecargaVisible | SobrecargaVisible | d3797df1 | 1999 | ok | 0 | 12 | 4 |
| BoolBits | BoolBits | 7b87929a | 876 | ok | 0 | 14 | 15 |
| ArrayNuevo | ArrayNuevo | 2feee85e | 2393 | ok | 0 | 27 | 17 |
| BucleConNombre | BucleConNombre | 6c35f20c | 1734 | ok | 0 | 11 | 12 |
| ErroresConTipo | ErroresConTipo | 7d9c7755 | 11493 | ok | 0 | 32 | 48 |
| IgualdadRefs | IgualdadRefs | 4a34e551 | 883 | ok | 0 | 10 | 8 |
| VarParam | VarParam | 1024398c | 3481 | ok | 0 | 14 | 12 |
| StrNulo | StrNulo | 4572ae05 | 2784 | ok | 0 | 11 | 14 |

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Nov 15 2026 10:15:34` imageCrc=`0ea1a041` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.elf (15-11 10:15)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=68292 fs=229376/516096 B vmHeapBytes=507904
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 10 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 196 | 16 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 7 | 9 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 321 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 23 | 12 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 60 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 398 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 134 | 20 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 427 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 88 | 24 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 58 | 18 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 55 | 12 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 65 | 12 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 239 | 22 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 282 | 26 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 237 | 32 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 58 | 13 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 30 | 12 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 87 | 14 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 100 | 27 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 68 | 11 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 657 | 32 | texto distinto |
| IgualdadRefs | **IDENTICO** | 63 | 10 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 99 | 14 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 114 | 11 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=69287 fsUsed=237568 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=69844 fsUsed=237568 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=70416 fsUsed=237568 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=71023 fsUsed=237568 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BorraCarpetaFichero: uptimeMs=71831 fsUsed=237568 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RutaFueraFlash: uptimeMs=73078 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
<details><summary>RutaFueraFlash: cola del stderr del host</summary>

```
throw: ATRAPADA por un catch del programa
[bpvm] throw: "appendFile('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "readFileBytes('/../x810'): no se pudo abrir"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] throw: "writeFileBytes('/../x810'): error de escritura"
[bpvm] throw: ATRAPADA por un catch del programa
[bpvm] pila: el main llegó a 244 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>RutaFueraFlash: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -13,7 +13,7 @@
 rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
 copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
 fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
-lastModified -> IOError: lastModified('/../x810'): no se pudo leer
+lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
 File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
 File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
...
```

oraculo (host):

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no se pudo leer
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no se pudo leer
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```

placa:

```
== /../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/../x810'): error de escritura
appendFile -> IOError: appendFile('/../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/../x810'): error de escritura
listDir -> IOError: listDir('/../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/../x810'): no se pudo crear
rmdir -> IOError: rmdir('/../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/../x810', "w"): no se pudo abrir
== /a/../../x810
fileExists -> false
isDirectory -> false
readFile -> IOError: readFile('/a/../../x810'): no se pudo abrir
writeFile -> IOError: writeFile('/a/../../x810'): error de escritura
appendFile -> IOError: appendFile('/a/../../x810'): error de escritura
readFileBytes -> IOError: readFileBytes('/a/../../x810'): no se pudo abrir
writeFileBytes -> IOError: writeFileBytes('/a/../../x810'): error de escritura
listDir -> IOError: listDir('/a/../../x810'): no se pudo abrir
mkdir -> IOError: mkdir('/a/../../x810'): no se pudo crear
rmdir -> IOError: rmdir('/a/../../x810'): no se pudo borrar (¿no vacío?)
removeFile -> IOError: removeFile('/a/../../x810'): error al borrar
rename(fuera, dentro) -> IOError: rename('/a/../../x810' -> '/data/fuera810b'): error al renombrar
copyFile(fuera, dentro) -> IOError: copyFile('/a/../../x810' -> '/data/fuera810b'): error al copiar
fileSize -> IOError: fileSize('/a/../../x810'): no se pudo leer el tamaño
lastModified -> IOError: lastModified('/a/../../x810'): no implementado en esta plataforma
File(r) -> IOError: File('/a/../../x810', "r"): no se pudo abrir
File(w) -> IOError: File('/a/../../x810', "w"): no se pudo abrir
rename(dentro, fuera) -> IOError: rename('/data/fuera810c' -> '/../x810'): error al renombrar
copyFile(dentro, fuera) -> IOError: copyFile('/data/fuera810c' -> '/../x810'): error al copiar
fin
```
</details>

- tras FlashCarpetas: uptimeMs=73685 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GcHuecoPila: uptimeMs=74342 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RefCaducadaBuiltin: uptimeMs=75031 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras DivBordes: uptimeMs=75640 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ListaBorde: uptimeMs=76549 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras IndiceObjeto: uptimeMs=78738 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GenericosOrden: uptimeMs=80541 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaEspecifica: uptimeMs=81198 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaVisible: uptimeMs=81923 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BoolBits: uptimeMs=82545 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayNuevo: uptimeMs=83303 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BucleConNombre: uptimeMs=84012 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ErroresConTipo: uptimeMs=86522 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
 o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] fin de RUN: la memoria del programa vuelve a su sitio (0 bloques sin liberar; plataforma: 9 vivos)

```
</details>

<details><summary>ErroresConTipo: diff oraculo vs placa</summary>

```diff
--- oraculo (host)
+++ placa
@@ -19,7 +19,7 @@
 ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
 ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
 ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
-ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
+ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
 ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
 ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
...
```

oraculo (host):

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no se pudo leer
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```

placa:

```
ok   división por cero: ArithmeticError: División por cero
ok   módulo por cero: ArithmeticError: Módulo por cero
ok   un long por cero: ArithmeticError: División por cero
ok   exponente negativo: ArithmeticError: exponente negativo en potencia entera
ok   byte(300): ArithmeticError: I32_TO_U8: valor fuera de rango 300
ok   factorial(13): ArithmeticError: factorial: 13 desborda integer (máx 12)
ok   un campo de null: NullReferenceError: referencia nula: acceso a un campo de un objeto null
ok   un método de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   un método privado de null: NullReferenceError: referencia nula: llamada a un método de un objeto null
ok   una cadena null: NullReferenceError: referencia nula: strlen de un string null
ok   un array null: NullReferenceError: referencia nula: acceso a un elemento de un array null
ok   un objeto eliminado: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una clave null: NullReferenceError: referencia nula: Map.put con una clave null
ok   un array: IndexError: ALOAD: índice fuera de rango 5 (length=2)
ok   un byte[]: IndexError: ALOAD_U8: idx fuera de rango 3 (len=2)
ok   charAt: IndexError: charAt: idx fuera de rango 7 (len=3)
ok   una List: IndexError: List: índice fuera de rango 3 (length=1)
ok   una conversión: CastError: conversion invalida: el valor no es un 'Perro'
ok   un fichero que no está: IOError: readFile('no_existe_h75.txt'): no se pudo abrir
ok   UTF-8 malo, por IOError: IOError: fromBytes: UTF-8 mal formado en el byte 1 (0xFF)
ok   fileSize de uno que no está: IOError: fileSize('no_existe_h75.txt'): no se pudo leer el tamaño
ok   lastModified de uno que no está: IOError: lastModified('no_existe_h75.txt'): no implementado en esta plataforma
ok   mkdir sobre un fichero: IOError: mkdir('h805_fichero.txt'): no se pudo crear
ok   rmdir de una que no está vacía: IOError: rmdir('h805_carpeta'): no se pudo borrar (¿no vacío?)
ok   RuntimeError las atrapa: RuntimeError: División por cero
ok   EncodingError, la suya: EncodingError: fromBytes: UTF-8 mal formado en el byte 0 (0xFF)
ok   una cadena eliminada, el uso 1: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 2: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 3: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 4: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 5: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 6: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 7: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 8: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 9: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 10: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 11: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   una cadena eliminada, el uso 12: NullReferenceError: referencia a objeto eliminado (use-after-free)
ok   un Mutex con un id que no vale: RuntimeError: mutex.lock: id inválido 999
ok   y su unlock: RuntimeError: mutex.unlock: id inválido 999
ok   un Mutex null: NullReferenceError: referencia nula: mutex.lock de un Mutex null
ok   y su unlock: NullReferenceError: referencia nula: mutex.unlock de un Mutex null
ok   un throw de un null: NullReferenceError: referencia nula: throw de un objeto null
ok   un throw de un null, con RuntimeError: RuntimeError: referencia nula: throw de un objeto null
ok   un throw de un null, con Exception: Exception: referencia nula: throw de un objeto null
ok   un throw de un null, sin tipo: sin tipo: referencia nula: throw de un objeto null
ok   un throw de un null, con su finally: finally, y atrapada sin nombre
fallos: 0
```
</details>

- tras IgualdadRefs: uptimeMs=87165 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras VarParam: uptimeMs=88039 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras StrNulo: uptimeMs=88831 fsUsed=253952 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- ejecutadas (con EXITED) 22 de 24 listadas; no terminaron 0; saltadas 2; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1611 | 2270c82e | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 295 | 9b053410 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 532 | 97dd1bb5 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 452 | 42a7e155 | ok |
| /app/BorraCarpetaFichero.mod | bpgenvm-c\build\tanda\BorraCarpetaFichero\BorraCarpetaFichero.mod | 2271 | be28edf2 | ok |
| /app/RutaFueraFlash.mod | bpgenvm-c\build\tanda\RutaFueraFlash\RutaFueraFlash.mod | 3213 | 79cbf8d4 | ok |
| /app/FlashCarpetas.mod | bpgenvm-c\build\tanda\FlashCarpetas\FlashCarpetas.mod | 733 | c21b2696 | ok |
| /app/GcHuecoPila.mod | bpgenvm-c\build\tanda\GcHuecoPila\GcHuecoPila.mod | 1190 | 3304c6f0 | ok |
| /app/RefCaducadaBuiltin.mod | bpgenvm-c\build\tanda\RefCaducadaBuiltin\RefCaducadaBuiltin.mod | 1688 | 845498b8 | ok |
| /app/DivBordes.mod | bpgenvm-c\build\tanda\DivBordes\DivBordes.mod | 798 | 70f63a2f | ok |
| /app/ListaBorde.mod | bpgenvm-c\build\tanda\ListaBorde\ListaBorde.mod | 2918 | e6ddaaf1 | ok |
| /app/IndiceObjeto.mod | bpgenvm-c\build\tanda\IndiceObjeto\IndiceObjeto.mod | 12474 | 80c280cb | ok (por trozos) |
| /app/GenericosOrden.mod | bpgenvm-c\build\tanda\GenericosOrden\GenericosOrden.mod | 8456 | 52134d69 | ok (por trozos) |
| /app/SobrecargaEspecifica.mod | bpgenvm-c\build\tanda\SobrecargaEspecifica\SobrecargaEspecifica.mod | 1358 | 555c6ff1 | ok |
| /app/SobrecargaVisible.mod | bpgenvm-c\build\tanda\SobrecargaVisible\SobrecargaVisible.mod | 1999 | d3797df1 | ok |
| /app/BoolBits.mod | bpgenvm-c\build\tanda\BoolBits\BoolBits.mod | 876 | 7b87929a | ok |
| /app/ArrayNuevo.mod | bpgenvm-c\build\tanda\ArrayNuevo\ArrayNuevo.mod | 2393 | 2feee85e | ok |
| /app/BucleConNombre.mod | bpgenvm-c\build\tanda\BucleConNombre\BucleConNombre.mod | 1734 | 6c35f20c | ok |
| /app/ErroresConTipo.mod | bpgenvm-c\build\tanda\ErroresConTipo\ErroresConTipo.mod | 11493 | 7d9c7755 | ok (por trozos) |
| /app/IgualdadRefs.mod | bpgenvm-c\build\tanda\IgualdadRefs\IgualdadRefs.mod | 883 | 4a34e551 | ok |
| /app/VarParam.mod | bpgenvm-c\build\tanda\VarParam\VarParam.mod | 3481 | 1024398c | ok |
| /app/StrNulo.mod | bpgenvm-c\build\tanda\StrNulo\StrNulo.mod | 2784 | 4572ae05 | ok |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia IO: en la zona de packs (pack:Stdlib/IO.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:Stdlib/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:Stdlib/Str.mod, crc igual): se ejecuta XIP, no se sube
- limpieza: 0 fichero(s) de /app borrados y los artefactos (GuiShot.shot)

### Matriz pruebas x placas

| prueba | stm32u575@COM5 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| GuiWinJson | SALTADA |
| GuiShot | SALTADA |
| BorraCarpetaFichero | IDENTICO |
| RutaFueraFlash | DIFIERE |
| FlashCarpetas | IDENTICO |
| GcHuecoPila | IDENTICO |
| RefCaducadaBuiltin | IDENTICO |
| DivBordes | IDENTICO |
| ListaBorde | IDENTICO |
| IndiceObjeto | IDENTICO |
| GenericosOrden | IDENTICO |
| SobrecargaEspecifica | IDENTICO |
| SobrecargaVisible | IDENTICO |
| BoolBits | IDENTICO |
| ArrayNuevo | IDENTICO |
| BucleConNombre | IDENTICO |
| ErroresConTipo | DIFIERE |
| IgualdadRefs | IDENTICO |
| VarParam | IDENTICO |
| StrNulo | IDENTICO |

SALTADA: GuiWinJson en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  

