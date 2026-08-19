/* sqlite_pack.c — el PEGAMENTO del pack de SQLite: shim de libc + entrada.
 *
 * Es el sucesor de `A/bios_shim.c`, que era una SONDA de la prueba A. Aquella
 * declaraba su PROPIA copia de la tabla de la BIOS, y esa copia se quedó rancia:
 * empieza en `version` cuando la de verdad empieza en `magic`, o sea 8 bytes de
 * desfase. Su `memcpy` (offset 4) leería el `version` real —un entero— y la
 * primera llamada saltaría a la dirección 2.
 *
 * Por eso aquí se INCLUYE `bpvm_bios.h` en vez de copiarlo. Una tabla que cruza
 * una frontera binaria se declara UNA vez; copiarla es fabricar un desfase que
 * no da error de compilación ni de enlace, sólo un salto a ninguna parte.
 *
 * ─── QUÉ CIERRA ESTE FICHERO ───
 *
 * `A/undef.txt` es el contrato: los símbolos que `sqlite3.o` deja abiertos.
 *   · 18 __aeabi_*  — NO se pueden envolver (ldivmod devuelve un PAR en r0:r1,
 *                     y eso no se expresa en C). Van de libgcc, al enlazar.
 *   · 16 de libc    — los envoltorios de aquí abajo, todos por la TABLA.
 *   · os_init/end   — el VFS. Vive en `vfs_bp.c`, al lado.
 *
 * Nada se llama por nombre: todo pasa por la tabla. Así una actualización de
 * firmware no rompe un pack ya grabado — que es el motivo de que la tabla exista.
 *
 * Se compila con -fno-builtin: sin eso GCC convierte el cuerpo de `memcpy` en
 * una llamada a `memcpy` y se come la pila.
 */
#include "bpvm_bios.h"
#include "sqlite3.h"

/* ─── AUTOTEST: APAGADO ───────────────────────────────────────────────────────
 *
 * En H3 este pack ERA el experimento, y su entrada hacia la prueba entera: leer
 * la BD del usuario, crear otra, reservar 64 KB y comprobarlo todo. Estaba bien
 * entonces — el pack no tenia a nadie a quien servir.
 *
 * En H4 el pack es una LIBRERIA: alguien (SQLite.bp) le va a pedir cosas. Su
 * entrada tiene que arrancar el motor, publicar la tabla y CALLARSE. Dejar el
 * autotest significaria que cada carga repite las pruebas de H3 —con sus
 * escrituras en la SD— antes de que el programa del usuario haga nada.
 *
 * No se borra: como diagnostico vale, y el dia que el pack no arranque en una
 * placa nueva se enciende esto y contesta solo. Compilar con -DBPSQL_AUTOTEST=1. */
#ifndef BPSQL_AUTOTEST
#define BPSQL_AUTOTEST 0
#endif

int bpvfs_instalar(const bpvm_bios_t* bios);   /* vfs_bp.c   */
int bpsql_publicar(const bpvm_bios_t* bios);   /* sqlite_shim.c */

/* Vive en .bss → base de RAM. Lo rellena la propia entrada, no el cargador:
 * así el cargador sólo necesita UN dato del pack (el offset de la entrada) en
 * vez de dos. La simplificación es de `mini.c` y se mantiene. */
static const bpvm_bios_t* g_bios;

/* ── memoria ── */
void*  memcpy (void* d, const void* s, size_t n)       { return g_bios->memcpy(d, s, n); }
void*  memmove(void* d, const void* s, size_t n)       { return g_bios->memmove(d, s, n); }
void*  memset (void* d, int c, size_t n)               { return g_bios->memset(d, c, n); }
int    memcmp (const void* a, const void* b, size_t n) { return g_bios->memcmp(a, b, n); }
void*  memchr (const void* p, int c, size_t n)         { return g_bios->memchr(p, c, n); }

/* ── cadenas ── */
size_t strlen (const char* s)                          { return g_bios->strlen(s); }
int    strcmp (const char* a, const char* b)           { return g_bios->strcmp(a, b); }
int    strncmp(const char* a, const char* b, size_t n) { return g_bios->strncmp(a, b, n); }
char*  strchr (const char* s, int c)                   { return g_bios->strchr(s, c); }
char*  strrchr(const char* s, int c)                   { return g_bios->strrchr(s, c); }
size_t strspn (const char* s, const char* a)           { return g_bios->strspn(s, a); }
size_t strcspn(const char* s, const char* a)           { return g_bios->strcspn(s, a); }

