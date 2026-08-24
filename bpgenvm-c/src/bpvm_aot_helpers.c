/*
 * bpvm_aot_helpers.c — instancia de la tabla aot_helpers_v1_t con
 * punteros a las funciones reales del runtime. Linkada SIEMPRE en
 * cualquier build (host o Pico), aunque no haya código AOT cargado.
 *
 * Si el linker se queja de "undefined reference" tras un cambio en
 * el header (p.ej. añadiste un slot), implementar el helper aquí
 * con un trampolín al símbolo del runtime correspondiente.
 */

#include "bpvm_aot_helpers.h"
#include "bpvm_internal.h"
#include "bpvm_bios.h"      /* V5/H4: el registro de packs, para `pack_sym` */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <inttypes.h>
#include <math.h>          /* #426: fmod/floor/isinf/fabs/exp/log de los helpers de double */

/* ---------- #186: slot de fault por worker ----------
 * Ver bpvm_internal.h para el diseño. En host hay N workers pthread →
 * TLS (__thread). En Pico la config validada es single-worker → un
 * global plano basta (multi-worker Pico = v2, necesitará task-local). */
#if defined(BPVM_PICO_NUM_CORES) || defined(ESP_PLATFORM)
   /* MCU (Pico / ESP32): el AOT no corre (no hay codegen para la ISA del
    * micro — el .mdn es ARM Thumb-2), así que el fault-slot NUNCA se arma
    * → un global plano basta y evita depender de __thread (ELF TLS) en el
    * toolchain del micro. */
#  define BPVM_AOT_TLS
#else
#  define BPVM_AOT_TLS __thread   /* host: N workers pthread */
#endif

static BPVM_AOT_TLS bpvm_aot_fault_t g_aot_fault;

bpvm_aot_fault_t* bpvm_aot_fault_slot(void) {
    return &g_aot_fault;
}

/* P-aot-call-bp: contexto del puente native→BP, por worker (mismo
 * razonamiento TLS que el fault-slot). Lo fija aot_call_guarded (interp.c)
 * alrededor de cada thunk; tc==NULL fuera de un thunk. */
static BPVM_AOT_TLS bpvm_aot_callctx_t g_aot_callctx;

bpvm_aot_callctx_t* bpvm_aot_callctx(void) {
    return &g_aot_callctx;
}

/* ---------- #302 paso 2 — frontera de referencias ----------
 * A(vm, ref): resuelve un HANDLE EMPAQUETADO (idx|TAG, gen descartada — la
 * representación de una ref en la ABI AOT v2) a su offset crudo en memory[].
 * bpref_regen le devuelve la gen viva; bpref_deref hace la indirección de tabla.
 * ref==0 (null) → addr 0. TODO helper que antes hacía `vm->memory + ref` debe
 * pasar por aquí: en v2 `ref` YA NO es un offset (lleva el TAG). */
static inline uint32_t A(bpvm_t* vm, uint32_t ref) {
    return bpref_deref(vm, bpref_regen(vm, ref));
}

/* Frontera thunk↔pila BP: una ref son 8 bytes en la pila (handle de 64b).
 * read_ref devuelve la palabra baja (handle empaquetado idx|TAG) que circula
 * por el cuerpo AOT; write_ref reconstruye la gen viva y escribe los 8 bytes. */
static uint32_t h_read_ref(const uint8_t* p) {
    return (uint32_t) bpvm_read_i64_be(p);
}
static void h_write_ref(bpvm_t* vm, uint8_t* p, uint32_t ref) {
    bpvm_write_i64_be(p, (int64_t) bpref_regen(vm, ref).v);
}

/* ---------- I/O memoria big-endian ----------
 * Estos están como static inline en bpvm_internal.h — necesitamos
 * un wrapper extern para tenerlos en la tabla. Mismas semánticas. */
static int32_t h_read_i32_be(const uint8_t* p) {
    return bpvm_read_i32_be(p);
}
static void h_write_i32_be(uint8_t* p, int32_t v) {
    bpvm_write_i32_be(p, v);
}
static int16_t h_read_i16_be(const uint8_t* p) {
    return (int16_t) bpvm_read_u16_be(p);
}
static void h_write_i16_be(uint8_t* p, int16_t v) {
    p[0] = (uint8_t)((v >> 8) & 0xFF);
    p[1] = (uint8_t)(v & 0xFF);
}
/* #381 — los de 8 bytes, para el marshalling de `long`. Mismas funciones que
 * usa el intérprete para poner un `long` en la pila, ni una más: si aquí se
 * escribiera otra cosa, un `long` valdría distinto según fuera interpretado o
 * compilado, que es justo el invariante que no se puede romper. */
static int64_t h_read_i64_be(const uint8_t* p) {
    return bpvm_read_i64_be(p);
}
static void h_write_i64_be(uint8_t* p, int64_t v) {
    bpvm_write_i64_be(p, v);
}

/* #381 — división y módulo de 64 bits para el código nativo (ver el .h).
 *
 * ESPEJO EXACTO de `OP_LDIV`/`OP_LMOD` del intérprete (`interp.c:1141`): el
 * mismo chequeo, el mismo mensaje y el mismo operador de C. Cualquier mejora
 * que se le hiciera a uno hay que hacérsela al otro el mismo día, o el mismo
 * programa daría resultados distintos según se ejecute compilado o interpretado.
 *
 * (Nota de un caso que YA diverge hoy, y que esto no empeora ni arregla:
 * `LONG_MIN / -1` desborda. En Java el resultado es `LONG_MIN`; en C es
 * comportamiento indefinido. El intérprete de la VM-C tiene el mismo `a / b`
 * a pelo, así que la divergencia es preexistente y le toca a su propia ficha —
 * arreglarlo sólo aquí sería introducir una diferencia nueva.) */
