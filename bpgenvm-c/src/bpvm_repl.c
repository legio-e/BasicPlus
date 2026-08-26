/*
 * bpvm_repl.c — los verbos COMUNES del REPL wire v1. Ver bpvm_repl.h para el
 * porqué y el reparto común/familia.
 *
 * Regla de la casa: cada verbo de aquí hace LO MISMO que hacía en la familia
 * de referencia (la Pico, el superconjunto), con las divergencias unificadas
 * a propósito y anotadas en el header. Nada de mejoras de paso: primero
 * idéntico y en placa, después ya se verá.
 */
#include "bpvm_repl.h"
#include "bpvm_wire_v1.h"
#include "bpvm_log.h"
#include "bpvm_rtc.h"

#include <stdio.h>
#include <string.h>

/* ── LOG_DUMP ────────────────────────────────────────────────────────────────
 * El texto del log no cabe en una línea del wire, así que la respuesta sale
 * POR PARTES sobre el cable del contrato: la cabeza y los chunks escapados por
 * `send_bulk` (bytes crudos, sin salto) y el cierre por `send_line`, que pone
 * el salto del framing. Es la forma que ya tenía el S3; la Pico escribía a
 * stdout directo (su cable lo permitía) y el STM32 PERDÍA chunks si el escape
 * no le cabía en un buffer estático. */
static void log_chunk_sink(const char* data, size_t len, void* user) {
    (void) user;
    char esc[256 * 2];   /* el núcleo entrega chunks de 256 B; peor caso = ×2 */
    size_t o = 0;
    for (size_t i = 0; i < len; i++) {
        char ch = data[i];
        const char* rep = NULL;
        switch (ch) {
            case '\"':  rep = "\\\""; break;
            case '\\': rep = "\\\\"; break;
            case '\n':  rep = "\\n";  break;
            case '\r':  rep = "\\r";  break;
            case '\t': rep = "\\t";  break;
            default: break;
        }
        if (rep) {
            if (o + 2 > sizeof esc) { wire_v1_send_bulk((const uint8_t*) esc, o); o = 0; }
            esc[o++] = rep[0];
            esc[o++] = rep[1];
        } else if ((unsigned char) ch < 0x20u) {
            /* control raro → espacio: el log es texto nuestro, no datos */
            if (o + 1 > sizeof esc) { wire_v1_send_bulk((const uint8_t*) esc, o); o = 0; }
            esc[o++] = ' ';
        } else {
            if (o + 1 > sizeof esc) { wire_v1_send_bulk((const uint8_t*) esc, o); o = 0; }
            esc[o++] = ch;
        }
    }
    if (o > 0) wire_v1_send_bulk((const uint8_t*) esc, o);
}

static void repl_log_dump(long id) {
    char head[64];
    int hn = snprintf(head, sizeof head,
                      "{\"type\":\"LOG_DUMP_REPLY\",\"id\":%ld,\"text\":\"", id);
    if (hn <= 0) return;
    wire_v1_send_bulk((const uint8_t*) head, (size_t) hn);
    log_dump(log_chunk_sink, NULL);
    wire_v1_send_line("\"}", 2);   /* cierra el JSON y pone el salto del framing */
}

int bpvm_repl_dispatch(const char* type, long id, const json_obj_t* obj) {
    if (strcmp(type, "PING") == 0) {
        wire_v1_send_reply_empty("PONG", id);
        return 1;
    }
    if (strcmp(type, "TIME") == 0) {
        long epochSec = json_get_long(obj, "epochSec", -1);
        if (epochSec < 0) {
            /* La divergencia unificada: esto ERA silencio-OK en S3/STM32. */
            wire_v1_send_error(id, "INVALID_PARAM", "TIME: falta 'epochSec' (>=0)");
            return 1;
        }
        bpvm_rtc_set_now_ms((int64_t) epochSec * 1000LL);
        wire_v1_send_reply_empty("TIME_REPLY", id);
        return 1;
    }
    if (strcmp(type, "LOG_DUMP") == 0) {
        repl_log_dump(id);
        return 1;
    }
    if (strcmp(type, "LOG_CLEAR") == 0) {
        log_clear_ram();
        log_clear_flash();
        log_printf("LOG cleared via wire v1");   /* fecha el corte en el log nuevo */
        wire_v1_send_reply_empty("LOG_CLEAR_REPLY", id);
        return 1;
    }
    return 0;   /* no es del común: la familia sigue con su cadena */
}
