/*
 * loader.c — parsea un .mod (file path o buffer) y vuelca data + code en
 * el memory[] de la VM. La spec canónica está en docs/MOD_FORMAT.md.
 *
 * Dos entry points:
 *   - bpvm_loader_load(vm, path)        — lee fichero, pasa a la versión buffer.
 *   - bpvm_loader_load_buffer(vm, data, size, name_hint) — parsea un blob
 *     que ya está en RAM (uso embebido: .mod compilado dentro de la imagen).
 *
 * F3+: registra exports en la symbol table global. Class fixups
 * cross-module se aplican luego en bpvm_link_all (link.c).
 */

#include "bpvm_internal.h"
#include "mdn_loader.h"   /* V6/N1.4: cargar el bloque nativo embebido */
#include "mdn_format.h"
#include "bpvm_examen.h"   /* V7/H8 E1: el examen del .mod, con la misma lectura de cabecera */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */

/* ---------- Cursor sobre el .mod: RAM o STREAM ----------------------
 *
 * El parser SIEMPRE avanza hacia delante, así que da igual de dónde salgan los
 * bytes. Dos fuentes:
 *   - `base != NULL`  → blob ya en RAM/flash (embebido en la imagen, XIP, host).
 *   - `rd != NULL`    → lectura por trozos (H11): el .mod se queda en el FS y
 *                       los bloques gordos aterrizan DIRECTAMENTE en su sitio
 *                       final dentro de memory[]. Sin fichero completo en RAM,
 *                       que es lo que sostenía el scratch de 128 KB del micro.
 * Nada más del loader cambia: el parseo es idéntico. */
typedef struct {
    const uint8_t*   base;    /* fuente RAM, o NULL si va por stream */
    bpvm_read_at_fn  rd;      /* fuente STREAM, o NULL si va por RAM */
    void*            rd_user;
    size_t           size;
    size_t           pos;
    int              err;     /* 1 si se intentó leer fuera de rango. */
} buf_cursor_t;

static int bc_read(buf_cursor_t* c, void* dst, size_t n) {
    if (c->err) return -1;
    if (c->pos + n > c->size) { c->err = 1; return -1; }
    if (dst && n > 0) {
        if (c->base) {
            memcpy(dst, c->base + c->pos, n);
        } else if (c->rd(c->rd_user, (uint32_t) c->pos, (uint8_t*) dst, (uint32_t) n)
                       != (long) n) {
            c->err = 1; return -1;
        }
    }
    c->pos += n;
    return 0;
}

static int bc_skip(buf_cursor_t* c, size_t n) {
    return bc_read(c, NULL, n);
}

static int bc_read_be32(buf_cursor_t* c, uint32_t* out) {
    uint8_t b[4];
    if (bc_read(c, b, 4) != 0) return -1;
    *out = bpvm_read_u32_be(b);
    return 0;
}

/* Lee un Java writeUTF (u16 length + bytes UTF-8). Trunca a dst_size-1.
 * Siempre consume `length` bytes del cursor. */
static int bc_read_writeutf(buf_cursor_t* c, char* dst, size_t dst_size) {
    uint8_t lenb[2];
    if (bc_read(c, lenb, 2) != 0) return -1;
    uint16_t len = bpvm_read_u16_be(lenb);
    if (len == 0) {
        if (dst_size > 0) dst[0] = '\0';
        return 0;
    }
    size_t to_copy = (len < dst_size - 1) ? len : dst_size - 1;
    if (bc_read(c, dst, to_copy) != 0) return -1;
    dst[to_copy] = '\0';
    if (to_copy < len) {
        if (bc_skip(c, len - to_copy) != 0) return -1;
    }
    return 0;
}

/* Quita el sufijo ".mod" y el directorio del path para obtener el
 * nombre lógico del módulo. */
static void derive_module_name(const char* path, char* dst, size_t dst_size) {
    const char* base = path;
    for (const char* p = path; *p; p++) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    size_t n = strlen(base);
    if (n >= 4 && strcmp(base + n - 4, ".mod") == 0) n -= 4;
    if (n >= dst_size) n = dst_size - 1;
    memcpy(dst, base, n);
    dst[n] = '\0';
}

/* V7/H8 E1 — LA CABECERA DEL .mod, leída por UNA función para cargar y para examinar (el LIST): qué MAGIC carga
 * esta VM y cuánto ocupa la cabecera de cada versión se decide aquí y en ningún otro sitio. */
typedef struct {
    uint32_t magic, data_size, imports_size, exports_size, code_size, library_size;
    uint32_t interface_size, native_size, gen, iface_crc;
    int32_t  main_offset;
    int      fmt;          /* 6..9 */
    uint32_t cab_bytes;    /* lo que ocupa en el fichero: 32 (v6), 36 (v7) o 44 (v8, v9) */
} mod_cab_t;

