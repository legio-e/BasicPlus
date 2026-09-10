# Bajo consumo: qué hace hoy cada micro cuando no tiene nada que hacer

> **Qué es esto.** El censo de `#490` (10-sep-2026), hecho con seis lecturas independientes
> (Pico, ESP32, STM32, el planificador de la VM, el bombeo de la GUI, la superficie BP), una pasada
> de refutación adversarial —**57 de 73 hallazgos sobrevivieron**, 16 tumbados— y un crítico de
> completitud que encontró trece huecos, varios de ellos gordos. **No es una decisión ni un diseño:
> es el punto de partida del estudio.**
>
> ⚠️ **Sin instrumento no hay números.** Ni un solo mA en este informe, y es a propósito.

# Qué hace hoy cada micro cuando no tiene nada que hacer

**Censo de #490 — 10-sep. Sin instrumento: ni un solo mA en este informe.**

Antes de la tabla, las cuatro cosas que hay que distinguir, porque se confunden siempre y sólo la última ahorra batería:

- **(a) el hilo cede la CPU** — el lazo se bloquea en el SO en vez de girar. Es reparto, no ahorro.
- **(b) la tarea ociosa corre** — hay hueco para que el kernel llegue a su `prvIdleTask`. Sigue sin ahorrar nada por sí solo.
- **(b′) el núcleo se para** — la ociosa ejecuta `WFI`/`waiti`: la CPU deja de buscar instrucciones hasta la siguiente interrupción. **PLL, relojes de bus, flash, PSRAM y periféricos siguen exactamente igual.** Ahorra algo, pero no es dormir.
- **(c) el micro entra en un modo de bajo consumo** — tickless/light-sleep/Stop: se suprime el tick, se paran relojes, se baja tensión. **Es el único escalón que cuenta para una pila.**

---

## 1. LA RESPUESTA EN UNA TABLA

Escenario: placa encendida, **sin programa**, REPL esperando al IDE.

| Familia | Qué hace el lazo de reposo | (a) cede | (b) ociosa corre | (b′) núcleo parado | (c) bajo consumo |
|---|---|---|---|---|---|
| **RP2350** (Pico 2 / Metro) | `getchar_timeout_us(0)` + `vTaskDelay(10 ms)` → ~100 vueltas/s | **SÍ** | **SÍ** | **NO** | **NO** |
| **ESP32-S3** | `wire_read_byte(100)` — bloquea ≤100 ms en el driver UART → ~10/s | **SÍ** | **SÍ** | **SÍ** (`waiti`) | **NO** |
| **ESP32-C3** | ídem por USB-Serial-JTAG (`usb_serial_jtag_read_bytes`, 100 ms) | **SÍ** | **SÍ** | **SÍ** (`wfi`) | **NO** |
| **ESP32-C6** | ídem C3 | **SÍ** | **SÍ** | **SÍ** (`wfi`) | **NO** |
| **ESP32-P4** | ídem S3 (UART0) | **SÍ** | **SÍ** | **SÍ** (`wfi`) | **NO** |
| **STM32U5 DK2** | `getchar()` devuelve −1 y sigue: **gira** | **NO** | **NO** | **NO** | **NO** |
| **STM32U5 Nucleo** | ídem DK2 | **NO** | **NO** | **NO** | **NO** |

**Ninguna de las siete llega a (c). Ninguna. Hoy no hay una sola línea en el árbol que meta a un micro en un modo de bajo consumo.**

### La evidencia, columna por columna

