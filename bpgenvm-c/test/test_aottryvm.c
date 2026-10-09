/*
 * test_aottryvm.c — [V7/#578] el `try` DENTRO de una native: el lado de la VM, con natives ESCRITAS A MANO.
 *
 * El C de abajo es el que emitirá el AOT (docs/V7_IDEAS.md §«#578», el paso 1): el aterrizaje (`void* t[7]` en el
 * marco de la native, con su __builtin_setjmp), las cuatro operaciones del límite por call_builtin (EH_PUSH, EH_POP,
 * EH_EXC, EH_ES) y el salto con __builtin_longjmp desde una función de ESTE fichero (`salta`): el mismo GCC a los dos
 * lados. Las funciones de samples/NatTryVm.bp son BP; aquí se les pone su native, y la receta (`make test-aottryvm`)
 * exige la salida del intérprete (`--interp`), a -O0, -O2 y -Os. Con `--desparejado`, la 10 cierra un aterrizaje que
 * no es el más interno: el error, con su nombre.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include "aot_registry.h"
#include "bpvm_aot_helpers.h"
#include "builtin_ids.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* la prueba cuenta que las natives SE USARON y que aterrizaron: con el mismo resultado, si no se registraran correría el
 * bytecode y la salida sería igual */
static int s_llamadas = 0, s_aterrizajes = 0;

/* ── lo que emitirá el AOT, una vez por módulo ── */
static void salta(void** t) __attribute__((noreturn, noinline));
static void salta(void** t) { s_aterrizajes++; __builtin_longjmp(t, 1); }

#define ATERRIZAJE(t) void* t[BPVM_ATERRIZAJE_PALABRAS]; t[BPVM_ATERRIZAJE_SALTA] = (void*) (uintptr_t) salta
#define EH(vm, op, t) do { int64_t a_[1]; a_[0] = (int64_t) (intptr_t) (t); \
                           (vm)->aot_helpers->call_builtin((vm), (op), "l", a_, 'v'); } while (0)

static uint32_t eh_exc(struct bpvm* vm) {
    return (uint32_t) vm->aot_helpers->call_builtin(vm, BPVM_NATIVA_EH_EXC, "", NULL, 'r');
}
static int32_t eh_es(struct bpvm* vm, uint32_t e, const char* clase) {
    int64_t a[2];
    a[0] = (int64_t) e;
    a[1] = (int64_t) (intptr_t) clase;
    return (int32_t) vm->aot_helpers->call_builtin(vm, BPVM_NATIVA_EH_ES, "rl", a, 'i');
}
static uint32_t busca(struct bpvm* vm, const char* nombre) {
    uint32_t f = vm->aot_helpers->find_function(vm, nombre);
    if (f == 0) vm->aot_helpers->throw_runtime(vm, "test_aottryvm: falta una funcion BP");
    return f;
}

/* 1. throw_runtime dentro del try; `r` cambia dentro y se lee en el catch */
static int32_t n_t1(struct bpvm* vm, int32_t x) {
    int32_t r = 0;
    {   ATERRIZAJE(t0);
        if (__builtin_setjmp(t0) == 0) {
            EH(vm, BPVM_NATIVA_EH_PUSH, t0);
            r = 10;
            if (x > 0) vm->aot_helpers->throw_runtime(vm, "t1");
            r = 20;
            EH(vm, BPVM_NATIVA_EH_POP, t0);
        } else {
            uint32_t e0 = eh_exc(vm);
            if (eh_es(vm, e0, "Core.RuntimeError")) { r = r + 100; }
            else vm->aot_helpers->throw_ref(vm, e0);
        }
    }
    return r;
}

/* 2. lo lanza un builtin (charAt fuera de rango), por call_builtin */
static int32_t n_t2(struct bpvm* vm, int32_t i) {
    int32_t r = 1;
    {   ATERRIZAJE(t0);
        if (__builtin_setjmp(t0) == 0) {
            int64_t a[2];
            EH(vm, BPVM_NATIVA_EH_PUSH, t0);
            a[0] = (int64_t) vm->aot_helpers->string_from_cstr(vm, "abc", 3);
            a[1] = i;
            (void) vm->aot_helpers->call_builtin(vm, BUILTIN_CHAR_AT, "ri", a, 'r');
            r = 2;
            EH(vm, BPVM_NATIVA_EH_POP, t0);
        } else {
            uint32_t e0 = eh_exc(vm);
            if (eh_es(vm, e0, "Core.RuntimeError")) r = 3;
            else vm->aot_helpers->throw_ref(vm, e0);
        }
    }
    return r;
}

