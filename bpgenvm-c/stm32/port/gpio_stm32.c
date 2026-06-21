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
        strncpy(buf, "nucleo-u575zi", len - 1);
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
    .resetCause    = stm32_reset_cause,        /* H10 — causa del último reset */
    .setMark       = stm32_set_mark_impl,      /* H10 — breadcrumb */
    .markCount     = stm32_mark_count_impl,
    .markAt        = stm32_mark_at_impl,
    .bootCount     = stm32_boot_count_impl,
};

/* ===================== SPI (H15.1) =====================================
 * Backend SPI sobre la HAL. MODELO ACTUAL (MVP): CubeMX ya inicializo
 * SPI1/SPI2 al boot (hspi1/hspi2 globales); aqui (re)ajustamos formato +
 * baud y ASIGNAMOS los pines que el usuario pasa en el .bp (dinamico: el
 * programa elige los pines). bus 0 -> SPI1, bus 1 -> SPI2 (ambos AF5). El
 * CS lo gestiona el usuario por GPIO, igual que en la Pico.
 *
 * Reloj de SPI = SYSCLK (160 MHz, segun el MspInit de CubeMX) -> el
 * prescaler se elige del baud pedido.
 *
 * [v3 "lo correcto"] que crear el objeto ACTIVE el periferico + clock y
 * asigne pines, sin depender del init de CubeMX en el boot.
 */
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;

/* bus N -> SPIn  (numero de bus = numero de instancia HW, igual que la Pico). */
static SPI_HandleTypeDef* spi_handle(int bus) {
    switch (bus) {
        case 1: return &hspi1;   /* SPI1 */
        case 2: return &hspi2;   /* SPI2 */
        case 3: return &hspi3;   /* SPI3 */
        default: return NULL;
    }
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
    /* (re)ajusta formato + baud sobre el handle que CubeMX dejo inicializado */
    h->Init.DataSize          = SPI_DATASIZE_8BIT;
    h->Init.CLKPolarity       = (mode & 0x2) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    h->Init.CLKPhase          = (mode & 0x1) ? SPI_PHASE_2EDGE   : SPI_PHASE_1EDGE;
    h->Init.NSS               = SPI_NSS_SOFT;
    h->Init.BaudRatePrescaler = spi_prescaler_for(baudrate);
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
 * Igual que SPI: CubeMX inicializo las instancias al boot (huart4/huart3
 * globales); aqui (re)ajustamos baud/formato y asignamos los pines del .bp.
 * bus 0 -> UART4 (AF8; pines tipicos PA0/PA1 = A0/A1 del header Arduino),
 * bus 1 -> USART3 (AF7).  El VCP del ST-LINK usa USART1 -> NO se toca.
 *
 * read(timeout): HAL_UART_Receive devuelve TIMEOUT si no completa n bytes a
 * tiempo; los recibidos = n - RxXferCount. available(): no soportado en el MVP
 * (-1, segun el contrato del header) -> usar read con timeout.
 */
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
#if defined(BPVM_BOARD_DK2)
extern UART_HandleTypeDef huart6;   /* DK2: USART6 (el U575 no lo tiene) */
#else
extern UART_HandleTypeDef huart5;   /* Nucleo: UART5 (la DK2 no lo tiene) */
#endif

/* bus N -> USARTn/UARTn (numero de bus = instancia HW). USART1 = VCP -> no se expone. */
static UART_HandleTypeDef* uart_handle(int bus) {
    switch (bus) {
        case 2: return &huart2;   /* USART2 (AF7) */
        case 3: return &huart3;   /* USART3 (AF7) */
        case 4: return &huart4;   /* UART4  (AF8) */
#if defined(BPVM_BOARD_DK2)
        case 6: return &huart6;   /* DK2: USART6 */
#else
        case 5: return &huart5;   /* Nucleo: UART5 (AF8) */
#endif
        default: return NULL;
    }
}

static void stm32_uart_init_impl(int bus, int tx, int rx, int baudrate,
                                 int data_bits, int stop_bits, int parity) {
    UART_HandleTypeDef* h = uart_handle(bus);
    if (!h) return;
    h->Init.BaudRate = (uint32_t) baudrate;
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
    h->Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(h);
    /* FIFO RX (8 bytes): sin el, el RX en polling pierde por overrun los bytes
     * que llegan en rafaga (un loopback de 4 bytes solo capturaba el primero).
     * CubeMX lo deja deshabilitado por defecto; lo habilitamos aqui. */
    HAL_UARTEx_SetTxFifoThreshold(h, UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(h, UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_EnableFifoMode(h);
    uint8_t af = (bus == 2 || bus == 3) ? GPIO_AF7_USART3 : GPIO_AF8_UART4;
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
 * Backend I2C sobre la HAL. Igual que SPI/UART: CubeMX inicializo I2C1..4
 * al boot (hi2c1..hi2c4; 100 kHz, 7-bit) y aqui reasignamos los pines que
 * pide el .bp. bus N -> I2Cn. En el U5 TODAS las instancias I2C usan AF4.
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
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
#if !defined(BPVM_BOARD_DK2)
extern I2C_HandleTypeDef hi2c3;   /* solo Nucleo (la DK2 expone I2C1/I2C2) */
extern I2C_HandleTypeDef hi2c4;
#endif

/* bus N -> I2Cn (numero de bus = instancia HW, igual que SPI/UART/Pico). */
static I2C_HandleTypeDef* i2c_handle(int bus) {
    switch (bus) {
        case 1: return &hi2c1;
        case 2: return &hi2c2;
#if !defined(BPVM_BOARD_DK2)
        case 3: return &hi2c3;
        case 4: return &hi2c4;
#endif
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
    (void) baudrate;   /* MVP: Timing de CubeMX (100 kHz) -- ver cabecera */
    /* El handle ya esta inicializado (HAL_I2C_Init en el boot); solo
     * (re)asignamos los pines del usuario. AF4 para toda instancia I2C. */
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

void stm32_hw_register(void) {
    bpvm_gpio_set_backend(&s_gpio_backend);
    bpvm_pico_set_backend(&s_pico_backend);
    bpvm_spi_set_backend(&s_spi_backend);     /* H15.1 */
    bpvm_uart_set_backend(&s_uart_backend);   /* H15.1 */
    bpvm_i2c_set_backend(&s_i2c_backend);     /* H15.2 */
    bpvm_wdt_set_backend(&s_wdt_backend);     /* H10 — watchdog IWDG */
    stm32_breadcrumb_init();                  /* H10 — breadcrumb RAM retenida (TAMP) */
    /* PWM/Rtc: anadir su backend HAL cuando toque (ADC temp ya en stm32_temp_c_impl). */
}
