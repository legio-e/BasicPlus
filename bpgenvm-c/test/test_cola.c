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
 *  5. (2b) EL `gc()` DEL PROGRAMA CON DOS RUNS (GcConcurrente: un hilo colecta sin parar mientras otro construye y
 *     comprueba cadenas): «errores: 0» en todas las pasadas. Sabotaje (--sabotaje-gc: el `gc()` por el camino de antes,
 *     sólo con el cerrojo): alguna pasada mal.
 *
 *  6. (2b) EVENTOS DE DOS HILOS A LA VEZ, uno en cada run (EvDosRuns), con la ventana ensanchada (encolar cede el turno
 *     a mitad): cada oyente cuenta los suyos, todos. Sabotaje (--sabotaje-eventos: encolar sin el cerrojo): alguno
 *     perdido o pisado.
 *  7. (2b) UN HILO QUE MUERE DEBIENDO UN EVENTO, con dos runs (EvHiloMuere), con la ventana ensanchada: las 40 líneas
 *     del main, el handler y RUNTIME_ERROR al final (#608). Sabotaje (--sabotaje-revive: resucitar a un hilo que otro
 *     run aún tiene entre manos): el programa, cortado.
 *  8. (2b) FICHEROS CON DOS RUNS (FicherosDosRuns), la ventana ensanchada: para escribir, uno a la vez; para leer, los
 *     dos (la regla de Eduardo). Sabotaje (--sabotaje-ficheros: los File sin su cerrojo): dos a la vez escribiendo.
 *
 *  9 a 15 — LA REVISIÓN ADVERSARIAL DE LA 2b (5-oct; docs/V7_IDEAS.md §H12): lo que comparte el código BP normal con
 *  dos runs. Los .mod, por su nombre, en el directorio de trabajo:
 *  9. un `owner` soltado dos veces (OwnerCarrera) —sabotaje, --sabotaje-owner: el proceso colgado o roto—;
 * 10. un `print` que no cabe en la cola de salida (LineasLargas) —--sabotaje-lineas: líneas partidas—;
 * 11. el `join` y la última línea del hilo (JoinLinea, la ventana ensanchada) —--sabotaje-suyo: «finN» antes que «aN»—;
 * 12. la zona de rascar del otro run (ListDirDos) —--sabotaje-zona: «ocupada» en un programa correcto—;
 * 13. un `gc()` tras un mundo parado ajeno (ColectaAjena) —--sabotaje-colecta: alguno sin colecta—;
 * 14. el error del programa, el primero, con uno y dos runs (ErrorPrimero) —--sabotaje-error: el de la atrapada—;
 * 15. un File compartido, uno escribe y otro cierra (FileCompartido, las ventanas ensanchadas, también la de la fachada
 *     del FS) —--sabotaje-ficheros: el proceso caído (un motor ya cerrado)—.
 * 16. (#662, la medida del 2º núcleo) un programa de UN hilo con dos runs (UnHiloCalcula): el otro run no se despierta en
 *     vano tras cada cuanto —--sabotaje-aviso: avisar tras cada uno, como antes: cientos de vueltas sin hilo—.
 * 17. (la 3.2) un hilo que concatena con dos runs (CadenasUnHilo): el GC no despierta al otro run —--sabotaje-mundo—.
 * 18. (LVGL con dos runs) dos hilos que tocan la GUI a la vez (GuiDosRuns): cada builtin de la GUI, en el run 0 —el hilo
 *     del run 1 viaja—; --sabotaje-gui: ejecutado donde se encuentra, como antes: alguno fuera del run 0.
 * 19. una llamada anidada de la GUI, larga, mientras otro run colecta (GuiAnidadaGc): el puente se aparca con el mundo
 *     parado; --sabotaje-anidada: vuelve a entrar sin fin, como antes: el programa colgado (lo para el plazo).
 *
 * Uso: test_cola Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod
 *                FicherosDosRuns.mod
 *                [--sabotaje-contador | --sabotaje-fin | --sabotaje-salida | --sabotaje-gc | --sabotaje-eventos |
 *                 --sabotaje-revive | --sabotaje-ficheros | --sabotaje-owner | --sabotaje-lineas | --sabotaje-suyo |
 *                 --sabotaje-zona | --sabotaje-colecta | --sabotaje-error | --sabotaje-aviso | --sabotaje-mundo |
 *                 --sabotaje-gui]
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

/* El error del programa (bpvm_runtime_error, lo que dice el EXITED) de la última pasada. */
static char s_error[256];

/* Una pasada: `mod` en un bloque de `mem_size` B con el heap hasta `heap` (0: la mitad), con `runs` runs; la salida, a
 * `cb` (con `user`). */
static bpvm_status_t corre_con(const char* mod, size_t mem_size, size_t heap, int runs,
                               void (*cb)(const char*, size_t, void*), void* user) {
    s_error[0] = 0;
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
    bpvm_set_output(vm, cb, user);
    s = (runs > 1) ? bpvm_run_smp(vm, runs) : bpvm_run(vm);
    snprintf(s_error, sizeof s_error, "%s", bpvm_runtime_error(vm));
    bpvm_destroy(vm);
    free(mem);
    return s;
}

/* Lo de siempre: la salida, a `out`. */
static bpvm_status_t corre(const char* mod, size_t mem_size, size_t heap, int runs, salida_t* out) {
    memset(out, 0, sizeof *out);
    return corre_con(mod, mem_size, heap, runs, guarda, out);
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

/* 5. (2b) El `gc()` del programa con dos runs: GcConcurrente (un hilo colecta sin parar, otro construye y comprueba). */
/* Lo que se mira es LA REGLA —ninguna colecta con otro run dentro de su cuanto (heap.c la cuenta)—, además del
 * resultado: el daño de saltársela (un objeto liberado vivo) cae en ventanas de pocas instrucciones y casi nunca se ve
 * (medido el 5-oct: con el camino de antes, 20 de 20 pasadas con el resultado bien). */
static int gc_concurrente(const char* mod, int sabotaje) {
    int malos = 0;
    unsigned long con_otro = 0;
    bpvm_status_t visto = BPVM_OK;
    bpvm_la_cola.sabotaje_gc = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre(mod, 512 * 1024, 0, 2, &o);
        unsigned long n = bpvm_la_cola.gc_con_otro_dentro;   /* de esta pasada (lo pone a 0 la siguiente al colgar) */
        con_otro += n;
        if (s != BPVM_OK || strstr(o.buf, "errores: 0") == NULL || n > 0) {
            malos++;
            if (s != BPVM_OK) visto = s;
        }
    }
    bpvm_la_cola.sabotaje_gc = 0;
    printf("[cola] 5. el gc() del programa con dos runs%s: %d de %d bien; colectas con otro run dentro de su cuanto, "
           "%lu%s%s %s\n", sabotaje ? ", SABOTEADO" : "", PASADAS - malos, PASADAS, con_otro,
           visto != BPVM_OK ? "; alguna dio " : "", visto != BPVM_OK ? bpvm_status_str(visto) : "",
           malos ? "FAIL" : "PASS");
    return malos ? 1 : 0;
}

/* 6. (2b) Eventos de dos hilos a la vez, cada uno en su run (EvDosRuns), con la ventana ensanchada: encolar cede el
 * turno a mitad. Cada oyente tiene que contar los suyos, todos. */
static int eventos_dos(const char* mod, int sabotaje) {
    int malos = 0;
    char ultimo[64] = "";
    bpvm_la_cola.ensayo_eventos = 1;
    bpvm_la_cola.sabotaje_eventos = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre(mod, 512 * 1024, 0, 2, &o);
        if (s != BPVM_OK || strstr(o.buf, "a: 500 b: 500") == NULL) {
            malos++;
            snprintf(ultimo, sizeof ultimo, "%s: %.40s", bpvm_status_str(s), o.buf);
            for (char* p = ultimo; *p; p++) if (*p == '\n') *p = ' ';   /* el resumen, en una línea */
        }
    }
    bpvm_la_cola.ensayo_eventos = 0;
    bpvm_la_cola.sabotaje_eventos = 0;
    printf("[cola] 6. eventos de dos hilos a la vez, uno en cada run%s: %d de %d bien%s%s %s\n",
           sabotaje ? ", SABOTEADO" : "", PASADAS - malos, PASADAS, malos ? "; la última mal, " : "", malos ? ultimo : "",
           malos ? "FAIL" : "PASS");
    return malos ? 1 : 0;
}

