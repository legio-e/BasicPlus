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
    /* En F1 leemos los nombres pero los descartamos (no hay linkAll).
       Reservamos buffer scratch para los strings. */
    {
        char tmp_name[256];
        char tmp_from[256];
        for (uint32_t i = 0; i < ext_count; i++) {
            if (read_writeutf(f, tmp_name, sizeof(tmp_name)) != 0) {
                fclose(f); return BPVM_ERR_IO;
            }
            if (read_writeutf(f, tmp_from, sizeof(tmp_from)) != 0) {
                fclose(f); return BPVM_ERR_IO;
            }
        }
    }

    /* --- Exports (los descartamos en F1; F3 los necesitará) --- */
    if (skip_bytes(f, exports_size) != 0)              { fclose(f); return BPVM_ERR_IO; }
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

    /* Leer el data block. */
    if (data_size > 0) {
        if (read_exact(f, vm->memory + data_start, data_size) != 0) {
            fclose(f); return BPVM_ERR_IO;
        }
    }

    /* Leer el code block. */
    if (code_size > 0) {
        if (read_exact(f, vm->memory + code_start, code_size) != 0) {
            fclose(f); return BPVM_ERR_IO;
        }
    }

    fclose(f);

    mod->module_base    = module_base;
    mod->ext_table_addr = module_base;
    mod->data_start     = data_start;
    mod->code_start     = code_start;
    mod->end_addr       = end_addr;

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
