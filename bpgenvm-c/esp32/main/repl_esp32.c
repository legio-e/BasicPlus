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
#include "board_mgr_esp32.h" /* H9: gestión de placa (STATE/ENV/PART) + board_boot_status */
#include "log.h"             /* log persistente (post-mortem) → LOG_DUMP / LOG_CLEAR */
#include "bpvm_dbg_wire.h"   /* #326: ramo de depuración, núcleo portable compartido */
#include "bpvm_pack.h"       /* #327: BPVM_PACK_BURN_CHUNK (bulk del PACK_BURN_DATA) */
#include "aot_registry.h"    /* H4 AOT: bpvm_aot_clear/count (hook esp_aot_register) */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"      /* esp_restart */
#include "esp_timer.h"       /* uptime */
#include "esp_mac.h"         /* INFO: uniqueId desde la MAC de efuse */
#include "esp_flash.h"       /* INFO: tamaño real de la flash montada */
#include "esp_heap_caps.h"   /* INFO: PSRAM mapeada (0 si el módulo no trae) */
/* mdn_loader NO va dentro del #if: el INFO publica `arch` (bpvm_mdn_host_arch)
 * en TODA la familia — el S3 también tiene que decir a qué ISA compilar. Sólo la
 * CARGA de .mdn dinámico es de momento del P4. */
#include "mdn_loader.h"      /* H4/H11: bpvm_load_mdn + bpvm_mdn_host_arch (INFO) */

#if defined(__riscv)
#include "esp_cache.h"       /* H4 AOT: sync de cachés tras copiar .mdn a RAM exec */
#include "esp_log.h"         /* H4 AOT: trazas del cargador .mdn (consola) */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* H4 AOT — hook de registro de funciones AOT baked-in. Se define FUERTE por
 * target en un fichero aparte que enlaza cada build (P4: aot_funcs_p4.c con el
 * thunk nativo RISC-V; S3: aot_funcs_stub.c no-op). Aqui SOLO se DECLARA: asi
 * repl.o deja la referencia sin resolver y el linker TIRA del objeto fuerte del
 * archive. (Con un weak no-op local el linker se conformaria con el y jamas
 * enlazaria el fuerte del .a → AOT muerto: lo vimos, esp_aot_register salia 'W'
 * y el thunk no aparecia.) Se llama tras link, antes de bpvm_run (igual Pico).
 * El .mdn dinamico cross-arch = Hito 2. */
void esp_aot_register(struct bpvm* vm);

/* Buffer VM compartido (definido en main.c). Ahora PUNTERO (no array): el S3 lo
 * apunta a un array estático en SRAM interna y el P4 a PSRAM reservada en boot.
 * Misma convención que la Pico/Metro (repl_v1.c, H7.2.b). */
extern uint8_t* s_vm_buffer;
extern uint32_t s_vm_buffer_size;

/* H9 — reparto heap/stacks de la VM: la región de stacks BP se TOPA en 512 KB y
 * el RESTO es heap (decisión Eduardo 19-jul, igual que el Pico repl_v1.c). El
 * default de bpvm_init (tamaño/2) desperdiciaba ~1 MB en stacks con los 2 MB de
 * PSRAM del P4. Cálculo ÚNICO, compartido por RUN (bpvm_init) e INFO. */
static size_t vm_stack_region_bytes(void) {
    size_t r = (size_t) s_vm_buffer_size / 2u;
    if (r > 512u * 1024u) r = 512u * 1024u;
    return r;
}

#define ESP32_BUILD_DATE  (__DATE__ " " __TIME__)