/* 7. (2b) Un hilo que muere sin atrapar debiendo un evento, con dos runs (EvHiloMuere), con la ventana ensanchada (cada
 * run se entretiene tras su cuanto, con el hilo entre manos): las 40 líneas del main, el handler, y RUNTIME_ERROR al
 * final (#608), no un programa cortado. */
static int hilo_muere(const char* mod, int sabotaje) {
    int malos = 0;
    char ultimo[80] = "";
    bpvm_la_cola.ensayo_tras_cuanto_ms = 2;
    bpvm_la_cola.sabotaje_revive = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre(mod, 512 * 1024, 0, 2, &o);
        if (s != BPVM_ERR_RUNTIME || !strstr(o.buf, "linea 39") || !strstr(o.buf, "handler")) {
            malos++;
            const char* fin = o.n > 40 ? o.buf + o.n - 40 : o.buf;
            snprintf(ultimo, sizeof ultimo, "%s, acaba en «%.40s»", bpvm_status_str(s), fin);
            for (char* p = ultimo; *p; p++) if (*p == '\n') *p = ' ';   /* el resumen, en una línea */
        }
    }
    bpvm_la_cola.ensayo_tras_cuanto_ms = 0;
    bpvm_la_cola.sabotaje_revive = 0;
    printf("[cola] 7. un hilo que muere debiendo un evento, con dos runs%s: %d de %d bien%s%s %s\n",
           sabotaje ? ", SABOTEADO" : "", PASADAS - malos, PASADAS, malos ? "; la última mal: " : "", malos ? ultimo : "",
           malos ? "FAIL" : "PASS");
    return malos ? 1 : 0;
}

