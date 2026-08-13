/*
 * stm32_repl.c — REPL wire v1 bare-metal (H9.2 / H9.4).
 *
 * H9.2.a: HELLO/INFO/TIME/PING/RESET + LIST/DF/LOG → el IDE conecta.
 * H9.2.b: + FS en RAM (PUT/GET/DEL/STAT/LIST real/DF real/MKDIR/FORMAT) y
 *         RUN (carga el .mod del FS, lo ejecuta, hace streaming de OUTPUT y
 *         emite EXITED) → "Run on STM32" completo.
 * H9.2.c: RUN resuelve imports (carga <owner>.mod del FS, ≤4 pasadas) + guard.
 * H9.4:   al boot registra los backends de HW (GPIO + info de MCU) y
 *         pre-instala la stdlib core en /lib (stm32_mods_install) → los
 *         programas que importan stdlib resuelven y controlan pines reales.
 *
 * Single-thread, sin FreeRTOS. La salida del programa BP se reenvía como
 * eventos OUTPUT (bytes verbatim, escapados a JSON → paridad de contenido).
 */
#include "stm32_repl.h"
#include "stm32_wire.h"
#include "stm32_fs.h"
#include "board_mgr_stm32.h"  /* H9 — arranque escalonado + STATE/ENV/PART */
#include "flash_layout_stm32.h" /* BP_ENV_SECTOR: s_put_buf hace de scratch del env */
#include "bpvm_pack.h"        /* H3 — BPVM_PACK_BURN_CHUNK (bulk del PACK_BURN_DATA) */
#include "log.h"              /* log persistente de diagnóstico (espejo del Pico) */
#include "bpvm_fs.h"          /* H19 — base-dir / main module por proyecto */
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */
#include "stm32_mods.h"     /* stdlib core embebida (pre-install /lib) */
#include "gpio_stm32.h"     /* stm32_hw_register (backends GPIO + Pico) */
#include "json_min.h"
#include "bpvm.h"
#include "bpvm_internal.h"   /* recorrido de vm->modules[] para los .mdn del AOT */
#include "bpvm_entry.h"      /* #344 — el RUN, escrito una vez */
#include "bpvm_rtc.h"        /* H10 — TIME aplica la hora al RTC (bpvm_rtc_set_now_ms) */
#include "mdn_loader.h"      /* H9.5: overlay AOT .mdn desde el FS (loader compartido) */
#include "aot_registry.h"    /* H9.5: bpvm_aot_clear entre RUNs (registry global) */

#include "main.h"
#include "board.h"              /* placa: BOARD_WIRE_UART, BOARD_NAME, BOARD_SRAM_BYTES, BOARD_LED_* */

#include <string.h>
#include <stdio.h>

#define SERVER_NAME "bpvm-stm32"
/* BOARD_NAME, BOARD_SRAM_BYTES y los BOARD_LED_* los provee board.h (por placa). */

/* Buffers estáticos (NO en stack: el stack C del micro es pequeño). */
static char    s_line[WIRE_LINE_MAX];
/* #294/#334 — el buffer del bulk YA NO tiene que dar para el fichero entero:
 * desde que el IDE sube por trozos (PUT_BEGIN/DATA/END, verificado en placa en
 * las 3 familias el 28-jul) sólo necesita el MAYOR de sus dos papeles:
 *   (a) un trozo de streaming            = PUT_STREAM_CHUNK (16 KB)
 *   (b) el scratch del gestor de placa
 *
 * #338 (2-ago) — los dos papeles ADELGAZARON, y con ellos el buffer, de 28 a 12 KB:
 *   (a) el IDE manda trozos de 8 KB (BpvmClient.PUT_STREAM_CHUNK). Un cliente viejo
 *       que mandase más recibe NO_SPACE y se drena: se queja, no se corrompe.
 *   (b) el gestor ya no pide TRES sectores aquí: las dos copias del env se las presta
 *       la zona de rascar compartida (board_mgr_stm32.c) y de este buffer sale sólo el
 *       sector de trabajo + la respuesta = BP_ENV_SECTOR + reply.
 * El sector del env es la página de BORRADO y aquí son 8 KB (el doble que en
 * RP2350/ESP32), así que esta familia se queda en 12 KB donde las otras bajan a 8:
 * es el silicio, no una excepción. La comprobación EN COMPILACIÓN de abajo sigue
 * marcando el suelo: si alguien lo baja de más, no compila. */
#define V1_PUT_BUF_SIZE  (BP_ENV_SECTOR + 4096u)
static uint8_t s_put_buf[V1_PUT_BUF_SIZE];
typedef char bp_chk_put_buf[(V1_PUT_BUF_SIZE >= 8u*1024u &&
                             V1_PUT_BUF_SIZE >= BP_ENV_SECTOR + 512u) ? 1 : -1];
/* H13 hallazgo 31 (5-ago) — 128 KB era un DESCUIDO, no una decisión: se fijó al
 * nacer el port y nadie volvió a mirarlo. Medido sobre el ELF de la Nucleo: de
 * los 768 KB de RAM el estático total eran ~247 KB (128 de éstos) ⇒ ~520 KB
 * PARADOS, en la placa que tenía el heap MÁS PEQUEÑO del parque (64 KB, contra
 * 96 del S3 y 257 de la Pico). Con 512 KB la regla común (25 % pilas, suelo 64,
 * techo 512) reparte 384 de heap + 128 de pila.
 *
 * El presupuesto que lo justifica, para quien lo revise: 768 total − 512 aquí
 * − ~119 del resto del estático = ~137 KB libres, y el linker sólo exige 20
 * (_Min_Heap_Size 16K + _Min_Stack_Size 4K). Margen de 6×.
 *
 * Lo comparten LAS DOS placas de la familia y la que manda es la Nucleo: la
 * Discovery tiene 3008 KB de RAM, así que aquí le sobra. Si algún día se quiere
 * afinar por placa, el sitio es board.h — no este #define. */
