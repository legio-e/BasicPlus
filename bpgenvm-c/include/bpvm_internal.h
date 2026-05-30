/*
 * bpvm_internal.h — tipos y constantes compartidos entre las TUs de la VM.
 * No exportado: el caller usa `bpvm.h`.
 */
#ifndef BPVM_INTERNAL_H
#define BPVM_INTERNAL_H

#include "bpvm.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

/* ============================================================ */
/*  Constantes extraídas de docs/MOD_FORMAT.md / HEAP_LAYOUT.md  */
/* ============================================================ */

#define BPVM_MAGIC          0x4D4F4435u   /* "MOD5" big-endian */
#define BPVM_HEADER_SIZE    28
#define BPVM_FORMAT_VERSION 5

/* Tamaño en bytes de una entrada de la ext-table (per-module). */
#define BPVM_EXT_ENTRY_SIZE 4

/* Stack del thread main; ver HEAP_LAYOUT.md §8 */
#define BPVM_MAIN_STACK_BYTES   (16 * 1024)
#define BPVM_THREAD_STACK_BYTES (2 * 1024)

/* Dirección inicial reservada (sentinela THREAD_EXIT en memory[0]). */
#define BPVM_INITIAL_FREE_ADDR  0x0100

/* Tag bits del header de objeto en heap (HEAP_LAYOUT.md §2.1). */
#define BPVM_TAG_MARK_BIT   0x80000000u
#define BPVM_TAG_FREE_BIT   0x40000000u
#define BPVM_TAG_TYPE_MASK  0x3F000000u
#define BPVM_TAG_TYPE_SHIFT 24

/* Códigos de tipo de heap. */
#define BPVM_TYPE_ARRAY_I8  0
#define BPVM_TYPE_ARRAY_I16 1
#define BPVM_TYPE_ARRAY_I32 2
#define BPVM_TYPE_ARRAY_REF 3
#define BPVM_TYPE_OBJECT    4

/* Header de objeto en heap. */
#define BPVM_OBJ_HEADER_SIZE 8
#define BPVM_MIN_FREE_BLOCK  12

/* Offsets del class descriptor (MOD_FORMAT.md §8). */
#define BPVM_CLS_OFF_NUM_FIELDS   0
#define BPVM_CLS_OFF_NUM_METHODS  2
#define BPVM_CLS_OFF_BITMAP_WORDS 4
#define BPVM_CLS_OFF_PARENT_OFF   8
#define BPVM_CLS_OFF_FIELD_BITMAP 12

/* Bytes sentinela en memory[0] — vea HEAP_LAYOUT.md §1. */
#define BPVM_SENTINEL_THREAD_EXIT 0x70

/* Máximo de módulos cargables simultáneamente.
   F1 no carga deps, así que 8 sobra. F3+ puede crecer si hace falta. */
#define BPVM_MAX_MODULES 16

/* Máximo de threads BP simultáneos. F1: sólo main (= 1). */
#define BPVM_MAX_THREADS 32

/* ============================================================ */
/*  Tipos                                                        */
/* ============================================================ */

/* L2 v3 — class fixup. Tras cargar todos los módulos, el linker resuelve
 * parent_qualified en la global symbol table y escribe parent_offset
 * relativo al CS del módulo del child. */
typedef struct {
    char     child_class_name[64];
    int32_t  child_cs_off;       /* offset del descriptor child relativo al CS */
    char     parent_qualified[128];
} bpvm_class_fixup_t;

typedef struct {
    char     library[64];      /* "" si no hay */
    char     name[64];
    uint32_t module_base;      /* dirección absoluta donde empieza la ext-table */
    uint32_t ext_table_addr;   /* = module_base */
    uint32_t ext_count;        /* nº de entries de la ext-table */
    uint32_t data_start;       /* = module_base + ext_count*4 */
    uint32_t data_size;
    uint32_t code_start;       /* = data_start + data_size; CS del módulo */
    uint32_t code_size;
    uint32_t end_addr;         /* code_start + code_size; primer byte libre */
    int32_t  main_offset;      /* -1 si no es entry-point */

    /* F3 — imports cualificados (e.g. "L2Lib.Counter.__init"). Cada
     * entry k corresponde al slot k de la ext-table. */
    char**   imports;          /* malloc-ed; cada slot también malloc-ed */
    int      import_count;     /* = ext_count si las leemos */

    /* L2 v3 — class fixups del módulo. */
    bpvm_class_fixup_t* class_fixups;
    int                 class_fixup_count;
} bpvm_module_t;

