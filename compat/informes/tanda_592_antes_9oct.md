# Tandas T1 — las placas conducidas

## Tanda 2026-10-09 08:48:11

- manifiesto: `bpgenvm-c\tools\tanda_592.json` (1 pruebas listadas)
- oraculo: jar `lexer-java\target\basicplus-frontend.jar` (2026-10-09 08:24:01) - exe `bpgenvm-c\build\bpgenvm-c.exe` (2026-10-09 08:13:46) - sabor `v2-gui1-lvgl0`
- placas conectadas: 3 (rp2350a@COM22, esp32p4@COM15, stm32u5g9@COM12)

### Oraculo por prueba (PC)

| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |
|---|---|---|---|---|---|---|---|
| NatPesadaPlaca | NatPesadaPlaca | b4115764 | 449 | ok | - | - | - |

### Placa rp2350a - 46CED5A69C399BD2

- sello: boardName=`rp2350a` uniqueId=`46CED5A69C399BD2` serverName=`bpvm-pico` serverBuild=`Oct  8 2026 08:51:35` imageCrc=`24a0ed2e` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS", "BOOTSEL"]
- imagen: OTRA: la compilada en el PC (bpgenvm-c/pico/build/bpvm_pico.elf (09-10 08:19)) da 219d7796
- imagen: arch=40 variant=A cpuFreqHz=150000000 resetReason=power-on/run uptimeMs=11523478 fs=237568/1048576 B vmHeapBytes=219344
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM22
- pantalla: no (INFO sin screenW/screenH: la imagen no lleva LVGL)
- inventario /lib+/app: 30 ficheros en /lib+/app, omitidos=0
- **ERROR de placa**: colgada en NatPesadaPlaca y no se recupera: no contesta al RESET por el wire, y rp2350a no se reinicia sin manos: desenchufa y enchufa la placa

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatPesadaPlaca | **COLGADA** | - | - | sin EXITED en 30 s; tras KILL: tampoco EXITED en 5 s; recuperacion: no contesta al RESET por el wire, y rp2350a no se reinicia sin manos: desenchufa y enchufa la placa |

<details><summary>NatPesadaPlaca: salida recibida de la placa (sin EXITED)</summary>

```
ok pesada: entra en la native
```
</details>

- ejecutadas (con EXITED) 0 de 1 listadas; no terminaron 1; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatPesadaPlaca.mod | bpgenvm-c\build\tanda\NatPesadaPlaca\aot-ARMV8M_MAIN_SOFTFP\NatPesadaPlaca.mod | 630 | 44d28f70 | ok |

notas de dependencias:

- la placa NO lleva la imagen compilada en el PC: imageCrc 24a0ed2e, bpgenvm-c/pico/build/bpvm_pico.elf (09-10 08:19) da 219d7796
- dependencia Core: ya en /lib/Core.mod (crc igual; resuelto por nombre, como el RUN)
- NatPesadaPlaca: COLGADA; no contesta al RESET por el wire, y rp2350a no se reinicia sin manos: desenchufa y enchufa la placa

### Placa esp32p4 - E8F60AE0B380

- sello: boardName=`esp32p4` uniqueId=`E8F60AE0B380` serverName=`bpvm-esp32p4` serverBuild=`Oct  8 2026 14:58:55` imageCrc=`4d69b438` capacidades=["META", "FILES", "TERMINAL", "DEBUG", "PACKS"]
- imagen: OTRA: la compilada en el PC (bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (09-10 08:22)) da 406b4cdc
- imagen: arch=243 variant=None cpuFreqHz=360000000 resetReason=power-on uptimeMs=11500221 fs=163840/16777216 B vmHeapBytes=24637440
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM15
- pantalla: si (INFO screenW x screenH = 480x800)
- inventario /lib+/app: 14 ficheros en /lib+/app, omitidos=0

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatPesadaPlaca | **COLGADA** | - | - | sin EXITED en 30 s; tras KILL: tampoco EXITED en 5 s; recuperacion: reinicio por el cable (RTS; el wire no contesto al RESET): vuelve (resetReason=power-on, uptimeMs=10318) |

- tras NatPesadaPlaca: uptimeMs=11167 fsUsed=167936 vmHeapBytes=24637440 pila libre: run=-1 io=-1 tareaREPL=28744 heap del sistema, minimo libre=427943
<details><summary>NatPesadaPlaca: log post-mortem de la placa, tras recuperarla</summary>

