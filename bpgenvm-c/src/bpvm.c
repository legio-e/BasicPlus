/*
 * bpvm.c — implementación de la API pública.
 *
 * En F1 mantenemos malloc/free para la estructura de control bpvm_t.
 * El memory[] sigue siendo del caller. F2+ podría ofrecer una variante
 * "todo-en-buffer-del-caller" para targets sin libc.
 */

#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"   /* H3 #158: tabla helpers para AOT */
#include "bpvm_pack.h"          /* H3.c: resolución de imports contra la zona de packs */
#include "bpvm_fs.h"            /* #310: un pack ejecutable vive en /app del FS */
#include "bpvm_alloc.h"         /* #339: guardián de fin de RUN */
#include "bpvm_entry.h"         /* #344: el RUN, escrito UNA vez */
#ifdef BPVM_GUI
#include "bpvm_gui.h"          /* #352: el modelo del GUI se va con la VM */
#endif
#include "bpvm_platform.h"      /* #339: candado de hoja de la lista de bloques */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>             /* #353: bpvm_diag es variádica */

/* ==========================================================================
 * #339 — GUARDIÁN DE FIN DE RUN (idea de Eduardo, 28-jul)
 *
 * "No preguntes cuánta memoria libre queda: pon un contador. Desde la marca de
 *  arranque del programa, todo bloque posterior que siga vivo es memoria que se
 *  quedó sin limpiar."
 *
 * Eso es literalmente lo que hay aquí: cada reserva del núcleo lleva delante
 * una cabecera con su NÚMERO DE SECUENCIA, su tamaño y dónde se pidió, y va
 * encadenada en una lista de bloques vivos. bpvm_init anota la secuencia del
 * momento; bpvm_destroy recorre la lista y canta lo que tenga secuencia >= esa
 * marca.
 *
 * La alternativa (preguntarle al sistema la memoria libre antes y después) se
 * DESCARTÓ: da un número que ensucian otras tareas y la fragmentación, obliga a
 * una cintura por micro —justo lo que la arquitectura BP evita— y sólo sabe
 * decir cuánto falta, no quién se lo quedó.
 *
 * Coste: la cabecera por bloque. Las reservas C del núcleo son POCAS y GRANDES
 * (tabla de handles, símbolos, imports, EH stacks), así que es despreciable;
 * la memoria del programa BP no pasa por aquí (vive en memory[], el bloque del
 * llamante, que el siguiente RUN reinicia entero).
 * ========================================================================== */

#define BPVM_BLK_MAGIC 0x424C4B21u   /* "BLK!" — cazar free() de puntero ajeno */

/* Las DOS bases de contador (idea de Eduardo). Muy separadas a propósito: el
 * rango de la secuencia ES la etiqueta de familia, sin campo extra. Con 2^62 de
 * hueco no hay forma de que una alcance a la otra ni en una eternidad de RUNs. */
#define BPVM_SEQ_OS_BASE  ((uint64_t) 1)
#define BPVM_SEQ_VM_BASE  ((uint64_t) 1 << 62)

typedef struct bpvm_blk {
    struct bpvm_blk* prev;
    struct bpvm_blk* next;
    uint64_t         seq;      /* número de secuencia: EL flag de Eduardo.
                                * >= BPVM_SEQ_VM_BASE → bloque de la VM. */
    size_t           size;     /* bytes útiles (sin cabecera) */
    const char*      file;     /* dónde se pidió... */
    int              line;     /* ...para poder señalar con el dedo */
    uint32_t         magic;
} bpvm_blk_t;

#define BLK_IS_VM(b) ((b)->seq >= BPVM_SEQ_VM_BASE)

/* Alineación: la cabecera se redondea para que el payload salga alineado como
 * lo estaría un malloc normal (double/uint64 en todas las familias). */
#define BPVM_BLK_HDR ((sizeof(bpvm_blk_t) + 15u) & ~(size_t)15u)

static bpvm_blk_t* g_blk_head = NULL;
static uint64_t    g_seq_os   = BPVM_SEQ_OS_BASE;
static uint64_t    g_seq_vm   = BPVM_SEQ_VM_BASE;
static uint32_t    g_live[2]  = { 0, 0 };   /* [VM], [OS] */
static uint64_t    g_bytes[2] = { 0, 0 };

/* Candado de HOJA: sólo protege el empalme en la lista, nunca se tiene cogido
 * mientras se hace otra cosa. Por eso no puede interbloquear con vm_lock (que
 * no es recursivo).
 *
 * El huevo y la gallina: crear el candado necesita un mutex de plataforma, que
 * RESERVA memoria, que querría el candado. Se corta con un pestillo de
 * reentrada — durante esa única reserva se pasa sin candado, que es correcto
 * porque ocurre en la primera reserva del proceso, cuando aún no hay más de un
 * hilo. Y como esa reserva es de familia OS, tampoco ensucia nunca el veredicto
 * sobre la memoria del programa. */
static bpvm_platform_mutex_handle_t g_blk_mtx = NULL;
static int g_blk_mtx_ready = 0;
static int g_blk_mtx_busy  = 0;

static int blk_lock(void) {
    if (!g_blk_mtx_ready) {
        if (g_blk_mtx_busy) return 0;          /* reentrada: aún single-thread */
        g_blk_mtx_busy = 1;
        int ok = (bpvm_platform_mutex_init(&g_blk_mtx) == 0);
        g_blk_mtx_busy = 0;
        if (!ok) return 0;   /* sin mutex: seguimos; el guardián no tumba la VM */
        g_blk_mtx_ready = 1;
    }
    bpvm_platform_mutex_lock(&g_blk_mtx);
    return 1;
}
static void blk_unlock(int locked) {
    if (locked) bpvm_platform_mutex_unlock(&g_blk_mtx);
}

static void blk_link(bpvm_blk_t* b, bpvm_alloc_kind_t k) {
    int lk = blk_lock();
    b->seq  = (k == BPVM_ALLOC_VM) ? g_seq_vm++ : g_seq_os++;
    b->prev = NULL;
    b->next = g_blk_head;
    if (g_blk_head) g_blk_head->prev = b;
    g_blk_head = b;
    g_live[k]++;
    g_bytes[k] += (uint64_t) b->size;
    blk_unlock(lk);
}

static void blk_unlink(bpvm_blk_t* b) {
    int k = BLK_IS_VM(b) ? BPVM_ALLOC_VM : BPVM_ALLOC_OS;
    int lk = blk_lock();
    if (b->prev) b->prev->next = b->next; else g_blk_head = b->next;
    if (b->next) b->next->prev = b->prev;
    g_live[k]--;
    g_bytes[k] -= (uint64_t) b->size;
    blk_unlock(lk);
}

