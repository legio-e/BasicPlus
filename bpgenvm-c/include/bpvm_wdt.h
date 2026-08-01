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
 *                      backend Pico sustituye 0/negativo por 100 ms
 *                      (el mínimo de verdad lo impone la clase BP
 *                      Wdt.Timer, MIN_TIMEOUT_MS=100), y recorta por
 *                      arriba al máximo del HW: en RP2350 el contador
 *                      es de 24 bits contando µs, así que el techo
 *                      son ~16777 ms. Pedir más NO da más plazo.
 *   feed():            resetea el contador del watchdog.
 *   disable():         para el watchdog. En RP2350 SÍ se puede: el
 *                      SDK expone watchdog_disable(), que limpia
 *                      WATCHDOG_CTRL_ENABLE_BITS y detiene el
 *                      contador sin rebotar.
 *                      OJO al implementar un backend nuevo: NO vale
 *                      "aproximar" con un timeout enorme. No existe
 *                      un valor que no dispare — el SDK recorta el
 *                      load a WATCHDOG_LOAD_BITS, así que ese apaño
 *                      deja el perro ARMADO (~16,8 s) y la placa se
 *                      resetea sola. Fue un bug real aquí.
 *                      Donde el HW no deje pararlo (STM32: el IWDG
 *                      no se para hasta el reset), el backend hace
 *                      lo mejor posible y lo DICE en su comentario;
 *                      no se finge que quede desactivado.
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
