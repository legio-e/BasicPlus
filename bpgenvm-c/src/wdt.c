/*
 * wdt.c — fachada Wdt para la VM C.
 *
 * #480 — CRITERIO DE EDUARDO (9-sep-2026): «el watchdog se ha de implementar
 * COMPLETO o no se implementa. Lo del STM32 o se arregla o se desactiva
 * completamente: no podemos tener medio watchdog.»
 *
 * Por eso esta fachada ya no tiene stub. Antes, sin backend, escribia
 * "[wdt] enable(N ms) (stub, no-op)" y seguia — o sea que el programa se creia
 * protegido y no lo estaba. Ahora la ausencia de backend es un ERROR que el
 * builtin convierte en excepcion BP atrapable, igual que el NeoPixel.
 *
 * Y de paso se cierra una rotura del invariante que nadie habia visto: los dos
 * textos de "no-op" no eran iguales (miVM decia "(host, no-op)" y esta VM
 * "(stub, no-op)"), asi que cualquier programa que tocara Wdt en el PC daba
 * stdout distinto en las dos VMs. No salto nunca porque ningun sample de Wdt
 * estaba en el corpus de paridad; ahora hay uno (samples/WdtCatch.bp).
 *
 * Devuelven 0 si se hizo, -1 si esta plataforma no tiene watchdog.
 */

#include "bpvm_wdt.h"
#include <stddef.h>

static const bpvm_wdt_backend_t* g_backend = NULL;

void bpvm_wdt_set_backend(const bpvm_wdt_backend_t* backend) {
    g_backend = backend;
}

int bpvm_wdt_enable(int timeoutMs) {
    if (g_backend && g_backend->enable) { g_backend->enable(timeoutMs); return 0; }
    return -1;
}

int bpvm_wdt_feed(void) {
    if (g_backend && g_backend->feed) { g_backend->feed(); return 0; }
    return -1;
}

int bpvm_wdt_disable(void) {
    if (g_backend && g_backend->disable) { g_backend->disable(); return 0; }
    return -1;
}
