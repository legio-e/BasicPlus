/*
 * gpio_stm32.c — backends de HW del STM32U575 (H9.4).
 *
 * Implementa los hooks portables de la VM sobre la HAL de ST:
 *   - GPIO  (bpvm_gpio_backend_t):  init/pull/write/read sobre HAL_GPIO_*.
 *   - Pico  (bpvm_machine_backend_t):  info de placa (gpioCount, freq, uptime,
 *                                   uniqueId, boardName) para que el módulo
 *                                   stdlib `Pico` y la clase `Gpio.Pin`
 *                                   (que valida contra Pico.gpioCount())
 *                                   funcionen en esta familia.
 *
 * Modelo de pin: la fachada portable usa un `int pin` plano; aquí lo
 * decodificamos a (puerto, bit) de STM32 con el convenio:
 *
 *     pin = (puerto << 4) | bit       puerto: 0=A,1=B,2=C,...,7=H ; bit: 0..15
 *
 * Ejemplos en la Nucleo-U575ZI-Q:  PA5 = 5,  PB7 = 23,  PC7 = 39 (LED verde),
 * PG2 = 98 (LED rojo).  gpioCount() reporta 128 (8 puertos × 16) → la clase
 * Pin acepta cualquiera de ellos.
 *
 * Registro: stm32_hw_register() se llama una vez al boot (desde el REPL),
 * después de la init de la HAL/BSP.
 */
#include "gpio_stm32.h"

#include "bpvm_gpio.h"
#include "bpvm_machine.h"
#include "bpvm_adc.h"   /* V7/H4 (el resto de #469): el backend del ADC externo */
#include "bpvm_spi.h"
#include "bpvm_uart.h"
#include "bpvm_i2c.h"
#include "bpvm_wdt.h"
#include "bpvm_rtc.h"
#include "bpvm_pwm.h"
#include "bpvm_pulse.h"

#include "main.h"   /* HAL + CMSIS (GPIOA.., UID_BASE, SystemCoreClock) */
#include "board.h"  /* BOARD_HAS_ADC_TEMP y demás capacidades de placa */
#if defined(BOARD_HAS_ADC_TEMP)
#include "stm32u5xx_ll_adc.h"   /* __LL_ADC_CALC_* + constantes de calibración del die */
#endif

#include <stdio.h>
#include <string.h>

#define STM32_GPIO_PORTS  8           /* A..H */
#define STM32_PIN_COUNT   (STM32_GPIO_PORTS * 16)

/* Estado por pin para reconfigurar sin perder el modo cuando llega un pull()
 * tras el init() (la HAL fija modo+pull juntos en HAL_GPIO_Init). */
static uint8_t s_mode[STM32_PIN_COUNT];   /* 0=INPUT, 1=OUTPUT */
static uint8_t s_pull[STM32_PIN_COUNT];   /* 0=none, 1=up, 2=down */

static GPIO_TypeDef* port_of(int idx) {
    switch (idx) {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        case 4: return GPIOE;
        case 5: return GPIOF;
        case 6: return GPIOG;
        case 7: return GPIOH;
        default: return NULL;
    }
}

static void enable_port_clk(int idx) {
    switch (idx) {
        case 0: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case 1: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case 2: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        case 3: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
        case 4: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
        case 5: __HAL_RCC_GPIOF_CLK_ENABLE(); break;
        case 6: __HAL_RCC_GPIOG_CLK_ENABLE(); break;
        case 7: __HAL_RCC_GPIOH_CLK_ENABLE(); break;
        default: break;
    }
}

/* Aplica modo+pull cacheados a la HAL. */
static void reconfigure(int pin) {
    int idx = pin >> 4;
    int bit = pin & 0x0F;
    GPIO_TypeDef* port = port_of(idx);
    if (!port) return;

    enable_port_clk(idx);

    GPIO_InitTypeDef cfg = {0};
    cfg.Pin   = (uint32_t) (1u << bit);
    cfg.Mode  = (s_mode[pin] == 1) ? GPIO_MODE_OUTPUT_PP : (s_mode[pin] == 2) ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_INPUT;   /* V7/H4: 2 = colector abierto */
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    cfg.Pull  = (s_pull[pin] == 1) ? GPIO_PULLUP
              : (s_pull[pin] == 2) ? GPIO_PULLDOWN
              : GPIO_NOPULL;
    HAL_GPIO_Init(port, &cfg);
}

/* ---- GPIO backend ---- */

static void stm32_gpio_init_impl(int pin, int mode) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return;
    s_mode[pin] = (mode == 1) ? 1 : (mode == 2) ? 2 : 0;
    s_pull[pin] = 0;   /* init resetea el pull */
    reconfigure(pin);
}

static void stm32_gpio_pull_impl(int pin, int pull_mode) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return;
    s_pull[pin] = (pull_mode == 1) ? 1 : (pull_mode == 2) ? 2 : 0;
    reconfigure(pin);   /* preserva el modo cacheado */
}

static void stm32_gpio_write_impl(int pin, int value) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return;
    GPIO_TypeDef* port = port_of(pin >> 4);
    if (!port) return;
    HAL_GPIO_WritePin(port, (uint16_t) (1u << (pin & 0x0F)),
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static int stm32_gpio_read_impl(int pin) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return 0;
    GPIO_TypeDef* port = port_of(pin >> 4);
    if (!port) return 0;
    return (HAL_GPIO_ReadPin(port, (uint16_t) (1u << (pin & 0x0F))) == GPIO_PIN_SET) ? 1 : 0;
}

/* V7 #667 (8-oct) — UN PIN QUE UN PROGRAMA SUELTA, A COMO ESTABA (Eduardo: «los pines por defecto deben ser input o
 * nada. Si están como output se debería revertir»): el reset del STM32 —analógico, sin pull: HAL_GPIO_DeInit—. Un
 * LED de la PLACA, en cambio, vuelve a ser del firmware (el latido, el RUN, el error), como lo dejó su arranque. */
static int stm32_led_de_placa(int pin) {
#if defined(BPVM_BOARD_DK2)
    if (pin == ((3 << 4) | 2) || pin == ((3 << 4) | 4)) {   /* PD2 rojo, PD4 verde: salidas de CubeMX */
        GPIO_InitTypeDef cfg = {0};
        cfg.Pin = (uint32_t) (1u << (pin & 0x0F));
        cfg.Mode = GPIO_MODE_OUTPUT_PP;
        cfg.Pull = GPIO_NOPULL;
        cfg.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_WritePin(GPIOD, (uint16_t) cfg.Pin, GPIO_PIN_RESET);
        HAL_GPIO_Init(GPIOD, &cfg);
        return 1;
    }
#else
    if (pin == ((2 << 4) | 7)) { BSP_LED_Init(LED_GREEN); return 1; }   /* PC7: el latido */
    if (pin == ((1 << 4) | 7)) { BSP_LED_Init(LED_BLUE);  return 1; }   /* PB7: el RUN */
    if (pin == ((6 << 4) | 2)) { BSP_LED_Init(LED_RED);   return 1; }   /* PG2: el error */
#endif
    return 0;
}

static void stm32_pin_al_reset(int pin) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return;
    s_mode[pin] = 0;
    s_pull[pin] = 0;
    if (stm32_led_de_placa(pin)) return;
    GPIO_TypeDef* port = port_of(pin >> 4);
    if (port) HAL_GPIO_DeInit(port, (uint32_t) (1u << (pin & 0x0F)));
}

static const bpvm_gpio_backend_t s_gpio_backend = {
    .init  = stm32_gpio_init_impl,
    .pull  = stm32_gpio_pull_impl,
    .write = stm32_gpio_write_impl,
    .read  = stm32_gpio_read_impl,
    .soltar = stm32_pin_al_reset,   /* V7 #667 */
};

/* ---- Pico (info de MCU) backend ---- */

/* ══════════ V7/H4 — LA TABLA DE CAPACIDADES del STM32 ══════════
 * Por ahora una por DIE, decidida por los #ifdef del CMSIS con los que se
 * compila cada proyecto (U575 → Nucleo, U5G9 → DK2): los puertos A..H (128
 * pines, modelo plano puerto<<4|bit) son digitales y con colector abierto de
 * verdad; PWM sólo en los pines que el backend mapea a un timer (s_pwm_map);
 * ANALOG los 24 del mapa del ADC (s_adc_mapa, leído de las hojas de datos el 26-sep: el resto de #469) y, sin
 * máscara, los del cable del IDE (#574);
 * las instancias de UART/I2C/SPI, las mismas que aceptan los backends. El
 * registro de PAQUETE (LQFP64/100/144…) y la máscara de pines bondeados por
 * paquete son el siguiente paso (V7_IDEAS §H4.2b): hoy `paquete` dice el
 * proyecto. */
static uint16_t s_caps_pin[STM32_PIN_COUNT];
static int8_t   s_caps_adc_u[STM32_PIN_COUNT];
static int8_t   s_caps_adc_c[STM32_PIN_COUNT];
static const uint8_t s_caps_uart[] = { 2, 3, 4,
#ifdef UART5
    5,
#endif
#ifdef USART6
    6,
#endif
};
static const uint8_t s_caps_i2c[] = { 1, 2,
#ifdef I2C3
    3,
#endif
#ifdef I2C4
    4,
#endif
#ifdef I2C5
    5,
#endif
};
static const uint8_t s_caps_spi[] = { 1, 2, 3 };
static bpvm_caps_t s_caps;
static void stm32_caps_init(void);   /* def. tras s_pwm_map, que necesita */

