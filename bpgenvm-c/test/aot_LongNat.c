/*
 * aot_LongNat.c — AUTOGENERADO por AotCEmitter (H3 #157).
 * NO EDITAR A MANO. Regenerar compilando LongNat.bp con --aot.
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
static int64_t aot_LongNat_acumula(struct bpvm* vm, int32_t veces, int64_t paso);
static int32_t aot_LongNat_bajaA32(struct bpvm* vm, int64_t v);
static int64_t aot_LongNat_sube(struct bpvm* vm, int32_t n, int64_t base);
static int64_t aot_LongNat_divL(struct bpvm* vm, int64_t a, int64_t b);
static int64_t aot_LongNat_modL(struct bpvm* vm, int64_t a, int64_t b);
static int64_t aot_LongNat_mulL(struct bpvm* vm, int64_t a, int64_t b);
static int64_t aot_LongNat_sumaL(struct bpvm* vm, int64_t a, int64_t b);
static int64_t aot_LongNat_mezcla(struct bpvm* vm, int32_t n, int64_t grande, int32_t m);

static int64_t aot_LongNat_sumaL(struct bpvm* vm, int64_t a, int64_t b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) a;
    (void) b;
    return (a + b);
}

static void thunk_LongNat_sumaL(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int64_t a1 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int64_t a0 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int64_t r = aot_LongNat_sumaL(vm, a0, a1);
    H->write_i64_be(mem + sp, r); sp += 8;  /* long: 8B */
    *sp_p = sp;
}

static int64_t aot_LongNat_mezcla(struct bpvm* vm, int32_t n, int64_t grande, int32_t m) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) n;
    (void) grande;
    (void) m;
    return ((grande + n) - m);
}

static void thunk_LongNat_mezcla(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a2 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int64_t a1 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int64_t r = aot_LongNat_mezcla(vm, a0, a1, a2);
    H->write_i64_be(mem + sp, r); sp += 8;  /* long: 8B */
    *sp_p = sp;
}

static int64_t aot_LongNat_mulL(struct bpvm* vm, int64_t a, int64_t b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) a;
    (void) b;
    return (a * b);
}

static void thunk_LongNat_mulL(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int64_t a1 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int64_t a0 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int64_t r = aot_LongNat_mulL(vm, a0, a1);
    H->write_i64_be(mem + sp, r); sp += 8;  /* long: 8B */
    *sp_p = sp;
}

static int64_t aot_LongNat_divL(struct bpvm* vm, int64_t a, int64_t b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) a;
    (void) b;
    return vm->aot_helpers->idiv64(vm, (int64_t)(a), (int64_t)(b));
}

static void thunk_LongNat_divL(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int64_t a1 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int64_t a0 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int64_t r = aot_LongNat_divL(vm, a0, a1);
    H->write_i64_be(mem + sp, r); sp += 8;  /* long: 8B */
    *sp_p = sp;
}

static int64_t aot_LongNat_modL(struct bpvm* vm, int64_t a, int64_t b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) a;
    (void) b;
    return vm->aot_helpers->imod64(vm, (int64_t)(a), (int64_t)(b));
}

static void thunk_LongNat_modL(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int64_t a1 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int64_t a0 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int64_t r = aot_LongNat_modL(vm, a0, a1);
    H->write_i64_be(mem + sp, r); sp += 8;  /* long: 8B */
    *sp_p = sp;
}

static int64_t aot_LongNat_acumula(struct bpvm* vm, int32_t veces, int64_t paso) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) veces;
    (void) paso;
    int64_t t = 0LL;
    int32_t i = 0;
    while ((i < veces)) {
        t = (t + paso);
        i = (i + 1);
    }
    return t;
}

static void thunk_LongNat_acumula(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int64_t a1 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int64_t r = aot_LongNat_acumula(vm, a0, a1);
    H->write_i64_be(mem + sp, r); sp += 8;  /* long: 8B */
    *sp_p = sp;
}

static int32_t aot_LongNat_bajaA32(struct bpvm* vm, int64_t v) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) v;
    return ((int32_t) (v));
}

static void thunk_LongNat_bajaA32(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int64_t a0 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int32_t r = aot_LongNat_bajaA32(vm, a0);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

static int64_t aot_LongNat_sube(struct bpvm* vm, int32_t n, int64_t base) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) n;
    (void) base;
    return (base + ((int64_t) (n)));
}

static void thunk_LongNat_sube(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v2* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int64_t a1 = H->read_i64_be(mem + sp - 8); sp -= 8;  /* long: 8B */
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int64_t r = aot_LongNat_sube(vm, a0, a1);
    H->write_i64_be(mem + sp, r); sp += 8;  /* long: 8B */
    *sp_p = sp;
}

/* Registra todas las funciones AOT de este módulo en el AOT
 * registry. Llamar tras link, antes de bpvm_run. Tolerante a
 * símbolos ausentes (skip silente si el .mod no está cargado). */
void aot_LongNat_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "LongNat.sumaL", thunk_LongNat_sumaL);
    bpvm_aot_register_by_name(vm, "LongNat.mezcla", thunk_LongNat_mezcla);
    bpvm_aot_register_by_name(vm, "LongNat.mulL", thunk_LongNat_mulL);
    bpvm_aot_register_by_name(vm, "LongNat.divL", thunk_LongNat_divL);
    bpvm_aot_register_by_name(vm, "LongNat.modL", thunk_LongNat_modL);
    bpvm_aot_register_by_name(vm, "LongNat.acumula", thunk_LongNat_acumula);
    bpvm_aot_register_by_name(vm, "LongNat.bajaA32", thunk_LongNat_bajaA32);
    bpvm_aot_register_by_name(vm, "LongNat.sube", thunk_LongNat_sube);
}