/* 8. (2b) Ficheros con dos runs (FicherosDosRuns), con la ventana ensanchada (abrir cede el turno entre «¿ya está
 * abierto?» y abrirlo): la regla de siempre sigue valiendo —para escribir, uno a la vez; para leer, los dos—. */
static int ficheros_dos(const char* mod, int sabotaje) {
    int malos = 0;
    char ultimo[64] = "";
    bpvm_la_cola.ensayo_ficheros = 1;
    bpvm_la_cola.sabotaje_ficheros = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre(mod, 512 * 1024, 0, 2, &o);
        if (s != BPVM_OK || strstr(o.buf, "escrituras: 600 a la vez: 0") == NULL
                         || strstr(o.buf, "lecturas: 600 fallidas: 0") == NULL) {
            malos++;
            snprintf(ultimo, sizeof ultimo, "%s: %.40s", bpvm_status_str(s), o.buf);
            for (char* p = ultimo; *p; p++) if (*p == '\n') *p = ' ';   /* el resumen, en una línea */
        }
    }
    bpvm_la_cola.ensayo_ficheros = 0;
    bpvm_la_cola.sabotaje_ficheros = 0;
    remove("comun_2b.txt");   /* el fichero de la prueba (en samples/, el cwd) */
    printf("[cola] 8. ficheros con dos runs: para escribir, uno a la vez; para leer, los dos%s: %d de %d bien%s%s %s\n",
           sabotaje ? ", SABOTEADO" : "", PASADAS - malos, PASADAS, malos ? "; la última mal, " : "", malos ? ultimo : "",
           malos ? "FAIL" : "PASS");
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

/* ─── 9 a 15: LA REVISIÓN DE LA 2b (5-oct) — lo que comparte el código BP normal con dos runs. Los .mod, con su nombre
 * en el directorio de trabajo (samples/). ─────────────────────────────────────────────────────────────────────────── */

/* Un resumen de la salida en una línea, para el informe. */
static void resume(char* dst, size_t cap, bpvm_status_t s, const char* buf) {
    snprintf(dst, cap, "%s: %.48s", bpvm_status_str(s), buf);
    for (char* p = dst; *p; p++) if (*p == '\n') *p = ' ';
}

