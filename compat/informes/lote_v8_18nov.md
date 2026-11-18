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


## Tanda 2026-11-18 19:23:17

- manifiesto: `..\..\Users\Eduardo\AppData\Local\Temp\claude\d1cffc43b1cdb88d\576fc834-e260-4649-ba6c-131c549812a9\scratchpad\pf\s825\lista_lote18nov.json` (24 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-11-17 19:37:00) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-11-18 18:42:51) - sabor `v2-gui1-lvgl0`
- placas conectadas: 4 (esp32s3@COM9, esp32c5@COM6, esp32c6@COM3, stm32u575@COM5)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 2270c82e | 1611 | ok | 0 | 28 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | 9b053410 | 295 | ok | 1 | 15 | 1 |
| ThreadTrasMain | ThreadTrasMain | 97dd1bb5 | 532 | ok | 0 | 322 | 3 |
| MachineId | MachineId | 42a7e155 | 452 | ok | 0 | 16 | 3 |
| GuiWinJson | GuiWinJson | 59428ce2 | 2041 | ok | 0 | 61 | 17 |
| GuiShot | GuiShot | 9c8d42ed | 1347 | ok | 0 | 382 | 8 |
| BorraCarpetaFichero | BorraCarpetaFichero | be28edf2 | 2271 | ok | 0 | 8 | 9 |
| RutaFueraFlash | RutaFueraFlash | 79cbf8d4 | 3213 | ok | 0 | 15 | 39 |
| FlashCarpetas | FlashCarpetas | c21b2696 | 733 | ok | 0 | 31 | 7 |
| GcHuecoPila | GcHuecoPila | 3304c6f0 | 1190 | ok | 0 | 7 | 3 |
| RefCaducadaBuiltin | RefCaducadaBuiltin | 845498b8 | 1688 | ok | 0 | 13 | 6 |
| DivBordes | DivBordes | 70f63a2f | 798 | ok | 0 | 11 | 10 |
| ListaBorde | ListaBorde | e6ddaaf1 | 2918 | ok | 0 | 28 | 26 |
| IndiceObjeto | IndiceObjeto | 80c280cb | 12474 | ok | 0 | 15 | 36 |
| GenericosOrden | GenericosOrden | 52134d69 | 8456 | ok | 0 | 16 | 30 |
| SobrecargaEspecifica | SobrecargaEspecifica | 555c6ff1 | 1358 | ok | 0 | 12 | 9 |
| SobrecargaVisible | SobrecargaVisible | d3797df1 | 1999 | ok | 0 | 7 | 4 |
| BoolBits | BoolBits | 7b87929a | 876 | ok | 0 | 6 | 15 |
| ArrayNuevo | ArrayNuevo | 2feee85e | 2393 | ok | 0 | 30 | 17 |
| BucleConNombre | BucleConNombre | 6c35f20c | 1734 | ok | 0 | 11 | 12 |
| ErroresConTipo | ErroresConTipo | 7d9c7755 | 11493 | ok | 0 | 23 | 48 |
| IgualdadRefs | IgualdadRefs | 4a34e551 | 883 | ok | 0 | 19 | 8 |
| VarParam | VarParam | 1024398c | 3481 | ok | 0 | 15 | 12 |
| StrNulo | StrNulo | 4572ae05 | 2784 | ok | 0 | 20 | 14 |

### Placa esp32s3 - 98A316E6EE90

- sello: boardName=`esp32s3` uniqueId=`98A316E6EE90` serverName=`bpvm-esp32` serverBuild=`Nov 18 2026 18:19:11` imageCrc=`636863b9` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32/build/bpvm_esp32.bin (18-11 19:20)
- imagen: arch=94 variant=None cpuFreqHz=160000000 resetReason=power-on uptimeMs=148230 fs=143360/9199616 B vmHeapBytes=7844864
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM9
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 15 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 190 | 28 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 11 | 15 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 19 | 16 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 61 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 382 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 304 | 8 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 415 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 170 | 31 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 91 | 7 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 50 | 13 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 62 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 230 | 28 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 270 | 15 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 226 | 16 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 56 | 12 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 26 | 7 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 84 | 6 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 96 | 30 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 64 | 11 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 659 | 23 | texto distinto |
| IgualdadRefs | **IDENTICO** | 59 | 19 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 99 | 15 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 108 | 20 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=149736 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=13316 io=9376 tareaREPL=9376 heap del sistema, minimo libre=58008
- tras ThrowSinAtrapar: uptimeMs=150319 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=13852 io=9312 tareaREPL=9312 heap del sistema, minimo libre=58008
- tras ThreadTrasMain: uptimeMs=151284 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=13180 io=9312 tareaREPL=9312 heap del sistema, minimo libre=57860
- tras MachineId: uptimeMs=152039 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=13180 io=9280 tareaREPL=9280 heap del sistema, minimo libre=57684
- tras BorraCarpetaFichero: uptimeMs=153362 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=56752
- tras RutaFueraFlash: uptimeMs=155127 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=55824
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