**RP2350 — cede, pero la ociosa gira a la frecuencia del reloj.**
El lazo cede: `pico/repl_v1.c:1720-1727`. Pero `prvIdleTask` es un bucle sin instrucción de espera, y esto está verificado **en la imagen construida**, no leyendo: `pico/build/bpvm_pico.dis:63665-63674` (push / bl `prvCheckTasksWaitingTermination` / ldr / cmp / bls / vPortYield). En las 82.898 líneas del desensamblado hay **cero `wfi`**; los tres `wfe` (líneas 51101, 51118, 62193) están en esperas de cerrojo del interop SDK/FreeRTOS, no en el camino de reposo. Causa: `configUSE_IDLE_HOOK 0` (`pico/FreeRTOSConfig.h:39`) y `configUSE_TICKLESS_IDLE` **que no aparece en ninguna parte** — ni en el FreeRTOSConfig.h (111 líneas), ni en `pico/CMakeLists.txt`, ni en el `build.ninja` generado — luego vale el default 0 del kernel (`FreeRTOS.h:2727-2728`), y el bloque de `tasks.c:5883-5927` se compila fuera.
Encima: SysTick a 1000 Hz permanente (`FreeRTOSConfig.h:43`, reload en `port.c:856`), 150 MHz fijos (default del SDK, `platform_defs.h:114`, sin override en el CMakeLists), y ninguna librería de bajo consumo enlazada (`pico/CMakeLists.txt:195-213`: nada de `hardware_powman` ni `pico_sleep`).

**ESP32 (los cuatro) — el núcleo sí se para, pero el chip sigue entero encendido.**
El kernel de IDF llama **siempre** a `esp_vApplicationIdleHook()` (`components/freertos/FreeRTOS-Kernel/tasks.c:4350`, fuera del `#if configUSE_IDLE_HOOK`), y con PM apagado eso es `esp_cpu_wait_for_intr()` (`esp_system/freertos_hooks.c:52-58`) → `waiti 0` en Xtensa / `wfi` en RISC-V (`esp_hw_support/cpu.c:52-64`).
Lo que no hay es el escalón (c): `# CONFIG_PM_ENABLE is not set` en las cuatro (`esp32/sdkconfig:1805`, `esp32c3:1600`, `esp32c6:1875`, `esp32p4:2227`), confirmado en el `sdkconfig.h` generado (el símbolo **no existe**). Y no es decisión nuestra: ninguno de los cuatro `sdkconfig.defaults` menciona PM, así que es el `default n` de IDF (`esp_pm/Kconfig:14-18`). Los cuatro chips **sí lo soportan** (`SOC_PM_SUPPORTED`/`SOC_LIGHT_SLEEP_SUPPORTED` = 1) y los cuatro son UNICORE, así que la cláusula `!FREERTOS_SMP` tampoco lo bloquea.
Consecuencias encadenadas: `CONFIG_FREERTOS_USE_TICKLESS_IDLE` y `CONFIG_PM_DFS_INIT_AUTO` **ni siquiera existen** en los sdkconfig (dependen de PM_ENABLE, Kconfig no las muestra); tick a 100 Hz siempre; frecuencia clavada en 160 MHz (S3/C3/C6) y 360 MHz (P4). Y **cero llamadas** en código propio a `esp_pm_configure`, `esp_light_sleep_start`, `esp_deep_sleep` o cualquier `esp_sleep_*` (grep sobre todo `bpgenvm-c` excluyendo `build/` y `managed_components`).
*Trampa que hay que tener presente:* varios `CONFIG_PM_*` y `CONFIG_ESP_SLEEP_*` **sí aparecen a `y`** (p.ej. `esp32c6/sdkconfig:1873-1889`). Son defectos de IDF para el camino de sleep e **inertes** mientras PM esté apagado y nadie llame a las APIs. Leerlos al revés es fácil.
*Salvedad medible:* con un depurador JTAG enganchado, `esp_cpu_wait_for_intr()` **retorna sin ejecutar el wfi** (`cpu.c:56-60`). O sea que midiendo con JTAG el núcleo no se para.

