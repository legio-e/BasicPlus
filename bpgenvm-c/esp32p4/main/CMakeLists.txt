# ESP32-P4 — VM.1: bring-up de red + núcleo de la VM-C ejecutando Hello.mod.
# El core (../../src) es C99 portable; la plataforma (FreeRTOS/esp_timer) se
# reutiliza del S3 (../../esp32/main/platform_esp32.c). Las fachadas HW van
# como stubs (sin backend todavía) — el Hello no usa HW.
#
# Transporte del wire (#138): UART0 por DEFECTO (imprescindible; el IDE conecta al
# bridge USB-UART, sirve también a placas sin Ethernet). TCP/Ethernet opcional para
# placas con PHY (ESP32-P4-Function-EV) con:  idf.py -DBPVM_P4_WIRE=tcp build
# Convivencia simultánea de varios transportes a la vez = V4.
if(NOT DEFINED BPVM_P4_WIRE)
    set(BPVM_P4_WIRE "uart")  # default = wire por UART0/COM14 (#138 resuelto); Ethernet
endif()                       # queda solo para logs. TCP completo con -DBPVM_P4_WIRE=tcp
if(BPVM_P4_WIRE STREQUAL "tcp")
    set(WIRE_TRANSPORT_SRC "wire_v1_tcp.c")                   # I/O del wire sobre TCP (lwIP)
else()
    set(WIRE_TRANSPORT_SRC "../../esp32/main/wire_v1.c")      # I/O del wire sobre UART0 (reúso del S3)
endif()

