/*
 * bpvm_pico.h — hooks de plataforma para los builtins Pico.*
 *
 * Información del microcontrolador físico:
 *   - Identificación: ID único del chip flash (8 bytes, 16 hex
 *     chars como string), nombre de la board.
 *   - Sensores internos: temperatura del die (ADC4 en RP2350).
 *   - Reloj: frecuencia actual del sys_clk, ms desde boot.
 *
 * Patrón habitual: si no hay backend (host stub), las funciones
 * imprimen por stdout y devuelven valores razonables para que el
 * código BP no rompa en desarrollo sin HW real.
 *
 * Convenciones:
 *   uniqueId(buf, len): escribe ID como string ASCII (16 chars hex
 *                       + null terminator → necesita len >= 17).
 *   boardName(buf, len): nombre legible de la board ("pico2" en
 *                        firmware Pico 2; "host" en host stub).
 *   tempC(): °C como float. Lee el ADC interno; debe inicializarlo
 *            la primera vez.
 *   cpuFreqHz(): Hz actuales del clk_sys (típicamente 150 MHz en
 *                RP2350 con la config por defecto).
 *   uptimeMs(): ms desde el boot del firmware. Wrap a 32-bit ≈ 49
 *               días, suficiente para uso normal.
 */
#ifndef BPVM_PICO_H
#define BPVM_PICO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void  (*uniqueId)(char* buf, size_t len);
    void  (*boardName)(char* buf, size_t len);
    float (*tempC)(void);
    int   (*cpuFreqHz)(void);
    int   (*uptimeMs)(void);
    /* setCpuFreqMHz: cambia el clk_sys. Debe clampear al máximo
     * soportado por la board. Devuelve 1 si se aplicó, 0 si falló
     * (la PLL no soporta esa frecuencia incluso después de clamp). */
    int   (*setCpuFreqMHz)(int mhz);
    /* H7.3 — board-aware: nº de GPIO de la placa (lo da el board_desc:
     * 30 RP2350A / 48 RP2350B, u override de /sys/board.json). */
    int   (*gpioCount)(void);
    /* H10 — causa del último reset como string ("watchdog (IWDG)", "power-on",
     * "software", "pin (NRST)", ...). NULL → el accesor devuelve "unknown". */
    const char* (*resetCause)(void);
    /* H10 — breadcrumb en RAM retenida (migas que sobreviven al reset):
     * setMark deja una miga; markCount/markAt leen el trail de ANTES del reset
     * (markAt(0) = 1ª marca pegajosa = causa original); bootCount = nº arranques.
     * NULL → stubs (0 / 0 / 1) — sin RAM retenida no hay diagnóstico de reset. */
    void (*setMark)(int code);
    int  (*markCount)(void);
    int  (*markAt)(int i);
    int  (*bootCount)(void);
    /* H14 — counts de periféricos board-aware (del board_desc / board.json):
     * canales ADC y slices PWM de la PLACA. NULL → stub 4/12 (perfil RP2350). */
    int  (*adcChannels)(void);
    int  (*pwmSlices)(void);
} bpvm_pico_backend_t;

void bpvm_pico_set_backend(const bpvm_pico_backend_t* backend);

/* Funciones efectivas. Stubs con logging si backend NULL. */
void  bpvm_pico_unique_id(char* buf, size_t len);
void  bpvm_pico_board_name(char* buf, size_t len);
float bpvm_pico_temp_c(void);
int   bpvm_pico_cpu_freq_hz(void);
int   bpvm_pico_uptime_ms(void);
int   bpvm_pico_set_cpu_freq_mhz(int mhz);
int   bpvm_pico_gpio_count(void);
int   bpvm_pico_adc_channels(void);
int   bpvm_pico_pwm_slices(void);
const char* bpvm_pico_reset_cause(void);   /* H10 — causa del último reset */
void bpvm_pico_set_mark(int code);         /* H10 — breadcrumb: deja una miga */
int  bpvm_pico_mark_count(void);           /* H10 — nº migas del trail previo */
int  bpvm_pico_mark_at(int i);             /* H10 — i-ésima miga (0 = origen) */
int  bpvm_pico_boot_count(void);           /* H10 — arranques desde power-on */

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_H */
