/*
 * net_host.c — backend de red del HOST para la fachada bpvm_net (H11).
 *
 * Sockets TCP de cliente sobre Winsock2 (Windows, WSAStartup lazy) o
 * POSIX (Linux/macOS). Tabla de handles propia: BP ve ints pequeños
 * (1..BPNET_MAX), nunca el SOCKET/fd del SO (en Win64 SOCKET es
 * UINT_PTR y no cabe seguro en el i32 de BP).
 *
 * Decisiones (paridad con la VM-Java, que usa java.net.Socket):
 *   - connect con timeout REAL: socket no-bloqueante + select(), luego
 *     se devuelve a bloqueante. Resolución por getaddrinfo (IPv4/6).
 *   - recv con timeout por llamada via SO_RCVTIMEO. 0 bytes leídos con
 *     el socket vivo = TIMEOUT (devuelve 0); recv()==0 del SO = el peer
 *     cerró (FIN) → BPVM_NET_CLOSED.
 *   - send reintenta hasta colocar TODO el buffer (o error → -1).
 *   - La tabla se protege con un mutex pthread: los workers BP son
 *     threads reales en host y dos threads pueden abrir/cerrar a la vez.
 */
#include "bpvm_net.h"

#include <string.h>
#include <stdio.h>
#include <pthread.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET bpnet_os_sock_t;
  #define BPNET_INVALID  INVALID_SOCKET
  #define bpnet_close_os closesocket
#else
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <sys/time.h>
  typedef int bpnet_os_sock_t;
  #define BPNET_INVALID  (-1)
  #define bpnet_close_os close
#endif

#define BPNET_MAX 16

static bpnet_os_sock_t s_socks[BPNET_MAX];   /* slot i → handle i+1 */
static int             s_used[BPNET_MAX];
static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static int             s_wsa_ready = 0;

static void ensure_wsa(void) {
#ifdef _WIN32
    if (!s_wsa_ready) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) s_wsa_ready = 1;
    }
#else
    s_wsa_ready = 1;
#endif
}

static void set_nonblocking(bpnet_os_sock_t s, int nb) {
#ifdef _WIN32
    u_long mode = nb ? 1u : 0u;
    ioctlsocket(s, FIONBIO, &mode);
#else
    int fl = fcntl(s, F_GETFL, 0);
    if (nb) fcntl(s, F_SETFL, fl | O_NONBLOCK);
    else    fcntl(s, F_SETFL, fl & ~O_NONBLOCK);
#endif
}

static int slot_alloc(bpnet_os_sock_t s) {
    pthread_mutex_lock(&s_lock);
    for (int i = 0; i < BPNET_MAX; i++) {
        if (!s_used[i]) {
            s_used[i]  = 1;
            s_socks[i] = s;
            pthread_mutex_unlock(&s_lock);
            return i + 1;
        }
    }
    pthread_mutex_unlock(&s_lock);
    return 0;   /* tabla llena */
}

static bpnet_os_sock_t slot_get(int h) {
    if (h < 1 || h > BPNET_MAX) return BPNET_INVALID;
    pthread_mutex_lock(&s_lock);
    bpnet_os_sock_t s = s_used[h - 1] ? s_socks[h - 1] : BPNET_INVALID;
    pthread_mutex_unlock(&s_lock);
    return s;
}

static void slot_free(int h) {
    if (h < 1 || h > BPNET_MAX) return;
    pthread_mutex_lock(&s_lock);
    s_used[h - 1] = 0;
    pthread_mutex_unlock(&s_lock);
}

