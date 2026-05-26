/*
 * bpvm.c — implementación de la API pública.
 *
 * En F1 mantenemos malloc/free para la estructura de control bpvm_t.
 * El memory[] sigue siendo del caller. F2+ podría ofrecer una variante
 * "todo-en-buffer-del-caller" para targets sin libc.
 */

#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"   /* H3 #158: tabla helpers para AOT */
#include <stdio.h>
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
    main_tc->blocked_on_mutex = -1;
    main_tc->blocked_on_join = -1;
    vm->thread_count = 1;
    vm->current_thread_idx = 0;

    /* F4: el alocador de stacks de threads BP empieza tras la región
     * del main. Cada Thread.start() reserva BPVM_THREAD_STACK_BYTES. */
    vm->next_thread_stack = main_tc->stack_top;

    /* H3 #158 — apuntar a la tabla global de helpers para AOT. El
     * código AOT C-emitido la usa vía vm->aot_helpers->func(...). */
    vm->aot_helpers = &bpvm_aot_helpers_v1;

    return vm;
}

/* Extrae el dirname de un path. dst debe tener al menos 256 bytes. */
static void path_dirname(const char* path, char* dst, size_t dst_size) {
    const char* last_sep = NULL;
    for (const char* p = path; *p; p++) {
        if (*p == '/' || *p == '\\') last_sep = p;
    }
    if (!last_sep) { dst[0] = '\0'; return; }
    size_t n = (size_t)(last_sep - path);
    if (n >= dst_size) n = dst_size - 1;
    memcpy(dst, path, n);
    dst[n] = '\0';
}

/* Comprueba si un módulo con nombre dado ya está cargado. */
static int module_loaded(const bpvm_t* vm, const char* library, const char* name) {
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (strcmp(m->name, name) == 0
                && strcmp(m->library, library ? library : "") == 0) {
            return 1;
        }
    }
    return 0;
}

/* Para un import qualified (e.g. "L2Lib.Counter.__init"), deriva el
 * (library, module) que apunta. Convención del frontend:
 *   - "Mod.sym"             → library="",  module="Mod"
 *   - "Lib.Mod.sym"         → library="Lib", module="Mod"
 *   - "Lib.Mod.Cls.sym"     → library="Lib", module="Mod"  (frontend pone
 *                              el nombre del módulo en parts[-2] cuando
 *                              hay 3+ componentes; F3 simple asume eso) */
static void derive_owner(const char* qualified, char* lib, size_t lib_size,
                         char* mod, size_t mod_size) {
    /* Split por '.'. parts[length-1] = símbolo, parts[length-2] = módulo,
     * el resto = library (juntado con '.'). */
    int dot_positions[16];
    int ndots = 0;
    for (const char* p = qualified; *p && ndots < 16; p++) {
        if (*p == '.') dot_positions[ndots++] = (int)(p - qualified);
    }
    lib[0] = '\0'; mod[0] = '\0';
    if (ndots < 1) return;   /* sin separador: no hay módulo. */
    int last = dot_positions[ndots - 1];
    int second_last = ndots >= 2 ? dot_positions[ndots - 2] : -1;
    /* Modulo = parts[ndots-2..ndots-1) = subcadena (second_last+1, last). */
    int mod_start = second_last + 1;
    int mod_len = last - mod_start;
    size_t mn = (size_t) mod_len;
    if (mn >= mod_size) mn = mod_size - 1;
    memcpy(mod, qualified + mod_start, mn);
    mod[mn] = '\0';
    if (ndots >= 2) {
        /* Library = subcadena [0, second_last). */
        int ll = second_last;
        size_t llz = (size_t) ll;
        if (llz >= lib_size) llz = lib_size - 1;
        memcpy(lib, qualified, llz);
        lib[llz] = '\0';
    }
}

/* Resuelve recursivamente las dependencias del módulo en `mod_idx`,
 * cargándolas desde `search_dir` por convención de naming
 * (<lib>.<mod>.mod o <mod>.mod). */
