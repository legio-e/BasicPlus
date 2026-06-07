/*
 * stm32_repl.c — REPL wire v1 bare-metal (H9.2).
 *
 * H9.2.a: HELLO/INFO/TIME/PING/RESET + LIST/DF/LOG → el IDE conecta.
 * H9.2.b: + FS en RAM (PUT/GET/DEL/STAT/LIST real/DF real/MKDIR/FORMAT) y
 *         RUN (carga el .mod del FS, lo ejecuta, hace streaming de OUTPUT y
 *         emite EXITED) → "Run on STM32" completo.
 *
 * Single-thread, sin FreeRTOS. La salida del programa BP se reenvía como
 * eventos OUTPUT (bytes verbatim, escapados a JSON → paridad de contenido).
 * Limitación MVP: RUN carga UN módulo (sin resolución de imports todavía);
 * programas con `import` llegan en un paso posterior.
 */
#include "stm32_repl.h"
#include "stm32_wire.h"
#include "stm32_fs.h"
#include "json_min.h"
#include "bpvm.h"

#include "main.h"
#include "stm32u5xx_nucleo.h"   /* BSP_LED_Toggle, LED_GREEN */

#include <string.h>
#include <stdio.h>

#define SERVER_NAME "bpvm-stm32"
#define BOARD_NAME  "nucleo-u575zi"

/* Buffers estáticos (NO en stack: el stack C del micro es pequeño). */
static char    s_line[WIRE_LINE_MAX];
static uint8_t s_put_buf[32u * 1024u];        /* payload de PUT */
static uint8_t s_vm_mem[128u * 1024u];        /* RAM que gestiona la VM */
static char    s_out_esc[2048];               /* salida escapada (sink) */
static char    s_out_msg[2300];               /* evento OUTPUT completo */
static long    s_session = 0;                 /* contador de sesiones RUN */
static long    s_run_session = 0;             /* sesión activa (para el sink) */

/* ---- helpers de envío ---- */

static void reply_empty(const char* type, long id) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf), "{\"type\":\"%s\",\"id\":%ld}", type, id);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

/* ---- META ---- */

static void handle_hello(long id) {
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"HELLO_REPLY\",\"id\":%ld,\"protoVersion\":1,"
        "\"serverName\":\"%s\",\"serverBuild\":\"%s %s\","
        "\"capabilities\":[\"META\",\"FILES\",\"TERMINAL\"]}",
        id, SERVER_NAME, __DATE__, __TIME__);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_info(long id) {
    uint32_t u0 = *(volatile uint32_t*) (UID_BASE + 0U);
    uint32_t u1 = *(volatile uint32_t*) (UID_BASE + 4U);
    uint32_t u2 = *(volatile uint32_t*) (UID_BASE + 8U);
    char buf[320];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"INFO_REPLY\",\"id\":%ld,"
        "\"uniqueId\":\"%08lX%08lX%08lX\","
        "\"boardName\":\"%s\",\"cpuFreqHz\":%lu,\"uptimeMs\":%lu,\"tempC\":0,"
        "\"fsTotalBytes\":%lu,\"fsUsedBytes\":%lu}",
        id, (unsigned long) u2, (unsigned long) u1, (unsigned long) u0,
        BOARD_NAME, (unsigned long) SystemCoreClock, (unsigned long) HAL_GetTick(),
        (unsigned long) fs_total_bytes(), (unsigned long) fs_used_bytes());
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

/* ---- FILES ---- */

static void handle_list(long id, json_obj_t* obj) {
    char prefix[64];
    if (json_get_str(obj, "path", prefix, sizeof(prefix)) < 0) prefix[0] = '\0';
    size_t plen = strlen(prefix);

    char buf[1024];
    size_t o = 0;
    o += (size_t) snprintf(buf + o, sizeof(buf) - o,
        "{\"type\":\"LIST_REPLY\",\"id\":%ld,\"entries\":[", id);
    int first = 1;
    int cnt = fs_count();
    for (int i = 0; i < cnt; i++) {
        const char* name; uint32_t size;
        if (fs_entry(i, &name, &size) != 0) continue;
        if (plen > 0 && strncmp(name, prefix, plen) != 0) continue;
        const char* rel = name + plen;             /* basename tras el prefijo */
        if (*rel == '/') rel++;
        int w = snprintf(buf + o, sizeof(buf) - o,
            "%s{\"name\":\"%s\",\"size\":%lu,\"isDir\":false,\"mtime\":0}",
            first ? "" : ",", rel, (unsigned long) size);
        if (w < 0 || (size_t) w >= sizeof(buf) - o) break;   /* no cabe más */
        o += (size_t) w;
        first = 0;
    }
    o += (size_t) snprintf(buf + o, sizeof(buf) - o, "]}");
    stm32_wire_send_line(buf, o);
}

