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
#include "bpvm_out.h"     /* V7/H7 #607: la línea del hilo (bpvm_linea_*), para decir que un hilo muere */
#include <stdarg.h>

/* V7/H12, 2b (la revisión) — EL ERROR DEL PROGRAMA, el que dice el EXITED (bpvm_runtime_error): lo fija el PRIMERO que
 * lo es —un hilo que muere sin atrapar, una excepción que no se pudo construir— y los de después no lo pisan. Antes lo
 * escribía CADA throw, también los que se atrapan: un hilo moría sin atrapar (#608), otro atrapaba después cualquier
 * error, y el EXITED decía el de éste. Y con dos runs, dos escrituras a la vez lo mezclaban. Bajo el cerrojo de la VM:
 * nadie llega aquí con él cogido (el intérprete, y builtin_throw, que se llama sin él). El prólogo de cada programa lo
 * deja vacío (bpvm.c). */
static void fija_runtime_error(bpvm_t* vm, const char* fmt, ...) {
    char texto[sizeof vm->runtime_error];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(texto, sizeof texto, fmt, ap);
    va_end(ap);
    bpvm_smp_lock(vm);
    if (vm->runtime_error[0] == '\0' || bpvm_la_cola.sabotaje_error)   /* (el sabotaje: SÓLO PRUEBAS, test_cola) */
        snprintf(vm->runtime_error, sizeof vm->runtime_error, "%s", texto);
    bpvm_smp_unlock(vm);
}

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
    for (int prof = 0; cur != 0 && prof < 64; prof++) {
        /* #532 — un `throw` de algo que no es un objeto (un .mod hecho a mano) traia una
         * «clase» que no es un descriptor: leer su padre era un segfault. Fuera de la
         * memoria de la VM no hay descriptor que valga. */
        if (cur + BPVM_CLS_OFF_FIELD_BITMAP > vm->memory_size) return 0;
        if (cur == target) return 1;
        int32_t parent_off = bpvm_read_i32_be(vm->memory + cur + BPVM_CLS_OFF_PARENT_OFF);
        if (parent_off == 0) return 0;
        uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, cur);
        cur = (uint32_t)((int32_t) cur_cs + parent_off);
    }
    return 0;
}

int bpvm_clase_desciende(const bpvm_t* vm, uint32_t clase, uint32_t de) {   /* V7 #668 (destruir.c) */
    return is_descendant_of(vm, clase, de);
}

/* V7 #578 — el `catch` por clase de una native: la misma subida que is_descendant_of, mirando el NOMBRE de cada clase
 * (el canónico de la 4.5 de su módulo). Una madre sin nombre (de un módulo sin 4.5) no puede ser la del `catch`: el
 * enlace no deja que un módulo nuevo nombre una clase de uno así. */
int bpvm_clase_se_llama(const bpvm_t* vm, uint32_t clase, const char* nombre) {
    uint32_t cur = clase;
    for (int prof = 0; cur != 0 && prof < 64; prof++) {
        if (cur + BPVM_CLS_OFF_FIELD_BITMAP > vm->memory_size) return 0;
        char nm[256];
        if (bpvm_link_nombre_de_clase(vm, cur, nm, sizeof nm) && strcmp(nm, nombre) == 0) return 1;
        int32_t parent_off = bpvm_read_i32_be(vm->memory + cur + BPVM_CLS_OFF_PARENT_OFF);
        if (parent_off == 0) return 0;
        uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, cur);
        cur = (uint32_t)((int32_t) cur_cs + parent_off);
    }
    return 0;
}

/* V7/H7 #607 (30-sep, Eduardo: «lo que sea pero que no sea mudo») — un hilo que NO es el main muere por una
 * excepción que nadie atrapa: se DICE en la salida del programa —la consola del PC y la del IDE con la placa—, no
 * sólo por el diagnóstico, que en la placa va al log. Pasa sobre todo mientras se desarrolla (el manejador de un
 * botón que falla deja la pantalla quieta), y sin esta línea no hay ninguna pista. Sólo el MENSAJE: la clase sólo se
 * sabe en MOD9, y el número del hilo lo reutiliza #604 (las dos VMs no darían lo mismo); el detalle sigue en el
 * diagnóstico. Por piezas a la línea del hilo (#600), sin otro búfer en la pila. La del main acaba el programa y ya
 * la dice el fin del RUN. Espejo de informaHiloMuerto (miVM). */
