/*
 * stm32_wire.c — EL CABLE del wire v1 en el STM32: USART del VCP, bare-metal.
 *
 * [V6/U2, 25-ago-2026] Este fichero implementa las CUATRO funciones de cable
 * del contrato (`bpvm_wire_v1.h`): recv_line, recv_bulk, send_line, send_bulk.
 * Los builders JSON y send_cstr/send_error/send_reply_empty/send_fatal viven en
 * `src/wire_v1_proto.c`, comunes a las cuatro familias. Aquí quedan además los
 * extras propios del USART (getchar, ring de RX, write) y el wrapper de FATAL
 * que enciende el LED de error de la placa.
 *
 * E/S por el hcom_uart[COM1] del BSP (ya inicializado por BSP_COM_Init en
 * main.c). RX por polling directo del registro (sin ISR ni NVIC → cero
 * cambios en el .ioc); TX por HAL_UART_Transmit.
 *
 * Nota de robustez: RX por lectura DIRECTA del registro (flag RXNE + RDR),
 * NO con HAL_UART_Receive() — su overhead (lock + máquina de estados + tick
 * por byte) a 4 MHz no seguía 115200 baud → overrun, se perdía el '
' y el
 * REPL se colgaba en recv_line. Limpiamos ORE por si un burst nos adelanta.
 */
#include "stm32_wire.h"

#include "main.h"
#include "board.h"   /* placa: BOARD_WIRE_UART, BOARD_LED_ERR_ON */
#include "log.h"     /* #495: si el DMA de la RX no arranca o se desborda, se dice */
#include "FreeRTOS.h"      /* V6/#473: mutex de TX — la linea es atomica */
#include "semphr.h"

#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef* wire_uart(void) { return BOARD_WIRE_UART; }

#if defined(BOARD_WIRE_IRQn)
/* ===== RX a un anillo de 4 KB — STM32 con BOARD_WIRE_IRQn (DK2, Nucleo… V3/H5.2, H10, #495) =====
 * La placa opta al anillo definiendo BOARD_WIRE_IRQn en board.h (antes era DK2-only).
 * La lectura directa del registro (#else) no tiene buffer software: si el lazo
 * no sondea durante >~700µs (la FIFO HW son 8 bytes) se pierden bytes por
 * overrun. Eso mataba el KILL durante Gui.run() — el bombeo de LVGL deja el UART
 * sin atender varios ms.
 *
 * #495 (V7/H8, 1-oct) — EL ANILLO LO LLENA EL DMA. Hasta aquí lo llenaba la IRQ de
 * RX del USART1, byte a byte, en un anillo de 256 B que tiraba en silencio lo que no
 * cabía. Con la ventana del wire (4 trozos de 512 B sin acuse) el PC manda 2 KB
 * seguidos justo mientras la placa escribe la flash: 256 B no los aguantan, y si la
 * CPU se para durante un borrado, la IRQ tampoco llega a tiempo a la FIFO de 8 B.
 * Decisión de Eduardo (1-oct): DMA circular de 4 KB. El GPDMA copia al anillo cada
 * byte que llega, sin interrupciones y aunque la CPU esté parada; hasta dónde ha
 * escrito lo dice lo que le falta del bloque (BNDT). Si el DMA no arrancara, el
 * anillo lo llena la IRQ como antes, y el log lo dice.
 * SPSC: un productor (el DMA, o la ISR) y un consumidor (getchar); índices uint16
 * alineados → acceso atómico en M33, volatile basta. */
#define RX_RING_SZ 4096u
static volatile uint8_t  s_rx_ring[RX_RING_SZ];
static volatile uint16_t s_rx_head = 0;   /* productor sin DMA: la ISR */
static volatile uint16_t s_rx_tail = 0;   /* consumidor: getchar */

#if defined(BOARD_WIRE_DMA_REQ)
static DMA_HandleTypeDef s_rx_dma;
static DMA_NodeTypeDef   s_rx_nodo;       /* un solo nodo, enlazado consigo mismo: el bucle */
static DMA_QListTypeDef  s_rx_cola;
static int               s_rx_por_dma = 0;

/* El canal en lista enlazada CIRCULAR: un nodo que copia del RDR del USART al
 * anillo, RX_RING_SZ bytes, y al acabar se recarga a sí mismo. Sin interrupciones
 * del DMA: el consumidor mira el contador. Devuelve 0, o el paso que falló. */