static void handle_stat(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    const uint8_t* d; uint32_t size;
    if (fs_get(path, &d, &size) != 0) {
        stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return;
    }
    char buf[128];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"STAT_REPLY\",\"id\":%ld,\"size\":%lu,\"isDir\":false,\"mtime\":0}",
        id, (unsigned long) size);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_df(long id) {
    uint32_t total = fs_total_bytes(), used = fs_used_bytes();
    char buf[160];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"DF_REPLY\",\"id\":%ld,\"totalBytes\":%lu,\"usedBytes\":%lu,"
        "\"freeBytes\":%lu,\"fileCount\":%d}",
        id, (unsigned long) total, (unsigned long) used,
        (unsigned long) (total - used), fs_count());
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_get(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    const uint8_t* d; uint32_t size;
    if (fs_get(path, &d, &size) != 0) {
        stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return;
    }
    char buf[96];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"GET_REPLY\",\"id\":%ld,\"bulk\":%lu}", id, (unsigned long) size);
    if (n > 0) {
        stm32_wire_send_line(buf, (size_t) n);
        stm32_wire_send_bulk(d, size);
    }
}

static void handle_del(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    if (fs_del(path) != 0) { stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return; }
    reply_empty("DEL_REPLY", id);
}

static void handle_put(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    long bulk = json_get_long(obj, "bulk", -1);
    if (bulk < 0) { stm32_wire_send_error(id, "INVALID_PARAM", "missing bulk"); return; }

    /* CRÍTICO: consumir SIEMPRE los `bulk` bytes para no desincronizar el wire. */
    if ((size_t) bulk > sizeof(s_put_buf)) {
        size_t rem = (size_t) bulk;
        while (rem > 0) {
            size_t chunk = rem < sizeof(s_put_buf) ? rem : sizeof(s_put_buf);
            if (stm32_wire_recv_bulk(s_put_buf, chunk) != 0) {
                stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
            }
            rem -= chunk;
        }
        stm32_wire_send_error(id, "NO_SPACE", "fichero demasiado grande");
        return;
    }
    if (stm32_wire_recv_bulk(s_put_buf, (size_t) bulk) != 0) {
        stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
    }
    if (fs_put(path, s_put_buf, (uint32_t) bulk) != 0) {
        stm32_wire_send_error(id, "NO_SPACE", "FS lleno"); return;
    }
    reply_empty("PUT_REPLY", id);
}

static void handle_format(long id, json_obj_t* obj) {
    char confirm[8];
    if (json_get_str(obj, "confirm", confirm, sizeof(confirm)) < 0 ||
        strcmp(confirm, "YES") != 0) {
        stm32_wire_send_error(id, "MISSING_CONFIRM", "confirm:\"YES\""); return;
    }
    fs_format();
    reply_empty("FORMAT_REPLY", id);
}

static void handle_log_dump(long id) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"LOG_DUMP_REPLY\",\"id\":%ld,\"text\":\"\"}", id);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

/* ---- TERMINAL: RUN + streaming ---- */

/* Cada PRINT_* de la VM llega aquí → evento OUTPUT con los bytes escapados. */
static void v1_output_sink(const char* s, size_t len, void* user) {
    (void) user;
    if (stm32_wire_json_escape(s, len, s_out_esc, sizeof(s_out_esc)) < 0) return;
    int n = snprintf(s_out_msg, sizeof(s_out_msg),
        "{\"type\":\"OUTPUT\",\"session\":%ld,\"stream\":\"stdout\",\"data\":\"%s\"}",
        s_run_session, s_out_esc);
    if (n > 0) stm32_wire_send_line(s_out_msg, (size_t) n);
}

