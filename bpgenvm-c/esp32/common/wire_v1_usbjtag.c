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

#include "driver/usb_serial_jtag.h"
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
    while (got < n) {
        int r = usb_serial_jtag_read_bytes(buf + got, n - got, pdMS_TO_TICKS(200));
        if (r > 0) got += (size_t) r;
    }
    return (int) n;
}

/* ===================== Escritura ===================== */

/* El driver puede escribir menos de lo pedido si su buffer se llena, así que se
 * insiste hasta vaciar o hasta que el tope diga que no hay nadie al otro lado.
 * `wire_v1.c` no lo necesita porque su UART va con tx_buffer=0 (síncrono). */
static void wire_write(const char* data, size_t n) {
    size_t off = 0;
    while (off < n) {
        int w = usb_serial_jtag_write_bytes(data + off, n - off, pdMS_TO_TICKS(WIRE_TX_MS));
        if (w <= 0) return;          /* nadie lee: se pierde esta respuesta, no la placa */
        off += (size_t) w;
    }
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
