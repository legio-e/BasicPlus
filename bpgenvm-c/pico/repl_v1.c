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
#include "bpvm_fs.h"          /* H19-F1: base-dir por proyecto (bpvm_fs_basedir / bpvm_fs_set_basedir_from_module) */
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */
#include "log.h"
#include "bpvm_dbg_wire.h"   /* #326: el ramo de depuración salió de aquí a src/ */
#include "aot_funcs.h"       /* H3 #160: registro AOT manual antes de run */
#include "mdn_loader.h"      /* H3 #158 fase D: cargar .mdn desde FS */

#include "bpvm.h"
#include "bpvm_internal.h"   /* inspect deps en handle_run */
#include "bpvm_pico.h"       /* INFO: uniqueId/boardName/temp/freq/uptime */
#include "board_desc.h"      /* INFO: variante/gpio/flash/psram del board_desc */
#include "board_mgr_pico.h"  /* H9: gestión de placa (entorno + particiones) */
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
/* INTERINO (16-jul): subido 16K→48K porque Json.mod ya pesa 17.8K (Gui 31.8K)
 * y el bulk se rechazaba con NO_SPACE. Cabe en los 520K de SRAM del RP2350
 * (VM 128K + FS s_data 128K + este 48K). El fix DE VERDAD (Eduardo) es el
 * "Streaming chunk a chunk a fs_put" de arriba — trocear el envío para NO
 * buferizar el fichero entero, así el tamaño deja de estar acotado. Los .mod
 * seguirán creciendo → subir el buffer es un parche con techo. Ver tarea/mejora. */
/* #294/#334 — el buffer del bulk YA NO tiene que dar para el fichero entero:
 * desde que el IDE sube por trozos (PUT_BEGIN/DATA/END, verificado en placa en
 * las 3 familias el 28-jul) sólo necesita el MAYOR de sus dos papeles:
 *   (a) un trozo de streaming            = PUT_STREAM_CHUNK (16 KB)
 *   (b) el scratch del gestor de placa   = 3*BP_ENV_SECTOR + 512
 * (b) NO es igual en todas: el sector del env es la página de borrado, 4 KB en
 * RP2350/ESP32 pero 8 KB en el U5 → el STM32 necesita 25088 B y los otros 12800.
 * Por eso el recorte no es uniforme, y por eso hay una comprobación EN COMPILACIÓN
 * más abajo: si alguien lo baja de más, no compila en vez de romperse en placa. */
#define V1_PUT_BUF_SIZE  (20 * 1024)
static uint8_t s_put_buf[V1_PUT_BUF_SIZE];
/* C99 no tiene _Static_assert: el truco del array de tamaño negativo. */
typedef char bp_chk_put_buf[(V1_PUT_BUF_SIZE >= 16u*1024u &&
                             V1_PUT_BUF_SIZE >= 3u*4096u + 512u) ? 1 : -1];

/* Reparto heap/stacks de la VM. DOS reglas superpuestas:
 *
 *  (a) Eduardo 19-jul — la región de stacks se TOPA en 512 KB y el resto es
 *      heap. Nació por la Metro: con 8 MB de PSRAM, un reparto proporcional
 *      dormía ~4 MB en stacks que usan decenas de KB.
 *  (b) Eduardo 28-jul — del bloque, 25% stacks y 75% heap (antes era mitad y
 *      mitad). Va junto con que la Pico ya no coge 128 KB fijos sino TODA la
 *      SRAM libre (main.c): con el bloque grande, el 25% siguen siendo MÁS
 *      stacks que los 64 KB de antes, así que no se pierde por ningún lado.
 *
 * Cómo queda:
 *   Pico  (343 KB): min(85K, 512K) = 85K  → heap 257 KB + stacks 85 KB.
 *   Metro (8 MB):   manda el tope, 512 KB → heap ~7,5 MB (IGUAL que antes:
 *                   el 25% serían 2 MB, así que el tope sigue mandando).
 *
 * ÚNICO cálculo, y esto es lo importante: lo usan el RUN (bpvm_init), el INFO
 * (mostrar el reparto) y el arranque de main.c. Si se copiara en cada sitio,
 * el día que cambie la regla unos dirían una cosa y otros otra — que es
 * exactamente cómo el INFO acabó enseñando 171+171. */
