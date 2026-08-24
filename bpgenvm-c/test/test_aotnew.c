/*
 * test_aotnew.c — [V6/N1.5] ¿SOBREVIVE al GC lo que una `native` FABRICA?
 *
 * Hermano de test_aotgc.c, y por la misma razón que aquél existe. Allí se
 * preguntaba si el GC podía reciclar un handle que sólo vivía en un registro de
 * C; la respuesta fue que sí, y de ahí salió el escaneo de la pila de C del
 * native (#302 paso 3, heap.c §2d, idea de Eduardo).
 *
 * N1.5 se apoya ENTERO en ese mecanismo: una native que crea un array o un
 * objeto tiene el handle recién nacido en un local de C y nada más. Si el
 * escaneo fallara, el array se recolectaría entre reservarlo y rellenarlo — y
 * no daría error: daría otro número.
 *
 * Por qué hace falta ejecutar y no basta con empaquetar: el `.mdn` de NatNew
 * compila y empaqueta para ARM y para RISC-V, pero eso sólo dice que el código
 * CABE. Aquí el mismo `.c` generado se compila con el compilador del host y se
 * EJECUTA de verdad, con `gc_bump_threshold = 1` — colecta en CADA alocación,
 * que es la técnica de siempre para volver certeza una ventana de lotería.
 *
 * VERDE = los tres valores exactos. ROJO = el GC se lleva algo a medio hacer.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_NatNew_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "NatNew.mod";
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

    aot_NatNew_register(vm);       /* hijack: las tres native → sus thunks */
    vm->gc_bump_threshold = 1;     /* GC en CADA alocación: la ventana, segura */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