/* 3. lo lanza una función BP por el puente: MiError, por su clase (la primera que casa) */
static int32_t n_t3(struct bpvm* vm, int32_t x) {
    int32_t r = 0;
    {   ATERRIZAJE(t0);
        if (__builtin_setjmp(t0) == 0) {
            EH(vm, BPVM_NATIVA_EH_PUSH, t0);
            r = vm->aot_helpers->call_bp_i32(vm, busca(vm, "NatTryVm.lanzaBP"), (int32_t[]){ x }, 1, 0u, 0);
            EH(vm, BPVM_NATIVA_EH_POP, t0);
        } else {
            uint32_t e0 = eh_exc(vm);
            if (eh_es(vm, e0, "NatTryVm.MiError")) r = -1;
            else if (eh_es(vm, e0, "Core.RuntimeError")) r = -2;
            else vm->aot_helpers->throw_ref(vm, e0);
        }
    }
    return r;
}

/* 4. un catch que no es suyo la relanza; la atrapa el try de fuera, en la misma native */
static int32_t n_t4(struct bpvm* vm, int32_t x) {
    int32_t r = 0;
    (void) x;
    {   ATERRIZAJE(t0);
        if (__builtin_setjmp(t0) == 0) {
            EH(vm, BPVM_NATIVA_EH_PUSH, t0);
            {   ATERRIZAJE(t1);
                if (__builtin_setjmp(t1) == 0) {
                    EH(vm, BPVM_NATIVA_EH_PUSH, t1);
                    r = 1;
                    vm->aot_helpers->throw_runtime(vm, "t4");
                    EH(vm, BPVM_NATIVA_EH_POP, t1);
                } else {
                    uint32_t e1 = eh_exc(vm);
                    if (eh_es(vm, e1, "NatTryVm.MiError")) r = 50;
                    else vm->aot_helpers->throw_ref(vm, e1);
                }
            }
            EH(vm, BPVM_NATIVA_EH_POP, t0);
        } else {
            uint32_t e0 = eh_exc(vm);
            if (eh_es(vm, e0, "Core.RuntimeError")) r = r + 40;
            else vm->aot_helpers->throw_ref(vm, e0);
        }
    }
    return r;
}

/* 5. de una native a otra, sin el intérprete: lanza la de dentro, atrapa la de fuera; `return` en el try y el catch */
static int32_t n_interior5(struct bpvm* vm, int32_t x) {
    if (x > 1) vm->aot_helpers->throw_runtime(vm, "t5");
    return x * 3;
}
static int32_t n_t5(struct bpvm* vm, int32_t x) {
    {   ATERRIZAJE(t0);
        if (__builtin_setjmp(t0) == 0) {
            EH(vm, BPVM_NATIVA_EH_PUSH, t0);
            {   int32_t ret = n_interior5(vm, x) + 1;
                EH(vm, BPVM_NATIVA_EH_POP, t0);
                return ret;
            }
            EH(vm, BPVM_NATIVA_EH_POP, t0);
        } else {
            uint32_t e0 = eh_exc(vm);
            if (eh_es(vm, e0, "Core.RuntimeError")) { return -5; }
            else vm->aot_helpers->throw_ref(vm, e0);
        }
    }
    return 0;
}

/* 6. la native llama a BP, que llama a OTRA native que lanza sin try: cruza su límite y el suelo del puente (#672) */
static int32_t n_lanzaNativa(struct bpvm* vm, int32_t x) {
    if (x > 0) vm->aot_helpers->throw_runtime(vm, "t6");
    return 1;
}
static int32_t n_t6(struct bpvm* vm, int32_t x) {
    int32_t r = 5;
    {   ATERRIZAJE(t0);
        if (__builtin_setjmp(t0) == 0) {
            EH(vm, BPVM_NATIVA_EH_PUSH, t0);
            r = vm->aot_helpers->call_bp_i32(vm, busca(vm, "NatTryVm.puente6"), (int32_t[]){ x }, 1, 0u, 0);
            EH(vm, BPVM_NATIVA_EH_POP, t0);
        } else {
            uint32_t e0 = eh_exc(vm);
            if (eh_es(vm, e0, "Core.RuntimeError")) r = r * 10;
            else vm->aot_helpers->throw_ref(vm, e0);
        }
    }
    return r;
}

