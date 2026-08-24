/*
 * wire_v1_proto.c — el PROTOCOLO del wire v1: construir JSON. Cero hardware.
 *
 * ─── De dónde sale (V6/U2.1, 24-ago-2026) ────────────────────────────────
 *
 * Estas quince funciones —cuatro helpers y once públicas— estaban TRIPLICADAS,
 * una copia en cada firmware con wire: la Pico, el ESP32-S3 y el ESP32-P4. Y no
 * eran copias divergidas: comparadas palabra por palabra eran IDÉNTICAS, 88
 * líneas por tres.
 *
 * Los tres ficheros lo decían, cada uno por su lado — *«los builders JSON son
 * COPIA… si cambia el formato del protocolo, mantener las tres copias en sync»*.
 * Y estaban en sync, o sea que alguien lo venía haciendo a mano y le salía bien.
 * Eso es suerte con disciplina, no un mecanismo. Ahora hay una sola copia.
 *
 * ⚠️ **Lo que NO está aquí, y no debe venir**: mover bytes. `send_line`,
 * `send_bulk`, `recv_line` y `recv_bulk` los pone cada familia porque el cable
 * es distinto de verdad (USB-CDC, UART0, sockets). Ver `bpvm_wire_v1.h`.
 *
 * 📌 Este fichero se movió TAL CUAL, sin reescribir una línea, a propósito: así
 * el código generado se puede comparar contra el de antes y la verificación en
 * placa confirma en vez de descubrir. Si algo de aquí hay que mejorarlo, va en
 * otro commit.
 */
#include "bpvm_wire_v1.h"

#include <stdio.h>    /* snprintf, en put_long */
#include <string.h>

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
    r = put_cstr(buf, buf_max, off, value ? "true" : "false"); if (r < 0) return -1; off = (size_t) r;
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

/* Envia una cadena ya terminada. Vive aqui y no en el transporte porque no
 * mueve bytes: delega en `wire_v1_send_line`, que si es de la familia. */
void wire_v1_send_cstr(const char* cstr) {
    wire_v1_send_line(cstr, strlen(cstr));
}
