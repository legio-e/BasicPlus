/*
 * test_aotkill.c — [V7/H5 P2b.1.d] el `kill` de una native en un bucle SIN FIN.
 *
 * Calcado de test_kill.c: un poll que pide el kill a la vigésima vez que el planificador lo llama. El
 * planificador sólo lo llama ENTRE quantums: una native que no cede nunca no le deja llegar, y el programa no
 * se para (la receta lo corta con un tope de tiempo). Con la línea del bucle, la native cede cada rebanada y
 * el kill llega. El control: la misma prueba interpretada.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern void aot_NatKill_register(struct bpvm* vm);

static int s_polls = 0;
static int kill_tras_20(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    s_polls++;
    return s_polls >= 20;
}

static long long ahora_ms(void) {
    return (long long) clock() * 1000LL / CLOCKS_PER_SEC;
}

static int pasada(const char* nombre, const char* mod_path, int con_aot) {
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    bpvm_t* vm;
    bpvm_status_t s;
    long long t0, dt;
    int ok;
    if (!mem) { printf("OOM\n"); return 1; }
    vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { printf("bpvm_init failed\n"); free(mem); return 1; }
    s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        printf("load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return 1;
    }
    if (con_aot) aot_NatKill_register(vm);
    s_polls = 0;
    bpvm_set_poll(vm, kill_tras_20, NULL);
    t0 = ahora_ms();
    s = bpvm_run(vm);
    dt = ahora_ms() - t0;
    ok = (s == BPVM_KILLED) && (dt < 5000);
    printf("  %s: %s: status=%s (%lld ms)\n", ok ? "ok  " : "FAIL", nombre, bpvm_status_str(s), dt);
    bpvm_destroy(vm); free(mem);
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    const char* mod_path = (argc > 1) ? argv[1] : "NatKill.mod";
    int rc = 0;
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    rc |= pasada("interpretado (el control)", mod_path, 0);
    rc |= pasada("con el AOT: una native en un bucle sin fin", mod_path, 1);
    printf("[status=%s]\n", rc ? "FAIL" : "OK");
    return rc;
}
