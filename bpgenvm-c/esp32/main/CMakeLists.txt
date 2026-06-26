# Componente `main` — H4.3. Core VM portable + backend ESP32 + wire v1
# (UART0) + FS RAM + REPL. El parser JSON (pico/json_min.c) es C portable
# y se reutiliza directamente.
idf_component_register(
    SRCS
        "main.c"
        "platform_esp32.c"
        "wire_v1.c"
        "fs_ram.c"
        "repl_esp32.c"
        "gpio_esp32.c"
        "esp32_mods.c"
        "json_min.c"
        # --- core VM (portable C99) ---
        "../../src/bpvm.c"
        "../../src/loader.c"
        "../../src/interp.c"
        "../../src/heap.c"
        "../../src/builtins.c"
        "../../src/fs_facade.c"        # #247: facade file I/O (backend ESP32 = fs_ram.c::fs_register_bpvm)
        "../../src/net.c"              # H11 (#241): facade Net — sin backend hasta H11.c (lwIP) → RuntimeError atrapable
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
        "../../src/bpvm_neopixel.c"   # fachada no-op sin backend (H7.4)
    INCLUDE_DIRS
        "."
        "../../include"
        "../../src"
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
