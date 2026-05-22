/*
 * bpvm.c — implementación de la API pública.
 *
 * En F1 mantenemos malloc/free para la estructura de control bpvm_t.
 * El memory[] sigue siendo del caller. F2+ podría ofrecer una variante
 * "todo-en-buffer-del-caller" para targets sin libc.
 */

#include "bpvm_internal.h"
#include <stdlib.h>
#include <string.h>

bpvm_t* bpvm_init(uint8_t* memory, size_t memory_size, size_t stack_base) {
    if (memory == NULL || memory_size < 4096) return NULL;
    if (stack_base == 0) stack_base = memory_size / 2;
    if (stack_base >= memory_size) return NULL;
    if (stack_base + BPVM_MAIN_STACK_BYTES > memory_size) return NULL;

    bpvm_t* vm = (bpvm_t*) calloc(1, sizeof(bpvm_t));
    if (!vm) return NULL;

    vm->memory = memory;
    vm->memory_size = memory_size;
    vm->stack_base = (uint32_t) stack_base;
    vm->next_free_address = BPVM_INITIAL_FREE_ADDR;
    vm->heap_start = (uint32_t) stack_base;   /* sin módulos cargados aún */
    vm->heap_next  = vm->heap_start;
    vm->main_absolute_address = 0;

    /* Pone el byte sentinela en memory[0]. */
    memory[0] = BPVM_SENTINEL_THREAD_EXIT;

    /* Thread main: región fija MAIN_STACK_BYTES a partir de stack_base. */
    bpvm_thread_t* main_tc = &vm->threads[0];
    main_tc->id = 0;
    main_tc->stack_base = (uint32_t) stack_base;
    main_tc->stack_top  = (uint32_t) stack_base + BPVM_MAIN_STACK_BYTES;
    main_tc->sp = main_tc->stack_base;
    main_tc->bp = main_tc->stack_base;
    main_tc->status = BPVM_THREAD_RUNNABLE;
    vm->thread_count = 1;
    vm->current_thread_idx = 0;

    return vm;
}

bpvm_status_t bpvm_load_mod(bpvm_t* vm, const char* path) {
    if (!vm || !path) return BPVM_ERR_IO;
    return bpvm_loader_load(vm, path);
}

bpvm_status_t bpvm_run(bpvm_t* vm) {
    if (!vm) return BPVM_ERR_BAD_PC;
    return bpvm_interp_run(vm);
}

void bpvm_set_output(bpvm_t* vm, bpvm_output_cb cb, void* user) {
    if (!vm) return;
    vm->output_cb = cb;
    vm->output_user = user;
}

void bpvm_set_tracing(bpvm_t* vm, int enabled) {
    if (!vm) return;
    vm->tracing = enabled ? true : false;
}

void bpvm_destroy(bpvm_t* vm) {
    if (!vm) return;
    free(vm->scratch);
    free(vm);
}

const char* bpvm_status_str(bpvm_status_t s) {
    switch (s) {
    case BPVM_OK:                 return "OK";
    case BPVM_ERR_IO:             return "IO error";
    case BPVM_ERR_BAD_MAGIC:      return "MAGIC inválido (no es un .mod v5)";
    case BPVM_ERR_BAD_HEADER:     return "header inconsistente";
    case BPVM_ERR_OOM:            return "memoria del buffer insuficiente";
    case BPVM_ERR_BAD_OPCODE:     return "opcode desconocido o no soportado";
    case BPVM_ERR_BAD_PC:         return "PC fuera de rango";
    case BPVM_ERR_STACK_OVERFLOW: return "stack overflow";
    case BPVM_ERR_DIV_BY_ZERO:    return "división por cero";
    case BPVM_ERR_NULL_RECEIVER:  return "INVOKE_VIRTUAL sobre null";
    case BPVM_ERR_RUNTIME:        return "RuntimeError BP no atrapado";
    default:                       return "?";
    }
}
