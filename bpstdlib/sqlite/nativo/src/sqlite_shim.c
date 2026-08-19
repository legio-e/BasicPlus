/* sqlite_shim.c — el pack OFRECE su API.
 *
 * Hasta aquí el pack sólo consumía: pedía memoria y ficheros por la tabla BIOS.
 * Esto es la otra mitad — construye su tabla de símbolos y la publica en el
 * punto de encuentro, para que un `native` de `SQLite.bp` pueda usar SQLite sin
 * que la VM sepa que SQLite existe.
 *
 * ─── POR QUÉ HAY UN SHIM Y NO SE PUBLICAN LOS SÍMBOLOS DE SQLITE ─────────────
 *
 * Se podría haber publicado `sqlite3_open_v2` tal cual. No se hace por tres
 * motivos, y los tres son de frontera:
 *
 *  1. **Adaptar los tipos.** `sqlite3_open_v2` toma flags y un `sqlite3**`;
 *     nuestro contrato dice «devuelve 0 si no se pudo».
 *  2. **Aislar la versión.** Si SQLite cambia una firma, cambia ESTE fichero y
 *     la tabla sigue igual — que es lo que un pack congelado debe prometer.
 *  3. **Repartir HANDLES, no punteros.** Ver justo abajo; es la parte que más
 *     cambia respecto a la primera versión.
 *
 * ⚠️ Lo que devuelven `col_name` y `get_str` es memoria de SQLite y vale hasta
 * el siguiente `fetch` o el `release`. El lado BP tiene que COPIAR.
 */
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "bpsql_api.h"
#include "sqlite3.h"

/* Para el log. Lo pone `bpsql_publicar`. */
static const bpvm_bios_t* g_bios;

/* Cadena vacía compartida: el contrato dice que `errmsg`, `col_name` y
 * `get_str` NUNCA devuelven NULL. Una columna NULL o un nombre ausente son
 * cosas normales, y devolver NULL obligaría a comprobar en cada llamada del
 * otro lado — donde antes o después se olvidaría. */
static const char VACIA[] = "";

static void avisa(const char* m) { if (g_bios && g_bios->log) g_bios->log(m); }

/* ════════════════════════════════════════════════════════════════════════════
 *  HANDLES, NO PUNTEROS
 *
 * La primera versión repartía el `sqlite3*` tal cual. No vale, y por un motivo
 * DURO que se descubrió al ir a escribir el emisor: **el AOT v1 sólo marshalla
 * valores de 4 bytes** — un `native` no puede tomar ni devolver un `long`.
 * Verificado con un caso real, no leído de un comentario:
 *
 *     AOT no soportado: el tipo 'long' ocupa 8 bytes y el AOT v1 sólo maneja
 *     valores de 4 (parámetros, retorno y variables locales).
 *
 * Un puntero cabe en 4 bytes en el micro pero NO en el PC de 64 bits, y el
 * mismo `SQLite.bp` tiene que valer en los dos. Así que se reparte un índice.
 *
 * ─── Y RESULTA QUE ES MEJOR, no un apaño ───
 *
 * Es exactamente la doctrina de los handles de V4 (`idx` + `gen`): un handle
 * caducado —de una BD ya cerrada— NO apunta a memoria liberada, sino a una
 * ranura cuya generación ya no cuadra. O sea que se DETECTA y se dice, en vez
 * de saltar por un puntero basura. Con punteros crudos, cerrar dos veces o usar
 * una consulta de una conexión cerrada era un use-after-free silencioso.
 *
 * Formato: [ gen:16 | idx:16 ], y 0 = handle nulo. La generación sube al
 * liberar la ranura, así que reusar el número viejo no cuela.
 * ════════════════════════════════════════════════════════════════════════════ */

#define MAX_DB   4     /* conexiones a la vez. Son pocas por diseño.          */
#define MAX_ST  64     /* consultas vivas. 64 es lo MEDIDO el 7-ago como tope
                        * probado; más caben, pero éste es el número con dato. */

typedef struct { void* p; uint16_t gen; } ranura_t;

static ranura_t s_db[MAX_DB];
static ranura_t s_st[MAX_ST];

