/*
 * bpvm_repl.c — los verbos COMUNES del REPL wire v1. Ver bpvm_repl.h para el
 * porqué y el reparto común/familia.
 *
 * Regla de la casa: cada verbo de aquí hace LO MISMO que hacía en la familia
 * de referencia (la Pico, el superconjunto), con las divergencias unificadas
 * a propósito y anotadas en el header. Nada de mejoras de paso: primero
 * idéntico y en placa, después ya se verá.
 */
#include "bpvm_machine.h"   /* V7/H4: la tabla de capacidades */
#include "bpvm_repl.h"
#include "bpvm_wire_v1.h"
#include "bpvm_log.h"
#include "bpvm_fs.h"
#include "bpvm_entry.h"     /* #466: bpvm_entry_resolve — STAT por NOMBRE de módulo */
#if defined(BPVM_LVGL) || defined(BPVM_GUI_PACK)
#include "bpvm_gui.h"       /* V6/T1: screenW/H en INFO (tamano nativo del panel); V7/H6: tambien por el pack */
#endif
#include "bpvm_listdir.h"   /* V6/U3.22: el núcleo de LIST_DIR ya era común */
#include "bpvm_sd_placa.h"  /* V7/H3: SD_INFO y SD_MOUNT, con la SD que registró la familia */
#include "bpvm_rtc.h"
#include "mdn_loader.h"     /* V7/H2 (#441): aotReceta/aotFlagsCrc en INFO */
#include "bpvm_pack.h"      /* V7/H2 (#493): BPVM_PACK_URI */
#include "bpvm_platform.h"  /* V6/U3 g9: now_ms para el durationMs del SAVE */
#include "bpvm_sqlmem.h"    /* V7/H8 #602: el motivo y el mínimo del bloque de RAM de los motores */
#include "bpvm_examen.h"    /* V7/H8 E1: el LIST examina los módulos y los packs */
#include "crc32.h"          /* V7/H8 #495 (paso 4): el CRC de lo mandado, en el GET_END */
#include "bpvm_io.h"        /* V7/H8 #495 (paso 4): la pila que le sobró a `io` (ioStackFree) */
#include "bpvm_boot.h"      /* V7/H10 #473 (punto 10): la puerta del arranque, en el común */
#include "bpvm_internal.h"  /* V7/H10, el RUN común: el latido mira los hilos */
#include "bpvm_dbg_wire.h"  /* V7/H10, el RUN común: el depurador, común */
#include "bpvm_mdn_scan.h"  /* V7/H10, el RUN común: el escáner de .mdn */
#include "aot_registry.h"   /* V7/H10, el RUN común: vaciar y contar el registro AOT */
#include "bpvm_bmgr_wire.h" /* V7/H10, el board_mgr común: la familia, la vista del ENV y el despachador */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ── LOG_DUMP ────────────────────────────────────────────────────────────────
 * El texto del log no cabe en una línea del wire, así que la respuesta sale
 * POR PARTES sobre el cable del contrato: la cabeza y los chunks escapados por
 * `send_bulk` (bytes crudos, sin salto) y el cierre por `send_line`, que pone
 * el salto del framing. Es la forma que ya tenía el S3; la Pico escribía a
 * stdout directo (su cable lo permitía) y el STM32 PERDÍA chunks si el escape
 * no le cabía en un buffer estático. */
static void log_chunk_sink(const char* data, size_t len, void* user) {
    (void) user;
    /* El núcleo entrega chunks de 256 B. El peor caso por byte es \uXXXX (6),
     * pero NO se dimensiona a ×6 (1,5 KB de pila en un micro pequeño): el buffer
     * se queda en 512 y se vacía en cuanto no caben los 6 del caso peor. */
    char esc[256 * 2];
    size_t o = 0;
    for (size_t i = 0; i < len; i++) {
        char ch = data[i];
        const char* rep = NULL;
        switch (ch) {
            case '\"':  rep = "\\\""; break;
            case '\\': rep = "\\\\"; break;
            case '\n':  rep = "\\n";  break;
            case '\r':  rep = "\\r";  break;
            case '\t': rep = "\\t";  break;
            default: break;
        }
        if (rep) {
            if (o + 2 > sizeof esc) { wire_v1_send_bulk((const uint8_t*) esc, o); o = 0; }
            esc[o++] = rep[0];
            esc[o++] = rep[1];
        } else if ((unsigned char) ch < 0x20u) {
            /* Control raro → \uXXXX, que es lo que hacía la Pico. El STM32 lo
             * sustituía por un espacio y con eso el byte se PIERDE; en un
             * volcado de log, que es justo donde se mira cuando algo va mal,
             * conviene poder distinguir un 0x01 de un 0x02. */
            if (o + 6 > sizeof esc) { wire_v1_send_bulk((const uint8_t*) esc, o); o = 0; }
            o += (size_t) snprintf(esc + o, sizeof esc - o, "\\u%04x", (unsigned) ch);
        } else {
            if (o + 1 > sizeof esc) { wire_v1_send_bulk((const uint8_t*) esc, o); o = 0; }
            esc[o++] = ch;
        }
    }
    if (o > 0) wire_v1_send_bulk((const uint8_t*) esc, o);
}

static void repl_log_dump(long id) {
    char head[64];
    int hn = snprintf(head, sizeof head,
                      "{\"type\":\"LOG_DUMP_REPLY\",\"id\":%ld,\"text\":\"", id);
    if (hn <= 0) return;
    wire_v1_send_bulk((const uint8_t*) head, (size_t) hn);
    log_dump(log_chunk_sink, NULL);
    wire_v1_send_line("\"}", 2);   /* cierra el JSON y pone el salto del framing */
}


/* ══════════════ GRUPO 2 — el FS por la FACHADA común ══════════════
 *
 * Solo verbos que se resuelven ENTEROS con `bpvm_fs_*`: DEL, MKDIR, RMDIR,
 * STAT, RENAME, GET. Los que piden primitivas de familia (DF con sus
 * contadores, FORMAT, SAVE) esperan al grupo de `ops`.
 *
 * Divergencias unificadas aquí (26-ago, medidas en U3.0b y al migrar):
 *   · códigos de error: el STM32 decía INVALID_PATH/"missing path" donde la
 *     Pico dice INVALID_PARAM/"falta path". Gana la referencia.
 *   · STAT: misma forma en las dos (size + isDir + mtime), y el `crc` BAJO
 *     DEMANDA (#398) que el STM32 no tenía.
 *   · RENAME y RMDIR no existían en el STM32: entran por el común — RENAME
 *     con primitiva real (fachada → lfs_rename); RMDIR hereda el stub v1
 *     de la Pico (los directorios son prefijos), a sabiendas.
 *   · el DEL del STM32 llamaba a fs_save() tras borrar — no-op documentado
 *     en littlefs (persiste en cada close): se cae sin cambio de conducta. */

/* La cintura de la familia (bpvm_repl_set_ops). Se declara aqui arriba
 * porque fs_fallo, que es de los primeros, ya necesita fs_total/used_bytes. */
static const bpvm_repl_ops_t* s_ops = NULL;

/* ── El fallo del FS, contado de verdad ────────────────────────────────────
 * Antes de V6/U3 esto estaba DOS veces (map_lfs_err en la Pico, log_fallo_fs en
 * el STM32) con la misma línea de log carácter por carácter, y encima el STM32
 * tiraba la información: respondiera lo que respondiera littlefs, contestaba
 * siempre "NO_SPACE / FS lleno". Los códigos son los que el IDE ya recibe de la
 * Pico, que es la que los mapeaba bien. */
static void fs_fallo(const char* op, const char* path, unsigned long size,
                     const char** code, const char** msg) {
    unsigned long tot = s_ops && s_ops->fs_total_bytes ? s_ops->fs_total_bytes() : 0;
    unsigned long usa = s_ops && s_ops->fs_used_bytes  ? s_ops->fs_used_bytes()  : 0;
    bpvm_fs_log_fail(op, path, size, (tot > usa) ? (tot - usa) : 0ul, tot);
    switch (bpvm_fs_last_fail()) {
        case BPVM_FS_FAIL_NO_SPACE:      *code = "NO_SPACE";       *msg = "FS lleno"; break;
        case BPVM_FS_FAIL_TOO_BIG:       *code = "NO_SPACE";       *msg = "fichero demasiado grande"; break;
        case BPVM_FS_FAIL_NAME_TOO_LONG: *code = "INVALID_PATH";   *msg = "nombre demasiado largo"; break;
        case BPVM_FS_FAIL_EXISTS:        *code = "EXISTS";         *msg = "ya existe"; break;
        case BPVM_FS_FAIL_NOT_FOUND:     *code = "NOT_FOUND";      *msg = "fichero no existe"; break;
        case BPVM_FS_FAIL_IO:            *code = "INTERNAL_ERROR"; *msg = "flash op falló"; break;
        default:                         *code = "INVALID_PARAM";  *msg = "argumento inválido"; break;
    }
}

static void repl_del(long id, json_obj_t* obj) {
    const char* path = json_str_inplace(obj, "path");   /* #456 - sin copia, sin tope */
    if (path == NULL) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    /* Dos fallos distintos que NO son el mismo: que no esté, y que esté y no se
     * pueda borrar. El común los juntaba en "no existe", que en el segundo caso
     * es simplemente falso. La Pico ya los distinguía. */
    if (!bpvm_fs_exists(path)) {
        wire_v1_send_error(id, "NOT_FOUND", "no existe");
        return;
    }
    if (bpvm_fs_remove(path) != 0) {
        const char *code, *msg;
        fs_fallo("del", path, 0, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }
    wire_v1_send_reply_empty("DEL_REPLY", id);
}

static void repl_stat(long id, json_obj_t* obj) {
    char buf[256];
    int por_nombre = 0;
    uint32_t size = 0;
    /* #456 - dos papeles distintos que antes compartian el MISMO buffer de 96,
     * y por eso el tope de entrada lo ponia el de salida: el path que MANDA el
     * peer no se copia (sin tope), y `resuelto` es el que CONSTRUYE el
     * resolvedor, que si necesita sitio donde caber. */
    char resuelto[BPVM_FS_PATH_MAX];
    const char* path = json_str_inplace(obj, "path");
    if (path == NULL) {
        /* #466 — STAT por NOMBRE de módulo: «¿tienes Math.mod, donde sea?». Es
         * la pregunta que el IDE hace por cada dependencia antes de subirla, y
         * la contesta EL MISMO resolvedor que usa el RUN (proyecto → literal →
         * /app → /lib → /sys): así el IDE no lleva un gemelo del orden de
         * búsqueda, que es como se desincronizó #463. `base` (opcional) es la
         * carpeta del proyecto que se va a ejecutar; vale sólo para esta
         * pregunta y se restaura. */
        const char* name = json_str_inplace(obj, "name");
        if (name == NULL) {
            wire_v1_send_error(id, "INVALID_PARAM", "falta path o name");
            return;
        }
        const char* base = json_str_inplace(obj, "base");
        char antes[BPVM_FS_PATH_MAX];   /* el basedir que se salva y se restaura */
        if (base) {
            snprintf(antes, sizeof antes, "%s", bpvm_fs_basedir());
            bpvm_fs_set_basedir(base);
        }
        /* [V7/H2, #493] El resolvedor del RUN prueba también la zona de packs
         * (la última): un módulo que sólo esté en un pack grabado sale como
         * `pack:<P>/X.mod`, y el IDE sabe que no hace falta subirlo. */
        int r = bpvm_entry_resolve(name, resuelto, sizeof(resuelto), &size);
        if (base) bpvm_fs_set_basedir(antes);
        if (r != 0) {
            wire_v1_send_error(id, "NOT_FOUND", "no existe");
            return;
        }
        path = resuelto;
        por_nombre = 1;
    } else if (bpvm_fs_stat(path, &size) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no existe");
        return;
    }
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "STAT_REPLY", id);
    if (off < 0) goto err;
    if (por_nombre) {           /* DÓNDE lo tiene: si hay que subirlo, se sube ahí */
        off = wire_v1_field_string(buf, sizeof buf, (size_t) off, "path", path);
        if (off < 0) goto err;
    }
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "size", (long) size);
    if (off < 0) goto err;
    /* #466 — el FORMATO de un módulo es su MAGIC ("MOD8", "MOD9"): 4 bytes, sin
     * cargar nada. Sólo si lo parece; lo que no empieza por "MOD" no lo lleva.
     * V7/H9 #624 — y desde MOD8, su VERSIÓN de verdad, de la cabecera: `gen` (la
     * «version» del proyecto, +36) y `ifaceCrc` (+40). Con eso decide el IDE si
     * sube una dependencia, con la regla de Eduardo (2-oct, la de Java): formato
     * compatible y versión igual o superior a la mínima —la de compilación—; el
     * CRC sólo distingue «idéntico» de «otra compilación que vale». Hasta hoy
     * contaba como versión el MAGIC, y con todo en MOD9 sólo quedaba el CRC. */
    {
        uint8_t h[44];
        long n = bpvm_fs_read_at(path, 0, h, (uint32_t) sizeof h);
        if (n >= 4 && h[0] == 'M' && h[1] == 'O' && h[2] == 'D') {
            char magic[5];
            memcpy(magic, h, 4); magic[4] = 0;
            off = wire_v1_field_string(buf, sizeof buf, (size_t) off, "magic", magic);
            if (off < 0) goto err;
            if (n >= 44 && (h[3] == '8' || h[3] == '9')) {
                uint32_t gen = ((uint32_t) h[36] << 24) | ((uint32_t) h[37] << 16) | ((uint32_t) h[38] << 8) | h[39];
                uint32_t ifc = ((uint32_t) h[40] << 24) | ((uint32_t) h[41] << 16) | ((uint32_t) h[42] << 8) | h[43];
                off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "gen", (long) gen);
                if (off < 0) goto err;
                off = wire_v1_field_ulong(buf, sizeof buf, (size_t) off, "ifaceCrc", (unsigned long) ifc);
                if (off < 0) goto err;
            }
        }
    }
    /* #398 — el CRC solo si se pide: es lo que antes calculaba el LIST para
     * TODOS los ficheros en cada refresco del árbol. */
    if (json_get_bool(obj, "crc", 0)) {
        uint32_t c = 0;
        /* #466 — SIN SIGNO: `long` es de 32 bits en las placas (y en MinGW), así
         * que un CRC con el bit 31 puesto salía NEGATIVO, y el IDE —que compara
         * sin signo— lo daba por distinto y volvía a subir la mitad de los
         * ficheros «idénticos». Lo cazó el smoke Java del STAT por nombre. */
        if (bpvm_fs_crc32(path, &c) == 0)
            off = wire_v1_field_ulong(buf, sizeof buf, (size_t) off, "crc", (unsigned long) c);
        else
            off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "crc", -1L);
        if (off < 0) goto err;
    }
    off = wire_v1_field_bool(buf, sizeof buf, (size_t) off, "isDir", 0);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "mtime", 0);
    if (off < 0) goto err;
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "STAT_REPLY no cabe");
}

static void repl_rename(long id, json_obj_t* obj) {
    const char* from = json_str_inplace(obj, "from");   /* #456 - sin copia, sin tope */
    const char* to   = json_str_inplace(obj, "to");
    if (from == NULL || to == NULL) {
        wire_v1_send_error(id, "INVALID_PARAM", "faltan from/to");
        return;
    }
    /* littlefs renombra él solo (mover una entrada de directorio): sin copia,
     * sin límite de tamaño, y ATÓMICO. #305. */
    if (bpvm_fs_rename(from, to) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no se pudo renombrar");
        return;
    }
    wire_v1_send_reply_empty("RENAME_REPLY", id);
}

/* #453 — cada trozo, tal cual al wire. Devuelve 0 siempre: aqui no hay forma
 * de saber si el otro lado sigue ahi, y cortar por nuestra cuenta dejaria el
 * `bulk` anunciado a medias sin motivo. */
static int repl_get_chunk(const uint8_t* data, uint32_t len, void* user) {
    (void) user;
    wire_v1_send_bulk(data, (size_t) len);
    return 0;
}

static void repl_get(long id, json_obj_t* obj) {
    const char* path = json_str_inplace(obj, "path");   /* #456 - sin copia, sin tope */
    char buf[96];
    if (path == NULL) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    /* El GET no carga el fichero: su TAMAÑO para la cabecera y después los
     * bytes POR TROZOS de 256 B — el trozo interno de littlefs, ni un número
     * nuevo ni lecturas partidas a la fuerza (#305/H11). */
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no existe");
        return;
    }
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "GET_REPLY", id);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "GET_REPLY no cabe"); return; }
    off = wire_v1_field_bulk(buf, sizeof buf, (size_t) off, (size_t) size);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "GET_REPLY no cabe"); return; }
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) { wire_v1_send_error(id, "INTERNAL_ERROR", "GET_REPLY no cabe"); return; }
    wire_v1_send_line(buf, (size_t) off);
    /* #453 — UNA apertura, no una por trozo.
     *
     * Esto era un bucle de `bpvm_fs_read_at` de 256 en 256 B, y `read_at` recibe
     * el PATH: cada trozo abria el fichero, hacia seek desde el principio, leia
     * y cerraba. Medido el 30-ago en un S3 con ficheros del propio proyecto:
     * uno de 9 KB (36 trozos) se abre y uno de 120 KB (472 trozos) NO -- se pasa
     * de los 10 s de timeout del IDE. Mismo codigo y misma placa: solo el tamano.
     *
     * Es la MISMA enfermedad que #398/#424 arreglo para el CRC, en el otro
     * llamador que se quedo sin arreglar. `bpvm_fs_read_stream` cae al bucle de
     * siempre si el backend no sabe leer en secuencia, asi que esto no rompe a
     * nadie: donde no haya `read_stream`, se comporta exactamente como antes.
     *
     * Si el FS falla a media transferencia ya no hay rectificacion posible -- la
     * cabecera con `bulk` ya salio--: se corta y el cliente lo ve por el bulk
     * incompleto, como con un cable desconectado. */
    (void) bpvm_fs_read_stream(path, repl_get_chunk, NULL);
}