static char    s_line_buf[WIRE_V1_LINE_MAX];
static char    s_reply_buf[2048];
#ifndef V1_PUT_BUF_SIZE
#define V1_PUT_BUF_SIZE  (48 * 1024)   /* 16K→48K (16-jul): Json.mod ya pesa 17.8K (Gui 31.8K)
                                        * y el bulk se rechazaba con NO_SPACE, igual que en la Pico.
                                        * placas con más RAM lo suben por -D (P4 = 64 KB). El fix
                                        * de verdad = streaming por chunks (mejora #294). */
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
    /* Tamaño FÍSICO del chip (SFDP/JEDEC), NO el del sobre del build: esp_flash_get_size
     * devuelve el FLASHSIZE del sdkconfig (horneado, "a piñon"). Mismo criterio que el
     * JEDEC del RP2350 (#292): la verdad es el chip. Fallback al sobre si el chip no
     * reporta SFDP. OJO — lo USABLE lo acota la tabla vendor (bpdata), tambien horneada
     * en el build: si fisica > sobre, hay flash de sobra sin particionar. */
    uint32_t flash_bytes = 0;
    if (esp_flash_get_physical_size(NULL, &flash_bytes) != ESP_OK || flash_bytes == 0) {
        if (esp_flash_get_size(NULL, &flash_bytes) != ESP_OK) flash_bytes = 0;
    }
    long psram_bytes = (long) heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    const repl_board_id_t *bid = s_board_id;
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "INFO_REPLY", id);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "boardName", bid->board_name);
    /* H11 — ARQUITECTURA del código nativo que ejecuta este firmware. El IDE la
     * necesita para compilar el .mdn a la ISA correcta cuando no hay proyecto
     * donde apuntarla (un `.bp` suelto). La sabe la placa, la dice la placa. */
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "arch", (long) bpvm_mdn_host_arch());
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
    /* H9 — reparto de la memoria de la VM (heap + stacks BP, tope 512K); mismo
     * cálculo que el RUN. El IDE lo muestra como "VM: heap X + stack Y". */
    {
        size_t vstack = vm_stack_region_bytes();
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "vmHeapBytes", (long)(s_vm_buffer_size - vstack));
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "vmStackBytes", (long) vstack);
    }
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
    /* H11 — CRC por trozos (buffer de 256 B en la pila dentro de la fachada), no
     * leyendo el fichero entero a un espejo. Misma función que usa el Pico. */
    uint32_t crc = 0;
    if (bpvm_fs_crc32(name, &crc) != 0) crc = 0;
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

/* ── SAVE / DF / MKDIR: el IDE los manda y la familia ESP32 no los tenía ──
 * SAVE lo dispara PicoExplorer tras cada subida y desde el botón de guardar, así
 * que el desfase saltaba en uso normal. Con littlefs la escritura ya es firme
 * (fs_save_to_flash es el punto de sincronización de la fachada), pero el
 * comando debe existir y contestar como en el Pico: el protocolo es UNO. */
static void handle_save(long id, const json_obj_t* obj) {
    (void) obj;
    uint32_t t0 = (uint32_t) (esp_timer_get_time() / 1000);
    fs_status_t s = fs_save_to_flash();
    uint32_t dt = (uint32_t) (esp_timer_get_time() / 1000) - t0;
    if (s != FS_OK) {
        const char* c; const char* m; map_fs_status(s, &c, &m);
        wire_v1_send_error(id, c, m);
        return;
    }
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "SAVE_REPLY", id);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "durationMs", (long) dt);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "SAVE_REPLY no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

static void handle_df(long id, const json_obj_t* obj) {
    (void) obj;
    long total = (long) fs_total_bytes();
    long used  = (long) fs_used_bytes();
    long fcnt  = (long) fs_file_count();
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "DF_REPLY", id);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "totalBytes", total);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "usedBytes", used);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "freeBytes", total - used);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "fileCount", fcnt);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "DF_REPLY no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* Igual que en el Pico: el `/` es namespace, no hay nodos de directorio → MKDIR
 * es idempotente y silenciosa. Existe para que el IDE no se coma un error. */
static void handle_mkdir(long id, const json_obj_t* obj) {
    (void) obj;
    wire_v1_send_reply_empty("MKDIR_REPLY", id);
}

/* ── LOG (post-mortem) ── Escapa el texto a JSON y lo va soltando por bulk, sin
 * buffer intermedio del tamaño del log. Mismo formato de reply que el Pico. */
static void log_chunk_sink(const char* data, size_t len, void* user) {
    (void) user;
    char esc[256 * 2];   /* el núcleo entrega chunks de 256 B; peor caso = x2 */
    size_t o = 0;
    for (size_t i = 0; i < len; i++) {
        char ch = data[i];
        const char* rep = NULL;
        switch (ch) {
            case '"':  rep = "\\\""; break;
            case '\\': rep = "\\\\"; break;
            case '\n': rep = "\\n";  break;
            case '\r': rep = "\\r";  break;
            case '\t': rep = "\\t";  break;
            default: break;
        }
        if (rep) {
            if (o + 2 > sizeof esc) { wire_v1_send_bulk((const uint8_t*) esc, o); o = 0; }
            esc[o++] = rep[0]; esc[o++] = rep[1];
        } else if ((unsigned char) ch < 0x20) {
            if (o + 6 > sizeof esc) { wire_v1_send_bulk((const uint8_t*) esc, o); o = 0; }
            o += (size_t) snprintf(esc + o, sizeof esc - o, "\\u%04x", (unsigned) ch);
        } else {
            if (o + 1 > sizeof esc) { wire_v1_send_bulk((const uint8_t*) esc, o); o = 0; }
            esc[o++] = ch;
        }
    }
    if (o) wire_v1_send_bulk((const uint8_t*) esc, o);
}