/* board-aware: canales ADC y slices PWM del STM32U575. Sin estos cb,
 * Pico.ADC_CHANNELS()/PWM_SLICES() caían al fallback host (4/12) de src/bpvm_machine.c
 * — el INFO ya daba 20/28 desde un string aparte (stm32_repl.c), de ahí el
 * mismatch. Valores casan con el INFO. */

static int stm32_cpu_freq_hz_impl(void) {
    return (int) SystemCoreClock;
}

static int stm32_uptime_ms_impl(void) {
    return (int) HAL_GetTick();
}

static void stm32_unique_id_impl(char* buf, size_t len) {
    /* UID de 96 bits del U575; reportamos los 64 bits bajos como 16 hex
     * (convenio del backend: 16 chars + null → len >= 17). */
    uint32_t u0 = *(volatile uint32_t*) (UID_BASE + 0U);
    uint32_t u1 = *(volatile uint32_t*) (UID_BASE + 4U);
    if (buf && len > 0) {
        snprintf(buf, len, "%08lX%08lX", (unsigned long) u1, (unsigned long) u0);
    }
}

static void stm32_board_name_impl(char* buf, size_t len) {
    if (buf && len > 0) {
        /* BOARD_NAME viene de board.h por placa (nucleo-u575zi / u5g9j-dk2). Antes
         * estaba hardcodeado a la Nucleo → mal en la DK2 (Pico.boardName()). */
        strncpy(buf, BOARD_NAME, len - 1);
        buf[len - 1] = '\0';
    }
}

#if defined(BOARD_HAS_ADC_TEMP)
/* ── Temperatura del die por ADC1 (H10) ──────────────────────────────────────
 * CubeMX inicializa hadc1 (14-bit, scan off, 1 conversión) pero NO genera ni la
 * calibración ni un ConfigChannel (el sensor queda como canal interno disponible,
 * sin rank). Aquí: calibramos una vez (lazy) y leemos en single-shot el sensor de
 * temperatura, corrigiendo el VDDA real con VREFINT. La conversión usa los macros
 * de fábrica __LL_ADC_CALC_* (TS_CAL1/CAL2 y VREFINT_CAL del die, en flash). */
extern ADC_HandleTypeDef hadc1;

static int s_adc_calibrated = 0;

/* Single-shot de un canal del ADC1: los internos (TEMPSENSOR / VREFINT, con `arranque_ms` = 1 para que arranquen el
 * sensor y el buffer) y, desde V7/H4, los externos (0). Devuelve raw (14 bits) o -1. */
static int stm32_adc_read_canal(uint32_t channel, int arranque_ms) {
    ADC_ChannelConfTypeDef c = {0};
    c.Channel      = channel;
    c.Rank         = ADC_REGULAR_RANK_1;
    c.SamplingTime = ADC_SAMPLETIME_814CYCLES;  /* canales internos → muestreo largo */
    c.SingleDiff   = ADC_SINGLE_ENDED;
    c.OffsetNumber = ADC_OFFSET_NONE;
    c.Offset       = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &c) != HAL_OK) return -1;
    if (arranque_ms > 0) HAL_Delay((uint32_t) arranque_ms);   /* arranque del sensor/buffer (TSEN/VREFEN) */
    if (HAL_ADC_Start(&hadc1) != HAL_OK) return -1;
    int raw = -1;
    if (HAL_ADC_PollForConversion(&hadc1, 50) == HAL_OK)
        raw = (int) HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return raw;
}

static float stm32_temp_c_impl(void) {
    if (!s_adc_calibrated) {
        /* Calibración de offset (CubeMX no la generó); deja el ADC listo. */
        if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
            return 0.0f;
        s_adc_calibrated = 1;
    }
    /* VDDA real vía VREFINT; si falla, 3.3 V nominales. */
    uint32_t vdda_mv = 3300u;
    int vref = stm32_adc_read_canal(ADC_CHANNEL_VREFINT, 1);
    if (vref > 0)
        vdda_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(ADC1, (uint32_t) vref, LL_ADC_RESOLUTION_14B);
    int ts = stm32_adc_read_canal(ADC_CHANNEL_TEMPSENSOR, 1);
    if (ts < 0) return 0.0f;
    int32_t t = __LL_ADC_CALC_TEMPERATURE(ADC1, vdda_mv, (uint32_t) ts, LL_ADC_RESOLUTION_14B);
    return (float) t;
}

/* ── V7/H4 (el resto de #469, 26-sep) — EL ADC EXTERNO: el ADC1, la unidad 1 de la tabla ────────────────────
 * Qué pin es qué canal lo dice la tabla (s_adc_mapa, de las hojas de datos); aquí sólo lo que toca el silicio: el pin
 * en modo analógico y una conversión por sondeo con el mismo hadc1 que la temperatura (14 bits). El contrato de BP es
 * de 12 (0..4095; `readVolts` = raw · 3,3 / 4095): se quitan los 2 bits bajos. Muestreo largo (814,5 ciclos), el de
 * los canales internos: un divisor de decenas de kΩ necesita tiempo para cargar el condensador de muestreo, y aquí la
 * velocidad no manda. */
static int stm32_adc_init_channel_impl(int ch) {
    int pin = bpvm_caps_pin_de_adc(0, ch);
    GPIO_TypeDef* port = (pin >= 0) ? port_of(pin >> 4) : NULL;
    if (!port) return -1;
    enable_port_clk(pin >> 4);
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin  = (uint32_t) (1u << (pin & 0x0F));
    cfg.Mode = GPIO_MODE_ANALOG;
    cfg.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &cfg);
    return pin;
}

static int stm32_adc_read_channel_impl(int ch) {
    if (bpvm_caps_pin_de_adc(0, ch) < 0) return -1;
    if (!s_adc_calibrated) {
        if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK) return -1;
        s_adc_calibrated = 1;
    }
    int raw = stm32_adc_read_canal(__LL_ADC_DECIMAL_NB_TO_CHANNEL((uint32_t) ch), 0);
    return (raw < 0) ? -1 : (raw >> 2);   /* 14 → 12 bits */
}

static const bpvm_adc_backend_t s_stm32_adc_backend = {
    .initChannel = stm32_adc_init_channel_impl,
    .readChannel = stm32_adc_read_channel_impl,
};
#else
static float stm32_temp_c_impl(void) {
    return 0.0f;   /* placa sin BOARD_HAS_ADC_TEMP: sensor no disponible */
}
#endif

static int stm32_set_cpu_freq_mhz_impl(int mhz) {
    (void) mhz;
    return 0;   /* reloj fijo (160 MHz); cambiar en runtime no soportado */
}

/* ===================== Breadcrumb — H10, V7/H4 (#487) ====================
 * El algoritmo vive ahora en el común (bpvm_machine.c); aquí sólo el BLOQUE
 * que este silicio conserva entre resets: los registros de backup del TAMP
 * (BKP0R..BKP31R, 32 palabras contiguas en 0x100..0x17C; no se borran en un
 * reset del sistema y, con VBAT, tampoco en power-off). Se usan 20. */
static void stm32_breadcrumb_init(void) {
    __HAL_RCC_RTCAPB_CLK_ENABLE();   /* acceso al TAMP (dominio RTC/backup) */
    HAL_PWR_EnableBkUpAccess();      /* DBP: permite escribir el dominio backup */
    bpvm_breadcrumb_attach((uint32_t*) &TAMP->BKP0R, 20u);
}

/* Los DOS nombres (V6, 9-sep): el micro es la familia de silicio; la PLACA sale
 * de BOARD_NAME, que ya distingue la Nucleo de la DK2 (son dos proyectos). */
static void stm32_micro_name_impl(char* buf, size_t len) {
    if (buf && len > 0) { strncpy(buf, "stm32u5", len - 1); buf[len - 1] = '\0'; }
}

static const bpvm_machine_backend_t s_pico_backend = {
    .uniqueId      = stm32_unique_id_impl,
    .microName     = stm32_micro_name_impl,
    .boardName     = stm32_board_name_impl,
    .tempC         = stm32_temp_c_impl,
    .cpuFreqHz     = stm32_cpu_freq_hz_impl,
    .uptimeMs      = stm32_uptime_ms_impl,
    .setCpuFreqMHz = stm32_set_cpu_freq_mhz_impl,
    .resetCause    = stm32_reset_cause,        /* H10 — causa del último reset */
};

/* ===================== SPI (H15.1) =====================================
 * Backend SPI sobre la HAL. v3 AUTO-CONFIG (HECHO): el constructor activa el
 * periferico + clock + pines del .bp (handles propios), SIN depender del init de
 * CubeMX en el boot -> el bus NO reserva pines (los elige el programa). bus N ->
 * SPIn (SPI1/2 = AF5, SPI3 = AF6). El CS lo gestiona el usuario por GPIO, igual
 * que en la Pico. Reloj de SPI = SYSCLK (160 MHz) -> el prescaler se elige del
 * baud pedido.
 */
static SPI_HandleTypeDef s_spi[3];   /* handles propios (v3); [0]=SPI1 [1]=SPI2 [2]=SPI3 */

static SPI_HandleTypeDef* spi_handle(int bus) {
    return (bus >= 1 && bus <= 3) ? &s_spi[bus - 1] : NULL;
}

/* Configura un pin (modelo plano puerto<<4|bit) en Alternate Function. */
static void cfg_af_pin(int pin, uint8_t af) {
    int idx = pin >> 4;
    int bit = pin & 0x0F;
    GPIO_TypeDef* port = port_of(idx);
    if (!port) return;
    enable_port_clk(idx);
    GPIO_InitTypeDef g = {0};
    g.Pin       = (uint32_t) (1u << bit);
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = af;
    HAL_GPIO_Init(port, &g);
}

