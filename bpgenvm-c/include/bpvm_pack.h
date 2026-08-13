/*
 * bpvm_pack.h — H3: lector/gestor de la zona de PACKS (XIP). Núcleo PORTABLE:
 * opera sobre una REGIÓN [base, size) que en el micro es la partición
 * BPVM_PART_PACKS mapeada en flash (XIP, lectura directa) y en host un buffer
 * RAM que simula esa flash (0xFF = borrado, como la NOR).
 *
 * Formato = espejo byte a byte del lado PC (pack/PackFormat.java, la fuente de
 * la verdad; spec en bp-analisis/temas/packs/especificacion). TODO big-endian.
 * Cabecera de pack 128 B + cadena de entradas de 48 B + datos alineados a 4.
 * Los packs se encadenan contiguos: siguiente = off + size_total (múltiplo del
 * bloque de borrado); magic 0xFFFFFFFF (flash virgen) = fin de cadena.
 *
 * Ops (orden de Eduardo): LIST (scan) → ADD (append al final de la cadena) →
 * REMOVE (tombstone: bit ALIVE 1→0, escribible en NOR sin borrar; crc_cab NO
 * cubre flags a propósito, así el tombstone no invalida la cabecera). El
 * espacio de los borrados se recupera con una COMPACTACIÓN (lado PC, no aquí).
 *
 * Sin malloc, sin stdio, sin dependencias — host-testable (make test-pack) y
 * el MISMO código va al firmware (las 3 familias).
 */
#ifndef BPVM_PACK_H
#define BPVM_PACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constantes del formato (paridad con PackFormat.java) ── */
#define BPVM_PACK_MAGIC        0x4250414BuL  /* 'B''P''A''K' */
#define BPVM_PACK_VERFMT       1             /* versión del layout que entiende esta impl */
#define BPVM_PACK_HEADER_SIZE  128
#define BPVM_PACK_ENTRY_HSIZE  48
#define BPVM_PACK_NAME_LEN     32
#define BPVM_PACK_TYPE_LEN     4
#define BPVM_PACK_VERCONT_LEN  16
#define BPVM_PACK_ALIVE_BIT    0x0001        /* tombstone = este bit a 0 */

/* CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, sin reflexión ni xorout).
 * update permite CRCs no contiguos (la cabecera salta el campo flags).
 * Comprobación estándar: crc("123456789") == 0x29B1. */
#define BPVM_PACK_CRC16_INIT   0xFFFF
uint16_t bpvm_pack_crc16(uint16_t crc, const uint8_t* data, uint32_t len);

/* #362 — prefijo que CUALIFICA un recurso: "pack:<Pack>/<fichero.ext>" pide ese
 * fichero DENTRO de ese pack, sin mirar ningún otro. Vive aquí, en un solo
 * sitio, porque lo escriben los programas BP y lo lee la VM: si se deletreara
 * en dos, un día dejarían de ser el mismo. */
#define BPVM_PACK_URI "pack:"

/* Descriptor ligero de un pack de la cadena (sin copiar datos). */
typedef struct {
    uint32_t off;            /* offset del pack dentro de la región */
    uint32_t size_total;     /* tamaño total reservado (múltiplo de bloque) */
    uint32_t content_end;    /* fin de entradas, RELATIVO al inicio del pack */
    uint32_t fecha;          /* unix time (informativo) */
    uint16_t verfmt;
    uint16_t flags;
    uint8_t  alive;          /* (flags & ALIVE_BIT) != 0 */
    uint8_t  crc_ok;         /* crc_cab (y contenido si verify_content) OK */
    uint16_t n_entries;
    char     nombre[BPVM_PACK_NAME_LEN + 1];    /* NUL-terminado */
    char     vercont[BPVM_PACK_VERCONT_LEN + 1];
} bpvm_pack_info_t;

/* Una entrada (fichero) dentro de un pack. data_off es ABSOLUTO en la región
 * → &base[data_off] es el puntero XIP al contenido. */
typedef struct {
    char     tipo[BPVM_PACK_TYPE_LEN + 1];      /* extensión, minúsculas */
    char     nombre[BPVM_PACK_NAME_LEN + 1];
    uint32_t len;
    uint32_t data_off;
} bpvm_pack_entry_t;

