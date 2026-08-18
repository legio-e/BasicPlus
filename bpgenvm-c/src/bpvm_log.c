/*
 * bpvm_log.c — núcleo portable del log (ver bpvm_log.h). Toda la lógica; cero
 * dependencias de plataforma (solo la cintura que le pasan). Espejo exacto de la
 * semántica del log del Pico (formato/header/overflow/timestamp), factorizada.
 */
#include "bpvm_log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_MAGIC     0x4C4F4731u   /* 'LOG1' */
#define LOG_VERSION   1u
#define LOG_HEADER    16u           /* magic(4) + version(4) + size(4) + reserved(4) */

static bpvm_log_cintura_t s_c;      /* cintura (copia por valor en init) */
static uint32_t s_used;             /* bytes de data válidos */
static int      s_init = 0;

/* La región = [header 16 B][data...]. La data empieza en region_buf + LOG_HEADER. */
static uint8_t* data_ptr(void) { return s_c.region_buf + LOG_HEADER; }
static uint32_t data_cap(void) { return s_c.region_size - LOG_HEADER; }

/* #433 — ANILLO, no truncado. Esto CORTABA POR EL FINAL: al llenarse dejaba de
 * anotar y se quedaba con el PRINCIPIO. Para un post-mortem es exactamente al
 * reves — lo que importa es lo ULTIMO que paso antes de morir. Y encima callaba:
 * el aviso "[LOG OVERFLOW]" solo se ponia si le cabian sus 16 bytes, asi que en
 * la practica truncaba EN SILENCIO y el log parecia terminar donde se habia
 * acabado el sitio.
 *
 * La Pico ya lo arreglo en #326 (pico/log.c), donde habia mandado la caza a un
 * sitio equivocado DOS veces. El COMUN se quedo atras, asi que el P4, el S3 y el
 * STM32 seguian con el bug — y volvio a morder el 17-ago: la instrumentacion del
 * lazo del GUI (#424) "no registraba nada" porque el log venia lleno del arranque
 * anterior (se restaura de flash al arrancar) y todo lo nuevo se tiraba callando.
 *
 * Ahora se tiran lineas ENTERAS del principio hasta que quepa lo nuevo, y el
 * volcado LO DICE. Mismo algoritmo que pico/log.c, que es la fuente. */
static int s_dropped = 0;

static void append_raw(const char* str, uint32_t n) {
    uint32_t cap = data_cap();
    uint8_t*  d  = data_ptr();
    if (n > cap) n = cap;                      /* linea absurda: recorta */
    if (s_used + n > cap) {
        uint32_t need = s_used + n - cap;
        uint32_t drop = 0;
        while (drop < need && drop < s_used) { /* por LINEAS, no por bytes */
            const uint8_t* nl = (const uint8_t*) memchr(d + drop, '\n', s_used - drop);
            if (!nl) { drop = s_used; break; }
            drop = (uint32_t) (nl - d) + 1u;
        }
        memmove(d, d + drop, s_used - drop);
        s_used -= drop;
        s_dropped = 1;
    }
    memcpy(d + s_used, str, n);
    s_used += n;
}

/* #439 — la cabecera, AL DÍA EN RAM. Antes sólo se escribía dentro de
 * `log_flush`; ahora es lo que permite que un buffer que sobrevive al reset se
 * reconozca a sí mismo. Tres words por línea. */
static void hdr_sync(void) {
    uint32_t magic = LOG_MAGIC, ver = LOG_VERSION, size = s_used;
    uint32_t drop = (uint32_t) s_dropped;
    memcpy(s_c.region_buf + 0,  &magic, 4);
    memcpy(s_c.region_buf + 4,  &ver,   4);
    memcpy(s_c.region_buf + 8,  &size,  4);
    memcpy(s_c.region_buf + 12, &drop,  4);
}

/* #439 — de dónde salió lo cargado (1 = el buffer sobrevivió al reset). Lo
 * consulta el arranque para decirlo: una autopsia que no sabe de cuándo es
 * manda la depuración al sitio equivocado. */
static int s_origen_ram = 0;
int bpvm_log_origen_ram(void) { return s_origen_ram; }

