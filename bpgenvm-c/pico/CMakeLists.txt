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
# Produce: build/bpvm_pico.uf2 — arrastrable a la Pico en modo BOOTSEL. V8/H5: con DOS firmwares dentro, P y M (abajo,
# «Los firmwares»); el .elf de P es build/bpvm_pico.elf, como siempre, y el de M, build/bpvm_pico_m.elf.
# (`ninja bpvm_pico` hace la imagen entera, como siempre.)

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
# V7/H7 (30-sep) - el paso 0 de fundir los backends de FreeRTOS (docs/FUSION_BACKENDS_FREERTOS.md): la Pico con
# el contrato COMUN (src/platform_freertos.c) y solo lo suyo aparte (platform_pico_hw.c), para MEDIRLA contra su
# fichero de siempre. Por defecto, el de siempre.
option(BPVM_PICO_PLATFORM_COMUN "Pico: el backend FreeRTOS comun (medida del paso 0)" OFF)
if (BPVM_PICO_PLATFORM_COMUN)
    set(BPVM_PICO_PLATFORM ${BPVM_C_ROOT}/src/platform_freertos.c ${CMAKE_CURRENT_LIST_DIR}/platform_pico_hw.c)
else()
    set(BPVM_PICO_PLATFORM ${CMAKE_CURRENT_LIST_DIR}/platform_freertos.c)
