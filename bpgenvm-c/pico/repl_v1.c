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
#include "bpvm_fs.h"
#include "bpvm_entry.h"   /* #344 — el RUN, escrito una vez */          /* H19-F1: base-dir por proyecto (bpvm_fs_basedir / bpvm_fs_set_basedir_from_module) */
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */
#include "log.h"
#include "bpvm_dbg_wire.h"   /* #326: el ramo de depuración salió de aquí a src/ */
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
#include "bpvm_sqlmem.h"   /* V5/H: motivo + minimo, para el aviso del INFO */
#include "bpvm_sd.h"       /* V5/H1: SD_INFO — la tarjeta se prueba desde aqui */
#include "bpvm_fs_fat.h"   /* V5/H2: SD_MOUNT — la tarjeta como sistema de ficheros */

extern uint8_t* s_vm_buffer;          /* H7.2.b: SRAM interna o ventana PSRAM */
extern uint8_t* s_sqlite_base;        /* V5/H: bloque de la BD (NULL = no hay) */
extern uint32_t s_sqlite_size;
extern int      s_sqlite_res;         /* motivo (bpvm_sqlite_res_t)            */
extern long     s_sqlite_asked_mb;    /* lo que pedia el ENV                   */
extern uint32_t s_vm_buffer_size;
extern TaskHandle_t g_vm_task;        /* #354: para su marca de agua en el INFO */

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
 *   (a) un trozo de streaming            = PUT_STREAM_CHUNK
 *   (b) el scratch del gestor de placa
 *
 * #338 (2-ago) — los dos papeles ADELGAZARON, y con ellos el buffer, de 20 a 8 KB:
 *   (a) el IDE manda trozos de 8 KB (BpvmClient.PUT_STREAM_CHUNK). Si un cliente
 *       viejo mandase más, el despachador responde NO_SPACE y drena: se queja, no
 *       se corrompe.
 *   (b) el gestor ya no pide TRES sectores aquí. Las dos copias del env se las
 *       presta la zona de rascar compartida (board_mgr_pico.c) y de este buffer
 *       sale sólo el sector de trabajo + la respuesta = BP_ENV_SECTOR + reply.
 * La respuesta se queda en 4 KB, que es con lo que el STM32 lleva funcionando
 * desde el primer día — no es una apuesta nueva.
 *
 * El sector del env es la página de borrado y NO es igual en todas (4 KB en
 * RP2350/ESP32, 8 KB en el U5), así que el número sigue siendo por familia. Por
 * eso la comprobación EN COMPILACIÓN de abajo: si alguien lo baja de más, no
 * compila, en vez de romperse en placa. */
#define V1_PUT_BUF_SIZE  (8 * 1024)
static uint8_t s_put_buf[V1_PUT_BUF_SIZE];
/* C99 no tiene _Static_assert: el truco del array de tamaño negativo. */
typedef char bp_chk_put_buf[(V1_PUT_BUF_SIZE >= 8u*1024u &&
                             V1_PUT_BUF_SIZE >= 4096u + 512u) ? 1 : -1];

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
    /* V5/H2 — pero SÓLO del FS propio del micro. Desde que la fachada publica
     * los montajes, este recorrido baja también al volumen montado, y ahí el
     * CRC es puro gasto: sirve para saltarse los PUT, y al montaje no se le
     * sube nada. Sin este corte, cada refresco del árbol se leería la tarjeta
     * ENTERA por SPI para tirar el resultado — con 119 GB delante eso deja de
     * ser un detalle.
     *
     * Se manda -1, que el IDE ya entiende como "este firmware no da CRC" y
     * resuelve comparando tamaños. Y va CON SIGNO: un (uint32_t)-1 impreso
     * como %u sale 4294967295, o sea un CRC de aspecto perfectamente normal
     * que no coincidiría nunca. Mentir con un número creíble es peor que
     * callarse. */
    long crc = -1;
    if (bpvm_fs_en_raiz(name)) {
        uint32_t c = 0;
        crc = (bpvm_fs_crc32(name, &c) == 0) ? (long) c : 0;
    }
    fprintf(stdout, ",\"size\":%u,\"crc\":%ld,\"isDir\":false}", (unsigned) size, crc);
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
/* V5/H1 — SD_INFO: ¿contesta la tarjeta, y qué dice de sí misma?
 *
 * Se dispara desde la consola del IDE y NO en el arranque, por lo mismo que el
 * cargador de packs: lo que puede colgarse se dispara cuando el usuario quiere.
 * Un cuelgue durante un comando se arregla desenchufando una vez; uno en el
 * arranque obliga a regrabar.
 *
 * Y vive AQUÍ y no en `bpvm_bmgr_wire.c` (que es común a las 3 familias) porque
 * hoy sólo la Pico tiene el driver dado de alta: meterlo en el núcleo dejaría al
 * ESP32 y al STM32 sin enlazar por algo que aún no tienen. Cuando la cadena esté
 * probada y se porte en bloque, sube.
 *
 * La respuesta lleva SIEMPRE el peldaño, vaya bien o mal: "no hay tarjeta",
 * "no contesta" y "no arranca" mandan a sitios distintos. */