**STM32U5 — no llega ni a (a).**
El lazo es `for(;;) { c = stm32_wire_getchar(); if (c=='{') dispatch(c); ...heartbeat }` en `stm32/port/stm32_repl.c:865-874` (reverificado hoy: el fichero se tocó a las 17:27; `for(;;)` en :865, el getchar en :866, el parpadeo en :870-872). Las dos variantes de `stm32_wire_getchar` devuelven −1 al instante (`stm32/port/stm32_wire.c:71-78` anillo de IRQ, `:82-92` sondeo directo — esta segunda es código muerto, las dos placas definen `BOARD_WIRE_IRQn`). No hay `vTaskDelay`, ni `taskYIELD`, ni WFI en ese camino.
Y como `vm` es la única tarea y corre a `tskIDLE_PRIORITY+2` (`Discovery_u5g9j/Core/Src/main.c:173`, `Nucleo_u575b/Core/Src/main.c:191`), **la tarea ociosa no se ejecuta jamás en reposo**.
Verificado hoy contra el artefacto, que era el hueco del censo: `wfi` = **0** y `wfe` = **0** en `Discovery_u5g9j/Debug/Discovery_u5g9j.list` (10-sep 17:28) y `wfi` = 0 en `Nucleo_u575b/Debug/Nucleo_u575b.list` (9-sep 16:11). `prvIdleTask` está en `0800c5c0` y es el mismo bucle de la Pico, sin instrucción de espera. Causas: `configUSE_IDLE_HOOK 0` y `configUSE_TICK_HOOK 0` (`stm32/port/FreeRTOSConfig.h:54-55`), `configUSE_TICKLESS_IDLE` ausente de las 146 líneas del fichero y sin `-D` en el proyecto CubeIDE, y el único `wfi` del port ARM_CM33_NTZ vive dentro de `#if (configUSE_TICKLESS_IDLE == 1)` (`port.c:598` y `:685`).
Ningún modo de bajo consumo del U5 se toca: cero `HAL_PWR_Enter*`/`HAL_PWREx_Enter*`/LPRUN/Stop/Standby fuera de los Drivers. Lo único de PWR que se ejecuta son habilitaciones de dominios (SMPS + VddIO2 en `Discovery/main.c:270-283`, ídem + UCPD en `Nucleo/main.c:263-281`, y `HAL_PWR_EnableBkUpAccess` en `stm32/port/gpio_stm32.c:267`). Reloj clavado en el máximo: VOS Range 1, PLL, AHB/APB todos DIV1, `FLASH_LATENCY_4`, y **siete fuentes de reloj encendidas a la vez en la DK2** (HSE, HSI, LSE, MSI, PLL1 en `main.c:205-214`; PLL2 para el HSPI en `stm32u5xx_hal_msp.c:203-213`; PLL3 para el LTDC en `:456-460`) — ninguna se apaga después.

### Y con un programa BP cargado que "no hace nada"

Este es el caso que de verdad importa para pilas, y es peor que el anterior en las tres familias:

| Reloj | Periodo | Dónde |
|---|---|---|
| Tick de FreeRTOS | 1 ms (Pico, STM32) / 10 ms (ESP) | `pico/FreeRTOSConfig.h:43`, `stm32/port/FreeRTOSConfig.h:50`, `CONFIG_FREERTOS_HZ=100` |
| **TIM17 del HAL** (sólo STM32) | **1 ms** | `stm32u5xx_hal_timebase_tim.c:41-81`, arrancado y nunca parado |
| Hilo `io` | 5 ms pedidos (→ 1 tick) | `src/bpvm_io.c:41` y `:105-119` |
| Tope del planificador BP | 50 ms | `src/scheduler.c:144` |
| Lazo de la GUI | ≤10 ms | `include/bpvm_gui.h:140` (`BPVM_GUI_OCIO_MAX_MS`) |
| Táctil GT911 (DK2 y P4) | 33 ms | `third_party/lvgl/src/indev/lv_indev.c:130` |

Dos matices con los que hay que tener cuidado al leer esa lista:

