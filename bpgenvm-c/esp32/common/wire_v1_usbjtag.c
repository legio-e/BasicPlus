/*
 * wire_v1_usbjtag.c — el CABLE del wire v1 sobre el USB-Serial-JTAG nativo.
 *
 * ─── Por qué existe (V6/P1.C3.3, 31-ago-2026) ───────────────────────────────
 *
 * El reparto de puertos de esta familia venía del S3 DevKitC, que tiene DOS
 * conectores USB: la consola por el USB-Serial-JTAG y el wire por UART0, que
 * allí sale a un puente USB-UART. Así el log de texto no ensucia el binario del
 * wire, y el IDE se conecta al puente.
 *
 * En una placa de UN SOLO conector —el C3 del ensayo— ese reparto deja al wire
 * sin salida: UART0 va a pines pelados. El razonamiento que lo desatasca es de
 * Eduardo, y es sobre la PLACA, no sobre el software:
 *
 *   «Tenemos un solo conector USB pero a cambio tenemos 2 pulsadores, uno para
 *    reset y otro para boot. Si queremos grabar la imagen hay que pulsar los 2
 *    y soltar reset. En un arranque normal el USB es NUESTRO.»
 *
 * Es decir: el USB no hace falta para el handshake de grabación —eso se hace a
 * mano con los botones—, así que en marcha está libre. Y si está libre, lo que
 * debe ocuparlo es **el wire**, porque es lo que necesita el usuario: un cable,
 * el IDE conecta, y ya. La consola se va a UART0, que es donde tiene sentido
 * que esté lo que sólo miramos nosotros cuando algo va mal.
 *
 * ⚠️ EL DEFECTO ES POR CHIP, no por familia. El S3 y el P4 tienen dos puertos y
 * su reparto funciona: no se tocan. Lo elige el `CMakeLists` de cada proyecto,
 * en el mismo hueco de transporte que el P4 ya usaba para su variante TCP.
 *
 * ─── El contrato ────────────────────────────────────────────────────────────
 *
 * Implementa exactamente el mismo que `wire_v1.c` (`include/bpvm_wire_v1.h`):
 * el CABLE. Todo lo de arriba —los constructores JSON, los verbos— es común y
 * no se entera de por dónde viaja.
 */
#include "wire_v1.h"
#include "bpvm_platform.h"   /* V6/#473: bpvm_platform_now_ms */
#include "log.h"             /* #545: un corte se dice en el log (sólo va al anillo de RAM) */

#include "driver/usb_serial_jtag.h"
#include "soc/io_mux_reg.h"   /* #574: USB_INT_PHY0_DM/DP_GPIO_NUM, los pines del USB nativo */
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <string.h>

/* El driver pide sus dos buffers. RX holgado por la misma razón que en UART0:
 * las tramas del wire llevan bulk detrás (PUT de .mod y de recursos). */
#define WIRE_RX_BUF   4096
#define WIRE_TX_BUF   2048

/* Cuánto se espera como mucho a que el host lea. Si el IDE se desconecta a media
 * respuesta, `usb_serial_jtag_write_bytes` se queda esperando sitio; con tope se
 * pierde ESA respuesta y la placa sigue viva, que es lo que se quiere. Sin tope
 * la VM se quedaría colgada en un printf. */
#define WIRE_TX_MS    2000

static const char* s_nombre = "USB-Serial-JTAG";

void wire_v1_transport_init(void) {
    usb_serial_jtag_driver_config_t cfg = {
        .tx_buffer_size = WIRE_TX_BUF,
        .rx_buffer_size = WIRE_RX_BUF,
    };
    /* Si el driver no instala, el wire queda mudo y HOY ESO NO SE VEIA: la placa
     * arrancaba entera y simplemente no contestaba. Que el nombre lo diga es lo
     * unico que hace falta para no perder una tarde. */
    esp_err_t rc = usb_serial_jtag_driver_install(&cfg);
    if (rc != ESP_OK) s_nombre = "USB-Serial-JTAG (FALLO AL INSTALAR)";
}

const char* wire_v1_transport_name(void) { return s_nombre; }

/* #574 — los pines que este cable le quita al programa: el D- y el D+ del USB nativo del chip (C3 18/19, C5 13/14,
 * C6 12/13; los dice el IDF para cada uno). */
uint64_t wire_v1_transport_pines(void) {
    return (1ULL << USB_INT_PHY0_DM_GPIO_NUM) | (1ULL << USB_INT_PHY0_DP_GPIO_NUM);
}

/* ===================== Lectura ===================== */

/* Bloquea hasta `ms` cediendo CPU dentro del driver — MISMO motivo que en
 * UART0: pollear con timeout 0 + vTaskDelay(0) no cede y el watchdog mata a
 * IDLE. Devuelve el byte o -1 si venció sin datos. */
static int wire_read_byte(int ms) {
    uint8_t b;
    int n = usb_serial_jtag_read_bytes(&b, 1, pdMS_TO_TICKS(ms));
    return (n == 1) ? (int) b : -1;
}

/* P-run-stop (#257) — variante NO bloqueante para el poll de KILL que la VM
 * invoca entre quanta durante un RUN. */
int wire_v1_try_getchar(void) {
    uint8_t b;
    int n = usb_serial_jtag_read_bytes(&b, 1, 0);
    return (n == 1) ? (int) b : -1;
}

