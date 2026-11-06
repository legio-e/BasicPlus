/*
 * net_esp32.c — V8/H3 (6-nov): la IP de la familia ESP. Lo que dice /sys/net.json lo decide bpvm_red.c; aquí se le pasa a
 * `esp_netif`. El enlace no se toca: de esto a la radio sólo llega la interfaz de la estación, que lwIP conecta sola.
 *
 * `Net` (los sockets de los programas) sigue sin backend: `connect` a NULL, y Net.* dice «sin red en esta plataforma»
 * hasta que se haga. Esto es sólo la IP de la placa.
 */
#include "net_esp32.h"
#include "bpvm_net.h"
#include "bpvm_red.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include <stdio.h>
#include <string.h>

static esp_netif_t* s_sta = NULL;

/* En la tarea de eventos del IDF; bpvm_red_hay_ip escribe en el log, y por eso su pila va a 4 KB (sdkconfig.defaults). */
static void al_evento(void* arg, esp_event_base_t base, int32_t id, void* datos) {
    (void) arg; (void) base; (void) datos;
    if (id == IP_EVENT_STA_GOT_IP)       bpvm_red_hay_ip(1);
    else if (id == IP_EVENT_STA_LOST_IP) bpvm_red_hay_ip(0);
}

int net_esp32_preparar(char* err, size_t n) {
    esp_err_t r;
    if (s_sta) return 0;
    r = esp_netif_init();
    if (r != ESP_OK) { snprintf(err, n, "esp_netif_init: %s", esp_err_to_name(r)); return -1; }
    r = esp_event_loop_create_default();
    if (r != ESP_OK && r != ESP_ERR_INVALID_STATE) {
        snprintf(err, n, "el bucle de eventos: %s", esp_err_to_name(r));
        return -1;
    }
    r = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, al_evento, NULL);
    if (r != ESP_OK) { snprintf(err, n, "los eventos de la IP: %s", esp_err_to_name(r)); return -1; }
    s_sta = esp_netif_create_default_wifi_sta();
    if (!s_sta) { snprintf(err, n, "la interfaz de la estacion"); return -1; }
    return 0;
}

static int ip_dinamica(const char* nombre) {
    esp_err_t r;
    if (!s_sta) return -1;
    r = esp_netif_dhcpc_start(s_sta);   /* si venía de una IP fija */
    if (r != ESP_OK && r != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) return -1;
    return esp_netif_set_hostname(s_sta, nombre) == ESP_OK ? 0 : -1;
}

static int ip_fija(uint32_t ip, uint32_t mascara, uint32_t puerta, uint32_t dns) {
    esp_netif_ip_info_t i;
    esp_err_t r;
    if (!s_sta) return -1;
    r = esp_netif_dhcpc_stop(s_sta);
    if (r != ESP_OK && r != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) return -1;
    memset(&i, 0, sizeof i);
    i.ip.addr      = ip;
    i.netmask.addr = mascara;
    i.gw.addr      = puerta;
    if (esp_netif_set_ip_info(s_sta, &i) != ESP_OK) return -1;
    if (dns) {
        esp_netif_dns_info_t d;
        memset(&d, 0, sizeof d);
        d.ip.type = ESP_IPADDR_TYPE_V4;
        d.ip.u_addr.ip4.addr = dns;
        if (esp_netif_set_dns_info(s_sta, ESP_NETIF_DNS_MAIN, &d) != ESP_OK) return -1;
    }
    return 0;
}

static int ip_leer(uint32_t* ip, uint32_t* mascara, uint32_t* puerta, uint32_t* dns) {
    esp_netif_ip_info_t i;
    esp_netif_dns_info_t d;
    if (!s_sta || esp_netif_get_ip_info(s_sta, &i) != ESP_OK) return -1;
    memset(&d, 0, sizeof d);
    (void) esp_netif_get_dns_info(s_sta, ESP_NETIF_DNS_MAIN, &d);
    *ip      = i.ip.addr;
    *mascara = i.netmask.addr;
    *puerta  = i.gw.addr;
    *dns     = d.ip.u_addr.ip4.addr;
    return 0;
}

static const bpvm_net_backend_t BACKEND = {
    .ip_dinamica = ip_dinamica,
    .ip_fija     = ip_fija,
    .ip_leer     = ip_leer,
};

void net_esp32_registrar(void) { bpvm_net_set_backend(&BACKEND); }