static uint8_t s_vm_mem[512u * 1024u];        /* RAM que gestiona la VM */
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
        "\"capabilities\":[\"META\",\"FILES\",\"TERMINAL\",\"PACKS\"]}",
        id, SERVER_NAME, __DATE__, __TIME__);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_info(long id) {
    /* N-stm32-info — el diálogo INFO del IDE (PicoExplorer.formatInfo) lee el
     * MISMO set de campos que manda la Pico; antes solo enviábamos 7 (y
     * "tempC" en vez de "tempMilliC") → el diálogo salía medio vacío.
     * Valores del NUCLEO-U575ZI-Q (datasheet DS13737): flash 2 MB (sufijo
     * ZI), SRAM 768 KB, sin PSRAM. gpioCount=114 (I/Os del LQFP144).
     * pioCount=0 (PIO es RP2350-only). pwmSlices=28 — el campo lleva
     * SALIDAS PWM, como en los otros ports (Pico 24, ESP32 8): TIM1/TIM8
     * avanzados (4+4) + TIM2/3/4/5 GP (4×4) + TIM15 (2) + TIM16/17 (1+1),
     * sin contar complementarias ni los 4 LPTIM.
     * adcChannels=20 (ADC1 14-bit, "up to 20 multiplexed channels"; hay
     * además un ADC4 12-bit con 19 canales externos). Son datos del CHIP:
     * los backends BP de Pwm/Adc en STM32 aún no están cableados (H9 doc).
     * tempMilliC=0 (el diálogo oculta la línea; el sensor interno queda
     * para más adelante). FLASH_SIZE real del registro por si montan otra
     * variante. */
    uint32_t u0 = *(volatile uint32_t*) (UID_BASE + 0U);
    uint32_t u1 = *(volatile uint32_t*) (UID_BASE + 4U);
    uint32_t u2 = *(volatile uint32_t*) (UID_BASE + 8U);
    unsigned long flash_bytes = (unsigned long) (*(volatile uint16_t*) FLASHSIZE_BASE) * 1024UL;
    char buf[512];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"INFO_REPLY\",\"id\":%ld,"
        "\"uniqueId\":\"%08lX%08lX%08lX\","
        "\"boardName\":\"%s\",\"cpuFreqHz\":%lu,\"uptimeMs\":%lu,"
        "\"tempMilliC\":0,\"resetReason\":\"%s\","
        /* H13 hallazgo 32 — OJO: estos cuatro numeros estan a mano AQUI y otra vez
         * en gpio_stm32.c (el backend que contesta a Pico.GPIO_COUNT() etc), y en
         * GPIO NO COINCIDEN: aqui 114 (las I/O del encapsulado LQFP144) y alli 128
         * (8 puertos x 16 = el rango de numeros de pin que el driver acepta). Los
         * dos son ciertos de cosas distintas, y en el STM32 NINGUNO sirve para
         * recorrer pines, porque se numeran puerto*16+bit y son dispersos.
         *   NO se unifica a pelo: decision de Eduardo (6-ago) = es INFORMATIVO, no
         * afecta a la funcionalidad, y el arreglo bueno es otro -> ver #378 en
         * docs/V5_IDEAS.md: que cada micro DIGA lo que tiene (preguntando a la HAL
         * del fabricante) y que la capa HAL BP tire de eso. STM vende el mismo core
         * en encapsulados con distinto pinado Y distintos perifericos, asi que una
         * imagen unica por familia obliga a preguntarselo a la placa, no a hornearlo. */
        "\"gpioCount\":114,\"pioCount\":0,\"pwmSlices\":28,\"adcChannels\":20,"
        "\"flashBytes\":%lu,\"sramBytes\":%lu,\"psramBytes\":0,"
        "\"fsTotalBytes\":%lu,\"fsUsedBytes\":%lu,"
        /* H13 hallazgo 30 — el reparto de la memoria de la VM, que las otras dos
         * familias ya mandaban y esta no. No es cosmetico: el panel de INFO es el
         * instrumento con el que se lee la tanda de memoria, y sin esta linea
         * habia que ir a buscar el numero al fuente. MISMO calculo que usa el RUN
         * (bpvm_stack_region_bytes sobre el bloque real), no una constante. */
        "\"vmHeapBytes\":%lu,\"vmStackBytes\":%lu,"
        /* H11 — arquitectura del nativo que ejecuta este firmware: el IDE la
         * usa para compilar el .mdn a la ISA correcta sin que nadie la teclee. */
        "\"arch\":%u}",
        id, (unsigned long) u2, (unsigned long) u1, (unsigned long) u0,
        BOARD_NAME, (unsigned long) SystemCoreClock, (unsigned long) HAL_GetTick(),
        stm32_reset_cause(),
        flash_bytes, BOARD_SRAM_BYTES,
        (unsigned long) fs_total_bytes(), (unsigned long) fs_used_bytes(),
        (unsigned long) (sizeof(s_vm_mem) - bpvm_stack_region_bytes(sizeof(s_vm_mem))),
        (unsigned long) bpvm_stack_region_bytes(sizeof(s_vm_mem)),
        (unsigned) bpvm_mdn_host_arch());
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

/* ---- FILES ---- */

