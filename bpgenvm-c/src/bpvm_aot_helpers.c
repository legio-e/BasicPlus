/*
 * bpvm_aot_helpers.c — instancia de la tabla aot_helpers_v1_t con
 * punteros a las funciones reales del runtime. Linkada SIEMPRE en
 * cualquier build (host o Pico), aunque no haya código AOT cargado.
 *
 * Si el linker se queja de "undefined reference" tras un cambio en
 * el header (p.ej. añadiste un slot), implementar el helper aquí
 * con un trampolín al símbolo del runtime correspondiente.
 */

#include "bpvm_aot_helpers.h"
#include "bpvm_internal.h"

#include <stdio.h>
#include <string.h>

/* ---------- I/O memoria big-endian ----------
 * Estos están como static inline en bpvm_internal.h — necesitamos
 * un wrapper extern para tenerlos en la tabla. Mismas semánticas. */
static int32_t h_read_i32_be(const uint8_t* p) {
    return bpvm_read_i32_be(p);
}
static void h_write_i32_be(uint8_t* p, int32_t v) {
    bpvm_write_i32_be(p, v);
}
static int16_t h_read_i16_be(const uint8_t* p) {
    return (int16_t) bpvm_read_u16_be(p);
}
static void h_write_i16_be(uint8_t* p, int16_t v) {
    p[0] = (uint8_t)((v >> 8) & 0xFF);
    p[1] = (uint8_t)(v & 0xFF);
}

/* ---------- Control de ejecución ----------
 * throw_runtime levanta un BpThreadFault que sube por la pila de
 * frames hasta el handler más cercano (o tumba el thread). */
static void h_throw_runtime(bpvm_t* vm, const char* msg) {
    /* Por ahora reportamos al stderr y marcamos status fault. La
     * integración con try/catch BP vendrá cuando el AOT lo soporte. */
    if (msg) fprintf(stderr, "[aot] throw_runtime: %s\n", msg);
    (void) vm;
    /* TODO: vm->thread_fault = BPVM_FAULT_RUNTIME; longjmp; */
}

/* ---------- Heap / GC ----------
 * Stubs por ahora — el AOT que use estos slots tiene que activar
 * via flag de capabilities en el .mdn. La fase A no los necesita. */
static int32_t h_newarray_i32(bpvm_t* vm, int32_t size) {
    (void) vm; (void) size;
    fprintf(stderr, "[aot] newarray_i32 stub — implementar al AOT-ear arrays\n");
    return 0;
}
static int32_t h_newarray_i8(bpvm_t* vm, int32_t size) {
    (void) vm; (void) size; return 0;
}
static int32_t h_newarray_i16(bpvm_t* vm, int32_t size) {
    (void) vm; (void) size; return 0;
}
static int32_t h_new_object(bpvm_t* vm, uint32_t class_addr) {
    (void) vm; (void) class_addr; return 0;
}

/* ---------- Output sink ----------
 * Replican los OP_PRINT_* del intérprete pero invocables como C. */
static void h_print_i32(bpvm_t* vm, int32_t v, int nl) {
    char buf[24];
    int n = snprintf(buf, sizeof(buf), nl ? "%d\n" : "%d", (int) v);
    if (n > 0 && vm->output_cb) {
        vm->output_cb(buf, (size_t) n, vm->output_user);
    } else if (n > 0) {
        fwrite(buf, 1, (size_t) n, stdout);
    }
}
static void h_print_f32(bpvm_t* vm, float v, int nl) {
    char buf[40];
    int n = snprintf(buf, sizeof(buf), nl ? "%g\n" : "%g", (double) v);
    if (n > 0 && vm->output_cb) {
        vm->output_cb(buf, (size_t) n, vm->output_user);
    } else if (n > 0) {
        fwrite(buf, 1, (size_t) n, stdout);
    }
}
static void h_print_string(bpvm_t* vm, uint32_t ref, int nl) {
    /* Stub básico — el AOT de strings vendrá más adelante. */
    (void) vm; (void) ref; (void) nl;
    fprintf(stderr, "[aot] print_string stub\n");
}
static void h_print_char(bpvm_t* vm, int32_t ch) {
    char c = (char) ch;
    if (vm->output_cb) vm->output_cb(&c, 1, vm->output_user);
    else fputc(ch, stdout);
}
static void h_print_nl(bpvm_t* vm) {
    if (vm->output_cb) vm->output_cb("\n", 1, vm->output_user);
    else fputc('\n', stdout);
}

