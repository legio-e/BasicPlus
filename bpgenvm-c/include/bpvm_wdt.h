/*
 * bpvm_wdt.h — hooks de plataforma para los builtins Wdt.*
 *
 * Watchdog timer del RP2350. Si no se "alimenta" (feed) dentro
 * del timeout configurado, el chip se resetea automáticamente.
 *
 * Casos de uso típicos:
 *   - Recovery de cuelgues en deployments unattended.
 *   - Detección de loops infinitos en código que debería ser
 *     responsive.
 *
 * Convenciones:
 *   enable(timeoutMs): activa el watchdog con timeout en ms. Si
 *                      timeoutMs es muy bajo (<10ms en RP2350 con
 *                      configs por defecto), puede ser inalcanzable
 *                      en práctica — la VM y el SDK ya queman
 *                      cycles antes de que feed() llegue. El
 *                      backend Pico aplica un mínimo de ~50 ms.
 *   feed():            resetea el contador del watchdog.
 *   disable():         desactiva. En RP2350 el watchdog no se
 *                      puede deshabilitar después de activado (es
 *                      un one-way switch del SDK) — el "disable"
 *                      lo aproximamos seteando un timeout enorme
 *                      (8.4M ms ≈ 2h) que nunca se va a alcanzar
 *                      en uso normal.
 *
 * El watchdog es un SINGLETON del MCU: solo hay uno. Construir
 * varias instancias de Wdt.Timer en BP es legal pero todas
 * operan sobre el mismo HW.
 *
 * AVISO de seguridad: si activas el watchdog con un timeout
 * corto y tu código se cuelga, el chip se resetea en bucle.
 * Solo se rompe el bucle reflasheando un firmware que NO active
 * el watchdog al boot, o entrando en BOOTSEL al arrancar.
 */
#ifndef BPVM_WDT_H
#define BPVM_WDT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*enable)(int timeoutMs);
    void (*feed)(void);
    void (*disable)(void);
} bpvm_wdt_backend_t;

void bpvm_wdt_set_backend(const bpvm_wdt_backend_t* backend);

void bpvm_wdt_enable(int timeoutMs);
void bpvm_wdt_feed(void);
void bpvm_wdt_disable(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_WDT_H */
