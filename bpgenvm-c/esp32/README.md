# BasicPlus VM — firmware ESP32-S3 (H4)

Port del intérprete BasicPlus (VM-C) al ESP32-S3 (Xtensa LX7). Es la
**segunda arquitectura** del proyecto (la primera es la Pico/RP2350,
ARM Cortex-M33) — la prueba de portabilidad que cierra V1.

El **core VM** (`../src/*`) es C99 portable y compila tal cual. Lo
específico de esta plataforma vive en `main/`: `platform_esp32.c`
(backend de `bpvm_platform.h`), `wire_v1.c` (wire sobre UART0),
`fs_ram.c` (FS en RAM) y `repl_esp32.c` (REPL wire v1). `esp32/` es
**autocontenido** (copia local de `fs.h`, `wire_v1.h`, `json_min.{c,h}`)
para no arrastrar la `FreeRTOSConfig.h` de la Pico por el include path.

> **AOT no aplica aquí.** El `.mdn` es código ARM Thumb-2 y no cruza a
> Xtensa. En el ESP32-S3 todo se ejecuta interpretado.

## Canales (¡importante!)

La ESP32-S3-WROOM-1 (DevKitC-like) expone **dos puertos USB**:

| Puerto | Aparece como | Uso |
|--------|--------------|-----|
| Bridge USB-UART | **CP210x / CH340** (COM estable) | **UART0 = wire v1.** El IDE conecta aquí (115200). Flasheo. |
| USB nativo | **USB JTAG/serial debug unit** (se re-enumera al resetear) | **Consola/logs ESP-IDF** (`printf`, panics). `idf.py monitor` para depurar. |

La consola está en USB-Serial-JTAG (`sdkconfig.defaults`), así que el
**wire binario en UART0 queda limpio** de logs de texto. Para depurar el
arranque, monitoriza el puerto **nativo**; el IDE/wire usa el del
**bridge**.

## Build

```powershell
cd bpgenvm-c/esp32
idf.py set-target esp32s3     # solo la primera vez (o si cambias sdkconfig.defaults)
idf.py build
```

## Flash

Por el puerto del **bridge** (auto-reset fiable). Sin `monitor` (ese
puerto es para el IDE/wire):
```powershell
idf.py -p COMb flash          # COMb = bridge CP210x/CH340 (p.ej. COM11)
```
Para ver logs/arranque, en paralelo y en el puerto **nativo**:
```powershell
idf.py -p COMnativo monitor   # USB JTAG/serial debug unit
```

## Probar el wire sin el IDE

`wire_test.py` manda un HELLO y muestra la respuesta (útil para aislar
firmware vs IDE):
```powershell
python wire_test.py COMb
# espera: [rx] {"type":"HELLO_REPLY","serverName":"bpvm-esp32",...}
```

## "Run on ESP32" desde el IDE

1. Backend serie del IDE → el COM del **bridge**, 115200 (default).
2. Compila un `.bp`, y **Run → "Run on Pico"** (genérico; habla wire v1).
3. El IDE: HELLO → PUT del `.mod` (al FS RAM) → RUN → streamea OUTPUT →
   EXITED. **Validado** con programas OO completos.

## Gotchas del bring-up (lecciones)

- **El IDE no debe resetear el ESP32 al abrir el puerto.** El bridge usa
  DTR/RTS como auto-reset (DTR→GPIO0, RTS→EN). `BpvmClient` pone
  `DTR=RTS=true` (iguales → no resetea; la Pico ignora RTS).
- **Tick de FreeRTOS = 100 Hz en ESP-IDF** (1000 Hz en Pico).
  `pdMS_TO_TICKS(5)=0` → `vTaskDelay(0)` no cede CPU → busy-spin → el task
  watchdog mata IDLE0. El wire **bloquea en `uart_read_bytes`** (cede CPU).
- **Con la consola en USB-JTAG, ESP-IDF no enruta UART0.** Hay que
  `uart_set_pin(UART_NUM_0, 43, 44, ...)` explícito o RX/TX quedan muertos.

## Estado (hitos H4)

- [x] **H4.1** — core VM compila Xtensa + Hello.mod imprime. Validado.
- [x] **H4.2** — `platform_esp32.c` (esp_timer, FreeRTOS). En uso.
- [x] **H4.3** — wire v1 sobre UART0 → "Run on ESP32" desde el IDE.
      **Validado end-to-end** (PUT + RUN + OUTPUT + EXITED).
- [x] **H4.4** — FS persistente (partición `bpfs` + `esp_partition`).
      **Validado con power-cycle**: el `.mod` sobrevive a apagar/encender.
- [ ] H4.5 — backend GPIO (blink). `Blink.mod` falla hasta tenerlo.
