# Tandas T1 — las placas conducidas

## Tanda 2026-11-18 18:35:50

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\pf\s825\lista_lote18nov.json` (24 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-17 19:37:00) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-18 17:54:23) - sabor `v2-gui1-lvgl0`
- placas conectadas: 4 (esp32p4@COM15, stm32u5g9@COM12, rp2350b@COM4, rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 2270c82e | 1611 | ok | 0 | 10 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 9b053410 | 295 | ok | 1 | 11 | 1 |
| ThreadTrasMain | ThreadTrasMain | 97dd1bb5 | 532 | ok | 0 | 323 | 3 |
| MachineId | MachineId | 42a7e155 | 452 | ok | 0 | 9 | 3 |
| GuiWinJson | GuiWinJson | 59428ce2 | 2041 | ok | 0 | 71 | 17 |
| GuiShot | GuiShot | 9c8d42ed | 1347 | ok | 0 | 392 | 8 |
| BorraCarpetaFichero | BorraCarpetaFichero | be28edf2 | 2271 | ok | 0 | 18 | 9 |
| RutaFueraFlash | RutaFueraFlash | 79cbf8d4 | 3213 | ok | 0 | 15 | 39 |
| FlashCarpetas | FlashCarpetas | c21b2696 | 733 | ok | 0 | 36 | 7 |
| GcHuecoPila | GcHuecoPila | 3304c6f0 | 1190 | ok | 0 | 13 | 3 |
| RefCaducadaBuiltin | RefCaducadaBuiltin | 845498b8 | 1688 | ok | 0 | 7 | 6 |
| DivBordes | DivBordes | 70f63a2f | 798 | ok | 0 | 11 | 10 |
| ListaBorde | ListaBorde | e6ddaaf1 | 2918 | ok | 0 | 21 | 26 |
| IndiceObjeto | IndiceObjeto | 80c280cb | 12474 | ok | 0 | 29 | 36 |
| GenericosOrden | GenericosOrden | 52134d69 | 8456 | ok | 0 | 12 | 30 |
| SobrecargaEspecifica | SobrecargaEspecifica | 555c6ff1 | 1358 | ok | 0 | 10 | 9 |
| SobrecargaVisible | SobrecargaVisible | d3797df1 | 1999 | ok | 0 | 9 | 4 |
| BoolBits | BoolBits | 7b87929a | 876 | ok | 0 | 13 | 15 |
| ArrayNuevo | ArrayNuevo | 2feee85e | 2393 | ok | 0 | 21 | 17 |
| BucleConNombre | BucleConNombre | 6c35f20c | 1734 | ok | 0 | 11 | 12 |
| ErroresConTipo | ErroresConTipo | 7d9c7755 | 11493 | ok | 0 | 39 | 48 |
| IgualdadRefs | IgualdadRefs | 4a34e551 | 883 | ok | 0 | 6 | 8 |
| VarParam | VarParam | 1024398c | 3481 | ok | 0 | 15 | 12 |
| StrNulo | StrNulo | 4572ae05 | 2784 | ok | 0 | 0 | 14 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Nov 18 2026 18:18:35` imageCrc=`079dd1c9` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (18-11 18:32)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=177439 fs=606208/16777216 B vmHeapBytes=25145344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 78 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 193 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 11 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 312 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 21 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 743 | 66 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 655 | 376 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 378 | 18 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 414 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 293 | 36 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 217 | 13 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 54 | 7 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 63 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 224 | 21 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 253 | 29 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 216 | 12 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 56 | 10 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 27 | 9 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 87 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 98 | 21 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 66 | 11 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 640 | 39 | texto distinto |
| IgualdadRefs | **IDENTICO** | 61 | 6 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 95 | 15 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 112 | 0 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=179740 fsUsed=610304 vmHeapBytes=25145344 pila libre: run=13552 io=25748 tareaREPL=25748 heap del sistema, minimo libre=154804
- tras ThrowSinAtrapar: uptimeMs=180686 fsUsed=610304 vmHeapBytes=25145344 pila libre: run=14052 io=25748 tareaREPL=25748 heap del sistema, minimo libre=154804
- tras ThreadTrasMain: uptimeMs=182060 fsUsed=610304 vmHeapBytes=25145344 pila libre: run=13472 io=25748 tareaREPL=25748 heap del sistema, minimo libre=154804
- tras MachineId: uptimeMs=183142 fsUsed=610304 vmHeapBytes=25145344 pila libre: run=13472 io=25748 tareaREPL=25748 heap del sistema, minimo libre=154804
- tras GuiWinJson: uptimeMs=187499 fsUsed=614400 vmHeapBytes=25145344 pila libre: run=12440 io=25528 tareaREPL=25528 heap del sistema, minimo libre=146636
- tras GuiShot: uptimeMs=190399 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11020 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=192918 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras RutaFueraFlash: uptimeMs=194988 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11020 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
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