static void handle_list(long id, json_obj_t* obj) {
    char prefix[64];
    if (json_get_str(obj, "path", prefix, sizeof(prefix)) < 0) prefix[0] = '\0';
    size_t plen = strlen(prefix);

    /* EFECTO VENTANA (Eduardo, 28-jul) — AQUÍ ESTABA. La respuesta entera se
     * armaba en `char buf[1024]` y, al llenarse, un `break` MUDO dejaba fuera el
     * resto: con ~80 B por entrada cabían unas DOCE. Por eso, cuantos más
     * módulos había en /app, más "desaparecían" los de /lib — /app se comía el
     * presupuesto y a /lib no le llegaba el turno. Los ficheros estaban ahí; lo
     * que se cortaba era el listado, sin decirlo.
     * El Pico y el ESP32 ya EMITEN el listado según lo recorren (cabecera →
     * entradas → cierre), sin buffer para la respuesta completa. El STM32 era el
     * único que no, así que se alinea: sin tope, y de paso las 3 familias hacen
     * lo mismo. Sólo queda un buffer POR ENTRADA, que sí tiene tamaño acotado. */
    char head[64];
    int hn = snprintf(head, sizeof(head),
                      "{\"type\":\"LIST_REPLY\",\"id\":%ld,\"entries\":[", id);
    stm32_wire_send_bulk((const uint8_t*) head, (size_t) hn);
    char ent[192];
    int first = 1;
    int cnt = fs_count();
    for (int i = 0; i < cnt; i++) {
        const char* name; uint32_t size;
        if (fs_entry(i, &name, &size) != 0) continue;
        if (plen > 0 && strncmp(name, prefix, plen) != 0) continue;
        const char* rel = name + plen;             /* basename tras el prefijo */
        /* Solo recortar la '/' del resto cuando HAY prefijo. Con LIST("")
         * (el del árbol del IDE) hay que devolver el nombre COMPLETO tal
         * cual está guardado ("/app/X.mod") — como hace el Pico — o el
         * DEL/GET del árbol mandan el path sin barra y find() exacto da
         * NOT_FOUND. */
        if (plen > 0 && *rel == '/') rel++;
        /* paso 4 cierre — CRC del contenido (== java.util.zip.CRC32) para el
         * skip-PUT por contenido real del device. fs_get por el nombre COMPLETO. */
        /* H11 — CRC por trozos (256 B en la pila dentro de la fachada), no
         * leyendo el fichero entero a un espejo. Igual que Pico y ESP32. */
        uint32_t crc = 0;
        if (bpvm_fs_crc32(name, &crc) != 0) crc = 0;
        int w = snprintf(ent, sizeof(ent),
            "%s{\"name\":\"%s\",\"size\":%lu,\"crc\":%lu,\"isDir\":false,\"mtime\":0}",
            first ? "" : ",", rel, (unsigned long) size, (unsigned long) crc);
        if (w <= 0) continue;
        if ((size_t) w >= sizeof(ent)) {   /* nombre absurdo: sáltalo, pero DILO */
            log_printf("fs: LIST se salta '%s' (no cabe en %u B)", rel, (unsigned) sizeof(ent));
            continue;
        }
        stm32_wire_send_bulk((const uint8_t*) ent, (size_t) w);
        first = 0;
    }
    stm32_wire_send_line("]}", 2);   /* cierra + '\n' */
}

static void handle_stat(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    /* H11 — el STAT sólo quiere el TAMAÑO. */
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) {
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
    /* H11 — el GET no carga el fichero: sólo su TAMAÑO para la cabecera, y
     * luego lo escupe POR TROZOS de 256 B (el mismo trozo interno de littlefs). */
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) {
        stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return;
    }
    char buf[96];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"GET_REPLY\",\"id\":%ld,\"bulk\":%lu}", id, (unsigned long) size);
    if (n > 0) {
        stm32_wire_send_line(buf, (size_t) n);
        uint32_t sent = 0;
        while (sent < size) {
            uint8_t chunk[256];
            long r = bpvm_fs_read_at(path, sent, chunk, sizeof chunk);
            if (r <= 0) break;
            stm32_wire_send_bulk(chunk, (uint32_t) r);
            sent += (uint32_t) r;
        }
    }
}

static void handle_del(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    if (fs_del(path) != 0) { stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return; }
    if (strncmp(path, "/lib/", 5) != 0) fs_save();   /* /lib se re-provee al boot */
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
    /* Persistir sólo lo que sobrevive al reset de forma útil: /lib lo re-instala
     * el embebido al boot, así que un PUT a /lib (el IDE lo hace cada Run) no
     * necesita flash → evita un erase+program por ejecución. */
    if (strncmp(path, "/lib/", 5) != 0) fs_save();
    reply_empty("PUT_REPLY", id);
}

/* #294 streaming PUT — subida por trozos (PUT_BEGIN/PUT_DATA/PUT_END), espejo del
 * BURN de packs. BEGIN crea/trunca, cada DATA apende (lee su propio bulk), END
 * verifica el tamaño y persiste UNA vez (fs_save por chunk seria un erase+program
 * por trozo). Una sesion a la vez. */
static struct {
    int      active;
    char     path[64];
    uint32_t received;
    uint32_t expected;
} s_put_sess;

static void reply_put_field(const char* type, long id, uint32_t val, const char* field) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf), "{\"type\":\"%s\",\"id\":%ld,\"%s\":%lu}",
                     type, id, field, (unsigned long) val);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_put_begin(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    if (fs_put(path, NULL, 0) != 0) { stm32_wire_send_error(id, "NO_SPACE", "FS lleno"); return; }
    s_put_sess.active   = 1;
    s_put_sess.received = 0;
    s_put_sess.expected = (uint32_t) json_get_long(obj, "size", 0);
    strncpy(s_put_sess.path, path, sizeof(s_put_sess.path) - 1);
    s_put_sess.path[sizeof(s_put_sess.path) - 1] = '\0';
    reply_put_field("PUT_BEGIN_REPLY", id, 0, "received");
}