void bpvm_log_init(const bpvm_log_cintura_t* cintura) {
    s_c = *cintura;

    /* #439 — ¿el buffer trae un log VÁLIDO de antes del reset? Si la cintura lo
     * declara en memoria que no se borra (`__uninitialized_ram` en la Pico,
     * `RTC_NOINIT_ATTR` en el ESP32, una sección `.noinit` en el STM32), aquí
     * llega intacto — y es MÁS RECIENTE que el flash, que sólo tiene hasta el
     * último flush. Ese es todo el arreglo.
     *
     * En arranque en frío trae basura y el magic no cuadra, que es el mismo
     * criterio que usa el SDK de la Pico con su token de doble reset.
     *
     * ⚠️ Si la plataforma NO lo declara persistente esto es inocuo: el buffer
     * llega a cero, el magic falla y se sigue por flash como siempre. */
    {
        uint32_t magic, ver, size, drop;
        memcpy(&magic, s_c.region_buf + 0,  4);
        memcpy(&ver,   s_c.region_buf + 4,  4);
        memcpy(&size,  s_c.region_buf + 8,  4);
        memcpy(&drop,  s_c.region_buf + 12, 4);
        if (magic == LOG_MAGIC && ver == LOG_VERSION && size <= data_cap()) {
            s_used    = size;
            s_dropped = (int) drop;   /* el anillo, que vive en .bss y se borra */
            s_origen_ram = 1;
            s_init = 1;
            return;
        }
    }

    s_used = 0;
    s_dropped = 0;   /* #433: arranque limpio; el anillo se marca al llenarse */
    s_origen_ram = 0;
    memset(s_c.region_buf, 0, s_c.region_size);

    /* Recupera el snapshot: lee la región y valida el header. */
    if (s_c.flash_read && s_c.flash_read(s_c.region_buf, s_c.region_size) == 0) {
        uint32_t magic, ver, size;
        memcpy(&magic, s_c.region_buf + 0, 4);
        memcpy(&ver,   s_c.region_buf + 4, 4);
        memcpy(&size,  s_c.region_buf + 8, 4);
        if (magic == LOG_MAGIC && ver == LOG_VERSION && size <= data_cap()) {
            s_used = size;
        } else {
            memset(s_c.region_buf, 0, s_c.region_size);   /* basura → empieza limpio */
        }
    } else {
        memset(s_c.region_buf, 0, s_c.region_size);
    }
    hdr_sync();      /* #439 — reconocible desde ya */
    s_init = 1;
}

/* #423 — el interruptor (ver bpvm_log.h). ENCENDIDO de salida: lo que ocurra
 * antes de que el arranque lea su entorno se registra siempre. */
static int s_enabled = 1;

void bpvm_log_set_enabled(int on) { s_enabled = on ? 1 : 0; }
int  bpvm_log_enabled(void)       { return s_enabled; }

void log_printf(const char* fmt, ...) {
    if (!s_init) return;
    if (!s_enabled) return;   /* #423 — apagado: no añade. Lo escrito se queda. */

    char line[256];
    uint32_t ms = s_c.now_ms ? s_c.now_ms() : 0u;
    int n = snprintf(line, sizeof line, "[%5u] ", (unsigned) ms);
    if (n < 0 || n >= (int) sizeof line) n = 0;

    va_list ap;
    va_start(ap, fmt);
    int m = vsnprintf(line + n, sizeof line - (size_t) n, fmt, ap);
    va_end(ap);
    if (m < 0) m = 0;

    int total = n + m;
    if (total >= (int) sizeof line - 1) total = (int) sizeof line - 2;
    if (total == 0 || line[total - 1] != '\n') {
        if (total < (int) sizeof line - 1) line[total++] = '\n';
    }
    append_raw(line, (uint32_t) total);
    hdr_sync();   /* #439 — que el buffer se reconozca tras un reset */
}

void log_flush(void) {
    if (!s_init || !s_c.flash_write) return;

    /* Escribe el header en region_buf[0..16) y rellena tras la data con 0xFF
     * (imagen limpia); luego vuelca la región ENTERA (el buffer ES la imagen). */
    uint32_t magic = LOG_MAGIC, ver = LOG_VERSION, size = s_used, rsv = 0u;
    memcpy(s_c.region_buf + 0,  &magic, 4);
    memcpy(s_c.region_buf + 4,  &ver,   4);
    memcpy(s_c.region_buf + 8,  &size,  4);
    memcpy(s_c.region_buf + 12, &rsv,   4);
    if (data_cap() > s_used)
        memset(data_ptr() + s_used, 0xFF, data_cap() - s_used);

    s_c.flash_write(s_c.region_buf, s_c.region_size);
}

void log_clear_ram(void) {
    if (!s_init) return;
    s_used = 0;
    /* #433 — y la bandera del anillo. Si no, tras vaciar el log el volcado
     * seguiria avisando de que "se tiraron lineas ANTIGUAS" cuando ya no falta
     * nada: el aviso mentiria, que es justo lo que este anillo vino a arreglar.
     * (Misma correccion que lleva pico/log.c.) */
    s_dropped = 0;
    memset(data_ptr(), 0, data_cap());
}

void log_clear_flash(void) {
    if (!s_init) return;
    log_clear_ram();
    log_flush();                 /* persiste un log vacío (header size=0) */
}

void log_dump(log_sink_t cb, void* user) {
    if (!s_init || !cb || s_used == 0) return;
    /* #433 — que se VEA que faltan lineas. Un log que empieza por el medio sin
     * decirlo se lee como un log completo, y su primera linea miente. */
    if (s_dropped) {
        const char* w = "[LOG: buffer lleno - se tiraron lineas ANTIGUAS "
                        "(anillo); lo de abajo es la cola]\n";
        cb(w, strlen(w), user);
    }
    const uint8_t* d = data_ptr();
    uint32_t off = 0;
    while (off < s_used) {
        uint32_t chunk = s_used - off;
        if (chunk > 256u) chunk = 256u;
        cb((const char*) d + off, (size_t) chunk, user);
        off += chunk;
    }
}

uint32_t log_used_bytes(void)  { return s_used; }
uint32_t log_total_bytes(void) { return s_init ? data_cap() : 0u; }