/* ── alocador ──
 * ⚠️ Con `SQLITE_CONFIG_HEAP` puesto, SQLite NO usa esto: se lo gestiona todo
 * con su MEMSYS5 dentro de la arena. Siguen aquí porque `sqlite3.o` los deja
 * como símbolos abiertos y el enlace los exige — y porque si algún día se
 * quitara el CONFIG_HEAP, el camino de respaldo tiene que existir y DECIR que
 * no hay arena en vez de devolver basura. */
void*  malloc (size_t n)                               { return g_bios->malloc(n); }
void   free   (void* p)                                { g_bios->free(p); }
void*  realloc(void* p, size_t n)                      { return g_bios->realloc(p, n); }

/* ── tiempo ── */
struct tm* localtime(const void* t)                    { return g_bios->localtime(t); }

/* ── LA EXCEPCIÓN A "TODO PASA POR LA TABLA": `fabs` ──────────────────────────
 *
 * Y no la rompe: la confirma. La tabla existe para que una actualización de
 * firmware no rompa un pack ya grabado. `fabs` no tiene firmware detrás — es
 * apagar el bit de signo. Implementarlo AQUÍ lo hace más independiente, no
 * menos, y no hay ranura de matemáticas en `bpvm_bios_t` justamente por esto.
 *
 * Apareció al compilar para RISC-V, no antes, y el motivo es del silicio: en
 * ARM (fpv5-sp-d16) GCC lo mete EN LÍNEA dentro del propio pack, así que el
 * símbolo nunca llegaba al enlace. Con `ilp32f` —FPU de simple precisión— no
 * hay `fabs.d` que emitir sobre un `double` y sale una llamada de verdad.
 *
 * O sea que en ARM el pack YA se autoproveía este cálculo; lo único nuevo es
 * que ahora se ve escrito. Por eso va en el fichero común y no en uno por
 * familia: la diferencia está en quién la inlinea, no en lo que hace.
 *
 * Sin <math.h> a propósito: el pack es freestanding. La unión evita el
 * type-punning por puntero, que con -O2 es UB y GCC se lo cree.
 */
double fabs(double x)
{
    union { double d; uint64_t u; } v;
    v.d = x;
    v.u &= 0x7FFFFFFFFFFFFFFFull;   /* fuera el bit de signo */
    return v.d;
}

/* ── LA VOZ DE SQLITE ─────────────────────────────────────────────────────────
 *
 * SQLite reporta sus errores internos por `sqlite3_log()`, y por defecto ese
 * canal va a la basura. Engancharlo cuesta tres líneas y convierte un
 * "rc=1 (ERROR)" —que no dice nada— en la frase que él mismo escribió.
 */
static void sql_log(void* pArg, int code, const char* msg)
{
    char b[112];
    (void) pArg;
    if (!g_bios || !g_bios->log) return;
    sqlite3_snprintf((int) sizeof b, b, "sqlite[%d]: %s", code, msg ? msg : "(sin texto)");
    g_bios->log(b);
}

/* ── El punto de extensión de SQLite ──────────────────────────────────────────
 *
 * Lo llama `sqlite3_initialize()`. Registrar aquí un VFS NO es opcional: si no
 * hay ninguno, `sqlite3MemdbInit()` hace `sqlite3_vfs_find(0)`, no encuentra
 * nada y devuelve SQLITE_ERROR — y como va envuelto en `NEVER()`, ni lo loguea.
 * Ése fue el fallo del 8-ago, y costó tres grabaciones antes de reproducirlo en
 * el PC en diez minutos.
 */
int sqlite3_os_init(void) { return bpvfs_instalar(g_bios); }
int sqlite3_os_end (void) { return SQLITE_OK; }

/* ── LA BASE DE DATOS, EN LA SD ────────────────────────────────────────────────
 *
 * Es EXACTAMENTE el mismo ejercicio que el arnés del PC
 * (`notas/v5-sqlite-prueba/H/vfstest.c`): mismas tablas, mismos tres INSERT,
 * misma consulta. Eso lo convierte en una COMPARACIÓN y no en una prueba nueva:
 * la respuesta conocida —COUNT=2, AVG=22.00— ya está establecida en host, así
 * que si aquí sale otra cosa, la diferencia es la cintura (FatFs + SD) y no la
 * consulta.
 *
 * Y el CIERRE-REAPERTURA no es ceremonia: es lo único que separa «escribió» de
 * «persistió». Sin él, una BD que viva entera en la caché de páginas pasaría
 * por buena.
 */