/* Prescaler SPI que da la mayor frecuencia <= baud desde SYSCLK (160 MHz). */
static uint32_t spi_prescaler_for(int baud) {
    uint32_t fclk     = 160000000u;
    uint32_t div[8]   = { 2, 4, 8, 16, 32, 64, 128, 256 };
    uint32_t code[8]  = {
        SPI_BAUDRATEPRESCALER_2,   SPI_BAUDRATEPRESCALER_4,
        SPI_BAUDRATEPRESCALER_8,   SPI_BAUDRATEPRESCALER_16,
        SPI_BAUDRATEPRESCALER_32,  SPI_BAUDRATEPRESCALER_64,
        SPI_BAUDRATEPRESCALER_128, SPI_BAUDRATEPRESCALER_256
    };
    int i;
    for (i = 0; i < 8; i++) {
        if (baud > 0 && (fclk / div[i]) <= (uint32_t) baud) return code[i];
    }
    return SPI_BAUDRATEPRESCALER_256;   /* baud muy bajo -> lo mas lento */
}

static void stm32_spi_init_impl(int bus, int sck, int mosi, int miso,
                                int baudrate, int mode) {
    SPI_HandleTypeDef* h = spi_handle(bus);
    if (!h) return;
    SPI_TypeDef* inst;
    switch (bus) {                                 /* instancia + clock (antes MspInit de CubeMX) */
        case 1: inst = SPI1; __HAL_RCC_SPI1_CLK_ENABLE(); break;
        case 2: inst = SPI2; __HAL_RCC_SPI2_CLK_ENABLE(); break;
        case 3: inst = SPI3; __HAL_RCC_SPI3_CLK_ENABLE(); break;
        default: return;
    }
    /* init COMPLETO (los 21 campos que ponia CubeMX); maestro, 8 bit, NSS soft. */
    h->Instance               = inst;
    h->Init.Mode              = SPI_MODE_MASTER;
    h->Init.Direction         = SPI_DIRECTION_2LINES;
    h->Init.DataSize          = SPI_DATASIZE_8BIT;
    h->Init.CLKPolarity       = (mode & 0x2) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    h->Init.CLKPhase          = (mode & 0x1) ? SPI_PHASE_2EDGE   : SPI_PHASE_1EDGE;
    h->Init.NSS               = SPI_NSS_SOFT;
    h->Init.BaudRatePrescaler = spi_prescaler_for(baudrate);
    h->Init.FirstBit          = SPI_FIRSTBIT_MSB;
    h->Init.TIMode            = SPI_TIMODE_DISABLE;
    h->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    h->Init.CRCPolynomial     = 0x7;
    h->Init.NSSPMode          = SPI_NSS_PULSE_ENABLE;
    h->Init.NSSPolarity       = SPI_NSS_POLARITY_LOW;
    h->Init.FifoThreshold     = SPI_FIFO_THRESHOLD_01DATA;
    h->Init.MasterSSIdleness        = SPI_MASTER_SS_IDLENESS_00CYCLE;
    h->Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    h->Init.MasterReceiverAutoSusp  = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    h->Init.MasterKeepIOState       = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    h->Init.IOSwap            = SPI_IO_SWAP_DISABLE;
    h->Init.ReadyMasterManagement   = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
    h->Init.ReadyPolarity     = SPI_RDY_POLARITY_HIGH;
    HAL_SPI_Init(h);
    /* asigna los pines que pidio el usuario (SPI1/2 = AF5, SPI3 = AF6) */
    uint8_t af = (bus == 3) ? GPIO_AF6_SPI3 : GPIO_AF5_SPI1;
    cfg_af_pin(sck,  af);
    cfg_af_pin(mosi, af);
    cfg_af_pin(miso, af);
}

/* V7/H6.c G4.a3 — la HAL cuenta en `uint16_t`: más de 65 535 bytes se TRUNCABAN en
 * silencio (y una operación de una transacción nativa puede llevarlos: el tope es de
 * tiempo, no de bytes). Troceado; el CS es un GPIO que lleva la HAL BP todo el turno. */
#define STM32_SPI_TROZO 65535u
static int stm32_spi_write_impl(int bus, const uint8_t* data, size_t n) {
    SPI_HandleTypeDef* h = spi_handle(bus);
    size_t hecho = 0;
    if (!h) return -1;
    while (hecho < n) {
        uint16_t t = (uint16_t) ((n - hecho > STM32_SPI_TROZO) ? STM32_SPI_TROZO : (n - hecho));
        if (HAL_SPI_Transmit(h, (uint8_t*) data + hecho, t, 1000) != HAL_OK) return -1;
        hecho += t;
    }
    return (int) n;
}

static int stm32_spi_read_impl(int bus, uint8_t* data, size_t n) {
    SPI_HandleTypeDef* h = spi_handle(bus);
    size_t hecho = 0;
    if (!h) return -1;
    while (hecho < n) {
        uint16_t t = (uint16_t) ((n - hecho > STM32_SPI_TROZO) ? STM32_SPI_TROZO : (n - hecho));
        if (HAL_SPI_Receive(h, data + hecho, t, 1000) != HAL_OK) return -1;
        hecho += t;
    }
    return (int) n;
}

static int stm32_spi_transfer_impl(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    SPI_HandleTypeDef* h = spi_handle(bus);
    size_t hecho = 0;
    if (!h) return -1;
    while (hecho < n) {
        uint16_t t = (uint16_t) ((n - hecho > STM32_SPI_TROZO) ? STM32_SPI_TROZO : (n - hecho));
        if (HAL_SPI_TransmitReceive(h, (uint8_t*) tx + hecho, rx + hecho, t, 1000) != HAL_OK) return -1;
        hecho += t;
    }
    return (int) n;
}

/* V7/H6.c G4.a1 — abrir UNA vez, ajustar aparte, cerrar. La regla de quién usa
 * el bus y con qué pines es de la HAL BP (bpvm_bus.c); aquí sólo el hardware:
 *   abrir   — el init de siempre, SÓLO si el bus no está abierto;
 *   ajustar — reconfigurar el periférico (prescaler, CPOL/CPHA) con los pines a
 *             -1, que `cfg_af_pin` ignora: HAL_SPI_Init sobre un handle ya READY
 *             no vuelve a pasar por el MspInit, sólo reescribe los registros;
 *   cerrar  — HAL_SPI_DeInit. */
static int s_spi_abierto[3], s_spi_hz[3], s_spi_modo[3];
static int stm32_spi_abrir(int bus, int sck, int mosi, int miso) {
    if (!spi_handle(bus)) return -1;
    if (s_spi_abierto[bus - 1]) return 0;                /* ya abierto: NO se reabre */
    stm32_spi_init_impl(bus, sck, mosi, miso, 1000000, 0);
    s_spi_abierto[bus - 1] = 1;
    s_spi_hz[bus - 1] = 1000000; s_spi_modo[bus - 1] = 0;
    return 0;
}
static void stm32_spi_ajustar(int bus, int baudrate, int mode) {
    if (!spi_handle(bus) || !s_spi_abierto[bus - 1]) return;
    if (s_spi_hz[bus - 1] == baudrate && s_spi_modo[bus - 1] == mode) return;
    stm32_spi_init_impl(bus, -1, -1, -1, baudrate, mode);
    s_spi_hz[bus - 1] = baudrate; s_spi_modo[bus - 1] = mode;
}
static void stm32_spi_cerrar(int bus) {
    SPI_HandleTypeDef* h = spi_handle(bus);
    if (!h || !s_spi_abierto[bus - 1]) return;
    HAL_SPI_DeInit(h);
    s_spi_abierto[bus - 1] = 0;
}

static const bpvm_spi_backend_t s_spi_backend = {
    .init     = stm32_spi_init_impl,
    .write    = stm32_spi_write_impl,
    .read     = stm32_spi_read_impl,
    .transfer = stm32_spi_transfer_impl,
    .abrir    = stm32_spi_abrir,       /* V7/H6.c G4.a1 */
    .ajustar  = stm32_spi_ajustar,
    .cerrar   = stm32_spi_cerrar,
};

/* ===================== UART (H15.1) ====================================
 * Igual que SPI (v3 auto-config): handles propios; el constructor activa
 * periferico + clock + pines del .bp, SIN CubeMX -> no se reservan pines en el
 * boot. bus N -> USARTn/UARTn (USART2/3/6 = AF7, UART4/5 = AF8); el chip decide
 * cuantas (via #ifdef del CMSIS). El VCP del ST-LINK usa USART1 -> NO se toca.
 *
 * read(timeout): HAL_UART_Receive devuelve TIMEOUT si no completa n bytes a
 * tiempo; los recibidos = n - RxXferCount. available(): no soportado en el MVP
 * (-1, segun el contrato del header) -> usar read con timeout.
 */
static UART_HandleTypeDef s_uart[7];   /* handles propios (v3); indexado por nº de bus (2..6) */

static UART_HandleTypeDef* uart_handle(int bus) {
    switch (bus) {                   /* expone las instancias que tenga el chip (#ifdef del CMSIS) */
        case 2: case 3: case 4:
#ifdef UART5
        case 5:
#endif
#ifdef USART6
        case 6:
#endif
            return &s_uart[bus];
        default: return NULL;
    }
}

/* ─── V6/#473 — el goteo del anillo de RX: una IRQ por bus ───────────────────
 *
 * Aqui ya se sabia que el problema existia, y se habia parcheado a medias: el
 * comentario del init de abajo cuenta que se encendio el FIFO de 8 bytes porque
 * «el RX en polling pierde por overrun los bytes que llegan en rafaga (un loopback
 * de 4 bytes solo capturaba el primero)». Ocho bytes es exactamente lo que Eduardo
 * describe: un buffer que siempre se queda corto. El anillo de 512 de la fachada
 * comun (src/uart.c) es el buffer de verdad; esto solo lo alimenta.
 *
 * Los handlers son DEBILES en el startup, asi que definirlos aqui los sustituye
 * sin tocar ni una linea de codigo generado por CubeMX — que es la norma de la
 * casa: al generador no se le parchea el resultado.
 *
 * La ISR no llama a nada de FreeRTOS (solo escribe en el anillo), asi que su
 * prioridad no esta atada al planificador; usa la misma que el wire por coherencia. */
