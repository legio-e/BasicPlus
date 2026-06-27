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
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */
#include "log.h"
#include "aot_funcs.h"       /* H3 #160: registro AOT manual antes de run */
#include "mdn_loader.h"      /* H3 #158 fase D: cargar .mdn desde FS */

#include "bpvm.h"
#include "bpvm_internal.h"   /* inspect deps en handle_run */
#include "bpvm_pico.h"       /* INFO: uniqueId/boardName/temp/freq/uptime */
#include "board_desc.h"      /* INFO: variante/gpio/flash/psram del board_desc */
#include "bpvm_rtc.h"        /* TIME: set epoch */
#include "aot_registry.h"    /* H3 #160: bpvm_aot_clear */

#include "pico/bootrom.h"    /* reset_usb_boot (BOOTSEL) */
#include "pico/stdlib.h"     /* getchar_timeout_us (poll de KILL, #257) */

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Buffer VM compartido (declarado en main.c). */
extern uint8_t* s_vm_buffer;          /* H7.2.b: SRAM interna o ventana PSRAM */
extern uint32_t s_vm_buffer_size;

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
/* DEBUGGER del device (H6.b.3 / #140) — pause_cb + comandos wire v1.
 *
 * El núcleo portable (#215: bpvm_debug_*, pause_cb, accessors de frame)
 * hace el trabajo; aquí está SÓLO el transporte Pico. Cuando un
 * breakpoint pausa, el MISMO hilo del REPL (camino single-thread
 * bpvm_run) atiende inline READ_INT/READ_STRING/LOCALS/STACK/SET_BP/
 * CLR_BP/CONTINUE/STEP/STOP hasta reanudar — sin cond-var ni separación
 * de tasks. Es la misma lógica que el server host (test/debug_listen.c),
 * ya verificada por dbg_client.py y el oráculo Java<->C
 * (DeviceWireOracleSmoke). Pendiente: flash+test en placa.
 *
 * NOTA SMP: el pause_cb lee USB; eso sólo es seguro en la task que ya es
 * dueña de stdin (la del REPL). Por eso handle_run fuerza bpvm_run
 * (single-thread) cuando hay sesión de debug, aunque el build sea SMP. */

/* Breakpoints fijados ANTES de RUN: la vm se crea por-RUN, así que se
 * acumulan y se aplican al arrancar. */
static uint32_t s_pending_bp_pc[BPVM_MAX_BREAKPOINTS];
static int      s_pending_bp_id[BPVM_MAX_BREAKPOINTS];
static int      s_pending_bp_n  = 0;
static int      s_bp_id_seq     = 1;     /* ids provisionales pre-RUN */
static int      s_pause_initial = 0;     /* PAUSE pre-RUN → romper en 1er opcode */
static bpvm_t*  s_dbg_vm        = NULL;  /* vm activa (no-NULL ⇒ depurando) */
static long     s_dbg_session   = 0;
/* Snapshot del frame pausado (para LOCALS/STACK/READ mientras pausados). */
static uint32_t s_pf_pc, s_pf_sp, s_pf_bp, s_pf_cs, s_pf_sbase;

