/*
 * bpvm_machine.h — hooks de plataforma para los builtins Pico.*
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
    /* H10 — causa del último reset como string ("watchdog (IWDG)", "power-on",
     * "software", "pin (NRST)", ...). NULL → el accesor devuelve "unknown". */
    const char* (*resetCause)(void);
} bpvm_machine_backend_t;

/* ══════════════════════════════════════════════════════════════════════════
 * V7/H4 (#378, #470, #479) — LA TABLA DE CAPACIDADES DEL MICRO
 *
 * «Que cada micro DIGA lo que tiene»: UNA tabla por micro (y por VARIANTE: la
 * elige la cintura en el arranque por lo que el silicio dice de sí mismo —
 * PACKAGE_SEL en el RP2350, el IDF en las ESP32, el CMSIS/IDCODE en el STM32),
 * y NINGÚN otro sitio declara un número: el INFO del wire, `Machine` en BP, la
 * validación de las fachadas y las clases de pines leen de aquí. Antes había
 * dos fuentes rellenadas a mano por familia (esta fachada y el INFO) y no
 * coincidían: la Pico decía 12 PWM por BP y 24 por el wire; un C5 se creía un
 * S3 por BP (45 GPIO, 20 ADC) mientras el wire decía 29/6.
 *
 * Los ARRAYS son de Eduardo (9-sep): «para cada micro unos arrays [0, 1, 3]
 * que indican los números válidos de UART, I2C, SPI…; el número es la longitud».
 * Para lo que en BP se construye por PIN (PWM, ADC, contadores de pulsos) lo
 * útil es el MAPA DE PINES: por GPIO, qué puede — y su (unidad, canal) de ADC,
 * que algún micro tiene más de uno (Eduardo, 22-sep).
 *
 * Lo que es de PLACA (LED, NeoPixel, panel, FS) NO va aquí: sigue en el ENV.
 * La RAM y la flash tampoco: se miden al arrancar (JEDEC, esp_flash_get_size,
 * FLASH_SIZE). Esto es verdad de SILICIO y viaja compilada en la imagen.
 * ══════════════════════════════════════════════════════════════════════════ */
#define BPVM_PIN_DIGITAL     0x01u   /* entrada/salida digital */
#define BPVM_PIN_ANALOG      0x02u   /* entrada de ADC (pin_adc_unit/ch dicen cuál) */
#define BPVM_PIN_PWM         0x04u   /* puede dar PWM */
#define BPVM_PIN_PULSE       0x08u   /* puede contar pulsos (PIO / PCNT / timer) */
#define BPVM_PIN_OPEN_DRAIN  0x10u   /* salida en colector abierto de verdad */
#define BPVM_PIN_INPUT_ONLY  0x20u   /* sólo entrada (no hay driver de salida) */
#define BPVM_CAPS_PIN_MAX    160     /* tope del mapa: 8 puertos × 16 + holgura (STM32 con I) */

/* #574 (V7/H4, 26-sep) — por qué un pin que EXISTE en el silicio no está para el programa. Su máscara es 0 —para BP
 * y para la validación, no está—; esto sólo le pone nombre en el error. En la familia ESP32, la tabla daba por buenos
 * los de la flash (en la C6, el GPIO25 es su MISO) y los del cable del IDE (decidido por Eduardo: también esos). */
#define BPVM_PIN_SIS_MEMORIA 1u      /* la flash o la PSRAM: el sistema los reserva al arrancar */
#define BPVM_PIN_SIS_WIRE    2u      /* el cable del IDE (el wire): quitárselo corta la conexión */

typedef struct {
    const char*     micro;          /* "esp32c5" — el ID canónico (Machine.getMicro, INFO.boardName) */
    const char*     paquete;        /* "QFN-60", "LQFP144"… — la VARIANTE elegida en el arranque */
    const char*     fabricante;     /* "Raspberry Pi" | "Espressif" | "STMicroelectronics" */
    uint32_t        cpu_hz_max;     /* POR MICRO (antes: los 300/18 MHz del RP2350 para todos) */
    uint32_t        cpu_hz_min;
    uint8_t         cpu_ajustable;  /* 1 = setCpuFreqMHz hace algo en este micro */
    const uint8_t*  uart; uint8_t n_uart;   /* las INSTANCIAS válidas, tal como las toma Uart.Port(id, …) */
    const uint8_t*  i2c;  uint8_t n_i2c;    /* … I2c.Bus(id, …) */
    const uint8_t*  spi;  uint8_t n_spi;    /* … Spi.Bus(id, …) */
    uint8_t         n_pwm;                  /* salidas PWM simultáneas… */
    const char*     pwm_unidad;             /* …y su UNIDAD dicha: "slice" (RP2350) | "canal" (LEDC, timer) */
    uint8_t         n_adc;                  /* unidades de ADC del silicio */
    const uint16_t* pin;  uint16_t n_pin;   /* el MAPA: por GPIO, la máscara BPVM_PIN_*; 0 = no existe */
    const int8_t*   pin_adc_unit;           /* por GPIO: unidad de ADC (0 = la primera) o -1 */
    const int8_t*   pin_adc_ch;             /* por GPIO: canal en esa unidad, o -1 */
    uint8_t         breadcrumb;             /* 1 = hay migas de verdad (RAM retenida); 0 = no las hay (#487) */
    const uint8_t*  pin_sistema;            /* #574: por GPIO, BPVM_PIN_SIS_* si es del sistema (máscara 0); NULL = ninguno */
} bpvm_caps_t;

