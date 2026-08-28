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
#include "bpvm_repl.h"      /* V6/U3: los verbos comunes del REPL */
#include "json_min.h"
#include "fs.h"
#include "bpvm_fs.h"
#include "bpvm_entry.h"   /* #344 — el RUN, escrito una vez */          /* H19-F1: base-dir por proyecto (bpvm_fs_basedir / bpvm_fs_set_basedir_from_module) */
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */
#include "log.h"
#include "bpvm_dbg_wire.h"   /* #326: el ramo de depuración salió de aquí a src/ */
#include "mdn_loader.h"      /* H3 #158 fase D: cargar .mdn desde FS */
#include "bpvm_mdn_scan.h"   /* V5/H4: el escaneo del .mdn, compartido */

#include "bpvm.h"
#include "bpvm_internal.h"   /* inspect deps en handle_run */
#include "bpvm_pico.h"       /* INFO: uniqueId/boardName/temp/freq/uptime */
#include "board_desc.h"      /* INFO: variante/gpio/flash/psram del board_desc */
#include "board_mgr_pico.h"  /* H9: gestión de placa (entorno + particiones) */
#include "bpvm_rtc.h"        /* TIME: set epoch */
#include "aot_registry.h"    /* H3 #160: bpvm_aot_clear */
#include "pack_pico.h"       /* V5/H4: cargar los packs nativos en el primer Run */

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
#include "bpvm_sd_blk.h"   /* V5/H6: la misma SD como dispositivo de bloque    */
#include "bpvm_listdir.h" /* V5/H6: LIST_DIR, nucleo comun a las familias    */
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


/* ============================================================ */
/* V6/U3 g8 — LIST vive en el común. El recorrido de allí no necesita la zona de
 * scratch (ésta la pedía para TODO el listado) ni tiene tope de entradas por
 * directorio. A cambio emite dentro del callback de la fachada, o sea con el
 * lock del FS cogido; ver la nota del commit. */


/* ============================================================ */
/* STAT */


/* ============================================================ */
/* GET — reply con bulk. */


/* ============================================================ */
/* PUT — request lleva bulk. */


/* ============================================================ */





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
/* V5/H2 — LA TARJETA PUEDE ENTRAR Y SALIR EN CALIENTE
 *
 * El montaje del arranque es de UNA VEZ, así que meter la tarjeta después no
 * hacía nada: nadie volvía a mirar. Y sacarla con el volumen montado era peor
 * —FatFs seguía creyendo que estaba— así que la siguiente escritura iría a un
 * bus mudo y podría dejar la FAT a medias. Eso corrompe tarjetas de verdad.
 *
 * Se mira el pin de detección desde el hueco en que la comm task no tiene nada
 * que hacer. UNA lectura de GPIO cada medio segundo; en el resto de vueltas ni
 * se entra. Y NO se toca el bus SPI hasta que el pin dice que hay algo.
 *
 * Medio segundo es de la escala de un dedo metiendo una tarjeta, no de la del
 * micro: más a menudo no se notaría y sólo gastaría.
 */
#define SD_VIGILA_CADA_MS  500

