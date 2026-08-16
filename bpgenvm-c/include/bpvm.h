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
#include "bpvm_pack.h" /* #310 — ejecutar un pack. La VM sabe de packs; el FS no. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bpvm bpvm_t;

typedef enum {
    BPVM_OK = 0,
    BPVM_ERR_IO,             /* no se pudo abrir/leer el fichero */
    BPVM_ERR_BAD_MAGIC,      /* MAGIC del .mod no coincide */
    BPVM_ERR_ABI_MOD_V5,     /* #284 — .mod v5: ABI anterior al ensanchado de
                              * refs 4->8B; no se puede garantizar y NO se carga
                              * (recompilar). El gate que hace GRITAR al blob
                              * rancio en vez de dejarlo corromper en silencio. */
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
    BPVM_DBG_STOPPED,        /* H6.b — el debugger abortó la ejecución
                              * (pause_cb devolvió BPVM_DBG_STOP). */
    BPVM_KILLED              /* P-run-stop (#257) — ejecución abortada por
                              * bpvm_request_kill() / poll_cb (KILL del wire).
                              * Parada limpia ENTRE opcodes: heap y FS quedan
                              * consistentes; la VM puede recargar y re-correr. */
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

/* Reparto heap/stacks del bloque de la VM. LA REGLA, EN UN SOLO SITIO: hasta hoy
 * estaba COPIADA en el Pico y en el ESP32 (con valores ya distintos entre sí) y
 * AUSENTE en el STM32, que se apoyaba en el default de bpvm_init. Tres estados
 * para una sola decisión — justo el patrón que lleva toda la semana costando
 * caro. Devuelve los bytes para stacks; el heap es el resto.
 *
 *   25% para stacks (Eduardo 28-jul)... pero acotado por los dos extremos:
 *   - SUELO 64 KB: por debajo, el stack principal (16 KB) + 2 KB por thread
 *     dejan tan pocos threads que es una regresión. En placas de 128 KB el 25%
 *     serían 32 KB = 8 threads, cuando hoy caben 24.
 *   - TECHO 512 KB (Eduardo 19-jul): con 8 MB de PSRAM un reparto proporcional
 *     dormiría megas en stacks que usan decenas de KB.
 *
 * Cómo queda: Pico 343 KB -> 85 KB · Metro 8 MB -> 512 KB (techo) ·
 * S3/STM32 128 KB -> 64 KB (suelo; IDÉNTICO a hoy, sin regresión). */
size_t bpvm_stack_region_bytes(size_t total_bytes);

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
 *   1. Lee y valida el header (MAGIC "MOD5" v5 o "MOD6" v6; v6 añade la
 *      sección `interface` entre exports y data, que el loader SALTA).
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
 * H11 — Carga un .mod POR TROZOS, sin tenerlo entero en RAM.
 *
 * `rd` lee `n` bytes del .mod a partir de `off` y devuelve cuántos leyó (o -1).
 * El loader sólo avanza hacia delante, así que le vale cualquier fuente: un
 * fichero del FS, un socket con seek, una región de flash.
 *
 * Lo que gana: los dos bloques gordos —data y code— se leen DIRECTAMENTE en su
 * sitio final dentro de memory[], y la única sección que necesita estar
 * residente (exports, la que el parser mira por offsets) se monta transitoria
 * en la arena libre por encima del módulo. En un micro eso es la diferencia
 * entre reservar un buffer del tamaño del fichero más grande imaginable y no
 * reservar nada.
 *
 * `size` es el tamaño total del .mod (el caller lo sabe: se lo dice el stat).
 */
typedef long (*bpvm_read_at_fn)(void* user, uint32_t off, uint8_t* dst, uint32_t n);
bpvm_status_t bpvm_load_mod_stream(bpvm_t* vm, bpvm_read_at_fn rd, void* user,
                                    size_t size, const char* name_hint);

/* ── #310: EJECUTAR UN PACK ──────────────────────────────────────────────────
 * Un pack ejecutable trae un manifest (`main=<módulo>`) que dice cuál de sus
 * módulos es el principal. El pack que se ejecuta vive como FICHERO en /app
 * del FS: no se carga a RAM (no cabe), se lee por trozos.
 *
 * Estado de la fuente: guarda la ruta, porque el callback de lectura la
 * necesita en cada trozo. Tiene que seguir VIVO mientras se use la fuente. */
#define BPVM_PACK_FS_PATH_MAX 128
typedef struct { char path[BPVM_PACK_FS_PATH_MAX]; } bpvm_pack_fs_t;

/* Abre un pack del FS como fuente de lectura. 0 = OK; -1 = no está, está
 * vacío, o la ruta no cabe. NO da puntero directo ⇒ sus módulos se cargarán
 * con código (ver bpvm_pack_src_ptr). */
int bpvm_pack_open_fs(bpvm_pack_src_t* src, bpvm_pack_fs_t* st, const char* path);

/* Carga el módulo PRINCIPAL del pack `pack_path` (el que declara su manifest).
 * Deja su nombre en `main_out` para que el caller pueda arrancarlo. NO resuelve
 * dependencias: eso es del caller (y del orden de búsqueda, que con un pack en
 * ejecución antepone el propio pack). */
bpvm_status_t bpvm_load_pack(bpvm_t* vm, const char* pack_path,
                             char* main_out, int main_cap);

/*
 * H11 — Reserva `n` bytes CONTIGUOS y PERMANENTES de la arena: por encima de
 * los módulos ya cargados y por debajo del heap, que se desplaza para dejarles
 * sitio. Para blobs que la VM mapea o EJECUTA EN SITIO y tienen que seguir
 * vivos toda la ejecución — hoy el overlay `.mdn`, cuyos thunks se registran
 * como punteros al propio buffer (zero-copy).
 *
 * Sustituye al patrón de "un scratch estático del tamaño del fichero más
 * grande imaginable": aquí se pide lo que ocupa de verdad, y si no cabe se
 * entera el caller en vez de corromper.
 *
 * DEBE llamarse ANTES de la primera alocación del heap (es decir, antes de
 * bpvm_run). Devuelve NULL si no cabe. `align` en bytes (0 o 1 = sin alinear).
 */
uint8_t* bpvm_arena_reserve(bpvm_t* vm, uint32_t n, uint32_t align);

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

/* #355 — enciende/apaga el RECOLECTOR de basura de esta VM. Apagado, no corre
 * por ninguna de las tres puertas: ni el umbral proactivo, ni el intento tras
 * una reserva fallida, ni el gc() manual del programa.
 *
 * Es un INSTRUMENTO DE DIAGNÓSTICO, no una opción de uso normal: sin recolector
 * la memoria es de un solo uso y cualquier programa que no quepa entero en el
 * heap acabará dando OOM. Existe porque separa en dos un experimento que de
 * otro modo no se puede partir — mismo micro, misma memoria, mismo programa,
 * con y sin GC — y esa diferencia es lo que distingue "se agota la memoria" de
 * "el recolector se lleva algo vivo". Encendido por defecto; apagarlo tiene que
 * ser deliberado (`--nogc` en el PC, `gc=0` en el ENV de la placa). */
void bpvm_set_gc_enabled(bpvm_t* vm, int enabled);

/* #430 — tope de la tabla de handles (slots; 0 = sin tope). El default lo pone
 * el build del puerto (BPVM_HANDLE_CAP_MAX); esto lo cambia en runtime — su
 * cliente real es el host de pruebas (`--handlecap`), que asi reproduce el
 * limite de una placa sin build especial. */
void bpvm_set_handle_cap_max(bpvm_t* vm, uint32_t slots);

/* ── #353: dónde habla la VM cuando NO habla el programa ─────────────────────
 *
 * bpvm_set_output es la salida DEL PROGRAMA (print). Esto es otra cosa: lo que
 * dice la VM al operador — "falta la dep 'Gui'", "'X' del FS eclipsa al del
 * pack", el veredicto del guardián de #339. Hasta aquí iba a `stderr` a pelo, y
 * eso en la Pico es un problema MEDIDO, no teórico:
 *
 *   - el `_write` del pico-sdk manda stdout Y stderr al MISMO USB CDC, que es
 *     por donde va el wire (newlib_interface.c:125);
 *   - `wire_v1_send_line` escribe bajo el mutex TX, pero un fprintf(stderr) del
 *     núcleo NO lo coge → puede partir un frame por la mitad.
 *
 * En el STM32 el efecto es el contrario y también malo: no hay `_write`
 * retargeteado, así que el diagnóstico se PIERDE. Y en el ESP32 el wire es una
 * UART aparte, así que ahí no molesta. Tres familias, tres respuestas: justo el
 * caso de "el núcleo dice QUÉ, la familia decide DÓNDE".
 *
 * Sin sink instalado el comportamiento es el de siempre (stderr + fflush), que
 * es lo que quieren el host y el micro simulado. Los firmwares instalan uno de
 * una línea que va al log persistente — y de paso el diagnóstico sobrevive al
 * reset, que en una placa vale más que verlo pasar. */
typedef void (*bpvm_diag_fn)(const char* linea);
void bpvm_diag_set_sink(bpvm_diag_fn fn);   /* NULL = vuelve a stderr */
void bpvm_diag(const char* fmt, ...);

/* #355 — CANAL URGENTE: el aviso llega a su soporte definitivo ANTES de seguir.
 *
 * En la Pico el log se acumula en RAM y sólo baja a flash en `log_flush()`. Eso
 * significa que si la VM se cuelga, se pierde TODO lo anotado desde el último
 * volcado — o sea que el instrumento calla exactamente en el caso que vinimos a
 * investigar, y de una forma que se lee como "no pasó nada" en vez de "no llegué
 * a contarlo". Medido por Eduardo: cuando la placa no se cuelga el log se graba;
 * cuando se cuelga, no.
 *
 * Reservado para lo que puede ir SEGUIDO DE UNA MUERTE: sin memoria, excepción
 * sin handler, bloque descarrilado. Volcar cada línea costaría un borrado de
 * flash por aviso; volcar sólo éstas cuesta nada porque son raras. El sink de
 * cada familia registra su volcado con bpvm_diag_set_flush (sin él, no-op). */
typedef void (*bpvm_diag_flush_fn)(void);
void bpvm_diag_set_flush(bpvm_diag_flush_fn fn);
void bpvm_diag_urgente(const char* fmt, ...);

/* ── #338: LA ZONA DE RASCAR COMPARTIDA ──────────────────────────────────────
 *
 * Hay operaciones que necesitan un buffer grande y lo necesitan UN MOMENTO:
 * borrar un pack (página de RMW), listar un directorio, escanear la zona de
 * packs. Cada una tenía el suyo `static`, y todos juntos se comían decenas de
 * KB de `.bss` PERMANENTES para trabajar unos milisegundos al día.
 *
 * Pero son mutuamente excluyentes: el wire es petición/respuesta en una sola
 * comm task — no listas packs mientras borras uno. Así que comparten UNA zona.
 * No es una idea nueva: `board_mgr_pico.c` ya tomaba prestado el buffer del PUT
 * exactamente así. Esto es ese préstamo, hecho explícito y vigilado.
 *
 * Se prefirió esto a mandarlos al heap (idea original de #338) por dos motivos:
 * en la Pico `malloc` va a un `ucHeap` de 32 KB, así que mover 8 KB de un sitio
 * a otro gana poco; y añadiría un camino de fallo —reserva fallida— justo en
 * operaciones que hoy no pueden fallar. Aquí no hay reserva que falle.
 *
 * EL GUARDIÁN ES LA MITAD DEL VALOR: si dos operaciones se solapan, la segunda
 * NO recibe la zona y se dice por el canal de diagnóstico con los dos nombres.
 * El día que alguien haga concurrente algo que hoy no lo es, sale a gritos en
 * vez de corromper el trabajo del otro en silencio. */
/* #338 (2-ago) — desde que el gestor de placa saca de aquí las DOS copias del env,
 * el suelo de la zona ya no es la página de RMW de packs sino 2*BP_ENV_SECTOR, y
 * ése es el sector de BORRADO de la flash: 4 KB en RP2350/ESP32, pero 8 KB en el
 * STM32U5. Por eso el valor por defecto mira la familia — no es lógica propietaria,
 * es un TAMAÑO que depende del silicio, y el macro lo pone ya el build del
 * fabricante. Quien quiera otro número lo define y este bloque se aparta.
 *
 * Si mañana entra una familia con sector mayor y nadie toca esto, NO se rompe en
 * placa: cada cintura lleva una comprobación EN COMPILACIÓN de que la zona da para
 * sus dos copias (bp_chk_scratch_env), así que no enlaza. */
#ifndef BPVM_SCRATCH_BYTES
#  if defined(STM32U575xx) || defined(STM32U5G9xx)
#    define BPVM_SCRATCH_BYTES 16384   /* 2 x 8 KB: el sector de borrado del U5 */
#  else
#    define BPVM_SCRATCH_BYTES 8192    /* 2 x 4 KB (RP2350, ESP32) y la RMW de packs */
#  endif
#endif

/* Devuelve la zona (alineada para cualquier tipo) o NULL: no cabe, o ya la
 * tiene otro. `quien` es un literal corto para el aviso ("PACK_DEL"). */
void*  bpvm_scratch_take(size_t n, const char* quien);
void   bpvm_scratch_give(const char* quien);
size_t bpvm_scratch_capacity(void);

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

/* ============================================================ */
/*  P-run-stop (#257) — KILL cooperativo.                        */
/* ============================================================ */

/* Callback de polling: el scheduler lo invoca ENTRE quanta (nunca a mitad
 * de opcode) y, con tope de ~50 ms, también mientras todos los threads BP
 * duermen. Pensado para mirar el transporte del wire sin bloquear: si ve
 * un KILL, responde el ack y devuelve != 0 → la VM termina con
 * BPVM_KILLED. Devolver 0 = seguir ejecutando. */
typedef int (*bpvm_poll_cb_t)(bpvm_t* vm, void* user);

/* Instala/desinstala (NULL) el poll-callback. THREAD-UNSAFE: llamar antes
 * de bpvm_run(). Sin callback el scheduler solo paga un null-check. */
void bpvm_set_poll(bpvm_t* vm, bpvm_poll_cb_t cb, void* user);

/* Pide terminar la ejecución (desde el propio poll_cb o desde OTRA task,
 * p. ej. la de RX de un transporte TCP). bpvm_run/bpvm_run_smp devuelven
 * en cuanto los workers cruzan el siguiente safepoint. El flag se limpia
 * al entrar en bpvm_run* (los re-runs no nacen muertos). */
void bpvm_request_kill(bpvm_t* vm);

/* True si la última ejecución terminó por KILL (útil cuando el status se
 * pierde por el camino, p. ej. bpvm_run_smp). */
int  bpvm_kill_requested(const bpvm_t* vm);

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

/*
 * Paso 4 (V3) — detalle legible del último fallo de LINK (lib/símbolo
 * cross-module no resuelto): p.ej. "falta la lib 'Json' (la usa 'Gui'; ...)".
 * "" si no hubo fallo de link. Válido tras bpvm_run/bpvm_run_smp. Los handlers
 * de RUN lo mandan al wire en vez del exit-code mudo (antes el detalle solo iba
 * a stderr). NO es propietario del puntero: válido mientras viva la VM.
 */
const char* bpvm_link_error(const bpvm_t* vm);

/* #421 — por qué falló la CARGA de un módulo (ruta + motivo), "" si no hubo
 * fallo. Hermano de bpvm_link_error: el REPL lo manda por el wire en vez del
 * «IO error» mudo. Lo rellena el cargador, tanto para el módulo principal como
 * para sus dependencias — que es donde pasa de verdad. */
const char* bpvm_load_error(const bpvm_t* vm);

/* Detalle del último RuntimeError lanzado (msg de bpvm_throw_runtime_error),
 * p.ej. "referencia a objeto eliminado (use-after-free)". "" si no hubo. Los
 * handlers de RUN lo surten al wire/host cuando el status es BPVM_ERR_RUNTIME,
 * en vez del "exit N" genérico. NO propietario: válido mientras viva la VM. */
const char* bpvm_runtime_error(const bpvm_t* vm);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_H */