static int32_t dar(ranura_t* t, int n, void* p)
{
    int i;
    for (i = 0; i < n; i++) {
        if (t[i].p == 0) {
            t[i].p = p;
            if (t[i].gen == 0) t[i].gen = 1;      /* gen 0 se reserva para nulo */
            return (int32_t) (((uint32_t) t[i].gen << 16) | (uint32_t) (i + 1));
        }
    }
    return 0;                                      /* sin ranuras */
}

static void* mira(ranura_t* t, int n, int32_t h)
{
    uint32_t u = (uint32_t) h;
    uint32_t idx = (u & 0xFFFFu);
    uint16_t gen = (uint16_t) (u >> 16);
    if (h == 0 || idx == 0 || (int) idx > n) return 0;
    if (t[idx - 1].gen != gen) return 0;           /* caducado: lo DICE */
    return t[idx - 1].p;
}

static void soltar(ranura_t* t, int n, int32_t h)
{
    uint32_t u = (uint32_t) h;
    uint32_t idx = (u & 0xFFFFu);
    uint16_t gen = (uint16_t) (u >> 16);
    if (h == 0 || idx == 0 || (int) idx > n) return;
    if (t[idx - 1].gen != gen) return;
    t[idx - 1].p = 0;
    t[idx - 1].gen++;                              /* el viejo ya no cuela */
    if (t[idx - 1].gen == 0) t[idx - 1].gen = 1;
}

static sqlite3*      db_de(int32_t h) { return (sqlite3*)      mira(s_db, MAX_DB, h); }
static sqlite3_stmt* st_de(int32_t h) { return (sqlite3_stmt*) mira(s_st, MAX_ST, h); }

/* ── LA CONEXIÓN ──────────────────────────────────────────────────────────── */

static int32_t sh_open(const char* camino, int32_t solo_lectura)
{
    sqlite3* db = 0;
    int32_t h;
    int flags = solo_lectura ? SQLITE_OPEN_READONLY
                             : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    int rc = sqlite3_open_v2(camino, &db, flags, 0);

    if (rc != SQLITE_OK || db == 0) {
        /* ⚠️ Al fallar, SQLite PUEDE dejar un handle igualmente y hay que
         * cerrarlo o se fuga. Y como devolvemos 0, su mensaje se pierde: por eso
         * se escribe en el log ANTES de soltarlo. Sin esto, «no se pudo abrir»
         * sería todo, y el motivo real —permisos, ruta, disco— acabaría escrito
         * y tirado a la basura. */
        if (g_bios && g_bios->log) {
            char b[128];
            sqlite3_snprintf((int) sizeof b, b, "sqlite: no abre '%s': %s",
                             camino, db ? sqlite3_errmsg(db) : sqlite3_errstr(rc));
            g_bios->log(b);
        }
        if (db) sqlite3_close_v2(db);
        return 0;
    }

    h = dar(s_db, MAX_DB, db);
    if (h == 0) {                       /* sin ranuras: se DICE, no se calla */
        avisa("sqlite: no quedan conexiones libres (4 abiertas) — falta un close()");
        sqlite3_close_v2(db);
    }
    return h;
}

static int32_t sh_exec(int32_t h, const char* sql)
{
    sqlite3* db = db_de(h);
    if (db == 0) { avisa("sqlite: exec sobre una conexion cerrada o invalida"); return -1; }
    /* Sin `errmsg` de salida a propósito: ése habría que liberarlo con
     * `sqlite3_free` y el contrato no tiene dónde decirlo. El mensaje sale por
     * `errmsg`, que no aloca nada. */
    return (int32_t) sqlite3_exec(db, sql, 0, 0, 0);
}

static int32_t sh_prepare(int32_t h, const char* sql)
{
    sqlite3* db = db_de(h);
    sqlite3_stmt* st = 0;
    int32_t hs;
    if (db == 0) { avisa("sqlite: prepare sobre una conexion cerrada o invalida"); return 0; }
    if (sqlite3_prepare_v2(db, sql, -1, &st, 0) != SQLITE_OK) return 0;
    hs = dar(s_st, MAX_ST, st);
    if (hs == 0) {
        avisa("sqlite: no quedan consultas libres (64 vivas) — falta algun release()");
        sqlite3_finalize(st);
    }
    return hs;
}

/* ⚠️ 8 BYTES POR ARRAY DE SALIDA, no por retorno. El AOT v1 no puede devolver
 * un `long`, y un rowid SÍ es de 64 bits. El array es un handle de 4 bytes, así
 * que cruza sin problema, y BP escribe/lee su elemento 0. El lado BP lo tapa:
 * `Db.lastInsertId(): long` sigue siendo `long` para el usuario. */
