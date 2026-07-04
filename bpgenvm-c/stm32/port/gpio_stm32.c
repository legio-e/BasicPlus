/*
 * gpio_stm32.c — backends de HW del STM32U575 (H9.4).
 *
 * Implementa los hooks portables de la VM sobre la HAL de ST:
 *   - GPIO  (bpvm_gpio_backend_t):  init/pull/write/read sobre HAL_GPIO_*.
 *   - Pico  (bpvm_pico_backend_t):  info de placa (gpioCount, freq, uptime,
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
#include "bpvm_pico.h"
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
    cfg.Mode  = (s_mode[pin] == 1) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    cfg.Pull  = (s_pull[pin] == 1) ? GPIO_PULLUP
              : (s_pull[pin] == 2) ? GPIO_PULLDOWN
              : GPIO_NOPULL;
    HAL_GPIO_Init(port, &cfg);
}

/* ---- GPIO backend ---- */

static void stm32_gpio_init_impl(int pin, int mode) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return;
    s_mode[pin] = (mode == 1) ? 1 : 0;
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

static const bpvm_gpio_backend_t s_gpio_backend = {
    .init  = stm32_gpio_init_impl,
    .pull  = stm32_gpio_pull_impl,
    .write = stm32_gpio_write_impl,
    .read  = stm32_gpio_read_impl,
};

/* ---- Pico (info de MCU) backend ---- */

static int stm32_gpio_count_impl(void) {
    return STM32_PIN_COUNT;   /* 128: puertos A..H del U575 */
}

/* board-aware: canales ADC y slices PWM del STM32U575. Sin estos cb,
 * Pico.ADC_CHANNELS()/PWM_SLICES() caían al fallback host (4/12) de src/pico.c
 * — el INFO ya daba 20/28 desde un string aparte (stm32_repl.c), de ahí el
 * mismatch. Valores casan con el INFO. */
static int stm32_adc_channels_impl(void) {
    return 20;   /* ADC1 14-bit, hasta 20 canales multiplexados */
}

static int stm32_pwm_slices_impl(void) {
    return 28;   /* canales PWM/timer expuestos (coincide con el INFO) */
}

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

/* Single-shot de un canal interno (TEMPSENSOR / VREFINT). Devuelve raw o -1. */
static int stm32_adc_read_internal(uint32_t channel) {
    ADC_ChannelConfTypeDef c = {0};
    c.Channel      = channel;
    c.Rank         = ADC_REGULAR_RANK_1;
    c.SamplingTime = ADC_SAMPLETIME_814CYCLES;  /* canales internos → muestreo largo */
    c.SingleDiff   = ADC_SINGLE_ENDED;
    c.OffsetNumber = ADC_OFFSET_NONE;
    c.Offset       = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &c) != HAL_OK) return -1;
    HAL_Delay(1);                               /* arranque del sensor/buffer (TSEN/VREFEN) */
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
    int vref = stm32_adc_read_internal(ADC_CHANNEL_VREFINT);
    if (vref > 0)
        vdda_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(ADC1, (uint32_t) vref, LL_ADC_RESOLUTION_14B);
    int ts = stm32_adc_read_internal(ADC_CHANNEL_TEMPSENSOR);
    if (ts < 0) return 0.0f;
    int32_t t = __LL_ADC_CALC_TEMPERATURE(ADC1, vdda_mv, (uint32_t) ts, LL_ADC_RESOLUTION_14B);
    return (float) t;
}
#else
static float stm32_temp_c_impl(void) {
    return 0.0f;   /* placa sin BOARD_HAS_ADC_TEMP: sensor no disponible */
}
#endif

static int stm32_set_cpu_freq_mhz_impl(int mhz) {
    (void) mhz;
    return 0;   /* reloj fijo (160 MHz); cambiar en runtime no soportado */
}

