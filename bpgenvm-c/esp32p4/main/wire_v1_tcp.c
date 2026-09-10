/*
 * wire_v1_tcp.c (ESP32-P4) — framing del wire BPVM v1 sobre TCP (sockets lwIP).
 *
 * Hermano de transporte de esp32/main/wire_v1.c (UART0) y pico/wire_v1.c
 * (USB-CDC): MISMA API (wire_v1.h), de modo que el dispatcher repl_esp32.c se
 * reutiliza TAL CUAL — el P4 no necesita su propio REPL. La diferencia es que
 * el P4 es SERVIDOR: el IDE conecta por Ethernet a 192.168.2.2:<port>.
 *
 * Decisión clave: el accept()/reconnect vive DENTRO de la lectura.
 *   - camino "idle" (repl_esp32_run llama recv_line(-1,...)): si no hay cliente
 *     se bloquea en accept(); si el cliente cae, cierra y vuelve a accept() de
 *     forma transparente — el bucle del REPL no se entera.
 *   - camino "poll" (la VM llama recv_line(c,...) entre quanta durante un RUN):
 *     NO se re-acepta; una caída devuelve -2 y el run sigue sin colgarse
 *     esperando reconexión (la salida y el EXITED simplemente se descartan).
 *
 * Los builders JSON son COPIA de wire_v1.c — misma política deliberada que
 * S3<->Pico ("cada firmware lleva su copia para no acoplar los transportes").
 * Si cambia el formato del protocolo, mantener las tres copias en sync.
 */
#include "wire_v1.h"
#include "bpvm_platform.h"   /* V6/#473: bpvm_platform_now_ms */
#include "wire_v1_tcp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

/* Log de bring-up (P4->PC:5555). Definido en main.c (no static). Si el canal
 * no está conectado, net_logf cae a la consola (USB-Serial-JTAG). */
extern void net_logf(const char *fmt, ...);

#define WIRE_RECV_TIMEOUT_MS 100
#define WIRE_DISCONNECTED    (-2)

static int s_listen_fd = -1;
static int s_client_fd = -1;

/* ===================== Servidor / conexión ===================== */

void wire_v1_tcp_server_init(int port)
{
    s_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_listen_fd < 0) { net_logf("[p4] wire: socket() err %d", errno); return; }

    int yes = 1;
    setsockopt(s_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t) port);

    if (bind(s_listen_fd, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
        net_logf("[p4] wire: bind(%d) err %d", port, errno);
        close(s_listen_fd); s_listen_fd = -1; return;
    }
    if (listen(s_listen_fd, 1) != 0) {
        net_logf("[p4] wire: listen err %d", errno);
        close(s_listen_fd); s_listen_fd = -1; return;
    }
    net_logf("[p4] wire v1 (TCP) escuchando en *:%d", port);
}

static void wire_close_client(void)
{
    if (s_client_fd >= 0) { close(s_client_fd); s_client_fd = -1; }
}

/* Bloquea en accept() hasta que haya cliente. Fija SO_RCVTIMEO (para que los
 * recv bloqueantes cedan CPU en vez de busy-spin) y TCP_NODELAY (respuestas
 * pequeñas sin esperar a Nagle: el wire es muchos mensajes cortos). */
