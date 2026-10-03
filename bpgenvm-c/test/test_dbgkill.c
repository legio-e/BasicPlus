/*
 * test_dbgkill.c — V7/H10 #634 (3-oct): UN KILL CON EL PROGRAMA EN PAUSA.
 *
 * El Stop del IDE, depurando, manda KILL. En placa ese KILL lo recoge `io` —el único lector del cable— y pone la
 * bandera de la VM; a la cola de control del depurador sólo van sus comandos. El lazo de pausa (src/bpvm_dbg_wire.c)
 * pide el siguiente comando con una espera de 200 ms y, si vence, vuelve a esperar: no miraba la bandera, y un
 * programa parado en un breakpoint no se paraba nunca.
 *
 * Aquí, sin placa: KillLoop.mod (un bucle sin fin) parado en su primer opcode por un PAUSE, y un `next_cmd` de
 * mentira que hace de cable:
 *   1. la placa: a la 3.ª espera vacía, «llega» un KILL como lo pone `io` (la bandera) → KILLED, y sin más esperas;
 *   2. el STOP del depurador → «detenido por el depurador», como siempre;
 *   3. el micro simulado: el KILL lo lee el propio depurador del socket → KILLED (antes era un STOP más).
 * Sin el arreglo, la 1 sigue esperando: el `next_cmd` de mentira la corta a la 20.ª con un STOP, y falla.
 *
 *   make test-dbgkill
 */
#include "bpvm.h"
#include "bpvm_dbg_wire.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(c, m) do { if (c) printf("  ok  : %s\n", (m)); else { printf("  FAIL: %s\n", (m)); g_fail++; } } while (0)

static bpvm_t* g_vm = NULL;
static int     g_caso = 0;
static int     g_llamadas = 0;
static int     g_bp_hit = 0;
static char    g_reply[512];

static void manda(const char* line, size_t len, void* user) {
    (void) user;
    if (len >= 11 && strstr(line, "\"BP_HIT\"") != NULL) g_bp_hit++;
}

/* El cable de mentira. Devuelve != 0 cuando «vence la espera» sin comando. */
static int siguiente(bpvm_dbg_cmd_t* out, void* user) {
    (void) user;
    g_llamadas++;
    if (g_llamadas >= 20) { out->kind = BPVM_DBGC_STOP; return 0; }   /* tope: que la prueba no se cuelgue */
    switch (g_caso) {
        case 1:   /* la placa: `io` pone la bandera mientras el depurador espera */
            if (g_llamadas == 3) bpvm_request_kill(g_vm);
            return -1;
        case 2:   /* el STOP del depurador */
            out->kind = bpvm_dbg_wire_kind("STOP");
            return 0;
        case 3:   /* el sim: el KILL llega por el mismo socket que lee el depurador */
            out->kind = bpvm_dbg_wire_kind("KILL");
            return 0;
    }
    return -1;
}

static bpvm_dbg_wire_t g_w = { siguiente, manda, NULL, g_reply, sizeof g_reply, 7 };

static bpvm_status_t corre(int caso, const char* mod) {
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { printf("  FAIL: sin memoria\n"); g_fail++; return BPVM_ERR_RUNTIME; }
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm || bpvm_load_mod(vm, mod) != BPVM_OK) {
        printf("  FAIL: no carga %s\n", mod); g_fail++;
        if (vm) bpvm_destroy(vm);
        free(mem);
        return BPVM_ERR_RUNTIME;
    }
    g_vm = vm; g_caso = caso; g_llamadas = 0; g_bp_hit = 0;
    bpvm_dbg_cmd_t pausa;
    memset(&pausa, 0, sizeof pausa);
    pausa.kind = BPVM_DBGC_PAUSE;
    bpvm_dbg_wire_handle(&g_w, &pausa);     /* PAUSE antes de correr: se para en el primer opcode */
    bpvm_dbg_wire_arm(&g_w, vm);
    bpvm_status_t s = bpvm_run(vm);
    bpvm_dbg_wire_reset();
    bpvm_destroy(vm);
    free(mem);
    return s;
}

int main(int argc, char** argv) {
    const char* mod = (argc > 1) ? argv[1] : "KillLoop.mod";
    char m[160];
    bpvm_fs_register_host();   /* el cargador busca Core.mod por la fachada */
    printf("--- #634: un KILL con el programa en pausa ---\n");

    bpvm_status_t s = corre(1, mod);
    snprintf(m, sizeof m, "1, la placa: KILL (la bandera) durante la pausa -> %s, tras %d esperas (esperado: KILLED, 3)",
             bpvm_status_str(s), g_llamadas);
    CHECK(g_bp_hit == 1 && s == BPVM_KILLED && g_llamadas == 3, m);

    s = corre(2, mod);
    snprintf(m, sizeof m, "2, el STOP del depurador -> %s (esperado: detenido por el depurador)", bpvm_status_str(s));
    CHECK(g_bp_hit == 1 && s == BPVM_DBG_STOPPED && g_llamadas == 1, m);

    s = corre(3, mod);
    snprintf(m, sizeof m, "3, el sim: el KILL lo lee el depurador -> %s (esperado: KILLED)", bpvm_status_str(s));
    CHECK(g_bp_hit == 1 && s == BPVM_KILLED && g_llamadas == 1, m);

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