/* ===================== Breadcrumb en RAM retenida — H10 ====================
 * Migas que sobreviven al reset, en los registros de backup del TAMP (no se
 * borran en un reset del sistema; con VBAT, tampoco en power-off). Layout:
 *   BKP0R=magic  BKP1R=boot_count  BKP2R=total(setMark de esta vida)
 *   BKP3R = 1ª marca PEGAJOSA (la causa original; no se sobrescribe nunca)
 *   BKP4R..BKP19R = anillo de las últimas 16 marcas
 * En el boot tomamos un SNAPSHOT del trail de la vida anterior en orden
 * cronológico (trail[0] = origen garantizado), reseteamos `total` para la vida
 * nueva e incrementamos boot_count. markCount/markAt leen el snapshot (estable,
 * inmune a las marcas nuevas). Board-agnóstico (todos los U5 tienen TAMP). */
#define BC_MAGIC      0xB9CBA001u
#define BC_RING_N     16u
#define BC_BKP_MAGIC  0u
#define BC_BKP_BOOT   1u
#define BC_BKP_TOTAL  2u
#define BC_BKP_FIRST  3u
#define BC_BKP_RING   4u    /* BKP4R..BKP19R */

static int      s_bc_trail[BC_RING_N + 1];   /* +1: la pegajosa puede prependerse */
static int      s_bc_trail_count = 0;
static uint32_t s_bc_boot_count  = 1;

/* BKP0R..BKP31R son 32 registros contiguos de 32 bits (offset 0x100..0x17C). */
static volatile uint32_t* bc_bkp(unsigned i) { return &(&TAMP->BKP0R)[i]; }

static void stm32_breadcrumb_init(void) {
    __HAL_RCC_RTCAPB_CLK_ENABLE();   /* acceso al TAMP (dominio RTC/backup) */
    HAL_PWR_EnableBkUpAccess();      /* DBP: permite escribir el dominio backup */

    if (*bc_bkp(BC_BKP_MAGIC) == BC_MAGIC) {
        /* Reset caliente: reconstruimos el trail de la vida anterior. */
        s_bc_boot_count   = *bc_bkp(BC_BKP_BOOT) + 1u;
        uint32_t total    = *bc_bkp(BC_BKP_TOTAL);
        uint32_t first    = *bc_bkp(BC_BKP_FIRST);
        s_bc_trail_count  = 0;
        if (total > 0u && total <= BC_RING_N) {
            for (uint32_t j = 0; j < total; j++)         /* anillo = orden de escritura */
                s_bc_trail[s_bc_trail_count++] = (int) *bc_bkp(BC_BKP_RING + j);
        } else if (total > BC_RING_N) {
            s_bc_trail[s_bc_trail_count++] = (int) first;   /* [0] = origen pegajoso */
            for (uint32_t j = 0; j < BC_RING_N; j++)        /* últimas 16, antigua→reciente */
                s_bc_trail[s_bc_trail_count++] =
                    (int) *bc_bkp(BC_BKP_RING + ((total + j) % BC_RING_N));
        }
    } else {
        /* Arranque en frío: inicializa el dominio. */
        *bc_bkp(BC_BKP_MAGIC) = BC_MAGIC;
        s_bc_boot_count  = 1u;
        s_bc_trail_count = 0;
    }
    *bc_bkp(BC_BKP_BOOT)  = s_bc_boot_count;
    *bc_bkp(BC_BKP_TOTAL) = 0u;       /* vida nueva: cuenta marcas desde cero */
}

static void stm32_set_mark_impl(int code) {
    uint32_t total = *bc_bkp(BC_BKP_TOTAL);
    if (total == 0u) *bc_bkp(BC_BKP_FIRST) = (uint32_t) code;   /* 1ª pegajosa */
    *bc_bkp(BC_BKP_RING + (total % BC_RING_N)) = (uint32_t) code;
    *bc_bkp(BC_BKP_TOTAL) = total + 1u;
}

static int stm32_mark_count_impl(void) { return s_bc_trail_count; }

static int stm32_mark_at_impl(int i) {
    if (i < 0 || i >= s_bc_trail_count) return 0;
    return s_bc_trail[i];
}

static int stm32_boot_count_impl(void) { return (int) s_bc_boot_count; }

