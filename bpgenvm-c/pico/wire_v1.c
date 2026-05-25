/*
 * wire_v1.c — implementación del framing wire BPVM v1.
 */

#include "wire_v1.h"

#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===================== Lectura ===================== */

static int try_get_char_v1(void) {
    return getchar_timeout_us(0);
}

int wire_v1_recv_line(int first_char_already_read,
                       char* buf, size_t buf_max) {
    size_t n = 0;
    /* Sembrado opcional con el char ya consumido por el dispatcher. */
    if (first_char_already_read >= 0) {
        if (n + 1 >= buf_max) return -1;
        buf[n++] = (char) first_char_already_read;
    }
    for (;;) {
        int c = try_get_char_v1();
        if (c < 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        if (c == '\n') return (int) n;
        if (c == '\r') continue;             /* tolerante: CR ignorado */
        if (n + 1 >= buf_max) return -1;     /* overflow */
        buf[n++] = (char) c;
    }
}

int wire_v1_recv_bulk(uint8_t* buf, size_t n, size_t buf_max) {
    if (n > buf_max) return -1;
    size_t got = 0;
    while (got < n) {
        int c = try_get_char_v1();
        if (c < 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        buf[got++] = (uint8_t) c;
    }
    return (int) n;
}

/* ===================== Escritura ===================== */

void wire_v1_send_line(const char* data, size_t len) {
    fwrite(data, 1, len, stdout);
    fputc('\n', stdout);
    fflush(stdout);
}

void wire_v1_send_cstr(const char* cstr) {
    wire_v1_send_line(cstr, strlen(cstr));
}

void wire_v1_send_bulk(const uint8_t* data, size_t n) {
    fwrite(data, 1, n, stdout);
    fflush(stdout);
}

/* ===================== Builders JSON ===================== */

/* Helper interno: append literal (sin escape). */
static int put_raw(char* buf, size_t buf_max, size_t off,
                    const char* s, size_t n) {
    if (off + n > buf_max) return -1;
    memcpy(buf + off, s, n);
    return (int)(off + n);
}

/* Helper interno: append C-string sin escape (longitud por strlen). */
static int put_cstr(char* buf, size_t buf_max, size_t off, const char* s) {
    return put_raw(buf, buf_max, off, s, strlen(s));
}

/* Helper interno: escapa y appendea un string JSON SIN comillas. */
static int put_escaped(char* buf, size_t buf_max, size_t off, const char* s) {
    while (*s) {
        char c = *s++;
        const char* esc = NULL;
        char one[3];
        size_t esc_n = 0;
        switch (c) {
            case '"':  esc = "\\\""; esc_n = 2; break;
            case '\\': esc = "\\\\"; esc_n = 2; break;
            case '\n': esc = "\\n";  esc_n = 2; break;
            case '\r': esc = "\\r";  esc_n = 2; break;
            case '\t': esc = "\\t";  esc_n = 2; break;
            default:
                if ((unsigned char) c < 0x20) {
                    /* Control char: \\uXXXX hex. */
                    if (off + 6 > buf_max) return -1;
                    static const char* HEX = "0123456789abcdef";
                    buf[off++] = '\\';
                    buf[off++] = 'u';
                    buf[off++] = '0';
                    buf[off++] = '0';
                    buf[off++] = HEX[(c >> 4) & 0xF];
                    buf[off++] = HEX[c & 0xF];
                    continue;
                }
                one[0] = c; esc = one; esc_n = 1; break;
        }
        if (off + esc_n > buf_max) return -1;
        memcpy(buf + off, esc, esc_n);
        off += esc_n;
    }
    return (int) off;
}

/* Helper interno: long → decimal ASCII appended. */
static int put_long(char* buf, size_t buf_max, size_t off, long v) {
    char tmp[24];
    int n = snprintf(tmp, sizeof(tmp), "%ld", v);
    if (n < 0) return -1;
    return put_raw(buf, buf_max, off, tmp, (size_t) n);
}

int wire_v1_msg_begin(char* buf, size_t buf_max, size_t off,
                       const char* type, long id) {
    int r = put_cstr(buf, buf_max, off, "{\"type\":\""); if (r < 0) return -1; off = (size_t) r;
    r = put_escaped(buf, buf_max, off, type);            if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\",\"id\":");       if (r < 0) return -1; off = (size_t) r;
    r = put_long(buf, buf_max, off, id);                  if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}

int wire_v1_msg_begin_event(char* buf, size_t buf_max, size_t off,
                             const char* type) {
    int r = put_cstr(buf, buf_max, off, "{\"type\":\""); if (r < 0) return -1; off = (size_t) r;
    r = put_escaped(buf, buf_max, off, type);            if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\"");                if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}