static int informe(const char* que, int sabotaje, int malos, int pasadas, const char* detalle, const char* ultimo) {
    printf("[cola] %s%s: %d de %d bien%s%s%s %s\n", que, sabotaje ? ", SABOTEADO" : "", pasadas - malos, pasadas,
           detalle ? detalle : "", malos ? "; la última mal, " : "", malos ? ultimo : "", malos ? "FAIL" : "PASS");
    return malos ? 1 : 0;
}

/* 9. Dos hilos asignan a la vez el MISMO campo owner, uno en cada run (OwnerCarrera): «fin» y OK siempre (el GC del final
 * recorre el heap; un bloque soltado dos veces lo descarrila, o deja la reserva dando vueltas). Sabotaje (--sabotaje-
 * owner: soltarlo como antes): alguna mal, o el proceso colgado o muerto (la red mira la salida). */
static int owner_carrera(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    bpvm_la_cola.sabotaje_owner = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre("OwnerCarrera.mod", 512 * 1024, 0, 2, &o);
        if (s != BPVM_OK || strstr(o.buf, "fin") == NULL) { malos++; resume(ultimo, sizeof ultimo, s, o.buf); }
    }
    bpvm_la_cola.sabotaje_owner = 0;
    return informe("9. dos hilos asignan a la vez el mismo campo owner (dos runs)", sabotaje, malos, PASADAS, NULL, ultimo);
}

/* 10. Dos hilos imprimen líneas que NO caben en la cola de salida (LineasLargas: 8192 caracteres; la cola, 4 KB): cada
 * línea entera, 8192 veces la misma letra, y las 40. Sabotaje (--sabotaje-lineas: sin esperar al que escribe a trozos):
 * alguna línea partida. */
typedef struct { int letra; long largo; int mezclada; long lineas; long malas; } vigia_t;
static void vigila(const char* s, size_t len, void* user) {
    vigia_t* v = (vigia_t*) user;
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\n') {
            if (v->largo != 8192 || v->mezclada) v->malas++;
            v->lineas++; v->largo = 0; v->mezclada = 0; v->letra = 0;
            continue;
        }
        if (v->largo == 0) v->letra = (unsigned char) c;
        else if ((unsigned char) c != v->letra) v->mezclada = 1;
        v->largo++;
    }
}
static int lineas_largas(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    bpvm_oq_sabotaje_lineas = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        vigia_t v; memset(&v, 0, sizeof v);
        bpvm_status_t s = corre_con("LineasLargas.mod", 512 * 1024, 0, 2, vigila, &v);
        if (s != BPVM_OK || v.lineas != 40 || v.malas != 0) {
            malos++;
            snprintf(ultimo, sizeof ultimo, "%s: %ld líneas, %ld partidas", bpvm_status_str(s), v.lineas, v.malas);
        }
    }
    bpvm_oq_sabotaje_lineas = 0;
    return informe("10. dos hilos imprimen líneas que no caben en la cola de salida (dos runs)", sabotaje, malos, PASADAS,
                   NULL, ultimo);
}

/* 11. El join no se adelanta a la última línea del hilo (JoinLinea), con la ventana ensanchada (el hilo que termina
 * tarda en soltar su línea a medias): con dos runs, la misma salida que con uno. Sabotaje (--sabotaje-suyo: el join y
 * el despertar, sin mirar si otro run tiene el hilo): alguna con «finN» antes que «aN». */
static int join_linea(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    salida_t uno;
    bpvm_status_t s1 = corre("JoinLinea.mod", 512 * 1024, 0, 1, &uno);   /* el oráculo: con un run */
    if (s1 != BPVM_OK || strstr(uno.buf, "a49 fin49") == NULL) {
        printf("[cola] 11. el oráculo con un run no sale como se esperaba (%s) FAIL\n", bpvm_status_str(s1));
        return 1;
    }
    bpvm_la_cola.ensayo_linea_ms = 3;
    bpvm_la_cola.sabotaje_suyo = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre("JoinLinea.mod", 512 * 1024, 0, 2, &o);
        if (s != BPVM_OK || strcmp(o.buf, uno.buf) != 0) { malos++; resume(ultimo, sizeof ultimo, s, o.buf); }
    }
    bpvm_la_cola.ensayo_linea_ms = 0;
    bpvm_la_cola.sabotaje_suyo = 0;
    return informe("11. el join espera a la última línea del hilo (dos runs, la ventana ensanchada)", sabotaje, malos,
                   PASADAS, NULL, ultimo);
}

