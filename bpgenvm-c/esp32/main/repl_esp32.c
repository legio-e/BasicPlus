/*
 * repl_esp32.c — dispatcher del wire BPVM v1 en el firmware ESP32-S3.
 *
 * Adaptado de pico/repl_v1.c. Diferencias clave:
 *  - I/O por UART0 (wire_v1.c ESP32), no stdio/USB-CDC.
 *  - OUTPUT / EXITED / LIST se emiten por el wire (no fputs a stdout —
 *    en ESP32 stdout es la consola USB-JTAG, canal distinto).
 *  - Sin AOT (.mdn es ARM Thumb-2, no cruza a Xtensa) ni mdn_loader.
 *  - Sin BOOTSEL (pico-specific). RESET → esp_restart().
 *  - FS en RAM (fs_ram.c).
 *
 * Subset de comandos: HELLO, INFO, PING, RESET, LIST, STAT, GET, PUT,
 * DEL, RUN. El resto responde UNSUPPORTED.
 */
#include "repl_esp32.h"
#include "wire_v1.h"
#include "json_min.h"
#include "fs.h"
#include "bpvm_fs.h"   /* H19-F1: base-dir por proyecto (bpvm_fs_set_basedir_from_module) */

#include "bpvm.h"
#include "bpvm_internal.h"   /* inspección de deps en handle_run */
#include "bpvm_rtc.h"        /* H14 — TIME del wire → RTC (bpvm_rtc_set_now_ms) */
#include "bpvm_pico.h"       /* paso 4 cierre — bpvm_pico_reset_cause (INFO) */
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"      /* esp_restart */
#include "esp_timer.h"       /* uptime */
#include "esp_mac.h"         /* INFO: uniqueId desde la MAC de efuse */
#include "esp_flash.h"       /* INFO: tamaño real de la flash montada */
#include "esp_heap_caps.h"   /* INFO: PSRAM mapeada (0 si el módulo no trae) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Buffer VM compartido (definido en main.c). */
extern uint8_t s_vm_buffer[];
extern const uint32_t s_vm_buffer_size;

#define ESP32_BUILD_DATE  (__DATE__ " " __TIME__)

static char    s_line_buf[WIRE_V1_LINE_MAX];
static char    s_reply_buf[2048];
#ifndef V1_PUT_BUF_SIZE
#define V1_PUT_BUF_SIZE  (16 * 1024)   /* placas con más RAM lo suben por -D (P4 = 64 KB) */
#endif
static uint8_t s_put_buf[V1_PUT_BUF_SIZE];

/* Identidad de placa para INFO/HELLO. Por defecto = ESP32-S3; una placa la
 * cambia con repl_set_board_id() (p.ej. el P4 desde p4_board_id.c).
 *
 * NB: NO usamos weak/strong. En ESP-IDF cada componente es un .a y el linker
 * solo tira del .o del override si algo referencia su símbolo; con la def débil
 * ya satisfecha, el .o "fuerte" no se enlazaba y el override se ignoraba (salía
 * esp32s3 en el P4). El setter explícito lo evita: main referencia la función
 * de install -> el .o se enlaza, y la llamada fija el puntero. */
static const repl_board_id_t s_default_board = {
    "esp32s3",        /* board_name   */
    "bpvm-esp32",     /* server_name  */
    240000000L,       /* cpu_freq_hz  */
    45,               /* gpio_count   */
    0,                /* pio_count    */
    8,                /* pwm_slices   */
    20,               /* adc_channels */
    512L * 1024L,     /* sram_bytes   */
};
static const repl_board_id_t *s_board_id = &s_default_board;

void repl_set_board_id(const repl_board_id_t *id) { if (id) s_board_id = id; }

/* ---- fs_status_t → (code, message) v1 ---- */
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
        default:                    *code = "INTERNAL_ERROR";  *msg = "fs unknown"; break;
    }
}

/* ====================== META ====================== */

static void handle_hello(long id, const json_obj_t* obj) {
    (void) obj;
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "HELLO_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "protoVersion", 1);
    if (off < 0) goto err;
    off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "serverName", s_board_id->server_name);
    if (off < 0) goto err;
    off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "serverBuild", ESP32_BUILD_DATE);
    if (off < 0) goto err;
    /* Capabilities: META + FILES + TERMINAL (RUN/OUTPUT/EXITED). Sin
     * BOOTSEL (no aplica en ESP32). */
    static const char* CAPS = ",\"capabilities\":[\"META\",\"FILES\",\"TERMINAL\"]";
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

