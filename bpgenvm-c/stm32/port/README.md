# stm32/port — glue de la VM para el STM32U575 (H9.1.2)

Capa mínima para correr la VM BasicPlus **bare-metal single-thread** en el U575
y sacar la salida por el **VCP del ST-LINK**. Se injerta en el proyecto CubeIDE
(`../Nucleo_u575b/...`) que ya hace blink + VCP (H9.1.0).

## Ficheros
| Fichero | Rol |
|---|---|
| `platform_stm32.c` | Implementa `bpvm_platform.h` (mutex/cond no-op, thread→error, `now`=HAL_GetTick, `sleep`=HAL_Delay, `busy_wait`=DWT). Sustituye a `src/platform_pthread.c`. |
| `bpvm_app.c` / `.h` | `bpvm_app_run_hello()`: buffer estático 128 KB + `output_cb`→VCP + carga `Hello.mod` embebido + `bpvm_run`. |
| `hello_mod.c` | `Hello.mod` embebido (`const`, va a flash) — generado con `xxd -i`. |
| `Hello.bp` / `Hello.mod` | Fuente + bytecode. Salida: `Hola desde STM32 (BasicPlus VM)` / `42`. |

Clave: en `interp.c::emit_text`, sin SMP y con `output_cb` instalado, la VM llama
el callback **directo** → **no hace falta comm-task ni FreeRTOS** para el Hello.

## Integración en CubeIDE (proyecto `Nucleo_u575b`)

1. **Include path del core**: Project ▸ Properties ▸ C/C++ Build ▸ Settings ▸
   *MCU GCC Compiler ▸ Include paths* ▸ añade:
   - `C:\lenguajes\pm\bpgenvm-c\include`
   - `C:\lenguajes\pm\bpgenvm-c\stm32\port`

2. **Core de la VM como carpeta enlazada**: clic derecho en el proyecto ▸ New ▸
   Folder ▸ *Advanced* ▸ "Link to alternate location (Linked Folder)" ▸ navega a
   `C:\lenguajes\pm\bpgenvm-c\src` ▸ nombre `bpvm_src` ▸ Finish.
   - **Excluir `platform_pthread.c`**: despliega `bpvm_src` ▸ clic derecho en
     `platform_pthread.c` ▸ Properties ▸ C/C++ Build ▸ marca **"Exclude resource
     from build"**. (Usa `pthread.h`, que no existe bare-metal; lo sustituye
     `platform_stm32.c`.) **Es el ÚNICO fichero a excluir** — el resto de `src/`
     es C99 portable y compila para M33.

3. **Glue como carpeta enlazada**: New ▸ Folder ▸ Advanced ▸ Linked Folder ▸
   `C:\lenguajes\pm\bpgenvm-c\stm32\port` ▸ nombre `bpvm_port` ▸ Finish.
   (Compila `platform_stm32.c`, `bpvm_app.c`, `hello_mod.c`.)

4. **Subir heap/stack del libc** (`bpvm_init` hace un `malloc` del struct de
   control; el default 0x200 es muy justo): `.ioc` ▸ Project Manager ▸ *Linker
   Settings* ▸ **Minimum Heap Size = `0x4000`**, **Minimum Stack Size = `0x1000`**
   ▸ regenerar. (O editar el `.ld`: `_Min_Heap_Size`/`_Min_Stack_Size`.)

5. **Llamar a la VM desde `Core/Src/main.c`**:
   - `USER CODE BEGIN Includes`: `#include "bpvm_app.h"`
   - `USER CODE BEGIN 3` (arranca una vez + sigue parpadeando):
     ```c
     static int s_started = 0;
     if (!s_started) { s_started = 1; bpvm_app_run_hello(); }
     BSP_LED_Toggle(LED_GREEN);
     HAL_Delay(500);
     ```

6. **Build ▸ Run** ▸ terminal serie @115200 sobre el COM del ST-LINK. Esperado:
   ```
   === BasicPlus VM en STM32U575 (H9.1) ===
   Hola desde STM32 (BasicPlus VM)
   42
   === FIN VM (status=OK) ===
   ```
   Las 2 líneas centrales son **byte-idénticas** a la VM-Java / VM-C host (paridad).

## Gotchas
- *Undefined reference a `bpvm_comm_*`*: no excluyas `comm_host.c` (sí compila
  bare-metal; sólo se excluye `platform_pthread.c`).
- *`bpvm_init` devuelve NULL*: sube el heap (paso 4).
- *RAM overflow en el link*: baja `BPVM_MEM_SIZE` en `bpvm_app.c`.
- *Las 2 líneas del programa salen "en escalera"*: el `print` de la VM emite
  `\n` (LF) sin CR — es correcto (paridad). Pon el terminal en "implicit CR on
  LF" si molesta; NO traducir en el `output_cb` (rompería la paridad).

## Siguiente (H9.2)
Activar FreeRTOS + RX del wire-v1 sobre el USART → "Run on STM32" desde el IDE.
Entonces `platform_stm32.c` se reemplaza por una copia de
`../../pico/platform_freertos.c` (API idéntica) y el `output_cb` pasa a la
output-queue + comm-task.
