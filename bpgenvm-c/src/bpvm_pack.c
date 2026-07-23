/*
 * bpvm_pack.c — H3: núcleo portable de la zona de packs. Ver bpvm_pack.h.
 *
 * Paridad ESTRICTA con el lado PC (pack/PackFormat.java + PackReader.java):
 * mismos offsets, mismo CRC (CCITT-FALSE), misma cobertura (crc_cab salta el
 * campo flags y a sí mismo; crc_contenido cubre [128, content_end) INCLUYENDO
 * el pad de alineación de la última entrada). Un byte distinto aquí = el micro
 * rechaza todos los packs del PC — cualquier cambio va acompañado de su
 * cambio espejo en Java y de make test-pack (fixtures generadas por Pack.jar).
 *
 * Regla de confianza del scan: crc_cab OK ⇒ size_total es fiable ⇒ se puede
 * SALTAR al siguiente pack aunque el contenido esté corrupto (crc_ok=0). Una
 * CABECERA mala corta la cadena (no hay salto fiable) → end_off=NO_SPACE y el
 * ADD se niega: eso lo repara la compactación desde el PC, no un append a ciegas.
 */
#include "bpvm_pack.h"
#include <string.h>

/* ── offsets de cabecera (espejo de PackFormat.java) ── */
#define OFF_MAGIC       0
#define OFF_VERFMT      4
#define OFF_FLAGS       6
#define OFF_SIZE_TOTAL  8
#define OFF_NOMBRE      12
#define OFF_FECHA       44
#define OFF_VERCONT     48
#define OFF_CRC_CONT    64
#define OFF_CRC_CAB     126
#define EOFF_TIPO       0
#define EOFF_NOMBRE     4
#define EOFF_LONGITUD   36
#define TIPO_END        0xFFFFFFFFuL   /* tipo == slack borrado = fin de entradas */

uint16_t bpvm_pack_crc16(uint16_t crc, const uint8_t* data, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        crc = (uint16_t) (crc ^ ((uint16_t) data[i] << 8));
        for (int b = 0; b < 8; b++)
            crc = (uint16_t) ((crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1));
    }
    return crc;
}

/* ── lecturas big-endian ── */
static uint32_t get_u32(const uint8_t* p) {
    return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16)
         | ((uint32_t) p[2] << 8)  |  (uint32_t) p[3];
}
static uint16_t get_u16(const uint8_t* p) {
    return (uint16_t) (((uint16_t) p[0] << 8) | p[1]);
}
static void put_u16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t) (v >> 8);
    p[1] = (uint8_t) v;
}

/* Campo fijo → C-string (hasta el 1er NUL, siempre NUL-terminado). */
static void read_fixed(char* dst, const uint8_t* src, int field_len) {
    int n = 0;
    while (n < field_len && src[n] != 0) { dst[n] = (char) src[n]; n++; }
    dst[n] = 0;
}

static uint32_t align4(uint32_t n) { return (n + 3u) & ~3u; }

/* Siguiente entrada del pack en base[pack_off], iterando con *rel (offset
 * RELATIVO al pack, arranca en HEADER_SIZE). 1 = entrada en *e; 0 = fin
 * (slack o fin de pack); -1 = entrada malformada (truncada / se sale). */
static int next_entry(const uint8_t* base, uint32_t pack_off, uint32_t size_total,
                      uint32_t* rel, bpvm_pack_entry_t* e) {
    uint32_t off = *rel;
    if (off + 4 > size_total) return 0;
    const uint8_t* p = base + pack_off + off;
    if (get_u32(p + EOFF_TIPO) == TIPO_END) return 0;
    if (off + BPVM_PACK_ENTRY_HSIZE > size_total) return -1;
    uint32_t len = get_u32(p + EOFF_LONGITUD);
    uint32_t data_rel = off + BPVM_PACK_ENTRY_HSIZE;
    if (len > size_total - data_rel) return -1;
    if (e) {
        read_fixed(e->tipo,   p + EOFF_TIPO,   BPVM_PACK_TYPE_LEN);
        read_fixed(e->nombre, p + EOFF_NOMBRE, BPVM_PACK_NAME_LEN);
        e->len      = len;
        e->data_off = pack_off + data_rel;
    }
    *rel = data_rel + align4(len);
    return 1;
}

