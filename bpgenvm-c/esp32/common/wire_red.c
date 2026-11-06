/*
 * wire_red.c — V8/H3 (6-nov): EL CABLE DE LA RED. El IDE habla con la placa por TCP, con los mismos mensajes que por la UART
 * (Eduardo, 19-oct: «el uso más inmediato: el IDE conectado a la placa por WiFi»). Viene de esp32p4/main/wire_v1_tcp.c, el
 * de la Ethernet de la P4 Kit (V3), con dos diferencias: no acepta dentro de la lectura, porque la tarea del REPL también
 * tiene que mirar el otro cable, y ya no es el único.
 *
 * SIN TAREA PROPIA: todo pasa en el sondeo, que llama la tarea del REPL mientras espera, y también durante un RUN: por eso se
 * puede entrar con un programa corriendo y pararlo.
 *
 * EL PUERTO se abre al tener IP y se cierra al perderla (al ir la WiFi, con la IP se va todo). El aviso llega por la tarea de
 * eventos: sólo deja una marca, y es el sondeo quien abre y cierra.
 *
 * UN CLIENTE: uno nuevo sustituye al anterior, que con la WiFi suele ser uno muerto. Para no quedarse con uno muerto sin saberlo:
 * SO_KEEPALIVE, y un envío atascado 5 s da al cliente por perdido. Una conexión abierta cuenta como actividad (bpvm_wifi.h).
 *
 * BUSCAR: un `{"type":"FIND"}` por UDP al mismo puerto, que el IDE manda a toda la red cuando se le pide, y cada placa contesta
 * quién es. La placa no se anuncia sola: mientras nadie pregunta, no gasta nada.
 *
 * LA SEGURIDAD, de momento no (Eduardo, 19-oct): la placa acepta a cualquier ordenador de la red.
 */
#include "wire_red.h"
#include "bpvm_machine.h"
#include "bpvm_platform.h"
#include "bpvm_red.h"
#include "bpvm_wifi.h"
#include "bpvm_wire_cables.h"
#include "bpvm_wire_v1.h"
#include "log.h"

#include "lwip/inet.h"
#include "lwip/sockets.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define MIRAR_CADA_MS     50      /* aceptar y contestar a una búsqueda: lo que tarda en verse, como mucho */
#define REINTENTO_MS      5000    /* si no se puede abrir el puerto */
#define ENVIO_ATASCADO_S  5
#define VIVO_INACTIVO_S   10      /* SO_KEEPALIVE: un cliente muerto se ve en 10 + 3 × 5 s */
#define VIVO_INTERVALO_S  5
#define VIVO_INTENTOS     3
#define RX_BYTES          512     /* lo que se lee de una vez: una línea son decenas de bytes */
#define TX_BYTES          1400    /* lo que cabe en un paquete */

static int s_escucha = -1, s_busca = -1, s_cliente = -1;
static volatile int s_hay_ip = 0;
static volatile int s_roto = 0;       /* al cliente le ha pasado algo: lo cierra el sondeo */
static int64_t s_mirado = 0, s_abrir_en = 0;

static uint8_t s_rx[RX_BYTES];
static size_t  s_rx_ini = 0, s_rx_fin = 0;
static int     s_rx_ms = -1;          /* el plazo que tiene puesto el socket */
static uint8_t s_tx[TX_BYTES];        /* con el cerrojo de la escritura */
static size_t  s_tx_n = 0;

static void al_cambiar_ip(int hay) { s_hay_ip = hay; }

/* ── El cliente ────────────────────────────────────────────────────────────────────────────────────────────────────── */