/* ══════════════ GRUPO 3 — LIST, el del episodio del /lib ══════════════
 *
 * BFS sobre la fachada (`bpvm_fs_list`), que ademas INYECTA los montajes hijos
 * al listar su padre (`emite_montajes_hijos`): /sd y compania entran solos.
 *
 * Tres cosas que este verbo UNIFICA, medidas al migrar (26-ago):
 *
 *   · CRC POR ENTRADA, FUERA (#398). El STM32 aun leia CADA fichero entero en
 *     CADA refresco del arbol para su CRC — la medida de la P4 (15-ago) fue el
 *     99 % del tiempo del refresco. El comun emite `crc:-1` («este firmware no
 *     da CRC en el listado», que el IDE ya entiende) y el CRC se pide fichero a
 *     fichero con STAT{crc:true}, que ya esta en el grupo 2.
 *
 *   · SIN SNAPSHOT. Las familias copiaban cada directorio a un snapshot antes
 *     de emitir, porque calcular el CRC en mitad del recorrido reentraba en el
 *     FS. Sin CRC no hay reentrada: se emite en STREAMING segun se recorre —
 *     ni scratch, ni tope de entradas por directorio, ni SNAP_MAX del FS
 *     entero. El unico tope que queda es el de DIRECTORIOS pendientes, y al
 *     pasarse se DICE (`omitted` + log, #425): un listado corto que no se
 *     declara corto es una mentira de las que se creen.
 *
 *   · `path` SE IGNORA, como en la referencia: el arbol del IDE manda "" y
 *     lista todo (el STM32 tenia un filtro por prefijo que nadie usaba; el
 *     por-directorio de verdad es el verbo LIST_DIR).
 *
 * Nombres como los guarda el FS: raiz → pelado («Hello.mod»); subdirectorio →
 * completo con barra («/lib/Core.mod»). El DEL/GET del arbol dependen de eso. */

#define REPL_LIST_MAX_DIRS  16
#define REPL_LIST_NAME_MAX  64

/* V6/U3.23 — EL DESGLOSE POR RAÍZ, subido del ESP32 (#398/#408).
 *
 * El ESP32 imprimía `ls: 25 ent en 160 ms | app:11/50ms lib:14/64ms` y el común
 * sólo `ls: N ent`. Esa diferencia es la razón por la que `LIST` se quedó sin
 * migrar tanto tiempo: hacerlo a secas habría sido **unificar hacia abajo**, que
 * es lo que `#455` demostró que se comete solo. Así que sube el desglose y lo
 * GANAN la Pico y el STM32.
 *
 * Cambia el instrumento, no el dato: el ESP32 cronometraba POR ENTRADA (dos
 * lecturas de reloj cada una); aquí se mide POR DIRECTORIO, que es la unidad que
 * el recorrido ya tiene. Sale la misma línea, más barata — y además atribuye bien
 * los subdirectorios, porque el común desciende y el recorrido plano del ESP32
 * no. */
#define REPL_LIST_RAICES_MAX  8
#define REPL_LIST_RAIZ_NAME   16

typedef struct {
    long id;
    int  trozos;          /* #495 (paso 4): 1 = el LIST troceado (las entradas, en líneas LIST_PART) */
    int  first;
    int  omitidas;
    int  emitidas;
    char (*pending)[REPL_LIST_NAME_MAX];
    int  tail;
    const char* dir;      /* el directorio en curso (para componer el nombre) */
    /* Casilleros por carpeta raíz: "app", "lib", "sd"… y "/" para los sueltos,
     * que también es un dato — dice si el coste está en la raíz o en un volumen. */
    int      nraices;
    char     raiz[REPL_LIST_RAICES_MAX][REPL_LIST_RAIZ_NAME];
    int      raiz_n[REPL_LIST_RAICES_MAX];
    long     raiz_ms[REPL_LIST_RAICES_MAX];
} repl_list_ctx_t;

/* Apunta lo que costó un directorio en el casillero de su carpeta raíz. Un
 * fichero suelto de la raíz cuenta como "/". Si no caben más raíces, el TOTAL
 * sigue siendo correcto: sólo se pierde el reparto. */
static void repl_list_apunta(repl_list_ctx_t* c, const char* dir, int n, long ms) {
    char r[REPL_LIST_RAIZ_NAME];
    const char* p = (dir[0] == '/') ? dir + 1 : dir;
    const char* sl = strchr(p, '/');
    if (*p == '\0')      snprintf(r, sizeof r, "/");
    else if (sl == NULL) snprintf(r, sizeof r, "%.*s", (int)(sizeof r - 1), p);
    else {
        size_t len = (size_t)(sl - p);
        if (len >= sizeof r) len = sizeof r - 1;
        memcpy(r, p, len); r[len] = '\0';
    }
    for (int i = 0; i < c->nraices; i++) {
        if (strcmp(c->raiz[i], r) == 0) { c->raiz_n[i] += n; c->raiz_ms[i] += ms; return; }
    }
    if (c->nraices >= REPL_LIST_RAICES_MAX) return;
    snprintf(c->raiz[c->nraices], sizeof c->raiz[0], "%s", r);
    c->raiz_n[c->nraices]  = n;
    c->raiz_ms[c->nraices] = ms;
    c->nraices++;
}

/* Escape minimo para el JSON del LIST: en un path (o un motivo) solo asoman `"` y el separador. */
static void repl_list_escapa(const char* s, char* esc, size_t cap) {
    size_t o = 0;
    for (const char* q = s; *q && o + 2 < cap; q++) {
        if (*q == '"' || *q == '\\') esc[o++] = '\\';
        esc[o++] = *q;
    }
    esc[o] = '\0';
}

/* Lo que ocupa `s` escapado como lo escapa repl_list_escapa. */
static size_t repl_list_escapado(const char* s) {
    size_t n = 0;
    for (; *s; s++) n += (*s == '"' || *s == '\\') ? 2u : 1u;
    return n;
}

/* #495 (paso 4) — LA PARTE DEL LIST TROCEADO: las entradas se juntan en una línea LIST_PART acotada, que sale ENTERA
 * (wire_v1_send_msg: una línea, de una vez) cuando la entrada siguiente no cabe o se acaba el directorio. 1 KB: la
 * entrada más larga —un nombre de 63 y un motivo de E1, escapados— son ~540 B. */
#define REPL_LIST_PARTE 1024
static char   s_lp[REPL_LIST_PARTE];
static size_t s_lp_n;          /* lo escrito en la parte abierta; 0 = no hay ninguna */

static void repl_list_parte_sale(void) {
    if (s_lp_n == 0) return;
    s_lp[s_lp_n++] = ']';
    s_lp[s_lp_n++] = '}';
    wire_v1_send_msg(s_lp, s_lp_n, NULL, 0);
    s_lp_n = 0;
}

/* La entrada, ENTERA en la parte abierta (si no cabe, la parte sale y se abre otra): una parte es JSON válido. */
static void repl_list_emite_parte(repl_list_ctx_t* c, const char* full, const char* esc, uint32_t size,
                                  const char* mal) {
    const size_t cap = sizeof s_lp - 3;   /* sitio para el "]}" del cierre y el NUL de snprintf */
    size_t falta = strlen(esc) + 96u + (mal ? repl_list_escapado(mal) + 10u : 0u);
    size_t ini, o;
    int w;
    if (s_lp_n > 0 && s_lp_n + falta > cap) repl_list_parte_sale();
    if (s_lp_n == 0) {
        w = snprintf(s_lp, cap, "{\"type\":\"LIST_PART\",\"id\":%ld,\"entries\":[", c->id);
        if (w <= 0 || (size_t) w >= cap) return;
        s_lp_n = (size_t) w;
        c->first = 1;
    }
    ini = s_lp_n;
    w = snprintf(s_lp + ini, cap - ini, "%s{\"name\":\"%s\",\"size\":%lu,\"crc\":-1,\"isDir\":false,\"mtime\":0",
                 c->first ? "" : ",", esc, (unsigned long) size);
    if (w <= 0 || ini + (size_t) w >= cap) goto no_cabe;
    o = ini + (size_t) w;
    if (mal) {
        static const char k[] = ",\"mal\":\"";
        if (o + sizeof k >= cap) goto no_cabe;
        memcpy(s_lp + o, k, sizeof k - 1);
        o += sizeof k - 1;
        repl_list_escapa(mal, s_lp + o, cap - o);
        o += strlen(s_lp + o);
        if (o + 1 >= cap) goto no_cabe;
        s_lp[o++] = '"';
    }
    if (o + 1 >= cap) goto no_cabe;
    s_lp[o++] = '}';
    s_lp_n = o;
    c->first = 0;
    c->emitidas++;
    return;
no_cabe:   /* no pasa (en una parte vacía cabe la entrada más larga); si pasara, se DICE (#425) */
    s_lp_n = ini;
    c->omitidas++;
    log_printf("fs: LISTADO INCOMPLETO - '%s' no cabe en una parte del LIST", full);
}

/* Una entrada del LIST. `mal` (V7/H8 E1): el motivo por el que esta VM no cargaría el fichero, o NULL. En el LIST
 * entero sale a TROZOS, como todo él (una línea que se escribe por partes): así el motivo no pide un búfer más en la
 * pila del callback del recorrido. En el troceado (#495, paso 4), entera en su parte. */
static void repl_list_emite(repl_list_ctx_t* c, const char* full, uint32_t size, const char* mal) {
    char esc[REPL_LIST_NAME_MAX * 2];
    repl_list_escapa(full, esc, sizeof esc);
    if (c->trozos) { repl_list_emite_parte(c, full, esc, size, mal); return; }
    {
        char ent[192];
        int w = snprintf(ent, sizeof ent,
            "%s{\"name\":\"%s\",\"size\":%lu,\"crc\":-1,\"isDir\":false,\"mtime\":0",
            c->first ? "" : ",", esc, (unsigned long) size);
        if (w <= 0 || (size_t) w >= sizeof ent) return;
        wire_v1_send_bulk((const uint8_t*) ent, (size_t) w);
        if (mal) {
            static const char k[] = ",\"mal\":\"";
            wire_v1_send_bulk((const uint8_t*) k, sizeof k - 1);
            repl_list_escapa(mal, ent, sizeof ent);
            wire_v1_send_bulk((const uint8_t*) ent, strlen(ent));
            wire_v1_send_bulk((const uint8_t*) "\"", 1);
        }
        wire_v1_send_bulk((const uint8_t*) "}", 1);
    }
    c->first = 0;
    c->emitidas++;
}

/* ── V7/H8 E1 — EL EXAMEN DE LOS MÓDULOS Y PACKS DEL LIST ─────────────────────────────────────────────────────
 *
 * Eduardo (1-oct): «En rojo mostrar módulos/packs dañados o con un formato no válido.» Cada `.mod`, `.mdn` y
 * `.pack` se examina con la regla de SU cargador (bpvm_examen.h) leyendo sólo su cabecera, y su entrada lleva el
 * motivo. La TRAMPA: el recorrido del FS emite las entradas desde su callback con el cerrojo del FS cogido, y leer
 * una cabecera ahí lo pediría otra vez —el de la Pico no es recursivo: se colgaría—. Así que el recorrido los
 * APUNTA (nombre y tamaño, en un búfer fijo) y se examinan y emiten al acabar cada directorio, con el cerrojo
 * suelto. Si el búfer se llena, lo que no cabe sale sin examinar, y el log lo dice. */
#define REPL_EXAMEN_MAX     48
#define REPL_EXAMEN_NOMBRES 1536
static char     s_ex_nombres[REPL_EXAMEN_NOMBRES];
static uint16_t s_ex_ini[REPL_EXAMEN_MAX];
static uint32_t s_ex_tam[REPL_EXAMEN_MAX];
static int      s_ex_n, s_ex_usado, s_ex_sin_sitio;

static const char* repl_list_ext(const char* nombre) {
    const char* p = strrchr(nombre, '.');
    return p ? p : "";
}

static int repl_list_examinable(const char* nombre) {
    const char* e = repl_list_ext(nombre);
    return strcmp(e, ".mod") == 0 || strcmp(e, ".mdn") == 0 || strcmp(e, ".pack") == 0;
}

static int repl_list_apunta_examen(const char* full, uint32_t size) {
    size_t l = strlen(full) + 1;
    if (s_ex_n >= REPL_EXAMEN_MAX || s_ex_usado + l > sizeof s_ex_nombres) { s_ex_sin_sitio++; return 0; }
    memcpy(s_ex_nombres + s_ex_usado, full, l);
    s_ex_ini[s_ex_n] = (uint16_t) s_ex_usado;
    s_ex_tam[s_ex_n] = size;
    s_ex_usado += (int) l;
    s_ex_n++;
    return 1;
}

static long repl_list_fs_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    return bpvm_fs_read_at((const char*) user, off, dst, n);
}

/* El examen de UN fichero, con el cerrojo del FS suelto. 0 = bien; 1 = mal, con el motivo. */
static int repl_list_examina_uno(const char* full, uint32_t size, char* motivo, size_t cap) {
    uint32_t cab32[BPVM_EXAMEN_CABECERA / 4];      /* alineado: la cabecera del .mdn se lee por campos */
    uint8_t* cab = (uint8_t*) cab32;
    memset(cab32, 0, sizeof cab32);
    const char* e = repl_list_ext(full);
    if (strcmp(e, ".pack") == 0) {
        bpvm_pack_src_t s;
        bpvm_pack_src_stream(&s, repl_list_fs_read_at, (void*) full, size);
        return bpvm_pack_examina(&s, motivo, cap);
    }
    long n = bpvm_fs_read_at(full, 0, cab, size < sizeof cab32 ? size : (uint32_t) sizeof cab32);
    if (n < 0) { snprintf(motivo, cap, "dañado: no se puede leer"); return 1; }
    if (strcmp(e, ".mod") == 0) return bpvm_mod_examina(cab, (size_t) n, size, motivo, cap);
    return bpvm_mdn_examina(cab, (size_t) n, size, motivo, cap);
}

/* Al acabar un directorio: lo apuntado, examinado y emitido. */
static void repl_list_examina(repl_list_ctx_t* c) {
    char motivo[160];
    for (int i = 0; i < s_ex_n; i++) {
        const char* full = s_ex_nombres + s_ex_ini[i];
        int mal = repl_list_examina_uno(full, s_ex_tam[i], motivo, sizeof motivo);
        repl_list_emite(c, full, s_ex_tam[i], mal ? motivo : NULL);
    }
    s_ex_n = 0;
    s_ex_usado = 0;
}

static void repl_list_cb(const char* name, int is_dir, uint32_t size, void* user) {
    repl_list_ctx_t* c = (repl_list_ctx_t*) user;
    int is_root = (c->dir[1] == '\0');
    if (is_dir) {
        if (c->tail < REPL_LIST_MAX_DIRS) {
            int need = snprintf(c->pending[c->tail], REPL_LIST_NAME_MAX, "%s%s%s",
                                c->dir, is_root ? "" : "/", name);
            /* #456 - un directorio cuyo nombre no cabe NO se encola recortado:
             * recorrerlo seria recorrer OTRA carpeta. Se declara y se sigue. */
            if (need < 0 || need >= REPL_LIST_NAME_MAX) {
                c->omitidas++;
                log_printf("fs: LISTADO INCOMPLETO - '%s' no cabe en %d y se omite",
                           name, REPL_LIST_NAME_MAX);
                return;
            }
            c->tail++;
        } else {
            c->omitidas++;   /* #425: un directorio sin recorrer TAMBIEN falta */
            log_printf("fs: LISTADO INCOMPLETO — mas de %d directorios; '%s' sin recorrer",
                       REPL_LIST_MAX_DIRS, name);
        }
        return;              /* los dirs no se emiten (legado plano) */
    }
    char full[REPL_LIST_NAME_MAX];
    int need = is_root ? snprintf(full, sizeof full, "%s", name)
                       : snprintf(full, sizeof full, "%s/%s", c->dir, name);
    /* #456 - desde que el wire acepta paths largos se puede CREAR un fichero
     * cuyo nombre no cabe aqui. Emitirlo recortado es lo peor de los tres
     * desenlaces: el IDE se lo cree y luego pide un fichero que no existe. El
     * tope de este buffer es RAM estatica x16 (#461), asi que no se sube: se
     * DECLARA, que es para lo que #425 puso el campo `omitted`. */
    if (need < 0 || (size_t) need >= sizeof full) {
        c->omitidas++;
        log_printf("fs: LISTADO INCOMPLETO - '%s' no cabe en %d y se omite",
                   name, REPL_LIST_NAME_MAX);
        return;
    }
    /* V7/H8 E1 — un módulo o un pack se apunta y se examina al acabar el directorio (repl_list_examina): aquí el
     * recorrido tiene el cerrojo del FS cogido, y leer su cabecera lo pediría otra vez. Si no cabe, sale sin examinar. */
    if (repl_list_examinable(full) && repl_list_apunta_examen(full, size)) return;
    repl_list_emite(c, full, size, NULL);
}

