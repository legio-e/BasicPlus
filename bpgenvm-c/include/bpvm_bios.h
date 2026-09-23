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
#define BPVM_BIOS_VERSION  7u   /* 7 = V7/H6.c G4.a2: el TURNO de los buses     */
                                /* 6 = V7/H6.c G1: env_get (la config de placa) */
                                /* 5 = V7/H6: la HAL (gpio/spi/i2c/reloj)     */
                                /* 4 = V5/H4: el PUNTO DE ENCUENTRO de packs  */
                                /* 3 = V5/H3: entra la ranura de la ARENA     */
                                /* 2 = V5/H2: entran las ranuras de ficheros  */
/*
 * ─── LA TABLA CRECE POR EL FINAL (V7/H6) ───
 *
 * Cada versión hasta la 4 obligó a regenerar todos los packs grabados: el pack
 * comparaba `version != la suya` y se negaba. Con dos motores (SQLite, LVGL) y
 * siete imágenes, eso son demasiadas grabaciones por cada ranura nueva. La
 * regla desde la v5: **añadir es sólo añadir AL FINAL** —las ranuras que un
 * pack viejo conoce siguen en su sitio— y el pack exige `version >= la suya`.
 * Quitar o reordenar sigue siendo cambio de época: entonces cambia el MAGIC.
 * Es el mismo truco que el campo `bytes` del ancla: «crecer sin romper».
 */

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

    /* ── V5/H3 — LA ARENA DE LA BD ────────────────────────────────────────────
     *
     * Devuelve la BASE del bloque libre para la base de datos, y deja su tamaño
     * en `*bytes`. NULL (y `*bytes` a 0) = no hay arena, porque no se pidió o
     * porque no cupo. Ver `bpvm_sqlmem.h` para la REGLA de cuánta se reserva.
     *
     * Es una FUNCIÓN y no dos campos de datos por dos motivos:
     *  · el verificador de arriba recorre las ranuras como punteros a función y
     *    exige que ninguna sea NULL; un par de campos de datos se quedaría fuera
     *    de esa red, que es exactamente el hueco mudo que la lista evita;
     *  · el firmware decide la base EN EL ARRANQUE (depende de si hay PSRAM y de
     *    lo que diga el ENV), así que hornearla en una constante volvería a
     *    poner una dirección donde no debe haberla.
     *
     * ⚠️ NO es un alocador: es el bloque CRUDO. Lo gestiona SQLite con su propio
     * MEMSYS5 (`sqlite3_config(SQLITE_CONFIG_HEAP, …)`), que es con lo que se
     * MIDIÓ el mínimo de 2 MB y la regla del ~40 %. Poner aquí un malloc nuestro
     * sería otro sistema de memoria y esas medidas dejarían de aplicar.
     *
     * ⚠️ Y NO es todo el bloque: por debajo están los estáticos (`.data`/`.bss`)
     * de CADA motor cargado, cada uno en el trozo que dice su sello. Esta ranura
     * devuelve lo que queda por ENCIMA del último (`ram_tope` del plan de
     * `bpvm_npack.h`) — quien la llama no tiene que saberse el reparto.
     *
     * V7/H6.c G1 — CON VARIOS MOTORES, LA ARENA TIENE DUEÑO. Hoy sólo la pide
     * SQLite (el pool de LVGL vive en su `.bss`), pero dos motores con la MISMA
     * arena se machacarían el heap el uno al otro EN SILENCIO. Así que es del
     * primero que la pide DESDE SU `bp_pack_init`; si la pide otro, NULL, y el
     * cargador escribe de quién es (como con los pines: el segundo recibe un
     * «no» con nombre). Fuera de una entrada también es NULL: sin saber quién
     * pregunta no se puede saber de quién es. La lleva `bpvm_bios_arena`
     * (portable, en bpvm_bios.c): cada familia sólo le dice dónde está el
     * bloque. El día que dos motores necesiten arena, cada uno declarará la
     * suya; hasta entonces sería forma sin uso.
     */
    void* (*arena)(size_t* bytes);

    /* ── V5/H4 — EL PUNTO DE ENCUENTRO ────────────────────────────────────────
     *
     * Hasta aquí la tabla iba en UN solo sentido: el firmware presta y el pack
     * consume. Pero un pack también tiene algo que ofrecer —SQLite tiene una
     * API entera— y hacía falta un sitio donde dejarla.
     *
     * Estas dos ranuras son ese sitio, y son LO ÚNICO que la VM sabe de todo
     * esto. No conoce SQLite, ni LVGL, ni ningún driver de pantalla: sólo sabe
     * guardar un puntero bajo una marca y devolverlo. Por eso añadir un pack
     * nuevo NO toca la VM — que es la propiedad que hace que los packs valgan
     * la pena, y que se perdería entera si cada librería necesitara builtins
     * propios (con 5 imágenes hoy y 7 en cuanto entren el C3 y el C6).
     *
     * ─── POR QUÉ UNA MARCA Y NO UN NOMBRE ───
     *
     * `'SQLI'`, `'LVGL'`, `'PANT'`… cuatro bytes comparados con `==`. Buscar
     * por nombre pediría `strcmp` en un bucle y, sobre todo, una convención de
     * nombres que alguien acabaría escribiendo mal en silencio. La marca entra
     * en un registro y el desajuste no existe: o coincide o no.
     *
     * ─── EL CONTRATO ───
     *
     * `publica` la llama el pack UNA vez, al arrancar, desde su propia entrada.
     * Devuelve 0, o <0 si no hay hueco o la marca ya estaba (republicar sería
     * un pack cargado dos veces, que es un problema, no una actualización).
     *
     * `busca` devuelve NULL si esa marca no está — y ese NULL es lo que
     * convierte "el usuario no grabó el pack de SQLite" en un mensaje claro en
     * vez de un salto a ninguna parte.
     *
     * ⚠️ La tabla que se publica DEBE empezar por su propia marca y versión: el
     * que la recoge tiene que poder comprobar que habla su idioma. El pack se
     * congela y el IDE no, así que ese desfase va a existir — mejor que GRITE.
     * Es el mismo gate que el .mod (#284) y que esta misma tabla.
     */
    int         (*publica)(uint32_t marca, const void* tabla);
    const void* (*busca)  (uint32_t marca);

    /* ── V7/H6 — LA HAL ────────────────────────────────────────────────────
     *
     * Lo que un DRIVER DE PANTALLA en un pack necesita y la tabla no
     * prestaba: un pin, un bus SPI, un bus I2C, un reloj. Son las fachadas
     * de la VM (`bpvm_gpio`, `bpvm_spi`, `bpvm_i2c`, `bpvm_platform`), las
     * mismas en las cinco familias — por eso van aquí y no `esp_lcd` ni
     * DMA, que sólo existen en una. Medido antes de decidirlo: LVGL no pide
     * NADA de hardware (17 símbolos, todos de libc); el que habla con el
     * silicio es el driver del panel (28 símbolos en el C6, 65 en el P4).
     *
     * `gpio_init` RECLAMA el pin como de pack (H4: contra la tabla de
     * capacidades y el estado de los pines) y devuelve -1 con el porqué en
     * el log de la VM; el fin de RUN no lo libera. `spi_init`/`i2c_init`
     * validan la instancia. Modos y numeración: los de `Gpio.Pin`/`Spi.Bus`.
     * `now_ms` es el tick de LVGL (`lv_tick_set_cb`); `sleep_ms`, las esperas
     * de la secuencia de reset de un panel. */
    int      (*gpio_init) (int pin, int mode);                 /* 0 INPUT · 1 OUTPUT · 2 OD; -1 = denegado */
    void     (*gpio_write)(int pin, int value);
    int      (*gpio_read) (int pin);
    int      (*spi_init)  (int bus, int sck, int mosi, int miso, int baudrate, int mode);
    int      (*spi_write) (int bus, const uint8_t* data, size_t n);   /* bytes, o -1 */
    int      (*i2c_init)  (int bus, int sda, int scl, int baudrate);
    int      (*i2c_write) (int bus, int addr, const uint8_t* data, size_t n);
    int      (*i2c_read)  (int bus, int addr, uint8_t* data, size_t n);
    uint32_t (*now_ms)    (void);
    void     (*sleep_ms)  (int ms);

    /* ── V7/H6.c G1 — LA CONFIG DE PLACA ─────────────────────────────────
     *
     * Un driver en un pack que no puede leer el ENV tiene sus pines, su bus y
     * su panel escritos DENTRO: un pack por placa. Con esta ranura los lee al
     * arrancar y el mismo pack sirve a todas las placas con el mismo panel —
     * es LA pieza que convierte «un npk por placa» en «un npk para todas».
     * Y la config de placa ya vive en el ENV (`display=` en el P4, H9): no
     * hay formato nuevo que aprender.
     *
     * De SÓLO LECTURA a propósito: el ENV es de la placa y lo escriben el
     * usuario y el IDE; un pack que lo cambiara cambiaría el arranque de la
     * placa por la puerta de atrás.
     *
     * Devuelve la longitud del valor (NUL-terminado en `buf`), o:
     *   -1  la clave no está          -2  no cabe en `cap` (`buf` queda "")
     *   -3  argumentos malos
     * «No cabe» NO trunca: un pin a medias es un número plausible y
     * equivocado —mejor nada que a medias, como `bpvm_pack_manifest_get`—.
     * Las claves no distinguen mayúsculas (decisión del 19-jul para el ENV). */
    int      (*env_get)   (const char* clave, char* buf, size_t cap);

    /* ── V7/H6.c G4.a2 — EL TURNO DE LOS BUSES (v7) ───────────────────────
     *
     * El bus se COMPARTE —en la C5 la TF y la pantalla van en el mismo SPI 2,
     * el expansor y el táctil en el mismo I2C 0—; lo que no se mezcla es la
     * TRANSACCIÓN. Un pack pide el turno con SU configuración —bus, CS,
     * velocidad y modo: nada se guarda entre turnos (Eduardo: «operaciones
     * petición-fin»)—, transfiere lo que quiera, puede subir el CS sin soltar
     * el turno, y lo suelta al acabar. La velocidad sólo se cambia si es otra
     * que la actual del bus.
     *
     * Nada espera para siempre: si el bus no queda libre a tiempo, -1 con el
     * porqué en el log de la VM («SPI 2 ocupado (CS 23, el firmware) desde
     * hace 1000 ms»). `cs` = -1: la BIOS no toca ningún CS (lo lleva el pack,
     * p. ej. en un expansor). El CS lo reserva el pack con `gpio_init`, como
     * siempre; la BIOS sólo lo mueve durante el turno.
     *
     * En I2C cada `i2c_write`/`i2c_read` ya es un turno; `i2c_escribir_leer`
     * hace las dos en uno —nadie habla en medio— y devuelve los bytes leídos.
     * Las ranuras v5 (`spi_write`) siguen valiendo: cada llamada, un turno. */
    int      (*spi_tomar)     (int bus, int cs, int hz, int modo);            /* 0, o -1 */
    int      (*spi_cs)        (int bus, int nivel);                           /* dentro del turno */
    int      (*spi_transferir)(int bus, const uint8_t* tx, uint8_t* rx, size_t n);   /* bytes, o -1 */
    void     (*spi_soltar)    (int bus);
    int      (*i2c_escribir_leer)(int bus, int addr, const uint8_t* tx, size_t ntx,
                                  uint8_t* rx, size_t nrx);                   /* leídos, o -1 */
} bpvm_bios_t;

