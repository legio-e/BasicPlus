/*
 * bpvm_dbg_wire.c — núcleo portable del ramo de depuración del wire v1.
 * Extraído LITERALMENTE de pico/repl_v1.c (H6.b.3 #140), que era la única
 * implementación y llevaba tiempo funcionando en placa. Ver el .h para el
 * porqué del contrato (sin JSON aquí, buffers prestados).
 *
 * El estado es GLOBAL a propósito (una sesión de depuración por dispositivo,
 * igual que antes): breakpoints pendientes pre-RUN + vm activa + snapshot del
 * frame pausado. No se toca desde varias tasks: el pause_cb corre en la MISMA
 * task del REPL que atiende los comandos (la VM está detenida dentro de él).
 */
#include "bpvm_dbg_wire.h"
/* BPVM_MAX_BREAKPOINTS, bpvm_mem_read_i32/u32 y las structs completas (vm->memory,
 * tc->stack_base) — el ramo lee memoria cruda de la VM, así que necesita el
 * interior, igual que lo necesitaba dentro de repl_v1.c. */
#include "bpvm_internal.h"

#include <stdio.h>
#include <string.h>

/* Breakpoints pedidos ANTES de que exista la vm (el IDE los pone al abrir el
 * fichero, mucho antes del RUN) → se acumulan y se aplican en el arm. */
static uint32_t s_pending_bp_pc[BPVM_MAX_BREAKPOINTS];
static int      s_pending_bp_id[BPVM_MAX_BREAKPOINTS];
static int      s_pending_bp_n  = 0;
static int      s_bp_id_seq     = 1;     /* ids provisionales pre-RUN */
static int      s_pause_initial = 0;     /* PAUSE pre-RUN → romper en el 1er opcode */
static bpvm_t*  s_dbg_vm        = NULL;  /* vm activa (no-NULL ⇒ depurando) */

/* Snapshot del frame pausado (para LOCALS/STACK/READ_* mientras está parado). */
static uint32_t s_pf_pc, s_pf_sp, s_pf_bp, s_pf_cs, s_pf_sbase;

static void emit(bpvm_dbg_wire_t* w, int off) {
    if (off > 0) w->send(w->reply, (size_t) off, w->user);
}

bpvm_dbg_cmd_kind_t bpvm_dbg_wire_kind(const char* type) {
    if (!type) return BPVM_DBGC_OTHER;
    if (strcmp(type, "CONTINUE")    == 0) return BPVM_DBGC_CONTINUE;
    if (strcmp(type, "STEP")        == 0) return BPVM_DBGC_STEP;
    if (strcmp(type, "STOP")        == 0) return BPVM_DBGC_STOP;
    if (strcmp(type, "KILL")        == 0) return BPVM_DBGC_STOP;
    if (strcmp(type, "PING")        == 0) return BPVM_DBGC_PING;
    if (strcmp(type, "PAUSE")       == 0) return BPVM_DBGC_PAUSE;
    if (strcmp(type, "SET_BP")      == 0) return BPVM_DBGC_SET_BP;
    if (strcmp(type, "CLR_BP")      == 0) return BPVM_DBGC_CLR_BP;
    if (strcmp(type, "READ_INT")    == 0) return BPVM_DBGC_READ_INT;
    if (strcmp(type, "READ_STRING") == 0) return BPVM_DBGC_READ_STRING;
    if (strcmp(type, "LOCALS")      == 0) return BPVM_DBGC_LOCALS;
    if (strcmp(type, "STACK")       == 0) return BPVM_DBGC_STACK;
    return BPVM_DBGC_OTHER;
}

/* ── handlers de cada comando ────────────────────────────────────────────── */

/* SET_BP{pc}: live si hay vm (pausada); si no, acumula pre-RUN. */
static void dbg_set_bp(bpvm_dbg_wire_t* w, const bpvm_dbg_cmd_t* c) {
    int bpId = -1;
    if (s_dbg_vm) {
        if (c->pc >= 0) bpId = bpvm_debug_add_breakpoint(s_dbg_vm, (uint32_t) c->pc);
    } else if (c->pc >= 0 && s_pending_bp_n < BPVM_MAX_BREAKPOINTS) {
        bpId = s_bp_id_seq++;
        s_pending_bp_pc[s_pending_bp_n] = (uint32_t) c->pc;
        s_pending_bp_id[s_pending_bp_n] = bpId;
        s_pending_bp_n++;
    }
    emit(w, snprintf(w->reply, w->reply_cap,
        "{\"type\":\"SET_BP_REPLY\",\"id\":%ld,\"bpId\":%d}", c->id, bpId));
}

