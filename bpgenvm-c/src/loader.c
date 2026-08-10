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

/* Cuerpo común de la carga. `xip` (H3.c): el código NO se copia a RAM — queda
 * direccionado en sitio (los bytes de `data` deben ser persistentes: la región
 * de packs montada). CS queda anclado en RAM como siempre; solo cb difiere. */
static bpvm_status_t load_buffer_impl(bpvm_t* vm, const uint8_t* data,
                                      bpvm_read_at_fn rd, void* rd_user,
                                      size_t size, const char* name_hint, int xip) {
    if (vm->module_count >= BPVM_MAX_MODULES) return BPVM_ERR_OOM;
    if (!data && !rd) return BPVM_ERR_IO;
    if (size < 28) return BPVM_ERR_BAD_HEADER;
    if (xip && !data) return BPVM_ERR_IO;   /* XIP ejecuta EN SITIO: exige blob mapeado */

    buf_cursor_t c = { data, rd, rd_user, size, 0, 0 };

    /* --- Header (v5=28 bytes / v6=32 bytes) --- */
    uint32_t magic, data_size, imports_size, exports_size, code_size, library_size;
    uint32_t interface_size = 0;   /* H6.a: sección interface (sólo v6) */
    int32_t  main_offset;
    if (bc_read_be32(&c, &magic) != 0)                 return BPVM_ERR_IO;
    /* #284 — GATE DE ABI. La versión del formato ES la declaración de ABI:
     * v6 garantiza refs de 8 bytes (nació DESPUÉS del ensanchado 4->8B), v5 es
     * AMBIGUO (puede ser de la era 4B y corromper memoria en silencio). Lo que
     * no se pueda garantizar se RECHAZA aquí, en la carga, con un error claro
     * -- nunca se ejecuta a ciegas. */
    if (magic == BPVM_MAGIC)                           return BPVM_ERR_ABI_MOD_V5;
    if (magic != BPVM_MAGIC_V6)                        return BPVM_ERR_BAD_MAGIC;
    int is_v6 = 1;
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
    if (is_v6 && bc_read_be32(&c, &interface_size) != 0) return BPVM_ERR_IO;

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
        mod->imports = (char**) bpvm_calloc(ext_count, sizeof(char*));
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
            mod->imports[i] = bpvm_strdup(tmp_name);
            if (!mod->imports[i]) return BPVM_ERR_OOM;
        }
    }
    (void) imports_size;

    /* --- Layout en memory[] ---
     *    moduleBase ─┬─ ext-table (ext_count * 4 bytes, zeroed)
     *                ├─ data block (data_size bytes)
     *                └─ code block (code_size bytes)   [XIP: NO ocupa RAM]
     */
    uint32_t module_base = vm->next_free_address;
    uint32_t ext_table_size = ext_count * BPVM_EXT_ENTRY_SIZE;
    uint32_t data_start  = module_base + ext_table_size;
    uint32_t code_start  = data_start + data_size;      /* = CS (ancla, SIEMPRE RAM) */
    uint32_t end_addr    = xip ? code_start : code_start + code_size;

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
        if (c.base) {
            exp_buf = c.base + c.pos;      /* RAM: cero copia, como siempre */
            c.pos += exports_size;
        } else {
            /* STREAM: es la ÚNICA sección que el parser mira por offsets, así
             * que tiene que estar residente. La montamos en la arena libre por
             * encima del módulo — memoria que nadie ha reclamado todavía
             * (next_free_address no llega ahí hasta el final) y que se olvida
             * sola al volver. Cero buffers estáticos. */
            if (end_addr + exports_size > vm->stack_base) return BPVM_ERR_OOM;
            if (bc_read(&c, vm->memory + end_addr, exports_size) != 0) return BPVM_ERR_IO;
            exp_buf = vm->memory + end_addr;
        }
    }

    /* H6.a — la sección interface va entre exports y data; la VM ejecuta sin
     * ella (es metadato de compilación). La saltamos íntegra. */
    if (interface_size > 0) {
        if (bc_skip(&c, interface_size) != 0) return BPVM_ERR_IO;
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
            char name[128]; size_t nl = nlen < sizeof(name) - 1 ? nlen : sizeof(name) - 1;
            memcpy(name, exp_buf + exp_off, nl); name[nl] = '\0';
            exp_off += nlen;
            int32_t rel = bpvm_read_i32_be(exp_buf + exp_off); exp_off += 4;
            uint32_t abs = mod_cb + (uint32_t) rel;   /* función → base de CÓDIGO (XIP: flash) */
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
                /* data export → ancla CS (RAM): descriptors/constantes viven en data. */
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
                    mod->class_fixups = (bpvm_class_fixup_t*) bpvm_calloc(fxcount,
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
            /* 4.4 eh-class fixups opcional (BUG-2 — catch cross-module). */
            if (exp_off + 4 <= exports_size) {
                uint32_t ehcount = bpvm_read_u32_be(exp_buf + exp_off); exp_off += 4;
                if (ehcount > 0) {
                    mod->eh_class_fixups = (bpvm_eh_class_fixup_t*) bpvm_calloc(ehcount,
                                          sizeof(bpvm_eh_class_fixup_t));
                    if (!mod->eh_class_fixups) return BPVM_ERR_OOM;
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
    vm->free_list_head    = 0;
    vm->last_gc_heap_next = vm->heap_next;
    vm->alloc_since_gc    = 0u;   /* #357 */
    vm->gc_bump_threshold = (vm->stack_base - vm->heap_start) / 8;
    if (vm->gc_bump_threshold < 4096) vm->gc_bump_threshold = 4096;
    /* #355 — arma la reserva de emergencia (ver el comentario largo en bpvm.c,
     * donde se hace lo mismo por el otro camino de carga). Los DOS sitios que
     * fijan el umbral del GC tienen que armarla: si sólo lo hiciera uno, la
     * mitad de las cargas se quedaría sin red y el fallo volvería a ser mudo
     * según por dónde hubiera entrado el módulo. */
    vm->heap_reserve = 1024u;
    if ((vm->stack_base - vm->heap_start) < 16u * 1024u) vm->heap_reserve = 0u;

    if (main_offset >= 0 && vm->main_absolute_address == 0) {
        vm->main_absolute_address = mod_cb + (uint32_t) main_offset;
    }

    vm->module_count++;
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
