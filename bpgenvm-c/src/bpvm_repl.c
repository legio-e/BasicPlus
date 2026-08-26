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
#include "bpvm_fs.h"
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


/* ══════════════ GRUPO 2 — el FS por la FACHADA común ══════════════
 *
 * Solo verbos que se resuelven ENTEROS con `bpvm_fs_*`: DEL, MKDIR, RMDIR,
 * STAT, RENAME, GET. Los que piden primitivas de familia (DF con sus
 * contadores, FORMAT, SAVE) esperan al grupo de `ops`.
 *
 * Divergencias unificadas aquí (26-ago, medidas en U3.0b y al migrar):
 *   · códigos de error: el STM32 decía INVALID_PATH/"missing path" donde la
 *     Pico dice INVALID_PARAM/"falta path". Gana la referencia.
 *   · STAT: misma forma en las dos (size + isDir + mtime), y el `crc` BAJO
 *     DEMANDA (#398) que el STM32 no tenía.
 *   · RENAME y RMDIR no existían en el STM32: entran por el común — RENAME
 *     con primitiva real (fachada → lfs_rename); RMDIR hereda el stub v1
 *     de la Pico (los directorios son prefijos), a sabiendas.
 *   · el DEL del STM32 llamaba a fs_save() tras borrar — no-op documentado
 *     en littlefs (persiste en cada close): se cae sin cambio de conducta. */

static void repl_del(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    if (bpvm_fs_remove(path) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no existe");
        return;
    }
    wire_v1_send_reply_empty("DEL_REPLY", id);
}

static void repl_stat(long id, const json_obj_t* obj) {
    char path[64];
    char buf[192];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no existe");
        return;
    }
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "STAT_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "size", (long) size);
    if (off < 0) goto err;
    /* #398 — el CRC solo si se pide: es lo que antes calculaba el LIST para
     * TODOS los ficheros en cada refresco del árbol. */
    if (json_get_bool(obj, "crc", 0)) {
        uint32_t c = 0;
        long v = (bpvm_fs_crc32(path, &c) == 0) ? (long) c : -1L;
        off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "crc", v);
        if (off < 0) goto err;
    }
    off = wire_v1_field_bool(buf, sizeof buf, (size_t) off, "isDir", 0);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "mtime", 0);
    if (off < 0) goto err;
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "STAT_REPLY no cabe");
}

static void repl_rename(long id, const json_obj_t* obj) {
    char from[64], to[64];
    if (json_get_str(obj, "from", from, sizeof from) < 0 ||
        json_get_str(obj, "to",   to,   sizeof to)   < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "faltan from/to");
        return;
    }
    /* littlefs renombra él solo (mover una entrada de directorio): sin copia,
     * sin límite de tamaño, y ATÓMICO. #305. */
    if (bpvm_fs_rename(from, to) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no se pudo renombrar");
        return;
    }
    wire_v1_send_reply_empty("RENAME_REPLY", id);
}

static void repl_get(long id, const json_obj_t* obj) {
    char path[64];
    char buf[96];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    /* El GET no carga el fichero: su TAMAÑO para la cabecera y después los
     * bytes POR TROZOS de 256 B — el trozo interno de littlefs, ni un número
     * nuevo ni lecturas partidas a la fuerza (#305/H11). */
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no existe");
        return;
    }
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "GET_REPLY", id);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "GET_REPLY no cabe"); return; }
    off = wire_v1_field_bulk(buf, sizeof buf, (size_t) off, (size_t) size);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "GET_REPLY no cabe"); return; }
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "GET_REPLY no cabe"); return; }
    wire_v1_send_line(buf, (size_t) off);
    /* Si el FS falla a media transferencia ya no hay rectificación posible —
     * la cabecera con `bulk` salió—: se corta y el cliente lo ve por el bulk
     * incompleto, como con un cable desconectado. */
    uint32_t sent = 0;
    while (sent < size) {
        uint8_t chunk[256];
        long n = bpvm_fs_read_at(path, sent, chunk, sizeof chunk);
        if (n <= 0) break;
        wire_v1_send_bulk(chunk, (size_t) n);
        sent += (uint32_t) n;
    }
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
    /* ── grupo 2: el FS por la fachada ── */
    if (strcmp(type, "DEL")    == 0) { repl_del(id, obj);    return 1; }
    if (strcmp(type, "STAT")   == 0) { repl_stat(id, obj);   return 1; }
    if (strcmp(type, "RENAME") == 0) { repl_rename(id, obj); return 1; }
    if (strcmp(type, "GET")    == 0) { repl_get(id, obj);    return 1; }
    if (strcmp(type, "MKDIR")  == 0) {
        /* En un FS plano con '/' como namespace no hay nodos de directorio:
         * idempotente y silenciosa (la semántica v1 de siempre). */
        wire_v1_send_reply_empty("MKDIR_REPLY", id);
        return 1;
    }
    if (strcmp(type, "RMDIR")  == 0) {
        /* Idem: v1 devuelve OK sin hacer nada — el stub que YA tenía la Pico
         * (U3.0b lo dejó escrito). El cliente vacía prefijos borrando. */
        wire_v1_send_reply_empty("RMDIR_REPLY", id);
        return 1;
    }
    return 0;   /* no es del común: la familia sigue con su cadena */
}