static void handle_info(long id, const json_obj_t* obj) {
    /* Mismo arreglo que en stm32_repl.c: el diálogo INFO del IDE
     * (PicoExplorer.formatInfo) lee el set completo de campos; antes solo
     * mandábamos 5 → diálogo medio vacío. Datos del CHIP ESP32-S3
     * (datasheet): 45 GPIOs (0-21 y 26-48), sin PIO (el RMT no es
     * comparable), PWM = 8 canales LEDC, ADC = 20 canales (ADC1+ADC2,
     * GPIO1..20), SRAM interna 512 KB. Flash y PSRAM se miden en runtime
     * (dependen del módulo montado). Los backends BP de Pwm/Adc en ESP32
     * aún no están cableados (solo GPIO) — esto describe el hardware.
     * tempMilliC=0: el diálogo oculta la línea (sensor interno, futuro). */
    (void) obj;
    long uptime  = (long)(esp_timer_get_time() / 1000LL);
    long fsTotal = (long) fs_total_bytes();
    long fsUsed  = (long) fs_used_bytes();
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    char uid[16];
    snprintf(uid, sizeof(uid), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    uint32_t flash_bytes = 0;
    if (esp_flash_get_size(NULL, &flash_bytes) != ESP_OK) flash_bytes = 0;
    long psram_bytes = (long) heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    const repl_board_id_t *bid = s_board_id;
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "INFO_REPLY", id);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "boardName", bid->board_name);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "uniqueId", uid);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "cpuFreqHz", bid->cpu_freq_hz);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "uptimeMs", uptime);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "resetReason", bpvm_pico_reset_cause());
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "tempMilliC", 0);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "gpioCount", bid->gpio_count);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "pioCount", bid->pio_count);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "pwmSlices", bid->pwm_slices);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "adcChannels", bid->adc_channels);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "flashBytes", (long) flash_bytes);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "sramBytes", bid->sram_bytes);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "psramBytes", psram_bytes);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "fsTotalBytes", fsTotal);
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "fsUsedBytes", fsUsed);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "INFO_REPLY no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

static void handle_ping(long id, const json_obj_t* obj) {
    (void) obj;
    wire_v1_send_reply_empty("PONG", id);
}

static void handle_reset(long id, const json_obj_t* obj) {
    (void) obj;
    wire_v1_send_reply_empty("RESET_REPLY", id);
    vTaskDelay(pdMS_TO_TICKS(100));   /* deja vaciar el TX del wire */
    esp_restart();                     /* no retorna */
}

/* ====================== FILES ====================== */

typedef struct { int first; } list_ctx_t;

static int list_cb(const char* name, uint32_t size, void* user) {
    /* Construimos cada entry en un buffer pequeño y la mandamos como bulk
     * raw (sin newline) — el wire en UART0 no tiene stdout. */
    list_ctx_t* ctx = (list_ctx_t*) user;
    /* paso 4 cierre — CRC del contenido (mismo que java.util.zip.CRC32) para
     * que el IDE salte el PUT por contenido REAL del device. fs_get es una
     * búsqueda barata en RAM; si fallara, crc=0 (el IDE re-subirá, seguro). */
    uint32_t crc = 0;
    const uint8_t* d; uint32_t sz;
    if (fs_get(name, &d, &sz) == FS_OK) crc = bpvm_crc32(d, sz);
    char e[128];
    int o = 0;
    if (!ctx->first) e[o++] = ',';
    ctx->first = 0;
    o += snprintf(e + o, sizeof(e) - o, "{\"name\":\"");
    for (const char* p = name; *p && o < (int) sizeof(e) - 64; p++) {
        if (*p == '"' || *p == '\\') e[o++] = '\\';
        e[o++] = *p;
    }
    o += snprintf(e + o, sizeof(e) - o, "\",\"size\":%lu,\"crc\":%lu,\"isDir\":false}",
                  (unsigned long) size, (unsigned long) crc);
    wire_v1_send_bulk((const uint8_t*) e, (size_t) o);
    return 0;
}

static void handle_list(long id, const json_obj_t* obj) {
    (void) obj;
    char head[64];
    int hn = snprintf(head, sizeof(head), "{\"type\":\"LIST_REPLY\",\"id\":%ld,\"entries\":[", id);
    wire_v1_send_bulk((const uint8_t*) head, (size_t) hn);
    list_ctx_t ctx = { 1 };
    fs_list(list_cb, &ctx);
    wire_v1_send_line("]}", 2);   /* cierra + '\n' */
}

