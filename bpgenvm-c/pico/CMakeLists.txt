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
if (NOT DEFINED PICO_BOARD)
    set(PICO_BOARD "pico2")
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
    ${BPVM_C_ROOT}/src/rtc.c
    ${BPVM_C_ROOT}/src/adc.c
    ${BPVM_C_ROOT}/src/wdt.c
    # Plataforma: usamos el backend FreeRTOS en vez del pthread del host.
    ${CMAKE_CURRENT_LIST_DIR}/platform_freertos.c
)

# === Firmware executable ================================================
add_executable(bpvm_pico
    main.c
    hello_mod.c
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
    # NOTA: drivers de dispositivo (PCA9554, BME280, SSD1306, ...) NO
    # se embeben aquí — viajan con la app del usuario al hacer upload
    # desde el IDE. Solo el core de stdlib (Math, IO, Gpio, I2c, ...)
    # va embebido.
    fs.c            # FP2 step 1+3: filesystem RAM + persistencia flash
    repl.c          # FP2 step 2: REPL sobre USB CDC (protocolo texto legacy)
    repl_v1.c       # H1 fase A: dispatcher wire BPVM v1 JSON
    json_min.c      # H1 fase A: parser JSON minimal (~200 líneas)
    wire_v1.c       # H1 fase A: framing line-delimited + bulk inline
    log.c           # FP2 step 4: log persistente
    ${BPVM_C_SRC}
)

target_include_directories(bpvm_pico PRIVATE
    ${BPVM_C_ROOT}/include
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
    # NOTA: NO añadir pico_flash — rompió USB CDC en FP2-fail.
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)

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
