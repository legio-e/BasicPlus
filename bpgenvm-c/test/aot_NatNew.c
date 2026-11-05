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

/* V7/H2 (#523) — busca un símbolo BP por nombre; si no está, lo DICE (no un 0 mudo). */
static inline uint32_t aot_NatNew_busca(struct bpvm* vm, const char* nombre, const char* aviso) {
    uint32_t f = vm->aot_helpers->find_function(vm, nombre);
    if (f == 0) vm->aot_helpers->throw_runtime(vm, aviso);   /* longjmp: no vuelve */
    return f;
}
/* V7/H5 — las conversiones estrechas (`byte(x)`…): el rango de OP_I32_TO_U8 y compañía, con su texto. */
static inline int32_t aot_NatNew_estrecha(struct bpvm* vm, int32_t v, int32_t min, int32_t max,
                                        const char* texto, int32_t largo) {
    if (v < min || v > max) {
        const struct aot_helpers_v2* H = vm->aot_helpers;
        uint32_t a = H->string_from_cstr(vm, texto, largo);
        uint32_t b = H->int_to_string(vm, v);
        H->throw_str(vm, H->string_concat(vm, a, b));   /* longjmp: no vuelve */
    }
    return v;
}
/* V7/H2 (#533) — la ranura de un método de clase externa/abierta, por clave y en ejecución; si no está, lo DICE. */
static inline int32_t aot_NatNew_ranura(struct bpvm* vm, const char* clase, const char* clave, const char* aviso) {
    int32_t s = vm->aot_helpers->slot_por_clave(vm, clase, clave, "NatNew");
    if (s < 0) vm->aot_helpers->throw_runtime(vm, aviso);   /* longjmp: no vuelve */
    return s;
}

/* Forward decls de las funciones AOT de este módulo. */
static int32_t aot_NatNew_areaDe(struct bpvm* vm, int32_t bp_a, int32_t bp_b);
static double aot_NatNew_mediaDobles(struct bpvm* vm);
static float aot_NatNew_sumaFloats(struct bpvm* vm);
static int64_t aot_NatNew_sumaLargos(struct bpvm* vm);
static int32_t aot_NatNew_tocaPalabras(struct bpvm* vm, int32_t bp_v);
static int32_t aot_NatNew_largoDeDos(struct bpvm* vm, int32_t bp_a, int32_t bp_b);
static int32_t aot_NatNew_sumaArray(struct bpvm* vm, int32_t bp_n);

static int32_t aot_NatNew_sumaArray(struct bpvm* vm, int32_t bp_n) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t __bpvm_cuenta = 1024;   /* P2b.1.d: la línea del bucle */
    (void) __bpvm_cuenta;
    volatile const uint32_t* __bpvm_pm = (volatile const uint32_t*) (vm->memory + BPVM_MARCA_TIEMPO_ADDR);   /* #592: la marca del 2º temporizador */
    uint32_t __bpvm_m = *__bpvm_pm;
    (void) bp_n;
    int32_t bp_v = ({ int32_t __arr = vm->aot_helpers->newarray_i32(vm, 4); vm->aot_helpers->array_store_i32(vm, (uint32_t) __arr, 0, 10); vm->aot_helpers->array_store_i32(vm, (uint32_t) __arr, 1, 20); vm->aot_helpers->array_store_i32(vm, (uint32_t) __arr, 2, 30); vm->aot_helpers->array_store_i32(vm, (uint32_t) __arr, 3, 40); __arr; });
    int32_t bp_t = 0;
    int32_t bp_i = 0;
    while ((bp_i < 4)) {
        if (__builtin_expect(*__bpvm_pm != __bpvm_m, 0)) { __bpvm_m = *__bpvm_pm; vm->aot_helpers->punto_control(vm); }   /* #592: la marca */
        bp_t = ((int32_t) ((uint32_t) (bp_t) + (uint32_t) (vm->aot_helpers->array_load_i32(vm, bp_v, bp_i))));
        bp_i = ((int32_t) ((uint32_t) (bp_i) + (uint32_t) (1)));
    }
    return ((int32_t) ((uint32_t) (bp_t) + (uint32_t) (bp_n)));
}

static int64_t aot_NatNew_sumaLargos(struct bpvm* vm) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t __bpvm_cuenta = 1024;   /* P2b.1.d: la línea del bucle */
    (void) __bpvm_cuenta;
    int32_t bp_v = ({ int32_t __arr = vm->aot_helpers->newarray_i64(vm, 2); vm->aot_helpers->array_store_i64(vm, (uint32_t) __arr, 0, 3000000000LL); vm->aot_helpers->array_store_i64(vm, (uint32_t) __arr, 1, 4000000000LL); __arr; });
    return ((int64_t) ((uint64_t) (vm->aot_helpers->array_load_i64(vm, bp_v, 0)) + (uint64_t) (vm->aot_helpers->array_load_i64(vm, bp_v, 1))));
}

static int32_t aot_NatNew_tocaPalabras(struct bpvm* vm, int32_t bp_v) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t __bpvm_cuenta = 1024;   /* P2b.1.d: la línea del bucle */
    (void) __bpvm_cuenta;
    (void) bp_v;
    vm->aot_helpers->array_store_i16(vm, bp_v, 0, 1000);
    vm->aot_helpers->array_store_i16(vm, bp_v, 1, 2000);
    vm->aot_helpers->array_store_i16(vm, bp_v, 2, 3000);
    return ((int32_t) ((uint32_t) (((int32_t) ((uint32_t) (vm->aot_helpers->array_load_u16(vm, bp_v, 0)) + (uint32_t) (vm->aot_helpers->array_load_u16(vm, bp_v, 1))))) + (uint32_t) (vm->aot_helpers->array_load_u16(vm, bp_v, 2))));
}

