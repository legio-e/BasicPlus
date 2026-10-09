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

/* V4/paso4: HANDLE de 64b = [gen:32 | idx|TAG:32]. bpref_t transporta los 64b;
 * bpref_deref/bpvm_ref_dead enmascaran la palabra baja para idx/tag y la alta
 * para gen. El TIPO se declara aquí arriba, y no en el bloque de la abstracción
 * de refs (más abajo), sólo porque hay structs anteriores que guardan una ref
 * como campo y necesitan el tipo completo. Todo lo demás —el ancho del slot,
 * los accesores, el deref— sigue viviendo en aquel bloque y sólo allí. */
typedef struct { uint64_t v; } bpref_t;
#include "bpvm_handles.h"   /* V6/U5 — la tabla de handles: su tipo, sus constantes y su API */

/* ============================================================ */
/*  Constantes extraídas de docs/MOD_FORMAT.md / HEAP_LAYOUT.md  */
/* ============================================================ */

#define BPVM_MAGIC          0x4D4F4435u   /* "MOD5" big-endian (v5, sin interfaz) */
#define BPVM_MAGIC_V6       0x4D4F4436u   /* "MOD6" big-endian (v6, H6.a: sección interface) */
/* [V6/N1.4] v7 — anade la seccion `native` (el `.mdn` embebido). NO cambia el
 * ABI —las refs siguen siendo de 8 bytes—, asi que v6 se sigue ejecutando y NO
 * hay que regenerar los .mod que ya existen. Mismo movimiento que hizo v6 con
 * el `.bpi` sobre v5. */
#define BPVM_MAGIC_V7       0x4D4F4437u   /* "MOD7" big-endian */
/* V7/H2 (#500) — v8: el módulo lleva contra qué se compiló: su generación `gen`
 * (10º entero) y el CRC32 de su sección interface (11º), y en la sección imports
 * una subtabla `deps` con el (gen, ifaceCrc) de cada dependencia. Aditivo: v6 y
 * v7 se siguen cargando (sin datos de versión: gen 0, CRC desconocido). */
#define BPVM_MAGIC_V8       0x4D4F4438u   /* "MOD8" big-endian */
/* V7/H2 (#523) — MOD9: ENLACE TARDIO. Misma cabecera que MOD8; lo nuevo son la
 * subtabla `refs` (3.3) en imports y la subtabla CLASES (4.5) en exports. Sin
 * marcadores Cls#clave#slot ni fixups 4.3/4.4: las clases ABIERTAS las construye
 * el enlazador contra el padre real, y las referencias se resuelven a la ext-table. */
#define BPVM_MAGIC_V9       0x4D4F4439u   /* "MOD9" big-endian */
#define BPVM_HEADER_SIZE    28            /* header v5; v6 = 32 (añade interfaceSize) */
#define BPVM_HEADER_SIZE_V6 32
#define BPVM_HEADER_SIZE_V7 36            /* añade nativeSize */
#define BPVM_HEADER_SIZE_V8 44            /* añade gen + ifaceCrc */
#define BPVM_FORMAT_VERSION 8

/* Tamaño en bytes de una entrada de la ext-table (per-module). */
#define BPVM_EXT_ENTRY_SIZE 4

/* Stack del thread main; ver HEAP_LAYOUT.md §8 */
#define BPVM_MAIN_STACK_BYTES   (16 * 1024)
#define BPVM_THREAD_STACK_BYTES (2 * 1024)   /* el de un Thread que no pide otro (Thread(0)); como miVM */

/* V7/H7 #605 (29-sep) — la ZONA ROJA de la pila: lo que una función puede apilar después de su ENTER sin volver a
 * mirar el tope (los operandos de una expresión, los argumentos de una llamada y los 12 B que empuja el CALL) hasta
 * el ENTER siguiente. La misma en miVM (PILA_ZONA_ROJA): el desbordamiento salta en la misma instrucción en las dos. */
#define BPVM_PILA_ZONA_ROJA 256u

/* Dirección inicial reservada (sentinela THREAD_EXIT en memory[0]). */
#define BPVM_INITIAL_FREE_ADDR  0x0100

/* Tag bits del header de objeto en heap (HEAP_LAYOUT.md §2.1). */
#define BPVM_TAG_MARK_BIT   0x80000000u
#define BPVM_TAG_FREE_BIT   0x40000000u
#define BPVM_TAG_TYPE_MASK  0x3F000000u
#define BPVM_TAG_TYPE_SHIFT 24
/* V7/H7 #595 — dos marcas que sólo existen MIENTRAS se compacta (heap.c, compactar). ENHEBRADO, el bit de la
 * marca del GC —a 0 en todo bloque tras el barrido—: la cabecera de un bloque con handle guarda el ÚLTIMO índice
 * que lo apunta, y la entrada de ese índice, lo que había antes en la cabecera. CLAVADO, un bit de los
 * reservados: un bloque que no se mueve aunque tenga handle (el estado de un fichero abierto). Espejo de miVM. */
#define BPVM_TAG_ENHEBRADO  0x80000000u
#define BPVM_TAG_CLAVADO    0x00800000u
/* #656 (7-oct) — otra marca que sólo existe MIENTRAS se suelta un owner (heap.c, bpvm_heap_soltar_owned), en otro bit
 * de los reservados: un bloque ya muerto cuyos hijos owner están por soltar, que no cupo en la pila de trabajo y espera
 * al repaso. Fuera de ahí vale 0. Sólo la VM-C (en la VM-Java, la pila es de Java y no se llena). */
#define BPVM_TAG_SOLTANDO   0x00400000u

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
#define BPVM_CLS_OFF_MODULO       6   /* V7/H2 (#523): indice del modulo dueno + 1 (0 = MOD7/8: por rango) */
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

/* V7 #592 (9-oct) — LA MARCA DEL 2º TEMPORIZADOR (idea de Eduardo; «B»: en la región reservada, sin subir la ABI).
 * Un contador de 32 bits en el orden de la CPU —no es un valor de BP: lo sube la plataforma y lo lee la native, la misma
 * CPU—, alineado, que el reloj de la plataforma sube cada ~5 ms (bpvm_marca_tiempo_avanza) y el KILL también
 * (bpvm_kill_marcar). Una native lo lee en cada vuelta de un bucle que llama a algo —por un puntero guardado al entrar,
 * por `vm->memory`, el prefijo congelado— y si cambió, llama a punto_control: así cede a los ~5 ms aunque cada vuelta
 * sea lenta. Lejos del 0 a propósito: un campo de un objeto null se lee en los primeros bytes (builtins.c, el array
 * null que leía memory[4..]). ⚠️ CONTRATO con los .mdn ya generados, como el prefijo congelado: NO SE MUEVE. */
#define BPVM_MARCA_TIEMPO_ADDR          0x80u

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

/* Hilos BP: SIN tope desde la etapa 2 de H12 (4-oct; Eduardo: «¿por qué tiene que haber un límite?»). Lo pone la
 * memoria: la struct de cada hilo (malloc) y su región de pila. Ver bpvm_t.cola. */

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