static int64_t h_idiv64(bpvm_t* vm, int64_t a, int64_t b) {
    if (b == 0) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "División por cero");   /* no retorna */
        return 0;
    }
    return a / b;
}
/* ---------- #426 (V6/N1.2): coma flotante de doble precision ----------
 *
 * Ninguna familia de hoy tiene FPU de doble, asi que un `a*b` de double escrito
 * en el .c generado se vuelve una llamada a libgcc — y un `.mdn` no resuelve
 * simbolos externos. Estos helpers viven DENTRO del firmware, que si esta
 * enlazado contra libgcc: le prestan al `.mdn` la coma flotante que ya hay.
 *
 * ⚠️ Son el ESPEJO EXACTO de los OP_D* del interprete (`interp.c`), no una
 * version "mejorada". Si un dia se toca uno hay que tocar el otro EL MISMO DIA,
 * o el mismo programa dara resultados distintos segun lleve `.mdn` o no — que es
 * el invariante que sostiene todo el proyecto. Con double pica mas que con long:
 * NaN, infinitos, -0.0 y las conversiones fuera de rango son cuatro sitios donde
 * es facil ser "mas correcto" que el interprete y romperlo.
 *
 * Ninguno lanza: la division por cero en coma flotante NO es un error, da
 * inf/NaN — igual que OP_DDIV, que tampoco comprueba nada. */
static double h_dadd(double a, double b) { return a + b; }
static double h_dsub(double a, double b) { return a - b; }
static double h_dmul(double a, double b) { return a * b; }
static double h_ddiv(double a, double b) { return a / b; }
static double h_dmod(double a, double b) { return fmod(a, b); }
static double h_dneg(double a)           { return -a; }

/* Las seis comparaciones, cada una por su lado. Con NaN NO son negaciones unas
 * de otras —`!(a<b)` no es `a>=b`— asi que un unico `dcmp` de -1/0/1 divergiria
 * del interprete en cuanto apareciera un NaN. */
static int32_t h_deq (double a, double b) { return a == b ? 1 : 0; }
static int32_t h_dneq(double a, double b) { return a != b ? 1 : 0; }
static int32_t h_dlt (double a, double b) { return a <  b ? 1 : 0; }
static int32_t h_dle (double a, double b) { return a <= b ? 1 : 0; }
static int32_t h_dgt (double a, double b) { return a >  b ? 1 : 0; }
static int32_t h_dge (double a, double b) { return a >= b ? 1 : 0; }

/* Conversiones: los mismos casts que OP_I2D/OP_D2I/OP_L2D/OP_D2L/OP_F2D/OP_D2F. */
static double  h_i2d(int32_t v) { return (double)  v; }
static int32_t h_d2i(double  d) { return (int32_t) d; }
static double  h_l2d(int64_t v) { return (double)  v; }
static int64_t h_d2l(double  d) { return (int64_t) d; }
static double  h_f2d(float   f) { return (double)  f; }
static float   h_d2f(double  d) { return (float)   d; }

static int64_t h_imod64(bpvm_t* vm, int64_t a, int64_t b) {
    if (b == 0) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "Módulo por cero");     /* no retorna */
        return 0;
    }
    return a % b;
}

/* ---------- Control de ejecución ----------
 * throw_runtime levanta un BpThreadFault que sube por la pila de
 * frames hasta el handler más cercano (o tumba el thread). */
static void h_throw_runtime(bpvm_t* vm, const char* msg) {
    (void) vm;
    /* #186: si el call-site AOT del intérprete armó un boundary en este
     * worker, copiamos el mensaje y hacemos longjmp de vuelta. El interp
     * construye el RuntimeError y lo propaga al try/catch BP (o termina
     * el thread). NO retornamos — el native NO sigue ejecutando con
     * basura como antes. */
    bpvm_aot_fault_t* f = bpvm_aot_fault_slot();
    if (f->armed) {
        size_t n = msg ? strlen(msg) : 0;
        if (n > sizeof(f->msg) - 1) n = sizeof(f->msg) - 1;
        if (msg && n) memcpy(f->msg, msg, n);
        f->msg[n] = '\0';
        longjmp(f->buf, 1);   /* no retorna */
    }
    /* Sin boundary armado: no debería ocurrir (los native sólo corren
     * vía el hijack AOT del intérprete). Reportamos al menos. */
    if (msg) bpvm_diag_urgente("[aot] throw_runtime sin boundary: %s", msg);
}

/* #175 — throw con mensaje COMPUTADO: el AOT pasa un string-handle BP (objeto
 * heap TYPE_ARRAY_I8 UTF-8) en vez de un literal C. Leemos sus bytes y
 * reusamos el camino de throw_runtime (construir RuntimeError + longjmp al
 * boundary de #186). NO retorna. */
static void h_throw_str(bpvm_t* vm, uint32_t msg_ref) {
    char buf[128];
    uint32_t n = 0;
    uint32_t addr = msg_ref ? A(vm, msg_ref) : 0;   /* v2: handle → offset */
    if (addr != 0) {
        n = bpvm_read_u32_be(vm->memory + addr);
        if (n > sizeof(buf) - 1) n = (uint32_t)(sizeof(buf) - 1);
        memcpy(buf, vm->memory + addr + 4, n);
    }
    buf[n] = '\0';
    h_throw_runtime(vm, buf);   /* no retorna (longjmp al boundary) */
}

/* #213 — throw de excepción YA construida (clase de usuario via factory).
 * Mismo viaje que h_throw_runtime pero sin mensaje: el boundary lee
 * pending_ref y lo propaga por el eh_stack. NO retorna. */
static void h_throw_ref(bpvm_t* vm, uint32_t exc_ref) {
    (void) vm;
    bpvm_aot_fault_t* f = bpvm_aot_fault_slot();
    if (f->armed) {
        f->pending_ref = exc_ref;
        f->msg[0] = 0;
        longjmp(f->buf, 1);   /* no retorna */
    }
    bpvm_diag_urgente("[aot] throw_ref sin boundary (ref=%" PRIu32 ")", exc_ref);
}

