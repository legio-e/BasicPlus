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
    ${BPVM_C_ROOT}/src/fs_lfs.c              # H2-B2: backend littlefs de la fachada (portable)
    ${BPVM_C_ROOT}/third_party/littlefs/lfs.c        # H2: motor littlefs v2.11.3 (in-tree)
    ${BPVM_C_ROOT}/third_party/littlefs/lfs_util.c
    ${BPVM_C_ROOT}/src/crc32.c               # paso 4 cierre: CRC del FS en LS (== java.util.zip.CRC32)
    # H9 — gestión de placa (entorno + particiones), núcleo portable compartido
    # con el boardsim de host; el firmware solo aporta la cintura de flash + el
    # puente (board_mgr_pico.c, abajo).
    ${BPVM_C_ROOT}/src/bpvm_env.c            # bloque de env A/B (formato + CRC + A/B)
    ${BPVM_C_ROOT}/src/bpvm_part.c           # particiones fijas, offsets derivados
    ${BPVM_C_ROOT}/src/bpvm_boot.c           # máquina de estados de arranque + STATE
    ${BPVM_C_ROOT}/src/bpvm_sqlmem.c         # V5/H: regla del bloque de memoria de la BD
    ${BPVM_C_ROOT}/src/bpvm_bios.c           # V5/I: tabla BIOS prestada al pack nativo
    ${BPVM_C_ROOT}/src/bpvm_bios_fs.c        # V5/H2: sus ranuras de ficheros (portable)
    ${BPVM_C_ROOT}/src/bpvm_npack.c          # V5/I: escalera de validacion del pack nativo
    # V5/H1: dialogo con la tarjeta SD en modo SPI. Portable de nacimiento (va
    # sobre las fachadas bpvm_spi_*/bpvm_gpio_*, sin un solo #ifdef), pero SOLO
    # se da de alta AQUI: criterio de Eduardo (8-ago) — a fondo en una placa, y
    # las demas en bloque cuando la cadena este probada. Darlo de alta en las 5
    # ahora seria cometer los mismos errores tres veces y corregirlos tres.
    ${BPVM_C_ROOT}/src/bpvm_sd.c             # V5/H1: SD por SPI (por ahora solo aqui)
    # V5/H2: la SD como sistema de ficheros. Mismo criterio de alta que bpvm_sd.c
    # — una sola placa hasta que la cadena este probada de punta a punta.
    ${BPVM_C_ROOT}/src/fs_fat.c              # backend FAT de la fachada + cintura de disco
    ${BPVM_C_ROOT}/third_party/fatfs/ff.c            # H2: motor FatFs R0.16 (in-tree)
    ${BPVM_C_ROOT}/third_party/fatfs/ffunicode.c     # tabla de la CP850 (nombres largos)
    ${CMAKE_CURRENT_LIST_DIR}/pack_pico.c    # V5/I: el cargador de packs de ESTA placa
    ${CMAKE_CURRENT_LIST_DIR}/bios_pico.c    # V5/I: la tabla de ESTA placa
    ${BPVM_C_ROOT}/src/bpvm_util.c           # #338/#353: zona de rascar + canal de diagnostico
    ${BPVM_C_ROOT}/src/bpvm_bmgr.c           # board manager (compone env+part)
    ${BPVM_C_ROOT}/src/bpvm_bmgr_wire.c      # adaptador de wire (replies = las del sim)
    ${BPVM_C_ROOT}/src/bpvm_dbg_wire.c       # #326: ramo de depuracion (salio de AQUI; ahora comun)
    ${BPVM_C_ROOT}/src/bpvm_pack.c           # H3 Packs V4: core portable de packs (lo llama bmgr_wire)
    ${BPVM_C_ROOT}/src/net.c                 # H11 (#241): facade Net — sin backend hasta H11.b/c → RuntimeError atrapable
    ${BPVM_C_ROOT}/src/link.c
    ${BPVM_C_ROOT}/src/scheduler.c
    ${BPVM_C_ROOT}/src/threading.c
    ${BPVM_C_ROOT}/src/events.c
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
    fs_lfs_pico.c   # H2-B2: littlefs (descriptor + cintura flash_range_* + shim fs.h)
    repl_v1.c       # el REPL: wire BPVM v1 (bucle + dispatcher). #305 retiró el
                    # protocolo texto legacy de repl.c, que era del Pico y sólo
                    # del Pico (ESP32/STM32 nacieron ya wire-v1).
    board_mgr_pico.c # H9: cintura de flash del env (A/B) + puente a bpvm_bmgr_wire
    json_min.c      # H1 fase A: parser JSON minimal (~200 líneas)
    wire_v1.c       # H1 fase A: framing line-delimited + bulk inline
    log.c           # FP2 step 4: log persistente
    flash_lock.c    # H2 #153: ventana XIP-safe (single + dual-core)
    aot_funcs.c              # hook de trazas del cargador .mdn (H13 h12: ya no precarga nada)
    ${BPVM_C_ROOT}/src/mdn_loader.c   # H3 #158 / H9.5: loader .mdn COMPARTIDO (Pico+STM32); trazas via bpvm_mdn_log (impl fuerte en aot_funcs.c)
    ${BPVM_C_SRC}
)