/* connect no-bloqueante + select con timeout. Devuelve handle o 0. */
static int host_connect(const char* host, int port, int timeout_ms) {
    ensure_wsa();
    if (timeout_ms <= 0) timeout_ms = 5000;

    char portstr[16];
    snprintf(portstr, sizeof(portstr), "%d", port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* res = NULL;
    if (getaddrinfo(host, portstr, &hints, &res) != 0 || !res) return 0;

    int handle = 0;
    for (struct addrinfo* ai = res; ai && !handle; ai = ai->ai_next) {
        bpnet_os_sock_t s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s == BPNET_INVALID) continue;

        set_nonblocking(s, 1);
        int rc = connect(s, ai->ai_addr, (int) ai->ai_addrlen);
        int in_progress;
#ifdef _WIN32
        in_progress = (rc != 0 && WSAGetLastError() == WSAEWOULDBLOCK);
#else
        in_progress = (rc != 0 && errno == EINPROGRESS);
#endif
        if (rc != 0 && !in_progress) { bpnet_close_os(s); continue; }

        if (rc != 0) {   /* esperar a que el connect resuelva */
            fd_set wf, ef;
            FD_ZERO(&wf); FD_SET(s, &wf);
            FD_ZERO(&ef); FD_SET(s, &ef);
            struct timeval tv;
            tv.tv_sec  = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
            int sel = select((int) (s + 1), NULL, &wf, &ef, &tv);
            int ok = (sel > 0) && FD_ISSET(s, &wf) && !FD_ISSET(s, &ef);
            if (ok) {   /* select writable puede ser éxito O error: verificar */
                int soerr = 0;
#ifdef _WIN32
                int slen = sizeof(soerr);
                getsockopt(s, SOL_SOCKET, SO_ERROR, (char*) &soerr, &slen);
#else
                socklen_t slen = sizeof(soerr);
                getsockopt(s, SOL_SOCKET, SO_ERROR, &soerr, &slen);
#endif
                ok = (soerr == 0);
            }
            if (!ok) { bpnet_close_os(s); continue; }
        }

        set_nonblocking(s, 0);
        handle = slot_alloc(s);
        if (!handle) bpnet_close_os(s);   /* tabla llena */
    }
    freeaddrinfo(res);
    return handle;
}

static int host_send(int h, const void* buf, int len) {
    bpnet_os_sock_t s = slot_get(h);
    if (s == BPNET_INVALID) return BPVM_NET_ERR;
    const char* p = (const char*) buf;
    int sent = 0;
    while (sent < len) {
        int n = (int) send(s, p + sent, len - sent, 0);
        if (n <= 0) return BPVM_NET_ERR;
        sent += n;
    }
    return sent;
}

static int host_recv(int h, void* buf, int max, int timeout_ms) {
    bpnet_os_sock_t s = slot_get(h);
    if (s == BPNET_INVALID) return BPVM_NET_ERR;
    if (max <= 0) return 0;
    /* timeout 0/negativo → 1 ms: SO_RCVTIMEO=0 significa "bloquear para
     * siempre" y recv SIEMPRE lleva timeout por contrato (paridad Java,
     * donde setSoTimeout(0) también es infinito y se clampa igual). */
    if (timeout_ms <= 0) timeout_ms = 1;

#ifdef _WIN32
    DWORD tv = (DWORD) timeout_ms;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof(tv));
#else
    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    int n = (int) recv(s, (char*) buf, max, 0);
    if (n > 0)  return n;
    if (n == 0) return BPVM_NET_CLOSED;        /* FIN ordenado del peer */
#ifdef _WIN32
    int err = WSAGetLastError();
    if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) return BPVM_NET_RECV_TIMEOUT;
#else
    if (errno == EAGAIN || errno == EWOULDBLOCK) return BPVM_NET_RECV_TIMEOUT;
#endif
    return BPVM_NET_ERR;
}

static void host_close(int h) {
    bpnet_os_sock_t s = slot_get(h);
    if (s != BPNET_INVALID) {
        bpnet_close_os(s);
        slot_free(h);
    }
}

static const bpvm_net_backend_t s_host_net = {
    .connect = host_connect,
    .send    = host_send,
    .recv    = host_recv,
    .close   = host_close,
};

void bpvm_net_register_host(void) {
    bpvm_net_set_backend(&s_host_net);
}
