/*
 * test_bt.c — [V8/H2, paso 5, la pieza 5] EL ARRANQUE Y EL APAGADO POR FALTA DE USO DEL BLUETOOTH (bpvm_bt.c), en el PC,
 * con la radio simulada (bt_host.c) y /sys/bluetooth.json de mentira: como la WiFi, se enciende al arrancar con `bt=1` o
 * con "autoStart", y `bt=0` manda; al acabar un programa que lo tocó, vuelve a eso; y se apaga solo a los "autoOff"
 * segundos sin uso (0, nunca; en el PC, nunca), y un uso lo retrasa.
 */
#include "bpvm_bt.h"
#include "bpvm_fs.h"
#include "bpvm_platform.h"

#include <stdio.h>
#include <string.h>

static int fallos = 0, buenas = 0;
static void ok(const char* que, int cond) {
    printf("%s %s\n", cond ? "ok   " : "FALLO", que);
    if (cond) buenas++; else fallos++;
}

/* /sys/bluetooth.json, de mentira: NULL, que no está. */
static const char* s_json = NULL;
static int fs_stat(const char* p, uint32_t* tam) {
    if (!s_json || strcmp(p, BPVM_BT_FICHERO) != 0) return -1;
    *tam = (uint32_t) strlen(s_json);
    return 0;
}
static long fs_read(const char* p, uint8_t* dst, uint32_t cap) {
    size_t n;
    if (!s_json || strcmp(p, BPVM_BT_FICHERO) != 0) return -1;
    n = strlen(s_json);
    if (n > cap) n = cap;
    memcpy(dst, s_json, n);
    return (long) n;
}
static const bpvm_fs_backend_t FS = { .stat = fs_stat, .read = fs_read };

/* Espera hasta `ms` a que el estado sea `e`; los ms que tardó, con el reloj (en Windows un sleep de 10 ms dura ~15), o -1. */
static long llega(int e, long ms) {
    int64_t t0 = bpvm_platform_now_ms(), t;
    for (;;) {
        t = bpvm_platform_now_ms() - t0;
        if (bpvm_bt_estado() == e) return (long) t;
        if (t > ms) return -1;
        bpvm_platform_thread_sleep_ms(10);
    }
}

int main(void) {
    long t;
    int i, siguio;

    bpvm_fs_set_backend(&FS);
    bpvm_bt_register_host();
    bpvm_bt_auto_off_fijo(-1);   /* el PC no se apaga solo: aquí, lo del fichero */

    printf("--- el arranque ---\n");
    bpvm_bt_arranque(-1);
    bpvm_platform_thread_sleep_ms(50);
    ok("sin fichero y sin bt=1: apagado", bpvm_bt_estado() == BPVM_BT_OFF);
    s_json = "{\"autoStart\":true,\"autoOff\":0}";
    bpvm_bt_arranque(0);
    bpvm_platform_thread_sleep_ms(50);
    ok("bt=0 manda sobre autoStart", bpvm_bt_estado() == BPVM_BT_OFF);
    bpvm_bt_arranque(-1);
    ok("autoStart, sin Entorno: se enciende", llega(BPVM_BT_ON, 2000) >= 0);
    bpvm_platform_thread_sleep_ms(1500);
    ok("con \"autoOff\": 0, no se apaga", bpvm_bt_estado() == BPVM_BT_ON);

    printf("--- al acabar un programa que lo tocó ---\n");
    bpvm_bt_programa_apaga();
    ok("un programa lo apaga", bpvm_bt_estado() == BPVM_BT_OFF);
    bpvm_bt_fin_de_run();
    ok("al acabar, vuelve encendido (autoStart)", llega(BPVM_BT_ON, 2000) >= 0);
    s_json = "{\"autoOff\":1}";
    bpvm_bt_programa_apaga();
    bpvm_bt_programa_enciende(NULL, 0);
    llega(BPVM_BT_ON, 2000);
    bpvm_bt_fin_de_run();
    ok("sin autoStart ni bt=1, al acabar se apaga", bpvm_bt_estado() == BPVM_BT_OFF);

    printf("--- el apagado por falta de uso ---\n");
    bpvm_bt_arranque(1);
    ok("bt=1, sin autoStart: se enciende", llega(BPVM_BT_ON, 2000) >= 0);
    t = llega(BPVM_BT_OFF, 4000);
    printf("      (apagado a los %ld ms)\n", t);
    ok("con \"autoOff\": 1, sin uso, se apaga solo (al segundo)", t >= 800 && t <= 2500);

    bpvm_bt_arranque(1);
    llega(BPVM_BT_ON, 2000);
    for (i = 0, siguio = 1; i < 10; i++) {   /* 2,5 s, con un uso cada cuarto de segundo: lo que manda un teclado */
        bpvm_platform_thread_sleep_ms(250);
        bpvm_bt_actividad();
        if (bpvm_bt_estado() != BPVM_BT_ON) siguio = 0;
    }
    ok("con uso, sigue encendido (2,5 s con \"autoOff\": 1)", siguio);
    t = llega(BPVM_BT_OFF, 4000);
    ok("y sin él, se apaga", t >= 0);

    bpvm_bt_arranque(1);
    llega(BPVM_BT_ON, 2000);
    for (i = 0, siguio = 1; i < 6; i++) {   /* una búsqueda cuenta como uso */
        char det[96];
        bpvm_platform_thread_sleep_ms(400);
        (void) bpvm_bt_buscar(10, det, sizeof det);
        if (bpvm_bt_estado() == BPVM_BT_OFF) siguio = 0;
    }
    ok("una búsqueda cuenta como uso", siguio);
    llega(BPVM_BT_OFF, 4000);

    s_json = "{\"autoOff\":-5}";
    bpvm_bt_arranque(1);
    llega(BPVM_BT_ON, 2000);
    bpvm_platform_thread_sleep_ms(1500);
    ok("un \"autoOff\" que no vale: el de por defecto (600 s)", bpvm_bt_estado() == BPVM_BT_ON);
    bpvm_bt_programa_apaga();
    bpvm_bt_fin_de_run();   /* bt=1: vuelve encendido */
    ok("al acabar un programa que lo apagó, con bt=1, vuelve encendido", llega(BPVM_BT_ON, 2000) >= 0);

    printf("--- el PC ---\n");
    s_json = "{\"autoOff\":1}";
    bpvm_bt_programa_apaga();
    bpvm_bt_auto_off_fijo(0);
    bpvm_bt_programa_enciende(NULL, 0);
    llega(BPVM_BT_ON, 2000);
    bpvm_platform_thread_sleep_ms(1800);
    ok("en el PC no se apaga solo, aunque el fichero diga 1", bpvm_bt_estado() == BPVM_BT_ON);
    bpvm_bt_programa_apaga();

    printf("[status=%s] %d/%d\n", fallos ? "FALLO" : "OK", buenas, buenas + fallos);
    return fallos ? 1 : 0;
}
