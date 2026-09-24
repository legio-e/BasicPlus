/*
 * test_bios.c — V5/I: la tabla que se le presta al pack nativo.
 *
 * Lo que se verifica es el CHIVATO, no la tabla: que un hueco se detecte ANTES
 * de ejecutar nada y que diga QUÉ falta, no sólo que algo falta. Un NULL en una
 * ranura es un cuelgue esperando a ocurrir; el valor de esto es convertirlo en
 * "la BIOS tiene huecos: memcpy" en vez de un hard fault mudo.
 *
 *   make test-bios
 */
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"   /* V7/H6: las tablas de pack que la reja exige */
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* Implementaciones de mentira: sólo hacen falta las DIRECCIONES. */
static void  f_log(const char* m) { (void) m; }
static void* f_mem3(void* a, const void* b, size_t n) { (void)b;(void)n; return a; }
static void* f_memset(void* a, int c, size_t n) { (void)c;(void)n; return a; }
static int   f_cmp3(const void* a, const void* b, size_t n) { (void)a;(void)b;(void)n; return 0; }
static void* f_memchr(const void* a, int c, size_t n) { (void)c;(void)n; return (void*) a; }
static size_t f_strlen(const char* s) { (void) s; return 0; }
static int   f_strcmp(const char* a, const char* b) { (void)a;(void)b; return 0; }
static int   f_strncmp(const char* a, const char* b, size_t n) { (void)a;(void)b;(void)n; return 0; }
static char* f_strchr(const char* s, int c) { (void)c; return (char*) s; }
static size_t f_span(const char* s, const char* a) { (void)s;(void)a; return 0; }
static void* f_malloc(size_t n) { (void) n; return 0; }
static void  f_free(void* p) { (void) p; }
static void* f_realloc(void* p, size_t n) { (void)n; return p; }
static struct tm* f_localtime(const void* t) { (void) t; return 0; }
/* V5/H2 — ficheros. Postizas: aqui no se prueba QUE hacen, se prueba que el
 * verificador se entera si FALTAN. */
static int  f_abrir(const char* c, int w) { (void)c;(void)w; return 1; }
static int  f_fd1(int fd) { (void) fd; return 0; }
static long f_leer(int fd, uint32_t o, void* d, uint32_t n) { (void)fd;(void)o;(void)d;(void)n; return 0; }
static long f_escr(int fd, uint32_t o, const void* s, uint32_t n) { (void)fd;(void)o;(void)s;(void)n; return 0; }
static int  f_trunc(int fd, uint32_t t) { (void)fd;(void)t; return 0; }
static long f_tam(int fd) { (void) fd; return 0; }
static int  f_camino(const char* c) { (void) c; return 0; }
/* V5/H3 — la arena. Devolver NULL es una respuesta LEGÍTIMA (no hay BD), así
 * que el doble puede hacerlo: lo que el verificador exige es que la ranura
 * exista, no que haya arena. */
static void* f_arena(size_t* b) { if (b) *b = 0; return NULL; }
/* V5/H4 — el punto de encuentro. Igual que la arena: que `busca` no encuentre
 * nada es una respuesta legítima; lo que se exige es que la ranura esté. */
