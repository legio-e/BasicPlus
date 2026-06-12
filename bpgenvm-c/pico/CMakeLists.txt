# CMakeLists.txt — firmware bpgenvm-c para Raspberry Pi Pico 2 (RP2350)
# sobre FreeRTOS-Kernel + Pico SDK.
#
# Uso (Windows con MinGW + Ninja del w64devkit):
#
#   set PICO_SDK_PATH=C:\lenguajes\pm\pico-sdk
#   set FREERTOS_KERNEL_PATH=C:\lenguajes\pm\FreeRTOS-LTS\FreeRTOS\FreeRTOS-Kernel
#   set PICO_PLATFORM=rp2350
#   set PICO_BOARD=pico2
#   mkdir build && cd build
#   cmake -G "Ninja" ..
#   cmake --build .
#
# Produce: build/bpvm_pico.uf2 — arrastrable a la Pico en modo BOOTSEL.

cmake_minimum_required(VERSION 3.13)

# Forzamos los defaults antes de que pico_sdk_init los detecte.
if (NOT DEFINED PICO_PLATFORM)
    set(PICO_PLATFORM "rp2350")
endif()
# H7.1 — board target GENÉRICO RP2350B (48 GPIO) por defecto. Header propio en
# sdk_board/bp_rp2350b.h (clon de pico2 con PICO_RP2350A=0). UNA imagen para
# Pico 2 (RP2350A: pines 30-47 no-op inocuo) y Metro/cualquier RP2350B (48 GPIO
# reales). Lo board-specific (NeoPixel, CS de PSRAM, LED) NO va aquí: /sys/board.json.
list(APPEND PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_LIST_DIR}/sdk_board")
if (NOT DEFINED PICO_BOARD)
    set(PICO_BOARD "bp_rp2350b")
endif()

# Importamos FREERTOS_KERNEL_PATH del entorno si no se ha pasado por -D.
if ((NOT DEFINED FREERTOS_KERNEL_PATH) AND DEFINED ENV{FREERTOS_KERNEL_PATH})
    set(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})
endif()
if (NOT DEFINED FREERTOS_KERNEL_PATH)
    message(FATAL_ERROR
        "FREERTOS_KERNEL_PATH no definido. Pásalo como:\n"
        "  -DFREERTOS_KERNEL_PATH=C:/path/to/FreeRTOS-Kernel\n"
        "o como variable de entorno.")
endif()
message(STATUS "FREERTOS_KERNEL_PATH = ${FREERTOS_KERNEL_PATH}")