/* 12. Dos hilos listan una carpeta a la vez (ListDirDos): ni un «zona de rascar ocupada». Sabotaje (--sabotaje-zona:
 * ocupada por el otro run es un error, como antes): alguno. */
static int listdir_dos(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    bpvm_la_cola.sabotaje_zona = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre("ListDirDos.mod", 512 * 1024, 0, 2, &o);
        if (s != BPVM_OK || strstr(o.buf, "listados: 400 fallos: 0") == NULL) { malos++; resume(ultimo, sizeof ultimo, s, o.buf); }
    }
    bpvm_la_cola.sabotaje_zona = 0;
    return informe("12. dos hilos listan una carpeta a la vez (dos runs)", sabotaje, malos, PASADAS, NULL, ultimo);
}

/* 13. Un gc() que encuentra el mundo parado por otra cosa (ColectaAjena: Heap.compact() con otro hilo vivo para el
 * mundo y no colecta): colecta igual, siempre. Sabotaje (--sabotaje-colecta: vuelve sin mirar si colectó): alguno sin. */
static int colecta_ajena(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    bpvm_la_cola.sabotaje_colecta = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre("ColectaAjena.mod", 512 * 1024, 0, 2, &o);
        if (s != BPVM_OK || strstr(o.buf, "gc sin colecta: 0") == NULL) { malos++; resume(ultimo, sizeof ultimo, s, o.buf); }
    }
    bpvm_la_cola.sabotaje_colecta = 0;
    return informe("13. un gc() con el mundo parado por otra cosa colecta igual (dos runs)", sabotaje, malos, PASADAS,
                   NULL, ultimo);
}

/* 14. El error del programa es el PRIMERO (ErrorPrimero: un hilo muere sin atrapar y después el main atrapa otro): lo
 * que dice el EXITED es el del hilo, con uno y con dos runs. Sabotaje (--sabotaje-error: lo escribe cada throw, como
 * antes): el de la atrapada. */
static int error_primero(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    bpvm_la_cola.sabotaje_error = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        int runs = 1 + (i % 2);
        salida_t o;
        bpvm_status_t s = corre("ErrorPrimero.mod", 512 * 1024, 0, runs, &o);
        if (s != BPVM_ERR_RUNTIME || strstr(o.buf, "atrapada") == NULL || strstr(o.buf, "fin") == NULL
                || strstr(s_error, "ALOAD") == NULL) {
            malos++;
            snprintf(ultimo, sizeof ultimo, "%s, con %d run%s: «%.40s»", bpvm_status_str(s), runs, runs > 1 ? "s" : "",
                     s_error);
        }
    }
    bpvm_la_cola.sabotaje_error = 0;
    return informe("14. el error del programa, el primero (uno y dos runs)", sabotaje, malos, PASADAS, NULL, ultimo);
}

/* 15. Un File compartido: un hilo escribe y el otro lo cierra (dos veces), con la ventana ensanchada (FileCompartido):
 * «rondas: 100» y OK. Sabotaje (--sabotaje-ficheros, el de la parte 8: los File sin su cerrojo): alguna mal, o el
 * proceso muerto (un FILE* cerrado dos veces). */
static int file_compartido(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    bpvm_la_cola.ensayo_ficheros = 1;
    bpvm_fs_ensayo_cursor = bpvm_platform_thread_yield;   /* y la de la fachada: entre «¿abierto?» y el motor */
    bpvm_la_cola.sabotaje_ficheros = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre("FileCompartido.mod", 512 * 1024, 0, 2, &o);
        if (s != BPVM_OK || strstr(o.buf, "rondas: 100") == NULL) { malos++; resume(ultimo, sizeof ultimo, s, o.buf); }
    }
    bpvm_la_cola.ensayo_ficheros = 0;
    bpvm_fs_ensayo_cursor = NULL;
    bpvm_la_cola.sabotaje_ficheros = 0;
    remove("compartido_2b.txt");
    return informe("15. un File compartido: uno escribe y otro lo cierra (dos runs, la ventana ensanchada)", sabotaje,
                   malos, PASADAS, NULL, ultimo);
}