static int         f_publica(uint32_t m, const void* t) { (void)m;(void)t; return 0; }
static const void* f_busca  (uint32_t m) { (void) m; return NULL; }
/* V7/H6 — la HAL. Postizas también: se prueba que el verificador las exige. */
static int      f_gpio_init(int p, int m) { (void)p;(void)m; return 0; }
static void     f_gpio_write(int p, int v) { (void)p;(void)v; }
static int      f_gpio_read(int p) { (void)p; return 0; }
static int      f_spi_init(int b, int a, int c, int d, int e, int m) { (void)b;(void)a;(void)c;(void)d;(void)e;(void)m; return 0; }
static int      f_spi_write(int b, const uint8_t* d, size_t n) { (void)b;(void)d; return (int) n; }
static int      f_i2c_init(int b, int a, int c, int d) { (void)b;(void)a;(void)c;(void)d; return 0; }
static int      f_i2c_write(int b, int a, const uint8_t* d, size_t n) { (void)b;(void)a;(void)d; return (int) n; }
static int      f_i2c_read(int b, int a, uint8_t* d, size_t n) { (void)b;(void)a;(void)d; return (int) n; }
static uint32_t f_now_ms(void) { return 0; }
static void     f_sleep_ms(int ms) { (void) ms; }
/* V7/H6.c G1 — la config de placa. «No está» es una respuesta legítima. */
static int      f_env_get(const char* k, char* b, size_t c) { (void)k; if (b && c) b[0] = 0; return -1; }
/* V7/H6.c G4.a2 — el turno de los buses (v7). */
static int      f_spi_tomar(int b, int cs, int hz, int m) { (void)b;(void)cs;(void)hz;(void)m; return 0; }
static int      f_spi_cs(int b, int v) { (void)b;(void)v; return 0; }
static int      f_spi_transferir(int b, const uint8_t* t, uint8_t* r, size_t n) { (void)b;(void)t;(void)r; return (int) n; }
static void     f_spi_soltar(int b) { (void)b; }
static int      f_i2c_escribir_leer(int b, int a, const uint8_t* t, size_t nt, uint8_t* r, size_t nr) { (void)b;(void)a;(void)t;(void)nt;(void)r; return (int) nr; }
/* V7/H6.c G4.a3 — la transacción (v8). */
static int      f_spi_transaccion(int b, int cs, int hz, int m, const bpvm_spi_op_t* o, int n) { (void)b;(void)cs;(void)hz;(void)m;(void)o; return n; }
static int      f_i2c_transaccion(int b, int a, const bpvm_i2c_msg_t* m, int n) { (void)b;(void)a;(void)m; return n; }

/* Devuelve una tabla COMPLETA y bien formada. Cada caso la estropea de una
 * manera — un solo cambio por caso, que es como se sabe qué causó qué. */
static bpvm_bios_t buena(void) {
    bpvm_bios_t b;
    memset(&b, 0, sizeof b);
    b.magic = BPVM_BIOS_MAGIC;  b.version = BPVM_BIOS_VERSION;
    b.log = f_log;
    b.memcpy = f_mem3; b.memmove = f_mem3; b.memset = f_memset;
    b.memcmp = f_cmp3; b.memchr = f_memchr;
    b.strlen = f_strlen; b.strcmp = f_strcmp; b.strncmp = f_strncmp;
    b.strchr = f_strchr; b.strrchr = f_strchr;
    b.strspn = f_span;   b.strcspn = f_span;
    b.malloc = f_malloc; b.free = f_free; b.realloc = f_realloc;
    b.localtime = f_localtime;
    b.abrir = f_abrir; b.cerrar = f_fd1;
    b.leer = f_leer;   b.escribir = f_escr;
    b.truncar = f_trunc; b.tamano = f_tam; b.sincronizar = f_fd1;
    b.borrar = f_camino; b.existe = f_camino;
    b.arena = f_arena;
    b.publica = f_publica; b.busca = f_busca;
    b.gpio_init = f_gpio_init; b.gpio_write = f_gpio_write; b.gpio_read = f_gpio_read;
    b.spi_init = f_spi_init; b.spi_write = f_spi_write;
    b.i2c_init = f_i2c_init; b.i2c_write = f_i2c_write; b.i2c_read = f_i2c_read;
    b.now_ms = f_now_ms; b.sleep_ms = f_sleep_ms;
    b.env_get = f_env_get;
    b.spi_tomar = f_spi_tomar; b.spi_cs = f_spi_cs; b.spi_transferir = f_spi_transferir;
    b.spi_soltar = f_spi_soltar; b.i2c_escribir_leer = f_i2c_escribir_leer;
    b.spi_transaccion = f_spi_transaccion; b.i2c_transaccion = f_i2c_transaccion;
    return b;
}

/* V7/H6 — tablas de pack DE VERDAD para el registro: desde la reja, `publica`
 * lee lo que le pasan, así que un `int` ya no vale como tabla. */
static void sym_a(void) {}
static const bpvm_pack_sym_t SYMS_A[] = { { "abrir", sym_a }, { "cerrar", sym_a } };
static const bpvm_pack_api_t TABLA_A = { 0x53514C49u, 1u, 2u, SYMS_A };            /* 'SQLI' */
static const bpvm_pack_api_t TABLA_B = { 0x4C56474Cu, 1u, 2u, SYMS_A };            /* 'LVGL' */
static const bpvm_pack_api_t TABLA_OTRA_MARCA = { 0x50414E54u, 1u, 2u, SYMS_A };   /* dice 'PANT' */
static const bpvm_pack_sym_t SYMS_ROTA[] = { { "abrir", sym_a }, { "cerrar", 0 } };
static const bpvm_pack_api_t TABLA_ROTA = { 0x53514C49u, 1u, 2u, SYMS_ROTA };      /* un símbolo sin función */
static const bpvm_pack_api_t TABLA_VACIA = { 0x53514C49u, 1u, 0u, SYMS_A };        /* no publica nada */