1. **No son "despertares del micro"**: hoy el micro no duerme nunca, así que son eventos sobre una CPU que ya está despierta. Son la lista de lo que **habría que eliminar o tolerar el día que se duerma**, no la factura de hoy.
2. **El tope de 50 ms del planificador es hoy redundante durante un RUN.** El motivo escrito era atender un KILL, pero desde V6/A1 eso lo hace el hilo `io`: `scheduler.c:100` se salta la llamada al poll cuando `vm->io != NULL`, y sin embargo el recorte de `:144` **sólo mira `poll_cb`, no `vm->io`**, así que se sigue aplicando. Son ~20 despertares/s de la tarea `vm` que hoy no sirven para nada.

**`sleep()` de BP es limpio** y acaba en `vTaskDelay` (`builtins.c:2492-2510` → `interp.c:1659-1662` → `scheduler.c:145` → los tres `vTaskDelay`), con dos peajes: el troceado en 50 ms de arriba, y el redondeo a ≥1 tick (en los ESP, cualquier `sleep` de 1..9 ms son 10 ms). **`sleepUs()` es lo contrario**: espera activa pura que no cede ni el hilo BP ni la CPU (`builtins.c:2512-2519`; `busy_wait_us` en la Pico, `DWT->CYCCNT` en el STM32, `esp_rom_delay_us` en los ESP). `sleepUs(500000)` son 0,5 s de micro a plena carga y su nombre no lo delata.

### Cargas estáticas que no dependen de la CPU (y que nadie apaga)

- **Metro (RP2350):** el NeoPixel onboard se enciende **verde tenue en el arranque y no lo apaga nadie** (`pico/main.c:1343-1347`, "test H7.4.a"), y reclama una SM del PIO (`pico/neopixel.c:30-49`). En la Pico pelada es no-op. *Esto contradice al hallazgo que decía cerrar la pregunta de "qué queda encendido": ese hallazgo sólo miró `BPVM_PICO_BOOT_LED`, el latido y la SD.*
- **S3 y P4:** PSRAM alimentada y con reloj siempre (OCT 80 MHz y HEX 200 MHz, en sus `sdkconfig.defaults`). En el S3 quien decide es la **placa**, no la imagen (`CONFIG_SPIRAM_IGNORE_NOTFOUND=y`). El RP2350 **también puede llevarla**, conducida por el ENV (`pico/main.c:1148`, `board_desc_psram_from_env(... "psram" ...)`).
- **STM32:** el LED verde del heartbeat conmuta cada 500 ms para siempre (`stm32_repl.c:870-872`). En la DK2 es el **mismo** LED que el de RUN, así que en reposo es 50 % y durante un RUN está fijo. En la Nucleo son LEDs distintos.
- **DK2:** el **LTDC escanea el panel desde el arranque**, corra o no un programa gráfico — `MX_LTDC_Init()` es incondicional (`main.c:151`), `HAL_LTDC_Init` pone LTDCEN (`stm32u5xx_hal_ltdc.c:308-311`), el pin LCD_ON en alto (`main.c:1167`) y el reloj de píxel es real (PLL3, `stm32u5xx_hal_msp.c:456-473`). La retroiluminación **sí** está apagada en reposo: su PWM (TIM3_CH4) sólo arranca al abrir la GUI (`gui_display_ltdc.c:101`), y CubeMX lo deja al **75 %**, no al 100 % (`main.c:837-856`) — o sea que en la DK2 **el circuito de atenuar ya está montado; lo que falta es el verbo**.
- **STM32, el bloque grande del que no se había escrito una línea:** **19 periféricos se inicializan incondicionalmente** en el arranque de la DK2 (`main.c:145-163`: GPIO, DMA2D, HSPI1, I2C1/2, ICACHE, LTDC, RTC, SPI1/2/3, TIM3, UART4, USART1/2/3/6, USB_OTG_HS_PCD, ADC1) y **16 en la Nucleo** (`main.c:137-152`: cuatro I2C, LPUART1, UART4/5, USART2/3, tres SPI, ADC1, RTC). Cinco UART y tres SPI con reloj de bus dado para siempre, más el PHY del USB HS alimentado. Contraste que prueba que no es inevitable: en la Pico el ADC es perezoso (`pico/main.c:605-608`).
- **P4 y C6 con GUI:** el backlight se enciende al máximo en el init y **no hay ningún camino que lo baje ni API en `Gui` para hacerlo** (`esp32p4/main/gui_display_dsi.c:150-175`, duty 1023; `esp32c6/main/gui_display_st7789.c:105-110`, GPIO a 1). El panel tampoco se apaga (`disp_on_off` sólo con `true`).
- **Negativos útiles, para no mandar a nadie donde no está:** el P4 construido hoy **no tiene Ethernet ni lwIP** (cero objetos de `libesp_eth`/`liblwip` en su `.map`; la red vive toda bajo `#if BPVM_P4_NETLOG`, apagado). **No hay radio**: cero llamadas a `esp_wifi_*`/`esp_bt_controller_*`/`esp_phy_*` en código propio, y `bpstdlib/Net.bp:10-13` declara explícitamente que no hay backend en firmwares — aunque `CONFIG_ESP_WIFI_ENABLED=y` aparezca en los cuatro sdkconfig, que es exactamente la misma trampa que los `CONFIG_PM_*`. El **planificador SMP es código muerto** en las cinco placas (sólo se alcanza con `-DBPVM_PICO_SMP_WORKERS`, que no está definido en ningún sitio).