endif()
set(BPVM_C_SRC
    ${BPVM_C_ROOT}/src/bpvm.c
    ${BPVM_C_ROOT}/src/loader.c
    ${BPVM_C_ROOT}/src/interp.c
    ${BPVM_C_ROOT}/src/heap.c
    ${BPVM_C_ROOT}/src/builtins.c
    ${BPVM_C_ROOT}/src/bpstr_vm.c            # V8 #730: la biblioteca de cadenas (bpstr/)
    ${BPVM_C_ROOT}/src/fs_facade.c           # #247: facade file I/O (readFile/writeFile/appendFile/fileExists)
    ${BPVM_C_ROOT}/src/fs_lfs.c              # H2-B2: backend littlefs de la fachada (portable)
    ${BPVM_C_ROOT}/third_party/littlefs/lfs.c        # H2: motor littlefs v2.11.3 (in-tree)
    ${BPVM_C_ROOT}/third_party/littlefs/lfs_util.c
    ${BPVM_C_ROOT}/src/crc32.c               # paso 4 cierre: CRC del FS en LS (== java.util.zip.CRC32)
    # H9 — gestión de placa (entorno + particiones), núcleo portable compartido
    # con el boardsim de host; el firmware solo aporta la cintura de flash + el
    # puente (board_mgr_pico.c, abajo).
    ${BPVM_C_ROOT}/src/bpvm_log.c            # V6/U1.2: el NUCLEO del log, comun a las 4 imagenes
    ${BPVM_C_ROOT}/src/bpvm_env.c            # bloque de env A/B (formato + CRC + A/B)
    ${BPVM_C_ROOT}/src/bpvm_part.c           # particiones fijas, offsets derivados
    ${BPVM_C_ROOT}/src/bpvm_boot.c           # máquina de estados de arranque + STATE
    ${BPVM_C_ROOT}/src/bpvm_sqlmem.c         # V5/H: regla del bloque de memoria de la BD
    ${BPVM_C_ROOT}/src/bpvm_mem.c            # V6/U6: el planificador de memoria de la VM (una regla, cinco placas)
    ${BPVM_C_ROOT}/src/bpvm_mods.c           # V6/#466: la regla del /lib de la imagen (común a las tres familias)
    ${BPVM_C_ROOT}/src/bpvm_handles.c        # V6/U5: la tabla de handles, en su módulo
    ${BPVM_C_ROOT}/src/bpvm_io.c  # V6/A1: el hilo `io` (segundo hilo de SO: wire + salida por lineas)
    ${BPVM_C_ROOT}/src/bpvm_bios.c           # V5/I: tabla BIOS prestada al pack nativo
    ${BPVM_C_ROOT}/src/bpvm_bios_fs.c        # V5/H2: sus ranuras de ficheros (portable)
    ${BPVM_C_ROOT}/src/bpvm_bios_hal.c       # V7/H6: sus ranuras de HARDWARE (portable: las fachadas de la VM)
    ${BPVM_C_ROOT}/src/bpvm_bus.c            # V7/H6.c G4.a1: los buses SPI/I2C, compartidos (la HAL BP)
    ${BPVM_C_ROOT}/src/bpvm_sonido.c         # V8/H4 (#708): el sonido, lo común de la HAL BP
    ${BPVM_C_ROOT}/src/bpvm_wifi.c           # V8/H3: la WiFi, el enlace, lo común de la HAL BP
    ${BPVM_C_ROOT}/src/bpvm_red.c            # V8/H3: la IP de la placa, lo común
    ${BPVM_C_ROOT}/src/bpvm_npack.c          # V5/I: escalera de validacion del pack nativo
    ${BPVM_C_ROOT}/src/bpvm_npack_carga.c    # V7/H7 (Boot 2, fase 2): el cargador comun de los motores
    ${BPVM_C_ROOT}/src/bpvm_boot2.c          # V7/H7 (Boot 2, fase 4): el arranque comun
    ${BPVM_C_ROOT}/src/bpvm_mdn_scan.c       # V5/H4: buscar el .mdn (FS + pack) en UN sitio
    ${BPVM_C_ROOT}/src/bpvm_nativa.c         # V7/H5 P2b.3: el ejecutor de una native (un hilo, y el testigo)
    ${BPVM_C_ROOT}/src/bpvm_fallo_cm33.c     # V7/H5 P2b.3: el informe de un HardFault, comun con las STM32
    # V5/H1: dialogo con la tarjeta SD en modo SPI. Portable de nacimiento (va
    # sobre las fachadas bpvm_spi_*/bpvm_gpio_*, sin un solo #ifdef), pero SOLO
    # se da de alta AQUI: criterio de Eduardo (8-ago) — a fondo en una placa, y
    # las demas en bloque cuando la cadena este probada. Darlo de alta en las 5
    # ahora seria cometer los mismos errores tres veces y corregirlos tres.
    ${BPVM_C_ROOT}/src/bpvm_sd.c             # V5/H1: SD por SPI (por ahora solo aqui)
    # V5/H6: la frontera entre FatFs y el medio. `bpvm_blk.c` es comun de verdad
    # (el P4 lo usara igual); `bpvm_sd_blk.c` es la implementacion de SPI, que
    # tambien es comun aunque hoy solo la monte esta placa — es PROTOCOLO, no
    # familia. Ver la cabecera de include/bpvm_blk.h.
    ${BPVM_C_ROOT}/src/bpvm_blk.c            # V5/H6: contrato de bloque + tabla de particiones
    ${BPVM_C_ROOT}/src/bpvm_listdir.c        # V5/H6: LIST_DIR comun (salio de repl_v1.c)
    ${BPVM_C_ROOT}/src/bpvm_sd_blk.c         # V5/H6: la SD por SPI cumpliendo ese contrato
    ${BPVM_C_ROOT}/src/bpvm_sd_spi_env.c     # V7/H3 SD.3: la linea `sd` del ENV (SPI) al registro comun
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
    ${BPVM_C_ROOT}/src/destruir.c
    ${BPVM_C_ROOT}/src/exceptions.c
    ${BPVM_C_ROOT}/src/gpio.c
    ${BPVM_C_ROOT}/src/i2c.c
    ${BPVM_C_ROOT}/src/spi.c
    ${BPVM_C_ROOT}/src/uart.c
    ${BPVM_C_ROOT}/src/pulse.c
    ${BPVM_C_ROOT}/src/pwm.c
    ${BPVM_C_ROOT}/src/bpvm_machine.c
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
    # Plataforma: usamos el backend FreeRTOS en vez del pthread del host (arriba: el propio o el comun).
    ${BPVM_PICO_PLATFORM}
)

