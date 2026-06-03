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
#include <setjmp.h>

/* ---------- #186: slot de fault por worker ----------
 * Ver bpvm_internal.h para el diseño. En host hay N workers pthread →
 * TLS (__thread). En Pico la config validada es single-worker → un
 * global plano basta (multi-worker Pico = v2, necesitará task-local). */
#if defined(BPVM_PICO_NUM_CORES) || defined(ESP_PLATFORM)
   /* MCU (Pico / ESP32): el AOT no corre (no hay codegen para la ISA del
    * micro — el .mdn es ARM Thumb-2), así que el fault-slot NUNCA se arma
    * → un global plano basta y evita depender de __thread (ELF TLS) en el
    * toolchain del micro. */
#  define BPVM_AOT_TLS
#else
#  define BPVM_AOT_TLS __thread   /* host: N workers pthread */
#endif

static BPVM_AOT_TLS bpvm_aot_fault_t g_aot_fault;

bpvm_aot_fault_t* bpvm_aot_fault_slot(void) {
    return &g_aot_fault;
}

/* P-aot-call-bp: contexto del puente native→BP, por worker (mismo
 * razonamiento TLS que el fault-slot). Lo fija aot_call_guarded (interp.c)
 * alrededor de cada thunk; tc==NULL fuera de un thunk. */
static BPVM_AOT_TLS bpvm_aot_callctx_t g_aot_callctx;

bpvm_aot_callctx_t* bpvm_aot_callctx(void) {
    return &g_aot_callctx;
}

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
    (void) vm;
    /* #186: si el call-site AOT del intérprete armó un boundary en este
     * worker, copiamos el mensaje y hacemos longjmp de vuelta. El interp
     * construye el RuntimeError y lo propaga al try/catch BP (o termina
     * el thread). NO retornamos — el native NO sigue ejecutando con
     * basura como antes. */
    bpvm_aot_fault_t* f = bpvm_aot_fault_slot();
    if (f->armed) {
        size_t n = msg ? strlen(msg) : 0;
        if (n > sizeof(f->msg) - 1) n = sizeof(f->msg) - 1;
        if (msg && n) memcpy(f->msg, msg, n);
        f->msg[n] = '\0';
        longjmp(f->buf, 1);   /* no retorna */
    }
    /* Sin boundary armado: no debería ocurrir (los native sólo corren
     * vía el hijack AOT del intérprete). Reportamos al menos. */
    if (msg) fprintf(stderr, "[aot] throw_runtime sin boundary: %s\n", msg);
}

/* #175 — throw con mensaje COMPUTADO: el AOT pasa un string-handle BP (objeto
 * heap TYPE_ARRAY_I8 UTF-8) en vez de un literal C. Leemos sus bytes y
 * reusamos el camino de throw_runtime (construir RuntimeError + longjmp al
 * boundary de #186). NO retorna. */
static void h_throw_str(bpvm_t* vm, uint32_t msg_ref) {
    char buf[128];
    uint32_t n = 0;
    if (msg_ref != 0) {
        n = bpvm_read_u32_be(vm->memory + msg_ref);
        if (n > sizeof(buf) - 1) n = (uint32_t)(sizeof(buf) - 1);
        memcpy(buf, vm->memory + msg_ref + 4, n);
    }
    buf[n] = '\0';
    h_throw_runtime(vm, buf);   /* no retorna (longjmp al boundary) */
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
    /* H2 (V2): string = byte[] UTF-8 → emite los bytes directamente, sin
     * truncar (paridad con OP_PRINT_STRING del intérprete). */
    if (ref != 0) {
        uint32_t nbytes = bpvm_read_u32_be(vm->memory + ref);
        const char* p = (const char*)(vm->memory + ref + 4);
        if (nbytes > 0) {
            if (vm->output_cb) vm->output_cb(p, (size_t) nbytes, vm->output_user);
            else fwrite(p, 1, (size_t) nbytes, stdout);
        }
    }
    if (nl) {
        if (vm->output_cb) vm->output_cb("\n", 1, vm->output_user);
        else fputc('\n', stdout);
    }
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

/* ---------- Builtins (H3 #168) ----------
 * Wrappers de los OP_CALL_BUILTIN más usados, callables como C
 * directo desde código AOT. */
extern int64_t bpvm_platform_now_ms(void);
static int32_t h_now_ms(bpvm_t* vm) {
    (void) vm;
    return (int32_t) bpvm_platform_now_ms();
}

/* ---------- Variables nivel-módulo (#172) ----------
 * Las module-globals son INTERNAS al módulo (no se exportan en la
 * sección 4.2 del .mod). Su dirección absoluta es CS + offset, donde
 * offset es fijo en tiempo de compilación (lo decide el bytecode
 * emitter y AotCEmitter lo bakea como literal C). El CS sí es runtime,
 * por lo que cada thunk resuelve UNA vez la CS del módulo al que
 * pertenece y la cachea.
 *
 * Acceso al campo `modules[]` directo: el `name` de bpvm_module_t es
 * el nombre lógico ("GlobalsAot") sin librería. */
static uint32_t h_find_module_cs(bpvm_t* vm, const char* module_name) {
    if (!vm || !module_name) return 0;
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (strcmp(m->name, module_name) == 0) {
            return m->code_start;
        }
    }
    return 0;
}