static void stm32_uart_isr_bus(int bus) {
    UART_HandleTypeDef* h = uart_handle(bus);
    if (!h || !h->Instance) return;
    /* Overrun: limpiarlo o la RX se queda muda para siempre. */
    if (__HAL_UART_GET_FLAG(h, UART_FLAG_ORE)) __HAL_UART_CLEAR_OREFLAG(h);
    while (__HAL_UART_GET_FLAG(h, UART_FLAG_RXNE)) {   /* RXFNE con FIFO on */
        bpvm_uart_rx_push(bus, (uint8_t) (h->Instance->RDR & 0xFFu));
    }
}

void USART2_IRQHandler(void) { stm32_uart_isr_bus(2); }
void USART3_IRQHandler(void) { stm32_uart_isr_bus(3); }
void UART4_IRQHandler(void)  { stm32_uart_isr_bus(4); }
#ifdef UART5
void UART5_IRQHandler(void)  { stm32_uart_isr_bus(5); }
#endif
#ifdef USART6
void USART6_IRQHandler(void) { stm32_uart_isr_bus(6); }
#endif

#ifndef BOARD_UART_IRQ_PRIO
#define BOARD_UART_IRQ_PRIO 5     /* la misma que el wire (board.h: BOARD_WIRE_IRQ_PRIO) */
#endif