static int leer(uint8_t* buf, size_t n, int ms) {
    int r;
    if (s_cliente < 0 || s_roto) return -1;
    if (s_rx_ini == s_rx_fin) {
        uint8_t* destino = (n >= sizeof s_rx) ? buf : s_rx;
        size_t   cabe    = (n >= sizeof s_rx) ? n : sizeof s_rx;
        if (ms > 0 && ms != s_rx_ms) {
            struct timeval tv = { .tv_sec = ms / 1000, .tv_usec = (ms % 1000) * 1000 };
            setsockopt(s_cliente, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
            s_rx_ms = ms;
        }
        r = recv(s_cliente, destino, cabe, ms > 0 ? 0 : MSG_DONTWAIT);
        if (r == 0) { s_roto = 1; return -1; }                    /* el otro lado ha cerrado */
        if (r < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;
            s_roto = 1;
            return -1;
        }
        if (destino == buf) return r;
        s_rx_ini = 0;
        s_rx_fin = (size_t) r;
    }
    r = (int) (s_rx_fin - s_rx_ini);
    if ((size_t) r > n) r = (int) n;
    memcpy(buf, s_rx + s_rx_ini, (size_t) r);
    s_rx_ini += (size_t) r;
    return r;
}

/* SO_SNDTIMEO: si en 5 s no sale nada, el cliente no está. Los bytes se pierden: quien escribe no se queda colgado. */
static void enviar(const uint8_t* datos, size_t n) {
    while (n && !s_roto) {
        int w = send(s_cliente, datos, n, 0);
        if (w <= 0) { s_roto = 1; return; }
        datos += w;
        n -= (size_t) w;
    }
}

static void vacia(void) {
    if (s_tx_n && s_cliente >= 0) enviar(s_tx, s_tx_n);
    s_tx_n = 0;
}

/* Lo junta en un paquete. Con TCP_NODELAY cada `send` es un paquete, y una línea con su '\n' serían dos. */
static void escribir(const uint8_t* datos, size_t n) {
    if (s_cliente < 0 || s_roto) return;
    if (n > sizeof s_tx - s_tx_n) vacia();
    if (n >= sizeof s_tx) { enviar(datos, n); return; }
    memcpy(s_tx + s_tx_n, datos, n);
    s_tx_n += n;
}

static int abierto(void) { return s_cliente >= 0; }

/* Con el cerrojo, para que nadie esté escribiendo en él. Si alguien lleva 5 s atascado escribiendo, el shutdown lo suelta. 1 si
 * había cliente. */
static int cierra_cliente(void) {
    int fd = s_cliente;
    if (fd < 0) return 0;
    shutdown(fd, SHUT_RDWR);
    bpvm_wire_cables_bloquear();
    s_cliente = -1;
    s_tx_n = 0;
    bpvm_wire_cables_soltar();
    close(fd);
    s_roto = 0;
    s_rx_ini = s_rx_fin = 0;
    s_rx_ms = -1;
    bpvm_wifi_conexion(0);
    log_printf("wire: el cliente de la red se ha ido");
    return 1;
}

static void opcion(int fd, int nivel, int cual, int valor) { setsockopt(fd, nivel, cual, &valor, sizeof valor); }

/* Un cliente nuevo, si lo hay. 1 si ha sustituido a otro. */
static int acepta(void) {
    struct sockaddr_in de;
    socklen_t largo = sizeof de;
    struct timeval tv = { .tv_sec = ENVIO_ATASCADO_S, .tv_usec = 0 };
    int ido, fd = accept(s_escucha, (struct sockaddr*) &de, &largo);
    if (fd < 0) return 0;
    ido = cierra_cliente();
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
    opcion(fd, IPPROTO_TCP, TCP_NODELAY, 1);        /* muchos mensajes cortos: sin esperar a Nagle */
    opcion(fd, SOL_SOCKET, SO_KEEPALIVE, 1);
    opcion(fd, IPPROTO_TCP, TCP_KEEPIDLE, VIVO_INACTIVO_S);
    opcion(fd, IPPROTO_TCP, TCP_KEEPINTVL, VIVO_INTERVALO_S);
    opcion(fd, IPPROTO_TCP, TCP_KEEPCNT, VIVO_INTENTOS);
    bpvm_wire_cables_bloquear();
    s_cliente = fd;
    bpvm_wire_cables_soltar();
    bpvm_wifi_conexion(1);
    log_printf("wire: cliente de la red, %s:%d", inet_ntoa(de.sin_addr), ntohs(de.sin_port));
    return ido;
}

/* ── Buscar ────────────────────────────────────────────────────────────────────────────────────────────────────────── */

static void contesta_busqueda(void) {
    char pide[96], resp[224], ip[20], micro[24], id[24];
    struct sockaddr_in de;
    socklen_t largo = sizeof de;
    int off, n = recvfrom(s_busca, pide, sizeof pide - 1, MSG_DONTWAIT, (struct sockaddr*) &de, &largo);
    if (n <= 0) return;
    pide[n] = 0;
    if (!strstr(pide, "\"FIND\"") || !bpvm_red_ip(ip, sizeof ip)) return;
    micro[0] = id[0] = 0;
    bpvm_machine_micro_name(micro, sizeof micro);
    bpvm_machine_unique_id(id, sizeof id);
    off = wire_v1_msg_begin_event(resp, sizeof resp, 0, "FIND_REPLY");
    if (off >= 0) off = wire_v1_field_string(resp, sizeof resp, (size_t) off, "name", bpvm_red_nombre());
    if (off >= 0) off = wire_v1_field_string(resp, sizeof resp, (size_t) off, "micro", micro);
    if (off >= 0) off = wire_v1_field_string(resp, sizeof resp, (size_t) off, "uniqueId", id);
    if (off >= 0) off = wire_v1_field_string(resp, sizeof resp, (size_t) off, "ip", ip);
    if (off >= 0) off = wire_v1_field_long(resp, sizeof resp, (size_t) off, "port", WIRE_RED_PUERTO);
    if (off >= 0) off = wire_v1_msg_end(resp, sizeof resp, (size_t) off);
    if (off < 0) return;
    sendto(s_busca, resp, (size_t) off, 0, (struct sockaddr*) &de, largo);
    bpvm_wifi_actividad();
}

/* ── El puerto ─────────────────────────────────────────────────────────────────────────────────────────────────────── */

static void cierra_puerto(void) {
    if (s_escucha >= 0) { close(s_escucha); s_escucha = -1; }
    if (s_busca >= 0)   { close(s_busca);   s_busca = -1; }
}

static int sin_bloquear(int fd) { return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK); }

