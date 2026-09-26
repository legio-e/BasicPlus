# Tandas T1 — las placas conducidas

## Tanda 2026-09-26 17:03:55

- manifiesto: `bpgenvm-c\tools\tanda_v7.json` (14 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-09-26 09:53:37) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-09-26 15:25:04) - sabor `gui1-lvgl0`
- placas conectadas: 1 (stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| MathRango | MathRango | 8d65a50d | 1616 | ok | 0 | 11 | 29 |
| ThrowSinAtrapar | ThrowSinAtrapar | f8e13c24 | 292 | ok | 1 | 12 | 1 |
| ThreadTrasMain | ThreadTrasMain | e1f986a3 | 530 | ok | 0 | 344 | 3 |
| MachineId | MachineId | 65fa93f7 | 484 | ok | 0 | 9 | 3 |
| Destructor | Destructor | 4c50b965 | 4542 | ok | 0 | 10 | 33 |
| TimeDemo | TimeDemo | 09b25f3e | 1552 | ok | 0 | 13 | 13 |
| ArrayFijo | ArrayFijo | b9a1e239 | 2395 | ok | 0 | 8 | 11 |
| MoveRango | MoveRango | 79dc5aca | 3214 | ok | 0 | 10 | 26 |
| HiloUsa | HiloUsa | 026f0359 | 2212 | ok | 0 | 17 | 8 |
| GuiWinJson | GuiWinJson | 093521a9 | 2116 | ok | 0 | 47 | 17 |
| GuiShot | GuiShot | a455f18e | 1391 | ok | 0 | 369 | 8 |
| NatNew | NatNew | 7e353e77 | 2059 | ok | 0 | 11 | 10 |
| NatV7 | NatV7 | cf910565 | 1558 | ok | 0 | 9 | 11 |
| FloatNat | FloatNat | 74b15e1e | 4870 | ok | 0 | 8 | 40 |

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Sep 26 2026 17:02:29` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=pin (NRST) uptimeMs=32465 fs=647168/2064384 B vmHeapBytes=1179648
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 56 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| MathRango | **IDENTICO** | 332 | 11 | EXITED OK exit=0; host rc=0 |
| ThrowSinAtrapar | **IDENTICO** | 128 | 12 | EXITED RUNTIME_ERROR exit=1 (excepcion no atrapada: Core.RuntimeError: ALOAD: índice fuera de rango 99 (length=3)); host rc=1 |
| ThreadTrasMain | **IDENTICO** | 460 | 344 | EXITED OK exit=0; host rc=0 |
| MachineId | **NO-PETA** | 230 | 9 | EXITED OK exit=0 |
| Destructor | **IDENTICO** | 529 | 10 | EXITED OK exit=0; host rc=0 |
| TimeDemo | **IDENTICO** | 825 | 13 | EXITED OK exit=0; host rc=0 |
| ArrayFijo | **IDENTICO** | 315 | 8 | EXITED OK exit=0; host rc=0 |
| MoveRango | **IDENTICO** | 408 | 10 | EXITED OK exit=0; host rc=0 |
| HiloUsa | **IDENTICO** | 546 | 17 | EXITED OK exit=0; host rc=0 |
| GuiWinJson | **IDENTICO** | 5507 | 48 | EXITED OK exit=0; host rc=0 |
| GuiShot | **NO-PETA** | 5737 | 364 | EXITED OK exit=0 |
| NatNew | **IDENTICO** | 240 | 11 | EXITED OK exit=0; host rc=0; nativo 7/7 thunks (ARMV8M_MAIN_SOFTFP) |
| NatV7 | **IDENTICO** | 301 | 9 | EXITED OK exit=0; host rc=0; nativo 4/4 thunks (ARMV8M_MAIN_SOFTFP) |
| FloatNat | **IDENTICO** | 367 | 8 | EXITED OK exit=0; host rc=0; nativo 23/23 thunks (ARMV8M_MAIN_SOFTFP) |

- tras MathRango: uptimeMs=34866 fsUsed=647168 vmHeapBytes=1179648
- tras ThrowSinAtrapar: uptimeMs=35933 fsUsed=647168 vmHeapBytes=1179648
- tras ThreadTrasMain: uptimeMs=37035 fsUsed=647168 vmHeapBytes=1179648
- tras MachineId: uptimeMs=38351 fsUsed=647168 vmHeapBytes=1179648
- tras Destructor: uptimeMs=40386 fsUsed=647168 vmHeapBytes=1179648
- tras TimeDemo: uptimeMs=42388 fsUsed=647168 vmHeapBytes=1179648
- tras ArrayFijo: uptimeMs=43604 fsUsed=647168 vmHeapBytes=1179648
- tras MoveRango: uptimeMs=44923 fsUsed=647168 vmHeapBytes=1179648
- tras HiloUsa: uptimeMs=46625 fsUsed=647168 vmHeapBytes=1179648
- tras GuiWinJson: uptimeMs=70065 fsUsed=745472 vmHeapBytes=1179648
- tras GuiShot: uptimeMs=78635 fsUsed=745472 vmHeapBytes=1179648
- artefacto GuiShot.shot de GuiShot: bajado a `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.shot` (7292 B, crc bd19c7be) -> PNG `C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png`; antes del RUN: existia: /GuiShot.shot (borrado)
- tras NatNew: uptimeMs=82108 fsUsed=745472 vmHeapBytes=1179648
- tras NatV7: uptimeMs=83692 fsUsed=745472 vmHeapBytes=1179648
- tras FloatNat: uptimeMs=86726 fsUsed=745472 vmHeapBytes=1179648
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
| /lib/Gui.mod | bpstdlib\Gui.mod | 45038 | ae0d2a7e | ok (por trozos) |
| /lib/Json.mod | bpstdlib\Json.mod | 22616 | 578601f2 | ok (por trozos) |
| /lib/Collections.mod | bpstdlib\Collections.mod | 12295 | feacb85e | ok (por trozos) |
| /lib/Str.mod | bpstdlib\Str.mod | 4641 | 8e56f753 | ok (por trozos) |
| /app/main.win | samples\formdemo\resources\main.win | 390 | 548edd67 | ok |
| /app/GuiWinJson.mod | bpgenvm-c\build\tanda\GuiWinJson\GuiWinJson.mod | 2116 | 093521a9 | ok |
| /app/GuiShot.mod | bpgenvm-c\build\tanda\GuiShot\GuiShot.mod | 1391 | a455f18e | ok |
| /app/NatNew.mod | bpgenvm-c\build\tanda\NatNew\aot-ARMV8M_MAIN_SOFTFP\NatNew.mod | 4123 | 4b8b9250 | ok (por trozos) |
| /app/NatV7.mod | bpgenvm-c\build\tanda\NatV7\aot-ARMV8M_MAIN_SOFTFP\NatV7.mod | 2296 | 17d5c3d9 | ok |
| /app/FloatNat.mod | bpgenvm-c\build\tanda\FloatNat\aot-ARMV8M_MAIN_SOFTFP\FloatNat.mod | 8449 | 6709344d | ok (por trozos) |

notas de dependencias:

- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Math: ya en /lib/Math.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Machine: ya en /lib/Machine.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Time: ya en /lib/Time.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia Rtc: ya en /lib/Rtc.mod (crc igual; resuelto por nombre, como el RUN)
- dependencia HiloLib: no es de bpstdlib: modulo de la app, va a /app con el programa
- dependencia Gui: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib
- dependencia Json: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib
- dependencia Collections: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib
- dependencia Str: el RUN no la resuelve (NOT_FOUND: no existe): se sube a /lib

### Matriz pruebas x placas

| prueba | stm32u5g9@COM12 |
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

NECESITA OJOS: GuiShot en stm32u5g9@COM12: C:\lenguajes\pm\bpgenvm-c\build\tanda\GuiShot\stm32u5g9_203538505532\GuiShot.png  

