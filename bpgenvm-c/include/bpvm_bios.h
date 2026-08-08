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
#define BPVM_BIOS_VERSION  2u   /* 2 = V5/H2: entran las ranuras de ficheros */

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

    /*
     * ── ficheros (V5/H2) ──
     *
     * Sin esto, un pack nativo NO PUEDE ABRIR UN FICHERO por muy montado que
     * esté el sistema de ficheros debajo: la tabla sólo prestaba memoria,
     * cadenas y tiempo. Es lo que le faltaba a SQLite para llegar a la SD.
     *
     * ─── POR QUÉ POR DESCRIPTOR Y NO POR CAMINO ───
     *
     * La fachada de dentro es por CAMINO (abre y cierra en cada operación).
     * Exponerla así habría sido más corto, pero una base de datos lee miles de
     * páginas y cada una volvería a recorrer el directorio. Y sobre todo: la
     * forma de la API es lo que un pack ya grabado NO puede cambiar. Si nace
     * por camino y luego hace falta velocidad, hay que romper el contrato.
     *
     * Así que la FORMA es la que quiere una BD —abre una vez, lee y escribe por
     * desplazamiento, cierra— y por dentro, de momento, es la fachada por
     * camino. Cuando la velocidad moleste, el descriptor pasa a guardar el
     * fichero abierto de verdad y **el pack no se entera**. Ese es el orden
     * correcto: acertar la forma ahora, optimizar detrás después.
     *
     * ⚠️ `sincronizar` hoy no hace nada más que devolver 0: los backends
     * vuelcan al cerrar cada operación, así que no hay nada pendiente. Está en
     * la tabla porque una BD la llama y porque el día que haya escritura con el
     * fichero abierto SÍ hará falta — y entonces el pack ya la estará llamando.
     */
    int   (*abrir)   (const char* camino, int para_escribir);  /* fd, o -1     */
    int   (*cerrar)  (int fd);                                  /* 0 / -1      */
    long  (*leer)    (int fd, uint32_t desde, void* dst, uint32_t n);
    long  (*escribir)(int fd, uint32_t desde, const void* src, uint32_t n);
    int   (*truncar) (int fd, uint32_t tam);
    long  (*tamano)  (int fd);                                  /* bytes / -1  */
    int   (*sincronizar)(int fd);
    int   (*borrar)  (const char* camino);
    int   (*existe)  (const char* camino);                      /* 1 / 0       */
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

/*
 * V5/H2 — las ranuras de FICHEROS, ya implementadas sobre la fachada `bpvm_fs`
 * (bpvm_bios_fs.c). Portables: cada firmware sólo apunta sus ranuras aquí en
 * vez de escribirlas otra vez, que es como divergen las familias.
 */
int  bpvm_bios_fs_abrir  (const char* camino, int para_escribir);
int  bpvm_bios_fs_cerrar (int fd);
long bpvm_bios_fs_leer   (int fd, uint32_t desde, void* dst, uint32_t n);
long bpvm_bios_fs_escribir(int fd, uint32_t desde, const void* src, uint32_t n);
int  bpvm_bios_fs_truncar(int fd, uint32_t tam);
long bpvm_bios_fs_tamano (int fd);
int  bpvm_bios_fs_sincronizar(int fd);
int  bpvm_bios_fs_borrar (const char* camino);
int  bpvm_bios_fs_existe (const char* camino);
/* Suelta TODOS los descriptores. Para el arranque y para las pruebas. */
void bpvm_bios_fs_reset  (void);

/* Cuántas ranuras de función tiene la tabla. Para el log del arranque: decir
 * "BIOS lista (17 ranuras)" es más útil que "BIOS lista". */
int bpvm_bios_slot_count(void);

/*
 * ─── EL ANCLA ─── idea de Eduardo (7-ago-2026)
 *
 * Un pack necesita la tabla de arriba, pero NO PUEDE SABER SU DIRECCIÓN: cada
 * enlace del firmware la mueve. Perseguir esa dirección nos costó, en una tarde,
 * un cuelgue de la placa y un diagnóstico equivocado — y el desfase era MUDO:
 * el número seguía pareciendo bueno.
 *
 * La salida es no acertar la dirección, sino BUSCARLA: se pone un texto
 * constante en la imagen y **los punteros justo detrás**. Quien los quiera barre
 * un trozo de flash, encuentra el texto, y lee lo que sigue — que lo rellenó el
 * enlazador, así que siempre es correcto PARA ESA IMAGEN.
 *
 * Y la marca no se comprueba sola: también `version` y `bytes`. Ocho bytes
 * pueden repetirse por casualidad en 1 MB de código; los cuatro campos a la vez,
 * no. Es el mismo criterio del gate del .mod (#284): que el desfase GRITE.
 *
 * `prueba` es el control del instrumento: un CRC-16 con RESPUESTA CONOCIDA
 * escrita en bpvm_pack.h — `crc16("123456789") == 0x29B1`. Un pack puede
 * llamarlo y comprobar el número ANTES de fiarse de nada más. Si sale 0x29B1,
 * sabe que encontró el ancla de verdad, que sabe llamar al firmware y que los
 * argumentos y el retorno cruzan bien. Sin eso, "no se colgó" es todo lo que
 * tendría, y eso no es una prueba.
 */
#define BPVM_ANCLA_VERSION 2u

typedef struct bpvm_ancla {
    char     magia[8];      /* 'B','P','A','N','C','L','A','1' — SIN NUL final */
    uint16_t version;       /* BPVM_ANCLA_VERSION                              */
    uint16_t bytes;         /* sizeof(bpvm_ancla_t): crecer sin romper a nadie */
    const bpvm_bios_t* bios;                                /* la tabla        */
    uint16_t (*prueba)(uint16_t, const uint8_t*, uint32_t);  /* CRC-16          */

    /* v2 — CARGAR UN PACK NATIVO: buscarlo en la zona XIP, subir la escalera y
     * saltar. Vive en el firmware y no en el pack/`.mdn` porque un `.mdn` no
     * puede llamar a nada por nombre y tendria que llevar SU COPIA de la
     * escalera — la unica regla que no puede divergir. Se llama desde aqui.
     *
     *   >= 0  se salto, y es lo que devolvio el pack
     *   <  0  no se salto; -valor es el peldano (bpvm_npack_res_t) */
    int32_t  (*cargar_pack)(void);
} bpvm_ancla_t;

/*
 * Barre [base, base+bytes) buscando el ancla. Devuelve NULL si no está.
 *
 * Avanza de 4 en 4 porque el ancla lleva punteros y el compilador la alinea a 4:
 * mirar las posiciones intermedias sería tiempo tirado. No escribe nada y no
 * aloca: se puede llamar desde donde sea, incluido un pack.
 *
 * ESTA es la función que el firmware usa para verificarse a sí mismo en el
 * arranque, y la misma regla que implementa el pack. Si el pack no encuentra el
 * ancla, no es por una discrepancia de criterio.
 */
const bpvm_ancla_t* bpvm_ancla_buscar(const void* base, uint32_t bytes);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_BIOS_H */