static void handle_put_data(long id, json_obj_t* obj) {
    long bulk = json_get_long(obj, "bulk", -1);
    if (bulk < 0) { stm32_wire_send_error(id, "INVALID_PARAM", "missing bulk"); return; }
    /* CONSUMIR SIEMPRE el bulk (aunque falle luego) para no desincronizar el wire. */
    if ((size_t) bulk > sizeof(s_put_buf)) {
        size_t rem = (size_t) bulk;
        while (rem > 0) {
            size_t chunk = rem < sizeof(s_put_buf) ? rem : sizeof(s_put_buf);
            if (stm32_wire_recv_bulk(s_put_buf, chunk) != 0) { stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return; }
            rem -= chunk;
        }
        s_put_sess.active = 0;
        stm32_wire_send_error(id, "NO_SPACE", "chunk mayor que el buffer"); return;
    }
    if (stm32_wire_recv_bulk(s_put_buf, (size_t) bulk) != 0) { stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return; }
    /* bulk ya consumido → ahora validar sesion + escribir. */
    if (!s_put_sess.active) { stm32_wire_send_error(id, "NO_SESSION", "PUT_DATA sin PUT_BEGIN"); return; }
    if (bulk > 0 && fs_put_append(s_put_sess.path, s_put_buf, (uint32_t) bulk) != 0) {
        s_put_sess.active = 0;
        stm32_wire_send_error(id, "NO_SPACE", "FS lleno"); return;
    }
    s_put_sess.received += (uint32_t) bulk;
    reply_put_field("PUT_DATA_REPLY", id, s_put_sess.received, "received");
}

static void handle_put_end(long id, json_obj_t* obj) {
    (void) obj;
    if (!s_put_sess.active) { stm32_wire_send_error(id, "NO_SESSION", "PUT_END sin PUT_BEGIN"); return; }
    uint32_t recv = s_put_sess.received;
    uint32_t exp  = s_put_sess.expected;
    s_put_sess.active = 0;
    if (exp != 0 && recv != exp) { stm32_wire_send_error(id, "SIZE_MISMATCH", "bytes != size"); return; }
    /* Persistir UNA sola vez (no por chunk). /lib lo re-instala el embebido al boot. */
    if (strncmp(s_put_sess.path, "/lib/", 5) != 0) fs_save();
    reply_put_field("PUT_END_REPLY", id, recv, "size");
}

static void handle_format(long id, json_obj_t* obj) {
    char confirm[8];
    if (json_get_str(obj, "confirm", confirm, sizeof(confirm)) < 0 ||
        strcmp(confirm, "YES") != 0) {
        stm32_wire_send_error(id, "MISSING_CONFIRM", "confirm:\"YES\""); return;
    }
    fs_format();
    fs_save();                       /* H9.3: persistir el formateo (FS vacío) */
    reply_empty("FORMAT_REPLY", id);
}

/* Sink que escapa cada chunk del log y lo escribe RAW → streaming como el Pico
 * (header + chunks + cierre), sin buffer para el log entero. */
static char s_log_esc[1600];   /* chunk de 256 B escapado (peor caso ~6x + NUL) */
static void log_chunk_sink(const char* data, size_t len, void* user) {
    (void) user;
    if (stm32_wire_json_escape(data, len, s_log_esc, sizeof(s_log_esc)) < 0) return;
    stm32_wire_write(s_log_esc, strlen(s_log_esc));
}

static void handle_log_dump(long id) {
    char hdr[64];
    int n = snprintf(hdr, sizeof(hdr),
        "{\"type\":\"LOG_DUMP_REPLY\",\"id\":%ld,\"text\":\"", id);
    if (n > 0) stm32_wire_write(hdr, (size_t) n);
    log_dump(log_chunk_sink, NULL);
    stm32_wire_write("\"}\n", 3);
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

/* P-run-stop (#257) + P-autorun (#256) — wire durante el run (la VM
 * invoca el poll entre quanta; bare-metal single-thread → el poll puede
 * responder directamente):
 *   KILL  → ack diferido (KILL_REPLY tras parar, antes del EXITED) + 1.
 *   HELLO → HELLO_REPLY inmediato — el IDE puede conectarse con un
 *           (auto)run en marcha y ofrecer Stop.
 *   otra  → error BUSY inmediato. */
static long s_kill_ack_id = -1;

static int stm32_run_poll_cb(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    int c = stm32_wire_getchar();
    if (c < 0) return 0;
    int n = stm32_wire_recv_line(c, s_line, sizeof(s_line));
    if (n < 0) return 0;                      /* rota/estancada: descartar */
    json_obj_t obj;
    if (json_parse(s_line, (size_t) n, &obj) != 0) return 0;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof(type));
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) { s_kill_ack_id = rid; return 1; }
    if (strcmp(type, "HELLO") == 0) { handle_hello(rid); return 0; }
    stm32_wire_send_error(rid, "BUSY", "ejecución en curso: solo HELLO/KILL");
    return 0;
}

/* Resuelve un nombre de módulo en el FS: prueba base-dir/name, name, /app/name,
 * /lib/name (el IDE sube las deps a /lib). 0 OK, -1 no está.
 *
 * H11 — devuelve la RUTA que existe y su tamaño, NO los bytes. Antes resolvía a
 * un puntero, y sostenerlo costaba un espejo estático del tamaño de la arena del
 * FS viejo: 496 KB en la Discovery, 96 KB en la Nucleo. Ahora el llamante abre
 * por esa ruta y lee por trozos. Mismo cambio que #305 en el Pico. */