/* La cola de directorios del recorrido (BFS). Una: la comparten el LIST entero y el troceado, que no van a la vez. */
static char s_list_pend[REPL_LIST_MAX_DIRS][REPL_LIST_NAME_MAX];

/* El contexto, listo para recorrer desde la raíz.
 *
 * U3.23 — `memset` y NO campo a campo, que es como estaba y me mordió: al
 * añadir los casilleros del desglose (`nraices`, `raiz*`) la inicialización
 * parcial los dejó con BASURA de pila, y `repl_list_apunta` usaba `nraices`
 * como tope de bucle → recorría memoria ajena y el LIST no contestaba.
 *
 * El código del ESP32 que sustituí ya hacía `memset` y llevaba el porqué
 * escrito («con la struct ya no de un solo campo, la inicialización parcial
 * saca -Wmissing-field-initializers»). Estaba dicho y no lo traje. */
static void repl_list_empieza(repl_list_ctx_t* c, long id, int trozos) {
    memset(c, 0, sizeof *c);
    c->id = id; c->first = 1; c->trozos = trozos;
    c->pending = s_list_pend;
    s_ex_n = 0; s_ex_usado = 0; s_ex_sin_sitio = 0;    /* V7/H8 E1 */
    s_lp_n = 0;                                         /* #495 (paso 4) */
    snprintf(s_list_pend[c->tail++], REPL_LIST_NAME_MAX, "/");
}

/* El directorio `i` de la cola: listarlo (el callback emite las entradas y apunta los subdirectorios y los módulos),
 * examinar lo apuntado con el cerrojo del FS suelto y anotar lo que costó. El LIST entero los hace todos seguidos; el
 * troceado, uno por paso. */
static void repl_list_un_dir(repl_list_ctx_t* c, int i) {
    char dir[REPL_LIST_NAME_MAX];
    snprintf(dir, sizeof dir, "%.63s", c->pending[i]);   /* copia: sin aliasing */
    c->dir = dir;
    int   antes = c->emitidas;
    long  t0    = (long) bpvm_platform_now_ms();
    (void) bpvm_fs_list(dir, repl_list_cb, c);   /* un volumen caido no borra el resto */
    repl_list_examina(c);                        /* V7/H8 E1: con el cerrojo del FS ya suelto */
    repl_list_apunta(c, dir, c->emitidas - antes,
                     (long) bpvm_platform_now_ms() - t0);
    c->dir = NULL;                               /* era de esta pila */
}

/* #398/#408 — el desglose, en UNA línea (el log es un bien escaso, #423).
 * `total` es lo que tarda el device; lo que el usuario ve incluye además el
 * viaje por el wire y el pintado del árbol, y eso lo mide el IDE. Que los dos
 * números existan es lo que dirá si el tiempo se va aquí o allí. (En el troceado,
 * el total va del primer paso al último: incluye lo que esperó entre medias.) */
static void repl_list_cuenta(const repl_list_ctx_t* c, long t_ini) {
    char det[160];
    int o = 0;
    for (int i = 0; i < c->nraices && o < (int) sizeof(det) - 24; i++) {
        o += snprintf(det + o, sizeof(det) - (size_t) o, " %s:%d/%ldms",
                      c->raiz[i], c->raiz_n[i], c->raiz_ms[i]);
    }
    det[o] = '\0';
    log_printf("ls: %d ent en %ld ms |%s (%d dirs omitidos)",
               c->emitidas, (long) bpvm_platform_now_ms() - t_ini, det, c->omitidas);
    if (s_ex_sin_sitio > 0)
        log_printf("ls: %d modulos/packs SIN EXAMINAR (no cabian en el bufer del examen: %d por directorio)",
                   s_ex_sin_sitio, REPL_EXAMEN_MAX);
}

static void repl_list(long id) {
    const long  t_ini = (long) bpvm_platform_now_ms();   /* U3.23: total del listado */
    char head[64];
    int hn = snprintf(head, sizeof head,
                      "{\"type\":\"LIST_REPLY\",\"id\":%ld,\"entries\":[", id);
    if (hn <= 0) return;
    wire_v1_send_bulk((const uint8_t*) head, (size_t) hn);

    repl_list_ctx_t c;
    repl_list_empieza(&c, id, 0);
    for (int i = 0; i < c.tail; i++) repl_list_un_dir(&c, i);   /* la cola crece según se recorre */
    char cola[40];
    int cn = snprintf(cola, sizeof cola, "],\"omitted\":%d}", c.omitidas);
    if (cn > 0) wire_v1_send_line(cola, (size_t) cn);
    repl_list_cuenta(&c, t_ini);
}

/* ── #495 (paso 4): EL LIST TROCEADO ─────────────────────────────────────────────────────────────────────────────
 * Con "chunked":true el recorrido no se hace de una vez: un directorio por paso (bpvm_repl_paso), sus entradas en
 * líneas LIST_PART, y al final LIST_END con `omitted`. Entre directorio y directorio, el cable es de los demás: la
 * salida del programa, un KILL, los trozos de una descarga. Uno a la vez: comparte con el LIST entero la cola de
 * directorios y la del examen. */
static struct {
    int  activo;
    int  head_i;
    long t_ini;
    repl_list_ctx_t c;
} s_ls;

static void ls_abortar(const char* por_que) {
    if (!s_ls.activo) return;
    s_ls.activo = 0;
    s_lp_n = 0;                      /* la parte a medias no sale */
    s_ex_n = 0; s_ex_usado = 0;
    log_printf("ls: listado cortado (%s)", por_que);
    wire_v1_send_error(s_ls.c.id, "ABORTED", por_que);
}

static void repl_list_troceado(long id) {
    if (s_ls.activo) { wire_v1_send_error(id, "BUSY", "ya hay un listado en curso"); return; }
    repl_list_empieza(&s_ls.c, id, 1);
    s_ls.head_i = 0;
    s_ls.t_ini  = (long) bpvm_platform_now_ms();
    s_ls.activo = 1;                 /* el primer directorio, en el próximo paso */
}

/* Un paso del listado: un directorio (sus LIST_PART), o el LIST_END. 1 si queda más. */
static int ls_paso(void) {
    repl_list_ctx_t* c = &s_ls.c;
    char lin[64];
    int off;
    if (!s_ls.activo) return 0;
    if (s_ls.head_i < c->tail) {
        repl_list_un_dir(c, s_ls.head_i++);
        repl_list_parte_sale();      /* lo de este directorio, fuera */
        if (s_ls.head_i < c->tail) return 1;
    }
    s_ls.activo = 0;
    off = wire_v1_msg_begin(lin, sizeof lin, 0, "LIST_END", c->id);
    if (off >= 0) off = wire_v1_field_long(lin, sizeof lin, (size_t) off, "omitted", (long) c->omitidas);
    if (off >= 0) off = wire_v1_msg_end(lin, sizeof lin, (size_t) off);
    if (off < 0) wire_v1_send_error(c->id, "INTERNAL_ERROR", "LIST_END no cabe");
    else         wire_v1_send_line(lin, (size_t) off);
    repl_list_cuenta(c, s_ls.t_ini);
    return 0;
}


/* ══════════════ GRUPO 4 — INFO, DF, FORMAT, SAVE: los de la cintura ══════════
 *
 * Primer grupo que NO se resuelve solo con contratos comunes: necesita datos y
 * acciones de la placa. Por eso existe `bpvm_repl_ops_t` (ver el header, donde
 * está la medida que le dio la forma).
 *
 * Lo que se unifica aquí, más allá de borrar tres copias:
 *   · el mensaje de INFO tiene LA MISMA forma en las tres familias, con los
 *     mismos 18 campos y en el mismo orden. Antes cada una emitía los suyos en
 *     su orden y con su `snprintf` a mano.
 *   · FORMAT exige `confirm:"YES"` en todas (la Pico y el STM32 ya lo pedían).
 *   · una familia sin `fs_format` responde UNSUPPORTED **con su nombre y el
 *     motivo**, no el «type no implementado» genérico de la cola del dispatch.
 */


void bpvm_repl_set_ops(const bpvm_repl_ops_t* ops) { s_ops = ops; }

/* Sin cintura registrada no se adivina: se dice. Vale para el port a medias y
 * para el arranque degradado, que son justo los momentos en que un mensaje
 * claro ahorra una tarde. */
static int sin_ops(long id, const char* verbo) {
    if (s_ops) return 0;
    char m[96];
    snprintf(m, sizeof m, "%s: este firmware no ha registrado su cintura de REPL", verbo);
    wire_v1_send_error(id, "UNSUPPORTED", m);
    return 1;
}

/* Una familia puede registrar la cintura A MEDIAS mientras migra por grupos: la
 * Pico lo hace. Que falte una pieza tiene que decirlo el verbo que la necesita
 * —no depender de que la cadena de la familia lo atrape antes—, porque eso
 * ultimo es cierto hasta que alguien reordena la cadena. */
static int falta_pieza(long id, const char* verbo, int hay,
                       const char* nombre) {
    if (hay) return 0;
    char m[112];
    snprintf(m, sizeof m, "%s: la cintura de este firmware no trae '%s'", verbo, nombre);
    wire_v1_send_error(id, "UNSUPPORTED", m);
    return 1;
}

static void repl_info(long id) {
    /* 1024 y no menos: la Pico manda 29 campos (los 18 comunes + 11 propios por
     * info_extra) y es la familia que mas dice. Su handler ya estaba
     * dimensionado asi; recortarlo aqui seria cortarle el INFO por la mitad. */
    char buf[1024];
    bpvm_repl_info_t in;
    memset(&in, 0, sizeof in);
    s_ops->info(&in);
    if (!in.unique_id)    in.unique_id = "";
    if (!in.board_name)   in.board_name = "";
    if (!in.reset_reason) in.reset_reason = "";

    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "INFO_REPLY", id);
    if (off < 0) goto err;
#define CAMPO_S(k, v)  do { off = wire_v1_field_string(buf, sizeof buf, (size_t) off, (k), (v));                             if (off < 0) goto err; } while (0)
#define CAMPO_L(k, v)  do { off = wire_v1_field_long(buf, sizeof buf, (size_t) off, (k), (long) (v));                             if (off < 0) goto err; } while (0)
#define CAMPO_UL(k, v) do { off = wire_v1_field_ulong(buf, sizeof buf, (size_t) off, (k), (unsigned long) (v));                   if (off < 0) goto err; } while (0)
    CAMPO_S("uniqueId",     in.unique_id);
    CAMPO_S("boardName",    in.board_name);
    CAMPO_L("arch",         in.arch);
    /* [V7/H2, #441] LA RECETA del nativo que esta imagen exige — nombre y CRC
     * del literal (bpvm_aot_recetas.h). No la rellena la familia: es
     * compile-time y la sabe el comun por la ISA con que se compilo, asi que
     * ninguna imagen puede olvidarla. "" / 0 donde no hay receta (PC, Xtensa).
     * Sin signo: un CRC con el bit alto en un `long` de 32 saldria negativo. */
    CAMPO_S ("aotReceta",   bpvm_mdn_host_receta_nombre());
    CAMPO_UL("aotFlagsCrc", bpvm_mdn_host_receta_crc());
    /* [V7/H5, 27-sep] Y LA ABI DE HELPERS (`MDN_ABI_VERSION`; 0 sin receta). Con la
     * receta, el IDE sabía compilar para la placa, pero no si la placa CARGARÍA el
     * resultado: un `.mdn` de otra ABI se rechaza en el log y el programa corre
     * interpretado sin ruido (`#504`, tres semanas). Con ella, el IDE lo dice. */
    CAMPO_L ("mdnAbi",      bpvm_mdn_host_abi());
    /* V7/H8 (28-sep) — el ABI de coma flotante y la zona de packs, en el COMÚN: eran de la Pico (su info_extra) y
     * la ESP32 y la STM32 no los daban (el IDE ya los pinta si llegan). `floatAbi`, junto a `arch`, es el sello:
     * `arch` sola no distingue hard de softfp. `packsXipBase` es la dirección que ve la CPU al EJECUTAR la zona —en
     * la S3 y la C3 no es la de lectura—; 0 y 0 si no hay zona montada. */
    CAMPO_S ("floatAbi",    bpvm_mdn_host_float_abi());
    {
        uint32_t zona_bytes = 0;
        const uint8_t* zona = bpvm_pack_mounted(&zona_bytes);
        CAMPO_UL("packsXipBase", zona ? (unsigned long) (uintptr_t) bpvm_pack_exec_ptr(zona) : 0ul);
        CAMPO_L ("packsBytes",   zona ? (long) zona_bytes : 0);
    }
    /* V7/H8 #602 — EL BLOQUE DE RAM DE LOS MOTORES, con los mismos nombres en las cuatro familias: dónde y cuánto
     * (0 y 0 = no hay), el motivo de la regla común (off/ok/low/nofit), la clave del ENV que lo pidió y lo que pedía
     * (en su unidad), y el mínimo de `SQLite=`, para que el remedio de un `low` lo diga la placa y no un número
     * copiado en el IDE (la decisión de V5: el AVISO vive en el INFO). Era de la Pico, como `sqlite*`. */
    CAMPO_UL("packRamBase",   in.pack_ram_base);
    CAMPO_L ("packRamBytes",  in.pack_ram_bytes);
    CAMPO_S ("packRamStatus", bpvm_sqlite_res_code((bpvm_sqlite_res_t) in.pack_ram_res));
    CAMPO_S ("packRamKey",    in.pack_ram_clave ? in.pack_ram_clave : "");
    CAMPO_L ("packRamAsked",  in.pack_ram_pedido);
    CAMPO_L ("packRamMinMb",  BPVM_SQLITE_MIN_MB);
    CAMPO_L("cpuFreqHz",    in.cpu_hz);
    CAMPO_L("uptimeMs",     in.uptime_ms);
    CAMPO_S("resetReason",  in.reset_reason);
    CAMPO_L("tempMilliC",   in.temp_milli_c);
    /* V7/H4 — los tres contadores salen de LA TABLA de capacidades, no de lo
     * que la familia ponga en `in` (que ya no se mira: era la segunda fuente).
     * Y lo nuevo: micro, paquete, la UNIDAD del PWM y las instancias válidas de
     * cada bus como lista "0,1" — el IDE lee por nombre, un campo más no le
     * estorba; el mapa de pines NO viaja (es para programas, no para el IDE). */
    {
        const bpvm_caps_t* c = bpvm_caps();
        char lst[3][40];
        for (int k = 0; k < 3; k++) {
            const uint8_t* l = (k == 0) ? c->uart : (k == 1) ? c->i2c : c->spi;
            int n = (k == 0) ? c->n_uart : (k == 1) ? c->n_i2c : c->n_spi;
            size_t o = 0; lst[k][0] = 0;
            for (int i = 0; i < n && o < sizeof lst[k] - 4; i++)
                o += (size_t) snprintf(lst[k] + o, sizeof lst[k] - o, "%s%d", i ? "," : "", (int) l[i]);
        }
        CAMPO_S("micro",        c->micro ? c->micro : "");
        CAMPO_S("paquete",      c->paquete ? c->paquete : "");
        CAMPO_L("gpioCount",    bpvm_caps_gpio_count());
        CAMPO_L("pioCount",     in.pio_count);
        CAMPO_L("pwmSlices",    (long) c->n_pwm);
        CAMPO_S("pwmUnidad",    c->pwm_unidad ? c->pwm_unidad : "");
        CAMPO_L("adcChannels",  bpvm_caps_pins_con(BPVM_PIN_ANALOG));
        CAMPO_L("adcUnidades",  (long) c->n_adc);
        CAMPO_S("uartIds",      lst[0]);
        CAMPO_S("i2cIds",       lst[1]);
        CAMPO_S("spiIds",       lst[2]);
    }
    CAMPO_L("flashBytes",   in.flash_bytes);
    CAMPO_L("sramBytes",    in.sram_bytes);
    CAMPO_L("psramBytes",   in.psram_bytes);
    CAMPO_L("vmHeapBytes",  in.vm_heap_bytes);
    CAMPO_L("vmStackBytes", in.vm_stack_bytes);
    /* #495 (paso 4) — lo que le sobró de pila al hilo `io` en el último RUN (-1: sin RUN, o el PC no lo sabe). Corre el
     * GET y el LIST troceados durante un RUN, con 4 KB de pila en la Pico y la STM32: se mira, no se supone. */
    CAMPO_L("ioStackFree",  bpvm_io_pila_libre_ultima());
    /* V7/H12, etapas 0 y 1: y lo que le sobró a la tarea `run` tras el último programa (-1: no la hay, o no lo sabe).
     * Con `run`, `ioStackFree` es la tarea del REPL, que hace de `io` durante el RUN. */
    CAMPO_L("runStackFree", bpvm_repl_run_pila_libre());
    /* V7/H12, 2b: los runs permanentes EN VIGOR —las tareas `run` que la familia creó y esperan en la cola—, no los que
     * pide el ENV (`vm.runs`): una familia que aún no crea el 2º, o que no pudo, dice 1 (0: sin tarea `run`). */
    CAMPO_L("runs",         bpvm_repl_runs());
    CAMPO_L("fsTotalBytes", in.fs_total_bytes);
    CAMPO_L("fsUsedBytes",  in.fs_used_bytes);
    /* V6/T1 (11-sep) — la PANTALLA, si la placa la tiene: el runner de las
     * tandas calcula el oraculo de una prueba grafica con `--screen=WxH` (el
     * modelo mide lo que mide el panel), y para eso tiene que saber el panel
     * POR EL WIRE, no por una tabla de nombres. Solo en builds con LVGL y solo
     * si el driver dice un tamano nativo (>0): el host SDL no lo tiene (dice 0)
     * y el sim lo publica por su propio info_extra, asi que no hay duplicados. */
