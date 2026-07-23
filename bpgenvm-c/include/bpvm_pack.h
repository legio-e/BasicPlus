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

/* Entradas del pack que empieza en pack_off (previamente localizado con scan).
 * Devuelve el nº de entradas (puede ser > max) o -1 si la cabecera no valida. */
int bpvm_pack_entries(const uint8_t* base, uint32_t region_size, uint32_t pack_off,
                      bpvm_pack_entry_t* out, int max);

/* Busca el fichero (tipo, nombre) en los packs ACTIVOS. Si varios lo tienen,
 * gana el ÚLTIMO de la cadena (= el añadido más recientemente: el flujo de
 * actualización es "add nuevo + tombstone viejo"). Devuelve el puntero XIP al
 * dato (dentro de la región) y su longitud en *len, o NULL si no está. */
const uint8_t* bpvm_pack_find(const uint8_t* base, uint32_t region_size,
                              const char* tipo, const char* nombre, uint32_t* len);

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

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PACK_H */
