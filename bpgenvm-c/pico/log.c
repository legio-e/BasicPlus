/*
 * log.c — implementación del log persistente.
 */

#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "flash_lock.h"     /* #153 — ventana XIP-safe (dual-core safe) */

#define LOG_MAGIC      0x4C4F4731u   /* 'LOG1' */
#define LOG_VERSION    1u

#include "flash_layout.h"   /* H9: el log vive en la ZONA 2 (kernel) del layout de 3 zonas */

/* H9: sector del log = zona 2 (0x012000). Antes vivía en 0x3FC000, calculado
 * contra el layout del FS LEGADO (FS_REGION_BYTES); con la unificación de
 * particiones el espacio [BP_PART_BASE, usable) es TODO de las particiones y
 * el log se muda al hueco del kernel — que el UF2 no graba (el log también
 * sobrevive al reflasheo). */
#define LOG_REGION_BYTES       FLASH_SECTOR_SIZE     /* 4 KB */
#define LOG_FLASH_OFFSET       BP_LOG_OFFSET

/* Header in-flash: 16 bytes alineados, data sigue. */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;       /* bytes válidos en data */
    uint32_t reserved;
} log_header_t;

#define LOG_DATA_BYTES   (LOG_REGION_BYTES - sizeof(log_header_t))

/* #338 — EL BUFFER DE RAM **ES** LA IMAGEN DE FLASH: [cabecera 16 B][datos].
 *
 * Antes había dos: `s_buf` con los datos (4.080 B) y, dentro de log_flush, un
 * `flash_buf` de 4.096 B más donde se armaba cabecera+datos para escribirlo. Un
 * buffer permanente de 4 KB que sólo servía durante el volcado, y una copia de
 * todo el log en cada flush.
 *
 * No es una idea nueva: es exactamente lo que hace el núcleo portable
 * `bpvm_log` («el buffer RAM que aporta el llamador ES la imagen de flash… el
 * Pico/STM32 se ahorran su antiguo flash_buf»). El ESP32 ya migró a él; la Pico
 * se quedó atrás. Esto aplica su idea aquí, sin mover todavía el fichero
 * entero: la migración completa es otra tanda.
 *
 * Y OJO: esto NO podía salir de la zona compartida de #338. `fs_list` la tiene
 * cogida cuando avisa de un listado incompleto, y ese aviso llama a log_printf
 * + log_flush; el guardián cantaría el choque, pero se perdería la entrada del
 * log — y el log es el instrumento del post-mortem. El log no depende de nadie.
 *
 * ANILLO por líneas: si se llena, se tiran las ANTIGUAS para dejar sitio a las
 * nuevas (ver append_raw). Lo que se conserva es la COLA, que es lo que sirve
 * en un post-mortem. */
static uint8_t s_region[LOG_REGION_BYTES] __attribute__((aligned(4)));
#define s_buf ((char*) (s_region + sizeof(log_header_t)))
static uint32_t s_used;
static int      s_initialized = 0;

/* ============================================================ */

/* ANILLO, no truncado. Antes esto cortaba POR EL FINAL: al llenarse los 4 KB
 * dejaba de anotar y se quedaba con el PRINCIPIO. Para un post-mortem eso es
 * exactamente al revés — lo que importa es lo ÚLTIMO que pasó antes de morir.
 * Y encima callaba: el aviso "[LOG OVERFLOW]" sólo se ponía si le cabían sus 16
 * bytes, así que en la práctica truncaba EN SILENCIO y el log parecía terminar
 * donde se había acabado el sitio. Eso mandó dos veces la caza de #326 a un
 * sitio equivocado: el rastro no acababa donde moría la placa, sino donde se
 * acababa el log.
 * Ahora se tiran líneas ENTERAS del principio hasta que quepa lo nuevo, y el
 * volcado lo DICE. */
static int s_dropped = 0;

static void append_raw(const char* str, size_t n) {
    if (n > LOG_DATA_BYTES) n = LOG_DATA_BYTES;   /* línea absurda: recorta */
    if (s_used + n > LOG_DATA_BYTES) {
        size_t need = s_used + n - LOG_DATA_BYTES;
        size_t drop = 0;
        while (drop < need && drop < s_used) {     /* por líneas, no por bytes */
            const char* nl = memchr(s_buf + drop, '\n', s_used - drop);
            if (!nl) { drop = s_used; break; }
            drop = (size_t) (nl - s_buf) + 1;
        }
        memmove(s_buf, s_buf + drop, s_used - drop);
        s_used -= drop;
        s_dropped = 1;
    }
    memcpy(s_buf + s_used, str, n);
    s_used += n;
}