/* SET_BP{pc}: live si hay vm (paused), si no acumula pre-RUN. */
static void dbg_set_bp(long id, const json_obj_t* obj) {
    long pc = json_get_long(obj, "pc", -1);
    int bpId = -1;
    if (s_dbg_vm) {
        if (pc >= 0) bpId = bpvm_debug_add_breakpoint(s_dbg_vm, (uint32_t) pc);
    } else if (pc >= 0 && s_pending_bp_n < BPVM_MAX_BREAKPOINTS) {
        bpId = s_bp_id_seq++;
        s_pending_bp_pc[s_pending_bp_n] = (uint32_t) pc;
        s_pending_bp_id[s_pending_bp_n] = bpId;
        s_pending_bp_n++;
    }
    int off = snprintf(s_reply_buf, sizeof s_reply_buf,
        "{\"type\":\"SET_BP_REPLY\",\"id\":%ld,\"bpId\":%d}", id, bpId);
    if (off > 0) wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* CLR_BP{bpId}: bpId<0 ⇒ limpiar todos. */
static void dbg_clr_bp(long id, const json_obj_t* obj) {
    long bpId = json_get_long(obj, "bpId", -1);
    if (s_dbg_vm) {
        if (bpId >= 0) bpvm_debug_clear_breakpoint(s_dbg_vm, (int) bpId);
        else           bpvm_debug_clear_breakpoints(s_dbg_vm);
    } else if (bpId < 0) {
        s_pending_bp_n = 0;
    } else {
        for (int i = 0; i < s_pending_bp_n; i++) {
            if (s_pending_bp_id[i] == (int) bpId) {
                for (int j = i + 1; j < s_pending_bp_n; j++) {
                    s_pending_bp_pc[j-1] = s_pending_bp_pc[j];
                    s_pending_bp_id[j-1] = s_pending_bp_id[j];
                }
                s_pending_bp_n--; break;
            }
        }
    }
    int off = snprintf(s_reply_buf, sizeof s_reply_buf,
        "{\"type\":\"CLR_BP_REPLY\",\"id\":%ld}", id);
    if (off > 0) wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* READ_INT{addr}: i32 crudo en dirección absoluta. */
static void dbg_read_int(long id, const json_obj_t* obj) {
    long addr = json_get_long(obj, "addr", -1);
    int32_t v = (s_dbg_vm && addr >= 0) ? bpvm_mem_read_i32(s_dbg_vm, (uint32_t) addr) : 0;
    int off = snprintf(s_reply_buf, sizeof s_reply_buf,
        "{\"type\":\"READ_INT_REPLY\",\"id\":%ld,\"value\":%ld}", id, (long) v);
    if (off > 0) wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* READ_STRING{ref}: string heap = [byte_len:u32 BE][bytes UTF-8]; ref 0 = "". */
static void dbg_read_string(long id, const json_obj_t* obj) {
    long ref = json_get_long(obj, "ref", 0);
    int off = snprintf(s_reply_buf, sizeof s_reply_buf,
        "{\"type\":\"READ_STRING_REPLY\",\"id\":%ld,\"value\":\"", id);
    if (off < 0) return;
    if (s_dbg_vm && ref > 0) {
        uint32_t blen = bpvm_mem_read_u32(s_dbg_vm, (uint32_t) ref);
        const uint8_t* b = s_dbg_vm->memory + ref + 4;
        for (uint32_t i = 0; i < blen && off < (int) sizeof s_reply_buf - 8; i++) {
            unsigned char c = b[i];
            if (c == '"' || c == '\\') { s_reply_buf[off++] = '\\'; s_reply_buf[off++] = (char) c; }
            else if (c == '\n')        { s_reply_buf[off++] = '\\'; s_reply_buf[off++] = 'n'; }
            else if (c == '\t')        { s_reply_buf[off++] = '\\'; s_reply_buf[off++] = 't'; }
            else if (c >= 0x20)          s_reply_buf[off++] = (char) c;  /* incl. UTF-8 >=0x80 */
        }
    }
    off += snprintf(s_reply_buf + off, sizeof s_reply_buf - off, "\"}");
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* LOCALS: i32 crudos entre bp y sp del frame pausado (el host resuelve
 * nombres con el .dbg). */
static void dbg_locals(long id) {
    int off = snprintf(s_reply_buf, sizeof s_reply_buf,
        "{\"type\":\"LOCALS_REPLY\",\"id\":%ld,\"locals\":[", id);
    if (s_dbg_vm && s_pf_sp > s_pf_bp) {
        int nl = (int) ((s_pf_sp - s_pf_bp) / 4);
        for (int i = 0; i < nl && off < (int) sizeof s_reply_buf - 24; i++)
            off += snprintf(s_reply_buf + off, sizeof s_reply_buf - off, "%s%ld",
                            i ? "," : "",
                            (long) bpvm_mem_read_i32(s_dbg_vm, s_pf_bp + i * 4));
    }
    off += snprintf(s_reply_buf + off, sizeof s_reply_buf - off, "]}");
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* STACK: walk de frames (saved pc en bp-12, saved bp en bp-8 = VM Java). */
static void dbg_stack(long id) {
    int off = snprintf(s_reply_buf, sizeof s_reply_buf,
        "{\"type\":\"STACK_REPLY\",\"id\":%ld,\"frames\":[", id);
    if (s_dbg_vm) {
        uint32_t cbp = s_pf_bp, cpc = s_pf_pc;
        int first = 1, safety = 0;
        while (cbp > s_pf_sbase && safety < 256 && off < (int) sizeof s_reply_buf - 48) {
            off += snprintf(s_reply_buf + off, sizeof s_reply_buf - off, "%s[%lu,%lu]",
                            first ? "" : ",", (unsigned long) cpc, (unsigned long) cbp);
            first = 0;
            cpc = bpvm_mem_read_u32(s_dbg_vm, cbp - 12);
            cbp = bpvm_mem_read_u32(s_dbg_vm, cbp - 8);
            safety++;
        }
        off += snprintf(s_reply_buf + off, sizeof s_reply_buf - off, "%s[%lu,%lu]",
                        first ? "" : ",", (unsigned long) cpc, (unsigned long) cbp);
    }
    off += snprintf(s_reply_buf + off, sizeof s_reply_buf - off, "]}");
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* pause_cb: snapshot del frame, emite BP_HIT y atiende comandos inline
 * hasta CONTINUE/STEP/STOP. Corre en la task del REPL (single-thread). */
static bpvm_dbg_action_t pico_pause_cb(bpvm_t* vm, bpvm_thread_t* tc,
                                       uint32_t pc, void* user) {
    (void) vm; (void) user;
    s_pf_pc = pc;
    s_pf_sp = bpvm_thread_sp(tc);
    s_pf_bp = bpvm_thread_bp(tc);
    s_pf_cs = bpvm_thread_cs(tc);
    s_pf_sbase = tc->stack_base;

    int off = snprintf(s_reply_buf, sizeof s_reply_buf,
        "{\"type\":\"BP_HIT\",\"session\":%ld,\"tid\":%d,"
        "\"pc\":%lu,\"sp\":%lu,\"bp\":%lu,\"cs\":%lu}",
        s_dbg_session, bpvm_thread_id(tc),
        (unsigned long) pc, (unsigned long) s_pf_sp,
        (unsigned long) s_pf_bp, (unsigned long) s_pf_cs);
    if (off > 0) wire_v1_send_line(s_reply_buf, (size_t) off);

    for (;;) {
        int n = wire_v1_recv_line(-1, s_line_buf, sizeof s_line_buf);
        if (n < 0) continue;                       /* overflow: ignora */
        json_obj_t obj;
        if (json_parse(s_line_buf, (size_t) n, &obj) != 0) continue;
        long id = json_get_long(&obj, "id", 0);
        char type[40];
        if (json_get_str(&obj, "type", type, sizeof type) < 0) continue;

        if      (strcmp(type, "CONTINUE") == 0) return BPVM_DBG_CONTINUE;
        else if (strcmp(type, "STEP")     == 0) return BPVM_DBG_STEP;
        else if (strcmp(type, "STOP") == 0 || strcmp(type, "KILL") == 0)
            return BPVM_DBG_STOP;
        else if (strcmp(type, "PING") == 0) {
            int o = snprintf(s_reply_buf, sizeof s_reply_buf,
                "{\"type\":\"PONG\",\"id\":%ld}", id);
            if (o > 0) wire_v1_send_line(s_reply_buf, (size_t) o);
        }
        else if (strcmp(type, "SET_BP")      == 0) dbg_set_bp(id, &obj);
        else if (strcmp(type, "CLR_BP")      == 0) dbg_clr_bp(id, &obj);
        else if (strcmp(type, "READ_INT")    == 0) dbg_read_int(id, &obj);
        else if (strcmp(type, "READ_STRING") == 0) dbg_read_string(id, &obj);
        else if (strcmp(type, "LOCALS")      == 0) dbg_locals(id);
        else if (strcmp(type, "STACK")       == 0) dbg_stack(id);
        else wire_v1_send_error(id, "UNSUPPORTED", "no valido en pausa");
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
    static const char* CAPS = ",\"capabilities\":[\"META\",\"FILES\",\"TERMINAL\",\"DEBUG\",\"BOOTSEL\"]";
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
    /* paso 4 cierre — CRC del contenido (== java.util.zip.CRC32) para que el
     * IDE salte el PUT por contenido REAL del device (fs_get = lookup barato). */
    uint32_t crc = 0;
    const uint8_t* d; uint32_t sz;
    if (fs_get(name, &d, &sz) == FS_OK) crc = bpvm_crc32(d, sz);
    fprintf(stdout, ",\"size\":%u,\"crc\":%u,\"isDir\":false}", (unsigned) size, (unsigned) crc);
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

/* Borra el log RAM + flash. Útil para bisects de instrumentación —
 * partir de 0 y ver inequívocamente qué persiste tras el siguiente
 * intento. NO reinicia el firmware. */
static void handle_log_clear(long id, const json_obj_t* obj) {
    (void) obj;
    log_clear_ram();
    log_clear_flash();
    log_printf("LOG cleared via wire v1");
    /* No log_flush() — dejamos el "cleared" en RAM. El siguiente
     * flush natural lo persistirá si interesa. */
    wire_v1_send_reply_empty("LOG_CLEAR_REPLY", id);
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
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                               (size_t) off, "resetReason", bpvm_pico_reset_cause());
    /* tempC: el wire v1 NO soporta floats (parser del cliente rechaza
     * decimales/científica). Enviamos como entero en milidegrees → el
     * cliente divide por 1000 para mostrar con precisión de display.
     * "tempMilliC":25430 → 25.43 °C en la UI. */
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "tempMilliC",
                                             (long)(tempC * 1000.0f));
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "fsTotalBytes", fsTotal);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "fsUsedBytes", fsUsed);
    /* H7 — descriptor de placa: variante, caps del chip, flash y PSRAM. */
    const board_desc_t* bd = board_desc();
    char variant[2]; variant[0] = bd->variant; variant[1] = '\0';
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                               (size_t) off, "variant", variant);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "gpioCount", bd->gpio_count);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "pioCount", bd->pio_count);
    /* PWM en SALIDAS, no slices: cada slice tiene 2 canales (A/B) → 24
     * en RP2350, que es la cifra que anuncian las placas. El campo del
     * wire conserva el nombre histórico "pwmSlices". */
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "pwmSlices", bd->pwm_slices * 2);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "adcChannels", bd->adc_channels);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "flashBytes", (long) bd->flash_bytes);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "sramBytes", 520L * 1024L);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "psramBytes", (long) bd->psram_bytes);
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
 * muchos writes cortos.
 *
 * El campo `stream:"stdout"` está por simetría con DebugServer (Java
 * VM) y por extensibilidad — wire v1 §6.3 lo contempla por si en
 * algún momento separamos stderr del programa. */
