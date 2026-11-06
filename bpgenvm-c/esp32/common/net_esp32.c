/*
 * net_esp32.c — V8/H3 (6-nov): la IP de la familia ESP. Lo que dice /sys/net.json lo decide bpvm_red.c; aquí se le pasa a
 * `esp_netif`. El enlace no se toca: de esto a la radio sólo llega la interfaz de la estación, que lwIP conecta sola.
 *
 * LOS SOCKETS DE LOS PROGRAMAS (`Net`) son los de lwIP, con el contrato de bpvm_net.h: un entero pequeño de cara a BP, no el
 * descriptor. Sin red, `connect` da 0: no se pudo, que es un resultado normal. Cada conexión abierta cuenta como actividad
 * para el apagado de la WiFi. Lo que un programa deje abierto se cierra al acabar: `Net.Tcp` tiene destructor, pero con
 * `tcpConnect` a secas no hay objeto que lo cierre.
 *
 * Bloquean la tarea del programa, como en el PC: por eso `recv` lleva siempre plazo.
 */
#include "net_esp32.h"
#include "bpvm_net.h"
#include "bpvm_platform.h"
#include "bpvm_red.h"
#include "bpvm_wifi.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define NET_MAX           8       /* conexiones de los programas a la vez */
#define NET_PLAZO_MS      5000    /* conectar, si el programa no dice otro */
#define NET_ENVIO_S       10      /* un envío que no sale en este tiempo, conexión rota */

static esp_netif_t* s_sta = NULL;

static int s_fd[NET_MAX];         /* el descriptor + 1; 0, libre. El handle de BP es la casilla + 1 */
static bpvm_platform_mutex_handle_t s_m;
static int s_m_listo = 0;

/* En la tarea de eventos del IDF; bpvm_red_hay_ip escribe en el log, y por eso su pila va a 4 KB (sdkconfig.defaults). */
static void al_evento(void* arg, esp_event_base_t base, int32_t id, void* datos) {
    (void) arg; (void) base; (void) datos;
    if (id == IP_EVENT_STA_GOT_IP)       bpvm_red_hay_ip(1);
    else if (id == IP_EVENT_STA_LOST_IP) bpvm_red_hay_ip(0);
}

int net_esp32_preparar(char* err, size_t n) {
    esp_err_t r;
    if (s_sta) return 0;
    r = esp_netif_init();
    if (r != ESP_OK) { snprintf(err, n, "esp_netif_init: %s", esp_err_to_name(r)); return -1; }
    r = esp_event_loop_create_default();
    if (r != ESP_OK && r != ESP_ERR_INVALID_STATE) {
        snprintf(err, n, "el bucle de eventos: %s", esp_err_to_name(r));
        return -1;
    }
    r = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, al_evento, NULL);
    if (r != ESP_OK) { snprintf(err, n, "los eventos de la IP: %s", esp_err_to_name(r)); return -1; }
    s_sta = esp_netif_create_default_wifi_sta();
    if (!s_sta) { snprintf(err, n, "la interfaz de la estacion"); return -1; }
    return 0;
}

static int ip_dinamica(const char* nombre) {
    esp_err_t r;
    if (!s_sta) return -1;
    r = esp_netif_dhcpc_start(s_sta);   /* si venía de una IP fija */
    if (r != ESP_OK && r != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) return -1;
    return esp_netif_set_hostname(s_sta, nombre) == ESP_OK ? 0 : -1;
}

static int ip_fija(uint32_t ip, uint32_t mascara, uint32_t puerta, uint32_t dns) {
    esp_netif_ip_info_t i;
    esp_err_t r;
    if (!s_sta) return -1;
    r = esp_netif_dhcpc_stop(s_sta);
    if (r != ESP_OK && r != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) return -1;
    memset(&i, 0, sizeof i);
    i.ip.addr      = ip;
    i.netmask.addr = mascara;
    i.gw.addr      = puerta;
    if (esp_netif_set_ip_info(s_sta, &i) != ESP_OK) return -1;
    if (dns) {
        esp_netif_dns_info_t d;
        memset(&d, 0, sizeof d);
        d.ip.type = ESP_IPADDR_TYPE_V4;
        d.ip.u_addr.ip4.addr = dns;
        if (esp_netif_set_dns_info(s_sta, ESP_NETIF_DNS_MAIN, &d) != ESP_OK) return -1;
    }
    return 0;
}

static int ip_leer(uint32_t* ip, uint32_t* mascara, uint32_t* puerta, uint32_t* dns) {
    esp_netif_ip_info_t i;
    esp_netif_dns_info_t d;
    if (!s_sta || esp_netif_get_ip_info(s_sta, &i) != ESP_OK) return -1;
    memset(&d, 0, sizeof d);
    (void) esp_netif_get_dns_info(s_sta, ESP_NETIF_DNS_MAIN, &d);
    *ip      = i.ip.addr;
    *mascara = i.netmask.addr;
    *puerta  = i.gw.addr;
    *dns     = d.ip.u_addr.ip4.addr;
    return 0;
}

