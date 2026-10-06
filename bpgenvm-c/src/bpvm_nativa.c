/*
 * bpvm_nativa.c — el ejecutor de una native (V7/H5 P2b.3). Ver bpvm_nativa.h.
 */
#include "bpvm_nativa.h"
#include "bpvm_alloc.h"
#include "bpvm_platform.h"
#include "bpvm_aot_helpers.h"

#include <stdint.h>
#include <string.h>

/* Pasar el testigo: dar el del otro y esperar el propio. */
static void pasar(bpvm_platform_sem_handle_t* al_otro, bpvm_platform_sem_handle_t* el_mio) {
    bpvm_platform_sem_give(al_otro);
    bpvm_platform_sem_take(el_mio);
}

/* El hilo del ejecutor: espera un encargo, corre la native con su límite de fallos (#186), le dice al intérprete
 * que ha terminado y vuelve a esperar. La orden de terminar (bpvm_destroy) lo saca del bucle; si lo pilla parado A
 * MITAD de una native, salta aquí desde donde esperaba (bpvm_nativa_ceder), abandonando esos marcos. */
static void ejecutor(void* arg) {
    bpvm_nativa_t* n = (bpvm_nativa_t*) arg;
    if (setjmp(n->fondo) == 0) {
        bpvm_platform_sem_take(&n->sem_nativa);          /* el primer encargo (o la orden de terminar) */
        while (!n->terminar) {
            bpvm_aot_callctx()->ejecutor = n;            /* «corro en un ejecutor»: el GC lo mira */
            n->resultado = bpvm_aot_call_guarded(n->vm, n->tc, n->thunk, &n->sp, &n->bp);
            bpvm_aot_callctx()->ejecutor = NULL;
            n->estado = BPVM_NATIVA_TERMINADA;
            pasar(&n->sem_worker, &n->sem_nativa);       /* al intérprete; y el siguiente encargo */
        }
    }
    n->pila_libre = bpvm_platform_pila_libre_actual();
    bpvm_platform_sem_give(&n->sem_worker);              /* terminado: a quien lo terminó */
}

/* Un ejecutor libre del depósito, o uno nuevo, YA RESERVADO para quien lo pide (CORRIENDO: lo va a usar en el acto).
 * NULL si no hay memoria para él. V7/H12 — con dos runs, dos natives a la vez no pueden coger el mismo: mirar y
 * reservar van juntos bajo el cerrojo de la VM, y uno nuevo entra en la lista con él (con un run, nada). Antes se
 * devolvía el libre sin reservarlo y lo marcaba quien llamaba, más tarde. (El sabotaje y la ventana: SÓLO PRUEBAS.) */
static bpvm_nativa_t* ejecutor_libre(bpvm_t* vm) {
    bpvm_nativa_t* n;
    const int cerrojo = !bpvm_la_cola.sabotaje_deposito;
    if (bpvm_la_cola.ensayo_sin_ejecutor) return NULL;   /* SÓLO PRUEBAS: como sin memoria para uno */
    if (cerrojo) bpvm_smp_lock(vm);
    for (n = vm->nativas; n != NULL; n = n->sig)
        if (n->estado == BPVM_NATIVA_LIBRE) break;
    if (n != NULL) {
        if (bpvm_la_cola.ensayo_deposito_ms > 0) bpvm_platform_thread_sleep_ms(bpvm_la_cola.ensayo_deposito_ms);
        n->estado = BPVM_NATIVA_CORRIENDO;
    }
    if (cerrojo) bpvm_smp_unlock(vm);
    if (n != NULL) return n;
    n = (bpvm_nativa_t*) bpvm_malloc(sizeof *n);
    if (n == NULL) return NULL;
    memset(n, 0, sizeof *n);
    n->vm = vm;
    n->pila_libre = -1;
    n->estado = BPVM_NATIVA_CORRIENDO;
    if (bpvm_platform_sem_init(&n->sem_nativa) != 0) { bpvm_free(n); return NULL; }
    if (bpvm_platform_sem_init(&n->sem_worker) != 0) {
        bpvm_platform_sem_destroy(&n->sem_nativa);
        bpvm_free(n);
        return NULL;
    }
    if (bpvm_platform_thread_create_nativa(&n->hilo, ejecutor, n, BPVM_NATIVA_PILA_BYTES) != 0) {
        bpvm_platform_sem_destroy(&n->sem_nativa);
        bpvm_platform_sem_destroy(&n->sem_worker);
        bpvm_free(n);
        return NULL;
    }
    if (cerrojo) bpvm_smp_lock(vm);
    n->sig = vm->nativas;
    vm->nativas = n;
    if (cerrojo) bpvm_smp_unlock(vm);
    return n;
}