/* CLR_BP{bpId}: bpId<0 ⇒ limpiar todos. */
static void dbg_clr_bp(bpvm_dbg_wire_t* w, const bpvm_dbg_cmd_t* c) {
    if (s_dbg_vm) {
        if (c->bpId >= 0) bpvm_debug_clear_breakpoint(s_dbg_vm, (int) c->bpId);
        else              bpvm_debug_clear_breakpoints(s_dbg_vm);
    } else if (c->bpId < 0) {
        s_pending_bp_n = 0;
    } else {
        for (int i = 0; i < s_pending_bp_n; i++) {
            if (s_pending_bp_id[i] == (int) c->bpId) {
                for (int j = i + 1; j < s_pending_bp_n; j++) {
                    s_pending_bp_pc[j-1] = s_pending_bp_pc[j];
                    s_pending_bp_id[j-1] = s_pending_bp_id[j];
                }
                s_pending_bp_n--; break;
            }
        }
    }
    emit(w, snprintf(w->reply, w->reply_cap,
        "{\"type\":\"CLR_BP_REPLY\",\"id\":%ld}", c->id));
}

/* ¿Cabe [addr, addr+n) DENTRO de la memoria de la VM?
 *
 * #326 — ESTO FALTABA, Y COSTABA LA PLACA. Las direcciones de READ_INT y
 * READ_STRING vienen DEL WIRE, y bpvm_mem_read_u32 es `vm->memory + addr` a
 * pelo, sin comprobar nada (bpvm_internal.h:635). Con un valor fuera de rango el
 * puntero se sale de la SRAM, el Cortex-M33 levanta un BusFault → HardFault, y
 * la Pico se queda colgada con el USB muerto: no hay mensaje, no hay reset, no
 * hay nada. Un depurador NUNCA puede matar al depurado porque el host le pida
 * una dirección mala; como mucho debe contestar que no puede.
 * El chequeo va en el núcleo portable, así que cubre las 3 familias + el
 * simulado de una vez. */
static int mem_ok(const bpvm_t* vm, uint64_t addr, uint64_t n) {
    return vm && vm->memory && addr + n >= addr            /* sin desbordar */
           && addr + n <= (uint64_t) vm->memory_size;
}

/* READ_INT{addr}: i32 crudo en dirección absoluta. */
static void dbg_read_int(bpvm_dbg_wire_t* w, const bpvm_dbg_cmd_t* c) {
    if (s_dbg_vm && c->addr >= 0 && !mem_ok(s_dbg_vm, (uint64_t) c->addr, 4)) {
        emit(w, snprintf(w->reply, w->reply_cap,
            "{\"type\":\"ERROR\",\"id\":%ld,\"code\":\"BAD_ADDR\","
            "\"message\":\"addr %ld fuera de la memoria (%lu B)\"}",
            c->id, c->addr, (unsigned long) s_dbg_vm->memory_size));
        return;
    }
    int32_t v = (s_dbg_vm && c->addr >= 0)
                ? bpvm_mem_read_i32(s_dbg_vm, (uint32_t) c->addr) : 0;
    emit(w, snprintf(w->reply, w->reply_cap,
        "{\"type\":\"READ_INT_REPLY\",\"id\":%ld,\"value\":%ld}", c->id, (long) v));
}