- tras FlashCarpetas: uptimeMs=155946 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11444 io=9280 tareaREPL=9280 heap del sistema, minimo libre=55824
- tras GcHuecoPila: uptimeMs=156827 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=54760
- tras RefCaducadaBuiltin: uptimeMs=157898 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11444 io=9280 tareaREPL=9280 heap del sistema, minimo libre=54760
- tras DivBordes: uptimeMs=158772 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=54760
- tras ListaBorde: uptimeMs=160901 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11444 io=9280 tareaREPL=9280 heap del sistema, minimo libre=53040
- tras IndiceObjeto: uptimeMs=164859 fsUsed=159744 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=52412
- tras GenericosOrden: uptimeMs=168053 fsUsed=155648 vmHeapBytes=7844864 pila libre: run=11444 io=9280 tareaREPL=9280 heap del sistema, minimo libre=52412
- tras SobrecargaEspecifica: uptimeMs=169103 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=52412
- tras SobrecargaVisible: uptimeMs=170280 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=52412
- tras BoolBits: uptimeMs=171342 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11444 io=9280 tareaREPL=9280 heap del sistema, minimo libre=52412
- tras ArrayNuevo: uptimeMs=172489 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=52412
- tras BucleConNombre: uptimeMs=173365 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11444 io=9280 tareaREPL=9280 heap del sistema, minimo libre=52412
- tras ErroresConTipo: uptimeMs=177178 fsUsed=155648 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=51684
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

- tras IgualdadRefs: uptimeMs=177913 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11444 io=9280 tareaREPL=9280 heap del sistema, minimo libre=51684
- tras VarParam: uptimeMs=179289 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11452 io=9280 tareaREPL=9280 heap del sistema, minimo libre=51684
- tras StrNulo: uptimeMs=180288 fsUsed=147456 vmHeapBytes=7844864 pila libre: run=11444 io=9280 tareaREPL=9280 heap del sistema, minimo libre=51684
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

### Placa esp32c5 - 10BDA3FEFFDF

