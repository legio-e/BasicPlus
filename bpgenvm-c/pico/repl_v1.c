/*
 * repl_v1.c — dispatcher de mensajes JSON wire v1 en el firmware Pico.
 *
 * Fase A: HELLO/HELLO_REPLY.
 * Fase B: FILES (LIST/STAT/GET/PUT/DEL/MKDIR/RMDIR/RENAME/FORMAT/
 *         SAVE/DF/LOG_DUMP).
 * Fase C-E: pendiente (TERMINAL, META, DEBUG).
 *
 * Patrón de cada handler:
 *   1. Validar campos requeridos. Si faltan → wire_v1_send_error(id,
 *      "INVALID_PARAM", ...).
 *   2. Hacer el trabajo (FS, etc.). Si falla → error con código
 *      apropiado.
 *   3. Enviar reply (reply_empty si no hay datos, o construir con
 *      builders).
 *
 * El thread caller es la task del REPL — single-threaded. No hace
 * falta sincronizar el acceso a stdout entre handlers.
 */

#include "repl_v1.h"
#include "wire_v1.h"
#include "json_min.h"
#include "fs.h"
#include "log.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BPVM_PICO_BUILD_DATE
#define BPVM_PICO_BUILD_DATE  __DATE__ " " __TIME__
#endif

/* Buffer reusable para la línea entrante (WIRE_V1_LINE_MAX = 2 KB). */
static char s_line_buf[WIRE_V1_LINE_MAX];

/* Buffer estático para construir replies pequeñas (no LIST/LOG_DUMP). */
static char s_reply_buf[1024];

/* Buffer estático para data PUT entrante. 16 KB es de sobra para los
 * .mod típicos (2-10 KB) y los driver bundles más generosos. PUTs
 * más grandes se rechazan con NO_SPACE en el dispatcher.
 *
 * Por qué no FS_DATA_SIZE (128 KB) entero: ya tenemos s_data (128 KB),
 * tmp del compact (128 KB) y VM_BUFFER (128 KB). Otro buffer del
 * mismo tamaño desbordaría los 512 KB SRAM del RP2350. Si alguna vez
 * hace falta subir ficheros >16 KB, opciones a explorar:
 *  - Heap dinámico (FreeRTOS heap actual es solo 32 KB; insuficiente).
 *  - Streaming chunk a chunk a fs_put (requiere extender la API FS
 *    para soportar PUT incremental).
 *  - Subir mediante múltiples PUTs con prefijo y un comando FINISH
 *    que concatene (workaround a nivel cliente, sin cambios FS). */
#define V1_PUT_BUF_SIZE  (16 * 1024)
static uint8_t s_put_buf[V1_PUT_BUF_SIZE];

/* ============================================================ */
/* Helper: convierte código fs_status_t en (code, message) v1. */
static void map_fs_status(fs_status_t s, const char** code, const char** msg) {
    switch (s) {
        case FS_OK:                 *code = "OK";              *msg = "ok"; break;
        case FS_ERR_NOT_FOUND:      *code = "NOT_FOUND";       *msg = "fichero no existe"; break;
        case FS_ERR_EXISTS:         *code = "EXISTS";          *msg = "ya existe"; break;
        case FS_ERR_NO_SPACE:       *code = "NO_SPACE";        *msg = "FS lleno"; break;
        case FS_ERR_NAME_TOO_LONG:  *code = "INVALID_PATH";    *msg = "nombre demasiado largo"; break;
        case FS_ERR_TOO_BIG:        *code = "NO_SPACE";        *msg = "fichero demasiado grande"; break;
        case FS_ERR_TABLE_FULL:     *code = "NO_SPACE";        *msg = "tabla FS llena"; break;
        case FS_ERR_BAD_FLASH:      *code = "INTERNAL_ERROR";  *msg = "flash op falló"; break;
        case FS_ERR_INVALID:        *code = "INVALID_PARAM";   *msg = "argumento inválido"; break;
        default:                     *code = "INTERNAL_ERROR";  *msg = "fs unknown"; break;
    }
}

/* ============================================================ */
/* HELLO — META. */

