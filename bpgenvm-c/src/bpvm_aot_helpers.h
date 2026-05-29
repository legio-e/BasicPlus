/*
 * bpvm_aot_helpers.h — tabla de helpers exportada del runtime al
 * código AOT (H3 #158 fase A).
 *
 * Esta tabla es la ABI estable VM↔AOT. Las funciones AOT acceden
 * a helpers del runtime indirectamente a través de un puntero a
 * esta estructura, que se les pasa vía `vm->aot_helpers`.
 *
 * Beneficios vs llamar directamente a bpvm_*:
 *   - El código AOT compilado con `-fpic` queda 100% relocatable
 *     (cero relocations a símbolos externos).
 *   - El .mdn puede llevarse como datos sin necesidad de linker.
 *   - Versionado: añadir slots SOLO al final de aot_helpers_v1_t.
 *     Si necesitas un helper nuevo y no quieres romper .mdn viejos,
 *     declarar aot_helpers_v2_t con los nuevos slots.
 *
 * Convenio: nombres en snake_case sin el prefijo bpvm_ — son los
 * mismos del runtime sin ruido. El generador AOT los referencia
 * vía vm->aot_helpers->name(...).
 */
#ifndef BPVM_AOT_HELPERS_H
#define BPVM_AOT_HELPERS_H

#include <stdint.h>
#include <stddef.h>

struct bpvm;

#ifdef __cplusplus
extern "C" {
#endif

/* Versión 1 del ABI. Slots añadidos solo al final; nunca reordenar
 * ni cambiar firmas en una version dada — el .mdn referencia por
 * offset (no por nombre), así que reordenar rompe módulos viejos. */
typedef struct aot_helpers_v1 aot_helpers_v1_t;
struct aot_helpers_v1 {
    /* --- I/O de memoria big-endian (los más usados) ----------- */
    int32_t  (*read_i32_be)(const uint8_t* p);
    void     (*write_i32_be)(uint8_t* p, int32_t v);
    int16_t  (*read_i16_be)(const uint8_t* p);
    void     (*write_i16_be)(uint8_t* p, int16_t v);

    /* --- Control de ejecución --------------------------------- */
    void     (*throw_runtime)(struct bpvm* vm, const char* msg);

    /* --- Heap / GC ------------------------------------------- */
    int32_t  (*newarray_i32)(struct bpvm* vm, int32_t size);
    int32_t  (*newarray_i8)(struct bpvm* vm, int32_t size);
    int32_t  (*newarray_i16)(struct bpvm* vm, int32_t size);
    int32_t  (*new_object)(struct bpvm* vm, uint32_t class_addr);

    /* --- Output sink ----------------------------------------- */
    void     (*print_i32)(struct bpvm* vm, int32_t v, int nl);
    void     (*print_f32)(struct bpvm* vm, float v, int nl);
    void     (*print_string)(struct bpvm* vm, uint32_t ref, int nl);
    void     (*print_char)(struct bpvm* vm, int32_t ch);
    void     (*print_nl)(struct bpvm* vm);

    /* --- AÑADIR AQUÍ slots futuros, NUNCA EN MEDIO ----------- */

    /* I/O float (H3 #166). El BP stack guarda los floats como bits
     * IEEE-754 en slots de 4 bytes big-endian. Los thunks AOT con
     * args/return float usan estas funciones para hacer la conversión
     * sin tener que hacer type-punning manual. */
    float    (*read_f32_be)(const uint8_t* p);
    void     (*write_f32_be)(uint8_t* p, float v);

    /* Acceso a arrays (H3 #167). El handle 'ref' es el offset al heap
     * donde vive el array (primeros 4 bytes = length BE, después los
     * elementos contiguos). Estos helpers hacen bounds-check + null-
     * check y lanzan runtime fault si fallan — mismo comportamiento
     * que los OP_ALOAD/OP_ASTORE del intérprete.
     *
     * Por ahora solo i32 (BP `integer[]`). Cuando AOT-eemos arrays
     * float/i8/i16/etc se añaden slots paralelos al final. */
    int32_t  (*array_load_i32)(struct bpvm* vm, uint32_t ref, int32_t idx);
    void     (*array_store_i32)(struct bpvm* vm, uint32_t ref, int32_t idx, int32_t v);
    int32_t  (*array_length)(struct bpvm* vm, uint32_t ref);

    /* Builtins comunes invocables desde código native (H3 #168).
     * Equivalente al OP_CALL_BUILTIN del intérprete pero accesible
     * como C call directa (sin push/pop del stack BP). */
    int32_t  (*now_ms)(struct bpvm* vm);   /* `now()` BP → ms desde boot */

    /* Variables nivel-módulo (#172). El offset CS-relativo del global
     * se conoce en compile-time (lo decide el bytecode emitter); el
     * thunk AOT lo bakea como constante. CS sí es runtime (depende de
     * en qué módulo cae cada uno al cargar), así que cada thunk
     * resuelve la CS de su módulo UNA VEZ con find_module_cs y la
     * cachea en una static. Los accesos posteriores son
     * read_i32_be(memory + CS + OFFSET_LITERAL) — coste idéntico al
     * intérprete pero sin dispatch de opcode.
     *
     * find_module_cs busca por nombre lógico del módulo (sin librería)
     * en la tabla `vm->modules`. Devuelve `code_start` del módulo, o
     * 0 si no se encontró (caller lanza runtime). */
    uint32_t (*find_module_cs)(struct bpvm* vm, const char* module_name);
};

/* Tabla v1 instanciada en el runtime con los punteros a las funciones
 * reales. Compartida entre todas las VMs del proceso. Pasada a cada
 * vm en bpvm_init via vm->aot_helpers. */
extern const aot_helpers_v1_t bpvm_aot_helpers_v1;

#ifdef __cplusplus
}
#endif

#endif /* BPVM_AOT_HELPERS_H */
