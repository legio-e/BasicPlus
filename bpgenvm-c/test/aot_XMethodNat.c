/*
 * aot_XMethodNat.c — AUTOGENERADO por AotCEmitter (H3 #157).
 * NO EDITAR A MANO. Regenerar compilando XMethodNat.bp con --aot.
 *
 * Funciones BP marcadas con `function native ...` traducidas a C.
 * El bytecode .mod se sigue generando normalmente; el runtime
 * decide qué versión usar via aot_registry tras link.
 */

#include "aot_registry.h"
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"   /* H3 #158 — helpers indirect */

/* V7/H2 (#523) — busca un símbolo BP por nombre; si no está, lo DICE (no un 0 mudo). */
static inline uint32_t aot_XMethodNat_busca(struct bpvm* vm, const char* nombre, const char* aviso) {
    uint32_t f = vm->aot_helpers->find_function(vm, nombre);
    if (f == 0) vm->aot_helpers->throw_runtime(vm, aviso);   /* longjmp: no vuelve */
    return f;
}
/* V7/H2 (#533) — la ranura de un método de clase externa/abierta, por clave y en ejecución; si no está, lo DICE. */
static inline int32_t aot_XMethodNat_ranura(struct bpvm* vm, const char* clase, const char* clave, const char* aviso) {
    int32_t s = vm->aot_helpers->slot_por_clave(vm, clase, clave, "XMethodNat");
    if (s < 0) vm->aot_helpers->throw_runtime(vm, aviso);   /* longjmp: no vuelve */
    return s;
}

/* Forward decls de las funciones AOT de este módulo. */
static int32_t aot_XMethodNat_useBox(struct bpvm* vm, int32_t b);

static int32_t aot_XMethodNat_useBox(struct bpvm* vm, int32_t b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) b;
    return (vm->aot_helpers->call_method_i32(vm, b, aot_XMethodNat_ranura(vm, "XClassLib.Box", "getDoubled()i", "AOT: la native 'useBox' pide 'XClassLib.Box.getDoubled()i' y la clase cargada no lo tiene (\302\277otra versi\303\263n de la dependencia?): recompila 'XMethodNat'"), (const int32_t*) 0, 0, 0u, 0) + 1);
}

static void thunk_XMethodNat_useBox(struct bpvm* vm,
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
    int32_t r = aot_XMethodNat_useBox(vm, a0);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

/* Registra todas las funciones AOT de este módulo en el AOT
 * registry. Llamar tras link, antes de bpvm_run. Tolerante a
 * símbolos ausentes (skip silente si el .mod no está cargado). */
void aot_XMethodNat_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "XMethodNat.useBox", thunk_XMethodNat_useBox);
}

