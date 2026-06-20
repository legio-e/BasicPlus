/*
 * pico.c — fachada de hooks Pico para la VM C.
 *
 * Mismo patrón que pulse.c / pwm.c. Stubs en host con valores
 * razonables ("host" como board, 25.0 °C, etc.) para que el código
 * BP corra en desarrollo sin HW. En el firmware Pico, main.c
 * registra el backend que llama al SDK del Pico.
 */

#include "bpvm_pico.h"
#include <stdio.h>
#include <string.h>

static const bpvm_pico_backend_t* g_backend = NULL;

void bpvm_pico_set_backend(const bpvm_pico_backend_t* backend) {
    g_backend = backend;
}

void bpvm_pico_unique_id(char* buf, size_t len) {
    if (g_backend && g_backend->uniqueId) {
        g_backend->uniqueId(buf, len);
        return;
    }
    /* Stub: ID estable conocido para que tests reproducibles
     * funcionen en host sin sorpresas. */
    const char* stub = "0000000000000000";
    size_t n = strlen(stub);
    if (len == 0) return;
    if (n > len - 1) n = len - 1;
    memcpy(buf, stub, n);
    buf[n] = '\0';
}

void bpvm_pico_board_name(char* buf, size_t len) {
    if (g_backend && g_backend->boardName) {
        g_backend->boardName(buf, len);
        return;
    }
    const char* stub = "host";
    size_t n = strlen(stub);
    if (len == 0) return;
    if (n > len - 1) n = len - 1;
    memcpy(buf, stub, n);
    buf[n] = '\0';
}

float bpvm_pico_temp_c(void) {
    if (g_backend && g_backend->tempC) {
        return g_backend->tempC();
    }
    printf("[pico] tempC (stub → 25.0)\n");
    return 25.0f;
}

int bpvm_pico_cpu_freq_hz(void) {
    if (g_backend && g_backend->cpuFreqHz) {
        return g_backend->cpuFreqHz();
    }
    printf("[pico] cpuFreqHz (stub → 150_000_000)\n");
    return 150000000;
}

int bpvm_pico_uptime_ms(void) {
    if (g_backend && g_backend->uptimeMs) {
        return g_backend->uptimeMs();
    }
    /* En host sin backend, devolver 0 — no tenemos un boot time
     * que sea relevante para BP en desarrollo. */
    return 0;
}

int bpvm_pico_gpio_count(void) {
    if (g_backend && g_backend->gpioCount) {
        return g_backend->gpioCount();
    }
    /* Stub host: perfil RP2350A (30 GPIO) — igual que el default antiguo
     * de Pico.GPIO_COUNT(). El device lo resuelve desde board_desc. */
    return 30;
}

const char* bpvm_pico_reset_cause(void) {
    if (g_backend && g_backend->resetCause) {
        return g_backend->resetCause();
    }
    /* Host / backend sin impl: no hay causa de reset de MCU. */
    return "unknown";
}

int bpvm_pico_set_cpu_freq_mhz(int mhz) {
    if (g_backend && g_backend->setCpuFreqMHz) {
        return g_backend->setCpuFreqMHz(mhz);
    }
    /* Stub: en host no podemos cambiar la frecuencia de nada,
     * pero reportamos "éxito" para que el código BP no rompa en
     * desarrollo. */
    printf("[pico] setCpuFreqMHz(%d) (stub, no-op)\n", mhz);
    return 1;
}