/* READ_STRING{ref}: string heap = [byte_len:u32 BE][bytes UTF-8]; ref 0 = "". */
static void dbg_read_string(bpvm_dbg_wire_t* w, const bpvm_dbg_cmd_t* c) {
    /* `ref` es una REFERENCIA, no una dirección — y desde H1 una referencia es un
     * HANDLE (idx|TAG), no un offset plano. Este código venía del modelo viejo y
     * usaba el valor como desplazamiento: con un handle real (medido en placa:
     * 1073741864 = 0x40000000|40, con un heap de 257 KB) el puntero se iba fuera
     * de la SRAM. bpref_regen + bpref_deref es la indirección oficial, y para una
     * ref SIN tag (null, dirección cruda, constante del data block) devuelve el
     * valor tal cual: sirve para los dos casos sin distinguirlos aquí. */
    uint32_t addr = 0;
    if (s_dbg_vm && c->ref > 0) {
        addr = bpref_deref(s_dbg_vm, bpref_regen(s_dbg_vm, (uint32_t) c->ref));
        if (addr == 0u) {   /* handle muerto o índice fuera de la tabla */
            emit(w, snprintf(w->reply, w->reply_cap,
                "{\"type\":\"ERROR\",\"id\":%ld,\"code\":\"BAD_REF\","
                "\"message\":\"ref %ld no resuelve a ningun objeto vivo\"}",
                c->id, c->ref));
            return;
        }
        /* La cabecera tiene que caber ANTES de leerla, y el cuerpo antes de
         * recorrerlo: blen sale de la propia memoria, o sea que tampoco es un
         * valor en el que se pueda confiar. */
        if (!mem_ok(s_dbg_vm, addr, 4)) {
            emit(w, snprintf(w->reply, w->reply_cap,
                "{\"type\":\"ERROR\",\"id\":%ld,\"code\":\"BAD_REF\","
                "\"message\":\"ref %ld -> %lu, fuera de la memoria (%lu B)\"}",
                c->id, c->ref, (unsigned long) addr,
                (unsigned long) s_dbg_vm->memory_size));
            return;
        }
    }
    int off = snprintf(w->reply, w->reply_cap,
        "{\"type\":\"READ_STRING_REPLY\",\"id\":%ld,\"value\":\"", c->id);
    if (off < 0) return;
    if (s_dbg_vm && addr != 0u) {
        uint32_t blen = bpvm_mem_read_u32(s_dbg_vm, addr);
        if (!mem_ok(s_dbg_vm, (uint64_t) addr + 4, blen)) {
            emit(w, snprintf(w->reply, w->reply_cap,
                "{\"type\":\"ERROR\",\"id\":%ld,\"code\":\"BAD_REF\","
                "\"message\":\"ref %ld dice medir %lu B y no cabe (%lu B)\"}",
                c->id, c->ref, (unsigned long) blen,
                (unsigned long) s_dbg_vm->memory_size));
            return;
        }
        const uint8_t* b = s_dbg_vm->memory + addr + 4;
        for (uint32_t i = 0; i < blen && off < (int) w->reply_cap - 8; i++) {
            unsigned char ch = b[i];
            if (ch == '"' || ch == '\\') { w->reply[off++] = '\\'; w->reply[off++] = (char) ch; }
            else if (ch == '\n')         { w->reply[off++] = '\\'; w->reply[off++] = 'n'; }
            else if (ch == '\t')         { w->reply[off++] = '\\'; w->reply[off++] = 't'; }
            else if (ch >= 0x20)           w->reply[off++] = (char) ch;  /* incl. UTF-8 >=0x80 */
        }
    }
    off += snprintf(w->reply + off, w->reply_cap - (size_t) off, "\"}");
    emit(w, off);
}

/* LOCALS: i32 crudos entre bp y sp del frame pausado (el host resuelve los
 * nombres con el .dbg). */
static void dbg_locals(bpvm_dbg_wire_t* w, long id) {
    int off = snprintf(w->reply, w->reply_cap,
        "{\"type\":\"LOCALS_REPLY\",\"id\":%ld,\"locals\":[", id);
    if (s_dbg_vm && s_pf_sp > s_pf_bp) {
        int nl = (int) ((s_pf_sp - s_pf_bp) / 4);
        for (int i = 0; i < nl && off < (int) w->reply_cap - 24; i++)
            off += snprintf(w->reply + off, w->reply_cap - (size_t) off, "%s%ld",
                            i ? "," : "",
                            (long) bpvm_mem_read_i32(s_dbg_vm, s_pf_bp + i * 4));
    }
    off += snprintf(w->reply + off, w->reply_cap - (size_t) off, "]}");
    emit(w, off);
}

/* STACK: walk de frames (saved pc en bp-12, saved bp en bp-8 = igual que la VM Java). */
static void dbg_stack(bpvm_dbg_wire_t* w, long id) {
    int off = snprintf(w->reply, w->reply_cap,
        "{\"type\":\"STACK_REPLY\",\"id\":%ld,\"frames\":[", id);
    if (s_dbg_vm) {
        uint32_t cbp = s_pf_bp, cpc = s_pf_pc;
        int first = 1, safety = 0;
        while (cbp > s_pf_sbase && safety < 256 && off < (int) w->reply_cap - 48) {
            off += snprintf(w->reply + off, w->reply_cap - (size_t) off, "%s[%lu,%lu]",
                            first ? "" : ",", (unsigned long) cpc, (unsigned long) cbp);
            first = 0;
            cpc = bpvm_mem_read_u32(s_dbg_vm, cbp - 12);
            cbp = bpvm_mem_read_u32(s_dbg_vm, cbp - 8);
            safety++;
        }
        off += snprintf(w->reply + off, w->reply_cap - (size_t) off, "%s[%lu,%lu]",
                        first ? "" : ",", (unsigned long) cpc, (unsigned long) cbp);
    }
    off += snprintf(w->reply + off, w->reply_cap - (size_t) off, "]}");
    emit(w, off);
}

