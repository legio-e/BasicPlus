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
    /* ── LA IDENTIDAD, EN DOS CAMPOS Y NO EN UNO (V6, 9-sep) ───────────────────
     *
     * Diseno de Eduardo: «devolvemos los 2 nombres, que sea el usuario el que
     * decida que es lo que le interesa». Antes habia UN campo `boardName` y cada
     * familia le daba un significado distinto — el RP2350 la placa (`pico2`), el
     * STM32 la placa (`nucleo-u575zi`), el P4 el micro (`esp32p4`) y la familia
     * ESP32 un DEVKIT (`esp32s3-devkitc`), el mismo para S3, C3 y C6. De ahi que
     * un programa en un C3 se creyera un S3.
     *
     * Con dos campos no hay nada que decidir ni que sincronizar:
     *   microName  el CHIP. Real y especifico: "rp2350", "esp32c3", "stm32u5"...
     *   boardName  la PLACA. `generic` cuando la imagen no conoce el modelo, que
     *              es lo normal: construimos imagenes para el MICRO y se quieren
     *              genericas. Quien se haga la suya, que ponga lo que quiera.
     *
     * ⚠️ Y LA REGLA DE CAPAS, que es lo que hace que esto no vuelva a divergir:
     * estas dos funciones son HAL BP, y las lee TODO EL MUNDO — el builtin de BP
     * (`Machine.getMicro()` / `getBoard()`) y el wire (el `INFO`). Lo que cambia
     * de una imagen a otra es la IMPLEMENTACION, no el sitio del que se lee.
     * NULL → el accesor devuelve "unknown" / "generic". */
    void  (*microName)(char* buf, size_t len);
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
/* Los DOS nombres. Los llama el builtin de BP y TAMBIEN el wire: una sola fuente
   y dos lectores, que es lo que la Pico ya hacia y las otras dos familias no. */
void  bpvm_pico_micro_name(char* buf, size_t len);
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