static int stm32_fs_resolve(const char* name, char* out, size_t out_cap, uint32_t* size) {
    /* #344 — la REGLA vive en el núcleo (bpvm_entry_resolve): basedir del
     * proyecto → tal cual → /app → /lib. Estas 15 líneas estaban COPIADAS
     * palabra por palabra en Pico, ESP32 y STM32; aquí ya sólo se traduce el
     * 0/-1 que espera el resto de este fichero. */
    return bpvm_entry_resolve(name, out, out_cap, size);
}

/* Núcleo del RUN — compartido entre el comando RUN del wire (id >= 0)
 * y el autorun de boot (#256, id < 0). Con id < 0 no hay cliente: sin
 * RUN_REPLY, y una ruta inexistente enciende el LED rojo (el idioma de
 * diagnóstico de este port) en vez de mandar un error al vacío. */
static void run_module_path(const char* path, long id) {
    /* H19-F1 — fija el base-dir/main-module del proyecto si el módulo vive en
     * /app/<proj>/ (el IDE manda la ruta cualificada). Plano → sin base-dir. */
    bpvm_fs_set_basedir_from_module(path);
    char main_path[80]; uint32_t size;
    if (stm32_fs_resolve(path, main_path, sizeof(main_path), &size) != 0) {
        if (id >= 0) stm32_wire_send_error(id, "NOT_FOUND", "no existe");
        else         BOARD_LED_ERR_ON();            /* autorun: ruta mala */
        return;
    }

    long session = ++s_session;
    s_run_session = session;
    if (id >= 0) { /* RUN_REPLY con la sesión, ANTES de ejecutar */
        char buf[80];
        int n = snprintf(buf, sizeof(buf),
            "{\"type\":\"RUN_REPLY\",\"id\":%ld,\"session\":%ld}", id, session);
        if (n > 0) stm32_wire_send_line(buf, (size_t) n);
    }

    /* Antes pasaba 0 = 'default de bpvm_init' (mitad y mitad): la única de las
     * tres familias que ni siquiera tenía la regla. Ahora usa LA MISMA que Pico
     * y ESP32. Con 128 KB manda el suelo ⇒ 64/64, idéntico a hoy. */
    size_t stack_region = bpvm_stack_region_bytes(sizeof(s_vm_mem));
    bpvm_t* vm = bpvm_init(s_vm_mem, sizeof(s_vm_mem), sizeof(s_vm_mem) - stack_region);
    if (!vm) { BOARD_LED_ERR_ON(); emit_exited(session, "INTERNAL_ERROR", -1, 0); return; }
    bpvm_set_output(vm, v1_output_sink, NULL);

    uint32_t t0 = HAL_GetTick();
    BOARD_LED_RUN_ON();                           /* LED RUN = ejecutando un programa */
    /* #344 — UNA carga: bpvm_load_entry despacha .mod/.pack, lee por trozos
     * (H11: el .mod se queda en el FS), resuelve las dependencias con la regla
     * común —FS primero, y si no está, los packs grabados en XIP, código EN
     * FLASH y cero copia— y NOMBRA la que falte.
     *
     * De las tres familias ésta era la que MÁS tenía (era la única con packs y
     * con el guard anti-hard-fault), y es justo lo que se subió al núcleo: aquí
     * desaparecen ~70 líneas sin perder ni una capacidad. El guard sigue siendo
     * imprescindible en bare-metal — un CALL_EXT sin resolver cuelga el micro —
     * sólo que ahora lo hace `first_missing` para las cinco. */
    bpvm_entry_t entry;
    memset(&entry, 0, sizeof entry);
    bpvm_status_t st = bpvm_load_entry(vm, path, &entry);
    char missing[40] = {0};
    if (entry.missing[0]) strncpy(missing, entry.missing, sizeof(missing) - 1);
    if (entry.from_pack)
        log_printf("run: pack '%s' (main=%s)", entry.resolved, entry.main_module);

    /* H9.5 — overlay AOT: para cada módulo cargado, si el FS tiene su
     * <Modulo>.mdn (PIC Thumb-2 de build_mdn.sh — mismo -mcpu=cortex-m33
     * que el RP2350, la U575 es el mismo core), registra sus thunks
     * zero-copy apuntando al buffer del FS (RAM, dirección estable durante
     * el RUN). El registry AOT es GLOBAL → clear antes de cada RUN para no
     * arrastrar thunks de una sesión anterior (buffers FS ya movidos).
     * Tolerante: sin .mdn o rc != OK → se ejecuta interpretado, sin más.
     * Nota U575: el código ejecuta desde SRAM (S-bus); el ICACHE del U5
     * cachea la ruta de flash (C-bus), así que no hace falta invalidación
     * — confirmar en placa con el primer smoke (fib_native). */
    bpvm_aot_clear();
    if (st == BPVM_OK && !missing[0]) {
        for (int mi = 0; mi < vm->module_count; mi++) {
            const char* mname = vm->modules[mi].name;
            if (!mname || !mname[0]) continue;
            char mdn_path[72];   /* name[64] + ".mdn" + NUL: sin -Wformat-truncation */
            snprintf(mdn_path, sizeof(mdn_path), "%s.mdn", mname);
            char mdn_real[80]; uint32_t mdn_size;
            if (stm32_fs_resolve(mdn_path, mdn_real, sizeof(mdn_real), &mdn_size) != 0) continue;
            /* H11 — el .mdn es ZERO-COPY: los thunks se registran como punteros
             * DENTRO de este buffer, así que tiene que seguir vivo y en RAM toda
             * la ejecución. Antes vivía en el espejo del fs_get (la razón de que
             * el espejo fuera permanente y del tamaño de la arena); ahora se le
             * reserva de la arena de la VM exactamente lo que ocupa, 4-alineado
             * que es lo que pide Thumb-2. Mismo remedio que el Pico. */
            uint8_t* mdn_data = bpvm_arena_reserve(vm, mdn_size, 4);
            if (!mdn_data) {
                log_printf("AOT: %s (%lu B) no cabe en la arena — sin overlay",
                           mdn_real, (unsigned long) mdn_size);
                continue;
            }
            if (bpvm_fs_read(mdn_real, mdn_data, mdn_size) != (long) mdn_size) {
                log_printf("AOT: %s no se pudo leer — sin overlay", mdn_real);
                continue;
            }
            int mrc = bpvm_load_mdn(vm, mdn_data, (size_t) mdn_size);
            /* Visible en la consola del IDE: sin esto, un fallo de carga
             * (p.ej. buffer desalineado) caía a interpretado EN SILENCIO. */
            char mmsg[96];
            int mn = snprintf(mmsg, sizeof(mmsg), "[AOT] %s %s (rc=%d)\n",
                              mdn_path, (mrc == 0) ? "OK" : "FALLO -> interpretado", mrc);
            if (mn > 0) v1_output_sink(mmsg, (size_t) mn, NULL);
        }
    }

    /* P-run-stop (#257) — poll del wire entre quanta (KILL/HELLO/BUSY). */
    s_kill_ack_id = -1;
    if (st == BPVM_OK && !missing[0]) bpvm_set_poll(vm, stm32_run_poll_cb, NULL);

    if (st == BPVM_OK && !missing[0]) st = bpvm_run(vm);
    BOARD_LED_RUN_OFF();
    uint32_t dt = HAL_GetTick() - t0;

    /* P-run-stop — ack diferido del KILL, antes del EXITED. */
    bpvm_set_poll(vm, NULL, NULL);
    if (s_kill_ack_id >= 0) { reply_empty("KILL_REPLY", s_kill_ack_id); s_kill_ack_id = -1; }

    if (missing[0]) {
        BOARD_LED_ERR_ON();
        char buf[160];
        int n = snprintf(buf, sizeof(buf),
            "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"RUNTIME_ERROR\","
            "\"exitCode\":-2,\"elapsedMs\":0,"
            "\"errorMessage\":\"falta el modulo %s en el FS (stdlib no embebida?)\"}",
            session, missing);
        if (n > 0) stm32_wire_send_line(buf, (size_t) n);
    } else {
        if (st != BPVM_OK && st != BPVM_KILLED) BOARD_LED_ERR_ON();
        const char* link_err = bpvm_link_error(vm);   /* paso 4 — "" salvo fallo de link */
        if (link_err[0]) {
            char buf[320];
            int n = snprintf(buf, sizeof(buf),
                "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"LINK_ERROR\","
                "\"exitCode\":%d,\"elapsedMs\":%lu,\"errorMessage\":\"%s\"}",
                session, (int) st, (unsigned long) dt, link_err);
            if (n > 0) stm32_wire_send_line(buf, (size_t) n);
        } else {
            /* #406 — el DETALLE del error de ejecucion, no solo la categoria.
             *
             * Esta familia se habia quedado corta: el Pico y los ESP32 ya
             * miraban bpvm_runtime_error() y el STM32 solo trataba link_err, asi
             * que en el IDE salia «RUNTIME_ERROR» pelado y el texto —el motivo
             * real— se perdia. Es el patron de siempre: el comun crecio y una
             * copia privada no.
             *
             * El mensaje lo rellena la VM tanto si el error lo lanza ella como
             * si es un `throw` de clase de usuario sin atrapar (eso ultimo es lo
             * que anadio #406 en src/exceptions.c). */
            const char* rt_err = bpvm_runtime_error(vm);
            if (st != BPVM_OK && st != BPVM_KILLED && rt_err[0]) {
                char buf[320];
                int n = snprintf(buf, sizeof(buf),
                    "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"RUNTIME_ERROR\","
                    "\"exitCode\":%d,\"elapsedMs\":%lu,\"errorMessage\":\"%s\"}",
                    session, (int) st, (unsigned long) dt, rt_err);
                if (n > 0) stm32_wire_send_line(buf, (size_t) n);
            } else {
                emit_exited(session,
                            (st == BPVM_OK)     ? "OK"
                          : (st == BPVM_KILLED) ? "KILLED" : "RUNTIME_ERROR",
                            (st == BPVM_KILLED) ? 130 : (int) st, dt);
            }
        }
    }
    bpvm_destroy(vm);
}

