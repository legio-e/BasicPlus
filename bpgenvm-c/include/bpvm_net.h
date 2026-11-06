/*
 * bpvm_net.h — fachada de red para los builtins Net.* (H11, #241).
 *
 * Mismo patrón que bpvm_gpio.h / la fachada fs: la VM core solo conoce
 * esta tabla; cada plataforma registra su backend al boot:
 *
 *   - Host (Windows/Linux/macOS): src/net_host.c — sockets reales
 *     (Winsock2 con WSAStartup lazy en Windows, POSIX en el resto).
 *     test/main.c llama a bpvm_net_register_host().
 *   - Firmwares con WiFi (V8/H3: la P4, la S3 y la C5):
 *     esp32/common/net_esp32.c, sobre lwIP. Los demás, sin backend:
 *     los builtins lanzan RuntimeError ATRAPABLE ("sin red en esta
 *     plataforma") — patrón honesto, no un fallo silencioso.
 *
 * Semántica de la tabla (deliberadamente mínima — cliente simple):
 *
 *   connect(host, port, timeout_ms) → handle ≥ 1, o 0 si no se pudo
 *       conectar (rechazo, timeout, DNS). El fallo de conexión es un
 *       resultado NORMAL (la clase BP devuelve boolean), no un error.
 *   send(h, buf, len) → bytes enviados (== len: el backend reintenta
 *       hasta completar), o -1 en error (conexión rota).
 *   recv(h, buf, max, timeout_ms) → n > 0 datos; 0 = TIMEOUT (no llegó
 *       nada en la ventana); -2 = el peer CERRÓ ordenadamente (FIN);
 *       -1 = error. recv SIEMPRE lleva timeout: en device bloquearía
 *       el worker FreeRTOS entero (decisión de diseño H11).
 *   close(h) → tolerante (handle inválido = no-op).
 *
 * Los handles son ints pequeños (1..N) de una tabla DEL BACKEND — no
 * exponemos SOCKET/fd del SO (en Win64 SOCKET es UINT_PTR y no cabe
 * seguro en el i32 de BP).
 *
 * V8/H3 (6-nov) — LA IP DE LA PLACA, en la misma tabla, porque es la misma capa: lo que no es el enlace. Las usa
 * bpvm_red.c; un backend que no las tenga las deja a NULL (el PC: la IP la lleva el sistema). Un firmware puede
 * tenerlas y no tener todavía `connect`: los builtins Net.* siguen diciendo «sin red» hasta que lo tenga.
 *   ip_dinamica(nombre) → 0, o -1: por DHCP, con ese nombre en la red.
 *   ip_fija(ip, mascara, puerta, dns) → 0, o -1. Antes de conectarse.
 *   ip_leer(...) → 0, o -1: la de ahora. Las direcciones, como en bpvm_red.h.
 */
#ifndef BPVM_NET_H
#define BPVM_NET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_NET_RECV_TIMEOUT  0
#define BPVM_NET_ERR          -1
#define BPVM_NET_CLOSED       -2

typedef struct {
    int  (*connect)(const char* host, int port, int timeout_ms);
    int  (*send)(int h, const void* buf, int len);
    int  (*recv)(int h, void* buf, int max, int timeout_ms);
    void (*close)(int h);
    int  (*ip_dinamica)(const char* nombre);
    int  (*ip_fija)(uint32_t ip, uint32_t mascara, uint32_t puerta, uint32_t dns);
    int  (*ip_leer)(uint32_t* ip, uint32_t* mascara, uint32_t* puerta, uint32_t* dns);
    void (*fin_de_run)(void);   /* opcional: cierra lo que el programa dejó abierto */
} bpvm_net_backend_t;

/* Cada plataforma llama una vez al boot (o nunca: sin red). */
void bpvm_net_set_backend(const bpvm_net_backend_t* backend);

/* 1 si hay backend con connect — los builtins lo consultan para dar
 * el RuntimeError "sin red" ANTES de tocar nada. */
int bpvm_net_available(void);

/* Wrappers efectivos que llaman los builtins. Sin backend: connect→0,
 * send/recv→BPVM_NET_ERR, close→no-op (los builtins ya habrán lanzado
 * el RuntimeError vía bpvm_net_available, esto es cinturón). */
int  bpvm_net_connect(const char* host, int port, int timeout_ms);
int  bpvm_net_send(int h, const void* buf, int len);
int  bpvm_net_recv(int h, void* buf, int max, int timeout_ms);
void bpvm_net_close(int h);

/* V8/H3: el fin de RUN. */
void bpvm_net_fin_de_run(void);

/* V8/H3: sin backend, o sin ellas, -1. */
int  bpvm_net_ip_dinamica(const char* nombre);
int  bpvm_net_ip_fija(uint32_t ip, uint32_t mascara, uint32_t puerta, uint32_t dns);
int  bpvm_net_ip_leer(uint32_t* ip, uint32_t* mascara, uint32_t* puerta, uint32_t* dns);

/* Solo host (net_host.c): registra el backend de sockets del SO. */
void bpvm_net_register_host(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_NET_H */