#ifdef BPVM_GUI_RENDER   /* V7/H6: LVGL dentro, o por el pack (si esta resuelto) */
    {
        int sw = 0, sh = 0;
        if (bpvm_gui_disp_native_size(&sw, &sh) && sw > 0 && sh > 0) {
            CAMPO_L("screenW", (unsigned long) sw);
            CAMPO_L("screenH", (unsigned long) sh);
        }
    }
#endif
#undef CAMPO_S
#undef CAMPO_L
#undef CAMPO_UL
    /* Y lo PROPIO de la familia, si tiene. */
    if (s_ops->info_extra) {
        off = s_ops->info_extra(buf, (unsigned long) sizeof buf, off);
        if (off < 0) goto err;
    }
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "INFO_REPLY no cabe");
}

static void repl_df(long id) {
    char buf[192];
    unsigned long total = s_ops->fs_total_bytes();
    unsigned long used  = s_ops->fs_used_bytes();
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "DF_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "totalBytes", (long) total);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "usedBytes", (long) used);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "freeBytes",
                             (long) (total > used ? total - used : 0));
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "fileCount",
                             (long) s_ops->fs_file_count());
    if (off < 0) goto err;
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "DF_REPLY no cabe");
}

static void repl_format(long id, json_obj_t* obj) {
    char confirm[8];
    /* El seguro, y en las tres igual: formatear no puede ser un clic de más. */
    if (json_get_str(obj, "confirm", confirm, sizeof confirm) < 0
            || strcmp(confirm, "YES") != 0) {
        wire_v1_send_error(id, "MISSING_CONFIRM", "confirm:\"YES\"");
        return;
    }
    if (!s_ops->fs_format) {
        wire_v1_send_error(id, "UNSUPPORTED",
                           "FORMAT: este firmware no sabe formatear su FS");
        return;
    }
    if (s_ops->fs_format() != 0) {
        /* #473 (466; 3-oct): las tres familias contestaban 0 fijo, y un FORMAT que fallaba decía OK (el sim sí lo
         * propagaba). Ahora llega aquí; el porqué, al log, como cualquier fallo del FS. */
        const char *code, *msg;
        fs_fallo("format", "/", 0, &code, &msg);
        wire_v1_send_error(id, "INTERNAL_ERROR", "FORMAT: el FS no se pudo formatear");
        return;
    }
    if (s_ops->fs_save) (void) s_ops->fs_save();   /* persistir el FS vacio */
    wire_v1_send_reply_empty("FORMAT_REPLY", id);
}

static void repl_save(long id) {
    /* Sin `fs_save` la respuesta es OK, y es la VERDAD: en littlefs cada close
     * ya persiste. No es un no-op disfrazado — es que no hay nada que hacer. */
    int64_t t0 = bpvm_platform_now_ms();
    if (s_ops->fs_save && s_ops->fs_save() != 0) {
        /* V6/U3 — DICE POR QUÉ, no sólo que no pudo. Esto era un
         * `INTERNAL_ERROR "el FS no se pudo persistir"` fijo, y al comparar con
         * el ESP32 (que sí mapeaba el estado del FS: `NO_SPACE`, `INVALID_PATH`…)
         * quedó claro que migrarlo tal cual habría unificado HACIA ABAJO. Como
         * las tres familias corren littlefs por debajo, `fs_fallo` sirve a las
         * tres: mismo mapeo, y además deja libre/total en el log. */
        const char *code, *msg;
        fs_fallo("save", "", 0, &code, &msg);
        wire_v1_send_error(id, code, msg);
        return;
    }
    /* `durationMs` lo daba SOLO la Pico, donde además mide un no-op y sale 0.
     * Donde de verdad significa algo es en el STM32, que sí escribe flash — y
     * ahí no existía. Se emite siempre: 0 es una respuesta honrada. */
    char buf[96];
    long dt = (long) (bpvm_platform_now_ms() - t0);
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "SAVE_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "durationMs", dt);
    if (off < 0) goto err;
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "SAVE_REPLY no cabe");
}


/* HELLO — el saludo. La FORMA es del común (protoVersion y el orden de los
 * campos); lo propio de cada familia son tres cadenas que trae la cintura.
 *
 * Las capacidades viajan como array JSON ya escrito por la familia: son una
 * lista de constantes, no datos, y montarlas campo a campo aquí sería inventar
 * un mecanismo para lo que ya es un literal. */
static void repl_hello(long id) {
    char buf[384];
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "HELLO_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "protoVersion", 1);
    if (off < 0) goto err;
    off = wire_v1_field_string(buf, sizeof buf, (size_t) off, "serverName",
                               s_ops->server_name ? s_ops->server_name : "bpvm");
    if (off < 0) goto err;
    off = wire_v1_field_string(buf, sizeof buf, (size_t) off, "serverBuild",
                               s_ops->server_build ? s_ops->server_build : "");
    if (off < 0) goto err;
    /* V7/H9 (2-oct) — el sello de la IMAGEN, si la familia lo da (ver bpvm_repl.h): ocho cifras hexadecimales. */
    if (s_ops->image_crc) {
        char hx[12];
        snprintf(hx, sizeof hx, "%08lx", (unsigned long) s_ops->image_crc());
        off = wire_v1_field_string(buf, sizeof buf, (size_t) off, "imageCrc", hx);
        if (off < 0) goto err;
    }
    /* #495 — el trozo y la ventana de la subida, si la familia los declara (un cliente viejo no los mira). */
    if (s_ops->put_trozo > 0 && s_ops->put_ventana > 0) {
        off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "putChunk", (long) s_ops->put_trozo);
        if (off < 0) goto err;
        off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "putWindow", (long) s_ops->put_ventana);
        if (off < 0) goto err;
    }
    /* El array, tal cual lo da la familia. */
    {
        const char* caps = s_ops->capabilities ? s_ops->capabilities
                                               : "[\"META\",\"FILES\",\"TERMINAL\"]";
        int n = snprintf(buf + off, sizeof buf - (size_t) off, ",\"capabilities\":%s", caps);
        if (n <= 0 || (size_t) (off + n) >= sizeof buf) goto err;
        off += n;
    }
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "HELLO_REPLY no cabe");
}



/* Traga los `n` bytes anunciados y los tira.
 *
 * CRÍTICO, y es la razón de que esta función exista: el bulk viaja DETRÁS de la
 * línea JSON, así que si no se consumen los bytes anunciados el siguiente
 * mensaje se lee a partir de la mitad de los datos y el wire queda
 * desincronizado — el síntoma no es "falló el PUT", es que a partir de ahí no
 * funciona nada. Hay que tragárselos quepan o no, ANTES de contestar el error.
 * Devuelve 0 si se consumieron todos; <0 si el cable se cortó a medias. */
int bpvm_repl_drain_bulk(unsigned long n) {
    /* Con scratch de la familia se traga a bocados grandes; sin él (la puerta
     * puede rechazar antes de que nadie registre cintura) vale uno de pila: lo
     * que importa es CONSUMIRLO, no la velocidad de tirarlo. */
    unsigned char pila[64];
    unsigned char* buf = (s_ops && s_ops->put_buf) ? s_ops->put_buf : pila;
    unsigned long  cap = (s_ops && s_ops->put_buf) ? s_ops->put_buf_size : sizeof pila;
    while (n > 0) {
        unsigned long chunk = n < cap ? n : cap;
        if (wire_v1_recv_bulk(buf, (size_t) chunk, (size_t) cap) < 0) return -1;
        n -= chunk;
    }
    return 0;
}

static int tragar_bulk(unsigned long n) { return bpvm_repl_drain_bulk(n); }

/* #455 — el PUT del wire CREA la carpeta que le falte al destino.
 *
 * `bpvm_fs_write` NO la crea, y hace bien: se comporta como `fopen`, y ése es el
 * contrato que el lenguaje expone (lo fija `test_fs_lfs`: «write sin padres →
 * -1»). Pero un PUT del wire no es un `fopen`: es «guarda este fichero ahí».
 *
 * Las TRES familias lo hacían, cada una en su `fs_put` (`fs_lfs_pico.c`,
 * `fs_lfs_stm32.c`, `fs_lfs_esp32.c` → `ensure_parent_dirs`), y ese `fs_put` es
 * justo lo que dejó de llamarse al subir PUT al común en `U3.12`. O sea que esto
 * no es una mejora de paso: es DEVOLVER lo que la familia de referencia tenía.
 *
 * Se perdió en silencio porque `/sys`, `/lib` y `/app` se crean al montar: sólo
 * muerde subiendo a una carpeta nueva, que es lo que el IDE aún no hace.
 *
 * #473 (lote D, 268; 3-oct): es `bpvm_fs_mkdir_padres`, de la fachada —la usan
 * también las tres familias para la stdlib—. Si no puede, ni se mira: que lo
 * rechace el write, con su motivo. */

/* PUT — subida de un tirón (para ficheros que caben en el scratch; los grandes
 * van por PUT_BEGIN/DATA/END). */
/* #495 — EL PUT DE UNA PIEZA QUE SE ROMPE, también se borra: la misma regla que la subida por trozos (Eduardo,
 * 1-oct: «si queda la versión anterior el usuario se va a pensar que tiene el nuevo»). Sin esto, un .mod de 5 KB —
 * casi todos suben de una pieza— que no llega dejaba en la placa el de antes. Si no estaba, no hay nada que borrar. */
static void put_unica_borrar(const char* path, unsigned long bytes, const char* por_que) {
    if (!bpvm_fs_exists(path)) return;
    int borrado = (bpvm_fs_remove(path) == 0);
    log_printf("put: %s no termino (%lu B de una pieza, %s): %s", path, bytes, por_que,
               borrado ? "borrado" : "NO SE PUDO BORRAR");
}

static void repl_put(long id, json_obj_t* obj) {
    const char* path = json_str_inplace(obj, "path");   /* #456 - sin copia, sin tope */
    if (path == NULL) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return;
    }
    long bulk = json_get_long(obj, "bulk", -1);
    if (bulk < 0) { wire_v1_send_error(id, "INVALID_PARAM", "falta bulk"); return; }

    if ((unsigned long) bulk > s_ops->put_buf_size) {
        if (tragar_bulk((unsigned long) bulk) < 0) {
            wire_v1_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
        }
        wire_v1_send_error(id, "NO_SPACE", "fichero demasiado grande");
        return;
    }
    if (wire_v1_recv_bulk(s_ops->put_buf, (size_t) bulk, (size_t) s_ops->put_buf_size) < 0) {
        put_unica_borrar(path, (unsigned long) bulk, "el bulk no llego entero");   /* #495 */
        wire_v1_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
    }
    (void) bpvm_fs_mkdir_padres(path);                  /* #455 */
    if (bpvm_fs_write(path, s_ops->put_buf, (uint32_t) bulk, 0) != 0) {
        const char *code, *msg;
        fs_fallo("put", path, (unsigned long) bulk, &code, &msg);   /* antes de borrar: mira el espacio */
        put_unica_borrar(path, (unsigned long) bulk, "no se pudo escribir");   /* #495 */
        wire_v1_send_error(id, code, msg);
        return;
    }
    if (s_ops->after_put) s_ops->after_put(path);

    char buf[96];
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "PUT_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "size", bulk);
    if (off < 0) goto err;
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "PUT_REPLY no cabe");
}


/* ── #294 streaming PUT — subida por trozos ────────────────────────────────
 * BEGIN crea/trunca, cada DATA apende su propio bulk, END verifica el tamaño y
 * persiste UNA sola vez (un after_put por chunk sería un erase+program por
 * trozo). Una sesión a la vez: el wire es un solo cable y el IDE sube de uno
 * en uno.
 *
 * V7/H8 #495 (1-oct) — DOS BÚFERES, no uno (Eduardo: «estamos mezclando dos cosas: un buffer de comunicaciones y el
 * buffer de archivos [...] yo los separaría»). Lo que llega por el cable aterriza en `put_buf` y se JUNTA en el de
 * ficheros (`fich_buf`, un bloque de borrado de la flash de la familia), que va al fichero cuando se llena; y el
 * fichero se queda ABIERTO toda la sesión (el cursor de #491), así que no se abre, añade y cierra por trozo. Así era
 * antes, y era el cuello: ~0,27 s por trozo en la C5, midiera 1 u 8 KB; 64 KB en trozos de 512 B, de 30 a 42 s en
 * las cuatro placas (con trozos de 8 KB, de 2 a 9). Y una subida a medias ya no deja el fichero a medias: littlefs no
 * da por buena una escritura hasta cerrar el fichero, y si se va la luz queda el de antes. */
#define PUT_ABIERTO_BYTES 1024u   /* el bloque del cursor: cabecera, estado del motor y ruta (FAT con 256 B: ~860) */
static struct {
    int           active;
    /* #456 - este SI se guarda: sobrevive de BEGIN a END, o sea que no puede
     * apuntar a la linea (que se sobreescribe con el siguiente mensaje). */
    char          path[BPVM_FS_PATH_MAX];
    unsigned long received;
    unsigned long expected;
    unsigned long lleno;                          /* lo juntado en el búfer de ficheros, sin escribir aún */
    uint64_t      abierto[PUT_ABIERTO_BYTES / 8]; /* el fichero abierto (#491); uint64_t: alineado a 8 */
} s_put_sess;

/* El búfer de ficheros, al fichero (un bloque entero, o lo que quede al cerrar). 0 / -1 (el porqué, en el FS). */
static int put_vaciar(void) {
    if (s_put_sess.lleno == 0) return 0;
    if (bpvm_fs_fwrite(s_put_sess.abierto, s_ops->fich_buf, (uint32_t) s_put_sess.lleno) != (long) s_put_sess.lleno)
        return -1;
    s_put_sess.lleno = 0;
    return 0;
}

/* Lo recibido, al búfer de ficheros; cada vez que se llena, al fichero. Sin búfer de ficheros (la familia no lo pone),
 * al fichero según llega —que sigue abierto: tampoco se abre ni se cierra por trozo—. */
static int put_juntar(const unsigned char* d, unsigned long n) {
    if (!s_ops->fich_buf || s_ops->fich_bytes == 0)
        return (bpvm_fs_fwrite(s_put_sess.abierto, d, (uint32_t) n) == (long) n) ? 0 : -1;
    while (n > 0) {
        unsigned long cabe = s_ops->fich_bytes - s_put_sess.lleno;
        unsigned long k = (n < cabe) ? n : cabe;
        memcpy(s_ops->fich_buf + s_put_sess.lleno, d, k);
        s_put_sess.lleno += k; d += k; n -= k;
        if (s_put_sess.lleno == s_ops->fich_bytes && put_vaciar() != 0) return -1;
    }
    return 0;
}

/* La sesión, cerrada: también la que se dejó a medias (otro PUT_BEGIN, un error). El fichero se cierra tal cual. */
/* #495 — LA SUBIDA QUE NO TERMINA SE BORRA. La sesión tiene el fichero abierto —uno de los dos huecos del FS— y a
 * medias; si no acaba en su PUT_END (otra subida, un cliente nuevo, un RUN, un error...), se cierra y se BORRA lo
 * subido. Decisión de Eduardo (1-oct): por el espacio —«en algunas placas no vamos sobrados»— y porque «si intentamos
 * subir un archivo y queda la versión anterior, el usuario se va a pensar que tiene el nuevo». Y el log lo dice: una
 * subida rota no es silenciosa. (Un corte de corriente a mitad no pasa por aquí: el fichero queda como estaba al
 * abrirlo, porque littlefs no da nada por bueno hasta cerrar.) */
static void put_borrar(const char* por_que) {
    int borrado = (bpvm_fs_remove(s_put_sess.path) == 0);
    log_printf("put: %s no termino (%lu de %lu B, %s): %s", s_put_sess.path, s_put_sess.received,
               s_put_sess.expected, por_que, borrado ? "borrado" : "NO SE PUDO BORRAR");
}

static void put_abortar(const char* por_que) {
    if (!s_put_sess.active) return;
    (void) bpvm_fs_fclose(s_put_sess.abierto);
    s_put_sess.active = 0;
    s_put_sess.lleno  = 0;
    put_borrar(por_que);
}