/* De vuelta al depósito: el hilo BP ya no está dentro de ninguna native. V7/H12: bajo el cerrojo de la VM, como quien
 * lo busca (con un run, nada). */
static void soltar(bpvm_nativa_t* n) {
    const int cerrojo = !bpvm_la_cola.sabotaje_deposito;
    if (cerrojo) bpvm_smp_lock(n->vm);
    if (n->tc) n->tc->nativa = NULL;
    n->tc = NULL;
    n->estado = BPVM_NATIVA_LIBRE;
    if (cerrojo) bpvm_smp_unlock(n->vm);
}

/* El lado del intérprete: le pasa el testigo a la native —nueva o suspendida— y vuelve cuando termina o se
 * suspende. Si en medio pide el GC, lo corre aquí, en la pila del worker, y se lo devuelve. Alrededor, la vista de
 * la TLS del worker: se guarda al pasar el testigo y se repone al recuperarlo (la native pone la suya). */
static int entrar(bpvm_nativa_t* n) {
    n->f_worker = *bpvm_aot_fault_slot();
    n->cc_worker = *bpvm_aot_callctx();
    n->t_corriendo = bpvm_platform_now_ms();   /* P2b.1.d: su rebanada empieza ahora */
    for (;;) {
        pasar(&n->sem_nativa, &n->sem_worker);
        *bpvm_aot_fault_slot() = n->f_worker;
        *bpvm_aot_callctx() = n->cc_worker;
        if (n->estado != BPVM_NATIVA_PIDE_GC) break;
        /* Mientras corre el GC la native sigue como PIDE_GC: el GC escanea su pila como la de una suspendida. */
        bpvm_gc_cuerpo(n->vm);
        n->estado = BPVM_NATIVA_CORRIENDO;
    }
    if (n->estado == BPVM_NATIVA_SUSPENDIDA) return BPVM_NATIVA_SUSPENDIDA_R;
    return n->resultado;   /* TERMINADA */
}

int bpvm_nativa_llamar(bpvm_t* vm, bpvm_thread_t* tc, bpvm_aot_thunk_t aot, uint32_t flags, uint32_t* sp,
                       uint32_t* bp, uint32_t pc_vuelta, uint32_t cs_vuelta) {
    bpvm_nativa_t* n = tc->nativa;
    int r;
    /* Directa (sin puntos de cesión) o anidada (el hilo ya corre en su ejecutor: native → BP → native): una llamada
     * de C más, en la pila en la que se esté. */
    if ((flags & BPVM_AOT_DIRECTA) || n != NULL)
        return bpvm_aot_call_guarded(vm, tc, aot, sp, bp);
    n = ejecutor_libre(vm);
    if (n == NULL) {
        /* Sin memoria para un ejecutor: corre como hasta P2b, en la pila del worker. Se dice una vez. */
        static int dicho = 0;
        if (!dicho) {
            dicho = 1;
            bpvm_diag_urgente("[bpvm] native sin ejecutor (no caben %u B de pila): corre en la del worker",
                              (unsigned) BPVM_NATIVA_PILA_BYTES);
        }
        return bpvm_aot_call_guarded(vm, tc, aot, sp, bp);
    }
    tc->nativa = n;
    n->tc = tc;
    n->thunk = aot;
    n->sp = *sp;
    n->bp = *bp;
    n->pc_vuelta = pc_vuelta;
    n->cs_vuelta = cs_vuelta;
    n->estado = BPVM_NATIVA_CORRIENDO;
    r = entrar(n);
    if (r != BPVM_NATIVA_SUSPENDIDA_R) {   /* terminó sin suspenderse: el intérprete sigue aquí mismo */
        *sp = n->sp;
        *bp = n->bp;
        soltar(n);
    }
    return r;
}

int bpvm_nativa_reanudar(bpvm_t* vm, bpvm_thread_t* tc, uint32_t* pc, uint32_t* cs, uint32_t* sp, uint32_t* bp) {
    bpvm_nativa_t* n = tc->nativa;
    int r;
    (void) vm;
    n->estado = BPVM_NATIVA_CORRIENDO;
    r = entrar(n);
    if (r != BPVM_NATIVA_SUSPENDIDA_R) {
        *pc = n->pc_vuelta;
        *cs = n->cs_vuelta;
        *sp = n->sp;
        *bp = n->bp;
        soltar(n);
    }
    return r;
}

/* El lado de la native: devuelve el testigo con su estado ya puesto y espera a que se lo devuelvan. Antes vuelca
 * los registros a su pila (setjmp) y apunta dónde se para: el GC escanea desde ahí —el truco de #302—. */
