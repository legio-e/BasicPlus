/*
 * aot_registry.h — registro de funciones AOT (H3).
 *
 * Tabla pequeña que mapea dirección absoluta de target BP → thunk
 * C que ejecuta la versión nativa. El OP_CALL / OP_CALL_EXT del
 * intérprete consulta el registry antes de hacer el dispatch BP
 * estándar; si hay match, llama al thunk y se salta el frame BP.
 *
 * El thunk es responsable de:
 *   1. Leer args del top del BP stack (mem[sp-4], mem[sp-8], ...).
 *   2. Llamar la función C AOT con esos args.
 *   3. Restaurar sp al pre-args, push del resultado.
 *   4. NO modificar bp (la AOT 'flota' sobre el stack del caller).
 *
 * Ver docs/opcodes.html §"ABI de frontera AOT ↔ BP" para el diseño
 * completo.
 *
 * Capacidad fija — para #160 (experimento manual) bastan ~16-32.
 * Si crece el número de funciones AOT, redimensionar o pasar a hash.
 */
#ifndef BPVM_AOT_REGISTRY_H
#define BPVM_AOT_REGISTRY_H

#include <stdint.h>

/* Forward decl. bpvm_t es `typedef struct bpvm bpvm_t` en bpvm.h.
 * Aquí usamos `struct bpvm` directo para no depender de bpvm.h. */
struct bpvm;

/* Thunk del patrón ② (BP→AOT): convierte la convención BP en C-ABI.
 * sp_p y bp_p son in/out — el thunk actualiza sp tras pop args y
 * push result. bp se preserva (el thunk no crea frame BP). */
typedef void (*bpvm_aot_thunk_t)(struct bpvm* vm,
                                  uint32_t* sp_p,
                                  uint32_t* bp_p);

/* Registra un thunk asociado a una dirección absoluta de target en
 * memoria. La dirección viene típicamente del entry point de una
 * función BP cargada en el módulo.
 *
 * Devuelve 0 OK, -1 si la tabla está llena. */
int bpvm_aot_register(uint32_t target_abs_addr, bpvm_aot_thunk_t thunk);

/* Lookup por dirección absoluta. Devuelve el thunk o NULL.
 * Llamado desde el hot path del intérprete (OP_CALL / OP_CALL_EXT)
 * — debe ser barato. Con tabla < 32 entries un scan lineal es OK. */
bpvm_aot_thunk_t bpvm_aot_lookup(uint32_t target_abs_addr);

/* Variante helper: resuelve `qualified` en la global symbol table
 * de `vm` (típicamente "ModuleName.functionName") y registra el
 * thunk al abs_addr resultante.
 *
 * Devuelve 0 OK, -1 tabla llena, -2 símbolo no encontrado. */
int bpvm_aot_register_by_name(struct bpvm* vm,
                                const char* qualified,
                                bpvm_aot_thunk_t thunk);

/* Limpia el registry. Útil entre tests. */
void bpvm_aot_clear(void);

/* Para diagnóstico: nº de entries actuales. */
int bpvm_aot_count(void);

#endif /* BPVM_AOT_REGISTRY_H */