- tras FlashCarpetas: uptimeMs=196272 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras GcHuecoPila: uptimeMs=198007 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11020 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras RefCaducadaBuiltin: uptimeMs=199190 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11020 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras DivBordes: uptimeMs=200092 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11020 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras ListaBorde: uptimeMs=201844 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras IndiceObjeto: uptimeMs=205815 fsUsed=634880 vmHeapBytes=25145344 pila libre: run=11020 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras GenericosOrden: uptimeMs=209067 fsUsed=630784 vmHeapBytes=25145344 pila libre: run=11020 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras SobrecargaEspecifica: uptimeMs=210465 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras SobrecargaVisible: uptimeMs=212185 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras BoolBits: uptimeMs=213105 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras ArrayNuevo: uptimeMs=214297 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=10888 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras BucleConNombre: uptimeMs=215278 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=10888 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras ErroresConTipo: uptimeMs=219035 fsUsed=630784 vmHeapBytes=25145344 pila libre: run=11744 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
its ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
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

- tras IgualdadRefs: uptimeMs=219941 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras VarParam: uptimeMs=221894 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=10888 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
- tras StrNulo: uptimeMs=223444 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=10888 io=25528 tareaREPL=25528 heap del sistema, minimo libre=145404
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

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Nov 18 2026 18:21:15` imageCrc=`f370b38a` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (18-11 18:21)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=186448 fs=155648/1032192 B vmHeapBytes=1032192
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 6 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 197 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 8 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 23 | 9 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 609 | 65 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 680 | 387 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 144 | 18 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 427 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 84 | 36 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 47 | 13 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 55 | 7 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 64 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 242 | 21 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 288 | 29 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 241 | 12 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 59 | 10 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 31 | 9 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 88 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 100 | 21 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 68 | 11 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 665 | 39 | texto distinto |
| IgualdadRefs | **IDENTICO** | 63 | 6 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 100 | 15 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 113 | 0 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=187464 fsUsed=163840 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=188032 fsUsed=163840 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=188632 fsUsed=163840 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=189252 fsUsed=163840 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiWinJson: uptimeMs=191302 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiShot: uptimeMs=193571 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=195265 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RutaFueraFlash: uptimeMs=196523 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
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

- tras FlashCarpetas: uptimeMs=197145 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GcHuecoPila: uptimeMs=197804 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RefCaducadaBuiltin: uptimeMs=198523 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras DivBordes: uptimeMs=199152 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ListaBorde: uptimeMs=200393 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras IndiceObjeto: uptimeMs=202595 fsUsed=188416 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GenericosOrden: uptimeMs=204394 fsUsed=188416 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaEspecifica: uptimeMs=205065 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaVisible: uptimeMs=205782 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BoolBits: uptimeMs=206420 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayNuevo: uptimeMs=207198 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BucleConNombre: uptimeMs=207908 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ErroresConTipo: uptimeMs=210408 fsUsed=188416 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
its ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
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

- tras IgualdadRefs: uptimeMs=211049 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras VarParam: uptimeMs=211914 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras StrNulo: uptimeMs=212752 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
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

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Nov 18 2026 18:18:01` imageCrc=`86bcfa77` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico_m.elf (18-11 18:18)
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=158937 fs=483328/10248192 B vmHeapBytes=6275072
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 60 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 23 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 280 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 317 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 16 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 71 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 392 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 241 | 18 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 58 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 103 | 36 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 47 | 13 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 547 | 7 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 18 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 46 | 21 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 71 | 29 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 60 | 12 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 20 | 10 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 19 | 9 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 20 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 23 | 21 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 17 | 11 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 1932 | 39 | texto distinto |
| IgualdadRefs | **IDENTICO** | 18 | 6 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 22 | 15 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 20 | 0 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=159807 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=13560 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=160746 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=13560 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=161761 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=13560 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=162328 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=13360 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras BorraCarpetaFichero: uptimeMs=162912 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras RutaFueraFlash: uptimeMs=163532 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
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

