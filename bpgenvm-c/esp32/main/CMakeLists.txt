# Componente `main` — H4.3. Core VM portable + backend ESP32 + wire v1
# (UART0) + FS RAM + REPL. El parser JSON (pico/json_min.c) es C portable
# y se reutiliza directamente.
idf_component_register(
    SRCS
        "main.c"
        "platform_esp32.c"
        "wire_v1.c"
        "fs_lfs_esp32.c"              # H2-B3: littlefs (cintura esp_partition + shim fs.h) — reemplaza fs_ram.c
        "repl_esp32.c"
        "gpio_esp32.c"
        "esp32_mods.c"
        "json_min.c"
        "board_mgr_esp32.c"           # H9: gestion de placa (env bpenv + climb + STATE/ENV/PART)
        # --- core VM (portable C99) ---
        "../../src/bpvm.c"
        "../../src/loader.c"
        "../../src/interp.c"
        "../../src/heap.c"
        "../../src/builtins.c"
        "../../src/fs_facade.c"        # #247: facade file I/O (backend ESP32 = fs_lfs_esp32.c)
        "../../src/fs_lfs.c"                       # H2-B3: backend littlefs de la fachada (portable)
        "../../third_party/littlefs/lfs.c"        # H2: motor littlefs v2.11.3 (in-tree)
        "../../third_party/littlefs/lfs_util.c"
        "../../src/crc32.c"            # paso 4 cierre: CRC del FS en LS (== java.util.zip.CRC32)
        # --- H9 kernel por capas (portable C99, host-testable) ---
        "../../src/bpvm_env.c"         # H9: bloque de env A/B
        "../../src/bpvm_part.c"        # H9: particiones (FS+Packs, un mando)
        "../../src/bpvm_boot.c"        # H9: maquina de estados del arranque
        "../../src/bpvm_bmgr.c"        # H9: board manager (compone env+part)
        "../../src/bpvm_bmgr_wire.c"   # H9: nucleo del protocolo STATE/ENV/PART
        "../../src/bpvm_pack.c"        # H3 Packs V4: core portable de packs (lo llama bmgr_wire)
        "aot_funcs_stub.c"            # H4 AOT: def fuerte no-op de esp_aot_register (S3 sin AOT baked-in)
        "../../src/net.c"              # H11 (#241): facade Net — sin backend hasta H11.c (lwIP) → RuntimeError atrapable
        "../../src/link.c"
        "../../src/scheduler.c"
        "../../src/events.c"           # H5.c: cola de eventos + inyección del handler entre quanta
        "../../src/threading.c"
        "../../src/exceptions.c"
        "../../src/aot_registry.c"
        # H11: aquí vive bpvm_mdn_host_arch(), la ISA que publica el INFO — sale de
        # los MISMOS macros que gatean el loader, así que van juntos (una sola
        # fuente de verdad). El S3 aún no carga .mdn: --gc-sections tira lo que no use.
        "../../src/mdn_loader.c"
        "../../src/bpvm_aot_helpers.c"
        "../../src/scheduler_smp.c"
        "../../src/comm_common.c"
        "../../src/comm_host.c"
        # --- fachadas HW (stubs sin backend) ---
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
        "../../src/bpvm_neopixel.c"   # fachada no-op sin backend (H7.4)
    INCLUDE_DIRS
        "."
        "../../include"
        "../../src"
        "../../third_party/littlefs"   # H2-B3: lfs.h
    REQUIRES
        esp_timer
        esp_driver_uart
        esp_partition
        esp_driver_gpio
        esp_driver_spi   # H16: backend SPI (spi_master)
        esp_driver_i2c   # H16: backend I2C (i2c_master, IDF v6)
        esp_driver_ledc  # H14: backend PWM (LEDC)
        esp_adc          # H14: backend ADC (esp_adc oneshot)
        esp_driver_pcnt  # H14: backend contador de pulsos (PCNT)
        spi_flash        # INFO: esp_flash_get_size (flash real montada)
        esp_hw_support   # INFO: esp_mac.h (uniqueId desde efuse)
)

# H2-B3: littlefs — buffers estáticos (LFS_NO_MALLOC) + trazas silenciadas
# (LFS_NO_DEBUG/WARN/ERROR: por stdout romperían consola/paridad, como en el RP2350).
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LFS_NO_DEBUG LFS_NO_WARN LFS_NO_ERROR LFS_NO_MALLOC)

# fs_lfs_esp32.c: el BFS de fs_list hace snprintf de paths (acotados por la
# estructura del FS, pero GCC no lo ve → -Wformat-truncation). ESP-IDF fuerza
# -Werror (el Pico no) → relajamos SOLO este fichero (mismo código probado en RP2350).
set_source_files_properties(fs_lfs_esp32.c PROPERTIES COMPILE_FLAGS -Wno-format-truncation)