static void handle_log_dump(long id, const json_obj_t* obj) {
    (void) obj;
    char head[64];
    int hn = snprintf(head, sizeof head, "{\"type\":\"LOG_DUMP_REPLY\",\"id\":%ld,\"text\":\"", id);
    wire_v1_send_bulk((const uint8_t*) head, (size_t) hn);
    log_dump(log_chunk_sink, NULL);
    wire_v1_send_line("\"}", 2);   /* cierra + '\n' */
}

static void handle_log_clear(long id, const json_obj_t* obj) {
    (void) obj;
    log_clear_ram();
    log_clear_flash();
    log_printf("LOG cleared via wire v1");
    wire_v1_send_reply_empty("LOG_CLEAR_REPLY", id);
}

/* ── #326 DEPURACIÓN: cintura del núcleo portable bpvm_dbg_wire ──
 * La máquina de depurar (breakpoints por pc, pausa, step) ya estaba en la placa
 * —vive en el intérprete— pero el ESP32 no tenía el cableado del wire, así que
 * "Debug on device" contestaba UNSUPPORTED. Lo único de aquí es traducir JSON ↔
 * comando tipado y prestar los buffers; la lógica es la MISMA que la del Pico
 * (de ahí se extrajo el núcleo).
 *
 * Sobre reusar s_line_buf en el bucle de pausa: es correcto porque el pause_cb
 * corre DENTRO de bpvm_run, en la misma task del REPL, y el `obj` de la request
 * que lanzó el RUN ya no se usa cuando la VM está corriendo. Mismo esquema que
 * el Pico lleva funcionando desde #140. */
static void dbgw_send(const char* line, size_t len, void* user) {
    (void) user;
    wire_v1_send_line(line, len);
}

static int dbgw_next_cmd(bpvm_dbg_cmd_t* out, void* user) {
    (void) user;
    int n = wire_v1_recv_line(-1, s_line_buf, sizeof(s_line_buf));
    if (n <= 0) return -1;                       /* overflow / vacía: reintentar */
    json_obj_t o;
    if (json_parse(s_line_buf, (size_t) n, &o) != 0) return -1;
    char type[40];
    if (json_get_str(&o, "type", type, sizeof type) < 0) return -1;
    out->kind = bpvm_dbg_wire_kind(type);
    out->id   = json_get_long(&o, "id",    0);
    out->pc   = json_get_long(&o, "pc",   -1);
    out->bpId = json_get_long(&o, "bpId", -1);
    out->addr = json_get_long(&o, "addr", -1);
    out->ref  = json_get_long(&o, "ref",   0);
    return 0;
}

/* Vive todo el RUN: el núcleo lo recibe como `user` del pause_cb. */
static bpvm_dbg_wire_t s_dbgw = {
    dbgw_next_cmd, dbgw_send, NULL, s_reply_buf, sizeof s_reply_buf, 0
};

/* Rellena un comando tipado desde el JSON ya parseado (camino fuera de pausa). */
static void dbgw_cmd_from_json(bpvm_dbg_cmd_t* c, long id,
                               const json_obj_t* obj, const char* type) {
    c->kind = bpvm_dbg_wire_kind(type);
    c->id   = id;
    c->pc   = json_get_long(obj, "pc",   -1);
    c->bpId = json_get_long(obj, "bpId", -1);
    c->addr = json_get_long(obj, "addr", -1);
    c->ref  = json_get_long(obj, "ref",   0);
}