static void sd_vigilar_tick(void) {
    extern bpvm_sd_pines_t s_sd_pines;
    extern int             s_sd_hay_config;
    if (!s_sd_hay_config) return;

    static int64_t s_proxima = 0;
    int64_t ahora = bpvm_platform_now_ms();
    if (ahora < s_proxima) return;
    s_proxima = ahora + SD_VIGILA_CADA_MS;

    char motivo[80];
    if (bpvm_fs_fat_vigilar(bpvm_sd_blk(&s_sd_pines), "/sd", motivo, sizeof motivo)) {
        /* Sólo se escribe cuando CAMBIA algo: un log que repite "sigue igual"
         * dos veces por segundo es un log que nadie lee. */
        log_printf("sd: %s", motivo[0] ? motivo : "cambio en el zocalo");
        log_flush();
    }
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

/* V5/H6 paso 3 — el cuerpo del verbo se fue a `src/bpvm_listdir.c`, común a
 * las familias. Aquí queda lo único que es DE ESTA PLACA: por dónde sale el
 * texto (stdout, que es su transporte) y con qué palabras se contesta el error.
 *
 * Lo movió el P4: usa el despachador del S3 y se quedó sin LIST_DIR, así que el
 * IDE le caía al LIST con CRC — o sea, leerse la tarjeta entera para pintar un
 * árbol. */
static void sink_stdout(const char* txt, size_t n, void* user) {
    (void) user;
    fwrite(txt, 1, n, stdout);
}

static void handle_list_dir(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        snprintf(path, sizeof path, "/");        /* sin path = la raíz */
    }

    switch (bpvm_listdir_emitir(path, id, sink_stdout, NULL, NULL)) {
    case BPVM_LISTDIR_OCUPADO:
        wire_v1_send_error(id, "BUSY", "zona de scratch ocupada");
        return;
    case BPVM_LISTDIR_NO_LISTA:
        wire_v1_send_error(id, "NOT_FOUND", "no se puede listar");
        return;
    case BPVM_LISTDIR_OK:
        break;
    }
    /* El cierre de línea lo pone el transporte: es lo único que cada familia
     * hace distinto, y por eso el núcleo no lo escribe. */
    fputc('\n', stdout);
    fflush(stdout);
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
    int r = bpvm_fs_fat_montar(bpvm_sd_blk(&s_sd_pines), "/sd", motivo, sizeof motivo);
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


/* ============================================================ */
/* MKDIR / RMDIR — no-op en FS plano. */



/* ============================================================ */
/* RENAME — copia + delete. */


/* ============================================================ */
/* FORMAT — borra todo el FS RAM. Requiere confirm:"YES". */


/* ============================================================ */
/* SAVE — persiste el FS RAM a flash. */


/* ============================================================ */
/* DF — stats del FS. */


/* ============================================================ */
/* LOG_DUMP — text del log persistente, embebido como string JSON. */

typedef struct {
    int first;        /* 1 = todavía no se ha escrito el header */
} log_ctx_t;



/* ============================================================ */
/* META — INFO, RESET, BOOTSEL. (TIME y PING viven en el común.) */

/* ── V6/U3 g10 — INFO: los 18 comunes por aqui, los 11 propios por info_extra ──
 * El reparto no es arbitrario: en `info` va lo que TODA placa tiene (serie,
 * reloj, memoria, FS) y en `info_extra` lo que solo significa algo aqui — la
 * zona de packs con su direccion XIP, el bloque de SQLite y las marcas de agua
 * de FreeRTOS. Meter esto ultimo en el struct comun obligaria a las otras
 * familias a declarar campos que no saben rellenar. */
static void pico_repl_info(bpvm_repl_info_t* out) {
    /* Estaticos porque el struct guarda punteros y el comun los lee DESPUES. */
    static char unique[20], board[16];
    bpvm_pico_unique_id(unique, sizeof unique);
    bpvm_pico_board_name(board, sizeof board);
    const board_desc_t* bd = board_desc();
    size_t vstack = vm_stack_region_bytes();

    out->unique_id    = unique;
    out->board_name   = board;
    /* H11 — la ARQUITECTURA del nativo que esta imagen ejecuta. Sin esto el IDE
     * no sabe a que ISA compilar el .mdn de un `.bp` suelto. Lo dice la placa. */
    out->arch         = (unsigned) bpvm_mdn_host_arch();
    out->cpu_hz       = (unsigned long) bpvm_pico_cpu_freq_hz();
    out->uptime_ms    = (unsigned long) bpvm_pico_uptime_ms();
    out->reset_reason = bpvm_pico_reset_cause();
    /* El wire v1 no lleva floats (el parser del cliente rechaza decimales), asi
     * que la temperatura viaja en milesimas: 20400 -> 20,4 grados en la UI. */
    out->temp_milli_c = (long) (bpvm_pico_temp_c() * 1000.0f);
    out->gpio_count   = bd->gpio_count;
    out->pio_count    = bd->pio_count;
    /* PWM en SALIDAS, no slices: cada slice tiene 2 canales (A/B) -> 24 en
     * RP2350, que es la cifra que anuncian las placas. El campo del wire
     * conserva el nombre historico "pwmSlices". */
    out->pwm_slices   = bd->pwm_slices * 2;
    out->adc_channels = bd->adc_channels;
    out->flash_bytes  = (unsigned long) bd->flash_bytes;
    out->sram_bytes   = 520UL * 1024UL;
    out->psram_bytes  = (unsigned long) bd->psram_bytes;
    /* El MISMO reparto que usa el RUN (vm_stack_region_bytes), visible sin
     * ejecutar nada. */
    out->vm_heap_bytes  = (unsigned long) (s_vm_buffer_size - vstack);
    out->vm_stack_bytes = (unsigned long) vstack;
    out->fs_total_bytes = (unsigned long) fs_total_bytes();
    out->fs_used_bytes  = (unsigned long) fs_used_bytes();
}

static int pico_repl_info_extra(char* buf, unsigned long buf_max, int off) {
    size_t max = (size_t) buf_max;
#define X_S(k, v) do { off = wire_v1_field_string(buf, max, (size_t) off, (k), (v)); \
                       if (off < 0) return off; } while (0)
#define X_L(k, v) do { off = wire_v1_field_long(buf, max, (size_t) off, (k), (long) (v)); \
                       if (off < 0) return off; } while (0)
    const board_desc_t* bd = board_desc();
    char variant[2]; variant[0] = bd->variant; variant[1] = '\0';
    X_S("variant", variant);

    /* V5/H — LO QUE EL IDE NECESITA PARA PRE-ENLAZAR UN PACK NATIVO.
     * El pack se enlaza en base 0 y el IDE lo realoja en el PC antes de grabar,
     * asi que tiene que saber DONDE va a caer. Y ninguno de estos se deduce
     * desde fuera:
     *  · packsXipBase — ⚠️ la direccion que ve la CPU (XIP_BASE + offset de la
     *    particion), NO el offset crudo en flash. Solo la placa conoce ese
     *    mapeo, y equivocarse ahi desplaza TODO el codigo por una constante, en
     *    silencio: es el error mas caro posible en este camino.
     *  · packsBytes  — para negarse ANTES de compilar 400 KB que no caben.
     *  · sqliteBase/Bytes — el bloque de RAM que el arranque reservo desde el
     *    ENV (`SQLite=<MB>`). 0 = no hay BD en esta placa.
     *  · floatAbi    — junto a `arch`, el SELLO. `arch` sola no distingue hard
     *    de softfp, y esa discrepancia da numeros mal sin avisar. */
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
        X_L("packsXipBase", packs_xip);
        X_L("packsBytes",   packs_len);
    }
    X_L("sqliteBase",  (long) (uintptr_t) s_sqlite_base);
    X_L("sqliteBytes", (long) s_sqlite_size);
    X_S("floatAbi",    bpvm_mdn_host_float_abi());
    /* El AVISO (decision de Eduardo: vive en el INFO). Van los TRES datos que
     * hacen falta para redactarlo sin mentir y sin que el IDE se invente nada:
     * el MOTIVO (0 bytes por "no se pidio" y por "se pidio poco" son cosas
     * distintas), lo que se PIDIO (para citarlo) y el MINIMO (para que el
     * remedio salga de la placa y no de un numero copiado en Java). */
    X_S("sqliteStatus",  bpvm_sqlite_res_code((bpvm_sqlite_res_t) s_sqlite_res));
    X_L("sqliteAskedMb", s_sqlite_asked_mb);
    X_L("sqliteMinMb",   (long) BPVM_SQLITE_MIN_MB);

    /* #354 — LO QUE NUNCA LE HEMOS PREGUNTADO A FreeRTOS: cuanto de lo que se le
     * reservo llego a usar. SOLO DIAGNOSTICO. Dos cifras que contestan a dos
     * preguntas distintas y conviene no confundir:
     *  · rtosHeapMinFreeBytes = lo que quedo libre de `ucHeap` en el PEOR
     *    momento. OJO: ese heap no tiene estructuras del kernel, tiene PILAS DE
     *    TAREAS — 16 KB solo vm_task y 4 KB por thread BP. Lo que sobra ahi es
     *    el TECHO DE THREADS, no memoria muerta: recortarlo baja una capacidad.
     *  · vmTaskStackFreeBytes = lo que le sobro a la pila de vm_task. Aqui si,
     *    si sobra mucho, sobra de verdad.
     * uxTaskGetStackHighWaterMark devuelve PALABRAS en FreeRTOS de serie (en
     * ESP-IDF son BYTES — la misma trampa que xTaskCreate), de ahi el producto.
     * Y una marca tomada sin haber ejecutado nada no dice nada: se mira DESPUES
     * de correr algo con carga. */
    X_L("rtosHeapMinFreeBytes", xPortGetMinimumEverFreeHeapSize());
    if (g_vm_task != NULL) {
        UBaseType_t words = uxTaskGetStackHighWaterMark(g_vm_task);
        X_L("vmTaskStackFreeBytes", (size_t) words * sizeof(StackType_t));
    }
