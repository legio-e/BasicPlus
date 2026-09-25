# Tandas T1 — las placas conducidas

## Tanda 2026-09-25 20:34:34

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-25 11:56:58) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-25 20:09:51) - sabor `gui1-lvgl0`
- placas conectadas: 1 (esp32c6@COM3)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 17 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 318 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 12 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 13 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 16 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 13 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 12 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 24 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 58 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 374 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 13 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 14 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 12 | 40 |

### Placa esp32c6 - 58E6C5FFFEF9

- sello: boardName=`esp32c6` uniqueId=`58E6C5FFFEF9` serverName=`bpvm-esp32c6` serverBuild=`Sep 25 2026 20:23:13` capacidades=["META", "FILES", "TERMINAL"]
- imagen: arch=243 variant=None cpuFreqHz=160000000 resetReason=software uptimeMs=37120 fs=208896/1261568 B vmHeapBytes=65536
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM3
- pantalla: si (INFO screenW x screenH = 240x240)
- inventario /lib+/app: 29 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 0 | 17 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 0 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 310 | 318 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 0 | 12 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 30 | 13 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 30 | 16 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 10 | 13 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 30 | 12 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 20 | 24 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 730 | 54 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 480 | 378 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 0 | 13 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (RV32IMC_ILP32) |
| NatV7 | **IDENTICO** | 10 | 14 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (RV32IMC_ILP32) |
| FloatNat | **IDENTICO** | 10 | 12 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (RV32IMC_ILP32) |

- tras MathRango: uptimeMs=39022 fsUsed=208896 vmHeapBytes=65536
- tras ThrowSinAtrapar: uptimeMs=40056 fsUsed=208896 vmHeapBytes=65536
- tras ThreadTrasMain: uptimeMs=41109 fsUsed=208896 vmHeapBytes=65536
- tras MachineId: uptimeMs=42578 fsUsed=208896 vmHeapBytes=65536
- tras Destructor: uptimeMs=44260 fsUsed=208896 vmHeapBytes=65536
- tras TimeDemo: uptimeMs=45760 fsUsed=208896 vmHeapBytes=65536
- tras ArrayFijo: uptimeMs=46796 fsUsed=208896 vmHeapBytes=65536
- tras MoveRango: uptimeMs=47847 fsUsed=208896 vmHeapBytes=65536
- tras HiloUsa: uptimeMs=49331 fsUsed=208896 vmHeapBytes=65536
- tras GuiWinJson: uptimeMs=53436 fsUsed=208896 vmHeapBytes=65536
- tras GuiShot: uptimeMs=56836 fsUsed=208896 vmHeapBytes=65536
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.shot` (3038 B, crc 7459129a) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=59040 fsUsed=208896 vmHeapBytes=65536
- tras NatV7: uptimeMs=60474 fsUsed=208896 vmHeapBytes=65536
- tras FloatNat: uptimeMs=63061 fsUsed=208896 vmHeapBytes=65536
- ejecutadas (con EXITED) 14 de 14 listadas; no terminaron 0; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/MathRango.mod | bpgenvm-c\build\tanda\MathRango\MathRango.mod | 1616 | 8d65a50d | ok |
| /app/ThrowSinAtrapar.mod | bpgenvm-c\build\tanda\ThrowSinAtrapar\ThrowSinAtrapar.mod | 292 | f8e13c24 | ok |
| /app/ThreadTrasMain.mod | bpgenvm-c\build\tanda\ThreadTrasMain\ThreadTrasMain.mod | 530 | e1f986a3 | ok |
| /app/MachineId.mod | bpgenvm-c\build\tanda\MachineId\MachineId.mod | 484 | 65fa93f7 | ok |
| /app/Destructor.mod | bpgenvm-c\build\tanda\Destructor\Destructor.mod | 4542 | 4c50b965 | ok (por trozos) |
| /app/TimeDemo.mod | bpgenvm-c\build\tanda\TimeDemo\TimeDemo.mod | 1552 | 09b25f3e | ok |
| /app/ArrayFijo.mod | bpgenvm-c\build\tanda\ArrayFijo\ArrayFijo.mod | 2395 | b9a1e239 | ok |
| /app/MoveRango.mod | bpgenvm-c\build\tanda\MoveRango\MoveRango.mod | 3214 | 79dc5aca | ok |
| /app/HiloLib.mod | bpgenvm-c\build\tanda\HiloUsa\HiloLib.mod | 1114 | 88bd91bd | ok |
| /app/HiloUsa.mod | bpgenvm-c\build\tanda\HiloUsa\HiloUsa.mod | 2212 | 026f0359 | ok |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2116 | 093521a9 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1391 | a455f18e | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-RV32IMC_ILP32\NatNew.mod | 4423 | 2d06f373 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-RV32IMC_ILP32\NatV7.mod | 2408 | 9e722734 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-RV32IMC_ILP32\FloatNat.mod | 9733 | 7a8312b9 | ok (por trozos) |

notas de dependencias:

- dependencia Core: en la zona de packs (pack:GuiMini/Core.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Math: en la zona de packs (pack:Stdlib/Math.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Machine: en la zona de packs (pack:Stdlib/Machine.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Time: en la zona de packs (pack:Stdlib/Time.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Rtc: en la zona de packs (pack:Stdlib/Rtc.mod, crc igual): se ejecuta XIP, no se sube
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: en la zona de packs (pack:GuiMini/Gui.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Json: en la zona de packs (pack:GuiMini/Json.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Collections: en la zona de packs (pack:GuiMini/Collections.mod, crc igual): se ejecuta XIP, no se sube
- dependencia Str: en la zona de packs (pack:GuiMini/Str.mod, crc igual): se ejecuta XIP, no se sube

### Matriz pruebas x placas

| prueba | esp32c6@COM3 |
|---|---|
| MathRango | IDENTICO |
| ThrowSinAtrapar | IDENTICO |
| ThreadTrasMain | IDENTICO |
| MachineId | NO-PETA |
| Destructor | IDENTICO |
| TimeDemo | IDENTICO |
| ArrayFijo | IDENTICO |
| MoveRango | IDENTICO |
| HiloUsa | IDENTICO |
| GuiWinJson | IDENTICO |
| GuiShot | NO-PETA |
| NatNew | IDENTICO |
| NatV7 | IDENTICO |
| FloatNat | IDENTICO |

NECESITA OJOS: GuiShot en esp32c6@COM3: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\esp32c6_58E6C5FFFEF9\GuiShot.png  

