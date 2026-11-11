/*
 * bt_esp32.h — V8/H2, paso 5 (11-nov): la radio Bluetooth de la familia ESP (NimBLE como anfitrión), publicada como
 * 'BT  '. El contrato, en bpvm_bt.h; el porqué, en bt_esp32.c.
 */
#ifndef BT_ESP32_H
#define BT_ESP32_H

#include <stddef.h>

/* EL ENLACE con el controlador, si no está en el chip (como el de la WiFi, wifi_esp32.h): la P4 llega al suyo por su C6
 * (radio_p4.c). `abrir`, antes de arrancar NimBLE: el enlace y el controlador encendido y unido a la pila (0, o -1 con el
 * motivo); `cerrar`, después de pararlo. */
typedef struct {
    int  (*abrir)(char* err, size_t n);
    void (*cerrar)(void);
} bt_esp32_enlace_t;

/* Publica la tabla 'BT  ' (no enciende nada: eso lo pide lo común, bpvm_bt.c). `enlace`: NULL con el controlador del
 * chip (la S3, la C5). */
void bt_esp32_publicar(const bt_esp32_enlace_t* enlace);

#endif /* BT_ESP32_H */