static void handle_stat(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return;
    }
    /* H11 — el STAT sólo quiere el TAMAÑO; leer el fichero entero para eso era
     * absurdo (y costaba el espejo de 64 KB). */
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) { wire_v1_send_error(id, "NOT_FOUND", "no existe"); return; }
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
    /* H11 — el GET NO carga el fichero: sólo necesita su TAMAÑO para la cabecera
     * y después lo va escupiendo POR TROZOS. Antes era un fs_get, o sea el
     * fichero ENTERO al espejo de 64 KB para copiarlo acto seguido al wire. */
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) { wire_v1_send_error(id, "NOT_FOUND", "no existe"); return; }
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "GET_REPLY", id);
    if (off >= 0) off = wire_v1_field_bulk(s_reply_buf, sizeof(s_reply_buf), (size_t) off, (size_t) size);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "GET_REPLY no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
    /* Trozo de 256 B = el mismo que usa littlefs por dentro: ni introduce un
     * número nuevo ni fuerza al motor a partir lecturas. Si el FS falla a media
     * transferencia ya no se puede rectificar (la cabecera con `bulk` ya salió),
     * así que se corta y el cliente lo detecta por el bulk incompleto. */
    uint32_t sent = 0;
    while (sent < size) {
        uint8_t chunk[256];
        long n = bpvm_fs_read_at(path, sent, chunk, sizeof chunk);
        if (n <= 0) break;
        wire_v1_send_bulk(chunk, (size_t) n);
        sent += (uint32_t) n;
    }
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

/* #294 streaming PUT — subida por trozos (PUT_BEGIN/PUT_DATA/PUT_END), espejo del
 * BURN de packs. Evita buferizar el fichero entero (el PUT clásico tope en
 * s_put_buf=48K): BEGIN crea/trunca, cada DATA apende un chunk, END verifica el
 * tamaño. Una sesión a la vez (un IDE, un puerto). */
static struct {
    int      active;
    char     path[FS_NAME_LEN];
    uint32_t received;
    uint32_t expected;   /* size anunciado en BEGIN (0 = no verificar) */
} s_put_sess;

static void put_reply(long id, const char* type, uint32_t val, const char* field) {
    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, type, id);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, field, (long) val);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "reply no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

static void handle_put_begin(long id, const json_obj_t* obj) {
    char path[FS_NAME_LEN];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return;
    }
    fs_status_t s = fs_put(path, NULL, 0);   /* crea/trunca + dirs padre */
    if (s != FS_OK) { const char* c; const char* m; map_fs_status(s, &c, &m); wire_v1_send_error(id, c, m); return; }
    s_put_sess.active   = 1;
    s_put_sess.received = 0;
    s_put_sess.expected = (uint32_t) json_get_long(obj, "size", 0);
    strncpy(s_put_sess.path, path, sizeof(s_put_sess.path) - 1);
    s_put_sess.path[sizeof(s_put_sess.path) - 1] = '\0';
    put_reply(id, "PUT_BEGIN_REPLY", 0, "received");
}

static void handle_put_data(long id, const json_obj_t* obj, const uint8_t* bulk, size_t bulk_size) {
    (void) obj;
    if (!s_put_sess.active) { wire_v1_send_error(id, "NO_SESSION", "PUT_DATA sin PUT_BEGIN"); return; }
    if (bulk_size > 0) {
        fs_status_t s = fs_put_append(s_put_sess.path, bulk, (uint32_t) bulk_size);
        if (s != FS_OK) {
            s_put_sess.active = 0;   /* sesión muerta ante error de escritura */
            const char* c; const char* m; map_fs_status(s, &c, &m); wire_v1_send_error(id, c, m); return;
        }
        s_put_sess.received += (uint32_t) bulk_size;
    }
    put_reply(id, "PUT_DATA_REPLY", s_put_sess.received, "received");
}

