/*
 * log_esp32.c — CINTURA del núcleo portable bpvm_log (src/bpvm_log.c) para la
 * familia ESP32 (S3 + P4). Solo lo NO portable: timestamp (esp_timer) + leer y
 * escribir la región del log en flash (esp_partition). La lógica (buffer,
 * formato, snapshot, dump, recuperación post-mortem) la pone el núcleo → mismo
 * comportamiento que Pico y STM32. Espejo de stm32/port/log.c.
 *
 * ── DÓNDE VIVE LA REGIÓN (la decisión de diseño de este fichero) ──
 * Dentro de la partición "bpenv" que YA EXISTE, a partir del sector 2.
 *
 * El env usa exactamente DOS sectores de bpenv (slot A en 0, slot B en 4096;
 * ver board_mgr_esp32.c), pero la partición está declarada de 32 KB = 8
 * sectores en las tres tablas (S3, P4 16M y P4 32M) ⇒ hay 24 KB ociosos ahí
 * desde siempre. El log se queda con dos de esos sectores libres.
 *
 * Por qué NO una partición "bplog" propia, que sería lo obvio: en las tablas
 * actuales bpdata empieza justo donde acaba bpenv, así que meter una partición
 * nueva DESPLAZA bpdata → littlefs se encontraría su volumen en otro sitio y el
 * FS de la placa se perdería. Un log de diagnóstico no vale el precio de
 * borrarle los datos a nadie. Además así el firmware nuevo funciona sobre las
 * placas YA flasheadas sin tocar la tabla.
 *
 * Encaja con el modelo H9: bpenv es la zona de metadatos DE PLACA (sobrevive al
 * reflasheo del firmware, no es del sistema de ficheros de la aplicación), y un
 * post-mortem es exactamente eso. Los 16 KB restantes quedan de reserva.
 */
#include "log.h"

#include "esp_partition.h"
#include "esp_timer.h"

#include <stdint.h>
#include <string.h>

/* Geometría dentro de bpenv. Debe cuadrar con BP_ENV_SECTOR de
 * board_mgr_esp32.c: los sectores 0 y 1 son del env y NO se tocan aquí. */
#define BP_ENV_SECTOR_SIZE   4096u
#define BP_LOG_SLOT          2u                       /* 1er sector libre de bpenv */
#define BP_LOG_OFFSET        (BP_LOG_SLOT * BP_ENV_SECTOR_SIZE)
#define BP_LOG_SIZE          (2u * BP_ENV_SECTOR_SIZE)  /* 8 KB, como el STM32 */

/* La región del log EN RAM (= imagen de flash: header + data). El núcleo escribe
 * aquí y el flush vuelca este mismo buffer, sin copia intermedia. Va en RAM
 * interna a propósito: el log tiene que estar vivo ANTES que la PSRAM. */
static uint8_t s_region[BP_LOG_SIZE];

static const esp_partition_t* s_part = NULL;

static uint32_t now_ms(void) { return (uint32_t) (esp_timer_get_time() / 1000); }

/* Red anti-desastre (misma filosofía que layout_ok del STM32): si no hay
 * partición o la región no cabe dentro, NO tocamos flash — devolvemos error y el
 * núcleo se queda con el log solo-RAM, que sigue siendo útil en caliente. */
static int part_ok(void) {
    return s_part != NULL && (BP_LOG_OFFSET + BP_LOG_SIZE) <= (uint32_t) s_part->size;
}

static int flash_read(uint8_t* dst, uint32_t len) {
    if (!part_ok()) return -1;
    return esp_partition_read(s_part, BP_LOG_OFFSET, dst, len) == ESP_OK ? 0 : -1;
}

static int flash_write(const uint8_t* src, uint32_t len) {
    if (!part_ok()) return -1;
    /* esp_partition_* son seguras para multitarea y no suspenden el XIP como el
     * RP2350 → sin gimnasia de IRQs. Borrado y escritura de la región entera. */
    if (esp_partition_erase_range(s_part, BP_LOG_OFFSET, BP_LOG_SIZE) != ESP_OK) return -1;
    return esp_partition_write(s_part, BP_LOG_OFFSET, src, len) == ESP_OK ? 0 : -1;
}

void log_init(void) {
    /* Búsqueda propia (no la de board_mgr): el log arranca ANTES del climb del
     * boot, que es justo cuando interesa que ya esté grabando. */
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                      ESP_PARTITION_SUBTYPE_ANY, "bpenv");

    bpvm_log_cintura_t c;
    c.now_ms      = now_ms;
    c.flash_read  = flash_read;
    c.flash_write = flash_write;
    c.region_buf  = s_region;
    c.region_size = BP_LOG_SIZE;
    bpvm_log_init(&c);
}