- tras FlashCarpetas: uptimeMs=164053 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras GcHuecoPila: uptimeMs=164651 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras RefCaducadaBuiltin: uptimeMs=165610 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras DivBordes: uptimeMs=166148 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras ListaBorde: uptimeMs=166829 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras IndiceObjeto: uptimeMs=168051 fsUsed=499712 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras GenericosOrden: uptimeMs=169254 fsUsed=495616 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras SobrecargaEspecifica: uptimeMs=169821 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras SobrecargaVisible: uptimeMs=170454 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras BoolBits: uptimeMs=170990 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras ArrayNuevo: uptimeMs=171537 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras BucleConNombre: uptimeMs=172076 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras ErroresConTipo: uptimeMs=174918 fsUsed=495616 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
its ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
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

- tras IgualdadRefs: uptimeMs=175455 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras VarParam: uptimeMs=176002 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
- tras StrNulo: uptimeMs=176555 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12168 io=9688 tareaREPL=9688 heap del sistema, minimo libre=6632
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

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Nov 18 2026 18:17:59` imageCrc=`cfdea82c` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (18-11 18:18)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=146805 fs=393216/1048576 B vmHeapBytes=295256
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 43 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 25 | 10 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 286 | 11 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 9 | 9 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 71 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 392 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 256 | 18 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 111 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 114 | 36 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 102 | 13 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 589 | 7 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 15 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 75 | 21 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 103 | 29 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 89 | 12 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 17 | 10 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 13 | 9 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 18 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 26 | 21 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 17 | 11 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 2039 | 39 | texto distinto |
| IgualdadRefs | **IDENTICO** | 15 | 6 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 28 | 15 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 27 | 0 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=147758 fsUsed=397312 vmHeapBytes=295256 pila libre: run=13488 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=148768 fsUsed=397312 vmHeapBytes=295256 pila libre: run=14128 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=149718 fsUsed=397312 vmHeapBytes=295256 pila libre: run=13312 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=150286 fsUsed=397312 vmHeapBytes=295256 pila libre: run=13416 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras BorraCarpetaFichero: uptimeMs=151450 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras RutaFueraFlash: uptimeMs=152085 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
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

- tras FlashCarpetas: uptimeMs=152608 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras GcHuecoPila: uptimeMs=153154 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras RefCaducadaBuiltin: uptimeMs=154207 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras DivBordes: uptimeMs=154743 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ListaBorde: uptimeMs=155412 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras IndiceObjeto: uptimeMs=156768 fsUsed=409600 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras GenericosOrden: uptimeMs=157901 fsUsed=405504 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras SobrecargaEspecifica: uptimeMs=158436 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras SobrecargaVisible: uptimeMs=158980 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras BoolBits: uptimeMs=159597 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ArrayNuevo: uptimeMs=160141 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras BucleConNombre: uptimeMs=160676 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ErroresConTipo: uptimeMs=164009 fsUsed=405504 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
<details><summary>ErroresConTipo: cola del stderr del host</summary>

```
its ALTOS: un bpref_t de 64b gu
[bpvm] UAF: idx=258  gen del handle=0  gen del slot=1  (slots en uso=260)
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

- tras IgualdadRefs: uptimeMs=164540 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras VarParam: uptimeMs=165099 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras StrNulo: uptimeMs=165641 fsUsed=397312 vmHeapBytes=295256 pila libre: run=12168 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
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

| prueba | esp32p4@COM15 | stm32u5g9@COM12 | rp2350b@COM4 | rp2350a@COM22 |
|---|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA | NO-PETA |
| GuiWinJson | IDENTICO | IDENTICO | SALTADA | SALTADA |
| GuiShot | NO-PETA | NO-PETA | SALTADA | SALTADA |
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

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  
SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  