/* ── PRIMERO: LEER UNA BD DE VERDAD, la que puso Eduardo en la tarjeta ────────
 *
 * 1,3 MB reales valen más que tres filas nuestras: obliga a leer muchas páginas
 * por SPI, a recorrer B-trees y a que el VFS sirva offsets por todo el fichero,
 * no sólo la primera página.
 *
 * Se abre en **SÓLO LECTURA** a propósito: es un dato del usuario y una prueba
 * no debe modificar lo que mide. Además así `abrir` exige que exista, y no se
 * crea un journal al lado.
 *
 * Y la pregunta es a `sqlite_master`, o sea a la BD por SU PROPIO esquema: la
 * prueba se describe sola sin que tengamos que saber de antemano qué tablas
 * tiene. Si contesta nombres legibles, es que el fichero se está leyendo bien.
 */
#if BPSQL_AUTOTEST
static int leer_bd_existente(char* buf, int nbuf)
{
    static const char* AJENA = "/sd/SmartMini.db";
    sqlite3*      db = 0;
    sqlite3_stmt* st = 0;

    if (!g_bios->existe(AJENA)) {
        g_bios->log("sqlite: no hay /sd/SmartMini.db — me salto la lectura de BD ajena");
        return 0;                       /* no está: no es un fallo nuestro */
    }

    int rc = sqlite3_open_v2(AJENA, &db, SQLITE_OPEN_READONLY, 0);
    if (rc != SQLITE_OK) {
        sqlite3_snprintf(nbuf, buf, "sqlite: open RO de la BD ajena FALLO rc=%d (%s)",
                         rc, db ? sqlite3_errmsg(db) : sqlite3_errstr(rc));
        g_bios->log(buf);
        if (db) sqlite3_close(db);
        return 21;
    }

    rc = sqlite3_prepare_v2(db,
        "SELECT type, name FROM sqlite_master WHERE type IN ('table','index')"
        " ORDER BY type, name", -1, &st, 0);
    if (rc != SQLITE_OK) {
        sqlite3_snprintf(nbuf, buf, "sqlite: leer el esquema FALLO (%s)", sqlite3_errmsg(db));
        g_bios->log(buf);
        sqlite3_close(db);
        return 22;
    }

    int n = 0, tablas = 0;
    while (sqlite3_step(st) == SQLITE_ROW) {
        const char* tipo = (const char*) sqlite3_column_text(st, 0);
        n++;
        if (tipo && tipo[0] == 't') tablas++;
        if (n <= 12) {                  /* cap: el log es un anillo, no lo ahogamos */
            sqlite3_snprintf(nbuf, buf, "sqlite:   %s %s",
                             tipo, sqlite3_column_text(st, 1));
            g_bios->log(buf);
        }
    }
    sqlite3_finalize(st);
    sqlite3_close(db);

    /* RESPUESTA CONOCIDA. El mismo arnés del PC leyó este fichero con ESTE
     * mismo `vfs_bp.c` y dio 25 objetos / 20 tablas. Así que esto no es "a ver
     * qué sale": es una comparación contra un oráculo. Si aquí sale otra cosa,
     * la diferencia está en la cintura (FatFs + SD), no en SQLite ni en el VFS. */
    sqlite3_snprintf(nbuf, buf,
                     "sqlite: SmartMini.db leida — %d objetos, %d tablas (el PC dijo 25 y 20)%s",
                     n, tablas, n > 12 ? " [mostrados 12]" : "");
    g_bios->log(buf);
    return (n > 0) ? 0 : 23;            /* cero objetos en 1,3 MB seria sospechoso */
}

