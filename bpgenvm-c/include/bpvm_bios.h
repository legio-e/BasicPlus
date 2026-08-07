/* bpvm_bios.h — la TABLA que el firmware le presta a un pack de código nativo.
 *
 * El pack se compila y se pre-enlaza en el PC SIN saber dónde va a caer ni qué
 * firmware lo va a ejecutar. Su único enlace con la placa es UN PUNTERO a esta
 * tabla (medido en la prueba A: `g_bios`, 16 referencias, y CERO símbolos sin
 * resolver tras el pre-enlazado).
 *
 * POR QUÉ UNA TABLA Y NO DIRECCIONES HORNEADAS: si el pack llevara dentro la
 * dirección del `memcpy` de este firmware, una actualización de firmware
 * rompería TODOS los packs ya grabados. Con la tabla, el pack sólo necesita que
 * el contrato (orden de los campos + versión) se mantenga.
 *
 * ─── LA PRIMERA RANURA ES `log`, Y ES A PROPÓSITO ───
 *
 * Criterio de Eduardo (7-ago): *"hay que poner chivatos que nos digan qué es lo
 * que no funciona, para saber dónde y no sólo funciona/no funciona"*.
 *
 * Si el pack no puede escribir en el log, es **mudo por construcción**: todo lo
 * que le pase dentro se reduce a "va / no va". `log` no es una comodidad de
 * depuración — es la condición para que el resto sea diagnosticable. Por eso va
 * la primera y por eso se verifica como cualquier otra.
 *
 * La lista de funciones NO está inventada: es la superficie que SQLite deja
 * abierta, MEDIDA en la prueba A (36 símbolos sin resolver, de los cuales 18 son
 * del runtime del compilador y se enlazan de libgcc — no pueden ir por tabla
 * porque `__aeabi_ldivmod` devuelve un par en r0:r1 y eso no se expresa en C).
 */
#ifndef BPVM_BIOS_H
#define BPVM_BIOS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 'BPBI'. Lo comprueba el PACK: si no cuadra, es que le han dado otra cosa.
 * Mismo principio que el gate de ABI del .mod (#284) — que el desfase GRITE. */
#define BPVM_BIOS_MAGIC    0x42504249u

/* Sube al AÑADIR, QUITAR o REORDENAR campos. El pack compara y se niega si no
 * habla su versión: mejor un "no" con nombre que llamar a la ranura equivocada,
 * que es un cuelgue mudo. */
#define BPVM_BIOS_VERSION  1u

struct tm;

typedef struct bpvm_bios {
    uint32_t magic;                 /* BPVM_BIOS_MAGIC                        */
    uint32_t version;               /* BPVM_BIOS_VERSION                      */

    /* ── LA VOZ DEL PACK. Sin esto no hay diagnóstico posible. ── */
    void   (*log)    (const char* msg);

    /* ── memoria ── */
    void*  (*memcpy) (void*, const void*, size_t);
    void*  (*memmove)(void*, const void*, size_t);
    void*  (*memset) (void*, int, size_t);
    int    (*memcmp) (const void*, const void*, size_t);
    void*  (*memchr) (const void*, int, size_t);

    /* ── cadenas ── */
    size_t (*strlen) (const char*);
    int    (*strcmp) (const char*, const char*);
    int    (*strncmp)(const char*, const char*, size_t);
    char*  (*strchr) (const char*, int);
    char*  (*strrchr)(const char*, int);
    size_t (*strspn) (const char*, const char*);
    size_t (*strcspn)(const char*, const char*);

    /* ── alocador ── el del pack, NO el heap BP (son de formas distintas:
     * bpvm_heap_alloc devuelve handles con tipo y sujetos al GC; esto quiere
     * punteros crudos). Sale de la arena reservada por el ENV. */
    void*  (*malloc) (size_t);
    void   (*free)   (void*);
    void*  (*realloc)(void*, size_t);

    /* ── tiempo ── */
    struct tm* (*localtime)(const void* t);
} bpvm_bios_t;

/*
 * Verifica la tabla ENTERA antes de entregársela a nadie.
 *
 * Un NULL en cualquier ranura es un CUELGUE ESPERANDO A OCURRIR, y —esto es lo
 * importante— es detectable AQUÍ, en el arranque, sin ejecutar nada del pack.
 * Convierte un hard fault mudo en "la BIOS tiene huecos: memcpy".
 *
 * Devuelve NULL si la tabla está completa; si no, el NOMBRE del primer campo que
 * falta (literal estático, nunca hay que liberarlo). Magic o versión malos
 * devuelven "magic"/"version".
 */
const char* bpvm_bios_verify(const bpvm_bios_t* b);

/* Cuántas ranuras de función tiene la tabla. Para el log del arranque: decir
 * "BIOS lista (17 ranuras)" es más útil que "BIOS lista". */
int bpvm_bios_slot_count(void);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_BIOS_H */
