/*
 * test_boot.c — H9: unidad de la máquina de estados del arranque (bpvm_boot).
 * Host-only, sin placa: las capas son callbacks de test que fallan a demanda, y
 * verificamos la subida 0→3, la parada en el 1er fallo (quedándose en el último
 * estado bueno con el motivo), el modo seguro (tope), el fallo de runtime (drop)
 * y el reporte STATE. Comprobamos también que las capas por encima del corte / del
 * tope NO se llaman.
 *
 *   make test-boot
 */
#include "bpvm_boot.h"
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* Contexto compartido por los callbacks de capa. */
typedef struct {
    int fail_at;     /* estado (1/2/3) en el que fallar, o 0 = ninguno */
    int called[4];   /* called[target] = 1 si se llamó ese callback */
} ctx_t;

static bpvm_boot_step_t step(ctx_t* c, int target, const char* failmsg) {
    c->called[target] = 1;
    bpvm_boot_step_t r;
    if (c->fail_at == target) {
        r.ok = 0;
        snprintf(r.reason, sizeof r.reason, "%s", failmsg);
    } else {
        r.ok = 1;
        r.reason[0] = '\0';
    }
    return r;
}
static bpvm_boot_step_t cb_part(void* u) { return step((ctx_t*) u, 1, "tabla de particiones no valida"); }
static bpvm_boot_step_t cb_fs(void* u)   { return step((ctx_t*) u, 2, "FS no montable"); }
static bpvm_boot_step_t cb_app(void* u)  { return step((ctx_t*) u, 3, "heap/app no arranca"); }

static bpvm_boot_layers_t mklayers(ctx_t* c, bpvm_boot_state_t max) {
    bpvm_boot_layers_t L;
    L.to_partitions = cb_part; L.to_fs = cb_fs; L.to_app = cb_app;
    L.user = c; L.max_state = max;
    return L;
}

int main(void) {
    printf("=== test_boot (H9 maquina de estados + STATE) ===\n");
    bpvm_boot_status_t st;

    /* --- 1. subida completa 0→3 --- */
    {
        ctx_t c = {0, {0}};
        bpvm_boot_layers_t L = mklayers(&c, BPVM_BOOT_APP);
        bpvm_boot_climb(&L, &st);
        CHECK(st.state == BPVM_BOOT_APP && !st.degraded, "subida completa → APP, no degradado");
        CHECK(c.called[1] && c.called[2] && c.called[3], "las 3 capas se intentaron");
    }

    /* --- 2. falla particiones → se queda en KERNEL --- */
    {
        ctx_t c = {1, {0}};
        bpvm_boot_layers_t L = mklayers(&c, BPVM_BOOT_APP);
        bpvm_boot_climb(&L, &st);
        CHECK(st.state == BPVM_BOOT_KERNEL && st.degraded, "particiones falla → estado KERNEL, degradado");
        CHECK(strstr(st.reason, "particiones") != NULL, "motivo = tabla de particiones");
        CHECK(c.called[1] && !c.called[2] && !c.called[3], "corto: no se intentan FS ni APP");
    }

    /* --- 3. falla FS → se queda en PARTICIONES --- */
    {
        ctx_t c = {2, {0}};
        bpvm_boot_layers_t L = mklayers(&c, BPVM_BOOT_APP);
        bpvm_boot_climb(&L, &st);
        CHECK(st.state == BPVM_BOOT_PARTITIONS && st.degraded, "FS falla → estado PARTICIONES");
        CHECK(strstr(st.reason, "FS") != NULL, "motivo = FS no montable");
        CHECK(c.called[1] && c.called[2] && !c.called[3], "no se intenta APP");
    }

    /* --- 4. falla APP → se queda en FS (comms+FS vivos: 'la app peto pero alcanzable') --- */
    {
        ctx_t c = {3, {0}};
        bpvm_boot_layers_t L = mklayers(&c, BPVM_BOOT_APP);
        bpvm_boot_climb(&L, &st);
        CHECK(st.state == BPVM_BOOT_FS && st.degraded, "APP falla → estado FS (alcanzable)");
        CHECK(strstr(st.reason, "heap") != NULL, "motivo = heap/app");
    }

    /* --- 5. modo seguro: tope KERNEL → no sube aunque las capas irian bien --- */
    {
        ctx_t c = {0, {0}};
        bpvm_boot_layers_t L = mklayers(&c, BPVM_BOOT_KERNEL);
        bpvm_boot_climb(&L, &st);
        CHECK(st.state == BPVM_BOOT_KERNEL && !st.degraded, "modo seguro KERNEL → KERNEL, NO degradado");
        CHECK(!c.called[1], "con tope KERNEL no se intenta ninguna capa");
    }

    /* --- 6. tope FS: sube a FS y para (no degradado) --- */
    {
        ctx_t c = {0, {0}};
        bpvm_boot_layers_t L = mklayers(&c, BPVM_BOOT_FS);
        bpvm_boot_climb(&L, &st);
        CHECK(st.state == BPVM_BOOT_FS && !st.degraded, "tope FS → FS, no degradado");
        CHECK(c.called[1] && c.called[2] && !c.called[3], "APP no se intenta (por encima del tope)");
    }

    /* --- 7. fallo en runtime: sube a APP y luego la app peta → baja a FS --- */
    {
        ctx_t c = {0, {0}};
        bpvm_boot_layers_t L = mklayers(&c, BPVM_BOOT_APP);
        bpvm_boot_climb(&L, &st);
        CHECK(st.state == BPVM_BOOT_APP && !st.degraded, "arranca sana en APP");
        bpvm_boot_fault(&st, BPVM_BOOT_FS, "programa del usuario colgo (KILL)");
        CHECK(st.state == BPVM_BOOT_FS && st.degraded, "runtime fault → baja a FS, degradado");
        CHECK(strstr(st.reason, "colgo") != NULL, "motivo del fault registrado");
        /* un fault a un estado NO inferior no sube el estado */
        bpvm_boot_fault(&st, BPVM_BOOT_APP, "otro");
        CHECK(st.state == BPVM_BOOT_FS, "fault a estado no-inferior no re-sube");
    }

    /* --- 8. reporte STATE para el wire --- */
    {
        ctx_t c = {2, {0}};
        bpvm_boot_layers_t L = mklayers(&c, BPVM_BOOT_APP);
        bpvm_boot_climb(&L, &st);   /* queda en PARTICIONES (FS falla) */
        char buf[128];
        int w = bpvm_boot_state_report(&st, buf, sizeof buf);
        CHECK(w > 0, "state_report produce texto");
        CHECK(strstr(buf, "state=1") && strstr(buf, "name=particiones")
              && strstr(buf, "degraded=1") && strstr(buf, "reason=FS"),
              "STATE = 'state=1 name=particiones degraded=1 reason=FS...'");
        printf("       STATE: %s\n", buf);
    }

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