static void handle_sd_info(long id, const json_obj_t* obj) {
    (void) obj;
    extern bpvm_sd_pines_t s_sd_pines;      /* los rellenó el arranque desde el env */
    extern int             s_sd_hay_config;
    extern char            s_sd_motivo[];

    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "SD_INFO_REPLY", id);

    if (!s_sd_hay_config) {
        if (off >= 0) off = wire_v1_field_bool  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "ok", 0);
        if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "motivo", s_sd_motivo);
        if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
        if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "SD_INFO_REPLY no cabe"); return; }
        wire_v1_send_line(s_reply_buf, (size_t) off);
        return;
    }

    /* 25 MHz es el techo de la clase estándar en modo SPI; subir de ahí es
     * territorio de tarjetas que lo anuncian, y eso se mira en otro momento. */
    bpvm_sd_info_t info;
    bpvm_sd_res_t r = bpvm_sd_init(&s_sd_pines, 25000000, &info);
    log_printf("sd: SD_INFO -> %s", bpvm_sd_res_str(r));

    if (off >= 0) off = wire_v1_field_bool  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "ok", r == BPVM_SD_OK);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "motivo", bpvm_sd_res_str(r));
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "peldano", (long) r);
    /* La TRAZA: los bytes que la tarjeta mandó de verdad esperando su respuesta.
     * Sin esto, "contesta pero no es lo que espero" no dice QUÉ contestó — y la
     * diferencia entre leer 0x00 (linea flotante) y 0x05 (comando ilegal) manda
     * a sitios opuestos. 0xAA en la traza = esa posicion no se llego a leer. */
    {
        static char hex[3 * 8 + 1];
        static const char* D = "0123456789ABCDEF";
        int k = 0;
        for (int i = 0; i < 8; i++) {
            if (i) hex[k++] = ' ';
            hex[k++] = D[(info.traza[i] >> 4) & 0xF];
            hex[k++] = D[info.traza[i] & 0xF];
        }
        hex[k] = '\0';
        if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "ultimoCmd", info.ultimo_cmd);
        if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "traza", hex);
    }
    if (r == BPVM_SD_OK) {
        if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "version",  info.version);
        if (off >= 0) off = wire_v1_field_bool  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "altaCap",  info.alta_cap);
        if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "bloques",  (long) info.bloques);
        if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "producto", info.producto);
        if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "oem",      info.oem);
        if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "fabricante", info.fabricante);
        if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "serie",    (long) info.serie);
        if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "anno",     info.anno);
        if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "mes",      info.mes);

        /* EL SECTOR 0 — y aquí se prueban DOS cosas por el precio de una:
         *
         *  · la RESPUESTA CONOCIDA: un sector de arranque acaba en 55 AA. Que
         *    la tarjeta se identifique demuestra que responde a COMANDOS; esto
         *    demuestra que entrega DATOS, que es otro camino.
         *  · y de regalo, el formato: los bytes 3..10 llevan el nombre de quien
         *    la formateó ("EXFAT   ", "MSDOS5.0", "mkfs.fat"...). O sea que
         *    sabemos en qué viene la tarjeta sin una línea de código de FS —
         *    dato para decidir H2.
         *
         * El buffer es estático: 512 B en la pila de la comm task no caben. */
        static uint8_t sec0[512];
        bpvm_sd_res_t rl = bpvm_sd_leer_bloque(&s_sd_pines, &info, 0, sec0);
        log_printf("sd: sector 0 -> %s", bpvm_sd_res_str(rl));
        if (off >= 0) off = wire_v1_field_bool(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "leeSector0", rl == BPVM_SD_OK);
        if (rl == BPVM_SD_OK) {
            static char firma[8], oemfs[9];
            static const char* D = "0123456789ABCDEF";
            firma[0] = D[(sec0[510] >> 4) & 0xF]; firma[1] = D[sec0[510] & 0xF];
            firma[2] = ' ';
            firma[3] = D[(sec0[511] >> 4) & 0xF]; firma[4] = D[sec0[511] & 0xF];
            firma[5] = '\0';
            if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                                       (size_t) off, "firma", firma);

            /* ¿MBR o VBR? Los bytes 3..10 son el nombre ASCII del formateador en
             * un sector de arranque de FS; en una tabla de particiones son
             * código o ceros. Ese es el desempate, y decide DÓNDE mirar después:
             * con MBR el sistema de ficheros no está aquí, está dentro de una
             * partición. Las SD grandes vienen así de fábrica. */
            int ascii = 1;
            for (int i = 0; i < 8; i++) {
                uint8_t c = sec0[3 + i];
                oemfs[i] = (c >= 32 && c < 127) ? (char) c : '.';
                if (c < 32 || c >= 127) ascii = 0;
            }
            oemfs[8] = '\0';
            if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                                       (size_t) off, "clase", ascii ? "VBR" : "MBR");
            if (ascii) {
                if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                                           (size_t) off, "oemFs", oemfs);
            } else {
                /* La tabla: 4 entradas de 16 B desde el 446. De cada una nos
                 * importan el TIPO (0x0B/0x0C = FAT32, 0x07 = exFAT/NTFS) y el
                 * LBA de arranque — que es donde vive el FS de verdad. */
                int cuantas = 0; uint32_t lba1 = 0; uint8_t tipo1 = 0; uint32_t secs1 = 0;
                for (int i = 0; i < 4; i++) {
                    const uint8_t* e = sec0 + 446 + i * 16;
                    if (e[4] == 0) continue;             /* entrada vacía */
                    if (cuantas == 0) {
                        tipo1 = e[4];
                        lba1  = (uint32_t) e[8] | ((uint32_t) e[9] << 8)
                              | ((uint32_t) e[10] << 16) | ((uint32_t) e[11] << 24);
                        secs1 = (uint32_t) e[12] | ((uint32_t) e[13] << 8)
                              | ((uint32_t) e[14] << 16) | ((uint32_t) e[15] << 24);
                    }
                    cuantas++;
                }
                if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                         (size_t) off, "particiones", cuantas);
                if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                         (size_t) off, "parteTipo", tipo1);
                if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                         (size_t) off, "parteLba", (long) lba1);
                if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                         (size_t) off, "parteSectores", (long) secs1);
                /* Y AHORA sí, el sector de arranque de esa partición: ahí está
                 * el nombre del formateador y su propio 55 AA. */
                if (cuantas > 0 && bpvm_sd_leer_bloque(&s_sd_pines, &info, lba1, sec0)
                                   == BPVM_SD_OK) {
                    for (int i = 0; i < 8; i++) {
                        uint8_t c = sec0[3 + i];
                        oemfs[i] = (c >= 32 && c < 127) ? (char) c : '.';
                    }
                    oemfs[8] = '\0';
                    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                                               (size_t) off, "oemFs", oemfs);
                }
            }
        } else {
            if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                                       (size_t) off, "motivoSector0",
                                                       bpvm_sd_res_str(rl));
        }
    }
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "SD_INFO_REPLY no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* ============================================================ */
/* LIST_DIR — V5/H2: las entradas de UN directorio
 *
 * VERBO NUEVO, no un LIST arreglado. LIST es carga estructural del Run (el
 * IDE salta los PUT comparando el CRC que trae cada entrada), y la norma de
 * V5 es añadir sin mover cimientos. Pero es que además responden a preguntas
 * DISTINTAS, y confundirlas es lo que hacía esto difícil:
 *
 *   · LIST     = "todo el FS interno, con CRC" — lo que necesita el Run.
 *   · LIST_DIR = "los hijos de ESTE directorio" — lo que necesita mirar.
 *
 * Y con una SD montada la diferencia deja de ser estética: LIST recorre el FS
 * ENTERO y calcula el CRC de cada fichero, o sea que sobre una tarjeta de
 * 119 GB se leería la tarjeta entera byte a byte por SPI. Aquí no hay CRC ni
 * recursión a propósito, no por ahorrar.
 *
 * El listado se hace en dos tiempos —fotografiar y luego emitir— y eso NO es
 * un rodeo: el callback de la fachada corre DENTRO del cerrojo del sistema de
 * ficheros, así que escribir al USB desde ahí retendría el cerrojo todo el
 * rato que el host tarde en leer, y cualquier thread BP que tocara un fichero
 * se quedaría esperando. Mismo motivo por el que fs_list ya lo hacía así.
 */

