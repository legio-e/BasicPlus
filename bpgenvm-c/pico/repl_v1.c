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

#include "bpvm.h"
#include "bpvm_internal.h"   /* inspect deps en handle_run */
#include "bpvm_pico.h"       /* INFO: uniqueId/boardName/temp/freq/uptime */
#include "bpvm_rtc.h"        /* TIME: set epoch */

#include "pico/bootrom.h"    /* reset_usb_boot (BOOTSEL) */

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Buffer VM compartido (declarado en main.c). */
extern uint8_t s_vm_buffer[];
extern const uint32_t s_vm_buffer_size;

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
    /* Capabilities — crecerá con cada fase. Hoy META completo (HELLO/
     * INFO/TIME/PING/RESET/BOOTSEL), FILES completo, TERMINAL parcial
     * (RUN/OUTPUT/EXITED, sin KILL ni PROMPT — depende de #136/#139).
     * Añadimos también "BOOTSEL" como capability separada porque es
     * Pico-specific (la VM Java NO la tiene). */
    static const char* CAPS = ",\"capabilities\":[\"META\",\"FILES\",\"TERMINAL\",\"BOOTSEL\"]";
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
/* META — INFO, TIME, PING, RESET, BOOTSEL. */

static void handle_info(long id, const json_obj_t* obj) {
    (void) obj;
    char unique[20]   = "";
    char board[16]    = "";
    bpvm_pico_unique_id(unique, sizeof(unique));
    bpvm_pico_board_name(board, sizeof(board));
    long freq    = (long) bpvm_pico_cpu_freq_hz();
    long uptime  = (long) bpvm_pico_uptime_ms();
    float tempC  = bpvm_pico_temp_c();
    long fsTotal = (long) fs_total_bytes();
    long fsUsed  = (long) fs_used_bytes();

    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0,
                                  "INFO_REPLY", id);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                               (size_t) off, "uniqueId", unique);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                               (size_t) off, "boardName", board);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "cpuFreqHz", freq);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "uptimeMs", uptime);
    /* tempC: JSON number con decimal. Los builders solo manejan long;
     * insertamos manualmente con snprintf. Truncamos a 2 decimales. */
    if (off >= 0) {
        char temp_frag[40];
        int nf = snprintf(temp_frag, sizeof(temp_frag), ",\"tempC\":%.2f", (double) tempC);
        if (nf > 0 && (size_t)(off + nf) < sizeof(s_reply_buf)) {
            memcpy(s_reply_buf + off, temp_frag, (size_t) nf);
            off += nf;
        }
    }
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "fsTotalBytes", fsTotal);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "fsUsedBytes", fsUsed);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf),
                                          (size_t) off);
    if (off < 0) {
        wire_v1_send_error(id, "INTERNAL_ERROR", "INFO_REPLY no cabe");
        return;
    }
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

static void handle_time(long id, const json_obj_t* obj) {
    long epochSec = json_get_long(obj, "epochSec", -1);
    if (epochSec < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "TIME: falta 'epochSec' (>=0)");
        return;
    }
    bpvm_rtc_set_now_ms((int64_t) epochSec * 1000LL);
    wire_v1_send_reply_empty("TIME_REPLY", id);
}

static void handle_ping(long id, const json_obj_t* obj) {
    (void) obj;
    wire_v1_send_reply_empty("PONG", id);
}

static void handle_reset(long id, const json_obj_t* obj) {
    (void) obj;
    /* El cliente espera la reply ANTES del reset. Mandamos primero,
     * después delay corto para que el USB CDC vacíe sus buffers, y
     * por fin watchdog_reboot. El protocolo §6 lo documenta así. */
    log_printf("RESET (wire v1): rebooting");
    log_flush();
    wire_v1_send_reply_empty("RESET_REPLY", id);
    vTaskDelay(pdMS_TO_TICKS(100));
    extern void watchdog_reboot(uint32_t, uint32_t, uint32_t);
    watchdog_reboot(0, 0, 0);
    /* no retorna */
}

static void handle_bootsel(long id, const json_obj_t* obj) {
    (void) obj;
    log_printf("BOOTSEL (wire v1): entering bootloader");
    log_flush();
    wire_v1_send_reply_empty("BOOTSEL_REPLY", id);
    vTaskDelay(pdMS_TO_TICKS(100));
    reset_usb_boot(0, 0);
    /* no retorna */
}

/* ============================================================ */
/* TERMINAL — RUN, OUTPUT streaming, EXITED. */

/* Resolución de módulo con paths /app/ y /lib/. Replica la lógica de
 * fs_get_resolve en repl.c. La duplicamos aquí en lugar de exportarla
 * para mantener el desacoplamiento entre los dos repls durante la
 * migración. Cuando el legacy se borre, este queda como la canónica. */