/* ── V7/H4 (#487) — EL BREADCRUMB, en el COMÚN sobre un bloque RETENIDO ──────
 * Migas que sobreviven al reset (setMark deja una; markCount/markAt leen el
 * rastro de la vida ANTERIOR, markAt(0) = la primera, pegajosa; bootCount).
 * Hasta hoy sólo el STM32 las tenía —en sus registros de backup, con el
 * algoritmo dentro de su cintura— y las otras cuatro familias contestaban
 * 0/0/1 MUDAS. Ahora el algoritmo vive aquí y cada familia sólo pone el
 * BLOQUE que su silicio conserva entre resets: el STM32 los BKPxR del TAMP, el
 * RP2350 una zona `.uninitialized_data` (el SDK no la borra al arrancar), las
 * ESP32 una `.noinit`. Layout del bloque (n >= 8 palabras): [0] magic ·
 * [1] arranques · [2] marcas de esta vida · [3] la primera (pegajosa) · [4..]
 * el anillo de las últimas n-4. Con el magic se distingue el arranque en frío
 * (basura) del reset caliente. Sin bloque (el PC): las migas no sobreviven a
 * nada y `caps.breadcrumb` lo dice. */
void bpvm_breadcrumb_attach(uint32_t* palabras, unsigned n);

/* La cintura la fija en el arranque (antes de la VM). Sin cintura: el perfil
 * del host (RP2350A), el mismo que miVM: así el PC y la placa cuentan igual. */
void bpvm_caps_set(const bpvm_caps_t* caps);
const bpvm_caps_t* bpvm_caps(void);                     /* nunca NULL */
/* Derivados (una sola forma de contar, para el INFO y para BP): */
int bpvm_caps_gpio_count(void);                          /* pines con máscara != 0 */
int bpvm_caps_pins_con(uint16_t bit);                    /* cuántos pines tienen ese bit */
int bpvm_caps_pin_mask(int pin);                         /* la máscara del pin, 0 si no existe */
/* #574 — el texto de por qué `pin` no está: «el pin 7 no existe en rp2350a», «el pin 25 de esp32c6 es de la flash o
 * la PSRAM», «el pin 12 de esp32c6 es del cable del IDE (el wire)». Para los errores de las fachadas. */
void bpvm_caps_pin_ausente(int pin, char* err, size_t cap);
int bpvm_caps_adc_de_pin(int pin, int* unit, int* ch);   /* 0 y rellena, o -1 si no es analógico */
int bpvm_caps_pin_de_adc(int unit, int ch);              /* el GPIO de ese canal, o -1 */
int bpvm_caps_instancia_ok(const uint8_t* lista, int n, int id);   /* ¿id está en la lista? */

void bpvm_machine_set_backend(const bpvm_machine_backend_t* backend);


/* Funciones efectivas. Stubs con logging si backend NULL. */
void  bpvm_machine_unique_id(char* buf, size_t len);
/* Los DOS nombres. Los llama el builtin de BP y TAMBIEN el wire: una sola fuente
   y dos lectores, que es lo que la Pico ya hacia y las otras dos familias no. */
void  bpvm_machine_micro_name(char* buf, size_t len);
void  bpvm_machine_board_name(char* buf, size_t len);
float bpvm_machine_temp_c(void);
int   bpvm_machine_cpu_freq_hz(void);
int   bpvm_machine_uptime_ms(void);
int   bpvm_machine_set_cpu_freq_mhz(int mhz);
int   bpvm_machine_gpio_count(void);
int   bpvm_machine_adc_channels(void);
int   bpvm_machine_pwm_slices(void);
const char* bpvm_machine_reset_cause(void);   /* H10 — causa del último reset */
void bpvm_machine_set_mark(int code);         /* H10 — breadcrumb: deja una miga */
int  bpvm_machine_mark_count(void);           /* H10 — nº migas del trail previo */
int  bpvm_machine_mark_at(int i);             /* H10 — i-ésima miga (0 = origen) */
int  bpvm_machine_boot_count(void);           /* H10 — arranques desde power-on */

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_H */