/* Tabla global de símbolos exportados (F3). */
typedef struct {
    char     name[128];        /* qualified: e.g. "L2Lib.Counter" o "Foo.__init" */
    uint32_t abs_addr;
} bpvm_symbol_t;

typedef enum {
    BPVM_THREAD_RUNNABLE = 0,
    BPVM_THREAD_RUNNING,
    BPVM_THREAD_TERMINATED,
    BPVM_THREAD_BLOCKED_SLEEP,
    BPVM_THREAD_BLOCKED_MUTEX,
    BPVM_THREAD_BLOCKED_JOIN,
    BPVM_THREAD_BLOCKED_PROMPT
} bpvm_thread_status_t;

/* NOTE: tagged como `struct bpvm_thread` para que el forward declarado
 * en bpvm.h (typedef struct bpvm_thread bpvm_thread_t;) cuadre — el
 * caller que sólo incluye bpvm.h ve el tipo opaco; los .c de la VM
 * (que incluyen bpvm_internal.h) ven la definición completa.
 *
 * `BPVM_THREAD_T_DEFINED` protege el typedef contra redefinición —
 * bpvm.h ya lo emitió como forward incompleto. */
#ifndef BPVM_THREAD_T_DEFINED
#define BPVM_THREAD_T_DEFINED
typedef struct bpvm_thread bpvm_thread_t;
#endif
struct bpvm_thread {
    int32_t  id;
    uint32_t pc;
    uint32_t sp;
    uint32_t bp;
    uint32_t cs;
    uint32_t stack_base;       /* dirección baja de su región de pila */
    uint32_t stack_top;        /* dirección alta (excluida) */
    bpvm_thread_status_t status;

    /* F4 — bloqueo y join. */
    int32_t  blocked_on_mutex;  /* mid o -1 */
    int32_t  blocked_on_join;   /* tid o -1 */
    int64_t  wake_at_ms;        /* timestamp absoluto (BLOCKED_SLEEP) */
    int32_t  thread_ref_heap;   /* user_ref del objeto Thread BP (para join) */

    /* F5 — Exception Handler Stack. Cada TRY_BEGIN empuja una entry;
     * TRY_END pop. THROW busca el primer handler cuya expected_class
     * matchee (o 0 = catch-all) y unwindea sp/bp/cs/pc desde lo
     * guardado allí. */
    struct bpvm_eh_entry* eh_stack;
    int  eh_stack_size;
    int  eh_stack_capacity;

    /* F5 — RuntimeError anclar para GC durante unwind. */
    int32_t  alloc_anchor;

    /* #139 — última línea origen vista por el debug hook; 0 = "ninguna
     * todavía". El hook sólo se invoca cuando la línea actual cambia
     * respecto a este valor, acotando la frecuencia a sentencias BP. */
    int  last_debug_line;

    /* H2 — Worker ID que actualmente está ejecutando el interp sobre
     * este tc. -1 = libre (puede ser pickeado). Sólo se modifica bajo
     * vm_lock. El scheduler usa esta flag (NO tc->status) para decidir
     * pickability porque tc->status lo escribe el interp sin sostener
     * vm_lock (write-races aceptados como triviales para status, pero
     * no para "está siendo ejecutado actualmente"). */
    int  sched_owner;
};

/* F5 — entry del handlerStack. */
typedef struct bpvm_eh_entry {
    int32_t handler_pc;        /* dirección absoluta donde saltar */
    int32_t saved_sp;
    int32_t saved_bp;
    int32_t saved_cs;
    int32_t expected_class;    /* dirección absoluta del class_ptr esperado;
                                  0 = catch-all. */
} bpvm_eh_entry_t;

