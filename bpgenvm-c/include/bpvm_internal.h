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

#define BPVM_MAGIC          0x4D4F4435u   /* "MOD5" big-endian (v5, sin interfaz) */
#define BPVM_MAGIC_V6       0x4D4F4436u   /* "MOD6" big-endian (v6, H6.a: sección interface) */
#define BPVM_HEADER_SIZE    28            /* header v5; v6 = 32 (añade interfaceSize) */
#define BPVM_HEADER_SIZE_V6 32
#define BPVM_FORMAT_VERSION 6

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
#define BPVM_TYPE_ARRAY_I64 5   /* H1.2 (V2): array de long, 8 bytes/elem */

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
/* Su DIRECCIÓN. El frame inicial de un thread guarda este pc para que, al
 * volver run(), el dispatch ejecute THREAD_EXIT y el thread muera sin tumbar
 * la VM. #342 la reusa: es la vuelta correcta de un handler inyectado en un
 * thread que ya había terminado. */
#define BPVM_SENTINEL_THREAD_EXIT_ADDR 0u

/* Sentinela del puente native→BP (P-aot-call-bp): byte OP_NATIVE_RETURN en
 * memory[1]. El helper bpvm_aot_call_bp_* monta un frame BP con saved_pc
 * apuntando a esta dirección; cuando la función BP llamada hace RET, el
 * dispatch ejecuta este byte y rompe el bucle anidado. Ambos viven en la
 * región reservada [0, BPVM_INITIAL_FREE_ADDR). */
#define BPVM_SENTINEL_NATIVE_RETURN      0xAA
#define BPVM_SENTINEL_NATIVE_RETURN_ADDR 1u

/* H5.c — sentinela de vuelta de un handler de evento: byte OP_EVENT_RETURN en
 * memory[2]. La inyección monta el frame con saved_pc apuntando aquí; al hacer
 * RET el handler, el dispatch encuentra este byte, tira el valor de retorno y
 * salta al PC guardado bajo los argumentos. Tercero de la región reservada. */
#define BPVM_SENTINEL_EVENT_RETURN      0x6F
#define BPVM_SENTINEL_EVENT_RETURN_ADDR 2u

/* H5.c — COLA DE EVENTOS. Aridad acotada (el usuario escribe la firma natural;
 * la entrada de cola es de tamaño fijo). `masks` la rellena el COMPILADOR:
 * bits 0-3 = el argumento es referencia (para el GC), bits 8-11 = ocupa 8
 * bytes (para montar el frame). La VM no adivina ninguna de las dos cosas. */
#define BPVM_EVENT_MAX_ARGS   4
#define BPVM_EVENT_QUEUE_CAP 16
typedef struct {
    uint64_t recv;      /* handle 64b del receptor (bpref_t.v; el typedef vive más abajo) */
    int32_t  dest;      /* slot de vtable del handler */
    int32_t  tid;       /* thread destino = el que hizo el raise */
    uint32_t masks;     /* ref (0-3) | ancho-8B (8-11) */
    uint8_t  nargs;
    int64_t  args[BPVM_EVENT_MAX_ARGS];
} bpvm_event_t;

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

/* BUG-2 — eh-class fixup. Parchea el operando clsOff (i32) de un TRY_BEGIN_EXT,
 * que vive en code_start + code_off, con (parent_abs - code_start) para que
 * cs+clsOff apunte al descriptor de la clase de excepción de otro módulo.
 * H3.c/XIP: en un módulo-en-pack el código está en flash y NO se puede
 * parchear → el link deja el valor RESUELTO aquí y OP_TRY_BEGIN_EXT lo
 * consulta por code_off (tabla lateral; camino frío). */
