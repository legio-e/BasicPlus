/*
 * bpvm_ctx.c — cambiar de pila (V7/H5 P2b.1.a). Ver bpvm_ctx.h para el porqué.
 */
#include "bpvm_ctx.h"
#include "bpvm_internal.h"   /* bpvm_diag_urgente */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if BPVM_CTX_DISPONIBLE && defined(__x86_64__) && defined(_WIN32)

/* ── x86-64, ABI de Windows (MinGW) ──────────────────────────────────────────────────────────────
 *
 * Lo que se conserva es lo que la ABI llama no volátil: rbx, rbp, rdi, rsi, r12-r15, xmm6-xmm15, los bits de
 * control del MXCSR y la palabra de control de la x87. Y, como hacen las fibras de Windows, los límites de
 * la pila que guarda el TIB —StackBase (gs:0x08), StackLimit (gs:0x10) y DeallocationStack (gs:0x1478)—:
 * el longjmp de MinGW deshace marcos con RtlUnwindEx, y éste rechaza un marco que no caiga entre esos
 * límites. Con la pila prestada fuera de ellos, el límite de fallos de #186 no funcionaría.
 *
 * El marco que queda en la pila de un contexto suspendido, desde su puntero de pila guardado:
 *
 *   +0x00 .. +0x9F   xmm6 .. xmm15 (movups: no hace falta alinear)
 *   +0xA0            MXCSR (4 B)            +0xA4  palabra de control x87 (2 B)
 *   +0xA8            TIB StackBase          +0xB0  TIB StackLimit          +0xB8  TIB DeallocationStack
 *   +0xC0 .. +0xF8   r15 r14 r13 r12 rdi rsi rbx rbp
 *   +0x100           la dirección de vuelta
 *
 * Al volver, `rax` lleva el dato (lo que devuelve bpvm_ctx_saltar) y `rcx` también: es el primer argumento
 * de fn cuando el destino es un contexto nuevo (bpvm_ctx_entrada). Tocar rcx es gratis: es volátil. */
__asm__(
    ".text\n"
    ".globl bpvm_ctx_saltar\n"
    ".def bpvm_ctx_saltar; .scl 2; .type 32; .endef\n"
    "bpvm_ctx_saltar:\n"                       /* rcx = guardar, rdx = destino, r8 = dato */
    "    pushq %rbp\n"
    "    pushq %rbx\n"
    "    pushq %rsi\n"
    "    pushq %rdi\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    "    subq $0xC0, %rsp\n"
    "    movups %xmm6,  0x00(%rsp)\n"
    "    movups %xmm7,  0x10(%rsp)\n"
    "    movups %xmm8,  0x20(%rsp)\n"
    "    movups %xmm9,  0x30(%rsp)\n"
    "    movups %xmm10, 0x40(%rsp)\n"
    "    movups %xmm11, 0x50(%rsp)\n"
    "    movups %xmm12, 0x60(%rsp)\n"
    "    movups %xmm13, 0x70(%rsp)\n"
    "    movups %xmm14, 0x80(%rsp)\n"
    "    movups %xmm15, 0x90(%rsp)\n"
    "    stmxcsr 0xA0(%rsp)\n"
    "    fnstcw  0xA4(%rsp)\n"
    "    movq %gs:0x08, %rax\n"
    "    movq %rax, 0xA8(%rsp)\n"
    "    movq %gs:0x10, %rax\n"
    "    movq %rax, 0xB0(%rsp)\n"
    "    movq %gs:0x1478, %rax\n"
    "    movq %rax, 0xB8(%rsp)\n"
    "    movq %rsp, (%rcx)\n"                  /* *guardar = el contexto que se deja */
    "    movq %rdx, %rsp\n"                    /* y a partir de aquí, la pila del destino */
    "    movups 0x00(%rsp), %xmm6\n"
    "    movups 0x10(%rsp), %xmm7\n"
    "    movups 0x20(%rsp), %xmm8\n"
    "    movups 0x30(%rsp), %xmm9\n"
    "    movups 0x40(%rsp), %xmm10\n"
    "    movups 0x50(%rsp), %xmm11\n"
    "    movups 0x60(%rsp), %xmm12\n"
    "    movups 0x70(%rsp), %xmm13\n"
    "    movups 0x80(%rsp), %xmm14\n"
    "    movups 0x90(%rsp), %xmm15\n"
    "    ldmxcsr 0xA0(%rsp)\n"
    "    fldcw   0xA4(%rsp)\n"
    "    movq 0xA8(%rsp), %rax\n"
    "    movq %rax, %gs:0x08\n"
    "    movq 0xB0(%rsp), %rax\n"
    "    movq %rax, %gs:0x10\n"
    "    movq 0xB8(%rsp), %rax\n"
    "    movq %rax, %gs:0x1478\n"
    "    addq $0xC0, %rsp\n"
    "    popq %r15\n"
    "    popq %r14\n"
    "    popq %r13\n"
    "    popq %r12\n"
    "    popq %rdi\n"
    "    popq %rsi\n"
    "    popq %rbx\n"
    "    popq %rbp\n"
    "    movq %r8, %rax\n"
    "    movq %r8, %rcx\n"
    "    ret\n"

    /* La primera vez que se salta a un contexto nuevo, el `ret` de arriba cae aquí con r12 = fn y rcx =
     * dato, y la pila en ≡ 8 (mod 16): se reservan los 32 B de sombra que pide la ABI y 8 más, para que el
     * `call` la deje alineada. fn no vuelve; si lo hiciera, bpvm_ctx_fn_volvio lo dice y para. */
    ".globl bpvm_ctx_entrada\n"
    ".def bpvm_ctx_entrada; .scl 2; .type 32; .endef\n"
    "bpvm_ctx_entrada:\n"
    "    subq $0x28, %rsp\n"
    "    callq *%r12\n"
    "    callq bpvm_ctx_fn_volvio\n"
    "    hlt\n"
);

