/*
 * wdt.c — fachada Wdt para la VM C.
 *
 * Stub portable: en host sin backend, loguea por stdout y no
 * hace nada (no hay chip que resetear). En Pico, delega.
 */

#include "bpvm_wdt.h"
#include <stdio.h>
#include <stddef.h>

static const bpvm_wdt_backend_t* g_backend = NULL;

void bpvm_wdt_set_backend(const bpvm_wdt_backend_t* backend) {
    g_backend = backend;
}

void bpvm_wdt_enable(int timeoutMs) {
    if (g_backend && g_backend->enable) {
        g_backend->enable(timeoutMs);
        return;
    }
    printf("[wdt] enable(%d ms) (stub, no-op)\n", timeoutMs);
}

void bpvm_wdt_feed(void) {
    if (g_backend && g_backend->feed) {
        g_backend->feed();
        return;
    }
    /* No print en feed — se llama demasiado a menudo. */
}

void bpvm_wdt_disable(void) {
    if (g_backend && g_backend->disable) {
        g_backend->disable();
        return;
    }
    printf("[wdt] disable (stub, no-op)\n");
}