#define LISTDIR_MAX_ENTRIES  96
#define LISTDIR_NAME_MAX     64

typedef struct {
    char     nombres[LISTDIR_MAX_ENTRIES][LISTDIR_NAME_MAX];
    uint32_t tam[LISTDIR_MAX_ENTRIES];
    uint8_t  esdir[LISTDIR_MAX_ENTRIES];
    int      n;
    int      omitidas;      /* las que NO cupieron — jamás en silencio */
} listdir_foto_t;

static void listdir_cb(const char* name, int is_dir, uint32_t size, void* user) {
    listdir_foto_t* f = (listdir_foto_t*) user;
    if (f->n >= LISTDIR_MAX_ENTRIES) { f->omitidas++; return; }
    snprintf(f->nombres[f->n], LISTDIR_NAME_MAX, "%s", name);
    f->tam[f->n]   = size;
    f->esdir[f->n] = (uint8_t) (is_dir ? 1 : 0);
    f->n++;
}

static void handle_list_dir(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        snprintf(path, sizeof path, "/");        /* sin path = la raíz */
    }
    if (path[0] == '\0') { path[0] = '/'; path[1] = '\0'; }

    listdir_foto_t* f = (listdir_foto_t*) bpvm_scratch_take(sizeof(*f), "LIST_DIR");
    if (!f) {
        /* Sin zona NO se contesta un listado vacío: eso se leería como "el
         * directorio está vacío", que es mentira y de las caras. */
        wire_v1_send_error(id, "BUSY", "zona de scratch ocupada");
        return;
    }
    f->n = 0; f->omitidas = 0;
    int r = bpvm_fs_list(path, listdir_cb, f);
    if (r != 0 && f->n == 0) {
        bpvm_scratch_give("LIST_DIR");
        wire_v1_send_error(id, "NOT_FOUND", "no se puede listar");
        return;
    }
    if (f->omitidas) {
        log_printf("fs: LIST_DIR '%s' INCOMPLETO — %d entradas fuera",
                   path, f->omitidas);
        log_flush();
    }

    /* Y ahora sí, FUERA del cerrojo, a escupirlo. */
    fputs("{\"type\":\"LIST_DIR_REPLY\",\"id\":", stdout);
    fprintf(stdout, "%ld,\"entries\":[", id);
    for (int i = 0; i < f->n; i++) {
        if (i) fputc(',', stdout);
        fputs("{\"name\":\"", stdout);
        for (const char* p = f->nombres[i]; *p; p++) {
            if (*p == '"' || *p == '\\') fputc('\\', stdout);
            fputc(*p, stdout);
        }
        fprintf(stdout, "\",\"size\":%u,\"isDir\":%s}",
                (unsigned) f->tam[i], f->esdir[i] ? "true" : "false");
    }
    /* El aviso de truncado viaja al IDE, no sólo al log: quien mira el listado
     * es quien tiene que enterarse de que no está entero. */
    fprintf(stdout, "],\"omitidas\":%d}\n", f->omitidas);
    fflush(stdout);
    bpvm_scratch_give("LIST_DIR");
}