static void handle_stat(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return;
    }
    const uint8_t* data; uint32_t size;
    fs_status_t s = fs_get(path, &data, &size);
    if (s != FS_OK) { const char* c; const char* m; map_fs_status(s, &c, &m); wire_v1_send_error(id, c, m); return; }
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "STAT_REPLY", id);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "size", (long) size);
    if (off >= 0) off = wire_v1_field_bool(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "isDir", 0);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "mtime", 0);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "STAT_REPLY no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

static void handle_get(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return;
    }
    const uint8_t* data; uint32_t size;
    fs_status_t s = fs_get(path, &data, &size);
    if (s != FS_OK) { const char* c; const char* m; map_fs_status(s, &c, &m); wire_v1_send_error(id, c, m); return; }
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "GET_REPLY", id);
    if (off >= 0) off = wire_v1_field_bulk(s_reply_buf, sizeof(s_reply_buf), (size_t) off, (size_t) size);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "GET_REPLY no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
    if (size > 0) wire_v1_send_bulk(data, (size_t) size);
}

static void handle_put(long id, const json_obj_t* obj, const uint8_t* bulk, size_t bulk_size) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return;
    }
    fs_status_t s = fs_put(path, bulk, (uint32_t) bulk_size);
    if (s != FS_OK) { const char* c; const char* m; map_fs_status(s, &c, &m); wire_v1_send_error(id, c, m); return; }
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "PUT_REPLY", id);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "size", (long) bulk_size);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "PUT_REPLY no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

static void handle_del(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return;
    }
    fs_status_t s = fs_delete(path);
    if (s != FS_OK) { const char* c; const char* m; map_fs_status(s, &c, &m); wire_v1_send_error(id, c, m); return; }
    wire_v1_send_reply_empty("DEL_REPLY", id);
}

/* ====================== TERMINAL (RUN) ====================== */

/* Resolución de módulo: base-dir del proyecto, luego /app/ y /lib/ (igual que Pico). */
static fs_status_t v1_get_resolve(const char* name, const uint8_t** data_out, uint32_t* size_out) {
    char path[FS_NAME_LEN];
    /* H19 — base-dir del proyecto PRIMERO (carpeta del módulo principal).
     * Plano (basedir="") o entry absoluto (name[0]=='/') → se salta. */
    const char* bd = bpvm_fs_basedir();
    if (bd && bd[0] && name[0] != '/') {
        snprintf(path, sizeof(path), "%s/%s", bd, name);
        if (fs_get(path, data_out, size_out) == FS_OK) return FS_OK;
    }
    fs_status_t s = fs_get(name, data_out, size_out);
    if (s == FS_OK) return FS_OK;
    snprintf(path, sizeof(path), "/app/%s", name);
    s = fs_get(path, data_out, size_out);
    if (s == FS_OK) return FS_OK;
    snprintf(path, sizeof(path), "/lib/%s", name);
    return fs_get(path, data_out, size_out);
}

static long s_active_session = 0;
static long s_next_session = 1;

typedef struct { long session; } v1_sink_ctx_t;

/* Sink de la VM: cada chunk del programa → evento OUTPUT por el wire.
 * Trocea en frames de ≤~900 bytes raw para no desbordar el buffer. */
static void v1_output_sink(const char* data, size_t len, void* user) {
    v1_sink_ctx_t* ctx = (v1_sink_ctx_t*) user;
    size_t i = 0;
    do {
        char buf[1024];
        int hn = snprintf(buf, sizeof(buf),
                          "{\"type\":\"OUTPUT\",\"session\":%ld,\"stream\":\"stdout\",\"data\":\"",
                          ctx->session);
        if (hn < 0) return;
        size_t o = (size_t) hn;
        while (i < len && o < sizeof(buf) - 8) {
            char c = data[i++];
            switch (c) {
                case '"':  buf[o++] = '\\'; buf[o++] = '"'; break;
                case '\\': buf[o++] = '\\'; buf[o++] = '\\'; break;
                case '\n': buf[o++] = '\\'; buf[o++] = 'n'; break;
                case '\r': buf[o++] = '\\'; buf[o++] = 'r'; break;
                case '\t': buf[o++] = '\\'; buf[o++] = 't'; break;
                default:
                    if ((unsigned char) c < 0x20) {
                        static const char* HEX = "0123456789abcdef";
                        buf[o++] = '\\'; buf[o++] = 'u'; buf[o++] = '0'; buf[o++] = '0';
                        buf[o++] = HEX[(c >> 4) & 0xF];
                        buf[o++] = HEX[c & 0xF];
                    } else {
                        buf[o++] = c;
                    }
                    break;
            }
        }
        buf[o++] = '"';
        buf[o++] = '}';
        wire_v1_send_line(buf, o);
    } while (i < len);
}