int wire_v1_recv_line(int first_char_already_read, char* buf, size_t buf_max) {
    size_t n = 0;
    if (first_char_already_read >= 0) {
        if (n + 1 >= buf_max) return -1;
        buf[n++] = (char) first_char_already_read;
    }
    int64_t ultimo = bpvm_platform_now_ms();   /* V6/#473: plazo de linea estancada */
    for (;;) {
        int c = wire_read_byte(100);    /* bloquea <=100 ms cediendo CPU */
        if (c < 0) {                     /* timeout sin datos */
            /* Sin plazo, un truncado se come el mensaje siguiente. El porque
             * entero, en include/bpvm_wire_v1.h (WIRE_V1_ESTANCADA_MS). */
            if (n > 0 && bpvm_platform_now_ms() - ultimo >= (int64_t) WIRE_V1_ESTANCADA_MS)
                return -2;
            continue;
        }
        ultimo = bpvm_platform_now_ms();
        if (c == '\n') {
            return (int) n;
        }
        if (c == '\r') continue;
        if (n + 1 >= buf_max) return -1;
        buf[n++] = (char) c;
    }
}

int wire_v1_recv_bulk(uint8_t* buf, size_t n, size_t buf_max) {
    if (n > buf_max) return -1;
    size_t got = 0;
    int64_t ultimo = bpvm_platform_now_ms();   /* #495: plazo de bulk estancado (bpvm_wire_v1.h) */
    while (got < n) {
        int r = usb_serial_jtag_read_bytes(buf + got, n - got, pdMS_TO_TICKS(200));
        if (r > 0) { got += (size_t) r; ultimo = bpvm_platform_now_ms(); continue; }
        if (bpvm_platform_now_ms() - ultimo >= (int64_t) WIRE_V1_BULK_ESTANCADO_MS) return -2;
    }
    return (int) n;
}

/* ===================== Escritura ===================== */

/* #545 — EL DRIVER ES TODO O NADA. Aquí se creía que «puede escribir menos de lo
 * pedido si su buffer se llena», y no: `usb_serial_jtag_write_bytes` mete el trozo
 * ENTERO en su anillo (un `xRingbufferSend` del trozo completo, IDF v6.0.1) o, al
 * vencer el plazo, devuelve 0. Con un anillo de WIRE_TX_BUF, un trozo mayor NO
 * CABE NUNCA: a los WIRE_TX_MS devolvía 0, la línea se perdía entera, salía sólo
 * el `\n` y la placa seguía como si nada. Lo destapó `PACK_LS` en la C5 (#539):
 * su respuesta pasó de ~2.025 B a ~2.055 y dejó de llegar. Callaban igual cualquier
 * otra respuesta de más de 2 KB de las placas de un solo USB (C3, C6, C5): un
 * `PACK_ENTRIES` largo, un `ENV_LS` con muchas claves, las del depurador.
 *
 * Así que se manda en TRAMOS que caben siempre, y cada uno espera su sitio.
 * `wire_v1.c` no lo necesita: su UART va con tx_buffer=0 (síncrono). */
#define WIRE_TX_TRAMO (WIRE_TX_BUF / 2)

/* Un aviso por corte, no uno por tramo: si el host se va a media descarga, un GET
 * de 120 KB son 472 tramos, y el log de arranque (el que sobrevive al reset) no
 * se puede llenar de lo mismo. Se rearma en cuanto una escritura sale entera. */
static int s_tx_cortado = 0;

static void wire_write(const char* data, size_t n) {
    size_t off = 0;
    while (off < n) {
        size_t tramo = n - off;
        if (tramo > WIRE_TX_TRAMO) tramo = WIRE_TX_TRAMO;
        int w = usb_serial_jtag_write_bytes(data + off, tramo, pdMS_TO_TICKS(WIRE_TX_MS));
        if (w <= 0) {                /* nadie lee: se pierde esta respuesta, no la placa */
            if (!s_tx_cortado)
                log_printf("wire: el host no lee en %d ms: respuesta cortada a %u de %u B",
                           WIRE_TX_MS, (unsigned) off, (unsigned) n);
            s_tx_cortado = 1;
            return;
        }
        off += (size_t) w;
    }
    s_tx_cortado = 0;
}

/* V6/#473 — LA LINEA ES ATOMICA, y lo exige el contrato desde siempre
 * (include/bpvm_wire_v1.h: «La linea debe ser ATOMICA frente a otros
 * escritores»). Lo cumplia SOLO la Pico; aqui estaba vigilado, no construido.
 *
 * Los dos escritores concurrentes son reales: el hilo `io` saca los OUTPUT del
 * programa mientras la tarea del wire contesta sus replies — el poll atiende
 * HELLO/BUSY EN CALIENTE, durante un run. Sin cerrojo, el IDE recibe dos JSON
 * entrelazados: corrupcion de framing, no estetica. Y no pasa en el PC.
 *
 * El cerrojo es del CABLE, no de la VM: el `tx_mtx` del comun (src/bpvm_io.c:84)
 * solo cubre la salida del programa —el poll contesta fuera de el— y ademas vive
 * dentro de la VM, asi que no existe entre ejecuciones.
 *
 * Init perezoso, y es seguro: la primera escritura es el banner de arranque, con
 * una sola tarea viva. Y tomar un mutex libre NO bloquea, asi que vale incluso
 * antes de que arranque el planificador. */
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
    if (len) wire_write(data, len);
    wire_write("\n", 1);
    tx_unlock();
}

void wire_v1_send_bulk(const uint8_t* data, size_t n) {
    if (n) wire_write((const char*) data, n);
}

/* #495 (paso 4) — la línea y sus bytes con el cerrojo cogido UNA vez: lo que tiene que ir junto (bpvm_wire_v1.h). */
void wire_v1_send_msg(const char* line, size_t len, const uint8_t* bulk, size_t n) {
    tx_lock();
    if (len) wire_write(line, len);
    wire_write("\n", 1);
    if (n) wire_write((const char*) bulk, n);
    tx_unlock();
}
