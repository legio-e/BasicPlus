/*
 * test_heapdescarrilado.c — V7 #494 (27-sep): el HEAP DESCARRILADO es UNA EXCEPCIÓN (decisión de Eduardo: ni avisar
 * y seguir, que era barrer con el mapa truncado y llevarse objetos vivos, ni parar la VM).
 *
 * Corre samples/HeapRoto.mod —reserva cadenas en un bucle dentro de un try— con un gancho de depuración que, en su
 * primera llamada con el heap ya ocupado, estropea la cabecera del PRIMER bloque: un array de bytes con un largo que se
 * sale del heap entero, así que el recorrido del siguiente GC no acaba en heap_next. Se comprueba que ese GC no barre y
 * que la reserva que lo pidió lanza la prefabricada «heap descarrilado (fallo interno de la VM)»: el programa la atrapa
 * y la imprime, y la ejecución acaba bien. El CONTROL, sin estropear nada: el programa acaba «sin excepcion» y ningún
 * GC descarrila. La salida del programa se recoge con el `line` del hilo io.
 *
 * El mismo caso en miVM: HeapDescarriladoTest (lexer-java), con VirtualMachine.estropeaHeapParaPrueba.
 *
 * V7/H7 #598 (28-sep) — una TERCERA pasada, sin estropear nada: la reserva del mapa de cabeceras del primer GC falla
 * (bpvm_alloc_prueba_fallar, la inyección de fallos del núcleo), como en una placa sin margen de malloc. Tiene que ser
 * la MISMA excepción y nada barrido: HeapRoto guarda una cadena de antes de la primera colecta y la imprime al final.
 * Sin el arreglo, ese GC no marcaba nada y lo barría TODO: el bucle seguía («sin excepcion») y leer la cadena guardada
 * era un «objeto eliminado». Sólo VM-C: en miVM el conjunto de cabeceras es un HashSet de Java.
 *
 * V7 #659 (7-oct) — una QUINTA, la de dos tareas: la marca de la reserva que falló es de la TAREA que reservó. Tras el
 * RUN del control —el heap, con sitio— se estropea a mano la cabecera del primer bloque; la tarea A pide una reserva
 * imposible —la colecta descarrila y la marca queda en A—; la tarea B lanza un error, que cabe sin colectar: NO tiene
 * que llevarse la del heap descarrilado; y entonces A lanza: la suya. Cada tarea es un hilo de C (su contexto: TLS en
 * el PC), y van una tras otra. Con la marca en la VM (antes), B se la llevaba y a A le tocaba la de sin memoria: con
 * dos runs, la excepción al que no era. (Si el throw de B tuviera que colectar, su colecta descarrilaría también y la
 * excepción sería suya de verdad: por eso el heap con sitio.)
 *
 * Salida: una línea por pasada y, si las dos van, «OK: ...» (la señal de check-testc). exit 0 si pasan.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_io.h"
#include "bpvm_fs.h"
#include "bpvm_alloc.h"   /* #598: bpvm_alloc_prueba_fallar */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>   /* #659: las dos tareas */

static int s_estropear = 0;
static int s_estropeado = 0;
static char s_salida[4096];
static size_t s_n = 0;

static void gancho(bpvm_t* vm, bpvm_thread_t* tc, uint32_t pc, int line, const char* source, void* user) {
    (void) tc; (void) pc; (void) line; (void) source; (void) user;
    if (!s_estropear || s_estropeado) return;
    if (vm->heap_next <= vm->heap_start) return;   /* todavía no hay ningún bloque */
    s_estropeado = 1;
    bpvm_write_u32_be(vm->memory + vm->heap_start, (uint32_t) BPVM_TYPE_ARRAY_I8 << BPVM_TAG_TYPE_SHIFT);
    if (s_estropear == 1) {
        /* Un largo que se sale del heap ENTERO (no sólo del usado): con uno menor, el salto puede caer en el borde de
         * otro bloque —las cadenas del programa miden todas lo mismo— y el recorrido se resincroniza en silencio (el
         * agujero del guardián que ya contaba heap.c, #355). Así el recorrido no puede acabar en heap_next. */
        bpvm_write_u32_be(vm->memory + vm->heap_start + 4, (vm->heap_top - vm->heap_start) + 4096u);
    } else {
        /* V7/H7 #593 (29-sep) — ESE agujero, a propósito: un largo que se traga JUSTO el bloque siguiente (medido en
         * esta VM: el primero es el mensaje de la excepción prefabricada del OOM y el segundo, su objeto, VIVO). El
         * recorrido salta al tercero, se recompone y acaba en heap_next: el guardián de #494 no lo ve, y sin #593 el
         * GC mataba el handle de la prefabricada sin decir nada y el programa seguía («sin excepcion»). */
        uint32_t t1 = bpvm_heap_bloque_tam(vm, vm->heap_start);
        uint32_t t2 = bpvm_heap_bloque_tam(vm, vm->heap_start + t1);
        bpvm_write_u32_be(vm->memory + vm->heap_start + 4, t1 + t2 - BPVM_OBJ_HEADER_SIZE);
    }
}