static int rx_dma_arrancar(void) {
    UART_HandleTypeDef* h = wire_uart();
    DMA_NodeConfTypeDef nc;
    memset(&nc, 0, sizeof nc);
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    nc.NodeType                         = DMA_GPDMA_LINEAR_NODE;
    nc.Init.Request                     = BOARD_WIRE_DMA_REQ;
    nc.Init.BlkHWRequest                = DMA_BREQ_SINGLE_BURST;
    nc.Init.Direction                   = DMA_PERIPH_TO_MEMORY;
    nc.Init.SrcInc                      = DMA_SINC_FIXED;
    nc.Init.DestInc                     = DMA_DINC_INCREMENTED;
    nc.Init.SrcDataWidth                = DMA_SRC_DATAWIDTH_BYTE;
    nc.Init.DestDataWidth               = DMA_DEST_DATAWIDTH_BYTE;
    nc.Init.Priority                    = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    nc.Init.SrcBurstLength              = 1;
    nc.Init.DestBurstLength             = 1;
    nc.Init.TransferAllocatedPort       = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
    nc.Init.TransferEventMode           = DMA_TCEM_BLOCK_TRANSFER;
    nc.Init.Mode                        = DMA_NORMAL;
    nc.TriggerConfig.TriggerPolarity    = DMA_TRIG_POLARITY_MASKED;
    nc.DataHandlingConfig.DataExchange  = DMA_EXCHANGE_NONE;
    nc.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    nc.SrcAddress                       = (uint32_t) &h->Instance->RDR;
    nc.DstAddress                       = (uint32_t) s_rx_ring;
    nc.DataSize                         = RX_RING_SZ;
    if (HAL_DMAEx_List_BuildNode(&nc, &s_rx_nodo) != HAL_OK)               return 1;
    if (HAL_DMAEx_List_InsertNode_Tail(&s_rx_cola, &s_rx_nodo) != HAL_OK)  return 2;
    if (HAL_DMAEx_List_SetCircularMode(&s_rx_cola) != HAL_OK)              return 3;
    s_rx_dma.Instance                         = BOARD_WIRE_DMA_CH;
    s_rx_dma.InitLinkedList.Priority          = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    s_rx_dma.InitLinkedList.LinkStepMode      = DMA_LSM_FULL_EXECUTION;
    s_rx_dma.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    s_rx_dma.InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;
    s_rx_dma.InitLinkedList.LinkedListMode    = DMA_LINKEDLIST_CIRCULAR;
    if (HAL_DMAEx_List_Init(&s_rx_dma) != HAL_OK)                          return 4;
    if (HAL_DMAEx_List_LinkQ(&s_rx_dma, &s_rx_cola) != HAL_OK)             return 5;
    if (HAL_DMAEx_List_Start(&s_rx_dma) != HAL_OK)                         return 6;
    SET_BIT(h->Instance->CR3, USART_CR3_DMAR);   /* el USART pide el DMA por cada byte */
    return 0;
}
#endif

/* Hasta dónde ha escrito el productor. Con DMA lo dice lo que le falta del bloque
 * (BNDT): al acabar vale 0 un instante y se recarga a RX_RING_SZ — de ahí el %. */
static uint16_t rx_head(void) {
#if defined(BOARD_WIRE_DMA_REQ)
    if (s_rx_por_dma)
        return (uint16_t) ((RX_RING_SZ - (uint32_t) __HAL_DMA_GET_COUNTER(&s_rx_dma)) % RX_RING_SZ);
#endif
    return s_rx_head;
}

void stm32_wire_rx_drain(void) {
    UART_HandleTypeDef* h = wire_uart();
    if (__HAL_UART_GET_FLAG(h, UART_FLAG_ORE)) __HAL_UART_CLEAR_OREFLAG(h);
#if defined(BOARD_WIRE_DMA_REQ)
    if (s_rx_por_dma) return;   /* el RDR es del DMA: leerlo aquí le quitaría el byte */
#endif
    while (__HAL_UART_GET_FLAG(h, UART_FLAG_RXNE)) {        /* RXFNE con FIFO on */
        uint8_t b = (uint8_t) (h->Instance->RDR & 0xFFU);
        uint16_t nh = (uint16_t) ((s_rx_head + 1u) % RX_RING_SZ);
        if (nh != s_rx_tail) { s_rx_ring[s_rx_head] = b; s_rx_head = nh; }
        /* ring lleno → descartar (no debería pasar con tráfico de comandos) */
    }
}

