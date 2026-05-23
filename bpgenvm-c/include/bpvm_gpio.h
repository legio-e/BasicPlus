/*
 * bpvm_gpio.h — hooks de plataforma para los builtins Gpio.*
 *
 * La VM C tiene los handlers de los builtins GPIO_INIT/PULL/WRITE/READ
 * en src/builtins.c. Esos handlers se limitan a delegar al backend
 * registrado vía esta tabla:
 *
 *   - En host (Linux/macOS/MinGW) NO hay GPIO real. Por defecto la
 *     VM trae stubs que solo loggean al stdout — mismo comportamiento
 *     que la VM Java.
 *   - En el firmware Pico, el `main.c` llama a `bpvm_gpio_set_backend`
 *     con una tabla cuyos punteros van a `gpio_init`, `gpio_put`, etc.
 *     del Pico SDK.
 *
 * La VM core no depende de ninguna implementación concreta: solo
 * llama a `g_gpio_backend.xxx(...)` y si los punteros son NULL, hace
 * fallback al stub.
 */
#ifndef BPVM_GPIO_H
#define BPVM_GPIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tabla de funciones que cada plataforma puede rellenar. Toda función
 * puede ser NULL: en ese caso la VM aplica un stub (logging por stdout). */
typedef struct {
    void (*init)(int pin, int mode);           /* mode: 0=INPUT, 1=OUTPUT */
    void (*pull)(int pin, int pull_mode);      /* pull: 0=none, 1=up, 2=down */
    void (*write)(int pin, int value);         /* value: 0=LOW, !=0=HIGH */
    int  (*read)(int pin);                     /* devuelve 0 o 1 */
} bpvm_gpio_backend_t;

/* Cada plataforma (main de Pico, main de host) llama una vez al boot. */
void bpvm_gpio_set_backend(const bpvm_gpio_backend_t* backend);

/* Funciones efectivas que llama la VM. Si backend==NULL o el slot
 * concreto es NULL, hacen fallback a stub con logging. */
void bpvm_gpio_init(int pin, int mode);
void bpvm_gpio_pull(int pin, int pull_mode);
void bpvm_gpio_write(int pin, int value);
int  bpvm_gpio_read(int pin);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_GPIO_H */