/* #495 — ¿Acaba este verbo con una subida a medias? Los que no pueden convivir con ella: un cliente nuevo (HELLO: el
 * que subía ya no está), un programa (RUN: no arranca con un fichero a medias ni con su hueco cogido), un RESET, otra
 * subida (una a la vez, Eduardo 1-oct) y los que escriben en el FS. Los que sólo leen (LIST, LIST_DIR, STAT, GET, DF)
 * y los demás (PING, INFO, LOG...) sí pueden ir entre medias: «si al mismo tiempo que se envía un archivo se hace un
 * list o un print, no hay problema» (Eduardo, 1-oct). */
static int acaba_la_subida(const char* type) {
    static const char* const verbos[] = {
        "HELLO", "RUN", "RESET", "PUT", "PUT_BEGIN", "DEL", "MKDIR", "RMDIR", "RENAME", "FORMAT", "SAVE"
    };
    size_t i;
    for (i = 0; i < sizeof verbos / sizeof verbos[0]; i++)
        if (strcmp(type, verbos[i]) == 0) return 1;
    return 0;
}

/* ══════════════ #495 (paso 4) — LA DESCARGA TROCEADA ══════════════
 *
 * Un GET con "chunked":true no se manda de una vez: deja aquí la descarga —el fichero abierto con el cursor de #491
 * (el cerrojo del FS, por lectura y no todo el envío), por dónde va y el CRC de lo mandado— y cada paso manda UN
 * trozo: un GET_PART con su `off` y GET_TROZO bytes de bulk (la línea y sus bytes de una vez, wire_v1_send_msg) y, al
 * acabar, el GET_END con el tamaño y el CRC-32 de lo mandado (el de STAT). Sin acuse: aquí recibe el PC, que lee
 * siempre; la ventana de la subida protege a la placa cuando es ella la que recibe.
 *
 * Una a la vez (Eduardo: «1 a 1») y no con una subida a medias. El trozo es el de la subida de las cuatro familias
 * (put_trozo, 512 B); el cliente junta por `off`, así que no hace falta anunciarlo. */
#define GET_TROZO 512u
static struct {
    int      activo;
    long     id;
    char     path[BPVM_FS_PATH_MAX];
    uint32_t size;       /* al empezar */
    uint32_t off;        /* lo mandado */
    uint32_t crc;        /* el estado del CRC-32 de lo mandado (crc32.h: le falta el xor final) */
    uint64_t abierto[PUT_ABIERTO_BYTES / 8];   /* el fichero abierto (#491), como el de la subida */
} s_get;
static uint8_t s_get_trozo[GET_TROZO];

/* La descarga, cortada: un ERROR con su id, para que el cliente no la espere (y el log lo dice). */
static void get_abortar(const char* code, const char* por_que) {
    if (!s_get.activo) return;
    (void) bpvm_fs_fclose(s_get.abierto);
    s_get.activo = 0;
    log_printf("get: %s cortado a %lu de %lu B (%s)", s_get.path, (unsigned long) s_get.off,
               (unsigned long) s_get.size, por_que);
    wire_v1_send_error(s_get.id, code, por_que);
}

static void repl_get_troceado(long id, json_obj_t* obj) {
    const char* path = json_str_inplace(obj, "path");   /* #456 - sin copia, sin tope (se copia al empezar) */
    uint32_t size = 0;
    if (path == NULL)                      { wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return; }
    if (s_get.activo)                      { wire_v1_send_error(id, "BUSY", "ya hay una descarga en curso: una cada vez"); return; }
    if (s_put_sess.active)                 { wire_v1_send_error(id, "BUSY", "hay una subida a medias: un fichero cada vez"); return; }
    if (strlen(path) >= sizeof s_get.path) { wire_v1_send_error(id, "INVALID_PATH", "ruta demasiado larga"); return; }
    if (bpvm_fs_stat(path, &size) != 0)    { wire_v1_send_error(id, "NOT_FOUND", "no existe"); return; }
    if (bpvm_fs_file_block_size(path) > sizeof s_get.abierto
            || bpvm_fs_fopen(s_get.abierto, (uint32_t) sizeof s_get.abierto, path, BPVM_FS_O_READ) != 0) {
        wire_v1_send_error(id, "INTERNAL_ERROR", "no se pudo abrir");
        return;
    }
    snprintf(s_get.path, sizeof s_get.path, "%s", path);
    s_get.id   = id;
    s_get.size = size;
    s_get.off  = 0;
    s_get.crc  = BPVM_CRC32_INIT;
    s_get.activo = 1;                /* el primer trozo, en el próximo paso */
}

/* Un paso de la descarga: el GET_PART siguiente, o el GET_END. 1 si queda más. */
static int get_paso(void) {
    char lin[112];
    int off;
    if (!s_get.activo) return 0;
    if (s_get.off < s_get.size) {
        uint32_t k = s_get.size - s_get.off;
        long r;
        if (k > GET_TROZO) k = GET_TROZO;
        r = bpvm_fs_fread(s_get.abierto, s_get_trozo, k);
        if (r < 0) { get_abortar("INTERNAL_ERROR", "no se pudo leer"); return 0; }
        if (r > 0) {
            off = wire_v1_msg_begin(lin, sizeof lin, 0, "GET_PART", s_get.id);
            if (off >= 0) off = wire_v1_field_ulong(lin, sizeof lin, (size_t) off, "off", (unsigned long) s_get.off);
            if (off >= 0) off = wire_v1_field_bulk(lin, sizeof lin, (size_t) off, (size_t) r);
            if (off >= 0) off = wire_v1_msg_end(lin, sizeof lin, (size_t) off);
            if (off < 0) { get_abortar("INTERNAL_ERROR", "GET_PART no cabe"); return 0; }
            wire_v1_send_msg(lin, (size_t) off, s_get_trozo, (size_t) r);
            s_get.crc = bpvm_crc32_update(s_get.crc, s_get_trozo, (size_t) r);
            s_get.off += (uint32_t) r;
            if (s_get.off < s_get.size) return 1;
        }
        /* r == 0 antes de llegar al tamaño: el fichero ha encogido mientras se bajaba. Se acaba con lo mandado. */
    }
    {
        /* El GET_END: el tamaño y el CRC-32 de lo MANDADO; y `changed` si el fichero cambió de tamaño mientras se
         * bajaba (el programa escribe en él) o se acabó antes: lo bajado puede no cuadrar, y el IDE lo dice. Lo que
         * cambie sin cambiar de tamaño no se ve: habría que releerlo entero. */
        uint32_t ahora = 0;
        int changed = (s_get.off != s_get.size || bpvm_fs_stat(s_get.path, &ahora) != 0 || ahora != s_get.size);
        (void) bpvm_fs_fclose(s_get.abierto);
        s_get.activo = 0;
        if (changed)
            log_printf("get: %s cambio mientras se bajaba (%lu B al empezar, %lu mandados, %lu ahora)", s_get.path,
                       (unsigned long) s_get.size, (unsigned long) s_get.off, (unsigned long) ahora);
        off = wire_v1_msg_begin(lin, sizeof lin, 0, "GET_END", s_get.id);
        if (off >= 0) off = wire_v1_field_ulong(lin, sizeof lin, (size_t) off, "size", (unsigned long) s_get.off);
        if (off >= 0) off = wire_v1_field_ulong(lin, sizeof lin, (size_t) off, "crc",
                                                 (unsigned long) bpvm_crc32_final(s_get.crc));
        if (off >= 0 && changed) off = wire_v1_field_bool(lin, sizeof lin, (size_t) off, "changed", 1);
        if (off >= 0) off = wire_v1_msg_end(lin, sizeof lin, (size_t) off);
        if (off < 0) wire_v1_send_error(s_get.id, "INTERNAL_ERROR", "GET_END no cabe");
        else         wire_v1_send_line(lin, (size_t) off);
    }
    return 0;
}

/* El GET y el LIST: troceados si el cliente lo pide; si no, enteros como siempre —y no con una transferencia de lo
 * mismo en curso: comparten con ella el fichero (uno cada vez) o la cola del recorrido—. */
static void repl_get_elige(long id, json_obj_t* obj) {
    if (json_get_bool(obj, "chunked", 0)) { repl_get_troceado(id, obj); return; }
    if (s_get.activo) { wire_v1_send_error(id, "BUSY", "hay una descarga en curso: un fichero cada vez"); return; }
    repl_get(id, obj);
}

static void repl_list_elige(long id, json_obj_t* obj) {
    if (json_get_bool(obj, "chunked", 0)) { repl_list_troceado(id); return; }
    if (s_ls.activo) { wire_v1_send_error(id, "BUSY", "hay un listado en curso"); return; }
    repl_list(id);
}

/* #495 (paso 4) — ¿Corta este verbo una descarga en curso? Los mismos que acaban una subida (un cliente nuevo, un
 * RESET, otra subida, los que escriben en el FS), menos el RUN: la descarga sigue durante el programa, en `io`. */
static int corta_la_descarga(const char* type) {
    return strcmp(type, "RUN") != 0 && acaba_la_subida(type);
}

/* Un cliente nuevo (o ido): lo que se le mandaba al de antes, cortado. */
static void corta_las_transferencias(const char* por_que) {
    get_abortar("ABORTED", por_que);
    ls_abortar(por_que);
}

void bpvm_repl_antes_de_despachar(const char* type) {
    char por_que[48];
    if (type == NULL) return;
    snprintf(por_que, sizeof por_que, "llego %s", type);
    if (s_put_sess.active && acaba_la_subida(type)) put_abortar(por_que);
    if (strcmp(type, "HELLO") == 0 || strcmp(type, "RESET") == 0) corta_las_transferencias(por_que);
    else if (s_get.activo && corta_la_descarga(type)) get_abortar("ABORTED", por_que);
}

void bpvm_repl_cliente_ido(void) {
    if (s_put_sess.active) put_abortar("el cliente se fue");
    corta_las_transferencias("el cliente se fue");
}

int bpvm_repl_paso(void) {
    int queda = get_paso();
    queda |= ls_paso();
    return queda;
}

int bpvm_repl_hay_transferencia(void) {
    return s_get.activo || s_ls.activo;
}

int bpvm_repl_atiende_en_run(const char* type, json_obj_t* obj) {
    if (type == NULL) return 0;
    if (strcmp(type, "STAT") == 0) return 1;
    if (strcmp(type, "GET") == 0 || strcmp(type, "LIST") == 0) return json_get_bool(obj, "chunked", 0);
    return 0;
}

/* ── V7/H10, EL BOARD_MGR COMÚN (3-oct; V7_IDEAS §H10 «el board_mgr común») ──────────────────────────────────────
 * Era el `handle` de cada placa —la Pico, la STM32 y la ESP32, el mismo código salvo la cintura— y el del sim. La
 * cintura la pone la familia registrada (bpvm_bmgr_familia_t); lo demás es esto.
 *
 * #338 — DOS prestamistas: del búfer prestado (el del PUT, libre durante un comando de gestión) salen el sector de
 * TRABAJO y la respuesta; las dos copias del ENV las presta la ZONA DE RASCAR, libre durante un comando del entorno
 * porque sus otros usuarios son los PACK_* y ésos no lo tocan (bpvm_bmgr_needs_env, la frontera). Los PACK_* no usan
 * sector de trabajo y llegan con el búfer ya mordido por su bulk: el resto va entero a la respuesta. */
void bpvm_bmgr_handle(long id, const json_obj_t* obj, const char* type,
                      unsigned char* scratch, unsigned long scratch_len,
                      const unsigned char* bulk, unsigned long bulk_len) {
    const bpvm_bmgr_familia_t* f = bpvm_bmgr_familia_actual();
    if (f == NULL) {
        wire_v1_send_error(id, "UNSUPPORTED", "esta placa no tiene gestor de placa");
        return;
    }
    {
        const char* motivo = f->no_vale ? f->no_vale() : NULL;   /* p. ej. el layout de flash inconsistente */
        if (motivo) {
            wire_v1_send_error(id, "INTERNAL_ERROR", motivo);
            return;
        }
    }
    const size_t sec = f->sector;
    const int con_env = bpvm_bmgr_needs_env(type);
    const unsigned long minimo = con_env ? (unsigned long) sec + 512u : 512u;
    if (scratch == NULL || scratch_len < minimo) {
        wire_v1_send_error(id, "INTERNAL_ERROR", "scratch insuficiente");
        return;
    }
    uint8_t* sc        = con_env ? scratch : NULL;
    char*    reply     = (char*)  (con_env ? scratch + sec : scratch);
    size_t   reply_cap = (size_t) (con_env ? scratch_len - sec : scratch_len);

    uint8_t* a = NULL;
    uint8_t* b = NULL;
    if (con_env) {
        /* Contiguas y en ese orden: `b` cuelga de `a`. Una petición, un dueño que soltar. */
        a = (uint8_t*) bpvm_scratch_take(2u * sec, "bmgr-env");
        if (a == NULL) {
            wire_v1_send_error(id, "INTERNAL_ERROR", "zona de scratch no disponible para el entorno");
            return;
        }
        b = a + sec;
        /* Las dos copias, frescas de la flash: el gestor edita la rancia aquí, y sólo la flash la ve si se escribe. */
        for (int i = 0; i < 2; i++) {
            uint8_t* dst = (i == 0) ? a : b;
            if (f->copia) memcpy(dst, f->copia(i), sec);
            else if (!f->leer || f->leer(i, dst) != 0) memset(dst, 0xFF, sec);
        }
    }

    bpvm_bmgr_t bm;
    memset(&bm, 0, sizeof bm);   /* campos nuevos futuros nunca con basura de pila */
    if (f->rellenar) f->rellenar(&bm);
    bm.a = a; bm.b = b; bm.scratch = sc;
    bm.sector = sec;

    bpvm_bmgr_req_t req;
    memset(&req, 0, sizeof req);
    snprintf(req.type, sizeof req.type, "%s", type);
    req.id = id;
    req.has_key   = json_get_str(obj, "key",   req.key,   sizeof req.key)   >= 0;
    req.has_value = json_get_str(obj, "value", req.value, sizeof req.value) >= 0;
    for (int i = 0; i < BPVM_PART_COUNT; i++)
        req.part_sizes[i] = json_get_long(obj, bpvm_part_name((bpvm_part_kind_t) i), -1);
    req.off      = json_get_long(obj, "offset", -1);   /* PACK_ENTRIES/DEL/READ */
    req.has_off  = req.off >= 0;
    req.size     = json_get_long(obj, "size", -1);     /* PACK_BURN_BEGIN */
    req.has_size = req.size >= 0;
    req.bulk     = bulk;                               /* PACK_BURN_DATA (ya recibido por la familia) */
    req.bulk_len = (long) bulk_len;
    {
        /* PACK_FORMAT: una CADENA "YES", no un booleano —el formateo borra la zona entera y el contrato pide
         * escribirlo, no marcar una casilla—. */
        char confirm[8];
        req.confirm_yes = json_get_str(obj, "confirm", confirm, sizeof confirm) >= 0
                          && strcmp(confirm, "YES") == 0;
    }

    int wrote = -1;
    int n = bpvm_bmgr_wire_dispatch(&bm, &req, reply, reply_cap, &wrote);
    /* La zona se suelta en TODAS las salidas —quedársela dejaría mudos los PACK_* y los comandos del entorno
     * siguientes— y DESPUÉS del volcado, porque el sector que va a la flash vive en ella. */
    if (n < 0) {
        if (con_env) bpvm_scratch_give("bmgr-env");
        wire_v1_send_error(id, "INTERNAL_ERROR", "reply de gestion no cabe");
        return;
    }
    /* #636 / D2 — la respuesta se construye ANTES de escribir; si la flash no escribe, no se manda el OK: FLASH_ERROR, y
     * sigue el valor de antes (A/B: la copia buena no se ha tocado). */
    if (wrote >= 0 && bpvm_bmgr_escribir(wrote, wrote == 0 ? a : b) != 0) {
        if (con_env) bpvm_scratch_give("bmgr-env");
        wire_v1_send_error(id, "FLASH_ERROR", BPVM_REPL_FLASH_ERROR);
        return;
    }
    if (con_env) bpvm_scratch_give("bmgr-env");
    wire_v1_send_line(reply, (size_t) n);
}

static void reply_put_field(const char* type, long id, unsigned long val,
                            const char* field) {
    char buf[96];
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, type, id);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, field, (long) val);
    if (off < 0) goto err;
    off = wire_v1_msg_end(buf, sizeof buf, (size_t) off);
    if (off < 0) goto err;
    wire_v1_send_line(buf, (size_t) off);
    return;
err:
    wire_v1_send_error(id, "INTERNAL_ERROR", "reply no cabe");
}

static void repl_put_begin(long id, json_obj_t* obj) {
    const char* path = json_str_inplace(obj, "path");   /* #456 - sin copia, sin tope */
    if (path == NULL) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return;
    }
    /* Lo unico que se copia es lo que hay que GUARDAR para el END. Y si no
     * cabe se dice AQUI, que es el sitio donde el peer puede enterarse. */
    if (strlen(path) >= sizeof s_put_sess.path) {
        wire_v1_send_error(id, "INVALID_PATH", "path demasiado largo"); return;
    }
    put_abortar("llego otra subida");                /* #495: una sesión a medias, borrada */
    (void) bpvm_fs_mkdir_padres(path);               /* #455 */
    if (bpvm_fs_file_block_size(path) > sizeof s_put_sess.abierto) {
        wire_v1_send_error(id, "INVALID_PATH", "path demasiado largo para abrir el fichero"); return;
    }
    if (bpvm_fs_fopen(s_put_sess.abierto, (uint32_t) sizeof s_put_sess.abierto, path, BPVM_FS_O_WRITE) != 0) {
        const char *code, *msg;                      /* #495: abierto toda la sesión; "w" = crea o trunca */
        fs_fallo("put", path, 0, &code, &msg);
        wire_v1_send_error(id, code, msg); return;
    }
    s_put_sess.active   = 1;
    s_put_sess.received = 0;
    s_put_sess.lleno    = 0;
    s_put_sess.expected = (unsigned long) json_get_long(obj, "size", 0);
    strncpy(s_put_sess.path, path, sizeof s_put_sess.path - 1);
    s_put_sess.path[sizeof s_put_sess.path - 1] = '\0';
    reply_put_field("PUT_BEGIN_REPLY", id, 0, "received");
}

