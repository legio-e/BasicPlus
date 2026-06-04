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
#include <stdbool.h>   /* H6.b — bool en la API pública (bpvm_debug_clear_breakpoint) */

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
    BPVM_ERR_RUNTIME,        /* RuntimeError BP no atrapado */
    BPVM_NATIVE_RETURN,      /* sentinela interno del puente native→BP
                              * (P-aot-call-bp): solo lo produce
                              * OP_NATIVE_RETURN dentro de un bucle anidado
                              * de bpvm_aot_call_bp_*; nunca escapa a
                              * bpvm_run. No es un error. */
    BPVM_DBG_STOPPED         /* H6.b — el debugger abortó la ejecución
                              * (pause_cb devolvió BPVM_DBG_STOP). */
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
 * H2 — Variante multi-worker. n_workers >= 1. El runtime arranca un
 * scheduler SMP con N pthreads + 1 comm task dedicado. El interp loop
 * corre lock-free salvo los safepoints (STW GC). Para n_workers=1 el
 * comportamiento es equivalente a bpvm_run() — ÚSALO solo si quieres
 * paralelismo real (n>=2). El bpvm_smp_destroy se llama automático
 * tras terminar el main thread.
 */
bpvm_status_t bpvm_run_smp(bpvm_t* vm, int n_workers);

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

/* ============================================================ */
/*  Debug hook (#139 P-interp-debug-hook).                       */
/* ============================================================ */

/*
 * Forward — definido en bpvm_internal.h. El hook recibe el thread
 * activo por puntero; el caller-VM no debe inspeccionar más allá de
 * id/pc/sp/bp/cs/stack_base.
 *
 * Guard para que bpvm_internal.h pueda usar `typedef struct bpvm_thread
 * { ... } bpvm_thread_t;` sin redefinición (C99-pedantic lo marca).
 */
#ifndef BPVM_THREAD_T_DEFINED
#define BPVM_THREAD_T_DEFINED
typedef struct bpvm_thread bpvm_thread_t;
#endif

/*
 * Resolución de pc absoluto → (línea origen, nombre de fichero). La
 * VM no parsea debug_lines del .mod en v1; el caller (el back-end de
 * debug, p.ej. el firmware de #140) suministra esta función con la
 * información que tenga.
 *
 * Devuelve la línea (>0) o ≤0 si el PC no tiene línea asociada (p.ej.
 * código generado, prólogos, etc.). Si `source_out` no es NULL, debe
 * apuntar al nombre del fichero fuente; el string debe permanecer
 * vivo el tiempo que la VM lo necesite (típicamente una cadena
 * interna del módulo).
 */
typedef int (*bpvm_pc_to_line_t)(uint32_t pc, const char** source_out,
                                  void* user);

/*
 * Hook invocado por la VM ANTES de despachar el opcode en `pc`, sólo
 * cuando la línea origen cambia respecto al opcode anterior. Esto
 * acota la frecuencia a sentencias BP, no a opcodes individuales.
 *
 * El hook puede:
 *  - Inspeccionar `tc` (id, pc/sp/bp/cs) — están sincronizados con
 *    el estado interno del intérprete justo antes de la llamada.
 *  - Bloquear el thread (semaforo, queue, etc.) hasta que el cliente
 *    de debug envíe continue/step.
 *  - NO debe mutar tc.pc/sp/bp/cs (edit-and-continue: deferred a v2).
 */
typedef void (*bpvm_debug_hook_t)(bpvm_t* vm, bpvm_thread_t* tc,
                                   uint32_t pc, int line,
                                   const char* source, void* user);

/*
 * Instala (o desinstala con NULL) el hook de debug. Cuando `hook` es
 * NULL la VM no paga coste alguno en el inner loop (un único null-
 * check por opcode); cuando está instalado paga además el
 * pc_to_line() callback y la comparación contra last_debug_line.
 *
 * `pc_to_line` puede ser NULL — en ese caso el hook se llama una vez
 * por opcode (modo "todo es una línea"). Útil para tests sintéticos
 * antes de tener la tabla de líneas del .mod.
 *
 * Thread-safety: el setter es THREAD-UNSAFE. Llamarlo ANTES de
 * arrancar la VM con bpvm_run().
 */
void bpvm_set_debug_hook(bpvm_t* vm,
                          bpvm_debug_hook_t hook,
                          bpvm_pc_to_line_t pc_to_line,
                          void* user);