static void recoge(const char* s, size_t len, void* user) {
    (void) user;
    if (s_n + len >= sizeof s_salida) len = sizeof s_salida - 1 - s_n;
    memcpy(s_salida + s_n, s, len);
    s_n += len;
    s_salida[s_n] = 0;
}

/* Una pasada; devuelve 0 si PASS. `sin_mapa` (#598): sin estropear, falla la reserva del mapa del primer GC. */
static int corre(const char* mod, int estropear, int sin_mapa, const char* esperado) {
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init fallo\n"); free(mem); return 1; }
    bpvm_status_t s = bpvm_load_mod(vm, mod);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return 1;
    }
    s_estropear = estropear;
    s_estropeado = 0;
    s_n = 0;
    s_salida[0] = 0;
    bpvm_set_debug_hook(vm, gancho, NULL, NULL);
    bpvm_io_ops_t ops;
    memset(&ops, 0, sizeof ops);
    ops.line = recoge;
    if (bpvm_io_start(vm, &ops, 0) != 0) {
        fprintf(stderr, "no arranca el hilo io\n");
        bpvm_destroy(vm); free(mem); return 1;
    }
    if (sin_mapa) bpvm_alloc_prueba_fallar("heap.c");   /* la ÚNICA reserva de heap.c: el mapa del GC */
    s = bpvm_run(vm);
    bpvm_alloc_prueba_fallar(NULL);
    bpvm_io_stop(vm);
    int descarrilo = vm->gc_descarrilo;
    /* Sin mapa, el GC siguiente ya lo tiene: el último no descarrila, así que ahí no se mira. */
    int pass = (s == BPVM_OK) && (s_estropeado == (estropear != 0)) && (sin_mapa || descarrilo == (estropear != 0))
               && strstr(s_salida, esperado) != NULL
               && strstr(s_salida, "lo de antes: guardado 42") != NULL;   /* lo vivo, intacto */
    printf("[heap] %s: status=%s, cabecera estropeada=%d, el ultimo GC descarrilo=%d, el programa dijo: %s",
           sin_mapa ? "sin mapa        " : estropear == 2 ? "traga el siguiente" : estropear ? "estropeado      "
           : "control         ", bpvm_status_str(s), s_estropeado, descarrilo, s_salida);
    printf("[heap] %s\n", pass ? "PASS" : "FAIL");
    bpvm_destroy(vm); free(mem);
    return pass ? 0 : 1;
}

/* ── V7 #659 — la marca, de la tarea: A reserva (y descarrila), B lanza, A lanza; en ese orden ── */
static pthread_mutex_t s_m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  s_c = PTHREAD_COND_INITIALIZER;
static int s_fase = 0;
static void pon_fase(int f) { pthread_mutex_lock(&s_m); s_fase = f; pthread_cond_broadcast(&s_c); pthread_mutex_unlock(&s_m); }
static void espera_fase(int f) { pthread_mutex_lock(&s_m); while (s_fase < f) pthread_cond_wait(&s_c, &s_m); pthread_mutex_unlock(&s_m); }

typedef struct { bpvm_t* vm; bpvm_thread_t* tc; bpref_t reserva, exc; } tarea_t;

static void* tarea_a(void* p) {   /* la reserva imposible —la marca, suya— y, cuando B haya lanzado, su throw */
    tarea_t* a = (tarea_t*) p;
    a->reserva = bpvm_heap_nuevo(a->vm, BPVM_TYPE_ARRAY_I8, 0u, (a->vm->heap_top - a->vm->heap_start) + 4096u);
    pon_fase(1);
    espera_fase(2);
    a->exc = bpvm_throw_runtime_error(a->vm, a->tc, "No space in heap");
    return NULL;
}