int wire_v1_field_long(char* buf, size_t buf_max, size_t off,
                        const char* key, long value) {
    int r = put_cstr(buf, buf_max, off, ",\""); if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, key);        if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\":");      if (r < 0) return -1; off = (size_t) r;
    r = put_long(buf, buf_max, off, value);      if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}

int wire_v1_field_bool(char* buf, size_t buf_max, size_t off,
                        const char* key, int value) {
    int r = put_cstr(buf, buf_max, off, ",\""); if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, key);        if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\":");      if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, value ? "true" : "false");
    if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}

int wire_v1_field_string(char* buf, size_t buf_max, size_t off,
                          const char* key, const char* value) {
    int r = put_cstr(buf, buf_max, off, ",\""); if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, key);        if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\":\"");    if (r < 0) return -1; off = (size_t) r;
    r = put_escaped(buf, buf_max, off, value);   if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\"");        if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}

int wire_v1_field_bulk(char* buf, size_t buf_max, size_t off, size_t n) {
    int r = put_cstr(buf, buf_max, off, ",\"bulk\":"); if (r < 0) return -1; off = (size_t) r;
    r = put_long(buf, buf_max, off, (long) n);          if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}

int wire_v1_msg_end(char* buf, size_t buf_max, size_t off) {
    if (off + 1 > buf_max) return -1;
    buf[off++] = '}';
    return (int) off;
}

/* ===================== Senders pre-empaquetados ===================== */

void wire_v1_send_reply_empty(const char* type, long id) {
    char buf[64];
    int off = wire_v1_msg_begin(buf, sizeof(buf), 0, type, id);
    if (off < 0) return;
    off = wire_v1_msg_end(buf, sizeof(buf), (size_t) off);
    if (off < 0) return;
    wire_v1_send_line(buf, (size_t) off);
}

void wire_v1_send_error(long id, const char* code, const char* message) {
    char buf[512];
    int off = wire_v1_msg_begin(buf, sizeof(buf), 0, "ERROR", id);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"ERROR\",\"id\":0}"); return; }
    off = wire_v1_field_string(buf, sizeof(buf), (size_t) off, "code", code);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"ERROR\",\"id\":0}"); return; }
    off = wire_v1_field_string(buf, sizeof(buf), (size_t) off, "message",
                                message ? message : "");
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"ERROR\",\"id\":0}"); return; }
    off = wire_v1_msg_end(buf, sizeof(buf), (size_t) off);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"ERROR\",\"id\":0}"); return; }
    wire_v1_send_line(buf, (size_t) off);
}

void wire_v1_send_fatal(const char* code, const char* message) {
    char buf[512];
    int off = wire_v1_msg_begin_event(buf, sizeof(buf), 0, "FATAL");
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"FATAL\"}"); return; }
    off = wire_v1_field_string(buf, sizeof(buf), (size_t) off, "code", code);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"FATAL\"}"); return; }
    off = wire_v1_field_string(buf, sizeof(buf), (size_t) off, "message",
                                message ? message : "");
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"FATAL\"}"); return; }
    off = wire_v1_msg_end(buf, sizeof(buf), (size_t) off);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"FATAL\"}"); return; }
    wire_v1_send_line(buf, (size_t) off);
}