static fs_status_t v1_get_resolve(const char* name,
                                   const uint8_t** data_out,
                                   uint32_t* size_out) {
    fs_status_t s = fs_get(name, data_out, size_out);
    if (s == FS_OK) return FS_OK;
    char path[FS_NAME_LEN];
    snprintf(path, sizeof(path), "/app/%s", name);
    s = fs_get(path, data_out, size_out);
    if (s == FS_OK) return FS_OK;
    snprintf(path, sizeof(path), "/lib/%s", name);
    return fs_get(path, data_out, size_out);
}

/* Sesión activa (0 = ninguna). Sólo soportamos una sesión RUN a la
 * vez por ahora — KILL multi-sesión y RUN concurrente vendrán cuando
 * tengamos #136 arch-tasks. */
static long s_active_session = 0;

/* Contador monotónico de sesiones. Empieza en 1 (0 reservado para
 * "ninguna"). */
static long s_next_session = 1;

/* Contexto del sink v1: lleva la session a embeber en cada OUTPUT. */
typedef struct {
    long session;
} v1_sink_ctx_t;

/* Sink que la VM invoca para cada chunk de output del programa BP.
 * Cada chunk se envía como un evento OUTPUT con escape JSON. Chunks
 * pequeños generan eventos pequeños; el USB CDC del Pico tolera bien
 * muchos writes cortos. */
static void v1_output_sink(const char* data, size_t len, void* user) {
    v1_sink_ctx_t* ctx = (v1_sink_ctx_t*) user;
    fputs("{\"type\":\"OUTPUT\",\"session\":", stdout);
    fprintf(stdout, "%ld,\"data\":\"", ctx->session);
    for (size_t i = 0; i < len; i++) {
        char c = data[i];
        switch (c) {
            case '"':  fputs("\\\"", stdout); break;
            case '\\': fputs("\\\\", stdout); break;
            case '\n': fputs("\\n",  stdout); break;
            case '\r': fputs("\\r",  stdout); break;
            case '\t': fputs("\\t",  stdout); break;
            default:
                if ((unsigned char) c < 0x20) {
                    fprintf(stdout, "\\u%04x", (unsigned) c);
                } else {
                    fputc(c, stdout);
                }
                break;
        }
    }
    fputs("\"}\n", stdout);
    fflush(stdout);
}

/* Mapea bpvm_status_t a (status_str, exitCode) del wire v1. */
static void map_vm_status(bpvm_status_t rs, const char** status, int* exit_code) {
    if (rs == BPVM_OK) {
        *status    = "OK";
        *exit_code = 0;
    } else {
        *status    = "RUNTIME_ERROR";
        *exit_code = (int) rs;
    }
}

