/*
 * test_cola.c — V7/H12, etapa 2a.2 (4-oct): LA COLA y su planificador, en lo que la red no ve por sí sola
 * (docs/V7_IDEAS.md §H12 «Etapa 2a.2», las pruebas).
 *
 *  1. EL INTERBLOQUEO de verdad (Interbloqueo.mod: dos hilos y dos Mutex cruzados): RUNTIME_ERROR con un run y con dos,
 *     después de los dos «tengo el primero». Con dos runs, antes de la 2a.2, el programa se quedaba colgado.
 *  2. EL INTERBLOQUEO FALSO, que no puede salir (MutexGc.mod con dos runs y el heap justo): todas las pasadas OK y con
 *     su total. La ventana, ensanchada: el run que espera a otro colector tarda en volver a contarse (ensayo_stw_ms),
 *     con su hilo a medias y el Mutex cogido. Sabotaje (--sabotaje-contador: «nadie tiene un hilo entre manos»,
 *     contado con el contador del GC): algún interbloqueo falso —y detrás, el programa descolgado con un run aún dentro
 *     de su cuanto, que al despertar ya no lo encuentra: es lo que la regla evita (la red mira el aviso del
 *     interbloqueo en stderr)—.
 *  3. EL ÚLTIMO QUE SALE (FinJunto.mod con dos runs: dos hilos que acaban casi a la vez, cada uno en un run): el fin se
 *     decide siempre después de que vuelva el último cuanto (las dos horas, en µs). La ventana, ensanchada: cada run se
 *     entretiene después de su cuanto, con el hilo entre manos —el 0, 5 ms; el 1, 25— (ensayo_tras_cuanto_ms).
 *     Sabotaje (--sabotaje-fin: el fin sin mirar si alguien tiene un hilo entre manos): pasadas al revés.
 *  4. NADIE DENTRO AL AVISAR (FinJunto con dos runs, colgado con un aviso como el de la placa): cuando la cola avisa a
 *     quien colgó el programa —que lo destruirá—, ya no queda ningún run dentro, tampoco uno sin hilo entre manos
 *     dormido en la cola (la revisión adversarial de la 2a.2: con dos runs permanentes, el otro miraba la VM ya
 *     destruida). Sabotaje (--sabotaje-salida: el que cierra no espera a que salgan los demás): alguno dentro.
 *
 * Uso: test_cola Interbloqueo.mod MutexGc.mod FinJunto.mod [--sabotaje-contador | --sabotaje-fin | --sabotaje-salida]
 * Sale 0, con la línea «OK:» de la red, si todo pasa.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASADAS 20

typedef struct { char buf[8192]; size_t n; } salida_t;

/* La salida del programa (con dos runs y sin `io`, la llama la comm task: un solo escritor). */
static void guarda(const char* s, size_t len, void* user) {
    salida_t* o = (salida_t*) user;
    if (o->n + len >= sizeof o->buf) len = sizeof o->buf - 1 - o->n;
    memcpy(o->buf + o->n, s, len);
    o->n += len;
    o->buf[o->n] = 0;
}

static int cuenta(const char* s, const char* sub) {
    int n = 0;
    for (const char* p = strstr(s, sub); p; p = strstr(p + 1, sub)) n++;
    return n;
}

/* Una pasada: `mod` en un bloque de `mem_size` B con el heap hasta `heap` (0: la mitad), con `runs` runs. */
static bpvm_status_t corre(const char* mod, size_t mem_size, size_t heap, int runs, salida_t* out) {
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) return BPVM_ERR_OOM;
    bpvm_t* vm = bpvm_init(mem, mem_size, heap);
    if (!vm) { free(mem); return BPVM_ERR_OOM; }
    bpvm_status_t s = bpvm_load_mod(vm, mod);
    if (s != BPVM_OK) {
        fprintf(stderr, "load %s: %s\n", mod, bpvm_status_str(s));
        bpvm_destroy(vm);
        free(mem);
        return s;
    }
    memset(out, 0, sizeof *out);
    bpvm_set_output(vm, guarda, out);
    s = (runs > 1) ? bpvm_run_smp(vm, runs) : bpvm_run(vm);
    bpvm_destroy(vm);
    free(mem);
    return s;
}

static int interbloqueo(const char* mod) {
    int fallos = 0;
    for (int runs = 1; runs <= 2; runs++) {
        salida_t o;
        bpvm_status_t s = corre(mod, 512 * 1024, 0, runs, &o);
        int vistos = cuenta(o.buf, "tengo el primero");
        int ok = (s == BPVM_ERR_RUNTIME) && vistos == 2 && strstr(o.buf, "nunca llega") == NULL;
        printf("[cola] 1. interbloqueo con %d run%s: %s tras %d «tengo el primero» %s\n", runs, runs > 1 ? "s" : "",
               bpvm_status_str(s), vistos, ok ? "PASS" : "FAIL");
        if (!ok) fallos++;
    }
    return fallos;
}

static int falso(const char* mod, int sabotaje) {
    int malos = 0;
    bpvm_status_t visto = BPVM_OK;
    bpvm_la_cola.ensayo_stw_ms = 3;
    bpvm_la_cola.sabotaje_contador = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        /* El heap justo, como HeapEstres en la red: 96 KB, 32 de heap (cientos de colectas por pasada). */
        bpvm_status_t s = corre(mod, 96 * 1024, 32 * 1024, 2, &o);
        if (s != BPVM_OK || strstr(o.buf, "total = 4000") == NULL) { malos++; visto = s; }
    }
    bpvm_la_cola.ensayo_stw_ms = 0;
    bpvm_la_cola.sabotaje_contador = 0;
    printf("[cola] 2. el interbloqueo falso (dos runs, el heap justo, la ventana ensanchada%s): %d de %d bien%s%s %s\n",
           sabotaje ? ", SABOTEADO" : "", PASADAS - malos, PASADAS, malos ? "; alguna dio " : "",
           malos ? bpvm_status_str(visto) : "", malos ? "FAIL" : "PASS");
    return malos ? 1 : 0;
}