## Tanda 2026-11-18 19:10:20

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\pf\s825\lista_lote18nov.json` (24 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-17 19:37:00) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-18 18:42:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 4 (esp32p4@COM15, stm32u5g9@COM12, rp2350b@COM4, rp2350a@COM22)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 2270c82e | 1611 | ok | 0 | 3 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 9b053410 | 295 | ok | 1 | 6 | 1 |
| ThreadTrasMain | ThreadTrasMain | 97dd1bb5 | 532 | ok | 0 | 323 | 3 |
| MachineId | MachineId | 42a7e155 | 452 | ok | 0 | 0 | 3 |
| GuiWinJson | GuiWinJson | 59428ce2 | 2041 | ok | 0 | 78 | 17 |
| GuiShot | GuiShot | 9c8d42ed | 1347 | ok | 0 | 389 | 8 |
| BorraCarpetaFichero | BorraCarpetaFichero | be28edf2 | 2271 | ok | 0 | 16 | 9 |
| RutaFueraFlash | RutaFueraFlash | 79cbf8d4 | 3213 | ok | 0 | 15 | 39 |
| FlashCarpetas | FlashCarpetas | c21b2696 | 733 | ok | 0 | 33 | 7 |
| GcHuecoPila | GcHuecoPila | 3304c6f0 | 1190 | ok | 0 | 12 | 3 |
| RefCaducadaBuiltin | RefCaducadaBuiltin | 845498b8 | 1688 | ok | 0 | 28 | 6 |
| DivBordes | DivBordes | 70f63a2f | 798 | ok | 0 | 11 | 10 |
| ListaBorde | ListaBorde | e6ddaaf1 | 2918 | ok | 0 | 22 | 26 |
| IndiceObjeto | IndiceObjeto | 80c280cb | 12474 | ok | 0 | 29 | 36 |
| GenericosOrden | GenericosOrden | 52134d69 | 8456 | ok | 0 | 8 | 30 |
| SobrecargaEspecifica | SobrecargaEspecifica | 555c6ff1 | 1358 | ok | 0 | 32 | 9 |
| SobrecargaVisible | SobrecargaVisible | d3797df1 | 1999 | ok | 0 | 11 | 4 |
| BoolBits | BoolBits | 7b87929a | 876 | ok | 0 | 21 | 15 |
| ArrayNuevo | ArrayNuevo | 2feee85e | 2393 | ok | 0 | 29 | 17 |
| BucleConNombre | BucleConNombre | 6c35f20c | 1734 | ok | 0 | 9 | 12 |
| ErroresConTipo | ErroresConTipo | 7d9c7755 | 11493 | ok | 0 | 29 | 48 |
| IgualdadRefs | IgualdadRefs | 4a34e551 | 883 | ok | 0 | 10 | 8 |
| VarParam | VarParam | 1024398c | 3481 | ok | 0 | 30 | 12 |
| StrNulo | StrNulo | 4572ae05 | 2784 | ok | 0 | 14 | 14 |

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Nov 18 2026 18:18:35` imageCrc=`306828a5` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (18-11 19:05)
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=138743 fs=606208/16777216 B vmHeapBytes=25145344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 78 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 193 | 3 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 10 | 6 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 21 | 0 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 743 | 67 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 639 | 387 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 391 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 414 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 205 | 33 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 265 | 12 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 54 | 28 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 61 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 223 | 22 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 253 | 29 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 216 | 8 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 55 | 32 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 30 | 11 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 86 | 21 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 97 | 29 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 66 | 9 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 645 | 29 | texto distinto |
| IgualdadRefs | **IDENTICO** | 61 | 10 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 95 | 30 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 111 | 14 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=141132 fsUsed=610304 vmHeapBytes=25145344 pila libre: run=13560 io=25748 tareaREPL=25748 heap del sistema, minimo libre=155268
- tras ThrowSinAtrapar: uptimeMs=142328 fsUsed=610304 vmHeapBytes=25145344 pila libre: run=14044 io=25748 tareaREPL=25748 heap del sistema, minimo libre=155268
- tras ThreadTrasMain: uptimeMs=143806 fsUsed=610304 vmHeapBytes=25145344 pila libre: run=13472 io=25748 tareaREPL=25748 heap del sistema, minimo libre=152664
- tras MachineId: uptimeMs=144806 fsUsed=610304 vmHeapBytes=25145344 pila libre: run=13472 io=25748 tareaREPL=25748 heap del sistema, minimo libre=152664
- tras GuiWinJson: uptimeMs=148639 fsUsed=614400 vmHeapBytes=25145344 pila libre: run=12400 io=25396 tareaREPL=25396 heap del sistema, minimo libre=146100
- tras GuiShot: uptimeMs=151780 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11224 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.shot` (8243 B, crc cde419d3) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=154680 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras RutaFueraFlash: uptimeMs=157001 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
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

- tras FlashCarpetas: uptimeMs=158115 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras GcHuecoPila: uptimeMs=159481 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras RefCaducadaBuiltin: uptimeMs=160921 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras DivBordes: uptimeMs=161876 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras ListaBorde: uptimeMs=163979 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras IndiceObjeto: uptimeMs=167776 fsUsed=634880 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras GenericosOrden: uptimeMs=170858 fsUsed=630784 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras SobrecargaEspecifica: uptimeMs=172120 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras SobrecargaVisible: uptimeMs=173532 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras BoolBits: uptimeMs=174615 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras ArrayNuevo: uptimeMs=176288 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11224 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras BucleConNombre: uptimeMs=177292 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras ErroresConTipo: uptimeMs=180955 fsUsed=630784 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
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

- tras IgualdadRefs: uptimeMs=182033 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras VarParam: uptimeMs=184488 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
- tras StrNulo: uptimeMs=186117 fsUsed=622592 vmHeapBytes=25145344 pila libre: run=11744 io=25396 tareaREPL=25396 heap del sistema, minimo libre=145980
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

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Nov 18 2026 19:07:22` imageCrc=`b86d1b6f` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (18-11 19:07)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=180842 fs=155648/1032192 B vmHeapBytes=1032192
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 6 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 197 | 3 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 7 | 6 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 23 | 0 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 612 | 63 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 680 | 391 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 156 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 428 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 92 | 33 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 63 | 12 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 56 | 28 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 65 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 243 | 22 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 288 | 29 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 242 | 8 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 59 | 32 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 30 | 11 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 88 | 21 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 100 | 29 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 68 | 9 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 663 | 29 | texto distinto |
| IgualdadRefs | **IDENTICO** | 63 | 10 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 100 | 30 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 114 | 14 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=181862 fsUsed=163840 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=182427 fsUsed=163840 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=183026 fsUsed=163840 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=183674 fsUsed=163840 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiWinJson: uptimeMs=185880 fsUsed=172032 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GuiShot: uptimeMs=188144 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (8718 B, crc d9978987) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=189862 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RutaFueraFlash: uptimeMs=191115 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
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