/* Los códigos de `env_get`, para que el pack no escriba números sueltos. */
#define BPVM_BIOS_ENV_NO_ESTA  (-1)
#define BPVM_BIOS_ENV_NO_CABE  (-2)
#define BPVM_BIOS_ENV_ARGS     (-3)

/*
 * ─── EL INICIALIZADOR, PEGADO A LA STRUCT ─────────────────────────────────────
 *
 * Rellena la tabla entera dejando SÓLO los huecos que de verdad dependen de la
 * placa. Nació al montar la segunda familia (V5/H7, el P4): de las 29
 * ranuras, **26 son idénticas** — 12 de libc que trae newlib (y el ESP32 usa
 * newlib igual que el RP2350), 9 de ficheros que ya son portables, 2 del punto
 * de encuentro, y 3 de memoria que hoy son chivatos.
 *
 * ⚠️ POR QUÉ AQUÍ Y NO EN UN .c POR FAMILIA. El orden de estos campos cruza una
 * frontera BINARIA: el pack se compila aparte, se graba, y llama por posición.
 * Si hubiera un inicializador por familia y alguien reordenara la struct
 * olvidándose de uno, ese firmware llamaría a `memcpy` y ejecutaría otra cosa —
 * sin error de compilación ni de enlace. Ya nos costó dos veces por el mismo
 * motivo (#299 layout de clase, #315 slots de vtable), y las dos veces la
 * solución fue la misma: que el orden viva en UN sitio. Aquí está pegado a la
 * declaración, así que reordenar sin tocarlo obliga a verlo.
 *
 * El llamante necesita `<string.h>` (las de libc se toman por su dirección) y
 * declara la tabla `static const`: vive en la imagen, que es lo que hace que se
 * le pueda prestar al pack sin que nadie tenga que mantenerla viva.
 *
 * Los seis parámetros son, en orden: la VOZ del pack, sus tres de memoria, el
 * tiempo, y (v6) dónde lee la placa su ENV.
 *
 * V7/H6.c G1 — la ARENA dejó de ser parámetro: con varios motores tiene dueño,
 * y esa regla no puede tener una copia por familia (es `bpvm_bios_arena`, en
 * bpvm_bios.c; la familia sólo le dice dónde está el bloque). Y `ENV_GET` entra
 * como parámetro, no como función portable, porque el ENV de cada familia vive
 * en un sitio distinto: así, una familia que no lo conecte NO COMPILA, en vez
 * de dar «la clave no está» para todo en silencio.
 */