/* ============================================================ */
/* SD_MOUNT — V5/H2: montar la tarjeta como sistema de ficheros
 *
 * VERBO APARTE, y no un añadido a SD_INFO, porque SD_INFO es un DIAGNÓSTICO y
 * un diagnóstico que cambia el estado deja de servir para diagnosticar: con las
 * dos cosas juntas no habría forma de mirar una tarjeta sin montarla, ni de
 * volver a mirarla después de cambiarla.
 *
 * Tampoco lo hace el arranque todavía. Cuando la cadena esté probada en placa,
 * el sitio natural es el estado 3 del boot leyendo la entrada `sd` del ENV — y
 * entonces `/sd` estará sin que nadie escriba nada. Hasta entonces, el hardware
 * que puede no estar se toca cuando el usuario lo pide.
 */

static void handle_sd_mount(long id, const json_obj_t* obj) {
    (void) obj;
    extern bpvm_sd_pines_t s_sd_pines;
    extern int             s_sd_hay_config;
    extern char            s_sd_motivo[];

    int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "SD_MOUNT_REPLY", id);

    if (!s_sd_hay_config) {
        if (off >= 0) off = wire_v1_field_bool  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "ok", 0);
        if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "motivo", s_sd_motivo);
        if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
        if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "SD_MOUNT_REPLY no cabe"); return; }
        wire_v1_send_line(s_reply_buf, (size_t) off);
        return;
    }

    static char motivo[80];
    int r = bpvm_fs_fat_montar(&s_sd_pines, "/sd", motivo, sizeof motivo);
    log_printf("sd: montar -> %s%s%s", r == 0 ? "OK" : "FALLO",
               r == 0 ? "" : " — ", r == 0 ? "" : motivo);

    if (off >= 0) off = wire_v1_field_bool  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "ok", r == 0);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "motivo",
                                               r == 0 ? "montada" : motivo);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "prefijo", "/sd");
    /* El LBA se manda SIEMPRE, también cuando falla: si el montaje se cae con
     * un LBA de 0 en una tarjeta que sí trae MBR, el fallo está en leer la
     * tabla, no en el FAT — y eso son dos sitios distintos. */
    if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "lba",
                                               (long) bpvm_fs_fat_lba_particion());

    if (r == 0) {
        bpvm_fs_fat_resumen_t res;
        if (bpvm_fs_fat_resumen(&res) == 0) {
            log_printf("sd: raiz %d entradas, 1a='%s'", res.entradas_raiz, res.primera);
            if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "etiqueta", res.etiqueta);
            if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "kbTotal",  (long) res.kb_total);
            if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "kbLibres", (long) res.kb_libres);
            if (off >= 0) off = wire_v1_field_long  (s_reply_buf, sizeof(s_reply_buf), (size_t) off, "entradasRaiz", res.entradas_raiz);
            if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "primera", res.primera);
        } else {
            /* Montada pero no recorrible: eso ES un hallazgo, no un detalle. */
            log_printf("sd: montada pero el resumen FALLA");
            if (off >= 0) off = wire_v1_field_bool(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "resumenFalla", 1);
        }
    }

    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "SD_MOUNT_REPLY no cabe"); return; }
    wire_v1_send_line(s_reply_buf, (size_t) off);
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
    /* V5/H — LO QUE EL IDE NECESITA PARA PRE-ENLAZAR UN PACK NATIVO.
     *
     * El pack se enlaza en base 0 y el IDE lo realoja en el PC antes de grabar,
     * así que tiene que saber DÓNDE va a caer. Cuatro datos, y ninguno se puede
     * deducir desde fuera:
     *
     *  · packsXipBase — ⚠️ la dirección que ve la CPU (XIP_BASE + offset de la
     *    partición), NO el offset crudo en flash. Sólo la placa conoce ese mapeo,
     *    y equivocarse ahí desplaza TODO el código por una constante, en
     *    silencio. Es el error más caro posible en este camino.
     *  · packsBytes  — para negarse ANTES de compilar 400 KB que no caben.
     *  · sqliteBase/Bytes — el bloque de RAM que el arranque reservó desde el
     *    ENV (`SQLite=<MB>`). 0 = no hay BD en esta placa.
     *  · floatAbi    — junto a `arch`, el SELLO. `arch` solo no distingue hard
     *    de softfp, y esa discrepancia da números mal sin avisar.
     */
    {
        long packs_xip = 0, packs_len = 0;
        const bpvm_part_layout_t* lay = board_partitions();
        if (lay) {
            const bpvm_part_t* pp = bpvm_part_get(lay, BPVM_PART_PACKS);
            if (pp && pp->size > 0) {
                packs_xip = (long) (XIP_BASE + pp->offset);
                packs_len = (long) pp->size;
            }
        }
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "packsXipBase", packs_xip);
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "packsBytes", packs_len);
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "sqliteBase",
                                                 (long)(uintptr_t) s_sqlite_base);
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "sqliteBytes",
                                                 (long) s_sqlite_size);
        if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                                   (size_t) off, "floatAbi",
                                                   bpvm_mdn_host_float_abi());
        /* El AVISO (decision de Eduardo: vive en el INFO). Van los TRES datos
         * que hacen falta para redactarlo sin mentir y sin que el IDE se invente
         * nada: el MOTIVO (0 bytes por "no se pidio" y por "se pidio poco" son
         * cosas distintas), lo que se PIDIO (para citarlo) y el MINIMO (para que
         * el remedio salga de la placa y no de un numero copiado en Java —
         * si un dia cambia, cambia en un sitio). */
        if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf),
                                                   (size_t) off, "sqliteStatus",
                                                   bpvm_sqlite_res_code(
                                                       (bpvm_sqlite_res_t) s_sqlite_res));
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "sqliteAskedMb",
                                                 s_sqlite_asked_mb);
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "sqliteMinMb",
                                                 (long) BPVM_SQLITE_MIN_MB);
    }
    /* #354 — LO QUE NUNCA LE HEMOS PREGUNTADO A FreeRTOS: cuanto de lo que se le
     * reservo llego a usar de verdad. Esto es SOLO DIAGNOSTICO — no se recorta
     * nada hasta ver los numeros con carga real, y en la duda se deja como esta.
     *
     * Las dos cifras contestan a dos preguntas distintas, y conviene no
     * confundirlas:
     *
     *  · rtosHeapMinFreeBytes = lo que quedo libre de `ucHeap` (32 KB) en el
     *    PEOR momento. OJO: ese heap NO tiene estructuras del kernel, tiene
     *    PILAS DE TAREAS — 16 KB solo la de vm_task, y 4 KB por cada thread BP.
     *    Es decir que lo que sobra ahi es el TECHO DE THREADS, no memoria
     *    muerta: recortarlo baja una capacidad.
     *
     *  · vmTaskStackFreeBytes = lo que le sobro a la PILA de vm_task de sus
     *    16 KB. Aqui si, si sobra mucho, sobra de verdad.
     *
     * uxTaskGetStackHighWaterMark devuelve PALABRAS en FreeRTOS de serie (en
     * ESP-IDF son BYTES — la misma trampa que xTaskCreate). Se multiplica por
     * sizeof(StackType_t) para mandar bytes en los dos casos.
     *
     * Una marca de agua tomada sin haber ejecutado nada no dice nada: hay que
     * mirarla DESPUES de correr algo con carga (GuiColorDemo, JsonDemo, threads). */
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                             (size_t) off, "rtosHeapMinFreeBytes",
                                             (long) xPortGetMinimumEverFreeHeapSize());
    if (g_vm_task != NULL) {
        UBaseType_t words = uxTaskGetStackHighWaterMark(g_vm_task);
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf),
                                                 (size_t) off, "vmTaskStackFreeBytes",
                                                 (long)((size_t) words * sizeof(StackType_t)));
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
    /* #344 — la REGLA vive en el nucleo (bpvm_entry_resolve): basedir del
     * proyecto -> tal cual -> /app -> /lib. Estas 15 lineas estaban COPIADAS
     * palabra por palabra en Pico, ESP32 y STM32. Aqui solo queda la
     * traduccion al fs_status_t que usa el mapeo de errores del wire. */
    return (bpvm_entry_resolve(name, out, out_cap, size_out) == 0)
           ? FS_OK : FS_ERR_NOT_FOUND;
}