/* ── FUENTE de lectura (#310) ────────────────────────────────────────────────
 * Un pack se lee de dos sitios y NO queremos dos lectores:
 *
 *   · zona de packs  — partición mapeada en flash. Direccionable: `direct`
 *                      apunta a ella y se puede leer (y EJECUTAR) en sitio.
 *   · fichero del FS — /app/loquesea.pack. NO direccionable (littlefs no da
 *                      bytes contiguos): sólo `read_at`, por trozos.
 *
 * De aquí sale sola la regla de Eduardo sobre con/sin código: un módulo se
 * carga XIP (sin copiar el código) EXACTAMENTE cuando su fuente da puntero
 * directo. Si no lo da, se carga por stream y el código se copia. No hay que
 * programar la distinción — se pregunta con bpvm_pack_src_ptr().
 *
 * En RAM no cabe un pack entero (restricción de Eduardo): de aquí sólo salen
 * cabeceras (128 B) y entradas (48 B) a buffers de pila. Sólo lectura.
 *
 * `read_at` tiene la MISMA firma que bpvm_read_at_fn del loader a propósito:
 * el callback que sirve para leer el pack sirve para cargar el módulo. */
typedef struct {
    long (*read_at)(void* user, uint32_t off, uint8_t* dst, uint32_t n);
    void*          user;
    const uint8_t* direct;   /* NULL = no direccionable (hay que leer por trozos) */
    uint32_t       size;     /* tamaño de la región / del fichero */
} bpvm_pack_src_t;

/* Fuente sobre una región mapeada (zona de packs, o el buffer RAM del host). */
void bpvm_pack_src_mem(bpvm_pack_src_t* s, const uint8_t* base, uint32_t size);

/* Fuente sobre algo que sólo se sabe leer por trozos (un fichero del FS). El
 * caller construye el callback: así este fichero sigue SIN dependencias (ni
 * stdio ni FS) y se puede seguir probando en host y compartir entre familias. */
void bpvm_pack_src_stream(bpvm_pack_src_t* s,
                          long (*read_at)(void*, uint32_t, uint8_t*, uint32_t),
                          void* user, uint32_t size);

/* Puntero directo a [off, off+len) si la fuente es direccionable y cabe; si no,
 * NULL. ESTE es el predicado de "¿XIP o copia?". */
const uint8_t* bpvm_pack_src_ptr(const bpvm_pack_src_t* s, uint32_t off, uint32_t len);

/* ── Región MONTADA (resolución de módulos, H3.c) ──
 * El arranque registra la zona de packs activa (host: --pack= sobre RAM;
 * firmware: la partición PACKS por XIP) y la resolución de imports la consulta
 * como fallback tras el FS (el FS ECLIPSA al pack, spec §4). NULL = sin packs. */
void bpvm_pack_mount(const uint8_t* base, uint32_t size);
const uint8_t* bpvm_pack_mounted(uint32_t* size_out);

/* LIST — recorre la cadena y rellena hasta `max` descriptores (activos Y
 * tombstones). Devuelve el nº de packs encontrados (puede ser > max; solo se
 * escriben los primeros max). end_off (opcional) = offset del primer hueco
 * libre, donde iría el siguiente ADD; si la cadena está corrupta (cabecera con
 * CRC malo, magic irreconocible, o un pack que se sale de la región) el scan
 * PARA ahí y end_off = BPVM_PACK_NO_SPACE (no hay punto de append fiable).
 * verify_content=1 verifica además crc_contenido de cada pack (O(datos): para
 * el boot/burn; el LIST normal con 0 solo verifica cabeceras). */
#define BPVM_PACK_NO_SPACE 0xFFFFFFFFuL
int bpvm_pack_scan(const uint8_t* base, uint32_t region_size,
                   bpvm_pack_info_t* out, int max,
                   int verify_content, uint32_t* end_off);
/* #310 — la misma sobre una fuente cualquiera. La variante de arriba es un
 * envoltorio de ésta con una fuente de memoria: hay UN lector, no dos. */