void log_init(void) {
    s_used = 0;
    s_dropped = 0;
    memset(s_buf, 0, LOG_DATA_BYTES);

    /* Intenta cargar del flash. */
    const uint8_t* flash_base = (const uint8_t*)(XIP_BASE + LOG_FLASH_OFFSET);
    log_header_t hdr;
    memcpy(&hdr, flash_base, sizeof(hdr));
    if (hdr.magic == LOG_MAGIC
            && hdr.version == LOG_VERSION
            && hdr.size <= LOG_DATA_BYTES) {
        memcpy(s_buf, flash_base + sizeof(hdr), hdr.size);
        s_used = hdr.size;
    }
    s_initialized = 1;
}

void log_printf(const char* fmt, ...) {
    if (!s_initialized) return;

    /* Prefijo timestamp ms. */
    char line[256];
    uint32_t ms = (uint32_t) (xTaskGetTickCount() * portTICK_PERIOD_MS);
    int n = snprintf(line, sizeof(line), "[%5u] ", (unsigned) ms);
    if (n < 0 || n >= (int) sizeof(line)) n = 0;

    va_list ap;
    va_start(ap, fmt);
    int m = vsnprintf(line + n, sizeof(line) - n, fmt, ap);
    va_end(ap);
    if (m < 0) m = 0;

    int total = n + m;
    if (total >= (int) sizeof(line) - 1) total = sizeof(line) - 2;

    /* Asegurar newline. */
    if (total == 0 || line[total - 1] != '\n') {
        if (total < (int) sizeof(line) - 1) {
            line[total++] = '\n';
        }
    }
    append_raw(line, (size_t) total);
}

void log_flush(void) {
    if (!s_initialized) return;

    /* #338 — la cabecera se escribe EN SITIO, delante de los datos que ya están
     * donde tienen que estar: `s_region` YA es la imagen. Ni segundo buffer ni
     * copia del log entero en cada volcado. */
    log_header_t hdr = { LOG_MAGIC, LOG_VERSION, s_used, 0 };
    memcpy(s_region, &hdr, sizeof(hdr));
    /* La cola sin usar a 0xFF (flash borrada): así el volcado no arrastra restos
     * de un log anterior más largo, que es justo lo que hacía el memset del
     * buffer que había aquí. */
    if (s_used < LOG_DATA_BYTES)
        memset(s_buf + s_used, 0xFF, LOG_DATA_BYTES - s_used);

    /* Erase + program. IRQs OFF para que FreeRTOS no cambie de contexto
     * y nadie acceda a XIP durante la operación. */
    uint32_t saved = bpvm_flash_lock_begin();
    flash_range_erase(LOG_FLASH_OFFSET, LOG_REGION_BYTES);
    flash_range_program(LOG_FLASH_OFFSET, s_region, LOG_REGION_BYTES);
    bpvm_flash_lock_end(saved);
}

void log_clear_ram(void) {
    s_used = 0;
    /* Y la bandera del anillo. Si no, tras vaciar el log el volcado seguía
     * avisando de que "se tiraron lineas ANTIGUAS" cuando ya no faltaba nada:
     * el aviso mentía, que es justo lo que este anillo vino a arreglar. */
    s_dropped = 0;
    if (s_initialized) memset(s_buf, 0, LOG_DATA_BYTES);
}

void log_clear_flash(void) {
    uint32_t saved = bpvm_flash_lock_begin();
    flash_range_erase(LOG_FLASH_OFFSET, LOG_REGION_BYTES);
    bpvm_flash_lock_end(saved);
}

void log_dump(log_sink_t cb, void* user) {
    if (!cb || s_used == 0) return;
    /* Que se VEA que faltan líneas. Un log que empieza por el medio sin decirlo
     * se lee como un log completo, y entonces su primera línea miente. */
    if (s_dropped) {
        const char* w = "[LOG: buffer lleno — se tiraron lineas ANTIGUAS "
                        "(anillo); lo de abajo es la cola]\n";
        cb(w, strlen(w), user);
    }
    /* Chunks de 256 bytes para no abusar de la pila del sink. */
    size_t off = 0;
    while (off < s_used) {
        size_t chunk = s_used - off;
        if (chunk > 256) chunk = 256;
        cb(s_buf + off, chunk, user);
        off += chunk;
    }
}

uint32_t log_used_bytes(void)  { return s_used; }
uint32_t log_total_bytes(void) { return LOG_DATA_BYTES; }
