/*
 * test_suelta.c — V7 #667 (8-oct): LO QUE UN OBJETO TOMA DEL HARDWARE LO SUELTA SU DESTRUCTOR; y lo que nadie suelta,
 * la RED del final del RUN. Con backends FALSOS que cuentan («dobles que cuentan», el plan de #668): cada init abre una
 * unidad (un PWM, un contador, una tira, una UART) o pone un pin de salida; cada release —o la red— la cierra. Al acabar
 * el RUN no puede quedar nada abierto, y se mira QUIÉN lo cerró.
 *
 *   Fase 1 — SueltaTodo.mod: objetos con vidas distintas (un owner local, una global, locales del main). Todo lo
 *            sueltan sus destructores —la red no tiene nada que hacer— y la tira se apaga antes de soltarla.
 *   Fase 2 — SueltaSinObjetos.mod: las funciones del módulo, sin objetos. Nadie tiene destructor: todo lo suelta la red.
 *
 * Salida: una línea «ok …» por comprobación y [status=OK]; si algo falla, «FAIL …» y [status=FAIL].
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include "bpvm_gpio.h"
#include "bpvm_pwm.h"
#include "bpvm_pulse.h"
#include "bpvm_neopixel.h"
#include "bpvm_uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 64
static int pwm_abierto[N], pulse_abierto[N], np_abierto[N], uart_abierto[8], pin_salida[N];
static int por_destructor, por_red, soltados_pin, np_shows;
static uint32_t np_ultimo[8];
static int np_ultimo_n;
static int fallos;

static void comprueba(int cond, const char* que) {
    if (cond) printf("ok  %s\n", que);
    else { printf("FAIL %s\n", que); fallos++; }
}

/* ── los dobles ── */
static int  pwm_init(int pin, int f) { (void) f; if (pin < 0 || pin >= N) return -1; pwm_abierto[pin] = 1; return pin; }
static void pwm_freq(int s, int f) { (void) s; (void) f; }
static void pwm_duty(int s, int pin, int d) { (void) s; (void) pin; (void) d; }
static void pwm_nada(int s) { (void) s; }
static void pwm_release(int s, int pin) { (void) s; if (pin >= 0 && pin < N && pwm_abierto[pin]) { pwm_abierto[pin] = 0; por_destructor++; } }
static int  pwm_fin(void) { int n = 0; for (int p = 0; p < N; p++) if (pwm_abierto[p]) { pwm_abierto[p] = 0; por_red++; n++; } return n; }
static const bpvm_pwm_backend_t PWM_FALSO = {
    .init = pwm_init, .setFreq = pwm_freq, .setDuty = pwm_duty, .start = pwm_nada, .stop = pwm_nada,
    .release = pwm_release, .fin_de_run = pwm_fin,
};

static int  pulse_init(int pin, int e) { (void) e; if (pin < 0 || pin >= N) return -1; pulse_abierto[pin] = 1; return pin; }
static void pulse_nada(int id) { (void) id; }
static int  pulse_valor(int id) { (void) id; return 0; }
static void pulse_release(int id, int pin) { (void) id; if (pin >= 0 && pin < N && pulse_abierto[pin]) { pulse_abierto[pin] = 0; por_destructor++; } }
static int  pulse_fin(void) { int n = 0; for (int p = 0; p < N; p++) if (pulse_abierto[p]) { pulse_abierto[p] = 0; por_red++; n++; } return n; }
static const bpvm_pulse_backend_t PULSE_FALSO = {
    .init = pulse_init, .start = pulse_nada, .stop = pulse_nada, .value = pulse_valor, .reset = pulse_nada,
    .release = pulse_release, .fin_de_run = pulse_fin,
};

static int  np_init(int pin, char* err, size_t n) { (void) err; (void) n; if (pin < 0 || pin >= N) return 0; np_abierto[pin] = 1; return 1; }
static void np_show(int pin, const uint32_t* grb, int count) {
    (void) pin;
    np_shows++;
    np_ultimo_n = count < 8 ? count : 8;
    for (int i = 0; i < np_ultimo_n; i++) np_ultimo[i] = grb[i];
}
static void np_release(int pin) { if (pin >= 0 && pin < N && np_abierto[pin]) { np_abierto[pin] = 0; por_destructor++; } }
static int  np_fin(void) { int n = 0; for (int p = 0; p < N; p++) if (np_abierto[p]) { np_abierto[p] = 0; por_red++; n++; } return n; }
static const bpvm_neopixel_backend_t NP_FALSO = { .init = np_init, .show = np_show, .release = np_release, .fin_de_run = np_fin };