/* ---------- Heap / GC ----------
 * [V6/N1.5] Dejaron de ser stubs. Eran cuatro `return 0` — o sea que un native
 * que creara un array recibia el ref NULO y seguia como si nada; el fallo salia
 * lejos del sitio, al usarlo. Ahora hacen lo MISMO que sus opcodes
 * (OP_NEWARRAY / _I8 / _I16 en interp.c:1410-1446), que es el contrato: si el
 * camino compilado alocara distinto que el interpretado, el mismo programa
 * daria dos resultados segun llevara `.mdn`.
 *
 * ⚠️ Lo que NO hace falta aqui y si hace falta en el interprete: el SAFEPOINT
 * (`tc->sp = sp`). El interprete lo pone para que el GC vea su pila; un native
 * no tiene pila BP que publicar — sus handles viven en locales de C y en
 * registros, y a esos los mira el GC desde V5 (heap.c §2d, la idea de Eduardo
 * de escanear la pila de C del native con el volcado de registros de setjmp).
 * Sin ese mecanismo, ninguno de estos cuatro helpers seria seguro: el array
 * recien creado podria recolectarse antes de que el native lo guardara.
 *
 * OOM: se LANZA, no se devuelve 0. `throw_runtime` hace longjmp al boundary y
 * el error sale como RuntimeError atrapable — igual que BPVM_RT_THROW("No space
 * in heap") en el intérprete. Un 0 devuelto en silencio es lo que hacia el
 * stub, y es justo lo que no queremos. */
/* El cuerpo comun: aloca `bytes`, escribe la longitud EN ELEMENTOS en la
 * cabecera y registra el handle. Devuelve el handle empaquetado. */
static int32_t h_newarray_tipo(bpvm_t* vm, int32_t size, uint32_t bytes_por_elem,
                               int tipo) {
    if (size < 0) { h_throw_runtime(vm, "tamaño de array negativo"); return 0; }
    uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) size * bytes_por_elem, tipo);
    if (ref == 0) { h_throw_runtime(vm, "No space in heap"); return 0; }
    bpvm_write_u32_be(vm->memory + ref, (uint32_t) size);
    bpref_t h = bpvm_handle_register(vm, ref);
    if (h.v == 0u) { h_throw_runtime(vm, "No space in heap"); return 0; }   /* #430 */
    return (int32_t) h.v;
}
static int32_t h_newarray_i32(bpvm_t* vm, int32_t size) {
    return h_newarray_tipo(vm, size, 4, BPVM_TYPE_ARRAY_I32);
}
static int32_t h_newarray_i8(bpvm_t* vm, int32_t size) {
    return h_newarray_tipo(vm, size, 1, BPVM_TYPE_ARRAY_I8);
}
static int32_t h_newarray_i16(bpvm_t* vm, int32_t size) {
    return h_newarray_tipo(vm, size, 2, BPVM_TYPE_ARRAY_I16);
}
/* [V6/N1.5] `long[]` y `double[]`: 8 bytes OPACOS por casilla (el GC no los
 * traza, que es justo lo que se quiere para numeros). Mismo tipo que usa
 * OP_NEWARRAY_I64 — un `double[]` es un I64 con otro contenido, como en el
 * intérprete. */
static int32_t h_newarray_i64(bpvm_t* vm, int32_t size) {
    return h_newarray_tipo(vm, size, 8, BPVM_TYPE_ARRAY_I64);
}

/* [V6/N1.5] Este SIGUE sin implementarse, y ahora lo DICE en vez de devolver 0.
 *
 * No es que no se pueda alocar el objeto — es que alocarlo no basta: un objeto
 * BP se construye ejecutando su CONSTRUCTOR, y el constructor es bytecode. Por
 * eso el emisor no genera `new_object`: genera una llamada por el puente a la
 * factoria `__cls_new_<Clase>` (call_bp_i32), que corre el ctor de verdad en el
 * intérprete y devuelve la instancia. Es la misma ruta que #213 abrio para
 * `throw MiExcepcion(...)`.
 *
 * El slot se queda —la tabla solo crece por el final— y falla ruidosamente por
 * si algun `.mdn` viejo lo invoca. */
static int32_t h_new_object(bpvm_t* vm, uint32_t class_addr) {
    (void) class_addr;
    h_throw_runtime(vm, "new_object: el AOT crea objetos por la factoria "
                        "__cls_new_ (el ctor es bytecode), no por este helper");
    return 0;
}

/* ---------- Output sink ----------
 * Replican los OP_PRINT_* del intérprete pero invocables como C. */
static void h_print_i32(bpvm_t* vm, int32_t v, int nl) {
    char buf[24];
    int n = snprintf(buf, sizeof(buf), nl ? "%d\n" : "%d", (int) v);
    if (n > 0 && vm->output_cb) {
        vm->output_cb(buf, (size_t) n, vm->output_user);
    } else if (n > 0) {
        fwrite(buf, 1, (size_t) n, stdout);
    }
}
static void h_print_f32(bpvm_t* vm, float v, int nl) {
    char buf[40];
    int n = snprintf(buf, sizeof(buf), nl ? "%g\n" : "%g", (double) v);
    if (n > 0 && vm->output_cb) {
        vm->output_cb(buf, (size_t) n, vm->output_user);
    } else if (n > 0) {
        fwrite(buf, 1, (size_t) n, stdout);
    }
}
static void h_print_string(bpvm_t* vm, uint32_t ref, int nl) {
    /* H2 (V2): string = byte[] UTF-8 → emite los bytes directamente, sin
     * truncar (paridad con OP_PRINT_STRING del intérprete). v2: deref el handle. */
    uint32_t addr = ref ? A(vm, ref) : 0;
    if (addr != 0) {
        uint32_t nbytes = bpvm_read_u32_be(vm->memory + addr);
        const char* p = (const char*)(vm->memory + addr + 4);
        if (nbytes > 0) {
            if (vm->output_cb) vm->output_cb(p, (size_t) nbytes, vm->output_user);
            else fwrite(p, 1, (size_t) nbytes, stdout);
        }
    }
    if (nl) {
        if (vm->output_cb) vm->output_cb("\n", 1, vm->output_user);
        else fputc('\n', stdout);
    }
}
static void h_print_char(bpvm_t* vm, int32_t ch) {
    char c = (char) ch;
    if (vm->output_cb) vm->output_cb(&c, 1, vm->output_user);
    else fputc(ch, stdout);
}
static void h_print_nl(bpvm_t* vm) {
    if (vm->output_cb) vm->output_cb("\n", 1, vm->output_user);
    else fputc('\n', stdout);
}