#define BPVM_BIOS_TABLA(LOG, MALLOC, FREE, REALLOC, LOCALTIME, ENV_GET) { \
    BPVM_BIOS_MAGIC, BPVM_BIOS_VERSION,                                 \
    (LOG),                                                              \
    memcpy, memmove, memset, memcmp, memchr,                            \
    strlen, strcmp, strncmp, strchr, strrchr, strspn, strcspn,          \
    (MALLOC), (FREE), (REALLOC),                                        \
    (LOCALTIME),                                                        \
    /* V5/H2 — ficheros: la implementación PORTABLE, sobre la fachada   \
     * `bpvm_fs`. Escribirlas por familia es exactamente como divergen  \
     * las familias. */                                                 \
    bpvm_bios_fs_abrir, bpvm_bios_fs_cerrar,                            \
    bpvm_bios_fs_leer,  bpvm_bios_fs_escribir,                          \
    bpvm_bios_fs_truncar, bpvm_bios_fs_tamano, bpvm_bios_fs_sincronizar,\
    bpvm_bios_fs_borrar, bpvm_bios_fs_existe,                           \
    /* V5/H3 — la arena; V7/H6.c G1 — portable y con dueño. */          \
    bpvm_bios_arena,                                                    \
    /* V5/H4 — el punto de encuentro. Portable: nada que decidir. */    \
    bpvm_bios_publica, bpvm_bios_busca,                                 \
    /* V7/H6 — la HAL: las fachadas de la VM (bpvm_bios_hal.c). */      \
    bpvm_bios_hal_gpio_init, bpvm_bios_hal_gpio_write, bpvm_bios_hal_gpio_read, \
    bpvm_bios_hal_spi_init, bpvm_bios_hal_spi_write,                    \
    bpvm_bios_hal_i2c_init, bpvm_bios_hal_i2c_write, bpvm_bios_hal_i2c_read, \
    bpvm_bios_hal_now_ms, bpvm_bios_hal_sleep_ms,                       \
    /* V7/H6.c G1 — el ENV de la placa (bpvm_env_get_exacto detrás). */ \
    (ENV_GET),                                                          \
    /* V7/H6.c G4.a2 — el turno de los buses (el árbitro, bpvm_bus.c). */ \
    bpvm_bios_hal_spi_tomar, bpvm_bios_hal_spi_cs,                      \
    bpvm_bios_hal_spi_transferir, bpvm_bios_hal_spi_soltar,             \
    bpvm_bios_hal_i2c_escribir_leer                                     \
}

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

