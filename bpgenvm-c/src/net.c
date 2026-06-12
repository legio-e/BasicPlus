/*
 * net.c — fachada de red de la VM (H11, #241). Ver bpvm_net.h.
 *
 * La VM core (builtins.c) llama a estos wrappers; el backend concreto
 * lo registra la plataforma. Sin backend, los wrappers degradan a
 * "sin red" (connect 0 / err) — pero el camino normal es que el
 * builtin consulte bpvm_net_available() y lance RuntimeError BP
 * atrapable antes de llegar aquí.
 */
#include "bpvm_net.h"

#include <stddef.h>

static const bpvm_net_backend_t* g_net = NULL;

void bpvm_net_set_backend(const bpvm_net_backend_t* backend) {
    g_net = backend;
}

int bpvm_net_available(void) {
    return g_net != NULL && g_net->connect != NULL;
}

int bpvm_net_connect(const char* host, int port, int timeout_ms) {
    if (!g_net || !g_net->connect) return 0;
    return g_net->connect(host, port, timeout_ms);
}

int bpvm_net_send(int h, const void* buf, int len) {
    if (!g_net || !g_net->send) return BPVM_NET_ERR;
    return g_net->send(h, buf, len);
}

int bpvm_net_recv(int h, void* buf, int max, int timeout_ms) {
    if (!g_net || !g_net->recv) return BPVM_NET_ERR;
    return g_net->recv(h, buf, max, timeout_ms);
}

void bpvm_net_close(int h) {
    if (g_net && g_net->close) g_net->close(h);
}