typedef struct {
    int32_t  code_off;           /* offset del operando i32 relativo al code block */
    char     parent_qualified[128];
    int32_t  resolved_cls_off;   /* XIP: (parent_abs - cs), puesto por el link */
    int      resolved;           /* XIP: 1 si resolved_cls_off es válido */
} bpvm_eh_class_fixup_t;

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
    uint32_t end_addr;         /* fin del módulo EN RAM: código RAM → fin del code
                                  block; XIP → == code_start (el código no ocupa RAM) */
    int32_t  main_offset;      /* -1 si no es entry-point */
    /* H3.c — base de CÓDIGO del módulo (transitoria hasta CALL_REL, #307):
     * módulo RAM → cb == code_start (todo como siempre); módulo-en-pack (XIP)
     * → cb = dirección VIRTUAL del código en la región montada, elegida tal
     * que `vm->memory + cb == puntero real` (en host la región vive DENTRO
     * del buffer; en micro de 32 bits la resta envuelve módulo 2^32 y cae en
     * la flash). El CS queda INTACTO anclado en RAM (globals, descriptors,
     * cls_off: nada cambia); solo las conversiones offset-de-función→dirección
     * usan cb: OP_CALL, dispatch de vtable, exports de función y main. El
     * rango de código para "¿de qué módulo es este pc?" es [cb, cb+code_size). */
    uint32_t cb;

    /* F3 — imports cualificados (e.g. "L2Lib.Counter.__init"). Cada
     * entry k corresponde al slot k de la ext-table. */
    char**   imports;          /* malloc-ed; cada slot también malloc-ed */
    int      import_count;     /* = ext_count si las leemos */

    /* L2 v3 — class fixups del módulo. */
    bpvm_class_fixup_t* class_fixups;
    int                 class_fixup_count;

    /* BUG-2 — eh-class fixups (catch cross-module). */
    bpvm_eh_class_fixup_t* eh_class_fixups;
    int                    eh_class_fixup_count;
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
    /* H3.c — caché de 1 entrada cs→cb (por-thread = SMP-safe). cb NO viaja en
     * frames ni en tc: se DERIVA del cs vigente cuando hace falta (OP_CALL). */
    uint32_t cb_cache_cs;
    uint32_t cb_cache_cb;
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

    /* H5.c — profundidad de handlers de evento inyectados en este thread.
     * >0 = hay un handler CORRIENDO y el drenaje NO inyecta otro. Sin esto,
     * el siguiente punto de planificación puede caer DENTRO del handler y
     * meterle otro encima: el segundo termina antes que el primero y los
     * eventos dejan de atenderse en orden (medido: pasa en la VM-Java, con
     * su quantum por TIEMPO, y no en la C, con quantum por opcodes). Un
     * handler corre hasta el final antes de despachar el siguiente, como el
     * EDT de Swing. Un `raise` DESDE un handler sigue valiendo: eso encola,
     * no inyecta. */
    int  ev_depth;

    /* #342 — DEUDA DE EVENTOS AL MORIR. Un thread cuyo `raise` es lo último
     * que hace muere antes de llegar a una frontera de quantum, y su evento
     * se quedaba encolado para un tid muerto: desaparecía SIN UN RUIDO. Ahora
     * el scheduler lo resucita para que salde lo que debía.
     *   -1 = todavía vivo / sin calcular.
     *    N = eventos que tenía encolados EN EL MOMENTO de terminar. Es un
     *        PRESUPUESTO, no un contador: se drena lo que se debía entonces,
     *        no lo que un handler post-mortem añada después — si no, un
     *        programa que se realimenta no terminaría nunca.
     *    0 = deuda saldada; ya no se resucita. */
    int  ev_post_mortem;

    /* #139 — última línea origen vista por el debug hook; 0 = "ninguna
     * todavía". El hook sólo se invoca cuando la línea actual cambia
     * respecto a este valor, acotando la frecuencia a sentencias BP. */
    int  last_debug_line;

    /* H6.b — single-step: si !=0, el intérprete pausa tras ejecutar el
     * próximo opcode (lo setea el pause_cb al devolver BPVM_DBG_STEP).
     * Vive en el tc para sobrevivir entre quantums del scheduler SMP. */
    int  dbg_step;

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
struct aot_helpers_v2;

/* H6.b — máximo de breakpoints simultáneos. Tabla fija (sin malloc). 32 es
 * holgado para un debugger y barato en RAM en el MCU. */
#define BPVM_MAX_BREAKPOINTS 32

struct bpvm {
    /* #339 — marca del guardián de fin de RUN: el número de secuencia de
     * reserva que había justo al arrancar este programa. Todo bloque con
     * secuencia >= esta marca que siga vivo cuando bpvm_destroy termine es
     * memoria que se quedó sin limpiar. Ver bpvm_alloc.h. */
    uint64_t run_mark;

    /* Buffer del caller. */
    uint8_t* memory;
    size_t   memory_size;
    uint32_t stack_base;       /* offset donde termina heap y empiezan stacks */
    /* H3.c — ventana de PC válido para código XIP (fuera de memory_size): la
     * unión [xip_lo, xip_hi) de los rangos de código de los módulos-en-pack.
     * La acumula el loader XIP; el guardián de PC del bucle la acepta.
     * Vacía (lo>hi) si no hay módulos XIP → cero coste en el caso RAM. */
    uint32_t xip_lo;
    uint32_t xip_hi;

    /* #310 — PACK EN EJECUCIÓN. Cuando la VM arranca desde un pack, ese pack
     * va PRIMERO al resolver imports (y recursos), por delante del orden
     * normal; la zona de packs sigue siendo lo último. Es ADITIVO: sin pack en
     * ejecución (run_pack_on = 0) la resolución es exactamente la de antes.
     * `run_pack_st` guarda la ruta y tiene que seguir viva mientras la fuente
     * se use — por eso vive aquí y no en la pila del que arranca. */
    int             run_pack_on;
    bpvm_pack_src_t run_pack_src;
    bpvm_pack_fs_t  run_pack_st;