static void send_exited(long session, const char* status, int exit_code,
                        long elapsed_ms, const char* errmsg) {
    int off = wire_v1_msg_begin_event(s_reply_buf, sizeof(s_reply_buf), 0, "EXITED");
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "session", session);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "status", status);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "exitCode", exit_code);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "elapsedMs", elapsed_ms);
    if (errmsg && off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "errorMessage", errmsg);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off >= 0) wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* P-run-stop (#257) + P-autorun (#256) — wire durante el run (la VM
 * invoca el poll entre quanta, en la MISMA task que corre bpvm_run —
 * aquí no hay comm task, así que el poll puede responder directamente
 * sin riesgo de entrelazado):
 *   KILL  → ack diferido (KILL_REPLY tras parar, antes del EXITED) +
 *           devuelve 1 (BPVM_KILLED).
 *   HELLO → HELLO_REPLY inmediato — el IDE puede conectarse con un
 *           (auto)run en marcha y ofrecer Stop.
 *   otra  → error BUSY inmediato. */
static long s_kill_ack_id = -1;

static int esp32_run_poll_cb(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    int c = wire_v1_try_getchar();
    if (c < 0) return 0;
    int n = wire_v1_recv_line(c, s_line_buf, sizeof(s_line_buf));
    if (n < 0) return 0;
    json_obj_t obj;
    if (json_parse(s_line_buf, (size_t) n, &obj) != 0) return 0;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof(type));
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) { s_kill_ack_id = rid; return 1; }
    if (strcmp(type, "HELLO") == 0) { handle_hello(rid, &obj); return 0; }
    wire_v1_send_error(rid, "BUSY", "ejecución en curso: solo HELLO/KILL");
    return 0;
}

/* Núcleo del RUN — compartido entre el comando RUN del wire (id >= 0)
 * y el autorun de boot (#256, id < 0). Con id < 0 no hay cliente: sin
 * RUN_REPLY y errores de resolución a la consola (USB-Serial-JTAG).
 * Lo demás (sesión, OUTPUT, poll, EXITED) es idéntico. */