---

## 2. DÓNDE ESTÁ EL TOPE REAL HOY

**La respuesta transversal: no hay tope. No hay nada que impida dormir — es que nadie ha escrito el código de dormir.** El escalón (c) no existe en el árbol: cero llamadas a modos de bajo consumo en las cinco familias. Dicho esto, si mañana se quisiera encender, **cada familia tiene una pieza distinta que hay que quitar primero**, y el orden está determinado:

**STM32 — la pieza es el lazo de reposo del REPL.** Mientras `stm32_repl.c:865-874` gire, la tarea ociosa no se ejecuta nunca, y todo lo que se ponga en la ociosa (idle hook, tickless) **no llegaría a ejecutarse ni una vez**. Encender `configUSE_TICKLESS_IDLE` sin tocar ese lazo no cambiaría absolutamente nada. Orden obligatorio: **(1)** que el lazo se bloquee de verdad; **(2)** que la ociosa tenga WFI; **(3)** el TIM17, que despertaría cada 1 ms igualmente y que el tickless de FreeRTOS **no sabe suprimir** (sólo sabe del SysTick). Es la familia que menos duerme y es la de bajo consumo de ST.

**RP2350 — la pieza es la tarea ociosa sin WFI.** Aquí todo lo demás ya cede. El cambio más pequeño concebible es un `vApplicationIdleHook` con WFI, o encender el tickless: **la maquinaria ya está escrita en el port** (`RP2350_ARM_NTZ/non_secure/port.c:599-817`, con su `__asm volatile("wfi")` en `:686`), sólo se compila fuera. *Aviso de detalle:* el port gatea con `== 1` y `tasks.c` con `!= 0`; un `configUSE_TICKLESS_IDLE=2` compilaría el llamante y no la implementación.

**ESP32 — la pieza es `CONFIG_PM_ENABLE`.** El núcleo ya se para en `waiti`/`wfi`; lo que no baja es nada más. Es un `y` en cuatro `sdkconfig.defaults`, y detrás de él aparecen tickless y DFS.

**Y un tope transversal que hoy nadie ha mirado y que valdría para las tres:** el **techo del sueño** lo pone el reloj de sondeo más rápido. Con un programa cargado, el hilo `io` a 5 ms y el tope de 50 ms de `scheduler.c:144` significan que, aunque todo lo de arriba estuviera encendido, **ningún micro podría dormir más de un tick seguido**. Y en los ESP hay un límite duro adicional que nadie ha censado como lo que es: el **Interrupt WDT por hardware a 300 ms** en las cuatro imágenes (`esp32/sdkconfig:2025,2027`, `esp32c3:1706,1708`, `esp32c6:2006,2008`, `esp32p4:2392,2394`), más el del bootloader a 9 s. La Pico sí para su watchdog de verdad (`pico/main.c:889-896`, `watchdog_disable()`) y el STM32 hoy no registra backend de watchdog a propósito.

