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
 * Salida: una línea por pasada y, si las dos van, «OK: ...» (la señal de check-testc). exit 0 si pasan.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_io.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int s_estropear = 0;
static int s_estropeado = 0;
static char s_salida[4096];
static size_t s_n = 0;

static void gancho(bpvm_t* vm, bpvm_thread_t* tc, uint32_t pc, int line, const char* source, void* user) {
    (void) tc; (void) pc; (void) line; (void) source; (void) user;
    if (!s_estropear || s_estropeado) return;
    if (vm->heap_next <= vm->heap_start) return;   /* todavía no hay ningún bloque */
    s_estropeado = 1;
    /* Un largo que se sale del heap ENTERO (no sólo del usado): con uno menor, el salto puede caer en el borde de
     * otro bloque —las cadenas del programa miden todas lo mismo— y el recorrido se resincroniza en silencio (el
     * agujero del guardián que ya contaba heap.c, #355). Así el recorrido no puede acabar en heap_next. */
    bpvm_write_u32_be(vm->memory + vm->heap_start, (uint32_t) BPVM_TYPE_ARRAY_I8 << BPVM_TAG_TYPE_SHIFT);
    bpvm_write_u32_be(vm->memory + vm->heap_start + 4, (vm->heap_top - vm->heap_start) + 4096u);
}

static void recoge(const char* s, size_t len, void* user) {
    (void) user;
    if (s_n + len >= sizeof s_salida) len = sizeof s_salida - 1 - s_n;
    memcpy(s_salida + s_n, s, len);
    s_n += len;
    s_salida[s_n] = 0;
}

/* Una pasada; devuelve 0 si PASS. */
static int corre(const char* mod, int estropear, const char* esperado) {
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
    s = bpvm_run(vm);
    bpvm_io_stop(vm);
    int descarrilo = vm->gc_descarrilo;
    int pass = (s == BPVM_OK) && (s_estropeado == estropear) && (descarrilo == estropear)
               && strstr(s_salida, esperado) != NULL;
    printf("[heap] %s: status=%s, cabecera estropeada=%d, el ultimo GC descarrilo=%d, el programa dijo: %s",
           estropear ? "estropeado" : "control   ", bpvm_status_str(s), s_estropeado, descarrilo, s_salida);
    printf("[heap] %s\n", pass ? "PASS" : "FAIL");
    bpvm_destroy(vm); free(mem);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();   /* el núcleo resuelve por la fachada del FS (como test_kill) */
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod = (argc > 1) ? argv[1] : "HeapRoto.mod";
    int rc = 0;
    rc |= corre(mod, 0, "sin excepcion");
    rc |= corre(mod, 1, "cazada: " BPVM_MSG_HEAP_DESCARRILADO);
    if (rc == 0)
        printf("OK: el heap descarrilado es una excepcion atrapable (y sin estropear, ninguna)\n");
    return rc;
}