static void v1_output_sink(const char* data, size_t len, void* user) {
    v1_sink_ctx_t* ctx = (v1_sink_ctx_t*) user;
    /* #256 — la línea OUTPUT se construye con varios fputs/fprintf:
     * el lock cubre la línea ENTERA para que el poll (HELLO/BUSY en-run)
     * no pueda intercalar su reply a mitad. */
    wire_v1_tx_lock();
    fputs("{\"type\":\"OUTPUT\",\"session\":", stdout);
    fprintf(stdout, "%ld,\"stream\":\"stdout\",\"data\":\"", ctx->session);
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
    wire_v1_tx_unlock();
}

/* Mapea bpvm_status_t a (status_str, exitCode) del wire v1. */
static void map_vm_status(bpvm_status_t rs, const char** status, int* exit_code) {
    if (rs == BPVM_OK) {
        *status    = "OK";
        *exit_code = 0;
    } else if (rs == BPVM_KILLED) {
        /* P-run-stop (#257) — abortado por KILL del IDE. exitCode 130
         * (convención 128+SIGINT); el cliente lo distingue de un error. */
        *status    = "KILLED";
        *exit_code = 130;
    } else {
        *status    = "RUNTIME_ERROR";
        *exit_code = (int) rs;
    }
}

/* ============================================================ */
/* P-run-stop (#257) + P-autorun (#256) — wire durante el run.   */
/*                                                               */
/* La VM invoca este poll ENTRE quanta (bpvm_set_poll), en la    */
/* task coordinadora (la misma que llamó a handle_run — la VM    */
/* corre en el worker). Lee el wire sin bloquear; si llega una   */
/* línea completa la consume:                                    */
/*   KILL  → marca el ack pendiente y devuelve 1 (BPVM_KILLED).  */
/*           El KILL_REPLY sale tras parar la VM, antes del      */
/*           EXITED (orden estable para el cliente).             */
/*   HELLO → HELLO_REPLY inmediato. Es la pieza que permite al   */
/*           IDE CONECTARSE con un (auto)run en marcha y ofrecer */
/*           Stop (#256: sin esto, un autorun infinito dejaría   */
/*           la placa inalcanzable salvo reflash).               */
/*   otra  → error BUSY inmediato (hasta #256 era diferido a fin */
/*           de run: el explorer se comía el timeout).           */
/* Las replies en caliente son seguras: cada línea del wire es   */
/* atómica por el tx_lock (#256) — la comm task con sus OUTPUTs  */
/* y este poll ya no pueden entrelazarse. s_reply_buf también es */
/* seguro: su dueño (esta misma task) está bloqueado en bpvm_run */
/* mientras el poll corre.                                       */
/* ============================================================ */
static long s_kill_ack_id = -1;   /* id del KILL recibido en-run, o -1 */