static int bd_en_la_sd(char* buf, int nbuf)
{
    static const char* RUTA = "/sd/bpsql.db";
    sqlite3*      db = 0;
    sqlite3_stmt* st = 0;
    char*         emsg = 0;
    int           rc;

    /* Borrar antes: si no, la 2ª vuelta encontraría 6 filas y la «respuesta
     * conocida» dejaría de serlo. Un test que no es repetible miente a la
     * segunda. Que no existan no es error. */
    g_bios->borrar(RUTA);
    g_bios->borrar("/sd/bpsql.db-journal");

    rc = sqlite3_open(RUTA, &db);
    if (rc != SQLITE_OK || db == 0) {
        sqlite3_snprintf(nbuf, buf, "sqlite: open('%s') FALLO rc=%d (%s)",
                         RUTA, rc, db ? sqlite3_errmsg(db) : sqlite3_errstr(rc));
        g_bios->log(buf);
        if (db) sqlite3_close(db);
        return 13;
    }
    g_bios->log("sqlite: BD abierta en /sd por el vfs 'bp'");

    rc = sqlite3_exec(db,
        "CREATE TABLE medidas(id INTEGER PRIMARY KEY, sensor TEXT, valor REAL);"
        "INSERT INTO medidas(sensor,valor) VALUES('temp', 21.5);"
        "INSERT INTO medidas(sensor,valor) VALUES('hum',  48.0);"
        "INSERT INTO medidas(sensor,valor) VALUES('temp', 22.5);", 0, 0, &emsg);
    if (rc != SQLITE_OK) {
        sqlite3_snprintf(nbuf, buf, "sqlite: CREATE/INSERT FALLO rc=%d (%s)",
                         rc, emsg ? emsg : sqlite3_errstr(rc));
        g_bios->log(buf);
        if (emsg) sqlite3_free(emsg);
        sqlite3_close(db);
        return 14;
    }
    g_bios->log("sqlite: CREATE TABLE + 3 INSERT — escritos en la tarjeta");

    if (sqlite3_close(db) != SQLITE_OK)         return 15;
    db = 0;

    /* Y aquí está la prueba de verdad: se vuelve a abrir el MISMO fichero. */
    rc = sqlite3_open(RUTA, &db);
    if (rc != SQLITE_OK) { sqlite3_close(db); return 16; }
    g_bios->log("sqlite: REABIERTA — lo de antes tiene que seguir ahi");

    rc = sqlite3_prepare_v2(db,
        "SELECT COUNT(*), AVG(valor) FROM medidas WHERE sensor='temp'", -1, &st, 0);
    if (rc != SQLITE_OK) {
        sqlite3_snprintf(nbuf, buf, "sqlite: prepare FALLO (%s)", sqlite3_errmsg(db));
        g_bios->log(buf);
        sqlite3_close(db);
        return 17;
    }
    if (sqlite3_step(st) != SQLITE_ROW) {
        sqlite3_finalize(st); sqlite3_close(db); return 18;
    }

    int    n   = sqlite3_column_int(st, 0);
    double med = sqlite3_column_double(st, 1);
    /* `%g` y no `%f`: el shim no trae los helpers de coma flotante de printf,
     * pero `sqlite3_snprintf` sí sabe formatear — es suyo. */
    sqlite3_snprintf(nbuf, buf, "sqlite: COUNT=%d  AVG=%g   (esperado 2 y 22)", n, med);
    g_bios->log(buf);
    sqlite3_finalize(st);
    sqlite3_close(db);

    if (n != 2)                                 return 19;
    if (med < 21.99 || med > 22.01)             return 20;

    g_bios->log("sqlite: >>> LA BASE DE DATOS FUNCIONA EN LA TARJETA <<<");
    return 0;
}

/* ── La entrada ───────────────────────────────────────────────────────────────
 *
 * La llama el cargador con la tabla ya localizada. Devuelve 0 si todo cuadra;
 * distinto de 0 = qué comprobación falló, para que el fallo diga DÓNDE.
 */
#endif /* BPSQL_AUTOTEST */