void* bpvm_alloc_raw(size_t n, int zero, bpvm_alloc_kind_t k,
                     const char* file, int line) {
    uint8_t* raw = (uint8_t*) malloc(BPVM_BLK_HDR + n);
    if (!raw) return NULL;
    bpvm_blk_t* b = (bpvm_blk_t*) raw;
    b->size = n; b->file = file; b->line = line; b->magic = BPVM_BLK_MAGIC;
    blk_link(b, k);
    if (zero) memset(raw + BPVM_BLK_HDR, 0, n);
    return raw + BPVM_BLK_HDR;
}

void bpvm_free(void* p) {
    if (!p) return;
    bpvm_blk_t* b = (bpvm_blk_t*) ((uint8_t*) p - BPVM_BLK_HDR);
    if (b->magic != BPVM_BLK_MAGIC) {
        /* Puntero que no salió de aquí. GRITA en vez de corromper la lista:
         * es un error de programación (reservado con malloc y liberado con
         * bpvm_free, o doble free), y en silencio sería indetectable. */
        bpvm_diag("[bpvm] bpvm_free: puntero que no reservó el núcleo (%p)", p);
        return;
    }
    blk_unlink(b);
    b->magic = 0;
    free(b);
}

void* bpvm_realloc_at(void* p, size_t n, const char* file, int line) {
    if (!p) return bpvm_alloc_raw(n, 0, BPVM_ALLOC_VM, file, line);
    bpvm_blk_t* old = (bpvm_blk_t*) ((uint8_t*) p - BPVM_BLK_HDR);
    if (old->magic != BPVM_BLK_MAGIC) {
        bpvm_diag("[bpvm] bpvm_realloc: puntero que no reservó el núcleo (%p)", p);
        return NULL;
    }
    bpvm_alloc_kind_t k = BLK_IS_VM(old) ? BPVM_ALLOC_VM : BPVM_ALLOC_OS;
    /* Se desencadena ANTES de realloc: puede mover el bloque, y los vecinos
     * apuntarían a memoria liberada. Al volver se re-encadena (secuencia NUEVA
     * de su misma familia: un realloc es un bloque nuevo para el guardián). */
    size_t keep_size = old->size;
    blk_unlink(old);
    uint8_t* raw = (uint8_t*) realloc((void*) old, BPVM_BLK_HDR + n);
    if (!raw) {
        /* realloc falló: el bloque original SIGUE vivo. Re-encadenarlo. */
        old->size = keep_size;
        blk_link(old, k);
        return NULL;
    }
    bpvm_blk_t* b = (bpvm_blk_t*) raw;
    b->size = n; b->file = file; b->line = line; b->magic = BPVM_BLK_MAGIC;
    blk_link(b, k);
    return raw + BPVM_BLK_HDR;
}

char* bpvm_strdup_at(const char* s, const char* file, int line) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* d = (char*) bpvm_alloc_raw(n, 0, BPVM_ALLOC_VM, file, line);
    if (d) memcpy(d, s, n);
    return d;
}

uint32_t bpvm_alloc_live_blocks(bpvm_alloc_kind_t k) { return g_live[k]; }
uint64_t bpvm_alloc_live_bytes (bpvm_alloc_kind_t k) { return g_bytes[k]; }

uint64_t bpvm_alloc_mark(void) { return g_seq_vm; }

/* ── #353: el canal de diagnóstico de la VM ──────────────────────────────────
 *
 * Vive aquí, en bpvm.c, A PROPÓSITO: un .c nuevo del núcleo hay que darlo de
 * alta en CINCO builds, y olvidar uno significa que esa familia no enlaza y no
 * te enteras hasta reconstruirla. bpvm.c ya está en los cinco.
 *
 * fflush OBLIGATORIO en el default: con stderr redirigido a fichero o tubería
 * (que es como corre el micro simulado bajo el IDE) el runtime lo vuelve
 * BUFFERIZADO, y si al proceso lo matan el aviso se queda dentro sin llegar a
 * nadie. Un guardián cuyo veredicto se pierde justo en el caso violento no
 * sirve para nada. */
static void diag_stderr(const char* linea) {
    fprintf(stderr, "%s\n", linea);   /* el '\n' lo pone EL SINK, no el llamante */
    fflush(stderr);
}
static bpvm_diag_fn g_diag = diag_stderr;

void bpvm_diag_set_sink(bpvm_diag_fn fn) { g_diag = fn ? fn : diag_stderr; }

void bpvm_diag(const char* fmt, ...) {
    /* Buffer de PILA y acotado: esto se llama desde el loader y desde el fin de
     * RUN, en micros con la pila contada. 224 B es lo que ya usaba el guardián
     * y da de sobra para un nombre de módulo con su explicación. */
    char linea[224];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(linea, sizeof linea, fmt, ap);
    va_end(ap);
    g_diag(linea);
}

/* El guardián de #339 mantiene su propio sink porque el test lo CAPTURA para
 * comprobar el veredicto palabra por palabra; si compartiera el de diag, el
 * test se comería también los mensajes del loader. Su default, eso sí, es el
 * canal común: así un firmware que instale el sink de diag se lleva el
 * veredicto del guardián al log sin tener que acordarse de dos cosas. */
static void report_via_diag(const char* linea) { g_diag(linea); }
static bpvm_alloc_report_fn g_report = report_via_diag;

void bpvm_alloc_set_report(bpvm_alloc_report_fn fn) {
    g_report = fn ? fn : report_via_diag;
}

uint64_t bpvm_alloc_sweep(uint64_t mark) {
    char linea[224];
    uint32_t n = 0;
    uint64_t bytes = 0;
    const bpvm_blk_t* peor = NULL;

    int lk = blk_lock();
    for (const bpvm_blk_t* b = g_blk_head; b; b = b->next) {
        if (!BLK_IS_VM(b)) continue;              /* plataforma: no es del programa */
        if (b->seq < mark) continue;              /* de antes del RUN: tampoco */
        n++;
        bytes += (uint64_t) b->size;
        if (!peor || b->size > peor->size) peor = b;
    }
    uint32_t os_n = g_live[BPVM_ALLOC_OS];
    blk_unlock(lk);

    /* Habla SIEMPRE. Un guardián que sólo abre la boca cuando hay problema no
     * se distingue de uno averiado — la lección de #326 (todo instrumento
     * necesita su control). Los bloques de plataforma van como DATO al final,
     * nunca mezclados con el veredicto. */
    if (n == 0) {
        snprintf(linea, sizeof linea,
                 "[bpvm] fin de RUN: la memoria del programa vuelve a su sitio "
                 "(0 bloques sin liberar; plataforma: %lu vivos)",
                 (unsigned long) os_n);
        g_report(linea);
        return 0;
    }
    snprintf(linea, sizeof linea,
             "[bpvm] fin de RUN: FUGA — %lu bloque(s) del programa sin liberar, %lu B; "
             "el mayor (%lu B) se pidio en %s linea %d (plataforma: %lu vivos)",
             (unsigned long) n, (unsigned long) bytes,
             (unsigned long) (peor ? peor->size : 0),
             peor && peor->file ? peor->file : "?",
             peor ? peor->line : 0,
             (unsigned long) os_n);
    g_report(linea);
    return bytes;
}