static void stm32_uart_rx_irq_on(int bus) {
    UART_HandleTypeDef* h = uart_handle(bus);
    IRQn_Type irq;
    if (!h) return;
    switch (bus) {
        case 2: irq = USART2_IRQn; break;
        case 3: irq = USART3_IRQn; break;
        case 4: irq = UART4_IRQn;  break;
#ifdef UART5
        case 5: irq = UART5_IRQn;  break;
#endif
#ifdef USART6
        case 6: irq = USART6_IRQn; break;
#endif
        default: return;
    }
    HAL_NVIC_SetPriority(irq, BOARD_UART_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(irq);
    __HAL_UART_ENABLE_IT(h, UART_IT_RXNE);
    bpvm_uart_rx_ring_enable(bus);
}

static int s_uart_tx[8] = { -1, -1, -1, -1, -1, -1, -1, -1 }, s_uart_rx[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };   /* V7 #667 */
static void stm32_uart_init_impl(int bus, int tx, int rx, int baudrate,
                                 int data_bits, int stop_bits, int parity) {
    UART_HandleTypeDef* h = uart_handle(bus);
    if (!h) return;
    if (bus >= 0 && bus < 8) { s_uart_tx[bus] = tx; s_uart_rx[bus] = rx; }
    USART_TypeDef* inst;
    switch (bus) {                                 /* instancia + clock (antes MspInit de CubeMX) */
        case 2: inst = USART2; __HAL_RCC_USART2_CLK_ENABLE(); break;
        case 3: inst = USART3; __HAL_RCC_USART3_CLK_ENABLE(); break;
        case 4: inst = UART4;  __HAL_RCC_UART4_CLK_ENABLE();  break;
#ifdef UART5
        case 5: inst = UART5;  __HAL_RCC_UART5_CLK_ENABLE();  break;
#endif
#ifdef USART6
        case 6: inst = USART6; __HAL_RCC_USART6_CLK_ENABLE(); break;
#endif
        default: return;
    }
    h->Instance      = inst;
    h->Init.BaudRate = (uint32_t) baudrate;        /* el HAL calcula el BRR del reloj vivo */
    /* En STM32 el WordLength CUENTA el bit de paridad: 8 datos sin paridad = 8B;
     * 8 datos + paridad = 9B. */
    if (parity != 0) {
        h->Init.WordLength = UART_WORDLENGTH_9B;
    } else if (data_bits == 7) {
        h->Init.WordLength = UART_WORDLENGTH_7B;
    } else {
        h->Init.WordLength = UART_WORDLENGTH_8B;
    }
    h->Init.StopBits = (stop_bits == 2) ? UART_STOPBITS_2 : UART_STOPBITS_1;
    h->Init.Parity   = (parity == 1) ? UART_PARITY_ODD
                     : (parity == 2) ? UART_PARITY_EVEN
                                     : UART_PARITY_NONE;
    h->Init.Mode           = UART_MODE_TX_RX;
    h->Init.HwFlowCtl      = UART_HWCONTROL_NONE;
    h->Init.OverSampling   = UART_OVERSAMPLING_16;
    h->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    h->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    h->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(h);
    /* FIFO RX (8 bytes): sin el, el RX en polling pierde por overrun los bytes
     * que llegan en rafaga (un loopback de 4 bytes solo capturaba el primero).
     * CubeMX lo deja deshabilitado por defecto; lo habilitamos aqui. */
    HAL_UARTEx_SetTxFifoThreshold(h, UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(h, UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_EnableFifoMode(h);
    uint8_t af = (bus == 2 || bus == 3 || bus == 6) ? GPIO_AF7_USART3   /* USART2/3/6 = AF7 */
                                                    : GPIO_AF8_UART4;  /* UART4/5 = AF8 */
    cfg_af_pin(tx, af);
    cfg_af_pin(rx, af);
    stm32_uart_rx_irq_on(bus);   /* V6/#473: y a partir de aqui, anillo de 512 B */
}

static int stm32_uart_write_impl(int bus, const uint8_t* data, size_t n) {
    UART_HandleTypeDef* h = uart_handle(bus);
    if (!h) return -1;
    return (HAL_UART_Transmit(h, (uint8_t*) data, (uint16_t) n, 1000) == HAL_OK)
           ? (int) n : -1;
}

static int stm32_uart_read_impl(int bus, uint8_t* data, size_t n, int timeout_ms) {
    UART_HandleTypeDef* h = uart_handle(bus);
    if (!h) return -1;
    uint32_t to = (timeout_ms <= 0) ? HAL_MAX_DELAY : (uint32_t) timeout_ms;
    HAL_StatusTypeDef st = HAL_UART_Receive(h, data, (uint16_t) n, to);
    if (st == HAL_OK) return (int) n;
    if (st == HAL_TIMEOUT) return (int) ((uint16_t) n - h->RxXferCount);  /* bytes recibidos */
    return -1;
}

static int stm32_uart_available_impl(int bus) {
    (void) bus;
    return -1;   /* MVP: no soportado (contrato del header) -> usar read con timeout */
}

/* V7 #667 — soltarla: su interrupción fuera, el periférico desinicializado y sus pines al reset. */
static void stm32_uart_release_impl(int bus, int tx, int rx) {
    UART_HandleTypeDef* h = uart_handle(bus);
    if (!h || !h->Instance || bus < 0 || bus >= 8) return;
    IRQn_Type irq;
    switch (bus) {
        case 2: irq = USART2_IRQn; break;
        case 3: irq = USART3_IRQn; break;
        case 4: irq = UART4_IRQn;  break;
#ifdef UART5
        case 5: irq = UART5_IRQn;  break;
#endif
#ifdef USART6
        case 6: irq = USART6_IRQn; break;
#endif
        default: return;
    }
    __HAL_UART_DISABLE_IT(h, UART_IT_RXNE);
    HAL_NVIC_DisableIRQ(irq);
    HAL_UART_DeInit(h);
    stm32_pin_al_reset(tx >= 0 ? tx : s_uart_tx[bus]);
    stm32_pin_al_reset(rx >= 0 ? rx : s_uart_rx[bus]);
    s_uart_tx[bus] = s_uart_rx[bus] = -1;
}

static const bpvm_uart_backend_t s_uart_backend = {
    .init      = stm32_uart_init_impl,
    .release   = stm32_uart_release_impl,   /* V7 #667 */
    .write     = stm32_uart_write_impl,
    .read      = stm32_uart_read_impl,
    .available = stm32_uart_available_impl,
};

/* ===================== I2C (H15.2) =====================================
 * Backend I2C sobre la HAL. Igual que SPI/UART (v3 auto-config): handles
 * propios; el constructor activa periferico + clock + pines (100 kHz, 7-bit),
 * SIN CubeMX. bus N -> I2Cn (el chip decide cuantas, via #ifdef; U575=1..4,
 * U5G9=1..6). En el U5 TODAS las instancias I2C usan AF4.
 *
 * OPEN-DRAIN, no push-pull: a diferencia de SPI/UART, las lineas I2C
 * (SDA/SCL) solo tiran a 0 y flotan a 1 -> se configuran AF_OD y necesitan
 * pull-ups EXTERNAS a VDD (4k7 tipico). Normalmente ya las trae el modulo
 * del sensor (los breakouts BMP280 GY-* y Adafruit las llevan). NO usamos el
 * pull-up interno del STM32 (~40k): demasiado debil para el tiempo de
 * subida de un bus I2C fiable -> mejor externas, como hace CubeMX (NOPULL).
 *
 * Direccion: el contrato del backend usa addr de 7 bits (igual que el SDK
 * del Pico); la HAL de ST la quiere desplazada a 8 bits -> addr << 1.
 *
 * Baud: MVP usa el Timing de CubeMX (100 kHz). Cambiarlo exige recalcular
 * el registro TIMINGR (no trivial); 100 kHz cubre el BMP280 y casi todo.
 */
/* v3 "lo correcto": handles PROPIOS; init completo en el constructor, sin
 * MX_I2Cn_Init de CubeMX -> los pines no se reservan en el boot. bus N -> I2Cn.
 * OJO DK2: I2C2 lo usa el TACTIL (GT911) via CubeMX -> en la DK2 NO uses el bus 2
 * desde BP (chocaria con el tactil). Desde #669 sus pines (PF0, PF1) son de la pantalla
 * (gui_display_ltdc.c): un I2c.Bus(2) en ellos recibe «es de la pantalla»; con OTROS pines
 * del I2C2 todavia chocaria (el bus del tactil: ver FICHAS #669). */
static I2C_HandleTypeDef s_i2c[7];   /* indexado por nº de bus (1..6) */

static I2C_HandleTypeDef* i2c_handle(int bus) {
    switch (bus) {                   /* expone las instancias que tenga el chip (#ifdef del CMSIS) */
        case 1: case 2:
#ifdef I2C3
        case 3:
#endif
#ifdef I2C4
        case 4:
#endif
#ifdef I2C5
        case 5:
#endif
#ifdef I2C6
        case 6:
#endif
            return &s_i2c[bus];
        default: return NULL;
    }
}

/* Igual que cfg_af_pin pero OPEN-DRAIN y sin pull interno (para I2C). */
static void cfg_af_pin_od(int pin, uint8_t af) {
    int idx = pin >> 4;
    int bit = pin & 0x0F;
    GPIO_TypeDef* port = port_of(idx);
    if (!port) return;
    enable_port_clk(idx);
    GPIO_InitTypeDef g = {0};
    g.Pin       = (uint32_t) (1u << bit);
    g.Mode      = GPIO_MODE_AF_OD;       /* open-drain (bus I2C) */
    g.Pull      = GPIO_NOPULL;           /* pull-ups EXTERNAS (modulo / 4k7) */
    g.Speed     = GPIO_SPEED_FREQ_LOW;   /* I2C 100k/400k: LOW basta (= CubeMX) */
    g.Alternate = af;
    HAL_GPIO_Init(port, &g);
}

static void stm32_i2c_init_impl(int bus, int sda, int scl, int baudrate) {
    I2C_HandleTypeDef* h = i2c_handle(bus);
    if (!h) return;
    (void) baudrate;   /* MVP: 100 kHz fijo (TIMINGR de abajo) -- ver cabecera */
    I2C_TypeDef* inst;
    switch (bus) {                                 /* instancia + clock (antes MspInit de CubeMX) */
        case 1: inst = I2C1; __HAL_RCC_I2C1_CLK_ENABLE(); break;
        case 2: inst = I2C2; __HAL_RCC_I2C2_CLK_ENABLE(); break;
#ifdef I2C3
        case 3: inst = I2C3; __HAL_RCC_I2C3_CLK_ENABLE(); break;
#endif
#ifdef I2C4
        case 4: inst = I2C4; __HAL_RCC_I2C4_CLK_ENABLE(); break;
#endif
#ifdef I2C5
        case 5: inst = I2C5; __HAL_RCC_I2C5_CLK_ENABLE(); break;
#endif
#ifdef I2C6
        case 6: inst = I2C6; __HAL_RCC_I2C6_CLK_ENABLE(); break;
#endif
        default: return;
    }
    /* init COMPLETO (antes CubeMX). TIMINGR = 100 kHz @ kernel 160 MHz; CubeMX usa
     * el mismo valor en I2C1-4 (PCLK1=PCLK3, todos los APB /1). I2C5/6 (solo U5G9)
     * asumen igual kernel clock -> verificar en placa si se usan. */
    h->Instance              = inst;
    h->Init.Timing           = 0x30909DEC;
    h->Init.OwnAddress1      = 0;
    h->Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    h->Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    h->Init.OwnAddress2      = 0;
    h->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    h->Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    h->Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(h);
    HAL_I2CEx_ConfigAnalogFilter(h, I2C_ANALOGFILTER_ENABLE);
    HAL_I2CEx_ConfigDigitalFilter(h, 0);
    /* pines del usuario: OPEN-DRAIN, AF4 (toda instancia I2C en el U5) */
    cfg_af_pin_od(sda, GPIO_AF4_I2C1);
    cfg_af_pin_od(scl, GPIO_AF4_I2C1);
}

static int stm32_i2c_write_impl(int bus, int addr, const uint8_t* data, size_t n) {
    I2C_HandleTypeDef* h = i2c_handle(bus);
    if (!h) return -1;
    /* Timeout corto: un device ausente responde NAK enseguida (no agota el
     * timeout), asi I2c.scan() recorre 0x08..0x77 sin colgarse; el timeout
     * solo cubre un bus electricamente atascado. */
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(h, (uint16_t) (addr << 1),
                                                   (uint8_t*) data, (uint16_t) n, 100);
    return (st == HAL_OK) ? (int) n : -1;
}

static int stm32_i2c_read_impl(int bus, int addr, uint8_t* data, size_t n) {
    I2C_HandleTypeDef* h = i2c_handle(bus);
    if (!h) return -1;
    HAL_StatusTypeDef st = HAL_I2C_Master_Receive(h, (uint16_t) (addr << 1),
                                                  data, (uint16_t) n, 100);
    return (st == HAL_OK) ? (int) n : -1;
}

/* V7/H6.c G4.a1 — abrir UNA vez y cerrar (ver el SPI). ⚠️ La velocidad sigue fija
 * en 100 kHz (el TIMINGR de init_impl): un `i2cN=…,khz:400` en esta familia hoy
 * va a 100. Es la deuda de antes, no de esto. */
static int s_i2c_abierto[7];
static int stm32_i2c_abrir(int bus, int sda, int scl, int baudrate) {
    if (!i2c_handle(bus)) return -1;
    if (s_i2c_abierto[bus]) return 0;                    /* ya abierto: NO se reabre */
    stm32_i2c_init_impl(bus, sda, scl, baudrate);
    s_i2c_abierto[bus] = 1;
    return 0;
}
static void stm32_i2c_cerrar(int bus) {
    I2C_HandleTypeDef* h = i2c_handle(bus);
    if (!h || !s_i2c_abierto[bus]) return;
    HAL_I2C_DeInit(h);
    s_i2c_abierto[bus] = 0;
}

/* V7/H6.c G4.a3 — LA TRANSACCIÓN, con repeated start, a nivel de REGISTRO: la HAL del U5
 * sólo la trae para registros de 1 o 2 bytes (HAL_I2C_Mem_Read) o con interrupciones
 * (HAL_I2C_Master_Seq_*_IT). El periférico I2C v2 la hace así —como la HAL por dentro—:
 * cada mensaje programa CR2 (dirección, sentido, NBYTES y START) y va byte a byte con
 * TXIS/RXNE; entre dos mensajes espera TC (sin AUTOEND el bus sigue siendo nuestro) y el
 * START del siguiente es un repeated start; el último lleva AUTOEND y el STOP lo pone el
 * periférico. Más de 255 bytes en un mensaje: RELOAD (TCR), como la HAL.
 * Un NAK: el periférico pone el STOP él solo (lo que espera la HAL tras NACKF); si no
 * llega a tiempo y el bus sigue ocupado, se pide a mano. Luego, flags y CR2 a limpio.
 * No toca el estado de la HAL: el turno del árbitro garantiza que nadie más usa este bus.
 * 0, o -1. */
#define STM32_I2C_PLAZO_MS 100u
static int i2c_espera(I2C_TypeDef* I, uint32_t flag, uint32_t t0) {
    while (!(I->ISR & flag)) {
        if (I->ISR & I2C_ISR_NACKF) return -1;
        if (HAL_GetTick() - t0 > STM32_I2C_PLAZO_MS) return -2;
    }
    return 0;
}
static int stm32_i2c_transaccion(int bus, int addr, const bpvm_i2c_msg_t* m, int nm) {
    I2C_HandleTypeDef* h = i2c_handle(bus);
    I2C_TypeDef* I;
    uint32_t t0 = HAL_GetTick();
    int i, r = 0;
    if (!h || !s_i2c_abierto[bus]) return -1;
    I = h->Instance;
    while (I->ISR & I2C_ISR_BUSY)
        if (HAL_GetTick() - t0 > STM32_I2C_PLAZO_MS) return -1;
    for (i = 0; i < nm && r == 0; i++) {
        uint32_t queda = m[i].n, hecho = 0, trozo = (queda > 255u) ? 255u : queda;
        int ultimo = (i == nm - 1);
        I->CR2 = (((uint32_t) addr << 1) & I2C_CR2_SADD) | (m[i].lee ? I2C_CR2_RD_WRN : 0u)
               | (trozo << I2C_CR2_NBYTES_Pos) | I2C_CR2_START
               | ((queda > 255u) ? I2C_CR2_RELOAD : (ultimo ? I2C_CR2_AUTOEND : 0u));
        while (queda > 0 && r == 0) {
            if (m[i].lee) { r = i2c_espera(I, I2C_ISR_RXNE, t0); if (r == 0) m[i].buf[hecho] = (uint8_t) I->RXDR; }
            else          { r = i2c_espera(I, I2C_ISR_TXIS, t0); if (r == 0) I->TXDR = m[i].buf[hecho]; }
            if (r != 0) break;
            hecho++; queda--; trozo--;
            if (trozo == 0 && queda > 0) {                        /* RELOAD: el trozo siguiente */
                r = i2c_espera(I, I2C_ISR_TCR, t0);
                if (r != 0) break;
                trozo = (queda > 255u) ? 255u : queda;
                I->CR2 = (I->CR2 & ~(I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_START))
                       | (trozo << I2C_CR2_NBYTES_Pos)
                       | ((queda > 255u) ? I2C_CR2_RELOAD : (ultimo ? I2C_CR2_AUTOEND : 0u));
            }
        }
        if (r == 0 && !ultimo) r = i2c_espera(I, I2C_ISR_TC, t0);   /* el bus, aún nuestro */
    }
    if (r == 0) {
        r = i2c_espera(I, I2C_ISR_STOPF, t0);                          /* el STOP del AUTOEND */
    } else {
        uint32_t t1 = HAL_GetTick();
        while (!(I->ISR & I2C_ISR_STOPF) && HAL_GetTick() - t1 <= STM32_I2C_PLAZO_MS) { }
        if (!(I->ISR & I2C_ISR_STOPF) && (I->ISR & I2C_ISR_BUSY)) {
            I->CR2 |= I2C_CR2_STOP;
            t1 = HAL_GetTick();
            while (!(I->ISR & I2C_ISR_STOPF) && HAL_GetTick() - t1 <= STM32_I2C_PLAZO_MS) { }
        }
    }
    I->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
    if (I->ISR & I2C_ISR_TXIS) I->TXDR = 0u;                            /* vaciar TXDR, como la HAL */
    if (!(I->ISR & I2C_ISR_TXE)) I->ISR |= I2C_ISR_TXE;
    I->CR2 &= ~(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);
    return (r == 0) ? 0 : -1;
}

static const bpvm_i2c_backend_t s_i2c_backend = {
    .init   = stm32_i2c_init_impl,
    .write  = stm32_i2c_write_impl,
    .read   = stm32_i2c_read_impl,
    .abrir  = stm32_i2c_abrir,         /* V7/H6.c G4.a1 */
    .cerrar = stm32_i2c_cerrar,
    .transaccion = stm32_i2c_transaccion,   /* V7/H6.c G4.a3: repeated start (a probar en la Nucleo) */
};

/* ===================== WDT — NO LO HAY EN ESTA FAMILIA (#480) ==============
 *
 * Aqui vivia el backend de watchdog sobre el IWDG (enable/feed/disable, el
 * calculo de PR/RLR y su registro). Se ha QUITADO, no comentado — git lo
 * guarda, y codigo muerto son avisos del compilador para siempre.
 *
 * POR QUE. El IWDG del U5 no se puede parar por software: una vez arrancado
 * (KR = 0xCCCC) solo lo apaga un reset. Eso es silicio, no un descuido. Asi que
 * `disable()` era un "mejor esfuerzo" que lo reprogramaba a ~131 s y lo
 * refrescaba, con la bandera de activo todavia puesta. Consecuencia REAL: la
 * placa se resetea sola si un sleep pasa de ahi — haciendo justo lo que
 * recomienda bpstdlib/Wdt.bp —, y desde BP no habia forma de enterarse, porque
 * el builtin devolvia 0 en las cinco familias.
 *
 * Criterio de Eduardo (9-sep-2026): «el watchdog se ha de implementar COMPLETO
 * o no se implementa. Lo del STM32 o se arregla o se desactiva completamente,
 * no podemos tener medio watchdog.» Arreglarlo no se puede, asi que esta imagen
 * NO registra backend de watchdog y los tres verbos lanzan una excepcion BP
 * atrapable ("no implementado en esta plataforma"), igual en las dos VMs.
 *
 * Si algun dia hay forma de pararlo de verdad (otro silicio, o una ventana
 * WWDG que si se pueda cerrar), se vuelve a registrar el backend COMPLETO: los
 * tres verbos o ninguno. */

/* ===================== Causa de reset — H10 ================================
 * Decodifica RCC->CSR (flags de la última causa de reset). Se lee/latchea UNA
 * vez —la 1ª llamada limpia los flags con RMVF— para que el valor sirva tanto
 * en el boot (print de diagnóstico) como, más adelante, desde BP. Prioridad:
 * causas específicas antes que PIN/power, que suelen co-activarse. */
const char* stm32_reset_cause(void) {
    static const char* s_cause = NULL;
    if (s_cause) return s_cause;
    uint32_t csr = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF;                  /* limpiar para el próximo reset */
    if      (csr & RCC_CSR_IWDGRSTF) s_cause = "watchdog (IWDG)";
    else if (csr & RCC_CSR_WWDGRSTF) s_cause = "window-watchdog (WWDG)";
    else if (csr & RCC_CSR_SFTRSTF)  s_cause = "software";
    else if (csr & RCC_CSR_LPWRRSTF) s_cause = "low-power";
    else if (csr & RCC_CSR_BORRSTF)  s_cause = "power-on/brown-out";
    else if (csr & RCC_CSR_PINRSTF)  s_cause = "pin (NRST)";
    else if (csr & RCC_CSR_OBLRSTF)  s_cause = "option-byte-loader";
    else                             s_cause = "desconocido";
    return s_cause;
}

#if defined(BOARD_HAS_RTC)
/* ===================== RTC HW — H10 =========================================
 * Reloj de calendario sobre el periférico RTC (CubeMX: hrtc, reloj LSI ~32 kHz).
 * La hora SOBREVIVE al reset (dominio backup; con VBAT/pila, al power-off).
 * Contrato BP = epoch ms (UTC) → convertimos a/desde fecha+hora del RTC. OJO:
 * con LSI deriva (~±%, no es cristal); para precisión, poblar el LSE. */
extern RTC_HandleTypeDef hrtc;

/* días desde 1970-01-01 ↔ (año, mes[1-12], día) — algoritmo civil (Hinnant). */
static long rtc_days_from_civil(long y, unsigned m, unsigned d) {
    y -= (m <= 2);
    long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153u * (m + (m > 2 ? -3u : 9u)) + 2u) / 5u + d - 1u;
    unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return era * 146097L + (long)doe - 719468L;
}
static void rtc_civil_from_days(long z, int* y, unsigned* m, unsigned* d) {
    z += 719468L;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097L);
    unsigned yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    long yy = (long)yoe + era * 400L;
    unsigned doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    unsigned mp = (5u * doy + 2u) / 153u;
    *d = doy - (153u * mp + 2u) / 5u + 1u;
    *m = mp + (mp < 10u ? 3u : -9u);
    *y = (int)(yy + (*m <= 2));
}

static int64_t stm32_rtc_now_ms(void) {
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);   /* GetTime ANTES de GetDate (bloqueo del shadow) */
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
    long days = rtc_days_from_civil(2000 + (int)d.Year, d.Month, d.Date);  /* RTC Year = año−2000 */
    int64_t secs = (int64_t)days * 86400 + t.Hours * 3600 + t.Minutes * 60 + t.Seconds;
    return secs * 1000;
}

