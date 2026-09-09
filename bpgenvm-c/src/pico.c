/*
 * pico.c — fachada de hooks Pico para la VM C.
 *
 * Mismo patrón que pulse.c / pwm.c. Stubs en host con valores
 * razonables ("host" como board, 25.0 °C, etc.) para que el código
 * BP corra en desarrollo sin HW. En el firmware Pico, main.c
 * registra el backend que llama al SDK del Pico.
 */

#include "bpvm_out.h"
#include "bpvm_pico.h"
#include "bpvm_platform.h"   /* H13 h41: reloj monotono para el uptime sin backend */
#include <stdio.h>
#include <string.h>

static const bpvm_pico_backend_t* g_backend = NULL;

void bpvm_pico_set_backend(const bpvm_pico_backend_t* backend) {
    g_backend = backend;
}

/* #471 - LA FRECUENCIA DEL HOST SE SIMULA, no se finge.
 *
 * Aviso de Eduardo (9-sep): «cuidado con la frecuencia, no es una constante. En
 * su dia hicimos pruebas en la Pico: la velocidad y la tension se podian ajustar
 * para hacer overclocking». Y al medirlo salio algo peor que una constante:
 *
 *     set(200) -> true        y acto seguido cpuFreqHz() -> 150000000
 *
 * O sea que el stub contestaba «hecho» a un cambio que no hacia, y la lectura lo
 * desmentia. Es el mismo vicio que el ADC de #480: fallar con un valor plausible.
 *
 * Ahora el host RECUERDA lo que se le pide. No es fingir: el host es el micro
 * SIMULADO (H10), y simular el cambio de frecuencia es exactamente su trabajo —
 * asi `set` y `get` son coherentes y un programa que ajusta la frecuencia se
 * puede probar en el PC, que es de lo que va la cascada. El valor inicial es el
 * perfil RP2350A, el mismo que ya usa gpioCount aqui y en miVM. */
static int s_host_cpu_hz = 150000000;

void bpvm_pico_unique_id(char* buf, size_t len) {
    if (g_backend && g_backend->uniqueId) {
        g_backend->uniqueId(buf, len);
        return;
    }
    /* Stub: ID estable para que los tests sean reproducibles. #471 - el texto
     * es el de miVM ("host-pc"): antes esta VM decia "0000000000000000" y la
     * otra "host-pc", o sea stdout distinto para el mismo programa. */
    const char* stub = "host-pc";
    size_t n = strlen(stub);
    if (len == 0) return;
    if (n > len - 1) n = len - 1;
    memcpy(buf, stub, n);
    buf[n] = '\0';
}

/* ── LOS DOS NOMBRES DE LA IDENTIDAD (V6, 9-sep) ───────────────────────────────
 * Ver el porque de que sean DOS en bpvm_pico.h. Aqui solo la mecanica: cada
 * imagen aporta su implementacion y estos accesores son el UNICO sitio del que
 * leen el builtin de BP y el wire. */

void bpvm_pico_micro_name(char* buf, size_t len) {
    if (g_backend && g_backend->microName) {
        g_backend->microName(buf, len);
        return;
    }
    /* Sin cintura estamos en el host: el micro es este PC. */
    const char* stub = "host";
    size_t n = strlen(stub);
    if (len == 0) return;
    if (n > len - 1) n = len - 1;
    memcpy(buf, stub, n);
    buf[n] = (char) 0;
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
    /* #471 - SIN traza: un getter que escribe una linea cada vez que lo llamas
     * es ruido -imagina tempC() en un bucle- y ademas miVM no la escribia, o sea
     * que era una rotura de paridad. El valor es el perfil del host. */
    return 25.0f;
}

int bpvm_pico_cpu_freq_hz(void) {
    if (g_backend && g_backend->cpuFreqHz) {
        return g_backend->cpuFreqHz();
    }

    return s_host_cpu_hz;   /* #471 - lo que se haya pedido; ver arriba */
}