static void sh_last_id(int32_t h, int64_t* destino)
{
    sqlite3* db = db_de(h);
    if (destino) *destino = db ? (int64_t) sqlite3_last_insert_rowid(db) : 0;
}

static int32_t sh_changes(int32_t h)
{
    sqlite3* db = db_de(h);
    return db ? (int32_t) sqlite3_changes(db) : 0;
}

static void sh_close(int32_t h)
{
    sqlite3* db = db_de(h);
    if (db == 0) return;                /* tolerante: cerrar dos veces no hace nada */
    sqlite3_close_v2(db);
    soltar(s_db, MAX_DB, h);
}

static const char* sh_errmsg(int32_t h)
{
    sqlite3* db = db_de(h);
    const char* m = db ? sqlite3_errmsg(db) : 0;
    return m ? m : VACIA;
}

/* ── LA CONSULTA ──────────────────────────────────────────────────────────── */

static int32_t sh_fetch(int32_t h)
{
    sqlite3_stmt* st = st_de(h);
    int rc;
    if (st == 0) { avisa("sqlite: fetch sobre una consulta ya soltada"); return -1; }
    rc = sqlite3_step(st);
    if (rc == SQLITE_ROW)  return 1;
    if (rc == SQLITE_DONE) return 0;
    return -1;                          /* error de verdad: en BP es excepción */
}

static int32_t sh_col_count(int32_t h) {
    sqlite3_stmt* st = st_de(h);
    return st ? (int32_t) sqlite3_column_count(st) : 0;
}
static int32_t sh_col_type(int32_t h, int32_t i) {
    sqlite3_stmt* st = st_de(h);
    return st ? (int32_t) sqlite3_column_type(st, (int) i) : BPSQL_NULL;
}
static const char* sh_col_name(int32_t h, int32_t i) {
    sqlite3_stmt* st = st_de(h);
    const char* n = st ? sqlite3_column_name(st, (int) i) : 0;
    return n ? n : VACIA;
}

static int32_t sh_get_int(int32_t h, int32_t i) {
    sqlite3_stmt* st = st_de(h);
    return st ? (int32_t) sqlite3_column_int(st, (int) i) : 0;
}
static const char* sh_get_str(int32_t h, int32_t i) {
    sqlite3_stmt* st = st_de(h);
    const unsigned char* t = st ? sqlite3_column_text(st, (int) i) : 0;
    return t ? (const char*) t : VACIA;
}

/* Los dos de 8 bytes, por array de salida. Motivo arriba, en `sh_last_id`. */
static void sh_get_long(int32_t h, int32_t i, int64_t* destino) {
    sqlite3_stmt* st = st_de(h);
    if (destino) *destino = st ? (int64_t) sqlite3_column_int64(st, (int) i) : 0;
}
static void sh_get_double(int32_t h, int32_t i, double* destino) {
    sqlite3_stmt* st = st_de(h);
    if (destino) *destino = st ? sqlite3_column_double(st, (int) i) : 0.0;
}

/* El blob se copia a un `byte[]` que trae el llamante, y devuelve cuántos bytes
 * había DE VERDAD — así BP sabe si su buffer se quedó corto en vez de creerse
 * que el dato medía lo que cupo. */
static int32_t sh_get_blob(int32_t h, int32_t i, unsigned char* destino, int32_t cabe)
{
    sqlite3_stmt* st = st_de(h);
    const void* p;
    int n;
    if (st == 0) return 0;
    p = sqlite3_column_blob(st, (int) i);
    n = sqlite3_column_bytes(st, (int) i);
    if (p && destino && cabe > 0) {
        int cuantos = (n < cabe) ? n : cabe;
        int k;
        for (k = 0; k < cuantos; k++) destino[k] = ((const unsigned char*) p)[k];
    }
    return (int32_t) (p ? n : 0);
}

static void sh_release(int32_t h)
{
    sqlite3_stmt* st = st_de(h);
    if (st == 0) return;                /* tolerante: soltar dos veces no hace nada */
    sqlite3_finalize(st);
    soltar(s_st, MAX_ST, h);
}

