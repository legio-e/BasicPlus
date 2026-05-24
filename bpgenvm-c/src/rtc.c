/*
 * rtc.c — fachada de hooks Rtc para la VM C.
 *
 * Stub portable: mantiene un offset entre epoch y now monotonic.
 * Si hay backend registrado (Pico), delega en él. Funcionalmente
 * equivalente al stub porque la implementación en main.c usa la
 * misma lógica (to_ms_since_boot + offset).
 */

#include "bpvm_rtc.h"
#include "bpvm_platform.h"
#include <stddef.h>

static const bpvm_rtc_backend_t* g_backend = NULL;

/* Offset entre epoch deseado y now_ms monotonic. Inicializado a 0,
 * lo que significa "sin calibrar" (nowMs devuelve los ms monotonic
 * crudos). */
static int64_t g_epoch_offset_ms = 0;

void bpvm_rtc_set_backend(const bpvm_rtc_backend_t* backend) {
    g_backend = backend;
}

int64_t bpvm_rtc_now_ms(void) {
    if (g_backend && g_backend->nowMs) {
        return g_backend->nowMs();
    }
    return bpvm_platform_now_ms() + g_epoch_offset_ms;
}

void bpvm_rtc_set_now_ms(int64_t epoch_ms) {
    if (g_backend && g_backend->setNowMs) {
        g_backend->setNowMs(epoch_ms);
        return;
    }
    /* offset := epoch - now_monotonic → al sumar en nowMs(), recuperamos epoch. */
    g_epoch_offset_ms = epoch_ms - bpvm_platform_now_ms();
}