size_t vm_stack_region_bytes(void) {
    /* La REGLA ya no vive aquí: es bpvm_stack_region_bytes() del núcleo, que
     * comparten las 3 familias (ver bpvm.h). Esto es sólo el envoltorio que le
     * pasa el tamaño de ESTA placa. */
    return bpvm_stack_region_bytes((size_t) s_vm_buffer_size);
}

/* #304 — accesor para que el REPL de texto (repl.c) comparta este buffer en vez
 * de duplicar 32 K (modo-texto y modo-wire son mutuamente excluyentes). */

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
/* ── #326 DEPURACIÓN: cintura del núcleo portable bpvm_dbg_wire ──
 * Todo este ramo (breakpoints, bucle de pausa, LOCALS/STACK/READ_*) vivía AQUÍ y
 * era la única implementación: ESP32 y STM32 se quedaron sin depurador. Se
 * extrajo LITERALMENTE a src/bpvm_dbg_wire.c (cc658bd) y se estrenó en la P4;
 * verificado en placa, el Pico se pasa al núcleo y borra su copia. Aquí queda
 * sólo lo no portable: traducir JSON ↔ comando tipado y prestar los buffers. */
static void dbgw_send(const char* line, size_t len, void* user) {
    (void) user;
    wire_v1_send_line(line, len);
}

