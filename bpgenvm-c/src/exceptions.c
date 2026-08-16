/*
 * exceptions.c — try/catch/throw + RuntimeError BP (F5).
 *
 * Modelo de la VM Java (docs/OPCODES.md §0x5B..0x5D):
 *   TRY_BEGIN <handler_rel:i32, cls_off:i16>:
 *     push entry { handler_pc=pc+handler_rel, saved_sp, saved_bp,
 *                  saved_cs, expected_class=cs+cls_off (0 = catch all) }.
 *   TRY_END:
 *     pop entry top.
 *   THROW:
 *     pop ref; busca entry cuya expected_class matchee (= 0 ó
 *     isDescendantOf(class(ref), expected)). Si lo encuentra:
 *     restaura sp/bp/cs/pc desde la entry, pop hasta esa entry,
 *     push ref. Si no, BpThreadFault con stack trace.
 */

#include "bpvm_internal.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */

void bpvm_eh_push(bpvm_thread_t* tc, int32_t handler_pc, int32_t saved_sp,
                  int32_t saved_bp, int32_t saved_cs, int32_t expected_class) {
    if (tc->eh_stack_size >= tc->eh_stack_capacity) {
        int new_cap = tc->eh_stack_capacity == 0 ? 4 : tc->eh_stack_capacity * 2;
        bpvm_eh_entry_t* arr = (bpvm_eh_entry_t*) bpvm_realloc(tc->eh_stack,
                                  (size_t) new_cap * sizeof(bpvm_eh_entry_t));
        if (!arr) return;
        tc->eh_stack = arr;
        tc->eh_stack_capacity = new_cap;
    }
    bpvm_eh_entry_t* e = &tc->eh_stack[tc->eh_stack_size++];
    e->handler_pc     = handler_pc;
    e->saved_sp       = saved_sp;
    e->saved_bp       = saved_bp;
    e->saved_cs       = saved_cs;
    e->expected_class = expected_class;
}

void bpvm_eh_pop(bpvm_thread_t* tc) {
    if (tc->eh_stack_size > 0) tc->eh_stack_size--;
}

/* Sube por la cadena de herencia para determinar si `obj_class`
 * desciende de `target`. Cross-module via getCSForDataAddr.
 * Equivalente a VirtualMachine.isDescendantOf. */
static int is_descendant_of(const bpvm_t* vm, uint32_t obj_class, uint32_t target) {
    uint32_t cur = obj_class;
    while (cur != 0) {
        if (cur == target) return 1;
        int32_t parent_off = bpvm_read_i32_be(vm->memory + cur + BPVM_CLS_OFF_PARENT_OFF);
        if (parent_off == 0) return 0;
        uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, cur);
        cur = (uint32_t)((int32_t) cur_cs + parent_off);
    }
    return 0;
}

