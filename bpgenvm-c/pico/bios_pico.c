/* bios_pico.c — la tabla BIOS de ESTA placa (V5/I).
 *
 * Rellena bpvm_bios_t con las funciones reales del firmware y la VERIFICA antes
 * de que nadie la use. La verificación no es ceremonia: un NULL en una ranura es
 * un cuelgue esperando a ocurrir, y aquí se detecta SIN ejecutar nada del pack.
 *
 * ─── DOS CLASES DE RANURA, Y CONVIENE NO CONFUNDIRLAS ───
 *
 * 1. REALES — las de memoria y cadenas: newlib ya las trae en la imagen.
 *    Cero coste: sólo se toma su dirección.
 *
 * 2. CHIVATOS — malloc/free/realloc/localtime: NO se implementan todavía, y
 *    en vez de dejarlas a NULL (que la verificación rechazaría) o apuntarlas al
 *    heap del sistema (que sería SILENCIOSAMENTE peligroso: el pack comería del
 *    heap de FreeRTOS, justo lo que la arena separada existe para evitar), se
 *    apuntan a stubs que GRITAN en el log y devuelven el fallo.
 *
 *    Eso convierte "el pack usa memoria por un camino que no habíamos previsto"
 *    —un fallo que de otro modo aparecería como corrupción semanas después— en
 *    una línea de log en el instante en que ocurre. Criterio de Eduardo (7-ago):
 *    *"hay que poner chivatos que nos digan qué es lo que no funciona, para
 *    saber dónde y no sólo funciona/no funciona"*.
 *
 *    Las tres de memoria saldrán de la arena del ENV cuando toque; `localtime`
 *    del Rtc. Mientras tanto, que se oigan.
 */
#include "bpvm_bios.h"
#include "bpvm_pack.h"   /* bpvm_pack_crc16: el control del ancla */
#include <stdint.h>

int32_t pack_pico_cargar(void);   /* pico/pack_pico.c */
#include "log.h"

#include <string.h>

/* ── 1. La voz del pack. La ranura más importante de la tabla. ── */
static void bios_log(const char* msg) {
    /* "%s" y no msg directo: el pack controla ese texto, y un '%' suelto en él
     * convertiría un mensaje de diagnóstico en un fallo de formato. */
    log_printf("pack: %s", msg ? msg : "(null)");
}

/* ── 2. Chivatos: aún no implementadas, pero NO mudas ── */
static void* bios_malloc(size_t n) {
    log_printf("BIOS: el pack llamo a malloc(%u) y AUN NO HAY ARENA -> NULL",
               (unsigned) n);
    return 0;
}
static void bios_free(void* p) {
    if (p) log_printf("BIOS: el pack llamo a free(%p) sin arena", p);
}
static void* bios_realloc(void* p, size_t n) {
    log_printf("BIOS: el pack llamo a realloc(%p,%u) y AUN NO HAY ARENA -> NULL",
               p, (unsigned) n);
    return 0;
}
static struct tm* bios_localtime(const void* t) {
    (void) t;
    log_printf("BIOS: el pack llamo a localtime y AUN NO HAY RTC conectado -> NULL");
    return 0;
}

/* ── 3. La tabla, estática: su dirección no cambia y se le puede prestar al
 *      pack sin que nadie tenga que mantenerla viva. ── */
static const bpvm_bios_t s_bios = {
    BPVM_BIOS_MAGIC, BPVM_BIOS_VERSION,
    bios_log,
    memcpy, memmove, memset, memcmp, memchr,
    strlen, strcmp, strncmp, strchr, strrchr, strspn, strcspn,
    bios_malloc, bios_free, bios_realloc,
    bios_localtime
};

/* ── 4. EL ANCLA, pegada a la tabla ──
 *
 * Idea de Eduardo: el pack no puede saber la dirección de `s_bios` —cada enlace
 * la mueve—, así que en vez de acertarla, la BUSCA. Aquí está la marca, y los
 * punteros van justo detrás; los rellena el enlazador, o sea que siempre son los
 * correctos PARA ESTA IMAGEN. El pack barre flash, encuentra el texto y lee.
 *
 * ⚠️ La marca se escribe carácter a carácter A PROPÓSITO, no como literal
 * `"BPANCLA1"`. Con un literal el compilador puede dejar además una COPIA suelta
 * en .rodata, y entonces habría dos sitios donde la búsqueda encuentra la marca
 * — uno de ellos sin nada útil detrás. Así sólo existe una.
 */
static const bpvm_ancla_t s_ancla = {
    { 'B','P','A','N','C','L','A','1' },
    BPVM_ANCLA_VERSION,
    (uint16_t) sizeof(bpvm_ancla_t),
    &s_bios,
    bpvm_pack_crc16,         /* el control: crc16("123456789") == 0x29B1 */
    pack_pico_cargar         /* v2: buscar + escalera + saltar */
};

/* Para que el arranque compare lo que ENCUENTRA con lo que SABE. */
const bpvm_ancla_t* bios_pico_ancla(void) { return &s_ancla; }

/*
 * Devuelve la tabla LISTA PARA USAR, o NULL si tiene huecos — y en ese caso deja
 * dicho en el log CUÁL falta, que es la diferencia entre "la BIOS falla" y
 * "la BIOS no tiene memcpy".
 *
 * Se llama en el arranque aunque no haya ningún pack: así el hueco se descubre
 * SIEMPRE, no sólo el día que alguien grabe uno. Un fallo que sólo aparece
 * cuando ya estás depurando otra cosa cuesta el doble.
 */
const bpvm_bios_t* bios_pico_get(void) {
    const char* falta = bpvm_bios_verify(&s_bios);
    if (falta) {
        log_printf("BIOS: INCOMPLETA — falta '%s' (%d ranuras esperadas)",
                   falta, bpvm_bios_slot_count());
        return 0;
    }
    return &s_bios;
}