static void informa_hilo_muerto(bpvm_t* vm, bpvm_thread_t* tc, const char* texto, size_t n) {
    static const char pre[] = "=== excepcion no atrapada en un hilo (el hilo termina): ";
    if (tc->id == 0) return;
    bpvm_linea_pieza(vm, tc, pre, sizeof pre - 1);
    bpvm_linea_pieza(vm, tc, texto, n);
    bpvm_linea_fin(vm, tc, " ===", 4);
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
            /* V7/H9 #622 — ya está en la pila del manejador: el ancla que la protegía desde que se construyó
             * (bpvm_throw_runtime_error) sobra, y no se soltaba nunca —la última excepción que la VM construyó
             * en el hilo seguía siendo raíz, con su mensaje—. La VM-Java suelta la suya al empezar cada
             * instrucción. */
            tc->alloc_anchor = 0;
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
    /* #532 — la CLASE de la excepcion, por su nombre (la 4.5 de su modulo: en MOD7/8 no hay y
     * se queda como estaba), y la guardia de que DESCIENDE de Core.Exception antes de leer
     * su campo 0 como el mensaje: un `throw` de un objeto que no es una excepcion (a mano;
     * desde una dep lo para el E10) derefenciaba un entero como handle y hacia segfault. */
    thrown_class = bpref_is_null(ref) ? 0u : (uint32_t) bpvm_read_i32_be(vm->memory + bpref_deref(vm, ref));
    char clase[256]; bpvm_link_nombre_de_clase(vm, thrown_class, clase, sizeof clase);
    uint32_t exc_cls = bpvm_link_lookup(vm, "Core.Exception");
    int es_excepcion = (exc_cls == 0u) || (thrown_class != 0u && is_descendant_of(vm, thrown_class, exc_cls));
    if (!es_excepcion) {
        bpvm_diag("[bpvm]   lo lanzado no es una excepcion (clase %s)", clase[0] ? clase : "desconocida");
        if (clase[0]) fija_runtime_error(vm, "throw de un objeto que no es una excepcion: %.140s", clase);
        else          fija_runtime_error(vm, "throw de un objeto que no es una excepcion");
        informa_hilo_muerto(vm, tc, "throw de un objeto que no es una excepcion",
                            strlen("throw de un objeto que no es una excepcion"));   /* #607 */
    } else
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
            /* #532 — con la clase delante cuando se sabe: «excepcion no atrapada: Lib.MiError: texto». */
            if (n > 0 && clase[0]) {
                fija_runtime_error(vm, "excepcion no atrapada: %.70s: %.90s", clase, buf);
            } else if (n > 0) {
                fija_runtime_error(vm, "excepcion no atrapada: %.160s", buf);
            } else if (clase[0]) {
                fija_runtime_error(vm, "excepcion no atrapada: %.150s (sin mensaje)", clase);
            } else {
                fija_runtime_error(vm, "excepcion no atrapada (sin mensaje)");
            }
            if (n > 0) informa_hilo_muerto(vm, tc, buf, n);                                  /* #607 */
            else       informa_hilo_muerto(vm, tc, "(sin mensaje)", strlen("(sin mensaje)"));
        } else {
            /* Ni siquiera hay objeto de mensaje. Que se sepa QUE paso: «exit 11»
             * a secas no distingue esto de un fallo de la propia VM. */
            if (clase[0]) fija_runtime_error(vm, "excepcion no atrapada: %.150s (sin mensaje)", clase);
            else          fija_runtime_error(vm, "excepcion no atrapada (sin mensaje)");
            informa_hilo_muerto(vm, tc, "(sin mensaje)", strlen("(sin mensaje)"));        /* #607 */
        }
    }
    tc->status = BPVM_THREAD_TERMINATED;
    return 0;
}

