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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ---------- Cursor sobre un buffer in-memory. -------------------- */
typedef struct {
    const uint8_t* base;
    size_t         size;
    size_t         pos;
    int            err;       /* 1 si se intentó leer fuera de rango. */
} buf_cursor_t;

static int bc_read(buf_cursor_t* c, void* dst, size_t n) {
    if (c->err) return -1;
    if (c->pos + n > c->size) { c->err = 1; return -1; }
    if (dst) memcpy(dst, c->base + c->pos, n);
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

bpvm_status_t bpvm_loader_load_buffer(bpvm_t* vm, const uint8_t* data,
                                       size_t size, const char* name_hint) {
    if (vm->module_count >= BPVM_MAX_MODULES) return BPVM_ERR_OOM;
    if (!data || size < 28) return BPVM_ERR_BAD_HEADER;

    buf_cursor_t c = { data, size, 0, 0 };

    /* --- Header (28 bytes) --- */
    uint32_t magic, data_size, imports_size, exports_size, code_size, library_size;
    int32_t  main_offset;
    if (bc_read_be32(&c, &magic) != 0)                 return BPVM_ERR_IO;
    if (magic != BPVM_MAGIC)                            return BPVM_ERR_BAD_MAGIC;
    if (bc_read_be32(&c, &data_size) != 0)             return BPVM_ERR_IO;
    {
        uint32_t mo;
        if (bc_read_be32(&c, &mo) != 0)                return BPVM_ERR_IO;
        main_offset = (int32_t) mo;
    }
    if (bc_read_be32(&c, &imports_size) != 0)          return BPVM_ERR_IO;
    if (bc_read_be32(&c, &exports_size) != 0)          return BPVM_ERR_IO;
    if (bc_read_be32(&c, &code_size) != 0)             return BPVM_ERR_IO;
    if (bc_read_be32(&c, &library_size) != 0)          return BPVM_ERR_IO;

    bpvm_module_t* mod = &vm->modules[vm->module_count];
    memset(mod, 0, sizeof(*mod));
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
    }

    /* --- Imports (count + (name UTF, fromPath UTF) * count) --- */
    uint32_t ext_count;
    if (bc_read_be32(&c, &ext_count) != 0)             return BPVM_ERR_IO;
    mod->ext_count = ext_count;
    mod->import_count = (int) ext_count;
    if (ext_count > 0) {
        mod->imports = (char**) calloc(ext_count, sizeof(char*));
        if (!mod->imports) return BPVM_ERR_OOM;
        char tmp_from[256];
        for (uint32_t i = 0; i < ext_count; i++) {
            char tmp_name[256];
            if (bc_read_writeutf(&c, tmp_name, sizeof(tmp_name)) != 0) {
                return BPVM_ERR_IO;
            }
            if (bc_read_writeutf(&c, tmp_from, sizeof(tmp_from)) != 0) {
                return BPVM_ERR_IO;
            }
            mod->imports[i] = strdup(tmp_name);
            if (!mod->imports[i]) return BPVM_ERR_OOM;
        }
    }
    (void) imports_size;

    /* --- Layout en memory[] ---
     *    moduleBase ─┬─ ext-table (ext_count * 4 bytes, zeroed)
     *                ├─ data block (data_size bytes)
     *                └─ code block (code_size bytes)
     */
    uint32_t module_base = vm->next_free_address;
    uint32_t ext_table_size = ext_count * BPVM_EXT_ENTRY_SIZE;
    uint32_t data_start  = module_base + ext_table_size;
    uint32_t code_start  = data_start + data_size;
    uint32_t end_addr    = code_start + code_size;

    if (end_addr > vm->stack_base) {
        return BPVM_ERR_OOM;
    }

    if (ext_table_size > 0) {
        memset(vm->memory + module_base, 0, ext_table_size);
    }

    /* Sección exports: leerla completa a un buffer (puntero al cursor mismo)
     * para poder detectar las sub-secciones opcionales sin reinventar el
     * scanning. Como ya es buffer-backed, sólo capturamos el offset y la
     * tamaño y avanzamos el cursor. */
    const uint8_t* exp_buf = NULL;
    if (exports_size > 0) {
        if (c.pos + exports_size > c.size) return BPVM_ERR_IO;
        exp_buf = c.base + c.pos;
        c.pos += exports_size;
    }

    /* Data block: lo copiamos en memory[]. */
    if (data_size > 0) {
        if (c.pos + data_size > c.size) return BPVM_ERR_IO;
        memcpy(vm->memory + data_start, c.base + c.pos, data_size);
        c.pos += data_size;
    }

    /* Code block: idem. */
    if (code_size > 0) {
        if (c.pos + code_size > c.size) return BPVM_ERR_IO;
        memcpy(vm->memory + code_start, c.base + c.pos, code_size);
        c.pos += code_size;
    }

    mod->module_base    = module_base;
    mod->ext_table_addr = module_base;
    mod->data_start     = data_start;
    mod->code_start     = code_start;
    mod->end_addr       = end_addr;

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
            char name[128]; size_t nl = nlen < sizeof(name) - 1 ? nlen : sizeof(name) - 1;
            memcpy(name, exp_buf + exp_off, nl); name[nl] = '\0';
            exp_off += nlen;
            int32_t rel = bpvm_read_i32_be(exp_buf + exp_off); exp_off += 4;
            uint32_t abs = code_start + (uint32_t) rel;
            char qual[320];
            snprintf(qual, sizeof(qual), "%s%s", export_prefix, name);
            bpvm_link_register_symbol(vm, qual, abs);
            if (mod->library[0]) {
                char short_q[320];
                snprintf(short_q, sizeof(short_q), "%s.%s", mod->name, name);
                if (bpvm_link_lookup(vm, short_q) == 0) {
                    bpvm_link_register_symbol(vm, short_q, abs);
                }
            }
        }

        /* 4.2 data exports opcional. */
        if (exp_off + 4 <= exports_size) {
            uint32_t dcount = bpvm_read_u32_be(exp_buf + exp_off); exp_off += 4;
            for (uint32_t i = 0; i < dcount; i++) {
                if (exp_off + 2 > exports_size) break;
                uint16_t nlen = bpvm_read_u16_be(exp_buf + exp_off); exp_off += 2;
                if (exp_off + nlen + 4 > exports_size) break;
                char name[128]; size_t nl = nlen < sizeof(name) - 1 ? nlen : sizeof(name) - 1;
                memcpy(name, exp_buf + exp_off, nl); name[nl] = '\0';
                exp_off += nlen;
                int32_t cs_off = bpvm_read_i32_be(exp_buf + exp_off); exp_off += 4;
                uint32_t abs = (uint32_t)((int32_t) code_start + cs_off);
                char qual[320];
                snprintf(qual, sizeof(qual), "%s%s", export_prefix, name);
                bpvm_link_register_symbol(vm, qual, abs);
                if (mod->library[0]) {
                    char short_q[320];
                    snprintf(short_q, sizeof(short_q), "%s.%s", mod->name, name);
                    if (bpvm_link_lookup(vm, short_q) == 0) {
                        bpvm_link_register_symbol(vm, short_q, abs);
                    }
                }
            }

            /* 4.3 class fixups opcional. */
            if (exp_off + 4 <= exports_size) {
                uint32_t fxcount = bpvm_read_u32_be(exp_buf + exp_off); exp_off += 4;
                if (fxcount > 0) {
                    mod->class_fixups = (bpvm_class_fixup_t*) calloc(fxcount,
                                          sizeof(bpvm_class_fixup_t));
                    if (!mod->class_fixups) return BPVM_ERR_OOM;
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
        }
    }

    vm->next_free_address = end_addr + 64; /* margen entre módulos */
    vm->heap_start        = vm->next_free_address;
    vm->heap_next         = vm->heap_start;
    /* H3 (V2): init del GC con free-list + umbral, con el heap_start real. */
    vm->free_list_head    = 0;
    vm->last_gc_heap_next = vm->heap_next;
    vm->gc_bump_threshold = (vm->stack_base - vm->heap_start) / 8;
    if (vm->gc_bump_threshold < 4096) vm->gc_bump_threshold = 4096;

    if (main_offset >= 0 && vm->main_absolute_address == 0) {
        vm->main_absolute_address = code_start + (uint32_t) main_offset;
    }

    vm->module_count++;
    return BPVM_OK;
}

bpvm_status_t bpvm_loader_load(bpvm_t* vm, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return BPVM_ERR_IO;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return BPVM_ERR_IO; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return BPVM_ERR_IO; }
    rewind(f);
    uint8_t* buf = (uint8_t*) malloc((size_t) sz);
    if (!buf) { fclose(f); return BPVM_ERR_OOM; }
    if (fread(buf, 1, (size_t) sz, f) != (size_t) sz) {
        free(buf); fclose(f); return BPVM_ERR_IO;
    }
    fclose(f);
    bpvm_status_t s = bpvm_loader_load_buffer(vm, buf, (size_t) sz, path);
    free(buf);
    return s;
}