idf_component_register(
    SRCS
        "main.c"
        "../../esp32/main/esp32_mods.c"       # P6 imagen única: stdlib embebida COMPARTIDA con el S3 (14 mods)
        "${WIRE_TRANSPORT_SRC}"               # VM.3: capa de I/O del wire v1 (TCP o UART0 según BPVM_P4_WIRE)
        "p4_board_id.c"                       # VM.3 pulido: identidad de placa esp32p4 (override fuerte de repl_board_id)
        "aot_funcs_p4.c"                      # H4 AOT RISC-V (Hito 1): override fuerte de esp_aot_register (SOLO P4)
        "aot_Bench.c"                         # H4 AOT RISC-V (Hito 1): fib nativo (mismo .c portable que el Pico, toolchain RISC-V)
        "gui_display_dsi.c"                   # G3: display MIPI-DSI (panel EK79007 + backlight LEDC)
        "../../esp32/main/repl_esp32.c"       # VM.3: dispatcher wire v1 REUTILIZADO (agnóstico del transporte)
        "../../esp32/main/json_min.c"         # VM.3: parser JSON del wire (C portable)
        "../../esp32/main/board_mgr_esp32.c"  # H9: gestion de placa (env bpenv + climb + STATE/ENV/PART) — compartido S3
        "../../esp32/main/platform_esp32.c"
        "../../esp32/main/gpio_esp32.c"       # H14: backends HW del P4 = REÚSO del S3 (GPIO/UART/SPI/I2C, ESP-IDF estándar)
        "../../esp32/main/fs_lfs_esp32.c"    # H2-B3: littlefs (reutilizado del S3) — reemplaza fs_ram.c
        # --- V5/H6: la SD por SDMMC ---------------------------------------
        "blk_sdmmc_p4.c"                      # H6 paso 2: cintura SDMMC sobre sdmmc_host
        "../../src/bpvm_blk.c"                # H6 paso 1: contrato de bloque + tabla de particiones
        "../../src/bpvm_blk_sdmmc_cfg.c"      # H6 paso 2: la linea `sd` del ENV (portable, test-sdio)
        "../../src/fs_fat.c"                  # H2: backend FAT de la fachada (el MISMO que la Metro)
        "../../third_party/fatfs/ff.c"        # H2: motor FatFs R0.16 — el NUESTRO, no el del IDF
        "../../third_party/fatfs/ffunicode.c" # tabla de la CP850 (nombres largos)
        # --- núcleo VM (portable C99) ---
        "../../src/bpvm.c"
        "../../src/loader.c"
        "../../src/interp.c"
        "../../src/heap.c"
        "../../src/builtins.c"
        "../../src/gui.c"                     # G6: modelo GUI + widgets LVGL (Gui.* de BasicPlus)
        "../../src/fs_facade.c"
        "../../src/fs_lfs.c"                  # H2-B3: backend littlefs de la fachada (portable)
        "../../third_party/littlefs/lfs.c"    # H2: motor littlefs v2.11.3 (in-tree)
        "../../third_party/littlefs/lfs_util.c"
        "../../src/crc32.c"                   # paso 4 cierre: CRC del FS en LS (== java.util.zip.CRC32)
        # --- H9 kernel por capas (portable C99, host-testable) ---
        "../../src/bpvm_env.c"                # H9: bloque de env A/B
        "../../src/bpvm_part.c"               # H9: particiones (FS+Packs, un mando)
        "../../src/bpvm_boot.c"               # H9: maquina de estados del arranque
        "../../src/bpvm_sqlmem.c"             # V5/H: regla del bloque de memoria de la BD
        "../../src/bpvm_bios.c"               # V5/I: tabla BIOS prestada al pack nativo
        "../../src/bpvm_npack.c"              # V5/I: escalera de validacion del pack nativo
        "../../src/bpvm_mdn_scan.c"           # V5/H4: buscar el .mdn (FS + pack) en UN sitio
        "../../src/bpvm_util.c"        # #338/#353: zona de rascar + canal de diagnostico
        "../../src/bpvm_bmgr.c"               # H9: board manager (compone env+part)
        "../../src/bpvm_bmgr_wire.c"          # H9: nucleo del protocolo STATE/ENV/PART
        "../../src/bpvm_pack.c"               # H3 Packs V4: core portable de packs (lo llama bmgr_wire)
        "../../src/net.c"
        "../../src/link.c"
        "../../src/scheduler.c"
        "../../src/events.c"           # H5.c: cola de eventos + inyección del handler entre quanta
        "../../src/bpvm_dbg_wire.c"   # #326: ramo de depuracion del wire (portable, ex-Pico)
        "../../src/bpvm_log.c"         # log post-mortem: núcleo portable (idéntico en las 3 familias)
        "../../esp32/main/log_esp32.c" # ... y su cintura ESP32 (esp_timer + bpenv sector 2) — compartida S3
        "../../src/threading.c"
        "../../src/exceptions.c"
        "../../src/aot_registry.c"
        "../../src/bpvm_aot_helpers.c"
        "../../src/mdn_loader.c"              # H4 AOT RISC-V (Hito 2): loader .mdn dinámico (SOLO P4)
        "../../src/scheduler_smp.c"
        "../../src/comm_common.c"
        "../../src/comm_host.c"
        # --- fachadas HW (backend ESP-IDF vía gpio_esp32.c; ADC/PWM aún stub) ---
        "../../src/gpio.c"
        "../../src/i2c.c"
        "../../src/spi.c"
        "../../src/uart.c"
        "../../src/pulse.c"
        "../../src/pwm.c"
        "../../src/pico.c"
        "../../src/rtc.c"
        "../../src/adc.c"
        "../../src/wdt.c"
        "../../src/bpvm_neopixel.c"
    INCLUDE_DIRS
        "."
        "../../include"
        "../../src"
        "../../third_party/littlefs"   # H2-B3: lfs.h
        "../../third_party/fatfs"      # V5/H6: ff.h (nuestro ffconf.h sale de ../../include)
        "../../esp32/main"
    REQUIRES
        esp_eth
        esp_netif
        esp_event
        esp_timer
        lwip
        esp_partition
        spi_flash        # INFO (repl_esp32): esp_flash_get_size (flash real montada)
        esp_hw_support   # INFO (repl_esp32): esp_mac.h (uniqueId desde efuse)
        esp_mm           # H4 AOT RISC-V: esp_cache_msync (sync I/D-cache del .mdn en RAM exec)
        esp_lcd          # G3: MIPI-DSI (esp_lcd_new_dsi_bus + panel ops)
        esp_driver_ledc  # G3: PWM del backlight (GPIO26)
        lvgl             # G4: LVGL 9.2.2 vendorizada (via EXTRA_COMPONENT_DIRS)
        esp_driver_i2c   # G5: bus I2C del táctil GT911 (i2c_new_master_bus) + backend I2C BP (H14)
        esp_driver_gpio  # H14: backend GPIO del P4 (driver/gpio.h)
        esp_driver_uart  # H14: backend UART del P4 (driver/uart.h)
        esp_driver_spi   # H14: backend SPI del P4 (driver/spi_master.h)
        esp_adc          # H14: backend ADC del P4 (esp_adc oneshot)
        esp_driver_tsens # H14: sensor de temperatura interno (Pico.tempC)
        esp_driver_pcnt  # H14: backend contador de pulsos (PCNT)
        esp_driver_sdmmc # V5/H6: sdmmc_host + sdmmc_cmd (la tarjeta por SDIO)
)

# H2-B3: littlefs — buffers estáticos (LFS_NO_MALLOC) + trazas silenciadas.
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LFS_NO_DEBUG LFS_NO_WARN LFS_NO_ERROR LFS_NO_MALLOC)
# fs_lfs_esp32.c: snprintf de paths acotado por el FS (falso positivo de -Wformat-truncation con -Werror).
set_source_files_properties("../../esp32/main/fs_lfs_esp32.c" PROPERTIES COMPILE_FLAGS -Wno-format-truncation)

# Activa el camino TCP/Ethernet en main.c (por defecto = UART0, sin este define).
if(BPVM_P4_WIRE STREQUAL "tcp")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BPVM_P4_WIRE_TCP=1)
endif()