    /* Allocator del data block (bump). */
    uint32_t next_free_address;
    uint32_t heap_start;       /* fijado tras último módulo cargado */
    uint32_t heap_next;        /* bump del heap (F2) */
    /* H3 (V2): GC con free-list + reuso + coalescing + retreat + disparo por
     * umbral (espejo de la VM-Java). Bloque libre = [tag FREE][size@+4][next@+8]. */
    uint32_t free_list_head;   /* 0 = lista vacía */
    uint32_t last_gc_heap_next;/* heap_next en el último GC (histórico; ya no dispara) */
    /* #357 — BYTES RESERVADOS desde el último GC, vengan de donde vengan (lista de
     * libres O bump). ESTE es el que dispara la colecta ahora.
     *
     * Antes el umbral se medía sobre la DISTANCIA DE BUMP, y eso tenía dos
     * consecuencias malas: (1) el bump avanzaba un umbral entero por colecta
     * pasara lo que pasara —con heap/8, ocho colectas y muerte, aunque lo vivo
     * fueran 48 bytes—; y (2) la colecta caía SIEMPRE justo tras una ráfaga de
     * bump, o sea con objetos recién nacidos pegados al techo, así que el último
     * hueco nunca tocaba heap_next y el bump NO RETROCEDÍA JAMÁS ("recupera 0 B"
     * en todos los logs). Contando volumen, la colecta también ocurre MIENTRAS se
     * sirve de la lista, que es cuando el techo ya está muerto y se puede
     * devolver. */
    uint32_t alloc_since_gc;
    uint32_t gc_bump_threshold;/* bump máx. desde el último GC antes de colectar (0 = off) */
    /* #355 — reserva de emergencia: bytes del final del heap que el programa NO
     * puede tocar. Se sueltan cuando una reserva ya ha fallado, para que quede
     * sitio con el que CONSTRUIR el RuntimeError de OOM (mensaje + objeto).
     * Sin esto, quedarse sin memoria significaba quedarse tambien sin poder
     * avisar. 0 = ya gastada (o desactivada). */
    uint32_t heap_reserve;
    /* #355 — 1 mientras se esta CONSTRUYENDO una excepcion. Solo entonces puede
     * gastarse la reserva de emergencia: soltarla en cualquier throw se la
     * llevaba el primer RuntimeError de cualquier clase, y el aviso ademas
     * mentia ("heap lleno" con el heap recien vaciado). Lo vio el log de la Pico. */
    int      building_error;
    /* Camino 1 (H-008, v3.0.1): bitmap "¿es inicio de cabecera real?" (1 bit por
     * palabra de 4B del heap). Se reconstruye al empezar cada mark; el scan
     * conservativo solo valida candidatos cuya (v-4) esté aquí → un entero que
     * cae a mitad de un objeto NO se toma por raíz ni se marca en banda (que
     * pisaba datos vivos). Espejo del set `valid` de la VM-Java. Alloc perezoso
     * en el 1er GC; se libera en bpvm_destroy. */
    uint8_t* gc_valid_map;
    size_t   gc_valid_map_size;

    /* V4 — TABLA DE HANDLES (paso 2b, espejo de miVM). Un objeto de HEAP se
     * registra aquí y su ref es un HANDLE = índice | tag; bpref_deref lo resuelve.
     * Modo neutro: monotónica, sin generación (pasos 3-4). handle_addr[i] = addr
     * físico. El GC se SUSPENDE durante la migración (gc_suspended). */
    uint32_t* handle_addr;
    /* Paso 3 — GENERACIÓN por índice (contrato B). Monotónica-no-reuso: 0 = vivo;
     * >0 = LIBERADO. El deref de PROGRAMA (bpvm_ref_dead) lo consulta → use-after-free
     * grita. Gen de 1 bit; el handle 64b (gen en los 32 altos) llega en el paso 4. */
    uint32_t* handle_gen;
    uint32_t  handle_cap;      /* capacidad de handle_addr Y handle_gen */
    uint32_t  handle_next;     /* 0 reservado para null */
    /* Paso 4c — FREE-LIST de slots reciclables (pila LIFO). owner-free empuja el slot;
     * handle_register lo reusa con su gen ya bumpeada. Reclamación inmediata (1 worker);
     * la diferida-a-safepoint (SMP/ARM) se pliega al paso 6. */
    uint32_t* handle_free_list;
    uint32_t  handle_free_top;
    uint32_t  handle_free_cap;
    int       gc_suspended;    /* 1 = GC no corre (migración a handles) */

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

    /* H5.c — cola de eventos pendientes (anillo). El scheduler saca de aquí
     * ENTRE QUANTA e inyecta el frame del handler en el thread destino. */
    bpvm_event_t ev_queue[BPVM_EVENT_QUEUE_CAP];
    int          ev_head;
    int          ev_count;

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
     * instancia global del runtime (bpvm_aot_helpers_v2). El código
     * AOT C-emitido accede a helpers vía vm->aot_helpers->func(...).
     * Inicializado en bpvm_init. Definición en bpvm_aot_helpers.h. */
    const struct aot_helpers_v2* aot_helpers;

