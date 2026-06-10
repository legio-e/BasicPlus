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
        "json_min.c"
        # --- core VM (portable C99) ---
        "../../src/bpvm.c"
        "../../src/loader.c"
        "../../src/interp.c"
        "../../src/heap.c"
        "../../src/builtins.c"
        "../../src/fs_facade.c"        # #247: facade file I/O (backend ESP32 = fs_ram.c::fs_register_bpvm)
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
    INCLUDE_DIRS
        "."
        "../../include"
        "../../src"
    REQUIRES
        esp_timer
        esp_driver_uart
        esp_partition
        esp_driver_gpio
)