int bpvm_eh_unwind(bpvm_t* vm, bpvm_thread_t* tc, bpref_t ref) {
    uint32_t thrown_class = 0;
    if (!bpref_is_null(ref)) {
        thrown_class = (uint32_t) bpvm_read_i32_be(vm->memory + bpref_deref(vm, ref));
    }
    /* Busca un handler que matchee, desde el top hacia abajo. */
    while (tc->eh_stack_size > 0) {
        bpvm_eh_entry_t e = tc->eh_stack[--tc->eh_stack_size];
        int matches = (e.expected_class == 0)
                   || (thrown_class != 0
                       && is_descendant_of(vm, thrown_class, (uint32_t) e.expected_class));
        if (matches) {
            tc->sp = (uint32_t) e.saved_sp;
            tc->bp = (uint32_t) e.saved_bp;
            tc->cs = (uint32_t) e.saved_cs;
            tc->pc = (uint32_t) e.handler_pc;
            /* Push ref para que el catch lo reciba como local. V4: handle 64b COMPLETO
             * (gen preservada) — si se truncara a 32b, un objeto-excepción en slot
             * reciclado (gen>0) daría gen-mismatch al leer e.msg y re-lanzaría. */
            bpvm_write_i64_be(vm->memory + tc->sp, (int64_t) ref.v);
            tc->sp += 8;
            return 1;
        }
        /* No matchee: seguimos popeando entries (los handlers más
         * exteriores). */
    }
    /* Sin handler: print error + terminar thread.
     *
     * #355/#353 — POR EL CANAL DE DIAG, NO POR stderr. Este es EL final de
     * camino de un thread que muere, y en la placa el stderr no llega a ningún
     * sitio: el programa se apagaba sin decir por qué.
     *
     * Y es justo el caso que nos tuvo ciegos: una excepción atrapada por el
     * `catch` del programa, cuyo manejador vuelve a quedarse sin memoria al
     * componer el mensaje → lanza OTRA VEZ, ya sin nadie que la recoja → el
     * thread termina en silencio a mitad del catch. Sin esta línea, la última
     * señal es la penúltima que el programa alcanzó a imprimir. */
    bpvm_diag_urgente("[bpvm] EXCEPCION NO ATRAPADA en el thread %" PRId32 ": el thread "
              "TERMINA aqui. Si esto sale dentro de un catch, el manejador se ha "
              "quedado sin memoria al construir su propio mensaje.", tc->id);
    if (!bpref_is_null(ref)) {
        /* Intenta leer field 0 = msg (asumiendo layout RuntimeError). */
        bpref_t msg_r; msg_r.v = (uint64_t) bpvm_read_i64_be(vm->memory + bpref_deref(vm, ref) + 4);   /* msg = handle 64b */
        if (msg_r.v != 0u) {
            uint32_t msg_addr = bpref_deref(vm, msg_r);   /* handle→addr */
            uint32_t mlen = bpvm_read_u32_be(vm->memory + msg_addr);   /* H2: bytes UTF-8 */
            char buf[256]; size_t n = 0;
            for (uint32_t i = 0; i < mlen && n < sizeof(buf) - 1; i++) {
                buf[n++] = (char) vm->memory[msg_addr + 4 + i];
            }
            buf[n] = '\0';
            bpvm_diag("[bpvm]   el mensaje que traia: \"%s\"", buf);
            /* #406 — Y AL SITIO DEL QUE LO SACA EL WIRE.
             *
             * Habia DOS formas de morir y solo una tenia voz: el detalle del
             * EXITED lo rellena bpvm_throw_runtime_error (abajo), o sea el
             * camino de los errores que lanza LA VM. Un `throw` de clase de
             * USUARIO no pasa por ahi, asi que en la placa el IDE recibia
             * `exit 11` pelado y el texto se perdia — justo la forma en que
             * muere el programa de un usuario normal.
             *
             * La linea de arriba ya lo decia, pero por el canal de diag: sirve
             * mirando la consola, no para que el host lo cuente. Esto es lo que
             * lo cruza.
             *
             * Va en el NUCLEO (src/) y no en cada repl: son tres familias, y el
             * fallo de esta manana fue tres veces «el comun crecio y la copia
             * privada no». Desde aqui viaja solo. */
            /* El recorte se DICE (%.160s) en vez de dejarlo al snprintf: cabe
             * de sobra en runtime_error[192] y asi no hay un aviso del
             * compilador que alguien tenga que volver a mirar.
             *
             * Y se distingue el mensaje VACIO: un «excepcion no atrapada: » con
             * nada detras es medio mensaje, y este arreglo va justo de eso. La
             * cadena puede existir y estar vacia, asi que no basta con mirar si
             * hay objeto. */
            if (n > 0) {
                snprintf(vm->runtime_error, sizeof(vm->runtime_error),
                         "excepcion no atrapada: %.160s", buf);
            } else {
                snprintf(vm->runtime_error, sizeof(vm->runtime_error),
                         "excepcion no atrapada (sin mensaje)");
            }
        } else {
            /* Ni siquiera hay objeto de mensaje. Que se sepa QUE paso: «exit 11»
             * a secas no distingue esto de un fallo de la propia VM. */
            snprintf(vm->runtime_error, sizeof(vm->runtime_error),
                     "excepcion no atrapada (sin mensaje)");
        }
    }
    tc->status = BPVM_THREAD_TERMINATED;
    return 0;
}