/* F4 — Mutex BP. Lo gestionamos en lookups por id (no por ref BP) — el
 * id es lo que devuelve __mutexCreate y lo que GET_FIELD del objeto
 * Mutex BP entrega al builtin __mutexLock/Unlock. */
typedef struct {
    int32_t owner_tid;          /* tid del thread que lo tiene, -1 = libre */
    int32_t* waiters;           /* array dinámico de tids */
    int      waiter_count;
    int      waiter_capacity;
} bpvm_bp_mutex_t;

/* H3 #158 — forward del struct de helpers para código AOT. Definido
 * en src/bpvm_aot_helpers.h; lo referenciamos sólo por puntero aquí. */
struct aot_helpers_v1;

struct bpvm {
    /* Buffer del caller. */
    uint8_t* memory;
    size_t   memory_size;
    uint32_t stack_base;       /* offset donde termina heap y empiezan stacks */

    /* Allocator del data block (bump). */
    uint32_t next_free_address;
    uint32_t heap_start;       /* fijado tras último módulo cargado */
    uint32_t heap_next;        /* bump del heap (F2) */

    /* Módulos cargados. */
    bpvm_module_t modules[BPVM_MAX_MODULES];
    int           module_count;
    uint32_t      main_absolute_address;   /* 0 = sin entry-point */

    /* F3 — global symbol table (función exports + data exports). */
    bpvm_symbol_t* symbols;
    int            symbol_count;
    int            symbol_capacity;

    /* F4 — alocador de regiones de stack para nuevos threads BP. Cada
     * Thread.start() reserva una región a partir de `next_thread_stack`. */
    uint32_t       next_thread_stack;

    /* F4 — pool de mutexes BP. id 0 = no usado; ids ≥ 1 son válidos. */
    bpvm_bp_mutex_t* mutexes;
    int              mutex_count;
    int              mutex_capacity;

    /* F4 — preferencias de scheduler. */
    int             quantum_ops;       /* opcodes por quantum de un tc */

    /* Threads BP. F1: sólo main. */
    bpvm_thread_t threads[BPVM_MAX_THREADS];
    int           thread_count;
    int           current_thread_idx;

    /* Output sink. */
    bpvm_output_cb output_cb;
    void*          output_user;

    /* Flags. */
    bool tracing;

    /* Buffer staging para imports/exports al cargar — re-usable. */
    uint8_t* scratch;
    size_t   scratch_size;

    /* H3 #158 — Tabla de helpers para código AOT. Apunta a la
     * instancia global del runtime (bpvm_aot_helpers_v1). El código
     * AOT C-emitido accede a helpers vía vm->aot_helpers->func(...).
     * Inicializado en bpvm_init. Definición en bpvm_aot_helpers.h. */
    const struct aot_helpers_v1* aot_helpers;

    /* #139 — Debug hook + lookup pc→línea. Si debug_hook == NULL el
     * inner loop sólo paga un null-check por opcode (negligible).
     * Cuando está instalado, paga además el debug_pc_to_line()
     * callback y la comparación contra tc->last_debug_line, con la
     * llamada efectiva al hook sólo en cambios de línea. */
    bpvm_debug_hook_t   debug_hook;
    bpvm_pc_to_line_t   debug_pc_to_line;
    void*               debug_user;

    /* H2 — Estado SMP (workers + comm task + locks). NULL = modo
     * single-worker legacy (F4 v1, scheduler.c). Cuando no-NULL, la
     * VM corre con scheduler_smp.c. Allocated by bpvm_smp_init(). */
    struct bpvm_smp* smp;
};

/* ============================================================ */
/*  Helpers (util.c)                                             */
/* ============================================================ */

