/*
 * BasicPlus — ESP32-P4: port de la VM-C (hito VM.1).
 *
 * Sobre el bring-up de red ya verificado (Ethernet IP101 + IP estática
 * 192.168.2.2 + cliente TCP a 192.168.2.1:5555), AHORA arrancamos el
 * núcleo de la VM-C y ejecutamos un Hello.mod EMBEBIDO. La salida de la
 * VM (y los checkpoints del arranque) salen por DOS canales:
 *   - TCP  → pc_logserver.py en el PC (canal de log que pidió Eduardo).
 *   - consola USB-Serial-JTAG → idf.py monitor.
 * Instrumentado a tope: si algo peta, lo vemos en vivo por el log.
 *
 * Compila/flashea Eduardo (idf.py). sdkconfig.defaults trae el fix de
 * revisión de silicon v1.0.
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
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

#include "bpvm.h"            /* núcleo de la VM-C (C99 portable) */

static const char *TAG = "bpvm_p4";

#define SERVER_IP    "192.168.2.1"
#define SERVER_PORT  5555
#define LINK_UP_BIT  BIT0

/* Hello.mod embebido (hello_mod.c, autogenerado). */
extern const uint8_t       hello_mod[];
extern const unsigned int  hello_mod_len;

/* Memoria de la VM (caller-provided). 128 KiB en RAM interna — de sobra
 * para el Hello; los programas grandes irán a PSRAM más adelante. */
#define VM_MEM_SIZE  (128 * 1024)
static uint8_t s_vm_mem[VM_MEM_SIZE];

static EventGroupHandle_t s_events;
static int s_sock = -1;     /* socket conectado (lo usa el output de la VM) */

/* Log instrumentado: a consola (idf.py monitor) Y al socket TCP. El caller
 * NO pone '\n' final; lo añade esta función para el lado TCP. */
static void net_logf(const char *fmt, ...)
{
    char buf[200];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf) - 2, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n > (int) sizeof(buf) - 2) n = sizeof(buf) - 2;
    ESP_LOGI(TAG, "%s", buf);
    buf[n++] = '\n';
    if (s_sock >= 0) send(s_sock, buf, n, 0);
}

/* Callback de salida de la VM (OPs de print) → consola + TCP, crudo. */
static void vm_out_cb(const char *s, size_t len, void *user)
{
    (void) user;
    if (len == 0) return;
    printf("%.*s", (int) len, s);
    if (s_sock >= 0) send(s_sock, s, len, 0);
}

/* ---- Ethernet (misma init verificada: IP101 / RMII, pines EV board) ---- */
static esp_err_t eth_init(esp_eth_handle_t *out)
{
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 1;
    phy_config.reset_gpio_num = 51;

    eth_esp32_emac_config_t emac_cfg = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    emac_cfg.smi_gpio.mdc_num  = 31;
    emac_cfg.smi_gpio.mdio_num = 52;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_cfg, &mac_config);
    if (mac == NULL) { ESP_LOGE(TAG, "MAC failed"); return ESP_FAIL; }
    esp_eth_phy_t *phy = esp_eth_phy_new_generic(&phy_config);
    if (phy == NULL) { ESP_LOGE(TAG, "PHY failed"); mac->del(mac); return ESP_FAIL; }
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
    case ETHERNET_EVENT_START: ESP_LOGI(TAG, "Ethernet Started"); break;
    case ETHERNET_EVENT_STOP:  ESP_LOGI(TAG, "Ethernet Stopped"); break;
    default: break;
    }
}

static void got_ip_handler(void *arg, esp_event_base_t base,
                           int32_t id, void *data)
{
    ip_event_got_ip_t *e = (ip_event_got_ip_t *) data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&e->ip_info.ip));
}

/* ---- Ejecuta el Hello.mod embebido en la VM-C, salida → TCP+consola ---- */
static void run_vm_hello(void)
{
    net_logf("[p4] === VM.2a: modulo de prueba embebido (%u bytes) ===",
             hello_mod_len);

    bpvm_t *vm = bpvm_init(s_vm_mem, VM_MEM_SIZE, 0);
    if (!vm) { net_logf("[p4] ERROR bpvm_init (mem=%d)", VM_MEM_SIZE); return; }
    net_logf("[p4] bpvm_init OK (mem=%d KiB)", VM_MEM_SIZE / 1024);

    bpvm_set_output(vm, vm_out_cb, NULL);

    bpvm_status_t st = bpvm_load_mod_buffer(vm, hello_mod, hello_mod_len, "StressP4");
    net_logf("[p4] load_mod_buffer -> %s", bpvm_status_str(st));

    if (st == BPVM_OK) {
        net_logf("[p4] --- salida de la VM ---");
        st = bpvm_run(vm);
        net_logf("[p4] --- fin VM, status=%s ---", bpvm_status_str(st));
    }
    bpvm_destroy(vm);
    net_logf("[p4] === VM destruida. Si has leido el Hello, VM.1 OK ===");
}

static void tcp_log_task(void *arg)
{
    xEventGroupWaitBits(s_events, LINK_UP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));

    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) { vTaskDelay(pdMS_TO_TICKS(2000)); continue; }
        struct sockaddr_in dest = {0};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(SERVER_PORT);
        dest.sin_addr.s_addr = inet_addr(SERVER_IP);

        if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) == 0) {
            s_sock = sock;
            net_logf("[p4] TCP conectado a %s:%d", SERVER_IP, SERVER_PORT);

            run_vm_hello();          /* <-- el hito: ejecutar la VM-C */

            /* heartbeats para confirmar que el firmware sigue vivo tras la VM */
            int n = 0;
            while (1) {
                net_logf("[p4] idle %d uptime=%lld ms", n++,
                         (long long)(esp_timer_get_time() / 1000));
                vTaskDelay(pdMS_TO_TICKS(3000));
                int probe = send(sock, "", 0, 0);
                if (probe < 0) break;
            }
            s_sock = -1;
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

    esp_netif_dhcpc_stop(eth_netif);
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

    /* La VM corre dentro de esta task → stack holgado (16 KiB). */
    xTaskCreate(tcp_log_task, "tcp_log", 16384, NULL, 5, NULL);

    ESP_LOGI(TAG, "P4 VM.1: IP 192.168.2.2 -> log/VM por %s:%d", SERVER_IP, SERVER_PORT);
}