#undef X_S
#undef X_L
    return off;
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

static int s_latido_on = 0;   /* V5/H4: lo enciende el ENV `latido=1` */

/* V5/H4 — EL LATIDO. Para un cuelgue, saber que "no responde" no vale de nada:
 * hace falta saber DONDE esta girando. El poll ya se llama periodicamente
 * durante el run, asi que cuesta un contador.
 *
 * Sale cada 256 pasadas para no ahogar el log —que en placa es flash— y dice el
 * pc y el estado de cada thread vivo. Si el cuelgue esta en el interprete,
 * veras el pc moverse (bucle) o clavado (bloqueado). Si NO sale ni una linea,
 * se colgo ANTES del primer quantum, que tambien es un dato.
 *
 * Se enciende con `aot_trace=1` en el ENV; apagado no cuesta ni una division. */
static void latido(bpvm_t* vm) {
    static unsigned n = 0;
    if ((n++ & 0xFFu) != 0) return;
    for (int i = 0; i < BPVM_MAX_THREADS; i++) {
        if (vm->threads[i].status == BPVM_THREAD_TERMINATED) continue;
        log_printf("latido: th%d pc=%lu estado=%d",
                   i, (unsigned long) vm->threads[i].pc,
                   (int) vm->threads[i].status);
    }
}