static void parar(bpvm_nativa_t* n, int estado) {
    jmp_buf regs;
    uintptr_t aqui;
    if (setjmp(regs) == 0) { /* volcado de registros, no un salto */ }
    aqui = (uintptr_t) &regs;
    if ((uintptr_t) &aqui < aqui) aqui = (uintptr_t) &aqui;   /* el menor manda */
    n->sp_parado = aqui;
    n->f_nativa = *bpvm_aot_fault_slot();
    n->cc_nativa = *bpvm_aot_callctx();
    n->estado = estado;
    pasar(&n->sem_worker, &n->sem_nativa);
    if (n->terminar) longjmp(n->fondo, 1);   /* la orden de terminar: al fondo del ejecutor */
    /* Reanudada —quizá por otro worker, con su propia TLS—: la vista de la native, otra vez. */
    *bpvm_aot_fault_slot() = n->f_nativa;
    *bpvm_aot_callctx() = n->cc_nativa;
}

int bpvm_nativa_ceder(bpvm_t* vm, bpvm_thread_t* tc) {
    bpvm_nativa_t* n = tc->nativa;
    (void) vm;
    if (n == NULL || n->estado != BPVM_NATIVA_CORRIENDO || bpvm_aot_callctx()->ejecutor != n) return 0;
    parar(n, BPVM_NATIVA_SUSPENDIDA);
    return 1;
}

void bpvm_nativa_gc_en_worker(bpvm_t* vm) {
    bpvm_nativa_t* n = bpvm_aot_callctx()->ejecutor;
    (void) vm;
    parar(n, BPVM_NATIVA_PIDE_GC);
}

void bpvm_nativa_punto_control(struct bpvm* vm) {
    bpvm_aot_callctx_t* cc = bpvm_aot_callctx();
    bpvm_thread_t* tc = cc->tc;
    bpvm_nativa_t* n = tc ? tc->nativa : NULL;
    if (tc == NULL) return;
    if (n != NULL && n->estado == BPVM_NATIVA_CORRIENDO && cc->ejecutor == n) {
        /* En su ejecutor: cede si hay un `kill` (el planificador lo verá y parará el programa) o si ya ha corrido
         * su rebanada (los demás hilos, y el cable: el planificador atiende al IDE entre quantums). Al reanudarse,
         * sigue en su bucle como si nada. */
        /* V7/H12: y si otro run ha parado el mundo para colectar —suspendida, su pila la escanea el colector (heap.c,
         * 2e); antes esperaba hasta acabar su rebanada—. */
        if (bpvm_kill_pedido(vm) || bpvm_platform_now_ms() - n->t_corriendo >= BPVM_NATIVA_REBANADA_MS
            || (vm->smp != NULL && vm->smp->stop_the_world)) {
            tc->status = BPVM_THREAD_RUNNABLE;
            bpvm_nativa_ceder(vm, tc);
        }
        return;
    }
    /* Sin ejecutor no puede suspenderse; pero un `kill` sí la para: lanza por el límite de fallos de #186 (si un
     * try/catch de BP lo atrapara, el planificador para el programa en el siguiente quantum). */
    if (bpvm_kill_pedido(vm))
        vm->aot_helpers->throw_runtime(vm, "el programa se ha detenido (kill) dentro de una native");
    /* V7/H12 — y el mundo parado de otro run se espera AQUÍ, en su línea de bucle, con su pila de C a la vista del
     * colector (heap.c). Antes el colector la esperaba hasta que acabase; y si su bucle esperaba a un hilo del otro
     * run, para siempre. */
    if (vm->smp != NULL && vm->smp->stop_the_world && !bpvm_la_cola.sabotaje_anidada) bpvm_stw_aparcar_anidado(vm);
}

void bpvm_nativa_liberar_todas(bpvm_t* vm) {
    bpvm_nativa_t* n = vm->nativas;
    while (n != NULL) {
        bpvm_nativa_t* sig = n->sig;
        /* Lo despierta —esté libre, esperando un encargo, o parado a mitad de una native— con la orden de
         * terminar, y espera a que acabe. */
        n->terminar = 1;
        pasar(&n->sem_nativa, &n->sem_worker);
        if (n->pila_libre >= 0)
            bpvm_diag("[bpvm] ejecutor de natives: pila de %u B, %ld sin tocar nunca (usados como mucho %ld)",
                      (unsigned) BPVM_NATIVA_PILA_BYTES, (long) n->pila_libre,
                      (long) BPVM_NATIVA_PILA_BYTES - (long) n->pila_libre);
        bpvm_platform_thread_join(&n->hilo);
        bpvm_platform_sem_destroy(&n->sem_nativa);
        bpvm_platform_sem_destroy(&n->sem_worker);
        if (n->tc) n->tc->nativa = NULL;
        bpvm_free(n);
        n = sig;
    }
    vm->nativas = NULL;
}