static void repl_put_data(long id, json_obj_t* obj) {
    long bulk = json_get_long(obj, "bulk", -1);
    if (bulk < 0) { wire_v1_send_error(id, "INVALID_PARAM", "falta bulk"); return; }

    if ((unsigned long) bulk > s_ops->put_buf_size) {
        if (tragar_bulk((unsigned long) bulk) < 0) {
            wire_v1_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
        }
        put_abortar("trozo mayor que el bufer");
        wire_v1_send_error(id, "NO_SPACE", "chunk mayor que el buffer"); return;
    }
    if (wire_v1_recv_bulk(s_ops->put_buf, (size_t) bulk, (size_t) s_ops->put_buf_size) < 0) {
        wire_v1_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
    }
    /* El bulk YA está consumido. Sólo a partir de aquí se puede contestar un
     * error sin dejar el wire a medias — de ahí que la sesión se valide DESPUÉS
     * de leer y no antes, que es lo que parecería natural. */
    if (!s_put_sess.active) {
        wire_v1_send_error(id, "NO_SESSION", "PUT_DATA sin PUT_BEGIN"); return;
    }
    if (bulk > 0 && put_juntar(s_ops->put_buf, (unsigned long) bulk) != 0) {   /* #495 */
        const char *code, *msg;
        fs_fallo("append", s_put_sess.path, (unsigned long) bulk, &code, &msg);   /* antes de borrar: mira el espacio */
        put_abortar("no se pudo escribir");
        wire_v1_send_error(id, code, msg); return;
    }
    s_put_sess.received += (unsigned long) bulk;
    reply_put_field("PUT_DATA_REPLY", id, s_put_sess.received, "received");
}

static void repl_put_end(long id, json_obj_t* obj) {
    (void) obj;
    if (!s_put_sess.active) {
        wire_v1_send_error(id, "NO_SESSION", "PUT_END sin PUT_BEGIN"); return;
    }
    unsigned long recv = s_put_sess.received;
    unsigned long exp  = s_put_sess.expected;
    /* #495: lo que quede en el búfer de ficheros, al fichero, y cerrarlo: es al cerrar cuando littlefs lo da por bueno. */
    int falla = (put_vaciar() != 0);
    if (bpvm_fs_fclose(s_put_sess.abierto) != 0) falla = 1;
    s_put_sess.active = 0;
    s_put_sess.lleno  = 0;
    if (falla) {
        const char *code, *msg;
        fs_fallo("put", s_put_sess.path, recv, &code, &msg);   /* antes de borrar: mira el espacio */
        put_borrar("no se pudo escribir el final");
        wire_v1_send_error(id, code, msg); return;
    }
    if (exp != 0 && recv != exp) {
        put_borrar("el tamano no cuadra");                     /* #495: lo que no está entero, no se queda */
        wire_v1_send_error(id, "SIZE_MISMATCH", "bytes recibidos != size anunciado");
        return;
    }
    if (s_ops->after_put) s_ops->after_put(s_put_sess.path);
    reply_put_field("PUT_END_REPLY", id, recv, "size");
}

/* ── LIST_DIR ─────────────────────────────────────────────────────────────────
 *
 * VERBO DISTINTO DE `LIST`, no un LIST arreglado — y confundirlos es lo que hacía
 * esto difícil. Responden a preguntas diferentes (doc que viene del Pico, V5/H2):
 *
 *   · LIST     = «todo el FS interno, con CRC» — lo que necesita el Run.
 *   · LIST_DIR = «los hijos de ESTE directorio» — lo que necesita mirar.
 *
 * Con una SD montada la diferencia deja de ser estética: `LIST` recorre el FS
 * ENTERO y calcula el CRC de cada fichero, o sea que sobre una tarjeta de 119 GB
 * se leería la tarjeta entera byte a byte. Aquí no hay CRC ni recursión **a
 * propósito**, no por ahorrar.
 *
 * El listado se hace en dos tiempos —fotografiar y luego emitir— y eso NO es un
 * rodeo: el callback de la fachada corre DENTRO del cerrojo del FS, así que
 * escribir al transporte desde ahí retendría el cerrojo todo lo que el host tarde
 * en leer, y cualquier thread BP que tocara un fichero se quedaría esperando.
 *
 * ── V6/U3.22: el envoltorio, también al común ────────────────────────────────
 * El listado de UN directorio. El NÚCLEO (`bpvm_listdir_emitir`) ya era común
 * desde V5/H6 —salió del REPL del Pico— y lo enlazaban las cuatro familias; lo
 * único que quedaba repartido era este envoltorio: 21 líneas en la Pico y 17 en
 * el ESP32, y **casi idénticas**. Comparadas antes de borrar, la única
 * diferencia era el TRANSPORTE:
 *
 *   Pico  : sink = fwrite(stdout)          cierre = fputc('
') + fflush
 *   ESP32 : sink = wire_v1_send_bulk       cierre = wire_v1_send_line("", 0)
 *
 * Y en el Pico esas dos cosas SON `wire_v1_send_bulk` y `wire_v1_send_line`
 * (mismo `fwrite`+`fflush`), así que usar el contrato deja los mismos bytes en
 * el cable — y de paso el Pico gana el `tx_lock` que su atajo se saltaba.
 *
 * 🎁 El STM32 GANA el verbo, y a coste cero de build: `bpvm_listdir.o` ya estaba
 * en su binario (su proyecto compila `src/` por carpetas) — compilado y sin que
 * nadie lo llamara. */
static void repl_listdir_sink(const char* txt, size_t n, void* user) {
    (void) user;
    wire_v1_send_bulk((const uint8_t*) txt, n);
}

/* U3.25 — cuenta entradas de un directorio. Sirve para que RMDIR diga la
 * verdad: `bpvm_fs_rmdir` devuelve -1 tanto si el dir no esta como si esta
 * lleno, y son dos fallos distintos (misma leccion que `repl_del`). */
static void contar_cb(const char* nombre, int is_dir, uint32_t size, void* u) {
    (void) nombre; (void) is_dir; (void) size;
    (*(int*) u)++;
}

static void repl_list_dir(long id, json_obj_t* obj) {
    const char* path = json_str_inplace(obj, "path");   /* #456 - sin copia, sin tope */
    if (path == NULL) path = "/";                       /* sin path = la raíz */

    switch (bpvm_listdir_emitir(path, id, repl_listdir_sink, NULL, NULL)) {
        case BPVM_LISTDIR_OCUPADO:
            wire_v1_send_error(id, "BUSY", "zona de scratch ocupada");
            return;
        case BPVM_LISTDIR_NO_LISTA:
            wire_v1_send_error(id, "NOT_FOUND", "no se puede listar");
            return;
        case BPVM_LISTDIR_OK:
            break;
    }
    /* El cierre de línea lo pone el llamante: es lo único que el núcleo no
     * escribe, justo porque cada transporte lo hace a su manera. */
    wire_v1_send_line("", 0);
}

/* V7/H8 #601 — la lista ÚNICA de los verbos que tocan el FS (ver bpvm_repl.h). */
int bpvm_repl_verbo_de_fs(const char* type) {
    static const char* const verbos[] = {
        "LIST", "LIST_DIR", "STAT", "GET", "PUT", "DEL", "MKDIR", "RMDIR", "RENAME", "FORMAT", "SAVE", "DF"
    };
    size_t i;
    if (!type) return 0;
    if (strncmp(type, "PUT_", 4) == 0) return 1;   /* #294, el PUT en streaming: PUT_BEGIN/PUT_DATA/PUT_END */
    for (i = 0; i < sizeof verbos / sizeof verbos[0]; i++)
        if (strcmp(type, verbos[i]) == 0) return 1;
    return 0;
}

/* V7/H10 #473 (punto 10; 3-oct) — la puerta del arranque, una (ver bpvm_repl.h). Los mensajes eran tres para lo
 * mismo; queda el de la Pico, que dice en qué estado se quedó el arranque. */
int bpvm_repl_puerta_arranque(const char* type, long id, json_obj_t* obj, int estado, int bulk_en_cable) {
    char msg[112];
    if (bpvm_repl_verbo_de_fs(type) && estado < BPVM_BOOT_FS) {
        if (bulk_en_cable) {
            long b_pend = json_get_long(obj, "bulk", 0);
            if (b_pend > 0) (void) bpvm_repl_drain_bulk((unsigned long) b_pend);
        }
        snprintf(msg, sizeof msg, "FS no disponible en estado %d (%s): configurar particiones",
                 estado, bpvm_boot_state_name((bpvm_boot_state_t) estado));
        wire_v1_send_error(id, "NOT_READY", msg);
        return 1;
    }
    if (type && strcmp(type, "RUN") == 0 && estado < BPVM_BOOT_APP) {
        snprintf(msg, sizeof msg, "VM no disponible en estado %d (%s): configurar particiones",
                 estado, bpvm_boot_state_name((bpvm_boot_state_t) estado));
        wire_v1_send_error(id, "NOT_READY", msg);
        return 1;
    }
    return 0;
}

static int run_depurador_previo(const char* type, long id, json_obj_t* obj);   /* abajo, con el RUN común */

int bpvm_repl_dispatch(const char* type, long id, json_obj_t* obj) {
    /* V7/H10, el RUN común, paso 6 (3-oct): el RUN, el KILL sin programa y el depurador de antes del RUN (PAUSE,
     * SET_BP, CLR_BP), AQUÍ, para las cuatro —antes, en el despachador de cada familia y en el del sim—. Con la cintura
     * del RUN puesta. Durante un RUN no llegan aquí: el poll común lee del cable el KILL y los del depurador, y a un
     * RUN le contesta BUSY. */
    if (s_ops && s_ops->run) {
        if (strcmp(type, "RUN") == 0) {
            bpvm_repl_run(id, json_str_inplace(obj, "path"), json_str_inplace(obj, "arg"));
            return 1;
        }
        if (strcmp(type, "KILL") == 0) { wire_v1_send_error(id, "NO_SESSION", "no hay programa en ejecución"); return 1; }
        if (run_depurador_previo(type, id, obj)) return 1;
    }
    if (strcmp(type, "PING") == 0) {
        wire_v1_send_reply_empty("PONG", id);
        return 1;
    }
    if (strcmp(type, "TIME") == 0) {
        long epochSec = json_get_long(obj, "epochSec", -1);
        if (epochSec < 0) {
            /* La divergencia unificada: esto ERA silencio-OK en S3/STM32. */
            wire_v1_send_error(id, "INVALID_PARAM", "TIME: falta 'epochSec' (>=0)");
            return 1;
        }
        bpvm_rtc_set_now_ms((int64_t) epochSec * 1000LL);
        wire_v1_send_reply_empty("TIME_REPLY", id);
        return 1;
    }
    if (strcmp(type, "LOG_DUMP") == 0) {
        repl_log_dump(id);
        return 1;
    }
    if (strcmp(type, "LOG_CLEAR") == 0) {
        log_clear_ram();
        log_clear_flash();
        log_printf("LOG cleared via wire v1");   /* fecha el corte en el log nuevo */
        wire_v1_send_reply_empty("LOG_CLEAR_REPLY", id);
        return 1;
    }
    if (strcmp(type, "LIST_DIR") == 0) { repl_list_dir(id, obj); return 1; }  /* U3.22 */
    /* V7/H3 (24-sep) — la SD: el dispositivo lo registra la familia (`bpvm_sd_placa`); sin él, la
     * respuesta dice que esta placa no tiene SD, no «type no implementado». */
    if (strcmp(type, "SD_INFO")  == 0) { bpvm_sd_placa_sd_info(id);  return 1; }
    if (strcmp(type, "SD_MOUNT") == 0) { bpvm_sd_placa_sd_mount(id); return 1; }
    if (strcmp(type, "LIST") == 0) { repl_list_elige(id, obj); return 1; }   /* #495 (paso 4): entero o troceado */
    /* ── grupo 4: los de la cintura ── */
    if (strcmp(type, "HELLO")  == 0) { if (!sin_ops(id, "HELLO"))  repl_hello(id);       return 1; }
    if (strncmp(type, "PUT", 3) == 0
            && (strcmp(type, "PUT") == 0 || strcmp(type, "PUT_BEGIN") == 0
             || strcmp(type, "PUT_DATA") == 0 || strcmp(type, "PUT_END") == 0)) {
        if (sin_ops(id, type)) return 1;
        if (falta_pieza(id, type, s_ops->put_buf != NULL, "put_buf")) return 1;
        if      (strcmp(type, "PUT")       == 0) repl_put(id, obj);
        /* #294 streaming: los tres van juntos o no van — comparten sesión. */
        else if (strcmp(type, "PUT_BEGIN") == 0) repl_put_begin(id, obj);
        else if (strcmp(type, "PUT_DATA")  == 0) repl_put_data(id, obj);
        else                                     repl_put_end(id, obj);
        return 1;
    }
    if (strcmp(type, "INFO")   == 0) {
        if (sin_ops(id, "INFO")) return 1;
        if (falta_pieza(id, "INFO", s_ops->info != NULL, "info")) return 1;
        repl_info(id); return 1;
    }
    if (strcmp(type, "DF")     == 0) { if (!sin_ops(id, "DF"))     repl_df(id);          return 1; }
    if (strcmp(type, "FORMAT") == 0) { if (!sin_ops(id, "FORMAT")) repl_format(id, obj); return 1; }
    if (strcmp(type, "SAVE")   == 0) { if (!sin_ops(id, "SAVE"))   repl_save(id);        return 1; }
    /* ── grupo 2: el FS por la fachada ── */
    if (strcmp(type, "DEL")    == 0) { repl_del(id, obj);    return 1; }
    if (strcmp(type, "STAT")   == 0) { repl_stat(id, obj);   return 1; }
    if (strcmp(type, "RENAME") == 0) { repl_rename(id, obj); return 1; }
    if (strcmp(type, "GET")    == 0) { repl_get_elige(id, obj); return 1; }   /* #495 (paso 4): entero o troceado */
    if (strcmp(type, "MKDIR")  == 0) {
        /* U3.25 — CREA EL DIRECTORIO DE VERDAD.
         *
         * Antes esto contestaba OK sin tocar nada, con el argumento de que «en un
         * FS plano con '/' como namespace no hay nodos de directorio». Eso era
         * cierto del FS en RAM del STM32 y dejó de serlo hace tiempo: los TRES
         * backends (`fs_lfs.c`, `fs_host.c`) hacen `mkdir` recursivo y ok-si-existe.
         * De hecho este mismo fichero ya lo llamaba —`bpvm_fs_mkdir_padres()` en cada
         * PUT (#455)—, así que el común creaba directorios al subir un fichero y
         * decía «hecho, nada» cuando se le pedía uno. Lo destapó el arnés del
         * simulador (`U3.24`), que traía un MKDIR de verdad: unificar al stub era
         * la regresión disfrazada de limpieza de #455. */
        const char* path = json_str_inplace(obj, "path");   /* #456 - sin copia, sin tope */
        if (path == NULL) {
            wire_v1_send_error(id, "INVALID_PARAM", "falta path");
            return 1;
        }
        if (bpvm_fs_mkdir(path) != 0) {
            /* #631 (2-oct): dice POR QUÉ, como DEL y PUT —un fichero con ese nombre es EXISTS, una unidad llena
             * NO_SPACE—; contestaba siempre INTERNAL_ERROR. Desde #628 el motivo es el de ESTE fallo. */
            const char *code, *msg;
            fs_fallo("mkdir", path, 0, &code, &msg);
            wire_v1_send_error(id, code, msg);
            return 1;
        }
        wire_v1_send_reply_empty("MKDIR_REPLY", id);
        return 1;
    }
    if (strcmp(type, "RMDIR")  == 0) {
        /* U3.25 — BORRA EL DIRECTORIO DE VERDAD (ver MKDIR arriba).
         *
         * Y separa los tres desenlaces, que el stub juntaba en un OK: no existe,
         * existe y tiene cosas dentro, y borrado. `bpvm_fs_rmdir` devuelve -1 en
         * los dos primeros casos, así que se mira antes con el listado —el mismo
         * gesto que `repl_del` hace con `bpvm_fs_exists`. */
        const char* path = json_str_inplace(obj, "path");   /* #456 - sin copia, sin tope */
        if (path == NULL) {
            wire_v1_send_error(id, "INVALID_PARAM", "falta path");
            return 1;
        }
        if (!bpvm_fs_isdir(path)) {
            wire_v1_send_error(id, "NOT_FOUND", "no existe o no es un directorio");
            return 1;
        }
        int n = 0;
        (void) bpvm_fs_list(path, contar_cb, &n);
        if (n > 0) {
            wire_v1_send_error(id, "NOT_EMPTY", "el directorio no está vacío");
            return 1;
        }
        if (bpvm_fs_rmdir(path) != 0) {
            wire_v1_send_error(id, "INTERNAL_ERROR", "no se pudo borrar el directorio");
            return 1;
        }
        wire_v1_send_reply_empty("RMDIR_REPLY", id);
        return 1;
    }
    return 0;   /* no es del común: la familia sigue con su cadena */
}