/* ---------- I/O float (H3 #166) ----------
 * Los floats viven en el stack BP como bits IEEE-754 i32 big-endian.
 * Estos wrappers hacen la conversión bits↔float para que el thunk AOT
 * no tenga que hacer type-punning manual.
 *
 * bits_to_float / float_to_bits replicados aquí inline; los originales
 * en interp.c son `static` y no exportados. */
static inline float aoth_bits_to_float(uint32_t bits) {
    union { float f; uint32_t u; } u; u.u = bits; return u.f;
}
static inline uint32_t aoth_float_to_bits(float f) {
    union { float f; uint32_t u; } u; u.f = f;    return u.u;
}
static float h_read_f32_be(const uint8_t* p) {
    return aoth_bits_to_float((uint32_t) bpvm_read_i32_be(p));
}
static void h_write_f32_be(uint8_t* p, float v) {
    bpvm_write_i32_be(p, (int32_t) aoth_float_to_bits(v));
}
/* #426 — los mismos, en 64 bits. El patron de bits de la pila BP es EL MISMO que
 * escribe el interprete (bpvm_write_i64_be sobre double_to_bits), asi que un
 * `double` que cruza el thunk y vuelve es identico bit a bit. */
static double h_read_f64_be(const uint8_t* p) {
    union { double d; uint64_t u; } u; u.u = (uint64_t) bpvm_read_i64_be(p);
    return u.d;
}
static void h_write_f64_be(uint8_t* p, double v) {
    union { double d; uint64_t u; } u; u.d = v;
    bpvm_write_i64_be(p, (int64_t) u.u);
}

/* ---------- Acceso a arrays (H3 #167) ----------
 * Layout heap: [length:u32 BE][el0:T][el1:T]... — T=4 bytes para i32.
 * Bounds check + null check; en fallo invocamos throw_runtime (que hoy
 * solo reporta a stderr — la integración con try/catch BP vendrá con
 * #175). */
static int32_t h_array_load_i32(bpvm_t* vm, uint32_t ref, int32_t idx) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_i32: null array");
        return 0;
    }
    uint8_t* mem = vm->memory;
    uint32_t addr = A(vm, ref);                 /* v2: handle → offset */
    uint32_t length = bpvm_read_u32_be(mem + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_i32: index out of bounds");
        return 0;
    }
    return bpvm_read_i32_be(mem + addr + 4 + (uint32_t) idx * 4);
}
static void h_array_store_i32(bpvm_t* vm, uint32_t ref, int32_t idx, int32_t v) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_store_i32: null array");
        return;
    }
    uint8_t* mem = vm->memory;
    uint32_t addr = A(vm, ref);
    uint32_t length = bpvm_read_u32_be(mem + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_store_i32: index out of bounds");
        return;
    }
    bpvm_write_i32_be(mem + addr + 4 + (uint32_t) idx * 4, v);
}
/* V5/H4 — elementos de 8 bytes. Calcados de los de i32 salvo el paso (8 en vez
 * de 4). `double` viaja por el mismo camino que `long`: mismos bits, misma
 * anchura — sólo cambia cómo se leen. */
static uint8_t* elem8(bpvm_t* vm, uint32_t ref, int32_t idx, const char* quien) {
    uint32_t addr, length;
    if (ref == 0) { bpvm_aot_helpers_v2.throw_runtime(vm, quien); return 0; }
    addr   = A(vm, ref);
    length = bpvm_read_u32_be(vm->memory + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, quien);   /* no retorna */
        return 0;
    }
    return vm->memory + addr + 4 + (uint32_t) idx * 8u;
}
static int64_t h_array_load_i64(bpvm_t* vm, uint32_t ref, int32_t idx) {
    uint8_t* p = elem8(vm, ref, idx, "long[]: indice fuera de rango o array nulo");
    return p ? bpvm_read_i64_be(p) : 0;
}
static void h_array_store_i64(bpvm_t* vm, uint32_t ref, int32_t idx, int64_t v) {
    uint8_t* p = elem8(vm, ref, idx, "long[]: indice fuera de rango o array nulo");
    if (p) bpvm_write_i64_be(p, v);
}
static double h_array_load_f64(bpvm_t* vm, uint32_t ref, int32_t idx) {
    uint8_t* p = elem8(vm, ref, idx, "double[]: indice fuera de rango o array nulo");
    if (!p) return 0.0;
    { int64_t bits = bpvm_read_i64_be(p); double d; memcpy(&d, &bits, 8); return d; }
}
static void h_array_store_f64(bpvm_t* vm, uint32_t ref, int32_t idx, double v) {
    uint8_t* p = elem8(vm, ref, idx, "double[]: indice fuera de rango o array nulo");
    if (p) { int64_t bits; memcpy(&bits, &v, 8); bpvm_write_i64_be(p, bits); }
}

static int32_t h_array_length(bpvm_t* vm, uint32_t ref) {
    if (ref == 0) return 0;   /* null array → length 0 (BP semantics) */
    return (int32_t) bpvm_read_u32_be(vm->memory + A(vm, ref));
}

/* #193 — arrays narrow de 1 byte (BP byte[]). Layout: [length:u32 BE][b0][b1]...
 * Mismo bounds/extensión que OP_ALOAD_I8/OP_ALOAD_U8/OP_ASTORE_I8 del intérprete
 * (interp.c): load_i8 extiende con signo, load_u8 con cero, store_i8 trunca. */
static int32_t h_array_load_i8(bpvm_t* vm, uint32_t ref, int32_t idx) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_i8: null array");
        return 0;
    }
    uint8_t* mem = vm->memory;
    uint32_t addr = A(vm, ref);                 /* v2: handle → offset */
    uint32_t length = bpvm_read_u32_be(mem + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_i8: index out of bounds");
        return 0;
    }
    return (int32_t)(int8_t) mem[addr + 4 + (uint32_t) idx];   /* sign-extend */
}
static int32_t h_array_load_u8(bpvm_t* vm, uint32_t ref, int32_t idx) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_u8: null array");
        return 0;
    }
    uint8_t* mem = vm->memory;
    uint32_t addr = A(vm, ref);
    uint32_t length = bpvm_read_u32_be(mem + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_u8: index out of bounds");
        return 0;
    }
    return (int32_t)(uint8_t) mem[addr + 4 + (uint32_t) idx];  /* zero-extend */
}
/* [V6/N1.5] Los de 16 bits, que FALTABAN — y su ausencia no daba error: el
 * emisor caia al helper de i32 y leia CUATRO bytes donde hay dos. Ver la nota
 * de `arrElemKind` en AotCEmitter: el limite estaba escrito («v1: solo
 * integer[]») y no lo hacia cumplir nadie. Un limite que no falla no es un
 * limite: es un numero equivocado.
 *
 * Dos cargas y un solo store, igual que los de 8 bits: la extension de signo
 * distingue `short` de `word`, la truncacion del store no. Los `u16`/`i16` van
 * en BIG-ENDIAN dentro del array, como los escribe el interprete. */
