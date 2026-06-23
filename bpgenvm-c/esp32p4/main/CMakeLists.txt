# ESP32-P4 — VM.1: bring-up de red + núcleo de la VM-C ejecutando Hello.mod.
# El core (../../src) es C99 portable; la plataforma (FreeRTOS/esp_timer) se
# reutiliza del S3 (../../esp32/main/platform_esp32.c). Las fachadas HW van
# como stubs (sin backend todavía) — el Hello no usa HW.
idf_component_register(
    SRCS
        "main.c"
        "hello_mod.c"
        "../../esp32/main/platform_esp32.c"
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
    REQUIRES
        esp_eth
        esp_netif
        esp_event
        esp_timer
        lwip
)
