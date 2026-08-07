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