/* Parsea + valida la cabecera del pack en `off`. 0 = cabecera FIABLE (crc_cab
 * OK, verfmt conocido, size_total dentro de la región → se puede encadenar);
 * -1 = cabecera NO fiable (corta la cadena). En ambos casos rellena `info`
 * con lo que haya (diagnóstico). El contenido se valida aparte. */
static int parse_header(const uint8_t* base, uint32_t region_size, uint32_t off,
                        bpvm_pack_info_t* info) {
    memset(info, 0, sizeof *info);
    info->off = off;
    if (off + BPVM_PACK_HEADER_SIZE > region_size) return -1;
    const uint8_t* h = base + off;

    info->verfmt = get_u16(h + OFF_VERFMT);
    info->flags  = get_u16(h + OFF_FLAGS);
    info->alive  = (info->flags & BPVM_PACK_ALIVE_BIT) != 0;
    info->size_total = get_u32(h + OFF_SIZE_TOTAL);
    info->fecha      = get_u32(h + OFF_FECHA);
    read_fixed(info->nombre,  h + OFF_NOMBRE,  BPVM_PACK_NAME_LEN);
    read_fixed(info->vercont, h + OFF_VERCONT, BPVM_PACK_VERCONT_LEN);

    /* crc_cab ANTES de fiarnos de nada: cubre [0,6) + [8,126) — salta flags
     * (para que el tombstone no la invalide) y el propio CRC. */
    uint16_t crc = bpvm_pack_crc16(BPVM_PACK_CRC16_INIT, h, OFF_FLAGS);
    crc = bpvm_pack_crc16(crc, h + OFF_SIZE_TOTAL, OFF_CRC_CAB - OFF_SIZE_TOTAL);
    if (crc != get_u16(h + OFF_CRC_CAB)) return -1;
    if (info->verfmt != BPVM_PACK_VERFMT) return -1;
    if (info->size_total < BPVM_PACK_HEADER_SIZE ||
        info->size_total > region_size - off) return -1;
    return 0;
}

/* Recorre las entradas del pack (cabecera ya validada): cuenta, calcula
 * content_end y opcionalmente verifica crc_contenido. Devuelve 1 si el
 * contenido es válido, 0 si está malformado o el CRC no cuadra. */
static int check_content(const uint8_t* base, uint32_t off, bpvm_pack_info_t* info,
                         int verify_crc) {
    uint32_t rel = BPVM_PACK_HEADER_SIZE, n = 0;
    int r;
    while ((r = next_entry(base, off, info->size_total, &rel, NULL)) == 1) n++;
    info->n_entries   = (uint16_t) n;
    info->content_end = rel;
    if (r < 0) return 0;
    if (verify_crc) {
        uint16_t crc = bpvm_pack_crc16(BPVM_PACK_CRC16_INIT,
                                       base + off + BPVM_PACK_HEADER_SIZE,
                                       rel - BPVM_PACK_HEADER_SIZE);
        if (crc != get_u16(base + off + OFF_CRC_CONT)) return 0;
    }
    return 1;
}

int bpvm_pack_scan(const uint8_t* base, uint32_t region_size,
                   bpvm_pack_info_t* out, int max,
                   int verify_content, uint32_t* end_off) {
    int count = 0;
    uint32_t off = 0;
    uint32_t end = BPVM_PACK_NO_SPACE;
    for (;;) {
        if (off + 4 > region_size) { end = off; break; }    /* región agotada */
        uint32_t magic = get_u32(base + off);
        if (magic == 0xFFFFFFFFuL) { end = off; break; }    /* flash virgen = fin */
        bpvm_pack_info_t tmp;
        bpvm_pack_info_t* slot = (out && count < max) ? &out[count] : &tmp;
        if (magic != BPVM_PACK_MAGIC) break;                /* basura: cadena corrupta */
        if (parse_header(base, region_size, off, slot) != 0) {
            count++;                                        /* cabecera mala: reportar y cortar */
            break;
        }
        slot->crc_ok = (uint8_t) check_content(base, off, slot, verify_content);
        count++;
        off += slot->size_total;                            /* salto fiable (crc_cab OK) */
    }
    if (end_off) *end_off = end;
    return count;
}

