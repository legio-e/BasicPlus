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
