/* bpvm_bios.c — verificación de la tabla que se le presta al pack nativo.
 *
 * Sin dependencias más allá de <stddef.h>: host-testable sin arrastrar la VM,
 * igual que bpvm_env/bpvm_part/bpvm_boot/bpvm_sqlmem.
 */
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"   /* V7/H6: la reja de `publica` (static inline, sin .c) */

/* La lista de ranuras vive en UN solo sitio: este array. Se recorre con offsets
 * en vez de escribir 17 `if (b->x == NULL)`, porque esa forma se desincroniza
 * el día que alguien añade un campo a la struct y olvida su comprobación — y
 * entonces el hueco vuelve a ser mudo, que es justo lo que esto evita.
 *
 * `log` va la PRIMERA también aquí: si falta, el mensaje debe salir antes que
 * cualquier otro, porque sin ella el pack no puede contar nada de lo demás. */
typedef struct { size_t off; const char* name; } ranura_t;

#define R(campo) { offsetof(bpvm_bios_t, campo), #campo }

static const ranura_t RANURAS[] = {
    R(log),                                    /* la voz del pack, primero */
    R(memcpy), R(memmove), R(memset), R(memcmp), R(memchr),
    R(strlen), R(strcmp), R(strncmp), R(strchr), R(strrchr),
    R(strspn), R(strcspn),
    R(malloc), R(free), R(realloc),
    R(localtime),
    /* V5/H2 — ficheros. Van AQUI y no solo en la struct: esta lista es la
     * unica fuente de verdad del verificador, y un campo sin su R() vuelve a
     * ser un hueco mudo, que es justo lo que este array evita. */
    R(abrir), R(cerrar), R(leer), R(escribir), R(truncar),
    R(tamano), R(sincronizar), R(borrar), R(existe),
    /* V5/H3 — la arena de la BD. Se exige NO NULA como cualquier otra: la
     * FUNCION tiene que estar siempre, aunque devuelva NULL porque no haya
     * arena. Son cosas distintas — "el firmware no sabe darte arena" es un
     * hueco en la tabla; "hoy no hay arena" es una respuesta legitima. */
    R(arena),
    /* V5/H4 — el punto de encuentro. Mismo criterio: las FUNCIONES tienen que
     * estar siempre, aunque `busca` conteste NULL porque nadie ha publicado
     * nada. "No sé guardar tablas de packs" y "hoy no hay ninguna" son cosas
     * distintas y sólo la primera es un hueco. */
    R(publica), R(busca),
    /* V7/H6 — la HAL. Igual que las de ficheros: la FUNCIÓN tiene que estar
     * siempre, aunque `gpio_init` conteste -1 porque el pin no exista. */
    R(gpio_init), R(gpio_write), R(gpio_read),
    R(spi_init), R(spi_write),
    R(i2c_init), R(i2c_write), R(i2c_read),
    R(now_ms), R(sleep_ms),
    /* V7/H6.c G1 — la config de placa. La FUNCIÓN tiene que estar siempre,
     * aunque conteste «no está» porque el ENV no tenga esa clave. */
    R(env_get),
    /* V7/H6.c G4.a2 — el turno (v7). Las FUNCIONES siempre, aunque `spi_tomar`
     * conteste -1 porque el bus esté ocupado más del plazo. */
    R(spi_tomar), R(spi_cs), R(spi_transferir), R(spi_soltar),
    R(i2c_escribir_leer),
    R(spi_transaccion), R(i2c_transaccion),
};

#define N_RANURAS ((int)(sizeof(RANURAS) / sizeof(RANURAS[0])))

int bpvm_bios_slot_count(void) { return N_RANURAS; }

const char* bpvm_bios_verify(const bpvm_bios_t* b)
{
    if (b == NULL)                          return "tabla";
    /* Magic y versión ANTES que las ranuras: si la tabla no es lo que creemos,
     * leer punteros de ella ya es leer basura. */
    if (b->magic   != BPVM_BIOS_MAGIC)      return "magic";
    if (b->version != BPVM_BIOS_VERSION)    return "version";

    for (int i = 0; i < N_RANURAS; i++) {
        /* Cada campo es un puntero a función; se lee genéricamente por su
         * offset. `void*` sobre puntero-a-función es formalmente feo en C99
         * pero universal en todo objetivo que soportamos, y la alternativa
         * (17 comprobaciones a mano) es la que se desincroniza. */
        const void* const* p =
            (const void* const*) ((const char*) b + RANURAS[i].off);
        if (*p == NULL) return RANURAS[i].name;
    }
    return NULL;                            /* completa */
}

