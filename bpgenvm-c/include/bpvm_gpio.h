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

#include <stddef.h>   /* size_t (V7/H4: bpvm_gpio_reclamar) */
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

/* ── V7/H4 (#479) — EL ESTADO DEL PIN, EN EL COMÚN ────────────────────────
 * Hasta hoy la fachada reenviaba y no guardaba nada (sólo el STM32 llevaba
 * cuenta, en privado), así que «analógico ⇒ no digital» no era comprobable.
 * Ahora el común guarda el modo de cada pin y VALIDA contra la tabla de
 * capacidades (bpvm_machine.h) ANTES de tocar el backend, con un mensaje con
 * nombre — el mismo en miVM. Modos: 0 INPUT · 1 OUTPUT · 2 OPEN_COLLECTOR ·
 * 3 ANALOG (lo pone AnPin / Adc.Channel; no llega al backend de GPIO). Usos que
 * no cambian el modo pero se comprueban: 4 PWM · 5 PULSE. */
enum { BPVM_GPIO_LIBRE = 0, BPVM_GPIO_INPUT = 1, BPVM_GPIO_OUTPUT = 2, BPVM_GPIO_OD = 3, BPVM_GPIO_ANALOG = 4 };
enum { BPVM_USO_INPUT = 0, BPVM_USO_OUTPUT = 1, BPVM_USO_OD = 2, BPVM_USO_ANALOG = 3, BPVM_USO_PWM = 4, BPVM_USO_PULSE = 5 };
int  bpvm_gpio_reclamar(int pin, int uso, char* err, size_t cap);   /* 0 = OK (y fija el modo); -1 con `err` */
/* V7/H6 — lo mismo, para un CÓDIGO NATIVO (la BIOS): el pin queda marcado DE
 * PACK, el fin de RUN no lo libera y un programa BP que lo pida recibe «lo usa
 * un código nativo». El pack vive mientras viva la placa; el programa, no. */
int  bpvm_gpio_reclamar_pack(int pin, int uso, char* err, size_t cap);
int  bpvm_gpio_estado(int pin);                                     /* BPVM_GPIO_* */
void bpvm_gpio_liberar(int pin);                                    /* release(): vuelve a LIBRE */
void bpvm_gpio_estado_reset(void);                                  /* fin de RUN */
/* ¿id está entre las instancias válidas de ese bus? 0 = OK; -1 con `err`. kind: 0 uart · 1 i2c · 2 spi */
int  bpvm_gpio_instancia_ok(int kind, int id, char* err, size_t cap);

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