static bpvm_status_t mod_cab_lee(buf_cursor_t* c, mod_cab_t* h) {
    memset(h, 0, sizeof *h);
    if (bc_read_be32(c, &h->magic) != 0)                 return BPVM_ERR_IO;
    /* #284 — GATE DE ABI. La versión del formato ES la declaración de ABI:
     * v6 garantiza refs de 8 bytes (nació DESPUÉS del ensanchado 4->8B), v5 es
     * AMBIGUO (puede ser de la era 4B y corromper memoria en silencio). Lo que
     * no se pueda garantizar se RECHAZA aquí, en la carga, con un error claro
     * -- nunca se ejecuta a ciegas. */
    if (h->magic == BPVM_MAGIC)                          return BPVM_ERR_ABI_MOD_V5;
    if (h->magic != BPVM_MAGIC_V6 && h->magic != BPVM_MAGIC_V7 && h->magic != BPVM_MAGIC_V8
        && h->magic != BPVM_MAGIC_V9)                    return BPVM_ERR_BAD_MAGIC;
    int is_v6 = 1;                       /* v6, v7 y v8 llevan interfaceSize */
    int is_v9 = (h->magic == BPVM_MAGIC_V9);   /* V7/H2 (#523): enlace tardio (refs 3.3 + CLASES 4.5) */
    int is_v8 = (h->magic == BPVM_MAGIC_V8) || is_v9;   /* V7/H2: gen + ifaceCrc + subtabla deps */
    int is_v7 = (h->magic == BPVM_MAGIC_V7) || is_v8;
    if (bc_read_be32(c, &h->data_size) != 0)             return BPVM_ERR_IO;
    {
        uint32_t mo;
        if (bc_read_be32(c, &mo) != 0)                   return BPVM_ERR_IO;
        h->main_offset = (int32_t) mo;
    }
    if (bc_read_be32(c, &h->imports_size) != 0)          return BPVM_ERR_IO;
    if (bc_read_be32(c, &h->exports_size) != 0)          return BPVM_ERR_IO;
    if (bc_read_be32(c, &h->code_size) != 0)             return BPVM_ERR_IO;
    if (bc_read_be32(c, &h->library_size) != 0)          return BPVM_ERR_IO;
    if (is_v6 && bc_read_be32(c, &h->interface_size) != 0) return BPVM_ERR_IO;
    if (is_v7 && bc_read_be32(c, &h->native_size) != 0)    return BPVM_ERR_IO;
    if (is_v8 && (bc_read_be32(c, &h->gen) != 0 || bc_read_be32(c, &h->iface_crc) != 0))
                                                         return BPVM_ERR_IO;
    h->fmt       = is_v9 ? 9 : (is_v8 ? 8 : (is_v7 ? 7 : 6));
    h->cab_bytes = 28u + 4u + (is_v7 ? 4u : 0u) + (is_v8 ? 8u : 0u);
    return BPVM_OK;
}

/* V7/H8 E1 — ¿CARGARÍA ESTA VM ESTE .mod? Con la cabecera (la misma lectura que la carga) y el tamaño del fichero,
 * que acaba justo donde acaba `code` (MOD_FORMAT §1: sin trailer ni checksum): más corto es truncado, más largo,
 * roto. El contenido no se mira: el formato no lleva checksum. */
int bpvm_mod_examina(const uint8_t* cab, size_t n, uint32_t tam, char* motivo, size_t cap) {
    buf_cursor_t c = { cab, NULL, NULL, n < (size_t) tam ? n : (size_t) tam, 0, 0 };
    mod_cab_t h;
    bpvm_status_t st = mod_cab_lee(&c, &h);
    if (st == BPVM_ERR_ABI_MOD_V5) {
        snprintf(motivo, cap, "formato no válido: MOD5, de antes del ensanchado de las referencias (hay que recompilarlo)");
        return 1;
    }
    if (st == BPVM_ERR_BAD_MAGIC) { snprintf(motivo, cap, "formato no válido: no es un .mod"); return 1; }
    if (st != BPVM_OK)            { snprintf(motivo, cap, "dañado: más corto que su cabecera"); return 1; }
    uint64_t dice = (uint64_t) h.cab_bytes + h.library_size + h.imports_size + h.exports_size
                  + h.interface_size + h.native_size + h.data_size + h.code_size;
    if (dice != (uint64_t) tam) {
        snprintf(motivo, cap, "dañado: la cabecera dice %lu B y el fichero tiene %lu",
                 dice > 0xFFFFFFFFull ? 0xFFFFFFFFul : (unsigned long) dice, (unsigned long) tam);
        return 1;
    }
    return 0;
}

/* Cuerpo común de la carga. `xip` (H3.c): el código NO se copia a RAM — queda
 * direccionado en sitio (los bytes de `data` deben ser persistentes: la región
 * de packs montada). CS queda anclado en RAM como siempre; solo cb difiere. */
/* V7 #651 (8-oct) — un nombre al bloque de nombres del módulo (mod->nombres): seguido del anterior, terminado en 0.
 * Cabe siempre: el bloque mide lo que la sección de imports, donde cada nombre ocupaba su longitud y 2 de prefijo.
 * Devuelve su DESPLAZAMIENTO, disfrazado de puntero hasta que el bloque se encoja y se pongan los de verdad
 * (nombres_a_punteros): encogerlo puede moverlo. */
static char* nombre_al_bloque(bpvm_module_t* mod, size_t* usado, const char* s) {
    size_t at = *usado, n = strlen(s) + 1u;
    memcpy(mod->nombres + at, s, n);
    *usado += n;
    return (char*) (uintptr_t) at;
}

/* El bloque, a su medida —se pidió lo que mide la sección, y en ella hay más que nombres: los prefijos, los fromPath
 * que no se guardan, las generaciones—, y los desplazamientos, a punteros. Si no puede encogerse, se queda el de
 * antes: vale igual. */
static void nombres_a_punteros(bpvm_module_t* mod, size_t usado) {
    if (!mod->nombres) return;
    char* a = (char*) bpvm_realloc(mod->nombres, usado ? usado : 1u);
    if (a) mod->nombres = a;
    for (int i = 0; i < mod->import_count; i++) mod->imports[i] = mod->nombres + (uintptr_t) mod->imports[i];
    for (int i = 0; i < mod->dep_count; i++) mod->dep_names[i] = mod->nombres + (uintptr_t) mod->dep_names[i];
    for (int i = 0; i < mod->ref_count; i++) {
        mod->refs[i].clase = mod->nombres + (uintptr_t) mod->refs[i].clase;
        mod->refs[i].clave = mod->nombres + (uintptr_t) mod->refs[i].clave;
    }
}

/* #685 (11-oct) — un nombre exportado se copia ENTERO o la carga falla con el motivo. Cabía en 128 B y el resto se
 * cortaba en silencio: una función pública de más de 127 caracteres —dos parámetros de una clase de nombre largo— no
 * enlazaba desde otro módulo («presente pero no exporta…»), mientras miVM la ejecutaba. El compilador no deja pasar uno
 * de más de BPVM_FIRMA_MAX (`ModWriter.exigirExportCorto`): uno más largo viene de un .mod que no es suyo. */