const char* bpvm_runtime_error(const bpvm_t* vm) {
    return (vm && vm->runtime_error[0]) ? vm->runtime_error : "";
}

/* V7 #494 — si la última reserva falló porque el recorrido del heap se descarriló, la excepción es la
 * PREFABRICADA del heap descarrilado —reservar ya no es seguro—, sea cual sea el mensaje con que se pida (casi
 * siempre el de sin memoria, que es lo que dicen los que reciben un 0 de bpvm_heap_alloc). Consume la marca.
 * bpref_null() si no toca. Espejo de miVM, donde heapAlloc la lanza directamente. V7 #659: la marca es de la TAREA que
 * reservó (bpvm_internal.h, bpvm_aot_callctx_t): con dos runs, el `throw` de otro ya no se la lleva. */
static bpref_t exc_heap_descarrilado(bpvm_t* vm) {
    bpvm_aot_callctx_t* cc = bpvm_aot_callctx();
    if (!cc->reserva_descarrilada) return bpref_null();
    cc->reserva_descarrilada = 0;
    if (!vm->gc_descarrilo || vm->heap_exc.v == 0u) return bpref_null();   /* una de otro RUN, sin consumir: no vale */
    /* (V7/H12, 2b: aquí se fijaba también el error del programa; ya no —puede atraparse—: si no se atrapa, lo fija el
     * camino de «no atrapada», con su clase y este mismo mensaje) */
    return vm->heap_exc;
}

