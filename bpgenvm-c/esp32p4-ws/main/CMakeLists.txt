# PASO 1: nucleo de la VM-C (portable C99) + plataforma ESP32 + Hello.mod embebido.
# Conjunto de fuentes = el del S3 (../../esp32/main) SIN el wire/REPL (wire_v1, repl_esp32,
# esp32_mods, json_min). Sin GUI (no se define BPVM_GUI -> builtins.c no arrastra gui.c).
# Reutiliza las MISMAS fuentes por path relativo; el esp32p4/ y el esp32/ quedan intactos.
idf_component_register(
    SRCS
        "main.c"
        "../../esp32/main/platform_esp32.c"
        "../../esp32/main/gpio_esp32.c"       # backend HW (el Hello no lo usa; resuelve simbolos de las fachadas)
        "../../esp32/main/fs_ram.c"           # FS en RAM + persistencia flash (particion bpfs)
        "../../esp32/main/esp32_mods.c"       # PASO 3: stdlib embebida (Core..Timer) -> /lib
        "../../esp32/main/wire_v1.c"          # PASO 4: wire v1 por UART0 (pines 37/38 board-aware en P4)
        "../../esp32/main/repl_esp32.c"       # PASO 4: dispatcher del wire (HELLO/PUT/RUN/KILL)
        "../../esp32/main/json_min.c"         # PASO 4: parser JSON del wire
        "gui_display_dsi.c"                   # PASO 6: display MIPI-DSI ST7701 + backlight + GT911 + costura LVGL
        # --- core VM (portable C99) ---
        "../../src/bpvm.c"
        "../../src/loader.c"
        "../../src/interp.c"
        "../../src/heap.c"
        "../../src/builtins.c"
        "../../src/gui.c"                     # PASO 6: modelo GUI + widgets LVGL (Gui.* de BP; bajo BPVM_GUI)
        "../../src/fs_facade.c"
        "../../src/crc32.c"
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
        # --- fachadas HW (sin backend real activo; presentes para el linker) ---
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
        esp_timer
        esp_driver_uart
        esp_partition
        esp_driver_gpio
        esp_driver_spi
        esp_driver_i2c
        esp_driver_ledc
        esp_adc
        esp_driver_pcnt
        spi_flash
        esp_hw_support
        esp_lcd          # PASO 6: MIPI-DSI (esp_lcd_new_dsi_bus + panel ops + st7701)
        lvgl             # PASO 6: LVGL 9.2.2 vendorizada (via EXTRA_COMPONENT_DIRS)
)
