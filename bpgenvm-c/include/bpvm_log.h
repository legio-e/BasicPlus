/*
 * bpvm_log.h — núcleo PORTABLE del log de diagnóstico (post-mortem).
 *
 * Unifica los logs de firmware (Pico/STM32/ESP32) en UNA implementación + una
 * cintura fina por micro — misma jugada que el FS (bpvm_fs) y el kernel H9
 * (bpvm_env/part/boot). "Hecho para uno, sirve para todos": la ESP32, que hoy no
 * tiene log, lo hereda con solo aportar su cintura.
 *
 * La LÓGICA (buffer RAM append-only + timestamp + snapshot con header
 * magic/version/size + dump por chunks + recuperación al arrancar) vive aquí y es
 * idéntica en las 3 familias. Lo ÚNICO no portable = la cintura: timestamp +
 * leer/escribir el sector de flash del log.
 *
 * Modelo de memoria (clave): el buffer RAM que aporta el llamador ES la imagen de
 * flash — [header 16 B][data...]. Así el flush vuelca ese mismo buffer sin copia
 * ni scratch aparte (el Pico/STM32 se ahorran su antiguo flash_buf de 4/8 KB).
 *
 * API de llamante IDÉNTICA a la de siempre (log_printf/flush/dump/clear/stats) →
 * el código que loguea no cambia entre familias.
 */
#ifndef BPVM_LOG_H
#define BPVM_LOG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Cintura de plataforma: lo único que cada micro debe aportar. */
typedef struct {
    /* ms desde el arranque (Pico: xTaskGetTickCount·tick; STM32: HAL_GetTick;
     * ESP32: esp_timer/xTaskGetTickCount). NULL → timestamp 0. */
    uint32_t (*now_ms)(void);

    /* Lee la región del log (region_size bytes) del flash a `dst`. 0 OK, !=0 error
     * (→ el núcleo arranca con log vacío). Pico: memcpy XIP; STM32: memcpy
     * FLASH_BASE+off; ESP32: esp_partition_read. */
    int (*flash_read)(uint8_t* dst, uint32_t len);

    /* Borra + escribe la región ENTERA (len == region_size, alineado a página de
     * borrado por construcción). 0 OK. Pico: erase+program bajo flash_lock; STM32:
     * stm32_flash_erase+write; ESP32: esp_partition_erase_range+write. */
    int (*flash_write)(const uint8_t* src, uint32_t len);

    /* RAM del llamador de tamaño `region_size`. ES la imagen de flash del log
     * (header+data). Persiste toda la vida del programa. */
    uint8_t* region_buf;

    /* Tamaño del sector de log en flash (múltiplo de la página de borrado del
     * micro: Pico 4 KB, STM32 8 KB, ESP32 4 KB). */
    uint32_t region_size;
} bpvm_log_cintura_t;

/* Instala la cintura y recupera el snapshot de la sesión anterior (si el header es
 * válido) → post-mortem. Llamar UNA vez antes del primer log_printf. */
void bpvm_log_init(const bpvm_log_cintura_t* cintura);

/* Añade una línea (auto-prefija "[ms] " + newline). No bloquea, no toca flash. */
void log_printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/*
 * #423 — EL INTERRUPTOR DEL LOG. Decisión de Eduardo (15-ago): *«una variable
 * de entorno log=0 o 1. El log de arranque hasta que llegue a las variables de
 * entorno va fijo, los siguientes ya depende del valor. Así solamente
 * registramos cuando queramos depurar»*.
 *
 * ─── EL PROBLEMA QUE RESUELVE ───
 *
 * La región son 8 KB (4 en el S3) y el GC escribe TRES líneas por colecta
 * —unos 300 B—, así que **~26 colectas y el log está lleno**. Y `append_raw`
 * es append-only: al llenarse deja de escribir y pone `[LOG OVERFLOW]`. O sea
 * que el log de una placa que se cuelga contenía el arranque y las primeras
 * colectas, y NO el momento del cuelgue — lo contrario de para lo que existe un
 * post-mortem. Se destapó en la P4 el 15-ago, en cuanto #420 conectó por fin el
 * diagnóstico de la VM al log de esa familia.
 *
 * ─── EL CONTRATO ───
 *
 * Arranca ENCENDIDO, y eso no es un detalle: lo que pase antes de que nadie
 * llame aquí queda registrado siempre. Si la placa se muere antes de leer su
 * entorno —que es cuando menos se puede preguntar nada—, el rastro está.
 *
 * Apagado NO borra: lo ya escrito se conserva y se sigue pudiendo volcar. Sólo
 * deja de añadir.
 *
 * `log_flush` sigue funcionando apagado, a propósito: apagar el log es decidir
 * qué se GUARDA, no impedir persistir lo que ya se guardó.
 */
void bpvm_log_set_enabled(int on);
int  bpvm_log_enabled(void);

/* Persiste el buffer al sector de flash (momentos críticos: boot, antes de RESET). */
void log_flush(void);

/* Vacía el buffer RAM; clear_flash además persiste un log vacío. */
void log_clear_ram(void);
void log_clear_flash(void);

/* Vuelca el log (RAM) a un sink, en chunks de 256 B. */
typedef void (*log_sink_t)(const char* data, size_t len, void* user);
void log_dump(log_sink_t cb, void* user);

/* Stats. */
uint32_t log_used_bytes(void);
uint32_t log_total_bytes(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_LOG_H */