/* ── V5/H4 — EL REGISTRO DE TABLAS DE PACK ────────────────────────────────────
 *
 * Cuatro entradas. No es una limitación disfrazada de decisión: los packs de
 * código nativo los construimos NOSOTROS y son contados (SQLite, y algún día
 * LVGL o los drivers de pantalla). Una lista fija se ve entera de un vistazo,
 * no aloca, y sobre todo no puede fallar por falta de memoria en el arranque —
 * que es cuando menos ganas hay de diagnosticar nada.
 *
 * Y si algún día se queda corta, `publica` lo DICE (devuelve -2) en vez de
 * ignorar la cuarta tabla en silencio.
 *
 * Portable a propósito: vive aquí, con el verificador, y no en la cintura de
 * cada familia. Guardar un puntero bajo una marca no tiene nada de específico
 * de un micro, y tenerlo cinco veces sería cinco sitios donde divergir.
 */
/* V7/H6: de cuatro a OCHO (`BPVM_BIOS_PACK_MAX`, en la cabecera para que el
 * que publique sepa el tope). Hoy hay una marca ('SQLI') y mañana tres
 * ('LVGL', 'PANT'); ocho es el doble de lo previsto y sigue cabiendo en un
 * vistazo. Medido antes de subirlo, como pedía §1.3 de V7_IDEAS. */
#define BPVM_PACK_MAX BPVM_BIOS_PACK_MAX

static struct { uint32_t marca; const void* tabla; } s_packs[BPVM_PACK_MAX];

int bpvm_bios_publica(uint32_t marca, const void* tabla)
{
    if (marca == 0 || tabla == 0) return -1;

    /* V7/H6 — LA REJA. Hasta aquí se guardaba cualquier puntero y la
     * convención «la tabla empieza por su marca y su versión» no la comprobaba
     * nadie: un pack que publicara otra cosa mandaba al que la recogiera a
     * saltar a donde no debe. Ahora se lee: la marca tiene que ser ÉSTA, y
     * cada símbolo tener nombre y función. La versión no se juzga aquí (el
     * que consume la tabla es quien sabe cuál espera: `bpvm_pack_api_verify`). */
    {
        const bpvm_pack_api_t* a = (const bpvm_pack_api_t*) tabla;
        if (bpvm_pack_api_verify(a, marca, a->version) != 0) return -4;
    }

    for (int i = 0; i < BPVM_PACK_MAX; i++) {
        /* Ya publicada. NO se sobrescribe: eso sería el mismo pack cargado dos
         * veces, o dos packs peleándose por la misma marca, y las dos cosas son
         * un problema que hay que ver — no una actualización que aceptar. */
        if (s_packs[i].marca == marca) return -3;
    }
    for (int i = 0; i < BPVM_PACK_MAX; i++) {
        if (s_packs[i].marca == 0) {
            s_packs[i].tabla = tabla;
            s_packs[i].marca = marca;   /* la marca la ÚLTIMA: hasta que se
                                         * escribe, `busca` no ve la entrada a
                                         * medio hacer */
            return 0;
        }
    }
    return -2;                          /* sin hueco */
}

const void* bpvm_bios_busca(uint32_t marca)
{
    if (marca == 0) return 0;
    for (int i = 0; i < BPVM_PACK_MAX; i++) {
        if (s_packs[i].marca == marca) return s_packs[i].tabla;
    }
    return 0;                           /* nadie la publicó: el que llama tiene
                                         * que saber decirlo con nombre */
}

void bpvm_bios_packs_reset(void)
{
    for (int i = 0; i < BPVM_PACK_MAX; i++) { s_packs[i].marca = 0; s_packs[i].tabla = 0; }
}

/* ── V7/H6.c G1 — LA ARENA, CON DUEÑO ─────────────────────────────────────────
 *
 * Portable por lo mismo que el registro: la regla «es del primero que la pide»
 * no tiene nada de ningún micro, y con una copia por familia bastaría con que
 * una se equivocara para que dos motores compartieran heap sin enterarse.
 *
 * Sin voz propia (este fichero no arrastra la VM): lo que se niega se APUNTA, y
 * el cargador —que sí tiene log— lo escribe al volver del pack.
 */
#define ARENA_NOMBRE_MAX 33                 /* BPVM_PACK_NAME_LEN + NUL */

static uint8_t*    s_arena_base;
static size_t      s_arena_bytes;
static char        s_arena_duena[ARENA_NOMBRE_MAX];   /* "" = libre      */
static char        s_arena_quien[ARENA_NOMBRE_MAX];   /* el que está en su entrada */
static int         s_arena_en_entrada;
static const char* s_arena_no;                        /* el último «no»  */
static char        s_arena_no_buf[96];

