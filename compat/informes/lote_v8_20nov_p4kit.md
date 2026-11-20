# Tandas T1 — las placas conducidas

## Tanda 2026-11-20 15:59:31

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\pf\s825\lista_lote18nov.json` (24 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-19 16:00:32) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-18 18:42:51) - sabor `v2-gui1-lvgl1`
- placas conectadas: 1 (esp32p4@COM14)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 2270c82e | 1611 | ok | 0 | 13 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 9b053410 | 295 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | 97dd1bb5 | 532 | ok | 0 | 322 | 3 |
| MachineId | MachineId | 42a7e155 | 452 | ok | 0 | 12 | 3 |
| GuiWinJson | GuiWinJson | 59428ce2 | 2041 | ok | 0 | 77 | 17 |
| GuiShot | GuiShot | 9c8d42ed | 1347 | ok | 0 | 388 | 8 |
| BorraCarpetaFichero | BorraCarpetaFichero | be28edf2 | 2271 | ok | 0 | 17 | 9 |
| RutaFueraFlash | RutaFueraFlash | 79cbf8d4 | 3213 | ok | 0 | 14 | 39 |
| FlashCarpetas | FlashCarpetas | c21b2696 | 733 | ok | 0 | 36 | 7 |
| GcHuecoPila | GcHuecoPila | 3304c6f0 | 1190 | ok | 0 | 2 | 3 |
| RefCaducadaBuiltin | RefCaducadaBuiltin | 845498b8 | 1688 | ok | 0 | 17 | 6 |
| DivBordes | DivBordes | 70f63a2f | 798 | ok | 0 | 9 | 10 |
| ListaBorde | ListaBorde | e6ddaaf1 | 2918 | ok | 0 | 24 | 26 |
| IndiceObjeto | IndiceObjeto | 80c280cb | 12474 | ok | 0 | 28 | 36 |
| GenericosOrden | GenericosOrden | 52134d69 | 8456 | ok | 0 | 25 | 30 |
| SobrecargaEspecifica | SobrecargaEspecifica | 555c6ff1 | 1358 | ok | 0 | 11 | 9 |
| SobrecargaVisible | SobrecargaVisible | d3797df1 | 1999 | ok | 0 | 12 | 4 |
| BoolBits | BoolBits | 7b87929a | 876 | ok | 0 | 10 | 15 |
| ArrayNuevo | ArrayNuevo | 2feee85e | 2393 | ok | 0 | 28 | 17 |
| BucleConNombre | BucleConNombre | 6c35f20c | 1734 | ok | 0 | 6 | 12 |
| ErroresConTipo | ErroresConTipo | 7d9c7755 | 11493 | ok | 0 | 31 | 48 |
| IgualdadRefs | IgualdadRefs | 4a34e551 | 883 | ok | 0 | 11 | 8 |
| VarParam | VarParam | 1024398c | 3481 | ok | 0 | 11 | 12 |
| StrNulo | StrNulo | 4572ae05 | 2784 | ok | 0 | 11 | 14 |

### Placa esp32p4 - 6055F9FB058D

- sello: boardName=`esp32p4` uniqueId=`6055F9FB058D` serverName=`bpvm-esp32p4` serverBuild=`Nov 18 2026 18:18:35` imageCrc=`306828a5` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (18-11 19:05)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=software uptimeMs=67189 fs=155648/7168000 B vmHeapBytes=25145344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM14
- pantalla: si (INFO screenW x screenH = 1024x600)
- inventario /lib+/app: 15 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 192 | 13 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 9 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 312 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 19 | 12 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 745 | 65 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 736 | 392 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 246 | 17 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 429 | 14 | texto distinto |
| FlashCarpetas | **IDENTICO** | 196 | 36 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 198 | 2 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 53 | 17 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 61 | 9 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 225 | 24 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 254 | 28 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 214 | 25 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 55 | 11 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 27 | 12 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 84 | 10 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 97 | 28 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 65 | 6 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 641 | 31 | texto distinto |
| IgualdadRefs | **IDENTICO** | 60 | 11 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 95 | 11 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 111 | 11 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=68687 fsUsed=159744 vmHeapBytes=25145344 pila libre: run=13552 io=25748 tareaREPL=25748 heap del sistema, minimo libre=224372
- tras ThrowSinAtrapar: uptimeMs=69360 fsUsed=159744 vmHeapBytes=25145344 pila libre: run=13552 io=25748 tareaREPL=25748 heap del sistema, minimo libre=224372
- tras ThreadTrasMain: uptimeMs=70587 fsUsed=159744 vmHeapBytes=25145344 pila libre: run=12520 io=25748 tareaREPL=25748 heap del sistema, minimo libre=224372
- tras MachineId: uptimeMs=71320 fsUsed=159744 vmHeapBytes=25145344 pila libre: run=13552 io=25748 tareaREPL=25748 heap del sistema, minimo libre=224372
- tras GuiWinJson: uptimeMs=74372 fsUsed=163840 vmHeapBytes=25145344 pila libre: run=13388 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216996
- tras GuiShot: uptimeMs=77124 fsUsed=176128 vmHeapBytes=25145344 pila libre: run=11356 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.shot` (12280 B, crc b46c8ec2) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=79793 fsUsed=176128 vmHeapBytes=25145344 pila libre: run=11356 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras RutaFueraFlash: uptimeMs=81278 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=11356 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
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

- tras FlashCarpetas: uptimeMs=82430 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=11356 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras GcHuecoPila: uptimeMs=83563 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=11356 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras RefCaducadaBuiltin: uptimeMs=84648 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=12212 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras DivBordes: uptimeMs=85349 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=12212 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras ListaBorde: uptimeMs=86765 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=12212 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras IndiceObjeto: uptimeMs=90337 fsUsed=196608 vmHeapBytes=25145344 pila libre: run=11356 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras GenericosOrden: uptimeMs=92995 fsUsed=192512 vmHeapBytes=25145344 pila libre: run=11356 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras SobrecargaEspecifica: uptimeMs=94007 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=11356 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras SobrecargaVisible: uptimeMs=94898 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=11356 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras BoolBits: uptimeMs=95701 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=12212 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras ArrayNuevo: uptimeMs=96965 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=12212 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras BucleConNombre: uptimeMs=97988 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=12212 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras ErroresConTipo: uptimeMs=101180 fsUsed=192512 vmHeapBytes=25145344 pila libre: run=11604 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
its ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=178  gen del handle=0  gen del slot=1  (slots en uso=180)
[bpvm]   gen 0 con el slot en 1: o una referencia CADUCADA al PRIMER objeto de ese slot (nace con gen 0: el programa lo libero y lo siguio usando), o un TRUNCAMIENTO (el handle perdio sus 32 bits ALTOS: un bpref_t de 64b gu
[bpvm] pila: el main llegó a 500 de 16384 B
[bpvm] hilos: 0 arrancados, como mucho 1 a la vez (el main incluido)
[bpvm] mutex: 1 creados, la tabla llegó a 1
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

- tras IgualdadRefs: uptimeMs=101962 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=11224 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras VarParam: uptimeMs=103700 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=11224 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
- tras StrNulo: uptimeMs=104923 fsUsed=184320 vmHeapBytes=25145344 pila libre: run=11604 io=25528 tareaREPL=25528 heap del sistema, minimo libre=216020
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

| prueba | esp32p4@COM14 |
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

NECESITA OJOS: GuiShot en esp32p4@COM14: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_6055F9FB058D\GuiShot.png  