static int pico_run_poll_cb(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    int c = getchar_timeout_us(0);
    if (c < 0) return 0;                      /* nada pendiente */
    int n = wire_v1_recv_line(c, s_line_buf, sizeof(s_line_buf));
    if (n < 0) return 0;                      /* línea rota: descartar */
    json_obj_t obj;
    if (json_parse(s_line_buf, (size_t) n, &obj) != 0) return 0;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof(type));
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) {
        s_kill_ack_id = rid;
        return 1;                              /* → BPVM_KILLED */
    }
    if (strcmp(type, "HELLO") == 0) {
        handle_hello(rid, &obj);               /* attach en caliente */
        return 0;
    }
    wire_v1_send_error(rid, "BUSY", "ejecución en curso: solo HELLO/KILL");
    return 0;
}

/* Núcleo del RUN — compartido entre el comando RUN del wire (id >= 0)
 * y el autorun de boot (#256, id < 0). Con id < 0 no hay cliente: se
 * omite el RUN_REPLY y los errores de resolución van al log persistente
 * en vez de al wire. Todo lo demás (sesión, OUTPUT events, poll de
 * KILL/HELLO, EXITED) es idéntico — un autorun ES un run normal. */
static void run_module_path(const char* path, long id) {
    if (s_active_session != 0) {
        if (id >= 0) wire_v1_send_error(id, "BUSY", "ya hay una sesión RUN en curso");
        else         log_printf("autorun: BUSY (sesión activa) — ignorado");
        return;
    }

    /* 1. Resolver el módulo principal en el FS. */
    const uint8_t* data; uint32_t size;
    fs_status_t fs_s = v1_get_resolve(path, &data, &size);
    if (fs_s != FS_OK) {
        const char* code; const char* msg;
        map_fs_status(fs_s, &code, &msg);
        if (id >= 0) wire_v1_send_error(id, code, msg);
        else         log_printf("autorun: %s: %s — REPL normal", path, msg);
        return;
    }

    /* 2. Asignar sessionId y mandar RUN_REPLY antes de empezar la
     *    ejecución — así el cliente sabe que su petición fue aceptada
     *    y empieza a esperar OUTPUT events. */
    long session = s_next_session++;
    s_active_session = session;
    if (id >= 0) {
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

    /* 4b. H3 #160 — registrar funciones AOT manualmente. Tras link,
     *     la global symbol table tiene "Bench.fib" si Bench.mod cargó;
     *     aot_funcs_register hace lookup y registra el thunk. Tolerante
     *     a símbolos ausentes (si no hay Bench.fib, no-op silencioso). */
    bpvm_aot_clear();
    aot_funcs_register(vm);

    /* 4c. H3 #158 fase D — para cada módulo cargado, buscar su .mdn
     *     correspondiente en el FS y, si existe, registrar sus thunks
     *     (zero-copy — apuntando al buffer FS). El registry queda con
     *     la versión .mdn más reciente para los símbolos en cuestión. */
    log_printf("AOT/FS: scanning %d modules for .mdn", vm->module_count);
    for (int mi = 0; mi < vm->module_count; mi++) {
        const char* mname = vm->modules[mi].name;
        if (!mname || !mname[0]) continue;
        char mdn_path[48];
        snprintf(mdn_path, sizeof(mdn_path), "%s.mdn", mname);
        const uint8_t* mdn_data; uint32_t mdn_size;
        fs_status_t fs_s = v1_get_resolve(mdn_path, &mdn_data, &mdn_size);
        if (fs_s != FS_OK) {
            log_printf("AOT/FS: %s not found (fs=%d) — sin overlay", mdn_path, (int) fs_s);
            continue;
        }
        int rc = bpvm_load_mdn(vm, mdn_data, (size_t) mdn_size);
        if (rc == MDN_OK) {
            log_printf("AOT/FS: %s loaded from FS (%u bytes) buf=%p",
                       mdn_path, (unsigned) mdn_size, (const void*) mdn_data);
        } else {
            log_printf("AOT/FS: %s load failed rc=%d", mdn_path, rc);
        }
    }
    log_printf("AOT/FS: scan done, about to bpvm_run");
    log_flush();   /* CHECKPOINT — si vemos hasta aquí, fase D loaded
                    * correctamente. Lo siguiente que crashee es la
                    * ejecución del thunk desde el buffer del FS. */

    /* 5. Ejecutar. Bloquea hasta que el programa termina. Cada print
     *    del programa pasa por v1_output_sink → genera un OUTPUT event.
     *
     * H2 Pico — Si BPVM_PICO_SMP_WORKERS está definido y ≥1, usamos
     * el scheduler multi-worker (bpvm_run_smp). Con N=1 ejercitamos
     * TODA la maquinaria SMP (worker + comm task + queue + STW dance)
     * sin paralelismo — validación safe sin riesgo de race. Con N=2
     * activamos paralelismo real (un worker por core del RP2350 cuando
     * #153 P-smp-tx-exclusive cierre el pinning).
     *
     * Sin el define (default actual), seguimos en el camino legacy
     * single-thread — el cambio del runtime SMP NO afecta a usuarios
     * que no opten in. */
    /* H6.b.3 #140 — modo debug: si el cliente fijó breakpoints o pidió
     * PAUSE antes de RUN, aplicar los pendientes + enganchar el pause_cb. */
    int debugging = (s_pending_bp_n > 0 || s_pause_initial);
    if (debugging) {
        for (int i = 0; i < s_pending_bp_n; i++)
            bpvm_debug_add_breakpoint(vm, s_pending_bp_pc[i]);
        bpvm_set_pause_cb(vm, pico_pause_cb, NULL);
        s_dbg_vm = vm;
        s_dbg_session = session;
        if (s_pause_initial) bpvm_debug_request_pause(vm);
        log_printf("RUN/v1: DEBUG mode (bps=%d pauseInit=%d)",
                   s_pending_bp_n, s_pause_initial);
    }

    /* P-run-stop (#257) — poll del wire entre quanta para poder atender
     * KILL (y desde #256, HELLO/BUSY en caliente). Solo en runs normales:
     * en modo debug el pause_cb ya es el dueño del USB (dos lectores se
     * robarían bytes). */
    s_kill_ack_id = -1;
    if (!debugging) bpvm_set_poll(vm, pico_run_poll_cb, NULL);

    uint32_t t0 = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    log_printf("RUN/v1 %s session=%ld", path, session);
    bpvm_status_t rs;
#if defined(BPVM_PICO_SMP_WORKERS) && BPVM_PICO_SMP_WORKERS >= 1
    if (debugging) {
        /* El pause_cb lee USB → sólo seguro en la task dueña de stdin
         * (la del REPL). Forzamos single-thread aunque el build sea SMP. */
        rs = bpvm_run(vm);
    } else {
        log_printf("RUN/v1: SMP path n_workers=%d", BPVM_PICO_SMP_WORKERS);
        rs = bpvm_run_smp(vm, BPVM_PICO_SMP_WORKERS);
    }
#else
    rs = bpvm_run(vm);
#endif
    uint32_t dt = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) - t0;
    log_printf("RUN/v1 %s finished: %s", path, bpvm_status_str(rs));

    /* P-run-stop — ack diferido del KILL. Orden: KILL_REPLY → EXITED. */
    bpvm_set_poll(vm, NULL, NULL);
    if (s_kill_ack_id >= 0) {
        wire_v1_send_reply_empty("KILL_REPLY", s_kill_ack_id);
        s_kill_ack_id = -1;
    }

    /* 6. Emit EXITED. */
    const char* status_str; int exit_code;
    map_vm_status(rs, &status_str, &exit_code);
    const char* link_err = bpvm_link_error(vm);   /* paso 4 — "" salvo fallo de link */
    if (link_err[0]) status_str = "LINK_ERROR";
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
                                                   link_err[0] ? link_err : bpvm_status_str(rs));
    }
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf),
                                          (size_t) off);
    if (off >= 0) wire_v1_send_line(s_reply_buf, (size_t) off);

    bpvm_destroy(vm);
    s_active_session = 0;
    /* H6.b.3 — limpiar estado de debug de esta sesión. */
    s_dbg_vm = NULL;
    s_pending_bp_n = 0;
    s_pause_initial = 0;
}

