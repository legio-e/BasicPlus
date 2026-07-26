/* ============================================================
 * events.c — H5.c paso 4: la cola de eventos y su drenaje.
 *
 * El `raise` NO llama al handler: encola (recv, dest, args) y sigue. El
 * scheduler, ENTRE QUANTA, saca un evento y lo INYECTA en el thread destino
 * montándole un frame de llamada igual que el que montaría una llamada normal
 * a ese método. Desde ahí el handler es código BP corriente: puede ceder,
 * bloquear y lanzar excepciones — que es justo lo que no podía hacer el camino
 * viejo (bpvm_call_bp_from_builtin, con su "la función BP no debe ceder").
 *
 * La inyección es un CALL simulado. El único detalle es la vuelta: cuando el
 * handler hace RET, deja en la pila su valor de retorno (aunque sea void, la
 * convención empuja uno), y el código interrumpido no lo espera. Por eso el
 * saved_pc del frame inyectado apunta a un OPCODE SENTINELA — OP_EVENT_RETURN,
 * en la región reservada [0, BPVM_INITIAL_FREE_ADDR), mismo patrón que
 * THREAD_EXIT y NATIVE_RETURN — que tira el valor y salta al PC real, que
 * dejamos guardado en la propia pila, debajo de los argumentos.
 * ============================================================ */
#include "bpvm_internal.h"
#include "bpvm_opcodes.h"

#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------- cola --- */

void bpvm_event_queue_init(bpvm_t* vm) {
    vm->ev_head = 0;
    vm->ev_count = 0;
}

int bpvm_event_enqueue(bpvm_t* vm, int tid, uint64_t recv, int32_t dest,
                       int nargs, uint32_t masks, const int64_t* args) {
    if (vm->ev_count >= BPVM_EVENT_QUEUE_CAP) {
        /* Que GRITE: un evento perdido en silencio es de los que cuesta una
         * tarde encontrar. La cola llena significa que se producen eventos más
         * deprisa de lo que el thread destino los atiende. */
        fprintf(stderr, "[bpvm] cola de eventos llena (%d): evento descartado "
                        "(tid=%d, slot=%d)\n", BPVM_EVENT_QUEUE_CAP, tid, (int) dest);
        return 0;
    }
    int idx = (vm->ev_head + vm->ev_count) % BPVM_EVENT_QUEUE_CAP;
    bpvm_event_t* e = &vm->ev_queue[idx];
    e->recv  = recv;
    e->dest  = dest;
    e->tid   = tid;
    e->nargs = (uint8_t) nargs;
    e->masks = masks;
    for (int i = 0; i < nargs && i < BPVM_EVENT_MAX_ARGS; i++) e->args[i] = args[i];
    vm->ev_count++;
    return 1;
}

/* Raíces de GC: el receptor y los argumentos que son referencias viven en la
 * cola, FUERA de vm->memory, así que el scan conservativo de pilas no los ve.
 * Sin esto, un objeto cuyo único holder es un evento pendiente se recolecta
 * EN VIVO y el handler recibe basura. Mismo agujero que tapó #302 con las
 * raíces del GUI. */
void bpvm_event_mark_roots(bpvm_t* vm, void (*visit)(bpvm_t*, uint32_t)) {
    for (int k = 0; k < vm->ev_count; k++) {
        const bpvm_event_t* e = &vm->ev_queue[(vm->ev_head + k) % BPVM_EVENT_QUEUE_CAP];
        bpref_t r; r.v = e->recv;
        visit(vm, bpref_addr(r));
        for (int i = 0; i < e->nargs; i++)
            if (e->masks & (1u << i)) visit(vm, (uint32_t) e->args[i]);
    }
}

/* ----------------------------------------------------------- inyección --- */

/* Resuelve el slot de vtable sobre la clase REAL del receptor, subiendo por la
 * herencia igual que INVOKE_VIRTUAL. Devuelve 0 si no resuelve. */
static int resolve_slot(bpvm_t* vm, uint32_t obj_addr, int32_t slot,
                        int32_t* out_off, uint32_t* out_cs) {
    uint8_t* mem = vm->memory;
    uint32_t desc = (uint32_t) bpvm_read_i32_be(mem + obj_addr);
    for (;;) {
        uint16_t bw    = bpvm_read_u16_be(mem + desc + BPVM_CLS_OFF_BITMAP_WORDS);
        uint16_t nmeth = bpvm_read_u16_be(mem + desc + BPVM_CLS_OFF_NUM_METHODS);
        uint32_t vt    = desc + BPVM_CLS_OFF_FIELD_BITMAP + 2u * (uint32_t) bw * 4u;
        if (slot >= 0 && slot < (int32_t) nmeth) {
            int32_t off = bpvm_read_i32_be(mem + vt + (uint32_t) slot * 4);
            if (off != -1) {
                *out_cs  = bpvm_get_cs_for_data_addr(vm, desc);
                *out_off = off;                  /* CS-relativo; el cb lo aplica inject */
                return 1;
            }
        }
        int32_t parent_off = bpvm_read_i32_be(mem + desc + BPVM_CLS_OFF_PARENT_OFF);
        if (parent_off == 0) return 0;
        uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, desc);
        desc = (uint32_t) ((int32_t) cur_cs + parent_off);
    }
}

