/*
 * interp.c — intérprete F1.
 *
 * Subset cubierto: opcodes 0x00..0x6C salvo los que requieren heap
 * (NEWARRAY*, ALOAD*, ASTORE*, CALL_BUILTIN, NEW_OBJECT, GET/SET_FIELD,
 * INVOKE_VIRTUAL, TRY/THROW, INSTANCEOF, FREE_REF, PRINT_STRING que
 * lee desde array). Eso queda para F2 (heap) y F3 (clases).
 *
 * Single-thread: usamos siempre vm->threads[0] (= main). El scheduler
 * propio de la VM viene en F4 con FreeRTOS.
 *
 * El intérprete trabaja sobre locales `pc`, `sp`, `bp`, `cs` que se
 * cargan/guardan al ThreadContext del thread main. Mismo patrón que
 * la VM Java.
 */

#include "bpvm_internal.h"
#include "bpvm_opcodes.h"

#include <stdio.h>
#include <string.h>

/* Helper: emite un texto al output sink (o stdout por defecto). */
static void emit_text(bpvm_t* vm, const char* s, size_t len) {
    if (vm->output_cb) {
        vm->output_cb(s, len, vm->output_user);
    } else {
        fwrite(s, 1, len, stdout);
    }
}

static void emit_int(bpvm_t* vm, int32_t v, int newline) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), newline ? "%d\n" : "%d", v);
    if (n > 0) emit_text(vm, buf, (size_t) n);
}

static void emit_newline(bpvm_t* vm) {
    emit_text(vm, "\n", 1);
}

/* Ejecuta el thread `tc` durante hasta `max_ops` opcodes o hasta que
 * el thread ceda (yield, sleep, mutex_lock contended, join, HALT,
 * THREAD_EXIT). Setea *yielded a 1 si cedió, 0 si quedó RUNNABLE
 * (consumió el quantum y volverá a correr). El scheduler le da
 * el siguiente quantum. */
