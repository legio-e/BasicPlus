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

/* ── FUENTE de lectura (#310) ──
 * Todo el camino de LECTURA pasa por aquí. Dos sabores (mapeada / por trozos)
 * y un solo lector: la alternativa —un lector por sitio— es justo la
 * divergencia que la regla de "mecanismo único" prohíbe.
 * El camino de ESCRITURA (add/remove/burn/format/del) sigue exigiendo región
 * mapeada y escribible: eso es instalar, no ejecutar. */
void bpvm_pack_src_mem(bpvm_pack_src_t* s, const uint8_t* base, uint32_t size) {
    if (!s) return;
    s->read_at = NULL;
    s->user    = NULL;
    s->direct  = base;
    s->size    = size;
}

void bpvm_pack_src_stream(bpvm_pack_src_t* s,
                          long (*read_at)(void*, uint32_t, uint8_t*, uint32_t),
                          void* user, uint32_t size) {
    if (!s) return;
    s->read_at = read_at;
    s->user    = user;
    s->direct  = NULL;
    s->size    = size;
}

const uint8_t* bpvm_pack_src_ptr(const bpvm_pack_src_t* s, uint32_t off, uint32_t len) {
    if (!s || !s->direct) return NULL;
    if (off > s->size || len > s->size - off) return NULL;
    return s->direct + off;
}

/* Lee n bytes en dst. 1 = OK, 0 = fuera de rango o error de la fuente.
 * El bucle es obligatorio: una lectura por trozos puede devolver MENOS de lo
 * pedido sin que sea un error (es lo normal en el FS). */
static int src_read(const bpvm_pack_src_t* s, uint32_t off, uint8_t* dst, uint32_t n) {
    if (!s || off > s->size || n > s->size - off) return 0;
    if (s->direct) { memcpy(dst, s->direct + off, n); return 1; }
    if (!s->read_at) return 0;
    uint32_t done = 0;
    while (done < n) {
        long r = s->read_at(s->user, off + done, dst + done, n - done);
        if (r <= 0) return 0;
        done += (uint32_t) r;
    }
    return 1;
}

/* CRC sobre [off, off+len) de la fuente. Si es direccionable va de una tacada;
 * si no, por trozos. MISMO resultado — es la misma función sobre los mismos
 * bytes, sólo cambia cómo llegan. */
static uint16_t src_crc(const bpvm_pack_src_t* s, uint32_t off, uint32_t len, int* ok) {
    if (ok) *ok = 1;
    if (s->direct) {
        if (off > s->size || len > s->size - off) { if (ok) *ok = 0; return 0; }
        return bpvm_pack_crc16(BPVM_PACK_CRC16_INIT, s->direct + off, len);
    }
    uint16_t crc = BPVM_PACK_CRC16_INIT;
    uint8_t buf[128];
    uint32_t done = 0;
    while (done < len) {
        uint32_t n = len - done;
        if (n > sizeof buf) n = (uint32_t) sizeof buf;
        if (!src_read(s, off + done, buf, n)) { if (ok) *ok = 0; return 0; }
        crc = bpvm_pack_crc16(crc, buf, n);
        done += n;
    }
    return crc;
}

/* ── región montada (ver bpvm_pack.h) ── */
static const uint8_t* s_mounted_base = NULL;
static uint32_t       s_mounted_size = 0;

void bpvm_pack_mount(const uint8_t* base, uint32_t size) {
    s_mounted_base = base;
    s_mounted_size = size;
}

