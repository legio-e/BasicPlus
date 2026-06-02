/*
 * interp.c — intérprete F1.
 *
 * Subset cubierto: opcodes 0x00..0x6C salvo los que requieren heap
 * (NEWARRAY*, ALOAD*, ASTORE*, CALL_BUILTIN, NEW_OBJECT, GET/SET_FIELD,
 * INVOKE_VIRTUAL, TRY/THROW, INSTANCEOF, FREE_REF, PRINT_STRING que
 * lee desde array). Eso queda para F2 (heap) y F3 (clases).
 *
 * Single-thread: usamos siempre vm->threads[0] (= main). El scheduler
 * propio de la VM viene en F4 con FreeRTOS.
 *
 * El intérprete trabaja sobre locales `pc`, `sp`, `bp`, `cs` que se
 * cargan/guardan al ThreadContext del thread main. Mismo patrón que
 * la VM Java.
 */

#include "bpvm_internal.h"
#include "bpvm_opcodes.h"
#include "aot_registry.h"   /* H3 #160: hijack BP→AOT en OP_CALL/CALL_EXT */

#include "bpvm_comm.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>      /* fmodf */

/* Helper: emite un texto al sink correcto.
 *
 * Orden de prioridad:
 *   1. Modo SMP — encola en la output queue. El comm task drena y, EN
 *      HILO ÚNICO, invoca output_cb (o fwrite a stdout si no hay). Esto
 *      serializa: dos workers no entrelazan ni fwrite ni una llamada al
 *      callback (importante en Pico, donde output_cb hace wire-v1
 *      wrapping con varios fputs por chunk). oq_push es atómico por
 *      llamada (toma el mutex de la queue), así que cada emit_text sale
 *      contiguo. NOTA: la atomicidad de LÍNEA completa (bufferizar
 *      per-tc hasta '\n') se intentó en #185 pero infló bpvm_t ~4 KB
 *      (×32 threads) y reventó la RAM de la Pico → revertido. Si se
 *      quiere, hacerlo per-worker en bpvm_smp_t (2×128 B), no per-thread.
 *   2. Modo legacy single-worker — si hay callback, va ahí; si no,
 *      fwrite directo. Sin overhead extra para el camino existente.
 */
static void emit_text(bpvm_t* vm, const char* s, size_t len) {
    if (vm->smp) {
        bpvm_comm_output_enqueue(vm, s, len);
    } else if (vm->output_cb) {
        vm->output_cb(s, len, vm->output_user);
    } else {
        fwrite(s, 1, len, stdout);
    }
}

static void emit_int(bpvm_t* vm, int32_t v, int newline) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), newline ? "%" PRId32 "\n" : "%" PRId32, v);
    if (n > 0) emit_text(vm, buf, (size_t) n);
}

/* H1.2 (V2): print de un long (i64). %lld + cast a long long para portabilidad. */
static void emit_long(bpvm_t* vm, int64_t v, int newline) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), newline ? "%lld\n" : "%lld", (long long) v);
    if (n > 0) emit_text(vm, buf, (size_t) n);
}

static void emit_newline(bpvm_t* vm) {
    emit_text(vm, "\n", 1);
}

/* #186 — invoca un thunk AOT con un boundary de fault armado.
 *
 * El setjmp vive AQUÍ (no en bpvm_interp_run_quantum) a propósito: si
 * estuviera en el intérprete, sus registros calientes (pc/sp/bp/cs/mem)
 * quedarían potencialmente clobbered por el longjmp (-Wclobbered) y
 * habría que marcarlos volatile, penalizando el hot loop. Con el setjmp
 * en este wrapper, el frame del intérprete nunca está en juego.
 *
 * Si el native lanza (throw_runtime → longjmp), construimos el
 * RuntimeError con el mensaje del fault-slot y lo propagamos por el
 * eh_stack del thread (maquinaria F5 reutilizada). Devuelve:
 *   0 = el native terminó normal,
 *   1 = fault cazado por un try/catch BP (tc->pc/sp/bp/cs → handler),
 *   2 = fault NO cazado (eh_unwind dejó el thread TERMINATED).
 * tc->cs debe estar fijado por el caller (throw_runtime_error localiza
 * la clase RuntimeError por el cs del módulo en curso). */
static int aot_call_guarded(bpvm_t* vm, bpvm_thread_t* tc,
                            bpvm_aot_thunk_t aot, uint32_t* sp, uint32_t* bp) {
    bpvm_aot_fault_t* f = bpvm_aot_fault_slot();
    int prev_armed = f->armed;   /* leaf natives: siempre 0; defensivo si anidaran */
    if (setjmp(f->buf) == 0) {
        f->armed = 1;
        aot(vm, sp, bp);
        f->armed = prev_armed;
        return 0;
    }
    f->armed = prev_armed;
    uint32_t obj = bpvm_throw_runtime_error(vm, tc, f->msg);
    return bpvm_eh_unwind(vm, tc, obj) ? 1 : 2;
}

/* GAP-4 — formateo canónico de double/float para print. Punto fijo estilo
 * Str.doubleToString (entero-based: escala por 1e6 a un int64, redondea, separa
 * parte entera/decimal, recorta ceros). Magnitudes fuera del rango seguro del
 * int64 (|x| >= 1e12 o 0 < |x| < 1e-6) → notación científica. TODO en
 * aritmética IEEE determinista (solo *,/,+ por literales exactos + cast a
 * int64) y %lld/%06lld para las piezas enteras → byte-idéntico a
 * VirtualMachine.formatBpDouble (Java). NO usa %g/%f de printf sobre el float. */
static int bpvm_format_double(char* out, double v) {
    if (isnan(v)) { memcpy(out, "NaN", 4); return 3; }
    if (isinf(v)) {
        if (v > 0.0) { memcpy(out, "Infinity", 9); return 8; }
        memcpy(out, "-Infinity", 10); return 9;
    }
    int neg = (v < 0.0);
    double ax = neg ? -v : v;
    if (ax == 0.0) { out[0] = '0'; out[1] = '\0'; return 1; }

    if (ax >= 1e12 || ax < 1e-6) {
        /* científico */
        int e = 0;
        double m = ax;
        while (m >= 10.0) { m = m / 10.0; e++; }
        while (m < 1.0)   { m = m * 10.0; e--; }
        long long scaled = (long long) (m * 1e6 + 0.5);
        if (scaled >= 10000000LL) { scaled = 1000000LL; e++; }
        long long ip = scaled / 1000000LL;
        long long fr = scaled % 1000000LL;
        char tmp[48];
        int n = snprintf(tmp, sizeof tmp, "%s%lld.%06lld", neg ? "-" : "", ip, fr);
        while (n > 0 && tmp[n - 1] == '0') n--;
        if (n > 0 && tmp[n - 1] == '.') n--;
        tmp[n] = '\0';
        return snprintf(out, 48, "%sE%d", tmp, e);
    } else {
        /* punto fijo, 6 decimales */
        long long scaled = (long long) (ax * 1e6 + 0.5);
        if (scaled == 0) neg = 0;               /* evita "-0" */
        long long ip = scaled / 1000000LL;
        long long fr = scaled % 1000000LL;
        int n = snprintf(out, 48, "%s%lld.%06lld", neg ? "-" : "", ip, fr);
        while (n > 0 && out[n - 1] == '0') n--;
        if (n > 0 && out[n - 1] == '.') n--;
        out[n] = '\0';
        return n;
    }
}

