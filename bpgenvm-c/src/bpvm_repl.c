/*
 * bpvm_repl.c — los verbos COMUNES del REPL wire v1. Ver bpvm_repl.h para el
 * porqué y el reparto común/familia.
 *
 * Regla de la casa: cada verbo de aquí hace LO MISMO que hacía en la familia
 * de referencia (la Pico, el superconjunto), con las divergencias unificadas
 * a propósito y anotadas en el header. Nada de mejoras de paso: primero
 * idéntico y en placa, después ya se verá.
 */
#include "bpvm_repl.h"
#include "bpvm_wire_v1.h"
#include "bpvm_log.h"
#include "bpvm_fs.h"
#include "bpvm_listdir.h"   /* V6/U3.22: el núcleo de LIST_DIR ya era común */
#include "bpvm_rtc.h"
#include "bpvm_platform.h"  /* V6/U3 g9: now_ms para el durationMs del SAVE */

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

static void repl_del(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
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

static void repl_stat(long id, const json_obj_t* obj) {
    char path[64];
    char buf[192];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path");
        return;
    }
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) {
        wire_v1_send_error(id, "NOT_FOUND", "no existe");
        return;
    }
    int off = wire_v1_msg_begin(buf, sizeof buf, 0, "STAT_REPLY", id);
    if (off < 0) goto err;
    off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "size", (long) size);
    if (off < 0) goto err;
    /* #398 — el CRC solo si se pide: es lo que antes calculaba el LIST para
     * TODOS los ficheros en cada refresco del árbol. */
    if (json_get_bool(obj, "crc", 0)) {
        uint32_t c = 0;
        long v = (bpvm_fs_crc32(path, &c) == 0) ? (long) c : -1L;
        off = wire_v1_field_long(buf, sizeof buf, (size_t) off, "crc", v);
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

static void repl_rename(long id, const json_obj_t* obj) {
    char from[64], to[64];
    if (json_get_str(obj, "from", from, sizeof from) < 0 ||
        json_get_str(obj, "to",   to,   sizeof to)   < 0) {
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

static void repl_get(long id, const json_obj_t* obj) {
    char path[64];
    char buf[96];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
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

static void repl_list_cb(const char* name, int is_dir, uint32_t size, void* user) {
    repl_list_ctx_t* c = (repl_list_ctx_t*) user;
    int is_root = (c->dir[1] == '\0');
    if (is_dir) {
        if (c->tail < REPL_LIST_MAX_DIRS) {
            snprintf(c->pending[c->tail], REPL_LIST_NAME_MAX, "%s%s%s",
                     c->dir, is_root ? "" : "/", name);
            c->tail++;
        } else {
            c->omitidas++;   /* #425: un directorio sin recorrer TAMBIEN falta */
            log_printf("fs: LISTADO INCOMPLETO — mas de %d directorios; '%s' sin recorrer",
                       REPL_LIST_MAX_DIRS, name);
        }
        return;              /* los dirs no se emiten (legado plano) */
    }
    char ent[192];
    char full[REPL_LIST_NAME_MAX];
    if (is_root) snprintf(full, sizeof full, "%s", name);
    else         snprintf(full, sizeof full, "%s/%s", c->dir, name);
    /* Escape minimo del nombre: en un path solo asoman `"` y el separador. */
    char esc[REPL_LIST_NAME_MAX * 2]; size_t o = 0;
    for (const char* q = full; *q && o + 2 < sizeof esc; q++) {
        if (*q == '"' || *q == '\\') esc[o++] = '\\';
        esc[o++] = *q;
    }
    esc[o] = '\0';
    int w = snprintf(ent, sizeof ent,
        "%s{\"name\":\"%s\",\"size\":%lu,\"crc\":-1,\"isDir\":false,\"mtime\":0}",
        c->first ? "" : ",", esc, (unsigned long) size);
    if (w > 0) { wire_v1_send_bulk((const uint8_t*) ent, (size_t) w); c->first = 0; c->emitidas++; }
}

static void repl_list(long id) {
    static char pending[REPL_LIST_MAX_DIRS][REPL_LIST_NAME_MAX];
    const long  t_ini = (long) bpvm_platform_now_ms();   /* U3.23: total del listado */
    char head[64];
    int hn = snprintf(head, sizeof head,
                      "{\"type\":\"LIST_REPLY\",\"id\":%ld,\"entries\":[", id);
    if (hn <= 0) return;
    wire_v1_send_bulk((const uint8_t*) head, (size_t) hn);

    /* U3.23 — `memset` y NO campo a campo, que es como estaba y me mordió: al
     * añadir los casilleros del desglose (`nraices`, `raiz*`) la inicialización
     * parcial los dejó con BASURA de pila, y `repl_list_apunta` usaba `nraices`
     * como tope de bucle → recorría memoria ajena y el LIST no contestaba.
     *
     * El código del ESP32 que sustituí ya hacía `memset` y llevaba el porqué
     * escrito («con la struct ya no de un solo campo, la inicialización parcial
     * saca -Wmissing-field-initializers»). Estaba dicho y no lo traje. */
    repl_list_ctx_t c;
    memset(&c, 0, sizeof c);
    c.id = id; c.first = 1;
    c.pending = pending;
    snprintf(pending[c.tail++], REPL_LIST_NAME_MAX, "/");
    int head_i = 0;
    while (head_i < c.tail) {
        char dir[REPL_LIST_NAME_MAX];
        snprintf(dir, sizeof dir, "%.63s", pending[head_i++]);   /* copia: sin aliasing */
        c.dir = dir;
        int   antes = c.emitidas;
        long  t0    = (long) bpvm_platform_now_ms();
        (void) bpvm_fs_list(dir, repl_list_cb, &c);   /* un volumen caido no borra el resto */
        repl_list_apunta(&c, dir, c.emitidas - antes,
                         (long) bpvm_platform_now_ms() - t0);
    }
    char cola[40];
    int cn = snprintf(cola, sizeof cola, "],\"omitted\":%d}", c.omitidas);
    if (cn > 0) wire_v1_send_line(cola, (size_t) cn);

    /* #398/#408 — el desglose, en UNA línea (el log es un bien escaso, #423).
     * `total` es lo que tarda el device; lo que el usuario ve incluye además el
     * viaje por el wire y el pintado del árbol, y eso lo mide el IDE. Que los dos
     * números existan es lo que dirá si el tiempo se va aquí o allí. */
    char det[160];
    int o = 0;
    for (int i = 0; i < c.nraices && o < (int) sizeof(det) - 24; i++) {
        o += snprintf(det + o, sizeof(det) - (size_t) o, " %s:%d/%ldms",
                      c.raiz[i], c.raiz_n[i], c.raiz_ms[i]);
    }
    det[o] = '\0';
    log_printf("ls: %d ent en %ld ms |%s (%d dirs omitidos)",
               c.emitidas, (long) bpvm_platform_now_ms() - t_ini, det, c.omitidas);
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
    CAMPO_S("uniqueId",     in.unique_id);
    CAMPO_S("boardName",    in.board_name);
    CAMPO_L("arch",         in.arch);
    CAMPO_L("cpuFreqHz",    in.cpu_hz);
    CAMPO_L("uptimeMs",     in.uptime_ms);
    CAMPO_S("resetReason",  in.reset_reason);
    CAMPO_L("tempMilliC",   in.temp_milli_c);
    CAMPO_L("gpioCount",    in.gpio_count);
    CAMPO_L("pioCount",     in.pio_count);
    CAMPO_L("pwmSlices",    in.pwm_slices);
    CAMPO_L("adcChannels",  in.adc_channels);
    CAMPO_L("flashBytes",   in.flash_bytes);
    CAMPO_L("sramBytes",    in.sram_bytes);
    CAMPO_L("psramBytes",   in.psram_bytes);
    CAMPO_L("vmHeapBytes",  in.vm_heap_bytes);
    CAMPO_L("vmStackBytes", in.vm_stack_bytes);
    CAMPO_L("fsTotalBytes", in.fs_total_bytes);
    CAMPO_L("fsUsedBytes",  in.fs_used_bytes);
#undef CAMPO_S
#undef CAMPO_L
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

static void repl_format(long id, const json_obj_t* obj) {
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
    char buf[320];
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
 * muerde subiendo a una carpeta nueva, que es lo que el IDE aún no hace. */
static void crear_dirs_padre(const char* path) {
    const char* ult = strrchr(path, '/');
    if (!ult || ult == path) return;      /* en la raíz o sin carpeta: nada que crear */
    char dir[64];
    size_t n = (size_t)(ult - path);
    if (n >= sizeof dir) return;          /* que lo rechace el write, con su error */
    memcpy(dir, path, n);
    dir[n] = '\0';
    bpvm_fs_mkdir(dir);                   /* recursivo y ok-si-existe en los 3 backends */
}

/* PUT — subida de un tirón (para ficheros que caben en el scratch; los grandes
 * van por PUT_BEGIN/DATA/END). */
static void repl_put(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof path) < 0) {
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
        wire_v1_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
    }
    crear_dirs_padre(path);                             /* #455 */
    if (bpvm_fs_write(path, s_ops->put_buf, (uint32_t) bulk, 0) != 0) {
        const char *code, *msg;
        fs_fallo("put", path, (unsigned long) bulk, &code, &msg);
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
 * en uno. */
static struct {
    int           active;
    char          path[64];
    unsigned long received;
    unsigned long expected;
} s_put_sess;

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

static void repl_put_begin(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof path) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta path"); return;
    }
    crear_dirs_padre(path);                          /* #455 */
    if (bpvm_fs_write(path, NULL, 0, 0) != 0) {      /* crea/trunca */
        const char *code, *msg;
        fs_fallo("put", path, 0, &code, &msg);
        wire_v1_send_error(id, code, msg); return;
    }
    s_put_sess.active   = 1;
    s_put_sess.received = 0;
    s_put_sess.expected = (unsigned long) json_get_long(obj, "size", 0);
    strncpy(s_put_sess.path, path, sizeof s_put_sess.path - 1);
    s_put_sess.path[sizeof s_put_sess.path - 1] = '\0';
    reply_put_field("PUT_BEGIN_REPLY", id, 0, "received");
}

static void repl_put_data(long id, const json_obj_t* obj) {
    long bulk = json_get_long(obj, "bulk", -1);
    if (bulk < 0) { wire_v1_send_error(id, "INVALID_PARAM", "falta bulk"); return; }

    if ((unsigned long) bulk > s_ops->put_buf_size) {
        if (tragar_bulk((unsigned long) bulk) < 0) {
            wire_v1_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
        }
        s_put_sess.active = 0;
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
    if (bulk > 0 && bpvm_fs_write(s_put_sess.path, s_ops->put_buf,
                                  (uint32_t) bulk, 1) != 0) {
        const char *code, *msg;
        s_put_sess.active = 0;
        fs_fallo("append", s_put_sess.path, (unsigned long) bulk, &code, &msg);
        wire_v1_send_error(id, code, msg); return;
    }
    s_put_sess.received += (unsigned long) bulk;
    reply_put_field("PUT_DATA_REPLY", id, s_put_sess.received, "received");
}

static void repl_put_end(long id, const json_obj_t* obj) {
    (void) obj;
    if (!s_put_sess.active) {
        wire_v1_send_error(id, "NO_SESSION", "PUT_END sin PUT_BEGIN"); return;
    }
    unsigned long recv = s_put_sess.received;
    unsigned long exp  = s_put_sess.expected;
    s_put_sess.active = 0;
    if (exp != 0 && recv != exp) {
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

static void repl_list_dir(long id, const json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof path) < 0)
        snprintf(path, sizeof path, "/");        /* sin path = la raíz */

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

int bpvm_repl_dispatch(const char* type, long id, const json_obj_t* obj) {
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
    if (strcmp(type, "LIST") == 0) { repl_list(id); return 1; }
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
    if (strcmp(type, "GET")    == 0) { repl_get(id, obj);    return 1; }
    if (strcmp(type, "MKDIR")  == 0) {
        /* U3.25 — CREA EL DIRECTORIO DE VERDAD.
         *
         * Antes esto contestaba OK sin tocar nada, con el argumento de que «en un
         * FS plano con '/' como namespace no hay nodos de directorio». Eso era
         * cierto del FS en RAM del STM32 y dejó de serlo hace tiempo: los TRES
         * backends (`fs_lfs.c`, `fs_host.c`) hacen `mkdir` recursivo y ok-si-existe.
         * De hecho este mismo fichero ya lo llamaba —`crear_dirs_padre()` en cada
         * PUT (#455)—, así que el común creaba directorios al subir un fichero y
         * decía «hecho, nada» cuando se le pedía uno. Lo destapó el arnés del
         * simulador (`U3.24`), que traía un MKDIR de verdad: unificar al stub era
         * la regresión disfrazada de limpieza de #455. */
        char path[64];
        if (json_get_str(obj, "path", path, sizeof path) < 0) {
            wire_v1_send_error(id, "INVALID_PARAM", "falta path");
            return 1;
        }
        if (bpvm_fs_mkdir(path) != 0) {
            wire_v1_send_error(id, "INTERNAL_ERROR", "no se pudo crear el directorio");
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
        char path[64];
        if (json_get_str(obj, "path", path, sizeof path) < 0) {
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
