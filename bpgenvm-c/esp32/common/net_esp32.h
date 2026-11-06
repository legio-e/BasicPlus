/*
 * net_esp32.h — V8/H3 (6-nov): la IP de la familia ESP: el backend de bpvm_net.h, sobre `esp_netif` y lwIP.
 */
#ifndef NET_ESP32_H
#define NET_ESP32_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* El backend, a la fachada. */
void net_esp32_registrar(void);

/* lwIP, el bucle de eventos y la interfaz de la estación. La primera vez que se enciende la WiFi, no antes: con la WiFi
 * apagada no gastan nada. Se quedan para la vez siguiente. 0, o -1 con el motivo. */
int  net_esp32_preparar(char* err, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* NET_ESP32_H */
