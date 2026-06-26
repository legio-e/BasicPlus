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

/* Paso 4 (V3) — compone un mensaje legible para un símbolo cross-module no
 * resuelto y lo guarda en vm->link_error, para que el handler de RUN lo mande
 * al wire (antes el detalle solo iba a stderr → exit-code mudo en el IDE).
 * Distingue "falta la lib" (el módulo dueño NO está cargado) de "lib presente
 * pero rancia" (cargado pero sin ese símbolo/slot). `who` = módulo que la usa.
 * El nombre de la lib = la parte antes del primer '.' del nombre cualificado
 * (p.ej. 'Json' en 'Json.JsonValue#asObject#15'). */
static void bpvm_link_set_error(bpvm_t* vm, const char* needed, const char* who) {
    char lib[40];
    size_t li = 0;
    while (needed[li] && needed[li] != '.' && li + 1 < sizeof(lib)) {
        lib[li] = needed[li]; li++;
    }
    lib[li] = '\0';

    int loaded = 0;
    if (lib[0]) {
        for (int i = 0; i < vm->module_count; i++) {
            if (strcmp(vm->modules[i].name, lib) == 0) { loaded = 1; break; }
        }
    }

    if (lib[0] && !loaded) {
        snprintf(vm->link_error, sizeof(vm->link_error),
                 "falta la lib '%s' (la usa '%s'; simbolo '%s')", lib, who, needed);
    } else if (lib[0]) {
        snprintf(vm->link_error, sizeof(vm->link_error),
                 "lib '%s' presente pero no exporta '%s' (la usa '%s'; version vieja?)",
                 lib, needed, who);
    } else {
        snprintf(vm->link_error, sizeof(vm->link_error),
                 "simbolo no resuelto '%s' (la usa '%s')", needed, who);
    }
}

/* Paso 4 (V3) — accessor público del detalle de fallo de link (ver bpvm.h). */
const char* bpvm_link_error(const bpvm_t* vm) {
    return (vm && vm->link_error[0]) ? vm->link_error : "";
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
                /* El loader registra el símbolo con y sin library-prefix, así
                 * que un lookup directo basta; si aún así falta, es lib ausente
                 * o rancia → mensaje claro al wire (paso 4). */
                bpvm_link_set_error(vm, needed, m->name);
                fprintf(stderr, "[bpvm-c link] %s\n", vm->link_error);
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
                bpvm_link_set_error(vm, fx->parent_qualified, m->name);
                fprintf(stderr, "[bpvm-c link] L2 v3 fixup (clase '%s'): %s\n",
                        fx->child_class_name, vm->link_error);
                return BPVM_ERR_RUNTIME;
            }
            uint32_t child_abs = m->code_start + fx->child_cs_off;
            int32_t parent_off_rel = (int32_t)(parent_abs - m->code_start);
            bpvm_write_i32_be(vm->memory + child_abs + BPVM_CLS_OFF_PARENT_OFF,
                              parent_off_rel);
        }

        /* BUG-2 — parcha el clsOff i32 de los TRY_BEGIN_EXT (catch cross-module). */
        for (int k = 0; k < m->eh_class_fixup_count; k++) {
            bpvm_eh_class_fixup_t* fx = &m->eh_class_fixups[k];
            uint32_t parent_abs = bpvm_link_lookup(vm, fx->parent_qualified);
            if (parent_abs == 0) {
                bpvm_link_set_error(vm, fx->parent_qualified, m->name);
                fprintf(stderr, "[bpvm-c link] BUG-2 eh-fixup: %s\n", vm->link_error);
                return BPVM_ERR_RUNTIME;
            }
            int32_t cls_off = (int32_t)(parent_abs - m->code_start);
            bpvm_write_i32_be(vm->memory + m->code_start + (uint32_t) fx->code_off,
                              cls_off);
        }
    }
    return BPVM_OK;
}