/* V7/H2 (#523) — una referencia simbólica de la subtabla 3.3 (ver TablasMod9.java). */
#define BPVM_REF_CLASE   0
#define BPVM_REF_MET     1
#define BPVM_REF_CAMPO   2
#define BPVM_REF_EV_RECV 3
#define BPVM_REF_EV_DEST 4
#define BPVM_REF_SUBTIPO 5
#define BPVM_REF_SUPER   6   /* V10/J-3: `super.m()` → direccion absoluta del m EFECTIVO del padre (por la cadena) */
#define BPVM_DEP_RAIZ   (-1)
#define BPVM_DEP_LOCAL  (-2)
typedef struct {
    uint8_t  kind;
    int16_t  dep_idx;     /* >= 0 índice en deps; BPVM_DEP_LOCAL = clase de este módulo */
    char*    clase;       /* malloc-ed: `[lib.]Mod.Cls` o el simple si es local */
    char*    clave;       /* malloc-ed: la clave canónica ("" en CLASE) */
} bpvm_ref_t;

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

    /* V5/H4 — ¿este módulo salió de un PACK (carga XIP) o del FS?
     *
     * Criterio de Eduardo, y es de coherencia: *«lo que está en un pack busca
     * primero dentro del pack»*. Un pack se graba como un CONJUNTO —motor,
     * módulo y puente, una versión— y nada de fuera debería poder subvertirlo:
     * si un `.mdn` suelto y olvidado en el FS ganara, algo que siempre
     * funcionó dejaría de funcionar sin que nadie tocara el pack.
     *
     * Lo usa `bpvm_mdn_escanear` para decidir en qué orden busca el `.mdn` de
     * CADA módulo: el puente sigue a su módulo. Se guarda como bandera en vez
     * de deducirse de [xip_lo, xip_hi) porque esa unión cubre también los
     * huecos entre packs: deducirlo sería cierto hoy y frágil siempre. */
    uint8_t  en_pack;
    /* V7/H2 (MOD8, #500) — versión del formato (6/7/8) y, si es 8, la generación
     * y el CRC de la interfaz que dice su cabecera. */
    uint8_t  fmt;
    uint32_t gen;
    uint32_t iface_crc;

    /* F3 — imports cualificados (e.g. "L2Lib.Counter.__init"). Cada
     * entry k corresponde al slot k de la ext-table. */
    char**   imports;          /* malloc-ed; cada slot apunta a `nombres` (V7 #651) */
    int      import_count;     /* = ext_count si las leemos */
    /* V7/H2 (MOD8) — la subtabla `deps`: contra qué (gen, ifaceCrc) se compiló
     * cada dependencia, por nombre lógico cualificado (`Core`, `acme.LibHdr`). */
    char**    dep_names;       /* malloc-ed */
    uint32_t* dep_gen;
    uint32_t* dep_crc;
    uint32_t* dep_flags;       /* bit 0 = EXTIENDE (una clase de aquí tiene base allí) */
    uint8_t*  dep_avisado;     /* «sin datos de version» dicho ya (una vez por dep) */
    int       dep_count;

    /* L2 v3 — class fixups del módulo. */
    bpvm_class_fixup_t* class_fixups;
    int                 class_fixup_count;

    /* BUG-2 — eh-class fixups (catch cross-module). */
    bpvm_eh_class_fixup_t* eh_class_fixups;
    int                    eh_class_fixup_count;

    /* V7/H2 (#523, MOD9) — el enlace tardío. `ext_count31` = entradas de imports
     * (3.1): la ref i vive en ext[ext_count31 + i]. `refs` se libera tras enlazar,
     * como los imports. `clases` es la subtabla 4.5 tal como viene en el fichero:
     * en XIP apunta a la flash; en RAM/stream se copia detrás del módulo (la lee el
     * enlazador y, después, la reflexión de Forms). */
    uint32_t     ext_count31;
    bpvm_ref_t*  refs;
    int          ref_count;
    const uint8_t* clases;
    uint32_t     clases_len;
    uint8_t      enlazado_tarde;   /* compilado contra otra interfaz: el enlace decidió (diag) */
    /* V7 #651 (8-oct) — los NOMBRES del módulo (imports[], dep_names[] y la clase y la clave de refs[]) viven aquí,
     * seguidos, cada uno terminado en 0: UN bloque por módulo, no uno por nombre. Se suelta entero. */
    char*        nombres;
} bpvm_module_t;

/* Tabla global de símbolos exportados (F3).
 *
 * #449 — EL NOMBRE NO VIVE AQUI, SINO EN UN POOL. Antes esto era
 * `char name[128]`, o sea 132 bytes POR SIMBOLO. Medido el 29-ago sobre el caso
 * real (JsonDemo + Json + Core): 460 simbolos, longitud MEDIA 24 y MAXIMA 35.
 * Es decir, 59 KB de tabla para 12 KB de nombres — y como crecia duplicando, el
 * `realloc` de 256 a 512 entradas pedia los dos arrays a la vez: 33 + 66 = 99 KB
 * dentro de un margen de `malloc` de 64. En la Pico 2 eso desbordaba hacia el
 * bloque de la VM y PISABA EL CODIGO de un modulo (#440).
 *
 * Con el pool son 8 B por entrada mas el nombre exacto: los mismos 460 simbolos
 * pasan de 59 KB a ~16.
 *
 * #639 (3-oct) — y el pool, en TROZOS de 2 KB que no se mueven nunca (Eduardo): era UN bloque que crecía con `realloc`,
 * y crecer pedía otro bloque entero, más grande —en la C6 con la GUI, 16,4 KB seguidos con el mayor hueco de 13,3—.
 * `name_off` dice el trozo y el sitio dentro (`trozo << 11 | sitio`); para leer el nombre, `bpvm_symbol_name()`. Como
 * los trozos no se mueven, el `const char*` que devuelve sigue valiendo tras un registro nuevo (con el pool de un
 * bloque, no). */
