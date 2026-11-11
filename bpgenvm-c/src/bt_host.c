/*
 * bt_host.c — V8/H2 (8-nov): EL BLUETOOTH DEL PC, simulado (Eduardo: «solo con el teclado conectado»; V8_IDEAS §E1, «En el
 * PC, lo decidido»). La búsqueda encuentra dos aparatos de mentira, fijos para que las dos VMs escriban lo mismo:
 * «Teclado simulado» y «Ratón simulado». El teclado, como el de verdad, no dice qué es en la búsqueda ("unknown"); lo dice
 * al conectarlo. Con él conectado, el teclado de la ventana de la GUI es el suyo. La VM-Java hace lo mismo (Bluetooth.java).
 * Es la radio más pequeña que publica 'BT  ': lo que corre encima, los estados y lo recordado, es el mismo código que en una
 * placa.
 */
#include "bpvm_bt.h"
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"

#include <stdio.h>
#include <string.h>

static const bpvm_bt_aparato_t SIMULADOS[2] = {
    { "Teclado simulado", "02:00:00:00:00:01", "random", "unknown", -40 },
    { "Ratón simulado",   "02:00:00:00:00:02", "random", "mouse",   -55 },
};
static const char* const CLASES[2] = { "keyboard", "mouse" };

static int h_encender(bpvm_bt_aviso_fn aviso, char* err, size_t n) {
    (void) err; (void) n;
    aviso(BPVM_BT_AV_LISTA, NULL, NULL);   /* el PC no tiene que esperar a nada */
    return 0;
}

static void h_apagar(void) { }

static int h_buscar(int ms, bpvm_bt_visto_fn visto, char* err, size_t n) {
    int i;
    (void) ms; (void) err; (void) n;
    for (i = 0; i < 2; i++) visto(&SIMULADOS[i]);   /* están ahí desde el principio; la búsqueda dura lo pedido */
    return 0;
}

/* Los simulados están siempre ahí: conectan en el acto, se espere o no (`ms`). */
static int h_conectar(const char* dir, const char* tipo, int ms, char* clase, size_t nclase, char* err, size_t n) {
    int i;
    (void) tipo; (void) ms;
    for (i = 0; i < 2; i++)
        if (strcmp(dir, SIMULADOS[i].dir) == 0) { snprintf(clase, nclase, "%s", CLASES[i]); return 0; }
    snprintf(err, n, "Bluetooth: no se encuentra el aparato %s", dir);
    return -1;
}

static void h_dir(const char* dir) { (void) dir; }

static const bpvm_pack_sym_t SIMBOLOS[] = {
    { "encender",    (bpvm_pack_fn_t) h_encender },
    { "apagar",      (bpvm_pack_fn_t) h_apagar },
    { "buscar",      (bpvm_pack_fn_t) h_buscar },
    { "conectar",    (bpvm_pack_fn_t) h_conectar },
    { "desconectar", (bpvm_pack_fn_t) h_dir },
    { "olvidar",     (bpvm_pack_fn_t) h_dir },
};
static const bpvm_pack_api_t TABLA = { BPVM_BT_MARCA, BPVM_BT_API_VER, 6, SIMBOLOS };

void bpvm_bt_register_host(void) {
    (void) bpvm_bios_publica(BPVM_BT_MARCA, &TABLA);
}