void stm32_wire_rx_arrancar(void) {
#if defined(BOARD_WIRE_DMA_REQ)
    int paso = rx_dma_arrancar();
    if (paso == 0) { s_rx_por_dma = 1; return; }
    log_printf("wire: el DMA de la RX no arranca (paso %d): la RX va por interrupcion", paso);
#endif
    UART_HandleTypeDef* h = wire_uart();
#if defined(BOARD_WIRE_IRQ_PRIO)
    /* Placa cuyo USART del wire NO lo gestiona CubeMX (Nucleo: USART1 lo inicia el
     * BSP, no está en el .ioc) → fijamos aquí la prioridad NVIC. El DK2 no define
     * BOARD_WIRE_IRQ_PRIO: respeta la que ya fijó CubeMX en el .ioc. */
    HAL_NVIC_SetPriority(BOARD_WIRE_IRQn, BOARD_WIRE_IRQ_PRIO, 0);
#endif
    HAL_NVIC_EnableIRQ(BOARD_WIRE_IRQn);        /* NVIC */
    __HAL_UART_ENABLE_IT(h, UART_IT_RXNE);      /* = RXFNE con FIFO → IRQ al llegar byte */
}

int stm32_wire_getchar(void) {
#if defined(BOARD_WIRE_DMA_REQ)
    /* Con DMA no hay ISR que mire el desbordamiento: si el DMA no llegara a tiempo a la
     * FIFO de 8 B, se han perdido bytes, y eso se dice (no debería pasar). */
    if (s_rx_por_dma && __HAL_UART_GET_FLAG(wire_uart(), UART_FLAG_ORE)) {
        __HAL_UART_CLEAR_OREFLAG(wire_uart());
        log_printf("wire: la RX del USART se ha desbordado (ORE) con DMA: se han perdido bytes");
    }
#endif
    uint16_t head = rx_head();
    if (s_rx_tail != head) {
        uint8_t b = s_rx_ring[s_rx_tail];
        s_rx_tail = (uint16_t) ((s_rx_tail + 1u) % RX_RING_SZ);
        return (int) b;
    }
    return -1;
}

#else  /* ---- resto de placas STM32: RX directo del registro (sin cambios) ---- */

int stm32_wire_getchar(void) {
    UART_HandleTypeDef* h = wire_uart();
    /* Overrun: límpialo para no bloquear la RX si un burst nos adelantó. */
    if (__HAL_UART_GET_FLAG(h, UART_FLAG_ORE)) {
        __HAL_UART_CLEAR_OREFLAG(h);
    }
    if (__HAL_UART_GET_FLAG(h, UART_FLAG_RXNE)) {
        return (int) (uint8_t) (h->Instance->RDR & 0xFFU);
    }
    return -1;
}

#endif

void stm32_wire_write(const char* buf, size_t len) {
    if (len == 0) return;
    HAL_UART_Transmit(wire_uart(), (uint8_t*) buf, (uint16_t) len, HAL_MAX_DELAY);
}

int wire_v1_recv_line(int first_char, char* buf, size_t max) {
    size_t n = 0;
    if (first_char >= 0 && first_char != '\n') {
        if (n < max) buf[n++] = (char) first_char;
        else return -1;
    }
    uint32_t last = HAL_GetTick();
    for (;;) {
        int c = stm32_wire_getchar();
        if (c < 0) {
            /* Anti-cuelgue: si la línea se estanca (byte perdido), abortar en
             * vez de girar para siempre — el REPL sigue vivo, el IDE reintenta. */
            if (HAL_GetTick() - last >= 300U) return -2;
            continue;
        }
        last = HAL_GetTick();
        if (c == '\n') return (int) n;  /* fin de línea */
        if (c == '\r') continue;        /* tolerar CRLF */
        if (n >= max) return -1;        /* overflow */
        buf[n++] = (char) c;
    }
}

