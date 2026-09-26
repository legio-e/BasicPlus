/* bios_stm32.c — V7/H6 G5.b0: la tabla BIOS de la FAMILIA STM32.
 *
 * Hermano de `pico/bios_pico.c` y `esp32/common/bios_esp32.c`, y tan pequeño como
 * ellos: de las ranuras de `bpvm_bios_t`, casi todas son idénticas en cualquier
 * placa y las pone `BPVM_BIOS_TABLA` (bpvm_bios.h, pegado a la struct — el orden
 * cruza una frontera BINARIA y no puede tener una copia por familia: #299, #315).
 * Aquí sólo viven las que dependen del hierro —su voz y dónde guarda su ENV— y
 * los chivatos de memoria.
 *
 * ─── DOS CLASES DE RANURA, COMO EN LAS OTRAS FAMILIAS ───
 *
 * 1. REALES — memoria y cadenas (newlib, ya en la imagen), la HAL (gpio/spi/i2c/
 *    reloj: las fachadas de la VM) y el FS: las pone la macro.
 * 2. CHIVATOS — malloc/free/realloc/localtime: no se dejan a NULL (la verificación
 *    las rechazaría) ni se apuntan al heap del sistema (el motor comería del heap
 *    de newlib, justo lo que la arena separada evita). GRITAN en el log y devuelven
 *    el fallo. Criterio de Eduardo (7-ago): chivatos que digan qué no funciona. Un
 *    motor que quiera memoria dinámica trae su alocador sobre la arena (SQLite:
 *    MEMSYS5; LVGL: TLSF).
 */
#include "bpvm_bios.h"
#include "bpvm_pack.h"          /* bpvm_pack_crc16: el control del ancla */
#include "pack_stm32.h"         /* pack_stm32_cargar: la v2 del ancla */
#include "board_mgr_stm32.h"    /* board_mgr_stm32_env_get: el ENV de ESTA placa */
#include "log.h"

#include <stdint.h>
#include <string.h>

/* ── 1. La voz del pack. La ranura más importante de la tabla. ── */
static void bios_log(const char* msg) {
    /* "%s" y no msg directo: el pack controla ese texto, y un '%' suelto en él
     * convertiría un mensaje de diagnóstico en un fallo de formato. */
    log_printf("pack: %s", msg ? msg : "(null)");
}

/* ── 2. Chivatos: aún no implementadas, pero NO mudas ── */
static void* bios_malloc(size_t n) {
    log_printf("BIOS: el pack llamo a malloc(%u) y AUN NO HAY ARENA -> NULL", (unsigned) n);
    return 0;
}
static void bios_free(void* p) {
    if (p) log_printf("BIOS: el pack llamo a free(%p) sin arena", p);
}
static void* bios_realloc(void* p, size_t n) {
    log_printf("BIOS: el pack llamo a realloc(%p,%u) y AUN NO HAY ARENA -> NULL", p, (unsigned) n);
    return 0;
}
static struct tm* bios_localtime(const void* t) {
    (void) t;
    log_printf("BIOS: el pack llamo a localtime y AUN NO HAY RTC conectado -> NULL");
    return 0;
}

/* ── LA CONFIG DE PLACA: el ENV de esta familia. La regla (entero o nada, los
 *    códigos) es común —bpvm_env_get_exacto, dentro de board_mgr_stm32_env_get—;
 *    aquí sólo se dice DÓNDE está el ENV de un STM32. ── */
static int bios_env_get(const char* clave, char* buf, size_t cap) {
    return board_mgr_stm32_env_get(clave, buf, cap);
}

/* ── 3. La tabla, estática: su dirección no cambia y se le puede prestar al pack
 *      sin que nadie tenga que mantenerla viva. ── */
static const bpvm_bios_t s_bios = BPVM_BIOS_TABLA(
    bios_log,
    bios_malloc, bios_free, bios_realloc,
    bios_localtime,
    bios_env_get);

/* ── 4. EL ANCLA, pegada a la tabla ──
 *
 * Idea de Eduardo: el pack no puede saber la dirección de `s_bios` —cada enlace la
 * mueve—, así que en vez de acertarla, la BUSCA: la marca y, justo detrás, los
 * punteros que rellena el enlazador. La marca va carácter a carácter A PROPÓSITO:
 * un literal "BPANCLA1" podría dejar una copia suelta en .rodata, y la búsqueda
 * encontraría dos. */
static const bpvm_ancla_t s_ancla = {
    { 'B','P','A','N','C','L','A','1' },
    BPVM_ANCLA_VERSION,
    (uint16_t) sizeof(bpvm_ancla_t),
    &s_bios,
    bpvm_pack_crc16,         /* el control: crc16("123456789") == 0x29B1 */
    pack_stm32_cargar        /* v2: recorrer la cadena + escalera + saltar */
};

/* Para que el arranque compare lo que ENCUENTRA con lo que SABE. */
const bpvm_ancla_t* bios_stm32_ancla(void) { return &s_ancla; }

/* La tabla LISTA PARA USAR, o NULL si tiene huecos — y entonces el log dice CUÁL
 * falta. Se llama en el arranque aunque no haya ningún pack: el hueco se descubre
 * SIEMPRE, no el día que alguien grabe uno. */
const bpvm_bios_t* bios_stm32_get(void) {
    const char* falta = bpvm_bios_verify(&s_bios);
    if (falta) {
        log_printf("BIOS: INCOMPLETA — falta '%s' (%d ranuras esperadas)",
                   falta, bpvm_bios_slot_count());
        return 0;
    }
    return &s_bios;
}
