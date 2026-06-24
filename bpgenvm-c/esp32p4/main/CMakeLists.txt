# ESP32-P4 — VM.1: bring-up de red + núcleo de la VM-C ejecutando Hello.mod.
# El core (../../src) es C99 portable; la plataforma (FreeRTOS/esp_timer) se
# reutiliza del S3 (../../esp32/main/platform_esp32.c). Las fachadas HW van
# como stubs (sin backend todavía) — el Hello no usa HW.
idf_component_register(
    SRCS
        "main.c"
        "p4_mods.c"
        "wire_v1_tcp.c"                       # VM.3: capa de I/O del wire v1 sobre TCP (sockets lwIP)
        "p4_board_id.c"                       # VM.3 pulido: identidad de placa esp32p4 (override fuerte de repl_board_id)
        "gui_display_dsi.c"                   # G3: display MIPI-DSI (panel EK79007 + backlight LEDC)
        "../../esp32/main/repl_esp32.c"       # VM.3: dispatcher wire v1 REUTILIZADO (agnóstico del transporte)
        "../../esp32/main/json_min.c"         # VM.3: parser JSON del wire (C portable)
        "../../esp32/main/platform_esp32.c"
        "../../esp32/main/fs_ram.c"          # VM.2b: FS en RAM (reutilizado del S3)
        # --- núcleo VM (portable C99) ---
        "../../src/bpvm.c"
        "../../src/loader.c"
        "../../src/interp.c"
        "../../src/heap.c"
        "../../src/builtins.c"
        "../../src/fs_facade.c"
        "../../src/net.c"
        "../../src/link.c"
        "../../src/scheduler.c"
        "../../src/threading.c"
        "../../src/exceptions.c"
        "../../src/aot_registry.c"
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
        "../../src/bpvm_neopixel.c"
    INCLUDE_DIRS
        "."
        "../../include"
        "../../src"
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
        esp_lcd          # G3: MIPI-DSI (esp_lcd_new_dsi_bus + panel ops)
        esp_driver_ledc  # G3: PWM del backlight (GPIO26)
        lvgl             # G4: LVGL 9.2.2 vendorizada (via EXTRA_COMPONENT_DIRS)
        esp_driver_i2c   # G5: bus I2C del táctil GT911 (i2c_new_master_bus)
)
