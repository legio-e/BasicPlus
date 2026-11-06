/*
 * wifi_host.c — V8/H3 (6-nov): la WiFi del PC. El PC ya tiene red, así que Air.Wifi.turnOn() da true, sin fichero y sin
 * radio; la VM-Java hace lo mismo (VirtualMachine, WIFI_*). Es la radio más pequeña que publica 'WIFI': lo que corre encima,
 * los estados y la tarea, es el mismo código que en una placa.
 */
#include "bpvm_wifi.h"
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "bpvm_red.h"

#include <stddef.h>

static bpvm_wifi_aviso_fn s_aviso = NULL;

static int  h_encender(const char* pais, bpvm_wifi_aviso_fn aviso, char* err, size_t n) {
    (void) pais; (void) err; (void) n;
    s_aviso = aviso;
    return 0;
}

static void h_apagar(void) { }

static int  h_conectar(const char* ssid, const char* clave, char* err, size_t n) {
    (void) ssid; (void) clave; (void) err; (void) n;
    s_aviso(BPVM_WIFI_AV_CONECTADA, 0);
    bpvm_red_hay_ip(1);
    return 0;
}

static const bpvm_pack_sym_t SIMBOLOS[] = {
    { "encender", (bpvm_pack_fn_t) h_encender },
    { "apagar",   (bpvm_pack_fn_t) h_apagar },
    { "conectar", (bpvm_pack_fn_t) h_conectar },
};
static const bpvm_pack_api_t TABLA = { BPVM_WIFI_MARCA, BPVM_WIFI_API_VER, 3, SIMBOLOS };

void bpvm_wifi_register_host(void) {
    static const bpvm_wifi_cfg_t CFG = { "la red del PC", "", "", 0, 0 };
    bpvm_wifi_cfg_fija(&CFG);
    (void) bpvm_bios_publica(BPVM_WIFI_MARCA, &TABLA);
}