static void ensure_client(void)
{
    while (s_client_fd < 0) {
        if (s_listen_fd < 0) { vTaskDelay(pdMS_TO_TICKS(1000)); continue; }
        struct sockaddr_in peer; socklen_t pl = sizeof(peer);
        int fd = accept(s_listen_fd, (struct sockaddr *) &peer, &pl);
        if (fd < 0) { vTaskDelay(pdMS_TO_TICKS(200)); continue; }
        struct timeval tv = { .tv_sec = 0, .tv_usec = WIRE_RECV_TIMEOUT_MS * 1000 };
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        int yes = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
        s_client_fd = fd;
        net_logf("[p4] wire: cliente conectado %s:%d",
                 inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
    }
}

/* ===================== Lectura ===================== */

/* Lee 1 byte. allow_accept=1 (idle): si no hay cliente, bloquea en accept.
 * allow_accept=0 (poll de un RUN): NO acepta (devuelve WIRE_DISCONNECTED) para
 * no colgar la VM esperando reconexión. Devuelve el byte, -1 si venció el
 * timeout sin datos, o WIRE_DISCONNECTED si el cliente cayó. */
static int wire_read_byte(int allow_accept)
{
    if (allow_accept) ensure_client();
    if (s_client_fd < 0) return WIRE_DISCONNECTED;
    uint8_t b;
    int n = recv(s_client_fd, &b, 1, 0);     /* bloquea <= SO_RCVTIMEO */
    if (n == 1) return (int) b;
    if (n == 0) { net_logf("[p4] wire: cliente desconectado"); wire_close_client(); return WIRE_DISCONNECTED; }
    if (errno == EWOULDBLOCK || errno == EAGAIN) return -1;   /* timeout sin datos */
    net_logf("[p4] wire: recv err %d", errno); wire_close_client(); return WIRE_DISCONNECTED;
}

int wire_v1_try_getchar(void)
{
    if (s_client_fd < 0) return -1;
    uint8_t b;
    int n = recv(s_client_fd, &b, 1, MSG_DONTWAIT);
    if (n == 1) return (int) b;
    if (n == 0) { net_logf("[p4] wire: cliente cayó (poll RUN)"); wire_close_client(); return -1; }
    return -1;   /* EAGAIN: nada pendiente */
}

int wire_v1_recv_line(int first_char_already_read, char *buf, size_t buf_max)
{
    int from_poll = (first_char_already_read >= 0);
    size_t n = 0;
    if (first_char_already_read >= 0) {
        if (n + 1 >= buf_max) return -1;
        buf[n++] = (char) first_char_already_read;
    }
    int64_t ultimo = bpvm_platform_now_ms();   /* V6/#473: plazo de linea estancada */
    for (;;) {
        int c = wire_read_byte(!from_poll);
        if (c == WIRE_DISCONNECTED) {
            if (from_poll) return -2;     /* no re-aceptar durante un run */
            n = 0; continue;              /* idle: el próximo wire_read_byte re-acepta */
        }
        if (c < 0) {                      /* timeout: reintenta */
            /* V6/#473 — con plazo: sin el, un mensaje truncado (el peer se cae a
             * mitad de envio) se come el siguiente. Ver include/bpvm_wire_v1.h. */
            if (n > 0 && bpvm_platform_now_ms() - ultimo >= (int64_t) WIRE_V1_ESTANCADA_MS)
                return -2;
            continue;
        }
        ultimo = bpvm_platform_now_ms();
        if (c == '\n') return (int) n;
        if (c == '\r') continue;
        if (n + 1 >= buf_max) return -1;  /* línea demasiado larga */
        buf[n++] = (char) c;
    }
}

int wire_v1_recv_bulk(uint8_t *buf, size_t n, size_t buf_max)
{
    if (n > buf_max) return -1;
    if (s_client_fd < 0) return -1;
    size_t got = 0;
    while (got < n) {
        int r = recv(s_client_fd, buf + got, n - got, 0);   /* <= SO_RCVTIMEO */
        if (r > 0) { got += (size_t) r; continue; }
        if (r == 0) {
            net_logf("[p4] wire: desconexión en bulk (%u/%u)", (unsigned) got, (unsigned) n);
            wire_close_client(); return -1;
        }
        if (errno == EWOULDBLOCK || errno == EAGAIN) continue;   /* timeout: sigue esperando el resto */
        net_logf("[p4] wire: bulk recv err %d", errno); wire_close_client(); return -1;
    }
    return (int) n;
}

/* ===================== Escritura ===================== */

static void wire_send_all(const void *data, size_t len)
{
    if (s_client_fd < 0 || len == 0) return;
    const uint8_t *p = (const uint8_t *) data;
    size_t sent = 0;
    while (sent < len) {
        int w = send(s_client_fd, p + sent, len - sent, 0);
        if (w > 0) { sent += (size_t) w; continue; }
        if (w < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) { vTaskDelay(pdMS_TO_TICKS(1)); continue; }
        net_logf("[p4] wire: send err %d -> cierro", errno);
        wire_close_client();
        return;
    }
}

void wire_v1_send_line(const char *data, size_t len)
{
    wire_send_all(data, len);
    wire_send_all("\n", 1);
}


void wire_v1_send_bulk(const uint8_t *data, size_t n)
{
    wire_send_all(data, n);
}


