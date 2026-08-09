/* bpvm_bios.c — verificación de la tabla que se le presta al pack nativo.
 *
 * Sin dependencias más allá de <stddef.h>: host-testable sin arrastrar la VM,
 * igual que bpvm_env/bpvm_part/bpvm_boot/bpvm_sqlmem.
 */
#include "bpvm_bios.h"

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
#define BPVM_PACK_MAX 4

static struct { uint32_t marca; const void* tabla; } s_packs[BPVM_PACK_MAX];

int bpvm_bios_publica(uint32_t marca, const void* tabla)
{
    if (marca == 0 || tabla == 0) return -1;

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