size_t bpvm_stack_region_bytes(size_t total_bytes) {
    size_t r = total_bytes / 4u;                    /* 25% para stacks */
    if (r < 64u * 1024u)  r = 64u * 1024u;          /* ...nunca menos (threads) */
    if (r > 512u * 1024u) r = 512u * 1024u;         /* ...nunca más (PSRAM) */
    if (r >= total_bytes) r = total_bytes / 2u;     /* bloque diminuto: mitad */
    return r;
}

bpvm_t* bpvm_init(uint8_t* memory, size_t memory_size, size_t stack_base) {
    if (memory == NULL || memory_size < 4096) return NULL;
    if (stack_base == 0) stack_base = memory_size / 2;
    if (stack_base >= memory_size) return NULL;
    if (stack_base + BPVM_MAIN_STACK_BYTES > memory_size) return NULL;

    /* #339 — la marca se toma AQUÍ, antes de reservar nada nuestro, para que la
     * propia estructura del vm quede DENTRO de la ventana: si algún día se
     * escapara, el guardián la vería como cualquier otro bloque. */
    uint64_t run_mark = bpvm_alloc_mark();

    bpvm_t* vm = (bpvm_t*) bpvm_calloc(1, sizeof(bpvm_t));
    if (!vm) return NULL;
    vm->run_mark = run_mark;
    /* H3.c — ventana XIP vacía (lo>hi imposible con lo=MAX): el guardián de PC
     * rechaza todo fuera de memory_size hasta que un loader XIP la abra. */
    vm->xip_lo = 0xFFFFFFFFu;
    vm->xip_hi = 0;

    vm->memory = memory;
    vm->memory_size = memory_size;
    vm->stack_base = (uint32_t) stack_base;
    vm->next_free_address = BPVM_INITIAL_FREE_ADDR;
    vm->heap_start = (uint32_t) stack_base;   /* sin módulos cargados aún */
    vm->heap_next  = vm->heap_start;
    /* H3 (V2): el loader fijará el umbral real con el heap_start tras cargar. */
    vm->free_list_head    = 0;
    vm->last_gc_heap_next = vm->heap_next;
    vm->gc_bump_threshold = 0;   /* off hasta entonces */
    vm->main_absolute_address = 0;
    /* V4 — tabla de handles (lazy) + GC suspendido durante la migración. */
    vm->handle_addr  = NULL;
    vm->handle_gen   = NULL;     /* paso 3: generación por índice (contrato B) */
    vm->handle_free_list = NULL; /* paso 4c: free-list de slots reciclables */
    vm->handle_free_top  = 0;
    vm->handle_free_cap  = 0;
    vm->handle_cap   = 0;
    vm->handle_next  = 1;        /* 0 = null */
    vm->gc_suspended = 0;        /* paso 6: GC reactivado (handle-aware: mark + barrido de tabla) */

    /* Pone los bytes sentinela en la región reservada:
     *   memory[0] = THREAD_EXIT (fin de Thread.run / hilo)
     *   memory[1] = NATIVE_RETURN (retorno del puente native→BP)
     *   memory[2] = EVENT_RETURN  (vuelta de un handler de evento, H5.c) */
    memory[0] = BPVM_SENTINEL_THREAD_EXIT;
    memory[BPVM_SENTINEL_NATIVE_RETURN_ADDR] = BPVM_SENTINEL_NATIVE_RETURN;
    memory[BPVM_SENTINEL_EVENT_RETURN_ADDR]  = BPVM_SENTINEL_EVENT_RETURN;
    bpvm_event_queue_init(vm);

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
    main_tc->sched_owner = -1;
    main_tc->ev_depth = 0;       /* H5.c: sin handler de evento en curso */
    main_tc->ev_post_mortem = -1; /* #342: vivo; la deuda se calcula al morir */
    vm->thread_count = 1;
    vm->current_thread_idx = 0;

    /* F4: el alocador de stacks de threads BP empieza tras la región
     * del main. Cada Thread.start() reserva BPVM_THREAD_STACK_BYTES. */
    vm->next_thread_stack = main_tc->stack_top;

    /* H3 #158 — apuntar a la tabla global de helpers para AOT. El
     * código AOT C-emitido la usa vía vm->aot_helpers->func(...). */
    vm->aot_helpers = &bpvm_aot_helpers_v2;

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

/* #310 — carga UN módulo desde una fuente de pack. Definida más abajo, junto
 * al resto de lo de packs; se declara aquí porque discover_deps la usa para el
 * pack en ejecución y bpvm_load_pack para el módulo principal: UNA función
 * para los dos, que es lo que garantiza que se carguen igual. */
static bpvm_status_t load_from_pack(bpvm_t* vm, const bpvm_pack_src_t* src,
                                    const bpvm_pack_entry_t* e, const char* name);

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
        /* Derivar filename + nombre de entrada en pack (mismo base-name).
         * `pname_file` = el nombre de fichero SIN directorio, que es lo que
         * come la regla de búsqueda común cuando search_dir no acierta. */
        char filename[512], pname[160], pname_file[168];
        if (lib[0]) {
            snprintf(filename, sizeof(filename), "%s%s%s.%s.mod",
                     search_dir, search_dir[0] ? "/" : "", lib, mod);
            snprintf(pname, sizeof(pname), "%s.%s", lib, mod);
        } else {
            snprintf(filename, sizeof(filename), "%s%s%s.mod",
                     search_dir, search_dir[0] ? "/" : "", mod);
            snprintf(pname, sizeof(pname), "%s", mod);
        }
        snprintf(pname_file, sizeof(pname_file), "%s.mod", pname);
        /* #310 — ORDEN DE BÚSQUEDA (spec §4, ADITIVO):
         *   0) el PACK EN EJECUCIÓN, si lo hay ← lo único nuevo
         *   1) el FS (que ECLIPSA a la zona de packs: shadow de desarrollo)
         *   2) la zona de packs montada, LO ÚLTIMO
         * Un pack que se ejecuta se lleva sus módulos dentro: buscarlos fuera
         * primero sería dejar que el entorno le cambie las tripas a una app
         * que viene cerrada. */
        if (vm->run_pack_on) {
            bpvm_pack_entry_t pe;
            if (bpvm_pack_find_src(&vm->run_pack_src, "mod", pname, &pe)) {
                int idx0 = vm->module_count;
                bpvm_status_t rs = load_from_pack(vm, &vm->run_pack_src, &pe, pname);
                if (rs != BPVM_OK) return rs;
                for (int j = idx0; j < vm->module_count; j++) {
                    bpvm_status_t r = discover_deps(vm, j, search_dir);
                    if (r != BPVM_OK) return r;
                }
                continue;                       /* resuelto dentro del pack */
            }
        }
        /* H3.c — resolución FS → packs (spec §4): el FS ECLIPSA al pack (shadow
         * de desarrollo, con aviso); si no está en FS, se carga DESDE el pack
         * montado (mismos bytes .mod; hoy con copia — el XIP es la tanda 2). */
        uint32_t pk_region_size = 0;
        const uint8_t* pk_region = bpvm_pack_mounted(&pk_region_size);
        uint32_t pk_len = 0;
        const uint8_t* pk_mod = pk_region
            ? bpvm_pack_find(pk_region, pk_region_size, "mod", pname, &pk_len)
            : NULL;

        /* #344 — POR LA FACHADA, no por fopen. Era lo único que ataba esta
         * función al host y por lo que las 3 familias tenían su propia copia
         * (más pobre). Además la búsqueda es ahora la ÚNICA: search_dir si lo
         * hay, y si no el basedir del proyecto → tal cual → /app → /lib. */
        char found[192];
        uint32_t fsz = 0;
        int have_fs = (bpvm_fs_stat(filename, &fsz) == 0);
        /* La precisión explícita no es cosmética: sin ella gcc avisa de posible
         * truncado en CADA build, y un build ruidoso es exactamente donde se
         * escondió la rotura de la Pico en #339. Truncar aquí es lo correcto —
         * `filename` ya pasó el stat, así que cabe salvo ruta absurda. */
        if (have_fs) snprintf(found, sizeof found, "%.*s",
                              (int)(sizeof found - 1), filename);
        else         have_fs = (bpvm_entry_resolve(pname_file, found, sizeof found, &fsz) == 0);

        if (!have_fs && !pk_mod) {
            bpvm_diag("[bpvm-c] dep '%s' (%s) no encontrado: %s",
                    imp, mod, filename);
            continue;   /* dejamos que linkAll dispare el error si falta. */
        }
        int idx_before = vm->module_count;
        bpvm_status_t s;
        if (have_fs) {
            if (pk_mod) {
                bpvm_diag("[bpvm-c] aviso: '%s' del FS eclipsa al del pack",
                        pname);
            }
            s = bpvm_load_entry_file(vm, found);
        } else {
            /* H3.c tanda 2 — carga XIP: el código se queda EN LA REGIÓN (flash
             * en placa); a RAM solo van ext-table + data block. */
            s = bpvm_loader_load_xip(vm, pk_mod, pk_len, pname);
            if (s == BPVM_OK) {
                bpvm_diag("[bpvm-c] '%s' cargado XIP desde pack (codigo en sitio)",
                        pname);
            }
        }
        if (s != BPVM_OK) return s;
        /* Recursivo: descubre las deps de esta nueva carga. */
        for (int j = idx_before; j < vm->module_count; j++) {
            bpvm_status_t r = discover_deps(vm, j, search_dir);
            if (r != BPVM_OK) return r;
        }
    }
    return BPVM_OK;
}