static void handle_run(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    run_module_path(path, id);
}

/* P-autorun (#256) — si existe /sys/auto.txt, ejecuta el módulo de su
 * primera línea por el mismo camino que un RUN del wire. El poll del
 * run atiende HELLO/KILL → el IDE puede conectar y parar la app. */
/* #345 paso 2 — cintura del gate. Cuatro funciones y ni un verbo nuevo: el
 * HELLO que el IDE ya manda al conectar dice "hay alguien", y el KILL que ya
 * manda el Stop dice "no arranques". La política está en el núcleo. */
static void stm32_autorun_anuncia(const char* path, int ventana_ms, void* user) {
    (void) user;
    log_printf("autorun: %s arranca en %d ms — Stop en el IDE para cancelar",
               path, ventana_ms);
}

static int stm32_autorun_escucha(void* user) {
    (void) user;
    int c = stm32_wire_getchar();
    if (c < 0) return 0;                       /* nada pendiente */
    int n = stm32_wire_recv_line(c, s_line, sizeof(s_line));
    if (n < 0) return 1;                       /* línea rota, pero HAY alguien */
    json_obj_t obj;
    if (json_parse(s_line, (size_t) n, &obj) != 0) return 1;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof(type));
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) { reply_empty("KILL_REPLY", rid); return 2; }
    if (strcmp(type, "HELLO") == 0) { handle_hello(rid); return 1; }
    return 1;                                  /* hay alguien: basta con eso */
}