/* ═════════════════════ V7/H10 — EL RUN, EN UN SOLO SITIO (el RUN común, paso 2; 3-oct) ═════════════════════
 *
 * Estaba escrito cuatro veces —la Pico, la ESP32, la STM32 y el micro simulado— y cada copia fallaba a su manera
 * (`V7_IDEAS` §H10 «RUN en un solo sitio»): el exitCode de un fallo de carga, el texto del KILL (dos anteponían una
 * excepción VIEJA), los avisos dentro de OUTPUT, el sink que tiraba o cortaba, el KILL que se podía perder entre
 * arrancar `io` y `bpvm_run`. Aquí, una vez, con las reglas de Eduardo (2-oct). La cintura, en bpvm_repl.h.
 *
 * El estado es de UN RUN a la vez (la sesión lo guarda): estáticos, y lo dice. */
static long             s_run_sesion    = 0;      /* la sesión en curso; 0 = ninguna */
static long             s_run_siguiente = 1;
static long             s_run_kill      = -1;     /* el id del KILL recibido durante el RUN */
static long             s_run_reset     = -1;     /* el del RESET */
static int              s_run_latido    = 0;
static bpvm_t*          s_run_vm        = NULL;   /* la VM del RUN: la ven el poll y el depurador */
static wire_v1_salida_t s_run_salida;

int bpvm_repl_run_activo(void) { return s_run_sesion != 0; }

static const bpvm_repl_run_ops_t* run_ops(void) { return s_ops ? s_ops->run : NULL; }

/* ═════════════════════ V7/H12, etapas 0 y 1 (4-oct) — LA TAREA `run` ═════════════════════
 *
 * El programa corre en una tarea propia y PERMANENTE —nace con el arranque y, sin programa, espera— y la tarea del
 * REPL hace de `io` durante el RUN: el lazo de bpvm_io, en su hilo (bpvm_io_lazo), sin crear otro. Decidido por Eduardo
 * (4-oct: «`run` permanente», las etapas 0 y 1 juntas; V7_IDEAS §H12 «Etapas 0 y 1 en la Pico 2»). La crea la familia
 * (bpvm_repl.h); sin ella, el RUN de siempre.
 *
 * V7/H12, etapa 2a.2 (4-oct; V7_IDEAS §H12 «Etapa 2a.2») — `run` espera en LA COLA (bpvm_smp.h): es un run permanente
 * y el RUN cuelga ahí el programa —el main a la cola, prepararlo, publicarlo— en vez de darle el aviso «trabajo».
 * Cuando acaba, avisa la cola (run_acabado, en `run`: el último run que sale, ya descolgado). Con el interruptor de la
 * imagen a 0 (BPVM_RUN_EN_COLA; el simulado, --run-etapa1), el relevo de la etapa 1.
 *
 * Entre las dos, los avisos —semáforos binarios, uno por sentido, y cada uno con UN solo que espera—:
 *   «trabajo» (io → run; sólo con el relevo de la etapa 1): la VM ya cargada (`vm`, escrito antes de darlo);
 *   «hecho»   (run → io): lo que dio el programa (`rs`, escrito antes de darlo).
 * La última línea antes del EXITED: `run`, al acabar, pone `fin` —desde ahí no escribe más— y el lazo de `io` cierra
 * la cola él mismo y sale cuando queda seca; después espera el «hecho», y sólo entonces EXITED y destruir la VM. `run`
 * no cierra ni destruye nada de `io`: lo que se destruye lo destruye quien lo cerró, en su hilo. */
#ifndef BPVM_RUN_EN_COLA
#define BPVM_RUN_EN_COLA 1
#endif
static int s_run_en_cola = BPVM_RUN_EN_COLA;

static struct {
    bpvm_platform_sem_handle_t trabajo, hecho;
    bpvm_t*      vm;            /* el programa: io, antes del «trabajo» (el relevo de la etapa 1) */
    int          rs;            /* lo que dio el programa: run, antes del «hecho» */
    volatile int fin;           /* run ha acabado: io cierra la cola (bpvm_io_lazo) */
    volatile int viva;          /* la tarea existe y espera el «trabajo» (etapa 1): sin ella, el RUN de siempre */
    int          en_cola;       /* los runs permanentes en la cola (registrados al preparar; 2b: los de `vm.runs`): el RUN cuelga ahí */
    long         pila_libre;    /* lo que le sobró de pila tras el último programa (-1: no lo sabe) */
    int          ensayo_ms;     /* SÓLO PRUEBAS: la ventana ensanchada entre el final del programa y `fin` */
    int          sabotaje;      /* SÓLO PRUEBAS: io cierra la cola sin esperar a `fin` */
} s_tarea = { NULL, NULL, NULL, 0, 0, 0, 0, -1, 0, 0 };

int  bpvm_repl_run_en_cola(void) { return s_run_en_cola; }
void bpvm_repl_run_tarea_en_cola(int si) { s_run_en_cola = si ? 1 : 0; }

/* Una por cada tarea `run` que va a crear la familia (2b: tantas como `vm.runs`), ANTES de crearla. Lo común, la
 * primera vez: el «hecho» (y el «trabajo», con el relevo de la etapa 1, que es de UN run: una segunda, -1). Y en la
 * cola, un run permanente más: desde aquí el RUN cuelga los programas en ella aunque la tarea todavía no haya empezado
 * (lo encontrará al nacer); si la familia no puede crearla, lo dice (bpvm_repl_run_tarea_no_nacio). */
int bpvm_repl_run_tarea_prepara(void) {
    if (!s_tarea.hecho) {
        if (!s_run_en_cola && bpvm_platform_sem_init(&s_tarea.trabajo) != 0) return -1;
        if (bpvm_platform_sem_init(&s_tarea.hecho) != 0) {
            if (!s_run_en_cola) bpvm_platform_sem_destroy(&s_tarea.trabajo);
            return -1;
        }
    } else if (!s_run_en_cola) {
        return -1;                                              /* el relevo de la etapa 1: un run */
    }
    if (s_run_en_cola) {
        if (bpvm_cola_registrar_permanente() != 0) return -1;   /* la cola nace aquí, en el arranque */
        s_tarea.en_cola++;
    }
    return 0;
}

void bpvm_repl_run_tarea_no_nacio(void) {
    if (s_tarea.en_cola > 0) {
        bpvm_cola_baja_permanente();
        s_tarea.en_cola--;
    }
}

/* 2a.2 — EL AVISO DE LA COLA cuando el programa acaba: lo da `run` (el último run que sale), con el programa ya
 * descolgado. Lo que hacía la etapa 1 después de bpvm_run: la pila que le sobró, lo que dio, `fin` (ya no escribe
 * más), despertar a `io` —que lo vea ya y no al vencer su espera de 5 ms; su cola vive hasta el «hecho»— y el «hecho».
 * Desde ahí `run` no toca la VM. */
static void run_acabado(bpvm_t* vm, int rs, void* user) {
    (void) user;
    if (s_tarea.ensayo_ms > 0) bpvm_platform_thread_sleep_ms(s_tarea.ensayo_ms);
    s_tarea.pila_libre = (long) bpvm_platform_pila_libre_actual();
    s_tarea.rs = rs;
    __atomic_store_n(&s_tarea.fin, 1, __ATOMIC_RELEASE);   /* 3.1, paso 6: io, quizá en el otro núcleo */
    bpvm_io_despertar(vm);
    bpvm_platform_sem_give(&s_tarea.hecho);
}

void bpvm_repl_run_tarea(void* arg) {
    int id = (int) (intptr_t) arg;   /* 2b: el número de este run (0, 1…), el que la familia le dio al crearla */
    /* NO VUELVE NUNCA: en FreeRTOS, una tarea que vuelve cuelga la placa (prvTaskExitError). Sin sus avisos —la familia
     * no llamó a `prepara`, o no pudo— se queda dormida y `viva` sigue a 0: el RUN de siempre. Tampoco escribe en el
     * log al nacer: nacería a la vez que el arranque (lo dice quien la crea). */
    if (!s_tarea.hecho)
        for (;;) bpvm_platform_thread_sleep_ms(60000);
    if (s_tarea.en_cola) {
        bpvm_cola_correr(1, id);  /* 2a.2: un run permanente, en la cola; no vuelve */
        for (;;) bpvm_platform_thread_sleep_ms(60000);
    }
    s_tarea.viva = 1;
    for (;;) {
        bpvm_platform_sem_take(&s_tarea.trabajo);
        bpvm_t* vm = s_tarea.vm;
        int rs = (int) bpvm_run(vm);
        if (s_tarea.ensayo_ms > 0) bpvm_platform_thread_sleep_ms(s_tarea.ensayo_ms);
        s_tarea.pila_libre = (long) bpvm_platform_pila_libre_actual();
        s_tarea.rs = rs;
        __atomic_store_n(&s_tarea.fin, 1, __ATOMIC_RELEASE);   /* ya no escribe más: io puede cerrar la cola */
        bpvm_io_despertar(vm);              /* y que lo vea ya, no al vencer su espera de 5 ms (la cola vive hasta el «hecho») */
        bpvm_platform_sem_give(&s_tarea.hecho);
    }
}

long bpvm_repl_run_pila_libre(void) { return s_tarea.pila_libre; }

int bpvm_repl_runs(void) { return s_tarea.en_cola; }

void bpvm_repl_run_tarea_ensayo(int espera_ms, int sabotaje) {
    s_tarea.ensayo_ms = espera_ms > 0 ? espera_ms : 0;
    s_tarea.sabotaje  = sabotaje ? 1 : 0;
}

/* La salida del programa: la llaman `io` (por líneas) y la VM si `io` no arrancó. El sink común (wire_v1_proto.c). */
static void run_salida(const char* s, size_t n, void* user) {
    wire_v1_salida((wire_v1_salida_t*) user, s, n);
}

/* V5/H4 — EL LATIDO (era de la Pico; desde el RUN común, de todas: `latido=1` en el ENV). Cada 256 vueltas del poll,
 * al log: para un cuelgue, dónde está girando. V7/H12, etapa 2: lo corre `io` mientras el programa sigue en `run`, y
 * las structs de los hilos se devuelven al acabar: no se recorre la cola desde aquí (sin cerrojo, sería leer memoria
 * liberada). El último hilo elegido y su pc, que el planificador deja en dos campos sin punteros. */
static void run_latido(bpvm_t* vm) {
    static unsigned n = 0;
    if ((n++ & 0xFFu) != 0) return;
    log_printf("latido: th%ld pc=%lu", (long) vm->latido_tid, (unsigned long) vm->latido_pc);
}

/* EL POLL, durante el RUN: lo llama `io` en cada vuelta (y la VM entre cuantos si `io` no arrancó).
 *   KILL / RESET  → se apunta su id y devuelve 1: el programa para. Y desde ahí, PEGAJOSO: devuelve 1 en cada vuelta
 *                   sin leer más —`preparar_run` borra la bandera al empezar, y un KILL que llegara entre arrancar `io`
 *                   y `bpvm_run` se perdía (V7_IDEAS: «el poll, pegajoso»)—;
 *   HELLO         → se contesta al vuelo (el IDE puede conectar con algo corriendo y ofrecer Stop);
 *   STAT, GET y LIST troceados → se atienden (#495, paso 4);
 *   el depurador  → a la cola de control, de donde lo saca el lazo de pausa (un solo lector del cable, A1.7);
 *   lo demás      → BUSY. */
static int run_poll(void* user) {
    const bpvm_repl_run_ops_t* r = run_ops();
    (void) user;
    if (s_run_latido && s_run_vm) run_latido(s_run_vm);
    (void) bpvm_repl_paso();                    /* #495 (paso 4): un trozo de lo que se esté mandando */
    if (s_run_kill >= 0 || s_run_reset >= 0) return 1;
    if (!r || !r->leer_linea) return 0;
    int n = r->leer_linea(r->linea, r->linea_cap);
    if (n == 0) return 0;
    if (n < 0) return 1;                        /* el cable se ha ido: el programa se para */
    json_obj_t obj;
    if (json_parse(r->linea, (size_t) n, &obj) != 0) return 0;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof type);
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0)  { s_run_kill = rid;  return 1; }
    if (strcmp(type, "RESET") == 0) { s_run_reset = rid; return 1; }
    if (strcmp(type, "HELLO") == 0) {
        bpvm_repl_antes_de_despachar(type);     /* #495 (paso 4): lo que se mandaba al cliente de antes, cortado */
        (void) bpvm_repl_dispatch(type, rid, &obj);
        return 0;
    }
    if (bpvm_repl_atiende_en_run(type, &obj)) { (void) bpvm_repl_dispatch(type, rid, &obj); return 0; }
    if (bpvm_dbg_wire_kind(type) != BPVM_DBGC_OTHER && s_run_vm && bpvm_io_running(s_run_vm)) {
        if (bpvm_io_ctrl_push(s_run_vm, r->linea, (size_t) n) == 0) return 0;
        /* no cupo: se contesta como siempre, que es mejor que el silencio */
    }
    wire_v1_send_error(rid, "BUSY", BPVM_REPL_OCUPADO_EN_RUN);
    return 0;
}
static int run_poll_vm(bpvm_t* vm, void* user) { (void) vm; return run_poll(user); }

/* V7/H10, el RUN común, paso 6 (3-oct): LA ESCUCHA DE LA VENTANA DEL AUTORUN (#345, `bpvm_autorun_wire_t.escucha`),
 * una para las tres familias —era la misma en las tres, con su lector—. Lee una línea por la cintura del RUN: un KILL
 * es el Stop del IDE (su KILL_REPLY, y «no arranques»: 2); un HELLO se contesta (hay alguien: 1); cualquier otra
 * línea dice también que hay alguien (1) y no se contesta —la atenderá el REPL si no se arranca—; nada, 0. Una línea
 * rota ya no cuenta como alguien: la cintura la descarta, como en el RUN. */
int bpvm_repl_autorun_escucha(void* user) {
    const bpvm_repl_run_ops_t* r = run_ops();
    (void) user;
    if (!r || !r->leer_linea) return 0;
    int n = r->leer_linea(r->linea, r->linea_cap);
    if (n <= 0) return 0;                       /* nada (o el cable se ha ido: nadie a quien esperar) */
    json_obj_t obj;
    if (json_parse(r->linea, (size_t) n, &obj) != 0) return 1;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof type);
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) { wire_v1_send_reply_empty("KILL_REPLY", rid); return 2; }
    if (strcmp(type, "HELLO") == 0) { (void) bpvm_repl_dispatch(type, rid, &obj); return 1; }
    return 1;
}

/* EL DEPURADOR, su cintura (era la de la Pico y la ESP32, idéntica). Las respuestas salen con el cerrojo del cable
 * de `io` (un escritor a la vez: `io` puede estar mandando la salida del programa). */
static void run_dbg_manda(const char* line, size_t len, void* user) {
    (void) user;
    bpvm_io_tx_lock(s_run_vm);
    wire_v1_send_line(line, len);
    bpvm_io_tx_unlock(s_run_vm);
}

static int run_dbg_siguiente(bpvm_dbg_cmd_t* out, void* user) {
    const bpvm_repl_run_ops_t* r = run_ops();
    (void) user;
    int n;
    if (!r) return -1;
    if (s_run_vm && bpvm_io_running(s_run_vm)) {
        /* con `io`, el cable es suyo: el comando llega por la cola de control (espera acotada: el lazo de pausa mira
         * el KILL en cada vuelta, #634) */
        n = (int) bpvm_io_ctrl_pop(s_run_vm, r->dbg_linea, r->dbg_linea_cap, 200);
    } else {
        /* sin `io` (no arrancó): se lee el cable desde aquí, sin esperar */
        n = r->leer_linea(r->dbg_linea, r->dbg_linea_cap);
        if (n == 0) { bpvm_platform_thread_sleep_ms(20); return -1; }
        if (n < 0)  { if (s_run_vm) bpvm_request_kill(s_run_vm); return -1; }
    }
    if (n <= 0) return -1;
    json_obj_t o;
    if (json_parse(r->dbg_linea, (size_t) n, &o) != 0) return -1;
    char type[40];
    if (json_get_str(&o, "type", type, sizeof type) < 0) return -1;
    out->kind = bpvm_dbg_wire_kind(type);
    out->id   = json_get_long(&o, "id",    0);
    out->pc   = json_get_long(&o, "pc",   -1);
    out->bpId = json_get_long(&o, "bpId", -1);
    out->addr = json_get_long(&o, "addr", -1);
    out->ref  = json_get_long(&o, "ref",   0);
    if (out->kind == BPVM_DBGC_KILL) s_run_kill = out->id;   /* su KILL_REPLY, como el de `io` */
    return 0;
}

