/* bios_esp32.c — la tabla BIOS de la FAMILIA ESP32 (V5/H7 en el P4; V7/H6 P1
 * al común: S3, C3, C6, C5 y P4 con la MISMA tabla).
 *
 * Hermano de `pico/bios_pico.c`, y conviene ver lo PEQUEÑO que es: de las 40
 * ranuras de `bpvm_bios_t`, **37 son idénticas en cualquier placa** y las pone
 * `BPVM_BIOS_TABLA` (en `bpvm_bios.h`, pegado a la struct). Aquí sólo viven las
 * TRES que dependen del hierro — su voz, su reloj y dónde guarda su ENV — más
 * los chivatos de memoria, que aún no salen de ningún sitio. (La arena era la
 * tercera hasta V7/H6.c G1: con varios motores tiene dueño y pasó a ser común.)
 *
 * ⚠️ Y ESE REPARTO NO ES COMODIDAD. El orden de los campos de esa tabla cruza
 * una frontera BINARIA: el pack se compila aparte, se graba, y llama POR
 * POSICIÓN. Si esta familia tuviera su propia copia del inicializador y alguien
 * reordenara la struct olvidándose de una, este firmware llamaría a `memcpy` y
 * ejecutaría otra cosa — sin error de compilación ni de enlace. Es exactamente
 * el fallo de #299 (layout de clase) y #315 (slots de vtable), y las dos veces
 * la cura fue la misma: que el orden viva en UN sitio.
 *
 * ─── DOS CLASES DE RANURA, COMO EN LA PICO ───
 *
 * 1. REALES — memoria y cadenas: newlib ya está en la imagen (el ESP-IDF usa
 *    newlib igual que el SDK del RP2350). Cero coste: sólo su dirección. Y desde
 *    V7/H6 la HAL (gpio/spi/i2c/reloj), que son las fachadas de la VM.
 *
 * 2. CHIVATOS — malloc/free/realloc/localtime: aún sin implementar. NO se dejan
 *    a NULL (la verificación las rechazaría) ni se apuntan al heap del sistema
 *    (que sería SILENCIOSAMENTE peligroso: el pack comería del heap de FreeRTOS,
 *    justo lo que la arena separada evita). Se apuntan a stubs que GRITAN en el
 *    log y devuelven el fallo. Criterio de Eduardo (7-ago): *"hay que poner
 *    chivatos que nos digan qué es lo que no funciona, para saber dónde y no
 *    sólo funciona/no funciona"*. Un motor que quiera memoria dinámica trae su
 *    alocador sobre la `arena` (SQLite: MEMSYS5; LVGL: TLSF).
 */
#include "bpvm_bios.h"
#include "bpvm_pack.h"   /* bpvm_pack_crc16: el control del ancla */
#include "pack_esp32.h"  /* el bloque de RAM del código nativo */
#include "board_mgr_esp32.h"   /* board_mgr_env: el ENV de ESTA placa (G1) */
#include "bpvm_env.h"
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

/* ── V7/H6.c G1 — LA CONFIG DE PLACA: el ENV de esta familia ──
 *
 * La regla (entero o nada, los códigos) es común: `bpvm_env_get_exacto`. Aquí
 * sólo se dice DÓNDE está el ENV de un ESP32, que es lo único que cambia entre
 * familias — y por eso es parámetro de `BPVM_BIOS_TABLA`: una familia que no
 * lo conectara no compilaría. (La arena, que vivía aquí, es ahora
 * `bpvm_bios_arena`: con varios motores tiene dueño, y esa regla no puede
 * tener una copia por familia.) */
static int bios_env_get(const char* clave, char* buf, size_t cap) {
    return bpvm_env_get_exacto(board_mgr_env(), clave, buf, cap);
}

/* ── 3. La tabla, estática: su dirección no cambia y se le puede prestar al
 *      pack sin que nadie tenga que mantenerla viva. ── */
static const bpvm_bios_t s_bios = BPVM_BIOS_TABLA(
    bios_log,
    bios_malloc, bios_free, bios_realloc,
    bios_localtime,
    bios_env_get);

/* ── 4. EL ANCLA, pegada a la tabla ──
 *
 * Idea de Eduardo: el pack no puede saber la dirección de `s_bios` —cada enlace
 * la mueve—, así que en vez de acertarla, la BUSCA. Aquí está la marca, y los
 * punteros van justo detrás; los rellena el enlazador, o sea que siempre son los
 * correctos PARA ESTA IMAGEN.
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
    pack_esp32_cargar        /* v2: buscar + escalera + saltar */
};

/* Para que el arranque compare lo que ENCUENTRA con lo que SABE. */
const bpvm_ancla_t* bios_esp32_ancla(void) { return &s_ancla; }

/*
 * Devuelve la tabla LISTA PARA USAR, o NULL si tiene huecos — y en ese caso deja
 * dicho en el log CUÁL falta, que es la diferencia entre "la BIOS falla" y
 * "la BIOS no tiene memcpy".
 *
 * Se llama en el arranque aunque no haya ningún pack: así el hueco se descubre
 * SIEMPRE, no sólo el día que alguien grabe uno. Un fallo que sólo aparece
 * cuando ya estás depurando otra cosa cuesta el doble.
 */
const bpvm_bios_t* bios_esp32_get(void) {
    const char* falta = bpvm_bios_verify(&s_bios);
    if (falta) {
        log_printf("BIOS: INCOMPLETA — falta '%s' (%d ranuras esperadas)",
                   falta, bpvm_bios_slot_count());
        return 0;
    }
    return &s_bios;
}
