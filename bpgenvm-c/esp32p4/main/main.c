/*
 * BasicPlus — ESP32-P4 bring-up de red (canal de log Ethernet hacia el PC).
 *
 * Proyecto MÍNIMO, PRE-VM: valida la cadena Ethernet del P4 de punta a punta
 * antes de portar la VM. Hace:
 *   1) init de Ethernet (EMAC interno + PHY genérico IP101 por RMII) — misma
 *      config que el ejemplo oficial `ethernet/basic` que ya dio Link Up en
 *      esta placa (MDC=31, MDIO=52, PHY_ADDR=1, RST=51; RMII por defecto).
 *   2) IP estática 192.168.2.2/24, gw 192.168.2.1 (cable cruzado directo al PC,
 *      sin servidor DHCP).
 *   3) un cliente TCP que conecta a 192.168.2.1:5555 (el pc_logserver.py) y
 *      vuelca un "hello" + heartbeats → canal de traza de bring-up.
 *
 * Compila/flashea Eduardo (idf.py). El server del PC lo corre Claude.
 * sdkconfig.defaults ya trae el fix de revisión de silicon (P4 v1.0).
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

static const char *TAG = "bpvm_p4_net";

#define SERVER_IP    "192.168.2.1"
#define SERVER_PORT  5555
#define LINK_UP_BIT  BIT0

static EventGroupHandle_t s_events;

/* Init de Ethernet — réplica exacta de la del ejemplo que funcionó (los pines
 * van horneados; el ejemplo los sacaba de CONFIG_EXAMPLE_ETH_*). PHY genérico
 * (sirve para el IP101 de la placa). Interfaz RMII por defecto del P4. */
static esp_err_t eth_init(esp_eth_handle_t *out)
{
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 1;            /* sdkconfig: EXAMPLE_ETH_PHY_ADDR=1     */
    phy_config.reset_gpio_num = 51;     /* sdkconfig: EXAMPLE_ETH_PHY_RST_GPIO=51 */

    eth_esp32_emac_config_t emac_cfg = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    emac_cfg.smi_gpio.mdc_num  = 31;    /* sdkconfig: EXAMPLE_ETH_MDC_GPIO=31   */
    emac_cfg.smi_gpio.mdio_num = 52;    /* sdkconfig: EXAMPLE_ETH_MDIO_GPIO=52  */
    /* PHY_INTERFACE_DEFAULT en el ejemplo => NO tocamos interface/clock/data:
     * ETH_ESP32_EMAC_DEFAULT_CONFIG ya trae el RMII + pines por defecto del P4. */

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_cfg, &mac_config);
    if (mac == NULL) {
        ESP_LOGE(TAG, "MAC instance failed");
        return ESP_FAIL;
    }
    esp_eth_phy_t *phy = esp_eth_phy_new_generic(&phy_config);
    if (phy == NULL) {
        ESP_LOGE(TAG, "PHY instance failed");
        mac->del(mac);
        return ESP_FAIL;
    }
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    return esp_eth_driver_install(&config, out);
}

static void eth_event_handler(void *arg, esp_event_base_t base,
                              int32_t id, void *data)
{
    switch (id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        xEventGroupSetBits(s_events, LINK_UP_BIT);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        xEventGroupClearBits(s_events, LINK_UP_BIT);
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/* Con IP estática esto confirma en el monitor que la dirección quedó aplicada. */
static void got_ip_handler(void *arg, esp_event_base_t base,
                           int32_t id, void *data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) data;
    ESP_LOGI(TAG, "Got IP: " IPSTR "  gw " IPSTR,
             IP2STR(&event->ip_info.ip), IP2STR(&event->ip_info.gw));
}

/* Cliente TCP de log: espera el link, conecta al server del PC y manda
 * hello + heartbeats. Reconecta solo si el server no está o se cae. */
static void tcp_log_task(void *arg)
{
    xEventGroupWaitBits(s_events, LINK_UP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));   /* deja asentar la pila */

    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        struct sockaddr_in dest = {0};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(SERVER_PORT);
        dest.sin_addr.s_addr = inet_addr(SERVER_IP);

        if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) == 0) {
            ESP_LOGI(TAG, "TCP conectado a %s:%d", SERVER_IP, SERVER_PORT);
            const char *hello = "HELLO desde ESP32-P4 - BasicPlus bring-up Ethernet\n";
            send(sock, hello, strlen(hello), 0);
            int n = 0;
            while (1) {
                char line[96];
                int len = snprintf(line, sizeof(line),
                                   "heartbeat %d  uptime=%lld ms\n",
                                   n++, (long long)(esp_timer_get_time() / 1000));
                if (send(sock, line, len, 0) < 0) {
                    ESP_LOGW(TAG, "send fallo; reconecto");
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else {
            ESP_LOGW(TAG, "no conecta a %s:%d (server del PC arrancado?), reintento",
                     SERVER_IP, SERVER_PORT);
        }
        close(sock);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    s_events = xEventGroupCreate();

    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(eth_init(&eth_handle));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    esp_eth_netif_glue_handle_t glue = esp_eth_new_netif_glue(eth_handle);
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, glue));

    /* IP estática (cable directo al PC, sin DHCP). */
    esp_netif_dhcpc_stop(eth_netif);   /* puede dar "already stopped" -> ignorar */
    esp_netif_ip_info_t ip = {0};
    esp_netif_set_ip4_addr(&ip.ip,      192, 168, 2, 2);
    esp_netif_set_ip4_addr(&ip.gw,      192, 168, 2, 1);
    esp_netif_set_ip4_addr(&ip.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netif, &ip));

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                               &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                               &got_ip_handler, NULL));

    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    xTaskCreate(tcp_log_task, "tcp_log", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "P4 netlog: IP estatica 192.168.2.2 -> server %s:%d",
             SERVER_IP, SERVER_PORT);
}
