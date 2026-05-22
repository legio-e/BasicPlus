/*
 * loader.c — lee un fichero .mod del disco, valida el header (MAGIC,
 * tamaños) y vuelca data + code en el memory[] de la VM. La spec
 * canónica está en docs/MOD_FORMAT.md.
 *
 * F1: no resuelve imports ni linka. F3 añadirá class_fixups y linkAll.
 */

#include "bpvm_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Lee N bytes desde FILE* a `dst`. Devuelve 0 en éxito, -1 en error. */
static int read_exact(FILE* f, void* dst, size_t n) {
    size_t got = fread(dst, 1, n, f);
    return got == n ? 0 : -1;
}

/* Skip N bytes consumiéndolos del stream. */
static int skip_bytes(FILE* f, size_t n) {
    char buf[256];
    while (n > 0) {
        size_t chunk = n > sizeof(buf) ? sizeof(buf) : n;
        if (read_exact(f, buf, chunk) != 0) return -1;
        n -= chunk;
    }
    return 0;
}

/* Lee un i32 big-endian. */
static int read_be32(FILE* f, uint32_t* out) {
    uint8_t b[4];
    if (read_exact(f, b, 4) != 0) return -1;
    *out = bpvm_read_u32_be(b);
    return 0;
}

/* Lee un string serializado por DataOutputStream.writeUTF de Java:
 * u16 big-endian length seguido de `length` bytes UTF-8. `dst` se
 * llena hasta `dst_size - 1` y se null-termina. Si el length excede
 * dst_size, se truncan los bytes sobrantes (pero se consumen).
 */