    /* #139 — Debug hook + lookup pc→línea. Si debug_hook == NULL el
     * inner loop sólo paga un null-check por opcode (negligible).
     * Cuando está instalado, paga además el debug_pc_to_line()
     * callback y la comparación contra tc->last_debug_line, con la
     * llamada efectiva al hook sólo en cambios de línea. */
    bpvm_debug_hook_t   debug_hook;
    bpvm_pc_to_line_t   debug_pc_to_line;
    void*               debug_user;

    /* H6.b — Debugger del device: breakpoints por pc + pausa/continue/step.
     * Coste hot-path cuando pause_cb==NULL: un único null-check por opcode
     * (igual que debug_hook). Tabla fija (sin malloc → MCU-friendly). */
    bpvm_pause_cb_t     pause_cb;
    void*               pause_user;
    volatile int        pause_requested;   /* PAUSE async (otro thread/task) */
    struct { uint32_t pc; int id; } breakpoints[BPVM_MAX_BREAKPOINTS];
    int                 bp_active;          /* nº de slots con id!=0 */
    int                 bp_next_id;         /* allocador de ids (>=1) */

    /* P-run-stop (#257) — KILL cooperativo. poll_cb (opcional) se invoca
     * desde el scheduler ENTRE quanta; kill_requested puede setearlo el
     * propio poll_cb o cualquier otra task (bpvm_request_kill). */
    bpvm_poll_cb_t      poll_cb;
    void*               poll_user;
    volatile int        kill_requested;

    /* H2 — Estado SMP (workers + comm task + locks). NULL = modo
     * single-worker legacy (F4 v1, scheduler.c). Cuando no-NULL, la
     * VM corre con scheduler_smp.c. Allocated by bpvm_smp_init(). */
    struct bpvm_smp* smp;

    /* Paso 4 (V3) — detalle legible del último fallo de link (lib/símbolo no
     * resuelto). Lo rellena bpvm_link_all; el handler de RUN lo manda al wire
     * en vez del exit-code mudo (antes el detalle solo iba a stderr). "" = sin
     * error. Tamaño fijo (sin malloc → MCU-friendly). */
    char link_error[160];

