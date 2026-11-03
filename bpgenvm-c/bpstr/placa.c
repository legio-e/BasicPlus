/*
 * placa.c — las pruebas y el banco de bpstr EN UNA PLACA (V8 #730, el paso 2): lo que los une a una imagen de prueba de
 * la VM. Sólo entra en una imagen de prueba, compilada con BPSTR_PLACA y BPSTR_PRUEBA_PLACA y con bpstr.c, prueba.c,
 * banco.c y esto: el arranque común (bpvm_boot2.c) llama a bpstr_en_placa justo antes del autorun, con el FS montado,
 * y lo que dicen las pruebas y el banco va, línea a línea, a /app/bpstr.txt (si algo revienta a medias, queda lo de
 * antes). Se baja con `wire_serie.py PUERTO get /app/bpstr.txt local`. El catálogo de Kuhn tiene que estar antes en
 * /app/UTF-8-test.txt (`wire_serie.py PUERTO puts …`).
 *
 * Común a las familias: el reloj (bpvm_platform_now_us), los ficheros (bpvm_fs) y el log. Lo único de una familia, la
 * PSRAM de las ESP (heap_caps_malloc); en las demás, NULL.
 */
#include "bpstr.h"
#include "bpvm_fs.h"
#include "bpvm_log.h"
#include "bpvm_platform.h"

#include <stdio.h>
#include <string.h>

#if defined(ESP_PLATFORM)
#include "sdkconfig.h"
#include "esp_heap_caps.h"
#endif

/* La RP2350 de dos núcleos arma un perro guardián de 15 s antes del planificador, y lo apaga al llegar el arranque al
 * REPL (pico_arranque_completo). La prueba corre antes del REPL y tarda más: sin apagarlo, la placa se reinicia en
 * mitad y vuelve a empezar (visto el 3-nov en la Pico 2: cada 15 s). */
#if defined(__has_include)
#  if __has_include("hardware/watchdog.h")
#    include "hardware/watchdog.h"
#    define BPSTR_PERRO_PICO 1
#  endif
#endif

int bpstr_prueba_main(int larga, const char* ruta_kuhn);   /* prueba.c */
int bpstr_banco_main(void);                                /* banco.c */
void bpstr_en_placa(void);

#define SALIDA "/app/bpstr.txt"
#define KUHN   "/app/UTF-8-test.txt"

/* Cada línea, al fichero y, además, a la salida estándar: en la Pico es el USB serie, que se atiende aunque el REPL
 * todavía no esté, así que se puede ver en directo cómo va (en las ESP, su consola). */
void bpstr_banco_linea(const char* s) {
    printf("[bpstr] %s\n", s);
    fflush(stdout);
    (void) bpvm_fs_write(SALIDA, (const uint8_t*) s, (uint32_t) strlen(s), 1);
    (void) bpvm_fs_write(SALIDA, (const uint8_t*) "\n", 1u, 1);
}

uint64_t bpstr_banco_ns(void) { return (uint64_t) bpvm_platform_now_us() * 1000u; }

long bpstr_placa_lee(const char* ruta, uint8_t* dst, uint32_t cap) { return bpvm_fs_read(ruta, dst, cap); }

void* bpstr_placa_psram(size_t n) {
#if defined(ESP_PLATFORM) && defined(CONFIG_SPIRAM) && CONFIG_SPIRAM
    return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    (void) n;
    return NULL;
#endif
}

/* Aquí mismo, en la tarea del arranque (la de la VM), antes del REPL. Probado (3-nov) en una tarea suya: en la Pico
 * reventó antes de escribir la primera línea —el FS, o el log, desde una tarea que no es la de la VM—. */
void bpstr_en_placa(void) {
    int r;
#ifdef BPSTR_PERRO_PICO
    watchdog_disable();
    log_printf("bpstr: el perro del arranque, apagado (la prueba pasa de sus 15 s)");
#endif
    (void) bpvm_fs_remove(SALIDA);
    log_printf("bpstr: las pruebas y el banco, a %s", SALIDA);
    r = bpstr_prueba_main(0, KUHN);
    (void) bpstr_banco_main();
    log_printf("bpstr: hecho (las pruebas, %s)", r ? "ROJO" : "VERDE");
}