static int pico_run_poll_cb(bpvm_t* vm, void* user) {
    (void) user;
    if (s_latido_on) latido(vm);
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
        bpvm_repl_dispatch(type, rid, &obj);   /* attach en caliente */
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
/* V5/H4 — LOS PACKS SE CARGAN AQUÍ, EN EL PRIMER Run.
 *
 * Un pack nativo publica su API al arrancar (`bp_pack_init` → `publica`), y un
 * programa BP que use esa API la busca por su marca. O sea que alguien tiene
 * que haber cargado el pack ANTES de que corra el programa. Ese alguien es
 * esto.
 *
 * ─── POR QUÉ EN EL Run Y NO EN EL ARRANQUE ───
 *
 * Decisión de Eduardo, de la tanda del 7-ago (commit 262bb01): el salto al
 * pack es el único paso que puede colgar. Un cuelgue durante un Run se arregla
 * desenchufando UNA vez; uno en el arranque se repite en CADA arranque y
 * obliga a regrabar. Con la Metro, que no tiene botón de reset, eso no es una
 * molestia: es perder la placa hasta reflashearla.
 *
 * ─── UNA VEZ, Y SÓLO SI SALIÓ BIEN ───
 *
 * El pack se carga y se queda; volver a saltar a su entrada intentaría publicar
 * la misma marca dos veces y el punto de encuentro lo rechazaría (-3). Por eso
 * la bandera se pone SÓLO cuando salió bien: si no hay pack grabado, el
 * siguiente Run lo vuelve a intentar — que es justo lo que quieres después de
 * grabarlo, sin reiniciar.
 *
 * Y no se trata como error: un programa que no use packs no tiene por qué
 * llevar ninguno. Quien los necesite se enterará por su nombre cuando el puente
 * no encuentre la marca. */
static void packs_cargar_una_vez(void) {
    static int s_ok = 0;
    if (s_ok) return;

    int32_t r = pack_pico_cargar();
    if (r >= 0) {
        s_ok = 1;
        log_printf("packs: cargado, la entrada devolvio %ld", (long) r);
    } else {
        /* r<0 = no se saltó; el detalle (cuántos candidatos, por qué peldaño)
         * ya lo dejó `pack_pico_cargar` en el log. Aquí sólo se dice que se
         * sigue sin pack, que es lo que el usuario tiene que saber. */
        log_printf("packs: sin pack utilizable (peldano %ld) — se sigue sin el",
                   (long) -r);
    }
}

/* ── V5/H4: la CINTURA del escaneo de .mdn en esta familia ─────────────────
 *
 * El bucle, el orden de búsqueda y los mensajes están en `bpvm_mdn_escanear`
 * (src/bpvm_mdn_scan.c), compartidos. Aquí queda lo único que es del RP2350:
 * de dónde sale la RAM ejecutable cuando el `.mdn` viene del FS.
 *
 * Del pack no pasa por aquí — se ejecuta EN SITIO desde la flash y no gasta
 * arena. Ésa es, de hecho, una de las ganancias de meterlo en el pack. */
static const uint8_t* pico_mdn_del_fs(void* user, const char* nombre,
                                      uint32_t* len) {
    bpvm_t* vm = (bpvm_t*) user;
    char     real[FS_NAME_LEN];
    uint32_t size = 0;
    if (v1_resolve_path(nombre, real, sizeof(real), &size) != FS_OK) return NULL;

    /* H11 — ZERO-COPY: los thunks se registran como punteros DENTRO de este
     * buffer, así que tiene que seguir vivo toda la ejecución. Se reserva de la
     * arena exactamente lo que ocupa, 4-alineado (lo que pide Thumb-2). */
    uint8_t* dst = bpvm_arena_reserve(vm, size, 4);
    if (!dst) {
        /* Se dice AQUÍ y no arriba: para el escaneo esto es "no está", y un
         * descarte por tamaño en silencio no se distinguiría de no tenerlo. */
        log_printf("AOT: %s (%u B) no cabe en la arena — sin overlay",
                   real, (unsigned) size);
        return NULL;
    }
    if (bpvm_fs_read(real, dst, size) != (long) size) {
        log_printf("AOT: %s no se pudo leer — sin overlay", real);
        return NULL;
    }
    *len = size;
    return dst;
}

static void pico_mdn_decir(void* user, const char* msg) {
    (void) user;
    log_printf("%s", msg);
}

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

    /* V5/H4 — antes de nada, los packs: si el programa usa una API de pack,
     * tiene que estar publicada cuando llegue la primera llamada. Ver arriba
     * por qué aquí y no en el arranque. */
    packs_cargar_una_vez();

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

    /* [V6/N1.4] EL REGISTRO AOT SE VACIA AQUI, ANTES DE CARGAR — y el sitio
     * importa. Estaba mas abajo, despues de la carga, porque la suposicion era
     * «lo que llena el registry es el escaneo del FS». Con el bloque nativo
     * EMBEBIDO en el .mod eso dejo de ser cierto: el modulo registra sus thunks
     * mientras se carga, y un clear() posterior los borraba.
     *
     * Se vio en placa el 23-ago y de la peor manera: el log decia «1/1 thunks
     * registrados (zero-copy)» y aun asi `fib(28) AOT` tardaba lo mismo que
     * interpretado. Todo correcto, y sin efecto.
     *
     * Vaciar antes de cargar es ademas lo que siempre se quiso decir: el registro
     * arranca limpio en cada RUN y luego ACUMULA — lo que traiga cada modulo y lo
     * que encuentre el barrido de .mdn sueltos. */
    bpvm_aot_clear();

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
        } else if (entry.fallo[0]) {
            /* #421 — el PORQUÉ, con la ruta. Antes aquí salía «load: IO error»
             * y el IDE mostraba `exit 1 (IO error)`: el firmware sabía lo que
             * había pasado y no lo contaba. */
            fprintf(stdout, "%ld,\"status\":\"RUNTIME_ERROR\",\"exitCode\":%d,"
                            "\"errorMessage\":\"load: %s\"}\n",
                    session, (int) ls, entry.fallo);
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

    /* 4b. (El vaciado del registro AOT se hizo ANTES de cargar — ver arriba.
     *     Aqui borraria los thunks que el propio .mod trajo embebidos.) */

    /* 4c. V5/H4 — el .mdn de cada módulo, del FS o del PACK.
     *
     * Esto era un bucle de 35 líneas aquí, y otros tres iguales en las otras
     * familias. Ahora el bucle, el orden de búsqueda (el puente sigue a su
     * módulo) y los mensajes están en `bpvm_mdn_escanear`; de esta familia
     * queda sólo la cintura de arriba. */
    log_printf("AOT: buscando el .mdn de %d modulos (FS + pack)", vm->module_count);
    bpvm_mdn_escanear(vm, pico_mdn_del_fs, pico_mdn_decir, vm);
    /* V5/H4 — QUE MODULOS HAY Y DE DONDE SALIERON, antes de arrancar.
     *
     * Sin esto no se ve si Core y Str llegaron a entrar, ni por que via. Y era
     * justo el hueco: la resolucion de dependencias solo hablaba al fallar. */
    log_printf("modulos cargados: %d", vm->module_count);
    for (int mi = 0; mi < vm->module_count; mi++) {
        log_printf("  [%d] %-12s %s cb=0x%08lX main=%ld",
                   mi, vm->modules[mi].name,
                   vm->modules[mi].en_pack ? "PACK" : "FS  ",
                   (unsigned long) vm->modules[mi].cb,
                   (long) vm->modules[mi].main_offset);
    }
    s_latido_on = board_env_bool("latido", 0);
    if (s_latido_on) log_printf("latido: ENCENDIDO (ENV latido=1)");
    /* #440 — EL TESTIGO DEL MPU, detrás del ENV `mpu=1` (por defecto NO).
     *
     * Marca el código de cada módulo como sólo lectura para que la escritura que
     * lo pisa levante un DACCVIOL con su PC — el manejador de #447 lo cuenta.
     * Se arma DESPUÉS de enlazar porque el enlace escribe en el código (el fixup
     * de eh_class). `bpvm_link_all` escribe valores resueltos absolutos, así que
     * es idempotente y el que hace `bpvm_run` a continuación no molesta.
     *
     * Andamio de diagnóstico, no producción: una región mal calculada tumbaría
     * una placa buena, y eso es peor que el bug que persigue. */
    if (board_env_bool("mpu", 0)) {
        extern void bpvm_pico_mpu_armar(const uint8_t*, const bpvm_module_t*, int);
        if (bpvm_link_all(vm) == BPVM_OK) {
            bpvm_pico_mpu_armar(vm->memory, vm->modules, vm->module_count);
        } else {
            log_printf("mpu: no se arma — el enlace fallo antes");
        }
    }

    log_printf("AOT: scan done, about to bpvm_run");
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
    /* #440 — desarmar SIEMPRE: la siguiente carga tiene que poder escribir el
     * código de los módulos, y dejar el MPU puesto convertiría el Run siguiente
     * en un fallo espurio. */
    { extern void bpvm_pico_mpu_desarmar(void); bpvm_pico_mpu_desarmar(); }

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
    if (strcmp(type, "HELLO") == 0) { bpvm_repl_dispatch(type, rid, &obj); return 1; }
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


/* ── V6/U3 g9 — la cintura del REPL de esta familia ────────────────────────
 * Se registra A MEDIAS a propósito: `info` y `put_buf` llegan en sus pasos. El
 * común no se fía del orden de la cadena para eso — cada verbo comprueba la
 * pieza que necesita y contesta UNSUPPORTED si falta. */
static int  pico_repl_fs_format(void) { fs_format_ram(); return 0; }
static int  pico_repl_fs_save(void)   { return fs_save_to_flash() == FS_OK ? 0 : -1; }
static unsigned long pico_repl_fs_total(void) { return (unsigned long) fs_total_bytes(); }
static unsigned long pico_repl_fs_used(void)  { return (unsigned long) fs_used_bytes(); }
static int  pico_repl_fs_count(void)  { return (int) fs_file_count(); }

static const bpvm_repl_ops_t s_repl_ops = {
    .info           = pico_repl_info,
    .info_extra     = pico_repl_info_extra,
    .server_name    = "bpvm-pico",
    .server_build   = BPVM_PICO_BUILD_DATE,
    /* BOOTSEL va como capacidad aparte: es de esta familia y el IDE decide con
     * ella si enseñar el botón. */
    .capabilities   = "[\"META\",\"FILES\",\"TERMINAL\",\"DEBUG\",\"BOOTSEL\"]",
    .put_buf        = s_put_buf,
    .put_buf_size   = sizeof s_put_buf,
    /* Sin after_put a propósito: littlefs committea en cada close, así que aquí
     * NO hay nada que persistir tras una subida (a diferencia del STM32). */
    .after_put      = NULL,
    .fs_total_bytes = pico_repl_fs_total,
    .fs_used_bytes  = pico_repl_fs_used,
    .fs_file_count  = pico_repl_fs_count,
    .fs_format      = pico_repl_fs_format,
    .fs_save        = pico_repl_fs_save,
};

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

    /* 3. id (puede ser 0 si el peer no lo mandó). */
    long id = json_get_long(&obj, "id", 0);

    /* 4. type. Se saca ANTES de tocar el bulk, y a propósito (V6/U3 g11): hay
     *    que saber QUÉ verbo es para decidir quién lee esos bytes. Leer el tipo
     *    del JSON ya parseado no toca el cable, así que adelantarlo es gratis. */
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        wire_v1_send_error(id, "PROTOCOL_ERROR", "falta 'type'");
        return;
    }

    /* 5. El bulk — pero SÓLO para quien no se lo lee él mismo.
     *
     * Los verbos PUT viven en el común desde V6/U3 y ahí el bulk se lee DENTRO
     * del handler. Si además se pre-leyera aquí, se leería DOS VECES: la segunda
     * lectura se comería el mensaje siguiente y el wire quedaría desincronizado.
     * No es hipotético — es exactamente el bug que tuvo el ESP32 en 0456da8, y
     * el síntoma no es "falla el PUT" sino que a partir de ahí no funciona nada.
     *
     * Lo que SÍ sigue pre-leyéndose es PACK_BURN_DATA y compañía: el gestor de
     * placa recibe el bulk ya en s_put_buf, y su camino no ha migrado. */
    int lo_lee_el_comun = (strcmp(type, "PUT") == 0 || strncmp(type, "PUT_", 4) == 0);
    long bulk = lo_lee_el_comun ? 0 : json_get_long(&obj, "bulk", 0);
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
            wire_v1_send_error(id, "NO_SPACE",
                                "bulk supera el buffer del servidor");
            return;
        }
        if (wire_v1_recv_bulk(s_put_buf, (size_t) bulk, sizeof(s_put_buf)) < 0) {
            wire_v1_send_fatal("PROTOCOL_ERROR", "lectura de bulk truncada");
            return;
        }
        bulk_size = (size_t) bulk;
    }

    /* 6. Despachar. */
    /* META */
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
            /* El bulk que esta puerta rechaza hay que TRAGARSELO igual, o se
             * queda en el cable. Sólo el de los verbos PUT: el resto ya lo
             * pre-leyó el paso 5. */
            if (lo_lee_el_comun) {
                long b_pend = json_get_long(&obj, "bulk", 0);
                if (b_pend > 0) (void) bpvm_repl_drain_bulk((unsigned long) b_pend);
            }
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
    if (strcmp(type, "SD_INFO")  == 0) { handle_sd_info(id, &obj);  return; }  /* V5/H1 */
    if (strcmp(type, "SD_MOUNT") == 0) { handle_sd_mount(id, &obj); return; }  /* V5/H2 */
    if (strcmp(type, "LIST_DIR") == 0) { handle_list_dir(id, &obj); return; }  /* V5/H2 */
    /* V6/U3 — el común, DESPUÉS de los propios y no antes (al revés que el
     * STM32) porque esta familia se migra por grupos: lo que esta placa siga
     * implementando gana, y lo que ya no, cae aquí. Al terminar la migración
     * este if sube al principio y la cadena de arriba desaparece.
     *
     * ⚠️ El grupo PUT NO puede migrar mientras el despachador de arriba lea el
     * bulk por adelantado (el común lo lee él): sería leerlo dos veces. Va en su
     * propio paso, junto con quitar esa pre-lectura. */
    if (bpvm_repl_dispatch(type, id, &obj)) return;

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
    bpvm_repl_set_ops(&s_repl_ops);   /* V6/U3 g9: la cintura, antes del primer mensaje */
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
            sd_vigilar_tick();                /* V5/H2: ¿ha entrado o salido la SD? */
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (first != '{') continue;           /* ruido entre requests: descartar */
        repl_v1_handle_request(first);
    }
}