/* ---- Puente native→BP (P-aot-call-bp) ----
 * find_function: resuelve un nombre cualificado a su dirección absoluta vía
 * la tabla global de símbolos (la misma que usa bpvm_aot_register_by_name).
 * Devuelve 0 si no existe. El thunk lo llama UNA vez y cachea el resultado.
 * (call_bp_i32 vive en interp.c — necesita las convenciones de frame.) */
static uint32_t h_find_function(bpvm_t* vm, const char* qualified) {
    if (!vm || !qualified) return 0;
    for (int i = 0; i < vm->symbol_count; i++) {
        if (strcmp(vm->symbols[i].name, qualified) == 0) {
            return vm->symbols[i].abs_addr;
        }
    }
    return 0;
}

/* ---------- Strings (H3 #173, H2 V2) ----------
 * String heap = [byte_len:u32 BE][bytes UTF-8] (TYPE_ARRAY_I8). Índice por
 * codepoint vía helpers utf8_* (bpvm_internal.h). Reusan bpvm_heap_alloc /
 * bpvm_heap_alloc_string del runtime. DEBEN coincidir con builtins.c. */
static int32_t h_string_length(bpvm_t* vm, uint32_t ref) {
    if (ref == 0) return 0;
    uint32_t nbytes = bpvm_read_u32_be(vm->memory + ref);
    return (int32_t) utf8_cp_count(vm->memory + ref + 4, nbytes);
}
static int32_t h_string_char_code_at(bpvm_t* vm, uint32_t ref, int32_t idx) {
    if (ref == 0) return 0;
    uint32_t nbytes = bpvm_read_u32_be(vm->memory + ref);
    const uint8_t* p = vm->memory + ref + 4;
    uint32_t ncp = utf8_cp_count(p, nbytes);
    if (idx < 0 || (uint32_t) idx >= ncp) return 0;
    uint32_t off = utf8_byte_offset(p, nbytes, (uint32_t) idx);
    uint32_t adv; return (int32_t) utf8_decode(p + off, nbytes - off, &adv);
}
static uint32_t h_string_char_at(bpvm_t* vm, uint32_t ref, int32_t idx) {
    uint8_t enc[4]; uint32_t enc_len = 0;
    if (ref != 0) {
        uint32_t nbytes = bpvm_read_u32_be(vm->memory + ref);
        const uint8_t* p = vm->memory + ref + 4;
        uint32_t ncp = utf8_cp_count(p, nbytes);
        if (idx >= 0 && (uint32_t) idx < ncp) {
            uint32_t off = utf8_byte_offset(p, nbytes, (uint32_t) idx);
            uint32_t adv; uint32_t cp = utf8_decode(p + off, nbytes - off, &adv);
            enc_len = utf8_encode(cp, enc);
        }
    }
    uint32_t out = bpvm_heap_alloc(vm, enc_len, BPVM_TYPE_ARRAY_I8);
    if (out) {
        bpvm_write_u32_be(vm->memory + out, enc_len);
        for (uint32_t k = 0; k < enc_len; k++) vm->memory[out + 4 + k] = enc[k];
    }
    return out;
}
static uint32_t h_string_concat(bpvm_t* vm, uint32_t a, uint32_t b) {
    uint32_t la = a ? bpvm_read_u32_be(vm->memory + a) : 0;   /* bytes */
    uint32_t lb = b ? bpvm_read_u32_be(vm->memory + b) : 0;
    uint32_t out = bpvm_heap_alloc(vm, la + lb, BPVM_TYPE_ARRAY_I8);
    if (!out) return 0;
    uint8_t* mem = vm->memory;   /* F2 no compacta: a/b siguen válidos */
    bpvm_write_u32_be(mem + out, la + lb);
    for (uint32_t i = 0; i < la; i++) mem[out + 4 + i] = mem[a + 4 + i];
    for (uint32_t i = 0; i < lb; i++) mem[out + 4 + la + i] = mem[b + 4 + i];
    return out;
}
static uint32_t h_string_substring(bpvm_t* vm, uint32_t ref, int32_t from, int32_t to) {
    if (ref == 0) return 0;
    uint32_t nbytes = bpvm_read_u32_be(vm->memory + ref);
    const uint8_t* p = vm->memory + ref + 4;
    uint32_t ncp = utf8_cp_count(p, nbytes);   /* índices en codepoints */
    if (from < 0) from = 0;
    if (to < 0)   to = 0;
    if ((uint32_t) to > ncp) to = (int32_t) ncp;
    if (from > to) from = to;
    uint32_t boff = utf8_byte_offset(p, nbytes, (uint32_t) from);
    uint32_t eoff = utf8_byte_offset(p, nbytes, (uint32_t) to);
    uint32_t n = eoff - boff;
    uint32_t out = bpvm_heap_alloc(vm, n, BPVM_TYPE_ARRAY_I8);
    if (!out) return 0;
    uint8_t* mem = vm->memory;
    bpvm_write_u32_be(mem + out, n);
    for (uint32_t i = 0; i < n; i++) mem[out + 4 + i] = mem[ref + 4 + boff + i];
    return out;
}
static int32_t h_string_eq(bpvm_t* vm, uint32_t a, uint32_t b) {
    if (a == b) return 1;              /* misma ref (incl. ambos null) */
    if (a == 0 || b == 0) return 0;
    uint8_t* mem = vm->memory;
    uint32_t la = bpvm_read_u32_be(mem + a);   /* bytes */
    uint32_t lb = bpvm_read_u32_be(mem + b);
    if (la != lb) return 0;
    for (uint32_t i = 0; i < la; i++)
        if (mem[a + 4 + i] != mem[b + 4 + i]) return 0;
    return 1;
}
static uint32_t h_string_from_cstr(bpvm_t* vm, const char* s, int32_t len) {
    if (!s || len < 0) return 0;
    return bpvm_heap_alloc_string(vm, s, (size_t) len);
}
static uint32_t h_int_to_string(bpvm_t* vm, int32_t v) {
    char buf[16];
    int n = snprintf(buf, sizeof(buf), "%d", (int) v);
    return bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
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
    .now_ms          = h_now_ms,
    .find_module_cs  = h_find_module_cs,
    .string_length       = h_string_length,
    .string_char_code_at = h_string_char_code_at,
    .string_char_at      = h_string_char_at,
    .string_concat       = h_string_concat,
    .string_substring    = h_string_substring,
    .string_eq           = h_string_eq,
    .string_from_cstr    = h_string_from_cstr,
    .int_to_string       = h_int_to_string,
    /* Puente native→BP (P-aot-call-bp). call_bp_i32 vive en interp.c. */
    .find_function       = h_find_function,
    .call_bp_i32         = bpvm_aot_call_bp_i32,
    /* #175 — throw con mensaje computado. */
    .throw_str           = h_throw_str,
    /* #174 (mitad-VM) — despacho virtual desde native. Vive en interp.c. */
    .call_method_i32     = bpvm_aot_call_method_i32,
};