static bpvm_status_t export_largo(bpvm_t* vm, const bpvm_module_t* mod, unsigned nlen) {
    snprintf(vm->load_error, sizeof vm->load_error, "'%s' exporta un nombre de %u caracteres (el maximo es %d): recompila '%s'",
             mod->name, nlen, BPVM_FIRMA_MAX, mod->name);
    return BPVM_ERR_BAD_HEADER;
}

/* V8 #828 — sin memoria al cargar: de cuál. Las tablas del módulo (sus nombres, lo que importa, sus dependencias y sus
 * referencias, los arreglos de sus clases) las pide al sistema (malloc); su bloque —la tabla externa, los datos y, si
 * no corre en sitio, el código—, al de la VM. Con el genérico del estado («memoria del buffer insuficiente») no se
 * sabía cuál, y en una placa sin PSRAM las dos se reparten la misma RAM: lo que el bloque no coge es del sistema. */
static bpvm_status_t sin_sistema(bpvm_t* vm, const bpvm_module_t* mod) {
    snprintf(vm->load_error, sizeof vm->load_error, "'%s': no hay memoria del sistema (malloc) para sus tablas (en una "
             "placa sin PSRAM, un vmheap= menor en el ENV le deja mas)", mod->name);
    return BPVM_ERR_OOM;
}

static bpvm_status_t sin_bloque(bpvm_t* vm, const bpvm_module_t* mod) {
    snprintf(vm->load_error, sizeof vm->load_error, "'%s' no cabe en el bloque de la VM", mod->name);
    return BPVM_ERR_OOM;
}