# H7.4 — ensambla el programa PIO del WS2812 (NeoPixel) que trae la pico-sdk
# (pico_status_led/ws2812.pio, BSD) con pioasm → genera ws2812.pio.h en el build,
# que incluye neopixel.c. Así no escribimos el ensamblador PIO a mano.
pico_generate_pio_header(bpvm_pico ${PICO_SDK_PATH}/src/rp2_common/pico_status_led/ws2812.pio)

target_include_directories(bpvm_pico PRIVATE
    ${BPVM_C_ROOT}/include
    ${BPVM_C_ROOT}/src                          # aot_registry.h (H3)
    ${BPVM_C_ROOT}/third_party/littlefs         # H2-B2: lfs.h
    # V5/H2: ff.h/diskio.h. VA DESPUES de include/ a proposito: el ffconf.h que
    # manda es el NUESTRO. El del paquete se borro al vendorizar justo para que
    # esto no dependa del orden — ver third_party/README.md.
    ${BPVM_C_ROOT}/third_party/fatfs
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
    pico_rand            # #347: get_rand_32() — TRNG del RP2350 para random()
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
    LFS_NO_DEBUG
    LFS_NO_WARN
    LFS_NO_ERROR
    LFS_NO_MALLOC   # H2-B2: todos los buffers de littlefs son estaticos
)
if (BPVM_PICO_NUM_CORES GREATER 1)
    target_link_libraries(bpvm_pico PRIVATE pico_multicore)
endif()

# stdio: usar USB CDC, deshabilitar UART (libera GP0/GP1)
pico_enable_stdio_usb(bpvm_pico  1)
pico_enable_stdio_uart(bpvm_pico 0)

# H9 — layout de flash en 3 ZONAS: linker script propio (FLASH_BOOT 64K fijos |
# hueco del env 16K que el UF2 NO graba | FLASH_MAIN hasta el FS en 0x200000).
# Si una zona se desborda, el link FALLA (region overflow) — el crecimiento de
# la imagen grita en build. Ver bp_memmap.ld + flash_layout.h + docs/H9_KERNEL_CAPAS.md.
pico_set_linker_script(bpvm_pico ${CMAKE_CURRENT_LIST_DIR}/bp_memmap.ld)

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

# #292 — BRING-UP ESCALONADO. Para placas que no enumeran: el USB no puede
# contarnos nada, así que en vez de deducir, se bisecta. Cada nivel añade UNA
# pieza del arranque y se queda ahí, imprimiendo por USB. La primera que no
# enumere es la culpable.
#
#   1 = SOLO stdio_init_all + bucle de prints. Nada más: sin LED, sin log, sin
#       FS, sin board_desc, sin PSRAM, sin scheduler. Responde a la única
#       pregunta que importa cuando estás ciego: ¿esta placa enumera con
#       nuestra imagen, sí o no?
#   2 = todo main(): led_init + stdio + log_init + backends, y PARA antes del
#       scheduler. log_init ya toca flash → separa "la flash mata" de "el FS mata".
#   3 = 2 + scheduler + fs_init()        (descriptor + montar/FORMATEAR littlefs)
#   4 = 3 + board_desc_init()            (JEDEC + board.json + sondeo QMI PSRAM)
#
# Los niveles 3/4 paran DENTRO de vm_task, no en main(): fs_init() usa
# bpvm_flash_lock, que en dual-core hace multicore_lockout_start_blocking() —
# antes del scheduler el core 1 no está armado como víctima y se colgaría
# esperando. Cada pieza se prueba donde de verdad corre.
#
# Ojo con el LED como testigo: led_init/led_set están a fuego en
# PICO_DEFAULT_LED_PIN (25) — en la Metro el 25 es el NeoPixel y el LED es el
# 23. Por eso el nivel 1 NO toca el LED: una variable menos.
#
# Activar:  cmake .. -DBPVM_PICO_BRINGUP=1
if (DEFINED BPVM_PICO_BRINGUP)
    message(STATUS "BPVM_PICO_BRINGUP = ${BPVM_PICO_BRINGUP}  (imagen de diagnóstico, NO de uso)")
    target_compile_definitions(bpvm_pico PRIVATE
        BPVM_PICO_BRINGUP=${BPVM_PICO_BRINGUP}
        # El stdio_usb del SDK TIRA la salida al suelo si el host no assertó DTR
        # (stdio_usb_out_chars: `if (stdio_usb_connected())`, y eso es
        # tud_cdc_connected() = DTR). Un testigo que sólo habla si el terminal
        # levanta DTR no sirve para diagnosticar: si no ves nada no sabes si la
        # placa está muerta o si tu terminal no assertó una señal. Con esto,
        # stdio_usb_connected() pasa a ser tud_ready() = "el USB está listo".
        # Sólo en la imagen de bring-up; el firmware normal no cambia.
        PICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1
    )
endif()
