/*
 * builtins.c — dispatch de CALL_BUILTIN para F2.
 *
 * IDs estables del enum Builtin de la VM Java (docs/BUILTINS.md).
 * F2 cubre el subset mínimo para programas que usan strings y arrays
 * comunes; el resto se va añadiendo según haga falta.
 */

#include "bpvm_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* IDs estables (= ordinal del enum Builtin Java). Sólo los que F2
 * implementa; los demás devuelven BAD_OPCODE para que el caller sepa
 * que falta. */
enum {
    BUILTIN_STRLEN          = 0,
    BUILTIN_PARSE_INT       = 1,
    BUILTIN_INT_TO_STRING   = 3,
    BUILTIN_BOOL_TO_STRING  = 5,
    BUILTIN_GC              = 43,
    BUILTIN_NEW_REF_ARRAY   = 44,
    BUILTIN_GROW_INT_ARRAY  = 46,
    BUILTIN_CHARS_TO_STRING = 47,
    BUILTIN_CHAR_CODE_AT    = 48
};

/* Helpers: pop / push del thread actual. */
static int32_t pop_i32(bpvm_t* vm, bpvm_thread_t* tc) {
    tc->sp -= 4;
    return bpvm_read_i32_be(vm->memory + tc->sp);
}
static void push_i32(bpvm_t* vm, bpvm_thread_t* tc, int32_t v) {
    bpvm_write_i32_be(vm->memory + tc->sp, v);
    tc->sp += 4;
}

/* Lee un string BP (TYPE_ARRAY_I32 con codepoints) a un buffer C UTF-8.
 * Devuelve el número de bytes escritos (sin null terminator). Si el
 * codepoint > 127 lo escribe como '?' (F2 v1 sólo ASCII). */
static size_t read_bp_string(const bpvm_t* vm, uint32_t ref, char* dst, size_t dst_size) {
    if (ref == 0) return 0;
    uint32_t len = bpvm_read_u32_be(vm->memory + ref);
    size_t out = 0;
    for (uint32_t i = 0; i < len && out + 1 < dst_size; i++) {
        uint32_t cp = bpvm_read_u32_be(vm->memory + ref + 4 + i * 4);
        dst[out++] = (cp < 128) ? (char) cp : '?';
    }
    if (out < dst_size) dst[out] = '\0';
    return out;
}

bpvm_status_t bpvm_call_builtin(bpvm_t* vm, bpvm_thread_t* tc, int id) {
    switch (id) {

    case BUILTIN_STRLEN: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        uint32_t len = (ref == 0) ? 0 : bpvm_read_u32_be(vm->memory + ref);
        push_i32(vm, tc, (int32_t) len);
        return BPVM_OK;
    }

    case BUILTIN_INT_TO_STRING: {
        int32_t v = pop_i32(vm, tc);
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%d", v);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }

    case BUILTIN_BOOL_TO_STRING: {
        int32_t v = pop_i32(vm, tc);
        const char* s = v ? "true" : "false";
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }

    case BUILTIN_PARSE_INT: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        char buf[64];
        read_bp_string(vm, ref, buf, sizeof(buf));
        /* trim simple */
        char* p = buf; while (*p == ' ' || *p == '\t') p++;
        char* end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\n' || end[-1] == '\r' || end[-1] == '\t')) {
            *--end = '\0';
        }
        long v = strtol(p, NULL, 10);
        push_i32(vm, tc, (int32_t) v);
        return BPVM_OK;
    }

    case BUILTIN_CHAR_CODE_AT: {
        int32_t i   = pop_i32(vm, tc);
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        if (ref == 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint32_t len = bpvm_read_u32_be(vm->memory + ref);
        if (i < 0 || (uint32_t) i >= len) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint32_t cp = bpvm_read_u32_be(vm->memory + ref + 4 + (uint32_t) i * 4);
        push_i32(vm, tc, (int32_t) cp);
        return BPVM_OK;
    }

    case BUILTIN_NEW_REF_ARRAY: {
        int32_t cap = pop_i32(vm, tc);
        if (cap < 0) return BPVM_ERR_RUNTIME;
        uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) cap * 4, BPVM_TYPE_ARRAY_REF);
        if (ref == 0) return BPVM_ERR_OOM;
        bpvm_write_u32_be(vm->memory + ref, (uint32_t) cap);
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }

    case BUILTIN_GROW_INT_ARRAY: {
        int32_t new_cap = pop_i32(vm, tc);
        uint32_t old_ref = (uint32_t) pop_i32(vm, tc);
        if (new_cap < 0) return BPVM_ERR_RUNTIME;
        uint32_t old_len = (old_ref == 0) ? 0 : bpvm_read_u32_be(vm->memory + old_ref);
        uint32_t new_ref = bpvm_heap_alloc(vm, (uint32_t) new_cap * 4, BPVM_TYPE_ARRAY_I32);
        if (new_ref == 0) return BPVM_ERR_OOM;
        bpvm_write_u32_be(vm->memory + new_ref, (uint32_t) new_cap);
        uint32_t copy = (old_len < (uint32_t) new_cap) ? old_len : (uint32_t) new_cap;
        for (uint32_t i = 0; i < copy; i++) {
            uint32_t v = bpvm_read_u32_be(vm->memory + old_ref + 4 + i * 4);
            bpvm_write_u32_be(vm->memory + new_ref + 4 + i * 4, v);
        }
        push_i32(vm, tc, (int32_t) new_ref);
        return BPVM_OK;
    }

    case BUILTIN_CHARS_TO_STRING: {
        int32_t len = pop_i32(vm, tc);
        uint32_t chars_ref = (uint32_t) pop_i32(vm, tc);
        if (len < 0) return BPVM_ERR_RUNTIME;
        uint32_t avail = (chars_ref == 0) ? 0 : bpvm_read_u32_be(vm->memory + chars_ref);
        if ((uint32_t) len > avail) return BPVM_ERR_RUNTIME;
        uint32_t new_ref = bpvm_heap_alloc(vm, (uint32_t) len * 4, BPVM_TYPE_ARRAY_I32);
        if (new_ref == 0) return BPVM_ERR_OOM;
        bpvm_write_u32_be(vm->memory + new_ref, (uint32_t) len);
        for (uint32_t i = 0; i < (uint32_t) len; i++) {
            uint32_t cp = bpvm_read_u32_be(vm->memory + chars_ref + 4 + i * 4);
            bpvm_write_u32_be(vm->memory + new_ref + 4 + i * 4, cp);
        }
        push_i32(vm, tc, (int32_t) new_ref);
        return BPVM_OK;
    }

    case BUILTIN_GC: {
        bpvm_heap_gc(vm);
        push_i32(vm, tc, 0);   /* void → push dummy */
        return BPVM_OK;
    }

    default:
        fprintf(stderr, "[bpvm-c] builtin id=%d no implementado (F2 subset)\n", id);
        return BPVM_ERR_BAD_OPCODE;
    }
}