static bpvm_status_t load_buffer_impl(bpvm_t* vm, const uint8_t* data,
                                      bpvm_read_at_fn rd, void* rd_user,
                                      size_t size, const char* name_hint, int xip) {
    if (vm->module_count >= BPVM_MAX_MODULES) return BPVM_ERR_OOM;
    if (!data && !rd) return BPVM_ERR_IO;
    if (size < 28) return BPVM_ERR_BAD_HEADER;
    if (xip && !data) return BPVM_ERR_IO;   /* XIP ejecuta EN SITIO: exige blob mapeado */

    buf_cursor_t c = { data, rd, rd_user, size, 0, 0 };

    /* --- Header: por mod_cab_lee, la MISMA lectura que el examen del LIST (V7/H8 E1) --- */
    mod_cab_t hc;
    {
        bpvm_status_t st = mod_cab_lee(&c, &hc);
        if (st != BPVM_OK) return st;
    }
    uint32_t data_size = hc.data_size, imports_size = hc.imports_size, exports_size = hc.exports_size;
    uint32_t code_size = hc.code_size, library_size = hc.library_size;
    uint32_t interface_size = hc.interface_size, native_size = hc.native_size;
    uint32_t gen = hc.gen, iface_crc = hc.iface_crc;
    int32_t  main_offset = hc.main_offset;
    int is_v9 = (hc.fmt == 9), is_v8 = (hc.fmt >= 8), is_v7 = (hc.fmt >= 7);

    bpvm_module_t* mod = &vm->modules[vm->module_count];
    memset(mod, 0, sizeof(*mod));
    mod->fmt = (uint8_t) (is_v9 ? 9 : (is_v8 ? 8 : (is_v7 ? 7 : 6)));
    mod->gen = gen;
    mod->iface_crc = iface_crc;
    /* Nombre lógico: si tenemos hint úsalo, si no "embedded<idx>". */
    if (name_hint && name_hint[0]) {
        derive_module_name(name_hint, mod->name, sizeof(mod->name));
    } else {
        snprintf(mod->name, sizeof(mod->name), "embedded%d", vm->module_count);
    }
    mod->main_offset = main_offset;
    mod->data_size   = data_size;
    mod->code_size   = code_size;

    /* --- Library (sin length prefix; raw UTF-8) --- */
    if (library_size > 0) {
        size_t n = library_size < sizeof(mod->library) - 1
                   ? library_size : sizeof(mod->library) - 1;
        if (bc_read(&c, mod->library, n) != 0)         return BPVM_ERR_IO;
        mod->library[n] = '\0';
        if (n < library_size && bc_skip(&c, library_size - n) != 0) {
            return BPVM_ERR_IO;
        }
        /* #518(b) — el nombre lógico NO lleva la library: el fichero se llama
         * `acme.LibHdr.mod` y el hint era el basename entero, así que `name`
         * salía «acme.LibHdr», el prefijo de exports «acme.acme.LibHdr.» y
         * module_loaded(lib, mod) no casaba nunca: la VM-C recargaba el mismo
         * módulo hasta agotar BPVM_MAX_MODULES. miVM ya lo recortaba. */
        size_t ll = strlen(mod->library);
        if (ll > 0 && strncmp(mod->name, mod->library, ll) == 0 && mod->name[ll] == '.') {
            memmove(mod->name, mod->name + ll + 1, strlen(mod->name + ll + 1) + 1);
        }
    }

    /* --- Imports (count + (name UTF, fromPath UTF) * count) --- */
    size_t imports_ini = c.pos;
    /* V7 #651 (8-oct) — LOS NOMBRES, EN UN BLOQUE: los de lo que importa, de sus dependencias y de sus referencias.
     * Antes, un bloque por nombre —~800 con la GUI; en la placa, cada uno con la cabecera del guardián de #339—. */
    size_t nombres_usado = 0;
    if (imports_size > 0) {
        mod->nombres = (char*) bpvm_malloc((size_t) imports_size + 1u);
        if (!mod->nombres) return sin_sistema(vm, mod);
    }
    uint32_t ext_count;
    if (bc_read_be32(&c, &ext_count) != 0)             return BPVM_ERR_IO;
    mod->ext_count = ext_count;
    mod->import_count = (int) ext_count;
    if (ext_count > 0) {
        mod->imports = (char**) bpvm_calloc(ext_count, sizeof(char*));
        if (!mod->imports) return sin_sistema(vm, mod);
        char tmp_from[256];
        for (uint32_t i = 0; i < ext_count; i++) {
            char tmp_name[256];
            if (bc_read_writeutf(&c, tmp_name, sizeof(tmp_name)) != 0) {
                return BPVM_ERR_IO;
            }
            if (bc_read_writeutf(&c, tmp_from, sizeof(tmp_from)) != 0) {
                return BPVM_ERR_IO;
            }
            mod->imports[i] = nombre_al_bloque(mod, &nombres_usado, tmp_name);
        }
    }
    /* V7/H2 (MOD8, #500) — la subtabla `deps` va DETRÁS de las entradas, dentro
     * de la sección: count + (modulo UTF, fromPath UTF, gen be32, ifaceCrc be32)*.
     * Se detecta por los bytes que quedan de imports_size (aditivo), y lo que
     * sobre después se salta por tamaño: así ninguna versión futura de esta
     * sección corre la de exports. */
    if (is_v8 && c.pos - imports_ini + 4 <= imports_size) {
        uint32_t nd;
        if (bc_read_be32(&c, &nd) != 0)                return BPVM_ERR_IO;
        if (nd > 0) {
            mod->dep_names   = (char**) bpvm_calloc(nd, sizeof(char*));
            mod->dep_gen     = (uint32_t*) bpvm_calloc(nd, sizeof(uint32_t));
            mod->dep_crc     = (uint32_t*) bpvm_calloc(nd, sizeof(uint32_t));
            mod->dep_flags   = (uint32_t*) bpvm_calloc(nd, sizeof(uint32_t));
            mod->dep_avisado = (uint8_t*)  bpvm_calloc(nd, sizeof(uint8_t));
            if (!mod->dep_names || !mod->dep_gen || !mod->dep_crc || !mod->dep_flags || !mod->dep_avisado)
                return sin_sistema(vm, mod);
            for (uint32_t i = 0; i < nd; i++) {
                char tmp_mod[160], tmp_from[256];
                if (bc_read_writeutf(&c, tmp_mod, sizeof(tmp_mod)) != 0)   return BPVM_ERR_IO;
                if (bc_read_writeutf(&c, tmp_from, sizeof(tmp_from)) != 0) return BPVM_ERR_IO;
                if (bc_read_be32(&c, &mod->dep_gen[i]) != 0)               return BPVM_ERR_IO;
                if (bc_read_be32(&c, &mod->dep_crc[i]) != 0)               return BPVM_ERR_IO;
                if (bc_read_be32(&c, &mod->dep_flags[i]) != 0)             return BPVM_ERR_IO;
                mod->dep_names[i] = nombre_al_bloque(mod, &nombres_usado, tmp_mod);
                mod->dep_count = (int) (i + 1);
            }
        }
    }
    /* V7/H2 (#523, MOD9) — la subtabla `refs` (3.3): count u16 + (kind u8, depIdx i16,
     * clase UTF, clave UTF)*. La ref i vivira en ext[ext_count + i]. */
    mod->ext_count31 = ext_count;
    if (is_v9 && c.pos - imports_ini + 2 <= imports_size) {
        uint8_t b2[2];
        if (bc_read(&c, b2, 2) != 0) return BPVM_ERR_IO;
        uint32_t nr = ((uint32_t) b2[0] << 8) | b2[1];
        if (nr > 0) {
            mod->refs = (bpvm_ref_t*) bpvm_calloc(nr, sizeof(bpvm_ref_t));
            if (!mod->refs) return sin_sistema(vm, mod);
            for (uint32_t i = 0; i < nr; i++) {
                uint8_t k[3]; char tmp_c[160], tmp_k[256];
                if (bc_read(&c, k, 3) != 0)                              return BPVM_ERR_IO;
                if (bc_read_writeutf(&c, tmp_c, sizeof(tmp_c)) != 0)     return BPVM_ERR_IO;
                if (bc_read_writeutf(&c, tmp_k, sizeof(tmp_k)) != 0)     return BPVM_ERR_IO;
                mod->refs[i].kind    = k[0];
                mod->refs[i].dep_idx = (int16_t) (((uint16_t) k[1] << 8) | k[2]);
                mod->refs[i].clase   = nombre_al_bloque(mod, &nombres_usado, tmp_c);
                mod->refs[i].clave   = nombre_al_bloque(mod, &nombres_usado, tmp_k);
                mod->ref_count = (int) (i + 1);
            }
        }
    }
    nombres_a_punteros(mod, nombres_usado);   /* V7 #651: el bloque a su medida, y ya con punteros */
    if (c.pos - imports_ini < imports_size) {
        if (bc_skip(&c, imports_size - (c.pos - imports_ini)) != 0) return BPVM_ERR_IO;
    }

    /* --- Layout en memory[] ---
     *    moduleBase ─┬─ ext-table (ext_count * 4 bytes, zeroed)
     *                ├─ data block (data_size bytes)
     *                └─ code block (code_size bytes)   [XIP: NO ocupa RAM]
     */
    /* V7/H12 (#660) — el bloque de cada módulo, ALINEADO a 4: sus globales están en huecos de 4 desde CS (GC-2), y con
     * la base alineada quedan alineadas en la memoria, que es lo que pide escribirlas de una vez (bpvm_write_u32_be).
     * Sin esto, el de un segundo módulo empezaba donde acabó el anterior (+64, abajo), cualquiera que fuese su tamaño.
     * Igual en miVM (ModuleManager.loadModuleToMemory): las dos, la misma memoria. */
    uint32_t module_base = (vm->next_free_address + 3u) & ~3u;
    uint32_t ext_table_size = (ext_count + (uint32_t) mod->ref_count) * BPVM_EXT_ENTRY_SIZE;   /* V7/H2 (#523): + las refs */
    uint32_t data_start  = module_base + ext_table_size;
    uint32_t code_start  = data_start + data_size;      /* = CS (ancla, SIEMPRE RAM) */
    uint32_t end_addr    = xip ? code_start : code_start + code_size;

    if (end_addr > vm->heap_top) {
        return sin_bloque(vm, mod);
    }

    if (ext_table_size > 0) {
        memset(vm->memory + module_base, 0, ext_table_size);
        /* V7/H2 (#523) — las entradas de las refs nacen SIN RESOLVER (0xFFFFFFFF: la
         * ranura 0 es legitima, toString); el enlazador las rellena o rechaza. */
        if (mod->ref_count > 0)
            memset(vm->memory + module_base + ext_count * BPVM_EXT_ENTRY_SIZE, 0xFF,
                   (size_t) mod->ref_count * BPVM_EXT_ENTRY_SIZE);
    }

    /* Sección exports: leerla completa a un buffer (puntero al cursor mismo)
     * para poder detectar las sub-secciones opcionales sin reinventar el
     * scanning. Como ya es buffer-backed, sólo capturamos el offset y la
     * tamaño y avanzamos el cursor. */
    const uint8_t* exp_buf = NULL;
    if (exports_size > 0) {
        if (c.pos + exports_size > c.size) return BPVM_ERR_IO;
        if (c.base) {
            exp_buf = c.base + c.pos;      /* RAM: cero copia, como siempre */
            c.pos += exports_size;
        } else {
            /* STREAM: es la ÚNICA sección que el parser mira por offsets, así
             * que tiene que estar residente. La montamos en la arena libre por
             * encima del módulo — memoria que nadie ha reclamado todavía
             * (next_free_address no llega ahí hasta el final) y que se olvida
             * sola al volver. Cero buffers estáticos. */
            if (end_addr + exports_size > vm->heap_top) return sin_bloque(vm, mod);
            if (bc_read(&c, vm->memory + end_addr, exports_size) != 0) return BPVM_ERR_IO;
            exp_buf = vm->memory + end_addr;
        }
    }

    /* H6.a — la sección interface va entre exports y data; la VM ejecuta sin
     * ella (es metadato de compilación). La saltamos íntegra. */
    /* [V6/N1.4] La seccion `native` va JUSTO DESPUES de `interface`. De momento
     * se salta: lo que se cierra hoy es el FORMATO, para que el registro de
     * thunks desde aqui pueda apoyarse en el. Saltarla mal desplazaria `data` y
     * `code`, que es como se corrompe un modulo en silencio. */
    if (interface_size > 0) {
        if (bc_skip(&c, interface_size) != 0) return BPVM_ERR_IO;
    }
    uint32_t native_off = 0;   /* offset de la seccion dentro de `data` (el buffer del .mod) */
    if (native_size > 0) {
        native_off = (uint32_t) c.pos;
        if (bc_skip(&c, native_size) != 0) return BPVM_ERR_IO;
    }

    /* Data block: lo copiamos en memory[] (mutable: SIEMPRE a RAM). */
    if (data_size > 0) {
        if (bc_read(&c, vm->memory + data_start, data_size) != 0) return BPVM_ERR_IO;
    }

    /* Code block: RAM → copia como siempre. XIP → NO se copia: cb apunta a los
     * bytes del .mod dentro de la región montada. La dirección VIRTUAL se elige
     * tal que `vm->memory + cb == puntero real` — en host la región vive DENTRO
     * del buffer (resta en rango); en micro de 32 bits la resta envuelve módulo
     * 2^32 y `mem + pc` cae en la flash. El fetch del intérprete no se toca. */
    uint32_t mod_cb = code_start;
    if (code_size > 0) {
        if (xip) {
            if (c.pos + code_size > c.size) return BPVM_ERR_IO;
            mod_cb = (uint32_t)((uintptr_t)(c.base + c.pos) - (uintptr_t) vm->memory);
            /* Ventana de PC válido del guardián del bucle (unión de rangos XIP). */
            if (mod_cb < vm->xip_lo) vm->xip_lo = mod_cb;
            if (mod_cb + code_size > vm->xip_hi) vm->xip_hi = mod_cb + code_size;
            c.pos += code_size;
        } else if (bc_read(&c, vm->memory + code_start, code_size) != 0) {
            return BPVM_ERR_IO;
        }
    }

    mod->module_base    = module_base;
    mod->ext_table_addr = module_base;
    mod->data_start     = data_start;
    mod->code_start     = code_start;
    mod->end_addr       = end_addr;
    mod->cb             = mod_cb;
    /* V5/H4 — de dónde vino. Se marca AQUÍ, en el único sitio que lo sabe de
     * primera mano, para que nadie tenga que adivinarlo después. */
    mod->en_pack        = (uint8_t) (xip ? 1 : 0);

    /* --- Procesar la sección exports ---
     *
     * Sub-secciones según docs/MOD_FORMAT.md §4:
     *   4.1 funcs:        count:i32  (name:UTF, relOffset:i32)*
     *   4.2 dataExports:  count:i32  (name:UTF, csOffset:i32)*       (opcional)
     *   4.3 classFixups:  count:i32  (childName:UTF, childCsOff:i32, parentQName:UTF)*  (opcional)
     */
    size_t exp_off = 0;
    char export_prefix[160];
    if (mod->library[0]) snprintf(export_prefix, sizeof(export_prefix),
                                   "%s.%s.", mod->library, mod->name);
    else                 snprintf(export_prefix, sizeof(export_prefix), "%s.",
                                   mod->name);

    if (exports_size >= 4) {
        uint32_t fcount = bpvm_read_u32_be(exp_buf + exp_off); exp_off += 4;
        for (uint32_t i = 0; i < fcount; i++) {
            if (exp_off + 2 > exports_size) break;
            uint16_t nlen = bpvm_read_u16_be(exp_buf + exp_off); exp_off += 2;
            if (exp_off + nlen + 4 > exports_size) break;
            if (nlen > BPVM_FIRMA_MAX) return export_largo(vm, mod, nlen);   /* #685: entero o nada */
            char name[BPVM_FIRMA_MAX + 1];
            memcpy(name, exp_buf + exp_off, nlen); name[nlen] = '\0';
            exp_off += nlen;
            int32_t rel = bpvm_read_i32_be(exp_buf + exp_off); exp_off += 4;
            uint32_t abs = mod_cb + (uint32_t) rel;   /* función → base de CÓDIGO (XIP: flash) */
            char qual[320];
            snprintf(qual, sizeof(qual), "%s%s", export_prefix, name);
            /* #639 — sin memoria para la tabla, la carga falla y lo dice (load_error): seguir sin el simbolo acababa en
             * un enlace que acusaba a una version vieja. */
            bpvm_status_t rsim = bpvm_link_register_symbol(vm, qual, abs);
            if (rsim == BPVM_OK && mod->library[0]) {
                char short_q[320];
                snprintf(short_q, sizeof(short_q), "%s.%s", mod->name, name);
                if (bpvm_link_lookup(vm, short_q) == 0) {
                    rsim = bpvm_link_register_symbol(vm, short_q, abs);
                }
            }
            if (rsim != BPVM_OK) return rsim;
        }

        /* 4.2 data exports opcional. */
        if (exp_off + 4 <= exports_size) {
            uint32_t dcount = bpvm_read_u32_be(exp_buf + exp_off); exp_off += 4;
            for (uint32_t i = 0; i < dcount; i++) {
                if (exp_off + 2 > exports_size) break;
                uint16_t nlen = bpvm_read_u16_be(exp_buf + exp_off); exp_off += 2;
                if (exp_off + nlen + 4 > exports_size) break;
                if (nlen > BPVM_FIRMA_MAX) return export_largo(vm, mod, nlen);   /* #685: entero o nada */
                char name[BPVM_FIRMA_MAX + 1];
                memcpy(name, exp_buf + exp_off, nlen); name[nlen] = '\0';
                exp_off += nlen;
                int32_t cs_off = bpvm_read_i32_be(exp_buf + exp_off); exp_off += 4;
                /* data export → ancla CS (RAM): descriptors/constantes viven en data. */
                uint32_t abs = (uint32_t)((int32_t) code_start + cs_off);
                char qual[320];
                snprintf(qual, sizeof(qual), "%s%s", export_prefix, name);
                bpvm_status_t rsim = bpvm_link_register_symbol(vm, qual, abs);   /* #639: con su error */
                if (rsim == BPVM_OK && mod->library[0]) {
                    char short_q[320];
                    snprintf(short_q, sizeof(short_q), "%s.%s", mod->name, name);
                    if (bpvm_link_lookup(vm, short_q) == 0) {
                        rsim = bpvm_link_register_symbol(vm, short_q, abs);
                    }
                }
                if (rsim != BPVM_OK) return rsim;
            }

            /* V7/H2 (#523, MOD9) — detras de 4.2 viene la subtabla CLASES (4.5), y no hay
             * 4.3/4.4. Se retiene tal cual: XIP → puntero a la flash (0 B de RAM); RAM o
             * stream → copia detras del modulo (el buffer de exports es temporal). */
            if (is_v9) {
                if (exp_off < exports_size) {
                    uint32_t len = exports_size - (uint32_t) exp_off;
                    if (xip) {
                        mod->clases = exp_buf + exp_off;
                    } else {
                        uint32_t dst = (end_addr + 3u) & ~3u;
                        if (dst + len > vm->heap_top) return sin_bloque(vm, mod);
                        memmove(vm->memory + dst, exp_buf + exp_off, len);
                        mod->clases = vm->memory + dst;
                        end_addr = dst + len;
                        mod->end_addr = end_addr;   /* la copia es del modulo: el siguiente empieza detras */
                    }
                    mod->clases_len = len;
                    { int vr = bpvm_link_validar_clases(vm, mod); if (vr != 0) return vr; }   /* V10: limites antes de leerla */
                }
                exp_off = exports_size;
            }
            /* 4.3 class fixups opcional. */
            if (!is_v9 && exp_off + 4 <= exports_size) {
                uint32_t fxcount = bpvm_read_u32_be(exp_buf + exp_off); exp_off += 4;
                if (fxcount > 0) {
                    mod->class_fixups = (bpvm_class_fixup_t*) bpvm_calloc(fxcount,
                                          sizeof(bpvm_class_fixup_t));
                    if (!mod->class_fixups) return sin_sistema(vm, mod);
                    for (uint32_t i = 0; i < fxcount; i++) {
                        if (exp_off + 2 > exports_size) break;
                        uint16_t nlen = bpvm_read_u16_be(exp_buf + exp_off); exp_off += 2;
                        char cname[64]; size_t nl = nlen < sizeof(cname) - 1
                                                    ? nlen : sizeof(cname) - 1;
                        memcpy(cname, exp_buf + exp_off, nl); cname[nl] = '\0';
                        exp_off += nlen;
                        int32_t cs_off = bpvm_read_i32_be(exp_buf + exp_off); exp_off += 4;
                        uint16_t plen = bpvm_read_u16_be(exp_buf + exp_off); exp_off += 2;
                        char pqual[128]; size_t pl = plen < sizeof(pqual) - 1
                                                     ? plen : sizeof(pqual) - 1;
                        memcpy(pqual, exp_buf + exp_off, pl); pqual[pl] = '\0';
                        exp_off += plen;
                        bpvm_class_fixup_t* fx = &mod->class_fixups[mod->class_fixup_count++];
                        size_t cnl = strlen(cname);
                        if (cnl >= sizeof(fx->child_class_name)) cnl = sizeof(fx->child_class_name) - 1;
                        memcpy(fx->child_class_name, cname, cnl);
                        fx->child_class_name[cnl] = '\0';
                        fx->child_cs_off = cs_off;
                        size_t pnl = strlen(pqual);
                        if (pnl >= sizeof(fx->parent_qualified)) pnl = sizeof(fx->parent_qualified) - 1;
                        memcpy(fx->parent_qualified, pqual, pnl);
                        fx->parent_qualified[pnl] = '\0';
                    }
                }
            }
            /* 4.4 eh-class fixups opcional (BUG-2 — catch cross-module). */
            if (!is_v9 && exp_off + 4 <= exports_size) {
                uint32_t ehcount = bpvm_read_u32_be(exp_buf + exp_off); exp_off += 4;
                if (ehcount > 0) {
                    mod->eh_class_fixups = (bpvm_eh_class_fixup_t*) bpvm_calloc(ehcount,
                                          sizeof(bpvm_eh_class_fixup_t));
                    if (!mod->eh_class_fixups) return sin_sistema(vm, mod);
                    for (uint32_t i = 0; i < ehcount; i++) {
                        if (exp_off + 4 > exports_size) break;
                        int32_t code_off = bpvm_read_i32_be(exp_buf + exp_off); exp_off += 4;
                        uint16_t plen = bpvm_read_u16_be(exp_buf + exp_off); exp_off += 2;
                        char pqual[128]; size_t pl = plen < sizeof(pqual) - 1
                                                     ? plen : sizeof(pqual) - 1;
                        memcpy(pqual, exp_buf + exp_off, pl); pqual[pl] = '\0';
                        exp_off += plen;
                        bpvm_eh_class_fixup_t* fx = &mod->eh_class_fixups[mod->eh_class_fixup_count++];
                        fx->code_off = code_off;
                        size_t pnl = strlen(pqual);
                        if (pnl >= sizeof(fx->parent_qualified)) pnl = sizeof(fx->parent_qualified) - 1;
                        memcpy(fx->parent_qualified, pqual, pnl);
                        fx->parent_qualified[pnl] = '\0';
                    }
                }
            }
        }
    }

    vm->next_free_address = end_addr + 64; /* margen entre módulos */
    vm->heap_start        = vm->next_free_address;
    vm->heap_next         = vm->heap_start;
    /* H3 (V2): init del GC con free-list + umbral, con el heap_start real. */
    bpvm_huecos_vaciar(vm);   /* V8 #718: las listas de huecos, vacías */
    vm->last_gc_heap_next = vm->heap_next;
    vm->alloc_since_gc    = 0u;   /* #357 */
    vm->gc_bump_threshold = (vm->heap_top - vm->heap_start) / 8;
    if (vm->gc_bump_threshold < 4096) vm->gc_bump_threshold = 4096;
    /* #355 — arma la reserva de emergencia (ver el comentario largo en bpvm.c,
     * donde se hace lo mismo por el otro camino de carga). Los DOS sitios que
     * fijan el umbral del GC tienen que armarla: si sólo lo hiciera uno, la
     * mitad de las cargas se quedaría sin red y el fallo volvería a ser mudo
     * según por dónde hubiera entrado el módulo. */
    vm->heap_reserve = 1024u;
    if ((vm->heap_top - vm->heap_start) < 16u * 1024u) vm->heap_reserve = 0u;

    if (main_offset >= 0 && vm->main_absolute_address == 0) {
        vm->main_absolute_address = mod_cb + (uint32_t) main_offset;
    }

    vm->module_count++;

    /* [V6/N1.4] EL BLOQUE NATIVO EMBEBIDO (.mod v7, seccion `native`).
     *
     * Va AQUI y no donde se salta la seccion, porque `bpvm_load_mdn` resuelve
     * los thunks POR NOMBRE contra los simbolos exportados del modulo — y esos
     * se registran mas arriba. Cargarlo antes no encontraria ninguno.
     *
     * La seccion son N blobs `.mdn` concatenados, cada uno autodelimitado por su
     * cabecera y autoidentificado por su `arch`. Se recorren en orden y se carga
     * el que coincide con esta imagen; los demas se saltan sin ruido, que es lo
     * normal en un modulo multifamilia.
     *
     * TOLERANTE a proposito: si un blob no carga, se dice y se sigue. El AOT es
     * una ACELERACION — sin el, el modulo corre interpretado y correcto. Lo que
     * no puede pasar es que un nativo malo impida ejecutar.
     *
     * V8 H9 (#786) — EN UN OVERLAY, NO: el código nativo lleva sus literales en crudo, y LEA_STR no pasa por ahí. Sin él
     * corre interpretado, que es lo que hace la VM-Java siempre. */
    if (native_size > 0 && !(vm->overlay != NULL && vm->overlay->cargando)) {
        /* ⚠️ EL .mod NO SIEMPRE ESTA EN RAM. El cursor tiene DOS fuentes (ver su
         * comentario arriba): `base` cuando el blob esta en RAM o en flash (host,
         * XIP, embebido en la imagen) y `rd` cuando se lee POR TROZOS — que es lo
         * que hace la placa desde H11: el .mod se queda en el FS y no cabe entero.
         *
         * La primera version usaba `data + off` a pelo. En host funcionaba y en la
         * Pico no hacia NADA, porque alli `data` es NULL: el bloque embebido no se
         * cargaba y el AOT se quedaba en el .mdn suelto. Se vio en placa —83 ms con
         * .mdn, 8518 sin el— y no dio ni un error.
         *
         * Con `base` se usa el puntero DIRECTO: es lo que hace falta para XIP, donde
         * el codigo se ejecuta en su sitio y copiarlo seria justo lo que la seccion
         * evita. Sin `base` hay que traerlo a RAM, y se libera al salir: los thunks
         * ya viven en el pool del AOT, no aqui. */
        /* El relleno que metio el escritor para que el PRIMER BLOB caiga en
         * multiplo de 4 DENTRO DEL FICHERO. Se calcula igual en los dos caminos,
         * pero se USA distinto — y confundirlos fue el cuelgue del 23-ago. */
        uint32_t pad = ((native_off + 3u) & ~3u) - native_off;
        if (pad > native_size) goto fin_nativo;          /* seccion absurda */
        uint32_t largo = native_size - pad;

        uint8_t* copia = NULL;
        const uint8_t* sec;
        if (c.base) {
            /* Puntero DIRECTO. Es lo que hace falta para XIP: el codigo se
             * ejecuta en su sitio y copiarlo seria justo lo que la seccion evita.
             * Aqui el relleno SI cuenta, porque `c.base + native_off` conserva la
             * posicion del fichero y es de ahi de donde sale la alineacion. */
            sec = c.base + native_off + pad;
        } else {
            /* Por TROZOS (la placa desde H11: el .mod se queda en el FS).
             *
             * ⚠️ Y aqui el relleno se SALTA, no se copia. La alineacion del blob
             * venia de su posicion EN EL FICHERO; al traerlo a un buffer nuevo esa
             * posicion desaparece y lo que manda es el offset dentro del buffer.
             * Copiar el relleno dejaba el blob en `copia + 1..3` — y un `malloc`
             * esta alineado, asi que eso es un acceso desalineado en Cortex-M33:
             * hard fault. En placa se vio como un CUELGUE seco, sin un mensaje.
             * Leyendo desde `native_off + pad`, el blob empieza en `copia + 0`. */
            /* ⚠️ ARENA DE LA VM, no `malloc`. El cargador de `.mdn` es ZERO-COPY
             * por diseno: registra los thunks apuntando DENTRO del buffer que se
             * le pasa, sin copiar. Asi que ese buffer tiene que seguir vivo, con
             * direccion estable y ejecutable, TODO EL RUN.
             *
             * La primera version pedia un `malloc` y lo liberaba al salir. En
             * placa eso fue un CUELGUE seco al ejecutar el thunk —saltaba a
             * memoria ya liberada— y el log llegaba justo hasta «about to
             * bpvm_run». Es la misma arena que usa el barrido del FS
             * (`repl_v1.c`), 4-alineada porque es lo que pide Thumb-2. */
            copia = bpvm_arena_reserve(vm, largo, 4);
            if (!copia) {
                bpvm_diag_urgente("[mdn] el bloque embebido (%u B) no cabe en la arena "
                                  "— se sigue interpretado", (unsigned) largo);
                goto fin_nativo;
            }
            if (!c.rd || c.rd(c.rd_user, native_off + pad, copia, largo) != (long) largo) {
                bpvm_diag_urgente("[mdn] no se pudo leer el bloque embebido — "
                                  "se sigue interpretado");
                goto fin_nativo;   /* la arena se recupera sola al acabar el RUN */
            }
            sec = copia;
        }

        uint32_t p = 0;   /* `sec` YA apunta al primer blob, en los dos caminos */
        int cargados = 0;
        while (p + MDN_HEADER_SIZE_V1 <= largo) {
            const mdn_header_t* h = (const mdn_header_t*) (const void*) (sec + p);
            if (h->magic[0] != 'M' || h->magic[1] != 'D' || h->magic[2] != 'N') break;
            /* [V7/H2, #441] la cabecera mide segun su version (v1 20 B, v2 24 B):
             * un blob v1 en un .mod de antes sigue recorriendose bien. */
            uint32_t tam = mdn_header_size(h->version)
                         + h->sym_count * (uint32_t) sizeof(mdn_symbol_t)
                         + h->code_size;
            tam = (tam + 3u) & ~3u;
            if (p + tam > largo) break;          /* truncado: no se adivina */

            /* El gate de arquitectura NO se reimplementa aqui: lo hace
             * `bpvm_load_mdn`, que ya distingue los casos raros (un `.mdn`
             * legacy con arch==0 vale solo en firmware ARM; en host no vale
             * ninguno). Duplicarlo seria una segunda copia que puede separarse
             * de la primera — y aqui equivocarse significa ejecutar codigo de
             * otra ISA. Asi que se le pasan TODOS y el decide.
             *
             * Por eso MDN_ERR_ARCH no se avisa: en un modulo multifamilia es lo
             * NORMAL —los blobs de las otras placas— y un aviso que salta
             * siempre se aprende a ignorar. */
            int rc = bpvm_load_mdn(vm, sec + p, tam);
            if (rc == MDN_OK) cargados++;
            else if (rc != MDN_ERR_ARCH)
                bpvm_diag_urgente("[mdn] bloque embebido rechazado (rc=%d) — se sigue "
                                  "interpretado", rc);
            p += tam;
        }
        if (cargados > 0)
            bpvm_diag_urgente("[mdn] %d bloque(s) nativo(s) desde el propio .mod", cargados);
        /* NO se libera: los thunks apuntan aqui dentro (zero-copy) y tienen que
         * seguir validos durante todo el RUN. La arena se recupera entera al
         * terminar, que es justo su contrato. */
    }
fin_nativo:
    (void) 0;

    return BPVM_OK;
}