/* ── #310: ejecutar un pack ──────────────────────────────────────────────── */

/* Lectura por trozos de un pack que vive en el FS. La fuente pide offsets
 * dentro del PACK; aquí se traducen a offsets del fichero, que son los mismos. */
static long pack_fs_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    const bpvm_pack_fs_t* st = (const bpvm_pack_fs_t*) user;
    return bpvm_fs_read_at(st->path, off, dst, n);
}

int bpvm_pack_open_fs(bpvm_pack_src_t* src, bpvm_pack_fs_t* st, const char* path) {
    if (!src || !st || !path) return -1;
    size_t n = strlen(path);
    if (n + 1 > sizeof st->path) return -1;      /* ruta larga: mejor decirlo */
    memcpy(st->path, path, n + 1);
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0 || size == 0) return -1;
    bpvm_pack_src_stream(src, pack_fs_read_at, st, size);
    return 0;
}

/* Lectura de UNA entrada del pack: el loader pide offsets desde 0 (el .mod
 * empieza donde empieza), así que aquí se le suma dónde vive esa entrada. */
typedef struct { const bpvm_pack_src_t* src; uint32_t base; } pack_entry_rd_t;

static long pack_entry_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    pack_entry_rd_t* e = (pack_entry_rd_t*) user;
    return e->src->read_at(e->src->user, e->base + off, dst, n);
}

/* ── #310 paso 4: los RECURSOS del pack en ejecución también van primero ──
 * Los recursos no se resuelven por donde los módulos (readFile y compañía van
 * por bpvm_fs), así que se engancha ahí: un overlay que la fachada consulta
 * ANTES del backend. Un sitio, y todos los lectores de recursos lo heredan
 * sin tocar ni uno.
 *
 * Del path sólo importa el BASENAME, porque dentro del pack una entrada es
 * (tipo, nombre) = (extensión, nombre sin extensión) — así lo empaqueta
 * PackStep. `/app/x/main.win` → ("win", "main"). */