/* Lectura big-endian del memory[] (sin alignment requirements). */
static inline uint32_t bpvm_read_u32_be(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}
static inline int32_t bpvm_read_i32_be(const uint8_t* p) {
    return (int32_t) bpvm_read_u32_be(p);
}
static inline uint16_t bpvm_read_u16_be(const uint8_t* p) {
    return (uint16_t)(((uint32_t)p[0] << 8) | (uint32_t)p[1]);
}
static inline int16_t bpvm_read_i16_be(const uint8_t* p) {
    return (int16_t) bpvm_read_u16_be(p);
}
static inline void bpvm_write_u32_be(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8);
    p[3] = (uint8_t) v;
}
static inline void bpvm_write_i32_be(uint8_t* p, int32_t v) {
    bpvm_write_u32_be(p, (uint32_t) v);
}

/* Acceso al memory[] por dirección absoluta. Sin bounds-check en
   release; F2+ añade variantes "checked" en debug. */
static inline uint32_t bpvm_mem_read_u32(const bpvm_t* vm, uint32_t addr) {
    return bpvm_read_u32_be(vm->memory + addr);
}
static inline int32_t bpvm_mem_read_i32(const bpvm_t* vm, uint32_t addr) {
    return bpvm_read_i32_be(vm->memory + addr);
}
static inline void bpvm_mem_write_i32(bpvm_t* vm, uint32_t addr, int32_t v) {
    bpvm_write_i32_be(vm->memory + addr, v);
}

/* ============================================================ */
/*  Interfaces internas entre TUs                                */
/* ============================================================ */

/* loader.c */
bpvm_status_t bpvm_loader_load(bpvm_t* vm, const char* path);
/* Variante buffer: parsea un .mod ya en memoria. `name_hint` se usa como
 * nombre lógico si el módulo no tiene library prefix (en target embebido
 * no hay path del cual derivarlo). Puede ser NULL → "embedded". */
bpvm_status_t bpvm_loader_load_buffer(bpvm_t* vm, const uint8_t* data,
                                       size_t size, const char* name_hint);

/* H2 SMP — lock helpers condicionales.
 *
 * En modo single-worker (vm->smp == NULL) son no-op. En SMP toman el
 * vm_lock global. Pensados para envolver mutaciones a estado
 * compartido (heap_alloc, thread_spawn, mutex_pool ops) sin tener que
 * duplicar el código por modo. Inline-static para coste cero cuando
 * no hay SMP.
 *
 * NO usar dentro de regiones ya bajo vm_lock (ej. dentro del
 * scheduler_smp.c que ya lo agarra explícitamente) — eso sería
 * recursión, y nuestro vm_lock NO es recursivo. */
#include "bpvm_smp.h"
static inline void bpvm_smp_lock(bpvm_t* vm) {
    if (vm && vm->smp) {
        bpvm_platform_mutex_lock(&vm->smp->vm_lock);
    }
}
static inline void bpvm_smp_unlock(bpvm_t* vm) {
    if (vm && vm->smp) {
        bpvm_platform_mutex_unlock(&vm->smp->vm_lock);
    }
}

/* interp.c */
bpvm_status_t bpvm_interp_run(bpvm_t* vm);

/* heap.c (F2) */
uint32_t bpvm_heap_alloc(bpvm_t* vm, uint32_t payload_bytes, int type);
uint32_t bpvm_heap_alloc_string(bpvm_t* vm, const char* s, size_t len);
void     bpvm_heap_gc(bpvm_t* vm);

/* builtins.c (F2) */
bpvm_status_t bpvm_call_builtin(bpvm_t* vm, bpvm_thread_t* tc, int id);

/* link.c (F3) */
bpvm_status_t bpvm_link_register_symbol(bpvm_t* vm, const char* qualified,
                                         uint32_t abs_addr);
uint32_t      bpvm_link_lookup(const bpvm_t* vm, const char* qualified);
bpvm_status_t bpvm_link_all(bpvm_t* vm);
uint32_t      bpvm_get_cs_for_data_addr(const bpvm_t* vm, uint32_t addr);
uint32_t      bpvm_get_cs_for_code_addr(const bpvm_t* vm, uint32_t code_addr);
uint32_t      bpvm_get_ext_table_addr(const bpvm_t* vm, uint32_t cs);