    /* Detalle del ÚLTIMO RuntimeError lanzado (msg de bpvm_throw_runtime_error):
     * p.ej. "referencia a objeto eliminado (use-after-free)". Espejo de
     * link_error → el host y el wire lo surten en vez del "exit N" mudo. Se
     * setea en CADA throw; solo se REPORTA cuando el run acaba con
     * BPVM_ERR_RUNTIME (no atrapado). "" = sin error. Fijo (MCU-friendly). */
    char runtime_error[192];
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
/* H1.2 (V2): long i64 big-endian (high word en p, low en p+4). */
static inline int64_t bpvm_read_i64_be(const uint8_t* p) {
    return (int64_t)(((uint64_t) bpvm_read_u32_be(p) << 32)
                   |  (uint64_t) bpvm_read_u32_be(p + 4));
}
static inline void bpvm_write_i64_be(uint8_t* p, int64_t v) {
    bpvm_write_u32_be(p,     (uint32_t)((uint64_t) v >> 32));
    bpvm_write_u32_be(p + 4, (uint32_t)  v);
}

/* ============================================================ */
/*  Referencia (V4) — LA "definición de referencia".            */
/*  Una referencia es un valor OPACO, distinto de un int y de un */
/*  long: NO se mezcla con int32/int64. Hoy encapsula el user_ref */
/*  plano (offset en memory[], = cabecera+4; 0 = null). El día    */
/*  que sea un handle (índice,generación) SOLO cambian los        */
/*  accesores de aquí + BPVM_REF_SIZE; los call sites no.         */
/*  Ver docs/V4_REF_ABSTRACTION.md.                               */
/* ============================================================ */

/* Bytes que ocupa una referencia EN memory[]/pila (el "slot"). Es el
 * footprint en MEMORIA, NO sizeof(bpref_t). Hoy: carril plano de 8 bytes. */
#define BPVM_REF_SIZE 8

/* V4/paso4: HANDLE de 64b = [gen:32 | idx|TAG:32]. bpref_t transporta los 64b;
 * bpref_deref/bpvm_ref_dead enmascaran la palabra baja para idx/tag y la alta para gen. */
typedef struct { uint64_t v; } bpref_t;

/* -- construcción / consulta -- */
static inline bpref_t bpref_null(void)               { bpref_t r; r.v = 0u; return r; }
static inline bool    bpref_is_null(bpref_t r)       { return r.v == 0u; }
static inline bool    bpref_eq(bpref_t a, bpref_t b) { return a.v == b.v; }

/* -- puente al mundo crudo: SOLO en la frontera de acceso a memory[].
 *    Aquí un handle se decodificaría a puntero/offset. Hoy, identidad. -- */
static inline uint32_t bpref_addr(bpref_t r)         { return r.v; }
static inline bpref_t  bpref_from_addr(uint32_t a)   { bpref_t r; r.v = a; return r; }

/* -- codificación en memory[]: LA frontera de representación (handle aquí) -- */
static inline bpref_t bpref_load(const bpvm_t* vm, uint32_t at) {
    bpref_t r; r.v = (uint64_t) bpvm_read_i64_be(vm->memory + at); return r;   /* 64b completo (gen en la palabra alta) */
}
static inline void bpref_store(bpvm_t* vm, uint32_t at, bpref_t r) {
    bpvm_write_i64_be(vm->memory + at, (int64_t) r.v);
}

/* -- pila del intérprete / builtins (unidades de BPVM_REF_SIZE) -- */
static inline bpref_t bpref_pop(bpvm_t* vm, bpvm_thread_t* tc) {
    tc->sp -= BPVM_REF_SIZE;
    return bpref_load(vm, tc->sp);
}
static inline void bpref_push(bpvm_t* vm, bpvm_thread_t* tc, bpref_t r) {
    bpref_store(vm, tc->sp, r);
    tc->sp += BPVM_REF_SIZE;
}

/* -- acceso a través de una referencia (genérico: la indirección, la cabecera y
 *    el stride viven AQUÍ, no en cada opcode). Cambiar el modelo de referencia
 *    (p.ej. handles) = tocar bpref_deref; los call sites no. -- */
#define BPVM_ARR_DATA_OFF 4u   /* bytes de user_ref al 1er elemento (prefijo length u32) */

/* V4 — bit 30 marca "es HANDLE de heap". null (0) y las CONSTANTES del data block
 * (dirección directa, inmutable/no-heap) tienen el bit a 0 → no necesitan tabla ni
 * generación. La memoria es <256KB (0x40000) → una dirección real jamás lo tiene. */
#define BPVM_HANDLE_TAG 0x40000000u

/* V4/paso4c: registra un objeto de HEAP y devuelve su HANDLE 64b (bpref_t =
 * gen(slot)<<32 | idx|TAG). Reusa slots de la free-list si los hay. Devuelve bpref_t
 * (NO uint32) a propósito: asignarlo a un uint32_t es error de compilación → el
 * compilador caza cada sitio que perdería la generación. Implementado en heap.c. */
bpref_t bpvm_handle_register(bpvm_t* vm, uint32_t addr);
/* Paso 3 — marca MUERTO el índice de un handle (owner-free). No-op para null y
 * constantes. Idempotente. Implementado en heap.c. */
void bpvm_handle_kill(bpvm_t* vm, bpref_t r);

/* -- H5.c: cola de eventos (events.c). El bpref_t del receptor viaja como
 *    uint64_t porque bpvm_event_t se declara antes del typedef. -- */
void bpvm_event_queue_init(bpvm_t* vm);
int  bpvm_event_enqueue(bpvm_t* vm, int tid, uint64_t recv, int32_t dest,
                        int nargs, uint32_t masks, const int64_t* args);
void bpvm_event_mark_roots(bpvm_t* vm, void (*visit)(bpvm_t*, uint32_t));
int  bpvm_event_drain_one(bpvm_t* vm, bpvm_thread_t* tc);
/* #342 — Vuelve a RUNNABLE los threads que ya terminaron pero dejaron eventos
 * SUYOS encolados, para que los atiendan antes de morir del todo. La llaman
 * los DOS schedulers desde su punto de wake-up: una sola regla, no una copia
 * por scheduler. Devuelve cuántos resucitó. */
int  bpvm_events_revive_terminated(bpvm_t* vm);

/* Paso 3 / contrato B — ¿es `r` un handle a un objeto LIBERADO? Solo los handles
 * (con TAG) pueden morir; null/constantes nunca. Lo consulta el deref de PROGRAMA
 * (opcodes de campo/array/invoke) para gritar "objeto eliminado". */
/* H13/#17 — cuando el deref grita, DECIR QUÉ handle. El mensaje "referencia a
 * objeto eliminado" a secas nos costó una tarde entera de cacería: no distingue
 * un truncamiento de una referencia caducada de verdad, y son bugs distintos con
 * arreglos distintos. Los tres números que hacen falta ya los tiene la VM en la
 * mano. Implementado en bpvm_util.c para no engordar este inline (sólo la rama
 * que ya va a abortar paga la llamada). */
void bpvm_uaf_report(uint32_t idx, uint32_t gen_handle, uint32_t gen_slot,
                     uint32_t handle_next);

static inline int bpvm_ref_dead(const bpvm_t* vm, bpref_t r) {
    if ((r.v & BPVM_HANDLE_TAG) == 0u) return 0;
    uint32_t idx = (uint32_t) r.v & ~BPVM_HANDLE_TAG;
    uint32_t gen = (uint32_t) (r.v >> 32);   /* generación embebida en la palabra alta */
    /* Paso 4b: compara la gen del handle con la del slot. Monotónico → todo handle
     * lleva gen=0, equivale al dead-flag; en 4c (reuso) un slot reciclado tiene gen
     * bumpeada y un handle rancio no matchea → grita. */
    if (vm->handle_gen == NULL || idx == 0u || idx >= vm->handle_next) return 0;
    if (vm->handle_gen[idx] == gen) return 0;
    bpvm_uaf_report(idx, gen, vm->handle_gen[idx], vm->handle_next);
    return 1;
}

/* V4 — reconstruye el HANDLE de 64b (gen VIVA<<32 | idx|TAG) para un `ref` uint32
 * que porta el TAG pero PERDIÓ la generación al truncarse desde un bpref_t de 64b
 * (p.ej. el retorno de bpvm_heap_alloc_string, o el trabajo interno de builtins.c
 * sobre uint32 idx|TAG). Debe invocarse cuando la gen del slot AÚN es la correcta
 * (objeto recién registrado, sin GC de por medio, o ref anclado). Un `ref` sin TAG
 * (null / dirección cruda / constante del data block) va tal cual, gen 0.
 * ÚNICA implementación del patrón: la usan push_ref (empujar a la pila) Y el guardado
 * del msg en el objeto RuntimeError (exceptions.c). Sin esto, un slot RECICLADO por el
 * GC (gen ≠ 0) recibe un handle con gen=0 → "referencia a objeto eliminado" al 1er uso. */
static inline bpref_t bpref_regen(const bpvm_t* vm, uint32_t ref) {
    if ((ref & BPVM_HANDLE_TAG) == 0u) return bpref_from_addr(ref);
    uint32_t idx = ref & ~BPVM_HANDLE_TAG;
    uint32_t gen = (vm->handle_gen != NULL && idx < vm->handle_next)
                   ? vm->handle_gen[idx] : 0u;
    bpref_t r; r.v = ((uint64_t) gen << 32) | (uint64_t) ref;
    return r;
}

/* Resolver una referencia a su offset en memory[] (DÓNDE vive el objeto). LA
 * indirección: sin TAG = null o constante del data block → identidad; con TAG =
 * consulta la tabla (defensivo: índice fuera de rango → 0). */
static inline uint32_t bpref_deref(const bpvm_t* vm, bpref_t r) {
    if ((r.v & BPVM_HANDLE_TAG) == 0u) return r.v;
    uint32_t idx = r.v & ~BPVM_HANDLE_TAG;
    if (idx == 0u || idx >= vm->handle_next) return 0u;
    /* Paso 7c — A1 publicación segura: ACQUIRE al leer el slot → garantiza VER el objeto
     * COMPLETAMENTE inicializado que el escritor publicó con RELEASE (bpvm_handle_register).
     * Es la mitad LECTOR del apretón de manos (el único punto de publicación = el slot).
     * INERTE en x86 (los loads ya son acquire); en ARM/RISC-V emite la barrera (dmb/ldar).
     * Validación real = fase de placa. */
    return __atomic_load_n(&vm->handle_addr[idx], __ATOMIC_ACQUIRE);
}
/* Longitud (nº de elementos) de un array, leída de su cabecera. 0 si null. */
static inline uint32_t bpref_arr_len(const bpvm_t* vm, bpref_t arr) {
    return bpref_is_null(arr) ? 0u
         : bpvm_read_u32_be(vm->memory + bpref_deref(vm, arr));
}
/* Puntero al elemento idx (SIN bounds check): deref + cabecera + idx*elem_size. */
static inline uint8_t* bpref_arr_elem(bpvm_t* vm, bpref_t arr, uint32_t idx, uint32_t elem_size) {
    return vm->memory + bpref_deref(vm, arr) + BPVM_ARR_DATA_OFF + idx * elem_size;
}
/* Puntero al campo `slot` de un objeto. Layout: user_ref -> [class_ptr u32][campos...]
 * (mismo +4 que arrays). El slot es de 4 bytes; el valor puede ser 4 u 8B. */
static inline uint8_t* bpref_field(bpvm_t* vm, bpref_t obj, uint32_t slot) {
    return vm->memory + bpref_deref(vm, obj) + BPVM_ARR_DATA_OFF + slot * 4u;
}

/* ---- H2 (V2): helpers UTF-8 sobre el payload de un string byte[] ----
 * Fuente UNICA para el intérprete (builtins.c) y el AOT (bpvm_aot_helpers.c):
 * deben coincidir byte a byte para la paridad bytecode <-> native. */
static inline uint32_t utf8_cp_count(const uint8_t* p, uint32_t nbytes) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < nbytes; i++)
        if ((p[i] & 0xC0u) != 0x80u) count++;
    return count;
}
/* Offset en bytes donde empieza el codepoint nº cp_index (0-based);
 * nbytes si cp_index >= nº de codepoints. */