static void stm32_autorun_espera(int ms, void* user) {
    (void) user; HAL_Delay((uint32_t) ms);
}

static uint32_t stm32_autorun_ahora(void* user) {
    (void) user; return HAL_GetTick();
}

static const bpvm_autorun_wire_t s_autorun_wire = {
    stm32_autorun_anuncia, stm32_autorun_escucha,
    stm32_autorun_espera,  stm32_autorun_ahora, NULL
};

static void autorun_boot(void) {
    /* #345 — leer y limpiar la primera línea lo hace el núcleo. (H11 sigue
     * valiendo: sólo la CABEZA del fichero, nunca un espejo.) */
    char path[64];
    if (!bpvm_autorun_entry(path, sizeof path)) return;   /* sin autorun */

    /* #345 paso 2 — la ventana de rescate. Decide el usuario. */
    if (!bpvm_autorun_gate(&s_autorun_wire, path, 500, 10000)) {
        log_printf("autorun: CANCELADO por el usuario (Stop) — REPL normal");
        return;
    }
    log_printf("autorun: %s", path);
    run_module_path(path, -1);
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

    /* H9 — gestión de placa (STATE/ENV_x/PART_x + H3 PACK_x): el host configura el
     * env/particiones cuando el arranque no llegó al FS, y consulta/graba la zona
     * de packs. Las replies las pone bpvm_bmgr_wire (idénticas al boardsim y a las
     * otras placas). s_put_buf presta el scratch; el bulk de PACK_BURN_DATA va a
     * su buffer propio (chunk ≤ 4K) para no pisar el scratch. */
    if (strcmp(type, "STATE") == 0
        || strncmp(type, "ENV_", 4) == 0
        || strncmp(type, "PART_", 5) == 0
        || strncmp(type, "PACK_", 5) == 0) {
        static uint8_t s_burn_chunk[BPVM_PACK_BURN_CHUNK];
        const uint8_t* bulk_ptr = NULL;
        unsigned long  bulk_n   = 0;
        long bulk = json_get_long(&obj, "bulk", -1);
        if (bulk > 0) {
            /* CRÍTICO: consumir SIEMPRE los bytes anunciados (como el PUT) para
             * no desincronizar el wire, quepan o no. */
            if ((size_t) bulk > sizeof s_burn_chunk) {
                size_t rem = (size_t) bulk;
                while (rem > 0) {
                    size_t chunk = rem < sizeof s_burn_chunk ? rem : sizeof s_burn_chunk;
                    if (stm32_wire_recv_bulk(s_burn_chunk, chunk) != 0) {
                        stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
                    }
                    rem -= chunk;
                }
                stm32_wire_send_error(id, "INVALID_PARAM", "chunk demasiado grande");
                return;
            }
            if (stm32_wire_recv_bulk(s_burn_chunk, (size_t) bulk) != 0) {
                stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
            }
            bulk_ptr = s_burn_chunk;
            bulk_n = (unsigned long) bulk;
        }
        board_mgr_stm32_handle(id, &obj, type, s_put_buf, sizeof s_put_buf, bulk_ptr, bulk_n);
        return;
    }
    /* H9 — gating: sin FS montado, las ops de fichero no valen; sin VM, el RUN no.
     * HELLO/INFO/STATE/PING/TIME/RESET van SIEMPRE (para conectar y configurar). */
    {
        int st = (int) board_boot_status()->state;
        int is_fs = (strcmp(type, "LIST")  == 0 || strcmp(type, "STAT") == 0 || strcmp(type, "DF")  == 0
                  || strcmp(type, "GET")   == 0 || strcmp(type, "PUT")  == 0 || strcmp(type, "DEL") == 0
                  || strncmp(type, "PUT_", 4) == 0   /* #294 streaming: PUT_BEGIN/DATA/END */
                  || strcmp(type, "MKDIR") == 0 || strcmp(type, "FORMAT") == 0);
        if (is_fs && st < BPVM_BOOT_FS) {
            stm32_wire_send_error(id, "NOT_READY", "FS no montado (configura particiones)");
            return;
        }
        if (strcmp(type, "RUN") == 0 && st < BPVM_BOOT_APP) {
            stm32_wire_send_error(id, "NOT_READY", "VM no lista (configura particiones)");
            return;
        }
    }

    if      (strcmp(type, "HELLO")     == 0) handle_hello(id);
    else if (strcmp(type, "INFO")      == 0) handle_info(id);
    else if (strcmp(type, "PING")      == 0) reply_empty("PONG", id);
    else if (strcmp(type, "TIME")      == 0) {   /* H10 — aplica la hora al RTC HW */
        long epochSec = json_get_long(&obj, "epochSec", -1);
        if (epochSec >= 0) bpvm_rtc_set_now_ms((int64_t) epochSec * 1000LL);
        reply_empty("TIME_REPLY", id);
    }
    else if (strcmp(type, "LIST")      == 0) handle_list(id, &obj);
    else if (strcmp(type, "STAT")      == 0) handle_stat(id, &obj);
    else if (strcmp(type, "DF")        == 0) handle_df(id);
    else if (strcmp(type, "GET")       == 0) handle_get(id, &obj);
    else if (strcmp(type, "PUT")       == 0) handle_put(id, &obj);
    /* #294 streaming PUT (subida por trozos, ficheros > buffer del wire). */
    else if (strcmp(type, "PUT_BEGIN") == 0) handle_put_begin(id, &obj);
    else if (strcmp(type, "PUT_DATA")  == 0) handle_put_data(id, &obj);
    else if (strcmp(type, "PUT_END")   == 0) handle_put_end(id, &obj);
    else if (strcmp(type, "DEL")       == 0) handle_del(id, &obj);
    else if (strcmp(type, "MKDIR")     == 0) reply_empty("MKDIR_REPLY", id);
    else if (strcmp(type, "FORMAT")    == 0) handle_format(id, &obj);
    else if (strcmp(type, "RUN")       == 0) handle_run(id, &obj);
    /* P-run-stop (#257) — KILL en idle: nada que matar (el útil llega
     * DURANTE un RUN y lo atiende stm32_run_poll_cb). */
    else if (strcmp(type, "KILL")      == 0)
        stm32_wire_send_error(id, "NO_SESSION", "no hay programa en ejecución");
    else if (strcmp(type, "LOG_DUMP")  == 0) handle_log_dump(id);
    else if (strcmp(type, "LOG_CLEAR") == 0) {
        log_clear_ram();
        log_clear_flash();
        reply_empty("LOG_CLEAR_REPLY", id);
    }
    else if (strcmp(type, "RESET")     == 0) {
        log_printf("RESET (wire): reinicio");
        log_flush();                 /* persiste la sesión antes de reiniciar */
        reply_empty("RESET_REPLY", id);
        HAL_Delay(50);
        NVIC_SystemReset();
    } else {
        char msg[96];
        snprintf(msg, sizeof(msg), "type '%s' no implementado (H9.2)", type);
        stm32_wire_send_error(id, "UNSUPPORTED", msg);
    }
}