# === Los firmwares: P y M, en una sola imagen ============================
# V8/H5 (22-oct) — DOS FIRMWARES EN LA MISMA IMAGEN (Eduardo: «Mantenemos la imagen única así que tenemos que hacer
# que con PSRAM se aproveche la RAM adicional pero en la Pico (el RP2350 sin PSRAM) sigue funcionando como antes»).
# De los mismos fuentes salen dos firmwares completos:
#   · bpvm_pico_p (P): el de siempre, con el código en la flash. Sus ficheros se llaman como siempre: bpvm_pico.elf,
#                      .elf.map, .dis, .bin.
#   · bpvm_pico_m (M): el mismo con el código caliente de la VM y el cerrojo de FreeRTOS en la SRAM (lo medido en la
#                      Metro: V8_IDEAS §E5), guardado y enlazado en 0x10100000. Sus ficheros, bpvm_pico_m.*.
# Y el objetivo bpvm_pico es LA IMAGEN: bpvm_pico.uf2, con los dos —la que se graba y la que se publica; `ninja
# bpvm_pico` la sigue haciendo, como dicen los documentos desde V4—. bpvm_pico_p.uf2 y bpvm_pico_m.uf2 son pasos
# intermedios. Arranca P; si el ENV dice psram=1 y la placa es una RP2350B, salta a M al empezar main() (main.c,
# pico_pasar_a_m). La función de abajo es la configuración de siempre, la misma para los dos.
set(BPVM_PICO_FUENTES
    main.c
    pico_mods.c     # V6/U4: la stdlib embebida, GENERADA por scripts/regen_mods.sh (15 mods + Hello). Sólo datos.
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
    ${BPVM_C_ROOT}/src/json_min.c            # V6/U1.1: parser JSON, COMUN a las 4 imagenes
    ${BPVM_C_ROOT}/src/wire_v1_proto.c       # V6/U2.1: los builders JSON del wire, COMUNES
    ${BPVM_C_ROOT}/src/bpvm_repl.c           # V6/U3: los verbos comunes del REPL
    ${BPVM_C_ROOT}/src/bpvm_sd_placa.c       # V7/H3: la SD de la placa (SD_INFO/SD_MOUNT), en el comun
    wire_v1.c       # H1 fase A: framing line-delimited + bulk inline
    log.c           # FP2 step 4: log persistente
    flash_lock.c    # H2 #153: ventana XIP-safe (single + dual-core)
    pico_flash.c    # V7/H7 U1.4: LA cintura de flash (FS, ENV, packs y log; el candado va dentro)
    aot_funcs.c              # hook de trazas del cargador .mdn (H13 h12: ya no precarga nada)
    ${BPVM_C_ROOT}/src/mdn_loader.c   # H3 #158 / H9.5: loader .mdn COMPARTIDO (Pico+STM32); trazas via bpvm_mdn_log (impl fuerte en aot_funcs.c)
    ${BPVM_C_SRC}
)

# H2 #153 — número de cores. Inyecta configNUMBER_OF_CORES (FreeRTOSConfig.h lo lee) y,
# con dos, enlaza pico_multicore para el flash lockout. Ver docs/SMP_ARCH.md runbook.
# V7/H12 (6-oct) — por defecto, DOS: la imagen de V7 de la Pico 2 y la Metro (Eduardo: «En la
# Pico y Metro, 2 nucleos y 2 runs»), validada en la etapa 3 de H12. Los runs por defecto
# siguen a los núcleos (bpvm_boot2_repartir). La de un núcleo, la vuelta atrás grabando:
#     cmake .. -DBPVM_PICO_NUM_CORES=1
# (sin grabar: `vm.cores=1` y `vm.runs=1` en el ENV).
if (NOT DEFINED BPVM_PICO_NUM_CORES)
    set(BPVM_PICO_NUM_CORES 2)
endif()
message(STATUS "BPVM_PICO_NUM_CORES = ${BPVM_PICO_NUM_CORES}")

# V7/H12, etapa 2a.2 — EL INTERRUPTOR de la imagen: la tarea `run` espera en LA COLA (1, por defecto) o el aviso
# «trabajo», como en la etapa 1 (0: `cmake .. -DBPVM_RUN_EN_COLA=0`; va a la caché: para volver, -DBPVM_RUN_EN_COLA=1).
# Aquí estaba BPVM_PICO_SMP_WORKERS, el opt-in al planificador de varios workers (H2), que ningún build definía:
# desde la 2a.2 el planificador es uno, el de la cola, y el segundo run llega en la 2b.
set(BPVM_RUN_EN_COLA 1 CACHE STRING "H12 2a.2: la tarea run espera en la cola (1) o el aviso trabajo de la etapa 1 (0)")
message(STATUS "BPVM_RUN_EN_COLA = ${BPVM_RUN_EN_COLA}")