bpvm_status_t bpvm_loader_load_buffer(bpvm_t* vm, const uint8_t* data,
                                       size_t size, const char* name_hint) {
    return load_buffer_impl(vm, data, NULL, NULL, size, name_hint, 0);
}

bpvm_status_t bpvm_loader_load_xip(bpvm_t* vm, const uint8_t* data,
                                    size_t size, const char* name_hint) {
    return load_buffer_impl(vm, data, NULL, NULL, size, name_hint, 1);
}

/* H11 — carga por trozos. El .mod se queda donde está y sólo pasan por RAM las
 * secciones pequeñas: data y code aterrizan directamente en memory[]. */
bpvm_status_t bpvm_loader_load_stream(bpvm_t* vm, bpvm_read_at_fn rd, void* user,
                                       size_t size, const char* name_hint) {
    return load_buffer_impl(vm, NULL, rd, user, size, name_hint, 0);
}

/* Lector por trozos sobre un FILE* — la fuente del host para el cursor stream. */
static long file_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    FILE* f = (FILE*) user;
    if (fseek(f, (long) off, SEEK_SET) != 0) return -1;
    return (long) fread(dst, 1, (size_t) n, f);
}

bpvm_status_t bpvm_loader_load(bpvm_t* vm, const char* path) {
    /* H11 — por trozos, IGUAL que el micro. Antes se leía el fichero entero a
     * un malloc; ahora el host recorre exactamente el mismo camino que la placa,
     * así que la batería de paridad ejercita el loader de streaming en cada
     * ejecución en vez de dejarlo sólo probado en el firmware. */
    FILE* f = fopen(path, "rb");
    if (!f) return BPVM_ERR_IO;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return BPVM_ERR_IO; }
    long sz = ftell(f);
    if (sz <= 0) { fclose(f); return BPVM_ERR_IO; }
    bpvm_status_t s = bpvm_loader_load_stream(vm, file_read_at, f, (size_t) sz, path);
    fclose(f);
    return s;
}
