/*
 * aot_NatNew.c — AUTOGENERADO por AotCEmitter (H3 #157).
 * NO EDITAR A MANO. Regenerar compilando NatNew.bp con --aot.
 *
 * Funciones BP marcadas con `function native ...` traducidas a C.
 * El bytecode .mod se sigue generando normalmente; el runtime
 * decide qué versión usar via aot_registry tras link.
 */

#include "aot_registry.h"
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"   /* H3 #158 — helpers indirect */

/* Forward decls de las funciones AOT de este módulo. */
static int32_t aot_NatNew_areaDe(struct bpvm* vm, int32_t a, int32_t b);
static double aot_NatNew_mediaDobles(struct bpvm* vm);
static float aot_NatNew_sumaFloats(struct bpvm* vm);
static int64_t aot_NatNew_sumaLargos(struct bpvm* vm);
static int32_t aot_NatNew_tocaPalabras(struct bpvm* vm, int32_t v);
static int32_t aot_NatNew_largoDeDos(struct bpvm* vm, int32_t a, int32_t b);
static int32_t aot_NatNew_sumaArray(struct bpvm* vm, int32_t n);

static int32_t aot_NatNew_sumaArray(struct bpvm* vm, int32_t n) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) n;
    int32_t v = ({ int32_t __arr = vm->aot_helpers->newarray_i32(vm, 4); vm->aot_helpers->array_store_i32(vm, (uint32_t) __arr, 0, 10); vm->aot_helpers->array_store_i32(vm, (uint32_t) __arr, 1, 20); vm->aot_helpers->array_store_i32(vm, (uint32_t) __arr, 2, 30); vm->aot_helpers->array_store_i32(vm, (uint32_t) __arr, 3, 40); __arr; });
    int32_t t = 0;
    int32_t i = 0;
    while ((i < 4)) {
        t += vm->aot_helpers->array_load_i32(vm, v, i);
        i += 1;
    }
    return (t + n);
}

static void thunk_NatNew_sumaArray(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t r = aot_NatNew_sumaArray(vm, a0);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

static int64_t aot_NatNew_sumaLargos(struct bpvm* vm) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t v = ({ int32_t __arr = vm->aot_helpers->newarray_i64(vm, 2); vm->aot_helpers->array_store_i64(vm, (uint32_t) __arr, 0, 3000000000LL); vm->aot_helpers->array_store_i64(vm, (uint32_t) __arr, 1, 4000000000LL); __arr; });
    return (vm->aot_helpers->array_load_i64(vm, v, 0) + vm->aot_helpers->array_load_i64(vm, v, 1));
}

static void thunk_NatNew_sumaLargos(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int64_t r = aot_NatNew_sumaLargos(vm);
    H->write_i64_be(mem + sp, r); sp += 8;  /* long: 8B */
    *sp_p = sp;
}

static int32_t aot_NatNew_tocaPalabras(struct bpvm* vm, int32_t v) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) v;
    vm->aot_helpers->array_store_i16(vm, v, 0, 1000);
    vm->aot_helpers->array_store_i16(vm, v, 1, 2000);
    vm->aot_helpers->array_store_i16(vm, v, 2, 3000);
    return ((vm->aot_helpers->array_load_u16(vm, v, 0) + vm->aot_helpers->array_load_u16(vm, v, 1)) + vm->aot_helpers->array_load_u16(vm, v, 2));
}

static void thunk_NatNew_tocaPalabras(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a0 = (int32_t) H->read_ref(mem + sp - 8); sp -= 8;  /* ref: 8B */
    int32_t r = aot_NatNew_tocaPalabras(vm, a0);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

static double aot_NatNew_mediaDobles(struct bpvm* vm) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t d = vm->aot_helpers->newarray_i64(vm, 2);
    vm->aot_helpers->array_store_f64(vm, d, 0, 3.0);
    vm->aot_helpers->array_store_f64(vm, d, 1, 5.0);
    return vm->aot_helpers->ddiv((double)((vm->aot_helpers->dadd((double)(vm->aot_helpers->array_load_f64(vm, d, 0)), (double)(vm->aot_helpers->array_load_f64(vm, d, 1))))), (double)(2.0));
}