static bpvm_dbg_wire_t s_run_dbgw = { run_dbg_siguiente, run_dbg_manda, NULL, NULL, 0, 0 };

static void run_dbg_prepara(void) {
    const bpvm_repl_run_ops_t* r = run_ops();
    s_run_dbgw.reply     = r ? r->mensaje : NULL;
    s_run_dbgw.reply_cap = r ? r->mensaje_cap : 0;
}

/* Los comandos del depurador ANTES del RUN (PAUSE, SET_BP, CLR_BP): 1 si lo atendió. Lo pregunta el despachador. */
static int run_depurador_previo(const char* type, long id, json_obj_t* obj) {
    bpvm_dbg_cmd_t c;
    if (!run_ops() || !run_ops()->mensaje) return 0;
    memset(&c, 0, sizeof c);
    c.kind = bpvm_dbg_wire_kind(type);
    if (c.kind == BPVM_DBGC_OTHER) return 0;
    c.id   = id;
    c.pc   = json_get_long(obj, "pc",   -1);
    c.bpId = json_get_long(obj, "bpId", -1);
    c.addr = json_get_long(obj, "addr", -1);
    c.ref  = json_get_long(obj, "ref",   0);
    run_dbg_prepara();
    return bpvm_dbg_wire_handle(&s_run_dbgw, &c);
}

/* Los avisos del escáner de `.mdn`: al LOG, no a OUTPUT, que es del programa (regla del 2-oct). */
static void run_mdn_decir(void* user, const char* msg) {
    (void) user;
    log_printf("%s", msg);
}

/* EL EXITED, UNO: los campos en el orden de siempre, y el mensaje recortado si no cabe (#557: sale SIEMPRE). */
static void run_exited(long sesion, const char* estado, int codigo, unsigned long ms, const char* msg) {
    const bpvm_repl_run_ops_t* r = run_ops();
    char*  b   = r->mensaje;
    size_t cap = r->mensaje_cap;
    int off = wire_v1_msg_begin_event(b, cap, 0, "EXITED");
    if (off >= 0) off = wire_v1_field_long(b, cap, (size_t) off, "session", sesion);
    if (off >= 0) off = wire_v1_field_string(b, cap, (size_t) off, "status", estado);
    if (off >= 0) off = wire_v1_field_long(b, cap, (size_t) off, "exitCode", codigo);
    if (off >= 0) off = wire_v1_field_long(b, cap, (size_t) off, "elapsedMs", (long) ms);
    if (off >= 0) off = wire_v1_field_long(b, cap, (size_t) off, "nativos", (long) bpvm_aot_count());
    if (msg && msg[0] && off >= 0) off = wire_v1_field_string_recortado(b, cap, (size_t) off, "errorMessage", msg, 1);
    if (off >= 0) off = wire_v1_msg_end(b, cap, (size_t) off);
    if (off >= 0) wire_v1_send_line(b, (size_t) off);
    else          wire_v1_send_cstr("{\"type\":\"EXITED\",\"status\":\"INTERNAL_ERROR\",\"exitCode\":3}");
}

/* V7/H12, etapa 2b — UN PROGRAMA CON UN SOLO RUN aunque haya dos (Eduardo, 5-oct: «de momento solamente 2 run
 * ejecutando código BP normal, sin AOT ni LVGL»): el depurador —su pausa congela el programa entero, y su estado es
 * de un hilo— y, mientras tanto, lo que no está preparado para dos: las natives (el contexto del AOT es global en los
 * micros), la GUI (LVGL no es reentrante) y SQLite (compilado sin hilos). `porque`, para el log. */
static int un_solo_run(const bpvm_t* vm, int depurando, const char** porque) {
    if (depurando)            { *porque = "el depurador"; return 1; }
    if (bpvm_aot_count() > 0) { *porque = "natives";      return 1; }
    for (int i = 0; i < vm->module_count; i++) {
        const char* n = vm->modules[i].name;
        if (!strcmp(n, "Gui"))    { *porque = "la GUI"; return 1; }
        if (!strcmp(n, "SQLite")) { *porque = "SQLite"; return 1; }
    }
    return 0;
}

void bpvm_repl_run(long id, const char* path, const char* arg) {
    const bpvm_repl_run_ops_t* r = run_ops();
    if (!r || !r->memoria || !r->leer_linea || !r->mensaje || !r->reiniciar) {
        if (id >= 0) wire_v1_send_error(id, "UNSUPPORTED", "RUN: a la cintura de esta familia le falta una pieza");
        return;
    }
    if (!path || !path[0]) {
        if (id >= 0) wire_v1_send_error(id, "INVALID_PARAM", "falta 'path'");
        return;
    }
    /* La ruta, COPIADA: en las placas apunta dentro del búfer de línea del REPL, y ése es el que lee `io` durante el
     * RUN (`linea` de la cintura): un HELLO a mitad de programa la pisaría antes del log del final. */
    char ruta[BPVM_FS_PATH_MAX];
    snprintf(ruta, sizeof ruta, "%s", path);
    path = ruta;
    if (s_run_sesion != 0) {
        if (id >= 0) wire_v1_send_error(id, "BUSY", "ya hay una sesión RUN en curso");
        else         log_printf("autorun: BUSY (sesión activa) — ignorado");
        return;
    }
    /* H19-F1 — la carpeta del proyecto (si el módulo vive en /app/<proj>/) y la ruta del principal. */
    bpvm_fs_set_basedir_from_module(path);
    /* V5/H4 — los motores nativos ANTES de resolver: si el programa usa una API de pack, tiene que estar publicada
     * cuando llegue la primera llamada (en el RUN y no en el arranque: Eduardo, 7-ago). */
    if (r->cargar_motores) r->cargar_motores();
    {
        char real[BPVM_FS_PATH_MAX];
        uint32_t tam = 0;
        if (bpvm_entry_resolve(path, real, sizeof real, &tam) != 0) {
            if (id >= 0) wire_v1_send_error(id, "NOT_FOUND", "fichero no existe");
            else {
                log_printf("autorun: %s: fichero no existe — REPL normal", path);
                if (r->led) r->led(-1);
            }
            return;
        }
    }

    long sesion = s_run_siguiente++;
    s_run_sesion = sesion;
    if (id >= 0) {   /* el RUN_REPLY con la sesión, ANTES de ejecutar: el cliente sabe que se aceptó */
        int off = wire_v1_msg_begin(r->mensaje, r->mensaje_cap, 0, "RUN_REPLY", id);
        if (off >= 0) off = wire_v1_field_long(r->mensaje, r->mensaje_cap, (size_t) off, "session", sesion);
        if (off >= 0) off = wire_v1_msg_end(r->mensaje, r->mensaje_cap, (size_t) off);
        if (off >= 0) wire_v1_send_line(r->mensaje, (size_t) off);
    }

    /* La VM: el bloque de la familia, con el reparto común (bpvm_stack_region_bytes). */
    uint8_t* base = NULL;
    size_t   bytes = 0;
    bpvm_t*  vm = NULL;
    if (r->memoria(&base, &bytes) == 0 && base && bytes > 0) {
        size_t pilas = bpvm_stack_region_bytes(bytes);
        log_printf("vm: memoria %u KB -> heap %u KB + pilas %u KB", (unsigned) (bytes >> 10),
                   (unsigned) ((bytes - pilas) >> 10), (unsigned) (pilas >> 10));
        vm = bpvm_init(base, bytes, bytes - pilas);
    } else {
        log_printf("run: sin memoria planificada para la VM — no se ejecuta");
    }
    if (!vm) {
        if (r->led) r->led(-1);
        run_exited(sesion, "INTERNAL_ERROR", -1, 0, "no se pudo inicializar la VM");
        s_run_sesion = 0;
        return;
    }
    bpvm_set_run_arg(vm, arg);                 /* V6/#412: lo recoge __runArg antes de entrar en Main */
    wire_v1_salida_inicia(&s_run_salida, sesion);
    bpvm_set_output(vm, run_salida, &s_run_salida);
    if (r->led) r->led(1);

    /* [V6/N1.4] El registro AOT se vacía ANTES de cargar: un .mod con su bloque nativo dentro registra sus thunks
     * mientras se carga, y vaciar después los borraba. */
    bpvm_aot_clear();
    bpvm_entry_t entry;
    memset(&entry, 0, sizeof entry);
    bpvm_status_t ls = bpvm_load_entry(vm, path, &entry);
    if (ls != BPVM_OK) {
        /* REGLA (2-oct): exitCode 2 —el «no se pudo cargar» de la tabla común y del CLI— y el porqué con su prefijo:
         * «falta el modulo 'X'» (y el consejo de la familia, si lo da), «load: <fallo>» o «load: <estado>». */
        char emsg[sizeof entry.fallo + 192];
        const char* consejo = r->consejo_falta ? r->consejo_falta() : NULL;
        if (entry.missing[0])
            snprintf(emsg, sizeof emsg, "falta el modulo '%s'%s%s", entry.missing,
                     consejo ? ": " : "", consejo ? consejo : "");
        else if (entry.fallo[0]) snprintf(emsg, sizeof emsg, "load: %s", entry.fallo);
        else                     snprintf(emsg, sizeof emsg, "load: %s", bpvm_status_str(ls));
        log_printf("run: %s no carga: %s", path, emsg);
        if (r->led) r->led(-1);
        run_exited(sesion, "RUNTIME_ERROR", 2, 0, emsg);
        bpvm_destroy(vm);
        s_run_sesion = 0;
        return;
    }
    if (entry.from_pack) log_printf("run: pack '%s' (main=%s)", entry.resolved, entry.main_module);

    /* #355 — el interruptor del recolector, por el ENV (`gc=0`; por defecto encendido). Era sólo de la Pico. */
    if (r->env_bool && !r->env_bool("gc", 1)) {
        bpvm_set_gc_enabled(vm, 0);
        log_printf("run: GC DESACTIVADO por el ENV (gc=0) — memoria de un solo uso");
    }
    if (r->tras_cargar) r->tras_cargar(vm);
    /* [V7/H2, #493] el `.mdn` de cada módulo, del FS o del pack, con el bucle común; de la familia, traerlo del FS. */
    (void) bpvm_mdn_escanear(vm, r->mdn_del_fs, run_mdn_decir, vm);
    s_run_latido = r->env_bool ? r->env_bool("latido", 0) : 0;
    if (s_run_latido) log_printf("latido: ENCENDIDO (ENV latido=1)");
    if (r->antes_de_correr) r->antes_de_correr(vm);

    /* El depurador: si el cliente dejó breakpoints o pidió PAUSE antes del RUN, los pendientes y el pause_cb. */
    int depurando = bpvm_dbg_wire_armed();
    run_dbg_prepara();
    s_run_dbgw.session = sesion;
    s_run_vm = vm;
    if (depurando) {
        bpvm_dbg_wire_arm(&s_run_dbgw, vm);
        log_printf("run: con el depurador");
    }
    s_run_kill  = -1;
    s_run_reset = -1;
    bpvm_set_poll(vm, run_poll_vm, NULL);      /* sólo si `io` no arranca: con `io`, el cable es suyo */
    bpvm_io_ops_t io_ops = { run_poll, run_salida, &s_run_salida };
    bpvm_status_t rs;
    int64_t t0;
    if (s_tarea.en_cola && bpvm_io_preparar(vm, &io_ops, (size_t) r->io_cola) == 0) {
        /* V7/H12, etapa 2a.2: el programa, a LA COLA; lo corre el run permanente y ésta hace de `io` hasta que acabe (el
         * protocolo, arriba). Sin memoria para las colas de `io`, el RUN de siempre (abajo: el programa en esta tarea,
         * que atiende el cable entre cuantos): la pila de `run` no está medida para el despachador del cable (la
         * revisión de la 2a.2). Un programa colgado así no lo coge el run permanente: no es para él. */
        log_printf("run: %s, sesión %ld", path, sesion);
        /* REGLA (2-oct): elapsedMs, de colgarlo a parar `io` (la última línea del programa, ya fuera). */
        t0 = bpvm_platform_now_ms();
        s_tarea.fin = 0;
        /* 2b: para los runs permanentes que haya (los de `vm.runs`), o para uno (un_solo_run) */
        const char* porque = NULL;
        int runs = un_solo_run(vm, depurando, &porque) ? 1 : s_tarea.en_cola;
        if (runs < s_tarea.en_cola) log_printf("run: con un run de %d (%s)", s_tarea.en_cola, porque);
        int ps = bpvm_cola_colgar(vm, runs, 1, run_acabado, NULL);
        if (ps != BPVM_OK) {
            rs = (bpvm_status_t) ps;               /* no se publicó (un fallo de enlace…): nadie más la ha tocado */
        } else {
            static volatile int ya = 1;   /* el sabotaje de las pruebas: cerrar la cola sin esperar a `run` */
            bpvm_io_lazo(vm, s_tarea.sabotaje ? &ya : &s_tarea.fin);
            bpvm_platform_sem_take(&s_tarea.hecho);   /* `run` ha soltado la VM: desde aquí, es de esta tarea */
            rs = (bpvm_status_t) s_tarea.rs;
        }
        bpvm_io_stop(vm);                          /* sin hilo que unir: libera las colas */
    } else if (s_tarea.viva && bpvm_io_preparar(vm, &io_ops, (size_t) r->io_cola) == 0) {
        /* V7/H12, etapas 0 y 1 (el relevo de la etapa 1, con el interruptor a 0): el programa, en la tarea `run`; ésta
         * hace de `io` hasta que acabe (el protocolo, arriba) */
        log_printf("run: %s, sesión %ld", path, sesion);
        /* REGLA (2-oct): elapsedMs, de `bpvm_run` a parar `io` (la última línea del programa, ya fuera). */
        t0 = bpvm_platform_now_ms();
        s_tarea.vm = vm;
        s_tarea.fin = 0;
        bpvm_platform_sem_give(&s_tarea.trabajo);
        {
            static volatile int ya = 1;   /* el sabotaje de las pruebas: cerrar la cola sin esperar a `run` */
            bpvm_io_lazo(vm, s_tarea.sabotaje ? &ya : &s_tarea.fin);
        }
        bpvm_platform_sem_take(&s_tarea.hecho);   /* `run` ha soltado la VM: desde aquí, es de esta tarea */
        rs = (bpvm_status_t) s_tarea.rs;
        bpvm_io_stop(vm);                          /* sin hilo que unir: libera las colas */
    } else {
        int con_io = bpvm_io_start(vm, &io_ops, (size_t) r->io_cola) == 0;
        log_printf("run: %s, sesión %ld", path, sesion);
        /* REGLA (2-oct): elapsedMs, de `bpvm_run` a parar `io` (la última línea del programa, ya fuera). */
        t0 = bpvm_platform_now_ms();
        rs = bpvm_run(vm);
        /* Parar `io` ANTES de nada más: drena la cola entera, y el cable vuelve a ser de esta tarea (KILL_REPLY y
         * EXITED). */
        if (con_io) bpvm_io_stop(vm);
    }
    unsigned long dt = (unsigned long) (bpvm_platform_now_ms() - t0);
    wire_v1_salida_cierra(&s_run_salida);      /* un carácter que se quedó a medias: U+FFFD, no se tira */
    bpvm_set_poll(vm, NULL, NULL);
    s_run_vm = NULL;
    if (r->led) r->led((rs == BPVM_OK || rs == BPVM_KILLED) ? 0 : -1);

    log_printf("run: %s acabó: %s", path,
               (rs == BPVM_KILLED && s_run_reset >= 0) ? BPVM_REPL_MSG_RESET : bpvm_status_str(rs));
    /* P-run-stop — el ack del KILL, diferido: KILL_REPLY y después EXITED (orden estable para el cliente). */
    if (s_run_kill >= 0) { wire_v1_send_reply_empty("KILL_REPLY", s_run_kill); s_run_kill = -1; }

    /* EL EXITED, UNA REGLA (2-oct): OK, sin mensaje; KILLED, el del RESET o el del KILL —NUNCA el de una excepción
     * vieja: `rt_err` se escribe en cada throw, también en los atrapados—; LINK_ERROR, el de enlace; el resto, el
     * detalle de la excepción o el nombre del estado. */
    const char* link_err = bpvm_link_error(vm);
    const char* rt_err   = bpvm_runtime_error(vm);
    const char* estado;
    const char* msg;
    if (rs == BPVM_OK)            { estado = "OK";            msg = NULL; }
    else if (rs == BPVM_KILLED)   { estado = "KILLED";        msg = (s_run_reset >= 0) ? BPVM_REPL_MSG_RESET : bpvm_status_str(rs); }
    else if (link_err[0])         { estado = "LINK_ERROR";    msg = link_err; }
    else                          { estado = "RUNTIME_ERROR"; msg = rt_err[0] ? rt_err : bpvm_status_str(rs); }
    run_exited(sesion, estado, bpvm_exit_code(rs), dt, msg);

    if (r->despues_de_correr) r->despues_de_correr(vm);
    bpvm_destroy(vm);
    s_run_sesion = 0;
    bpvm_dbg_wire_reset();                     /* la siguiente sesión de depuración parte limpia */

    /* V6/E1 (#452) — el RESET pedido durante el RUN, servido AHORA: el EXITED ya salió. */
    if (s_run_reset >= 0) {
        long rid = s_run_reset;
        s_run_reset = -1;
        r->reiniciar(rid);
    }
}
