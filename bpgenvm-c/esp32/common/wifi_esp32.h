/*
 * wifi_esp32.h — V8/H3 (6-nov): la radio de la familia ESP, publicada como 'WIFI' (bpvm_wifi.h).
 *
 * Las funciones `esp_wifi_*` son las mismas en todas: en la S3 van a su radio y en la P4 el IDF 6.1 las lleva a la C6. Lo
 * único distinto es que la P4 tiene que abrir antes el enlace con la C6, y eso se pasa aquí.
 */
#ifndef WIFI_ESP32_H
#define WIFI_ESP32_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lo que una placa con la radio en otro chip hace antes de encenderla y después de apagarla. */
typedef struct {
    int  (*abrir)(char* err, size_t n);   /* 0, o -1 con el motivo */
    void (*cerrar)(void);
} wifi_esp32_enlace_t;

/* Publica la radio y el backend de la IP. `enlace`: NULL si la radio está en el propio chip. */
void wifi_esp32_publicar(const wifi_esp32_enlace_t* enlace);

/* El enlace con la radio se ha roto, y ya no va a contestar. */
void wifi_esp32_radio_rota(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_ESP32_H */