/* scheduler.c (F4) — single-worker cooperative scheduler.
 * Pickea el siguiente tc RUNNABLE, le da un quantum, lo procesa. Wake
 * up de BLOCKED_SLEEP cuando expira wakeAt. Detecta deadlock si todos
 * están bloqueados sin posibilidad de progreso. */
bpvm_status_t bpvm_scheduler_run(bpvm_t* vm);

/* interp.c (F4) — ejecuta el tc dado por un quantum o hasta que ceda. */
bpvm_status_t bpvm_interp_run_quantum(bpvm_t* vm, bpvm_thread_t* tc,
                                       int max_ops, int* yielded);

/* Mutex BP helpers (F4). */
int  bpvm_mutex_alloc(bpvm_t* vm);                    /* devuelve nuevo mid */
void bpvm_mutex_add_waiter(bpvm_t* vm, int mid, int tid);
int  bpvm_mutex_pop_waiter(bpvm_t* vm, int mid);

/* Thread BP helpers (F4). */
int  bpvm_thread_spawn(bpvm_t* vm, uint32_t thread_ref);  /* devuelve tid o -1 */

/* F5 — Exception handling helpers. */
void bpvm_eh_push(bpvm_thread_t* tc, int32_t handler_pc, int32_t saved_sp,
                  int32_t saved_bp, int32_t saved_cs, int32_t expected_class);
void bpvm_eh_pop(bpvm_thread_t* tc);

/* Aloca un RuntimeError BP con `msg` y empuja el ref al stack de `tc`.
 * Devuelve el ref alocado (también queda en tc.alloc_anchor para que
 * el GC no lo libere). Si no encuentra la clase RuntimeError (módulo
 * no la exporta), devuelve 0 y el caller debe abortar el thread. */
uint32_t bpvm_throw_runtime_error(bpvm_t* vm, bpvm_thread_t* tc,
                                   const char* msg);

/* Realiza el unwind del stack para encontrar un handler que matchee.
 * Si encuentra: ajusta tc->pc/sp/bp/cs y deja `ref` en el top. Si no,
 * deja tc en estado terminado con stack trace al stderr. Devuelve 1
 * si fue atrapado, 0 si no. */
int bpvm_eh_unwind(bpvm_t* vm, bpvm_thread_t* tc, uint32_t ref);

/* ---- #186: boundary de fault para código AOT native ----
 *
 * El código native es C puro sin frame BP: un fault (bounds/null via
 * throw_runtime, o `throw RuntimeError(...)` explícito) no puede
 * "retornar" un error por el stack BP. En su lugar, el call-site AOT
 * del intérprete (interp.c OP_CALL/OP_CALL_EXT) arma un setjmp en este
 * slot ANTES de invocar el thunk; el helper throw_runtime hace longjmp
 * de vuelta, y el intérprete construye el RuntimeError + propaga via
 * bpvm_eh_unwind al try/catch BP que envuelva la llamada (o termina el
 * thread). Reutiliza toda la maquinaria F5 existente.
 *
 * El slot es POR WORKER (no por tc): sólo hay un native activo por
 * worker a la vez, así que basta uno por hilo de ejecución — y evita
 * inflar bpvm_thread_t ×N (la regresión OOM de #185). En host son N
 * pthreads → TLS (__thread); en Pico single-worker basta un global.
 * Multi-worker Pico (v2, con #153 dual-core) requerirá task-local. */
typedef struct {
    jmp_buf      buf;        /* destino del longjmp = boundary del interp */
    char         msg[128];   /* mensaje del fault, copiado por throw_runtime */
    volatile int armed;      /* 1 entre setjmp y fin del thunk */
} bpvm_aot_fault_t;

/* Devuelve el slot de fault del worker actual (TLS en host). */
bpvm_aot_fault_t* bpvm_aot_fault_slot(void);

#endif /* BPVM_INTERNAL_H */