static int dbgw_next_cmd(bpvm_dbg_cmd_t* out, void* user) {
    (void) user;
    int n = wire_v1_recv_line(-1, s_line_buf, sizeof s_line_buf);
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
     * IDE salte el PUT por contenido REAL del device.
     * #305: por TROZOS. Antes esto era un fs_get, o sea leer el fichero ENTERO al
     * scratch de 128 KB... por cada fichero del listado, uno detrás de otro. Con
     * bpvm_fs_crc32 el coste es un buffer de 256 B en la pila, y el resultado es
     * bit a bit el mismo (mismo algoritmo, sólo encadenado). */
    uint32_t crc = 0;
    if (bpvm_fs_crc32(name, &crc) != 0) crc = 0;
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
    /* #305 — el STAT sólo publica el TAMAÑO, y para eso leía el fichero ENTERO
     * al scratch. Ahora es lo que siempre debió ser: un stat. */
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no existe");
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
    /* #305 — el GET NO carga el fichero: sólo necesita su TAMAÑO para la cabecera
     * y después lo va escupiendo por trozos. Antes era un fs_get, o sea el
     * fichero ENTERO al scratch de 128 KB para copiarlo acto seguido al wire. */
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no existe");
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
    /* Y los bytes raw, POR TROZOS. El tamaño del trozo es el mismo 256 B que usa
     * littlefs internamente: ni introduce un número nuevo ni fuerza al motor a
     * partir lecturas. Si el FS falla a media transferencia ya no se puede
     * rectificar —la cabecera con `bulk` ya salió— así que se corta y el cliente
     * lo detecta por el bulk incompleto; es lo mismo que pasaría con un cable
     * desconectado, y el wire no tiene forma mejor de decirlo. */
    uint32_t sent = 0;
    while (sent < size) {
        uint8_t chunk[256];
        long n = bpvm_fs_read_at(path, sent, chunk, sizeof chunk);
        if (n <= 0) break;
        wire_v1_send_bulk(chunk, (size_t) n);
        sent += (uint32_t) n;
    }
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
/* #294 streaming PUT — subida por trozos (PUT_BEGIN/PUT_DATA/PUT_END), espejo del
 * BURN de packs. Evita el techo del PUT clasico (s_put_buf=48K): BEGIN crea/trunca,
 * cada DATA apende un chunk, END verifica el tamaño. Una sesion a la vez. */

static struct {
    int      active;
    char     path[FS_NAME_LEN];
    uint32_t received;
    uint32_t expected;   /* size anunciado en BEGIN (0 = no verificar) */
} s_put_sess;

static void put_stream_reply(long id, const char* type, uint32_t val, const char* field) {
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
    put_stream_reply(id, "PUT_BEGIN_REPLY", 0, "received");
}

static void handle_put_data(long id, const json_obj_t* obj, const uint8_t* bulk, size_t bulk_size) {
    (void) obj;
    if (!s_put_sess.active) { wire_v1_send_error(id, "NO_SESSION", "PUT_DATA sin PUT_BEGIN"); return; }
    if (bulk_size > 0) {
        fs_status_t s = fs_put_append(s_put_sess.path, bulk, (uint32_t) bulk_size);
        if (s != FS_OK) {
            s_put_sess.active = 0;
            const char* c; const char* m; map_fs_status(s, &c, &m); wire_v1_send_error(id, c, m); return;
        }
        s_put_sess.received += (uint32_t) bulk_size;
    }
    put_stream_reply(id, "PUT_DATA_REPLY", s_put_sess.received, "received");
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
    put_stream_reply(id, "PUT_END_REPLY", recv, "size");
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
    /* #305 — RENAME nativo. Antes esto era leer-copiar-escribir-borrar: el
     * fichero al scratch, de ahí al buffer del PUT y de ahí al FS otra vez, con
     * un tope artificial de 16 KB "por el buffer". littlefs sabe renombrar él
     * solo —es mover una entrada de directorio— así que no se copia ni un byte,
     * no hay límite de tamaño, y además es ATÓMICO: antes, si fallaba el borrado
     * del origen, quedaban las dos copias. */
    if (bpvm_fs_rename(from, to) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no se pudo renombrar");
        return;
    }
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
    /* H11 — la ARQUITECTURA del código nativo que este firmware ejecuta
     * (MDN_ARCH_ARM/RISCV). Sin esto el IDE no sabe a qué ISA compilar el .mdn
     * de un `.bp` suelto, que no tiene proyecto donde apuntarla. La placa es
     * quien lo sabe, así que lo dice ella. */
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "arch",
                                             (long) bpvm_mdn_host_arch());
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
    /* Reparto de la memoria de la VM (heap/stacks BP) — el MISMO cálculo que
     * usa el RUN (vm_stack_region_bytes), visible sin ejecutar nada. */
    {
        size_t vstack = vm_stack_region_bytes();
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "vmHeapBytes",
                                                 (long)(s_vm_buffer_size - vstack));
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "vmStackBytes", (long) vstack);
    }
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

/* Resolución de módulo: base-dir del proyecto, luego /app/ y /lib/. Replica la
 * lógica de fs_get_resolve en repl.c. La duplicamos aquí en lugar de exportarla
 * para mantener el desacoplamiento entre los dos repls durante la
 * migración. Cuando el legacy se borre, este queda como la canónica. */