static int32_t h_array_load_i16(bpvm_t* vm, uint32_t ref, int32_t idx) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_i16: null array");
        return 0;
    }
    uint8_t* mem = vm->memory;
    uint32_t addr = A(vm, ref);
    uint32_t length = bpvm_read_u32_be(mem + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_i16: index out of bounds");
        return 0;
    }
    return (int32_t) bpvm_read_i16_be(mem + addr + 4 + (uint32_t) idx * 2u);
}
static int32_t h_array_load_u16(bpvm_t* vm, uint32_t ref, int32_t idx) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_u16: null array");
        return 0;
    }
    uint8_t* mem = vm->memory;
    uint32_t addr = A(vm, ref);
    uint32_t length = bpvm_read_u32_be(mem + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_u16: index out of bounds");
        return 0;
    }
    return (int32_t)(uint16_t) bpvm_read_i16_be(mem + addr + 4 + (uint32_t) idx * 2u);
}
static void h_array_store_i16(bpvm_t* vm, uint32_t ref, int32_t idx, int32_t v) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_store_i16: null array");
        return;
    }
    uint8_t* mem = vm->memory;
    uint32_t addr = A(vm, ref);
    uint32_t length = bpvm_read_u32_be(mem + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_store_i16: index out of bounds");
        return;
    }
    /* Big-endian byte a byte, igual que OP_ASTORE_I16: no hay write_u16_be. */
    uint8_t* e = mem + addr + 4 + (uint32_t) idx * 2u;
    e[0] = (uint8_t)((v >> 8) & 0xFF);
    e[1] = (uint8_t)(v & 0xFF);
}

/* ---------- [V6/N1.5b] ARRAYS DE REFERENCIAS ----------
 *
 * Estos faltaban por un motivo que resulto ser FALSO. Lo escrito era que un
 * `TYPE_ARRAY_REF` guarda handles de 64 bits CON generacion y que en esta ABI
 * una ref cruza con la generacion DESCARTADA — leido como si fuera perdida de
 * informacion. No lo es, y lo corrigio Eduardo el 24-ago-2026: *«desde V5 todas
 * las referencias a objetos, arrays y strings deberian poder pasarse tal cual»*.
 *
 * `bpref_regen(vm, ref)` RECONSTRUYE la generacion viva consultando
 * `vm->handle_gen[idx]`. Es un limite de TRANSPORTE (no se lleva), no de
 * CAPACIDAD (no se puede recuperar). Y de hecho estas dos lineas ya estaban
 * escritas dos veces en este mismo fichero: `h_read_ref` y `h_write_ref`, la
 * frontera del thunk desde #302.
 *
 * Un elemento ocupa BPVM_REF_SIZE (8 bytes) y lleva el handle COMPLETO — que es
 * lo que el GC necesita para trazarlo, y por eso un array de refs no puede ser
 * un TYPE_ARRAY_I64: seria 8 bytes opacos que el GC no mira. Los opcodes que
 * hace el interprete son ALOAD_I64/ASTORE_I64 sobre un array creado por
 * BUILTIN_NEW_REF_ARRAY; esto es exactamente eso. */
static int32_t h_newarray_ref(bpvm_t* vm, int32_t size) {
    return h_newarray_tipo(vm, size, BPVM_REF_SIZE, BPVM_TYPE_ARRAY_REF);
}
static int32_t h_array_load_ref(bpvm_t* vm, uint32_t ref, int32_t idx) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_ref: null array");
        return 0;
    }
    uint32_t addr = A(vm, ref);
    uint32_t length = bpvm_read_u32_be(vm->memory + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_load_ref: index out of bounds");
        return 0;
    }
    /* El elemento guarda el handle de 64b; al codigo native le va la palabra
     * baja (idx|TAG), que es lo que dice el convenio de esta ABI. */
    return (int32_t) bpvm_read_i64_be(vm->memory + addr + BPVM_ARR_DATA_OFF
                                      + (uint32_t) idx * BPVM_REF_SIZE);
}
static void h_array_store_ref(bpvm_t* vm, uint32_t ref, int32_t idx, int32_t v) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_store_ref: null array");
        return;
    }
    uint32_t addr = A(vm, ref);
    uint32_t length = bpvm_read_u32_be(vm->memory + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_store_ref: index out of bounds");
        return;
    }
    /* La gen VIVA se reconstruye aqui: es la frontera. Guardar la palabra baja
     * a secas dejaria un handle con gen 0 — y un gen 0 no casa con nada, asi que
     * el objeto se leeria como muerto (use-after-free) en el primer acceso. */
    bpvm_write_i64_be(vm->memory + addr + BPVM_ARR_DATA_OFF
                      + (uint32_t) idx * BPVM_REF_SIZE,
                      (int64_t) bpref_regen(vm, (uint32_t) v).v);
}

/* ---------- [V6/N1.5b] `float[]` ----------
 * Cuatro bytes por casilla, como `integer[]` (el interprete usa NEWARRAY para
 * los dos). Lo que faltaba no era el alocador: era mover el PATRON DE BITS sin
 * que pase por un int32 por el camino. Gemelos de read_f32_be/write_f32_be. */
static float h_array_load_f32(bpvm_t* vm, uint32_t ref, int32_t idx) {
    return aoth_bits_to_float((uint32_t) h_array_load_i32(vm, ref, idx));
}
static void h_array_store_f32(bpvm_t* vm, uint32_t ref, int32_t idx, float v) {
    h_array_store_i32(vm, ref, idx, (int32_t) aoth_float_to_bits(v));
}