/* 16. (#662) Un programa de UN hilo con dos runs (UnHiloCalcula): el run ocioso no se despierta en vano tras cada cuanto
 * —sus vueltas sin hilo, unas pocas (el arranque y el fin)—, y el resultado, bien. Sabotaje (--sabotaje-aviso: avisar a
 * la cola tras CADA cuanto, como antes): cientos. */
static int un_hilo(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    unsigned long mas = 0;
    bpvm_la_cola.sabotaje_aviso = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre("UnHiloCalcula.mod", 512 * 1024, 0, 2, &o);
        unsigned long v = bpvm_la_cola.vueltas_sin_hilo;   /* de esta pasada (lo pone a 0 la siguiente al colgar) */
        if (v > mas) mas = v;
        if (s != BPVM_OK || strstr(o.buf, "fib(22) = 17711") == NULL || v > 20) {
            malos++;
            snprintf(ultimo, sizeof ultimo, "%s, %lu vueltas sin hilo", bpvm_status_str(s), v);
        }
    }
    bpvm_la_cola.sabotaje_aviso = 0;
    char detalle[64];
    snprintf(detalle, sizeof detalle, "; vueltas sin hilo, %lu como mucho", mas);
    return informe("16. un hilo con dos runs: el otro no se despierta en vano (#662)", sabotaje, malos, PASADAS, detalle,
                   ultimo);
}

/* 17. (V7/H12, la 3.2) Un programa de UN hilo que concatena con dos runs (CadenasUnHilo): cada «+» reserva y el GC
 * colecta cientos de veces; el mundo parado tiene su propia condición, así que las colectas no despiertan al run
 * ocioso —sus vueltas sin hilo, unas pocas— y el resultado, bien. Sabotaje (--sabotaje-mundo: el mundo avisa también
 * a la cola, como antes): tantas vueltas sin hilo como colectas. */
static int cadenas_un_hilo(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    unsigned long mas = 0;
    bpvm_la_cola.sabotaje_mundo = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre("CadenasUnHilo.mod", 512 * 1024, 0, 2, &o);
        unsigned long v = bpvm_la_cola.vueltas_sin_hilo;   /* de esta pasada (lo pone a 0 la siguiente al colgar) */
        if (v > mas) mas = v;
        if (s != BPVM_OK || strstr(o.buf, "resultado = 1920") == NULL || v > 20) {
            malos++;
            snprintf(ultimo, sizeof ultimo, "%s, %lu vueltas sin hilo", bpvm_status_str(s), v);
        }
    }
    bpvm_la_cola.sabotaje_mundo = 0;
    char detalle[64];
    snprintf(detalle, sizeof detalle, "; vueltas sin hilo, %lu como mucho", mas);
    return informe("17. un hilo que reserva, con dos runs: el GC no despierta al otro (la 3.2)", sabotaje, malos,
                   PASADAS, detalle, ultimo);
}

/* 18. (V7/H12, LVGL con dos runs) DOS hilos que tocan la GUI a la vez, con dos runs (GuiDosRuns: cada uno crea sus
 * etiquetas, les cambia el texto, la x y el ancho y los lee, con cálculo entre medias): cada builtin de la GUI lo
 * ejecuta el run 0 —el hilo que llega a uno en el run 1 VIAJA—. Ninguno fuera del run 0, alguno de viaje en cada pasada
 * (el camino, recorrido) y el resultado, el de un run y la VM-Java. Sabotaje (--sabotaje-gui: el builtin se ejecuta en
 * el run que lo encuentra, como antes): alguno fuera del run 0. */
