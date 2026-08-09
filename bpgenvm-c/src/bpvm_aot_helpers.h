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
#include "bpvm_pack_api.h"   /* V5/H4: bpvm_pack_fn_t para `pack_sym` */

struct bpvm;

#ifdef __cplusplus
extern "C" {
#endif

/* Versión 2 del ABI (#302 paso 2 — modelo de handles). Slots añadidos solo al
 * final; nunca reordenar ni cambiar firmas en una version dada — el .mdn
 * referencia por offset (no por nombre), así que reordenar rompe módulos viejos.
 *
 * CONVENIO DE REFERENCIAS (v2): una `ref` que cruza esta ABI es un HANDLE
 * EMPAQUETADO — la palabra baja `idx|TAG` de un handle de 64b, con la generación
 * DESCARTADA (misma convención que la capa de builtins, docs/AOT_HANDLE_MODEL.md).
 * NO es el offset crudo al heap (eso era v1/pre-handles, y hoy corrompería: un
 * handle empaquetado lleva el bit TAG=0x40000000 → `vm->memory + ref` se sale de
 * rango). Cada helper DEREFIA dentro (bpref_deref∘bpref_regen); los que ALOCAN
 * devuelven un handle empaquetado (bpvm_handle_register). El ensanchado a 8 bytes
 * con la gen VIVA ocurre SOLO en las dos fronteras: el thunk (read_ref/write_ref,
 * contra la pila BP) y el puente native→BP (bridge_run_bp_frame vía ref_mask). */
typedef struct aot_helpers_v2 aot_helpers_v2_t;
struct aot_helpers_v2 {
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

    /* --- Strings (H3 #173) ----------------------------------------
     * Un string BP es un objeto heap con layout
     *   [nbytes:u32 BE][byte_0]...[byte_{nbytes-1}]
     * (TYPE_ARRAY_I8 — los bytes UTF-8 TAL CUAL, 1 byte por elemento; ver
     * `bpvm_heap_alloc_string` en heap.c, que es la autoridad).
     *
     * ⚠️ Este comentario decía «codepoint de 4 bytes / TYPE_ARRAY_I32», que era
     * cierto en la época de #173 y dejó de serlo en H2.1 (V4), cuando las
     * cadenas pasaron a UTF-8. Se corrigió al escribir `string_to_cstr`: de
     * haberme fiado del comentario habría decodificado codepoints de 4 bytes
     * sobre bytes UTF-8, y eso no da error — da basura.
     *
     * El "handle" que circula por el código native es el ref empaquetado
     * (0 = string null). Estos helpers
     * replican las ops de string del intérprete/builtins para que el
     * código AOT las invoque como C call directa.
     *
     * Los que devuelven un string nuevo (char_at, concat, substring,
     * from_cstr, int_to_string) alocan en el heap BP vía bpvm_heap_alloc
     * — pueden disparar GC. En F2 el GC no compacta (bump sin reuse),
     * así que los refs de entrada siguen válidos tras la alocación. */
    int32_t  (*string_length)(struct bpvm* vm, uint32_t ref);
    int32_t  (*string_char_code_at)(struct bpvm* vm, uint32_t ref, int32_t idx);
    uint32_t (*string_char_at)(struct bpvm* vm, uint32_t ref, int32_t idx);
    uint32_t (*string_concat)(struct bpvm* vm, uint32_t a, uint32_t b);
    uint32_t (*string_substring)(struct bpvm* vm, uint32_t ref, int32_t from, int32_t to);
    int32_t  (*string_eq)(struct bpvm* vm, uint32_t a, uint32_t b);
    /* Literal C (ASCII/UTF-8 byte→codepoint) → string heap. El AOT
     * emite los literales de string del .bp con esto. */
    uint32_t (*string_from_cstr)(struct bpvm* vm, const char* s, int32_t len);
    /* int → string decimal. Para format ("x = " + i) desde native. */
    uint32_t (*int_to_string)(struct bpvm* vm, int32_t v);

    /* --- Puente native→BP (P-aot-call-bp, §8) -----------------------
     * Permiten que código native (AOT) invoque funciones BP INTERPRETADAS:
     * tuplas (constructor BP), métodos (dispatch virtual BP), helpers BP de
     * otro módulo (#169 caso BP-target), etc.
     *
     * find_function: resuelve un nombre cualificado ("Modulo.func") a su
     *   dirección absoluta en memoria, o 0 si no existe. El thunk lo resuelve
     *   UNA vez y lo cachea en un static (patrón §4 Opción A).
     * call_bp_i32: llama a la función BP en `target_abs` con `nargs` args de 4
     *   bytes (cada uno un i32 o un HANDLE EMPAQUETADO para los args-ref) y
     *   devuelve su retorno (i32, o el handle empaquetado si ret_is_ref). Ver
     *   bpvm_aot_call_bp_i32 (interp.c) para la mecánica del frame falso + bucle
     *   anidado + sentinela. Restricción v1: el target no debe ceder al
     *   scheduler.
     *   #302 paso 2 — `ref_mask`: bit i = el arg i es una REFERENCIA → el puente
     *   lo ensancha a un handle de 8 bytes con la gen viva (bpref_regen) antes de
     *   escribirlo en el frame BP; los no-ref van a 4 bytes. `ret_is_ref`: el
     *   retorno de la función BP es una ref (8 bytes en la pila) → el puente
     *   popea 8 y devuelve el handle empaquetado. Sin esto un arg/retorno ref se
     *   truncaba y la función BP recibía un handle con gen basura. */
    uint32_t (*find_function)(struct bpvm* vm, const char* qualified);
    int32_t  (*call_bp_i32)(struct bpvm* vm, uint32_t target_abs,
                            const int32_t* args, int nargs,
                            uint32_t ref_mask, int ret_is_ref);

    /* #175 — throw con mensaje COMPUTADO desde native. Recibe un string-handle
     * BP (objeto heap UTF-8) en vez de un literal C; construye un RuntimeError
     * con ese mensaje y hace longjmp al boundary de #186 (propaga a try/catch
     * BP). NO retorna. Para `throw RuntimeError("lit")` se sigue usando
     * throw_runtime con el literal directo (sin alocar el string). */
    void (*throw_str)(struct bpvm* vm, uint32_t msg_ref);

    /* #174 (mitad-VM) — despacho VIRTUAL de un método público desde native.
     * `this_ref` = receptor; `slot` = índice de vtable (lo da el compilador);
     * args/retorno i32 (incl. refs). La VM resuelve obj→class→vtable[slot]
     * (con fallback al padre) y corre el cuerpo BP por el puente con `this_ref`
     * como arg0. NO requiere que el método esté exportado. call_method_i32 vive
     * en interp.c. */
    int32_t (*call_method_i32)(struct bpvm* vm, uint32_t this_ref, int slot,
                               const int32_t* args, int nargs,
                               uint32_t ref_mask, int ret_is_ref);

    /* #193 — arrays narrow de 1 byte (BP `byte[]`). Mismo layout/bounds que
     * OP_ALOAD_I8/OP_ALOAD_U8/OP_ASTORE_I8 del intérprete (paridad): el load_i8
     * extiende con signo a i32, load_u8 con cero, store_i8 trunca a 1 byte.
     * Para procesar bytes (descompresión, parsers binarios) desde código native
     * sin caer al helper i32 (que leería 4 bytes/elemento — incorrecto). */
    int32_t (*array_load_i8)(struct bpvm* vm, uint32_t ref, int32_t idx);
    int32_t (*array_load_u8)(struct bpvm* vm, uint32_t ref, int32_t idx);
    void    (*array_store_i8)(struct bpvm* vm, uint32_t ref, int32_t idx, int32_t v);

    /* #213 — throw de una excepción YA CONSTRUIDA (ref de objeto heap, p.ej.
     * una clase de usuario creada con call_bp_i32 a su factory __cls_new_*).
     * Deja el ref pendiente en el fault-slot y hace longjmp al boundary de
     * #186, que lo propaga TAL CUAL por el eh_stack BP (sin construir un
     * RuntimeError). NO retorna. Como siempre: slot nuevo AL FINAL. */
    void (*throw_ref)(struct bpvm* vm, uint32_t exc_ref);

    /* #302 paso 2 — FRONTERA thunk↔pila BP para referencias. Una ref ocupa 8
     * bytes en la pila BP (handle de 64b). read_ref lee esos 8 bytes y devuelve
     * el handle EMPAQUETADO (idx|TAG, palabra baja) que circula por el cuerpo
     * AOT; write_ref toma un handle empaquetado, reconstruye la gen VIVA
     * (bpref_regen) y escribe el handle de 8 bytes. Son el equivalente AOT del
     * read_i32_be/write_i32_be pero de 8 bytes y con regen — el thunk los usa
     * para los args/retorno de tipo referencia. */
    uint32_t (*read_ref)(const uint8_t* p);
    void     (*write_ref)(struct bpvm* vm, uint8_t* p, uint32_t ref);

    /* ── V5/H4 — LO QUE HACE FALTA PARA LLAMAR A UN PACK ─────────────────────
     *
     * Los dos son GENÉRICOS: ni la VM ni estos slots saben qué es SQLite. Como
     * siempre, AL FINAL — el .mdn referencia por offset, así que meterlos en
     * medio rompería los módulos ya compilados.
     *
     * `string_to_cstr` es la inversa de `string_from_cstr`, que ya estaba. Hacía
     * falta porque una cadena BP es un array de bytes CON LONGITUD y sin NUL
     * final, y cualquier función de C que reciba texto quiere un `const char*`.
     * Copia y termina en NUL; devuelve los bytes copiados (sin contar el NUL), o
     * -1 si no cabe. NO trunca en silencio: un camino de fichero cortado a la
     * mitad abriría OTRO fichero, y eso es peor que un error.
     *
     * `pack_sym` resuelve un símbolo de un pack por NOMBRE (el modelo
     * publics/externals). NULL = no está, y ese NULL es lo que convierte «el
     * usuario no grabó ese pack» en una excepción BP con nombre en vez de un
     * salto a ninguna parte. La versión se comprueba dentro: gate grueso. */
    int32_t        (*string_to_cstr)(struct bpvm* vm, uint32_t ref,
                                     char* dst, int32_t cap);
    bpvm_pack_fn_t (*pack_sym)(uint32_t marca, uint32_t version,
                               const char* nombre);

    /* V5/H4 — el AVISO cuando el puente al pack no puede seguir. NO RETORNA.
     *
     * Este tercer slot NO estaba en el plan: lo obligó una MEDIDA. El código
     * generado no puede llevarse el texto consigo, porque un literal de C vive
     * en `.rodata` y el `.mdn` empaqueta `.text` Y NADA MÁS —sin aplicar ni una
     * reloc—. Comprobado con el toolchain de verdad: `.rodata.str1.1` más un
     * `R_ARM_REL32` en `.text`, en TODOS los niveles de optimización. En placa
     * ese puntero no apuntaría a ningún sitio, y pasárselo a la excepción sería
     * cambiar un fallo con nombre por un cuelgue.
     *
     * Así que el texto se queda AQUÍ, en la VM, que se compila y enlaza normal.
     * El thunk sólo dice QUÉ pasó y CON QUÉ; la frase la redacta la VM:
     *
     *   motivo 1 → ese pack no ofrece ese símbolo. Los tres casos posibles —no
     *              está grabado, es otra versión, no trae esa función— se
     *              cuentan igual porque desde el thunk son indistinguibles; el
     *              mensaje enumera los tres para que el usuario sepa dónde mirar.
     *   motivo 2 → una cadena no cabe en el buffer del puente.
     *
     * `marca` se imprime como los 4 caracteres que es ('SQLI'), no como número:
     * 0x53514C49 no le dice nada a nadie. */
    void (*pack_fallo)(struct bpvm* vm, uint32_t marca, uint32_t version,
                       const char* nombre, int32_t motivo);

    /* V5/H4 — elementos de 8 bytes: `long[]` y `double[]`.
     *
     * Faltaban desde H1.2c: el lenguaje tiene `long[]` desde entonces, pero la
     * tabla sólo llegaba a 4 bytes (i32) y 1 (i8). Nadie lo notó porque hasta
     * ahora ningún `native` había tocado uno.
     *
     * Los saca a la luz el puente a un pack: un `rowid` o una marca de tiempo en
     * milisegundos NO caben en 32 bits, y como el AOT v1 no marshalla 8 bytes
     * (tarea #381), la vuelta es por ARRAY DE SALIDA — el array cruza como
     * handle de 4 bytes y el thunk deja el valor en su elemento 0. Sin estos
     * cuatro slots ese rodeo no se podía cerrar.
     *
     * Mismo contrato que los de i32: índice fuera de rango o array nulo lanzan,
     * no devuelven basura. */
    int64_t (*array_load_i64) (struct bpvm* vm, uint32_t ref, int32_t idx);
    void    (*array_store_i64)(struct bpvm* vm, uint32_t ref, int32_t idx, int64_t v);
    double  (*array_load_f64) (struct bpvm* vm, uint32_t ref, int32_t idx);
    void    (*array_store_f64)(struct bpvm* vm, uint32_t ref, int32_t idx, double v);
};

/* V5/H4 — cuánto texto cabe cruzando hacia un pack, en BYTES.
 *
 * Un pack recibe `const char*`, así que la cadena BP hay que copiarla a un
 * buffer, y ese buffer sale de la PILA del thunk. De ahí el número: no es una
 * limitación de las cadenas de BP, es cuánta pila se le puede pedir prestada a
 * un micro por llamada. Si no cabe, se DICE (motivo 2) — nunca se trunca.
 *
 * Está aquí, en un solo sitio, porque lo tienen que compartir dos lados que se
 * compilan por separado: el emisor lo estampa en el `.c` generado y la VM lo
 * nombra en el mensaje de error. Si se separan, el aviso mentiría. */
#define BPVM_AOT_PACK_STR  512

/* Tabla v2 instanciada en el runtime con los punteros a las funciones
 * reales. Compartida entre todas las VMs del proceso. Pasada a cada
 * vm en bpvm_init via vm->aot_helpers. */
extern const aot_helpers_v2_t bpvm_aot_helpers_v2;

#ifdef __cplusplus
}
#endif

#endif /* BPVM_AOT_HELPERS_H */