int bpvm_pack_scan_src(const bpvm_pack_src_t* src,
                       bpvm_pack_info_t* out, int max,
                       int verify_content, uint32_t* end_off);

/* Entradas del pack que empieza en pack_off (previamente localizado con scan).
 * Devuelve el nº de entradas (puede ser > max) o -1 si la cabecera no valida. */
int bpvm_pack_entries(const uint8_t* base, uint32_t region_size, uint32_t pack_off,
                      bpvm_pack_entry_t* out, int max);
int bpvm_pack_entries_src(const bpvm_pack_src_t* src, uint32_t pack_off,
                          bpvm_pack_entry_t* out, int max);

/* Busca el fichero (tipo, nombre) en los packs ACTIVOS. Si varios lo tienen,
 * gana el ÚLTIMO de la cadena (= el añadido más recientemente: el flujo de
 * actualización es "add nuevo + tombstone viejo"). Devuelve el puntero XIP al
 * dato (dentro de la región) y su longitud en *len, o NULL si no está. */
const uint8_t* bpvm_pack_find(const uint8_t* base, uint32_t region_size,
                              const char* tipo, const char* nombre, uint32_t* len);

/* #362 — la CLAVE (tipo, nombre) de un recurso a partir de su path, y el
 * nombre del pack si viene CUALIFICADO como "pack:<Pack>/<fichero.ext>".
 * Buffers del caller: tipo[BPVM_PACK_TYPE_LEN+1], nombre[BPVM_PACK_NAME_LEN+1],
 * pack[BPVM_PACK_NAME_LEN+1] (queda "" si no se cualificó).
 * 1 = clave derivada; 0 = ese path no puede nombrar una entrada de pack. */
int bpvm_pack_key_de_path(const char* path, char* tipo, char* nombre, char* pack);

/* #310 — la misma sobre una fuente. NO devuelve puntero (una fuente por trozos
 * no lo tiene): devuelve la ENTRADA (data_off + len) y el caller decide con
 * bpvm_pack_src_ptr() si puede usarla en sitio o tiene que leerla por trozos.
 * 1 = encontrada, 0 = no está. */
int bpvm_pack_find_src(const bpvm_pack_src_t* src,
                       const char* tipo, const char* nombre,
                       bpvm_pack_entry_t* out);

/* #362 — la misma búsqueda pero DENTRO DE UN PACK con nombre: el resto de la
 * cadena ni se mira. Es lo que hace falta para decir "esta fuente, la de este
 * pack", cuando dos packs traen un recurso que se llama igual. Si ese pack no
 * está en la cadena devuelve 0, igual que si estuviera y no llevara el
 * fichero: para quien pregunta son el mismo "no lo tengo". */
int bpvm_pack_find_in_src(const bpvm_pack_src_t* src, const char* pack,
                          const char* tipo, const char* nombre,
                          bpvm_pack_entry_t* out);

/* ¿Empieza esta fuente por un pack? 1 = sí, 0 = no (o no cabe la cabecera).
 * Existe para que quien abre un pack pueda decir "esto NO es un pack" en vez
 * de echarle la culpa a lo primero que no encuentre dentro. Lee por la fuente,
 * así que vale igual para un pack mapeado que para uno leído por trozos. */
int bpvm_pack_src_is_pack(const bpvm_pack_src_t* src);

/* ── MANIFEST: lo que hace EJECUTABLE a un pack (#310) ───────────────────────
 * Entrada de tipo 'mft' llamada "manifest", texto `clave=valor` por líneas
 * (modelo jar). La sintetiza PackStep desde el `main` del .bpbuild; espejo de
 * PackFormat.java — un cambio aquí va con su cambio allí. */
#define BPVM_PACK_TYPE_MANIFEST  "mft"
#define BPVM_PACK_MANIFEST_NAME  "manifest"
/* Tope de manifest que se mira. No es el tamaño del manifest: es cuánto se
 * lee de él. Un pack NO cabe en RAM y el manifest tampoco tiene por qué. */
#define BPVM_PACK_MANIFEST_MAX   256

/* Valor de `clave` en el manifest del pack, NUL-terminado en `out`.
 * 1 = encontrada; 0 = no hay manifest, no está la clave, o no cabe en `cap`.
 * El módulo principal de un pack ejecutable es la clave "main". */