/*
 * Accessor: id del thread BP. El struct interno es opaco para callers
 * fuera de la VM; este getter cubre el único campo que el hook
 * típicamente necesita exponer (qué tid disparó el break). Otros
 * accessors se irán añadiendo a medida que el back-end de debug los
 * pida (sp/bp/cs para frame walking, status para join, etc.).
 */
int bpvm_thread_id(const bpvm_thread_t* tc);

/* ============================================================ */
/*  H6.b — Debugger del device: breakpoints por pc + pausa.     */
/* ============================================================ */
/*
 * REGLA DE ORO (H6): el device trabaja SÓLO en pc/direcciones; el host
 * (IDE) tiene el `.dbg` y hace toda la traducción simbólica (línea↔pc,
 * slot/dir→nombre). Por eso los breakpoints aquí son POR PC ABSOLUTO: el
 * host convierte línea→pc y registra el pc; el core sólo compara.
 *
 * El "transporte" (servidor wire en host C, o tasks FreeRTOS en la Pico)
 * NO vive en el core: el embedder inyecta un bpvm_pause_cb_t que bloquea
 * como pueda (condvar / cola) y devuelve la acción siguiente. El core es
 * portable y agnóstico del transporte.
 */

/* Acción que el embedder devuelve desde el pause-callback. */
typedef enum {
    BPVM_DBG_CONTINUE = 0,   /* reanuda hasta el próximo breakpoint/pausa */
    BPVM_DBG_STEP     = 1,   /* ejecuta UNA instrucción y vuelve a pausar  */
    BPVM_DBG_STOP     = 2     /* aborta la ejecución (status BPVM_DBG_STOPPED) */
} bpvm_dbg_action_t;

/*
 * Pause-callback: lo llama el intérprete cuando alcanza una condición de
 * pausa (pc en un breakpoint, pausa pedida, o paso completado). El embedder
 * DEBE bloquear aquí (enviar BP_HIT al host, esperar continue/step/stop) y
 * devolver la acción. `tc` está sincronizado (pc/sp/bp/cs) para inspección;
 * NO mutar pc/sp/bp/cs (edit-and-continue diferido). `pc` = pc actual.
 */
typedef bpvm_dbg_action_t (*bpvm_pause_cb_t)(bpvm_t* vm, bpvm_thread_t* tc,
                                              uint32_t pc, void* user);

/* Instala/desinstala (NULL) el pause-callback. THREAD-UNSAFE: llamar antes
 * de bpvm_run(). Cuando es NULL el inner loop sólo paga un null-check. */
void bpvm_set_pause_cb(bpvm_t* vm, bpvm_pause_cb_t cb, void* user);

/* Registra un breakpoint en el pc absoluto dado. Devuelve un bpId>0, o
 * -1 si la tabla está llena. Idempotente por pc (si ya existe, devuelve su id). */
int  bpvm_debug_add_breakpoint(bpvm_t* vm, uint32_t pc);

/* Borra el breakpoint con el bpId dado. true si existía. */
bool bpvm_debug_clear_breakpoint(bpvm_t* vm, int bp_id);

/* Borra todos los breakpoints. */
void bpvm_debug_clear_breakpoints(bpvm_t* vm);

/* Vuelca los breakpoints activos en los buffers del caller (pc + id en
 * paralelo, hasta `max`). Devuelve cuántos hay. Cualquiera de los punteros
 * puede ser NULL si sólo interesa el conteo. */
int  bpvm_debug_list_breakpoints(bpvm_t* vm, uint32_t* out_pcs, int* out_ids, int max);

/* Pide una pausa asíncrona: el intérprete romperá en el próximo opcode.
 * Pensado para llamarse desde otro thread/task (el de RX del wire). */
void bpvm_debug_request_pause(bpvm_t* vm);

/* Accessors del frame para el embedder (reporta pc/sp/bp/cs crudos en
 * BP_HIT; el host resuelve nombres con el `.dbg`). */
uint32_t bpvm_thread_pc(const bpvm_thread_t* tc);
uint32_t bpvm_thread_sp(const bpvm_thread_t* tc);
uint32_t bpvm_thread_bp(const bpvm_thread_t* tc);
uint32_t bpvm_thread_cs(const bpvm_thread_t* tc);

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
