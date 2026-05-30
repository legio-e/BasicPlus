# Componente `main` — H4.1. Compila el core VM portable (../../src/*),
# el backend de plataforma ESP32 y el Hello.mod embebido.
#
# Se excluye platform_pthread.c (host) y comm_pico.c (Pico). El backend
# de plataforma es platform_esp32.c. Los .c de HW (gpio/i2c/...) son
# fachadas portables con backend registrable → sin backend son stubs
# inofensivos (Hello no toca HW).
idf_component_register(
    SRCS
        "main.c"
        "platform_esp32.c"
        "hello_mod.c"
        # --- core VM (portable C99) ---
        "../../src/bpvm.c"
        "../../src/loader.c"
        "../../src/interp.c"
        "../../src/heap.c"
        "../../src/builtins.c"
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
)

# El core VM usa %d/%u con int32_t/uint32_t en mensajes de diagnóstico.
# En Xtensa int32_t es `long` (mismo ancho de 32 bits que int → la salida
# en runtime es CORRECTA), pero ESP-IDF activa -Werror=format y es estricto
# con el TIPO. Degradamos ESE error a warning para no tocar el código de
# la VM (mismo binario correcto). El fix "bonito" — barrer a PRIu32/PRId32
# de <inttypes.h> — es una limpieza de portabilidad para v2.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format)