/* 7. un try en un bucle (GC en cada reserva): `s` cambia dentro del try y en el catch, `i` fuera */
static int32_t n_t7(struct bpvm* vm, int32_t n) {
    int32_t s = 0, i = 0;
    while (i < n) {
        {   ATERRIZAJE(t0);
            if (__builtin_setjmp(t0) == 0) {
                EH(vm, BPVM_NATIVA_EH_PUSH, t0);
                if (i % 3 == 0) {
                    const struct aot_helpers_v2* H = vm->aot_helpers;
                    H->throw_str(vm, H->string_concat(vm, H->string_from_cstr(vm, "t7 ", 3), H->int_to_string(vm, i)));
                }
                s += i;
                EH(vm, BPVM_NATIVA_EH_POP, t0);
            } else {
                uint32_t e0 = eh_exc(vm);
                if (eh_es(vm, e0, "Core.RuntimeError")) s += 1000;
                else vm->aot_helpers->throw_ref(vm, e0);
            }
        }
        i += 1;
    }
    return s;
}

/* 8. lo que lanza un catch va hacia fuera: su aterrizaje ya no está */
static int32_t n_t8(struct bpvm* vm, int32_t x) {
    (void) x;
    {   ATERRIZAJE(t0);
        if (__builtin_setjmp(t0) == 0) {
            EH(vm, BPVM_NATIVA_EH_PUSH, t0);
            vm->aot_helpers->throw_runtime(vm, "t8 dentro");
            EH(vm, BPVM_NATIVA_EH_POP, t0);
        } else {
            uint32_t e0 = eh_exc(vm);
            if (eh_es(vm, e0, "Core.RuntimeError")) vm->aot_helpers->throw_runtime(vm, "t8 desde el catch");
            else vm->aot_helpers->throw_ref(vm, e0);
        }
    }
    return 0;
}

/* 9. la excepción que aterriza, entera: la construye la factoría (por el puente) y el catch lee su mensaje */
static uint32_t n_t9(struct bpvm* vm, int32_t x) {
    const struct aot_helpers_v2* H = vm->aot_helpers;
    {   ATERRIZAJE(t0);
        if (__builtin_setjmp(t0) == 0) {
            uint32_t m, ex;
            EH(vm, BPVM_NATIVA_EH_PUSH, t0);
            m = H->string_concat(vm, H->string_from_cstr(vm, "nueve ", 6), H->int_to_string(vm, x));
            ex = (uint32_t) H->call_bp_i32(vm, busca(vm, "NatTryVm.__cls_new_MiError(s)"), (int32_t[]){ (int32_t) m },
                                           1, 1u, 1);
            H->throw_ref(vm, ex);
            EH(vm, BPVM_NATIVA_EH_POP, t0);
        } else {
            uint32_t e0 = eh_exc(vm);
            if (eh_es(vm, e0, "Core.RuntimeError")) {
                int32_t slot = H->slot_por_clave(vm, "Core.Exception", "getMsg()s", "NatTryVm");
                uint32_t msg;
                if (slot < 0) H->throw_runtime(vm, "test_aottryvm: sin la ranura de getMsg");
                msg = (uint32_t) H->call_method_i32(vm, e0, slot, (const int32_t*) 0, 0, 0u, 1);
                return H->string_concat(vm, H->string_from_cstr(vm, "t9 ", 3), msg);
            }
            else H->throw_ref(vm, e0);
        }
    }
    return H->string_from_cstr(vm, "FALLO 9", 7);
}

