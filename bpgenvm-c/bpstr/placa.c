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

#include <string.h>

#if defined(ESP_PLATFORM)
#include "sdkconfig.h"
#include "esp_heap_caps.h"
#endif

int bpstr_prueba_main(int larga, const char* ruta_kuhn);   /* prueba.c */
int bpstr_banco_main(void);                                /* banco.c */
void bpstr_en_placa(void);

#define SALIDA "/app/bpstr.txt"
#define KUHN   "/app/UTF-8-test.txt"

void bpstr_banco_linea(const char* s) {
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

void bpstr_en_placa(void) {
    int r;
    (void) bpvm_fs_remove(SALIDA);
    log_printf("bpstr: las pruebas y el banco, a %s", SALIDA);
    r = bpstr_prueba_main(0, KUHN);
    (void) bpstr_banco_main();
    log_printf("bpstr: hecho (las pruebas, %s)", r ? "ROJO" : "VERDE");
}