int bpvm_pack_entries(const uint8_t* base, uint32_t region_size, uint32_t pack_off,
                      bpvm_pack_entry_t* out, int max) {
    bpvm_pack_info_t info;
    if (pack_off + 4 > region_size) return -1;
    if (get_u32(base + pack_off) != BPVM_PACK_MAGIC) return -1;
    if (parse_header(base, region_size, pack_off, &info) != 0) return -1;
    uint32_t rel = BPVM_PACK_HEADER_SIZE;
    int n = 0, r;
    bpvm_pack_entry_t e;
    while ((r = next_entry(base, pack_off, info.size_total, &rel, &e)) == 1) {
        if (out && n < max) out[n] = e;
        n++;
    }
    return (r < 0) ? -1 : n;
}

const uint8_t* bpvm_pack_find(const uint8_t* base, uint32_t region_size,
                              const char* tipo, const char* nombre, uint32_t* len) {
    const uint8_t* best = NULL;
    uint32_t best_len = 0;
    uint32_t off = 0;
    for (;;) {
        if (off + 4 > region_size) break;
        if (get_u32(base + off) != BPVM_PACK_MAGIC) break;  /* virgen o corrupto = fin */
        bpvm_pack_info_t info;
        if (parse_header(base, region_size, off, &info) != 0) break;
        if (info.alive) {
            uint32_t rel = BPVM_PACK_HEADER_SIZE;
            bpvm_pack_entry_t e;
            while (next_entry(base, off, info.size_total, &rel, &e) == 1) {
                if (strcmp(e.tipo, tipo) == 0 && strcmp(e.nombre, nombre) == 0) {
                    best = base + e.data_off;               /* el ÚLTIMO de la cadena gana */
                    best_len = e.len;
                }
            }
        }
        off += info.size_total;
    }
    if (len) *len = best_len;
    return best;
}

int32_t bpvm_pack_add(uint8_t* base, uint32_t region_size,
                      const uint8_t* img, uint32_t img_len) {
    /* La imagen se valida ENTERA antes de tocar la región (magic, verfmt,
     * crc_cab, tamaño exacto, entradas bien formadas y crc_contenido). */
    bpvm_pack_info_t info;
    if (img_len < BPVM_PACK_HEADER_SIZE) return BPVM_PACK_ERR_BADIMG;
    if (get_u32(img) != BPVM_PACK_MAGIC) return BPVM_PACK_ERR_BADIMG;
    if (parse_header(img, img_len, 0, &info) != 0) return BPVM_PACK_ERR_BADIMG;
    if (info.size_total != img_len) return BPVM_PACK_ERR_BADIMG;
    if (!check_content(img, 0, &info, 1)) return BPVM_PACK_ERR_BADIMG;

    uint32_t end;
    (void) bpvm_pack_scan(base, region_size, NULL, 0, 0, &end);
    if (end == BPVM_PACK_NO_SPACE) return BPVM_PACK_ERR_NOSPACE;   /* cadena corrupta */
    if (img_len > region_size - end) return BPVM_PACK_ERR_NOSPACE; /* no cabe */
    memcpy(base + end, img, img_len);
    return (int32_t) end;
}

int bpvm_pack_remove_at(uint8_t* base, uint32_t region_size, uint32_t pack_off) {
    bpvm_pack_info_t info;
    if (pack_off + 4 > region_size) return -1;
    if (get_u32(base + pack_off) != BPVM_PACK_MAGIC) return -1;
    if (parse_header(base, region_size, pack_off, &info) != 0) return -1;
    if (!info.alive) return -1;
    /* Tombstone: bit ALIVE 1→0. En NOR es una escritura legal sin borrado;
     * crc_cab no cubre flags, así que la cabecera sigue siendo válida. */
    put_u16(base + pack_off + OFF_FLAGS,
            (uint16_t) (info.flags & ~BPVM_PACK_ALIVE_BIT));
    return 0;
}

int32_t bpvm_pack_remove(uint8_t* base, uint32_t region_size, const char* nombre) {
    int32_t target = -1;
    uint32_t off = 0;
    for (;;) {
        if (off + 4 > region_size) break;
        if (get_u32(base + off) != BPVM_PACK_MAGIC) break;
        bpvm_pack_info_t info;
        if (parse_header(base, region_size, off, &info) != 0) break;
        if (info.alive && strcmp(info.nombre, nombre) == 0)
            target = (int32_t) off;                         /* el ÚLTIMO activo gana */
        off += info.size_total;
    }
    if (target < 0) return -1;
    if (bpvm_pack_remove_at(base, region_size, (uint32_t) target) != 0) return -1;
    return target;
}