static int pack_res_entry(bpvm_t* vm, const char* path, bpvm_pack_entry_t* out) {
    if (!vm->run_pack_on || !path) return 0;
    /* Nunca reclamar el fichero del PROPIO pack: se lee por el FS de verdad y
     * reclamarlo aquí sería morderse la cola. */
    if (strcmp(path, vm->run_pack_st.path) == 0) return 0;
    const char* base = path;
    for (const char* p = path; *p; p++) if (*p == '/' || *p == '\\') base = p + 1;
    const char* dot = NULL;
    for (const char* p = base; *p; p++) if (*p == '.') dot = p;
    if (!dot || dot == base) return 0;                  /* sin extensión: no sé el tipo */
    char tipo[BPVM_PACK_TYPE_LEN + 1], nombre[BPVM_PACK_NAME_LEN + 1];
    size_t nlen = (size_t)(dot - base), tlen = strlen(dot + 1);
    if (nlen > BPVM_PACK_NAME_LEN || tlen > BPVM_PACK_TYPE_LEN) return 0;
    memcpy(nombre, base, nlen); nombre[nlen] = '\0';
    for (size_t i = 0; i < tlen; i++) {                 /* el tipo va en minúsculas */
        char c = dot[1 + i];
        tipo[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
    }
    tipo[tlen] = '\0';
    return bpvm_pack_find_src(&vm->run_pack_src, tipo, nombre, out);
}

static int pack_res_stat(void* user, const char* path, uint32_t* size) {
    bpvm_pack_entry_t e;
    if (!pack_res_entry((bpvm_t*) user, path, &e)) return -1;   /* no es mío */
    if (size) *size = e.len;
    return 0;
}

static long pack_res_read(void* user, const char* path, uint32_t off,
                          uint8_t* dst, uint32_t cap) {
    bpvm_t* vm = (bpvm_t*) user;
    bpvm_pack_entry_t e;
    if (!pack_res_entry(vm, path, &e)) return -1;
    if (off >= e.len) return 0;
    uint32_t n = e.len - off;
    if (n > cap) n = cap;
    const uint8_t* p = bpvm_pack_src_ptr(&vm->run_pack_src, e.data_off + off, n);
    if (p) { memcpy(dst, p, n); return (long) n; }       /* mapeado: en sitio */
    return vm->run_pack_src.read_at(vm->run_pack_src.user,
                                    e.data_off + off, dst, n);
}

/* La regla de con/sin código vive AQUÍ y en un solo sitio: no se mira de dónde
 * viene el pack, se le PREGUNTA a la fuente si da puntero. */
static bpvm_status_t load_from_pack(bpvm_t* vm, const bpvm_pack_src_t* src,
                                    const bpvm_pack_entry_t* e, const char* name) {
    const uint8_t* xip = bpvm_pack_src_ptr(src, e->data_off, e->len);
    if (xip) return bpvm_loader_load_xip(vm, xip, e->len, name);
    pack_entry_rd_t rd = { src, e->data_off };
    return bpvm_load_mod_stream(vm, pack_entry_read_at, &rd, e->len, name);
}

bpvm_status_t bpvm_load_pack(bpvm_t* vm, const char* pack_path,
                             char* main_out, int main_cap) {
    if (!vm || !pack_path) return BPVM_ERR_IO;
    /* La fuente vive en la VM, no en la pila: el pack sigue en ejecución
     * después de esta función y su ruta tiene que seguir viva. */
    bpvm_pack_src_t* src = &vm->run_pack_src;
    bpvm_pack_fs_t*  st  = &vm->run_pack_st;
    vm->run_pack_on = 0;
    if (bpvm_pack_open_fs(src, st, pack_path) != 0) {
        bpvm_diag("[bpvm-c] pack '%s' no se puede abrir", pack_path);
        return BPVM_ERR_IO;
    }
    /* Antes de hablar del manifest, comprobar que esto es un pack: si no,
     * el mensaje del manifest MIENTE (culpa al manifest de un fichero que ni
     * siquiera es un pack) y manda a buscar donde no es. */
    if (!bpvm_pack_src_is_pack(src)) {
        bpvm_diag("[bpvm-c] '%s' no es un pack (cabecera invalida)", pack_path);
        return BPVM_ERR_IO;
    }
    char mainmod[BPVM_PACK_NAME_LEN + 1];
    if (!bpvm_pack_manifest_get(src, "main", mainmod, (int) sizeof mainmod)) {
        /* Un pack SIN manifest es perfectamente válido — es una librería. Lo
         * que no es válido es pedirle que se ejecute, y hay que decirlo. */
        bpvm_diag("[bpvm-c] '%s' no es ejecutable: su manifest no declara 'main'",
                pack_path);
        return BPVM_ERR_IO;
    }
    bpvm_pack_entry_t e;
    if (!bpvm_pack_find_src(src, "mod", mainmod, &e)) {
        bpvm_diag("[bpvm-c] '%s' declara main='%s' pero no lleva ese modulo",
                pack_path, mainmod);
        return BPVM_ERR_IO;
    }
    int idx_before = vm->module_count;
    bpvm_status_t s = load_from_pack(vm, src, &e, mainmod);
    if (s != BPVM_OK) return s;
    /* A partir de aquí HAY pack en ejecución: sus imports se buscan DENTRO
     * antes que en ningún otro sitio. Se enciende DESPUÉS de cargar el main
     * para que un pack roto no deje la resolución tocada. */
    vm->run_pack_on = 1;
    /* Y los RECURSOS: el overlay hace que readFile y compañía miren dentro del
     * pack antes que en el FS, sin que ninguno de ellos se entere. */
    bpvm_fs_set_overlay(pack_res_stat, pack_res_read, vm);
    /* Y las dependencias, como en bpvm_load_mod: primero dentro del pack (lo
     * de arriba), y lo que no esté, en el directorio del propio pack. */
    char dir[256];
    path_dirname(pack_path, dir, sizeof dir);
    for (int j = idx_before; j < vm->module_count; j++) {
        bpvm_status_t r = discover_deps(vm, j, dir);
        if (r != BPVM_OK) return r;
    }
    if (main_out && main_cap > 0) {
        size_t n = strlen(mainmod);
        if (n + 1 > (size_t) main_cap) return BPVM_ERR_IO;
        memcpy(main_out, mainmod, n + 1);
    }
    return BPVM_OK;
}

bpvm_status_t bpvm_load_mod_buffer(bpvm_t* vm, const uint8_t* data,
                                    size_t size, const char* name_hint) {
    if (!vm || !data || size == 0) return BPVM_ERR_IO;
    /* En target embebido NO descubrimos deps. El caller las carga manualmente. */
    return bpvm_loader_load_buffer(vm, data, size, name_hint);
}

/* ==========================================================================
 * #344 — EL RUN, ESCRITO UNA VEZ. Ver include/bpvm_entry.h.
 *
 * Había CINCO cargas de "lo que se va a ejecutar", y no eran cinco variantes:
 * eran la misma regla copiada (las 15 líneas de v1_resolve_path están palabra
 * por palabra en Pico, ESP32 y STM32). Peor: las cuatro de los REPL eran
 * versiones POBRES de la del núcleo — cortaban el import por el primer punto
 * (así que un módulo con nombre compuesto no resolvía) y no sabían nada del
 * pack en ejecución, que es justo lo que impedía llevar #310 a la placa.
 *
 * Así que esto no escribe un sexto resolvedor: hace PORTABLE el bueno. Lo
 * único que ataba `discover_deps` al host era un fopen; con la fachada del FS
 * (bpvm_fs_stat/read_at, que ya existía) desaparece y las 5 familias corren
 * exactamente el mismo código.
 * ========================================================================== */

int bpvm_entry_resolve(const char* name, char* out, size_t out_cap,
                       uint32_t* size_out) {
    if (!name || !out || out_cap == 0) return -1;
    uint32_t dummy; if (!size_out) size_out = &dummy;
    /* H19 — el base-dir del proyecto PRIMERO (la carpeta del módulo principal):
     * un import resuelve contra /app/<proj>/ antes que contra nada más. Plano
     * (basedir="") o ruta absoluta → se salta este candidato. */
    const char* bd = bpvm_fs_basedir();
    if (bd && bd[0] && name[0] != '/') {
        snprintf(out, out_cap, "%s/%s", bd, name);
        if (bpvm_fs_stat(out, size_out) == 0) return 0;
    }
    snprintf(out, out_cap, "%s", name);
    if (bpvm_fs_stat(out, size_out) == 0) return 0;
    snprintf(out, out_cap, "/app/%s", name);
    if (bpvm_fs_stat(out, size_out) == 0) return 0;
    snprintf(out, out_cap, "/lib/%s", name);
    if (bpvm_fs_stat(out, size_out) == 0) return 0;
    out[0] = '\0';
    return -1;
}

/* Adaptador de la fachada al loader por trozos: el .mod se queda en el FS y
 * sólo se traen los pedazos que hacen falta. Igual en host y en placa. */
static long entry_fs_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    return bpvm_fs_read_at((const char*) user, off, dst, n);
}