static void stm32_rtc_set_now_ms(int64_t epoch_ms) {
    int64_t secs = epoch_ms / 1000;
    if (secs < 0) secs = 0;
    long days = (long)(secs / 86400);
    int rem = (int)(secs % 86400);
    int y; unsigned m, dd;
    rtc_civil_from_days(days, &y, &m, &dd);
    if (y < 2000) y = 2000;   /* el RTC guarda el año 0..99 desde 2000 */
    RTC_DateTypeDef d = {0};
    d.Year    = (uint8_t)(y - 2000);
    d.Month   = (uint8_t)m;
    d.Date    = (uint8_t)dd;
    d.WeekDay = (uint8_t)(((days + 3) % 7) + 1);   /* 1970-01-01 = jueves; 1=lun..7=dom */
    RTC_TimeTypeDef t = {0};
    t.Hours   = (uint8_t)(rem / 3600);
    t.Minutes = (uint8_t)((rem % 3600) / 60);
    t.Seconds = (uint8_t)(rem % 60);
    HAL_RTC_SetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &d, RTC_FORMAT_BIN);
}

static const bpvm_rtc_backend_t s_rtc_backend = {
    .nowMs    = stm32_rtc_now_ms,
    .setNowMs = stm32_rtc_set_now_ms,
};
#endif /* BOARD_HAS_RTC */

/* ===================== PWM — H10 ===========================================
 * PWM por timer HW. Tabla pin → (timer, canal, AF) de los pines soportados.
 * HOY: LEDs de la Nucleo verde PC7=TIM3_CH2 y azul PB7=TIM4_CH2 (rojo PG2 no
 * tiene canal de timer → no entra). Self-config (sin CubeMX): initSlice arranca
 * el timer (PSC/ARR para la freq, ARR=999 → 1000 pasos de duty) + pone el pin
 * en AF + canal PWM. setDuty = CCR % (PWM1, activo-alto = LED active-high).
 * OJO: al poner el pin en AF, deja de ser el LED de estado del firmware. */
typedef struct {
    int          pin;       /* pin BP = (puerto<<4)|bit */
    TIM_TypeDef* inst;      /* TIM3 / TIM4 */
    uint32_t     channel;   /* TIM_CHANNEL_x */
    uint8_t      af;        /* GPIO_AFx_TIMy */
} pwm_map_t;

static const pwm_map_t s_pwm_map[] = {
#if defined(BPVM_BOARD_DK2)
    { (1 << 4) | 8, TIM16, TIM_CHANNEL_1, GPIO_AF14_TIM16 },  /* PB8 — CN1 pin 12 (PWM; TIM16 libre) */
#else
    { (2 << 4) | 7, TIM3, TIM_CHANNEL_2, GPIO_AF2_TIM3 },  /* PC7 — LED verde */
    { (1 << 4) | 7, TIM4, TIM_CHANNEL_2, GPIO_AF2_TIM4 },  /* PB7 — LED azul  */
    { (2 << 4) | 8, TIM3, TIM_CHANNEL_3, GPIO_AF2_TIM3 },  /* PC8 — pin de cabecera (fuente PWM para probar el contador) */
#endif
};
#define PWM_SLICE_N    ((int)(sizeof(s_pwm_map) / sizeof(s_pwm_map[0])))

