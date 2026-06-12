/*
 * bpvm_net.h — fachada de red para los builtins Net.* (H11, #241).
 *
 * Mismo patrón que bpvm_gpio.h / la fachada fs: la VM core solo conoce
 * esta tabla; cada plataforma registra su backend al boot:
 *
 *   - Host (Windows/Linux/macOS): src/net_host.c — sockets reales
 *     (Winsock2 con WSAStartup lazy en Windows, POSIX en el resto).
 *     test/main.c llama a bpvm_net_register_host().
 *   - Firmwares: SIN backend por ahora (H11.b/c traerán lwIP en
 *     Pico 2 W / ESP32-S3). Backend NULL → los builtins lanzan
 *     RuntimeError ATRAPABLE ("sin red en esta plataforma") — patrón
 *     honesto, no un fallo silencioso. STM32 U575: NULL siempre.
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
 */
#ifndef BPVM_NET_H
#define BPVM_NET_H

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

/* Solo host (net_host.c): registra el backend de sockets del SO. */
void bpvm_net_register_host(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_NET_H */