static double aot_NatNew_mediaDobles(struct bpvm* vm) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t __bpvm_cuenta = 1024;   /* P2b.1.d: la línea del bucle */
    (void) __bpvm_cuenta;
    int32_t bp_d = vm->aot_helpers->newarray_i64(vm, 2);
    vm->aot_helpers->array_store_f64(vm, bp_d, 0, 3.0);
    vm->aot_helpers->array_store_f64(vm, bp_d, 1, 5.0);
    return vm->aot_helpers->ddiv((vm->aot_helpers->dadd(vm->aot_helpers->array_load_f64(vm, bp_d, 0), vm->aot_helpers->array_load_f64(vm, bp_d, 1))), 2.0);
}

static int32_t aot_NatNew_largoDeDos(struct bpvm* vm, int32_t bp_a, int32_t bp_b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t __bpvm_cuenta = 1024;   /* P2b.1.d: la línea del bucle */
    (void) __bpvm_cuenta;
    (void) bp_a;
    (void) bp_b;
    int32_t bp_v = ({ int32_t __arr = vm->aot_helpers->newarray_ref(vm, 3); vm->aot_helpers->array_store_ref(vm, (uint32_t) __arr, 0, vm->aot_helpers->int_to_string(vm, bp_a)); vm->aot_helpers->array_store_ref(vm, (uint32_t) __arr, 1, vm->aot_helpers->int_to_string(vm, bp_b)); vm->aot_helpers->array_store_ref(vm, (uint32_t) __arr, 2, vm->aot_helpers->string_from_cstr(vm, "xy", 2)); __arr; });
    return ((int32_t) ((uint32_t) (((int32_t) ((uint32_t) (vm->aot_helpers->string_length(vm, (uint32_t) (vm->aot_helpers->array_load_ref(vm, bp_v, 0)))) + (uint32_t) (vm->aot_helpers->string_length(vm, (uint32_t) (vm->aot_helpers->array_load_ref(vm, bp_v, 1))))))) + (uint32_t) (vm->aot_helpers->string_length(vm, (uint32_t) (vm->aot_helpers->array_load_ref(vm, bp_v, 2))))));
}

static float aot_NatNew_sumaFloats(struct bpvm* vm) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t __bpvm_cuenta = 1024;   /* P2b.1.d: la línea del bucle */
    (void) __bpvm_cuenta;
    int32_t bp_f = ({ int32_t __arr = vm->aot_helpers->newarray_i32(vm, 2); vm->aot_helpers->array_store_f32(vm, (uint32_t) __arr, 0, 1.5f); vm->aot_helpers->array_store_f32(vm, (uint32_t) __arr, 1, 2.5f); __arr; });
    return BPVM_AOT_FADD(vm, vm->aot_helpers->array_load_f32(vm, bp_f, 0), vm->aot_helpers->array_load_f32(vm, bp_f, 1));
}

static int32_t aot_NatNew_areaDe(struct bpvm* vm, int32_t bp_a, int32_t bp_b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    int32_t __bpvm_cuenta = 1024;   /* P2b.1.d: la línea del bucle */
    (void) __bpvm_cuenta;
    (void) bp_a;
    (void) bp_b;
    int32_t bp_p = vm->aot_helpers->call_bp_i32(vm, aot_NatNew_busca(vm, "NatNew.__cls_new_Punto(ii)", "AOT: la native 'areaDe' pide 'NatNew.__cls_new_Punto(ii)' y no est\303\241 en los m\303\263dulos cargados (\302\277otra versi\303\263n de la dependencia?): recompila 'NatNew'"), (int32_t[]){ bp_a, bp_b }, 2, 0u, 1);
    return ((int32_t) ((uint32_t) (vm->aot_helpers->call_method_i32(vm, bp_p, aot_NatNew_ranura(vm, "NatNew.Punto", "getX()i", "AOT: la native 'areaDe' pide 'NatNew.Punto.getX()i' y la clase cargada no lo tiene (\302\277otra versi\303\263n de la dependencia?): recompila 'NatNew'"), (const int32_t*) 0, 0, 0u, 0)) * (uint32_t) (vm->aot_helpers->call_method_i32(vm, bp_p, aot_NatNew_ranura(vm, "NatNew.Punto", "getY()i", "AOT: la native 'areaDe' pide 'NatNew.Punto.getY()i' y la clase cargada no lo tiene (\302\277otra versi\303\263n de la dependencia?): recompila 'NatNew'"), (const int32_t*) 0, 0, 0u, 0))));
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

/* P2b.3: DIRECTA — sin puntos de cesión: corre en la pila del worker, sin ejecutor. */
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

/* P2b.3: DIRECTA — sin puntos de cesión: corre en la pila del worker, sin ejecutor. */
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

/* P2b.3: DIRECTA — sin puntos de cesión: corre en la pila del worker, sin ejecutor. */
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

/* P2b.3: DIRECTA — sin puntos de cesión: corre en la pila del worker, sin ejecutor. */
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

/* P2b.3: DIRECTA — sin puntos de cesión: corre en la pila del worker, sin ejecutor. */
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
    bpvm_aot_register_by_name_flags(vm, "NatNew.sumaLargos", thunk_NatNew_sumaLargos, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name_flags(vm, "NatNew.tocaPalabras", thunk_NatNew_tocaPalabras, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name_flags(vm, "NatNew.mediaDobles", thunk_NatNew_mediaDobles, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name_flags(vm, "NatNew.largoDeDos", thunk_NatNew_largoDeDos, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name_flags(vm, "NatNew.sumaFloats", thunk_NatNew_sumaFloats, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name(vm, "NatNew.areaDe", thunk_NatNew_areaDe);
}