static void emit_float(bpvm_t* vm, float v, int newline) {
    char buf[52];
    int n = bpvm_format_double(buf, (double) v);
    if (n <= 0) return;
    if (newline && n + 1 < (int) sizeof(buf)) { buf[n++] = '\n'; buf[n] = '\0'; }
    emit_text(vm, buf, (size_t) n);
}

/* Helpers de reinterpretación int↔float (tipo punning seguro en C99 vía
 * memcpy; los compiladores lo optimizan a 0 instrucciones). */
static inline float bits_to_float(int32_t bits) {
    float f; memcpy(&f, &bits, 4); return f;
}
static inline int32_t float_to_bits(float f) {
    int32_t bits; memcpy(&bits, &f, 4); return bits;
}
/* H1.3 (V2): reinterpretación int64↔double (raw bits, sin conversión). */
static inline double bits_to_double(int64_t bits) {
    double d; memcpy(&d, &bits, 8); return d;
}
static inline int64_t double_to_bits(double d) {
    int64_t bits; memcpy(&bits, &d, 8); return bits;
}
/* GAP-4: print de un double via bpvm_format_double (punto fijo + sci,
 * byte-idéntico a la VM Java). */
static void emit_double(bpvm_t* vm, double v, int newline) {
    char buf[52];
    int n = bpvm_format_double(buf, v);
    if (n <= 0) return;
    if (newline && n + 1 < (int) sizeof(buf)) { buf[n++] = '\n'; buf[n] = '\0'; }
    emit_text(vm, buf, (size_t) n);
}

/* Ejecuta el thread `tc` durante hasta `max_ops` opcodes o hasta que
 * el thread ceda (yield, sleep, mutex_lock contended, join, HALT,
 * THREAD_EXIT). Setea *yielded a 1 si cedió, 0 si quedó RUNNABLE
 * (consumió el quantum y volverá a correr). El scheduler le da
 * el siguiente quantum. */
