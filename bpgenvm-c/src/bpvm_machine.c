/*
 * pico.c — fachada de hooks Pico para la VM C.
 *
 * Mismo patrón que pulse.c / pwm.c. Stubs en host con valores
 * razonables ("host" como board, 25.0 °C, etc.) para que el código
 * BP corra en desarrollo sin HW. En el firmware Pico, main.c
 * registra el backend que llama al SDK del Pico.
 */

#include "bpvm_out.h"
#include "bpvm_machine.h"
#include "bpvm_gpio.h"       /* #669: bpvm_gpio_de_alguien */
#include "bpvm_platform.h"   /* H13 h41: reloj monotono para el uptime sin backend */
#include <stdio.h>
#include <string.h>

static const bpvm_machine_backend_t* g_backend = NULL;

void bpvm_machine_set_backend(const bpvm_machine_backend_t* backend) {
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

void bpvm_machine_unique_id(char* buf, size_t len) {
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
 * Ver el porque de que sean DOS en bpvm_machine.h. Aqui solo la mecanica: cada
 * imagen aporta su implementacion y estos accesores son el UNICO sitio del que
 * leen el builtin de BP y el wire. */

void bpvm_machine_board_name(char* buf, size_t len) {
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

float bpvm_machine_temp_c(void) {
    if (g_backend && g_backend->tempC) {
        return g_backend->tempC();
    }
    /* #471 - SIN traza: un getter que escribe una linea cada vez que lo llamas
     * es ruido -imagina tempC() en un bucle- y ademas miVM no la escribia, o sea
     * que era una rotura de paridad. El valor es el perfil del host. */
    return 25.0f;
}

int bpvm_machine_cpu_freq_hz(void) {
    if (g_backend && g_backend->cpuFreqHz) {
        return g_backend->cpuFreqHz();
    }

    return s_host_cpu_hz;   /* #471 - lo que se haya pedido; ver arriba */
}

int bpvm_machine_uptime_ms(void) {
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

/* ══════════ V7/H4 — LA TABLA DE CAPACIDADES (ver bpvm_machine.h) ══════════ */

/* El perfil del HOST = RP2350A, el mismo de miVM: 30 GPIO (todos digitales, PWM
 * y PIO), ADC en GP26..29 (unidad 0, canales 0..3), sin colector abierto (el
 * RP2350 no lo tiene), UART/I2C/SPI 0 y 1, 12 slices, migas no. */
static const uint8_t  s_host_uart[] = { 0, 1 };
static const uint8_t  s_host_i2c[]  = { 0, 1 };
static const uint8_t  s_host_spi[]  = { 0, 1 };
#define HP (BPVM_PIN_DIGITAL | BPVM_PIN_PWM | BPVM_PIN_PULSE)
#define HA (HP | BPVM_PIN_ANALOG)
static const uint16_t s_host_pin[30] = {
    HP, HP, HP, HP, HP, HP, HP, HP, HP, HP, HP, HP, HP, HP, HP, HP,
    HP, HP, HP, HP, HP, HP, HP, HP, HP, HP, HA, HA, HA, HA };
#undef HP
#undef HA
static const int8_t s_host_adc_unit[30] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0, 0, 0 };
static const int8_t s_host_adc_ch[30] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 1, 2, 3 };
static const bpvm_caps_t s_host_caps = {
    .micro = "host", .paquete = "", .fabricante = "",   /* el PC: los NÚMEROS son del RP2350A, la identidad no */
    .cpu_hz_max = 300000000u, .cpu_hz_min = 18000000u, .cpu_ajustable = 1,
    .uart = s_host_uart, .n_uart = 2, .i2c = s_host_i2c, .n_i2c = 2, .spi = s_host_spi, .n_spi = 2,
    .n_pwm = 12, .pwm_unidad = "slice", .n_adc = 1,
    .pin = s_host_pin, .n_pin = 30, .pin_adc_unit = s_host_adc_unit, .pin_adc_ch = s_host_adc_ch,
    .breadcrumb = 0,
};
static const bpvm_caps_t* g_caps = &s_host_caps;
/* V7 #669 — las reservas del SISTEMA hechas al arrancar (la pantalla de la imagen), ENCIMA de la tabla de la familia:
 * ésa es const, y quien sabe qué pines lleva la pantalla es su driver. Por pin, el BPVM_PIN_SIS_*; 0 = ninguna. Todo
 * el que lee la tabla pasa por bpvm_caps_pin_mask, que las mira. */
static uint8_t s_reserva_sis[BPVM_CAPS_PIN_MAX];

void bpvm_caps_set(const bpvm_caps_t* caps) {
    g_caps = caps ? caps : &s_host_caps;
    memset(s_reserva_sis, 0, sizeof s_reserva_sis);   /* #669: una tabla nueva, sin reservas */
}
const bpvm_caps_t* bpvm_caps(void) { return g_caps; }

int bpvm_caps_pin_mask(int pin) {
    if (pin < 0 || pin >= (int) g_caps->n_pin || !g_caps->pin) return 0;
    if (pin < BPVM_CAPS_PIN_MAX && s_reserva_sis[pin]) return 0;   /* #669: del sistema desde el arranque */
    return (int) g_caps->pin[pin];
}
void bpvm_caps_pin_ausente(int pin, char* err, size_t cap) {
    int sis = (pin >= 0 && pin < BPVM_CAPS_PIN_MAX && s_reserva_sis[pin]) ? s_reserva_sis[pin]          /* #669 */
            : (g_caps->pin_sistema && pin >= 0 && pin < (int) g_caps->n_pin) ? g_caps->pin_sistema[pin] : 0;
    if (sis == BPVM_PIN_SIS_MEMORIA)
        snprintf(err, cap, "el pin %d de %s es de la flash o la PSRAM: no se puede usar", pin, g_caps->micro);
    else if (sis == BPVM_PIN_SIS_WIRE)
        snprintf(err, cap, "el pin %d de %s es del cable del IDE (el wire): no se puede usar", pin, g_caps->micro);
    else if (sis == BPVM_PIN_SIS_PANTALLA)
        snprintf(err, cap, "el pin %d de %s es de la pantalla: no se puede usar", pin, g_caps->micro);
    else if (sis == BPVM_PIN_SIS_SONIDO)   /* V8/H4 (#708) */
        snprintf(err, cap, "el pin %d de %s es del sonido: no se puede usar", pin, g_caps->micro);
    else
        snprintf(err, cap, "el pin %d no existe en %s", pin, g_caps->micro);
}
int bpvm_caps_pins_con(uint16_t bit) {
    int n = 0;
    for (int i = 0; i < (int) g_caps->n_pin; i++) if (bpvm_caps_pin_mask(i) & bit) n++;   /* #669: por la máscara */
    return n;
}
int bpvm_caps_gpio_count(void) {
    int n = 0;
    for (int i = 0; i < (int) g_caps->n_pin; i++) if (bpvm_caps_pin_mask(i)) n++;         /* #669: por la máscara */
    return n;
}
int bpvm_caps_adc_de_pin(int pin, int* unit, int* ch) {
    if (!(bpvm_caps_pin_mask(pin) & BPVM_PIN_ANALOG) || !g_caps->pin_adc_unit || !g_caps->pin_adc_ch) return -1;
    if (unit) *unit = g_caps->pin_adc_unit[pin];
    if (ch)   *ch   = g_caps->pin_adc_ch[pin];
    return 0;
}
int bpvm_caps_pin_de_adc(int unit, int ch) {
    for (int i = 0; i < (int) g_caps->n_pin; i++)
        if ((bpvm_caps_pin_mask(i) & BPVM_PIN_ANALOG) && g_caps->pin_adc_unit[i] == unit && g_caps->pin_adc_ch[i] == ch) return i;
    return -1;
}
int bpvm_caps_reservar_sistema(int pin, unsigned motivo, char* err, size_t cap) {
    if (pin < 0 || pin >= BPVM_CAPS_PIN_MAX || motivo == 0u || motivo > 255u) {
        snprintf(err, cap, "el pin %d no se puede reservar: fuera de la tabla", pin);
        return -1;
    }
    if (s_reserva_sis[pin] == (uint8_t) motivo) return 0;           /* ya lo era: la misma reserva otra vez */
    if (!bpvm_caps_pin_mask(pin)) { bpvm_caps_pin_ausente(pin, err, cap); return -1; }   /* no existe, o es de otro */
    if (bpvm_gpio_de_alguien(pin)) { snprintf(err, cap, "el pin %d ya está en uso", pin); return -1; }
    s_reserva_sis[pin] = (uint8_t) motivo;
    return 0;
}

int bpvm_caps_instancia_ok(const uint8_t* lista, int n, int id) {
    for (int i = 0; i < n; i++) if (lista && lista[i] == id) return 1;
    return 0;
}

/* El nombre del micro también sale de la tabla: la cintura que la fija ya dijo
 * quién es (y en el RP2350, qué variante). El callback microName queda de
 * respaldo para una cintura que aún no fije tabla. */
void bpvm_machine_micro_name(char* buf, size_t len) {
    const char* s = (g_caps != &s_host_caps && g_caps->micro) ? g_caps->micro : NULL;
    if (!s && g_backend && g_backend->microName) { g_backend->microName(buf, len); return; }
    if (!s) s = "host";
    size_t n = strlen(s);
    if (len == 0) return;
    if (n > len - 1) n = len - 1;
    memcpy(buf, s, n);
    buf[n] = (char) 0;
}

/* Los tres contadores de siempre, ahora DERIVADOS de la tabla (una sola cuenta
 * para el INFO y para BP; antes cada familia los rellenaba a mano dos veces). */
int bpvm_machine_gpio_count(void)   { return bpvm_caps_gpio_count(); }
int bpvm_machine_adc_channels(void) { return bpvm_caps_pins_con(BPVM_PIN_ANALOG); }
int bpvm_machine_pwm_slices(void)   { return (int) g_caps->n_pwm; }

const char* bpvm_machine_reset_cause(void) {
    if (g_backend && g_backend->resetCause) {
        return g_backend->resetCause();
    }
    /* Host / backend sin impl: no hay causa de reset de MCU. */
    return "unknown";
}

/* V7/H10 (#626) — el ENV de la placa, para Machine.getEnv (ver bpvm_machine.h). Un puntero y no un campo del
 * backend: dónde está el ENV de cada familia ya lo sabe el arranque común (su gancho `env`), y lo registra él. */
static bpvm_machine_env_fn s_env_fn = NULL;

void bpvm_machine_set_env(bpvm_machine_env_fn fn) { s_env_fn = fn; }

int bpvm_machine_env_get(const char* clave, char* buf, size_t cap) {
    if (buf && cap) buf[0] = '\0';
    if (!s_env_fn || !clave || !buf || cap == 0) return -1;   /* el PC: no hay ENV */
    return s_env_fn(clave, buf, cap);
}

/* ══════════ V7/H4 (#487) — el breadcrumb, en el común (ver bpvm_machine.h) ══════════
 * El algoritmo que vivía en gpio_stm32.c (H10), sin cambiar su contrato: en el
 * arranque se toma un SNAPSHOT del rastro de la vida anterior en orden
 * cronológico ([0] = la primera marca, garantizada aunque el anillo haya dado
 * la vuelta), se pone a cero la cuenta de la vida nueva y se incrementa el
 * número de arranques. markCount/markAt leen el snapshot (estable, inmune a
 * las marcas nuevas). El host no tiene bloque: setMark deja la marca en RAM
 * corriente (no sobrevive a nada), y markCount/bootCount dicen 0 y 1. */
#define BC_MAGIC 0xB9CBA001u
#define BC_MAX   32
static uint32_t* s_bc = NULL;
static unsigned  s_bc_n = 0;
static int       s_bc_trail[BC_MAX + 1];
static int       s_bc_trail_count = 0;
static uint32_t  s_bc_boot_count = 1;
static uint32_t  s_host_bc[8];          /* el PC: RAM corriente, para que setMark no sea un no-op mudo */

void bpvm_breadcrumb_attach(uint32_t* palabras, unsigned n) {
    if (!palabras || n < 8) return;
    if (n > BC_MAX + 4) n = BC_MAX + 4;
    s_bc = palabras; s_bc_n = n;
    unsigned ring = n - 4;
    s_bc_trail_count = 0;
    if (s_bc[0] == BC_MAGIC) {
        /* Reset caliente: reconstruir el rastro de la vida anterior. */
        s_bc_boot_count = s_bc[1] + 1u;
        uint32_t total = s_bc[2], first = s_bc[3];
        if (total > 0u && total <= ring) {
            for (uint32_t j = 0; j < total; j++) s_bc_trail[s_bc_trail_count++] = (int) s_bc[4 + j];
        } else if (total > ring) {
            s_bc_trail[s_bc_trail_count++] = (int) first;                     /* [0] = origen pegajoso */
            for (uint32_t j = 0; j < ring; j++)                                /* las últimas, antigua→reciente */
                s_bc_trail[s_bc_trail_count++] = (int) s_bc[4 + ((total + j) % ring)];
        }
    } else {
        s_bc[0] = BC_MAGIC;                                                     /* arranque en frío */
        s_bc_boot_count = 1u;
    }
    s_bc[1] = s_bc_boot_count;
    s_bc[2] = 0u;                                                               /* vida nueva */
}

void bpvm_machine_set_mark(int code) {
    uint32_t* b = s_bc ? s_bc : s_host_bc;
    unsigned  n = s_bc ? s_bc_n : 8u;
    uint32_t total = b[2];
    if (total == 0u) b[3] = (uint32_t) code;                                    /* la primera, pegajosa */
    b[4 + (total % (n - 4))] = (uint32_t) code;
    b[2] = total + 1u;
}
int bpvm_machine_mark_count(void) { return s_bc ? s_bc_trail_count : 0; }
int bpvm_machine_mark_at(int i)   { return (s_bc && i >= 0 && i < s_bc_trail_count) ? s_bc_trail[i] : 0; }
int bpvm_machine_boot_count(void) { return s_bc ? (int) s_bc_boot_count : 1; }

int bpvm_machine_set_cpu_freq_mhz(int mhz) {
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