/* H11 — devuelve la RUTA que existe y su tamaño, no los bytes. Antes esto
 * resolvía a un puntero, y sostener ese puntero costaba un scratch estático de
 * 128 KB: el fichero entero en RAM sólo para que el loader lo copiase. Ahora el
 * llamante abre por esa ruta y lee por trozos. */
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

    /* H19-F1 — fija el base-dir del proyecto si el módulo vive en /app/<proj>/
     * (el IDE manda la ruta cualificada). Plano (/app/X.mod o nombre suelto) →
     * sin base-dir = modo plano. Se resetea en cada run. */
    bpvm_fs_set_basedir_from_module(path);

    /* 1. Resolver el módulo principal en el FS (ruta + tamaño; los bytes se
     *    leen por trozos al cargarlo). */
    char main_path[FS_NAME_LEN]; uint32_t size;
    fs_status_t fs_s = v1_resolve_path(path, main_path, sizeof(main_path), &size);
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

    /* 3. Init VM + cargar módulo + resolver deps. Reparto heap/stacks =
     * vm_stack_region_bytes (cálculo único, compartido con INFO). */
    size_t stack_region = vm_stack_region_bytes();
    log_printf("vm: memoria %u KB -> heap %u KB + stacks %u KB",
               (unsigned)(s_vm_buffer_size >> 10),
               (unsigned)((s_vm_buffer_size - stack_region) >> 10),
               (unsigned)(stack_region >> 10));
    bpvm_t* vm = bpvm_init(s_vm_buffer, s_vm_buffer_size,
                           s_vm_buffer_size - stack_region);
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

    bpvm_status_t ls = bpvm_load_mod_stream(vm, v1_mod_read_at, main_path, size, path);
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
                char dep_path[FS_NAME_LEN]; uint32_t dep_size;
                if (v1_resolve_path(fname, dep_path, sizeof(dep_path), &dep_size) != FS_OK) continue;
                bpvm_status_t ds = bpvm_load_mod_stream(vm, v1_mod_read_at, dep_path,
                                                         dep_size, owner);
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
        char mdn_real[FS_NAME_LEN]; uint32_t mdn_size;
        fs_status_t fs_s = v1_resolve_path(mdn_path, mdn_real, sizeof(mdn_real), &mdn_size);
        if (fs_s != FS_OK) {
            log_printf("AOT/FS: %s not found (fs=%d) — sin overlay", mdn_path, (int) fs_s);
            continue;
        }
        /* H11 — el .mdn es ZERO-COPY: los thunks se registran como punteros
         * DENTRO de este buffer, así que tiene que seguir vivo y en RAM toda la
         * ejecución. Antes vivía en el scratch de 128 KB; ahora se le reserva
         * de la arena exactamente lo que ocupa (4-alineado, que es lo que pide
         * Thumb-2). Si no cabe, se dice y se sigue sin overlay. */
        uint8_t* mdn_data = bpvm_arena_reserve(vm, mdn_size, 4);
        if (!mdn_data) {
            log_printf("AOT/FS: %s (%u B) no cabe en la arena — sin overlay",
                       mdn_real, (unsigned) mdn_size);
            continue;
        }
        if (bpvm_fs_read(mdn_real, mdn_data, mdn_size) != (long) mdn_size) {
            log_printf("AOT/FS: %s no se pudo leer — sin overlay", mdn_real);
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
    int debugging = bpvm_dbg_wire_armed();
    if (debugging) {
        s_dbgw.session = session;
        bpvm_dbg_wire_arm(&s_dbgw, vm);
        log_printf("RUN/v1: DEBUG mode");
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
    const char* rt_err   = bpvm_runtime_error(vm); /* detalle del RuntimeError no atrapado */
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
        /* Detalle > genérico: link_error, luego el detalle del RuntimeError
         * (p.ej. "referencia a objeto eliminado"), y como último recurso el
         * status genérico. Antes siempre iba el genérico "exit N". */
        const char* emsg = link_err[0] ? link_err
                         : (rt_err[0]   ? rt_err
                                        : bpvm_status_str(rs));
        if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                                   (size_t) off, "errorMessage", emsg);
    }
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf),
                                          (size_t) off);
    if (off >= 0) wire_v1_send_line(s_reply_buf, (size_t) off);

    bpvm_destroy(vm);
    s_active_session = 0;
    /* H6.b.3 — limpiar estado de debug de esta sesión. */
    bpvm_dbg_wire_reset();
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
    /* #305 — de auto.txt sólo se lee la PRIMERA LÍNEA, así que no hay que traerse
     * el fichero: con un buffer de pila del tamaño de un path sobra. */
    uint8_t data[FS_NAME_LEN + 8];
    long rd = bpvm_fs_read("/sys/auto.txt", data, sizeof data);
    if (rd <= 0) return;                                          /* sin autorun */
    uint32_t size = (uint32_t) rd;

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
    /* H9 — gating por estado REAL del boot: sin particiones/FS (estado < 2)
     * los comandos de fichero se RECHAZAN con error claro (nunca cuelgan ni
     * inventan un FS); RUN además necesita la VM (estado 3). Norma de Eduardo:
     * "si no hay partición, nada con el sistema de ficheros". */
    {
        const bpvm_boot_status_t* bs = board_boot_status();
        int is_fs_cmd = strcmp(type, "LIST") == 0 || strcmp(type, "STAT") == 0
                     || strcmp(type, "GET") == 0  || strcmp(type, "PUT") == 0
                     || strncmp(type, "PUT_", 4) == 0   /* #294 streaming: PUT_BEGIN/DATA/END */
                     || strcmp(type, "DEL") == 0  || strcmp(type, "MKDIR") == 0
                     || strcmp(type, "RMDIR") == 0 || strcmp(type, "RENAME") == 0
                     || strcmp(type, "FORMAT") == 0 || strcmp(type, "SAVE") == 0
                     || strcmp(type, "DF") == 0;
        if (is_fs_cmd && bs->state < BPVM_BOOT_FS) {
            char msg[96];
            snprintf(msg, sizeof msg,
                     "FS no disponible en estado %d (%s): configurar particiones",
                     (int) bs->state, bpvm_boot_state_name(bs->state));
            wire_v1_send_error(id, "NOT_READY", msg);
            return;
        }
        if (strcmp(type, "RUN") == 0 && bs->state < BPVM_BOOT_APP) {
            wire_v1_send_error(id, "NOT_READY",
                               "VM no disponible en el estado actual del boot");
            return;
        }
    }
    /* FILES */
    if (strcmp(type, "LIST")     == 0) { handle_list(id, &obj);     return; }
    if (strcmp(type, "STAT")     == 0) { handle_stat(id, &obj);     return; }
    if (strcmp(type, "GET")      == 0) { handle_get(id, &obj);      return; }
    if (strcmp(type, "PUT")      == 0) { handle_put(id, &obj, s_put_buf, bulk_size); return; }
    /* #294 streaming PUT (subida por trozos, ficheros > buffer del wire). */
    if (strcmp(type, "PUT_BEGIN") == 0) { handle_put_begin(id, &obj); return; }
    if (strcmp(type, "PUT_DATA")  == 0) { handle_put_data(id, &obj, s_put_buf, bulk_size); return; }
    if (strcmp(type, "PUT_END")   == 0) { handle_put_end(id, &obj); return; }
    if (strcmp(type, "DEL")      == 0) { handle_del(id, &obj);      return; }
    if (strcmp(type, "MKDIR")    == 0) { handle_mkdir(id, &obj);    return; }
    if (strcmp(type, "RMDIR")    == 0) { handle_rmdir(id, &obj);    return; }
    if (strcmp(type, "RENAME")   == 0) { handle_rename(id, &obj);   return; }
    if (strcmp(type, "FORMAT")   == 0) { handle_format(id, &obj);   return; }
    if (strcmp(type, "SAVE")     == 0) { handle_save(id, &obj);     return; }
    if (strcmp(type, "DF")       == 0) { handle_df(id, &obj);       return; }
    if (strcmp(type, "LOG_DUMP") == 0) { handle_log_dump(id, &obj); return; }
    if (strcmp(type, "LOG_CLEAR")== 0) { handle_log_clear(id, &obj); return; }
    /* BOARD (H9) — gestión de placa: entorno + particiones (núcleo compartido con
     * el boardsim vía bpvm_bmgr_wire). */
    if (strcmp(type, "STATE") == 0
        || strncmp(type, "ENV_", 4) == 0
        || strncmp(type, "PART_", 5) == 0
        /* #327 — PACK_* también. La capacidad ya estaba en el núcleo compartido
         * (bpvm_bmgr_wire atiende PACK_LS y compañía) y este firmware YA lo
         * enlazaba: sólo faltaba encaminarle los comandos, así que el panel de
         * packs del IDE se comía un UNSUPPORTED. El STM32 era el único que lo
         * hacía; ahora las 3. */
        || strncmp(type, "PACK_", 5) == 0) {
        /* El bulk de PACK_BURN_DATA YA lo ha leído el despachador, arriba, en
         * s_put_buf — NO se relee (hacerlo se comería el mensaje siguiente y
         * desincronizaría el wire; ese fue el bug del ESP32 en 0456da8).
         * s_put_buf hace de las dos cosas: el bulk ocupa el principio y el
         * gestor recibe el tramo de DETRÁS, para que no puedan pisarse. */
        size_t used = bulk_size > sizeof s_put_buf ? sizeof s_put_buf : bulk_size;
        board_mgr_pico_handle(id, &obj, type,
                              s_put_buf + used, sizeof s_put_buf - used,
                              bulk_size ? s_put_buf : NULL,
                              (unsigned long) bulk_size);
        return;
    }
    /* TERMINAL */
    if (strcmp(type, "RUN")      == 0) { handle_run(id, &obj);      return; }
    /* DEBUG (H6.b.3 #140) — pre-RUN: acumular breakpoints / pedir pausa
     * inicial. Durante un RUN en modo debug, los comandos READ_INT,
     * READ_STRING, LOCALS, STACK, CONTINUE y STEP los atiende el bucle de
     * pausa del NÚCLEO (bpvm_dbg_wire) inline mientras la VM esta pausada. */
    {
        bpvm_dbg_cmd_t c;
        dbgw_cmd_from_json(&c, id, &obj, type);
        if (c.kind != BPVM_DBGC_OTHER && bpvm_dbg_wire_handle(&s_dbgw, &c)) return;
    }
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

