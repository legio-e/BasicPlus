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

int wire_v1_field_ulong(char* buf, size_t buf_max, size_t off,
                         const char* key, unsigned long value) {
    char tmp[24];
    snprintf(tmp, sizeof tmp, "%lu", value);
    int r = put_cstr(buf, buf_max, off, ",\""); if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, key);        if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\":");      if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, tmp);        if (r < 0) return -1; off = (size_t) r;
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

/* ─── V7 (#557, 25-sep) — UN CAMPO DE TEXTO QUE SE RECORTA EN VEZ DE HUNDIR EL MENSAJE ───
 *
 * `wire_v1_field_string` devuelve -1 si el texto no cabe, y quien construye el mensaje lo
 * abandona entero. Para casi todo es lo correcto —un campo a medias sería un dato falso—,
 * pero no para el `errorMessage` del EXITED: el error de enlace llega a 1.536 B
 * (`link_error`, por paridad con miVM) y el EXITED de la Pico tiene 1.024 B, el del STM32
 * 320. Con un error largo el EXITED NO SALÍA, y el IDE (o la tanda) se quedaba esperando el
 * final de un programa que ya había terminado — justo cuando más falta hacía saber por qué.
 *
 * Aquí el texto va entero si cabe; si no, se corta en un límite de CARÁCTER —ni a media
 * secuencia UTF-8 ni a medio escape— y se marca con «...». `reserva` son los bytes que
 * quien llama necesita DESPUÉS del campo (el `}` del final, como poco). Devuelve -1 sólo
 * si no cabe ni la clave. */
static const char MARCA_RECORTE[] = "...";

/* Bytes de ENTRADA del carácter que empieza en `p`: 1 si es ASCII o si la secuencia UTF-8
 * está rota (se copia byte a byte, como hace put_escaped). Nunca lee más allá del '\0'. */
static size_t bytes_de_caracter(const unsigned char* p) {
    unsigned char c = *p;
    size_t n = (c < 0x80) ? 1 : ((c & 0xE0) == 0xC0) ? 2 : ((c & 0xF0) == 0xE0) ? 3
             : ((c & 0xF8) == 0xF0) ? 4 : 1;
    for (size_t k = 1; k < n; k++)
        if ((p[k] & 0xC0) != 0x80) return 1;
    return n;
}

int wire_v1_field_string_recortado(char* buf, size_t buf_max, size_t off,
                                   const char* key, const char* value, size_t reserva) {
    if (reserva >= buf_max) return -1;
    if (!value) value = "";
    size_t lim = buf_max - reserva;
    int r = wire_v1_field_string(buf, lim, off, key, value);
    if (r >= 0) return r;                                           /* cabe entero */
    r = put_cstr(buf, lim, off, ",\"");   if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, lim, off, key);      if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, lim, off, "\":\"");  if (r < 0) return -1; off = (size_t) r;
    size_t cola = (sizeof MARCA_RECORTE - 1) + 1;                   /* «...» y la comilla */
    if (off + cola > lim) return -1;
    size_t tope = lim - cola;
    const unsigned char* p = (const unsigned char*) value;
    while (*p) {
        size_t n = bytes_de_caracter(p);
        if (n == 1) {
            char uno[2] = { (char) *p, '\0' };
            r = put_escaped(buf, tope, off, uno);                  /* 1, 2 o 6 bytes */
        } else {
            r = put_raw(buf, tope, off, (const char*) p, n);        /* el carácter entero */
        }
        if (r < 0) break;                                           /* aquí se corta */
        off = (size_t) r;
        p += n;
    }
    r = put_cstr(buf, lim, off, MARCA_RECORTE); if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, lim, off, "\"");          if (r < 0) return -1; off = (size_t) r;
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

/* ===================== V7/H10 — la salida del programa (OUTPUT), común ===================== *
 * El RUN común, paso 1 (3-oct). Las reglas y el porqué, en bpvm_wire_v1.h. */
static char s_trama[WIRE_V1_SALIDA_TRAMA + 96];

/* Cuántos bytes tiene el carácter que empieza por `b` (1-4), o 0 si `b` no puede empezar uno. */
static unsigned largo_utf8(unsigned char b) {
    if (b < 0x80) return 1;
    if (b >= 0xC2 && b <= 0xDF) return 2;
    if (b >= 0xE0 && b <= 0xEF) return 3;
    if (b >= 0xF0 && b <= 0xF4) return 4;
    return 0;   /* una continuación suelta, C0/C1 (overlong) o > F4 */
}