int bp_pack_init(const bpvm_bios_t* bios)
{
    char buf[112];

    if (bios == 0)                              return 1;
    if (bios->magic   != BPVM_BIOS_MAGIC)       return 2;
    if (bios->version != BPVM_BIOS_VERSION)     return 3;
    if (bios->log     == 0)                     return 4;
    g_bios = bios;

    g_bios->log("sqlite: pack vivo");

    /* De la .rodata de SQLite, atravesando una relocalización: si el realojado
     * hubiera fallado esto imprimiría basura. El mensaje es su propia prueba. */
    const char* v = sqlite3_libversion();
    if (v == 0)                                 return 5;
    if (v[0] != '3' || v[1] != '.')             return 6;
    g_bios->log(v);

    /* La arena la da el firmware, que es quien sabe dónde cayó: preguntarla en
     * vez de calcularla es la misma regla del ancla. */
    size_t arena_bytes = 0;
    void*  arena = g_bios->arena(&arena_bytes);
    if (arena == 0 || arena_bytes == 0)         return 7;

    sqlite3_snprintf((int) sizeof buf, buf, "sqlite: arena %u KB en %p",
                     (unsigned) (arena_bytes / 1024u), arena);
    g_bios->log(buf);

    /* Antes que nada, su voz: si algo de lo que viene falla, queremos su
     * explicación y no sólo su número. */
    sqlite3_config(SQLITE_CONFIG_LOG, sql_log, (void*) 0);

    /* MEMSYS5 sobre ESA arena. 64 = alocación mínima, el valor con el que se
     * midió el 7-ago. */
    if (sqlite3_config(SQLITE_CONFIG_HEAP, arena, (int) arena_bytes, 64)
            != SQLITE_OK)                       return 8;

    /* ⚠️ ESTA LÍNEA NO ES OPCIONAL, y es fácil que parezca un detalle.
     *
     * El búfer del ordenador viene a 250 páginas (≈1 MB de UN bloque contiguo).
     * Con eso, una arena de 2 MB rinde IGUAL que una de 1 MB —se atasca a las
     * 5.000 filas— porque el buddy no puede dar ese bloque. Bajándolo a 64, las
     * mismas 2 MB llegan a 1.000.000 de filas. Medido el 7-ago.
     *
     * O sea: el mínimo de 2 MB que dice `bpvm_sqlmem.h` SÓLO vale con esto
     * puesto. Quitarlo no da error — da una BD que se queda corta y parece que
     * la culpa es del tamaño de la arena. */
    if (sqlite3_config(SQLITE_CONFIG_PMASZ, (unsigned) 64) != SQLITE_OK)
                                                return 9;

    /* Aquí dentro se llama a `sqlite3_os_init()`, que registra el VFS. */
    int rc = sqlite3_initialize();
    if (rc != SQLITE_OK) {
        sqlite3_snprintf((int) sizeof buf, buf,
                         "sqlite: initialize FALLO rc=%d (%s)", rc, sqlite3_errstr(rc));
        g_bios->log(buf);
        return 10;
    }
    g_bios->log("sqlite: initialize OK — motor arrancado y vfs 'bp' registrado");

    /* ── V5/H4: OFRECER LA API ────────────────────────────────────────────────
     *
     * Hasta esta línea el pack sólo consumía. Aquí publica su tabla en el punto
     * de encuentro de la BIOS, y a partir de ahora un `native` de `SQLite.bp`
     * puede pedirla con `busca('SQLI')`.
     *
     * Va DESPUÉS de `initialize` a propósito: publicar una API cuyo motor no ha
     * arrancado sería ofrecer algo que no funciona, y el que la recogiera se
     * encontraría el fallo tres llamadas más tarde y en otro sitio. */
    int rcpub = bpsql_publicar(g_bios);
    if (rcpub != 0)                             return 24;

    /* Y AQUI ACABA. El pack esta arrancado y su tabla publicada; lo que venga
     * despues lo pide quien la use.
     *
     * Antes seguian dos ejercicios (leer una BD que ya existe, crear otra en la
     * SD). Ahora viven bajo BPSQL_AUTOTEST, apagado — ver la nota de arriba. */
#if BPSQL_AUTOTEST
    /* El CONTROL de que la arena es memoria de verdad y no una direccion que
     * cuadra. Pedir, ESCRIBIR, releer y devolver: sin la escritura, esto solo
     * probaria que el alocador lleva bien las cuentas. */
    {
        unsigned char* p = (unsigned char*) sqlite3_malloc(64 * 1024);
        if (p == 0)                             return 11;
        p[0] = 0xA5; p[64 * 1024 - 1] = 0x5A;
        if (p[0] != 0xA5 || p[64 * 1024 - 1] != 0x5A) { sqlite3_free(p); return 12; }
        sqlite3_snprintf((int) sizeof buf, buf,
                         "sqlite: memoria viva - 64 KB pedidos, %u B en uso",
                         (unsigned) sqlite3_memory_used());
        g_bios->log(buf);
        sqlite3_free(p);
    }
    /* Dos ejercicios, y en este orden: primero LEER una BD real que ya existe
     * (sin tocarla), luego CREAR una nueva. Si el primero va y el segundo no,
     * el problema esta en la escritura y no en el camino de lectura. */
    {
        int r = leer_bd_existente(buf, (int) sizeof buf);
        if (r != 0) return r;
        return bd_en_la_sd(buf, (int) sizeof buf);
    }
#else
    (void) buf;
    return 0;
#endif
}