# Las imágenes de diagnóstico (los bloques de dentro de la función los explican): lo dicen una vez, aquí.
if (DEFINED BPVM_PICO_SMP_TRACE)
    message(STATUS "BPVM_PICO_SMP_TRACE = ${BPVM_PICO_SMP_TRACE}")
endif()
if (DEFINED BPVM_PICO_BOOT_LED)
    message(STATUS "BPVM_PICO_BOOT_LED = ${BPVM_PICO_BOOT_LED}")
endif()
if (DEFINED BPVM_PICO_BRINGUP)
    message(STATUS "BPVM_PICO_BRINGUP = ${BPVM_PICO_BRINGUP}  (imagen de diagnóstico, NO de uso)")
endif()

# V8/H5 — el .uf2 de cada uno lo hace el bloque del final, con su nombre, y luego los junta: el del SDK se apaga.
set(PICO_NO_UF2 1)

# La configuración de un firmware: T, el objetivo; SALIDA, el nombre de sus ficheros (.elf, .map…); MEMMAP, su script
# de enlace (sale de bp_memmap.ld, al final). El cuerpo es el de siempre, sin sangrar, con `${T}` donde decía `bpvm_pico`.
function(bpvm_rp2350_firmware T SALIDA MEMMAP)
add_executable(${T} ${BPVM_PICO_FUENTES})
set_target_properties(${T} PROPERTIES OUTPUT_NAME ${SALIDA})   # antes de pico_add_extra_outputs, que lo lee

# H7.4 — ensambla el programa PIO del WS2812 (NeoPixel) que trae la pico-sdk
# (pico_status_led/ws2812.pio, BSD) con pioasm → genera ws2812.pio.h en el build,
# que incluye neopixel.c. Así no escribimos el ensamblador PIO a mano. V8/H5: cada firmware, en su carpeta.
pico_generate_pio_header(${T} ${PICO_SDK_PATH}/src/rp2_common/pico_status_led/ws2812.pio
                         OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${T})

target_include_directories(${T} PRIVATE
    ${BPVM_C_ROOT}/include
    ${BPVM_C_ROOT}/src                          # aot_registry.h (H3)
    ${BPVM_C_ROOT}/third_party/littlefs         # H2-B2: lfs.h
    # V5/H2: ff.h/diskio.h. VA DESPUES de include/ a proposito: el ffconf.h que
    # manda es el NUESTRO. El del paquete se borro al vendorizar justo para que
    # esto no dependa del orden — ver third_party/README.md.
    ${BPVM_C_ROOT}/third_party/fatfs
    ${CMAKE_CURRENT_LIST_DIR}                   # FreeRTOSConfig.h + pico_mods.h
)