static void h_array_store_i8(bpvm_t* vm, uint32_t ref, int32_t idx, int32_t v) {
    if (ref == 0) {
        if (vm) bpvm_aot_helpers_v2.throw_runtime(vm, "array_store_i8: null array");
        return;
    }
    uint8_t* mem = vm->memory;
    uint32_t addr = A(vm, ref);
    uint32_t length = bpvm_read_u32_be(mem + addr);
    if (idx < 0 || (uint32_t) idx >= length) {
        bpvm_aot_helpers_v2.throw_runtime(vm, "array_store_i8: index out of bounds");
        return;
    }
    mem[addr + 4 + (uint32_t) idx] = (uint8_t)(v & 0xFF);      /* truncate */
}

/* ---------- Builtins (H3 #168) ----------
 * Wrappers de los OP_CALL_BUILTIN más usados, callables como C
 * directo desde código AOT. */
extern int64_t bpvm_platform_now_ms(void);
static int32_t h_now_ms(bpvm_t* vm) {
    (void) vm;
    return (int32_t) bpvm_platform_now_ms();
}

/* ---------- Variables nivel-módulo (#172) ----------
 * Las module-globals son INTERNAS al módulo (no se exportan en la
 * sección 4.2 del .mod). Su dirección absoluta es CS + offset, donde
 * offset es fijo en tiempo de compilación (lo decide el bytecode
 * emitter y AotCEmitter lo bakea como literal C). El CS sí es runtime,
 * por lo que cada thunk resuelve UNA vez la CS del módulo al que
 * pertenece y la cachea.
 *
 * Acceso al campo `modules[]` directo: el `name` de bpvm_module_t es
 * el nombre lógico ("GlobalsAot") sin librería. */
static uint32_t h_find_module_cs(bpvm_t* vm, const char* module_name) {
    if (!vm || !module_name) return 0;
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (strcmp(m->name, module_name) == 0) {
            return m->code_start;
        }
    }
    return 0;
}

/* ---- Puente native→BP (P-aot-call-bp) ----
 * find_function: resuelve un nombre cualificado a su dirección absoluta vía
 * la tabla global de símbolos (la misma que usa bpvm_aot_register_by_name).
 * Devuelve 0 si no existe. El thunk lo llama UNA vez y cachea el resultado.
 * (call_bp_i32 vive en interp.c — necesita las convenciones de frame.) */
static uint32_t h_find_function(bpvm_t* vm, const char* qualified) {
    if (!vm || !qualified) return 0;
    for (int i = 0; i < vm->symbol_count; i++) {
        if (strcmp(vm->symbols[i].name, qualified) == 0) {
            return vm->symbols[i].abs_addr;
        }
    }
    return 0;
}

/* ---------- Strings (H3 #173, H2 V2) ----------
 * String heap = [byte_len:u32 BE][bytes UTF-8] (TYPE_ARRAY_I8). Índice por
 * codepoint vía helpers utf8_* (bpvm_internal.h). Reusan bpvm_heap_alloc /
 * bpvm_heap_alloc_string del runtime. DEBEN coincidir con builtins.c. */
/* v2: los `ref` de entrada son handles empaquetados → A(vm,ref) los resuelve.
 * Los que ALOCAN (char_at/concat/substring) llaman bpvm_heap_alloc (offset crudo)
 * y devuelven el handle EMPAQUETADO (bpvm_handle_register). La alocación es un
 * safepoint: re-derefiamos los inputs DESPUÉS de alocar (regla de la dirección
 * transitoria; en F2 no compacta, pero mantiene la disciplina del modelo). */