- tras FlashCarpetas: uptimeMs=191728 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GcHuecoPila: uptimeMs=192391 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RefCaducadaBuiltin: uptimeMs=193108 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras DivBordes: uptimeMs=193718 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ListaBorde: uptimeMs=194527 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras IndiceObjeto: uptimeMs=196708 fsUsed=188416 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GenericosOrden: uptimeMs=198495 fsUsed=188416 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaEspecifica: uptimeMs=199177 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaVisible: uptimeMs=199910 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BoolBits: uptimeMs=200529 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayNuevo: uptimeMs=201302 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BucleConNombre: uptimeMs=202011 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ErroresConTipo: uptimeMs=204498 fsUsed=188416 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
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

- tras IgualdadRefs: uptimeMs=205145 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras VarParam: uptimeMs=206010 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras StrNulo: uptimeMs=206813 fsUsed=180224 vmHeapBytes=1032192 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
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

### Placa rp2350b - 4CE219E14C8F8E1C

- sello: boardName=`rp2350b` uniqueId=`4CE219E14C8F8E1C` serverName=`bpvm-pico` serverBuild=`Nov 18 2026 18:18:01` imageCrc=`d1728d89` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico_m.elf (18-11 19:04)
- imagen: arch=40 variant=B cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=187658 fs=483328/10248192 B vmHeapBytes=6275072
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM4
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 60 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 22 | 3 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 260 | 6 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 317 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 15 | 0 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 78 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 389 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 198 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 58 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 97 | 33 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 47 | 12 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 389 | 28 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 17 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 46 | 22 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 71 | 29 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 60 | 8 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 20 | 32 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 18 | 11 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 19 | 21 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 24 | 29 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 18 | 9 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 162 | 29 | texto distinto |
| IgualdadRefs | **IDENTICO** | 18 | 10 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 22 | 30 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 21 | 14 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=188518 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=13436 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=189049 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=14128 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=190063 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=14128 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=190621 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=13300 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras BorraCarpetaFichero: uptimeMs=191227 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12152 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras RutaFueraFlash: uptimeMs=191846 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12152 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
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