static inline uint32_t utf8_byte_offset(const uint8_t* p, uint32_t nbytes, uint32_t cp_index) {
    uint32_t i = 0, cp = 0;
    while (i < nbytes && cp < cp_index) {
        i++;
        while (i < nbytes && (p[i] & 0xC0u) == 0x80u) i++;
        cp++;
    }
    return i;
}
/* Decodifica el codepoint que empieza en p[0]; *adv = bytes consumidos. */
static inline uint32_t utf8_decode(const uint8_t* p, uint32_t nbytes, uint32_t* adv) {
    if (nbytes == 0) { *adv = 0; return 0; }
    uint8_t b0 = p[0];
    if (b0 < 0x80u)                           { *adv = 1; return b0; }
    if ((b0 & 0xE0u) == 0xC0u && nbytes >= 2) { *adv = 2; return ((uint32_t)(b0 & 0x1Fu) << 6) | (p[1] & 0x3Fu); }
    if ((b0 & 0xF0u) == 0xE0u && nbytes >= 3) { *adv = 3; return ((uint32_t)(b0 & 0x0Fu) << 12) | ((uint32_t)(p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu); }
    if ((b0 & 0xF8u) == 0xF0u && nbytes >= 4) { *adv = 4; return ((uint32_t)(b0 & 0x07u) << 18) | ((uint32_t)(p[1] & 0x3Fu) << 12) | ((uint32_t)(p[2] & 0x3Fu) << 6) | (p[3] & 0x3Fu); }
    *adv = 1; return b0;   /* byte inválido: tratar como Latin-1 */
}
/* Codifica cp en out (hasta 4 bytes). Devuelve nº de bytes escritos. */
static inline uint32_t utf8_encode(uint32_t cp, uint8_t* out) {
    if (cp < 0x80u)    { out[0] = (uint8_t) cp; return 1; }
    if (cp < 0x800u)   { out[0] = (uint8_t)(0xC0u | (cp >> 6));  out[1] = (uint8_t)(0x80u | (cp & 0x3Fu)); return 2; }
    if (cp < 0x10000u) { out[0] = (uint8_t)(0xE0u | (cp >> 12)); out[1] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu)); out[2] = (uint8_t)(0x80u | (cp & 0x3Fu)); return 3; }
    out[0] = (uint8_t)(0xF0u | (cp >> 18)); out[1] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
    out[2] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu)); out[3] = (uint8_t)(0x80u | (cp & 0x3Fu)); return 4;
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
/* H3.c — variante XIP (módulo-en-pack): `data` debe apuntar a los bytes del
 * .mod DENTRO de la región de packs montada (persistentes toda la vida de la
 * VM). A RAM van solo ext-table + data block; el CÓDIGO no se copia — queda
 * direccionado por cb (ver bpvm_module_t.cb). El resto, idéntico. */