/* ── bucle de pausa ──────────────────────────────────────────────────────── */

/* Snapshot del frame, emite BP_HIT y atiende comandos INLINE hasta
 * CONTINUE/STEP/STOP. Corre en la task del REPL, con la VM detenida dentro. */
static bpvm_dbg_action_t pause_cb(bpvm_t* vm, bpvm_thread_t* tc,
                                  uint32_t pc, void* user) {
    (void) vm;
    bpvm_dbg_wire_t* w = (bpvm_dbg_wire_t*) user;

    s_pf_pc    = pc;
    s_pf_sp    = bpvm_thread_sp(tc);
    s_pf_bp    = bpvm_thread_bp(tc);
    s_pf_cs    = bpvm_thread_cs(tc);
    s_pf_sbase = tc->stack_base;

    emit(w, snprintf(w->reply, w->reply_cap,
        "{\"type\":\"BP_HIT\",\"session\":%ld,\"tid\":%d,"
        "\"pc\":%lu,\"sp\":%lu,\"bp\":%lu,\"cs\":%lu}",
        w->session, bpvm_thread_id(tc),
        (unsigned long) pc, (unsigned long) s_pf_sp,
        (unsigned long) s_pf_bp, (unsigned long) s_pf_cs));

    for (;;) {
        bpvm_dbg_cmd_t c;
        memset(&c, 0, sizeof c);
        c.pc = c.bpId = c.addr = -1;
        if (w->next_cmd(&c, w->user) != 0) continue;   /* línea ilegible: ignora */

        switch (c.kind) {
            case BPVM_DBGC_CONTINUE: return BPVM_DBG_CONTINUE;
            case BPVM_DBGC_STEP:     return BPVM_DBG_STEP;
            case BPVM_DBGC_STOP:     return BPVM_DBG_STOP;
            case BPVM_DBGC_PING:
                emit(w, snprintf(w->reply, w->reply_cap,
                     "{\"type\":\"PONG\",\"id\":%ld}", c.id));
                break;
            case BPVM_DBGC_SET_BP:      dbg_set_bp(w, &c);      break;
            case BPVM_DBGC_CLR_BP:      dbg_clr_bp(w, &c);      break;
            case BPVM_DBGC_READ_INT:    dbg_read_int(w, &c);    break;
            case BPVM_DBGC_READ_STRING: dbg_read_string(w, &c); break;
            case BPVM_DBGC_LOCALS:      dbg_locals(w, c.id);    break;
            case BPVM_DBGC_STACK:       dbg_stack(w, c.id);     break;
            default:
                emit(w, snprintf(w->reply, w->reply_cap,
                     "{\"type\":\"ERROR\",\"id\":%ld,\"code\":\"UNSUPPORTED\","
                     "\"message\":\"no valido en pausa\"}", c.id));
                break;
        }
    }
}

/* ── API pública ─────────────────────────────────────────────────────────── */

int bpvm_dbg_wire_handle(bpvm_dbg_wire_t* w, const bpvm_dbg_cmd_t* cmd) {
    switch (cmd->kind) {
        case BPVM_DBGC_PAUSE:
            s_pause_initial = 1;
            emit(w, snprintf(w->reply, w->reply_cap,
                 "{\"type\":\"PAUSE_REPLY\",\"id\":%ld}", cmd->id));
            return 1;
        case BPVM_DBGC_SET_BP: dbg_set_bp(w, cmd); return 1;
        case BPVM_DBGC_CLR_BP: dbg_clr_bp(w, cmd); return 1;
        default: return 0;
    }
}

int bpvm_dbg_wire_armed(void) {
    return (s_pending_bp_n > 0 || s_pause_initial);
}

void bpvm_dbg_wire_arm(bpvm_dbg_wire_t* w, bpvm_t* vm) {
    if (!bpvm_dbg_wire_armed()) return;
    for (int i = 0; i < s_pending_bp_n; i++)
        bpvm_debug_add_breakpoint(vm, s_pending_bp_pc[i]);
    bpvm_set_pause_cb(vm, pause_cb, w);
    s_dbg_vm = vm;
    if (s_pause_initial) bpvm_debug_request_pause(vm);
}

void bpvm_dbg_wire_reset(void) {
    s_dbg_vm        = NULL;
    s_pending_bp_n  = 0;
    s_pause_initial = 0;
}