/* Monta el frame del handler en la pila de `tc`. El thread NO puede estar
 * corriendo (lo llama el scheduler con el tc en la mano). */
static int inject(bpvm_t* vm, bpvm_thread_t* tc, const bpvm_event_t* e) {
    /* El receptor puede haber muerto entre el raise y el drenaje — es el caso
     * normal cuando se destruye un suscriptor con eventos pendientes, y el
     * diseño dice que un evento sin quien lo escuche se IGNORA, no revienta. */
    bpref_t recv; recv.v = e->recv;
    if (bpref_is_null(recv) || bpvm_ref_dead(vm, recv)) return 0;
    uint32_t obj = bpref_deref(vm, recv);

    int32_t  hoff; uint32_t hcs;
    if (!resolve_slot(vm, obj, e->dest, &hoff, &hcs)) {
        fprintf(stderr, "[bpvm] evento: slot %d no resoluble en la clase del "
                        "receptor — descartado\n", (int) e->dest);
        return 0;
    }

    /* Espacio: pc guardado (4) + this (REF) + args + 3 saves (12). */
    uint32_t need = 4u + BPVM_REF_SIZE + 12u;
    for (int i = 0; i < e->nargs; i++) need += (e->masks & (1u << (8 + i))) ? 8u : 4u;
    if (tc->sp + need > tc->stack_top) {
        fprintf(stderr, "[bpvm] evento: sin pila en tid=%d — descartado\n", tc->id);
        return 0;
    }

    uint32_t sp = tc->sp;
    /* (1) el PC de reanudación, DEBAJO de todo: OP_EVENT_RETURN lo lee de aquí.
     *     Guardarlo en la pila (y no en el tc) hace la inyección reentrante sin
     *     estado extra: si un handler es interrumpido por otro evento, cada
     *     frame lleva su propia vuelta. */
    bpvm_write_i32_be(vm->memory + sp, (int32_t) tc->pc); sp += 4;

    /* (2) los argumentos, EXACTAMENTE como los pondría una llamada normal:
     *     `this` primero y cada argumento en su ancho natural. El compilador
     *     nos dice cuál es (bits 8-11 de masks); la VM no lo adivina. */
    bpref_store(vm, sp, recv); sp += BPVM_REF_SIZE;
    for (int i = 0; i < e->nargs; i++) {
        if (e->masks & (1u << (8 + i))) { bpvm_write_i64_be(vm->memory + sp, e->args[i]); sp += 8; }
        else { bpvm_write_i32_be(vm->memory + sp, (int32_t) e->args[i]); sp += 4; }
    }

    /* (3) los tres saves del CALL. El pc guardado es el SENTINELA. */
    bpvm_write_i32_be(vm->memory + sp, (int32_t) BPVM_SENTINEL_EVENT_RETURN_ADDR); sp += 4;
    bpvm_write_i32_be(vm->memory + sp, (int32_t) tc->bp); sp += 4;
    bpvm_write_i32_be(vm->memory + sp, (int32_t) tc->cs); sp += 4;

    tc->sp = sp;
    tc->bp = sp;
    tc->cs = hcs;
    tc->ev_depth++;
    tc->pc = bpvm_cb_for_cs(vm, tc, hcs) + (uint32_t) hoff;   /* H3.c: vtable → cb */
    return 1;
}

/* Saca UN evento para `tc` y lo inyecta. Uno solo por punto de planificación:
 * inyectar dos seguidos los ejecutaría en orden inverso (el segundo frame
 * queda encima), y los eventos son FIFO. */
int bpvm_event_drain_one(bpvm_t* vm, bpvm_thread_t* tc) {
    if (tc->ev_depth > 0) return 0;   /* un handler a la vez: FIFO de verdad */
    for (int k = 0; k < vm->ev_count; k++) {
        int idx = (vm->ev_head + k) % BPVM_EVENT_QUEUE_CAP;
        bpvm_event_t* e = &vm->ev_queue[idx];
        if (e->tid != tc->id) continue;
        bpvm_event_t copy = *e;
        /* Sacar de la cola compactando: los de delante bajan un hueco. */
        for (int j = k; j > 0; j--) {
            int dst = (vm->ev_head + j) % BPVM_EVENT_QUEUE_CAP;
            int src = (vm->ev_head + j - 1) % BPVM_EVENT_QUEUE_CAP;
            vm->ev_queue[dst] = vm->ev_queue[src];
        }
        vm->ev_head = (vm->ev_head + 1) % BPVM_EVENT_QUEUE_CAP;
        vm->ev_count--;
        return inject(vm, tc, &copy);
    }
    return 0;
}