static void run_module_path(const char* path, long id) {
    if (s_active_session != 0) {
        if (id >= 0) wire_v1_send_error(id, "BUSY", "ya hay una sesión RUN en curso");
        else         printf("[autorun] BUSY — ignorado\n");
        return;
    }

    /* H19-F1 — fija el base-dir del proyecto si el módulo vive en /app/<proj>/
     * (el IDE manda la ruta cualificada). Plano (/app/X.mod o nombre suelto) →
     * sin base-dir = modo plano. Se resetea en cada run. */
    bpvm_fs_set_basedir_from_module(path);
    /* H19-F2 diag — confirma en consola (idf.py monitor) la raíz del proyecto:
     * con proyecto sale basedir='/app/<proj>'; en fichero-suelto basedir=''.
     * Si esta línea NO aparece, el firmware es PRE-H19 (reflashear). */
    printf("[run] entry='%s' basedir='%s'\n", path, bpvm_fs_basedir());

    const uint8_t* data; uint32_t size;
    fs_status_t fs_s = v1_get_resolve(path, &data, &size);
    if (fs_s != FS_OK) {
        const char* c; const char* m; map_fs_status(fs_s, &c, &m);
        if (id >= 0) wire_v1_send_error(id, c, m);
        else         printf("[autorun] %s: %s — REPL normal\n", path, m);
        return;
    }

    long session = s_next_session++;
    s_active_session = session;
    if (id >= 0) {   /* RUN_REPLY con session antes de ejecutar. */
        int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "RUN_REPLY", id);
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "session", session);
        if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
        if (off >= 0) wire_v1_send_line(s_reply_buf, (size_t) off);
    }

    bpvm_t* vm = bpvm_init(s_vm_buffer, s_vm_buffer_size, 0);
    if (!vm) { send_exited(session, "INTERNAL_ERROR", -1, 0, "bpvm_init failed"); s_active_session = 0; return; }

    v1_sink_ctx_t sink_ctx = { session };
    bpvm_set_output(vm, v1_output_sink, &sink_ctx);

    bpvm_status_t ls = bpvm_load_mod_buffer(vm, data, size, path);
    if (ls != BPVM_OK) {
        send_exited(session, "RUNTIME_ERROR", (int) ls, 0, bpvm_status_str(ls));
        bpvm_destroy(vm); s_active_session = 0; return;
    }

    /* Resolución iterativa de deps (mismo loop que la Pico). */
    for (int pass = 0; pass < 4; pass++) {
        int loaded_any = 0;
        int n_before = vm->module_count;
        for (int mi = 0; mi < n_before; mi++) {
            bpvm_module_t* m = &vm->modules[mi];
            for (int k = 0; k < m->import_count; k++) {
                const char* imp = m->imports[k];
                if (!imp || !imp[0]) continue;
                char owner[40]; size_t ol = 0;
                while (imp[ol] && imp[ol] != '.' && ol < sizeof(owner) - 1) { owner[ol] = imp[ol]; ol++; }
                owner[ol] = '\0';
                if (!owner[0]) continue;
                int already = 0;
                for (int j = 0; j < vm->module_count; j++)
                    if (strcmp(vm->modules[j].name, owner) == 0) { already = 1; break; }
                if (already) continue;
                char fname[48];
                snprintf(fname, sizeof(fname), "%s.mod", owner);
                const uint8_t* dep; uint32_t dep_size;
                if (v1_get_resolve(fname, &dep, &dep_size) != FS_OK) continue;
                bpvm_status_t ds = bpvm_load_mod_buffer(vm, dep, dep_size, owner);
                if (ds != BPVM_OK) {
                    char em[80]; snprintf(em, sizeof(em), "dep %s: %s", fname, bpvm_status_str(ds));
                    send_exited(session, "RUNTIME_ERROR", (int) ds, 0, em);
                    bpvm_destroy(vm); s_active_session = 0; return;
                }
                loaded_any = 1;
            }
        }
        if (!loaded_any) break;
    }

    /* Ejecutar. NO AOT en Xtensa (el .mdn es ARM). bpvm_run single-thread
     * (el SMP en ESP32 es H4.2+, no necesario para H4.3). */
    s_kill_ack_id = -1;
    bpvm_set_poll(vm, esp32_run_poll_cb, NULL);   /* P-run-stop (#257) */

    uint32_t t0 = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    bpvm_status_t rs = bpvm_run(vm);
    uint32_t dt = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) - t0;

    /* P-run-stop — ack diferido del KILL, ANTES del EXITED. */
    bpvm_set_poll(vm, NULL, NULL);
    if (s_kill_ack_id >= 0) { wire_v1_send_reply_empty("KILL_REPLY", s_kill_ack_id); s_kill_ack_id = -1; }

    const char* link_err = bpvm_link_error(vm);   /* paso 4 — "" salvo fallo de link */
    const char* status_str = (rs == BPVM_OK)     ? "OK"
                           : (rs == BPVM_KILLED) ? "KILLED"
                           : (link_err[0])       ? "LINK_ERROR" : "RUNTIME_ERROR";
    int exit_code = (rs == BPVM_OK) ? 0 : (rs == BPVM_KILLED) ? 130 : (int) rs;
    const char* err_msg = (rs == BPVM_OK) ? NULL
                        : (link_err[0] ? link_err : bpvm_status_str(rs));
    send_exited(session, status_str, exit_code, (long) dt, err_msg);

    bpvm_destroy(vm);
    s_active_session = 0;
}

static void handle_run(long id, const json_obj_t* obj) {
    char path[FS_NAME_LEN];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta 'path'"); return;
    }
    run_module_path(path, id);
}

/* P-autorun (#256) — si existe /sys/auto.txt, ejecuta el módulo de su
 * primera línea por el mismo camino que un RUN del wire. app_main lo
 * llama tras fs_init + wire init, antes del bucle REPL. El poll del
 * run atiende HELLO/KILL → la placa nunca queda sorda. */