static void* tarea_b(void* p) {   /* otra tarea lanza en medio (lo suyo cabe: el contador de volumen, a cero, para que
                                    * no colecte por adelantado) */
    tarea_t* b = (tarea_t*) p;
    espera_fase(1);
    b->vm->alloc_since_gc = 0u;
    b->exc = bpvm_throw_runtime_error(b->vm, b->tc, "de otra tarea");
    pon_fase(2);
    return NULL;
}

static int corre_dos_tareas(const char* mod) {
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm || bpvm_load_mod(vm, mod) != BPVM_OK) { fprintf(stderr, "no carga %s\n", mod); if (vm) bpvm_destroy(vm); free(mem); return 1; }
    s_estropear = 0; s_estropeado = 0; s_n = 0; s_salida[0] = 0;   /* el control: el heap, con sitio */
    bpvm_set_debug_hook(vm, gancho, NULL, NULL);
    bpvm_io_ops_t ops;
    memset(&ops, 0, sizeof ops);
    ops.line = recoge;
    if (bpvm_io_start(vm, &ops, 0) != 0) { fprintf(stderr, "no arranca el hilo io\n"); bpvm_destroy(vm); free(mem); return 1; }
    bpvm_status_t s = bpvm_run(vm);
    bpvm_io_stop(vm);
    bpvm_set_debug_hook(vm, NULL, NULL, NULL);
    /* La cabecera del primer bloque, estropeada a mano (lo de la pasada «estropeado»): la colecta siguiente descarrila. */
    int hay_bloque = vm->heap_next > vm->heap_start;
    if (hay_bloque) {
        bpvm_write_u32_be(vm->memory + vm->heap_start, (uint32_t) BPVM_TYPE_ARRAY_I8 << BPVM_TAG_TYPE_SHIFT);
        bpvm_write_u32_be(vm->memory + vm->heap_start + 4, (vm->heap_top - vm->heap_start) + 4096u);
    }
    tarea_t a, b;
    memset(&a, 0, sizeof a); memset(&b, 0, sizeof b);
    a.vm = b.vm = vm;
    a.tc = b.tc = bpvm_hilos_primero(vm);
    s_fase = 0;
    pthread_t ta, tb;
    int pass = (s == BPVM_OK) && hay_bloque && vm->heap_exc.v != 0u && a.tc != NULL
               && pthread_create(&ta, NULL, tarea_a, &a) == 0;
    if (pass) {
        pthread_create(&tb, NULL, tarea_b, &b);
        pthread_join(tb, NULL);
        pthread_join(ta, NULL);
        pass = a.reserva.v == 0u && vm->gc_descarrilo && b.exc.v != 0u && b.exc.v != vm->heap_exc.v
               && a.exc.v == vm->heap_exc.v;
    }
    printf("[heap] dos tareas      : la reserva de A %s (el GC %s); el throw de B, %s; el de A, %s\n",
           a.reserva.v == 0u ? "fallo" : "NO fallo", vm->gc_descarrilo ? "descarrilo" : "NO descarrilo",
           b.exc.v == vm->heap_exc.v ? "LA DEL HEAP DESCARRILADO (era de A)" : b.exc.v == 0u ? "NINGUNO" : "el suyo",
           a.exc.v == vm->heap_exc.v ? "la del heap descarrilado" : "OTRA (se la llevo B)");
    printf("[heap] %s\n", pass ? "PASS" : "FAIL");
    bpvm_destroy(vm); free(mem);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();   /* el núcleo resuelve por la fachada del FS (como test_kill) */
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod = (argc > 1) ? argv[1] : "HeapRoto.mod";
    int rc = 0;
    rc |= corre(mod, 0, 0, "sin excepcion");
    rc |= corre(mod, 1, 0, "cazada: " BPVM_MSG_HEAP_DESCARRILADO);
    rc |= corre(mod, 0, 1, "cazada: " BPVM_MSG_HEAP_DESCARRILADO);   /* #598 */
    rc |= corre(mod, 2, 0, "cazada: " BPVM_MSG_HEAP_DESCARRILADO);   /* #593: el que se recomponía en silencio */
    rc |= corre_dos_tareas(mod);                                       /* #659: la marca, de la tarea */
    if (rc == 0)
        printf("OK: el heap descarrilado es una excepcion atrapable (y sin estropear, ninguna); sin memoria para el "
               "mapa del GC, la misma, y nada barrido; y un bloque que se traga justo el siguiente, tambien (#593); y la "
               "del heap descarrilado, a la tarea que reservo, no a otra (#659)\n");
    return rc;
}