extern void bpvm_ctx_entrada(void);

/* No `static`: la llama el ensamblador por su nombre. */
void bpvm_ctx_fn_volvio(void);
void bpvm_ctx_fn_volvio(void) {
    bpvm_diag_urgente("[bpvm] ctx: la función de un contexto VOLVIÓ (no debe: tiene que saltar a otro); se para");
    abort();
}

bpvm_ctx_t bpvm_ctx_crear(void* pila, size_t tam, void (*fn)(void* dato)) {
    if (pila == NULL || fn == NULL || tam < BPVM_CTX_PILA_MIN) return NULL;
    uintptr_t base = (uintptr_t) pila;
    uintptr_t tope = (base + tam) & ~(uintptr_t) 15u;
    uint8_t* a = (uint8_t*) (tope - 0x110u);   /* el marco de arriba + 8 B de relleno bajo el tope */
    uint32_t mxcsr;
    uint16_t fpcw;
    uint64_t v;
    memset(a, 0, 0x110u);
    /* El contexto nuevo hereda el entorno de coma flotante de quien lo crea (redondeo, excepciones). */
    __asm__ volatile ("stmxcsr %0" : "=m"(mxcsr));
    __asm__ volatile ("fnstcw %0" : "=m"(fpcw));
    memcpy(a + 0xA0, &mxcsr, 4);
    memcpy(a + 0xA4, &fpcw, 2);
    v = (uint64_t) tope;             memcpy(a + 0xA8, &v, 8);   /* TIB StackBase: el tope */
    v = (uint64_t) base;             memcpy(a + 0xB0, &v, 8);   /* TIB StackLimit: el fondo */
    v = (uint64_t) base;             memcpy(a + 0xB8, &v, 8);   /* TIB DeallocationStack */
    v = (uint64_t) (uintptr_t) fn;   memcpy(a + 0xD8, &v, 8);   /* r12 = fn */
    v = (uint64_t) (uintptr_t) bpvm_ctx_entrada;
    memcpy(a + 0x100, &v, 8);                                   /* el `ret` del primer salto va ahí */
    return (bpvm_ctx_t) a;
}

#else  /* sin implementación para esta arquitectura todavía (P2b.3) */

bpvm_ctx_t bpvm_ctx_crear(void* pila, size_t tam, void (*fn)(void* dato)) {
    (void) pila; (void) tam; (void) fn;
    return NULL;
}

void* bpvm_ctx_saltar(bpvm_ctx_t* guardar, bpvm_ctx_t destino, void* dato) {
    (void) guardar; (void) destino; (void) dato;
    bpvm_diag_urgente("[bpvm] ctx: esta arquitectura no sabe cambiar de pila todavía (P2b.3)");
    abort();
    return NULL;
}

#endif