const uint8_t* bpvm_pack_mounted(uint32_t* size_out) {
    if (size_out) *size_out = s_mounted_size;
    return s_mounted_base;
}

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
static int next_entry(const bpvm_pack_src_t* src, uint32_t pack_off, uint32_t size_total,
                      uint32_t* rel, bpvm_pack_entry_t* e) {
    uint32_t off = *rel;
    if (off + 4 > size_total) return 0;
    uint8_t p[BPVM_PACK_ENTRY_HSIZE];
    /* Primero SÓLO el tipo: puede ser el centinela de fin, y en ese caso la
     * cabecera entera puede no estar (el slack no tiene por qué llegar). */
    if (!src_read(src, pack_off + off + EOFF_TIPO, p + EOFF_TIPO, 4)) return -1;
    if (get_u32(p + EOFF_TIPO) == TIPO_END) return 0;
    if (off + BPVM_PACK_ENTRY_HSIZE > size_total) return -1;
    if (!src_read(src, pack_off + off, p, BPVM_PACK_ENTRY_HSIZE)) return -1;
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
static int parse_header(const bpvm_pack_src_t* src, uint32_t off,
                        bpvm_pack_info_t* info) {
    uint32_t region_size = src->size;
    memset(info, 0, sizeof *info);
    info->off = off;
    if (off + BPVM_PACK_HEADER_SIZE > region_size) return -1;
    uint8_t h[BPVM_PACK_HEADER_SIZE];
    if (!src_read(src, off, h, BPVM_PACK_HEADER_SIZE)) return -1;

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
static int check_content(const bpvm_pack_src_t* src, uint32_t off, bpvm_pack_info_t* info,
                         int verify_crc) {
    uint32_t rel = BPVM_PACK_HEADER_SIZE, n = 0;
    int r;
    while ((r = next_entry(src, off, info->size_total, &rel, NULL)) == 1) n++;
    info->n_entries   = (uint16_t) n;
    info->content_end = rel;
    if (r < 0) return 0;
    if (verify_crc) {
        int ok = 0;
        uint16_t crc = src_crc(src, off + BPVM_PACK_HEADER_SIZE,
                               rel - BPVM_PACK_HEADER_SIZE, &ok);
        if (!ok) return 0;
        uint8_t cc[2];
        if (!src_read(src, off + OFF_CRC_CONT, cc, 2)) return 0;
        if (crc != get_u16(cc)) return 0;
    }
    return 1;
}

int bpvm_pack_scan_src(const bpvm_pack_src_t* src,
                       bpvm_pack_info_t* out, int max,
                       int verify_content, uint32_t* end_off) {
    int count = 0;
    uint32_t off = 0;
    uint32_t end = BPVM_PACK_NO_SPACE;
    for (;;) {
        if (off + 4 > src->size) { end = off; break; }      /* región agotada */
        uint8_t mg[4];
        if (!src_read(src, off, mg, 4)) break;              /* fuente rota = fin */
        uint32_t magic = get_u32(mg);
        if (magic == 0xFFFFFFFFuL) { end = off; break; }    /* flash virgen = fin */
        bpvm_pack_info_t tmp;
        bpvm_pack_info_t* slot = (out && count < max) ? &out[count] : &tmp;
        if (magic != BPVM_PACK_MAGIC) break;                /* basura: cadena corrupta */
        if (parse_header(src, off, slot) != 0) {
            count++;                                        /* cabecera mala: reportar y cortar */
            break;
        }
        slot->crc_ok = (uint8_t) check_content(src, off, slot, verify_content);
        count++;
        off += slot->size_total;                            /* salto fiable (crc_cab OK) */
    }
    if (end_off) *end_off = end;
    return count;
}

int bpvm_pack_scan(const uint8_t* base, uint32_t region_size,
                   bpvm_pack_info_t* out, int max,
                   int verify_content, uint32_t* end_off) {
    bpvm_pack_src_t src;
    bpvm_pack_src_mem(&src, base, region_size);
    return bpvm_pack_scan_src(&src, out, max, verify_content, end_off);
}

int bpvm_pack_entries_src(const bpvm_pack_src_t* src, uint32_t pack_off,
                          bpvm_pack_entry_t* out, int max) {
    bpvm_pack_info_t info;
    if (pack_off + 4 > src->size) return -1;
    uint8_t mg[4];
    if (!src_read(src, pack_off, mg, 4)) return -1;
    if (get_u32(mg) != BPVM_PACK_MAGIC) return -1;
    if (parse_header(src, pack_off, &info) != 0) return -1;
    uint32_t rel = BPVM_PACK_HEADER_SIZE;
    int n = 0, r;
    bpvm_pack_entry_t e;
    while ((r = next_entry(src, pack_off, info.size_total, &rel, &e)) == 1) {
        if (out && n < max) out[n] = e;
        n++;
    }
    return (r < 0) ? -1 : n;
}

int bpvm_pack_entries(const uint8_t* base, uint32_t region_size, uint32_t pack_off,
                      bpvm_pack_entry_t* out, int max) {
    bpvm_pack_src_t src;
    bpvm_pack_src_mem(&src, base, region_size);
    return bpvm_pack_entries_src(&src, pack_off, out, max);
}

int bpvm_pack_find_src(const bpvm_pack_src_t* src,
                       const char* tipo, const char* nombre,
                       bpvm_pack_entry_t* out) {
    int found = 0;
    uint32_t off = 0;
    for (;;) {
        if (off + 4 > src->size) break;
        uint8_t mg[4];
        if (!src_read(src, off, mg, 4)) break;
        if (get_u32(mg) != BPVM_PACK_MAGIC) break;          /* virgen o corrupto = fin */
        bpvm_pack_info_t info;
        if (parse_header(src, off, &info) != 0) break;
        if (info.alive) {
            uint32_t rel = BPVM_PACK_HEADER_SIZE;
            bpvm_pack_entry_t e;
            while (next_entry(src, off, info.size_total, &rel, &e) == 1) {
                if (strcmp(e.tipo, tipo) == 0 && strcmp(e.nombre, nombre) == 0) {
                    if (out) *out = e;                      /* el ÚLTIMO de la cadena gana */
                    found = 1;
                }
            }
        }
        off += info.size_total;
    }
    return found;
}

const uint8_t* bpvm_pack_find(const uint8_t* base, uint32_t region_size,
                              const char* tipo, const char* nombre, uint32_t* len) {
    bpvm_pack_src_t src;
    bpvm_pack_src_mem(&src, base, region_size);
    bpvm_pack_entry_t e;
    if (!bpvm_pack_find_src(&src, tipo, nombre, &e)) {
        if (len) *len = 0;
        return NULL;
    }
    if (len) *len = e.len;
    return bpvm_pack_src_ptr(&src, e.data_off, e.len);
}

int32_t bpvm_pack_add(uint8_t* base, uint32_t region_size,
                      const uint8_t* img, uint32_t img_len) {
    /* La imagen se valida ENTERA antes de tocar la región (magic, verfmt,
     * crc_cab, tamaño exacto, entradas bien formadas y crc_contenido). */
    bpvm_pack_info_t info;
    if (img_len < BPVM_PACK_HEADER_SIZE) return BPVM_PACK_ERR_BADIMG;
    if (get_u32(img) != BPVM_PACK_MAGIC) return BPVM_PACK_ERR_BADIMG;
    bpvm_pack_src_t isrc;
    bpvm_pack_src_mem(&isrc, img, img_len);
    if (parse_header(&isrc, 0, &info) != 0) return BPVM_PACK_ERR_BADIMG;
    if (info.size_total != img_len) return BPVM_PACK_ERR_BADIMG;
    if (!check_content(&isrc, 0, &info, 1)) return BPVM_PACK_ERR_BADIMG;

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
    bpvm_pack_src_t src;
    bpvm_pack_src_mem(&src, base, region_size);
    if (parse_header(&src, pack_off, &info) != 0) return -1;
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
    bpvm_pack_src_t src;
    bpvm_pack_src_mem(&src, base, region_size);
    for (;;) {
        if (off + 4 > region_size) break;
        if (get_u32(base + off) != BPVM_PACK_MAGIC) break;
        bpvm_pack_info_t info;
        if (parse_header(&src, off, &info) != 0) break;
        if (info.alive && strcmp(info.nombre, nombre) == 0)
            target = (int32_t) off;                         /* el ÚLTIMO activo gana */
        off += info.size_total;
    }
    if (target < 0) return -1;
    if (bpvm_pack_remove_at(base, region_size, (uint32_t) target) != 0) return -1;
    return target;
}

/* ── BURN por chunks (ver bpvm_pack.h para el contrato y la disciplina) ── */

int32_t bpvm_pack_burn_begin(const uint8_t* base, uint32_t region_size,
                             const bpvm_pack_flash_t* fl, uint32_t size,
                             bpvm_pack_burn_t* s) {
    s->active = 0;                                          /* abandona sesión previa */
    if (size < BPVM_PACK_HEADER_SIZE) return BPVM_PACK_ERR_BADIMG;
    if (fl->erase_block == 0 || (size % fl->erase_block) != 0)
        return BPVM_PACK_ERR_ALIGN;
    uint32_t end;
    (void) bpvm_pack_scan(base, region_size, NULL, 0, 0, &end);
    if (end == BPVM_PACK_NO_SPACE) return BPVM_PACK_ERR_NOSPACE;   /* cadena corrupta */
    if (size > region_size - end) return BPVM_PACK_ERR_NOSPACE;    /* no cabe */
    /* end es múltiplo del bloque por inducción (todos los size_total lo son). */
    if (fl->erase(fl->user, end, size) != 0) return BPVM_PACK_ERR_IO;
    s->active = 1;
    s->off = end;
    s->total = size;
    s->received = 0;
    return (int32_t) end;
}

int bpvm_pack_burn_data(bpvm_pack_burn_t* s, const bpvm_pack_flash_t* fl,
                        const uint8_t* data, uint32_t len) {
    if (!s->active) return BPVM_PACK_ERR_STATE;
    /* CONTRATO: chunks múltiplos de 16. Con la cabecera de 128 retenida, TODAS
     * las escrituras quedan alineadas y en múltiplos de quadword — obligatorio
     * en flash con ECC (U5: un quadword solo se programa una vez; un chunk
     * arbitrario partiría un quadword entre dos program). total es múltiplo
     * del bloque ⇒ el último chunk también cumple. Regla UNIFORME en las 3
     * familias y el sim: el PC no puede coger vicios que solo rompen en placa. */
    if (len == 0 || (len % 16u) != 0 || len > s->total - s->received) {
        s->active = 0;
        return BPVM_PACK_ERR_STATE;
    }
    uint32_t pos = s->received;
    /* Los primeros 128 B (cabecera) se RETIENEN en RAM — se graban en el END,
     * con el magic al final. El resto va directo a flash según llega. */
    if (pos < BPVM_PACK_HEADER_SIZE) {
        uint32_t h = BPVM_PACK_HEADER_SIZE - pos;
        if (h > len) h = len;
        memcpy(s->hdr + pos, data, h);
        pos += h; data += h; len -= h;
        s->received = pos;
    }
    if (len > 0) {
        if (fl->program(fl->user, s->off + pos, data, len) != 0) {
            s->active = 0;
            return BPVM_PACK_ERR_IO;
        }
        s->received = pos + len;
    }
    return 0;
}

int32_t bpvm_pack_burn_end(const uint8_t* base, uint32_t region_size,
                           bpvm_pack_burn_t* s, const bpvm_pack_flash_t* fl) {
    (void) region_size;
    if (!s->active) return BPVM_PACK_ERR_STATE;
    s->active = 0;                                          /* la sesión muere aquí, pase lo que pase */
    if (s->received != s->total) return BPVM_PACK_ERR_STATE;

    /* 1) cabecera retenida: magic + verfmt + size_total coherente + crc_cab. */
    const uint8_t* h = s->hdr;
    if (get_u32(h + OFF_MAGIC) != BPVM_PACK_MAGIC) return BPVM_PACK_ERR_BADIMG;
    if (get_u16(h + OFF_VERFMT) != BPVM_PACK_VERFMT) return BPVM_PACK_ERR_BADIMG;
    if (get_u32(h + OFF_SIZE_TOTAL) != s->total) return BPVM_PACK_ERR_BADIMG;
    uint16_t crc = bpvm_pack_crc16(BPVM_PACK_CRC16_INIT, h, OFF_FLAGS);
    crc = bpvm_pack_crc16(crc, h + OFF_SIZE_TOTAL, OFF_CRC_CAB - OFF_SIZE_TOTAL);
    if (crc != get_u16(h + OFF_CRC_CAB)) return BPVM_PACK_ERR_BADIMG;

    /* 2) contenido: recorrido + crc_contenido LEYENDO DE LA FLASH (readback). */
    {
        bpvm_pack_src_t vsrc;
        bpvm_pack_src_mem(&vsrc, base, region_size);
        uint32_t rel = BPVM_PACK_HEADER_SIZE;
        int r;
        while ((r = next_entry(&vsrc, s->off, s->total, &rel, NULL)) == 1) { }
        if (r < 0) return BPVM_PACK_ERR_VERIFY;
        uint16_t cc = bpvm_pack_crc16(BPVM_PACK_CRC16_INIT,
                                      base + s->off + BPVM_PACK_HEADER_SIZE,
                                      rel - BPVM_PACK_HEADER_SIZE);
        if (cc != get_u16(h + OFF_CRC_CONT)) return BPVM_PACK_ERR_VERIFY;
    }

    /* 3) ACTIVAR: cabecera [16,128) primero, el quadword del MAGIC al FINAL.
     * Un corte entre medias deja magic=0xFF → el pack nunca existió. */
    if (fl->program(fl->user, s->off + 16, s->hdr + 16, BPVM_PACK_HEADER_SIZE - 16) != 0)
        return BPVM_PACK_ERR_IO;
    if (fl->program(fl->user, s->off, s->hdr, 16) != 0)
        return BPVM_PACK_ERR_IO;
    /* 4) verificación final de la cabecera en flash (readback del quadword). */
    if (memcmp(base + s->off, s->hdr, BPVM_PACK_HEADER_SIZE) != 0)
        return BPVM_PACK_ERR_VERIFY;
    return (int32_t) s->off;
}

int bpvm_pack_format(const bpvm_pack_flash_t* fl, uint32_t region_size) {
    /* La región de particiones viene alineada al sector; el clamp al múltiplo
     * del bloque es red por si acaso (nunca dejar un erase parcial colgando). */
    uint32_t len = region_size - (region_size % fl->erase_block);
    if (len == 0) return BPVM_PACK_ERR_IO;
    return (fl->erase(fl->user, 0, len) == 0) ? 0 : BPVM_PACK_ERR_IO;
}

int bpvm_pack_del(const uint8_t* base, uint32_t region_size,
                  const bpvm_pack_flash_t* fl, uint32_t pack_off,
                  uint8_t* page_buf) {
    bpvm_pack_info_t info;
    uint32_t blk = fl->erase_block;
    if (pack_off + 4 > region_size || (pack_off % blk) != 0) return BPVM_PACK_ERR_BADIMG;
    if (get_u32(base + pack_off) != BPVM_PACK_MAGIC) return BPVM_PACK_ERR_BADIMG;
    bpvm_pack_src_t dsrc;
    bpvm_pack_src_mem(&dsrc, base, region_size);
    if (parse_header(&dsrc, pack_off, &info) != 0) return BPVM_PACK_ERR_BADIMG;
    if (!info.alive) return BPVM_PACK_ERR_STATE;

    /* RMW de la 1ª página: copiar, tumbar el bit ALIVE, borrar, regrabar.
     * crc_cab NO cubre flags (a propósito) → la cabecera regrabada sigue
     * siendo válida byte a byte salvo el bit. */
    memcpy(page_buf, base + pack_off, blk);
    put_u16(page_buf + OFF_FLAGS, (uint16_t) (info.flags & ~BPVM_PACK_ALIVE_BIT));
    if (fl->erase(fl->user, pack_off, blk) != 0) return BPVM_PACK_ERR_IO;
    if (fl->program(fl->user, pack_off, page_buf, blk) != 0) return BPVM_PACK_ERR_IO;
    if (memcmp(base + pack_off, page_buf, blk) != 0) return BPVM_PACK_ERR_VERIFY;
    return 0;
}