bpvm_status_t bpvm_interp_run_quantum(bpvm_t* vm, bpvm_thread_t* tc,
                                       int max_ops, int* yielded) {
    if (yielded) *yielded = 0;
    uint8_t* mem = vm->memory;

    /* Registros locales del intérprete — más rápidos que tocar tc.* */
    uint32_t pc = tc->pc;
    uint32_t sp = tc->sp;
    uint32_t bp = tc->bp;
    uint32_t cs = tc->cs;

    tc->status = BPVM_THREAD_RUNNING;
    bpvm_status_t exit_status = BPVM_OK;
    int ops = 0;

    for (;;) {
        if (ops >= max_ops) {
            /* Quantum agotado — el scheduler decide. Quedamos RUNNABLE. */
            tc->status = BPVM_THREAD_RUNNABLE;
            break;
        }
        /* H2 — Safepoint para STW GC. Si otro worker pidió GC, salimos
         * del quantum aquí. El worker_loop verá stop_the_world y se
         * parqueará en cond_wait hasta que GC termine. Coste hot-path:
         * un null-check + un volatile read si SMP, NADA si single-worker
         * (vm->smp es NULL en legacy). */
        if (vm->smp && vm->smp->stop_the_world) {
            tc->status = BPVM_THREAD_RUNNABLE;
            break;
        }
        ops++;
        if (pc >= vm->memory_size) { exit_status = BPVM_ERR_BAD_PC; break; }

        if (vm->tracing) {
            fprintf(stderr, "[trace] pc=%" PRIu32 " sp=%" PRIu32 " bp=%" PRIu32 " cs=%" PRIu32 " op=0x%02X\n",
                    pc, sp, bp, cs, mem[pc]);
        }

        /* #139 — Debug hook (cambio de línea). Coste cuando NO está
         * instalado: un único null-check por opcode. Cuando lo está:
         * un callback pc_to_line + comparación contra last_debug_line.
         * El hook puede bloquear el thread; al volver NO mutamos
         * pc/sp/bp/cs (edit-and-continue queda para v2). */
        if (vm->debug_hook != NULL) {
            int dbg_line = -1;
            const char* dbg_src = NULL;
            if (vm->debug_pc_to_line != NULL) {
                dbg_line = vm->debug_pc_to_line(pc, &dbg_src, vm->debug_user);
            } else {
                /* Modo "todo es una línea": cada opcode dispara. */
                dbg_line = (int) pc;
            }
            if (dbg_line > 0 && dbg_line != tc->last_debug_line) {
                tc->last_debug_line = dbg_line;
                /* Sincronizar registros antes del hook para que el
                 * back-end pueda inspeccionar tc.pc/sp/bp/cs. */
                tc->pc = pc; tc->sp = sp; tc->bp = bp; tc->cs = cs;
                vm->debug_hook(vm, tc, pc, dbg_line, dbg_src, vm->debug_user);
            }
        }

        uint8_t op = mem[pc++];
        switch (op) {

        case OP_HALT:
            /* HALT: termina el thread y la VM si es el main. */
            if (tc->id != 0) {
                fprintf(stderr, "[bpvm-c] HALT en thread no-main (tid=%" PRId32 ")\n", tc->id);
                exit_status = BPVM_ERR_RUNTIME;
            } else {
                exit_status = BPVM_OK;
            }
            tc->status = BPVM_THREAD_TERMINATED;
            if (yielded) *yielded = 1;
            goto done;

        case OP_THREAD_EXIT:
            /* THREAD_EXIT: termina sólo este thread. */
            tc->status = BPVM_THREAD_TERMINATED;
            if (yielded) *yielded = 1;
            goto done;

        /* ---- Push de inmediatos ---- */
        case OP_PUSH: {
            int32_t v = bpvm_read_i32_be(mem + pc); pc += 4;
            bpvm_write_i32_be(mem + sp, v); sp += 4;
            break;
        }
        case OP_PUSH_0:    bpvm_write_i32_be(mem + sp, 0);   sp += 4; break;
        case OP_PUSH_1:    bpvm_write_i32_be(mem + sp, 1);   sp += 4; break;
        case OP_PUSH_2:    bpvm_write_i32_be(mem + sp, 2);   sp += 4; break;
        case OP_PUSH_3:    bpvm_write_i32_be(mem + sp, 3);   sp += 4; break;
        case OP_PUSH_4:    bpvm_write_i32_be(mem + sp, 4);   sp += 4; break;
        case OP_PUSH_NEG1: bpvm_write_i32_be(mem + sp, -1);  sp += 4; break;

        /* ---- Aritmética entera ---- */
        case OP_ADD: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a + b); sp += 4; break; }
        case OP_SUB: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a - b); sp += 4; break; }
        case OP_MUL: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a * b); sp += 4; break; }
        case OP_DIV: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       if (b == 0) {
                           tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
                           uint32_t ref = bpvm_throw_runtime_error(vm, tc, "División por cero");
                           if (ref && bpvm_eh_unwind(vm, tc, ref)) {
                               pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
                               mem = vm->memory;
                               break;
                           }
                           exit_status = BPVM_ERR_RUNTIME;
                           if (yielded) *yielded = 1;
                           goto done;
                       }
                       bpvm_write_i32_be(mem+sp, a / b); sp += 4; break; }
        case OP_MOD: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       if (b == 0) {
                           tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
                           uint32_t ref = bpvm_throw_runtime_error(vm, tc, "Módulo por cero");
                           if (ref && bpvm_eh_unwind(vm, tc, ref)) {
                               pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
                               mem = vm->memory;
                               break;
                           }
                           exit_status = BPVM_ERR_RUNTIME;
                           if (yielded) *yielded = 1;
                           goto done;
                       }
                       bpvm_write_i32_be(mem+sp, a % b); sp += 4; break; }
        case OP_NEG: { sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, -a); sp += 4; break; }

        /* ---- Comparaciones ---- */
        case OP_EQ:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a == b ? 1 : 0); sp += 4; break; }
        case OP_NEQ: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a != b ? 1 : 0); sp += 4; break; }
        case OP_LT:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a <  b ? 1 : 0); sp += 4; break; }
        case OP_LE:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a <= b ? 1 : 0); sp += 4; break; }
        case OP_GT:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a >  b ? 1 : 0); sp += 4; break; }
        case OP_GE:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a >= b ? 1 : 0); sp += 4; break; }

        /* ---- Lógico booleano (no bitwise) ---- */
        case OP_AND: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, (a && b) ? 1 : 0); sp += 4; break; }
        case OP_OR:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                       sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, (a || b) ? 1 : 0); sp += 4; break; }
        case OP_NOT: { sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, !a ? 1 : 0); sp += 4; break; }

        /* ---- Bitwise ---- */
        case OP_BAND: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                        sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                        bpvm_write_i32_be(mem+sp, a & b); sp += 4; break; }
        case OP_BOR:  { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                        sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                        bpvm_write_i32_be(mem+sp, a | b); sp += 4; break; }
        case OP_BXOR: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                        sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                        bpvm_write_i32_be(mem+sp, a ^ b); sp += 4; break; }
        case OP_BNOT: { sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                        bpvm_write_i32_be(mem+sp, ~a); sp += 4; break; }
        case OP_SHL:   { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                         sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                         bpvm_write_i32_be(mem+sp, a << (b & 31)); sp += 4; break; }
        case OP_SHR_S: { sp -= 4; int32_t b = bpvm_read_i32_be(mem+sp);
                         sp -= 4; int32_t a = bpvm_read_i32_be(mem+sp);
                         bpvm_write_i32_be(mem+sp, a >> (b & 31)); sp += 4; break; }
        case OP_SHR_U: { sp -= 4; uint32_t b = bpvm_read_u32_be(mem+sp);
                         sp -= 4; uint32_t a = bpvm_read_u32_be(mem+sp);
                         bpvm_write_u32_be(mem+sp, a >> (b & 31)); sp += 4; break; }

        /* ---- Locales y globales ---- */
        case OP_GET_LOCAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            int32_t v = bpvm_read_i32_be(mem + (uint32_t)((int32_t)bp + soff));
            bpvm_write_i32_be(mem + sp, v); sp += 4; break;
        }
        case OP_SET_LOCAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + (uint32_t)((int32_t)bp + soff), v);
            break;
        }
        case OP_GET_LOCAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            int32_t v = bpvm_read_i32_be(mem + (uint32_t)((int32_t)bp + soff));
            bpvm_write_i32_be(mem + sp, v); sp += 4; break;
        }
        case OP_SET_LOCAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + (uint32_t)((int32_t)bp + soff), v);
            break;
        }
        case OP_LEA_LOCAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            bpvm_write_i32_be(mem + sp, (int32_t)((int32_t)bp + soff));
            sp += 4; break;
        }
        case OP_LEA_LOCAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            bpvm_write_i32_be(mem + sp, (int32_t)((int32_t)bp + soff));
            sp += 4; break;
        }
        case OP_GET_GLOBAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            int32_t v = bpvm_read_i32_be(mem + (uint32_t)((int32_t)cs + soff));
            bpvm_write_i32_be(mem + sp, v); sp += 4; break;
        }
        case OP_SET_GLOBAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + (uint32_t)((int32_t)cs + soff), v);
            break;
        }
        case OP_GET_GLOBAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            int32_t v = bpvm_read_i32_be(mem + (uint32_t)((int32_t)cs + soff));
            bpvm_write_i32_be(mem + sp, v); sp += 4; break;
        }
        case OP_SET_GLOBAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + (uint32_t)((int32_t)cs + soff), v);
            break;
        }
        case OP_LEA_GLOBAL: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            bpvm_write_i32_be(mem + sp, (int32_t)((int32_t)cs + soff));
            sp += 4; break;
        }
        case OP_LEA_GLOBAL_S8: {
            int8_t soff = (int8_t) mem[pc++];
            bpvm_write_i32_be(mem + sp, (int32_t)((int32_t)cs + soff));
            sp += 4; break;
        }

        /* ---- Control flow ----
         *
         * Convención CRÍTICA (que difiere del estilo JVM): los offsets de
         * los jumps son RELATIVOS AL INSTRUCTION ADDRESS (= pc del byte
         * opcode), no al pc post-operando. La VM Java los implementa así
         * desde el principio y los .mod ya emitidos asumen esta
         * convención. Ver VirtualMachine.java case 0x0D para referencia.
         */
        case OP_JUMP: {
            uint32_t instr_addr = pc - 1;
            int32_t rel = bpvm_read_i32_be(mem + pc);  /* pc NO avanza */
            pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint32_t instr_addr = pc - 1;
            int32_t rel = bpvm_read_i32_be(mem + pc); pc += 4;
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            if (v == 0) pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP8: {
            uint32_t instr_addr = pc - 1;
            int8_t rel = (int8_t) mem[pc];   /* pc NO avanza */
            pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP_IF_FALSE8: {
            uint32_t instr_addr = pc - 1;
            int8_t rel = (int8_t) mem[pc++];
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            if (v == 0) pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP16: {
            uint32_t instr_addr = pc - 1;
            int16_t rel = bpvm_read_i16_be(mem + pc);  /* pc NO avanza */
            pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }
        case OP_JUMP_IF_FALSE16: {
            uint32_t instr_addr = pc - 1;
            int16_t rel = bpvm_read_i16_be(mem + pc); pc += 2;
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            if (v == 0) pc = (uint32_t)((int32_t) instr_addr + rel);
            break;
        }

        /* ---- Calls + frame setup ---- */
        case OP_CALL: {
            int32_t target_rel = bpvm_read_i32_be(mem + pc); pc += 4;
            uint32_t target_abs = cs + (uint32_t) target_rel;
            /* H3 #160 — AOT hijack. Si el target tiene un thunk
             * registrado, divertir SIN crear frame BP. */
            bpvm_aot_thunk_t aot = bpvm_aot_lookup(target_abs);
            if (aot) {
                /* #186 — boundary de fault. Si el native lanza, el wrapper
                 * propaga al try/catch BP que envuelva la llamada. */
                tc->cs = cs;
                int r = aot_call_guarded(vm, tc, aot, &sp, &bp);
                if (r != 0) {
                    /* Fault: recargamos regs desde tc (eh_unwind los fijó
                     * al handler en r==1; en r==2 el thread está muerto). */
                    pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
                    if (r == 2) {
                        exit_status = BPVM_ERR_RUNTIME;
                        if (yielded) *yielded = 1;
                        goto done;
                    }
                }
                break;
            }
            /* BP estándar: push saved pc/bp/cs, fija bp = sp, salta */
            bpvm_write_i32_be(mem + sp, (int32_t) pc); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) bp); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) cs); sp += 4;
            bp = sp;
            pc = target_abs;
            break;
        }
        case OP_RET: {
            uint8_t params_count = mem[pc++];
            /* Convención: el return value lo dejó el callee POPEABLE en el
             * top del stack del callee (no en [bp + 0] como sugería un
             * comentario obsoleto). Mi RET hace POP, restaura cs/bp/pc,
             * ajusta sp al pre-args del caller y push del ret_val. Mismo
             * comportamiento que VirtualMachine.java case 0x08. */
            sp -= 4; int32_t ret_val = bpvm_read_i32_be(mem + sp);
            int32_t saved_pc = bpvm_read_i32_be(mem + bp - 12);
            int32_t saved_bp = bpvm_read_i32_be(mem + bp - 8);
            int32_t saved_cs = bpvm_read_i32_be(mem + bp - 4);
            uint32_t target_caller_sp = bp - 12 - (uint32_t) params_count * 4;
            pc = (uint32_t) saved_pc;
            bp = (uint32_t) saved_bp;
            cs = (uint32_t) saved_cs;
            sp = target_caller_sp;
            bpvm_write_i32_be(mem + sp, ret_val);
            sp += 4;
            break;
        }
        case OP_LRET: {  /* H1.2 (V2): RET con return value de 8 bytes (long). */
            uint8_t params_count = mem[pc++];   /* slot-count de params */
            sp -= 8; int64_t ret_val = bpvm_read_i64_be(mem + sp);
            int32_t saved_pc = bpvm_read_i32_be(mem + bp - 12);
            int32_t saved_bp = bpvm_read_i32_be(mem + bp - 8);
            int32_t saved_cs = bpvm_read_i32_be(mem + bp - 4);
            uint32_t target_caller_sp = bp - 12 - (uint32_t) params_count * 4;
            pc = (uint32_t) saved_pc;
            bp = (uint32_t) saved_bp;
            cs = (uint32_t) saved_cs;
            sp = target_caller_sp;
            bpvm_write_i64_be(mem + sp, ret_val);
            sp += 8;
            break;
        }
        case OP_ENTER: {
            uint16_t bytes = bpvm_read_u16_be(mem + pc); pc += 2;
            sp += bytes;
            break;
        }

        /* ---- Output ---- */
        case OP_PRINT: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            emit_int(vm, v, /*newline=*/1);
            break;
        }
        case OP_PRINT_NONL: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            emit_int(vm, v, /*newline=*/0);
            break;
        }
        case OP_PRINT_NL:
            emit_newline(vm);
            break;
        case OP_PRINT_CHAR: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            char c = (char)(v & 0xFF);
            emit_text(vm, &c, 1);
            break;
        }

        /* ---- Float ops (0x28..0x37, 0x57) — paridad con VM Java. ---- */
        case OP_FPUSH: {
            int32_t bits = bpvm_read_i32_be(mem + pc); pc += 4;
            bpvm_write_i32_be(mem + sp, bits); sp += 4;
            break;
        }
        case OP_FADD: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, float_to_bits(a + b)); sp += 4;
            break;
        }
        case OP_FSUB: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, float_to_bits(a - b)); sp += 4;
            break;
        }
        case OP_FMUL: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, float_to_bits(a * b)); sp += 4;
            break;
        }
        case OP_FDIV: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, float_to_bits(a / b)); sp += 4;
            break;
        }
        case OP_FMOD: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, float_to_bits(fmodf(a, b))); sp += 4;
            break;
        }
        case OP_FNEG: {
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, float_to_bits(-a)); sp += 4;
            break;
        }
        case OP_FEQ: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, a == b ? 1 : 0); sp += 4;
            break;
        }
        case OP_FNEQ: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, a != b ? 1 : 0); sp += 4;
            break;
        }
        case OP_FLT: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, a <  b ? 1 : 0); sp += 4;
            break;
        }
        case OP_FLE: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, a <= b ? 1 : 0); sp += 4;
            break;
        }
        case OP_FGT: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, a >  b ? 1 : 0); sp += 4;
            break;
        }
        case OP_FGE: {
            sp -= 4; float b = bits_to_float(bpvm_read_i32_be(mem + sp));
            sp -= 4; float a = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, a >= b ? 1 : 0); sp += 4;
            break;
        }
        case OP_FPRINT: {
            sp -= 4; float v = bits_to_float(bpvm_read_i32_be(mem + sp));
            emit_float(vm, v, /*newline=*/1);
            break;
        }
        case OP_FPRINT_NONL: {
            sp -= 4; float v = bits_to_float(bpvm_read_i32_be(mem + sp));
            emit_float(vm, v, /*newline=*/0);
            break;
        }
        case OP_I2F: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + sp, float_to_bits((float) v)); sp += 4;
            break;
        }
        case OP_F2I: {
            sp -= 4; float fv = bits_to_float(bpvm_read_i32_be(mem + sp));
            bpvm_write_i32_be(mem + sp, (int32_t) fv); sp += 4;
            break;
        }

        /* ---- Casts narrow (sin runtime check para no RuntimeError-ear
                en F1; F5 lo añade) ---- */
        case OP_I32_TO_I8: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            int32_t r = (int32_t)(int8_t)(v & 0xFF);
            bpvm_write_i32_be(mem + sp, r); sp += 4; break;
        }
        case OP_I32_TO_U8: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + sp, v & 0xFF); sp += 4; break;
        }
        case OP_I32_TO_I16: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            int32_t r = (int32_t)(int16_t)(v & 0xFFFF);
            bpvm_write_i32_be(mem + sp, r); sp += 4; break;
        }
        case OP_I32_TO_U16: {
            sp -= 4; int32_t v = bpvm_read_i32_be(mem + sp);
            bpvm_write_i32_be(mem + sp, v & 0xFFFF); sp += 4; break;
        }

        /* ---- H1.2 (V2): long (i64). 8 bytes / 2 slots; paridad VM Java. ---- */
        case OP_LPUSH: {
            int64_t v = bpvm_read_i64_be(mem + pc); pc += 8;
            bpvm_write_i64_be(mem + sp, v); sp += 8; break;
        }
        case OP_LADD: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                        sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                        bpvm_write_i64_be(mem+sp, a + b); sp += 8; break; }
        case OP_LSUB: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                        sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                        bpvm_write_i64_be(mem+sp, a - b); sp += 8; break; }
        case OP_LMUL: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                        sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                        bpvm_write_i64_be(mem+sp, a * b); sp += 8; break; }
        case OP_LDIV: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                        sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                        if (b == 0) {
                            tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
                            uint32_t ref = bpvm_throw_runtime_error(vm, tc, "División por cero");
                            if (ref && bpvm_eh_unwind(vm, tc, ref)) {
                                pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
                                mem = vm->memory; break;
                            }
                            exit_status = BPVM_ERR_RUNTIME;
                            if (yielded) *yielded = 1;
                            goto done;
                        }
                        bpvm_write_i64_be(mem+sp, a / b); sp += 8; break; }
        case OP_LMOD: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                        sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                        if (b == 0) {
                            tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
                            uint32_t ref = bpvm_throw_runtime_error(vm, tc, "Módulo por cero");
                            if (ref && bpvm_eh_unwind(vm, tc, ref)) {
                                pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
                                mem = vm->memory; break;
                            }
                            exit_status = BPVM_ERR_RUNTIME;
                            if (yielded) *yielded = 1;
                            goto done;
                        }
                        bpvm_write_i64_be(mem+sp, a % b); sp += 8; break; }
        case OP_LNEG: { sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                        bpvm_write_i64_be(mem+sp, -a); sp += 8; break; }
        case OP_LBAND: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                         sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                         bpvm_write_i64_be(mem+sp, a & b); sp += 8; break; }
        case OP_LBOR: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                        sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                        bpvm_write_i64_be(mem+sp, a | b); sp += 8; break; }
        case OP_LBXOR: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                         sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                         bpvm_write_i64_be(mem+sp, a ^ b); sp += 8; break; }
        case OP_LBNOT: { sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                         bpvm_write_i64_be(mem+sp, ~a); sp += 8; break; }
        case OP_LSHL: { sp -= 8; int64_t n = bpvm_read_i64_be(mem+sp);
                        sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                        bpvm_write_i64_be(mem+sp, (int64_t)((uint64_t) a << (n & 63))); sp += 8; break; }
        case OP_LSHR_S: { sp -= 8; int64_t n = bpvm_read_i64_be(mem+sp);
                          sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                          bpvm_write_i64_be(mem+sp, a >> (n & 63)); sp += 8; break; }
        case OP_LSHR_U: { sp -= 8; int64_t n = bpvm_read_i64_be(mem+sp);
                          sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                          bpvm_write_i64_be(mem+sp, (int64_t)((uint64_t) a >> (n & 63))); sp += 8; break; }
        case OP_LEQ: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                       sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a == b ? 1 : 0); sp += 4; break; }
        case OP_LNEQ: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                        sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                        bpvm_write_i32_be(mem+sp, a != b ? 1 : 0); sp += 4; break; }
        case OP_LLT: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                       sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a <  b ? 1 : 0); sp += 4; break; }
        case OP_LLE: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                       sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a <= b ? 1 : 0); sp += 4; break; }
        case OP_LGT: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                       sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a >  b ? 1 : 0); sp += 4; break; }
        case OP_LGE: { sp -= 8; int64_t b = bpvm_read_i64_be(mem+sp);
                       sp -= 8; int64_t a = bpvm_read_i64_be(mem+sp);
                       bpvm_write_i32_be(mem+sp, a >= b ? 1 : 0); sp += 4; break; }
        case OP_LPRINT: { sp -= 8; int64_t v = bpvm_read_i64_be(mem+sp);
                          emit_long(vm, v, 1); break; }
        case OP_LPRINT_NONL: { sp -= 8; int64_t v = bpvm_read_i64_be(mem+sp);
                               emit_long(vm, v, 0); break; }
        case OP_I32_TO_I64: { sp -= 4; int32_t v = bpvm_read_i32_be(mem+sp);
                              bpvm_write_i64_be(mem+sp, (int64_t) v); sp += 8; break; }
        case OP_I64_TO_I32: { sp -= 8; int64_t v = bpvm_read_i64_be(mem+sp);
                              bpvm_write_i32_be(mem+sp, (int32_t) v); sp += 4; break; }
        case OP_GET_LOCAL_L: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            int64_t v = bpvm_read_i64_be(mem + (uint32_t)((int32_t) bp + soff));
            bpvm_write_i64_be(mem + sp, v); sp += 8; break;
        }
        case OP_SET_LOCAL_L: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            sp -= 8; int64_t v = bpvm_read_i64_be(mem + sp);
            bpvm_write_i64_be(mem + (uint32_t)((int32_t) bp + soff), v); break;
        }
        case OP_GET_GLOBAL_L: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            int64_t v = bpvm_read_i64_be(mem + (uint32_t)((int32_t) cs + soff));
            bpvm_write_i64_be(mem + sp, v); sp += 8; break;
        }
        case OP_SET_GLOBAL_L: {
            int16_t soff = bpvm_read_i16_be(mem + pc); pc += 2;
            sp -= 8; int64_t v = bpvm_read_i64_be(mem + sp);
            bpvm_write_i64_be(mem + (uint32_t)((int32_t) cs + soff), v); break;
        }
        case OP_NEWARRAY_I64: {
            sp -= 4; int32_t size = bpvm_read_i32_be(mem + sp);
            if (size < 0) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) size * 8, BPVM_TYPE_ARRAY_I64);
            if (ref == 0) { exit_status = BPVM_ERR_OOM; goto done; }
            bpvm_write_u32_be(mem + ref, (uint32_t) size);
            bpvm_write_i32_be(mem + sp, (int32_t) ref); sp += 4;
            mem = vm->memory;
            break;
        }
        case OP_ALOAD_I64: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            int64_t v = bpvm_read_i64_be(mem + ref + 4 + (uint32_t) idx * 8);
            bpvm_write_i64_be(mem + sp, v); sp += 8;
            break;
        }
        case OP_ASTORE_I64: {
            sp -= 8; int64_t v   = bpvm_read_i64_be(mem + sp);
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            bpvm_write_i64_be(mem + ref + 4 + (uint32_t) idx * 8, v);
            break;
        }

        /* ---- H1.3 (V2): double (f64). Aritmética + conversiones; paridad VM Java. ---- */
        case OP_DPUSH: {
            int64_t bits = bpvm_read_i64_be(mem + pc); pc += 8;
            bpvm_write_i64_be(mem + sp, bits); sp += 8; break;
        }
        case OP_DADD: { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i64_be(mem+sp, double_to_bits(a + b)); sp += 8; break; }
        case OP_DSUB: { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i64_be(mem+sp, double_to_bits(a - b)); sp += 8; break; }
        case OP_DMUL: { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i64_be(mem+sp, double_to_bits(a * b)); sp += 8; break; }
        case OP_DDIV: { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i64_be(mem+sp, double_to_bits(a / b)); sp += 8; break; }
        case OP_DMOD: { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i64_be(mem+sp, double_to_bits(fmod(a, b))); sp += 8; break; }
        case OP_DNEG: { sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i64_be(mem+sp, double_to_bits(-a)); sp += 8; break; }
        case OP_DEQ:  { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i32_be(mem+sp, a == b ? 1 : 0); sp += 4; break; }
        case OP_DNEQ: { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i32_be(mem+sp, a != b ? 1 : 0); sp += 4; break; }
        case OP_DLT:  { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i32_be(mem+sp, a <  b ? 1 : 0); sp += 4; break; }
        case OP_DLE:  { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i32_be(mem+sp, a <= b ? 1 : 0); sp += 4; break; }
        case OP_DGT:  { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i32_be(mem+sp, a >  b ? 1 : 0); sp += 4; break; }
        case OP_DGE:  { sp -= 8; double b = bits_to_double(bpvm_read_i64_be(mem+sp));
                        sp -= 8; double a = bits_to_double(bpvm_read_i64_be(mem+sp));
                        bpvm_write_i32_be(mem+sp, a >= b ? 1 : 0); sp += 4; break; }
        case OP_DPRINT:      { sp -= 8; double v = bits_to_double(bpvm_read_i64_be(mem+sp)); emit_double(vm, v, 1); break; }
        case OP_DPRINT_NONL: { sp -= 8; double v = bits_to_double(bpvm_read_i64_be(mem+sp)); emit_double(vm, v, 0); break; }
        case OP_I2D: { sp -= 4; int32_t v = bpvm_read_i32_be(mem+sp); bpvm_write_i64_be(mem+sp, double_to_bits((double) v)); sp += 8; break; }
        case OP_D2I: { sp -= 8; double d = bits_to_double(bpvm_read_i64_be(mem+sp)); bpvm_write_i32_be(mem+sp, (int32_t) d); sp += 4; break; }
        case OP_L2D: { sp -= 8; int64_t v = bpvm_read_i64_be(mem+sp); bpvm_write_i64_be(mem+sp, double_to_bits((double) v)); sp += 8; break; }
        case OP_D2L: { sp -= 8; double d = bits_to_double(bpvm_read_i64_be(mem+sp)); bpvm_write_i64_be(mem+sp, (int64_t) d); sp += 8; break; }
        case OP_F2D: { sp -= 4; float f = bits_to_float(bpvm_read_i32_be(mem+sp)); bpvm_write_i64_be(mem+sp, double_to_bits((double) f)); sp += 8; break; }
        case OP_D2F: { sp -= 8; double d = bits_to_double(bpvm_read_i64_be(mem+sp)); bpvm_write_i32_be(mem+sp, float_to_bits((float) d)); sp += 4; break; }
        case OP_L2F: { sp -= 8; int64_t v = bpvm_read_i64_be(mem+sp); bpvm_write_i32_be(mem+sp, float_to_bits((float) v)); sp += 4; break; }
        case OP_F2L: { sp -= 4; float f = bits_to_float(bpvm_read_i32_be(mem+sp)); bpvm_write_i64_be(mem+sp, (int64_t) f); sp += 8; break; }

        /* ---- F2: arrays ---- */
        case OP_NEWARRAY: {
            sp -= 4; int32_t size = bpvm_read_i32_be(mem + sp);
            if (size < 0) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) size * 4, BPVM_TYPE_ARRAY_I32);
            if (ref == 0) { exit_status = BPVM_ERR_OOM; goto done; }
            bpvm_write_u32_be(mem + ref, (uint32_t) size);
            bpvm_write_i32_be(mem + sp, (int32_t) ref); sp += 4;
            mem = vm->memory;  /* heap_alloc no realoca pero defensivo */
            break;
        }
        case OP_NEWARRAY_I8: {
            sp -= 4; int32_t size = bpvm_read_i32_be(mem + sp);
            if (size < 0) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) size, BPVM_TYPE_ARRAY_I8);
            if (ref == 0) { exit_status = BPVM_ERR_OOM; goto done; }
            bpvm_write_u32_be(mem + ref, (uint32_t) size);
            bpvm_write_i32_be(mem + sp, (int32_t) ref); sp += 4;
            break;
        }
        case OP_NEWARRAY_I16: {
            sp -= 4; int32_t size = bpvm_read_i32_be(mem + sp);
            if (size < 0) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) size * 2, BPVM_TYPE_ARRAY_I16);
            if (ref == 0) { exit_status = BPVM_ERR_OOM; goto done; }
            bpvm_write_u32_be(mem + ref, (uint32_t) size);
            bpvm_write_i32_be(mem + sp, (int32_t) ref); sp += 4;
            break;
        }
        case OP_ALOAD: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) {
                exit_status = BPVM_ERR_RUNTIME; goto done;
            }
            int32_t v = bpvm_read_i32_be(mem + ref + 4 + (uint32_t) idx * 4);
            bpvm_write_i32_be(mem + sp, v); sp += 4;
            break;
        }
        case OP_ASTORE: {
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) {
                exit_status = BPVM_ERR_RUNTIME; goto done;
            }
            bpvm_write_i32_be(mem + ref + 4 + (uint32_t) idx * 4, v);
            break;
        }
        case OP_ALEN: {
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            bpvm_write_i32_be(mem + sp, (int32_t) length); sp += 4;
            break;
        }
        case OP_ALOAD_I8: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            int8_t v = (int8_t) mem[ref + 4 + (uint32_t) idx];
            bpvm_write_i32_be(mem + sp, (int32_t) v); sp += 4;
            break;
        }
        case OP_ALOAD_U8: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint8_t v = mem[ref + 4 + (uint32_t) idx];
            bpvm_write_i32_be(mem + sp, (int32_t) v); sp += 4;
            break;
        }
        case OP_ALOAD_I16: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            int16_t v = bpvm_read_i16_be(mem + ref + 4 + (uint32_t) idx * 2);
            bpvm_write_i32_be(mem + sp, (int32_t) v); sp += 4;
            break;
        }
        case OP_ALOAD_U16: {
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            uint16_t v = bpvm_read_u16_be(mem + ref + 4 + (uint32_t) idx * 2);
            bpvm_write_i32_be(mem + sp, (int32_t) v); sp += 4;
            break;
        }
        case OP_ASTORE_I8: {
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            mem[ref + 4 + (uint32_t) idx] = (uint8_t)(v & 0xFF);
            break;
        }
        case OP_ASTORE_I16: {
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; int32_t idx = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            uint32_t length = (ref == 0) ? 0 : bpvm_read_u32_be(mem + ref);
            if (idx < 0 || (uint32_t) idx >= length) { exit_status = BPVM_ERR_RUNTIME; goto done; }
            int16_t v16 = (int16_t)(v & 0xFFFF);
            bpvm_write_u32_be(mem + ref + 4 + (uint32_t) idx * 2, 0); /* unused */
            mem[ref + 4 + (uint32_t) idx * 2]     = (uint8_t)((v16 >> 8) & 0xFF);
            mem[ref + 4 + (uint32_t) idx * 2 + 1] = (uint8_t)(v16 & 0xFF);
            break;
        }

        /* ---- F2: print strings ---- */
        case OP_PRINT_STRING:
        case OP_PRINT_STR_NONL: {
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref != 0) {
                /* H2 (V2): strings son byte[] UTF-8 → emitimos los bytes
                 * directamente, sin truncar (Unicode completo). */
                uint32_t nbytes = bpvm_read_u32_be(mem + ref);
                emit_text(vm, (const char*)(mem + ref + 4), nbytes);
            }
            if (op == OP_PRINT_STRING) emit_newline(vm);
            break;
        }

        /* ---- F2: GC trigger manual ---- */
        case OP_GC_COLLECT: {
            tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
            bpvm_heap_gc(vm);
            mem = vm->memory;
            break;
        }

        /* ---- F2: builtins ---- */
        case OP_CALL_BUILTIN: {
            uint16_t id = bpvm_read_u16_be(mem + pc); pc += 2;
            tc->sp = sp; tc->bp = bp; tc->pc = pc; tc->cs = cs;
            bpvm_status_t bs = bpvm_call_builtin(vm, tc, id);
            sp = tc->sp; bp = tc->bp; pc = tc->pc; cs = tc->cs;
            mem = vm->memory;
            if (bs != BPVM_OK) { exit_status = bs; goto done; }
            /* F4: si el builtin cambió status a BLOCKED_* o RUNNABLE
             * (yield), salimos del quantum para que el scheduler
             * decida. */
            if (tc->status != BPVM_THREAD_RUNNING) {
                if (yielded) *yielded = 1;
                goto done;
            }
            break;
        }

        /* ---- F3: clases ---- */
        case OP_NEW_OBJECT: {
            int16_t cs_off = bpvm_read_i16_be(mem + pc); pc += 2;
            uint32_t class_ptr = (uint32_t)((int32_t) cs + cs_off);
            uint16_t num_fields = bpvm_read_u16_be(mem + class_ptr
                                                    + BPVM_CLS_OFF_NUM_FIELDS);
            uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) num_fields * 4,
                                            BPVM_TYPE_OBJECT);
            if (ref == 0) { exit_status = BPVM_ERR_OOM; goto done; }
            bpvm_write_u32_be(mem + ref, class_ptr);   /* slot[0] = class_ptr */
            /* fields ya zeroed por heap_alloc. */
            bpvm_write_i32_be(mem + sp, (int32_t) ref); sp += 4;
            mem = vm->memory;
            break;
        }
        case OP_GET_FIELD: {
            uint8_t slot = mem[pc++];
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            int32_t v = bpvm_read_i32_be(mem + ref + 4 + (uint32_t) slot * 4);
            bpvm_write_i32_be(mem + sp, v); sp += 4;
            break;
        }
        case OP_SET_FIELD: {
            uint8_t slot = mem[pc++];
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            bpvm_write_i32_be(mem + ref + 4 + (uint32_t) slot * 4, v);
            break;
        }
        /* BUG-6: campos de instancia de 8 bytes (long/double). slot = índice de
         * slot de 4 bytes; el valor ocupa 2 slots consecutivos en ref+4+slot*4. */
        case OP_GET_FIELD_LONG: {
            uint8_t slot = mem[pc++];
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            int64_t v = bpvm_read_i64_be(mem + ref + 4 + (uint32_t) slot * 4);
            bpvm_write_i64_be(mem + sp, v); sp += 8;
            break;
        }
        case OP_SET_FIELD_LONG: {
            uint8_t slot = mem[pc++];
            sp -= 8; int64_t v   = bpvm_read_i64_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            bpvm_write_i64_be(mem + ref + 4 + (uint32_t) slot * 4, v);
            break;
        }

        case OP_INVOKE_VIRTUAL: {
            uint8_t vt_slot = mem[pc++];
            uint8_t num_args = mem[pc++];
            uint32_t this_addr = sp - 4 - (uint32_t) num_args * 4;
            uint32_t this_ref = (uint32_t) bpvm_read_i32_be(mem + this_addr);
            if (this_ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            uint32_t class_ptr = (uint32_t) bpvm_read_i32_be(mem + this_ref);

            /* L2 v3: fall-back al parent si vt[slot] == -1 o slot >= num_methods.
             * El bucle termina al encontrar un methodOff válido o llegar
             * a la raíz (parent_offset == 0). */
            uint32_t desc = class_ptr;
            int32_t method_off = -1;
            uint32_t target_cs = 0;
            for (;;) {
                uint16_t bw       = bpvm_read_u16_be(mem + desc
                                                      + BPVM_CLS_OFF_BITMAP_WORDS);
                uint16_t nmeth    = bpvm_read_u16_be(mem + desc
                                                      + BPVM_CLS_OFF_NUM_METHODS);
                uint32_t vt_base  = desc + BPVM_CLS_OFF_FIELD_BITMAP
                                         + 2u * (uint32_t) bw * 4u;
                if (vt_slot < nmeth) {
                    int32_t off = bpvm_read_i32_be(mem + vt_base + (uint32_t) vt_slot * 4);
                    if (off != -1) {
                        method_off = off;
                        target_cs  = bpvm_get_cs_for_data_addr(vm, desc);
                        break;
                    }
                }
                int32_t parent_off = bpvm_read_i32_be(mem + desc
                                                       + BPVM_CLS_OFF_PARENT_OFF);
                if (parent_off == 0) {
                    fprintf(stderr,
                            "[bpvm-c] INVOKE_VIRTUAL slot %u no resoluble en cadena de herencia\n",
                            vt_slot);
                    exit_status = BPVM_ERR_RUNTIME;
                    goto done;
                }
                uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, desc);
                desc = (uint32_t)((int32_t) cur_cs + parent_off);
            }

            uint32_t target_pc = target_cs + (uint32_t) method_off;
            /* push saved pc/bp/cs, bp = sp, salta */
            bpvm_write_i32_be(mem + sp, (int32_t) pc); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) bp); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) cs); sp += 4;
            bp = sp;
            pc = target_pc;
            cs = target_cs;
            break;
        }

        case OP_INSTANCEOF: {
            int16_t cs_off = bpvm_read_i16_be(mem + pc); pc += 2;
            uint32_t expected = (uint32_t)((int32_t) cs + cs_off);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            int32_t result = 0;
            if (ref != 0) {
                uint32_t cur = (uint32_t) bpvm_read_i32_be(mem + ref);
                while (cur != 0) {
                    if (cur == expected) { result = 1; break; }
                    int32_t parent_off = bpvm_read_i32_be(mem + cur
                                                            + BPVM_CLS_OFF_PARENT_OFF);
                    if (parent_off == 0) break;
                    uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, cur);
                    cur = (uint32_t)((int32_t) cur_cs + parent_off);
                }
            }
            bpvm_write_i32_be(mem + sp, result); sp += 4;
            break;
        }

        case OP_FREE_REF: {
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref != 0) {
                /* Sólo objetos. Para arrays / strings, NOP. */
                uint32_t header = ref - 4;
                uint32_t tag = bpvm_read_u32_be(mem + header);
                int type = (int)((tag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
                if (type == BPVM_TYPE_OBJECT) {
                    /* TODO: recorrer owner_bitmap y FREE_REF recursivo de
                     * fields owners. F3 v1 sólo libera el objeto raíz. */
                    bpvm_write_u32_be(mem + header, tag | BPVM_TAG_FREE_BIT);
                }
            }
            break;
        }

        case OP_SET_FIELD_OWNER: {
            uint8_t slot = mem[pc++];
            sp -= 4; int32_t v   = bpvm_read_i32_be(mem + sp);
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            if (ref == 0) { exit_status = BPVM_ERR_NULL_RECEIVER; goto done; }
            uint32_t field_addr = ref + 4 + (uint32_t) slot * 4;
            uint32_t old_val = (uint32_t) bpvm_read_i32_be(mem + field_addr);
            if (old_val != 0) {
                uint32_t old_header = old_val - 4;
                if (old_val >= vm->heap_start && old_val < vm->heap_next) {
                    uint32_t otag = bpvm_read_u32_be(mem + old_header);
                    int otype = (int)((otag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
                    if (otype == BPVM_TYPE_OBJECT) {
                        bpvm_write_u32_be(mem + old_header, otag | BPVM_TAG_FREE_BIT);
                    }
                }
            }
            bpvm_write_i32_be(mem + field_addr, v);
            break;
        }

        /* ---- F3: CALL_EXT ---- */
        case OP_CALL_EXT: {
            uint16_t idx = bpvm_read_u16_be(mem + pc); pc += 2;
            /* La ext-table del módulo actual vive al inicio del módulo.
             * Lo encontramos buscando el módulo cuyo code_start == cs. */
            uint32_t ext_table_addr = bpvm_get_ext_table_addr(vm, cs);
            if (ext_table_addr == 0) {
                fprintf(stderr, "[bpvm-c] CALL_EXT sin ext-table en cs=%" PRIu32 "\n", cs);
                exit_status = BPVM_ERR_RUNTIME; goto done;
            }
            uint32_t target = (uint32_t) bpvm_read_i32_be(mem
                                + ext_table_addr + (uint32_t) idx * 4);
            if (target == 0) {
                fprintf(stderr, "[bpvm-c] CALL_EXT idx=%u no resuelto\n", idx);
                exit_status = BPVM_ERR_RUNTIME; goto done;
            }
            /* H3 #160 — AOT hijack para CALL_EXT (mismo patrón que CALL). */
            {
                bpvm_aot_thunk_t aot_ext = bpvm_aot_lookup(target);
                if (aot_ext) {
                    /* #186 — mismo boundary de fault que OP_CALL. */
                    tc->cs = cs;
                    int r = aot_call_guarded(vm, tc, aot_ext, &sp, &bp);
                    if (r != 0) {
                        pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
                        if (r == 2) {
                            exit_status = BPVM_ERR_RUNTIME;
                            if (yielded) *yielded = 1;
                            goto done;
                        }
                    }
                    break;
                }
            }
            /* Determinar el CS del módulo destino para que el frame
             * tenga el cs correcto al ejecutar el target. */
            uint32_t target_cs = bpvm_get_cs_for_code_addr(vm, target);
            bpvm_write_i32_be(mem + sp, (int32_t) pc); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) bp); sp += 4;
            bpvm_write_i32_be(mem + sp, (int32_t) cs); sp += 4;
            bp = sp;
            pc = target;
            cs = target_cs;
            break;
        }

        /* ---- F5: exception handling ---- */
        case OP_TRY_BEGIN: {
            /* Operando: handler_rel:i32 + cls_off:i16. handler_pc es
             * RELATIVO al instruction address del TRY_BEGIN (= pc-1).
             * Misma convención que JUMP. */
            uint32_t instr_addr = pc - 1;
            int32_t handler_rel = bpvm_read_i32_be(mem + pc); pc += 4;
            int16_t cls_off     = bpvm_read_i16_be(mem + pc); pc += 2;
            int32_t handler_pc  = (int32_t) instr_addr + handler_rel;
            int32_t expected_cls = (cls_off == 0) ? 0
                                   : (int32_t)((int32_t) cs + cls_off);
            bpvm_eh_push(tc, handler_pc, (int32_t) sp, (int32_t) bp,
                         (int32_t) cs, expected_cls);
            break;
        }
        case OP_TRY_END: {
            bpvm_eh_pop(tc);
            break;
        }
        case OP_THROW: {
            sp -= 4; uint32_t ref = (uint32_t) bpvm_read_i32_be(mem + sp);
            tc->pc = pc; tc->sp = sp; tc->bp = bp; tc->cs = cs;
            int caught = bpvm_eh_unwind(vm, tc, ref);
            if (!caught) {
                exit_status = BPVM_ERR_RUNTIME;
                if (yielded) *yielded = 1;
                /* tc ya marcado TERMINATED por eh_unwind. */
                goto done;
            }
            /* Recargar registros tras unwind. */
            pc = tc->pc; sp = tc->sp; bp = tc->bp; cs = tc->cs;
            break;
        }

        default:
            fprintf(stderr, "[bpvm-c] opcode 0x%02X desconocido en PC %" PRIu32 "\n",
                    op, pc - 1);
            exit_status = BPVM_ERR_BAD_OPCODE;
            goto done;
        }
    }

done:
    /* Persistir registros al ThreadContext. El status ya lo seteó cada
     * case (RUNNABLE si quantum, TERMINATED si HALT/THREAD_EXIT,
     * BLOCKED_* si yield builtin, RUNNING si nada — defensivo). */
    tc->pc = pc; tc->sp = sp; tc->bp = bp; tc->cs = cs;
    if (exit_status != BPVM_OK) {
        tc->status = BPVM_THREAD_TERMINATED;
        if (yielded) *yielded = 1;
    }
    return exit_status;
}