bpvm_status_t bpvm_interp_run_quantum(bpvm_t* vm, bpvm_thread_t* tc,
                                       int max_ops, int* yielded) {
    if (yielded) *yielded = 0;
    uint8_t* mem = vm->memory;

    /* Registros locales del intérprete — más rápidos que tocar tc.* */
    uint32_t pc = tc->pc;
    uint32_t sp = tc->sp;
    uint32_t bp = tc->bp;
    uint32_t cs = tc->cs;

    tc->status = BPVM_THREAD_RUNNING;
    bpvm_status_t exit_status = BPVM_OK;
    int ops = 0;

    for (;;) {
        if (ops >= max_ops) {
            /* Quantum agotado — el scheduler decide. Quedamos RUNNABLE. */
            tc->status = BPVM_THREAD_RUNNABLE;
            break;
        }
        ops++;
        if (pc >= vm->memory_size) { exit_status = BPVM_ERR_BAD_PC; break; }

        if (vm->tracing) {
            fprintf(stderr, "[trace] pc=%u sp=%u bp=%u cs=%u op=0x%02X\n",
                    pc, sp, bp, cs, mem[pc]);
        }

        uint8_t op = mem[pc++];
        switch (op) {

        case OP_HALT:
            /* HALT: termina el thread y la VM si es el main. */
            if (tc->id != 0) {
                fprintf(stderr, "[bpvm-c] HALT en thread no-main (tid=%d)\n", tc->id);
                exit_status = BPVM_ERR_RUNTIME;
            } else {
                exit_status = BPVM_OK;
            }
            tc->status = BPVM_THREAD_TERMINATED;
            if (yielded) *yielded = 1;
            goto done;

        case OP_THREAD_EXIT:
            /* THREAD_EXIT: termina sólo este thread. */
            tc->status = BPVM_THREAD_TERMINATED;
            if (yielded) *yielded = 1;
            goto done;

        /* ---- Push de inmediatos ---- */
        case OP_PUSH: {
            int32_t v = bpvm_read_i32_be(mem + pc); pc += 4;
            bpvm_write_i32_be(mem + sp, v); sp += 4;
            break;
        }
        case OP_PUSH_0:    bpvm_write_i32_be(mem + sp, 0);   sp += 4; break;
        case OP_PUSH_1:    bpvm_write_i32_be(mem + sp, 1);   sp += 4; break;
        case OP_PUSH_2:    bpvm_write_i32_be(mem + sp, 2);   sp += 4; break;
        case OP_PUSH_3:    bpvm_write_i32_be(mem + sp, 3);   sp += 4; break;
        case OP_PUSH_4:    bpvm_write_i32_be(mem + sp, 4);   sp += 4; break;
        case OP_PUSH_NEG1: bpvm_write_i32_be(mem + sp, -1);  sp += 4; break;

        /* ---- Aritmética entera ---- */
        case OP_ADD: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a + b); sp += 4; break; }
        case OP_SUB: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a - b); sp += 4; break; }
        case OP_MUL: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a * b); sp += 4; break; }
        case OP_DIV: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       if (b == 0) {
                           tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
                           uint32_t ref = bpvm_throw_runtime_error(vm, tc, "División por cero");
                           if (ref && bpvm_eh_unwind(vm, tc, ref)) {
                               pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
                               mem = vm->memory;
                               break;
                           }
                           exit_status = BPVM_ERR_RUNTIME;
                           if (yielded) *yielded = 1;
                           goto done;
                       }
                       bpvm_write_i32_be(mem+sp, a / b); sp += 4; break; }
        case OP_MOD: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       if (b == 0) {
                           tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
                           uint32_t ref = bpvm_throw_runtime_error(vm, tc, "Módulo por cero");
                           if (ref && bpvm_eh_unwind(vm, tc, ref)) {
                               pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
                               mem = vm->memory;
                               break;
                           }
                           exit_status = BPVM_ERR_RUNTIME;
                           if (yielded) *yielded = 1;
                           goto done;
                       }
                       bpvm_write_i32_be(mem+sp, a % b); sp += 4; break; }
        case OP_NEG: { sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, -a); sp += 4; break; }

        /* ---- Comparaciones ---- */
        case OP_EQ:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a == b ? 1 : 0); sp += 4; break; }
        case OP_NEQ: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a != b ? 1 : 0); sp += 4; break; }
        case OP_LT:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a <  b ? 1 : 0); sp += 4; break; }
        case OP_LE:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a <= b ? 1 : 0); sp += 4; break; }
        case OP_GT:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a >  b ? 1 : 0); sp += 4; break; }
        case OP_GE:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a >= b ? 1 : 0); sp += 4; break; }

        /* ---- Lógico booleano (no bitwise) ---- */
        case OP_AND: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, (a && b) ? 1 : 0); sp += 4; break; }
        case OP_OR:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, (a || b) ? 1 : 0); sp += 4; break; }
        case OP_NOT: { sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, !a ? 1 : 0); sp += 4; break; }

        /* ---- Bitwise ---- */
        case OP_BAND: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                        sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                        bpvm_write_i32_be(mem+sp, a & b); sp += 4; break; }
        case OP_BOR:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                        sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                        bpvm_write_i32_be(mem+sp, a | b); sp += 4; break; }
        case OP_BXOR: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                        sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                        bpvm_write_i32_be(mem+sp, a ^ b); sp += 4; break; }
        case OP_BNOT: { sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                        bpvm_write_i32_be(mem+sp, ~a); sp += 4; break; }
        case OP_SHL:   { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                         sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                         bpvm_write_i32_be(mem+sp, a << (b & 31)); sp += 4; break; }
        case OP_SHR_S: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                         sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                         bpvm_write_i32_be(mem+sp, a >> (b & 31)); sp += 4; break; }
        case OP_SHR_U: { sp -= 4; uint32_t b = bpvm_read_u32_be(mem+sp);
                         sp -= 4; uint32_t a = bpvm_read_u32_be(mem+sp);
                         bpvm_write_u32_be(mem+sp, a >> (b & 31)); sp += 4; break; }

        /* ---- Locales y globales ---- */
        case OP_GET_LOCAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            int32_t v = bpvm_read_i32_be(mem + (uint32_t)((int32_t)bp + soff));
            bpvm_write_i32_be(mem + sp, v); sp += 4; break;
        }
        case OP_SET_LOCAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + (uint32_t)((int32_t)bp + soff), v);
            break;
        }
        case OP_GET_LOCAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            int32_t v = bpvm_read_i32_be(mem + (uint32_t)((int32_t)bp + soff));
            bpvm_write_i32_be(mem + sp, v); sp += 4; break;
        }
        case OP_SET_LOCAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + (uint32_t)((int32_t)bp + soff), v);
            break;
        }
        case OP_LEA_LOCAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            bpvm_write_i32_be(mem + sp, (int32_t)((int32_t)bp + soff));
            sp += 4; break;
        }
        case OP_LEA_LOCAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            bpvm_write_i32_be(mem + sp, (int32_t)((int32_t)bp + soff));
            sp += 4; break;
        }
        case OP_GET_GLOBAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            int32_t v = bpvm_read_i32_be(mem + (uint32_t)((int32_t)cs + soff));
            bpvm_write_i32_be(mem + sp, v); sp += 4; break;
        }
        case OP_SET_GLOBAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + (uint32_t)((int32_t)cs + soff), v);
            break;
        }
        case OP_GET_GLOBAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            int32_t v = bpvm_read_i32_be(mem + (uint32_t)((int32_t)cs + soff));
            bpvm_write_i32_be(mem + sp, v); sp += 4; break;
        }
        case OP_SET_GLOBAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + (uint32_t)((int32_t)cs + soff), v);
            break;
        }
        case OP_LEA_GLOBAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            bpvm_write_i32_be(mem + sp, (int32_t)((int32_t)cs + soff));
            sp += 4; break;
        }
        case OP_LEA_GLOBAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            bpvm_write_i32_be(mem + sp, (int32_t)((int32_t)cs + soff));
            sp += 4; break;
        }

        /* ---- Control flow ----
         *
         * Convención CRÍTICA (que difiere del estilo JVM): los offsets de
         * los jumps son RELATIVOS AL INSTRUCTION ADDRESS (= pc del byte
         * opcode), no al pc post-operando. La VM Java los implementa así
         * desde el principio y los .mod ya emitidos asumen esta
         * convención. Ver VirtualMachine.java case 0x0D para referencia.
         */
        case OP_JUMP: {
            uint32_t instr_addr = pc - 1;
            int32_t rel = bpvm_read_i32_be(mem + pc);  /* pc NO avanza */
            pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint32_t instr_addr = pc - 1;
            int32_t rel = bpvm_read_i32_be(mem + pc); pc += 4;
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            if (v == 0) pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP8: {
            uint32_t instr_addr = pc - 1;
            int8_t rel = (int8_t) mem[pc];   /* pc NO avanza */
            pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP_IF_FALSE8: {
            uint32_t instr_addr = pc - 1;
            int8_t rel = (int8_t) mem[pc++];
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            if (v == 0) pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP16: {
            uint32_t instr_addr = pc - 1;
            int16_t rel = bpvm_read_i16_be(mem + pc);  /* pc NO avanza */
            pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP_IF_FALSE16: {
            uint32_t instr_addr = pc - 1;
            int16_t rel = bpvm_read_i16_be(mem + pc); pc += 2;
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            if (v == 0) pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }

        /* ---- Calls + frame setup ---- */
        case OP_CALL: {
            int32_t target_rel = bpvm_read_i32_be(mem + pc); pc += 4;
            /* push saved pc/bp/cs, fija bp = sp tras los saves, salta */
            bpvm_write_i32_be(mem + sp, (int32_t) pc); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) bp); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) cs); sp += 4;
            bp = sp;
            pc = cs + (uint32_t) target_rel;
            break;
        }
        case OP_RET: {
            uint8_t params_count = mem[pc++];
            /* Convención: el return value lo dejó el callee POPEABLE en el
             * top del stack del callee (no en [bp + 0] como sugería un
             * comentario obsoleto). Mi RET hace POP, restaura cs/bp/pc,
             * ajusta sp al pre-args del caller y push del ret_val. Mismo
             * comportamiento que VirtualMachine.java case 0x08. */
            sp -= 4; int32_t ret_val = bpvm_read_i32_be(mem + sp);
            int32_t saved_pc = bpvm_read_i32_be(mem + bp - 12);
            int32_t saved_bp = bpvm_read_i32_be(mem + bp - 8);
            int32_t saved_cs = bpvm_read_i32_be(mem + bp - 4);
            uint32_t target_caller_sp = bp - 12 - (uint32_t) params_count * 4;
            pc = (uint32_t) saved_pc;
            bp = (uint32_t) saved_bp;
            cs = (uint32_t) saved_cs;
            sp = target_caller_sp;
            bpvm_write_i32_be(mem + sp, ret_val);
            sp += 4;
            break;
        }
        case OP_ENTER: {
            uint16_t bytes = bpvm_read_u16_be(mem + pc); pc += 2;
            sp += bytes;
            break;
        }

        /* ---- Output ---- */
        case OP_PRINT: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            emit_int(vm, v, /*newline=*/1);
            break;
        }
        case OP_PRINT_NONL: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            emit_int(vm, v, /*newline=*/0);
            break;
        }
        case OP_PRINT_NL:
            emit_newline(vm);
            break;
        case OP_PRINT_CHAR: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            char c = (char)(v & 0xFF);
            emit_text(vm, &c, 1);
            break;
        }

        /* ---- Casts narrow (sin runtime check para no RuntimeError-ear
                en F1; F5 lo añade) ---- */
        case OP_I32_TO_I8: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            int32_t r = (int32_t)(int8_t)(v & 0xFF);
            bpvm_write_i32_be(mem + sp, r); sp += 4; break;
        }
        case OP_I32_TO_U8: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + sp, v & 0xFF); sp += 4; break;
        }
        case OP_I32_TO_I16: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            int32_t r = (int32_t)(int16_t)(v & 0xFFFF);
            bpvm_write_i32_be(mem + sp, r); sp += 4; break;
        }
        case OP_I32_TO_U16: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + sp, v & 0xFFFF); sp += 4; break;
        }

        /* ---- F2: arrays ---- */
        case OP_NEWARRAY: {
            sp -= 4; int32_t size = bpvm_read_i32_be(mem + sp);
            if (size < 0) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) size * 4, BPVM_TYPE_ARRAY_I32);
            if (ref == 0) { exit_status = BPVM_ERR_OOM; goto done; }
            bpvm_write_u32_be(mem + ref, (uint32_t) size);
            bpvm_write_i32_be(mem + sp, (int32_t) ref); sp += 4;
            mem = vm->memory;  /* heap_alloc no realoca pero defensivo */
            break;
        }
        case OP_NEWARRAY_I8: {
            sp -= 4; int32_t size = bpvm_read_i32_be(mem + sp);
            if (size < 0) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) size, BPVM_TYPE_ARRAY_I8);
            if (ref == 0) { exit_status = BPVM_ERR_OOM; goto done; }
            bpvm_write_u32_be(mem + ref, (uint32_t) size);
            bpvm_write_i32_be(mem + sp, (int32_t) ref); sp += 4;
            break;
        }
        case OP_NEWARRAY_I16: {
            sp -= 4; int32_t size = bpvm_read_i32_be(mem + sp);
            if (size < 0) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) size * 2, BPVM_TYPE_ARRAY_I16);
            if (ref == 0) { exit_status = BPVM_ERR_OOM; goto done; }
            bpvm_write_u32_be(mem + ref, (uint32_t) size);
            bpvm_write_i32_be(mem + sp, (int32_t) ref); sp += 4;
            break;
        }
        case OP_ALOAD: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) {
                exit_status = BPVM_ERR_RUNTIME; goto done;
            }
            int32_t v = bpvm_read_i32_be(mem + ref + 4 + (uint32_t) idx * 4);
            bpvm_write_i32_be(mem + sp, v); sp += 4;
            break;
        }
        case OP_ASTORE: {
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) {
                exit_status = BPVM_ERR_RUNTIME; goto done;
            }
            bpvm_write_i32_be(mem + ref + 4 + (uint32_t) idx * 4, v);
            break;
        }
        case OP_ALEN: {
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            bpvm_write_i32_be(mem + sp, (int32_t) length); sp += 4;
            break;
        }
        case OP_ALOAD_I8: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            int8_t v = (int8_t) mem[ref + 4 + (uint32_t) idx];
            bpvm_write_i32_be(mem + sp, (int32_t) v); sp += 4;
            break;
        }
        case OP_ALOAD_U8: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint8_t v = mem[ref + 4 + (uint32_t) idx];
            bpvm_write_i32_be(mem + sp, (int32_t) v); sp += 4;
            break;
        }
        case OP_ALOAD_I16: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            int16_t v = bpvm_read_i16_be(mem + ref + 4 + (uint32_t) idx * 2);
            bpvm_write_i32_be(mem + sp, (int32_t) v); sp += 4;
            break;
        }
        case OP_ALOAD_U16: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint16_t v = bpvm_read_u16_be(mem + ref + 4 + (uint32_t) idx * 2);
            bpvm_write_i32_be(mem + sp, (int32_t) v); sp += 4;
            break;
        }
        case OP_ASTORE_I8: {
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            mem[ref + 4 + (uint32_t) idx] = (uint8_t)(v & 0xFF);
            break;
        }
        case OP_ASTORE_I16: {
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            int16_t v16 = (int16_t)(v & 0xFFFF);
            bpvm_write_u32_be(mem + ref + 4 + (uint32_t) idx * 2, 0); /* unused */
            mem[ref + 4 + (uint32_t) idx * 2]     = (uint8_t)((v16 >> 8) & 0xFF);
            mem[ref + 4 + (uint32_t) idx * 2 + 1] = (uint8_t)(v16 & 0xFF);
            break;
        }

        /* ---- F2: print strings ---- */
        case OP_PRINT_STRING:
        case OP_PRINT_STR_NONL: {
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref != 0) {
                uint32_t length = bpvm_read_u32_be(mem + ref);
                for (uint32_t i = 0; i < length; i++) {
                    uint32_t cp = bpvm_read_u32_be(mem + ref + 4 + i * 4);
                    char c = (cp < 128) ? (char) cp : '?';
                    emit_text(vm, &c, 1);
                }
            }
            if (op == OP_PRINT_STRING) emit_newline(vm);
            break;
        }

        /* ---- F2: GC trigger manual ---- */
        case OP_GC_COLLECT: {
            tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
            bpvm_heap_gc(vm);
            mem = vm->memory;
            break;
        }

        /* ---- F2: builtins ---- */
        case OP_CALL_BUILTIN: {
            uint16_t id = bpvm_read_u16_be(mem + pc); pc += 2;
            tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
            bpvm_status_t bs = bpvm_call_builtin(vm, tc, id);
            sp = tc->sp; bp = tc->bp; pc = tc->pc; cs = tc->cs;
            mem = vm->memory;
            if (bs != BPVM_OK) { exit_status = bs; goto done; }
            /* F4: si el builtin cambió status a BLOCKED_* o RUNNABLE
             * (yield), salimos del quantum para que el scheduler
             * decida. */
            if (tc->status != BPVM_THREAD_RUNNING) {
                if (yielded) *yielded = 1;
                goto done;
            }
            break;
        }

        /* ---- F3: clases ---- */
        case OP_NEW_OBJECT: {
            int16_t cs_off = bpvm_read_i16_be(mem + pc); pc += 2;
            uint32_t class_ptr = (uint32_t)((int32_t) cs + cs_off);
            uint16_t num_fields = bpvm_read_u16_be(mem + class_ptr
                                                    + BPVM_CLS_OFF_NUM_FIELDS);
            uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) num_fields * 4,
                                            BPVM_TYPE_OBJECT);
            if (ref == 0) { exit_status = BPVM_ERR_OOM; goto done; }
            bpvm_write_u32_be(mem + ref, class_ptr);   /* slot[0] = class_ptr */
            /* fields ya zeroed por heap_alloc. */
            bpvm_write_i32_be(mem + sp, (int32_t) ref); sp += 4;
            mem = vm->memory;
            break;
        }
        case OP_GET_FIELD: {
            uint8_t slot = mem[pc++];
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            int32_t v = bpvm_read_i32_be(mem + ref + 4 + (uint32_t) slot * 4);
            bpvm_write_i32_be(mem + sp, v); sp += 4;
            break;
        }
        case OP_SET_FIELD: {
            uint8_t slot = mem[pc++];
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            bpvm_write_i32_be(mem + ref + 4 + (uint32_t) slot * 4, v);
            break;
        }

        case OP_INVOKE_VIRTUAL: {
            uint8_t vt_slot = mem[pc++];
            uint8_t num_args = mem[pc++];
            uint32_t this_addr = sp - 4 - (uint32_t) num_args * 4;
            uint32_t this_ref = (uint32_t) bpvm_read_i32_be(mem + this_addr);
            if (this_ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            uint32_t class_ptr = (uint32_t) bpvm_read_i32_be(mem + this_ref);

            /* L2 v3: fall-back al parent si vt[slot] == -1 o slot >= num_methods.
             * El bucle termina al encontrar un methodOff válido o llegar
             * a la raíz (parent_offset == 0). */
            uint32_t desc = class_ptr;
            int32_t method_off = -1;
            uint32_t target_cs = 0;
            for (;;) {
                uint16_t bw       = bpvm_read_u16_be(mem + desc
                                                      + BPVM_CLS_OFF_BITMAP_WORDS);
                uint16_t nmeth    = bpvm_read_u16_be(mem + desc
                                                      + BPVM_CLS_OFF_NUM_METHODS);
                uint32_t vt_base  = desc + BPVM_CLS_OFF_FIELD_BITMAP
                                         + 2u * (uint32_t) bw * 4u;
                if (vt_slot < nmeth) {
                    int32_t off = bpvm_read_i32_be(mem + vt_base + (uint32_t) vt_slot * 4);
                    if (off != -1) {
                        method_off = off;
                        target_cs  = bpvm_get_cs_for_data_addr(vm, desc);
                        break;
                    }
                }
                int32_t parent_off = bpvm_read_i32_be(mem + desc
                                                       + BPVM_CLS_OFF_PARENT_OFF);
                if (parent_off == 0) {
                    fprintf(stderr,
                            "[bpvm-c] INVOKE_VIRTUAL slot %u no resoluble en cadena de herencia\n",
                            vt_slot);
                    exit_status = BPVM_ERR_RUNTIME;
                    goto done;
                }
                uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, desc);
                desc = (uint32_t)((int32_t) cur_cs + parent_off);
            }

            uint32_t target_pc = target_cs + (uint32_t) method_off;
            /* push saved pc/bp/cs, bp = sp, salta */
            bpvm_write_i32_be(mem + sp, (int32_t) pc); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) bp); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) cs); sp += 4;
            bp = sp;
            pc = target_pc;
            cs = target_cs;
            break;
        }

        case OP_INSTANCEOF: {
            int16_t cs_off = bpvm_read_i16_be(mem + pc); pc += 2;
            uint32_t expected = (uint32_t)((int32_t) cs + cs_off);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            int32_t result = 0;
            if (ref != 0) {
                uint32_t cur = (uint32_t) bpvm_read_i32_be(mem + ref);
                while (cur != 0) {
                    if (cur == expected) { result = 1; break; }
                    int32_t parent_off = bpvm_read_i32_be(mem + cur
                                                            + BPVM_CLS_OFF_PARENT_OFF);
                    if (parent_off == 0) break;
                    uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, cur);
                    cur = (uint32_t)((int32_t) cur_cs + parent_off);
                }
            }
            bpvm_write_i32_be(mem + sp, result); sp += 4;
            break;
        }

        case OP_FREE_REF: {
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref != 0) {
                /* Sólo objetos. Para arrays / strings, NOP. */
                uint32_t header = ref - 4;
                uint32_t tag = bpvm_read_u32_be(mem + header);
                int type = (int)((tag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
                if (type == BPVM_TYPE_OBJECT) {
                    /* TODO: recorrer owner_bitmap y FREE_REF recursivo de
                     * fields owners. F3 v1 sólo libera el objeto raíz. */
                    bpvm_write_u32_be(mem + header, tag | BPVM_TAG_FREE_BIT);
                }
            }
            break;
        }

        case OP_SET_FIELD_OWNER: {
            uint8_t slot = mem[pc++];
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            uint32_t field_addr = ref + 4 + (uint32_t) slot * 4;
            uint32_t old_val = (uint32_t) bpvm_read_i32_be(mem + field_addr);
            if (old_val != 0) {
                uint32_t old_header = old_val - 4;
                if (old_val >= vm->heap_start && old_val < vm->heap_next) {
                    uint32_t otag = bpvm_read_u32_be(mem + old_header);
                    int otype = (int)((otag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
                    if (otype == BPVM_TYPE_OBJECT) {
                        bpvm_write_u32_be(mem + old_header, otag | BPVM_TAG_FREE_BIT);
                    }
                }
            }
            bpvm_write_i32_be(mem + field_addr, v);
            break;
        }

        /* ---- F3: CALL_EXT ---- */
        case OP_CALL_EXT: {
            uint16_t idx = bpvm_read_u16_be(mem + pc); pc += 2;
            /* La ext-table del módulo actual vive al inicio del módulo.
             * Lo encontramos buscando el módulo cuyo code_start == cs. */
            uint32_t ext_table_addr = bpvm_get_ext_table_addr(vm, cs);
            if (ext_table_addr == 0) {
                fprintf(stderr, "[bpvm-c] CALL_EXT sin ext-table en cs=%u\n", cs);
                exit_status = BPVM_ERR_RUNTIME; goto done;
            }
            uint32_t target = (uint32_t) bpvm_read_i32_be(mem
                                + ext_table_addr + (uint32_t) idx * 4);
            if (target == 0) {
                fprintf(stderr, "[bpvm-c] CALL_EXT idx=%u no resuelto\n", idx);
                exit_status = BPVM_ERR_RUNTIME; goto done;
            }
            /* Determinar el CS del módulo destino para que el frame
             * tenga el cs correcto al ejecutar el target. */
            uint32_t target_cs = bpvm_get_cs_for_code_addr(vm, target);
            bpvm_write_i32_be(mem + sp, (int32_t) pc); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) bp); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) cs); sp += 4;
            bp = sp;
            pc = target;
            cs = target_cs;
            break;
        }

        /* ---- F5: exception handling ---- */
        case OP_TRY_BEGIN: {
            /* Operando: handler_rel:i32 + cls_off:i16. handler_pc es
             * RELATIVO al instruction address del TRY_BEGIN (= pc-1).
             * Misma convención que JUMP. */
            uint32_t instr_addr = pc - 1;
            int32_t handler_rel = bpvm_read_i32_be(mem + pc); pc += 4;
            int16_t cls_off     = bpvm_read_i16_be(mem + pc); pc += 2;
            int32_t handler_pc  = (int32_t) instr_addr + handler_rel;
            int32_t expected_cls = (cls_off == 0) ? 0
                                   : (int32_t)((int32_t) cs + cls_off);
            bpvm_eh_push(tc, handler_pc, (int32_t) sp, (int32_t) bp,
                         (int32_t) cs, expected_cls);
            break;
        }
        case OP_TRY_END: {
            bpvm_eh_pop(tc);
            break;
        }
        case OP_THROW: {
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            tc->pc = pc; tc->sp = sp; tc->bp = bp; tc->cs = cs;
            int caught = bpvm_eh_unwind(vm, tc, ref);
            if (!caught) {
                exit_status = BPVM_ERR_RUNTIME;
                if (yielded) *yielded = 1;
                /* tc ya marcado TERMINATED por eh_unwind. */
                goto done;
            }
            /* Recargar registros tras unwind. */
            pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
            break;
        }

        default:
            fprintf(stderr, "[bpvm-c] opcode 0x%02X desconocido en PC %u\n",
                    op, pc - 1);
            exit_status = BPVM_ERR_BAD_OPCODE;
            goto done;
        }
    }

done:
    /* Persistir registros al ThreadContext. El status ya lo seteó cada
     * case (RUNNABLE si quantum, TERMINATED si HALT/THREAD_EXIT,
     * BLOCKED_* si yield builtin, RUNNING si nada — defensivo). */
    tc->pc = pc; tc->sp = sp; tc->bp = bp; tc->cs = cs;
    if (exit_status != BPVM_OK) {
        tc->status = BPVM_THREAD_TERMINATED;
        if (yielded) *yielded = 1;
    }
    return exit_status;
}