static int32_t h_string_length(bpvm_t* vm, uint32_t ref) {
    if (ref == 0) return 0;
    uint32_t addr = A(vm, ref);
    uint32_t nbytes = bpvm_read_u32_be(vm->memory + addr);
    return (int32_t) utf8_cp_count(vm->memory + addr + 4, nbytes);
}
static int32_t h_string_char_code_at(bpvm_t* vm, uint32_t ref, int32_t idx) {
    if (ref == 0) return 0;
    uint32_t addr = A(vm, ref);
    uint32_t nbytes = bpvm_read_u32_be(vm->memory + addr);
    const uint8_t* p = vm->memory + addr + 4;
    uint32_t ncp = utf8_cp_count(p, nbytes);
    if (idx < 0 || (uint32_t) idx >= ncp) return 0;
    uint32_t off = utf8_byte_offset(p, nbytes, (uint32_t) idx);
    uint32_t adv; return (int32_t) utf8_decode(p + off, nbytes - off, &adv);
}
static uint32_t h_string_char_at(bpvm_t* vm, uint32_t ref, int32_t idx) {
    uint8_t enc[4]; uint32_t enc_len = 0;
    if (ref != 0) {
        uint32_t addr = A(vm, ref);
        uint32_t nbytes = bpvm_read_u32_be(vm->memory + addr);
        const uint8_t* p = vm->memory + addr + 4;
        uint32_t ncp = utf8_cp_count(p, nbytes);
        if (idx >= 0 && (uint32_t) idx < ncp) {
            uint32_t off = utf8_byte_offset(p, nbytes, (uint32_t) idx);
            uint32_t adv; uint32_t cp = utf8_decode(p + off, nbytes - off, &adv);
            enc_len = utf8_encode(cp, enc);
        }
    }
    uint32_t out = bpvm_heap_alloc(vm, enc_len, BPVM_TYPE_ARRAY_I8);
    if (!out) return 0;
    bpvm_write_u32_be(vm->memory + out, enc_len);
    for (uint32_t k = 0; k < enc_len; k++) vm->memory[out + 4 + k] = enc[k];
    return (uint32_t) bpvm_handle_register(vm, out).v;
}
static uint32_t h_string_concat(bpvm_t* vm, uint32_t a, uint32_t b) {
    uint32_t aa = a ? A(vm, a) : 0, ba = b ? A(vm, b) : 0;
    uint32_t la = aa ? bpvm_read_u32_be(vm->memory + aa) : 0;   /* bytes */
    uint32_t lb = ba ? bpvm_read_u32_be(vm->memory + ba) : 0;
    uint32_t out = bpvm_heap_alloc(vm, la + lb, BPVM_TYPE_ARRAY_I8);
    if (!out) return 0;
    aa = a ? A(vm, a) : 0; ba = b ? A(vm, b) : 0;   /* re-deref tras el safepoint */
    uint8_t* mem = vm->memory;
    bpvm_write_u32_be(mem + out, la + lb);
    for (uint32_t i = 0; i < la; i++) mem[out + 4 + i] = mem[aa + 4 + i];
    for (uint32_t i = 0; i < lb; i++) mem[out + 4 + la + i] = mem[ba + 4 + i];
    return (uint32_t) bpvm_handle_register(vm, out).v;
}
static uint32_t h_string_substring(bpvm_t* vm, uint32_t ref, int32_t from, int32_t to) {
    if (ref == 0) return 0;
    uint32_t addr = A(vm, ref);
    uint32_t nbytes = bpvm_read_u32_be(vm->memory + addr);
    const uint8_t* p = vm->memory + addr + 4;
    uint32_t ncp = utf8_cp_count(p, nbytes);   /* índices en codepoints */
    if (from < 0) from = 0;
    if (to < 0)   to = 0;
    if ((uint32_t) to > ncp) to = (int32_t) ncp;
    if (from > to) from = to;
    uint32_t boff = utf8_byte_offset(p, nbytes, (uint32_t) from);
    uint32_t eoff = utf8_byte_offset(p, nbytes, (uint32_t) to);
    uint32_t n = eoff - boff;
    uint32_t out = bpvm_heap_alloc(vm, n, BPVM_TYPE_ARRAY_I8);
    if (!out) return 0;
    addr = A(vm, ref);   /* re-deref tras el safepoint */
    uint8_t* mem = vm->memory;
    bpvm_write_u32_be(mem + out, n);
    for (uint32_t i = 0; i < n; i++) mem[out + 4 + i] = mem[addr + 4 + boff + i];
    return (uint32_t) bpvm_handle_register(vm, out).v;
}
static int32_t h_string_eq(bpvm_t* vm, uint32_t a, uint32_t b) {
    if (a == b) return 1;              /* mismo handle (incl. ambos null) */
    if (a == 0 || b == 0) return 0;
    uint8_t* mem = vm->memory;
    uint32_t aa = A(vm, a), ba = A(vm, b);
    uint32_t la = bpvm_read_u32_be(mem + aa);   /* bytes */
    uint32_t lb = bpvm_read_u32_be(mem + ba);
    if (la != lb) return 0;
    for (uint32_t i = 0; i < la; i++)
        if (mem[aa + 4 + i] != mem[ba + 4 + i]) return 0;
    return 1;
}
/* ── V5/H4 — la INVERSA de string_from_cstr ────────────────────────────────
 *
 * Hacía falta para poder pasarle una cadena BP a una función de C: una cadena
 * BP es un array de bytes CON LONGITUD y sin NUL, y todo C quiere `const char*`.
 *
 * NO TRUNCA EN SILENCIO. Si no cabe devuelve -1 y deja el buffer vacío. Truncar
 * un camino de fichero abriría OTRO fichero — un fallo que no se parece a un
 * fallo, que es la peor clase. */
static int32_t h_string_to_cstr(bpvm_t* vm, uint32_t ref, char* dst, int32_t cap) {
    uint32_t addr, nbytes, i;
    if (dst == 0 || cap <= 0) return -1;
    dst[0] = 0;
    if (ref == 0) return 0;                       /* cadena nula -> "" */
    addr   = A(vm, ref);
    nbytes = bpvm_read_u32_be(vm->memory + addr);
    if ((int32_t) nbytes >= cap) return -1;       /* +1 para el NUL */
    for (i = 0; i < nbytes; i++) dst[i] = (char) vm->memory[addr + 4 + i];
    dst[nbytes] = 0;
    return (int32_t) nbytes;
}

/* ── V5/H4 — resolver un símbolo de un pack POR NOMBRE ─────────────────────
 *
 * Es TODO lo que la VM sabe de los packs: busca la tabla por su marca,
 * comprueba que habla la versión que se le pide (gate grueso) y devuelve el
 * símbolo (gate fino). No conoce SQLite ni conocerá LVGL.
 *
 * NULL significa «ese pack no está grabado» o «no trae esa función», y es lo
 * que el thunk convierte en una excepción BP CON NOMBRE. Sin este NULL, un pack
 * que falta sería un salto a ninguna parte. */
static bpvm_pack_fn_t h_pack_sym(uint32_t marca, uint32_t version, const char* nombre) {
    const bpvm_pack_api_t* api = (const bpvm_pack_api_t*) bpvm_bios_busca(marca);
    if (bpvm_pack_api_verify(api, marca, version) != 0) return 0;
    return bpvm_pack_api_find(api, nombre);
}

/* V5/H4 — redacta y lanza. NO retorna (longjmp al boundary del intérprete).
 * El porqué de que el texto viva aquí y no en el código generado, en la
 * cabecera: el `.mdn` no se lleva `.rodata`. */
static void h_pack_fallo(bpvm_t* vm, uint32_t marca, uint32_t version,
                         const char* nombre, int32_t motivo) {
    char m[4], buf[200];
    m[0] = (char) ((marca >> 24) & 0xFF); m[1] = (char) ((marca >> 16) & 0xFF);
    m[2] = (char) ((marca >>  8) & 0xFF); m[3] = (char) ( marca        & 0xFF);
    if (nombre == 0) nombre = "?";
    if (motivo == 2) {
        snprintf(buf, sizeof buf,
                 "pack '%.4s': el texto que se le pasa a '%s' no cabe en el puente"
                 " (%d bytes como mucho)", m, nombre, BPVM_AOT_PACK_STR);
    } else {
        snprintf(buf, sizeof buf,
                 "pack '%.4s': no ofrece '%s'. O no esta grabado en esta placa, o"
                 " no habla la version %u, o no trae esa funcion",
                 m, nombre, (unsigned) version);
    }
    h_throw_runtime(vm, buf);   /* no retorna */
}