---

## 3. QUÉ SE PODRÍA HACER, DE MENOS A MÁS INVASIVO

Sin diseñar nada; sólo qué se gana y qué se arriesga.

**1. Apagar cargas estáticas que no toca nadie.** El NeoPixel de la Metro (un `test H7.4.a` que se quedó), el LED de heartbeat del STM32, y sobre todo hacer perezosos los 19/16 periféricos del arranque del STM32 como ya lo es el ADC de la Pico.
*Gana:* consumo que **no depende de la CPU** y que por tanto no mejora ni un ápice aunque algún día se duerma perfectamente. Es lo único de esta lista que da fruto sin tocar la VM.
*Riesga:* el LED y el NeoPixel, nada. Los periféricos del STM32, sí: quitar un `MX_*_Init` puede romper algo que lo usaba sin que se note hasta la placa. Es un trabajo de ir uno a uno, no un barrido.

**2. Que la ociosa deje de girar en las dos familias ARM** (idle hook con WFI, o el tickless que ya viene hecho en el port RP2350).
*Gana:* el escalón (b′) que los ESP ya tienen y las ARM no. Es el cambio más pequeño con efecto real en la Pico.
*Riesga:* en la Pico, poco — y hay una pista: el SysTick se programó **una vez** desde `clock_get_hz(clk_sys)` y nadie lo recalcula, así que tickless y frecuencia variable se pisan. En el STM32 no se puede hacer sin el paso 3. Y ojo con la moraleja de #462: allí se quitó un `__WFI()` del **bombeo del GUI**, donde sí había otras cosas que necesitaban turno (los hilos BP dentro de la misma tarea `vm`) — eso **no dice nada** del WFI en la ociosa, que por construcción sólo corre cuando no hay nada ejecutable. No hay que dar el tema por decidido por #462.

**3. Que el lazo de reposo del STM32 se bloquee.** Es el requisito previo de todo lo demás en esa familia.
*Gana:* pasa de (a)=NO a (a)=SÍ, que es el escalón que las otras dos ya tienen desde hace tiempo. Cierra la anomalía de que la única tarea ociosa que nunca corre sea la de la placa de bajo consumo.
*Riesga:* el wire. Hoy el getchar lee un anillo que llena una IRQ; para bloquear hace falta una espera del SO que esa IRQ despierte, y **si eso funciona o no no se puede afirmar leyendo** lo que hay hoy. También arrastra el bug histórico de la memoria: con la ociosa muerta de hambre, `vTaskDelete(NULL)` no recicla nada — el diseño actual lo esquiva suspendiendo y borrando desde `join` (`src/platform_freertos.c:208-237` y `:318`), y ese aviso hay que mantenerlo escrito pase lo que pase.

**4. Encender lo que ya viene hecho: PM en los ESP, tickless en la Pico.**
*Gana:* el primer escalón (c) real del proyecto, y en el ESP viene con DFS de regalo.
*Riesga:* el más alto de la lista en efectos laterales. PM cambia la frecuencia por debajo de los periféricos (UART, SPI, LEDC), y el wire es justo un periférico. Y no rinde hasta resolver los relojes de sondeo del punto 5 — con `io` a 5 ms, el tickless no tendría nada que suprimir.