static void handle_hello(long id, const json_obj_t* obj) {
    (void) obj;
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0,
                                  "HELLO_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "protoVersion", 1);
    if (off < 0) goto err;
    off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                                "serverName", "bpvm-pico");
    if (off < 0) goto err;
    off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                                "serverBuild", BPVM_PICO_BUILD_DATE);
    if (off < 0) goto err;
    /* Capabilities — crecerá con cada fase. Hoy META + FILES. */
    static const char* CAPS = ",\"capabilities\":[\"META\",\"FILES\"]";
    size_t caps_len = strlen(CAPS);
    if ((size_t) off + caps_len + 1 > sizeof(s_reply_buf)) goto err;
    memcpy(s_reply_buf + off, CAPS, caps_len);
    off += (int) caps_len;
    off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(s_reply_buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "HELLO_REPLY no cabe");
}

/* ============================================================ */
/* LIST — emite entries por streaming a stdout porque pueden ser muchas. */

typedef struct {
    long id;
    int  first;       /* 1 = todavía no se ha escrito ninguna entry */
} list_ctx_t;

static int list_cb(const char* name, uint32_t size, void* user) {
    list_ctx_t* ctx = (list_ctx_t*) user;
    /* Emit por bloques pequeños — el USB CDC del Pico tolera writes
     * cortos en streaming sin problema. */
    if (!ctx->first) fputc(',', stdout);
    ctx->first = 0;
    /* {"name":"<escaped>","size":<n>,"isDir":false} */
    fputs("{\"name\":\"", stdout);
    /* Escape inline: solo " y \ son comunes en paths. Resto literal. */
    for (const char* p = name; *p; p++) {
        char c = *p;
        if (c == '"' || c == '\\') fputc('\\', stdout);
        fputc(c, stdout);
    }
    fputc('"', stdout);
    fprintf(stdout, ",\"size\":%u,\"isDir\":false}", (unsigned) size);
    return 0;
}

static void handle_list(long id, const json_obj_t* obj) {
    (void) obj;   /* el path es informativo; FS plano lista todo */
    fputs("{\"type\":\"LIST_REPLY\",\"id\":", stdout);
    fprintf(stdout, "%ld,\"entries\":[", id);
    list_ctx_t ctx = { id, 1 };
    fs_list(list_cb, &ctx);
    fputs("]}\n", stdout);
    fflush(stdout);
}

/* ============================================================ */
/* STAT */

