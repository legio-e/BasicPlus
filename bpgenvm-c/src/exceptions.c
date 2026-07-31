/*
 * exceptions.c — try/catch/throw + RuntimeError BP (F5).
 *
 * Modelo de la VM Java (docs/OPCODES.md §0x5B..0x5D):
 *   TRY_BEGIN <handler_rel:i32, cls_off:i16>:
 *     push entry { handler_pc=pc+handler_rel, saved_sp, saved_bp,
 *                  saved_cs, expected_class=cs+cls_off (0 = catch all) }.
 *   TRY_END:
 *     pop entry top.
 *   THROW:
 *     pop ref; busca entry cuya expected_class matchee (= 0 ó
 *     isDescendantOf(class(ref), expected)). Si lo encuentra:
 *     restaura sp/bp/cs/pc desde la entry, pop hasta esa entry,
 *     push ref. Si no, BpThreadFault con stack trace.
 */

#include "bpvm_internal.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */

void bpvm_eh_push(bpvm_thread_t* tc, int32_t handler_pc, int32_t saved_sp,
                  int32_t saved_bp, int32_t saved_cs, int32_t expected_class) {
    if (tc->eh_stack_size >= tc->eh_stack_capacity) {
        int new_cap = tc->eh_stack_capacity == 0 ? 4 : tc->eh_stack_capacity * 2;
        bpvm_eh_entry_t* arr = (bpvm_eh_entry_t*) bpvm_realloc(tc->eh_stack,
                                  (size_t) new_cap * sizeof(bpvm_eh_entry_t));
        if (!arr) return;
        tc->eh_stack = arr;
        tc->eh_stack_capacity = new_cap;
    }
    bpvm_eh_entry_t* e = &tc->eh_stack[tc->eh_stack_size++];
    e->handler_pc     = handler_pc;
    e->saved_sp       = saved_sp;
    e->saved_bp       = saved_bp;
    e->saved_cs       = saved_cs;
    e->expected_class = expected_class;
}

void bpvm_eh_pop(bpvm_thread_t* tc) {
    if (tc->eh_stack_size > 0) tc->eh_stack_size--;
}

/* Sube por la cadena de herencia para determinar si `obj_class`
 * desciende de `target`. Cross-module via getCSForDataAddr.
 * Equivalente a VirtualMachine.isDescendantOf. */
static int is_descendant_of(const bpvm_t* vm, uint32_t obj_class, uint32_t target) {
    uint32_t cur = obj_class;
    while (cur != 0) {
        if (cur == target) return 1;
        int32_t parent_off = bpvm_read_i32_be(vm->memory + cur + BPVM_CLS_OFF_PARENT_OFF);
        if (parent_off == 0) return 0;
        uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, cur);
        cur = (uint32_t)((int32_t) cur_cs + parent_off);
    }
    return 0;
}

int bpvm_eh_unwind(bpvm_t* vm, bpvm_thread_t* tc, bpref_t ref) {
    uint32_t thrown_class = 0;
    if (!bpref_is_null(ref)) {
        thrown_class = (uint32_t) bpvm_read_i32_be(vm->memory + bpref_deref(vm, ref));
    }
    /* Busca un handler que matchee, desde el top hacia abajo. */
    while (tc->eh_stack_size > 0) {
        bpvm_eh_entry_t e = tc->eh_stack[--tc->eh_stack_size];
        int matches = (e.expected_class == 0)
                   || (thrown_class != 0
                       && is_descendant_of(vm, thrown_class, (uint32_t) e.expected_class));
        if (matches) {
            tc->sp = (uint32_t) e.saved_sp;
            tc->bp = (uint32_t) e.saved_bp;
            tc->cs = (uint32_t) e.saved_cs;
            tc->pc = (uint32_t) e.handler_pc;
            /* Push ref para que el catch lo reciba como local. V4: handle 64b COMPLETO
             * (gen preservada) — si se truncara a 32b, un objeto-excepción en slot
             * reciclado (gen>0) daría gen-mismatch al leer e.msg y re-lanzaría. */
            bpvm_write_i64_be(vm->memory + tc->sp, (int64_t) ref.v);
            tc->sp += 8;
            return 1;
        }
        /* No matchee: seguimos popeando entries (los handlers más
         * exteriores). */
    }
    /* Sin handler: print error + terminar thread. */
    fprintf(stderr, "[bpvm-c] excepción no atrapada en tid=%" PRId32 "\n", tc->id);
    if (!bpref_is_null(ref)) {
        /* Intenta leer field 0 = msg (asumiendo layout RuntimeError). */
        bpref_t msg_r; msg_r.v = (uint64_t) bpvm_read_i64_be(vm->memory + bpref_deref(vm, ref) + 4);   /* msg = handle 64b */
        if (msg_r.v != 0u) {
            uint32_t msg_addr = bpref_deref(vm, msg_r);   /* handle→addr */
            uint32_t mlen = bpvm_read_u32_be(vm->memory + msg_addr);   /* H2: bytes UTF-8 */
            char buf[256]; size_t n = 0;
            for (uint32_t i = 0; i < mlen && n < sizeof(buf) - 1; i++) {
                buf[n++] = (char) vm->memory[msg_addr + 4 + i];
            }
            buf[n] = '\0';
            fprintf(stderr, "  RuntimeError: %s\n", buf);
        }
    }
    tc->status = BPVM_THREAD_TERMINATED;
    return 0;
}