**5. Quitar los relojes de sondeo que hoy sobran.** Empezando por el tope de 50 ms del planificador, que durante un RUN con hilo `io` **ya no lo usa nadie**; y por el caso **autorun sin IDE** (`pico/main.c:1413`, `esp32/common/main.c:253`, `stm32/port/stm32_repl.c:630` y `:862`), que es exactamente el modo en que vive un aparato a pilas: arranca solo y **no hay nadie al otro lado del cable**, así que los 200 sondeos/s del hilo `io` se pagan íntegros para atender un KILL que no puede llegar. Nadie ha preguntado nunca si en ese modo hacen falta.
*Gana:* es lo que decide si el sueño dura 5 ms o dura lo que pida el programa. Sin esto, todo lo anterior es decorativo.
*Riesga:* **toca el invariante dual-VM.** Y hay un dato que cambia el encuadre: **miVM ya hace lo correcto** — calcula `earliestSleepWakeMs()` y espera **el delta completo** con `vmLock.wait(delta)`, sin tope, porque quien llega lo despierta con un `notify` (`miVM/.../VirtualMachine.java:543` y `:2305-2317`). O sea que el tope de `scheduler.c:144` es una limitación de la VM-C, no del diseño, y el patrón bueno ya está escrito en la referencia.

**6. El contrato en BP, que es lo que plantea la propia ficha #490.** Hoy no existe ningún verbo que diga "despiértame": `bpstdlib/Rtc.bp` sólo tiene `epochSec()`/`setEpochSec()` (`:62`, `:68`), sin alarma; `bpstdlib/Timer.bp` es **sondeo puro** por diseño declarado en su cabecera (`:1-22`, `wait()` acaba en `sleep(rem)` en `:92-98`); y no hay ningún temporizador hardware expuesto a BP en ninguna familia.
Además, la palanca que **sí** está declarada en la API está desigualmente implementada: `Machine.bp` declara `MAX_CPU_MHZ()=300` y **`MIN_CPU_MHZ()=18`** (`:116-124`) para todas las familias, pero sólo la Pico tiene backend real — `pico/main.c:706-773`, que ajusta `clk_sys` **y** escala Vdd_core por tabla (≤200 MHz→1,10 V … >280→1,30 V) con la regla anti-cuelgue correcta. El STM32 devuelve 0 (`stm32/port/gpio_stm32.c:235-238`, "reloj fijo (160 MHz)"), el ESP también (`esp32/common/gpio_esp32.c:130-133`, `esp32p4/main/p4_board_id.c:66`). O sea: **desde BP se puede pedir bajar a 18 MHz en cinco familias y sólo una lo hace.** Y hay una mentira colateral en el S3: contesta 240 MHz a `Pico.cpuFreqHz` (`gpio_esp32.c:122-124`) mientras la imagen se compila a 160.
*Y el aviso de la propia ficha que este censo no contesta:* **timer y RTC no son intercambiables** — qué reloj sobrevive a qué modo es verdad de silicio y distinta por micro. Hoy el **RTC de hardware sólo tiene backend en el STM32** (`stm32/port/gpio_stm32.c:790`, `:1009`); el ESP usa a propósito el stub portable (`esp32/common/gpio_esp32.c:595`) y la Pico tampoco registra ninguno. **El reloj que se propone como despertador existe justo en la familia que hoy no llega ni a ejecutar su tarea ociosa, y no existe en las otras cuatro.**

---

## 4. QUÉ NO SE SABE — y necesita amperímetro

**Todos los mA. Sin excepción.** No hay instrumento y no voy a poner un número. En concreto, esto es lo que hoy es **no determinado — haría falta medir con un amperímetro**:

- Cuánto ahorra realmente cada escalón: qué diferencia hay entre la ociosa girando de la Pico y el `waiti` del ESP; qué diferencia hay entre eso y un tickless.
- Cuánto cuesta el **LTDC de la DK2 barriendo el panel con la retro apagada** — que es el estado de reposo sin GUI, y probablemente el consumidor más gordo de esa placa.
- Cuánto pesa la **PSRAM** del S3 y del P4 en reposo, con el micro sin tocarla.
- Cuánto pesan el LED de heartbeat del STM32 (50 % de duty) y el NeoPixel de la Metro.
- Cuánto cuestan los 19/16 periféricos del arranque del STM32, y cuáles de ellos importan.
- Qué se gana bajando la frecuencia en la Pico. Y hay que decirlo con rigor: **bajar la frecuencia no es dormir** — la ociosa sigue girando, sólo que más despacio.