static size_t salida_cabecera(const wire_v1_salida_t* s) {
    int n = snprintf(s_trama, sizeof s_trama, "{\"type\":\"OUTPUT\",\"session\":%ld,\"stream\":\"stdout\",\"data\":\"",
                     s->session);
    return (n > 0) ? (size_t) n : 0;
}

static void salida_manda(size_t o) {
    s_trama[o++] = '"'; s_trama[o++] = '}';
    wire_v1_send_line(s_trama, o);
}

/* Mete un trozo YA escapado (entero: un escape o un carácter) en la trama; si no cabe, la manda y empieza otra. */
static size_t salida_pon(const wire_v1_salida_t* s, size_t o, size_t h, const char* p, size_t n) {
    if (o - h + n > WIRE_V1_SALIDA_TRAMA) { salida_manda(o); o = salida_cabecera(s); }
    memcpy(s_trama + o, p, n);
    return o + n;
}

void wire_v1_salida_inicia(wire_v1_salida_t* s, long session) {
    s->session = session; s->nresto = 0;
}

void wire_v1_salida(wire_v1_salida_t* s, const char* data, size_t len) {
    static const char HEX[] = "0123456789abcdef";
    size_t h = salida_cabecera(s), o = h, i = 0;
    int algo = 0;
    while (i < len || s->nresto) {
        unsigned char car[4]; unsigned n;
        if (s->nresto) {                    /* lo que quedó partido, completado con lo que llega */
            unsigned want = largo_utf8(s->resto[0]);
            while (s->nresto < want && i < len && ((unsigned char) data[i] & 0xC0) == 0x80)
                s->resto[s->nresto++] = (unsigned char) data[i++];
            if (s->nresto < want && i >= len) break;          /* sigue a medias: a la próxima */
            n = s->nresto; memcpy(car, s->resto, n); s->nresto = 0;
            if (n < want) { car[0] = 0; n = 0; }             /* cortado por un byte que no continúa: U+FFFD */
        } else {
            unsigned char b = (unsigned char) data[i];
            unsigned want = largo_utf8(b);
            if (want <= 1) { car[0] = b; n = 1; i++; if (want == 0) n = 0; }
            else {
                unsigned k = 1;
                while (k < want && i + k < len && ((unsigned char) data[i + k] & 0xC0) == 0x80) k++;
                if (k < want && i + k >= len) {               /* el carácter sigue en la llamada siguiente */
                    memcpy(s->resto, data + i, k); s->nresto = k; i += k;
                    break;
                }
                if (k < want) { n = 0; i += k; }              /* le falta un trozo: U+FFFD */
                else { memcpy(car, data + i, want); n = want; i += want; }
            }
        }
        char e[8]; size_t ne;
        if (n == 0) { memcpy(e, "\\ufffd", 6); ne = 6; }
        else if (n == 1) {
            unsigned char c = car[0];
            switch (c) {
                case '"':  memcpy(e, "\\\"", 2); ne = 2; break;
                case '\\': memcpy(e, "\\\\", 2); ne = 2; break;
                case '\n': memcpy(e, "\\n", 2);  ne = 2; break;
                case '\r': memcpy(e, "\\r", 2);  ne = 2; break;
                case '\t': memcpy(e, "\\t", 2);  ne = 2; break;
                default:
                    if (c < 0x20) { e[0] = '\\'; e[1] = 'u'; e[2] = '0'; e[3] = '0'; e[4] = HEX[c >> 4]; e[5] = HEX[c & 15]; ne = 6; }
                    else { e[0] = (char) c; ne = 1; }
            }
        } else { memcpy(e, car, n); ne = n; }
        o = salida_pon(s, o, h, e, ne);
        algo = 1;
    }
    if (algo && o > h) salida_manda(o);
}

void wire_v1_salida_cierra(wire_v1_salida_t* s) {
    if (s->nresto == 0) return;
    s->nresto = 0;
    size_t h = salida_cabecera(s);
    salida_manda(salida_pon(s, h, h, "\\ufffd", 6));
}