bpvm_status_t bpvm_load_entry_file(bpvm_t* vm, const char* resolved_path) {
    uint32_t size = 0;
    if (bpvm_fs_stat(resolved_path, &size) != 0 || size == 0) return BPVM_ERR_IO;
    return bpvm_load_mod_stream(vm, entry_fs_read_at, (void*) resolved_path,
                                (size_t) size, resolved_path);
}

/* #345 — la primera línea de /sys/auto.txt, limpia. Ver bpvm_entry.h. */
int bpvm_autorun_entry(char* out, size_t out_cap) {
    if (!out || out_cap == 0) return 0;
    out[0] = '\0';
    /* Sólo la CABEZA del fichero: lo que se busca es un renglón, no un
     * fichero. En un micro traerse el fichero entero para esto sería el mismo
     * error que ya costó el espejo de 128 KB en #305. */
    uint8_t head[160];
    long got = bpvm_fs_read_at("/sys/auto.txt", 0, head,
                               (uint32_t) sizeof head);
    if (got <= 0) return 0;                       /* no hay autorun: normal */

    size_t size = (size_t) got, i = 0, n = 0;
    while (i < size && (head[i] == ' ' || head[i] == '\t')) i++;
    while (i < size && head[i] != '\n' && head[i] != '\r' && n + 1 < out_cap)
        out[n++] = (char) head[i++];
    while (n > 0 && (out[n - 1] == ' ' || out[n - 1] == '\t')) n--;
    out[n] = '\0';
    return n > 0;
}

/* #345 paso 2 — la ventana de rescate. Ver bpvm_entry.h para el porqué. */
int bpvm_autorun_gate(const bpvm_autorun_wire_t* w, const char* path,
                      int saludo_ms, int ventana_ms) {
    /* Sin cintura no hay a quién preguntar → se arranca, que es lo que hacía
     * antes de todo esto. Nunca se queda una placa sin arrancar por un fallo
     * de este mecanismo: el Auto es lo que el usuario pidió. */
    if (!w || !w->escucha || !w->ahora_ms) return 1;

    if (w->anuncia) w->anuncia(path, ventana_ms, w->user);

    /* Fase 1: ¿hay alguien? El IDE manda HELLO al conectar — ya lo hacía. Corta
     * a propósito: si está conectado, su HELLO ya llegó; si no contesta es que
     * no está, y entonces NO se paga ninguna espera (una placa suelta, sin
     * nadie delante, arranca a su ritmo como siempre). */
    uint32_t t0 = w->ahora_ms(w->user);
    int hay_alguien = 0;
    while ((uint32_t)(w->ahora_ms(w->user) - t0) < (uint32_t) saludo_ms) {
        int r = w->escucha(w->user);
        if (r == 2) return 0;               /* KILL de entrada: ni se arranca */
        if (r == 1) { hay_alguien = 1; break; }
        if (w->espera_ms) w->espera_ms(5, w->user);
    }
    if (!hay_alguien) return 1;

    /* Fase 2: la espera de verdad. Es del usuario, no de la placa: le da tiempo
     * a ver el aviso y darle a Stop, que manda el KILL de siempre (#257). */
    t0 = w->ahora_ms(w->user);
    while ((uint32_t)(w->ahora_ms(w->user) - t0) < (uint32_t) ventana_ms) {
        if (w->escucha(w->user) == 2) return 0;
        if (w->espera_ms) w->espera_ms(10, w->user);
    }
    return 1;
}

/* ¿Queda algún import sin dueño cargado? Devuelve 1 y deja el nombre en `out`.
 * Mejor decir "falta 'Gui'" que dejar que el link reviente doscientas líneas
 * más tarde con un símbolo que no le dice nada a nadie. */
static int first_missing(const bpvm_t* vm, char* out, size_t cap) {
    for (int mi = 0; mi < vm->module_count; mi++) {
        const bpvm_module_t* m = &vm->modules[mi];
        for (int k = 0; k < m->import_count; k++) {
            const char* imp = m->imports[k];
            if (!imp || !imp[0]) continue;
            char lib[64], mod[64];
            derive_owner(imp, lib, sizeof lib, mod, sizeof mod);
            if (!mod[0] || module_loaded(vm, lib, mod)) continue;
            /* Precisión explícita: el nombre es para un mensaje de error, así
             * que truncarlo es aceptable — pero hay que DECIRLO, o gcc avisa en
             * cada build y el aviso de verdad se pierde entre el ruido. */
            if (lib[0]) snprintf(out, cap, "%.*s.%.*s",
                                 (int)(cap / 2 - 1), lib, (int)(cap / 2 - 1), mod);
            else        snprintf(out, cap, "%.*s", (int)(cap - 1), mod);
            return 1;
        }
    }
    out[0] = '\0';
    return 0;
}