static void uart_init(int bus, int tx, int rx, int b, int d, int s, int p) {
    (void) tx; (void) rx; (void) b; (void) d; (void) s; (void) p;
    if (bus >= 0 && bus < 8) uart_abierto[bus] = 1;
}
static int  uart_write(int bus, const uint8_t* data, size_t n) { (void) bus; (void) data; return (int) n; }
static int  uart_read(int bus, uint8_t* data, size_t n, int t) { (void) bus; (void) data; (void) n; (void) t; return 0; }
static int  uart_disp(int bus) { (void) bus; return 0; }
/* La red de la UART es la fachada (bpvm_uart_fin_de_run): llama a este release con los pines a -1. */
static void uart_release(int bus, int tx, int rx) {
    (void) rx;
    if (bus >= 0 && bus < 8 && uart_abierto[bus]) { uart_abierto[bus] = 0; if (tx < 0) por_red++; else por_destructor++; }
}
static const bpvm_uart_backend_t UART_FALSA = { .init = uart_init, .write = uart_write, .read = uart_read, .available = uart_disp, .release = uart_release };

static void gpio_init(int pin, int mode) { if (pin >= 0 && pin < N && mode == 1) pin_salida[pin] = 1; }
static void gpio_pull(int pin, int m) { (void) pin; (void) m; }
static void gpio_write(int pin, int v) { (void) pin; (void) v; }
static int  gpio_read(int pin) { (void) pin; return 0; }
static void gpio_soltar(int pin) { if (pin >= 0 && pin < N) { pin_salida[pin] = 0; soltados_pin++; } }
static const bpvm_gpio_backend_t GPIO_FALSO = { .init = gpio_init, .pull = gpio_pull, .write = gpio_write, .read = gpio_read, .soltar = gpio_soltar };

static int nada_abierto(void) {
    for (int i = 0; i < N; i++) if (pwm_abierto[i] || pulse_abierto[i] || np_abierto[i] || pin_salida[i]) return 0;
    for (int i = 0; i < 8; i++) if (uart_abierto[i]) return 0;
    return 1;
}

static void salida(const char* s, size_t len, void* user) { (void) user; fwrite(s, 1, len, stdout); }

static int correr(const char* mod) {
    memset(pwm_abierto, 0, sizeof pwm_abierto); memset(pulse_abierto, 0, sizeof pulse_abierto);
    memset(np_abierto, 0, sizeof np_abierto); memset(uart_abierto, 0, sizeof uart_abierto);
    memset(pin_salida, 0, sizeof pin_salida);
    por_destructor = por_red = soltados_pin = np_shows = np_ultimo_n = 0;
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { printf("FAIL sin memoria\n"); return -1; }
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { printf("FAIL bpvm_init\n"); free(mem); return -1; }
    bpvm_set_output(vm, salida, NULL);
    bpvm_status_t s = bpvm_load_mod(vm, mod);
    if (s != BPVM_OK) { printf("FAIL load_mod %s: %s\n", mod, bpvm_status_str(s)); bpvm_destroy(vm); free(mem); return -1; }
    s = bpvm_run(vm);
    bpvm_destroy(vm);   /* aquí corre la red del final del RUN */
    free(mem);
    return (int) s;
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* todo   = (argc > 1) ? argv[1] : "SueltaTodo.mod";
    const char* sinobj = (argc > 2) ? argv[2] : "SueltaSinObjetos.mod";
    bpvm_pwm_set_backend(&PWM_FALSO);
    bpvm_pulse_set_backend(&PULSE_FALSO);
    bpvm_neopixel_set_backend(&NP_FALSO);
    bpvm_uart_set_backend(&UART_FALSA);
    bpvm_gpio_set_backend(&GPIO_FALSO);

    printf("--- fase 1: %s (objetos: los sueltan sus destructores) ---\n", todo);
    int rs = correr(todo);
    comprueba(rs == BPVM_OK, "fase 1: el programa acaba bien");
    comprueba(nada_abierto(), "fase 1: al acabar no queda nada abierto (ni un PWM, ni un contador, ni una tira, ni una UART, ni un pin de salida)");
    comprueba(por_destructor == 4, "fase 1: el PWM, la UART, el contador y la tira, cada uno por su destructor");
    comprueba(por_red == 0, "fase 1: la red no tuvo nada que soltar");
    comprueba(soltados_pin == 1, "fase 1: el pin del Gpio.Pin, devuelto al reset por su destructor");
    {
        int apagada = np_ultimo_n == 3;
        for (int i = 0; i < np_ultimo_n; i++) if (np_ultimo[i] != 0u) apagada = 0;
        comprueba(np_shows == 2 && apagada, "fase 1: la tira, apagada antes de soltarla (lo último que se le mandó, todo a cero)");
    }

    printf("--- fase 2: %s (sin objetos: lo suelta la red del final del RUN) ---\n", sinobj);
    rs = correr(sinobj);
    comprueba(rs == BPVM_OK, "fase 2: el programa acaba bien");
    comprueba(nada_abierto(), "fase 2: al acabar no queda nada abierto");
    comprueba(por_destructor == 0, "fase 2: ningún destructor (no hay objetos)");
    comprueba(por_red == 3, "fase 2: la red suelta el PWM, el contador y la UART");
    comprueba(soltados_pin == 1, "fase 2: y devuelve al reset el pin que quedó de salida");

    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