static int ultimo(const char* mod, int sabotaje) {
    int malos = 0, al_reves = 0;
    bpvm_la_cola.ensayo_tras_cuanto_ms = 5;   /* el run 0 se entretiene 5 ms tras cada cuanto; el 1, 25 */
    bpvm_la_cola.sabotaje_fin = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre(mod, 512 * 1024, 0, 2, &o);
        int64_t fin = bpvm_la_cola.t_fin, cuanto = bpvm_la_cola.t_ultimo_cuanto;
        if (fin < cuanto) al_reves++;
        if (s != BPVM_OK || !strstr(o.buf, "corto:") || !strstr(o.buf, "main:") || fin < cuanto) malos++;
    }
    bpvm_la_cola.ensayo_tras_cuanto_ms = 0;
    bpvm_la_cola.sabotaje_fin = 0;
    printf("[cola] 3. el último que sale (dos runs, la ventana ensanchada%s): %d de %d bien; el fin antes del último "
           "cuanto, %d %s\n", sabotaje ? ", SABOTEADO" : "", PASADAS - malos, PASADAS, al_reves, malos ? "FAIL" : "PASS");
    return malos ? 1 : 0;
}

/* 4. El aviso de la cola, como el de la placa (run_acabado): mira cuántos runs quedan dentro al avisar. */
static int s_dentro_al_avisar = -1;
static void al_acabar(bpvm_t* vm, int rs, void* user) {
    (void) vm; (void) rs; (void) user;
    s_dentro_al_avisar = bpvm_la_cola.dentro;
}

static int nadie_dentro(const char* mod, int sabotaje) {
    int malos = 0, con_alguien = 0;
    /* Las ventanas, ensanchadas: la de 3, porque sin ella el programa acaba antes de que el segundo run llegue a
     * entrar; y la de la salida —el run que sale tarda 30 ms en contarse fuera—, porque sin ella sale mientras el que
     * cierra escribe las líneas del final, y no queda nadie dentro al avisar (lo midió el sabotaje: 0 de 20 las dos
     * veces). */
    bpvm_la_cola.ensayo_tras_cuanto_ms = 5;
    bpvm_la_cola.ensayo_salida_ms = 30;
    bpvm_la_cola.sabotaje_salida = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        uint8_t* mem = (uint8_t*) calloc(1, 512 * 1024);
        bpvm_t* vm = mem ? bpvm_init(mem, 512 * 1024, 0) : NULL;
        if (!vm || bpvm_load_mod(vm, mod) != BPVM_OK) { malos++; if (vm) bpvm_destroy(vm); free(mem); continue; }
        salida_t o;
        memset(&o, 0, sizeof o);
        bpvm_set_output(vm, guarda, &o);
        s_dentro_al_avisar = -1;
        int s = bpvm_cola_colgar(vm, 2, 0, al_acabar, NULL);   /* dos runs temporales, con aviso */
        if (s == BPVM_OK) {
            bpvm_cola_correr_n(2);
            s = bpvm_la_cola.rs;
        }
        if (s_dentro_al_avisar > 0) con_alguien++;
        if (s != BPVM_OK || s_dentro_al_avisar != 0 || !strstr(o.buf, "corto:") || !strstr(o.buf, "main:")) malos++;
        bpvm_destroy(vm);
        free(mem);
    }
    bpvm_la_cola.ensayo_tras_cuanto_ms = 0;
    bpvm_la_cola.ensayo_salida_ms = 0;
    bpvm_la_cola.sabotaje_salida = 0;
    printf("[cola] 4. nadie dentro al avisar (dos runs, la ventana ensanchada%s): %d de %d bien; con algún run aún "
           "dentro, %d %s\n",
           sabotaje ? ", SABOTEADO" : "", PASADAS - malos, PASADAS, con_alguien, malos ? "FAIL" : "PASS");
    return malos ? 1 : 0;
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 4) {
        fprintf(stderr, "uso: test_cola Interbloqueo.mod MutexGc.mod FinJunto.mod "
                        "[--sabotaje-contador|--sabotaje-fin|--sabotaje-salida]\n");
        return 2;
    }
    int sab_contador = 0, sab_fin = 0, sab_salida = 0;
    for (int i = 4; i < argc; i++) {
        if (!strcmp(argv[i], "--sabotaje-contador")) sab_contador = 1;
        else if (!strcmp(argv[i], "--sabotaje-fin")) sab_fin = 1;
        else if (!strcmp(argv[i], "--sabotaje-salida")) sab_salida = 1;
    }
    int sabotaje = sab_contador || sab_fin || sab_salida;
    int fallos = 0;
    if (!sabotaje) fallos += interbloqueo(argv[1]);
    if (!sabotaje || sab_contador) fallos += falso(argv[2], sab_contador);
    if (!sabotaje || sab_fin) fallos += ultimo(argv[3], sab_fin);
    if (!sabotaje || sab_salida) fallos += nadie_dentro(argv[3], sab_salida);
    if (fallos == 0 && !sabotaje)
        printf("OK: la cola (H12 2a.2): el interbloqueo, con uno y dos runs; ni uno falso; el fin, del último que sale; "
               "y nadie dentro al avisar\n");
    return fallos ? 1 : 0;
}