bpvm_status_t bpvm_load_entry(bpvm_t* vm, const char* path, bpvm_entry_t* e) {
    bpvm_entry_t local;
    if (!e) { memset(&local, 0, sizeof local); e = &local; }
    e->missing[0] = e->resolved[0] = e->main_module[0] = '\0';
    e->from_pack = 0;
    if (!vm || !path || !path[0]) return BPVM_ERR_IO;

    /* 1. La regla de búsqueda, una sola. */
    uint32_t size = 0;
    if (bpvm_entry_resolve(path, e->resolved, sizeof e->resolved, &size) != 0)
        return BPVM_ERR_IO;

    /* 2. EL `if` DE .mod/.pack, y vive AQUÍ. Antes sólo el CLI del host sabía
     *    despachar packs; por eso llevarlos a la placa era copiar la regla en
     *    cuatro sitios más. */
    size_t n = strlen(e->resolved);
    e->from_pack = (n > 5 && strcmp(e->resolved + n - 5, ".pack") == 0);

    bpvm_status_t s;
    int idx_before = vm->module_count;
    if (e->from_pack) {
        s = bpvm_load_pack(vm, e->resolved, e->main_module, (int) sizeof e->main_module);
        if (s != BPVM_OK) return s;
        /* bpvm_load_pack ya descubre las deps del main (dentro del pack primero,
         * y luego junto al propio pack); aquí sólo queda el barrido final. */
    } else {
        s = bpvm_load_entry_file(vm, e->resolved);
        if (s != BPVM_OK) return s;
        if (vm->module_count > idx_before)
            /* Precisión explícita otra vez: main_module tiene el ancho de un
             * nombre de pack (33) y un nombre de módulo puede ser mayor. Lo
             * cazó el gcc de ARM, no el del host — otra razón para compilar la
             * placa en cada tanda, no sólo el host. */
            snprintf(e->main_module, sizeof e->main_module, "%.*s",
                     (int)(sizeof e->main_module - 1), vm->modules[idx_before].name);
        /* El directorio del PROPIO módulo es el primer sitio donde buscar sus
         * dependencias — un proyecto se lleva sus .mod al lado. (Se me olvidó
         * en el primer intento y la batería lo cazó a la primera: seis targets
         * con "falta la lib 'X'".) */
        char dir[256];
        path_dirname(e->resolved, dir, sizeof dir);
        for (int j = idx_before; j < vm->module_count; j++) {
            s = discover_deps(vm, j, dir);
            if (s != BPVM_OK) return s;
        }
    }

    if (e->on_module) {
        for (int j = idx_before; j < vm->module_count; j++)
            e->on_module(vm->modules[j].name, e->from_pack ? "pack" : "fs", 0, e->user);
    }

    /* 3. El guardián: si algo se quedó sin dueño, se NOMBRA. */
    if (first_missing(vm, e->missing, sizeof e->missing)) return BPVM_ERR_IO;
    return BPVM_OK;
}

uint8_t* bpvm_arena_reserve(bpvm_t* vm, uint32_t n, uint32_t align) {
    if (!vm || n == 0) return NULL;
    uint32_t base = vm->next_free_address;
    if (align > 1) base = (base + (align - 1)) & ~(align - 1);
    if (base + n > vm->stack_base) return NULL;      /* no cabe: que lo sepa el caller */
    vm->next_free_address = base + n;
    /* El heap empieza tras lo último reservado — MISMO invariante (y mismos
     * campos) que fija el loader al terminar de cargar un módulo, incluido el
     * umbral del GC, que se recalcula porque la arena disponible ha encogido. */
    vm->heap_start        = vm->next_free_address;
    vm->heap_next         = vm->heap_start;
    vm->free_list_head    = 0;
    vm->last_gc_heap_next = vm->heap_next;
    vm->gc_bump_threshold = (vm->stack_base - vm->heap_start) / 8;
    if (vm->gc_bump_threshold < 4096) vm->gc_bump_threshold = 4096;
    return vm->memory + base;
}

bpvm_status_t bpvm_load_mod_stream(bpvm_t* vm, bpvm_read_at_fn rd, void* user,
                                    size_t size, const char* name_hint) {
    if (!vm || !rd || size == 0) return BPVM_ERR_IO;
    /* Igual que load_mod_buffer: en target embebido NO descubrimos deps, las
     * carga el caller. */
    return bpvm_loader_load_stream(vm, rd, user, size, name_hint);
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
    vm->kill_requested = 0;   /* P-run-stop: los re-runs no nacen muertos */
    /* F3 — resolver imports y aplicar class fixups antes de ejecutar. */
    bpvm_status_t ls = bpvm_link_all(vm);
    if (ls != BPVM_OK) return ls;

    /* Inicializar thread main desde el entry-point antes de entrar al
     * scheduler. */
    if (vm->main_absolute_address == 0) return BPVM_ERR_BAD_PC;
    bpvm_thread_t* main_tc = &vm->threads[0];
    main_tc->pc = vm->main_absolute_address;
    /* cs del módulo del entry-point (H3.c: por rango de CÓDIGO [cb, cb+size)). */
    {
        const bpvm_module_t* m = bpvm_module_for_code_addr(vm, vm->main_absolute_address);
        if (m) main_tc->cs = m->code_start;
    }
    if (main_tc->cs == 0) return BPVM_ERR_BAD_PC;
    main_tc->sp = main_tc->stack_base;
    main_tc->bp = main_tc->stack_base;
    main_tc->status = BPVM_THREAD_RUNNABLE;

    /* Default quantum si no se ajustó. */
    if (vm->quantum_ops == 0) vm->quantum_ops = 1024;

    bpvm_status_t rs = bpvm_scheduler_run(vm);
    /* #310 (Eduardo) — al TERMINAR se vuelve al camino estándar: el del pack
     * desaparece. Si no, en un daemon (que es como lo usa el IDE) el Run
     * siguiente heredaría el pack del anterior y resolvería imports contra un
     * pack que ya no se está ejecutando: la peor clase de fallo, porque
     * funciona hasta que alguien encadena dos ejecuciones. */
    vm->run_pack_on = 0;
    bpvm_fs_set_overlay(NULL, NULL, NULL);   /* y los recursos, al camino normal */
    return rs;
}

/* H2 — variante SMP. Misma puesta a punto del main tc + n workers. */
#include "bpvm_smp.h"
bpvm_status_t bpvm_run_smp(bpvm_t* vm, int n_workers) {
    if (!vm) return BPVM_ERR_BAD_PC;
    if (n_workers < 1) n_workers = 1;
    vm->kill_requested = 0;   /* P-run-stop: los re-runs no nacen muertos */
    bpvm_status_t ls = bpvm_link_all(vm);
    if (ls != BPVM_OK) return ls;
    if (vm->main_absolute_address == 0) return BPVM_ERR_BAD_PC;
    bpvm_thread_t* main_tc = &vm->threads[0];
    main_tc->pc = vm->main_absolute_address;
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
    if (vm->quantum_ops == 0) vm->quantum_ops = 1024;

    if (bpvm_smp_init(vm, n_workers) != 0) return BPVM_ERR_OOM;
    int rc = bpvm_scheduler_run_smp(vm);
    bpvm_smp_destroy(vm);
    vm->run_pack_on = 0;   /* #310 — mismo cierre que bpvm_run: fuera el camino del pack */
    bpvm_fs_set_overlay(NULL, NULL, NULL);
    if (vm->kill_requested) return BPVM_KILLED;   /* P-run-stop */
    return rc == 0 ? BPVM_OK : BPVM_ERR_RUNTIME;
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

void bpvm_set_debug_hook(bpvm_t* vm,
                          bpvm_debug_hook_t hook,
                          bpvm_pc_to_line_t pc_to_line,
                          void* user) {
    if (!vm) return;
    /* Setear todos juntos. El inner loop lee debug_hook primero — si
     * es NULL no toca los otros dos campos. */
    vm->debug_hook        = hook;
    vm->debug_pc_to_line  = pc_to_line;
    vm->debug_user        = user;
}

int bpvm_thread_id(const bpvm_thread_t* tc) {
    return tc ? (int) tc->id : -1;
}

/* ============================================================ */
/*  H6.b — Debugger del device: API de breakpoints + pausa.     */
/* ============================================================ */

/* P-run-stop (#257) — KILL cooperativo. */
void bpvm_set_poll(bpvm_t* vm, bpvm_poll_cb_t cb, void* user) {
    if (!vm) return;
    vm->poll_cb   = cb;
    vm->poll_user = user;
}

void bpvm_request_kill(bpvm_t* vm) {
    if (!vm) return;
    vm->kill_requested = 1;
}

int bpvm_kill_requested(const bpvm_t* vm) {
    return vm ? vm->kill_requested : 0;
}

void bpvm_set_pause_cb(bpvm_t* vm, bpvm_pause_cb_t cb, void* user) {
    if (!vm) return;
    vm->pause_cb   = cb;
    vm->pause_user = user;
}

int bpvm_debug_add_breakpoint(bpvm_t* vm, uint32_t pc) {
    if (!vm) return -1;
    /* Idempotente por pc: si ya existe, devuelve su id. */
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS; i++)
        if (vm->breakpoints[i].id != 0 && vm->breakpoints[i].pc == pc)
            return vm->breakpoints[i].id;
    /* Buscar slot libre (id==0). */
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS; i++) {
        if (vm->breakpoints[i].id == 0) {
            int id = ++vm->bp_next_id;
            vm->breakpoints[i].pc = pc;
            vm->breakpoints[i].id = id;
            vm->bp_active++;
            return id;
        }
    }
    return -1;   /* tabla llena */
}