static void emit_exited(long session, const char* status, int code, uint32_t ms) {
    char buf[200];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"%s\",\"exitCode\":%d,"
        "\"elapsedMs\":%lu}", session, status, code, (unsigned long) ms);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_run(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    const uint8_t* data; uint32_t size;
    if (fs_get(path, &data, &size) != 0) {
        stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return;
    }

    long session = ++s_session;
    s_run_session = session;
    { /* RUN_REPLY con la sesión, ANTES de ejecutar */
        char buf[80];
        int n = snprintf(buf, sizeof(buf),
            "{\"type\":\"RUN_REPLY\",\"id\":%ld,\"session\":%ld}", id, session);
        if (n > 0) stm32_wire_send_line(buf, (size_t) n);
    }

    bpvm_t* vm = bpvm_init(s_vm_mem, sizeof(s_vm_mem), 0);
    if (!vm) { emit_exited(session, "INTERNAL_ERROR", -1, 0); return; }
    bpvm_set_output(vm, v1_output_sink, NULL);

    uint32_t t0 = HAL_GetTick();
    bpvm_status_t st = bpvm_load_mod_buffer(vm, data, size, path);
    if (st == BPVM_OK) st = bpvm_run(vm);
    uint32_t dt = HAL_GetTick() - t0;

    emit_exited(session, (st == BPVM_OK) ? "OK" : "RUNTIME_ERROR", (int) st, dt);
    bpvm_destroy(vm);
}

/* ---- dispatch ---- */

static void dispatch(int first_char) {
    int len = stm32_wire_recv_line(first_char, s_line, sizeof(s_line));
    if (len == -2) return;   /* línea estancada: silencio; el IDE reintenta */
    if (len < 0)  { stm32_wire_send_fatal("PROTOCOL_ERROR", "line too long"); return; }

    json_obj_t obj;
    if (json_parse(s_line, (size_t) len, &obj) != 0) {
        stm32_wire_send_fatal("PROTOCOL_ERROR", "bad JSON"); return;
    }
    long id = json_get_long(&obj, "id", 0);
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        stm32_wire_send_error(id, "PROTOCOL_ERROR", "missing type"); return;
    }

    if      (strcmp(type, "HELLO")     == 0) handle_hello(id);
    else if (strcmp(type, "INFO")      == 0) handle_info(id);
    else if (strcmp(type, "PING")      == 0) reply_empty("PONG", id);
    else if (strcmp(type, "TIME")      == 0) reply_empty("TIME_REPLY", id);
    else if (strcmp(type, "LIST")      == 0) handle_list(id, &obj);
    else if (strcmp(type, "STAT")      == 0) handle_stat(id, &obj);
    else if (strcmp(type, "DF")        == 0) handle_df(id);
    else if (strcmp(type, "GET")       == 0) handle_get(id, &obj);
    else if (strcmp(type, "PUT")       == 0) handle_put(id, &obj);
    else if (strcmp(type, "DEL")       == 0) handle_del(id, &obj);
    else if (strcmp(type, "MKDIR")     == 0) reply_empty("MKDIR_REPLY", id);
    else if (strcmp(type, "FORMAT")    == 0) handle_format(id, &obj);
    else if (strcmp(type, "RUN")       == 0) handle_run(id, &obj);
    else if (strcmp(type, "LOG_DUMP")  == 0) handle_log_dump(id);
    else if (strcmp(type, "LOG_CLEAR") == 0) reply_empty("LOG_CLEAR_REPLY", id);
    else if (strcmp(type, "RESET")     == 0) {
        reply_empty("RESET_REPLY", id);
        HAL_Delay(50);
        NVIC_SystemReset();
    } else {
        char msg[96];
        snprintf(msg, sizeof(msg), "type '%s' no implementado (H9.2)", type);
        stm32_wire_send_error(id, "UNSUPPORTED", msg);
    }
}

void stm32_repl_run(void) {
    /* FIFO RX/TX (8 bytes): absorbe el hueco de procesado entre la línea JSON
     * y los bytes bulk que la siguen → PUT fiable aunque la CPU vaya lenta.
     * (El fix definitivo del timing es subir el reloj a 160 MHz.) */
    HAL_UARTEx_SetTxFifoThreshold(&hcom_uart[COM1], UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(&hcom_uart[COM1], UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_EnableFifoMode(&hcom_uart[COM1]);

    stm32_wire_send_cstr("=== bpvm-stm32 REPL (wire v1) listo ===");

    uint32_t last_blink = HAL_GetTick();
    for (;;) {
        int c = stm32_wire_getchar();
        if (c == '{') dispatch(c);

        uint32_t now = HAL_GetTick();
        if (now - last_blink >= 500U) {     /* heartbeat */
            last_blink = now;
            BSP_LED_Toggle(LED_GREEN);
        }
    }
}