bpref_t bpvm_throw_runtime_error(bpvm_t* vm, bpvm_thread_t* tc,
                                  const char* msg) {
    {
        bpref_t d = exc_heap_descarrilado(vm);   /* V7 #494 */
        if (d.v != 0u) return d;
    }
    /* (Aquí se guardaba el detalle en CADA throw, para el EXITED si acababa sin atrapar. V7/H12, 2b: ya no —lo pisaba
     * un throw atrapado después, de otro hilo—. Si la excepción se construye y no se atrapa, lo fija el camino de «no
     * atrapada» con su texto; si NO se puede construir, el detalle lo fija aquí abajo cada salida nula, y el hilo muere
     * con él. fija_runtime_error, arriba.) */
    if (bpvm_la_cola.sabotaje_error)   /* SÓLO PRUEBAS (test_cola): el camino de antes */
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
         * util. De paso cierra uno de los 29 fprintf de #353.
         *
         * V6/P1.C3.3 — Y DICE CUAL DE LAS DOS COSAS ES. Este mismo aviso lo
         * disparaba tambien la PREFABRICACION del OOM (`#430`), que corre en
         * todo arranque de programa y que en un modulo que no importa `Core` no
         * puede hacerse — y no pasa nada, el codigo lo contempla
         * (`.v == 0 si ni esto se pudo`). Sonaba identico a un OOM real, con la
         * misma frase y el mismo texto «No space in heap», en el log de una
         * placa. Me costo un diagnostico entero: di por hecho que el programa
         * se habia quedado sin heap y llegue a cambiar el tamano del bloque de
         * la VM por eso. Un aviso que no distingue un no-evento de un fallo es
         * peor que no tenerlo. */
        if (vm->prefabricando_oom) {
            bpvm_diag("[bpvm] OOM sin prefabricar: este modulo no exporta "
                      "RuntimeError. No es un fallo; si de verdad falta memoria, "
                      "se reportara sin excepcion.");
        } else {
            bpvm_diag_urgente("[bpvm] throw: SIN CLASE RuntimeError exportada, no hay con "
                      "que construir la excepcion: %s", msg ? msg : "");
            fija_runtime_error(vm, "%s", msg ? msg : "");   /* V7/H12, 2b: el detalle, para el EXITED */
        }
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
     * ya estamos construyendo un error, que es para lo que existe.
     *
     * V7/H12, 2b (5-oct): «estamos construyendo un error» lo dice cada una de las dos reservas (las _error), no una
     * bandera de la VM: con dos runs, la del otro run. */

    /* Alocar el string del mensaje. */
    size_t mlen = msg ? strlen(msg) : 0;
    uint32_t msg_ref = bpvm_heap_alloc_string_error(vm, msg ? msg : "", mlen);
    if (msg_ref == 0) {
        bpref_t d = exc_heap_descarrilado(vm);   /* V7 #494: el GC de esta reserva descarriló */
        if (d.v != 0u) return d;
        /* #430 — sin memoria ni para el mensaje: la PREFABRICADA (idea de
         * Eduardo). Construida en el prologo del RUN cuando construir era
         * gratis; lanzarla no aloja NADA. El catch de BP ve e.msg="No space in
         * heap", que es la verdad (el detalle, en el aviso). */
        if (vm->oom_exc.v != 0) {
            bpvm_diag("[bpvm] throw: sin memoria para el mensaje -> "
                      "excepcion PREFABRICADA (#430)");
            return vm->oom_exc;
        }
        bpvm_diag_urgente("[bpvm] throw: sin memoria para el MENSAJE (%u B) ni con la "
                  "reserva de emergencia soltada", (unsigned) mlen);
        fija_runtime_error(vm, "%s", msg ? msg : "");   /* V7/H12, 2b: el detalle, para el EXITED */
        return bpref_null();
    }
    /* GC-safety: el msg AÚN no vive en ninguna raíz y la alocación del objeto de
     * abajo PUEDE disparar el GC (umbral/OOM) → anclarlo para que no lo recicle
     * (su gen queda intacta, clave para el bpref_regen de más abajo). */
    tc->alloc_anchor = (int32_t) msg_ref;

    /* Alocar el objeto RuntimeError. */
    uint16_t num_fields = bpvm_read_u16_be(vm->memory + class_ptr + BPVM_CLS_OFF_NUM_FIELDS);
    bpref_t obj_h = bpvm_heap_nuevo_error(vm, BPVM_TYPE_OBJECT, class_ptr, (uint32_t) num_fields * 4);   /* V7/H7 #593: con su clase y su handle, al nacer */
    if (obj_h.v == 0u) {
        bpref_t d = exc_heap_descarrilado(vm);   /* V7 #494: idem */
        if (d.v != 0u) return d;
        if (vm->oom_exc.v != 0) {   /* #430: idem — la prefabricada */
            bpvm_diag("[bpvm] throw: sin memoria para el objeto -> "
                      "excepcion PREFABRICADA (#430)");
            return vm->oom_exc;
        }
        bpvm_diag_urgente("[bpvm] throw: mensaje OK pero sin memoria para el OBJETO "
                  "RuntimeError (%u campos)", (unsigned) num_fields);
        fija_runtime_error(vm, "%s", msg ? msg : "");   /* V7/H12, 2b: el detalle, para el EXITED */
        return bpref_null();
    }
    /* La clase ya está en la cabecera y el handle dado de alta (el método único, #593); nulo arriba era también
     * «sin SLOT en la tabla» (#430), con su prefabricada. */
    uint32_t obj_addr = bpref_deref(vm, obj_h);
    /* slot 0 = msg (convención del frontend). Guardamos el HANDLE COMPLETO de 64b:
     * bpref_regen re-adjunta la gen VIVA del slot del msg (heap_alloc_string truncó
     * a uint32 → gen=0). Sin esto, si el slot del msg fue reciclado (gen ≠ 0), leer
     * e.msg daría "referencia a objeto eliminado" sobre un mensaje vivo. */
    if (num_fields > 0) {
        bpref_store(vm, obj_addr + 4 + 0 * 4, bpref_regen(vm, msg_ref));
    }

    /* Anclar para GC; el caller decide si pasarlo a eh_unwind o usarlo
     * de otra forma. No tocamos el stack BP aquí — eso lo hace
     * eh_unwind tras encontrar handler. El ancla es idx|TAG. */
    tc->alloc_anchor = (int32_t) (uint32_t) obj_h.v;
    return obj_h;
}