static int read_writeutf(FILE* f, char* dst, size_t dst_size) {
    uint8_t lenb[2];
    if (read_exact(f, lenb, 2) != 0) return -1;
    uint16_t len = bpvm_read_u16_be(lenb);
    if (len == 0) {
        if (dst_size > 0) dst[0] = '\0';
        return 0;
    }
    size_t to_copy = (len < dst_size - 1) ? len : dst_size - 1;
    if (read_exact(f, dst, to_copy) != 0) return -1;
    dst[to_copy] = '\0';
    if (to_copy < len) {
        /* truncado pero seguimos consumiendo. */
        if (skip_bytes(f, len - to_copy) != 0) return -1;
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

bpvm_status_t bpvm_loader_load(bpvm_t* vm, const char* path) {
    if (vm->module_count >= BPVM_MAX_MODULES) return BPVM_ERR_OOM;

    FILE* f = fopen(path, "rb");
    if (!f) return BPVM_ERR_IO;

    /* --- Header (28 bytes) --- */
    uint32_t magic, data_size, imports_size, exports_size, code_size, library_size;
    int32_t  main_offset;
    if (read_be32(f, &magic) != 0)                     { fclose(f); return BPVM_ERR_IO; }
    if (magic != BPVM_MAGIC)                            { fclose(f); return BPVM_ERR_BAD_MAGIC; }
    if (read_be32(f, &data_size) != 0)                 { fclose(f); return BPVM_ERR_IO; }
    {
        uint32_t mo;
        if (read_be32(f, &mo) != 0)                    { fclose(f); return BPVM_ERR_IO; }
        main_offset = (int32_t) mo;
    }
    if (read_be32(f, &imports_size) != 0)              { fclose(f); return BPVM_ERR_IO; }
    if (read_be32(f, &exports_size) != 0)              { fclose(f); return BPVM_ERR_IO; }
    if (read_be32(f, &code_size) != 0)                 { fclose(f); return BPVM_ERR_IO; }
    if (read_be32(f, &library_size) != 0)              { fclose(f); return BPVM_ERR_IO; }

    bpvm_module_t* mod = &vm->modules[vm->module_count];
    memset(mod, 0, sizeof(*mod));
    derive_module_name(path, mod->name, sizeof(mod->name));
    mod->main_offset = main_offset;
    mod->data_size   = data_size;
    mod->code_size   = code_size;

    /* --- Library (sin length prefix; raw UTF-8) --- */
    if (library_size > 0) {
        size_t n = library_size < sizeof(mod->library) - 1
                   ? library_size : sizeof(mod->library) - 1;
        if (read_exact(f, mod->library, n) != 0)       { fclose(f); return BPVM_ERR_IO; }
        mod->library[n] = '\0';
        if (n < library_size && skip_bytes(f, library_size - n) != 0) {
            fclose(f); return BPVM_ERR_IO;
        }
    }

    /* --- Imports (count + (name UTF, fromPath UTF) * count) --- */
    uint32_t ext_count;
    if (read_be32(f, &ext_count) != 0)                 { fclose(f); return BPVM_ERR_IO; }
    mod->ext_count = ext_count;
    mod->import_count = (int) ext_count;
    if (ext_count > 0) {
        mod->imports = (char**) calloc(ext_count, sizeof(char*));
        if (!mod->imports) { fclose(f); return BPVM_ERR_OOM; }
        char tmp_from[256];
        for (uint32_t i = 0; i < ext_count; i++) {
            char tmp_name[256];
            if (read_writeutf(f, tmp_name, sizeof(tmp_name)) != 0) {
                fclose(f); return BPVM_ERR_IO;
            }
            if (read_writeutf(f, tmp_from, sizeof(tmp_from)) != 0) {
                fclose(f); return BPVM_ERR_IO;
            }
            mod->imports[i] = strdup(tmp_name);
            if (!mod->imports[i]) { fclose(f); return BPVM_ERR_OOM; }
        }
    }
    (void) imports_size; /* Ya consumido como ext_count + pares. */

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
        /* No cabe. */
        fclose(f);
        return BPVM_ERR_OOM;
    }

    /* Cero la ext-table. */
    if (ext_table_size > 0) {
        memset(vm->memory + module_base, 0, ext_table_size);
    }

    /* --- Sección exports: leerla COMPLETA a un buffer para poder
     * detectar las sub-secciones opcionales (data symbols B3 v2 + class
     * fixups L2 v3). El loader Java hace lo mismo (ModuleManager.java). */
    uint8_t* exp_buf = (uint8_t*) malloc(exports_size);
    if (!exp_buf && exports_size > 0) { fclose(f); return BPVM_ERR_OOM; }
    if (exports_size > 0) {
        if (read_exact(f, exp_buf, exports_size) != 0) {
            free(exp_buf); fclose(f); return BPVM_ERR_IO;
        }
    }

    /* Leer el data block. */
    if (data_size > 0) {
        if (read_exact(f, vm->memory + data_start, data_size) != 0) {
            free(exp_buf); fclose(f); return BPVM_ERR_IO;
        }
    }

    /* Leer el code block. */
    if (code_size > 0) {
        if (read_exact(f, vm->memory + code_start, code_size) != 0) {
            free(exp_buf); fclose(f); return BPVM_ERR_IO;
        }
    }

    fclose(f);

    mod->module_base    = module_base;
    mod->ext_table_addr = module_base;
    mod->data_start     = data_start;
    mod->code_start     = code_start;
    mod->end_addr       = end_addr;

    /* --- Procesar la sección exports del buffer ---
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
            /* También sin library prefix si la hay (compat). */
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
                    if (!mod->class_fixups) {
                        free(exp_buf); return BPVM_ERR_OOM;
                    }
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
    free(exp_buf);

    vm->next_free_address = end_addr + 64; /* margen entre módulos */
    vm->heap_start        = vm->next_free_address;
    vm->heap_next         = vm->heap_start;

    /* Si tiene entry-point y todavía no hemos fijado el principal,
       este es el módulo raíz. */
    if (main_offset >= 0 && vm->main_absolute_address == 0) {
        vm->main_absolute_address = code_start + (uint32_t) main_offset;
    }

    vm->module_count++;
    return BPVM_OK;
}