# === Pico SDK ============================================================
# Acepta tanto -DPICO_SDK_PATH=... como la variable de entorno.
if ((NOT DEFINED PICO_SDK_PATH) AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()
if (NOT DEFINED PICO_SDK_PATH)
    message(FATAL_ERROR
        "PICO_SDK_PATH no definido. Pásalo como:\n"
        "  -DPICO_SDK_PATH=C:/path/to/pico-sdk\n"
        "o como variable de entorno.")
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# === Proyecto =============================================================
project(bpvm_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# === FreeRTOS-Kernel =====================================================
# El import cmake del port localiza el kernel via FREERTOS_KERNEL_PATH
# y crea el target INTERFACE library `FreeRTOS-Kernel` + un allocator
# `FreeRTOS-Kernel-Heap4` (heap_4).
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/Community-Supported-Ports/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

# === VM-C sources (re-usamos los del host) ==============================
set(BPVM_C_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(BPVM_C_SRC
    ${BPVM_C_ROOT}/src/bpvm.c
    ${BPVM_C_ROOT}/src/loader.c
    ${BPVM_C_ROOT}/src/interp.c
    ${BPVM_C_ROOT}/src/heap.c
    ${BPVM_C_ROOT}/src/builtins.c
    ${BPVM_C_ROOT}/src/fs_facade.c           # #247: facade file I/O (readFile/writeFile/appendFile/fileExists)
    ${BPVM_C_ROOT}/src/net.c                 # H11 (#241): facade Net — sin backend hasta H11.b/c → RuntimeError atrapable
    ${BPVM_C_ROOT}/src/link.c
    ${BPVM_C_ROOT}/src/scheduler.c
    ${BPVM_C_ROOT}/src/threading.c
    ${BPVM_C_ROOT}/src/exceptions.c
    ${BPVM_C_ROOT}/src/gpio.c
    ${BPVM_C_ROOT}/src/i2c.c
    ${BPVM_C_ROOT}/src/spi.c
    ${BPVM_C_ROOT}/src/uart.c
    ${BPVM_C_ROOT}/src/pulse.c
    ${BPVM_C_ROOT}/src/pwm.c
    ${BPVM_C_ROOT}/src/pico.c
    ${BPVM_C_ROOT}/src/bpvm_neopixel.c   # H7.4: fachada NeoPixel
    ${BPVM_C_ROOT}/src/rtc.c
    ${BPVM_C_ROOT}/src/adc.c
    ${BPVM_C_ROOT}/src/wdt.c
    ${BPVM_C_ROOT}/src/aot_registry.c        # H3 #160: AOT hijack registry
    ${BPVM_C_ROOT}/src/bpvm_aot_helpers.c    # H3 #158: tabla helpers AOT
    # H2 SMP: scheduler multi-worker + comm task. comm_common.c es el
    # ring buffer compartido; comm_pico.c es el backend del comm task
    # sobre FreeRTOS (en vez del pthread del host).
    ${BPVM_C_ROOT}/src/scheduler_smp.c
    ${BPVM_C_ROOT}/src/comm_common.c
    ${CMAKE_CURRENT_LIST_DIR}/comm_pico.c
    # Plataforma: usamos el backend FreeRTOS en vez del pthread del host.
    ${CMAKE_CURRENT_LIST_DIR}/platform_freertos.c
)

# === Firmware executable ================================================
add_executable(bpvm_pico
    main.c
    hello_mod.c
    core_mod.c      # #248: Core (Exception/RuntimeError) embebido
    gpio_mod.c      # stdlib core pre-instalada
    i2c_mod.c       # stdlib core pre-instalada
    spi_mod.c       # stdlib core pre-instalada
    uart_mod.c      # stdlib core pre-instalada
    pulse_mod.c     # stdlib core pre-instalada
    pwm_mod.c       # stdlib core pre-instalada
    pico_mod.c      # stdlib core pre-instalada (info MCU)
    rtc_mod.c       # stdlib core pre-instalada (wall clock)
    adc_mod.c       # stdlib core pre-instalada (ADC del MCU)
    wdt_mod.c       # stdlib core pre-instalada (watchdog)
    timer_mod.c     # stdlib core pre-instalada (Timer.Alarm BP-puro)
    neopixel_mod.c  # H7.4: stdlib Neopixel.Strip (WS2812 vía PIO)
    # NOTA: drivers de dispositivo (PCA9554, BME280, SSD1306, ...) NO
    # se embeben aquí — viajan con la app del usuario al hacer upload
    # desde el IDE. Solo el core de stdlib (Math, IO, Gpio, I2c, ...)
    # va embebido.
    board_desc.c    # H7.3: descriptor de placa (variante A/B + /sys/board.json)
    psram.c         # H7.2.a: detección + init de PSRAM (QMI CS1, APS6404)
    neopixel.c      # H7.4: driver WS2812 (NeoPixel) vía PIO
    fs.c            # FP2 step 1+3: filesystem RAM + persistencia flash
    repl.c          # FP2 step 2: REPL sobre USB CDC (protocolo texto legacy)
    repl_v1.c       # H1 fase A: dispatcher wire BPVM v1 JSON
    json_min.c      # H1 fase A: parser JSON minimal (~200 líneas)
    wire_v1.c       # H1 fase A: framing line-delimited + bulk inline
    log.c           # FP2 step 4: log persistente
    flash_lock.c    # H2 #153: ventana XIP-safe (single + dual-core)
    bench.c         # H3 #159: micro-bench AOT ceiling (fib_native vs BP)
    aot_funcs.c              # H3 #160: agregador del registro AOT
    aot_Bench.c              # H3 #157: AOT linkado (estado working confirmado)
    ${BPVM_C_ROOT}/src/mdn_loader.c   # H3 #158 / H9.5: loader .mdn COMPARTIDO (Pico+STM32); trazas via bpvm_mdn_log (impl fuerte en aot_funcs.c)
    embedded_bench_mdn.c     # H3 #158 fase C v2: blob .mdn limpio (124 bytes)
    ${BPVM_C_SRC}
)

# H7.4 — ensambla el programa PIO del WS2812 (NeoPixel) que trae la pico-sdk
# (pico_status_led/ws2812.pio, BSD) con pioasm → genera ws2812.pio.h en el build,
# que incluye neopixel.c. Así no escribimos el ensamblador PIO a mano.
pico_generate_pio_header(bpvm_pico ${PICO_SDK_PATH}/src/rp2_common/pico_status_led/ws2812.pio)

# H3 #158 fase B — compilar el AOT generado como position-independent.
# Esto es el paso preparatorio para extraer .text al .mdn. Si la
# perf se mantiene (~84ms para fib(28)), el código PIC vale tal cual
# para el loader dinámico de fase C/D.
set_source_files_properties(
    aot_Bench.c
    PROPERTIES
    COMPILE_FLAGS "-fpic -fno-jump-tables"
)

target_include_directories(bpvm_pico PRIVATE
    ${BPVM_C_ROOT}/include
    ${BPVM_C_ROOT}/src                          # aot_registry.h (H3)
    ${CMAKE_CURRENT_LIST_DIR}                   # FreeRTOSConfig.h + embedded_mods.h
)

target_link_libraries(bpvm_pico PRIVATE
    pico_stdlib
    hardware_gpio
    hardware_i2c         # I2c.* backend
    hardware_spi         # Spi.* backend
    hardware_uart        # Uart.* backend
    hardware_pwm         # Pulse.* backend (PWM input-gate edge counting)
    hardware_adc         # Pico.tempC() — sensor de temperatura interno
    pico_unique_id       # Pico.uniqueId() — flash chip ID
    hardware_flash       # FP2 step 3: erase/program de la región del FS
    hardware_sync        # save_and_disable_interrupts/restore_interrupts
    hardware_pio         # H7.4: NeoPixel WS2812 vía PIO
    # NOTA: NO añadir pico_flash — rompió USB CDC en FP2-fail. Para el
    # lockout dual-core (#153) usamos pico_multicore directamente (abajo),
    # NO pico_flash.
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)

# H2 #153 — número de cores. Default 1 (firmware que se envía hoy).
# Activar dual-core con:  cmake .. -DBPVM_PICO_NUM_CORES=2
# Eso inyecta configNUMBER_OF_CORES=2 (FreeRTOSConfig.h lo lee) y enlaza
# pico_multicore para el flash lockout. Ver docs/SMP_ARCH.md runbook.
if (NOT DEFINED BPVM_PICO_NUM_CORES)
    set(BPVM_PICO_NUM_CORES 1)
endif()
message(STATUS "BPVM_PICO_NUM_CORES = ${BPVM_PICO_NUM_CORES}")
target_compile_definitions(bpvm_pico PRIVATE
    BPVM_PICO_NUM_CORES=${BPVM_PICO_NUM_CORES}
)
if (BPVM_PICO_NUM_CORES GREATER 1)
    target_link_libraries(bpvm_pico PRIVATE pico_multicore)
endif()

# stdio: usar USB CDC, deshabilitar UART (libera GP0/GP1)
pico_enable_stdio_usb(bpvm_pico  1)
pico_enable_stdio_uart(bpvm_pico 0)

# Generar .uf2 + .elf + .bin
pico_add_extra_outputs(bpvm_pico)

# Flags útiles para diagnóstico durante el bring-up:
target_compile_options(bpvm_pico PRIVATE
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
    -Os
)

# stdio_usb: no esperar al DTR — el banner del REPL se repite 3× con
# delays, así que el IDE recoge al menos uno de los 3 al conectarse.
# Reducir el wait acelera el reset y elimina el "se queda 5 s parado".
target_compile_definitions(bpvm_pico PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=0
)

# H2 Pico SMP — opt-in al scheduler multi-worker.
# Por defecto NO se define ⇒ repl_v1.c usa bpvm_run() (legacy).
# Activar con:
#   cmake .. -DBPVM_PICO_SMP_WORKERS=1   # validar arquitectura, sin paralelismo
#   cmake .. -DBPVM_PICO_SMP_WORKERS=2   # paralelismo real (#153 cierra TX-excl)
if (DEFINED BPVM_PICO_SMP_WORKERS)
    message(STATUS "BPVM_PICO_SMP_WORKERS = ${BPVM_PICO_SMP_WORKERS}")
    target_compile_definitions(bpvm_pico PRIVATE
        BPVM_PICO_SMP_WORKERS=${BPVM_PICO_SMP_WORKERS}
    )
endif()

# H2 #153 — trazas a flash del comm task para depurar el cuelgue SMP.
# Migas que sobreviven a un reset; se leen con el comando LOG.
# Activar:  cmake .. -DBPVM_PICO_SMP_TRACE=1
if (DEFINED BPVM_PICO_SMP_TRACE)
    message(STATUS "BPVM_PICO_SMP_TRACE = ${BPVM_PICO_SMP_TRACE}")
    target_compile_definitions(bpvm_pico PRIVATE BPVM_PICO_SMP_TRACE=1)
endif()

# H2 #153 — indicador LED de boot para diagnosticar cuelgues de arranque
# (sobre todo dual-core, cuando el USB ni enumera). LED ON antes del
# scheduler, OFF al llegar al REPL. Activar: cmake .. -DBPVM_PICO_BOOT_LED=1
if (DEFINED BPVM_PICO_BOOT_LED)
    message(STATUS "BPVM_PICO_BOOT_LED = ${BPVM_PICO_BOOT_LED}")
    target_compile_definitions(bpvm_pico PRIVATE BPVM_PICO_BOOT_LED=1)
endif()
