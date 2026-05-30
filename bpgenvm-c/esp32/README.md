# BasicPlus VM — firmware ESP32-S3 (H4)

Port del intérprete BasicPlus (VM-C) al ESP32-S3 (Xtensa LX7). Es la
**segunda arquitectura** del proyecto (la primera es la Pico/RP2350,
ARM Cortex-M33) — la prueba de portabilidad que cierra V1.

El **core VM** (`../src/*`) es C99 portable y compila tal cual. Lo único
específico de esta plataforma es `main/platform_esp32.c` (backend de
`bpvm_platform.h` sobre ESP-IDF) + el glue del proyecto.

> **AOT no aplica aquí.** El `.mdn` es código ARM Thumb-2 y no cruza a
> Xtensa. En el ESP32-S3 todo se ejecuta interpretado. Generar código
> Xtensa sería un esfuerzo aparte (v2).

## Requisitos

- ESP-IDF **v6.x** instalado y con el entorno activado (`export.ps1` /
  "ESP-IDF PowerShell"). Verifica con `idf.py --version`.

## Build

```powershell
cd bpgenvm-c/esp32
idf.py set-target esp32s3     # solo la primera vez
idf.py build
```

## Flash + monitor  ⚠️ ojo con el puerto

La placa ESP32-S3-WROOM-1 (DevKitC-like) expone **dos puertos USB**:

| Puerto | Aparece como | Uso |
|--------|--------------|-----|
| USB nativo | **"USB JTAG/serial debug unit"** | Flashea, pero **se re-enumera** en cada reset (da `Waiting for the device to reconnect`). |
| Bridge USB-UART | **"CP210x" / "CH340"** | Estable. **UART0** = donde sale nuestra consola. |

La consola está configurada en **UART0** (`sdkconfig.defaults`), así que
**hay que flashear y monitorizar por el puerto del BRIDGE** (CP210x/CH340),
no por el nativo — si no, no se ve la salida de la app.

```powershell
idf.py -p COMb flash monitor    # COMb = puerto del bridge (CP210x/CH340)
```

- Con **BOOT sin pulsar** (si GPIO0 queda en bajo, arranca en
  `boot:0x0 DOWNLOAD` en vez de ejecutar la app).
- Salir del monitor: **Ctrl + ]**.

Salida esperada (H4.1, `Hello.mod` embebido):
```
=== BasicPlus VM en ESP32-S3 (H4.1) ===
Hello from BasicPlus on ESP32-S3!
sum 0..9 =
45
VM OK
[done] status = OK
```

> *Alternativa de una sola conexión:* poner la consola en USB-Serial-JTAG
> (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`) y usar solo el puerto nativo
> para flash + consola. Pendiente de decidir al cablear el wire (H4.3).

## Estado (hitos H4)

- [x] **H4.1** — esqueleto ESP-IDF + core VM compila Xtensa + Hello.mod
      imprime por consola. **Validado en placa.**
- [ ] H4.2 — platform_esp32 (ya escrito; falta ejercitar threads/SMP).
- [ ] H4.3 — wire v1 sobre serie → "Run on ESP32" desde el IDE.
- [ ] H4.4 — FS (SPIFFS/partición) para cargar/persistir `.mod`.
- [ ] H4.5 — backend GPIO (blink).