- tras FlashCarpetas: uptimeMs=192446 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12100 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras GcHuecoPila: uptimeMs=192978 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12152 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras RefCaducadaBuiltin: uptimeMs=193923 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12100 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras DivBordes: uptimeMs=194459 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12152 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ListaBorde: uptimeMs=195130 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12100 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras IndiceObjeto: uptimeMs=196419 fsUsed=499712 vmHeapBytes=6275072 pila libre: run=12152 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras GenericosOrden: uptimeMs=197540 fsUsed=495616 vmHeapBytes=6275072 pila libre: run=12100 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras SobrecargaEspecifica: uptimeMs=198081 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12152 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras SobrecargaVisible: uptimeMs=198694 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12100 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras BoolBits: uptimeMs=199233 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12100 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ArrayNuevo: uptimeMs=199766 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12152 io=9632 tareaREPL=9632 heap del sistema, minimo libre=6632
- tras BucleConNombre: uptimeMs=200365 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12100 io=9632 tareaREPL=9632 heap del sistema, minimo libre=6632
- tras ErroresConTipo: uptimeMs=201453 fsUsed=495616 vmHeapBytes=6275072 pila libre: run=12152 io=9632 tareaREPL=9632 heap del sistema, minimo libre=6632
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

- tras IgualdadRefs: uptimeMs=201993 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12100 io=9632 tareaREPL=9632 heap del sistema, minimo libre=6632
- tras VarParam: uptimeMs=202965 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12152 io=9632 tareaREPL=9632 heap del sistema, minimo libre=6632
- tras StrNulo: uptimeMs=203520 fsUsed=487424 vmHeapBytes=6275072 pila libre: run=12100 io=9632 tareaREPL=9632 heap del sistema, minimo libre=6632
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

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Nov 18 2026 18:17:59` imageCrc=`714ae80e` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: la COMPILADA en el PC: bpgenvm-c/pico/build/bpvm_pico.elf (18-11 19:04)
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=177148 fs=393216/1048576 B vmHeapBytes=295248
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 43 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 25 | 3 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 289 | 6 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 323 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 9 | 0 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 78 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 389 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 211 | 16 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 111 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 110 | 33 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 105 | 12 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 444 | 28 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 14 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 76 | 22 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 102 | 29 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 89 | 8 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 18 | 32 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 13 | 11 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 19 | 21 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 27 | 29 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 17 | 9 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 235 | 29 | texto distinto |
| IgualdadRefs | **IDENTICO** | 14 | 10 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 28 | 30 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 28 | 14 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=178186 fsUsed=397312 vmHeapBytes=295248 pila libre: run=13396 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ThrowSinAtrapar: uptimeMs=179129 fsUsed=397312 vmHeapBytes=295248 pila libre: run=14128 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ThreadTrasMain: uptimeMs=180060 fsUsed=397312 vmHeapBytes=295248 pila libre: run=14128 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras MachineId: uptimeMs=180626 fsUsed=397312 vmHeapBytes=295248 pila libre: run=13268 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras BorraCarpetaFichero: uptimeMs=181314 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras RutaFueraFlash: uptimeMs=181937 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
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

- tras FlashCarpetas: uptimeMs=182476 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12172 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras GcHuecoPila: uptimeMs=183158 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras RefCaducadaBuiltin: uptimeMs=184113 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12172 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras DivBordes: uptimeMs=184647 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ListaBorde: uptimeMs=185745 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12172 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras IndiceObjeto: uptimeMs=187031 fsUsed=409600 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras GenericosOrden: uptimeMs=188145 fsUsed=405504 vmHeapBytes=295248 pila libre: run=12172 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras SobrecargaEspecifica: uptimeMs=188679 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras SobrecargaVisible: uptimeMs=189361 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12172 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras BoolBits: uptimeMs=189901 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ArrayNuevo: uptimeMs=190447 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12172 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras BucleConNombre: uptimeMs=190982 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras ErroresConTipo: uptimeMs=192555 fsUsed=405504 vmHeapBytes=295248 pila libre: run=12172 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
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

- tras IgualdadRefs: uptimeMs=193093 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras VarParam: uptimeMs=193642 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
- tras StrNulo: uptimeMs=194337 fsUsed=397312 vmHeapBytes=295248 pila libre: run=12120 io=9696 tareaREPL=9696 heap del sistema, minimo libre=6632
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

| prueba | esp32p4@COM15 | stm32u5g9@COM12 | rp2350b@COM4 | rp2350a@COM22 |
|---|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA | NO-PETA |
| GuiWinJson | IDENTICO | IDENTICO | SALTADA | SALTADA |
| GuiShot | NO-PETA | NO-PETA | SALTADA | SALTADA |
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

NECESITA OJOS: GuiShot en esp32p4@COM15: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32p4_E8F60AE0B380\GuiShot.png  
NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  
SALTADA: GuiWinJson en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350b@COM4: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiWinJson en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en rp2350a@COM22: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  