static bpvm_status_t discover_deps(bpvm_t* vm, int mod_idx, const char* search_dir) {
    /* Re-snapshot del import_count: si cargamos un dep, vm->modules crece
     * pero el mod actual no muta. */
    bpvm_module_t* m = &vm->modules[mod_idx];
    int n = m->import_count;
    for (int k = 0; k < n; k++) {
        const char* imp = m->imports[k];
        if (!imp || !imp[0]) continue;
        char lib[64], mod[64];
        derive_owner(imp, lib, sizeof(lib), mod, sizeof(mod));
        if (!mod[0]) continue;
        if (module_loaded(vm, lib, mod)) continue;
        /* Derivar filename. */
        char filename[512];
        if (lib[0]) {
            snprintf(filename, sizeof(filename), "%s%s%s.%s.mod",
                     search_dir, search_dir[0] ? "/" : "", lib, mod);
        } else {
            snprintf(filename, sizeof(filename), "%s%s%s.mod",
                     search_dir, search_dir[0] ? "/" : "", mod);
        }
        FILE* f = fopen(filename, "rb");
        if (!f) {
            fprintf(stderr, "[bpvm-c] dep '%s' (%s) no encontrado: %s\n",
                    imp, mod, filename);
            continue;   /* dejamos que linkAll dispare el error si falta. */
        }
        fclose(f);
        int idx_before = vm->module_count;
        bpvm_status_t s = bpvm_loader_load(vm, filename);
        if (s != BPVM_OK) return s;
        /* Recursivo: descubre las deps de esta nueva carga. */
        for (int j = idx_before; j < vm->module_count; j++) {
            bpvm_status_t r = discover_deps(vm, j, search_dir);
            if (r != BPVM_OK) return r;
        }
    }
    return BPVM_OK;
}

bpvm_status_t bpvm_load_mod_buffer(bpvm_t* vm, const uint8_t* data,
                                    size_t size, const char* name_hint) {
    if (!vm || !data || size == 0) return BPVM_ERR_IO;
    /* En target embebido NO descubrimos deps. El caller las carga manualmente. */
    return bpvm_loader_load_buffer(vm, data, size, name_hint);
}

bpvm_status_t bpvm_load_mod(bpvm_t* vm, const char* path) {
    if (!vm || !path) return BPVM_ERR_IO;
    int idx_before = vm->module_count;
    bpvm_status_t s = bpvm_loader_load(vm, path);
    if (s != BPVM_OK) return s;
    /* Descubrir y cargar recursivamente las dependencias del módulo,
     * buscándolas en el mismo directorio que el .mod cargado. */
    char dir[256];
    path_dirname(path, dir, sizeof(dir));
    for (int j = idx_before; j < vm->module_count; j++) {
        bpvm_status_t r = discover_deps(vm, j, dir);
        if (r != BPVM_OK) return r;
    }
    return BPVM_OK;
}

bpvm_status_t bpvm_run(bpvm_t* vm) {
    if (!vm) return BPVM_ERR_BAD_PC;
    /* F3 — resolver imports y aplicar class fixups antes de ejecutar. */
    bpvm_status_t ls = bpvm_link_all(vm);
    if (ls != BPVM_OK) return ls;

    /* Inicializar thread main desde el entry-point antes de entrar al
     * scheduler. */
    if (vm->main_absolute_address == 0) return BPVM_ERR_BAD_PC;
    bpvm_thread_t* main_tc = &vm->threads[0];
    main_tc->pc = vm->main_absolute_address;
    /* cs del módulo del entry-point. */
    for (int i = 0; i < vm->module_count; i++) {
        bpvm_module_t* m = &vm->modules[i];
        if (m->code_start <= vm->main_absolute_address
                && vm->main_absolute_address < m->end_addr) {
            main_tc->cs = m->code_start;
            break;
        }
    }
    if (main_tc->cs == 0) return BPVM_ERR_BAD_PC;
    main_tc->sp = main_tc->stack_base;
    main_tc->bp = main_tc->stack_base;
    main_tc->status = BPVM_THREAD_RUNNABLE;

    /* Default quantum si no se ajustó. */
    if (vm->quantum_ops == 0) vm->quantum_ops = 1024;

    return bpvm_scheduler_run(vm);
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
    /* Liberar módulos cargados. */
    for (int i = 0; i < vm->module_count; i++) {
        bpvm_module_t* m = &vm->modules[i];
        if (m->imports) {
            for (int k = 0; k < m->import_count; k++) free(m->imports[k]);
            free(m->imports);
        }
        free(m->class_fixups);
    }
    /* Liberar EH stacks y mutex waiters. */
    for (int i = 0; i < vm->thread_count; i++) {
        free(vm->threads[i].eh_stack);
    }
    for (int i = 0; i < vm->mutex_count; i++) {
        free(vm->mutexes[i].waiters);
    }
    free(vm->mutexes);
    free(vm->symbols);
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