static void copia_nombre(char* dst, const char* src) {
    int i = 0;
    if (src) while (i < ARENA_NOMBRE_MAX - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}
static int mismo_nombre(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

void bpvm_bios_arena_fija(void* base, size_t bytes)
{
    s_arena_base  = (uint8_t*) base;
    s_arena_bytes = base ? bytes : 0;
    s_arena_duena[0] = 0;
    s_arena_en_entrada = 0;
    s_arena_no = 0;
}

void bpvm_bios_arena_entra(const char* pack)
{
    copia_nombre(s_arena_quien, pack ? pack : "?");
    s_arena_en_entrada = 1;
    s_arena_no = 0;                         /* lo que se apunta es de ESTE pack */
}

void bpvm_bios_arena_sale(void) { s_arena_en_entrada = 0; }

const char* bpvm_bios_arena_negada(void) { return s_arena_no; }
const char* bpvm_bios_arena_duena(void)  { return s_arena_duena; }

void* bpvm_bios_arena(size_t* bytes)
{
    if (bytes) *bytes = 0;
    if (!s_arena_en_entrada) {
        s_arena_no = "la arena se pide desde bp_pack_init (fuera de una entrada no se sabe quien pregunta)";
        return 0;
    }
    if (s_arena_base == 0 || s_arena_bytes == 0) {
        s_arena_no = "no hay arena: ni packRam ni SQLite en el ENV, o los estaticos de los motores se comieron el bloque";
        return 0;
    }
    if (s_arena_duena[0] != 0 && !mismo_nombre(s_arena_duena, s_arena_quien)) {
        /* El texto se compone aquí y no con snprintf: este fichero no arrastra
         * stdio (se enlaza tal cual en el test y en las cinco imágenes). */
        static const char pre[] = "la arena ya es de '";
        int k = 0;
        for (int i = 0; pre[i] && k < (int) sizeof s_arena_no_buf - 2; i++) s_arena_no_buf[k++] = pre[i];
        for (int i = 0; s_arena_duena[i] && k < (int) sizeof s_arena_no_buf - 2; i++) s_arena_no_buf[k++] = s_arena_duena[i];
        s_arena_no_buf[k++] = '\'';
        s_arena_no_buf[k] = 0;
        s_arena_no = s_arena_no_buf;
        return 0;
    }
    copia_nombre(s_arena_duena, s_arena_quien);   /* el primero que la pide */
    if (bytes) *bytes = s_arena_bytes;
    return s_arena_base;
}

/*
 * Busca el ancla. Portable y sin dependencias: el mismo código lo usa el
 * firmware para verificarse al arrancar y lo replica el pack para encontrar la
 * tabla. Ver la explicación en bpvm_bios.h.
 */
const bpvm_ancla_t* bpvm_ancla_buscar(const void* base, uint32_t bytes)
{
    if (base == 0 || bytes < sizeof(bpvm_ancla_t)) return 0;

    const unsigned char* p = (const unsigned char*) base;
    /* Alinear el arranque: el ancla está a 4, empezar a 1 no la encontraría. */
    while (((uintptr_t) p & 3u) != 0u) { p++; if (bytes == 0) return 0; bytes--; }
    if (bytes < sizeof(bpvm_ancla_t)) return 0;

    const unsigned char* fin = p + (bytes - sizeof(bpvm_ancla_t));
    for (; p <= fin; p += 4) {
        /* Los 4 primeros bytes descartan casi todo sin tocar nada más. */
        if (p[0] != 'B' || p[1] != 'P' || p[2] != 'A' || p[3] != 'N') continue;
        if (p[4] != 'C' || p[5] != 'L' || p[6] != 'A' || p[7] != '1') continue;

        /* ⚠️ La marca SOLA no basta. Ocho bytes pueden repetirse por casualidad
         * en un megabyte de código, y creerse el primero que aparece es
         * exactamente el fallo que esto viene a evitar: leeríamos punteros de
         * basura y saltaríamos a cualquier parte. Los cuatro campos a la vez no
         * coinciden por azar. */
        const bpvm_ancla_t* a = (const bpvm_ancla_t*) (const void*) p;
        if (a->version != BPVM_ANCLA_VERSION)   continue;
        if (a->bytes   != sizeof(bpvm_ancla_t)) continue;
        if (a->bios    == 0)                    continue;
        if (a->prueba  == 0)                    continue;
        if (a->cargar_pack == 0)                continue;
        return a;
    }
    return 0;
}