const char* bpvm_runtime_error(const bpvm_t* vm) {
    return (vm && vm->runtime_error[0]) ? vm->runtime_error : "";
}

bpref_t bpvm_throw_runtime_error(bpvm_t* vm, bpvm_thread_t* tc,
                                  const char* msg) {
    /* Guarda el detalle para que el host / wire lo surtan si el error acaba
     * NO atrapado (espejo de link_error). En cada throw; solo se reporta si el
     * run termina en BPVM_ERR_RUNTIME. */
    snprintf(vm->runtime_error, sizeof(vm->runtime_error), "%s", msg ? msg : "");
    /* Buscar el class_ptr de RuntimeError exportado por algún módulo.
     * El frontend Java sintetiza la clase y la exporta como data
     * symbol "<lib>.<mod>.RuntimeError" / "<mod>.RuntimeError" — la
     * global symbol table del linker ya las tiene registradas. */
    uint32_t class_ptr = 0;
    /* #248 — primero la clase ÚNICA de Core (Object -> Exception ->
     * RuntimeError). Los fallbacks de abajo cubren .mods legado con copia
     * per-módulo. */
    class_ptr = bpvm_link_lookup(vm, "Core.RuntimeError");
    if (class_ptr) goto have_class;
    /* Probamos primero el módulo del cs actual. */
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (m->code_start != tc->cs) continue;
        char qual[160];
        if (m->library[0]) snprintf(qual, sizeof(qual), "%s.%s.RuntimeError",
                                     m->library, m->name);
        else               snprintf(qual, sizeof(qual), "%s.RuntimeError",
                                     m->name);
        class_ptr = bpvm_link_lookup(vm, qual);
        if (class_ptr) break;
    }
    /* Fallback: cualquier módulo que la haya exportado. */
    if (!class_ptr) {
        for (int i = 0; i < vm->module_count && !class_ptr; i++) {
            const bpvm_module_t* m = &vm->modules[i];
            char qual[160];
            if (m->library[0]) snprintf(qual, sizeof(qual), "%s.%s.RuntimeError",
                                         m->library, m->name);
            else               snprintf(qual, sizeof(qual), "%s.RuntimeError",
                                         m->name);
            class_ptr = bpvm_link_lookup(vm, qual);
        }
    }