target_link_libraries(${T} PRIVATE
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

# H2 #153 — número de cores (lo decide el bloque de antes de la función).
target_compile_definitions(${T} PRIVATE
    BPVM_PICO_NUM_CORES=${BPVM_PICO_NUM_CORES}
    LFS_NO_DEBUG
    LFS_NO_WARN
    LFS_NO_ERROR
    LFS_NO_MALLOC   # H2-B2: todos los buffers de littlefs son estaticos
)
if (BPVM_PICO_NUM_CORES GREATER 1)
    target_link_libraries(${T} PRIVATE pico_multicore)
endif()

# stdio: usar USB CDC, deshabilitar UART (libera GP0/GP1)
pico_enable_stdio_usb(${T}  1)
pico_enable_stdio_uart(${T} 0)

# H9 — layout de flash en 3 ZONAS: linker script propio (FLASH_BOOT 64K fijos |
# hueco del env 16K que el UF2 NO graba | FLASH_MAIN hasta el FS en 0x200000).
# Si una zona se desborda, el link FALLA (region overflow) — el crecimiento de
# la imagen grita en build. Ver bp_memmap.ld + flash_layout.h + docs/H9_KERNEL_CAPAS.md.
# V8/H5 — el script de ESTE firmware, sacado de la plantilla bp_memmap.ld (al final).
pico_set_linker_script(${T} ${MEMMAP})

# Generar .elf + .bin + .dis + .map (el .uf2, con su nombre, al final)
pico_add_extra_outputs(${T})

# Flags útiles para diagnóstico durante el bring-up:
target_compile_options(${T} PRIVATE
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
    -Os
)

# stdio_usb: no esperar al DTR — el banner del REPL se repite 3× con
# delays, así que el IDE recoge al menos uno de los 3 al conectarse.
# Reducir el wait acelera el reset y elimina el "se queda 5 s parado".
target_compile_definitions(${T} PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=0
)

# #495 (V7/H8) — EL BÚFER DE COMUNICACIONES de la Pico: la FIFO de recepción del CDC.
# La del SDK es de 64 B a velocidad completa (pico_stdio_usb/include/tusb_config.h), y con
# ella la Pico recibía a 12 KB/s: un paquete USB por cada siesta de 5 ms del cable (medido el
# 1-oct: 64 KB sin escribir nada, 5,2 s; la C5, 0,11 s). A 2 KB cabe la ventana del wire
# —4 trozos de 512 B— y la interrupción de TinyUSB la llena de fondo mientras el cable
# duerme: el RP2350 no tiene DMA para el USB (no hay DREQ del USB), y esa interrupción hace
# su papel. El USB no se desborda: con la FIFO llena, la Pico contesta NAK y el PC reintenta.
target_compile_definitions(${T} PRIVATE
    CFG_TUD_CDC_RX_BUFSIZE=2048
)

# #430 — TOPE de la tabla de handles. Las dos tablas salen del malloc de
# PLATAFORMA, que aquí es SRAM (520 KB en todo el RP2350), no del heap de la VM
# (que puede estar en PSRAM). Doblar a 65536 slots pide 512 KB *y* el realloc
# necesita las viejas a la vez: imposible. Y el fallo del malloc de FreeRTOS
# acaba en vApplicationMallocFailedHook, que PARPADEA PARA SIEMPRE — un cuelgue,
# no un error (así se colgaba la Metro con AotGcRt2, 17-ago). Con el tope, pasar
# de aquí es un OOM atrapable y el programa decide qué hacer.
#   16384 slots = 128 KB las dos tablas ≈ 1/4 de la SRAM. Un programa que
#   necesite más objetos VIVOS a la vez pide más de lo que esta placa da.
target_compile_definitions(${T} PRIVATE
    BPVM_HANDLE_CAP_MAX=16384
    # #447 — el panic del SDK cuenta el MOTIVO por stdout, que aqui es el mismo
    # cable del wire: el IDE lo tira y el cuelgue parece mudo. Con esto el motivo
    # va al log post-mortem, que es donde se puede leer despues del reset.
    PICO_PANIC_FUNCTION=bpvm_pico_panic

    # #448 — QUE `malloc` DEVUELVA NULL EN VEZ DE MATAR LA PLACA.
    #
    # El wrapper del SDK trae PICO_MALLOC_PANIC=1 por defecto y hace
    # `panic("Out of memory")` ANTES de devolver NULL (pico_malloc/malloc.c,
    # check_alloc). O sea que en esta placa `malloc` no falla: mata.
    #
    # Y eso anula lo que V4 construyo a proposito: `bpvm_alloc_raw` maneja el
    # NULL y da paso al OOM ATRAPABLE (#355), que es lo que hacen el host y las
    # otras familias — alli malloc si devuelve NULL. Aqui ese camino no se
    # ejecutaba NUNCA. Medido el 28-ago con `MemT5_Gc`: `PANIC: Out of memory`
    # y la placa muerta y muda, mientras el mismo programa da RuntimeError en el
    # PC.
    #
    # Quitarlo NO deja hueco: el guardian de la pila vive en `_sbrk`
    # (newlib_interface.c), que devuelve -1 al llegar a __StackLimit — y
    # PICO_USE_OPTIMISTIC_SBRK no esta definido, asi que falla limpio. El
    # `check_alloc` del wrapper era una segunda red sobre la primera.
    PICO_MALLOC_PANIC=0
)

# V7/H12, etapa 2a.2 — EL INTERRUPTOR de la imagen (lo decide el bloque de antes de la función).
target_compile_definitions(${T} PRIVATE BPVM_RUN_EN_COLA=${BPVM_RUN_EN_COLA})
# V7/H12 (N1) — el contexto de las natives, uno por TAREA (el puntero por tarea de FreeRTOS: FreeRTOSConfig.h): con dos
# runs, dos natives a la vez no se pisan el suyo. En las dos imágenes (con un núcleo, dos runs también se turnan).
target_compile_definitions(${T} PRIVATE BPVM_AOT_POR_TAREA=1)

# H2 #153 — trazas a flash del comm task para depurar el cuelgue SMP.
# Migas que sobreviven a un reset; se leen con el comando LOG.
# Activar:  cmake .. -DBPVM_PICO_SMP_TRACE=1
if (DEFINED BPVM_PICO_SMP_TRACE)
    target_compile_definitions(${T} PRIVATE BPVM_PICO_SMP_TRACE=1)
endif()

# H2 #153 — indicador LED de boot para diagnosticar cuelgues de arranque
# (sobre todo dual-core, cuando el USB ni enumera). LED ON antes del
# scheduler, OFF al llegar al REPL. Activar: cmake .. -DBPVM_PICO_BOOT_LED=1
if (DEFINED BPVM_PICO_BOOT_LED)
    target_compile_definitions(${T} PRIVATE BPVM_PICO_BOOT_LED=1)
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
    target_compile_definitions(${T} PRIVATE
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
endfunction()

# === V8/H5 (22-oct): los dos firmwares y la imagen que los junta ===========
# Dónde va M: a 1 MB del principio de la flash, la misma dirección para su enlace (abajo) y para el salto (main.c).
set(BPVM_IMAGEN_M_BASE 0x10100000)

# El script de enlace de cada uno, de la plantilla bp_memmap.ld (configure_file: las variables entre arrobas).
# P: el de siempre, pero su zona 3 acaba donde empieza M: si P crece hasta allí, el enlace falla.
set(BP_FLASH_BOOT        0x10000000)
set(BP_FLASH_MAIN        0x10014000)
set(BP_FLASH_MAIN_LARGO  0xEC000)                                     # hasta 0x10100000
set(BP_ZONA1_FIN         "ORIGIN(FLASH_BOOT) + LENGTH(FLASH_BOOT)")
set(BP_ZONA3_INI         "ORIGIN(FLASH_MAIN)")
set(BP_EN_RAM            "")
configure_file(${CMAKE_CURRENT_LIST_DIR}/bp_memmap.ld ${CMAKE_CURRENT_BINARY_DIR}/bp_memmap_p.ld @ONLY)
# M: su «zona 1» en 0x10100000 y su código detrás, hasta las particiones (0x10200000); la zona 2 —el ENV y el log—
# es la de P. Y a la SRAM (van a .data, que el arranque copia de la flash) los ficheros del código caliente: la VM y el
# cerrojo de FreeRTOS. Es la «imagen B» medida en la Metro (V8_IDEAS §E5): el censo de cadenas −25/−29 %, el GC −35 %;
# cuesta ~83 KB de la SRAM, que con la VM en la PSRAM sobra.
set(BP_FLASH_BOOT        0x10100000)
set(BP_FLASH_MAIN        0x10110000)
set(BP_FLASH_MAIN_LARGO  0xF0000)                                     # hasta 0x10200000 (BP_PART_BASE)
set(BP_ZONA1_FIN         0x10010000)
set(BP_ZONA3_INI         0x10014000)
set(BPVM_EN_RAM_M
    */interp.c.obj */builtins.c.obj */heap.c.obj */bpvm_handles.c.obj */scheduler.c.obj    # la VM caliente
    */bpstr_vm.c.obj    # V8 #730: lo de las cadenas, que antes estaba dentro de builtins.c y de su RAM
    */queue.c.obj */tasks.c.obj */list.c.obj */port.c.obj */portasm.c.obj                  # el cerrojo de FreeRTOS
    */platform_freertos.c.obj
    # memcpy, memset, memmove, memcmp y memchr de nuestra newlib (en libg.a, o en libc.a según el compilador): sólo en
    # M, con quien los llama (#720, Eduardo: «SI, las mem* sólo en la Metro»; en P eran un trueque, por los veneers)
    *libg.a:libc_a-mem*.o *libc.a:libc_a-mem*.o)
string(JOIN " " BP_EN_RAM ${BPVM_EN_RAM_M})
configure_file(${CMAKE_CURRENT_LIST_DIR}/bp_memmap.ld ${CMAKE_CURRENT_BINARY_DIR}/bp_memmap_m.ld @ONLY)

# La PAREJA: un número de esta configuración del build, el mismo en los dos. P sólo salta a una M con el suyo: una M
# de otra compilación que se hubiera quedado en la flash no se arranca. Sólo lo ve main.c (al reconfigurar, sólo se
# recompila él).
string(RANDOM LENGTH 8 ALPHABET 0123456789abcdef BPVM_PAREJA)
set_source_files_properties(main.c PROPERTIES COMPILE_DEFINITIONS
    "BPVM_PAREJA=0x${BPVM_PAREJA}u;BPVM_IMAGEN_M_BASE=${BPVM_IMAGEN_M_BASE}u")

# V8 #730: la biblioteca de cadenas (bpgenvm-c/bpstr/, por src/bpstr_vm.c), con -O2 (decidido por Eduardo el 3-nov) y
# sin que GCC cambie su bucle de rellenar por una llamada a memset.
set_source_files_properties(${BPVM_C_ROOT}/src/bpstr_vm.c PROPERTIES COMPILE_OPTIONS
    "-O2;-fno-tree-loop-distribute-patterns")

bpvm_rp2350_firmware(bpvm_pico_p bpvm_pico   ${CMAKE_CURRENT_BINARY_DIR}/bp_memmap_p.ld)
bpvm_rp2350_firmware(bpvm_pico_m bpvm_pico_m ${CMAKE_CURRENT_BINARY_DIR}/bp_memmap_m.ld)
target_compile_definitions(bpvm_pico_m PRIVATE BPVM_IMAGEN_M=1)

# El .uf2 de cada uno (lo mismo que haría el SDK: bpvm_pico_p.uf2 y bpvm_pico_m.uf2), y LA IMAGEN: los dos juntos, en
# bpvm_pico.uf2 (el objetivo bpvm_pico).
pico_init_picotool()
set(BP_UF2_ARGS --family ${PICO_PLATFORM})
if (PICO_RP2350_A2_SUPPORTED)
    list(APPEND BP_UF2_ARGS --abs-block)
endif()
foreach (objetivo bpvm_pico_p bpvm_pico_m)
    add_custom_command(TARGET ${objetivo} POST_BUILD
        COMMAND picotool uf2 convert --quiet $<TARGET_FILE:${objetivo}> ${CMAKE_CURRENT_BINARY_DIR}/${objetivo}.uf2 ${BP_UF2_ARGS}
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${objetivo}.uf2
        COMMAND_EXPAND_LISTS
        VERBATIM)
endforeach()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# Antes de juntarlos, el script comprueba que sean pareja y que la RAM que sobrevive a un reinicio esté en el mismo
# sitio en los dos (por eso los .elf).
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bpvm_pico.uf2
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/juntar_uf2.py
            ${CMAKE_CURRENT_BINARY_DIR}/bpvm_pico.uf2
            ${CMAKE_CURRENT_BINARY_DIR}/bpvm_pico_p.uf2
            ${CMAKE_CURRENT_BINARY_DIR}/bpvm_pico_m.uf2
            $<TARGET_FILE:bpvm_pico_p> $<TARGET_FILE:bpvm_pico_m>
    DEPENDS bpvm_pico_p bpvm_pico_m ${CMAKE_CURRENT_LIST_DIR}/scripts/juntar_uf2.py
    COMMENT "La imagen: P y M, juntos en bpvm_pico.uf2"
    VERBATIM)
add_custom_target(bpvm_pico ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bpvm_pico.uf2)