/* V6/#473 — LA LINEA ES ATOMICA, y lo exige el contrato desde siempre
 * (include/bpvm_wire_v1.h). Lo cumplia SOLO la Pico. Dos escritores reales: el
 * hilo `io` con los OUTPUT del programa y la tarea del wire con sus replies (el
 * poll atiende HELLO/BUSY en caliente). Sin cerrojo, dos JSON entrelazados en el
 * IDE: corrupcion de framing, y no pasa en el PC. El cerrojo es del CABLE: el
 * `tx_mtx` del comun no cubre el poll y ademas vive dentro de la VM. */
static SemaphoreHandle_t s_tx_mutex = NULL;

static void tx_lock(void) {
    if (s_tx_mutex == NULL) s_tx_mutex = xSemaphoreCreateMutex();
    if (s_tx_mutex != NULL) xSemaphoreTake(s_tx_mutex, portMAX_DELAY);
}
static void tx_unlock(void) {
    if (s_tx_mutex != NULL) xSemaphoreGive(s_tx_mutex);
}

void wire_v1_send_line(const char* data, size_t len) {
    tx_lock();
    stm32_wire_write(data, len);
    stm32_wire_write("\n", 1);
    tx_unlock();
}

/* [V6/U2] send_cstr y send_error se fueron al comun (`wire_v1_proto.c`).
 * El de aqui metia `message` SIN ESCAPAR: una comilla en un mensaje de error
 * rompia el framing. El comun escapa siempre (field_string → put_escaped). */

void stm32_wire_send_fatal(const char* code, const char* message) {
    BOARD_LED_ERR_ON();    /* señal de error: LED encendido hasta el reset — el
                            * matiz de esta placa; el JSON lo arma el comun */
    wire_v1_send_fatal(code, message);
}

int wire_v1_recv_bulk(uint8_t* buf, size_t n, size_t buf_max) {
    if (n > buf_max) return -1;   /* el contrato: nunca desbordar al llamante */
    size_t got = 0;
    uint32_t last = HAL_GetTick();
    while (got < n) {
        int c = stm32_wire_getchar();
        if (c < 0) {
            if (HAL_GetTick() - last >= WIRE_V1_BULK_ESTANCADO_MS) return -2;   /* bulk estancado (bpvm_wire_v1.h) */
            continue;
        }
        last = HAL_GetTick();
        buf[got++] = (uint8_t) c;
    }
    return (int) n;
}

void wire_v1_send_bulk(const uint8_t* data, size_t n) {
    size_t off = 0;
    while (off < n) {
        size_t chunk = n - off;
        if (chunk > 0xFFFFU) chunk = 0xFFFFU;   /* HAL usa uint16 len */
        HAL_UART_Transmit(wire_uart(), (uint8_t*) (data + off),
                          (uint16_t) chunk, HAL_MAX_DELAY);
        off += chunk;
    }
}

/* #495 (paso 4) — la línea y sus bytes con el cerrojo cogido UNA vez: lo que tiene que ir junto (bpvm_wire_v1.h). */
void wire_v1_send_msg(const char* line, size_t len, const uint8_t* bulk, size_t n) {
    tx_lock();
    stm32_wire_write(line, len);
    stm32_wire_write("\n", 1);
    if (n) wire_v1_send_bulk(bulk, n);   /* el bulk no coge cerrojo: va por tramos (el uint16 de la HAL) */
    tx_unlock();
}

int stm32_wire_json_escape(const char* src, size_t srclen, char* dst, size_t dstmax) {
    size_t o = 0;
    for (size_t i = 0; i < srclen; i++) {
        unsigned char c = (unsigned char) src[i];
        const char* esc = NULL;
        char tmp[8];
        switch (c) {
            case '"':  esc = "\\\""; break;
            case '\\': esc = "\\\\"; break;
            case '\n': esc = "\\n";  break;
            case '\r': esc = "\\r";  break;
            case '\t': esc = "\\t";  break;
            default:
                if (c < 0x20U) { snprintf(tmp, sizeof(tmp), "\\u%04x", c); esc = tmp; }
                break;
        }
        if (esc) {
            size_t l = strlen(esc);
            if (o + l >= dstmax) return -1;
            memcpy(dst + o, esc, l);
            o += l;
        } else {
            if (o + 1 >= dstmax) return -1;
            dst[o++] = (char) c;   /* ASCII imprimible y bytes UTF-8 tal cual */
        }
    }
    if (o >= dstmax) return -1;
    dst[o] = '\0';
    return (int) o;
}