const char* bpvm_runtime_error(const bpvm_t* vm) {
    return (vm && vm->runtime_error[0]) ? vm->runtime_error : "";
}

bpref_t bpvm_throw_runtime_error(bpvm_t* vm, bpvm_thread_t* tc,
                                  const char* msg) {
    /* Guarda el detalle para que el host / wire lo surtan si el error acaba
     * NO atrapado (espejo de link_error). En cada throw; solo se reporta si el
     * run termina en BPVM_ERR_RUNTIME. */
    snprintf(vm->runtime_error, sizeof(vm->runtime_error), "%s", msg ? msg : "");
    /* Buscar el class_ptr de RuntimeError exportado por algún módulo.
     * El frontend Java sintetiza la clase y la exporta como data
     * symbol "<lib>.<mod>.RuntimeError" / "<mod>.RuntimeError" — la
     * global symbol table del linker ya las tiene registradas. */
    uint32_t class_ptr = 0;
    /* #248 — primero la clase ÚNICA de Core (Object -> Exception ->
     * RuntimeError). Los fallbacks de abajo cubren .mods legado con copia
     * per-módulo. */
    class_ptr = bpvm_link_lookup(vm, "Core.RuntimeError");
    if (class_ptr) goto have_class;
    /* Probamos primero el módulo del cs actual. */
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (m->code_start != tc->cs) continue;
        char qual[160];
        if (m->library[0]) snprintf(qual, sizeof(qual), "%s.%s.RuntimeError",
                                     m->library, m->name);
        else               snprintf(qual, sizeof(qual), "%s.RuntimeError",
                                     m->name);
        class_ptr = bpvm_link_lookup(vm, qual);
        if (class_ptr) break;
    }
    /* Fallback: cualquier módulo que la haya exportado. */
    if (!class_ptr) {
        for (int i = 0; i < vm->module_count && !class_ptr; i++) {
            const bpvm_module_t* m = &vm->modules[i];
            char qual[160];
            if (m->library[0]) snprintf(qual, sizeof(qual), "%s.%s.RuntimeError",
                                         m->library, m->name);
            else               snprintf(qual, sizeof(qual), "%s.RuntimeError",
                                         m->name);
            class_ptr = bpvm_link_lookup(vm, qual);
        }
    }
have_class:
    if (!class_ptr) {
        /* Sin RuntimeError disponible — caller debe usar BpThreadFault
         * equivalente (= terminar thread). Aquí imprimimos al menos. */
        fprintf(stderr, "[bpvm-c] RuntimeError sin clase exportada: %s\n",
                msg ? msg : "");
        return bpref_null();
    }

    /* Alocar el string del mensaje. */
    size_t mlen = msg ? strlen(msg) : 0;
    uint32_t msg_ref = bpvm_heap_alloc_string(vm, msg ? msg : "", mlen);
    if (msg_ref == 0) return bpref_null();
    /* GC-safety: el msg AÚN no vive en ninguna raíz y la alocación del objeto de
     * abajo PUEDE disparar el GC (umbral/OOM) → anclarlo para que no lo recicle
     * (su gen queda intacta, clave para el bpref_regen de más abajo). */
    tc->alloc_anchor = (int32_t) msg_ref;

    /* Alocar el objeto RuntimeError. */
    uint16_t num_fields = bpvm_read_u16_be(vm->memory + class_ptr + BPVM_CLS_OFF_NUM_FIELDS);
    uint32_t obj_addr = bpvm_heap_alloc(vm, (uint32_t) num_fields * 4, BPVM_TYPE_OBJECT);
    if (obj_addr == 0) return bpref_null();
    bpvm_write_u32_be(vm->memory + obj_addr, class_ptr);
    /* slot 0 = msg (convención del frontend). Guardamos el HANDLE COMPLETO de 64b:
     * bpref_regen re-adjunta la gen VIVA del slot del msg (heap_alloc_string truncó
     * a uint32 → gen=0). Sin esto, si el slot del msg fue reciclado (gen ≠ 0), leer
     * e.msg daría "referencia a objeto eliminado" sobre un mensaje vivo. */
    if (num_fields > 0) {
        bpref_store(vm, obj_addr + 4 + 0 * 4, bpref_regen(vm, msg_ref));
    }
    bpref_t obj_h = bpvm_handle_register(vm, obj_addr);   /* V4: handle 64b (gen preservada — clave si el slot fue reciclado) */

    /* Anclar para GC; el caller decide si pasarlo a eh_unwind o usarlo
     * de otra forma. No tocamos el stack BP aquí — eso lo hace
     * eh_unwind tras encontrar handler. El ancla es idx|TAG. */
    tc->alloc_anchor = (int32_t) (uint32_t) obj_h.v;
    return obj_h;
}