/* (El lector por trozos vivía aquí. Se lo llevó #344: ahora el que abre el .mod
 * y lo lee a cachos es bpvm_load_entry_file, en el núcleo, para las cinco.) */

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

    /* #344 — UNA carga: bpvm_load_entry despacha .mod/.pack, lee por trozos,
     * resuelve las dependencias con la regla comun (FS y, si no esta, los packs
     * grabados en XIP) y NOMBRA la que falte. Aqui vivia el bucle de 4 pasadas
     * copiado en las 4 familias, y ademas era una version POBRE: cortaba el
     * import por el primer punto y no sabia nada del pack en ejecucion — que es
     * lo que impedia ejecutar packs en la placa. */
    bpvm_entry_t entry;
    memset(&entry, 0, sizeof entry);
    bpvm_status_t ls = bpvm_load_entry(vm, path, &entry);
    if (ls != BPVM_OK) {
        fputs("{\"type\":\"EXITED\",\"session\":", stdout);
        if (entry.missing[0]) {
            fprintf(stdout, "%ld,\"status\":\"RUNTIME_ERROR\",\"exitCode\":%d,"
                            "\"errorMessage\":\"falta el modulo '%s'\"}\n",
                    session, (int) ls, entry.missing);
        } else {
            fprintf(stdout, "%ld,\"status\":\"RUNTIME_ERROR\",\"exitCode\":%d,"
                            "\"errorMessage\":\"load: %s\"}\n",
                    session, (int) ls, bpvm_status_str(ls));
        }
        fflush(stdout);
        bpvm_destroy(vm);
        s_active_session = 0;
        return;
    }
    if (entry.from_pack)
        log_printf("run: pack '%s' (main=%s)", entry.resolved, entry.main_module);

    /* #355 — INTERRUPTOR DEL RECOLECTOR (idea de Eduardo). `gc=0` en el ENV y la
     * VM no recolecta: ni por umbral, ni al fallar una reserva, ni con el gc()
     * manual (los tres pasan por bpvm_gc, que mira gc_suspended).
     *
     * Para qué: el bucle que se rompe mueve ~170 KB en un heap de 268 KB, o sea
     * que CABE ENTERO sin recolectar. Eso permite partir el experimento en dos
     * dejando TODO lo demás igual —misma placa, misma memoria, mismo programa—
     * y quitando una sola cosa. Si sin GC va limpio, el agotamiento no es la
     * causa; si se rompe igual, el GC queda absuelto y hay que mirar al
     * alocador. Es la diferencia entre medir y opinar.
     *
     * Se lee del ENV y no de una macro para no gastar una grabación por cada
     * cambio de idea. Por defecto va ENCENDIDO: apagarlo tiene que ser un acto
     * deliberado, nunca el estado en que se queda la placa por descuido. */
    if (!board_env_bool("gc", 1)) {
        bpvm_set_gc_enabled(vm, 0);
        log_printf("run: GC DESACTIVADO por el ENV (gc=0) — memoria de un solo uso");
    }

    /* 4b. El registro AOT arranca VACÍO en cada RUN. Antes había aquí dos
     *     etapas de precarga horneadas en la imagen (Bench linkado + un .mdn
     *     embebido); se retiraron en H13 (hallazgo 12) — ver aot_funcs.c.
     *     Lo que llena el registry es el escaneo del FS de justo abajo. */
    bpvm_aot_clear();

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
/* #345 paso 2 — cintura del gate. Cuatro funciones y ni un verbo nuevo: el
 * HELLO que el IDE ya manda al conectar dice "hay alguien", y el KILL que ya
 * manda el Stop dice "no arranques". La política está en el núcleo. */