static void handle_run(long id, const json_obj_t* obj) {
    char path[FS_NAME_LEN];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta 'path'");
        return;
    }
    run_module_path(path, id);
}

/* ============================================================ */
/* P-autorun (#256) — arranque autónomo desde /sys/auto.txt.     */
/*                                                               */
/* main.c lo invoca tras FS + stdlib + board, justo ANTES del    */
/* repl_run() — el wire ya está operativo y el poll del run      */
/* atiende HELLO/KILL, así que la placa nunca queda sorda: el    */
/* IDE puede conectarse con el autorun corriendo y pararlo.      */
/* Formato del fichero: primera línea = ruta del módulo (p.ej.   */
/* "/app/MiApp.mod"); espacios y CR/LF tolerados; vacío o ruta   */
/* inexistente → log + REPL normal (nunca un boot-loop).         */
/* Vías de escape: Stop en el IDE → borrar /sys/auto.txt         */
/* (comando `autorun off` de la consola) → reset.                */
/* ============================================================ */
void repl_v1_autorun(void) {
    const uint8_t* data; uint32_t size;
    if (fs_get("/sys/auto.txt", &data, &size) != FS_OK) return;  /* sin autorun */

    char path[FS_NAME_LEN];
    size_t n = 0, i = 0;
    while (i < size && (data[i] == ' ' || data[i] == '\t')) i++;
    while (i < size && data[i] != '\n' && data[i] != '\r'
           && n + 1 < sizeof(path)) {
        path[n++] = (char) data[i++];
    }
    while (n > 0 && (path[n - 1] == ' ' || path[n - 1] == '\t')) n--;
    path[n] = '\0';
    if (n == 0) {
        log_printf("autorun: /sys/auto.txt vacío — REPL normal");
        return;
    }
    log_printf("autorun: %s", path);
    /* Gracia de arranque: el camino del RUN hace log_flush (erase de
     * flash con IRQs off) y la app puede tocar flash también — si eso
     * coincide con la ENUMERACIÓN USB del host, Windows da el puerto
     * por muerto ("dispositivo desconocido"). 2 s dejan a TinyUSB
     * terminar la enumeración antes de arrancar. Boots sin auto.txt
     * no pagan nada (return arriba). */
    vTaskDelay(pdMS_TO_TICKS(2000));
    run_module_path(path, -1);
    log_printf("autorun: terminado — REPL normal");
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
    if (strcmp(type, "LOG_CLEAR")== 0) { handle_log_clear(id, &obj); return; }
    /* TERMINAL */
    if (strcmp(type, "RUN")      == 0) { handle_run(id, &obj);      return; }
    /* DEBUG (H6.b.3 #140) — pre-RUN: acumular breakpoints / pedir pausa
     * inicial. Durante un RUN en modo debug, los comandos READ_INT,
     * READ_STRING, LOCALS, STACK, CONTINUE y STEP los atiende el
     * pico_pause_cb inline mientras la VM esta pausada. */
    if (strcmp(type, "PAUSE")    == 0) { s_pause_initial = 1;
                                         wire_v1_send_reply_empty("PAUSE_REPLY", id); return; }
    if (strcmp(type, "SET_BP")   == 0) { dbg_set_bp(id, &obj);      return; }
    if (strcmp(type, "CLR_BP")   == 0) { dbg_clr_bp(id, &obj);      return; }
    /* P-run-stop (#257) — KILL en idle: no hay nada que matar. El KILL
     * útil llega DURANTE un RUN y lo atiende pico_run_poll_cb (la VM
     * polea el wire entre quanta y termina con BPVM_KILLED). */
    if (strcmp(type, "KILL")     == 0) {
        wire_v1_send_error(id, "NO_SESSION",
                            "no hay programa en ejecución");
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