static void handle_run(long id, const json_obj_t* obj) {
    char path[FS_NAME_LEN];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta 'path'");
        return;
    }
    if (s_active_session != 0) {
        wire_v1_send_error(id, "BUSY", "ya hay una sesión RUN en curso");
        return;
    }

    /* 1. Resolver el módulo principal en el FS. */
    const uint8_t* data; uint32_t size;
    fs_status_t fs_s = v1_get_resolve(path, &data, &size);
    if (fs_s != FS_OK) {
        const char* code; const char* msg;
        map_fs_status(fs_s, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }

    /* 2. Asignar sessionId y mandar RUN_REPLY antes de empezar la
     *    ejecución — así el cliente sabe que su petición fue aceptada
     *    y empieza a esperar OUTPUT events. */
    long session = s_next_session++;
    s_active_session = session;
    {
        int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0,
                                      "RUN_REPLY", id);
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "session", session);
        if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf),
                                              (size_t) off);
        if (off >= 0) wire_v1_send_line(s_reply_buf, (size_t) off);
    }

    /* 3. Init VM + cargar módulo + resolver deps. */
    bpvm_t* vm = bpvm_init(s_vm_buffer, s_vm_buffer_size, 0);
    if (!vm) {
        /* No podemos mandar RUN_REPLY de error porque ya enviamos el
         * RUN_REPLY positivo. Emitimos EXITED con código de error. */
        fputs("{\"type\":\"EXITED\",\"session\":", stdout);
        fprintf(stdout, "%ld,\"status\":\"INTERNAL_ERROR\",\"exitCode\":-1,"
                        "\"errorMessage\":\"bpvm_init failed\"}\n", session);
        fflush(stdout);
        s_active_session = 0;
        return;
    }

    v1_sink_ctx_t sink_ctx = { session };
    bpvm_set_output(vm, v1_output_sink, &sink_ctx);

    bpvm_status_t ls = bpvm_load_mod_buffer(vm, data, size, path);
    if (ls != BPVM_OK) {
        fputs("{\"type\":\"EXITED\",\"session\":", stdout);
        fprintf(stdout, "%ld,\"status\":\"RUNTIME_ERROR\",\"exitCode\":%d,"
                        "\"errorMessage\":\"load: %s\"}\n",
                session, (int) ls, bpvm_status_str(ls));
        fflush(stdout);
        bpvm_destroy(vm);
        s_active_session = 0;
        return;
    }

    /* 4. Resolución iterativa de deps (mismo loop que cmd_run legacy). */
    for (int pass = 0; pass < 4; pass++) {
        int loaded_any = 0;
        int n_before = vm->module_count;
        for (int mi = 0; mi < n_before; mi++) {
            bpvm_module_t* m = &vm->modules[mi];
            for (int k = 0; k < m->import_count; k++) {
                const char* imp = m->imports[k];
                if (!imp || !imp[0]) continue;
                char owner[40]; size_t ol = 0;
                while (imp[ol] && imp[ol] != '.' && ol < sizeof(owner) - 1) {
                    owner[ol] = imp[ol]; ol++;
                }
                owner[ol] = '\0';
                if (!owner[0]) continue;
                int already = 0;
                for (int j = 0; j < vm->module_count; j++) {
                    if (strcmp(vm->modules[j].name, owner) == 0) {
                        already = 1; break;
                    }
                }
                if (already) continue;
                char fname[48];
                snprintf(fname, sizeof(fname), "%s.mod", owner);
                const uint8_t* dep; uint32_t dep_size;
                if (v1_get_resolve(fname, &dep, &dep_size) != FS_OK) continue;
                bpvm_status_t ds = bpvm_load_mod_buffer(vm, dep, dep_size, owner);
                if (ds != BPVM_OK) {
                    fputs("{\"type\":\"EXITED\",\"session\":", stdout);
                    fprintf(stdout, "%ld,\"status\":\"RUNTIME_ERROR\",\"exitCode\":%d,"
                                    "\"errorMessage\":\"dep %s: %s\"}\n",
                            session, (int) ds, fname, bpvm_status_str(ds));
                    fflush(stdout);
                    bpvm_destroy(vm);
                    s_active_session = 0;
                    return;
                }
                loaded_any = 1;
            }
        }
        if (!loaded_any) break;
    }

    /* 5. Ejecutar. Bloquea hasta que el programa termina. Cada print
     *    del programa pasa por v1_output_sink → genera un OUTPUT event. */
    uint32_t t0 = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    log_printf("RUN/v1 %s session=%ld", path, session);
    bpvm_status_t rs = bpvm_run(vm);
    uint32_t dt = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) - t0;
    log_printf("RUN/v1 %s finished: %s", path, bpvm_status_str(rs));

    /* 6. Emit EXITED. */
    const char* status_str; int exit_code;
    map_vm_status(rs, &status_str, &exit_code);
    int off = wire_v1_msg_begin_event(s_reply_buf, sizeof(s_reply_buf), 0, "EXITED");
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "session", session);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                               (size_t) off, "status", status_str);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "exitCode", exit_code);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "elapsedMs", (long) dt);
    if (rs != BPVM_OK) {
        if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                                   (size_t) off, "errorMessage",
                                                   bpvm_status_str(rs));
    }
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf),
                                          (size_t) off);
    if (off >= 0) wire_v1_send_line(s_reply_buf, (size_t) off);

    bpvm_destroy(vm);
    s_active_session = 0;
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
    if (strcmp(type, "INFO")     == 0) { handle_info(id, &obj);     return; }
    if (strcmp(type, "TIME")     == 0) { handle_time(id, &obj);     return; }
    if (strcmp(type, "PING")     == 0) { handle_ping(id, &obj);     return; }
    if (strcmp(type, "RESET")    == 0) { handle_reset(id, &obj);    return; }
    if (strcmp(type, "BOOTSEL")  == 0) { handle_bootsel(id, &obj);  return; }
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
    /* TERMINAL */
    if (strcmp(type, "RUN")      == 0) { handle_run(id, &obj);      return; }
    /* KILL: requeriría interrumpir bpvm_run desde otra task. La VM C
     * actual no expone un mecanismo de cancelación; #136 (arch-tasks)
     * lo desbloqueará al separar VM y REPL en tasks independientes.
     * Por ahora rechazamos limpiamente. */
    if (strcmp(type, "KILL")     == 0) {
        wire_v1_send_error(id, "UNSUPPORTED",
                            "KILL no soportado todavía en el firmware Pico");
        return;
    }
    /* PROMPT_RESPONSE: el builtin IO.prompt() aún no está implementado
     * en la VM C, así que nunca emitimos PROMPT_REQUEST. Si llega un
     * RESPONSE huérfano, ack silente. */
    if (strcmp(type, "PROMPT_RESPONSE") == 0) {
        wire_v1_send_reply_empty("PROMPT_RESPONSE_REPLY", id);
        return;
    }

    /* Fase D-E: META resto (INFO/TIME/PING/RESET/BOOTSEL), DEBUG. */
    wire_v1_send_error(id, "UNSUPPORTED",
                        "type no implementado en este firmware");
}