int bpvm_pack_manifest_get(const bpvm_pack_src_t* src, const char* key,
                           char* out, int cap);

/* ADD — valida la imagen completa (magic, verfmt, crc_cab, crc_contenido,
 * img_len == size_total) y la copia al final de la cadena. SOLO para regiones
 * escribibles por memcpy (host RAM); el burn real a flash lo hace la cintura
 * del firmware con esta misma validación + su protocolo de recuperación.
 * Devuelve el offset donde quedó, o <0: */
#define BPVM_PACK_ERR_BADIMG   (-1)  /* imagen corrupta o de formato desconocido */
#define BPVM_PACK_ERR_NOSPACE  (-2)  /* no cabe / cadena corrupta (sin punto de append) */
int32_t bpvm_pack_add(uint8_t* base, uint32_t region_size,
                      const uint8_t* img, uint32_t img_len);

/* REMOVE en un offset concreto: tombstone (ALIVE 1→0) del pack en pack_off.
 * Devuelve 0, o -1 si ahí no hay un pack válido (o ya está tombstoned). */
int bpvm_pack_remove_at(uint8_t* base, uint32_t region_size, uint32_t pack_off);

/* REMOVE por nombre: tombstone del ÚLTIMO pack ACTIVO llamado `nombre` (el que
 * "se ve", por la regla del find). Devuelve su offset o -1 si no está. */
int32_t bpvm_pack_remove(uint8_t* base, uint32_t region_size, const char* nombre);

/* ── BURN por CHUNKS (grabación real a flash, RAM constante) ──
 *
 * La imagen llega en trozos y se graba SECUENCIAL según llega; solo la
 * cabecera (128 B) se retiene en RAM. La validación completa (ambos CRC) se
 * hace en el END *leyendo de la flash* (XIP = el readback de verdad) y solo
 * entonces se graba la cabecera — [16,128) primero y el primer quadword (el
 * MAGIC) al FINAL. Corte de luz o CRC malo en cualquier punto ⇒ el magic
 * queda 0xFF ⇒ para el scan el pack NUNCA EXISTIÓ (disciplina append-only);
 * el siguiente burn re-borra esa zona. El PC conserva el pack y reintenta.
 *
 * La cintura pone las escrituras; offsets RELATIVOS a la región. erase_block =
 * granularidad de borrado del target (STM32 U5 8K, Pico/ESP 4K, sim 4K): el
 * begin exige size múltiplo (mantiene la cadena alineada para siempre). */
typedef struct bpvm_pack_flash {   /* con tag: bpvm_bmgr.h lo forward-declara */
    int (*erase)(void* user, uint32_t off, uint32_t len);              /* len mult. de block */
    int (*program)(void* user, uint32_t off, const uint8_t* d, uint32_t len);
    void*    user;
    uint32_t erase_block;
} bpvm_pack_flash_t;

/* Cuál de las verificaciones de `burn_end` dijo que no. Las TRES devolvían
 * BPVM_PACK_ERR_VERIFY y el wire contestaba el mismo "el CRC no cuadra" a las
 * tres: un chivato que no distingue "lo grabado está mal" de "la cabecera no se
 * quedó" no dice dónde mirar. Con esto, el mensaje del IDE nombra el paso. */
#define BPVM_PACK_PASO_NINGUNO   0
#define BPVM_PACK_PASO_RECORRIDO 1  /* la cadena de entradas leída de flash descarrila */
#define BPVM_PACK_PASO_CRC_CONT  2  /* la cadena va, pero el contenido no es el mandado */
#define BPVM_PACK_PASO_CABECERA  3  /* el contenido va; la cabecera no se quedó escrita */

/* Sesión de burn (la guarda el llamador; una a la vez por región). */
typedef struct {
    int      active;
    uint32_t off;                            /* destino en la región (append) */
    uint32_t total;                          /* tamaño anunciado en el BEGIN */
    uint32_t received;
    uint8_t  paso;                           /* BPVM_PACK_PASO_*: quién dijo VERIFY */
    uint8_t  hdr[BPVM_PACK_HEADER_SIZE];     /* cabecera retenida (se graba al final) */
} bpvm_pack_burn_t;