static void thunk_NatNew_mediaDobles(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    double r = aot_NatNew_mediaDobles(vm);
    H->write_f64_be(mem + sp, r); sp += 8;  /* double: 8B */
    *sp_p = sp;
}

static int32_t aot_NatNew_largoDeDos(struct bpvm* vm, int32_t a, int32_t b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) a;
    (void) b;
    int32_t v = ({ int32_t __arr = vm->aot_helpers->newarray_ref(vm, 3); vm->aot_helpers->array_store_ref(vm, (uint32_t) __arr, 0, vm->aot_helpers->int_to_string(vm, a)); vm->aot_helpers->array_store_ref(vm, (uint32_t) __arr, 1, vm->aot_helpers->int_to_string(vm, b)); vm->aot_helpers->array_store_ref(vm, (uint32_t) __arr, 2, vm->aot_helpers->string_from_cstr(vm, "xy", 2)); __arr; });
    return ((vm->aot_helpers->string_length(vm, (uint32_t) (vm->aot_helpers->array_load_ref(vm, v, 0))) + vm->aot_helpers->string_length(vm, (uint32_t) (vm->aot_helpers->array_load_ref(vm, v, 1)))) + vm->aot_helpers->string_length(vm, (uint32_t) (vm->aot_helpers->array_load_ref(vm, v, 2))));
}

static void thunk_NatNew_largoDeDos(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a1 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t r = aot_NatNew_largoDeDos(vm, a0, a1);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

static float aot_NatNew_sumaFloats(struct bpvm* vm) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t f = ({ int32_t __arr = vm->aot_helpers->newarray_i32(vm, 2); vm->aot_helpers->array_store_f32(vm, (uint32_t) __arr, 0, 1.5f); vm->aot_helpers->array_store_f32(vm, (uint32_t) __arr, 1, 2.5f); __arr; });
    return (vm->aot_helpers->array_load_f32(vm, f, 0) + vm->aot_helpers->array_load_f32(vm, f, 1));
}

static void thunk_NatNew_sumaFloats(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    float r = aot_NatNew_sumaFloats(vm);
    H->write_f32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

static int32_t aot_NatNew_areaDe(struct bpvm* vm, int32_t a, int32_t b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) a;
    (void) b;
    int32_t p = vm->aot_helpers->call_bp_i32(vm, vm->aot_helpers->find_function(vm, "NatNew.__cls_new_Punto"), (int32_t[]){ a, b }, 2, 0u, 1);
    return (vm->aot_helpers->call_method_i32(vm, p, 2, (const int32_t*) 0, 0, 0u, 0) * vm->aot_helpers->call_method_i32(vm, p, 4, (const int32_t*) 0, 0, 0u, 0));
}

static void thunk_NatNew_areaDe(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a1 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t r = aot_NatNew_areaDe(vm, a0, a1);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

/* Registra todas las funciones AOT de este módulo en el AOT
 * registry. Llamar tras link, antes de bpvm_run. Tolerante a
 * símbolos ausentes (skip silente si el .mod no está cargado). */
void aot_NatNew_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "NatNew.sumaArray", thunk_NatNew_sumaArray);
    bpvm_aot_register_by_name(vm, "NatNew.sumaLargos", thunk_NatNew_sumaLargos);
    bpvm_aot_register_by_name(vm, "NatNew.tocaPalabras", thunk_NatNew_tocaPalabras);
    bpvm_aot_register_by_name(vm, "NatNew.mediaDobles", thunk_NatNew_mediaDobles);
    bpvm_aot_register_by_name(vm, "NatNew.largoDeDos", thunk_NatNew_largoDeDos);
    bpvm_aot_register_by_name(vm, "NatNew.sumaFloats", thunk_NatNew_sumaFloats);
    bpvm_aot_register_by_name(vm, "NatNew.areaDe", thunk_NatNew_areaDe);
}

