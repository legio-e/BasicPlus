/*
 * link.c — linker dinámico tras cargar todos los módulos (F3).
 *
 * Responsabilidades:
 *   1. Lookup de un símbolo cualificado en la global symbol table.
 *   2. Helpers de mapeo addr → módulo (getCSForDataAddr y similares).
 *   3. bpvm_link_all() — para cada módulo, resuelve las entradas de su
 *      ext-table escribiéndolas en memory[]. Aplica también los class
 *      fixups L2 v3 (parche de parent_offset cross-module).
 *
 * F3 v1 — la global symbol table vive como una lista plana de pares
 * (name, abs_addr) construida durante el load. Lookup linear O(N);
 * suficiente para ~100 símbolos. Si crece, hashmap.
 */

#include "bpvm_internal.h"
#include <stdio.h>
#include <string.h>

/* ---- Helpers de mapeo addr → módulo ---- */

uint32_t bpvm_get_cs_for_data_addr(const bpvm_t* vm, uint32_t addr) {
    /* Busca el módulo cuyo rango [data_start, code_start) contiene addr.
     * O bien [moduleBase, code_start + codeSize) en el caso general.
     * Devolvemos code_start (= CS del módulo).
     *
     * NOTA: para class descriptors, addr cae en [data_start, code_start).
     * Para code addrs, en [code_start, end_addr). Ambos casos los cubre
     * la rama amplia [module_base, end_addr).
     */
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (addr >= m->module_base && addr < m->end_addr) {
            return m->code_start;
        }
    }
    return 0;
}

uint32_t bpvm_get_cs_for_code_addr(const bpvm_t* vm, uint32_t code_addr) {
    /* Variante para direcciones en el CODE block. Si code_addr está en
     * [code_start, end_addr), devuelve code_start. */
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (code_addr >= m->code_start && code_addr < m->end_addr) {
            return m->code_start;
        }
    }
    return 0;
}

uint32_t bpvm_get_ext_table_addr(const bpvm_t* vm, uint32_t cs) {
    /* Dado el CS de un módulo, devuelve su ext_table_addr (= module_base). */
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (m->code_start == cs) return m->ext_table_addr;
    }
    return 0;
}

/* ---- Global symbol table ----
 *
 * Almacenada en vm->symbols (array dinámico, alocado con realloc al
 * añadir). Las claves son strings hasta 128 chars; los valores son
 * direcciones absolutas en memory[].
 */

bpvm_status_t bpvm_link_register_symbol(bpvm_t* vm, const char* qualified,
                                         uint32_t abs_addr) {
    if (vm->symbol_count >= vm->symbol_capacity) {
        int new_cap = vm->symbol_capacity == 0 ? 32 : vm->symbol_capacity * 2;
        bpvm_symbol_t* new_arr = (bpvm_symbol_t*) realloc(vm->symbols,
                                  (size_t) new_cap * sizeof(bpvm_symbol_t));
        if (!new_arr) return BPVM_ERR_OOM;
        vm->symbols = new_arr;
        vm->symbol_capacity = new_cap;
    }
    bpvm_symbol_t* s = &vm->symbols[vm->symbol_count++];
    size_t n = strlen(qualified);
    if (n >= sizeof(s->name)) n = sizeof(s->name) - 1;
    memcpy(s->name, qualified, n);
    s->name[n] = '\0';
    s->abs_addr = abs_addr;
    return BPVM_OK;
}

uint32_t bpvm_link_lookup(const bpvm_t* vm, const char* qualified) {
    for (int i = 0; i < vm->symbol_count; i++) {
        if (strcmp(vm->symbols[i].name, qualified) == 0) {
            return vm->symbols[i].abs_addr;
        }
    }
    return 0;
}

/* ---- linkAll: resuelve imports y aplica class fixups ---- */

bpvm_status_t bpvm_link_all(bpvm_t* vm) {
    for (int i = 0; i < vm->module_count; i++) {
        bpvm_module_t* m = &vm->modules[i];

        /* Resolver imports: m->imports[k] → ext_table_addr + k*4. */
        for (int k = 0; k < (int) m->ext_count; k++) {
            const char* needed = m->imports[k];
            if (!needed || !needed[0]) continue;
            uint32_t addr = bpvm_link_lookup(vm, needed);
            if (addr == 0) {
                /* Heurística: ¿el caller usa el qualifiedName SIN library
                 * prefix cuando el dueño no tiene library? El loader ya
                 * registra ambas formas (con y sin prefix) — así que
                 * lookup directo bastará. Si aún así falta, error. */
                fprintf(stderr, "[bpvm-c link] símbolo no resuelto: '%s'"
                        " (módulo='%s')\n", needed, m->name);
                return BPVM_ERR_RUNTIME;
            }
            bpvm_write_i32_be(vm->memory + m->ext_table_addr
                              + (uint32_t) k * 4, (int32_t) addr);
        }

        /* L2 v3 — aplicar class fixups del módulo. */
        for (int k = 0; k < m->class_fixup_count; k++) {
            bpvm_class_fixup_t* fx = &m->class_fixups[k];
            uint32_t parent_abs = bpvm_link_lookup(vm, fx->parent_qualified);
            if (parent_abs == 0) {
                fprintf(stderr, "[bpvm-c link] L2 v3 fixup: parent '%s'"
                        " no resuelto para clase '%s' (módulo='%s')\n",
                        fx->parent_qualified, fx->child_class_name, m->name);
                return BPVM_ERR_RUNTIME;
            }
            uint32_t child_abs = m->code_start + fx->child_cs_off;
            int32_t parent_off_rel = (int32_t)(parent_abs - m->code_start);
            bpvm_write_i32_be(vm->memory + child_abs + BPVM_CLS_OFF_PARENT_OFF,
                              parent_off_rel);
        }
    }
    return BPVM_OK;
}