void repl_esp32_autorun(void) {
    const uint8_t* data; uint32_t size;
    if (fs_get("/sys/auto.txt", &data, &size) != FS_OK) return;

    char path[FS_NAME_LEN];
    size_t n = 0, i = 0;
    while (i < size && (data[i] == ' ' || data[i] == '\t')) i++;
    while (i < size && data[i] != '\n' && data[i] != '\r'
           && n + 1 < sizeof(path)) {
        path[n++] = (char) data[i++];
    }
    while (n > 0 && (path[n - 1] == ' ' || path[n - 1] == '\t')) n--;
    path[n] = '\0';
    if (n == 0) { printf("[autorun] /sys/auto.txt vacío — REPL normal\n"); return; }
    printf("[autorun] %s\n", path);
    run_module_path(path, -1);
    printf("[autorun] terminado — REPL normal\n");
}

/* ====================== Dispatcher ====================== */

static void handle_request(const char* line, int len) {
    json_obj_t obj;
    if (json_parse(line, (size_t) len, &obj) != 0) {
        wire_v1_send_fatal("PROTOCOL_ERROR", "JSON inválido"); return;
    }
    long bulk = json_get_long(&obj, "bulk", 0);
    size_t bulk_size = 0;
    if (bulk > 0) {
        if (bulk > (long) sizeof(s_put_buf)) {
            static uint8_t drain[64];
            long remaining = bulk;
            while (remaining > 0) {
                size_t chunk = (size_t)(remaining > (long) sizeof(drain) ? (long) sizeof(drain) : remaining);
                if (wire_v1_recv_bulk(drain, chunk, sizeof(drain)) < 0) break;
                remaining -= (long) chunk;
            }
            wire_v1_send_error(json_get_long(&obj, "id", 0), "NO_SPACE", "bulk supera el buffer del servidor");
            return;
        }
        if (wire_v1_recv_bulk(s_put_buf, (size_t) bulk, sizeof(s_put_buf)) < 0) {
            wire_v1_send_fatal("PROTOCOL_ERROR", "lectura de bulk truncada"); return;
        }
        bulk_size = (size_t) bulk;
    }

    long id = json_get_long(&obj, "id", 0);
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        wire_v1_send_error(id, "PROTOCOL_ERROR", "falta 'type'"); return;
    }

    if (strcmp(type, "HELLO") == 0) { handle_hello(id, &obj); return; }
    if (strcmp(type, "INFO")  == 0) { handle_info(id, &obj);  return; }
    if (strcmp(type, "PING")  == 0) { handle_ping(id, &obj);  return; }
    if (strcmp(type, "RESET") == 0) { handle_reset(id, &obj); return; }
    if (strcmp(type, "LIST")  == 0) { handle_list(id, &obj);  return; }
    if (strcmp(type, "STAT")  == 0) { handle_stat(id, &obj);  return; }
    if (strcmp(type, "GET")   == 0) { handle_get(id, &obj);   return; }
    if (strcmp(type, "PUT")   == 0) { handle_put(id, &obj, s_put_buf, bulk_size); return; }
    if (strcmp(type, "DEL")   == 0) { handle_del(id, &obj);   return; }
    if (strcmp(type, "RUN")   == 0) { handle_run(id, &obj);   return; }
    if (strcmp(type, "TIME")  == 0) {   /* H14 — sync de hora del IDE → RTC */
        long epochSec = json_get_long(&obj, "epochSec", -1);
        if (epochSec >= 0) bpvm_rtc_set_now_ms((int64_t) epochSec * 1000LL);
        wire_v1_send_reply_empty("TIME_REPLY", id);
        return;
    }
    /* P-run-stop (#257) — KILL en idle: nada que matar (el KILL útil llega
     * DURANTE un RUN y lo atiende esp32_run_poll_cb). */
    if (strcmp(type, "KILL")  == 0) {
        wire_v1_send_error(id, "NO_SESSION", "no hay programa en ejecución");
        return;
    }

    wire_v1_send_error(id, "UNSUPPORTED", "type no implementado en el firmware ESP32");
}

void repl_esp32_run(void) {
    for (;;) {
        int n = wire_v1_recv_line(-1, s_line_buf, sizeof(s_line_buf));
        if (n < 0) { wire_v1_send_fatal("PROTOCOL_ERROR", "línea excede WIRE_V1_LINE_MAX"); continue; }
        if (n == 0) continue;
        if (s_line_buf[0] != '{') continue;   /* ignora ruido no-v1 */
        handle_request(s_line_buf, n);
    }
}