/* 10. con --desparejado: cierra un aterrizaje que no es el más interno (un error del emisor): un error con nombre */
static int32_t n_t10(struct bpvm* vm, int32_t x) { (void) vm; return x; }
static int32_t n_t10_mal(struct bpvm* vm, int32_t x) {
    ATERRIZAJE(t0);
    ATERRIZAJE(t1);
    if (__builtin_setjmp(t0) == 0) {
        EH(vm, BPVM_NATIVA_EH_PUSH, t0);
        EH(vm, BPVM_NATIVA_EH_POP, t1);   /* t1 nunca se apiló */
        EH(vm, BPVM_NATIVA_EH_POP, t0);
    } else {
        uint32_t e0 = eh_exc(vm);
        vm->aot_helpers->throw_ref(vm, e0);
    }
    return x;
}

/* ── los thunks: la pila BP ↔ C, como los del AOT ── */
#define THUNK_II(th, fn) \
    static void th(struct bpvm* vm, uint32_t* sp_p, uint32_t* bp_p) { \
        const struct aot_helpers_v2* H = vm->aot_helpers; uint8_t* mem = vm->memory; uint32_t sp = *sp_p; \
        int32_t a0, r; (void) bp_p; s_llamadas++; \
        a0 = H->read_i32_be(mem + sp - 4); sp -= 4; \
        r = fn(vm, a0); \
        H->write_i32_be(mem + sp, r); sp += 4; *sp_p = sp; }
THUNK_II(th_t1, n_t1)
THUNK_II(th_t2, n_t2)
THUNK_II(th_t3, n_t3)
THUNK_II(th_t4, n_t4)
THUNK_II(th_interior5, n_interior5)
THUNK_II(th_t5, n_t5)
THUNK_II(th_lanzaNativa, n_lanzaNativa)
THUNK_II(th_t6, n_t6)
THUNK_II(th_t7, n_t7)
THUNK_II(th_t8, n_t8)
THUNK_II(th_t10, n_t10)
THUNK_II(th_t10_mal, n_t10_mal)
static void th_t9(struct bpvm* vm, uint32_t* sp_p, uint32_t* bp_p) {
    const struct aot_helpers_v2* H = vm->aot_helpers; uint8_t* mem = vm->memory; uint32_t sp = *sp_p;
    int32_t a0; uint32_t r; (void) bp_p; s_llamadas++;
    a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    r = n_t9(vm, a0);
    H->write_ref(vm, mem + sp, r); sp += 8;   /* ref: 8 B */
    *sp_p = sp;
}

static void registra(struct bpvm* vm, int desparejado) {
    /* las que sólo llaman a helpers y builtins, DIRECTAS (en la pila del worker); las que llaman a BP o tienen un
     * bucle, en su ejecutor (P2b.3) */
    bpvm_aot_register_by_name_flags(vm, "NatTryVm.t1", th_t1, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name_flags(vm, "NatTryVm.t2", th_t2, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name(vm, "NatTryVm.t3", th_t3);
    bpvm_aot_register_by_name_flags(vm, "NatTryVm.t4", th_t4, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name_flags(vm, "NatTryVm.interior5", th_interior5, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name_flags(vm, "NatTryVm.t5", th_t5, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name_flags(vm, "NatTryVm.lanzaNativa", th_lanzaNativa, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name(vm, "NatTryVm.t6", th_t6);
    bpvm_aot_register_by_name(vm, "NatTryVm.t7", th_t7);
    bpvm_aot_register_by_name_flags(vm, "NatTryVm.t8", th_t8, BPVM_AOT_DIRECTA);
    bpvm_aot_register_by_name(vm, "NatTryVm.t9", th_t9);
    if (desparejado) bpvm_aot_register_by_name_flags(vm, "NatTryVm.t10", th_t10_mal, BPVM_AOT_DIRECTA);
    else             bpvm_aot_register_by_name_flags(vm, "NatTryVm.t10", th_t10, BPVM_AOT_DIRECTA);
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "NatTryVm.mod";
    const char* modo = (argc > 2) ? argv[2] : "";
    int interp = strcmp(modo, "--interp") == 0;             /* sin natives: el bytecode */
    int desparejado = strcmp(modo, "--desparejado") == 0;
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    bpvm_status_t s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    if (!interp) registra(vm, desparejado);
    vm->gc_bump_threshold = 1;     /* GC en CADA alocación: la excepción que aterriza tiene que seguir viva */

    s = bpvm_run(vm);
    fprintf(stderr, "[nativas=%d aterrizajes=%d]\n", s_llamadas, s_aterrizajes);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