bool bpvm_debug_clear_breakpoint(bpvm_t* vm, int bp_id) {
    if (!vm || bp_id <= 0) return false;
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS; i++) {
        if (vm->breakpoints[i].id == bp_id) {
            vm->breakpoints[i].id = 0;
            vm->breakpoints[i].pc = 0;
            vm->bp_active--;
            return true;
        }
    }
    return false;
}

void bpvm_debug_clear_breakpoints(bpvm_t* vm) {
    if (!vm) return;
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS; i++) {
        vm->breakpoints[i].id = 0;
        vm->breakpoints[i].pc = 0;
    }
    vm->bp_active = 0;
}

int bpvm_debug_list_breakpoints(bpvm_t* vm, uint32_t* out_pcs, int* out_ids, int max) {
    if (!vm) return 0;
    int n = 0;
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS && n < max; i++) {
        if (vm->breakpoints[i].id != 0) {
            if (out_pcs) out_pcs[n] = vm->breakpoints[i].pc;
            if (out_ids) out_ids[n] = vm->breakpoints[i].id;
            n++;
        }
    }
    return n;
}

void bpvm_debug_request_pause(bpvm_t* vm) {
    if (!vm) return;
    vm->pause_requested = 1;
}

uint32_t bpvm_thread_pc(const bpvm_thread_t* tc) { return tc ? tc->pc : 0; }
uint32_t bpvm_thread_sp(const bpvm_thread_t* tc) { return tc ? tc->sp : 0; }
uint32_t bpvm_thread_bp(const bpvm_thread_t* tc) { return tc ? tc->bp : 0; }
uint32_t bpvm_thread_cs(const bpvm_thread_t* tc) { return tc ? tc->cs : 0; }

void bpvm_destroy(bpvm_t* vm) {
    if (!vm) return;
    /* #339 — a una local: la estructura del vm se libera aquí abajo y el barrido
     * va DESPUÉS, cuando ya no queda nada del programa a lo que preguntar. */
    uint64_t run_mark = vm->run_mark;

    bpvm_free(vm->handle_addr);   /* V4: tabla de handles */
    bpvm_free(vm->handle_gen);    /* V4/paso 3: generación */
    bpvm_free(vm->handle_free_list);   /* V4/paso 4c: free-list */
    /* Liberar módulos cargados. */
    for (int i = 0; i < vm->module_count; i++) {
        bpvm_module_t* m = &vm->modules[i];
        if (m->imports) {
            for (int k = 0; k < m->import_count; k++) bpvm_free(m->imports[k]);
            bpvm_free(m->imports);
        }
        bpvm_free(m->class_fixups);
        bpvm_free(m->eh_class_fixups);
    }
    /* Liberar EH stacks y mutex waiters. */
    for (int i = 0; i < vm->thread_count; i++) {
        bpvm_free(vm->threads[i].eh_stack);
    }
    for (int i = 0; i < vm->mutex_count; i++) {
        bpvm_free(vm->mutexes[i].waiters);
    }
    bpvm_free(vm->mutexes);
    bpvm_free(vm->symbols);
    bpvm_free(vm->scratch);
    bpvm_free(vm->gc_valid_map);
    /* #352 — el modelo del GUI es de la VM: se va con ella. Antes vivía en
     * globales de fichero y sobrevivía al programa (ver bpvm_gui_reset). */
#ifdef BPVM_GUI
    bpvm_gui_reset();
#endif
    bpvm_free(vm);

    /* Y ahora el juicio: lo que quede vivo con secuencia >= la marca es memoria
     * del programa que nadie limpió. Habla siempre (control). */
    bpvm_alloc_sweep(run_mark);
}

const char* bpvm_status_str(bpvm_status_t s) {
    switch (s) {
    case BPVM_OK:                 return "OK";
    case BPVM_ERR_IO:             return "IO error";
    case BPVM_ERR_BAD_MAGIC:      return "MAGIC inválido (no es un .mod; se esperaba \"MOD6\")";
    case BPVM_ERR_ABI_MOD_V5:     return ".mod v5: es anterior al ensanchado de refs 4->8B, "
                                         "su ABI no se puede garantizar (si es de la era 4B "
                                         "corrompería memoria). Recompílalo con el compilador actual";
    case BPVM_ERR_BAD_HEADER:     return "header inconsistente";
    case BPVM_ERR_OOM:            return "memoria del buffer insuficiente";
    case BPVM_ERR_BAD_OPCODE:     return "opcode desconocido o no soportado";
    case BPVM_ERR_BAD_PC:         return "PC fuera de rango";
    case BPVM_ERR_STACK_OVERFLOW: return "stack overflow";
    case BPVM_ERR_DIV_BY_ZERO:    return "división por cero";
    case BPVM_ERR_NULL_RECEIVER:  return "INVOKE_VIRTUAL sobre null";
    case BPVM_ERR_RUNTIME:        return "RuntimeError BP no atrapado";
    case BPVM_NATIVE_RETURN:      return "native-return (interno)";
    case BPVM_DBG_STOPPED:        return "detenido por el debugger";
    case BPVM_KILLED:             return "terminado por KILL";
    default:                       return "?";
    }
}