typedef struct {
    uint32_t name_off;         /* trozo << 11 | sitio en el trozo (la cadena, terminada en 0) */
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
    uint32_t stack_top;        /* dirección alta (excluida): la base + lo que el hilo PIDIÓ (#604) */
    uint32_t stack_cap;        /* V7/H7 #604 — lo que mide su REGIÓN, que puede ser más (una reutilizada) */
    uint32_t pila_max;         /* V7/H7 #605 — el sp más alto visto en un ENTER, con los locales: cuánta pila usó */
    bpvm_thread_status_t status;

    /* F4 — bloqueo y join. */
    int32_t  blocked_on_mutex;  /* mid o -1 */
    int32_t  blocked_on_join;   /* tid o -1 */
    int64_t  wake_at_ms;        /* timestamp absoluto (BLOCKED_SLEEP) */
    bpref_t  thread_ref_heap;   /* ref del objeto Thread BP (handle COMPLETO, con gen) */

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
    /* V7/H5 P2b.1 — la pila que el intérprete le PRESTA cuando entra en una native (bpvm_nativa.h). NULL
     * mientras no haya entrado en ninguna: sólo pagan los hilos que lo hacen. Un puntero y no la cabecera
     * dentro del hilo, por lo mismo que el límite de fallos es por worker (#185: inflar el hilo ×N). */
    struct bpvm_nativa* nativa;
    /* V7/H5 P2b.2 — un builtin que se ejecuta llamado DESDE UNA NATIVE (call_builtin): si lanza, no deshace
     * marcos él mismo —los de C de la native siguen vivos— sino que deja la excepción aquí y la propaga el
     * límite de fallos de #186. El puente nativo→BP la apaga mientras corre código BP anidado. */
    int      builtin_de_nativa;
    uint32_t excepcion_pendiente;   /* handle empaquetado (idx|TAG) de esa excepción, o 0 */
    /* V7/H7 #600 — LA LÍNEA DE UN `print` A MEDIAS (bpvm_out.h): las piezas de `print "Resultado=", r` se
     * guardan aquí y la línea sale entera con su salto. NULL hasta la primera pieza de un `print` de varias; se
     * reutiliza (hasta BPVM_LINEA_MAX) y se libera en bpvm_destroy. Un puntero y no el búfer dentro del hilo:
     * #185 lo metió dentro (4 KB ×32 hilos en bpvm_t) y no cupo en la Pico. */
    char*    linea;
    uint32_t linea_len;
    uint32_t linea_cap;
    /* V7/H12, etapa 2 (4-oct) — LA COLA DE HILOS (bpvm_la_cola, bpvm_smp.h): el siguiente y el anterior, una lista
     * doble y circular de las structs de los hilos vivos. Sólo los toca threading.c, bajo vm_lock. Al final de la
     * struct: lo de arriba no se mueve. */
    struct bpvm_thread* next;
    struct bpvm_thread* back;
    /* V7/H12 — LA GUI, SÓLO EN EL RUN 0 (docs/V7_IDEAS.md §H12 «LVGL con dos runs»): 1 mientras el hilo espera a que el
     * run 0 lo elija para repetir el builtin de la GUI que encontró en otro run (interp.c, OP_CALL_BUILTIN). Lo pone el
     * intérprete del run que lo suelta; lo borra el run 0 al elegirlo (scheduler.c, bajo vm_lock). Al final: lo de
     * arriba no se mueve. */
    int al_run0;
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

/* V7 #494 — el mensaje de la excepción del heap descarrilado; el MISMO en miVM (VirtualMachine). */
#define BPVM_MSG_HEAP_DESCARRILADO "heap descarrilado (fallo interno de la VM)"
/* V7/H7 #595 — los textos de Heap.compact() cuando no se cumplen sus condiciones; los MISMOS en miVM. */
#define BPVM_MSG_COMPACT_MAIN  "Heap.compact: sólo se puede llamar desde Main"
#define BPVM_MSG_COMPACT_HILOS "Heap.compact: no se puede compactar con otros hilos vivos"

struct bpvm {
    /* ══ PREFIJO CONGELADO ══════════════════════════════════════════════════
     *
     * NO REORDENAR. NO METER NADA DELANTE. Un campo nuevo va DESPUÉS.
     *
     * El código AOT de un `.mdn` accede a estos dos campos POR SU
     * DESPLAZAMIENTO, que lleva horneado desde que se generó — es código
     * relocatable puro y no puede resolver nada por nombre. Los thunks de
     * SQLite, por ejemplo, hacen 34 accesos a `aot_helpers` y 16 a `memory`.
     *
     * ─── POR QUÉ ESTO ESTÁ AQUÍ (11-ago, y costó una mañana) ───
     *
     * Las DOS TABLAS de este sistema están bien diseñadas: la BIOS le llega al
     * pack COMO PARÁMETRO, y la de helpers está versionada y sólo crece por el
     * final. Ninguna se mueve.
     *
     * Lo que se movía no era ninguna tabla: era el PUNTERO a la tabla. Añadí un
     * `uint8_t` a `bpvm_module_t` —que va en `modules[16]` aquí dentro— y el
     * desplazamiento de `aot_helpers` se corrió 128 bytes. Los `.mdn` ya
     * generados siguieron leyendo la dirección vieja, cogieron basura y
     * saltaron ahí: cuelgue mudo en placa, sin un solo mensaje.
     *
     * O sea: EL CONTRATO SE HABÍA FILTRADO. El `.mdn` dependía del layout de
     * una struct interna que nadie había prometido mantener quieta.
     *
     * Criterio de Eduardo, que es el que arregla esto de raíz: *«tendría que
     * ser algo casi inmutable, debería como mucho crecer al final»*. Pues eso
     * — aquí delante, quietos, y con los asertos de abajo para que meter algo
     * delante ROMPA EL BUILD en vez de producir un firmware que cuelga. */
    uint8_t*                     memory;       /* offset 0 */
    const struct aot_helpers_v2* aot_helpers;  /* offset sizeof(void*) */
    /* ══ FIN DEL PREFIJO CONGELADO ══════════════════════════════════════════ */

    /* #339 — marca del guardián de fin de RUN: el número de secuencia de
     * reserva que había justo al arrancar este programa. Todo bloque con
     * secuencia >= esta marca que siga vivo cuando bpvm_destroy termine es
     * memoria que se quedó sin limpiar. Ver bpvm_alloc.h. */
    uint64_t run_mark;

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
    /* [V7/H2, #493] Si el principal vive en la ZONA de packs (RUN por nombre de
     * un modulo que solo esta ahi), la fuente en ejecucion es la zona entera y
     * aqui va el nombre de SU pack: sus modulos y recursos van primero, los de
     * los otros packs de la zona los ultimos. "" = pack del FS (#310), sin filtro. */
    char            run_pack_only[BPVM_PACK_NAME_LEN + 1];

    /* Allocator del data block (bump). */
    uint32_t next_free_address;
    uint32_t heap_start;       /* fijado tras último módulo cargado */
    uint32_t heap_next;        /* bump del heap (F2) */
    /* #451 — hasta dónde puede crecer el heap. NO es lo mismo que `stack_base`
     * (dónde empieza la pila del main), aunque hoy valgan igual: entre los dos
     * es donde va a vivir la tabla de handles. Todo lo que signifique «el final
     * del heap» pregunta por esto. */
    uint32_t heap_top;
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
    uint32_t gc_colectas;      /* V7 (28-sep) — colectas hechas en este RUN (Heap.getCollections) */
    /* #355 — reserva de emergencia: bytes del final del heap que el programa NO
     * puede tocar. Se sueltan cuando una reserva ya ha fallado, para que quede
     * sitio con el que CONSTRUIR el RuntimeError de OOM (mensaje + objeto).
     * Sin esto, quedarse sin memoria significaba quedarse tambien sin poder
     * avisar. 0 = ya gastada (o desactivada). */
    uint32_t heap_reserve;
    /* (#355: aquí estuvo `building_error`, 1 mientras se construía una excepción: la reserva de arriba sólo se gasta
     * entonces. V7/H12, 2b: con dos runs era de la VM, no del hilo —la veía el otro run—; ahora lo dice la propia
     * reserva: bpvm_heap_nuevo_error.) */
    /* Camino 1 (H-008, v3.0.1): bitmap "¿es inicio de cabecera real?" (1 bit por
     * palabra de 4B del heap). Se reconstruye al empezar cada mark; el scan
     * conservativo solo valida candidatos cuya (v-4) esté aquí → un entero que
     * cae a mitad de un objeto NO se toma por raíz ni se marca en banda (que
     * pisaba datos vivos). Espejo del set `valid` de la VM-Java. Alloc perezoso
     * en el 1er GC; se libera en bpvm_destroy. */
    uint8_t* gc_valid_map;
    size_t   gc_valid_map_size;

    /* V6/U5 — LA TABLA DE HANDLES, en su módulo: el tipo, sus constantes y su API
     * están en bpvm_handles.h y el código en bpvm_handles.c. Antes eran diez
     * campos sueltos aquí (paso 2b→4c de V4, #430, #451); la historia de cada
     * uno vive ahora junto al código que lo usa. */
    bpvm_handles_t handles;
    int       heap_oom_avisado;    /* idem para el heap lleno DE VERDAD */
    /* #430 — la excepcion PREFABRICADA del OOM (idea de Eduardo): se construye
     * en el prologo del RUN, cuando construir es gratis, y se lanza cuando la
     * construccion normal del error falla por falta de memoria (de heap o de
     * tabla). Raiz explicita del GC (gc_mark_phase 2a-bis). 0 = no hay. */
    bpref_t   oom_exc;
    int       prefabricando_oom;   /* V6/P1.C3.3: 1 solo mientras se PREFABRICA la
                                    * de #430. Sin esto, el aviso de "no hay
                                    * RuntimeError" suena igual que un OOM de
                                    * verdad — y engaña: me costo un diagnostico. */
    /* V7 #494 (27-sep, Eduardo: ni avisar y seguir ni parar la VM — «una excepción») — EL HEAP
     * DESCARRILADO. `gc_descarrilo`: el recorrido de cabeceras del último GC no acabó en heap_next
     * (build_gc_valid_map); ese GC ni marca ni barre, porque con el mapa truncado barrer se lleva
     * objetos vivos. La reserva que pidió ese GC falla por eso (devuelve 0, como sin memoria) y lo marca
     * en SU tarea (`reserva_descarrilada`, en el contexto de la native: #659); bpvm_throw_runtime_error
     * lo consume y lanza `heap_exc` —la prefabricada, construida en el prólogo del RUN como `oom_exc` y
     * raíz del GC como ella— en vez de la de sin memoria. Espejo de miVM (gcDescarrilo / heapExc). */
    bpref_t   heap_exc;
    uint8_t   gc_descarrilo;
    /* V7 #659 (7-oct) — los avisos de un `throw` (builtin_throw e interp_throw_rt), los cinco primeros DE ESTE RUN. Eran
     * `static`, de proceso: en una placa, pasado el quinto de la sesión ningún RUN decía nada; y con dos runs, el `++`
     * perdía cuentas (ahora es atómico). */
    int       avisos_throw;
    int       avisos_throw_interp;
    int       gc_suspended;    /* 1 = GC no corre (migración a handles) */

    /* Módulos cargados. */
    bpvm_module_t modules[BPVM_MAX_MODULES];
    int           module_count;
    uint32_t      main_absolute_address;   /* 0 = sin entry-point */

    /* F3 — global symbol table (función exports + data exports). */
    bpvm_symbol_t* symbols;
    int            symbol_count;
    int            symbol_capacity;
    /* #449 — pool de nombres: las cadenas van seguidas, cada una terminada en 0. #639 (3-oct): en TROZOS de 2 KB que no
     * se mueven (ver bpvm_symbol_t); un nombre no se parte entre dos. */
    char**         sym_trozos;
    int            sym_ntrozos;
    int            sym_trozos_cap;
    uint32_t       sym_trozo_usado;   /* lo ocupado del último trozo */
    uint32_t       sym_pool_len;      /* los bytes de nombres, en total (para el diag) */

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

    /* V7/H12, etapa 2 (4-oct, el diseño de Eduardo) — LOS HILOS. Cada hilo, para la VM, es una bpvm_thread_t pedida
     * con malloc al arrancarlo (la del main, al crear la VM) y devuelta cuando sale de la cola; la cola son esas
     * structs, una lista doble y circular (`next`, `back`): sin array y sin tope (antes, 32 huecos fijos aquí dentro,
     * 4,3 KB en un micro, se usaran o no). La cola y su referencia —el punto donde sigue la rueda— son PERMANENTES
     * desde la 2a.2: no viven aquí sino en `bpvm_la_cola` (bpvm_smp.h). El Thread de BP tiene su handle, `__tid`, que
     * no se reutiliza (`sig_tid` sólo crece, como en miVM): resolverlo es dar UNA vuelta (bpvm_hilo_por_tid); si no
     * está, el hilo acabó. Todo, en threading.c. */
    bpvm_thread_t* hilo_main;     /* el del main (tid 0): entra en la cola al colgar el programa y sale con él */
    int32_t        sig_tid;
    /* Las regiones de pila devueltas: una lista FIFO guardada DENTRO de ellas ([base] la siguiente, [base+4] su
     * tope), con la política de miVM (freedStackRegions): la primera en la que quepa, recortada a lo pedido. 0:
     * ninguna. */
    uint32_t       pilas_libres;
    uint32_t       pilas_libres_fin;
    /* Para el latido (bpvm_repl.c), que lo lee DESDE OTRA TAREA y sin cerrojo: el último hilo elegido, sin punteros
     * (una struct se puede devolver mientras tanto). */
    volatile int32_t  latido_tid;
    volatile uint32_t latido_pc;
    /* V7/H7 #605 — el hilo (que no es el main) que más cerca estuvo de su tope en este RUN: lo que usó y lo que tenía.
     * Con la cola (H12) un hilo acabado ya no está al final del RUN: se apunta al devolverlo (threading.c). */
    uint32_t       pila_peor_usada;
    uint32_t       pila_peor_total;
    /* H12, etapa 2 — cuántos hubo en la cola como mucho en este RUN (cuántos hay ahora: la cola, `en_cola`): la línea
     * `hilos:` del final (bpvm.c), que enseña que la memoria DECRECE —un hilo que acaba sale y devuelve su struct—
     * (`make test-hilos`). */
    int32_t        hilos_max;

    /* Output sink. */
    bpvm_output_cb output_cb;
    void*          output_user;

    /* Flags. */
    bool tracing;

    /* Buffer staging para imports/exports al cargar — re-usable. */
    uint8_t* scratch;
    size_t   scratch_size;


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

    /* LA COLA (V7/H12, etapa 2a.2; bpvm_smp.h), cuando el programa lo ejecuta MÁS DE UN run: el cerrojo de verdad
     * para el heap, los Mutex y los hilos (bpvm_smp_lock), y el mundo parado del GC. NULL con un run: nadie más toca
     * nada de eso mientras corre. Lo pone y lo quita la cola al publicar y descolgar el programa. */
    struct bpvm_cola* smp;

    /* V6/A1 — el hilo `io` (bpvm_io.c). NULL = nadie lo ha arrancado y todo
     * sigue como antes: `print` escribe en el sitio y el scheduler llama al
     * `poll_cb`. No-NULL = hay dos hilos de SO: `vm` sólo ejecuta opcodes y
     * `io` se encarga del wire y de la salida. Va AQUÍ, detrás del prefijo
     * congelado, por la misma razón que todo lo demás (ver arriba). */
    struct bpvm_io* io;

    /* Paso 4 (V3) — detalle legible del último fallo de link (lib/símbolo no
     * resuelto). Lo rellena bpvm_link_all; el handler de RUN lo manda al wire
     * en vez del exit-code mudo (antes el detalle solo iba a stderr). "" = sin
     * error. Tamaño fijo (sin malloc → MCU-friendly). */
    char link_error[1536];  /* V7/H2 (#523): un E3 con hasta 8 candidatas de 160 B (paridad con miVM: nunca se trunca) */

    /* Detalle del ÚLTIMO RuntimeError lanzado (msg de bpvm_throw_runtime_error):
     * p.ej. "referencia a objeto eliminado (use-after-free)". Espejo de
     * link_error → el host y el wire lo surten en vez del "exit N" mudo. Se
     * setea en CADA throw; solo se REPORTA cuando el run acaba con
     * BPVM_ERR_RUNTIME (no atrapado). "" = sin error. Fijo (MCU-friendly). */
    char runtime_error[192];

    /* #421 — detalle del ÚLTIMO fallo de CARGA de un módulo: qué ruta y qué le
     * pasa. Tercer hermano de `link_error` y `runtime_error`, y por la misma
     * razón: sin él, el wire mandaba «IO error» y el IDE enseñaba
     * `exit 1 (IO error)` — que en esa función significa a la vez «no existe»,
     * «mide cero», «está truncado» y «la ruta venía vacía», y nunca decía CUÁL
     * era la ruta.
     *
     * Va en la VM y no en un parámetro porque el fallo que de verdad ocurre no
     * es el del módulo principal, sino el de una DEPENDENCIA — y ésas se cargan
     * en `discover_deps`, que recorre los imports por su cuenta y no ve la estructura del llamante.
     * Ése fue el agujero del primer intento (16-ago): se arregló el camino menos
     * frecuente. "" = sin error. Fijo, sin malloc. */
    char load_error[256];

    /* V6/#412 — EL ARGUMENTO DE EJECUCION. Lo fija quien lanza el programa (el
     * verbo RUN del wire, o el CLI), y lo lee el builtin __runArg antes de que
     * arranque Main. "" = no se dio ninguno.
     *
     * La idea de Eduardo es la que simplifica esto: si el argumento se sube al
     * heap y se pasa la referencia, el caso SIN argumento pasa a ser el caso CON
     * argumento "" tambien en el heap — y deja de haber dos caminos. Hoy si los
     * hay, y no son inocuos: el argumento horneado es un literal de la ZONA DE
     * DATOS (sin cabecera de bloque, por debajo de heap_start) y uno de
     * ejecucion seria una cadena del HEAP. Son las dos formas que #389 tuvo que
     * reconocer en el CHECKCAST. Con esto, SIEMPRE es del heap.
     *
     * Fijo y sin malloc, como load_error. Va AL FINAL: el prefijo congelado del
     * .mdn (memory, aot_helpers) esta arriba y no se toca. */
    char run_arg[128];

    /* #486 - LOS DOS DISPATCHERS DEL GUI, RESUELTOS UNA VEZ.
     *
     * `GUI_RUN_ONCE` (y su gemelo `GUI_RUN`) buscaban `Gui.__guiDispatch` y
     * `Gui.__guiDispatchChange` recorriendo la tabla de simbolos ENTERA con
     * strcmp, en CADA pasada del bombeo — para dos funciones que no cambian
     * nunca dentro de un RUN. Medido en host: 12 us por vuelta con 1420
     * simbolos (~8,5 ns por simbolo).
     *
     * miVM ya lo cacheaba (`guiDispatchPc`, con centinela -2), asi que esto
     * ademas quita una asimetria entre las dos VMs.
     *
     * POR QUE EN LA VM Y NO EN UN `static`: la direccion depende del programa
     * CARGADO, y un mismo proceso ejecuta muchos RUN seguidos (el wire). Un
     * static sobreviviria al RUN y devolveria la direccion del programa
     * anterior. Aqui muere con la VM, que es lo correcto.
     *
     * Centinela: 0 = "sin resolver todavia". 1 = "buscado y NO existe" (el
     * programa no importa Gui, o no hay handlers): tambien hay que recordarlo,
     * o el caso "no esta" seguiria barriendo la tabla en cada vuelta. Ninguna
     * direccion real vale 0 ni 1.
     *
     * Va AL FINAL, como run_arg: el prefijo congelado del .mdn no se toca. */
    uint32_t gui_disp_click;
    uint32_t gui_disp_change;
    /* V7/H3 (#491) — EL REGISTRO DE FICHEROS ABIERTOS, enhebrado por los propios
     * bloques de estado (la ref del byte[] del objeto File; `next` en la cabecera
     * de la fachada). Sin tabla ni tope. Tres clientes: el GC (cierra lo
     * inalcanzable entre la marca y el barrido), el fin de RUN (cierra y cuenta)
     * y la apertura duplicada. Al final, como todo lo nuevo: el prefijo del .mdn
     * no se toca. 0 = ninguno. */
    uint32_t files_head;
    /* V7 #657 (7-oct) — y los que tiene abiertos un builtin de fichero entero MIENTRAS DURA (writeFile, readFile,
     * removeFile, rename, copyFile…: un open y un close, como un File): cada apunte vive en la pila del builtin
     * (builtins.c, abrir_breves). NULL = ninguno. */
    struct bpvm_abierto_breve* abiertos_breves;
    /* V7/H5 P2b.3 — EL DEPÓSITO DE EJECUTORES de natives (src/bpvm_nativa.h): los hilos de la plataforma en los que
     * corren las natives, creados al entrar en la primera y reutilizados; se terminan en bpvm_destroy. Al final,
     * como todo lo nuevo: el prefijo del .mdn no se toca. */
    struct bpvm_nativa* nativas;
    /* V7/H7 #593 (29-sep) — cuántos bloques NO libres hay en el heap, según la reserva: +1 al nacer (bpvm_heap_nuevo,
     * el método único), -1 al soltar un owner (bpvm_heap_free_block), y el barrido deja los que quedan. El recorrido
     * de cada GC cuenta los suyos; si no cuadran, se ha saltado bloques o se ha inventado alguno: heap descarrilado
     * (#494). Espejo de heapBloques en miVM. Al final: el prefijo del .mdn no se toca. */
    uint32_t heap_bloques;
    /* V7/H7 #608 (29-sep, Eduardo: la A) — un hilo que NO es el main murió con una excepción que nadie atrapó: ese hilo
     * se acabó y los demás siguieron, y al final el RUN dice que hubo un error (BPVM_ERR_RUNTIME, con el mensaje en
     * runtime_error), para que el IDE o un script no lo den por bueno. Lo pone a 0 cada RUN (preparar_run). */
    int      hilo_sin_atrapar;
    /* #639 (3-oct) — LOS SÍMBOLOS QUE NO CUPIERON en la tabla (sin memoria ni para lo justo): cuántos, y el primero con
     * lo que ocupaba la tabla. La carga sigue —un programa que no los use corre, como antes—; si el enlace busca uno
     * que no está, su mensaje da esta causa (link.c). Al final: el prefijo del .mdn no se toca. */
    int      sym_perdidos;
    char     sym_perdido[200];
    /* V7 #668 (8-oct) — LIBERAR ES DESTRUIR (destruir.c): el registro de lo que tiene destructor, la cola de lo
     * pendiente de destruir, la marca por clase y la fase del final. NULL hasta el primer objeto. Al final: el prefijo
     * del .mdn no se toca. */
    struct bpvm_destruir* destruir;
};

/* V7/H12, etapa 3.1, paso 6 — EL KILL CRUZA NÚCLEOS: lo pide `io` (o el poll, o el depurador) y lo mira el run, que con
 * dos núcleos corre en el otro. `volatile` sólo impide que el compilador se lo guarde; el orden entre núcleos lo dice
 * el modelo de memoria: se escribe con release y se lee con acquire (lo que dejó dicho `A4`). En la M33 de la RP2350
 * —en orden y sin caché de datos— cuesta lo mismo; en un micro con cachés (la P4), es lo que lo hace correcto. Se mira
 * una vez por cuanto: no está en el camino de cada paso. */
static inline int  bpvm_kill_pedido(const bpvm_t* vm) { return __atomic_load_n(&vm->kill_requested, __ATOMIC_ACQUIRE); }
/* V7 #592 — y un KILL sube la MARCA del 2º temporizador: una native en un bucle que llama a algo lo ve en la vuelta
 * siguiente y llama a punto_control, que la para (sin esperar al reloj). */
static inline void bpvm_kill_marcar(bpvm_t* vm, int v) {
    __atomic_store_n(&vm->kill_requested, v, __ATOMIC_RELEASE);
    if (v && vm->memory != NULL)
        (void) __atomic_fetch_add((uint32_t*) (void*) (vm->memory + BPVM_MARCA_TIEMPO_ADDR), 1u, __ATOMIC_RELEASE);
}

/* V7/H3 (#491) — en builtins.c. */
void bpvm_files_gc(bpvm_t* vm);          /* entre la marca y el barrido: cierra lo inalcanzable */
void bpvm_files_clavar(bpvm_t* vm);      /* V7/H7 #595: antes de compactar, el estado de lo abierto no se mueve */
void bpvm_files_fin_de_run(bpvm_t* vm);  /* al destruir la VM: cierra lo que quede y lo dice */


/* El guardián del prefijo congelado. Si alguien mete un campo delante de
 * `memory` o `aot_helpers`, o los reordena, ESTO NO COMPILA — que es
 * exactamente lo que hacía falta: el fallo pasa de "cuelgue mudo en placa" a
 * "no compila, y dice por qué". Ver el comentario largo dentro de la struct. */
#include <stddef.h>
_Static_assert(offsetof(struct bpvm, memory) == 0,
    "PREFIJO CONGELADO: `memory` tiene que estar la PRIMERA — el codigo AOT de "
    "los .mdn ya generados lee ese offset horneado. Pon tu campo DESPUES.");
_Static_assert(offsetof(struct bpvm, aot_helpers) == sizeof(void*),
    "PREFIJO CONGELADO: `aot_helpers` va JUSTO DETRAS de `memory`. Pon tu campo "
    "DESPUES del prefijo.");

/* ============================================================ */
/*  Helpers (util.c)                                             */
/* ============================================================ */

/* Lectura big-endian del memory[] (sin alignment requirements).
 *
 * V7/H12 (#660) — UNA PALABRA, DE UNA VEZ. Con dos runs (y con dos núcleos), otro hilo puede leer una palabra mientras
 * ésta se escribe: byte a byte —en la Pico, a -Os, cuatro STRB; en el PC el compilador los junta y no se ve— se lee a
 * medias, y una referencia a medias es una dirección que no es de nadie. Alineada, se lee y se escribe de una vez: una
 * carga o un almacén atómicos y relajados (sin orden: sólo de una pieza), con su orden de bytes —en la M33, LDR/STR y
 * REV, más corto que los cuatro bytes—. Sin alinear —los operandos del código: nadie los escribe a la vez—, según la
 * arquitectura (abajo). Y lo compartido está siempre alineado: los objetos y los arrays (bpvm.c), las globales en huecos de 4
 * desde CS (GC-2) con el bloque de cada módulo alineado por el cargador, la pila. `may_alias`: la memoria es de bytes. */
typedef uint32_t __attribute__((__may_alias__)) bpvm_u32_a;
typedef uint16_t __attribute__((__may_alias__)) bpvm_u16_a;
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BPVM_BE32(x) (x)
#define BPVM_BE16(x) (x)
#else
#define BPVM_BE32(x) __builtin_bswap32(x)
#define BPVM_BE16(x) __builtin_bswap16(x)
#endif
/* POR ARQUITECTURA (medido el 5-oct en la Pico 2). La M33 (la Pico y las STM32) y el x86 (el PC) leen y escriben una
 * palabra SIN ALINEAR con una sola instrucción, que es de una pieza si la dirección está alineada (lo compartido lo está
 * siempre) y vale igual si no (los operandos del código): ahí no se comprueba nada, y en línea SIEMPRE —a -Os el
 * compilador de la Pico no metía en línea la versión con la comprobación y cada acceso de la pila era una llamada:
 * fib(26) +6 %; así, leer una local y empujarla son un LDR y un STR—. El resto (RISC-V y Xtensa: las ESP32) no lee
 * palabras sin alinear: allí se comprueba la alineación, y lo raro, byte a byte y fuera de línea (bpvm_util.c). */
#if defined(__ARM_FEATURE_UNALIGNED) || defined(__x86_64__) || defined(__i386__)
#define BPVM_PALABRA_SIN_COMPROBAR 1
#define BPVM_EN_LINEA __attribute__((__always_inline__))
#else
#define BPVM_EN_LINEA
#endif
uint32_t bpvm_read_u32_be_bytes(const uint8_t* p);
uint16_t bpvm_read_u16_be_bytes(const uint8_t* p);
void     bpvm_write_u32_be_bytes(uint8_t* p, uint32_t v);
void     bpvm_write_u16_be_bytes(uint8_t* p, uint16_t v);
static inline BPVM_EN_LINEA uint32_t bpvm_read_u32_be(const uint8_t* p) {
#ifndef BPVM_PALABRA_SIN_COMPROBAR
    if (__builtin_expect((((uintptr_t) p) & 3u) != 0u, 0)) return bpvm_read_u32_be_bytes(p);
#endif
    return BPVM_BE32(__atomic_load_n((const bpvm_u32_a*) (const void*) p, __ATOMIC_RELAXED));
}
static inline BPVM_EN_LINEA int32_t bpvm_read_i32_be(const uint8_t* p) {
    return (int32_t) bpvm_read_u32_be(p);
}
static inline BPVM_EN_LINEA uint16_t bpvm_read_u16_be(const uint8_t* p) {
#ifndef BPVM_PALABRA_SIN_COMPROBAR
    if (__builtin_expect((((uintptr_t) p) & 1u) != 0u, 0)) return bpvm_read_u16_be_bytes(p);
#endif
    return BPVM_BE16(__atomic_load_n((const bpvm_u16_a*) (const void*) p, __ATOMIC_RELAXED));
}
static inline BPVM_EN_LINEA int16_t bpvm_read_i16_be(const uint8_t* p) {
    return (int16_t) bpvm_read_u16_be(p);
}
static inline BPVM_EN_LINEA void bpvm_write_u32_be(uint8_t* p, uint32_t v) {
#ifndef BPVM_PALABRA_SIN_COMPROBAR
    if (__builtin_expect((((uintptr_t) p) & 3u) != 0u, 0)) { bpvm_write_u32_be_bytes(p, v); return; }
#endif
    __atomic_store_n((bpvm_u32_a*) (void*) p, BPVM_BE32(v), __ATOMIC_RELAXED);
}
/* V7/H2 (#523) — u16 big-endian (el sello +6 del descriptor y las ranuras efectivas). */
static inline BPVM_EN_LINEA void bpvm_write_u16_be(uint8_t* p, uint16_t v) {
#ifndef BPVM_PALABRA_SIN_COMPROBAR
    if (__builtin_expect((((uintptr_t) p) & 1u) != 0u, 0)) { bpvm_write_u16_be_bytes(p, v); return; }
#endif
    __atomic_store_n((bpvm_u16_a*) (void*) p, (uint16_t) BPVM_BE16(v), __ATOMIC_RELAXED);
}
static inline BPVM_EN_LINEA void bpvm_write_i32_be(uint8_t* p, int32_t v) {
    bpvm_write_u32_be(p, (uint32_t) v);
}
/* H1.2 (V2): long i64 big-endian (high word en p, low en p+4). */
static inline BPVM_EN_LINEA int64_t bpvm_read_i64_be(const uint8_t* p) {
    return (int64_t)(((uint64_t) bpvm_read_u32_be(p) << 32)
                   |  (uint64_t) bpvm_read_u32_be(p + 4));
}
static inline BPVM_EN_LINEA void bpvm_write_i64_be(uint8_t* p, int64_t v) {
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

/* bpref_t (el HANDLE de 64b) está declarado arriba del todo, junto a los
 * includes: hay structs anteriores a este bloque que lo guardan como campo. */

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

/* V4 — bpvm_handle_register / bpvm_handle_kill: en bpvm_handles.h (V6/U5). */

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
int  bpvm_events_con_deuda(const bpvm_t* vm, const bpvm_thread_t* tc);   /* V7/H7 #595 */

/* Paso 3 / contrato B — ¿es `r` un handle a un objeto LIBERADO? Solo los handles
 * (con TAG) pueden morir; null/constantes nunca. Lo consulta el deref de PROGRAMA
 * (opcodes de campo/array/invoke) para gritar "objeto eliminado". */
static inline int bpvm_ref_dead(const bpvm_t* vm, bpref_t r) {
    uint32_t gen = (uint32_t) (r.v >> 32);   /* generación embebida en la palabra alta */
    if ((r.v & BPVM_HANDLE_TAG) == 0u) {
        /* V7/H12 (#660) — sin TAG es null o una constante del bloque de datos, y su palabra alta es SIEMPRE 0. Si no lo
         * es, es una referencia a medio escribir por otro run (la generación de un valor y la palabra baja de otro): hoy
         * se daba por viva y se resolvía a la dirección 0, donde la «longitud» de un array son los bytes del principio
         * de la memoria y un ASTORE escribía en cualquier sitio. Muerta: «objeto eliminado». (El sabotaje: SÓLO PRUEBAS,
         * test-palabras.) */
        if (gen == 0u || bpvm_sabotaje_desgarro) return 0;
        bpvm_ref_rota_report(gen, (uint32_t) r.v, vm->handles.next);
        return 1;
    }
    uint32_t idx = (uint32_t) r.v & ~BPVM_HANDLE_TAG;
    /* Paso 4b: compara la gen del handle con la del slot. Monotónico → todo handle
     * lleva gen=0, equivale al dead-flag; en 4c (reuso) un slot reciclado tiene gen
     * bumpeada y un handle rancio no matchea → grita. */
    /* V7/H12 (#660) — un handle que no puede existir (sin tabla, el índice 0 o fuera de ella) ya no se da por vivo: se
     * resolvía a la dirección 0, como arriba. */
    if (vm->handles.gen == NULL || idx == 0u || idx >= vm->handles.next) {
        bpvm_ref_rota_report(gen, (uint32_t) r.v, vm->handles.next);
        return 1;
    }
    if (vm->handles.gen[idx] == gen) return 0;
    bpvm_uaf_report(idx, gen, vm->handles.gen[idx], vm->handles.next);
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
    uint32_t gen = (vm->handles.gen != NULL && idx < vm->handles.next)
                   ? vm->handles.gen[idx] : 0u;
    bpref_t r; r.v = ((uint64_t) gen << 32) | (uint64_t) ref;
    return r;
}

/* Resolver una referencia a su offset en memory[] (DÓNDE vive el objeto). LA
 * indirección: sin TAG = null o constante del data block → identidad; con TAG =
 * consulta la tabla (defensivo: índice fuera de rango → 0). */
static inline uint32_t bpref_deref(const bpvm_t* vm, bpref_t r) {
    if ((r.v & BPVM_HANDLE_TAG) == 0u) return r.v;
    uint32_t idx = r.v & ~BPVM_HANDLE_TAG;
    if (idx == 0u || idx >= vm->handles.next) return 0u;
    /* Paso 7c — A1 publicación segura: ACQUIRE al leer el slot → garantiza VER el objeto
     * COMPLETAMENTE inicializado que el escritor publicó con RELEASE (bpvm_handle_register).
     * Es la mitad LECTOR del apretón de manos (el único punto de publicación = el slot).
     * INERTE en x86 (los loads ya son acquire); en ARM/RISC-V emite la barrera (dmb/ldar).
     * Validación real = fase de placa. */
    return __atomic_load_n(&vm->handles.addr[idx], __ATOMIC_ACQUIRE);
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
 * Con un run (vm->smp == NULL) son no-op. Con más de uno toman el cerrojo
 * de la cola (V7/H12, etapa 2a.2: bpvm_smp.h). Pensados para envolver
 * mutaciones a estado compartido (heap_alloc, thread_spawn, mutex_pool
 * ops) sin tener que duplicar el código por modo. Inline-static para
 * coste cero con un run.
 *
 * NO usar dentro de regiones ya bajo vm_lock (ej. dentro del
 * planificador, scheduler.c, que ya lo agarra explícitamente) — eso
 * sería recursión, y nuestro vm_lock NO es recursivo. */
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

/* heap.c, V7/H12 — esperar al mundo parado de OTRO run a mitad de algo: la pila de C de una native que espera dentro
 * de sí misma, a la vista del colector (poner/quitar, con el cerrojo cogido; `nodo` en el marco de quien espera); y el
 * puente —una llamada anidada que no vuelve al planificador— aparcado mientras dure el mundo parado. */
void bpvm_aparcada_poner(bpvm_cola_t* q, bpvm_aparcada_t* nodo);
void bpvm_aparcada_quitar(bpvm_cola_t* q, bpvm_aparcada_t* nodo);
void bpvm_stw_aparcar_anidado(bpvm_t* vm);

/* interp.c */
bpvm_status_t bpvm_interp_run(bpvm_t* vm);

/* heap.c (F2) — V7/H7 #593 (29-sep): LA reserva, en UN método. Bajo el cerrojo y de una vez: el hueco, la cabecera
 * ENTERA —el tipo y `cabecera`, la longitud de un array o la clase de un objeto—, el contenido a cero, la cuenta del
 * bloque y el handle. Devuelve el handle (bpref_deref da la dirección, para llenar el contenido) o nulo si no cabe,
 * en el heap o en la tabla: quien llama lanza «No space in heap». No hay otra forma de poner un bloque en el heap. */
bpref_t bpvm_heap_nuevo(bpvm_t* vm, int type, uint32_t cabecera, uint32_t payload_bytes);
/* #355 — lo mismo, para CONSTRUIR UN ERROR (bpvm_throw_runtime_error: su mensaje y su objeto): la única reserva que,
 * si no cabe, puede gastar la reserva de emergencia. V7/H12, 2b (5-oct): lo dice la reserva, no una bandera de la VM
 * —con dos runs, la del otro run—. */
bpref_t bpvm_heap_nuevo_error(bpvm_t* vm, int type, uint32_t cabecera, uint32_t payload_bytes);
/* El tamaño total de un bloque según su cabecera (el que usan el recorrido y el barrido): para las pruebas (#593). */
uint32_t bpvm_heap_bloque_tam(const bpvm_t* vm, uint32_t header_addr);

/* V7 (28-sep) — lo que un programa puede preguntar del heap (Heap.bp; V7_IDEAS §H7): sin reservar nada ni recorrer el
 * heap (sólo la lista de libres). Números de ESTA VM. Espejo de VirtualMachine.cuentasDelHeap. */
typedef struct {
    uint32_t tamano;       /* lo que un programa puede llegar a ocupar: [heap_start, heap_top - heap_reserve) */
    uint32_t libre;        /* el final sin usar más la lista de libres */
    uint32_t mayor_libre;  /* el mayor bloque libre (el final o el mayor hueco) */
    uint32_t huecos;       /* bloques en la lista de libres */
    uint32_t handles;      /* objetos en el heap: vivos y muertos sin recoger (slots de la tabla en uso) */
    uint32_t colectas;     /* colectas de este RUN */
} bpvm_heap_cuentas_t;
void bpvm_heap_cuentas(bpvm_t* vm, bpvm_heap_cuentas_t* out);
/* V7/H7 #595 (28-sep) — Heap.compact(): con el mundo parado, mira que `tc` sea el único hilo vivo y colecta y
 * compacta. «Desde Main» lo mira el builtin. 0 = hecho (o el GC apagado: no hace nada); 1 = el recorrido del heap se
 * descarriló (#494): no se compacta, y queda armada la prefabricada, que lanza bpvm_throw_runtime_error; 2 = hay
 * otro hilo vivo: ni colecta ni compacta. */
int bpvm_heap_compactar(bpvm_t* vm, const bpvm_thread_t* tc);
/* H-010 (v3.0.1): libera un bloque de objeto dejándolo consistente (size@+4 +
 * free-list), para que el recorrido del heap no se desincronice tras FREE_REF. */
void     bpvm_heap_free_block(bpvm_t* vm, uint32_t header_addr);
/* #656 — soltar un owner: su handle muere y, en cascada, los de sus campos owner y los elementos de un array owner, y
 * sus bloques vuelven a la lista de libres (con dos runs, al siguiente GC). SIN recursión. La llama el FREE_REF y la
 * asignación a un campo owner (interp.c, bpvm_free_owned). */
void     bpvm_heap_soltar_owned(bpvm_t* vm, bpref_t r);
/* #355 — suelta la reserva de emergencia. La llama SOLO una reserva para construir un error (bpvm_heap_nuevo_error):
 * si la soltara la reserva normal al fallar, se la quedaria el programa y el
 * error se volveria a quedar sin memoria con la que construirse. */
uint32_t bpvm_heap_release_reserve(bpvm_t* vm);
uint32_t bpvm_heap_alloc_string(bpvm_t* vm, const char* s, size_t len);
/* V7 #668 — soltar UN bloque y nada más (Core.__liberaSolo): sus hijos owner ya se destruyeron, antes que él. */
void     bpvm_heap_soltar_solo(bpvm_t* vm, bpref_t r);
uint32_t bpvm_heap_alloc_string_error(bpvm_t* vm, const char* s, size_t len);   /* #355: el mensaje de un error */
void     bpvm_heap_gc(bpvm_t* vm);

/* destruir.c — V7 #668 (8-oct): LIBERAR ES DESTRUIR. Un objeto con destructor se destruye lo libere quien lo libere:
 * su owner (Core.__destruye), el GC (lo resucita y lo deja pendiente) o el final del programa (todo lo vivo, el más
 * nuevo primero). Los destructores corren como código BP corriente: Core.__destruyePendientes, metida en un hilo
 * entre cuantos como un evento. Las primitivas de Core (272..277) las llama builtins.c. */
void     bpvm_destruir_al_nacer(bpvm_t* vm, bpref_t h, uint32_t class_ptr);    /* NEW_OBJECT: con destructor, al registro */
void     bpvm_destruir_en_el_gc(bpvm_t* vm, void (*marca)(bpvm_t*, uint32_t)); /* el marcado: pendientes y resucitados */
void     bpvm_destruir_inyecta(bpvm_t* vm, bpvm_thread_t* tc);                 /* entre cuantos, tras los eventos */
int      bpvm_destruir_en_fase_final(const bpvm_t* vm);                         /* la fase del final: sin eventos */
void     bpvm_destruir_vigila(bpvm_t* vm);                                      /* el tope de la fase del final */
int      bpvm_destruir_fase_final(bpvm_t* vm);   /* antes de acabar (cerrojo cogido): 1 = hay una ronda montada */
void     bpvm_destruir_liberar(bpvm_t* vm);                                     /* bpvm_destroy */
int      bpvm_destruir_libera_si_sin(bpvm_t* vm, bpref_t r);
int      bpvm_destruir_orden(bpvm_t* vm, bpref_t r, uint64_t** out, uint32_t* n);
int      bpvm_destruir_prepara(bpvm_t* vm, bpref_t r);
void     bpvm_destruir_nombre(bpvm_t* vm, bpref_t r, char* out, size_t cap);
uint64_t bpvm_destruir_siguiente(bpvm_t* vm, bpvm_thread_t* tc);
/* exceptions.c — V7 #668: ¿la clase `clase` desciende de `de`? (la regla de los catch; destruir.c la usa con Thread) */
int      bpvm_clase_desciende(const bpvm_t* vm, uint32_t clase, uint32_t de);

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
/* V7/H12 — ¿es `id` un builtin de la GUI (los BUILTIN_GUI_*: entran en gui.c, y por él en LVGL)? Con varios runs sólo
 * los ejecuta el run 0 (interp.c, OP_CALL_BUILTIN; docs/V7_IDEAS.md §H12 «LVGL con dos runs»). */
int bpvm_builtin_de_la_gui(int id);

/* V7/H5 P2b.2 — ¿este builtin cede o bloquea el hilo? (sleep, sleepSec, yield, Thread.join, Mutex.lock: los que
 * cambian tc->status). Sólo lo usa call_builtin como RESPALDO, donde la native no puede suspenderse: allí los
 * rechaza con nombre ANTES de ejecutarlos, sin efectos a medias. Con un ejecutor (P2b.3) se ejecutan y la native
 * se suspende. */
int bpvm_builtin_cede(int id);

/* link.c (F3). #639: si no cabe ni lo justo, el símbolo se APUNTA como perdido (vm->sym_perdidos) y devuelve OK: la
 * carga sigue, y el enlace lo dice si alguien lo busca. */
bpvm_status_t bpvm_link_register_symbol(bpvm_t* vm, const char* qualified,
                                         uint32_t abs_addr);
uint32_t      bpvm_link_lookup(const bpvm_t* vm, const char* qualified);
const char*   bpvm_symbol_name(const bpvm_t* vm, int i);   /* #449 — ver el pool */
bpvm_status_t bpvm_link_all(bpvm_t* vm);
uint32_t      bpvm_get_cs_for_data_addr(const bpvm_t* vm, uint32_t addr);
/* V7/H2 (#523) — reflexion de Forms con MOD9: slot de un metodo por nombre simple, o -1. */
int           bpvm_link_slot_por_nombre(const bpvm_t* vm, uint32_t class_ptr, const char* nombre);
/* V7/H2 (#533) — la ranura de la CLAVE exacta (`m(i)i`) subiendo por la cadena desde class_ptr;
 * un privado de un módulo distinto de `modulo_llamante` (nombre canónico; NULL = ninguno) no
 * cuenta, como en el enlace. -1 = no está. */
int           bpvm_link_slot_por_clave(const bpvm_t* vm, uint32_t class_ptr, const char* clave, const char* modulo_llamante);
/* #532 — el nombre cualificado (`Mod.Cls`) de la clase cuyo descriptor es class_ptr, por la 4.5
 * de su modulo (el sello +6). 1 si lo encontro; 0 (y out vacio) si no hay 4.5 (MOD7/8) o no casa. */
int           bpvm_link_nombre_de_clase(const bpvm_t* vm, uint32_t class_ptr, char* out, size_t cap);

/* L2 v3 — LA regla del despacho virtual: la ranura `slot` de la clase `desc`, subiendo por la
 * cadena de herencia —cada padre con el cs de SU módulo, que puede ser otro— hasta la primera
 * entrada que no sea -1 (-1 = «heredada»). 0, con el método (cs-relativo) en *off y el cs de su
 * módulo en *tcs; -1 si ninguna clase de la cadena la tiene. Espejo de `resolverVirtual` de miVM.
 * #530 (V7): el arranque de un hilo leía la ranura 0 (run()) SIN subir, y una clase cuyo run()
 * venía de un padre de otro módulo no arrancaba. La usan OP_INVOKE_VIRTUAL, bpvm_thread_spawn,
 * el puente AOT (call_method) y, desde #606, el manejador por ranura de Forms: una regla, un sitio. */
static inline int bpvm_vt_resolver(const bpvm_t* vm, uint32_t desc, uint32_t slot,
                                   int32_t* off, uint32_t* tcs) {
    const uint8_t* mem = vm->memory;
    for (;;) {
        uint16_t bw    = bpvm_read_u16_be(mem + desc + BPVM_CLS_OFF_BITMAP_WORDS);
        uint16_t nmeth = bpvm_read_u16_be(mem + desc + BPVM_CLS_OFF_NUM_METHODS);
        uint32_t vt_base = desc + BPVM_CLS_OFF_FIELD_BITMAP + 2u * (uint32_t) bw * 4u;
        if (slot < nmeth) {
            int32_t o = bpvm_read_i32_be(mem + vt_base + slot * 4u);
            if (o != -1) {
                *off = o;
                *tcs = bpvm_get_cs_for_data_addr(vm, desc);
                return 0;
            }
        }
        int32_t parent_off = bpvm_read_i32_be(mem + desc + BPVM_CLS_OFF_PARENT_OFF);
        if (parent_off == 0) return -1;
        desc = (uint32_t) ((int32_t) bpvm_get_cs_for_data_addr(vm, desc) + parent_off);
    }
}
/* V10 (16-sep, R2/R3/R6/R9) — la subtabla CLASES (4.5) recien retenida: filas dentro de la seccion, csOff dentro del
 * modulo, parentDepIdx en rango, ranuras coherentes. 0 si vale; si no, deja el motivo en vm->load_error y devuelve
 * BPVM_ERR_BAD_HEADER (un .mod corrupto se RECHAZA, no se lee basura). */
int bpvm_link_validar_clases(bpvm_t* vm, const bpvm_module_t* mod);
uint32_t      bpvm_get_cs_for_code_addr(const bpvm_t* vm, uint32_t code_addr);
uint32_t      bpvm_get_ext_table_addr(const bpvm_t* vm, uint32_t cs);

/* scheduler.c — V7/H12, etapa 2a.2: LA COLA y UN planificador (bpvm_smp.h: colgar, correr, despertar). Lo que la cola
 * le pide al programa (bpvm.c): PREPARARLO antes de publicarlo —enlazar, el main desde el punto de entrada, las
 * excepciones prefabricadas— y CERRARLO cuando acaba —las líneas a medias de cada hilo y las líneas `pila:` e
 * `hilos:`—. */
bpvm_status_t bpvm_programa_preparar(bpvm_t* vm);
void          bpvm_programa_cerrar(bpvm_t* vm);

/* interp.c (F4) — ejecuta el tc dado por un quantum o hasta que ceda. */
bpvm_status_t bpvm_interp_run_quantum(bpvm_t* vm, bpvm_thread_t* tc,
                                       int max_ops, int* yielded);

/* Mutex BP helpers (F4). */
int  bpvm_mutex_alloc(bpvm_t* vm);                    /* devuelve nuevo mid */
/* La propiedad de un mutex se toma y se suelta SOLO por estas dos, que llevan el
   cerrojo dentro. El check-and-set a pelo desde el builtin era la carrera que
   colgaba --smp=2 (ver el comentario largo en threading.c).
     try_acquire:  1 = tomado, 0 = bloqueado (ya apuntado como waiter), -1 = re-entrada
     release:      0 = ok, -1 = no era suyo (deja el owner real en *owner_out) */
int  bpvm_mutex_try_acquire(bpvm_t* vm, int mid, int tid);
int  bpvm_mutex_release(bpvm_t* vm, int mid, int tid, int* owner_out);

/* Variantes que asumen el cerrojo YA tomado (las usan las dos de arriba). */
void bpvm_mutex_add_waiter_locked(bpvm_t* vm, int mid, int tid);
int  bpvm_mutex_pop_waiter_locked(bpvm_t* vm, int mid);

void bpvm_mutex_add_waiter(bpvm_t* vm, int mid, int tid);
int  bpvm_mutex_pop_waiter(bpvm_t* vm, int mid);

/* Thread BP helpers (F4). */
int  bpvm_thread_spawn(bpvm_t* vm, bpref_t thread_ref);   /* tid; -1 no hay memoria para su struct (H12); -2 run() no resoluble (#530); -3 no cabe su pila (#604) */

/* V7/H12, etapa 2 — LA COLA DE HILOS (threading.c; la cola, bpvm_smp.h). Los hilos de `vm` son los de la cola si `vm`
 * es su dueño (de colgarlo a destruirlo); si no, ninguno. Recorrerlos es dar UNA vuelta:
 *     for (bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h)) ...
 * (sin sacar a nadie mientras tanto: eso es bpvm_hilos_recoger). Con más de un run, bajo vm_lock. */
bpvm_thread_t* bpvm_hilos_primero(const bpvm_t* vm);
bpvm_thread_t* bpvm_hilos_siguiente(const bpvm_t* vm, const bpvm_thread_t* h);
/* El handle de un Thread de BP (su __tid) → su struct; NULL si ya no está (el hilo acabó y salió de la cola). */
bpvm_thread_t* bpvm_hilo_por_tid(const bpvm_t* vm, int32_t tid);
/* El siguiente listo en rueda —desde el siguiente al punto de la rueda, UNA vuelta—, y la rueda sigue en él. Con
 * `libre`, sólo uno que no tenga ya un worker (SMP). `run`: el número del run que elige —uno que espera al run 0
 * (`al_run0`, la GUI) sólo lo elige el 0—. NULL si no hay ninguno. */
bpvm_thread_t* bpvm_hilos_elegir(bpvm_t* vm, int libre, int run);
/* V7/H12 — ¿Hay un hilo listo que sólo puede elegir el run 0 (`al_run0`)? Para un run que no es el 0 y no encuentra
 * hilo: eso no es un interbloqueo, es trabajo del run 0. Bajo vm_lock. */
int bpvm_hilos_esperan_al_run0(const bpvm_t* vm);
/* Saca de la cola y devuelve (struct y pila) los hilos que acabaron DEL TODO. Lo llaman los planificadores en su
 * pasada de despertar, bajo vm_lock. Devuelve cuántos. */
int  bpvm_hilos_recoger(bpvm_t* vm);
/* La struct del main (bpvm_init; todavía fuera de la cola); 0 o -1 si no hay memoria. Meterla en la cola, al colgar el
 * programa —con el cerrojo de la cola cogido—: 0, o -1 si la cola es de otro programa. Y todas fuera, con la cola
 * vacía después (bpvm_destroy). */
int  bpvm_hilos_crear_main(bpvm_t* vm, uint32_t stack_base);
int  bpvm_hilos_meter_main(bpvm_t* vm);
void bpvm_hilos_liberar(bpvm_t* vm);
/* #605 — apunta la pila de `h` si es el hilo (no main) más cerca de su tope en este RUN. */
void bpvm_hilo_apuntar_pila(bpvm_t* vm, const bpvm_thread_t* h);

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
    /* #571 (V7/H5, 26-sep) — un PUNTERO al `jmp_buf` del límite MÁS INTERNO, que vive en el marco de su
     * `aot_call_guarded`. Era el `jmp_buf` mismo, uno por worker: al anidar native → BP → native, la
     * interior lo pisaba con el suyo y, al volver, lo dejaba armado apuntando a su marco muerto; si la
     * exterior lanzaba después, el `longjmp` saltaba ahí (el PC moría en silencio). Ahora cada invocación
     * tiene el suyo y restaura el de fuera al salir. */
    jmp_buf*     buf;        /* destino del longjmp = boundary del interp (el más interno) */
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
    /* #302 paso 3 — TECHO DE LA PILA DE C del native en curso (0 = no hay).
     *
     * Idea de Eduardo (16-ago): en vez del shadow stack del diseño, que el GC
     * ESCANEE la pila que el native ya usa. Un handle que el native retiene
     * vive en sus locales/registros — en la pila de C, no en la de BP — y el
     * marcado no lo veía: `make test-aotgc` demostró que un intermedio de
     * `"valor " + intToString(n)` se recicla en mitad de la expresión (NULs con
     * status=OK, corrupción muda).
     *
     * `aot_call_guarded` apunta aquí la dirección de un local suyo al entrar al
     * thunk MÁS EXTERNO (con anidamiento native→BP→native gana el de fuera: sus
     * frames están por encima y también hay que verlos). El GC, que corre más
     * hondo en la misma pila (native → helper → alloc → gc), escanea desde su
     * propio frame hasta este techo. La pila crece hacia abajo en las tres
     * arquitecturas de la casa (x86-64, ARM, RISC-V).
     *
     * Cero coste fuera del AOT: sin thunk activo esto es 0 y el GC ni mira. Y
     * cero cambio de ABI: el thunk no sabe que esto existe — por eso los `.mdn`
     * ya grabados se vuelven seguros sin regenerarlos. */
    uintptr_t      cstack_hi;
    /* V7/H5 P2b.3 — el EJECUTOR en el que corre este hilo (src/bpvm_nativa.h), o NULL en el worker. El GC lo mira:
     * pedido dentro de un ejecutor, le pasa el testigo al worker y corre en su pila. Va aquí porque esto ya es lo
     * que cada hilo ve como suyo (TLS en el PC; en las placas, la vista que se turnan el worker y la native). */
    struct bpvm_nativa* ejecutor;
    /* V7 #659 (7-oct) — LA RESERVA QUE FALLÓ POR EL HEAP DESCARRILADO (#494), de ESTA tarea. Era una marca de la VM, y
     * con dos runs la consumía el `throw` del otro: le llegaba la excepción del heap descarrilado a quien no reservó, y
     * a quien reservó, la de sin memoria. La reserva que falla y el `throw` que la sigue van en la misma tarea, sin
     * ceder entre medias: aquí cada una tiene la suya. Sólo vale con el heap de ESTE RUN descarrilado
     * (`vm->gc_descarrilo`, que se borra al empezar cada RUN): una que se quedara sin consumir no se cuela en otro. */
    uint8_t        reserva_descarrilada;
} bpvm_aot_callctx_t;

/* Contexto AOT del worker actual (TLS en host). */
bpvm_aot_callctx_t* bpvm_aot_callctx(void);

/* V7/H12 (N1) — EL CONTEXTO DE LA NATIVE, UNO POR TAREA, en los micros con dos runs (docs/V7_IDEAS.md §H12 «Las natives
 * con dos runs»). En el PC lo da `__thread` (BPVM_TIENE_TLS). En un micro con BPVM_AOT_POR_TAREA, la plataforma guarda
 * por tarea un PUNTERO a uno de éstos (la Pico, en el puntero por tarea de FreeRTOS; la familia ESP32, en un
 * `__thread` de 4 bytes: el IDF le da a cada tarea su zona TLS); lo ponen las tareas que corren natives —cada run y
 * cada ejecutor— y las demás usan el de siempre, uno para todo el micro. Sin esto, dos natives a la vez —una en cada
 * run— se pisaban el contexto: el hilo, su pila y adónde saltar si lanza. */
typedef struct bpvm_aot_contexto {
    bpvm_aot_fault_t   fault;
    bpvm_aot_callctx_t cc;
} bpvm_aot_contexto_t;
#if defined(BPVM_AOT_POR_TAREA)
/* El de la tarea que llama (NULL: no puso ninguno) y ponerle uno (lo hace ella misma, al empezar). */
bpvm_aot_contexto_t* bpvm_platform_aot_contexto(void);
void                 bpvm_platform_aot_contexto_poner(bpvm_aot_contexto_t* c);
#endif
/* ¿Tiene esta imagen el contexto de la native por tarea (el PC, o N1 en el micro)? Con él, un programa con natives
 * puede ir con dos runs (bpvm_repl.c, un_solo_run). */
#if defined(BPVM_TIENE_TLS) || defined(BPVM_AOT_POR_TAREA)
#  define BPVM_AOT_CONTEXTO_POR_TAREA 1
#else
#  define BPVM_AOT_CONTEXTO_POR_TAREA 0
#endif

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