- sello: boardName=`esp32c5` uniqueId=`10BDA3FEFFDF` serverName=`bpvm-esp32c5` serverBuild=`Nov 18 2026 18:20:18` imageCrc=`d9559635` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c5/build/bpvm_esp32c5.bin (18-11 19:21)
- imagen: arch=243 variant=None cpuFreqHz=240000000 resetReason=USB uptimeMs=137541 fs=208896/16777216 B vmHeapBytes=7320576
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM6
- pantalla: si (INFO screenW x screenH = 240x320)
- inventario /lib+/app: 32 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 14 | 28 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 14 | 15 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 322 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 14 | 16 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 934 | 76 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 622 | 392 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 197 | 8 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 88 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 121 | 31 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 95 | 7 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 19 | 13 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 15 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 51 | 28 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 72 | 15 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 61 | 16 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 16 | 12 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 15 | 7 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 15 | 6 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 47 | 30 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 15 | 11 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 238 | 23 | texto distinto |
| IgualdadRefs | **IDENTICO** | 15 | 19 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 21 | 15 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 20 | 20 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=138493 fsUsed=212992 vmHeapBytes=7320576 pila libre: run=13764 io=9864 tareaREPL=9864 heap del sistema, minimo libre=45348
- tras ThrowSinAtrapar: uptimeMs=139271 fsUsed=212992 vmHeapBytes=7320576 pila libre: run=13764 io=9864 tareaREPL=9864 heap del sistema, minimo libre=45348
- tras ThreadTrasMain: uptimeMs=140174 fsUsed=212992 vmHeapBytes=7320576 pila libre: run=13576 io=9864 tareaREPL=9864 heap del sistema, minimo libre=45180
- tras MachineId: uptimeMs=140721 fsUsed=212992 vmHeapBytes=7320576 pila libre: run=13576 io=9864 tareaREPL=9864 heap del sistema, minimo libre=45024
- tras GuiWinJson: uptimeMs=143467 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=12400 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36384
- tras GuiShot: uptimeMs=145740 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.shot` (3350 B, crc f0589b1a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=146453 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras RutaFueraFlash: uptimeMs=147047 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
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

- tras FlashCarpetas: uptimeMs=147761 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras GcHuecoPila: uptimeMs=148280 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras RefCaducadaBuiltin: uptimeMs=148810 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras DivBordes: uptimeMs=149362 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras ListaBorde: uptimeMs=150522 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras IndiceObjeto: uptimeMs=152411 fsUsed=229376 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras GenericosOrden: uptimeMs=153893 fsUsed=225280 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras SobrecargaEspecifica: uptimeMs=154460 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras SobrecargaVisible: uptimeMs=155189 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras BoolBits: uptimeMs=155702 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras ArrayNuevo: uptimeMs=156238 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras BucleConNombre: uptimeMs=156783 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras ErroresConTipo: uptimeMs=158401 fsUsed=225280 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
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

- tras IgualdadRefs: uptimeMs=158905 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras VarParam: uptimeMs=159423 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
- tras StrNulo: uptimeMs=159970 fsUsed=217088 vmHeapBytes=7320576 pila libre: run=11428 io=9668 tareaREPL=9668 heap del sistema, minimo libre=36252
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

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Nov 18 2026 18:20:50` imageCrc=`9c7087d1` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/esp32c6/build/bpvm_esp32c6.bin (18-11 19:22)
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=USB uptimeMs=129769 fs=172032/1261568 B vmHeapBytes=114688
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 5 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 6 | 28 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 4 | 15 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 313 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 5 | 16 | EXITED OK exit=0 |
| GuiWinJson | **IDENTICO** | 1029 | 60 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 575 | 406 | EXITED OK exit=0 |
| BorraCarpetaFichero | **IDENTICO** | 240 | 8 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 71 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 161 | 31 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 113 | 7 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 5 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 38 | 28 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 64 | 15 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 52 | 16 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 8 | 12 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 6 | 7 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 6 | 6 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 10 | 30 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 6 | 11 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 170 | 23 | texto distinto |
| IgualdadRefs | **IDENTICO** | 6 | 19 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 12 | 15 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 10 | 20 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=130564 fsUsed=176128 vmHeapBytes=114688 pila libre: run=7496 io=5776 tareaREPL=5776 heap del sistema, minimo libre=71944
- tras ThrowSinAtrapar: uptimeMs=131085 fsUsed=176128 vmHeapBytes=114688 pila libre: run=7496 io=5776 tareaREPL=5776 heap del sistema, minimo libre=71944
- tras ThreadTrasMain: uptimeMs=132013 fsUsed=176128 vmHeapBytes=114688 pila libre: run=7432 io=5776 tareaREPL=5776 heap del sistema, minimo libre=71776
- tras MachineId: uptimeMs=132645 fsUsed=176128 vmHeapBytes=114688 pila libre: run=7432 io=5776 tareaREPL=5776 heap del sistema, minimo libre=71624
- tras GuiWinJson: uptimeMs=135551 fsUsed=180224 vmHeapBytes=114688 pila libre: run=6064 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras GuiShot: uptimeMs=137635 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: no existia
- tras BorraCarpetaFichero: uptimeMs=138778 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras RutaFueraFlash: uptimeMs=139394 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
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

- tras FlashCarpetas: uptimeMs=139967 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras GcHuecoPila: uptimeMs=140475 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras RefCaducadaBuiltin: uptimeMs=140986 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras DivBordes: uptimeMs=141493 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras ListaBorde: uptimeMs=142429 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras IndiceObjeto: uptimeMs=143574 fsUsed=192512 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras GenericosOrden: uptimeMs=144573 fsUsed=188416 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras SobrecargaEspecifica: uptimeMs=145063 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras SobrecargaVisible: uptimeMs=145576 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras BoolBits: uptimeMs=146066 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras ArrayNuevo: uptimeMs=146577 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras BucleConNombre: uptimeMs=147131 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras ErroresConTipo: uptimeMs=148482 fsUsed=188416 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
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

- tras IgualdadRefs: uptimeMs=148993 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras VarParam: uptimeMs=149934 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
- tras StrNulo: uptimeMs=150470 fsUsed=180224 vmHeapBytes=114688 pila libre: run=5284 io=5440 tareaREPL=5440 heap del sistema, minimo libre=30092
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

### Placa stm32u575 - 203436414230500D00350041

