/*
 * test_aotgc.c — #302 paso 3: ¿puede el GC reciclar un objeto cuyo único
 * handle vive en un REGISTRO DE C de una función native compilada?
 *
 * El paso 3 (raíces GC del native COMPILADO) se difirió con este argumento:
 * «el native corre síncrono dentro de un quantum sin GC asíncrono y F2 no
 * compacta». Este test comprueba si ese argumento SIGUE EN PIE en V4:
 *
 *   - el GC ya NO es solo-bump: recicla (free-list) y MATA handles de objetos
 *     inalcanzables (gc_table_sweep_phase, gen bump);
 *   - y se dispara DENTRO de bpvm_heap_alloc — también cuando quien aloca es
 *     un helper llamado desde código nativo.
 *
 * El escenario, que es RoTest tal cual: `"valor " + intToString(n)` emite
 *
 *     string_concat(vm, string_from_cstr(...), int_to_string(...))
 *
 * El handle que devuelve la primera alocación espera EN UN TEMPORAL DE C
 * mientras la segunda aloca. Si esa segunda dispara el GC, ese handle no está
 * en ninguna raíz que el marcado vea: ni en la pila BP (tc->sp además queda
 * RANCIO durante el native — el camino AOT no lo sincroniza como hacen los 19
 * safepoints del intérprete), ni en los globales. Objeto inalcanzable →
 * handle muerto → la concat usa un handle asesinado.
 *
 * Para no depender de la lotería del umbral, aquí el GC se fuerza a colecta
 * POR CADA ALOCACIÓN (gc_bump_threshold = 1), que es la técnica de siempre
 * para bugs probabilísticos: convertir la ventana en certeza.
 *
 * VERDE = el programa imprime lo suyo (el argumento del aplazamiento aguanta).
 * ROJO  = UAF/handle muerto en un programa CORRECTO → el paso 3 ya no es
 *         aplazable con ese argumento.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_RoTest_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "RoTest.mod";
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    bpvm_status_t s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    aot_RoTest_register(vm);       /* hijack: etiqueta() → thunk compilado */
    vm->gc_bump_threshold = 1;     /* GC en CADA alocación: la ventana, segura */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
