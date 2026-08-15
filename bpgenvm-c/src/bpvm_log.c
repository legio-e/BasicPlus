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

static void append_raw(const char* str, uint32_t n) {
    uint32_t cap = data_cap();
    uint8_t*  d  = data_ptr();
    if (s_used + n > cap) {
        const char* trunc = "\n[LOG OVERFLOW]\n";
        uint32_t tl = (uint32_t) strlen(trunc);
        if (s_used + tl <= cap && s_used > 0 && d[s_used - 1] != ']') {
            memcpy(d + s_used, trunc, tl);
            s_used += tl;
        }
        return;
    }
    memcpy(d + s_used, str, n);
    s_used += n;
}

void bpvm_log_init(const bpvm_log_cintura_t* cintura) {
    s_c = *cintura;
    s_used = 0;
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
    memset(data_ptr(), 0, data_cap());
}

void log_clear_flash(void) {
    if (!s_init) return;
    log_clear_ram();
    log_flush();                 /* persiste un log vacío (header size=0) */
}

void log_dump(log_sink_t cb, void* user) {
    if (!s_init || !cb || s_used == 0) return;
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
