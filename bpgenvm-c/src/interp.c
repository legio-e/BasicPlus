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

bpvm_status_t bpvm_interp_run(bpvm_t* vm) {
    if (vm->main_absolute_address == 0) return BPVM_ERR_BAD_PC;
    if (vm->thread_count == 0)          return BPVM_ERR_BAD_PC;

    bpvm_thread_t* tc = &vm->threads[0];
    uint8_t* mem = vm->memory;

    /* Inicializar el thread main desde la entry-point.
     * El frontend emite un wrapper __startup en mainOffset que llama
     * a __init + main(arg=""). El primer opcode que ejecutamos es ese
     * __startup. La pila empieza limpia: sp = bp = stack_base. */
    tc->pc = vm->main_absolute_address;
    /* cs se determina por el módulo del entry-point. */
    bpvm_module_t* main_mod = NULL;
    for (int i = 0; i < vm->module_count; i++) {
        bpvm_module_t* m = &vm->modules[i];
        if (m->code_start <= vm->main_absolute_address
                && vm->main_absolute_address < m->end_addr) {
            main_mod = m;
            break;
        }
    }
    if (!main_mod) return BPVM_ERR_BAD_PC;
    tc->cs = main_mod->code_start;
    tc->sp = tc->stack_base;
    tc->bp = tc->stack_base;
    tc->status = BPVM_THREAD_RUNNING;

    /* Registros locales del intérprete — más rápidos que tocar tc.* */
    uint32_t pc = tc->pc;
    uint32_t sp = tc->sp;
    uint32_t bp = tc->bp;
    uint32_t cs = tc->cs;

    bpvm_status_t exit_status = BPVM_OK;

    for (;;) {
        if (pc >= vm->memory_size) { exit_status = BPVM_ERR_BAD_PC; break; }

        if (vm->tracing) {
            fprintf(stderr, "[trace] pc=%u sp=%u bp=%u cs=%u op=0x%02X\n",
                    pc, sp, bp, cs, mem[pc]);
        }

        uint8_t op = mem[pc++];
        switch (op) {

        case OP_HALT:
            exit_status = BPVM_OK;
            goto done;

        case OP_THREAD_EXIT:
            /* F1: sólo hay main thread; THREAD_EXIT termina la VM
               igual que HALT. F4 cambia esto. */
            exit_status = BPVM_OK;
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
                       if (b == 0) { exit_status = BPVM_ERR_DIV_BY_ZERO; goto done; }
                       bpvm_write_i32_be(mem+sp, a / b); sp += 4; break; }
        case OP_MOD: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       if (b == 0) { exit_status = BPVM_ERR_DIV_BY_ZERO; goto done; }
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
            /* Convención: el return value lo dejó el callee en [bp + 0]
             * antes del RET. Lo leemos, restauramos cs/bp/pc, ajustamos
             * sp y dejamos el return value en la nueva cima. */
            int32_t ret_val  = bpvm_read_i32_be(mem + bp);
            int32_t saved_cs = bpvm_read_i32_be(mem + bp - 4);
            int32_t saved_bp = bpvm_read_i32_be(mem + bp - 8);
            int32_t saved_pc = bpvm_read_i32_be(mem + bp - 12);
            sp = (uint32_t) bp - 12 - (uint32_t) params_count * 4;
            bp = (uint32_t) saved_bp;
            cs = (uint32_t) saved_cs;
            pc = (uint32_t) saved_pc;
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

        /* ---- Aún no implementados en F1 ---- */
        case OP_NEWARRAY: case OP_NEWARRAY_I8: case OP_NEWARRAY_I16:
        case OP_ALOAD: case OP_ASTORE: case OP_ALEN:
        case OP_ALOAD_I8: case OP_ALOAD_U8: case OP_ALOAD_I16: case OP_ALOAD_U16:
        case OP_ASTORE_I8: case OP_ASTORE_I16:
        case OP_PRINT_STRING: case OP_PRINT_STR_NONL:
        case OP_NEW_OBJECT: case OP_GET_FIELD: case OP_SET_FIELD:
        case OP_INVOKE_VIRTUAL: case OP_INSTANCEOF: case OP_FREE_REF:
        case OP_SET_FIELD_OWNER:
        case OP_CALL_EXT: case OP_CALL_BUILTIN:
        case OP_TRY_BEGIN: case OP_TRY_END: case OP_THROW:
        case OP_GC_COLLECT:
            fprintf(stderr, "[bpvm-c F1] opcode 0x%02X aún no soportado en PC %u\n",
                    op, pc - 1);
            exit_status = BPVM_ERR_BAD_OPCODE;
            goto done;

        default:
            fprintf(stderr, "[bpvm-c] opcode 0x%02X desconocido en PC %u\n",
                    op, pc - 1);
            exit_status = BPVM_ERR_BAD_OPCODE;
            goto done;
        }
    }

done:
    /* Persistir registros al ThreadContext (para inspección futura). */
    tc->pc = pc; tc->sp = sp; tc->bp = bp; tc->cs = cs;
    tc->status = BPVM_THREAD_TERMINATED;
    return exit_status;
}