static void abre_puerto(void) {
    struct sockaddr_in yo;
    memset(&yo, 0, sizeof yo);
    yo.sin_family = AF_INET;
    yo.sin_addr.s_addr = htonl(INADDR_ANY);
    yo.sin_port = htons(WIRE_RED_PUERTO);
    s_escucha = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    s_busca   = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_escucha >= 0 && s_busca >= 0) {
        opcion(s_escucha, SOL_SOCKET, SO_REUSEADDR, 1);
        opcion(s_busca, SOL_SOCKET, SO_BROADCAST, 1);
        if (bind(s_escucha, (struct sockaddr*) &yo, sizeof yo) == 0 && listen(s_escucha, 2) == 0
            && bind(s_busca, (struct sockaddr*) &yo, sizeof yo) == 0
            && sin_bloquear(s_escucha) == 0 && sin_bloquear(s_busca) == 0) {
            log_printf("wire: la red escucha en el puerto %d", WIRE_RED_PUERTO);
            return;
        }
    }
    log_printf("wire: el puerto %d no se pudo abrir (errno %d): se reintenta", WIRE_RED_PUERTO, errno);
    cierra_puerto();
    s_abrir_en = bpvm_platform_now_ms() + REINTENTO_MS;
}

/* ── El sondeo ─────────────────────────────────────────────────────────────────────────────────────────────────────── */

static int sondeo(void) {
    int ido = 0;
    int64_t ahora;
    if (s_roto) ido = cierra_cliente();
    ahora = bpvm_platform_now_ms();
    if (ahora - s_mirado < MIRAR_CADA_MS) return ido ? BPVM_WIRE_CLIENTE_IDO : BPVM_WIRE_SIGUE;
    s_mirado = ahora;
    if (s_hay_ip && s_escucha < 0 && ahora >= s_abrir_en) abre_puerto();
    if (!s_hay_ip && s_escucha >= 0) {
        ido |= cierra_cliente();
        cierra_puerto();
        log_printf("wire: la red ya no escucha");
    }
    if (s_escucha >= 0) {
        ido |= acepta();
        contesta_busqueda();
    }
    return ido ? BPVM_WIRE_CLIENTE_IDO : BPVM_WIRE_SIGUE;
}

static const bpvm_wire_cable_t CABLE = { "la red", leer, escribir, vacia, sondeo, abierto };

void wire_red_iniciar(void) {
    bpvm_red_al_cambiar(al_cambiar_ip);
    bpvm_wire_cables_red(&CABLE);
}