/* Tamaño máximo de chunk que el protocolo anuncia al PC (cabe en cualquier
 * transporte/buffer de los 3 micros). */
#define BPVM_PACK_BURN_CHUNK 4096

#define BPVM_PACK_ERR_ALIGN    (-3)  /* size no es múltiplo del bloque de borrado */
#define BPVM_PACK_ERR_STATE    (-4)  /* sesión inválida (sin begin / tamaño no cuadra) */
#define BPVM_PACK_ERR_VERIFY   (-5)  /* CRC no cuadra al verificar EN FLASH */
#define BPVM_PACK_ERR_IO       (-6)  /* la cintura de flash falló */

/* Abre sesión: valida size (>=128, múltiplo de erase_block), localiza el punto
 * de append (scan), comprueba que cabe y BORRA el rango destino. Devuelve el
 * offset destino (>=0) o BPVM_PACK_ERR_*. Una sesión activa previa se abandona
 * (su magic nunca se escribió → invisible). */
int32_t bpvm_pack_burn_begin(const uint8_t* base, uint32_t region_size,
                             const bpvm_pack_flash_t* fl, uint32_t size,
                             bpvm_pack_burn_t* s);

/* Un chunk más (en orden). len debe ser MÚLTIPLO DE 16 (contrato uniforme: así
 * cada program queda alineado a quadword, obligatorio en flash con ECC como el
 * U5; total es múltiplo del bloque ⇒ el último chunk también cumple). Los
 * primeros 128 B se retienen en s->hdr; el resto se graba directo. 0 OK, o
 * BPVM_PACK_ERR_STATE/IO (la sesión se cierra). */
int bpvm_pack_burn_data(bpvm_pack_burn_t* s, const bpvm_pack_flash_t* fl,
                        const uint8_t* data, uint32_t len);

/* Cierra: exige received==total, valida la cabecera retenida (crc_cab, verfmt,
 * size_total==total) y el contenido LEYENDO de la flash (crc_contenido); solo
 * si todo cuadra graba la cabecera (magic al final). Devuelve el offset del
 * pack ya VISIBLE (>=0) o BPVM_PACK_ERR_* (y la zona queda invisible). */
int32_t bpvm_pack_burn_end(const uint8_t* base, uint32_t region_size,
                           bpvm_pack_burn_t* s, const bpvm_pack_flash_t* fl);

/* FORMAT — borra la región ENTERA (estreno de la zona o recuperación de una
 * cadena corrupta: una zona recién reparticionada lleva RESTOS de su uso
 * anterior —littlefs, FS viejo— y el scan la ve corrupta, correctamente).
 * Operación EXPLÍCITA del usuario (con confirmación en el wire), nunca
 * automática. Borra TODOS los packs. 0 OK, BPVM_PACK_ERR_IO si falla. */
int bpvm_pack_format(const bpvm_pack_flash_t* fl, uint32_t region_size);

/* DEL en FLASH REAL (tombstone vía cintura): marca borrado el pack de
 * `pack_off` haciendo READ-MODIFY-WRITE de su PRIMERA página de borrado —
 * en la flash interna con ECC (U5) los flags comparten quadword con el magic
 * y un quadword solo se programa una vez, así que el bit no se puede tocar
 * in-situ. Los packs van alineados al bloque ⇒ la página solo contiene el
 * arranque de ESTE pack. `page_buf` lo pone el llamador (>= erase_block).
 * Ventana asumida (spec §6.2a): un corte entre erase y program deja la página
 * borrada → la cadena se corta AHÍ (los packs anteriores sobreviven; los
 * posteriores los recupera la compactación del PC, que conserva los packs).
 * 0 OK · BADIMG = ahí no hay pack válido · STATE = ya estaba borrado ·
 * IO/VERIFY = fallo de flash o el readback no cuadra. */
int bpvm_pack_del(const uint8_t* base, uint32_t region_size,
                  const bpvm_pack_flash_t* fl, uint32_t pack_off,
                  uint8_t* page_buf);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PACK_H */
