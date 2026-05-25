/*
 * repl_v1.c — dispatcher de mensajes JSON wire v1 en el firmware Pico.
 *
 * Fase A: HELLO/HELLO_REPLY funcional. Los demás tipos del wire v1
 * responden con ERROR code=UNSUPPORTED hasta que las fases B-E los
 * añadan.
 *
 * Patrón de cada handler:
 *   1. Validar campos requeridos (json_get_*). Si falta o tipo
 *      inválido → wire_v1_send_error(id, "INVALID_PARAM", ...).
 *   2. Hacer el trabajo (acceder al FS, lanzar la VM, etc.).
 *   3. Enviar reply (wire_v1_send_reply_empty si no hay datos, o
 *      construir mensaje con los builders).
 *
 * El thread caller es la task del REPL — single-threaded. No hace
 * falta sincronizar el acceso a stdout entre handlers.
 */

#include "repl_v1.h"
#include "wire_v1.h"
#include "json_min.h"

#include <stdio.h>
#include <string.h>

/* Build date para el HELLO_REPLY. */
#ifndef BPVM_PICO_BUILD_DATE
#define BPVM_PICO_BUILD_DATE  __DATE__ " " __TIME__
#endif

/* ============================================================ */
/* Buffer estático para la línea entrante. Reusable entre requests
 * porque el handler procesa una sola request a la vez antes de
 * volver a leer del wire. Tamaño WIRE_V1_LINE_MAX (2 KB). */
static char s_line_buf[WIRE_V1_LINE_MAX];

/* ============================================================ */
/* Handlers individuales — Fase A. */

static void handle_hello(long id, const json_obj_t* obj) {
    (void) obj;   /* protoVersion, clientName, clientBuild son informativos */
    char out[256];
    int off = wire_v1_msg_begin(out, sizeof(out), 0, "HELLO_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(out, sizeof(out), (size_t) off,
                              "protoVersion", 1);
    if (off < 0) goto err;
    off = wire_v1_field_string(out, sizeof(out), (size_t) off,
                                "serverName", "bpvm-pico");
    if (off < 0) goto err;
    off = wire_v1_field_string(out, sizeof(out), (size_t) off,
                                "serverBuild", BPVM_PICO_BUILD_DATE);
    if (off < 0) goto err;
    /* capabilities: array literal embebido. Los builders solo manejan
     * strings/longs/bools — para arrays simples lo inline-amos.
     * Las capabilities reales se irán añadiendo conforme las fases
     * B-E se implementen. Por ahora solo META (HELLO/PING/...). */
    static const char* CAPS = ",\"capabilities\":[\"META\"]";
    size_t caps_len = strlen(CAPS);
    if ((size_t) off + caps_len + 1 > sizeof(out)) goto err;
    memcpy(out + off, CAPS, caps_len);
    off += (int) caps_len;
    off = wire_v1_msg_end(out, sizeof(out), (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(out, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "HELLO_REPLY no cabe en buffer");
}

/* ============================================================ */
/* Dispatcher principal. */

void repl_v1_handle_request(int first_char) {
    /* 1. Leer la línea JSON completa. */
    int n = wire_v1_recv_line(first_char, s_line_buf, sizeof(s_line_buf));
    if (n < 0) {
        wire_v1_send_fatal("PROTOCOL_ERROR", "línea excede WIRE_V1_LINE_MAX");
        return;
    }

    /* 2. Parsear el JSON. */
    json_obj_t obj;
    if (json_parse(s_line_buf, (size_t) n, &obj) != 0) {
        wire_v1_send_fatal("PROTOCOL_ERROR", "JSON inválido");
        return;
    }

    /* 3. Si hay bulk en el request, consumirlo del wire ANTES de
     *    despachar — si no, el siguiente request quedaría desalineado
     *    con los bytes binarios.
     *
     *    Fase A no implementa handlers con bulk in (PUT), pero
     *    drenamos el bulk para evitar romper el wire en caso de que
     *    llegue un PUT prematuro. */
    long bulk = json_get_long(&obj, "bulk", 0);
    if (bulk > 0) {
        /* Buffer temporal pequeño y vamos descartando en trozos. */
        static uint8_t drain[64];
        long remaining = bulk;
        while (remaining > 0) {
            size_t chunk = (size_t) (remaining > (long) sizeof(drain)
                                     ? (long) sizeof(drain) : remaining);
            if (wire_v1_recv_bulk(drain, chunk, sizeof(drain)) < 0) break;
            remaining -= (long) chunk;
        }
    }

    /* 4. Obtener id (puede ser 0 si el peer no lo mandó — protocolo
     *    requiere que sí, pero somos tolerantes). */
    long id = json_get_long(&obj, "id", 0);

    /* 5. Obtener type. */
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        wire_v1_send_error(id, "PROTOCOL_ERROR", "falta campo 'type'");
        return;
    }

    /* 6. Despachar. */
    if (strcmp(type, "HELLO") == 0)         { handle_hello(id, &obj); return; }

    /* Fase B-E: por implementar. Por ahora respondemos UNSUPPORTED
     * para que el cliente sepa que el tipo no está disponible aún. */
    wire_v1_send_error(id, "UNSUPPORTED",
                        "type no implementado en Fase A (solo HELLO)");
}