static const bpvm_pico_backend_t s_pico_backend = {
    .uniqueId      = stm32_unique_id_impl,
    .boardName     = stm32_board_name_impl,
    .tempC         = stm32_temp_c_impl,
    .cpuFreqHz     = stm32_cpu_freq_hz_impl,
    .uptimeMs      = stm32_uptime_ms_impl,
    .setCpuFreqMHz = stm32_set_cpu_freq_mhz_impl,
    .gpioCount     = stm32_gpio_count_impl,
    .adcChannels   = stm32_adc_channels_impl,  /* board-aware: 20 (era fallback 4) */
    .pwmSlices     = stm32_pwm_slices_impl,     /* board-aware: 28 (era fallback 12) */
    .resetCause    = stm32_reset_cause,        /* H10 — causa del último reset */
    .setMark       = stm32_set_mark_impl,      /* H10 — breadcrumb */
    .markCount     = stm32_mark_count_impl,
    .markAt        = stm32_mark_at_impl,
    .bootCount     = stm32_boot_count_impl,
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

static int stm32_spi_write_impl(int bus, const uint8_t* data, size_t n) {
    SPI_HandleTypeDef* h = spi_handle(bus);
    if (!h) return -1;
    return (HAL_SPI_Transmit(h, (uint8_t*) data, (uint16_t) n, 1000) == HAL_OK)
           ? (int) n : -1;
}

static int stm32_spi_read_impl(int bus, uint8_t* data, size_t n) {
    SPI_HandleTypeDef* h = spi_handle(bus);
    if (!h) return -1;
    return (HAL_SPI_Receive(h, data, (uint16_t) n, 1000) == HAL_OK)
           ? (int) n : -1;
}

static int stm32_spi_transfer_impl(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    SPI_HandleTypeDef* h = spi_handle(bus);
    if (!h) return -1;
    return (HAL_SPI_TransmitReceive(h, (uint8_t*) tx, rx, (uint16_t) n, 1000) == HAL_OK)
           ? (int) n : -1;
}

static const bpvm_spi_backend_t s_spi_backend = {
    .init     = stm32_spi_init_impl,
    .write    = stm32_spi_write_impl,
    .read     = stm32_spi_read_impl,
    .transfer = stm32_spi_transfer_impl,
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

static void stm32_uart_init_impl(int bus, int tx, int rx, int baudrate,
                                 int data_bits, int stop_bits, int parity) {
    UART_HandleTypeDef* h = uart_handle(bus);
    if (!h) return;
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

static const bpvm_uart_backend_t s_uart_backend = {
    .init      = stm32_uart_init_impl,
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
 * desde BP (chocaria con el tactil). */
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

static const bpvm_i2c_backend_t s_i2c_backend = {
    .init  = stm32_i2c_init_impl,
    .write = stm32_i2c_write_impl,
    .read  = stm32_i2c_read_impl,
};

/* ===================== WDT (IWDG) — H10 ====================================
 * Watchdog independiente por REGISTROS (CMSIS, sin HAL ni CubeMX): así es
 * autocontenido — no necesita IWDG en el .ioc (que metería un MX_IWDG_Init
 * arrancándolo en el BOOT, a ~512 ms → bucle de reset) ni HAL_IWDG_MODULE_ENABLED;
 * funciona igual en Nucleo y DK2 y sobrevive a regeneraciones. Contador de 12 bits
 * sobre el LSI (~32 kHz), prescaler /4../1024; al expirar resetea el chip.
 *
 * timeout(ms) → (PR, RLR):  cuentas = ms·LSI/1000/div ; RLR = cuentas−1 ≤ 4095.
 * Elegimos el divisor más pequeño que entre (mejor resolución).
 * Rango ≈ 0.1 ms … 131 s.  PR: 0=/4,1=/8,…,6=/256,7=/512,8=/1024.
 *
 * LIMITACIÓN HW vs RP2350: el IWDG NO se puede PARAR una vez arrancado → disable()
 * es el mejor esfuerzo (reprograma al timeout máximo ~131 s y refresca); sigue
 * vivo. Para un sleep muy largo, no lo actives hasta necesitarlo. */
#define STM32_LSI_HZ        32000u    /* LSI nominal; el IWDG es impreciso de por sí */
#define STM32_IWDG_RLR_MAX  4095u
#define IWDG_KR_WRITE       0x5555u   /* desbloquea PR/RLR */
#define IWDG_KR_REFRESH     0xAAAAu   /* recarga el contador */
#define IWDG_KR_START       0xCCCCu   /* arranca el IWDG (y el LSI) */

static int s_wdt_on = 0;

/* timeout(ms) → código de prescaler PR (0..8) y reload RLR (0..4095). */
static void stm32_iwdg_calc(uint32_t ms, uint32_t* pr, uint32_t* reload) {
    uint32_t div = 4u;
    for (uint32_t code = 0u; code <= 8u; code++, div <<= 1) {
        uint32_t counts = (ms * (STM32_LSI_HZ / 1000u)) / div;   /* ms·32/div */
        if (counts == 0u) counts = 1u;
        if (counts <= (STM32_IWDG_RLR_MAX + 1u)) {
            *pr = code; *reload = counts - 1u; return;
        }
    }
    *pr = 8u; *reload = STM32_IWDG_RLR_MAX;     /* timeout enorme → tope (/1024) */
}

/* Reprograma PR/RLR (con el IWDG ya arrancado) y recarga el contador. */
static void stm32_iwdg_program(uint32_t ms) {
    uint32_t pr, reload;
    stm32_iwdg_calc(ms, &pr, &reload);
    IWDG->KR  = IWDG_KR_WRITE;                  /* desbloquear PR/RLR */
    IWDG->PR  = pr;
    IWDG->RLR = reload;
    uint32_t guard = 200000u;                   /* anti-cuelgue si el LSI fallara */
    while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) && --guard) { }
    IWDG->KR  = IWDG_KR_REFRESH;                /* cargar el contador */
}

static void stm32_wdt_enable_impl(int timeoutMs) {
    if (timeoutMs < 1) timeoutMs = 1;
    IWDG->KR = IWDG_KR_START;                   /* arrancar (enciende el LSI) ANTES de programar */
    stm32_iwdg_program((uint32_t) timeoutMs);
    s_wdt_on = 1;
}

static void stm32_wdt_feed_impl(void) {
    if (s_wdt_on) IWDG->KR = IWDG_KR_REFRESH;
}

static void stm32_wdt_disable_impl(void) {
    /* No se puede parar el IWDG: mejor esfuerzo = timeout máximo + refresco. */
    if (s_wdt_on) stm32_iwdg_program(131000u);
}

static const bpvm_wdt_backend_t s_wdt_backend = {
    .enable  = stm32_wdt_enable_impl,
    .feed    = stm32_wdt_feed_impl,
    .disable = stm32_wdt_disable_impl,
};

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

static const bpvm_pwm_backend_t s_pwm_backend = {
    .init    = stm32_pwm_init_impl,
    .setFreq = stm32_pwm_set_freq_impl,
    .setDuty = stm32_pwm_set_duty_impl,
    .start   = stm32_pwm_start_impl,
    .stop    = stm32_pwm_stop_impl,
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

static const bpvm_pulse_backend_t s_pulse_backend = {
    .init  = stm32_pulse_init_impl,
    .start = stm32_pulse_start_impl,
    .stop  = stm32_pulse_stop_impl,
    .value = stm32_pulse_value_impl,
    .reset = stm32_pulse_reset_impl,
};

void stm32_hw_register(void) {
    bpvm_gpio_set_backend(&s_gpio_backend);
    bpvm_pico_set_backend(&s_pico_backend);
    bpvm_spi_set_backend(&s_spi_backend);     /* H15.1 */
    bpvm_uart_set_backend(&s_uart_backend);   /* H15.1 */
    bpvm_i2c_set_backend(&s_i2c_backend);     /* H15.2 */
    bpvm_wdt_set_backend(&s_wdt_backend);     /* H10 — watchdog IWDG */
    stm32_breadcrumb_init();                  /* H10 — breadcrumb RAM retenida (TAMP) */
#if defined(BOARD_HAS_RTC)
    bpvm_rtc_set_backend(&s_rtc_backend);     /* H10 — RTC HW (hora sobrevive al reset) */
#endif
    bpvm_pwm_set_backend(&s_pwm_backend);     /* H10 — PWM por timer (LEDs verde/azul) */
    bpvm_pulse_set_backend(&s_pulse_backend); /* H10 — contador de pulsos (TIM8/TI1 en PC6) */
}