/* ── Los sockets ───────────────────────────────────────────────────────────────────────────────────────────────────── */

/* Dos runs pueden abrir a la vez. */
static int apunta(int fd) {
    int h = 0;
    bpvm_platform_mutex_lock(&s_m);
    for (int i = 0; i < NET_MAX && !h; i++)
        if (s_fd[i] == 0) { s_fd[i] = fd + 1; h = i + 1; }
    bpvm_platform_mutex_unlock(&s_m);
    return h;
}

static int fd_de(int h) {
    return (h >= 1 && h <= NET_MAX && s_fd[h - 1] != 0) ? s_fd[h - 1] - 1 : -1;
}

/* Espera a que se pueda leer, o escribir, hasta `ms`. 1, 0 si vence el plazo, -1 si falla. */
static int espera(int fd, int para_escribir, int ms) {
    fd_set f;
    struct timeval tv = { .tv_sec = ms / 1000, .tv_usec = (ms % 1000) * 1000 };
    FD_ZERO(&f);
    FD_SET(fd, &f);
    return select(fd + 1, para_escribir ? NULL : &f, para_escribir ? &f : NULL, NULL, &tv);
}

static int n_connect(const char* host, int port, int timeout_ms) {
    struct addrinfo pista, *res = NULL;
    char puerto[8];
    int fd, h, r, err = 0, si = 1;
    socklen_t largo = sizeof err;
    struct timeval tv = { .tv_sec = NET_ENVIO_S, .tv_usec = 0 };
    if (timeout_ms <= 0) timeout_ms = NET_PLAZO_MS;
    memset(&pista, 0, sizeof pista);
    pista.ai_family = AF_INET;
    pista.ai_socktype = SOCK_STREAM;
    snprintf(puerto, sizeof puerto, "%d", port);
    if (getaddrinfo(host, puerto, &pista, &res) != 0 || !res) return 0;
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { freeaddrinfo(res); return 0; }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    r = connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (r != 0 && errno != EINPROGRESS) { close(fd); return 0; }
    if (r != 0 && (espera(fd, 1, timeout_ms) != 1 || getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &largo) != 0 || err != 0)) {
        close(fd);
        return 0;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &si, sizeof si);
    h = apunta(fd);
    if (!h) { close(fd); return 0; }
    bpvm_wifi_conexion(1);
    return h;
}

static int n_send(int h, const void* buf, int len) {
    const uint8_t* p = (const uint8_t*) buf;
    int fd = fd_de(h), hecho = 0;
    if (fd < 0) return BPVM_NET_ERR;
    while (hecho < len) {
        int w = send(fd, p + hecho, (size_t) (len - hecho), 0);
        if (w <= 0) return BPVM_NET_ERR;
        hecho += w;
    }
    return hecho;
}

static int n_recv(int h, void* buf, int max, int timeout_ms) {
    int r, fd = fd_de(h);
    if (fd < 0) return BPVM_NET_ERR;
    if (timeout_ms <= 0) timeout_ms = 1;
    r = espera(fd, 0, timeout_ms);
    if (r == 0) return BPVM_NET_RECV_TIMEOUT;
    if (r < 0) return BPVM_NET_ERR;
    r = recv(fd, buf, (size_t) max, 0);
    if (r == 0) return BPVM_NET_CLOSED;
    return (r < 0) ? BPVM_NET_ERR : r;
}

static void n_close(int h) {
    int fd;
    bpvm_platform_mutex_lock(&s_m);
    fd = fd_de(h);
    if (fd >= 0) s_fd[h - 1] = 0;
    bpvm_platform_mutex_unlock(&s_m);
    if (fd < 0) return;
    close(fd);
    bpvm_wifi_conexion(0);
}

static void n_fin_de_run(void) {
    for (int h = 1; h <= NET_MAX; h++) n_close(h);
}

static const bpvm_net_backend_t BACKEND = {
    .connect     = n_connect,
    .send        = n_send,
    .recv        = n_recv,
    .close       = n_close,
    .fin_de_run  = n_fin_de_run,
    .ip_dinamica = ip_dinamica,
    .ip_fija     = ip_fija,
    .ip_leer     = ip_leer,
};

void net_esp32_registrar(void) {
    if (!s_m_listo) {
        if (bpvm_platform_mutex_init(&s_m) != 0) return;
        s_m_listo = 1;
    }
    bpvm_net_set_backend(&BACKEND);
}