/*
 * V5/H4 — el registro del punto de encuentro, ya implementado y PORTABLE
 * (bpvm_bios.c). Cada firmware apunta sus ranuras `publica`/`busca` aquí; no
 * hay nada que decidir por familia y tenerlo cinco veces sólo daría cinco
 * sitios donde divergir.
 */
int         bpvm_bios_publica(uint32_t marca, const void* tabla);
const void* bpvm_bios_busca  (uint32_t marca);
/* V7/H6 — `publica` ya no acepta cualquier puntero: la tabla TIENE que ser una
 * `bpvm_pack_api_t` bien formada que empiece por ESA marca (§1.1 de V7_IDEAS:
 * la convención pasa a reja). Devuelve 0; -1 argumentos; -2 sin hueco; -3 la
 * marca ya estaba; -4 la tabla no es lo que dice (marca distinta, sin
 * publics, o un símbolo sin nombre o sin función). Ocho marcas caben. */
#define BPVM_BIOS_PACK_MAX 8

/* V7/H6 — las ranuras de la HAL, PORTABLES (bpvm_bios_hal.c): cada firmware
 * las apunta desde `BPVM_BIOS_TABLA`, no las reescribe. */
int      bpvm_bios_hal_gpio_init (int pin, int mode);
void     bpvm_bios_hal_gpio_write(int pin, int value);
int      bpvm_bios_hal_gpio_read (int pin);
int      bpvm_bios_hal_spi_init  (int bus, int sck, int mosi, int miso, int baudrate, int mode);
int      bpvm_bios_hal_spi_write (int bus, const uint8_t* data, size_t n);
int      bpvm_bios_hal_i2c_init  (int bus, int sda, int scl, int baudrate);
int      bpvm_bios_hal_i2c_write (int bus, int addr, const uint8_t* data, size_t n);
int      bpvm_bios_hal_i2c_read  (int bus, int addr, uint8_t* data, size_t n);
/* V7/H6.c G4.a2 — el turno (v7). */
int      bpvm_bios_hal_spi_tomar (int bus, int cs, int hz, int modo);
int      bpvm_bios_hal_spi_cs    (int bus, int nivel);
int      bpvm_bios_hal_spi_transferir(int bus, const uint8_t* tx, uint8_t* rx, size_t n);
void     bpvm_bios_hal_spi_soltar(int bus);
int      bpvm_bios_hal_i2c_escribir_leer(int bus, int addr, const uint8_t* tx, size_t ntx,
                                         uint8_t* rx, size_t nrx);