- sello: boardName=`stm32u575` uniqueId=`203436414230500D00350041` serverName=`bpvm-stm32` serverBuild=`Nov 18 2026 19:07:35` imageCrc=`32766b07` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: la COMPILADA en el PC: bpgenvm-c/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.elf (18-11 19:07)
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=108440 fs=253952/516096 B vmHeapBytes=507904
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM5
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 11 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 196 | 28 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 7 | 15 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.IndexError: ALOAD: índice fuera de rango 99 (length=3)) en ThrowSinAtrapar.bp:26; host rc=1 |
| ThreadTrasMain | **IDENTICO** | 311 | 322 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 24 | 16 | EXITED OK exit=0 |
| GuiWinJson | **SALTADA** | - | 61 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| GuiShot | **SALTADA** | - | 382 | necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL) |
| BorraCarpetaFichero | **IDENTICO** | 134 | 8 | EXITED OK exit=0; host rc=0 |
| RutaFueraFlash | **DIFIERE** | 427 | 15 | texto distinto |
| FlashCarpetas | **IDENTICO** | 76 | 31 | EXITED OK exit=0; host rc=0 |
| GcHuecoPila | **IDENTICO** | 43 | 7 | EXITED OK exit=0; host rc=0 |
| RefCaducadaBuiltin | **IDENTICO** | 55 | 13 | EXITED OK exit=0; host rc=0 |
| DivBordes | **IDENTICO** | 64 | 11 | EXITED OK exit=0; host rc=0 |
| ListaBorde | **IDENTICO** | 240 | 28 | EXITED OK exit=0; host rc=0 |
| IndiceObjeto | **IDENTICO** | 283 | 15 | EXITED OK exit=0; host rc=0 |
| GenericosOrden | **IDENTICO** | 237 | 16 | EXITED OK exit=0; host rc=0 |
| SobrecargaEspecifica | **IDENTICO** | 59 | 12 | EXITED OK exit=0; host rc=0 |
| SobrecargaVisible | **IDENTICO** | 30 | 7 | EXITED OK exit=0; host rc=0 |
| BoolBits | **IDENTICO** | 87 | 6 | EXITED OK exit=0; host rc=0 |
| ArrayNuevo | **IDENTICO** | 100 | 30 | EXITED OK exit=0; host rc=0 |
| BucleConNombre | **IDENTICO** | 67 | 11 | EXITED OK exit=0; host rc=0 |
| ErroresConTipo | **DIFIERE** | 660 | 23 | texto distinto |
| IgualdadRefs | **IDENTICO** | 63 | 19 | EXITED OK exit=0; host rc=0 |
| VarParam | **IDENTICO** | 99 | 15 | EXITED OK exit=0; host rc=0 |
| StrNulo | **IDENTICO** | 113 | 20 | EXITED OK exit=0; host rc=0 |

- tras MathRango: uptimeMs=109431 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThrowSinAtrapar: uptimeMs=109995 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ThreadTrasMain: uptimeMs=110591 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras MachineId: uptimeMs=111214 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BorraCarpetaFichero: uptimeMs=112025 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RutaFueraFlash: uptimeMs=113296 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
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

- tras FlashCarpetas: uptimeMs=113919 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GcHuecoPila: uptimeMs=114597 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras RefCaducadaBuiltin: uptimeMs=115302 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras DivBordes: uptimeMs=115927 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ListaBorde: uptimeMs=117292 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras IndiceObjeto: uptimeMs=119488 fsUsed=270336 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras GenericosOrden: uptimeMs=121278 fsUsed=270336 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaEspecifica: uptimeMs=121959 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras SobrecargaVisible: uptimeMs=122711 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BoolBits: uptimeMs=123341 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ArrayNuevo: uptimeMs=124111 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras BucleConNombre: uptimeMs=124827 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras ErroresConTipo: uptimeMs=127335 fsUsed=270336 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
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

- tras IgualdadRefs: uptimeMs=127955 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras VarParam: uptimeMs=128829 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
- tras StrNulo: uptimeMs=129650 fsUsed=262144 vmHeapBytes=507904 pila libre: run=-1 io=3248 tareaREPL=None heap del sistema, minimo libre=None
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

| prueba | esp32s3@COM9 | esp32c5@COM6 | esp32c6@COM3 | stm32u575@COM5 |
|---|---|---|---|---|
| MathRango | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThrowSinAtrapar | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| ThreadTrasMain | IDENTICO | IDENTICO | IDENTICO | IDENTICO |
| MachineId | NO-PETA | NO-PETA | NO-PETA | NO-PETA |
| GuiWinJson | SALTADA | IDENTICO | IDENTICO | SALTADA |
| GuiShot | SALTADA | NO-PETA | NO-PETA | SALTADA |
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

SALTADA: GuiWinJson en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en esp32s3@COM9: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
NECESITA OJOS: GuiShot en esp32c5@COM6: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c5_10BDA3FEFFDF\GuiShot.png  
NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  
SALTADA: GuiWinJson en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  
SALTADA: GuiShot en stm32u575@COM5: necesita pantalla y la placa no la tiene (INFO sin screenW/screenH: la imagen no lleva LVGL)  