static int gui_dos_runs(int sabotaje) {
    int malos = 0; char ultimo[160] = "";
    unsigned long fuera = 0, viajes = 0;
    bpvm_la_cola.sabotaje_gui = sabotaje;
    for (int i = 0; i < PASADAS; i++) {
        salida_t o;
        bpvm_status_t s = corre("GuiDosRuns.mod", 512 * 1024, 0, 2, &o);
        unsigned long f = bpvm_la_cola.gui_fuera_del_run0, v = bpvm_la_cola.gui_viajes;   /* de esta pasada */
        fuera += f;
        viajes += v;
        if (s != BPVM_OK || strstr(o.buf, "pintor 1000: 29274680") == NULL
            || strstr(o.buf, "pintor 2000: 29314680") == NULL || f != 0) {
            malos++;
            snprintf(ultimo, sizeof ultimo, "%s, %lu fuera del run 0, %lu viajes: %.40s", bpvm_status_str(s), f, v,
                     s_error);
        }
    }
    /* El camino del viaje, recorrido: en el conjunto de las pasadas, no en cada una (en una pasada el run 0 puede
     * quedarse con los dos pintores y no viajar nadie, sin que nada esté mal). */
    if (viajes == 0 && !sabotaje) {
        malos++;
        snprintf(ultimo, sizeof ultimo, "ningún viaje al run 0 en %d pasadas: el camino, sin recorrer", PASADAS);
    }
    bpvm_la_cola.sabotaje_gui = 0;
    char detalle[96];
    snprintf(detalle, sizeof detalle, "; fuera del run 0, %lu; viajes al run 0, %lu", fuera, viajes);
    return informe("18. dos hilos que tocan la GUI, con dos runs: todo en el run 0 (LVGL con dos runs)", sabotaje,
                   malos, PASADAS, detalle, ultimo);
}

/* 19. (V7/H12) UNA LLAMADA ANIDADA larga —`Gui.__guiInvokeBySlot` corre el método dentro del builtin, en el run 0, por
 * el puente del despacho de cada clic— mientras otro hilo reserva sin parar (GuiAnidadaGc): el puente se aparca con
 * el mundo parado y el resultado sale. Sabotaje (--sabotaje-anidada: como antes, vuelve a entrar sin fin): el programa
 * se cuelga —el run 0 gira y el colector del otro lo espera para siempre— y lo para el plazo de la red. */