uint32_t bpvm_bios_hal_now_ms    (void);
void     bpvm_bios_hal_sleep_ms  (int ms);
/* Olvida todas las tablas. Para el arranque y para las pruebas. */
void        bpvm_bios_packs_reset(void);

/*
 * V7/H6.c G1 — LA ARENA, portable y con dueño (ver la ranura `arena`).
 *
 * El cargador de cada familia la maneja así:
 *   bpvm_bios_arena_fija(base, bytes)   tras planear: lo que queda por encima
 *                                       del último motor (`ram_tope` → fin del
 *                                       bloque). Olvida al dueño anterior.
 *   bpvm_bios_arena_entra("SQLite")     justo antes de saltar a su entrada
 *   bpvm_bios_arena_sale()              al volver
 *   bpvm_bios_arena_negada()            tras volver: por qué se le dijo que no
 *                                       (NULL si no se le negó) — para el log,
 *                                       que este fichero no tiene voz propia
 *
 * El nombre que se pasa a `entra` se COPIA: el del plan es de la pila del
 * cargador y no vive lo que vive el dueño.
 */
void*       bpvm_bios_arena        (size_t* bytes);   /* LA RANURA */
void        bpvm_bios_arena_fija   (void* base, size_t bytes);
void        bpvm_bios_arena_entra  (const char* pack);
void        bpvm_bios_arena_sale   (void);
const char* bpvm_bios_arena_negada (void);
const char* bpvm_bios_arena_duena  (void);            /* "" = libre */

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

    /* v2 — CARGAR LOS PACKS NATIVOS: recorrer la cadena de la zona, subir la
     * escalera de cada motor y saltar. Vive en el firmware y no en el
     * pack/`.mdn` porque un `.mdn` no puede llamar a nada por nombre y tendria
     * que llevar SU COPIA de la escalera — la unica regla que no puede
     * divergir. Se llama desde aqui.
     *
     *   >= 1  a cuántos motores se saltó (lo que devolvió cada uno, en el log)
     *   <  0  no se saltó a ninguno; -valor es el peldaño del último que se
     *         rechazó, o MAGIC si no había ninguno (bpvm_npack_res_t)
     * (Hasta G1 cargaba UNO y devolvía lo que devolvió; con varios, lo que
     * importa al que llama es si se saltó a alguno.) */
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
