/*
 * log.c — log persistente (STM32/U5). Cintura de plataforma del ESPEJO del log
 * del Pico (pico/log.c): la LÓGICA es idéntica (buffer RAM append-only, formato
 * con timestamp, snapshot con header magic/version/size, dump por chunks). Solo
 * cambian 3 puntos de plataforma:
 *   - timestamp : HAL_GetTick()            (Pico: xTaskGetTickCount * tick)
 *   - lectura   : memcpy desde FLASH_BASE  (Pico: XIP_BASE)
 *   - erase/prog: stm32_flash_erase/write  (Pico: flash_range_* + flash_lock)
 *
 * Sector = BP_LOG (1 página de 8 KB JUSTO ANTES del env, en el hueco que cede el
 * firmware al encoger la región FLASH del .ld). SIN secciones → el flasheo no lo
 * graba → el log sobrevive al reflasheo, igual que el env.
 *
 * El STM32 es bare-metal single-thread → sin lock (a diferencia del Pico
 * dual-core); stm32_flash ya envuelve ICACHE + unlock/lock.
 */
#include "log.h"

#include "flash_layout_stm32.h"   /* BP_LOG_OFFSET / BP_LOG_SIZE + __bp_log_start */
#include "stm32_flash.h"          /* stm32_flash_write / erase (compartidas con FS/env) */
#include "main.h"                 /* HAL_GetTick + FLASH_BASE (CMSIS) */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#define LOG_MAGIC      0x4C4F4731u   /* 'LOG1' — mismo que el Pico */
#define LOG_VERSION    1u

#define LOG_REGION_BYTES   BP_LOG_SIZE     /* 8 KB (1 página de borrado U5) */
#define LOG_FLASH_OFFSET   BP_LOG_OFFSET   /* offset desde FLASH_BASE */

/* Header in-flash: 16 bytes; data sigue. */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;       /* bytes válidos en data */
    uint32_t reserved;
} log_header_t;

#define LOG_DATA_BYTES   (LOG_REGION_BYTES - sizeof(log_header_t))

/* Buffer en RAM. Append-only; si se llena, descarta (no wrap). */
static char     s_buf[LOG_DATA_BYTES];
static uint32_t s_used;
static int      s_initialized = 0;
static int      s_flash_ok = 0;   /* 0 si el .ld y el header divergen → log solo-RAM */

/* ============================================================ */

static void append_raw(const char* str, size_t n) {
    if (s_used + n > LOG_DATA_BYTES) {
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

    /* Red anti-divergencia (misma filosofía que board_mgr): si el .ld y
     * flash_layout_stm32.h no cuadran, NO tocamos flash (podríamos borrar código)
     * → el log queda solo-RAM, pero NUNCA corrompe. __bp_log_start lo exporta el
     * .ld; LOG_FLASH_OFFSET lo calcula el header. Deben coincidir. */
    s_flash_ok = ((uintptr_t) __bp_log_start
                  == (uintptr_t) (FLASH_BASE + LOG_FLASH_OFFSET));

    if (s_flash_ok) {
        const uint8_t* fb = (const uint8_t*) (uintptr_t) (FLASH_BASE + LOG_FLASH_OFFSET);
        log_header_t hdr;
        memcpy(&hdr, fb, sizeof(hdr));
        if (hdr.magic == LOG_MAGIC
                && hdr.version == LOG_VERSION
                && hdr.size <= LOG_DATA_BYTES) {
            memcpy(s_buf, fb + sizeof(hdr), hdr.size);
            s_used = hdr.size;
        }
    }
    s_initialized = 1;
}

void log_printf(const char* fmt, ...) {
    if (!s_initialized) return;

    char line[256];
    uint32_t ms = HAL_GetTick();
    int n = snprintf(line, sizeof(line), "[%5u] ", (unsigned) ms);
    if (n < 0 || n >= (int) sizeof(line)) n = 0;

    va_list ap;
    va_start(ap, fmt);
    int m = vsnprintf(line + n, sizeof(line) - n, fmt, ap);
    va_end(ap);
    if (m < 0) m = 0;

    int total = n + m;
    if (total >= (int) sizeof(line) - 1) total = sizeof(line) - 2;

    if (total == 0 || line[total - 1] != '\n') {
        if (total < (int) sizeof(line) - 1) line[total++] = '\n';
    }
    append_raw(line, (size_t) total);
}

void log_flush(void) {
    if (!s_initialized || !s_flash_ok) return;

    static uint8_t flash_buf[LOG_REGION_BYTES];
    memset(flash_buf, 0xFF, sizeof(flash_buf));

    log_header_t hdr = { LOG_MAGIC, LOG_VERSION, s_used, 0 };
    memcpy(flash_buf, &hdr, sizeof(hdr));
    memcpy(flash_buf + sizeof(hdr), s_buf, s_used);

    uint32_t addr = FLASH_BASE + LOG_FLASH_OFFSET;
    if (stm32_flash_erase(addr, 1u) == 0)                 /* 1 página de 8 KB */
        stm32_flash_write(addr, flash_buf, sizeof(flash_buf));
}

void log_clear_ram(void) {
    s_used = 0;
    if (s_initialized) memset(s_buf, 0, sizeof(s_buf));
}

void log_clear_flash(void) {
    if (!s_flash_ok) return;
    stm32_flash_erase(FLASH_BASE + LOG_FLASH_OFFSET, 1u);
}

void log_dump(log_sink_t cb, void* user) {
    if (!cb || s_used == 0) return;
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