bpvm_status_t bpvm_loader_load_xip(bpvm_t* vm, const uint8_t* data,
                                    size_t size, const char* name_hint);
/* H11 — carga por trozos (ver bpvm_load_mod_stream en bpvm.h). */
bpvm_status_t bpvm_loader_load_stream(bpvm_t* vm, bpvm_read_at_fn rd, void* user,
                                       size_t size, const char* name_hint);

/* H3.c — cb del módulo cuyo CS es `cs` (caché 1-entrada en tc; módulos RAM
 * devuelven cs). Transitoria hasta CALL_REL (#307). */
uint32_t bpvm_cb_for_cs(const bpvm_t* vm, bpvm_thread_t* tc, uint32_t cs);
/* Módulo por dirección de CÓDIGO (rango [cb, cb+code_size)) o NULL. */
const bpvm_module_t* bpvm_module_for_code_addr(const bpvm_t* vm, uint32_t addr);

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
/* H-010 (v3.0.1): libera un bloque de objeto dejándolo consistente (size@+4 +
 * free-list), para que el recorrido del heap no se desincronice tras FREE_REF. */
void     bpvm_heap_free_block(bpvm_t* vm, uint32_t header_addr);
/* #355 — suelta la reserva de emergencia. La llama SOLO el camino del throw:
 * si la soltara la reserva normal al fallar, se la quedaria el programa y el
 * error se volveria a quedar sin memoria con la que construirse. */
uint32_t bpvm_heap_release_reserve(bpvm_t* vm);
uint32_t bpvm_heap_alloc_string(bpvm_t* vm, const char* s, size_t len);
void     bpvm_heap_gc(bpvm_t* vm);

/* interp.c — GAP-4: formateo canónico de double/float (byte-idéntico a
 * VirtualMachine.formatBpDouble). L13: también lo usan los builtins
 * FLOAT/DOUBLE_TO_STRING del concat. Devuelve la longitud escrita. */
int bpvm_format_double(char* out, double v);