static void pico_autorun_anuncia(const char* path, int ventana_ms, void* user) {
    (void) user;
    log_printf("autorun: %s arranca en %d ms — Stop en el IDE para cancelar",
               path, ventana_ms);
}

static int pico_autorun_escucha(void* user) {
    (void) user;
    int c = getchar_timeout_us(0);
    if (c < 0) return 0;                       /* nada pendiente */
    int n = wire_v1_recv_line(c, s_line_buf, sizeof(s_line_buf));
    if (n < 0) return 1;                       /* línea rota, pero HAY alguien */
    json_obj_t obj;
    if (json_parse(s_line_buf, (size_t) n, &obj) != 0) return 1;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof(type));
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) {
        /* Se le contesta como a un KILL normal: para el IDE esto es su Stop de
         * siempre, no un caso especial. */
        wire_v1_send_reply_empty("KILL_REPLY", rid);
        return 2;
    }
    if (strcmp(type, "HELLO") == 0) { handle_hello(rid, &obj); return 1; }
    /* Cualquier otra cosa: hay alguien al otro lado, que es lo que se
     * preguntaba. No se contesta — el REPL la atenderá si no se arranca. */
    return 1;
}

static void pico_autorun_espera(int ms, void* user) {
    (void) user; vTaskDelay(pdMS_TO_TICKS(ms));
}