int bpvm_pico_uptime_ms(void) {
    if (g_backend && g_backend->uptimeMs) {
        return g_backend->uptimeMs();
    }
    /* H13 hallazgo 41 (6-ago-2026) — AQUI SE DEVOLVIA 0 A SECAS. El comentario
     * decia "en host sin backend no tenemos un boot time relevante para BP en
     * desarrollo", y para el host de desarrollo era razonable. Dejo de serlo
     * cuando el MICRO SIMULADO paso a ser producto (H10): el sim tampoco registra
     * backend, asi que Pico.uptimeMs() valia 0 SIEMPRE y CUALQUIER medida de
     * tiempo en el emulador salia `0 ms`. Lo vio Eduardo corriendo el Bench:
     *     fib(28) interp = 317811 in 0 ms
     * El resultado es correcto —el bucle SE EJECUTO— y el reloj es el que miente.
     * Cuarto "instrumento que miente" de esta campana, y el mas visible: los
     * benchmarks viajan en el ZIP desde hoy (hallazgo 35).
     *
     * Ahora: reloj MONOTONO de la plataforma, referido al primer uso, que es lo
     * que significa "uptime" para quien lo llama. No se inventa un boot time: se
     * mide desde que el programa pregunta por primera vez, y las diferencias
     * —que es para lo que sirve— son exactas. */
    static int64_t t0 = 0;
    int64_t now = bpvm_platform_now_ms();
    if (t0 == 0) t0 = now;
    return (int) (now - t0);
}

int bpvm_pico_gpio_count(void) {
    if (g_backend && g_backend->gpioCount) {
        return g_backend->gpioCount();
    }
    /* Stub host: perfil RP2350A (30 GPIO) — igual que el default antiguo
     * de Pico.GPIO_COUNT(). El device lo resuelve desde board_desc. */
    return 30;
}

int bpvm_pico_adc_channels(void) {
    if (g_backend && g_backend->adcChannels) {
        return g_backend->adcChannels();
    }
    return 4;   /* host: perfil RP2350 (4 ADC). Device: board_desc / board.json. */
}

int bpvm_pico_pwm_slices(void) {
    if (g_backend && g_backend->pwmSlices) {
        return g_backend->pwmSlices();
    }
    return 12;  /* host: perfil RP2350 (12 PWM). Device: board_desc / board.json. */
}

const char* bpvm_pico_reset_cause(void) {
    if (g_backend && g_backend->resetCause) {
        return g_backend->resetCause();
    }
    /* Host / backend sin impl: no hay causa de reset de MCU. */
    return "unknown";
}

void bpvm_pico_set_mark(int code) {
    if (g_backend && g_backend->setMark) g_backend->setMark(code);
    /* Host: no-op (sin RAM retenida). */
}

int bpvm_pico_mark_count(void) {
    if (g_backend && g_backend->markCount) return g_backend->markCount();
    return 0;   /* host: sin trail */
}

int bpvm_pico_mark_at(int i) {
    if (g_backend && g_backend->markAt) return g_backend->markAt(i);
    return 0;   /* host: sin trail */
}

int bpvm_pico_boot_count(void) {
    if (g_backend && g_backend->bootCount) return g_backend->bootCount();
    return 1;   /* host: el proceso = 1 "arranque" */
}

int bpvm_pico_set_cpu_freq_mhz(int mhz) {
    if (g_backend && g_backend->setCpuFreqMHz) {
        return g_backend->setCpuFreqMHz(mhz);
    }
    /* #471 - el host SIMULA el cambio y lo RECUERDA, para que `set` y `get` no
     * se contradigan. El texto es contrato de paridad con miVM. */
    if (mhz <= 0) return 0;
    s_host_cpu_hz = mhz * 1000000;
    bpvm_out("[pico] setCpuFreqMHz(%d) (host, simulado)\n", mhz);
    return 1;
}