/* interp.c — formateo de un int64 a decimal con signo, byte-idéntico y portable
 * en newlib-nano (NO usa %lld de snprintf, que el STM32 no lleva). L13: lo usa
 * BUILTIN_LONG_TO_STRING de builtins.c. Devuelve la longitud (sin '\0'). */
int bpvm_format_i64(char* out, int64_t v);

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
bpref_t bpvm_throw_runtime_error(bpvm_t* vm, bpvm_thread_t* tc,
                                  const char* msg);   /* V4: devuelve el handle 64b (gen); bpref_null() si no hay clase */

/* Realiza el unwind del stack para encontrar un handler que matchee.
 * Si encuentra: ajusta tc->pc/sp/bp/cs y deja `ref` en el top. Si no,
 * deja tc en estado terminado con stack trace al stderr. Devuelve 1
 * si fue atrapado, 0 si no. */
int bpvm_eh_unwind(bpvm_t* vm, bpvm_thread_t* tc, bpref_t ref);   /* V4: ref de excepción = handle 64b (gen preservada) */

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
    /* #213 — ref de una excepción YA construida (throw_ref desde native, p.ej.
     * una clase de usuario creada via factory __cls_new_* por el puente). El
     * boundary la propaga tal cual en vez de construir un RuntimeError. */
    volatile uint32_t pending_ref;
} bpvm_aot_fault_t;

/* Devuelve el slot de fault del worker actual (TLS en host). */
bpvm_aot_fault_t* bpvm_aot_fault_slot(void);

/* ---- P-aot-call-bp: contexto del puente native→BP ----
 * Cuando el intérprete divierte a un thunk AOT (OP_CALL/OP_CALL_EXT hijack),
 * fija este contexto POR WORKER con el `tc` activo y los punteros a los
 * registros sp/bp VIVOS del intérprete (los que el thunk muta). Así el helper
 * bpvm_aot_call_bp_*, invocado desde DENTRO del thunk, alcanza `tc` y los
 * registros sin cambiar el ABI del thunk (que NO recibe tc → no rompe .mdn).
 * Mismo razonamiento POR-WORKER que el fault-slot (#186): solo hay un native
 * activo por worker a la vez. tc==NULL ⇒ no estamos dentro de un thunk. */
typedef struct {
    bpvm_thread_t* tc;      /* thread cuyo stack usa el puente */
    uint32_t*      sp_p;    /* &sp local del intérprete (in/out) */
    uint32_t*      bp_p;    /* &bp local del intérprete (in/out) */
} bpvm_aot_callctx_t;

/* Contexto AOT del worker actual (TLS en host). */
bpvm_aot_callctx_t* bpvm_aot_callctx(void);

/* Puente native→BP (docs/AOT_CROSS_MODULE.md §8). Llamado desde un thunk AOT
 * (vía vm->aot_helpers->call_bp_i32): monta un frame BP para la función en
 * `target_abs`, empuja `nargs` args de 4 bytes, corre un bucle de intérprete
 * ANIDADO sobre el mismo tc hasta que la función hace RET (saved_pc =
 * sentinela OP_NATIVE_RETURN), y devuelve su valor de retorno (4 bytes).
 * Restricción v1: la función BP llamada debe completar sin ceder al scheduler
 * (sin sleep/mutex-contended/join). Si lo hace, o si target_abs es inválido,
 * lanza un RuntimeError BP vía el boundary de #186. */
int32_t bpvm_aot_call_bp_i32(struct bpvm* vm, uint32_t target_abs,
                             const int32_t* args, int nargs,
                             uint32_t ref_mask, int ret_is_ref);

/* H4 — puente builtin→función BP (upcall de eventos GUI). Como
 * bpvm_aot_call_bp_i32 pero desde un builtin (registros vivos en tc->sp/bp).
 * Ver interp.c. */
/* #302 paso 1 — ref_mask: bit i = arg i es una REFERENCIA (handle) → se empuja a
 * 8 bytes con la gen viva (bpref_regen), no a 4. Los args no-ref van a 4 bytes. */
int32_t bpvm_call_bp_from_builtin(bpvm_t* vm, bpvm_thread_t* tc,
                                  uint32_t target_abs, const int32_t* args, int nargs,
                                  uint32_t ref_mask);

/* P-aot-methods (#174, mitad-VM). Despacho VIRTUAL de un método público desde
 * native: resuelve la dirección vía la vtable de la clase REAL de `this_ref`
 * (slot dado por el compilador) y corre el cuerpo BP por el puente con
 * `this_ref` como arg0. Devuelve el retorno (4 bytes). NO requiere que el
 * método esté exportado. #302 paso 2: `ref_mask` marca los args-ref (el `this`
 * SIEMPRE es ref y lo añade la propia función); `ret_is_ref` el retorno-ref. */
int32_t bpvm_aot_call_method_i32(struct bpvm* vm, uint32_t this_ref, int slot,
                                 const int32_t* args, int nargs,
                                 uint32_t ref_mask, int ret_is_ref);

#endif /* BPVM_INTERNAL_H */