have_class:
    if (!class_ptr) {
        /* Sin RuntimeError disponible — caller debe usar BpThreadFault
         * equivalente (= terminar thread). Aquí imprimimos al menos. */
        /* #355 — al canal de diag: en placa el stderr no llega a ningun sitio
         * util. De paso cierra uno de los 29 fprintf de #353. */
        bpvm_diag_urgente("[bpvm] throw: SIN CLASE RuntimeError exportada, no hay con "
                  "que construir la excepcion: %s", msg ? msg : "");
        return bpref_null();
    }

    /* #355 — AQUI se suelta la reserva de emergencia (idea de Eduardo), justo
     * antes de las dos reservas que hacen falta para MATERIALIZAR el error: la
     * cadena del mensaje y el objeto RuntimeError.
     *
     * Era la pescadilla que nos mordia la cola: si el heap esta lleno, estas dos
     * reservas fallan tambien, se devolvia bpref_null() y NO HABIA EXCEPCION QUE
     * LANZAR. El programa seguia con refs nulas y el fallo aparecia diez pasos
     * mas alla, disfrazado de cadenas vacias (#355, una manana entera).
     *
     * El sitio importa: soltarla en el camino de reserva normal seria peor que
     * no tenerla, porque se la quedaria el programa. Aqui solo se gasta cuando
     * ya estamos construyendo un error, que es para lo que existe. */
    vm->building_error = 1;

    /* Alocar el string del mensaje. */
    size_t mlen = msg ? strlen(msg) : 0;
    uint32_t msg_ref = bpvm_heap_alloc_string(vm, msg ? msg : "", mlen);
    if (msg_ref == 0) {
        /* #430 — sin memoria ni para el mensaje: la PREFABRICADA (idea de
         * Eduardo). Construida en el prologo del RUN cuando construir era
         * gratis; lanzarla no aloja NADA. El detalle especifico ya quedo en
         * vm->runtime_error (arriba) para el reporte de no-atrapados; el
         * catch de BP ve e.msg="No space in heap", que es la verdad. */
        if (vm->oom_exc.v != 0) {
            bpvm_diag("[bpvm] throw: sin memoria para el mensaje -> "
                      "excepcion PREFABRICADA (#430)");
            vm->building_error = 0;
            return vm->oom_exc;
        }
        bpvm_diag_urgente("[bpvm] throw: sin memoria para el MENSAJE (%u B) ni con la "
                  "reserva de emergencia soltada", (unsigned) mlen);
        vm->building_error = 0;
        return bpref_null();
    }
    /* GC-safety: el msg AÚN no vive en ninguna raíz y la alocación del objeto de
     * abajo PUEDE disparar el GC (umbral/OOM) → anclarlo para que no lo recicle
     * (su gen queda intacta, clave para el bpref_regen de más abajo). */
    tc->alloc_anchor = (int32_t) msg_ref;

    /* Alocar el objeto RuntimeError. */
    uint16_t num_fields = bpvm_read_u16_be(vm->memory + class_ptr + BPVM_CLS_OFF_NUM_FIELDS);
    uint32_t obj_addr = bpvm_heap_alloc(vm, (uint32_t) num_fields * 4, BPVM_TYPE_OBJECT);
    if (obj_addr == 0) {
        if (vm->oom_exc.v != 0) {   /* #430: idem — la prefabricada */
            bpvm_diag("[bpvm] throw: sin memoria para el objeto -> "
                      "excepcion PREFABRICADA (#430)");
            vm->building_error = 0;
            return vm->oom_exc;
        }
        bpvm_diag_urgente("[bpvm] throw: mensaje OK pero sin memoria para el OBJETO "
                  "RuntimeError (%u campos)", (unsigned) num_fields);
        vm->building_error = 0;
        return bpref_null();
    }
    bpvm_write_u32_be(vm->memory + obj_addr, class_ptr);
    /* slot 0 = msg (convención del frontend). Guardamos el HANDLE COMPLETO de 64b:
     * bpref_regen re-adjunta la gen VIVA del slot del msg (heap_alloc_string truncó
     * a uint32 → gen=0). Sin esto, si el slot del msg fue reciclado (gen ≠ 0), leer
     * e.msg daría "referencia a objeto eliminado" sobre un mensaje vivo. */
    if (num_fields > 0) {
        bpref_store(vm, obj_addr + 4 + 0 * 4, bpref_regen(vm, msg_ref));
    }
    bpref_t obj_h = bpvm_handle_register(vm, obj_addr);   /* V4: handle 64b (gen preservada — clave si el slot fue reciclado) */
    if (obj_h.v == 0u && vm->oom_exc.v != 0) {
        /* #430 — objeto construido pero sin SLOT para registrarlo: prefabricada. */
        bpvm_diag("[bpvm] throw: sin slot para la excepcion -> PREFABRICADA (#430)");
        vm->building_error = 0;
        return vm->oom_exc;
    }

    /* Anclar para GC; el caller decide si pasarlo a eh_unwind o usarlo
     * de otra forma. No tocamos el stack BP aquí — eso lo hace
     * eh_unwind tras encontrar handler. El ancla es idx|TAG. */
    tc->alloc_anchor = (int32_t) (uint32_t) obj_h.v;
    vm->building_error = 0;
    return obj_h;
}
