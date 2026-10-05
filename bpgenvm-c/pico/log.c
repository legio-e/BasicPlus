/*
 * log.c (Pico/RP2350) — CINTURA del núcleo portable bpvm_log (src/bpvm_log.c).
 *
 * Aquí sólo vive lo que NO es portable del RP2350: el reloj (FreeRTOS), leer la
 * región del log por XIP, y escribirla con erase+program por la cintura de flash
 * de la familia (pico_flash.c, V7/H7 U1.4, con el candado dentro). La
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
#include "pico_flash.h"     /* V7/H7 U1.4: la cintura de flash (con el candado de #153 dentro) */
#include "flash_layout.h"   /* H9: el log vive en la ZONA 2 (kernel) */
#include "nucleos.h"        /* V7/H12, 3.1: las marcas (0x4X, el volcado) */

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

/* Erase + program de la región ENTERA, por la cintura (V7/H7 U1.4): el candado
 * —IRQs fuera y el otro núcleo parado, porque ejecutar desde flash mientras se
 * borra cuelga la placa— va dentro, en dos ventanas (borrar, programar). */
#if ( configNUMBER_OF_CORES > 1 )
static volatile int s_log_fatal = 0;   /* V7/H12 3.1, paso 4: ver log_modo_fatal */
void log_modo_fatal(void) { s_log_fatal = 1; }
#else
void log_modo_fatal(void) {}
#endif

static int flash_write(const uint8_t* src, uint32_t len) {
#if ( configNUMBER_OF_CORES > 1 )
    if (s_log_fatal) return -1;   /* tras un fallo fatal, aparcar al otro núcleo podría no volver: el log, en la RAM
                                     * (y el arranque siguiente lo vuelca) */
#endif
    pico_marca_arranque(0x40);   /* V7/H12 3.1: el volcado, a punto de borrar */
    if (pico_flash_erase(LOG_FLASH_OFFSET, len) != 0) return -1;
    pico_marca_arranque(0x41);   /* borrado; a punto de grabar */
    int r = pico_flash_write(LOG_FLASH_OFFSET, src, len);
    pico_marca_arranque(0x42);   /* grabado */
    return r;
}

/* #653 (4-oct) — la sección crítica de una línea: las interrupciones enmascaradas, guardando el estado (vale en
 * cualquier contexto: los ganchos de FreeRTOS, el arranque antes del planificador). Con UN núcleo basta.
 * V7/H12, etapa 3.1 — con DOS, además un spinlock (el del SDK: en la RP2350 va por exclusivas, por su errata de los
 * del SIO), que vale en una tarea, en una interrupción y antes del planificador: apagar las interrupciones sólo para
 * al núcleo propio. Nunca se aparca al otro núcleo con él cogido (el volcado a la flash va FUERA: bpvm_log.c). */
#if ( configNUMBER_OF_CORES > 1 )
static spin_lock_t* s_log_spin = NULL;
static uint32_t log_entrar(void) { return spin_lock_blocking(s_log_spin); }
static void     log_salir(uint32_t estado) { spin_unlock(s_log_spin, estado); }
#else
static uint32_t log_entrar(void) { return save_and_disable_interrupts(); }
static void     log_salir(uint32_t estado) { restore_interrupts(estado); }
#endif

void log_init(void) {
#if ( configNUMBER_OF_CORES > 1 )
    if (s_log_spin == NULL) s_log_spin = spin_lock_init(spin_lock_claim_unused(true));   /* V7/H12, etapa 3.1 */
#endif
    bpvm_log_cintura_t c;
    memset(&c, 0, sizeof c);   /* #653: los campos que se añadan nacen a NULL */
    c.now_ms      = now_ms;
    c.flash_read  = flash_read;
    c.flash_write = flash_write;
    c.region_buf  = bplog_region;
    c.region_size = LOG_REGION_BYTES;
    c.entrar      = log_entrar;
    c.salir       = log_salir;
    bpvm_log_init(&c);
}