/* #353 — sink de diagnóstico de la VM: al log persistente. */
static void diag_al_log(const char* linea) { log_printf("%s", linea); }

void stm32_repl_run(void) {
    /* H9 — arranque ESCALONADO: identidad → particiones del env → FS → VM, parando
     * en la 1ª capa que falla. Sin particiones/FS el climb se queda abajo y el host
     * conduce (Gestión de placa: proponer defaults → aplicar → reset). Sustituye al
     * fs_load() de región fija. */
    log_init();                     /* recupera el log de la sesión anterior (post-mortem) */
    /* #353 — lo que dice la VM (deps que faltan, packs, veredicto del guardián
     * de #339) al log. Aquí el problema era el opuesto al de la Pico: sin un
     * `_write` retargeteado, el stderr del núcleo se PERDÍA. Esto devuelve al
     * log los avisos de pack que este port tenía a mano antes de #344. */
    bpvm_diag_set_sink(diag_al_log);
    board_mgr_stm32_boot();
    const bpvm_boot_status_t* bs = board_boot_status();
    log_printf("boot: estado %d (%s)%s%s", (int) bs->state, bpvm_boot_state_name(bs->state),
               bs->degraded ? " DEGRADED: " : "", bs->degraded ? bs->reason : "");
    /* stdlib core embebida en /lib SOLO con el FS montado (si el climb no llegó al
     * FS, no hay dónde instalarla; el host configura particiones primero). */
    if (bs->state >= BPVM_BOOT_FS) {
        stm32_mods_install();       /* stdlib core -> /lib (si-ausente) */
        stm32_fs_register_bpvm();   /* #247 — readFile/writeFile/... sobre el FS */
        log_printf("fs: %u/%u bytes usados", (unsigned) fs_used_bytes(),
                   (unsigned) fs_total_bytes());
    }
    stm32_hw_register();            /* backends de HW (GPIO, info de MCU) — siempre */
    log_flush();                    /* persiste la historia del boot (post-mortem tras corte) */

    /* FIFO RX/TX (8 bytes): absorbe el hueco de procesado entre la línea JSON
     * y los bytes bulk que la siguen → PUT fiable aunque la CPU vaya lenta.
     * (El fix definitivo del timing es subir el reloj a 160 MHz.) */
    HAL_UARTEx_SetTxFifoThreshold(BOARD_WIRE_UART, UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(BOARD_WIRE_UART, UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_EnableFifoMode(BOARD_WIRE_UART);

#if defined(BOARD_WIRE_IRQn)
    /* RX por IRQ → ring (V3/H5.2 DK2; H10 también Nucleo). La FIFO de 8B (≈700µs)
     * no basta cuando el lazo deja el UART sin sondear ms (bombeo de LVGL en
     * Gui.run()) y se perdían los primeros bytes del KILL. Con la IRQ drenando a un
     * ring de 256B, getchar() no pierde nada y el GUI puede dormir entre frames
     * (__WFI). Tras EnableFifoMode para que RXFNE refleje la FIFO ya activa.
     * La placa opta definiendo BOARD_WIRE_IRQn en board.h. */
    stm32_wire_rx_irq_enable();
#endif

    stm32_wire_send_cstr("=== bpvm-stm32 REPL (wire v1) listo ===");
    {   /* H10 — causa del último reset (diagnóstico; revela WDT/soft/power-on). */
        char rc[64];
        snprintf(rc, sizeof(rc), "reset cause: %s", stm32_reset_cause());
        stm32_wire_send_cstr(rc);
    }

    /* P-autorun (#256) — el wire ya está vivo: si la app de auto.txt se
     * queda en bucle, el IDE puede conectar (HELLO) y matarla (KILL). */
    autorun_boot();

    uint32_t last_blink = HAL_GetTick();
    for (;;) {
        int c = stm32_wire_getchar();
        if (c == '{') dispatch(c);

        uint32_t now = HAL_GetTick();
        if (now - last_blink >= 500U) {     /* heartbeat */
            last_blink = now;
            BOARD_LED_BEAT_TOGGLE();
        }
    }
}