/* ── LOS PUBLICS ──────────────────────────────────────────────────────────────
 *
 * Modelo de Eduardo: igual que un `.mod` lleva su sección EXPORTS, el pack lleva
 * la suya. Cada entrada es NOMBRE → función, y el nombre es exactamente el que
 * escribe `SQLite.bp` en su `native function <nombre>(...)`.
 *
 * Y por eso el ORDEN YA NO ES EL CONTRATO: se puede reordenar o meter uno en
 * medio y nada se rompe — el casado es por nombre. Lo peor que puede pasar es
 * que un nombre no aparezca, y eso el loader lo DICE.
 *
 * `static const` ⇒ vive en la `.rodata` del pack: va a flash, no gasta RAM.
 *
 * ⚠️ Cada nombre tiene que ser IDENTIFICADOR VÁLIDO DE BP y no palabra
 * reservada. Aquí ya mordió: `step` lo es (el `for … step`), de ahí `fetch`. */
/* El nombre publicado lleva prefijo `sql_` porque en BP estos símbolos
 * conviven con los MÉTODOS homónimos de `Db` (`open`, `exec`, `close`…) y
 * un nombre suelto igual que un método es justo la ambigüedad que no se
 * quiere tener que razonar. El prefijo lo pone la macro, así que sigue sin
 * haber dos listas que mantener. */
#define SIM(n)  { "sql_" #n, (bpvm_pack_fn_t) sh_##n }

static const bpvm_pack_sym_t PUBLICS[] = {
    SIM(open), SIM(exec), SIM(prepare), SIM(last_id), SIM(changes),
    SIM(close), SIM(errmsg),
    SIM(fetch), SIM(col_count), SIM(col_name), SIM(col_type),
    SIM(get_int), SIM(get_long), SIM(get_double), SIM(get_str), SIM(get_blob),
    SIM(release)
};

/* El nombre sale del propio identificador con `#n`, así que nombre y puntero NO
 * PUEDEN desalinearse: no hay dos listas que mantener. Era el fallo clásico de
 * las tablas nombre↔función escritas a mano. */
static const bpvm_pack_api_t LA_TABLA = {
    BPSQL_MARCA, BPSQL_VERSION,
    (uint32_t) (sizeof PUBLICS / sizeof PUBLICS[0]),
    PUBLICS
};

/*
 * Publica la tabla en el punto de encuentro. La llama la entrada del pack, UNA
 * vez, después de que SQLite esté arrancado.
 *
 * Se VERIFICA antes de publicarla aunque la acabemos de escribir tres líneas
 * más arriba: lo que se comprueba de verdad es que la tabla y `bpvm_pack_api.h`
 * siguen diciendo lo mismo, y que ningún símbolo se quedó a medio poner.
 *
 *   0  publicada
 *  -1  la tabla tiene un hueco (se dice cuál en el log)
 *  -2  el firmware no la aceptó (marca repetida, o registro lleno)
 */
int bpsql_publicar(const bpvm_bios_t* bios)
{
    const char* falta;
    int rc;
    int i;

    g_bios = bios;
    if (bios == 0 || bios->publica == 0) return -2;

    /* Las ranuras arrancan limpias. Importa en el HOST, donde el mismo proceso
     * puede publicar dos veces en una prueba; en placa el .bss ya viene a cero. */
    for (i = 0; i < MAX_DB; i++) { s_db[i].p = 0; s_db[i].gen = 1; }
    for (i = 0; i < MAX_ST; i++) { s_st[i].p = 0; s_st[i].gen = 1; }

    falta = bpvm_pack_api_verify(&LA_TABLA, BPSQL_MARCA, BPSQL_VERSION);
    if (falta != 0) {
        char b[96];
        sqlite3_snprintf((int) sizeof b, b, "sqlite: mi tabla no vale: %s", falta);
        avisa(b);
        return -1;
    }

    rc = bios->publica(BPSQL_MARCA, &LA_TABLA);
    if (rc != 0) {
        char b[96];
        sqlite3_snprintf((int) sizeof b, b,
                         "sqlite: el firmware NO acepto la tabla (rc=%d)", rc);
        avisa(b);
        return -2;
    }

    {
        char b[96];
        sqlite3_snprintf((int) sizeof b, b,
                         "sqlite: API publicada como 'SQLI' — %d simbolos, v%d",
                         (int) LA_TABLA.n, (int) BPSQL_VERSION);
        avisa(b);
    }
    return 0;
}