static uint32_t pico_autorun_ahora(void* user) {
    (void) user; return (uint32_t) (xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static const bpvm_autorun_wire_t s_autorun_wire = {
    pico_autorun_anuncia, pico_autorun_escucha,
    pico_autorun_espera,  pico_autorun_ahora, NULL
};

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
    /* #345 — leer y limpiar la primera línea lo hace el núcleo: eran las mismas
     * doce líneas en Pico, ESP32 y STM32. (#305 sigue valiendo: sólo la CABEZA
     * del fichero, que aquí se busca un renglón, no un fichero.) */
    char path[FS_NAME_LEN];
    if (!bpvm_autorun_entry(path, sizeof path)) return;   /* sin autorun */
    log_printf("autorun: %s", path);

    /* Gracia de arranque: el camino del RUN hace log_flush (erase de
     * flash con IRQs off) y la app puede tocar flash también — si eso
     * coincide con la ENUMERACIÓN USB del host, Windows da el puerto
     * por muerto ("dispositivo desconocido"). 2 s dejan a TinyUSB
     * terminar la enumeración antes de arrancar. Boots sin auto.txt
     * no pagan nada (return arriba). */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* #345 paso 2 — la ventana de rescate. Decide el usuario: si el IDE está
     * conectado (su HELLO llega) se le dan 10 s para darle a Stop. Sin nadie
     * escuchando no se espera nada. Los dos verbos son los de siempre.
     *
     * DESPUÉS de la gracia de USB, y no antes: el gate pregunta "¿hay alguien?"
     * mirando el CDC, y antes de que TinyUSB enumere ahí no puede haber llegado
     * nada aunque el IDE esté abierto. Preguntando antes, la respuesta sería
     * siempre "no hay nadie" y la ventana no se abriría nunca — un mecanismo de
     * rescate que sólo funciona cuando no hace falta. */
    if (!bpvm_autorun_gate(&s_autorun_wire, path, 500, 10000)) {
        log_printf("autorun: CANCELADO por el usuario (Stop) — REPL normal");
        return;
    }
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
    if (strcmp(type, "SD_INFO")  == 0) { handle_sd_info(id, &obj);  return; }  /* V5/H1 */
    if (strcmp(type, "SD_MOUNT") == 0) { handle_sd_mount(id, &obj); return; }  /* V5/H2 */
    if (strcmp(type, "LIST_DIR") == 0) { handle_list_dir(id, &obj); return; }  /* V5/H2 */
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
    /* #326 CONTROL: se marca en TODO arranque, haya depurador o no. Es la
     * prueba de que el portador funciona — el equivalente a ejecutar T antes
     * de JsonDemo. Sin control, el silencio del rastro no dice nada. */
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