int main(void) {
    printf("== tabla BIOS del pack nativo (V5/I) ==\n");

    /* ── 1. Completa ── */
    {
        bpvm_bios_t b = buena();
        CHECK(bpvm_bios_verify(&b) == NULL, "tabla completa -> NULL (sin quejas)");
    }

    /* ── 2. Un hueco se detecta Y SE NOMBRA ──
     * Lo que importa no es que falle: es que diga CUÁL. "algo falta" obliga a
     * mirar 17 campos a mano; "memcpy" te lleva al sitio. */
    {
        bpvm_bios_t b = buena(); b.memcpy = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "memcpy") == 0, "hueco en memcpy -> lo NOMBRA");
    }
    {
        bpvm_bios_t b = buena(); b.realloc = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "realloc") == 0, "hueco en realloc -> lo NOMBRA");
    }
    {
        /* La ÚLTIMA de la lista, que es donde el bucle podría quedarse corto.
         * Cuando la tabla crece hay que MOVER este caso a la nueva última: si
         * se queda en la de antes deja de probar el borde y nadie se entera —
         * seguiría en verde comprobando una ranura del medio. */
        /* (Hasta G1 este caso seguía mirando `busca`, y desde la v5 la última
         * era `sleep_ms`: pasó justo lo que este comentario avisaba. En G1 pasó a
         * `env_get`, en la v7 (G4.a2) a `i2c_escribir_leer`, y desde la v8
         * (G4.a3) la última es `i2c_transaccion`; las de antes se quedan como
         * casos del medio.) */
        bpvm_bios_t b = buena(); b.i2c_transaccion = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "i2c_transaccion") == 0, "hueco en la ULTIMA ranura (i2c_transaccion) -> tambien");
    }
    {
        bpvm_bios_t b = buena(); b.i2c_escribir_leer = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "i2c_escribir_leer") == 0, "hueco en i2c_escribir_leer (la ultima de la v7) -> tambien");
    }
    {
        bpvm_bios_t b = buena(); b.env_get = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "env_get") == 0, "hueco en env_get (la ultima de la v6) -> lo NOMBRA");
    }
    {
        bpvm_bios_t b = buena(); b.busca = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "busca") == 0, "hueco en busca -> lo NOMBRA");
    }
    {
        bpvm_bios_t b = buena(); b.arena = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "arena") == 0, "hueco en arena -> lo NOMBRA");
    }

    /* ── V5/H4: el REGISTRO del punto de encuentro ──
     * Es lo que hace que añadir un pack no toque la VM, así que su contrato se
     * prueba entero: guardar, recuperar, y NEGARSE en los casos que son un
     * problema disfrazado de éxito. */
    {
        bpvm_bios_packs_reset();

        CHECK(bpvm_bios_busca(0x53514C49u) == NULL,
              "buscar una marca que nadie publico -> NULL (no basura)");
        CHECK(bpvm_bios_publica(0x53514C49u, &TABLA_A) == 0, "publicar 'SQLI' -> 0");
        CHECK(bpvm_bios_busca(0x53514C49u) == &TABLA_A,
              "buscar 'SQLI' -> LA MISMA tabla que se publico");
        CHECK(bpvm_bios_busca(0x4C56474Cu) == NULL,
              "otra marca sigue sin estar (no devuelve la primera que haya)");

        /* Republicar es un pack cargado dos veces, o dos packs peleandose por
         * la marca. Sobrescribir en silencio dejaria al primero con clientes
         * apuntando a una tabla que ya no es la suya. */
        CHECK(bpvm_bios_publica(0x53514C49u, &TABLA_B) < 0,
              "republicar la MISMA marca -> se NIEGA (no la pisa)");
        CHECK(bpvm_bios_busca(0x53514C49u) == &TABLA_A,
              "y tras negarse, la tabla original sigue en su sitio");

        CHECK(bpvm_bios_publica(0, &TABLA_B) < 0,        "marca 0 -> se niega");
        CHECK(bpvm_bios_publica(0x4C56474Cu, NULL) < 0,  "tabla NULL -> se niega");

        /* V7/H6 — LA REJA: la tabla tiene que SER lo que dice. Hasta hoy
         * `publica` guardaba cualquier puntero; el que la recogiera saltaba a
         * donde no debe. Ahora se lee, y el desajuste tiene código (-4). */
        CHECK(bpvm_bios_publica(0x4C56474Cu, &TABLA_OTRA_MARCA) == -4,
              "REJA: publicar bajo 'LVGL' una tabla que dice 'PANT' -> -4");
        CHECK(bpvm_bios_busca(0x4C56474Cu) == NULL,
              "y no se guarda nada bajo esa marca");
        CHECK(bpvm_bios_publica(0x50414E54u, &TABLA_ROTA) == -4,
              "REJA: un simbolo sin funcion -> -4 (el hueco se ve AQUI, no al saltar)");
        CHECK(bpvm_bios_publica(0x50414E54u, &TABLA_VACIA) == -4,
              "REJA: una tabla que no publica nada -> -4");

        /* Llenar el registro y comprobar que el desbordamiento se DICE. Un
         * pack ignorado en silencio seria un fallo imposible de leer. Ocho
         * marcas desde V7/H6 (antes cuatro). */
        CHECK(bpvm_bios_publica(0x4C56474Cu, &TABLA_B) == 0, "cabe una segunda");
        {
            static bpvm_pack_api_t mas[6];
            int ok = 1;
            for (int k = 0; k < 6; k++) {
                mas[k] = TABLA_B; mas[k].marca = 0x41414100u + (uint32_t) k;   /* 'AAA0'.. */
                if (bpvm_bios_publica(mas[k].marca, &mas[k]) != 0) ok = 0;
            }
            CHECK(ok, "caben OCHO en total (BPVM_BIOS_PACK_MAX)");
            static bpvm_pack_api_t novena; novena = TABLA_B; novena.marca = 0x58585858u;
            CHECK(bpvm_bios_publica(novena.marca, &novena) == -2,
                  "la NOVENA no cabe -> -2 (no la descarta callando)");
        }

        bpvm_bios_packs_reset();
        CHECK(bpvm_bios_busca(0x53514C49u) == NULL, "reset -> el registro se vacia");
    }

    /* ── V7/H6.c G1: LA ARENA, CON DUEÑO ──
     * Con varios motores en la zona, dos con la MISMA arena se machacarían el
     * heap en silencio. Lo que se prueba es que el segundo recibe un NO CON
     * NOMBRE, y que fuera de una entrada nadie se la lleva. */
    {
        static unsigned char bloque[4096];
        size_t n = 123;

        bpvm_bios_arena_fija(bloque, sizeof bloque);
        CHECK(bpvm_bios_arena(&n) == NULL && n == 0,
              "arena pedida FUERA de una entrada -> NULL (no se sabe quien pregunta)");
        CHECK(bpvm_bios_arena_negada() != NULL, "y queda apuntado por que");

        bpvm_bios_arena_entra("SQLite");
        CHECK(bpvm_bios_arena(&n) == bloque && n == sizeof bloque,
              "el primero que la pide desde su entrada -> se la lleva entera");
        CHECK(bpvm_bios_arena(&n) == bloque, "pedirla otra vez el MISMO -> la misma (idempotente)");
        CHECK(bpvm_bios_arena_negada() == NULL, "y a el no se le nego nada");
        bpvm_bios_arena_sale();
        CHECK(strcmp(bpvm_bios_arena_duena(), "SQLite") == 0, "la duena queda anotada: SQLite");

        bpvm_bios_arena_entra("Otro");
        CHECK(bpvm_bios_arena(&n) == NULL && n == 0, "un SEGUNDO motor la pide -> NULL");
        CHECK(bpvm_bios_arena_negada() != NULL
              && strstr(bpvm_bios_arena_negada(), "SQLite") != NULL,
              "y el porque NOMBRA a la duena ('la arena ya es de SQLite')");
        bpvm_bios_arena_sale();

        bpvm_bios_arena_fija(NULL, 0);
        bpvm_bios_arena_entra("SQLite");
        CHECK(bpvm_bios_arena(&n) == NULL && bpvm_bios_arena_negada() != NULL,
              "sin bloque -> NULL con motivo (no se pidio, o los estaticos se lo comieron)");
        bpvm_bios_arena_sale();

        bpvm_bios_arena_fija(bloque, sizeof bloque);
        CHECK(bpvm_bios_arena_duena()[0] == 0, "fijar el bloque de nuevo olvida a la duena");
        {   /* El nombre se COPIA: el del plan vive en la pila del cargador. */
            char tmp[16]; strcpy(tmp, "Gui");
            bpvm_bios_arena_entra(tmp);
            (void) bpvm_bios_arena(&n);
            bpvm_bios_arena_sale();
            strcpy(tmp, "XXX");
            CHECK(strcmp(bpvm_bios_arena_duena(), "Gui") == 0,
                  "el nombre de la duena es una COPIA (el del que llama puede morir)");
        }
        bpvm_bios_arena_fija(NULL, 0);
    }

    /* ── 3. `log` es la voz del pack: si falta, se dice ANTES que nada ──
     * Sin log el pack es mudo, así que ese hueco es cualitativamente peor: no
     * sólo rompe una función, deja ciego todo lo demás. */
    {
        bpvm_bios_t b = buena(); b.log = NULL; b.memcpy = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "log") == 0,
              "faltan log Y memcpy -> se reporta LOG (sin voz no hay diagnostico)");
    }

    /* ── 4. Magic y version ANTES que las ranuras ──
     * Si la tabla no es lo que creemos, leer punteros de ella es leer basura:
     * la comprobacion barata va primero. */
    {
        bpvm_bios_t b = buena(); b.magic = 0xDEADBEEF;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "magic") == 0, "magic malo -> 'magic', no se leen punteros");
    }
    {
        bpvm_bios_t b = buena(); b.version = BPVM_BIOS_VERSION + 1; b.log = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "version") == 0,
              "version distinta -> 'version' AUNQUE tambien falte log");
    }

    /* ── 5. Tabla NULL: no debe explotar ── */
    {
        const char* r = bpvm_bios_verify(NULL);
        CHECK(r && strcmp(r, "tabla") == 0, "tabla NULL -> 'tabla', sin reventar");
    }

    /* ── 6. Tabla a ceros (el caso del que se olvida de rellenarla) ── */
    {
        bpvm_bios_t b; memset(&b, 0, sizeof b);
        CHECK(bpvm_bios_verify(&b) != NULL, "tabla a CEROS -> se queja (no pasa por buena)");
    }

    /* ── 7. El contador de ranuras cuadra con la struct ──
     * Si alguien añade un campo y olvida ponerlo en RANURAS, el hueco vuelve a
     * ser mudo. Esto no lo detecta del todo, pero deja el numero a la vista. */
    CHECK(bpvm_bios_slot_count() == 47,
          "47 ranuras (17 prueba A + 9 ficheros H2 + arena H3 + 2 del encuentro H4 + 10 de la HAL H6 + env_get G1 + 5 del turno G4.a2 + 2 de la transaccion G4.a3)");
    CHECK(offsetof(bpvm_bios_t, env_get) > offsetof(bpvm_bios_t, sleep_ms),
          "env_get va DETRAS de la HAL: un pack de la v5 sigue encontrando sus ranuras");
    CHECK(offsetof(bpvm_bios_t, spi_tomar) > offsetof(bpvm_bios_t, env_get),
          "el turno (v7) va DETRAS de env_get: un pack de la v6 sigue encontrando sus ranuras");
    CHECK(offsetof(bpvm_bios_t, spi_transaccion) > offsetof(bpvm_bios_t, i2c_escribir_leer),
          "la transaccion (v8) va DETRAS del turno: un pack de la v7 sigue encontrando sus ranuras");
    {   /* V7/H6 — un hueco en la HAL se NOMBRA como cualquier otro, y la HAL
         * va DETRAS de todo lo anterior (la tabla crece por el final). */
        bpvm_bios_t b = buena(); b.now_ms = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "now_ms") == 0, "hueco en now_ms -> lo NOMBRA");
        CHECK(offsetof(bpvm_bios_t, gpio_init) > offsetof(bpvm_bios_t, busca),
              "la HAL va DETRAS de `busca`: un pack de la v4 sigue encontrando sus ranuras");
    }

    /* ─────────────── EL ANCLA (idea de Eduardo, 7-ago) ───────────────
     *
     * El pack no puede saber la direccion de la tabla: cada enlace la mueve.
     * Asi que se pone una marca en la imagen con los punteros detras, y se
     * BUSCA. Lo que se prueba aqui no es que encuentre una marca buena — eso es
     * lo facil — sino que NO se trague las malas: ocho bytes pueden repetirse
     * por casualidad en un megabyte de codigo, y creerse el primero que aparece
     * seria leer punteros de basura y saltar a cualquier parte. */
    {
        /* Una "imagen" de mentira, con basura alrededor para que la busqueda
         * tenga que trabajar. Alineada a 4 como la de verdad. */
        static bpvm_bios_t s_tabla_falsa;
        union { unsigned char b[256]; uint32_t alinea; } img;
        for (int i = 0; i < 256; i++) img.b[i] = (unsigned char) (i * 7 + 3);

        bpvm_ancla_t a;
        memset(&a, 0, sizeof a);
        memcpy(a.magia, "BPANCLA1", 8);
        a.version = BPVM_ANCLA_VERSION;
        a.bytes   = (uint16_t) sizeof(bpvm_ancla_t);
        a.bios    = &s_tabla_falsa;
        a.prueba  = (uint16_t (*)(uint16_t, const unsigned char*, uint32_t)) (void*) &s_tabla_falsa;
        a.cargar_pack = (int32_t (*)(void)) (void*) &s_tabla_falsa;

        const unsigned OFF = 64;      /* 64 es multiplo de 4 */
        memcpy(img.b + OFF, &a, sizeof a);

        CHECK(bpvm_ancla_buscar(img.b, sizeof img.b)
              == (const bpvm_ancla_t*) (const void*) (img.b + OFF),
              "el ancla se encuentra entre la basura, y en su sitio exacto");

        CHECK(bpvm_ancla_buscar(0, 1024) == 0, "base NULL -> NULL (sin reventar)");
        CHECK(bpvm_ancla_buscar(img.b, 4) == 0, "zona mas corta que el ancla -> NULL");
        CHECK(bpvm_ancla_buscar(img.b, OFF) == 0,
              "si la zona ACABA antes del ancla, no se lee de mas");

        /* Los tres casos que de verdad importan: la marca esta, pero lo de
         * detras NO es un ancla. Fiarse solo de los 8 bytes seria el fallo. */
        { bpvm_ancla_t m = a; m.version = 99; memcpy(img.b + OFF, &m, sizeof m);
          CHECK(bpvm_ancla_buscar(img.b, sizeof img.b) == 0,
                "marca OK pero VERSION de otra epoca -> se rechaza"); }
        { bpvm_ancla_t m = a; m.bytes = 4; memcpy(img.b + OFF, &m, sizeof m);
          CHECK(bpvm_ancla_buscar(img.b, sizeof img.b) == 0,
                "marca OK pero TAMANO que no cuadra -> se rechaza"); }
        { bpvm_ancla_t m = a; m.bios = 0; memcpy(img.b + OFF, &m, sizeof m);
          CHECK(bpvm_ancla_buscar(img.b, sizeof img.b) == 0,
                "marca OK pero sin tabla detras -> se rechaza"); }
        { bpvm_ancla_t m = a; m.prueba = 0; memcpy(img.b + OFF, &m, sizeof m);
          CHECK(bpvm_ancla_buscar(img.b, sizeof img.b) == 0,
                "marca OK pero sin el control detras -> se rechaza"); }
        { bpvm_ancla_t m = a; m.cargar_pack = 0; memcpy(img.b + OFF, &m, sizeof m);
          CHECK(bpvm_ancla_buscar(img.b, sizeof img.b) == 0,
                "marca OK pero sin el cargador (v2) detras -> se rechaza"); }

        /* Y la marca SUELTA, sin nada detras: el caso de la copia del literal
         * que dejaria el compilador si se escribiera "BPANCLA1" a la ligera. */
        for (int i = 0; i < 256; i++) img.b[i] = (unsigned char) (i * 7 + 3);
        memcpy(img.b + OFF, "BPANCLA1", 8);
        CHECK(bpvm_ancla_buscar(img.b, sizeof img.b) == 0,
              "la marca SOLA, sin ancla detras -> no cuela");
    }

    printf("\n[status=%s]\n", g_fail == 0 ? "OK" : "FAIL");
    return g_fail != 0;
}
