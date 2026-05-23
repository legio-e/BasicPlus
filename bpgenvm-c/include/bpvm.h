/*
 * bpvm.h — API pública de la VM BasicPlus en C.
 *
 * Implementación C99 del bytecode .mod descrito en docs/MOD_FORMAT.md,
 * docs/OPCODES.md, docs/HEAP_LAYOUT.md y docs/BUILTINS.md. La VM Java
 * en miVM/ es la implementación de referencia; si esta C diverge,
 * gana la spec en docs/.
 *
 * Fase 1 (F1): single-thread, sin heap dinámico, subset de opcodes
 * suficiente para programas puramente aritméticos + print int.
 *
 * Convenciones:
 *   - El caller PROVEE el buffer de memoria. La VM no llama malloc
 *     en runtime (sólo posiblemente al cargar para buffers temporales
 *     internos; F2+ los moverá a vm_init).
 *   - Todos los enteros del .mod y del memory[] son big-endian.
 *   - Esta API es thread-unsafe en F1: una sola vm_t y un solo
 *     thread la usa. F4 introducirá la abstracción FreeRTOS.
 */
#ifndef BPVM_H
#define BPVM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bpvm bpvm_t;

typedef enum {
    BPVM_OK = 0,
    BPVM_ERR_IO,             /* no se pudo abrir/leer el fichero */
    BPVM_ERR_BAD_MAGIC,      /* MAGIC del .mod no coincide */
    BPVM_ERR_BAD_HEADER,     /* tamaños inconsistentes en el header */
    BPVM_ERR_OOM,            /* memoria caller-provided insuficiente */
    BPVM_ERR_BAD_OPCODE,     /* opcode no implementado */
    BPVM_ERR_BAD_PC,         /* PC fuera de rango */
    BPVM_ERR_STACK_OVERFLOW,
    BPVM_ERR_DIV_BY_ZERO,
    BPVM_ERR_NULL_RECEIVER,  /* INVOKE_VIRTUAL sobre 0 — F3+ */
    BPVM_ERR_RUNTIME         /* RuntimeError BP no atrapado */
} bpvm_status_t;

/*
 * Callback de output. La VM lo invoca para PRINT_CHAR / PRINT / PRINT_*.
 * `s` no es necesariamente null-terminated; `len` indica los bytes
 * válidos. El caller puede agregar newline según el opcode (la VM ya
 * mete '\n' en los opcodes "con newline" como PRINT y PRINT_STRING).
 *
 * Si la VM no tiene callback registrado, los outputs van a stdout
 * via fwrite() — útil para Linux dev y tests.
 */
typedef void (*bpvm_output_cb)(const char* s, size_t len, void* user);

/*
 * Inicializa la VM con el buffer del caller. Devuelve un puntero a una
 * estructura bpvm_t alocada *internamente* (en F1 esto sí usa malloc
 * para la estructura de control — el `memory[]` es siempre del caller).
 * F2 expondrá una variante "all-static" para targets sin heap libc.
 *
 *   memory       — bloque de bytes que la VM gestiona como su RAM.
 *   memory_size  — tamaño del buffer en bytes.
 *   stack_base   — offset donde termina el heap y empiezan los stacks.
 *                  0 = default (memory_size / 2).
 *
 * El buffer debe seguir accesible durante toda la vida del bpvm_t.
 * Devuelve NULL si los parámetros son inválidos.
 */
bpvm_t* bpvm_init(uint8_t* memory, size_t memory_size, size_t stack_base);

/*
 * Variante embebida: carga un .mod desde un buffer ya en memoria. No
 * descubre dependencias (no hay filesystem). El caller debe pre-cargar
 * todas las dependencias llamando esta función en orden (deps primero).
 *
 *   data       — bytes del .mod (puede ser un array C generado con
 *                `xxd -i hello.mod` o cualquier blob que tengas en flash).
 *   size       — tamaño en bytes.
 *   name_hint  — nombre lógico opcional para módulos sin library prefix.
 *                Si es NULL, se genera "embedded<N>".
 *
 * El buffer NO necesita persistir tras la llamada: el loader copia los
 * data/code blocks al memory[] de la VM.
 */
bpvm_status_t bpvm_load_mod_buffer(bpvm_t* vm, const uint8_t* data,
                                    size_t size, const char* name_hint);

/*
 * Carga un .mod desde un fichero. Puede llamarse múltiples veces para
 * cargar el módulo principal + sus dependencias en orden.
 *
 * El loader:
 *   1. Lee y valida el header (MAGIC = 0x4D4F4435 / "MOD5").
 *   2. Inyecta la ext-table (zeroed), data block y code block en
 *      memory[] empezando en next_free_address.
 *   3. Registra el módulo en la tabla interna de loadedModules.
 *   4. Si tiene mainOffset >= 0 y es el primer módulo cargado,
 *      guarda el entry-point absoluto.
 *
 * En F1 NO se resuelven imports (sin linkAll, sin CALL_EXT funcional).
 * F2 / F3 lo añade.
 */
bpvm_status_t bpvm_load_mod(bpvm_t* vm, const char* path);

/*
 * Ejecuta el módulo cargado empezando en el entry-point principal.
 * Devuelve BPVM_OK si terminó normalmente (HALT del main thread).
 * Otros códigos indican el motivo del fallo.
 */
bpvm_status_t bpvm_run(bpvm_t* vm);

/*
 * Registra un callback para los opcodes PRINT_*. Si nunca se llama,
 * la VM escribe a stdout vía fwrite().
 */
void bpvm_set_output(bpvm_t* vm, bpvm_output_cb cb, void* user);

/*
 * Activa traza per-instrucción al stderr (para debug del intérprete
 * mismo). Coste alto, sólo para development.
 */
void bpvm_set_tracing(bpvm_t* vm, int enabled);

/*
 * Libera la estructura de control. NO toca el `memory[]` que pasó el
 * caller — eso es responsabilidad de quien lo asignó.
 */
void bpvm_destroy(bpvm_t* vm);

/*
 * Texto humano del status. Útil para logs.
 */
const char* bpvm_status_str(bpvm_status_t s);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_H */