/* --- Bucle de transporte (#305) ------------------------------------
 *
 * Antes vivía en repl.c, mezclado con un REPL de TEXTO (HELLO/LS/PUT/GET/RUN/
 * HELP...) que era el protocolo original del Pico, de cuando aún no existía el
 * wire. Al llegar el wire v1 no se retiró: el bucle hacía peek del primer
 * carácter y elegía protocolo — '{' al wire, cualquier otra cosa a modo texto.
 *
 * Esa convivencia costaba de verdad. El modo texto tenía su PROPIA resolución
 * de módulos (cmd_run duplicaba lo que hace handle_run aquí al lado), su propio
 * GET, su propio PUT... es decir, un segundo camino para las mismas cosas que
 * nadie ejercitaba y que iba divergiendo en silencio. Y sobre todo: mantenía
 * vivos cuatro fs_get, que es justo lo que #305 viene a quitar.
 *
 * El ESP32 y el STM32 nunca tuvieron modo texto — nacieron ya hablando wire v1,
 * con un solo fichero de REPL cada uno. El Pico era el raro por ser el primero.
 * Ahora los tres son iguales.
 *
 * Nada se pierde por funcionalidad: el wire v1 cubre TIME, RESET y BOOTSEL, que
 * era lo único del modo texto sin equivalente obvio. */
void repl_v1_run(void) {
    log_printf("REPL entry (wire v1)");
    log_flush();   /* snapshot del estado de arranque a flash */

    /* Banner x3: si el host abre el COM con retraso se pierde el primero, y sin
     * banner no hay forma de distinguir "la placa no arrancó" de "el puerto no
     * es este". Cada uno con su flush y su delay para forzar rondas de
     * tud_task(). El IDE drena a la conexión, así que no le estorba. */
    for (int i = 0; i < 3; i++) {
        printf("\n=========================================\n");
        printf(" bpvm-pico listo (wire BPVM v1).\n");
        printf("=========================================\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    for (;;) {
        int first = getchar_timeout_us(0);
        if (first < 0) {                      /* nada que leer: ceder la CPU */
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (first != '{') continue;           /* ruido entre requests: descartar */
        repl_v1_handle_request(first);
    }
}
