/*
 * test_aotpesada.c — V7 #592 (9-oct): una native cuyas VUELTAS SON LENTAS cede a tiempo, y el `kill` llega.
 *
 * Calcado de test_aotkill.c: un poll que pide el kill a la vigésima vez que el planificador lo llama, y el planificador
 * sólo lo llama entre quantums —cuando la native cede—. `NatPesada` llama en cada vuelta a un builtin que recorre una
 * cadena de 256 KB (el indexOf de lo que no está). Con el contador de 1024 vueltas (lo de antes de #592), la native
 * cedía cada 1024 vueltas lentas: veinte cesiones eran SEGUNDOS. Con la MARCA del 2º temporizador —el bucle llama a
 * algo, así que la mira en cada vuelta; el reloj del PC la sube cada 5 ms (en Windows, ~15)—, cada rebanada: unos
 * cientos de ms. El control, la misma interpretada (el intérprete cede cada quantum).
 *
 * El sabotaje (anotado en FICHAS #592): el emisor con el contador en los bucles que llaman a algo, o el reloj del PC
 * parado, y la pasada con el AOT pasa del tope.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern void aot_NatPesada_register(struct bpvm* vm);

#define TOPE_MS 2500   /* con la marca, unos cientos de ms; con el contador de antes, segundos */

static int s_polls = 0;
static int kill_tras_20(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    s_polls++;
    return s_polls >= 20;
}

static long long ahora_ms(void) {
    return (long long) clock() * 1000LL / CLOCKS_PER_SEC;
}

/* `registrar`: el AOT del módulo (NULL = interpretado). */
static int pasada(const char* nombre, const char* mod_path, void (*registrar)(struct bpvm*)) {
    size_t mem_size = 4u * 1024u * 1024u;   /* la cadena de 256 KB y su array, holgados */
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
    if (registrar) registrar(vm);
    s_polls = 0;
    bpvm_set_poll(vm, kill_tras_20, NULL);
    t0 = ahora_ms();
    s = bpvm_run(vm);
    dt = ahora_ms() - t0;
    ok = (s == BPVM_KILLED) && (dt < TOPE_MS);
    printf("  %s: %s: status=%s (%lld ms, tope %d)\n", ok ? "ok  " : "FAIL", nombre, bpvm_status_str(s), dt, TOPE_MS);
    bpvm_destroy(vm); free(mem);
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    const char* mod_path = (argc > 1) ? argv[1] : "NatPesada.mod";
    int rc = 0;
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    rc |= pasada("interpretada (el control)", mod_path, NULL);
    rc |= pasada("con el AOT: vueltas lentas, cede por la MARCA del 2º temporizador (#592)", mod_path,
                 aot_NatPesada_register);
    printf("[status=%s]\n", rc ? "FAIL" : "OK");
    return rc;
}