static uint32_t h_string_from_cstr(bpvm_t* vm, const char* s, int32_t len) {
    if (!s || len < 0) return 0;
    return bpvm_heap_alloc_string(vm, s, (size_t) len);
}
static uint32_t h_int_to_string(bpvm_t* vm, int32_t v) {
    char buf[16];
    int n = snprintf(buf, sizeof(buf), "%d", (int) v);
    return bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
}

/* ---------- Instancia exportada ----------
 * `const` para que viva en .rodata (flash en el Pico). */
const aot_helpers_v2_t bpvm_aot_helpers_v2 = {
    .read_i32_be     = h_read_i32_be,
    .write_i32_be    = h_write_i32_be,
    .read_i16_be     = h_read_i16_be,
    .write_i16_be    = h_write_i16_be,
    .throw_runtime   = h_throw_runtime,
    .newarray_i32    = h_newarray_i32,
    .newarray_i8     = h_newarray_i8,
    .newarray_i16    = h_newarray_i16,
    .new_object      = h_new_object,
    .print_i32       = h_print_i32,
    .print_f32       = h_print_f32,
    .print_string    = h_print_string,
    .print_char      = h_print_char,
    .print_nl        = h_print_nl,
    .read_f32_be     = h_read_f32_be,
    .write_f32_be    = h_write_f32_be,
    .array_load_i32  = h_array_load_i32,
    .array_store_i32 = h_array_store_i32,
    .array_length    = h_array_length,
    .now_ms          = h_now_ms,
    .find_module_cs  = h_find_module_cs,
    .string_length       = h_string_length,
    .string_char_code_at = h_string_char_code_at,
    .string_char_at      = h_string_char_at,
    .string_concat       = h_string_concat,
    .string_substring    = h_string_substring,
    .string_eq           = h_string_eq,
    .string_from_cstr    = h_string_from_cstr,
    /* V5/H4 — llamar a un pack: convertir la cadena y resolver el símbolo. */
    .string_to_cstr      = h_string_to_cstr,
    .pack_sym            = h_pack_sym,
    .pack_fallo          = h_pack_fallo,
    .array_load_i64      = h_array_load_i64,
    .array_store_i64     = h_array_store_i64,
    .array_load_f64      = h_array_load_f64,
    .array_store_f64     = h_array_store_f64,
    .int_to_string       = h_int_to_string,
    /* Puente native→BP (P-aot-call-bp). call_bp_i32 vive en interp.c. */
    .find_function       = h_find_function,
    .call_bp_i32         = bpvm_aot_call_bp_i32,
    /* #175 — throw con mensaje computado. */
    .throw_str           = h_throw_str,
    /* #174 (mitad-VM) — despacho virtual desde native. Vive en interp.c. */
    .call_method_i32     = bpvm_aot_call_method_i32,
    /* #193 — arrays narrow de 1 byte (byte[]). */
    .array_load_i8       = h_array_load_i8,
    .array_load_u8       = h_array_load_u8,
    .array_store_i8      = h_array_store_i8,
    /* #213 — throw de excepción construida (clase de usuario desde native). */
    .throw_ref           = h_throw_ref,
    /* #302 paso 2 — frontera de referencias thunk↔pila BP. */
    .read_ref            = h_read_ref,
    .write_ref           = h_write_ref,
    /* #381 — la misma frontera para `long`: 8 bytes big-endian, igual que los
     * escribe el intérprete. Son las funciones del runtime tal cual; el `.mdn`
     * no puede llamarlas por nombre y por eso pasan por aquí. */
    .read_i64_be         = h_read_i64_be,
    .write_i64_be        = h_write_i64_be,
    /* #381 — la división de 64 bits, que el micro no tiene en hardware. */
    .idiv64              = h_idiv64,
    .imod64              = h_imod64,
    /* #426 — la coma flotante de doble, que ningun micro de hoy tiene en
     * hardware. `dpow` NO se reimplementa: es bpvm_dpow, la MISMA que ejecuta
     * OP_DPOW (su algoritmo viene copiado byte a byte de VirtualMachine.java, y
     * dos copias de eso se separan solas). */
    .dadd                = h_dadd,
    .dsub                = h_dsub,
    .dmul                = h_dmul,
    .ddiv                = h_ddiv,
    .dmod                = h_dmod,
    .dneg                = h_dneg,
    .dpow                = bpvm_dpow,
    .deq                 = h_deq,
    .dneq                = h_dneq,
    .dlt                 = h_dlt,
    .dle                 = h_dle,
    .dgt                 = h_dgt,
    .dge                 = h_dge,
    .i2d                 = h_i2d,
    .d2i                 = h_d2i,
    .l2d                 = h_l2d,
    .d2l                 = h_d2l,
    .f2d                 = h_f2d,
    .d2f                 = h_d2f,
    .read_f64_be         = h_read_f64_be,
    .write_f64_be        = h_write_f64_be,
    /* [V6/N1.5] crear arrays desde native: los tres de arriba dejaron de ser
     * stubs y este es nuevo (`long[]`/`double[]`). */
    .newarray_i64        = h_newarray_i64,
    .array_load_i16      = h_array_load_i16,
    .array_load_u16      = h_array_load_u16,
    .array_store_i16     = h_array_store_i16,
    /* [V6/N1.5b] arrays de REFERENCIAS y de `float`. */
    .newarray_ref        = h_newarray_ref,
    .array_load_ref      = h_array_load_ref,
    .array_store_ref     = h_array_store_ref,
    .array_load_f32      = h_array_load_f32,
    .array_store_f32     = h_array_store_f32,
};

/* La potencia: UNA implementacion, usada por OP_DPOW y por el helper. Ver la
 * nota de arriba — es la unica de las diecinueve con algoritmo propio.
 * Exponente entero (incl. x^2) -> cuadrados en f64 (parity-safe);
 * fraccionario -> exp(e*ln base). Misma logica byte-a-byte que
 * VirtualMachine.java case 0xAE. */
double bpvm_dpow(double base, double e) {
    if (e == floor(e) && !isinf(e) && fabs(e) <= 1024.0) {
        int64_t n = (int64_t) e; int neg = (n < 0); if (neg) n = -n;
        double r = 1.0, bb = base;
        while (n > 0) { if (n & 1) r *= bb; bb *= bb; n >>= 1; }
        return neg ? 1.0 / r : r;
    }
    return exp(e * log(base));
}