/* V7/H4 (el resto de #469, 26-sep) — EL MAPA DEL ADC, leído de las hojas de datos (DS13737 del U575 y la del
 * U5G7/U5G9, tabla «pin definitions»; las dos dan el mismo): los 17 canales del ADC1 —la unidad 1 de la tabla, la que
 * atiende el backend— y los 7 pines que sólo tiene el ADC4 (unidad 2: error con nombre). Pines en el modelo plano
 * puerto<<4|bit. El U5G9 tiene además un ADC2, en los mismos pines que el ADC1: no añade ninguno. */
static const struct { uint8_t pin; int8_t unidad; int8_t canal; } s_adc_mapa[] = {
    { 0x20, 0, 1 },  { 0x21, 0, 2 },  { 0x22, 0, 3 },  { 0x23, 0, 4 },    /* PC0..PC3  ADC1_IN1..4   */
    { 0x00, 0, 5 },  { 0x01, 0, 6 },  { 0x02, 0, 7 },  { 0x03, 0, 8 },    /* PA0..PA3  ADC1_IN5..8   */
    { 0x04, 0, 9 },  { 0x05, 0, 10 }, { 0x06, 0, 11 }, { 0x07, 0, 12 },   /* PA4..PA7  ADC1_IN9..12  */
    { 0x24, 0, 13 }, { 0x25, 0, 14 },                                     /* PC4, PC5  ADC1_IN13, 14 */
    { 0x10, 0, 15 }, { 0x11, 0, 16 }, { 0x12, 0, 17 },                    /* PB0..PB2  ADC1_IN15..17 */
    { 0x5E, 1, 5 },  { 0x5F, 1, 6 },  { 0x60, 1, 7 },  { 0x61, 1, 8 },    /* PF14, PF15, PG0, PG1: sólo ADC4 */
    { 0x3B, 1, 15 }, { 0x3C, 1, 16 }, { 0x3D, 1, 17 },                    /* PD11..PD13: sólo ADC4 */
};
static uint8_t s_caps_sis[STM32_PIN_COUNT];   /* #574: los pines del cable del IDE */

static void stm32_caps_init(void) {
    for (int p = 0; p < STM32_PIN_COUNT; p++) {
        s_caps_pin[p] = BPVM_PIN_DIGITAL | BPVM_PIN_OPEN_DRAIN;
        s_caps_adc_u[p] = -1; s_caps_adc_c[p] = -1;
        s_caps_sis[p] = 0;
    }
    for (int i = 0; i < PWM_SLICE_N; i++)
        if (s_pwm_map[i].pin >= 0 && s_pwm_map[i].pin < STM32_PIN_COUNT) s_caps_pin[s_pwm_map[i].pin] |= BPVM_PIN_PWM;
    for (unsigned i = 0; i < sizeof s_adc_mapa / sizeof s_adc_mapa[0]; i++) {
        s_caps_pin[s_adc_mapa[i].pin] |= BPVM_PIN_ANALOG;
        s_caps_adc_u[s_adc_mapa[i].pin] = s_adc_mapa[i].unidad;
        s_caps_adc_c[s_adc_mapa[i].pin] = s_adc_mapa[i].canal;
    }
    /* #574 (Eduardo, 26-sep: «los pines del wire también») — el cable del IDE es el USART1 del VCP en las dos placas,
     * PA9 (TX) y PA10 (RX): para el programa no están. La flash es interna: no ocupa pines. */
    s_caps_pin[0x09] = 0; s_caps_sis[0x09] = BPVM_PIN_SIS_WIRE;
    s_caps_pin[0x0A] = 0; s_caps_sis[0x0A] = BPVM_PIN_SIS_WIRE;
#if defined(STM32U5G9xx)
    s_caps.micro = "stm32u5g9"; s_caps.paquete = "LQFP144";    /* la DK2: STM32U5G9ZJTxQ (su proyecto de CubeMX) */
#else
    s_caps.micro = "stm32u575"; s_caps.paquete = "LQFP144";    /* la Nucleo */
#endif
    s_caps.fabricante = "STMicroelectronics";
    s_caps.cpu_hz_max = 160000000u; s_caps.cpu_hz_min = 160000000u; s_caps.cpu_ajustable = 0;
    s_caps.uart = s_caps_uart; s_caps.n_uart = (uint8_t) (sizeof s_caps_uart / sizeof s_caps_uart[0]);
    s_caps.i2c  = s_caps_i2c;  s_caps.n_i2c  = (uint8_t) (sizeof s_caps_i2c / sizeof s_caps_i2c[0]);
    s_caps.spi  = s_caps_spi;  s_caps.n_spi  = 3;
    s_caps.n_pwm = (uint8_t) PWM_SLICE_N; s_caps.pwm_unidad = "canal";
#if defined(STM32U5G9xx)
    s_caps.n_adc = 3;                                       /* ADC1, ADC2 y ADC4 en el U5G9 (su hoja de datos) */
#else
    s_caps.n_adc = 2;                                       /* ADC1 y ADC4 en el U575 */
#endif
    s_caps.pin = s_caps_pin; s_caps.n_pin = STM32_PIN_COUNT;
    s_caps.pin_adc_unit = s_caps_adc_u; s_caps.pin_adc_ch = s_caps_adc_c;
    s_caps.pin_sistema = s_caps_sis;                        /* #574 */
    s_caps.breadcrumb = 1;                                  /* el único que las tiene hoy (#487) */
    bpvm_caps_set(&s_caps);
}
#define PWM_ARR        999u   /* 1000 pasos → resolución de duty 0.1% */

static TIM_HandleTypeDef s_pwm_htim[PWM_SLICE_N];
static int               s_pwm_on[PWM_SLICE_N];

/* Reloj de los timers de APB1 (TIM3/TIM4): PCLK1, ×2 si el divisor APB1 ≠ 1. */
static uint32_t pwm_timer_clk(void) {
    RCC_ClkInitTypeDef clk; uint32_t lat;
    HAL_RCC_GetClockConfig(&clk, &lat);
    uint32_t p = HAL_RCC_GetPCLK1Freq();
    return (clk.APB1CLKDivider == RCC_HCLK_DIV1) ? p : p * 2u;
}

static uint32_t pwm_psc_for(int freqHz) {
    if (freqHz < 1) freqHz = 1;
    uint32_t div = (uint32_t) freqHz * (PWM_ARR + 1u);
    uint32_t psc = pwm_timer_clk() / (div ? div : 1u);
    return (psc > 0u) ? (psc - 1u) : 0u;
}

static int stm32_pwm_init_impl(int pin, int freqHz) {
    int idx = -1;
    for (int i = 0; i < PWM_SLICE_N; i++) if (s_pwm_map[i].pin == pin) { idx = i; break; }
    if (idx < 0) return -1;   /* pin sin canal de timer en la tabla */

    if      (s_pwm_map[idx].inst == TIM3)  __HAL_RCC_TIM3_CLK_ENABLE();
    else if (s_pwm_map[idx].inst == TIM16) __HAL_RCC_TIM16_CLK_ENABLE();  /* PWM de la DK2 */
    else                                   __HAL_RCC_TIM4_CLK_ENABLE();

    TIM_HandleTypeDef* h = &s_pwm_htim[idx];
    h->Instance           = s_pwm_map[idx].inst;
    h->Init.Prescaler     = pwm_psc_for(freqHz);
    h->Init.CounterMode   = TIM_COUNTERMODE_UP;
    h->Init.Period        = PWM_ARR;
    h->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    h->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(h) != HAL_OK) return -1;

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = 0;                       /* duty 0 inicial */
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(h, &oc, s_pwm_map[idx].channel) != HAL_OK) return -1;

    cfg_af_pin(pin, s_pwm_map[idx].af);      /* pin → AF del timer (pisa el GPIO de estado) */
    HAL_TIM_PWM_Start(h, s_pwm_map[idx].channel);
    s_pwm_on[idx] = 1;
    return idx;   /* sliceId */
}

static void stm32_pwm_set_duty_impl(int sliceId, int pin, int dutyPct) {
    (void) pin;
    if (sliceId < 0 || sliceId >= PWM_SLICE_N || !s_pwm_on[sliceId]) return;
    if (dutyPct < 0) dutyPct = 0;
    if (dutyPct > 100) dutyPct = 100;
    uint32_t ccr = (PWM_ARR + 1u) * (uint32_t) dutyPct / 100u;
    __HAL_TIM_SET_COMPARE(&s_pwm_htim[sliceId], s_pwm_map[sliceId].channel, ccr);
}

static void stm32_pwm_set_freq_impl(int sliceId, int freqHz) {
    if (sliceId < 0 || sliceId >= PWM_SLICE_N || !s_pwm_on[sliceId]) return;
    __HAL_TIM_SET_PRESCALER(&s_pwm_htim[sliceId], pwm_psc_for(freqHz));
}

static void stm32_pwm_start_impl(int sliceId) {
    if (sliceId >= 0 && sliceId < PWM_SLICE_N && s_pwm_on[sliceId])
        HAL_TIM_PWM_Start(&s_pwm_htim[sliceId], s_pwm_map[sliceId].channel);
}
static void stm32_pwm_stop_impl(int sliceId) {
    if (sliceId >= 0 && sliceId < PWM_SLICE_N && s_pwm_on[sliceId])
        HAL_TIM_PWM_Stop(&s_pwm_htim[sliceId], s_pwm_map[sliceId].channel);
}

/* V7 #667 — SOLTARLO: el canal parado, el timer desinicializado si no lo usa otro canal (en la Nucleo el TIM3 lo
 * comparten PC7 y PC8) y el pin a como estaba —el LED de la placa, a su firmware—. */
