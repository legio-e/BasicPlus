/*
 * aot_CompressBench.c — AUTOGENERADO por AotCEmitter (H3 #157).
 * NO EDITAR A MANO. Regenerar compilando CompressBench.bp con --aot.
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
static int32_t aot_CompressBench_decompress(struct bpvm* vm, int32_t src, int32_t srcLen, int32_t dst, int32_t dstCap);

static int32_t aot_CompressBench_decompress(struct bpvm* vm, int32_t src, int32_t srcLen, int32_t dst, int32_t dstCap) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) src;
    (void) srcLen;
    (void) dst;
    (void) dstCap;
    if ((srcLen < 4)) {
        return (-1);
    }
    int32_t outSize = (((((vm->aot_helpers->array_load_u8(vm, src, 0) << 24)) + ((vm->aot_helpers->array_load_u8(vm, src, 1) << 16))) + ((vm->aot_helpers->array_load_u8(vm, src, 2) << 8))) + vm->aot_helpers->array_load_u8(vm, src, 3));
    if ((outSize < 0)) {
        return (-1);
    }
    if ((outSize > dstCap)) {
        return (-1);
    }
    int32_t ip = 4;
    int32_t op = 0;
    while ((op < outSize)) {
        if ((ip >= srcLen)) {
            return (-1);
        }
        int32_t flags = vm->aot_helpers->array_load_u8(vm, src, ip);
        ip = (ip + 1);
        int32_t b = 0;
        while ((b < 8)) {
            if ((op >= outSize)) {
                return op;
            }
            if ((((((flags >> b)) % 2)) == 1)) {
                if ((ip >= srcLen)) {
                    return (-1);
                }
                vm->aot_helpers->array_store_i8(vm, dst, op, vm->aot_helpers->array_load_u8(vm, src, ip));
                ip = (ip + 1);
                op = (op + 1);
            } else {
                if ((((ip + 1)) >= srcLen)) {
                    return (-1);
                }
                int32_t b0 = vm->aot_helpers->array_load_u8(vm, src, ip);
                int32_t b1 = vm->aot_helpers->array_load_u8(vm, src, (ip + 1));
                ip = (ip + 2);
                int32_t off = (((b0 << 4)) + ((b1 >> 4)));
                int32_t leng = (((b1 % 16)) + 3);
                int32_t dist = (off + 1);
                if ((dist > op)) {
                    return (-1);
                }
                int32_t k = 0;
                while ((k < leng)) {
                    if ((op >= outSize)) {
                        return op;
                    }
                    vm->aot_helpers->array_store_i8(vm, dst, op, vm->aot_helpers->array_load_u8(vm, dst, (op - dist)));
                    op = (op + 1);
                    k = (k + 1);
                }
            }
            b = (b + 1);
        }
    }
    return op;
}

static void thunk_CompressBench_decompress(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v1* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a3 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t a2 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t a1 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t r = aot_CompressBench_decompress(vm, a0, a1, a2, a3);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

/* Registra todas las funciones AOT de este módulo en el AOT
 * registry. Llamar tras link, antes de bpvm_run. Tolerante a
 * símbolos ausentes (skip silente si el .mod no está cargado). */
void aot_CompressBench_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "CompressBench.decompress", thunk_CompressBench_decompress);
}