/* ---------- I/O float (H3 #166) ----------
 * Los floats viven en el stack BP como bits IEEE-754 i32 big-endian.
 * Estos wrappers hacen la conversión bits↔float para que el thunk AOT
 * no tenga que hacer type-punning manual.
 *
 * bits_to_float / float_to_bits replicados aquí inline; los originales
 * en interp.c son `static` y no exportados. */
static inline float aoth_bits_to_float(uint32_t bits) {
    union { float f; uint32_t u; } u; u.u = bits; return u.f;
}
static inline uint32_t aoth_float_to_bits(float f) {
    union { float f; uint32_t u; } u; u.f = f;    return u.u;
}
static float h_read_f32_be(const uint8_t* p) {
    return aoth_bits_to_float((uint32_t) bpvm_read_i32_be(p));
}
static void h_write_f32_be(uint8_t* p, float v) {
    bpvm_write_i32_be(p, (int32_t) aoth_float_to_bits(v));
}

/* ---------- Acceso a arrays (H3 #167) ----------
 * Layout heap: [length:u32 BE][el0:T][el1:T]... — T=4 bytes para i32.
 * Bounds check + null check; en fallo invocamos throw_runtime (que hoy
 * solo reporta a stderr — la integración con try/catch BP vendrá con
 * #175). */
static int32_t h_array_load_i32(bpvm_t* vm, uint32_t ref, int32_t idx) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v1.throw_runtime(vm, "array_load_i32: null array");
        return 0;
    }
    uint8_t* mem = vm->memory;
    uint32_t length = bpvm_read_u32_be(mem + ref);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v1.throw_runtime(vm, "array_load_i32: index out of bounds");
        return 0;
    }
    return bpvm_read_i32_be(mem + ref + 4 + (uint32_t) idx * 4);
}
static void h_array_store_i32(bpvm_t* vm, uint32_t ref, int32_t idx, int32_t v) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v1.throw_runtime(vm, "array_store_i32: null array");
        return;
    }
    uint8_t* mem = vm->memory;
    uint32_t length = bpvm_read_u32_be(mem + ref);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v1.throw_runtime(vm, "array_store_i32: index out of bounds");
        return;
    }
    bpvm_write_i32_be(mem + ref + 4 + (uint32_t) idx * 4, v);
}
static int32_t h_array_length(bpvm_t* vm, uint32_t ref) {
    if (ref == 0) return 0;   /* null array → length 0 (BP semantics) */
    return (int32_t) bpvm_read_u32_be(vm->memory + ref);
}

/* ---------- Instancia exportada ----------
 * `const` para que viva en .rodata (flash en el Pico). */
const aot_helpers_v1_t bpvm_aot_helpers_v1 = {
    .read_i32_be     = h_read_i32_be,
    .write_i32_be    = h_write_i32_be,
    .read_i16_be     = h_read_i16_be,
    .write_i16_be    = h_write_i16_be,
    .throw_runtime   = h_throw_runtime,
    .newarray_i32    = h_newarray_i32,
    .newarray_i8     = h_newarray_i8,
    .newarray_i16    = h_newarray_i16,
    .new_object      = h_new_object,
    .print_i32       = h_print_i32,
    .print_f32       = h_print_f32,
    .print_string    = h_print_string,
    .print_char      = h_print_char,
    .print_nl        = h_print_nl,
    .read_f32_be     = h_read_f32_be,
    .write_f32_be    = h_write_f32_be,
    .array_load_i32  = h_array_load_i32,
    .array_store_i32 = h_array_store_i32,
    .array_length    = h_array_length,
};