static int gui_anidada(int sabotaje) {
    int malos = 0; char ultimo[80] = "";
    const int pasadas = sabotaje ? 1 : 10;
    bpvm_la_cola.sabotaje_anidada = sabotaje;
    for (int i = 0; i < pasadas; i++) {
        salida_t o;
        bpvm_status_t s = corre("GuiAnidadaGc.mod", 512 * 1024, 0, 2, &o);
        if (s != BPVM_OK || strstr(o.buf, "resultado = 472668") == NULL) {
            malos++;
            resume(ultimo, sizeof ultimo, s, o.buf);
        }
    }
    bpvm_la_cola.sabotaje_anidada = 0;
    return informe("19. una llamada anidada de la GUI, larga, mientras otro run colecta: el puente se aparca",
                   sabotaje, malos, pasadas, NULL, ultimo);
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 8) {
        fprintf(stderr, "uso: test_cola Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod "
                        "EvHiloMuere.mod FicherosDosRuns.mod [--sabotaje-contador|--sabotaje-fin|--sabotaje-salida|"
                        "--sabotaje-gc|--sabotaje-eventos|--sabotaje-revive|--sabotaje-ficheros]\n");
        return 2;
    }
    int sab_contador = 0, sab_fin = 0, sab_salida = 0, sab_gc = 0, sab_eventos = 0, sab_revive = 0, sab_ficheros = 0;
    int sab_owner = 0, sab_lineas = 0, sab_suyo = 0, sab_zona = 0, sab_colecta = 0, sab_error = 0, sab_aviso = 0;
    int sab_mundo = 0, sab_gui = 0, sab_anidada = 0;
    for (int i = 8; i < argc; i++) {
        if (!strcmp(argv[i], "--sabotaje-contador")) sab_contador = 1;
        else if (!strcmp(argv[i], "--sabotaje-fin")) sab_fin = 1;
        else if (!strcmp(argv[i], "--sabotaje-salida")) sab_salida = 1;
        else if (!strcmp(argv[i], "--sabotaje-gc")) sab_gc = 1;
        else if (!strcmp(argv[i], "--sabotaje-eventos")) sab_eventos = 1;
        else if (!strcmp(argv[i], "--sabotaje-revive")) sab_revive = 1;
        else if (!strcmp(argv[i], "--sabotaje-ficheros")) sab_ficheros = 1;
        else if (!strcmp(argv[i], "--sabotaje-owner")) sab_owner = 1;
        else if (!strcmp(argv[i], "--sabotaje-lineas")) sab_lineas = 1;
        else if (!strcmp(argv[i], "--sabotaje-suyo")) sab_suyo = 1;
        else if (!strcmp(argv[i], "--sabotaje-zona")) sab_zona = 1;
        else if (!strcmp(argv[i], "--sabotaje-colecta")) sab_colecta = 1;
        else if (!strcmp(argv[i], "--sabotaje-error")) sab_error = 1;
        else if (!strcmp(argv[i], "--sabotaje-aviso")) sab_aviso = 1;
        else if (!strcmp(argv[i], "--sabotaje-mundo")) sab_mundo = 1;
        else if (!strcmp(argv[i], "--sabotaje-gui")) sab_gui = 1;
        else if (!strcmp(argv[i], "--sabotaje-anidada")) sab_anidada = 1;
    }
    int sabotaje = sab_contador || sab_fin || sab_salida || sab_gc || sab_eventos || sab_revive || sab_ficheros
                || sab_owner || sab_lineas || sab_suyo || sab_zona || sab_colecta || sab_error || sab_aviso
                || sab_mundo || sab_gui || sab_anidada;
    int fallos = 0;
    if (!sabotaje) fallos += interbloqueo(argv[1]);
    if (!sabotaje || sab_contador) fallos += falso(argv[2], sab_contador);
    if (!sabotaje || sab_fin) fallos += ultimo(argv[3], sab_fin);
    if (!sabotaje || sab_salida) fallos += nadie_dentro(argv[3], sab_salida);
    if (!sabotaje || sab_gc) fallos += gc_concurrente(argv[4], sab_gc);
    if (!sabotaje || sab_eventos) fallos += eventos_dos(argv[5], sab_eventos);
    if (!sabotaje || sab_revive) fallos += hilo_muere(argv[6], sab_revive);
    if (!sabotaje || sab_ficheros) fallos += ficheros_dos(argv[7], sab_ficheros);
    /* la revisión de la 2b (5-oct) */
    if (!sabotaje || sab_owner) fallos += owner_carrera(sab_owner);
    if (!sabotaje || sab_lineas) fallos += lineas_largas(sab_lineas);
    if (!sabotaje || sab_suyo) fallos += join_linea(sab_suyo);
    if (!sabotaje || sab_zona) fallos += listdir_dos(sab_zona);
    if (!sabotaje || sab_colecta) fallos += colecta_ajena(sab_colecta);
    if (!sabotaje || sab_error) fallos += error_primero(sab_error);
    if (!sabotaje || sab_ficheros) fallos += file_compartido(sab_ficheros);
    if (!sabotaje || sab_aviso) fallos += un_hilo(sab_aviso);   /* #662 */
    if (!sabotaje || sab_mundo) fallos += cadenas_un_hilo(sab_mundo);   /* la 3.2: el mundo, con su condición */
    if (!sabotaje || sab_gui) fallos += gui_dos_runs(sab_gui);   /* LVGL con dos runs: la GUI, en el run 0 */
    if (!sabotaje || sab_anidada) fallos += gui_anidada(sab_anidada);   /* y el puente, aparcado con el mundo */
    if (fallos == 0 && !sabotaje)
        printf("OK: la cola (H12 2a.2 y 2b): el interbloqueo, con uno y dos runs; ni uno falso; el fin, del último que "
               "sale; nadie dentro al avisar; el gc() del programa, con el mundo parado; los eventos y los ficheros con "
               "dos runs; lo de la revisión: owner, líneas largas, el join, la zona, la colecta, el error y un File "
               "compartido; un hilo con dos runs no despierta al otro en vano, ni tras un cuanto (#662) ni tras una "
               "colecta (la 3.2); y la GUI con dos runs, toda en el run 0, con su puente aparcado si otro run colecta\n");
    return fallos ? 1 : 0;
}