```
[   61] flash: el FS va ARRIBA, 16384..32768 KB (16384 KB); bpdata, entera para los packs (10144 KB); part.fs.size no se aplica en esta placa
[   62] particiones: el FS arriba de bpdata (la flash del chip; el ENV no reparte) — fs 16384 KB @0x9e8000, packs 10144 KB @0x0
[   63] fs: bpfsalto @0x1000000 size=16384 KB | region FS +0x0 size=16384 KB
[  146] fs: 15 ficheros, 155648/16777216 bytes usados
[  266] boot: estado 3 (app)
[  267] pack: zona 10144 KB en bpdata+0x0 (fisica 6240..16384 KB) | INST ok @0x400c8000 | DATA ok @0x400c8000
[  267] pack: zona montada en 0x400c8000 (10144 KB) — modulos y .mdn visibles
[  268] packs: zona lista, 3 pack(s) (3 vivo(s)), 8688 KB libres de 10144
[  336] pantalla: el P4 publica 'PANT' desde la imagen (MIPI-DSI, el panel del ENV, tactil GT911)
[  337] pantalla: 2 pines suyos (la luz y el reset): fuera de Machine.pins() (#669)
[  337] pantalla: el I2C 0 del tactil (sda 7, scl 8, 100 kHz) es de la placa y se comparte (#669)
[  337] sd: SDIO slot 0, 4 bit(s), clk 43 cmd 44 d0 39 | pwr 45 (activo bajo) | ldo 4 | 20000 kHz (por defecto)
[  416] sd: montada en /sd por SDMMC (particion en el bloque 2048)
[  416] boot: imagen de DOS nucleos
[  417] run: la tarea del programa, creada (16 KB de pila, prioridad 5; espera en la cola)
[  417] run: el 2o run, creado (vm.runs=2; 16 KB de pila, RAM interna): dos runs en la cola
[   56] === boot ESP32-P4 ===
[   56] log: arranque en frio (cargado de flash)
[   60] packs: bloque de RAM 4096 KB @0x48001000 en PSRAM (packRam=0 KB, SQLite=4 MB) — los estaticos de cada motor y la arena se reparten al cargar
[   60] psram: libre 28669 KB | mayor 28160 KB | bloques: 2 libres, 1 usados
[   60] vm: 24573 KB en PSRAM (todo lo que deja el margen de la región)
[   60] vm: heap 24572 KB en PSRAM @0x48401004 (PSRAM libre 4093 KB) | DRAM interna libre 478727 B (bloque mayor 376832 B)
[   60] bios: tabla COMPLETA (47 ranuras)
[   61] flash: configurada 16384 KB | chip fisico 32768 KB | bpdata acaba en 16384 KB
[   61] flash: el FS va ARRIBA, 16384..32768 KB (16384 KB); bpdata, entera para los packs (10144 KB); part.fs.size no se aplica en esta placa
[   62] particiones: el FS arriba de bpdata (la flash del chip; el ENV no reparte) — fs 16384 KB @0x9e8000, packs 10144 KB @0x0
[   63] fs: bpfsalto @0x1000000 size=16384 KB | region FS +0x0 size=16384 KB
[  144] fs: 18 ficheros, 167936/16777216 bytes usados
[  266] boot: estado 3 (app)
[  267] pack: zona 10144 KB en bpdata+0x0 (fisica 6240..16384 KB) | INST ok @0x400c8000 | DATA ok @0x400c8000
[  267] pack: zona montada en 0x400c8000 (10144 KB) — modulos y .mdn visibles
[  268] packs: zona lista, 3 pack(s) (3 vivo(s)), 8688 KB libres de 10144
[  337] pantalla: el P4 publica 'PANT' desde la imagen (MIPI-DSI, el panel del ENV, tactil GT911)
[  337] pantalla: 2 pines suyos (la luz y el reset): fuera de Machine.pins() (#669)
[  337] pantalla: el I2C 0 del tactil (sda 7, scl 8, 100 kHz) es de la placa y se comparte (#669)
[  337] sd: SDIO slot 0, 4 bit(s), clk 43 cmd 44 d0 39 | pwr 45 (activo bajo) | ldo 4 | 20000 kHz (por defecto)
[  377] sd: montada en /sd por SDMMC (particion en el bloque 2048)
[  377] boot: imagen de DOS nucleos
[  377] run: la tarea del programa, creada (16 KB de pila, prioridad 5; espera en la cola)
[  377] run: el 2o run, creado (vm.runs=2; 16 KB de pila, RAM interna): dos runs en la cola
```
</details>

