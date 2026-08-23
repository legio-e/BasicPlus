/*
 * log.c (Pico/RP2350) — CINTURA del núcleo portable bpvm_log (src/bpvm_log.c).
 *
 * Aquí sólo vive lo que NO es portable del RP2350: el reloj (FreeRTOS), leer la
 * región del log por XIP, y escribirla con erase+program bajo `flash_lock`. La
 * lógica —buffer, anillo, formato, snapshot, volcado, el interruptor `log=`— la
 * pone el núcleo, así que las cuatro imágenes se comportan igual.
 *
 * ─── POR QUÉ ESTE FICHERO ENCOGIÓ DE 280 A ~90 LÍNEAS (V6/U1.2) ───
 *
 * La Pico era la ÚNICA familia que no usaba el núcleo común: llevaba su propia
 * copia de las 15 funciones y su `CMakeLists` ni siquiera nombraba
 * `src/bpvm_log.c`. El ESP32 y el STM32 ya sólo aportaban su cintura. Lo dejó
 * dicho el propio código que había aquí: *«el ESP32 ya migró a él; la Pico se
 * quedó atrás… la migración completa es otra tanda»*. Ésta es esa tanda.
 *
 * Lo que NO se pierde al migrar, porque el núcleo ya lo hace igual:
 *   · #338 — el buffer de RAM ES la imagen de flash (cabecera + datos), sin
 *     segundo buffer ni copia del log entero en cada volcado.
 *   · #433 — ANILLO por líneas: al llenarse se tiran las ANTIGUAS y se conserva
 *     la COLA, que es lo que sirve en un post-mortem, y el volcado lo DICE.
 *   · #439 — la región vive en RAM que el arranque no borra, y al reiniciar
 *     MANDA sobre el flash porque es más reciente. El núcleo lo comprueba con el
 *     mismo criterio (magic + version + size) y su propio comentario nombra este
 *     `__uninitialized_ram`.
 *   · #423 — el interruptor `log=0|1`, que `main.c` aplica tras leer el entorno.
 *
 * ⚠️ UN CAMBIO DE COMPORTAMIENTO, a propósito: el `log_clear_flash` de aquí
 * borraba SÓLO la flash y dejaba la RAM con el log dentro, así que el siguiente
 * volcado lo devolvía a flash. El del núcleo vacía las dos. Si vacías el log,
 * esperas que se vaya.
 */
#include "log.h"

#include "bpvm_log.h"       /* el núcleo portable + la struct de cintura */
#include "flash_lock.h"     /* #153 — ventana XIP-safe (dual-core safe) */
#include "flash_layout.h"   /* H9: el log vive en la ZONA 2 (kernel) */

#include "FreeRTOS.h"
#include "task.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#include <stdint.h>
#include <string.h>

/* H9: sector del log = zona 2 (0x012000). Antes vivía en 0x3FC000, calculado
 * contra el layout del FS LEGADO; con la unificación de particiones el espacio
 * [BP_PART_BASE, usable) es TODO de las particiones y el log se muda al hueco
 * del kernel — que el UF2 no graba, así que el log sobrevive al reflasheo. */
#define LOG_REGION_BYTES   FLASH_SECTOR_SIZE     /* 4 KB */
#define LOG_FLASH_OFFSET   BP_LOG_OFFSET

/* #439 — LA REGIÓN VIVE EN RAM QUE SOBREVIVE AL RESET.
 *
 * `__uninitialized_ram` la pone en `.uninitialized_data`, que el crt0 NO toca
 * (la sección es NOLOAD en el linker script del SDK). La idea es de Eduardo
 * —«había una zona de RAM que se mantenía, igual se puede utilizar de pequeña
 * caché para no tener que grabar todo cada vez en la flash»— y es mucho mejor
 * que escribir a flash por línea: cero desgaste y cero coste.
 *
 * Y no es un truco de la casa: el propio SDK lo usa igual para detectar el doble
 * reset (`pico_bootsel_via_double_reset`), token mágico incluido. Aquí el token
 * ya lo tenemos, porque la CABECERA vive dentro de la región.
 *
 * ⚠️ Sobrevive al RESET, no al corte de alimentación. Por eso el volcado a flash
 * sigue existiendo: es la red para cuando se va la luz. */
static uint8_t __uninitialized_ram(bplog_region)[LOG_REGION_BYTES]
    __attribute__((aligned(4)));

static uint32_t now_ms(void) {
    return (uint32_t) (xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/* Lectura por XIP: la flash está mapeada, así que es un memcpy y no hace falta
 * tomar el candado (no se toca el controlador). */
static int flash_read(uint8_t* dst, uint32_t len) {
    memcpy(dst, (const void*) (uintptr_t) (XIP_BASE + LOG_FLASH_OFFSET), len);
    return 0;
}

/* Erase + program de la región ENTERA. El candado apaga IRQs y para al otro
 * núcleo: durante la operación nadie puede acceder a XIP, y ejecutar desde
 * flash mientras se borra cuelga la placa. */
static int flash_write(const uint8_t* src, uint32_t len) {
    uint32_t saved = bpvm_flash_lock_begin();
    flash_range_erase(LOG_FLASH_OFFSET, len);
    flash_range_program(LOG_FLASH_OFFSET, src, len);
    bpvm_flash_lock_end(saved);
    return 0;
}

void log_init(void) {
    bpvm_log_cintura_t c;
    c.now_ms      = now_ms;
    c.flash_read  = flash_read;
    c.flash_write = flash_write;
    c.region_buf  = bplog_region;
    c.region_size = LOG_REGION_BYTES;
    bpvm_log_init(&c);
}