static void handle_put_end(long id, const json_obj_t* obj) {
    (void) obj;
    if (!s_put_sess.active) { wire_v1_send_error(id, "NO_SESSION", "PUT_END sin PUT_BEGIN"); return; }
    uint32_t recv = s_put_sess.received;
    uint32_t exp  = s_put_sess.expected;
    s_put_sess.active = 0;
    if (exp != 0 && recv != exp) {
        wire_v1_send_error(id, "SIZE_MISMATCH", "bytes recibidos != size anunciado"); return;
    }
    put_reply(id, "PUT_END_REPLY", recv, "size");
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

/* Resolución de módulo: base-dir del proyecto, luego /app/ y /lib/ (igual que Pico).
 *
 * H11 — devuelve la RUTA que existe y su tamaño, NO los bytes. Antes resolvía a un
 * puntero, y sostener ese puntero costaba un espejo estático de 64 KB: el fichero
 * entero en RAM sólo para que el loader lo copiase a memory[]. Ahora el llamante
 * abre por esa ruta y lee por trozos. Mismo cambio que cerró #305 en el Pico. */
static fs_status_t v1_resolve_path(const char* name, char* out, size_t out_cap,
                                   uint32_t* size_out) {
    /* H19 — base-dir del proyecto PRIMERO (carpeta del módulo principal): un
     * import resuelve contra /app/<proj>/ antes que el resto del path. Plano
     * (basedir="") o entry absoluto (name[0]=='/') → se salta este candidato. */
    const char* bd = bpvm_fs_basedir();
    if (bd && bd[0] && name[0] != '/') {
        snprintf(out, out_cap, "%s/%s", bd, name);
        if (bpvm_fs_stat(out, size_out) == 0) return FS_OK;
    }
    snprintf(out, out_cap, "%s", name);
    if (bpvm_fs_stat(out, size_out) == 0) return FS_OK;
    snprintf(out, out_cap, "/app/%s", name);
    if (bpvm_fs_stat(out, size_out) == 0) return FS_OK;
    snprintf(out, out_cap, "/lib/%s", name);
    if (bpvm_fs_stat(out, size_out) == 0) return FS_OK;
    return FS_ERR_NOT_FOUND;
}

/* Lector por trozos para el loader: el .mod se queda en el FS. */
static long v1_mod_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    return bpvm_fs_read_at((const char*) user, off, dst, n);
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

#if defined(__riscv)
/* H4 AOT — RAM ejecutable de los .mdn cargados en este RUN. El loader es
 * zero-copy (los thunks apuntan a estos buffers), así que persisten durante el
 * run y se liberan justo después. */
#define MDN_MAX_EXEC 16
static void* s_mdn_exec[MDN_MAX_EXEC];
static int   s_mdn_exec_n;
#endif

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

    char main_path[FS_NAME_LEN]; uint32_t size;
    fs_status_t fs_s = v1_resolve_path(path, main_path, sizeof(main_path), &size);
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

    bpvm_t* vm = bpvm_init(s_vm_buffer, s_vm_buffer_size,
                           s_vm_buffer_size - (uint32_t) vm_stack_region_bytes());
    if (!vm) { send_exited(session, "INTERNAL_ERROR", -1, 0, "bpvm_init failed"); s_active_session = 0; return; }

    v1_sink_ctx_t sink_ctx = { session };
    bpvm_set_output(vm, v1_output_sink, &sink_ctx);

    /* H11 — el .mod se queda en el FS y el loader lo lee por trozos directamente
     * a memory[]. Antes se mirroreaba entero en s_get_scratch (64 KB de .bss). */
    bpvm_status_t ls = bpvm_load_mod_stream(vm, v1_mod_read_at, main_path, size, path);
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
                char dep_path[FS_NAME_LEN]; uint32_t dep_size;
                if (v1_resolve_path(fname, dep_path, sizeof(dep_path), &dep_size) != FS_OK) continue;
                bpvm_status_t ds = bpvm_load_mod_stream(vm, v1_mod_read_at, dep_path,
                                                        dep_size, owner);
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

    /* H4 AOT — registrar funciones AOT baked-in tras link, antes de run (mismo
     * punto que la Pico). En el P4 (RISC-V) esp_aot_register (fuerte, aot_funcs_p4.c)
     * arma los thunks nativos compilados para RISC-V; en el S3 (Xtensa) es weak
     * no-op. Clear primero: cada RUN recarga módulos en direcciones frescas. */
    bpvm_aot_clear();
    esp_aot_register(vm);

#if defined(__riscv)
    /* H4 AOT — .mdn DINÁMICO (RISC-V). Por cada módulo cargado, buscar su .mdn en
     * el FS y, si existe, COPIARLO a RAM EJECUTABLE (la DRAM del ESP32 no ejecuta;
     * el Pico sí, por eso allí el loader es zero-copy desde el buffer del FS) y
     * registrar sus thunks. El gate de arch del loader rechaza un .mdn que no sea
     * RISC-V. El buffer exec persiste durante el run; se libera al terminar. */
    s_mdn_exec_n = 0;
    for (int mi = 0; mi < vm->module_count && s_mdn_exec_n < MDN_MAX_EXEC; mi++) {
        const char* mname = vm->modules[mi].name;
        if (!mname || !mname[0]) continue;
        char mdn_path[80];   /* nombre de módulo (<=63) + ".mdn" + NUL, holgado */
        snprintf(mdn_path, sizeof(mdn_path), "%s.mdn", mname);
        char mdn_real[FS_NAME_LEN]; uint32_t mdn_size;
        if (v1_resolve_path(mdn_path, mdn_real, sizeof(mdn_real), &mdn_size) != FS_OK) continue;
        /* El P4 (RISC-V) NO tiene MALLOC_CAP_EXEC (su RAM interna es unificada =
         * ejecutable; ese cap solo existe en targets con split IRAM/DRAM como
         * Xtensa). RAM interna normal, que la CPU puede ejecutar. */
#ifdef MALLOC_CAP_EXEC
        void* exec = heap_caps_malloc(mdn_size, MALLOC_CAP_EXEC);
#else
        void* exec = heap_caps_malloc(mdn_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#endif
        if (!exec) { ESP_LOGW("AOT", "%s: sin RAM exec (%u B)", mdn_path, (unsigned) mdn_size); continue; }
        /* H11 — del FS DIRECTO a la RAM ejecutable. Antes iba al espejo de 64 KB y
         * de ahí un memcpy: dos copias y un estático permanente para nada. */
        if (bpvm_fs_read(mdn_real, (uint8_t*) exec, mdn_size) != (long) mdn_size) {
            ESP_LOGW("AOT", "%s: lectura incompleta", mdn_path);
            heap_caps_free(exec);
            continue;
        }
        /* Coherencia de cachés (RISC-V): escribir la D-cache a memoria (C2M),
         * invalidar la I-cache, y fence.i para sincronizar el flujo de
         * instrucciones. Sin esto se ejecutaría código rancio/basura. */
        esp_cache_msync(exec, mdn_size,
            ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
        esp_cache_msync(exec, mdn_size,
            ESP_CACHE_MSYNC_FLAG_TYPE_INST | ESP_CACHE_MSYNC_FLAG_INVALIDATE
            | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
        __asm__ volatile ("fence.i" ::: "memory");
        int rc = bpvm_load_mdn(vm, (const uint8_t*) exec, (size_t) mdn_size);
        if (rc == MDN_OK) {
            s_mdn_exec[s_mdn_exec_n++] = exec;
            ESP_LOGI("AOT", "%s: .mdn RISC-V cargado (%u B) en RAM exec %p",
                     mdn_path, (unsigned) mdn_size, exec);
        } else {
            heap_caps_free(exec);
            ESP_LOGW("AOT", "%s: bpvm_load_mdn rc=%d (arch/abi/formato?)", mdn_path, rc);
        }
    }
#endif

    /* Ejecutar. bpvm_run single-thread (el SMP en ESP32 es H4.2+). */
    s_kill_ack_id = -1;
    bpvm_set_poll(vm, esp32_run_poll_cb, NULL);   /* P-run-stop (#257) */

    /* #326 — si el IDE dejó breakpoints o pidió PAUSE antes del RUN, engancha el
     * depurador: aplica los pendientes y registra el callback de pausa. Si no hay
     * nada armado no hace nada (coste cero en un run normal). */
    s_dbgw.session = session;
    bpvm_dbg_wire_arm(&s_dbgw, vm);

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

#if defined(__riscv)
    /* H4 AOT — el run terminó: los thunks .mdn ya no se ejecutan → liberar la RAM
     * ejecutable. bpvm_aot_clear() del próximo RUN limpia el registry. */
    for (int k = 0; k < s_mdn_exec_n; k++) heap_caps_free(s_mdn_exec[k]);
    s_mdn_exec_n = 0;
#endif

    /* #326 — la sesión de depuración muere con el RUN: olvida la vm y los
     * breakpoints pendientes para que la siguiente parta limpia. */
    bpvm_dbg_wire_reset();

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
    /* H11 — sólo interesa la PRIMERA LÍNEA (una ruta), así que se lee la cabeza
     * del fichero a la pila. Antes se cargaba entero al espejo de 64 KB. */
    uint8_t head[FS_NAME_LEN + 8];
    long got = bpvm_fs_read_at("/sys/auto.txt", 0, head, sizeof head);
    if (got <= 0) return;
    const uint8_t* data = head;
    uint32_t size = (uint32_t) got;

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
            /* #329 — este NO_SPACE no tiene NADA que ver con el FS (el volumen
             * puede estar vacío): el bulk no cabe en s_put_buf. Como el IDE sólo
             * ve el código, era indistinguible de "FS lleno" y mandaba a mirar
             * el sitio equivocado. Dejamos las cifras en el log y el mensaje
             * dice de qué buffer habla. */
            char t[40] = {0};
            json_get_str(&obj, "type", t, sizeof t);   /* `type` se lee más abajo */
            log_printf("wire: bulk RECHAZADO %ld B > buffer %u B ('%s') — NO es el FS",
                       bulk, (unsigned) sizeof(s_put_buf), t);
            log_flush();
            {
                char m[96];
                snprintf(m, sizeof m, "bulk %ld B supera el buffer del servidor (%u B)",
                         bulk, (unsigned) sizeof(s_put_buf));
                wire_v1_send_error(json_get_long(&obj, "id", 0), "BULK_TOO_BIG", m);
            }
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
    /* H9 — gestión de placa (env + particiones): mismo núcleo que boardsim/Pico. */
    if (strcmp(type, "STATE") == 0
        || strncmp(type, "ENV_", 4) == 0
        || strncmp(type, "PART_", 5) == 0
        /* #327 — PACK_* también. La capacidad ya estaba en el núcleo compartido
         * (bpvm_bmgr_wire atiende PACK_LS y compañía) y este firmware YA lo
         * enlazaba: lo único que faltaba era encaminarle los comandos, así que
         * el panel de packs del IDE se comía un UNSUPPORTED. Sólo el STM32 lo
         * hacía. */
        || strncmp(type, "PACK_", 5) == 0) {
        /* PACK_BURN_DATA trae BULK. Buffer propio (≤4K) para no pisar el scratch
         * que presta s_put_buf al gestor. CRÍTICO: consumir SIEMPRE los bytes
         * anunciados —quepan o no— o el wire se desincroniza, igual que el PUT. */
        static uint8_t s_burn_chunk[BPVM_PACK_BURN_CHUNK];
        const uint8_t* bulk_ptr = NULL;
        unsigned long  bulk_n   = 0;
        long bulk = json_get_long(&obj, "bulk", -1);
        if (bulk > 0) {
            if ((size_t) bulk > sizeof s_burn_chunk) {
                size_t rem = (size_t) bulk;
                while (rem > 0) {
                    size_t chunk = rem < sizeof s_burn_chunk ? rem : sizeof s_burn_chunk;
                    if (wire_v1_recv_bulk(s_burn_chunk, chunk, sizeof s_burn_chunk) < 0) {
                        wire_v1_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
                    }
                    rem -= chunk;
                }
                wire_v1_send_error(id, "INVALID_PARAM", "chunk demasiado grande");
                return;
            }
            if (wire_v1_recv_bulk(s_burn_chunk, (size_t) bulk, sizeof s_burn_chunk) < 0) {
                wire_v1_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
            }
            bulk_ptr = s_burn_chunk;
            bulk_n   = (unsigned long) bulk;
        }
        board_mgr_esp32_handle(id, &obj, type, s_put_buf, sizeof s_put_buf,
                               bulk_ptr, bulk_n);
        return;
    }
    /* H9 — gating por estado REAL del boot: sin FS (estado<2) los comandos de
     * fichero → NOT_READY; RUN necesita la VM (estado 3). "Sin partición, nada
     * con el sistema de ficheros" (Eduardo). */
    {
        const bpvm_boot_status_t* bs = board_boot_status();
        int is_fs = strcmp(type, "LIST") == 0 || strcmp(type, "STAT") == 0
                 || strcmp(type, "GET")  == 0 || strcmp(type, "PUT")  == 0
                 || strncmp(type, "PUT_", 4) == 0   /* #294 streaming: PUT_BEGIN/DATA/END */
                 || strcmp(type, "DEL")  == 0
                 || strcmp(type, "SAVE") == 0 || strcmp(type, "DF") == 0;
        if (is_fs && bs->state < BPVM_BOOT_FS) {
            wire_v1_send_error(id, "NOT_READY", "FS no disponible: configurar particiones");
            return;
        }
        if (strcmp(type, "RUN") == 0 && bs->state < BPVM_BOOT_APP) {
            wire_v1_send_error(id, "NOT_READY", "VM no disponible en el estado actual del boot");
            return;
        }
    }
    if (strcmp(type, "LIST")  == 0) { handle_list(id, &obj);  return; }
    if (strcmp(type, "STAT")  == 0) { handle_stat(id, &obj);  return; }
    if (strcmp(type, "GET")   == 0) { handle_get(id, &obj);   return; }
    if (strcmp(type, "PUT")   == 0) { handle_put(id, &obj, s_put_buf, bulk_size); return; }
    /* #294 streaming PUT (subida por trozos, ficheros > buffer del wire). */
    if (strcmp(type, "PUT_BEGIN") == 0) { handle_put_begin(id, &obj); return; }
    if (strcmp(type, "PUT_DATA")  == 0) { handle_put_data(id, &obj, s_put_buf, bulk_size); return; }
    if (strcmp(type, "PUT_END")   == 0) { handle_put_end(id, &obj); return; }
    if (strcmp(type, "DEL")   == 0) { handle_del(id, &obj);   return; }
    if (strcmp(type, "SAVE")  == 0) { handle_save(id, &obj);  return; }
    if (strcmp(type, "DF")    == 0) { handle_df(id, &obj);    return; }
    if (strcmp(type, "MKDIR") == 0) { handle_mkdir(id, &obj); return; }
    /* El log NO se gatea por el estado del boot: si el arranque se ha quedado a
     * medias es justo cuando hace falta leerlo (vive en RAM + bpenv, no en el FS). */
    if (strcmp(type, "LOG_DUMP")  == 0) { handle_log_dump(id, &obj);  return; }
    if (strcmp(type, "LOG_CLEAR") == 0) { handle_log_clear(id, &obj); return; }
    if (strcmp(type, "RUN")   == 0) { handle_run(id, &obj);   return; }
    /* #326 DEPURACIÓN pre-RUN: acumular breakpoints / pedir pausa inicial. Los
     * demás (CONTINUE, STEP, LOCALS, STACK, READ_*) los atiende el bucle de
     * pausa del núcleo INLINE, mientras la VM está detenida. */
    {
        bpvm_dbg_cmd_t c;
        dbgw_cmd_from_json(&c, id, &obj, type);
        if (c.kind != BPVM_DBGC_OTHER && bpvm_dbg_wire_handle(&s_dbgw, &c)) return;
    }
    /* PROMPT_RESPONSE huérfano (IO.prompt() aún no emite PROMPT_REQUEST en la
     * VM-C): ack silente, como en el Pico — que el IDE no se quede esperando. */
    if (strcmp(type, "PROMPT_RESPONSE") == 0) {
        wire_v1_send_reply_empty("PROMPT_RESPONSE_REPLY", id);
        return;
    }
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

    /* El mensaje DICE CUÁL. El de antes ("type no implementado") obligaba a
     * adivinar qué comando había mandado el IDE — costó una vuelta entera de
     * diagnóstico. Si el wire crece y una familia se queda atrás, que el error
     * nombre al culpable. */
    {
        char msg[96];
        snprintf(msg, sizeof msg, "comando '%s' no implementado en el firmware ESP32", type);
        wire_v1_send_error(id, "UNSUPPORTED", msg);
    }
}

void repl_esp32_run(void) {
    /* #329 — igual que el Pico ("REPL entry (wire v1)"): marca la frontera entre
     * el arranque y el diálogo con el IDE. Si el log se corta antes de esta
     * línea, el problema es de boot; si aparece, la placa estaba escuchando y lo
     * que venga después es del comando. También deja a la vista el tamaño del
     * buffer de bulk, que es distinto por placa (S3 48K / P4 64K). */
    log_printf("REPL entry (wire v1) — buffer de bulk %u B", (unsigned) sizeof(s_put_buf));
    log_flush();
    for (;;) {
        int n = wire_v1_recv_line(-1, s_line_buf, sizeof(s_line_buf));
        if (n < 0) { wire_v1_send_fatal("PROTOCOL_ERROR", "línea excede WIRE_V1_LINE_MAX"); continue; }
        if (n == 0) continue;
        if (s_line_buf[0] != '{') continue;   /* ignora ruido no-v1 */
        handle_request(s_line_buf, n);
    }
}