<details><summary>NatPesadaPlaca: salida recibida de la placa (sin EXITED)</summary>

```
ok pesada: entra en la native
```
</details>

- ejecutadas (con EXITED) 0 de 1 listadas; no terminaron 1; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatPesadaPlaca.mod | bpgenvm-c\build\tanda\NatPesadaPlaca\aot-RV32IMAFC_ILP32F\NatPesadaPlaca.mod | 658 | 7a447ac4 | ok |

notas de dependencias:

- la placa NO lleva la imagen compilada en el PC: imageCrc 4d69b438, bpgenvm-c/esp32p4/build/bpvm_esp32p4.bin (09-10 08:22) da 406b4cdc
- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- NatPesadaPlaca: COLGADA; reinicio por el cable (RTS; el wire no contesto al RESET): vuelve (resetReason=power-on, uptimeMs=10318)
- limpieza: 0 fichero(s) de /app borrados

### Placa stm32u5g9 - 20353850553250040015002D

- sello: boardName=`stm32u5g9` uniqueId=`20353850553250040015002D` serverName=`bpvm-stm32` serverBuild=`Oct  8 2026 16:20:34` imageCrc=`b2891a18` capacidades=["META", "FILES", "TERMINAL", "PACKS"]
- imagen: OTRA: la compilada en el PC (bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (09-10 08:20)) da 8a51ed36
- imagen: arch=40 variant=None cpuFreqHz=160000000 resetReason=power-on/brown-out uptimeMs=11555295 fs=81920/1032192 B vmHeapBytes=786432
- al conectar: nada corria (KILL: NO_SESSION)
- transporte: serie COM12
- pantalla: si (INFO screenW x screenH = 800x480)
- inventario /lib+/app: 1 ficheros en /lib+/app, omitidos=0
- **ERROR de placa**: colgada en NatPesadaPlaca y no se recupera: no contesta al RESET por el wire, y stm32u5g9 no se reinicia sin manos: reiniciala con la sonda (STM32_Programmer_CLI -hardRst) o su boton

| prueba | veredicto | ms placa | ms host | detalle |
|---|---|---|---|---|
| NatPesadaPlaca | **COLGADA** | - | - | sin EXITED en 30 s; tras KILL: tampoco EXITED en 5 s; recuperacion: no contesta al RESET por el wire, y stm32u5g9 no se reinicia sin manos: reiniciala con la sonda (STM32_Programmer_CLI -hardRst) o su boton |

<details><summary>NatPesadaPlaca: salida recibida de la placa (sin EXITED)</summary>

```
ok pesada: entra en la native
```
</details>

- ejecutadas (con EXITED) 0 de 1 listadas; no terminaron 1; saltadas 0; no llegaron a arrancar 0

subido a /app:

| remoto | origen | tamano | crc32 | resultado |
|---|---|---|---|---|
| /app/NatPesadaPlaca.mod | bpgenvm-c\build\tanda\NatPesadaPlaca\aot-ARMV8M_MAIN_SOFTFP\NatPesadaPlaca.mod | 630 | 44d28f70 | ok |

notas de dependencias:

- la placa NO lleva la imagen compilada en el PC: imageCrc b2891a18, bpgenvm-c/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.elf (09-10 08:20) da 8a51ed36
- dependencia Core: en la zona de packs (pack:Stdlib/Core.mod, crc igual): se ejecuta XIP, no se sube
- NatPesadaPlaca: COLGADA; no contesta al RESET por el wire, y stm32u5g9 no se reinicia sin manos: reiniciala con la sonda (STM32_Programmer_CLI -hardRst) o su boton

### Matriz pruebas x placas

| prueba | rp2350a@COM22 | esp32p4@COM15 | stm32u5g9@COM12 |
|---|---|---|---|
| NatPesadaPlaca | COLGADA | COLGADA | COLGADA |