static void stm32_pwm_release_impl(int sliceId, int pin) {
    (void) pin;
    if (sliceId < 0 || sliceId >= PWM_SLICE_N || !s_pwm_on[sliceId]) return;
    HAL_TIM_PWM_Stop(&s_pwm_htim[sliceId], s_pwm_map[sliceId].channel);
    s_pwm_on[sliceId] = 0;
    int otro = 0;
    for (int i = 0; i < PWM_SLICE_N; i++) if (s_pwm_on[i] && s_pwm_map[i].inst == s_pwm_map[sliceId].inst) otro = 1;
    if (!otro) HAL_TIM_PWM_DeInit(&s_pwm_htim[sliceId]);
    stm32_pin_al_reset(s_pwm_map[sliceId].pin);
}

static int stm32_pwm_fin_de_run(void) {
    int n = 0;
    for (int i = 0; i < PWM_SLICE_N; i++) if (s_pwm_on[i]) { stm32_pwm_release_impl(i, s_pwm_map[i].pin); n++; }
    return n;
}

static const bpvm_pwm_backend_t s_pwm_backend = {
    .init    = stm32_pwm_init_impl,
    .setFreq = stm32_pwm_set_freq_impl,
    .setDuty = stm32_pwm_set_duty_impl,
    .start   = stm32_pwm_start_impl,
    .stop    = stm32_pwm_stop_impl,
    .release    = stm32_pwm_release_impl,   /* V7 #667 */
    .fin_de_run = stm32_pwm_fin_de_run,
};

/* ===================== Pulse counter — H10 ================================
 * Cuenta flancos HW en un pin sin coste de CPU: un timer en "external clock
 * mode 1" usa su entrada TI1 como reloj del contador, así que CNT avanza con
 * cada flanco del pin (no con el reloj del sistema). Tabla pin → (timer, AF).
 * Por placa (tabla abajo): Nucleo PC6=TIM8_CH1/TI1; DK2 PB7=TIM4_CH2/TI2. El
 * contador es de 16 bits (0..65535), como en el RP2350; para más cuentas, ventana
 * más corta o reset()+acumular en BP. Bonus sobre el RP2350: soporta BOTH (ambos
 * flancos) — el SDK del Pico no. counterId = índice en la tabla. El modo reloj-
 * externo toma TODO el timer (no por canal) → el contador NO debe compartir timer
 * con el PWM de la placa. */
typedef struct {
    int          pin;        /* pin BP = (puerto<<4)|bit */
    TIM_TypeDef* inst;       /* TIM8 (Nucleo) / TIM4 (DK2) */
    uint8_t      af;         /* GPIO_AFx_TIMy */
    uint32_t     clk_source; /* TIM_CLOCKSOURCE_TI1 (CH1) o _TI2 (CH2) */
} pulse_map_t;

static const pulse_map_t s_pulse_map[] = {
#if defined(BPVM_BOARD_DK2)
    { (1 << 4) | 7, TIM4, GPIO_AF2_TIM4, TIM_CLOCKSOURCE_TI2 },  /* PB7 — CN1 pin 10, TIM4_CH2 (TI2) */
#else
    { (2 << 4) | 6, TIM8, GPIO_AF3_TIM8, TIM_CLOCKSOURCE_TI1 },  /* PC6 — TIM8_CH1 (TI1) */
#endif
};
#define PULSE_N    ((int)(sizeof(s_pulse_map) / sizeof(s_pulse_map[0])))

static TIM_HandleTypeDef s_pulse_htim[PULSE_N];
static int               s_pulse_on[PULSE_N];

/* edgeKind BP (0=RISING,1=FALLING,2=BOTH) → polaridad del reloj TIx de la HAL. */
static uint32_t pulse_edge_to_polarity(int edgeKind) {
    switch (edgeKind) {
        case 1:  return TIM_CLOCKPOLARITY_FALLING;
        case 2:  return TIM_CLOCKPOLARITY_BOTHEDGE;
        default: return TIM_CLOCKPOLARITY_RISING;
    }
}

static int stm32_pulse_init_impl(int pin, int edgeKind) {
    int idx = -1;
    for (int i = 0; i < PULSE_N; i++) if (s_pulse_map[i].pin == pin) { idx = i; break; }
    if (idx < 0) return -1;   /* pin sin entrada de timer en la tabla */

    if      (s_pulse_map[idx].inst == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();
    else if (s_pulse_map[idx].inst == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();   /* contador de la DK2 */

    TIM_HandleTypeDef* h = &s_pulse_htim[idx];
    h->Instance               = s_pulse_map[idx].inst;
    h->Init.Prescaler         = 0;
    h->Init.CounterMode       = TIM_COUNTERMODE_UP;
    h->Init.Period            = 0xFFFFu;     /* 16-bit: cuenta 0..65535 */
    h->Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    h->Init.RepetitionCounter = 0;           /* timer avanzado (TIM8) */
    h->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(h) != HAL_OK) return -1;

    /* External clock mode 1: el contador se alimenta de los flancos de la entrada
     * TIx del pin (TI1=CH1 o TI2=CH2, según la tabla). */
    TIM_ClockConfigTypeDef clk = {0};
    clk.ClockSource    = s_pulse_map[idx].clk_source;
    clk.ClockPolarity  = pulse_edge_to_polarity(edgeKind);
    clk.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    clk.ClockFilter    = 0;
    if (HAL_TIM_ConfigClockSource(h, &clk) != HAL_OK) return -1;

    cfg_af_pin(pin, s_pulse_map[idx].af);   /* pin → AF del timer (entrada TI1) */
    __HAL_TIM_SET_COUNTER(h, 0);
    s_pulse_on[idx] = 1;
    return idx;   /* counterId */
}

static void stm32_pulse_start_impl(int counterId) {
    if (counterId >= 0 && counterId < PULSE_N && s_pulse_on[counterId])
        HAL_TIM_Base_Start(&s_pulse_htim[counterId]);   /* CEN=1 → cuenta flancos */
}
static void stm32_pulse_stop_impl(int counterId) {
    if (counterId >= 0 && counterId < PULSE_N && s_pulse_on[counterId])
        HAL_TIM_Base_Stop(&s_pulse_htim[counterId]);    /* CEN=0; CNT se conserva */
}
static int stm32_pulse_value_impl(int counterId) {
    if (counterId < 0 || counterId >= PULSE_N || !s_pulse_on[counterId]) return 0;
    return (int) __HAL_TIM_GET_COUNTER(&s_pulse_htim[counterId]);
}
static void stm32_pulse_reset_impl(int counterId) {
    if (counterId >= 0 && counterId < PULSE_N && s_pulse_on[counterId])
        __HAL_TIM_SET_COUNTER(&s_pulse_htim[counterId], 0);
}

/* V7 #667 — SOLTARLO: el timer (entero: el reloj externo lo toma todo) parado y desinicializado, y el pin a como
 * estaba. */
static void stm32_pulse_release_impl(int counterId, int pin) {
    (void) pin;
    if (counterId < 0 || counterId >= PULSE_N || !s_pulse_on[counterId]) return;
    HAL_TIM_Base_Stop(&s_pulse_htim[counterId]);
    HAL_TIM_Base_DeInit(&s_pulse_htim[counterId]);
    s_pulse_on[counterId] = 0;
    stm32_pin_al_reset(s_pulse_map[counterId].pin);
}

static int stm32_pulse_fin_de_run(void) {
    int n = 0;
    for (int i = 0; i < PULSE_N; i++) if (s_pulse_on[i]) { stm32_pulse_release_impl(i, s_pulse_map[i].pin); n++; }
    return n;
}

static const bpvm_pulse_backend_t s_pulse_backend = {
    .init  = stm32_pulse_init_impl,
    .start = stm32_pulse_start_impl,
    .stop  = stm32_pulse_stop_impl,
    .value = stm32_pulse_value_impl,
    .reset = stm32_pulse_reset_impl,
    .release    = stm32_pulse_release_impl,   /* V7 #667 */
    .fin_de_run = stm32_pulse_fin_de_run,
};

void stm32_hw_register(void) {
    bpvm_gpio_set_backend(&s_gpio_backend);
    stm32_caps_init();                         /* V7/H4: la tabla de ESTE die */
    bpvm_machine_set_backend(&s_pico_backend);
    bpvm_spi_set_backend(&s_spi_backend);     /* H15.1 */
    bpvm_uart_set_backend(&s_uart_backend);   /* H15.1 */
    bpvm_i2c_set_backend(&s_i2c_backend);     /* H15.2 */
    /* #480 - SIN backend de watchdog A PROPOSITO: el IWDG no se puede parar y
     * medio watchdog es peor que ninguno. El porque, entero, donde estaba el
     * codigo (busca "WDT - NO LO HAY EN ESTA FAMILIA" en este mismo fichero). */
    stm32_breadcrumb_init();                  /* H10 — breadcrumb RAM retenida (TAMP) */
#if defined(BOARD_HAS_RTC)
    bpvm_rtc_set_backend(&s_rtc_backend);     /* H10 — RTC HW (hora sobrevive al reset) */
#endif
    bpvm_pwm_set_backend(&s_pwm_backend);     /* H10 — PWM por timer (LEDs verde/azul) */
    bpvm_pulse_set_backend(&s_pulse_backend); /* H10 — contador de pulsos (TIM8/TI1 en PC6) */
#if defined(BOARD_HAS_ADC_TEMP)
    bpvm_adc_set_backend(&s_stm32_adc_backend); /* V7/H4 (el resto de #469) — el ADC1 en los pines de la tabla */
#endif
}
