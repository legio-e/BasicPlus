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

#define LOG_MAGIC      0x4C4F4731u   /* 'LOG1' */
#define LOG_VERSION    1u

/* Sector dedicado en flash inmediatamente antes de la región FS.
 * Pico 2 con 4 MB: FS está en los últimos 12 KB (0x3FD000-0x3FFFFF),
 * el log usa el sector previo (0x3FC000-0x3FCFFF). */
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#endif

#define FS_REGION_BYTES        (12u * 1024u)         /* coincide con fs.c */
#define LOG_REGION_BYTES       FLASH_SECTOR_SIZE     /* 4 KB */
#define LOG_FLASH_OFFSET       (PICO_FLASH_SIZE_BYTES - FS_REGION_BYTES - LOG_REGION_BYTES)

/* Header in-flash: 16 bytes alineados, data sigue. */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;       /* bytes válidos en data */
    uint32_t reserved;
} log_header_t;

#define LOG_DATA_BYTES   (LOG_REGION_BYTES - sizeof(log_header_t))

/* Buffer en RAM. Append-only; si se llena, los siguientes log_printf
 * se descartan (no wrap). Suficiente para una sesión de bring-up. */
static char     s_buf[LOG_DATA_BYTES];
static uint32_t s_used;
static int      s_initialized = 0;

/* ============================================================ */

static void append_raw(const char* str, size_t n) {
    if (s_used + n > LOG_DATA_BYTES) {
        /* Trunca y marca overflow. */
        const char* trunc = "\n[LOG OVERFLOW]\n";
        size_t tlen = strlen(trunc);
        if (s_used + tlen <= LOG_DATA_BYTES && s_used > 0
                && s_buf[s_used - 1] != ']') {
            memcpy(s_buf + s_used, trunc, tlen);
            s_used += tlen;
        }
        return;
    }
    memcpy(s_buf + s_used, str, n);
    s_used += n;
}

void log_init(void) {
    s_used = 0;
    memset(s_buf, 0, sizeof(s_buf));

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

    static uint8_t flash_buf[LOG_REGION_BYTES];
    memset(flash_buf, 0xFF, sizeof(flash_buf));

    log_header_t hdr = { LOG_MAGIC, LOG_VERSION, s_used, 0 };
    memcpy(flash_buf, &hdr, sizeof(hdr));
    memcpy(flash_buf + sizeof(hdr), s_buf, s_used);

    /* Erase + program. IRQs OFF para que FreeRTOS no cambie de contexto
     * y nadie acceda a XIP durante la operación. */
    uint32_t saved = save_and_disable_interrupts();
    flash_range_erase(LOG_FLASH_OFFSET, LOG_REGION_BYTES);
    flash_range_program(LOG_FLASH_OFFSET, flash_buf, sizeof(flash_buf));
    restore_interrupts(saved);
}

void log_clear_ram(void) {
    s_used = 0;
    if (s_initialized) memset(s_buf, 0, sizeof(s_buf));
}

void log_clear_flash(void) {
    uint32_t saved = save_and_disable_interrupts();
    flash_range_erase(LOG_FLASH_OFFSET, LOG_REGION_BYTES);
    restore_interrupts(saved);
}

void log_dump(log_sink_t cb, void* user) {
    if (!cb || s_used == 0) return;
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
