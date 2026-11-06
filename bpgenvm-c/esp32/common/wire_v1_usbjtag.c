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
#include "bpvm_wire_cables.h"   /* V8/H3: este cable, y encima los dos */
#include "log.h"             /* #545: un corte se dice en el log (sólo va al anillo de RAM) */

#include "driver/usb_serial_jtag.h"
#include "soc/io_mux_reg.h"   /* #574: USB_INT_PHY0_DM/DP_GPIO_NUM, los pines del USB nativo */
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

const char* wire_v1_transport_name(void) { return s_nombre; }

/* #574 — los pines que este cable le quita al programa: el D- y el D+ del USB nativo del chip (C3 18/19, C5 13/14,
 * C6 12/13; los dice el IDF para cada uno). */
uint64_t wire_v1_transport_pines(void) {
    return (1ULL << USB_INT_PHY0_DM_GPIO_NUM) | (1ULL << USB_INT_PHY0_DP_GPIO_NUM);
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

/* V8/H3 (6-nov) — ESTE CABLE SÓLO MUEVE BYTES: la línea, el bulk, sus plazos y el cerrojo son de src/wire_cables.c.
 *
 * Bloquea hasta `ms` cediendo CPU dentro del driver — MISMO motivo que en UART0: pollear con timeout 0 + vTaskDelay(0) no cede
 * y el watchdog mata a IDLE. Una espera de menos de un tick es un tick. */
static int usb_leer(uint8_t* buf, size_t n, int ms) {
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if (ms > 0 && ticks == 0) ticks = 1;
    int r = usb_serial_jtag_read_bytes(buf, n, ticks);
    return (r > 0) ? r : 0;
}

static void usb_escribir(const uint8_t* datos, size_t n) { wire_write((const char*) datos, n); }

static const bpvm_wire_cable_t CABLE_USB = { "USB-Serial-JTAG", usb_leer, usb_escribir, NULL, NULL, NULL };

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
    bpvm_wire_cables_local(&CABLE_USB);
}