**Lo que NO necesita amperímetro y sin embargo tampoco se sabe** (y esto sí es un hueco, porque se mide con lo que ya hay):

- **Cuánto cuesta hoy una vuelta en reposo, en las tres familias.** El único número del censo — 0,4 ms/vuelta, "~4 % de un núcleo" — es del **P4**, es de **antes de #462**, medía sólo `lv_timer_handler`, y su propia revisión dice que **no se puede extender al C6**. El instrumento ya está construido y nadie lo ha usado: el contador de vueltas de `esp32p4/main/gui_display_dsi.c:600-614`. Sin ese dato, la escalera de #490 no tiene línea base contra la que compararse.
- Si el **GT911 marca "buffer listo" sin que haya dedo**: de eso depende que el sondeo de 33 ms sean una o dos transacciones I2C por pasada. Se mira en el bus, no en el código.
- Si en el STM32 **una IRQ de UART despertaría de un WFI** puesto en la ociosa. No leído.
- **Qué reloj alimenta el RTC en cada placa STM32.** El comentario de `gpio_stm32.c:732` dice LSI ~32 kHz, pero el arranque de la DK2 enciende también el LSE. Sale de un `HAL_RCCEx_PeriphCLKConfig` que nadie ha leído — y **eso decide hasta qué modo sobrevive el despertador**.
- **Qué `Gui.mod` hay instalado en cada placa.** `Gui.mod` no viaja en el blob embebido (la tabla tiene 15 módulos y Gui no está: `esp32/common/esp32_mods.c:4405-4419`) y el IDE no sube la stdlib en cada Run (`BpIde/.../FrmMain.java:2419-2423`). Cualquier device cuya stdlib no se haya reinstalado **después del commit `a5a00daa`** tiene el lazo de GUI viejo, que **no tiene pausa ninguna** y gira a tope. El síntoma no es un fallo: es consumo. Y hay dos copias locales de `Gui.mod` con fechas distintas (`bpstdlib/Gui.mod` y `bpstdlib/out/Gui.mod`), así que antes de dar la receta "reinstala la stdlib" hay que saber cuál se instala por `stdlibDir`.

---

## Apéndice — dos avisos sobre este censo

**Una afirmación del censo era falsa y la he verificado:** se llegó a escribir que `bpvm_platform_thread_yield()` "en el STM32 es un no-op declarado". **No lo es.** `src/platform_freertos.c:323` es `void bpvm_platform_thread_yield(void) { taskYIELD(); }`, y ese es el fichero que enlaza el STM32; en `stm32/port/platform_stm32.c` no hay ninguna definición que lo tape (comprobado). Dejarlo pasar habría mandado a concluir que allí el planificador no cede ni al mismo nivel de prioridad.

**Y una lección de método:** el hallazgo que decía "esto cierra la pregunta de qué más hay encendido" **no la cerraba** — se le escapó el NeoPixel de la Metro, que está encendido desde el arranque. Una afirmación de completitud sin censo detrás es el peor tipo de hallazgo falso, porque apaga la búsqueda. Este informe **no** afirma ser completo: la lista de puntos 5 y 6 de arriba (autorun, contrato BP, RTC por placa) son subsistemas que hoy están sin censar, no descartados.

**Frescura de los artefactos** en que se apoyan las verificaciones: `pico/build/bpvm_pico.dis` y `.elf.map` de hoy 17:23; `esp32c6` 17:24; `esp32p4` 17:09; DK2 `.list` hoy 17:28. Del 9-sep: `esp32/build/bpvm_esp32.map` (S3) y `esp32c3/build/bpvm_esp32c3.map` (16:10-16:11) y `Nucleo_u575b.list` (16:11) — coinciden con sus fuentes, así que valen, pero conviene decirlo en vez de darlo por hecho: en la carpeta del P4 conviven dos `.map`, y el segundo (`bpvm_p4_netlog.map`) es del 3-ago y es **otra variante**, la que sí lleva red.