static void handle_stat(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    const uint8_t* data; uint32_t size;
    fs_status_t s = fs_get(path, &data, &size);
    if (s != FS_OK) {
        const char* code; const char* msg;
        map_fs_status(s, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0,
                                  "STAT_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "size", (long) size);
    if (off < 0) goto err;
    off = wire_v1_field_bool(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "isDir", 0);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "mtime", 0);   /* FS no tiene mtime */
    if (off < 0) goto err;
    off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(s_reply_buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "STAT_REPLY no cabe");
}

/* ============================================================ */
/* GET — reply con bulk. */

static void handle_get(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    const uint8_t* data; uint32_t size;
    fs_status_t s = fs_get(path, &data, &size);
    if (s != FS_OK) {
        const char* code; const char* msg;
        map_fs_status(s, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }
    /* Header: {"type":"GET_REPLY","id":N,"bulk":<size>} */
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0,
                                  "GET_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_bulk(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              (size_t) size);
    if (off < 0) goto err;
    off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(s_reply_buf, (size_t) off);
    /* Y los bytes raw. */
    if (size > 0) wire_v1_send_bulk(data, (size_t) size);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "GET_REPLY no cabe");
}

/* ============================================================ */
/* PUT — request lleva bulk. */

/* PUT necesita un manejo especial: el dispatcher común DRAINA el bulk
 * antes de despachar; aquí necesitamos en su lugar LEERLO al buffer.
 * Por eso el dispatcher pasa los bytes ya leídos a través del puntero
 * y la longitud. */
static void handle_put(long id, const json_obj_t* obj,
                       const uint8_t* bulk, size_t bulk_size) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    fs_status_t s = fs_put(path, bulk, (uint32_t) bulk_size);
    if (s != FS_OK) {
        const char* code; const char* msg;
        map_fs_status(s, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }
    /* Reply incluye size para que el cliente confirme cuánto se escribió. */
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0,
                                  "PUT_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "size", (long) bulk_size);
    if (off < 0) goto err;
    off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(s_reply_buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "PUT_REPLY no cabe");
}

/* ============================================================ */
/* DEL */

static void handle_del(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    fs_status_t s = fs_delete(path);
    if (s != FS_OK) {
        const char* code; const char* msg;
        map_fs_status(s, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }
    wire_v1_send_reply_empty("DEL_REPLY", id);
}

/* ============================================================ */
/* MKDIR / RMDIR — no-op en FS plano. */

static void handle_mkdir(long id, const json_obj_t* obj) {
    (void) obj;
    /* En FS plano con `/` como namespace, no hay nodos de directorio.
     * MKDIR es idempotente y silenciosa. */
    wire_v1_send_reply_empty("MKDIR_REPLY", id);
}

static void handle_rmdir(long id, const json_obj_t* obj) {
    (void) obj;
    /* Idem MKDIR. RMDIR de un "directorio" no-vacío debería fallar
     * según el spec, pero para v1 simplemente devolvemos OK. El
     * cliente puede iterar y borrar ficheros uno a uno si quiere
     * vaciar un prefijo. */
    wire_v1_send_reply_empty("RMDIR_REPLY", id);
}

/* ============================================================ */
/* RENAME — copia + delete. */

static void handle_rename(long id, const json_obj_t* obj) {
    char from[64], to[64];
    if (json_get_str(obj, "from", from, sizeof(from)) < 0 ||
        json_get_str(obj, "to",   to,   sizeof(to))   < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "faltan from/to");
        return;
    }
    const uint8_t* data; uint32_t size;
    fs_status_t s = fs_get(from, &data, &size);
    if (s != FS_OK) {
        const char* code; const char* msg;
        map_fs_status(s, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }
    /* Copia a buffer estático antes de tocar el FS, porque fs_put
     * puede invalidar el puntero `data` si compacta. Limitado por
     * V1_PUT_BUF_SIZE igual que PUT. */
    if (size > V1_PUT_BUF_SIZE) {
        wire_v1_send_error(id, "NO_SPACE",
                            "RENAME: source >16KB (límite del buffer v1)");
        return;
    }
    memcpy(s_put_buf, data, size);
    s = fs_put(to, s_put_buf, size);
    if (s != FS_OK) {
        const char* code; const char* msg;
        map_fs_status(s, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }
    fs_delete(from);   /* tolerante: si falla, el cliente ya tiene la copia */
    wire_v1_send_reply_empty("RENAME_REPLY", id);
}

/* ============================================================ */
/* FORMAT — borra todo el FS RAM. Requiere confirm:"YES". */

static void handle_format(long id, const json_obj_t* obj) {
    char confirm[8];
    if (json_get_str(obj, "confirm", confirm, sizeof(confirm)) < 0 ||
        strcmp(confirm, "YES") != 0) {
        wire_v1_send_error(id, "MISSING_CONFIRM",
                            "FORMAT requiere {\"confirm\":\"YES\"}");
        return;
    }
    fs_format_ram();
    wire_v1_send_reply_empty("FORMAT_REPLY", id);
}

/* ============================================================ */
/* SAVE — persiste el FS RAM a flash. */

static void handle_save(long id, const json_obj_t* obj) {
    (void) obj;
    uint32_t t0 = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    fs_status_t s = fs_save_to_flash();
    uint32_t dt = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) - t0;
    if (s != FS_OK) {
        const char* code; const char* msg;
        map_fs_status(s, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0,
                                  "SAVE_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "durationMs", (long) dt);
    if (off < 0) goto err;
    off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(s_reply_buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "SAVE_REPLY no cabe");
}

/* ============================================================ */
/* DF — stats del FS. */

static void handle_df(long id, const json_obj_t* obj) {
    (void) obj;
    long total = (long) fs_total_bytes();
    long used  = (long) fs_used_bytes();
    long fcnt  = (long) fs_file_count();
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0,
                                  "DF_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "totalBytes", total);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "usedBytes", used);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "freeBytes", total - used);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off,
                              "fileCount", fcnt);
    if (off < 0) goto err;
    off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(s_reply_buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "DF_REPLY no cabe");
}

/* ============================================================ */
/* LOG_DUMP — text del log persistente, embebido como string JSON. */

typedef struct {
    int first;        /* 1 = todavía no se ha escrito el header */
} log_ctx_t;

/* Escapa y emite chunks del log directamente a stdout, dentro de las
 * comillas del campo "text". */
static void log_chunk_sink(const char* data, size_t len, void* user) {
    (void) user;
    for (size_t i = 0; i < len; i++) {
        char c = data[i];
        switch (c) {
            case '"':  fputs("\\\"", stdout); break;
            case '\\': fputs("\\\\", stdout); break;
            case '\n': fputs("\\n", stdout);  break;
            case '\r': fputs("\\r", stdout);  break;
            case '\t': fputs("\\t", stdout);  break;
            default:
                if ((unsigned char) c < 0x20) {
                    fprintf(stdout, "\\u%04x", (unsigned) c);
                } else {
                    fputc(c, stdout);
                }
                break;
        }
    }
}

static void handle_log_dump(long id, const json_obj_t* obj) {
    (void) obj;
    fputs("{\"type\":\"LOG_DUMP_REPLY\",\"id\":", stdout);
    fprintf(stdout, "%ld,\"text\":\"", id);
    log_dump(log_chunk_sink, NULL);
    fputs("\"}\n", stdout);
    fflush(stdout);
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

    /* 3. Si hay bulk, leerlo al buffer ANTES de despachar. Si fuera
     *    de rango, drain y error. */
    long bulk = json_get_long(&obj, "bulk", 0);
    size_t bulk_size = 0;
    if (bulk > 0) {
        if (bulk > (long) sizeof(s_put_buf)) {
            /* Drain en chunks pequeños y responder NO_SPACE. */
            static uint8_t drain[64];
            long remaining = bulk;
            while (remaining > 0) {
                size_t chunk = (size_t)(remaining > (long) sizeof(drain)
                                         ? (long) sizeof(drain) : remaining);
                if (wire_v1_recv_bulk(drain, chunk, sizeof(drain)) < 0) break;
                remaining -= (long) chunk;
            }
            long id_err = json_get_long(&obj, "id", 0);
            wire_v1_send_error(id_err, "NO_SPACE",
                                "bulk supera el buffer del servidor");
            return;
        }
        if (wire_v1_recv_bulk(s_put_buf, (size_t) bulk, sizeof(s_put_buf)) < 0) {
            wire_v1_send_fatal("PROTOCOL_ERROR", "lectura de bulk truncada");
            return;
        }
        bulk_size = (size_t) bulk;
    }

    /* 4. id (puede ser 0 si el peer no lo mandó). */
    long id = json_get_long(&obj, "id", 0);

    /* 5. type. */
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        wire_v1_send_error(id, "PROTOCOL_ERROR", "falta 'type'");
        return;
    }

    /* 6. Despachar. */
    /* META */
    if (strcmp(type, "HELLO")    == 0) { handle_hello(id, &obj);    return; }
    /* FILES */
    if (strcmp(type, "LIST")     == 0) { handle_list(id, &obj);     return; }
    if (strcmp(type, "STAT")     == 0) { handle_stat(id, &obj);     return; }
    if (strcmp(type, "GET")      == 0) { handle_get(id, &obj);      return; }
    if (strcmp(type, "PUT")      == 0) { handle_put(id, &obj, s_put_buf, bulk_size); return; }
    if (strcmp(type, "DEL")      == 0) { handle_del(id, &obj);      return; }
    if (strcmp(type, "MKDIR")    == 0) { handle_mkdir(id, &obj);    return; }
    if (strcmp(type, "RMDIR")    == 0) { handle_rmdir(id, &obj);    return; }
    if (strcmp(type, "RENAME")   == 0) { handle_rename(id, &obj);   return; }
    if (strcmp(type, "FORMAT")   == 0) { handle_format(id, &obj);   return; }
    if (strcmp(type, "SAVE")     == 0) { handle_save(id, &obj);     return; }
    if (strcmp(type, "DF")       == 0) { handle_df(id, &obj);       return; }
    if (strcmp(type, "LOG_DUMP") == 0) { handle_log_dump(id, &obj); return; }

    /* Fase C-E: TERMINAL, META resto, DEBUG. */
    wire_v1_send_error(id, "UNSUPPORTED",
                        "type no implementado en este firmware");
}
